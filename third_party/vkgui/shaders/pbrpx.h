#ifndef pbrpx_h_
#define pbrpx_h_


struct Light
{
	vec3 direction;
	float range;

	vec3 color;
	float intensity;

	vec3 position;
	float innerConeCos;

	float outerConeCos;
	int type;
};


const int LightType_Directional = 0;
const int LightType_Point = 1;
const int LightType_Spot = 2;
#ifndef LIGHT_COUNT
#define LIGHT_COUNT 80
#endif



struct pbr_material_tex
{
uniform int u_MipCount;
uniform mat3 u_EnvRotation;

// General Material

uniform float u_NormalScale;
uniform int u_NormalUVSet;
uniform mat3 u_NormalUVTransform;

uniform vec3 u_EmissiveFactor;
uniform int u_EmissiveUVSet;
uniform mat3 u_EmissiveUVTransform;

uniform int u_OcclusionUVSet;
uniform float u_OcclusionStrength;
uniform mat3 u_OcclusionUVTransform;

uniform int u_BaseColorUVSet;
uniform mat3 u_BaseColorUVTransform;

uniform int u_MetallicRoughnessUVSet;
uniform mat3 u_MetallicRoughnessUVTransform;

uniform int u_DiffuseUVSet;
uniform mat3 u_DiffuseUVTransform;

uniform int u_SpecularGlossinessUVSet;
uniform mat3 u_SpecularGlossinessUVTransform;

uniform int u_ClearcoatUVSet;
uniform mat3 u_ClearcoatUVTransform;

uniform int u_ClearcoatRoughnessUVSet;
uniform mat3 u_ClearcoatRoughnessUVTransform;

uniform int u_ClearcoatNormalUVSet;
uniform mat3 u_ClearcoatNormalUVTransform;
uniform float u_ClearcoatNormalScale;

uniform int u_SheenColorUVSet;
uniform mat3 u_SheenColorUVTransform;
uniform int u_SheenRoughnessUVSet;
uniform mat3 u_SheenRoughnessUVTransform;

uniform int u_SpecularUVSet;
uniform mat3 u_SpecularUVTransform;
uniform int u_SpecularColorUVSet;
uniform mat3 u_SpecularColorUVTransform;

uniform int u_TransmissionUVSet;
uniform mat3 u_TransmissionUVTransform;
uniform ivec2 u_TransmissionFramebufferSize;

uniform int u_ThicknessUVSet;
uniform mat3 u_ThicknessUVTransform;

uniform int u_IridescenceUVSet;
uniform mat3 u_IridescenceUVTransform;

uniform int u_IridescenceThicknessUVSet;
uniform mat3 u_IridescenceThicknessUVTransform;

uniform int u_DiffuseTransmissionUVSet;
uniform mat3 u_DiffuseTransmissionUVTransform;

uniform int u_DiffuseTransmissionColorUVSet;
uniform mat3 u_DiffuseTransmissionColorUVTransform;

uniform int u_AnisotropyUVSet;
uniform mat3 u_AnisotropyUVTransform;
};

#ifdef USE_PUNCTUAL
uniform Light u_Lights[LIGHT_COUNT + 1]; //Array [0] is not allowed
#endif

uniform samplerCube u_LambertianEnvSampler;//*u_EnvRotation
uniform samplerCube u_GGXEnvSampler;//*u_EnvRotation
uniform sampler2D u_GGXLUT;
uniform samplerCube u_CharlieEnvSampler;//*u_EnvRotation
uniform sampler2D u_CharlieLUT;
uniform sampler2D u_SheenELUT;

uniform sampler2D u_NormalSampler;

uniform sampler2D u_EmissiveSampler;

uniform sampler2D u_OcclusionSampler;

#ifdef ID_SSAO
layout(set = 1, binding = ID_SSAO) uniform sampler2D ssaoSampler;
#endif

#ifdef MATERIAL_METALLICROUGHNESS
uniform sampler2D u_BaseColorSampler;
uniform sampler2D u_MetallicRoughnessSampler;
#endif

#ifdef MATERIAL_SPECULARGLOSSINESS

uniform sampler2D u_DiffuseSampler;
uniform sampler2D u_SpecularGlossinessSampler;
#endif

#ifdef MATERIAL_CLEARCOAT

uniform sampler2D u_ClearcoatSampler;
uniform sampler2D u_ClearcoatRoughnessSampler;
uniform sampler2D u_ClearcoatNormalSampler;
#endif
#ifdef MATERIAL_SHEEN

uniform sampler2D u_SheenColorSampler;
uniform sampler2D u_SheenRoughnessSampler;
#endif
#ifdef MATERIAL_SPECULAR

uniform sampler2D u_SpecularSampler;
uniform sampler2D u_SpecularColorSampler;
#endif

#ifdef MATERIAL_TRANSMISSION

uniform sampler2D u_TransmissionSampler;
uniform sampler2D u_TransmissionFramebufferSampler;


#endif


// Volume Material 
#ifdef MATERIAL_VOLUME

uniform sampler2D u_ThicknessSampler;
#endif 
// Iridescence 
#ifdef MATERIAL_IRIDESCENCE

uniform sampler2D u_IridescenceSampler;
uniform sampler2D u_IridescenceThicknessSampler;
#endif


// Diffuse Transmission

#ifdef MATERIAL_DIFFUSE_TRANSMISSION

uniform sampler2D u_DiffuseTransmissionSampler;
uniform sampler2D u_DiffuseTransmissionColorSampler;
#endif

// Anisotropy

#ifdef MATERIAL_ANISOTROPY
uniform sampler2D u_AnisotropySampler;
#endif


struct pbr_param
{
	uniform mat4 u_ModelMatrix;
	uniform mat4 u_ViewMatrix;	//vp合并 uniform mat4 u_ProjectionMatrix;

	uniform float u_EnvIntensity;

	// Metallic Roughness
	uniform float u_MetallicFactor;
	uniform float u_RoughnessFactor;
	uniform vec4 u_BaseColorFactor;

	// Specular Glossiness
	uniform vec3 u_SpecularFactor;
	uniform vec4 u_DiffuseFactor;
	uniform float u_GlossinessFactor;

	// Sheen
	uniform float u_SheenRoughnessFactor;
	uniform vec3 u_SheenColorFactor;

	// Clearcoat
	uniform float u_ClearcoatFactor;
	uniform float u_ClearcoatRoughnessFactor;

	// Specular
	uniform vec3 u_KHR_materials_specular_specularColorFactor;
	uniform float u_KHR_materials_specular_specularFactor;

	// Transmission
	uniform float u_TransmissionFactor;

	// Volume
	uniform float u_ThicknessFactor;
	uniform vec3 u_AttenuationColor;
	uniform float u_AttenuationDistance;

	// Iridescence
	uniform float u_IridescenceFactor;
	uniform float u_IridescenceIor;
	uniform float u_IridescenceThicknessMinimum;
	uniform float u_IridescenceThicknessMaximum;

	// Diffuse Transmission
	uniform float u_DiffuseTransmissionFactor;
	uniform vec3 u_DiffuseTransmissionColorFactor;

	// Emissive Strength
	uniform float u_EmissiveStrength;

	// IOR
	uniform float u_Ior;

	// Anisotropy
	uniform vec3 u_Anisotropy;

	// Dispersion
	uniform float u_Dispersion;

	// Alpha mode
	uniform float u_AlphaCutoff;

	uniform vec3 u_Camera;

#ifdef MATERIAL_TRANSMISSION
	uniform ivec2 u_ScreenSize;
#endif
};

in vec4 v_Color0;
in vec2 v_texcoord_0;
in vec2 v_texcoord_1;
in vec3 v_Position;







#ifndef M_PI
const float M_PI = 3.14159265358979323846f;
#endif // !M_PI

// brdf
#if 1
//
// Fresnel
//
// http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
// https://github.com/wdas/brdf/tree/master/src/brdfs
// https://google.github.io/filament/Filament.md.html
//

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 F_Schlick(vec3 f0, vec3 f90, float VdotH)
{
	return f0 + (f90 - f0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

float F_Schlick(float f0, float f90, float VdotH)
{
	float x = clamp(1.0 - VdotH, 0.0, 1.0);
	float x2 = x * x;
	float x5 = x * x2 * x2;
	return f0 + (f90 - f0) * x5;
}

float F_Schlick(float f0, float VdotH)
{
	float f90 = 1.0; //clamp(50.0 * f0, 0.0, 1.0);
	return F_Schlick(f0, f90, VdotH);
}

vec3 F_Schlick(vec3 f0, float f90, float VdotH)
{
	float x = clamp(1.0 - VdotH, 0.0, 1.0);
	float x2 = x * x;
	float x5 = x * x2 * x2;
	return f0 + (f90 - f0) * x5;
}

vec3 F_Schlick(vec3 f0, float VdotH)
{
	float f90 = 1.0; //clamp(dot(f0, vec3(50.0 * 0.33)), 0.0, 1.0);
	return F_Schlick(f0, f90, VdotH);
}

vec3 Schlick_to_F0(vec3 f, vec3 f90, float VdotH) {
	float x = clamp(1.0 - VdotH, 0.0, 1.0);
	float x2 = x * x;
	float x5 = clamp(x * x2 * x2, 0.0, 0.9999);

	return (f - f90 * x5) / vec3(1.0 - x5);
}

float Schlick_to_F0(float f, float f90, float VdotH) {
	float x = clamp(1.0 - VdotH, 0.0, 1.0);
	float x2 = x * x;
	float x5 = clamp(x * x2 * x2, 0.0, 0.9999);

	return (f - f90 * x5) / (1.0 - x5);
}

vec3 Schlick_to_F0(vec3 f, float VdotH) {
	return Schlick_to_F0(f, vec3(1.0), VdotH);
}

float Schlick_to_F0(float f, float VdotH) {
	return Schlick_to_F0(f, 1.0, VdotH);
}


// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float V_GGX(float NdotL, float NdotV, float alphaRoughness)
{
	float alphaRoughnessSq = alphaRoughness * alphaRoughness;

	float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
	float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

	float GGX = GGXV + GGXL;
	if (GGX > 0.0)
	{
		return 0.5 / GGX;
	}
	return 0.0;
}


// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float D_GGX(float NdotH, float alphaRoughness)
{
	float alphaRoughnessSq = alphaRoughness * alphaRoughness;
	float f = (NdotH * NdotH) * (alphaRoughnessSq - 1.0) + 1.0;
	return alphaRoughnessSq / (M_PI * f * f);
}


float lambdaSheenNumericHelper(float x, float alphaG)
{
	float oneMinusAlphaSq = (1.0 - alphaG) * (1.0 - alphaG);
	float a = mix(21.5473, 25.3245, oneMinusAlphaSq);
	float b = mix(3.82987, 3.32435, oneMinusAlphaSq);
	float c = mix(0.19823, 0.16801, oneMinusAlphaSq);
	float d = mix(-1.97760, -1.27393, oneMinusAlphaSq);
	float e = mix(-4.32054, -4.85967, oneMinusAlphaSq);
	return a / (1.0 + b * pow(x, c)) + d * x + e;
}


float lambdaSheen(float cosTheta, float alphaG)
{
	if (abs(cosTheta) < 0.5)
	{
		return exp(lambdaSheenNumericHelper(cosTheta, alphaG));
	}
	else
	{
		return exp(2.0 * lambdaSheenNumericHelper(0.5, alphaG) - lambdaSheenNumericHelper(1.0 - cosTheta, alphaG));
	}
}


float V_Sheen(float NdotL, float NdotV, float sheenRoughness)
{
	sheenRoughness = max(sheenRoughness, 0.000001); //clamp (0,1]
	float alphaG = sheenRoughness * sheenRoughness;

	return clamp(1.0 / ((1.0 + lambdaSheen(NdotV, alphaG) + lambdaSheen(NdotL, alphaG)) *
		(4.0 * NdotV * NdotL)), 0.0, 1.0);
}


//Sheen implementation-------------------------------------------------------------------------------------
// See  https://github.com/sebavan/glTF/tree/KHR_materials_sheen/extensions/2.0/Khronos/KHR_materials_sheen

// Estevez and Kulla http://www.aconty.com/pdf/s2017_pbs_imageworks_sheen.pdf
float D_Charlie(float sheenRoughness, float NdotH)
{
	sheenRoughness = max(sheenRoughness, 0.000001f); //clamp (0,1]
	float alphaG = sheenRoughness * sheenRoughness;
	float invR = 1.0 / alphaG;
	float cos2h = NdotH * NdotH;
	float sin2h = 1.0 - cos2h;
	return (2.0 + invR) * pow(sin2h, invR * 0.5) / (2.0 * M_PI);
}


//https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
vec3 BRDF_lambertian(vec3 diffuseColor)
{
	// see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	diffuseColor /= M_PI; return diffuseColor;
}

//  https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
vec3 BRDF_specularGGX(float alphaRoughness, float NdotL, float NdotV, float NdotH)
{
	float Vis = V_GGX(NdotL, NdotV, alphaRoughness);
	float D = D_GGX(NdotH, alphaRoughness);

	return vec3(Vis * D);
}


#ifdef MATERIAL_ANISOTROPY
// GGX Distribution Anisotropic (Same as Babylon.js)
// https://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf Addenda
float D_GGX_anisotropic(float NdotH, float TdotH, float BdotH, float anisotropy, float at, float ab)
{
	float a2 = at * ab;
	vec3 f = vec3(ab * TdotH, at * BdotH, a2 * NdotH);
	float w2 = a2 / dot(f, f);
	return a2 * w2 * w2 / M_PI;
}

// GGX Mask/Shadowing Anisotropic (Same as Babylon.js - smithVisibility_GGXCorrelated_Anisotropic)
// Heitz http://jcgt.org/published/0003/02/03/paper.pdf
float V_GGX_anisotropic(float NdotL, float NdotV, float BdotV, float TdotV, float TdotL, float BdotL, float at, float ab)
{
	float GGXV = NdotL * length(vec3(at * TdotV, ab * BdotV, NdotV));
	float GGXL = NdotV * length(vec3(at * TdotL, ab * BdotL, NdotL));
	float v = 0.5 / (GGXV + GGXL);
	return clamp(v, 0.0, 1.0);
}

vec3 BRDF_specularGGXAnisotropy(float alphaRoughness, float anisotropy, vec3 n, vec3 v, vec3 l, vec3 h, vec3 t, vec3 b)
{
	// Roughness along the anisotropy bitangent is the material roughness, while the tangent roughness increases with anisotropy.
	float at = mix(alphaRoughness, 1.0, anisotropy * anisotropy);
	float ab = clamp(alphaRoughness, 0.001, 1.0);

	float NdotL = clamp(dot(n, l), 0.0, 1.0);
	float NdotH = clamp(dot(n, h), 0.001, 1.0);
	float NdotV = dot(n, v);

	float V = V_GGX_anisotropic(NdotL, NdotV, dot(b, v), dot(t, v), dot(t, l), dot(b, l), at, ab);
	float D = D_GGX_anisotropic(NdotH, dot(t, h), dot(b, h), anisotropy, at, ab);

	return vec3(V * D);
}
#endif


// f_sheen
vec3 BRDF_specularSheen(vec3 sheenColor, float sheenRoughness, float NdotL, float NdotV, float NdotH)
{
	float sheenDistribution = D_Charlie(sheenRoughness, NdotH);
	float sheenVisibility = V_Sheen(NdotL, NdotV, sheenRoughness);
	return sheenColor * sheenDistribution * sheenVisibility;
}

#endif // 1
// !brdf

#ifndef no_funcs

struct NormalInfo {
	vec3 ng;   // Geometry normal
	vec3 t;    // Geometry tangent
	vec3 b;    // Geometry bitangent
	vec3 n;    // Shading normal
	vec3 ntex; // Normal from texture, scaling is accounted for.
};


float clampedDot(vec3 x, vec3 y)
{
	return clamp(dot(x, y), 0.0, 1.0);
}


float max3(vec3 v)
{
	return max(max(v.x, v.y), v.z);
}


float sq(float t)
{
	return t * t;
}

vec2 sq(vec2 t)
{
	return t * t;
}

vec3 sq(vec3 t)
{
	return t * t;
}

vec4 sq(vec4 t)
{
	return t * t;
}


float applyIorToRoughness(float roughness, float ior)
{
	// Scale roughness with IOR so that an IOR of 1.0 results in no microfacet refraction and
	// an IOR of 1.5 results in the default amount of microfacet refraction.
	return roughness * clamp(ior * 2.0 - 2.0, 0.0, 1.0);
}

vec3 rgb_mix(vec3 base, vec3 layer, vec3 rgb_alpha)
{
	float rgb_alpha_max = max(rgb_alpha.r, max(rgb_alpha.g, rgb_alpha.b));
	return vec3(1.0 - rgb_alpha_max) * base + rgb_alpha * layer;
}

#endif // !no_funcs

#ifndef no_textures
// IBL



vec2 getNormalUV()
{
	vec3 uv = vec3(u_NormalUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);

#ifdef HAS_NORMAL_UV_TRANSFORM
	uv = u_NormalUVTransform * uv;
#endif

	return vec2(uv);
}


vec2 getEmissiveUV()
{
	vec3 uv = vec3(u_EmissiveUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);

#ifdef HAS_EMISSIVE_UV_TRANSFORM
	uv = u_EmissiveUVTransform * uv;
#endif

	return vec2(uv);
}


vec2 getOcclusionUV()
{
	vec3 uv = vec3(u_OcclusionUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);

#ifdef HAS_OCCLUSION_UV_TRANSFORM
	uv = u_OcclusionUVTransform * uv;
#endif

	return vec2(uv);
}


// Metallic Roughness Material


#ifdef MATERIAL_METALLICROUGHNESS


vec2 getBaseColorUV()
{
	vec3 uv = vec3(u_BaseColorUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);

#ifdef HAS_BASECOLOR_UV_TRANSFORM
	uv = u_BaseColorUVTransform * uv;
#endif

	return vec2(uv);
}

vec2 getMetallicRoughnessUV()
{
	vec3 uv = vec3(u_MetallicRoughnessUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);

#ifdef HAS_METALLICROUGHNESS_UV_TRANSFORM
	uv = u_MetallicRoughnessUVTransform * uv;
#endif

	return vec2(uv);
}

#endif


// Specular Glossiness Material


#ifdef MATERIAL_SPECULARGLOSSINESS


vec2 getSpecularGlossinessUV()
{
	vec3 uv = vec3(u_SpecularGlossinessUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);

#ifdef HAS_SPECULARGLOSSINESS_UV_TRANSFORM
	uv = u_SpecularGlossinessUVTransform * uv;
#endif

	return vec2(uv);
}

vec2 getDiffuseUV()
{
	vec3 uv = vec3(u_DiffuseUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);

#ifdef HAS_DIFFUSE_UV_TRANSFORM
	uv = u_DiffuseUVTransform * uv;
#endif

	return vec2(uv);
}

#endif


// Clearcoat Material


#ifdef MATERIAL_CLEARCOAT


vec2 getClearcoatUV()
{
	vec3 uv = vec3(u_ClearcoatUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_CLEARCOAT_UV_TRANSFORM
	uv = u_ClearcoatUVTransform * uv;
#endif
	return vec2(uv);
}

vec2 getClearcoatRoughnessUV()
{
	vec3 uv = vec3(u_ClearcoatRoughnessUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_CLEARCOATROUGHNESS_UV_TRANSFORM
	uv = u_ClearcoatRoughnessUVTransform * uv;
#endif
	return vec2(uv);
}

vec2 getClearcoatNormalUV()
{
	vec3 uv = vec3(u_ClearcoatNormalUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_CLEARCOATNORMAL_UV_TRANSFORM
	uv = u_ClearcoatNormalUVTransform * uv;
#endif
	return vec2(uv);
}

#endif


// Sheen Material


#ifdef MATERIAL_SHEEN



vec2 getSheenColorUV()
{
	vec3 uv = vec3(u_SheenColorUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_SHEENCOLOR_UV_TRANSFORM
	uv = u_SheenColorUVTransform * uv;
#endif
	return vec2(uv);
}

vec2 getSheenRoughnessUV()
{
	vec3 uv = vec3(u_SheenRoughnessUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_SHEENROUGHNESS_UV_TRANSFORM
	uv = u_SheenRoughnessUVTransform * uv;
#endif
	return vec2(uv);
}

#endif


// Specular Material


#ifdef MATERIAL_SPECULAR



vec2 getSpecularUV()
{
	vec3 uv = vec3(u_SpecularUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_SPECULAR_UV_TRANSFORM
	uv = u_SpecularUVTransform * uv;
#endif
	return vec2(uv);
}

vec2 getSpecularColorUV()
{
	vec3 uv = vec3(u_SpecularColorUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_SPECULARCOLOR_UV_TRANSFORM
	uv = u_SpecularColorUVTransform * uv;
#endif
	return vec2(uv);
}

#endif


// Transmission Material


#ifdef MATERIAL_TRANSMISSION

vec2 getTransmissionUV()
{
	vec3 uv = vec3(u_TransmissionUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_TRANSMISSION_UV_TRANSFORM
	uv = u_TransmissionUVTransform * uv;
#endif
	return vec2(uv);
}

#endif


// Volume Material


#ifdef MATERIAL_VOLUME

vec2 getThicknessUV()
{
	vec3 uv = vec3(u_ThicknessUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_THICKNESS_UV_TRANSFORM
	uv = u_ThicknessUVTransform * uv;
#endif
	return vec2(uv);
}

#endif


// Iridescence


#ifdef MATERIAL_IRIDESCENCE

vec2 getIridescenceUV()
{
	vec3 uv = vec3(u_IridescenceUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_IRIDESCENCE_UV_TRANSFORM
	uv = u_IridescenceUVTransform * uv;
#endif
	return vec2(uv);
}

vec2 getIridescenceThicknessUV()
{
	vec3 uv = vec3(u_IridescenceThicknessUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_IRIDESCENCETHICKNESS_UV_TRANSFORM
	uv = u_IridescenceThicknessUVTransform * uv;
#endif
	return vec2(uv);
}

#endif


// Diffuse Transmission

#ifdef MATERIAL_DIFFUSE_TRANSMISSION


vec2 getDiffuseTransmissionUV()
{
	vec3 uv = vec3(u_DiffuseTransmissionUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_DIFFUSE_TRANSMISSION_UV_TRANSFORM
	uv = u_DiffuseTransmissionUVTransform * uv;
#endif
	return vec2(uv);
}

vec2 getDiffuseTransmissionColorUV()
{
	vec3 uv = vec3(u_DiffuseTransmissionColorUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_DIFFUSE_TRANSMISSION_COLOR_UV_TRANSFORM
	uv = u_DiffuseTransmissionColorUVTransform * uv;
#endif
	return vec2(uv);
}

#endif

// Anisotropy

#ifdef MATERIAL_ANISOTROPY

vec2 getAnisotropyUV()
{
	vec3 uv = vec3(u_AnisotropyUVSet < 1 ? v_texcoord_0 : v_texcoord_1, 1.0);
#ifdef HAS_ANISOTROPY_UV_TRANSFORM
	uv = u_AnisotropyUVTransform * uv;
#endif
	return vec2(uv);
}

#endif

#endif // !no_textures

// 灯光
#if 1
// KHR_lights_punctual extension.
// see https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
float getRangeAttenuation(float range, float distance)
{
	if (range <= 0.0)
	{
		// negative range means unlimited
		return 1.0 / pow(distance, 2.0);
	}
	return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}


// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
float getSpotAttenuation(vec3 pointToLight, vec3 spotDirection, float outerConeCos, float innerConeCos)
{
	float actualCos = dot(normalize(spotDirection), normalize(-pointToLight));
	if (actualCos > outerConeCos)
	{
		if (actualCos < innerConeCos)
		{
			float angularAttenuation = (actualCos - outerConeCos) / (innerConeCos - outerConeCos);
			return angularAttenuation * angularAttenuation;
		}
		return 1.0;
	}
	return 0.0;
}


vec3 getLighIntensity(Light light, vec3 pointToLight)
{
	float rangeAttenuation = 1.0;
	float spotAttenuation = 1.0;

	if (light.type != LightType_Directional)
	{
		rangeAttenuation = getRangeAttenuation(light.range, length(pointToLight));
	}
	if (light.type == LightType_Spot)
	{
		spotAttenuation = getSpotAttenuation(pointToLight, light.direction, light.outerConeCos, light.innerConeCos);
	}

	return rangeAttenuation * spotAttenuation * light.intensity * light.color;
}


vec3 getPunctualRadianceTransmission(vec3 normal, vec3 view, vec3 pointToLight, float alphaRoughness,
	vec3 f0, vec3 f90, vec3 baseColor, float ior)
{
	float transmissionRougness = applyIorToRoughness(alphaRoughness, ior);

	vec3 n = normalize(normal);           // Outward direction of surface point
	vec3 v = normalize(view);             // Direction from surface point to view
	vec3 l = normalize(pointToLight);
	vec3 l_mirror = normalize(l + vec3(2.0) * n * dot(-l, n));     // Mirror light reflection vector on surface
	vec3 h = normalize(l_mirror + v);            // Halfway vector between transmission light vector and v

	float D = D_GGX(clamp(dot(n, h), 0.0, 1.0), transmissionRougness);
	vec3 F = F_Schlick(f0, f90, clamp(dot(v, h), 0.0, 1.0));
	float Vis = V_GGX(clamp(dot(n, l_mirror), 0.0, 1.0), clamp(dot(n, v), 0.0, 1.0), transmissionRougness);

	// Transmission BTDF
	return (vec3(1.0) - F) * baseColor * D * Vis;
}


vec3 getPunctualRadianceClearCoat(vec3 clearcoatNormal, vec3 v, vec3 l, vec3 h, float VdotH, vec3 f0, vec3 f90, float clearcoatRoughness)
{
	float NdotL = clampedDot(clearcoatNormal, l);
	float NdotV = clampedDot(clearcoatNormal, v);
	float NdotH = clampedDot(clearcoatNormal, h);
	return NdotL * BRDF_specularGGX(clearcoatRoughness * clearcoatRoughness, NdotL, NdotV, NdotH);
}


vec3 getPunctualRadianceSheen(vec3 sheenColor, float sheenRoughness, float NdotL, float NdotV, float NdotH)
{
	return NdotL * BRDF_specularSheen(sheenColor, sheenRoughness, NdotL, NdotV, NdotH);
}


// Compute attenuated light as it travels through a volume.
vec3 applyVolumeAttenuation(vec3 radiance, float transmissionDistance, vec3 attenuationColor, float attenuationDistance)
{
	if (attenuationDistance == 0.0)
	{
		// Attenuation distance is +∞ (which we indicate by zero), i.e. the transmitted color is not attenuated at all.
		return radiance;
	}
	else
	{
		// Compute light attenuation using Beer's law.
		vec3 transmittance = pow(attenuationColor, vec3(transmissionDistance / attenuationDistance));
		return transmittance * radiance;
	}
}


vec3 getVolumeTransmissionRay(vec3 n, vec3 v, float thickness, float ior, mat4 modelMatrix)
{
	// Direction of refracted light.
	vec3 refractionVector = refract(-v, normalize(n), 1.0 / ior);

	// Compute rotation-independant scaling of the model matrix.
	vec3 modelScale;
	modelScale.x = length(vec3(modelMatrix[0]));
	modelScale.y = length(vec3(modelMatrix[1]));
	modelScale.z = length(vec3(modelMatrix[2]));

	// The thickness is specified in local space.
	return normalize(refractionVector) * thickness * modelScale;
}

#endif // 1


#ifndef no_ibl
vec3 getDiffuseLight(vec3 n)
{
	vec4 textureSample = texture(u_LambertianEnvSampler, u_EnvRotation * n);
#ifndef __cplusplus
	textureSample.rgb *= u_EnvIntensity;
#endif // __cplusplus
	return vec3(textureSample);
}


vec4 getSpecularSample(vec3 reflection, float lod)
{
	vec4 textureSample = textureLod(u_GGXEnvSampler, u_EnvRotation * reflection, lod);
#ifndef __cplusplus
	textureSample.rgb *= u_EnvIntensity;
#endif // __cplusplus
	return textureSample;
}


vec4 getSheenSample(vec3 reflection, float lod)
{
	vec4 textureSample = textureLod(u_CharlieEnvSampler, u_EnvRotation * reflection, lod);
#ifndef __cplusplus
	textureSample.rgb *= u_EnvIntensity;
#endif // __cplusplus
	return textureSample;
}

vec3 getIBLGGXFresnel(vec3 n, vec3 v, float roughness, vec3 F0, float specularWeight)
{
	// see https://bruop.github.io/ibl/#single_scattering_results at Single Scattering Results
	// Roughness dependent fresnel, from Fdez-Aguera
	float NdotV = clampedDot(n, v);
	vec2 brdfSamplePoint = clamp(vec2(NdotV, roughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
	vec2 f_ab = vec2(texture(u_GGXLUT, brdfSamplePoint));// .rg;
	vec3 Fr = max(vec3(1.0 - roughness), F0) - F0;
	vec3 k_S = F0 + Fr * pow(1.0 - NdotV, 5.0);
	vec3 FssEss = specularWeight * (k_S * f_ab.x + f_ab.y);

	// Multiple scattering, from Fdez-Aguera
	float Ems = (1.0 - (f_ab.x + f_ab.y));
	vec3 F_avg = specularWeight * (F0 + (vec3(1.0) - F0) / 21.0f);
	vec3 FmsEms = Ems * FssEss * F_avg / (1.0f - F_avg * Ems);

	return FssEss + FmsEms;
}

vec3 getIBLRadianceGGX(vec3 n, vec3 v, float roughness)
{
	float NdotV = clampedDot(n, v);
	float lod = roughness * float(u_MipCount - 1);
	vec3 reflection = normalize(reflect(-v, n));
	vec4 specularSample = getSpecularSample(reflection, lod);

	//vec3 specularLight = specularSample.rgb;

	return vec3(specularSample);
}


#ifdef MATERIAL_TRANSMISSION
vec3 getTransmissionSample(vec2 fragCoord, float roughness, float ior)
{
	float framebufferLod = log2(float(u_TransmissionFramebufferSize.x)) * applyIorToRoughness(roughness, ior);
	vec3 transmittedLight = vec3(textureLod(u_TransmissionFramebufferSampler, fragCoord, framebufferLod));

	return transmittedLight;
}
#endif


#ifdef MATERIAL_TRANSMISSION
vec3 getIBLVolumeRefraction(vec3 n, vec3 v, float perceptualRoughness, vec3 baseColor, vec3 f0, vec3 f90,
	vec3 position, mat4 modelMatrix, mat4 viewMatrix, mat4 projMatrix, float ior, float thickness, vec3 attenuationColor, float attenuationDistance, float dispersion)
{
#ifdef MATERIAL_DISPERSION
	// Dispersion will spread out the ior values for each r,g,b channel
	float halfSpread = (ior - 1.0) * 0.025 * dispersion;
	vec3 iors = vec3(ior - halfSpread, ior, ior + halfSpread);

	vec3 transmittedLight;
	float transmissionRayLength;
	for (int i = 0; i < 3; i++)
	{
		vec3 transmissionRay = getVolumeTransmissionRay(n, v, thickness, iors[i], modelMatrix);
		// TODO: taking length of blue ray, ideally we would take the length of the green ray. For now overwriting seems ok
		transmissionRayLength = length(transmissionRay);
		vec3 refractedRayExit = position + transmissionRay;

		// Project refracted vector on the framebuffer, while mapping to normalized device coordinates.
		vec4 ndcPos = projMatrix * viewMatrix * vec4(refractedRayExit, 1.0);
		vec2 refractionCoords = ndcPos.xy / ndcPos.w;
		refractionCoords += 1.0;
		refractionCoords /= 2.0;

		// Sample framebuffer to get pixel the refracted ray hits for this color channel.
		transmittedLight[i] = getTransmissionSample(refractionCoords, perceptualRoughness, iors[i])[i];
	}
#else
	vec3 transmissionRay = getVolumeTransmissionRay(n, v, thickness, ior, modelMatrix);
	float transmissionRayLength = length(transmissionRay);
	vec3 refractedRayExit = position + transmissionRay;

	// Project refracted vector on the framebuffer, while mapping to normalized device coordinates.
	vec4 ndcPos = projMatrix * viewMatrix * vec4(refractedRayExit, 1.0);
	vec2 refractionCoords = vec2(ndcPos) / ndcPos.w;
	refractionCoords += 1.0;
	refractionCoords /= 2.0;

	// Sample framebuffer to get pixel the refracted ray hits.
	vec3 transmittedLight = getTransmissionSample(refractionCoords, perceptualRoughness, ior);

#endif // MATERIAL_DISPERSION
	vec3 attenuatedColor = applyVolumeAttenuation(transmittedLight, transmissionRayLength, attenuationColor, attenuationDistance);

	// Sample GGX LUT to get the specular component.
	float NdotV = clampedDot(n, v);
	vec2 brdfSamplePoint = clamp(vec2(NdotV, perceptualRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
	vec2 brdf = vec2(texture(u_GGXLUT, brdfSamplePoint));// .rg;
	vec3 specularColor = f0 * brdf.x + f90 * brdf.y;

	return (1.0f - specularColor) * attenuatedColor * baseColor;
}
#endif


#ifdef MATERIAL_ANISOTROPY
vec3 getIBLRadianceAnisotropy(vec3 n, vec3 v, float roughness, float anisotropy, vec3 anisotropyDirection)
{
	float NdotV = clampedDot(n, v);

	float tangentRoughness = mix(roughness, 1.0, anisotropy * anisotropy);
	vec3  anisotropicTangent = cross(anisotropyDirection, v);
	vec3  anisotropicNormal = cross(anisotropicTangent, anisotropyDirection);
	float bendFactor = 1.0 - anisotropy * (1.0 - roughness);
	float bendFactorPow4 = bendFactor * bendFactor * bendFactor * bendFactor;
	vec3  bentNormal = normalize(mix(anisotropicNormal, n, bendFactorPow4));

	float lod = roughness * float(u_MipCount - 1);
	vec3 reflection = normalize(reflect(-v, bentNormal));

	vec4 specularSample = getSpecularSample(reflection, lod);

	vec3 specularLight = specularSample.rgb;

	return specularLight;
}
#endif

vec3 getIBLRadianceCharlie(vec3 n, vec3 v, float sheenRoughness, vec3 sheenColor)
{
	float NdotV = clampedDot(n, v);
	float lod = sheenRoughness * float(u_MipCount - 1);
	vec3 reflection = normalize(reflect(-v, n));

	vec2 brdfSamplePoint = clamp(vec2(NdotV, sheenRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
	float brdf = texture(u_CharlieLUT, brdfSamplePoint).b;
	vec4 sheenSample = getSheenSample(reflection, lod);

	vec3 sheenLight = vec3(sheenSample);
	return sheenLight * sheenColor * brdf;
}

#endif // !no_ibl
#ifndef no_iridescence
// XYZ to sRGB color space
const mat3 XYZ_TO_REC709 = mat3(
	3.2404542, -0.9692660, 0.0556434,
	-1.5371385, 1.8760108, -0.2040259,
	-0.4985314, 0.0415560, 1.0572252
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

	vec3 xyz = val * sqrt(2.0f * M_PI * var) * cos(pos * phase + shift) * exp(-sq(phase) * var);
	xyz.x += 9.7470e-14 * sqrt(2.0 * M_PI * 4.5282e+09) * cos(2.2399e+06 * phase + shift[0]) * exp(-4.5282e+09 * sq(phase));
	xyz /= 1.0685e-7;

	vec3 srgb = XYZ_TO_REC709 * xyz;
	return srgb;
}

vec3 evalIridescence(float outsideIOR, float eta2, float cosTheta1, float thinFilmThickness, vec3 baseF0) {
	vec3 I;

	// Force iridescenceIor -> outsideIOR when thinFilmThickness -> 0.0
	float iridescenceIor = mix(outsideIOR, eta2, smoothstep(0.0f, 0.03f, thinFilmThickness));
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
		vec3 Sm = 2.0f * evalSensitivity(float(m) * OPD, float(m) * phi);
		I += Cm * Sm;
	}

	// Since out of gamut colors might be produced, negative color values are clamped to 0.
	return max(I, vec3(0.0));
}

#endif // !no_iridescence
// material_info
#if 1


struct MaterialInfo
{
	float ior;
	float perceptualRoughness;      // roughness value, as authored by the model creator (input to shader)
	vec3 f0_dielectric;

	float alphaRoughness;           // roughness mapped to a more linear change in the roughness (proposed by [2])

	float fresnel_w;

	vec3 f90;                       // reflectance color at grazing angle
	vec3 f90_dielectric;
	float metallic;

	vec3 baseColor;

	float sheenRoughnessFactor;
	vec3 sheenColorFactor;

	vec3 clearcoatF0;
	vec3 clearcoatF90;
	float clearcoatFactor;
	vec3 clearcoatNormal;
	float clearcoatRoughness;

	// KHR_materials_specular 
	float specularWeight; // product of specularFactor and specularTexture.a

	float transmissionFactor;

	float thickness;
	vec3 attenuationColor;
	float attenuationDistance;

	// KHR_materials_iridescence
	float iridescenceFactor;
	float iridescenceIor;
	float iridescenceThickness;

	float diffuseTransmissionFactor;
	vec3 diffuseTransmissionColorFactor;

	// KHR_materials_anisotropy
	vec3 anisotropicT;
	vec3 anisotropicB;
	float anisotropyStrength;

	// KHR_materials_dispersion
	float dispersion;
};


// Get normal, tangent and bitangent vectors.
NormalInfo getNormalInfo(vec3 v)
{
	vec2 UV = getNormalUV();
	vec2 uv_dx = dFdx(UV);
	vec2 uv_dy = dFdy(UV);

	if (length(uv_dx) <= 1e-2) {
		uv_dx = vec2(1.0, 0.0);
	}

	if (length(uv_dy) <= 1e-2) {
		uv_dy = vec2(0.0, 1.0);
	}

	vec3 t_ = (uv_dy.t * dFdx(v_Position) - uv_dx.t * dFdy(v_Position)) /
		(uv_dx.s * uv_dy.t - uv_dy.s * uv_dx.t);

	vec3 n, t, b, ng;

	// Compute geometrical TBN:
#ifdef HAS_NORMAL_VEC3
#ifdef HAS_TANGENT_VEC4
	// Trivial TBN computation, present as vertex attribute.
	// Normalize eigenvectors as matrix is linearly interpolated.
	t = normalize(v_TBN[0]);
	b = normalize(v_TBN[1]);
	ng = normalize(v_TBN[2]);
#else
	// Normals are either present as vertex attributes or approximated.
	ng = normalize(v_Normal);
	t = normalize(t_ - ng * dot(ng, t_));
	b = cross(ng, t);
#endif
#else
	ng = normalize(cross(dFdx(v_Position), dFdy(v_Position)));
	t = normalize(t_ - ng * dot(ng, t_));
	b = cross(ng, t);
#endif


	// For a back-facing surface, the tangential basis vectors are negated.
	if (gl_FrontFacing == false)
	{
		t *= -1.0;
		b *= -1.0;
		ng *= -1.0;
	}

	// Compute normals:
	NormalInfo info;
	info.ng = ng;
#ifdef HAS_NORMAL_MAP
	info.ntex = texture(u_NormalSampler, UV).rgb * 2.0 - vec3(1.0);
	info.ntex *= vec3(u_NormalScale, u_NormalScale, 1.0);
	info.ntex = normalize(info.ntex);
	info.n = normalize(mat3(t, b, ng) * info.ntex);
#else
	info.n = ng;
#endif
	info.t = t;
	info.b = b;
	return info;
}


#ifdef MATERIAL_CLEARCOAT
vec3 getClearcoatNormal(NormalInfo normalInfo)
{
#ifdef HAS_CLEARCOAT_NORMAL_MAP
	vec3 n = texture(u_ClearcoatNormalSampler, getClearcoatNormalUV()).rgb * 2.0 - vec3(1.0);
	n *= vec3(u_ClearcoatNormalScale, u_ClearcoatNormalScale, 1.0);
	n = mat3(normalInfo.t, normalInfo.b, normalInfo.ng) * normalize(n);
	return n;
#else
	return normalInfo.ng;
#endif
}
#endif

vec4 getVertexColor()
{
	vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

#ifdef ID_4PS_COLOR_0
	color = v_Color0;
#endif

	return color;
}

vec4 getBaseColor()
{
	vec4 baseColor = vec4(1);

#if defined(MATERIAL_SPECULARGLOSSINESS)
	baseColor = u_DiffuseFactor;
#elif defined(MATERIAL_METALLICROUGHNESS)
	baseColor = u_BaseColorFactor;
#endif

#if defined(MATERIAL_SPECULARGLOSSINESS) && defined(HAS_DIFFUSE_MAP)
	baseColor *= texture(u_DiffuseSampler, getDiffuseUV());
#elif defined(MATERIAL_METALLICROUGHNESS) && defined(HAS_BASE_COLOR_MAP)
	baseColor *= texture(u_BaseColorSampler, getBaseColorUV());
#endif

	return baseColor * getVertexColor();
}


#ifdef MATERIAL_SPECULARGLOSSINESS
MaterialInfo getSpecularGlossinessInfo(MaterialInfo info)
{
	info.f0_dielectric = u_SpecularFactor;
	info.perceptualRoughness = u_GlossinessFactor;

#ifdef HAS_SPECULAR_GLOSSINESS_MAP
	vec4 sgSample = texture(u_SpecularGlossinessSampler, getSpecularGlossinessUV());
	info.perceptualRoughness *= sgSample.a; // glossiness to roughness
	info.f0_dielectric *= sgSample.rgb; // specular
#endif // ! HAS_SPECULAR_GLOSSINESS_MAP

	info.perceptualRoughness = 1.0 - info.perceptualRoughness; // 1 - glossiness
	return info;
}
#endif


#ifdef MATERIAL_METALLICROUGHNESS
MaterialInfo getMetallicRoughnessInfo(MaterialInfo info)
{
	info.metallic = u_MetallicFactor;
	info.perceptualRoughness = u_RoughnessFactor;

#ifdef HAS_METALLIC_ROUGHNESS_MAP
	// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
	// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
	vec4 mrSample = texture(u_MetallicRoughnessSampler, getMetallicRoughnessUV());
	info.perceptualRoughness *= mrSample.g;
	info.metallic *= mrSample.b;
#endif

	return info;
}
#endif


#ifdef MATERIAL_SHEEN
MaterialInfo getSheenInfo(MaterialInfo info)
{
	info.sheenColorFactor = u_SheenColorFactor;
	info.sheenRoughnessFactor = u_SheenRoughnessFactor;

#ifdef HAS_SHEEN_COLOR_MAP
	vec4 sheenColorSample = texture(u_SheenColorSampler, getSheenColorUV());
	info.sheenColorFactor *= sheenColorSample.rgb;
#endif

#ifdef HAS_SHEEN_ROUGHNESS_MAP
	vec4 sheenRoughnessSample = texture(u_SheenRoughnessSampler, getSheenRoughnessUV());
	info.sheenRoughnessFactor *= sheenRoughnessSample.a;
#endif
	return info;
}
#endif


#ifdef MATERIAL_SPECULAR
MaterialInfo getSpecularInfo(MaterialInfo info)
{
	vec4 specularTexture = vec4(1.0);
#ifdef HAS_SPECULAR_MAP
	specularTexture.a = texture(u_SpecularSampler, getSpecularUV()).a;
#endif
#ifdef HAS_SPECULAR_COLOR_MAP
	specularTexture.rgb = texture(u_SpecularColorSampler, getSpecularColorUV()).rgb;
#endif

	info.f0_dielectric = min(info.f0_dielectric * u_KHR_materials_specular_specularColorFactor * specularTexture.rgb, vec3(1.0));
	info.specularWeight = u_KHR_materials_specular_specularFactor * specularTexture.a;
	info.f90_dielectric = vec3(info.specularWeight);
	return info;
}
#endif


#ifdef MATERIAL_TRANSMISSION
MaterialInfo getTransmissionInfo(MaterialInfo info)
{
	info.transmissionFactor = u_TransmissionFactor;

#ifdef HAS_TRANSMISSION_MAP
	vec4 transmissionSample = texture(u_TransmissionSampler, getTransmissionUV());
	info.transmissionFactor *= transmissionSample.r;
#endif

#ifdef MATERIAL_DISPERSION
	info.dispersion = u_Dispersion;
#else
	info.dispersion = 0.0;
#endif
	return info;
}
#endif

#ifdef MATERIAL_VOLUME
MaterialInfo getVolumeInfo(MaterialInfo info)
{
	info.thickness = u_ThicknessFactor;
	info.attenuationColor = u_AttenuationColor;
	info.attenuationDistance = u_AttenuationDistance;

#ifdef HAS_THICKNESS_MAP
	vec4 thicknessSample = texture(u_ThicknessSampler, getThicknessUV());
	info.thickness *= thicknessSample.g;
#endif
	return info;
}
#endif


#ifdef MATERIAL_IRIDESCENCE
MaterialInfo getIridescenceInfo(MaterialInfo info)
{
	info.iridescenceFactor = u_IridescenceFactor;
	info.iridescenceIor = u_IridescenceIor;
	info.iridescenceThickness = u_IridescenceThicknessMaximum;

#ifdef HAS_IRIDESCENCE_MAP
	info.iridescenceFactor *= texture(u_IridescenceSampler, getIridescenceUV()).r;
#endif

#ifdef HAS_IRIDESCENCE_THICKNESS_MAP
	float thicknessSampled = texture(u_IridescenceThicknessSampler, getIridescenceThicknessUV()).g;
	float thickness = mix(u_IridescenceThicknessMinimum, u_IridescenceThicknessMaximum, thicknessSampled);
	info.iridescenceThickness = thickness;
#endif

	return info;
}
#endif


#ifdef MATERIAL_DIFFUSE_TRANSMISSION
MaterialInfo getDiffuseTransmissionInfo(MaterialInfo info)
{
	info.diffuseTransmissionFactor = u_DiffuseTransmissionFactor;
	info.diffuseTransmissionColorFactor = u_DiffuseTransmissionColorFactor;

#ifdef HAS_DIFFUSE_TRANSMISSION_MAP
	info.diffuseTransmissionFactor *= texture(u_DiffuseTransmissionSampler, getDiffuseTransmissionUV()).a;
#endif

#ifdef HAS_DIFFUSE_TRANSMISSION_COLOR_MAP
	info.diffuseTransmissionColorFactor *= texture(u_DiffuseTransmissionColorSampler, getDiffuseTransmissionColorUV()).rgb;
#endif

	return info;
}
#endif


#ifdef MATERIAL_CLEARCOAT
MaterialInfo getClearCoatInfo(MaterialInfo info, NormalInfo normalInfo)
{
	info.clearcoatFactor = u_ClearcoatFactor;
	info.clearcoatRoughness = u_ClearcoatRoughnessFactor;
	info.clearcoatF0 = vec3(pow((info.ior - 1.0) / (info.ior + 1.0), 2.0));
	info.clearcoatF90 = vec3(1.0);

#ifdef HAS_CLEARCOAT_MAP
	vec4 clearcoatSample = texture(u_ClearcoatSampler, getClearcoatUV());
	info.clearcoatFactor *= clearcoatSample.r;
#endif

#ifdef HAS_CLEARCOAT_ROUGHNESS_MAP
	vec4 clearcoatSampleRoughness = texture(u_ClearcoatRoughnessSampler, getClearcoatRoughnessUV());
	info.clearcoatRoughness *= clearcoatSampleRoughness.g;
#endif

	info.clearcoatNormal = getClearcoatNormal(normalInfo);
	info.clearcoatRoughness = clamp(info.clearcoatRoughness, 0.0, 1.0);
	return info;
}
#endif


#ifdef MATERIAL_IOR
MaterialInfo getIorInfo(MaterialInfo info)
{
	info.f0_dielectric = vec3(pow((u_Ior - 1.0) / (u_Ior + 1.0), 2.0));
	info.ior = u_Ior;
	return info;
}
#endif

#ifdef MATERIAL_ANISOTROPY
MaterialInfo getAnisotropyInfo(MaterialInfo info, NormalInfo normalInfo)
{
	vec2 direction = vec2(1.0, 0.0);
	float strengthFactor = 1.0;
#ifdef HAS_ANISOTROPY_MAP
	vec3 anisotropySample = texture(u_AnisotropySampler, getAnisotropyUV()).xyz;
	direction = anisotropySample.xy * 2.0 - vec2(1.0);
	strengthFactor = anisotropySample.z;
#endif
	vec2 directionRotation = u_Anisotropy.xy; // cos(theta), sin(theta)
	mat2 rotationMatrix = mat2(directionRotation.x, directionRotation.y, -directionRotation.y, directionRotation.x);
	direction = rotationMatrix * direction.xy;

	info.anisotropicT = mat3(normalInfo.t, normalInfo.b, normalInfo.n) * normalize(vec3(direction, 0.0));
	info.anisotropicB = cross(normalInfo.ng, info.anisotropicT);
	info.anisotropyStrength = clamp(u_Anisotropy.z * strengthFactor, 0.0, 1.0);
	return info;
}
#endif


float albedoSheenScalingLUT(float NdotV, float sheenRoughnessFactor)
{
	return texture(u_SheenELUT, vec2(NdotV, sheenRoughnessFactor)).r;
}

#endif // 1

#ifdef pbr_glsl



vec4 pbr_main()
{
	vec4 finalColor = vec4(0.0);
	vec4 baseColor = getBaseColor();

#if ALPHAMODE == ALPHAMODE_OPAQUE
	baseColor.a = 1.0;
#endif
	vec3 color = vec3(0);

	vec3 v = normalize(u_Camera - v_Position);
	NormalInfo normalInfo = getNormalInfo(v);
	vec3 n = normalInfo.n;
	vec3 t = normalInfo.t;
	vec3 b = normalInfo.b;

	float NdotV = clampedDot(n, v);
	float TdotV = clampedDot(t, v);
	float BdotV = clampedDot(b, v);

	MaterialInfo materialInfo;
	materialInfo.baseColor = baseColor.rgb;

	// The default index of refraction of 1.5 yields a dielectric normal incidence reflectance of 0.04.
	materialInfo.ior = 1.5;
	materialInfo.f0_dielectric = vec3(0.04);
	materialInfo.specularWeight = 1.0;

	// Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
	materialInfo.f90 = vec3(1.0);
	materialInfo.f90_dielectric = materialInfo.f90;

	// If the MR debug output is selected, we have to enforce evaluation of the non-iridescence BRDF functions.
#if DEBUG == DEBUG_METALLIC_ROUGHNESS
#undef MATERIAL_IRIDESCENCE
#endif

#ifdef MATERIAL_IOR
	materialInfo = getIorInfo(materialInfo);
#endif

#ifdef MATERIAL_SPECULARGLOSSINESS
	materialInfo = getSpecularGlossinessInfo(materialInfo);
#endif

#ifdef MATERIAL_METALLICROUGHNESS
	materialInfo = getMetallicRoughnessInfo(materialInfo);
#endif

#ifdef MATERIAL_SHEEN
	materialInfo = getSheenInfo(materialInfo);
#endif

#ifdef MATERIAL_CLEARCOAT
	materialInfo = getClearCoatInfo(materialInfo, normalInfo);
#endif

#ifdef MATERIAL_SPECULAR
	materialInfo = getSpecularInfo(materialInfo);
#endif

#ifdef MATERIAL_TRANSMISSION
	materialInfo = getTransmissionInfo(materialInfo);
#endif

#ifdef MATERIAL_VOLUME
	materialInfo = getVolumeInfo(materialInfo);
#endif

#ifdef MATERIAL_IRIDESCENCE
	materialInfo = getIridescenceInfo(materialInfo);
#endif

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
	materialInfo = getDiffuseTransmissionInfo(materialInfo);
#endif

#ifdef MATERIAL_ANISOTROPY
	materialInfo = getAnisotropyInfo(materialInfo, normalInfo);
#endif

	materialInfo.perceptualRoughness = clamp(materialInfo.perceptualRoughness, 0.0, 1.0);
	materialInfo.metallic = clamp(materialInfo.metallic, 0.0, 1.0);

	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness.
	materialInfo.alphaRoughness = materialInfo.perceptualRoughness * materialInfo.perceptualRoughness;


	// LIGHTING
	vec3 f_specular_dielectric = vec3(0.0);
	vec3 f_specular_metal = vec3(0.0);
	vec3 f_diffuse = vec3(0.0);
	vec3 f_dielectric_brdf_ibl = vec3(0.0);
	vec3 f_metal_brdf_ibl = vec3(0.0);
	vec3 f_emissive = vec3(0.0);
	vec3 clearcoat_brdf = vec3(0.0);
	vec3 f_sheen = vec3(0.0);
	vec3 f_specular_transmission = vec3(0.0);
	vec3 f_diffuse_transmission = vec3(0.0);

	float clearcoatFactor = 0.0;
	vec3 clearcoatFresnel = vec3(0);

	float albedoSheenScaling = 1.0;
	float diffuseTransmissionThickness = 1.0;

#ifdef MATERIAL_IRIDESCENCE
	vec3 iridescenceFresnel_dielectric = evalIridescence(1.0, materialInfo.iridescenceIor, NdotV, materialInfo.iridescenceThickness, materialInfo.f0_dielectric);
	vec3 iridescenceFresnel_metallic = evalIridescence(1.0, materialInfo.iridescenceIor, NdotV, materialInfo.iridescenceThickness, baseColor.rgb);

	if (materialInfo.iridescenceThickness == 0.0) {
		materialInfo.iridescenceFactor = 0.0;
	}
#endif

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
#ifdef MATERIAL_VOLUME
	diffuseTransmissionThickness = materialInfo.thickness *
		(length(vec3(u_ModelMatrix[0].xyz)) + length(vec3(u_ModelMatrix[1].xyz)) + length(vec3(u_ModelMatrix[2].xyz))) / 3.0;
#endif
#endif

#ifdef MATERIAL_CLEARCOAT
	clearcoatFactor = materialInfo.clearcoatFactor;
	clearcoatFresnel = F_Schlick(materialInfo.clearcoatF0, materialInfo.clearcoatF90, clampedDot(materialInfo.clearcoatNormal, v));
#endif

	// Calculate lighting contribution from image based lighting source (IBL)
#ifdef USE_IBL

	f_diffuse = getDiffuseLight(n) * baseColor.rgb;

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
	vec3 diffuseTransmissionIBL = getDiffuseLight(-n) * materialInfo.diffuseTransmissionColorFactor;
#ifdef MATERIAL_VOLUME
	diffuseTransmissionIBL = applyVolumeAttenuation(diffuseTransmissionIBL, diffuseTransmissionThickness, materialInfo.attenuationColor, materialInfo.attenuationDistance);
#endif
	f_diffuse = mix(f_diffuse, diffuseTransmissionIBL, materialInfo.diffuseTransmissionFactor);
#endif


#if defined(MATERIAL_TRANSMISSION)
	f_specular_transmission = getIBLVolumeRefraction(
		n, v,
		materialInfo.perceptualRoughness,
		baseColor.rgb, materialInfo.f0_dielectric, materialInfo.f90,
		v_Position, u_ModelMatrix, u_ViewMatrix, u_ProjectionMatrix,
		materialInfo.ior, materialInfo.thickness, materialInfo.attenuationColor, materialInfo.attenuationDistance, materialInfo.dispersion);
	f_diffuse = mix(f_diffuse, f_specular_transmission, materialInfo.transmissionFactor);
#endif

#ifdef MATERIAL_ANISOTROPY
	f_specular_metal = getIBLRadianceAnisotropy(n, v, materialInfo.perceptualRoughness, materialInfo.anisotropyStrength, materialInfo.anisotropicB);
	f_specular_dielectric = f_specular_metal;
#else
	f_specular_metal = getIBLRadianceGGX(n, v, materialInfo.perceptualRoughness);
	f_specular_dielectric = f_specular_metal;
#endif

	// Calculate fresnel mix for IBL  

	vec3 f_metal_fresnel_ibl = getIBLGGXFresnel(n, v, materialInfo.perceptualRoughness, baseColor.rgb, 1.0);
	f_metal_brdf_ibl = f_metal_fresnel_ibl * f_specular_metal;

	vec3 f_dielectric_fresnel_ibl = getIBLGGXFresnel(n, v, materialInfo.perceptualRoughness, materialInfo.f0_dielectric, materialInfo.specularWeight);
	f_dielectric_brdf_ibl = mix(f_diffuse, f_specular_dielectric, f_dielectric_fresnel_ibl);

#ifdef MATERIAL_IRIDESCENCE
	f_metal_brdf_ibl = mix(f_metal_brdf_ibl, f_specular_metal * iridescenceFresnel_metallic, materialInfo.iridescenceFactor);
	f_dielectric_brdf_ibl = mix(f_dielectric_brdf_ibl, rgb_mix(f_diffuse, f_specular_dielectric, iridescenceFresnel_dielectric), materialInfo.iridescenceFactor);
#endif


#ifdef MATERIAL_CLEARCOAT
	clearcoat_brdf = getIBLRadianceGGX(materialInfo.clearcoatNormal, v, materialInfo.clearcoatRoughness);
#endif

#ifdef MATERIAL_SHEEN
	f_sheen = getIBLRadianceCharlie(n, v, materialInfo.sheenRoughnessFactor, materialInfo.sheenColorFactor);
	albedoSheenScaling = 1.0 - max3(materialInfo.sheenColorFactor) * albedoSheenScalingLUT(NdotV, materialInfo.sheenRoughnessFactor);
#endif

	color = mix(f_dielectric_brdf_ibl, f_metal_brdf_ibl, materialInfo.metallic);
	color = f_sheen + color * albedoSheenScaling;
	color = mix(color, clearcoat_brdf, clearcoatFactor * clearcoatFresnel);

#ifdef HAS_OCCLUSION_MAP
	float ao = 1.0;
	ao = texture(u_OcclusionSampler, getOcclusionUV()).r;
	color = color * (1.0 + u_OcclusionStrength * (ao - 1.0));
#endif

#endif //end USE_IBL

	f_diffuse = vec3(0.0);
	f_specular_dielectric = vec3(0.0);
	f_specular_metal = vec3(0.0);
	vec3 f_dielectric_brdf = vec3(0.0);
	vec3 f_metal_brdf = vec3(0.0);

#ifdef USE_PUNCTUAL
	for (int i = 0; i < LIGHT_COUNT; ++i)
	{
		Light light = u_Lights[i];

		vec3 pointToLight;
		if (light.type != LightType_Directional)
		{
			pointToLight = light.position - v_Position;
		}
		else
		{
			pointToLight = -light.direction;
		}

		// BSTF
		vec3 l = normalize(pointToLight);   // Direction from surface point to light
		vec3 h = normalize(l + v);          // Direction of the vector between l and v, called halfway vector
		float NdotL = clampedDot(n, l);
		float NdotV = clampedDot(n, v);
		float NdotH = clampedDot(n, h);
		float LdotH = clampedDot(l, h);
		float VdotH = clampedDot(v, h);

		vec3 dielectric_fresnel = F_Schlick(materialInfo.f0_dielectric * materialInfo.specularWeight, materialInfo.f90_dielectric, abs(VdotH));
		vec3 metal_fresnel = F_Schlick(baseColor.rgb, vec3(1.0), abs(VdotH));

		vec3 lightIntensity = getLighIntensity(light, pointToLight);

		vec3 l_diffuse = lightIntensity * NdotL * BRDF_lambertian(baseColor.rgb);
		vec3 l_specular_dielectric = vec3(0.0);
		vec3 l_specular_metal = vec3(0.0);
		vec3 l_dielectric_brdf = vec3(0.0);
		vec3 l_metal_brdf = vec3(0.0);
		vec3 l_clearcoat_brdf = vec3(0.0);
		vec3 l_sheen = vec3(0.0);
		float l_albedoSheenScaling = 1.0;

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
		vec3 diffuse_btdf = lightIntensity * clampedDot(-n, l) * BRDF_lambertian(materialInfo.diffuseTransmissionColorFactor);

#ifdef MATERIAL_VOLUME
		diffuse_btdf = applyVolumeAttenuation(diffuse_btdf, diffuseTransmissionThickness, materialInfo.attenuationColor, materialInfo.attenuationDistance);
#endif
		l_diffuse = mix(l_diffuse, diffuse_btdf, materialInfo.diffuseTransmissionFactor);
#endif // MATERIAL_DIFFUSE_TRANSMISSION

		// BTDF (Bidirectional Transmittance Distribution Function)
#ifdef MATERIAL_TRANSMISSION
		// If the light ray travels through the geometry, use the point it exits the geometry again.
		// That will change the angle to the light source, if the material refracts the light ray.
		vec3 transmissionRay = getVolumeTransmissionRay(n, v, materialInfo.thickness, materialInfo.ior, u_ModelMatrix);
		pointToLight -= transmissionRay;
		l = normalize(pointToLight);

		vec3 transmittedLight = lightIntensity * getPunctualRadianceTransmission(n, v, l, materialInfo.alphaRoughness, materialInfo.f0_dielectric, materialInfo.f90, baseColor.rgb, materialInfo.ior);

#ifdef MATERIAL_VOLUME
		transmittedLight = applyVolumeAttenuation(transmittedLight, length(transmissionRay), materialInfo.attenuationColor, materialInfo.attenuationDistance);
#endif
		l_diffuse = mix(l_diffuse, transmittedLight, materialInfo.transmissionFactor);
#endif

		// Calculation of analytical light
		// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
		vec3 intensity = getLighIntensity(light, pointToLight);

#ifdef MATERIAL_ANISOTROPY
		l_specular_metal = intensity * NdotL * BRDF_specularGGXAnisotropy(materialInfo.alphaRoughness, materialInfo.anisotropyStrength, n, v, l, h, materialInfo.anisotropicT, materialInfo.anisotropicB);
		l_specular_dielectric = l_specular_metal;
#else
		l_specular_metal = intensity * NdotL * BRDF_specularGGX(materialInfo.alphaRoughness, NdotL, NdotV, NdotH);
		l_specular_dielectric = l_specular_metal;
#endif

		l_metal_brdf = metal_fresnel * l_specular_metal;
		l_dielectric_brdf = mix(l_diffuse, l_specular_dielectric, dielectric_fresnel); // Do we need to handle vec3 fresnel here?

#ifdef MATERIAL_IRIDESCENCE
		l_metal_brdf = mix(l_metal_brdf, l_specular_metal * iridescenceFresnel_metallic, materialInfo.iridescenceFactor);
		l_dielectric_brdf = mix(l_dielectric_brdf, rgb_mix(l_diffuse, l_specular_dielectric, iridescenceFresnel_dielectric), materialInfo.iridescenceFactor);
#endif

#ifdef MATERIAL_CLEARCOAT
		l_clearcoat_brdf = intensity * getPunctualRadianceClearCoat(materialInfo.clearcoatNormal, v, l, h, VdotH,
			materialInfo.clearcoatF0, materialInfo.clearcoatF90, materialInfo.clearcoatRoughness);
#endif

#ifdef MATERIAL_SHEEN
		l_sheen = intensity * getPunctualRadianceSheen(materialInfo.sheenColorFactor, materialInfo.sheenRoughnessFactor, NdotL, NdotV, NdotH);
		l_albedoSheenScaling = min(1.0 - max3(materialInfo.sheenColorFactor) * albedoSheenScalingLUT(NdotV, materialInfo.sheenRoughnessFactor),
			1.0 - max3(materialInfo.sheenColorFactor) * albedoSheenScalingLUT(NdotL, materialInfo.sheenRoughnessFactor));
#endif

		vec3 l_color = mix(l_dielectric_brdf, l_metal_brdf, materialInfo.metallic);
		l_color = l_sheen + l_color * l_albedoSheenScaling;
		l_color = mix(l_color, l_clearcoat_brdf, clearcoatFactor * clearcoatFresnel);
		color += l_color;
	}
#endif // USE_PUNCTUAL

	f_emissive = u_EmissiveFactor;
#ifdef MATERIAL_EMISSIVE_STRENGTH
	f_emissive *= u_EmissiveStrength;
#endif
#ifdef HAS_EMISSIVE_MAP
	f_emissive *= texture(u_EmissiveSampler, getEmissiveUV()).rgb;
#endif


#ifdef MATERIAL_UNLIT
	color = baseColor.rgb;
#else
	color = f_emissive * (1.0 - clearcoatFactor * clearcoatFresnel) + color;
#endif
#ifndef DEBUG_NONE
#define DEBUG_NONE 0
#endif // !DEBUG_NONE
#ifndef DEBUG
#define DEBUG DEBUG_NONE
#endif // !DEBUG

#ifndef PXOUT_rcd

#if ALPHAMODE == ALPHAMODE_MASK
	// Late discard to avoid sampling artifacts. See https://github.com/KhronosGroup/glTF-Sample-Viewer/issues/267
	if (baseColor.a < u_AlphaCutoff)
	{
		discard;
	}
	baseColor.a = 1.0;
#endif

#ifdef TONEMAP_OPT
	finalColor = vec4(toneMap(color), baseColor.a);
#else
	//LINEAR_OUTPUT
	finalColor = vec4(color.rgb, baseColor.a);
#endif

#else
	// In case of missing data for a debug view, render a checkerboard.
	finalColor = vec4(1.0);
	{
		float frequency = 0.02;
		float gray = 0.9;

		vec2 v1 = step(0.5, fract(frequency * gl_FragCoord.xy));
		vec2 v2 = step(0.5, vec2(1.0) - fract(frequency * gl_FragCoord.xy));
		finalColor.rgb *= gray + v1.x * v1.y + v2.x * v2.y;
	}
#endif

	// Debug views:

	// Generic:
#if DEBUG == DEBUG_UV_0 && defined(HAS_TEXCOORD_0_VEC2)
	finalColor.rgb = vec3(v_texcoord_0, 0);
#endif
#if DEBUG == DEBUG_UV_1 && defined(HAS_TEXCOORD_1_VEC2)
	finalColor.rgb = vec3(v_texcoord_1, 0);
#endif
#if DEBUG == DEBUG_NORMAL_TEXTURE && defined(HAS_NORMAL_MAP)
	finalColor.rgb = (normalInfo.ntex + 1.0) / 2.0;
#endif
#if DEBUG == DEBUG_NORMAL_SHADING
	finalColor.rgb = (n + 1.0) / 2.0;
#endif
#if DEBUG == DEBUG_NORMAL_GEOMETRY
	finalColor.rgb = (normalInfo.ng + 1.0) / 2.0;
#endif
#if DEBUG == DEBUG_TANGENT
	finalColor.rgb = (normalInfo.t + 1.0) / 2.0;
#endif
#if DEBUG == DEBUG_BITANGENT
	finalColor.rgb = (normalInfo.b + 1.0) / 2.0;
#endif
#if DEBUG == DEBUG_ALPHA
	finalColor.rgb = vec3(baseColor.a);
#endif
#if DEBUG == DEBUG_OCCLUSION && defined(HAS_OCCLUSION_MAP)
	finalColor.rgb = vec3(ao);
#endif
#if DEBUG == DEBUG_EMISSIVE
	finalColor.rgb = linearTosRGB(f_emissive);
#endif

	// MR:
#ifdef MATERIAL_METALLICROUGHNESS
#if DEBUG == DEBUG_METALLIC
	finalColor.rgb = vec3(materialInfo.metallic);
#endif
#if DEBUG == DEBUG_ROUGHNESS
	finalColor.rgb = vec3(materialInfo.perceptualRoughness);
#endif
#if DEBUG == DEBUG_BASE_COLOR
	finalColor.rgb = linearTosRGB(materialInfo.baseColor);
#endif
#endif

	// Clearcoat:
#ifdef MATERIAL_CLEARCOAT
#if DEBUG == DEBUG_CLEARCOAT_FACTOR
	finalColor.rgb = vec3(materialInfo.clearcoatFactor);
#endif
#if DEBUG == DEBUG_CLEARCOAT_ROUGHNESS
	finalColor.rgb = vec3(materialInfo.clearcoatRoughness);
#endif
#if DEBUG == DEBUG_CLEARCOAT_NORMAL
	finalColor.rgb = (materialInfo.clearcoatNormal + vec3(1)) / 2.0;
#endif
#endif

	// Sheen:
#ifdef MATERIAL_SHEEN
#if DEBUG == DEBUG_SHEEN_COLOR
	finalColor.rgb = materialInfo.sheenColorFactor;
#endif
#if DEBUG == DEBUG_SHEEN_ROUGHNESS
	finalColor.rgb = vec3(materialInfo.sheenRoughnessFactor);
#endif
#endif

	// Specular:
#ifdef MATERIAL_SPECULAR
#if DEBUG == DEBUG_SPECULAR_FACTOR
	finalColor.rgb = vec3(materialInfo.specularWeight);
#endif

#if DEBUG == DEBUG_SPECULAR_COLOR
	vec3 specularTexture = vec3(1.0);
#ifdef HAS_SPECULAR_COLOR_MAP
	specularTexture.rgb = texture(u_SpecularColorSampler, getSpecularColorUV()).rgb;
#endif
	finalColor.rgb = u_KHR_materials_specular_specularColorFactor * specularTexture.rgb;
#endif
#endif

	// Transmission, Volume:
#ifdef MATERIAL_TRANSMISSION
#if DEBUG == DEBUG_TRANSMISSION_FACTOR
	finalColor.rgb = vec3(materialInfo.transmissionFactor);
#endif
#endif
#ifdef MATERIAL_VOLUME
#if DEBUG == DEBUG_VOLUME_THICKNESS
	finalColor.rgb = vec3(materialInfo.thickness / u_ThicknessFactor);
#endif
#endif

	// Iridescence:
#ifdef MATERIAL_IRIDESCENCE
#if DEBUG == DEBUG_IRIDESCENCE_FACTOR
	finalColor.rgb = vec3(materialInfo.iridescenceFactor);
#endif
#if DEBUG == DEBUG_IRIDESCENCE_THICKNESS
	finalColor.rgb = vec3(materialInfo.iridescenceThickness / 1200.0);
#endif
#endif

	// Anisotropy:
#ifdef MATERIAL_ANISOTROPY
#if DEBUG == DEBUG_ANISOTROPIC_STRENGTH
	finalColor.rgb = vec3(materialInfo.anisotropyStrength);
#endif
#if DEBUG == DEBUG_ANISOTROPIC_DIRECTION
	vec2 direction = vec2(1.0, 0.0);
#ifdef HAS_ANISOTROPY_MAP
	direction = texture(u_AnisotropySampler, getAnisotropyUV()).xy;
	direction = direction * 2.0 - vec2(1.0); // [0, 1] -> [-1, 1]
#endif
	vec2 directionRotation = u_Anisotropy.xy; // cos(theta), sin(theta)
	mat2 rotationMatrix = mat2(directionRotation.x, directionRotation.y, -directionRotation.y, directionRotation.x);
	direction = (direction + vec2(1.0)) * 0.5; // [-1, 1] -> [0, 1]

	finalColor.rgb = vec3(direction, 0.0);
#endif
#endif

	// Diffuse Transmission:
#ifdef MATERIAL_DIFFUSE_TRANSMISSION
#if DEBUG == DEBUG_DIFFUSE_TRANSMISSION_FACTOR
	finalColor.rgb = linearTosRGB(vec3(materialInfo.diffuseTransmissionFactor));
#endif
#if DEBUG == DEBUG_DIFFUSE_TRANSMISSION_COLOR_FACTOR
	finalColor.rgb = linearTosRGB(materialInfo.diffuseTransmissionColorFactor);
#endif
#endif
	return finalColor;
}

#endif // !pbr_glsl



#endif // !pbrpx_h_