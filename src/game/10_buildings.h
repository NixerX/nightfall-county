// Nightfall County: buildings.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// doorSide: 0 north(z0) 1 south(z1) 2 west(x0) 3 east(x1)
enum VKind { VK_CIVIL, VK_POLICE, VK_AMBULANCE, VK_FIRE, VK_DELIVERY, VK_GARBAGE, VK_TRACTOR, VK_MILITARY, VK_TRUCK };
static void car(float x, float z, float yaw, float burnP, int kind);
static void crate(Vector2 p, int table);
static int forceWallTex = -1; static bool upstairsGen = false;
struct Gap { Vector2 c; bool vert; }; static std::vector<Gap> curGaps; // interior doorways of the building being made
static int makeBuildingRaw(float x0, float z0, float w, float d, int type, int floors, int doorSide, int town, Color wallC, Color roofC);
static int makeBuilding(float x0, float z0, float w, float d, int type, int floors, int doorSide, int town, Color wallC, Color roofC) {
    curGaps.clear(); size_t p0 = props.size(); int bi = makeBuildingRaw(x0, z0, w, d, type, floors, doorSide, town, wallC, roofC); if (bi < 0) return bi;
    // keep every doorway walkable: nothing solid in front of or behind a door or an interior opening
    std::vector<Rectangle> keep;
    for (auto& g : curGaps) keep.push_back(g.vert ? Rectangle{g.c.x - 1.3f, g.c.y - .6f, 2.6f, 1.2f} : Rectangle{g.c.x - .6f, g.c.y - 1.3f, 1.2f, 2.6f});
    for (auto& dr : blds[bi].doors) { if (dr.obj < 0) continue; const DoorObj& D = doorObjs[dr.obj]; Vector2 in = Vector2Subtract(D.c, Vector2Scale(D.outN, 2.4f)); float hw = D.w / 2 + .35f;
        keep.push_back({std::min(D.c.x, in.x) - (D.horiz ? hw : .3f), std::min(D.c.y, in.y) - (D.horiz ? .3f : hw), fabsf(D.c.x - in.x) + (D.horiz ? 2 * hw : .6f), fabsf(D.c.y - in.y) + (D.horiz ? .6f : 2 * hw)}); }
    for (auto& g : curGaps) blds[bi].gaps.push_back(g.c);
    auto hitK = [&](float a0, float b0, float a1, float b1) { for (auto& k : keep) if (a0 < k.x + k.width && a1 > k.x && b0 < k.y + k.height && b1 > k.y) return true; return false; };
    for (auto& so : solids) if (so.bld == bi && so.kind == SK_FURN && hitK(so.x0, so.z0, so.x1, so.z1)) { so.x0 = so.x1 = so.z0 = so.z1 = -500; }
    props.erase(std::remove_if(props.begin() + p0, props.end(), [&](const Prop& p) { return p.bld == bi && p.layer == L_ALWAYS && p.p.y < 2.4f && hitK(p.p.x - .3f, p.p.z - .3f, p.p.x + .3f, p.p.z + .3f); }), props.end());
    return bi;
}
static int makeBuildingRaw(float x0, float z0, float w, float d, int type, int floors, int doorSide, int town, Color wallC, Color roofC) {
    if (!boxFree(x0 - .3f, z0 - .3f, x0 + w + .3f, z0 + d + .3f) || rectOnRoad(x0, z0, x0 + w, z0 + d, 1.2f)) return -1;
    aoRect(x0, z0, x0 + w, z0 + d, 2.2f, .5f);
    int bi = (int)blds.size(); Building B; B.x0 = x0; B.z0 = z0; B.x1 = x0 + w; B.z1 = z0 + d; B.type = type; B.town = town;
    { float r = frand(0, 1); B.cond = r < .3f ? 0 : r < .72f ? 1 : 2; } curCond = B.cond; furnB.on = false;
    if (type == B_HOUSE) wallC = houseColor();
    if (B.cond == 1) wallC = lerpC(wallC, rgba(120, 112, 98), .18f); else if (B.cond == 2) { wallC = lerpC(wallC, rgba(84, 80, 70), .42f); roofC = lerpC(roofC, rgba(52, 62, 44), .35f); }
    B.enter = type != B_APART && type != B_OFFICE; float H = 3.f; B.h = floors * H; blds.push_back(B);
    for (int cx = gcl(x0); cx <= gcl(x0 + w); cx++) for (int cz = gcl(z0); cz <= gcl(z0 + d); cz++) bgrid[cx][cz].push_back(bi);
    float x1 = x0 + w, z1 = z0 + d;
    // floor
    bool ind = type == B_BARN || type == B_GARAGE || type == B_HANGAR || type == B_SHED || type == B_FACTORY || type == B_FIRE, pub = type == B_CLINIC || type == B_STORE || type == B_POLICE || type == B_SCHOOL || type == B_GAS;
    Color floorC = ind ? rgba(110, 106, 98) : pub ? rgba(190, 190, 184) : rgba(124, 98, 76);
    int floorTex = ind ? TX_CONCRETE : (pub || type == B_OFFICE) ? TX_TILE : TX_HARDWOOD;
    addProp(M_CUBE, V3(x0 + w / 2, .03f, z0 + d / 2), V3(w, .06f, d), 0, floorC, bi, L_FLOOR, floorTex);
    switch (type) { case B_HOUSE: curWallTex = chance(.65f) ? TX_SIDING : TX_BRICK; break; case B_CABIN: curWallTex = TX_CABIN; break; case B_BARN: curWallTex = TX_BARNWOOD; break;
        case B_APART: case B_OFFICE: curWallTex = chance(.6f) ? TX_BRICK : TX_PLASTER; break; case B_GARAGE: case B_HANGAR: case B_TRAILER: curWallTex = TX_METAL; break; case B_SHED: curWallTex = chance(.5f) ? TX_BARNWOOD : TX_METAL; break;
        case B_FACTORY: curWallTex = chance(.5f) ? TX_METAL : TX_BRICK; break; case B_FIRE: case B_SCHOOL: curWallTex = TX_BRICK; break; default: curWallTex = chance(.5f) ? TX_PLASTER : TX_BRICK; }
    if (forceWallTex >= 0) curWallTex = forceWallTex;
    if (curWallTex == TX_BRICK && type != B_HOUSE) wallC = lerpC(wallC, rgba(150, 82, 64), .6f); else if (curWallTex == TX_BRICK) wallC = lerpC(wallC, rgba(160, 90, 70), .7f);
    float dw = type == B_BARN || type == B_GARAGE || type == B_HANGAR || type == B_FACTORY || type == B_FIRE ? 3.6f : type == B_SHED || type == B_TRAILER ? 1.2f : 1.6f;
    auto side = [&](int s, float ax, float az, float bx, float bz, float outN) {
        bool horiz = s < 2; bool hasDoor = B.enter && (s == doorSide || (w * d > 140 && s == (doorSide ^ 1)));
        if (hasDoor) {
            float L = horiz ? bx - ax : bz - az, c = L * frand(.35f, .65f);
            if (horiz) { wallSeg(ax, az, ax + c - dw / 2, az, H, wallC, bi, true, true, outN); wallSeg(ax + c + dw / 2, az, bx, bz, H, wallC, bi, true, true, outN);
                blds[bi].doors.push_back({V2(ax + c, az + outN * 1.6f), V2(ax + c, az - outN * 1.6f), makeDoor(V2(ax + c, az), true, dw, bi)}); doorObjs.back().outN = V2(0, outN);
                addProp(M_CUBE, V3(ax + c, H - .25f, az), V3(dw, .5f, .24f), 0, wallC, bi, L_WALLDET); }
            else { wallSeg(ax, az, ax, az + c - dw / 2, H, wallC, bi, true, true, outN); wallSeg(ax, az + c + dw / 2, bx, bz, H, wallC, bi, true, true, outN);
                blds[bi].doors.push_back({V2(ax + outN * 1.6f, az + c), V2(ax - outN * 1.6f, az + c), makeDoor(V2(ax, az + c), false, dw, bi)}); doorObjs.back().outN = V2(outN, 0);
                addProp(M_CUBE, V3(ax, H - .25f, az + c), V3(.24f, .5f, dw), 0, wallC, bi, L_WALLDET); }
        } else wallSeg(ax, az, bx, bz, H, wallC, bi, true, true, outN);
    };
    side(0, x0, z0, x1, z0, -1); side(1, x0, z1, x1, z1, 1); side(2, x0, z0, x0, z1, -1); side(3, x1, z0, x1, z1, 1);
    // interior partition
    struct RoomR { float x0, z0, x1, z1; int kind = 0; }; std::vector<RoomR> rooms;
    if (type == B_HOUSE) { // 3-5 rooms by recursive splits, each wall with a doorway
        rooms.push_back({x0, z0, x1, z1}); int want = irand(3, 5); int saveTex = curWallTex; curWallTex = TX_PLASTER; Color inC = lerpC(wallC, rgba(214, 206, 190), .55f);
        for (int it = 0; it < 12 && (int)rooms.size() < want; it++) {
            int bi2 = -1; float ba = 0; for (int i = 0; i < (int)rooms.size(); i++) { const RoomR& r = rooms[i]; float a = (r.x1 - r.x0) * (r.z1 - r.z0); if (std::max(r.x1 - r.x0, r.z1 - r.z0) >= 5.2f && a > ba) { ba = a; bi2 = i; } }
            if (bi2 < 0) break; RoomR r = rooms[bi2]; bool vert = (r.x1 - r.x0) >= (r.z1 - r.z0); float t = .5f;
            for (int tr = 0; tr < 8; tr++) { t = frand(.38f, .62f); float sp = vert ? r.x0 + (r.x1 - r.x0) * t : r.z0 + (r.z1 - r.z0) * t; bool bad = false; // keep inner walls clear of the front door
                for (auto& dr : blds[bi].doors) { Vector2 dc = Vector2Scale(Vector2Add(dr.out, dr.in), .5f); if (fabsf((vert ? dc.x : dc.y) - sp) < 1.4f) bad = true; }
                for (auto& g : curGaps) { if (vert ? (fabsf(g.c.x - sp) < 1.1f && g.c.y > r.z0 - .2f && g.c.y < r.z1 + .2f) : (fabsf(g.c.y - sp) < 1.1f && g.c.x > r.x0 - .2f && g.c.x < r.x1 + .2f)) bad = true; } // never wall off an existing doorway
                if (!bad) break; if (tr == 7) t = -1; }
            if (t < 0) continue;
            if (vert) { float sx = r.x0 + (r.x1 - r.x0) * t, gc = frand(r.z0 + .9f, r.z1 - .9f); wallSeg(sx, r.z0, sx, gc - .6f, H, inC, bi, false, false, 0); wallSeg(sx, gc + .6f, sx, r.z1, H, inC, bi, false, false, 0); curGaps.push_back({V2(sx, gc), true});
                rooms[bi2] = {r.x0, r.z0, sx, r.z1}; rooms.push_back({sx, r.z0, r.x1, r.z1}); }
            else { float sz = r.z0 + (r.z1 - r.z0) * t, gc = frand(r.x0 + .9f, r.x1 - .9f); wallSeg(r.x0, sz, gc - .6f, sz, H, inC, bi, false, false, 0); wallSeg(gc + .6f, sz, r.x1, sz, H, inC, bi, false, false, 0); curGaps.push_back({V2(gc, sz), false});
                rooms[bi2] = {r.x0, r.z0, r.x1, sz}; rooms.push_back({r.x0, sz, r.x1, r.z1}); } }
        curWallTex = saveTex;
        std::sort(rooms.begin(), rooms.end(), [](const RoomR& a, const RoomR& b) { return (a.x1 - a.x0) * (a.z1 - a.z0) > (b.x1 - b.x0) * (b.z1 - b.z0); });
        for (int i = 0; i < (int)rooms.size(); i++) rooms[i].kind = upstairsGen ? ((i == (int)rooms.size() - 1 && rooms.size() >= 3) ? 3 : 2) : i == 0 ? 0 : i == 1 ? 1 : (i == (int)rooms.size() - 1 && rooms.size() >= 4) ? 3 : 2; // 0 living 1 kitchen 2 bedroom 3 bath. Upstairs: bedrooms and a bath
    }
    else if (B.enter && w > 9 && d > 7 && type != B_GARAGE && type != B_BARRACKS && type != B_FIRE && type != B_FACTORY) { float px = x0 + w * frand(.45f, .6f), g = z0 + d * frand(.35f, .65f); if (type == B_CLINIC) curWallTex = TX_HTILE; wallSeg(px, z0, px, g - .8f, H, mulC(wallC, .92f), bi, false, false, 0); wallSeg(px, g + .8f, px, z1, H, mulC(wallC, .92f), bi, false, false, 0); curGaps.push_back({V2(px, g), true}); }
    // upper floors & roof
    if (floors > 1) addProp(M_CUBE, V3(x0 + w / 2, H + (floors - 1) * H / 2, z0 + d / 2), V3(w + .22f, (floors - 1) * H, d + .22f), 0, wallC, bi, L_ROOF, curWallTex);
    if (floors > 1) for (int f = 1; f < floors; f++) for (float xx = x0 + 1.6f; xx < x1 - 1; xx += 3.2f) {
        addProp(M_CUBE, V3(xx, f * H + 1.7f, z1 + .14f), V3(1.1f, 1.f, .05f), 0, rgba(28, 34, 42), bi, L_ROOF);
        addProp(M_CUBE, V3(xx, f * H + 1.7f, z0 - .14f), V3(1.1f, 1.f, .05f), 0, rgba(28, 34, 42), bi, L_ROOF);
    }
    float top = floors * H;
    bool gable = type == B_HOUSE || type == B_BARN || type == B_CABIN || type == B_CHURCH || type == B_SHED;
    if (gable) {
        float rh = type == B_BARN ? 3.2f : type == B_CHURCH ? 4.f : type == B_SHED ? 1.2f : 2.2f; bool alongX = w >= d;
        addProp(M_GABLE, V3(x0 + w / 2, top, z0 + d / 2), alongX ? V3(w + .7f, rh, d + .7f) : V3(d + .7f, rh, w + .7f), alongX ? 0 : PI / 2, roofC, bi, L_ROOF, TX_ROOFTILE);
        if (type == B_CHURCH) { addProp(M_CUBE, V3(x0 + 2, top + 3, z0 + d / 2), V3(2.5f, 6, 2.5f), 0, wallC, bi, L_ROOF); addProp(M_CONE, V3(x0 + 2, top + 6, z0 + d / 2), V3(3, 4, 3), 0, roofC, bi, L_ROOF); }
        if (type == B_HOUSE && chance(.6f)) addProp(M_CUBE, V3(x0 + w * .75f, top + 1.6f, z0 + d * .3f), V3(.7f, 2.f, .7f), 0, rgba(110, 70, 60), bi, L_ROOF);
    } else {
        addProp(M_CUBE, V3(x0 + w / 2, top + .1f, z0 + d / 2), V3(w + .3f, .2f, d + .3f), 0, roofC, bi, L_ROOF, TX_CONCRETE);
        addProp(M_CUBE, V3(x0 + w / 2, top + .45f, z0 + .1f), V3(w + .3f, .5f, .2f), 0, mulC(roofC, .8f), bi, L_ROOF);
        addProp(M_CUBE, V3(x0 + w / 2, top + .45f, z1 - .1f), V3(w + .3f, .5f, .2f), 0, mulC(roofC, .8f), bi, L_ROOF);
        addProp(M_CUBE, V3(x0 + .1f, top + .45f, z0 + d / 2), V3(.2f, .5f, d + .3f), 0, mulC(roofC, .8f), bi, L_ROOF);
        addProp(M_CUBE, V3(x1 - .1f, top + .45f, z0 + d / 2), V3(.2f, .5f, d + .3f), 0, mulC(roofC, .8f), bi, L_ROOF);
        for (int i = 0; i < irand(1, 3); i++) addProp(M_CUBE, V3(x0 + frand(1.5f, w - 1.5f), top + .7f, z0 + frand(1.5f, d - 1.5f)), V3(frand(1, 2), 1, frand(1, 1.6f)), 0, rgba(120, 126, 134), bi, L_ROOF);
    }
    if (!B.enter) return bi;
    // furniture & loot spots by type
    Color wood = rgba(110, 80, 52), metal = rgba(100, 106, 112), white = rgba(200, 200, 196);
    auto rp = [&](float m) { return V2(frand(x0 + m, x1 - m), frand(z0 + m, z1 - m)); };
    switch (type) {
    case B_HOUSE: if (!rooms.empty() && AS("interior/Bed_Single") >= 0) {
        for (auto& r : rooms) { furnB = {true, r.x0, r.z0, r.x1, r.z1}; float rw = r.x1 - r.x0, rd = r.z1 - r.z0;
            switch (r.kind) {
            case 0: furnA(bi, "interior/Couch_Medium1", r.x0 + rw * .5f, r.z1, 2, chance(.4f) ? LT_HOUSE : -1); furnA(bi, "interior/Table_RoundSmall", r.x0 + rw * .5f, r.z0 + rd * .45f, 0, LT_HOUSE);
                furnA(bi, chance(.5f) ? "interior/Bookshelf" : "interior/Drawer_1", r.x1, r.z0 + rd * .3f, 1, LT_HOUSE); if (chance(.6f)) furnA(bi, "interior/Carpet_1", r.x0 + rw * .5f, r.z0 + rd * .55f, irand(0, 3), -1, .5f, false);
                if (chance(.5f)) furnA(bi, "interior/Houseplant_1", r.x0, r.z0, 0, -1, .5f, false); break;
            case 1: furnA(bi, "interior/Kitchen_Fridge", r.x0, r.z0, 0, LT_KITCHEN); furnA(bi, "interior/Kitchen_2Drawers", r.x0 + 1.2f, r.z0, 0, LT_KITCHEN); furnA(bi, "interior/Kitchen_Oven", r.x0 + 2.2f, r.z0, 0, chance(.5f) ? LT_KITCHEN : -1);
                if (rw > 3.4f) furnA(bi, "interior/Kitchen_Sink", r.x0 + 3.2f, r.z0, 0, -1); if (rd > 3.2f) { furnA(bi, "interior/Table_RoundSmall", r.x0 + rw * .55f, r.z1 - 1.2f, 0, LT_KITCHEN); furnA(bi, "interior/Chair_1", r.x0 + rw * .55f - .75f, r.z1 - 1.2f, 3, -1, .5f, false); } break;
            case 2: furnA(bi, rw * rd > 11 && chance(.6f) ? "interior/Bed_King" : "interior/Bed_Single", r.x1, r.z1, 2, LT_HOUSE); furnA(bi, "interior/NightStand_1", r.x1 - 2.1f, r.z1, 2, LT_HOUSE);
                furnA(bi, "interior/Drawer_1", r.x0, r.z0 + rd * .4f, 3, LT_WARDROBE); if (chance(.5f)) blds[bi].spots.push_back({V3(r.x0 + .5f, .1f, r.z1 - .5f), LT_WARDROBE}); break;
            case 3: furnA(bi, "interior/Bathroom_Toilet", r.x0, r.z0, 0, -1, .5f); if (chance(.5f)) blds[bi].spots.push_back({V3(r.x0 + rw * .6f, .9f, r.z0 + .5f), LT_MED}); break; } }
        furnB.on = false; if (B.cond == 2 && chance(.15f)) blds[bi].spots.push_back({V3(rooms.back().x0 + .6f, .1f, rooms.back().z0 + .6f), LT_CACHE}); // someone's stash, missed
        break; }
    [[fallthrough]];
    case B_CABIN: {
        if (!furnA(bi, "interior/Kitchen_Fridge", x0 + .5f, z0 + .4f, 0, LT_KITCHEN)) { furn(bi, x0 + 1.0f, z0 + 1.2f, 1.6f, .6f, .9f, wood, LT_KITCHEN); furn(bi, x1 - 1.3f, z1 - 1.4f, 1.4f, 2.0f, .5f, rgba(130, 60, 60), LT_HOUSE); break; }
        furnA(bi, "interior/Kitchen_2Drawers", x0 + 1.7f, z0 + .3f, 0, LT_KITCHEN); furnA(bi, "interior/Kitchen_Oven", x0 + 2.75f, z0 + .3f, 0, chance(.5f) ? LT_KITCHEN : -1);
        furnA(bi, chance(.5f) ? "interior/Bed_King" : "interior/Bed_Single", x1 - .8f, z1 - 1.2f, 2, LT_HOUSE); furnA(bi, "interior/NightStand_1", x1 - .3f, z1 - 2.6f, 1, LT_WARDROBE);
        if (w > 7.5f) furnA(bi, "interior/Couch_Medium1", x0 + 1.4f, z1 - .5f, 2, chance(.4f) ? LT_HOUSE : -1);
        { Vector2 t = V2(x0 + w * .35f, z0 + d * .55f); furnA(bi, "interior/Table_RoundSmall", t.x, t.y, 0, LT_HOUSE); furnA(bi, "interior/Chair_1", t.x - .75f, t.y, 3, -1, .5f, false); furnA(bi, "interior/Chair_1", t.x + .75f, t.y, 1, -1, .5f, false); }
        furnA(bi, chance(.5f) ? "interior/Bookshelf" : "interior/Drawer_1", x1 - .3f, z0 + 1.2f, 1, LT_HOUSE);
        if (chance(.6f)) furnA(bi, "interior/Houseplant_1", x0 + .35f, z1 - 1.6f, 0, -1, .5f, false);
        if (chance(.6f)) furnA(bi, "interior/Carpet_1", x0 + w * .55f, z0 + d * .6f, irand(0, 3), -1, .5f, false);
        break; }
    case B_STORE:
        for (float sz = z0 + 2.2f; sz < z1 - 1.5f; sz += 2.4f) { int ai = AS("interior/Shelf_Large"); if (ai >= 0) { float sc = std::min(.6f, (w * .5f) / std::max(.1f, assets[ai].size.x)); { int tb = chance(.3f) ? LT_CLOTHES : LT_STORE; furnA(bi, "interior/Shelf_Large", x0 + w * .45f, sz, 0, tb, sc); if (chance(.6f)) blds[bi].spots.push_back({V3(x0 + w * .45f + frand(-1, 1), .9f, sz), tb}); } }
            else furn(bi, x0 + w * .45f, sz, w * .55f, .5f, 1.4f, metal, LT_STORE); }
        furn(bi, x1 - 1.2f, z0 + 1.0f, 1.8f, .8f, 1.0f, wood, LT_STORE); break;
    case B_POLICE:
        if (!furnA(bi, "interior/Kitchen_2Drawers", x0 + 1.5f, z0 + .4f, 0, LT_OFFICE)) furn(bi, x0 + 1.5f, z0 + 1.2f, 2.2f, .8f, .8f, wood, LT_OFFICE);
        furnA(bi, "interior/Chair_1", x0 + 1.5f, z0 + 1.3f, 2, -1, .5f, false);
        if (!furnA(bi, "interior/Bookshelf", x1 - .3f, z0 + 1.5f, 1, LT_POLICE)) furn(bi, x1 - .6f, z0 + 1.5f, .6f, 2.4f, 1.8f, metal, LT_POLICE);
        if (!furnA(bi, "interior/Drawer_1", x1 - .3f, z1 - 1.5f, 1, LT_POLICE)) furn(bi, x1 - .6f, z1 - 1.5f, .6f, 2.4f, 1.8f, metal, LT_POLICE);
        { Vector2 t = rp(2.5f); if (!furnA(bi, "interior/Table_RoundSmall", t.x, t.y, 0, LT_POLICE)) furn(bi, t.x, t.y, 1.4f, .8f, .8f, wood, LT_POLICE); } break;
    case B_CLINIC:
        for (int i = 0; i < 3; i++) if (!furnA(bi, "interior/Bed_Single", x0 + 1.0f + i * 1.6f, z1 - 1.1f, 2, LT_MED)) furn(bi, x0 + 1.4f + i * 2.4f, z1 - 1.3f, 1.0f, 2.0f, .7f, white, LT_MED);
        if (!furnA(bi, "interior/Shelf_Small1", x1 - .3f, z0 + 2, 1, LT_MED)) furn(bi, x1 - .6f, z0 + 2, .6f, 2.4f, 1.8f, white, LT_MED);
        if (!furnA(bi, "interior/Kitchen_Sink", x0 + 1.2f, z0 + .3f, 0, LT_MED)) furn(bi, x0 + 2, z0 + 1.0f, 2.4f, .7f, .9f, white, LT_MED);
        furnA(bi, "interior/Kitchen_2Drawers", x0 + 2.3f, z0 + .3f, 0, LT_MED); break;
    case B_GARAGE: case B_BARN: case B_HANGAR:
        furn(bi, x0 + .7f, z0 + d / 2, .8f, d * .5f, 1.0f, metal, type == B_BARN ? LT_BARN : LT_GARAGE);
        furn(bi, x1 - 1.5f, z1 - 1.5f, 2.f, 2.f, 1.2f, rgba(170, 150, 90), type == B_BARN ? LT_BARN : LT_GARAGE);
        if (type == B_HANGAR) { furn(bi, x0 + w / 2, z0 + d / 2, 6, 3, 1.8f, rgba(70, 80, 60), LT_MIL); }
        if (type == B_BARN) { // hayloft over the back third, ladder at its edge
            float ly = 3.4f, lw = w * .38f; addProp(M_CUBE, V3(x1 - lw / 2 - .1f, ly, z0 + d / 2), V3(lw, .2f, d - .4f), 0, rgba(120, 90, 60), bi, L_LOFT, TX_PLANK);
            addProp(M_CUBE, V3(x1 - lw - .1f, ly + .5f, z0 + d * .3f), V3(.08f, .08f, d * .5f), 0, rgba(100, 78, 54), bi, L_LOFT); // railing along the open edge
            for (int k = 0; k < 4; k++) addProp(M_CUBE, V3(x1 - .7f, ly + .45f, z0 + 1.0f + k * 1.25f), V3(1.1f, .7f, .8f), frand(-.2f, .2f), rgba(196, 170, 90), bi, L_LOFT);
            if (chance(.5f)) addProp(M_CUBE, V3(x1 - lw * .5f, ly + .25f, z1 - 1.4f), V3(.8f, .3f, 1.9f), 0, rgba(90, 96, 70), bi, L_LOFT); // someone's cot
            float lx = x1 - lw - .25f, lz = z0 + d * .7f; addProp(M_CUBE, V3(lx, ly / 2, lz - .25f), V3(.08f, ly + .4f, .08f), 0, rgba(90, 70, 50), bi, L_ALWAYS); addProp(M_CUBE, V3(lx, ly / 2, lz + .25f), V3(.08f, ly + .4f, .08f), 0, rgba(90, 70, 50), bi, L_ALWAYS);
            for (float ry = .4f; ry < ly; ry += .45f) addProp(M_CUBE, V3(lx, ry, lz), V3(.06f, .06f, .55f), 0, rgba(100, 78, 54), bi, L_ALWAYS);
            lofts.push_back({bi, V2(lx - .7f, lz), V2(lx + .7f, lz), ly + .12f, x1 - lw - .05f, z0 + .3f, x1 - .3f, z1 - .3f}); blds[bi].spots.push_back({V3(x1 - lw * .5f, ly + .2f, z0 + 1.5f), LT_BARN}); }
        { int g = AS("items/GasCan"); if (g >= 0) for (int k = 0; k < 2; k++) addAProp(g, V3(x0 + 1.5f + k * .6f, .06f, z1 - .6f), fitScale(g, .45f, 1), frand(0, 6), bi); }
        { int tc = AS("items/Trashcan"); if (tc >= 0) addAProp(tc, V3(x1 - .6f, .06f, z0 + .6f), fitScale(tc, .9f, 1), 0, bi); }
        break;
    case B_SHED:
        furn(bi, x0 + w / 2, z1 - .45f, w - .8f, .55f, 1.6f, rgba(110, 96, 74), LT_SHED); furn(bi, x0 + .45f, z0 + d / 2, .6f, d * .5f, .9f, rgba(120, 100, 70), LT_SHED);
        { int g = AS("items/GasCan"); if (g >= 0 && chance(.5f)) addAProp(g, V3(x1 - .5f, .06f, z0 + .6f), fitScale(g, .45f, 1), frand(0, 6), bi); } break;
    case B_FACTORY: {
        for (float mx = x0 + 4; mx < x1 - 4; mx += 6) for (float mz = z0 + 4; mz < z1 - 3; mz += 5.5f) if (chance(.75f)) furn(bi, mx, mz, frand(2.4f, 3.6f), frand(1.4f, 2.2f), frand(1.2f, 2.2f), pick(std::vector<Color>{rgba(90, 104, 110), rgba(150, 120, 50), rgba(80, 84, 88)}), chance(.45f) ? LT_FACTORY : -1);
        for (float sz = z0 + 1.5f; sz < z1 - 1.5f; sz += 3.f) furn(bi, x1 - .6f, sz, .7f, 2.4f, 2.2f, rgba(70, 76, 84), LT_FACTORY);
        int tc = AS("items/Trashcan"); if (tc >= 0) addAProp(tc, V3(x0 + .7f, .06f, z1 - .7f), fitScale(tc, .9f, 1), 0, bi); break; }
    case B_FIRE:
        car(x0 + w * .35f, z0 + d / 2, PI / 2, 0, VK_FIRE);
        for (float sz = z0 + 1.2f; sz < z1 - 1; sz += 1.4f) furn(bi, x1 - .45f, sz, .55f, 1.1f, 1.9f, rgba(150, 36, 30), LT_FIRE);
        furn(bi, x1 - 3.f, z0 + .6f, 2.4f, .8f, .9f, metal, LT_FIRE); break;
    case B_SCHOOL:
        for (float dz = z0 + 2.5f; dz < z1 - 2; dz += 1.8f) for (float dx = x0 + 2.5f; dx < x0 + w * .6f; dx += 1.6f) furn(bi, dx, dz, .8f, .55f, .75f, rgba(150, 120, 80), chance(.12f) ? LT_SCHOOL : -1);
        for (float lz = z0 + 1; lz < z1 - 1; lz += 1.f) furn(bi, x1 - .35f, lz, .45f, .9f, 1.8f, pick(std::vector<Color>{rgba(60, 90, 140), rgba(150, 50, 40), rgba(60, 110, 70)}), chance(.35f) ? LT_SCHOOL : -1);
        furn(bi, x0 + w * .75f, z0 + 1.2f, 1.8f, .8f, .8f, wood, LT_SCHOOL); break;
    case B_TRAILER:
        if (!furnA(bi, "interior/Bed_Single", x1 - 1.0f, z0 + d / 2, 1, LT_TRAILER)) furn(bi, x1 - 1.2f, z0 + d / 2, 2.0f, 1.0f, .6f, rgba(120, 100, 90), LT_TRAILER);
        furn(bi, x0 + 1.2f, z0 + .4f, 2.0f, .6f, .9f, rgba(170, 160, 140), LT_KITCHEN); furn(bi, x0 + w * .45f, z1 - .5f, 2.2f, .8f, .5f, rgba(110, 80, 70), LT_TRAILER); break;
    case B_GAS:
        for (float sz = z0 + 2.f; sz < z1 - 1.5f; sz += 2.f) furn(bi, x0 + w * .4f, sz, w * .45f, .5f, 1.4f, metal, LT_GAS);
        furn(bi, x1 - 1.2f, z0 + 1.0f, 1.6f, .8f, 1.0f, wood, LT_GAS); furn(bi, x1 - .5f, z1 - 1.5f, .6f, 2.0f, 1.8f, rgba(220, 220, 225), LT_GAS); break;
    case B_BARRACKS:
        for (float bx = x0 + 1.2f; bx < x1 - 1; bx += 2.2f) { if (!furnA(bi, "interior/Bed_Bunk", bx, z0 + 1.2f, 0, LT_MIL)) furn(bi, bx, z0 + 1.3f, 1.0f, 2.0f, .6f, rgba(80, 90, 70), LT_MIL);
            if (!furnA(bi, "interior/Bed_Bunk", bx, z1 - 1.2f, 2, chance(.5f) ? LT_MIL : -1)) furn(bi, bx, z1 - 1.3f, 1.0f, 2.0f, .6f, rgba(80, 90, 70), chance(.5f) ? LT_MIL : -1); }
        break;
    case B_CHURCH: for (float bz = z0 + 2; bz < z1 - 2; bz += 1.8f) { furn(bi, x0 + w * .3f, bz, w * .3f, .5f, .5f, wood, chance(.2f) ? LT_HOUSE : -1); furn(bi, x0 + w * .7f, bz, w * .3f, .5f, .5f, wood, -1); } break;
    default: break;
    }
    return bi;
}
static void aoRect(float x0, float z0, float x1, float z1, float R, float k);
