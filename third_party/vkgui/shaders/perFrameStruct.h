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