// Nightfall County 3D — top-down open-world zombie survival. C++17 + raylib 5.5.
// Stage 1: fixed towns every run + random countryside, enterable buildings, survival stats,
// calibers & rarity loot, lighting shader with flashlight/fog/night, persistent 3D gore.
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <random>
#include <fstream>
#include <map>
#include <memory>
#include <iomanip>
#include <fstream>
#include "poses.h"

// The game is split into src/game/NN_*.h files, included here in order (one translation unit).
#include "game/01_util.h"
#include "game/02_settings.h"
#include "game/03_shaders.h"
#include "game/04_render_resources.h"
#include "game/05_surface_textures.h"
#include "game/06_gltf_assets.h"
#include "game/07_world_data.h"
#include "game/08_ground_paint.h"
#include "game/09_building_parts.h"
#include "game/10_buildings.h"
#include "game/11_wrecks.h"
#include "game/12_towns.h"
#include "game/13_life_map.h"
#include "game/14_interiors.h"
#include "game/15_gen_world.h"
#include "game/16_static_merge.h"
#include "game/17_items.h"
#include "game/18_clothing.h"
#include "game/19_entities.h"
#include "game/20_player.h"
#include "game/21_state.h"
#include "game/22_audio.h"
#include "game/23_inventory.h"
#include "game/24_effects.h"
#include "game/25_spawning.h"
#include "game/26_loot_gen.h"
#include "game/27_combat.h"
#include "game/28_death.h"
#include "game/29_classes.h"
#include "game/30_new_game.h"
#include "game/31_actions.h"
#include "game/32_interaction.h"
#include "game/33_update_player.h"
#include "game/34_zombie_ai.h"
#include "game/35_update_world.h"
#include "game/36_render_entities.h"
#include "game/37_shadows.h"
#include "game/38_render_world.h"
#include "game/39_hud.h"
#include "game/40_hotbar.h"
#include "game/41_inventory_ui.h"
#include "game/42_inventory_draw.h"
#include "game/43_map.h"
#include "game/44_save_load.h"
#include "game/45_menus.h"
#include "game/46_autotest.h"
#include "game/47_ambience.h"

// ============================================================ main
int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    loadCfg();
    InitWindow(1600, 900, TextFormat("Nightfall County %s", VERSION));
    if (cfg.fullscreen || cfg.res != 1)
        applyDisplay();
    SetExitKey(KEY_NULL);
    SetTargetFPS(cfg.fps);
    rlSetClipPlanes(1.0, 600.0);
    W = GetScreenWidth();
    H = GetScreenHeight();
    const char* fonts[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf", "/usr/share/fonts/noto/NotoSans-Bold.ttf"};
    font = GetFontDefault();
    for (const char* f : fonts)
        if (FileExists(f)) {
            int cps[256];
            int n = 0;
            for (int c = 32; c < 127; c++)
                cps[n++] = c;
            cps[n++] = 0xB0;
            cps[n++] = 0xB7;
            font = LoadFontEx(f, 64, cps, n);
            SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
            break;
        }
    initRender();
    SHADOW_N = cfg.shadows == 1 ? 1024 : 2048;
    initShadows();
    if (cfg.shadows == 0)
        shadowsOk = false;
    loadAllAssets();
    initSurfaces();
    initGroundAtlas();
    buildLooks();
    buildAudio();
    loadBest();
    HideCursor();
    rng.seed((unsigned)time(nullptr));
    // title backdrop: Cedar Falls at dusk
    genWorld();
    genLoot();
    P = Player();
    P.p = TOWNS[0].c;
    gTime = CYCLE * .43f;
    camPos = TOWNS[0].c;
    for (int i = 0; i < 30; i++) {
        Vector2 p = Vector2Add(TOWNS[0].c, polar(frand(0, 6), frand(5, 50)));
        if (freeAt(p.x, p.y, .6f))
            spawnZ(chance(.8f) ? Z_WALKER : Z_RUNNER, p);
    }
    float rainSndT = 0, windT = 0;
    while (!WindowShouldClose()) {
        W = GetScreenWidth();
        H = GetScreenHeight();
        float dt = std::min(.033f, GetFrameTime());
#ifdef NF_AUTOTEST
        dt = 1.f / 30;
        autotest();
#endif
        if (IsKeyPressed(KEY_F10))
            muted = !muted;
        if (IsKeyPressed(KEY_F11)) {
            cfg.fullscreen ^= 1;
            applyDisplay();
            saveCfg();
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0 &&
            (state == ST_TITLE || (state == ST_PLAY && !(IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)))))
            camZoom = Clamp(camZoom + wheel * .1f, .5f, 2.0f);
        if (state == ST_PLAY) {
            if (IsKeyPressed(KEY_SPACE))
                dodge();
            if (IsKeyPressed(KEY_R))
                startReload();
            if (IsKeyPressed(KEY_E))
                interact();
            if (IsKeyPressed(KEY_B))
                quickUse(false);
            if (IsKeyPressed(KEY_N))
                quickUse(true);
            if (IsKeyPressed(KEY_G))
                throwItem(false);
            if (IsKeyPressed(KEY_H))
                throwItem(true);
            if (IsKeyPressed(KEY_X)) {
                P.autorun = !P.autorun;
                P.autoA = P.moving ? P.moveA : P.a;
                if (P.autorun)
                    P.crouch = false;
            }
            if (IsKeyPressed(KEY_V) || IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
                melee();
            if (IsKeyPressed(KEY_F))
                toggleLight();
            if (IsKeyPressed(KEY_U) && P.light)
                P.uv = !P.uv;
            if (IsKeyPressed(KEY_C))
                P.crouch = !P.crouch;
            {
                static const int HK[10] = {KEY_ONE, KEY_TWO,   KEY_THREE, KEY_FOUR, KEY_FIVE,
                                           KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE, KEY_ZERO};
                for (int i = 0; i < 10; i++)
                    if (IsKeyPressed(HK[i]))
                        useHot(i);
            }
            if (IsKeyPressed(KEY_Q))
                kick();
            if (wheel != 0 && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))) {
                int dir = wheel > 0 ? 1 : 2;
                for (int k = 1; k <= 3; k++) {
                    int n = (P.active + k * dir) % 3;
                    if (n == 2 || P.hasGun[n]) {
                        setActive(n);
                        break;
                    }
                }
            }
            if (IsKeyPressed(KEY_Z))
                useActive();
            if (IsKeyPressed(KEY_TAB))
                state = ST_INV;
            else if (IsKeyPressed(KEY_M))
                state = ST_MAP;
            else if (IsKeyPressed(KEY_K))
                state = ST_SKILLS;
            else if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE))
                state = ST_PAUSE;
            else
                update(dt);
        } else if (state == ST_INV || state == ST_MAP || state == ST_SKILLS) { // world keeps running, like DayZ
            if (state == ST_INV && (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ESCAPE))) {
                cancelDrag();
                ctxMenu.on = false;
            }
            if ((state == ST_INV && (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ESCAPE))) ||
                (state == ST_MAP && (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE))) ||
                (state == ST_SKILLS && (IsKeyPressed(KEY_K) || IsKeyPressed(KEY_ESCAPE))))
                state = ST_PLAY;
            else {
                State s = state;
                state = ST_PLAY;
                uiOpen = true;
                update(dt);
                uiOpen = false;
                if (state == ST_PLAY)
                    state = s;
            }
        } else if (state == ST_PAUSE) {
            if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE))
                state = ST_PLAY;
        } else if (state == ST_SETTINGS) {
        } else if (state == ST_TITLE) {
            if (IsKeyPressed(KEY_ESCAPE))
                break;
            gTime += dt * .4f;
            camPos = Vector2Add(TOWNS[0].c, V2(cosf((float)GetTime() / 14) * 30, sinf((float)GetTime() / 14) * 22));
            for (auto& z : zombies) {
                z.wt -= dt;
                if (z.wt <= 0) {
                    z.wa = frand(0, 6.28f);
                    z.wt = frand(3, 7);
                }
                z.a = z.wa;
                Vector2 mv = polar(z.wa, .5f * dt);
                z.p = Vector2Add(z.p, mv);
                z.walk += .5f * dt;
                collide(z.p, z.r);
            }
            updateWorld(dt);
        } else if (state == ST_DEAD) {
            deadT += dt;
            updateWorld(dt);
            shake = std::max(0.f, shake - dt * 2);
        }
        // audio beds
        updateAmbience(dt);
        if (audioOk && !bedOk[BD_RAIN]) {
            rainSndT -= dt;
            windT -= dt;
            bool playing = state != ST_TITLE && !muted;
            if (playing && rainLevel > .3f && rainSndT <= 0) {
                SetSoundVolume(snd[SFX_RAIN][0], rainLevel * .6f);
                PlaySound(snd[SFX_RAIN][0]);
                rainSndT = 3.9f;
            }
            if (playing && isNight() && windT <= 0) {
                SetSoundVolume(snd[SFX_WIND][0], .35f);
                PlaySound(snd[SFX_WIND][0]);
                windT = 4.9f;
            }
        }
        // camera
        float shx = shake > 0 ? frand(-shake, shake) : 0, shz = shake > 0 ? frand(-shake, shake) : 0;
        Vector3 tgt = V3(camPos.x + shx, 0, camPos.y + shz);
        float dist = 27.f / camZoom;
        cam3.target = Vector3Add(tgt, V3(0, .8f, 0));
        cam3.position = Vector3Add(tgt, V3(0, dist * .86f, dist * .52f));
        cam3.up = V3(0, 1, 0);
        cam3.fovy = 42;
        cam3.projection = CAMERA_PERSPECTIVE;
        Matrix view = GetCameraMatrix(cam3);
        camRight = V3(view.m0, view.m4, view.m8);
        camUp = V3(view.m1, view.m5, view.m9);
        camFwd = Vector3Normalize(Vector3Subtract(cam3.target, cam3.position));
        computeFades(dt);
        BeginDrawing();
        setupLighting(cam3.position);
        renderShadows();
        renderFlashShadow();
        Vector3 fogC = Vector3Lerp(V3(.55f, .58f, .6f), V3(.006f, .008f, .014f), darkK());
        if (inCell(P.p) && state != ST_TITLE)
            fogC = V3(0, 0, 0);
        ClearBackground(rgba((int)(fogC.x * 255), (int)(fogC.y * 255), (int)(fogC.z * 255)));
        BeginMode3D(cam3);
        renderWorld3D();
        EndMode3D();
        // world-space floaters
        for (auto& f : floaters) {
            Vector2 s = GetWorldToScreen(f.p, cam3);
            text(f.txt, s.x, s.y, f.size, withA(f.c, std::min(1.f, f.t / f.mt * 2)), 1);
        }
        DrawTexturePro(vignTex, {0, 0, 512, 512}, {-W * .08f, -H * .08f, W * 1.16f, H * 1.16f}, V2(0, 0), 0,
                       rgba(0, 0, 0, (int)(140 + 60 * darkK())));
        if (cfg.grain)
            DrawTexturePro(grainTex,
                           {(float)GetRandomValue(0, 255), (float)GetRandomValue(0, 255), (float)W / 2, (float)H / 2},
                           {0, 0, (float)W, (float)H}, V2(0, 0), 0, rgba(255, 255, 255, 9));
        if (state == ST_PLAY || state == ST_PAUSE || state == ST_SKILLS || state == ST_INV || state == ST_MAP ||
            (state == ST_DEAD && deadT < 1.2f))
            drawHUD();
        if (state == ST_SETTINGS)
            drawSettings();
        else if (state == ST_TITLE)
            drawTitle();
        else if (state == ST_SKILLS)
            drawSkills();
        else if (state == ST_PAUSE)
            drawPause();
        else if (state == ST_DEAD)
            drawDead();
        else if (state == ST_INV)
            drawInventory();
        else if (state == ST_MAP)
            drawMap();
        EndDrawing();
    }
    if (audioOk)
        CloseAudioDevice();
    CloseWindow();
    return 0;
}
