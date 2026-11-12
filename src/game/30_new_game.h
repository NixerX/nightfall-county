// Nightfall County: new game.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ new game
static void dressHotZones() { // bodies and blood (decals/corpses are wiped after world gen, so this runs last)
    for (auto& b : blds) {
        if (b.cond != 2 || !b.enter || !chance(.45f))
            continue; // abandoned houses: something bad happened here
        for (int k = 0; k < irand(2, 5); k++)
            decal(V2(frand(b.x0 + .6f, b.x1 - .6f), frand(b.z0 + .6f, b.z1 - .6f)), frand(.2f, .6f), BLOOD_DRY,
                  frand(0, .5f));
        if (chance(.3f) && !lookPool[0].empty()) {
            Vector2 p = V2(frand(b.x0 + 1, b.x1 - 1), frand(b.z0 + 1, b.z1 - 1));
            if (freeAt(p.x, p.y, .4f))
                corpses.push_back({p, frand(0, 6), Z_WALKER, true, chance(.7f), true, 0,
                                   lookPool[0][irand(0, (int)lookPool[0].size() - 1)], frand(0, 1)});
        }
    }
    for (auto& h : hotZones) {
        int n = h.kind == HZ_GRAVE ? 28 : irand(6, 11);
        for (int i = 0; i < n; i++) {
            Vector2 p =
                Vector2Add(h.c, polar(frand(0, 2 * PI), h.r * (h.kind == HZ_GRAVE ? .4f : .75f) * sqrtf(frand(0, 1))));
            if (!freeAt(p.x, p.y, .4f))
                continue;
            int pool = h.kind == HZ_CHECKPOINT || h.kind == HZ_EVAC ? (chance(.5f) ? 5 : 0) : 0;
            int look = lookPool[pool].empty() ? -1 : lookPool[pool][irand(0, (int)lookPool[pool].size() - 1)];
            corpses.push_back({p, frand(0, 2 * PI), pool == 5 ? Z_SOLDIER : Z_WALKER, chance(.8f), chance(.8f),
                               chance(.85f), 0, look, frand(0, 1)});
            decal(p, frand(.3f, .6f), BLOOD_DRY, frand(.1f, .5f));
        }
        for (int i = 0; i < 14; i++)
            decal(Vector2Add(h.c, polar(frand(0, 2 * PI), frand(0, h.r * .8f))), frand(.2f, .8f), BLOOD_DRY);
    }
}
static void resetHotbar();
static unsigned worldSeed = 0;
static void newGame(unsigned seed = 0) {
    worldSeed = seed ? seed : ((unsigned)time(nullptr) ^ (unsigned)(GetTime() * 1000));
    rng.seed(worldSeed);
    genWorld();
    genLoot();
    zombies.clear();
    corpses.clear();
    parts.clear();
    gibs.clear();
    bullets.clear();
    bombs.clear();
    acid.clear();
    decals.clear();
    decalHead = 0;
    floaters.clear();
    lights.clear();
    sdrops.clear();
    msgs.clear();
    gTime = CYCLE * frand(.05f, .2f);
    phase = isNight() ? 'n' : 'd';
    spawnT = 0;
    horde = {};
    hordeDay = 0;
    pendingLv = 0;
    nights = 0;
    shake = 0;
    kills = 0;
    bestRar = -1;
    bestName.clear();
    gibCount = 0;
    rainLevel = 0;
    rainTarget = 0;
    lastTown = -2;
    fires.clear();
    playerFemale = chance(.5f);
    P = Player();
    P.next = xpNeed(1);
    P.p = pick(spawnPts);
    P.a = atan2f(TOWNS[0].c.y - P.p.y, TOWNS[0].c.x - P.p.x);
    P.cls = irand(0, 4);
    recalcPerks();
    std::string s1 = randomSkillPoint(), s2 = randomSkillPoint();
    P.hp = P.perks.maxhp;
    P.melee = MK_FISTS;
    P.active = 2;
    P.batt = 35;
    dressStart();
    {
        static bool hb = false;
        if (!hb) {
            resetHotbar();
            hb = true;
        }
    } // your hotbar layout survives death // everyone starts with fists and rags. Your class is what you know, not what you carry
    camPos = P.p;
    for (int i = 0; i < 6; i++)
        spawnAround(Z_WALKER, 30, 70);
    dressHotZones();
    showBanner(std::string("YOU ARE ") + (strchr("AEIOUaeiou", CL().n[0]) ? "AN " : "A ") + CL().n, s1 + "   /   " + s2,
               CL().c);
    msg("Class: " + std::string(CL().n) + ". [Z] " + CL().act + ", [K] skills.", CL().c);
    msg("Fists, rags and whatever's in your pockets. [Tab] inventory. Clothes are your storage.", BONE);
    msg("The county's been redrawn. Same towns, new places. Check the map.", BONE);
    state = ST_PLAY;
}
