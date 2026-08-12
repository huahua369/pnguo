#version 460
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shader_draw_parameters : require
layout(row_major) uniform;
layout(row_major) buffer;

#line 21 0
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

#line 28
layout(scalar, binding = 0) readonly buffer StructuredBuffer_matrixx3Cfloatx2C4x2C4x3E_t_0 {
    mat4x4 _data[];
} instance_model_matrix_0;

#line 1
layout(location = 0)
out vec2 entryPointParam_main_uv_0;


#line 1
layout(location = 1)
out vec4 entryPointParam_main_color_0;


#line 1
layout(location = 2)
out vec4 entryPointParam_main_color1_0;


#line 3120 1
layout(location = 0)
in vec3 input_pos_0;


#line 3120
layout(location = 1)
in vec2 input_uv_0;


#line 3120
layout(location = 2)
in vec4 input_color_0;


#line 3120
layout(location = 3)
in vec4 input_color1_0;


#line 11 0
struct VSOutput_0
{
    vec4 pos_0;
    vec2 uv_0;
    vec4 color_0;
    vec4 color1_0;
};


#line 30
void main()
{
    VSOutput_0 output_0;

    output_0.pos_0 = (((vec4(input_pos_0, 1.0)) * ((((instance_model_matrix_0._data[uint(uint(gl_InstanceIndex - gl_BaseInstance))]) * (pc_0.mvp_0))))));
    output_0.uv_0 = input_uv_0;
    output_0.color_0 = input_color_0;

    output_0.color1_0 = input_color1_0;

    VSOutput_0 _S1 = output_0;

#line 40
    gl_Position = output_0.pos_0;

#line 40
    entryPointParam_main_uv_0 = _S1.uv_0;

#line 40
    entryPointParam_main_color_0 = _S1.color_0;

#line 40
    entryPointParam_main_color1_0 = _S1.color1_0;

#line 40
    return;
}

