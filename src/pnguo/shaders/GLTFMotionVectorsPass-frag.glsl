#version 450


#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

precision highp float;

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------

#include "GLTF_VS2PS_IO.h"
layout (location = 0) in VS2PS Input;

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
 
#include "perFrameStruct.h"
 
//
#include "PBRTextures.h" 
//#include "functions.h"
//#include "bsdf_functions.h"
//#include "PixelParams.h"

#ifdef HAS_MOTION_VECTORS
    layout (location = HAS_MOTION_VECTORS) out vec2 motionVect;
#endif

#ifdef HAS_NORMALS
    layout (location = HAS_NORMALS) out vec4 normals;
#endif
 
//--------------------------------------------------------------------------------------
// main
//--------------------------------------------------------------------------------------

void main()
{
    discardPixelIfAlphaCutOff(Input);

#ifdef HAS_MOTION_VECTORS
    motionVect = Input.CurrPosition.xy / Input.CurrPosition.w -
                 Input.PrevPosition.xy / Input.PrevPosition.w;
#endif

#ifdef HAS_NORMALS
    normals = vec4((getPixelNormal(Input) + 1) / 2, 0);
#endif
}