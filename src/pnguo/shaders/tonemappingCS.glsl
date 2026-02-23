#version 420


#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_compute_shader  : enable

layout (std140, binding = 0) uniform perBatch 
{
    float u_exposure; 
    int u_toneMapper; 
} myPerScene;

layout (local_size_x = 8, local_size_y = 8) in;
layout (binding = 1, rgba16f) uniform image2D HDR;

#include "tonemappers.glsl"

vec3 Tonemap(vec3 color, float exposure, int tonemapper)
{
    color *= exposure;

    switch (tonemapper)
    {
        case 0: return AMDTonemapper(color);
        case 1: return DX11DSK(color);
        case 2: return Reinhard(color);
        case 3: return Uncharted2Tonemap(color);
        case 4: return tonemapACES(color);
        case 5: return color;
        default: return vec3(1, 1, 1);
    } 
}

void main() 
{
	ivec2 coords = ivec2(gl_GlobalInvocationID.xy);

    vec4 texColor = imageLoad(HDR, coords).rgba;

    vec3 color = Tonemap(texColor.rgb, myPerScene.u_exposure, myPerScene.u_toneMapper);

    imageStore(HDR, coords, vec4(color, texColor.a));
}
