#ifndef pbrpixel_h_
#define pbrpixel_h_
#ifndef LIGHT_COUNT
#define LIGHT_COUNT 3
#endif // !LIGHT_COUNT

struct pbrMaterial_t {
	// Metallic Roughness
	float u_MetallicFactor;
	float u_RoughnessFactor;
	vec4 u_BaseColorFactor;

	// Sheen
	float u_SheenRoughnessFactor;
	vec3 u_SheenColorFactor;

	// Clearcoat
	float u_ClearcoatFactor;
	float u_ClearcoatRoughnessFactor;

	// Specular
	vec3 u_KHR_materials_specular_specularColorFactor;
	float u_KHR_materials_specular_specularFactor;

	// Transmission
	float u_TransmissionFactor;

	// Volume
	float u_ThicknessFactor;
	vec3 u_AttenuationColor;
	float u_AttenuationDistance;

	// Iridescence
	float u_IridescenceFactor;
	float u_IridescenceIor;
	float u_IridescenceThicknessMinimum;
	float u_IridescenceThicknessMaximum;

	// Diffuse Transmission
	float u_DiffuseTransmissionFactor;
	vec3 u_DiffuseTransmissionColorFactor;

	// Emissive Strength
	float u_EmissiveStrength;

	// IOR
	float u_Ior;

	// Anisotropy
	vec3 u_Anisotropy;

	// Dispersion
	float u_Dispersion;

	// Alpha mode
	float u_AlphaCutoff;

	// MATERIAL_TRANSMISSION
	// ivec2 u_ScreenSize;
};
struct pbrTexture_t {
	// General Material
	int u_NormalSampler;	// 纹理数组id
	float u_NormalScale;
	int u_NormalUVSet;

	vec3 u_EmissiveFactor;
	int u_EmissiveSampler;
	int u_EmissiveUVSet;

	int u_OcclusionSampler;
	int u_OcclusionUVSet;
	float u_OcclusionStrength;

	int u_BaseColorSampler;
	int u_BaseColorUVSet;

	int u_MetallicRoughnessSampler;
	int u_MetallicRoughnessUVSet;

	int u_DiffuseSampler;
	int u_DiffuseUVSet;

	int u_SpecularGlossinessSampler;
	int u_SpecularGlossinessUVSet;

	int u_ClearcoatSampler;
	int u_ClearcoatUVSet;

	int u_ClearcoatRoughnessSampler;
	int u_ClearcoatRoughnessUVSet;

	int u_ClearcoatNormalSampler;
	int u_ClearcoatNormalUVSet;
	float u_ClearcoatNormalScale;

	int u_SheenColorSampler;
	int u_SheenColorUVSet;
	int u_SheenRoughnessSampler;
	int u_SheenRoughnessUVSet;

	int u_SpecularSampler;
	int u_SpecularUVSet;
	int u_SpecularColorSampler;
	int u_SpecularColorUVSet;

	int u_TransmissionSampler;
	int u_TransmissionUVSet;
	int u_TransmissionFramebufferSampler;
	ivec2 u_TransmissionFramebufferSize;

	int u_ThicknessSampler;
	int u_ThicknessUVSet;

	int u_IridescenceSampler;
	int u_IridescenceUVSet;

	int u_IridescenceThicknessSampler;
	int u_IridescenceThicknessUVSet;

	int u_DiffuseTransmissionSampler;
	int u_DiffuseTransmissionUVSet;

	int u_DiffuseTransmissionColorSampler;
	int u_DiffuseTransmissionColorUVSet;

	int u_AnisotropySampler;
	int u_AnisotropyUVSet;
};
//756,84
struct pbrTextureMat_t {
	int u_NormalUVTransform;
	int u_EmissiveUVTransform;
	int u_OcclusionUVTransform;
	int u_BaseColorUVTransform;
	int u_MetallicRoughnessUVTransform;
	int u_DiffuseUVTransform;
	int u_SpecularGlossinessUVTransform;
	int u_ClearcoatUVTransform;
	int u_ClearcoatRoughnessUVTransform;
	int u_ClearcoatNormalUVTransform;
	int u_SheenColorUVTransform;
	int u_SheenRoughnessUVTransform;
	int u_SpecularUVTransform;
	int u_SpecularColorUVTransform;
	int u_TransmissionUVTransform;
	int u_ThicknessUVTransform;
	int u_IridescenceUVTransform;
	int u_IridescenceThicknessUVTransform;
	int u_DiffuseTransmissionUVTransform;
	int u_DiffuseTransmissionColorUVTransform;
	int u_AnisotropyUVTransform;
};

#ifdef USE_PUNCTUAL
// KHR_lights_punctual extension.
// see https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual
struct Light_t
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


#endif
struct perFrame_t
{
	mat4 u_ModelMatrix;
	mat4 u_ViewMatrix;
	mat4 u_ProjectionMatrix;

	mat3 u_EnvRotation;
	// 相机坐标
	vec3 u_Camera;
	float u_Exposure;
	float u_EnvIntensity;
	// IBL
	int u_MipCount;
	int u_LambertianEnvSampler;
	int u_GGXEnvSampler;
	int u_GGXLUT;
	int u_CharlieEnvSampler;
	int u_CharlieLUT;
	int u_SheenELUT;

#ifdef USE_PUNCTUAL
	Light_t u_Lights[LIGHT_COUNT + 1]; //Array [0] is not allowed
#endif
};


#ifdef __cplusplus
perFrame_t u_perFrame;
mat4 u_mCurrWorld;
mat4 u_mPrevWorld;
mat3 u_matuv[1];
pbrMaterial_t u_pbr;
pbrTexture_t u_tex;
pbrTextureMat_t u_tex_mat;
#else
layout(set = 0, binding = 0) uniform perFrame
{
	perFrame_t u_perFrame;
};

layout(set = 0, binding = 1) uniform perObject
{
	mat4 u_mCurrWorld;
	mat4 u_mPrevWorld;
	pbrMaterial_t u_pbr;
	pbrTexture_t u_tex;
	pbrTextureMat_t u_tex_mat;
};


#ifdef ID_MATUV_DATA
// UV矩阵的数据
layout(set = 0, binding = ID_MATUV_DATA) buffer readonly per_u_matuv
{
	mat3 u_matuv[];
};
#endif
#endif // __cplusplus

#ifdef ID_TextureA
layout(set = 1, binding = ID_TextureA) uniform sampler2D u_ColorSampler[25];
#endif
#ifdef ID_Cube
layout(set = 1, binding = ID_Cube) uniform samplerCube u_EnvSampler[3]; // 环境贴图3种
#endif
#ifdef ID_shadowMap
layout(set = 1, binding = ID_shadowMap) uniform sampler2DShadow u_shadowMap[MAX_SHADOW_INSTANCES];
#endif


struct psInput_t
{
	vec3 v_Position;
#ifdef HAS_NORMAL_VEC3
#ifdef HAS_TANGENT_VEC4
	mat3 v_TBN;
#else
	vec3 v_Normal;
#endif
#endif
#ifdef HAS_COLOR_0_VEC3
	vec3 v_Color;
#endif
#ifdef HAS_COLOR_0_VEC4
	vec4 v_Color;
#endif
	vec2 v_texcoord_0;
	vec2 v_texcoord_1;
};
#ifdef PSINPUT
layout(location = 0) in psInput_t inInput;
#else
psInput_t inInput;
#endif
//#include <tonemapping.glsl>
#ifdef TONEMAPPING

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
	return vec3(pow(srgbIn.xyz, vec3(GAMMA)));
}


vec4 sRGBToLinear(vec4 srgbIn)
{
	return vec4(sRGBToLinear(srgbIn.xyz), srgbIn.w);
}


// ACES tone map (faster approximation)
// see: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 toneMapACES_Narkowicz(vec3 color)
{
	const float A = 2.51;
	const float B = 0.03;
	const float C = 2.43;
	const float D = 0.59;
	const float E = 0.14;
	return clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0);
}


// ACES filmic tone map approximation
// see https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
vec3 RRTAndODTFit(vec3 color)
{
	vec3 a = color * (color + 0.0245786) - 0.000090537;
	vec3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
	return a / b;
}


// tone mapping
vec3 toneMapACES_Hill(vec3 color)
{
	color = ACESInputMat * color;

	// Apply RRT and ODT
	color = RRTAndODTFit(color);

	color = ACESOutputMat * color;

	// Clamp to [0, 1]
	color = clamp(color, 0.0, 1.0);

	return color;
}

// Khronos PBR neutral tone mapping
#ifdef TONEMAP_KHR_PBR_NEUTRAL
vec3 toneMap_KhronosPbrNeutral(vec3 color)
{
	const float startCompression = 0.8 - 0.04;
	const float desaturation = 0.15;

	float x = min(color.r, min(color.g, color.b));
	float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
	color -= offset;

	float peak = max(color.r, max(color.g, color.b));
	if (peak < startCompression) return color;

	const float d = 1. - startCompression;
	float newPeak = 1. - d * d / (peak + d - startCompression);
	color *= newPeak / peak;

	float g = 1. - 1. / (desaturation * (peak - newPeak) + 1.);
	return mix(color, newPeak * vec3(1, 1, 1), g);
}
#endif

vec3 toneMap(vec3 color)
{
	color *= u_perFrame.u_Exposure;

#ifdef TONEMAP_ACES_NARKOWICZ
	color = toneMapACES_Narkowicz(color);
#endif

#ifdef TONEMAP_ACES_HILL
	color = toneMapACES_Hill(color);
#endif

#ifdef TONEMAP_ACES_HILL_EXPOSURE_BOOST
	// boost exposure as discussed in https://github.com/mrdoob/three.js/pull/19621
	// this factor is based on the exposure correction of Krzysztof Narkowicz in his
	// implemetation of ACES tone mapping
	color /= 0.6;
	color = toneMapACES_Hill(color);
#endif

#ifdef TONEMAP_KHR_PBR_NEUTRAL
	color = toneMap_KhronosPbrNeutral(color);
#endif

	return linearTosRGB(color);
}

#endif // TONEMAPPING

//#include <textures.glsl>
#ifdef TEXTURES
vec2 getNormalUV()
{
	vec3 uv = vec3(u_tex.u_NormalUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);

#ifdef HAS_NORMAL_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_NormalUVTransform] * uv;
#endif

	return uv.xy;
}


vec2 getEmissiveUV()
{
	vec3 uv = vec3(u_tex.u_EmissiveUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);

#ifdef HAS_EMISSIVE_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_EmissiveUVTransform] * uv;
#endif

	return uv.xy;
}


vec2 getOcclusionUV()
{
	vec3 uv = vec3(u_tex.u_OcclusionUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);

#ifdef HAS_OCCLUSION_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_OcclusionUVTransform] * uv;
#endif

	return uv.xy;
}


// Metallic Roughness Material


#ifdef MATERIAL_METALLICROUGHNESS


vec2 getBaseColorUV()
{
	vec3 uv = vec3(u_tex.u_BaseColorUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);

#ifdef HAS_BASECOLOR_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_BaseColorUVTransform] * uv;
#endif

	return uv.xy;
}

vec2 getMetallicRoughnessUV()
{
	vec3 uv = vec3(u_tex.u_MetallicRoughnessUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);

#ifdef HAS_METALLICROUGHNESS_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_MetallicRoughnessUVTransform] * uv;
#endif

	return uv.xy;
}

#endif


// Specular Glossiness Material


#ifdef MATERIAL_SPECULARGLOSSINESS



vec2 getSpecularGlossinessUV()
{
	vec3 uv = vec3(u_tex.u_SpecularGlossinessUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);

#ifdef HAS_SPECULARGLOSSINESS_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_SpecularGlossinessUVTransform] * uv;
#endif

	return uv.xy;
}

vec2 getDiffuseUV()
{
	vec3 uv = vec3(u_tex.u_DiffuseUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);

#ifdef HAS_DIFFUSE_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_DiffuseUVTransform] * uv;
#endif

	return uv.xy;
}

#endif


// Clearcoat Material


#ifdef MATERIAL_CLEARCOAT


vec2 getClearcoatUV()
{
	vec3 uv = vec3(u_tex.u_ClearcoatUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_CLEARCOAT_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_ClearcoatUVTransform] * uv;
#endif
	return uv.xy;
}

vec2 getClearcoatRoughnessUV()
{
	vec3 uv = vec3(u_tex.u_ClearcoatRoughnessUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_CLEARCOATROUGHNESS_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_ClearcoatRoughnessUVTransform] * uv;
#endif
	return uv.xy;
}

vec2 getClearcoatNormalUV()
{
	vec3 uv = vec3(u_tex.u_ClearcoatNormalUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_CLEARCOATNORMAL_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_ClearcoatNormalUVTransform] * uv;
#endif
	return uv.xy;
}

#endif


// Sheen Material


#ifdef MATERIAL_SHEEN


vec2 getSheenColorUV()
{
	vec3 uv = vec3(u_tex.u_SheenColorUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_SHEENCOLOR_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_SheenColorUVTransform] * uv;
#endif
	return uv.xy;
}

vec2 getSheenRoughnessUV()
{
	vec3 uv = vec3(u_tex.u_SheenRoughnessUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_SHEENROUGHNESS_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_SheenRoughnessUVTransform] * uv;
#endif
	return uv.xy;
}

#endif


// Specular Material


#ifdef MATERIAL_SPECULAR


vec2 getSpecularUV()
{
	vec3 uv = vec3(u_tex.u_SpecularUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_SPECULAR_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_SpecularUVTransform] * uv;
#endif
	return uv.xy;
}

vec2 getSpecularColorUV()
{
	vec3 uv = vec3(u_tex.u_SpecularColorUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_SPECULARCOLOR_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_SpecularColorUVTransform] * uv;
#endif
	return uv.xy;
}

#endif


// Transmission Material


#ifdef MATERIAL_TRANSMISSION

vec2 getTransmissionUV()
{
	vec3 uv = vec3(u_tex.u_TransmissionUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_TRANSMISSION_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_TransmissionUVTransform] * uv;
#endif
	return uv.xy;
}

#endif


// Volume Material


#ifdef MATERIAL_VOLUME


vec2 getThicknessUV()
{
	vec3 uv = vec3(u_tex.u_ThicknessUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_THICKNESS_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_ThicknessUVTransform] * uv;
#endif
	return uv.xy;
}

#endif


// Iridescence


#ifdef MATERIAL_IRIDESCENCE


vec2 getIridescenceUV()
{
	vec3 uv = vec3(u_tex.u_IridescenceUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_IRIDESCENCE_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_IridescenceUVTransform] * uv;
#endif
	return uv.xy;
}

vec2 getIridescenceThicknessUV()
{
	vec3 uv = vec3(u_tex.u_IridescenceThicknessUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_IRIDESCENCETHICKNESS_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_IridescenceThicknessUVTransform] * uv;
#endif
	return uv.xy;
}

#endif


// Diffuse Transmission

#ifdef MATERIAL_DIFFUSE_TRANSMISSION


vec2 getDiffuseTransmissionUV()
{
	vec3 uv = vec3(u_tex.u_DiffuseTransmissionUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_DIFFUSETRANSMISSION_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_DiffuseTransmissionUVTransform] * uv;
#endif
	return uv.xy;
}

vec2 getDiffuseTransmissionColorUV()
{
	vec3 uv = vec3(u_tex.u_DiffuseTransmissionColorUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_DIFFUSETRANSMISSIONCOLOR_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_DiffuseTransmissionColorUVTransform] * uv;
#endif
	return uv.xy;
}

#endif

// Anisotropy

#ifdef MATERIAL_ANISOTROPY

vec2 getAnisotropyUV()
{
	vec3 uv = vec3(u_tex.u_AnisotropyUVSet < 1 ? inInput.v_texcoord_0 : inInput.v_texcoord_1, 1.0);
#ifdef HAS_ANISOTROPY_UV_TRANSFORM
	uv = u_matuv[u_tex_mat.u_AnisotropyUVTransform] * uv;
#endif
	return uv.xy;
}

#endif

#endif // TEXTURES

//#include <functions.glsl>
#ifdef FUNCTIONS
const float M_PI = 3.141592653589793;



vec4 getVertexColor()
{
	vec4 color = vec4(1.0);

#ifdef HAS_COLOR_0_VEC3
	color.rgb = inInput.v_Color.rgb;
#endif
#ifdef HAS_COLOR_0_VEC4
	color = inInput.v_Color;
#endif

	return color;
}


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
	return (1.0 - rgb_alpha_max) * base + rgb_alpha * layer;
}


#endif // FUNCTIONS

//#include <brdf.glsl>
#ifdef BRDF
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

	return (f - f90 * x5) / (1.0 - x5);
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
	sheenRoughness = max(sheenRoughness, 0.000001); //clamp (0,1]
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
	return (diffuseColor / M_PI);
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

#endif // BRDF

//#include <punctual.glsl>
#ifdef PUNCTUAL


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


vec3 getLighIntensity(Light_t light, vec3 pointToLight)
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
	vec3 baseColor, float ior)
{
	float transmissionRougness = applyIorToRoughness(alphaRoughness, ior);

	vec3 n = normalize(normal);           // Outward direction of surface point
	vec3 v = normalize(view);             // Direction from surface point to view
	vec3 l = normalize(pointToLight);
	vec3 l_mirror = normalize(l + 2.0 * n * dot(-l, n));     // Mirror light reflection vector on surface
	vec3 h = normalize(l_mirror + v);            // Halfway vector between transmission light vector and v

	float D = D_GGX(clamp(dot(n, h), 0.0, 1.0), transmissionRougness);
	float Vis = V_GGX(clamp(dot(n, l_mirror), 0.0, 1.0), clamp(dot(n, v), 0.0, 1.0), transmissionRougness);

	// Transmission BTDF
	return baseColor * D * Vis;
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
	modelScale.x = length(vec3(modelMatrix[0].xyz));
	modelScale.y = length(vec3(modelMatrix[1].xyz));
	modelScale.z = length(vec3(modelMatrix[2].xyz));

	// The thickness is specified in local space.
	return normalize(refractionVector) * thickness * modelScale;
}

#endif // PUNCTUAL

//#include <ibl.glsl>
#ifdef IBL
vec3 getDiffuseLight(vec3 n)
{
	vec4 textureSample = texture(u_ColorSampler[u_LambertianEnvSampler], u_perFrame.u_EnvRotation * n);
	textureSample.rgb *= u_perFrame.u_EnvIntensity;
	return textureSample.rgb;
}


vec4 getSpecularSample(vec3 reflection, float lod)
{
	vec4 textureSample = textureLod(u_ColorSampler[u_GGXEnvSampler], u_perFrame.u_EnvRotation * reflection, lod);
	textureSample.rgb *= u_perFrame.u_EnvIntensity;
	return textureSample;
}


vec4 getSheenSample(vec3 reflection, float lod)
{
	vec4 textureSample = textureLod(u_ColorSampler[u_CharlieEnvSampler], u_perFrame.u_EnvRotation * reflection, lod);
	textureSample.rgb *= u_perFrame.u_EnvIntensity;
	return textureSample;
}

vec3 getIBLGGXFresnel(vec3 n, vec3 v, float roughness, vec3 F0, float specularWeight)
{
	// see https://bruop.github.io/ibl/#single_scattering_results at Single Scattering Results
	// Roughness dependent fresnel, from Fdez-Aguera
	float NdotV = clampedDot(n, v);
	vec2 brdfSamplePoint = clamp(vec2(NdotV, roughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
	vec2 f_ab = texture(u_ColorSampler[u_GGXLUT], brdfSamplePoint).rg;
	vec3 Fr = max(vec3(1.0 - roughness), F0) - F0;
	vec3 k_S = F0 + Fr * pow(1.0 - NdotV, 5.0);
	vec3 FssEss = specularWeight * (k_S * f_ab.x + f_ab.y);

	// Multiple scattering, from Fdez-Aguera
	float Ems = (1.0 - (f_ab.x + f_ab.y));
	vec3 F_avg = specularWeight * (F0 + (1.0 - F0) / 21.0);
	vec3 FmsEms = Ems * FssEss * F_avg / (1.0 - F_avg * Ems);

	return FssEss + FmsEms;
}

vec3 getIBLRadianceGGX(vec3 n, vec3 v, float roughness)
{
	float NdotV = clampedDot(n, v);
	float lod = roughness * float(u_perFrame.u_MipCount - 1);
	vec3 reflection = normalize(reflect(-v, n));
	vec4 specularSample = getSpecularSample(reflection, lod);

	vec3 specularLight = specularSample.rgb;

	return specularLight;
}


#ifdef MATERIAL_TRANSMISSION
vec3 getTransmissionSample(vec2 fragCoord, float roughness, float ior)
{
	float framebufferLod = log2(float(u_TransmissionFramebufferSize.x)) * applyIorToRoughness(roughness, ior);
	vec3 transmittedLight = textureLod(u_ColorSampler[u_TransmissionFramebufferSampler], fragCoord.xy, framebufferLod).rgb;

	return transmittedLight;
}
#endif


#ifdef MATERIAL_TRANSMISSION
vec3 getIBLVolumeRefraction(vec3 n, vec3 v, float perceptualRoughness, vec3 baseColor, vec3 position, mat4 modelMatrix,
	mat4 viewMatrix, mat4 projMatrix, float ior, float thickness, vec3 attenuationColor, float attenuationDistance, float dispersion)
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
	vec2 refractionCoords = ndcPos.xy / ndcPos.w;
	refractionCoords += 1.0;
	refractionCoords /= 2.0;

	// Sample framebuffer to get pixel the refracted ray hits.
	vec3 transmittedLight = getTransmissionSample(refractionCoords, perceptualRoughness, ior);

#endif // MATERIAL_DISPERSION
	vec3 attenuatedColor = applyVolumeAttenuation(transmittedLight, transmissionRayLength, attenuationColor, attenuationDistance);

	return attenuatedColor * baseColor;
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

	float lod = roughness * float(u_perFrame.u_MipCount - 1);
	vec3 reflection = normalize(reflect(-v, bentNormal));

	vec4 specularSample = getSpecularSample(reflection, lod);

	vec3 specularLight = specularSample.rgb;

	return specularLight;
}
#endif


vec3 getIBLRadianceCharlie(vec3 n, vec3 v, float sheenRoughness, vec3 sheenColor)
{
	float NdotV = clampedDot(n, v);
	float lod = sheenRoughness * float(u_perFrame.u_MipCount - 1);
	vec3 reflection = normalize(reflect(-v, n));

	vec2 brdfSamplePoint = clamp(vec2(NdotV, sheenRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
	float brdf = texture(u_ColorSampler[u_CharlieLUT], brdfSamplePoint).b;
	vec4 sheenSample = getSheenSample(reflection, lod);

	vec3 sheenLight = sheenSample.rgb;
	return sheenLight * sheenColor * brdf;
}

#endif // IBL

//#include <material_info.glsl>
#if 1


struct MaterialInfo_t
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
#ifdef PSINPUT
	vec2 UV = getNormalUV();
#else
	vec2 UV;
#endif
	vec2 uv_dx = dFdx(UV);
	vec2 uv_dy = dFdy(UV);

	if (length(uv_dx) <= 1e-2) {
		uv_dx = vec2(1.0, 0.0);
	}

	if (length(uv_dy) <= 1e-2) {
		uv_dy = vec2(0.0, 1.0);
	}

#ifdef PSINPUT
	vec3 t_ = (uv_dy.t * dFdx(inInput.v_Position) - uv_dx.t * dFdy(inInput.v_Position)) / (uv_dx.s * uv_dy.t - uv_dy.s * uv_dx.t);
#else
	vec3 t_;
#endif
	vec3 n, t, b, ng;

	// Compute geometrical TBN:
#ifdef HAS_NORMAL_VEC3
#ifdef HAS_TANGENT_VEC4
	// Trivial TBN computation, present as vertex attribute.
	// Normalize eigenvectors as matrix is linearly interpolated.
	t = normalize(inInput.v_TBN[0]);
	b = normalize(inInput.v_TBN[1]);
	ng = normalize(inInput.v_TBN[2]);
#else
	// Normals are either present as vertex attributes or approximated.
	ng = normalize(inInput.v_Normal);
	t = normalize(t_ - ng * dot(ng, t_));
	b = cross(ng, t);
#endif
#else
#ifdef PSINPUT
	ng = normalize(cross(dFdx(inInput.v_Position), dFdy(inInput.v_Position)));
#endif
	t = normalize(t_ - ng * dot(ng, t_));
	b = cross(ng, t);
#endif

#ifndef NOT_TRIANGLE
	// For a back-facing surface, the tangential basis vectors are negated.
	if (gl_FrontFacing == false)
	{
		t *= -1.0;
		b *= -1.0;
		ng *= -1.0;
	}
#endif

	// Compute normals:
	NormalInfo info;
	info.ng = ng;
#ifdef HAS_NORMAL_MAP
	info.ntex = texture(u_ColorSampler[u_ColorSampler[u_NormalSampler], UV).rgb * 2.0 - vec3(1.0);
	info.ntex *= vec3(u_tex.u_NormalScale, u_tex.u_NormalScale, 1.0);
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
	vec3 n = texture(u_ColorSampler[u_ColorSampler[u_ClearcoatNormalSampler], getClearcoatNormalUV()).rgb * 2.0 - vec3(1.0);
	n *= vec3(u_ClearcoatNormalScale, u_ClearcoatNormalScale, 1.0);
	n = mat3(normalInfo.t, normalInfo.b, normalInfo.ng) * normalize(n);
	return n;
#else
	return normalInfo.ng;
#endif
}
#endif


vec4 getBaseColor()
{
	vec4 baseColor = u_pbr.u_BaseColorFactor;

#if defined(HAS_BASE_COLOR_MAP)
	baseColor *= texture(u_ColorSampler[u_tex.u_BaseColorSampler], getBaseColorUV());
#endif

	return baseColor * getVertexColor();
}


#ifdef MATERIAL_METALLICROUGHNESS
MaterialInfo_t getMetallicRoughnessInfo(MaterialInfo_t info)
{
	info.metallic = u_MetallicFactor;
	info.perceptualRoughness = u_RoughnessFactor;

#ifdef HAS_METALLIC_ROUGHNESS_MAP
	// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
	// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
	vec4 mrSample = texture(u_ColorSampler[u_MetallicRoughnessSampler], getMetallicRoughnessUV());
	info.perceptualRoughness *= mrSample.g;
	info.metallic *= mrSample.b;
#endif

	return info;
}
#endif


#ifdef MATERIAL_SHEEN
MaterialInfo_t getSheenInfo(MaterialInfo_t info)
{
	info.sheenColorFactor = u_SheenColorFactor;
	info.sheenRoughnessFactor = u_SheenRoughnessFactor;

#ifdef HAS_SHEEN_COLOR_MAP
	vec4 sheenColorSample = texture(u_ColorSampler[u_SheenColorSampler], getSheenColorUV());
	info.sheenColorFactor *= sheenColorSample.rgb;
#endif

#ifdef HAS_SHEEN_ROUGHNESS_MAP
	vec4 sheenRoughnessSample = texture(u_ColorSampler[u_SheenRoughnessSampler], getSheenRoughnessUV());
	info.sheenRoughnessFactor *= sheenRoughnessSample.a;
#endif
	return info;
}
#endif


#ifdef MATERIAL_SPECULAR
MaterialInfo_t getSpecularInfo(MaterialInfo_t info)
{
	vec4 specularTexture = vec4(1.0);
#ifdef HAS_SPECULAR_MAP
	specularTexture.a = texture(u_ColorSampler[u_SpecularSampler], getSpecularUV()).a;
#endif
#ifdef HAS_SPECULAR_COLOR_MAP
	specularTexture.rgb = texture(u_ColorSampler[u_SpecularColorSampler], getSpecularColorUV()).rgb;
#endif

	info.f0_dielectric = min(info.f0_dielectric * u_KHR_materials_specular_specularColorFactor * specularTexture.rgb, vec3(1.0));
	info.specularWeight = u_KHR_materials_specular_specularFactor * specularTexture.a;
	info.f90_dielectric = vec3(info.specularWeight);
	return info;
}
#endif


#ifdef MATERIAL_TRANSMISSION
MaterialInfo_t getTransmissionInfo(MaterialInfo_t info)
{
	info.transmissionFactor = u_TransmissionFactor;

#ifdef HAS_TRANSMISSION_MAP
	vec4 transmissionSample = texture(u_ColorSampler[u_TransmissionSampler], getTransmissionUV());
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
MaterialInfo_t getVolumeInfo(MaterialInfo_t info)
{
	info.thickness = u_ThicknessFactor;
	info.attenuationColor = u_AttenuationColor;
	info.attenuationDistance = u_AttenuationDistance;

#ifdef HAS_THICKNESS_MAP
	vec4 thicknessSample = texture(u_ColorSampler[u_ThicknessSampler], getThicknessUV());
	info.thickness *= thicknessSample.g;
#endif
	return info;
}
#endif
// material_info

#ifdef MATERIAL_IRIDESCENCE
MaterialInfo_t getIridescenceInfo(MaterialInfo_t info)
{
	info.iridescenceFactor = u_IridescenceFactor;
	info.iridescenceIor = u_IridescenceIor;
	info.iridescenceThickness = u_IridescenceThicknessMaximum;

#ifdef HAS_IRIDESCENCE_MAP
	info.iridescenceFactor *= texture(u_ColorSampler[u_IridescenceSampler], getIridescenceUV()).r;
#endif

#ifdef HAS_IRIDESCENCE_THICKNESS_MAP
	float thicknessSampled = texture(u_ColorSampler[u_IridescenceThicknessSampler], getIridescenceThicknessUV()).g;
	float thickness = mix(u_IridescenceThicknessMinimum, u_IridescenceThicknessMaximum, thicknessSampled);
	info.iridescenceThickness = thickness;
#endif

	return info;
}
#endif


#ifdef MATERIAL_DIFFUSE_TRANSMISSION
MaterialInfo_t getDiffuseTransmissionInfo(MaterialInfo_t info)
{
	info.diffuseTransmissionFactor = u_DiffuseTransmissionFactor;
	info.diffuseTransmissionColorFactor = u_DiffuseTransmissionColorFactor;

#ifdef HAS_DIFFUSE_TRANSMISSION_MAP
	info.diffuseTransmissionFactor *= texture(u_ColorSampler[u_DiffuseTransmissionSampler], getDiffuseTransmissionUV()).a;
#endif

#ifdef HAS_DIFFUSE_TRANSMISSION_COLOR_MAP
	info.diffuseTransmissionColorFactor *= texture(u_ColorSampler[u_DiffuseTransmissionColorSampler], getDiffuseTransmissionColorUV()).rgb;
#endif

	return info;
}
#endif


#ifdef MATERIAL_CLEARCOAT
MaterialInfo_t getClearCoatInfo(MaterialInfo_t info, NormalInfo normalInfo)
{
	info.clearcoatFactor = u_ClearcoatFactor;
	info.clearcoatRoughness = u_ClearcoatRoughnessFactor;
	info.clearcoatF0 = vec3(pow((info.ior - 1.0) / (info.ior + 1.0), 2.0));
	info.clearcoatF90 = vec3(1.0);

#ifdef HAS_CLEARCOAT_MAP
	vec4 clearcoatSample = texture(u_ColorSampler[u_ClearcoatSampler], getClearcoatUV());
	info.clearcoatFactor *= clearcoatSample.r;
#endif

#ifdef HAS_CLEARCOAT_ROUGHNESS_MAP
	vec4 clearcoatSampleRoughness = texture(u_ColorSampler[u_ClearcoatRoughnessSampler], getClearcoatRoughnessUV());
	info.clearcoatRoughness *= clearcoatSampleRoughness.g;
#endif

	info.clearcoatNormal = getClearcoatNormal(normalInfo);
	info.clearcoatRoughness = clamp(info.clearcoatRoughness, 0.0, 1.0);
	return info;
}
#endif


#ifdef MATERIAL_IOR
MaterialInfo_t getIorInfo(MaterialInfo_t info)
{
	info.f0_dielectric = vec3(pow((u_Ior - 1.0) / (u_Ior + 1.0), 2.0));
	info.ior = u_Ior;
	return info;
}
#endif

#ifdef MATERIAL_ANISOTROPY
MaterialInfo_t getAnisotropyInfo(MaterialInfo_t info, NormalInfo normalInfo)
{
	vec2 direction = vec2(1.0, 0.0);
	float strengthFactor = 1.0;
#ifdef HAS_ANISOTROPY_MAP
	vec3 anisotropySample = texture(u_ColorSampler[u_AnisotropySampler], getAnisotropyUV()).xyz;
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

#ifdef MATERIAL_SHEEN
float albedoSheenScalingLUT(float NdotV, float sheenRoughnessFactor)
{
	return texture(u_ColorSampler[u_SheenELUT], vec2(NdotV, sheenRoughnessFactor)).r;
}
#endif

#endif // 1

#ifdef MATERIAL_IRIDESCENCE
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



vec4 px_main()
{
	vec4 finalColor = vec4(0.0);
	vec4 baseColor = getBaseColor();
#ifdef ALPHAMODE
#if ALPHAMODE == ALPHAMODE_OPAQUE
	baseColor.a = 1.0;
#endif
#endif // ALPHAMODE
	vec3 color = vec3(0);

#ifdef PSINPUT
	vec3 v = normalize(u_perFrame.u_Camera - inInput.v_Position);
#else
	vec3 v;
#endif
	NormalInfo normalInfo = getNormalInfo(v);
	vec3 n = normalInfo.n;
	vec3 t = normalInfo.t;
	vec3 b = normalInfo.b;

	float NdotV = clampedDot(n, v);
	float TdotV = clampedDot(t, v);
	float BdotV = clampedDot(b, v);

	MaterialInfo_t materialInfo;
#ifdef PSINPUT
	materialInfo.baseColor = baseColor.rgb;
#endif
	// The default index of refraction of 1.5 yields a dielectric normal incidence reflectance of 0.04.
	materialInfo.ior = 1.5;
	materialInfo.f0_dielectric = vec3(0.04);
	materialInfo.specularWeight = 1.0;

	// Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
	materialInfo.f90 = vec3(1.0);
	materialInfo.f90_dielectric = materialInfo.f90;

#ifdef MATERIAL_IOR
	materialInfo = getIorInfo(materialInfo);
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
		(length(vec3(u_perFrame.u_ModelMatrix[0].xyz)) + length(vec3(u_perFrame.u_ModelMatrix[1].xyz)) + length(vec3(u_perFrame.u_ModelMatrix[2].xyz))) / 3.0;
#endif
#endif

#ifdef MATERIAL_CLEARCOAT
	clearcoatFactor = materialInfo.clearcoatFactor;
	clearcoatFresnel = F_Schlick(materialInfo.clearcoatF0, materialInfo.clearcoatF90, clampedDot(materialInfo.clearcoatNormal, v));
#endif

	// Calculate lighting contribution from image based lighting source (IBL)

#if defined(USE_IBL) || defined(MATERIAL_TRANSMISSION)

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
		baseColor.rgb, inInput.v_Position, u_perFrame.u_ModelMatrix, u_perFrame.u_ViewMatrix, u_perFrame.u_ProjectionMatrix,
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
	ao = texture(u_ColorSampler[u_OcclusionSampler], getOcclusionUV()).r;
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
		Light_t light = u_perFrame.u_Lights[i];

		vec3 pointToLight;
		if (light.type != LightType_Directional)
		{
			pointToLight = light.position - inInput.v_Position;
		}
		else
		{
			pointToLight = -light.direction;
		}
		vec3 baseColor3;
#ifndef __cplusplus
		baseColor3 = baseColor.rgb;
#endif
		// BSTF
		vec3 l = normalize(pointToLight);   // Direction from surface point to light
		vec3 h = normalize(l + v);          // Direction of the vector between l and v, called halfway vector
		float NdotL = clampedDot(n, l);
		float NdotV = clampedDot(n, v);
		float NdotH = clampedDot(n, h);
		float LdotH = clampedDot(l, h);
		float VdotH = clampedDot(v, h);

		vec3 dielectric_fresnel = F_Schlick(materialInfo.f0_dielectric * materialInfo.specularWeight, materialInfo.f90_dielectric, abs(VdotH));
		vec3 metal_fresnel = F_Schlick(baseColor3, vec3(1.0), abs(VdotH));

		vec3 lightIntensity = getLighIntensity(light, pointToLight);

		vec3 l_diffuse = lightIntensity * NdotL * BRDF_lambertian(baseColor3);
		vec3 l_specular_dielectric = vec3(0.0);
		vec3 l_specular_metal = vec3(0.0);
		vec3 l_dielectric_brdf = vec3(0.0);
		vec3 l_metal_brdf = vec3(0.0);
		vec3 l_clearcoat_brdf = vec3(0.0);
		vec3 l_sheen = vec3(0.0);
		float l_albedoSheenScaling = 1.0;




#ifdef MATERIAL_DIFFUSE_TRANSMISSION
		l_diffuse = l_diffuse * (1.0 - materialInfo.diffuseTransmissionFactor);
		if (dot(n, l) < 0.0) {
			float diffuseNdotL = clampedDot(-n, l);
			vec3 diffuse_btdf = lightIntensity * diffuseNdotL * BRDF_lambertian(materialInfo.diffuseTransmissionColorFactor);

			vec3 l_mirror = normalize(l + 2.0 * n * dot(-l, n)); // Mirror light reflection vector on surface
			float diffuseVdotH = clampedDot(v, normalize(l_mirror + v));
			dielectric_fresnel = F_Schlick(materialInfo.f0_dielectric * materialInfo.specularWeight, materialInfo.f90_dielectric, abs(diffuseVdotH));

#ifdef MATERIAL_VOLUME
			diffuse_btdf = applyVolumeAttenuation(diffuse_btdf, diffuseTransmissionThickness, materialInfo.attenuationColor, materialInfo.attenuationDistance);
#endif
			l_diffuse += diffuse_btdf * materialInfo.diffuseTransmissionFactor;
		}

#endif // MATERIAL_DIFFUSE_TRANSMISSION

		// BTDF (Bidirectional Transmittance Distribution Function)
#ifdef MATERIAL_TRANSMISSION
		// If the light ray travels through the geometry, use the point it exits the geometry again.
		// That will change the angle to the light source, if the material refracts the light ray.
		vec3 transmissionRay = getVolumeTransmissionRay(n, v, materialInfo.thickness, materialInfo.ior, u_perFrame.u_ModelMatrix);
		pointToLight -= transmissionRay;
		l = normalize(pointToLight);

		vec3 transmittedLight = lightIntensity * getPunctualRadianceTransmission(n, v, l, materialInfo.alphaRoughness, baseColor.rgb, materialInfo.ior);

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

	f_emissive = u_tex.u_EmissiveFactor;
#ifdef MATERIAL_EMISSIVE_STRENGTH
	f_emissive *= u_pbr.u_EmissiveStrength;
#endif
#ifdef HAS_EMISSIVE_MAP
	f_emissive *= texture(u_ColorSampler[u_EmissiveSampler], getEmissiveUV()).rgb;
#endif


#ifdef MATERIAL_UNLIT
	color = baseColor.rgb;
#elif defined(NOT_TRIANGLE) && !defined(HAS_NORMAL_VEC3)
	//Points or Lines with no NORMAL attribute SHOULD be rendered without lighting and instead use the sum of the base color value and the emissive value.
	color = f_emissive + baseColor.rgb;
#else

#ifdef PSINPUT
	color = f_emissive * (1.0 - clearcoatFactor * clearcoatFresnel) + color;
#endif
#endif

#if DEBUG == DEBUG_NONE

#if ALPHAMODE == ALPHAMODE_MASK
	// Late discard to avoid sampling artifacts. See https://github.com/KhronosGroup/glTF-Sample-Viewer/issues/267
	if (baseColor.a < u_AlphaCutoff)
	{
		discard;
	}
	baseColor.a = 1.0;
#endif

#ifdef LINEAR_OUTPUT
	finalColor = vec4(color.rgb, baseColor.a);
#else
	finalColor = vec4(toneMap(color), baseColor.a);
#endif

#else
	// In case of missing data for a debug view, render a checkerboard.
	finalColor = vec4(1.0);
	{
		float frequency = 0.02;
		float gray = 0.9;

#ifdef PSINPUT
		vec2 v1 = step(0.5, fract(frequency * gl_FragCoord.xy));
		vec2 v2 = step(0.5, vec2(1.0) - fract(frequency * gl_FragCoord.xy));
		finalColor.rgb *= gray + v1.x * v1.y + v2.x * v2.y;
#endif
	}
#endif

	// Debug views:

	// Generic:
#if DEBUG == DEBUG_UV_0 && defined(HAS_TEXCOORD_0_VEC2)
	finalColor.rgb = vec3(inInput.v_texcoord_0, 0);
#endif
#if DEBUG == DEBUG_UV_1 && defined(HAS_TEXCOORD_1_VEC2)
	finalColor.rgb = vec3(inInput.v_texcoord_1, 0);
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


#if DEBUG == DEBUG_METALLIC
	finalColor.rgb = vec3(materialInfo.metallic);
#endif
#if DEBUG == DEBUG_ROUGHNESS
	finalColor.rgb = vec3(materialInfo.perceptualRoughness);
#endif
#if DEBUG == DEBUG_BASE_COLOR
	finalColor.rgb = linearTosRGB(materialInfo.baseColor);
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
	specularTexture.rgb = texture(u_ColorSampler[u_SpecularColorSampler], getSpecularColorUV()).rgb;
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
	direction = texture(u_ColorSampler[u_AnisotropySampler], getAnisotropyUV()).xy;
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


#endif
//pbrpixel_h_