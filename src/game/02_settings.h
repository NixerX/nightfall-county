// Nightfall County: settings.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ settings (saved to ~/.nightfall3d_settings)
struct Settings { int fps = 144; int shadows = 2; int foliage = 2; float master = 1, sfx = 1, music = .8f; int controls = 1; int diff = 1; int fullscreen = 0; int res = 1; int grain = 1; int autoEquip = 0; } cfg;
static const int RES[6][2] = {{1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3840, 2160}, {0, 0}}; // last = native monitor
static const char* DIFFN[4] = {"Easy", "Normal", "Hard", "Nightmare"};
static std::string cfgPath() { const char* h = getenv("HOME"); return std::string(h ? h : ".") + "/.nightfall3d_settings"; }
static void loadCfg() { std::ifstream f(cfgPath()); if (f) f >> cfg.fps >> cfg.shadows >> cfg.foliage >> cfg.master >> cfg.sfx >> cfg.music >> cfg.controls >> cfg.diff >> cfg.fullscreen >> cfg.res; if (!(f >> cfg.grain)) cfg.grain = 1; if (!(f >> cfg.autoEquip)) cfg.autoEquip = 0; cfg.diff = std::max(0, std::min(3, cfg.diff)); cfg.res = std::max(0, std::min(5, cfg.res)); cfg.fullscreen = cfg.fullscreen ? 1 : 0; cfg.controls = std::max(0, std::min(2, cfg.controls)); cfg.shadows = std::max(0, std::min(2, cfg.shadows)); cfg.foliage = std::max(0, std::min(2, cfg.foliage)); }
static void saveCfg() { std::ofstream f(cfgPath()); f << cfg.fps << " " << cfg.shadows << " " << cfg.foliage << " " << cfg.master << " " << cfg.sfx << " " << cfg.music << " " << cfg.controls << " " << cfg.diff << " " << cfg.fullscreen << " " << cfg.res << " " << cfg.grain << " " << cfg.autoEquip; }
