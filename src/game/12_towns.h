// Nightfall County: towns.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ------------------------------------------------------------ town generation (fixed seed -> same every run)
static void genTown(int ti) {
    const Town& T = TOWNS[ti];
    std::mt19937 saved = rng;
    rng.seed(T.seed);
    float S = T.kind == TK_CITY       ? 46.f
              : T.kind == TK_TOWN     ? 52.f
              : T.kind == TK_MILITARY ? 60.f
                                      : 48.f,
          rw = T.kind == TK_CITY ? 9.f : 7.f;
    int n = (int)(T.r / S) + 1;
    // ground: pavement / town dirt
    curLayer = T.kind == TK_MILITARY ? 7 : T.kind == TK_CITY ? 6 : 0;
    paintCircle(T.c, T.r * 1.1f, T.kind == TK_MILITARY ? rgba(110, 104, 84) : rgba(96, 96, 86), .55f);
    curLayer = -1;
    if (T.kind == TK_MILITARY) {
        float R = T.r;
        Vector2 c = T.c;
        Color fc = rgba(120, 124, 120);
        fence(c.x - R, c.y - R, c.x + R, c.y - R, fc, 2.6f);
        fence(c.x - R, c.y + R, c.x - 6, c.y + R, fc, 2.6f);
        fence(c.x + 6, c.y + R, c.x + R, c.y + R, fc, 2.6f);
        fence(c.x - R, c.y - R, c.x - R, c.y + R, fc, 2.6f);
        fence(c.x + R, c.y - R, c.x + R, c.y + R, fc, 2.6f);
        addRoad(V2(c.x, c.y + R + 10), V2(c.x, c.y - R + 10), 8, true);
        addRoad(V2(c.x - R + 10, c.y), V2(c.x + R - 10, c.y), 8, true);
        for (int i = 0; i < 4; i++) {
            float bx = c.x - 34 + (i % 2) * 40, bz = c.y - 40 + (i / 2) * 15;
            makeBuilding(bx, bz, 24, 9, B_BARRACKS, 1, i / 2 ? 1 : 0, ti, rgba(120, 124, 104), rgba(70, 76, 64));
        }
        makeBuilding(c.x + 10, c.y + 10, 26, 18, B_HANGAR, 2, 2, ti, rgba(110, 116, 112), rgba(84, 90, 88));
        makeBuilding(c.x - 40, c.y + 30, 12, 9, B_POLICE, 1, 3, ti, rgba(130, 130, 116), rgba(70, 76, 64));
        for (int i = 0; i < 8; i++) {
            Vector2 p = V2(c.x + frand(8, 44), c.y + frand(-48, -10));
            if (boxFree(p.x - .7f, p.y - .7f, p.x + .7f, p.y + .7f)) {
                addProp(M_CUBE, V3(p.x, .5f, p.y), V3(1.4f, 1, 1), frand(0, 3), rgba(80, 90, 60));
                addSolid(p.x - .7f, p.y - .7f, p.x + .7f, p.y + .7f, 1, SK_CRATE);
                yardSpots.push_back({V3(p.x, 1.1f, p.y), LT_MIL});
            }
        }
        for (int i = 0; i < 4; i++) {
            float tx = c.x + (i % 2 ? R - 4 : -R + 4), tz = c.y + (i / 2 ? R - 4 : -R + 4);
            for (int k = 0; k < 4; k++)
                addProp(M_CUBE, V3(tx + (k % 2 ? 1.2f : -1.2f), 2.5f, tz + (k / 2 ? 1.2f : -1.2f)), V3(.2f, 5, .2f), 0,
                        rgba(80, 70, 56));
            addProp(M_CUBE, V3(tx, 5.2f, tz), V3(3.2f, .3f, 3.2f), 0, rgba(90, 76, 60));
            addProp(M_CUBE, V3(tx, 6.3f, tz), V3(3.4f, .2f, 3.4f), 0, rgba(70, 76, 64));
            addSolid(tx - 1.4f, tz - 1.4f, tx + 1.4f, tz + 1.4f, 5, SK_TOWER);
        }
        for (int i = 0; i < 6; i++) {
            Vector2 p = V2(c.x + frand(-R + 8, -8), c.y + frand(9, 24));
            if (boxFree(p.x - 2.5f, p.y - 2, p.x + 2.5f, p.y + 2)) {
                int ta = AS("items/Tent");
                if (ta >= 0) {
                    float sc = fitScale(ta, 5.f, 2), yaw = chance(.5f) ? 0 : PI / 2;
                    Vector2 fp = aSize(ta, sc, yaw);
                    addAProp(ta, V3(p.x, 0, p.y), sc, yaw);
                    addSolid(p.x - fp.x / 2, p.y - fp.y / 2, p.x + fp.x / 2, p.y + fp.y / 2, 2.2f, SK_TENT);
                    yardSpots.push_back({V3(p.x, .1f, p.y + fp.y / 2 + .6f), LT_MIL});
                } else {
                    addProp(M_GABLE, V3(p.x, 0, p.y), V3(5, 2.4f, 4), 0, rgba(118, 124, 92));
                    addSolid(p.x - 2.5f, p.y - 2, p.x + 2.5f, p.y + 2, 2.4f, SK_TENT);
                }
            }
        }
        staticLights.push_back({V3(c.x - 10, 1, c.y + 6), rgba(255, 120, 40), 9, 2});
        for (int i = 0; i < 6; i++)
            car(c.x + frand(14, 44), c.y + frand(8, 30), chance(.5f) ? 0 : PI / 2, .2f, VK_MILITARY);
    } else {
        for (int g = -n; g <= n; g++) {
            float off = g * S;
            if (fabsf(off) > T.r)
                continue;
            float half = sqrtf(std::max(0.f, T.r * T.r - off * off));
            if (T.kind != TK_VILLAGE || g == 0) {
                int tr = T.kind == TK_VILLAGE ? 1 : 0;
                addRoad(V2(T.c.x - half, T.c.y + off), V2(T.c.x + half, T.c.y + off), rw, true, tr);
                addRoad(V2(T.c.x + off, T.c.y - half), V2(T.c.x + off, T.c.y + half), rw, true, tr);
            }
        }
        int specials[4] = {B_POLICE, B_CLINIC, B_FIRE, B_STORE};
        int si = 0;
        bool school = T.kind == TK_VILLAGE;
        for (int gx = -n; gx < n; gx++)
            for (int gz = -n; gz < n; gz++) {
                float bx = T.c.x + gx * S + rw / 2 + 3, bz = T.c.y + gz * S + rw / 2 + 3, bs = S - rw - 6;
                Vector2 bc = V2(bx + bs / 2, bz + bs / 2);
                float dc = Vector2Distance(bc, T.c);
                if (dc > T.r - 6)
                    continue;
                if (T.kind == TK_VILLAGE && !(gx == -1 || gx == 0) && !(gz == -1 || gz == 0))
                    continue;
                bool core = dc < T.r * .45f;
                if (T.kind == TK_CITY && core) {
                    // dense: 2-3 commercial / apartment buildings
                    if (si < 4 && chance(.5f)) {
                        int t = specials[si++];
                        int sb = makeBuilding(bx + 2, bz + 2, bs * .5f, bs * .42f, t, 1, 1, ti,
                                              t == B_CLINIC   ? rgba(210, 212, 210)
                                              : t == B_POLICE ? rgba(120, 130, 150)
                                              : t == B_FIRE   ? rgba(170, 70, 60)
                                                              : rgba(170, 160, 140),
                                              rgba(70, 72, 76));
                        if (sb >= 0 && t != B_FIRE)
                            serviceCar(sb, t);
                    } else
                        makeBuilding(bx + 2, bz + 2, bs * .5f, bs * .42f, chance(.5f) ? B_STORE : B_HOUSE,
                                     chance(.5f) ? 2 : 1, 1, ti, pick(HOUSE_WALL), rgba(70, 72, 76));
                    makeBuilding(
                        bx + bs * .58f, bz + 1, bs * .4f, bs - 2, chance(.6f) ? B_APART : B_OFFICE, irand(3, 6), 2, ti,
                        rgba(150 + irand(-20, 20), 140 + irand(-20, 20), 130 + irand(-20, 20)), rgba(64, 66, 70));
                    makeBuilding(bx + 2, bz + bs * .5f, bs * .5f, bs * .45f, chance(.4f) ? B_STORE : B_GARAGE, 1, 1, ti,
                                 pick(HOUSE_WALL), rgba(70, 72, 76));
                    curLayer = 9;
                    paintRect(bx, bz, bx + bs, bz + bs, rgba(84, 84, 82), .8f, .04f);
                    curLayer = -1;
                } else {
                    // houses with yards
                    if (si < 4 && chance(.3f) && T.kind != TK_VILLAGE) {
                        int t = specials[si++];
                        int sb = makeBuilding(bx + 3, bz + 3, bs * .6f, bs * .5f, t, 1, 1, ti,
                                              t == B_CLINIC ? rgba(210, 212, 210)
                                              : t == B_FIRE ? rgba(170, 70, 60)
                                                            : rgba(140, 140, 150),
                                              rgba(70, 72, 76));
                        if (sb >= 0 && t != B_FIRE)
                            serviceCar(sb, t);
                        continue;
                    }
                    if (!school && chance(.25f)) {
                        if (makeBuilding(bx + 3, bz + 3, bs * .78f, bs * .5f, B_SCHOOL, 1, 1, ti, rgba(170, 120, 90),
                                         rgba(70, 72, 76)) >= 0) {
                            school = true;
                            for (int k = 0; k < 3; k++)
                                crate(V2(bx + 4 + k * 3.f, bz + bs * .5f + 6), LT_SCHOOL);
                            continue;
                        }
                    }
                    if ((T.kind == TK_CITY && dc > T.r * .6f && chance(.35f)) ||
                        (T.kind == TK_TOWN && chance(.12f))) { // industrial lot
                        int fb = makeBuilding(bx + 3, bz + 3, bs * .72f, bs * .55f, B_FACTORY, 2, 1, ti,
                                              rgba(140, 140, 136), rgba(80, 84, 88));
                        if (fb >= 0) {
                            for (int k = 0; k < irand(2, 5); k++)
                                crate(V2(bx + 4 + frand(0, bs * .7f), bz + bs * .55f + frand(5, bs * .4f)),
                                      chance(.5f) ? LT_FACTORY : LT_GARAGE);
                            if (chance(.6f))
                                car(bx + bs * .8f, bz + bs * .75f, PI / 2, .15f, chance(.5f) ? VK_DELIVERY : VK_TRUCK);
                            curLayer = 9;
                            paintRect(bx + 1, bz + 1, bx + bs - 1, bz + bs - 1, rgba(84, 84, 82), .7f, .04f);
                            curLayer = -1;
                            continue;
                        }
                    }
                    if (T.kind == TK_TOWN && chance(.08f)) {
                        makeBuilding(bx + 6, bz + 6, 10, 18, B_CHURCH, 1, 1, ti, rgba(210, 205, 196), rgba(60, 60, 64));
                        continue;
                    }
                    for (int hx = 0; hx < 2; hx++)
                        for (int hz = 0; hz < 2; hz++) {
                            if (chance(.25f))
                                continue;
                            float lx = bx + hx * bs / 2, lz = bz + hz * bs / 2, ls = bs / 2;
                            float hw = frand(9, 12), hd = frand(7.5f, 9.5f);
                            if (hw > ls - 2)
                                hw = ls - 2;
                            if (hd > ls - 2)
                                hd = ls - 2;
                            int bi = makeBuilding(lx + (ls - hw) / 2, lz + (ls - hd) / 2, hw, hd,
                                                  chance(.12f) ? B_GARAGE : B_HOUSE, chance(.25f) ? 2 : 1,
                                                  hz == 0 ? 0 : 1, ti, pick(HOUSE_WALL), pick(ROOF_C));
                            if (bi >= 0 && chance(.12f))
                                woodpile(V2(lx + ls - 2.2f, lz + ls - 1.6f), -1);
                            if (bi >= 0 && chance(.3f)) {
                                float sx = chance(.5f) ? lx + 1.2f : lx + ls - 4.6f,
                                      sz = hz == 0 ? lz + ls - 4.8f : lz + 1.2f;
                                makeBuilding(sx, sz, 3.2f, 3.4f, B_SHED, 1, hz == 0 ? 0 : 1, ti, rgba(130, 110, 86),
                                             rgba(70, 64, 60));
                            }
                            if (bi >= 0 && chance(.45f)) {
                                Color fc = blds[bi].cond == 0 && chance(.5f)
                                               ? rgba(226, 222, 212)
                                               : rgba(150, 130, 100); // white picket for the tidy ones
                                float gx = blds[bi].doors.empty() ? lx + ls / 2 : blds[bi].doors[0].out.x;
                                auto gated = [&](float zz, float xa,
                                                 float xb) { // leave a gate in line with the front door
                                    if (gx - 1.6f > xa + .5f)
                                        fence(xa, zz, gx - 1.6f, zz, fc);
                                    if (gx + 1.6f < xb - .5f)
                                        fence(gx + 1.6f, zz, xb, zz, fc);
                                    if (gx + 1.6f <= xa || gx - 1.6f >= xb)
                                        fence(xa, zz, xb, zz, fc);
                                };
                                if (hz == 0)
                                    gated(lz + .5f, lx + .5f, lx + ls - .5f);
                                else
                                    fence(lx + .5f, lz + .5f, lx + ls - .5f, lz + .5f, fc);
                                if (hz == 1)
                                    gated(lz + ls - .5f, lx + .5f, lx + ls * .4f);
                                else
                                    fence(lx + .5f, lz + ls - .5f, lx + ls * .4f, lz + ls - .5f, fc);
                                fence(lx + .5f, lz + .5f, lx + .5f, lz + ls - .5f, fc);
                            }
                            {
                                int cnd = bi >= 0 ? blds[bi].cond : 1;
                                curLayer = cnd == 0 ? 0 : cnd == 1 ? 1 : 4;
                                paintRect(lx + 1, lz + 1, lx + ls - 1, lz + ls - 1, rgba(70, 92, 56),
                                          cnd == 0 ? .85f : .65f);
                                curLayer = -1;
                                if (cnd == 2) {
                                    int tc = AS("interior/Trashcan_Large");
                                    for (int k = 0; k < irand(2, 5); k++) {
                                        Vector2 tp = V2(frand(lx + 1, lx + ls - 1), frand(lz + 1, lz + ls - 1));
                                        if (!freeAt(tp.x, tp.y, .6f))
                                            continue;
                                        if (k == 0 && tc >= 0)
                                            addAProp(tc, V3(tp.x, 0, tp.y), fitScale(tc, 1.f, 1), frand(0, 6));
                                        else
                                            addProp(M_SPHERE, V3(tp.x, .25f, tp.y), V3(.7f, .5f, .6f), frand(0, 6),
                                                    rgba(24, 24, 26));
                                    }
                                }
                            }
                        }
                }
            }
        // streetlights at intersections, signs and trash
        {
            int sl = AS("street/Streetlight_Single"), ss = AS("street/Sign_Stop"), tc = AS("interior/Trashcan_Large");
            for (int gx = -n; gx <= n; gx++)
                for (int gz = -n; gz <= n; gz++) {
                    if (T.kind == TK_VILLAGE && !(gx == 0 && gz == 0))
                        continue;
                    Vector2 ip = V2(T.c.x + gx * S, T.c.y + gz * S);
                    if (Vector2Distance(ip, T.c) > T.r - 8)
                        continue;
                    float o = rw / 2 + 1.3f;
                    Vector2 lp = V2(ip.x + (chance(.5f) ? o : -o), ip.y + (chance(.5f) ? o : -o));
                    if (sl >= 0 && freeAt(lp.x, lp.y, .6f)) {
                        float sc = fitScale(sl, 5.6f, 1);
                        addAProp(sl, V3(lp.x, 0, lp.y), sc, frand(0, 4) > 2 ? 0 : PI);
                        addSolid(lp.x - .15f, lp.y - .15f, lp.x + .15f, lp.y + .15f, 5, SK_TOWER);
                        if (chance(T.kind == TK_CITY ? .3f : .15f))
                            staticLights.push_back({V3(lp.x, 5.2f, lp.y), rgba(255, 176, 90), 15, 3});
                    }
                    Vector2 sp = V2(ip.x - (lp.x - ip.x), ip.y + (lp.y - ip.y));
                    if (ss >= 0 && chance(.45f) && freeAt(sp.x, sp.y, .5f))
                        addAProp(ss, V3(sp.x, 0, sp.y), fitScale(ss, 2.4f, 1), frand(0, 6));
                    Vector2 tp = Vector2Add(ip, V2(frand(-S * .4f, S * .4f), o * (chance(.5f) ? 1 : -1)));
                    if (tc >= 0 && chance(.5f) && freeAt(tp.x, tp.y, .6f)) {
                        addAProp(tc, V3(tp.x, 0, tp.y), fitScale(tc, 1.0f, 1), frand(0, 6));
                        addSolid(tp.x - .3f, tp.y - .3f, tp.x + .3f, tp.y + .3f, 1, SK_BARREL);
                    }
                }
        }
        // street wrecks & lamps
        for (int i = 0; i < (T.kind == TK_CITY ? 22 : 8); i++) {
            int g = irand(-n, n);
            float along = frand(-T.r * .8f, T.r * .8f);
            bool hz = chance(.5f);
            float side = (chance(.5f) ? 1 : -1) * rw * .25f;
            int vk = T.kind == TK_CITY ? (chance(.04f)   ? VK_FIRE
                                          : chance(.05f) ? VK_GARBAGE
                                          : chance(.04f) ? VK_AMBULANCE
                                                         : VK_CIVIL)
                                       : VK_CIVIL;
            if (hz)
                car(T.c.x + along, T.c.y + g * S + side, chance(.5f) ? 0 : PI, .25f, vk);
            else
                car(T.c.x + g * S + side, T.c.y + along, chance(.5f) ? PI / 2 : -PI / 2, .25f, vk);
        }
        for (int i = 0; i < (T.kind == TK_CITY ? 5 : 2); i++) {
            Vector2 p = Vector2Add(T.c, polar(frand(0, 2 * PI), frand(5, T.r * .6f)));
            if (freeAt(p.x, p.y, 2)) {
                addProp(M_CYL, V3(p.x, 0, p.y), V3(.7f, .9f, .7f), 0, rgba(70, 50, 40));
                addSolid(p.x - .35f, p.y - .35f, p.x + .35f, p.y + .35f, .9f, SK_BARREL);
                staticLights.push_back({V3(p.x, 1.2f, p.y), rgba(255, 120, 40), 10, 2});
            }
        }
    }
    rng = saved;
}

static void buildChunks();
