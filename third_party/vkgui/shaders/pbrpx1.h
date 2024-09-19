#ifndef pbrpx_h_
#define pbrpx_h_

// KHR_lights_punctual extension.
// see https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual
#ifndef MAX_LIGHT_INSTANCES
#define MAX_LIGHT_INSTANCES  80
#endif
#define MAX_SHADOW_INSTANCES 32

struct Light
{
	mat4          mLightViewProj;
	mat4          mLightView;

	vec3          direction;
	float         range;            // 方向光的角度大小，聚光灯和点光源为 1/范围

	vec3          color;
	float         intensity;        // 光照度
	vec3          position;
	// spot
	float         innerConeCos;
	float         outerConeCos;
	int           type;
	float         depthBias;
	int           shadowMapIndex;
};

const int LightType_Directional = 0;
const int LightType_Point = 1;
const int LightType_Spot = 2;

struct PerFrame
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
	float  pad1;
	float  pad2;
	int           u_lightCount;
	Light         u_lights[MAX_LIGHT_INSTANCES];
};

struct pbrMaterial
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

	// Iridescence
	float iridescenceFactor;
	float iridescenceIor;
	float iridescenceThicknessMinimum;
	float iridescenceThicknessMaximum;

	float clearcoatFactor;
	float clearcoatRoughness;
	float clearcoatNormalScale;
	float envIntensity;

	int unlit;
	float pad[3];
	mat3 uvTransform;
};

struct MaterialInfo
{
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	vec3 reflectance0;            // full reflectance color (normal incidence angle)

	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	vec3 diffuseColor;            // color contribution from diffuse lighting

	vec3 reflectance90;           // reflectance color at grazing angle
	vec3 specularColor;           // color contribution from specular lighting
};
struct gpuMaterial
{
	vec3 reflectance0;            // full reflectance color (normal incidence angle)
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)

	vec3 diffuseColor;            // color contribution from diffuse lighting
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])

	vec3 reflectance90;           // reflectance color at grazing angle
	float alpha;
	vec3 specularColorlight;           // color contribution from specular lighting
	float dispersion;

	vec4  baseColor;  // base color 
	vec2  roughness;  // 0 = smooth, 1 = rough (anisotropic: x = U, y = V)

	vec3 f90;                       // reflectance color at grazing angle
	vec3 f90_dielectric;

	float metallic;   // 0 = dielectric, 1 = metallic
	vec3  emissive;   // emissive color

	vec3 N;   // shading normal
	vec3 T;   // shading normal
	vec3 B;   // shading normal
	vec3 Ng;  // geometric normal

	vec2 uv;

	float ior1;  // index of refraction : current medium (i.e. air)
	float ior2;  // index of refraction : the other side (i.e. glass)

	float ior;
	vec3 f0_dielectric;

	float specularWeight; // product of specularFactor and specularTexture.a

	float specular;       // weight of the dielectric specular layer
	vec3  specularColor;  // color of the dielectric specular layer
	float transmission;   // KHR_materials_transmission

	vec3  attenuationColor;     // KHR_materials_volume
	float attenuationDistance;  //
	float thickness;            // Replace for isThinWalled?

	// KHR_materials_clearcoat
	vec3 clearcoatF0;
	vec3 clearcoatF90;
	float clearcoatFactor;
	vec3 clearcoatNormal;
	float clearcoatRoughness;

	float iridescence;
	float iridescenceIor;
	float iridescenceThickness;

	vec3  sheenColor;
	float sheenRoughness;
	float ao;

};

// alphaMode
#define ALPHA_OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2

struct MeshState
{
	vec3 N;   // Normal
	vec3 T;   // Tangent
	vec3 B;   // Bitangent
	vec3 Ng;  // Geometric normal
	vec2 tc;  // Texture coordinates
	float emissiveFactor;
	bool isInside;
};
#define MICROFACET_MIN_ROUGHNESS 0.0014142f

struct NormalInfo {
	vec3 ng;   // Geometry normal
	vec3 t;    // Geometry tangent
	vec3 b;    // Geometry bitangent
	vec3 n;    // Shading normal
	vec3 ntex; // Normal from texture, scaling is accounted for.
};

#ifdef __cplusplus

PerFrame myPerFrame;
pbrMaterial u_pbrParams;
#else

layout(scalar, set = 0, binding = 0) uniform perFrame
{
	PerFrame myPerFrame;
};

layout(scalar, set = 0, binding = 1) uniform perObject
{
	mat4 myPerObject_u_mCurrWorld;
	mat4 myPerObject_u_mPrevWorld;

	pbrMaterial u_pbrParams;
};

#endif // __cplusplus



// 纹理
#if 1

#ifdef ID_TEXCOORD_1
#define TEXCOORD(id) vec2(id < 1 ? Input.UV0 : Input.UV1);
#else
#define TEXCOORD(id) vec2(Input.UV0);
#endif

//disable texcoords that are not in the VS2PS structure
#if defined(ID_TEXCOORD_0)==false
#if ID_normalTexCoord == 0
#undef ID_normalTexture
#undef ID_normalTexCoord
#endif
#if ID_emissiveTexCoord == 0
#undef ID_emissiveTexture
#undef ID_emissiveTexCoord
#endif
#if ID_baseTexCoord == 0
#undef ID_baseColorTexture
#undef ID_baseTexCoord
#endif
#if ID_metallicRoughnessTexCoord == 0
#undef ID_metallicRoughnessTexture
#undef ID_metallicRoughnessTexCoord
#endif
#endif

#ifdef ID_baseColorTexture
layout(set = 1, binding = ID_baseColorTexture) uniform sampler2D u_BaseColorSampler;
#endif

#ifdef ID_normalTexture
layout(set = 1, binding = ID_normalTexture) uniform sampler2D u_NormalSampler;
#endif

#ifdef ID_emissiveTexture
layout(set = 1, binding = ID_emissiveTexture) uniform sampler2D u_EmissiveSampler;
#endif

#ifdef ID_metallicRoughnessTexture
layout(set = 1, binding = ID_metallicRoughnessTexture) uniform sampler2D u_MetallicRoughnessSampler;
#endif

#ifdef ID_occlusionTexture
layout(set = 1, binding = ID_occlusionTexture) uniform sampler2D u_OcclusionSampler;
float u_OcclusionStrength = 1.0;
#endif

#ifdef ID_diffuseTexture
layout(set = 1, binding = ID_diffuseTexture) uniform sampler2D u_diffuseSampler;
#endif

#ifdef ID_specularGlossinessTexture
layout(set = 1, binding = ID_specularGlossinessTexture) uniform sampler2D u_specularGlossinessSampler;
#endif

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
#ifdef ID_CharlieTexture
layout(set = 1, binding = ID_CharlieTexture) uniform sampler2D u_CharlieLUT;
#endif

#ifdef ID_specularTexture
layout(set = 1, binding = ID_specularTexture) uniform sampler2D u_specularTexture;
#endif
#ifdef ID_specularColorTexture
layout(set = 1, binding = ID_specularColorTexture) uniform sampler2D u_specularColorTexture;
#endif
#ifdef ID_transmissionTexture
layout(set = 1, binding = ID_transmissionTexture) uniform sampler2D u_transmissionTexture;
#endif  
#ifdef ID_thicknessTexture
layout(set = 1, binding = ID_thicknessTexture) uniform sampler2D u_thicknessTexture;
#endif

#ifdef ID_clearcoatRoughnessTexture
layout(set = 1, binding = ID_clearcoatRoughnessTexture) uniform sampler2D u_clearcoatRoughnessTexture;
#endif
#ifdef ID_clearcoatNormalTexture
layout(set = 1, binding = ID_clearcoatNormalTexture) uniform sampler2D u_clearcoatNormalTexture;
#endif
#ifdef ID_iridescenceTexture
layout(set = 1, binding = ID_iridescenceTexture) uniform sampler2D u_iridescenceTexture;
#endif
#ifdef ID_iridescenceThicknessTexture
layout(set = 1, binding = ID_iridescenceThicknessTexture) uniform sampler2D u_iridescenceThicknessTexture;
#endif
#ifdef ID_anisotropyTexture
layout(set = 1, binding = ID_anisotropyTexture) uniform sampler2D u_anisotropyTexture;
#endif
#ifdef ID_sheenColorTexture
layout(set = 1, binding = ID_sheenColorTexture) uniform sampler2D u_sheenColorTexture;
#endif
#ifdef ID_CharlieEnvSampler
layout(set = 1, binding = ID_CharlieEnvSampler) uniform samplerCube u_CharlieEnvSampler;
#endif
#ifdef ID_sheenRoughnessTexture
layout(set = 1, binding = ID_sheenRoughnessTexture) uniform sampler2D u_sheenRoughnessTexture;
#endif

#ifdef ID_shadowMap
layout(set = 1, binding = ID_shadowMap) uniform sampler2DShadow u_shadowMap[MAX_SHADOW_INSTANCES];
#endif
#ifdef ID_SSAO
layout(set = 1, binding = ID_SSAO) uniform sampler2D ssaoSampler;
#endif

#ifdef ID_transmissionFramebufferTexture
layout(set = 1, binding = ID_transmissionFramebufferTexture) uniform sampler2D u_TransmissionFramebufferSampler;
#endif
//------------------------------------------------------------
// UV getters
//------------------------------------------------------------
vec2 getNormalUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_normalTexCoord
	uv.xy = TEXCOORD(ID_normalTexCoord);
#ifdef HAS_NORMAL_UV_TRANSFORM
	uv *= u_pbrParams.uvTransform;
#endif
#endif
	return uv.xy;
#endif
}

#if 0

vec2 getEmissiveUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_emissiveTexCoord
	uv.xy = TEXCOORD(ID_emissiveTexCoord);
#ifdef HAS_EMISSIVE_UV_TRANSFORM
	uv *= u_EmissiveUVTransform;
#endif
#endif
	return uv.xy;
#endif
}

vec2 getOcclusionUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_occlusionTexCoord
	uv.xy = TEXCOORD(ID_occlusionTexCoord);
#ifdef HAS_OCCLSION_UV_TRANSFORM
	uv *= u_OcclusionUVTransform;
#endif
#endif
	return uv.xy;
#endif
}

vec2 getBaseColorUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_baseTexCoord
	uv.xy = TEXCOORD(ID_baseTexCoord);
#ifdef HAS_BASECOLOR_UV_TRANSFORM
	uv *= u_BaseColorUVTransform;
#endif
#endif
	return uv.xy;
#endif
}

vec2 getMetallicRoughnessUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_metallicRoughnessTexCoord
	uv.xy = TEXCOORD(ID_metallicRoughnessTexCoord);
#ifdef HAS_METALLICROUGHNESS_UV_TRANSFORM
	uv *= u_MetallicRoughnessUVTransform;
#endif
#endif
	return uv.xy;
#endif 
}

vec2 getSpecularGlossinessUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_specularGlossinessTexture
	uv.xy = TEXCOORD(ID_specularGlossinessTexCoord);
#ifdef HAS_SPECULARGLOSSINESS_UV_TRANSFORM
	uv *= u_SpecularGlossinessUVTransform;
#endif
#endif
	return uv.xy;
#endif
}

vec2 getDiffuseUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_diffuseTexture
	uv.xy = TEXCOORD(ID_diffuseTexCoord);
#ifdef HAS_DIFFUSE_UV_TRANSFORM
	uv *= u_DiffuseUVTransform;
#endif
#endif
	return uv.xy;
#endif
}
#endif
vec4 getBaseColorTexture(VS2PS Input, vec2 uv)
{
#ifdef ID_baseColorTexture
	return texture(u_BaseColorSampler, uv, myPerFrame.u_LodBias);
#else
	return vec4(1, 1, 1, 1); //OPAQUE
#endif
}

vec4 getDiffuseTexture(VS2PS Input, vec2 uv)
{
#ifdef ID_diffuseTexture
	return texture(u_diffuseSampler, uv, myPerFrame.u_LodBias);
#else
	return vec4(1, 1, 1, 1);
#endif 
}

vec4 getMetallicRoughnessTexture(VS2PS Input, vec2 uv)
{
#ifdef ID_metallicRoughnessTexture
	return texture(u_MetallicRoughnessSampler, uv, myPerFrame.u_LodBias);
#else 
	return vec4(1, 1, 1, 1);
#endif   
}

vec4 getSpecularGlossinessTexture(VS2PS Input, vec2 uv)
{
#ifdef ID_specularGlossinessTexture    
	return texture(u_specularGlossinessSampler, uv, myPerFrame.u_LodBias);
#else 
	return vec4(1, 1, 1, 1);
#endif   
}


#endif // 1

// 通用函数
#if 1

const float c_MinReflectance = 0.04;
#ifndef M_PI
const float M_PI = 3.141592653589793F;  // PI
#endif
const float M_TWO_PI = 6.2831853071795F;  // 2*PI
const float M_PI_2 = 1.5707963267948F;  // PI/2
const float M_PI_4 = 0.7853981633974F;  // PI/4
const float M_1_OVER_PI = 0.3183098861837F;  // 1/PI
const float M_2_OVER_PI = 0.6366197723675F;  // 2/PI
const float M_1_PI = 0.3183098861837F;  // 1/PI

#ifndef INFINITE
const float INFINITE = 1e32F;
#endif

struct AngularInfo
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector

	float VdotH;                  // cos angle between view direction and half vector

	vec3 padding;
};

vec4 getVertexColor()
{
	vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

#ifdef ID_4PS_COLOR_0
	color.rgb = v_Color0.rgb;
#endif

	return color;
}



float getPerceivedBrightness(vec3 vector)
{
	return sqrt(0.299 * vector.r * vector.r + 0.587 * vector.g * vector.g + 0.114 * vector.b * vector.b);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness/examples/convert-between-workflows/js/three.pbrUtilities.js#L34
float solveMetallic(vec3 diffuse, vec3 specular, float oneMinusSpecularStrength) {
	float specularBrightness = getPerceivedBrightness(specular);

	if (specularBrightness < c_MinReflectance) {
		return 0.0;
	}

	float diffuseBrightness = getPerceivedBrightness(diffuse);

	float a = c_MinReflectance;
	float b = diffuseBrightness * oneMinusSpecularStrength / (1.0 - c_MinReflectance) + specularBrightness - 2.0 * c_MinReflectance;
	float c = c_MinReflectance - specularBrightness;
	float D = b * b - 4.0 * a * c;

	return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
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


#endif // 1

// 阴影
#if 1 
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

	if (light.type != LightType_Spot && light.type != LightType_Directional)
		return 1.0; // no other light types cast shadows for now

	vec4 shadowTexCoord = light.mLightViewProj * vec4(vPosition, 1.0);
	shadowTexCoord.xyz = shadowTexCoord.xyz / shadowTexCoord.w;

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

	// offsets of the center of the shadow map atlas
	/*float offsetsX[4] = { 0.0, 1.0, 0.0, 1.0 };
	float offsetsY[4] = { 0.0, 0.0, 1.0, 1.0 };
	shadowTexCoord.x += offsetsX[light.shadowMapIndex] * .5;
	shadowTexCoord.y += offsetsY[light.shadowMapIndex] * .5;*/

	shadowTexCoord.z -= light.depthBias;

	return FilterShadow(light.shadowMapIndex, shadowTexCoord.xyz);
#else
	return 1.0f;
#endif
}

#endif // 1
// 像素计算
#if 1

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
vec3 getPixelNormal(VS2PS Input, vec2 UV)
{
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
	vec3 n = tbn[2];
	n = normalize(n);
#endif

	return n;
}
vec3 getPixelNormal(VS2PS Input)
{
	vec2 UV = getNormalUV(Input);
	return getPixelNormal(Input, UV);
}
void get_tangent(VS2PS Input, vec2 uv, out vec3 tangent, out vec3 binormal)
{
#ifndef ID_TANGENT
	vec3 pos_dx = dFdx(Input.WorldPos);
	vec3 pos_dy = dFdy(Input.WorldPos);
	vec3 tex_dx = dFdx(vec3(uv, 0.0));
	vec3 tex_dy = dFdy(vec3(uv, 0.0));
	vec3 t = (tex_dy.y * pos_dx - tex_dx.y * pos_dy) / (tex_dx.x * tex_dy.y - tex_dy.x * tex_dx.y);

#ifdef ID_NORMAL
	vec3 ng = normalize(Input.Normal);
#else
	vec3 ng = cross(pos_dx, pos_dy);
#endif

	t = normalize(t - ng * dot(ng, t));
	vec3 b = normalize(cross(ng, t));
	//mat3 tbn = mat3(t, b, ng);
	tangent = t; binormal = b;
#else  
	tangent = Input.Tangent;
	binormal = Input.Binormal;
#endif
}

gpuMaterial defaultPbrMaterial()
{
	gpuMaterial mat;
	mat.baseColor = vec4(1.0F);
	mat.roughness = vec2(1.0F);
	mat.metallic = 1.0F;
	mat.emissive = vec3(0.0F);

	mat.N = vec3(0.0F, 0.0F, 1.0F);
	mat.Ng = vec3(0.0F, 0.0F, 1.0F);
	mat.T = vec3(1.0F, 0.0F, 0.0F);
	mat.B = vec3(0.0F, 1.0F, 0.0F);

	mat.ior1 = 1.0F;
	mat.ior2 = 1.5F;

	mat.specular = 1.0F;
	mat.specularColor = vec3(1.0F);
	mat.transmission = 0.0F;

	mat.attenuationColor = vec3(1.0F);
	mat.attenuationDistance = 1.0F;
	mat.thickness = 0.0F;

	mat.clearcoatFactor = 0.0F;
	mat.clearcoatRoughness = 0.0F;
	mat.clearcoatNormal = vec3(0.0F, 0.0F, 1.0F);

	mat.iridescence = 0.0F;
	mat.iridescenceIor = 1.5F;
	mat.iridescenceThickness = 0.1F;

	mat.sheenColor = vec3(0.0F);
	mat.sheenRoughness = 0.0F;
	mat.ao = 0;
	return mat;
}
void orthonormalBasis(vec3 normal, out vec3 tangent, out vec3 bitangent)
{
	if (normal.z < -0.99998796F)  // Handle the singularity
	{
		tangent = vec3(0.0F, -1.0F, 0.0F);
		bitangent = vec3(-1.0F, 0.0F, 0.0F);
		return;
	}
	float a = 1.0F / (1.0F + normal.z);
	float b = -normal.x * normal.y * a;
	tangent = vec3(1.0F - normal.x * normal.x * a, b, -normal.x);
	bitangent = vec3(b, 1.0f - normal.y * normal.y * a, -normal.y);
}
gpuMaterial defaultPbrMaterial(vec3 baseColor, float metallic, float roughness, vec3 N, vec3 Ng)
{
	gpuMaterial mat = defaultPbrMaterial();
	mat.baseColor = vec4(baseColor, 1.0f);
	mat.metallic = metallic;
	mat.roughness = vec2(roughness * roughness);
	mat.Ng = Ng;
	mat.N = N;
	orthonormalBasis(mat.N, mat.T, mat.B);

	return mat;
}




vec4 getBaseColor(VS2PS Input, vec2 uv)
{
	vec4 baseColor = vec4(0.0, 0.0, 0.0, 1.0);
#ifdef MATERIAL_SPECULARGLOSSINESS
	baseColor = getDiffuseTexture(Input, uv);
#endif

#ifdef MATERIAL_METALLICROUGHNESS
	// The albedo may be defined from a base texture or a flat color
	baseColor = getBaseColorTexture(Input, uv);
#endif
	return baseColor;
}

vec4 getBaseColor2(VS2PS Input, pbrMaterial params, vec2 uv)
{
	vec4 baseColor = getBaseColor(Input, uv);

#ifdef MATERIAL_SPECULARGLOSSINESS
	baseColor *= params.pbrDiffuseFactor;
#endif

#ifdef MATERIAL_METALLICROUGHNESS
	baseColor *= params.baseColorFactor;
#endif

	baseColor *= getPixelColor(Input);
	return baseColor;
}

void discardPixelIfAlphaCutOff(VS2PS Input)
{
#ifdef ID_TEXCOORD_0
	vec2 uv = Input.UV0;
#else
	vec2 uv = vec2(0.0, 0.0);
#endif
	vec4 baseColor = getBaseColor(Input, uv);

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

vec4 get_roughness(VS2PS Input, pbrMaterial params, vec2 uv, out vec3 diffuseColor, out vec3  specularColor, out float perceptualRoughness, out float metallic)
{
	perceptualRoughness = 0.0;
	diffuseColor = vec3(0.0, 0.0, 0.0);
	specularColor = vec3(0.0, 0.0, 0.0);
	metallic = 0.0;
	vec3 f0 = vec3(0.04, 0.04, 0.04);
	vec4 baseColor = getBaseColor2(Input, params, uv);
	// Metallic and Roughness material properties are packed together
	// In glTF, these factors can be specified by fixed scalar values
	// or from a metallic-roughness map
#ifdef MATERIAL_SPECULARGLOSSINESS
	vec4 sgSample = getSpecularGlossinessTexture(Input, uv);
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
	// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
	// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
	vec4 mrSample = getMetallicRoughnessTexture(Input, uv);
	perceptualRoughness = mrSample.g * params.roughnessFactor;
	metallic = mrSample.b * params.metallicFactor;
	diffuseColor = vec3(baseColor) * (vec3(1.0, 1.0, 1.0) - f0) * (1.0 - metallic);
	specularColor = mix(f0, vec3(baseColor), metallic);
#endif // ! MATERIAL_METALLICROUGHNESS

	perceptualRoughness = clamp(perceptualRoughness, 0.0f, 1.0f);
	metallic = clamp(metallic, 0.0f, 1.0f);
	return baseColor;
}

vec4 get_roughness(VS2PS Input, pbrMaterial params, vec2 uv, out vec3 diffuseColor, out vec3  specularColor, out float perceptualRoughness)
{
	float metallic;
	return get_roughness(Input, params, uv, diffuseColor, specularColor, perceptualRoughness, metallic);
}
//-----------------------------------------------------------------------

void getPBRParams(VS2PS Input, pbrMaterial material, inout gpuMaterial m)
{
	MeshState mesh;
	vec2 uv = m.uv;
	mesh.N = getPixelNormal(Input, uv);
	mesh.Ng = mesh.N;
	get_tangent(Input, uv, mesh.T, mesh.B);
	mesh.tc = uv;
	mesh.isInside = false;
	mesh.emissiveFactor = myPerFrame.u_EmissiveFactor;
	vec3 uv3 = vec3(mesh.tc, 1);
	// KHR_texture_transform 
#ifdef MATERIAL_TEXTURE_TRANSFORM
	uv3 *= material.uvTransform;
#endif
	vec2 texCoord = vec2(uv3);
	uv = texCoord;
	// Metallic and Roughness material properties are packed together
	// In glTF, these factors can be specified by fixed scalar values
	// or from a metallic-roughness map

	vec3 f0 = vec3(0.04, 0.04, 0.04);

	m.baseColor = get_roughness(Input, material, uv, m.diffuseColor, m.specularColorlight, m.perceptualRoughness, m.metallic);
	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness.
	m.alphaRoughness = m.perceptualRoughness * m.perceptualRoughness;
	m.alpha = m.baseColor.a;

#if 1
	// Metallic-Roughness
	float roughness = material.roughnessFactor;
	roughness = max(m.perceptualRoughness, MICROFACET_MIN_ROUGHNESS);
	m.roughness = vec2(roughness * roughness);  // Square roughness for the microfacet model 

	// Normal Map
	vec3 normal = mesh.N;
	vec3 tangent = mesh.T;
	vec3 bitangent = mesh.B;
#ifdef ID_normalTexture
	//if (material.normalTexture > -1)
	{
		mat3 tbn = mat3(mesh.T, mesh.B, mesh.N);
		vec3 normal_vector = getPixelNormal(Input, texCoord);
		normal_vector = normal_vector * 2.0F - 1.0F;
		normal_vector *= vec3(material.normalScale, material.normalScale, 1.0F);
		normal = normalize(tbn * normal_vector);
	}
#endif // ID_normalTexture
	// We should always have B = cross(N, T) * bitangentSign. Orthonormalize,
	// in case the normal was changed or the input wasn't orthonormal:
	m.N = normal;
	m.B = cross(m.N, tangent);
	float bitangentSign = sign(dot(bitangent, m.B));
	m.B = normalize(m.B) * bitangentSign;
	m.T = normalize(cross(m.B, m.N)) * bitangentSign;
	m.Ng = mesh.Ng;

	// Emissive term
	vec3 emissive = material.emissiveFactor * mesh.emissiveFactor;
#ifdef ID_emissiveTexture
	emissive *= texture(u_EmissiveSampler, texCoord).rgb;
#endif 
	m.emissive = max(vec3(0.0F), emissive);

	// KHR_materials_specular
	// https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_specular
	m.specularColor = material.specularColorFactor;
#ifdef ID_specularColorTexture  
	m.specularColor *= texture(u_specularColorTexture, texCoord).rgb;
#endif 
	m.specular = material.specularFactor;
#ifdef ID_specularTexture  
	m.specular *= texture(u_specularTexture, texCoord).a;
#endif

	// Dielectric Specular
	float ior1 = 1.0F;                                   // IOR of the current medium (e.g., air)
	float ior2 = material.ior;                           // IOR of the material
	if (mesh.isInside && (material.thicknessFactor > 0))  // If the material is thin-walled, we don't need to consider the inside IOR.
	{
		ior1 = material.ior;
		ior2 = 1.0F;
	}
	m.ior1 = ior1;
	m.ior2 = ior2;


	// KHR_materials_transmission
	m.transmission = material.transmissionFactor;
#ifdef ID_transmissionTexture  
	m.transmission *= texture(u_transmissionTexture, texCoord).r;
#endif
	// KHR_materials_volume
	m.attenuationColor = material.attenuationColor;
	m.attenuationDistance = material.attenuationDistance;
	m.thickness = material.thicknessFactor;
#ifdef ID_thicknessTexture  
	m.thickness *= texture(u_thicknessTexture, texCoord).r;
#endif
#if MATERIAL_CLEARCOAT
	// KHR_materials_clearcoat
	m.clearcoatFactor = material.clearcoatFactor;
	m.clearcoatRoughness = material.clearcoatRoughness;
	m.clearcoatF0 = vec3(pow((m.ior - 1.0) / (m.ior + 1.0), 2.0));
	m.clearcoatF90 = vec3(1.0);
	m.clearcoatNormal = m.N;
#endif

#ifdef ID_clearcoatTexture
	m.clearcoatFactor *= texture(u_clearcoatTexture, texCoord).r;
#endif

#ifdef ID_clearcoatRoughnessTexture
	m.clearcoatRoughness *= texture(u_clearcoatRoughnessTexture, texCoord).g;
#endif
	//m.clearcoatRoughness = max(m.clearcoatRoughness, 0.001F);
	m.clearcoatRoughness = clamp(m.clearcoatRoughness, 0.0f, 1.0f);

#ifdef ID_clearcoatNormalTexture 
	mat3 tbn = mat3(m.T, m.B, m.clearcoatNormal);
	vec3 normal_vector = texture(u_clearcoatNormalTexture, texCoord).xyz;
	normal_vector = normal_vector * 2.0F - 1.0F;
	m.clearcoatNormal = normalize(tbn * normal_vector);
#endif

	// Iridescence
	float iridescence = material.iridescenceFactor;
#ifdef ID_iridescenceTexture 
	iridescence *= texture(u_iridescenceTexture, texCoord).x;
#endif
	float iridescenceThickness = material.iridescenceThicknessMaximum;
#ifdef ID_iridescenceThicknessTexture
	const float t = texture(u_iridescenceThicknessTexture, texCoord).y;
	iridescenceThickness = mix(material.iridescenceThicknessMinimum, material.iridescenceThicknessMaximum, t);
#endif
	m.iridescence = (iridescenceThickness > 0.0f) ? iridescence : 0.0f;  // No iridescence when the thickness is zero.
	m.iridescenceIor = material.iridescenceIor;
	m.iridescenceThickness = iridescenceThickness;

#if 0
	// KHR_materials_anisotropy
	float anisotropyStrength = material.anisotropyStrength;
	vec2  anisotropyDirection = vec2(1.0f, 0.0f);  // By default the anisotropy strength is along the tangent.
#ifdef ID_anisotropyTexture 
	const vec4 anisotropyTex = texture(u_anisotropyTexture, texCoord);
	// .xy encodes the direction in (tangent, bitangent) space. Remap from [0, 1] to [-1, 1].
	anisotropyDirection = normalize(vec2(anisotropyTex) * 2.0f - 1.0f);
	// .z encodes the strength in range [0, 1].
	anisotropyStrength *= anisotropyTex.z;
#endif

	// If the anisotropyStrength == 0.0f (default), the roughness is isotropic.
	// No need to rotate the anisotropyDirection or tangent space.
	if (anisotropyStrength > 0.0F)
	{
		m.roughness.x = mix(m.roughness.y, 1.0f, anisotropyStrength * anisotropyStrength);

		const float s = sin(material.anisotropyRotation);  // FIXME PERF Precalculate sin, cos on host.
		const float c = cos(material.anisotropyRotation);

		anisotropyDirection =
			vec2(c * anisotropyDirection.x + s * anisotropyDirection.y, c * anisotropyDirection.y - s * anisotropyDirection.x);

		const vec3 T_aniso = m.T * anisotropyDirection.x + m.B * anisotropyDirection.y;

		m.B = normalize(cross(m.N, T_aniso));
		m.T = cross(m.B, m.N);
	}
#endif

	// KHR_materials_sheen
	vec3 sheenColor = material.sheenColorFactor;
#ifdef ID_sheenColorTexture 
	sheenColor *= vec3(texture(u_sheenColorTexture, texCoord));  // sRGB
#endif
	m.sheenColor = sheenColor;  // No sheen if this is black.

	float sheenRoughness = material.sheenRoughnessFactor;
#ifdef ID_sheenRoughnessTexture 
	sheenRoughness *= texture(u_sheenRoughnessTexture, texCoord).w;
#endif
	sheenRoughness = max(MICROFACET_MIN_ROUGHNESS, sheenRoughness);
	m.sheenRoughness = sheenRoughness;

#ifdef ID_occlusionTexture
	m.ao = texture(u_OcclusionSampler, texCoord).r;
#else
	m.ao = 1;
#endif
#endif 

}

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

#endif // 1
// 灯光
#if 1
#ifdef USE_IBL
vec3 getIBLContribution(gpuMaterial materialInfo, vec3 n, vec3 v)
{
	float NdotV = clamp(dot(n, v), 0.0f, 1.0f);

	float u_MipCount = 9.0; // resolution of 512x512 of the IBL
	float lod = clamp(materialInfo.perceptualRoughness * float(u_MipCount), 0.0f, float(u_MipCount));
	vec3 reflection = normalize(reflect(-v, n));

	vec2 brdfSamplePoint = clamp(vec2(NdotV, materialInfo.perceptualRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
	// retrieve a scale and bias to F0. See [1], Figure 3
	vec2 brdf = texture(u_brdfLUT, brdfSamplePoint).rg;

	vec3 diffuseLight = texture(u_DiffuseEnvSampler, n).rgb;

#ifdef USE_TEX_LOD
	vec3 specularLight = textureLod(u_SpecularEnvSampler, reflection, lod).rgb;
#else
	vec3 specularLight = texture(u_SpecularEnvSampler, reflection).rgb;
#endif

	vec3 diffuse = diffuseLight * materialInfo.diffuseColor;
	vec3 specular = specularLight * (materialInfo.specularColorlight * brdf.x + brdf.y);

	return diffuse + specular;
}
#endif

// Lambert lighting
// see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
vec3 diffuse(gpuMaterial materialInfo)
{
	return materialInfo.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(gpuMaterial materialInfo, AngularInfo angularInfo)
{
	return materialInfo.reflectance0 + (materialInfo.reflectance90 - materialInfo.reflectance0) * pow(clamp(1.0 - angularInfo.VdotH, 0.0, 1.0), 5.0);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float visibilityOcclusion(gpuMaterial materialInfo, AngularInfo angularInfo)
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

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(gpuMaterial materialInfo, AngularInfo angularInfo)
{
	float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;
	float f = (angularInfo.NdotH * alphaRoughnessSq - angularInfo.NdotH) * angularInfo.NdotH + 1.0;
	return alphaRoughnessSq / (M_PI * f * f + 0.000001f);
}

vec3 getPointShade(vec3 pointToLight, gpuMaterial materialInfo, vec3 normal, vec3 view)
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

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
float getRangeAttenuation(float range, float distance)
{
	if (range < 0.0)
	{
		// negative range means unlimited
		return 1.0;
	}
	return max(mix(1, 0, distance / range), 0);//max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
float getSpotAttenuation(vec3 pointToLight, vec3 spotDirection, float outerConeCos, float innerConeCos)
{
	float actualCos = dot(normalize(spotDirection), normalize(-pointToLight));
	if (actualCos > outerConeCos)
	{
		if (actualCos < innerConeCos)
		{
			return smoothstep(outerConeCos, innerConeCos, actualCos);
		}
		return 1.0;
	}
	return 0.0;
}

// 计算穿过体积的衰减光 Compute attenuated light as it travels through a volume.
vec3 applyVolumeAttenuation(vec3 radiance, float transmissionDistance, vec3 attenuationColor, float attenuationDistance)
{
	if (attenuationDistance == 0.0)
	{
		// Attenuation distance is +鈭?(which we indicate by zero), i.e. the transmitted color is not attenuated at all.
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
	vec3 refractionVector = refract(-v, normalize(n), 1.0f / ior);

	// Compute rotation-independant scaling of the model matrix.
	vec3 modelScale;
	modelScale.x = length(vec3(modelMatrix[0]));
	modelScale.y = length(vec3(modelMatrix[1]));
	modelScale.z = length(vec3(modelMatrix[2]));

	// The thickness is specified in local space.
	return normalize(refractionVector) * thickness * modelScale;
}


struct LightContrib
{
	vec3  incidentVector;
	float halfAngularSize;
	vec3  intensity;
	float distance;
	vec3 pointToLight;
};
LightContrib get_lc()
{
	LightContrib contrib;
	contrib.incidentVector = vec3(0.0F);
	contrib.halfAngularSize = 0.0F;
	contrib.intensity = vec3(0.0F);
	contrib.distance = INFINITE;
	return contrib;
}
LightContrib applyDirectionalLight(Light light, gpuMaterial materialInfo, vec3 normal, vec3 view)
{
	vec3 pointToLight = light.direction;
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);

	LightContrib contrib = get_lc();
	contrib.intensity = light.intensity * light.color * shade;
	contrib.incidentVector = light.direction;
	contrib.pointToLight = pointToLight;
	return contrib;
}

LightContrib applyPointLight(Light light, gpuMaterial materialInfo, vec3 normal, vec3 worldPos, vec3 view)
{
	vec3 pointToLight = light.position - worldPos;
	float distance = length(pointToLight);
	float attenuation = getRangeAttenuation(light.range, distance);
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	LightContrib contrib = get_lc();
	contrib.intensity = attenuation * light.intensity * light.color * shade;
	float r_distance = 1.0F / distance;
	contrib.incidentVector = pointToLight * r_distance;
	contrib.pointToLight = pointToLight;
	return contrib;
}

LightContrib applySpotLight(Light light, gpuMaterial materialInfo, vec3 normal, vec3 worldPos, vec3 view)
{
	vec3 pointToLight = light.position - worldPos;
	float distance = length(pointToLight);
	float rangeAttenuation = getRangeAttenuation(light.range, distance);
	float spotAttenuation = getSpotAttenuation(pointToLight, -light.direction, light.outerConeCos, light.innerConeCos);
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	vec3 color = rangeAttenuation * spotAttenuation * light.intensity * light.color * shade;

	LightContrib contrib = get_lc();
	contrib.intensity = color;
	float r_distance = 1.0F / distance;
	contrib.incidentVector = pointToLight * r_distance;
	contrib.pointToLight = pointToLight;
	return contrib;
}

#if 1 
//vec3 getDiffuseLight(vec3 n)
//{
//#ifndef __cplusplus  
//	vec3 dir = rotate(n, vec3(0, 1, 0), -myPerFrame.envRotation);
//	return texture(u_DiffuseEnvSampler, dir).rgb * myPerFrame.u_iblFactor;
//#else
//	return {};
//#endif
//}

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

#if __cplusplus
#define textureQueryLevels(s) (0)
#define u_SpecularEnvSampler
#ifndef texture
#define texture()
#endif
#endif // __cplusplus



#endif

float applyIorToRoughness(float roughness, float ior)
{
	// Scale roughness with IOR so that an IOR of 1.0 results in no microfacet refraction and
	// an IOR of 1.5 results in the default amount of microfacet refraction.
	return roughness * clamp(ior * 2.0 - 2.0, 0.0, 1.0);
}
float clampedDot(vec3 x, vec3 y)
{
	return clamp(dot(x, y), 0.0, 1.0);
}
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



vec3 doPbrLighting(VS2PS Input, PerFrame perFrame, gpuMaterial m)
{
	vec3 outColor = vec3(m.baseColor);
#ifdef __cplusplus
	pbrMaterial u_pbrParams = {};
	auto myPerFrame = perFrame;
#endif
#ifdef MATERIAL_UNLIT
	return outColor;
#endif

	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness [2].
	float alphaRoughness = m.perceptualRoughness * m.perceptualRoughness;
	vec3 specularColor = m.specularColorlight;
	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	vec3 specularEnvironmentR0 = specularColor;
	// Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
	vec3 specularEnvironmentR90 = vec3(clamp(reflectance * 50.0, 0.0, 1.0));

	m.reflectance0 = specularEnvironmentR0;
	m.alphaRoughness = alphaRoughness;
	m.reflectance90 = specularEnvironmentR90;
	//materialInfo.u_ModelMatrix = myPerFrame.u_mCameraCurrViewProjInverse;

	// LIGHTING

	vec3 color = vec3(0.0, 0.0, 0.0);
	vec3 normal = getPixelNormal(Input);
	vec3 worldPos = Input.WorldPos;
	vec3 f90 = vec3(1.0f);
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

	vec3 clearcoat_brdf = vec3(0.0);
	float clearcoatFactor = 0.0;
	vec3 clearcoatFresnel = vec3(0);

#ifdef MATERIAL_CLEARCOAT
	clearcoat_brdf = getIBLRadianceGGX(m.clearcoatNormal, view, m.clearcoatRoughness);
	clearcoatFactor = m.clearcoatFactor;
	clearcoatFresnel = F_Schlick(m.clearcoatF0, m.clearcoatF90, clampedDot(m.clearcoatNormal, view));

#endif 

#ifdef USE_PUNCTUAL
	vec3 c = vec3(0.0, 0.0, 0.0);
	for (int i = 0; i < myPerFrame.u_lightCount; ++i)
	{
		Light light = myPerFrame.u_lights[i];

		float shadowFactor = DoSpotShadow(worldPos, light);
		LightContrib lc;
		if (light.type == LightType_Directional)
		{
			lc = applyDirectionalLight(light, m, normal, view); lc.intensity *= shadowFactor;
		}
		else if (light.type == LightType_Point)
		{
			lc = applyPointLight(light, m, normal, worldPos, view);
		}
		else if (light.type == LightType_Spot)
		{
			lc = applySpotLight(light, m, normal, worldPos, view); lc.intensity *= shadowFactor;
		}
		color += lc.intensity;
	}
#endif

	// Calculate lighting contribution from image based lighting source (IBL)
#ifdef USE_IBL
	color += getIBLContribution(m, normal, view) * myPerFrame.u_iblFactor * GetSSAO(gl_FragCoord.xy * myPerFrame.u_invScreenResolution);
#endif

	//color = mix(color, clearcoat_brdf, clearcoatFactor * clearcoatFresnel);
	//color = vec3(clearcoat_brdf);
	// Apply optional PBR terms for additional (optional) shading
#ifdef ID_occlusionTexture 
	color = color * m.ao; //mix(color, color * ao, myPerFrame.u_OcclusionStrength);
#endif
	color += m.emissive;// *(vec3(1.0) - m.clearcoatFactor * clearcoatFresnel);

	color = clamp(color, vec3(0.0), vec3(1.0));


#ifndef DEBUG_OUTPUT // no debug 
	// regular shading
	outColor = color;
#else // debug output

#ifdef DEBUG_METALLIC
	outColor.rgb = vec3(metallic);
#endif

#ifdef DEBUG_ROUGHNESS
	outColor.rgb = vec3(perceptualRoughness);
#endif

#ifdef DEBUG_NORMAL
#ifdef ID_normalTexCoord
	outColor.rgb = texture(u_NormalSampler, getNormalUV()).rgb;
#else
	outColor.rgb = vec3(0.5, 0.5, 1.0);
#endif
#endif

#ifdef DEBUG_BASECOLOR
	outColor.rgb = (baseColor.rgb);
#endif

#ifdef DEBUG_OCCLUSION
	outColor.rgb = vec3(ao);
#endif

#ifdef DEBUG_EMISSIVE
	outColor.rgb = (emissive);
#endif

#ifdef DEBUG_F0
	outColor.rgb = vec3(f0);
#endif

#ifdef DEBUG_ALPHA
	outColor.rgb = vec3(baseColor.a);
#endif

#endif // !DEBUG_OUTPUT 
	return outColor;
}

#ifdef USE_IBL
vec3 getIBLContribution(MaterialInfo materialInfo, vec3 n, vec3 v)
{
	float NdotV = clamp(dot(n, v), 0.0, 1.0);

	float u_MipCount = 9.0; // resolution of 512x512 of the IBL
	float lod = clamp(materialInfo.perceptualRoughness * float(u_MipCount), 0.0, float(u_MipCount));
	vec3 reflection = normalize(reflect(-v, n));

	vec2 brdfSamplePoint = clamp(vec2(NdotV, materialInfo.perceptualRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
	// retrieve a scale and bias to F0. See [1], Figure 3
	vec2 brdf = texture(u_brdfLUT, brdfSamplePoint).rg;

	vec3 diffuseLight = texture(u_DiffuseEnvSampler, n).rgb;

#ifdef USE_TEX_LOD
	vec3 specularLight = textureLod(u_SpecularEnvSampler, reflection, lod).rgb;
#else
	vec3 specularLight = texture(u_SpecularEnvSampler, reflection).rgb;
#endif

	vec3 diffuse = diffuseLight * materialInfo.diffuseColor;
	vec3 specular = specularLight * (materialInfo.specularColor * brdf.x + brdf.y);

	return diffuse + specular;
}
#endif

// Lambert lighting
// see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
vec3 diffuse(MaterialInfo materialInfo)
{
	return materialInfo.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(MaterialInfo materialInfo, AngularInfo angularInfo)
{
	return materialInfo.reflectance0 + (materialInfo.reflectance90 - materialInfo.reflectance0) * pow(clamp(1.0 - angularInfo.VdotH, 0.0, 1.0), 5.0);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float visibilityOcclusion(MaterialInfo materialInfo, AngularInfo angularInfo)
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
float microfacetDistribution(MaterialInfo materialInfo, AngularInfo angularInfo)
{
	float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;
	float f = (angularInfo.NdotH * alphaRoughnessSq - angularInfo.NdotH) * angularInfo.NdotH + 1.0;
	return alphaRoughnessSq / (M_PI * f * f + 0.000001f);
}

vec3 getPointShade(vec3 pointToLight, MaterialInfo materialInfo, vec3 normal, vec3 view)
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
vec3 applyDirectionalLight(Light light, MaterialInfo materialInfo, vec3 normal, vec3 view)
{
	vec3 pointToLight = light.direction;
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	return light.intensity * light.color * shade;
}

vec3 applyPointLight(Light light, MaterialInfo materialInfo, vec3 normal, vec3 worldPos, vec3 view)
{
	vec3 pointToLight = light.position - worldPos;
	float distance = length(pointToLight);
	float attenuation = getRangeAttenuation(light.range, distance);
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	return attenuation * light.intensity * light.color * shade;
}

vec3 applySpotLight(Light light, MaterialInfo materialInfo, vec3 normal, vec3 worldPos, vec3 view)
{
	vec3 pointToLight = light.position - worldPos;
	float distance = length(pointToLight);
	float rangeAttenuation = getRangeAttenuation(light.range, distance);
	float spotAttenuation = getSpotAttenuation(pointToLight, -light.direction, light.outerConeCos, light.innerConeCos);
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	return rangeAttenuation * spotAttenuation * light.intensity * light.color * shade;
}

vec3 doPbrLighting_old(VS2PS Input, PerFrame perFrame, vec2 uv, vec3 diffuseColor, vec3 specularColor, float perceptualRoughness, vec4 baseColor)
{
#ifdef __cplusplus
	vec3 outColor = baseColor;
#else
	vec3 outColor = baseColor.rgb;
#endif
#ifdef MATERIAL_UNLIT 
	return outColor;
#endif

	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness [2].
	float alphaRoughness = perceptualRoughness * perceptualRoughness;

	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	vec3 specularEnvironmentR0 = specularColor;
	// Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
	vec3 specularEnvironmentR90 = vec3(clamp(reflectance * 50.0, 0.0, 1.0));

	MaterialInfo materialInfo;
	materialInfo.perceptualRoughness = perceptualRoughness;
	materialInfo.reflectance0 = specularEnvironmentR0;
	materialInfo.alphaRoughness = alphaRoughness;
	materialInfo.diffuseColor = diffuseColor;
	materialInfo.reflectance90 = specularEnvironmentR90;
	materialInfo.specularColor = specularColor;

	// LIGHTING

	vec3 color = vec3(0.0, 0.0, 0.0);
	vec3 normal = getPixelNormal(Input);
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
		vec3 c = vec3(0);
		float shadowFactor = DoSpotShadow(worldPos, light);
		if (light.type == LightType_Directional)
		{
			c = applyDirectionalLight(light, materialInfo, normal, view);
		}
		else if (light.type == LightType_Point)
		{
			c = applyPointLight(light, materialInfo, normal, worldPos, view);
		}
		else if (light.type == LightType_Spot)
		{
			c = applySpotLight(light, materialInfo, normal, worldPos, view);
		}
		color += c * shadowFactor;
	}
#endif

	// Calculate lighting contribution from image based lighting source (IBL)
#ifdef USE_IBL
	color += getIBLContribution(materialInfo, normal, view) * myPerFrame.u_iblFactor * GetSSAO(gl_FragCoord.xy * perFrame.u_invScreenResolution);
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
	emissive = u_pbrParams.emissiveFactor * perFrame.u_EmissiveFactor;
#endif
	color += emissive;

#ifndef DEBUG_OUTPUT // no debug

	// regular shading
	outColor = color;

#else // debug output

#ifdef DEBUG_METALLIC
	outColor.rgb = vec3(metallic);
#endif

#ifdef DEBUG_ROUGHNESS
	outColor.rgb = vec3(perceptualRoughness);
#endif

#ifdef DEBUG_NORMAL
#ifdef ID_normalTexCoord
	outColor.rgb = texture(u_NormalSampler, uv).rgb;
#else
	outColor.rgb = vec3(0.5, 0.5, 1.0);
#endif
#endif

#ifdef DEBUG_BASECOLOR
	outColor.rgb = (baseColor.rgb);
#endif

#ifdef DEBUG_OCCLUSION
	outColor.rgb = vec3(ao);
#endif

#ifdef DEBUG_EMISSIVE
	outColor.rgb = (emissive);
#endif

#ifdef DEBUG_F0
	outColor.rgb = vec3(f0);
#endif

#ifdef DEBUG_ALPHA
	outColor.rgb = vec3(baseColor.a);
#endif

#endif // !DEBUG_OUTPUT

	return outColor;
}
#endif // 1


#endif