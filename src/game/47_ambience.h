// Nightfall County: ambience.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ ambience & adaptive music: looping beds whose volumes follow the world
enum Bed { BD_DAY, BD_NIGHT, BD_RAIN, BD_RAIN_IN, BD_WIND, BD_STORM, BD_FIRE, BD_FLIES, BD_TENSION, BD_HORDE, BD_VOLATILE, BD_HEART, BD_COUNT };
static Music beds[BD_COUNT]; static bool bedOk[BD_COUNT]; static float bedVol[BD_COUNT]; static bool bedsLoaded = false; static float thunderT = 30, dogT = 40;
static void loadBeds() {
    static const char* f[BD_COUNT] = {"amb_day", "amb_night", "amb_rain", "amb_rain_in", "amb_wind", "amb_storm", "amb_fire", "amb_flies", "mus_tension", "mus_horde", "mus_night", "mus_heart"};
    std::string dir = assetDir.empty() ? std::string("assets/sounds/") : assetDir + "sounds/";
    for (int i = 0; i < BD_COUNT; i++) { std::string p = dir + f[i] + ".ogg"; if (!FileExists(p.c_str())) continue; beds[i] = LoadMusicStream(p.c_str()); bedOk[i] = beds[i].frameCount > 0;
        if (bedOk[i]) { beds[i].looping = true; SetMusicVolume(beds[i], 0); PlayMusicStream(beds[i]); } }
    bedsLoaded = true;
}
static void updateAmbience(float dt) {
    if (!audioOk) return; if (!bedsLoaded) loadBeds();
    float tgt[BD_COUNT] = {}; bool on = state != ST_TITLE && !muted;
    float dk = darkK(), rain = rainLevel; bool inside = buildingAt(P.p) >= 0 || P.loft >= 0; float out = inside ? .35f : 1;
    if (on) {
        tgt[BD_DAY] = (1 - dk) * (1 - rain * .8f) * .45f * out; tgt[BD_NIGHT] = dk * (1 - rain * .6f) * .5f * out;
        tgt[BD_RAIN] = inside ? 0 : rain * .55f; tgt[BD_RAIN_IN] = inside ? rain * .6f : 0; tgt[BD_STORM] = Clamp((rain - .7f) * 3, 0, 1) * .45f * out;
        tgt[BD_WIND] = (dk * .18f + rain * .2f) * out;
        float fd = 99; for (auto& f : fires) fd = std::min(fd, Vector2Distance(f.p, P.p)); for (auto& l : staticLights) if (l.kind <= 2) fd = std::min(fd, Vector2Distance(V2(l.p.x, l.p.z), P.p));
        tgt[BD_FIRE] = Clamp(1 - fd / 14, 0, 1) * .6f;
        // threat: the closest zombie that actually knows you're there; the horde gets its own layer
        float near = 99, hordeD = 999; for (auto& z : zombies) { float d = Vector2Distance(z.p, P.p); if (z.sees || z.aggro > 0) near = std::min(near, d); if (z.hordeM) hordeD = std::min(hordeD, d); }
        int swarm = 0; for (auto& z : zombies) if (!z.dead && Vector2Distance(z.p, P.p) < 12) swarm++; // flies only when the dead are piled up around you
        tgt[BD_FLIES] = Clamp((swarm - 5) / 10.f, 0, 1) * .55f + Clamp(1 - hordeD / 30, 0, 1) * .35f;
        tgt[BD_TENSION] = Clamp(1 - near / 32, 0, 1) * .6f; tgt[BD_HORDE] = Clamp(1 - hordeD / 110, 0, 1) * .75f;
        bool vol = false; for (auto& z : zombies) if (z.type == Z_VOLATILE && Vector2Distance(z.p, P.p) < 45) vol = true; tgt[BD_VOLATILE] = (vol && dk > .5f) ? .45f : 0;
        tgt[BD_HEART] = std::max(Clamp(1 - near / 7, 0, 1), P.hp < P.perks.maxhp * .3f ? .7f : 0.f) * .7f;
        if (state == ST_DEAD) for (float& t : tgt) t *= .3f;
        thunderT -= dt; if (rain > .75f && thunderT <= 0) { thunderT = frand(18, 50); sfx(SFX_THUNDER, inside ? .5f : .9f, .1f); }
        dogT -= dt; if (dogT <= 0) { dogT = frand(40, 110); if (townAt(P.p, 1.4f) >= 0 && dk < .7f) sfx(SFX_DOG, .25f, .1f); }
    }
    for (int i = 0; i < BD_COUNT; i++) { if (!bedOk[i]) continue; float k = i == BD_TENSION || i == BD_HEART ? 2.5f : .6f; bedVol[i] += (tgt[i] - bedVol[i]) * std::min(1.f, dt * k);
        bool music = i >= BD_TENSION; SetMusicVolume(beds[i], bedVol[i] * cfg.master * (music ? cfg.music : cfg.sfx)); if (i == BD_HEART) SetMusicPitch(beds[i], 1 + Clamp(1 - (P.hp / P.perks.maxhp), 0, 1) * .5f);
        UpdateMusicStream(beds[i]); }
}

