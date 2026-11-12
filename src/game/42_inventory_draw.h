// Nightfall County: inventory draw.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
static void drawInventory() {
    DrawRectangle(0, 0, W, H, rgba(5, 6, 9, 160)); Vector2 m = GetMousePosition(); bool lp = IsMouseButtonPressed(MOUSE_BUTTON_LEFT), lr = IsMouseButtonReleased(MOUSE_BUTTON_LEFT), rp = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    const InvItem* hover = nullptr; Gear g = gear();
    auto panel = [](float x, float y, float w, float h, const char* t) { DrawRectangleRec({x, y, w, h}, rgba(12, 14, 20, 240)); DrawRectangleLinesEx({x, y, w, h}, 1, rgba(60, 68, 80)); text(t, x + 14, y + 10, 18, BONE); };
    bool ctxHit = false;
    if (ctxMenu.on && (lp || rp)) { /* resolved below in menu draw */ }
    // --- paper doll
    float dx = 20, dy = 40, dw = 300, dh = H - 60.f; panel(dx, dy, dw, dh, "WORN");
    { char b[128]; snprintf(b, 128, "Warmth %d  Rain %d%%  Armor %d%%", (int)(std::min(1.f, g.warm) * 100), (int)(g.water * 100), (int)(std::min(.75f, g.armor) * 100)); text(b, dx + 14, dy + 34, 12, STEEL);
      snprintf(b, 128, "Noise x%.2f  Visibility x%.2f  Wet %d%%", g.noise, g.vis, (int)(P.wet * 100)); text(b, dx + 14, dy + 50, 12, P.wet > .4f ? UVC : STEEL); }
    static const int dollOrder[CS_COUNT] = {CS_HEAD, CS_EYES, CS_FACE, CS_BACK, CS_TORSO, CS_OUTER, CS_HANDS, CS_LEGS, CS_SOCKS, CS_FEET};
    int hoverSlot = -1;
    for (int k = 0; k < CS_COUNT; k++) { int s = dollOrder[k]; Rectangle r = {dx + 14 + (k % 2) * 140.f, dy + 72 + (k / 2) * 62.f, 132, 54}; bool hov = CheckCollisionPointRec(m, r); if (hov) hoverSlot = s;
        bool ok = drag.on && drag.it.kind == 1 && CD[drag.it.c->def].slot == s;
        DrawRectangleRec(r, ok && hov ? rgba(40, 60, 40, 240) : ok ? rgba(26, 40, 28, 240) : rgba(20, 24, 30, 240)); DrawRectangleLinesEx(r, 1, rgba(54, 60, 70)); text(CSN[s], r.x + 4, r.y + 2, 10, rgba(110, 116, 126));
        if (P.eq[s]) { InvItem tmp; tmp.kind = 1; tmp.c = P.eq[s]; drawItemBox(tmp, {r.x + 3, r.y + 14, r.width - 6, r.height - 17}, hov); if (hov && !drag.on) { static InvItem hv; hv = tmp; hover = &hv; }
            if (hov && lp && !ctxMenu.on) { drag = {}; drag.on = true; drag.it = tmp; drag.from = 1; drag.slot = s; P.eq[s].reset(); } } }
    float wy = dy + 72 + 5 * 62.f + 6; text("WEAPONS", dx + 14, wy, 12, STEEL);
    for (int i = 0; i < 3; i++) { Rectangle r = {dx + 14, wy + 18 + i * 40.f, dw - 28, 34}; bool hov = CheckCollisionPointRec(m, r); DrawRectangleRec(r, hov ? rgba(30, 36, 46) : rgba(20, 24, 30)); DrawRectangleLinesEx(r, 1, rgba(54, 60, 70));
        if (i < 2) { text(i == 0 ? "PRIMARY" : "SIDEARM", r.x + 6, r.y + 2, 10, rgba(110, 116, 126)); if (P.hasGun[i]) { if (hov && !drag.on && !ctxMenu.on) gunTooltip(P.gun[i], m, i); text(P.gun[i].name, r.x + 6, r.y + 14, 13, RARC[P.gun[i].rar]); char b[48]; snprintf(b, 48, "%d/%d", P.gun[i].clip, P.gun[i].mag); text(b, r.x + r.width - 6, r.y + 14, 12, STEEL, 2);
                if (hov && rp) { Loot o; o.kind = LK_GUN; o.id = P.gun[i].key; o.w = P.gun[i]; o.p = xz3(P.p, .05f); o.rot = frand(0, 6); loot.push_back(o); P.hasGun[i] = false; if (P.active == i) P.active = 2; msg("Dropped it.", STEEL); } } else text("-", r.x + 6, r.y + 14, 13, STEEL); }
        else { text("MELEE", r.x + 6, r.y + 2, 10, rgba(110, 116, 126)); text(MB[P.melee].n, r.x + 6, r.y + 14, 13, BONE); if (hov && rp && P.melee != MK_FISTS) { Loot o; o.kind = LK_MELEE; o.id = P.melee; o.p = xz3(P.p, .05f); o.rot = frand(0, 6); loot.push_back(o); P.melee = MK_FISTS; } } }
    text("Right-click a weapon to drop it", dx + 14, wy + 140, 11, rgba(110, 116, 126));
    { float sy = wy + 162; text("SETS", dx + 14, sy, 12, STEEL); sy += 16; bool any = false;
      for (int i = 0; i < 5; i++) { int n = setCount(i); if (!n) continue; any = true; const SetDef& S = SETS[i]; text(std::string(S.n) + "  " + std::to_string(n) + "/4" + (P.cls == i ? "  (your class)" : ""), dx + 14, sy, 13, S.c); sy += 15;
          text(n >= 2 ? (n >= 4 ? S.b4 : S.b2) : "Wear 2 for a bonus", dx + 22, sy, 11, n >= 2 ? OKC : STEEL); sy += 15; if (sy > dy + dh - 20) break; }
      if (!any) text("Wear 2+ pieces of a class set for bonuses", dx + 14, sy, 11, rgba(110, 116, 126)); }
    // --- pockets
    float cx = dx + dw + 14, cy = 40, cw = CELL * 6 + 40; panel(cx, cy, cw, H - 60.f, "POCKETS"); text(std::to_string(g.used) + " / " + std::to_string(g.slots), cx + cw - 14, cy + 14, 12, STEEL, 2);
    struct Hit { std::shared_ptr<Cloth> c; int gx, gy; bool valid; int mx, my; } hit{nullptr, 0, 0, false, 0, 0};
    float yy = cy + 40 - invScroll; Rectangle clip = {cx, cy + 36, cw, H - 100.f};
    BeginScissorMode((int)clip.x, (int)clip.y, (int)clip.width, (int)clip.height);
    bool anyCont = false;
    for (int s : CONT_ORDER) { auto c = P.eq[s]; if (!c || !gridH(*c)) continue; anyCont = true; text(c->name, cx + 14, yy, 13, RARC[c->rar]); yy += 18; float gx0 = cx + 14, gy0 = yy;
        for (int y = 0; y < gridH(*c); y++) for (int x = 0; x < gridW(*c); x++) { DrawRectangle((int)(gx0 + x * CELL), (int)(gy0 + y * CELL), (int)CELL - 2, (int)CELL - 2, rgba(22, 26, 32, 240)); }
        Rectangle gr = {gx0, gy0, gridW(*c) * CELL, gridH(*c) * CELL};
        if (drag.on && CheckCollisionPointRec(m, gr) && CheckCollisionPointRec(m, clip)) { int w, h; itemDims(drag.it, w, h); int gx = (int)floorf((m.x - gx0) / CELL - w / 2.f + .5f), gy = (int)floorf((m.y - gy0) / CELL - h / 2.f + .5f);
            bool v = fits(*c, gx, gy, w, h) && !(drag.it.kind == 1 && drag.it.c == c); hit = {c, gx, gy, v, (int)((m.x - gx0) / CELL), (int)((m.y - gy0) / CELL)};
            DrawRectangle((int)(gx0 + gx * CELL), (int)(gy0 + gy * CELL), (int)(w * CELL - 2), (int)(h * CELL - 2), v ? rgba(60, 140, 70, 110) : rgba(160, 50, 40, 110)); }
        for (int k = 0; k < (int)c->items.size(); k++) { InvItem& it = c->items[k]; int w, h; itemDims(it, w, h); Rectangle r = {gx0 + it.x * CELL, gy0 + it.y * CELL, w * CELL - 2, h * CELL - 2};
            bool hov = !drag.on && !ctxMenu.on && CheckCollisionPointRec(m, r) && CheckCollisionPointRec(m, clip); drawItemBox(it, r, hov);
            if (hov) { hover = &it;
                if (lp) { if (IsKeyDown(KEY_LEFT_SHIFT) && it.kind == 0) { InvItem cp = it; c->items.erase(c->items.begin() + k); dropToGround(cp); sfx(SFX_PICK); break; }
                    drag = {}; drag.on = true; drag.it = it; drag.from = 0; drag.owner = c; drag.ox = it.x; drag.oy = it.y; drag.orot = it.rot; c->items.erase(c->items.begin() + k); hover = nullptr; break; }
                if (rp) { ctxMenu.on = true; ctxMenu.it = &it; ctxMenu.at = m; ctxHit = true; } } }
        yy += gridH(*c) * CELL + 12; }
    EndScissorMode();
    if (!anyCont) text("No pockets. Find pants or a shirt.", cx + 14, cy + 44, 13, STEEL);
    float contentH = yy + invScroll - (cy + 40); if (CheckCollisionPointRec(m, clip)) invScroll = Clamp(invScroll - GetMouseWheelMove() * 40, 0, std::max(0.f, contentH - clip.height + 20));
    // --- ground + craft
    float vx = cx + cw + 14, vw = std::min(360.f, W - vx - 20), vh = (H - 74.f) * .55f; panel(vx, 40, vw, vh, "GROUND"); Rectangle vrect = {vx, 40, vw, vh};
    std::vector<int> near; for (int i = 0; i < (int)loot.size(); i++) if (loot[i].settled && Vector2Distance(V2(loot[i].p.x, loot[i].p.z), P.p) < 2.6f) near.push_back(i);
    if (drag.on && CheckCollisionPointRec(m, vrect)) DrawRectangleLinesEx(vrect, 2, rgba(200, 160, 80));
    { float y = 70; int shown = 0; for (int li : near) { if (y > 40 + vh - 30) break; Loot& l = loot[li]; Rectangle r = {vx + 10, y, vw - 20, 28}; bool hov = !drag.on && !ctxMenu.on && CheckCollisionPointRec(m, r);
        std::string nm = l.kind == LK_GUN ? l.w.name : l.kind == LK_MELEE ? MB[l.id].n : l.kind == LK_CLOTH ? l.c->name + "  (" + CONDN[condTier(l.c->cond)] + ")" : std::string(IDEF[l.id].n) + (l.count > 1 ? " x" + std::to_string(l.count) : "");
        Color cc = l.kind == LK_GUN ? RARC[l.w.rar] : l.kind == LK_CLOTH ? RARC[l.c->rar] : BONE; DrawRectangleRec(r, hov ? rgba(36, 42, 52) : rgba(20, 24, 30)); DrawRectangle((int)r.x, (int)r.y, 4, 28, l.kind == LK_CLOTH ? l.c->col : l.kind == LK_ITEM ? IDEF[l.id].c : cc);
        { const char* ic = l.kind == LK_CLOTH ? clothIcon(l.c->def) : l.kind == LK_ITEM ? ITEM_ICON[l.id] : l.kind == LK_MELEE ? MELEE_ICON[l.id] : GUN_ICON[l.w.key]; drawIcon(ic, {r.x + 6, r.y, 28, 28}, iconTint(l.kind == LK_CLOTH ? l.c->col : l.kind == LK_ITEM ? IDEF[l.id].c : cc), .08f); }
        if (l.kind == LK_CLOTH && l.c->set >= 0) cc = SETS[l.c->set].c; text(nm, r.x + 40, r.y + 7, 13, cc);
        if (hov && l.kind == LK_CLOTH) { static InvItem hv; hv.kind = 1; hv.c = l.c; hover = &hv; } if (hov && l.kind == LK_ITEM) { static InvItem hv2; hv2.kind = 0; hv2.id = l.id; hv2.n = l.count; hover = &hv2; }
        if (hov && lp) { if (l.kind == LK_GUN || l.kind == LK_MELEE) { nearLoot = li; pickUp(); break; }
            drag = {}; drag.on = true; drag.from = 2; drag.lp = l.p; drag.it.kind = l.kind == LK_CLOTH ? 1 : 0; drag.it.id = l.id; drag.it.n = l.count; drag.it.c = l.c; loot.erase(loot.begin() + li); break; }
        if (hov && rp) { nearLoot = li; pickUp(); break; } y += 32; shown++; }
      if (near.empty()) text("Nothing here.", vx + 14, 72, 13, STEEL); }
    { float ky = 40 + vh + 10, kh = H - 20 - ky; panel(vx, ky, vw, kh, "CRAFT");
      bool rag = invCount(IT_BANDAGE) + invCount(IT_RAG) >= 1;
      struct R { const char* n; const char* need; bool ok; int what; } rs[5] = {
          {"Splint", "2 kindling + bandage/rag", invCount(IT_KINDLING) >= 2 && rag, 0}, {"Campfire", "3 kindling + 1 match, outdoors", invCount(IT_KINDLING) >= 3 && invCount(IT_MATCHES) >= 1, 1},
          {"Split a log", "1 log, axe/hatchet in hand", invCount(IT_LOG) >= 1 && canChop(P.melee), 2}, {P.shieldHP > 0 ? "Repair shield" : "Plank shield", "1 log + 3 kindling + rope or tape", invCount(IT_LOG) >= 1 && invCount(IT_KINDLING) >= 3 && (invCount(IT_ROPE) || invCount(IT_TAPE)), 3},
          {"Tear clothes into rags", "Drag a shirt here: 2-4 rags", false, 4}};
      Rectangle kr = {vx, ky, vw, kh}; if (drag.on && drag.it.kind == 1 && CheckCollisionPointRec(m, kr)) DrawRectangleLinesEx(kr, 2, OKC);
      for (int i = 0; i < 5; i++) { Rectangle r = {vx + 10, ky + 36 + i * 40.f, vw - 20, 36}; if (r.y + r.height > ky + kh) break; bool hov = !drag.on && CheckCollisionPointRec(m, r);
          DrawRectangleRec(r, hov && rs[i].ok ? rgba(30, 36, 46) : rgba(18, 22, 30)); DrawRectangle((int)r.x, (int)r.y, 4, 36, rs[i].ok ? OKC : rgba(70, 74, 80)); text(rs[i].n, r.x + 12, r.y + 3, 14, rs[i].ok ? BONE : STEEL); text(rs[i].need, r.x + 12, r.y + 20, 11, STEEL);
          if (hov && rs[i].ok && lp) { if (rs[i].what == 0) craftSplint(); else if (rs[i].what == 1) { buildFire(); state = ST_PLAY; }
              else if (rs[i].what == 3) { invTake(IT_LOG, 1); invTake(IT_KINDLING, 3); if (!invTake(IT_ROPE, 1)) invTake(IT_TAPE, 1); P.shieldHP = 120 * (1 + P.perks.block * 2); msg("Lashed planks into a shield. Hold right-click to block.", OKC); sfx(SFX_HIT_WOOD); }
              else if (rs[i].what == 2) useItem(invFind(IT_LOG)); } }
      // drop a piece of clothing on the craft panel to tear it into rags
      if (drag.on && lr && drag.it.kind == 1 && CheckCollisionPointRec(m, kr)) { auto c = drag.it.c; for (auto& in : c->items) if (!placeAnywhere(in)) dropToGround(in); int n = CD[c->def].slot == CS_OUTER || CD[c->def].slot == CS_TORSO || CD[c->def].slot == CS_LEGS ? irand(2, 4) : 1;
          int left = invAdd(IT_RAG, n); if (left) spawnItem(xz3(P.p, .05f), IT_RAG, left); msg("Tore the " + c->name + " into " + std::to_string(n) + " rags.", OKC); sfx(SFX_PICK); drag.on = false; } }
    // --- context menu
    if (ctxMenu.on && !ctxHit) {
        InvItem* it = ctxMenu.it; bool alive = false; forInv([&](InvItem& x) { if (&x == it) alive = true; });
        if (!alive) ctxMenu.on = false;
        else { struct O { std::string t; int act; std::shared_ptr<Cloth> tg; }; std::vector<O> os;
            if (it->kind == 0) { if (IDEF[it->id].use[0] && (IDEF[it->id].food || IDEF[it->id].water || it->id <= IT_SPLINT || it->id == IT_RAG)) os.push_back({IDEF[it->id].use, 0, nullptr});
                for (int s = 0; s < CS_COUNT; s++) if (P.eq[s] && toolFits(it->id, *P.eq[s])) os.push_back({std::string(IDEF[it->id].use[0] ? IDEF[it->id].use : "Use on") + ": " + P.eq[s]->name, 1, P.eq[s]}); }
            else { int s = CD[it->c->def].slot; os.push_back({P.eq[s] ? "Swap with worn " + P.eq[s]->name : std::string("Wear"), 3, nullptr}); }
            os.push_back({"Drop", 2, nullptr});
            float w = 0; for (auto& o : os) w = std::max(w, textW(o.t, 13)); w += 24; float x = std::min(ctxMenu.at.x, W - w - 4), y = std::min(ctxMenu.at.y, H - os.size() * 24.f - 8);
            DrawRectangleRec({x, y, w, os.size() * 24.f + 6}, rgba(8, 10, 14, 250)); DrawRectangleLinesEx({x, y, w, os.size() * 24.f + 6}, 1, rgba(90, 96, 106));
            int pickO = -1; for (int i = 0; i < (int)os.size(); i++) { Rectangle r = {x + 2, y + 3 + i * 24.f, w - 4, 22}; bool hov = CheckCollisionPointRec(m, r); if (hov) DrawRectangleRec(r, rgba(40, 48, 60)); text(os[i].t, r.x + 8, r.y + 4, 13, hov ? BONE : STEEL); if (hov && lp) pickO = i; }
            if (pickO >= 0) { O o = os[pickO]; ctxMenu.on = false;
                if (o.act == 0) useItem(it); else if (o.act == 1) applyTool(it, *o.tg);
                else if (o.act == 2) { InvItem cp = *it; for (int s : CONT_ORDER) if (P.eq[s]) for (int k = 0; k < (int)P.eq[s]->items.size(); k++) if (&P.eq[s]->items[k] == it) { P.eq[s]->items.erase(P.eq[s]->items.begin() + k); goto dropped; } dropped: dropToGround(cp); }
                else if (o.act == 3) { InvItem cp = *it; int s = CD[cp.c->def].slot; for (int q : CONT_ORDER) if (P.eq[q]) for (int k = 0; k < (int)P.eq[q]->items.size(); k++) if (&P.eq[q]->items[k] == it) { P.eq[q]->items.erase(P.eq[q]->items.begin() + k); goto removed; }
                    removed: { auto old = P.eq[s]; P.eq[s] = cp.c; if (old) { InvItem oi; oi.kind = 1; oi.c = old; if (!placeAnywhere(oi)) dropCloth(old, P.p); } msg("Put on " + cp.c->name, RARC[cp.c->rar]); sfx(SFX_PICK); } } }
            else if (lp || rp) ctxMenu.on = false; } }
    drawHotbar(true);
    // --- drag & drop
    if (drag.on) { if (IsKeyPressed(KEY_R)) drag.it.rot = !drag.it.rot; int w, h; itemDims(drag.it, w, h); drawItemBox(drag.it, {m.x - w * CELL / 2, m.y - h * CELL / 2, w * CELL - 2, h * CELL - 2}, true);
        if (lr) { bool done = false;
            for (int i = 0; i < 10 && !done; i++) if (CheckCollisionPointRec(m, hotRect(i)) && drag.it.kind == 0) { for (auto& h : hotbar) if (h.kind == 1 && h.id == drag.it.id) h = {}; hotbar[i] = {1, drag.it.id}; cancelDrag(); drag.on = false; done = true; sfx(SFX_PICK, .3f); }
            if (done) {} else
            if (hit.c && hit.valid) { InvItem it = drag.it; it.x = hit.gx; it.y = hit.gy;
                if (it.kind == 0) for (auto& o : hit.c->items) { int ow, oh; itemDims(o, ow, oh); if (o.kind == 0 && o.id == it.id && o.n < IDEF[it.id].stack && hit.gx >= o.x && hit.gx < o.x + ow && hit.gy >= o.y && hit.gy < o.y + oh) { int k = std::min(it.n, IDEF[it.id].stack - o.n); o.n += k; it.n -= k; } }
                if (it.n > 0) hit.c->items.push_back(it); done = true; sfx(SFX_PICK, .4f); }
            else if (hit.c && drag.it.kind == 0) { for (auto& o : hit.c->items) { int ow, oh; itemDims(o, ow, oh); // drop onto a matching stack to merge
                    if (o.kind == 0 && o.id == drag.it.id && o.n < IDEF[o.id].stack && hit.mx >= o.x && hit.mx < o.x + ow && hit.my >= o.y && hit.my < o.y + oh) { int k = std::min(drag.it.n, IDEF[o.id].stack - o.n); o.n += k; drag.it.n -= k; if (drag.it.n <= 0) done = true; break; } } }
            if (!done && hoverSlot >= 0 && drag.it.kind == 1 && CD[drag.it.c->def].slot == hoverSlot) { auto old = P.eq[hoverSlot]; P.eq[hoverSlot] = drag.it.c; if (old) { InvItem oi; oi.kind = 1; oi.c = old; if (!placeAnywhere(oi)) dropCloth(old, P.p); } done = true; sfx(SFX_PICK); msg("Put on " + drag.it.c->name, RARC[drag.it.c->rar]); }
            if (!done && hoverSlot >= 0 && drag.it.kind == 0 && P.eq[hoverSlot] && toolFits(drag.it.id, *P.eq[hoverSlot])) { InvItem t = drag.it; applyTool(&t, *P.eq[hoverSlot]); if (t.n > 0) { drag.it = t; cancelDrag(); } done = true; }
            if (!done && (CheckCollisionPointRec(m, vrect) || m.x > vx + vw || (!hit.c && hoverSlot < 0 && m.x > cx + cw))) { dropToGround(drag.it); done = true; sfx(SFX_PICK, .3f); }
            if (done) drag.on = false; else cancelDrag(); } }
    else if (hover && !ctxMenu.on) { tooltip(*hover, m); if (lp && hover->kind == 0) {} }
    drawTabs(0);
    text("Drag to move   R rotate   Drop on a slot to wear   Drag a tool onto clothes to repair   Shift-click to drop   Right-click for options", W / 2.f, H - 16.f, 11, rgba(120, 126, 136), 1);
    if (hoverGun >= 0 && P.hasGun[hoverGun]) gunTooltip(P.gun[hoverGun], m, hoverGun);
    drawCursor();
}
