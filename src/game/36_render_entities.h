// Nightfall County: render entities.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ rendering
static Vector3 camRight{1, 0, 0}, camUp{0, 1, 0}, camFwd{0, 0, 1};
static void quad3(Vector3 c, Vector3 ax, Vector3 ay, Vector3 n, Color col, Rectangle uv) {
    rlColor4ub(col.r, col.g, col.b, col.a); rlNormal3f(n.x, n.y, n.z);
    rlTexCoord2f(uv.x, uv.y); rlVertex3f(c.x - ax.x - ay.x, c.y - ax.y - ay.y, c.z - ax.z - ay.z);
    rlTexCoord2f(uv.x, uv.y + uv.height); rlVertex3f(c.x - ax.x + ay.x, c.y - ax.y + ay.y, c.z - ax.z + ay.z);
    rlTexCoord2f(uv.x + uv.width, uv.y + uv.height); rlVertex3f(c.x + ax.x + ay.x, c.y + ax.y + ay.y, c.z + ax.z + ay.z);
    rlTexCoord2f(uv.x + uv.width, uv.y); rlVertex3f(c.x + ax.x - ay.x, c.y + ax.y - ay.y, c.z + ax.z - ay.z);
}
static void groundQuad(Vector2 p, float y, float r, float rot, Color c, Rectangle uv) { Vector3 ax = V3(cosf(rot) * r, 0, sinf(rot) * r), ay = V3(-sinf(rot) * r, 0, cosf(rot) * r); quad3(V3(p.x, y, p.y), ax, ay, V3(0, 1, 0), c, uv); }
static void billboard(Vector3 p, float s, Color c) { quad3(p, Vector3Scale(camRight, s), Vector3Scale(camUp, s), Vector3Negate(camFwd), c, {0, 0, 1, 1}); }

// humanoid built from boxes. local forward = +x
static void drawHuman(Vector2 pos, float yaw, float walk, Color skin, Color shirt, Color pants, float h, float width, bool armL, bool armR, bool head, int pose, float lieT, Color tint, int extra, float swingT) {
    // pose: 0 zombie reach, 1 player w/ gun, 2 player melee, 3 corpse
    Matrix base = MatrixMultiply(MatrixRotateY(-yaw), MatrixTranslate(pos.x, 0, pos.y));
    if (pose == 3) base = MatrixMultiply(MatrixMultiply(MatrixRotateZ(-PI / 2), MatrixTranslate(-h * .05f, .14f, 0)), base);
    float k = h / 1.8f, sw = sinf(walk * 3.2f) * .7f;
    auto T = [&](Color c) { if (pose == 3) return lerpC(c, rgba(60, 20, 22), .35f); return rgba(c.r * tint.r / 255, c.g * tint.g / 255, c.b * tint.b / 255, 255); };
    auto part = [&](Vector3 size, Vector3 pivot, Vector3 off, float rz, float rx, Color c) {
        Matrix m = MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixScale(size.x, size.y, size.z), MatrixTranslate(off.x, off.y, off.z)), MatrixMultiply(MatrixRotateX(rx), MatrixRotateZ(rz))), MatrixTranslate(pivot.x, pivot.y, pivot.z));
        drawMeshM(M_CUBE, MatrixMultiply(m, base), T(c)); };
    float legL = .85f * k, torso = .62f * k, hipY = legL;
    float lsw = pose == 3 ? .15f : sw, rsw = pose == 3 ? -.1f : -sw;
    part(V3(.2f * width, legL, .2f * width), V3(0, hipY, -.12f * width), V3(0, -legL / 2, 0), lsw * .6f, 0, pants);
    part(V3(.2f * width, legL, .2f * width), V3(0, hipY, .12f * width), V3(0, -legL / 2, 0), rsw * .6f, 0, pants);
    part(V3(.28f * width, torso, .46f * width), V3(0, hipY, 0), V3(0, torso / 2, 0), pose == 0 ? -.18f : 0, 0, shirt);
    if (extra == 1) part(V3(.22f, .42f * k, .36f), V3(-.2f * width, hipY + torso * .55f, 0), V3(0, 0, 0), 0, 0, rgba(80, 70, 52)); // backpack
    float shY = hipY + torso - .06f * k, armLen = .62f * k;
    auto arm = [&](bool has, float side, float rz, float rx) {
        if (has) part(V3(.13f * width, armLen, .13f * width), V3(0, shY, side * .3f * width), V3(0, -armLen / 2, 0), rz, rx, pose == 0 ? skin : shirt);
        else part(V3(.15f, .12f, .15f), V3(0, shY, side * .3f * width), V3(0, -.04f, 0), 0, 0, rgba(140, 16, 22)); };
    if (pose == 0) { float r = sinf(walk * 2.1f) * .12f; arm(armL, -1, 1.35f + r, .05f); arm(armR, 1, 1.4f - r, -.05f); }
    else if (pose == 1) { arm(true, -1, 1.45f, .45f); arm(true, 1, 1.45f, -.35f); }
    else if (pose == 2) { float s = swingT >= 0 ? (-1.2f + 2.4f * swingT) : 0; arm(true, -1, .2f + sw * .3f, 0); arm(true, 1, 1.5f, s); }
    else { arm(armL, -1, .3f, .2f); arm(armR, 1, -.2f, -.2f); }
    if (head) {
        float hs = .26f * k; part(V3(hs, hs * 1.05f, hs), V3(0, shY + .04f * k, 0), V3(.02f, hs * .55f, 0), pose == 0 ? -.25f : 0, 0, skin);
        if (pose == 0 || pose == 3) part(V3(.04f, .05f, .1f), V3(0, shY + .04f * k, 0), V3(hs * .52f, hs * .55f, 0), pose == 0 ? -.25f : 0, 0, rgba(90, 10, 14));
        if (extra == 1) part(V3(hs * 1.05f, hs * .35f, hs * 1.08f), V3(0, shY + .04f * k, 0), V3(0, hs * 1.0f, 0), 0, 0, rgba(60, 64, 50));
        if (extra == 2) part(V3(hs * 1.15f, hs * .45f, hs * 1.15f), V3(0, shY + .04f * k, 0), V3(0, hs * .95f, 0), 0, 0, rgba(70, 80, 60)); // helmet
    } else part(V3(.16f, .08f, .16f), V3(0, shY, 0), V3(0, .04f, 0), 0, 0, rgba(130, 14, 20));
    (void)lieT;
}
static void drawGibM(const Gib& g) {
    Matrix m = MatrixMultiply(MatrixMultiply(MatrixScale(g.s.x, g.s.y, g.s.z), MatrixRotate(g.axis, g.rot)), MatrixTranslate(g.p.x, g.p.y, g.p.z));
    if (g.kind == G_GUTS) { for (int i = 0; i < 4; i++) drawMeshM(M_SPHERE, MatrixMultiply(MatrixScale(g.s.x * .6f, g.s.y, g.s.z * .6f), MatrixTranslate(g.p.x + cosf(i * 1.6f) * .12f, g.p.y, g.p.z + sinf(i * 1.6f) * .12f)), i % 2 ? g.c : rgba(120, 30, 40)); return; }
    drawMeshM(g.kind == G_HEAD ? M_CUBE : M_CUBE, m, g.c);
    if (g.kind == G_LIMB || g.kind == G_HEAD) { Vector3 e = Vector3Transform(V3(0, -.5f, 0), MatrixMultiply(MatrixRotate(g.axis, g.rot), MatrixScale(1, 1, 1))); Vector3 ep = Vector3Add(g.p, V3(e.x * g.s.y, e.y * g.s.y, e.z * g.s.y)); drawMeshM(M_SPHERE, TRS(ep, V3(g.s.x * 1.1f, g.s.x * 1.1f, g.s.x * 1.1f), 0), rgba(140, 16, 22)); }
}
struct LightC { Vector3 p; float r; Color c; float d; };
static Vector3 sunDirNow() { float sunA = (hourOf() - 6) / 12 * PI; return Vector3Normalize(V3(-cosf(sunA) * .8f, -std::max(.3f, sinf(sunA)), .45f)); } // night: moon hangs high-ish
static void setupLighting(Vector3 viewPos) {
    float h = hourOf(), dk = darkK();
    Vector3 sunDir = sunDirNow();
    Vector3 sunCol = Vector3Lerp(V3(1.0f, .93f, .82f), V3(.05f, .06f, .12f), dk);
    float warm = h < 12 ? 1 - smoothstep01(5.5f, 8.5f, h) : smoothstep01(16.5f, 18.5f, h) * (1 - smoothstep01(19.5f, 21.f, h)); // dawn and dusk tint fade in and out
    sunCol = Vector3Lerp(sunCol, V3(1.f, .6f, .37f), warm * (1 - dk));
    sunCol = Vector3Scale(sunCol, 1 - rainLevel * .45f);
    Vector3 amb = Vector3Lerp(V3(.4f, .42f, .46f), V3(.032f, .038f, .065f), dk); amb = Vector3Scale(amb, 1 - rainLevel * .25f);
    Vector3 sky = Vector3Lerp(V3(.35f, .42f, .55f), V3(.01f, .015f, .03f), dk);
    Vector3 fog = Vector3Lerp(V3(.55f, .58f, .6f), V3(.006f, .008f, .014f), dk); fog = Vector3Lerp(fog, V3(.4f, .43f, .46f), rainLevel * (1 - dk));
    float fogN = Lerp(42, 9, dk) * (1 - rainLevel * .4f), fogF = Lerp(90, 30, dk) * (1 - rainLevel * .35f);
    if (state == ST_TITLE) { fogN = 30; fogF = 70; }
    setU(uSunDir, &sunDir, SHADER_UNIFORM_VEC3); setU(uSunCol, &sunCol, SHADER_UNIFORM_VEC3); setU(uAmb, &amb, SHADER_UNIFORM_VEC3); setU(uSky, &sky, SHADER_UNIFORM_VEC3);
    setU(uFogCol, &fog, SHADER_UNIFORM_VEC3); Vector2 fc = state == ST_TITLE ? camPos : P.p; setU(uFogC, &fc, SHADER_UNIFORM_VEC2); setU(uFogN, &fogN, SHADER_UNIFORM_FLOAT); setU(uFogF, &fogF, SHADER_UNIFORM_FLOAT);
    setU(uView, &viewPos, SHADER_UNIFORM_VEC3); float wet = rainLevel; setU(uWet, &wet, SHADER_UNIFORM_FLOAT);
    // flashlight
    bool on = P.light && state != ST_TITLE && state != ST_DEAD;
    Vector3 flp = xz3(Vector2Add(P.p, polar(P.a, .3f)), 1.4f); Vector2 fd = polar(P.a, 1); Vector3 fdir = Vector3Normalize(V3(fd.x, -.3f, fd.y));
    Vector3 flc = on ? (P.uv ? V3(.9f, .5f, 2.2f) : V3(2.6f, 2.4f, 2.0f)) : V3(0, 0, 0); float flcos = P.uv ? .93f : .9f, flr = P.uv ? 18.f : 28.f;
    setU(uFlPos, &flp, SHADER_UNIFORM_VEC3); setU(uFlDir, &fdir, SHADER_UNIFORM_VEC3); setU(uFlCol, &flc, SHADER_UNIFORM_VEC3);
    setU(uFlCos, &flcos, SHADER_UNIFORM_FLOAT); setU(uFlRange, &flr, SHADER_UNIFORM_FLOAT);
    // point lights
    std::vector<LightC> cand; Vector2 c = state == ST_TITLE ? camPos : P.p;
    for (auto& l : lights) { float a = l.t / l.mt; cand.push_back({l.p, l.r, rgba((int)(l.c.r * a), (int)(l.c.g * a), (int)(l.c.b * a)), Vector2Distance(V2(l.p.x, l.p.z), c)}); }
    for (auto& l : staticLights) { float d = Vector2Distance(V2(l.p.x, l.p.z), c); if (d < 60) { float fl = .85f + .15f * sinf(gTime * 17 + l.p.x) * sinf(gTime * 7.3f + l.p.z); cand.push_back({l.p, l.r * fl, mulC(l.c, .3f + .7f * dk), d}); } }
    for (auto& b : bombs) if (b.flare) cand.push_back({Vector3Add(b.p, V3(0, .3f, 0)), 13, rgba(255, 50, 30), Vector2Distance(V2(b.p.x, b.p.z), c)});
    for (auto& f : fires) { float fl = .85f + .15f * sinf(gTime * 19 + f.p.x) * sinf(gTime * 7 + f.p.y); cand.push_back({xz3(f.p, .9f), 12 * fl * std::min(1.f, f.fuel / 20), rgba(255, 140, 60), Vector2Distance(f.p, c) - 5}); }
    for (auto& z : zombies) if (z.burn > 0) cand.push_back({xz3(z.p, 1), 5, rgba(255, 120, 40), Vector2Distance(z.p, c)});
    std::sort(cand.begin(), cand.end(), [](const LightC& a, const LightC& b) { return a.d < b.d; });
    int n = std::min(16, (int)cand.size()); Vector4 lp[16]; Vector3 lc[16];
    for (int i = 0; i < n; i++) { lp[i] = Vector4{cand[i].p.x, cand[i].p.y, cand[i].p.z, cand[i].r}; lc[i] = V3(cand[i].c.r / 255.f * 1.6f, cand[i].c.g / 255.f * 1.6f, cand[i].c.b / 255.f * 1.6f); }
    setU(uNL, &n, SHADER_UNIFORM_INT); if (n) { setU(uLPos, lp, SHADER_UNIFORM_VEC4, n); setU(uLCol, lc, SHADER_UNIFORM_VEC3, n); }
}
static void computeFades(float dt) {
    Vector3 cp = cam3.position; Vector3 head = xz3(P.p, 1.4f); Ray r{cp, Vector3Normalize(Vector3Subtract(head, cp))}; float pd = Vector3Distance(cp, head);
    int pb = buildingAt(P.p);
    for (int i = 0; i < (int)blds.size(); i++) {
        Building& b = blds[i]; if (fabsf((b.x0 + b.x1) / 2 - P.p.x) > 40 || fabsf((b.z0 + b.z1) / 2 - P.p.y) > 40) { b.fade = 1; b.cut = 1; continue; }
        bool occl = false; if (state != ST_TITLE) { RayCollision rc = GetRayCollisionBox(r, BoundingBox{V3(b.x0, 0, b.z0), V3(b.x1, b.h + 1, b.z1)}); occl = rc.hit && rc.distance < pd - .5f; }
        bool in = i == pb; float tf = in ? 0.f : occl ? .2f : 1.f, tc = in ? 0.f : occl ? .3f : 1.f;
        b.fade += (tf - b.fade) * std::min(1.f, dt * 8); b.cut += (tc - b.cut) * std::min(1.f, dt * 8);
    }
    for (auto& t : trees) { if (fabsf(t.p.x - P.p.x) > 30 || fabsf(t.p.y - P.p.y) > 30) { t.fade = 1; continue; } float dx = fabsf(t.p.x - P.p.x), dz = t.p.y - P.p.y; bool occ = dx < t.r + 1 && dz > -t.r && dz < t.h * .7f; t.fade += ((occ ? .25f : 1.f) - t.fade) * std::min(1.f, dt * 8); }
}
static void hull(std::vector<Vector2>& pts) {
    std::sort(pts.begin(), pts.end(), [](Vector2 a, Vector2 b) { return a.x < b.x || (a.x == b.x && a.y < b.y); });
    std::vector<Vector2> h; auto cross = [](Vector2 o, Vector2 a, Vector2 b) { return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x); };
    for (int pass = 0; pass < 2; pass++) { size_t st = h.size(); for (auto& p : pts) { while (h.size() >= st + 2 && cross(h[h.size() - 2], h.back(), p) <= 0) h.pop_back(); h.push_back(p); } h.pop_back(); std::reverse(pts.begin(), pts.end()); }
    pts = h;
}
static RenderTexture2D flRT{}; static bool flOk = false;
static RenderTexture2D makeDepthRT(int N) { RenderTexture2D r{}; r.id = rlLoadFramebuffer(); if (!r.id) return r; r.texture.width = N; r.texture.height = N; rlEnableFramebuffer(r.id);
    r.depth.id = rlLoadTextureDepth(N, N, false); r.depth.width = N; r.depth.height = N; r.depth.format = 19; r.depth.mipmaps = 1; rlFramebufferAttach(r.id, r.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
    if (!rlFramebufferComplete(r.id)) r.id = 0; rlDisableFramebuffer(); return r; }
