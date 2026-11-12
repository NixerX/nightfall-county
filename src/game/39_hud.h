// Nightfall County: hud.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ HUD & UI
static void text(const std::string& s, float x, float y, float size, Color c, int align = 0, bool shadow = true) {
    Vector2 m = MeasureTextEx(font, s.c_str(), size, size / 14);
    float ox = align == 1 ? -m.x / 2 : align == 2 ? -m.x : 0;
    if (shadow)
        DrawTextEx(font, s.c_str(), V2(x + ox + 1.5f, y + 1.5f), size, size / 14, rgba(0, 0, 0, (int)(c.a * .7f)));
    DrawTextEx(font, s.c_str(), V2(x + ox, y), size, size / 14, c);
}
static float textW(const std::string& s, float size) {
    return MeasureTextEx(font, s.c_str(), size, size / 14).x;
}
static void bar(float x, float y, float w, float h, float v, Color c) {
    DrawRectangleRec({x, y, w, h}, rgba(8, 10, 14, 180));
    DrawRectangleRec({x, y, w * Clamp(v, 0, 1), h}, c);
    DrawRectangleLinesEx({x, y, w, h}, 1, rgba(255, 255, 255, 28));
}
static std::string hhmm() {
    float h = hourOf();
    int hh = (int)h, mm = (int)((h - hh) * 60);
    char b[8];
    snprintf(b, sizeof b, "%02d:%02d", hh, mm);
    return b;
}
static void drawCursor() {
    Vector2 m = GetMousePosition();
    DrawTriangle(m, V2(m.x, m.y + 18), V2(m.x + 12, m.y + 13), BONE);
}
static bool button(const char* label, float x, float y, float w, float h, bool primary) {
    Rectangle r = {x, y, w, h};
    bool hov = CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRec(r, primary ? (hov ? rgba(255, 184, 90) : SODIUM) : (hov ? rgba(40, 46, 56) : rgba(18, 22, 30)));
    if (!primary)
        DrawRectangleLinesEx(r, 1, rgba(60, 68, 80));
    text(label, x + w / 2, y + h / 2 - 9, 18, primary ? rgba(26, 18, 6) : BONE, 1, false);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static void dim(float a) {
    DrawRectangle(0, 0, W, H, rgba(5, 6, 9, (int)(255 * a)));
}
static std::map<std::string, Texture2D> iconTex;
static Texture2D* icon(const std::string& n) {
    auto it = iconTex.find(n);
    if (it != iconTex.end())
        return it->second.id ? &it->second : nullptr;
    std::string p = (assetDir.empty() ? std::string("assets/") : assetDir) + "icons/" + n + ".png";
    Texture2D t{};
    if (FileExists(p.c_str())) {
        t = LoadTexture(p.c_str());
        SetTextureFilter(t, TEXTURE_FILTER_BILINEAR);
    }
    iconTex[n] = t;
    return t.id ? &iconTex[n] : nullptr;
}
static const char* ITEM_ICON[IT_COUNT] = {
    "opened-food-can", "peach",          "chocolate-bar",  "opened-food-can", "water-bottle",     "soda-can",
    "bandage-roll",    "medicine-pills", "pill",           "syringe",         "battery-pack",     "match-head",
    "dynamite",        "bullets",        "shotgun-rounds", "ammo-box",        "ammo-box",         "log",
    "wood-pile",       "wood-pile",      "matchbox",       "rolled-cloth",    "crime-scene-tape", "sewing-needle",
    "leather-vest",    "wax-tablet",     "chemical-drop",  "chemical-drop",   "chemical-drop",    "rope-coil",
    "jerrycan"};
static const char* MELEE_ICON[MK_COUNT] = {"fist",    "bowie-knife", "machete",       "wood-axe", "baseball-bat",
                                           "crowbar", "hatchet",     "monkey-wrench", "lead-pipe"};
static const char* GUN_ICON[W_COUNT] = {"pistol-gun", "pistol-gun", "mp5", "shotgun", "ak47", "ak47", "rifle"};
static const char* clothIcon(int def) {
    const char* n = CD[def].n;
    auto has = [&](const char* k) {
        return strstr(n, k) != nullptr;
    };
    switch (CD[def].slot) {
    case CS_HEAD:
        return has("Helmet") || has("Hard Hat") ? (has("Riot") || has("Combat") ? "visored-helm" : "helmet")
               : has("Cowboy")                  ? "western-hat"
                                                : "outback-hat";
    case CS_EYES:
        return has("Sunglasses") ? "sunglasses" : "spectacles";
    case CS_FACE:
        return has("Gas") ? "gas-mask" : has("Balaclava") ? "balaclava" : "bandana";
    case CS_TORSO:
        return has("T-Shirt") || has("Thermal") ? "t-shirt" : "shirt";
    case CS_OUTER:
        return has("Vest") || has("Carrier")     ? "kevlar-vest"
               : has("Hoodie") || has("Sweater") ? "hoodie"
                                                 : "moncler-jacket";
    case CS_HANDS:
        return has("Wool") ? "winter-gloves" : "gloves";
    case CS_LEGS:
        return "trousers";
    case CS_SOCKS:
        return "socks";
    case CS_FEET:
        return has("Rubber") ? "rubber-boot" : has("Sneakers") || has("Dress") ? "running-shoe" : "boots";
    default:
        return has("School") ? "school-bag" : has("Drawstring") ? "knapsack" : "backpack";
    }
}
static void drawIcon(const char* n, Rectangle r, Color c, float pad = .16f) {
    Texture2D* t = icon(n);
    if (!t)
        return;
    float sz = std::min(r.width, r.height) * (1 - 2 * pad);
    DrawTexturePro(*t, {0, 0, (float)t->width, (float)t->height},
                   {r.x + r.width / 2 - sz / 2, r.y + r.height / 2 - sz / 2, sz, sz}, V2(0, 0), 0, c);
}
static Color iconTint(Color c) {
    float l = (c.r + c.g + c.b) / 765.f;
    return l < .35f ? lerpC(c, rgba(230, 230, 230), .45f) : c;
} // keep dark colors readable
