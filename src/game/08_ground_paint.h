// Nightfall County: ground paint.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ------------------------------------------------------------ ground painting
// terrain splat: 12 photo layers + AO at 1 px per meter. Every paint call also paints a layer when curLayer >= 0.
// 0 short grass, 1 long grass, 2 lawn, 3 mossy forest floor, 4 leafy dirt, 5 packed earth, 6 gray dirt, 7 gravel, 8 dark soil, 9 concrete, 10 rubble dirt, 11 sand
constexpr int SPN = (int)MAP / 2, SPC = 13;
static std::vector<unsigned char> splat;
static int curLayer = -1; // splat: 1 px per 2 m
static void sblend(int sx, int sy, int layer, float a) {
    if (sx < 0 || sy < 0 || sx >= SPN || sy >= SPN)
        return;
    unsigned char* p = &splat[((size_t)sy * SPN + sx) * SPC];
    for (int k = 0; k < 12; k++) {
        float t = k == layer ? 255.f : 0.f;
        p[k] = (unsigned char)(p[k] + (t - p[k]) * Clamp(a, 0, 1));
    }
}
static void aoAt(int sx, int sy, float k) {
    if (sx < 0 || sy < 0 || sx >= SPN || sy >= SPN)
        return;
    unsigned char& o = splat[((size_t)sy * SPN + sx) * SPC + 12];
    o = (unsigned char)std::max((float)o, std::min(255.f, k * 255));
}
static void aoCircle(Vector2 c, float r, float k) {
    for (int y = (int)((c.y - r) / 2); y <= (int)((c.y + r) / 2); y++)
        for (int x = (int)((c.x - r) / 2); x <= (int)((c.x + r) / 2); x++) {
            float d = Vector2Distance(V2(x * 2 + 1.f, y * 2 + 1.f), c) / r;
            if (d < 1)
                aoAt(x, y, k * (1 - d * d));
        }
}
static void gblend(int x, int y, Color c, float a) {
    if (x >= 0 && y >= 0 && x < GTEX && y < GTEX) {
        Color& o = gpx[y * GTEX + x];
        o = lerpC(o, c, a);
        o.a = 255;
        if (curLayer >= 0 && !((x | y) & 1))
            sblend(x >> 1, y >> 1, curLayer, a);
    }
}
static void paintRect(float x0, float z0, float x1, float z1, Color c, float a = 1, float jitter = 0) {
    for (int y = (int)(z0 * PPM); y < (int)(z1 * PPM); y++)
        for (int x = (int)(x0 * PPM); x < (int)(x1 * PPM); x++)
            gblend(x, y, jitter > 0 ? mulC(c, 1 + frand(-jitter, jitter)) : c, a);
}
static void paintCircle(Vector2 c, float r, Color col, float a, bool soft = true) {
    for (int y = (int)((c.y - r) * PPM); y < (int)((c.y + r) * PPM); y++)
        for (int x = (int)((c.x - r) * PPM); x < (int)((c.x + r) * PPM); x++) {
            float d = Vector2Distance(V2((float)x / PPM, (float)y / PPM), c) / r;
            if (d < 1)
                gblend(x, y, col, a * (soft ? (1 - d * d) : 1));
        }
}
static void paintSeg(Vector2 a, Vector2 b, float w, Color c, float alpha) {
    float x0 = std::min(a.x, b.x) - w, x1 = std::max(a.x, b.x) + w, z0 = std::min(a.y, b.y) - w,
          z1 = std::max(a.y, b.y) + w;
    for (int y = (int)(z0 * PPM); y < (int)(z1 * PPM); y++)
        for (int x = (int)(x0 * PPM); x < (int)(x1 * PPM); x++) {
            float d = segDist(V2((float)x / PPM, (float)y / PPM), a, b);
            if (d < w)
                gblend(x, y, c, alpha * Clamp((w - d) / 1.5f, 0, 1));
        }
}

// soft contact darkening on the ground around a footprint (cheap baked AO)
static void aoRect(float x0, float z0, float x1, float z1, float R, float k) {
    for (int y = (int)((z0 - R) * PPM); y < (int)((z1 + R) * PPM); y++)
        for (int x = (int)((x0 - R) * PPM); x < (int)((x1 + R) * PPM); x++) {
            float px = (float)x / PPM, pz = (float)y / PPM, dx = std::max({x0 - px, 0.f, px - x1}),
                  dz = std::max({z0 - pz, 0.f, pz - z1}), d = sqrtf(dx * dx + dz * dz);
            if (d < R) {
                float a = 1 - d / R;
                gblend(x, y, rgba(10, 12, 8), k * a * a);
                if (!((x | y) & 1))
                    aoAt(x >> 1, y >> 1, k * a * a * .9f);
            }
        }
}
