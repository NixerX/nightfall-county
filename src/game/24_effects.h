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
static void decal(Vector2 p, float r, Color c, float grow = 0) {
    Decal d{p, r, frand(0, 2 * PI), 1, irand(0, 3), c, grow, (buildingAt(p) >= 0 ? .08f : .05f) + frand(0, .004f)};
    if ((int)decals.size() < 6000)
        decals.push_back(d);
    else {
        decals[decalHead] = d;
        decalHead = (decalHead + 1) % 6000;
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
    if (gibs.size() > 420)
        gibs.erase(gibs.begin());
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
