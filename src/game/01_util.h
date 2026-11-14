// Nightfall County: util.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ util
static const char* VERSION = "v0.8.8";
static std::mt19937 rng(1337);
static float frand(float a, float b) {
    return a + (b - a) * std::uniform_real_distribution<float>(0.f, 1.f)(rng);
}
static int irand(int a, int b) {
    return std::uniform_int_distribution<int>(a, b)(rng);
}
static bool chance(float p) {
    return frand(0, 1) < p;
}
template <class T> static const T& pick(const std::vector<T>& v) {
    return v[irand(0, (int)v.size() - 1)];
}
static float angDiff(float a, float b) {
    float d = a - b;
    while (d > PI)
        d -= 2 * PI;
    while (d < -PI)
        d += 2 * PI;
    return d;
}
static Vector2 V2(float x, float y) {
    return Vector2{x, y};
}
static Vector3 V3(float x, float y, float z) {
    return Vector3{x, y, z};
}
static Vector2 polar(float a, float r) {
    return V2(cosf(a) * r, sinf(a) * r);
}
static Color rgba(int r, int g, int b, int a = 255) {
    return Color{(unsigned char)Clamp((float)r, 0, 255), (unsigned char)Clamp((float)g, 0, 255),
                 (unsigned char)Clamp((float)b, 0, 255), (unsigned char)Clamp((float)a, 0, 255)};
}
static Color withA(Color c, float a) {
    c.a = (unsigned char)Clamp(a * 255.f, 0, 255);
    return c;
}
static Color mulC(Color c, float m) {
    return rgba((int)(c.r * m), (int)(c.g * m), (int)(c.b * m), c.a);
}
static Color lerpC(Color a, Color b, float t) {
    return rgba((int)(a.r + (b.r - a.r) * t), (int)(a.g + (b.g - a.g) * t), (int)(a.b + (b.b - a.b) * t),
                (int)(a.a + (b.a - a.a) * t));
}
static Vector3 xz3(Vector2 p, float y) {
    return V3(p.x, y, p.y);
}
static float segDist(Vector2 p, Vector2 a, Vector2 b) {
    Vector2 ab = Vector2Subtract(b, a);
    float t = Clamp(Vector2DotProduct(Vector2Subtract(p, a), ab) / std::max(1e-4f, Vector2DotProduct(ab, ab)), 0, 1);
    return Vector2Distance(p, Vector2Add(a, Vector2Scale(ab, t)));
}

static const Color SODIUM = rgba(240, 164, 58), BONE = rgba(226, 221, 208), STEEL = rgba(134, 146, 160),
                   BLOODC = rgba(192, 48, 58), UVC = rgba(167, 139, 250), OKC = rgba(126, 210, 126),
                   COLDC = rgba(110, 170, 255);
static const Color RARC[4] = {rgba(207, 211, 214), rgba(111, 155, 255), rgba(242, 210, 75), rgba(255, 138, 31)};
static const char* RARN[4] = {"Common", "Magic", "Rare", "Legendary"};
