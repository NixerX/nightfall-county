// Nightfall County: audio.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ audio (synthesized)
enum SfxId {
    SFX_PISTOL,
    SFX_SMG,
    SFX_SHOTGUN,
    SFX_RIFLE,
    SFX_HIT,
    SFX_BOOM,
    SFX_SPLAT,
    SFX_PICK,
    SFX_LOOT,
    SFX_LEGEND,
    SFX_HURT,
    SFX_BEEP,
    SFX_RELOAD,
    SFX_ROLL,
    SFX_GROAN,
    SFX_SCREAM,
    SFX_HORN,
    SFX_LEVEL,
    SFX_THROW,
    SFX_ROAR,
    SFX_EMPTY,
    SFX_GORE,
    SFX_SWING,
    SFX_CHOP,
    SFX_RAIN,
    SFX_WIND,
    SFX_EAT,
    SFX_STEP,
    SFX_DOOR_OPEN,
    SFX_DOOR_CLOSE,
    SFX_DOOR_BANG,
    SFX_CREAK,
    SFX_FIRE,
    SFX_HIT_WOOD,
    SFX_HIT_SLICE,
    SFX_HIT_HEAVY,
    SFX_HIT_PUNCH,
    SFX_ZATTACK,
    SFX_ZDEATH,
    SFX_THUNDER,
    SFX_STING,
    SFX_DOG,
    SFX_BLOCK,
    SFX_CAR_DOOR,
    SFX_DOOR_METAL,
    SFX_CAR_ALARM,
    SFX_COUNT
};
constexpr int ALIASES = 6;
static Sound snd[SFX_COUNT][ALIASES];
static int sndNext[SFX_COUNT];
static bool audioOk = false;
static std::vector<Sound> bank[SFX_COUNT];
struct Synth {
    std::vector<float> d;
    int sr = 44100;
    explicit Synth(float dur) : d((size_t)(dur * 44100), 0.f) {}
    void noise(float t0, float dur, float vol, float lp, float hp = 0, float decay = 6) {
        float yl = 0, yh = 0, xp = 0, al = 1 - expf(-2 * PI * lp / sr), ah = hp > 0 ? expf(-2 * PI * hp / sr) : 0;
        for (int i = (int)(t0 * sr); i < (int)((t0 + dur) * sr) && i < (int)d.size(); i++) {
            float t = (i - t0 * sr) / sr, x = frand(-1, 1);
            yl += al * (x - yl);
            float y = yl;
            if (hp > 0) {
                yh = ah * (yh + y - xp);
                xp = y;
                y = yh;
            }
            d[i] += y * vol * expf(-t * decay / dur);
        }
    }
    void tone(float t0, float dur, float f1, float f2, float vol, int wave = 0, float decay = 5, float attack = 0) {
        float ph = 0;
        for (int i = (int)(t0 * sr); i < (int)((t0 + dur) * sr) && i < (int)d.size(); i++) {
            float t = (i - t0 * sr) / sr, f = f1 * powf(f2 / f1, t / dur);
            ph += f / sr;
            float s = wave == 0   ? sinf(ph * 2 * PI)
                      : wave == 1 ? (fmodf(ph, 1) * 2 - 1)
                      : wave == 2 ? (fmodf(ph, 1) < .5f ? 1.f : -1.f)
                                  : (fabsf(fmodf(ph, 1) * 4 - 2) - 1);
            float env = expf(-t * decay / dur);
            if (attack > 0 && t < attack)
                env *= t / attack;
            d[i] += s * vol * env;
        }
    }
    Sound make() {
        std::vector<short> s(d.size());
        for (size_t i = 0; i < d.size(); i++)
            s[i] = (short)(Clamp(tanhf(d[i] * 1.2f), -1, 1) * 30000);
        Wave wv{};
        wv.frameCount = (unsigned)s.size();
        wv.sampleRate = sr;
        wv.sampleSize = 16;
        wv.channels = 1;
        wv.data = s.data();
        return LoadSoundFromWave(wv);
    }
};
static void buildAudio() {
    InitAudioDevice();
    audioOk = IsAudioDeviceReady();
    if (!audioOk)
        return;
    for (int id = 0; id < SFX_COUNT; id++) {
        Synth s(.1f);
        switch (id) {
        case SFX_PISTOL:
            s = Synth(.35f);
            s.noise(0, .3f, .9f, 4000, 250, 7);
            s.tone(0, .16f, 170, 50, .7f, 0, 6);
            s.noise(.05f, .3f, .15f, 1200, 0, 3);
            break;
        case SFX_SMG:
            s = Synth(.16f);
            s.noise(0, .12f, .7f, 5000, 400, 6);
            s.tone(0, .08f, 190, 70, .5f);
            break;
        case SFX_SHOTGUN:
            s = Synth(.7f);
            s.noise(0, .6f, 1.1f, 2200, 70, 6);
            s.tone(0, .35f, 120, 38, .9f, 0, 5);
            break;
        case SFX_RIFLE:
            s = Synth(1.f);
            s.noise(0, .9f, 1.1f, 3000, 50, 7);
            s.tone(0, .45f, 95, 30, 1.f);
            break;
        case SFX_HIT:
            s = Synth(.1f);
            s.noise(0, .08f, .6f, 1100, 150, 5);
            s.tone(0, .06f, 200, 90, .3f, 3);
            break;
        case SFX_BOOM:
            s = Synth(2.f);
            s.noise(0, 1.9f, 1.4f, 500, 0, 4);
            s.tone(0, 1.4f, 85, 22, 1.2f, 0, 4);
            break;
        case SFX_SPLAT:
            s = Synth(.5f);
            s.noise(0, .45f, 1.f, 700, 0, 5);
            s.tone(0, .3f, 150, 50, .4f, 1, 5);
            break;
        case SFX_PICK:
            s = Synth(.12f);
            s.noise(0, .08f, .4f, 3000, 600, 4);
            break;
        case SFX_LOOT:
            s = Synth(.35f);
            s.tone(0, .18f, 520, 1040, .3f, 3);
            s.tone(.08f, .22f, 780, 1560, .2f);
            break;
        case SFX_LEGEND: {
            s = Synth(1.f);
            float f[4] = {392, 523, 659, 784};
            for (int i = 0; i < 4; i++)
                s.tone(i * .09f, .6f, f[i], f[i] * 1.01f, .3f, 3, 3);
            break;
        }
        case SFX_HURT:
            s = Synth(.25f);
            s.tone(0, .18f, 170, 80, .45f, 2, 5);
            s.noise(0, .14f, .5f, 1200, 0, 5);
            break;
        case SFX_BEEP:
            s = Synth(.06f);
            s.tone(0, .05f, 1500, 1500, .25f, 2, 1);
            break;
        case SFX_RELOAD:
            s = Synth(.35f);
            s.noise(0, .05f, .5f, 6000, 1500, 4);
            s.noise(.2f, .06f, .6f, 5000, 1200, 4);
            break;
        case SFX_ROLL:
            s = Synth(.25f);
            s.noise(0, .22f, .5f, 700, 0, 3);
            break;
        case SFX_GROAN:
            s = Synth(1.3f);
            s.tone(0, 1.2f, frand(78, 98), 55, .25f, 1, 2.5f, .2f);
            s.noise(0, 1.2f, .14f, 380, 0, 3);
            break;
        case SFX_SCREAM:
            s = Synth(.8f);
            s.tone(0, .7f, 300, 1300, .35f, 1, 2, .05f);
            s.noise(0, .7f, .3f, 3000, 1000, 3);
            break;
        case SFX_CAR_ALARM:
            s = Synth(1.2f);
            s.tone(0, .6f, 600, 1400, .35f, 2, 0, .02f);
            s.tone(.6f, .6f, 1400, 600, .35f, 2, 0, .02f);
            break;
        case SFX_HORN:
            s = Synth(2.2f);
            s.tone(0, 2.1f, 110, 104, .4f, 1, 1.5f, .3f);
            s.tone(0, 2.1f, 165, 157, .25f, 1, 1.5f, .3f);
            break;
        case SFX_LEVEL: {
            s = Synth(.6f);
            float f[3] = {523, 659, 784};
            for (int i = 0; i < 3; i++)
                s.tone(i * .1f, .3f, f[i], f[i], .3f, 3, 3);
            break;
        }
        case SFX_THROW:
            s = Synth(.2f);
            s.noise(0, .16f, .4f, 2500, 600, 3);
            break;
        case SFX_ROAR:
            s = Synth(.9f);
            s.tone(0, .8f, 95, 48, .7f, 1, 2.5f, .05f);
            s.noise(0, .7f, .6f, 350, 0, 3);
            break;
        case SFX_EMPTY:
            s = Synth(.05f);
            s.tone(0, .03f, 900, 900, .2f, 2, 2);
            break;
        case SFX_GORE:
            s = Synth(.6f);
            s.noise(0, .5f, 1.1f, 900, 60, 4);
            s.noise(.05f, .3f, .7f, 300, 0, 3);
            s.tone(0, .25f, 90, 40, .6f, 1, 4);
            break;
        case SFX_SWING:
            s = Synth(.22f);
            s.noise(0, .2f, .5f, 3500, 900, 2);
            break;
        case SFX_CHOP:
            s = Synth(.3f);
            s.noise(0, .22f, 1.f, 1500, 100, 5);
            s.tone(0, .12f, 140, 60, .6f, 0, 4);
            break;
        case SFX_RAIN:
            s = Synth(4.f);
            s.noise(0, 4.f, .2f, 6000, 800, .01f);
            break;
        case SFX_WIND:
            s = Synth(5.f);
            s.noise(0, 5.f, .25f, 300, 60, .01f);
            break;
        case SFX_EAT:
            s = Synth(.4f);
            for (int i = 0; i < 3; i++)
                s.noise(i * .12f, .08f, .4f, 2000, 300, 4);
            break;
        case SFX_STEP:
            s = Synth(.08f);
            s.noise(0, .06f, .25f, 900, 120, 6);
            break;
        case SFX_DOOR_OPEN:
        case SFX_DOOR_CLOSE:
            s = Synth(.3f);
            s.noise(0, .25f, .6f, 900, 80, 5);
            s.tone(0, .12f, 110, 70, .4f, 0, 5);
            break;
        case SFX_DOOR_BANG:
            s = Synth(.35f);
            s.noise(0, .3f, 1.f, 600, 50, 6);
            s.tone(0, .2f, 80, 45, .8f, 0, 5);
            break;
        case SFX_CREAK:
            s = Synth(.5f);
            s.tone(0, .45f, 240, 180, .25f, 1, 2, .05f);
            break;
        case SFX_FIRE:
            s = Synth(1.5f);
            for (int i = 0; i < 14; i++)
                s.noise(frand(0, 1.4f), .03f, .5f, 3000, 600, 8);
            s.noise(0, 1.5f, .12f, 500, 0, .1f);
            break;
        }
        snd[id][0] = s.make();
        for (int a = 1; a < ALIASES; a++)
            snd[id][a] = LoadSoundAlias(snd[id][0]);
    }
    // recorded samples (Kenney, CC0) replace or layer over the synth where we have them
    std::string dir = assetDir.empty() ? std::string("assets/sounds/") : assetDir + "sounds/";
    auto add = [&](int id, const char* prefix, int n0, int n1, const char* fmt) {
        for (int i = n0; i <= n1; i++) {
            char fn[96];
            snprintf(fn, 96, fmt, prefix, i);
            std::string f = dir + fn;
            if (!FileExists(f.c_str()))
                continue;
            Sound so = LoadSound(f.c_str());
            if (!so.frameCount)
                continue;
            bank[id].push_back(so);
            bank[id].push_back(LoadSoundAlias(so));
        }
    };
    add(SFX_STEP, "footstep", 0, 9, "%s%02d.ogg");
    add(SFX_DOOR_OPEN, "doorOpen_", 0, 9, "%s%d.ogg");
    add(SFX_DOOR_OPEN, "doorOpen_", 0, 9, "%s%03d.ogg");
    add(SFX_DOOR_CLOSE, "doorClose_", 0, 9, "%s%d.ogg");
    add(SFX_DOOR_CLOSE, "doorClose_", 0, 9, "%s%03d.ogg");
    add(SFX_DOOR_BANG, "doorClose_", 1, 4, "%s%d.ogg");
    add(SFX_CHOP, "chop", 0, 0, "%s.ogg");
    add(SFX_CHOP, "knifeSlice", 0, 0, "%s.ogg");
    add(SFX_CHOP, "knifeSlice", 2, 2, "%s%d.ogg");
    add(SFX_SWING, "drawKnife", 1, 3, "%s%d.ogg");
    add(SFX_CREAK, "creak", 1, 3, "%s%d.ogg");
    add(SFX_PICK, "cloth", 1, 4, "%s%d.ogg");
    add(SFX_HIT_WOOD, "hit_wood_", 0, 9, "%s%d.ogg");
    add(SFX_HIT_SLICE, "hit_slice_", 0, 9, "%s%d.ogg");
    add(SFX_HIT_SLICE, "hit_blade_", 0, 0, "%s%d.ogg");
    add(SFX_HIT_HEAVY, "hit_heavy_", 0, 9, "%s%d.ogg");
    add(SFX_HIT_PUNCH, "hit_punch_", 0, 9, "%s%d.ogg");
    add(SFX_BLOCK, "hit_wood_", 0, 9, "%s%d.ogg");
    add(SFX_GROAN, "z_groan_", 0, 9, "%s%d.ogg");
    add(SFX_SCREAM, "z_scream_", 0, 9, "%s%d.ogg");
    add(SFX_ROAR, "z_brute_", 0, 9, "%s%d.ogg");
    add(SFX_ZATTACK, "z_attack_", 0, 9, "%s%d.ogg");
    add(SFX_ZDEATH, "z_death_", 0, 9, "%s%d.ogg");
    add(SFX_GORE, "gore_", 0, 9, "%s%d.ogg");
    add(SFX_THUNDER, "thunder_", 0, 9, "%s%d.ogg");
    add(SFX_STING, "sting_horde", 0, 0, "%s.ogg");
    add(SFX_DOG, "amb_dog_", 0, 9, "%s%d.ogg");
    add(SFX_DOOR_METAL, "door_metal_", 0, 9, "%s%d.ogg");
    add(SFX_DOOR_METAL, "impactMetal_", 0, 4, "%s%03d.ogg");
    bank[SFX_DOOR_BANG].clear();
    add(SFX_DOOR_BANG, "door_wood_", 0, 9, "%s%d.ogg");
    add(SFX_DOOR_BANG, "hit_heavy_", 0, 9, "%s%d.ogg");
    if (bank[SFX_DOOR_BANG].empty())
        add(SFX_DOOR_BANG, "doorClose_", 1, 4, "%s%d.ogg");
    add(SFX_CAR_DOOR, "car_door_", 0, 9, "%s%d.ogg");
    add(SFX_RELOAD, "metalLatch", 0, 0, "%s.ogg");
    add(SFX_EMPTY, "metalClick", 0, 0, "%s.ogg");
    add(SFX_BOOM, "explosionCrunch_", 0, 4, "%s%03d.ogg");
    add(SFX_BOOM, "lowFrequency_explosion_", 0, 1, "%s%03d.ogg");
    int nb = 0;
    for (auto& b : bank)
        nb += (int)b.size() / 2;
    TraceLog(LOG_INFO, "NIGHTFALL: %d recorded sound samples", nb);
}
struct CarAlarm {
    bool on = false;
    float t = 0, beat = 0;
    Vector2 p{};
};
static CarAlarm alarm;
static void startCarAlarm(Vector2 cp) {
    alarm.on = true;
    alarm.t = 10;
    alarm.beat = 0;
    alarm.p = cp;
    msg("Car alarm! Get moving.", rgba(255, 107, 91));
    int n = std::min(22, irand(12, 16) + dayN());
    float a = frand(0, 2 * PI);
    for (int i = 0; i < n; i++) {
        int type;
        if (dayN() >= 2 && chance(.05f))
            type = Z_BLOATER;
        else if (isNight() && chance(.25f))
            type = Z_RUNNER;
        else if (!isNight() && chance(.12f))
            type = Z_RUNNER;
        else
            type = Z_WALKER;
        spawnAround(type, 30, 45, a, .6f);
    }
    noiseAt(cp, 50);
}
static void sfx(int id, float vol = 1, float pv = .08f) {
    if (!audioOk || muted)
        return;
    vol *= cfg.master * cfg.sfx;
    if (!bank[id].empty()) {
        Sound& b = bank[id][irand(0, (int)bank[id].size() - 1)];
        SetSoundVolume(b, vol * (id == SFX_STEP ? .9f : .75f));
        SetSoundPitch(b, 1 + frand(-pv, pv));
        PlaySound(b);
        if (id != SFX_BOOM)
            return;
    }
    Sound& s = snd[id][sndNext[id]];
    sndNext[id] = (sndNext[id] + 1) % ALIASES;
    SetSoundVolume(s, vol * .6f);
    SetSoundPitch(s, 1 + frand(-pv, pv));
    PlaySound(s);
}
static void sfxAt(int id, Vector2 at, float vol = 1, float pv = .08f) {
    float d = Vector2Distance(at, P.p);
    if (d > 70)
        return;
    sfx(id, vol * Clamp(1 - d / 70, 0, 1), pv);
}
