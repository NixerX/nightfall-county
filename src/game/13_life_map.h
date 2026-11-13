// Nightfall County: life map.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ------------------------------------------------------------ this life's map: town positions, hot zones
static const Vector2 TOWN_DEFAULT[6] = {{820, 760}, {380, 1200}, {1250, 380}, {300, 330}, {1280, 1260}, {1360, 830}};
static void randomizeTowns() {
    for (int attempt = 0; attempt < 60; attempt++) {
        bool ok = true;
        for (int i = 0; i < 6 && ok; i++) {
            const float m = TOWNS[i].r + 100;
            bool placed = false;
            for (int t = 0; t < 1500 && !placed; t++) {
                Vector2 c = V2(frand(m, MAP - m), frand(m, MAP - m));
                placed = true;
                for (int j = 0; j < i; j++)
                    if (Vector2Distance(c, TOWNS[j].c) < TOWNS[i].r + TOWNS[j].r + 170) {
                        placed = false;
                        break;
                    }
                if (placed)
                    TOWNS[i].c = c;
            }
            ok = placed;
        }
        if (ok)
            return;
    }
    for (int i = 0; i < 6; i++)
        TOWNS[i].c = TOWN_DEFAULT[i];
}
static void lootSpot(Vector2 p, float y, int table) {
    yardSpots.push_back({V3(p.x, y, p.y), table});
}
static void crate(Vector2 p, int table) {
    if (!boxFree(p.x - .7f, p.y - .7f, p.x + .7f, p.y + .7f) ||
        rectOnRoad(p.x - .7f, p.y - .7f, p.x + .7f, p.y + .7f, .3f))
        return;
    addProp(M_CUBE, V3(p.x, .5f, p.y), V3(1.4f, 1, 1), frand(0, 3), rgba(80, 90, 60));
    addSolid(p.x - .7f, p.y - .7f, p.x + .7f, p.y + .7f, 1, SK_CRATE);
    aoRect(p.x - .7f, p.y - .7f, p.x + .7f, p.y + .7f, .9f, .4f);
    lootSpot(p, 1.1f, table);
}
static void tent(Vector2 p, int table) {
    int ta = AS("items/Tent");
    if (ta < 0 || !boxFree(p.x - 2.5f, p.y - 2, p.x + 2.5f, p.y + 2) ||
        rectOnRoad(p.x - 2.5f, p.y - 2, p.x + 2.5f, p.y + 2, .3f))
        return;
    float sc = fitScale(ta, 5.f, 2), yaw = chance(.5f) ? 0 : PI / 2;
    Vector2 fp = aSize(ta, sc, yaw);
    addAProp(ta, V3(p.x, 0, p.y), sc, yaw);
    addSolid(p.x - fp.x / 2, p.y - fp.y / 2, p.x + fp.x / 2, p.y + fp.y / 2, 2.2f, SK_TENT);
    lootSpot(V2(p.x, p.y + fp.y / 2 + .6f), .1f, table);
} // loot sits at the tent flap where you can reach it
static void genHotZones() {
    hotZones.clear();
    int want = (int)(irand(4, 6) * AREA * .55f);
    for (int tries = 0; tries < 6000 && (int)hotZones.size() < want; tries++) {
        HotZone h;
        h.kind = irand(0, HZ_COUNT - 1);
        h.r = frand(34, 52);
        bool nearTown = chance(.35f);
        if (nearTown) {
            const Town& T = TOWNS[irand(0, 5)];
            if (T.kind == TK_MILITARY)
                continue;
            h.c = Vector2Add(T.c, polar(frand(0, 2 * PI), T.r + h.r * .7f));
        } else
            h.c = V2(frand(h.r + 40, MAP - h.r - 40), frand(h.r + 40, MAP - h.r - 40));
        if (h.c.x < h.r + 20 || h.c.y < h.r + 20 || h.c.x > MAP - h.r - 20 || h.c.y > MAP - h.r - 20)
            continue;
        if (townAt(h.c, .85f) >= 0 || hotAt(h.c, 4.f) >= 0 || roadDist(h.c) > 45)
            continue;
        bool nearSpawn = false;
        for (auto& sp : spawnPts)
            if (Vector2Distance(sp, h.c) < 200)
                nearSpawn = true;
        if (nearSpawn)
            continue;
        hotZones.push_back(h);
        // dress it
        Vector2 c = h.c;
        float R = h.r;
        curLayer = 10;
        paintCircle(c, R, rgba(64, 58, 44), .5f);
        curLayer = -1;
        for (int k = 0; k < 6; k++) {
            Vector2 sp = Vector2Add(c, polar(frand(0, 6), frand(0, R * .7f)));
            float sr = frand(3, 7);
            paintCircle(sp, sr, rgba(30, 26, 22), .5f);
            aoCircle(sp, sr, .55f);
        }
        auto rp = [&](float f) {
            return Vector2Add(c, polar(frand(0, 2 * PI), R * f * sqrtf(frand(0, 1))));
        };
        float ry = 0;
        {
            float bd = 1e9f;
            for (auto& r : roads) {
                float d = segDist(c, r.a, r.b);
                if (d < bd) {
                    bd = d;
                    ry = atan2f(r.b.y - r.a.y, r.b.x - r.a.x);
                }
            }
        }
        switch (h.kind) {
        case HZ_CHECKPOINT: {
            Vector2 f = polar(ry, 1), nrm = V2(-f.y, f.x);
            Color fc = rgba(120, 124, 120);
            for (int sd = -1; sd <= 1; sd += 2) {
                Vector2 a = Vector2Add(c, Vector2Scale(nrm, 9.f * sd));
                Vector2 e = Vector2Add(a, Vector2Scale(f, 14));
                Vector2 b0 = Vector2Subtract(a, Vector2Scale(f, 14));
                if (fabsf(f.x) > fabsf(f.y))
                    fence(std::min(b0.x, e.x), a.y, std::max(b0.x, e.x), a.y, fc, 2.2f);
                else
                    fence(a.x, std::min(b0.y, e.y), a.x, std::max(b0.y, e.y), fc, 2.2f);
            }
            for (int k = 0; k < 5; k++)
                crate(rp(.6f), k < 3 ? LT_CACHE : LT_MIL);
            for (int k = 0; k < 2; k++)
                tent(rp(.7f), LT_MIL);
            for (int k = 0; k < 3; k++) {
                Vector2 p = rp(.8f);
                car(p.x, p.y, ry + frand(-.4f, .4f), .5f, k == 0 ? VK_POLICE : VK_MILITARY);
            }
            staticLights.push_back({xz3(rp(.4f), 1.2f), rgba(255, 120, 40), 10, 2});
            break;
        }
        case HZ_MEDEVAC: {
            float yaw = frand(0, 2 * PI);
            Vector2 f = polar(yaw, 1);
            bool heli = boxFree(c.x - 6, c.y - 6, c.x + 6, c.y + 6);
            if (heli) {
                addProp(M_CUBE, V3(c.x, 1.3f, c.y), V3(9, 2.4f, 2.8f), yaw, rgba(66, 72, 64));
                addProp(M_CUBE, V3(c.x - f.x * 7, 1.6f, c.y - f.y * 7), V3(6, .7f, .7f), yaw, rgba(60, 64, 58));
                addProp(M_CUBE, V3(c.x + f.x * 1, 2.7f, c.y + f.y * 1), V3(11, .1f, .3f), yaw + .7f, rgba(40, 40, 40));
                addProp(M_CUBE, V3(c.x + f.x * 3.5f, 1.2f, c.y + f.y * 3.5f), V3(2.4f, 1.8f, 2.6f), yaw,
                        rgba(210, 210, 200));
                addSolid(c.x - 4.5f, c.y - 4.5f, c.x + 4.5f, c.y + 4.5f, 2.4f, SK_CAR);
                aoRect(c.x - 4.5f, c.y - 4.5f, c.x + 4.5f, c.y + 4.5f, 3, .6f);
                paintCircle(c, 10, rgba(20, 18, 16), .7f);
                aoCircle(c, 10, .65f);
                staticLights.push_back({V3(c.x, 1.6f, c.y), rgba(255, 110, 30), 12, 1});
            }
            for (int k = 0; k < 4; k++)
                crate(rp(.6f), k < 2 ? LT_MED : LT_CACHE);
            {
                Vector2 ap = rp(.8f);
                car(ap.x, ap.y, frand(0, 6), .3f, VK_AMBULANCE);
            }
            if (heli)
                lootSpot(Vector2Add(c, Vector2Scale(f, 5.5f)), .1f, LT_MED);
            break;
        }
        case HZ_CONVOY: {
            Vector2 f = polar(ry, 1);
            for (int k = -3; k <= 3; k++) {
                Vector2 p = Vector2Add(c, Vector2Scale(f, k * 6.5f + frand(-1, 1)));
                car(p.x + frand(-1.5f, 1.5f), p.y + frand(-1.5f, 1.5f), ry + frand(-.6f, .6f), .8f,
                    chance(.7f) ? VK_MILITARY : VK_CIVIL);
            }
            for (int k = 0; k < 4; k++)
                crate(rp(.7f), k < 2 ? LT_CACHE : LT_POLICE);
            break;
        }
        case HZ_GRAVE: {
            curLayer = 5;
            paintCircle(c, R * .45f, rgba(84, 66, 46), .8f);
            curLayer = -1;
            for (int k = 0; k < 3; k++)
                crate(rp(.5f), k == 0 ? LT_CACHE : LT_HOUSE);
            for (int k = 0; k < 4; k++)
                lootSpot(rp(.5f), .1f, k < 2 ? LT_HOUSE : LT_CACHE);
            staticLights.push_back({xz3(rp(.3f), 1.2f), rgba(255, 120, 40), 9, 2});
            break;
        }
        case HZ_EVAC: {
            for (int k = 0; k < 4; k++)
                tent(rp(.7f), k == 0 ? LT_CACHE : LT_MIL);
            for (int k = 0; k < 2; k++) {
                Vector2 mp = rp(.8f);
                car(mp.x, mp.y, frand(0, 6), .3f, VK_MILITARY);
            }
            for (int k = 0; k < 4; k++)
                crate(rp(.6f), k < 2 ? LT_CACHE : LT_MED);
            curLayer = 9;
            paintCircle(c, 6, rgba(90, 90, 86), .8f, false);
            curLayer = -1;
            paintCircle(c, 4.5f, rgba(200, 190, 60), .35f, false);
            staticLights.push_back({xz3(rp(.5f), .3f), rgba(255, 50, 30), 8, 1});
            break;
        }
        }
    }
}
