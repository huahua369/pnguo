#version 450
#extension GL_EXT_scalar_block_layout : require
layout(row_major) uniform;
layout(row_major) buffer;

#line 57 0
layout(binding = 0)
uniform sampler2D samplerColor_0;


#line 1092 1
layout(location = 0)
out vec4 entryPointParam_fragMain_0;


#line 1092
layout(location = 0)
in vec2 input_uv_0;


#line 1092
layout(location = 1)
in vec4 input_color_0;


#line 1092
layout(location = 2)
in vec4 input_color1_0;


#line 62 0
void main()
{
    vec4 color_0 = input_color_0;

#line 64
    vec4 _S1;

    if(gl_FrontFacing)
    {

#line 66
        _S1 = input_color_0;

#line 66
    }
    else
    {

#line 66
        _S1 = input_color1_0;

#line 66
    }

    vec4 _S2 = _S1 * (texture((samplerColor_0), (input_uv_0)));

#line 68
    color_0 = _S2;

#line 73
    color_0.xyz = _S2.xyz * _S2.w;

#line 73
    entryPointParam_fragMain_0 = color_0;

#line 73
    return;
}

