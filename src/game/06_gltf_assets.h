// Nightfall County: gltf assets.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ glTF assets (Quaternius, CC0)
static std::string assetDir;
struct Asset { Model m{}; BoundingBox bb{}; Vector3 size{}; std::vector<std::string> matName; std::vector<Color> base; bool skinned = false; Rig rig; ModelAnimation anim{}; Transform* fp = nullptr; Image tex{}; };
static std::vector<Asset> assets; static std::map<std::string, int> assetIdx;
static std::vector<std::string> glbMaterialNames(const std::string& path) {
    std::vector<std::string> out; std::ifstream f(path, std::ios::binary); if (!f) return out;
    char hdr[20]; f.read(hdr, 20); unsigned len; memcpy(&len, hdr + 12, 4); std::string j(len, ' '); f.read(&j[0], len);
    size_t m = j.find("\"materials\""); if (m == std::string::npos) return out; size_t a = j.find('[', m); int depth = 0; size_t e = a;
    for (; e < j.size(); e++) { if (j[e] == '[' || j[e] == '{') depth++; else if (j[e] == ']' || j[e] == '}') { depth--; if (depth == 0) break; } }
    std::string arr = j.substr(a, e - a); int d = 0;
    for (size_t i = 0; i < arr.size(); i++) { char ch = arr[i]; if (ch == '{' || ch == '[') d++; else if (ch == '}' || ch == ']') d--;
        else if (d == 2 && arr.compare(i, 7, "\"name\":") == 0) { size_t q0 = arr.find('"', i + 7), q1 = arr.find('"', q0 + 1); out.push_back(arr.substr(q0 + 1, q1 - q0 - 1)); i = q1; } }
    return out;
}
static int loadAsset(const std::string& key) {
    std::string path = assetDir + key + ".glb"; if (!FileExists(path.c_str())) return -1;
    Asset a; a.m = LoadModel(path.c_str()); if (a.m.meshCount == 0) return -1;
    std::vector<std::string> names = glbMaterialNames(path); a.matName.assign(a.m.materialCount, "");
    for (int i = 1; i < a.m.materialCount && i - 1 < (int)names.size(); i++) a.matName[i] = names[i - 1];
    for (int i = 0; i < a.m.meshCount; i++) if (a.m.meshes[i].boneIds) a.skinned = true;
    for (int i = 0; i < a.m.materialCount; i++) {
        Color& c = a.m.materials[i].maps[MATERIAL_MAP_DIFFUSE].color;
        c = rgba((int)(255 * powf(c.r / 255.f, 1 / 2.2f)), (int)(255 * powf(c.g / 255.f, 1 / 2.2f)), (int)(255 * powf(c.b / 255.f, 1 / 2.2f)), c.a);
        a.base.push_back(c); a.m.materials[i].shader = a.skinned ? litSkin : lit;
    }
    if (a.skinned) { a.rig = makeRig(a.m); a.fp = (Transform*)MemAlloc(sizeof(Transform) * a.m.boneCount); a.anim.boneCount = a.m.boneCount; a.anim.frameCount = 1; a.anim.bones = a.m.bones; }
    a.bb = GetModelBoundingBox(a.m); a.size = Vector3Subtract(a.bb.max, a.bb.min);
    for (int i = 0; i < a.m.materialCount; i++) { Texture2D t = a.m.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture; if (t.id && t.id != rlGetTextureIdDefault() && t.width > 4) { a.tex = LoadImageFromTexture(t); ImageFormat(&a.tex, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8); break; } } // palette textures get baked into vertex colors
    assets.push_back(a); int idx = (int)assets.size() - 1; if (assets[idx].skinned) assets[idx].anim.framePoses = &assets[idx].fp;
    assetIdx[key] = idx; return idx;
}
static int AS(const std::string& key) { auto it = assetIdx.find(key); return it == assetIdx.end() ? -1 : it->second; }
static bool haveChars = false;
static const char* CHAR_KEYS[] = {"chars/M_Casual", "chars/M_Casual2", "chars/M_Farmer", "chars/M_Punk", "chars/M_Suit", "chars/M_Worker", "chars/M_Beach", "chars/M_Adventurer", "chars/M_Swat",
    "chars/F_Casual", "chars/F_Punk", "chars/F_Suit", "chars/F_Worker", "chars/F_Formal", "chars/F_Adventurer", "chars/F_Soldier"};
static void findAssetDir() {
    std::vector<std::string> c = {std::string(GetApplicationDirectory()) + "assets/", std::string(GetApplicationDirectory()) + "../assets/", "assets/", "../assets/"};
    for (auto& d : c) if (FileExists((d + "chars/M_Casual.glb").c_str())) { assetDir = d; return; }
}
static std::string texDir() { if (assetDir.empty()) findAssetDir(); return assetDir.empty() ? std::string("assets/textures/") : assetDir + "textures/"; }
static void loadAllAssets() {
    assets.reserve(256); findAssetDir(); if (assetDir.empty()) { TraceLog(LOG_WARNING, "NIGHTFALL: assets folder not found, using blocky fallback models"); return; }
    for (const char* k : CHAR_KEYS) loadAsset(k);
    haveChars = AS("chars/M_Casual") >= 0;
    const char* rest[] = {"nature/PineTree_1", "nature/PineTree_2", "nature/PineTree_3", "nature/PineTree_4", "nature/PineTree_5", "nature/CommonTree_1", "nature/CommonTree_2", "nature/CommonTree_3", "nature/CommonTree_4", "nature/CommonTree_5",
        "nature/BirchTree_1", "nature/BirchTree_2", "nature/BirchTree_3", "nature/CommonTree_Dead_1", "nature/CommonTree_Dead_2", "nature/PineTree_Autumn_1", "nature/CommonTree_Autumn_2", "nature/Bush_1", "nature/Bush_2", "nature/BushBerries_1",
        "nature/Rock_1", "nature/Rock_2", "nature/Rock_3", "nature/Rock_4", "nature/Rock_Moss_1", "nature/Rock_Moss_2", "nature/Rock_Moss_3", "nature/TreeStump", "nature/WoodLog", "nature/WoodLog_Moss", "nature/Grass", "nature/Grass_2", "nature/Plant_1",
        "interior/Bed_Single", "interior/Bed_King", "interior/Bed_Bunk", "interior/Couch_Medium1", "interior/Couch_Small1", "interior/Table_RoundSmall", "interior/Chair_1", "interior/Kitchen_Fridge", "interior/Kitchen_Oven", "interior/Kitchen_2Drawers",
        "interior/Kitchen_Sink", "interior/Bookshelf", "interior/Drawer_1", "interior/Shelf_Large", "interior/Shelf_Small1", "interior/Bathroom_Toilet", "interior/Trashcan_Large", "interior/NightStand_1", "interior/Houseplant_1", "interior/Carpet_1",
        "items/Pistol_1", "items/Pistol_2", "items/Revolver_1", "items/Shotgun_1", "items/Shotgun_2", "items/Shotgun_ShortStock", "items/Knife", "items/Axe", "items/Bandages", "items/FirstAidKit", "items/WaterBottle_1", "items/Can_Closed", "items/Can_Red",
        "items/Battery_Small", "items/PropaneTank", "items/Tent", "items/Backpack", "items/GasCan", "items/Trashcan", "items/Bonfire_Fire", "items/FlareGun", "items/Pot",
        "street/Streetlight_Single", "street/Streetlight_Double", "street/TrafficLight", "street/Sign_Stop", "street/Sign_NoParking",
        "vehicles/sedan", "vehicles/sedan-sports", "vehicles/hatchback-sports", "vehicles/suv", "vehicles/suv-luxury", "vehicles/van", "vehicles/taxi", "vehicles/police", "vehicles/ambulance", "vehicles/firetruck",
        "vehicles/delivery", "vehicles/delivery-flat", "vehicles/truck", "vehicles/truck-flat", "vehicles/garbage-truck", "vehicles/tractor", "vehicles/tractor-shovel",
        "vehicles/debris-door", "vehicles/debris-tire", "vehicles/debris-bumper", "vehicles/debris-plate-a", "vehicles/debris-drivetrain-axle"};
    for (const char* k : rest) loadAsset(k);
    for (auto& a : assets) if (a.skinned) a.anim.framePoses = &a.fp;
    TraceLog(LOG_INFO, "NIGHTFALL: loaded %d models from %s", (int)assets.size(), assetDir.c_str());
}
// draw a static (unskinned) asset with optional tint and alpha
static void drawAsset(int ai, const Matrix& M, float alpha = 1, Color tint = WHITE, float darken = 1) {
    if (ai < 0) return; Asset& a = assets[ai];
    for (int i = 0; i < a.m.meshCount; i++) { int mi = a.m.meshMaterial[i]; Color c = a.base[mi]; c = rgba(c.r * tint.r / 255 * darken, c.g * tint.g / 255 * darken, c.b * tint.b / 255 * darken, (int)(255 * alpha));
        a.m.materials[mi].maps[MATERIAL_MAP_DIFFUSE].color = c; DrawMesh(a.m.meshes[i], a.m.materials[mi], MatrixMultiply(a.m.transform, M)); }
}
// fit helpers: scale so that the given axis length equals target
static float fitScale(int ai, float target, int axis = -1) { if (ai < 0) return 1; Vector3 s = assets[ai].size; float L = axis == 0 ? s.x : axis == 1 ? s.y : axis == 2 ? s.z : std::max({s.x, s.y, s.z}); return L > 1e-4f ? target / L : 1; }
// Model matrix that places the asset's bbox-center (xz) and bottom at pos, scaled & yawed
static Matrix placeM(int ai, Vector3 pos, float sc, float yaw, bool bottom = true) {
    Asset& a = assets[ai]; Vector3 c = Vector3Scale(Vector3Add(a.bb.min, a.bb.max), .5f);
    Matrix M = MatrixMultiply(MatrixMultiply(MatrixTranslate(-c.x, bottom ? -a.bb.min.y : -c.y, -c.z), MatrixScale(sc, sc, sc)), MatrixRotateY(-yaw));
    return MatrixMultiply(M, MatrixTranslate(pos.x, pos.y, pos.z));
}

// ------------------------------------------------------------ character looks (survivor / zombie palettes)
struct Look { int asset; std::vector<Color> col, col0; std::vector<int> zone; Vector3 scale; bool female; }; // zone per material: 0 skin/other, 1 torso, 2 legs, 3 feet
static std::vector<Look> looks; static std::vector<int> lookPool[7]; static int playerLooks[2] = {-1, -1};
static bool hasStr(const std::string& s, const char* k) { return s.find(k) != std::string::npos; }
static Look makeLook(int ai, int kind) { // kind: 0 survivor, 1 zombie, 2 volatile, 3 bloater, 4 brute, 5 soldier zombie, 6 runner
    Asset& a = assets[ai]; Look L; L.asset = ai; L.col = a.base; float h = 1.8f / std::max(.5f, a.size.y); L.female = hasStr(CHAR_KEYS[0], "") && a.rig.names.size() > 49;
    L.scale = V3(h, h, h);
    if (kind == 0) { // classify clothing materials by where their vertices sit on the body so worn clothes can recolor them
        L.zone.assign(L.col.size(), 0); std::vector<double> sy(L.col.size(), 0); std::vector<int> cnt(L.col.size(), 0); float y0 = a.bb.min.y, hy = std::max(.01f, a.bb.max.y - a.bb.min.y);
        for (int i = 0; i < a.m.meshCount; i++) { int mi = a.m.meshMaterial[i]; if (mi < 0 || mi >= (int)sy.size()) continue; Mesh& m = a.m.meshes[i]; for (int v = 0; v < m.vertexCount; v += 3) { sy[mi] += (m.vertices[v * 3 + 1] - y0) / hy; cnt[mi]++; } }
        for (int i = 0; i < (int)L.col.size(); i++) { const std::string& n = a.matName[i]; if (!cnt[i] || hasStr(n, "Skin") || hasStr(n, "Eye") || hasStr(n, "Hair") || hasStr(n, "Eyebrow") || hasStr(n, "Moustache")) continue;
            float my = (float)(sy[i] / cnt[i]); L.zone[i] = my > .55f && my < .86f ? 1 : my > .14f && my <= .55f ? 2 : my <= .14f ? 3 : 0; }
        L.col0 = L.col; return L; }
    static const std::vector<Color> skins = {rgba(132, 142, 108), rgba(150, 144, 134), rgba(120, 124, 98), rgba(158, 146, 140), rgba(140, 150, 120), rgba(118, 112, 110)};
    Color skin = kind == 2 ? rgba(226, 228, 232) : kind == 3 ? rgba(150, 176, 84) : kind == 4 ? rgba(124, 108, 118) : pick(skins);
    std::vector<int> cloth;
    for (int i = 0; i < (int)L.col.size(); i++) {
        const std::string& n = a.matName[i]; Color& c = L.col[i];
        if (hasStr(n, "Skin")) c = mulC(skin, hasStr(n, "Darker") ? .9f : 1);
        else if (n == "Eye") c = kind == 2 ? rgba(255, 60, 30) : rgba(210, 206, 160);
        else if (hasStr(n, "Hair") || hasStr(n, "Eyebrows") || hasStr(n, "Moustache")) c = mulC(c, kind == 2 ? .2f : .65f);
        else if (!n.empty()) { c = mulC(lerpC(c, rgba(72, 56, 42), kind == 2 ? .5f : .35f), kind == 2 ? .45f : .78f); cloth.push_back(i); }
    }
    if (!cloth.empty()) { int k = pick(cloth); L.col[k] = lerpC(L.col[k], rgba(96, 16, 16), .55f); }
    if (cloth.size() > 2 && chance(.5f)) { int k = pick(cloth); L.col[k] = lerpC(L.col[k], rgba(70, 14, 14), .4f); }
    float j = frand(.94f, 1.06f);
    if (kind == 3) L.scale = V3(h * 1.38f, h * 1.02f, h * 1.42f);
    else if (kind == 4) L.scale = V3(h * 1.34f, h * 1.28f, h * 1.34f);
    else L.scale = V3(h * j, h * j, h * j);
    return L;
}
static void buildLooks() {
    looks.clear(); for (auto& p : lookPool) p.clear(); if (!haveChars) return;
    std::vector<int> civ, sold;
    for (const char* k : CHAR_KEYS) { int ai = AS(k); if (ai < 0) continue; if (hasStr(k, "Swat") || hasStr(k, "Soldier")) sold.push_back(ai); else civ.push_back(ai); }
    auto add = [&](int pool, int ai, int kind) { looks.push_back(makeLook(ai, kind)); lookPool[pool].push_back((int)looks.size() - 1); };
    for (int r = 0; r < 3; r++) for (int ai : civ) add(0 /*walker*/, ai, 1);
    for (int ai : civ) add(1 /*runner*/, ai, 1);
    for (int r = 0; r < 2; r++) add(2 /*brute*/, pick(civ), 4);
    for (int r = 0; r < 3; r++) add(3 /*bloater*/, pick(civ), 3);
    for (int r = 0; r < 4; r++) add(4 /*volatile*/, pick(civ), 2);
    for (int r = 0; r < 3; r++) for (int ai : sold) add(5 /*soldier*/, ai, 1);
    int pm = AS("chars/M_Adventurer"), pf = AS("chars/F_Adventurer");
    if (pm >= 0) { looks.push_back(makeLook(pm, 0)); playerLooks[0] = (int)looks.size() - 1; }
    if (pf >= 0) { looks.push_back(makeLook(pf, 0)); playerLooks[1] = (int)looks.size() - 1; }
}
struct CharOut { Vector3 hand[2], head, chest; Matrix M; }; static float charY = 0;
static int outfitLook(); // applies worn clothes to the player look
static void drawCharacter(int lookIdx, const PoseIn& pin, Vector2 pos, float yaw, Color tint, float tintAmt, bool lying, CharOut* out = nullptr, float alpha = 1) {
    Look& L = looks[lookIdx]; Asset& a = assets[L.asset];
    std::vector<Quaternion> D; std::vector<float> S; Vector3 ro = buildPose(a.rig, pin, D, S); solvePose(a.rig, D, S, ro, a.fp); UpdateModelAnimationBones(a.m, a.anim, 0);
    Matrix M = MatrixMultiply(MatrixMultiply(MatrixScale(L.scale.x, L.scale.y, L.scale.z), MatrixRotateY(PI / 2 - yaw)), MatrixTranslate(pos.x, charY, pos.y));
    if (lying) M = MatrixMultiply(MatrixMultiply(MatrixRotateX(PI / 2), MatrixTranslate(0, .13f / L.scale.y, -.88f)), M);
    for (int i = 0; i < a.m.meshCount; i++) { int mi = a.m.meshMaterial[i]; Color c = L.col[mi]; if (tintAmt > 0) c = lerpC(c, tint, tintAmt); c.a = (unsigned char)(255 * alpha);
        a.m.materials[mi].maps[MATERIAL_MAP_DIFFUSE].color = c; DrawMesh(a.m.meshes[i], a.m.materials[mi], MatrixMultiply(a.m.transform, M)); }
    if (out) { for (int s = 0; s < 2; s++) out->hand[s] = a.rig.hand[s] >= 0 ? Vector3Transform(a.fp[a.rig.hand[s]].translation, M) : xz3(pos, 1.2f);
        out->head = a.rig.head >= 0 ? Vector3Transform(a.fp[a.rig.head].translation, M) : xz3(pos, 1.7f); out->chest = a.rig.chest >= 0 ? Vector3Transform(a.fp[a.rig.chest].translation, M) : xz3(pos, 1.4f); out->M = M; }
}

