// Nightfall County: state.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ state
enum State {
    ST_TITLE,
    ST_PLAY,
    ST_SKILLS,
    ST_PAUSE,
    ST_DEAD,
    ST_INV,
    ST_MAP,
    ST_SETTINGS,
    ST_TRADE
};
static State state = ST_TITLE;
static float gTime = 0, shake = 0, spawnT = 0, groanCd = 0, deadT = 0, rainLevel = 0, rainTarget = 0, rainT = 0;
static int zid = 0, kills = 0, nights = 0, pendingLv = 0, hordeDay = 0, lastTown = -2, bestRar = -1, gibCount = 0;
static char phase = 'd';
static std::string bestName;
struct Horde {
    bool on = false;
    float a = 0, t = 0;
    int left = 0;
    Vector2 end{}, lastSeen{};
} horde;
struct CarAlarm { // a searched car went off
    bool on = false;
    float t = 0, beat = 0;
    Vector2 p{};
} alarm;
struct Fire {
    Vector2 p;
    float fuel, smokeT = 0, lureT = 0;
    bool warned = false;
};
static std::vector<Fire> fires;
struct Banner {
    float t = 0;
    std::string a, b;
    Color c;
} banner;
struct Msg {
    std::string t;
    Color c;
    float life;
};
static std::vector<Msg> msgs;
static Vector2 camPos{MAP / 2, MAP / 2};
static float camZoom = 1.f;
static float camLow = 0;  // test scenes only: 0 normal camera, 1 low side-on view
static float camYaw = 0;  // orbit around the player (radians, 0 = looking north like it always was)
static float camTilt = 0; // player tilt: -.4 steeper/top-down .. .6 lower/side-on
// screen-local <-> world (xz). Local +x = screen right, local -z (y here) = screen up.
static Vector2 camRot(Vector2 v) {
    float c = cosf(camYaw), s = sinf(camYaw);
    return V2(v.x * c - v.y * s, v.x * s + v.y * c);
}
static Vector2 camUnrot(Vector2 v) {
    float c = cosf(camYaw), s = sinf(camYaw);
    return V2(v.x * c + v.y * s, -v.x * s + v.y * c);
}
// world-axis box that holds the (rotated) view rectangle: half-widths hx, hz around center ctr
static void viewAABB(Vector2 c, float vr, float padX, float padZ, Vector2& ctr, float& hx, float& hz) {
    float A = vr + padX, B = vr * .8f + padZ, cs = fabsf(cosf(camYaw)), sn = fabsf(sinf(camYaw));
    ctr = Vector2Add(c, camRot(V2(0, -6)));
    hx = cs * A + sn * B;
    hz = sn * A + cs * B;
}
static bool muted = false;
static int nearLoot = -1;
static bool uiOpen = false;
static int bestHours = 0, bestKills = 0;
constexpr float CYCLE = 720.f; // 12 min day
static float dayT() {
    return fmodf(gTime, CYCLE) / CYCLE;
} // 0 = 06:00
static int dayN() {
    return (int)(gTime / CYCLE) + 1;
}
static float hourOf() {
    return fmodf(6 + dayT() * 24, 24);
}
static bool isNight() {
    float h = hourOf();
    return h >= 20.5f || h < 5.5f;
}
static bool inBasement = false;
static float smoothstep01(float a, float b, float x) {
    float t = Clamp((x - a) / (b - a), 0, 1);
    return t * t * (3 - 2 * t);
}
static float darkK() {
    if (inBasement)
        return 1;
    float h = hourOf();
    return h >= 12 ? smoothstep01(18.5f, 21.f, h) : 1 - smoothstep01(4.5f, 7.f, h);
} // one eased curve, no corners
static void msg(const std::string& t, Color c = BONE) {
    msgs.push_back({t, c, 5});
    if (msgs.size() > 6)
        msgs.erase(msgs.begin());
}
static void showBanner(const std::string& a, const std::string& b, Color c = SODIUM) {
    banner = {4.5f, a, b, c};
}
static float xpNeed(int l) {
    return roundf(80 * powf((float)l, 1.5f));
}
