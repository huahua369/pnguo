#pragma once
/* cpp版vkvg头


*/
#include "vkvg.h"
// 创建径向渐变
VkvgPattern vkvg_pattern_create_radial(float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse);
vkvg_status_t vkvg_pattern_edit_radial(VkvgPattern pat, float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse);
// 创建锥形渐变，输入中心坐标，角度[0,2]
VkvgPattern vkvg_pattern_create_sweep(float cx, float cy, float start_angle, float end_angle);
// 设置缩放box比例
vkvg_status_t vkvg_pattern_set_scale(VkvgPattern pat, float scale_x, float scale_y);

#ifndef vkvg_pattern_add_color_stop_rgba
#define vkvg_pattern_add_color_stop_rgba vkvg_pattern_add_color_stop
#endif 

