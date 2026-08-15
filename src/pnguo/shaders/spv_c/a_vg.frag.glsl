#version 450
#extension GL_EXT_scalar_block_layout : require
layout(row_major) uniform;
layout(row_major) buffer;

#line 19 0
struct uboGrad_0
{
    vec4  colors_0[32];
    float  stops_0[32];
    vec4  cp_0[2];
    ivec4 m_0;
    vec2 scale_0;
    uint count_0;
    int extend_0;
};


#line 47
layout(binding = 0, set = 3)
layout(scalar) uniform block_uboGrad_0
{
    vec4  colors_0[32];
    float  stops_0[32];
    vec4  cp_0[2];
    ivec4 m_0;
    vec2 scale_0;
    uint count_0;
    int extend_0;
}uboGrad_1;

#line 48
layout(binding = 0, set = 2)
uniform sampler2D source_0;


#line 125
vec2 gpu_apply_minv_0(ivec4 m_1, vec2 v_0)
{
    vec4 mf_0 = vec4(m_1) * 0.0009765625;
    float _S1 = v_0.x;

#line 128
    float _S2 = v_0.y;

#line 128
    return vec2(mf_0.x * _S1 + mf_0.y * _S2, mf_0.z * _S1 + mf_0.w * _S2);
}


#line 90
float gpu_extend_t_0(float t_0, int extend_1)
{
    if(extend_1 == 1)
    {

#line 93
        return t_0 - floor(t_0);
    }
    else
    {

#line 95
        if(extend_1 == 2)
        {

#line 96
            float u_0 = t_0 - 2.0 * floor(t_0 * 0.5);

#line 96
            float _S3;
            if(u_0 > 1.0)
            {

#line 97
                _S3 = 2.0 - u_0;

#line 97
            }
            else
            {

#line 97
                _S3 = u_0;

#line 97
            }

#line 97
            return _S3;
        }

#line 92
    }

#line 99
    return clamp(t_0, 0.0, 1.0);
}


#line 84
vec4 gpu_stop_color_0(int i_0, out float offset_0)
{
    offset_0 = uboGrad_1.stops_0[i_0];
    return uboGrad_1.colors_0[i_0];
}


#line 102
vec4 gpu_eval_stops_0(int stop_count_0, float t_1)
{
    float off_prev_0;
    vec4 col_prev_0 = gpu_stop_color_0(0, off_prev_0);
    if(t_1 <= off_prev_0)
    {

#line 107
        return col_prev_0;
    }

#line 107
    vec4 col_prev_1 = col_prev_0;

#line 107
    int i_1 = 1;
    for(;;)
    {

#line 108
        if(i_1 < stop_count_0)
        {
        }
        else
        {

#line 108
            break;
        }
        float off_0;
        vec4 col_0 = gpu_stop_color_0(i_1, off_0);
        if(t_1 <= off_0)
        {
            float span_0 = off_0 - off_prev_0;

#line 114
            float f_0;
            if(span_0 > 9.99999997475242708e-07)
            {

#line 115
                f_0 = (t_1 - off_prev_0) / span_0;

#line 115
            }
            else
            {

#line 115
                f_0 = 0.0;

#line 115
            }
            vec4 pm_0 = mix(col_prev_1, col_0, vec4(f_0));
            if((pm_0.w) > 9.99999997475242708e-07)
            {

#line 117
                col_prev_1 = pm_0;

#line 117
            }
            else
            {

#line 117
                col_prev_1 = vec4(0.0);

#line 117
            }

#line 117
            return col_prev_1;
        }

        off_prev_0 = off_0;

#line 108
        int _S4 = i_1 + 1;

#line 108
        col_prev_1 = col_0;

#line 108
        i_1 = _S4;

#line 108
    }

#line 122
    return col_prev_1;
}


#line 137
vec4 gpu_sample_linear_0(vec2 renderCoord_0, vec2 box_0, int stop_count_1, int extend_2)
{



    vec2 _S5 = uboGrad_1.cp_0[0].xy;
    vec2 d_0 = uboGrad_1.cp_0[0].zw / box_0 - _S5 / box_0;
    float denom_0 = dot(d_0, d_0);
    if(denom_0 < 9.99999997475242708e-07)
    {

#line 145
        return vec4(0.0);
    }

#line 152
    return gpu_eval_stops_0(stop_count_1, gpu_extend_t_0(dot(gpu_apply_minv_0(uboGrad_1.m_0, (renderCoord_0 - _S5) / box_0), d_0) / denom_0, extend_2));
}


vec4 gpu_sample_radial_0(vec2 renderCoord_1, vec2 box_1, int stop_count_2, int extend_3)
{



    vec2 _S6 = uboGrad_1.cp_0[0].xy;

    vec2 cd_0 = uboGrad_1.cp_0[1].xy / box_1 - _S6 / box_1;
    float _S7 = box_1.x;

#line 164
    float r0_0 = uboGrad_1.cp_0[0].z / _S7;

    float dr_0 = uboGrad_1.cp_0[1].z / _S7 - r0_0;


    vec2 p_0 = gpu_apply_minv_0(uboGrad_1.m_0, (renderCoord_1 - _S6) / box_1);

    float A_0 = dot(cd_0, cd_0) - dr_0 * dr_0;
    float B_0 = -2.0 * (dot(p_0, cd_0) + r0_0 * dr_0);
    float C_0 = dot(p_0, p_0) - r0_0 * r0_0;

#line 173
    float t_2;


    if((abs(A_0)) > 9.99999997475242708e-07)
    {
        float disc_0 = B_0 * B_0 - 4.0 * A_0 * C_0;
        if(disc_0 < 0.0)
        {

#line 179
            return vec4(0.0);
        }

#line 180
        float sq_0 = sqrt(disc_0);


        float _S8 = - B_0;

#line 183
        float _S9 = 2.0 * A_0;

#line 183
        float t1_0 = (_S8 + sq_0) / _S9;
        float t2_0 = (_S8 - sq_0) / _S9;
        if((r0_0 + t1_0 * dr_0) >= 0.0)
        {

#line 185
            t_2 = t1_0;

#line 185
        }
        else
        {

#line 185
            t_2 = t2_0;

#line 185
        }

#line 176
    }
    else
    {

#line 189
        if((abs(B_0)) < 9.99999997475242708e-07)
        {

#line 189
            return vec4(0.0);
        }

#line 189
        t_2 = - C_0 / B_0;

#line 176
    }

#line 194
    return gpu_eval_stops_0(stop_count_2, gpu_extend_t_0(t_2, extend_3));
}

vec4 gpu_sample_sweep_0(vec2 renderCoord_2, vec2 box_2, int stop_count_3, int extend_4)
{

    vec2 rCoord_0 = renderCoord_2 / box_2;

    ivec4 m_2 = uboGrad_1.m_0;
    vec2 p0_0 = uboGrad_1.cp_0[0].xy / box_2;
    float a0_0 = uboGrad_1.cp_0[0].z;

    float span_1 = uboGrad_1.cp_0[0].w - a0_0;
    if((abs(span_1)) < 9.99999997475242708e-07)
    {

#line 207
        return vec4(0.0);
    }

    vec2 p_1 = gpu_apply_minv_0(m_2, normalize(rCoord_0 - p0_0));

    float ang_0 = (atan((p_1.y),(p_1.x))) / 3.14159274101257324;

#line 212
    float ang_1;
    if(ang_0 < 0.0)
    {

#line 213
        ang_1 = ang_0 + 2.0;

#line 213
    }
    else
    {

#line 213
        ang_1 = ang_0;

#line 213
    }


    return gpu_eval_stops_0(stop_count_3, gpu_extend_t_0((ang_1 - a0_0) / span_1, extend_4));
}


#line 290
vec4 gpu_paint_0(vec2 renderCoord_3, vec4 inSrc_0, mat2x3 inMat_0, int inPatType_0)
{
    vec2 box_3 = inSrc_0.xy;
    vec2 box_4 = box_3 * uboGrad_1.scale_0;

    int extend_5 = uboGrad_1.extend_0;
    int stop_count_4 = int(uboGrad_1.count_0);

#line 296
    vec4 col_1;

    switch(inPatType_0)
    {
    case 1:
        {

#line 298
            col_1 = (texture((source_0), ((((vec3(renderCoord_3.xy - box_3, 1.0)) * (inMat_0))) / inSrc_0.zw)));

#line 306
            break;
        }
    case 2:
        {

#line 306
            col_1 = gpu_sample_linear_0(renderCoord_3, box_4, stop_count_4, extend_5);


            break;
        }
    case 3:
        {

#line 309
            col_1 = gpu_sample_radial_0(renderCoord_3, box_4, stop_count_4, extend_5);


            break;
        }
    case 6:
        {

#line 312
            col_1 = gpu_sample_sweep_0(renderCoord_3, box_4, stop_count_4, extend_5);


            break;
        }
    default:
        {

#line 315
            col_1 = inSrc_0;

#line 315
            break;
        }
    }

#line 317
    float _S10 = col_1.w;

    return vec4(col_1.xyz * _S10, _S10);
}


#line 319
layout(location = 0)
out vec4 entryPointParam_fragMain_0;


#line 319
layout(location = 1)
in vec4 input_Src_0;


#line 319
flat layout(location = 2)
in int input_PatType_0;


#line 319
flat layout(location = 3)
in float input_Opacity_0;


#line 319
layout(location = 4)
in mat2x3 input_Mat_0;

void main()
{

#line 322
    entryPointParam_fragMain_0 = gpu_paint_0(gl_FragCoord.xy, input_Src_0, input_Mat_0, input_PatType_0) * input_Opacity_0;

#line 322
    return;
}

