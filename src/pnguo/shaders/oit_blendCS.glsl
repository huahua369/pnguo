#version 420

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
#extension GL_ARB_compute_shader  : enable

layout (local_size_x = 8, local_size_y = 8) in; 
layout(set = 0, binding = 0, rgba16f) uniform image2D opaqueTex;  // 不透明纹理 
layout(set = 0, binding = 1, rgba32f) uniform image2D accumTex;   // 积累纹理 
layout(set = 0, binding = 2, r32f) uniform image2D weightTex;  // 权重纹理 
 
void main() 
{
	ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
    vec4 opaqueColor = imageLoad(opaqueTex, coords).rgba;
    vec4 accum = imageLoad(accumTex, coords).rgba;
    float weight = imageLoad(weightTex, coords).r;
    // 计算半透明颜色（归一化） 
    vec3 transparentColor = accum.rgb  / (weight + 1e-6);  // 避免除以零
    float transparentAlpha = accum.a / (weight + 1e-6);   // 半透明总透明度     
    // 混合不透明与半透明结果（alpha混合）
    vec4 color = opaqueColor * (1.0 - transparentAlpha) + vec4(transparentColor, 1.0) * transparentAlpha;
    imageStore(opaqueTex, coords, color);
}
