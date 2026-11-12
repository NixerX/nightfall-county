// Nightfall County: inventory ui.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ---- DayZ-style inventory: paper doll on the left, pockets in the middle, the ground on the right. Drag stuff around.
struct DragSt { bool on = false; InvItem it; int from = 0; std::shared_ptr<Cloth> owner; int ox = 0, oy = 0; bool orot = false; int slot = -1; Vector3 lp{}; }; // from: 0 pocket, 1 worn slot, 2 ground
static DragSt drag; static struct { bool on = false; InvItem* it = nullptr; Vector2 at{}; } ctxMenu; static float invScroll = 0;
constexpr float CELL = 50;
static void itemColorName(const InvItem& it, Color& c, std::string& n) { if (it.kind == 1) { c = it.c->col; n = it.c->name; } else { c = IDEF[it.id].c; n = IDEF[it.id].n; } }
static void drawItemBox(const InvItem& it, Rectangle r, bool hov) {
    Color c; std::string n; itemColorName(it, c, n); bool set = it.kind == 1 && it.c->set >= 0; DrawRectangleRec(r, hov ? rgba(46, 54, 66, 235) : rgba(26, 30, 38, 235));
    if (set) { float g = .55f + .45f * sinf((float)GetTime() * 3); DrawRectangleLinesEx({r.x - 1, r.y - 1, r.width + 2, r.height + 2}, 2, withA(SETS[it.c->set].c, g)); }
    else DrawRectangleLinesEx(r, 1, it.kind == 1 ? RARC[it.c->rar] : rgba(90, 96, 106));
    DrawRectangle((int)r.x + 3, (int)r.y + 3, (int)r.width - 6, 3, c);
    const char* ic = it.kind == 1 ? clothIcon(it.c->def) : ITEM_ICON[it.id]; if (icon(ic)) drawIcon(ic, r, iconTint(c)); else { std::string sn = n.size() > (size_t)(r.width / 7) ? n.substr(0, (size_t)(r.width / 7)) : n; text(sn, r.x + 4, r.y + 11, 11, BONE); }
    if (it.kind == 1 && it.c->pattern == 2) for (int k = 0; k < 3; k++) DrawCircle((int)(r.x + r.width * (.3f + .2f * k)), (int)(r.y + r.height * .78f), 3, mulC(c, .6f));
    if (it.kind == 1 && !it.c->items.empty()) DrawCircle((int)(r.x + r.width - 7), (int)(r.y + 12), 3, SODIUM);
    if (set) text("SET", r.x + 4, r.y + 7, 10, SETS[it.c->set].c);
    if (it.kind == 0 && it.n > 1) text("x" + std::to_string(it.n), r.x + r.width - 4, r.y + r.height - 15, 12, STEEL, 2);
    if (it.kind == 1) { float cd = it.c->cond; DrawRectangle((int)r.x + 3, (int)(r.y + r.height - 5), (int)((r.width - 6) * cd), 2, cd > .55f ? OKC : cd > .3f ? SODIUM : BLOODC); }
}
static void tooltip(const InvItem& it, Vector2 m) {
    std::vector<std::pair<std::string, Color>> L;
    if (it.kind == 1) { const Cloth& c = *it.c; const CDef& d = CD[c.def]; static const char* RN[4] = {"Common", "Uncommon", "Rare", "Legendary"};
        L.push_back({c.name, RARC[c.rar]}); L.push_back({std::string(RN[c.rar]) + " " + CSN[d.slot] + "   " + CONDN[condTier(c.cond)] + " (" + std::to_string((int)(c.cond * 100)) + "%)", STEEL});
        if (!c.decor.empty()) L.push_back({"\"" + c.decor + "\"", rgba(200, 190, 160)});
        if (c.set >= 0) { const SetDef& S = SETS[c.set]; int n = setCount(c.set); L.push_back({std::string(S.n) + " set  (" + std::to_string(n) + "/4 worn)", S.c});
            L.push_back({std::string("2: ") + S.b2, n >= 2 ? OKC : STEEL}); L.push_back({std::string("4: ") + S.b4, n >= 4 ? OKC : STEEL}); L.push_back({S.bc, n >= 2 && P.cls == c.set ? OKC : STEEL}); }
        char b[96]; snprintf(b, 96, "Warmth %d   Waterproof %d%%   Bite armor %d%%", (int)(clothWarm(c) * 100), (int)(clothWater(c) * 100), (int)(clothArmor(c) * 100)); L.push_back({b, BONE});
        if (gridH(c)) { snprintf(b, 96, "Storage %dx%d (%d slots)%s", gridW(c), gridH(c), gridW(c) * gridH(c), c.items.empty() ? "" : "  - has stuff in it"); L.push_back({b, BONE}); }
        float nz = d.noise - c.bQuiet, vs = d.vis * c.bVis; if (nz > 1.02f) L.push_back({"Noisy", rgba(255, 140, 110)}); else if (nz < .98f) L.push_back({"Quiet", OKC});
        if (vs > 1.02f) L.push_back({"Bright - easy to spot", rgba(255, 140, 110)}); else if (vs < .98f) L.push_back({"Camouflaged", OKC});
        if (c.upg) L.push_back({"Leather patches: " + std::to_string(c.upg) + "/3", rgba(200, 160, 110)}); }
    else { const ItemDef& d = IDEF[it.id]; L.push_back({d.n, BONE}); if (it.id >= IT_A9 && it.id <= IT_A308) { // ammo: caliber, rarity and every weapon that takes it
        int cal = 0; for (int c = 0; c < C_COUNT; c++) if (ammoItem(c) == it.id) cal = c;
        std::string fits; for (int i = 0; i < W_COUNT; i++) if (WB[i].cal == cal) { if (!fits.empty()) fits += ", "; fits += WB[i].n; }
        L.push_back({std::string("Ammo  -  ") + CALN[cal] + "  -  Common", STEEL}); L.push_back({"Fits: " + fits, BONE}); } if (d.food || d.water) { char b[64]; snprintf(b, 64, "Food %+d   Water %+d", (int)d.food, (int)d.water); L.push_back({b, STEEL}); }
        if (it.id == IT_RAG) L.push_back({"Stops bleeding (sometimes). Patches clothes.", STEEL}); else if (it.id == IT_SEWKIT) L.push_back({"Repairs clothing a lot.", STEEL}); else if (it.id == IT_TAPE) L.push_back({"Repairs clothing up to Worn.", STEEL});
        else if (it.id == IT_LEATHER) L.push_back({"Sew onto clothes: +bite armor (3 max).", STEEL}); else if (it.id == IT_WAX) L.push_back({"Waterproofs clothing.", STEEL}); else if (it.id >= IT_DYE_BLACK && it.id <= IT_DYE_BROWN) L.push_back({"Dye clothing. Darker = harder to spot.", STEEL});
        L.push_back({"Right-click: use / options", rgba(150, 156, 166)}); }
    float w = 0; for (auto& l : L) w = std::max(w, textW(l.first, 13)); w += 20; float h = L.size() * 18.f + 12; float x = std::min(m.x + 16, W - w - 6), y = std::min(m.y + 16, H - h - 6);
    DrawRectangleRec({x, y, w, h}, rgba(8, 10, 14, 245)); DrawRectangleLinesEx({x, y, w, h}, 1, rgba(80, 86, 96)); for (int i = 0; i < (int)L.size(); i++) text(L[i].first, x + 10, y + 7 + i * 18.f, 13, L[i].second);
}
static void dropToGround(InvItem it) { Vector2 at = Vector2Add(P.p, polar(frand(0, 6), .5f)); if (it.kind == 1) dropCloth(it.c, at); else spawnItem(xz3(at, .05f), it.id, it.n); }
static void cancelDrag() {
    if (!drag.on) return; drag.on = false; InvItem it = drag.it;
    if (drag.from == 0 && drag.owner) { it.x = drag.ox; it.y = drag.oy; it.rot = drag.orot; drag.owner->items.push_back(it); }
    else if (drag.from == 1 && drag.slot >= 0 && !P.eq[drag.slot]) P.eq[drag.slot] = it.c;
    else if (drag.from == 2) { if (it.kind == 1) spawnCloth(drag.lp, it.c); else spawnItem(drag.lp, it.id, it.n); }
    else if (!placeAnywhere(it)) dropToGround(it); }
static void drawTabs(int cur) { const char* T[2] = {"GEAR  [Tab]", "SKILLS  [K]"}; for (int i = 0; i < 2; i++) { Rectangle r = {W / 2.f - 160 + i * 164, 6, 156, 28}; bool hov = CheckCollisionPointRec(GetMousePosition(), r);
        DrawRectangleRec(r, i == cur ? rgba(40, 46, 58, 245) : hov ? rgba(28, 32, 40, 240) : rgba(14, 16, 22, 230)); DrawRectangleLinesEx(r, 1, i == cur ? SODIUM : rgba(60, 68, 80)); text(T[i], r.x + r.width / 2, r.y + 6, 14, i == cur ? BONE : STEEL, 1);
        if (hov && i != cur && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { if (i == 0) state = ST_INV; else { cancelDrag(); ctxMenu.on = false; state = ST_SKILLS; } } } }
static void gunTooltip(const Weapon& w, Vector2 m, int slot) { // hover card for a gun in the inventory's weapon slots
    std::vector<std::pair<std::string, Color>> L;
    L.push_back({w.name, RARC[w.rar]});
    L.push_back({std::string(RARN[w.rar]) + "  -  " + CALN[w.cal] + "  -  " + (slot == 1 ? "Sidearm" : "Primary"), STEEL});
    char b[96]; snprintf(b, 96, "DMG %.0f%s   RPM %d   MAG %d", w.dmg, w.pel > 1 ? (" x" + std::to_string(w.pel)).c_str() : "", (int)(60 / w.rate), w.mag); L.push_back({b, BONE});
    L.push_back({"Loaded " + std::to_string(w.clip) + "/" + std::to_string(w.mag) + "   Pack: " + std::to_string(invCount(ammoItem(w.cal))) + " " + CALN[w.cal] + " rounds", BONE});
    for (int id : w.aff) L.push_back({std::string("* ") + AFF[id].desc, RARC[1]});
    L.push_back({"Right-click: drop it", rgba(150, 156, 166)});
    float tw = 0; for (auto& l : L) tw = std::max(tw, textW(l.first, 13)); tw += 20; float th = L.size() * 18.f + 12; float x = std::min(m.x + 16, W - tw - 6), y = std::min(m.y + 16, H - th - 6);
    DrawRectangleRec({x, y, tw, th}, rgba(8, 10, 14, 245)); DrawRectangleLinesEx({x, y, tw, th}, 1, RARC[w.rar]); for (int i = 0; i < (int)L.size(); i++) text(L[i].first, x + 10, y + 7 + i * 18.f, 13, L[i].second);
}
