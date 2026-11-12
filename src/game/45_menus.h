// Nightfall County: menus.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
static State settingsBack = ST_PAUSE;
static void drawTitle() {
    dim(.45f); float x = std::max(40.f, W / 2.f - 380), y = H * .14f, big = std::min(104.f, W * .09f);
    text(std::string("LINN-BENTON QUARANTINE ZONE  -  NO RADIO CONTACT SINCE DAY 9      ") + VERSION, x, y, 14, STEEL); y += 28;
    text("NIGHTFALL", x, y, big, SODIUM); y += big - 8; text("COUNTY", x, y, big, BLOODC); y += big + 12;
    text("Cedar Falls, Harlow, Millbrook, Pine Ridge, Dunmore, Fort Calloway. Same towns every life, never in the same place.", x, y, 16, rgba(199, 194, 182)); y += 22;
    text("Each life rolls a class, scatters your skill points for you, and redraws the roads, loot and hot zones.", x, y, 16, rgba(199, 194, 182)); y += 36;
    const char* keys[][2] = {{"WASD", "Move"}, {"Mouse / LMB", "Aim / shoot or swing (V melee)"}, {"Shift / C", "Sprint / crouch"}, {"Space", "Dodge roll"}, {"RMB / Q", "Block / kick"}, {"R", "Reload"},
        {"E", "Pick up / use"}, {"1-0 / Ctrl+Wheel", "Hotbar / weapons (wheel zooms)"}, {"Tab", "Backpack (eat, drink, bandage)"}, {"F / U", "Flashlight / UV mode"}, {"G / H", "Pipe bomb / road flare"}, {"M", "Map"}, {"Z / K", "Class skill / skill trees"}, {"B / N", "Bandage / best meds"}, {"X", "Autorun"}, {"P / Esc", "Pause"}};
    for (int i = 0; i < 16; i++) { float cx = x + (i % 2) * 380, cy = y + (i / 2) * 26; DrawRectangleRec({cx, cy - 2, 110, 22}, rgba(18, 22, 30)); DrawRectangleLinesEx({cx, cy - 2, 110, 22}, 1, rgba(60, 68, 80)); text(keys[i][0], cx + 55, cy, 13, BONE, 1, false); text(keys[i][1], cx + 122, cy, 15, rgba(199, 194, 182)); }
    y += 8 * 26 + 22;
    if (hasSave()) { if (button("CONTINUE", x, y, 200, 50, true) || IsKeyPressed(KEY_ENTER)) { if (!loadGame()) { deleteSave(); newGame(); } } y += 60;
        if (button("NEW LIFE", x, y, 200, 40, false)) { deleteSave(); newGame(); } }
    else if (button("HEAD OUT", x, y, 200, 50, true) || IsKeyPressed(KEY_ENTER)) newGame();
    if (button("SETTINGS", x, y + 60, 200, 40, false)) { settingsBack = ST_TITLE; state = ST_SETTINGS; }
    if (bestHours > 0) text("Longest survival: " + std::to_string(bestHours) + " hours, " + std::to_string(bestKills) + " kills", x + 220, y + 16, 15, STEEL);
    drawCursor();
}
static void wrapText(const std::string& d, float x, float y, float w, float size, Color c) { std::string word, acc; for (size_t k = 0; k <= d.size(); k++) { if (k == d.size() || d[k] == ' ') { std::string t = acc.empty() ? word : acc + " " + word; if (textW(t, size) > w && !acc.empty()) { text(acc, x, y, size, c); y += size + 4; acc = word; } else acc = t; word.clear(); } else word += d[k]; } if (!acc.empty()) text(acc, x, y, size, c); }
static void drawSkills() {
    dim(.72f); const ClassDef& C = CL(); float pw = std::min(1180.f, W - 60.f), px = W / 2.f - pw / 2, py = 50;
    text("THIS LIFE", px, py, 13, STEEL); text(C.n, px, py + 16, 46, C.c); wrapText(C.blurb, px, py + 70, pw * .55f, 15, rgba(199, 194, 182));
    { float bx = px + pw - 380; DrawRectangleRec({bx, py + 12, 380, 74}, rgba(12, 14, 20, 235)); DrawRectangleLinesEx({bx, py + 12, 380, 74}, 1, C.c);
      text(std::string("[Z]  ") + C.act, bx + 14, py + 22, 18, C.c); text(C.actD, bx + 14, py + 46, 13, BONE); char b[64]; snprintf(b, 64, "Recharge %ds", (int)roundf(C.actCd * (1 - P.perks.cdr))); text(b, bx + 14, py + 64, 12, STEEL); }
    float cw = (pw - 32) / 3, ty = py + 120;
    for (int t = 0; t < 3; t++) { float cx = px + t * (cw + 16); int tot = 0; for (int i = 0; i < 4; i++) tot += P.rank[t][i];
        text(C.t[t].n, cx, ty, 22, BONE); text(std::to_string(tot) + " pts", cx + cw, ty + 6, 13, STEEL, 2);
        for (int i = 0; i < 4; i++) { const Skill& sk = C.t[t].s[i]; float y = ty + 36 + i * 96; int r = P.rank[t][i]; bool open = i == 0 || P.rank[t][i - 1] > 0;
            Rectangle box = {cx, y, cw, 84}; DrawRectangleRec(box, r > 0 ? rgba(24, 28, 36, 245) : rgba(14, 16, 22, 235)); DrawRectangleLinesEx(box, r > 0 ? 1.5f : 1.f, r > 0 ? C.c : open ? rgba(60, 68, 80) : rgba(36, 40, 48));
            if (i > 0) DrawRectangle((int)(cx + 24), (int)(y - 12), 2, 12, P.rank[t][i - 1] > 0 ? C.c : rgba(50, 56, 66));
            text(sk.n, cx + 14, y + 10, 17, r > 0 ? C.c : open ? BONE : rgba(120, 124, 130));
            for (int k = 0; k < sk.max; k++) { Rectangle pip = {cx + cw - 16 - (sk.max - k) * 18.f, y + 12, 12, 12}; if (k < r) DrawRectangleRec(pip, C.c); else DrawRectangleLinesEx(pip, 1, rgba(80, 86, 96)); }
            wrapText(sk.d, cx + 14, y + 36, cw - 28, 13, r > 0 ? rgba(210, 205, 194) : rgba(140, 142, 146)); } }
    text("You don't choose. Every level, fate drops one point into a skill you've unlocked.   [K] close", W / 2.f, ty + 36 + 4 * 96 + 8, 14, STEEL, 1);
    drawCursor();
    drawTabs(1);
}
static void drawPause() { dim(.6f); float x = W / 2.f - 250, y = H * .35f; text("PAUSED", W / 2.f, y, 60, BONE, 1); if (button("RESUME", x, y + 100, 150, 46, true)) state = ST_PLAY;
    if (button("SETTINGS", x + 170, y + 100, 150, 46, false)) { settingsBack = ST_PAUSE; state = ST_SETTINGS; } if (button("SAVE & QUIT", x + 340, y + 100, 170, 46, false)) { saveGame(); state = ST_TITLE; }
    if (button("ABANDON RUN", x + 530, y + 100, 170, 46, false)) { deleteSave(); state = ST_TITLE; } drawCursor(); }
static void applyDisplay() {
    int mon = GetCurrentMonitor(); int w = cfg.res == 5 ? GetMonitorWidth(mon) : RES[cfg.res][0], h = cfg.res == 5 ? GetMonitorHeight(mon) : RES[cfg.res][1];
    if (IsWindowFullscreen()) ToggleFullscreen();
    if (cfg.fullscreen) { SetWindowSize(w, h); ToggleFullscreen(); }
    else { w = std::min(w, GetMonitorWidth(mon)); h = std::min(h, GetMonitorHeight(mon) - 60); SetWindowSize(w, h); SetWindowPosition((GetMonitorWidth(mon) - w) / 2, std::max(30, (GetMonitorHeight(mon) - h) / 2)); }
    W = GetScreenWidth(); H = GetScreenHeight();
}
static void drawSettings() {
    dim(.78f); float x = W / 2.f - 300, y = H * .06f; text("SETTINGS", x, y, 40, BONE); y += 56;
    static const int fpsOpt[5] = {30, 60, 90, 144, 0}; int fi = 0; for (int i = 0; i < 5; i++) if (fpsOpt[i] == cfg.fps) fi = i;
    auto row = [&](const char* label, const std::string& val, const char* hint) -> int { text(label, x, y + 10, 18, BONE); text(val, x + 420, y + 10, 18, SODIUM, 1); text(hint, x, y + 42, 12, STEEL);
        int r = 0; if (button("<", x + 300, y + 2, 40, 34, false)) r = -1; if (button(">", x + 500, y + 2, 40, 34, false)) r = 1; y += H < 800 ? 50 : 58; return r; };
    int d;
    if ((d = row("Difficulty", DIFFN[cfg.diff], "Zombie health, damage and numbers. They also get tougher every level you gain."))) cfg.diff = (cfg.diff + d + 4) % 4;
    if ((d = row("Display", cfg.fullscreen ? "Fullscreen" : "Windowed", "F11 toggles anytime."))) { cfg.fullscreen ^= 1; applyDisplay(); }
    { std::string rn = cfg.res == 5 ? std::string("Native ") + std::to_string(GetMonitorWidth(GetCurrentMonitor())) + "x" + std::to_string(GetMonitorHeight(GetCurrentMonitor())) : std::to_string(RES[cfg.res][0]) + "x" + std::to_string(RES[cfg.res][1]);
      if ((d = row("Resolution", rn, "Window size, or the screen mode in fullscreen."))) { cfg.res = (cfg.res + d + 6) % 6; applyDisplay(); } }
    static const char* q3[3] = {"Low", "Medium", "High"}; static const char* sh3[3] = {"Off", "Low (1024)", "High (2048)"};
    if ((d = row("Frame rate cap", cfg.fps ? std::to_string(cfg.fps) + " FPS" : std::string("Unlimited"), "Lower = less heat and power. 60 is plenty for this game."))) { fi = (fi + d + 5) % 5; cfg.fps = fpsOpt[fi]; SetTargetFPS(cfg.fps); }
    if ((d = row("Shadows", sh3[cfg.shadows], "Sun and flashlight shadows. Off is the biggest speedup on weak GPUs."))) { cfg.shadows = (cfg.shadows + d + 3) % 3; reinitShadows(); }
    { static const char* CN[3] = {"Classic", "Aim-relative", "Click-to-move"}; static const char* CH[3] = {"W/A/S/D are fixed compass directions; two keys for diagonals.", "W walks toward the cursor, S backs off, A/D circle around it.", "Diablo style: click to walk, click a zombie to attack it, click loot to grab it. Alt+click attacks in place."};
      if ((d = row("Controls", CN[cfg.controls], CH[cfg.controls]))) cfg.controls = (cfg.controls + d + 3) % 3; }
    if ((d = row("Foliage density", q3[cfg.foliage], "Grass, bushes and tall grass. Applies to the next map (next life)."))) cfg.foliage = (cfg.foliage + d + 3) % 3;
    auto pct = [](float v) { return std::to_string((int)roundf(v * 100)) + "%"; };
    if ((d = row("Film grain", cfg.grain ? "On" : "Off", "Animated static over the screen. Off is cleaner and a little faster."))) cfg.grain ^= 1;
    if ((d = row("Auto-equip guns", cfg.autoEquip ? "On" : "Off", "Switch to a gun as soon as you pick it up. Off keeps what you're holding."))) cfg.autoEquip ^= 1;
    if ((d = row("Master volume", pct(cfg.master), ""))) cfg.master = Clamp(cfg.master + d * .1f, 0, 1);
    if ((d = row("Effects volume", pct(cfg.sfx), "Guns, zombies, ambience"))) cfg.sfx = Clamp(cfg.sfx + d * .1f, 0, 1);
    if ((d = row("Music volume", pct(cfg.music), "Tension, horde drums, heartbeat"))) cfg.music = Clamp(cfg.music + d * .1f, 0, 1);
    text(TextFormat("Running at %d FPS", GetFPS()), x, y + 6, 14, STEEL);
    if (button("BACK", x, y + 36, 150, 46, true) || IsKeyPressed(KEY_ESCAPE)) { saveCfg(); state = settingsBack; }
    drawCursor();
}
static void drawDead() {
    if (deadT < 1.2f) return; dim(std::min(.75f, (deadT - 1.2f)));
    float x = std::max(40.f, W / 2.f - 360), y = H * .25f; int hrs = (int)(P.alive / CYCLE * 24);
    text("RUN OVER", x, y, 14, STEEL); text(deathLine, x, y + 22, std::min(70.f, W * .06f), BLOODC); y += 120;
    std::string vals[4] = {std::to_string(hrs) + "h", std::to_string(kills), std::to_string((int)(P.walked / 1000)) + "." + std::to_string(((int)P.walked % 1000) / 100) + " km", std::to_string(P.lvl)}; const char* lab[4] = {"SURVIVED", "KILLS", "WALKED", "LEVEL"};
    for (int i = 0; i < 4; i++) { float cx = x + i * 170; DrawLineEx(V2(cx, y), V2(cx + 150, y), 1, rgba(60, 68, 80)); text(vals[i], cx, y + 10, 32, BONE); text(lab[i], cx, y + 48, 11, STEEL); }
    y += 84; text(std::string("Died a ") + CL().n + " with " + std::to_string(P.points) + " skill points. The next life rolls a new class and a new map.", x, y, 15, CL().c);
    y += 22; text(std::to_string(gibCount) + " body parts left on the roads of Nightfall County.", x, y, 15, STEEL);
    if (bestRar >= 0) { y += 22; text("Best find: " + bestName + " (" + RARN[bestRar] + ")", x, y, 15, RARC[bestRar]); }
    if (newRecord) { y += 22; text("Longest you've ever lasted.", x, y, 15, SODIUM); }
    if (button("NEW LIFE", x, y + 40, 200, 50, true) || IsKeyPressed(KEY_ENTER)) newGame(); drawCursor();
}
