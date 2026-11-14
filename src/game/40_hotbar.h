// Nightfall County: hotbar.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ---- hotbar: keys 1-0. Weapons sit in 1-3 by default; drag anything else onto a slot in the inventory
struct HotSlot {
    int kind = 0, id = -1;
};
static HotSlot hotbar[10]; // kind 0 empty, 1 item, 2 weapon slot
static void resetHotbar() {
    for (auto& h : hotbar)
        h = {};
    hotbar[0] = {2, 0};
    hotbar[1] = {2, 1};
    hotbar[2] = {2, 2};
    hotbar[3] = {1, IT_BANDAGE};
    hotbar[4] = {1, IT_RAG};
    hotbar[5] = {1, IT_WATER};
}
static Rectangle hotRect(int i) {
    float sw = 50, x0 = W / 2.f - sw * 5;
    return {x0 + i * sw + 2, H - 112.f, sw - 4, 46};
}
static void useHot(int i) {
    HotSlot& h = hotbar[i];
    if (h.kind == 2) {
        if (h.id == 2 || P.hasGun[h.id])
            setActive(h.id);
        return;
    }
    if (h.kind == 1) {
        InvItem* it = invFind(h.id);
        if (!it) {
            msg(std::string("No ") + IDEF[h.id].n + " left.", STEEL);
            return;
        }
        if (IDEF[h.id].use[0] || h.id == IT_FLARE || h.id == IT_PIPEBOMB || h.id == IT_KINDLING || h.id == IT_LOG)
            useItem(it);
        else
            msg(std::string(IDEF[h.id].n) + ": right-click it in the inventory to pick a target.", STEEL);
    }
}
static void drawHotbar(bool inv) {
    Vector2 m = GetMousePosition();
    for (int i = 0; i < 10; i++) {
        Rectangle r = hotRect(i);
        HotSlot& h = hotbar[i];
        bool act = h.kind == 2 && P.active == h.id;
        bool hov = inv && CheckCollisionPointRec(m, r);
        DrawRectangleRec(r, hov ? rgba(40, 46, 58, 230) : rgba(10, 12, 16, 190));
        DrawRectangleLinesEx(r, act ? 2 : 1, act ? SODIUM : rgba(70, 76, 86));
        text(std::to_string((i + 1) % 10), r.x + 3, r.y + 2, 10, STEEL);
        std::string nm;
        Color c = BONE;
        int n = -1;
        if (h.kind == 2) {
            if (h.id == 2)
                nm = MB[P.melee].n;
            else if (P.hasGun[h.id]) {
                nm = P.gun[h.id].name;
                c = RARC[P.gun[h.id].rar];
            } else {
                nm = h.id ? "sidearm" : "primary";
                c = rgba(90, 96, 106);
            }
        } else if (h.kind == 1) {
            nm = IDEF[h.id].n;
            n = invCount(h.id);
            if (n == 0)
                c = rgba(100, 104, 112);
            DrawRectangle((int)r.x + 3, (int)r.y + 14, (int)r.width - 6, 3, n ? IDEF[h.id].c : rgba(60, 60, 64));
        }
        {
            const char* ic = h.kind == 2   ? (h.id == 2        ? MELEE_ICON[P.melee]
                                              : P.hasGun[h.id] ? GUN_ICON[P.gun[h.id].key]
                                                               : nullptr)
                             : h.kind == 1 ? ITEM_ICON[h.id]
                                           : nullptr;
            if (ic && icon(ic)) {
                drawIcon(ic, {r.x, r.y + 6, r.width, r.height - 8},
                         h.kind == 1 ? (n ? iconTint(IDEF[h.id].c) : rgba(80, 84, 90)) : c, .2f);
                nm.clear();
            }
        }
        if (!nm.empty()) {
            size_t mx = 8;
            if (nm.size() > mx) {
                size_t sp = nm.rfind(' ');
                nm = sp != std::string::npos && nm.size() - sp - 1 <= mx ? nm.substr(sp + 1) : nm.substr(0, mx);
            }
            text(nm, r.x + r.width / 2, r.y + 21, 10, c, 1);
        }
        if (n >= 0)
            text(std::to_string(n), r.x + r.width - 4, r.y + 32, 11, n ? BONE : rgba(100, 104, 112), 2);
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && h.kind == 1)
            h = {};
    }
    if (inv)
        text("Drag items onto the bar. Right-click a slot to clear it.", W / 2.f, H - 126.f, 11, rgba(120, 126, 136),
             1);
}
static void drawHUD() {
    float m = 22;
    for (auto& d : sdrops) {
        float a = d.t / d.mt;
        DrawCircleV(d.p, d.r, rgba(110, 4, 8, (int)(170 * a)));
        DrawCircleV(Vector2Add(d.p, V2(d.r * .3f, d.r * .8f)), d.r * .4f, rgba(110, 4, 8, (int)(140 * a)));
        DrawRectangleRec({d.p.x - d.r * .12f, d.p.y, d.r * .24f, d.r * 2.2f * (1 - a) + d.r},
                         rgba(100, 4, 8, (int)(120 * a)));
    }
    float lo = P.hp < 30 ? (30 - P.hp) / 40 : 0, hv = std::max(lo, P.hurt > 0 ? P.hurt * .9f : 0);
    if (hv > 0)
        DrawTexturePro(vignTex, {0, 0, 512, 512}, {0, 0, (float)W, (float)H}, V2(0, 0), 0,
                       rgba(170, 0, 10, (int)(255 * std::min(.75f, hv))));
    if (P.blockFx > 0)
        DrawTexturePro(vignTex, {0, 0, 512, 512}, {0, 0, (float)W, (float)H}, V2(0, 0), 0,
                       rgba(255, 230, 180, (int)(400 * P.blockFx)));
    if (P.shiver > 0)
        DrawTexturePro(vignTex, {0, 0, 512, 512}, {0, 0, (float)W, (float)H}, V2(0, 0), 0,
                       rgba(120, 170, 255, (int)(110 * P.shiver)));
    // top-left: time & place
    int t = townAt(P.p, 1.0f);
    std::string place = t >= 0 ? TOWNS[t].name : (roadDist(P.p) < 6 ? "Highway" : "Wilderness");
    {
        int cb = cellOf(P.p);
        if (cb >= 0)
            place = blds[cb].cellKind == 2 ? "Basement" : "Upstairs";
    }
    text("DAY " + std::to_string(dayN()) + "   " + hhmm(), m, m, 24, BONE);
    {
        float h = hourOf(), bw = 220, by = m + 50;
        DrawRectangle((int)m, (int)by, (int)bw, 6, rgba(0, 0, 0, 120));
        DrawRectangle((int)m, (int)by, (int)(bw * h / 24), 6, rgba(235, 200, 120));
        for (float t : {5.5f, 18.5f})
            DrawRectangle((int)(m + bw * t / 24) - 1, (int)by - 3, 2, 12, rgba(255, 255, 255, 170));
        for (float t : {13.f, 18.f})
            DrawRectangle((int)(m + bw * t / 24) - 1, (int)by + 8, 2, 4, rgba(255, 90, 60, 230));
        auto hoursTo = [&](float t) {
            return fmodf(t - h + 24, 24);
        };
        auto fmt = [](float hr) {
            int mn = (int)roundf(hr * 60);
            return TextFormat("%d:%02d", mn / 60, mn % 60);
        };
        std::string cd = (h >= 18.5f && h < 20.5f) ? "Dusk now"
                         : isNight()               ? std::string("Night. Dawn in ") + fmt(hoursTo(5.5f))
                                                   : std::string("Dusk in ") + fmt(hoursTo(18.5f));
        if (h >= 13 && h < 18 && !horde.on)
            cd += "  ·  Horde window open";
        else if (h < 13 && !horde.on && hordeDay < dayN())
            cd += "  ·  Horde window at 13:00";
        text(cd, m + bw + 10, by - 6, 14, STEEL);
    }
    text(place + (rainLevel > .4f ? "  ·  Rain" : ""), m, m + 30, 15, STEEL);
    text(std::to_string(kills) + " killed", m, m + 50, 13, STEEL);
    text(std::string(CL().n) + "  ·  " + std::to_string(P.points) + " skill pts  ·  [K]", m, m + 68, 13,
         withA(CL().c, .85f));
    {
        int hz = hotAt(P.p, 1.f);
        if (hz >= 0)
            text(std::string("HOT ZONE: ") + HZN[hotZones[hz].kind], m, m + 86, 14, rgba(255, 90, 60));
    }
    {
        float ax = W / 2.f - 130, ay = H - m - 34;
        bool ready = P.actCd <= 0;
        DrawRectangleRec({ax, ay, 260, 26}, rgba(8, 10, 14, 180));
        if (!ready)
            DrawRectangleRec({ax, ay, 260 * (1 - P.actCd / (CL().actCd * (1 - P.perks.cdr))), 26}, withA(CL().c, .25f));
        if (P.actT > 0 && (P.cls == 0 || P.cls >= 3))
            DrawRectangleLinesEx({ax - 2, ay - 2, 264, 30}, 2, CL().c);
        DrawRectangleLinesEx({ax, ay, 260, 26}, 1, ready ? CL().c : rgba(60, 68, 80));
        text(std::string("[Z]  ") + CL().act + (ready ? "" : "  " + std::to_string((int)ceilf(P.actCd)) + "s"),
             ax + 130, ay + 5, 14, ready ? CL().c : STEEL, 1, false);
    }
    drawHotbar(false);
    // messages
    float my = H - m - 190;
    for (int i = (int)msgs.size() - 1; i >= 0; i--) {
        float a = std::min(1.f, msgs[i].life);
        text(msgs[i].t, m, my, 15, withA(msgs[i].c, a));
        my -= 20;
    }
    // vitals
    float by = H - m - 120, bw = 220;
    auto vit = [&](const char* l, float v, Color c, float y) {
        text(l, m, y - 2, 12, STEEL);
        bar(m + 66, y, bw, 9, v, c);
    };
    vit("HEALTH", P.hp / P.perks.maxhp, P.hp < 30 ? rgba(224, 65, 62) : BLOODC, by);
    vit("FOOD", P.food / 100, P.food < 20 ? rgba(240, 120, 60) : rgba(200, 160, 80), by + 17);
    vit("WATER", P.water / 100, P.water < 20 ? rgba(240, 120, 60) : rgba(90, 160, 220), by + 34);
    vit("STAMINA", P.sta / 100, rgba(217, 196, 122), by + 51);
    {
        text("TEMP", m, by + 66, 12, STEEL);
        float tv = (P.temp - 33) / 5.5f;
        bar(m + 66, by + 68, bw, 9, 1, rgba(30, 36, 48));
        DrawRectangleGradientH((int)(m + 66), (int)(by + 68), (int)bw, 9, COLDC, rgba(255, 120, 80));
        DrawRectangle((int)(m + 66 + bw * Clamp(tv, 0, 1)) - 2, (int)by + 65, 4, 15, BONE);
        char tb[16];
        snprintf(tb, 16, "%.1f°", P.temp);
        text(tb, m + 66 + bw + 8, by + 64, 13, P.temp < 35.5f ? COLDC : BONE);
    }
    bar(m + 66, by + 85, bw * .6f, 6, P.batt / 100, P.uv ? rgba(183, 156, 255) : rgba(200, 200, 160));
    text(P.light ? (P.uv ? "UV" : "LIGHT") : "light", m, by + 81, 12, P.light ? BONE : STEEL);
    float sx = m;
    float sy = by - 26;
    auto status = [&](const std::string& s, Color c) {
        DrawRectangleRec({sx, sy, textW(s, 13) + 14, 20}, rgba(10, 12, 16, 200));
        DrawRectangleLinesEx({sx, sy, textW(s, 13) + 14, 20}, 1, c);
        text(s, sx + 7, sy + 3, 13, c, 0, false);
        sx += textW(s, 13) + 20;
    };
    if (P.bleeds)
        status("BLEEDING x" + std::to_string(P.bleeds), BLOODC);
    if (P.sick > 0)
        status(P.sick > CYCLE ? "FEVER" : "SICK", rgba(170, 200, 90));
    if (P.temp < 35.5f)
        status("COLD", COLDC);
    if (P.broken)
        status(P.splinted ? "SPLINTED " + std::to_string((int)roundf(legFactor() * 100)) + "%"
                          : std::string("BROKEN LEG"),
               rgba(255, 150, 90));
    if ((P.crouch && gExposure > .02f) || P.hideK < .5f) {
        Color ec = gExposure < .33f ? lerpC(rgba(90, 220, 110), rgba(235, 210, 60), gExposure / .33f)
                   : gExposure < .66f
                       ? lerpC(rgba(235, 210, 60), rgba(255, 140, 40), (gExposure - .33f) / .33f)
                       : lerpC(rgba(255, 140, 40), rgba(255, 50, 40), Clamp((gExposure - .66f) / .34f, 0, 1));
        if (gExposure > .85f && ((int)(GetTime() * 6) % 2 == 0))
            ec = rgba(255, 40, 40);
        status(gExposure > .85f   ? "SPOTTED!"
               : gExposure > .02f ? "NOTICED"
               : P.loft >= 0      ? "IN THE HAYLOFT"
                                  : "HIDDEN",
               ec);
    }
    if (P.shieldHP > 0)
        status("SHIELD " + std::to_string((int)roundf(P.shieldHP / (120 * (1 + P.perks.block * 2)) * 100)) + "%",
               rgba(200, 170, 120));
    if (P.blocking)
        status("BLOCKING", BONE);
    if (P.autorun)
        status("AUTORUN", BONE);
    if (P.food < 15)
        status("STARVING", rgba(240, 140, 60));
    if (P.water < 15)
        status("PARCHED", rgba(110, 170, 230));
    if (P.crouch)
        status("CROUCHED", STEEL);
    DrawRectangle(0, H - 4, W, 4, rgba(0, 0, 0, 150));
    DrawRectangle(0, H - 4, (int)(W * P.xp / P.next), 4, SODIUM);
    text("LV " + std::to_string(P.lvl), m, by + 98, 13, SODIUM);
    // weapons
    float rx = W - m;
    Weapon* g = curGun();
    if (g) {
        std::string am = std::to_string(g->clip) + " / " + std::to_string(invCount(ammoItem(g->cal)));
        text(am, rx, by + 30, 36, g->clip == 0 ? rgba(224, 65, 62) : BONE, 2);
        text(CALN[g->cal], rx, by + 70, 13, STEEL, 2);
        if (P.reload > 0)
            bar(rx - 180, by + 90, 180, 4, 1 - P.reload * P.perks.reload / g->rel, BONE);
    } else
        text(MB[P.melee].n, rx, by + 40, 26, BONE, 2);
    for (int i = 0; i < 3; i++) {
        float y = by - 34 + i * 20 - 30;
        bool c = i == P.active;
        std::string nm =
            i < 2 ? (P.hasGun[i] ? P.gun[i].name : (i == 0 ? "- primary -" : "- sidearm -")) : MB[P.melee].n;
        Color cc = i < 2 && P.hasGun[i] ? RARC[P.gun[i].rar] : BONE;
        text(std::string(c ? "> " : "") + std::to_string(i + 1) + "  " + nm, rx, y, c ? 15.f : 13.f,
             c ? cc : withA(cc, .5f), 2);
    }
    text("FLARES " + std::to_string(invCount(IT_FLARE)) + "   PIPE BOMBS " + std::to_string(invCount(IT_PIPEBOMB)), rx,
         by + 106, 12, STEEL, 2);
    // compass strip
    {
        float cw = 320, cx = W / 2.f - cw / 2, cy = 18;
        DrawRectangleRec({cx, cy, cw, 22}, rgba(8, 10, 14, 150));
        const char* dirs[8] = {"E", "SE", "S", "SW", "W", "NW", "N", "NE"};
        BeginScissorMode((int)cx, (int)cy, (int)cw, 22);
        for (int i = -8; i < 16; i++) {
            float a = i * PI / 4;
            float dx = angDiff(a, P.a);
            if (fabsf(dx) > 1.2f)
                continue;
            text(dirs[((i % 8) + 8) % 8], W / 2.f + dx / 1.2f * cw / 2, cy + 3, 14, i % 2 == 0 ? BONE : STEEL, 1,
                 false);
        }
        if (horde.on) {
            float dx = angDiff(horde.a + PI, P.a);
            if (fabsf(dx) < 1.2f)
                text("!", W / 2.f + dx / 1.2f * cw / 2, cy + 2, 16, rgba(255, 90, 60), 1, false);
        }
        EndScissorMode();
        DrawLine((int)(W / 2), (int)cy, (int)(W / 2), (int)(cy + 22), SODIUM);
    }
    if (banner.t > 0) {
        float a = std::min({1.f, banner.t, (4.5f - banner.t) * 3});
        text(banner.a, W / 2.f, H * .17f, std::min(60.f, W * .055f), withA(banner.c, a), 1);
        text(banner.b, W / 2.f, H * .17f + std::min(60.f, W * .055f) + 6, 17, withA(BONE, a * .9f), 1);
    }
    // loot labels & prompt
    for (int i = 0; i < (int)loot.size(); i++) {
        const Loot& l = loot[i];
        if (Vector2Distance(V2(l.p.x, l.p.z), P.p) > 7 || lootHidden(l))
            continue;
        Vector2 s = GetWorldToScreen(Vector3Add(l.p, V3(0, .4f, 0)), cam3);
        std::string nm = l.kind == LK_GUN     ? l.w.name
                         : l.kind == LK_MELEE ? MB[l.id].n
                         : l.kind == LK_CLOTH
                             ? l.c->name
                             : std::string(IDEF[l.id].n) + (l.count > 1 ? " x" + std::to_string(l.count) : "");
        Color c = l.kind == LK_GUN                     ? RARC[l.w.rar]
                  : l.kind == LK_CLOTH && l.c->rar > 0 ? RARC[l.c->rar]
                  : i == nearLoot                      ? BONE
                                                       : rgba(200, 200, 190, 170);
        float w = textW(nm, 12) + 10;
        DrawRectangleRec({s.x - w / 2, s.y - 9, w, 18}, rgba(8, 10, 14, i == nearLoot ? 220 : 140));
        text(nm, s.x, s.y - 7, 12, c, 1, false);
        if (i == nearLoot)
            text("[E] Take", s.x, s.y + 12, 13, SODIUM, 1);
    }
    if (nearLoot >= 0 && nearLoot < (int)loot.size() && loot[nearLoot].kind == LK_GUN) {
        const Weapon& nw = loot[nearLoot].w;
        float pw = 280, h = 90 + nw.aff.size() * 18;
        Vector2 s = GetWorldToScreen(loot[nearLoot].p, cam3);
        float px = Clamp(s.x + 30, 8, W - pw - 8), py = Clamp(s.y - 140, 8, H - h - 8);
        DrawRectangleRec({px, py, pw, h}, rgba(10, 12, 16, 235));
        DrawRectangleLinesEx({px, py, pw, h}, 1.5f, RARC[nw.rar]);
        text(nw.name, px + 12, py + 10, 15, RARC[nw.rar]);
        char b[96];
        snprintf(b, 96, "%s  ·  %s  ·  %s", RARN[nw.rar], CALN[nw.cal], nw.slot ? "Sidearm" : "Primary");
        text(b, px + 12, py + 30, 12, STEEL);
        snprintf(b, 96, "DMG %.0f%s   RPM %d   MAG %d", nw.dmg,
                 nw.pel > 1 ? (" x" + std::to_string(nw.pel)).c_str() : "", (int)(60 / nw.rate), nw.mag);
        text(b, px + 12, py + 50, 13, BONE);
        float y = py + 70;
        for (int id : nw.aff) {
            text(std::string("* ") + AFF[id].desc, px + 12, y, 13, RARC[1]);
            y += 18;
        }
    }
    if (nearLoot < 0 && nearAct.kind != A_NONE && state == ST_PLAY) {
        Vector2 s = GetWorldToScreen(xz3(P.p, 2.3f + charY + (P.loft >= 0 ? lofts[P.loft].y : 0)), cam3);
        text("[E] " + actLabel(nearAct), s.x, s.y, 14, SODIUM, 1);
    }
    // crosshair
    if (state == ST_PLAY) {
        Vector2 mp = GetMousePosition();
        float s = 7 + (g ? g->spread * 120 : 4) * (P.sprint ? 1.8f : P.crouch ? .6f : 1) * (1 + P.shiver);
        Color cc = P.reload > 0 ? rgba(226, 221, 208, 120) : BONE;
        for (int pass = 0; pass < 2; pass++) { // dark outline first, then the line: shows up on snow and concrete too
            float th = pass ? 2 : 4.5f, e = pass ? 0 : 1.2f;
            Color c = pass ? cc : rgba(8, 8, 10, P.reload > 0 ? 110 : 220);
            DrawLineEx(V2(mp.x - s - 7 - e, mp.y), V2(mp.x - s + e, mp.y), th, c);
            DrawLineEx(V2(mp.x + s - e, mp.y), V2(mp.x + s + 7 + e, mp.y), th, c);
            DrawLineEx(V2(mp.x, mp.y - s - 7 - e), V2(mp.x, mp.y - s + e), th, c);
            DrawLineEx(V2(mp.x, mp.y + s - e), V2(mp.x, mp.y + s + 7 + e), th, c);
        }
        if (P.reload > 0 && g)
            DrawRing(mp, s + 11, s + 14, -90, -90 + 360 * (1 - P.reload * P.perks.reload / g->rel), 32, SODIUM);
    }
    if (muted)
        text("MUTED", W / 2.f, H - 26.f, 12, STEEL, 1);
}
