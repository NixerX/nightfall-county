// Nightfall County: spawning.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ spawning
static float diffK() { static const float base[4] = {.7f, 1.f, 1.3f, 1.65f}; return base[cfg.diff]; }
static int playerLevel();
static float scaleK() { return diffK() * (1 + .07f * (playerLevel() - 1)); } // zombies get tougher as you level up
static Zombie& spawnZ(int type, Vector2 p) {
    const ZDef& t = ZT[type]; float s = 1 + .12f * (dayN() - 1);
    Zombie z{}; z.id = ++zid; z.type = type; z.p = p; z.r = t.r; z.hp = z.mhp = t.hp * s * scaleK(); z.spd = t.spd * frand(.85f, 1.15f) * (cfg.diff >= 2 ? 1.08f : cfg.diff == 0 ? .92f : 1); z.dmg = t.dmg * (1 + .06f * (dayN() - 1)) * (.6f + .4f * scaleK());
    z.seed = frand(0, 10); z.a = frand(0, 2 * PI); z.side = chance(.5f) ? 1 : -1; z.wa = frand(0, 2 * PI);
    { static const int pool[6] = {0, 1, 2, 3, 4, 5}; auto& lp = lookPool[pool[type]]; if (!lp.empty()) z.look = lp[irand(0, (int)lp.size() - 1)]; else if (!lookPool[0].empty()) z.look = lookPool[0][irand(0, (int)lookPool[0].size() - 1)]; }
    if ((type == Z_WALKER || type == Z_RUNNER) && chance(.07f)) { // uniforms: more of them near the places they worked
        int t = townAt(p, 1.2f), h = hotAt(p, 1.3f); std::vector<int> pool{J_MEDIC, J_COP, J_FIRE, J_MECH, J_HIKER};
        if (t < 0) { pool = {J_HUNTER, J_HUNTER, J_HIKER, J_HIKER, J_MECH}; } if (h >= 0) { pool.push_back(J_MEDIC); pool.push_back(J_MEDIC); pool.push_back(J_COP); }
        z.job = pick(pool); }
    zombies.push_back(z); return zombies.back();
}
static Zombie* spawnAround(int type, float minD, float maxD, float ang = -999, float spreadA = .5f) {
    for (int i = 0; i < 16; i++) { float a = ang < -99 ? frand(0, 2 * PI) : ang + frand(-spreadA, spreadA), d = frand(minD, maxD); Vector2 p = Vector2Add(P.p, polar(a, d));
        if (freeAt(p.x, p.y, .6f)) return &spawnZ(type, p); }
    return nullptr;
}
static int zoneTown() { return townAt(P.p, 1.3f); }
static int pickType(int town) {
    int d = dayN(); bool n = isNight();
    if (town >= 0 && TOWNS[town].kind == TK_MILITARY && chance(.6f)) return Z_SOLDIER;
    int hz = hotAt(P.p, 1.6f); if (hz >= 0) { int k = hotZones[hz].kind; float r = frand(0, 1);
        if ((k == HZ_CHECKPOINT || k == HZ_EVAC || k == HZ_CONVOY) && r < .35f) return Z_SOLDIER; if (r < .55f) return Z_RUNNER; if (r < .62f) return Z_BRUTE; if (r < .68f && d >= 2) return Z_BLOATER; if (n && r < .72f) return Z_VOLATILE; }
    float w[6] = {70, 10.f + d * 3 + (n ? 18 : 0), 6.f + d, d >= 2 ? 2.f + d : 0, n ? 8.f + d * 3 : 0, 0};
    float tot = 0; for (float x : w) tot += x; float r = frand(0, tot);
    for (int i = 0; i < 6; i++) { r -= w[i]; if (r <= 0) return i; }
    return 0;
}

