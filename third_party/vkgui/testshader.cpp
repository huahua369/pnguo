#include "pch1.h"
/*
pbr材质：金属度metalness、粗糙度roughness、清漆.clearcoat、透光率(透射度).transmission
KHR_materials_ior ：折射率描述了光在穿过对象时是如何散射的。 通过使艺术家能够控制 IOR 值，
						可以使各种透明材料看起来更逼真，包括空气、水、眼睛、玻璃、蓝宝石和钻石。
KHR_materials_volume ：体积扩展使网格表面能够充当体积之间的界面，并实现更逼真的折射和吸收特性，如透明材料中所见。
							这种延伸使半透明材料具有深度和重量的外观。 对于无法进行光线追踪的实时引擎，此扩展还提供了一个厚度纹理贴图，以实现光与大量材料相互作用的快速近似。
KHR_materials_specular ：镜面属性是一个对象的类似镜子的属性：它有规律地反射光线的能力，创建其他对象的相干反射。
						与其前身 KHR_materials_pbrSpecularGlossiness 不同，这个新的镜面反射扩展在 glTF 的 PBR 材料模型核心的现代金属/粗糙度工作流程中运行，使彩色镜面高光与高级 PBR 材料扩展阵列兼容。
*/


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


#define MAX_LIGHT_INSTANCES  32
#define ID_TEXCOORD_0  1
#define ID_TEXCOORD_1  2
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
vec4 textureLod(void*, const vec2& uv, float lod) {
	return vec4(uv, lod, 0.0);
}


#if 0
#include "shaders/functions.h"
#include "shaders/GLTF_VS2PS_IO.h"
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
//#define MATERIAL_TRANSMISSION 1 
#define MATERIAL_METALLICROUGHNESS 1
#define MATERIAL_SPECULARGLOSSINESS 1
#define MATERIAL_ANISOTROPY 1
#define MATERIAL_VOLUME
#define MATERIAL_IRIDESCENCE
#define MATERIAL_DIFFUSE_TRANSMISSION
#define MATERIAL_SHEEN 
#define MATERIAL_CLEARCOAT 
#define MATERIAL_SPECULAR
#define MATERIAL_IRIDESCENCE 
#define MATERIAL_IOR

#define pbr_glsl 1
#define pbr_iridescence 1
#define pbr_ibl 1
#define USE_IBL0 1


#define USE_PUNCTUAL
struct FragCoord {
	glm::vec2 xy;
};
FragCoord gl_FragCoord;
bool gl_FrontFacing = 1;
//--------------------------------------------------------------------------------------
//  PS Inputs
//--------------------------------------------------------------------------------------

#include "shaders/GLTF_VS2PS_IO.h"
VS2PS Input;

namespace pbr
{

#include "shaders/pbrpx.h"

}


#if 0

#include "shaders/perFrameStruct.h"

PerFrame myPerFrame;

//--------------------------------------------------------------------------------------
// PerFrame structure, must match the one in GltfPbrPass.h
//--------------------------------------------------------------------------------------

#include "shaders/PBRTextures.h"


	//PBRFactors u_pbrParams;
pbrMaterial u_pbrParams;
//mat4 myPerObject_u_mCurrWorld;

//--------------------------------------------------------------------------------------
// mainPS
//--------------------------------------------------------------------------------------

#include "shaders/functions.h"
#include "shaders/shadowFiltering.h"
#include "shaders/bsdf_functions.h"
#include "shaders/PixelParams.h"
#include "shaders/GLTFPBRLighting.h"

void amain()
{
	discardPixelIfAlphaCutOff(Input);
	gpuMaterial m = defaultPbrMaterial();
#ifdef ID_TEXCOORD_0
	vec2 uv = Input.UV0;
#else
	vec2 uv = vec2(0.0, 0.0);
#endif
	getPBRParams(Input, u_pbrParams, m);
	//getPBRParams(Input, u_pbrParams, diffuseColor, specularColor, perceptualRoughness, alpha, baseColor);

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


