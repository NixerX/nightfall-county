// Nightfall County: shaders.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
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
