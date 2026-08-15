#version 450
#extension GL_EXT_scalar_block_layout : require
layout(row_major) uniform;
layout(row_major) buffer;

#line 32 0
struct PushConsts_0
{
    mat2x3 mat_0;
    mat2x3 matInv_0;
    vec4 source_0;
    vec2 size_0;
    int fullScreenQuad_srcType_0;
    float opacity_0;
};




layout(binding = 0, set = 1)
layout(scalar) uniform block_PushConsts_0
{
    mat2x3 mat_0;
    mat2x3 matInv_0;
    vec4 source_0;
    vec2 size_0;
    int fullScreenQuad_srcType_0;
    float opacity_0;
}pc_0;

#line 13092 1
layout(location = 0)
out vec2 entryPointParam_main_UV_0;


#line 13092
layout(location = 1)
out vec4 entryPointParam_main_Src_0;


#line 13092
flat layout(location = 2)
out int entryPointParam_main_PatType_0;


#line 13092
flat layout(location = 3)
out float entryPointParam_main_Opacity_0;


#line 13092
layout(location = 4)
out mat2x3 entryPointParam_main_Mat_0;


#line 13092
layout(location = 0)
in vec2 input_inPos_0;


#line 13092
layout(location = 1)
in vec2 input_inUV_0;


#line 13092
layout(location = 2)
in vec4 input_inColor_0;


#line 9 0
struct VSOutput_0
{
    vec4 pos_0;
    vec2 UV_0;
    vec4 Src_0;
    int PatType_0;
    float Opacity_0;
    mat2x3 Mat_0;
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


    output_0.pos_0 = vec4((((vec3(input_inPos_0, 1.0)) * (pc_0.mat_0))) * 2.0 / pc_0.size_0 - 1.0, 0.0, 1.0);
    VSOutput_0 _S4 = output_0;

#line 80
    gl_Position = output_0.pos_0;

#line 80
    entryPointParam_main_UV_0 = _S4.UV_0;

#line 80
    entryPointParam_main_Src_0 = _S4.Src_0;

#line 80
    entryPointParam_main_PatType_0 = _S4.PatType_0;

#line 80
    entryPointParam_main_Opacity_0 = _S4.Opacity_0;

#line 80
    entryPointParam_main_Mat_0 = _S4.Mat_0;

#line 80
    return;
}

