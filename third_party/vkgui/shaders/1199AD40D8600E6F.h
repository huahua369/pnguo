//#version 450
#define DEF_alphaCutoff 0.500000
#define DEF_alphaMode_OPAQUE 1
#define DEF_doubleSided 1
#define HAS_FORWARD_RT 0
#define HAS_MOTION_VECTORS 1
#define HAS_MOTION_VECTORS_RT 1
#define ID_GGXLUT 0
#define ID_NORMAL 0
#define ID_PER_FRAME 0
#define ID_PER_OBJECT 1
#define ID_POSITION 1
#define ID_TEXCOORD_0 2
#define ID_brdfTexture 0
#define ID_diffuseCube 1
#define ID_shadowMap 3
#define ID_specularCube 2
#define MATERIAL_METALLICROUGHNESS 1
#define USE_IBL 1
#define UVT_count 
#define pbr_glsl 1


// Portions Copyright 2019 Advanced Micro Devices, Inc.All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
 
// This shader code was ported from https://github.com/KhronosGroup/glTF-WebGL-PBR
// All credits should go to his original author.

//
// This fragment shader defines a reference implementation for Physically Based Shading of
// a microfacet surface material defined by a glTF model.
//
// References:
// [1] Real Shading in Unreal Engine 4
//     http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// [2] Physically Based Shading at Disney
//     http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
// [3] README.md - Environment Maps
//     https://github.com/KhronosGroup/glTF-WebGL-PBR/#environment-maps
// [4] "An Inexpensive BRDF Model for Physically based Rendering" by Christophe Schlick
//     https://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf

//#extension GL_OES_standard_derivatives : enable

 

#define USE_PUNCTUAL

//--------------------------------------------------------------------------------------
//  PS Inputs
//--------------------------------------------------------------------------------------

#include "GLTF_VS2PS_IO.h"
  VS2PS Input;

//--------------------------------------------------------------------------------------
// PS Outputs
//--------------------------------------------------------------------------------------

#ifdef HAS_MOTION_VECTORS_RT
  out vec2 Output_motionVect;
#endif

#ifdef HAS_FORWARD_RT
  out vec4 Output_finalColor;
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

//--------------------------------------------------------------------------------------
//
// Constant Buffers 
//
//--------------------------------------------------------------------------------------

#ifdef pbr_glsl
#include "pbrpx.h"
#else
//--------------------------------------------------------------------------------------
// Per Frame structure, must match the one in GlTFCommon.h
//--------------------------------------------------------------------------------------

#include "perFrameStruct.h"

layout (scalar, set=0, binding = 0) uniform perFrame 
{
	PerFrame myPerFrame; 
};

//--------------------------------------------------------------------------------------
// PerFrame structure, must match the one in GltfPbrPass.h
//--------------------------------------------------------------------------------------
 
#include "PBRTextures.h" 

layout (scalar, set=0, binding = 1) uniform perObject 
{
    mat4 myPerObject_u_mCurrWorld;
    mat4 myPerObject_u_mPrevWorld;

	//PBRFactors u_pbrParams;
	pbrMaterial u_pbrParams;
};


#include "functions.h"
#include "shadowFiltering.h"
#include "bsdf_functions.h"
#include "PixelParams.h"
#include "GLTFPBRLighting.h"
#endif

//--------------------------------------------------------------------------------------
// mainPS
//--------------------------------------------------------------------------------------
void main()
{

#ifdef pbr_glsl 
	vsio_ps ipt;
#ifdef ID_COLOR_0
	ipt.v_Color0 = Input.Color0;
#else
	ipt.v_Color0 = vec4(0);
#endif
#ifdef ID_TEXCOORD_0
	ipt.v_texcoord[0] = Input.UV0;
#else
	ipt.v_texcoord[0] = vec2(0.0, 0.0);
#endif 
#ifdef ID_TEXCOORD_1
	ipt.v_texcoord[1] = Input.UV1;
#else
	ipt.v_texcoord[1] = vec2(0.0, 0.0);
#endif 
	ipt.v_Position = Input.WorldPos;	
	ipt.u_ModelMatrix = myPerObject_u_mCurrWorld;
	ipt.u_ViewMatrix = myPerFrame.u_mCameraCurrViewProj;
	vec4 color = pbr_main(ipt);
#else
	discardPixelIfAlphaCutOff(Input);
	gpuMaterial m = defaultPbrMaterial();
#ifdef ID_TEXCOORD_0
	vec2 uv = Input.UV0;
#else
	vec2 uv = vec2(0.0, 0.0);
#endif
	m.uv = uv;
	getPBRParams(Input, u_pbrParams, m);
	if (u_pbrParams.alphaMode == ALPHA_MASK)
	{
		if (m.alpha < u_pbrParams.alphaCutoff)
			discard;
	}
	vec3 c3 = doPbrLighting(Input, myPerFrame, m);
	vec4 color = vec4(c3, m.baseColor.a);
#endif
#if 1
	float perceptualRoughness;
	vec3 diffuseColor;
	vec3 specularColor;
	vec4 baseColor = get_roughness(Input, u_pbrParams, uv, diffuseColor, specularColor, perceptualRoughness); 
	vec4 color1 = vec4(doPbrLighting_old(Input, myPerFrame, uv, diffuseColor, specularColor, perceptualRoughness, baseColor), baseColor.a);
	color = color1;
#endif 


#ifdef HAS_MOTION_VECTORS_RT
//	Output_motionVect = Input.CurrPosition.xy / Input.CurrPosition.w - Input.PrevPosition.xy / Input.PrevPosition.w;
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
	//Output_finalColor = mix(color, vec4(myPerFrame.u_WireframeOptions.rgb, 1.0), myPerFrame.u_WireframeOptions.w);  
#endif
}
