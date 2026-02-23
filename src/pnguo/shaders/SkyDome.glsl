#version 400


#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
layout (std140, binding = 0) uniform perFrame
{
    mat4 u_mClipToWord; 
} myPerFrame;


//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------
layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
layout(set=0, binding=1) uniform samplerCube inputSampler;

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------
void main()
{
    vec4 clip = vec4(2 * inTexCoord.x - 1, 1 - 2 * inTexCoord.y, 1, 1);

    vec4 pixelDir = myPerFrame.u_mClipToWord * clip;

    outColor = texture(inputSampler, pixelDir.xyz);
}
