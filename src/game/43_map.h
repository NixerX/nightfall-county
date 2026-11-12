// Nightfall County: map.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
static void drawMap() {
    static float zoom = 1; static Vector2 vc{MAP / 2, MAP / 2}; static bool dragging = false; static Vector2 dragM{};
    dim(.75f); float sz = std::min(W - 80.f, H - 100.f), fx = W / 2.f - sz / 2, fy = H / 2.f - sz / 2 + 10; Vector2 m = GetMousePosition(); Rectangle fr = {fx, fy, sz, sz};
    Vector2 me = P.p; { int cb = cellOf(P.p); if (cb >= 0 && blds[cb].parent >= 0) { const Building& pb = blds[blds[cb].parent]; me = V2((pb.x0 + pb.x1) / 2, (pb.z0 + pb.z1) / 2); } }
    float base = sz / MAP; auto toW = [&](Vector2 s2) { float sc = base * zoom; return V2(vc.x + (s2.x - (fx + sz / 2)) / sc, vc.y + (s2.y - (fy + sz / 2)) / sc); };
    float wh = GetMouseWheelMove(); if (wh != 0 && CheckCollisionPointRec(m, fr)) { Vector2 before = toW(m); zoom = Clamp(zoom * (wh > 0 ? 1.25f : .8f), 1, 8); Vector2 after = toW(m); vc = Vector2Add(vc, Vector2Subtract(before, after)); }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, fr)) { dragging = true; dragM = m; } if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) dragging = false;
    if (dragging) { vc = Vector2Subtract(vc, Vector2Scale(Vector2Subtract(m, dragM), 1 / (base * zoom))); dragM = m; }
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_C)) { vc = me; if (zoom < 2.5f) zoom = 2.5f; } if (IsKeyPressed(KEY_HOME)) { zoom = 1; vc = V2(MAP / 2, MAP / 2); }
    float half = MAP / 2 / zoom; vc.x = Clamp(vc.x, half, MAP - half); vc.y = Clamp(vc.y, half, MAP - half);
    float sc = base * zoom, mx = fx + sz / 2 - vc.x * sc, my = fy + sz / 2 - vc.y * sc;
    BeginScissorMode((int)fx, (int)fy, (int)sz, (int)sz);
    DrawTexturePro(mapTex, {0, 0, (float)mapTex.width, (float)mapTex.height}, {mx, my, MAP * sc, MAP * sc}, V2(0, 0), 0, rgba(230, 225, 210));
    if (zoom >= 2.5f) for (auto& b : blds) { if (b.cellKind) continue; Rectangle r = {mx + b.x0 * sc, my + b.z0 * sc, (b.x1 - b.x0) * sc, (b.z1 - b.z0) * sc}; if (r.x > fx + sz || r.y > fy + sz || r.x + r.width < fx || r.y + r.height < fy) continue; DrawRectangleLinesEx(r, 1, rgba(40, 34, 28, 200)); }
    for (auto& h : hotZones) { Vector2 hp = V2(mx + h.c.x * sc, my + h.c.y * sc); DrawCircleV(hp, h.r * sc * 1.15f, rgba(200, 40, 30, 70)); DrawRing(hp, h.r * sc * 1.15f - 1.5f, h.r * sc * 1.15f, 0, 360, 32, rgba(200, 40, 30, 200));
        text(HZN[h.kind], hp.x, hp.y + h.r * sc + 4, 13, rgba(255, 130, 110), 1, true); }
    { Vector2 hc{}; int hn = 0; for (auto& z : zombies) if (z.hordeM && !z.dead && !z.gone) { hc = Vector2Add(hc, z.p); hn++; } bool hv = hn > 0; if (hv) hc = Vector2Scale(hc, 1.f / hn); else if (horde.on) { hc = horde.end; hv = true; }
      if (hv && Vector2Distance(hc, me) < 150) { // only shown when the horde is within 150 m of you
        Vector2 hp = V2(mx + hc.x * sc, my + hc.y * sc); bool off = !CheckCollisionPointRec(hp, fr); hp.x = Clamp(hp.x, fx + 14, fx + sz - 14); hp.y = Clamp(hp.y, fy + 14, fy + sz - 14);
        float pulse = 7 + 2 * sinf((float)GetTime() * 5); DrawCircleV(hp, pulse, rgba(230, 40, 30, off ? 150 : 230)); DrawRing(hp, pulse + 2, pulse + 4, 0, 360, 24, rgba(230, 40, 30, 120));
        if (horde.on) { Vector2 dv = Vector2Subtract(horde.end, hc); float dl = Vector2Length(dv); if (dl > 1) DrawLineEx(hp, Vector2Add(hp, Vector2Scale(Vector2Scale(dv, 1 / dl), 22)), 3, rgba(230, 40, 30, 220)); }
        text(off ? "HORDE (edge)" : "HORDE", hp.x, hp.y + 12, 13, rgba(255, 130, 110), 1, true); } }
    for (int i = 0; i < 6; i++) { const Town& T = TOWNS[i]; text(T.name, mx + T.c.x * sc, my + T.c.y * sc - T.r * sc - 18, 16, T.kind == TK_MILITARY ? rgba(220, 70, 60) : rgba(30, 26, 20), 1, false); }
    Vector2 pp = V2(mx + me.x * sc, my + me.y * sc); DrawCircleV(pp, 6, rgba(220, 40, 40)); DrawLineEx(pp, Vector2Add(pp, polar(P.a, 14)), 2, rgba(220, 40, 40));
    EndScissorMode(); DrawRectangleLinesEx(fr, 2, rgba(90, 80, 60));
    text("MAP   [M] close   Wheel zoom   Drag to pan   Space: find me   Home: whole county", fx, fy - 30, 16, BONE); drawCursor();
}
