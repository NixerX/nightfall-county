// Nightfall County: surface textures.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ surface textures
// Photo PBR sets from ambientCG (CC0) where we have them; procedural height maps (with derived normals) for the rest.
enum TexId {
    TX_NONE,
    TX_SIDING,
    TX_BRICK,
    TX_PLASTER,
    TX_SHINGLE,
    TX_PLANK,
    TX_TILE,
    TX_LOG,
    TX_METAL,
    TX_ASPHALT,
    TX_ASPHALT_WET,
    TX_CONCRETE,
    TX_HTILE,
    TX_SMTILE,
    TX_ROOFTILE,
    TX_CABIN,
    TX_HARDWOOD,
    TX_BARNWOOD,
    TX_PLYWOOD,
    TX_CHAIN,
    TX_COUNT
};
static Material surfMat[TX_COUNT];
static float surfScale[TX_COUNT];
static bool surfPhoto[TX_COUNT];
static unsigned hsh(int a, int b) {
    unsigned h = (unsigned)a * 374761393u + (unsigned)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float hf(int a, int b) {
    return (hsh(a, b) & 0xffff) / 65535.f;
}
static Texture2D texFromImage(Image im, bool srgbColor) {
    (void)srgbColor;
    Texture2D t = LoadTextureFromImage(im);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(t, TEXTURE_WRAP_REPEAT);
    return t;
}
static Image genSurfHeight(int kind) {
    const int N = 512;
    Image im = GenImageColor(N, N, WHITE);
    Color* px = (Color*)im.data;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            float u = x * 2.f / N, v = y * 2.f / N, val = 1, g = frand(-.035f, .035f);
            switch (kind) {
            case TX_SIDING: {
                float bh = .2f;
                int row = (int)(v / bh);
                float fv = fmodf(v, bh) / bh;
                val = .78f + .22f * fv + (hf(row, 3) - .5f) * .06f;
                if (fv < .07f)
                    val = .5f;
                break;
            }
            case TX_SHINGLE: {
                float rh = .22f, tw = .32f;
                int row = (int)(v / rh);
                float off = hf(row, 1) * tw;
                int col = (int)((u + off) / tw);
                float fv = fmodf(v, rh) / rh, tu = fmodf(u + off, tw);
                val = (.62f + .38f * fv) * (.82f + hf(row, col) * .3f);
                if (tu < .012f)
                    val *= .6f;
                if (fv < .06f)
                    val *= .55f;
                break;
            }
            case TX_PLANK: {
                float pw = .16f;
                int row = (int)(v / pw);
                float len = .9f + hf(row, 9) * 1.1f, off = hf(row, 2) * len;
                int seg = (int)((u + off) / len);
                float fv = fmodf(v, pw) / pw, fu = fmodf(u + off, len);
                val = .72f + hf(row, seg) * .3f + sinf(u * 40 + hf(row, seg) * 9) * .03f;
                if (fv < .05f || fu < .008f)
                    val *= .55f;
                break;
            }
            case TX_LOG: {
                float lh = .26f;
                int row = (int)(v / lh);
                float fv = fmodf(v, lh) / lh;
                val = .55f + .45f * sinf(fv * PI) + (hf(row, 5) - .5f) * .1f;
                break;
            }
            }
            float c = Clamp(val + g, 0, 1) * 255;
            px[y * N + x] = rgba((int)c, (int)c, (int)c);
        }
    return im;
}
static Image normalFromHeight(const Image& h, float strength) {
    int N = h.width;
    Image out = GenImageColor(N, N, BLANK);
    const Color* hp = (const Color*)h.data;
    Color* op = (Color*)out.data;
    auto H = [&](int x, int y) {
        return hp[((y + N) % N) * N + ((x + N) % N)].r / 255.f;
    };
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            float dx = (H(x + 1, y) - H(x - 1, y)) * strength, dy = (H(x, y + 1) - H(x, y - 1)) * strength;
            Vector3 n = Vector3Normalize(V3(-dx, dy, 1));
            op[y * N + x] =
                rgba((int)((n.x * .5f + .5f) * 255), (int)((n.y * .5f + .5f) * 255), (int)((n.z * .5f + .5f) * 255));
        }
    return out;
}
static std::string texDir();
static void initSurfaces() {
    static const char* photo[TX_COUNT] = {nullptr,
                                          nullptr,
                                          "wall_brick",
                                          "wall_plaster_dirty",
                                          nullptr,
                                          nullptr,
                                          "hospital_floor",
                                          nullptr,
                                          "metal_rusty_paint",
                                          "street_asphalt",
                                          "street_asphalt_wet",
                                          "sidewalk_concrete",
                                          "hospital_wall_tiles",
                                          "hospital_small_tiles",
                                          "roof_tiles",
                                          "cabin_logs",
                                          "hardwood_floor",
                                          "barn_wood",
                                          "plywood",
                                          "chainlink"};
    static const float scale[TX_COUNT] = {0,   .5f,  .5f, .33f, .5f,  .5f,  .4f, .5f,  .4f, .4f,
                                          .4f, .35f, .5f, .6f,  .45f, .35f, .4f, .35f, .5f, .5f};
    static const float roughP[TX_COUNT] = {0, .7f, 0, 0, .8f, .55f, 0, .85f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::string dir = texDir();
    for (int k = 1; k < TX_COUNT; k++) {
        surfMat[k] = LoadMaterialDefault();
        surfMat[k].shader = lit;
        surfScale[k] = scale[k];
        std::string base = photo[k] ? dir + photo[k] : "";
        std::string colF = base + "_Color.jpg";
        if (photo[k] && !FileExists(colF.c_str()))
            colF = base + "_Color.png";
        if (photo[k] && FileExists(colF.c_str())) {
            surfPhoto[k] = true;
            surfMat[k].maps[MATERIAL_MAP_NORMAL].texture = LoadTexture(colF.c_str());
            surfMat[k].maps[MATERIAL_MAP_ROUGHNESS].texture = LoadTexture((base + "_Normal.jpg").c_str());
            surfMat[k].maps[MATERIAL_MAP_OCCLUSION].texture = LoadTexture((base + "_Roughness.jpg").c_str());
            for (int m : {MATERIAL_MAP_NORMAL, MATERIAL_MAP_ROUGHNESS, MATERIAL_MAP_OCCLUSION}) {
                Texture2D& t = surfMat[k].maps[m].texture;
                GenTextureMipmaps(&t);
                SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
                SetTextureWrap(t, TEXTURE_WRAP_REPEAT);
            }
            continue;
        }
        // procedural fallback (or the four we never had photos for)
        int kind = k == TX_ROOFTILE                                                                         ? TX_SHINGLE
                   : k == TX_CABIN                                                                          ? TX_LOG
                   : k == TX_HARDWOOD || k == TX_BARNWOOD || k == TX_PLYWOOD                                ? TX_PLANK
                   : k == TX_CHAIN                                                                          ? TX_METAL
                   : (k == TX_BRICK || k == TX_PLASTER || k == TX_TILE || k == TX_METAL || k >= TX_ASPHALT) ? TX_PLANK
                                                                                                            : k;
        Image h = genSurfHeight(kind);
        Image nm = normalFromHeight(h, kind == TX_LOG ? 6.f : 9.f);
        Image r = GenImageColor(8, 8, rgba((int)(roughP[k] > 0 ? roughP[k] * 255 : 200), 0, 0));
        surfMat[k].maps[MATERIAL_MAP_NORMAL].texture = texFromImage(h, true);
        surfMat[k].maps[MATERIAL_MAP_ROUGHNESS].texture = texFromImage(nm, false);
        surfMat[k].maps[MATERIAL_MAP_OCCLUSION].texture = texFromImage(r, false);
        UnloadImage(h);
        UnloadImage(nm);
        UnloadImage(r);
    }
}
static Texture2D groundAtlas[2], splatTex[3];
static bool groundAtlasOk = false;
static void initGroundAtlas() { // 12 layers, 512px tiles with 16px wrapped gutters so mipmaps don't bleed across tiles
    const int T = 512, G = 16, C = T + 2 * G, AW = C * 4, AH = C * 3;
    std::string dir = texDir() + "ground/";
    for (int kind = 0; kind < 2; kind++) {
        Image at = GenImageColor(AW, AH, kind ? rgba(128, 128, 255) : GRAY);
        Color* ap = (Color*)at.data;
        for (int i = 0; i < 12; i++) {
            char fn[64];
            snprintf(fn, 64, "L%02d_%s.jpg", i, kind ? "Normal" : "Color");
            std::string f = dir + fn;
            if (!FileExists(f.c_str())) {
                UnloadImage(at);
                return;
            }
            Image im = LoadImage(f.c_str());
            ImageFormat(&im, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            if (im.width != T)
                ImageResize(&im, T, T);
            Color* p = (Color*)im.data;
            int ox = (i % 4) * C, oy = (i / 4) * C;
            for (int y = 0; y < C; y++)
                for (int x = 0; x < C; x++)
                    ap[(oy + y) * AW + ox + x] = p[((y - G + T) % T) * T + ((x - G + T) % T)];
            if (!kind) {
                double r = 0, g = 0, b = 0;
                for (int k = 0; k < T * T; k++) {
                    r += p[k].r;
                    g += p[k].g;
                    b += p[k].b;
                }
                float m[3] = {(float)(r / T / T / 255), (float)(g / T / T / 255), (float)(b / T / T / 255)};
                SetShaderValue(lit, GetShaderLocation(lit, TextFormat("layerMean[%d]", i)), m, SHADER_UNIFORM_VEC3);
            }
            UnloadImage(im);
        }
        groundAtlas[kind] = LoadTextureFromImage(at);
        UnloadImage(at);
        GenTextureMipmaps(&groundAtlas[kind]);
        SetTextureFilter(groundAtlas[kind], TEXTURE_FILTER_TRILINEAR);
        SetTextureWrap(groundAtlas[kind], TEXTURE_WRAP_CLAMP);
    }
    groundAtlasOk = true;
}
static int texModeFor(int tex, int mesh) {
    if (tex == TX_NONE)
        return 0;
    if (mesh == M_GABLE || tex == TX_SHINGLE || tex == TX_ROOFTILE)
        return 3;
    if (tex == TX_PLANK || tex == TX_TILE || tex == TX_SMTILE || tex == TX_ASPHALT || tex == TX_ASPHALT_WET ||
        tex == TX_CONCRETE || tex == TX_HARDWOOD)
        return 4;
    return 2;
}
static void drawTexMesh(int mesh, const Matrix& M, Color c, int tex) {
    if (tex == TX_NONE) {
        setTexMode(0);
        drawMeshM(mesh, M, c);
        return;
    }
    // photo textures carry their own color; the prop tint only nudges them
    if (surfPhoto[tex]) {
        float k = tex == TX_ASPHALT || tex == TX_ASPHALT_WET ? .62f : 1.f;
        c = rgba((int)((200 + (c.r - 128) / 4) * k), (int)((200 + (c.g - 128) / 4) * k),
                 (int)((200 + (c.b - 128) / 4) * k), c.a);
    }
    setTexMode(texModeFor(tex, mesh), surfScale[tex]);
    surfMat[tex].maps[MATERIAL_MAP_DIFFUSE].color = c;
    DrawMesh(meshes[mesh], surfMat[tex], M);
}
