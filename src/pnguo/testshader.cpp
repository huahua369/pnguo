
/*
pbr材质：金属度metalness、粗糙度roughness、清漆.clearcoat、透光率(透射度).transmission
KHR_materials_ior ：折射率描述了光在穿过对象时是如何散射的。 通过使艺术家能够控制 IOR 值，
						可以使各种透明材料看起来更逼真，包括空气、水、眼睛、玻璃、蓝宝石和钻石。
KHR_materials_volume ：体积扩展使网格表面能够充当体积之间的界面，并实现更逼真的折射和吸收特性，如透明材料中所见。
							这种延伸使半透明材料具有深度和重量的外观。 对于无法进行光线追踪的实时引擎，此扩展还提供了一个厚度纹理贴图，以实现光与大量材料相互作用的快速近似。
KHR_materials_specular ：镜面属性是一个对象的类似镜子的属性：它有规律地反射光线的能力，创建其他对象的相干反射。
						与其前身 KHR_materials_pbrSpecularGlossiness 不同，这个新的镜面反射扩展在 glTF 的 PBR 材料模型核心的现代金属/粗糙度工作流程中运行，使彩色镜面高光与高级 PBR 材料扩展阵列兼容。
*/

#include "pch1.h"
using namespace glm;
//using glm::vec2;
//using glm::vec3;
//using glm::vec4;
//using glm::mat3x4;
//using glm::mat3;
//using glm::mat4;

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif // !M_PI

#define out
#define in
#define inout
#define discard
#define uniform
#define sampler2D void*
#define samplerCube void*
#define sampler2DShadow void*


#define MAX_LIGHT_INSTANCES  4 
//#define ID_baseTexCoord  0

glm::vec3 px(const glm::vec2& pss) {
	return glm::vec3(pss, 0.0f);// 读坐标像素
}
template <typename T>
inline T dFdx(const T& x)
{
	return px({ x.x + 1,x.y }) - px(x);
}

template <typename T>
inline T dFdy(const T& x)
{
	return px({ x.x ,x.y + 1 }) - px(x);
}

template <typename T>
inline T fwidth(const T& x)
{
	return dFdx(x) + dFdy(x);
}
float clamp(float x, double c0, double c1)
{
	return glm::clamp((double)x, c0, c1);
}
vec2 clamp(const vec2& x, double c0, double c1)
{
	return glm::clamp((glm::dvec2)x, c0, c1);
}
vec3 clamp(const vec3& x, double c0, double c1)
{
	return glm::clamp((glm::dvec3)x, c0, c1);
}
vec4 clamp(const vec4& x, double c0, double c1)
{
	return glm::clamp((glm::dvec4)x, c0, c1);
}

float max(float x, double y)
{
	return glm::max(x, (float)y);
}
vec3 refract(vec3 l, vec3 n, double t) {
	return glm::refract(l, n, (float)t);
}
vec4 texture(void*, const vec2& uv) {
	return vec4(uv, 0.0f, 0.0f);
}
vec2 textureSize(void*, int lod) {
	return vec2(0.0f, 0.0f);
}
vec4 textureLod(void*, const vec2& uv, float lod) {
	return vec4(uv, lod, 0.0);
}


#include "shaders/GLTF_VS2PS_IO.h"
#if 0
#include "shaders/functions.h"
#include "shaders/perFrameStruct.h"
#include "shaders/shadowFiltering.h"
#include "shaders/PBRTextures.h"
#include "shaders/PixelParams.h"
#include "shaders/GLTFPBRLighting.h"
#include "shaders/skinning.h"

vec2 testshader() {
	vec2 k = {};












	return k;
}
#endif
#if 0
#define DEF_alphaCutoff 0.500000
#define DEF_alphaMode_OPAQUE 1
#define DEF_doubleSided 0
#define HAS_FORWARD_RT 0
#define HAS_MOTION_VECTORS 1
#define HAS_MOTION_VECTORS_RT 1
#define ID_NORMAL 0
#define ID_PER_FRAME 0
#define ID_PER_OBJECT 1
#define ID_POSITION 1
#define ID_TEXCOORD_0 2
#define ID_brdfTexture 1
#define ID_diffuseCube 2
#define ID_shadowMap 4
#define ID_specularCube 3
#define ID_thicknessTexCoord 0
#define ID_thicknessTexture 0
#define MATERIAL_METALLICROUGHNESS 1
#define MATERIAL_TRANSMISSION 1
#define MATERIAL_VOLUME 1
#define USE_IBL 1

#endif 
struct FragCoord {
	glm::vec2 xy;
};
FragCoord gl_FragCoord;
bool gl_FrontFacing = 1;
//--------------------------------------------------------------------------------------
//  PS Inputs
//--------------------------------------------------------------------------------------

//#include "shaders/1199AD40D8600E6F.h"


namespace pbr
{

	//#include "shaders/pbrpx.h"

}

mat4 GetWorldMatrix()
{
	return mat4(1.0);
}
mat4 GetCameraViewProj()
{
	return mat4(1.0);
}

mat4 GetCameraView()
{
	return mat4(1.0);
}

vec3 a_Position;
vec4 gl_Position;
VS2PS Output;
#if 1
#define USE_dIBL
#define S_VERT
#define USE_PUNCTUAL0
#include "shaders/pbr_px.h"
#include "shaders/GLTFVertexFactory.h"

namespace pbr {
//#include "shaders/pbr_pixel.h"
}
//--------------------------------------------------------------------------------------
// PerFrame structure, must match the one in GltfPbrPass.h
//--------------------------------------------------------------------------------------


	//PBRFactors u_pbrParams;
//mat4 myPerObject_u_mCurrWorld;
VS2PS Input;
//--------------------------------------------------------------------------------------
// mainPS
//--------------------------------------------------------------------------------------


void amain()
{
	//discardPixelIfAlphaCutOff(Input);
	gpuMaterial m = defaultPbrMaterial();
#ifdef ID_TEXCOORD_0
	vec2 uv = Input.UV0;
#else
	vec2 uv = vec2(0.0, 0.0);
#endif
	getPBRParams(Input, u_pbrParams, m);

	if (u_pbrParams.alphaMode == ALPHA_MASK)
	{
		if (m.alpha < u_pbrParams.alphaCutoff)
			discard;
	}
	vec3 c3 = doPbrLighting(Input, myPerFrame, m);
	vec4 color = vec4(c3, m.baseColor.w);

	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness [2].

#ifdef HAS_MOTION_VECTORS_RT
	//Output_motionVect = Input.CurrPosition.xy / Input.CurrPosition.w - Input.PrevPosition.xy / Input.PrevPosition.w;
#endif

#ifdef HAS_SPECULAR_ROUGHNESS_RT
	Output_specularRoughness = vec4(m.specularColor, m.alphaRoughness);
#endif

#ifdef HAS_DIFFUSE_RT
	Output_diffuseColor = vec4(m.diffuseColor, 0);
#endif

#ifdef HAS_NORMALS_RT
	Output_normal = vec4((getPixelNormal(Input) + 1) / 2, 0);
#endif

#ifdef HAS_FORWARD_RT
	vec4 Output_finalColor = vec4(doPbrLighting(Input, myPerFrame, m), m.alpha);
	vec4 wfo = myPerFrame.u_WireframeOptions;
	wfo.w = 1;
	Output_finalColor = mix(Output_finalColor, wfo, myPerFrame.u_WireframeOptions.w);
#endif
}
#endif


void testp() {

	glm::vec3 worldPos = {};
	glm::vec3 n = {};
	glm::vec3 v = {};
	glm::vec3 baseColor = {};
	vec3 cxf = {};	// clearcoatFactor * clearcoatFresnel

	gpuMaterial m = defaultPbrMaterial();

	// out
	vec3 color;

#ifdef USE_PUNCTUAL
	vec3 c = vec3(0.0, 0.0, 0.0);
	for (int i = 0; i < myPerFrame.u_lightCount; ++i)
	{
		Light light = myPerFrame.u_lights[i];

		float shadowFactor = DoSpotShadow(worldPos, light);
		vec3 pointToLight;
		if (light.type != LightType_Directional)
		{
			pointToLight = light.position - worldPos;
		}
		else
		{
			pointToLight = light.direction;
		}

		// BSTF
		vec3 l = normalize(pointToLight);   // Direction from surface point to light
		vec3 h = normalize(l + v);          // Direction of the vector between l and v, called halfway vector
		float NdotL = clampedDot(n, l);
		float NdotV = clampedDot(n, v);
		float NdotH = clampedDot(n, h);
		float LdotH = clampedDot(l, h);
		float VdotH = clampedDot(v, h);

		vec3 dielectric_fresnel = F_Schlick(m.f0_dielectric * m.specularWeight, m.f90_dielectric, abs(VdotH));
		vec3 metal_fresnel = F_Schlick(baseColor, vec3(1.0), abs(VdotH));

		vec3 lightIntensity = getLighIntensity(light, pointToLight, m, n, v, 1.0) * shadowFactor;

		vec3 l_diffuse = lightIntensity * NdotL * BRDF_lambertian(baseColor);
		vec3 l_specular_dielectric = vec3(0.0);
		vec3 l_specular_metal = vec3(0.0);
		vec3 l_dielectric_brdf = vec3(0.0);
		vec3 l_metal_brdf = vec3(0.0);
		vec3 l_clearcoat_brdf = vec3(0.0);
		vec3 l_sheen = vec3(0.0);
		float l_albedoSheenScaling = 1.0;

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
		vec3 diffuse_btdf = lightIntensity * clampedDot(-n, l) * BRDF_lambertian(m.diffuseTransmissionColorFactor);
#ifdef MATERIAL_VOLUME
		diffuse_btdf = applyVolumeAttenuation(diffuse_btdf, diffuseTransmissionThickness, m.attenuationColor, m.attenuationDistance);
#endif
		l_diffuse = mix(l_diffuse, diffuse_btdf, m.diffuseTransmissionFactor);
#endif	// MATERIAL_DIFFUSE_TRANSMISSION
		// BTDF (Bidirectional Transmittance Distribution Function)
#ifdef MATERIAL_TRANSMISSION
		// If the light ray travels through the geometry, use the point it exits the geometry again.
		// That will change the angle to the light source, if the material refracts the light ray.
		vec3 transmissionRay = getVolumeTransmissionRay(n, v, m.thickness, m.ior, u_ModelMatrix);
		pointToLight -= transmissionRay;
		l = normalize(pointToLight);
		vec3 transmittedLight = lightIntensity * getPunctualRadianceTransmission(n, v, l, m.alphaRoughness, m.f0_dielectric, m.f90, baseColor.rgb, m.ior);
#ifdef MATERIAL_VOLUME
		transmittedLight = applyVolumeAttenuation(transmittedLight, length(transmissionRay), m.attenuationColor, m.attenuationDistance);
#endif
		l_diffuse = mix(l_diffuse, transmittedLight, m.transmissionFactor);
#endif
		// Calculation of analytical light
		// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
		vec3 intensity = getLighIntensity(light, pointToLight, m, n, v, 1.0) * shadowFactor;
#ifdef MATERIAL_ANISOTROPY
		l_specular_metal = intensity * NdotL * BRDF_specularGGXAnisotropy(m.alphaRoughness, m.anisotropyStrength, n, v, l, h, m.anisotropicT, m.anisotropicB);
		l_specular_dielectric = l_specular_metal;
#else
		l_specular_metal = intensity * NdotL * BRDF_specularGGX(m.alphaRoughness, NdotL, NdotV, NdotH);
		l_specular_dielectric = l_specular_metal;
#endif
		l_metal_brdf = metal_fresnel * l_specular_metal;
		l_dielectric_brdf = mix(l_diffuse, l_specular_dielectric, dielectric_fresnel); // Do we need to handle vec3 fresnel here?
#ifdef MATERIAL_IRIDESCENCE
		l_metal_brdf = mix(l_metal_brdf, l_specular_metal * iridescenceFresnel_metallic, m.iridescenceFactor);
		l_dielectric_brdf = mix(l_dielectric_brdf, rgb_mix(l_diffuse, l_specular_dielectric, iridescenceFresnel_dielectric), m.iridescenceFactor);
#endif
#ifdef MATERIAL_CLEARCOAT
		l_clearcoat_brdf = intensity * getPunctualRadianceClearCoat(m.clearcoatNormal, v, l, h, VdotH, m.clearcoatF0, m.clearcoatF90, m.clearcoatRoughness);
#endif
#ifdef MATERIAL_SHEEN
		l_sheen = intensity * getPunctualRadianceSheen(m.sheenColorFactor, m.sheenRoughnessFactor, NdotL, NdotV, NdotH);
		l_albedoSheenScaling = min(1.0 - max3(m.sheenColorFactor) * albedoSheenScalingLUT(NdotV, m.sheenRoughnessFactor), 1.0 - max3(m.sheenColorFactor) * albedoSheenScalingLUT(NdotL, m.sheenRoughnessFactor));
#endif
		vec3 l_color = mix(l_dielectric_brdf, l_metal_brdf, m.metallic);
		l_color = l_sheen + l_color * l_albedoSheenScaling;
		l_color = mix(l_color, l_clearcoat_brdf, cxf);
		l_color = max(l_color, vec3(0.0));
		color += l_color + intensity;

	}
#endif

	color += m.emissive * (vec3(1.0) - cxf);
	color = max(color, vec3(0.0));


}
#if 0
#version 450
#extension GL_EXT_scalar_block_layout : enable // 确保结构体内存布局与C++一致 

// 1. 关键帧数据结构（与C++结构体对齐）
struct Keyframe {
	float time;         // 关键帧时间戳
	vec3 translation;   // 平移向量（局部空间）
	vec4 rotation;      // 旋转四元数（wxyz，单位化） 
	vec3 scale;         // 缩放因子 
};

// 2. 骨骼数据结构（包含多个关键帧） 
struct Bone {
	uint keyframeCount; // 关键帧数量 
	Keyframe keyframes[]; // 柔性数组（实际存储时按骨骼分配）
};

// 3. 输入资源（Descriptor Set绑定）
layout(set = 0, binding = 0) buffer Bones {
	Bone bones[]; // 所有骨骼的关键帧数据（Storage Buffer）
};

// 4. 输出资源（插值后的骨骼变换矩阵）
layout(set = 0, binding = 1) buffer InterpolatedTransforms {
	mat4 transforms[]; // 每个骨骼的当前变换矩阵（供顶点着色器使用）
};

// 5. 当前时间（Uniform Buffer，或用Push Constant优化）
layout(set = 0, binding = 2) uniform Time {
	float currentTime; // 当前动画时间（如0.0~10.0秒） 
};

// 6. 工作组配置（每个工作组处理64个骨骼，可根据GPU调整）
layout(local_size_x = 64) in;

// 7. 插值工具函数 
// 线性插值（平移/缩放）
vec3 lerp(vec3 a, vec3 b, float t) {
	return mix(a, b, t); // GLSL内置mix函数等价于线性插值 
}

// 球面线性插值（Slerp，旋转四元数专用）
vec4 slerp(vec4 q1, vec4 q2, float t) {
	float dotProduct = dot(q1, q2);
	// 确保插值走最短路径（dotProduct非负）
	if (dotProduct < 0.0) {
		q2 = -q2;
		dotProduct = -dotProduct;
	}
	// 避免除以零（当两四元数非常接近时，用线性插值代替）
	if (dotProduct > 0.9995) {
		return normalize(q1 + t * (q2 - q1));
	}
	// 计算Slerp
	float theta = acos(dotProduct);
	float sinTheta = sin(theta);
	float t1 = sin((1.0 - t) * theta) / sinTheta;
	float t2 = sin(t * theta) / sinTheta;
	return q1 * t1 + q2 * t2;
}

// 8. 组合变换矩阵（缩放→旋转→平移）
mat4 composeTransform(vec3 trans, vec4 rot, vec3 scale) {
	// 旋转矩阵（四元数转矩阵）
	mat4 rotMat = mat4(1.0);
	float w = rot.w, x = rot.x, y = rot.y, z = rot.z;
	rotMat[0]() = vec4(1.0 - 2 * y * y - 2 * z * z, 2 * x * y + 2 * z * w, 2 * x * z - 2 * y * w, 0.0);
	rotMat[1]() = vec4(2 * x * y - 2 * z * w, 1.0 - 2 * x * x - 2 * z * z, 2 * y * z + 2 * x * w, 0.0);
	rotMat[2]() = vec4(2 * x * z + 2 * y * w, 2 * y * z - 2 * x * w, 1.0 - 2 * x * x - 2 * y * y, 0.0);
	// 缩放矩阵 
	mat4 scaleMat = mat4(scale.x, 0.0, 0.0, 0.0,
		0.0, scale.y, 0.0, 0.0,
		0.0, 0.0, scale.z, 0.0,
		0.0, 0.0, 0.0, 1.0);
	// 平移矩阵 
	mat4 transMat = mat4(1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		trans.x, trans.y, trans.z, 1.0);
	// 组合顺序：缩放→旋转→平移（符合图形管线惯例）
	return transMat * rotMat * scaleMat;
}

// 9. 主函数（每个工作组处理64个骨骼）
void main() {
	uint boneIndex = gl_GlobalInvocationID.x; // 骨骼全局索引（从0开始）
	if (boneIndex >= bones.length())  return; // 边界检查（避免越界）

	Bone bone = bones[boneIndex];
	if (bone.keyframeCount < 2) {
		// 关键帧不足，直接使用第一个关键帧
		Keyframe kf = bone.keyframes[0]();
		transforms[boneIndex] = composeTransform(kf.translation, kf.rotation, kf.scale);
		return;
	}

	// 10. 查找当前时间所在的关键帧区间（线性查找，可优化为二分查找）
	uint left = 0, right = bone.keyframeCount - 1;
	for (uint i = 1; i < bone.keyframeCount; i++) {
		if (bone.keyframes[i].time >= currentTime) {
			right = i;
			left = i - 1;
			break;
		}
	}

	// 11. 插值计算（当前时间在left和right关键帧之间）
	Keyframe kfLeft = bone.keyframes[left];
	Keyframe kfRight = bone.keyframes[right];
	float t = clamp(
		(currentTime - kfLeft.time) / (kfRight.time - kfLeft.time),
		0.0, 1.0
	); // 确保t∈[0,1] 

	// 12. 插值平移、旋转、缩放 
	vec3 trans = lerp(kfLeft.translation, kfRight.translation, t);
	vec4 rot = slerp(kfLeft.rotation, kfRight.rotation, t); // 旋转用Slerp
	vec3 scale = lerp(kfLeft.scale, kfRight.scale, t);

	// 13. 生成骨骼变换矩阵（输出到Storage Buffer）
	transforms[boneIndex] = composeTransform(trans, rot, scale);
}
#endif // 0
