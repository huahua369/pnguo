#version 450
#extension GL_EXT_scalar_block_layout : require
layout(row_major) uniform;
layout(row_major) buffer;

#line 33 0
struct PushConsts_0
{
    vec4 source_0;
    vec2 size_0;
    int fullScreenQuad_srcType_0;
    float opacity_0;
    mat3x2 mat_0;
    mat3x2 matInv_0;
};

layout(push_constant)
layout(scalar) uniform block_PushConsts_0
{
    vec4 source_0;
    vec2 size_0;
    int fullScreenQuad_srcType_0;
    float opacity_0;
    mat3x2 mat_0;
    mat3x2 matInv_0;
}pc_0;

#line 2
layout(location = 0)
out vec3 entryPointParam_main_UV_0;


#line 2
layout(location = 1)
out vec4 entryPointParam_main_Src_0;


#line 2
flat layout(location = 2)
out int entryPointParam_main_PatType_0;


#line 2
flat layout(location = 3)
out float entryPointParam_main_Opacity_0;


#line 2
layout(location = 4)
out mat3x2 entryPointParam_main_Mat_0;


#line 2
layout(location = 0)
in vec2 input_inPos_0;


#line 2
layout(location = 1)
in vec4 input_inColor_0;


#line 2
layout(location = 2)
in vec3 input_inUV_0;


#line 9
struct VSOutput_0
{
    vec3 UV_0;
    vec4 Src_0;
    int PatType_0;
    float Opacity_0;
    mat3x2 Mat_0;
    vec4 pos_0;
};


#line 57
void main()
{
    VSOutput_0 output_0;
    int _S1 = (pc_0.fullScreenQuad_srcType_0) & 255;

#line 60
    output_0.PatType_0 = _S1;
    output_0.Mat_0 = pc_0.matInv_0;

#line 61
    vec4 _S2;
    if(_S1 == 0)
    {

#line 62
        _S2 = input_inColor_0;

#line 62
    }
    else
    {

#line 62
        _S2 = pc_0.source_0;

#line 62
    }

#line 62
    output_0.Src_0 = _S2;
    output_0.Opacity_0 = pc_0.opacity_0;

    if(((pc_0.fullScreenQuad_srcType_0) & 268435456) == 268435456)
    {
        output_0.pos_0 = vec4(input_inPos_0, 0.0, 1.0);
        output_0.UV_0 = vec3(0.0, 0.0, -1.0);
        VSOutput_0 _S3 = output_0;

#line 69
        entryPointParam_main_UV_0 = output_0.UV_0;

#line 69
        entryPointParam_main_Src_0 = _S3.Src_0;

#line 69
        entryPointParam_main_PatType_0 = _S3.PatType_0;

#line 69
        entryPointParam_main_Opacity_0 = _S3.Opacity_0;

#line 69
        entryPointParam_main_Mat_0 = _S3.Mat_0;

#line 69
        gl_Position = _S3.pos_0;

#line 69
        return;
    }

    output_0.UV_0 = input_inUV_0;


    float _S4 = input_inPos_0.x;

#line 75
    float _S5 = input_inPos_0.y;



    output_0.pos_0 = vec4(vec2(pc_0.mat_0[0][0] * _S4 + pc_0.mat_0[1][0] * _S5 + pc_0.mat_0[2][0], pc_0.mat_0[0][1] * _S4 + pc_0.mat_0[1][1] * _S5 + pc_0.mat_0[2][1]) * 2.0 / pc_0.size_0 - 1.0, 0.0, 1.0);
    VSOutput_0 _S6 = output_0;

#line 80
    entryPointParam_main_UV_0 = output_0.UV_0;

#line 80
    entryPointParam_main_Src_0 = _S6.Src_0;

#line 80
    entryPointParam_main_PatType_0 = _S6.PatType_0;

#line 80
    entryPointParam_main_Opacity_0 = _S6.Opacity_0;

#line 80
    entryPointParam_main_Mat_0 = _S6.Mat_0;

#line 80
    gl_Position = _S6.pos_0;

#line 80
    return;
}

