// Nightfall County: shadows.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ------------------------------------------------------------ sun shadow map
static RenderTexture2D shadowRT{};
static int SHADOW_N = 2048;
static bool shadowsOk = false, shadowsActive = false;
static void initShadows();
static void reinitShadows() {
    if (shadowRT.id) {
        rlUnloadFramebuffer(shadowRT.id);
        rlUnloadTexture(shadowRT.depth.id);
        shadowRT = RenderTexture2D{};
    }
    shadowsOk = false;
    if (cfg.shadows > 0) {
        SHADOW_N = cfg.shadows == 1 ? 1024 : 2048;
        initShadows();
    }
}
static void initShadows() {
    shadowRT.id = rlLoadFramebuffer();
    if (!shadowRT.id)
        return;
    shadowRT.texture.width = SHADOW_N;
    shadowRT.texture.height = SHADOW_N;
    rlEnableFramebuffer(shadowRT.id);
    shadowRT.depth.id = rlLoadTextureDepth(SHADOW_N, SHADOW_N, false);
    shadowRT.depth.width = SHADOW_N;
    shadowRT.depth.height = SHADOW_N;
    shadowRT.depth.format = 19;
    shadowRT.depth.mipmaps = 1;
    rlFramebufferAttach(shadowRT.id, shadowRT.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
    shadowsOk = rlFramebufferComplete(shadowRT.id);
    rlDisableFramebuffer();
    TraceLog(LOG_INFO, "NIGHTFALL: shadow map %s", shadowsOk ? "ready" : "unavailable, using blob shadows");
    flRT = makeDepthRT(1024);
    flOk = flRT.id != 0;
}
static void setMatU(int u, const Matrix& m) {
    Shader* sh[2] = {&lit, &litSkin};
    for (int k = 0; k < 2; k++)
        if (ULOC[k][u] >= 0)
            SetShaderValueMatrix(*sh[k], ULOC[k][u], m);
}
static void drawCasters(Vector2 c, float R, bool withPlayer = true) {
    litMat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    setTexMode(0);
    for (int cx = gcl(c.x - R); cx <= gcl(c.x + R); cx++)
        for (int cz = gcl(c.y - R); cz <= gcl(c.y + R); cz++) {
            if (chunkHas[cx][cz])
                DrawMesh(chunkMesh[cx][cz], litMat, MatrixIdentity());
            for (int pi : pgrid[cx][cz]) {
                const Prop& p = props[pi];
                if (p.layer == L_ALWAYS || p.layer == L_FLOOR || p.layer == L_WALLDET)
                    continue;
                drawMeshM(p.mesh, TRS(p.p, p.s, p.yaw), WHITE);
            }
            (void)0;
            for (int ti : tgrid[cx][cz]) {
                const Tree& t = trees[ti];
                if (t.asset >= 0) {
                    drawAsset(t.asset, placeM(t.asset, V3(t.p.x, -.05f, t.p.y), t.sc, t.yaw));
                    continue;
                }
                drawMeshM(t.kind == 0 ? M_CONE : M_SPHERE,
                          TRS(V3(t.p.x, t.h * .4f, t.p.y), V3(t.r * 2, t.h * .6f, t.r * 2), 0), WHITE);
            }
        }
    for (auto& d : doorObjs) {
        if (d.broken || d.anim > .5f || fabsf(d.c.x - c.x) > R || fabsf(d.c.y - c.y) > R)
            continue; // closed doors block light like walls do
        drawMeshM(M_CUBE, TRS(V3(d.c.x, 1.25f, d.c.y), d.horiz ? V3(d.w, 2.5f, .12f) : V3(.12f, 2.5f, d.w), 0), WHITE);
    }
    if (!haveChars)
        return;
    int n = 0;
    for (auto& z : zombies) {
        if (z.look < 0 || fabsf(z.p.x - c.x) > R || fabsf(z.p.y - c.y) > R || ++n > 50)
            continue;
        PoseIn pin;
        pin.kind = z.type == Z_BRUTE ? PK_BRUTE : PK_ZOMBIE;
        pin.phase = z.walk * 3.6f;
        pin.move = 1;
        pin.seed = z.seed;
        pin.armL = z.armL;
        pin.armR = z.armR;
        drawCharacter(z.look, pin, z.p, z.a, WHITE, 0, false);
    }
    if (withPlayer && state != ST_DEAD && state != ST_TITLE && playerLooks[0] >= 0) {
        PoseIn pin;
        pin.kind = PK_PLAYER;
        pin.phase = P.step * 1.85f;
        pin.crouch = P.crouch;
        pin.hold = curGunHold();
        drawCharacter(outfitLook(), pin, P.p, P.a, WHITE, 0, false);
    }
}
static void renderShadows() {
    rlActiveTextureSlot(11);
    rlDisableTexture();
    rlActiveTextureSlot(0);
    float dk = darkK();
    int on = shadowsOk && cfg.shadows > 0 && dk < .9f ? 1 : 0;
    shadowsActive = on;
    setU(uShadowOn, &on, SHADER_UNIFORM_INT);
    if (!on)
        return;
    Vector3 sd = sunDirNow();
    float vr = 48 / camZoom, ext = 2 * vr + 24, texel = ext / SHADOW_N;
    Vector3 tgt = V3(roundf(camPos.x / texel) * texel, 0, roundf((camPos.y - 5) / texel) * texel);
    Camera3D lc{};
    lc.target = tgt;
    lc.position = Vector3Subtract(tgt, Vector3Scale(sd, 200));
    lc.up = V3(0, 1, 0);
    lc.fovy = ext;
    lc.projection = CAMERA_ORTHOGRAPHIC;
    int one = 1, zero = 0;
    setU(uShadowPass, &one, SHADER_UNIFORM_INT);
    BeginTextureMode(shadowRT);
    ClearBackground(WHITE);
    BeginMode3D(lc);
    Matrix lv = rlGetMatrixModelview(), lp = rlGetMatrixProjection();
    drawCasters(V2(tgt.x, tgt.z), ext * .75f);
    EndMode3D();
    EndTextureMode();
    setU(uShadowPass, &zero, SHADER_UNIFORM_INT);
    setMatU(uLightVP, MatrixMultiply(lv, lp));
    float tx = 1.f / SHADOW_N;
    setU(uShadowTexel, &tx, SHADER_UNIFORM_FLOAT);
    rlActiveTextureSlot(11);
    rlEnableTexture(shadowRT.depth.id);
    rlActiveTextureSlot(0);
}
static void renderFlashShadow() {
    rlActiveTextureSlot(12);
    rlDisableTexture();
    rlActiveTextureSlot(0);
    int on = flOk && cfg.shadows > 0 && P.light && state != ST_TITLE && state != ST_DEAD ? 1 : 0;
    setU(uFlShadowOn, &on, SHADER_UNIFORM_INT);
    if (!on)
        return;
    Vector2 fd = polar(P.a, 1);
    Vector3 dir = Vector3Normalize(V3(fd.x, -.3f, fd.y));
    Vector3 pos = xz3(Vector2Add(P.p, polar(P.a, .3f)), 1.4f + (P.loft >= 0 ? lofts[P.loft].y : 0));
    Camera3D lc{};
    lc.position = pos;
    lc.target = Vector3Add(pos, dir);
    lc.up = V3(0, 1, 0);
    lc.fovy = P.uv ? 52 : 62;
    lc.projection = CAMERA_PERSPECTIVE;
    int one = 1, zero = 0;
    setU(uShadowPass, &one, SHADER_UNIFORM_INT);
    rlSetClipPlanes(.15, 40);
    BeginTextureMode(flRT);
    ClearBackground(WHITE);
    BeginMode3D(lc);
    Matrix lv = rlGetMatrixModelview(), lp = rlGetMatrixProjection();
    drawCasters(Vector2Add(P.p, Vector2Scale(fd, 12)), 18, false);
    EndMode3D();
    EndTextureMode();
    rlSetClipPlanes(1.0, 600.0);
    setU(uShadowPass, &zero, SHADER_UNIFORM_INT);
    setMatU(uFlVP, MatrixMultiply(lv, lp));
    rlActiveTextureSlot(12);
    rlEnableTexture(flRT.depth.id);
    rlActiveTextureSlot(0);
}
