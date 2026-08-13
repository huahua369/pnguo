#version 450
#extension GL_EXT_scalar_block_layout : require
layout(row_major) uniform;
layout(row_major) buffer;

#line 32 0
struct PushConsts_0
{
    float  mat_0[8];
    float  matInv_0[8];
    vec4 source_0;
    vec2 size_0;
    int fullScreenQuad_srcType_0;
    float opacity_0;
};




layout(binding = 0, set = 1)
layout(scalar) uniform block_PushConsts_0
{
    float  mat_0[8];
    float  matInv_0[8];
    vec4 source_0;
    vec2 size_0;
    int fullScreenQuad_srcType_0;
    float opacity_0;
}pc_0;

#line 16
layout(location = 0)
out vec2 entryPointParam_main_UV_0;


#line 16
layout(location = 1)
out vec4 entryPointParam_main_Src_0;


#line 16
flat layout(location = 2)
out int entryPointParam_main_PatType_0;


#line 16
flat layout(location = 3)
out float entryPointParam_main_Opacity_0;


#line 16
layout(location = 4)
out float  entryPointParam_main_Mat_0[8];


#line 16
layout(location = 0)
in vec2 input_inPos_0;


#line 16
layout(location = 1)
in vec2 input_inUV_0;


#line 16
layout(location = 2)
in vec4 input_inColor_0;


#line 9
struct VSOutput_0
{
    vec4 pos_0;
    vec2 UV_0;
    vec4 Src_0;
    int PatType_0;
    float Opacity_0;
    float  Mat_0[8];
};


#line 61
void main()
{
    VSOutput_0 output_0;
    int _S1 = (pc_0.fullScreenQuad_srcType_0) & 255;

#line 64
    output_0.PatType_0 = _S1;
    output_0.Mat_0 = pc_0.matInv_0;

#line 65
    vec4 _S2;
    if(_S1 == 0)
    {

#line 66
        _S2 = input_inColor_0;

#line 66
    }
    else
    {

#line 66
        _S2 = pc_0.source_0;

#line 66
    }

#line 66
    output_0.Src_0 = _S2;
    output_0.Opacity_0 = pc_0.opacity_0;

    if(((pc_0.fullScreenQuad_srcType_0) & 268435456) == 268435456)
    {
        output_0.pos_0 = vec4(input_inPos_0, 0.0, 1.0);
        output_0.UV_0 = vec2(0.0, 0.0);
        VSOutput_0 _S3 = output_0;

#line 73
        gl_Position = output_0.pos_0;

#line 73
        entryPointParam_main_UV_0 = _S3.UV_0;

#line 73
        entryPointParam_main_Src_0 = _S3.Src_0;

#line 73
        entryPointParam_main_PatType_0 = _S3.PatType_0;

#line 73
        entryPointParam_main_Opacity_0 = _S3.Opacity_0;

#line 73
        entryPointParam_main_Mat_0 = _S3.Mat_0;

#line 73
        return;
    }

    output_0.UV_0 = input_inUV_0;


    float _S4 = input_inPos_0.x;

#line 79
    float _S5 = input_inPos_0.y;



    output_0.pos_0 = vec4(vec2(pc_0.mat_0[0] * _S4 + pc_0.mat_0[2] * _S5 + pc_0.mat_0[4], pc_0.mat_0[1] * _S4 + pc_0.mat_0[3] * _S5 + pc_0.mat_0[5]) * 2.0 / pc_0.size_0 - 1.0, 0.0, 1.0);
    VSOutput_0 _S6 = output_0;

#line 84
    gl_Position = output_0.pos_0;

#line 84
    entryPointParam_main_UV_0 = _S6.UV_0;

#line 84
    entryPointParam_main_Src_0 = _S6.Src_0;

#line 84
    entryPointParam_main_PatType_0 = _S6.PatType_0;

#line 84
    entryPointParam_main_Opacity_0 = _S6.Opacity_0;

#line 84
    entryPointParam_main_Mat_0 = _S6.Mat_0;

#line 84
    return;
}

