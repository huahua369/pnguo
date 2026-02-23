
#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "tonemappers.glsl"

layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

layout (std140, binding = 0) uniform perBatch 
{
    float u_exposure; 
    int u_toneMapper; 
    int u_applyGamma;
} myPerScene;

layout(set=0, binding=1) uniform sampler2D sSampler;

vec3 ApplyGamma(vec3 color)
{
    return pow(abs(color.rgb), vec3(1.0 / 2.2));    
}

vec3 Tonemap(vec3 color, float exposure, int tonemapper)
{
    color *= exposure;

    switch (tonemapper)
    {
        case 0: return AMDTonemapper(color);
        case 1: return DX11DSK(color);
        case 2: return Reinhard(color);
        case 3: return Uncharted2Tonemap(color);
        case 4: return tonemapACES( color );
        case 5: return color;
        default: return vec3(1, 1, 1);
    } 
}

void main() 
{
    if (myPerScene.u_exposure<0)
    {
        outColor = texture(sSampler, inTexCoord.st);
        return;
    }

    vec4 texColor = texture(sSampler, inTexCoord.st);

    vec3 color = Tonemap(texColor.rgb, myPerScene.u_exposure, myPerScene.u_toneMapper);

    outColor = vec4(ApplyGamma(color),1.0);
}
