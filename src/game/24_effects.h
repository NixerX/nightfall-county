// Nightfall County: effects.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ effects
static const Color BLOOD_DRY = rgba(58, 5, 8, 235), BLOOD_FRESH = rgba(120, 12, 18), ACID_C = rgba(120, 165, 40, 220);
static void floatText(Vector3 p, const std::string& t, Color c, float size = 15) {
    if (floaters.size() > 60)
        floaters.erase(floaters.begin());
    floaters.push_back({p, t, c, size, 1.2f, 1.2f});
}
static float goreK() { // the Gore setting scales how much of everything
    return cfg.gore == 2 ? 1.6f : cfg.gore == 0 ? .8f : 1.f;
}
static bool isBloodC(Color c) {
    return c.r > 40 && c.g < 40 && c.b < 40;
}
static void pushDecal(const Decal& d) {
    if ((int)decals.size() < 6000)
        decals.push_back(d);
    else {
        decals[decalHead] = d;
        decalHead = (decalHead + 1) % 6000;
    }
}
static void decal(Vector2 p, float r, Color c, float grow = 0) {
    Decal d{p, r, frand(0, 2 * PI), 1, irand(0, 3), c, grow, (buildingAt(p) >= 0 ? .08f : .05f) + frand(0, .004f)};
    d.born = gTime;
    d.blood = isBloodC(c);
    pushDecal(d);
}
// a streak of blood starting at `at` and running `len` metres along ang (the way the hit was going)
static void streak(Vector2 at, float ang, float len, Color c = BLOOD_DRY) {
    if (len < .05f)
        return;
    Vector2 p = Vector2Add(at, polar(ang, len * .45f));
    Decal d{p, len * .55f, ang, 1, irand(4, 7), c, 0, (buildingAt(p) >= 0 ? .08f : .05f) + frand(0, .004f)};
    d.born = gTime;
    d.blood = isBloodC(c);
    pushDecal(d);
}
// blood hitting a wall: from where the drop was (`from`) toward where it was going (`to`), find the face and mark it
static void wallSplat(Vector3 from, Vector3 to, float r) {
    bool xb = pointSolid(to.x, from.z, to.y), zb = !xb && pointSolid(from.x, to.z, to.y);
    if (!xb && !zb)
        return;
    Vector2 n = xb ? V2(to.x > from.x ? -1.f : 1.f, 0) : V2(0, to.z > from.z ? -1.f : 1.f);
    float lo = 0, hi = 1; // bisect to the wall face along the blocked axis
    for (int i = 0; i < 6; i++) {
        float m = (lo + hi) * .5f;
        float x = xb ? Lerp(from.x, to.x, m) : from.x, z = zb ? Lerp(from.z, to.z, m) : from.z;
        if (pointSolid(x, z, to.y))
            hi = m;
        else
            lo = m;
    }
    Vector3 p = V3(xb ? Lerp(from.x, to.x, lo) : from.x, Clamp(to.y, .05f, 2.6f), zb ? Lerp(from.z, to.z, lo) : from.z);
    WallDecal w{p, n, r, frand(0, 2 * PI), irand(0, 3), 0, 0, gTime};
    if (r > .07f && p.y > .3f && chance(.55f))
        w.dripMax = std::min(p.y - .05f, frand(.2f, .7f)); // runs down the wall
    if ((int)wallDecals.size() < 1500)
        wallDecals.push_back(w);
    else {
        wallDecals[wallHead] = w;
        wallHead = (wallHead + 1) % 1500;
    }
}
static void spray(Vector3 p, float ang, int n, float spread, float smin, float smax, Color c, bool acidK = false,
                  float up = 2.5f) {
    for (int i = 0; i < n; i++) {
        float a = ang < -99 ? frand(0, 2 * PI) : ang + frand(-spread, spread), s = frand(smin, smax);
        Vector2 h = polar(a, s);
        parts.push_back({p, V3(h.x, frand(.5f, up) * (s * .35f + 1), h.y), 2.f, 2.f, frand(.03f, .07f), c,
                         acidK ? P_ACID : P_BLOOD});
    }
}
static void mist(Vector3 p, int n, Color c) {
    for (int i = 0; i < n; i++)
        parts.push_back({Vector3Add(p, V3(frand(-.2f, .2f), frand(-.1f, .2f), frand(-.2f, .2f))),
                         V3(frand(-1.5f, 1.5f), frand(.2f, 1.2f), frand(-1.5f, 1.5f)), frand(.6f, 1.2f), 1.2f,
                         frand(.4f, .8f), c, P_MIST});
}
static void addGib(Vector3 p, int kind, Color c, Vector3 size, float ang, float spd, bool acidK = false) {
    if (cfg.gore == 0 && !acidK)
        return; // Low gore: blood, but nothing flies
    if (gibs.size() > (cfg.gore == 2 ? 640u : 420u)) {
        if (gibs.front().look >= 0)
            partGibs--;
        gibs.erase(gibs.begin());
    }
    Vector2 h = polar(ang, spd);
    gibs.push_back({p, V3(h.x, frand(2, 6), h.y), Vector3Normalize(V3(frand(-1, 1), frand(-1, 1), frand(-1, 1))),
                    frand(0, 6), frand(-14, 14), kind, c, size, 0, 0, acidK, false});
    gibCount++;
}
static void screenGore(int n) {
    for (int i = 0; i < n; i++) {
        Vector2 p = chance(.5f) ? V2(frand(0, (float)W), chance(.5f) ? frand(0, H * .25f) : frand(H * .75f, (float)H))
                                : V2(chance(.5f) ? frand(0, W * .2f) : frand(W * .8f, (float)W), frand(0, (float)H));
        sdrops.push_back({p, frand(12, 42), 3.f, 3.f});
    }
}
// blood from a hit, by weapon: pistols puff, rifles spray out the far side, shotguns throw a cone,
// blades fling an arc, blunt hits burst and splat at the feet
static void impactBlood(Vector2 zp, float zh, float ang, float dmg, bool head, int style, bool acidK = false) {
    Color c = acidK ? ACID_C : BLOOD_FRESH, dc = acidK ? ACID_C : BLOOD_DRY;
    Vector3 hp = xz3(zp, head ? zh - .15f : frand(.9f, 1.3f));
    float k = Clamp(dmg / 30.f, .5f, 2.5f) * goreK();
    auto N = [&](float n) {
        return std::max(1, (int)(n * k));
    };
    switch (style) {
    case HS_RIFLE:
        spray(hp, ang, N(12), .22f, 4, 12, c, acidK, 1.f);
        streak(Vector2Add(zp, polar(ang, .3f)), ang + frand(-.08f, .08f), frand(1.4f, 2.8f) * sqrtf(k), dc);
        mist(Vector3Add(hp, xz3(polar(ang, .4f), 0)), 3, withA(c, .6f));
        break;
    case HS_SHOTGUN:
        spray(hp, ang, N(16), .9f, 2, 9, c, acidK, 1.6f);
        for (int i = 0; i < irand(2, 3); i++)
            streak(Vector2Add(zp, polar(ang, .2f)), ang + frand(-.5f, .5f), frand(.8f, 2.f), dc);
        break;
    case HS_BLADE: {
        float side = chance(.5f) ? 1.f : -1.f, a = ang + side * 1.1f;
        spray(hp, a, N(10), .6f, 2, 7, c, acidK, 1.4f);
        streak(zp, a + frand(-.3f, .3f), frand(.8f, 1.7f), dc);
        break;
    }
    case HS_BLUNT:
        spray(hp, -999, N(8), 0, .5f, 3, c, acidK, 3.f);
        decal(Vector2Add(zp, polar(frand(0, 6), frand(0, .3f))), frand(.25f, .45f), dc);
        break;
    case HS_BLAST:
        spray(hp, -999, N(20), 0, 2, 10, c, acidK, 2.5f);
        break;
    default:
        spray(hp, ang, N(6), .5f, 1.5f, 6, c, acidK, 1.2f);
        if (chance(.7f))
            streak(Vector2Add(zp, polar(ang, .2f)), ang + frand(-.2f, .2f), frand(.5f, 1.1f) * sqrtf(k), dc);
    }
}
