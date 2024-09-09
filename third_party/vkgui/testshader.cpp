#include "pch1.h"
#define PT_CPP
using namespace glm;

#ifndef M_PI
#define M_PI00 3.14159265358979323846
#endif // !M_PI
#define out

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
