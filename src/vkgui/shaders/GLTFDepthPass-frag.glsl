#version 450

// AMD Cauldron code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

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
	vec2 uv = Input.UV0;
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
