// Procedural skeletal poses for Quaternius-rigged characters (model faces +Z, left side +X, Y up).
// Rotations are model-space deltas applied at each bone's head and carried by the parent's motion.
#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <cstring>
#include <cmath>

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
static void solvePose(const Rig& r, const std::vector<Quaternion>& D, const std::vector<float>& S, Vector3 rootOff, Transform* out) {
    int n = (int)r.bind.size();
    for (int i = 0; i < n; i++) {
        const Transform& b = r.bind[i]; int p = r.parent[i];
        if (p < 0) { out[i] = {Vector3Add(b.translation, rootOff), QuaternionMultiply(D[i], b.rotation), {S[i], S[i], S[i]}}; continue; }
        Quaternion pd = QuaternionMultiply(out[p].rotation, QuaternionInvert(r.bind[p].rotation));
        float ps = out[p].scale.x;
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

enum PoseKind { PK_PLAYER, PK_ZOMBIE, PK_RUNNER, PK_BRUTE, PK_CORPSE };
struct PoseIn {
    int kind = PK_PLAYER; float phase = 0, move = 0; // move: 0 idle .. 1 walk .. 2 sprint
    int hold = 0;          // player: 0 empty/melee, 1 pistol, 2 long gun
    float crouch = 0, swing = -1, attack = -1, roll = -1, stagger = 0, hurt = 0, recoil = 0;
    bool armL = true, armR = true, head = true; float seed = 0;
    float twist = 0; // player: upper body turned toward the aim while the legs follow the movement
    float kick = -1; bool guard = false;
};
// Builds the pose. Returns root offset (for crouch/bob). Corpse pose expects the caller to lay the model down.
static Vector3 buildPose(const Rig& r, const PoseIn& in, std::vector<Quaternion>& D, std::vector<float>& S) {
    int n = (int)r.bind.size(); D.assign(n, QuaternionIdentity()); S.assign(n, 1.f);
    auto set = [&](int b, Quaternion q) { if (b >= 0) D[b] = q; };
    float ph = in.phase, mv = in.move, s = sinf(ph), c = cosf(ph);
    Vector3 root = {0, 0, 0};
    float armDown = 1.3f;
    if (in.kind == PK_CORPSE) {
        float k = in.seed;
        set(r.ua[0], QM(QY(-.4f - k * .5f), QZ(-.4f))); set(r.ua[1], QM(QY(.3f + k * .6f), QZ(.5f)));
        set(r.la[0], QY(-.6f * k)); set(r.la[1], QY(.5f));
        set(r.ul[0], QM(QZ(.15f), QX(-.25f * k))); set(r.ul[1], QM(QZ(-.2f), QX(.2f))); set(r.ll[0], QX(.5f * k)); set(r.ll[1], QX(.2f));
        set(r.neck, QY(.6f * (k - .5f)));
    } else if (in.kind == PK_PLAYER) {
        float stride = mv > 1.2f ? .8f : .45f * std::min(mv, 1.f), cr = in.crouch;
        set(r.ul[0], QX(-s * stride - cr * 1.1f)); set(r.ul[1], QX(s * stride - cr * 1.1f));
        set(r.ll[0], QX(std::max(0.f, -c) * stride * 1.3f + cr * 1.8f + .05f)); set(r.ll[1], QX(std::max(0.f, c) * stride * 1.3f + cr * 1.8f + .05f));
        set(r.foot[0], QX(-cr * .6f)); set(r.foot[1], QX(-cr * .6f));
        root.y = -cr * .38f - fabsf(c) * .03f * std::min(mv, 1.6f);
        float lean = (mv > 1.2f ? .22f : .05f * mv) + cr * .35f; set(r.torso, QX(lean));
        float sw = s * stride * .9f;
        if (in.hold == 0 && in.swing < 0) { set(r.ua[0], QM(QX(sw), QZ(-armDown))); set(r.ua[1], QM(QX(-sw), QZ(armDown))); set(r.la[0], QY(-.2f - std::max(0.f, sw) * .6f)); set(r.la[1], QY(.2f + std::max(0.f, -sw) * .6f)); }
        if (in.hold == 1) { // pistol, two hands forward
            float rk = in.recoil * .35f;
            set(r.ua[1], QM(QX(-.05f - rk), QY(1.45f))); set(r.la[1], QY(.05f));
            set(r.ua[0], QM(QX(-.05f - rk), QY(-1.25f))); set(r.la[0], QY(-.55f));
        }
        if (in.hold == 2) { // long gun at shoulder
            float rk = in.recoil * .2f;
            set(r.ua[1], QM(QX(.25f - rk), QM(QY(1.05f), QZ(.25f)))); set(r.la[1], QY(.85f));
            set(r.ua[0], QM(QX(-.1f - rk), QY(-1.35f))); set(r.la[0], QY(-.25f));
            set(r.chest, QY(.15f));
        }
        if (in.swing >= 0) { // melee: right arm overhead to across
            float t = in.swing, a = -1.8f + 3.2f * t;
            set(r.ua[1], QM(QY(.9f + a * .55f), QM(QX(-.7f), QZ(.2f + (1 - t) * 1.2f)))); set(r.la[1], QY(.6f - t * .5f));
            set(r.ua[0], QM(QX(sw), QZ(-armDown))); set(r.chest, QY(-.5f + t * 1.0f)); set(r.torso, QX(.12f));
        }
        if (in.roll >= 0) { set(r.ul[0], QX(-1.6f)); set(r.ul[1], QX(-1.6f)); set(r.ll[0], QX(2.0f)); set(r.ll[1], QX(2.0f)); set(r.torso, QX(.9f)); set(r.ua[0], QM(QX(-1.f), QZ(-1.f))); set(r.ua[1], QM(QX(-1.f), QZ(1.f))); root.y = -.45f; }
        set(r.neck, QX(-lean * .5f));
        if (in.kick >= 0) { float k = sinf(in.kick * PI); set(r.ul[1], QX(-1.55f * k)); set(r.ll[1], QX(.25f * (1 - k))); set(r.ul[0], QX(.15f * k)); set(r.torso, QX(-.25f * k)); root.y = -.04f * k; }
        if (in.guard && in.hold == 0) { set(r.ua[0], QM(QX(-.35f), QY(-1.35f))); set(r.la[0], QY(-1.1f)); set(r.ua[1], QM(QX(-.25f), QY(1.2f))); set(r.la[1], QY(1.0f)); }
        if (in.twist != 0) { float t = std::max(-1.4f, std::min(1.4f, in.twist)); if (r.abdomen >= 0) D[r.abdomen] = QM(QY(-t * .35f), D[r.abdomen]); if (r.torso >= 0) D[r.torso] = QM(QY(-t * .35f), D[r.torso]); if (r.chest >= 0) D[r.chest] = QM(QY(-t * .3f), D[r.chest]); }
    } else { // zombies
        bool run = in.kind == PK_RUNNER, brute = in.kind == PK_BRUTE;
        float stride = run ? .85f : brute ? .35f : .3f, lean = run ? .45f : brute ? .3f : .22f;
        float lim = run ? 0 : sinf(ph * .5f + in.seed) * .08f;
        set(r.ul[0], QX(-s * stride * std::min(mv, 1.5f) + lim)); set(r.ul[1], QX(s * stride * std::min(mv, 1.5f) - lim * .5f));
        set(r.ll[0], QX(std::max(0.f, -c) * stride * 1.4f + .15f)); set(r.ll[1], QX(std::max(0.f, c) * stride * 1.4f + (run ? .15f : .35f)));
        root.y = -fabsf(c) * (run ? .06f : .025f) - (run ? .05f : .03f);
        set(r.torso, QM(QX(lean + in.stagger * .4f), QZ(sinf(ph * .5f + in.seed) * (run ? .05f : .12f))));
        set(r.neck, QM(QX(-.15f), QZ(sinf(in.seed * 3) * .35f)));
        set(r.head, QY(sinf(in.seed * 5 + ph * .3f) * .2f));
        if (run) { float a = s * 1.1f; set(r.ua[0], QM(QX(a), QZ(-1.15f))); set(r.ua[1], QM(QX(-a), QZ(1.15f))); set(r.la[0], QY(-1.1f)); set(r.la[1], QY(1.1f)); }
        else {
            float reach = brute ? .9f : 1.0f, wob = sinf(ph * .7f + in.seed) * .12f;
            float atk = in.attack >= 0 ? sinf(in.attack * PI) : 0;
            set(r.ua[0], QM(QX(.25f - atk * .6f + wob), QY(-1.45f * reach))); set(r.ua[1], QM(QX(.32f - atk * .9f - wob), QY(1.4f * reach)));
            set(r.la[0], QY(-.25f + atk * .3f)); set(r.la[1], QY(.3f - atk * .3f));
            if (atk > 0) set(r.chest, QM(QX(.2f * atk), QY(-.3f * atk)));
        }
    }
    if (!in.armL && r.ua[0] >= 0) S[r.la[0]] = 0;
    if (!in.armR && r.ua[1] >= 0) S[r.la[1]] = 0;
    if (!in.head && r.head >= 0) S[r.head] = 0;
    if (in.hurt > 0) set(r.chest, QM(D[r.chest >= 0 ? r.chest : 0], QX(-in.hurt * .3f)));
    return root;
}
