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
#define PT_CPP
using namespace glm;

#ifndef M_PI
#define M_PI00 3.14159265358979323846
#endif // !M_PI
#define out
#define MAX_LIGHT_INSTANCES  32

glm::vec3 px(const glm::vec2& pss) {
	return vec3(pss, 0.0);// 读坐标像素
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

#include "shaders/functions.h"
#include "shaders/GLTF_VS2PS_IO.glsl"
#include "shaders/perFrameStruct.h"
#include "shaders/shadowFiltering.h"
#include "shaders/PixelParams.glsl"
#include "shaders/GLTFPBRLighting.h"
#include "shaders/skinning.h"

vec2 testshader() {
	vec2 k = {};

	return k;
}
