#version 450
#extension GL_EXT_scalar_block_layout : require
layout(row_major) uniform;
layout(row_major) buffer;

#line 57 0
layout(binding = 0)
uniform sampler2D samplerColor_0;


#line 21
struct PushConsts_0
{
    mat4x4 mvp_0;
    float masktime_0;
};


#line 26
layout(push_constant)
layout(scalar) uniform block_PushConsts_0
{
    mat4x4 mvp_0;
    float masktime_0;
}pc_0;

#line 59
layout(binding = 1)
uniform sampler2D samplerMask_0;


#line 14549 1
layout(location = 0)
out vec4 entryPointParam_fragMain_0;


#line 14549
layout(location = 0)
in vec2 input_uv_0;


#line 14549
layout(location = 1)
in vec4 input_color_0;


#line 62 0
void main()
{

#line 70
    vec4 _S1 = input_color_0 * (texture((samplerColor_0), (input_uv_0))) * (1.0 - step(pc_0.masktime_0, (texture((samplerMask_0), (input_uv_0))).x));

#line 64
    vec4 color_0 = _S1;

#line 73
    color_0.xyz = _S1.xyz * _S1.w;

#line 73
    entryPointParam_fragMain_0 = color_0;

#line 73
    return;
}

