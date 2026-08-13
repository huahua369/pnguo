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


#line 129
vec2 gpu_apply_minv_0(ivec4 m_1, vec2 v_0)
{
    vec4 mf_0 = vec4(m_1) * 0.0009765625;
    float _S1 = v_0.x;

#line 132
    float _S2 = v_0.y;

#line 132
    return vec2(mf_0.x * _S1 + mf_0.y * _S2, mf_0.z * _S1 + mf_0.w * _S2);
}


#line 94
float gpu_extend_t_0(float t_0, int extend_1)
{
    if(extend_1 == 1)
    {

#line 97
        return t_0 - floor(t_0);
    }
    else
    {

#line 99
        if(extend_1 == 2)
        {

#line 100
            float u_0 = t_0 - 2.0 * floor(t_0 * 0.5);

#line 100
            float _S3;
            if(u_0 > 1.0)
            {

#line 101
                _S3 = 2.0 - u_0;

#line 101
            }
            else
            {

#line 101
                _S3 = u_0;

#line 101
            }

#line 101
            return _S3;
        }

#line 96
    }

#line 103
    return clamp(t_0, 0.0, 1.0);
}


#line 88
vec4 gpu_stop_color_0(int i_0, out float offset_0)
{
    offset_0 = uboGrad_1.stops_0[i_0];
    return uboGrad_1.colors_0[i_0];
}


#line 106
vec4 gpu_eval_stops_0(int stop_count_0, float t_1)
{
    float off_prev_0;
    vec4 col_prev_0 = gpu_stop_color_0(0, off_prev_0);
    if(t_1 <= off_prev_0)
    {

#line 111
        return col_prev_0;
    }

#line 111
    vec4 col_prev_1 = col_prev_0;

#line 111
    int i_1 = 1;
    for(;;)
    {

#line 112
        if(i_1 < stop_count_0)
        {
        }
        else
        {

#line 112
            break;
        }
        float off_0;
        vec4 col_0 = gpu_stop_color_0(i_1, off_0);
        if(t_1 <= off_0)
        {
            float span_0 = off_0 - off_prev_0;

#line 118
            float f_0;
            if(span_0 > 9.99999997475242708e-07)
            {

#line 119
                f_0 = (t_1 - off_prev_0) / span_0;

#line 119
            }
            else
            {

#line 119
                f_0 = 0.0;

#line 119
            }
            vec4 pm_0 = mix(col_prev_1, col_0, vec4(f_0));
            if((pm_0.w) > 9.99999997475242708e-07)
            {

#line 121
                col_prev_1 = pm_0;

#line 121
            }
            else
            {

#line 121
                col_prev_1 = vec4(0.0);

#line 121
            }

#line 121
            return col_prev_1;
        }

        off_prev_0 = off_0;

#line 112
        int _S4 = i_1 + 1;

#line 112
        col_prev_1 = col_0;

#line 112
        i_1 = _S4;

#line 112
    }

#line 126
    return col_prev_1;
}


#line 141
vec4 gpu_sample_linear_0(vec2 renderCoord_0, vec2 box_0, int stop_count_1, int extend_2)
{

    vec2 p0_0 = uboGrad_1.cp_0[0].xy / box_0;
    vec2 d_0 = uboGrad_1.cp_0[0].zw / box_0 - p0_0;
    float denom_0 = dot(d_0, d_0);
    if(denom_0 < 9.99999997475242708e-07)
    {

#line 147
        return vec4(0.0);
    }

#line 153
    return gpu_eval_stops_0(stop_count_1, gpu_extend_t_0(dot(gpu_apply_minv_0(uboGrad_1.m_0, renderCoord_0 - p0_0), d_0) / denom_0, extend_2));
}


#line 179
vec4 gpu_sample_radial_0(vec2 renderCoord_1, vec2 box_1, int stop_count_2, int extend_3)
{

    vec2 c0_r_0 = uboGrad_1.cp_0[0].xy / box_1;

    vec2 cd_0 = uboGrad_1.cp_0[1].xy / box_1 - c0_r_0;
    float _S5 = box_1.x;

#line 185
    float r0_0 = uboGrad_1.cp_0[0].z / _S5;

    float dr_0 = uboGrad_1.cp_0[1].z / _S5 - r0_0;

    vec2 p_0 = gpu_apply_minv_0(uboGrad_1.m_0, renderCoord_1 - c0_r_0);

    float A_0 = dot(cd_0, cd_0) - dr_0 * dr_0;
    float B_0 = -2.0 * (dot(p_0, cd_0) + r0_0 * dr_0);
    float C_0 = dot(p_0, p_0) - r0_0 * r0_0;

#line 193
    float t_2;


    if((abs(A_0)) > 9.99999997475242708e-07)
    {
        float disc_0 = B_0 * B_0 - 4.0 * A_0 * C_0;
        if(disc_0 < 0.0)
        {

#line 199
            return vec4(0.0);
        }

#line 200
        float sq_0 = sqrt(disc_0);


        float _S6 = - B_0;

#line 203
        float _S7 = 2.0 * A_0;

#line 203
        float t1_0 = (_S6 + sq_0) / _S7;
        float t2_0 = (_S6 - sq_0) / _S7;
        if((r0_0 + t1_0 * dr_0) >= 0.0)
        {

#line 205
            t_2 = t1_0;

#line 205
        }
        else
        {

#line 205
            t_2 = t2_0;

#line 205
        }

#line 196
    }
    else
    {

#line 209
        if((abs(B_0)) < 9.99999997475242708e-07)
        {

#line 209
            return vec4(0.0);
        }

#line 209
        t_2 = - C_0 / B_0;

#line 196
    }

#line 214
    return gpu_eval_stops_0(stop_count_2, gpu_extend_t_0(t_2, extend_3));
}


#line 248
vec4 gpu_sample_sweep_0(vec2 renderCoord_2, vec2 box_2, int stop_count_3, int extend_4)
{

    ivec4 m_2 = uboGrad_1.m_0;
    vec2 p0_1 = uboGrad_1.cp_0[0].xy / box_2;
    float a0_0 = uboGrad_1.cp_0[0].z;

    float span_1 = uboGrad_1.cp_0[0].w - a0_0;
    if((abs(span_1)) < 9.99999997475242708e-07)
    {

#line 256
        return vec4(0.0);
    }
    vec2 p_1 = gpu_apply_minv_0(m_2, normalize(renderCoord_2 - p0_1));

    float ang_0 = (atan((p_1.y),(p_1.x))) / 3.14159274101257324;

#line 260
    float ang_1;
    if(ang_0 < 0.0)
    {

#line 261
        ang_1 = ang_0 + 2.0;

#line 261
    }
    else
    {

#line 261
        ang_1 = ang_0;

#line 261
    }


    return gpu_eval_stops_0(stop_count_3, gpu_extend_t_0((ang_1 - a0_0) / span_1, extend_4));
}


#line 338
vec4 gpu_paint_0(vec2 renderCoord_3, vec4 inSrc_0, float  inMat_0[8], int inPatType_0)
{
    vec2 box_3 = inSrc_0.xy;
    vec2 box_4 = box_3 * uboGrad_1.scale_0;
    vec2 _S8 = renderCoord_3 / box_4;

    int extend_5 = uboGrad_1.extend_0;
    int stop_count_4 = int(uboGrad_1.count_0);

#line 345
    vec4 col_1;

    switch(inPatType_0)
    {
    case 1:
        {

#line 349
            vec2 p_2 = _S8.xy - box_3;

            float _S9 = p_2.x;

#line 351
            float _S10 = p_2.y;

#line 351
            col_1 = (texture((source_0), (vec2(inMat_0[0] * _S9 + inMat_0[2] * _S10 + inMat_0[4], inMat_0[1] * _S9 + inMat_0[3] * _S10 + inMat_0[5]) / inSrc_0.zw)));

#line 360
            break;
        }
    case 2:
        {

#line 360
            col_1 = gpu_sample_linear_0(_S8, box_4, stop_count_4, extend_5);


            break;
        }
    case 3:
        {

#line 363
            col_1 = gpu_sample_radial_0(_S8, box_4, stop_count_4, extend_5);


            break;
        }
    case 6:
        {

#line 366
            col_1 = gpu_sample_sweep_0(_S8, box_4, stop_count_4, extend_5);


            break;
        }
    default:
        {

#line 369
            col_1 = inSrc_0;

#line 369
            break;
        }
    }

#line 371
    float _S11 = col_1.w;

    return vec4(col_1.xyz * _S11, _S11);
}


#line 373
layout(location = 0)
out vec4 entryPointParam_fragMain_0;


#line 373
layout(location = 1)
in vec4 input_Src_0;


#line 373
flat layout(location = 2)
in int input_PatType_0;


#line 373
flat layout(location = 3)
in float input_Opacity_0;


#line 373
layout(location = 4)
in float  input_Mat_0[8];

void main()
{

#line 376
    entryPointParam_fragMain_0 = gpu_paint_0(gl_FragCoord.xy, input_Src_0, input_Mat_0, input_PatType_0) * input_Opacity_0;

#line 376
    return;
}

