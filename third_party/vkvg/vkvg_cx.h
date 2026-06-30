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
vkvg_status_t vkvg_pattern_set_rotate(VkvgPattern pat, int angle);
vkvg_status_t vkvg_set_glutess(VkvgContext ctx, bool enable);
void vkvg_clear_rect(VkvgContext ctx, int x, int y, int width, int height);

#ifndef vkvg_pattern_add_color_stop_rgba
#define vkvg_pattern_add_color_stop_rgba vkvg_pattern_add_color_stop
#endif 

class vg_ctx
{
public:
	vg_ctx();
	~vg_ctx();

	VkvgContext vkvg_create(VkvgSurface surf);
	void vkvg_destroy(VkvgContext ctx);
	vkvg_status_t vkvg_status(VkvgContext ctx);
	const char* vkvg_status_to_string(vkvg_status_t status);
	VkvgContext vkvg_reference(VkvgContext ctx);
	uint32_t vkvg_get_reference_count(VkvgContext ctx);
	void vkvg_flush(VkvgContext ctx);
	void vkvg_new_path(VkvgContext ctx);
	void vkvg_close_path(VkvgContext ctx);
	void vkvg_new_sub_path(VkvgContext ctx);
	void vkvg_path_extents(VkvgContext ctx, float* const x1, float* const y1, float* const x2, float* const y2);
	void vkvg_get_current_point(VkvgContext ctx, float* x, float* y);
	void vkvg_line_to(VkvgContext ctx, float x, float y);
	void vkvg_rel_line_to(VkvgContext ctx, float dx, float dy);
	void vkvg_move_to(VkvgContext ctx, float x, float y);
	void vkvg_rel_move_to(VkvgContext ctx, float x, float y);
	void vkvg_arc(VkvgContext ctx, float xc, float yc, float radius, float a1, float a2);
	void vkvg_arc_negative(VkvgContext ctx, float xc, float yc, float radius, float a1, float a2);
	void vkvg_curve_to(VkvgContext ctx, float x1, float y1, float x2, float y2, float x3, float y3);
	void vkvg_rel_curve_to(VkvgContext ctx, float x1, float y1, float x2, float y2, float x3, float y3);
	void vkvg_quadratic_to(VkvgContext ctx, float x1, float y1, float x2, float y2);
	void vkvg_rel_quadratic_to(VkvgContext ctx, float x1, float y1, float x2, float y2);
	vkvg_status_t vkvg_rectangle(VkvgContext ctx, float x, float y, float w, float h);
	vkvg_status_t vkvg_rounded_rectangle(VkvgContext ctx, float x, float y, float w, float h, float radius);
	void vkvg_rounded_rectangle2(VkvgContext ctx, float x, float y, float w, float h, float rx, float ry);
	void vkvg_ellipse(VkvgContext ctx, float radiusX, float radiusY, float x, float y, float rotationAngle);
	void vkvg_elliptic_arc_to(VkvgContext ctx, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);
	void vkvg_rel_elliptic_arc_to(VkvgContext ctx, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);
	void vkvg_stroke(VkvgContext ctx);
	void vkvg_stroke_preserve(VkvgContext ctx);
	void vkvg_fill(VkvgContext ctx);
	void vkvg_fill_preserve(VkvgContext ctx);
	void vkvg_paint(VkvgContext ctx);
	void vkvg_clear(VkvgContext ctx);
	void vkvg_reset_clip(VkvgContext ctx);
	void vkvg_clip(VkvgContext ctx);
	void vkvg_clip_preserve(VkvgContext ctx);
	void vkvg_set_opacity(VkvgContext ctx, float opacity);
	float vkvg_get_opacity(VkvgContext ctx);
	void vkvg_set_source_color(VkvgContext ctx, uint32_t c);
	void vkvg_set_source_rgba(VkvgContext ctx, float r, float g, float b, float a);
	void vkvg_set_source_rgb(VkvgContext ctx, float r, float g, float b);
	void vkvg_set_source_surface(VkvgContext ctx, VkvgSurface surf, float x, float y);
	void vkvg_set_source(VkvgContext ctx, VkvgPattern pat);
	void vkvg_set_line_width(VkvgContext ctx, float width);
	void vkvg_set_miter_limit(VkvgContext ctx, float limit);
	float vkvg_get_miter_limit(VkvgContext ctx);
	void vkvg_set_line_cap(VkvgContext ctx, vkvg_line_cap_t cap);
	void vkvg_set_line_join(VkvgContext ctx, vkvg_line_join_t join);
	void vkvg_set_operator(VkvgContext ctx, vkvg_operator_t op);
	void vkvg_set_fill_rule(VkvgContext ctx, vkvg_fill_rule_t fr);
	void vkvg_set_dash(VkvgContext ctx, const float* dashes, uint32_t num_dashes, float offset);
	void vkvg_get_dash(VkvgContext ctx, const float* dashes, uint32_t* num_dashes, float* offset);
	float vkvg_get_line_width(VkvgContext ctx);
	vkvg_line_cap_t vkvg_get_line_cap(VkvgContext ctx);
	vkvg_line_join_t vkvg_get_line_join(VkvgContext ctx);
	vkvg_operator_t vkvg_get_operator(VkvgContext ctx);
	vkvg_fill_rule_t vkvg_get_fill_rule(VkvgContext ctx);
	VkvgPattern vkvg_get_source(VkvgContext ctx);
	VkvgSurface vkvg_get_target(VkvgContext ctx);
	bool vkvg_has_current_point(VkvgContext ctx);
	void vkvg_save(VkvgContext ctx);
	void vkvg_restore(VkvgContext ctx);
	void vkvg_translate(VkvgContext ctx, float dx, float dy);
	void vkvg_scale(VkvgContext ctx, float sx, float sy);
	void vkvg_rotate(VkvgContext ctx, float radians);
	void vkvg_transform(VkvgContext ctx, const vkvg_matrix_t* matrix);
	void vkvg_set_matrix(VkvgContext ctx, const vkvg_matrix_t* matrix);
	void vkvg_get_matrix(VkvgContext ctx, vkvg_matrix_t* const matrix);
	void vkvg_identity_matrix(VkvgContext ctx);
private:

};

class vgpath_ctx;

vgpath_ctx* new_vgctx();
void free_vgctx(vgpath_ctx* p);
