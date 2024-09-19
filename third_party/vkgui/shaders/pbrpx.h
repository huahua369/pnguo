#ifndef pbrpx_h_
#define pbrpx_h_


const int LightType_Directional = 0;
const int LightType_Point = 1;
const int LightType_Spot = 2;
#ifndef MAX_LIGHT_INSTANCES
#define MAX_LIGHT_INSTANCES  80
#endif
#ifndef MAX_SHADOW_INSTANCES
#define MAX_SHADOW_INSTANCES 32
#endif

// alphaMode
#define ALPHA_OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2

#define USE_PUNCTUAL

struct Light
{
	mat4          mLightViewProj; // 阴影用
	mat4          mLightView;

	vec3          direction;
	float         range;

	vec3          color;
	float         intensity;

	vec3          position;
	float         innerConeCos;

	float         outerConeCos;
	int           type;
	float         depthBias;
	int           shadowMapIndex;
};

struct PerFrame_t
{
	mat4          u_mCameraCurrViewProj;
	mat4          u_mCameraPrevViewProj;
	mat4          u_mCameraCurrViewProjInverse;

	vec4          u_CameraPos;
	float         u_iblFactor;
	float         u_EmissiveFactor;
	vec2          u_invScreenResolution;

	vec4          u_WireframeOptions;
	float         u_LodBias;
	vec2          u_padding;
	int           u_lightCount;
	Light		  u_lights[MAX_LIGHT_INSTANCES];

};

struct pbr_factors_t
{
	// pbrMetallicRoughness
	vec4 baseColorFactor;
	float metallicFactor;
	float roughnessFactor;
	float normalScale;
	// KHR_materials_emissive_strength
	float emissiveStrength;

	vec3 emissiveFactor;
	int   alphaMode;

	float alphaCutoff;
	float occlusionStrength;
	//KHR_materials_ior
	float ior;
	int mipCount;

	// KHR_materials_pbrSpecularGlossiness
	vec4 pbrDiffuseFactor;
	vec3 pbrSpecularFactor;
	float glossinessFactor;


	// Specular KHR_materials_specular
	vec3  specularColorFactor;
	float specularFactor;
	// KHR_materials_sheen 
	vec3 sheenColorFactor;
	float sheenRoughnessFactor;
	// KHR_materials_anisotropy
	vec3 anisotropy;
	// KHR_materials_transmission
	float transmissionFactor;

	ivec2 transmissionFramebufferSize;
	float thicknessFactor;
	float diffuseTransmissionFactor;

	vec3 diffuseTransmissionColorFactor;
	//KHR_materials_dispersion
	float dispersion;

	vec3 attenuationColor;
	float attenuationDistance;

	// KHR_materials_iridescence
	float iridescenceFactor;
	float iridescenceIor;
	float iridescenceThicknessMinimum;
	float iridescenceThicknessMaximum;
	// KHR_materials_clearcoat
	float clearcoatFactor;
	float clearcoatRoughness;
	float clearcoatNormalScale;
	float envIntensity;

	int unlit;
	float pad[3];
	mat3 uvTransform;
};



#ifdef __cplusplus

uniform samplerCube u_DiffuseEnvSampler;//*u_EnvRotation 
uniform samplerCube u_SpecularEnvSampler;//*u_EnvRotation 
uniform sampler2D u_GGXLUT;
uniform samplerCube u_CharlieEnvSampler;//*u_EnvRotation 
uniform sampler2D u_CharlieLUT;
uniform sampler2D u_SheenELUT;
uniform sampler2D u_NormalSampler;
uniform sampler2D u_EmissiveSampler;
uniform sampler2D u_OcclusionSampler;
uniform sampler2D ssaoSampler;

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

uniform sampler2DShadow u_shadowMap[MAX_SHADOW_INSTANCES];

#endif
#if 1
#ifdef USE_IBL
#ifdef ID_diffuseCube
layout(set = 1, binding = ID_diffuseCube) uniform samplerCube u_DiffuseEnvSampler;
#endif
#ifdef ID_specularCube
layout(set = 1, binding = ID_specularCube) uniform samplerCube u_SpecularEnvSampler;
#endif
#define USE_TEX_LOD
#endif


#ifdef ID_brdfTexture
layout(set = 1, binding = ID_brdfTexture) uniform sampler2D u_brdfLUT;
#endif
#ifdef ID_charlieEnvTexture
layout(set = 1, binding = ID_charlieEnvTexture) uniform samplerCube u_CharlieEnvSampler;//*u_EnvRotation
#endif
#ifdef ID_charlieLUT
layout(set = 1, binding = ID_charlieLUT) uniform sampler2D u_CharlieLUT;
#endif
#ifdef ID_sheenELUT
layout(set = 1, binding = ID_sheenELUT) uniform sampler2D u_SheenELUT;
#endif
#ifdef ID_normalTexture
layout(set = 1, binding = ID_normalTexture) uniform sampler2D u_NormalSampler;
#endif
#ifdef ID_emissiveTexture
layout(set = 1, binding = ID_emissiveTexture) uniform sampler2D u_EmissiveSampler;
#endif
#ifdef ID_occlusionTexture
layout(set = 1, binding = ID_occlusionTexture) uniform sampler2D u_OcclusionSampler;
#endif 

#ifdef ID_SSAO
layout(set = 1, binding = ID_SSAO) uniform sampler2D ssaoSampler;
#endif

#ifdef MATERIAL_METALLICROUGHNESS
#ifdef ID_baseColorTexture
layout(set = 1, binding = ID_baseColorTexture) uniform sampler2D u_BaseColorSampler;
#endif
#ifdef ID_metallicRoughnessTexture
layout(set = 1, binding = ID_metallicRoughnessTexture) uniform sampler2D u_MetallicRoughnessSampler;
#endif
#endif

#ifdef MATERIAL_SPECULARGLOSSINESS
#ifdef ID_diffuseTexture
layout(set = 1, binding = ID_diffuseTexture) uniform sampler2D u_DiffuseSampler;
#endif
#ifdef ID_specularGlossinessTexture
layout(set = 1, binding = ID_specularGlossinessTexture) uniform sampler2D u_SpecularGlossinessSampler;
#endif
#endif

#ifdef ID_clearcoatTexture
layout(set = 1, binding = ID_clearcoatTexture) uniform sampler2D u_ClearcoatSampler;
#endif
#ifdef ID_clearcoatRoughnessTexture
layout(set = 1, binding = ID_clearcoatRoughnessTexture) uniform sampler2D u_ClearcoatRoughnessSampler;
#endif
#ifdef ID_clearcoatNormalTexture
layout(set = 1, binding = ID_clearcoatNormalTexture) uniform sampler2D u_ClearcoatNormalSampler;
#endif

#ifdef ID_sheenColorTexture
layout(set = 1, binding = ID_sheenColorTexture) uniform sampler2D u_SheenColorSampler;
#endif
#ifdef ID_sheenRoughnessTexture
layout(set = 1, binding = ID_sheenRoughnessTexture) uniform sampler2D u_SheenRoughnessSampler;
#endif
#ifdef ID_specularTexture
layout(set = 1, binding = ID_specularTexture) uniform sampler2D u_SpecularSampler;
#endif
#ifdef ID_specularColorTexture
layout(set = 1, binding = ID_specularColorTexture) uniform sampler2D u_SpecularColorSampler;
#endif

#ifdef MATERIAL_TRANSMISSION
#ifdef ID_transmissionTexture
layout(set = 1, binding = ID_transmissionTexture) uniform sampler2D u_TransmissionSampler;
#endif
#ifdef ID_transmissionFramebufferTexture
layout(set = 1, binding = ID_transmissionFramebufferTexture) uniform sampler2D u_TransmissionFramebufferSampler;
#endif
#endif


// Volume Material 
#ifdef ID_thicknessTexture
layout(set = 1, binding = ID_thicknessTexture) uniform sampler2D u_ThicknessSampler;
#endif 
// Iridescence 
#ifdef ID_iridescenceTexture
layout(set = 1, binding = ID_iridescenceTexture) uniform sampler2D u_IridescenceSampler;
#endif
#if ID_iridescenceThicknessTexture
layout(set = 1, binding = ID_iridescenceThicknessTexture) uniform sampler2D u_IridescenceThicknessSampler;
#endif


// Diffuse Transmission

#ifdef ID_diffuseTransmissionTexture
layout(set = 1, binding = ID_diffuseTransmissionTexture) uniform sampler2D u_DiffuseTransmissionSampler;
#endif
#ifdef  ID_diffuseTransmissionColorTexture
layout(set = 1, binding = ID_diffuseTransmissionColorTexture) uniform sampler2D u_DiffuseTransmissionColorSampler;
#endif

// Anisotropy

#ifdef ID_AnisotropyTexture
layout(set = 1, binding = ID_AnisotropyTexture) uniform sampler2D u_AnisotropySampler;
#endif

#ifdef ID_shadowMap
layout(set = 1, binding = ID_shadowMap) uniform sampler2DShadow u_shadowMap[MAX_SHADOW_INSTANCES];
#endif
#endif // __cplusplus


struct vsio_ps
{
	mat4 u_ModelMatrix;
	mat4 u_ViewMatrix;	//vp合并  mat4 u_ProjectionMatrix;
	vec4 v_Color0;
	vec2 v_texcoord[2];
	vec3 v_Position;
};



#ifndef __cplusplus
layout(scalar, set = 0, binding = 0) uniform perFrame
{
	PerFrame_t myPerFrame;
};

layout(scalar, set = 0, binding = 1) uniform perObject
{
	mat4 myPerObject_u_mCurrWorld;
	mat4 myPerObject_u_mPrevWorld;
	//PBRFactors u_pbrParams;
	pbr_factors_t u_pbrParams;
};
#ifdef  ID_MATUV_DATA
layout(set = 0, binding = ID_MATUV_DATA) buffer per_matuv
{
	mat3 u_matuv[];
};
#endif
#else
PerFrame_t myPerFrame;

mat4 myPerObject_u_mCurrWorld;
mat4 myPerObject_u_mPrevWorld;

//PBRFactors u_pbrParams;
pbr_factors_t u_pbrParams;
//mat3 u_EnvRotation;
mat3 u_matuv[];
#endif // !__cplusplus

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
	float at = mix(alphaRoughness, 1.0f, anisotropy * anisotropy);
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

const float GAMMA = 2.2;
const float INV_GAMMA = 1.0 / GAMMA;


// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3 ACESInputMat = mat3
(
	0.59719, 0.07600, 0.02840,
	0.35458, 0.90834, 0.13383,
	0.04823, 0.01566, 0.83777
);


// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat = mat3
(
	1.60475, -0.10208, -0.00327,
	-0.53108, 1.10813, -0.07276,
	-0.07367, -0.00605, 1.07602
);


// linear to sRGB approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 linearTosRGB(vec3 color)
{
	return pow(color, vec3(INV_GAMMA));
}


// sRGB to linear approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 sRGBToLinear(vec3 srgbIn)
{
	return vec3(pow(srgbIn, vec3(GAMMA)));
}


vec4 sRGBToLinear(vec4 srgbIn)
{
	return vec4(sRGBToLinear(vec3(srgbIn)), srgbIn.w);
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

#ifndef ID_normalTexCoord
#define ID_normalTexCoord 0
#endif 
#ifndef ID_emissiveTexCoord
#define ID_emissiveTexCoord 0
#endif  
#ifndef ID_occlusionTexCoord
#define ID_occlusionTexCoord 0
#endif  
#ifndef ID_baseColorTexCoord
#define ID_baseColorTexCoord 0
#endif  
#ifndef ID_metallicRoughnessTexCoord
#define ID_metallicRoughnessTexCoord 0
#endif  
#ifndef ID_specularGlossinessTexCoord
#define ID_specularGlossinessTexCoord 0
#endif  
#ifndef ID_diffuseTexCoord
#define ID_diffuseTexCoord 0
#endif  
#ifndef ID_clearcoatTexCoord
#define ID_clearcoatTexCoord 0
#endif  
#ifndef ID_clearcoatRoughnessTexCoord
#define ID_clearcoatRoughnessTexCoord 0
#endif  
#ifndef ID_clearcoatNormalTexCoord
#define ID_clearcoatNormalTexCoord 0
#endif  
#ifndef ID_sheenColorTexCoord
#define ID_sheenColorTexCoord 0
#endif
#ifndef ID_sheenRoughnessTexCoord
#define ID_sheenRoughnessTexCoord 0
#endif 
#ifndef ID_specularTexCoord
#define ID_specularTexCoord 0
#endif 
#ifndef ID_specularColorTexCoord
#define ID_specularColorTexCoord 0
#endif 
#ifndef ID_transmissionTexCoord
#define ID_transmissionTexCoord 0
#endif 
#ifndef ID_thicknessTexCoord
#define ID_thicknessTexCoord 0
#endif 
#ifndef ID_iridescenceTexCoord
#define ID_iridescenceTexCoord 0
#endif 
#ifndef ID_iridescenceThicknessTexCoord
#define ID_iridescenceThicknessTexCoord 0
#endif 
#ifndef ID_diffuseTransmissionTexCoord
#define ID_diffuseTransmissionTexCoord 0
#endif 
#ifndef ID_diffuseTransmissionColorTexCoord
#define ID_diffuseTransmissionColorTexCoord 0
#endif 
#ifndef ID_anisotropyTexCoord
#define ID_anisotropyTexCoord 0
#endif 
#endif 
#if 1
vec2 getNormalUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_normalTexCoord], 1.0f);


#ifdef HAS_NORMAL_UV_TRANSFORM
	//uv = u_NormalUVTransform * uv;
	uv = u_matuv[u_NormalUVTransform] * uv;
#endif
	return vec2(uv);
}


vec2 getEmissiveUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_emissiveTexCoord], 1.0f);

#ifdef HAS_EMISSIVE_UV_TRANSFORM
	//uv = u_EmissiveUVTransform * uv;
	uv = u_matuv[u_EmissiveUVTransform] * uv;
#endif

	return vec2(uv);
}


vec2 getOcclusionUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_occlusionTexCoord], 1.0f);

#ifdef HAS_OCCLUSION_UV_TRANSFORM
	//uv = u_OcclusionUVTransform * uv;
	uv = u_matuv[u_OcclusionUVTransform] * uv;
#endif

	return vec2(uv);
}


// Metallic Roughness Material


#ifdef MATERIAL_METALLICROUGHNESS


vec2 getBaseColorUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_baseColorTexCoord], 1.0f);

#ifdef HAS_BASECOLOR_UV_TRANSFORM
	//uv = u_BaseColorUVTransform * uv;
	uv = u_matuv[u_BaseColorUVTransform] * uv;
#endif

	return vec2(uv);

}

vec2 getMetallicRoughnessUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_metallicRoughnessTexCoord], 1.0f);

#ifdef HAS_METALLICROUGHNESS_UV_TRANSFORM
	//uv = u_MetallicRoughnessUVTransform * uv;
	uv = u_matuv[u_MetallicRoughnessUVTransform] * uv;
#endif

	return vec2(uv);
}

#endif


// Specular Glossiness Material


#ifdef MATERIAL_SPECULARGLOSSINESS


vec2 getSpecularGlossinessUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_specularGlossinessTexCoord], 1.0f);

#ifdef HAS_SPECULARGLOSSINESS_UV_TRANSFORM
	//uv = u_SpecularGlossinessUVTransform * uv;
	uv = u_matuv[u_SpecularGlossinessUVTransform] * uv;
#endif

	return vec2(uv);
}

vec2 getDiffuseUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_diffuseTexCoord], 1.0f);

#ifdef HAS_DIFFUSE_UV_TRANSFORM
	//uv = u_DiffuseUVTransform * uv;
	uv = u_matuv[u_DiffuseUVTransform] * uv;
#endif

	return vec2(uv);
}

#endif


// Clearcoat Material


#ifdef MATERIAL_CLEARCOAT


vec2 getClearcoatUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_clearcoatTexCoord], 1.0f);
#ifdef HAS_CLEARCOAT_UV_TRANSFORM
	//uv = u_ClearcoatUVTransform * uv;
	uv = u_matuv[u_ClearcoatUVTransform] * uv;
#endif
	return vec2(uv);
}

vec2 getClearcoatRoughnessUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_clearcoatRoughnessTexCoord], 1.0f);
#ifdef HAS_CLEARCOATROUGHNESS_UV_TRANSFORM
	//uv = u_ClearcoatRoughnessUVTransform * uv;
	uv = u_matuv[u_ClearcoatRoughnessUVTransform] * uv;
#endif
	return vec2(uv);
}

vec2 getClearcoatNormalUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_clearcoatNormalTexCoord], 1.0f);
#ifdef HAS_CLEARCOATNORMAL_UV_TRANSFORM
	//uv = u_ClearcoatNormalUVTransform * uv;
	uv = u_matuv[u_ClearcoatNormalUVTransform] * uv;
#endif
	return vec2(uv);
}

#endif


// Sheen Material


#ifdef MATERIAL_SHEEN



vec2 getSheenColorUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_sheenColorTexCoord], 1.0f);
#ifdef HAS_SHEENCOLOR_UV_TRANSFORM
	//uv = u_SheenColorUVTransform * uv;
	uv = u_matuv[u_SheenColorUVTransform] * uv;
#endif
	return vec2(uv);
}

vec2 getSheenRoughnessUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_sheenRoughnessTexCoord], 1.0f);
#ifdef HAS_SHEENROUGHNESS_UV_TRANSFORM
	//uv = u_SheenRoughnessUVTransform * uv;
	uv = u_matuv[u_SheenRoughnessUVTransform] * uv;
#endif
	return vec2(uv);
}

#endif


// Specular Material


#ifdef MATERIAL_SPECULAR



vec2 getSpecularUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_specularTexCoord], 1.0f);
#ifdef HAS_SPECULAR_UV_TRANSFORM
	//uv = u_SpecularUVTransform * uv;
	uv = u_matuv[u_SpecularUVTransform] * uv;
#endif
	return vec2(uv);
}

vec2 getSpecularColorUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_specularColorTexCoord], 1.0f);
#ifdef HAS_SPECULARCOLOR_UV_TRANSFORM
	//uv = u_SpecularColorUVTransform * uv;
	uv = u_matuv[u_SpecularColorUVTransform] * uv;
#endif
	return vec2(uv);
}

#endif


// Transmission Material


#ifdef MATERIAL_TRANSMISSION

vec2 getTransmissionUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_transmissionTexCoord], 1.0f);
#ifdef HAS_TRANSMISSION_UV_TRANSFORM
	//uv = u_TransmissionUVTransform * uv;
	uv = u_matuv[u_TransmissionUVTransform] * uv;
#endif
	return vec2(uv);
}

#endif


// Volume Material


#ifdef MATERIAL_VOLUME

vec2 getThicknessUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_thicknessTexCoord], 1.0f);
#ifdef HAS_THICKNESS_UV_TRANSFORM
	//uv = u_ThicknessUVTransform * uv;
	uv = u_matuv[u_ThicknessUVTransform] * uv;
#endif
	return vec2(uv);
}

#endif


// Iridescence


#ifdef MATERIAL_IRIDESCENCE

vec2 getIridescenceUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_iridescenceTexCoord], 1.0f);
#ifdef HAS_IRIDESCENCE_UV_TRANSFORM
	//uv = u_IridescenceUVTransform * uv;
	uv = u_matuv[u_IridescenceUVTransform] * uv;
#endif
	return vec2(uv);
}

vec2 getIridescenceThicknessUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_iridescenceThicknessTexCoord], 1.0f);
#ifdef HAS_IRIDESCENCETHICKNESS_UV_TRANSFORM
	//uv = u_IridescenceThicknessUVTransform * uv;
	uv = u_matuv[u_IridescenceThicknessUVTransform] * uv;
#endif
	return vec2(uv);
}

#endif


// Diffuse Transmission

#ifdef MATERIAL_DIFFUSE_TRANSMISSION


vec2 getDiffuseTransmissionUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_diffuseTransmissionTexCoord], 1.0f);
#ifdef HAS_DIFFUSE_TRANSMISSION_UV_TRANSFORM
	//uv = u_DiffuseTransmissionUVTransform * uv;
	uv = u_matuv[u_DiffuseTransmissionUVTransform] * uv;
#endif
	return vec2(uv);
}

vec2 getDiffuseTransmissionColorUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_diffuseTransmissionColorTexCoord], 1.0f);
#ifdef HAS_DIFFUSE_TRANSMISSION_COLOR_UV_TRANSFORM
	//uv = u_DiffuseTransmissionColorUVTransform * uv;
	uv = u_matuv[u_DiffuseTransmissionColorUVTransform] * uv;
#endif
	return vec2(uv);
}

#endif

// Anisotropy

#ifdef MATERIAL_ANISOTROPY

vec2 getAnisotropyUV(vsio_ps vp)
{
	vec3 uv = vec3(vp.v_texcoord[ID_anisotropyTexCoord], 1.0f);
#ifdef HAS_ANISOTROPY_UV_TRANSFORM
	//uv = u_AnisotropyUVTransform * uv;
	uv = u_matuv[u_AnisotropyUVTransform] * uv;
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

#if 1
//pbr_ibl
vec3 getDiffuseLight(vec3 n)
{
#ifdef  ID_diffuseCube
#ifdef ID_MATUV_DATA
	n = u_matuv[0] * n;
#endif // ID_MATUV_DATA
	vec4 textureSample = texture(u_DiffuseEnvSampler, n);// u_pbrParams.envRotation* n);
#else
	vec4 textureSample = vec4(1.0);
#endif
#ifndef __cplusplus
	textureSample.rgb *= u_pbrParams.envIntensity;
#endif // __cplusplus
	return vec3(textureSample);
}


vec4 getSpecularSample(vec3 reflection, float lod)
{
	vec4 textureSample = vec4(1.0);
#ifdef  ID_specularCube
#ifdef ID_MATUV_DATA
	reflection = u_matuv[0] * reflection;
#endif // ID_MATUV_DATA
	textureSample = textureLod(u_SpecularEnvSampler, reflection, lod);//u_pbrParams.envRotation * reflection, lod);
#endif
#ifndef __cplusplus
	textureSample.rgb *= u_pbrParams.envIntensity;
#endif // __cplusplus
	return textureSample;
}


vec4 getSheenSample(vec3 reflection, float lod)
{
#ifdef  ID_charlieEnvTexture
#ifdef ID_MATUV_DATA
	reflection = u_matuv[0] * reflection;
#endif // ID_MATUV_DATA
	vec4 textureSample = textureLod(u_CharlieEnvSampler, reflection, lod);//u_pbrParams.envRotation * reflection, lod);
#else
	vec4 textureSample = vec4(1.0);
#endif
#ifndef __cplusplus
	textureSample.rgb *= u_pbrParams.envIntensity;
#endif // __cplusplus
	return textureSample;
}
#ifdef  ID_brdfTexture
vec3 getIBLGGXFresnel(vec3 n, vec3 v, float roughness, vec3 F0, float specularWeight)
{
	// see https://bruop.github.io/ibl/#single_scattering_results at Single Scattering Results
	// Roughness dependent fresnel, from Fdez-Aguera
	float NdotV = clampedDot(n, v);
	vec2 brdfSamplePoint = clamp(vec2(NdotV, roughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
	vec2 f_ab = vec2(texture(u_brdfLUT, brdfSamplePoint));// .rg;
	vec3 Fr = max(vec3(1.0 - roughness), F0) - F0;
	vec3 k_S = F0 + Fr * pow(1.0 - NdotV, 5.0);
	vec3 FssEss = specularWeight * (k_S * f_ab.x + f_ab.y);

	// Multiple scattering, from Fdez-Aguera
	float Ems = (1.0 - (f_ab.x + f_ab.y));
	vec3 F_avg = specularWeight * (F0 + (vec3(1.0) - F0) / 21.0f);
	vec3 FmsEms = Ems * FssEss * F_avg / (1.0f - F_avg * Ems);

	return FssEss + FmsEms;
}
#endif
vec3 getIBLRadianceGGX(vec3 n, vec3 v, float roughness)
{
	float NdotV = clampedDot(n, v);
	float lod = roughness * float(u_pbrParams.mipCount - 1);
	lod = clamp(lod, 0.0f, float(u_pbrParams.mipCount - 1));
	vec3 reflection = normalize(reflect(-v, n));
	vec4 specularSample = getSpecularSample(reflection, lod);

	//vec3 specularLight = specularSample.rgb;

	return vec3(specularSample);
}


#ifdef MATERIAL_TRANSMISSION
vec3 getTransmissionSample(vec2 fragCoord, float roughness, float ior)
{
	float framebufferLod = log2(float(u_pbrParams.transmissionFramebufferSize.x)) * applyIorToRoughness(roughness, ior);
#ifdef ID_transmissionFramebufferTexture
	vec3 transmittedLight = vec3(textureLod(u_TransmissionFramebufferSampler, fragCoord, framebufferLod));
	return transmittedLight;
#else
	return vec3(0);
#endif
}
#endif


#ifdef MATERIAL_TRANSMISSION
vec3 getIBLVolumeRefraction(vec3 n, vec3 v, float perceptualRoughness, vec3 baseColor, vec3 f0, vec3 f90,
	vec3 position, mat4 modelMatrix, mat4 viewMatrix,/* mat4 projMatrix,*/ float ior, float thickness, vec3 attenuationColor, float attenuationDistance, float dispersion)
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
		vec4 ndcPos = /*projMatrix **/ viewMatrix * vec4(refractedRayExit, 1.0);
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
	vec4 ndcPos = /*projMatrix **/ viewMatrix * vec4(refractedRayExit, 1.0);
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
	vec2 brdf = vec2(texture(u_brdfLUT, brdfSamplePoint));// .rg;
	vec3 specularColor = f0 * brdf.x + f90 * brdf.y;

	return (1.0f - specularColor) * attenuatedColor * baseColor;
}
#endif


#ifdef MATERIAL_ANISOTROPY
vec3 getIBLRadianceAnisotropy(vec3 n, vec3 v, float roughness, float anisotropy, vec3 anisotropyDirection)
{
	float NdotV = clampedDot(n, v);

	float tangentRoughness = mix(roughness, 1.0f, anisotropy * anisotropy);
	vec3  anisotropicTangent = cross(anisotropyDirection, v);
	vec3  anisotropicNormal = cross(anisotropicTangent, anisotropyDirection);
	float bendFactor = 1.0 - anisotropy * (1.0 - roughness);
	float bendFactorPow4 = bendFactor * bendFactor * bendFactor * bendFactor;
	vec3  bentNormal = normalize(mix(anisotropicNormal, n, bendFactorPow4));

	float lod = roughness * float(u_pbrParams.mipCount - 1);
	vec3 reflection = normalize(reflect(-v, bentNormal));

	vec4 specularSample = getSpecularSample(reflection, lod);

	vec3 specularLight = vec3(specularSample);

	return specularLight;
}
#endif
#ifdef  ID_charlieLUT
vec3 getIBLRadianceCharlie(vec3 n, vec3 v, float sheenRoughness, vec3 sheenColor)
{
	float NdotV = clampedDot(n, v);
	float lod = sheenRoughness * float(u_pbrParams.mipCount - 1);
	vec3 reflection = normalize(reflect(-v, n));

	vec2 brdfSamplePoint = clamp(vec2(NdotV, sheenRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
	float brdf = texture(u_CharlieLUT, brdfSamplePoint).b;
	vec4 sheenSample = getSheenSample(reflection, lod);

	vec3 sheenLight = vec3(sheenSample);
	return sheenLight * sheenColor * brdf;
}
#endif
#endif // !no_ibl
#ifdef pbr_iridescence
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
	vsio_ps vp;
};


// Get normal, tangent and bitangent vectors.
NormalInfo getNormalInfo1(vec3 v, vsio_ps vp)
{
	vec2 UV = getNormalUV(vp);
	vec2 uv_dx = dFdx(UV);
	vec2 uv_dy = dFdy(UV);

	if (length(uv_dx) <= 1e-2) {
		uv_dx = vec2(1.0, 0.0);
	}

	if (length(uv_dy) <= 1e-2) {
		uv_dy = vec2(0.0, 1.0);
	}

	vec3 t_ = (uv_dy.t * dFdx(vp.v_Position) - uv_dx.t * dFdy(vp.v_Position)) /
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
	ng = normalize(cross(dFdx(vp.v_Position), dFdy(vp.v_Position)));
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
#ifdef ID_normalTexture
	info.ntex = vec3(texture(u_NormalSampler, UV)) * 2.0 - vec3(1.0);
	info.ntex *= vec3(u_pbrParams.normalScale, u_pbrParams.normalScale, 1.0);
	info.ntex = normalize(info.ntex);
	info.n = normalize(mat3(t, b, ng) * info.ntex);
#else
	info.n = ng;
#endif
	info.t = t;
	info.b = b;
	return info;
}
NormalInfo getNormalInfo(vsio_ps vp)
{
	vec2 UV = getNormalUV(vp);
	vec3 ng;
#ifndef ID_TANGENT
	vec3 pos_dx = dFdx(vp.v_Position);
	vec3 pos_dy = dFdy(vp.v_Position);
	vec3 tex_dx = dFdx(vec3(UV, 0.0));
	vec3 tex_dy = dFdy(vec3(UV, 0.0));
	vec3 t = (tex_dy.y * pos_dx - tex_dx.y * pos_dy) / (tex_dx.x * tex_dy.y - tex_dy.x * tex_dx.y);

#ifdef ID_NORMAL
	ng = normalize(Input.Normal);
#else
	ng = cross(pos_dx, pos_dy);
#endif

	t = normalize(t - ng * dot(ng, t));
	vec3 b = normalize(cross(ng, t));
	mat3 tbn = mat3(t, b, ng);
#else // HAS_TANGENTS
	mat3 tbn = mat3(Input.Tangent, Input.Binormal, Input.Normal);
	vec3 t = Input.Tangent;
	vec3 b = Input.Binormal;
	ng = normalize(Input.Normal);
#endif

#ifdef ID_normalTexture
	vec2 xy = 2.0 * texture(u_NormalSampler, UV, myPerFrame.u_LodBias).rg - 1.0;
	float z = sqrt(1.0 - dot(xy, xy));
	vec3 n = vec3(xy, z);
	n = normalize(tbn * (n /* * vec3(u_NormalScale, u_NormalScale, 1.0) */));
#else
	// The tbn matrix is linearly interpolated, so we need to re-normalize
	vec3 n = tbn[2];
	n = normalize(n);
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
#ifdef ID_normalTexture
	info.ntex = vec3(texture(u_NormalSampler, UV)) * 2.0 - vec3(1.0);
	info.ntex *= vec3(u_pbrParams.normalScale, u_pbrParams.normalScale, 1.0);
	info.ntex = normalize(info.ntex);
	info.n = normalize(mat3(t, b, ng) * info.ntex);
#else
	info.n = ng;
#endif
	info.t = t;
	info.b = b;
	return info;
}

vec3 getPixelNormal(vsio_ps vp)
{
	vec2 UV = getNormalUV(vp);
	// Retrieve the tangent space matrix
#ifndef ID_TANGENT
	vec3 pos_dx = dFdx(vp.v_Position);
	vec3 pos_dy = dFdy(vp.v_Position);
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
	vec3 n = tbn[2];
	n = normalize(n);
#endif

	return n;
}

#ifdef MATERIAL_CLEARCOAT
vec3 getClearcoatNormal(NormalInfo normalInfo, vsio_ps vp)
{
#ifdef ID_clearcoatNormalTexture
	vec3 n = texture(u_ClearcoatNormalSampler, getClearcoatNormalUV(vp)).rgb * 2.0 - vec3(1.0);
	n *= vec3(u_pbrParams.clearcoatNormalScale, u_pbrParams.clearcoatNormalScale, 1.0);
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

vec4 getBaseColor(vsio_ps vp)
{
	vec4 baseColor = vec4(1);

#if defined(MATERIAL_SPECULARGLOSSINESS)
	baseColor = u_pbrParams.pbrDiffuseFactor;
#elif defined(MATERIAL_METALLICROUGHNESS)
	baseColor = u_pbrParams.baseColorFactor;
#endif

#if defined(MATERIAL_SPECULARGLOSSINESS) && defined(ID_diffuseTexture)
	baseColor *= texture(u_DiffuseSampler, getDiffuseUV(vp));
#elif defined(MATERIAL_METALLICROUGHNESS) && defined(ID_baseColorTexture)
	baseColor *= texture(u_BaseColorSampler, getBaseColorUV(vp));
#endif

	return baseColor * getVertexColor();
}



vec4 get_roughness(MaterialInfo info, pbr_factors_t params, vec2 uv, out vec3 diffuseColor, out vec3  specularColor, out float perceptualRoughness, out float metallic)
{
	perceptualRoughness = 0.0;
	diffuseColor = vec3(0.0, 0.0, 0.0);
	specularColor = vec3(0.0, 0.0, 0.0);
	metallic = 0.0;
	vec3 f0 = vec3(0.04, 0.04, 0.04);
	vec4 baseColor = getBaseColor(info.vp);
	// Metallic and Roughness material properties are packed together
	// In glTF, these factors can be specified by fixed scalar values
	// or from a metallic-roughness map
#ifdef MATERIAL_SPECULARGLOSSINESS
#ifdef ID_specularGlossinessTexture
	vec4 sgSample = texture(u_SpecularGlossinessSampler, getSpecularGlossinessUV(info.vp));
#else
	vec4 sgSample = vec4(1.0);
#endif  
	perceptualRoughness = (1.0 - sgSample.a * params.glossinessFactor); // glossiness to roughness
	f0 = vec3(sgSample) * params.pbrSpecularFactor; // specular

	// f0 = specular
	specularColor = f0;
	float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);
	diffuseColor = vec3(baseColor) * oneMinusSpecularStrength;

#ifdef DEBUG_METALLIC
	// do conversion between metallic M-R and S-G metallic
	metallic = solveMetallic(vec3(baseColor), specularColor, oneMinusSpecularStrength);
#endif // ! DEBUG_METALLIC
#endif // ! MATERIAL_SPECULARGLOSSINESS

#ifdef MATERIAL_METALLICROUGHNESS

#ifdef ID_metallicRoughnessTexture 
	vec4 mrSample = texture(u_MetallicRoughnessSampler, getMetallicRoughnessUV(info.vp));
#else
	vec4 mrSample = vec4(1.0);
#endif
	info.perceptualRoughness *= mrSample.g;
	info.metallic *= mrSample.b;

	// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
	// This layout intentionally reserves the 'r' channel for (optional) occlusion map data 
	perceptualRoughness = mrSample.g * params.roughnessFactor;
	metallic = mrSample.b * params.metallicFactor;
	diffuseColor = vec3(baseColor) * (vec3(1.0, 1.0, 1.0) - f0) * (1.0 - metallic);
	specularColor = mix(f0, vec3(baseColor), metallic);
#endif // ! MATERIAL_METALLICROUGHNESS

	perceptualRoughness = clamp(perceptualRoughness, 0.0f, 1.0f);
	metallic = clamp(metallic, 0.0f, 1.0f);
	return baseColor;
}

#ifdef MATERIAL_SPECULARGLOSSINESS

MaterialInfo getSpecularGlossinessInfo(MaterialInfo info)
{
	info.f0_dielectric = u_pbrParams.pbrSpecularFactor;
	info.perceptualRoughness = u_pbrParams.glossinessFactor;

#ifdef ID_specularGlossinessTexture
	vec4 sgSample = texture(u_SpecularGlossinessSampler, getSpecularGlossinessUV(info.vp));
#else
	vec4 sgSample = vec4(1.0);
#endif 
	info.perceptualRoughness *= sgSample.a; // glossiness to roughness
	info.f0_dielectric *= vec3(sgSample); // specular

	info.perceptualRoughness = 1.0 - info.perceptualRoughness; // 1 - glossiness
	return info;
}
#endif


MaterialInfo getMetallicRoughnessInfo(MaterialInfo info)
{
#ifdef MATERIAL_METALLICROUGHNESS
	info.metallic = u_pbrParams.metallicFactor;
	info.perceptualRoughness = u_pbrParams.roughnessFactor;

#ifdef ID_metallicRoughnessTexture
	// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
	// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
	vec4 mrSample = texture(u_MetallicRoughnessSampler, getMetallicRoughnessUV(info.vp));
	info.perceptualRoughness *= mrSample.g;
	info.metallic *= mrSample.b;
#endif

#endif
	return info;
}


#ifdef MATERIAL_SHEEN
MaterialInfo getSheenInfo(MaterialInfo info)
{
	info.sheenColorFactor = u_pbrParams.sheenColorFactor;
	info.sheenRoughnessFactor = u_pbrParams.sheenRoughnessFactor;

#ifdef ID_sheenColorTexture
	vec4 sheenColorSample = texture(u_SheenColorSampler, getSheenColorUV(info.vp));
	info.sheenColorFactor *= sheenColorSample.rgb;
#endif

#ifdef ID_sheenRoughnessTexture
	vec4 sheenRoughnessSample = texture(u_SheenRoughnessSampler, getSheenRoughnessUV(info.vp));
	info.sheenRoughnessFactor *= sheenRoughnessSample.a;
#endif
	return info;
}
#endif


#ifdef MATERIAL_SPECULAR
MaterialInfo getSpecularInfo(MaterialInfo info)
{
	vec4 specularTexture = vec4(1.0);
#ifdef ID_specularTexture
	specularTexture.a = texture(u_SpecularSampler, getSpecularUV(info.vp)).a;
#endif
#ifdef ID_specularColorTexture
	specularTexture.rgb = texture(u_SpecularColorSampler, getSpecularColorUV(info.vp)).rgb;
#endif

	info.f0_dielectric = min(info.f0_dielectric * u_pbrParams.specularColorFactor * vec3(specularTexture), vec3(1.0));
	info.specularWeight = u_pbrParams.specularFactor * specularTexture.a;
	info.f90_dielectric = vec3(info.specularWeight);
	return info;
}
#endif


#ifdef MATERIAL_TRANSMISSION
MaterialInfo getTransmissionInfo(MaterialInfo info)
{
	info.transmissionFactor = u_pbrParams.transmissionFactor;

#ifdef ID_transmissionTexture
	vec4 transmissionSample = texture(u_TransmissionSampler, getTransmissionUV(info.vp));
	info.transmissionFactor *= transmissionSample.r;
#endif

#ifdef MATERIAL_DISPERSION
	info.dispersion = u_pbrParams.dispersion;
#else
	info.dispersion = 0.0;
#endif
	return info;
}
#endif

#ifdef MATERIAL_VOLUME
MaterialInfo getVolumeInfo(MaterialInfo info)
{
	info.thickness = u_pbrParams.thicknessFactor;
	info.attenuationColor = u_pbrParams.attenuationColor;
	info.attenuationDistance = u_pbrParams.attenuationDistance;

#ifdef ID_thicknessTexture
	vec4 thicknessSample = texture(u_ThicknessSampler, getThicknessUV(info.vp));
	info.thickness *= thicknessSample.g;
#endif
	return info;
}
#endif


#ifdef MATERIAL_IRIDESCENCE
MaterialInfo getIridescenceInfo(MaterialInfo info)
{
	info.iridescenceFactor = u_pbrParams.iridescenceFactor;
	info.iridescenceIor = u_pbrParams.iridescenceIor;
	info.iridescenceThickness = u_pbrParams.iridescenceThicknessMaximum;

#ifdef ID_iridescenceTexture
	info.iridescenceFactor *= texture(u_IridescenceSampler, getIridescenceUV(info.vp)).r;
#endif

#ifdef ID_iridescenceThicknessTexture
	float thicknessSampled = texture(u_IridescenceThicknessSampler, getIridescenceThicknessUV(info.vp)).g;
	float thickness = mix(u_pbrParams.iridescenceThicknessMinimum, u_pbrParams.iridescenceThicknessMaximum, thicknessSampled);
	info.iridescenceThickness = thickness;
#endif

	return info;
}
#endif


#ifdef MATERIAL_DIFFUSE_TRANSMISSION
MaterialInfo getDiffuseTransmissionInfo(MaterialInfo info)
{
	info.diffuseTransmissionFactor = u_pbrParams.diffuseTransmissionFactor;
	info.diffuseTransmissionColorFactor = u_pbrParams.diffuseTransmissionColorFactor;

#ifdef ID_diffuseTransmissionTexture
	info.diffuseTransmissionFactor *= texture(u_DiffuseTransmissionSampler, getDiffuseTransmissionUV(info.vp)).a;
#endif

#ifdef ID_diffuseTransmissionColorTexture
	info.diffuseTransmissionColorFactor *= texture(u_DiffuseTransmissionColorSampler, getDiffuseTransmissionColorUV(info.vp)).rgb;
#endif

	return info;
}
#endif


#ifdef MATERIAL_CLEARCOAT
MaterialInfo getClearCoatInfo(MaterialInfo info, NormalInfo normalInfo)
{
	info.clearcoatFactor = u_pbrParams.clearcoatFactor;
	info.clearcoatRoughness = u_pbrParams.clearcoatRoughness;
	info.clearcoatF0 = vec3(pow((info.ior - 1.0) / (info.ior + 1.0), 2.0));
	info.clearcoatF90 = vec3(1.0);

#ifdef ID_clearcoatTexture
	vec4 clearcoatSample = texture(u_ClearcoatSampler, getClearcoatUV(info.vp));
	info.clearcoatFactor *= clearcoatSample.r;
#endif

#ifdef ID_clearcoatRoughnessTexture
	vec4 clearcoatSampleRoughness = texture(u_ClearcoatRoughnessSampler, getClearcoatRoughnessUV(info.vp));
	info.clearcoatRoughness *= clearcoatSampleRoughness.g;
#endif

	info.clearcoatNormal = getClearcoatNormal(normalInfo, info.vp);
	info.clearcoatRoughness = clamp(info.clearcoatRoughness, 0.0, 1.0);
	return info;
}
#endif


#ifdef MATERIAL_IOR
MaterialInfo getIorInfo(MaterialInfo info)
{
	info.f0_dielectric = vec3(pow((u_pbrParams.ior - 1.0) / (u_pbrParams.ior + 1.0), 2.0));
	info.ior = u_pbrParams.ior;
	return info;
}
#endif

#ifdef MATERIAL_ANISOTROPY
MaterialInfo getAnisotropyInfo(MaterialInfo info, NormalInfo normalInfo)
{
	vec2 direction = vec2(1.0, 0.0);
	float strengthFactor = 1.0;
#ifdef ID_AnisotropyTexture
	vec3 anisotropySample = texture(u_AnisotropySampler, getAnisotropyUV(info.vp)).xyz;
	direction = anisotropySample.xy * 2.0 - vec2(1.0);
	strengthFactor = anisotropySample.z;
#endif
	vec2 directionRotation = vec2(u_pbrParams.anisotropy); // cos(theta), sin(theta)
	mat2 rotationMatrix = mat2(directionRotation.x, directionRotation.y, -directionRotation.y, directionRotation.x);
	direction = rotationMatrix * direction;

	info.anisotropicT = mat3(normalInfo.t, normalInfo.b, normalInfo.n) * normalize(vec3(direction, 0.0));
	info.anisotropicB = cross(normalInfo.ng, info.anisotropicT);
	info.anisotropyStrength = clamp(u_pbrParams.anisotropy.z * strengthFactor, 0.0, 1.0);
	return info;
}
#endif

#ifdef ID_sheenELUT
float albedoSheenScalingLUT(float NdotV, float sheenRoughnessFactor)
{
	return texture(u_SheenELUT, vec2(NdotV, sheenRoughnessFactor)).r;
}
#endif
#ifdef ID_SSAO 
float GetSSAO(vec2 coords)
{
	return texture(ssaoSampler, coords).r;
}
#else
float GetSSAO(vec2 coords)
{
	return 1.0f;
}
#endif

// 阴影计算
// shadowmap filtering
float FilterShadow(int shadowIndex, vec3 uv)
{
	float shadow = 0.0;
#ifdef ID_shadowMap
	ivec2 texDim = textureSize(u_shadowMap[shadowIndex], 0);
	float scale = 1.0;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);


	int kernelLevel = 2;
	int kernelWidth = 2 * kernelLevel + 1;
	for (int i = -kernelLevel; i <= kernelLevel; i++)
	{
		for (int j = -kernelLevel; j <= kernelLevel; j++)
		{
			shadow += texture(u_shadowMap[shadowIndex], uv + vec3(dx * i, dy * j, 0)).r;
		}
	}

	shadow /= (kernelWidth * kernelWidth);
#endif
	return shadow;
}

//
// Project world space point onto shadowmap
//
float DoSpotShadow(vec3 vPosition, Light light)
{
#ifdef ID_shadowMap
	if (light.shadowMapIndex < 0)
		return 1.0f;

	if (light.type == LightType_Point)//|| light.type != LightType_Directional)
		return 1.0; // no other light types cast shadows for now

	vec4 shadowTexCoord = light.mLightViewProj * vec4(vPosition, 1.0);
#ifndef __cplusplus
	shadowTexCoord.xyz = shadowTexCoord.xyz / shadowTexCoord.w;
#endif
	// Re-scale to 0-1
	shadowTexCoord.x = (1.0 + shadowTexCoord.x) * 0.5;
	shadowTexCoord.y = (1.0 - shadowTexCoord.y) * 0.5;

	if (light.type == LightType_Spot)
	{
		if ((shadowTexCoord.y < 0) || (shadowTexCoord.y > 1)) return 0;
		if ((shadowTexCoord.x < 0) || (shadowTexCoord.x > 1)) return 0;
		if (shadowTexCoord.z < 0.0f) return 0.0f;
		if (shadowTexCoord.z > 1.0f) return 1.0f;
	}
	else if (light.type == LightType_Directional)
	{
		// This is the sun, so outside of the volume we do have light
		if ((shadowTexCoord.y < 0) || (shadowTexCoord.y > 1)) return 1.0f;
		if ((shadowTexCoord.x < 0) || (shadowTexCoord.x > 1)) return 1.0f;
		if (shadowTexCoord.z < 0.0f) return 1.0f;
		if (shadowTexCoord.z > 1.0f) return 1.0f;
	}

	shadowTexCoord.z -= light.depthBias;

	return FilterShadow(light.shadowMapIndex, vec3(shadowTexCoord));
#else
	return 1.0f;
#endif
}


#endif // 1
// 内部pbr用
struct MaterialInfo_old
{
	vec4 baseColor;
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	float alpha;
	vec3 reflectance0;            // full reflectance color (normal incidence angle)

	vec3 diffuseColor;            // color contribution from diffuse lighting

	vec3 reflectance90;           // reflectance color at grazing angle
	vec3 specularColor;           // color contribution from specular lighting

};
struct AngularInfo
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector

	float VdotH;                  // cos angle between view direction and half vector

	vec3 padding;
};

// Lambert lighting
// see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
vec3 diffuse(MaterialInfo_old materialInfo)
{
	return materialInfo.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(MaterialInfo_old materialInfo, AngularInfo angularInfo)
{
	return materialInfo.reflectance0 + (materialInfo.reflectance90 - materialInfo.reflectance0) * pow(clamp(1.0 - angularInfo.VdotH, 0.0, 1.0), 5.0);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float visibilityOcclusion(MaterialInfo_old materialInfo, AngularInfo angularInfo)
{
	float NdotL = angularInfo.NdotL;
	float NdotV = angularInfo.NdotV;
	float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;

	float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
	float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

	float GGX = GGXV + GGXL;
	if (GGX > 0.0)
	{
		return 0.5 / GGX;
	}
	return 0.0;
}
float microfacetDistribution(MaterialInfo_old materialInfo, AngularInfo angularInfo)
{
	float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;
	float f = (angularInfo.NdotH * alphaRoughnessSq - angularInfo.NdotH) * angularInfo.NdotH + 1.0;
	return alphaRoughnessSq / (M_PI * f * f + 0.000001f);
}


AngularInfo getAngularInfo(vec3 pointToLight, vec3 normal, vec3 view)
{
	// Standard one-letter names
	vec3 n = normalize(normal);           // Outward direction of surface point
	vec3 v = normalize(view);             // Direction from surface point to view
	vec3 l = normalize(pointToLight);     // Direction from surface point to light
	vec3 h = normalize(l + v);            // Direction of the vector between l and v
	float c0 = 0.0f, c1 = 1.0f;
	float NdotL = clamp(dot(n, l), c0, c1);
	float NdotV = clamp(dot(n, v), c0, c1);
	float NdotH = clamp(dot(n, h), c0, c1);
	float LdotH = clamp(dot(l, h), c0, c1);
	float VdotH = clamp(dot(v, h), c0, c1);

	return AngularInfo(
		NdotL,
		NdotV,
		NdotH,
		LdotH,
		VdotH,
		vec3(0, 0, 0)
	);
}
vec3 getPointShade(vec3 pointToLight, MaterialInfo_old materialInfo, vec3 normal, vec3 view)
{
	AngularInfo angularInfo = getAngularInfo(pointToLight, normal, view);

	if (angularInfo.NdotL > 0.0 || angularInfo.NdotV > 0.0)
	{
		// Calculate the shading terms for the microfacet specular shading model
		vec3 F = specularReflection(materialInfo, angularInfo);
		float Vis = visibilityOcclusion(materialInfo, angularInfo);
		float D = microfacetDistribution(materialInfo, angularInfo);

		// Calculation of analytical lighting contribution
#ifndef __cplusplus
		vec3 diffuseContrib = (1.0 - F) * diffuse(materialInfo);
#else
		vec3 f1 = vec3(1, 1, 1) - F;
		vec3 diffuseContrib = f1 * diffuse(materialInfo);
#endif // __cplusplus 
		vec3 specContrib = F * Vis * D;

		// Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
		return angularInfo.NdotL * (diffuseContrib + specContrib);
	}

	return vec3(0.0, 0.0, 0.0);
}
vec3 applyDirectionalLight(Light light, MaterialInfo_old materialInfo, vec3 normal, vec3 view)
{
	vec3 pointToLight = light.direction;
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	return light.intensity * light.color * shade;
}

vec3 applyPointLight(Light light, MaterialInfo_old materialInfo, vec3 normal, vec3 worldPos, vec3 view)
{
	vec3 pointToLight = light.position - worldPos;
	float distance = length(pointToLight);
	float attenuation = getRangeAttenuation(light.range, distance);
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	return attenuation * light.intensity * light.color * shade;
}

vec3 applySpotLight(Light light, MaterialInfo_old materialInfo, vec3 normal, vec3 worldPos, vec3 view)
{
	vec3 pointToLight = light.position - worldPos;
	float distance = length(pointToLight);
	float rangeAttenuation = getRangeAttenuation(light.range, distance);
	float spotAttenuation = getSpotAttenuation(pointToLight, -light.direction, light.outerConeCos, light.innerConeCos);
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	return rangeAttenuation * spotAttenuation * light.intensity * light.color * shade;
}
#ifdef USE_IBL
vec3 getIBLContribution(MaterialInfo_old materialInfo, vec3 n, vec3 v)
{
	float NdotV = clamp(dot(n, v), 0.0f, 1.0f);

	float u_MipCount = 9.0; // resolution of 512x512 of the IBL
	float lod = clamp(materialInfo.perceptualRoughness * float(u_MipCount - 1.0f), 0.0f, float(u_MipCount));
	vec3 reflection = normalize(reflect(-v, n));

	vec2 brdfSamplePoint = clamp(vec2(NdotV, materialInfo.perceptualRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
	// retrieve a scale and bias to F0. See [1], Figure 3
#ifndef __cplusplus
	vec2 brdf = texture(u_brdfLUT, brdfSamplePoint).rg;

	vec3 diffuseLight = texture(u_DiffuseEnvSampler, n).rgb;

#ifdef USE_TEX_LOD
	vec3 specularLight = textureLod(u_SpecularEnvSampler, reflection, lod).rgb;
#else
	vec3 specularLight = texture(u_SpecularEnvSampler, reflection).rgb;
#endif

	vec3 d = diffuseLight * materialInfo.diffuseColor;
	vec3 specular = specularLight * (materialInfo.specularColor * brdf.x + brdf.y);
	return d + specular;
#else
	return vec3(0.0);
#endif
}
#endif
vec3 doPbrLighting(MaterialInfo m, vec2 uv, vec3 diffuseColor, vec3 specularColor, float perceptualRoughness, vec4 baseColor)
{
	vec3 outColor = vec3(baseColor);
#ifdef __cplusplus
	//pbrMaterial u_pbrParams = {}; 
#endif
#ifdef MATERIAL_UNLIT
	return outColor;
#endif

	float alphaRoughness = perceptualRoughness * perceptualRoughness;

	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	vec3 specularEnvironmentR0 = specularColor;
	// Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
	vec3 specularEnvironmentR90 = vec3(clamp(reflectance * 50.0, 0.0, 1.0));

	MaterialInfo_old materialInfo;
	materialInfo.perceptualRoughness = perceptualRoughness;
	materialInfo.reflectance0 = specularEnvironmentR0;
	materialInfo.alphaRoughness = alphaRoughness;
	materialInfo.diffuseColor = diffuseColor;
	materialInfo.reflectance90 = specularEnvironmentR90;
	materialInfo.specularColor = specularColor;

	// LIGHTING

	vec3 color = vec3(0.0, 0.0, 0.0);
	vec3 normal = getPixelNormal(m.vp);
	vec3 worldPos = Input.WorldPos;
#ifdef __cplusplus
	vec3 cpos = myPerFrame.u_CameraPos;
	vec3 view = normalize(cpos - worldPos);
#else
	vec3 view = normalize(myPerFrame.u_CameraPos.xyz - worldPos);
#endif

#if (DEF_doubleSided == 1)
	if (dot(normal, view) < 0)
	{
		normal = -normal;
	}
#endif

#ifdef USE_PUNCTUAL
	for (int i = 0; i < myPerFrame.u_lightCount; ++i)
	{
		Light light = myPerFrame.u_lights[i];
		float shadowFactor = DoSpotShadow(worldPos, light);
		color = light.color;
		if (light.type == LightType_Directional)
		{
			color = applyDirectionalLight(light, materialInfo, normal, view) * shadowFactor;
		}
		else if (light.type == LightType_Point)
		{
			color = applyPointLight(light, materialInfo, normal, worldPos, view);
		}
		else if (light.type == LightType_Spot)
		{
			color = applySpotLight(light, materialInfo, normal, worldPos, view) * shadowFactor;
		}
	}
#endif

	// Calculate lighting contribution from image based lighting source (IBL)
#ifdef USE_IBL
	color += getIBLContribution(materialInfo, normal, view) * myPerFrame.u_iblFactor * GetSSAO(gl_FragCoord.xy * myPerFrame.u_invScreenResolution);
#endif

	float ao = 1.0;
	// Apply optional PBR terms for additional (optional) shading
#ifdef ID_occlusionTexture
	ao = texture(u_OcclusionSampler, uv).r;// getOcclusionUV(Input)).r;
	color = color * ao; //mix(color, color * ao, myPerFrame.u_OcclusionStrength);
#endif

	vec3 emissive = vec3(0);
#ifdef ID_emissiveTexture
	emissive = (texture(u_EmissiveSampler, uv)).rgb * u_pbrParams.emissiveFactor.rgb * myPerFrame.u_EmissiveFactor;
#else
	emissive = u_pbrParams.emissiveFactor * myPerFrame.u_EmissiveFactor;
#endif
	color += emissive;

	return color;
}
vec3 get_light_intensity(Light light, MaterialInfo_old materialInfo, vec3 normal, vec3 view, vec3 worldPos) {
	vec3 color = light.color;
	if (light.type == LightType_Directional)
	{
		color = applyDirectionalLight(light, materialInfo, normal, view);
	}
	else if (light.type == LightType_Point)
	{
		color = applyPointLight(light, materialInfo, normal, worldPos, view);
	}
	else if (light.type == LightType_Spot)
	{
		color = applySpotLight(light, materialInfo, normal, worldPos, view);
	}
	return color;
}


vec3 applyDirectionalLight1(Light light, MaterialInfo_old materialInfo, vec3 normal, vec3 view, vec3 pointToLight)
{
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	return light.intensity * light.color * shade;
}

vec3 applyPointLight1(Light light, MaterialInfo_old materialInfo, vec3 normal, vec3 view, vec3 pointToLight)
{
	float distance = length(pointToLight);
	float attenuation = getRangeAttenuation(light.range, distance);
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	return attenuation * light.intensity * light.color * shade;
}

vec3 applySpotLight1(Light light, MaterialInfo_old materialInfo, vec3 normal, vec3 view, vec3 pointToLight)
{
	float distance = length(pointToLight);
	float rangeAttenuation = getRangeAttenuation(light.range, distance);
	float spotAttenuation = getSpotAttenuation(pointToLight, -light.direction, light.outerConeCos, light.innerConeCos);
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	return rangeAttenuation * spotAttenuation * light.intensity * light.color * shade;
}

vec3 get_light_intensity1(Light light, MaterialInfo_old materialInfo, vec3 normal, vec3 view, vec3 pointToLight) {
	vec3 color = light.color;
	if (light.type == LightType_Directional)
	{
		color = applyDirectionalLight1(light, materialInfo, normal, view, pointToLight);
	}
	else if (light.type == LightType_Point)
	{
		color = applyPointLight1(light, materialInfo, normal, view, pointToLight);
	}
	else if (light.type == LightType_Spot)
	{
		color = applySpotLight1(light, materialInfo, normal, view, pointToLight);
	}
	return color;
}

vec3 do_punctual(MaterialInfo materialInfo, float clearcoatFactor, vec3 clearcoatFresnel, vec3 v, vec3 n, float diffuseTransmissionThickness)
{
	vec3 color = vec3(0);
#ifdef USE_PUNCTUAL
	float perceptualRoughness;
	float metallic;
	vec3 diffuseColor;
	vec3 specularColor;
	vec4 color1 = get_roughness(materialInfo, u_pbrParams, materialInfo.vp.v_texcoord[0], diffuseColor, specularColor, perceptualRoughness, metallic);
	color = vec3(color1);

	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
	// Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
	vec3 specularEnvironmentR90 = vec3(clamp(reflectance * 50.0, 0.0, 1.0));

	MaterialInfo_old mo;
	mo.perceptualRoughness = materialInfo.perceptualRoughness;
	mo.reflectance0 = specularColor;
	mo.alphaRoughness = materialInfo.alphaRoughness;
	mo.diffuseColor = diffuseColor;
	mo.reflectance90 = specularEnvironmentR90;
	mo.specularColor = specularColor;

	for (int i = 0; i < myPerFrame.u_lightCount; ++i)
	{
		Light light = myPerFrame.u_lights[i];

		vec3 pointToLight;
		if (light.type != LightType_Directional)
		{
			pointToLight = light.position - materialInfo.vp.v_Position;
		}
		else
		{
			pointToLight = light.direction;//-
		}

		float shadowFactor = DoSpotShadow(materialInfo.vp.v_Position, light);
		// BSTF
		vec3 l = normalize(pointToLight);   // Direction from surface point to light
		vec3 h = normalize(l + v);          // Direction of the vector between l and v, called halfway vector
		float NdotL = clampedDot(n, l);
		float NdotV = clampedDot(n, v);
		float NdotH = clampedDot(n, h);
		float LdotH = clampedDot(l, h);
		float VdotH = clampedDot(v, h);

		vec3 dielectric_fresnel = F_Schlick(materialInfo.f0_dielectric * materialInfo.specularWeight, materialInfo.f90_dielectric, abs(VdotH));
		vec3 metal_fresnel = F_Schlick(vec3(materialInfo.baseColor), vec3(1.0), abs(VdotH));

		vec3 lightIntensity = getLighIntensity(light, pointToLight);
		vec3 lightIntensity1 = get_light_intensity(light, mo, n, v, materialInfo.vp.v_Position);
		return color + lightIntensity1 * shadowFactor;
		vec3 l_diffuse = lightIntensity * NdotL * BRDF_lambertian(vec3(materialInfo.baseColor));
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
		vec3 transmissionRay = getVolumeTransmissionRay(n, v, materialInfo.thickness, materialInfo.ior, materialInfo.vp.u_ModelMatrix);
		pointToLight -= transmissionRay;
		l = normalize(pointToLight);

		vec3 transmittedLight = lightIntensity * getPunctualRadianceTransmission(n, v, l, materialInfo.alphaRoughness, materialInfo.f0_dielectric, materialInfo.f90, materialInfo.baseColor.rgb, materialInfo.ior);

#ifdef MATERIAL_VOLUME
		transmittedLight = applyVolumeAttenuation(transmittedLight, length(transmissionRay), materialInfo.attenuationColor, materialInfo.attenuationDistance);
#endif
		l_diffuse = mix(l_diffuse, transmittedLight, materialInfo.transmissionFactor);
#endif

		// Calculation of analytical light
		// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
		vec3 intensity = get_light_intensity1(light, mo, n, v, pointToLight); //getLighIntensity(light, pointToLight);

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

#ifdef ID_charlieLUT
		l_sheen = intensity * getPunctualRadianceSheen(materialInfo.sheenColorFactor, materialInfo.sheenRoughnessFactor, NdotL, NdotV, NdotH);
#endif
#ifdef ID_sheenELUT
		l_albedoSheenScaling = min(1.0 - max3(materialInfo.sheenColorFactor) * albedoSheenScalingLUT(NdotV, materialInfo.sheenRoughnessFactor),
			1.0 - max3(materialInfo.sheenColorFactor) * albedoSheenScalingLUT(NdotL, materialInfo.sheenRoughnessFactor));
#endif

		vec3 l_color = mix(l_dielectric_brdf, l_metal_brdf, materialInfo.metallic);
		l_color = l_sheen + l_color * l_albedoSheenScaling;
		l_color = mix(l_color, l_clearcoat_brdf, clearcoatFactor * clearcoatFresnel);

		//color += l_color;// *shadowFactor;
		color = vec3(lightIntensity);
		//color = vec3(shadowFactor); //阴影正常
	}
#endif // USE_PUNCTUAL
	return color;
}

vec3 get_ibl(MaterialInfo materialInfo, float clearcoatFactor, vec3 clearcoatFresnel, vec3 v, vec3 n, float diffuseTransmissionThickness)
{
	vec3 color = vec3(0.0);
	vec3 f_specular_dielectric = vec3(0.0);
	vec3 f_specular_metal = vec3(0.0);
	vec3 f_dielectric_brdf_ibl = vec3(0.0);
	vec3 f_metal_brdf_ibl = vec3(0.0);
	vec3 f_sheen = vec3(0.0);
	vec3 clearcoat_brdf = vec3(0.0);
	float albedoSheenScaling = 1.0;

	vec3 f_specular_transmission = vec3(0.0);
	//vec3 f_diffuse_transmission = vec3(0.0);
	// Calculate lighting contribution from image based lighting source (IBL)
#ifdef USE_IBL

	vec3 f_diffuse = getDiffuseLight(n) * vec3(materialInfo.baseColor);

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
		materialInfo.baseColor.rgb, materialInfo.f0_dielectric, materialInfo.f90,
		materialInfo.vp.v_Position, materialInfo.vp.u_ModelMatrix, materialInfo.vp.u_ViewMatrix, //u_ProjectionMatrix,
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
#ifdef ID_brdfTexture
	// todo lut有问题？
	vec3 f_metal_fresnel_ibl = getIBLGGXFresnel(n, v, materialInfo.perceptualRoughness, vec3(materialInfo.baseColor), 1.0);
	f_metal_brdf_ibl = f_metal_fresnel_ibl * f_specular_metal;

	vec3 f_dielectric_fresnel_ibl = getIBLGGXFresnel(n, v, materialInfo.perceptualRoughness, materialInfo.f0_dielectric, materialInfo.specularWeight);
	f_dielectric_brdf_ibl = mix(f_diffuse, f_specular_dielectric, f_dielectric_fresnel_ibl);

#ifdef MATERIAL_IRIDESCENCE
	f_metal_brdf_ibl = mix(f_metal_brdf_ibl, f_specular_metal * iridescenceFresnel_metallic, materialInfo.iridescenceFactor);
	f_dielectric_brdf_ibl = mix(f_dielectric_brdf_ibl, rgb_mix(f_diffuse, f_specular_dielectric, iridescenceFresnel_dielectric), materialInfo.iridescenceFactor);
#endif
#endif 

#ifdef MATERIAL_CLEARCOAT
	clearcoat_brdf = getIBLRadianceGGX(materialInfo.clearcoatNormal, v, materialInfo.clearcoatRoughness);
#endif

#ifdef ID_charlieLUT
	f_sheen = getIBLRadianceCharlie(n, v, materialInfo.sheenRoughnessFactor, materialInfo.sheenColorFactor);
#endif
#ifdef ID_sheenELUT
	albedoSheenScaling = 1.0 - max3(materialInfo.sheenColorFactor) * albedoSheenScalingLUT(NdotV, materialInfo.sheenRoughnessFactor);
#endif

	color = mix(f_dielectric_brdf_ibl, f_metal_brdf_ibl, materialInfo.metallic);
	color = f_sheen + color * albedoSheenScaling; //光泽度
	color = mix(color, clearcoat_brdf, clearcoatFactor * clearcoatFresnel);//清漆
	// 测试vec3(albedoSheenScaling);
	//color = f_dielectric_brdf_ibl;
	//color = vec3(materialInfo.metallic);//正常
	color *= myPerFrame.u_iblFactor * GetSSAO(gl_FragCoord.xy * myPerFrame.u_invScreenResolution);
	float ao = 1.0;
#ifdef ID_occlusionTexture
	ao = texture(u_OcclusionSampler, getOcclusionUV(materialInfo.vp)).r;
	color = color * (1.0 + u_pbrParams.occlusionStrength * (ao - 1.0));
#endif

#endif //end USE_IBL
	return color;
}

// pbr材质计算主函数，返回vec4颜色
#if 1
vec4 pbr_main(vsio_ps vp)
{
	vec4 finalColor = vec4(0.0);
	vec4 baseColor = getBaseColor(vp);

	// Late discard to avoid sampling artifacts. See https://github.com/KhronosGroup/glTF-Sample-Viewer/issues/267 
#if defined(DEF_alphaMode_BLEND)
	if (baseColor.a <= 0)
		discard;
#elif defined(DEF_alphaMode_MASK) && defined(DEF_alphaCutoff)
	if (baseColor.a < DEF_alphaCutoff)
		discard;
#else
	//OPAQUE
#endif
	vec3 color = vec3(0.0);

	vec3 v = normalize(vec3(myPerFrame.u_CameraPos) - vp.v_Position);
	NormalInfo normalInfo = getNormalInfo(vp);
	vec3 normal = getPixelNormal(vp); 
	vec3 n = normalInfo.n;
	vec3 t = normalInfo.t;
	vec3 b = normalInfo.b;
	float NdotV = clampedDot(n, v);
	float TdotV = clampedDot(t, v);
	float BdotV = clampedDot(b, v);


	MaterialInfo materialInfo;
	materialInfo.baseColor = vec3(baseColor);
	materialInfo.vp = vp;
	// The default index of refraction of 1.5 yields a dielectric normal incidence reflectance of 0.04.
	materialInfo.ior = 1.5;
	materialInfo.f0_dielectric = vec3(0.04);
	materialInfo.specularWeight = 1.0;

	// Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
	materialInfo.f90 = vec3(1.0);
	materialInfo.f90_dielectric = materialInfo.f90;

#if 0

	float perceptualRoughness;
	float metallic;
	vec3 diffuseColor;
	vec3 specularColor;
	vec4 color1 = get_roughness(materialInfo, u_pbrParams, vp.v_texcoord[0], diffuseColor, specularColor, perceptualRoughness, metallic);
	color1 += vec4(doPbrLighting(materialInfo, vp.v_texcoord[0], diffuseColor, specularColor, perceptualRoughness, color1), color1.a);
	//color = color1;
	return color1;
#endif
	// If the MR debug output is selected, we have to enforce evaluation of the non-iridescence BRDF functions.
#if DEBUG == DEBUG_METALLIC_ROUGHNESS
#undef MATERIAL_IRIDESCENCE
#endif

#ifdef MATERIAL_IOR
	materialInfo = getIorInfo(materialInfo);
#endif
#ifdef MATERIAL_METALLICROUGHNESS
	materialInfo = getMetallicRoughnessInfo(materialInfo);
#endif
#ifdef MATERIAL_SPECULARGLOSSINESS
	materialInfo.metallic = 0;
	materialInfo.perceptualRoughness = 1;
	materialInfo = getSpecularGlossinessInfo(materialInfo);
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

	//color = vec3(baseColor);
#ifdef MATERIAL_UNLIT
#else
	materialInfo.perceptualRoughness = clamp(materialInfo.perceptualRoughness, 0.0, 1.0);
	materialInfo.metallic = clamp(materialInfo.metallic, 0.0, 1.0);

	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness.
	materialInfo.alphaRoughness = materialInfo.perceptualRoughness * materialInfo.perceptualRoughness;


	// LIGHTING 
	vec3 f_emissive = vec3(0.0);

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
		(length(vec3(vp.u_ModelMatrix[0])) + length(vec3(vp.u_ModelMatrix[1])) + length(vec3(vp.u_ModelMatrix[2]))) / 3.0;
#endif
#endif

#ifdef MATERIAL_CLEARCOAT
	clearcoatFactor = materialInfo.clearcoatFactor;
	clearcoatFresnel = F_Schlick(materialInfo.clearcoatF0, materialInfo.clearcoatF90, clampedDot(materialInfo.clearcoatNormal, v));
#endif
	// todo 计算ibl
	//color = get_ibl(materialInfo, clearcoatFactor, clearcoatFresnel, v, n, diffuseTransmissionThickness);
	f_emissive = u_pbrParams.emissiveFactor; // 自发光
#ifdef MATERIAL_EMISSIVE_STRENGTH
	f_emissive *= u_pbrParams.emissiveStrength;
#endif
#ifdef ID_emissiveTexture
	f_emissive *= vec3(texture(u_EmissiveSampler, getEmissiveUV(vp)));
#endif

	//计算灯光
	//color += do_punctual(materialInfo, clearcoatFactor, clearcoatFresnel, v, n, diffuseTransmissionThickness);

#ifdef USE_PUNCTUAL
	float perceptualRoughness;
	float metallic;
	vec3 diffuseColor;
	vec3 specularColor;
	vec4 color1 = get_roughness(materialInfo, u_pbrParams, materialInfo.vp.v_texcoord[0], diffuseColor, specularColor, perceptualRoughness, metallic);
	color = vec3(color1);

	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
	// Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
	vec3 specularEnvironmentR90 = vec3(clamp(reflectance * 50.0, 0.0, 1.0));

	MaterialInfo_old mo;
	mo.perceptualRoughness = materialInfo.perceptualRoughness;
	mo.reflectance0 = specularColor;
	mo.alphaRoughness = materialInfo.alphaRoughness;
	mo.diffuseColor = diffuseColor;
	mo.reflectance90 = specularEnvironmentR90;
	mo.specularColor = specularColor;

	for (int i = 0; i < myPerFrame.u_lightCount; ++i)
	{
		Light light = myPerFrame.u_lights[i];

		vec3 pointToLight;
		if (light.type != LightType_Directional)
		{
			pointToLight = light.position - materialInfo.vp.v_Position;
		}
		else
		{
			pointToLight = light.direction;//-
		}

		float shadowFactor = DoSpotShadow(materialInfo.vp.v_Position, light);
		// BSTF
		vec3 l = normalize(pointToLight);   // Direction from surface point to light
		vec3 h = normalize(l + v);          // Direction of the vector between l and v, called halfway vector
		float NdotL = clampedDot(n, l);
		float NdotV = clampedDot(n, v);
		float NdotH = clampedDot(n, h);
		float LdotH = clampedDot(l, h);
		float VdotH = clampedDot(v, h);

		vec3 dielectric_fresnel = F_Schlick(materialInfo.f0_dielectric * materialInfo.specularWeight, materialInfo.f90_dielectric, abs(VdotH));
		vec3 metal_fresnel = F_Schlick(vec3(materialInfo.baseColor), vec3(1.0), abs(VdotH));

		vec3 lightIntensity = getLighIntensity(light, pointToLight);
		vec3 lightIntensity1 = get_light_intensity(light, mo, n, v, materialInfo.vp.v_Position);// *shadowFactor;
		color += vec3(lightIntensity);
		break;
		vec3 l_diffuse = lightIntensity1 * NdotL * BRDF_lambertian(vec3(materialInfo.baseColor));
		vec3 l_specular_dielectric = vec3(0.0);
		vec3 l_specular_metal = vec3(0.0);
		vec3 l_dielectric_brdf = vec3(0.0);
		vec3 l_metal_brdf = vec3(0.0);
		vec3 l_clearcoat_brdf = vec3(0.0);
		vec3 l_sheen = vec3(0.0);
		float l_albedoSheenScaling = 1.0;
		//color += lightIntensity1;
		//color += lightIntensity1 * NdotL * BRDF_lambertian(vec3(materialInfo.baseColor));

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
		vec3 transmissionRay = getVolumeTransmissionRay(n, v, materialInfo.thickness, materialInfo.ior, materialInfo.vp.u_ModelMatrix);
		pointToLight -= transmissionRay;
		l = normalize(pointToLight);

		vec3 transmittedLight = lightIntensity * getPunctualRadianceTransmission(n, v, l, materialInfo.alphaRoughness, materialInfo.f0_dielectric, materialInfo.f90, materialInfo.baseColor.rgb, materialInfo.ior);

#ifdef MATERIAL_VOLUME
		transmittedLight = applyVolumeAttenuation(transmittedLight, length(transmissionRay), materialInfo.attenuationColor, materialInfo.attenuationDistance);
#endif
		l_diffuse = mix(l_diffuse, transmittedLight, materialInfo.transmissionFactor);
#endif

		// Calculation of analytical light
		// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
		vec3 intensity = get_light_intensity1(light, mo, n, v, pointToLight) * shadowFactor; //getLighIntensity(light, pointToLight);

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

#ifdef ID_charlieLUT
		l_sheen = intensity * getPunctualRadianceSheen(materialInfo.sheenColorFactor, materialInfo.sheenRoughnessFactor, NdotL, NdotV, NdotH);
#endif
#ifdef ID_sheenELUT
		l_albedoSheenScaling = min(1.0 - max3(materialInfo.sheenColorFactor) * albedoSheenScalingLUT(NdotV, materialInfo.sheenRoughnessFactor),
			1.0 - max3(materialInfo.sheenColorFactor) * albedoSheenScalingLUT(NdotL, materialInfo.sheenRoughnessFactor));
#endif

		vec3 l_color = mix(l_dielectric_brdf, l_metal_brdf, materialInfo.metallic);
		l_color = l_sheen + l_color * l_albedoSheenScaling;
		l_color = mix(l_color, l_clearcoat_brdf, clearcoatFactor * clearcoatFresnel);

		color += l_color;
		//color += vec3(lightIntensity);
		//color = vec3(shadowFactor); //阴影正常
	}
#endif // USE_PUNCTUAL
	color += f_emissive * (1.0f - clearcoatFactor * clearcoatFresnel);
#endif

#ifndef DEBUG_NONE
#define DEBUG_NONE 0
#endif // !DEBUG_NONE
#ifndef DEBUG
#define DEBUG DEBUG_NONE
#endif // !DEBUG

	finalColor = vec4(color, baseColor.a);
	//finalColor = vec4(materialInfo.perceptualRoughness);
	//finalColor = vec4(baseColor.rgb + f_emissive, baseColor.a);

#ifdef TONEMAP_OPT
	finalColor = vec4(toneMap(color), baseColor.a);
#endif
#ifdef DB_PX
	def __cplusplus
#ifdef PXOUT_rcd	 
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
#ifdef ID_specularColorTexture
	specularTexture.rgb = texture(u_SpecularColorSampler, getSpecularColorUV(vp)).rgb;
#endif
	finalColor.rgb = u_pbrParams.specularColorFactor * specularTexture.rgb;
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
	finalColor.rgb = vec3(materialInfo.thickness / u_pbrParams.thicknessFactor);
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
#ifdef ID_AnisotropyTexture
	direction = texture(u_AnisotropySampler, getAnisotropyUV(vp)).xy;
	direction = direction * 2.0 - vec2(1.0); // [0, 1] -> [-1, 1]
#endif
	vec2 directionRotation = u_pbrParams.anisotropy.xy; // cos(theta), sin(theta)
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
#endif
	return finalColor;
}
#endif // !pbr_glsl



#endif // !pbrpx_h_
