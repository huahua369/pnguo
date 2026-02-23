#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

precision highp float;

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------

#include "GLTF_VS2PS_IO.h"
layout (location = 0) in VS2PS Input;
#define S_VERT
#include "perFrameStruct.h"
 
PerFrame myPerFrame;
 
//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------

#include "PBRTextures.h" 
//#include "functions.h"
//#include "bsdf_functions.h"
//#include "PixelParams.h"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

void main()
{ 
	myPerFrame.u_LodBias = 0.0;
#ifdef ID_TEXCOORD_0
	vec2 uv = Input.UV[0];
#else
	vec2 uv = vec2(0.0, 0.0);
#endif
	vec4 baseColor = getBaseColor(Input, uv);
	 
	if (baseColor.a < 0.01)
		discard;
#ifdef DEF_alphaCutoff
	if (baseColor.a < DEF_alphaCutoff)
		discard;
#endif
	//discardPixelIfAlphaCutOff(Input);
}
