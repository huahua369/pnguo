// Portions Copyright 2019 Advanced Micro Devices, Inc.All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "PBRTextures.glsl"
 
#ifdef MATERIAL_IRIDESCENCE
// XYZ to sRGB color space
const mat3 XYZ_TO_REC709 = mat3(
     3.2404542, -0.9692660,  0.0556434,
    -1.5371385,  1.8760108, -0.2040259,
    -0.4985314,  0.0415560,  1.0572252
);

// Assume air interface for top
// Note: We don't handle the case fresnel0 == 1
vec3 Fresnel0ToIor(vec3 fresnel0) {
    vec3 sqrtF0 = sqrt(fresnel0);
    return (vec3(1.0) + sqrtF0) / (vec3(1.0) - sqrtF0);
}

// Conversion FO/IOR
vec3 IorToFresnel0(vec3 transmittedIor, float incidentIor) {
    return sq((transmittedIor - vec3(incidentIor)) / (transmittedIor + vec3(incidentIor)));
}

// ior is a value between 1.0 and 3.0. 1.0 is air interface
float IorToFresnel0(float transmittedIor, float incidentIor) {
    return sq((transmittedIor - incidentIor) / (transmittedIor + incidentIor));
}

// Fresnel equations for dielectric/dielectric interfaces.
// Ref: https://belcour.github.io/blog/research/2017/05/01/brdf-thin-film.html
// Evaluation XYZ sensitivity curves in Fourier space
vec3 evalSensitivity(float OPD, vec3 shift) {
    float phase = 2.0 * M_PI * OPD * 1.0e-9;
    vec3 val = vec3(5.4856e-13, 4.4201e-13, 5.2481e-13);
    vec3 pos = vec3(1.6810e+06, 1.7953e+06, 2.2084e+06);
    vec3 var = vec3(4.3278e+09, 9.3046e+09, 6.6121e+09);

    vec3 xyz = val * sqrt(2.0 * M_PI * var) * cos(pos * phase + shift) * exp(-sq(phase) * var);
    xyz.x += 9.7470e-14 * sqrt(2.0 * M_PI * 4.5282e+09) * cos(2.2399e+06 * phase + shift[0]) * exp(-4.5282e+09 * sq(phase));
    xyz /= 1.0685e-7;

    vec3 srgb = XYZ_TO_REC709 * xyz;
    return srgb;
}

vec3 evalIridescence(float outsideIOR, float eta2, float cosTheta1, float thinFilmThickness, vec3 baseF0) {
    vec3 I;

    // Force iridescenceIor -> outsideIOR when thinFilmThickness -> 0.0
    float iridescenceIor = mix(outsideIOR, eta2, smoothstep(0.0, 0.03, thinFilmThickness));
    // Evaluate the cosTheta on the base layer (Snell law)
    float sinTheta2Sq = sq(outsideIOR / iridescenceIor) * (1.0 - sq(cosTheta1));

    // Handle TIR:
    float cosTheta2Sq = 1.0 - sinTheta2Sq;
    if (cosTheta2Sq < 0.0) {
        return vec3(1.0);
    }

    float cosTheta2 = sqrt(cosTheta2Sq);

    // First interface
    float R0 = IorToFresnel0(iridescenceIor, outsideIOR);
    float R12 = F_Schlick(R0, cosTheta1);
    float R21 = R12;
    float T121 = 1.0 - R12;
    float phi12 = 0.0;
    if (iridescenceIor < outsideIOR) phi12 = M_PI;
    float phi21 = M_PI - phi12;

    // Second interface
    vec3 baseIOR = Fresnel0ToIor(clamp(baseF0, 0.0, 0.9999)); // guard against 1.0
    vec3 R1 = IorToFresnel0(baseIOR, iridescenceIor);
    vec3 R23 = F_Schlick(R1, cosTheta2);
    vec3 phi23 = vec3(0.0);
    if (baseIOR[0] < iridescenceIor) phi23[0] = M_PI;
    if (baseIOR[1] < iridescenceIor) phi23[1] = M_PI;
    if (baseIOR[2] < iridescenceIor) phi23[2] = M_PI;

    // Phase shift
    float OPD = 2.0 * iridescenceIor * thinFilmThickness * cosTheta2;
    vec3 phi = vec3(phi21) + phi23;

    // Compound terms
    vec3 R123 = clamp(R12 * R23, 1e-5, 0.9999);
    vec3 r123 = sqrt(R123);
    vec3 Rs = sq(T121) * R23 / (vec3(1.0) - R123);

    // Reflectance term for m = 0 (DC term amplitude)
    vec3 C0 = R12 + Rs;
    I = C0;

    // Reflectance term for m > 0 (pairs of diracs)
    vec3 Cm = Rs - T121;
    for (int m = 1; m <= 2; ++m)
    {
        Cm *= r123;
        vec3 Sm = 2.0 * evalSensitivity(float(m) * OPD, float(m) * phi);
        I += Cm * Sm;
    }

    // Since out of gamut colors might be produced, negative color values are clamped to 0.
    return max(I, vec3(0.0));
}

#endif 
//!MATERIAL_IRIDESCENCE



vec4 getPixelColor(VS2PS Input)
{
   vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

#ifdef ID_COLOR_0
    color.xyz = Input.Color0;
#endif

   return color;
}

// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
vec3 getPixelNormal(VS2PS Input)
{
    vec2 UV = getNormalUV(Input);

    // Retrieve the tangent space matrix
#ifndef ID_TANGENT
    vec3 pos_dx = dFdx(Input.WorldPos);
    vec3 pos_dy = dFdy(Input.WorldPos);
    vec3 tex_dx = dFdx(vec3(UV, 0.0));
    vec3 tex_dy = dFdy(vec3(UV, 0.0));
    vec3 t = (tex_dy.y * pos_dx - tex_dx.y * pos_dy) / (tex_dx.x * tex_dy.y - tex_dy.x * tex_dx.y);

#ifdef ID_NORMAL
    vec3 ng = normalize(Input.Normal);
#else
    vec3 ng = cross(pos_dx, pos_dy);
#endif

    t = normalize(t - ng * dot(ng, t));
    vec3 b = normalize(cross(ng, t));
    mat3 tbn = mat3(t, b, ng);
#else // HAS_TANGENTS
    mat3 tbn = mat3(Input.Tangent, Input.Binormal, Input.Normal);
#endif

#ifdef ID_normalTexture
    vec2 xy = 2.0 * texture(u_NormalSampler, UV, myPerFrame.u_LodBias).rg - 1.0;
    float z = sqrt(1.0 - dot(xy, xy));
    vec3 n = vec3(xy, z);
    n = normalize(tbn * (n /* * vec3(u_NormalScale, u_NormalScale, 1.0) */));
#else
    // The tbn matrix is linearly interpolated, so we need to re-normalize
    vec3 n =tbn[2];
    n = normalize(n);
#endif
    
    return n;
}

// 内部pbr用

struct PbrMaterial
{
  vec3  baseColor;  // base color
  float opacity;    // 1 = opaque, 0 = fully transparent
  vec2  roughness;  // 0 = smooth, 1 = rough (anisotropic: x = U, y = V)
  float metallic;   // 0 = dielectric, 1 = metallic
  vec3  emissive;   // emissive color

  vec3 N;   // shading normal
  vec3 T;   // shading normal
  vec3 B;   // shading normal
  vec3 Ng;  // geometric normal


  float ior1;  // index of refraction : current medium (i.e. air)
  float ior2;  // index of refraction : the other side (i.e. glass)

  float specular;       // weight of the dielectric specular layer
  vec3  specularColor;  // color of the dielectric specular layer
  float transmission;   // KHR_materials_transmission

  vec3  attenuationColor;     // KHR_materials_volume
  float attenuationDistance;  //
  float thickness;            // Replace for isThinWalled?

  float clearcoat;           // KHR_materials_clearcoat
  float clearcoatRoughness;  //
  vec3  Nc;                  // clearcoat normal

  float iridescence;
  float iridescenceIor;
  float iridescenceThickness;

  vec3  sheenColor;
  float sheenRoughness;
};
#ifdef __cplusplus
#ifndef OUT_TYPE
#define OUT_TYPE(T) T&
#endif
#else
#define OUT_TYPE(T) out T
#endif
void orthonormalBasis(vec3 normal, OUT_TYPE(vec3) tangent, OUT_TYPE(vec3) bitangent)
{
  if(normal.z < -0.99998796F)  // Handle the singularity
  {
    tangent   = vec3(0.0F, -1.0F, 0.0F);
    bitangent = vec3(-1.0F, 0.0F, 0.0F);
    return;
  }
  float a   = 1.0F / (1.0F + normal.z);
  float b   = -normal.x * normal.y * a;
  tangent   = vec3(1.0F - normal.x * normal.x * a, b, -normal.x);
  bitangent = vec3(b, 1.0f - normal.y * normal.y * a, -normal.y);
}
PbrMaterial defaultPbrMaterial()
{
  PbrMaterial mat;
  mat.baseColor = vec3(1.0F);
  mat.opacity   = 1.0F;
  mat.roughness = vec2(1.0F);
  mat.metallic  = 1.0F;
  mat.emissive  = vec3(0.0F);

  mat.N  = vec3(0.0F, 0.0F, 1.0F);
  mat.Ng = vec3(0.0F, 0.0F, 1.0F);
  mat.T  = vec3(1.0F, 0.0F, 0.0F);
  mat.B  = vec3(0.0F, 1.0F, 0.0F);

  mat.ior1 = 1.0F;
  mat.ior2 = 1.5F;

  mat.specular      = 1.0F;
  mat.specularColor = vec3(1.0F);
  mat.transmission  = 0.0F;

  mat.attenuationColor    = vec3(1.0F);
  mat.attenuationDistance = 1.0F;
  mat.thickness           = 0.0F;

  mat.clearcoat          = 0.0F;
  mat.clearcoatRoughness = 0.01F;
  mat.Nc                 = vec3(0.0F, 0.0F, 1.0F);

  mat.iridescence          = 0.0F;
  mat.iridescenceIor       = 1.5F;
  mat.iridescenceThickness = 0.1F;

  mat.sheenColor     = vec3(0.0F);
  mat.sheenRoughness = 0.0F;

  return mat;
}

PbrMaterial defaultPbrMaterial(vec3 baseColor, float metallic, float roughness, vec3 N, vec3 Ng)
{
  PbrMaterial mat = defaultPbrMaterial();
  mat.baseColor   = baseColor;
  mat.metallic    = metallic;
  mat.roughness   = vec2(roughness * roughness);
  mat.Ng          = Ng;
  mat.N           = N;
  orthonormalBasis(mat.N, mat.T, mat.B);

  return mat;
}
//------------------------------------------------------------
// PBR getters
//------------------------------------------------------------

struct PBRFactors
{
    vec4 u_EmissiveFactor;

    // pbrMetallicRoughness
    vec4 u_BaseColorFactor;
    float u_MetallicFactor;
    float u_RoughnessFactor;
    vec2 padding;

    // KHR_materials_pbrSpecularGlossiness
    vec4 u_DiffuseFactor;
    vec3 u_SpecularFactor;
    float u_GlossinessFactor;
     
};

// alphaMode
#define ALPHA_OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2

struct GltfShadeMaterial
{
    // Core
    vec4  pbrBaseColorFactor;           // offset 0 - 16 bytes
    vec3  emissiveFactor;               // offset 16 - 12 bytes
    int   pbrBaseColorTexture;          // offset 28 - 4 bytes
    int   normalTexture;                // offset 32 - 4 bytes
    float normalTextureScale;           // offset 36 - 4 bytes
    int   _pad0;                        // offset 40 - 4 bytes
    float pbrRoughnessFactor;           // offset 44 - 4 bytes
    float pbrMetallicFactor;            // offset 48 - 4 bytes
    int   pbrMetallicRoughnessTexture;  // offset 52 - 4 bytes
    int   emissiveTexture;              // offset 56 - 4 bytes
    int   alphaMode;                    // offset 60 - 4 bytes
    float alphaCutoff;                  // offset 64 - 4 bytes

    // KHR_materials_transmission
    float transmissionFactor;   // offset 68 - 4 bytes
    int   transmissionTexture;  // offset 72 - 4 bytes

    // KHR_materials_ior
    float ior;  // offset 76 - 4 bytes

    // KHR_materials_volume
    vec3  attenuationColor;     // offset 80 - 12 bytes
    float thicknessFactor;      // offset 92 - 4 bytes
    int   thicknessTexture;     // offset 96 - 4 bytes
    float attenuationDistance;  // offset 100 - 4 bytes

    // KHR_materials_clearcoat
    float clearcoatFactor;            // offset 104 - 4 bytes
    float clearcoatRoughness;         // offset 108 - 4 bytes
    int   clearcoatTexture;           // offset 112 - 4 bytes
    int   clearcoatRoughnessTexture;  // offset 116 - 4 bytes
    int   clearcoatNormalTexture;     // offset 120 - 4 bytes

    // KHR_materials_specular
    vec3  specularColorFactor;   // offset 124 - 12 bytes
    float specularFactor;        // offset 136 - 4 bytes
    int   specularTexture;       // offset 140 - 4 bytes
    int   specularColorTexture;  // offset 144 - 4 bytes

    // KHR_materials_unlit
    int unlit;  // offset 148 - 4 bytes

    // KHR_materials_iridescence
    float iridescenceFactor;            // offset 152 - 4 bytes
    int   iridescenceTexture;           // offset 156 - 4 bytes
    float iridescenceThicknessMaximum;  // offset 160 - 4 bytes
    float iridescenceThicknessMinimum;  // offset 164 - 4 bytes
    int   iridescenceThicknessTexture;  // offset 168 - 4 bytes
    float iridescenceIor;               // offset 172 - 4 bytes

    // KHR_materials_anisotropy
    float anisotropyStrength;  // offset 176 - 4 bytes
    int   anisotropyTexture;   // offset 180 - 4 bytes
    float anisotropyRotation;  // offset 184 - 4 bytes

    // KHR_texture_transform
    mat3 uvTransform;  // offset 188 - 48 bytes (mat3 occupies 3 vec4, thus 3 * 16 bytes = 48 bytes)
    vec4 _pad3;        // offset 236 - 16 bytes (to cover the 36 bytes of the mat3 (C++))

    // KHR_materials_sheen
    int   sheenColorTexture;      // offset 252 - 4 bytes
    float sheenRoughnessFactor;   // offset 256 - 4 bytes
    int   sheenRoughnessTexture;  // offset 260 - 4 bytes
    vec3  sheenColorFactor;       // offset 264 - 12 bytes
    int   _pad2;                  // offset 276 - 4 bytes (padding to align to 16 bytes)

    // Total size: 280 bytes
};




vec4 getBaseColor(VS2PS Input)
{
    vec4 baseColor = vec4(0.0, 0.0, 0.0, 1.0);
#ifdef MATERIAL_SPECULARGLOSSINESS
    baseColor = getDiffuseTexture(Input);
#endif

#ifdef MATERIAL_METALLICROUGHNESS
    // The albedo may be defined from a base texture or a flat color
    baseColor = getBaseColorTexture(Input);
#endif
    return baseColor;
}

vec4 getBaseColor(VS2PS Input, PBRFactors params)
{
    vec4 baseColor = getBaseColor(Input);

#ifdef MATERIAL_SPECULARGLOSSINESS
    baseColor *= params.u_DiffuseFactor;
#endif

#ifdef MATERIAL_METALLICROUGHNESS
    baseColor *= params.u_BaseColorFactor;
#endif

    baseColor *= getPixelColor(Input);
    return baseColor;
}

void discardPixelIfAlphaCutOff(VS2PS Input)
{
    vec4 baseColor = getBaseColor(Input);

#if defined(DEF_alphaMode_BLEND)
        if (baseColor.a == 0)
            discard;
#elif defined(DEF_alphaMode_MASK) && defined(DEF_alphaCutoff)
        if (baseColor.a < DEF_alphaCutoff)
            discard;
#else
        //OPAQUE
#endif
}

void getPBRParams(VS2PS Input, PBRFactors params, out vec3 diffuseColor, out vec3  specularColor, out float perceptualRoughness, out float alpha, out vec4 baseColor)
{
    // Metallic and Roughness material properties are packed together
    // In glTF, these factors can be specified by fixed scalar values
    // or from a metallic-roughness map
    alpha = 0.0;
    perceptualRoughness = 0.0;
    diffuseColor = vec3(0.0, 0.0, 0.0);
    specularColor = vec3(0.0, 0.0, 0.0);
    float metallic = 0.0;
    vec3 f0 = vec3(0.04, 0.04, 0.04);

    baseColor = getBaseColor(Input, params);

#ifdef MATERIAL_SPECULARGLOSSINESS
    vec4 sgSample = getSpecularGlossinessTexture(Input);
    perceptualRoughness = (1.0 - sgSample.a * params.u_GlossinessFactor); // glossiness to roughness
    f0 = sgSample.rgb * params.u_SpecularFactor; // specular

    // f0 = specular
    specularColor = f0;
    float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);
    diffuseColor = baseColor.rgb * oneMinusSpecularStrength;

#ifdef DEBUG_METALLIC
    // do conversion between metallic M-R and S-G metallic
    metallic = solveMetallic(baseColor.rgb, specularColor, oneMinusSpecularStrength);
#endif // ! DEBUG_METALLIC
#endif // ! MATERIAL_SPECULARGLOSSINESS

#ifdef MATERIAL_METALLICROUGHNESS
    // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
    // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
    vec4 mrSample = getMetallicRoughnessTexture(Input);
    perceptualRoughness = mrSample.g * params.u_RoughnessFactor;
    metallic = mrSample.b * params.u_MetallicFactor;

    diffuseColor = baseColor.rgb * (vec3(1.0, 1.0, 1.0) - f0) * (1.0 - metallic);
    specularColor = mix(f0, baseColor.rgb, metallic);
#endif // ! MATERIAL_METALLICROUGHNESS

 
    perceptualRoughness = clamp(perceptualRoughness, 0.0f, 1.0f);

    alpha = baseColor.a;
}
