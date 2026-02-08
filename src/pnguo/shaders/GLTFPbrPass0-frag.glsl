#version 450
//#extension GL_OES_standard_derivatives : enable
#extension GL_EXT_shader_texture_lod: enable 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
// this makes the structures declared with a scalar layout match the c structures
#extension GL_EXT_scalar_block_layout : enable

precision highp float;

//--------------------------------------------------------------------------------------
//  PS Inputs
//--------------------------------------------------------------------------------------

#include "GLTF_VS2PS_IO.h"
layout (location = 0) in VS2PS inInput;

//--------------------------------------------------------------------------------------
// PS Outputs
//--------------------------------------------------------------------------------------

#ifdef HAS_MOTION_VECTORS_RT
layout(location = HAS_MOTION_VECTORS_RT) out vec2 Output_motionVect;
#endif

#ifdef HAS_FORWARD_RT
layout (location = HAS_FORWARD_RT) out vec4 Output_finalColor;
#endif

#ifdef HAS_SPECULAR_ROUGHNESS_RT
layout (location = HAS_SPECULAR_ROUGHNESS_RT) out vec4 Output_specularRoughness;
#endif

#ifdef HAS_DIFFUSE_RT
layout (location = HAS_DIFFUSE_RT) out vec4 Output_diffuseColor;
#endif

#ifdef HAS_NORMALS_RT
layout (location = HAS_NORMALS_RT) out vec4 Output_normal;
#endif

#ifdef HAS_OIT_ACCUM_RT
layout (location = HAS_OIT_ACCUM_RT) out vec4 outAccum;	// 积累纹理（RGBA32F）
layout (location = HAS_OIT_WEIGHT_RT) out float outReveal;	// 权重纹理（R32F）
#endif
//--------------------------------------------------------------------------------------
//
// Constant Buffers 
//
//--------------------------------------------------------------------------------------
#define LINEAR_OUTPUT 1

#include "pbr_pixel.h"
 
//--------------------------------------------------------------------------------------
// mainPS
//--------------------------------------------------------------------------------------
void main()
{  
	discardPixelIfAlphaCutOff(inInput);
	Output_finalColor = px_main();
}
