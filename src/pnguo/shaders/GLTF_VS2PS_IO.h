
#ifndef VS2PS_struct
#define VS2PS_struct

struct VS2PS
{
#ifdef ID_COLOR_0
	vec3 Color0;
#endif

	vec2 UV[2];

#ifdef ID_NORMAL
	vec3 Normal;
#endif

#ifdef ID_TANGENT
	vec3 Tangent;
	vec3 Binormal;
#endif

	vec3 WorldPos;

#ifdef HAS_MOTION_VECTORS
	vec4 CurrPosition;
	vec4 PrevPosition;
#endif
	float depth;
};

#endif
