// Nightfall County: interiors.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ------------------------------------------------------------ countryside (random each run)
// ------------------------------------------------------------ interior cells: upstairs and basements are separate little worlds off the east edge
struct Stair {
    Vector2 a, b;
    int bA, bB;
    bool down;
};
static std::vector<Stair> stairs;
static float cellCurX = CELLX, cellCurZ = 20;
static bool inCell(Vector2 p) {
    return p.x > MAP + 20;
}
static int cellOf(Vector2 p) {
    if (!inCell(p))
        return -1;
    return buildingAt(p);
}
static void stairProps(Vector2 p, bool down, int bi,
                       bool horiz) { // down: a dark opening with rails; up: a flight of steps
    Color wood = rgba(110, 82, 58);
    if (down) {
        addProp(M_CUBE, V3(p.x, .065f, p.y), horiz ? V3(2.2f, .02f, 1.0f) : V3(1.0f, .02f, 2.2f), 0, rgba(8, 8, 10), bi,
                L_ALWAYS);
        for (int k = 0; k < 3; k++) {
            float t = -.7f + k * .5f;
            addProp(M_CUBE, V3(p.x + (horiz ? t : 0), .055f - k * .01f, p.y + (horiz ? 0 : t)),
                    horiz ? V3(.35f, .015f, .9f) : V3(.9f, .015f, .35f), 0, mulC(wood, .5f - k * .12f), bi, L_ALWAYS);
        }
        for (int sd = -1; sd <= 1; sd += 2)
            addProp(M_CUBE, V3(p.x + (horiz ? 0 : sd * .55f), .5f, p.y + (horiz ? sd * .55f : 0)),
                    horiz ? V3(2.2f, .06f, .06f) : V3(.06f, .06f, 2.2f), 0, wood, bi, L_ALWAYS);
    } else
        for (int k = 0; k < 6; k++) {
            float t = -1.0f + k * .36f, hgt = .18f + k * .2f;
            addProp(M_CUBE, V3(p.x + (horiz ? t : 0), hgt / 2, p.y + (horiz ? 0 : t)),
                    horiz ? V3(.36f, hgt, .95f) : V3(.95f, hgt, .36f), 0, mulC(wood, .8f + k * .04f), bi, L_ALWAYS,
                    TX_PLANK);
        }
}
static int makeCell(int bi, int kind) { // kind 1 upstairs, 2 basement
    Building B = blds[bi];
    float w = B.x1 - B.x0, d = B.z1 - B.z0;
    if (w > 20 || d > 20)
        return -1;
    if (cellCurZ + d > MAP - 20) {
        cellCurZ = 20;
        cellCurX += 24;
    }
    if (cellCurX + w > WORLDX - 6)
        return -1;
    float X = cellCurX, Z = cellCurZ;
    cellCurZ += 24;
    int cb;
    if (kind == 1) {
        upstairsGen = true;
        cb = makeBuilding(X, Z, w, d, B.type == B_CABIN ? B_CABIN : B_HOUSE, 1, -1, -1, rgba(200, 196, 186),
                          rgba(70, 70, 70));
        upstairsGen = false;
    } else {
        forceWallTex = TX_PLASTER;
        cb = makeBuilding(X, Z, w, d, B_GARAGE, 1, -1, -1, rgba(120, 118, 112), rgba(60, 60, 60));
        forceWallTex = -1;
        if (cb >= 0) {
            for (auto& sp : blds[cb].spots)
                sp.table = LT_BASEMENT;
            Color sh = rgba(90, 84, 70);
            for (int k = 0; k < 2; k++) {
                float sz = Z + d * (.3f + .4f * k);
                if (freeAt(X + w - .6f, sz, .5f)) {
                    addProp(M_CUBE, V3(X + w - .5f, .9f, sz), V3(.6f, 1.8f, 1.8f), 0, sh, cb, L_ALWAYS);
                    addSolid(X + w - .8f, sz - .9f, X + w - .2f, sz + .9f, 1.8f, SK_FURN, cb);
                    blds[cb].spots.push_back({V3(X + w - .5f, 1.0f, sz), LT_BASEMENT});
                }
            }
            if (freeAt(X + 1.2f, Z + d - 1.2f, .9f)) {
                addProp(M_CYL, V3(X + 1.2f, 0, Z + d - 1.2f), V3(1.1f, 1.7f, 1.1f), 0, rgba(70, 70, 74), cb, L_ALWAYS);
                addSolid(X + .65f, Z + d - 1.75f, X + 1.75f, Z + d - .65f, 1.7f, SK_FURN, cb);
            }
        }
    } // water heater
    if (cb < 0)
        return -1;
    blds[cb].cellKind = kind;
    blds[cb].parent = bi;
    blds[cb].cond = B.cond;
    blds[cb].town = B.town;
    for (int t = 0; t < 400; t++) { // a spot that's clear on both floors and not in a doorway
        float rx = frand(1.0f, w - 1.0f), rz = frand(1.0f, d - 1.0f);
        Vector2 a = V2(B.x0 + rx, B.z0 + rz), b = V2(X + rx, Z + rz);
        float need = t < 200 ? 1.0f : .8f;
        if (!freeAt(a.x, a.y, need) || !freeAt(b.x, b.y, need))
            continue;
        bool nearDoor = false;
        for (auto& dr : blds[bi].doors)
            if (Vector2Distance(dr.in, a) < 2.6f || Vector2Distance(doorObjs[dr.obj].c, a) < 2.2f)
                nearDoor = true;
        for (auto& dr : blds[cb].doors)
            if (Vector2Distance(doorObjs[dr.obj].c, b) < 2.2f)
                nearDoor = true;
        if (nearDoor)
            continue;
        bool hz = chance(.5f);
        stairProps(a, kind == 2, bi, hz);
        stairProps(b, kind == 1, cb, hz);
        stairs.push_back({a, b, bi, cb, kind == 2});
        return cb;
    }
    return cb;
}
static void genCells() {
    stairs.clear();
    cellCurX = CELLX;
    cellCurZ = 20;
    int n = (int)blds.size();
    for (int i = 0; i < n; i++) {
        const Building& b = blds[i];
        if (!b.enter || b.cellKind)
            continue;
        bool home = b.type == B_HOUSE || b.type == B_CABIN;
        if (!home)
            continue;
        if (b.h > 4.f)
            makeCell(i, 1); // two-story: there's an upstairs
        if (chance(b.type == B_CABIN ? .2f : .32f))
            makeCell(i, 2);
    }
}
