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

// ============================================================ util
static const char* VERSION = "v0.8.7";
static std::mt19937 rng(1337);
static float frand(float a, float b) { return a + (b - a) * std::uniform_real_distribution<float>(0.f, 1.f)(rng); }
static int irand(int a, int b) { return std::uniform_int_distribution<int>(a, b)(rng); }
static bool chance(float p) { return frand(0, 1) < p; }
template <class T> static const T& pick(const std::vector<T>& v) { return v[irand(0, (int)v.size() - 1)]; }
static float angDiff(float a, float b) { float d = a - b; while (d > PI) d -= 2 * PI; while (d < -PI) d += 2 * PI; return d; }
static Vector2 V2(float x, float y) { return Vector2{x, y}; }
static Vector3 V3(float x, float y, float z) { return Vector3{x, y, z}; }
static Vector2 polar(float a, float r) { return V2(cosf(a) * r, sinf(a) * r); }
static Color rgba(int r, int g, int b, int a = 255) { return Color{(unsigned char)Clamp((float)r, 0, 255), (unsigned char)Clamp((float)g, 0, 255), (unsigned char)Clamp((float)b, 0, 255), (unsigned char)Clamp((float)a, 0, 255)}; }
static Color withA(Color c, float a) { c.a = (unsigned char)Clamp(a * 255.f, 0, 255); return c; }
static Color mulC(Color c, float m) { return rgba((int)(c.r * m), (int)(c.g * m), (int)(c.b * m), c.a); }
static Color lerpC(Color a, Color b, float t) { return rgba((int)(a.r + (b.r - a.r) * t), (int)(a.g + (b.g - a.g) * t), (int)(a.b + (b.b - a.b) * t), (int)(a.a + (b.a - a.a) * t)); }
static Vector3 xz3(Vector2 p, float y) { return V3(p.x, y, p.y); }
static float segDist(Vector2 p, Vector2 a, Vector2 b) { Vector2 ab = Vector2Subtract(b, a); float t = Clamp(Vector2DotProduct(Vector2Subtract(p, a), ab) / std::max(1e-4f, Vector2DotProduct(ab, ab)), 0, 1); return Vector2Distance(p, Vector2Add(a, Vector2Scale(ab, t))); }

static const Color SODIUM = rgba(240, 164, 58), BONE = rgba(226, 221, 208), STEEL = rgba(134, 146, 160), BLOODC = rgba(192, 48, 58), UVC = rgba(167, 139, 250), OKC = rgba(126, 210, 126), COLDC = rgba(110, 170, 255);
static const Color RARC[4] = {rgba(207, 211, 214), rgba(111, 155, 255), rgba(242, 210, 75), rgba(255, 138, 31)};
static const char* RARN[4] = {"Common", "Magic", "Rare", "Legendary"};

// ============================================================ settings (saved to ~/.nightfall3d_settings)
struct Settings { int fps = 144; int shadows = 2; int foliage = 2; float master = 1, sfx = 1, music = .8f; int controls = 1; int diff = 1; int fullscreen = 0; int res = 1; int grain = 1; } cfg;
static const int RES[6][2] = {{1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3840, 2160}, {0, 0}}; // last = native monitor
static const char* DIFFN[4] = {"Easy", "Normal", "Hard", "Nightmare"};
static std::string cfgPath() { const char* h = getenv("HOME"); return std::string(h ? h : ".") + "/.nightfall3d_settings"; }
static void loadCfg() { std::ifstream f(cfgPath()); if (f) f >> cfg.fps >> cfg.shadows >> cfg.foliage >> cfg.master >> cfg.sfx >> cfg.music >> cfg.controls >> cfg.diff >> cfg.fullscreen >> cfg.res; if (!(f >> cfg.grain)) cfg.grain = 1; cfg.diff = std::max(0, std::min(3, cfg.diff)); cfg.res = std::max(0, std::min(5, cfg.res)); cfg.fullscreen = cfg.fullscreen ? 1 : 0; cfg.controls = std::max(0, std::min(2, cfg.controls)); cfg.shadows = std::max(0, std::min(2, cfg.shadows)); cfg.foliage = std::max(0, std::min(2, cfg.foliage)); }
static void saveCfg() { std::ofstream f(cfgPath()); f << cfg.fps << " " << cfg.shadows << " " << cfg.foliage << " " << cfg.master << " " << cfg.sfx << " " << cfg.music << " " << cfg.controls << " " << cfg.diff << " " << cfg.fullscreen << " " << cfg.res << " " << cfg.grain; }

// ============================================================ shaders
static const char* LIT_VS = R"(#version 330
in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal; in vec4 vertexColor;
uniform mat4 mvp; uniform mat4 matModel; uniform mat4 matNormal;
out vec3 fragPos; out vec2 fragTexCoord; out vec4 fragColor; out vec3 fragNormal;
void main(){
  fragPos = vec3(matModel*vec4(vertexPosition,1.0));
  fragTexCoord = vertexTexCoord; fragColor = vertexColor;
  fragNormal = normalize(vec3(matNormal*vec4(vertexNormal,0.0)));
  gl_Position = mvp*vec4(vertexPosition,1.0);
})";
static const char* SKIN_VS = R"(#version 330
in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal; in vec4 vertexColor;
in vec4 vertexBoneIds; in vec4 vertexBoneWeights;
uniform mat4 boneMatrices[64];
uniform mat4 mvp; uniform mat4 matModel; uniform mat4 matNormal;
out vec3 fragPos; out vec2 fragTexCoord; out vec4 fragColor; out vec3 fragNormal;
void main(){
  mat4 S = boneMatrices[int(vertexBoneIds.x)]*vertexBoneWeights.x + boneMatrices[int(vertexBoneIds.y)]*vertexBoneWeights.y
         + boneMatrices[int(vertexBoneIds.z)]*vertexBoneWeights.z + boneMatrices[int(vertexBoneIds.w)]*vertexBoneWeights.w;
  vec4 p = S*vec4(vertexPosition,1.0); vec3 n = mat3(S)*vertexNormal;
  fragPos = vec3(matModel*p); fragTexCoord = vertexTexCoord; fragColor = vertexColor;
  fragNormal = normalize(vec3(matNormal*vec4(n,0.0)));
  gl_Position = mvp*p;
})";
static const char* LIT_FS = R"(#version 330
in vec3 fragPos; in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragNormal;
uniform sampler2D texture0; uniform sampler2D texture1; uniform sampler2D texture2; uniform sampler2D texture3; uniform sampler2D texture4; uniform sampler2D texture5; uniform sampler2D texture6;
uniform sampler2D shadowMap; uniform mat4 lightVP; uniform int shadowOn; uniform float shadowTexel; uniform int shadowPass;
uniform sampler2D flShadowMap; uniform mat4 flVP; uniform int flShadowOn;
uniform vec4 colDiffuse; uniform int texMode; uniform float texScale; uniform vec3 layerMean[12];
uniform vec3 sunDir; uniform vec3 sunCol; uniform vec3 ambCol; uniform vec3 skyCol;
uniform vec3 fogCol; uniform vec2 fogCenter; uniform float fogNear; uniform float fogFar;
uniform vec3 viewPos; uniform float wet; uniform int useDetail; uniform float exposure;
uniform vec3 flPos; uniform vec3 flDir; uniform vec3 flCol; uniform float flCos; uniform float flRange;
#define MAXL 16
uniform int nLights; uniform vec4 lPos[MAXL]; uniform vec3 lCol[MAXL];
out vec4 finalColor;
float shadowAt(vec3 n){
  if(shadowOn==0) return 1.0;
  vec4 lp = lightVP*vec4(fragPos + n*0.08, 1.0); vec3 p = lp.xyz/lp.w*0.5 + 0.5;
  if(p.x<0.0||p.y<0.0||p.x>1.0||p.y>1.0||p.z>1.0) return 1.0;
  float bias = 0.0009, s = 0.0;
  for(int x=-1;x<=1;x++) for(int y=-1;y<=1;y++) s += (p.z - bias > texture(shadowMap, p.xy + vec2(x,y)*shadowTexel*1.2).r) ? 0.0 : 1.0;
  float edge = smoothstep(0.0, 0.06, min(min(p.x,p.y), min(1.0-p.x,1.0-p.y)));
  return mix(1.0, s/9.0, edge);
}
float flShadowAt(vec3 n){ // the flashlight's own shadow map: walls, cars and bodies block the beam
  if(flShadowOn==0) return 1.0;
  vec4 lp = flVP*vec4(fragPos + n*0.06, 1.0); if(lp.w <= 0.0) return 1.0; vec3 p = lp.xyz/lp.w*0.5 + 0.5;
  if(p.x<0.0||p.y<0.0||p.x>1.0||p.y>1.0||p.z>1.0) return 1.0;
  float s = 0.0; for(int x=-1;x<=1;x++) for(int y=-1;y<=1;y++) s += (p.z - 0.0006 > texture(flShadowMap, p.xy + vec2(x,y)/1024.0).r) ? 0.0 : 1.0;
  return s/9.0;
}
vec3 aces(vec3 x){ return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14), 0.0, 1.0); }
void main(){
  if(shadowPass==1){ if(texture(texture0, fragTexCoord).a*colDiffuse.a*fragColor.a < 0.5) discard; finalColor = vec4(1.0); return; }
  vec4 base = texture(texture0, fragTexCoord)*colDiffuse*fragColor;
  if(base.a < 0.01) discard;
  vec3 n = normalize(fragNormal); if(!gl_FrontFacing) n = -n;
  vec3 V = normalize(viewPos - fragPos);
  float rough = 0.85, ao = 1.0;
  if(texMode==5){ // terrain: 12 photo layers from an atlas, weighted by 3 splat maps + baked AO
    vec2 suv = fragPos.xz/3200.0; vec4 s0 = texture(texture4, suv), s1 = texture(texture5, suv), s2 = texture(texture6, suv);
    float w[12]; w[1]=s0.r; w[2]=s0.g; w[3]=s0.b; w[4]=s0.a; w[5]=s1.r; w[6]=s1.g; w[7]=s1.b; w[8]=s1.a; w[9]=s2.r; w[10]=s2.g; w[11]=s2.b;
    float sum = 0.0; for(int i=1;i<12;i++) sum += w[i]; w[0] = max(0.0, 1.0 - sum);
    vec2 tuv = fragPos.xz*texScale; vec2 gx = dFdx(tuv)*vec2(512.0/2176.0, 512.0/1632.0), gy = dFdy(tuv)*vec2(512.0/2176.0, 512.0/1632.0);
    vec3 c = vec3(0.0), tn = vec3(0.0); float tot = 0.0;
    for(int i=0;i<12;i++){ if(w[i] < 0.03) continue;
      vec2 cell = vec2(float(i - (i/4)*4), float(i/4)); vec2 a = (cell*544.0 + 16.0 + fract(tuv)*512.0)/vec2(2176.0, 1632.0);
      vec3 ci = textureGrad(texture2, a, gx, gy).rgb, mi = layerMean[i]; // boost what detail survives the distance
      c += w[i]*clamp(mi + (ci - mi)*2.0, 0.0, 1.0); tn += w[i]*(textureGrad(texture3, a, gx, gy).xyz*2.0 - 1.0); tot += w[i]; }
    c /= max(tot, 1e-3); tn /= max(tot, 1e-3);
    float d = texture(texture1, fragPos.xz*0.031).r, d2 = texture(texture1, fragPos.xz*0.0061 + 0.37).r;
    c *= (0.62 + 0.55*d)*(0.75 + 0.5*d2);                              // break up the tiling with two scales of noise
    base.rgb = c*0.72*(1.0 - s2.a*0.75);
    n = normalize(vec3(tn.x, max(tn.z, 0.3), -tn.y));
    rough = 0.9;
  } else if(useDetail==1){ float d = texture(texture1, fragPos.xz*0.18).r*0.6 + texture(texture1, fragPos.xz*0.031).r*0.4; base.rgb *= 0.72 + 0.56*d; }
  if(texMode>=2 && texMode<=4){
    vec2 uv; vec3 T;
    if(texMode==2){ if(abs(n.y)>0.7){ uv = fragPos.xz; T = vec3(1,0,0); } else if(abs(n.x)>abs(n.z)){ uv = vec2(fragPos.z, -fragPos.y); T = vec3(0,0,n.x>0.0?1.0:-1.0); } else { uv = vec2(fragPos.x, -fragPos.y); T = vec3(n.z>0.0?-1.0:1.0,0,0); } }
    else if(texMode==3){ uv = vec2(fragPos.x, (fragPos.z+fragPos.y)*1.2); T = vec3(1,0,0); }
    else { uv = fragPos.xz; T = vec3(1,0,0); }
    uv *= texScale;
    vec4 tc = texture(texture2, uv); if(tc.a < 0.45) discard; base.rgb *= tc.rgb*1.12;
    vec3 tn = texture(texture3, uv).xyz*2.0 - 1.0;
    T = T - n*dot(n,T); T = dot(T,T) > 1e-4 ? normalize(T) : normalize(cross(n, vec3(0,0,1)) + vec3(1e-3,0,0)); vec3 B = cross(n, T);
    if(texMode==4) B = -B;
    n = normalize(T*tn.x + B*tn.y + n*max(tn.z, 0.2));
    rough = texture(texture4, uv).r;
  }
  // height AO: walls darken toward the ground, everything darkens a touch at floor level
  if(abs(n.y) < 0.6) ao = mix(0.55, 1.0, smoothstep(0.0, 1.4, fragPos.y));
  float wetK = wet*clamp(n.y,0.0,1.0)*(texMode==5 ? 0.35 : 1.0);
  float puddle = wetK*smoothstep(0.55, 0.2, rough);
  rough = mix(rough, 0.08, wetK*0.8);
  base.rgb *= 1.0 - 0.3*wetK;
  float gloss = 1.0 - rough, sp = exp2(3.0 + gloss*8.0), specK = 0.04 + gloss*0.5;
  float hemi = 0.5 + 0.5*n.y;
  vec3 light = (ambCol*(0.6+0.4*hemi) + skyCol*hemi*0.25)*ao;
  float sd = max(dot(n,-sunDir),0.0);
  float sh = sd > 0.0 ? shadowAt(normalize(fragNormal)) : 1.0;
  light += sunCol*sd*sh;
  vec3 spec = sunCol*sh*pow(max(dot(n, normalize(-sunDir+V)),0.0), sp)*specK*(1.0+wetK*2.0);
  // flashlight
  vec3 L = flPos - fragPos; float d = length(L); L /= max(d,1e-3);
  float sc = dot(-L, flDir);
  float spot = smoothstep(flCos, flCos + 0.06, sc);
  float fatt = clamp(1.0 - d/flRange, 0.0, 1.0);
  float fsh = (spot > 0.0 || d < 5.0) ? flShadowAt(normalize(fragNormal)) : 1.0;
  float fl = spot*fatt*fatt*(0.35 + 0.65*max(dot(n,L),0.0))*fsh;
  light += flCol*fl;
  light += flCol*0.2*clamp(1.0 - d/5.0, 0.0, 1.0)*fsh;
  spec += flCol*fsh*spot*fatt*pow(max(dot(n, normalize(L+V)),0.0), sp)*specK*(1.0+wetK*3.0);
  for(int i=0;i<nLights;i++){
    vec3 D = lPos[i].xyz - fragPos; float dd = length(D);
    float att = clamp(1.0 - dd/lPos[i].w, 0.0, 1.0); att *= att;
    vec3 Ld = D/max(dd,1e-3);
    light += lCol[i]*att*(0.25 + 0.75*max(dot(n,Ld),0.0));
    // wet streets: long glossy streaks under lamps
    spec += lCol[i]*att*pow(max(dot(n, normalize(Ld+V)),0.0), mix(sp, sp*0.25, wetK))*(specK + wetK*1.6);
  }
  vec3 col = base.rgb*light + spec;
  float fres = pow(1.0 - max(dot(n,V),0.0), 4.0);
  col = mix(col, skyCol*1.3 + fogCol*0.4, clamp(fres*(0.15 + wetK*0.6 + puddle*0.8), 0.0, 0.7));
  float fd = length(fragPos.xz - fogCenter);
  float f = clamp((fd - fogNear)/(fogFar - fogNear), 0.0, 1.0);
  col = mix(col, fogCol, f*f*(3.0-2.0*f));
  col = pow(aces(pow(max(col, vec3(0.0)), vec3(2.2))*exposure), vec3(1.0/2.2)); // filmic curve in linear space
  float lum = dot(col, vec3(0.299,0.587,0.114));
  col = mix(vec3(lum), col, 0.86);                                   // a little drained
  col *= mix(vec3(0.92,0.98,1.06), vec3(1.05,1.0,0.92), smoothstep(0.1,0.7,lum)); // cool shadows, warm highlights
  finalColor = vec4(col, base.a);
})";

// ============================================================ render resources
enum MeshId { M_CUBE, M_SPHERE, M_CYL, M_CONE, M_GABLE, M_COUNT };
static Mesh meshes[M_COUNT];
static Mesh groundMesh;
static Material litMat, groundMat;
static Shader lit, litSkin;
enum { uSunDir, uSunCol, uAmb, uSky, uFogCol, uFogC, uFogN, uFogF, uView, uWet, uDetail, uFlPos, uFlDir, uFlCol, uFlCos, uFlRange, uNL, uLPos, uLCol, uTexMode, uTexScale, uShadowOn, uShadowTexel, uShadowPass, uLightVP, uShadowMap, uExposure, uFlVP, uFlShadowMap, uFlShadowOn, U_COUNT };
static const char* UNAMES[U_COUNT] = {"sunDir", "sunCol", "ambCol", "skyCol", "fogCol", "fogCenter", "fogNear", "fogFar", "viewPos", "wet", "useDetail", "flPos", "flDir", "flCol", "flCos", "flRange", "nLights", "lPos", "lCol", "texMode", "texScale", "shadowOn", "shadowTexel", "shadowPass", "lightVP", "shadowMap", "exposure", "flVP", "flShadowMap", "flShadowOn"};
static int ULOC[2][U_COUNT];
static void setU(int u, const void* v, int type, int count = 1) { Shader* sh[2] = {&lit, &litSkin}; for (int k = 0; k < 2; k++) if (ULOC[k][u] >= 0) { if (count > 1) SetShaderValueV(*sh[k], ULOC[k][u], v, type, count); else SetShaderValue(*sh[k], ULOC[k][u], v, type); } }
static void setTexMode(int m, float sc = .5f) { static int last = -1; static float lastS = -1; if (m == last && sc == lastS) return; rlDrawRenderBatchActive(); last = m; lastS = sc; SetShaderValue(lit, ULOC[0][uTexMode], &m, SHADER_UNIFORM_INT); SetShaderValue(lit, ULOC[0][uTexScale], &sc, SHADER_UNIFORM_FLOAT); }
static Texture2D groundTex, detailTex, bloodTex, softTex, vignTex, grainTex, mapTex;
static Font font;
static Camera3D cam3{};
static int W = 1600, H = 900;

static Mesh buildMesh(const std::vector<Vector3>& tri, const std::vector<Vector2>* uv = nullptr) {
    Mesh m{}; m.vertexCount = (int)tri.size(); m.triangleCount = m.vertexCount / 3;
    m.vertices = (float*)MemAlloc(sizeof(float) * 3 * m.vertexCount);
    m.normals = (float*)MemAlloc(sizeof(float) * 3 * m.vertexCount);
    m.texcoords = (float*)MemAlloc(sizeof(float) * 2 * m.vertexCount);
    for (int i = 0; i < m.vertexCount; i += 3) {
        Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(tri[i + 1], tri[i]), Vector3Subtract(tri[i + 2], tri[i])));
        for (int k = 0; k < 3; k++) {
            const Vector3& v = tri[i + k];
            m.vertices[(i + k) * 3] = v.x; m.vertices[(i + k) * 3 + 1] = v.y; m.vertices[(i + k) * 3 + 2] = v.z;
            m.normals[(i + k) * 3] = n.x; m.normals[(i + k) * 3 + 1] = n.y; m.normals[(i + k) * 3 + 2] = n.z;
            Vector2 t = uv ? (*uv)[i + k] : V2(v.x + .5f, v.z + .5f);
            m.texcoords[(i + k) * 2] = t.x; m.texcoords[(i + k) * 2 + 1] = t.y;
        }
    }
    UploadMesh(&m, false); return m;
}
static Mesh gableMesh() { // base [-.5,.5]^2 at y=0, ridge along x at y=1, z=0. CCW from outside.
    Vector3 a = V3(-.5f, 0, -.5f), b = V3(.5f, 0, -.5f), c = V3(.5f, 0, .5f), d = V3(-.5f, 0, .5f), r0 = V3(-.5f, 1, 0), r1 = V3(.5f, 1, 0);
    std::vector<Vector3> t = {a, r0, r1, a, r1, b, d, c, r1, d, r1, r0, a, d, r0, b, r1, c};
    return buildMesh(t);
}
static void initRender() {
    meshes[M_CUBE] = GenMeshCube(1, 1, 1); meshes[M_SPHERE] = GenMeshSphere(.5f, 8, 10);
    meshes[M_CYL] = GenMeshCylinder(.5f, 1, 10); meshes[M_CONE] = GenMeshCone(.5f, 1, 8); meshes[M_GABLE] = gableMesh();
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
    litSkin.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(litSkin, "matModel"); litSkin.locs[SHADER_LOC_MATRIX_NORMAL] = GetShaderLocation(litSkin, "matNormal");
    litSkin.locs[SHADER_LOC_BONE_MATRICES] = GetShaderLocation(litSkin, "boneMatrices");
    litSkin.locs[SHADER_LOC_VERTEX_BONEIDS] = GetShaderLocationAttrib(litSkin, "vertexBoneIds"); litSkin.locs[SHADER_LOC_VERTEX_BONEWEIGHTS] = GetShaderLocationAttrib(litSkin, "vertexBoneWeights");
    for (int u = 0; u < U_COUNT; u++) { ULOC[0][u] = GetShaderLocation(lit, UNAMES[u]); ULOC[1][u] = GetShaderLocation(litSkin, UNAMES[u]); }
    { int z = 0; setU(uTexMode, &z, SHADER_UNIFORM_INT); setU(uDetail, &z, SHADER_UNIFORM_INT); setU(uShadowOn, &z, SHADER_UNIFORM_INT); setU(uShadowPass, &z, SHADER_UNIFORM_INT); int slot = 11; setU(uShadowMap, &slot, SHADER_UNIFORM_INT); int slot2 = 12; setU(uFlShadowMap, &slot2, SHADER_UNIFORM_INT); setU(uFlShadowOn, &z, SHADER_UNIFORM_INT); float ex = 1.5f; setU(uExposure, &ex, SHADER_UNIFORM_FLOAT); float ts = .5f; setU(uTexScale, &ts, SHADER_UNIFORM_FLOAT); }
    litMat = LoadMaterialDefault(); litMat.shader = lit;
    // detail noise
    Image dn = GenImagePerlinNoise(256, 256, 0, 0, 5.f); detailTex = LoadTextureFromImage(dn); UnloadImage(dn);
    SetTextureWrap(detailTex, TEXTURE_WRAP_REPEAT); SetTextureFilter(detailTex, TEXTURE_FILTER_BILINEAR); GenTextureMipmaps(&detailTex);
    // blood atlas: 4 splat variants in 256x64
    Image bi = GenImageColor(256, 64, BLANK);
    for (int v = 0; v < 4; v++) {
        std::vector<Vector3> blobs; int nb = 5 + v * 3;
        for (int i = 0; i < nb; i++) blobs.push_back(V3(32 + frand(-14, 14) * (i ? 1.f : 0.f), 32 + frand(-14, 14) * (i ? 1.f : 0.f), i ? frand(3, 9) : frand(12, 17)));
        for (int i = 0; i < 6 + v; i++) { float a = frand(0, 2 * PI); for (int k = 0; k < 6; k++) blobs.push_back(V3(32 + cosf(a) * (14 + k * 2.6f), 32 + sinf(a) * (14 + k * 2.6f), 2.6f - k * .35f)); }
        for (int y = 0; y < 64; y++) for (int x = 0; x < 64; x++) {
            float f = 0; for (auto& b : blobs) { float dx = x - b.x, dy = y - b.y; f += b.z * b.z / (dx * dx + dy * dy + 1); }
            float a = Clamp((f - .9f) * 3.f, 0, 1); if (a > 0) ImageDrawPixel(&bi, v * 64 + x, y, rgba(255, 255, 255, (int)(a * 255 * (.75f + .25f * frand(0, 1)))));
        }
    }
    bloodTex = LoadTextureFromImage(bi); UnloadImage(bi); SetTextureFilter(bloodTex, TEXTURE_FILTER_BILINEAR);
    Image si = GenImageGradientRadial(64, 64, 0.f, WHITE, BLANK); softTex = LoadTextureFromImage(si); UnloadImage(si); SetTextureFilter(softTex, TEXTURE_FILTER_BILINEAR);
    Image vi = GenImageGradientRadial(512, 512, .35f, rgba(0, 0, 0, 0), rgba(0, 0, 0, 255)); vignTex = LoadTextureFromImage(vi); UnloadImage(vi); SetTextureFilter(vignTex, TEXTURE_FILTER_BILINEAR);
    Image gi = GenImageWhiteNoise(256, 256, .5f); grainTex = LoadTextureFromImage(gi); UnloadImage(gi); SetTextureWrap(grainTex, TEXTURE_WRAP_REPEAT);
}
static Matrix TRS(Vector3 p, Vector3 s, float yaw) { return MatrixMultiply(MatrixMultiply(MatrixScale(s.x, s.y, s.z), MatrixRotateY(-yaw)), MatrixTranslate(p.x, p.y, p.z)); }
static void drawMeshM(int m, const Matrix& M, Color c) { litMat.maps[MATERIAL_MAP_DIFFUSE].color = c; DrawMesh(meshes[m], litMat, M); }
static void drawBox(Vector3 center, Vector3 size, float yaw, Color c) { drawMeshM(M_CUBE, TRS(center, size, yaw), c); }

// ============================================================ surface textures
// Photo PBR sets from ambientCG (CC0) where we have them; procedural height maps (with derived normals) for the rest.
enum TexId { TX_NONE, TX_SIDING, TX_BRICK, TX_PLASTER, TX_SHINGLE, TX_PLANK, TX_TILE, TX_LOG, TX_METAL, TX_ASPHALT, TX_ASPHALT_WET, TX_CONCRETE, TX_HTILE, TX_SMTILE, TX_ROOFTILE, TX_CABIN, TX_HARDWOOD, TX_BARNWOOD, TX_PLYWOOD, TX_CHAIN, TX_COUNT };
static Material surfMat[TX_COUNT]; static float surfScale[TX_COUNT]; static bool surfPhoto[TX_COUNT];
static unsigned hsh(int a, int b) { unsigned h = (unsigned)a * 374761393u + (unsigned)b * 668265263u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16); }
static float hf(int a, int b) { return (hsh(a, b) & 0xffff) / 65535.f; }
static Texture2D texFromImage(Image im, bool srgbColor) { (void)srgbColor; Texture2D t = LoadTextureFromImage(im); GenTextureMipmaps(&t); SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR); SetTextureWrap(t, TEXTURE_WRAP_REPEAT); return t; }
static Image genSurfHeight(int kind) {
    const int N = 512; Image im = GenImageColor(N, N, WHITE); Color* px = (Color*)im.data;
    for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
        float u = x * 2.f / N, v = y * 2.f / N, val = 1, g = frand(-.035f, .035f);
        switch (kind) {
        case TX_SIDING: { float bh = .2f; int row = (int)(v / bh); float fv = fmodf(v, bh) / bh; val = .78f + .22f * fv + (hf(row, 3) - .5f) * .06f; if (fv < .07f) val = .5f; break; }
        case TX_SHINGLE: { float rh = .22f, tw = .32f; int row = (int)(v / rh); float off = hf(row, 1) * tw; int col = (int)((u + off) / tw); float fv = fmodf(v, rh) / rh, tu = fmodf(u + off, tw);
            val = (.62f + .38f * fv) * (.82f + hf(row, col) * .3f); if (tu < .012f) val *= .6f; if (fv < .06f) val *= .55f; break; }
        case TX_PLANK: { float pw = .16f; int row = (int)(v / pw); float len = .9f + hf(row, 9) * 1.1f, off = hf(row, 2) * len; int seg = (int)((u + off) / len); float fv = fmodf(v, pw) / pw, fu = fmodf(u + off, len);
            val = .72f + hf(row, seg) * .3f + sinf(u * 40 + hf(row, seg) * 9) * .03f; if (fv < .05f || fu < .008f) val *= .55f; break; }
        case TX_LOG: { float lh = .26f; int row = (int)(v / lh); float fv = fmodf(v, lh) / lh; val = .55f + .45f * sinf(fv * PI) + (hf(row, 5) - .5f) * .1f; break; }
        }
        float c = Clamp(val + g, 0, 1) * 255; px[y * N + x] = rgba((int)c, (int)c, (int)c);
    }
    return im;
}
static Image normalFromHeight(const Image& h, float strength) {
    int N = h.width; Image out = GenImageColor(N, N, BLANK); const Color* hp = (const Color*)h.data; Color* op = (Color*)out.data;
    auto H = [&](int x, int y) { return hp[((y + N) % N) * N + ((x + N) % N)].r / 255.f; };
    for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
        float dx = (H(x + 1, y) - H(x - 1, y)) * strength, dy = (H(x, y + 1) - H(x, y - 1)) * strength;
        Vector3 n = Vector3Normalize(V3(-dx, dy, 1)); op[y * N + x] = rgba((int)((n.x * .5f + .5f) * 255), (int)((n.y * .5f + .5f) * 255), (int)((n.z * .5f + .5f) * 255));
    }
    return out;
}
static std::string texDir();
static void initSurfaces() {
    static const char* photo[TX_COUNT] = {nullptr, nullptr, "wall_brick", "wall_plaster_dirty", nullptr, nullptr, "hospital_floor", nullptr, "metal_rusty_paint", "street_asphalt", "street_asphalt_wet", "sidewalk_concrete", "hospital_wall_tiles", "hospital_small_tiles",
        "roof_tiles", "cabin_logs", "hardwood_floor", "barn_wood", "plywood", "chainlink"};
    static const float scale[TX_COUNT] = {0, .5f, .5f, .33f, .5f, .5f, .4f, .5f, .4f, .4f, .4f, .35f, .5f, .6f, .45f, .35f, .4f, .35f, .5f, .5f};
    static const float roughP[TX_COUNT] = {0, .7f, 0, 0, .8f, .55f, 0, .85f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::string dir = texDir();
    for (int k = 1; k < TX_COUNT; k++) {
        surfMat[k] = LoadMaterialDefault(); surfMat[k].shader = lit; surfScale[k] = scale[k];
        std::string base = photo[k] ? dir + photo[k] : "";
        std::string colF = base + "_Color.jpg"; if (photo[k] && !FileExists(colF.c_str())) colF = base + "_Color.png";
        if (photo[k] && FileExists(colF.c_str())) {
            surfPhoto[k] = true;
            surfMat[k].maps[MATERIAL_MAP_NORMAL].texture = LoadTexture(colF.c_str());
            surfMat[k].maps[MATERIAL_MAP_ROUGHNESS].texture = LoadTexture((base + "_Normal.jpg").c_str());
            surfMat[k].maps[MATERIAL_MAP_OCCLUSION].texture = LoadTexture((base + "_Roughness.jpg").c_str());
            for (int m : {MATERIAL_MAP_NORMAL, MATERIAL_MAP_ROUGHNESS, MATERIAL_MAP_OCCLUSION}) { Texture2D& t = surfMat[k].maps[m].texture; GenTextureMipmaps(&t); SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR); SetTextureWrap(t, TEXTURE_WRAP_REPEAT); }
            continue;
        }
        // procedural fallback (or the four we never had photos for)
        int kind = k == TX_ROOFTILE ? TX_SHINGLE : k == TX_CABIN ? TX_LOG : k == TX_HARDWOOD || k == TX_BARNWOOD || k == TX_PLYWOOD ? TX_PLANK : k == TX_CHAIN ? TX_METAL : (k == TX_BRICK || k == TX_PLASTER || k == TX_TILE || k == TX_METAL || k >= TX_ASPHALT) ? TX_PLANK : k;
        Image h = genSurfHeight(kind); Image nm = normalFromHeight(h, kind == TX_LOG ? 6.f : 9.f);
        Image r = GenImageColor(8, 8, rgba((int)(roughP[k] > 0 ? roughP[k] * 255 : 200), 0, 0));
        surfMat[k].maps[MATERIAL_MAP_NORMAL].texture = texFromImage(h, true); surfMat[k].maps[MATERIAL_MAP_ROUGHNESS].texture = texFromImage(nm, false); surfMat[k].maps[MATERIAL_MAP_OCCLUSION].texture = texFromImage(r, false);
        UnloadImage(h); UnloadImage(nm); UnloadImage(r);
    }
}
static Texture2D groundAtlas[2], splatTex[3]; static bool groundAtlasOk = false;
static void initGroundAtlas() { // 12 layers, 512px tiles with 16px wrapped gutters so mipmaps don't bleed across tiles
    const int T = 512, G = 16, C = T + 2 * G, AW = C * 4, AH = C * 3; std::string dir = texDir() + "ground/";
    for (int kind = 0; kind < 2; kind++) {
        Image at = GenImageColor(AW, AH, kind ? rgba(128, 128, 255) : GRAY); Color* ap = (Color*)at.data;
        for (int i = 0; i < 12; i++) { char fn[64]; snprintf(fn, 64, "L%02d_%s.jpg", i, kind ? "Normal" : "Color"); std::string f = dir + fn;
            if (!FileExists(f.c_str())) { UnloadImage(at); return; }
            Image im = LoadImage(f.c_str()); ImageFormat(&im, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8); if (im.width != T) ImageResize(&im, T, T); Color* p = (Color*)im.data;
            int ox = (i % 4) * C, oy = (i / 4) * C;
            for (int y = 0; y < C; y++) for (int x = 0; x < C; x++) ap[(oy + y) * AW + ox + x] = p[((y - G + T) % T) * T + ((x - G + T) % T)];
            if (!kind) { double r = 0, g = 0, b = 0; for (int k = 0; k < T * T; k++) { r += p[k].r; g += p[k].g; b += p[k].b; } float m[3] = {(float)(r / T / T / 255), (float)(g / T / T / 255), (float)(b / T / T / 255)}; SetShaderValue(lit, GetShaderLocation(lit, TextFormat("layerMean[%d]", i)), m, SHADER_UNIFORM_VEC3); }
            UnloadImage(im); }
        groundAtlas[kind] = LoadTextureFromImage(at); UnloadImage(at); GenTextureMipmaps(&groundAtlas[kind]); SetTextureFilter(groundAtlas[kind], TEXTURE_FILTER_TRILINEAR); SetTextureWrap(groundAtlas[kind], TEXTURE_WRAP_CLAMP);
    }
    groundAtlasOk = true;
}
static int texModeFor(int tex, int mesh) { if (tex == TX_NONE) return 0; if (mesh == M_GABLE || tex == TX_SHINGLE || tex == TX_ROOFTILE) return 3; if (tex == TX_PLANK || tex == TX_TILE || tex == TX_SMTILE || tex == TX_ASPHALT || tex == TX_ASPHALT_WET || tex == TX_CONCRETE || tex == TX_HARDWOOD) return 4; return 2; }
static void drawTexMesh(int mesh, const Matrix& M, Color c, int tex) {
    if (tex == TX_NONE) { setTexMode(0); drawMeshM(mesh, M, c); return; }
    // photo textures carry their own color; the prop tint only nudges them
    if (surfPhoto[tex]) { float k = tex == TX_ASPHALT || tex == TX_ASPHALT_WET ? .62f : 1.f; c = rgba((int)((200 + (c.r - 128) / 4) * k), (int)((200 + (c.g - 128) / 4) * k), (int)((200 + (c.b - 128) / 4) * k), c.a); }
    setTexMode(texModeFor(tex, mesh), surfScale[tex]); surfMat[tex].maps[MATERIAL_MAP_DIFFUSE].color = c; DrawMesh(meshes[mesh], surfMat[tex], M);
}

// ============================================================ glTF assets (Quaternius, CC0)
static std::string assetDir;
struct Asset { Model m{}; BoundingBox bb{}; Vector3 size{}; std::vector<std::string> matName; std::vector<Color> base; bool skinned = false; Rig rig; ModelAnimation anim{}; Transform* fp = nullptr; Image tex{}; };
static std::vector<Asset> assets; static std::map<std::string, int> assetIdx;
static std::vector<std::string> glbMaterialNames(const std::string& path) {
    std::vector<std::string> out; std::ifstream f(path, std::ios::binary); if (!f) return out;
    char hdr[20]; f.read(hdr, 20); unsigned len; memcpy(&len, hdr + 12, 4); std::string j(len, ' '); f.read(&j[0], len);
    size_t m = j.find("\"materials\""); if (m == std::string::npos) return out; size_t a = j.find('[', m); int depth = 0; size_t e = a;
    for (; e < j.size(); e++) { if (j[e] == '[' || j[e] == '{') depth++; else if (j[e] == ']' || j[e] == '}') { depth--; if (depth == 0) break; } }
    std::string arr = j.substr(a, e - a); int d = 0;
    for (size_t i = 0; i < arr.size(); i++) { char ch = arr[i]; if (ch == '{' || ch == '[') d++; else if (ch == '}' || ch == ']') d--;
        else if (d == 2 && arr.compare(i, 7, "\"name\":") == 0) { size_t q0 = arr.find('"', i + 7), q1 = arr.find('"', q0 + 1); out.push_back(arr.substr(q0 + 1, q1 - q0 - 1)); i = q1; } }
    return out;
}
static int loadAsset(const std::string& key) {
    std::string path = assetDir + key + ".glb"; if (!FileExists(path.c_str())) return -1;
    Asset a; a.m = LoadModel(path.c_str()); if (a.m.meshCount == 0) return -1;
    std::vector<std::string> names = glbMaterialNames(path); a.matName.assign(a.m.materialCount, "");
    for (int i = 1; i < a.m.materialCount && i - 1 < (int)names.size(); i++) a.matName[i] = names[i - 1];
    for (int i = 0; i < a.m.meshCount; i++) if (a.m.meshes[i].boneIds) a.skinned = true;
    for (int i = 0; i < a.m.materialCount; i++) {
        Color& c = a.m.materials[i].maps[MATERIAL_MAP_DIFFUSE].color;
        c = rgba((int)(255 * powf(c.r / 255.f, 1 / 2.2f)), (int)(255 * powf(c.g / 255.f, 1 / 2.2f)), (int)(255 * powf(c.b / 255.f, 1 / 2.2f)), c.a);
        a.base.push_back(c); a.m.materials[i].shader = a.skinned ? litSkin : lit;
    }
    if (a.skinned) { a.rig = makeRig(a.m); a.fp = (Transform*)MemAlloc(sizeof(Transform) * a.m.boneCount); a.anim.boneCount = a.m.boneCount; a.anim.frameCount = 1; a.anim.bones = a.m.bones; }
    a.bb = GetModelBoundingBox(a.m); a.size = Vector3Subtract(a.bb.max, a.bb.min);
    for (int i = 0; i < a.m.materialCount; i++) { Texture2D t = a.m.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture; if (t.id && t.id != rlGetTextureIdDefault() && t.width > 4) { a.tex = LoadImageFromTexture(t); ImageFormat(&a.tex, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8); break; } } // palette textures get baked into vertex colors
    assets.push_back(a); int idx = (int)assets.size() - 1; if (assets[idx].skinned) assets[idx].anim.framePoses = &assets[idx].fp;
    assetIdx[key] = idx; return idx;
}
static int AS(const std::string& key) { auto it = assetIdx.find(key); return it == assetIdx.end() ? -1 : it->second; }
static bool haveChars = false;
static const char* CHAR_KEYS[] = {"chars/M_Casual", "chars/M_Casual2", "chars/M_Farmer", "chars/M_Punk", "chars/M_Suit", "chars/M_Worker", "chars/M_Beach", "chars/M_Adventurer", "chars/M_Swat",
    "chars/F_Casual", "chars/F_Punk", "chars/F_Suit", "chars/F_Worker", "chars/F_Formal", "chars/F_Adventurer", "chars/F_Soldier"};
static void findAssetDir() {
    std::vector<std::string> c = {std::string(GetApplicationDirectory()) + "assets/", std::string(GetApplicationDirectory()) + "../assets/", "assets/", "../assets/"};
    for (auto& d : c) if (FileExists((d + "chars/M_Casual.glb").c_str())) { assetDir = d; return; }
}
static std::string texDir() { if (assetDir.empty()) findAssetDir(); return assetDir.empty() ? std::string("assets/textures/") : assetDir + "textures/"; }
static void loadAllAssets() {
    assets.reserve(256); findAssetDir(); if (assetDir.empty()) { TraceLog(LOG_WARNING, "NIGHTFALL: assets folder not found, using blocky fallback models"); return; }
    for (const char* k : CHAR_KEYS) loadAsset(k);
    haveChars = AS("chars/M_Casual") >= 0;
    const char* rest[] = {"nature/PineTree_1", "nature/PineTree_2", "nature/PineTree_3", "nature/PineTree_4", "nature/PineTree_5", "nature/CommonTree_1", "nature/CommonTree_2", "nature/CommonTree_3", "nature/CommonTree_4", "nature/CommonTree_5",
        "nature/BirchTree_1", "nature/BirchTree_2", "nature/BirchTree_3", "nature/CommonTree_Dead_1", "nature/CommonTree_Dead_2", "nature/PineTree_Autumn_1", "nature/CommonTree_Autumn_2", "nature/Bush_1", "nature/Bush_2", "nature/BushBerries_1",
        "nature/Rock_1", "nature/Rock_2", "nature/Rock_3", "nature/Rock_4", "nature/Rock_Moss_1", "nature/Rock_Moss_2", "nature/Rock_Moss_3", "nature/TreeStump", "nature/WoodLog", "nature/WoodLog_Moss", "nature/Grass", "nature/Grass_2", "nature/Plant_1",
        "interior/Bed_Single", "interior/Bed_King", "interior/Bed_Bunk", "interior/Couch_Medium1", "interior/Couch_Small1", "interior/Table_RoundSmall", "interior/Chair_1", "interior/Kitchen_Fridge", "interior/Kitchen_Oven", "interior/Kitchen_2Drawers",
        "interior/Kitchen_Sink", "interior/Bookshelf", "interior/Drawer_1", "interior/Shelf_Large", "interior/Shelf_Small1", "interior/Bathroom_Toilet", "interior/Trashcan_Large", "interior/NightStand_1", "interior/Houseplant_1", "interior/Carpet_1",
        "items/Pistol_1", "items/Pistol_2", "items/Revolver_1", "items/Shotgun_1", "items/Shotgun_2", "items/Shotgun_ShortStock", "items/Knife", "items/Axe", "items/Bandages", "items/FirstAidKit", "items/WaterBottle_1", "items/Can_Closed", "items/Can_Red",
        "items/Battery_Small", "items/PropaneTank", "items/Tent", "items/Backpack", "items/GasCan", "items/Trashcan", "items/Bonfire_Fire", "items/FlareGun", "items/Pot",
        "street/Streetlight_Single", "street/Streetlight_Double", "street/TrafficLight", "street/Sign_Stop", "street/Sign_NoParking",
        "vehicles/sedan", "vehicles/sedan-sports", "vehicles/hatchback-sports", "vehicles/suv", "vehicles/suv-luxury", "vehicles/van", "vehicles/taxi", "vehicles/police", "vehicles/ambulance", "vehicles/firetruck",
        "vehicles/delivery", "vehicles/delivery-flat", "vehicles/truck", "vehicles/truck-flat", "vehicles/garbage-truck", "vehicles/tractor", "vehicles/tractor-shovel",
        "vehicles/debris-door", "vehicles/debris-tire", "vehicles/debris-bumper", "vehicles/debris-plate-a", "vehicles/debris-drivetrain-axle"};
    for (const char* k : rest) loadAsset(k);
    for (auto& a : assets) if (a.skinned) a.anim.framePoses = &a.fp;
    TraceLog(LOG_INFO, "NIGHTFALL: loaded %d models from %s", (int)assets.size(), assetDir.c_str());
}
// draw a static (unskinned) asset with optional tint and alpha
static void drawAsset(int ai, const Matrix& M, float alpha = 1, Color tint = WHITE, float darken = 1) {
    if (ai < 0) return; Asset& a = assets[ai];
    for (int i = 0; i < a.m.meshCount; i++) { int mi = a.m.meshMaterial[i]; Color c = a.base[mi]; c = rgba(c.r * tint.r / 255 * darken, c.g * tint.g / 255 * darken, c.b * tint.b / 255 * darken, (int)(255 * alpha));
        a.m.materials[mi].maps[MATERIAL_MAP_DIFFUSE].color = c; DrawMesh(a.m.meshes[i], a.m.materials[mi], MatrixMultiply(a.m.transform, M)); }
}
// fit helpers: scale so that the given axis length equals target
static float fitScale(int ai, float target, int axis = -1) { if (ai < 0) return 1; Vector3 s = assets[ai].size; float L = axis == 0 ? s.x : axis == 1 ? s.y : axis == 2 ? s.z : std::max({s.x, s.y, s.z}); return L > 1e-4f ? target / L : 1; }
// Model matrix that places the asset's bbox-center (xz) and bottom at pos, scaled & yawed
static Matrix placeM(int ai, Vector3 pos, float sc, float yaw, bool bottom = true) {
    Asset& a = assets[ai]; Vector3 c = Vector3Scale(Vector3Add(a.bb.min, a.bb.max), .5f);
    Matrix M = MatrixMultiply(MatrixMultiply(MatrixTranslate(-c.x, bottom ? -a.bb.min.y : -c.y, -c.z), MatrixScale(sc, sc, sc)), MatrixRotateY(-yaw));
    return MatrixMultiply(M, MatrixTranslate(pos.x, pos.y, pos.z));
}

// ------------------------------------------------------------ character looks (survivor / zombie palettes)
struct Look { int asset; std::vector<Color> col, col0; std::vector<int> zone; Vector3 scale; bool female; }; // zone per material: 0 skin/other, 1 torso, 2 legs, 3 feet
static std::vector<Look> looks; static std::vector<int> lookPool[7]; static int playerLooks[2] = {-1, -1};
static bool hasStr(const std::string& s, const char* k) { return s.find(k) != std::string::npos; }
static Look makeLook(int ai, int kind) { // kind: 0 survivor, 1 zombie, 2 volatile, 3 bloater, 4 brute, 5 soldier zombie, 6 runner
    Asset& a = assets[ai]; Look L; L.asset = ai; L.col = a.base; float h = 1.8f / std::max(.5f, a.size.y); L.female = hasStr(CHAR_KEYS[0], "") && a.rig.names.size() > 49;
    L.scale = V3(h, h, h);
    if (kind == 0) { // classify clothing materials by where their vertices sit on the body so worn clothes can recolor them
        L.zone.assign(L.col.size(), 0); std::vector<double> sy(L.col.size(), 0); std::vector<int> cnt(L.col.size(), 0); float y0 = a.bb.min.y, hy = std::max(.01f, a.bb.max.y - a.bb.min.y);
        for (int i = 0; i < a.m.meshCount; i++) { int mi = a.m.meshMaterial[i]; if (mi < 0 || mi >= (int)sy.size()) continue; Mesh& m = a.m.meshes[i]; for (int v = 0; v < m.vertexCount; v += 3) { sy[mi] += (m.vertices[v * 3 + 1] - y0) / hy; cnt[mi]++; } }
        for (int i = 0; i < (int)L.col.size(); i++) { const std::string& n = a.matName[i]; if (!cnt[i] || hasStr(n, "Skin") || hasStr(n, "Eye") || hasStr(n, "Hair") || hasStr(n, "Eyebrow") || hasStr(n, "Moustache")) continue;
            float my = (float)(sy[i] / cnt[i]); L.zone[i] = my > .55f && my < .86f ? 1 : my > .14f && my <= .55f ? 2 : my <= .14f ? 3 : 0; }
        L.col0 = L.col; return L; }
    static const std::vector<Color> skins = {rgba(132, 142, 108), rgba(150, 144, 134), rgba(120, 124, 98), rgba(158, 146, 140), rgba(140, 150, 120), rgba(118, 112, 110)};
    Color skin = kind == 2 ? rgba(226, 228, 232) : kind == 3 ? rgba(150, 176, 84) : kind == 4 ? rgba(124, 108, 118) : pick(skins);
    std::vector<int> cloth;
    for (int i = 0; i < (int)L.col.size(); i++) {
        const std::string& n = a.matName[i]; Color& c = L.col[i];
        if (hasStr(n, "Skin")) c = mulC(skin, hasStr(n, "Darker") ? .9f : 1);
        else if (n == "Eye") c = kind == 2 ? rgba(255, 60, 30) : rgba(210, 206, 160);
        else if (hasStr(n, "Hair") || hasStr(n, "Eyebrows") || hasStr(n, "Moustache")) c = mulC(c, kind == 2 ? .2f : .65f);
        else if (!n.empty()) { c = mulC(lerpC(c, rgba(72, 56, 42), kind == 2 ? .5f : .35f), kind == 2 ? .45f : .78f); cloth.push_back(i); }
    }
    if (!cloth.empty()) { int k = pick(cloth); L.col[k] = lerpC(L.col[k], rgba(96, 16, 16), .55f); }
    if (cloth.size() > 2 && chance(.5f)) { int k = pick(cloth); L.col[k] = lerpC(L.col[k], rgba(70, 14, 14), .4f); }
    float j = frand(.94f, 1.06f);
    if (kind == 3) L.scale = V3(h * 1.38f, h * 1.02f, h * 1.42f);
    else if (kind == 4) L.scale = V3(h * 1.34f, h * 1.28f, h * 1.34f);
    else L.scale = V3(h * j, h * j, h * j);
    return L;
}
static void buildLooks() {
    looks.clear(); for (auto& p : lookPool) p.clear(); if (!haveChars) return;
    std::vector<int> civ, sold;
    for (const char* k : CHAR_KEYS) { int ai = AS(k); if (ai < 0) continue; if (hasStr(k, "Swat") || hasStr(k, "Soldier")) sold.push_back(ai); else civ.push_back(ai); }
    auto add = [&](int pool, int ai, int kind) { looks.push_back(makeLook(ai, kind)); lookPool[pool].push_back((int)looks.size() - 1); };
    for (int r = 0; r < 3; r++) for (int ai : civ) add(0 /*walker*/, ai, 1);
    for (int ai : civ) add(1 /*runner*/, ai, 1);
    for (int r = 0; r < 2; r++) add(2 /*brute*/, pick(civ), 4);
    for (int r = 0; r < 3; r++) add(3 /*bloater*/, pick(civ), 3);
    for (int r = 0; r < 4; r++) add(4 /*volatile*/, pick(civ), 2);
    for (int r = 0; r < 3; r++) for (int ai : sold) add(5 /*soldier*/, ai, 1);
    int pm = AS("chars/M_Adventurer"), pf = AS("chars/F_Adventurer");
    if (pm >= 0) { looks.push_back(makeLook(pm, 0)); playerLooks[0] = (int)looks.size() - 1; }
    if (pf >= 0) { looks.push_back(makeLook(pf, 0)); playerLooks[1] = (int)looks.size() - 1; }
}
struct CharOut { Vector3 hand[2], head, chest; Matrix M; }; static float charY = 0;
static int outfitLook(); // applies worn clothes to the player look
static void drawCharacter(int lookIdx, const PoseIn& pin, Vector2 pos, float yaw, Color tint, float tintAmt, bool lying, CharOut* out = nullptr, float alpha = 1) {
    Look& L = looks[lookIdx]; Asset& a = assets[L.asset];
    std::vector<Quaternion> D; std::vector<float> S; Vector3 ro = buildPose(a.rig, pin, D, S); solvePose(a.rig, D, S, ro, a.fp); UpdateModelAnimationBones(a.m, a.anim, 0);
    Matrix M = MatrixMultiply(MatrixMultiply(MatrixScale(L.scale.x, L.scale.y, L.scale.z), MatrixRotateY(PI / 2 - yaw)), MatrixTranslate(pos.x, charY, pos.y));
    if (lying) M = MatrixMultiply(MatrixMultiply(MatrixRotateX(PI / 2), MatrixTranslate(0, .13f / L.scale.y, -.88f)), M);
    for (int i = 0; i < a.m.meshCount; i++) { int mi = a.m.meshMaterial[i]; Color c = L.col[mi]; if (tintAmt > 0) c = lerpC(c, tint, tintAmt); c.a = (unsigned char)(255 * alpha);
        a.m.materials[mi].maps[MATERIAL_MAP_DIFFUSE].color = c; DrawMesh(a.m.meshes[i], a.m.materials[mi], MatrixMultiply(a.m.transform, M)); }
    if (out) { for (int s = 0; s < 2; s++) out->hand[s] = a.rig.hand[s] >= 0 ? Vector3Transform(a.fp[a.rig.hand[s]].translation, M) : xz3(pos, 1.2f);
        out->head = a.rig.head >= 0 ? Vector3Transform(a.fp[a.rig.head].translation, M) : xz3(pos, 1.7f); out->chest = a.rig.chest >= 0 ? Vector3Transform(a.fp[a.rig.chest].translation, M) : xz3(pos, 1.4f); out->M = M; }
}

// ============================================================ world data
constexpr float MAP = 3200.f, GCELL = 32.f, CELLX = MAP + 40.f, WORLDX = MAP + 330.f; constexpr int GN = (int)(WORLDX / GCELL) + 1; // interiors (basements, upstairs) live in a strip east of the map
constexpr int PPM = 1; constexpr int GTEX = (int)MAP * PPM; constexpr float AREA = (MAP / 1600.f) * (MAP / 1600.f); // content scales with area
enum SolidKind { SK_WALL, SK_FURN, SK_CAR, SK_TRUNK, SK_FENCE, SK_ROCK, SK_BARREL, SK_TENT, SK_CRATE, SK_TOWER };
struct Solid { float x0, z0, x1, z1, h; int kind; int bld; int q = 0; int door = -1; };
enum Layer { L_ALWAYS, L_ROOF, L_WALL, L_WALLDET, L_FLOOR, L_LOFT };
struct Prop { int mesh; Vector3 p, s; float yaw; Color c; int bld; int layer; int asset = -1; int tex = 0; };
enum BType { B_HOUSE, B_STORE, B_POLICE, B_CLINIC, B_APART, B_GARAGE, B_BARN, B_BARRACKS, B_CHURCH, B_CABIN, B_OFFICE, B_HANGAR, B_SHED, B_FACTORY, B_FIRE, B_SCHOOL, B_TRAILER, B_GAS };
struct LootSpot { Vector3 p; int table; };
struct Door { Vector2 out, in; int obj = -1; };
// real doors: closable, block sight and movement, can be beaten down
struct DoorObj { Vector2 c; bool horiz; float w; bool open, broken = false; float hp = 140, bang = 0; int bld; int solid; float anim = 0; bool metal = false; float hpMax = 140, pitch = 1, cond = 1; Vector2 outN{0, 1}; };
static std::vector<DoorObj> doorObjs;
struct Loft { int bld; Vector2 base, top; float y; float x0 = 0, z0 = 0, x1 = 0, z1 = 0; }; static std::vector<Loft> lofts;      // barn haylofts
struct Cover { Vector2 p; float r; }; static std::vector<Cover> covers;                   // tall grass & bushes you can hide in
struct CarObj { Vector2 p; float yaw; bool burned, searched = false; }; static std::vector<CarObj> carObjs;
struct Building { float x0, z0, x1, z1, h; int type; bool enter; std::vector<Door> doors; std::vector<LootSpot> spots; int town; float fade = 1; float cut = 1; int cond = 1; int cellKind = 0, parent = -1; bool visited = false; std::vector<Vector2> gaps; }; // cond: 0 kept, 1 worn, 2 abandoned
struct Tree { Vector2 p; float r, h; int kind; Color c; float fade = 1; int asset = -1; float sc = 1, yaw = 0; int chops = 0, logs = 0; float shake = 0; };
struct Road { Vector2 a, b; float w; bool town; int tier = 0; }; // tier: 0 paved, 1 gravel, 2 dirt
enum TKind { TK_CITY, TK_TOWN, TK_VILLAGE, TK_MILITARY };
struct Town { const char* name; Vector2 c; float r; int kind; unsigned seed; };
static Town TOWNS[6] = { // positions are rerolled every life; layouts (seeds) never change
    {"Cedar Falls", {820, 760}, 150, TK_CITY, 101}, {"Harlow", {380, 1200}, 95, TK_TOWN, 202}, {"Millbrook", {1250, 380}, 70, TK_VILLAGE, 303},
    {"Pine Ridge", {300, 330}, 62, TK_VILLAGE, 404}, {"Fort Calloway", {1280, 1260}, 58, TK_MILITARY, 505}, {"Dunmore", {1360, 830}, 55, TK_VILLAGE, 606}};
static std::vector<Solid> solids; static std::vector<int> sgrid[GN][GN];
static std::vector<Prop> props; static std::vector<int> pgrid[GN][GN];
static std::vector<Building> blds; static std::vector<int> bgrid[GN][GN]; static std::vector<Tree> trees; static std::vector<int> tgrid[GN][GN];
static std::vector<Road> roads; static std::vector<Vector2> spawnPts; struct LootSpot; static std::vector<LootSpot> yardSpots;
struct Lamp { Vector3 p; Color c; float r; int kind; }; // static lights: fires, flares, lamps
static std::vector<Lamp> staticLights;
static Image groundImg; static Color* gpx = nullptr;
static Mesh chunkMesh[GN][GN]; static bool chunkHas[GN][GN];
static int qid = 0; static std::vector<int> qbuf;

static int gcl(float v) { return std::max(0, std::min(GN - 1, (int)(v / GCELL))); }
static void addSolid(float x0, float z0, float x1, float z1, float h, int kind, int bld = -1) {
    if (x0 > x1) std::swap(x0, x1); if (z0 > z1) std::swap(z0, z1);
    int idx = (int)solids.size(); solids.push_back({x0, z0, x1, z1, h, kind, bld});
    for (int cx = gcl(x0); cx <= gcl(x1); cx++) for (int cz = gcl(z0); cz <= gcl(z1); cz++) sgrid[cx][cz].push_back(idx);
}
static void addProp(int mesh, Vector3 p, Vector3 s, float yaw, Color c, int bld = -1, int layer = L_ALWAYS, int tex = 0) { int idx = (int)props.size(); props.push_back({mesh, p, s, yaw, c, bld, layer}); props.back().tex = tex; pgrid[gcl(p.x)][gcl(p.z)].push_back(idx); }
static void addAProp(int ai, Vector3 p, float sc, float yaw, int bld = -1, Color tint = WHITE) { if (ai < 0) return; int idx = (int)props.size(); props.push_back({-1, p, V3(sc, sc, sc), yaw, tint, bld, L_ALWAYS}); props.back().asset = ai; pgrid[gcl(p.x)][gcl(p.z)].push_back(idx); }
static Vector2 aSize(int ai, float sc, float yaw) { Vector3 z = Vector3Scale(assets[ai].size, sc); float c = fabsf(cosf(yaw)), s = fabsf(sinf(yaw)); return V2(z.x * c + z.z * s, z.x * s + z.z * c); }
static std::vector<int>& query(float x, float z, float r) {
    qid++; qbuf.clear();
    for (int cx = gcl(x - r); cx <= gcl(x + r); cx++) for (int cz = gcl(z - r); cz <= gcl(z + r); cz++)
        for (int i : sgrid[cx][cz]) if (solids[i].q != qid) { solids[i].q = qid; qbuf.push_back(i); }
    return qbuf;
}
static bool solidOn(const Solid& s) { return s.door < 0 || !(doorObjs[s.door].open || doorObjs[s.door].broken); }
static bool pointSolid(float x, float z, float minH = 0.f) {
    if (x < 0 || z < 0 || x >= MAP || z >= MAP) return true;
    for (int i : sgrid[gcl(x)][gcl(z)]) { const Solid& s = solids[i]; if (solidOn(s) && s.h > minH && x >= s.x0 && x <= s.x1 && z >= s.z0 && z <= s.z1) return true; }
    return false;
}
static bool freeAt(float x, float z, float r) {
    if (x < r || z < r || x > WORLDX - r || z > MAP - r) return false;
    for (int i : query(x, z, r)) { const Solid& s = solids[i]; if (!solidOn(s)) continue; float cx = Clamp(x, s.x0, s.x1), cz = Clamp(z, s.z0, s.z1); if ((x - cx) * (x - cx) + (z - cz) * (z - cz) < r * r) return false; }
    return true;
}
static bool boxFree(float x0, float z0, float x1, float z1) {
    for (int i : query((x0 + x1) / 2, (z0 + z1) / 2, std::max(x1 - x0, z1 - z0))) { const Solid& s = solids[i]; if (s.x0 < x1 && s.x1 > x0 && s.z0 < z1 && s.z1 > z0) return false; }
    return true;
}
static void collide(Vector2& o, float r) {
    for (int i : query(o.x, o.y, r)) {
        const Solid& s = solids[i]; if (!solidOn(s)) continue;
        float cx = Clamp(o.x, s.x0, s.x1), cz = Clamp(o.y, s.z0, s.z1), dx = o.x - cx, dz = o.y - cz, d2 = dx * dx + dz * dz;
        if (d2 < r * r) {
            if (d2 < 1e-8f) { float l = o.x - s.x0, rr = s.x1 - o.x, t = o.y - s.z0, b = s.z1 - o.y, m = std::min({l, rr, t, b}); if (m == l) o.x = s.x0 - r; else if (m == rr) o.x = s.x1 + r; else if (m == t) o.y = s.z0 - r; else o.y = s.z1 + r; }
            else { float d = sqrtf(d2); o.x += dx / d * (r - d); o.y += dz / d * (r - d); }
        }
    }
    o.x = Clamp(o.x, r, (o.x > MAP + 10 ? WORLDX : MAP) - r); o.y = Clamp(o.y, r, MAP - r);
}
static bool losClear(Vector2 a, Vector2 b) {
    float d = Vector2Distance(a, b); int n = (int)(d / .18f) + 1; // walls and doors are ~.25 m thick: step finer than that
    for (int i = 1; i < n; i++) { Vector2 p = Vector2Lerp(a, b, (float)i / n); if (pointSolid(p.x, p.y, 1.4f)) return false; }
    return true;
}
static int buildingAt(Vector2 p) { if (p.x < 0 || p.y < 0 || p.x >= WORLDX || p.y >= MAP) return -1; for (int i : bgrid[gcl(p.x)][gcl(p.y)]) { const Building& b = blds[i]; if (b.enter && p.x > b.x0 && p.x < b.x1 && p.y > b.z0 && p.y < b.z1) return i; } return -1; }
// hot zones: rerolled each life, denser and nastier, better caches
enum HZKind { HZ_CHECKPOINT, HZ_MEDEVAC, HZ_CONVOY, HZ_GRAVE, HZ_EVAC, HZ_COUNT };
static const char* HZN[HZ_COUNT] = {"Quarantine Checkpoint", "Crashed Medevac", "Burned Convoy", "Mass Grave", "Overrun Evac Point"};
static const char* HZD[HZ_COUNT] = {"The Guard held here for a day. Then it didn't.", "Somebody's meds are still in the wreck.", "A dozen trucks, no survivors, plenty of ammo.", "They ran out of lime before they ran out of bodies.", "The last helicopter left without them."};
struct HotZone { Vector2 c; float r; int kind; bool seen = false; };
static std::vector<HotZone> hotZones;
static int hotAt(Vector2 p, float k = 1.f) { for (int i = 0; i < (int)hotZones.size(); i++) if (Vector2Distance(p, hotZones[i].c) < hotZones[i].r * k) return i; return -1; }
static int townAt(Vector2 p, float k = 1.15f) { for (int i = 0; i < 6; i++) if (Vector2Distance(p, TOWNS[i].c) < TOWNS[i].r * k) return i; return -1; }
static float roadDist(Vector2 p) { float m = 1e9f; for (auto& r : roads) m = std::min(m, segDist(p, r.a, r.b) - r.w / 2); return m; }
// true if any road (plus its shoulder) touches the rectangle: nothing gets built on the asphalt
static bool rectOnRoad(float x0, float z0, float x1, float z1, float pad = .8f) {
    for (auto& r : roads) { float hw = r.w / 2 + pad;
        if (std::max(r.a.x, r.b.x) + hw < x0 || std::min(r.a.x, r.b.x) - hw > x1 || std::max(r.a.y, r.b.y) + hw < z0 || std::min(r.a.y, r.b.y) - hw > z1) continue;
        float len = Vector2Distance(r.a, r.b); int n = std::max(1, (int)(len / .75f));
        for (int i = 0; i <= n; i++) { Vector2 p = Vector2Lerp(r.a, r.b, (float)i / n); float cx = Clamp(p.x, x0, x1), cz = Clamp(p.y, z0, z1); if ((p.x - cx) * (p.x - cx) + (p.y - cz) * (p.y - cz) < hw * hw) return true; } }
    return false;
}

// ------------------------------------------------------------ ground painting
// terrain splat: 12 photo layers + AO at 1 px per meter. Every paint call also paints a layer when curLayer >= 0.
// 0 short grass, 1 long grass, 2 lawn, 3 mossy forest floor, 4 leafy dirt, 5 packed earth, 6 gray dirt, 7 gravel, 8 dark soil, 9 concrete, 10 rubble dirt, 11 sand
constexpr int SPN = (int)MAP / 2, SPC = 13; static std::vector<unsigned char> splat; static int curLayer = -1; // splat: 1 px per 2 m
static void sblend(int sx, int sy, int layer, float a) { if (sx < 0 || sy < 0 || sx >= SPN || sy >= SPN) return; unsigned char* p = &splat[((size_t)sy * SPN + sx) * SPC];
    for (int k = 0; k < 12; k++) { float t = k == layer ? 255.f : 0.f; p[k] = (unsigned char)(p[k] + (t - p[k]) * Clamp(a, 0, 1)); } }
static void aoAt(int sx, int sy, float k) { if (sx < 0 || sy < 0 || sx >= SPN || sy >= SPN) return; unsigned char& o = splat[((size_t)sy * SPN + sx) * SPC + 12]; o = (unsigned char)std::max((float)o, std::min(255.f, k * 255)); }
static void aoCircle(Vector2 c, float r, float k) { for (int y = (int)((c.y - r) / 2); y <= (int)((c.y + r) / 2); y++) for (int x = (int)((c.x - r) / 2); x <= (int)((c.x + r) / 2); x++) { float d = Vector2Distance(V2(x * 2 + 1.f, y * 2 + 1.f), c) / r; if (d < 1) aoAt(x, y, k * (1 - d * d)); } }
static void gblend(int x, int y, Color c, float a) { if (x >= 0 && y >= 0 && x < GTEX && y < GTEX) { Color& o = gpx[y * GTEX + x]; o = lerpC(o, c, a); o.a = 255; if (curLayer >= 0 && !((x | y) & 1)) sblend(x >> 1, y >> 1, curLayer, a); } }
static void paintRect(float x0, float z0, float x1, float z1, Color c, float a = 1, float jitter = 0) {
    for (int y = (int)(z0 * PPM); y < (int)(z1 * PPM); y++) for (int x = (int)(x0 * PPM); x < (int)(x1 * PPM); x++) gblend(x, y, jitter > 0 ? mulC(c, 1 + frand(-jitter, jitter)) : c, a);
}
static void paintCircle(Vector2 c, float r, Color col, float a, bool soft = true) {
    for (int y = (int)((c.y - r) * PPM); y < (int)((c.y + r) * PPM); y++) for (int x = (int)((c.x - r) * PPM); x < (int)((c.x + r) * PPM); x++) {
        float d = Vector2Distance(V2((float)x / PPM, (float)y / PPM), c) / r; if (d < 1) gblend(x, y, col, a * (soft ? (1 - d * d) : 1));
    }
}
static void paintSeg(Vector2 a, Vector2 b, float w, Color c, float alpha) {
    float x0 = std::min(a.x, b.x) - w, x1 = std::max(a.x, b.x) + w, z0 = std::min(a.y, b.y) - w, z1 = std::max(a.y, b.y) + w;
    for (int y = (int)(z0 * PPM); y < (int)(z1 * PPM); y++) for (int x = (int)(x0 * PPM); x < (int)(x1 * PPM); x++) {
        float d = segDist(V2((float)x / PPM, (float)y / PPM), a, b); if (d < w) gblend(x, y, c, alpha * Clamp((w - d) / 1.5f, 0, 1));
    }
}

// soft contact darkening on the ground around a footprint (cheap baked AO)
static void aoRect(float x0, float z0, float x1, float z1, float R, float k) {
    for (int y = (int)((z0 - R) * PPM); y < (int)((z1 + R) * PPM); y++) for (int x = (int)((x0 - R) * PPM); x < (int)((x1 + R) * PPM); x++) {
        float px = (float)x / PPM, pz = (float)y / PPM, dx = std::max({x0 - px, 0.f, px - x1}), dz = std::max({z0 - pz, 0.f, pz - z1}), d = sqrtf(dx * dx + dz * dz);
        if (d < R) { float a = 1 - d / R; gblend(x, y, rgba(10, 12, 8), k * a * a); if (!((x | y) & 1)) aoAt(x >> 1, y >> 1, k * a * a * .9f); }
    }
}
// ------------------------------------------------------------ building construction
static const std::vector<Color> HOUSE_WALL = {rgba(176, 168, 150), rgba(150, 160, 168), rgba(190, 180, 158), rgba(140, 150, 130), rgba(170, 120, 100), rgba(200, 196, 184)};
static const std::vector<Color> ROOF_C = {rgba(70, 60, 58), rgba(60, 66, 74), rgba(92, 58, 48), rgba(56, 60, 56), rgba(80, 78, 76)};
static const std::vector<Color> CAR_C = {rgba(120, 36, 36), rgba(40, 64, 100), rgba(90, 94, 100), rgba(130, 118, 84), rgba(46, 86, 66), rgba(38, 40, 44), rgba(170, 172, 176)};
enum LootTable { LT_HOUSE, LT_KITCHEN, LT_STORE, LT_POLICE, LT_MED, LT_GARAGE, LT_BARN, LT_MIL, LT_OFFICE, LT_CACHE, LT_WOODPILE, LT_CAR, LT_WARDROBE, LT_SHED, LT_FACTORY, LT_FIRE, LT_SCHOOL, LT_GAS, LT_TRAILER, LT_HUNT, LT_CLOTHES, LT_BASEMENT };

static int curWallTex = 0, curCond = 1;
static Color houseColor() { // paint: whites, creams, pastels, earth tones, with jitter
    static const Color P[] = {rgba(222, 218, 206), rgba(232, 222, 190), rgba(186, 204, 214), rgba(170, 190, 160), rgba(226, 206, 140), rgba(160, 160, 158), rgba(176, 96, 80), rgba(140, 110, 84), rgba(176, 210, 192), rgba(206, 170, 150), rgba(110, 126, 140), rgba(150, 70, 60)};
    Color c = P[irand(0, 11)]; int j = 14; return rgba(c.r + irand(-j, j), c.g + irand(-j, j), c.b + irand(-j, j)); }
struct FurnBounds { bool on = false; float x0, z0, x1, z1; } furnB;
static void wallSeg(float ax, float az, float bx, float bz, float H, Color c, int bi, bool exterior, bool windows, float outN) {
    const float t = .22f; bool horiz = fabsf(az - bz) < .01f; float len = horiz ? fabsf(bx - ax) : fabsf(bz - az);
    if (len < .05f) return;
    float cx = (ax + bx) / 2, cz = (az + bz) / 2;
    addProp(M_CUBE, V3(cx, H / 2, cz), horiz ? V3(len + t, H, t) : V3(t, H, len + t), 0, c, bi, L_WALL, curWallTex);
    if (horiz) addSolid(std::min(ax, bx) - t / 2, az - t / 2, std::max(ax, bx) + t / 2, az + t / 2, H, SK_WALL, bi);
    else addSolid(ax - t / 2, std::min(az, bz) - t / 2, ax + t / 2, std::max(az, bz) + t / 2, H, SK_WALL, bi);
    if (exterior && windows && len > 2.6f) {
        int nw = (int)(len / 3.2f);
        for (int i = 0; i < nw; i++) {
            float f = (i + .5f) / nw; float wx = horiz ? std::min(ax, bx) + f * len : ax, wz = horiz ? az : std::min(az, bz) + f * len;
            static const float bp[3] = {.02f, .18f, .55f}, brk[3] = {0, .15f, .3f}; bool board = chance(bp[curCond]), broken = !board && chance(brk[curCond]);
            Color wc = board ? rgba(110, 86, 60) : broken ? rgba(12, 12, 14) : rgba(58, 74, 92);
            Vector3 p = horiz ? V3(wx, 1.75f, wz + outN * (t / 2 + .02f)) : V3(wx + outN * (t / 2 + .02f), 1.75f, wz);
            Vector3 fr = horiz ? V3(wx, 1.75f, wz + outN * (t / 2 + .01f)) : V3(wx + outN * (t / 2 + .01f), 1.75f, wz);
            addProp(M_CUBE, fr, horiz ? V3(1.26f, 1.16f, .04f) : V3(.04f, 1.16f, 1.26f), 0, rgba(214, 210, 200), bi, L_WALLDET);
            addProp(M_CUBE, p, horiz ? V3(1.1f, 1.0f, .05f) : V3(.05f, 1.0f, 1.1f), 0, wc, bi, L_WALLDET, board ? TX_PLYWOOD : 0);
            Vector3 sill = horiz ? V3(wx, 1.22f, wz + outN * (t / 2 + .06f)) : V3(wx + outN * (t / 2 + .06f), 1.22f, wz);
            addProp(M_CUBE, sill, horiz ? V3(1.3f, .08f, .12f) : V3(.12f, .08f, 1.3f), 0, mulC(c, .8f), bi, L_WALLDET);
        }
    }
}
// model furniture: facing 0=+Z(south) 1=-X(west) 2=-Z(north) 3=+X(east); keeps footprint inside building
static bool furnA(int bi, const char* key, float x, float z, int facing, int table, float sc = .5f, bool solid = true) {
    int ai = AS(key); if (ai < 0) return false; const Building& B = blds[bi];
    static const float TH[4] = {0, -PI / 2, PI, PI / 2}; float yaw = -TH[facing & 3];
    if (curCond == 2 && chance(.3f)) return true; // looted, burned, hauled off
    if (curCond == 2 && chance(.3f)) yaw += frand(-.8f, .8f); // shoved around
    Vector2 fp = aSize(ai, sc, yaw); float bx0 = furnB.on ? furnB.x0 : B.x0, bz0 = furnB.on ? furnB.z0 : B.z0, bx1 = furnB.on ? furnB.x1 : B.x1, bz1 = furnB.on ? furnB.z1 : B.z1;
    x = Clamp(x, bx0 + fp.x / 2 + .14f, std::max(bx0 + fp.x / 2 + .14f, bx1 - fp.x / 2 - .14f)); z = Clamp(z, bz0 + fp.y / 2 + .14f, std::max(bz0 + fp.y / 2 + .14f, bz1 - fp.y / 2 - .14f));
    float h = assets[ai].size.y * sc;
    addAProp(ai, V3(x, .06f, z), sc, yaw, bi);
    if (solid) addSolid(x - fp.x / 2, z - fp.y / 2, x + fp.x / 2, z + fp.y / 2, h, SK_FURN, bi);
    if (table >= 0) blds[bi].spots.push_back({V3(x, std::min(h, 1.0f) + .08f, z), table});
    return true;
}
static void furn(int bi, float x, float z, float w, float d, float h, Color c, int table, bool solid = true) {
    addProp(M_CUBE, V3(x, h / 2 + .06f, z), V3(w, h, d), 0, c, bi, L_ALWAYS);
    if (solid) addSolid(x - w / 2, z - d / 2, x + w / 2, z + d / 2, h, SK_FURN, bi);
    if (table >= 0) blds[bi].spots.push_back({V3(x, h + .1f, z), table});
}
static int makeDoor(Vector2 c, bool horiz, float w, int bi) {
    int di = (int)doorObjs.size(); DoorObj d; d.c = c; d.horiz = horiz; d.w = w; d.bld = bi; int ty = blds[bi].type, cnd = blds[bi].cond;
    d.metal = (ty == B_POLICE || ty == B_STORE || ty == B_CLINIC || ty == B_GARAGE || ty == B_HANGAR || ty == B_BARRACKS || ty == B_OFFICE || ty == B_FACTORY || ty == B_FIRE || ty == B_SCHOOL || ty == B_GAS || ty == B_TRAILER) ? chance(.7f) : ty == B_SHED ? chance(.4f) : chance(.1f);
    d.cond = Clamp(frand(.35f, 1.f) - cnd * .25f, .1f, 1); d.hpMax = d.hp = (d.metal ? frand(260, 420) : frand(90, 170)) * (.4f + .6f * d.cond) * (w > 3 ? 1.6f : 1); d.pitch = frand(.85f, 1.15f);
    static const float openP[3] = {.15f, .45f, .45f}; d.open = chance(openP[cnd]); if (cnd == 2 && chance(.35f)) { d.broken = true; d.hp = 0; }
    float t = .14f; if (horiz) addSolid(c.x - w / 2, c.y - t, c.x + w / 2, c.y + t, 2.6f, SK_WALL, bi); else addSolid(c.x - t, c.y - w / 2, c.x + t, c.y + w / 2, 2.6f, SK_WALL, bi);
    d.solid = (int)solids.size() - 1; solids.back().door = di; d.anim = d.open ? 1 : 0; doorObjs.push_back(d); return di;
}
static void woodpile(Vector2 p, int bi) { // stacked logs; the yard spot rolls logs, kindling, matches, often an axe
    if (!boxFree(p.x - 1, p.y - .5f, p.x + 1, p.y + .5f) || rectOnRoad(p.x - 1, p.y - .5f, p.x + 1, p.y + .5f)) return;
    for (int k = 0; k < 3; k++) addProp(M_CUBE, V3(p.x, .2f + k * .32f, p.y), V3(1.8f - k * .3f, .3f, .7f), 0, mulC(rgba(110, 76, 48), .85f + k * .08f), bi);
    addSolid(p.x - .9f, p.y - .35f, p.x + .9f, p.y + .35f, 1, SK_CRATE); yardSpots.push_back({V3(p.x + 1.3f, .1f, p.y), LT_WOODPILE});
}
// doorSide: 0 north(z0) 1 south(z1) 2 west(x0) 3 east(x1)
enum VKind { VK_CIVIL, VK_POLICE, VK_AMBULANCE, VK_FIRE, VK_DELIVERY, VK_GARBAGE, VK_TRACTOR, VK_MILITARY, VK_TRUCK };
static void car(float x, float z, float yaw, float burnP, int kind);
static void crate(Vector2 p, int table);
static int forceWallTex = -1; static bool upstairsGen = false;
struct Gap { Vector2 c; bool vert; }; static std::vector<Gap> curGaps; // interior doorways of the building being made
static int makeBuildingRaw(float x0, float z0, float w, float d, int type, int floors, int doorSide, int town, Color wallC, Color roofC);
static int makeBuilding(float x0, float z0, float w, float d, int type, int floors, int doorSide, int town, Color wallC, Color roofC) {
    curGaps.clear(); size_t p0 = props.size(); int bi = makeBuildingRaw(x0, z0, w, d, type, floors, doorSide, town, wallC, roofC); if (bi < 0) return bi;
    // keep every doorway walkable: nothing solid in front of or behind a door or an interior opening
    std::vector<Rectangle> keep;
    for (auto& g : curGaps) keep.push_back(g.vert ? Rectangle{g.c.x - 1.3f, g.c.y - .6f, 2.6f, 1.2f} : Rectangle{g.c.x - .6f, g.c.y - 1.3f, 1.2f, 2.6f});
    for (auto& dr : blds[bi].doors) { if (dr.obj < 0) continue; const DoorObj& D = doorObjs[dr.obj]; Vector2 in = Vector2Subtract(D.c, Vector2Scale(D.outN, 2.4f)); float hw = D.w / 2 + .35f;
        keep.push_back({std::min(D.c.x, in.x) - (D.horiz ? hw : .3f), std::min(D.c.y, in.y) - (D.horiz ? .3f : hw), fabsf(D.c.x - in.x) + (D.horiz ? 2 * hw : .6f), fabsf(D.c.y - in.y) + (D.horiz ? .6f : 2 * hw)}); }
    for (auto& g : curGaps) blds[bi].gaps.push_back(g.c);
    auto hitK = [&](float a0, float b0, float a1, float b1) { for (auto& k : keep) if (a0 < k.x + k.width && a1 > k.x && b0 < k.y + k.height && b1 > k.y) return true; return false; };
    for (auto& so : solids) if (so.bld == bi && so.kind == SK_FURN && hitK(so.x0, so.z0, so.x1, so.z1)) { so.x0 = so.x1 = so.z0 = so.z1 = -500; }
    props.erase(std::remove_if(props.begin() + p0, props.end(), [&](const Prop& p) { return p.bld == bi && p.layer == L_ALWAYS && p.p.y < 2.4f && hitK(p.p.x - .3f, p.p.z - .3f, p.p.x + .3f, p.p.z + .3f); }), props.end());
    return bi;
}
static int makeBuildingRaw(float x0, float z0, float w, float d, int type, int floors, int doorSide, int town, Color wallC, Color roofC) {
    if (!boxFree(x0 - .3f, z0 - .3f, x0 + w + .3f, z0 + d + .3f) || rectOnRoad(x0, z0, x0 + w, z0 + d, 1.2f)) return -1;
    aoRect(x0, z0, x0 + w, z0 + d, 2.2f, .5f);
    int bi = (int)blds.size(); Building B; B.x0 = x0; B.z0 = z0; B.x1 = x0 + w; B.z1 = z0 + d; B.type = type; B.town = town;
    { float r = frand(0, 1); B.cond = r < .3f ? 0 : r < .72f ? 1 : 2; } curCond = B.cond; furnB.on = false;
    if (type == B_HOUSE) wallC = houseColor();
    if (B.cond == 1) wallC = lerpC(wallC, rgba(120, 112, 98), .18f); else if (B.cond == 2) { wallC = lerpC(wallC, rgba(84, 80, 70), .42f); roofC = lerpC(roofC, rgba(52, 62, 44), .35f); }
    B.enter = type != B_APART && type != B_OFFICE; float H = 3.f; B.h = floors * H; blds.push_back(B);
    for (int cx = gcl(x0); cx <= gcl(x0 + w); cx++) for (int cz = gcl(z0); cz <= gcl(z0 + d); cz++) bgrid[cx][cz].push_back(bi);
    float x1 = x0 + w, z1 = z0 + d;
    // floor
    bool ind = type == B_BARN || type == B_GARAGE || type == B_HANGAR || type == B_SHED || type == B_FACTORY || type == B_FIRE, pub = type == B_CLINIC || type == B_STORE || type == B_POLICE || type == B_SCHOOL || type == B_GAS;
    Color floorC = ind ? rgba(110, 106, 98) : pub ? rgba(190, 190, 184) : rgba(124, 98, 76);
    int floorTex = ind ? TX_CONCRETE : (pub || type == B_OFFICE) ? TX_TILE : TX_HARDWOOD;
    addProp(M_CUBE, V3(x0 + w / 2, .03f, z0 + d / 2), V3(w, .06f, d), 0, floorC, bi, L_FLOOR, floorTex);
    switch (type) { case B_HOUSE: curWallTex = chance(.65f) ? TX_SIDING : TX_BRICK; break; case B_CABIN: curWallTex = TX_CABIN; break; case B_BARN: curWallTex = TX_BARNWOOD; break;
        case B_APART: case B_OFFICE: curWallTex = chance(.6f) ? TX_BRICK : TX_PLASTER; break; case B_GARAGE: case B_HANGAR: case B_TRAILER: curWallTex = TX_METAL; break; case B_SHED: curWallTex = chance(.5f) ? TX_BARNWOOD : TX_METAL; break;
        case B_FACTORY: curWallTex = chance(.5f) ? TX_METAL : TX_BRICK; break; case B_FIRE: case B_SCHOOL: curWallTex = TX_BRICK; break; default: curWallTex = chance(.5f) ? TX_PLASTER : TX_BRICK; }
    if (forceWallTex >= 0) curWallTex = forceWallTex;
    if (curWallTex == TX_BRICK && type != B_HOUSE) wallC = lerpC(wallC, rgba(150, 82, 64), .6f); else if (curWallTex == TX_BRICK) wallC = lerpC(wallC, rgba(160, 90, 70), .7f);
    float dw = type == B_BARN || type == B_GARAGE || type == B_HANGAR || type == B_FACTORY || type == B_FIRE ? 3.6f : type == B_SHED || type == B_TRAILER ? 1.2f : 1.6f;
    auto side = [&](int s, float ax, float az, float bx, float bz, float outN) {
        bool horiz = s < 2; bool hasDoor = B.enter && (s == doorSide || (w * d > 140 && s == (doorSide ^ 1)));
        if (hasDoor) {
            float L = horiz ? bx - ax : bz - az, c = L * frand(.35f, .65f);
            if (horiz) { wallSeg(ax, az, ax + c - dw / 2, az, H, wallC, bi, true, true, outN); wallSeg(ax + c + dw / 2, az, bx, bz, H, wallC, bi, true, true, outN);
                blds[bi].doors.push_back({V2(ax + c, az + outN * 1.6f), V2(ax + c, az - outN * 1.6f), makeDoor(V2(ax + c, az), true, dw, bi)}); doorObjs.back().outN = V2(0, outN);
                addProp(M_CUBE, V3(ax + c, H - .25f, az), V3(dw, .5f, .24f), 0, wallC, bi, L_WALLDET); }
            else { wallSeg(ax, az, ax, az + c - dw / 2, H, wallC, bi, true, true, outN); wallSeg(ax, az + c + dw / 2, bx, bz, H, wallC, bi, true, true, outN);
                blds[bi].doors.push_back({V2(ax + outN * 1.6f, az + c), V2(ax - outN * 1.6f, az + c), makeDoor(V2(ax, az + c), false, dw, bi)}); doorObjs.back().outN = V2(outN, 0);
                addProp(M_CUBE, V3(ax, H - .25f, az + c), V3(.24f, .5f, dw), 0, wallC, bi, L_WALLDET); }
        } else wallSeg(ax, az, bx, bz, H, wallC, bi, true, true, outN);
    };
    side(0, x0, z0, x1, z0, -1); side(1, x0, z1, x1, z1, 1); side(2, x0, z0, x0, z1, -1); side(3, x1, z0, x1, z1, 1);
    // interior partition
    struct RoomR { float x0, z0, x1, z1; int kind = 0; }; std::vector<RoomR> rooms;
    if (type == B_HOUSE) { // 3-5 rooms by recursive splits, each wall with a doorway
        rooms.push_back({x0, z0, x1, z1}); int want = irand(3, 5); int saveTex = curWallTex; curWallTex = TX_PLASTER; Color inC = lerpC(wallC, rgba(214, 206, 190), .55f);
        for (int it = 0; it < 12 && (int)rooms.size() < want; it++) {
            int bi2 = -1; float ba = 0; for (int i = 0; i < (int)rooms.size(); i++) { const RoomR& r = rooms[i]; float a = (r.x1 - r.x0) * (r.z1 - r.z0); if (std::max(r.x1 - r.x0, r.z1 - r.z0) >= 5.2f && a > ba) { ba = a; bi2 = i; } }
            if (bi2 < 0) break; RoomR r = rooms[bi2]; bool vert = (r.x1 - r.x0) >= (r.z1 - r.z0); float t = .5f;
            for (int tr = 0; tr < 8; tr++) { t = frand(.38f, .62f); float sp = vert ? r.x0 + (r.x1 - r.x0) * t : r.z0 + (r.z1 - r.z0) * t; bool bad = false; // keep inner walls clear of the front door
                for (auto& dr : blds[bi].doors) { Vector2 dc = Vector2Scale(Vector2Add(dr.out, dr.in), .5f); if (fabsf((vert ? dc.x : dc.y) - sp) < 1.4f) bad = true; }
                for (auto& g : curGaps) { if (vert ? (fabsf(g.c.x - sp) < 1.1f && g.c.y > r.z0 - .2f && g.c.y < r.z1 + .2f) : (fabsf(g.c.y - sp) < 1.1f && g.c.x > r.x0 - .2f && g.c.x < r.x1 + .2f)) bad = true; } // never wall off an existing doorway
                if (!bad) break; if (tr == 7) t = -1; }
            if (t < 0) continue;
            if (vert) { float sx = r.x0 + (r.x1 - r.x0) * t, gc = frand(r.z0 + .9f, r.z1 - .9f); wallSeg(sx, r.z0, sx, gc - .6f, H, inC, bi, false, false, 0); wallSeg(sx, gc + .6f, sx, r.z1, H, inC, bi, false, false, 0); curGaps.push_back({V2(sx, gc), true});
                rooms[bi2] = {r.x0, r.z0, sx, r.z1}; rooms.push_back({sx, r.z0, r.x1, r.z1}); }
            else { float sz = r.z0 + (r.z1 - r.z0) * t, gc = frand(r.x0 + .9f, r.x1 - .9f); wallSeg(r.x0, sz, gc - .6f, sz, H, inC, bi, false, false, 0); wallSeg(gc + .6f, sz, r.x1, sz, H, inC, bi, false, false, 0); curGaps.push_back({V2(gc, sz), false});
                rooms[bi2] = {r.x0, r.z0, r.x1, sz}; rooms.push_back({r.x0, sz, r.x1, r.z1}); } }
        curWallTex = saveTex;
        std::sort(rooms.begin(), rooms.end(), [](const RoomR& a, const RoomR& b) { return (a.x1 - a.x0) * (a.z1 - a.z0) > (b.x1 - b.x0) * (b.z1 - b.z0); });
        for (int i = 0; i < (int)rooms.size(); i++) rooms[i].kind = upstairsGen ? ((i == (int)rooms.size() - 1 && rooms.size() >= 3) ? 3 : 2) : i == 0 ? 0 : i == 1 ? 1 : (i == (int)rooms.size() - 1 && rooms.size() >= 4) ? 3 : 2; // 0 living 1 kitchen 2 bedroom 3 bath. Upstairs: bedrooms and a bath
    }
    else if (B.enter && w > 9 && d > 7 && type != B_GARAGE && type != B_BARRACKS && type != B_FIRE && type != B_FACTORY) { float px = x0 + w * frand(.45f, .6f), g = z0 + d * frand(.35f, .65f); if (type == B_CLINIC) curWallTex = TX_HTILE; wallSeg(px, z0, px, g - .8f, H, mulC(wallC, .92f), bi, false, false, 0); wallSeg(px, g + .8f, px, z1, H, mulC(wallC, .92f), bi, false, false, 0); curGaps.push_back({V2(px, g), true}); }
    // upper floors & roof
    if (floors > 1) addProp(M_CUBE, V3(x0 + w / 2, H + (floors - 1) * H / 2, z0 + d / 2), V3(w + .22f, (floors - 1) * H, d + .22f), 0, wallC, bi, L_ROOF, curWallTex);
    if (floors > 1) for (int f = 1; f < floors; f++) for (float xx = x0 + 1.6f; xx < x1 - 1; xx += 3.2f) {
        addProp(M_CUBE, V3(xx, f * H + 1.7f, z1 + .14f), V3(1.1f, 1.f, .05f), 0, rgba(28, 34, 42), bi, L_ROOF);
        addProp(M_CUBE, V3(xx, f * H + 1.7f, z0 - .14f), V3(1.1f, 1.f, .05f), 0, rgba(28, 34, 42), bi, L_ROOF);
    }
    float top = floors * H;
    bool gable = type == B_HOUSE || type == B_BARN || type == B_CABIN || type == B_CHURCH || type == B_SHED;
    if (gable) {
        float rh = type == B_BARN ? 3.2f : type == B_CHURCH ? 4.f : type == B_SHED ? 1.2f : 2.2f; bool alongX = w >= d;
        addProp(M_GABLE, V3(x0 + w / 2, top, z0 + d / 2), alongX ? V3(w + .7f, rh, d + .7f) : V3(d + .7f, rh, w + .7f), alongX ? 0 : PI / 2, roofC, bi, L_ROOF, TX_ROOFTILE);
        if (type == B_CHURCH) { addProp(M_CUBE, V3(x0 + 2, top + 3, z0 + d / 2), V3(2.5f, 6, 2.5f), 0, wallC, bi, L_ROOF); addProp(M_CONE, V3(x0 + 2, top + 6, z0 + d / 2), V3(3, 4, 3), 0, roofC, bi, L_ROOF); }
        if (type == B_HOUSE && chance(.6f)) addProp(M_CUBE, V3(x0 + w * .75f, top + 1.6f, z0 + d * .3f), V3(.7f, 2.f, .7f), 0, rgba(110, 70, 60), bi, L_ROOF);
    } else {
        addProp(M_CUBE, V3(x0 + w / 2, top + .1f, z0 + d / 2), V3(w + .3f, .2f, d + .3f), 0, roofC, bi, L_ROOF, TX_CONCRETE);
        addProp(M_CUBE, V3(x0 + w / 2, top + .45f, z0 + .1f), V3(w + .3f, .5f, .2f), 0, mulC(roofC, .8f), bi, L_ROOF);
        addProp(M_CUBE, V3(x0 + w / 2, top + .45f, z1 - .1f), V3(w + .3f, .5f, .2f), 0, mulC(roofC, .8f), bi, L_ROOF);
        addProp(M_CUBE, V3(x0 + .1f, top + .45f, z0 + d / 2), V3(.2f, .5f, d + .3f), 0, mulC(roofC, .8f), bi, L_ROOF);
        addProp(M_CUBE, V3(x1 - .1f, top + .45f, z0 + d / 2), V3(.2f, .5f, d + .3f), 0, mulC(roofC, .8f), bi, L_ROOF);
        for (int i = 0; i < irand(1, 3); i++) addProp(M_CUBE, V3(x0 + frand(1.5f, w - 1.5f), top + .7f, z0 + frand(1.5f, d - 1.5f)), V3(frand(1, 2), 1, frand(1, 1.6f)), 0, rgba(120, 126, 134), bi, L_ROOF);
    }
    if (!B.enter) return bi;
    // furniture & loot spots by type
    Color wood = rgba(110, 80, 52), metal = rgba(100, 106, 112), white = rgba(200, 200, 196);
    auto rp = [&](float m) { return V2(frand(x0 + m, x1 - m), frand(z0 + m, z1 - m)); };
    switch (type) {
    case B_HOUSE: if (!rooms.empty() && AS("interior/Bed_Single") >= 0) {
        for (auto& r : rooms) { furnB = {true, r.x0, r.z0, r.x1, r.z1}; float rw = r.x1 - r.x0, rd = r.z1 - r.z0;
            switch (r.kind) {
            case 0: furnA(bi, "interior/Couch_Medium1", r.x0 + rw * .5f, r.z1, 2, chance(.4f) ? LT_HOUSE : -1); furnA(bi, "interior/Table_RoundSmall", r.x0 + rw * .5f, r.z0 + rd * .45f, 0, LT_HOUSE);
                furnA(bi, chance(.5f) ? "interior/Bookshelf" : "interior/Drawer_1", r.x1, r.z0 + rd * .3f, 1, LT_HOUSE); if (chance(.6f)) furnA(bi, "interior/Carpet_1", r.x0 + rw * .5f, r.z0 + rd * .55f, irand(0, 3), -1, .5f, false);
                if (chance(.5f)) furnA(bi, "interior/Houseplant_1", r.x0, r.z0, 0, -1, .5f, false); break;
            case 1: furnA(bi, "interior/Kitchen_Fridge", r.x0, r.z0, 0, LT_KITCHEN); furnA(bi, "interior/Kitchen_2Drawers", r.x0 + 1.2f, r.z0, 0, LT_KITCHEN); furnA(bi, "interior/Kitchen_Oven", r.x0 + 2.2f, r.z0, 0, chance(.5f) ? LT_KITCHEN : -1);
                if (rw > 3.4f) furnA(bi, "interior/Kitchen_Sink", r.x0 + 3.2f, r.z0, 0, -1); if (rd > 3.2f) { furnA(bi, "interior/Table_RoundSmall", r.x0 + rw * .55f, r.z1 - 1.2f, 0, LT_KITCHEN); furnA(bi, "interior/Chair_1", r.x0 + rw * .55f - .75f, r.z1 - 1.2f, 3, -1, .5f, false); } break;
            case 2: furnA(bi, rw * rd > 11 && chance(.6f) ? "interior/Bed_King" : "interior/Bed_Single", r.x1, r.z1, 2, LT_HOUSE); furnA(bi, "interior/NightStand_1", r.x1 - 2.1f, r.z1, 2, LT_HOUSE);
                furnA(bi, "interior/Drawer_1", r.x0, r.z0 + rd * .4f, 3, LT_WARDROBE); if (chance(.5f)) blds[bi].spots.push_back({V3(r.x0 + .5f, .1f, r.z1 - .5f), LT_WARDROBE}); break;
            case 3: furnA(bi, "interior/Bathroom_Toilet", r.x0, r.z0, 0, -1, .5f); if (chance(.5f)) blds[bi].spots.push_back({V3(r.x0 + rw * .6f, .9f, r.z0 + .5f), LT_MED}); break; } }
        furnB.on = false; if (B.cond == 2 && chance(.15f)) blds[bi].spots.push_back({V3(rooms.back().x0 + .6f, .1f, rooms.back().z0 + .6f), LT_CACHE}); // someone's stash, missed
        break; }
    [[fallthrough]];
    case B_CABIN: {
        if (!furnA(bi, "interior/Kitchen_Fridge", x0 + .5f, z0 + .4f, 0, LT_KITCHEN)) { furn(bi, x0 + 1.0f, z0 + 1.2f, 1.6f, .6f, .9f, wood, LT_KITCHEN); furn(bi, x1 - 1.3f, z1 - 1.4f, 1.4f, 2.0f, .5f, rgba(130, 60, 60), LT_HOUSE); break; }
        furnA(bi, "interior/Kitchen_2Drawers", x0 + 1.7f, z0 + .3f, 0, LT_KITCHEN); furnA(bi, "interior/Kitchen_Oven", x0 + 2.75f, z0 + .3f, 0, chance(.5f) ? LT_KITCHEN : -1);
        furnA(bi, chance(.5f) ? "interior/Bed_King" : "interior/Bed_Single", x1 - .8f, z1 - 1.2f, 2, LT_HOUSE); furnA(bi, "interior/NightStand_1", x1 - .3f, z1 - 2.6f, 1, LT_WARDROBE);
        if (w > 7.5f) furnA(bi, "interior/Couch_Medium1", x0 + 1.4f, z1 - .5f, 2, chance(.4f) ? LT_HOUSE : -1);
        { Vector2 t = V2(x0 + w * .35f, z0 + d * .55f); furnA(bi, "interior/Table_RoundSmall", t.x, t.y, 0, LT_HOUSE); furnA(bi, "interior/Chair_1", t.x - .75f, t.y, 3, -1, .5f, false); furnA(bi, "interior/Chair_1", t.x + .75f, t.y, 1, -1, .5f, false); }
        furnA(bi, chance(.5f) ? "interior/Bookshelf" : "interior/Drawer_1", x1 - .3f, z0 + 1.2f, 1, LT_HOUSE);
        if (chance(.6f)) furnA(bi, "interior/Houseplant_1", x0 + .35f, z1 - 1.6f, 0, -1, .5f, false);
        if (chance(.6f)) furnA(bi, "interior/Carpet_1", x0 + w * .55f, z0 + d * .6f, irand(0, 3), -1, .5f, false);
        break; }
    case B_STORE:
        for (float sz = z0 + 2.2f; sz < z1 - 1.5f; sz += 2.4f) { int ai = AS("interior/Shelf_Large"); if (ai >= 0) { float sc = std::min(.6f, (w * .5f) / std::max(.1f, assets[ai].size.x)); { int tb = chance(.3f) ? LT_CLOTHES : LT_STORE; furnA(bi, "interior/Shelf_Large", x0 + w * .45f, sz, 0, tb, sc); if (chance(.6f)) blds[bi].spots.push_back({V3(x0 + w * .45f + frand(-1, 1), .9f, sz), tb}); } }
            else furn(bi, x0 + w * .45f, sz, w * .55f, .5f, 1.4f, metal, LT_STORE); }
        furn(bi, x1 - 1.2f, z0 + 1.0f, 1.8f, .8f, 1.0f, wood, LT_STORE); break;
    case B_POLICE:
        if (!furnA(bi, "interior/Kitchen_2Drawers", x0 + 1.5f, z0 + .4f, 0, LT_OFFICE)) furn(bi, x0 + 1.5f, z0 + 1.2f, 2.2f, .8f, .8f, wood, LT_OFFICE);
        furnA(bi, "interior/Chair_1", x0 + 1.5f, z0 + 1.3f, 2, -1, .5f, false);
        if (!furnA(bi, "interior/Bookshelf", x1 - .3f, z0 + 1.5f, 1, LT_POLICE)) furn(bi, x1 - .6f, z0 + 1.5f, .6f, 2.4f, 1.8f, metal, LT_POLICE);
        if (!furnA(bi, "interior/Drawer_1", x1 - .3f, z1 - 1.5f, 1, LT_POLICE)) furn(bi, x1 - .6f, z1 - 1.5f, .6f, 2.4f, 1.8f, metal, LT_POLICE);
        { Vector2 t = rp(2.5f); if (!furnA(bi, "interior/Table_RoundSmall", t.x, t.y, 0, LT_POLICE)) furn(bi, t.x, t.y, 1.4f, .8f, .8f, wood, LT_POLICE); } break;
    case B_CLINIC:
        for (int i = 0; i < 3; i++) if (!furnA(bi, "interior/Bed_Single", x0 + 1.0f + i * 1.6f, z1 - 1.1f, 2, LT_MED)) furn(bi, x0 + 1.4f + i * 2.4f, z1 - 1.3f, 1.0f, 2.0f, .7f, white, LT_MED);
        if (!furnA(bi, "interior/Shelf_Small1", x1 - .3f, z0 + 2, 1, LT_MED)) furn(bi, x1 - .6f, z0 + 2, .6f, 2.4f, 1.8f, white, LT_MED);
        if (!furnA(bi, "interior/Kitchen_Sink", x0 + 1.2f, z0 + .3f, 0, LT_MED)) furn(bi, x0 + 2, z0 + 1.0f, 2.4f, .7f, .9f, white, LT_MED);
        furnA(bi, "interior/Kitchen_2Drawers", x0 + 2.3f, z0 + .3f, 0, LT_MED); break;
    case B_GARAGE: case B_BARN: case B_HANGAR:
        furn(bi, x0 + .7f, z0 + d / 2, .8f, d * .5f, 1.0f, metal, type == B_BARN ? LT_BARN : LT_GARAGE);
        furn(bi, x1 - 1.5f, z1 - 1.5f, 2.f, 2.f, 1.2f, rgba(170, 150, 90), type == B_BARN ? LT_BARN : LT_GARAGE);
        if (type == B_HANGAR) { furn(bi, x0 + w / 2, z0 + d / 2, 6, 3, 1.8f, rgba(70, 80, 60), LT_MIL); }
        if (type == B_BARN) { // hayloft over the back third, ladder at its edge
            float ly = 3.4f, lw = w * .38f; addProp(M_CUBE, V3(x1 - lw / 2 - .1f, ly, z0 + d / 2), V3(lw, .2f, d - .4f), 0, rgba(120, 90, 60), bi, L_LOFT, TX_PLANK);
            addProp(M_CUBE, V3(x1 - lw - .1f, ly + .5f, z0 + d * .3f), V3(.08f, .08f, d * .5f), 0, rgba(100, 78, 54), bi, L_LOFT); // railing along the open edge
            for (int k = 0; k < 4; k++) addProp(M_CUBE, V3(x1 - .7f, ly + .45f, z0 + 1.0f + k * 1.25f), V3(1.1f, .7f, .8f), frand(-.2f, .2f), rgba(196, 170, 90), bi, L_LOFT);
            if (chance(.5f)) addProp(M_CUBE, V3(x1 - lw * .5f, ly + .25f, z1 - 1.4f), V3(.8f, .3f, 1.9f), 0, rgba(90, 96, 70), bi, L_LOFT); // someone's cot
            float lx = x1 - lw - .25f, lz = z0 + d * .7f; addProp(M_CUBE, V3(lx, ly / 2, lz - .25f), V3(.08f, ly + .4f, .08f), 0, rgba(90, 70, 50), bi, L_ALWAYS); addProp(M_CUBE, V3(lx, ly / 2, lz + .25f), V3(.08f, ly + .4f, .08f), 0, rgba(90, 70, 50), bi, L_ALWAYS);
            for (float ry = .4f; ry < ly; ry += .45f) addProp(M_CUBE, V3(lx, ry, lz), V3(.06f, .06f, .55f), 0, rgba(100, 78, 54), bi, L_ALWAYS);
            lofts.push_back({bi, V2(lx - .7f, lz), V2(lx + .7f, lz), ly + .12f, x1 - lw - .05f, z0 + .3f, x1 - .3f, z1 - .3f}); blds[bi].spots.push_back({V3(x1 - lw * .5f, ly + .2f, z0 + 1.5f), LT_BARN}); }
        { int g = AS("items/GasCan"); if (g >= 0) for (int k = 0; k < 2; k++) addAProp(g, V3(x0 + 1.5f + k * .6f, .06f, z1 - .6f), fitScale(g, .45f, 1), frand(0, 6), bi); }
        { int tc = AS("items/Trashcan"); if (tc >= 0) addAProp(tc, V3(x1 - .6f, .06f, z0 + .6f), fitScale(tc, .9f, 1), 0, bi); }
        break;
    case B_SHED:
        furn(bi, x0 + w / 2, z1 - .45f, w - .8f, .55f, 1.6f, rgba(110, 96, 74), LT_SHED); furn(bi, x0 + .45f, z0 + d / 2, .6f, d * .5f, .9f, rgba(120, 100, 70), LT_SHED);
        { int g = AS("items/GasCan"); if (g >= 0 && chance(.5f)) addAProp(g, V3(x1 - .5f, .06f, z0 + .6f), fitScale(g, .45f, 1), frand(0, 6), bi); } break;
    case B_FACTORY: {
        for (float mx = x0 + 4; mx < x1 - 4; mx += 6) for (float mz = z0 + 4; mz < z1 - 3; mz += 5.5f) if (chance(.75f)) furn(bi, mx, mz, frand(2.4f, 3.6f), frand(1.4f, 2.2f), frand(1.2f, 2.2f), pick(std::vector<Color>{rgba(90, 104, 110), rgba(150, 120, 50), rgba(80, 84, 88)}), chance(.45f) ? LT_FACTORY : -1);
        for (float sz = z0 + 1.5f; sz < z1 - 1.5f; sz += 3.f) furn(bi, x1 - .6f, sz, .7f, 2.4f, 2.2f, rgba(70, 76, 84), LT_FACTORY);
        int tc = AS("items/Trashcan"); if (tc >= 0) addAProp(tc, V3(x0 + .7f, .06f, z1 - .7f), fitScale(tc, .9f, 1), 0, bi); break; }
    case B_FIRE:
        car(x0 + w * .35f, z0 + d / 2, PI / 2, 0, VK_FIRE);
        for (float sz = z0 + 1.2f; sz < z1 - 1; sz += 1.4f) furn(bi, x1 - .45f, sz, .55f, 1.1f, 1.9f, rgba(150, 36, 30), LT_FIRE);
        furn(bi, x1 - 3.f, z0 + .6f, 2.4f, .8f, .9f, metal, LT_FIRE); break;
    case B_SCHOOL:
        for (float dz = z0 + 2.5f; dz < z1 - 2; dz += 1.8f) for (float dx = x0 + 2.5f; dx < x0 + w * .6f; dx += 1.6f) furn(bi, dx, dz, .8f, .55f, .75f, rgba(150, 120, 80), chance(.12f) ? LT_SCHOOL : -1);
        for (float lz = z0 + 1; lz < z1 - 1; lz += 1.f) furn(bi, x1 - .35f, lz, .45f, .9f, 1.8f, pick(std::vector<Color>{rgba(60, 90, 140), rgba(150, 50, 40), rgba(60, 110, 70)}), chance(.35f) ? LT_SCHOOL : -1);
        furn(bi, x0 + w * .75f, z0 + 1.2f, 1.8f, .8f, .8f, wood, LT_SCHOOL); break;
    case B_TRAILER:
        if (!furnA(bi, "interior/Bed_Single", x1 - 1.0f, z0 + d / 2, 1, LT_TRAILER)) furn(bi, x1 - 1.2f, z0 + d / 2, 2.0f, 1.0f, .6f, rgba(120, 100, 90), LT_TRAILER);
        furn(bi, x0 + 1.2f, z0 + .4f, 2.0f, .6f, .9f, rgba(170, 160, 140), LT_KITCHEN); furn(bi, x0 + w * .45f, z1 - .5f, 2.2f, .8f, .5f, rgba(110, 80, 70), LT_TRAILER); break;
    case B_GAS:
        for (float sz = z0 + 2.f; sz < z1 - 1.5f; sz += 2.f) furn(bi, x0 + w * .4f, sz, w * .45f, .5f, 1.4f, metal, LT_GAS);
        furn(bi, x1 - 1.2f, z0 + 1.0f, 1.6f, .8f, 1.0f, wood, LT_GAS); furn(bi, x1 - .5f, z1 - 1.5f, .6f, 2.0f, 1.8f, rgba(220, 220, 225), LT_GAS); break;
    case B_BARRACKS:
        for (float bx = x0 + 1.2f; bx < x1 - 1; bx += 2.2f) { if (!furnA(bi, "interior/Bed_Bunk", bx, z0 + 1.2f, 0, LT_MIL)) furn(bi, bx, z0 + 1.3f, 1.0f, 2.0f, .6f, rgba(80, 90, 70), LT_MIL);
            if (!furnA(bi, "interior/Bed_Bunk", bx, z1 - 1.2f, 2, chance(.5f) ? LT_MIL : -1)) furn(bi, bx, z1 - 1.3f, 1.0f, 2.0f, .6f, rgba(80, 90, 70), chance(.5f) ? LT_MIL : -1); }
        break;
    case B_CHURCH: for (float bz = z0 + 2; bz < z1 - 2; bz += 1.8f) { furn(bi, x0 + w * .3f, bz, w * .3f, .5f, .5f, wood, chance(.2f) ? LT_HOUSE : -1); furn(bi, x0 + w * .7f, bz, w * .3f, .5f, .5f, wood, -1); } break;
    default: break;
    }
    return bi;
}
static void aoRect(float x0, float z0, float x1, float z1, float R, float k);
// wrecks: real vehicle models picked by place, burned-out ones charred, some with parts scattered around
static void car(float x, float z, float yaw, float burnP = .25f, int kind = VK_CIVIL) {
    if (kind == VK_CIVIL) { float r = frand(0, 1); kind = r < .03f ? VK_POLICE : r < .05f ? VK_DELIVERY : r < .14f ? VK_TRUCK : VK_CIVIL; }
    const char* key; float L;
    switch (kind) {
    case VK_POLICE: key = "vehicles/police"; L = 4.8f; break; case VK_AMBULANCE: key = "vehicles/ambulance"; L = 6.2f; break; case VK_FIRE: key = "vehicles/firetruck"; L = 9.f; break;
    case VK_DELIVERY: key = chance(.5f) ? "vehicles/delivery" : "vehicles/delivery-flat"; L = 6.6f; break; case VK_GARBAGE: key = "vehicles/garbage-truck"; L = 8.5f; break;
    case VK_TRACTOR: key = chance(.7f) ? "vehicles/tractor" : "vehicles/tractor-shovel"; L = 4.2f; break; case VK_MILITARY: key = chance(.6f) ? "vehicles/truck" : "vehicles/truck-flat"; L = 6.4f; break;
    case VK_TRUCK: key = chance(.75f) ? "vehicles/truck" : "vehicles/van"; L = 5.4f; break;
    default: { static const char* civ[] = {"vehicles/sedan", "vehicles/sedan", "vehicles/sedan-sports", "vehicles/hatchback-sports", "vehicles/suv", "vehicles/suv", "vehicles/suv-luxury", "vehicles/van", "vehicles/taxi"}; key = civ[irand(0, 8)]; L = 4.6f; }
    }
    int ai = AS(key); bool burn = chance(burnP);
    if (ai < 0) { // fallback box car
        float Wd = 1.9f; L = 4.4f; Vector2 f = polar(yaw, 1); float hx = fabsf(f.x) * L / 2 + fabsf(f.y) * Wd / 2, hz = fabsf(f.y) * L / 2 + fabsf(f.x) * Wd / 2;
        if (!boxFree(x - hx, z - hz, x + hx, z + hz)) return; Color c = burn ? rgba(46, 40, 38) : pick(CAR_C);
        addProp(M_CUBE, V3(x, .55f, z), V3(L, .8f, Wd), yaw, c); addProp(M_CUBE, V3(x - f.x * .3f, 1.2f, z - f.y * .3f), V3(2.2f, .6f, Wd * .9f), yaw, burn ? rgba(24, 22, 20) : rgba(30, 36, 44));
        addSolid(x - hx, z - hz, x + hx, z + hz, 1.6f, SK_CAR); carObjs.push_back({V2(x, z), yaw, burn}); return; }
    const Asset& A = assets[ai]; bool alongZ = A.size.z >= A.size.x; float sc = L / std::max(.1f, alongZ ? A.size.z : A.size.x), my = alongZ ? yaw - PI / 2 : yaw;
    Vector2 fp = aSize(ai, sc, my); float hx = fp.x / 2, hz = fp.y / 2; if (!boxFree(x - hx, z - hz, x + hx, z + hz)) return;
    Color tint = WHITE;
    if (kind == VK_MILITARY) tint = rgba(150, 165, 110);
    if (burn) tint = rgba(70, 58, 50); else if (chance(.35f)) tint = lerpC(tint, rgba(150, 120, 95), frand(.15f, .35f)); // rust and grime
    addAProp(ai, V3(x, 0, z), sc, my, -1, tint);
    addSolid(x - hx, z - hz, x + hx, z + hz, std::min(2.6f, A.size.y * sc), SK_CAR); aoRect(x - hx, z - hz, x + hx, z + hz, 1.2f, .45f); carObjs.push_back({V2(x, z), yaw, burn});
    if (burn || chance(.25f)) { static const char* deb[] = {"vehicles/debris-door", "vehicles/debris-tire", "vehicles/debris-bumper", "vehicles/debris-plate-a", "vehicles/debris-drivetrain-axle"};
        for (int k = 0; k < irand(1, 3); k++) { int di = AS(deb[irand(0, 4)]); if (di < 0) continue; Vector2 dp = V2(x + frand(-hx - 2, hx + 2), z + frand(-hz - 2, hz + 2)); if (!freeAt(dp.x, dp.y, .5f)) continue;
            addAProp(di, V3(dp.x, 0, dp.y), sc * frand(.9f, 1.1f), frand(0, 6), -1, burn ? rgba(80, 70, 62) : WHITE); } }
    if (burn) { paintCircle(V2(x, z), 3.5f, rgba(26, 24, 22), .55f); aoCircle(V2(x, z), 3.5f, .6f); }
    if (burn && chance(.5f)) staticLights.push_back({V3(x, 1.2f, z), rgba(255, 110, 30), 7, 1});
}
static void serviceCar(int bi, int type) { // a matching vehicle out front of police stations, clinics and stores
    const Building& b = blds[bi]; float x = (b.x0 + b.x1) / 2 + frand(-2, 2), z = b.z1 + 3.2f; int k = type == B_POLICE ? VK_POLICE : type == B_CLINIC ? VK_AMBULANCE : chance(.5f) ? VK_DELIVERY : VK_CIVIL;
    if (type != B_STORE || chance(.6f)) car(x, z, chance(.5f) ? 0 : PI, .15f, k);
}
static void addTree(Vector2 p, int kind) {
    if (!freeAt(p.x, p.y, 1.2f) || hotAt(p, .85f) >= 0) return;
    Tree t; t.p = p; t.kind = kind; t.h = kind == 0 ? frand(8, 14) : frand(6, 10); t.r = kind == 0 ? frand(1.8f, 2.8f) : frand(2.4f, 3.8f);
    { static const char* pines[] = {"nature/PineTree_1", "nature/PineTree_2", "nature/PineTree_3", "nature/PineTree_4", "nature/PineTree_5"};
      static const char* broad[] = {"nature/CommonTree_1", "nature/CommonTree_2", "nature/CommonTree_3", "nature/CommonTree_4", "nature/CommonTree_5", "nature/BirchTree_1", "nature/BirchTree_2", "nature/BirchTree_3"};
      const char* key = kind == 0 ? (chance(.06f) ? "nature/PineTree_Autumn_1" : pines[irand(0, 4)]) : (chance(.07f) ? (chance(.5f) ? "nature/CommonTree_Dead_1" : "nature/CommonTree_Dead_2") : chance(.08f) ? "nature/CommonTree_Autumn_2" : broad[irand(0, 7)]);
      int ai = AS(key); if (ai >= 0) { t.asset = ai; t.h = kind == 0 ? frand(9, 16) : frand(7, 12); t.sc = t.h / std::max(.1f, assets[ai].size.y); t.yaw = frand(0, 2 * PI); t.r = std::max(assets[ai].size.x, assets[ai].size.z) * t.sc * .42f; } }
    t.c = kind == 0 ? rgba(48 + irand(-8, 8), 80 + irand(-10, 10), 50 + irand(-8, 8)) : rgba(56 + irand(-10, 10), 84 + irand(-10, 10), 42 + irand(-8, 8));
    trees.push_back(t); tgrid[gcl(p.x)][gcl(p.y)].push_back((int)trees.size() - 1);
    addSolid(p.x - .3f, p.y - .3f, p.x + .3f, p.y + .3f, 3, SK_TRUNK);
}
static void addRoad(Vector2 a, Vector2 b, float w, bool town, int tier = 0) {
    roads.push_back({a, b, w, town, tier});
    if (tier == 0) { // paved: a wide, soft shoulder so the grass fades into the asphalt instead of stopping at a line
        curLayer = 5; paintSeg(a, b, w / 2 + 4.6f, rgba(104, 96, 78), .3f); paintSeg(a, b, w / 2 + 3.0f, rgba(100, 92, 74), .45f);
        curLayer = town ? 9 : 7; paintSeg(a, b, w / 2 + 2.2f, rgba(92, 84, 68), .6f); curLayer = -1; }
    else if (tier == 1) { curLayer = 5; paintSeg(a, b, w / 2 + 1.6f, rgba(110, 100, 80), .5f); curLayer = 7; paintSeg(a, b, w / 2 + .4f, rgba(120, 114, 100), 1.f); curLayer = -1; }
    else { curLayer = 1; paintSeg(a, b, w / 2 + 1.6f, rgba(80, 96, 56), .5f); curLayer = 5; paintSeg(a, b, w / 2 + .5f, rgba(104, 86, 62), .95f); curLayer = 8; paintSeg(a, b, w / 2 - .4f, rgba(84, 70, 52), .45f); curLayer = -1; }
}
static Vector2 nearestRoadPt(Vector2 p, float* dist = nullptr, int maxTier = 2) { Vector2 best = p; float bd = 1e9f;
    for (auto& r : roads) { if (r.tier > maxTier) continue; Vector2 ab = Vector2Subtract(r.b, r.a); float t = Clamp(Vector2DotProduct(Vector2Subtract(p, r.a), ab) / std::max(1e-4f, Vector2DotProduct(ab, ab)), 0, 1); Vector2 q = Vector2Add(r.a, Vector2Scale(ab, t)); float d = Vector2Distance(p, q); if (d < bd) { bd = d; best = q; } }
    if (dist) *dist = bd; return best; }
static void track(Vector2 from, int tier = 2, float maxLen = 450) { // dirt/gravel lane out to the nearest road, with a bend so it doesn't look ruled
    float d; Vector2 q = nearestRoadPt(from, &d); if (d < 3 || d > maxLen) return; float w = tier == 2 ? 3.4f : 4.2f;
    auto hitsBld = [&](Vector2 a, Vector2 b) { float L = Vector2Distance(a, b); for (float t = 0; t <= L; t += 1.f) { Vector2 p = Vector2Lerp(a, b, t / std::max(.01f, L)); if (Vector2Distance(p, from) < 2.5f) continue;
            for (int bi : bgrid[gcl(p.x)][gcl(p.y)]) { const Building& B = blds[bi]; float m = w / 2 + .4f; if (p.x > B.x0 - m && p.x < B.x1 + m && p.y > B.z0 - m && p.y < B.z1 + m) return true; } } return false; };
    for (int tryN = 0; tryN < 8; tryN++) { float bend = tryN == 0 ? frand(-.12f, .12f) : (tryN % 2 ? 1 : -1) * (.12f + tryN * .06f);
        Vector2 m = Vector2Add(Vector2Lerp(from, q, .5f), Vector2Scale(V2(-(q.y - from.y), q.x - from.x), bend));
        if (hitsBld(from, m) || hitsBld(m, q)) continue; addRoad(from, m, w, false, tier); addRoad(m, q, w, false, tier); return; } }
static void fence(float x0, float z0, float x1, float z1, Color c, float h = 1.1f) {
    bool horiz = fabsf(z1 - z0) < .01f; float len = horiz ? fabsf(x1 - x0) : fabsf(z1 - z0);
    { // split around roads: build only the runs that stay off the asphalt
        int n = std::max(1, (int)(len / .5f)); bool any = false, all = true; std::vector<bool> ok(n + 1);
        for (int i = 0; i <= n; i++) { float t = (float)i / n; Vector2 p = V2(x0 + (x1 - x0) * t, z0 + (z1 - z0) * t); ok[i] = roadDist(p) > .5f; any |= !ok[i]; all &= ok[i]; }
        if (any) { if (!all) { int st = -1; for (int i = 0; i <= n + 1; i++) { bool g = i <= n && ok[i]; if (g && st < 0) st = i; if (!g && st >= 0) { if (i - 1 - st >= 3) { float ta = (float)st / n, tb = (float)(i - 1) / n;
                        fence(x0 + (x1 - x0) * ta, z0 + (z1 - z0) * ta, x0 + (x1 - x0) * tb, z0 + (z1 - z0) * tb, c, h); } st = -1; } } } return; } }
    if (h > 2) addProp(M_CUBE, V3((x0 + x1) / 2, h / 2, (z0 + z1) / 2), horiz ? V3(len, h, .04f) : V3(.04f, h, len), 0, rgba(170, 170, 170), -1, L_WALLDET, TX_CHAIN); // chain-link
    else addProp(M_CUBE, V3((x0 + x1) / 2, h / 2, (z0 + z1) / 2), horiz ? V3(len, h, .08f) : V3(.08f, h, len), 0, c);
    for (float s = 0; s <= len; s += 2.5f) addProp(M_CUBE, V3(horiz ? std::min(x0, x1) + s : x0, h / 2 + .1f, horiz ? z0 : std::min(z0, z1) + s), V3(.14f, h + .2f, .14f), 0, mulC(c, .8f));
    addSolid(horiz ? std::min(x0, x1) : x0 - .06f, horiz ? z0 - .06f : std::min(z0, z1), horiz ? std::max(x0, x1) : x0 + .06f, horiz ? z0 + .06f : std::max(z0, z1), h, SK_FENCE);
}

// ------------------------------------------------------------ town generation (fixed seed -> same every run)
static void genTown(int ti) {
    const Town& T = TOWNS[ti];
    std::mt19937 saved = rng; rng.seed(T.seed);
    float S = T.kind == TK_CITY ? 46.f : T.kind == TK_TOWN ? 52.f : T.kind == TK_MILITARY ? 60.f : 48.f, rw = T.kind == TK_CITY ? 9.f : 7.f;
    int n = (int)(T.r / S) + 1;
    // ground: pavement / town dirt
    curLayer = T.kind == TK_MILITARY ? 7 : T.kind == TK_CITY ? 6 : 0; paintCircle(T.c, T.r * 1.1f, T.kind == TK_MILITARY ? rgba(110, 104, 84) : rgba(96, 96, 86), .55f); curLayer = -1;
    if (T.kind == TK_MILITARY) {
        float R = T.r; Vector2 c = T.c;
        Color fc = rgba(120, 124, 120);
        fence(c.x - R, c.y - R, c.x + R, c.y - R, fc, 2.6f); fence(c.x - R, c.y + R, c.x - 6, c.y + R, fc, 2.6f); fence(c.x + 6, c.y + R, c.x + R, c.y + R, fc, 2.6f);
        fence(c.x - R, c.y - R, c.x - R, c.y + R, fc, 2.6f); fence(c.x + R, c.y - R, c.x + R, c.y + R, fc, 2.6f);
        addRoad(V2(c.x, c.y + R + 10), V2(c.x, c.y - R + 10), 8, true); addRoad(V2(c.x - R + 10, c.y), V2(c.x + R - 10, c.y), 8, true);
        for (int i = 0; i < 4; i++) { float bx = c.x - 34 + (i % 2) * 40, bz = c.y - 40 + (i / 2) * 15; makeBuilding(bx, bz, 24, 9, B_BARRACKS, 1, i / 2 ? 1 : 0, ti, rgba(120, 124, 104), rgba(70, 76, 64)); }
        makeBuilding(c.x + 10, c.y + 10, 26, 18, B_HANGAR, 2, 2, ti, rgba(110, 116, 112), rgba(84, 90, 88));
        makeBuilding(c.x - 40, c.y + 30, 12, 9, B_POLICE, 1, 3, ti, rgba(130, 130, 116), rgba(70, 76, 64));
        for (int i = 0; i < 8; i++) { Vector2 p = V2(c.x + frand(8, 44), c.y + frand(-48, -10)); if (boxFree(p.x - .7f, p.y - .7f, p.x + .7f, p.y + .7f)) { addProp(M_CUBE, V3(p.x, .5f, p.y), V3(1.4f, 1, 1), frand(0, 3), rgba(80, 90, 60)); addSolid(p.x - .7f, p.y - .7f, p.x + .7f, p.y + .7f, 1, SK_CRATE); yardSpots.push_back({V3(p.x, 1.1f, p.y), LT_MIL}); } }
        for (int i = 0; i < 4; i++) { float tx = c.x + (i % 2 ? R - 4 : -R + 4), tz = c.y + (i / 2 ? R - 4 : -R + 4);
            for (int k = 0; k < 4; k++) addProp(M_CUBE, V3(tx + (k % 2 ? 1.2f : -1.2f), 2.5f, tz + (k / 2 ? 1.2f : -1.2f)), V3(.2f, 5, .2f), 0, rgba(80, 70, 56));
            addProp(M_CUBE, V3(tx, 5.2f, tz), V3(3.2f, .3f, 3.2f), 0, rgba(90, 76, 60)); addProp(M_CUBE, V3(tx, 6.3f, tz), V3(3.4f, .2f, 3.4f), 0, rgba(70, 76, 64)); addSolid(tx - 1.4f, tz - 1.4f, tx + 1.4f, tz + 1.4f, 5, SK_TOWER); }
        for (int i = 0; i < 6; i++) { Vector2 p = V2(c.x + frand(-R + 8, -8), c.y + frand(9, 24)); if (boxFree(p.x - 2.5f, p.y - 2, p.x + 2.5f, p.y + 2)) { int ta = AS("items/Tent"); if (ta >= 0) { float sc = fitScale(ta, 5.f, 2), yaw = chance(.5f) ? 0 : PI / 2; Vector2 fp = aSize(ta, sc, yaw); addAProp(ta, V3(p.x, 0, p.y), sc, yaw); addSolid(p.x - fp.x / 2, p.y - fp.y / 2, p.x + fp.x / 2, p.y + fp.y / 2, 2.2f, SK_TENT); yardSpots.push_back({V3(p.x, .1f, p.y + fp.y / 2 + .6f), LT_MIL}); }
                else { addProp(M_GABLE, V3(p.x, 0, p.y), V3(5, 2.4f, 4), 0, rgba(118, 124, 92)); addSolid(p.x - 2.5f, p.y - 2, p.x + 2.5f, p.y + 2, 2.4f, SK_TENT); } } }
        staticLights.push_back({V3(c.x - 10, 1, c.y + 6), rgba(255, 120, 40), 9, 2});
        for (int i = 0; i < 6; i++) car(c.x + frand(14, 44), c.y + frand(8, 30), chance(.5f) ? 0 : PI / 2, .2f, VK_MILITARY);
    } else {
        for (int g = -n; g <= n; g++) {
            float off = g * S; if (fabsf(off) > T.r) continue; float half = sqrtf(std::max(0.f, T.r * T.r - off * off));
            if (T.kind != TK_VILLAGE || g == 0) { int tr = T.kind == TK_VILLAGE ? 1 : 0; addRoad(V2(T.c.x - half, T.c.y + off), V2(T.c.x + half, T.c.y + off), rw, true, tr); addRoad(V2(T.c.x + off, T.c.y - half), V2(T.c.x + off, T.c.y + half), rw, true, tr); }
        }
        int specials[4] = {B_POLICE, B_CLINIC, B_FIRE, B_STORE}; int si = 0; bool school = T.kind == TK_VILLAGE;
        for (int gx = -n; gx < n; gx++) for (int gz = -n; gz < n; gz++) {
            float bx = T.c.x + gx * S + rw / 2 + 3, bz = T.c.y + gz * S + rw / 2 + 3, bs = S - rw - 6;
            Vector2 bc = V2(bx + bs / 2, bz + bs / 2); float dc = Vector2Distance(bc, T.c); if (dc > T.r - 6) continue;
            if (T.kind == TK_VILLAGE && !(gx == -1 || gx == 0) && !(gz == -1 || gz == 0)) continue;
            bool core = dc < T.r * .45f;
            if (T.kind == TK_CITY && core) {
                // dense: 2-3 commercial / apartment buildings
                if (si < 4 && chance(.5f)) { int t = specials[si++]; int sb = makeBuilding(bx + 2, bz + 2, bs * .5f, bs * .42f, t, 1, 1, ti, t == B_CLINIC ? rgba(210, 212, 210) : t == B_POLICE ? rgba(120, 130, 150) : t == B_FIRE ? rgba(170, 70, 60) : rgba(170, 160, 140), rgba(70, 72, 76)); if (sb >= 0 && t != B_FIRE) serviceCar(sb, t); }
                else makeBuilding(bx + 2, bz + 2, bs * .5f, bs * .42f, chance(.5f) ? B_STORE : B_HOUSE, chance(.5f) ? 2 : 1, 1, ti, pick(HOUSE_WALL), rgba(70, 72, 76));
                makeBuilding(bx + bs * .58f, bz + 1, bs * .4f, bs - 2, chance(.6f) ? B_APART : B_OFFICE, irand(3, 6), 2, ti, rgba(150 + irand(-20, 20), 140 + irand(-20, 20), 130 + irand(-20, 20)), rgba(64, 66, 70));
                makeBuilding(bx + 2, bz + bs * .5f, bs * .5f, bs * .45f, chance(.4f) ? B_STORE : B_GARAGE, 1, 1, ti, pick(HOUSE_WALL), rgba(70, 72, 76));
                curLayer = 9; paintRect(bx, bz, bx + bs, bz + bs, rgba(84, 84, 82), .8f, .04f); curLayer = -1;
            } else {
                // houses with yards
                if (si < 4 && chance(.3f) && T.kind != TK_VILLAGE) { int t = specials[si++]; int sb = makeBuilding(bx + 3, bz + 3, bs * .6f, bs * .5f, t, 1, 1, ti, t == B_CLINIC ? rgba(210, 212, 210) : t == B_FIRE ? rgba(170, 70, 60) : rgba(140, 140, 150), rgba(70, 72, 76)); if (sb >= 0 && t != B_FIRE) serviceCar(sb, t); continue; }
                if (!school && chance(.25f)) { if (makeBuilding(bx + 3, bz + 3, bs * .78f, bs * .5f, B_SCHOOL, 1, 1, ti, rgba(170, 120, 90), rgba(70, 72, 76)) >= 0) { school = true; for (int k = 0; k < 3; k++) crate(V2(bx + 4 + k * 3.f, bz + bs * .5f + 6), LT_SCHOOL); continue; } }
                if ((T.kind == TK_CITY && dc > T.r * .6f && chance(.35f)) || (T.kind == TK_TOWN && chance(.12f))) { // industrial lot
                    int fb = makeBuilding(bx + 3, bz + 3, bs * .72f, bs * .55f, B_FACTORY, 2, 1, ti, rgba(140, 140, 136), rgba(80, 84, 88));
                    if (fb >= 0) { for (int k = 0; k < irand(2, 5); k++) crate(V2(bx + 4 + frand(0, bs * .7f), bz + bs * .55f + frand(5, bs * .4f)), chance(.5f) ? LT_FACTORY : LT_GARAGE); if (chance(.6f)) car(bx + bs * .8f, bz + bs * .75f, PI / 2, .15f, chance(.5f) ? VK_DELIVERY : VK_TRUCK);
                        curLayer = 9; paintRect(bx + 1, bz + 1, bx + bs - 1, bz + bs - 1, rgba(84, 84, 82), .7f, .04f); curLayer = -1; continue; } }
                if (T.kind == TK_TOWN && chance(.08f)) { makeBuilding(bx + 6, bz + 6, 10, 18, B_CHURCH, 1, 1, ti, rgba(210, 205, 196), rgba(60, 60, 64)); continue; }
                for (int hx = 0; hx < 2; hx++) for (int hz = 0; hz < 2; hz++) {
                    if (chance(.25f)) continue;
                    float lx = bx + hx * bs / 2, lz = bz + hz * bs / 2, ls = bs / 2;
                    float hw = frand(9, 12), hd = frand(7.5f, 9.5f); if (hw > ls - 2) hw = ls - 2; if (hd > ls - 2) hd = ls - 2;
                    int bi = makeBuilding(lx + (ls - hw) / 2, lz + (ls - hd) / 2, hw, hd, chance(.12f) ? B_GARAGE : B_HOUSE, chance(.25f) ? 2 : 1, hz == 0 ? 0 : 1, ti, pick(HOUSE_WALL), pick(ROOF_C));
                    if (bi >= 0 && chance(.12f)) woodpile(V2(lx + ls - 2.2f, lz + ls - 1.6f), -1);
                    if (bi >= 0 && chance(.3f)) { float sx = chance(.5f) ? lx + 1.2f : lx + ls - 4.6f, sz = hz == 0 ? lz + ls - 4.8f : lz + 1.2f; makeBuilding(sx, sz, 3.2f, 3.4f, B_SHED, 1, hz == 0 ? 0 : 1, ti, rgba(130, 110, 86), rgba(70, 64, 60)); }
                    if (bi >= 0 && chance(.45f)) { Color fc = blds[bi].cond == 0 && chance(.5f) ? rgba(226, 222, 212) : rgba(150, 130, 100); // white picket for the tidy ones
                        float gx = blds[bi].doors.empty() ? lx + ls / 2 : blds[bi].doors[0].out.x; auto gated = [&](float zz, float xa, float xb) { // leave a gate in line with the front door
                            if (gx - 1.6f > xa + .5f) fence(xa, zz, gx - 1.6f, zz, fc); if (gx + 1.6f < xb - .5f) fence(gx + 1.6f, zz, xb, zz, fc); if (gx + 1.6f <= xa || gx - 1.6f >= xb) fence(xa, zz, xb, zz, fc); };
                        if (hz == 0) gated(lz + .5f, lx + .5f, lx + ls - .5f); else fence(lx + .5f, lz + .5f, lx + ls - .5f, lz + .5f, fc);
                        if (hz == 1) gated(lz + ls - .5f, lx + .5f, lx + ls * .4f); else fence(lx + .5f, lz + ls - .5f, lx + ls * .4f, lz + ls - .5f, fc);
                        fence(lx + .5f, lz + .5f, lx + .5f, lz + ls - .5f, fc); }
                    { int cnd = bi >= 0 ? blds[bi].cond : 1; curLayer = cnd == 0 ? 0 : cnd == 1 ? 1 : 4; paintRect(lx + 1, lz + 1, lx + ls - 1, lz + ls - 1, rgba(70, 92, 56), cnd == 0 ? .85f : .65f); curLayer = -1;
                      if (cnd == 2) { int tc = AS("interior/Trashcan_Large"); for (int k = 0; k < irand(2, 5); k++) { Vector2 tp = V2(frand(lx + 1, lx + ls - 1), frand(lz + 1, lz + ls - 1)); if (!freeAt(tp.x, tp.y, .6f)) continue;
                          if (k == 0 && tc >= 0) addAProp(tc, V3(tp.x, 0, tp.y), fitScale(tc, 1.f, 1), frand(0, 6)); else addProp(M_SPHERE, V3(tp.x, .25f, tp.y), V3(.7f, .5f, .6f), frand(0, 6), rgba(24, 24, 26)); } } }
                }
            }
        }
        // streetlights at intersections, signs and trash
        { int sl = AS("street/Streetlight_Single"), ss = AS("street/Sign_Stop"), tc = AS("interior/Trashcan_Large");
          for (int gx = -n; gx <= n; gx++) for (int gz = -n; gz <= n; gz++) {
            if (T.kind == TK_VILLAGE && !(gx == 0 && gz == 0)) continue;
            Vector2 ip = V2(T.c.x + gx * S, T.c.y + gz * S); if (Vector2Distance(ip, T.c) > T.r - 8) continue;
            float o = rw / 2 + 1.3f; Vector2 lp = V2(ip.x + (chance(.5f) ? o : -o), ip.y + (chance(.5f) ? o : -o));
            if (sl >= 0 && freeAt(lp.x, lp.y, .6f)) { float sc = fitScale(sl, 5.6f, 1); addAProp(sl, V3(lp.x, 0, lp.y), sc, frand(0, 4) > 2 ? 0 : PI); addSolid(lp.x - .15f, lp.y - .15f, lp.x + .15f, lp.y + .15f, 5, SK_TOWER);
                if (chance(T.kind == TK_CITY ? .3f : .15f)) staticLights.push_back({V3(lp.x, 5.2f, lp.y), rgba(255, 176, 90), 15, 3}); }
            Vector2 sp = V2(ip.x - (lp.x - ip.x), ip.y + (lp.y - ip.y)); if (ss >= 0 && chance(.45f) && freeAt(sp.x, sp.y, .5f)) addAProp(ss, V3(sp.x, 0, sp.y), fitScale(ss, 2.4f, 1), frand(0, 6));
            Vector2 tp = Vector2Add(ip, V2(frand(-S * .4f, S * .4f), o * (chance(.5f) ? 1 : -1))); if (tc >= 0 && chance(.5f) && freeAt(tp.x, tp.y, .6f)) { addAProp(tc, V3(tp.x, 0, tp.y), fitScale(tc, 1.0f, 1), frand(0, 6)); addSolid(tp.x - .3f, tp.y - .3f, tp.x + .3f, tp.y + .3f, 1, SK_BARREL); }
          } }
        // street wrecks & lamps
        for (int i = 0; i < (T.kind == TK_CITY ? 22 : 8); i++) {
            int g = irand(-n, n); float along = frand(-T.r * .8f, T.r * .8f); bool hz = chance(.5f);
            float side = (chance(.5f) ? 1 : -1) * rw * .25f;
            int vk = T.kind == TK_CITY ? (chance(.04f) ? VK_FIRE : chance(.05f) ? VK_GARBAGE : chance(.04f) ? VK_AMBULANCE : VK_CIVIL) : VK_CIVIL;
            if (hz) car(T.c.x + along, T.c.y + g * S + side, chance(.5f) ? 0 : PI, .25f, vk); else car(T.c.x + g * S + side, T.c.y + along, chance(.5f) ? PI / 2 : -PI / 2, .25f, vk);
        }
        for (int i = 0; i < (T.kind == TK_CITY ? 5 : 2); i++) { Vector2 p = Vector2Add(T.c, polar(frand(0, 2 * PI), frand(5, T.r * .6f))); if (freeAt(p.x, p.y, 2)) { addProp(M_CYL, V3(p.x, 0, p.y), V3(.7f, .9f, .7f), 0, rgba(70, 50, 40)); addSolid(p.x - .35f, p.y - .35f, p.x + .35f, p.y + .35f, .9f, SK_BARREL); staticLights.push_back({V3(p.x, 1.2f, p.y), rgba(255, 120, 40), 10, 2}); } }
    }
    rng = saved;
}

static void buildChunks();
// ------------------------------------------------------------ this life's map: town positions, hot zones
static const Vector2 TOWN_DEFAULT[6] = {{820, 760}, {380, 1200}, {1250, 380}, {300, 330}, {1280, 1260}, {1360, 830}};
static void randomizeTowns() {
    for (int attempt = 0; attempt < 60; attempt++) {
        bool ok = true;
        for (int i = 0; i < 6 && ok; i++) { const float m = TOWNS[i].r + 100; bool placed = false;
            for (int t = 0; t < 1500 && !placed; t++) { Vector2 c = V2(frand(m, MAP - m), frand(m, MAP - m)); placed = true;
                for (int j = 0; j < i; j++) if (Vector2Distance(c, TOWNS[j].c) < TOWNS[i].r + TOWNS[j].r + 170) { placed = false; break; }
                if (placed) TOWNS[i].c = c; }
            ok = placed; }
        if (ok) return;
    }
    for (int i = 0; i < 6; i++) TOWNS[i].c = TOWN_DEFAULT[i];
}
static void lootSpot(Vector2 p, float y, int table) { yardSpots.push_back({V3(p.x, y, p.y), table}); }
static void crate(Vector2 p, int table) { if (!boxFree(p.x - .7f, p.y - .7f, p.x + .7f, p.y + .7f) || rectOnRoad(p.x - .7f, p.y - .7f, p.x + .7f, p.y + .7f, .3f)) return; addProp(M_CUBE, V3(p.x, .5f, p.y), V3(1.4f, 1, 1), frand(0, 3), rgba(80, 90, 60)); addSolid(p.x - .7f, p.y - .7f, p.x + .7f, p.y + .7f, 1, SK_CRATE); aoRect(p.x - .7f, p.y - .7f, p.x + .7f, p.y + .7f, .9f, .4f); lootSpot(p, 1.1f, table); }
static void tent(Vector2 p, int table) { int ta = AS("items/Tent"); if (ta < 0 || !boxFree(p.x - 2.5f, p.y - 2, p.x + 2.5f, p.y + 2) || rectOnRoad(p.x - 2.5f, p.y - 2, p.x + 2.5f, p.y + 2, .3f)) return; float sc = fitScale(ta, 5.f, 2), yaw = chance(.5f) ? 0 : PI / 2; Vector2 fp = aSize(ta, sc, yaw);
    addAProp(ta, V3(p.x, 0, p.y), sc, yaw); addSolid(p.x - fp.x / 2, p.y - fp.y / 2, p.x + fp.x / 2, p.y + fp.y / 2, 2.2f, SK_TENT); lootSpot(V2(p.x, p.y + fp.y / 2 + .6f), .1f, table); } // loot sits at the tent flap where you can reach it
static void genHotZones() {
    hotZones.clear(); int want = (int)(irand(4, 6) * AREA * .55f);
    for (int tries = 0; tries < 6000 && (int)hotZones.size() < want; tries++) {
        HotZone h; h.kind = irand(0, HZ_COUNT - 1); h.r = frand(34, 52);
        bool nearTown = chance(.35f); if (nearTown) { const Town& T = TOWNS[irand(0, 5)]; if (T.kind == TK_MILITARY) continue; h.c = Vector2Add(T.c, polar(frand(0, 2 * PI), T.r + h.r * .7f)); }
        else h.c = V2(frand(h.r + 40, MAP - h.r - 40), frand(h.r + 40, MAP - h.r - 40));
        if (h.c.x < h.r + 20 || h.c.y < h.r + 20 || h.c.x > MAP - h.r - 20 || h.c.y > MAP - h.r - 20) continue;
        if (townAt(h.c, .85f) >= 0 || hotAt(h.c, 4.f) >= 0 || roadDist(h.c) > 45) continue;
        bool nearSpawn = false; for (auto& sp : spawnPts) if (Vector2Distance(sp, h.c) < 200) nearSpawn = true; if (nearSpawn) continue;
        hotZones.push_back(h);
        // dress it
        Vector2 c = h.c; float R = h.r;
        curLayer = 10; paintCircle(c, R, rgba(64, 58, 44), .5f); curLayer = -1; for (int k = 0; k < 6; k++) { Vector2 sp = Vector2Add(c, polar(frand(0, 6), frand(0, R * .7f))); float sr = frand(3, 7); paintCircle(sp, sr, rgba(30, 26, 22), .5f); aoCircle(sp, sr, .55f); }
        auto rp = [&](float f) { return Vector2Add(c, polar(frand(0, 2 * PI), R * f * sqrtf(frand(0, 1)))); };
        float ry = 0; { float bd = 1e9f; for (auto& r : roads) { float d = segDist(c, r.a, r.b); if (d < bd) { bd = d; ry = atan2f(r.b.y - r.a.y, r.b.x - r.a.x); } } }
        switch (h.kind) {
        case HZ_CHECKPOINT: {
            Vector2 f = polar(ry, 1), nrm = V2(-f.y, f.x); Color fc = rgba(120, 124, 120);
            for (int sd = -1; sd <= 1; sd += 2) { Vector2 a = Vector2Add(c, Vector2Scale(nrm, 9.f * sd)); Vector2 e = Vector2Add(a, Vector2Scale(f, 14)); Vector2 b0 = Vector2Subtract(a, Vector2Scale(f, 14));
                if (fabsf(f.x) > fabsf(f.y)) fence(std::min(b0.x, e.x), a.y, std::max(b0.x, e.x), a.y, fc, 2.2f); else fence(a.x, std::min(b0.y, e.y), a.x, std::max(b0.y, e.y), fc, 2.2f); }
            for (int k = 0; k < 5; k++) crate(rp(.6f), k < 3 ? LT_CACHE : LT_MIL);
            for (int k = 0; k < 2; k++) tent(rp(.7f), LT_MIL);
            for (int k = 0; k < 3; k++) { Vector2 p = rp(.8f); car(p.x, p.y, ry + frand(-.4f, .4f), .5f, k == 0 ? VK_POLICE : VK_MILITARY); }
            staticLights.push_back({xz3(rp(.4f), 1.2f), rgba(255, 120, 40), 10, 2}); break; }
        case HZ_MEDEVAC: {
            float yaw = frand(0, 2 * PI); Vector2 f = polar(yaw, 1);
            if (boxFree(c.x - 6, c.y - 6, c.x + 6, c.y + 6)) {
                addProp(M_CUBE, V3(c.x, 1.3f, c.y), V3(9, 2.4f, 2.8f), yaw, rgba(66, 72, 64)); addProp(M_CUBE, V3(c.x - f.x * 7, 1.6f, c.y - f.y * 7), V3(6, .7f, .7f), yaw, rgba(60, 64, 58));
                addProp(M_CUBE, V3(c.x + f.x * 1, 2.7f, c.y + f.y * 1), V3(11, .1f, .3f), yaw + .7f, rgba(40, 40, 40)); addProp(M_CUBE, V3(c.x + f.x * 3.5f, 1.2f, c.y + f.y * 3.5f), V3(2.4f, 1.8f, 2.6f), yaw, rgba(210, 210, 200));
                addSolid(c.x - 4.5f, c.y - 4.5f, c.x + 4.5f, c.y + 4.5f, 2.4f, SK_CAR); aoRect(c.x - 4.5f, c.y - 4.5f, c.x + 4.5f, c.y + 4.5f, 3, .6f); paintCircle(c, 10, rgba(20, 18, 16), .7f); aoCircle(c, 10, .65f);
                staticLights.push_back({V3(c.x, 1.6f, c.y), rgba(255, 110, 30), 12, 1}); }
            for (int k = 0; k < 4; k++) crate(rp(.6f), k < 2 ? LT_MED : LT_CACHE); { Vector2 ap = rp(.8f); car(ap.x, ap.y, frand(0, 6), .3f, VK_AMBULANCE); }
            lootSpot(Vector2Add(c, Vector2Scale(f, 5.5f)), .1f, LT_MED); break; }
        case HZ_CONVOY: {
            Vector2 f = polar(ry, 1);
            for (int k = -3; k <= 3; k++) { Vector2 p = Vector2Add(c, Vector2Scale(f, k * 6.5f + frand(-1, 1))); car(p.x + frand(-1.5f, 1.5f), p.y + frand(-1.5f, 1.5f), ry + frand(-.6f, .6f), .8f, chance(.7f) ? VK_MILITARY : VK_CIVIL); }
            for (int k = 0; k < 4; k++) crate(rp(.7f), k < 2 ? LT_CACHE : LT_POLICE);
            break; }
        case HZ_GRAVE: {
            curLayer = 5; paintCircle(c, R * .45f, rgba(84, 66, 46), .8f); curLayer = -1;
            for (int k = 0; k < 3; k++) crate(rp(.5f), k == 0 ? LT_CACHE : LT_HOUSE);
            for (int k = 0; k < 4; k++) lootSpot(rp(.5f), .1f, k < 2 ? LT_HOUSE : LT_CACHE);
            staticLights.push_back({xz3(rp(.3f), 1.2f), rgba(255, 120, 40), 9, 2}); break; }
        case HZ_EVAC: {
            for (int k = 0; k < 4; k++) tent(rp(.7f), k == 0 ? LT_CACHE : LT_MIL); for (int k = 0; k < 2; k++) { Vector2 mp = rp(.8f); car(mp.x, mp.y, frand(0, 6), .3f, VK_MILITARY); }
            for (int k = 0; k < 4; k++) crate(rp(.6f), k < 2 ? LT_CACHE : LT_MED);
            curLayer = 9; paintCircle(c, 6, rgba(90, 90, 86), .8f, false); curLayer = -1; paintCircle(c, 4.5f, rgba(200, 190, 60), .35f, false);
            staticLights.push_back({xz3(rp(.5f), .3f), rgba(255, 50, 30), 8, 1}); break; }
        }
    }
}
// ------------------------------------------------------------ countryside (random each run)
// ------------------------------------------------------------ interior cells: upstairs and basements are separate little worlds off the east edge
struct Stair { Vector2 a, b; int bA, bB; bool down; }; static std::vector<Stair> stairs; static float cellCurX = CELLX, cellCurZ = 20;
static bool inCell(Vector2 p) { return p.x > MAP + 20; }
static int cellOf(Vector2 p) { if (!inCell(p)) return -1; return buildingAt(p); }
static void stairProps(Vector2 p, bool down, int bi, bool horiz) { // down: a dark opening with rails; up: a flight of steps
    Color wood = rgba(110, 82, 58);
    if (down) { addProp(M_CUBE, V3(p.x, .065f, p.y), horiz ? V3(2.2f, .02f, 1.0f) : V3(1.0f, .02f, 2.2f), 0, rgba(8, 8, 10), bi, L_ALWAYS);
        for (int k = 0; k < 3; k++) { float t = -.7f + k * .5f; addProp(M_CUBE, V3(p.x + (horiz ? t : 0), .055f - k * .01f, p.y + (horiz ? 0 : t)), horiz ? V3(.35f, .015f, .9f) : V3(.9f, .015f, .35f), 0, mulC(wood, .5f - k * .12f), bi, L_ALWAYS); }
        for (int sd = -1; sd <= 1; sd += 2) addProp(M_CUBE, V3(p.x + (horiz ? 0 : sd * .55f), .5f, p.y + (horiz ? sd * .55f : 0)), horiz ? V3(2.2f, .06f, .06f) : V3(.06f, .06f, 2.2f), 0, wood, bi, L_ALWAYS); }
    else for (int k = 0; k < 6; k++) { float t = -1.0f + k * .36f, hgt = .18f + k * .2f; addProp(M_CUBE, V3(p.x + (horiz ? t : 0), hgt / 2, p.y + (horiz ? 0 : t)), horiz ? V3(.36f, hgt, .95f) : V3(.95f, hgt, .36f), 0, mulC(wood, .8f + k * .04f), bi, L_ALWAYS, TX_PLANK); }
}
static int makeCell(int bi, int kind) { // kind 1 upstairs, 2 basement
    Building B = blds[bi]; float w = B.x1 - B.x0, d = B.z1 - B.z0; if (w > 20 || d > 20) return -1;
    if (cellCurZ + d > MAP - 20) { cellCurZ = 20; cellCurX += 24; } if (cellCurX + w > WORLDX - 6) return -1;
    float X = cellCurX, Z = cellCurZ; cellCurZ += 24;
    int cb;
    if (kind == 1) { upstairsGen = true; cb = makeBuilding(X, Z, w, d, B.type == B_CABIN ? B_CABIN : B_HOUSE, 1, -1, -1, rgba(200, 196, 186), rgba(70, 70, 70)); upstairsGen = false; }
    else { forceWallTex = TX_PLASTER; cb = makeBuilding(X, Z, w, d, B_GARAGE, 1, -1, -1, rgba(120, 118, 112), rgba(60, 60, 60)); forceWallTex = -1;
        if (cb >= 0) { for (auto& sp : blds[cb].spots) sp.table = LT_BASEMENT; Color sh = rgba(90, 84, 70);
            for (int k = 0; k < 2; k++) { float sz = Z + d * (.3f + .4f * k); if (freeAt(X + w - .6f, sz, .5f)) { addProp(M_CUBE, V3(X + w - .5f, .9f, sz), V3(.6f, 1.8f, 1.8f), 0, sh, cb, L_ALWAYS); addSolid(X + w - .8f, sz - .9f, X + w - .2f, sz + .9f, 1.8f, SK_FURN, cb); blds[cb].spots.push_back({V3(X + w - .5f, 1.0f, sz), LT_BASEMENT}); } }
            if (freeAt(X + 1.2f, Z + d - 1.2f, .9f)) { addProp(M_CYL, V3(X + 1.2f, 0, Z + d - 1.2f), V3(1.1f, 1.7f, 1.1f), 0, rgba(70, 70, 74), cb, L_ALWAYS); addSolid(X + .65f, Z + d - 1.75f, X + 1.75f, Z + d - .65f, 1.7f, SK_FURN, cb); } } } // water heater
    if (cb < 0) return -1;
    blds[cb].cellKind = kind; blds[cb].parent = bi; blds[cb].cond = B.cond; blds[cb].town = B.town;
    for (int t = 0; t < 400; t++) { // a spot that's clear on both floors and not in a doorway
        float rx = frand(1.0f, w - 1.0f), rz = frand(1.0f, d - 1.0f); Vector2 a = V2(B.x0 + rx, B.z0 + rz), b = V2(X + rx, Z + rz); float need = t < 200 ? 1.0f : .8f;
        if (!freeAt(a.x, a.y, need) || !freeAt(b.x, b.y, need)) continue; bool nearDoor = false; for (auto& dr : blds[bi].doors) if (Vector2Distance(dr.in, a) < 2.6f || Vector2Distance(doorObjs[dr.obj].c, a) < 2.2f) nearDoor = true; for (auto& dr : blds[cb].doors) if (Vector2Distance(doorObjs[dr.obj].c, b) < 2.2f) nearDoor = true; if (nearDoor) continue;
        bool hz = chance(.5f); stairProps(a, kind == 2, bi, hz); stairProps(b, kind == 1, cb, hz);
        stairs.push_back({a, b, bi, cb, kind == 2}); return cb; }
    return cb;
}
static void genCells() {
    stairs.clear(); cellCurX = CELLX; cellCurZ = 20; int n = (int)blds.size();
    for (int i = 0; i < n; i++) { const Building& b = blds[i]; if (!b.enter || b.cellKind) continue;
        bool home = b.type == B_HOUSE || b.type == B_CABIN; if (!home) continue;
        if (b.h > 4.f) makeCell(i, 1); // two-story: there's an upstairs
        if (chance(b.type == B_CABIN ? .2f : .32f)) makeCell(i, 2); }
}
static void genWorld() {
    randomizeTowns(); hotZones.clear(); doorObjs.clear(); lofts.clear(); covers.clear(); carObjs.clear();
    solids.clear(); props.clear(); blds.clear(); trees.clear(); roads.clear(); stairs.clear(); spawnPts.clear(); staticLights.clear(); yardSpots.clear();
    for (auto& c : sgrid) for (auto& v : c) v.clear(); for (auto& c : pgrid) for (auto& v : c) v.clear(); for (auto& c : tgrid) for (auto& v : c) v.clear(); for (auto& c : bgrid) for (auto& v : c) v.clear();
    if (gpx) UnloadImage(groundImg);
    groundImg = GenImageColor(GTEX, GTEX, rgba(70, 92, 52)); gpx = (Color*)groundImg.data;
    Image pn = GenImagePerlinNoise(GTEX / 4, GTEX / 4, irand(0, 10000), irand(0, 10000), 6.f); Color* pp = (Color*)pn.data;
    for (int y = 0; y < GTEX; y++) for (int x = 0; x < GTEX; x++) {
        float nv = pp[(y / 4) * (GTEX / 4) + x / 4].r / 255.f;
        Color c = lerpC(rgba(58, 80, 44), rgba(98, 108, 62), nv); c = lerpC(c, rgba(110, 100, 70), Clamp((nv - .7f) * 2, 0, 1) * .5f);
        gpx[y * GTEX + x] = mulC(c, .94f + frand(0, .12f));
    }
    splat.assign((size_t)SPN * SPN * SPC, 0);
    for (int y = 0; y < SPN; y++) for (int x = 0; x < SPN; x++) { // meadow: short grass, long grass where the noise is high, bare earth in the highest spots
        float nv = pp[std::min(GTEX / 4 - 1, y / 2) * (GTEX / 4) + std::min(GTEX / 4 - 1, x / 2)].r / 255.f; unsigned char* p = &splat[((size_t)y * SPN + x) * SPC];
        float lg = Clamp((nv - .42f) * 4, 0, 1), dirt = Clamp((nv - .78f) * 6, 0, 1); p[0] = (unsigned char)(255 * (1 - lg) * (1 - dirt)); p[1] = (unsigned char)(255 * lg * (1 - dirt)); p[5] = (unsigned char)(255 * dirt); }
    UnloadImage(pn);
    // highways: minimum spanning tree over this life's town positions, plus the two shortest extra links for loops
    auto edgePt = [&](int i, Vector2 toward) { // land on the end of one of the town's own grid streets
        const Town& T = TOWNS[i]; Vector2 d = Vector2Normalize(Vector2Subtract(toward, T.c)); float S = T.kind == TK_CITY ? 46.f : T.kind == TK_TOWN ? 52.f : T.kind == TK_MILITARY ? 60.f : 48.f;
        bool one = T.kind == TK_VILLAGE || T.kind == TK_MILITARY; float R = T.r * .95f; bool ew = fabsf(d.x) > fabsf(d.y);
        float off = one ? 0 : roundf((ew ? d.y : d.x) * R / S) * S; while (fabsf(off) > R * .8f) off -= copysignf(S, off);
        float along = sqrtf(std::max(0.f, R * R - off * off)); if (T.kind == TK_MILITARY) along = T.r - 10;
        return ew ? V2(T.c.x + copysignf(along, d.x), T.c.y + off) : V2(T.c.x + off, T.c.y + copysignf(along, d.y)); };
    auto hwy = [&](int i, int j) { Vector2 a = edgePt(i, TOWNS[j].c), b = edgePt(j, TOWNS[i].c); float bend = frand(-.12f, .12f);
        Vector2 m = V2((a.x + b.x) / 2 + (a.y - b.y) * bend, (a.y + b.y) / 2 + (b.x - a.x) * bend); addRoad(a, m, 8, false); addRoad(m, b, 8, false); };
    { bool in[6] = {true, false, false, false, false, false}; bool used[6][6] = {};
      for (int k = 1; k < 6; k++) { float bd = 1e9f; int bi = -1, bj = -1;
          for (int i = 0; i < 6; i++) if (in[i]) for (int j = 0; j < 6; j++) if (!in[j]) { float d = Vector2Distance(TOWNS[i].c, TOWNS[j].c); if (d < bd) { bd = d; bi = i; bj = j; } }
          in[bj] = true; used[bi][bj] = used[bj][bi] = true; hwy(bi, bj); }
      for (int extra = 0; extra < 2; extra++) { float bd = 1e9f; int bi = -1, bj = -1;
          for (int i = 0; i < 6; i++) for (int j = i + 1; j < 6; j++) if (!used[i][j]) { float d = Vector2Distance(TOWNS[i].c, TOWNS[j].c); if (d < bd) { bd = d; bi = i; bj = j; } }
          if (bi >= 0) { used[bi][bj] = used[bj][bi] = true; hwy(bi, bj); } } }
    // map-edge roads: where you can wander in from, different every life
    for (int i = 0; i < 8; i++) {
        int side = i % 4; float t = frand(120, MAP - 120); Vector2 e = side == 0 ? V2(8, t) : side == 1 ? V2(MAP - 8, t) : side == 2 ? V2(t, 8) : V2(t, MAP - 8);
        int nt = 0; float nd = 1e9f; for (int k = 0; k < 6; k++) { float d = Vector2Distance(e, TOWNS[k].c); if (d < nd) { nd = d; nt = k; } }
        { float r = frand(0, 1); int tr = r < .45f ? 0 : r < .75f ? 1 : 2; addRoad(e, edgePt(nt, e), tr == 0 ? 7 : tr == 1 ? 5.5f : 4, false, tr); } spawnPts.push_back(Vector2Lerp(e, TOWNS[nt].c, 12.f / nd));
    }
    for (int i = 0; i < 6; i++) genTown(i);
    genHotZones();
    // farms (random)
    for (int f = 0, tries = 0; f < (int)(10 * AREA) && tries < (int)(400 * AREA); tries++) {
        Vector2 p = V2(frand(80, MAP - 80), frand(80, MAP - 80));
        if (townAt(p, 1.6f) >= 0) continue; float rd = roadDist(p); if (rd < 18 || rd > 70) continue;
        float fx = p.x, fz = p.y;
        for (int k = 0; k < irand(2, 4); k++) { float w = frand(40, 90), d = frand(40, 90), ox = fx + frand(-90, 30), oz = fz + frand(-90, 30); Color fc = pick(std::vector<Color>{rgba(140, 120, 70), rgba(110, 90, 60), rgba(150, 140, 80), rgba(90, 110, 60)});
            curLayer = chance(.6f) ? 8 : chance(.5f) ? 1 : 5; for (int y = (int)(oz * PPM); y < (int)((oz + d) * PPM); y++) for (int x = (int)(ox * PPM); x < (int)((ox + w) * PPM); x++) gblend(x, y, ((y / 3) % 2) ? fc : mulC(fc, .82f), .85f); curLayer = -1; }
        int fh = makeBuilding(fx, fz, frand(8, 10), frand(7, 8), B_HOUSE, chance(.5f) ? 2 : 1, 1, -1, pick(HOUSE_WALL), pick(ROOF_C)); if (chance(.6f)) woodpile(V2(fx - 3, fz + 4), -1);
        if (makeBuilding(fx + 16, fz - 4, 14, 20, B_BARN, 1, 2, -1, rgba(130, 50, 40), rgba(70, 64, 60)) >= 0 && chance(.7f)) car(fx + 11, fz + 18, frand(0, 6), .05f, VK_TRACTOR);
        for (int k = 0; k < irand(0, 2); k++) { float sx = fx + 33 + k * 5.5f, sz = fz + 2; if (boxFree(sx - 2.4f, sz - 2.4f, sx + 2.4f, sz + 2.4f) && !rectOnRoad(sx - 2.4f, sz - 2.4f, sx + 2.4f, sz + 2.4f)) { // grain silos
            Color sc = chance(.6f) ? rgba(170, 172, 174) : rgba(150, 110, 80); float h = frand(9, 13); addProp(M_CYL, V3(sx, 0, sz), V3(4.4f, h, 4.4f), 0, sc, -1, L_ALWAYS, TX_METAL); addProp(M_SPHERE, V3(sx, h, sz), V3(4.4f, 2.4f, 4.4f), 0, mulC(sc, .9f));
            addSolid(sx - 2.1f, sz - 2.1f, sx + 2.1f, sz + 2.1f, h, SK_TOWER); aoCircle(V2(sx, sz), 4.f, .5f); } }
        if (chance(.6f)) makeBuilding(fx - 6, fz - 7.5f, 3.4f, 3.6f, B_SHED, 1, 1, -1, rgba(130, 110, 86), rgba(70, 64, 60));
        if (fh >= 0) { const Building& hb = blds[fh]; Vector2 dp = hb.doors.empty() ? V2(hb.x0 + 4, hb.z1 + 2) : hb.doors[0].out; track(Vector2Add(dp, V2(0, 2)), chance(.6f) ? 2 : 1); }
        fence(fx - 6, fz - 10, fx + 34, fz - 10, rgba(130, 110, 80)); fence(fx - 6, fz + 22, fx + 34, fz + 22, rgba(130, 110, 80));
        f++;
    }
    // forests
    for (int i = 0; i < (int)(22 * AREA); i++) {
        Vector2 c = V2(frand(0, MAP), frand(0, MAP)); float R = frand(50, 170); if (townAt(c, 1.3f) >= 0) continue;
        curLayer = chance(.75f) ? 4 : 3; paintCircle(c, R * 1.1f, rgba(46, 56, 34), .7f); curLayer = -1;
        if (chance(.5f)) { Vector2 p = Vector2Add(c, polar(frand(0, 2 * PI), R * .4f)); int cb = makeBuilding(p.x, p.y, 7, 6, B_CABIN, 1, irand(0, 3), -1, rgba(110, 82, 58), rgba(60, 54, 48));
            if (cb >= 0) { woodpile(V2(p.x - 2.2f, p.y + 3), -1); if (chance(.4f)) makeBuilding(p.x + 9, p.y + 1, 3.2f, 3.2f, B_SHED, 1, 2, -1, rgba(110, 90, 66), rgba(60, 54, 48)); track(blds[cb].doors.empty() ? V2(p.x + 3.5f, p.y + 8) : Vector2Add(blds[cb].doors[0].out, Vector2Scale(doorObjs[blds[cb].doors[0].obj].outN, 1.5f)), 2); } }
        for (int k = 0; k < (chance(.6f) ? irand(1, 2) : 0); k++) { Vector2 p = Vector2Add(c, polar(frand(0, 2 * PI), R * frand(.5f, .9f))); if (!boxFree(p.x - 2, p.y - 2, p.x + 2, p.y + 2) || roadDist(p) < 4) continue; // hunting stand
            Color wd = rgba(96, 74, 52), lg = rgba(84, 66, 48); float hh = 4.2f; for (int q = 0; q < 4; q++) addProp(M_CUBE, V3(p.x + (q % 2 ? .9f : -.9f), hh / 2, p.y + (q / 2 ? .9f : -.9f)), V3(.16f, hh, .16f), 0, lg);
            addProp(M_CUBE, V3(p.x, hh, p.y), V3(2.2f, .14f, 2.2f), 0, wd); addProp(M_CUBE, V3(p.x, hh + .5f, p.y - 1.05f), V3(2.2f, .9f, .08f), 0, mulC(wd, .9f)); addProp(M_CUBE, V3(p.x - 1.05f, hh + .5f, p.y), V3(.08f, .9f, 2.2f), 0, mulC(wd, .9f));
            addProp(M_CUBE, V3(p.x, hh + 1.6f, p.y), V3(2.4f, .1f, 2.4f), 0, rgba(70, 76, 56)); for (float ry = .4f; ry < hh; ry += .45f) addProp(M_CUBE, V3(p.x + 1.3f, ry, p.y), V3(.06f, .06f, .55f), 0, lg);
            for (int q = 0; q < 4; q++) addSolid(p.x + (q % 2 ? .8f : -1.f), p.y + (q / 2 ? .8f : -1.f), p.x + (q % 2 ? 1.f : -.8f), p.y + (q / 2 ? 1.f : -.8f), hh, SK_TRUNK);
            addProp(M_CUBE, V3(p.x - .2f, .3f, p.y + .1f), V3(.9f, .6f, .6f), .3f, rgba(70, 80, 56)); lootSpot(V2(p.x - .2f, p.y + .1f), .65f, LT_HUNT); lootSpot(V2(p.x + .3f, p.y - .4f), .1f, LT_HUNT);
            track(V2(p.x + 2.5f, p.y), 2, 260); }
        int nt = (int)(R * R * .012f);
        for (int k = 0; k < nt; k++) {
            Vector2 p = Vector2Add(c, polar(frand(0, 2 * PI), R * sqrtf(frand(0, 1))));
            if (p.x < 5 || p.y < 5 || p.x > MAP - 5 || p.y > MAP - 5 || townAt(p, 1.05f) >= 0 || roadDist(p) < 3) continue;
            addTree(p, chance(.7f) ? 0 : 1);
        }
    }
    // trailer parks: gravel loop off a road, a few singlewides in rows
    for (int k = 0, tries = 0; k < (int)(3 * AREA) && tries < 300; tries++) {
        Vector2 c = V2(frand(120, MAP - 120), frand(120, MAP - 120)); if (townAt(c, 1.5f) >= 0 || hotAt(c, 1.3f) >= 0) continue; float rd = roadDist(c); if (rd < 20 || rd > 60) continue;
        curLayer = 7; paintCircle(c, 26, rgba(120, 114, 100), .5f); curLayer = -1; int made = 0;
        for (int i = 0; i < 8; i++) { float tx = c.x - 22 + (i % 4) * 11.f, tz = c.y + (i / 4 ? 4.f : -12.f); Color tc = pick(std::vector<Color>{rgba(210, 206, 196), rgba(190, 200, 210), rgba(200, 190, 160), rgba(170, 180, 170)});
            if (chance(.8f) && makeBuilding(tx, tz, 4.6f, 8.f + frand(0, 3), B_TRAILER, 1, i / 4 ? 1 : 0, -1, tc, rgba(140, 140, 140)) >= 0) { made++; if (chance(.3f)) car(tx + 7.5f, tz + 4, PI / 2, .3f); } }
        if (made) { addRoad(V2(c.x - 24, c.y), V2(c.x + 24, c.y), 4.5f, false, 1); track(V2(c.x + 25, c.y), 1); k++; }
    }
    // pine ridge sits in forest
    for (int k = 0; k < 500; k++) { Vector2 p = Vector2Add(TOWNS[3].c, polar(frand(0, 2 * PI), frand(TOWNS[3].r * 1.05f, TOWNS[3].r * 2.4f))); if (roadDist(p) > 3) addTree(p, 0); }
    // scattered trees & rocks
    for (int k = 0; k < (int)(1400 * AREA); k++) { Vector2 p = V2(frand(5, MAP - 5), frand(5, MAP - 5)); if (townAt(p, 1.1f) >= 0 || roadDist(p) < 4) continue; if (chance(.6f)) addTree(p, irand(0, 1)); }
    { static const char* rocks[] = {"nature/Rock_1", "nature/Rock_2", "nature/Rock_3", "nature/Rock_4", "nature/Rock_Moss_1", "nature/Rock_Moss_2", "nature/Rock_Moss_3"};
      for (int k = 0; k < (int)(220 * AREA); k++) { Vector2 p = V2(frand(5, MAP - 5), frand(5, MAP - 5)); if (townAt(p, 1.1f) >= 0 || roadDist(p) < 4 || !freeAt(p.x, p.y, 2)) continue; float s = frand(.8f, 2.4f); int ra = AS(rocks[irand(0, 6)]);
        if (ra >= 0) { float sc = fitScale(ra, s * 1.4f), yaw = frand(0, 6); Vector2 fp = aSize(ra, sc, yaw); addAProp(ra, V3(p.x, -.05f, p.y), sc, yaw); addSolid(p.x - fp.x * .4f, p.y - fp.y * .4f, p.x + fp.x * .4f, p.y + fp.y * .4f, assets[ra].size.y * sc, SK_ROCK); }
        else { addProp(M_SPHERE, V3(p.x, s * .25f, p.y), V3(s * 1.4f, s * .9f, s * 1.2f), frand(0, 6), rgba(108, 106, 100)); addSolid(p.x - s * .6f, p.y - s * .5f, p.x + s * .6f, p.y + s * .5f, s * .7f, SK_ROCK); } }
      static const char* bushes[] = {"nature/Bush_1", "nature/Bush_2", "nature/BushBerries_1", "nature/Plant_1"};
      static const float FOL[3] = {.3f, .65f, 1.f}; float fol = FOL[cfg.foliage];
      for (int k = 0; k < (int)(2600 * fol * AREA); k++) { Vector2 p = V2(frand(5, MAP - 5), frand(5, MAP - 5)); int t = townAt(p, 1.0f); if ((t >= 0 && chance(.75f)) || roadDist(p) < 3 || !freeAt(p.x, p.y, .8f)) continue; int ba = AS(bushes[irand(0, 3)]); float bs = frand(.9f, 1.8f); if (ba >= 0) { addAProp(ba, V3(p.x, -.05f, p.y), fitScale(ba, bs), frand(0, 6)); if (bs > 1.2f) covers.push_back({p, bs * .55f}); } }
      { int ga = AS("nature/Grass"), gb = AS("nature/Grass_2"); if (ga >= 0 && gb >= 0) for (int k = 0; k < (int)(220 * std::max(.5f, fol) * AREA); k++) { // tall grass patches to hide in
          Vector2 c = V2(frand(20, MAP - 20), frand(20, MAP - 20)); if (townAt(c, 1.05f) >= 0 || roadDist(c) < 6) continue;
          for (int j = 0; j < irand(6, 14); j++) { Vector2 p = Vector2Add(c, polar(frand(0, 2 * PI), frand(0, 3.2f))); if (!freeAt(p.x, p.y, .5f)) continue; int g = chance(.5f) ? ga : gb;
              addAProp(g, V3(p.x, -.02f, p.y), fitScale(g, frand(1.0f, 1.4f), 1), frand(0, 6)); covers.push_back({p, .85f}); } } }
      for (int k = 0; k < (int)(500 * AREA); k++) { Vector2 p = V2(frand(5, MAP - 5), frand(5, MAP - 5)); if (townAt(p, 1.1f) >= 0 || roadDist(p) < 3 || !freeAt(p.x, p.y, 1.2f)) continue; bool log = chance(.5f); int la = AS(log ? (chance(.5f) ? "nature/WoodLog" : "nature/WoodLog_Moss") : "nature/TreeStump");
        if (la >= 0) { float sc = fitScale(la, log ? frand(2.5f, 4.f) : frand(.7f, 1.1f)), yaw = frand(0, 6); Vector2 fp = aSize(la, sc, yaw); addAProp(la, V3(p.x, -.03f, p.y), sc, yaw); addSolid(p.x - fp.x * .4f, p.y - fp.y * .4f, p.x + fp.x * .4f, p.y + fp.y * .4f, assets[la].size.y * sc, SK_ROCK); } }
      int ga = AS("nature/Grass"), gb = AS("nature/Grass_2");
      if (ga >= 0 && gb >= 0) for (int k = 0; k < (int)(9000 * fol * AREA * .75f); k++) { Vector2 p = V2(frand(5, MAP - 5), frand(5, MAP - 5)); if (townAt(p, 1.0f) >= 0 || roadDist(p) < 2.5f || pointSolid(p.x, p.y)) continue; int g = chance(.5f) ? ga : gb; addAProp(g, V3(p.x, -.02f, p.y), fitScale(g, frand(.35f, .7f), 1), frand(0, 6)); } }
    genCells();
    // highway wrecks & a few gas stations
    for (auto& r : std::vector<Road>(roads)) {
        if (r.town) continue; float len = Vector2Distance(r.a, r.b); float yaw = atan2f(r.b.y - r.a.y, r.b.x - r.a.x);
        for (float s = 20; s < len; s += frand(40, 110)) { Vector2 p = Vector2Lerp(r.a, r.b, s / len); Vector2 nrm = V2(-sinf(yaw), cosf(yaw)); car(p.x + nrm.x * frand(-2.5f, 2.5f), p.y + nrm.y * frand(-2.5f, 2.5f), yaw + frand(-.5f, .5f) + (chance(.5f) ? PI : 0), .25f, chance(.15f) ? VK_DELIVERY : VK_CIVIL); }
        if (chance(.3f) && len > 120) { Vector2 p = Vector2Lerp(r.a, r.b, .5f); Vector2 nrm = V2(-sinf(yaw), cosf(yaw)); Vector2 g = Vector2Add(p, Vector2Scale(nrm, 14)); if (townAt(g, 1.2f) < 0) { int bi = makeBuilding(g.x - 5, g.y - 4, 10, 8, B_GAS, 1, 0, -1, rgba(200, 196, 186), rgba(170, 40, 40)); if (bi >= 0) { curLayer = 9; paintCircle(g, 18, rgba(80, 80, 78), .8f, false); curLayer = -1;
                Vector2 nd = Vector2Normalize(Vector2Subtract(p, g)); bool ax = fabsf(nd.x) > fabsf(nd.y); Vector2 pc = Vector2Add(g, ax ? V2(copysignf(9.5f, nd.x), 0) : V2(0, copysignf(8.5f, nd.y))); // pump island under a canopy between the shop and the road
                float cw = ax ? 5.f : 10.f, cd = ax ? 10.f : 5.f; for (int q = 0; q < 4; q++) addProp(M_CUBE, V3(pc.x + (q % 2 ? 1 : -1) * (cw / 2 - .6f), 2.2f, pc.y + (q / 2 ? 1 : -1) * (cd / 2 - .6f)), V3(.3f, 4.4f, .3f), 0, rgba(220, 220, 220));
                addProp(M_CUBE, V3(pc.x, 4.5f, pc.y), V3(cw, .5f, cd), 0, rgba(200, 40, 36)); addProp(M_CUBE, V3(pc.x, 4.2f, pc.y), V3(cw - .4f, .1f, cd - .4f), 0, rgba(235, 235, 230));
                for (int q = 0; q < 2; q++) { float px = pc.x + (ax ? 0 : (q ? 2.f : -2.f)), pz = pc.y + (ax ? (q ? 2.f : -2.f) : 0); addProp(M_CUBE, V3(px, .1f, pz), ax ? V3(1.0f, .2f, 1.6f) : V3(1.6f, .2f, 1.0f), 0, rgba(160, 160, 156)); addProp(M_CUBE, V3(px, .85f, pz), V3(.5f, 1.5f, .5f), 0, rgba(210, 50, 40)); addSolid(px - .25f, pz - .25f, px + .25f, pz + .25f, 1.6f, SK_BARREL); }
                lootSpot(Vector2Add(pc, ax ? V2(1.2f, 0) : V2(0, 1.2f)), .1f, LT_GAS); } } }
    }
    buildChunks();
    // upload ground
    if (groundTex.id) UnloadTexture(groundTex);
    groundTex = LoadTextureFromImage(groundImg); GenTextureMipmaps(&groundTex); SetTextureFilter(groundTex, TEXTURE_FILTER_TRILINEAR);
    std::vector<Vector3> q = {V3(0, 0, 0), V3(0, 0, MAP), V3(MAP, 0, MAP), V3(0, 0, 0), V3(MAP, 0, MAP), V3(MAP, 0, 0)};
    std::vector<Vector2> uv = {V2(0, 0), V2(0, 1), V2(1, 1), V2(0, 0), V2(1, 1), V2(1, 0)};
    if (groundMesh.vaoId) UnloadMesh(groundMesh);
    groundMesh = buildMesh(q, &uv);
    groundMat = LoadMaterialDefault(); groundMat.shader = lit; groundMat.maps[MATERIAL_MAP_DIFFUSE].texture = groundTex; groundMat.maps[MATERIAL_MAP_SPECULAR].texture = detailTex;
    if (groundAtlasOk) {
        for (int k = 0; k < 3; k++) { if (splatTex[k].id) UnloadTexture(splatTex[k]);
            Image im = GenImageColor(SPN, SPN, BLANK); unsigned char* d = (unsigned char*)im.data;
            for (size_t i = 0; i < (size_t)SPN * SPN; i++) for (int c = 0; c < 4; c++) d[i * 4 + c] = splat[i * SPC + 1 + k * 4 + c];
            splatTex[k] = LoadTextureFromImage(im); UnloadImage(im); SetTextureFilter(splatTex[k], TEXTURE_FILTER_BILINEAR); SetTextureWrap(splatTex[k], TEXTURE_WRAP_CLAMP); }
        groundMat.maps[MATERIAL_MAP_NORMAL].texture = groundAtlas[0]; groundMat.maps[MATERIAL_MAP_ROUGHNESS].texture = groundAtlas[1];
        groundMat.maps[MATERIAL_MAP_OCCLUSION].texture = splatTex[0]; groundMat.maps[MATERIAL_MAP_EMISSION].texture = splatTex[1]; groundMat.maps[MATERIAL_MAP_HEIGHT].texture = splatTex[2];
    }
    // map texture
    if (mapTex.id) UnloadTexture(mapTex);
    Image mi = ImageCopy(groundImg); ImageResize(&mi, 800, 800);
    const float ms = 800.f / MAP;
    for (auto& r : roads) ImageDrawLineEx(&mi, V2(r.a.x * ms, r.a.y * ms), V2(r.b.x * ms, r.b.y * ms), r.tier == 2 ? 1 : r.town ? 2 : 3, r.tier == 2 ? rgba(120, 92, 60) : r.tier == 1 ? rgba(150, 144, 130) : rgba(50, 50, 52));
    for (auto& b : blds) ImageDrawRectangle(&mi, (int)(b.x0 * ms), (int)(b.z0 * ms), std::max(1, (int)((b.x1 - b.x0) * ms)), std::max(1, (int)((b.z1 - b.z0) * ms)), b.enter ? rgba(170, 160, 140) : rgba(120, 116, 110));
    mapTex = LoadTextureFromImage(mi); UnloadImage(mi);
}

// merge static (L_ALWAYS) props of each 32m cell into one vertex-colored mesh
static void appendMesh(std::vector<float>& V, std::vector<float>& N, std::vector<unsigned char>& Cc, const Mesh& m, const Prop& p) {
    Matrix M = TRS(p.p, p.s, p.yaw); float cy = cosf(-p.yaw), sy = sinf(-p.yaw);
    int n = m.indices ? m.triangleCount * 3 : m.vertexCount;
    for (int k = 0; k < n; k++) {
        int i = m.indices ? m.indices[k] : k;
        Vector3 v = Vector3Transform(V3(m.vertices[i * 3], m.vertices[i * 3 + 1], m.vertices[i * 3 + 2]), M);
        Vector3 nn = V3(m.normals[i * 3] / p.s.x, m.normals[i * 3 + 1] / p.s.y, m.normals[i * 3 + 2] / p.s.z);
        nn = Vector3Normalize(V3(cy * nn.x + sy * nn.z, nn.y, -sy * nn.x + cy * nn.z));
        V.push_back(v.x); V.push_back(v.y); V.push_back(v.z); N.push_back(nn.x); N.push_back(nn.y); N.push_back(nn.z);
        Cc.push_back(p.c.r); Cc.push_back(p.c.g); Cc.push_back(p.c.b); Cc.push_back(255);
    }
}
static void buildChunks() {
    for (int x = 0; x < GN; x++) for (int z = 0; z < GN; z++) { if (chunkHas[x][z]) UnloadMesh(chunkMesh[x][z]); chunkHas[x][z] = false; }
    for (int x = 0; x < GN; x++) for (int z = 0; z < GN; z++) {
        std::vector<float> V, N; std::vector<unsigned char> Cc;
        for (int pi : pgrid[x][z]) { const Prop& p = props[pi]; if (p.layer != L_ALWAYS) continue;
            if (p.asset >= 0) { Asset& a = assets[p.asset]; Matrix M = MatrixMultiply(a.m.transform, placeM(p.asset, p.p, p.s.x, p.yaw)); Matrix NM = MatrixTranspose(MatrixInvert(M));
                for (int mi = 0; mi < a.m.meshCount; mi++) { const Mesh& m = a.m.meshes[mi]; Color col0 = a.base[a.m.meshMaterial[mi]]; col0 = rgba(col0.r * p.c.r / 255, col0.g * p.c.g / 255, col0.b * p.c.b / 255); int n = m.indices ? m.triangleCount * 3 : m.vertexCount;
                    bool tx = a.tex.data && m.texcoords; const Color* tp = tx ? (const Color*)a.tex.data : nullptr;
                    for (int k = 0; k < n; k++) { int i = m.indices ? m.indices[k] : k; Vector3 v = Vector3Transform(V3(m.vertices[i * 3], m.vertices[i * 3 + 1], m.vertices[i * 3 + 2]), M);
                        Color col = col0; if (tx) { int u = (int)(fmodf(fmodf(m.texcoords[i * 2], 1) + 1, 1) * (a.tex.width - 1)), w = (int)(fmodf(fmodf(m.texcoords[i * 2 + 1], 1) + 1, 1) * (a.tex.height - 1)); Color t = tp[w * a.tex.width + u]; col = rgba(col0.r * t.r / 255, col0.g * t.g / 255, col0.b * t.b / 255); }
                        Vector3 nn = m.normals ? V3(m.normals[i * 3], m.normals[i * 3 + 1], m.normals[i * 3 + 2]) : V3(0, 1, 0);
                        nn = Vector3Normalize(V3(NM.m0 * nn.x + NM.m4 * nn.y + NM.m8 * nn.z, NM.m1 * nn.x + NM.m5 * nn.y + NM.m9 * nn.z, NM.m2 * nn.x + NM.m6 * nn.y + NM.m10 * nn.z));
                        V.push_back(v.x); V.push_back(v.y); V.push_back(v.z); N.push_back(nn.x); N.push_back(nn.y); N.push_back(nn.z); Cc.push_back(col.r); Cc.push_back(col.g); Cc.push_back(col.b); Cc.push_back(255); } }
                continue; }
            appendMesh(V, N, Cc, meshes[p.mesh], p); }
        if (V.empty()) continue;
        Mesh m{}; m.vertexCount = (int)V.size() / 3; m.triangleCount = m.vertexCount / 3;
        m.vertices = (float*)MemAlloc(V.size() * sizeof(float)); memcpy(m.vertices, V.data(), V.size() * sizeof(float));
        m.normals = (float*)MemAlloc(N.size() * sizeof(float)); memcpy(m.normals, N.data(), N.size() * sizeof(float));
        m.colors = (unsigned char*)MemAlloc(Cc.size()); memcpy(m.colors, Cc.data(), Cc.size());
        m.texcoords = (float*)MemAlloc(m.vertexCount * 2 * sizeof(float)); memset(m.texcoords, 0, m.vertexCount * 2 * sizeof(float));
        UploadMesh(&m, false); chunkMesh[x][z] = m; chunkHas[x][z] = true;
    }
}

// ============================================================ items
enum Cal { C_9MM, C_12GA, C_556, C_308, C_COUNT };
static const char* CALN[4] = {"9mm", "12ga", "5.56", ".308"};
enum ItemId { IT_BEANS, IT_APPLE, IT_CHIPS, IT_PEACHES, IT_WATER, IT_SODA, IT_BANDAGE, IT_ANTIBIO, IT_PAINKILL, IT_MORPHINE, IT_BATTERY, IT_FLARE, IT_PIPEBOMB, IT_A9, IT_A12, IT_A556, IT_A308, IT_LOG, IT_KINDLING, IT_SPLINT, IT_MATCHES, IT_RAG, IT_TAPE, IT_SEWKIT, IT_LEATHER, IT_WAX, IT_DYE_BLACK, IT_DYE_GREEN, IT_DYE_BROWN, IT_ROPE, IT_GAS, IT_COUNT };
struct ItemDef { const char* n; float food, water; int stack; Color c; const char* use; };
static const ItemDef IDEF[IT_COUNT] = {
    {"Canned Beans", 38, -2, 4, rgba(170, 90, 60), "Eat"}, {"Apple", 12, 6, 6, rgba(180, 40, 40), "Eat"}, {"Potato Chips", 20, -6, 4, rgba(220, 190, 70), "Eat"},
    {"Canned Peaches", 26, 12, 4, rgba(230, 150, 80), "Eat"}, {"Water Bottle", 0, 45, 4, rgba(120, 180, 220), "Drink"}, {"Soda Can", 4, 26, 6, rgba(200, 50, 50), "Drink"},
    {"Bandage", 0, 0, 6, rgba(230, 226, 214), "Bandage"}, {"Antibiotics", 0, 0, 4, rgba(240, 240, 240), "Take"}, {"Painkillers", 0, 0, 6, rgba(220, 220, 255), "Take"},
    {"Morphine", 0, 0, 3, rgba(200, 220, 200), "Inject"}, {"Battery", 0, 0, 4, rgba(110, 90, 180), "Swap"}, {"Road Flare", 0, 0, 6, rgba(230, 60, 40), "Light"},
    {"Pipe Bomb", 0, 0, 4, rgba(120, 124, 130), "Throw"}, {"9mm Rounds", 0, 0, 60, rgba(200, 170, 90), ""}, {"12ga Shells", 0, 0, 30, rgba(200, 60, 50), ""},
    {"5.56 Rounds", 0, 0, 60, rgba(200, 170, 90), ""}, {".308 Rounds", 0, 0, 30, rgba(200, 170, 90), ""},
    {"Firewood Log", 0, 0, 2, rgba(120, 84, 52), "Split"}, {"Kindling", 0, 0, 12, rgba(190, 150, 100), "Build fire"}, {"Splint", 0, 0, 2, rgba(200, 170, 120), "Apply"}, {"Matches", 0, 0, 40, rgba(220, 120, 60), ""},
    {"Rag", 0, 0, 6, rgba(200, 196, 180), "Bandage"}, {"Duct Tape", 0, 0, 1, rgba(160, 164, 170), "Repair"}, {"Sewing Kit", 0, 0, 1, rgba(200, 80, 90), "Repair"}, {"Leather Patch", 0, 0, 4, rgba(130, 84, 50), "Patch"},
    {"Wax Tin", 0, 0, 1, rgba(210, 190, 110), "Waterproof"}, {"Black Dye", 0, 0, 1, rgba(30, 30, 34), "Dye"}, {"Olive Dye", 0, 0, 1, rgba(90, 104, 60), "Dye"}, {"Brown Dye", 0, 0, 1, rgba(110, 76, 46), "Dye"},
    {"Rope", 0, 0, 1, rgba(180, 160, 110), ""}, {"Jerry Can", 0, 0, 1, rgba(170, 40, 30), ""}};
static const int ISZ[IT_COUNT][2] = {{1, 2}, {1, 1}, {1, 2}, {1, 2}, {1, 2}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 2}, {1, 2}, {1, 1}, {1, 1}, {1, 1}, {1, 1},
    {2, 3}, {1, 2}, {1, 3}, {1, 1}, {1, 1}, {1, 1}, {1, 2}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {2, 2}, {2, 3}}; // grid footprint of each item
static bool isAmmo(int id) { return id >= IT_A9 && id <= IT_A308; }
static int ammoItem(int cal) { return IT_A9 + cal; }

enum WKey { W_M9, W_GLOCK, W_MP5, W_SHOTGUN, W_AR15, W_M4, W_HUNT, W_COUNT };
struct WBase { const char* n; int slot, cal; float dmg, rate, spread; int pel; float spd, range; int mag; float rel; bool autoF; float kick; int snd; float len; int pierce; float dism, noise; };
static const WBase WB[W_COUNT] = {
    {"M9 Pistol", 1, C_9MM, 30, .24f, .04f, 1, 230, 60, 15, 1.1f, false, 2.5f, 0, .26f, 0, .08f, 70},
    {"Glock 17", 1, C_9MM, 27, .17f, .05f, 1, 230, 55, 17, 1.0f, false, 2.f, 0, .24f, 0, .06f, 65},
    {"MP5", 0, C_9MM, 22, .075f, .075f, 1, 240, 60, 30, 1.6f, true, 1.4f, 1, .5f, 0, .06f, 75},
    {"Pump Shotgun", 0, C_12GA, 17, .85f, .2f, 8, 200, 24, 6, 2.4f, false, 8, 2, .75f, 0, .3f, 110},
    {"AR-15", 0, C_556, 40, .11f, .035f, 1, 320, 95, 30, 1.9f, true, 2.4f, 0, .78f, 0, .14f, 120},
    {"M4A1", 0, C_556, 42, .085f, .04f, 1, 330, 95, 30, 1.8f, true, 2.6f, 0, .8f, 0, .14f, 130},
    {"Hunting Rifle", 0, C_308, 140, 1.0f, .008f, 1, 420, 150, 5, 2.6f, false, 10, 3, 1.0f, 2, .65f, 150}};
struct Weapon { int key = -1, rar = 0, cal = 0, slot = 0; float dmg, rate, spread, spd, range, rel, kick, len, dism, crit = 0, leech = 0, noise; int pel, mag, snd, pierce, clip = 0; bool autoF, burn = false; std::vector<int> aff; std::string name; };
struct Affix { const char* pre; const char* suf; const char* desc; };
static const Affix AFF[10] = {{"Vicious", nullptr, "+35% damage"}, {"Rapid", nullptr, "+30% fire rate"}, {nullptr, "of Plenty", "+60% magazine"}, {nullptr, "of Piercing", "Pierces 2 more targets"},
    {nullptr, "of Embers", "Sets the dead on fire"}, {nullptr, "of the Leech", "Heals on hit"}, {"Steady", nullptr, "Half the spread"}, {"Quick", nullptr, "40% faster reload"}, {"Brutal", nullptr, "+12% headshot chance"}, {"Butcher's", nullptr, "Rips limbs off more often"}};
static const std::vector<std::string> LEG = {"Hellbender", "Last Rites", "Widowmaker", "Gravedigger", "Sunrise", "Bad Neighbor", "Mercy", "Old Faithful", "Pale Rider", "Cascadia"};
static Weapon makeWeapon(int key, int rar) {
    const WBase& b = WB[key]; Weapon w; w.key = key; w.rar = rar; w.cal = b.cal; w.slot = b.slot; w.dmg = b.dmg; w.rate = b.rate; w.spread = b.spread; w.pel = b.pel; w.spd = b.spd; w.range = b.range;
    w.mag = b.mag; w.rel = b.rel; w.autoF = b.autoF; w.kick = b.kick; w.snd = b.snd; w.len = b.len; w.pierce = b.pierce; w.dism = b.dism; w.noise = b.noise;
    static const float rm[4] = {1, 1.08f, 1.18f, 1.32f}; w.dmg *= rm[rar];
    std::vector<int> pool; for (int i = 0; i < 10; i++) pool.push_back(i);
    for (int i = 0; i < rar; i++) { int j = irand(0, (int)pool.size() - 1), id = pool[j]; pool.erase(pool.begin() + j); w.aff.push_back(id);
        switch (id) { case 0: w.dmg *= 1.35f; break; case 1: w.rate /= 1.3f; break; case 2: w.mag = (int)roundf(w.mag * 1.6f); break; case 3: w.pierce += 2; break; case 4: w.burn = true; break;
        case 5: w.leech += .5f; break; case 6: w.spread *= .5f; break; case 7: w.rel *= .6f; break; case 8: w.crit += .12f; break; case 9: w.dism = std::min(.9f, w.dism * 2.5f + .15f); break; } }
    if (rar == 3) w.name = "\"" + pick(LEG) + "\" " + b.n;
    else { const char* pre = nullptr; const char* suf = nullptr; for (int id : w.aff) { if (!pre && AFF[id].pre) pre = AFF[id].pre; if (!suf && AFF[id].suf) suf = AFF[id].suf; }
        w.name = std::string(pre ? std::string(pre) + " " : "") + b.n + (suf ? std::string(" ") + suf : ""); }
    w.clip = irand(0, w.mag / 2); return w;
}
enum MKey { MK_FISTS, MK_KNIFE, MK_MACHETE, MK_AXE, MK_BAT, MK_CROWBAR, MK_HATCHET, MK_WRENCH, MK_PIPE, MK_COUNT };
struct MBase { const char* n; float dmg, rate, range, arc, dism, len, sta; Color c; };
static const MBase MB[MK_COUNT] = {{"Fists", 10, .45f, 1.3f, .7f, 0, 0, 6, BONE}, {"Kitchen Knife", 26, .38f, 1.5f, .7f, .15f, .35f, 7, rgba(200, 205, 210)},
    {"Machete", 55, .5f, 1.9f, 1.0f, .5f, .7f, 10, rgba(200, 205, 210)}, {"Fire Axe", 85, .85f, 2.0f, .9f, .75f, .85f, 16, rgba(190, 40, 30)}, {"Baseball Bat", 42, .55f, 2.0f, 1.1f, .1f, .85f, 11, rgba(160, 120, 80)},
    {"Crowbar", 48, .6f, 1.8f, .9f, .1f, .75f, 11, rgba(150, 40, 36)}, {"Hatchet", 50, .55f, 1.6f, .8f, .45f, .45f, 10, rgba(170, 140, 100)}, {"Pipe Wrench", 44, .62f, 1.7f, .9f, .08f, .45f, 11, rgba(90, 100, 120)}, {"Lead Pipe", 38, .55f, 1.9f, 1.f, .05f, .8f, 10, rgba(120, 124, 128)}};
static bool canChop(int mk) { return mk == MK_AXE || mk == MK_HATCHET; }
// ============================================================ clothing (DayZ style: your clothes are your storage)
enum CSlot { CS_HEAD, CS_EYES, CS_FACE, CS_TORSO, CS_OUTER, CS_HANDS, CS_LEGS, CS_SOCKS, CS_FEET, CS_BACK, CS_COUNT };
static const char* CSN[CS_COUNT] = {"HEAD", "EYES", "FACE", "SHIRT", "JACKET", "HANDS", "LEGS", "SOCKS", "FEET", "BACK"};
enum CFam { F_CIV = 1, F_WORK = 2, F_POLICE = 4, F_MIL = 8, F_FIRE = 16, F_FARM = 32, F_HUNT = 64, F_SCHOOL = 128, F_MED = 256, F_STORE = 512 };
enum CPal { PL_CASUAL, PL_WORK, PL_MIL, PL_POLICE, PL_HUNT, PL_NEUTRAL, PL_DENIM, PL_BRIGHT };
struct CDef { const char* n; int slot; float warm, water, armor, noise, vis; int gw, gh, iw, ih, fam, pal; float rare; };
static const CDef CD[] = {
    {"Beanie", CS_HEAD, .08f, .05f, 0, 1, 1, 0, 0, 1, 1, F_CIV | F_STORE | F_WORK, PL_CASUAL, 1}, {"Baseball Cap", CS_HEAD, .02f, .1f, 0, 1, 1, 0, 0, 1, 1, F_CIV | F_STORE | F_SCHOOL, PL_BRIGHT, 1},
    {"Cowboy Hat", CS_HEAD, .03f, .25f, 0, 1, 1, 0, 0, 2, 2, F_FARM | F_CIV, PL_WORK, .5f}, {"Hard Hat", CS_HEAD, 0, .2f, .12f, 1, 1.05f, 0, 0, 2, 2, F_WORK, PL_BRIGHT, .7f},
    {"Firefighter Helmet", CS_HEAD, .04f, .4f, .2f, 1, 1.05f, 0, 0, 2, 2, F_FIRE, PL_BRIGHT, .6f}, {"Combat Helmet", CS_HEAD, .02f, .2f, .3f, 1, .95f, 0, 0, 2, 2, F_MIL, PL_MIL, .25f},
    {"Blaze Orange Cap", CS_HEAD, .04f, .1f, 0, 1, 1.2f, 0, 0, 1, 1, F_HUNT, PL_BRIGHT, .8f}, {"Riot Helmet", CS_HEAD, .02f, .3f, .26f, 1, 1, 0, 0, 2, 2, F_POLICE, PL_POLICE, .3f},
    {"Sunglasses", CS_EYES, 0, 0, 0, 1, 1, 0, 0, 1, 1, F_CIV | F_STORE, PL_NEUTRAL, 1}, {"Safety Glasses", CS_EYES, 0, 0, .02f, 1, 1, 0, 0, 1, 1, F_WORK | F_MED, PL_NEUTRAL, .8f},
    {"Ski Goggles", CS_EYES, .03f, .05f, .01f, 1, 1, 0, 0, 1, 1, F_STORE | F_CIV, PL_BRIGHT, .4f}, {"Tactical Goggles", CS_EYES, .02f, .05f, .03f, 1, .97f, 0, 0, 1, 1, F_MIL | F_POLICE, PL_MIL, .3f},
    {"Bandana", CS_FACE, .02f, 0, 0, 1, 1, 0, 0, 1, 1, F_CIV | F_FARM | F_STORE, PL_BRIGHT, 1}, {"Surgical Mask", CS_FACE, 0, 0, 0, 1, 1, 0, 0, 1, 1, F_MED, PL_NEUTRAL, 1},
    {"Balaclava", CS_FACE, .1f, .05f, .02f, 1, .95f, 0, 0, 1, 1, F_MIL | F_STORE, PL_NEUTRAL, .5f}, {"Gas Mask", CS_FACE, 0, 0, .04f, 1.05f, 1, 0, 0, 2, 2, F_MIL | F_FIRE | F_POLICE, PL_MIL, .2f},
    {"T-Shirt", CS_TORSO, .04f, 0, 0, 1, 1, 2, 2, 2, 2, F_CIV | F_STORE | F_SCHOOL, PL_BRIGHT, 1}, {"Flannel Shirt", CS_TORSO, .1f, .05f, .02f, 1, 1, 2, 2, 2, 2, F_CIV | F_FARM | F_WORK, PL_CASUAL, 1},
    {"Thermal Top", CS_TORSO, .16f, 0, 0, 1, 1, 2, 2, 2, 2, F_STORE | F_HUNT | F_CIV, PL_NEUTRAL, .6f}, {"Work Shirt", CS_TORSO, .06f, .05f, .04f, 1, 1, 3, 2, 2, 2, F_WORK | F_FARM, PL_WORK, .8f},
    {"Police Shirt", CS_TORSO, .05f, 0, .02f, 1, 1, 3, 2, 2, 2, F_POLICE, PL_POLICE, .7f}, {"Hoodie", CS_OUTER, .2f, .05f, .02f, 1, 1, 3, 2, 2, 3, F_CIV | F_SCHOOL | F_STORE, PL_CASUAL, 1},
    {"Wool Sweater", CS_OUTER, .26f, .08f, .02f, 1, 1, 2, 2, 2, 3, F_CIV | F_FARM, PL_CASUAL, .8f}, {"Denim Jacket", CS_OUTER, .14f, .1f, .06f, 1, 1, 3, 2, 2, 3, F_CIV | F_STORE, PL_DENIM, .8f},
    {"Rain Jacket", CS_OUTER, .1f, .75f, .02f, 1.25f, 1.05f, 3, 3, 3, 3, F_STORE | F_CIV | F_FARM, PL_BRIGHT, .7f}, {"Leather Jacket", CS_OUTER, .18f, .3f, .15f, 1.05f, 1, 3, 2, 3, 3, F_CIV | F_STORE, PL_NEUTRAL, .45f},
    {"Puffer Jacket", CS_OUTER, .42f, .3f, .04f, 1.15f, 1.05f, 3, 3, 3, 3, F_STORE | F_CIV, PL_BRIGHT, .4f}, {"Hunting Coat", CS_OUTER, .32f, .35f, .06f, .95f, .82f, 4, 3, 3, 4, F_HUNT | F_FARM, PL_HUNT, .4f},
    {"Military Jacket", CS_OUTER, .24f, .3f, .12f, 1, .85f, 4, 3, 3, 4, F_MIL, PL_MIL, .45f}, {"Firefighter Coat", CS_OUTER, .28f, .45f, .16f, 1.1f, 1.1f, 3, 3, 3, 4, F_FIRE, PL_BRIGHT, .4f},
    {"Police Vest", CS_OUTER, .05f, .05f, .3f, 1.05f, 1, 3, 2, 3, 3, F_POLICE, PL_POLICE, .25f}, {"Plate Carrier", CS_OUTER, .05f, .05f, .4f, 1.1f, .95f, 4, 2, 3, 3, F_MIL, PL_MIL, .12f},
    {"Work Gloves", CS_HANDS, .05f, .05f, .05f, 1, 1, 0, 0, 1, 2, F_WORK | F_FARM | F_STORE, PL_WORK, 1}, {"Wool Gloves", CS_HANDS, .1f, 0, 0, 1, 1, 0, 0, 1, 2, F_CIV | F_STORE, PL_CASUAL, .9f},
    {"Leather Gloves", CS_HANDS, .07f, .1f, .06f, 1, 1, 0, 0, 1, 2, F_CIV | F_POLICE, PL_NEUTRAL, .6f}, {"Tactical Gloves", CS_HANDS, .05f, .1f, .08f, 1, .97f, 0, 0, 1, 2, F_MIL | F_POLICE, PL_MIL, .4f},
    {"Jeans", CS_LEGS, .07f, .05f, .05f, 1, 1, 2, 3, 2, 3, F_CIV | F_STORE | F_SCHOOL | F_FARM, PL_DENIM, 1}, {"Sweatpants", CS_LEGS, .1f, 0, 0, .9f, 1, 2, 2, 2, 3, F_CIV | F_SCHOOL, PL_CASUAL, 1},
    {"Cargo Pants", CS_LEGS, .08f, .1f, .05f, 1, 1, 3, 3, 2, 3, F_CIV | F_STORE | F_WORK, PL_WORK, .7f}, {"Work Pants", CS_LEGS, .08f, .1f, .08f, 1, 1, 2, 3, 2, 3, F_WORK | F_FARM, PL_WORK, .8f},
    {"Hunting Pants", CS_LEGS, .14f, .3f, .05f, .95f, .85f, 3, 3, 2, 3, F_HUNT, PL_HUNT, .5f}, {"Military Pants", CS_LEGS, .1f, .2f, .1f, 1, .87f, 3, 3, 2, 3, F_MIL, PL_MIL, .45f},
    {"Firefighter Pants", CS_LEGS, .15f, .4f, .1f, 1.1f, 1.08f, 2, 3, 2, 3, F_FIRE, PL_BRIGHT, .45f}, {"Cotton Socks", CS_SOCKS, .03f, 0, 0, 1, 1, 0, 0, 1, 1, F_CIV | F_STORE | F_SCHOOL, PL_NEUTRAL, 1},
    {"Wool Socks", CS_SOCKS, .08f, .05f, 0, 1, 1, 0, 0, 1, 1, F_CIV | F_STORE | F_HUNT | F_FARM, PL_CASUAL, .8f}, {"Sneakers", CS_FEET, .03f, 0, .01f, .85f, 1, 0, 0, 2, 2, F_CIV | F_SCHOOL | F_STORE, PL_BRIGHT, 1},
    {"Work Boots", CS_FEET, .06f, .2f, .05f, 1.05f, 1, 0, 0, 2, 2, F_WORK | F_FARM, PL_WORK, .8f}, {"Hiking Boots", CS_FEET, .08f, .35f, .04f, 1, 1, 0, 0, 2, 2, F_STORE | F_HUNT | F_CIV, PL_WORK, .6f},
    {"Rubber Boots", CS_FEET, .04f, .8f, .02f, 1.1f, 1, 0, 0, 2, 2, F_FARM | F_STORE, PL_BRIGHT, .6f}, {"Combat Boots", CS_FEET, .07f, .35f, .08f, 1, 1, 0, 0, 2, 2, F_MIL | F_POLICE, PL_NEUTRAL, .45f},
    {"Dress Shoes", CS_FEET, .02f, .05f, 0, 1.2f, 1, 0, 0, 2, 2, F_CIV, PL_NEUTRAL, .6f}, {"Drawstring Bag", CS_BACK, 0, 0, 0, 1, 1, 3, 3, 2, 2, F_CIV | F_SCHOOL | F_STORE, PL_BRIGHT, 1},
    {"School Backpack", CS_BACK, 0, .05f, 0, 1, 1, 4, 4, 3, 3, F_SCHOOL | F_CIV, PL_BRIGHT, .8f}, {"Hiking Pack", CS_BACK, 0, .2f, 0, 1.05f, 1, 5, 5, 3, 4, F_STORE | F_HUNT, PL_BRIGHT, .4f},
    {"Hunting Pack", CS_BACK, 0, .2f, 0, .95f, .9f, 5, 4, 3, 4, F_HUNT, PL_HUNT, .35f}, {"Military Pack", CS_BACK, 0, .25f, .02f, 1, .92f, 6, 6, 4, 4, F_MIL, PL_MIL, .15f}};
constexpr int CD_N = sizeof(CD) / sizeof(CD[0]);
static const char* PATN[4] = {"", "Plaid ", "Camo ", "Faded "};
static const char* CONDN[5] = {"Pristine", "Worn", "Damaged", "Badly Damaged", "Ruined"};
struct InvItem; struct Cloth;
struct InvItem { int kind = 0, id = -1, n = 1, x = 0, y = 0; bool rot = false; std::shared_ptr<Cloth> c; }; // kind 0 = item, 1 = clothing
struct Cloth { int def = 0; float cond = 1; Color col{}; int pattern = 0, rar = 0, upg = 0, extraRows = 0, set = -1; float bWarm = 0, bWater = 0, bArmor = 0, bQuiet = 0, bVis = 1; std::string name, decor; std::vector<InvItem> items; };
static int condTier(float c) { return c > .8f ? 0 : c > .55f ? 1 : c > .3f ? 2 : c > 0 ? 3 : 4; }
static int gridW(const Cloth& c) { return CD[c.def].gw; } static int gridH(const Cloth& c) { return CD[c.def].gh ? CD[c.def].gh + c.extraRows : 0; }
static Color palColor(int pal) {
    static const std::vector<Color> P[8] = {{rgba(150, 40, 40), rgba(40, 70, 120), rgba(60, 100, 70), rgba(120, 110, 90), rgba(90, 60, 110), rgba(170, 140, 60), rgba(70, 70, 76)},
        {rgba(130, 100, 60), rgba(110, 104, 90), rgba(200, 110, 40), rgba(90, 90, 92), rgba(150, 130, 90)}, {rgba(84, 92, 58), rgba(140, 126, 90), rgba(70, 74, 66)},
        {rgba(28, 36, 64), rgba(30, 30, 34)}, {rgba(84, 92, 58), rgba(110, 90, 60), rgba(220, 100, 20)}, {rgba(30, 30, 32), rgba(120, 120, 124), rgba(220, 218, 210), rgba(80, 70, 60)},
        {rgba(50, 70, 110), rgba(70, 90, 130), rgba(40, 46, 60)}, {rgba(210, 50, 40), rgba(240, 190, 40), rgba(40, 120, 200), rgba(50, 160, 80), rgba(230, 110, 30), rgba(240, 240, 236), rgba(200, 60, 140)}};
    const auto& v = P[pal]; Color c = v[irand(0, (int)v.size() - 1)]; int j = 10; return rgba(c.r + irand(-j, j), c.g + irand(-j, j), c.b + irand(-j, j)); }
static int rollRarity(int mn);
static int setOfDef(int def); static void makeSetPiece(Cloth& c, int si);
static std::shared_ptr<Cloth> makeCloth(int def, int rar, float condMax = 1) {
    auto c = std::make_shared<Cloth>(); const CDef& d = CD[def]; c->def = def; c->rar = rar; c->cond = std::max(.08f, frand(.15f, condMax)); c->col = palColor(d.pal);
    c->pattern = d.pal == PL_HUNT || d.pal == PL_MIL ? (chance(.6f) ? 2 : 0) : (d.slot == CS_TORSO || d.slot == CS_OUTER) && chance(.25f) ? 1 : chance(.15f) ? 3 : 0;
    if (c->pattern == 3) c->col = lerpC(c->col, rgba(170, 166, 156), .35f);
    static const char* deco[] = {"Cedar Falls Lumber", "Harlow High Football", "Millbrook Feed & Seed", "Fort Calloway", "Pine Ridge Volunteer FD", "Dunmore Diner", "Linn County Fair '24", "Oregon Ducks", "Beavers", "Rogue Brewing"};
    if ((d.slot == CS_TORSO || d.slot == CS_OUTER || d.slot == CS_HEAD) && chance(.3f)) c->decor = deco[irand(0, 9)];
    static const char* affN[5] = {"Insulated", "Reinforced", "Waterproofed", "Deep-Pocket", "Quiet"};
    std::string pre;
    for (int i = 0; i < rar; i++) { int a = irand(0, 4); if (a == 3 && !d.gh) a = 1; switch (a) { case 0: c->bWarm += .08f; break; case 1: c->bArmor += .06f; break; case 2: c->bWater += .25f; break; case 3: c->extraRows++; break; case 4: c->bQuiet += .1f; break; } if (i == 0) pre = std::string(affN[a]) + " "; }
    static const char* leg[] = {"Dad's", "The Sheriff's", "Lucky", "Old Glory", "Grandma's", "Bigfoot's"};
    c->name = rar == 3 ? std::string(leg[irand(0, 5)]) + " " + d.n : pre + PATN[c->pattern] + d.n;
    { int si = setOfDef(def); if (si >= 0 && chance(rar >= 2 ? .45f : rar == 1 ? .12f : .04f)) makeSetPiece(*c, si); }
    return c;
}
static float clothWarm(const Cloth& c) { return (CD[c.def].warm + c.bWarm) * (c.cond <= 0 ? .2f : .4f + .6f * c.cond); }
static float clothArmor(const Cloth& c) { return (CD[c.def].armor + c.bArmor) * (c.cond <= 0 ? 0 : .3f + .7f * c.cond); }
static float clothWater(const Cloth& c) { return std::min(.95f, (CD[c.def].water + c.bWater) * (c.cond <= 0 ? .2f : .5f + .5f * c.cond)); }
static int clothFor(int fam, int slotMask = -1) { // weighted pick of a clothing def for a loot family
    float tot = 0; for (int i = 0; i < CD_N; i++) if ((CD[i].fam & fam) && (slotMask < 0 || (slotMask >> CD[i].slot & 1))) tot += CD[i].rare; if (tot <= 0) return 0;
    float r = frand(0, tot); for (int i = 0; i < CD_N; i++) if ((CD[i].fam & fam) && (slotMask < 0 || (slotMask >> CD[i].slot & 1))) { r -= CD[i].rare; if (r <= 0) return i; } return 0; }
static int cdIndex(const char* n) { for (int i = 0; i < CD_N; i++) if (!strcmp(CD[i].n, n)) return i; return 0; }
// ---- class sets: loud colors, bonuses at 2 and 4 pieces, and extra if the set matches your class (set index == class index)
struct SetDef { const char* n; const char* pieces[5]; Color c; const char* b2; const char* b4; const char* bc; };
static const SetDef SETS[5] = {
    {"Thin Blue Line", {"Riot Helmet", "Police Shirt", "Police Vest", "Tactical Gloves", "Combat Boots"}, {40, 110, 255, 255}, "+8% armor", "+12% pistol and shotgun damage, +10% reload", "Police: Adrenaline Dump recharges 25% faster"},
    {"Golden Hour", {"Surgical Mask", "Safety Glasses", "Thermal Top", "Rain Jacket", "Cargo Pants"}, {160, 255, 40, 255}, "Bandages heal 40% more", "+15% bleed resistance, faster regen", "Paramedic: Trauma Kit recharges 25% faster"},
    {"Grease Monkey", {"Hard Hat", "Work Shirt", "Work Gloves", "Work Pants", "Work Boots"}, {255, 214, 0, 255}, "+10% melee damage", "+15% stamina, kicks 25% harder", "Mechanic: Nail Bomb recharges 25% faster"},
    {"Deep Woods", {"Blaze Orange Cap", "Hunting Coat", "Hunting Pants", "Hiking Boots", "Hunting Pack"}, {60, 255, 120, 255}, "Spotted 8% later", "+15% rifle damage, 12% quieter", "Hunter: Deadeye recharges 25% faster"},
    {"County Issue", {"Beanie", "Hoodie", "Sweatpants", "Sneakers", "Bandana"}, {255, 110, 0, 255}, "Better loot odds", "+12% melee damage, kicks 30% harder", "Ex-Con: Bloodlust recharges 25% faster"}};
static int setOfDef(int def) { for (int i = 0; i < 5; i++) for (auto p : SETS[i].pieces) if (!strcmp(p, CD[def].n)) return i; return -1; }
static void makeSetPiece(Cloth& c, int si) { c.set = si; c.col = SETS[si].c; c.pattern = 0; c.decor.clear(); c.rar = std::max(c.rar, 2); c.cond = std::max(c.cond, .6f); c.name = std::string(SETS[si].n) + " " + CD[c.def].n; }

enum LootKind { LK_ITEM, LK_GUN, LK_MELEE, LK_CLOTH };
struct Loot { int kind, id, count; Weapon w; Vector3 p, v; float rot, t = 0; bool settled = true; std::shared_ptr<Cloth> c; };
static std::vector<Loot> loot;
static bool playerFemale = false;
static int gunAsset(int key) { static const char* k[W_COUNT] = {"items/Pistol_1", "items/Pistol_2", "items/Shotgun_ShortStock", "items/Shotgun_1", "items/Shotgun_1", "items/Shotgun_ShortStock", "items/Shotgun_2"}; return AS(k[key]); }
static float gunScale(int key, int ai) { static const float L[W_COUNT] = {.24f, .22f, .6f, 1.0f, .95f, .85f, 1.15f}; return fitScale(ai, L[key], 0); }
static int meleeAsset(int key) { if (key == MK_KNIFE || key == MK_MACHETE) return AS("items/Knife"); if (key == MK_AXE || key == MK_HATCHET) return AS("items/Axe"); return -1; }
static float meleeScale(int key, int ai) { return fitScale(ai, MB[key].len); }
static Matrix meleeAlign(int ai) { Vector3 s = assets[ai].size; return s.y >= s.x && s.y >= s.z ? MatrixRotateZ(-PI / 2) : (s.z > s.x ? MatrixRotateY(PI / 2) : MatrixIdentity()); }
static int itemAsset(int id) {
    switch (id) { case IT_BEANS: return AS("items/Can_Closed"); case IT_PEACHES: case IT_SODA: return AS("items/Can_Red"); case IT_WATER: return AS("items/WaterBottle_1"); case IT_BANDAGE: return AS("items/Bandages");
        case IT_MORPHINE: case IT_ANTIBIO: return AS("items/FirstAidKit"); case IT_BATTERY: return AS("items/Battery_Small"); case IT_PIPEBOMB: return AS("items/PropaneTank"); default: return -1; }
}
static float itemScale(int id, int ai) { float t = id == IT_WATER ? .26f : id == IT_MORPHINE || id == IT_ANTIBIO ? .3f : id == IT_PIPEBOMB ? .3f : id == IT_BANDAGE ? .14f : id == IT_BATTERY ? .09f : id == IT_SODA ? .12f : .13f; return fitScale(ai, t); }

// ============================================================ entities
enum ZType { Z_WALKER, Z_RUNNER, Z_BRUTE, Z_BLOATER, Z_VOLATILE, Z_SOLDIER };
struct ZDef { float hp, spd, r, dmg, h; Color skin, shirt, pants; float xp; };
static const ZDef ZT[6] = {
    {55, 1.5f, .34f, 9, 1.75f, rgba(150, 160, 130), rgba(90, 96, 110), rgba(60, 64, 76), 10},
    {40, 5.0f, .3f, 7, 1.7f, rgba(170, 150, 128), rgba(120, 50, 50), rgba(70, 60, 54), 14},
    {480, 2.0f, .55f, 26, 2.25f, rgba(130, 110, 120), rgba(60, 64, 72), rgba(50, 50, 56), 70},
    {80, 1.1f, .5f, 6, 1.8f, rgba(160, 190, 90), rgba(120, 140, 60), rgba(80, 90, 50), 22},
    {170, 7.2f, .32f, 20, 1.85f, rgba(214, 220, 226), rgba(150, 156, 164), rgba(120, 126, 134), 50},
    {110, 1.8f, .34f, 12, 1.8f, rgba(140, 150, 124), rgba(84, 96, 70), rgba(76, 86, 64), 24}};
enum ZJob { J_NONE, J_MEDIC, J_COP, J_FIRE, J_HUNTER, J_MECH, J_HIKER, J_COUNT }; // what they were before. Shows in their clothes, and in their pockets
static const Color JOBC[J_COUNT] = {{0, 0, 0, 0}, {225, 232, 240, 255}, {34, 44, 80, 255}, {200, 170, 60, 255}, {230, 110, 30, 255}, {70, 96, 130, 255}, {170, 50, 40, 255}};
static float gExposure = 0; // highest detection meter among zombies near you (drives the HIDDEN/NOTICED colour)
struct Zombie {
    int job = 0;
    int id, type; Vector2 p, k{}; float r, hp, mhp, spd, dmg, atk = .4f, seed, a, aggro = 0; int side;
    float burn = 0, slow = 0, flash = 0, chg = 0, chgT = 0, ca = 0, lureT = 0; Vector2 lp{}, np{}; float noiseT = 0, wt = 0, wa, stun = 0, bleed = 0, losT = 0, walk = 0; bool sees = false;
    bool armL = true, armR = true, dead = false, gone = false; int via = 0; int look = -1;
    bool inSight = false; float notice = 0; // detection build-up: 0 unseen, 1 spotted
    bool hordeM = false; int hstate = 0; bool chasing = false; int pound = -1; float poundT = 0, ignoreT = 0; // horde marchers: 0 marching, 1 searching your last known spot
};
struct Corpse { Vector2 p; float yaw; int type; bool armL, armR, head; float t; int look = -1; float seed = 0; };
enum PKind { P_BLOOD, P_SPARK, P_FIRE, P_SMOKE, P_MIST, P_DUST, P_ACID, P_CASING, P_RAIN };
struct Particle { Vector3 p, v; float t, mt, s; Color c; int kind; };
enum GibKind { G_LIMB, G_HEAD, G_CHUNK, G_RIB, G_GUTS };
struct Gib { Vector3 p, v, axis; float rot, rotv; int kind; Color c; Vector3 s; float t, trail; bool acid, rest; };
struct Bullet { Vector2 p, v; float dmg, life; int pierce; int hits[8]; int nh = 0; bool burn, shotgun; float leech, crit, dism; };
struct Bomb { Vector3 p, v; float t, beep = 0, blink = 0; bool flare; };
struct AcidPool { Vector2 p; float r, t; };
struct Decal { Vector2 p; float r, rot, a; int v; Color c; float grow, y; };
struct Floater { Vector3 p; std::string txt; Color c; float size, t, mt; };
struct Light { Vector3 p; float r, t, mt; Color c; };
struct ScreenDrop { Vector2 p; float r, t, mt; };
static std::vector<Zombie> zombies; static std::vector<Corpse> corpses; static std::vector<Particle> parts; static std::vector<Gib> gibs;
static std::vector<Bullet> bullets; static std::vector<Bomb> bombs; static std::vector<AcidPool> acid; static std::vector<Decal> decals; static int decalHead = 0;
static std::vector<Floater> floaters; static std::vector<Light> lights; static std::vector<ScreenDrop> sdrops;

// ============================================================ player
struct Perks { float dmg = 1, rate = 1, crit = .06f, metab = 1, sta = 1, luck = 1, blast = 1, cold = 1, melee = 1, noise = 1, medic = 1; float regen = 0;
    float maxhp = 100, reload = 1, armor = 0, speed = 1, pistol = 1, shotgun = 1, rifle = 1, stealth = 0, cdr = 0, ammo = 1, batt = 1, bleedres = 0, block = 0, kick = 1; };
struct InvSlot { int id, n; };
struct Player {
    Vector2 p{0, 0}, k{}, rv{}; float a = 0, hp = 100, food = 80, water = 80, temp = 37.f, sta = 100, staD = 0, roll = 0, rollCd = 0, iframe = 0, hurt = 0;
    int bleeds = 0; float sick = 0, pain = 0, shiver = 0; bool broken = false;
    Weapon gun[2]; bool hasGun[2] = {false, false}; int melee = MK_FISTS; int active = 2;
    std::shared_ptr<Cloth> eq[CS_COUNT]; float wet = 0;
    float batt = 60, xp = 0, next = 0; int lvl = 1; bool light = false, uv = false, sprint = false, crouch = false, fired = false;
    float reload = 0, cd = 0, step = 0, swing = 0, swingCd = 0, swingA = 0, walked = 0, alive = 0; Perks perks;
    int cls = 0; int rank[3][4] = {}; int points = 0; float actCd = 0, actT = 0;
    bool splinted = false; float splintT = 0; int loft = -1; float moveA = 0; bool moving = false; float hideK = 1, firedT = 0;
    bool blocking = false, autorun = false; float shieldHP = 0, kickCd = 0, kickT = 0, autoA = 0, blockFx = 0;
    Vector2 vel{}; bool back = false; float rmbT = 0, shoveCd = 0, vaultT = 0, vaultH = 0; Vector2 vaultA{}, vaultB{}; float animA = 0;
};
static Player P;

// ============================================================ state
enum State { ST_TITLE, ST_PLAY, ST_SKILLS, ST_PAUSE, ST_DEAD, ST_INV, ST_MAP, ST_SETTINGS };
static State state = ST_TITLE;
static float gTime = 0, shake = 0, spawnT = 0, groanCd = 0, deadT = 0, rainLevel = 0, rainTarget = 0, rainT = 0;
static int zid = 0, kills = 0, nights = 0, pendingLv = 0, hordeDay = 0, lastTown = -2, bestRar = -1, gibCount = 0;
static char phase = 'd'; static std::string bestName;
struct Horde { bool on = false; float a = 0, t = 0; int left = 0; Vector2 end{}, lastSeen{}; } horde;
struct Fire { Vector2 p; float fuel, smokeT = 0, lureT = 0; bool warned = false; }; static std::vector<Fire> fires;
struct Banner { float t = 0; std::string a, b; Color c; } banner;
struct Msg { std::string t; Color c; float life; };
static std::vector<Msg> msgs;
static Vector2 camPos{MAP / 2, MAP / 2}; static float camZoom = 1.f;
static bool muted = false;
static int nearLoot = -1; static bool uiOpen = false;
static int bestHours = 0, bestKills = 0;
constexpr float CYCLE = 720.f; // 12 min day
static float dayT() { return fmodf(gTime, CYCLE) / CYCLE; }   // 0 = 06:00
static int dayN() { return (int)(gTime / CYCLE) + 1; }
static float hourOf() { return fmodf(6 + dayT() * 24, 24); }
static bool isNight() { float h = hourOf(); return h >= 20.5f || h < 5.5f; }
static bool inBasement = false;
static float smoothstep01(float a, float b, float x) { float t = Clamp((x - a) / (b - a), 0, 1); return t * t * (3 - 2 * t); }
static float darkK() { if (inBasement) return 1; float h = hourOf(); return h >= 12 ? smoothstep01(18.5f, 21.f, h) : 1 - smoothstep01(4.5f, 7.f, h); } // one eased curve, no corners
static void msg(const std::string& t, Color c = BONE) { msgs.push_back({t, c, 5}); if (msgs.size() > 6) msgs.erase(msgs.begin()); }
static void showBanner(const std::string& a, const std::string& b, Color c = SODIUM) { banner = {4.5f, a, b, c}; }
static float xpNeed(int l) { return roundf(80 * powf((float)l, 1.5f)); }

// ============================================================ audio (synthesized)
enum SfxId { SFX_PISTOL, SFX_SMG, SFX_SHOTGUN, SFX_RIFLE, SFX_HIT, SFX_BOOM, SFX_SPLAT, SFX_PICK, SFX_LOOT, SFX_LEGEND, SFX_HURT, SFX_BEEP, SFX_RELOAD, SFX_ROLL,
    SFX_GROAN, SFX_SCREAM, SFX_HORN, SFX_LEVEL, SFX_THROW, SFX_ROAR, SFX_EMPTY, SFX_GORE, SFX_SWING, SFX_CHOP, SFX_RAIN, SFX_WIND, SFX_EAT, SFX_STEP, SFX_DOOR_OPEN, SFX_DOOR_CLOSE, SFX_DOOR_BANG, SFX_CREAK, SFX_FIRE,
    SFX_HIT_WOOD, SFX_HIT_SLICE, SFX_HIT_HEAVY, SFX_HIT_PUNCH, SFX_ZATTACK, SFX_ZDEATH, SFX_THUNDER, SFX_STING, SFX_DOG, SFX_BLOCK, SFX_CAR_DOOR, SFX_DOOR_METAL, SFX_COUNT };
constexpr int ALIASES = 6;
static Sound snd[SFX_COUNT][ALIASES]; static int sndNext[SFX_COUNT]; static bool audioOk = false; static std::vector<Sound> bank[SFX_COUNT];
struct Synth {
    std::vector<float> d; int sr = 44100;
    explicit Synth(float dur) : d((size_t)(dur * 44100), 0.f) {}
    void noise(float t0, float dur, float vol, float lp, float hp = 0, float decay = 6) {
        float yl = 0, yh = 0, xp = 0, al = 1 - expf(-2 * PI * lp / sr), ah = hp > 0 ? expf(-2 * PI * hp / sr) : 0;
        for (int i = (int)(t0 * sr); i < (int)((t0 + dur) * sr) && i < (int)d.size(); i++) { float t = (i - t0 * sr) / sr, x = frand(-1, 1); yl += al * (x - yl); float y = yl; if (hp > 0) { yh = ah * (yh + y - xp); xp = y; y = yh; } d[i] += y * vol * expf(-t * decay / dur); }
    }
    void tone(float t0, float dur, float f1, float f2, float vol, int wave = 0, float decay = 5, float attack = 0) {
        float ph = 0;
        for (int i = (int)(t0 * sr); i < (int)((t0 + dur) * sr) && i < (int)d.size(); i++) { float t = (i - t0 * sr) / sr, f = f1 * powf(f2 / f1, t / dur); ph += f / sr;
            float s = wave == 0 ? sinf(ph * 2 * PI) : wave == 1 ? (fmodf(ph, 1) * 2 - 1) : wave == 2 ? (fmodf(ph, 1) < .5f ? 1.f : -1.f) : (fabsf(fmodf(ph, 1) * 4 - 2) - 1);
            float env = expf(-t * decay / dur); if (attack > 0 && t < attack) env *= t / attack; d[i] += s * vol * env; }
    }
    Sound make() { std::vector<short> s(d.size()); for (size_t i = 0; i < d.size(); i++) s[i] = (short)(Clamp(tanhf(d[i] * 1.2f), -1, 1) * 30000);
        Wave wv{}; wv.frameCount = (unsigned)s.size(); wv.sampleRate = sr; wv.sampleSize = 16; wv.channels = 1; wv.data = s.data(); return LoadSoundFromWave(wv); }
};
static void buildAudio() {
    InitAudioDevice(); audioOk = IsAudioDeviceReady(); if (!audioOk) return;
    for (int id = 0; id < SFX_COUNT; id++) {
        Synth s(.1f);
        switch (id) {
        case SFX_PISTOL: s = Synth(.35f); s.noise(0, .3f, .9f, 4000, 250, 7); s.tone(0, .16f, 170, 50, .7f, 0, 6); s.noise(.05f, .3f, .15f, 1200, 0, 3); break;
        case SFX_SMG: s = Synth(.16f); s.noise(0, .12f, .7f, 5000, 400, 6); s.tone(0, .08f, 190, 70, .5f); break;
        case SFX_SHOTGUN: s = Synth(.7f); s.noise(0, .6f, 1.1f, 2200, 70, 6); s.tone(0, .35f, 120, 38, .9f, 0, 5); break;
        case SFX_RIFLE: s = Synth(1.f); s.noise(0, .9f, 1.1f, 3000, 50, 7); s.tone(0, .45f, 95, 30, 1.f); break;
        case SFX_HIT: s = Synth(.1f); s.noise(0, .08f, .6f, 1100, 150, 5); s.tone(0, .06f, 200, 90, .3f, 3); break;
        case SFX_BOOM: s = Synth(2.f); s.noise(0, 1.9f, 1.4f, 500, 0, 4); s.tone(0, 1.4f, 85, 22, 1.2f, 0, 4); break;
        case SFX_SPLAT: s = Synth(.5f); s.noise(0, .45f, 1.f, 700, 0, 5); s.tone(0, .3f, 150, 50, .4f, 1, 5); break;
        case SFX_PICK: s = Synth(.12f); s.noise(0, .08f, .4f, 3000, 600, 4); break;
        case SFX_LOOT: s = Synth(.35f); s.tone(0, .18f, 520, 1040, .3f, 3); s.tone(.08f, .22f, 780, 1560, .2f); break;
        case SFX_LEGEND: { s = Synth(1.f); float f[4] = {392, 523, 659, 784}; for (int i = 0; i < 4; i++) s.tone(i * .09f, .6f, f[i], f[i] * 1.01f, .3f, 3, 3); break; }
        case SFX_HURT: s = Synth(.25f); s.tone(0, .18f, 170, 80, .45f, 2, 5); s.noise(0, .14f, .5f, 1200, 0, 5); break;
        case SFX_BEEP: s = Synth(.06f); s.tone(0, .05f, 1500, 1500, .25f, 2, 1); break;
        case SFX_RELOAD: s = Synth(.35f); s.noise(0, .05f, .5f, 6000, 1500, 4); s.noise(.2f, .06f, .6f, 5000, 1200, 4); break;
        case SFX_ROLL: s = Synth(.25f); s.noise(0, .22f, .5f, 700, 0, 3); break;
        case SFX_GROAN: s = Synth(1.3f); s.tone(0, 1.2f, frand(78, 98), 55, .25f, 1, 2.5f, .2f); s.noise(0, 1.2f, .14f, 380, 0, 3); break;
        case SFX_SCREAM: s = Synth(.8f); s.tone(0, .7f, 300, 1300, .35f, 1, 2, .05f); s.noise(0, .7f, .3f, 3000, 1000, 3); break;
        case SFX_HORN: s = Synth(2.2f); s.tone(0, 2.1f, 110, 104, .4f, 1, 1.5f, .3f); s.tone(0, 2.1f, 165, 157, .25f, 1, 1.5f, .3f); break;
        case SFX_LEVEL: { s = Synth(.6f); float f[3] = {523, 659, 784}; for (int i = 0; i < 3; i++) s.tone(i * .1f, .3f, f[i], f[i], .3f, 3, 3); break; }
        case SFX_THROW: s = Synth(.2f); s.noise(0, .16f, .4f, 2500, 600, 3); break;
        case SFX_ROAR: s = Synth(.9f); s.tone(0, .8f, 95, 48, .7f, 1, 2.5f, .05f); s.noise(0, .7f, .6f, 350, 0, 3); break;
        case SFX_EMPTY: s = Synth(.05f); s.tone(0, .03f, 900, 900, .2f, 2, 2); break;
        case SFX_GORE: s = Synth(.6f); s.noise(0, .5f, 1.1f, 900, 60, 4); s.noise(.05f, .3f, .7f, 300, 0, 3); s.tone(0, .25f, 90, 40, .6f, 1, 4); break;
        case SFX_SWING: s = Synth(.22f); s.noise(0, .2f, .5f, 3500, 900, 2); break;
        case SFX_CHOP: s = Synth(.3f); s.noise(0, .22f, 1.f, 1500, 100, 5); s.tone(0, .12f, 140, 60, .6f, 0, 4); break;
        case SFX_RAIN: s = Synth(4.f); s.noise(0, 4.f, .2f, 6000, 800, .01f); break;
        case SFX_WIND: s = Synth(5.f); s.noise(0, 5.f, .25f, 300, 60, .01f); break;
        case SFX_EAT: s = Synth(.4f); for (int i = 0; i < 3; i++) s.noise(i * .12f, .08f, .4f, 2000, 300, 4); break;
        case SFX_STEP: s = Synth(.08f); s.noise(0, .06f, .25f, 900, 120, 6); break;
        case SFX_DOOR_OPEN: case SFX_DOOR_CLOSE: s = Synth(.3f); s.noise(0, .25f, .6f, 900, 80, 5); s.tone(0, .12f, 110, 70, .4f, 0, 5); break;
        case SFX_DOOR_BANG: s = Synth(.35f); s.noise(0, .3f, 1.f, 600, 50, 6); s.tone(0, .2f, 80, 45, .8f, 0, 5); break;
        case SFX_CREAK: s = Synth(.5f); s.tone(0, .45f, 240, 180, .25f, 1, 2, .05f); break;
        case SFX_FIRE: s = Synth(1.5f); for (int i = 0; i < 14; i++) s.noise(frand(0, 1.4f), .03f, .5f, 3000, 600, 8); s.noise(0, 1.5f, .12f, 500, 0, .1f); break;
        }
        snd[id][0] = s.make(); for (int a = 1; a < ALIASES; a++) snd[id][a] = LoadSoundAlias(snd[id][0]);
    }
    // recorded samples (Kenney, CC0) replace or layer over the synth where we have them
    std::string dir = assetDir.empty() ? std::string("assets/sounds/") : assetDir + "sounds/";
    auto add = [&](int id, const char* prefix, int n0, int n1, const char* fmt) { for (int i = n0; i <= n1; i++) { char fn[96]; snprintf(fn, 96, fmt, prefix, i); std::string f = dir + fn; if (!FileExists(f.c_str())) continue;
        Sound so = LoadSound(f.c_str()); if (!so.frameCount) continue; bank[id].push_back(so); bank[id].push_back(LoadSoundAlias(so)); } };
    add(SFX_STEP, "footstep", 0, 9, "%s%02d.ogg"); add(SFX_DOOR_OPEN, "doorOpen_", 0, 9, "%s%d.ogg"); add(SFX_DOOR_OPEN, "doorOpen_", 0, 9, "%s%03d.ogg");
    add(SFX_DOOR_CLOSE, "doorClose_", 0, 9, "%s%d.ogg"); add(SFX_DOOR_CLOSE, "doorClose_", 0, 9, "%s%03d.ogg"); add(SFX_DOOR_BANG, "doorClose_", 1, 4, "%s%d.ogg");
    add(SFX_CHOP, "chop", 0, 0, "%s.ogg"); add(SFX_CHOP, "knifeSlice", 0, 0, "%s.ogg"); add(SFX_CHOP, "knifeSlice", 2, 2, "%s%d.ogg");
    add(SFX_SWING, "drawKnife", 1, 3, "%s%d.ogg"); add(SFX_CREAK, "creak", 1, 3, "%s%d.ogg"); add(SFX_PICK, "cloth", 1, 4, "%s%d.ogg");
    add(SFX_HIT_WOOD, "hit_wood_", 0, 9, "%s%d.ogg"); add(SFX_HIT_SLICE, "hit_slice_", 0, 9, "%s%d.ogg"); add(SFX_HIT_SLICE, "hit_blade_", 0, 0, "%s%d.ogg");
    add(SFX_HIT_HEAVY, "hit_heavy_", 0, 9, "%s%d.ogg"); add(SFX_HIT_PUNCH, "hit_punch_", 0, 9, "%s%d.ogg"); add(SFX_BLOCK, "hit_wood_", 0, 9, "%s%d.ogg");
    add(SFX_GROAN, "z_groan_", 0, 9, "%s%d.ogg"); add(SFX_SCREAM, "z_scream_", 0, 9, "%s%d.ogg"); add(SFX_ROAR, "z_brute_", 0, 9, "%s%d.ogg"); add(SFX_ZATTACK, "z_attack_", 0, 9, "%s%d.ogg");
    add(SFX_ZDEATH, "z_death_", 0, 9, "%s%d.ogg"); add(SFX_GORE, "gore_", 0, 9, "%s%d.ogg"); add(SFX_THUNDER, "thunder_", 0, 9, "%s%d.ogg"); add(SFX_STING, "sting_horde", 0, 0, "%s.ogg");
    add(SFX_DOG, "amb_dog_", 0, 9, "%s%d.ogg"); add(SFX_DOOR_METAL, "door_metal_", 0, 9, "%s%d.ogg"); add(SFX_DOOR_METAL, "impactMetal_", 0, 4, "%s%03d.ogg");
    bank[SFX_DOOR_BANG].clear(); add(SFX_DOOR_BANG, "door_wood_", 0, 9, "%s%d.ogg"); add(SFX_DOOR_BANG, "hit_heavy_", 0, 9, "%s%d.ogg"); if (bank[SFX_DOOR_BANG].empty()) add(SFX_DOOR_BANG, "doorClose_", 1, 4, "%s%d.ogg"); add(SFX_CAR_DOOR, "car_door_", 0, 9, "%s%d.ogg");
    add(SFX_RELOAD, "metalLatch", 0, 0, "%s.ogg"); add(SFX_EMPTY, "metalClick", 0, 0, "%s.ogg"); add(SFX_BOOM, "explosionCrunch_", 0, 4, "%s%03d.ogg"); add(SFX_BOOM, "lowFrequency_explosion_", 0, 1, "%s%03d.ogg");
    int nb = 0; for (auto& b : bank) nb += (int)b.size() / 2; TraceLog(LOG_INFO, "NIGHTFALL: %d recorded sound samples", nb);
}
static void sfx(int id, float vol = 1, float pv = .08f) { if (!audioOk || muted) return;
    vol *= cfg.master * cfg.sfx;
    if (!bank[id].empty()) { Sound& b = bank[id][irand(0, (int)bank[id].size() - 1)]; SetSoundVolume(b, vol * (id == SFX_STEP ? .9f : .75f)); SetSoundPitch(b, 1 + frand(-pv, pv)); PlaySound(b); if (id != SFX_BOOM) return; }
    Sound& s = snd[id][sndNext[id]]; sndNext[id] = (sndNext[id] + 1) % ALIASES; SetSoundVolume(s, vol * .6f); SetSoundPitch(s, 1 + frand(-pv, pv)); PlaySound(s); }
static void sfxAt(int id, Vector2 at, float vol = 1, float pv = .08f) { float d = Vector2Distance(at, P.p); if (d > 70) return; sfx(id, vol * Clamp(1 - d / 70, 0, 1), pv); }

// ============================================================ inventory
// ---- container grids. Only clothes with pockets hold stuff; lose the jacket and you lose what's in it
static const int CONT_ORDER[4] = {CS_TORSO, CS_LEGS, CS_OUTER, CS_BACK};
static void itemDims(const InvItem& it, int& w, int& h) { if (it.kind == 1) { w = CD[it.c->def].iw; h = CD[it.c->def].ih; } else { w = ISZ[it.id][0]; h = ISZ[it.id][1]; } if (it.rot) std::swap(w, h); }
static bool fits(const Cloth& c, int x, int y, int w, int h, const InvItem* ignore = nullptr) {
    if (x < 0 || y < 0 || x + w > gridW(c) || y + h > gridH(c)) return false;
    for (auto& o : c.items) { if (&o == ignore) continue; int ow, oh; itemDims(o, ow, oh); if (x < o.x + ow && o.x < x + w && y < o.y + oh && o.y < y + h) return false; } return true; }
static bool findSpot(const Cloth& c, InvItem& it) {
    for (int r = 0; r < 2; r++) { it.rot = r; int w, h; itemDims(it, w, h); for (int y = 0; y + h <= gridH(c); y++) for (int x = 0; x + w <= gridW(c); x++) if (fits(c, x, y, w, h)) { it.x = x; it.y = y; return true; } }
    it.rot = false; return false; }
static bool placeAnywhere(const InvItem& src) { InvItem it = src; for (int s : CONT_ORDER) { auto& c = P.eq[s]; if (c && gridH(*c) > 0 && findSpot(*c, it)) { c->items.push_back(it); return true; } } return false; }
template <class F> static void forInv(F f) { for (int s : CONT_ORDER) if (P.eq[s]) for (auto& it : P.eq[s]->items) f(it); }
static int invCount(int id) { int n = 0; forInv([&](InvItem& it) { if (it.kind == 0 && it.id == id) n += it.n; }); return n; }
static int invAdd(int id, int n) { // returns leftover
    forInv([&](InvItem& it) { if (n > 0 && it.kind == 0 && it.id == id && it.n < IDEF[id].stack) { int k = std::min(n, IDEF[id].stack - it.n); it.n += k; n -= k; } });
    while (n > 0) { InvItem it; it.kind = 0; it.id = id; it.n = std::min(n, IDEF[id].stack); if (!placeAnywhere(it)) break; n -= it.n; } return n; }
static void invPrune() { for (int s : CONT_ORDER) if (P.eq[s]) { auto& v = P.eq[s]->items; v.erase(std::remove_if(v.begin(), v.end(), [](const InvItem& i) { return i.kind == 0 && i.n <= 0; }), v.end()); } }
static int invTake(int id, int n) { int took = 0; for (int s : CONT_ORDER) if (P.eq[s]) for (int i = (int)P.eq[s]->items.size() - 1; i >= 0 && n > 0; i--) { auto& it = P.eq[s]->items[i]; if (it.kind == 0 && it.id == id) { int k = std::min(n, it.n); it.n -= k; n -= k; took += k; } } invPrune(); return took; }
static InvItem* invFind(int id) { InvItem* r = nullptr; forInv([&](InvItem& it) { if (!r && it.kind == 0 && it.id == id && it.n > 0) r = &it; }); return r; }
static int setCount(int si) { int n = 0; for (int s = 0; s < CS_COUNT; s++) if (P.eq[s] && P.eq[s]->set == si && P.eq[s]->cond > 0) n++; return n; }
struct Gear { float warm = 0, water = 0, armor = 0, noise = 1, vis = 1; int slots = 0, used = 0; };
static Gear gear() { Gear g; float wsum = 0;
    for (int s = 0; s < CS_COUNT; s++) { auto& c = P.eq[s]; if (!c) continue; const CDef& d = CD[c->def]; g.warm += clothWarm(*c); g.armor += clothArmor(*c);
        float cov = s == CS_OUTER ? .45f : s == CS_TORSO ? .2f : s == CS_LEGS ? .2f : s == CS_FEET ? .1f : s == CS_HEAD ? .05f : 0; g.water += clothWater(*c) * cov; wsum += cov;
        g.noise *= std::max(.6f, d.noise - c->bQuiet); g.vis *= d.vis * c->bVis; if (gridH(*c)) { g.slots += gridW(*c) * gridH(*c); for (auto& it : c->items) { int w, h; itemDims(it, w, h); g.used += w * h; } } }
    g.water = std::min(.95f, g.water / .95f); return g; }
static void wearDamage(float dmg) { // a bite eats the outermost layer first
    static const int order[] = {CS_OUTER, CS_TORSO, CS_LEGS, CS_HANDS, CS_HEAD, CS_BACK};
    for (int k = 0; k < 2; k++) { int s = order[irand(0, 5)]; auto& c = P.eq[s]; if (!c || c->cond <= 0) continue;
        float before = c->cond; c->cond = std::max(0.f, c->cond - dmg * frand(.004f, .012f) * (1 - std::min(.6f, clothArmor(*c))));
        if (condTier(before) != condTier(c->cond)) msg("Your " + c->name + " is " + CONDN[condTier(c->cond)] + ".", condTier(c->cond) >= 3 ? BLOODC : STEEL);
        if (c->cond <= 0 && gridH(*c) == 0) {} } }
static void dressStart() { // fists, rags, and a bad day
    auto put = [](const char* n, float cmax) { auto c = makeCloth(cdIndex(n), 0, cmax); c->cond = frand(.3f, cmax); c->pattern = 3; c->col = lerpC(c->col, rgba(150, 146, 136), .4f); c->name = std::string("Ragged ") + CD[c->def].n; c->decor.clear(); P.eq[CD[c->def].slot] = c; };
    put("T-Shirt", .6f); put(chance(.6f) ? "Jeans" : "Sweatpants", .6f); put("Sneakers", .55f); put("Cotton Socks", .6f);
    invAdd(IT_RAG, 2); invAdd(IT_APPLE, 1); }


// ============================================================ effects
static const Color BLOOD_DRY = rgba(58, 5, 8, 235), BLOOD_FRESH = rgba(120, 12, 18), ACID_C = rgba(120, 165, 40, 220);
static void floatText(Vector3 p, const std::string& t, Color c, float size = 15) { if (floaters.size() > 60) floaters.erase(floaters.begin()); floaters.push_back({p, t, c, size, 1.2f, 1.2f}); }
static void decal(Vector2 p, float r, Color c, float grow = 0) {
    Decal d{p, r, frand(0, 2 * PI), 1, irand(0, 3), c, grow, (buildingAt(p) >= 0 ? .08f : .05f) + frand(0, .004f)};
    if ((int)decals.size() < 6000) decals.push_back(d); else { decals[decalHead] = d; decalHead = (decalHead + 1) % 6000; }
}
static void spray(Vector3 p, float ang, int n, float spread, float smin, float smax, Color c, bool acidK = false, float up = 2.5f) {
    for (int i = 0; i < n; i++) { float a = ang < -99 ? frand(0, 2 * PI) : ang + frand(-spread, spread), s = frand(smin, smax); Vector2 h = polar(a, s);
        parts.push_back({p, V3(h.x, frand(.5f, up) * (s * .35f + 1), h.y), 2.f, 2.f, frand(.03f, .07f), c, acidK ? P_ACID : P_BLOOD}); }
}
static void mist(Vector3 p, int n, Color c) { for (int i = 0; i < n; i++) parts.push_back({Vector3Add(p, V3(frand(-.2f, .2f), frand(-.1f, .2f), frand(-.2f, .2f))), V3(frand(-1.5f, 1.5f), frand(.2f, 1.2f), frand(-1.5f, 1.5f)), frand(.6f, 1.2f), 1.2f, frand(.4f, .8f), c, P_MIST}); }
static void addGib(Vector3 p, int kind, Color c, Vector3 size, float ang, float spd, bool acidK = false) {
    if (gibs.size() > 420) gibs.erase(gibs.begin());
    Vector2 h = polar(ang, spd); gibs.push_back({p, V3(h.x, frand(2, 6), h.y), Vector3Normalize(V3(frand(-1, 1), frand(-1, 1), frand(-1, 1))), frand(0, 6), frand(-14, 14), kind, c, size, 0, 0, acidK, false}); gibCount++;
}
static void screenGore(int n) { for (int i = 0; i < n; i++) { Vector2 p = chance(.5f) ? V2(frand(0, (float)W), chance(.5f) ? frand(0, H * .25f) : frand(H * .75f, (float)H)) : V2(chance(.5f) ? frand(0, W * .2f) : frand(W * .8f, (float)W), frand(0, (float)H)); sdrops.push_back({p, frand(12, 42), 3.f, 3.f}); } }

// ============================================================ spawning
static float diffK() { static const float base[4] = {.7f, 1.f, 1.3f, 1.65f}; return base[cfg.diff]; }
static int playerLevel();
static float scaleK() { return diffK() * (1 + .07f * (playerLevel() - 1)); } // zombies get tougher as you level up
static Zombie& spawnZ(int type, Vector2 p) {
    const ZDef& t = ZT[type]; float s = 1 + .12f * (dayN() - 1);
    Zombie z{}; z.id = ++zid; z.type = type; z.p = p; z.r = t.r; z.hp = z.mhp = t.hp * s * scaleK(); z.spd = t.spd * frand(.85f, 1.15f) * (cfg.diff >= 2 ? 1.08f : cfg.diff == 0 ? .92f : 1); z.dmg = t.dmg * (1 + .06f * (dayN() - 1)) * (.6f + .4f * scaleK());
    z.seed = frand(0, 10); z.a = frand(0, 2 * PI); z.side = chance(.5f) ? 1 : -1; z.wa = frand(0, 2 * PI);
    { static const int pool[6] = {0, 1, 2, 3, 4, 5}; auto& lp = lookPool[pool[type]]; if (!lp.empty()) z.look = lp[irand(0, (int)lp.size() - 1)]; else if (!lookPool[0].empty()) z.look = lookPool[0][irand(0, (int)lookPool[0].size() - 1)]; }
    if ((type == Z_WALKER || type == Z_RUNNER) && chance(.07f)) { // uniforms: more of them near the places they worked
        int t = townAt(p, 1.2f), h = hotAt(p, 1.3f); std::vector<int> pool{J_MEDIC, J_COP, J_FIRE, J_MECH, J_HIKER};
        if (t < 0) { pool = {J_HUNTER, J_HUNTER, J_HIKER, J_HIKER, J_MECH}; } if (h >= 0) { pool.push_back(J_MEDIC); pool.push_back(J_MEDIC); pool.push_back(J_COP); }
        z.job = pick(pool); }
    zombies.push_back(z); return zombies.back();
}
static Zombie* spawnAround(int type, float minD, float maxD, float ang = -999, float spreadA = .5f) {
    for (int i = 0; i < 16; i++) { float a = ang < -99 ? frand(0, 2 * PI) : ang + frand(-spreadA, spreadA), d = frand(minD, maxD); Vector2 p = Vector2Add(P.p, polar(a, d));
        if (freeAt(p.x, p.y, .6f)) return &spawnZ(type, p); }
    return nullptr;
}
static int zoneTown() { return townAt(P.p, 1.3f); }
static int pickType(int town) {
    int d = dayN(); bool n = isNight();
    if (town >= 0 && TOWNS[town].kind == TK_MILITARY && chance(.6f)) return Z_SOLDIER;
    int hz = hotAt(P.p, 1.6f); if (hz >= 0) { int k = hotZones[hz].kind; float r = frand(0, 1);
        if ((k == HZ_CHECKPOINT || k == HZ_EVAC || k == HZ_CONVOY) && r < .35f) return Z_SOLDIER; if (r < .55f) return Z_RUNNER; if (r < .62f) return Z_BRUTE; if (r < .68f && d >= 2) return Z_BLOATER; if (n && r < .72f) return Z_VOLATILE; }
    float w[6] = {70, 10.f + d * 3 + (n ? 18 : 0), 6.f + d, d >= 2 ? 2.f + d : 0, n ? 8.f + d * 3 : 0, 0};
    float tot = 0; for (float x : w) tot += x; float r = frand(0, tot);
    for (int i = 0; i < 6; i++) { r -= w[i]; if (r <= 0) return i; }
    return 0;
}

// ============================================================ loot generation
static int rollRarity(int mn = 0) { float r = frand(0, 1) / (P.perks.luck * (1 + .05f * (dayN() - 1))); return std::max(mn, r < .03f ? 3 : r < .12f ? 2 : r < .38f ? 1 : 0); }
static void spawnItem(Vector3 p, int id, int n) { Loot l; l.kind = LK_ITEM; l.id = id; l.count = n; l.p = p; l.v = V3(0, 0, 0); l.rot = frand(0, 6); loot.push_back(l); }
static void spawnGun(Vector3 p, int key, int rar) { Loot l; l.kind = LK_GUN; l.id = key; l.count = 1; l.w = makeWeapon(key, rar); l.p = p; l.rot = frand(0, 6); loot.push_back(l); if (rar > bestRar) { bestRar = rar; bestName = loot.back().w.name; } }
static void spawnMelee(Vector3 p, int key) { Loot l; l.kind = LK_MELEE; l.id = key; l.count = 1; l.p = p; l.rot = frand(0, 6); loot.push_back(l); }
static void spawnCloth(Vector3 p, std::shared_ptr<Cloth> c);
static void rollTable(Vector3 p, int table) { // loot by place: tools in sheds and garages, clothes in closets and stores, guns where guns would be
    auto it = [&](int id, int a, int b) { int n = irand(a, b); if (isAmmo(id)) n = (int)roundf(n * P.perks.ammo); spawnItem(Vector3Add(p, V3(frand(-.3f, .3f), 0, frand(-.3f, .3f))), id, n); };
    auto cl = [&](int fam, int mask = -1, float cmax = 1) { auto c = makeCloth(clothFor(fam, mask), rollRarity(), cmax); spawnCloth(Vector3Add(p, V3(frand(-.3f, .3f), 0, frand(-.3f, .3f))), c); };
    auto pk = [](std::initializer_list<int> v) { return *(v.begin() + irand(0, (int)v.size() - 1)); };
    const int TOPS = 1 << CS_TORSO | 1 << CS_OUTER, LEGS = 1 << CS_LEGS, SMALL = 1 << CS_HEAD | 1 << CS_EYES | 1 << CS_FACE | 1 << CS_HANDS | 1 << CS_SOCKS, FEET = 1 << CS_FEET, BAGS = 1 << CS_BACK;
    float r = frand(0, 1);
    switch (table) {
    case LT_WOODPILE: it(IT_LOG, 1, 2); if (chance(.6f)) it(IT_KINDLING, 2, 5); if (chance(.3f)) spawnMelee(p, chance(.6f) ? MK_HATCHET : MK_AXE); if (chance(.3f)) it(IT_MATCHES, 4, 10); break;
    case LT_CAR: if (r < .12f) it(pk({IT_A9, IT_A12}), 4, 12); else if (r < .24f) it(chance(.5f) ? IT_WATER : IT_SODA, 1, 1); else if (r < .33f) it(IT_CHIPS, 1, 2); else if (r < .42f) it(IT_FLARE, 1, 2);
        else if (r < .5f) it(IT_BANDAGE, 1, 1); else if (r < .55f) it(IT_PAINKILL, 1, 1); else if (r < .62f) it(IT_BATTERY, 1, 1); else if (r < .67f) it(IT_MATCHES, 3, 8); else if (r < .71f) spawnGun(p, chance(.7f) ? W_M9 : W_GLOCK, rollRarity());
        else if (r < .76f) it(IT_TAPE, 1, 1); else if (r < .8f) spawnMelee(p, pk({MK_CROWBAR, MK_WRENCH})); else if (r < .86f) cl(F_CIV, SMALL | TOPS); else if (r < .89f) cl(F_CIV | F_SCHOOL, BAGS); else if (r < .915f) it(IT_ANTIBIO, 1, 1); break;
    case LT_KITCHEN: if (chance(.12f)) it(IT_MATCHES, 4, 12); if (r < .35f) it(pk({IT_BEANS, IT_PEACHES, IT_CHIPS}), 1, 2); else if (r < .55f) it(IT_WATER, 1, 1); else if (r < .62f) spawnMelee(p, MK_KNIFE); else if (r < .7f) it(IT_SODA, 1, 2); else if (r < .76f) it(IT_RAG, 1, 2); else if (r < .79f) it(IT_TAPE, 1, 1); else if (r < .815f) it(IT_ANTIBIO, 1, 1); break;
    case LT_HOUSE: if (r < .13f) it(IT_BANDAGE, 1, 2); else if (r < .22f) it(IT_APPLE, 1, 3); else if (r < .29f) it(IT_BATTERY, 1, 1); else if (r < .33f) spawnGun(p, chance(.6f) ? W_M9 : W_GLOCK, rollRarity()); else if (r < .38f) it(IT_A9, 6, 15);
        else if (r < .41f) spawnMelee(p, MK_BAT); else if (r < .46f) it(IT_PAINKILL, 1, 2); else if (r < .49f) it(IT_FLARE, 1, 2); else if (r < .54f) it(IT_MATCHES, 3, 10); else if (r < .6f) it(IT_RAG, 1, 3); else if (r < .63f) it(IT_SEWKIT, 1, 1); else if (r < .7f) cl(F_CIV, SMALL); else if (r < .735f) it(IT_ANTIBIO, 1, 1); break;
    case LT_WARDROBE: if (r < .3f) cl(F_CIV, TOPS); else if (r < .5f) cl(F_CIV, LEGS); else if (r < .65f) cl(F_CIV, SMALL); else if (r < .75f) cl(F_CIV, FEET); else if (r < .8f) cl(F_CIV | F_SCHOOL, BAGS); else if (r < .86f) it(IT_SEWKIT, 1, 1); else if (r < .92f) it(IT_RAG, 1, 3); break;
    case LT_CLOTHES: if (r < .35f) cl(F_STORE, TOPS); else if (r < .55f) cl(F_STORE, LEGS); else if (r < .7f) cl(F_STORE, SMALL); else if (r < .82f) cl(F_STORE, FEET); else if (r < .9f) cl(F_STORE, BAGS); else if (r < .95f) it(pk({IT_DYE_BLACK, IT_DYE_GREEN, IT_DYE_BROWN}), 1, 1); else it(IT_SEWKIT, 1, 1); break;
    case LT_STORE: if (r < .38f) it(pk({IT_BEANS, IT_PEACHES, IT_CHIPS, IT_SODA, IT_WATER, IT_APPLE}), 1, 3); else if (r < .47f) it(IT_BATTERY, 1, 2); else if (r < .52f) it(IT_FLARE, 1, 2); else if (r < .58f) it(IT_BANDAGE, 1, 2); else if (r < .65f) it(IT_MATCHES, 5, 15);
        else if (r < .7f) it(IT_TAPE, 1, 1); else if (r < .74f) it(IT_SEWKIT, 1, 1); else if (r < .78f) it(IT_WAX, 1, 1); else if (r < .88f) cl(F_STORE); break;
    case LT_GAS: if (r < .35f) it(pk({IT_CHIPS, IT_SODA, IT_WATER, IT_BEANS}), 1, 2); else if (r < .45f) it(IT_TAPE, 1, 1); else if (r < .55f) it(IT_MATCHES, 4, 12); else if (r < .62f) it(IT_GAS, 1, 1); else if (r < .7f) it(IT_BATTERY, 1, 1); else if (r < .76f) it(IT_FLARE, 1, 2); else if (r < .82f) cl(F_STORE | F_WORK, SMALL); else if (r < .85f) spawnMelee(p, MK_WRENCH); break;
    case LT_POLICE: if (r < .26f) spawnGun(p, pk({W_M9, W_GLOCK, W_GLOCK, W_SHOTGUN, W_MP5}), rollRarity()); else if (r < .52f) it(pk({IT_A9, IT_A9, IT_A12}), 8, 24); else if (r < .62f) it(IT_BANDAGE, 1, 3); else if (r < .7f) it(IT_FLARE, 1, 3); else if (r < .86f) cl(F_POLICE); break;
    case LT_MED: if (r < .32f) it(IT_BANDAGE, 1, 3); else if (r < .5f) it(IT_ANTIBIO, 1, 1); else if (r < .65f) it(IT_PAINKILL, 1, 2); else if (r < .74f) it(IT_MORPHINE, 1, 1); else if (r < .82f) it(IT_RAG, 2, 4); else if (r < .9f) cl(F_MED); break;
    case LT_GARAGE: case LT_SHED: { bool sh = table == LT_SHED;
        if (r < .2f) spawnMelee(p, pk({MK_CROWBAR, MK_WRENCH, MK_HATCHET, MK_PIPE, MK_AXE, MK_MACHETE})); else if (r < .3f) it(IT_TAPE, 1, 1); else if (r < .38f) it(IT_ROPE, 1, 1); else if (r < .44f) it(IT_GAS, 1, 1); else if (r < .5f) it(IT_BATTERY, 1, 2);
        else if (r < .55f) it(sh ? IT_KINDLING : IT_PIPEBOMB, 1, sh ? 4 : 1); else if (r < .6f) it(IT_FLARE, 1, 2); else if (r < .66f) it(IT_WAX, 1, 1); else if (r < .72f) it(IT_LEATHER, 1, 2); else if (r < .8f) cl(F_WORK, SMALL | FEET); else if (r < .84f) it(IT_RAG, 1, 3); break; }
    case LT_FACTORY: if (r < .22f) spawnMelee(p, pk({MK_CROWBAR, MK_WRENCH, MK_PIPE, MK_PIPE})); else if (r < .34f) it(IT_TAPE, 1, 1); else if (r < .42f) it(IT_ROPE, 1, 1); else if (r < .5f) it(IT_GAS, 1, 1); else if (r < .62f) cl(F_WORK); else if (r < .7f) it(IT_LEATHER, 1, 3); else if (r < .76f) it(IT_BATTERY, 1, 2); else if (r < .8f) it(IT_PIPEBOMB, 1, 1); break;
    case LT_FIRE: if (r < .28f) cl(F_FIRE); else if (r < .4f) spawnMelee(p, chance(.7f) ? MK_AXE : MK_CROWBAR); else if (r < .55f) it(IT_BANDAGE, 1, 3); else if (r < .62f) it(IT_MORPHINE, 1, 1); else if (r < .72f) it(IT_FLARE, 1, 3); else if (r < .78f) it(IT_ROPE, 1, 1); else if (r < .84f) it(IT_GAS, 1, 1); break;
    case LT_SCHOOL: if (r < .25f) cl(F_SCHOOL, BAGS); else if (r < .45f) cl(F_SCHOOL); else if (r < .58f) it(pk({IT_CHIPS, IT_SODA, IT_APPLE, IT_WATER}), 1, 2); else if (r < .65f) it(IT_BANDAGE, 1, 1); else if (r < .7f) spawnMelee(p, MK_BAT); else if (r < .75f) it(IT_TAPE, 1, 1); break;
    case LT_TRAILER: if (r < .14f) it(pk({IT_BEANS, IT_CHIPS, IT_SODA}), 1, 2); else if (r < .26f) cl(F_CIV); else if (r < .32f) it(IT_TAPE, 1, 1); else if (r < .38f) spawnGun(p, chance(.6f) ? W_SHOTGUN : W_M9, rollRarity()); else if (r < .45f) it(pk({IT_A12, IT_A9}), 4, 10); else if (r < .52f) it(IT_RAG, 1, 3); else if (r < .57f) spawnMelee(p, pk({MK_BAT, MK_PIPE, MK_KNIFE})); else if (r < .62f) it(IT_MATCHES, 2, 6); break;
    case LT_HUNT: if (r < .25f) cl(F_HUNT); else if (r < .38f) spawnGun(p, chance(.7f) ? W_HUNT : W_SHOTGUN, rollRarity()); else if (r < .55f) it(chance(.6f) ? IT_A308 : IT_A12, 4, 12); else if (r < .62f) spawnMelee(p, chance(.5f) ? MK_KNIFE : MK_HATCHET); else if (r < .7f) it(IT_ROPE, 1, 1); else if (r < .76f) it(IT_MATCHES, 4, 10); else if (r < .82f) it(IT_WAX, 1, 1); else if (r < .86f) it(IT_ANTIBIO, 1, 1); break;
    case LT_BASEMENT: if (r < .14f) it(pk({IT_BEANS, IT_PEACHES, IT_WATER}), 1, 3); else if (r < .24f) spawnMelee(p, pk({MK_CROWBAR, MK_WRENCH, MK_HATCHET, MK_PIPE, MK_AXE})); else if (r < .32f) it(IT_TAPE, 1, 1); else if (r < .38f) it(IT_GAS, 1, 1); else if (r < .45f) spawnGun(p, pk({W_SHOTGUN, W_HUNT, W_M9, W_GLOCK}), rollRarity());
        else if (r < .54f) it(pk({IT_A9, IT_A12, IT_A308}), 6, 18); else if (r < .6f) it(IT_BATTERY, 1, 2); else if (r < .66f) cl(F_WORK | F_HUNT | F_CIV, TOPS | FEET); else if (r < .72f) it(IT_ROPE, 1, 1); else if (r < .77f) it(IT_MORPHINE, 1, 1); else if (r < .82f) it(IT_LEATHER, 1, 2); break;
    case LT_BARN: if (r < .14f) spawnGun(p, chance(.6f) ? W_HUNT : W_SHOTGUN, rollRarity()); else if (r < .3f) it(chance(.5f) ? IT_A308 : IT_A12, 4, 12); else if (r < .42f) spawnMelee(p, pk({MK_AXE, MK_HATCHET, MK_PIPE})); else if (r < .52f) it(IT_APPLE, 2, 5); else if (r < .6f) it(IT_ROPE, 1, 1); else if (r < .72f) cl(F_FARM); else if (r < .78f) it(IT_LEATHER, 1, 2); else if (r < .81f) it(IT_ANTIBIO, 1, 1); break;
    case LT_MIL: if (r < .26f) spawnGun(p, pk({W_AR15, W_M4, W_M4, W_MP5, W_HUNT}), rollRarity(1)); else if (r < .54f) it(pk({IT_A556, IT_A556, IT_A9, IT_A308}), 15, 40); else if (r < .62f) it(IT_PIPEBOMB, 1, 2); else if (r < .7f) it(IT_MORPHINE, 1, 1); else if (r < .76f) it(IT_BANDAGE, 2, 3); else if (r < .92f) cl(F_MIL); break;
    case LT_CACHE: if (r < .34f) spawnGun(p, pk({W_GLOCK, W_SHOTGUN, W_MP5, W_AR15, W_M4, W_HUNT}), rollRarity(1));
        else if (r < .62f) it(pk({IT_A9, IT_A12, IT_A556, IT_A308}), 15, 35); else if (r < .74f) it(IT_PIPEBOMB, 1, 2); else if (r < .82f) it(chance(.5f) ? IT_MORPHINE : IT_ANTIBIO, 1, 1); else if (r < .9f) { auto c = makeCloth(clothFor(F_MIL | F_HUNT, BAGS | TOPS), rollRarity(1)); spawnCloth(p, c); } else if (r < .95f) spawnMelee(p, MK_AXE); else it(IT_BATTERY, 2, 3); break;
    case LT_OFFICE: if (r < .2f) it(IT_SODA, 1, 2); else if (r < .3f) it(IT_BATTERY, 1, 1); else if (r < .36f) it(IT_PAINKILL, 1, 1); else if (r < .42f) it(IT_TAPE, 1, 1); else if (r < .5f) cl(F_CIV, TOPS | SMALL); break;
    }
}
static void genLoot() { loot.clear(); for (auto& b : blds) { static const float lc[3] = {.85f, .7f, .38f}; for (auto& s : b.spots) if (s.table == LT_CACHE || chance(lc[b.cond])) rollTable(s.p, s.table); }
    for (auto& s : yardSpots) { if (s.table == LT_WOODPILE) { rollTable(s.p, s.table); continue; } if (s.table == LT_CACHE) { rollTable(s.p, s.table); rollTable(s.p, chance(.5f) ? LT_CACHE : LT_MIL); } else if (chance(.8f)) rollTable(s.p, s.table); } }
static void dropFromZombie(Vector2 p, int type, int job);
static void dropJob(Vector2 p, int job) { Vector3 g = xz3(p, .05f); auto at = [&]() { return Vector3Add(g, V3(frand(-.4f, .4f), 0, frand(-.4f, .4f))); };
    auto cl = [&](int fam, float cm) { if (chance(.5f)) spawnCloth(at(), makeCloth(clothFor(fam), rollRarity(), cm)); };
    switch (job) {
    case J_MEDIC: spawnItem(at(), chance(.55f) ? IT_ANTIBIO : IT_BANDAGE, 1); if (chance(.5f)) spawnItem(at(), IT_BANDAGE, irand(1, 2)); if (chance(.2f)) spawnItem(at(), IT_MORPHINE, 1); if (chance(.25f)) spawnItem(at(), IT_PAINKILL, 1); cl(F_MED, .85f); break;
    case J_COP: spawnItem(at(), IT_A9, irand(6, 15)); if (chance(.2f)) spawnGun(at(), chance(.6f) ? W_GLOCK : W_M9, rollRarity()); cl(F_POLICE, .8f); break;
    case J_FIRE: if (chance(.35f)) spawnMelee(at(), MK_AXE); spawnItem(at(), IT_FLARE, irand(1, 2)); cl(F_FIRE, .85f); break;
    case J_HUNTER: spawnItem(at(), chance(.6f) ? IT_A308 : IT_A12, irand(3, 8)); if (chance(.3f)) spawnMelee(at(), chance(.6f) ? MK_KNIFE : MK_HATCHET); cl(F_HUNT, .85f); break;
    case J_MECH: if (chance(.5f)) spawnItem(at(), IT_TAPE, 1); if (chance(.3f)) spawnMelee(at(), MK_WRENCH); if (chance(.25f)) spawnItem(at(), IT_GAS, 1); cl(F_WORK, .85f); break;
    case J_HIKER: spawnItem(at(), chance(.5f) ? IT_WATER : IT_BEANS, 1); if (chance(.5f)) spawnItem(at(), IT_MATCHES, irand(3, 8)); if (chance(.4f)) spawnCloth(at(), makeCloth(clothFor(F_STORE, 1 << CS_BACK), rollRarity(), .8f)); break; } }
static void dropFromZombie(Vector2 p, int type) { dropFromZombie(p, type, 0); }
static void dropFromZombie(Vector2 p, int type, int job) {
    if (job) { dropJob(p, job); return; }
    float r = frand(0, 1); Vector3 g = xz3(p, .05f);
    if (type == Z_BRUTE) { spawnGun(g, pick(std::vector<int>{W_SHOTGUN, W_AR15, W_HUNT, W_MP5}), rollRarity(1)); return; }
    if (type == Z_SOLDIER) { if (r < .35f) spawnItem(g, chance(.7f) ? IT_A556 : IT_A9, irand(5, 20)); else if (r < .4f) spawnGun(g, chance(.5f) ? W_M4 : W_AR15, rollRarity()); else if (r < .5f) spawnItem(g, IT_BANDAGE, 1); return; }
    if (type == Z_VOLATILE && r < .3f) { spawnGun(g, pick(std::vector<int>{W_SHOTGUN, W_M4, W_MP5}), rollRarity(1)); return; }
    float L = P.perks.luck;
    if (chance(.07f * L)) { int fam = type == Z_SOLDIER ? F_MIL : chance(.15f) ? pick(std::vector<int>{F_POLICE, F_FIRE, F_MED, F_WORK, F_FARM}) : F_CIV; auto c = makeCloth(clothFor(fam), rollRarity(), .55f); spawnCloth(g, c); return; } // they died in their clothes. Mostly ruined
    if (r < .05f * L) spawnItem(g, pick(std::vector<int>{IT_A9, IT_A12, IT_A556}), (int)roundf(irand(3, 10) * P.perks.ammo)); else if (r < .09f * L) spawnItem(g, IT_BANDAGE, 1); else if (r < .12f * L) spawnItem(g, pick(std::vector<int>{IT_SODA, IT_CHIPS, IT_APPLE}), 1);
}

// ============================================================ combat & gore
static void die();
static void throwItem(bool flareK, bool free = false);
static int playerLevel() { return P.lvl; }
static void gainXP(float v) { P.xp += v; while (P.xp >= P.next) { P.xp -= P.next; P.lvl++; P.next = xpNeed(P.lvl); pendingLv++; } }
static void noiseAt(Vector2 p, float r) { for (auto& z : zombies) if (Vector2DistanceSqr(z.p, p) < r * r && z.aggro <= 0 && z.ignoreT <= 0 && z.pound < 0) { z.np = Vector2Add(p, V2(frand(-4, 4), frand(-4, 4))); z.noiseT = 12; } }
static void hitPlayer(float dmg, Vector2 src, int srcType) {
    if (P.iframe > 0 || state != ST_PLAY) return;
    if (P.blocking && srcType >= 0 && srcType < 99 && fabsf(angDiff(atan2f(src.y - P.p.y, src.x - P.p.x), P.a)) < 1.2f) { // blocked from the front
        float cost = P.shieldHP > 0 ? 5.f : 11.f;
        if (P.sta >= cost) { P.sta -= cost; P.staD = .7f; float red = std::min(.95f, (P.shieldHP > 0 ? .88f : .6f) + P.perks.block); float taken = dmg * (1 - red);
            if (P.shieldHP > 0) { P.shieldHP -= dmg * (1 - P.perks.block * 2) * .8f; if (P.shieldHP <= 0) { P.shieldHP = 0; msg("Your shield splinters apart.", BLOODC); } }
            for (auto& z : zombies) if (Vector2Distance(z.p, src) < .3f) { z.stun = .45f; z.k = Vector2Add(z.k, polar(atan2f(z.p.y - P.p.y, z.p.x - P.p.x), 3)); }
            sfx(SFX_BLOCK, .9f, .15f); P.blockFx = .15f; shake = std::min(.3f, shake + .1f); P.hp -= taken; if (P.hp <= 0) die(); return; }
        msg("Guard broken!", BLOODC); P.blocking = false; }
    Gear gr = gear(); dmg *= 1 - std::min(.7f, P.perks.armor + gr.armor * .5f); P.hp -= dmg; if (srcType >= 0 && srcType < 99) wearDamage(dmg); P.hurt = .35f; P.iframe = .15f; shake = std::min(.6f, shake + .25f); P.pain = std::max(P.pain, 20.f);
    spray(xz3(P.p, 1.2f), -999, 10, 0, 1, 4, BLOOD_FRESH); sfx(SFX_HURT); screenGore(dmg > 15 ? 4 : 2);
    if (srcType >= 0 && srcType < 99) {
        if (chance((srcType == Z_VOLATILE ? .6f : .35f) * (1 - P.perks.bleedres) * (1 - std::min(.75f, gr.armor)))) { P.bleeds++; msg("You're bleeding.", BLOODC); }
        if (chance(srcType == Z_BLOATER ? .5f : .06f) && P.sick <= 0) { P.sick = 1; msg("You feel sick. Find antibiotics.", rgba(170, 200, 90)); }
        if (srcType == Z_BRUTE && chance(.12f) && !P.broken) { P.broken = true; msg("Your leg is broken. Morphine will fix it.", BLOODC); }
        float a = atan2f(P.p.y - src.y, P.p.x - src.x); P.k = Vector2Add(P.k, polar(a, srcType == Z_BRUTE ? 14.f : 4.f));
    }
    if (P.hp <= 0) die();
}
static void corpse(const Zombie& z, float ang, bool head) {
    if (corpses.size() > 260) corpses.erase(corpses.begin());
    corpses.push_back({z.p, ang, z.type, z.armL, z.armR, head, 0, z.look, frand(0, 1)});
    decal(z.p, .3f, BLOOD_DRY, frand(.9f, 1.5f));
}
static void gibZombie(Zombie& z, float ang) {
    const ZDef& t = ZT[z.type]; Vector3 c = xz3(z.p, 1.0f);
    addGib(xz3(z.p, 1.6f), G_HEAD, t.skin, V3(.24f, .26f, .24f), ang + frand(-.5f, .5f), frand(4, 9));
    if (z.armL) addGib(c, G_LIMB, t.skin, V3(.12f, .6f, .12f), ang + frand(-1.2f, 1.2f), frand(4, 8));
    if (z.armR) addGib(c, G_LIMB, t.skin, V3(.12f, .6f, .12f), ang + frand(-1.2f, 1.2f), frand(4, 8));
    for (int i = 0; i < 2; i++) addGib(V3(z.p.x, .5f, z.p.y), G_LIMB, t.pants, V3(.16f, .8f, .16f), ang + frand(-1, 1), frand(2, 6));
    for (int i = 0; i < irand(5, 9); i++) addGib(c, G_CHUNK, chance(.5f) ? rgba(120, 20, 26) : t.shirt, V3(frand(.1f, .2f), frand(.08f, .16f), frand(.1f, .2f)), ang + frand(-1.4f, 1.4f), frand(3, 10));
    for (int i = 0; i < 3; i++) addGib(c, G_RIB, rgba(225, 215, 195), V3(.04f, .04f, .34f), ang + frand(-1, 1), frand(3, 7));
    addGib(c, G_GUTS, rgba(150, 60, 72), V3(.3f, .14f, .3f), ang + frand(-.5f, .5f), frand(1, 4));
    spray(c, ang, 60, 1.2f, 2, 12, BLOOD_FRESH); mist(c, 10, rgba(130, 12, 18, 160));
    for (int i = 0; i < 4; i++) decal(Vector2Add(z.p, polar(ang + frand(-.6f, .6f), frand(.5f, 3))), frand(.6f, 1.3f), BLOOD_DRY);
    decal(z.p, .4f, BLOOD_DRY, 1.6f);
    sfxAt(SFX_GORE, z.p); shake = std::min(.7f, shake + .2f);
    if (Vector2Distance(z.p, P.p) < 3) screenGore(5);
}
static void acidBoom(Vector2 p);
static void killZ(Zombie& z, float ang, bool head, float overkill, bool explosive) {
    if (z.dead) return; z.dead = true; kills++; if (chance(.3f)) sfxAt(SFX_ZDEATH, z.p, .6f, .2f);
    gainXP(ZT[z.type].xp * (1 + .1f * (dayN() - 1)));
    if (z.type == Z_BLOATER) acidBoom(z.p);
    else if (explosive || overkill > z.mhp * .5f || (z.type == Z_RUNNER && overkill > 25)) gibZombie(z, ang);
    else if (head) {
        Vector3 hp = xz3(z.p, ZT[z.type].h - .15f);
        for (int i = 0; i < 7; i++) addGib(hp, G_CHUNK, chance(.4f) ? ZT[z.type].skin : rgba(130, 18, 24), V3(frand(.05f, .1f), frand(.05f, .09f), frand(.05f, .1f)), ang + frand(-.8f, .8f), frand(3, 8));
        addGib(hp, G_RIB, rgba(230, 220, 200), V3(.05f, .03f, .12f), ang, frand(3, 6));
        spray(hp, ang, 34, .7f, 3, 11, BLOOD_FRESH, false, 1.2f); mist(hp, 6, rgba(150, 14, 20, 170));
        decal(Vector2Add(z.p, polar(ang, frand(1.5f, 3))), frand(.8f, 1.4f), BLOOD_DRY);
        corpse(z, ang, false); floatText(hp, "HEADSHOT", rgba(255, 210, 63), 14); sfxAt(SFX_GORE, z.p, .7f);
    } else { spray(xz3(z.p, 1.1f), ang, 18, .8f, 1, 6, BLOOD_FRESH); corpse(z, ang, true); }
    if (z.type == Z_VOLATILE) sfxAt(SFX_SCREAM, z.p);
    dropFromZombie(z.p, z.type, z.job);
}
static void dismember(Zombie& z, float ang) {
    if (!z.armL && !z.armR) return; bool left = z.armL && (!z.armR || chance(.5f)); if (left) z.armL = false; else z.armR = false;
    Vector2 sp = Vector2Add(z.p, polar(z.a + (left ? -PI / 2 : PI / 2), z.r));
    addGib(xz3(sp, 1.35f), G_LIMB, ZT[z.type].skin, V3(.12f, .6f, .12f), ang + frand(-.8f, .8f), frand(3, 7));
    spray(xz3(sp, 1.35f), ang, 16, .7f, 1, 6, BLOOD_FRESH); z.bleed = 8; z.dmg *= .75f; sfxAt(SFX_CHOP, z.p, .7f);
}
static void hurtZ(Zombie& z, float dmg, bool head, float ang, bool dot, float dism = 0, bool explosive = false) {
    if (z.dead) return;
    if (head && z.type == Z_SOLDIER && chance(.5f)) { head = false; dmg *= .5f; for (int i = 0; i < 4; i++) parts.push_back({xz3(z.p, 1.7f), V3(frand(-3, 3), frand(1, 3), frand(-3, 3)), .15f, .15f, .04f, rgba(255, 230, 160), P_SPARK}); }
    if (head) dmg *= 2.5f;
    z.hp -= dmg;
    if (!dot) {
        z.flash = .06f; z.aggro = std::max(z.aggro, 8.f); z.stun = .1f;
        if (ang > -99) { float kb = z.type == Z_BRUTE ? .6f : z.type == Z_VOLATILE ? 1.5f : 3.f; z.k = Vector2Add(z.k, polar(ang, kb));
            spray(xz3(z.p, 1.2f), ang, 6, .5f, 1.5f, 6, z.type == Z_BLOATER ? ACID_C : BLOOD_FRESH, z.type == Z_BLOATER, 1.2f);
            if (chance(.5f)) decal(Vector2Add(z.p, polar(ang, frand(.4f, 1.6f))), frand(.15f, .35f), z.type == Z_BLOATER ? ACID_C : BLOOD_DRY); }
        if (z.hp > 0 && dism > 0 && z.type != Z_BLOATER && chance(dism)) dismember(z, ang);
        sfxAt(SFX_HIT, z.p, .5f, .2f);
    }
    if (z.hp <= 0) killZ(z, ang < -99 ? frand(0, 2 * PI) : ang, head, -z.hp + (dot ? 0 : dmg * .3f), explosive);
}
static void explode(Vector2 p, float R, float dmg, float self) {
    for (auto& z : zombies) { float d = Vector2Distance(z.p, p); if (d < R + z.r) { float a = atan2f(z.p.y - p.y, z.p.x - p.x); z.k = Vector2Add(z.k, polar(a, 10)); z.burn = std::max(z.burn, 2.f); hurtZ(z, dmg * (1 - .6f * d / (R + z.r)), false, a, false, .7f, true); } }
    float pd = Vector2Distance(P.p, p); if (self > 0 && pd < R * .8f) hitPlayer(self * (1 - pd / R), p, 99);
    for (int i = 0; i < 70; i++) { Vector2 h = polar(frand(0, 2 * PI), frand(1, 16)); parts.push_back({xz3(p, .5f), V3(h.x, frand(1, 9), h.y), frand(.3f, .9f), .9f, frand(.15f, .4f), pick(std::vector<Color>{rgba(255, 180, 60), rgba(255, 120, 40), rgba(255, 220, 140)}), P_FIRE}); }
    for (int i = 0; i < 30; i++) { Vector2 h = polar(frand(0, 2 * PI), frand(.5f, 4)); parts.push_back({xz3(p, .8f), V3(h.x, frand(.5f, 2.5f), h.y), frand(2, 4), 4, frand(1, 2.2f), rgba(50, 50, 54, 200), P_SMOKE}); }
    lights.push_back({xz3(p, 2), R * 4, .5f, .5f, rgba(255, 170, 80)});
    decal(p, R * .5f, rgba(12, 12, 12, 230)); shake = std::min(1.2f, shake + .8f); sfxAt(SFX_BOOM, p, 1.3f, .1f); noiseAt(p, 160);
}
static void acidBoom(Vector2 p) {
    acid.push_back({p, 3.2f, 8}); spray(xz3(p, 1), -999, 50, 0, 2, 9, ACID_C, true); mist(xz3(p, 1), 12, rgba(140, 190, 50, 150));
    for (int i = 0; i < 6; i++) addGib(xz3(p, 1), G_CHUNK, rgba(150, 180, 60), V3(.15f, .1f, .15f), frand(0, 2 * PI), frand(3, 8), true);
    for (auto& z : zombies) if (z.type != Z_BLOATER && Vector2Distance(z.p, p) < 3.5f) hurtZ(z, 30, false, atan2f(z.p.y - p.y, z.p.x - p.x), false);
    if (Vector2Distance(P.p, p) < 3.5f) hitPlayer(16, p, Z_BLOATER);
    lights.push_back({xz3(p, 1.5f), 9, .4f, .4f, rgba(150, 220, 80)}); shake = std::min(.8f, shake + .3f); sfxAt(SFX_SPLAT, p); noiseAt(p, 40);
}

// ============================================================ save / death
static void deleteSave();
static std::string savePath() { const char* h = getenv("HOME"); return std::string(h ? h : ".") + "/.nightfall3d_best"; }
static void loadBest() { std::ifstream f(savePath()); if (f) f >> bestHours >> bestKills; }
static bool newRecord = false; static std::string deathLine;
static void die() {
    state = ST_DEAD; P.hp = 0; deadT = 0; deleteSave(); /* permadeath: the save goes with you */ int hrs = (int)(P.alive / CYCLE * 24);
    newRecord = hrs > bestHours || (hrs == bestHours && kills > bestKills);
    if (newRecord) { bestHours = hrs; bestKills = kills; std::ofstream f(savePath()); f << bestHours << " " << bestKills; }
    static const std::vector<std::string> lines = {"YOU DIED.", "THE COUNTY KEEPS YOU.", "ANOTHER ONE FOR THE ROAD.", "SHOULD HAVE STAYED INSIDE."}; deathLine = pick(lines);
    spray(xz3(P.p, 1), -999, 80, 0, 1, 6, BLOOD_FRESH); decal(P.p, .5f, BLOOD_DRY, 2.f); screenGore(10);
}

// ============================================================ classes & random skill trees
// Every life rolls a class. You start with 2 skill points and every level adds 1, all placed by fate, never by you.
enum Stat { S_DMG, S_RATE, S_CRIT, S_METAB, S_STA, S_LUCK, S_BLAST, S_COLD, S_MELEE, S_NOISE, S_MEDIC, S_REGEN, S_MAXHP, S_RELOAD, S_ARMOR, S_SPEED, S_PISTOL, S_SHOTGUN, S_RIFLE, S_STEALTH, S_CDR, S_AMMO, S_BATT, S_BLEEDRES, S_BLOCK, S_KICK, S_COUNT };
struct Skill { const char* n; const char* d; int stat; float per; int max; };
struct SkillTree { const char* n; Skill s[4]; };
struct ClassDef { const char* n; const char* blurb; const char* act; const char* actD; float actCd; Color c; SkillTree t[3]; };
static const ClassDef CLASSES[5] = {
    {"Police Officer", "Sheriff's deputy. Knows a pistol, owns a vest, never got the overtime.", "Adrenaline Dump", "6s of +60% fire rate and instant reloads.", 60, rgba(110, 155, 255),
     {{"Marksman", {{"Steady Hands", "+12% pistol and SMG damage", S_PISTOL, .12f, 3}, {"Sight Picture", "+4% headshot chance", S_CRIT, .04f, 3}, {"Speed Loader", "+15% reload speed", S_RELOAD, .15f, 3}, {"Double Tap", "+8% fire rate", S_RATE, .08f, 2}}},
      {"Patrol", {{"Body Armor", "Take 6% less damage", S_ARMOR, .06f, 3}, {"Beat Cop", "Sprinting drains 10% less stamina", S_STA, .10f, 3}, {"Evidence Locker", "+15% ammo found", S_AMMO, .15f, 3}, {"Tactical Light", "Flashlight drains 20% slower", S_BATT, .20f, 2}}},
      {"Riot", {{"Breacher", "+15% shotgun damage", S_SHOTGUN, .15f, 3}, {"Baton Work", "+15% melee damage", S_MELEE, .15f, 3}, {"Thick Neck", "+10 max health", S_MAXHP, 10, 3}, {"Shield Wall", "Blocks stop 10% more; shields last longer", S_BLOCK, .10f, 2}}}}},
    {"Paramedic", "Twelve years on the ambulance. Has seen worse. Not much worse.", "Trauma Kit", "Heal 40, close every wound, kill the pain, set a broken leg.", 75, rgba(126, 210, 126),
     {{"Triage", {{"Pressure Points", "Bandages heal 50% more and close extra wounds", S_MEDIC, .5f, 3}, {"Clotting Agent", "15% chance a hit doesn't bleed", S_BLEEDRES, .15f, 3}, {"Second Wind", "Regenerate faster while healthy", S_REGEN, .08f, 3}, {"Rapid Response", "Trauma Kit recharges 15% faster", S_CDR, .15f, 2}}},
      {"Pharmacist", {{"Iron Stomach", "Hunger and thirst 10% slower", S_METAB, .10f, 3}, {"Pill Counter", "Better loot and drop odds", S_LUCK, .10f, 3}, {"Immune System", "+8 max health", S_MAXHP, 8, 3}, {"Night Shift", "Lose body heat 15% slower", S_COLD, .15f, 3}}},
      {"Survivor", {{"Light Step", "Make 12% less noise", S_NOISE, .12f, 3}, {"Scrubs Gray", "The dead spot you 8% later", S_STEALTH, .08f, 3}, {"Cardio", "+4% move speed", S_SPEED, .04f, 3}, {"Shove", "Kicks hit 25% harder and send them flying", S_KICK, .25f, 2}}}}},
    {"Mechanic", "Fixed every truck in the county. Now breaks things on purpose.", "Nail Bomb", "Wire a pipe bomb out of scrap and throw it. Free.", 50, rgba(240, 164, 58),
     {{"Demolition", {{"Short Fuse", "+12% blast radius", S_BLAST, .12f, 3}, {"Shrapnel", "+6% damage with everything", S_DMG, .06f, 3}, {"Scrap Hoarder", "+12% ammo found", S_AMMO, .12f, 3}, {"Quick Hands", "Nail Bomb recharges 12% faster", S_CDR, .12f, 2}}},
      {"Grease Monkey", {{"Coveralls", "Take 5% less damage", S_ARMOR, .05f, 3}, {"Work Boots", "Sprinting drains 10% less stamina", S_STA, .10f, 3}, {"Lunchbox", "Hunger and thirst 10% slower", S_METAB, .10f, 3}, {"Welded Shield", "Blocks stop 10% more; shields last longer", S_BLOCK, .10f, 2}}},
      {"Wrench", {{"Heavy Swing", "+15% melee damage", S_MELEE, .15f, 3}, {"Tool Belt", "+12% reload speed", S_RELOAD, .12f, 3}, {"Bull Neck", "+10 max health", S_MAXHP, 10, 3}, {"Shotgun Rack", "+12% shotgun damage", S_SHOTGUN, .12f, 2}}}}},
    {"Hunter", "Hunted elk off the ridge every fall. Elk don't get back up.", "Deadeye", "8s where every bullet finds the head.", 70, rgba(170, 200, 90),
     {{"Rifleman", {{"Hunting Load", "+15% rifle damage", S_RIFLE, .15f, 3}, {"Breathing", "+5% headshot chance", S_CRIT, .05f, 3}, {"Bolt Work", "+7% fire rate", S_RATE, .07f, 3}, {"Big Game", "+5% damage with everything", S_DMG, .05f, 2}}},
      {"Tracker", {{"Downwind", "The dead spot you 10% later", S_STEALTH, .10f, 3}, {"Soft Soles", "Make 12% less noise", S_NOISE, .12f, 3}, {"Long Legs", "+4% move speed", S_SPEED, .04f, 3}, {"Patience", "Deadeye recharges 12% faster", S_CDR, .12f, 2}}},
      {"Woodsman", {{"Wool Layers", "Lose body heat 20% slower", S_COLD, .20f, 3}, {"Jerky", "Hunger and thirst 12% slower", S_METAB, .12f, 3}, {"Skinner", "+12% melee damage", S_MELEE, .12f, 3}, {"Boot to the Head", "Kicks hit 25% harder and send them flying", S_KICK, .25f, 2}}}}},
    {"Ex-Con", "Got out the week it started. Prison was better prep than you'd think.", "Bloodlust", "8s of double melee damage. Every hit heals you.", 60, rgba(255, 107, 91),
     {{"Butcher", {{"Shank", "+18% melee damage", S_MELEE, .18f, 3}, {"Yard Muscle", "+10 max health", S_MAXHP, 10, 3}, {"Bruiser", "Take 5% less damage", S_ARMOR, .05f, 3}, {"Rage", "Bloodlust recharges 12% faster", S_CDR, .12f, 2}}},
      {"Street", {{"Five-Finger", "Better loot and drop odds", S_LUCK, .12f, 3}, {"Lookout", "The dead spot you 8% later", S_STEALTH, .08f, 3}, {"Sticky Fingers", "+15% ammo found", S_AMMO, .15f, 3}, {"Dirty Fighting", "Kicks hit 30% harder and send them flying", S_KICK, .30f, 2}}},
      {"Hard Time", {{"Hard to Kill", "Regenerate faster while healthy", S_REGEN, .08f, 3}, {"Cast Iron Gut", "Hunger and thirst 12% slower", S_METAB, .12f, 3}, {"Jailhouse Ink", "12% chance a hit doesn't bleed", S_BLEEDRES, .12f, 3}, {"Zip Gun", "+12% pistol and SMG damage", S_PISTOL, .12f, 2}}}}}};
static const ClassDef& CL() { return CLASSES[P.cls]; }
static int setCount(int si);
static void recalcPerks() {
    float st[S_COUNT] = {}; const ClassDef& C = CL();
    for (int t = 0; t < 3; t++) for (int i = 0; i < 4; i++) st[C.t[t].s[i].stat] += C.t[t].s[i].per * P.rank[t][i];
    Perks& k = P.perks; k = Perks();
    k.dmg = 1 + st[S_DMG]; k.rate = 1 + st[S_RATE]; k.crit = .06f + st[S_CRIT]; k.metab = 1 - st[S_METAB]; k.sta = 1 - st[S_STA]; k.luck = 1 + st[S_LUCK]; k.blast = 1 + st[S_BLAST]; k.cold = 1 - st[S_COLD];
    k.melee = 1 + st[S_MELEE]; k.noise = 1 - st[S_NOISE]; k.medic = 1 + st[S_MEDIC]; k.regen = st[S_REGEN]; k.maxhp = 100 + st[S_MAXHP]; k.reload = 1 + st[S_RELOAD]; k.armor = st[S_ARMOR]; k.speed = 1 + st[S_SPEED];
    k.pistol = 1 + st[S_PISTOL]; k.shotgun = 1 + st[S_SHOTGUN]; k.rifle = 1 + st[S_RIFLE]; k.stealth = st[S_STEALTH]; k.cdr = st[S_CDR]; k.ammo = 1 + st[S_AMMO]; k.batt = 1 - st[S_BATT]; k.bleedres = st[S_BLEEDRES]; k.block = st[S_BLOCK]; k.kick = 1 + st[S_KICK];
    // set bonuses
    int sc[5]; for (int i = 0; i < 5; i++) sc[i] = setCount(i);
    if (sc[0] >= 2) k.armor += .08f; if (sc[0] >= 4) { k.pistol += .12f; k.shotgun += .12f; k.reload += .1f; }
    if (sc[1] >= 2) k.medic += .4f; if (sc[1] >= 4) { k.bleedres += .15f; k.regen += .3f; }
    if (sc[2] >= 2) k.melee += .1f; if (sc[2] >= 4) { k.sta *= .85f; k.kick += .25f; }
    if (sc[3] >= 2) k.stealth += .08f; if (sc[3] >= 4) { k.rifle += .15f; k.noise *= .88f; }
    if (sc[4] >= 2) k.luck += .12f; if (sc[4] >= 4) { k.melee += .12f; k.kick += .3f; }
    if (P.cls >= 0 && P.cls < 5 && sc[P.cls] >= 2) k.cdr += .25f; // wearing your own class's set
}
// Fate picks: any skill whose rank isn't maxed and whose predecessor in its tree has at least one point.
static std::string randomSkillPoint() {
    std::vector<std::pair<int, int>> c; const ClassDef& C = CL();
    for (int t = 0; t < 3; t++) for (int i = 0; i < 4; i++) if (P.rank[t][i] < C.t[t].s[i].max && (i == 0 || P.rank[t][i - 1] > 0)) c.push_back({t, i});
    if (c.empty()) { P.perks.maxhp += 2; return "Every tree is full. +2 max health instead."; }
    auto pk = c[irand(0, (int)c.size() - 1)]; P.rank[pk.first][pk.second]++; P.points++; float mh = P.perks.maxhp; recalcPerks(); P.hp += P.perks.maxhp - mh;
    const Skill& sk = C.t[pk.first].s[pk.second];
    return std::string(C.t[pk.first].n) + ": " + sk.n + " " + std::to_string(P.rank[pk.first][pk.second]) + "/" + std::to_string(sk.max) + "  (" + sk.d + ")";
}
static void useActive() {
    if (P.actCd > 0) { floatText(xz3(P.p, 2.2f), std::string(CL().act) + " " + std::to_string((int)ceilf(P.actCd)) + "s", STEEL, 13); sfx(SFX_EMPTY); return; }
    P.actCd = CL().actCd * (1 - P.perks.cdr); sfx(SFX_LEVEL, .8f, 0);
    switch (P.cls) {
    case 0: P.actT = 6; msg("Adrenaline dump. Make it count.", CL().c); break;
    case 1: P.hp = std::min(P.perks.maxhp, P.hp + 40); P.bleeds = 0; P.pain = 0;
        if (P.broken) { P.broken = false; P.splinted = false; msg("Trauma kit: patched up and the leg's set properly. You can run again.", CL().c); showBanner("LEG FIXED", "Trauma kit", CL().c); }
        else msg("Trauma kit: patched up.", CL().c); break;
    case 2: throwItem(false, true); msg("Nail bomb out.", CL().c); break;
    case 3: P.actT = 8; msg("Deadeye. Breathe out.", CL().c); break;
    case 4: P.actT = 8; msg("Bloodlust.", CL().c); break;
    }
    floatText(xz3(P.p, 2.4f), CL().act, CL().c, 16);
}
static void grantLevelPoints() {
    while (pendingLv > 0) { pendingLv--; std::string r = randomSkillPoint(); showBanner("LEVEL " + std::to_string(P.lvl), "+1  " + r, SODIUM); msg("+1 " + r, SODIUM); sfx(SFX_LEVEL, 1, 0); }
}

// ============================================================ new game
static void dressHotZones() { // bodies and blood (decals/corpses are wiped after world gen, so this runs last)
    for (auto& b : blds) { if (b.cond != 2 || !b.enter || !chance(.45f)) continue; // abandoned houses: something bad happened here
        for (int k = 0; k < irand(2, 5); k++) decal(V2(frand(b.x0 + .6f, b.x1 - .6f), frand(b.z0 + .6f, b.z1 - .6f)), frand(.2f, .6f), BLOOD_DRY, frand(0, .5f));
        if (chance(.3f) && !lookPool[0].empty()) { Vector2 p = V2(frand(b.x0 + 1, b.x1 - 1), frand(b.z0 + 1, b.z1 - 1)); if (freeAt(p.x, p.y, .4f)) corpses.push_back({p, frand(0, 6), Z_WALKER, true, chance(.7f), true, 0, lookPool[0][irand(0, (int)lookPool[0].size() - 1)], frand(0, 1)}); } }
    for (auto& h : hotZones) { int n = h.kind == HZ_GRAVE ? 28 : irand(6, 11);
        for (int i = 0; i < n; i++) { Vector2 p = Vector2Add(h.c, polar(frand(0, 2 * PI), h.r * (h.kind == HZ_GRAVE ? .4f : .75f) * sqrtf(frand(0, 1)))); if (!freeAt(p.x, p.y, .4f)) continue;
            int pool = h.kind == HZ_CHECKPOINT || h.kind == HZ_EVAC ? (chance(.5f) ? 5 : 0) : 0; int look = lookPool[pool].empty() ? -1 : lookPool[pool][irand(0, (int)lookPool[pool].size() - 1)];
            corpses.push_back({p, frand(0, 2 * PI), pool == 5 ? Z_SOLDIER : Z_WALKER, chance(.8f), chance(.8f), chance(.85f), 0, look, frand(0, 1)});
            decal(p, frand(.3f, .6f), BLOOD_DRY, frand(.1f, .5f)); }
        for (int i = 0; i < 14; i++) decal(Vector2Add(h.c, polar(frand(0, 2 * PI), frand(0, h.r * .8f))), frand(.2f, .8f), BLOOD_DRY); }
}
static void resetHotbar();
static unsigned worldSeed = 0;
static void newGame(unsigned seed = 0) {
    worldSeed = seed ? seed : ((unsigned)time(nullptr) ^ (unsigned)(GetTime() * 1000)); rng.seed(worldSeed);
    genWorld(); genLoot();
    zombies.clear(); corpses.clear(); parts.clear(); gibs.clear(); bullets.clear(); bombs.clear(); acid.clear(); decals.clear(); decalHead = 0; floaters.clear(); lights.clear(); sdrops.clear(); msgs.clear();
    gTime = CYCLE * frand(.05f, .2f); phase = isNight() ? 'n' : 'd'; spawnT = 0; horde = {}; hordeDay = 0; pendingLv = 0; nights = 0; shake = 0; kills = 0; bestRar = -1; bestName.clear(); gibCount = 0;
    rainLevel = 0; rainTarget = 0; lastTown = -2; fires.clear();
    playerFemale = chance(.5f);
    P = Player(); P.next = xpNeed(1); P.p = pick(spawnPts); P.a = atan2f(TOWNS[0].c.y - P.p.y, TOWNS[0].c.x - P.p.x);
    P.cls = irand(0, 4); recalcPerks(); std::string s1 = randomSkillPoint(), s2 = randomSkillPoint(); P.hp = P.perks.maxhp;
    P.melee = MK_FISTS; P.active = 2; P.batt = 35; dressStart(); { static bool hb = false; if (!hb) { resetHotbar(); hb = true; } } // your hotbar layout survives death // everyone starts with fists and rags. Your class is what you know, not what you carry
    camPos = P.p;
    for (int i = 0; i < 6; i++) spawnAround(Z_WALKER, 30, 70);
    dressHotZones();
    showBanner(std::string("YOU ARE ") + (strchr("AEIOUaeiou", CL().n[0]) ? "AN " : "A ") + CL().n, s1 + "   /   " + s2, CL().c);
    msg("Class: " + std::string(CL().n) + ". [Z] " + CL().act + ", [K] skills.", CL().c);
    msg("Fists, rags and whatever's in your pockets. [Tab] inventory. Clothes are your storage.", BONE);
    msg("The county's been redrawn. Same towns, new places. Check the map.", BONE);
    state = ST_PLAY;
}

// ============================================================ actions
static Vector2 aimPoint() {
    Ray r = GetScreenToWorldRay(GetMousePosition(), cam3); float y = 1.2f;
    if (fabsf(r.direction.y) < 1e-4f) return P.p; float t = (y - r.position.y) / r.direction.y; return V2(r.position.x + r.direction.x * t, r.position.z + r.direction.z * t);
}
static Weapon* curGun() { return P.active < 2 && P.hasGun[P.active] ? &P.gun[P.active] : nullptr; }
static int curGunHold() { Weapon* g = curGun(); return g ? (g->slot == 1 ? 1 : 2) : 0; }
static void finishReloadNow();
static void startReload() {
    Weapon* w = curGun(); if (!w || P.reload > 0 || w->clip >= w->mag) return;
    if (invCount(ammoItem(w->cal)) <= 0) { floatText(xz3(P.p, 2.2f), std::string("NO ") + CALN[w->cal], rgba(255, 107, 91)); sfx(SFX_EMPTY); return; }
    if (P.actT > 0 && P.cls == 0) { finishReloadNow(); return; }
    P.reload = w->rel / P.perks.reload; sfx(SFX_RELOAD);
}
static void finishReload() { Weapon* w = curGun(); if (!w) return; w->clip += invTake(ammoItem(w->cal), w->mag - w->clip); }
static void finishReloadNow() { finishReload(); sfx(SFX_RELOAD, .7f); }
static void setActive(int i) { if (i == P.active) return; if (i < 2 && !P.hasGun[i]) return; P.active = i; P.reload = 0; P.cd = .2f; sfx(SFX_RELOAD, .5f); }
static bool tryVault();
static void dodge() {
    if (P.vaultT > 0 || P.loft >= 0) return; if (tryVault()) return;
    if (P.roll > 0 || P.rollCd > 0 || P.broken) return; float cost = 25; if (P.sta < cost) return; P.sta -= cost; P.staD = .8f;
    Vector2 in = V2((float)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A)), (float)(IsKeyDown(KEY_S) - IsKeyDown(KEY_W))); if (Vector2Length(in) < .1f) in = polar(P.a, 1);
    P.rv = Vector2Scale(Vector2Normalize(in), 10); P.roll = .28f; P.iframe = .32f; P.rollCd = .45f; P.crouch = false; sfx(SFX_ROLL);
}
static void shove() { // quick two-handed push for the one that's in your face
    if (P.shoveCd > 0 || P.roll > 0 || P.loft >= 0 || P.sta < 6) return; P.sta -= 6; P.staD = .4f; P.shoveCd = .4f; P.kickT = 0; bool hit = false;
    for (auto& z : zombies) { if (z.dead) continue; float d = Vector2Distance(z.p, P.p), a = atan2f(z.p.y - P.p.y, z.p.x - P.p.x);
        if (d < 1.4f + z.r && fabsf(angDiff(a, P.a)) < 1.1f) { z.k = Vector2Add(z.k, polar(a, (z.type == Z_BRUTE ? 1.5f : 5.5f) * P.perks.kick)); z.stun = std::max(z.stun, z.type == Z_BRUTE ? .1f : .5f); z.atk = std::max(z.atk, .5f); hit = true; } }
    sfx(hit ? SFX_HIT_PUNCH : SFX_SWING, hit ? .7f : .35f, .2f); P.blockFx = hit ? .08f : 0;
}
static bool tryVault() { // Space into a low obstacle (fence, crate, car, rock, woodpile) hops it
    Vector2 dir = P.moving ? polar(P.moveA, 1) : polar(P.a, 1); float hb = 0; float tObs = -1;
    for (float t = .35f; t <= 1.2f; t += .15f) { Vector2 q = Vector2Add(P.p, Vector2Scale(dir, t));
        for (int i : sgrid[gcl(q.x)][gcl(q.y)]) { const Solid& sd = solids[i]; if (!solidOn(sd) || q.x < sd.x0 || q.x > sd.x1 || q.y < sd.z0 || q.y > sd.z1) continue;
            if (sd.h > 1.75f || sd.kind == SK_WALL || sd.kind == SK_TOWER || sd.kind == SK_TRUNK || sd.kind == SK_TENT) return false; hb = std::max(hb, sd.h); tObs = t; }
        if (tObs > 0) break; }
    if (tObs < 0) return false;
    for (float t = tObs + .3f; t < tObs + 3.5f; t += .15f) { Vector2 q = Vector2Add(P.p, Vector2Scale(dir, t)); if (freeAt(q.x, q.y, .36f)) {
        if (P.sta < 10) return true; P.sta -= 10; P.staD = .6f; P.vaultA = P.p; P.vaultB = q; P.vaultT = .45f; P.vaultH = hb; P.iframe = .2f; P.crouch = false; P.autorun = false; sfx(SFX_PICK, .8f); sfx(SFX_STEP, .6f); return true; } }
    return false;
}
static void kick() {
    if (P.kickCd > 0 || P.roll > 0 || P.loft >= 0 || P.sta < 14 || P.broken) return; P.sta -= 14; P.staD = .6f; P.kickCd = .75f; P.kickT = .35f; bool hit = false;
    for (auto& z : zombies) { if (z.dead) continue; float d = Vector2Distance(z.p, P.p), a = atan2f(z.p.y - P.p.y, z.p.x - P.p.x);
        if (d < 1.7f + z.r && fabsf(angDiff(a, P.a)) < .95f) { float kb = (z.type == Z_BRUTE ? 2.5f : 8.f) * P.perks.kick; hurtZ(z, 9 * P.perks.kick * P.perks.melee, false, a, false); z.k = Vector2Add(z.k, polar(a, kb)); z.stun = z.type == Z_BRUTE ? .25f : .8f; hit = true; } }
    sfx(hit ? SFX_HIT_PUNCH : SFX_SWING, hit ? 1.f : .5f, .15f); if (hit) { sfx(SFX_HIT_HEAVY, .5f, .1f); shake = std::min(.35f, shake + .12f); }
    if (!hit) for (auto& d : doorObjs) if (!d.open && !d.broken && Vector2Distance(d.c, P.p) < 1.9f) { d.open = true; sfx(SFX_DOOR_BANG); noiseAt(d.c, 15); break; } // boot a closed door open
    noiseAt(P.p, 8 * P.perks.noise);
}
static void melee() {
    const MBase& m = MB[P.melee]; if (P.swingCd > 0 || P.roll > 0 || P.sta < m.sta || P.blocking) return;
    P.sta -= m.sta; P.staD = .6f; P.swing = .22f; P.swingCd = m.rate; P.swingA = P.a; sfx(SFX_SWING); bool hit = false;
    for (auto& z : zombies) {
        if (z.dead) continue; float d = Vector2Distance(z.p, P.p), a = atan2f(z.p.y - P.p.y, z.p.x - P.p.x);
        if (d < m.range + z.r && (fabsf(angDiff(a, P.a)) < m.arc || d < z.r + .75f)) { /* point-blank always connects */ bool rage = P.actT > 0 && P.cls == 4; bool hd = chance(P.perks.crit + .08f); hurtZ(z, m.dmg * P.perks.melee * P.perks.dmg * (rage ? 2 : 1), hd, a, false, m.dism + (rage ? .2f : 0)); if (rage) P.hp = std::min(P.perks.maxhp, P.hp + 4); z.k = Vector2Add(z.k, polar(a, 4)); hit = true; }
    }
    if (hit) { static const int mat[MK_COUNT] = {SFX_HIT_PUNCH, SFX_HIT_SLICE, SFX_HIT_SLICE, SFX_HIT_HEAVY, SFX_HIT_WOOD, SFX_HIT_HEAVY, SFX_HIT_HEAVY, SFX_HIT_HEAVY, SFX_HIT_HEAVY}; sfx(mat[P.melee], 1, .1f); if (canChop(P.melee)) sfx(SFX_CHOP, .6f); shake = std::min(.4f, shake + .15f); }
    else if (canChop(P.melee)) { // nothing alive in reach: chop the nearest tree in front of you
        Tree* tb = nullptr; float bd = 1e9f;
        for (int cx = gcl(P.p.x - 4); cx <= gcl(P.p.x + 4); cx++) for (int cz = gcl(P.p.y - 4); cz <= gcl(P.p.y + 4); cz++) for (int ti : tgrid[cx][cz]) { Tree& t = trees[ti];
            float d = Vector2Distance(t.p, P.p), a = atan2f(t.p.y - P.p.y, t.p.x - P.p.x); if (d < m.range + .5f && fabsf(angDiff(a, P.a)) < m.arc && d < bd) { bd = d; tb = &t; } }
        if (tb) { tb->chops++; tb->shake = .35f; sfx(SFX_CHOP, 1, .15f); for (int i = 0; i < 6; i++) parts.push_back({xz3(Vector2Lerp(P.p, tb->p, .8f), 1.0f), V3(frand(-2, 2), frand(1, 3), frand(-2, 2)), .5f, .5f, .05f, rgba(200, 160, 110), P_DUST});
            if (tb->logs >= 3) msg("Nothing left worth taking from this one.", STEEL);
            else if (tb->chops % 4 == 0) { tb->logs++; Loot l; l.kind = LK_ITEM; l.id = IT_LOG; l.count = 1; l.p = xz3(Vector2Lerp(tb->p, P.p, .5f), .05f); l.rot = frand(0, 6); loot.push_back(l); msg("A log comes loose.", BONE); } }
    }
    noiseAt(P.p, 6 * P.perks.noise);
}
static bool fire() {
    Weapon* wp = curGun(); if (!wp) { melee(); return false; } Weapon& w = *wp;
    if (P.reload > 0 || P.cd > 0 || P.roll > 0 || P.swing > 0 || P.blocking) return false;
    if (w.clip <= 0) { startReload(); if (P.reload <= 0) sfx(SFX_EMPTY); return false; }
    w.clip--; P.firedT = 2; P.cd = w.rate / (P.perks.rate * (P.actT > 0 && P.cls == 0 ? 1.6f : 1)); float sm = (P.sprint ? 1.8f : P.crouch ? .6f : 1) * (1 + P.shiver * .8f) * (P.pain > 0 ? 1.4f : 1);
    Vector2 m0 = Vector2Add(P.p, polar(P.a, .55f + w.len));
    for (int i = 0; i < w.pel; i++) { float a = P.a + frand(-w.spread, w.spread) * sm, sp = w.spd * frand(.94f, 1.04f); Bullet b{}; b.p = m0; b.v = polar(a, sp); b.dmg = w.dmg * P.perks.dmg * (w.key == W_SHOTGUN ? P.perks.shotgun : w.cal == C_9MM ? P.perks.pistol : P.perks.rifle); b.life = w.range / w.spd; b.pierce = w.pierce;
        b.burn = w.burn; b.leech = w.leech; b.crit = w.crit; b.dism = w.dism; b.shotgun = w.key == W_SHOTGUN; bullets.push_back(b); }
    shake = std::min(.6f, shake + w.kick * .02f); lights.push_back({xz3(m0, 1.3f), 14, .06f, .06f, rgba(255, 200, 120)});
    for (int i = 0; i < 5; i++) { Vector2 h = polar(P.a + frand(-.4f, .4f), frand(2, 9)); parts.push_back({xz3(m0, 1.3f), V3(h.x, frand(-.5f, .8f), h.y), .07f, .07f, .05f, rgba(255, 210, 120), P_SPARK}); }
    { Vector2 h = polar(P.a + 1.7f, frand(1.5f, 3)); parts.push_back({xz3(P.p, 1.3f), V3(h.x, 2, h.y), .8f, .8f, .025f, rgba(214, 170, 80), P_CASING}); }
    parts.push_back({xz3(m0, 1.3f), V3(0, .4f, 0), .5f, .5f, .25f, rgba(140, 140, 145, 120), P_SMOKE});
    P.k = Vector2Subtract(P.k, polar(P.a, w.kick * .25f));
    noiseAt(P.p, w.noise); static const int sid[3] = {SFX_PISTOL, SFX_SMG, SFX_SHOTGUN}; sfx(w.key == W_HUNT || w.cal == C_556 ? SFX_RIFLE : sid[w.snd], w.cal == C_556 ? .75f : 1);
    if (w.clip == 0) startReload(); return true;
}
static void throwItem(bool flareK, bool free) {
    int id = flareK ? IT_FLARE : IT_PIPEBOMB; if (!free && !invTake(id, 1)) { msg(std::string("No ") + IDEF[id].n + "s.", STEEL); return; }
    Vector2 tgt = aimPoint(); float d = std::min(22.f, Vector2Distance(tgt, P.p)); float t = .9f; Vector2 h = polar(P.a, d / t);
    bombs.push_back({xz3(P.p, 1.5f), V3(h.x, 4.5f, h.y), flareK ? 60.f : 3.2f, 0, 0, flareK}); sfx(SFX_THROW);
}
static void toggleLight() { if (!P.light && P.batt <= 0) { msg("Flashlight battery is dead.", UVC); return; } P.light = !P.light; if (!P.light) P.uv = false; }
static void buildFire() {
    if (buildingAt(P.p) >= 0) { msg("Not indoors. You'd smoke yourself out.", STEEL); return; }
    if (invCount(IT_KINDLING) < 3) { msg("A fire needs 3 kindling.", STEEL); return; }
    if (invCount(IT_MATCHES) < 1) { msg("Nothing to light it with. Find matches.", STEEL); return; }
    Vector2 p = Vector2Add(P.p, polar(P.a, 1.1f)); if (!freeAt(p.x, p.y, .6f)) { msg("No room for a fire here.", STEEL); return; }
    invTake(IT_KINDLING, 3); invTake(IT_MATCHES, 1); fires.push_back({p, 150}); msg("Fire's going. Feed it kindling or logs with E.", rgba(255, 170, 90)); sfx(SFX_THROW);
}
static void craftSplint() {
    if (invCount(IT_KINDLING) < 2 || invCount(IT_BANDAGE) + invCount(IT_RAG) < 1) { msg("Splint: 2 kindling + 1 bandage or rag.", STEEL); return; }
    invTake(IT_KINDLING, 2); if (!invTake(IT_RAG, 1)) invTake(IT_BANDAGE, 1); sfx(SFX_PICK);
    if (P.broken && !P.splinted) { P.splinted = true; P.splintT = 0; msg("Made a splint and strapped it on. The leg will come back slowly.", OKC); showBanner("LEG SPLINTED", "Slow at first, better in 12 hours", OKC); return; }
    if (invAdd(IT_SPLINT, 1)) { spawnItem(xz3(P.p, .05f), IT_SPLINT, 1); msg("Made a splint. No room, so it's on the ground.", SODIUM); } else msg("Made a splint. It's in your pockets for when you need it.", OKC);
}
static float legFactor() { // broken: 45%. splint: 52% -> 70% over 12 in-game hours -> 100% over 2 more days
    if (!P.broken) return 1; if (!P.splinted) return .45f; float t = P.splintT, h12 = CYCLE / 2, d2 = CYCLE * 2;
    if (t < h12) return .52f + .18f * t / h12; return .70f + .30f * std::min(1.f, (t - h12) / d2);
}

static void useItem(InvItem* it);
static void quickUse(bool meds) { // B: bandage (or a rag). N: whatever you need most
    InvItem* sl = nullptr;
    if (!meds) { sl = invFind(IT_BANDAGE); if (!sl) sl = invFind(IT_RAG); if (!sl) { msg("No bandages or rags.", STEEL); return; } }
    else if (P.broken && !P.splinted && invFind(IT_SPLINT)) sl = invFind(IT_SPLINT);
    else if (P.broken && invFind(IT_MORPHINE)) sl = invFind(IT_MORPHINE);
    else if (P.sick > 0 && invFind(IT_ANTIBIO)) sl = invFind(IT_ANTIBIO);
    else if (invFind(IT_PAINKILL) && (P.pain > 0 || P.hp < P.perks.maxhp * .7f)) sl = invFind(IT_PAINKILL);
    else if (P.broken && !P.splinted) { msg(invCount(IT_KINDLING) >= 2 && (invCount(IT_BANDAGE) || invCount(IT_RAG)) ? "Make a splint in the inventory (Tab)." : "No meds for that leg. Splint = 2 kindling + bandage or rag.", STEEL); return; }
    if (!sl) { msg("Nothing useful to take.", STEEL); return; } useItem(sl);
}
static void useItem(InvItem* it) {
    if (!it || it->kind != 0) return; int id = it->id; const ItemDef& d = IDEF[id]; bool used = true;
    switch (id) {
    case IT_BANDAGE: case IT_RAG: if (P.bleeds > 0) { bool rag = id == IT_RAG; if (rag && chance(.35f)) { msg("The rag soaks through. Still bleeding.", BLOODC); break; } P.bleeds = std::max(0, P.bleeds - (P.perks.medic > 1 ? 2 : 1)); P.hp = std::min(P.perks.maxhp, P.hp + (rag ? 1 : 4) * P.perks.medic); msg(P.bleeds ? "Wrapped one wound." : "Bleeding stopped.", OKC); } else { used = false; msg("You're not bleeding.", STEEL); } break;
    case IT_ANTIBIO: if (P.sick > 0) { P.sick = 0; msg("The fever breaks.", OKC); } else { used = false; msg("You're not sick.", STEEL); } break;
    case IT_PAINKILL: P.pain = 0; P.hp = std::min(P.perks.maxhp, P.hp + 5); msg("Painkillers kick in.", OKC); break;
    case IT_MORPHINE: if (P.broken) { P.broken = false; P.splinted = false; msg("Morphine and a hard yank. Leg's set. You can run again.", OKC); } else { P.pain = 0; P.hp = std::min(P.perks.maxhp, P.hp + 10); } break;
    case IT_BATTERY: if (P.batt >= 100) { used = false; msg("Battery is already full.", STEEL); } else { P.batt = 100; msg("Fresh battery in the flashlight.", UVC); } break;
    case IT_FLARE: case IT_PIPEBOMB: used = false; throwItem(id == IT_FLARE); return;
    case IT_SPLINT: if (!P.broken) { used = false; msg("Nothing's broken. Yet.", STEEL); } else if (P.splinted) { used = false; msg("Already splinted. Give it time.", STEEL); } else { P.splinted = true; P.splintT = 0; msg("Leg splinted. It'll come back slowly.", OKC); sfx(SFX_PICK); } break;
    case IT_LOG: if (!canChop(P.melee)) { used = false; msg("You need an axe or hatchet in hand to split it.", STEEL); } else { it->n--; invPrune(); if (invAdd(IT_KINDLING, 4)) msg("No room for all the kindling.", STEEL); else msg("Split the log into kindling.", OKC); sfx(SFX_CHOP); return; } break;
    case IT_KINDLING: used = false; buildFire(); return;
    default:
        if (d.food != 0 || d.water != 0) { P.food = Clamp(P.food + d.food, 0, 110); P.water = Clamp(P.water + d.water, 0, 110); msg(std::string(d.use) + ": " + d.n, OKC); sfx(SFX_EAT); }
        else { used = false; if (d.use[0]) msg(std::string("Right-click the ") + d.n + " to pick what to use it on.", STEEL); }
    }
    if (used) { it->n--; invPrune(); }
}
static bool toolFits(int id, const Cloth& c) { // what a repair/upgrade item can do to a piece of clothing
    switch (id) {
    case IT_RAG: return c.cond < .6f && c.cond > 0; case IT_SEWKIT: return c.cond < .95f; case IT_TAPE: return c.cond < .75f;
    case IT_LEATHER: return c.upg < 3 && CD[c.def].slot != CS_EYES && CD[c.def].slot != CS_SOCKS; case IT_WAX: return c.bWater < .5f && CD[c.def].slot != CS_EYES;
    case IT_DYE_BLACK: case IT_DYE_GREEN: case IT_DYE_BROWN: return CD[c.def].slot != CS_EYES; } return false; }
static void applyTool(InvItem* tool, Cloth& c) {
    if (!tool || !toolFits(tool->id, c)) { msg("That won't help it.", STEEL); return; } std::string n = c.name;
    switch (tool->id) {
    case IT_RAG: c.cond = std::min(.6f, c.cond + .12f); msg("Patched the " + n + " with a rag.", OKC); break;
    case IT_SEWKIT: c.cond = std::min(.95f, std::max(.05f, c.cond) + .5f); msg("Stitched up the " + n + ".", OKC); break;
    case IT_TAPE: c.cond = std::min(.75f, std::max(.05f, c.cond) + .3f); msg("Duct-taped the " + n + ". Classy.", OKC); break;
    case IT_LEATHER: c.bArmor += .07f; c.upg++; c.cond = std::min(1.f, c.cond + .08f); msg("Sewed leather onto the " + n + ". Harder to bite through.", OKC); break;
    case IT_WAX: c.bWater = std::min(.5f, c.bWater + .3f); msg("Waxed the " + n + ". Rain beads right off.", OKC); break;
    default: { Color dc = tool->id == IT_DYE_BLACK ? rgba(34, 34, 38) : tool->id == IT_DYE_GREEN ? rgba(78, 92, 54) : rgba(96, 70, 44); c.col = lerpC(c.col, dc, .8f); c.bVis = std::min(c.bVis, tool->id == IT_DYE_BLACK ? .94f : .9f);
        if (c.pattern == 1) c.pattern = 0; msg("Dyed the " + n + ". Less of a target now.", OKC); } }
    tool->n--; invPrune(); sfx(SFX_PICK);
}
static void dropCloth(std::shared_ptr<Cloth> c, Vector2 at) { Loot l; l.kind = LK_CLOTH; l.id = c->def; l.count = 1; l.c = c; l.p = xz3(at, .05f); l.rot = frand(0, 6); loot.push_back(l); }
static void spawnCloth(Vector3 p, std::shared_ptr<Cloth> c) { Loot l; l.kind = LK_CLOTH; l.id = c->def; l.count = 1; l.c = c; l.p = p; l.rot = frand(0, 6); loot.push_back(l); }
static bool takeCloth(std::shared_ptr<Cloth> c) { // equip if the slot is free, otherwise stuff it in a pocket
    int s = CD[c->def].slot; if (!P.eq[s]) { P.eq[s] = c; msg("Put on " + c->name + " (" + CONDN[condTier(c->cond)] + ")", RARC[c->rar]); return true; }
    InvItem it; it.kind = 1; it.c = c; if (placeAnywhere(it)) { msg("Stowed " + c->name, RARC[c->rar]); return true; } return false; }
static void pickUp() {
    if (nearLoot < 0 || nearLoot >= (int)loot.size()) return; Loot l = loot[nearLoot];
    if (l.kind == LK_ITEM) { int left = invAdd(l.id, l.count); if (left == l.count) { msg("No room. Find clothes with more pockets.", rgba(255, 107, 91)); return; } msg("Picked up " + std::string(IDEF[l.id].n) + (l.count - left > 1 ? " x" + std::to_string(l.count - left) : ""), BONE);
        if (left > 0) loot[nearLoot].count = left; else loot.erase(loot.begin() + nearLoot); sfx(SFX_PICK); return; }
    if (l.kind == LK_CLOTH) { if (!takeCloth(l.c)) { msg("No room for that. Open the inventory (Tab) to swap.", rgba(255, 107, 91)); return; } loot.erase(loot.begin() + nearLoot); sfx(SFX_PICK); return; }
    loot.erase(loot.begin() + nearLoot);
    if (l.kind == LK_GUN) { int s = l.w.slot; if (P.hasGun[s]) { Loot o; o.kind = LK_GUN; o.id = P.gun[s].key; o.w = P.gun[s]; o.p = xz3(P.p, .05f); o.rot = frand(0, 6); loot.push_back(o); }
        P.gun[s] = l.w; P.hasGun[s] = true; P.active = s; P.reload = 0; msg("Equipped " + l.w.name, RARC[l.w.rar]); if (l.w.rar == 3) sfx(SFX_LEGEND, 1, 0); else sfx(SFX_PICK); }
    else { if (P.melee != MK_FISTS) { Loot o; o.kind = LK_MELEE; o.id = P.melee; o.p = xz3(P.p, .05f); o.rot = frand(0, 6); loot.push_back(o); } P.melee = l.id; msg("Equipped " + std::string(MB[l.id].n), BONE); sfx(SFX_PICK); }
}
static int outfitLook() {
    int pl = playerLooks[playerFemale && playerLooks[1] >= 0 ? 1 : 0]; if (pl < 0) return pl; Look& L = looks[pl]; if (L.col0.empty()) return pl;
    auto wear = [](int s) -> Color { auto& c = P.eq[s]; Color k = c->col; if (c->pattern == 2) k = mulC(k, .85f); float dirt = (1 - c->cond) * .45f; return lerpC(k, rgba(84, 72, 58), dirt); };
    Color skin = rgba(206, 160, 130); for (int i = 0; i < (int)L.col0.size(); i++) if (hasStr(assets[L.asset].matName[i], "Skin")) { skin = L.col0[i]; break; }
    Color top = P.eq[CS_OUTER] ? wear(CS_OUTER) : P.eq[CS_TORSO] ? wear(CS_TORSO) : skin, legs = P.eq[CS_LEGS] ? wear(CS_LEGS) : mulC(skin, .95f), feet = P.eq[CS_FEET] ? wear(CS_FEET) : P.eq[CS_SOCKS] ? wear(CS_SOCKS) : skin;
    for (int i = 0; i < (int)L.col.size(); i++) { int z = L.zone[i]; Color b = L.col0[i]; float lum = (b.r + b.g + b.b) / 3.f / 255.f, sh = .75f + .5f * std::min(1.f, lum * 1.6f); // keep the model's light/dark detail
        L.col[i] = z == 1 ? mulC(top, std::min(1.15f, sh)) : z == 2 ? mulC(legs, std::min(1.15f, sh)) : z == 3 ? mulC(feet, std::min(1.1f, sh)) : b; }
    return pl;
}

// ============================================================ interaction (E): loot first, then doors, ladders, fires, cars
enum ActKind { A_NONE, A_DOOR, A_LADDER, A_FIRE, A_CAR, A_STAIRS };
struct Act { int kind = A_NONE, idx = -1; }; static Act nearAct;
static Act findAct() {
    Act a; float best = 1e9f;
    if (P.loft >= 0) { if (Vector2Distance(P.p, lofts[P.loft].top) < 1.6f) { a.kind = A_LADDER; a.idx = P.loft; } return a; }
    for (int i = 0; i < (int)doorObjs.size(); i++) { const DoorObj& d = doorObjs[i]; if (d.broken) continue; float dd = Vector2Distance(d.c, P.p); if (dd < 2.2f && dd < best) { best = dd; a = {A_DOOR, i}; } }
    for (int i = 0; i < (int)lofts.size(); i++) { float dd = Vector2Distance(lofts[i].base, P.p); if (dd < 1.6f && dd < best) { best = dd; a = {A_LADDER, i}; } }
    for (int i = 0; i < (int)stairs.size(); i++) { float dd = std::min(Vector2Distance(stairs[i].a, P.p), Vector2Distance(stairs[i].b, P.p)); if (dd < 1.9f && dd < best) { best = dd; a = {A_STAIRS, i}; } }
    if (invCount(IT_KINDLING) || invCount(IT_LOG)) for (int i = 0; i < (int)fires.size(); i++) { float dd = Vector2Distance(fires[i].p, P.p); if (dd < 2.2f && dd < best) { best = dd; a = {A_FIRE, i}; } }
    if (a.kind == A_NONE) for (int i = 0; i < (int)carObjs.size(); i++) { const CarObj& c = carObjs[i]; if (c.searched) continue; float dd = Vector2Distance(c.p, P.p); if (dd < 3.f && dd < best) { best = dd; a = {A_CAR, i}; } }
    return a;
}
static std::string actLabel(const Act& a) {
    switch (a.kind) { case A_DOOR: return doorObjs[a.idx].open ? "Close door" : "Open door"; case A_LADDER: return P.loft >= 0 ? "Climb down" : "Climb to the hayloft";
        case A_FIRE: return invCount(IT_LOG) ? "Throw a log on the fire" : "Feed the fire kindling"; case A_CAR: return carObjs[a.idx].burned ? "Pick through the wreck" : "Search car";
        case A_STAIRS: { const Stair& st = stairs[a.idx]; bool atA = Vector2Distance(st.a, P.p) < Vector2Distance(st.b, P.p); return st.down ? (atA ? "Go down to the basement" : "Climb out of the basement") : (atA ? "Go upstairs" : "Go downstairs"); } }
    return "";
}
static bool bodyInDoor(const DoorObj& d) {
    auto in = [&](Vector2 p, float r) { return d.horiz ? fabsf(p.y - d.c.y) < .25f + r && fabsf(p.x - d.c.x) < d.w / 2 + r : fabsf(p.x - d.c.x) < .25f + r && fabsf(p.y - d.c.y) < d.w / 2 + r; };
    if (in(P.p, .35f)) return true; for (auto& z : zombies) if (in(z.p, z.r)) return true; return false;
}
struct Follower { int type; float t; Vector2 to; int cell; }; static std::vector<Follower> followers; // zombies coming after you through the stairs
static void takeStairs(int si) {
    const Stair& st = stairs[si]; bool atA = Vector2Distance(st.a, P.p) < Vector2Distance(st.b, P.p); Vector2 from = atA ? st.a : st.b, to = atA ? st.b : st.a; int toB = atA ? st.bB : st.bA;
    for (auto& z : zombies) if (!z.dead && z.aggro > 0 && Vector2Distance(z.p, from) < 26) { followers.push_back({z.type, Vector2Distance(z.p, from) / 2.6f + frand(1.5f, 4), to, toB}); z.gone = true; }
    Building& tb = blds[toB]; Vector2 ctr = V2((tb.x0 + tb.x1) / 2, (tb.z0 + tb.z1) / 2); Vector2 dir = Vector2Normalize(Vector2Subtract(ctr, to)); Vector2 dst = Vector2Add(to, Vector2Scale(dir, 1.5f));
    if (!freeAt(dst.x, dst.y, .35f)) for (int k = 0; k < 16; k++) { Vector2 q = Vector2Add(to, polar(k * PI / 8, 1.5f)); if (freeAt(q.x, q.y, .35f)) { dst = q; break; } }
    P.p = dst; camPos = P.p; P.k = V2(0, 0); P.vel = V2(0, 0); P.autorun = false; sfx(SFX_STEP, .6f); sfx(SFX_DOOR_OPEN, .3f);
    if (tb.cellKind && !tb.visited) { tb.visited = true; int n = tb.cellKind == 2 ? (chance(.55f) ? irand(1, 3) : 0) : (chance(.35f) ? irand(1, 2) : 0); // someone was already down here
        for (int k = 0; k < n; k++) for (int t = 0; t < 20; t++) { Vector2 q = V2(frand(tb.x0 + 1, tb.x1 - 1), frand(tb.z0 + 1, tb.z1 - 1)); if (Vector2Distance(q, P.p) > 3.5f && freeAt(q.x, q.y, .4f)) { Zombie& z = spawnZ(Z_WALKER, q); z.aggro = 0; break; } } }
    msg(tb.cellKind == 2 ? "The basement. Smells like mildew and worse." : tb.cellKind == 1 ? "Upstairs." : "Back on the ground floor.", STEEL);
}
static void doAct(const Act& a) {
    switch (a.kind) {
    case A_STAIRS: takeStairs(a.idx); return;
    case A_DOOR: { DoorObj& d = doorObjs[a.idx]; if (d.open && bodyInDoor(d)) { msg("Something's in the doorway.", STEEL); return; } d.open = !d.open; sfx(d.open ? SFX_DOOR_OPEN : SFX_DOOR_CLOSE); noiseAt(d.c, 6); break; }
    case A_LADDER: if (P.loft >= 0) { P.p = lofts[P.loft].base; P.loft = -1; msg("Back on the ground.", STEEL); } else { P.loft = a.idx; P.p = lofts[a.idx].top; P.crouch = true; msg("Up in the hayloft. Stay quiet and they'll lose interest.", OKC); } sfx(SFX_CREAK); break;
    case A_FIRE: { Fire& f = fires[a.idx]; if (invTake(IT_LOG, 1)) { f.fuel = std::min(900.f, f.fuel + 260); msg("Log on the fire.", rgba(255, 170, 90)); } else if (invTake(IT_KINDLING, 1)) { f.fuel = std::min(900.f, f.fuel + 60); msg("Fed the fire.", rgba(255, 170, 90)); } sfx(SFX_PICK); break; }
    case A_CAR: { CarObj& c = carObjs[a.idx]; c.searched = true; sfx(SFX_DOOR_OPEN); Vector3 drop = xz3(Vector2Add(P.p, polar(P.a + PI, .5f)), .05f);
        if (c.burned) { if (chance(.2f)) { rollTable(drop, LT_CAR); msg("Something survived the fire.", BONE); } else msg("Burned out. Nothing left but ash.", STEEL); }
        else { int n = 0; size_t before = loot.size(); rollTable(drop, LT_CAR); if (chance(.5f)) rollTable(drop, LT_CAR); n = (int)(loot.size() - before); msg(n ? "Found something in the car." : "Glovebox and trunk: nothing.", n ? BONE : STEEL); }
        noiseAt(c.p, 8); break; }
    }
}
static void interact() { if (nearLoot >= 0) { pickUp(); return; } if (nearAct.kind != A_NONE) doAct(nearAct); }

// ============================================================ update
static std::vector<int> zg[GN * 2][GN * 2]; static std::vector<int> znb;
static int zcl(float v) { return std::max(0, std::min(GN * 2 - 1, (int)(v / (GCELL / 2)))); }
static std::vector<int>& zNear(Vector2 p) { znb.clear(); int cx = zcl(p.x), cz = zcl(p.y); for (int i = cx - 1; i <= cx + 1; i++) for (int j = cz - 1; j <= cz + 1; j++) { if (i < 0 || j < 0 || i >= GN * 2 || j >= GN * 2) continue; for (int k : zg[i][j]) znb.push_back(k); } return znb; }
static float stepT = 0;
static void updatePlayer(float dt) {
    P.alive += dt;
    Vector2 in = V2((float)((IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) - (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))), (float)((IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) - (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))));
    float il = Vector2Length(in); if (il > 0) in = Vector2Scale(in, 1 / il);
    Vector2 ap = aimPoint(); P.a = atan2f(ap.y - P.p.y, ap.x - P.p.x);
    // click-to-move (Diablo): left-click the ground to walk, a zombie to go hit it, loot to go grab it. WASD still overrides
    static int cm = 0, cmZ = -1; static Vector2 cmPt{}; bool cmSwing = false, cmFire = false, cmPick = false;
    if (cfg.controls == 2 && state == ST_PLAY && !uiOpen && P.loft < 0) {
        bool lp = IsMouseButtonPressed(MOUSE_BUTTON_LEFT), ld = IsMouseButtonDown(MOUSE_BUTTON_LEFT), alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT); Vector2 gp = ap;
        int onZ = -1; float bz = 1.1f; for (auto& z : zombies) if (!z.dead) { float dd = Vector2Distance(z.p, gp); if (dd < bz) { bz = dd; onZ = z.id; } }
        if (lp && !alt) { if (onZ >= 0) { cm = 2; cmZ = onZ; } else { float bl = .9f; int li = -1; for (int i = 0; i < (int)loot.size(); i++) { float dd = Vector2Distance(V2(loot[i].p.x, loot[i].p.z), gp); if (dd < bl) { bl = dd; li = i; } }
                if (li >= 0) { cm = 3; cmPt = V2(loot[li].p.x, loot[li].p.z); } else { cm = 1; cmPt = gp; } } }
        if (ld && cm == 1 && !alt) cmPt = gp; // hold to keep walking toward the cursor
        if (il > 0) cm = 0;
        bool gun = P.active < 2 && P.hasGun[P.active];
        cmFire = ld && (alt || (gun && onZ >= 0) || (gun && cm == 2));
        if (cm == 2) { Zombie* t = nullptr; for (auto& z : zombies) if (z.id == cmZ && !z.dead) t = &z; if (!t) cm = 0;
            else { float dd = Vector2Distance(t->p, P.p); P.a = atan2f(t->p.y - P.p.y, t->p.x - P.p.x);
                if (gun) { cmFire = ld || cmFire; if (!ld) cm = 0; } else if (dd > MB[P.melee].range + t->r - .25f) { in = Vector2Normalize(Vector2Subtract(t->p, P.p)); il = 1; } else cmSwing = true; } }
        else if (cm == 1 || cm == 3) { Vector2 dv = Vector2Subtract(cmPt, P.p); float dd = Vector2Length(dv); if (dd < (cm == 3 ? .9f : .3f)) { if (cm == 3) cmPick = true; cm = 0; } else { in = Vector2Scale(dv, 1 / dd); il = 1; if (!cmFire) P.a = atan2f(in.y, in.x); } }
        if (alt && !gun && ld) cmSwing = true;
    }
    // autorun (X): keeps going until you touch a movement key, crouch, roll, get hunted, or hit a wall
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_D) || P.crouch || P.roll > 0 || P.loft >= 0) P.autorun = false;
    if (P.autorun) { for (auto& z : zombies) if (z.sees && Vector2Distance(z.p, P.p) < 25) { P.autorun = false; msg("Autorun off: you've been spotted.", STEEL); break; } }
    if (P.autorun && il == 0) { in = polar(P.autoA, 1); il = 1; }
    // Aim-relative: W walks toward the cursor, S backs off, A/D circle-strafe. Classic: WASD are the 8 compass directions.
    if (cfg.controls == 1 && il > 0 && !(P.autorun && !(IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) || IsKeyDown(KEY_D) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT)))) {
        Vector2 aimD = Vector2Subtract(ap, P.p); bool close = Vector2Length(aimD) < 1.2f; // cursor on top of you: don't spin in place
        float fwd = -in.y, str = in.x, base = close ? -PI / 2 : P.a; in = Vector2Normalize(Vector2Add(polar(base, fwd), polar(base + PI / 2, str))); }
    P.moving = il > 0; if (il > 0) P.moveA = atan2f(in.y, in.x);
    float dirK = P.blocking ? .6f : 1; if (il > 0) { float df = fabsf(angDiff(P.moveA, P.a)); P.back = P.back ? df > 1.75f : df > 2.1f; dirK *= P.back ? .7f : df > 1.1f ? .85f : 1; } // backpedal 70%, strafe 85%
    P.actCd = std::max(0.f, P.actCd - dt); P.actT -= dt; P.firedT -= dt; P.kickCd -= dt; P.kickT -= dt; P.blockFx -= dt;
    bool rmb = IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && state == ST_PLAY && !uiOpen; P.shoveCd -= dt;
    if (rmb) P.rmbT += dt; else { if (P.rmbT > 0 && P.rmbT < .2f) shove(); P.rmbT = 0; }   // tap = shove, hold = block
    P.blocking = rmb && P.rmbT > .15f && P.roll <= 0 && P.loft < 0 && P.vaultT <= 0;
    if (P.broken && P.splinted) { P.splintT += dt; if (legFactor() >= .999f) { P.broken = false; P.splinted = false; msg("Your leg's healed. Took long enough.", OKC); } }
    P.iframe -= dt; P.hurt -= dt; P.staD -= dt; P.rollCd -= dt; P.swing -= dt; P.swingCd -= dt; P.pain = std::max(0.f, P.pain - dt);
    float noiseR = 0;
    if (P.vaultT > 0) { P.vaultT -= dt; float t = 1 - std::max(0.f, P.vaultT) / .45f; P.p = Vector2Lerp(P.vaultA, P.vaultB, t); P.vel = V2(0, 0); }
    else if (P.roll > 0) { P.roll -= dt; P.p = Vector2Add(P.p, Vector2Scale(P.rv, dt)); }
    else {
        P.sprint = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && il > 0 && P.sta > 1 && !P.broken && !P.crouch;
        static float mudK = 1, mudChk = 0; if ((mudChk -= dt) <= 0) { mudChk = .25f; mudK = 1; if (rainLevel > .3f && !inCell(P.p)) for (auto& r : roads) if (r.tier == 2 && segDist(P.p, r.a, r.b) < r.w / 2 + .3f) { mudK = 1 - .25f * std::min(1.f, rainLevel); break; } }
        float sp = mudK * 3.4f * (P.sprint ? 1.85f : P.crouch ? .5f : 1) * (P.reload > 0 ? .8f : 1) * legFactor() * dirK * (P.food <= 0 || P.water <= 0 ? .8f : 1) * P.perks.speed;
        if (P.sprint) { P.sta -= 16 * P.perks.sta * dt; P.staD = .8f; }
        Vector2 tv = Vector2Scale(in, sp); P.vel = Vector2Lerp(P.vel, tv, 1 - expf(-dt * (il > 0 ? 14.f : 18.f))); // quick but not instant: no more snapping on diagonals
        Vector2 mv = Vector2Scale(P.vel, dt); P.p = Vector2Add(P.p, mv); P.walked += Vector2Length(mv);
        if (il > 0) { P.step += dt * sp; noiseR = (P.sprint ? 22.f : P.crouch ? 2.5f : 9.f) * P.perks.noise * gear().noise; stepT -= dt * sp; if (stepT <= 0) { stepT = 1.3f; sfx(SFX_STEP, P.crouch ? .2f : P.sprint ? .55f : .35f, .2f); } }
    }
    if (P.staD <= 0) P.sta = std::min(100.f, P.sta + 18 * dt * (P.food > 20 ? 1 : .4f));
    if (P.loft >= 0) { const Loft& L = lofts[P.loft]; P.k = V2(0, 0); P.p.x = Clamp(P.p.x, L.x0 + .35f, L.x1 - 1.3f); P.p.y = Clamp(P.p.y, L.z0 + .35f, L.z1 - .35f); // walk the loft; bales line the back wall
        if (P.food > 15 && P.water > 15 && P.bleeds == 0 && P.sick <= 0) P.hp = std::min(P.perks.maxhp, P.hp + .35f * dt); } else if (P.vaultT > 0) {} else { Vector2 before = P.p; P.p = Vector2Add(P.p, Vector2Scale(P.k, dt)); P.k = Vector2Scale(P.k, powf(.002f, dt)); collide(P.p, .35f);
        static float stuck = 0; if (P.autorun && il > 0) { stuck = Vector2Distance(before, P.p) < .3f * dt ? stuck + dt : 0; if (stuck > .4f) { P.autorun = false; stuck = 0; } } }
    // hiding: crouched in tall grass or a bush
    { bool inCover = false; for (auto& c : covers) if (fabsf(c.p.x - P.p.x) < 2 && fabsf(c.p.y - P.p.y) < 2 && Vector2Distance(c.p, P.p) < c.r) { inCover = true; break; }
      P.hideK = (inCover && P.crouch && !P.light) ? (il > 0 ? .35f : .12f) : 1;
      { int hb = buildingAt(P.p); bool shut = hb >= 0 && !blds[hb].doors.empty(); if (shut) for (auto& dr : blds[hb].doors) if (dr.obj >= 0 && doorObjs[dr.obj].open) shut = false; if (!inCover && P.crouch && !P.light && shut) P.hideK = .3f; }
      if (P.loft >= 0) P.hideK = .2f; }
    { static float tickT = 0; if (gExposure >= .6f) { tickT -= dt; if (tickT <= 0) { sfx(SFX_BEEP, .22f, .05f); tickT = gExposure > .85f ? .18f : .45f; } } else tickT = 0; } // warning tick at orange and red
    gExposure = 0;
    { static float pkT = 0; if ((pkT -= dt) <= 0) { pkT = .5f; recalcPerks(); } } // picks up set bonuses as you change clothes
    nearAct = findAct(); { int cb = cellOf(P.p); inBasement = cb >= 0 && blds[cb].cellKind == 2; }
    if (noiseR > 0) { static float nt = 0; nt -= dt; if (nt <= 0) { nt = .5f; noiseAt(P.p, noiseR); } }
    // survival
    float burn = (P.sprint ? 2.2f : 1) * P.perks.metab;
    P.food = std::max(0.f, P.food - .055f * burn * dt); P.water = std::max(0.f, P.water - .085f * burn * (P.sick > 0 ? 1.8f : 1) * dt);
    int bi = buildingAt(P.p); bool inside = bi >= 0; float fireNear = 0;
    for (auto& l : staticLights) if (l.kind == 2 && Vector2Distance(P.p, V2(l.p.x, l.p.z)) < 5) fireNear = 1;
    for (auto& f : fires) if (Vector2Distance(P.p, f.p) < 4.5f) fireNear = 1.2f;
    Gear gr = gear(); bool sheltered = inside || P.loft >= 0;
    if (rainLevel > .1f && !sheltered) P.wet = std::min(1.f, P.wet + rainLevel * .02f * (1 - gr.water) * dt); else P.wet = std::max(0.f, P.wet - (fireNear > 0 ? .03f : inside ? .006f : .003f) * dt);
    float target = 37.f - darkK() * 1.6f - rainLevel * 1.4f * (inside ? .15f : 1) - P.wet * 1.8f + (inside ? .6f : 0) + fireNear * 2.5f + (P.sprint ? .6f : 0) + std::min(1.f, gr.warm) * 2.2f - .9f;
    float rate = target < P.temp ? .012f * P.perks.cold : .03f; P.temp += (target - P.temp) * rate * dt * 1.5f;
    P.temp = Clamp(P.temp, 33.f, 38.5f);
    P.shiver = Clamp((35.8f - P.temp) / 1.5f, 0, 1);
    if (P.bleeds > 0) { P.hp -= .45f * P.bleeds * dt; static float bt = 0; bt -= dt; if (bt <= 0) { bt = .25f; decal(Vector2Add(P.p, V2(frand(-.2f, .2f), frand(-.2f, .2f))), frand(.06f, .14f), BLOOD_DRY); } }
    if (P.sick > 0) { P.hp -= .12f * dt; P.sick += dt; }
    if (P.food <= 0) P.hp -= .35f * dt; if (P.water <= 0) P.hp -= .55f * dt;
    if (P.temp < 35.f) P.hp -= .25f * dt;
    bool healthy = P.food > 45 && P.water > 45 && P.bleeds == 0 && P.sick <= 0 && P.temp > 35.5f;
    if (healthy) P.hp = std::min(P.perks.maxhp, P.hp + (.25f + P.perks.regen) * dt);
    if (P.hp <= 0) { die(); return; }
    // weapons
    P.cd -= dt; if (P.reload > 0) { P.reload -= dt; if (P.reload <= 0) finishReload(); }
    bool md = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && state == ST_PLAY && !uiOpen; if (cfg.controls == 2) { md = cmFire || cmSwing; if (cmSwing) P.fired = false; }
    if (md && !P.fired) { if (P.active == 2) { melee(); P.fired = true; } else if (fire()) P.fired = !curGun()->autoF; }
    if (!md) P.fired = false;
    if (P.light) { P.batt -= (P.uv ? 1.6f : .55f) * P.perks.batt * dt; if (P.batt <= 0) { P.batt = 0; P.light = false; P.uv = false; msg("Flashlight died.", UVC); } }
    // loot
    nearLoot = -1; float nd = 1.8f;
    { float ly = P.loft >= 0 ? lofts[P.loft].y : 0; for (int i = 0; i < (int)loot.size(); i++) { Loot& l = loot[i]; if (!l.settled || fabsf(l.p.y - ly) > 1.6f) continue; float d = Vector2Distance(V2(l.p.x, l.p.z), P.p); if (d < nd) { nd = d; nearLoot = i; } } }
    if (cmPick && nearLoot >= 0) pickUp();
    // town entry banner
    { int hz = hotAt(P.p, 1.f); if (hz >= 0 && !hotZones[hz].seen) { HotZone& h = hotZones[hz]; h.seen = true; showBanner("HOT ZONE", std::string(HZN[h.kind]) + ". " + HZD[h.kind], rgba(255, 90, 60)); sfx(SFX_HORN, .7f, 0);
        for (int i = 0; i < 8; i++) { Zombie* z = spawnAround(pickType(-1), 18, 34); if (z) { z->noiseT = 10; z->np = P.p; } } } }
    int t = townAt(P.p, 1.0f); if (t != lastTown) { if (t >= 0) showBanner(TOWNS[t].name, TOWNS[t].kind == TK_MILITARY ? "Restricted area. Heavy infection." : TOWNS[t].kind == TK_CITY ? "Population 42,000. Formerly." : "", BONE); lastTown = t; }
}
static void separateFromPlayer() { // last word on spacing: nothing ends a frame standing inside you (volatiles' lunges used to slip through)
    if (P.loft >= 0) return;
    for (auto& z : zombies) { if (z.dead) continue; Vector2 dv = Vector2Subtract(z.p, P.p); float pd = Vector2Length(dv), minD = z.r + .34f; if (pd >= minD) continue;
        Vector2 nrm = pd > 1e-3f ? Vector2Scale(dv, 1 / pd) : polar(P.a, 1); z.p = Vector2Add(P.p, Vector2Scale(nrm, minD)); collide(z.p, z.r);
        if (Vector2Distance(z.p, P.p) < minD * .8f) { P.p = Vector2Subtract(z.p, Vector2Scale(nrm, minD)); collide(P.p, .35f); } } }
// ---- detection and hunting tuning, per zombie type
static float detRate(int t) { switch (t) { case Z_RUNNER: return 1.0f; case Z_VOLATILE: return .9f; case Z_SOLDIER: return .8f; case Z_BRUTE: return .55f; case Z_BLOATER: return .5f; default: return .65f; } }
static float keepT(const Zombie& z) { // seconds a zombie keeps hunting after losing sight: walkers give up fast, specials stick
    float t; switch (z.type) { case Z_WALKER: t = 2.f; break; case Z_RUNNER: t = 3.f; break; case Z_BRUTE: t = 4.f; break; case Z_VOLATILE: t = 7.f; break; case Z_SOLDIER: t = 4.5f; break; default: t = 3.f; }
    if (P.hideK < 1 || !P.moving) t *= .6f; return t; } // hidden or standing still: they lose you sooner
static float doorMul(int t) { switch (t) { case Z_WALKER: return .3f; case Z_RUNNER: return .5f; case Z_BRUTE: return 2.4f; case Z_SOLDIER: return .7f; case Z_BLOATER: return .8f; default: return .6f; } } // how hard each type hits a door
static void updateZombies(float dt) {
    for (auto& c : zg) for (auto& v : c) v.clear();
    for (int i = 0; i < (int)zombies.size(); i++) zg[zcl(zombies[i].p.x)][zcl(zombies[i].p.y)].push_back(i);
    bool night = isNight(); int pb = buildingAt(P.p);
    float sight = (night ? 14.f : 30.f) * (P.crouch ? .55f : 1) * (rainLevel > .5f ? .8f : 1) + (P.light ? 18.f : 0); sight *= (1 - P.perks.stealth) * P.hideK * gear().vis;
    for (int zi = 0; zi < (int)zombies.size(); zi++) {
        Zombie& z = zombies[zi]; if (z.dead) continue;
        z.flash -= dt; z.stun -= dt; z.slow -= dt; z.aggro -= dt; z.lureT -= dt; z.losT -= dt; z.ignoreT -= dt;
        if (z.burn > 0) { z.burn -= dt; hurtZ(z, 12 * dt, false, -999, true); if (chance(dt * 18)) parts.push_back({xz3(Vector2Add(z.p, V2(frand(-.3f, .3f), frand(-.3f, .3f))), frand(.5f, 1.6f)), V3(0, frand(1, 2.5f), 0), .5f, .5f, frand(.1f, .2f), rgba(255, 150, 50), P_FIRE}); if (z.dead) continue; }
        if (z.bleed > 0) { z.bleed -= dt; if (chance(dt * 6)) decal(Vector2Add(z.p, V2(frand(-.2f, .2f), frand(-.2f, .2f))), frand(.06f, .15f), BLOOD_DRY); }
        float dx = P.p.x - z.p.x, dz = P.p.y - z.p.y, d = sqrtf(dx * dx + dz * dz);
        if (d > 180) { z.gone = true; continue; }
        bool lit = P.uv && P.light && d < 14 && fabsf(angDiff(atan2f(-dz, -dx), P.a)) < .35f;
        if (lit) { if (z.type == Z_VOLATILE) { z.slow = .25f; hurtZ(z, 30 * dt, false, -999, true); if (chance(dt * 20)) parts.push_back({xz3(z.p, 1.5f), V3(frand(-.5f, .5f), 1.5f, frand(-.5f, .5f)), .6f, .6f, .3f, rgba(200, 170, 255, 160), P_SMOKE}); if (z.dead) continue; } else if (z.type == Z_RUNNER) z.slow = .2f; }
        if (z.losT <= 0) { z.losT = frand(.2f, .35f);
            z.inSight = P.loft >= 0 ? (P.firedT > 0 && d < 9 && losClear(z.p, P.p)) : (d < sight && losClear(z.p, P.p));
            if (z.inSight && z.hordeM) { horde.lastSeen = P.p; for (auto& o : zombies) if (o.hordeM && Vector2Distance(o.p, z.p) < 30) o.aggro = std::max(o.aggro, keepT(o)); } }
        { float rate = detRate(z.type) * (1 + 1.5f * Clamp(1 - d / std::max(sight, 1.f), 0, 1)) * (P.sprint ? 1.4f : 1) * (P.crouch ? .8f : 1);
            if (!P.crouch) rate = 99.f; // standing: spotted at once, like before. The meter is for crouching only
            if (z.inSight) z.notice = std::min(1.2f, z.notice + rate * dt); else z.notice = std::max(0.f, z.notice - .8f * dt);
            bool close = d < 2.2f && P.hideK < 1; if (close) z.notice = 1; z.sees = z.notice >= 1;
            if (P.crouch && d < 40 && z.notice > gExposure) gExposure = std::min(1.f, z.notice); }
        Vector2 tgt = P.p; float mult = 1; bool chase = false;
        if (z.lureT > 0) tgt = z.lp;
        else if (z.sees || (z.aggro > 0 && d < 60)) { chase = true; if (z.sees) z.aggro = std::max(z.aggro, keepT(z)); }
        else if (z.noiseT > 0) { tgt = z.np; z.noiseT -= dt; mult = z.hordeM ? .75f : .8f; if (Vector2Distance(tgt, z.p) < (z.hordeM ? 4.f : 1.5f)) z.noiseT = 0;
            if (z.noiseT <= 0 && z.hordeM) { // reached a waypoint: searching done -> resume the march; march done -> drift off
                if (z.hstate == 1 || Vector2Distance(z.p, horde.end) > 10) { z.hstate = 0; z.np = Vector2Add(horde.end, V2(frand(-10, 10), frand(-10, 10))); z.noiseT = 240; } else z.hordeM = false; } }
        else { z.wt -= dt; if (z.wt <= 0) { z.wa = frand(0, 2 * PI); z.wt = frand(3, 7); } tgt = Vector2Add(z.p, polar(z.wa, 3)); mult = .3f; }
        if (chase && z.type == Z_VOLATILE && z.lureT <= 0 && d > 6) { float dd = std::max(d, .01f); Vector2 perp = V2(-dz / dd, dx / dd); tgt = Vector2Add(P.p, Vector2Scale(perp, sinf(gTime * .9f + z.seed * 6.28f) * 2.2f)); } // weave in, don't charge
        if (z.chasing && !chase && z.hordeM) { z.hstate = 1; z.np = Vector2Add(horde.lastSeen, V2(frand(-9, 9), frand(-9, 9))); z.noiseT = 14; } // lost you: search your last spot
        z.chasing = chase;
        // doors
        bool banging = false;
        if (chase || z.noiseT > 0) {
            int zb = buildingAt(z.p), tb = buildingAt(tgt);
            if (zb != tb) {
                int b = zb >= 0 ? zb : tb; const Building& B = blds[b]; bool exiting = zb >= 0;
                float bd = 1e9f; const Door* best = nullptr; for (auto& dr : B.doors) { float dd = Vector2Distance(z.p, exiting ? dr.in : dr.out); if (dd < bd) { bd = dd; best = &dr; } }
                if (best) { Vector2 first = exiting ? best->in : best->out, second = exiting ? best->out : best->in;
                    DoorObj* dobj = best->obj >= 0 ? &doorObjs[best->obj] : nullptr; bool shut = dobj && !dobj->open && !dobj->broken;
                    if (shut && z.via != 2) { // closed door between us: walk up to it, then pound until it gives or they lose interest
                        Vector2 tan = V2(-dobj->outN.y, dobj->outN.x); Vector2 spot = Vector2Add(Vector2Add(dobj->c, Vector2Scale(dobj->outN, exiting ? -.85f : .85f)), Vector2Scale(tan, z.side * .45f * (z.id % 3)));
                        if (Vector2Distance(z.p, spot) < .7f) { if (z.pound != best->obj) { z.pound = best->obj; z.poundT = (chase || z.aggro > 0) ? frand(22, 40) : frand(10, 16); } }
                        else { tgt = spot; z.via = 1; } }
                    else if (z.via == 2 || Vector2Distance(z.p, first) < 1.0f) { tgt = second; z.via = 2; } else { tgt = first; z.via = 1; } }
            } else z.via = 0;
        }
        if (z.pound >= 0) { // pounding state
            DoorObj& dd = doorObjs[z.pound]; int db = dd.bld;
            if (dd.open || dd.broken) { z.pound = -1; z.via = 1; }
            else {
                z.poundT -= dt; if (z.sees) z.poundT = std::max(z.poundT, 20.f);
                if (buildingAt(P.p) == db && Vector2Distance(P.p, dd.c) < 22 && (P.firedT > 0 || (P.moving && !P.crouch))) z.poundT = std::max(z.poundT, 12.f); // they hear you in there
                if (z.poundT <= 0) { z.pound = -1; z.aggro = 0; z.noiseT = 0; z.ignoreT = 10; z.wa = atan2f(z.p.y - dd.c.y, z.p.x - dd.c.x) + frand(-.8f, .8f); z.wt = 6; }
                else { banging = true; tgt = dd.c; z.atk -= dt;
                    if (z.atk <= 0) { z.atk = frand(.85f, 1.3f); dd.hp -= z.dmg * doorMul(z.type) * frand(.7f, 1.3f); dd.bang = .25f;
                        sfxAt(dd.metal ? SFX_DOOR_METAL : SFX_DOOR_BANG, dd.c, dd.metal ? .8f : .95f, .06f); if (dd.hp < dd.hpMax * .35f && chance(.4f)) sfxAt(SFX_CREAK, dd.c, .6f, .2f);
                        if (!dd.metal) for (int k = 0; k < 3; k++) parts.push_back({xz3(dd.c, frand(.6f, 2.f)), V3(frand(-1.5f, 1.5f), frand(.5f, 2), frand(-1.5f, 1.5f)), .5f, .5f, .04f, rgba(140, 100, 60), P_DUST});
                        if (dd.hp <= 0) { dd.broken = true; sfxAt(SFX_BOOM, dd.c, .25f); sfxAt(dd.metal ? SFX_DOOR_METAL : SFX_DOOR_BANG, dd.c, 1.f, .1f); for (int k = 0; k < 12; k++) parts.push_back({xz3(dd.c, 1.2f), V3(frand(-3, 3), frand(1, 3), frand(-3, 3)), .6f, .6f, .06f, dd.metal ? rgba(150, 150, 150) : rgba(140, 100, 60), P_DUST});
                            if (Vector2Distance(dd.c, P.p) < 30) msg(dd.metal ? "They tore the metal door off its hinges." : "They broke the door down.", BLOODC); } } } } }
        float ang = atan2f(tgt.y - z.p.y, tgt.x - z.p.x);
        if (z.type == Z_VOLATILE && lit) ang += PI;
        if (z.type == Z_WALKER || z.type == Z_SOLDIER) ang += sinf(gTime * 1.3f + z.seed) * .3f;
        float look = z.r + .5f;
        if (pointSolid(z.p.x + cosf(ang) * look, z.p.y + sinf(ang) * look, .4f)) { bool ok = false;
            for (float da : {.6f, 1.1f, 1.6f, 2.2f}) { float a2 = ang + da * z.side; if (!pointSolid(z.p.x + cosf(a2) * look, z.p.y + sinf(a2) * look, .4f)) { ang = a2; ok = true; break; } }
            if (!ok) z.side = -z.side; }
        float sp = z.spd * mult * (night && (z.type == Z_WALKER || z.type == Z_RUNNER || z.type == Z_SOLDIER) ? 1.35f : 1) * (z.slow > 0 ? .4f : 1) * (z.stun > 0 ? .3f : 1);
        if (z.type == Z_BRUTE) { z.chg -= dt;
            if (z.chgT > 0) { z.chgT -= dt; ang = z.ca; sp = z.spd * 4.5f; if (chance(dt * 30)) parts.push_back({xz3(z.p, .1f), V3(frand(-1, 1), .5f, frand(-1, 1)), .6f, .6f, .4f, rgba(90, 80, 70, 150), P_DUST}); if (d < z.r + .5f) { hitPlayer(z.dmg * 1.3f, z.p, Z_BRUTE); z.chgT = 0; } }
            else if (chase && d < 12 && d > 3 && z.chg <= 0) { z.chgT = .8f; z.chg = 5; z.ca = atan2f(dz, dx); sfxAt(SFX_ROAR, z.p); shake = std::min(.5f, shake + .15f); } }
        z.a = ang; Vector2 mv = polar(ang, sp * dt); z.p = Vector2Add(z.p, mv); z.walk += Vector2Length(mv);
        z.p = Vector2Add(z.p, Vector2Scale(z.k, dt)); z.k = Vector2Scale(z.k, powf(.002f, dt));
        for (int o : zNear(z.p)) { if (o == zi) continue; Zombie& q = zombies[o]; float ex = z.p.x - q.p.x, ez = z.p.y - q.p.y, md = z.r + q.r, e2 = ex * ex + ez * ez;
            if (e2 < md * md && e2 > 1e-4f) { float e = sqrtf(e2), pp = (md - e) * .5f; z.p.x += ex / e * pp; z.p.y += ez / e * pp; } }
        collide(z.p, z.r);
        if (P.loft < 0 && P.roll <= 0 && P.vaultT <= 0) { // they get in your face, but they can't stand inside you
            Vector2 dv = Vector2Subtract(z.p, P.p); float pd = Vector2Length(dv), minD = z.r + .36f;
            if (pd < minD) { Vector2 nrm = pd > 1e-3f ? Vector2Scale(dv, 1 / pd) : polar(frand(0, 6), 1); float ov = minD - pd; float zk = z.type == Z_BRUTE ? .25f : .7f;
                z.p = Vector2Add(z.p, Vector2Scale(nrm, ov * zk)); P.p = Vector2Subtract(P.p, Vector2Scale(nrm, ov * (1 - zk))); collide(z.p, z.r); collide(P.p, .35f); } }
        if (z.type == Z_BLOATER && d < 1.4f) { z.hp = 0; killZ(z, atan2f(dz, dx), false, 0, false); continue; }
        if (banging) { z.a = atan2f(tgt.y - z.p.y, tgt.x - z.p.x); for (int o : zNear(z.p)) { if (o == zi) continue; Zombie& q = zombies[o]; float ex = z.p.x - q.p.x, ez = z.p.y - q.p.y, md = z.r + q.r, e2 = ex * ex + ez * ez;
                if (e2 < md * md && e2 > 1e-4f) { float e = sqrtf(e2), pp = (md - e) * .5f; z.p.x += ex / e * pp; z.p.y += ez / e * pp; } } collide(z.p, z.r); continue; }
        if (d < z.r + .35f + .35f && P.roll <= 0 && P.loft < 0) { z.atk -= dt; if (z.atk <= 0) { z.atk = z.type == Z_VOLATILE ? .6f : .95f; if (chance(.45f)) sfxAt(z.type == Z_BRUTE ? SFX_ROAR : z.type == Z_VOLATILE ? SFX_SCREAM : SFX_ZATTACK, z.p, .8f, .15f); hitPlayer(z.dmg, z.p, z.type); } } else z.atk = std::max(z.atk, .35f);
        if (groanCd <= 0 && d < 18 && chance(dt * .5f)) { groanCd = frand(1.5f, 4); sfxAt(SFX_GROAN, z.p, .8f, .2f); }
    }
    (void)pb;
}
static void updateBullets(float dt) {
    for (int i = (int)bullets.size() - 1; i >= 0; i--) {
        Bullet& b = bullets[i]; b.life -= dt; bool dead = b.life <= 0;
        int steps = std::max(1, (int)ceilf(Vector2Length(b.v) * dt / .3f)); Vector2 st = Vector2Scale(b.v, dt / steps);
        for (int s = 0; s < steps && !dead; s++) {
            b.p = Vector2Add(b.p, st);
            if (pointSolid(b.p.x, b.p.y, 1.1f)) { dead = true; Vector2 bp = Vector2Subtract(b.p, st);
                for (int k = 0; k < 6; k++) parts.push_back({xz3(bp, 1.3f), V3(frand(-4, 4), frand(0, 4), frand(-4, 4)), .15f, .15f, .03f, rgba(255, 220, 150), P_SPARK});
                parts.push_back({xz3(bp, 1.3f), V3(0, .3f, 0), .6f, .6f, .2f, rgba(150, 140, 130, 120), P_DUST}); lights.push_back({xz3(bp, 1.3f), 3, .05f, .05f, rgba(255, 200, 140)}); break; }
            for (int zi : zNear(b.p)) {
                Zombie& z = zombies[zi]; if (z.dead) continue; bool hb = false; for (int h = 0; h < b.nh; h++) if (b.hits[h] == z.id) hb = true; if (hb) continue;
                if (Vector2DistanceSqr(z.p, b.p) < (z.r + .08f) * (z.r + .08f)) {
                    if (b.nh < 8) b.hits[b.nh++] = z.id; bool hd = chance(P.perks.crit + b.crit) || (P.actT > 0 && P.cls == 3);
                    if (b.burn) z.burn = 3; if (b.leech > 0) P.hp = std::min(P.perks.maxhp, P.hp + b.leech);
                    hurtZ(z, b.dmg, hd, atan2f(b.v.y, b.v.x), false, b.dism, b.shotgun && Vector2Distance(z.p, P.p) < 3.5f);
                    if (b.pierce > 0) { b.pierce--; b.dmg *= .8f; } else { dead = true; break; }
                }
            }
        }
        if (dead) { bullets[i] = bullets.back(); bullets.pop_back(); }
    }
}
static void updateWorld(float dt) {
    const float G = 18;
    for (int i = (int)bombs.size() - 1; i >= 0; i--) {
        Bomb& b = bombs[i]; b.v.y -= G * dt; Vector3 n = Vector3Add(b.p, Vector3Scale(b.v, dt));
        if (pointSolid(n.x, b.p.z, n.y)) b.v.x *= -.4f; else b.p.x = n.x; if (pointSolid(b.p.x, n.z, n.y)) b.v.z *= -.4f; else b.p.z = n.z;
        b.p.y = n.y; if (b.p.y < .08f) { b.p.y = .08f; b.v.y *= -.3f; b.v.x *= .6f; b.v.z *= .6f; }
        b.t -= dt;
        if (b.flare) { if (chance(dt * 30)) parts.push_back({b.p, V3(frand(-.3f, .3f), frand(1, 2), frand(-.3f, .3f)), .5f, .5f, .2f, rgba(255, 60, 40, 140), P_SMOKE});
            if (chance(dt * 40)) parts.push_back({b.p, V3(frand(-2, 2), frand(1, 3), frand(-2, 2)), .3f, .3f, .05f, rgba(255, 120, 90), P_SPARK});
            for (auto& z : zombies) if (Vector2Distance(z.p, V2(b.p.x, b.p.z)) < 30) { z.lureT = .3f; z.lp = V2(b.p.x, b.p.z); }
            if (b.t <= 0) bombs.erase(bombs.begin() + i); continue; }
        b.beep -= dt; b.blink -= dt;
        if (b.beep <= 0) { b.beep = std::max(.08f, b.t * .17f); b.blink = .07f; sfxAt(SFX_BEEP, V2(b.p.x, b.p.z), .6f, 0); lights.push_back({b.p, 5, .08f, .08f, rgba(255, 60, 40)}); }
        for (auto& z : zombies) if (Vector2Distance(z.p, V2(b.p.x, b.p.z)) < 35) { z.lureT = .2f; z.lp = V2(b.p.x, b.p.z); }
        if (b.t <= 0) { Vector2 bp = V2(b.p.x, b.p.z); bombs.erase(bombs.begin() + i); explode(bp, 5.5f * P.perks.blast, 180 * (1 + .1f * (dayN() - 1)), 50); }
    }
    for (int i = (int)acid.size() - 1; i >= 0; i--) { AcidPool& p = acid[i]; p.t -= dt; if (p.t <= 0) { acid.erase(acid.begin() + i); continue; } float f = std::min(1.f, p.t / 1.5f);
        if (Vector2Distance(P.p, p.p) < p.r * f && state == ST_PLAY) { P.hp -= 9 * dt; P.hurt = std::max(P.hurt, .1f); if (P.hp <= 0) die(); }
        for (auto& z : zombies) if (z.type != Z_BLOATER && Vector2Distance(z.p, p.p) < p.r * f) hurtZ(z, 8 * dt, false, -999, true);
        if (chance(dt * 8)) parts.push_back({xz3(Vector2Add(p.p, polar(frand(0, 6), frand(0, p.r * .7f))), .1f), V3(0, .6f, 0), .8f, .8f, .12f, rgba(170, 220, 80, 160), P_MIST}); }
    for (int i = (int)parts.size() - 1; i >= 0; i--) {
        Particle& p = parts[i]; p.t -= dt;
        bool blood = p.kind == P_BLOOD || p.kind == P_ACID;
        if (blood || p.kind == P_SPARK || p.kind == P_CASING) p.v.y -= G * dt;
        if (p.kind == P_SMOKE || p.kind == P_MIST || p.kind == P_DUST) p.v = Vector3Scale(p.v, powf(.3f, dt));
        if (p.kind == P_FIRE) { p.v = Vector3Scale(p.v, powf(.15f, dt)); p.v.y += 3 * dt; }
        Vector3 np = Vector3Add(p.p, Vector3Scale(p.v, dt));
        if (blood && pointSolid(np.x, np.z, np.y)) { p.t = 0; p.v = V3(0, 0, 0); }
        else p.p = np;
        float floor = buildingAt(V2(p.p.x, p.p.z)) >= 0 ? .07f : .02f;
        if (p.p.y < floor) { p.p.y = floor; if (blood) { decal(V2(p.p.x, p.p.z), p.s * frand(2, 4.5f), p.kind == P_ACID ? ACID_C : BLOOD_DRY); p.t = 0; } else if (p.kind == P_CASING) { p.v = Vector3Scale(p.v, .3f); p.v.y = fabsf(p.v.y); } else p.v.y = 0; }
        if (p.t <= 0) { parts[i] = parts.back(); parts.pop_back(); }
    }
    if (parts.size() > 3500) parts.erase(parts.begin(), parts.begin() + (parts.size() - 3500));
    for (auto& g : gibs) {
        if (g.rest) continue; g.t += dt; g.v.y -= G * dt;
        Vector3 n = Vector3Add(g.p, Vector3Scale(g.v, dt));
        if (pointSolid(n.x, g.p.z, n.y)) { g.v.x *= -.35f; g.rotv *= .6f; decal(V2(g.p.x, g.p.z), .12f, BLOOD_DRY); } else g.p.x = n.x;
        if (pointSolid(g.p.x, n.z, n.y)) { g.v.z *= -.35f; g.rotv *= .6f; } else g.p.z = n.z;
        g.p.y = n.y; float fl = (g.kind == G_RIB ? .03f : g.s.y * .5f) + (buildingAt(V2(g.p.x, g.p.z)) >= 0 ? .06f : 0);
        if (g.p.y < fl) { g.p.y = fl; if (fabsf(g.v.y) > 1.5f) decal(V2(g.p.x, g.p.z), frand(.12f, .3f), g.acid ? ACID_C : BLOOD_DRY); g.v.y *= -.3f; g.v.x *= .55f; g.v.z *= .55f; g.rotv *= .5f; }
        g.rot += g.rotv * dt; g.trail += Vector3Length(g.v) * dt;
        if (g.trail > .35f && g.p.y < .4f) { g.trail = 0; decal(V2(g.p.x, g.p.z), frand(.05f, .12f), g.acid ? ACID_C : BLOOD_DRY); }
        if (g.t > 1 && Vector3Length(g.v) < .3f) g.rest = true;
    }
    for (auto& d : decals) if (d.grow > 0) { float add = std::min(d.grow, dt * .35f); d.r += add; d.grow -= add; }
    for (int i = (int)floaters.size() - 1; i >= 0; i--) { floaters[i].t -= dt; floaters[i].p.y += dt * .8f; if (floaters[i].t <= 0) floaters.erase(floaters.begin() + i); }
    for (int i = (int)lights.size() - 1; i >= 0; i--) { lights[i].t -= dt; if (lights[i].t <= 0) { lights[i] = lights.back(); lights.pop_back(); } }
    for (int i = (int)sdrops.size() - 1; i >= 0; i--) { sdrops[i].t -= dt; sdrops[i].p.y += dt * 8; if (sdrops[i].t <= 0) { sdrops[i] = sdrops.back(); sdrops.pop_back(); } }
    for (int i = (int)msgs.size() - 1; i >= 0; i--) { msgs[i].life -= dt; if (msgs[i].life <= 0) msgs.erase(msgs.begin() + i); }
    for (auto& l : loot) if (!l.settled) { l.v.y -= G * dt; l.p = Vector3Add(l.p, Vector3Scale(l.v, dt)); if (l.p.y < .05f) { l.p.y = .05f; l.settled = true; } }
    // campfires: burn fuel, warm you, draw the dead at night, die in the rain unless a tree's over them
    for (int i = (int)fires.size() - 1; i >= 0; i--) { Fire& f = fires[i]; f.fuel -= dt;
        bool sheltered = false; for (int cx = gcl(f.p.x - 6); cx <= gcl(f.p.x + 6); cx++) for (int cz = gcl(f.p.y - 6); cz <= gcl(f.p.y + 6); cz++) for (int ti : tgrid[cx][cz]) if (Vector2Distance(trees[ti].p, f.p) < trees[ti].r * .95f + .3f) sheltered = true;
        if (rainLevel > .35f && !sheltered) { f.fuel -= dt * 18; if (!f.warned && Vector2Distance(f.p, P.p) < 40) { f.warned = true; msg("The rain's drowning your fire. Should've built it under a tree.", COLDC); } }
        f.smokeT -= dt; if (f.smokeT <= 0 && Vector2Distance(f.p, P.p) < 70) { f.smokeT = .07f; parts.push_back({xz3(Vector2Add(f.p, V2(frand(-.25f, .25f), frand(-.25f, .25f))), .25f), V3(frand(-.2f, .2f), frand(1.4f, 2.6f), frand(-.2f, .2f)), .6f, .6f, frand(.12f, .24f), rgba(255, 140, 50), P_FIRE});
            if (chance(.3f)) parts.push_back({xz3(f.p, 1.2f), V3(frand(-.3f, .3f), frand(.8f, 1.6f), frand(-.3f, .3f)), 3, 3, frand(.6f, 1.2f), rgba(50, 50, 54, 140), P_SMOKE}); }
        f.lureT -= dt; if (f.lureT <= 0) { f.lureT = 4; noiseAt(f.p, isNight() ? 38.f : 16.f); }
        if (f.fuel <= 0) { decal(f.p, .9f, rgba(18, 16, 14, 230)); if (Vector2Distance(f.p, P.p) < 30) msg("The fire's out.", STEEL); fires.erase(fires.begin() + i); } }
    // ambient fire smoke
    if (chance(dt * 10)) for (auto& l : staticLights) { if (Vector2Distance(V2(l.p.x, l.p.z), P.p) > 60) continue;
        parts.push_back({Vector3Add(l.p, V3(frand(-.2f, .2f), 0, frand(-.2f, .2f))), V3(frand(-.2f, .2f), frand(1.5f, 3), frand(-.2f, .2f)), .6f, .6f, frand(.12f, .25f), rgba(255, 140, 50), P_FIRE});
        if (chance(.4f)) parts.push_back({Vector3Add(l.p, V3(0, 1, 0)), V3(frand(-.3f, .3f), frand(.8f, 1.6f), frand(-.3f, .3f)), 3, 3, frand(.6f, 1.2f), rgba(40, 40, 44, 150), P_SMOKE}); }
}
static void update(float dt) {
    gTime += dt; P.alive += 0; shake = std::max(0.f, shake - dt * 2); groanCd -= dt; if (banner.t > 0) banner.t -= dt;
    char ph = isNight() ? 'n' : 'd';
    if (ph != phase) { phase = ph; if (ph == 'n') { showBanner("NIGHTFALL", "They see your flashlight. Volatiles are hunting.", rgba(255, 90, 60)); sfx(SFX_HORN, 1, 0); }
        else { nights++; int bonus = (int)roundf(100.f * nights); gainXP((float)bonus); showBanner("DAWN", "Survived night " + std::to_string(nights) + ".  +" + std::to_string(bonus) + " XP"); sfx(SFX_LEVEL); } }
    // weather
    rainT -= dt; if (rainT <= 0) { rainT = frand(90, 240); rainTarget = chance(.35f) ? frand(.6f, 1.f) : 0; }
    rainLevel += (rainTarget - rainLevel) * std::min(1.f, dt * .05f);
    // horde
    if (!horde.on && hordeDay < dayN() && hourOf() > 13 && hourOf() < 18 && dayN() >= 2) { hordeDay = dayN(); horde = {true, frand(0, 2 * PI), 0, 14 + dayN() * 8};
        horde.end = Vector2Add(Vector2Add(P.p, polar(horde.a + PI, 150)), polar(horde.a + PI / 2, frand(-40, 40))); horde.lastSeen = P.p;
        static const char* dirs[8] = {"EAST", "SOUTHEAST", "SOUTH", "SOUTHWEST", "WEST", "NORTHWEST", "NORTH", "NORTHEAST"}; int di = ((int)roundf(fmodf(horde.a + 2 * PI, 2 * PI) / (PI / 4))) % 8;
        showBanner("HORDE INBOUND", std::string("Moving in from the ") + dirs[di] + ". Get inside or get moving.", rgba(255, 90, 60)); sfx(SFX_HORN, .6f, 0); sfx(SFX_STING, 1, 0); }
    for (int i = (int)followers.size() - 1; i >= 0; i--) { Follower& f = followers[i]; f.t -= dt; if (f.t > 0) continue; // arrives at the top/bottom of the stairs, only if you are still on that floor
        if (buildingAt(P.p) == f.cell || Vector2Distance(P.p, f.to) < 30) { Zombie& z = spawnZ(f.type, Vector2Add(f.to, V2(frand(-.4f, .4f), frand(-.4f, .4f)))); z.aggro = 20; z.losT = 0; } followers.erase(followers.begin() + i); }
    if (horde.on && !inCell(P.p)) { horde.t -= dt; if (horde.t <= 0) { horde.t = .25f; Zombie* z = spawnAround(chance(.2f) ? Z_RUNNER : Z_WALKER, 70, 85, horde.a, .3f); if (z) { z->hordeM = true; z->noiseT = 240; z->np = Vector2Add(horde.end, V2(frand(-12, 12), frand(-12, 12))); } if (--horde.left <= 0) horde.on = false; } }
    // director
    spawnT -= dt;
    if (inCell(P.p)) spawnT = 1; // nothing wanders into a basement on its own
    if (spawnT <= 0) { spawnT = .6f; int t = zoneTown(); int base = t < 0 ? 7 : TOWNS[t].kind == TK_CITY ? 46 : TOWNS[t].kind == TK_MILITARY ? 38 : TOWNS[t].kind == TK_TOWN ? 28 : 16;
        if (hotAt(P.p, 1.6f) >= 0) base = (int)(std::max(base, 18) * 2.5f);
        int target = (int)(base * (isNight() ? 1.4f : 1) * (1 + .1f * (dayN() - 1)) * (.55f + .45f * scaleK()));
        int nearN = 0; for (auto& z : zombies) if (Vector2Distance(z.p, P.p) < 110) nearN++;
        if (nearN < target) spawnAround(pickType(t), 45, 95); }
    updatePlayer(dt); if (state != ST_PLAY) return; updateZombies(dt); separateFromPlayer(); updateBullets(dt); updateWorld(dt);
    zombies.erase(std::remove_if(zombies.begin(), zombies.end(), [](const Zombie& z) { return z.dead || z.gone; }), zombies.end());
    Vector2 ap = aimPoint(); Vector2 lead = Vector2Scale(V2(Clamp(ap.x - P.p.x, -12, 12), Clamp(ap.y - P.p.y, -9, 9)), .28f);
    camPos = Vector2Lerp(camPos, Vector2Add(P.p, lead), 1 - powf(.002f, dt));
    if (pendingLv > 0 && state == ST_PLAY) grantLevelPoints();
}

// ============================================================ rendering
static Vector3 camRight{1, 0, 0}, camUp{0, 1, 0}, camFwd{0, 0, 1};
static void quad3(Vector3 c, Vector3 ax, Vector3 ay, Vector3 n, Color col, Rectangle uv) {
    rlColor4ub(col.r, col.g, col.b, col.a); rlNormal3f(n.x, n.y, n.z);
    rlTexCoord2f(uv.x, uv.y); rlVertex3f(c.x - ax.x - ay.x, c.y - ax.y - ay.y, c.z - ax.z - ay.z);
    rlTexCoord2f(uv.x, uv.y + uv.height); rlVertex3f(c.x - ax.x + ay.x, c.y - ax.y + ay.y, c.z - ax.z + ay.z);
    rlTexCoord2f(uv.x + uv.width, uv.y + uv.height); rlVertex3f(c.x + ax.x + ay.x, c.y + ax.y + ay.y, c.z + ax.z + ay.z);
    rlTexCoord2f(uv.x + uv.width, uv.y); rlVertex3f(c.x + ax.x - ay.x, c.y + ax.y - ay.y, c.z + ax.z - ay.z);
}
static void groundQuad(Vector2 p, float y, float r, float rot, Color c, Rectangle uv) { Vector3 ax = V3(cosf(rot) * r, 0, sinf(rot) * r), ay = V3(-sinf(rot) * r, 0, cosf(rot) * r); quad3(V3(p.x, y, p.y), ax, ay, V3(0, 1, 0), c, uv); }
static void billboard(Vector3 p, float s, Color c) { quad3(p, Vector3Scale(camRight, s), Vector3Scale(camUp, s), Vector3Negate(camFwd), c, {0, 0, 1, 1}); }

// humanoid built from boxes. local forward = +x
static void drawHuman(Vector2 pos, float yaw, float walk, Color skin, Color shirt, Color pants, float h, float width, bool armL, bool armR, bool head, int pose, float lieT, Color tint, int extra, float swingT) {
    // pose: 0 zombie reach, 1 player w/ gun, 2 player melee, 3 corpse
    Matrix base = MatrixMultiply(MatrixRotateY(-yaw), MatrixTranslate(pos.x, 0, pos.y));
    if (pose == 3) base = MatrixMultiply(MatrixMultiply(MatrixRotateZ(-PI / 2), MatrixTranslate(-h * .05f, .14f, 0)), base);
    float k = h / 1.8f, sw = sinf(walk * 3.2f) * .7f;
    auto T = [&](Color c) { if (pose == 3) return lerpC(c, rgba(60, 20, 22), .35f); return rgba(c.r * tint.r / 255, c.g * tint.g / 255, c.b * tint.b / 255, 255); };
    auto part = [&](Vector3 size, Vector3 pivot, Vector3 off, float rz, float rx, Color c) {
        Matrix m = MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixScale(size.x, size.y, size.z), MatrixTranslate(off.x, off.y, off.z)), MatrixMultiply(MatrixRotateX(rx), MatrixRotateZ(rz))), MatrixTranslate(pivot.x, pivot.y, pivot.z));
        drawMeshM(M_CUBE, MatrixMultiply(m, base), T(c)); };
    float legL = .85f * k, torso = .62f * k, hipY = legL;
    float lsw = pose == 3 ? .15f : sw, rsw = pose == 3 ? -.1f : -sw;
    part(V3(.2f * width, legL, .2f * width), V3(0, hipY, -.12f * width), V3(0, -legL / 2, 0), lsw * .6f, 0, pants);
    part(V3(.2f * width, legL, .2f * width), V3(0, hipY, .12f * width), V3(0, -legL / 2, 0), rsw * .6f, 0, pants);
    part(V3(.28f * width, torso, .46f * width), V3(0, hipY, 0), V3(0, torso / 2, 0), pose == 0 ? -.18f : 0, 0, shirt);
    if (extra == 1) part(V3(.22f, .42f * k, .36f), V3(-.2f * width, hipY + torso * .55f, 0), V3(0, 0, 0), 0, 0, rgba(80, 70, 52)); // backpack
    float shY = hipY + torso - .06f * k, armLen = .62f * k;
    auto arm = [&](bool has, float side, float rz, float rx) {
        if (has) part(V3(.13f * width, armLen, .13f * width), V3(0, shY, side * .3f * width), V3(0, -armLen / 2, 0), rz, rx, pose == 0 ? skin : shirt);
        else part(V3(.15f, .12f, .15f), V3(0, shY, side * .3f * width), V3(0, -.04f, 0), 0, 0, rgba(140, 16, 22)); };
    if (pose == 0) { float r = sinf(walk * 2.1f) * .12f; arm(armL, -1, 1.35f + r, .05f); arm(armR, 1, 1.4f - r, -.05f); }
    else if (pose == 1) { arm(true, -1, 1.45f, .45f); arm(true, 1, 1.45f, -.35f); }
    else if (pose == 2) { float s = swingT >= 0 ? (-1.2f + 2.4f * swingT) : 0; arm(true, -1, .2f + sw * .3f, 0); arm(true, 1, 1.5f, s); }
    else { arm(armL, -1, .3f, .2f); arm(armR, 1, -.2f, -.2f); }
    if (head) {
        float hs = .26f * k; part(V3(hs, hs * 1.05f, hs), V3(0, shY + .04f * k, 0), V3(.02f, hs * .55f, 0), pose == 0 ? -.25f : 0, 0, skin);
        if (pose == 0 || pose == 3) part(V3(.04f, .05f, .1f), V3(0, shY + .04f * k, 0), V3(hs * .52f, hs * .55f, 0), pose == 0 ? -.25f : 0, 0, rgba(90, 10, 14));
        if (extra == 1) part(V3(hs * 1.05f, hs * .35f, hs * 1.08f), V3(0, shY + .04f * k, 0), V3(0, hs * 1.0f, 0), 0, 0, rgba(60, 64, 50));
        if (extra == 2) part(V3(hs * 1.15f, hs * .45f, hs * 1.15f), V3(0, shY + .04f * k, 0), V3(0, hs * .95f, 0), 0, 0, rgba(70, 80, 60)); // helmet
    } else part(V3(.16f, .08f, .16f), V3(0, shY, 0), V3(0, .04f, 0), 0, 0, rgba(130, 14, 20));
    (void)lieT;
}
static void drawGibM(const Gib& g) {
    Matrix m = MatrixMultiply(MatrixMultiply(MatrixScale(g.s.x, g.s.y, g.s.z), MatrixRotate(g.axis, g.rot)), MatrixTranslate(g.p.x, g.p.y, g.p.z));
    if (g.kind == G_GUTS) { for (int i = 0; i < 4; i++) drawMeshM(M_SPHERE, MatrixMultiply(MatrixScale(g.s.x * .6f, g.s.y, g.s.z * .6f), MatrixTranslate(g.p.x + cosf(i * 1.6f) * .12f, g.p.y, g.p.z + sinf(i * 1.6f) * .12f)), i % 2 ? g.c : rgba(120, 30, 40)); return; }
    drawMeshM(g.kind == G_HEAD ? M_CUBE : M_CUBE, m, g.c);
    if (g.kind == G_LIMB || g.kind == G_HEAD) { Vector3 e = Vector3Transform(V3(0, -.5f, 0), MatrixMultiply(MatrixRotate(g.axis, g.rot), MatrixScale(1, 1, 1))); Vector3 ep = Vector3Add(g.p, V3(e.x * g.s.y, e.y * g.s.y, e.z * g.s.y)); drawMeshM(M_SPHERE, TRS(ep, V3(g.s.x * 1.1f, g.s.x * 1.1f, g.s.x * 1.1f), 0), rgba(140, 16, 22)); }
}
struct LightC { Vector3 p; float r; Color c; float d; };
static Vector3 sunDirNow() { float sunA = (hourOf() - 6) / 12 * PI; return Vector3Normalize(V3(-cosf(sunA) * .8f, -std::max(.3f, sinf(sunA)), .45f)); } // night: moon hangs high-ish
static void setupLighting(Vector3 viewPos) {
    float h = hourOf(), dk = darkK();
    Vector3 sunDir = sunDirNow();
    Vector3 sunCol = Vector3Lerp(V3(1.0f, .93f, .82f), V3(.05f, .06f, .12f), dk);
    float warm = h < 12 ? 1 - smoothstep01(5.5f, 8.5f, h) : smoothstep01(16.5f, 18.5f, h) * (1 - smoothstep01(19.5f, 21.f, h)); // dawn and dusk tint fade in and out
    sunCol = Vector3Lerp(sunCol, V3(1.f, .6f, .37f), warm * (1 - dk));
    sunCol = Vector3Scale(sunCol, 1 - rainLevel * .45f);
    Vector3 amb = Vector3Lerp(V3(.4f, .42f, .46f), V3(.032f, .038f, .065f), dk); amb = Vector3Scale(amb, 1 - rainLevel * .25f);
    Vector3 sky = Vector3Lerp(V3(.35f, .42f, .55f), V3(.01f, .015f, .03f), dk);
    Vector3 fog = Vector3Lerp(V3(.55f, .58f, .6f), V3(.006f, .008f, .014f), dk); fog = Vector3Lerp(fog, V3(.4f, .43f, .46f), rainLevel * (1 - dk));
    float fogN = Lerp(42, 9, dk) * (1 - rainLevel * .4f), fogF = Lerp(90, 30, dk) * (1 - rainLevel * .35f);
    if (state == ST_TITLE) { fogN = 30; fogF = 70; }
    setU(uSunDir, &sunDir, SHADER_UNIFORM_VEC3); setU(uSunCol, &sunCol, SHADER_UNIFORM_VEC3); setU(uAmb, &amb, SHADER_UNIFORM_VEC3); setU(uSky, &sky, SHADER_UNIFORM_VEC3);
    setU(uFogCol, &fog, SHADER_UNIFORM_VEC3); Vector2 fc = state == ST_TITLE ? camPos : P.p; setU(uFogC, &fc, SHADER_UNIFORM_VEC2); setU(uFogN, &fogN, SHADER_UNIFORM_FLOAT); setU(uFogF, &fogF, SHADER_UNIFORM_FLOAT);
    setU(uView, &viewPos, SHADER_UNIFORM_VEC3); float wet = rainLevel; setU(uWet, &wet, SHADER_UNIFORM_FLOAT);
    // flashlight
    bool on = P.light && state != ST_TITLE && state != ST_DEAD;
    Vector3 flp = xz3(Vector2Add(P.p, polar(P.a, .3f)), 1.4f); Vector2 fd = polar(P.a, 1); Vector3 fdir = Vector3Normalize(V3(fd.x, -.3f, fd.y));
    Vector3 flc = on ? (P.uv ? V3(.9f, .5f, 2.2f) : V3(2.6f, 2.4f, 2.0f)) : V3(0, 0, 0); float flcos = P.uv ? .93f : .9f, flr = P.uv ? 18.f : 28.f;
    setU(uFlPos, &flp, SHADER_UNIFORM_VEC3); setU(uFlDir, &fdir, SHADER_UNIFORM_VEC3); setU(uFlCol, &flc, SHADER_UNIFORM_VEC3);
    setU(uFlCos, &flcos, SHADER_UNIFORM_FLOAT); setU(uFlRange, &flr, SHADER_UNIFORM_FLOAT);
    // point lights
    std::vector<LightC> cand; Vector2 c = state == ST_TITLE ? camPos : P.p;
    for (auto& l : lights) { float a = l.t / l.mt; cand.push_back({l.p, l.r, rgba((int)(l.c.r * a), (int)(l.c.g * a), (int)(l.c.b * a)), Vector2Distance(V2(l.p.x, l.p.z), c)}); }
    for (auto& l : staticLights) { float d = Vector2Distance(V2(l.p.x, l.p.z), c); if (d < 60) { float fl = .85f + .15f * sinf(gTime * 17 + l.p.x) * sinf(gTime * 7.3f + l.p.z); cand.push_back({l.p, l.r * fl, mulC(l.c, .3f + .7f * dk), d}); } }
    for (auto& b : bombs) if (b.flare) cand.push_back({Vector3Add(b.p, V3(0, .3f, 0)), 13, rgba(255, 50, 30), Vector2Distance(V2(b.p.x, b.p.z), c)});
    for (auto& f : fires) { float fl = .85f + .15f * sinf(gTime * 19 + f.p.x) * sinf(gTime * 7 + f.p.y); cand.push_back({xz3(f.p, .9f), 12 * fl * std::min(1.f, f.fuel / 20), rgba(255, 140, 60), Vector2Distance(f.p, c) - 5}); }
    for (auto& z : zombies) if (z.burn > 0) cand.push_back({xz3(z.p, 1), 5, rgba(255, 120, 40), Vector2Distance(z.p, c)});
    std::sort(cand.begin(), cand.end(), [](const LightC& a, const LightC& b) { return a.d < b.d; });
    int n = std::min(16, (int)cand.size()); Vector4 lp[16]; Vector3 lc[16];
    for (int i = 0; i < n; i++) { lp[i] = Vector4{cand[i].p.x, cand[i].p.y, cand[i].p.z, cand[i].r}; lc[i] = V3(cand[i].c.r / 255.f * 1.6f, cand[i].c.g / 255.f * 1.6f, cand[i].c.b / 255.f * 1.6f); }
    setU(uNL, &n, SHADER_UNIFORM_INT); if (n) { setU(uLPos, lp, SHADER_UNIFORM_VEC4, n); setU(uLCol, lc, SHADER_UNIFORM_VEC3, n); }
}
static void computeFades(float dt) {
    Vector3 cp = cam3.position; Vector3 head = xz3(P.p, 1.4f); Ray r{cp, Vector3Normalize(Vector3Subtract(head, cp))}; float pd = Vector3Distance(cp, head);
    int pb = buildingAt(P.p);
    for (int i = 0; i < (int)blds.size(); i++) {
        Building& b = blds[i]; if (fabsf((b.x0 + b.x1) / 2 - P.p.x) > 40 || fabsf((b.z0 + b.z1) / 2 - P.p.y) > 40) { b.fade = 1; b.cut = 1; continue; }
        bool occl = false; if (state != ST_TITLE) { RayCollision rc = GetRayCollisionBox(r, BoundingBox{V3(b.x0, 0, b.z0), V3(b.x1, b.h + 1, b.z1)}); occl = rc.hit && rc.distance < pd - .5f; }
        bool in = i == pb; float tf = in ? 0.f : occl ? .2f : 1.f, tc = in ? 0.f : occl ? .3f : 1.f;
        b.fade += (tf - b.fade) * std::min(1.f, dt * 8); b.cut += (tc - b.cut) * std::min(1.f, dt * 8);
    }
    for (auto& t : trees) { if (fabsf(t.p.x - P.p.x) > 30 || fabsf(t.p.y - P.p.y) > 30) { t.fade = 1; continue; } float dx = fabsf(t.p.x - P.p.x), dz = t.p.y - P.p.y; bool occ = dx < t.r + 1 && dz > -t.r && dz < t.h * .7f; t.fade += ((occ ? .25f : 1.f) - t.fade) * std::min(1.f, dt * 8); }
}
static void hull(std::vector<Vector2>& pts) {
    std::sort(pts.begin(), pts.end(), [](Vector2 a, Vector2 b) { return a.x < b.x || (a.x == b.x && a.y < b.y); });
    std::vector<Vector2> h; auto cross = [](Vector2 o, Vector2 a, Vector2 b) { return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x); };
    for (int pass = 0; pass < 2; pass++) { size_t st = h.size(); for (auto& p : pts) { while (h.size() >= st + 2 && cross(h[h.size() - 2], h.back(), p) <= 0) h.pop_back(); h.push_back(p); } h.pop_back(); std::reverse(pts.begin(), pts.end()); }
    pts = h;
}
static RenderTexture2D flRT{}; static bool flOk = false;
static RenderTexture2D makeDepthRT(int N) { RenderTexture2D r{}; r.id = rlLoadFramebuffer(); if (!r.id) return r; r.texture.width = N; r.texture.height = N; rlEnableFramebuffer(r.id);
    r.depth.id = rlLoadTextureDepth(N, N, false); r.depth.width = N; r.depth.height = N; r.depth.format = 19; r.depth.mipmaps = 1; rlFramebufferAttach(r.id, r.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
    if (!rlFramebufferComplete(r.id)) r.id = 0; rlDisableFramebuffer(); return r; }
// ------------------------------------------------------------ sun shadow map
static RenderTexture2D shadowRT{}; static int SHADOW_N = 2048; static bool shadowsOk = false, shadowsActive = false;
static void initShadows();
static void reinitShadows() { if (shadowRT.id) { rlUnloadFramebuffer(shadowRT.id); rlUnloadTexture(shadowRT.depth.id); shadowRT = RenderTexture2D{}; } shadowsOk = false; if (cfg.shadows > 0) { SHADOW_N = cfg.shadows == 1 ? 1024 : 2048; initShadows(); } }
static void initShadows() {
    shadowRT.id = rlLoadFramebuffer(); if (!shadowRT.id) return;
    shadowRT.texture.width = SHADOW_N; shadowRT.texture.height = SHADOW_N;
    rlEnableFramebuffer(shadowRT.id);
    shadowRT.depth.id = rlLoadTextureDepth(SHADOW_N, SHADOW_N, false); shadowRT.depth.width = SHADOW_N; shadowRT.depth.height = SHADOW_N; shadowRT.depth.format = 19; shadowRT.depth.mipmaps = 1;
    rlFramebufferAttach(shadowRT.id, shadowRT.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
    shadowsOk = rlFramebufferComplete(shadowRT.id); rlDisableFramebuffer();
    TraceLog(LOG_INFO, "NIGHTFALL: shadow map %s", shadowsOk ? "ready" : "unavailable, using blob shadows");
    flRT = makeDepthRT(1024); flOk = flRT.id != 0;
}
static void setMatU(int u, const Matrix& m) { Shader* sh[2] = {&lit, &litSkin}; for (int k = 0; k < 2; k++) if (ULOC[k][u] >= 0) SetShaderValueMatrix(*sh[k], ULOC[k][u], m); }
static void drawCasters(Vector2 c, float R, bool withPlayer = true) {
    litMat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE; setTexMode(0);
    for (int cx = gcl(c.x - R); cx <= gcl(c.x + R); cx++) for (int cz = gcl(c.y - R); cz <= gcl(c.y + R); cz++) {
        if (chunkHas[cx][cz]) DrawMesh(chunkMesh[cx][cz], litMat, MatrixIdentity());
        for (int pi : pgrid[cx][cz]) { const Prop& p = props[pi]; if (p.layer == L_ALWAYS || p.layer == L_FLOOR || p.layer == L_WALLDET) continue; drawMeshM(p.mesh, TRS(p.p, p.s, p.yaw), WHITE); }
        (void)0;
        for (int ti : tgrid[cx][cz]) { const Tree& t = trees[ti];
            if (t.asset >= 0) { drawAsset(t.asset, placeM(t.asset, V3(t.p.x, -.05f, t.p.y), t.sc, t.yaw)); continue; }
            drawMeshM(t.kind == 0 ? M_CONE : M_SPHERE, TRS(V3(t.p.x, t.h * .4f, t.p.y), V3(t.r * 2, t.h * .6f, t.r * 2), 0), WHITE); }
    }
    for (auto& d : doorObjs) { if (d.broken || d.anim > .5f || fabsf(d.c.x - c.x) > R || fabsf(d.c.y - c.y) > R) continue; // closed doors block light like walls do
        drawMeshM(M_CUBE, TRS(V3(d.c.x, 1.25f, d.c.y), d.horiz ? V3(d.w, 2.5f, .12f) : V3(.12f, 2.5f, d.w), 0), WHITE); }
    if (!haveChars) return;
    int n = 0; for (auto& z : zombies) { if (z.look < 0 || fabsf(z.p.x - c.x) > R || fabsf(z.p.y - c.y) > R || ++n > 50) continue;
        PoseIn pin; pin.kind = z.type == Z_BRUTE ? PK_BRUTE : PK_ZOMBIE; pin.phase = z.walk * 3.6f; pin.move = 1; pin.seed = z.seed; pin.armL = z.armL; pin.armR = z.armR; drawCharacter(z.look, pin, z.p, z.a, WHITE, 0, false); }
    if (withPlayer && state != ST_DEAD && state != ST_TITLE && playerLooks[0] >= 0) { PoseIn pin; pin.kind = PK_PLAYER; pin.phase = P.step * 1.85f; pin.crouch = P.crouch; pin.hold = curGunHold(); drawCharacter(outfitLook(), pin, P.p, P.a, WHITE, 0, false); }
}
static void renderShadows() {
    rlActiveTextureSlot(11); rlDisableTexture(); rlActiveTextureSlot(0);
    float dk = darkK(); int on = shadowsOk && cfg.shadows > 0 && dk < .9f ? 1 : 0; shadowsActive = on; setU(uShadowOn, &on, SHADER_UNIFORM_INT);
    if (!on) return;
    Vector3 sd = sunDirNow(); float vr = 48 / camZoom, ext = 2 * vr + 24, texel = ext / SHADOW_N;
    Vector3 tgt = V3(roundf(camPos.x / texel) * texel, 0, roundf((camPos.y - 5) / texel) * texel);
    Camera3D lc{}; lc.target = tgt; lc.position = Vector3Subtract(tgt, Vector3Scale(sd, 200)); lc.up = V3(0, 1, 0); lc.fovy = ext; lc.projection = CAMERA_ORTHOGRAPHIC;
    int one = 1, zero = 0; setU(uShadowPass, &one, SHADER_UNIFORM_INT);
    BeginTextureMode(shadowRT); ClearBackground(WHITE); BeginMode3D(lc);
    Matrix lv = rlGetMatrixModelview(), lp = rlGetMatrixProjection();
    drawCasters(V2(tgt.x, tgt.z), ext * .75f);
    EndMode3D(); EndTextureMode();
    setU(uShadowPass, &zero, SHADER_UNIFORM_INT);
    setMatU(uLightVP, MatrixMultiply(lv, lp)); float tx = 1.f / SHADOW_N; setU(uShadowTexel, &tx, SHADER_UNIFORM_FLOAT);
    rlActiveTextureSlot(11); rlEnableTexture(shadowRT.depth.id); rlActiveTextureSlot(0);
}
static void renderFlashShadow() {
    rlActiveTextureSlot(12); rlDisableTexture(); rlActiveTextureSlot(0);
    int on = flOk && cfg.shadows > 0 && P.light && state != ST_TITLE && state != ST_DEAD ? 1 : 0; setU(uFlShadowOn, &on, SHADER_UNIFORM_INT); if (!on) return;
    Vector2 fd = polar(P.a, 1); Vector3 dir = Vector3Normalize(V3(fd.x, -.3f, fd.y)); Vector3 pos = xz3(Vector2Add(P.p, polar(P.a, .3f)), 1.4f + (P.loft >= 0 ? lofts[P.loft].y : 0));
    Camera3D lc{}; lc.position = pos; lc.target = Vector3Add(pos, dir); lc.up = V3(0, 1, 0); lc.fovy = P.uv ? 52 : 62; lc.projection = CAMERA_PERSPECTIVE;
    int one = 1, zero = 0; setU(uShadowPass, &one, SHADER_UNIFORM_INT); rlSetClipPlanes(.15, 40);
    BeginTextureMode(flRT); ClearBackground(WHITE); BeginMode3D(lc); Matrix lv = rlGetMatrixModelview(), lp = rlGetMatrixProjection();
    drawCasters(Vector2Add(P.p, Vector2Scale(fd, 12)), 18, false); EndMode3D(); EndTextureMode();
    rlSetClipPlanes(1.0, 600.0); setU(uShadowPass, &zero, SHADER_UNIFORM_INT); setMatU(uFlVP, MatrixMultiply(lv, lp));
    rlActiveTextureSlot(12); rlEnableTexture(flRT.depth.id); rlActiveTextureSlot(0);
}
static void renderWorld3D() {
    float vr = 48 / camZoom; Vector2 c = state == ST_TITLE ? camPos : camPos;
    auto vis = [&](float x, float z, float m) { return fabsf(x - c.x) < vr + m && fabsf(z - c.y + 6) < vr * .8f + m; };
    float dk = darkK(); int uDet1 = 1, uDet0 = 0;
    BeginShaderMode(lit);
    SetShaderValue(lit, ULOC[0][uDetail], &uDet1, SHADER_UNIFORM_INT); groundMat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE; if (groundAtlasOk) setTexMode(5, 1.f / 5.f); DrawMesh(groundMesh, groundMat, MatrixIdentity()); setTexMode(0);
    SetShaderValue(lit, ULOC[0][uDetail], &uDet0, SHADER_UNIFORM_INT);
    // roads
    for (auto& r : roads) { if (r.tier > 0) continue; // gravel and dirt are painted into the ground
        Vector2 a = r.a, b = r.b; float len = Vector2Distance(a, b), yaw = atan2f(b.y - a.y, b.x - a.x);
        // cull: skip if far from view
        if (segDist(c, a, b) > vr + 20) continue;
        float segL = 30; int ns = (int)(len / segL) + 1;
        for (int i = 0; i < ns; i++) { float s0 = i * len / ns, s1 = (i + 1) * len / ns; Vector2 m = Vector2Lerp(a, b, (s0 + s1) / 2 / len); if (!vis(m.x, m.y, 20)) continue;
            drawTexMesh(M_CUBE, TRS(V3(m.x, .02f, m.y), V3(s1 - s0 + .02f, .04f, r.w), yaw), r.town ? rgba(128, 128, 128) : rgba(112, 112, 116), rainLevel > .35f ? TX_ASPHALT_WET : TX_ASPHALT); setTexMode(0);
            if (!r.town) for (float q = s0; q < s1; q += 6) { Vector2 dp = Vector2Lerp(a, b, (q + 1.5f) / len); drawBox(V3(dp.x, .045f, dp.y), V3(2.5f, .01f, .14f), yaw, rgba(200, 170, 80)); }
            if (r.tier == 0) { // ragged edge: broken patches of asphalt and gravel hugging both sides (fixed per position, so no shimmer)
                Vector2 nrm = V2(-sinf(yaw), cosf(yaw));
                for (float q = s0; q < s1; q += 1.6f) for (int sd = -1; sd <= 1; sd += 2) {
                    float x1 = sinf(q * 12.9898f + sd * 78.233f) * 43758.5453f, x2 = sinf(q * 39.346f + sd * 11.135f) * 24634.634f; float h1 = x1 - floorf(x1), h2 = x2 - floorf(x2);
                    if (h1 < .45f) continue; float off = r.w / 2 + .2f + h2 * 2.6f; Vector2 p = Vector2Add(Vector2Lerp(a, b, (q + .4f) / len), Vector2Scale(nrm, off * sd));
                    if (!vis(p.x, p.y, 4)) continue;
                    drawTexMesh(M_CUBE, TRS(V3(p.x, .03f, p.y), V3(.7f + h2, .03f, .5f + h1 * .6f), yaw + h1 * 2), rgba(94 + (int)(h2 * 30), 92 + (int)(h2 * 26), 88 + (int)(h2 * 20)), TX_ASPHALT); setTexMode(0);
                } }
            else { Vector2 nrm = V2(-sinf(yaw), cosf(yaw)); for (int sd = -1; sd <= 1; sd += 2) drawTexMesh(M_CUBE, TRS(V3(m.x + nrm.x * (r.w / 2 + .9f) * sd, .08f, m.y + nrm.y * (r.w / 2 + .9f) * sd), V3(s1 - s0, .16f, 1.8f), yaw), rgba(150, 150, 146), TX_CONCRETE); setTexMode(0); } }
    }
    // static chunks
    for (int cx = gcl(c.x - vr - 20); cx <= gcl(c.x + vr + 20); cx++) for (int cz = gcl(c.y - vr - 20); cz <= gcl(c.y + vr + 10); cz++) if (chunkHas[cx][cz]) { litMat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE; DrawMesh(chunkMesh[cx][cz], litMat, MatrixIdentity()); }
    // props (opaque)
    std::vector<int> transp; static std::vector<int> loftT; loftT.clear();
    for (int cx = gcl(c.x - vr - 20); cx <= gcl(c.x + vr + 20); cx++) for (int cz = gcl(c.y - vr - 20); cz <= gcl(c.y + vr + 10); cz++) for (int pi : pgrid[cx][cz]) {
        const Prop& p = props[pi]; if (p.layer == L_ALWAYS || !vis(p.p.x, p.p.z, 10)) continue;
        float fade = 1, cut = 1; if (p.bld >= 0) { fade = blds[p.bld].fade; cut = blds[p.bld].cut; }
        if (p.layer == L_ROOF && fade < .98f) { if (fade > .03f) transp.push_back(pi); continue; }
        if (p.layer == L_LOFT && P.loft < 0 && buildingAt(P.p) == p.bld) { loftT.push_back(pi); continue; }
        if (p.layer == L_WALLDET && cut < .6f) continue;
        if (p.layer == L_WALL && cut < .98f) { float h = Lerp(1.0f, p.s.y, cut); drawTexMesh(M_CUBE, TRS(V3(p.p.x, h / 2, p.p.z), V3(p.s.x, h, p.s.z), p.yaw), p.c, p.tex); continue; }
        drawTexMesh(p.mesh, TRS(p.p, p.s, p.yaw), p.c, p.tex);
    }
    setTexMode(0);
    // doors
    for (auto& d : doorObjs) { if (!vis(d.c.x, d.c.y, 4)) continue; float cut = blds[d.bld].cut, hgt = Lerp(1.0f, 2.45f, cut); bool barn = d.w > 3; int tx = d.metal ? TX_METAL : barn ? TX_BARNWOOD : TX_HARDWOOD; Color dc = d.metal ? rgba(150, 156, 160) : barn ? rgba(150, 70, 50) : rgba(150, 120, 90);
        dc = lerpC(dc, rgba(60, 54, 46), (1 - d.cond) * .5f + (d.hpMax > 0 ? (1 - d.hp / d.hpMax) * .4f : 0)); // weathered, then beaten
        d.anim += ((d.open || d.broken ? 1.f : 0.f) - d.anim) * std::min(1.f, GetFrameTime() * 10); d.bang -= GetFrameTime();
        if (d.broken) { drawTexMesh(M_CUBE, TRS(V3(d.c.x + (d.horiz ? 0 : .9f), .06f, d.c.y + (d.horiz ? .9f : 0)), d.horiz ? V3(d.w * .9f, .08f, 2.2f) : V3(2.2f, .08f, d.w * .9f), .3f), mulC(dc, .7f), tx); continue; }
        float sw = d.anim * PI / 2 * .95f, jig = d.bang > 0 ? sinf(gTime * 90) * .03f : 0;
        Vector2 hinge = d.horiz ? V2(d.c.x - d.w / 2, d.c.y) : V2(d.c.x, d.c.y - d.w / 2); Vector2 dir = d.horiz ? polar(sw, 1) : polar(PI / 2 - sw, 1); // swings inward-ish
        Vector2 mid = Vector2Add(hinge, Vector2Scale(dir, d.w / 2)); float yaw = atan2f(dir.y, dir.x) + jig;
        drawTexMesh(M_CUBE, TRS(V3(mid.x, hgt / 2 + .03f, mid.y), V3(d.w, hgt, .08f), yaw), dc, tx); }
    setTexMode(0);
    // campfires: stones and logs
    for (auto& f : fires) { if (!vis(f.p.x, f.p.y, 2)) continue; for (int k = 0; k < 8; k++) { Vector2 sp = Vector2Add(f.p, polar(k * PI / 4, .55f)); drawMeshM(M_SPHERE, TRS(xz3(sp, .07f), V3(.22f, .16f, .2f), k), rgba(110, 108, 100)); }
        for (int k = 0; k < 3; k++) drawMeshM(M_CUBE, TRS(xz3(f.p, .12f + k * .04f), V3(.8f, .1f, .1f), k * 1.05f), rgba(60, 40, 26)); }
    // trees
    std::vector<int> tvis;
    for (int cx = gcl(c.x - vr - 10); cx <= gcl(c.x + vr + 10); cx++) for (int cz = gcl(c.y - vr - 10); cz <= gcl(c.y + vr + 10); cz++) for (int ti : tgrid[cx][cz]) { const Tree& t = trees[ti]; if (!vis(t.p.x, t.p.y, 8)) continue; tvis.push_back(ti);
        if (t.asset >= 0) { if (t.fade > .97f) drawAsset(t.asset, placeM(t.asset, V3(t.p.x, -.05f, t.p.y), t.sc, t.yaw)); continue; }
        drawMeshM(M_CYL, TRS(V3(t.p.x, 0, t.p.y), V3(.45f, t.h * .5f, .45f), 0), rgba(70, 52, 38));
        if (t.fade > .97f) { if (t.kind == 0) { drawMeshM(M_CONE, TRS(V3(t.p.x, t.h * .25f, t.p.y), V3(t.r * 2, t.h * .55f, t.r * 2), t.p.x), t.c); drawMeshM(M_CONE, TRS(V3(t.p.x, t.h * .55f, t.p.y), V3(t.r * 1.5f, t.h * .45f, t.r * 1.5f), t.p.y), mulC(t.c, 1.12f)); }
            else { drawMeshM(M_SPHERE, TRS(V3(t.p.x, t.h * .62f, t.p.y), V3(t.r * 2, t.r * 1.6f, t.r * 2), 0), t.c); drawMeshM(M_SPHERE, TRS(V3(t.p.x + t.r * .4f, t.h * .78f, t.p.y - t.r * .3f), V3(t.r * 1.2f, t.r, t.r * 1.2f), 0), mulC(t.c, 1.15f)); } } }
    // loot
    for (auto& l : loot) { if (!vis(l.p.x, l.p.z, 2)) continue;
        { int ai = -1; float sc = 1; Vector3 at = l.p; Matrix M;
          if (l.kind == LK_GUN) { ai = gunAsset(l.id); if (ai >= 0) { sc = gunScale(l.id, ai); M = MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixScale(sc, sc, sc), MatrixRotateX(PI / 2)), MatrixRotateY(-l.rot)), MatrixTranslate(at.x, at.y + .05f, at.z)); drawAsset(ai, M, 1, l.w.key == W_AR15 || l.w.key == W_M4 || l.w.key == W_MP5 ? rgba(110, 110, 115) : WHITE); continue; } }
          else if (l.kind == LK_MELEE) { ai = meleeAsset(l.id); if (ai >= 0) { sc = meleeScale(l.id, ai); M = MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixScale(sc, sc, sc), MatrixRotateX(PI / 2)), MatrixRotateY(-l.rot)), MatrixTranslate(at.x, at.y + .03f, at.z)); drawAsset(ai, M); continue; } }
          else if (l.kind == LK_ITEM) { ai = itemAsset(l.id); if (ai >= 0) { sc = itemScale(l.id, ai); drawAsset(ai, placeM(ai, at, sc, l.rot)); continue; } } }
        if (l.kind == LK_GUN) { drawBox(V3(l.p.x, l.p.y + .05f, l.p.z), V3(WB[l.id].len + .3f, .08f, .1f), l.rot, rgba(40, 42, 46)); drawBox(V3(l.p.x, l.p.y + .05f, l.p.z), V3(.15f, .1f, .12f), l.rot, RARC[l.w.rar]); }
        else if (l.kind == LK_MELEE) drawBox(V3(l.p.x, l.p.y + .03f, l.p.z), V3(MB[l.id].len, .05f, .06f), l.rot, MB[l.id].c);
        else if (l.kind == LK_CLOTH) { int sl = CD[l.c->def].slot; Vector3 sz = sl == CS_BACK ? V3(.45f, .25f, .32f) : sl == CS_FEET ? V3(.3f, .12f, .22f) : sl == CS_HEAD || sl == CS_EYES || sl == CS_FACE || sl == CS_SOCKS || sl == CS_HANDS ? V3(.2f, .06f, .16f) : V3(.42f, .07f, .34f);
            drawBox(V3(l.p.x, l.p.y + sz.y / 2, l.p.z), sz, l.rot, l.c->col); if (l.c->pattern == 1) drawBox(V3(l.p.x, l.p.y + sz.y + .005f, l.p.z), V3(sz.x * .9f, .01f, .04f), l.rot, mulC(l.c->col, .6f)); }
        else { Color ic = IDEF[l.id].c; if (isAmmo(l.id)) drawBox(V3(l.p.x, l.p.y + .06f, l.p.z), V3(.22f, .12f, .14f), l.rot, rgba(80, 74, 52)); else if (l.id == IT_LOG) drawMeshM(M_CYL, MatrixMultiply(MatrixMultiply(MatrixScale(.28f, .9f, .28f), MatrixRotateZ(PI / 2)), MatrixMultiply(MatrixRotateY(-l.rot), MatrixTranslate(l.p.x, l.p.y + .14f, l.p.z))), ic); else if (l.id == IT_KINDLING || l.id == IT_SPLINT) { for (int k = 0; k < 3; k++) drawBox(V3(l.p.x, l.p.y + .04f + k * .05f, l.p.z), V3(.5f, .04f, .07f), l.rot + k * .5f, ic); } else if (l.id == IT_MATCHES) drawBox(V3(l.p.x, l.p.y + .03f, l.p.z), V3(.12f, .04f, .07f), l.rot, ic); else drawMeshM(M_CYL, TRS(l.p, V3(.12f, .18f, .12f), 0), ic); } }
    // corpses
    { int drawn = 0; for (int ci = (int)corpses.size() - 1; ci >= 0; ci--) { const Corpse& cp = corpses[ci]; if (cp.look < 0 || !haveChars || !vis(cp.p.x, cp.p.y, 3)) continue; if (++drawn > 70) break;
        PoseIn pin; pin.kind = PK_CORPSE; pin.seed = cp.seed; pin.armL = cp.armL; pin.armR = cp.armR; pin.head = cp.head;
        drawCharacter(cp.look, pin, cp.p, cp.yaw + PI, rgba(40, 30, 26), .3f, true); } }
    for (auto& cp : corpses) { if (cp.look >= 0 && haveChars) continue; if (!vis(cp.p.x, cp.p.y, 3)) continue; const ZDef& t = ZT[cp.type]; drawHuman(cp.p, cp.yaw + PI, 0, t.skin, t.shirt, t.pants, t.h, cp.type == Z_BRUTE ? 1.5f : 1, cp.armL, cp.armR, cp.head, 3, 1, WHITE, cp.type == Z_SOLDIER ? 2 : 0, -1); }
    for (auto& g : gibs) if (vis(g.p.x, g.p.z, 2)) drawGibM(g);
    // zombies
    for (auto& z : zombies) { if (!vis(z.p.x, z.p.y, 3)) continue; const ZDef& t = ZT[z.type];
        if (z.look >= 0 && haveChars) {
            PoseIn pin; bool fast = (z.type == Z_RUNNER || z.type == Z_VOLATILE) && (z.sees || z.aggro > 0 || z.noiseT > 0);
            pin.kind = z.type == Z_BRUTE ? PK_BRUTE : fast ? PK_RUNNER : PK_ZOMBIE; pin.phase = z.walk * (fast ? 2.4f : 3.6f); pin.move = fast ? 1.5f : 1; pin.seed = z.seed;
            pin.armL = z.armL; pin.armR = z.armR; pin.stagger = z.stun > 0 ? 1 : 0; if (z.atk < .3f) pin.attack = 1 - z.atk / .3f;
            Color tc = z.flash > 0 ? WHITE : z.burn > 0 ? rgba(255, 120, 40) : z.job ? JOBC[z.job] : WHITE; float ta = z.flash > 0 ? .7f : z.burn > 0 ? .35f : z.job ? .5f : 0;
            CharOut co; drawCharacter(z.look, pin, z.p, z.a, tc, ta, false, &co);
            if (!z.armL || !z.armR) for (int sd = 0; sd < 2; sd++) if (!(sd ? z.armR : z.armL)) { Vector3 sp = Vector3Lerp(co.chest, co.hand[sd], .18f); drawMeshM(M_SPHERE, TRS(sp, V3(.11f, .11f, .11f), 0), rgba(130, 12, 18)); }
            continue; }
        Color tint = z.flash > 0 ? rgba(255, 255, 255) : z.burn > 0 ? rgba(255, 150, 110) : WHITE; Color sk = z.flash > 0 ? rgba(255, 230, 230) : t.skin;
        drawHuman(z.p, z.a, z.walk, sk, t.shirt, t.pants, t.h, z.type == Z_BRUTE ? 1.5f : z.type == Z_BLOATER ? 1.4f : 1, z.armL, z.armR, true, 0, 0, tint, z.type == Z_SOLDIER ? 2 : 0, -1);
        if (z.type == Z_BLOATER) for (int k = 0; k < 4; k++) drawMeshM(M_SPHERE, TRS(xz3(Vector2Add(z.p, polar(z.a + k * 1.5f, .3f)), 1.1f + k * .1f), V3(.22f, .22f, .22f), 0), rgba(190, 220, 110)); }
    // player
    if (state != ST_DEAD && state != ST_TITLE) {
        Weapon* g = curGun(); int pose = g ? 1 : 2; float swingT = P.swing > 0 ? 1 - P.swing / .22f : -1;
        Color tint = P.hurt > 0 ? rgba(255, 140, 140) : WHITE;
        if (haveChars && playerLooks[0] >= 0) {
            PoseIn pin; pin.kind = PK_PLAYER; pin.phase = P.step * 1.85f; float spd = Vector2Length(P.k) > 3 ? 0 : 1; (void)spd;
            bool moving = P.moving; // real movement, so click-to-move and autorun animate the legs too
            pin.move = !moving ? 0 : P.sprint ? 1.9f : P.crouch ? .6f : 1; pin.crouch = P.crouch ? 1 : 0;
            pin.hold = g ? (g->slot == 1 ? 1 : 2) : 0; pin.swing = P.swing > 0 ? swingT : -1; pin.roll = P.roll > 0 ? 1 - P.roll / .28f : -1; pin.recoil = g ? Clamp(P.cd / std::max(.05f, g->rate), 0, 1) : 0;
            if (P.sprint && g) pin.hold = 0;
            CharOut co; int pl = outfitLook();
            float yaw = P.a; if (P.roll > 0) yaw = atan2f(P.rv.y, P.rv.x);
            else if (pin.move > 0) { P.animA += angDiff(P.moveA, P.animA) * std::min(1.f, GetFrameTime() * 14); // legs follow movement (smoothed), upper body tracks the aim; walk backwards when backpedaling
                float df = angDiff(P.a, P.animA); if (!P.back) { yaw = P.animA; pin.twist = df; } else { yaw = P.animA + PI; pin.twist = angDiff(P.a, yaw); pin.phase = -pin.phase; } }
            else P.animA = P.a;
            if (P.vaultT > 0) { float t = 1 - P.vaultT / .45f; yaw = atan2f(P.vaultB.y - P.vaultA.y, P.vaultB.x - P.vaultA.x); pin.crouch = .7f; pin.move = 0; pin.twist = 0; charY = sinf(t * PI) * (P.vaultH * .8f + .2f); }
            if (P.loft >= 0) charY = lofts[P.loft].y;
            if (P.kickT > 0) pin.kick = 1 - P.kickT / .35f; pin.guard = P.blocking;
            drawCharacter(pl, pin, P.p, yaw, rgba(255, 60, 60), P.hurt > 0 ? .35f : 0, false, &co);
            if (P.shieldHP > 0 && P.roll <= 0) { Vector3 hp = Vector3Add(co.hand[0], xz3(polar(P.a, P.blocking ? .2f : .05f), 0)); if (!P.blocking) hp = Vector3Add(hp, V3(0, -.15f, 0));
                drawTexMesh(M_CUBE, TRS(hp, V3(.62f, .82f, .05f), P.a + PI / 2 + (P.blocking ? 0 : .5f)), rgba(170, 150, 120), TX_PLYWOOD); setTexMode(0); }
            if (P.roll <= 0) {
                if (g && !P.sprint) { int ga = gunAsset(g->key); if (ga >= 0) { float sc = gunScale(g->key, ga); Vector3 hp = co.hand[1]; if (pin.hold == 2) hp = Vector3Lerp(co.hand[1], co.hand[0], .2f);
                    Matrix M = MatrixMultiply(MatrixMultiply(MatrixScale(sc, sc, sc), MatrixRotateY(-P.a)), MatrixTranslate(hp.x, hp.y, hp.z)); drawAsset(ga, M, 1, g->key == W_AR15 || g->key == W_M4 || g->key == W_MP5 ? rgba(110, 110, 115) : WHITE); } }
                else if (P.melee != MK_FISTS) { int ma = meleeAsset(P.melee); Vector3 hp = co.hand[1]; float ang = P.swing > 0 ? P.swingA - 1.2f + 2.4f * swingT : P.a + .9f;
                    if (ma >= 0) { float sc = meleeScale(P.melee, ma); Matrix M = MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixScale(sc, sc, sc), meleeAlign(ma)), MatrixRotateY(-ang)), MatrixTranslate(hp.x, hp.y, hp.z)); drawAsset(ma, M); }
                    else { Vector3 tip = Vector3Add(hp, xz3(polar(ang, MB[P.melee].len), 0)); drawBox(Vector3Lerp(hp, tip, .5f), V3(MB[P.melee].len, .05f, .06f), ang, MB[P.melee].c); } } }
            charY = 0; goto player_done;
        }
        drawHuman(P.p, P.a, P.step * .55f, rgba(200, 160, 126), rgba(64, 86, 96), rgba(52, 58, 76), P.crouch ? 1.4f : 1.8f, 1, true, true, true, pose, 0, tint, 1, swingT);
        float sh = P.crouch ? 1.05f : 1.32f;
        if (g) { Vector2 gp = Vector2Add(P.p, polar(P.a, .45f + g->len / 2)); drawBox(xz3(gp, sh), V3(g->len, .09f, .08f), P.a, rgba(34, 36, 40)); drawBox(xz3(Vector2Add(P.p, polar(P.a, .5f)), sh - .08f), V3(.12f, .14f, .07f), P.a, RARC[g->rar]); }
        else if (P.melee != MK_FISTS) { float sa = P.swing > 0 ? P.swingA - 1.2f + 2.4f * swingT : P.a + .5f; Vector2 mp = Vector2Add(P.p, polar(sa, .5f + MB[P.melee].len / 2)); drawBox(xz3(mp, sh), V3(MB[P.melee].len, .05f, .06f), sa, MB[P.melee].c); }
    }
    player_done:;
    // ---- batch pass: decals, shadows, particles
    rlDrawRenderBatchActive(); Matrix I = MatrixIdentity(); SetShaderValueMatrix(lit, lit.locs[SHADER_LOC_MATRIX_MODEL], I); SetShaderValueMatrix(lit, lit.locs[SHADER_LOC_MATRIX_NORMAL], I);
    litMat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE; Vector4 wc = {1, 1, 1, 1}; SetShaderValue(lit, lit.locs[SHADER_LOC_COLOR_DIFFUSE], &wc, SHADER_UNIFORM_VEC4);
    rlDisableDepthMask(); rlDisableBackfaceCulling();
    // sun shadows
    float sunA = (hourOf() - 6) / 12 * PI; float elev = std::max(.25f, sinf(sunA)); Vector2 sv = Vector2Scale(V2(-cosf(sunA) * .8f, .45f), 1.f / elev * .55f);
    float shA = (1 - dk) * (1 - rainLevel * .6f) * .42f;
    if (shA > .02f && !shadowsActive) {
        rlSetTexture(rlGetTextureIdDefault()); rlBegin(RL_TRIANGLES); rlColor4ub(0, 0, 0, (unsigned char)(shA * 255)); rlNormal3f(0, 1, 0);
        for (auto& b : blds) { if (!vis((b.x0 + b.x1) / 2, (b.z0 + b.z1) / 2, 30)) continue; float hh = b.h + (b.type == B_HOUSE || b.type == B_BARN ? 1.8f : 0); Vector2 s = Vector2Scale(sv, hh);
            std::vector<Vector2> pts = {V2(b.x0, b.z0), V2(b.x1, b.z0), V2(b.x1, b.z1), V2(b.x0, b.z1), V2(b.x0 + s.x, b.z0 + s.y), V2(b.x1 + s.x, b.z0 + s.y), V2(b.x1 + s.x, b.z1 + s.y), V2(b.x0 + s.x, b.z1 + s.y)}; hull(pts);
            for (size_t i = 1; i + 1 < pts.size(); i++) { rlVertex3f(pts[0].x, .046f, pts[0].y); rlVertex3f(pts[i + 1].x, .046f, pts[i + 1].y); rlVertex3f(pts[i].x, .046f, pts[i].y); } }
        rlEnd();
        rlSetTexture(softTex.id); rlBegin(RL_QUADS);
        for (int ti : tvis) { const Tree& t = trees[ti]; Vector2 s = Vector2Scale(sv, t.h * .55f); groundQuad(Vector2Add(t.p, s), .04f, t.r * 1.3f, 0, rgba(0, 0, 0, (int)(shA * 330)), {0, 0, 1, 1}); }
        rlEnd();
    }
    rlSetTexture(softTex.id); rlBegin(RL_QUADS);
    for (auto& z : zombies) if (vis(z.p.x, z.p.y, 2)) groundQuad(z.p, .05f, z.r * 1.5f, 0, rgba(0, 0, 0, shadowsActive ? 70 : 120), {0, 0, 1, 1});
    if (state == ST_PLAY || state == ST_PAUSE || state == ST_INV || state == ST_MAP || state == ST_SKILLS) groundQuad(P.p, .05f, .6f, 0, rgba(0, 0, 0, 130), {0, 0, 1, 1});
    for (auto& a : acid) { float f = std::min(1.f, a.t / 1.5f); groundQuad(a.p, .06f, a.r * f * 1.2f, 0, rgba(140, 200, 50, (int)(170 * f)), {0, 0, 1, 1}); }
    rlEnd();
    // blood decals
    rlSetTexture(bloodTex.id); rlBegin(RL_QUADS);
    for (auto& d : decals) { if (!vis(d.p.x, d.p.y, 2)) continue; groundQuad(d.p, d.y, d.r, d.rot, d.c, {d.v * .25f, 0, .25f, 1}); }
    rlEnd();
    // particles (lit)
    rlSetTexture(softTex.id); rlBegin(RL_QUADS);
    for (auto& p : parts) { if (!vis(p.p.x, p.p.z, 2)) continue; float a = p.t / p.mt;
        switch (p.kind) {
        case P_BLOOD: case P_ACID: billboard(p.p, p.s, p.c); break;
        case P_SMOKE: billboard(p.p, p.s * (2 - a), withA(p.c, a * (p.c.a / 255.f) * .6f)); break;
        case P_MIST: billboard(p.p, p.s * (1.8f - a * .8f), withA(p.c, a * (p.c.a / 255.f) * .6f)); break;
        case P_DUST: billboard(p.p, p.s * (2 - a), withA(p.c, a * .5f)); break;
        case P_CASING: billboard(p.p, p.s, p.c); break;
        default: break; } }
    rlEnd(); rlDrawRenderBatchActive(); rlEnableDepthMask(); rlEnableBackfaceCulling();
    // ---- transparent: faded roofs & canopies
    for (int pi : transp) { const Prop& p = props[pi]; Color cc = withA(p.c, blds[p.bld].fade * blds[p.bld].fade); drawTexMesh(p.mesh, TRS(p.p, p.s, p.yaw), cc, p.tex); }
    for (int pi : loftT) { const Prop& p = props[pi]; drawTexMesh(p.mesh, TRS(p.p, p.s, p.yaw), withA(p.c, .22f), p.tex); }
    setTexMode(0);
    for (int ti : tvis) { const Tree& t = trees[ti]; if (t.fade > .97f) continue; if (t.asset >= 0) { drawAsset(t.asset, placeM(t.asset, V3(t.p.x, -.05f, t.p.y), t.sc, t.yaw), t.fade); continue; } Color cc = withA(t.c, t.fade);
        if (t.kind == 0) { drawMeshM(M_CONE, TRS(V3(t.p.x, t.h * .25f, t.p.y), V3(t.r * 2, t.h * .55f, t.r * 2), t.p.x), cc); drawMeshM(M_CONE, TRS(V3(t.p.x, t.h * .55f, t.p.y), V3(t.r * 1.5f, t.h * .45f, t.r * 1.5f), t.p.y), withA(mulC(t.c, 1.12f), t.fade)); }
        else drawMeshM(M_SPHERE, TRS(V3(t.p.x, t.h * .62f, t.p.y), V3(t.r * 2, t.r * 1.6f, t.r * 2), 0), cc); }
    EndShaderMode();
    // ---- emissive (default shader, additive)
    BeginBlendMode(BLEND_ADDITIVE); rlDisableDepthMask();
    rlSetTexture(softTex.id); rlBegin(RL_QUADS);
    for (auto& p : parts) { if (!vis(p.p.x, p.p.z, 2)) continue; float a = p.t / p.mt; if (p.kind == P_FIRE) billboard(p.p, p.s * (.6f + a), withA(p.c, a)); else if (p.kind == P_SPARK) billboard(p.p, p.s * 2, withA(p.c, a)); }
    for (auto& l : lights) if (l.r > 10 && l.r < 18 && l.t / l.mt > .4f) billboard(l.p, .55f, rgba(255, 220, 160, 230));
    for (auto& b : bombs) { if (b.flare) { billboard(b.p, .9f + sinf(gTime * 40) * .1f, rgba(255, 70, 40, 255)); billboard(b.p, .3f, rgba(255, 220, 200, 255)); } else if (b.blink > 0) billboard(b.p, .3f, rgba(255, 50, 30, 255)); }
    for (auto& l : staticLights) if (Vector2Distance(V2(l.p.x, l.p.z), c) < vr + 10) billboard(l.p, .5f + .1f * sinf(gTime * 20 + l.p.x), rgba(255, 140, 50, (int)(150 + 100 * dk)));
    for (auto& f : fires) if (vis(f.p.x, f.p.y, 2)) { billboard(xz3(f.p, .45f), .6f + .1f * sinf(gTime * 23 + f.p.x), rgba(255, 150, 60, 230)); billboard(xz3(f.p, .5f), 1.6f, rgba(255, 110, 40, (int)(50 + 60 * dk))); }
    for (auto& z : zombies) { if (!vis(z.p.x, z.p.y, 2)) continue; bool vol = z.type == Z_VOLATILE; if (!vol && dk < .5f) continue; Vector2 hp = Vector2Add(z.p, polar(z.a, .14f + z.r * .3f)); Vector2 rt = polar(z.a + PI / 2, .07f); float hy = ZT[z.type].h * .9f;
        Color e = vol ? rgba(255, 70, 40, 255) : rgba(200, 40, 30, (int)(160 * dk)); billboard(xz3(Vector2Add(hp, rt), hy), vol ? .09f : .05f, e); billboard(xz3(Vector2Subtract(hp, rt), hy), vol ? .09f : .05f, e); if (vol) billboard(xz3(hp, hy), .4f, rgba(255, 60, 30, 70)); }
    for (auto& l : loot) if (l.kind == LK_GUN && l.w.rar >= 2 && vis(l.p.x, l.p.z, 2)) { Color rc = RARC[l.w.rar]; for (int k = 0; k < 6; k++) billboard(Vector3Add(l.p, V3(0, .3f + k * .45f, 0)), .28f - k * .03f, withA(rc, .35f - k * .05f)); }
    rlEnd();
    // tracers
    rlSetTexture(rlGetTextureIdDefault()); rlBegin(RL_QUADS);
    for (auto& b : bullets) { Vector2 d = Vector2Normalize(b.v); Vector3 a = xz3(Vector2Subtract(b.p, Vector2Scale(d, 2.2f)), 1.3f), e = xz3(b.p, 1.3f); Vector3 side = V3(-d.y * .025f, 0, d.x * .025f);
        Color tc = b.burn ? rgba(255, 140, 60, 230) : rgba(255, 225, 160, 200); rlColor4ub(tc.r, tc.g, tc.b, 0); rlVertex3f(a.x - side.x, a.y, a.z - side.z); rlVertex3f(a.x + side.x, a.y, a.z + side.z); rlColor4ub(tc.r, tc.g, tc.b, tc.a); rlVertex3f(e.x + side.x, e.y, e.z + side.z); rlVertex3f(e.x - side.x, e.y, e.z - side.z); }
    if (P.swing > 0 && state == ST_PLAY) { float t = 1 - P.swing / .22f; const MBase& m = MB[P.melee]; for (int i = 0; i < 10; i++) { float a0 = P.swingA - m.arc + 2 * m.arc * t * i / 10.f, a1 = P.swingA - m.arc + 2 * m.arc * t * (i + 1) / 10.f;
        Vector2 p0 = Vector2Add(P.p, polar(a0, m.range * .5f)), p1 = Vector2Add(P.p, polar(a1, m.range * .5f)), q0 = Vector2Add(P.p, polar(a0, m.range)), q1 = Vector2Add(P.p, polar(a1, m.range)); int al = (int)(90 * (1 - t));
        rlColor4ub(220, 230, 240, 0); rlVertex3f(p0.x, 1.2f, p0.y); rlVertex3f(p1.x, 1.2f, p1.y); rlColor4ub(220, 230, 240, al); rlVertex3f(q1.x, 1.2f, q1.y); rlVertex3f(q0.x, 1.2f, q0.y); } }
    rlEnd();
    // rain
    if (rainLevel > .05f) { rlBegin(RL_LINES); int n = (int)(700 * rainLevel); for (int i = 0; i < n; i++) { float rx = c.x + fmodf(i * 97.31f + gTime * 3, vr * 2) - vr, rz = c.y + fmodf(i * 53.7f, vr * 1.7f) - vr * .8f; float ry = 14 - fmodf(gTime * (22 + (i % 5) * 2) + i * 3.17f, 14);
        rlColor4ub(170, 190, 220, (unsigned char)(70 * (1 - dk * .6f))); rlVertex3f(rx, ry, rz); rlVertex3f(rx - .1f, ry + .9f, rz + .05f); } rlEnd(); }
    rlDrawRenderBatchActive(); rlEnableDepthMask(); EndBlendMode();
}

// ============================================================ HUD & UI
static void text(const std::string& s, float x, float y, float size, Color c, int align = 0, bool shadow = true) {
    Vector2 m = MeasureTextEx(font, s.c_str(), size, size / 14); float ox = align == 1 ? -m.x / 2 : align == 2 ? -m.x : 0;
    if (shadow) DrawTextEx(font, s.c_str(), V2(x + ox + 1.5f, y + 1.5f), size, size / 14, rgba(0, 0, 0, (int)(c.a * .7f)));
    DrawTextEx(font, s.c_str(), V2(x + ox, y), size, size / 14, c);
}
static float textW(const std::string& s, float size) { return MeasureTextEx(font, s.c_str(), size, size / 14).x; }
static void bar(float x, float y, float w, float h, float v, Color c) { DrawRectangleRec({x, y, w, h}, rgba(8, 10, 14, 180)); DrawRectangleRec({x, y, w * Clamp(v, 0, 1), h}, c); DrawRectangleLinesEx({x, y, w, h}, 1, rgba(255, 255, 255, 28)); }
static std::string hhmm() { float h = hourOf(); int hh = (int)h, mm = (int)((h - hh) * 60); char b[8]; snprintf(b, sizeof b, "%02d:%02d", hh, mm); return b; }
static void drawCursor() { Vector2 m = GetMousePosition(); DrawTriangle(m, V2(m.x, m.y + 18), V2(m.x + 12, m.y + 13), BONE); }
static bool button(const char* label, float x, float y, float w, float h, bool primary) {
    Rectangle r = {x, y, w, h}; bool hov = CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRec(r, primary ? (hov ? rgba(255, 184, 90) : SODIUM) : (hov ? rgba(40, 46, 56) : rgba(18, 22, 30))); if (!primary) DrawRectangleLinesEx(r, 1, rgba(60, 68, 80));
    text(label, x + w / 2, y + h / 2 - 9, 18, primary ? rgba(26, 18, 6) : BONE, 1, false); return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static void dim(float a) { DrawRectangle(0, 0, W, H, rgba(5, 6, 9, (int)(255 * a))); }
static std::map<std::string, Texture2D> iconTex;
static Texture2D* icon(const std::string& n) { auto it = iconTex.find(n); if (it != iconTex.end()) return it->second.id ? &it->second : nullptr;
    std::string p = (assetDir.empty() ? std::string("assets/") : assetDir) + "icons/" + n + ".png"; Texture2D t{}; if (FileExists(p.c_str())) { t = LoadTexture(p.c_str()); SetTextureFilter(t, TEXTURE_FILTER_BILINEAR); } iconTex[n] = t; return t.id ? &iconTex[n] : nullptr; }
static const char* ITEM_ICON[IT_COUNT] = {"opened-food-can", "peach", "chocolate-bar", "opened-food-can", "water-bottle", "soda-can", "bandage-roll", "medicine-pills", "pill", "syringe", "battery-pack", "match-head", "dynamite",
    "bullets", "shotgun-rounds", "ammo-box", "ammo-box", "log", "wood-pile", "wood-pile", "matchbox", "rolled-cloth", "crime-scene-tape", "sewing-needle", "leather-vest", "wax-tablet", "chemical-drop", "chemical-drop", "chemical-drop", "rope-coil", "jerrycan"};
static const char* MELEE_ICON[MK_COUNT] = {"fist", "bowie-knife", "machete", "wood-axe", "baseball-bat", "crowbar", "hatchet", "monkey-wrench", "lead-pipe"};
static const char* GUN_ICON[W_COUNT] = {"pistol-gun", "pistol-gun", "mp5", "shotgun", "ak47", "ak47", "rifle"};
static const char* clothIcon(int def) { const char* n = CD[def].n; auto has = [&](const char* k) { return strstr(n, k) != nullptr; };
    switch (CD[def].slot) {
    case CS_HEAD: return has("Helmet") || has("Hard Hat") ? (has("Riot") || has("Combat") ? "visored-helm" : "helmet") : has("Cowboy") ? "western-hat" : "outback-hat";
    case CS_EYES: return has("Sunglasses") ? "sunglasses" : "spectacles";
    case CS_FACE: return has("Gas") ? "gas-mask" : has("Balaclava") ? "balaclava" : "bandana";
    case CS_TORSO: return has("T-Shirt") || has("Thermal") ? "t-shirt" : "shirt";
    case CS_OUTER: return has("Vest") || has("Carrier") ? "kevlar-vest" : has("Hoodie") || has("Sweater") ? "hoodie" : "moncler-jacket";
    case CS_HANDS: return has("Wool") ? "winter-gloves" : "gloves";
    case CS_LEGS: return "trousers"; case CS_SOCKS: return "socks";
    case CS_FEET: return has("Rubber") ? "rubber-boot" : has("Sneakers") || has("Dress") ? "running-shoe" : "boots";
    default: return has("School") ? "school-bag" : has("Drawstring") ? "knapsack" : "backpack"; } }
static void drawIcon(const char* n, Rectangle r, Color c, float pad = .16f) { Texture2D* t = icon(n); if (!t) return; float sz = std::min(r.width, r.height) * (1 - 2 * pad);
    DrawTexturePro(*t, {0, 0, (float)t->width, (float)t->height}, {r.x + r.width / 2 - sz / 2, r.y + r.height / 2 - sz / 2, sz, sz}, V2(0, 0), 0, c); }
static Color iconTint(Color c) { float l = (c.r + c.g + c.b) / 765.f; return l < .35f ? lerpC(c, rgba(230, 230, 230), .45f) : c; } // keep dark colors readable
// ---- hotbar: keys 1-0. Weapons sit in 1-3 by default; drag anything else onto a slot in the inventory
struct HotSlot { int kind = 0, id = -1; }; static HotSlot hotbar[10]; // kind 0 empty, 1 item, 2 weapon slot
static void resetHotbar() { for (auto& h : hotbar) h = {}; hotbar[0] = {2, 0}; hotbar[1] = {2, 1}; hotbar[2] = {2, 2}; hotbar[3] = {1, IT_BANDAGE}; hotbar[4] = {1, IT_RAG}; hotbar[5] = {1, IT_WATER}; }
static Rectangle hotRect(int i) { float sw = 50, x0 = W / 2.f - sw * 5; return {x0 + i * sw + 2, H - 112.f, sw - 4, 46}; }
static void useHot(int i) { HotSlot& h = hotbar[i]; if (h.kind == 2) { if (h.id == 2 || P.hasGun[h.id]) setActive(h.id); return; }
    if (h.kind == 1) { InvItem* it = invFind(h.id); if (!it) { msg(std::string("No ") + IDEF[h.id].n + " left.", STEEL); return; } if (IDEF[h.id].use[0] || h.id == IT_FLARE || h.id == IT_PIPEBOMB || h.id == IT_KINDLING || h.id == IT_LOG) useItem(it); else msg(std::string(IDEF[h.id].n) + ": right-click it in the inventory to pick a target.", STEEL); } }
static void drawHotbar(bool inv) { Vector2 m = GetMousePosition();
    for (int i = 0; i < 10; i++) { Rectangle r = hotRect(i); HotSlot& h = hotbar[i]; bool act = h.kind == 2 && P.active == h.id; bool hov = inv && CheckCollisionPointRec(m, r);
        DrawRectangleRec(r, hov ? rgba(40, 46, 58, 230) : rgba(10, 12, 16, 190)); DrawRectangleLinesEx(r, act ? 2 : 1, act ? SODIUM : rgba(70, 76, 86)); text(std::to_string((i + 1) % 10), r.x + 3, r.y + 2, 10, STEEL);
        std::string nm; Color c = BONE; int n = -1;
        if (h.kind == 2) { if (h.id == 2) nm = MB[P.melee].n; else if (P.hasGun[h.id]) { nm = P.gun[h.id].name; c = RARC[P.gun[h.id].rar]; } else { nm = h.id ? "sidearm" : "primary"; c = rgba(90, 96, 106); } }
        else if (h.kind == 1) { nm = IDEF[h.id].n; n = invCount(h.id); if (n == 0) c = rgba(100, 104, 112); DrawRectangle((int)r.x + 3, (int)r.y + 14, (int)r.width - 6, 3, n ? IDEF[h.id].c : rgba(60, 60, 64)); }
        { const char* ic = h.kind == 2 ? (h.id == 2 ? MELEE_ICON[P.melee] : P.hasGun[h.id] ? GUN_ICON[P.gun[h.id].key] : nullptr) : h.kind == 1 ? ITEM_ICON[h.id] : nullptr;
          if (ic && icon(ic)) { drawIcon(ic, {r.x, r.y + 6, r.width, r.height - 8}, h.kind == 1 ? (n ? iconTint(IDEF[h.id].c) : rgba(80, 84, 90)) : c, .2f); nm.clear(); } }
        if (!nm.empty()) { size_t mx = 8; if (nm.size() > mx) { size_t sp = nm.rfind(' '); nm = sp != std::string::npos && nm.size() - sp - 1 <= mx ? nm.substr(sp + 1) : nm.substr(0, mx); } text(nm, r.x + r.width / 2, r.y + 21, 10, c, 1); }
        if (n >= 0) text(std::to_string(n), r.x + r.width - 4, r.y + 32, 11, n ? BONE : rgba(100, 104, 112), 2);
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && h.kind == 1) h = {}; }
    if (inv) text("Drag items onto the bar. Right-click a slot to clear it.", W / 2.f, H - 126.f, 11, rgba(120, 126, 136), 1); }
static void drawHUD() {
    float m = 22;
    for (auto& d : sdrops) { float a = d.t / d.mt; DrawCircleV(d.p, d.r, rgba(110, 4, 8, (int)(170 * a))); DrawCircleV(Vector2Add(d.p, V2(d.r * .3f, d.r * .8f)), d.r * .4f, rgba(110, 4, 8, (int)(140 * a))); DrawRectangleRec({d.p.x - d.r * .12f, d.p.y, d.r * .24f, d.r * 2.2f * (1 - a) + d.r}, rgba(100, 4, 8, (int)(120 * a))); }
    float lo = P.hp < 30 ? (30 - P.hp) / 40 : 0, hv = std::max(lo, P.hurt > 0 ? P.hurt * .9f : 0);
    if (hv > 0) DrawTexturePro(vignTex, {0, 0, 512, 512}, {0, 0, (float)W, (float)H}, V2(0, 0), 0, rgba(170, 0, 10, (int)(255 * std::min(.75f, hv))));
    if (P.blockFx > 0) DrawTexturePro(vignTex, {0, 0, 512, 512}, {0, 0, (float)W, (float)H}, V2(0, 0), 0, rgba(255, 230, 180, (int)(400 * P.blockFx)));
    if (P.shiver > 0) DrawTexturePro(vignTex, {0, 0, 512, 512}, {0, 0, (float)W, (float)H}, V2(0, 0), 0, rgba(120, 170, 255, (int)(110 * P.shiver)));
    // top-left: time & place
    int t = townAt(P.p, 1.0f); std::string place = t >= 0 ? TOWNS[t].name : (roadDist(P.p) < 6 ? "Highway" : "Wilderness"); { int cb = cellOf(P.p); if (cb >= 0) place = blds[cb].cellKind == 2 ? "Basement" : "Upstairs"; }
    text("DAY " + std::to_string(dayN()) + "   " + hhmm(), m, m, 24, BONE);
    { float h = hourOf(), bw = 220, by = m + 50; DrawRectangle((int)m, (int)by, (int)bw, 6, rgba(0, 0, 0, 120)); DrawRectangle((int)m, (int)by, (int)(bw * h / 24), 6, rgba(235, 200, 120));
      for (float t : {5.5f, 18.5f}) DrawRectangle((int)(m + bw * t / 24) - 1, (int)by - 3, 2, 12, rgba(255, 255, 255, 170));
      for (float t : {13.f, 18.f}) DrawRectangle((int)(m + bw * t / 24) - 1, (int)by + 8, 2, 4, rgba(255, 90, 60, 230));
      auto hoursTo = [&](float t) { return fmodf(t - h + 24, 24); }; auto fmt = [](float hr) { int mn = (int)roundf(hr * 60); return TextFormat("%d:%02d", mn / 60, mn % 60); };
      std::string cd = (h >= 18.5f && h < 20.5f) ? "Dusk now" : isNight() ? std::string("Night. Dawn in ") + fmt(hoursTo(5.5f)) : std::string("Dusk in ") + fmt(hoursTo(18.5f));
      if (h >= 13 && h < 18 && !horde.on) cd += "  ·  Horde window open"; else if (h < 13 && !horde.on && hordeDay < dayN()) cd += "  ·  Horde window at 13:00";
      text(cd, m + bw + 10, by - 6, 14, STEEL); } text(place + (rainLevel > .4f ? "  ·  Rain" : ""), m, m + 30, 15, STEEL);
    text(std::to_string(kills) + " killed", m, m + 50, 13, STEEL);
    text(std::string(CL().n) + "  ·  " + std::to_string(P.points) + " skill pts  ·  [K]", m, m + 68, 13, withA(CL().c, .85f));
    { int hz = hotAt(P.p, 1.f); if (hz >= 0) text(std::string("HOT ZONE: ") + HZN[hotZones[hz].kind], m, m + 86, 14, rgba(255, 90, 60)); }
    { float ax = W / 2.f - 130, ay = H - m - 34; bool ready = P.actCd <= 0; DrawRectangleRec({ax, ay, 260, 26}, rgba(8, 10, 14, 180)); if (!ready) DrawRectangleRec({ax, ay, 260 * (1 - P.actCd / (CL().actCd * (1 - P.perks.cdr))), 26}, withA(CL().c, .25f));
      if (P.actT > 0 && (P.cls == 0 || P.cls >= 3)) DrawRectangleLinesEx({ax - 2, ay - 2, 264, 30}, 2, CL().c);
      DrawRectangleLinesEx({ax, ay, 260, 26}, 1, ready ? CL().c : rgba(60, 68, 80)); text(std::string("[Z]  ") + CL().act + (ready ? "" : "  " + std::to_string((int)ceilf(P.actCd)) + "s"), ax + 130, ay + 5, 14, ready ? CL().c : STEEL, 1, false); }
    drawHotbar(false);
    // messages
    float my = H - m - 190; for (int i = (int)msgs.size() - 1; i >= 0; i--) { float a = std::min(1.f, msgs[i].life); text(msgs[i].t, m, my, 15, withA(msgs[i].c, a)); my -= 20; }
    // vitals
    float by = H - m - 120, bw = 220;
    auto vit = [&](const char* l, float v, Color c, float y) { text(l, m, y - 2, 12, STEEL); bar(m + 66, y, bw, 9, v, c); };
    vit("HEALTH", P.hp / P.perks.maxhp, P.hp < 30 ? rgba(224, 65, 62) : BLOODC, by);
    vit("FOOD", P.food / 100, P.food < 20 ? rgba(240, 120, 60) : rgba(200, 160, 80), by + 17);
    vit("WATER", P.water / 100, P.water < 20 ? rgba(240, 120, 60) : rgba(90, 160, 220), by + 34);
    vit("STAMINA", P.sta / 100, rgba(217, 196, 122), by + 51);
    { text("TEMP", m, by + 66, 12, STEEL); float tv = (P.temp - 33) / 5.5f; bar(m + 66, by + 68, bw, 9, 1, rgba(30, 36, 48)); DrawRectangleGradientH((int)(m + 66), (int)(by + 68), (int)bw, 9, COLDC, rgba(255, 120, 80)); DrawRectangle((int)(m + 66 + bw * Clamp(tv, 0, 1)) - 2, (int)by + 65, 4, 15, BONE);
      char tb[16]; snprintf(tb, 16, "%.1f°", P.temp); text(tb, m + 66 + bw + 8, by + 64, 13, P.temp < 35.5f ? COLDC : BONE); }
    bar(m + 66, by + 85, bw * .6f, 6, P.batt / 100, P.uv ? rgba(183, 156, 255) : rgba(200, 200, 160)); text(P.light ? (P.uv ? "UV" : "LIGHT") : "light", m, by + 81, 12, P.light ? BONE : STEEL);
    float sx = m; float sy = by - 26; auto status = [&](const std::string& s, Color c) { DrawRectangleRec({sx, sy, textW(s, 13) + 14, 20}, rgba(10, 12, 16, 200)); DrawRectangleLinesEx({sx, sy, textW(s, 13) + 14, 20}, 1, c); text(s, sx + 7, sy + 3, 13, c, 0, false); sx += textW(s, 13) + 20; };
    if (P.bleeds) status("BLEEDING x" + std::to_string(P.bleeds), BLOODC); if (P.sick > 0) status("SICK", rgba(170, 200, 90)); if (P.temp < 35.5f) status("COLD", COLDC);
    if (P.broken) status(P.splinted ? "SPLINTED " + std::to_string((int)roundf(legFactor() * 100)) + "%" : std::string("BROKEN LEG"), rgba(255, 150, 90)); if ((P.crouch && gExposure > .02f) || P.hideK < .5f) { Color ec = gExposure < .33f ? lerpC(rgba(90, 220, 110), rgba(235, 210, 60), gExposure / .33f) : gExposure < .66f ? lerpC(rgba(235, 210, 60), rgba(255, 140, 40), (gExposure - .33f) / .33f) : lerpC(rgba(255, 140, 40), rgba(255, 50, 40), Clamp((gExposure - .66f) / .34f, 0, 1));
      if (gExposure > .85f && ((int)(GetTime() * 6) % 2 == 0)) ec = rgba(255, 40, 40);
      status(gExposure > .85f ? "SPOTTED!" : gExposure > .02f ? "NOTICED" : P.loft >= 0 ? "IN THE HAYLOFT" : "HIDDEN", ec); }
    if (P.shieldHP > 0) status("SHIELD " + std::to_string((int)roundf(P.shieldHP / (120 * (1 + P.perks.block * 2)) * 100)) + "%", rgba(200, 170, 120)); if (P.blocking) status("BLOCKING", BONE); if (P.autorun) status("AUTORUN", BONE); if (P.food < 15) status("STARVING", rgba(240, 140, 60)); if (P.water < 15) status("PARCHED", rgba(110, 170, 230)); if (P.crouch) status("CROUCHED", STEEL);
    DrawRectangle(0, H - 4, W, 4, rgba(0, 0, 0, 150)); DrawRectangle(0, H - 4, (int)(W * P.xp / P.next), 4, SODIUM); text("LV " + std::to_string(P.lvl), m, by + 98, 13, SODIUM);
    // weapons
    float rx = W - m; Weapon* g = curGun();
    if (g) { std::string am = std::to_string(g->clip) + " / " + std::to_string(invCount(ammoItem(g->cal))); text(am, rx, by + 30, 36, g->clip == 0 ? rgba(224, 65, 62) : BONE, 2); text(CALN[g->cal], rx, by + 70, 13, STEEL, 2); if (P.reload > 0) bar(rx - 180, by + 90, 180, 4, 1 - P.reload * P.perks.reload / g->rel, BONE); }
    else text(MB[P.melee].n, rx, by + 40, 26, BONE, 2);
    for (int i = 0; i < 3; i++) { float y = by - 34 + i * 20 - 30; bool c = i == P.active; std::string nm = i < 2 ? (P.hasGun[i] ? P.gun[i].name : (i == 0 ? "- primary -" : "- sidearm -")) : MB[P.melee].n; Color cc = i < 2 && P.hasGun[i] ? RARC[P.gun[i].rar] : BONE;
        text(std::string(c ? "> " : "") + std::to_string(i + 1) + "  " + nm, rx, y, c ? 15.f : 13.f, c ? cc : withA(cc, .5f), 2); }
    text("FLARES " + std::to_string(invCount(IT_FLARE)) + "   PIPE BOMBS " + std::to_string(invCount(IT_PIPEBOMB)), rx, by + 106, 12, STEEL, 2);
    // compass strip
    { float cw = 320, cx = W / 2.f - cw / 2, cy = 18; DrawRectangleRec({cx, cy, cw, 22}, rgba(8, 10, 14, 150)); const char* dirs[8] = {"E", "SE", "S", "SW", "W", "NW", "N", "NE"};
      BeginScissorMode((int)cx, (int)cy, (int)cw, 22); for (int i = -8; i < 16; i++) { float a = i * PI / 4; float dx = angDiff(a, P.a); if (fabsf(dx) > 1.2f) continue; text(dirs[((i % 8) + 8) % 8], W / 2.f + dx / 1.2f * cw / 2, cy + 3, 14, i % 2 == 0 ? BONE : STEEL, 1, false); }
      if (horde.on) { float dx = angDiff(horde.a + PI, P.a); if (fabsf(dx) < 1.2f) text("!", W / 2.f + dx / 1.2f * cw / 2, cy + 2, 16, rgba(255, 90, 60), 1, false); }
      EndScissorMode(); DrawLine((int)(W / 2), (int)cy, (int)(W / 2), (int)(cy + 22), SODIUM); }
    if (banner.t > 0) { float a = std::min({1.f, banner.t, (4.5f - banner.t) * 3}); text(banner.a, W / 2.f, H * .17f, std::min(60.f, W * .055f), withA(banner.c, a), 1); text(banner.b, W / 2.f, H * .17f + std::min(60.f, W * .055f) + 6, 17, withA(BONE, a * .9f), 1); }
    // loot labels & prompt
    for (int i = 0; i < (int)loot.size(); i++) { const Loot& l = loot[i]; if (Vector2Distance(V2(l.p.x, l.p.z), P.p) > 7) continue; Vector2 s = GetWorldToScreen(Vector3Add(l.p, V3(0, .4f, 0)), cam3);
        std::string nm = l.kind == LK_GUN ? l.w.name : l.kind == LK_MELEE ? MB[l.id].n : l.kind == LK_CLOTH ? l.c->name : std::string(IDEF[l.id].n) + (l.count > 1 ? " x" + std::to_string(l.count) : "");
        Color c = l.kind == LK_GUN ? RARC[l.w.rar] : l.kind == LK_CLOTH && l.c->rar > 0 ? RARC[l.c->rar] : i == nearLoot ? BONE : rgba(200, 200, 190, 170); float w = textW(nm, 12) + 10;
        DrawRectangleRec({s.x - w / 2, s.y - 9, w, 18}, rgba(8, 10, 14, i == nearLoot ? 220 : 140)); text(nm, s.x, s.y - 7, 12, c, 1, false);
        if (i == nearLoot) text("[E] Take", s.x, s.y + 12, 13, SODIUM, 1); }
    if (nearLoot >= 0 && nearLoot < (int)loot.size() && loot[nearLoot].kind == LK_GUN) {
        const Weapon& nw = loot[nearLoot].w; float pw = 280, h = 90 + nw.aff.size() * 18; Vector2 s = GetWorldToScreen(loot[nearLoot].p, cam3); float px = Clamp(s.x + 30, 8, W - pw - 8), py = Clamp(s.y - 140, 8, H - h - 8);
        DrawRectangleRec({px, py, pw, h}, rgba(10, 12, 16, 235)); DrawRectangleLinesEx({px, py, pw, h}, 1.5f, RARC[nw.rar]);
        text(nw.name, px + 12, py + 10, 15, RARC[nw.rar]); char b[96]; snprintf(b, 96, "%s  ·  %s  ·  %s", RARN[nw.rar], CALN[nw.cal], nw.slot ? "Sidearm" : "Primary"); text(b, px + 12, py + 30, 12, STEEL);
        snprintf(b, 96, "DMG %.0f%s   RPM %d   MAG %d", nw.dmg, nw.pel > 1 ? (" x" + std::to_string(nw.pel)).c_str() : "", (int)(60 / nw.rate), nw.mag); text(b, px + 12, py + 50, 13, BONE);
        float y = py + 70; for (int id : nw.aff) { text(std::string("* ") + AFF[id].desc, px + 12, y, 13, RARC[1]); y += 18; }
    }
    if (nearLoot < 0 && nearAct.kind != A_NONE && state == ST_PLAY) { Vector2 s = GetWorldToScreen(xz3(P.p, 2.3f + charY + (P.loft >= 0 ? lofts[P.loft].y : 0)), cam3); text("[E] " + actLabel(nearAct), s.x, s.y, 14, SODIUM, 1); }
    // crosshair
    if (state == ST_PLAY) { Vector2 mp = GetMousePosition(); float s = 7 + (g ? g->spread * 120 : 4) * (P.sprint ? 1.8f : P.crouch ? .6f : 1) * (1 + P.shiver); Color cc = P.reload > 0 ? rgba(226, 221, 208, 120) : BONE;
        DrawLineEx(V2(mp.x - s - 7, mp.y), V2(mp.x - s, mp.y), 2, cc); DrawLineEx(V2(mp.x + s, mp.y), V2(mp.x + s + 7, mp.y), 2, cc); DrawLineEx(V2(mp.x, mp.y - s - 7), V2(mp.x, mp.y - s), 2, cc); DrawLineEx(V2(mp.x, mp.y + s), V2(mp.x, mp.y + s + 7), 2, cc);
        if (P.reload > 0 && g) DrawRing(mp, s + 11, s + 14, -90, -90 + 360 * (1 - P.reload * P.perks.reload / g->rel), 32, SODIUM); }
    if (muted) text("MUTED", W / 2.f, H - 26.f, 12, STEEL, 1);
}
// ---- DayZ-style inventory: paper doll on the left, pockets in the middle, the ground on the right. Drag stuff around.
struct DragSt { bool on = false; InvItem it; int from = 0; std::shared_ptr<Cloth> owner; int ox = 0, oy = 0; bool orot = false; int slot = -1; Vector3 lp{}; }; // from: 0 pocket, 1 worn slot, 2 ground
static DragSt drag; static struct { bool on = false; InvItem* it = nullptr; Vector2 at{}; } ctxMenu; static float invScroll = 0;
constexpr float CELL = 50;
static void itemColorName(const InvItem& it, Color& c, std::string& n) { if (it.kind == 1) { c = it.c->col; n = it.c->name; } else { c = IDEF[it.id].c; n = IDEF[it.id].n; } }
static void drawItemBox(const InvItem& it, Rectangle r, bool hov) {
    Color c; std::string n; itemColorName(it, c, n); bool set = it.kind == 1 && it.c->set >= 0; DrawRectangleRec(r, hov ? rgba(46, 54, 66, 235) : rgba(26, 30, 38, 235));
    if (set) { float g = .55f + .45f * sinf((float)GetTime() * 3); DrawRectangleLinesEx({r.x - 1, r.y - 1, r.width + 2, r.height + 2}, 2, withA(SETS[it.c->set].c, g)); }
    else DrawRectangleLinesEx(r, 1, it.kind == 1 ? RARC[it.c->rar] : rgba(90, 96, 106));
    DrawRectangle((int)r.x + 3, (int)r.y + 3, (int)r.width - 6, 3, c);
    const char* ic = it.kind == 1 ? clothIcon(it.c->def) : ITEM_ICON[it.id]; if (icon(ic)) drawIcon(ic, r, iconTint(c)); else { std::string sn = n.size() > (size_t)(r.width / 7) ? n.substr(0, (size_t)(r.width / 7)) : n; text(sn, r.x + 4, r.y + 11, 11, BONE); }
    if (it.kind == 1 && it.c->pattern == 2) for (int k = 0; k < 3; k++) DrawCircle((int)(r.x + r.width * (.3f + .2f * k)), (int)(r.y + r.height * .78f), 3, mulC(c, .6f));
    if (it.kind == 1 && !it.c->items.empty()) DrawCircle((int)(r.x + r.width - 7), (int)(r.y + 12), 3, SODIUM);
    if (set) text("SET", r.x + 4, r.y + 7, 10, SETS[it.c->set].c);
    if (it.kind == 0 && it.n > 1) text("x" + std::to_string(it.n), r.x + r.width - 4, r.y + r.height - 15, 12, STEEL, 2);
    if (it.kind == 1) { float cd = it.c->cond; DrawRectangle((int)r.x + 3, (int)(r.y + r.height - 5), (int)((r.width - 6) * cd), 2, cd > .55f ? OKC : cd > .3f ? SODIUM : BLOODC); }
}
static void tooltip(const InvItem& it, Vector2 m) {
    std::vector<std::pair<std::string, Color>> L;
    if (it.kind == 1) { const Cloth& c = *it.c; const CDef& d = CD[c.def]; static const char* RN[4] = {"Common", "Uncommon", "Rare", "Legendary"};
        L.push_back({c.name, RARC[c.rar]}); L.push_back({std::string(RN[c.rar]) + " " + CSN[d.slot] + "   " + CONDN[condTier(c.cond)] + " (" + std::to_string((int)(c.cond * 100)) + "%)", STEEL});
        if (!c.decor.empty()) L.push_back({"\"" + c.decor + "\"", rgba(200, 190, 160)});
        if (c.set >= 0) { const SetDef& S = SETS[c.set]; int n = setCount(c.set); L.push_back({std::string(S.n) + " set  (" + std::to_string(n) + "/4 worn)", S.c});
            L.push_back({std::string("2: ") + S.b2, n >= 2 ? OKC : STEEL}); L.push_back({std::string("4: ") + S.b4, n >= 4 ? OKC : STEEL}); L.push_back({S.bc, n >= 2 && P.cls == c.set ? OKC : STEEL}); }
        char b[96]; snprintf(b, 96, "Warmth %d   Waterproof %d%%   Bite armor %d%%", (int)(clothWarm(c) * 100), (int)(clothWater(c) * 100), (int)(clothArmor(c) * 100)); L.push_back({b, BONE});
        if (gridH(c)) { snprintf(b, 96, "Storage %dx%d (%d slots)%s", gridW(c), gridH(c), gridW(c) * gridH(c), c.items.empty() ? "" : "  - has stuff in it"); L.push_back({b, BONE}); }
        float nz = d.noise - c.bQuiet, vs = d.vis * c.bVis; if (nz > 1.02f) L.push_back({"Noisy", rgba(255, 140, 110)}); else if (nz < .98f) L.push_back({"Quiet", OKC});
        if (vs > 1.02f) L.push_back({"Bright - easy to spot", rgba(255, 140, 110)}); else if (vs < .98f) L.push_back({"Camouflaged", OKC});
        if (c.upg) L.push_back({"Leather patches: " + std::to_string(c.upg) + "/3", rgba(200, 160, 110)}); }
    else { const ItemDef& d = IDEF[it.id]; L.push_back({d.n, BONE}); if (it.id >= IT_A9 && it.id <= IT_A308) { // ammo: caliber, rarity and every weapon that takes it
        int cal = 0; for (int c = 0; c < C_COUNT; c++) if (ammoItem(c) == it.id) cal = c;
        std::string fits; for (int i = 0; i < W_COUNT; i++) if (WB[i].cal == cal) { if (!fits.empty()) fits += ", "; fits += WB[i].n; }
        L.push_back({std::string("Ammo  -  ") + CALN[cal] + "  -  Common", STEEL}); L.push_back({"Fits: " + fits, BONE}); } if (d.food || d.water) { char b[64]; snprintf(b, 64, "Food %+d   Water %+d", (int)d.food, (int)d.water); L.push_back({b, STEEL}); }
        if (it.id == IT_RAG) L.push_back({"Stops bleeding (sometimes). Patches clothes.", STEEL}); else if (it.id == IT_SEWKIT) L.push_back({"Repairs clothing a lot.", STEEL}); else if (it.id == IT_TAPE) L.push_back({"Repairs clothing up to Worn.", STEEL});
        else if (it.id == IT_LEATHER) L.push_back({"Sew onto clothes: +bite armor (3 max).", STEEL}); else if (it.id == IT_WAX) L.push_back({"Waterproofs clothing.", STEEL}); else if (it.id >= IT_DYE_BLACK && it.id <= IT_DYE_BROWN) L.push_back({"Dye clothing. Darker = harder to spot.", STEEL});
        L.push_back({"Right-click: use / options", rgba(150, 156, 166)}); }
    float w = 0; for (auto& l : L) w = std::max(w, textW(l.first, 13)); w += 20; float h = L.size() * 18.f + 12; float x = std::min(m.x + 16, W - w - 6), y = std::min(m.y + 16, H - h - 6);
    DrawRectangleRec({x, y, w, h}, rgba(8, 10, 14, 245)); DrawRectangleLinesEx({x, y, w, h}, 1, rgba(80, 86, 96)); for (int i = 0; i < (int)L.size(); i++) text(L[i].first, x + 10, y + 7 + i * 18.f, 13, L[i].second);
}
static void dropToGround(InvItem it) { Vector2 at = Vector2Add(P.p, polar(frand(0, 6), .5f)); if (it.kind == 1) dropCloth(it.c, at); else spawnItem(xz3(at, .05f), it.id, it.n); }
static void cancelDrag() {
    if (!drag.on) return; drag.on = false; InvItem it = drag.it;
    if (drag.from == 0 && drag.owner) { it.x = drag.ox; it.y = drag.oy; it.rot = drag.orot; drag.owner->items.push_back(it); }
    else if (drag.from == 1 && drag.slot >= 0 && !P.eq[drag.slot]) P.eq[drag.slot] = it.c;
    else if (drag.from == 2) { if (it.kind == 1) spawnCloth(drag.lp, it.c); else spawnItem(drag.lp, it.id, it.n); }
    else if (!placeAnywhere(it)) dropToGround(it); }
static void drawTabs(int cur) { const char* T[2] = {"GEAR  [Tab]", "SKILLS  [K]"}; for (int i = 0; i < 2; i++) { Rectangle r = {W / 2.f - 160 + i * 164, 6, 156, 28}; bool hov = CheckCollisionPointRec(GetMousePosition(), r);
        DrawRectangleRec(r, i == cur ? rgba(40, 46, 58, 245) : hov ? rgba(28, 32, 40, 240) : rgba(14, 16, 22, 230)); DrawRectangleLinesEx(r, 1, i == cur ? SODIUM : rgba(60, 68, 80)); text(T[i], r.x + r.width / 2, r.y + 6, 14, i == cur ? BONE : STEEL, 1);
        if (hov && i != cur && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { if (i == 0) state = ST_INV; else { cancelDrag(); ctxMenu.on = false; state = ST_SKILLS; } } } }
static void gunTooltip(const Weapon& w, Vector2 m, int slot) { // hover card for a gun in the inventory's weapon slots
    std::vector<std::pair<std::string, Color>> L;
    L.push_back({w.name, RARC[w.rar]});
    L.push_back({std::string(RARN[w.rar]) + "  -  " + CALN[w.cal] + "  -  " + (slot == 1 ? "Sidearm" : "Primary"), STEEL});
    char b[96]; snprintf(b, 96, "DMG %.0f%s   RPM %d   MAG %d", w.dmg, w.pel > 1 ? (" x" + std::to_string(w.pel)).c_str() : "", (int)(60 / w.rate), w.mag); L.push_back({b, BONE});
    L.push_back({"Loaded " + std::to_string(w.clip) + "/" + std::to_string(w.mag) + "   Pack: " + std::to_string(invCount(ammoItem(w.cal))) + " " + CALN[w.cal] + " rounds", BONE});
    for (int id : w.aff) L.push_back({std::string("* ") + AFF[id].desc, RARC[1]});
    L.push_back({"Right-click: drop it", rgba(150, 156, 166)});
    float tw = 0; for (auto& l : L) tw = std::max(tw, textW(l.first, 13)); tw += 20; float th = L.size() * 18.f + 12; float x = std::min(m.x + 16, W - tw - 6), y = std::min(m.y + 16, H - th - 6);
    DrawRectangleRec({x, y, tw, th}, rgba(8, 10, 14, 245)); DrawRectangleLinesEx({x, y, tw, th}, 1, RARC[w.rar]); for (int i = 0; i < (int)L.size(); i++) text(L[i].first, x + 10, y + 7 + i * 18.f, 13, L[i].second);
}
static void drawInventory() {
    DrawRectangle(0, 0, W, H, rgba(5, 6, 9, 160)); Vector2 m = GetMousePosition(); bool lp = IsMouseButtonPressed(MOUSE_BUTTON_LEFT), lr = IsMouseButtonReleased(MOUSE_BUTTON_LEFT), rp = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    const InvItem* hover = nullptr; Gear g = gear();
    auto panel = [](float x, float y, float w, float h, const char* t) { DrawRectangleRec({x, y, w, h}, rgba(12, 14, 20, 240)); DrawRectangleLinesEx({x, y, w, h}, 1, rgba(60, 68, 80)); text(t, x + 14, y + 10, 18, BONE); };
    bool ctxHit = false;
    if (ctxMenu.on && (lp || rp)) { /* resolved below in menu draw */ }
    // --- paper doll
    float dx = 20, dy = 40, dw = 300, dh = H - 60.f; panel(dx, dy, dw, dh, "WORN");
    { char b[128]; snprintf(b, 128, "Warmth %d  Rain %d%%  Armor %d%%", (int)(std::min(1.f, g.warm) * 100), (int)(g.water * 100), (int)(std::min(.75f, g.armor) * 100)); text(b, dx + 14, dy + 34, 12, STEEL);
      snprintf(b, 128, "Noise x%.2f  Visibility x%.2f  Wet %d%%", g.noise, g.vis, (int)(P.wet * 100)); text(b, dx + 14, dy + 50, 12, P.wet > .4f ? UVC : STEEL); }
    static const int dollOrder[CS_COUNT] = {CS_HEAD, CS_EYES, CS_FACE, CS_BACK, CS_TORSO, CS_OUTER, CS_HANDS, CS_LEGS, CS_SOCKS, CS_FEET};
    int hoverSlot = -1;
    for (int k = 0; k < CS_COUNT; k++) { int s = dollOrder[k]; Rectangle r = {dx + 14 + (k % 2) * 140.f, dy + 72 + (k / 2) * 62.f, 132, 54}; bool hov = CheckCollisionPointRec(m, r); if (hov) hoverSlot = s;
        bool ok = drag.on && drag.it.kind == 1 && CD[drag.it.c->def].slot == s;
        DrawRectangleRec(r, ok && hov ? rgba(40, 60, 40, 240) : ok ? rgba(26, 40, 28, 240) : rgba(20, 24, 30, 240)); DrawRectangleLinesEx(r, 1, rgba(54, 60, 70)); text(CSN[s], r.x + 4, r.y + 2, 10, rgba(110, 116, 126));
        if (P.eq[s]) { InvItem tmp; tmp.kind = 1; tmp.c = P.eq[s]; drawItemBox(tmp, {r.x + 3, r.y + 14, r.width - 6, r.height - 17}, hov); if (hov && !drag.on) { static InvItem hv; hv = tmp; hover = &hv; }
            if (hov && lp && !ctxMenu.on) { drag = {}; drag.on = true; drag.it = tmp; drag.from = 1; drag.slot = s; P.eq[s].reset(); } } }
    float wy = dy + 72 + 5 * 62.f + 6; text("WEAPONS", dx + 14, wy, 12, STEEL);
    for (int i = 0; i < 3; i++) { Rectangle r = {dx + 14, wy + 18 + i * 40.f, dw - 28, 34}; bool hov = CheckCollisionPointRec(m, r); DrawRectangleRec(r, hov ? rgba(30, 36, 46) : rgba(20, 24, 30)); DrawRectangleLinesEx(r, 1, rgba(54, 60, 70));
        if (i < 2) { text(i == 0 ? "PRIMARY" : "SIDEARM", r.x + 6, r.y + 2, 10, rgba(110, 116, 126)); if (P.hasGun[i]) { if (hov && !drag.on && !ctxMenu.on) gunTooltip(P.gun[i], m, i); text(P.gun[i].name, r.x + 6, r.y + 14, 13, RARC[P.gun[i].rar]); char b[48]; snprintf(b, 48, "%d/%d", P.gun[i].clip, P.gun[i].mag); text(b, r.x + r.width - 6, r.y + 14, 12, STEEL, 2);
                if (hov && rp) { Loot o; o.kind = LK_GUN; o.id = P.gun[i].key; o.w = P.gun[i]; o.p = xz3(P.p, .05f); o.rot = frand(0, 6); loot.push_back(o); P.hasGun[i] = false; if (P.active == i) P.active = 2; msg("Dropped it.", STEEL); } } else text("-", r.x + 6, r.y + 14, 13, STEEL); }
        else { text("MELEE", r.x + 6, r.y + 2, 10, rgba(110, 116, 126)); text(MB[P.melee].n, r.x + 6, r.y + 14, 13, BONE); if (hov && rp && P.melee != MK_FISTS) { Loot o; o.kind = LK_MELEE; o.id = P.melee; o.p = xz3(P.p, .05f); o.rot = frand(0, 6); loot.push_back(o); P.melee = MK_FISTS; } } }
    text("Right-click a weapon to drop it", dx + 14, wy + 140, 11, rgba(110, 116, 126));
    { float sy = wy + 162; text("SETS", dx + 14, sy, 12, STEEL); sy += 16; bool any = false;
      for (int i = 0; i < 5; i++) { int n = setCount(i); if (!n) continue; any = true; const SetDef& S = SETS[i]; text(std::string(S.n) + "  " + std::to_string(n) + "/4" + (P.cls == i ? "  (your class)" : ""), dx + 14, sy, 13, S.c); sy += 15;
          text(n >= 2 ? (n >= 4 ? S.b4 : S.b2) : "Wear 2 for a bonus", dx + 22, sy, 11, n >= 2 ? OKC : STEEL); sy += 15; if (sy > dy + dh - 20) break; }
      if (!any) text("Wear 2+ pieces of a class set for bonuses", dx + 14, sy, 11, rgba(110, 116, 126)); }
    // --- pockets
    float cx = dx + dw + 14, cy = 40, cw = CELL * 6 + 40; panel(cx, cy, cw, H - 60.f, "POCKETS"); text(std::to_string(g.used) + " / " + std::to_string(g.slots), cx + cw - 14, cy + 14, 12, STEEL, 2);
    struct Hit { std::shared_ptr<Cloth> c; int gx, gy; bool valid; int mx, my; } hit{nullptr, 0, 0, false, 0, 0};
    float yy = cy + 40 - invScroll; Rectangle clip = {cx, cy + 36, cw, H - 100.f};
    BeginScissorMode((int)clip.x, (int)clip.y, (int)clip.width, (int)clip.height);
    bool anyCont = false;
    for (int s : CONT_ORDER) { auto c = P.eq[s]; if (!c || !gridH(*c)) continue; anyCont = true; text(c->name, cx + 14, yy, 13, RARC[c->rar]); yy += 18; float gx0 = cx + 14, gy0 = yy;
        for (int y = 0; y < gridH(*c); y++) for (int x = 0; x < gridW(*c); x++) { DrawRectangle((int)(gx0 + x * CELL), (int)(gy0 + y * CELL), (int)CELL - 2, (int)CELL - 2, rgba(22, 26, 32, 240)); }
        Rectangle gr = {gx0, gy0, gridW(*c) * CELL, gridH(*c) * CELL};
        if (drag.on && CheckCollisionPointRec(m, gr) && CheckCollisionPointRec(m, clip)) { int w, h; itemDims(drag.it, w, h); int gx = (int)floorf((m.x - gx0) / CELL - w / 2.f + .5f), gy = (int)floorf((m.y - gy0) / CELL - h / 2.f + .5f);
            bool v = fits(*c, gx, gy, w, h) && !(drag.it.kind == 1 && drag.it.c == c); hit = {c, gx, gy, v, (int)((m.x - gx0) / CELL), (int)((m.y - gy0) / CELL)};
            DrawRectangle((int)(gx0 + gx * CELL), (int)(gy0 + gy * CELL), (int)(w * CELL - 2), (int)(h * CELL - 2), v ? rgba(60, 140, 70, 110) : rgba(160, 50, 40, 110)); }
        for (int k = 0; k < (int)c->items.size(); k++) { InvItem& it = c->items[k]; int w, h; itemDims(it, w, h); Rectangle r = {gx0 + it.x * CELL, gy0 + it.y * CELL, w * CELL - 2, h * CELL - 2};
            bool hov = !drag.on && !ctxMenu.on && CheckCollisionPointRec(m, r) && CheckCollisionPointRec(m, clip); drawItemBox(it, r, hov);
            if (hov) { hover = &it;
                if (lp) { if (IsKeyDown(KEY_LEFT_SHIFT) && it.kind == 0) { InvItem cp = it; c->items.erase(c->items.begin() + k); dropToGround(cp); sfx(SFX_PICK); break; }
                    drag = {}; drag.on = true; drag.it = it; drag.from = 0; drag.owner = c; drag.ox = it.x; drag.oy = it.y; drag.orot = it.rot; c->items.erase(c->items.begin() + k); hover = nullptr; break; }
                if (rp) { ctxMenu.on = true; ctxMenu.it = &it; ctxMenu.at = m; ctxHit = true; } } }
        yy += gridH(*c) * CELL + 12; }
    EndScissorMode();
    if (!anyCont) text("No pockets. Find pants or a shirt.", cx + 14, cy + 44, 13, STEEL);
    float contentH = yy + invScroll - (cy + 40); if (CheckCollisionPointRec(m, clip)) invScroll = Clamp(invScroll - GetMouseWheelMove() * 40, 0, std::max(0.f, contentH - clip.height + 20));
    // --- ground + craft
    float vx = cx + cw + 14, vw = std::min(360.f, W - vx - 20), vh = (H - 74.f) * .55f; panel(vx, 40, vw, vh, "GROUND"); Rectangle vrect = {vx, 40, vw, vh};
    std::vector<int> near; for (int i = 0; i < (int)loot.size(); i++) if (loot[i].settled && Vector2Distance(V2(loot[i].p.x, loot[i].p.z), P.p) < 2.6f) near.push_back(i);
    if (drag.on && CheckCollisionPointRec(m, vrect)) DrawRectangleLinesEx(vrect, 2, rgba(200, 160, 80));
    { float y = 70; int shown = 0; for (int li : near) { if (y > 40 + vh - 30) break; Loot& l = loot[li]; Rectangle r = {vx + 10, y, vw - 20, 28}; bool hov = !drag.on && !ctxMenu.on && CheckCollisionPointRec(m, r);
        std::string nm = l.kind == LK_GUN ? l.w.name : l.kind == LK_MELEE ? MB[l.id].n : l.kind == LK_CLOTH ? l.c->name + "  (" + CONDN[condTier(l.c->cond)] + ")" : std::string(IDEF[l.id].n) + (l.count > 1 ? " x" + std::to_string(l.count) : "");
        Color cc = l.kind == LK_GUN ? RARC[l.w.rar] : l.kind == LK_CLOTH ? RARC[l.c->rar] : BONE; DrawRectangleRec(r, hov ? rgba(36, 42, 52) : rgba(20, 24, 30)); DrawRectangle((int)r.x, (int)r.y, 4, 28, l.kind == LK_CLOTH ? l.c->col : l.kind == LK_ITEM ? IDEF[l.id].c : cc);
        { const char* ic = l.kind == LK_CLOTH ? clothIcon(l.c->def) : l.kind == LK_ITEM ? ITEM_ICON[l.id] : l.kind == LK_MELEE ? MELEE_ICON[l.id] : GUN_ICON[l.w.key]; drawIcon(ic, {r.x + 6, r.y, 28, 28}, iconTint(l.kind == LK_CLOTH ? l.c->col : l.kind == LK_ITEM ? IDEF[l.id].c : cc), .08f); }
        if (l.kind == LK_CLOTH && l.c->set >= 0) cc = SETS[l.c->set].c; text(nm, r.x + 40, r.y + 7, 13, cc);
        if (hov && l.kind == LK_CLOTH) { static InvItem hv; hv.kind = 1; hv.c = l.c; hover = &hv; } if (hov && l.kind == LK_ITEM) { static InvItem hv2; hv2.kind = 0; hv2.id = l.id; hv2.n = l.count; hover = &hv2; }
        if (hov && lp) { if (l.kind == LK_GUN || l.kind == LK_MELEE) { nearLoot = li; pickUp(); break; }
            drag = {}; drag.on = true; drag.from = 2; drag.lp = l.p; drag.it.kind = l.kind == LK_CLOTH ? 1 : 0; drag.it.id = l.id; drag.it.n = l.count; drag.it.c = l.c; loot.erase(loot.begin() + li); break; }
        if (hov && rp) { nearLoot = li; pickUp(); break; } y += 32; shown++; }
      if (near.empty()) text("Nothing here.", vx + 14, 72, 13, STEEL); }
    { float ky = 40 + vh + 10, kh = H - 20 - ky; panel(vx, ky, vw, kh, "CRAFT");
      bool rag = invCount(IT_BANDAGE) + invCount(IT_RAG) >= 1;
      struct R { const char* n; const char* need; bool ok; int what; } rs[5] = {
          {"Splint", "2 kindling + bandage/rag", invCount(IT_KINDLING) >= 2 && rag, 0}, {"Campfire", "3 kindling + 1 match, outdoors", invCount(IT_KINDLING) >= 3 && invCount(IT_MATCHES) >= 1, 1},
          {"Split a log", "1 log, axe/hatchet in hand", invCount(IT_LOG) >= 1 && canChop(P.melee), 2}, {P.shieldHP > 0 ? "Repair shield" : "Plank shield", "1 log + 3 kindling + rope or tape", invCount(IT_LOG) >= 1 && invCount(IT_KINDLING) >= 3 && (invCount(IT_ROPE) || invCount(IT_TAPE)), 3},
          {"Tear clothes into rags", "Drag a shirt here: 2-4 rags", false, 4}};
      Rectangle kr = {vx, ky, vw, kh}; if (drag.on && drag.it.kind == 1 && CheckCollisionPointRec(m, kr)) DrawRectangleLinesEx(kr, 2, OKC);
      for (int i = 0; i < 5; i++) { Rectangle r = {vx + 10, ky + 36 + i * 40.f, vw - 20, 36}; if (r.y + r.height > ky + kh) break; bool hov = !drag.on && CheckCollisionPointRec(m, r);
          DrawRectangleRec(r, hov && rs[i].ok ? rgba(30, 36, 46) : rgba(18, 22, 30)); DrawRectangle((int)r.x, (int)r.y, 4, 36, rs[i].ok ? OKC : rgba(70, 74, 80)); text(rs[i].n, r.x + 12, r.y + 3, 14, rs[i].ok ? BONE : STEEL); text(rs[i].need, r.x + 12, r.y + 20, 11, STEEL);
          if (hov && rs[i].ok && lp) { if (rs[i].what == 0) craftSplint(); else if (rs[i].what == 1) { buildFire(); state = ST_PLAY; }
              else if (rs[i].what == 3) { invTake(IT_LOG, 1); invTake(IT_KINDLING, 3); if (!invTake(IT_ROPE, 1)) invTake(IT_TAPE, 1); P.shieldHP = 120 * (1 + P.perks.block * 2); msg("Lashed planks into a shield. Hold right-click to block.", OKC); sfx(SFX_HIT_WOOD); }
              else if (rs[i].what == 2) useItem(invFind(IT_LOG)); } }
      // drop a piece of clothing on the craft panel to tear it into rags
      if (drag.on && lr && drag.it.kind == 1 && CheckCollisionPointRec(m, kr)) { auto c = drag.it.c; for (auto& in : c->items) if (!placeAnywhere(in)) dropToGround(in); int n = CD[c->def].slot == CS_OUTER || CD[c->def].slot == CS_TORSO || CD[c->def].slot == CS_LEGS ? irand(2, 4) : 1;
          int left = invAdd(IT_RAG, n); if (left) spawnItem(xz3(P.p, .05f), IT_RAG, left); msg("Tore the " + c->name + " into " + std::to_string(n) + " rags.", OKC); sfx(SFX_PICK); drag.on = false; } }
    // --- context menu
    if (ctxMenu.on && !ctxHit) {
        InvItem* it = ctxMenu.it; bool alive = false; forInv([&](InvItem& x) { if (&x == it) alive = true; });
        if (!alive) ctxMenu.on = false;
        else { struct O { std::string t; int act; std::shared_ptr<Cloth> tg; }; std::vector<O> os;
            if (it->kind == 0) { if (IDEF[it->id].use[0] && (IDEF[it->id].food || IDEF[it->id].water || it->id <= IT_SPLINT || it->id == IT_RAG)) os.push_back({IDEF[it->id].use, 0, nullptr});
                for (int s = 0; s < CS_COUNT; s++) if (P.eq[s] && toolFits(it->id, *P.eq[s])) os.push_back({std::string(IDEF[it->id].use[0] ? IDEF[it->id].use : "Use on") + ": " + P.eq[s]->name, 1, P.eq[s]}); }
            else { int s = CD[it->c->def].slot; os.push_back({P.eq[s] ? "Swap with worn " + P.eq[s]->name : std::string("Wear"), 3, nullptr}); }
            os.push_back({"Drop", 2, nullptr});
            float w = 0; for (auto& o : os) w = std::max(w, textW(o.t, 13)); w += 24; float x = std::min(ctxMenu.at.x, W - w - 4), y = std::min(ctxMenu.at.y, H - os.size() * 24.f - 8);
            DrawRectangleRec({x, y, w, os.size() * 24.f + 6}, rgba(8, 10, 14, 250)); DrawRectangleLinesEx({x, y, w, os.size() * 24.f + 6}, 1, rgba(90, 96, 106));
            int pickO = -1; for (int i = 0; i < (int)os.size(); i++) { Rectangle r = {x + 2, y + 3 + i * 24.f, w - 4, 22}; bool hov = CheckCollisionPointRec(m, r); if (hov) DrawRectangleRec(r, rgba(40, 48, 60)); text(os[i].t, r.x + 8, r.y + 4, 13, hov ? BONE : STEEL); if (hov && lp) pickO = i; }
            if (pickO >= 0) { O o = os[pickO]; ctxMenu.on = false;
                if (o.act == 0) useItem(it); else if (o.act == 1) applyTool(it, *o.tg);
                else if (o.act == 2) { InvItem cp = *it; for (int s : CONT_ORDER) if (P.eq[s]) for (int k = 0; k < (int)P.eq[s]->items.size(); k++) if (&P.eq[s]->items[k] == it) { P.eq[s]->items.erase(P.eq[s]->items.begin() + k); goto dropped; } dropped: dropToGround(cp); }
                else if (o.act == 3) { InvItem cp = *it; int s = CD[cp.c->def].slot; for (int q : CONT_ORDER) if (P.eq[q]) for (int k = 0; k < (int)P.eq[q]->items.size(); k++) if (&P.eq[q]->items[k] == it) { P.eq[q]->items.erase(P.eq[q]->items.begin() + k); goto removed; }
                    removed: { auto old = P.eq[s]; P.eq[s] = cp.c; if (old) { InvItem oi; oi.kind = 1; oi.c = old; if (!placeAnywhere(oi)) dropCloth(old, P.p); } msg("Put on " + cp.c->name, RARC[cp.c->rar]); sfx(SFX_PICK); } } }
            else if (lp || rp) ctxMenu.on = false; } }
    drawHotbar(true);
    // --- drag & drop
    if (drag.on) { if (IsKeyPressed(KEY_R)) drag.it.rot = !drag.it.rot; int w, h; itemDims(drag.it, w, h); drawItemBox(drag.it, {m.x - w * CELL / 2, m.y - h * CELL / 2, w * CELL - 2, h * CELL - 2}, true);
        if (lr) { bool done = false;
            for (int i = 0; i < 10 && !done; i++) if (CheckCollisionPointRec(m, hotRect(i)) && drag.it.kind == 0) { for (auto& h : hotbar) if (h.kind == 1 && h.id == drag.it.id) h = {}; hotbar[i] = {1, drag.it.id}; cancelDrag(); drag.on = false; done = true; sfx(SFX_PICK, .3f); }
            if (done) {} else
            if (hit.c && hit.valid) { InvItem it = drag.it; it.x = hit.gx; it.y = hit.gy;
                if (it.kind == 0) for (auto& o : hit.c->items) { int ow, oh; itemDims(o, ow, oh); if (o.kind == 0 && o.id == it.id && o.n < IDEF[it.id].stack && hit.gx >= o.x && hit.gx < o.x + ow && hit.gy >= o.y && hit.gy < o.y + oh) { int k = std::min(it.n, IDEF[it.id].stack - o.n); o.n += k; it.n -= k; } }
                if (it.n > 0) hit.c->items.push_back(it); done = true; sfx(SFX_PICK, .4f); }
            else if (hit.c && drag.it.kind == 0) { for (auto& o : hit.c->items) { int ow, oh; itemDims(o, ow, oh); // drop onto a matching stack to merge
                    if (o.kind == 0 && o.id == drag.it.id && o.n < IDEF[o.id].stack && hit.mx >= o.x && hit.mx < o.x + ow && hit.my >= o.y && hit.my < o.y + oh) { int k = std::min(drag.it.n, IDEF[o.id].stack - o.n); o.n += k; drag.it.n -= k; if (drag.it.n <= 0) done = true; break; } } }
            if (!done && hoverSlot >= 0 && drag.it.kind == 1 && CD[drag.it.c->def].slot == hoverSlot) { auto old = P.eq[hoverSlot]; P.eq[hoverSlot] = drag.it.c; if (old) { InvItem oi; oi.kind = 1; oi.c = old; if (!placeAnywhere(oi)) dropCloth(old, P.p); } done = true; sfx(SFX_PICK); msg("Put on " + drag.it.c->name, RARC[drag.it.c->rar]); }
            if (!done && hoverSlot >= 0 && drag.it.kind == 0 && P.eq[hoverSlot] && toolFits(drag.it.id, *P.eq[hoverSlot])) { InvItem t = drag.it; applyTool(&t, *P.eq[hoverSlot]); if (t.n > 0) { drag.it = t; cancelDrag(); } done = true; }
            if (!done && (CheckCollisionPointRec(m, vrect) || m.x > vx + vw || (!hit.c && hoverSlot < 0 && m.x > cx + cw))) { dropToGround(drag.it); done = true; sfx(SFX_PICK, .3f); }
            if (done) drag.on = false; else cancelDrag(); } }
    else if (hover && !ctxMenu.on) { tooltip(*hover, m); if (lp && hover->kind == 0) {} }
    drawTabs(0);
    text("Drag to move   R rotate   Drop on a slot to wear   Drag a tool onto clothes to repair   Shift-click to drop   Right-click for options", W / 2.f, H - 16.f, 11, rgba(120, 126, 136), 1);
    drawCursor();
}
static void drawMap() {
    static float zoom = 1; static Vector2 vc{MAP / 2, MAP / 2}; static bool dragging = false; static Vector2 dragM{};
    dim(.75f); float sz = std::min(W - 80.f, H - 100.f), fx = W / 2.f - sz / 2, fy = H / 2.f - sz / 2 + 10; Vector2 m = GetMousePosition(); Rectangle fr = {fx, fy, sz, sz};
    Vector2 me = P.p; { int cb = cellOf(P.p); if (cb >= 0 && blds[cb].parent >= 0) { const Building& pb = blds[blds[cb].parent]; me = V2((pb.x0 + pb.x1) / 2, (pb.z0 + pb.z1) / 2); } }
    float base = sz / MAP; auto toW = [&](Vector2 s2) { float sc = base * zoom; return V2(vc.x + (s2.x - (fx + sz / 2)) / sc, vc.y + (s2.y - (fy + sz / 2)) / sc); };
    float wh = GetMouseWheelMove(); if (wh != 0 && CheckCollisionPointRec(m, fr)) { Vector2 before = toW(m); zoom = Clamp(zoom * (wh > 0 ? 1.25f : .8f), 1, 8); Vector2 after = toW(m); vc = Vector2Add(vc, Vector2Subtract(before, after)); }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, fr)) { dragging = true; dragM = m; } if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) dragging = false;
    if (dragging) { vc = Vector2Subtract(vc, Vector2Scale(Vector2Subtract(m, dragM), 1 / (base * zoom))); dragM = m; }
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_C)) { vc = me; if (zoom < 2.5f) zoom = 2.5f; } if (IsKeyPressed(KEY_HOME)) { zoom = 1; vc = V2(MAP / 2, MAP / 2); }
    float half = MAP / 2 / zoom; vc.x = Clamp(vc.x, half, MAP - half); vc.y = Clamp(vc.y, half, MAP - half);
    float sc = base * zoom, mx = fx + sz / 2 - vc.x * sc, my = fy + sz / 2 - vc.y * sc;
    BeginScissorMode((int)fx, (int)fy, (int)sz, (int)sz);
    DrawTexturePro(mapTex, {0, 0, (float)mapTex.width, (float)mapTex.height}, {mx, my, MAP * sc, MAP * sc}, V2(0, 0), 0, rgba(230, 225, 210));
    if (zoom >= 2.5f) for (auto& b : blds) { if (b.cellKind) continue; Rectangle r = {mx + b.x0 * sc, my + b.z0 * sc, (b.x1 - b.x0) * sc, (b.z1 - b.z0) * sc}; if (r.x > fx + sz || r.y > fy + sz || r.x + r.width < fx || r.y + r.height < fy) continue; DrawRectangleLinesEx(r, 1, rgba(40, 34, 28, 200)); }
    for (auto& h : hotZones) { Vector2 hp = V2(mx + h.c.x * sc, my + h.c.y * sc); DrawCircleV(hp, h.r * sc * 1.15f, rgba(200, 40, 30, 70)); DrawRing(hp, h.r * sc * 1.15f - 1.5f, h.r * sc * 1.15f, 0, 360, 32, rgba(200, 40, 30, 200));
        text(HZN[h.kind], hp.x, hp.y + h.r * sc + 4, 13, rgba(255, 130, 110), 1, true); }
    { Vector2 hc{}; int hn = 0; for (auto& z : zombies) if (z.hordeM && !z.dead && !z.gone) { hc = Vector2Add(hc, z.p); hn++; } bool hv = hn > 0; if (hv) hc = Vector2Scale(hc, 1.f / hn); else if (horde.on) { hc = horde.end; hv = true; }
      if (hv && Vector2Distance(hc, me) < 150) { // only shown when the horde is within 150 m of you
        Vector2 hp = V2(mx + hc.x * sc, my + hc.y * sc); bool off = !CheckCollisionPointRec(hp, fr); hp.x = Clamp(hp.x, fx + 14, fx + sz - 14); hp.y = Clamp(hp.y, fy + 14, fy + sz - 14);
        float pulse = 7 + 2 * sinf((float)GetTime() * 5); DrawCircleV(hp, pulse, rgba(230, 40, 30, off ? 150 : 230)); DrawRing(hp, pulse + 2, pulse + 4, 0, 360, 24, rgba(230, 40, 30, 120));
        if (horde.on) { Vector2 dv = Vector2Subtract(horde.end, hc); float dl = Vector2Length(dv); if (dl > 1) DrawLineEx(hp, Vector2Add(hp, Vector2Scale(Vector2Scale(dv, 1 / dl), 22)), 3, rgba(230, 40, 30, 220)); }
        text(off ? "HORDE (edge)" : "HORDE", hp.x, hp.y + 12, 13, rgba(255, 130, 110), 1, true); } }
    for (int i = 0; i < 6; i++) { const Town& T = TOWNS[i]; text(T.name, mx + T.c.x * sc, my + T.c.y * sc - T.r * sc - 18, 16, T.kind == TK_MILITARY ? rgba(220, 70, 60) : rgba(30, 26, 20), 1, false); }
    Vector2 pp = V2(mx + me.x * sc, my + me.y * sc); DrawCircleV(pp, 6, rgba(220, 40, 40)); DrawLineEx(pp, Vector2Add(pp, polar(P.a, 14)), 2, rgba(220, 40, 40));
    EndScissorMode(); DrawRectangleLinesEx(fr, 2, rgba(90, 80, 60));
    text("MAP   [M] close   Wheel zoom   Drag to pan   Space: find me   Home: whole county", fx, fy - 30, 16, BONE); drawCursor();
}
// ============================================================ save & quit (one slot; dying deletes it)
static std::string runSavePath() { const char* h = getenv("HOME"); return std::string(h ? h : ".") + "/.nightfall3d_save"; }
static bool hasSave() { return FileExists(runSavePath().c_str()); }
static void deleteSave() { std::remove(runSavePath().c_str()); }
static void wCloth(std::ostream& o, const Cloth& c);
static void wItem(std::ostream& o, const InvItem& it) { o << it.kind << ' ' << it.id << ' ' << it.n << ' ' << it.x << ' ' << it.y << ' ' << it.rot << '\n'; if (it.kind == 1) wCloth(o, *it.c); }
static void wCloth(std::ostream& o, const Cloth& c) {
    o << c.def << ' ' << c.cond << ' ' << (int)c.col.r << ' ' << (int)c.col.g << ' ' << (int)c.col.b << ' ' << c.pattern << ' ' << c.rar << ' ' << c.upg << ' ' << c.extraRows << ' ' << c.set << ' '
      << c.bWarm << ' ' << c.bWater << ' ' << c.bArmor << ' ' << c.bQuiet << ' ' << c.bVis << ' ' << std::quoted(c.name) << ' ' << std::quoted(c.decor) << ' ' << c.items.size() << '\n';
    for (auto& it : c.items) wItem(o, it); }
static std::shared_ptr<Cloth> rCloth(std::istream& in);
static InvItem rItem(std::istream& in) { InvItem it; in >> it.kind >> it.id >> it.n >> it.x >> it.y >> it.rot; if (it.kind == 1) it.c = rCloth(in); return it; }
static std::shared_ptr<Cloth> rCloth(std::istream& in) { auto c = std::make_shared<Cloth>(); int r, g, b; size_t n;
    in >> c->def >> c->cond >> r >> g >> b >> c->pattern >> c->rar >> c->upg >> c->extraRows >> c->set >> c->bWarm >> c->bWater >> c->bArmor >> c->bQuiet >> c->bVis >> std::quoted(c->name) >> std::quoted(c->decor) >> n;
    c->col = rgba(r, g, b); c->def = std::max(0, std::min(CD_N - 1, c->def)); for (size_t i = 0; i < n && in; i++) c->items.push_back(rItem(in)); return c; }
static void wGun(std::ostream& o, const Weapon& w) { o << w.key << ' ' << w.rar << ' ' << w.cal << ' ' << w.slot << ' ' << w.dmg << ' ' << w.rate << ' ' << w.spread << ' ' << w.spd << ' ' << w.range << ' ' << w.rel << ' ' << w.kick << ' ' << w.len << ' ' << w.dism << ' ' << w.crit << ' ' << w.leech << ' ' << w.noise << ' '
    << w.pel << ' ' << w.mag << ' ' << w.snd << ' ' << w.pierce << ' ' << w.clip << ' ' << w.autoF << ' ' << w.burn << ' ' << w.aff.size(); for (int a : w.aff) o << ' ' << a; o << ' ' << std::quoted(w.name) << '\n'; }
static Weapon rGun(std::istream& in) { Weapon w; size_t n; in >> w.key >> w.rar >> w.cal >> w.slot >> w.dmg >> w.rate >> w.spread >> w.spd >> w.range >> w.rel >> w.kick >> w.len >> w.dism >> w.crit >> w.leech >> w.noise >> w.pel >> w.mag >> w.snd >> w.pierce >> w.clip >> w.autoF >> w.burn >> n;
    for (size_t i = 0; i < n && in; i++) { int a; in >> a; w.aff.push_back(a); } in >> std::quoted(w.name); return w; }
static void saveGame() {
    std::ofstream o(runSavePath()); if (!o) { msg("Couldn't write the save file.", BLOODC); return; }
    o << "NFSAVE 1\n" << worldSeed << ' ' << cfg.foliage << ' ' << cfg.diff << '\n';
    o << gTime << ' ' << nights << ' ' << kills << ' ' << hordeDay << ' ' << playerFemale << ' ' << gibCount << '\n';
    o << P.p.x << ' ' << P.p.y << ' ' << P.a << ' ' << P.hp << ' ' << P.food << ' ' << P.water << ' ' << P.temp << ' ' << P.sta << ' ' << P.bleeds << ' ' << P.sick << ' ' << P.pain << ' ' << P.broken << ' ' << P.splinted << ' ' << P.splintT << ' '
      << P.wet << ' ' << P.batt << ' ' << P.xp << ' ' << P.next << ' ' << P.lvl << ' ' << P.melee << ' ' << P.active << ' ' << P.cls << ' ' << P.points << ' ' << P.actCd << ' ' << P.shieldHP << ' ' << P.walked << ' ' << P.alive << '\n';
    for (int t = 0; t < 3; t++) for (int i = 0; i < 4; i++) o << P.rank[t][i] << ' '; o << '\n';
    for (int g = 0; g < 2; g++) { o << P.hasGun[g] << '\n'; if (P.hasGun[g]) wGun(o, P.gun[g]); }
    for (int s = 0; s < CS_COUNT; s++) { o << (P.eq[s] ? 1 : 0) << '\n'; if (P.eq[s]) wCloth(o, *P.eq[s]); }
    for (auto& h : hotbar) o << h.kind << ' ' << h.id << ' '; o << '\n';
    o << loot.size() << '\n'; for (auto& l : loot) { o << l.kind << ' ' << l.id << ' ' << l.count << ' ' << l.p.x << ' ' << l.p.y << ' ' << l.p.z << ' ' << l.rot << '\n'; if (l.kind == LK_GUN) wGun(o, l.w); if (l.kind == LK_CLOTH) wCloth(o, *l.c); }
    o << horde.on << ' ' << horde.a << ' ' << horde.t << ' ' << horde.left << ' ' << horde.end.x << ' ' << horde.end.y << ' ' << horde.lastSeen.x << ' ' << horde.lastSeen.y << ' ' << hordeDay << '\n';
    o << doorObjs.size() << '\n'; for (auto& d : doorObjs) o << d.open << ' ' << d.broken << ' ' << d.hp << ' '; o << '\n';
    o << blds.size() << '\n'; for (auto& b : blds) o << b.visited << ' '; o << '\n';
    o << carObjs.size() << '\n'; for (auto& c : carObjs) o << c.searched << ' '; o << '\n';
    o << "END\n";
}
static bool loadGame() {
    std::ifstream in(runSavePath()); std::string tag; int ver; if (!(in >> tag >> ver) || tag != "NFSAVE") return false;
    unsigned seed; int fol, diff; in >> seed >> fol >> diff; int keepF = cfg.foliage; cfg.foliage = fol; newGame(seed); cfg.foliage = keepF; cfg.diff = diff;
    in >> gTime >> nights >> kills >> hordeDay >> playerFemale >> gibCount;
    in >> P.p.x >> P.p.y >> P.a >> P.hp >> P.food >> P.water >> P.temp >> P.sta >> P.bleeds >> P.sick >> P.pain >> P.broken >> P.splinted >> P.splintT >> P.wet >> P.batt >> P.xp >> P.next >> P.lvl >> P.melee >> P.active >> P.cls >> P.points >> P.actCd >> P.shieldHP >> P.walked >> P.alive;
    for (int t = 0; t < 3; t++) for (int i = 0; i < 4; i++) in >> P.rank[t][i];
    for (int g = 0; g < 2; g++) { in >> P.hasGun[g]; if (P.hasGun[g]) P.gun[g] = rGun(in); }
    for (int s = 0; s < CS_COUNT; s++) { int has; in >> has; P.eq[s] = has ? rCloth(in) : nullptr; }
    for (auto& h : hotbar) in >> h.kind >> h.id;
    size_t n; in >> n; loot.clear(); for (size_t i = 0; i < n && in; i++) { Loot l; in >> l.kind >> l.id >> l.count >> l.p.x >> l.p.y >> l.p.z >> l.rot; l.settled = true; if (l.kind == LK_GUN) l.w = rGun(in); if (l.kind == LK_CLOTH) l.c = rCloth(in); loot.push_back(l); }
    { int on = 0; in >> on >> horde.a >> horde.t >> horde.left >> horde.end.x >> horde.end.y >> horde.lastSeen.x >> horde.lastSeen.y >> hordeDay; horde.on = on != 0; }
    in >> n; for (size_t i = 0; i < n && in; i++) { DoorObj tmp; in >> tmp.open >> tmp.broken >> tmp.hp; if (i < doorObjs.size()) { doorObjs[i].open = tmp.open; doorObjs[i].broken = tmp.broken; doorObjs[i].hp = tmp.hp; doorObjs[i].anim = tmp.open || tmp.broken ? 1 : 0; } }
    in >> n; for (size_t i = 0; i < n && in; i++) { bool v; in >> v; if (i < blds.size()) blds[i].visited = v; }
    in >> n; for (size_t i = 0; i < n && in; i++) { bool v; in >> v; if (i < carObjs.size()) carObjs[i].searched = v; }
    in >> tag; if (tag != "END") { msg("Save file looked damaged; some things may be off.", SODIUM); }
    zombies.clear(); camPos = P.p; phase = isNight() ? 'n' : 'd'; recalcPerks(); msgs.clear(); msg("Picked up where you left off.", OKC); state = ST_PLAY; return true;
}
static State settingsBack = ST_PAUSE;
static void drawTitle() {
    dim(.45f); float x = std::max(40.f, W / 2.f - 380), y = H * .14f, big = std::min(104.f, W * .09f);
    text(std::string("LINN-BENTON QUARANTINE ZONE  -  NO RADIO CONTACT SINCE DAY 9      ") + VERSION, x, y, 14, STEEL); y += 28;
    text("NIGHTFALL", x, y, big, SODIUM); y += big - 8; text("COUNTY", x, y, big, BLOODC); y += big + 12;
    text("Cedar Falls, Harlow, Millbrook, Pine Ridge, Dunmore, Fort Calloway. Same towns every life, never in the same place.", x, y, 16, rgba(199, 194, 182)); y += 22;
    text("Each life rolls a class, scatters your skill points for you, and redraws the roads, loot and hot zones.", x, y, 16, rgba(199, 194, 182)); y += 36;
    const char* keys[][2] = {{"WASD", "Move"}, {"Mouse / LMB", "Aim / shoot or swing (V melee)"}, {"Shift / C", "Sprint / crouch"}, {"Space", "Dodge roll"}, {"RMB / Q", "Block / kick"}, {"R", "Reload"},
        {"E", "Pick up / use"}, {"1-0 / Ctrl+Wheel", "Hotbar / weapons (wheel zooms)"}, {"Tab", "Backpack (eat, drink, bandage)"}, {"F / U", "Flashlight / UV mode"}, {"G / H", "Pipe bomb / road flare"}, {"M", "Map"}, {"Z / K", "Class skill / skill trees"}, {"B / N", "Bandage / best meds"}, {"X", "Autorun"}, {"P / Esc", "Pause"}};
    for (int i = 0; i < 16; i++) { float cx = x + (i % 2) * 380, cy = y + (i / 2) * 26; DrawRectangleRec({cx, cy - 2, 110, 22}, rgba(18, 22, 30)); DrawRectangleLinesEx({cx, cy - 2, 110, 22}, 1, rgba(60, 68, 80)); text(keys[i][0], cx + 55, cy, 13, BONE, 1, false); text(keys[i][1], cx + 122, cy, 15, rgba(199, 194, 182)); }
    y += 8 * 26 + 22;
    if (hasSave()) { if (button("CONTINUE", x, y, 200, 50, true) || IsKeyPressed(KEY_ENTER)) { if (!loadGame()) { deleteSave(); newGame(); } } y += 60;
        if (button("NEW LIFE", x, y, 200, 40, false)) { deleteSave(); newGame(); } }
    else if (button("HEAD OUT", x, y, 200, 50, true) || IsKeyPressed(KEY_ENTER)) newGame();
    if (button("SETTINGS", x, y + 60, 200, 40, false)) { settingsBack = ST_TITLE; state = ST_SETTINGS; }
    if (bestHours > 0) text("Longest survival: " + std::to_string(bestHours) + " hours, " + std::to_string(bestKills) + " kills", x + 220, y + 16, 15, STEEL);
    drawCursor();
}
static void wrapText(const std::string& d, float x, float y, float w, float size, Color c) { std::string word, acc; for (size_t k = 0; k <= d.size(); k++) { if (k == d.size() || d[k] == ' ') { std::string t = acc.empty() ? word : acc + " " + word; if (textW(t, size) > w && !acc.empty()) { text(acc, x, y, size, c); y += size + 4; acc = word; } else acc = t; word.clear(); } else word += d[k]; } if (!acc.empty()) text(acc, x, y, size, c); }
static void drawSkills() {
    dim(.72f); const ClassDef& C = CL(); float pw = std::min(1180.f, W - 60.f), px = W / 2.f - pw / 2, py = 50;
    text("THIS LIFE", px, py, 13, STEEL); text(C.n, px, py + 16, 46, C.c); wrapText(C.blurb, px, py + 70, pw * .55f, 15, rgba(199, 194, 182));
    { float bx = px + pw - 380; DrawRectangleRec({bx, py + 12, 380, 74}, rgba(12, 14, 20, 235)); DrawRectangleLinesEx({bx, py + 12, 380, 74}, 1, C.c);
      text(std::string("[Z]  ") + C.act, bx + 14, py + 22, 18, C.c); text(C.actD, bx + 14, py + 46, 13, BONE); char b[64]; snprintf(b, 64, "Recharge %ds", (int)roundf(C.actCd * (1 - P.perks.cdr))); text(b, bx + 14, py + 64, 12, STEEL); }
    float cw = (pw - 32) / 3, ty = py + 120;
    for (int t = 0; t < 3; t++) { float cx = px + t * (cw + 16); int tot = 0; for (int i = 0; i < 4; i++) tot += P.rank[t][i];
        text(C.t[t].n, cx, ty, 22, BONE); text(std::to_string(tot) + " pts", cx + cw, ty + 6, 13, STEEL, 2);
        for (int i = 0; i < 4; i++) { const Skill& sk = C.t[t].s[i]; float y = ty + 36 + i * 96; int r = P.rank[t][i]; bool open = i == 0 || P.rank[t][i - 1] > 0;
            Rectangle box = {cx, y, cw, 84}; DrawRectangleRec(box, r > 0 ? rgba(24, 28, 36, 245) : rgba(14, 16, 22, 235)); DrawRectangleLinesEx(box, r > 0 ? 1.5f : 1.f, r > 0 ? C.c : open ? rgba(60, 68, 80) : rgba(36, 40, 48));
            if (i > 0) DrawRectangle((int)(cx + 24), (int)(y - 12), 2, 12, P.rank[t][i - 1] > 0 ? C.c : rgba(50, 56, 66));
            text(sk.n, cx + 14, y + 10, 17, r > 0 ? C.c : open ? BONE : rgba(120, 124, 130));
            for (int k = 0; k < sk.max; k++) { Rectangle pip = {cx + cw - 16 - (sk.max - k) * 18.f, y + 12, 12, 12}; if (k < r) DrawRectangleRec(pip, C.c); else DrawRectangleLinesEx(pip, 1, rgba(80, 86, 96)); }
            wrapText(sk.d, cx + 14, y + 36, cw - 28, 13, r > 0 ? rgba(210, 205, 194) : rgba(140, 142, 146)); } }
    text("You don't choose. Every level, fate drops one point into a skill you've unlocked.   [K] close", W / 2.f, ty + 36 + 4 * 96 + 8, 14, STEEL, 1);
    drawCursor();
    drawTabs(1);
}
static void drawPause() { dim(.6f); float x = W / 2.f - 250, y = H * .35f; text("PAUSED", W / 2.f, y, 60, BONE, 1); if (button("RESUME", x, y + 100, 150, 46, true)) state = ST_PLAY;
    if (button("SETTINGS", x + 170, y + 100, 150, 46, false)) { settingsBack = ST_PAUSE; state = ST_SETTINGS; } if (button("SAVE & QUIT", x + 340, y + 100, 170, 46, false)) { saveGame(); state = ST_TITLE; }
    if (button("ABANDON RUN", x + 530, y + 100, 170, 46, false)) { deleteSave(); state = ST_TITLE; } drawCursor(); }
static void applyDisplay() {
    int mon = GetCurrentMonitor(); int w = cfg.res == 5 ? GetMonitorWidth(mon) : RES[cfg.res][0], h = cfg.res == 5 ? GetMonitorHeight(mon) : RES[cfg.res][1];
    if (IsWindowFullscreen()) ToggleFullscreen();
    if (cfg.fullscreen) { SetWindowSize(w, h); ToggleFullscreen(); }
    else { w = std::min(w, GetMonitorWidth(mon)); h = std::min(h, GetMonitorHeight(mon) - 60); SetWindowSize(w, h); SetWindowPosition((GetMonitorWidth(mon) - w) / 2, std::max(30, (GetMonitorHeight(mon) - h) / 2)); }
    W = GetScreenWidth(); H = GetScreenHeight();
}
static void drawSettings() {
    dim(.78f); float x = W / 2.f - 300, y = H * .06f; text("SETTINGS", x, y, 40, BONE); y += 56;
    static const int fpsOpt[5] = {30, 60, 90, 144, 0}; int fi = 0; for (int i = 0; i < 5; i++) if (fpsOpt[i] == cfg.fps) fi = i;
    auto row = [&](const char* label, const std::string& val, const char* hint) -> int { text(label, x, y + 10, 18, BONE); text(val, x + 420, y + 10, 18, SODIUM, 1); text(hint, x, y + 42, 12, STEEL);
        int r = 0; if (button("<", x + 300, y + 2, 40, 34, false)) r = -1; if (button(">", x + 500, y + 2, 40, 34, false)) r = 1; y += H < 800 ? 50 : 58; return r; };
    int d;
    if ((d = row("Difficulty", DIFFN[cfg.diff], "Zombie health, damage and numbers. They also get tougher every level you gain."))) cfg.diff = (cfg.diff + d + 4) % 4;
    if ((d = row("Display", cfg.fullscreen ? "Fullscreen" : "Windowed", "F11 toggles anytime."))) { cfg.fullscreen ^= 1; applyDisplay(); }
    { std::string rn = cfg.res == 5 ? std::string("Native ") + std::to_string(GetMonitorWidth(GetCurrentMonitor())) + "x" + std::to_string(GetMonitorHeight(GetCurrentMonitor())) : std::to_string(RES[cfg.res][0]) + "x" + std::to_string(RES[cfg.res][1]);
      if ((d = row("Resolution", rn, "Window size, or the screen mode in fullscreen."))) { cfg.res = (cfg.res + d + 6) % 6; applyDisplay(); } }
    static const char* q3[3] = {"Low", "Medium", "High"}; static const char* sh3[3] = {"Off", "Low (1024)", "High (2048)"};
    if ((d = row("Frame rate cap", cfg.fps ? std::to_string(cfg.fps) + " FPS" : std::string("Unlimited"), "Lower = less heat and power. 60 is plenty for this game."))) { fi = (fi + d + 5) % 5; cfg.fps = fpsOpt[fi]; SetTargetFPS(cfg.fps); }
    if ((d = row("Shadows", sh3[cfg.shadows], "Sun and flashlight shadows. Off is the biggest speedup on weak GPUs."))) { cfg.shadows = (cfg.shadows + d + 3) % 3; reinitShadows(); }
    { static const char* CN[3] = {"Classic", "Aim-relative", "Click-to-move"}; static const char* CH[3] = {"W/A/S/D are fixed compass directions; two keys for diagonals.", "W walks toward the cursor, S backs off, A/D circle around it.", "Diablo style: click to walk, click a zombie to attack it, click loot to grab it. Alt+click attacks in place."};
      if ((d = row("Controls", CN[cfg.controls], CH[cfg.controls]))) cfg.controls = (cfg.controls + d + 3) % 3; }
    if ((d = row("Foliage density", q3[cfg.foliage], "Grass, bushes and tall grass. Applies to the next map (next life)."))) cfg.foliage = (cfg.foliage + d + 3) % 3;
    auto pct = [](float v) { return std::to_string((int)roundf(v * 100)) + "%"; };
    if ((d = row("Film grain", cfg.grain ? "On" : "Off", "Animated static over the screen. Off is cleaner and a little faster."))) cfg.grain ^= 1;
    if ((d = row("Master volume", pct(cfg.master), ""))) cfg.master = Clamp(cfg.master + d * .1f, 0, 1);
    if ((d = row("Effects volume", pct(cfg.sfx), "Guns, zombies, ambience"))) cfg.sfx = Clamp(cfg.sfx + d * .1f, 0, 1);
    if ((d = row("Music volume", pct(cfg.music), "Tension, horde drums, heartbeat"))) cfg.music = Clamp(cfg.music + d * .1f, 0, 1);
    text(TextFormat("Running at %d FPS", GetFPS()), x, y + 6, 14, STEEL);
    if (button("BACK", x, y + 36, 150, 46, true) || IsKeyPressed(KEY_ESCAPE)) { saveCfg(); state = settingsBack; }
    drawCursor();
}
static void drawDead() {
    if (deadT < 1.2f) return; dim(std::min(.75f, (deadT - 1.2f)));
    float x = std::max(40.f, W / 2.f - 360), y = H * .25f; int hrs = (int)(P.alive / CYCLE * 24);
    text("RUN OVER", x, y, 14, STEEL); text(deathLine, x, y + 22, std::min(70.f, W * .06f), BLOODC); y += 120;
    std::string vals[4] = {std::to_string(hrs) + "h", std::to_string(kills), std::to_string((int)(P.walked / 1000)) + "." + std::to_string(((int)P.walked % 1000) / 100) + " km", std::to_string(P.lvl)}; const char* lab[4] = {"SURVIVED", "KILLS", "WALKED", "LEVEL"};
    for (int i = 0; i < 4; i++) { float cx = x + i * 170; DrawLineEx(V2(cx, y), V2(cx + 150, y), 1, rgba(60, 68, 80)); text(vals[i], cx, y + 10, 32, BONE); text(lab[i], cx, y + 48, 11, STEEL); }
    y += 84; text(std::string("Died a ") + CL().n + " with " + std::to_string(P.points) + " skill points. The next life rolls a new class and a new map.", x, y, 15, CL().c);
    y += 22; text(std::to_string(gibCount) + " body parts left on the roads of Nightfall County.", x, y, 15, STEEL);
    if (bestRar >= 0) { y += 22; text("Best find: " + bestName + " (" + RARN[bestRar] + ")", x, y, 15, RARC[bestRar]); }
    if (newRecord) { y += 22; text("Longest you've ever lasted.", x, y, 15, SODIUM); }
    if (button("NEW LIFE", x, y + 40, 200, 50, true) || IsKeyPressed(KEY_ENTER)) newGame(); drawCursor();
}

#ifdef NF_AUTOTEST
static int atFrame = 0;
static void autotest() {
    atFrame++; if (getenv("NF_JUMP") && atFrame == 26) atFrame = atoi(getenv("NF_JUMP")); int f = atFrame;
    auto shot = [](const char* n) { TakeScreenshot(n); };
    if (f == 20) shot("shot_title.png");
    if (f == 25) { newGame(); P.p = Vector2Add(TOWNS[0].c, V2(-30, 20)); camPos = P.p; gTime = CYCLE * .2f; for (int i = 0; i < 12; i++) spawnAround(Z_WALKER, 6, 14); P.gun[0] = makeWeapon(W_SHOTGUN, 2); P.hasGun[0] = true; P.active = 0; P.gun[0].clip = 6; invAdd(IT_A12, 30); }
    if (f == 60) shot("shot_day_city.png");
    if (f == 62) { camZoom = 1.6f; }
    if (f == 70) shot("shot_closeup.png");
    if (f == 72) { camZoom = 1.f; }
    if (f > 60 && f < 140 && f % 6 == 0) { // blast nearby zombies
        int best = -1; float bd = 1e9; for (int i = 0; i < (int)zombies.size(); i++) { float d = Vector2Distance(zombies[i].p, P.p); if (d < bd) { bd = d; best = i; } }
        if (best >= 0) { P.a = atan2f(zombies[best].p.y - P.p.y, zombies[best].p.x - P.p.x); P.cd = 0; P.reload = 0; if (P.gun[0].clip == 0) P.gun[0].clip = 6; fire(); } }
    if (f == 150) shot("shot_gore.png");
    if (f == 151) { P.p = Vector2Add(TOWNS[0].c, V2(40, 40)); camPos = P.p; for (int i = 0; i < 6; i++) { Zombie& z = spawnZ(i % 2 ? Z_RUNNER : Z_SOLDIER, Vector2Add(P.p, V2(-6 + i * 2.5f, -7))); killZ(z, PI / 2 + frand(-.3f, .3f), i == 2, 0, false); } for (int i = 0; i < 3; i++) spawnZ(i == 0 ? Z_BRUTE : Z_BLOATER, Vector2Add(P.p, V2(-4 + i * 4.f, 5))); }
    if (f == 154) shot("shot_corpses.png");
    if (f == 156) camZoom = 1.5f;
    if (f == 160) shot("shot_corpses_close.png");
    if (f == 162) camZoom = 1.f;
    if (f == 155) { int b = -1; for (int i = 0; i < (int)blds.size(); i++) if (blds[i].enter && blds[i].town == 0 && blds[i].type == B_HOUSE) { b = i; break; } if (b < 0) for (int i = 0; i < (int)blds.size(); i++) if (blds[i].enter && blds[i].town == 0) { b = i; break; }
        if (b >= 0) { P.p = V2((blds[b].x0 + blds[b].x1) / 2, (blds[b].z0 + blds[b].z1) / 2); camPos = P.p; } }
    if (f == 200) shot("shot_interior.png");
    if (f == 205) { gTime = CYCLE * (16.f / 24.f) + CYCLE; P.light = true; P.batt = 100; P.p = Vector2Add(TOWNS[0].c, V2(10, -10)); camPos = P.p; staticLights.push_back({xz3(Vector2Add(P.p, V2(-6, -4)), 1.2f), rgba(255, 120, 40), 10, 2}); for (int i = 0; i < 8; i++) { Zombie* z = spawnAround(i < 2 ? Z_VOLATILE : Z_WALKER, 5, 12); if (z) z->aggro = 0; } }
    if (f == 250) shot("shot_night.png");
    if (f == 255) { P.p = V2(500, 500); for (auto& t : trees) if (Vector2Distance(t.p, P.p) < 3000 && Vector2Distance(t.p, TOWNS[3].c) < 140) { P.p = Vector2Add(t.p, V2(4, 4)); break; } camPos = P.p; gTime = CYCLE * 1.35f; rainLevel = 1; rainTarget = 1; P.light = false; }
    if (f == 300) shot("shot_forest_rain.png");
    if (f == 305) { rainLevel = 0; rainTarget = 0; gTime = CYCLE * 2.2f; P.p = Vector2Add(TOWNS[4].c, V2(-20, 10)); camPos = P.p; }
    if (f == 30) { printf("START melee=%s guns=%d,%d worn:", MB[P.melee].n, P.hasGun[0], P.hasGun[1]); for (int i = 0; i < CS_COUNT; i++) if (P.eq[i]) printf(" [%s %s %.0f%% %dx%d]", CSN[i], P.eq[i]->name.c_str(), P.eq[i]->cond * 100, gridW(*P.eq[i]), gridH(*P.eq[i])); Gear g = gear(); printf("\n  gear warm=%.2f water=%.2f armor=%.2f slots=%d used=%d rags=%d\n", g.warm, g.water, g.armor, g.slots, g.used, invCount(IT_RAG));
        int left = invAdd(IT_BEANS, 20); printf("INV beans added=%d leftover=%d\n", 20 - left, left);
        auto pack = makeCloth(cdIndex("Hiking Pack"), 2); takeCloth(pack); left = invAdd(IT_BEANS, 20); printf("INV with %s (%dx%d): leftover=%d count=%d\n", pack->name.c_str(), gridW(*pack), gridH(*pack), left, invCount(IT_BEANS));
        invTake(IT_BEANS, 999); auto sh = P.eq[CS_TORSO]; float c0 = sh->cond; invAdd(IT_SEWKIT, 1); applyTool(invFind(IT_SEWKIT), *sh); printf("REPAIR %s %.2f -> %.2f kits=%d\n", sh->name.c_str(), c0, sh->cond, invCount(IT_SEWKIT));
        invAdd(IT_LEATHER, 1); float a0 = clothArmor(*sh); applyTool(invFind(IT_LEATHER), *sh); printf("PATCH armor %.3f -> %.3f\n", a0, clothArmor(*sh)); for (int k = 0; k < 10; k++) wearDamage(20); printf("WEAR shirt cond=%.2f\n", sh->cond);
        for (int k = 0; k < 6; k++) { auto c = makeCloth(clothFor(k % 2 ? F_MIL : F_CIV), rollRarity()); printf("ROLL %s [%s] cond=%.0f%% rar=%d warm=%.2f armor=%.2f water=%.2f grid=%dx%d decor='%s'\n", c->name.c_str(), CSN[CD[c->def].slot], c->cond * 100, c->rar, clothWarm(*c), clothArmor(*c), clothWater(*c), gridW(*c), gridH(*c), c->decor.c_str()); }
        int nc = 0, tools = 0, clothInGarage = 0; for (auto& l : loot) { if (l.kind == LK_CLOTH) nc++; if (l.kind == LK_MELEE && l.id >= MK_CROWBAR) tools++; } printf("LOOT clothes=%d newTools=%d total=%d\n", nc, tools, (int)loot.size()); (void)clothInGarage; }
    if (f == 31) { camZoom = 1.6f; gTime = CYCLE * .3f; for (auto& z : zombies) z.gone = true; }
    if (f == 36) { shot("shot_outfit.png"); P.eq[CS_OUTER] = makeCloth(cdIndex("Hunting Coat"), 1); P.eq[CS_LEGS] = makeCloth(cdIndex("Cargo Pants"), 0); P.eq[CS_FEET] = makeCloth(cdIndex("Rubber Boots"), 0); }
    if (f == 40) { shot("shot_outfit2.png"); camZoom = 1.f; }
    if (f == 351) { for (int k = 0; k < 4; k++) spawnCloth(xz3(Vector2Add(P.p, V2(frand(-1, 1), frand(-1, 1))), .05f), makeCloth(clothFor(F_STORE | F_MIL), irand(0, 3))); spawnItem(xz3(P.p, .05f), IT_TAPE, 1); invAdd(IT_BANDAGE, 2); invAdd(IT_WATER, 1); }
    if (f == 350) shot("shot_military.png");
    if (f == 352) state = ST_INV;
    if (f == 356) shot("shot_inventory.png");
    if (f == 360) shot("shot_inventory.png");
    if (f == 362) state = ST_MAP;
    if (f == 370) shot("shot_map.png");
    if (f == 372) { state = ST_SKILLS; for (int i = 0; i < 6; i++) { pendingLv++; } P.lvl = 7; grantLevelPoints(); }
    if (f == 376) shot("shot_skills.png");
    if (f == 377) { state = ST_PLAY; if (!hotZones.empty()) { P.p = Vector2Add(hotZones[0].c, V2(3, 6)); camPos = P.p; gTime = CYCLE * 3.25f; dressHotZones(); } }
    if (f == 379) { useActive(); }
    if (f == 381) shot("shot_hotzone.png");
    if (f == 386) { gTime = CYCLE * 4.3f; rainLevel = 0; rainTarget = 0; camZoom = 1.35f; for (auto& z : zombies) z.gone = true;
        if (!lofts.empty()) { P.p = lofts[0].base; camPos = P.p; doAct({A_LADDER, 0}); } }
    if (f == 392) { shot("shot_loft.png"); printf("LOFT loft=%d hideK=%.2f\n", P.loft, P.hideK); }
    if (f == 394) { if (P.loft >= 0) doAct({A_LADDER, P.loft}); int di = -1; for (int i = 0; i < (int)doorObjs.size(); i++) if (blds[doorObjs[i].bld].type == B_HOUSE) { di = i; break; }
        if (di >= 0) { DoorObj& d = doorObjs[di]; d.open = true; Vector2 n = d.horiz ? V2(0, 1.7f) : V2(1.7f, 0); P.p = Vector2Add(d.c, n); if (buildingAt(P.p) >= 0) P.p = Vector2Subtract(d.c, n); camPos = P.p; P.a = atan2f(d.c.y - P.p.y, d.c.x - P.p.x); } }
    if (f == 395) { camZoom = 1.6f; P.p = Vector2Add(P.p, V2(1.2f, 0)); }
    if (f == 396) shot("shot_door_open.png");
    if (f == 397) { for (auto& d : doorObjs) if (Vector2Distance(d.c, P.p) < 3) printf("DOOR c=%.1f,%.1f horiz=%d w=%.1f open=%d anim=%.2f bld=%d cut=%.2f\n", d.c.x, d.c.y, d.horiz, d.w, d.open, d.anim, d.bld, blds[d.bld].cut); nearAct = findAct(); printf("ACT kind=%d label=%s\n", nearAct.kind, actLabel(nearAct).c_str()); if (nearAct.kind == A_DOOR) doAct(nearAct); }
    if (f == 401) shot("shot_door.png");
    if (f == 402) { Vector2 cp = V2(-1, -1); for (auto& c : covers) if (townAt(c.p, 1.2f) < 0 && hotAt(c.p, 1.5f) < 0) { cp = c.p; break; } P.p = cp; camPos = cp; P.crouch = true; P.light = false; gTime = CYCLE * 4.6f;
        invAdd(IT_KINDLING, 8); invAdd(IT_MATCHES, 3); invAdd(IT_LOG, 1); P.melee = MK_AXE; P.active = 2; P.a = 0; buildFire(); }
    if (f == 408) { printf("HIDE hideK=%.2f fires=%d\n", P.hideK, (int)fires.size()); shot("shot_fire_hide.png"); }
    if (f == 410) { int ci = -1; for (int i = 0; i < (int)carObjs.size(); i++) if (!carObjs[i].burned) { ci = i; break; } if (ci >= 0) { size_t n0 = loot.size(); P.p = Vector2Add(carObjs[ci].p, V2(0, 2.6f)); camPos = P.p; doAct({A_CAR, ci}); printf("CAR loot +%d\n", (int)(loot.size() - n0)); } }
    if (f == 412) { P.broken = true; P.splinted = false; craftSplint(); quickUse(true); printf("SPLINT splinted=%d leg=%.2f", P.splinted, legFactor()); P.splintT = CYCLE / 2; printf(" after12h=%.2f", legFactor()); P.splintT = CYCLE * 2.5f; printf(" after2.5d=%.2f\n", legFactor()); }
    if (f == 414) { state = ST_INV; }
    if (f == 418) shot("shot_craft.png");
    if (f == 419) { state = ST_PLAY; camZoom = 1.f; gTime = CYCLE * 5.25f; rainLevel = 0; P.crouch = false; Vector2 bp = P.p; float bd = 1e9f;
        for (auto& t : trees) { float d = Vector2Distance(t.p, TOWNS[1].c); if (d > TOWNS[1].r * 1.4f && d < bd) { bd = d; bp = t.p; } } P.p = Vector2Add(bp, V2(6, 6)); camPos = P.p; }
    if (f == 422) shot("shot_ground.png");
    if (f == 426 - 2) {}
    if (f == 428) { int ov = 0; for (auto& b : blds) if (rectOnRoad(b.x0, b.z0, b.x1, b.z1, 0)) ov++; int ovT = 0; for (auto& b : blds) for (auto& r : roads) if (r.tier > 0 && segDist(V2((b.x0 + b.x1) / 2, (b.z0 + b.z1) / 2), r.a, r.b) < std::min(b.x1 - b.x0, b.z1 - b.z0) / 2 - .5f) { ovT++; break; } printf("TRACKS through_buildings=%d\n", ovT); printf("ROADS touching_any_road=%d of %d\n", ov, (int)blds.size());
        for (auto& z : zombies) z.gone = true; P.hp = P.perks.maxhp; P.a = 0; Zombie& z = spawnZ(Z_WALKER, Vector2Add(P.p, V2(1.3f, 0))); z.losT = 99; float hp0 = P.hp; P.blocking = true; P.sta = 100; hitPlayer(20, z.p, Z_WALKER); float b1 = hp0 - P.hp;
        P.shieldHP = 120; hp0 = P.hp; hitPlayer(20, z.p, Z_WALKER); float b2 = hp0 - P.hp; P.blocking = false; P.iframe = 0; hp0 = P.hp; hitPlayer(20, z.p, Z_WALKER); float b3 = hp0 - P.hp;
        printf("BLOCK dmg bare-block=%.1f shield=%.1f unblocked=%.1f shieldHP=%.0f\n", b1, b2, b3, P.shieldHP); Vector2 zp = z.p; P.kickCd = 0; P.sta = 100; kick(); printf("KICK knock=%.1f stun=%.2f\n", Vector2Length(z.k), z.stun); (void)zp; }
    if (f == 430) { gTime = CYCLE * 6.95f; int b = -1; for (int i = 0; i < (int)blds.size(); i++) if (blds[i].enter && blds[i].type == B_HOUSE && blds[i].x1 - blds[i].x0 > 8) { b = i; break; }
        if (b >= 0) { P.p = V2(blds[b].x0 + 2.2f, (blds[b].z0 + blds[b].z1) / 2); camPos = P.p; P.a = PI; } P.light = true; P.batt = 100; camZoom = 1.3f; }
    if (f == 436) shot("shot_flash_wall.png");
    if (f == 437) { P.a = 0; }
    if (f == 442) shot("shot_flash_room.png");
    if (f == 443) { camZoom = 1.f; state = ST_SETTINGS; }
    if (f == 447) { shot("shot_settings.png"); state = ST_PLAY; }
    if (f == 452) { state = ST_PLAY; camZoom = 1.25f; gTime = CYCLE * 8.3f; rainLevel = 0; rainTarget = 0; for (auto& z : zombies) z.gone = true; int hb = -1;
        for (int i = 0; i < (int)blds.size(); i++) if (blds[i].type == B_HOUSE && blds[i].enter && !blds[i].doors.empty() && blds[i].doors[0].obj >= 0 && blds[i].x1 - blds[i].x0 > 9) { hb = i; break; }
        if (hb >= 0) { Building& b = blds[hb]; P.p = V2((b.x0 + b.x1) / 2, (b.z0 + b.z1) / 2); collide(P.p, .35f); camPos = P.p; P.light = false;
            for (auto& dr : b.doors) if (dr.obj >= 0) { DoorObj& d = doorObjs[dr.obj]; d.open = false; d.broken = false; d.hp = d.hpMax; }
            DoorObj& d0 = doorObjs[b.doors[0].obj]; for (int k = 0; k < 3; k++) { Zombie& z = spawnZ(Z_WALKER, Vector2Add(Vector2Add(d0.c, Vector2Scale(d0.outN, 1.8f)), V2(frand(-1.f, 1.f), 0))); z.aggro = 30; z.losT = 0; }
            printf("HOUSE type=%d cond=%d door metal=%d hp=%.0f c=%.1f,%.1f outN=%.1f,%.1f P=%.1f,%.1f bld=%.1f,%.1f-%.1f,%.1f\n", b.type, b.cond, d0.metal, d0.hp, d0.c.x, d0.c.y, d0.outN.x, d0.outN.y, P.p.x, P.p.y, b.x0, b.z0, b.x1, b.z1);
} }
    if (f == 466) shot("shot_house_rooms.png");
    if (f == 478) { for (auto& d : doorObjs) if (Vector2Distance(d.c, P.p) < 12) { printf("POUND door hp=%.0f/%.0f broken=%d\n", d.hp, d.hpMax, d.broken); break; } int np = 0; for (auto& z : zombies) if (z.pound >= 0) np++; printf("POUNDING zombies=%d\n", np); shot("shot_pound.png"); }
    if (f == 479) { for (auto& z : zombies) if (z.pound >= 0) { z.poundT = .05f; z.aggro = 0; } P.moving = false; }
    if (f == 486) { int np = 0; for (auto& z : zombies) if (z.pound >= 0) { np++; printf("  pounder id=%d poundT=%.1f aggro=%.1f sees=%d ignore=%.1f type=%d\n", z.id, z.poundT, z.aggro, z.sees, z.ignoreT, z.type); } printf("GAVEUP still_pounding=%d firedT=%.1f moving=%d\n", np, P.firedT, P.moving); }
    if (f == 480 || f == 482) for (auto& z : zombies) if (z.pound >= 0 || z.ignoreT > 0) printf("  f%d z%d pound=%d poundT=%.2f ignore=%.1f aggro=%.1f sees=%d\n", f, z.id, z.pound, z.poundT, z.ignoreT, z.aggro, z.sees);
    if (f == 488) { for (auto& z : zombies) z.gone = true; int ci = -1; for (int i = 0; i < (int)carObjs.size(); i++) if (townAt(carObjs[i].p, 1.f) == 0) { ci = i; break; } if (ci < 0) ci = 0;
        if (!carObjs.empty()) { P.p = Vector2Add(carObjs[ci].p, V2(0, 6)); camPos = P.p; } camZoom = 1.1f; }
    if (f == 494) shot("shot_cars.png");
    if (f == 496) { bool done = false; for (int i = 0; i < (int)solids.size() && !done; i++) { const Solid& sd = solids[i]; if (sd.kind != SK_FENCE || sd.h > 1.5f) continue; bool hz = sd.x1 - sd.x0 > sd.z1 - sd.z0; Vector2 c = V2((sd.x0 + sd.x1) / 2, (sd.z0 + sd.z1) / 2);
            Vector2 st = hz ? V2(c.x, sd.z0 - .9f) : V2(sd.x0 - .9f, c.y); if (!freeAt(st.x, st.y, .4f)) continue; P.p = st; P.a = hz ? PI / 2 : 0; P.moving = false; P.sta = 100; P.rollCd = 0; P.roll = 0; dodge(); printf("VAULT t=%.2f from %.1f,%.1f to %.1f,%.1f h=%.2f\n", P.vaultT, P.vaultA.x, P.vaultA.y, P.vaultB.x, P.vaultB.y, P.vaultH); done = true; } }
    if (f == 504) { Zombie& z = spawnZ(Z_WALKER, P.p); z.aggro = 0; z.losT = 99; }
    if (f == 510) { float md = 99; for (auto& z : zombies) md = std::min(md, Vector2Distance(z.p, P.p)); printf("SPACING nearest=%.2f\n", md); }
    if (f == 512) { int nb = 0; for (auto& b : blds) if (b.enter) nb++; printf("STATS blds=%d enterable=%d props=%d solids=%d trees=%d loot=%d roads=%d zombies=%d decals=%d gibs=%d corpses=%d fps=%d\n", (int)blds.size(), nb, (int)props.size(), (int)solids.size(), (int)trees.size(), (int)loot.size(), (int)roads.size(), (int)zombies.size(), (int)decals.size(), (int)gibs.size(), (int)corpses.size(), GetFPS()); fflush(stdout); }
    if (f == 514) { int up = 0, dn = 0; for (auto& st : stairs) (st.down ? dn : up)++; printf("CELLS stairs up=%d down=%d blds=%d\n", up, dn, (int)blds.size());
        for (auto& z : zombies) z.gone = true; gTime = CYCLE * 8.25f; for (int i = 0; i < (int)stairs.size(); i++) if (stairs[i].down) { P.p = Vector2Add(stairs[i].a, V2(.6f, 0)); camPos = P.p; nearAct = findAct(); printf("STAIRS act=%d label=%s\n", nearAct.kind, actLabel(nearAct).c_str()); spawnZ(Z_WALKER, Vector2Add(P.p, V2(4, 0))).aggro = 20; doAct(nearAct);
            int cb = cellOf(P.p); printf("BASEMENT inCell=%d cell=%d kind=%d dark=%.2f followers=%d loot_here=%d\n", inCell(P.p), cb, cb >= 0 ? blds[cb].cellKind : -1, darkK(), (int)followers.size(), (int)std::count_if(loot.begin(), loot.end(), [](const Loot& l) { return Vector2Distance(V2(l.p.x, l.p.z), P.p) < 12; })); P.light = true; break; } }
    if (f == 522) { int nz = 0; for (auto& z : zombies) if (Vector2Distance(z.p, P.p) < 15) nz++; printf("BASEMENT zombies_near=%d followers_left=%d\n", nz, (int)followers.size()); shot("shot_basement.png"); }
    if (f == 524) { for (auto& z : zombies) z.gone = true; gTime = CYCLE * .45f; P.light = false; for (int i = 0; i < (int)stairs.size(); i++) if (!stairs[i].down) { P.p = Vector2Add(stairs[i].a, V2(.5f, 0)); camPos = P.p; nearAct = findAct(); doAct(nearAct); int cb = cellOf(P.p); printf("UPSTAIRS cell=%d kind=%d spots=%d\n", cb, cb >= 0 ? blds[cb].cellKind : -1, cb >= 0 ? (int)blds[cb].spots.size() : -1); break; } }
    if (f == 530) shot("shot_upstairs.png");
    if (f == 532) { nearAct = findAct(); printf("BACK label=%s\n", actLabel(nearAct).c_str()); doAct(nearAct); printf("BACK inCell=%d\n", inCell(P.p)); }
    if (f == 535) { int bad = 0, tested = 0, badStairs = 0; // walk test: from inside the front door, can you reach every doorway and stair?
        for (int bi = 0; bi < (int)blds.size(); bi++) { Building& b = blds[bi]; if (!b.enter || b.doors.empty() || b.doors[0].obj < 0) continue; tested++;
            const float S = .3f; int nx = (int)((b.x1 - b.x0) / S) + 1, nz = (int)((b.z1 - b.z0) / S) + 1; std::vector<char> seen(nx * nz, 0); std::vector<int> q;
            Vector2 st = Vector2Subtract(doorObjs[b.doors[0].obj].c, Vector2Scale(doorObjs[b.doors[0].obj].outN, .9f));
            int sx = (int)((st.x - b.x0) / S), sz = (int)((st.y - b.z0) / S); if (sx < 0 || sz < 0 || sx >= nx || sz >= nz) continue; seen[sz * nx + sx] = 1; q.push_back(sz * nx + sx);
            for (size_t k = 0; k < q.size(); k++) { int cx = q[k] % nx, cz = q[k] / nx; static const int D4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                for (auto& dd : D4) { int ax = cx + dd[0], az = cz + dd[1]; if (ax < 0 || az < 0 || ax >= nx || az >= nz || seen[az * nx + ax]) continue; float wx = b.x0 + ax * S, wz = b.z0 + az * S; if (!freeAt(wx, wz, .3f)) continue; seen[az * nx + ax] = 1; q.push_back(az * nx + ax); } }
            auto reach = [&](Vector2 p) { for (int dz = -2; dz <= 2; dz++) for (int dx = -2; dx <= 2; dx++) { int ax = (int)((p.x - b.x0) / S) + dx, az = (int)((p.y - b.z0) / S) + dz; if (ax >= 0 && az >= 0 && ax < nx && az < nz && seen[az * nx + ax]) return true; } return false; };
            bool ok = true; for (auto& g : b.gaps) if (!reach(g)) ok = false; for (auto& stp : stairs) if (stp.bA == bi && !reach(stp.a)) { ok = false; badStairs++; }
            if (!ok) { bad++; if (bad <= 3) printf("  WALKFAIL type=%d at %.0f,%.0f gaps=%d\n", b.type, b.x0, b.z0, (int)b.gaps.size()); } }
        printf("WALKTEST tested=%d blocked=%d stairs_blocked=%d\n", tested, bad, badStairs); }
    if (f == 536) { int bt[18] = {}; for (auto& b : blds) if (!b.cellKind) bt[b.type]++; printf("BTYPES house=%d store=%d garage=%d barn=%d cabin=%d shed=%d factory=%d fire=%d school=%d trailer=%d gas=%d\n", bt[B_HOUSE], bt[B_STORE], bt[B_GARAGE], bt[B_BARN], bt[B_CABIN], bt[B_SHED], bt[B_FACTORY], bt[B_FIRE], bt[B_SCHOOL], bt[B_TRAILER], bt[B_GAS]);
        int rt[3] = {}; for (auto& r : roads) rt[r.tier]++; printf("ROADTIERS paved=%d gravel=%d dirt=%d\n", rt[0], rt[1], rt[2]);
        for (auto& z : zombies) z.gone = true; gTime = CYCLE * .4f; camZoom = .8f; for (auto& b : blds) if (b.type == B_TRAILER) { P.p = V2(b.x1 + 3, b.z1 + 2); camPos = P.p; break; } }
    if (f == 540) shot("shot_trailers.png");
    if (f == 541) { for (auto& b : blds) if (b.type == B_GAS) { P.p = V2(b.x1 + 4, b.z1 + 6); camPos = P.p; break; } }
    if (f == 545) shot("shot_gas.png");
    if (f == 546) { for (auto& b : blds) if (b.type == B_BARN && !b.cellKind) { P.p = V2(b.x1 + 8, b.z0 + 4); camPos = P.p; break; } camZoom = .7f; }
    if (f == 550) shot("shot_farm.png");
    if (f == 551) { for (auto& b : blds) if (b.type == B_FACTORY) { P.p = V2((b.x0 + b.x1) / 2, (b.z0 + b.z1) / 2); camPos = P.p; break; } camZoom = 1.f; }
    if (f == 555) shot("shot_factory.png");
    if (f == 556) { for (auto& b : blds) if (b.type == B_FIRE) { P.p = V2((b.x0 + b.x1) / 2, (b.z0 + b.z1) / 2 + 1); camPos = P.p; break; } }
    if (f == 559) shot("shot_firestation.png");
    if (f == 640) { state = ST_PLAY; P.cls = 0; for (const char* n : {"Riot Helmet", "Police Shirt", "Police Vest", "Combat Boots"}) { auto c = makeCloth(cdIndex(n), 0); makeSetPiece(*c, 0); P.eq[CD[c->def].slot] = c; } recalcPerks();
        printf("SETS police=%d armor=%.2f pistol=%.2f cdr=%.2f name=%s\n", setCount(0), P.perks.armor, P.perks.pistol, P.perks.cdr, P.eq[CS_TORSO]->name.c_str());
        invAdd(IT_BANDAGE, 3); P.hp = 55; P.lvl = 4; kills = 17; hotbar[7] = {1, IT_BANDAGE}; Vector2 pp = P.p; int nl = (int)loot.size(); int d0 = -1; for (int i = 0; i < (int)doorObjs.size(); i++) if (!doorObjs[i].broken) { doorObjs[i].broken = true; d0 = i; break; }
        saveGame(); printf("SAVE wrote=%d size=%ld\n", hasSave(), (long)GetFileLength(runSavePath().c_str()));
        P.hp = 1; P.eq[CS_TORSO].reset(); kills = 0; loadGame();
        printf("LOAD hp=%.0f lvl=%d kills=%d torso=%s set=%d bandages=%d hot7=%d pos_ok=%d loot=%d/%d door_broken=%d\n", P.hp, P.lvl, kills, P.eq[CS_TORSO] ? P.eq[CS_TORSO]->name.c_str() : "none", setCount(0), invCount(IT_BANDAGE), hotbar[7].id, Vector2Distance(pp, P.p) < .01f, (int)loot.size(), nl, d0 >= 0 ? doorObjs[d0].broken : -1);
        deleteSave(); printf("SAVE deleted=%d\n", !hasSave()); }
    if (f == 642) { state = ST_INV; }
    if (f == 645) { int up = 0; for (auto& l : lofts) { P.loft = (int)(&l - &lofts[0]); P.p = l.top; up++; break; } float x0 = P.p.x; for (int k = 0; k < 30; k++) { P.p.x -= .2f; } printf("LOFT walkable test start=%.1f\n", x0); state = ST_PLAY; }
    if (f == 650) { printf("LOFT pos clamp ok=%d loft=%d\n", P.loft >= 0 && P.p.x >= lofts[P.loft].x0, P.loft); P.loft = -1; CloseWindow(); exit(0); }
    if (f == 600) { gTime = CYCLE * .42f; rainLevel = 0; rainTarget = 0; for (auto& z : zombies) z.gone = true; camZoom = .75f; state = ST_PLAY; P.hp = 1e6f;
        Vector2 c = V2(MAP * .5f, MAP * .5f); for (int t = 0; t < 400; t++) { Vector2 q = V2(frand(200, MAP - 200), frand(200, MAP - 200)); if (roadDist(q) < 5 && townAt(q, 1.2f) < 0) { c = q; break; } } P.p = c; camPos = c;
        for (int i = 0; i < 70; i++) { Vector2 q = Vector2Add(c, polar(frand(-.6f, .6f), frand(7, 22))); if (!freeAt(q.x, q.y, .4f)) continue; Zombie& z = spawnZ(i % 9 == 0 ? Z_RUNNER : i % 23 == 0 ? Z_BRUTE : Z_WALKER, q); z.aggro = 20; } P.a = 0; }
    if (f == 615) shot("shot_horde_day.png");
    if (f == 616) { gTime = CYCLE * .9f; P.light = true; P.batt = 100; for (auto& z : zombies) z.gone = true; Vector2 c = P.p; for (int i = 0; i < 80; i++) { Vector2 q = Vector2Add(c, polar(frand(0, 6.28f), frand(5, 16))); if (freeAt(q.x, q.y, .4f)) spawnZ(i % 7 == 0 ? Z_RUNNER : Z_WALKER, q).aggro = 20; } }
    if (f == 628) shot("shot_horde_night.png");
    if (f == 630) { CloseWindow(); exit(0); }
    if (f == 560) { printf("DOORS %d LOFTS %d COVERS %d CARS %d\n", (int)doorObjs.size(), (int)lofts.size(), (int)covers.size(), (int)carObjs.size()); for (int i = 0; i < 6; i++) printf("TOWN %s %.0f %.0f\n", TOWNS[i].name, TOWNS[i].c.x, TOWNS[i].c.y); for (auto& h : hotZones) printf("HOT %s %.0f %.0f\n", HZN[h.kind], h.c.x, h.c.y); printf("CLASS %s pts=%d\n", CL().n, P.points); fflush(stdout); CloseWindow(); exit(0); }
}
#endif
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

// ============================================================ main
int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    loadCfg();
    InitWindow(1600, 900, TextFormat("Nightfall County %s", VERSION)); if (cfg.fullscreen || cfg.res != 1) applyDisplay(); SetExitKey(KEY_NULL); SetTargetFPS(cfg.fps); rlSetClipPlanes(1.0, 600.0);
    W = GetScreenWidth(); H = GetScreenHeight();
    const char* fonts[] = {"/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf", "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf", "/usr/share/fonts/noto/NotoSans-Bold.ttf"};
    font = GetFontDefault(); for (const char* f : fonts) if (FileExists(f)) { int cps[256]; int n = 0; for (int c = 32; c < 127; c++) cps[n++] = c; cps[n++] = 0xB0; cps[n++] = 0xB7; font = LoadFontEx(f, 64, cps, n); SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR); break; }
    initRender(); SHADOW_N = cfg.shadows == 1 ? 1024 : 2048; initShadows(); if (cfg.shadows == 0) shadowsOk = false; loadAllAssets(); initSurfaces(); initGroundAtlas(); buildLooks(); buildAudio(); loadBest(); HideCursor();
    rng.seed((unsigned)time(nullptr));
    // title backdrop: Cedar Falls at dusk
    genWorld(); genLoot(); P = Player(); P.p = TOWNS[0].c; gTime = CYCLE * .43f; camPos = TOWNS[0].c;
    for (int i = 0; i < 30; i++) { Vector2 p = Vector2Add(TOWNS[0].c, polar(frand(0, 6), frand(5, 50))); if (freeAt(p.x, p.y, .6f)) spawnZ(chance(.8f) ? Z_WALKER : Z_RUNNER, p); }
    float rainSndT = 0, windT = 0;
    while (!WindowShouldClose()) {
        W = GetScreenWidth(); H = GetScreenHeight();
        float dt = std::min(.033f, GetFrameTime());
#ifdef NF_AUTOTEST
        dt = 1.f / 30; autotest();
#endif
        if (IsKeyPressed(KEY_F10)) muted = !muted; if (IsKeyPressed(KEY_F11)) { cfg.fullscreen ^= 1; applyDisplay(); saveCfg(); }
        float wheel = GetMouseWheelMove(); if (wheel != 0 && (state == ST_TITLE || (state == ST_PLAY && !(IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))))) camZoom = Clamp(camZoom + wheel * .1f, .5f, 2.0f);
        if (state == ST_PLAY) {
            if (IsKeyPressed(KEY_SPACE)) dodge(); if (IsKeyPressed(KEY_R)) startReload(); if (IsKeyPressed(KEY_E)) interact(); if (IsKeyPressed(KEY_B)) quickUse(false); if (IsKeyPressed(KEY_N)) quickUse(true);
            if (IsKeyPressed(KEY_G)) throwItem(false); if (IsKeyPressed(KEY_H)) throwItem(true);
            if (IsKeyPressed(KEY_X)) { P.autorun = !P.autorun; P.autoA = P.moving ? P.moveA : P.a; if (P.autorun) P.crouch = false; }
            if (IsKeyPressed(KEY_V) || IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) melee(); if (IsKeyPressed(KEY_F)) toggleLight(); if (IsKeyPressed(KEY_U) && P.light) P.uv = !P.uv;
            if (IsKeyPressed(KEY_C)) P.crouch = !P.crouch;
            { static const int HK[10] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE, KEY_ZERO}; for (int i = 0; i < 10; i++) if (IsKeyPressed(HK[i])) useHot(i); }
            if (IsKeyPressed(KEY_Q)) kick();
            if (wheel != 0 && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))) { int dir = wheel > 0 ? 1 : 2; for (int k = 1; k <= 3; k++) { int n = (P.active + k * dir) % 3; if (n == 2 || P.hasGun[n]) { setActive(n); break; } } }
            if (IsKeyPressed(KEY_Z)) useActive();
            if (IsKeyPressed(KEY_TAB)) state = ST_INV; else if (IsKeyPressed(KEY_M)) state = ST_MAP; else if (IsKeyPressed(KEY_K)) state = ST_SKILLS;
            else if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE) || !IsWindowFocused()) state = ST_PAUSE;
            else update(dt);
        } else if (state == ST_INV || state == ST_MAP || state == ST_SKILLS) { // world keeps running, like DayZ
            if (state == ST_INV && (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ESCAPE))) { cancelDrag(); ctxMenu.on = false; }
            if ((state == ST_INV && (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ESCAPE))) || (state == ST_MAP && (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE))) || (state == ST_SKILLS && (IsKeyPressed(KEY_K) || IsKeyPressed(KEY_ESCAPE)))) state = ST_PLAY;
            else { State s = state; state = ST_PLAY; uiOpen = true; update(dt); uiOpen = false; if (state == ST_PLAY) state = s; }
        } else if (state == ST_PAUSE) { if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) state = ST_PLAY; }
        else if (state == ST_SETTINGS) {}
        else if (state == ST_TITLE) { if (IsKeyPressed(KEY_ESCAPE)) break; gTime += dt * .4f; camPos = Vector2Add(TOWNS[0].c, V2(cosf((float)GetTime() / 14) * 30, sinf((float)GetTime() / 14) * 22));
            for (auto& z : zombies) { z.wt -= dt; if (z.wt <= 0) { z.wa = frand(0, 6.28f); z.wt = frand(3, 7); } z.a = z.wa; Vector2 mv = polar(z.wa, .5f * dt); z.p = Vector2Add(z.p, mv); z.walk += .5f * dt; collide(z.p, z.r); } updateWorld(dt); }
        else if (state == ST_DEAD) { deadT += dt; updateWorld(dt); shake = std::max(0.f, shake - dt * 2); }
        // audio beds
        updateAmbience(dt);
        if (audioOk && !bedOk[BD_RAIN]) { rainSndT -= dt; windT -= dt; bool playing = state != ST_TITLE && !muted;
            if (playing && rainLevel > .3f && rainSndT <= 0) { SetSoundVolume(snd[SFX_RAIN][0], rainLevel * .6f); PlaySound(snd[SFX_RAIN][0]); rainSndT = 3.9f; }
            if (playing && isNight() && windT <= 0) { SetSoundVolume(snd[SFX_WIND][0], .35f); PlaySound(snd[SFX_WIND][0]); windT = 4.9f; } }
        // camera
        float shx = shake > 0 ? frand(-shake, shake) : 0, shz = shake > 0 ? frand(-shake, shake) : 0;
        Vector3 tgt = V3(camPos.x + shx, 0, camPos.y + shz); float dist = 27.f / camZoom;
        cam3.target = Vector3Add(tgt, V3(0, .8f, 0)); cam3.position = Vector3Add(tgt, V3(0, dist * .86f, dist * .52f)); cam3.up = V3(0, 1, 0); cam3.fovy = 42; cam3.projection = CAMERA_PERSPECTIVE;
        Matrix view = GetCameraMatrix(cam3); camRight = V3(view.m0, view.m4, view.m8); camUp = V3(view.m1, view.m5, view.m9); camFwd = Vector3Normalize(Vector3Subtract(cam3.target, cam3.position));
        computeFades(dt);
        BeginDrawing();
        setupLighting(cam3.position); renderShadows(); renderFlashShadow();
        Vector3 fogC = Vector3Lerp(V3(.55f, .58f, .6f), V3(.006f, .008f, .014f), darkK()); if (inCell(P.p) && state != ST_TITLE) fogC = V3(0, 0, 0); ClearBackground(rgba((int)(fogC.x * 255), (int)(fogC.y * 255), (int)(fogC.z * 255)));
        BeginMode3D(cam3); renderWorld3D(); EndMode3D();
        // world-space floaters
        for (auto& f : floaters) { Vector2 s = GetWorldToScreen(f.p, cam3); text(f.txt, s.x, s.y, f.size, withA(f.c, std::min(1.f, f.t / f.mt * 2)), 1); }
        DrawTexturePro(vignTex, {0, 0, 512, 512}, {-W * .08f, -H * .08f, W * 1.16f, H * 1.16f}, V2(0, 0), 0, rgba(0, 0, 0, (int)(140 + 60 * darkK())));
        if (cfg.grain) DrawTexturePro(grainTex, {(float)GetRandomValue(0, 255), (float)GetRandomValue(0, 255), (float)W / 2, (float)H / 2}, {0, 0, (float)W, (float)H}, V2(0, 0), 0, rgba(255, 255, 255, 9));
        if (state == ST_PLAY || state == ST_PAUSE || state == ST_SKILLS || state == ST_INV || state == ST_MAP || (state == ST_DEAD && deadT < 1.2f)) drawHUD();
        if (state == ST_SETTINGS) drawSettings(); else if (state == ST_TITLE) drawTitle(); else if (state == ST_SKILLS) drawSkills(); else if (state == ST_PAUSE) drawPause(); else if (state == ST_DEAD) drawDead(); else if (state == ST_INV) drawInventory(); else if (state == ST_MAP) drawMap();
        EndDrawing();
    }
    if (audioOk) CloseAudioDevice(); CloseWindow(); return 0;
}
