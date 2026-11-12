// Nightfall County: gen world.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
static void genWorld() {
    randomizeTowns();
    hotZones.clear();
    doorObjs.clear();
    lofts.clear();
    covers.clear();
    carObjs.clear();
    solids.clear();
    props.clear();
    blds.clear();
    trees.clear();
    roads.clear();
    stairs.clear();
    spawnPts.clear();
    staticLights.clear();
    yardSpots.clear();
    for (auto& c : sgrid)
        for (auto& v : c)
            v.clear();
    for (auto& c : pgrid)
        for (auto& v : c)
            v.clear();
    for (auto& c : tgrid)
        for (auto& v : c)
            v.clear();
    for (auto& c : bgrid)
        for (auto& v : c)
            v.clear();
    if (gpx)
        UnloadImage(groundImg);
    groundImg = GenImageColor(GTEX, GTEX, rgba(70, 92, 52));
    gpx = (Color*)groundImg.data;
    Image pn = GenImagePerlinNoise(GTEX / 4, GTEX / 4, irand(0, 10000), irand(0, 10000), 6.f);
    Color* pp = (Color*)pn.data;
    for (int y = 0; y < GTEX; y++)
        for (int x = 0; x < GTEX; x++) {
            float nv = pp[(y / 4) * (GTEX / 4) + x / 4].r / 255.f;
            Color c = lerpC(rgba(58, 80, 44), rgba(98, 108, 62), nv);
            c = lerpC(c, rgba(110, 100, 70), Clamp((nv - .7f) * 2, 0, 1) * .5f);
            gpx[y * GTEX + x] = mulC(c, .94f + frand(0, .12f));
        }
    splat.assign((size_t)SPN * SPN * SPC, 0);
    for (int y = 0; y < SPN; y++)
        for (int x = 0; x < SPN;
             x++) { // meadow: short grass, long grass where the noise is high, bare earth in the highest spots
            float nv = pp[std::min(GTEX / 4 - 1, y / 2) * (GTEX / 4) + std::min(GTEX / 4 - 1, x / 2)].r / 255.f;
            unsigned char* p = &splat[((size_t)y * SPN + x) * SPC];
            float lg = Clamp((nv - .42f) * 4, 0, 1), dirt = Clamp((nv - .78f) * 6, 0, 1);
            p[0] = (unsigned char)(255 * (1 - lg) * (1 - dirt));
            p[1] = (unsigned char)(255 * lg * (1 - dirt));
            p[5] = (unsigned char)(255 * dirt);
        }
    UnloadImage(pn);
    // highways: minimum spanning tree over this life's town positions, plus the two shortest extra links for loops
    auto edgePt = [&](int i, Vector2 toward) { // land on the end of one of the town's own grid streets
        const Town& T = TOWNS[i];
        Vector2 d = Vector2Normalize(Vector2Subtract(toward, T.c));
        float S = T.kind == TK_CITY ? 46.f : T.kind == TK_TOWN ? 52.f : T.kind == TK_MILITARY ? 60.f : 48.f;
        bool one = T.kind == TK_VILLAGE || T.kind == TK_MILITARY;
        float R = T.r * .95f;
        bool ew = fabsf(d.x) > fabsf(d.y);
        float off = one ? 0 : roundf((ew ? d.y : d.x) * R / S) * S;
        while (fabsf(off) > R * .8f)
            off -= copysignf(S, off);
        float along = sqrtf(std::max(0.f, R * R - off * off));
        if (T.kind == TK_MILITARY)
            along = T.r - 10;
        return ew ? V2(T.c.x + copysignf(along, d.x), T.c.y + off) : V2(T.c.x + off, T.c.y + copysignf(along, d.y));
    };
    auto hwy = [&](int i, int j) {
        Vector2 a = edgePt(i, TOWNS[j].c), b = edgePt(j, TOWNS[i].c);
        float bend = frand(-.12f, .12f);
        Vector2 m = V2((a.x + b.x) / 2 + (a.y - b.y) * bend, (a.y + b.y) / 2 + (b.x - a.x) * bend);
        addRoad(a, m, 8, false);
        addRoad(m, b, 8, false);
    };
    {
        bool in[6] = {true, false, false, false, false, false};
        bool used[6][6] = {};
        for (int k = 1; k < 6; k++) {
            float bd = 1e9f;
            int bi = -1, bj = -1;
            for (int i = 0; i < 6; i++)
                if (in[i])
                    for (int j = 0; j < 6; j++)
                        if (!in[j]) {
                            float d = Vector2Distance(TOWNS[i].c, TOWNS[j].c);
                            if (d < bd) {
                                bd = d;
                                bi = i;
                                bj = j;
                            }
                        }
            in[bj] = true;
            used[bi][bj] = used[bj][bi] = true;
            hwy(bi, bj);
        }
        for (int extra = 0; extra < 2; extra++) {
            float bd = 1e9f;
            int bi = -1, bj = -1;
            for (int i = 0; i < 6; i++)
                for (int j = i + 1; j < 6; j++)
                    if (!used[i][j]) {
                        float d = Vector2Distance(TOWNS[i].c, TOWNS[j].c);
                        if (d < bd) {
                            bd = d;
                            bi = i;
                            bj = j;
                        }
                    }
            if (bi >= 0) {
                used[bi][bj] = used[bj][bi] = true;
                hwy(bi, bj);
            }
        }
    }
    // map-edge roads: where you can wander in from, different every life
    for (int i = 0; i < 8; i++) {
        int side = i % 4;
        float t = frand(120, MAP - 120);
        Vector2 e = side == 0 ? V2(8, t) : side == 1 ? V2(MAP - 8, t) : side == 2 ? V2(t, 8) : V2(t, MAP - 8);
        int nt = 0;
        float nd = 1e9f;
        for (int k = 0; k < 6; k++) {
            float d = Vector2Distance(e, TOWNS[k].c);
            if (d < nd) {
                nd = d;
                nt = k;
            }
        }
        {
            float r = frand(0, 1);
            int tr = r < .45f ? 0 : r < .75f ? 1 : 2;
            addRoad(e, edgePt(nt, e), tr == 0 ? 7 : tr == 1 ? 5.5f : 4, false, tr);
        }
        spawnPts.push_back(Vector2Lerp(e, TOWNS[nt].c, 12.f / nd));
    }
    for (int i = 0; i < 6; i++)
        genTown(i);
    genHotZones();
    // farms (random)
    for (int f = 0, tries = 0; f < (int)(10 * AREA) && tries < (int)(400 * AREA); tries++) {
        Vector2 p = V2(frand(80, MAP - 80), frand(80, MAP - 80));
        if (townAt(p, 1.6f) >= 0)
            continue;
        float rd = roadDist(p);
        if (rd < 18 || rd > 70)
            continue;
        float fx = p.x, fz = p.y;
        for (int k = 0; k < irand(2, 4); k++) {
            float w = frand(40, 90), d = frand(40, 90), ox = fx + frand(-90, 30), oz = fz + frand(-90, 30);
            Color fc =
                pick(std::vector<Color>{rgba(140, 120, 70), rgba(110, 90, 60), rgba(150, 140, 80), rgba(90, 110, 60)});
            curLayer = chance(.6f) ? 8 : chance(.5f) ? 1 : 5;
            for (int y = (int)(oz * PPM); y < (int)((oz + d) * PPM); y++)
                for (int x = (int)(ox * PPM); x < (int)((ox + w) * PPM); x++)
                    gblend(x, y, ((y / 3) % 2) ? fc : mulC(fc, .82f), .85f);
            curLayer = -1;
        }
        int fh = makeBuilding(fx, fz, frand(8, 12), frand(7, 10), B_HOUSE, chance(.55f) ? 2 : 1, 1, -1, pick(HOUSE_WALL),
                              pick(ROOF_C));
        if (chance(.6f))
            woodpile(V2(fx - 3, fz + 4), -1);
        Color barnC = pick(std::vector<Color>{rgba(130, 50, 40), rgba(130, 50, 40), rgba(150, 146, 136), rgba(118, 96, 74), rgba(200, 196, 186)});
        float barnW = frand(12, 16), barnD = frand(16, 22);
        if (makeBuilding(fx + 16, fz - 4, barnW, barnD, B_BARN, 1, 2, -1, barnC, rgba(70, 64, 60)) >= 0 &&
            chance(.7f))
            car(fx + 11, fz + 18, frand(0, 6), .05f, VK_TRACTOR);
        for (int k = 0; k < irand(0, 2); k++) {
            float sx = fx + 33 + k * 5.5f, sz = fz + 2;
            if (boxFree(sx - 2.4f, sz - 2.4f, sx + 2.4f, sz + 2.4f) &&
                !rectOnRoad(sx - 2.4f, sz - 2.4f, sx + 2.4f, sz + 2.4f)) { // grain silos
                Color sc = chance(.6f) ? rgba(170, 172, 174) : rgba(150, 110, 80);
                float h = frand(9, 13);
                addProp(M_CYL, V3(sx, 0, sz), V3(4.4f, h, 4.4f), 0, sc, -1, L_ALWAYS, TX_METAL);
                addProp(M_SPHERE, V3(sx, h, sz), V3(4.4f, 2.4f, 4.4f), 0, mulC(sc, .9f));
                addSolid(sx - 2.1f, sz - 2.1f, sx + 2.1f, sz + 2.1f, h, SK_TOWER);
                aoCircle(V2(sx, sz), 4.f, .5f);
            }
        }
        if (chance(.6f))
            makeBuilding(fx - 6, fz - 7.5f, 3.4f, 3.6f, B_SHED, 1, 1, -1, rgba(130, 110, 86), rgba(70, 64, 60));
        if (fh >= 0) {
            const Building& hb = blds[fh];
            Vector2 dp = hb.doors.empty() ? V2(hb.x0 + 4, hb.z1 + 2) : hb.doors[0].out;
            track(Vector2Add(dp, V2(0, 2)), chance(.6f) ? 2 : 1);
        }
        fence(fx - 6, fz - 10, fx + 34, fz - 10, rgba(130, 110, 80));
        fence(fx - 6, fz + 22, fx + 34, fz + 22, rgba(130, 110, 80));
        f++;
    }
    // forests
    for (int i = 0; i < (int)(22 * AREA); i++) {
        Vector2 c = V2(frand(0, MAP), frand(0, MAP));
        float R = frand(50, 170);
        if (townAt(c, 1.3f) >= 0)
            continue;
        curLayer = chance(.75f) ? 4 : 3;
        paintCircle(c, R * 1.1f, rgba(46, 56, 34), .7f);
        curLayer = -1;
        if (chance(.5f)) {
            Vector2 p = Vector2Add(c, polar(frand(0, 2 * PI), R * .4f));
            Color cabC = pick(std::vector<Color>{rgba(110, 82, 58), rgba(92, 70, 52), rgba(128, 100, 72), rgba(84, 78, 70)});
            int cb = makeBuilding(p.x, p.y, frand(6, 9), frand(5, 7), B_CABIN, chance(.25f) ? 2 : 1, irand(0, 3), -1, cabC, rgba(60, 54, 48));
            if (cb >= 0) {
                woodpile(V2(p.x - 2.2f, p.y + 3), -1);
                if (chance(.4f))
                    makeBuilding(p.x + 9, p.y + 1, 3.2f, 3.2f, B_SHED, 1, 2, -1, rgba(110, 90, 66), rgba(60, 54, 48));
                track(blds[cb].doors.empty()
                          ? V2(p.x + 3.5f, p.y + 8)
                          : Vector2Add(blds[cb].doors[0].out, Vector2Scale(doorObjs[blds[cb].doors[0].obj].outN, 1.5f)),
                      2);
            }
        }
        for (int k = 0; k < (chance(.6f) ? irand(1, 2) : 0); k++) {
            Vector2 p = Vector2Add(c, polar(frand(0, 2 * PI), R * frand(.5f, .9f)));
            if (!boxFree(p.x - 2, p.y - 2, p.x + 2, p.y + 2) || roadDist(p) < 4)
                continue; // hunting stand
            Color wd = rgba(96, 74, 52), lg = rgba(84, 66, 48);
            float hh = 4.2f;
            for (int q = 0; q < 4; q++)
                addProp(M_CUBE, V3(p.x + (q % 2 ? .9f : -.9f), hh / 2, p.y + (q / 2 ? .9f : -.9f)), V3(.16f, hh, .16f),
                        0, lg);
            addProp(M_CUBE, V3(p.x, hh, p.y), V3(2.2f, .14f, 2.2f), 0, wd);
            addProp(M_CUBE, V3(p.x, hh + .5f, p.y - 1.05f), V3(2.2f, .9f, .08f), 0, mulC(wd, .9f));
            addProp(M_CUBE, V3(p.x - 1.05f, hh + .5f, p.y), V3(.08f, .9f, 2.2f), 0, mulC(wd, .9f));
            addProp(M_CUBE, V3(p.x, hh + 1.6f, p.y), V3(2.4f, .1f, 2.4f), 0, rgba(70, 76, 56));
            for (float ry = .4f; ry < hh; ry += .45f)
                addProp(M_CUBE, V3(p.x + 1.3f, ry, p.y), V3(.06f, .06f, .55f), 0, lg);
            for (int q = 0; q < 4; q++)
                addSolid(p.x + (q % 2 ? .8f : -1.f), p.y + (q / 2 ? .8f : -1.f), p.x + (q % 2 ? 1.f : -.8f),
                         p.y + (q / 2 ? 1.f : -.8f), hh, SK_TRUNK);
            addProp(M_CUBE, V3(p.x - .2f, .3f, p.y + .1f), V3(.9f, .6f, .6f), .3f, rgba(70, 80, 56));
            lootSpot(V2(p.x - .2f, p.y + .1f), .65f, LT_HUNT);
            lootSpot(V2(p.x + .3f, p.y - .4f), .1f, LT_HUNT);
            track(V2(p.x + 2.5f, p.y), 2, 260);
        }
        int nt = (int)(R * R * .012f);
        for (int k = 0; k < nt; k++) {
            Vector2 p = Vector2Add(c, polar(frand(0, 2 * PI), R * sqrtf(frand(0, 1))));
            if (p.x < 5 || p.y < 5 || p.x > MAP - 5 || p.y > MAP - 5 || townAt(p, 1.05f) >= 0 || roadDist(p) < 3)
                continue;
            addTree(p, chance(.7f) ? 0 : 1);
        }
    }
    // trailer parks: gravel loop off a road, a few singlewides in rows
    for (int k = 0, tries = 0; k < (int)(3 * AREA) && tries < 300; tries++) {
        Vector2 c = V2(frand(120, MAP - 120), frand(120, MAP - 120));
        if (townAt(c, 1.5f) >= 0 || hotAt(c, 1.3f) >= 0)
            continue;
        float rd = roadDist(c);
        if (rd < 20 || rd > 60)
            continue;
        curLayer = 7;
        paintCircle(c, 26, rgba(120, 114, 100), .5f);
        curLayer = -1;
        int made = 0;
        for (int i = 0; i < 8; i++) {
            float tx = c.x - 22 + (i % 4) * 11.f, tz = c.y + (i / 4 ? 4.f : -12.f);
            Color tc = pick(
                std::vector<Color>{rgba(210, 206, 196), rgba(190, 200, 210), rgba(200, 190, 160), rgba(170, 180, 170)});
            if (chance(.8f) && makeBuilding(tx, tz, 4.6f, 8.f + frand(0, 3), B_TRAILER, 1, i / 4 ? 1 : 0, -1, tc,
                                            rgba(140, 140, 140)) >= 0) {
                made++;
                if (chance(.3f))
                    car(tx + 7.5f, tz + 4, PI / 2, .3f);
            }
        }
        if (made) {
            addRoad(V2(c.x - 24, c.y), V2(c.x + 24, c.y), 4.5f, false, 1);
            track(V2(c.x + 25, c.y), 1);
            k++;
        }
    }
    // roadside homesteads: ranch houses, two-stories and weathered shacks, facing the road
    for (int k = 0, tries = 0; k < (int)(14 * AREA) && tries < (int)(600 * AREA); tries++) {
        Vector2 p = V2(frand(80, MAP - 80), frand(80, MAP - 80));
        if (townAt(p, 1.4f) >= 0 || hotAt(p, 1.2f) >= 0)
            continue;
        float rd = roadDist(p);
        if (rd < 12 || rd > 30)
            continue;
        Vector2 r = nearestRoadPt(p);
        float dx = r.x - p.x, dz = r.y - p.y;
        int side = fabsf(dx) > fabsf(dz) ? (dx < 0 ? 2 : 3) : (dz < 0 ? 0 : 1);
        float roll = frand(0, 1);
        int type, floors;
        Color wall, roof;
        if (roll < .45f) {
            type = B_HOUSE;
            float w = frand(10, 13), d = frand(7, 8.5f);
            floors = 1;
            wall = pick(HOUSE_WALL);
            roof = pick(ROOF_C);
        } else if (roll < .7f) {
            type = B_HOUSE;
            float w = frand(8, 10), d = frand(8, 9);
            floors = 2;
            wall = pick(HOUSE_WALL);
            roof = pick(ROOF_C);
        } else {
            type = B_CABIN;
            float w = frand(5.5f, 6.5f), d = frand(5, 5.5f);
            floors = 1;
            wall = pick(std::vector<Color>{rgba(120, 104, 84), rgba(96, 90, 82), rgba(140, 128, 106), rgba(110, 96, 80)});
            roof = rgba(70, 64, 60);
        }
        int hb = makeBuilding(p.x - w / 2, p.y - d / 2, w, d, type, floors, side, -1, wall, roof);
        if (hb < 0)
            continue;
        if (chance(.5f))
            woodpile(V2(p.x - w / 2 - 1.5f, p.y + d / 2 - 1), -1);
        if (chance(.35f))
            makeBuilding(p.x + w / 2 + 3, p.y - d / 2, 3.2f, 3.4f, B_SHED, 1, side, -1, rgba(130, 110, 86), rgba(70, 64, 60));
        if (chance(.3f))
            car(p.x + w / 2 + 2, p.y + d / 2 + 3, frand(0, 6), .3f);
        const Building& b = blds[hb];
        track(b.doors.empty() ? p : b.doors[0].out, 1);
        k++;
    }

    // pine ridge sits in forest
    for (int k = 0; k < 500; k++) {
        Vector2 p = Vector2Add(TOWNS[3].c, polar(frand(0, 2 * PI), frand(TOWNS[3].r * 1.05f, TOWNS[3].r * 2.4f)));
        if (roadDist(p) > 3)
            addTree(p, 0);
    }
    // scattered trees & rocks
    for (int k = 0; k < (int)(1400 * AREA); k++) {
        Vector2 p = V2(frand(5, MAP - 5), frand(5, MAP - 5));
        if (townAt(p, 1.1f) >= 0 || roadDist(p) < 4)
            continue;
        if (chance(.6f))
            addTree(p, irand(0, 1));
    }
    {
        static const char* rocks[] = {"nature/Rock_1",      "nature/Rock_2",      "nature/Rock_3",     "nature/Rock_4",
                                      "nature/Rock_Moss_1", "nature/Rock_Moss_2", "nature/Rock_Moss_3"};
        for (int k = 0; k < (int)(220 * AREA); k++) {
            Vector2 p = V2(frand(5, MAP - 5), frand(5, MAP - 5));
            if (townAt(p, 1.1f) >= 0 || roadDist(p) < 4 || !freeAt(p.x, p.y, 2))
                continue;
            float s = frand(.8f, 2.4f);
            int ra = AS(rocks[irand(0, 6)]);
            if (ra >= 0) {
                float sc = fitScale(ra, s * 1.4f), yaw = frand(0, 6);
                Vector2 fp = aSize(ra, sc, yaw);
                addAProp(ra, V3(p.x, -.05f, p.y), sc, yaw);
                addSolid(p.x - fp.x * .4f, p.y - fp.y * .4f, p.x + fp.x * .4f, p.y + fp.y * .4f, assets[ra].size.y * sc,
                         SK_ROCK);
            } else {
                addProp(M_SPHERE, V3(p.x, s * .25f, p.y), V3(s * 1.4f, s * .9f, s * 1.2f), frand(0, 6),
                        rgba(108, 106, 100));
                addSolid(p.x - s * .6f, p.y - s * .5f, p.x + s * .6f, p.y + s * .5f, s * .7f, SK_ROCK);
            }
        }
        static const char* bushes[] = {"nature/Bush_1", "nature/Bush_2", "nature/BushBerries_1", "nature/Plant_1"};
        static const float FOL[3] = {.3f, .65f, 1.f};
        float fol = FOL[cfg.foliage];
        for (int k = 0; k < (int)(2600 * fol * AREA); k++) {
            Vector2 p = V2(frand(5, MAP - 5), frand(5, MAP - 5));
            int t = townAt(p, 1.0f);
            if ((t >= 0 && chance(.75f)) || roadDist(p) < 3 || !freeAt(p.x, p.y, .8f))
                continue;
            int ba = AS(bushes[irand(0, 3)]);
            float bs = frand(.9f, 1.8f);
            if (ba >= 0) {
                addAProp(ba, V3(p.x, -.05f, p.y), fitScale(ba, bs), frand(0, 6));
                if (bs > 1.2f)
                    covers.push_back({p, bs * .55f});
            }
        }
        {
            int ga = AS("nature/Grass"), gb = AS("nature/Grass_2");
            if (ga >= 0 && gb >= 0)
                for (int k = 0; k < (int)(220 * std::max(.5f, fol) * AREA); k++) { // tall grass patches to hide in
                    Vector2 c = V2(frand(20, MAP - 20), frand(20, MAP - 20));
                    if (townAt(c, 1.05f) >= 0 || roadDist(c) < 6)
                        continue;
                    for (int j = 0; j < irand(6, 14); j++) {
                        Vector2 p = Vector2Add(c, polar(frand(0, 2 * PI), frand(0, 3.2f)));
                        if (!freeAt(p.x, p.y, .5f))
                            continue;
                        int g = chance(.5f) ? ga : gb;
                        addAProp(g, V3(p.x, -.02f, p.y), fitScale(g, frand(1.0f, 1.4f), 1), frand(0, 6));
                        covers.push_back({p, .85f});
                    }
                }
        }
        for (int k = 0; k < (int)(500 * AREA); k++) {
            Vector2 p = V2(frand(5, MAP - 5), frand(5, MAP - 5));
            if (townAt(p, 1.1f) >= 0 || roadDist(p) < 3 || !freeAt(p.x, p.y, 1.2f))
                continue;
            bool log = chance(.5f);
            int la = AS(log ? (chance(.5f) ? "nature/WoodLog" : "nature/WoodLog_Moss") : "nature/TreeStump");
            if (la >= 0) {
                float sc = fitScale(la, log ? frand(2.5f, 4.f) : frand(.7f, 1.1f)), yaw = frand(0, 6);
                Vector2 fp = aSize(la, sc, yaw);
                addAProp(la, V3(p.x, -.03f, p.y), sc, yaw);
                addSolid(p.x - fp.x * .4f, p.y - fp.y * .4f, p.x + fp.x * .4f, p.y + fp.y * .4f, assets[la].size.y * sc,
                         SK_ROCK);
            }
        }
        int ga = AS("nature/Grass"), gb = AS("nature/Grass_2");
        if (ga >= 0 && gb >= 0)
            for (int k = 0; k < (int)(9000 * fol * AREA * .75f); k++) {
                Vector2 p = V2(frand(5, MAP - 5), frand(5, MAP - 5));
                if (townAt(p, 1.0f) >= 0 || roadDist(p) < 2.5f || pointSolid(p.x, p.y))
                    continue;
                int g = chance(.5f) ? ga : gb;
                addAProp(g, V3(p.x, -.02f, p.y), fitScale(g, frand(.35f, .7f), 1), frand(0, 6));
            }
    }
    genCells();
    // highway wrecks & a few gas stations
    for (auto& r : std::vector<Road>(roads)) {
        if (r.town)
            continue;
        float len = Vector2Distance(r.a, r.b);
        float yaw = atan2f(r.b.y - r.a.y, r.b.x - r.a.x);
        for (float s = 20; s < len; s += frand(40, 110)) {
            Vector2 p = Vector2Lerp(r.a, r.b, s / len);
            Vector2 nrm = V2(-sinf(yaw), cosf(yaw));
            car(p.x + nrm.x * frand(-2.5f, 2.5f), p.y + nrm.y * frand(-2.5f, 2.5f),
                yaw + frand(-.5f, .5f) + (chance(.5f) ? PI : 0), .25f, chance(.15f) ? VK_DELIVERY : VK_CIVIL);
        }
        if (chance(.3f) && len > 120) {
            Vector2 p = Vector2Lerp(r.a, r.b, .5f);
            Vector2 nrm = V2(-sinf(yaw), cosf(yaw));
            Vector2 g = Vector2Add(p, Vector2Scale(nrm, 14));
            if (townAt(g, 1.2f) < 0) {
                int bi = makeBuilding(g.x - 5, g.y - 4, 10, 8, B_GAS, 1, 0, -1, rgba(200, 196, 186), rgba(170, 40, 40));
                if (bi >= 0) {
                    curLayer = 9;
                    paintCircle(g, 18, rgba(80, 80, 78), .8f, false);
                    curLayer = -1;
                    Vector2 nd = Vector2Normalize(Vector2Subtract(p, g));
                    bool ax = fabsf(nd.x) > fabsf(nd.y);
                    Vector2 pc = Vector2Add(
                        g,
                        ax ? V2(copysignf(9.5f, nd.x), 0)
                           : V2(0, copysignf(8.5f, nd.y))); // pump island under a canopy between the shop and the road
                    float cw = ax ? 5.f : 10.f, cd = ax ? 10.f : 5.f;
                    for (int q = 0; q < 4; q++)
                        addProp(M_CUBE,
                                V3(pc.x + (q % 2 ? 1 : -1) * (cw / 2 - .6f), 2.2f,
                                   pc.y + (q / 2 ? 1 : -1) * (cd / 2 - .6f)),
                                V3(.3f, 4.4f, .3f), 0, rgba(220, 220, 220));
                    addProp(M_CUBE, V3(pc.x, 4.5f, pc.y), V3(cw, .5f, cd), 0, rgba(200, 40, 36));
                    addProp(M_CUBE, V3(pc.x, 4.2f, pc.y), V3(cw - .4f, .1f, cd - .4f), 0, rgba(235, 235, 230));
                    for (int q = 0; q < 2; q++) {
                        float px = pc.x + (ax ? 0 : (q ? 2.f : -2.f)), pz = pc.y + (ax ? (q ? 2.f : -2.f) : 0);
                        addProp(M_CUBE, V3(px, .1f, pz), ax ? V3(1.0f, .2f, 1.6f) : V3(1.6f, .2f, 1.0f), 0,
                                rgba(160, 160, 156));
                        addProp(M_CUBE, V3(px, .85f, pz), V3(.5f, 1.5f, .5f), 0, rgba(210, 50, 40));
                        addSolid(px - .25f, pz - .25f, px + .25f, pz + .25f, 1.6f, SK_BARREL);
                    }
                    lootSpot(Vector2Add(pc, ax ? V2(1.2f, 0) : V2(0, 1.2f)), .1f, LT_GAS);
                }
            }
        }
    }
    buildChunks();
    // upload ground
    if (groundTex.id)
        UnloadTexture(groundTex);
    groundTex = LoadTextureFromImage(groundImg);
    GenTextureMipmaps(&groundTex);
    SetTextureFilter(groundTex, TEXTURE_FILTER_TRILINEAR);
    std::vector<Vector3> q = {V3(0, 0, 0), V3(0, 0, MAP), V3(MAP, 0, MAP), V3(0, 0, 0), V3(MAP, 0, MAP), V3(MAP, 0, 0)};
    std::vector<Vector2> uv = {V2(0, 0), V2(0, 1), V2(1, 1), V2(0, 0), V2(1, 1), V2(1, 0)};
    if (groundMesh.vaoId)
        UnloadMesh(groundMesh);
    groundMesh = buildMesh(q, &uv);
    groundMat = LoadMaterialDefault();
    groundMat.shader = lit;
    groundMat.maps[MATERIAL_MAP_DIFFUSE].texture = groundTex;
    groundMat.maps[MATERIAL_MAP_SPECULAR].texture = detailTex;
    if (groundAtlasOk) {
        for (int k = 0; k < 3; k++) {
            if (splatTex[k].id)
                UnloadTexture(splatTex[k]);
            Image im = GenImageColor(SPN, SPN, BLANK);
            unsigned char* d = (unsigned char*)im.data;
            for (size_t i = 0; i < (size_t)SPN * SPN; i++)
                for (int c = 0; c < 4; c++)
                    d[i * 4 + c] = splat[i * SPC + 1 + k * 4 + c];
            splatTex[k] = LoadTextureFromImage(im);
            UnloadImage(im);
            SetTextureFilter(splatTex[k], TEXTURE_FILTER_BILINEAR);
            SetTextureWrap(splatTex[k], TEXTURE_WRAP_CLAMP);
        }
        groundMat.maps[MATERIAL_MAP_NORMAL].texture = groundAtlas[0];
        groundMat.maps[MATERIAL_MAP_ROUGHNESS].texture = groundAtlas[1];
        groundMat.maps[MATERIAL_MAP_OCCLUSION].texture = splatTex[0];
        groundMat.maps[MATERIAL_MAP_EMISSION].texture = splatTex[1];
        groundMat.maps[MATERIAL_MAP_HEIGHT].texture = splatTex[2];
    }
    // map texture
    if (mapTex.id)
        UnloadTexture(mapTex);
    Image mi = ImageCopy(groundImg);
    ImageResize(&mi, 800, 800);
    const float ms = 800.f / MAP;
    for (auto& r : roads)
        ImageDrawLineEx(&mi, V2(r.a.x * ms, r.a.y * ms), V2(r.b.x * ms, r.b.y * ms),
                        r.tier == 2 ? 1
                        : r.town    ? 2
                                    : 3,
                        r.tier == 2   ? rgba(120, 92, 60)
                        : r.tier == 1 ? rgba(150, 144, 130)
                                      : rgba(50, 50, 52));
    for (auto& b : blds)
        ImageDrawRectangle(&mi, (int)(b.x0 * ms), (int)(b.z0 * ms), std::max(1, (int)((b.x1 - b.x0) * ms)),
                           std::max(1, (int)((b.z1 - b.z0) * ms)), b.enter ? rgba(170, 160, 140) : rgba(120, 116, 110));
    mapTex = LoadTextureFromImage(mi);
    UnloadImage(mi);
}
