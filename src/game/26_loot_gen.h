// Nightfall County: loot gen.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ loot generation
static int rollRarity(int mn = 0) {
    float r = frand(0, 1) / (P.perks.luck * (1 + .05f * (dayN() - 1)));
    return std::max(mn, r < .03f ? 3 : r < .12f ? 2 : r < .38f ? 1 : 0);
}
static void spawnItem(Vector3 p, int id, int n) {
    Loot l;
    l.kind = LK_ITEM;
    l.id = id;
    l.count = n;
    l.p = p;
    l.v = V3(0, 0, 0);
    l.rot = frand(0, 6);
    loot.push_back(l);
}
static void spawnGun(Vector3 p, int key, int rar) {
    Loot l;
    l.kind = LK_GUN;
    l.id = key;
    l.count = 1;
    l.w = makeWeapon(key, rar);
    l.p = p;
    l.rot = frand(0, 6);
    loot.push_back(l);
    if (rar > bestRar) {
        bestRar = rar;
        bestName = loot.back().w.name;
    }
}
static void spawnMelee(Vector3 p, int key) {
    Loot l;
    l.kind = LK_MELEE;
    l.id = key;
    l.count = 1;
    l.p = p;
    l.rot = frand(0, 6);
    loot.push_back(l);
}
static void spawnCloth(Vector3 p, std::shared_ptr<Cloth> c);
static void rollTable(
    Vector3 p,
    int table) { // loot by place: tools in sheds and garages, clothes in closets and stores, guns where guns would be
    auto it = [&](int id, int a, int b) {
        int n = irand(a, b);
        if (isAmmo(id))
            n = (int)roundf(n * P.perks.ammo);
        spawnItem(Vector3Add(p, V3(frand(-.3f, .3f), 0, frand(-.3f, .3f))), id, n);
    };
    auto cl = [&](int fam, int mask = -1, float cmax = 1) {
        auto c = makeCloth(clothFor(fam, mask), rollRarity(), cmax);
        spawnCloth(Vector3Add(p, V3(frand(-.3f, .3f), 0, frand(-.3f, .3f))), c);
    };
    auto pk = [](std::initializer_list<int> v) {
        return *(v.begin() + irand(0, (int)v.size() - 1));
    };
    const int TOPS = 1 << CS_TORSO | 1 << CS_OUTER, LEGS = 1 << CS_LEGS,
              SMALL = 1 << CS_HEAD | 1 << CS_EYES | 1 << CS_FACE | 1 << CS_HANDS | 1 << CS_SOCKS, FEET = 1 << CS_FEET,
              BAGS = 1 << CS_BACK;
    float r = frand(0, 1);
    switch (table) {
    case LT_WOODPILE:
        it(IT_LOG, 1, 2);
        if (chance(.6f))
            it(IT_KINDLING, 2, 5);
        if (chance(.3f))
            spawnMelee(p, chance(.6f) ? MK_HATCHET : MK_AXE);
        if (chance(.3f))
            it(IT_MATCHES, 4, 10);
        break;
    case LT_CAR:
        if (r < .12f)
            it(pk({IT_A9, IT_A12}), 4, 12);
        else if (r < .24f)
            it(chance(.5f) ? IT_WATER : IT_SODA, 1, 1);
        else if (r < .33f)
            it(IT_CHIPS, 1, 2);
        else if (r < .42f)
            it(IT_FLARE, 1, 2);
        else if (r < .5f)
            it(IT_BANDAGE, 1, 1);
        else if (r < .55f)
            it(IT_PAINKILL, 1, 1);
        else if (r < .62f)
            it(IT_BATTERY, 1, 1);
        else if (r < .67f)
            it(IT_MATCHES, 3, 8);
        else if (r < .71f)
            spawnGun(p, chance(.7f) ? W_M9 : W_GLOCK, rollRarity());
        else if (r < .76f)
            it(IT_TAPE, 1, 1);
        else if (r < .8f)
            spawnMelee(p, pk({MK_CROWBAR, MK_WRENCH}));
        else if (r < .86f)
            cl(F_CIV, SMALL | TOPS);
        else if (r < .89f)
            cl(F_CIV | F_SCHOOL, BAGS);
        else if (r < .915f)
            it(IT_ANTIBIO, 1, 1);
        break;
    case LT_KITCHEN:
        if (chance(.12f))
            it(IT_MATCHES, 4, 12);
        if (r < .35f)
            it(pk({IT_BEANS, IT_PEACHES, IT_CHIPS}), 1, 2);
        else if (r < .55f)
            it(IT_WATER, 1, 1);
        else if (r < .62f)
            spawnMelee(p, MK_KNIFE);
        else if (r < .7f)
            it(IT_SODA, 1, 2);
        else if (r < .76f)
            it(IT_RAG, 1, 2);
        else if (r < .79f)
            it(IT_TAPE, 1, 1);
        else if (r < .815f)
            it(IT_ANTIBIO, 1, 1);
        break;
    case LT_HOUSE:
        if (r < .13f)
            it(IT_BANDAGE, 1, 2);
        else if (r < .22f)
            it(IT_APPLE, 1, 3);
        else if (r < .29f)
            it(IT_BATTERY, 1, 1);
        else if (r < .33f)
            spawnGun(p, chance(.6f) ? W_M9 : W_GLOCK, rollRarity());
        else if (r < .38f)
            it(IT_A9, 6, 15);
        else if (r < .41f)
            spawnMelee(p, MK_BAT);
        else if (r < .46f)
            it(IT_PAINKILL, 1, 2);
        else if (r < .49f)
            it(IT_FLARE, 1, 2);
        else if (r < .54f)
            it(IT_MATCHES, 3, 10);
        else if (r < .6f)
            it(IT_RAG, 1, 3);
        else if (r < .63f)
            it(IT_SEWKIT, 1, 1);
        else if (r < .7f)
            cl(F_CIV, SMALL);
        else if (r < .735f)
            it(IT_ANTIBIO, 1, 1);
        break;
    case LT_WARDROBE:
        if (r < .3f)
            cl(F_CIV, TOPS);
        else if (r < .5f)
            cl(F_CIV, LEGS);
        else if (r < .65f)
            cl(F_CIV, SMALL);
        else if (r < .75f)
            cl(F_CIV, FEET);
        else if (r < .8f)
            cl(F_CIV | F_SCHOOL, BAGS);
        else if (r < .86f)
            it(IT_SEWKIT, 1, 1);
        else if (r < .92f)
            it(IT_RAG, 1, 3);
        break;
    case LT_CLOTHES:
        if (r < .35f)
            cl(F_STORE, TOPS);
        else if (r < .55f)
            cl(F_STORE, LEGS);
        else if (r < .7f)
            cl(F_STORE, SMALL);
        else if (r < .82f)
            cl(F_STORE, FEET);
        else if (r < .9f)
            cl(F_STORE, BAGS);
        else if (r < .95f)
            it(pk({IT_DYE_BLACK, IT_DYE_GREEN, IT_DYE_BROWN}), 1, 1);
        else
            it(IT_SEWKIT, 1, 1);
        break;
    case LT_STORE:
        if (r < .38f)
            it(pk({IT_BEANS, IT_PEACHES, IT_CHIPS, IT_SODA, IT_WATER, IT_APPLE}), 1, 3);
        else if (r < .47f)
            it(IT_BATTERY, 1, 2);
        else if (r < .52f)
            it(IT_FLARE, 1, 2);
        else if (r < .58f)
            it(IT_BANDAGE, 1, 2);
        else if (r < .65f)
            it(IT_MATCHES, 5, 15);
        else if (r < .7f)
            it(IT_TAPE, 1, 1);
        else if (r < .74f)
            it(IT_SEWKIT, 1, 1);
        else if (r < .78f)
            it(IT_WAX, 1, 1);
        else if (r < .88f)
            cl(F_STORE);
        break;
    case LT_GAS:
        if (r < .35f)
            it(pk({IT_CHIPS, IT_SODA, IT_WATER, IT_BEANS}), 1, 2);
        else if (r < .45f)
            it(IT_TAPE, 1, 1);
        else if (r < .55f)
            it(IT_MATCHES, 4, 12);
        else if (r < .62f)
            it(IT_GAS, 1, 1);
        else if (r < .7f)
            it(IT_BATTERY, 1, 1);
        else if (r < .76f)
            it(IT_FLARE, 1, 2);
        else if (r < .82f)
            cl(F_STORE | F_WORK, SMALL);
        else if (r < .85f)
            spawnMelee(p, MK_WRENCH);
        break;
    case LT_POLICE:
        if (r < .26f)
            spawnGun(p, pk({W_M9, W_GLOCK, W_GLOCK, W_SHOTGUN, W_MP5}), rollRarity());
        else if (r < .52f)
            it(pk({IT_A9, IT_A9, IT_A12}), 8, 24);
        else if (r < .62f)
            it(IT_BANDAGE, 1, 3);
        else if (r < .7f)
            it(IT_FLARE, 1, 3);
        else if (r < .86f)
            cl(F_POLICE);
        break;
    case LT_MED:
        if (r < .32f)
            it(IT_BANDAGE, 1, 3);
        else if (r < .5f)
            it(IT_ANTIBIO, 1, 1);
        else if (r < .65f)
            it(IT_PAINKILL, 1, 2);
        else if (r < .74f)
            it(IT_MORPHINE, 1, 1);
        else if (r < .82f)
            it(IT_RAG, 2, 4);
        else if (r < .9f)
            cl(F_MED);
        break;
    case LT_GARAGE:
    case LT_SHED: {
        bool sh = table == LT_SHED;
        if (r < .2f)
            spawnMelee(p, pk({MK_CROWBAR, MK_WRENCH, MK_HATCHET, MK_PIPE, MK_AXE, MK_MACHETE}));
        else if (r < .3f)
            it(IT_TAPE, 1, 1);
        else if (r < .38f)
            it(IT_ROPE, 1, 1);
        else if (r < .44f)
            it(IT_GAS, 1, 1);
        else if (r < .5f)
            it(IT_BATTERY, 1, 2);
        else if (r < .55f)
            it(sh ? IT_KINDLING : IT_PIPEBOMB, 1, sh ? 4 : 1);
        else if (r < .6f)
            it(IT_FLARE, 1, 2);
        else if (r < .66f)
            it(IT_WAX, 1, 1);
        else if (r < .72f)
            it(IT_LEATHER, 1, 2);
        else if (r < .8f)
            cl(F_WORK, SMALL | FEET);
        else if (r < .84f)
            it(IT_RAG, 1, 3);
        break;
    }
    case LT_FACTORY:
        if (r < .22f)
            spawnMelee(p, pk({MK_CROWBAR, MK_WRENCH, MK_PIPE, MK_PIPE}));
        else if (r < .34f)
            it(IT_TAPE, 1, 1);
        else if (r < .42f)
            it(IT_ROPE, 1, 1);
        else if (r < .5f)
            it(IT_GAS, 1, 1);
        else if (r < .62f)
            cl(F_WORK);
        else if (r < .7f)
            it(IT_LEATHER, 1, 3);
        else if (r < .76f)
            it(IT_BATTERY, 1, 2);
        else if (r < .8f)
            it(IT_PIPEBOMB, 1, 1);
        break;
    case LT_FIRE:
        if (r < .28f)
            cl(F_FIRE);
        else if (r < .4f)
            spawnMelee(p, chance(.7f) ? MK_AXE : MK_CROWBAR);
        else if (r < .55f)
            it(IT_BANDAGE, 1, 3);
        else if (r < .62f)
            it(IT_MORPHINE, 1, 1);
        else if (r < .72f)
            it(IT_FLARE, 1, 3);
        else if (r < .78f)
            it(IT_ROPE, 1, 1);
        else if (r < .84f)
            it(IT_GAS, 1, 1);
        break;
    case LT_SCHOOL:
        if (r < .25f)
            cl(F_SCHOOL, BAGS);
        else if (r < .45f)
            cl(F_SCHOOL);
        else if (r < .58f)
            it(pk({IT_CHIPS, IT_SODA, IT_APPLE, IT_WATER}), 1, 2);
        else if (r < .65f)
            it(IT_BANDAGE, 1, 1);
        else if (r < .7f)
            spawnMelee(p, MK_BAT);
        else if (r < .75f)
            it(IT_TAPE, 1, 1);
        break;
    case LT_TRAILER:
        if (r < .14f)
            it(pk({IT_BEANS, IT_CHIPS, IT_SODA}), 1, 2);
        else if (r < .26f)
            cl(F_CIV);
        else if (r < .32f)
            it(IT_TAPE, 1, 1);
        else if (r < .38f)
            spawnGun(p, chance(.6f) ? W_SHOTGUN : W_M9, rollRarity());
        else if (r < .45f)
            it(pk({IT_A12, IT_A9}), 4, 10);
        else if (r < .52f)
            it(IT_RAG, 1, 3);
        else if (r < .57f)
            spawnMelee(p, pk({MK_BAT, MK_PIPE, MK_KNIFE}));
        else if (r < .62f)
            it(IT_MATCHES, 2, 6);
        break;
    case LT_HUNT:
        if (r < .25f)
            cl(F_HUNT);
        else if (r < .38f)
            spawnGun(p, chance(.7f) ? W_HUNT : W_SHOTGUN, rollRarity());
        else if (r < .55f)
            it(chance(.6f) ? IT_A308 : IT_A12, 4, 12);
        else if (r < .62f)
            spawnMelee(p, chance(.5f) ? MK_KNIFE : MK_HATCHET);
        else if (r < .7f)
            it(IT_ROPE, 1, 1);
        else if (r < .76f)
            it(IT_MATCHES, 4, 10);
        else if (r < .82f)
            it(IT_WAX, 1, 1);
        else if (r < .86f)
            it(IT_ANTIBIO, 1, 1);
        break;
    case LT_BASEMENT:
        if (r < .14f)
            it(pk({IT_BEANS, IT_PEACHES, IT_WATER}), 1, 3);
        else if (r < .24f)
            spawnMelee(p, pk({MK_CROWBAR, MK_WRENCH, MK_HATCHET, MK_PIPE, MK_AXE}));
        else if (r < .32f)
            it(IT_TAPE, 1, 1);
        else if (r < .38f)
            it(IT_GAS, 1, 1);
        else if (r < .45f)
            spawnGun(p, pk({W_SHOTGUN, W_HUNT, W_M9, W_GLOCK}), rollRarity());
        else if (r < .54f)
            it(pk({IT_A9, IT_A12, IT_A308}), 6, 18);
        else if (r < .6f)
            it(IT_BATTERY, 1, 2);
        else if (r < .66f)
            cl(F_WORK | F_HUNT | F_CIV, TOPS | FEET);
        else if (r < .72f)
            it(IT_ROPE, 1, 1);
        else if (r < .77f)
            it(IT_MORPHINE, 1, 1);
        else if (r < .82f)
            it(IT_LEATHER, 1, 2);
        break;
    case LT_BARN:
        if (r < .14f)
            spawnGun(p, chance(.6f) ? W_HUNT : W_SHOTGUN, rollRarity());
        else if (r < .3f)
            it(chance(.5f) ? IT_A308 : IT_A12, 4, 12);
        else if (r < .42f)
            spawnMelee(p, pk({MK_AXE, MK_HATCHET, MK_PIPE}));
        else if (r < .52f)
            it(IT_APPLE, 2, 5);
        else if (r < .6f)
            it(IT_ROPE, 1, 1);
        else if (r < .72f)
            cl(F_FARM);
        else if (r < .78f)
            it(IT_LEATHER, 1, 2);
        else if (r < .81f)
            it(IT_ANTIBIO, 1, 1);
        break;
    case LT_MIL:
        if (r < .26f)
            spawnGun(p, pk({W_AR15, W_M4, W_M4, W_MP5, W_HUNT}), rollRarity(1));
        else if (r < .54f)
            it(pk({IT_A556, IT_A556, IT_A9, IT_A308}), 15, 40);
        else if (r < .62f)
            it(IT_PIPEBOMB, 1, 2);
        else if (r < .7f)
            it(IT_MORPHINE, 1, 1);
        else if (r < .76f)
            it(IT_BANDAGE, 2, 3);
        else if (r < .92f)
            cl(F_MIL);
        break;
    case LT_CACHE:
        if (r < .34f)
            spawnGun(p, pk({W_GLOCK, W_SHOTGUN, W_MP5, W_AR15, W_M4, W_HUNT}), rollRarity(1));
        else if (r < .62f)
            it(pk({IT_A9, IT_A12, IT_A556, IT_A308}), 15, 35);
        else if (r < .74f)
            it(IT_PIPEBOMB, 1, 2);
        else if (r < .82f)
            it(chance(.5f) ? IT_MORPHINE : IT_ANTIBIO, 1, 1);
        else if (r < .9f) {
            auto c = makeCloth(clothFor(F_MIL | F_HUNT, BAGS | TOPS), rollRarity(1));
            spawnCloth(p, c);
        } else if (r < .95f)
            spawnMelee(p, MK_AXE);
        else
            it(IT_BATTERY, 2, 3);
        break;
    case LT_OFFICE:
        if (r < .2f)
            it(IT_SODA, 1, 2);
        else if (r < .3f)
            it(IT_BATTERY, 1, 1);
        else if (r < .36f)
            it(IT_PAINKILL, 1, 1);
        else if (r < .42f)
            it(IT_TAPE, 1, 1);
        else if (r < .5f)
            cl(F_CIV, TOPS | SMALL);
        break;
    }
}
static void genLoot() {
    loot.clear();
    for (auto& b : blds) {
        static const float lc[3] = {.85f, .7f, .38f};
        for (auto& s : b.spots)
            if (s.table == LT_CACHE || chance(lc[b.cond]))
                rollTable(s.p, s.table);
    }
    for (auto& s : yardSpots) {
        if (s.table == LT_WOODPILE) {
            rollTable(s.p, s.table);
            continue;
        }
        if (s.table == LT_CACHE) {
            rollTable(s.p, s.table);
            rollTable(s.p, chance(.5f) ? LT_CACHE : LT_MIL);
        } else if (chance(.8f))
            rollTable(s.p, s.table);
    }
}
static void dropFromZombie(Vector2 p, int type, int job);
static void dropJob(Vector2 p, int job) {
    Vector3 g = xz3(p, .05f);
    auto at = [&]() {
        return Vector3Add(g, V3(frand(-.4f, .4f), 0, frand(-.4f, .4f)));
    };
    auto cl = [&](int fam, float cm) {
        if (chance(.5f))
            spawnCloth(at(), makeCloth(clothFor(fam), rollRarity(), cm));
    };
    switch (job) {
    case J_MEDIC:
        spawnItem(at(), chance(.55f) ? IT_ANTIBIO : IT_BANDAGE, 1);
        if (chance(.5f))
            spawnItem(at(), IT_BANDAGE, irand(1, 2));
        if (chance(.2f))
            spawnItem(at(), IT_MORPHINE, 1);
        if (chance(.25f))
            spawnItem(at(), IT_PAINKILL, 1);
        cl(F_MED, .85f);
        break;
    case J_COP:
        spawnItem(at(), IT_A9, irand(6, 15));
        if (chance(.2f))
            spawnGun(at(), chance(.6f) ? W_GLOCK : W_M9, rollRarity());
        cl(F_POLICE, .8f);
        break;
    case J_FIRE:
        if (chance(.35f))
            spawnMelee(at(), MK_AXE);
        spawnItem(at(), IT_FLARE, irand(1, 2));
        cl(F_FIRE, .85f);
        break;
    case J_HUNTER:
        spawnItem(at(), chance(.6f) ? IT_A308 : IT_A12, irand(3, 8));
        if (chance(.3f))
            spawnMelee(at(), chance(.6f) ? MK_KNIFE : MK_HATCHET);
        cl(F_HUNT, .85f);
        break;
    case J_MECH:
        if (chance(.5f))
            spawnItem(at(), IT_TAPE, 1);
        if (chance(.3f))
            spawnMelee(at(), MK_WRENCH);
        if (chance(.25f))
            spawnItem(at(), IT_GAS, 1);
        cl(F_WORK, .85f);
        break;
    case J_HIKER:
        spawnItem(at(), chance(.5f) ? IT_WATER : IT_BEANS, 1);
        if (chance(.5f))
            spawnItem(at(), IT_MATCHES, irand(3, 8));
        if (chance(.4f))
            spawnCloth(at(), makeCloth(clothFor(F_STORE, 1 << CS_BACK), rollRarity(), .8f));
        break;
    }
}
static void dropFromZombie(Vector2 p, int type) {
    dropFromZombie(p, type, 0);
}
static void dropFromZombie(Vector2 p, int type, int job) {
    if (job) {
        dropJob(p, job);
        return;
    }
    float r = frand(0, 1);
    Vector3 g = xz3(p, .05f);
    if (type == Z_BRUTE) {
        spawnGun(g, pick(std::vector<int>{W_SHOTGUN, W_AR15, W_HUNT, W_MP5}), rollRarity(1));
        return;
    }
    if (type == Z_SOLDIER) {
        if (r < .35f)
            spawnItem(g, chance(.7f) ? IT_A556 : IT_A9, irand(5, 20));
        else if (r < .4f)
            spawnGun(g, chance(.5f) ? W_M4 : W_AR15, rollRarity());
        else if (r < .5f)
            spawnItem(g, IT_BANDAGE, 1);
        return;
    }
    if (type == Z_VOLATILE && r < .3f) {
        spawnGun(g, pick(std::vector<int>{W_SHOTGUN, W_M4, W_MP5}), rollRarity(1));
        return;
    }
    float L = P.perks.luck;
    if (chance(.07f * L)) {
        int fam = type == Z_SOLDIER ? F_MIL
                  : chance(.15f)    ? pick(std::vector<int>{F_POLICE, F_FIRE, F_MED, F_WORK, F_FARM})
                                    : F_CIV;
        auto c = makeCloth(clothFor(fam), rollRarity(), .55f);
        spawnCloth(g, c);
        return;
    } // they died in their clothes. Mostly ruined
    if (r < .05f * L)
        spawnItem(g, pick(std::vector<int>{IT_A9, IT_A12, IT_A556}), (int)roundf(irand(3, 10) * P.perks.ammo));
    else if (r < .09f * L)
        spawnItem(g, IT_BANDAGE, 1);
    else if (r < .12f * L)
        spawnItem(g, pick(std::vector<int>{IT_SODA, IT_CHIPS, IT_APPLE}), 1);
}
