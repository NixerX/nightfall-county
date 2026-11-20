// Procedural skeletal poses for Quaternius-rigged characters (model faces +Z, left side +X, Y up).
// Rotations are model-space deltas applied at each bone's head and carried by the parent's motion.
#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>

struct Rig {
    std::vector<int> parent; std::vector<Transform> bind; std::vector<std::string> names;
    int hips = -1, abdomen = -1, torso = -1, chest = -1, neck = -1, head = -1, ua[2] = {-1, -1}, la[2] = {-1, -1}, hand[2] = {-1, -1}, ul[2] = {-1, -1}, ll[2] = {-1, -1}, foot[2] = {-1, -1}, sh[2] = {-1, -1};
    float hipY = .9f;
};
static int rigBone(const Rig& r, const char* n) { for (size_t i = 0; i < r.names.size(); i++) if (r.names[i] == n) return (int)i; return -1; }
static Rig makeRig(Model& m) {
    Rig r; for (int i = 0; i < m.boneCount; i++) { m.bindPose[i].scale = {1, 1, 1}; r.parent.push_back(m.bones[i].parent); r.bind.push_back(m.bindPose[i]); r.names.push_back(m.bones[i].name); }
    r.hips = rigBone(r, "Hips"); r.abdomen = rigBone(r, "Abdomen"); r.torso = rigBone(r, "Torso"); r.chest = rigBone(r, "Chest"); r.neck = rigBone(r, "Neck"); r.head = rigBone(r, "Head");
    const char* sfx[2] = {".L", ".R"};
    for (int s = 0; s < 2; s++) {
        r.ua[s] = rigBone(r, ("UpperArm" + std::string(sfx[s])).c_str()); r.la[s] = rigBone(r, ("LowerArm" + std::string(sfx[s])).c_str()); r.hand[s] = rigBone(r, ("Hand" + std::string(sfx[s])).c_str());
        r.ul[s] = rigBone(r, ("UpperLeg" + std::string(sfx[s])).c_str()); r.ll[s] = rigBone(r, ("LowerLeg" + std::string(sfx[s])).c_str()); r.foot[s] = rigBone(r, ("Foot" + std::string(sfx[s])).c_str());
        r.sh[s] = rigBone(r, ("Shoulder" + std::string(sfx[s])).c_str());
    }
    if (r.hips >= 0) r.hipY = r.bind[r.hips].translation.y;
    return r;
}
// FK: D = model-space delta rotation per bone, S = scale factor per bone (0 collapses a limb), rootOff moves the hips.
// local = true: a bone's scale doesn't pass to its children (collapse a parent, keep a limb: body-part gibs)
static void solvePose(const Rig& r, const std::vector<Quaternion>& D, const std::vector<float>& S, Vector3 rootOff, Transform* out, bool local = false) {
    int n = (int)r.bind.size();
    for (int i = 0; i < n; i++) {
        const Transform& b = r.bind[i]; int p = r.parent[i];
        if (p < 0) { out[i] = {Vector3Add(b.translation, rootOff), QuaternionMultiply(D[i], b.rotation), {S[i], S[i], S[i]}}; continue; }
        Quaternion pd = QuaternionMultiply(out[p].rotation, QuaternionInvert(r.bind[p].rotation));
        float ps = local ? 1.f : out[p].scale.x;
        Vector3 off = Vector3Scale(Vector3RotateByQuaternion(Vector3Subtract(b.translation, r.bind[p].translation), pd), ps);
        Vector3 pos = Vector3Add(out[p].translation, off);
        if (i == r.hips) pos = Vector3Add(pos, rootOff);
        Quaternion dw = QuaternionMultiply(QuaternionMultiply(pd, D[i]), QuaternionInvert(pd));
        float s = ps * S[i];
        out[i] = {pos, QuaternionMultiply(dw, QuaternionMultiply(pd, b.rotation)), {s, s, s}};
    }
}
static Quaternion QX(float a) { return QuaternionFromAxisAngle({1, 0, 0}, a); }
static Quaternion QY(float a) { return QuaternionFromAxisAngle({0, 1, 0}, a); }
static Quaternion QZ(float a) { return QuaternionFromAxisAngle({0, 0, 1}, a); }
static Quaternion QM(Quaternion a, Quaternion b) { return QuaternionMultiply(a, b); } // apply b first, then a
static float pclamp(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static float max0(float v) { return v > 0 ? v : 0; }
static float plerp(float a, float b, float t) { return a + (b - a) * t; }

// Axes (model space): +Z forward, +X the character's left, +Y up.
//   QX(+) tips a bone's far end backward when it hangs down (legs, arms) and tips the torso forward.
//   QZ(+) tips the torso toward its right (-X). For a T-posed arm, QZ(+) lifts the left arm, QZ(-) lifts the right.
// Gait phase: leg 0 (left) is forward when sin(phase) > 0 and swings forward (foot in the air) when cos(phase) > 0.
enum PoseKind { PK_PLAYER, PK_ZOMBIE, PK_RUNNER, PK_BRUTE, PK_CORPSE, PK_CRAWL };
struct PoseIn {
    int kind = PK_PLAYER; float phase = 0, move = 0; // move: 0 idle .. 1 walk .. ~1.9 sprint (speed / walking speed)
    int hold = 0;          // player: 0 empty/melee, 1 pistol, 2 long gun
    float crouch = 0, swing = -1, attack = -1, roll = -1, stagger = 0, hurt = 0, recoil = 0;
    bool armL = true, armR = true, head = true; float seed = 0;
    float twist = 0; // player: upper body turned toward the aim while the legs follow the movement
    float kick = -1; bool guard = false;
    float time = 0;           // seconds: idle breathing, weight shift, head turns
    float turn = 0;           // yaw rate in rad/s (+ = turning toward +angle): lean into the turn
    float hitX = 0, hitZ = 0; // knock: model-space direction the upper body is shoved (x left, z forward), length = strength
    float alert = 1;          // zombies: 0 wandering (arms hang) .. 1 locked on (arms reach)
    float drag = 0;           // zombies: dragged right foot 0..1
    float hp = 1;             // health fraction: badly hurt zombies reel
    float pound = -1;         // zombies: banging a door, 0..1 through each blow
    signed char cut[4] = {0, 0, 0, 0}; // arm L, arm R, leg L, leg R: 0 whole, 1 lost below the joint, 2 lost at the root
    bool noUpper = false;     // everything above the hips is gone
    bool useCache = false;    // draw the last blended pose (shadow pass) instead of building a new one
};

// Pose blending (inertialization). The target pose is rebuilt every frame; when a bone's target jumps (a swing
// ends, a gun comes up, a zombie spots you) the jump goes into a per-bone offset that eases out, so nothing pops.
// Continuous motion (walk cycles) passes straight through, so strides keep their full size.
struct PoseState {
    std::vector<Quaternion> T, O, D; std::vector<float> S;
    Vector3 rT{0, 0, 0}, rO{0, 0, 0}, root{0, 0, 0};
    unsigned frame = 0; bool init = false;
};
static float qAngle(Quaternion q) { float w = fabsf(q.w); return 2 * acosf(w > 1 ? 1 : w); }
static void blendPose(PoseState& ps, const std::vector<Quaternion>& D, const std::vector<float>& S, Vector3 root, float dt, float rate) {
    size_t n = D.size();
    if (!ps.init || ps.T.size() != n) {
        ps.T = D; ps.O.assign(n, QuaternionIdentity()); ps.rT = root; ps.rO = {0, 0, 0}; ps.init = true;
    } else {
        float thr = .05f + dt * 12, k = expf(-dt * rate);
        for (size_t i = 0; i < n; i++) {
            if (qAngle(QuaternionMultiply(D[i], QuaternionInvert(ps.T[i]))) > thr) {
                Quaternion out = QuaternionMultiply(ps.O[i], ps.T[i]);
                ps.O[i] = QuaternionMultiply(out, QuaternionInvert(D[i]));
            }
            ps.O[i] = QuaternionSlerp(QuaternionIdentity(), ps.O[i], k);
            ps.T[i] = D[i];
        }
        if (Vector3Distance(root, ps.rT) > .02f + dt * 1.5f) ps.rO = Vector3Subtract(Vector3Add(ps.rO, ps.rT), root);
        ps.rO = Vector3Scale(ps.rO, k);
        ps.rT = root;
    }
    ps.D.resize(n);
    for (size_t i = 0; i < n; i++) ps.D[i] = QuaternionMultiply(ps.O[i], ps.T[i]);
    ps.S = S;
    ps.root = Vector3Add(root, ps.rO);
}

// Builds the pose. Returns root offset (for crouch/bob). Corpse and crawl poses expect the caller to lay the model down.
static Vector3 buildPose(const Rig& r, const PoseIn& in, std::vector<Quaternion>& D, std::vector<float>& S) {
    int n = (int)r.bind.size(); D.assign(n, QuaternionIdentity()); S.assign(n, 1.f);
    auto set = [&](int b, Quaternion q) { if (b >= 0) D[b] = q; };
    auto add = [&](int b, Quaternion q) { if (b >= 0) D[b] = QM(q, D[b]); }; // layer on top of what's there
    float ph = in.phase, mv = in.move, s = sinf(ph), c = cosf(ph), t = in.time, sd = in.seed;
    Vector3 root = {0, 0, 0};
    float armDown = 1.3f;
    float walkK = pclamp(mv, 0, 1), idleK = 1 - walkK;
    if (in.kind == PK_CORPSE) {
        float k = in.seed - floorf(in.seed);
        set(r.ua[0], QM(QY(-.4f - k * .5f), QZ(-.4f))); set(r.ua[1], QM(QY(.3f + k * .6f), QZ(.5f)));
        set(r.la[0], QY(-.6f * k)); set(r.la[1], QY(.5f));
        set(r.ul[0], QM(QZ(.15f), QX(-.25f * k))); set(r.ul[1], QM(QZ(-.2f), QX(.2f))); set(r.ll[0], QX(.5f * k)); set(r.ll[1], QX(.2f));
        set(r.neck, QY(.6f * (k - .5f)));
    } else if (in.kind == PK_CRAWL) { // lying face down, head forward: arms reach and pull in turn, legs drag
        float a0 = .5f + .5f * s, a1 = .5f - .5f * s; // 1 = reaching out ahead, 0 = pulled back under the chest
        set(r.ua[0], QM(QX(-.25f * a0), QZ(plerp(.25f, 1.45f, a0)))); set(r.la[0], QY(plerp(-1.2f, -.15f, a0)));
        set(r.ua[1], QM(QX(-.25f * a1), QZ(-plerp(.25f, 1.45f, a1)))); set(r.la[1], QY(plerp(1.2f, .15f, a1)));
        set(r.chest, QZ(s * .12f)); set(r.torso, QZ(-s * .08f));
        set(r.neck, QX(-.55f)); set(r.head, QM(QX(-.25f), QY(sinf(t * .7f + sd) * .25f)));
        set(r.ul[0], QM(QZ(.1f), QX(.1f + c * .08f))); set(r.ul[1], QM(QZ(-.12f), QX(.15f - c * .08f)));
        set(r.ll[0], QX(.3f + max0(c) * .4f)); set(r.ll[1], QX(.2f + max0(-c) * .4f));
        set(r.foot[0], QX(.5f)); set(r.foot[1], QX(.5f));
        root.y = fabsf(s) * .03f;
    } else if (in.kind == PK_PLAYER) {
        float runK = pclamp((mv - 1.1f) / .7f, 0, 1), cr = in.crouch;
        // crouch-walking is half speed: measure the stride against crouch speed so the legs still really step
        walkK = pclamp(mv / (1 - cr * .5f), 0, 1); idleK = 1 - walkK;
        float stride = walkK * plerp(.5f, .85f, runK) * (1 - cr * .1f);
        float ul0 = -s * stride - cr * 1.1f, ul1 = s * stride - cr * 1.1f;
        float knee = plerp(1.25f, 1.75f, runK);
        float ll0 = max0(c) * stride * knee + .06f + cr * 1.8f, ll1 = max0(-c) * stride * knee + .06f + cr * 1.8f;
        set(r.ul[0], QX(ul0)); set(r.ul[1], QX(ul1)); set(r.ll[0], QX(ll0)); set(r.ll[1], QX(ll1));
        // feet: roughly level with the ground, toes up as the heel strikes, toes down as it pushes off
        float f0 = -(ul0 + ll0) * .75f - .22f * max0(s) * walkK + .32f * max0(-s) * max0(c) * walkK;
        float f1 = -(ul1 + ll1) * .75f - .22f * max0(-s) * walkK + .32f * max0(s) * max0(-c) * walkK;
        set(r.foot[0], QX(f0 - cr * .2f)); set(r.foot[1], QX(f1 - cr * .2f));
        // hips turn with the stride, shoulders turn against them; the weight rolls onto the planted foot
        float hipT = s * walkK * plerp(.1f, .16f, runK);
        set(r.hips, QM(QY(-hipT), QZ(c * .035f * walkK + sinf(t * .33f + sd) * .03f * idleK)));
        root.x = -c * .025f * walkK * (1 - runK * .6f) + sinf(t * .33f + sd) * .022f * idleK;
        // bob: lowest with both feet down (legs spread), highest passing over the planted foot
        root.y = -cr * .38f - fabsf(s) * plerp(.03f, .06f, runK) * walkK - runK * .03f;
        float lean = .04f * walkK + .2f * runK + cr * .35f;
        float turnLean = pclamp(in.turn * .035f * walkK * (1 + runK), -.22f, .22f);
        set(r.abdomen, QY(hipT * .5f));
        set(r.torso, QM(QZ(turnLean), QX(lean)));
        set(r.chest, QM(QY(hipT * .9f), QX(sinf(t * 1.7f) * .022f * (1 + runK))));  // + breathing
        float sw = s * plerp(.6f, .95f, runK) * walkK * (1 - cr * .5f);
        if (in.hold == 0 && in.swing < 0) {
            float hang = 1.42f - runK * .3f;
            float elbow = .2f + runK * 1.0f;
            set(r.ua[0], QM(QX(sw), QZ(-hang + sinf(t * 1.7f) * .02f))); set(r.ua[1], QM(QX(-sw), QZ(hang - sinf(t * 1.7f) * .02f)));
            set(r.la[0], QY(-elbow - max0(sw) * .6f)); set(r.la[1], QY(elbow + max0(-sw) * .6f));
        }
        float gb = sinf(ph * 2) * .035f * walkK; // weapon bob with each step
        if (in.hold == 1) { // pistol, two hands forward
            float rk = in.recoil * .35f;
            set(r.ua[1], QM(QX(-.05f - rk + gb), QY(1.45f))); set(r.la[1], QY(.05f + rk * .4f));
            set(r.ua[0], QM(QX(-.05f - rk + gb), QY(-1.25f))); set(r.la[0], QY(-.55f - rk * .3f));
        }
        if (in.hold == 2) { // long gun at shoulder
            float rk = in.recoil * .2f;
            set(r.ua[1], QM(QX(.25f - rk + gb), QM(QY(1.05f), QZ(.25f)))); set(r.la[1], QY(.85f));
            set(r.ua[0], QM(QX(-.1f - rk + gb), QY(-1.35f))); set(r.la[0], QY(-.25f));
            add(r.chest, QM(QY(.15f), QX(-rk * .5f)));
        }
        if (in.swing >= 0) { // melee: right arm overhead to across, body follows through
            float tt = in.swing, a = -1.8f + 3.2f * tt;
            set(r.ua[1], QM(QY(.9f + a * .55f), QM(QX(-.7f), QZ(.2f + (1 - tt) * 1.2f)))); set(r.la[1], QY(.6f - tt * .5f));
            set(r.ua[0], QM(QX(sw), QZ(-armDown))); set(r.chest, QY(-.5f + tt * 1.0f)); set(r.torso, QX(.12f + sinf(tt * PI) * .12f));
            add(r.hips, QY(-.25f + tt * .4f));
        }
        if (in.roll >= 0) { set(r.ul[0], QX(-1.6f)); set(r.ul[1], QX(-1.6f)); set(r.ll[0], QX(2.0f)); set(r.ll[1], QX(2.0f)); set(r.torso, QX(.9f)); set(r.ua[0], QM(QX(-1.f), QZ(-1.f))); set(r.ua[1], QM(QX(-1.f), QZ(1.f))); set(r.hips, QuaternionIdentity()); root = {0, -.45f, 0}; }
        set(r.neck, QM(QY(sinf(t * .21f + sd) * .14f * idleK), QX(-lean * .5f)));
        if (in.kick >= 0) { float k = sinf(in.kick * PI); set(r.ul[1], QX(-1.55f * k)); set(r.ll[1], QX(.25f * (1 - k))); set(r.ul[0], QX(.15f * k)); set(r.ll[0], QX(.15f * k)); set(r.torso, QX(-.25f * k)); root.y = -.04f * k; }
        if (in.guard && in.hold == 0) { set(r.ua[0], QM(QX(-.35f), QY(-1.35f))); set(r.la[0], QY(-1.1f)); set(r.ua[1], QM(QX(-.25f), QY(1.2f))); set(r.la[1], QY(1.0f)); }
        if (in.twist != 0) { float tw = pclamp(in.twist, -1.4f, 1.4f); add(r.abdomen, QY(-tw * .35f)); add(r.torso, QY(-tw * .35f)); add(r.chest, QY(-tw * .3f)); }
    } else { // zombies
        bool run = in.kind == PK_RUNNER, brute = in.kind == PK_BRUTE;
        float h = pclamp(in.hp, 0, 1), reel = pclamp(1 - h / .3f, 0, 1); // under 30% health they reel
        float drag = run ? 0 : in.drag;
        float stride = (run ? .6f : brute ? .38f : .32f) * pclamp(mv, 0, 1.5f);
        float lean = run ? .45f : brute ? .3f : .22f;
        float lim = run ? 0 : sinf(ph * .5f + sd) * .08f;
        float cr = in.crouch;
        float ul0 = -s * stride + lim - cr * 1.0f, ul1 = s * stride * (1 - drag * .55f) - lim * .5f - cr * 1.0f;
        float ll0 = max0(c) * stride * 1.4f + .15f + cr * 1.7f, ll1 = max0(-c) * stride * 1.4f * (1 - drag * .6f) + (run ? .15f : .35f) + drag * .15f + cr * 1.7f;
        set(r.ul[0], QX(ul0)); set(r.ul[1], QM(QZ(-drag * .12f), QX(ul1))); set(r.ll[0], QX(ll0)); set(r.ll[1], QX(ll1));
        set(r.foot[0], QX(-(ul0 + ll0) * .6f)); set(r.foot[1], QX(-(ul1 + ll1) * .6f + drag * .7f)); // dragged foot: toes scraping
        float roll = brute ? s * .1f * walkK : 0; // heavy side-to-side
        set(r.hips, QM(QY(-s * stride * .25f), QZ(roll + drag * .06f * s - c * .03f * walkK)));
        root.y = -fabsf(s) * (run ? .07f : brute ? .06f : .03f) * walkK - (run ? .05f : .03f) - cr * .35f - drag * .02f;
        root.x = -c * (brute ? .05f : .02f) * walkK;
        float sway = sinf(ph * .5f + sd) * (run ? .05f : .12f) * walkK + sinf(t * .8f + sd) * .07f * idleK + sinf(t * 2.3f + sd) * .14f * reel;
        float turnLean = pclamp(in.turn * (run ? .05f : .03f), -.25f, .25f);
        set(r.torso, QM(QZ(sway + turnLean - drag * .1f), QX(lean + in.stagger * .4f + reel * .15f)));
        set(r.neck, QM(QX(-.15f + idleK * .3f * (1 - in.alert)), QZ(sinf(sd * 3) * .35f + sinf(t * .5f + sd) * .25f * idleK)));
        set(r.head, QY(sinf(sd * 5 + ph * .3f + t * .2f) * .2f));
        set(r.chest, QX(sinf(t * 1.3f + sd) * .025f)); // ragged breathing
        float atk = in.attack >= 0 ? sinf(in.attack * PI) : 0;
        if (run) {
            float fl = sinf(t * 7.3f + sd * 13) * .35f, fl2 = sinf(t * 6.1f + sd * 7) * .35f; // flailing
            float a = s * 1.1f;
            set(r.ua[0], QM(QX(a + fl - atk * .9f), QZ(-1.15f + fl2 * .5f))); set(r.ua[1], QM(QX(-a - fl2 - atk * .9f), QZ(1.15f + fl * .5f)));
            set(r.la[0], QY(-1.1f + fl2 * .4f)); set(r.la[1], QY(1.1f - fl * .4f));
        } else {
            float reach = brute ? .9f : 1.0f, wob = sinf(ph * .7f + sd) * .12f;
            float al = pclamp(in.alert, 0, 1);
            float pnd = in.pound >= 0 ? in.pound : -1;
            if (pnd >= 0) atk = sinf(pnd * PI);
            Quaternion reachL = QM(QX(.25f - atk * .6f + wob), QY(-1.45f * reach)), reachR = QM(QX(.32f - atk * .9f - wob), QY(1.4f * reach));
            float hs = sinf(ph * .5f + sd) * .12f * walkK + sinf(t * .9f + sd) * .05f; // arms dangle and swing loosely
            Quaternion hangL = QM(QX(.12f + hs + s * .15f * walkK), QZ(-armDown * .95f)), hangR = QM(QX(.08f - hs - s * .15f * walkK), QZ(armDown * .95f));
            if (pnd >= 0) { // hammering a door: both fists up and down
                float u = sinf(pnd * PI);
                reachL = QM(QX(-.9f + u * 1.1f), QY(-1.3f)); reachR = QM(QX(-1.0f + u * 1.2f), QY(1.3f));
                al = 1;
            }
            set(r.ua[0], QuaternionSlerp(hangL, reachL, al)); set(r.ua[1], QuaternionSlerp(hangR, reachR, al));
            set(r.la[0], QY(plerp(-.15f, -.25f + atk * .3f, al))); set(r.la[1], QY(plerp(.15f, .3f - atk * .3f, al)));
            if (atk > 0) add(r.chest, QM(QX(.2f * atk), QY(-.3f * atk)));
        }
    }
    if (in.kind != PK_CORPSE && in.kind != PK_CRAWL) { // knocked by a hit: the upper body goes with it
        float hz = in.hitZ, hx = in.hitX;
        if (hz == 0 && hx == 0 && in.hurt > 0) hz = -in.hurt * .7f;
        if (hz != 0 || hx != 0) { add(r.chest, QM(QX(hz * .3f), QZ(-hx * .3f))); add(r.torso, QM(QX(hz * .25f), QZ(-hx * .25f))); add(r.neck, QX(-hz * .2f)); }
    }
    if (!in.armL && r.la[0] >= 0) S[r.la[0]] = 0;
    if (!in.armR && r.la[1] >= 0) S[r.la[1]] = 0;
    for (int k = 0; k < 2; k++) {
        if (in.cut[k] == 1 && r.la[k] >= 0) S[r.la[k]] = 0;
        if (in.cut[k] == 2 && r.ua[k] >= 0) S[r.ua[k]] = 0;
        if (in.cut[2 + k] == 1 && r.ll[k] >= 0) S[r.ll[k]] = 0;
        if (in.cut[2 + k] == 2 && r.ul[k] >= 0) S[r.ul[k]] = 0;
    }
    if (!in.head && r.head >= 0) S[r.head] = 0;
    if (in.noUpper && r.abdomen >= 0) S[r.abdomen] = 0;
    return root;
}
