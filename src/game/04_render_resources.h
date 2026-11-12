// Nightfall County: render resources.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ render resources
enum MeshId {
    M_CUBE,
    M_SPHERE,
    M_CYL,
    M_CONE,
    M_GABLE,
    M_COUNT
};
static Mesh meshes[M_COUNT];
static Mesh groundMesh;
static Material litMat, groundMat;
static Shader lit, litSkin;
enum {
    uSunDir,
    uSunCol,
    uAmb,
    uSky,
    uFogCol,
    uFogC,
    uFogN,
    uFogF,
    uView,
    uWet,
    uDetail,
    uFlPos,
    uFlDir,
    uFlCol,
    uFlCos,
    uFlRange,
    uNL,
    uLPos,
    uLCol,
    uTexMode,
    uTexScale,
    uShadowOn,
    uShadowTexel,
    uShadowPass,
    uLightVP,
    uShadowMap,
    uExposure,
    uFlVP,
    uFlShadowMap,
    uFlShadowOn,
    U_COUNT
};
static const char* UNAMES[U_COUNT] = {"sunDir",  "sunCol",    "ambCol",   "skyCol",   "fogCol",      "fogCenter",
                                      "fogNear", "fogFar",    "viewPos",  "wet",      "useDetail",   "flPos",
                                      "flDir",   "flCol",     "flCos",    "flRange",  "nLights",     "lPos",
                                      "lCol",    "texMode",   "texScale", "shadowOn", "shadowTexel", "shadowPass",
                                      "lightVP", "shadowMap", "exposure", "flVP",     "flShadowMap", "flShadowOn"};
static int ULOC[2][U_COUNT];
static void setU(int u, const void* v, int type, int count = 1) {
    Shader* sh[2] = {&lit, &litSkin};
    for (int k = 0; k < 2; k++)
        if (ULOC[k][u] >= 0) {
            if (count > 1)
                SetShaderValueV(*sh[k], ULOC[k][u], v, type, count);
            else
                SetShaderValue(*sh[k], ULOC[k][u], v, type);
        }
}
static void setTexMode(int m, float sc = .5f) {
    static int last = -1;
    static float lastS = -1;
    if (m == last && sc == lastS)
        return;
    rlDrawRenderBatchActive();
    last = m;
    lastS = sc;
    SetShaderValue(lit, ULOC[0][uTexMode], &m, SHADER_UNIFORM_INT);
    SetShaderValue(lit, ULOC[0][uTexScale], &sc, SHADER_UNIFORM_FLOAT);
}
static Texture2D groundTex, detailTex, bloodTex, softTex, vignTex, grainTex, mapTex;
static Font font;
static Camera3D cam3{};
static int W = 1600, H = 900;

static Mesh buildMesh(const std::vector<Vector3>& tri, const std::vector<Vector2>* uv = nullptr) {
    Mesh m{};
    m.vertexCount = (int)tri.size();
    m.triangleCount = m.vertexCount / 3;
    m.vertices = (float*)MemAlloc(sizeof(float) * 3 * m.vertexCount);
    m.normals = (float*)MemAlloc(sizeof(float) * 3 * m.vertexCount);
    m.texcoords = (float*)MemAlloc(sizeof(float) * 2 * m.vertexCount);
    for (int i = 0; i < m.vertexCount; i += 3) {
        Vector3 n = Vector3Normalize(
            Vector3CrossProduct(Vector3Subtract(tri[i + 1], tri[i]), Vector3Subtract(tri[i + 2], tri[i])));
        for (int k = 0; k < 3; k++) {
            const Vector3& v = tri[i + k];
            m.vertices[(i + k) * 3] = v.x;
            m.vertices[(i + k) * 3 + 1] = v.y;
            m.vertices[(i + k) * 3 + 2] = v.z;
            m.normals[(i + k) * 3] = n.x;
            m.normals[(i + k) * 3 + 1] = n.y;
            m.normals[(i + k) * 3 + 2] = n.z;
            Vector2 t = uv ? (*uv)[i + k] : V2(v.x + .5f, v.z + .5f);
            m.texcoords[(i + k) * 2] = t.x;
            m.texcoords[(i + k) * 2 + 1] = t.y;
        }
    }
    UploadMesh(&m, false);
    return m;
}
static Mesh gableMesh() { // base [-.5,.5]^2 at y=0, ridge along x at y=1, z=0. CCW from outside.
    Vector3 a = V3(-.5f, 0, -.5f), b = V3(.5f, 0, -.5f), c = V3(.5f, 0, .5f), d = V3(-.5f, 0, .5f), r0 = V3(-.5f, 1, 0),
            r1 = V3(.5f, 1, 0);
    std::vector<Vector3> t = {a, r0, r1, a, r1, b, d, c, r1, d, r1, r0, a, d, r0, b, r1, c};
    return buildMesh(t);
}
static void initRender() {
    meshes[M_CUBE] = GenMeshCube(1, 1, 1);
    meshes[M_SPHERE] = GenMeshSphere(.5f, 8, 10);
    meshes[M_CYL] = GenMeshCylinder(.5f, 1, 10);
    meshes[M_CONE] = GenMeshCone(.5f, 1, 8);
    meshes[M_GABLE] = gableMesh();
    lit = LoadShaderFromMemory(LIT_VS, LIT_FS);
    lit.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(lit, "matModel");
    lit.locs[SHADER_LOC_MATRIX_NORMAL] = GetShaderLocation(lit, "matNormal");
    lit.locs[SHADER_LOC_MAP_SPECULAR] = GetShaderLocation(lit, "texture1");
    lit.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(lit, "texture2");
    lit.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(lit, "texture3");
    lit.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(lit, "texture4");
    lit.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(lit, "texture5");
    lit.locs[SHADER_LOC_MAP_HEIGHT] = GetShaderLocation(lit, "texture6");
    litSkin = LoadShaderFromMemory(SKIN_VS, LIT_FS);
    litSkin.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(litSkin, "matModel");
    litSkin.locs[SHADER_LOC_MATRIX_NORMAL] = GetShaderLocation(litSkin, "matNormal");
    litSkin.locs[SHADER_LOC_BONE_MATRICES] = GetShaderLocation(litSkin, "boneMatrices");
    litSkin.locs[SHADER_LOC_VERTEX_BONEIDS] = GetShaderLocationAttrib(litSkin, "vertexBoneIds");
    litSkin.locs[SHADER_LOC_VERTEX_BONEWEIGHTS] = GetShaderLocationAttrib(litSkin, "vertexBoneWeights");
    for (int u = 0; u < U_COUNT; u++) {
        ULOC[0][u] = GetShaderLocation(lit, UNAMES[u]);
        ULOC[1][u] = GetShaderLocation(litSkin, UNAMES[u]);
    }
    {
        int z = 0;
        setU(uTexMode, &z, SHADER_UNIFORM_INT);
        setU(uDetail, &z, SHADER_UNIFORM_INT);
        setU(uShadowOn, &z, SHADER_UNIFORM_INT);
        setU(uShadowPass, &z, SHADER_UNIFORM_INT);
        int slot = 11;
        setU(uShadowMap, &slot, SHADER_UNIFORM_INT);
        int slot2 = 12;
        setU(uFlShadowMap, &slot2, SHADER_UNIFORM_INT);
        setU(uFlShadowOn, &z, SHADER_UNIFORM_INT);
        float ex = 1.5f;
        setU(uExposure, &ex, SHADER_UNIFORM_FLOAT);
        float ts = .5f;
        setU(uTexScale, &ts, SHADER_UNIFORM_FLOAT);
    }
    litMat = LoadMaterialDefault();
    litMat.shader = lit;
    // detail noise
    Image dn = GenImagePerlinNoise(256, 256, 0, 0, 5.f);
    detailTex = LoadTextureFromImage(dn);
    UnloadImage(dn);
    SetTextureWrap(detailTex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(detailTex, TEXTURE_FILTER_BILINEAR);
    GenTextureMipmaps(&detailTex);
    // blood atlas: 4 splat variants in 256x64
    Image bi = GenImageColor(256, 64, BLANK);
    for (int v = 0; v < 4; v++) {
        std::vector<Vector3> blobs;
        int nb = 5 + v * 3;
        for (int i = 0; i < nb; i++)
            blobs.push_back(V3(32 + frand(-14, 14) * (i ? 1.f : 0.f), 32 + frand(-14, 14) * (i ? 1.f : 0.f),
                               i ? frand(3, 9) : frand(12, 17)));
        for (int i = 0; i < 6 + v; i++) {
            float a = frand(0, 2 * PI);
            for (int k = 0; k < 6; k++)
                blobs.push_back(V3(32 + cosf(a) * (14 + k * 2.6f), 32 + sinf(a) * (14 + k * 2.6f), 2.6f - k * .35f));
        }
        for (int y = 0; y < 64; y++)
            for (int x = 0; x < 64; x++) {
                float f = 0;
                for (auto& b : blobs) {
                    float dx = x - b.x, dy = y - b.y;
                    f += b.z * b.z / (dx * dx + dy * dy + 1);
                }
                float a = Clamp((f - .9f) * 3.f, 0, 1);
                if (a > 0)
                    ImageDrawPixel(&bi, v * 64 + x, y,
                                   rgba(255, 255, 255, (int)(a * 255 * (.75f + .25f * frand(0, 1)))));
            }
    }
    bloodTex = LoadTextureFromImage(bi);
    UnloadImage(bi);
    SetTextureFilter(bloodTex, TEXTURE_FILTER_BILINEAR);
    Image si = GenImageGradientRadial(64, 64, 0.f, WHITE, BLANK);
    softTex = LoadTextureFromImage(si);
    UnloadImage(si);
    SetTextureFilter(softTex, TEXTURE_FILTER_BILINEAR);
    Image vi = GenImageGradientRadial(512, 512, .35f, rgba(0, 0, 0, 0), rgba(0, 0, 0, 255));
    vignTex = LoadTextureFromImage(vi);
    UnloadImage(vi);
    SetTextureFilter(vignTex, TEXTURE_FILTER_BILINEAR);
    Image gi = GenImageWhiteNoise(256, 256, .5f);
    grainTex = LoadTextureFromImage(gi);
    UnloadImage(gi);
    SetTextureWrap(grainTex, TEXTURE_WRAP_REPEAT);
}
static Matrix TRS(Vector3 p, Vector3 s, float yaw) {
    return MatrixMultiply(MatrixMultiply(MatrixScale(s.x, s.y, s.z), MatrixRotateY(-yaw)),
                          MatrixTranslate(p.x, p.y, p.z));
}
static void drawMeshM(int m, const Matrix& M, Color c) {
    litMat.maps[MATERIAL_MAP_DIFFUSE].color = c;
    DrawMesh(meshes[m], litMat, M);
}
static void drawBox(Vector3 center, Vector3 size, float yaw, Color c) {
    drawMeshM(M_CUBE, TRS(center, size, yaw), c);
}
