// Nightfall County: gen world.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ countryside plots
// A plot claims its land first, then lays out a template that scales with its size and faces the road:
// house at the front, outbuildings behind, a driveway out to the road, a fence with a gate where the drive crosses it.
enum PlotKind {
    PK_SHACK,
    PK_HOMESTEAD,
    PK_FARM,
    PK_RANCH
};
static void paintField(Vector2 a, Vector2 b, Color fc, int layer) { // striped crop field between two corners
    float x0 = std::max(0.f, std::min(a.x, b.x)), x1 = std::min((float)MAP, std::max(a.x, b.x));
    float z0 = std::max(0.f, std::min(a.y, b.y)), z1 = std::min((float)MAP, std::max(a.y, b.y));
    curLayer = layer;
    for (int y = (int)(z0 * PPM); y < (int)(z1 * PPM); y++)
        for (int x = (int)(x0 * PPM); x < (int)(x1 * PPM); x++)
            gblend(x, y, ((y / 3) % 2) ? fc : mulC(fc, .82f), .85f);
    curLayer = -1;
}
static bool makePlot(int kind) {
    static const float SZ[4][4] = {{18, 25, 18, 25}, {25, 35, 25, 35}, {40, 55, 40, 55}, {60, 80, 55, 75}};
    static const float RD[4][2] = {{6, 25}, {8, 30}, {10, 40}, {15, 60}}; // front edge to road
    float W = frand(SZ[kind][0], SZ[kind][1]), D = frand(SZ[kind][2], SZ[kind][3]);
    Vector2 C = V2(frand(80, MAP - 80), frand(80, MAP - 80));
    Vector2 q = nearestRoadPt(C, nullptr, 1);
    float dx = q.x - C.x, dz = q.y - C.y;
    int s = fabsf(dx) > fabsf(dz) ? (dx < 0 ? 2 : 3) : (dz < 0 ? 0 : 1); // side facing the road (= door side)
    Vector2 f = s == 0 ? V2(0, -1) : s == 1 ? V2(0, 1) : s == 2 ? V2(-1, 0) : V2(1, 0); // plot "front"
    Vector2 r = V2(-f.y, f.x);                                                          // plot "right"
    if (chance(.5f))
        r = Vector2Scale(r, -1); // mirrored layout
    bool alongZ = f.x == 0;
    // plot space (u right, v front) -> world
    auto world = [&](float u, float v) {
        return Vector2Add(C, Vector2Add(Vector2Scale(r, u), Vector2Scale(f, v)));
    };
    float ex = alongZ ? W : D, ez = alongZ ? D : W; // plot extent in world x / z
    float px0 = C.x - ex / 2, pz0 = C.y - ez / 2, px1 = C.x + ex / 2, pz1 = C.y + ez / 2;
    // land check: away from towns, hot zones, roads, other plots, anything already built
    for (Vector2 k : {C, V2(px0, pz0), V2(px1, pz0), V2(px0, pz1), V2(px1, pz1)})
        if (townAt(k, 1.2f) >= 0 || hotAt(k, 1.1f) >= 0)
            return false;
    if (px0 < 40 || pz0 < 40 || px1 > MAP - 40 || pz1 > MAP - 40 || inPlot(C, std::max(ex, ez) / 2 + 6))
        return false;
    for (auto& o : plotRects)
        if (px0 < o.x + o.width + 6 && px1 > o.x - 6 && pz0 < o.y + o.height + 6 && pz1 > o.y - 6)
            return false;
    if (!boxFree(px0 - 4, pz0 - 4, px1 + 4, pz1 + 4) || rectOnRoad(px0, pz0, px1, pz1, 2.5f))
        return false;
    float frontGap;
    nearestRoadPt(world(0, D / 2), &frontGap, 1);
    if (frontGap < RD[kind][0] || frontGap > RD[kind][1])
        return false;
    // condition: kept / rundown / abandoned, shared by every building on the plot
    float cr = frand(0, 1);
    int pc = cr < .45f ? 0 : cr < .8f ? 1 : 2;
    auto bcond = [&]() {
        return pc == 0 ? (chance(.75f) ? 0 : 1) : pc;
    };
    // a building in plot space, door toward the road; stays 2 m inside the fence
    auto build = [&](float u, float v, float w, float d, int type, int floors, Color wc, Color rc) {
        if (fabsf(u) + w / 2 > W / 2 - 2 || fabsf(v) + d / 2 > D / 2 - 2)
            return -1;
        Vector2 P = world(u, v);
        float bx = alongZ ? w : d, bz = alongZ ? d : w;
        forceCond = bcond();
        int bi = makeBuilding(P.x - bx / 2, P.y - bz / 2, bx, bz, type, floors, s, -1, wc, rc);
        forceCond = -1;
        return bi;
    };
    auto carAt = [&](Vector2 p, float burnP, int vk) { // wrecks don't check for room themselves
        if (boxFree(p.x - 3, p.y - 3, p.x + 3, p.y + 3) && !rectOnRoad(p.x - 3, p.y - 3, p.x + 3, p.y + 3, .5f))
            car(p.x, p.y, (alongZ ? PI / 2 : 0) + (chance(.5f) ? PI : 0) + frand(-.2f, .2f), burnP, vk);
    };
    auto silo = [&](Vector2 p) {
        if (!boxFree(p.x - 2.6f, p.y - 2.6f, p.x + 2.6f, p.y + 2.6f) ||
            rectOnRoad(p.x - 2.4f, p.y - 2.4f, p.x + 2.4f, p.y + 2.4f))
            return;
        Color sc = chance(.6f) ? rgba(170, 172, 174) : rgba(150, 110, 80);
        float h = frand(9, 13);
        addProp(M_CYL, V3(p.x, 0, p.y), V3(4.4f, h, 4.4f), 0, sc, -1, L_ALWAYS, TX_METAL);
        addProp(M_SPHERE, V3(p.x, h, p.y), V3(4.4f, 2.4f, 4.4f), 0, mulC(sc, .9f));
        addSolid(p.x - 2.1f, p.y - 2.1f, p.x + 2.1f, p.y + 2.1f, h, SK_TOWER);
        aoCircle(p, 4.f, .5f);
    };
    Color weathered =
        pick(std::vector<Color>{rgba(120, 104, 84), rgba(96, 90, 82), rgba(140, 128, 106), rgba(110, 96, 80)});
    Color shedC = rgba(130, 110, 86), darkRoof = rgba(70, 64, 60);
    Color barnC = pick(std::vector<Color>{rgba(130, 50, 40), rgba(130, 50, 40), rgba(150, 146, 136), rgba(118, 96, 74),
                                          rgba(200, 196, 186)});
    // ---- the main house: if it won't fit, there's no plot
    float hw, hd, hu, hv;
    int floors = 1, htype = B_HOUSE;
    Color hwc = pick(HOUSE_WALL), hrc = pick(ROOF_C);
    float setback = kind == PK_SHACK ? frand(4, 7) : kind == PK_RANCH ? frand(8, 12) : frand(5, 9);
    if (kind == PK_SHACK) {
        htype = B_CABIN;
        hw = frand(5.5f, 6.5f);
        hd = frand(5, 5.5f);
        hwc = weathered;
        hrc = darkRoof;
        hu = frand(-.15f, .15f) * W;
    } else if (kind == PK_HOMESTEAD) {
        bool two = chance(.4f);
        hw = two ? frand(8, 10) : frand(10, 13);
        hd = two ? frand(8, 9) : frand(7, 8.5f);
        floors = two ? 2 : 1;
        hu = frand(-.15f, .15f) * W;
    } else if (kind == PK_FARM) {
        hw = frand(8, 12);
        hd = frand(7, 10);
        floors = chance(.55f) ? 2 : 1;
        hu = -W * .22f;
    } else {
        hw = frand(10, 13);
        hd = frand(9, 11);
        floors = 2;
        hu = -frand(.02f, .1f) * W; // nudged toward the barn side, clear of the corral
    }
    hv = D / 2 - setback - hd / 2;
    int hb = build(hu, hv, hw, hd, htype, floors, hwc, hrc);
    if (hb < 0)
        return false;
    // ---- outbuildings and yard
    Vector2 houseSide = world(hu - hw / 2 - 1.6f, hv - hd * .2f); // beside the house, for the woodpile
    float bu = 0, bv = 0, bw = 0, bd = 0;
    int barn = -1;
    switch (kind) {
    case PK_SHACK:
        if (chance(.7f))
            woodpile(houseSide, -1);
        if (chance(.5f))
            build(W * .3f, -D * .25f, 3.2f, 3.4f, B_SHED, 1, weathered, darkRoof);
        if (chance(.4f))
            carAt(world(-W * .28f, -D * .2f), .5f, VK_CIVIL);
        break;
    case PK_HOMESTEAD:
        if (chance(.6f))
            woodpile(houseSide, -1);
        if (chance(.6f)) {
            float side = chance(.5f) ? 1 : -1;
            build(side * (W / 2 - 4.2f), -(D / 2 - 4.4f), 3.4f, 3.6f, B_SHED, 1, shedC, darkRoof);
        }
        if (chance(.4f)) { // vegetable garden
            Vector2 a = world(W * .12f, -D * .08f), b = world(W * .12f + frand(6, 9), -D * .08f - frand(5, 8));
            paintField(a, b, pick(std::vector<Color>{rgba(96, 74, 52), rgba(84, 70, 50)}), 1);
        }
        if (chance(.5f))
            carAt(world(hu + hw / 2 + 3.2f, hv + hd / 2 + 2), .1f, VK_CIVIL);
        break;
    case PK_FARM:
    case PK_RANCH: {
        bool ranch = kind == PK_RANCH;
        bw = ranch ? frand(16, 20) : frand(12, 16);
        bd = ranch ? frand(20, 26) : frand(16, 22);
        bu = ranch ? -W * .25f : W * .2f;
        bv = -D / 2 + (ranch ? 5 : 4) + bd / 2;
        barn = build(bu, bv, bw, bd, B_BARN, 1, barnC, darkRoof);
        if (barn >= 0) {
            Vector2 apron = world(bu, bv + bd / 2 + 4); // packed dirt in front of the barn doors
            curLayer = 1;
            paintCircle(apron, ranch ? 9 : 7, rgba(112, 98, 76), .45f);
            curLayer = -1;
            if (chance(.7f))
                carAt(world(bu + bw * .25f, bv + bd / 2 + 5), .05f, VK_TRACTOR);
            int ns = ranch ? irand(2, 3) : irand(0, 2);
            float sg = bu > 0 ? 1 : -1, su = bu + sg * (bw / 2 + 3.2f); // outer side of the barn,
            if (fabsf(su) + 2.4f > W / 2 - 1.5f)                        // or the inner side if that's
                su = bu - sg * (bw / 2 + 3.2f);                         // past the fence
            for (int k = 0; k < ns; k++)
                silo(world(su, bv - bd / 2 + 3 + k * 5.5f));
        }
        if (ranch) { // machine shed opposite the barn
            float mw = frand(12, 16), md = frand(10, 12);
            build(W * .25f, -D / 2 + 5 + md / 2, mw, md, chance(.5f) ? B_GARAGE : B_BARN, 1,
                  pick(std::vector<Color>{rgba(150, 146, 136), rgba(118, 96, 74), barnC}), darkRoof);
            float cs = frand(15, 20), cu = (bu > 0 ? -1 : 1) * (W / 2 - 3 - cs / 2), cv = frand(-.05f, .1f) * D;
            Vector2 a = world(cu - cs / 2, cv - cs / 2), b = world(cu + cs / 2, cv + cs / 2); // corral
            float ax = std::min(a.x, b.x), az = std::min(a.y, b.y), bx = std::max(a.x, b.x), bz = std::max(a.y, b.y);
            Color cc = rgba(120, 98, 70);
            fence(ax, az, bx, az, cc, 1.4f);
            fence(ax, bz, bx, bz, cc, 1.4f);
            fence(ax, az, ax, bz, cc, 1.4f);
            fence(bx, az, bx, az + (bz - az) * .6f, cc, 1.4f); // gate gap on one side
        } else if (chance(.6f)) {
            build(-(W / 2 - 4.2f), -(D / 2 - 4.4f), 3.4f, 3.6f, B_SHED, 1, shedC, darkRoof);
        }
        if (chance(.6f))
            woodpile(houseSide, -1);
        // fields around the yard
        int nf = ranch ? irand(3, 4) : irand(2, 3);
        for (int k = 0; k < nf; k++) {
            Color fc =
                pick(std::vector<Color>{rgba(140, 120, 70), rgba(110, 90, 60), rgba(150, 140, 80), rgba(90, 110, 60)});
            int layer = chance(.6f) ? 8 : chance(.5f) ? 1 : 5;
            int where = k == 0 ? 0 : irand(0, 2); // 0 behind, 1/2 the sides
            float len = frand(25, ranch ? 70 : 55);
            if (where == 0)
                paintField(world(-W / 2 * frand(.6f, 1.2f), -D / 2 - 3),
                           world(W / 2 * frand(.6f, 1.2f), -D / 2 - 3 - len), fc, layer);
            else {
                float sd = where == 1 ? 1 : -1;
                paintField(world(sd * (W / 2 + 3), D / 2 * frand(-.2f, .6f)),
                           world(sd * (W / 2 + 3 + len), -D / 2 * frand(.6f, 1.3f)), fc, layer);
            }
        }
        break;
    }
    }
    // ---- driveway, then the fence (fence() leaves a gap where it meets the drive: the gate)
    const Building& H = blds[hb];
    Vector2 door = H.doors.empty() ? world(hu, hv + hd / 2 + 1) : H.doors[0].out;
    track(Vector2Add(door, Vector2Scale(f, 2)), kind == PK_RANCH ? 1 : chance(.6f) ? 2 : 1);
    if (!(kind == PK_SHACK && chance(.5f))) {
        Color fc = kind == PK_HOMESTEAD && chance(.4f) ? rgba(200, 196, 186)
                   : kind == PK_SHACK                  ? rgba(100, 86, 70)
                                                       : rgba(130, 110, 80);
        float fh = kind == PK_HOMESTEAD ? .95f : 1.1f;
        auto edge = [&](float ax, float az, float bx, float bz) {
            if (pc == 0) {
                fence(ax, az, bx, bz, fc, fh);
                return;
            }
            float L = std::max(fabsf(bx - ax), fabsf(bz - az));
            int n = std::max(1, (int)(L / 8));
            for (int i = 0; i < n; i++) { // rundown: gaps; abandoned: only scraps left
                if (pc == 1 ? chance(.25f) : !chance(.15f))
                    continue;
                float t0 = (float)i / n, t1 = (float)(i + 1) / n;
                fence(ax + (bx - ax) * t0, az + (bz - az) * t0, ax + (bx - ax) * t1, az + (bz - az) * t1, fc, fh);
            }
        };
        edge(px0, pz0, px1, pz0);
        edge(px0, pz1, px1, pz1);
        edge(px0, pz0, px0, pz1);
        edge(px1, pz0, px1, pz1);
    }
    plotRects.push_back({px0, pz0, px1 - px0, pz1 - pz0});
    return true;
}
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
    plotRects.clear();
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
    // countryside plots: ranches first (they need the most room), then farms, homesteads, shack lots
    {
        static const int WANT[4] = {6, 10, 8, 3}; // shack, homestead, farm, ranch (x AREA)
        for (int kind : {PK_RANCH, PK_FARM, PK_HOMESTEAD, PK_SHACK})
            for (int made = 0, tries = 0; made < (int)(WANT[kind] * AREA) && tries < (int)(80 * WANT[kind] * AREA);
                 tries++)
                made += makePlot(kind);
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
            Color cabC =
                pick(std::vector<Color>{rgba(110, 82, 58), rgba(92, 70, 52), rgba(128, 100, 72), rgba(84, 78, 70)});
            int cb = inPlot(p, 12) ? -1
                                   : makeBuilding(p.x, p.y, frand(6, 9), frand(5, 7), B_CABIN, chance(.25f) ? 2 : 1,
                                                  irand(0, 3), -1, cabC, rgba(60, 54, 48));
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
            if (!boxFree(p.x - 2, p.y - 2, p.x + 2, p.y + 2) || roadDist(p) < 4 || inPlot(p, 3))
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
        if (townAt(c, 1.5f) >= 0 || hotAt(c, 1.3f) >= 0 || inPlot(c, 32))
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
            if (townAt(p, 1.1f) >= 0 || roadDist(p) < 4 || !freeAt(p.x, p.y, 2) || inPlot(p, 2))
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
