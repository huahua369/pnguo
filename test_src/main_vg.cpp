/*

_start_cmd_for_render_pass
_clip_preserve
_fill_preserve
_stroke_preserve

*/
#include <pch1.h> 
#include <vg.h>
#include <render.h>
#include <print_time.h>
#include <gpu_vk.h>
#include <pnguo.h>
#include <gui.h>
#include <app.h>
#include <event.h>
#include <vkvg_cx.h>
#include <win_core.h>
#define white 1, 1, 1
#define red   1, 0, 0
#define green 0, 1, 0
#define blue  0, 0, 1

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::ivec4;
float gpu_extend_t(float t, int extend)
{
	if (extend == 1) {           /* HB_PAINT_EXTEND_REPEAT */
		return t - floor(t);
	}
	else if (extend == 2) {    /* HB_PAINT_EXTEND_REFLECT */
		float u = t - 2.0 * floor(t * 0.5);
		return u > 1.0 ? 2.0 - u : u;
	}
	return glm::clamp(t, 0.0f, 1.0f);  /* PAD (default) */
}


float gpu_sample_radial_hb(vec2 renderCoord, vec2 box, int stop_count, int extend, vec3* cp)
{
	ivec4 m = ivec4(1024, 0, 0, 1024);
	vec2 c0_r = cp[0];
	vec2 cd = cp[1];
	c0_r /= box;
	cd /= box;
	float r0 = cp[1].z / box.x;
	float r1 = cp[0].z / box.x;
	float dr = r1 - r0;
	//vec2 p = gpu_apply_minv(m, renderCoord - c0_r);
	vec2 p = normalize(renderCoord - c0_r);

	float A = dot(cd, cd) - dr * dr;
	float B = -2.0 * (dot(p, cd) + r0 * dr);
	float C = dot(p, p) - r0 * r0;

	float t;
	if (abs(A) > 1e-6)
	{
		float disc = B * B - 4.0 * A * C;
		if (disc < 0.0) return (0.0);
		float sq = sqrt(disc);
		/* Prefer the larger root; fall back to the smaller if the
		 * larger gives a negative interpolated radius. */
		float t1 = (-B + sq) / (2.0 * A);
		float t2 = (-B - sq) / (2.0 * A);
		t = (r0 + t1 * dr >= 0.0) ? t1 : t2;
	}
	else
	{
		if (abs(B) < 1e-6) return (0.0);
		t = -C / B;
	}

	t = gpu_extend_t(t, extend);
	return t;
	//return gpu_eval_stops(stop_count, t);
}

float gpu_sample_radial(vec2 renderCoord, vec2 box, int stop_count, int extend, vec3* cp)
{
	vec2 c0 = cp[0]; c0 /= box;
	vec2 c1 = cp[1]; c1 /= box;
	float r0 = cp[0].z / box.x;
	float r1 = cp[1].z / box.x;
	float gradLength = 1.0;
	vec2 diff = c0 - c1;
	vec2 rayDir = normalize(renderCoord - c0);
	float a = dot(rayDir, rayDir);
	float b = 2.0 * dot(rayDir, diff);
	float cc = dot(diff, diff) - r1 * r1;
	float disc = b * b - 4.0 * a * cc;
	if (disc >= 0.0)
	{
		float t = (-b + sqrt(abs(disc))) / (2.0 * a);
		vec2 projection = c0 + rayDir * t;
		gradLength = distance(projection, c0) - r0;
	}
	else
	{
		return  (0.0);
		//gradient is undefined for this coordinate
	}
	float grad = (distance(renderCoord, c0) - r0) / gradLength;
	float t = grad;
	t = gpu_extend_t(t, extend);
	/*return gpu_eval_stops(stop_count, t);*/
	return t;
}
void test_vkvg(const char* fn, dev_info_c* dc)
{

	vkvg_dev* vctx = new_vkvgdev(dc, 8);
	auto dev = vctx->ctx;
	if (!dev)return;
	//new_spv_base(vctx->vkdev);
	//vkvg_log_level = -1;
	VkvgSurface surf = vctx->new_surface(1024, 1024);
	VkvgContext ctx = vkvg_create(surf);
	vkvg_clear(ctx);
	//vkvg_save(ctx);
	if (1) {
		print_time ptt("vkvg");
		VkvgPattern pat;
		VkvgContext  cr = ctx;


		vkvg_rectangle(cr, 12, 20, 50, 30);
		vkvg_clip(cr);

		pat = vkvg_pattern_create_linear(0.0, 0.0, 0.0, 256.0);
		vkvg_pattern_add_color_stop(pat, 1, 0, 0, 0, 1);
		vkvg_pattern_add_color_stop(pat, 0, 1, 1, 1, 1);
		vkvg_rectangle(cr, 0, 0, 256, 256);
		vkvg_set_source(cr, pat);
		vkvg_fill(cr);
		vkvg_pattern_destroy(pat);
		pat = vkvg_pattern_create_radial(115.2, 102.4, 25.6, 102.4, 102.4, 128.0);
		vkvg_pattern_add_color_stop(pat, 0, 1, 1, 1, 1);
		vkvg_pattern_add_color_stop(pat, 1, 0, 0, 0, 1);
		vkvg_set_source(cr, pat);
		vkvg_arc(cr, 128.0, 128.0, 76.8, 0, 2 * 3.1415926);
		vkvg_fill(cr);
		vkvg_pattern_destroy(pat);
		//vkvg_move_to(cr, 20, 100);
		//vkvg_set_source_color(cr, 0xff0cC616);
		//vkvg_select_font_face(ctx, (char*)u8"新宋体");
		//vkvg_set_font_size(cr, 26);
		//vkvg_show_text(cr, (char*)u8"abc123g0加0123");
		vkvg_set_line_width(ctx, 2);
		vkvg_set_source_rgba(ctx, red, 0.8);
		float scale = 2.0;
		draw_arrow(ctx, glm::vec2(100.5, 300.5), glm::vec2(512.5, 520.5), 5, 20);
		vkvg_set_line_width(ctx, 1);
		vkvg_set_source_rgba(ctx, green, 0.8);
		double        vertices_array[] = { 100, 100, 400, 100, 400, 400, 100, 400, 300, 200, 200, 200, 200, 300, 300, 300 };
		const double* contours_array[] = { vertices_array, vertices_array + 8, vertices_array + 16 };
		int           contours_size = 3;
		for (int i = 0; i < contours_size - 1; i++) {
			auto p = contours_array[i];
			vkvg_move_to(ctx, (p[0] * scale) + 0.5, (p[1] * scale + 0.5));
			p += 2;
			while (p < contours_array[i + 1]) {
				draw_arrow_to(ctx, (p[0] * scale) + 0.5, (p[1] * scale) + 0.5);
				p += 2;
			}
			vkvg_stroke(ctx);
		}

		float dashes[] = { 50.0,  /* ink */
				   10.0,  /* skip */
				   10.0,  /* ink */
				   10.0   /* skip*/
		};
		int    ndash = sizeof(dashes) / sizeof(dashes[0]);
		double offset = -50.0;
		vkvg_save(cr);
		vkvg_set_dash(cr, dashes, ndash, offset);
		vkvg_set_line_width(cr, 10.0);

		vkvg_move_to(cr, 128.0, 25.6);
		vkvg_line_to(cr, 230.4, 230.4);
		vkvg_rel_line_to(cr, -102.4, 0.0);
		vkvg_curve_to(cr, 51.2, 230.4, 51.2, 128.0, 128.0, 128.0);

		vkvg_stroke(cr);
		vkvg_restore(cr);
		vkvg_save(cr);
		vkvg_rectangle(cr, 12, 20, 50, 30);
		vkvg_clip(cr);
		bspline_ct bs;
		std::vector<glm::vec2> pts = { {100,500},{200,600},{300,400},{400,700},{500,500} };
		auto bptr = bs.new_bspline(pts.data(), pts.size());
		if (bptr)
		{
			auto v = bs.sample2(64);
			vkvg_move_to(cr, v[0].x, v[0].y);
			for (size_t i = 1; i < v.size(); i++)
			{
				vkvg_line_to(cr, v[i].x, v[i].y);
			}
			vkvg_set_source_color(cr, 0xff0080ff);
			vkvg_scale(cr, 1.52, 1.52);
			vkvg_set_line_width(cr, 2.0);
			vkvg_stroke(cr);
		}
		vkvg_restore(cr);
		vkvg_set_source_color(cr, 0xff0020ff);
		vkvg_move_to(cr, 128.0, 25.6);
		vkvg_line_to(cr, 230.4, 230.4);
		vkvg_rel_line_to(cr, -102.4, 0.0);
		vkvg_curve_to(cr, 51.2, 230.4, 51.2, 128.0, 128.0, 128.0);
		vkvg_stroke(cr);

		vkvg_select_font_face(ctx, "times");
		vkvg_move_to(ctx, 100.f, 100);
		vkvg_set_font_size(ctx, 30);
		static const char* txt = "The quick brown fox jumps over the lazy dog";
		//vkvg_show_text(ctx, txt);

	}

	vkvg_flush(ctx);
	if (!fn || !*fn)
		fn = "temp/offscreen_vkvg1634.png";
	vkvg_surface_resolve(surf);//msaa采样转换输出

	vkvg_surface_write_to_png(surf, fn);
	vkvg_destroy(ctx);
	vkvg_surface_destroy(surf);
	//vkvg_set_source_rgb(ctx, 0, 0, 0);
	//vkvg_paint(ctx);
	//vkvg_set_source_rgba(ctx, 1, 0.5, 0, 0.9);

	//vkvg_load_font_from_path(ctx, "C:\\Windows\\Fonts\\consola.ttf", "consola");
	//vkvg_set_font_size(ctx, 12);
	//vkvg_select_font_face(ctx, "consola");
	//vkvg_set_font_size(ctx, 16);

	//vkvg_move_to(ctx, 100.0, 100.0);
	//vkvg_show_text(ctx, "vkvg");

	//vkvg_font_extents_t fe;
	//vkvg_font_extents(ctx, &fe);
	//print_boxed(ctx, "abcdefghijklmnopqrstuvwxyz", 20, 60, 20);
	//print_boxed(ctx, "ABC", 20, 160, 60);
	//vkvg_select_font_face(ctx, "mono");
	//print_boxed(ctx, "This is a test string!", 20, 250, 20);
	//print_boxed(ctx, "ANOTHER ONE TO CHECK..", 20, 350, 20);
	//free_vkvgdev(vctx);

	{
		const char* filename = "temp/vkvg_gradient.png";
		VkvgSurface surf = vctx->new_surface(256, 856);
		auto cr = vkvg_create(surf);
		{
			vec2 renderCoord = {};
			vec2 box = { 856,856 };
			int stop_count = 1;
			int extend = 0;
			vec3 cp[2] = {};
			cp[0] = vec3(115.2, 102.4, 25.6);
			cp[1] = vec3(102.4, 102.4, 128.0);
			std::vector<vec2> tv;
			for (size_t y = 0; y < 256; y++)
			{
				for (size_t x = 0; x < 256; x++)
				{
					renderCoord = { x,y };
					renderCoord /= box;
					auto t1 = gpu_sample_radial_hb(renderCoord, box, stop_count, extend, cp);
					auto t2 = gpu_sample_radial(renderCoord, box, stop_count, extend, cp);
					tv.push_back({ t1,t2 });
				}
			}
			print_time ptt(filename);
			VkvgPattern pat;

			pat = vkvg_pattern_create_linear(0.0, 0.0, 0.0, 256.0);
			vkvg_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, 1);
			vkvg_pattern_add_color_stop_rgba(pat, 1, 0, 0, 0, 1);
			vkvg_rectangle(cr, 0, 0, 256, 256);
			vkvg_set_source(cr, pat);
			vkvg_fill(cr);
			vkvg_pattern_destroy(pat);
			//vkvg_translate(cr, 0, 260);
			pat = vkvg_pattern_create_radial(115.2, 102.4, 25.6, 102.4, 102.4, 128.0, true);
			vkvg_pattern_add_color_stop(pat, 0, 1, 1, 1, 1);
			vkvg_pattern_add_color_stop(pat, 1, 0, 0, 0, 1);
			//vkvg_pattern_set_extend(pat, VKVG_EXTEND_REPEAT);
			vkvg_set_source(cr, pat);
			//vkvg_set_source_color(cr, 0xffff8000);
			vkvg_arc(cr, 128.0, 128.0, 76.8, 0, 2 * glm::pi<float>());
			//vkvg_rectangle(cr, 0, 0, 256, 256);
			vkvg_fill(cr);
			vkvg_pattern_destroy(pat);

			vkvg_translate(cr, 0, 260);
			VkvgPattern sg = vkvg_pattern_create_sweep(128, 128, 0, 2);
			vkvg_pattern_add_color_stop(sg, 0.0, white, 1);
			vkvg_pattern_add_color_stop(sg, 0.25, red, 1);
			vkvg_pattern_add_color_stop(sg, 0.55, green, 1);
			vkvg_pattern_add_color_stop(sg, 0.70, blue, 0.51);
			vkvg_pattern_add_color_stop(sg, 1.00, white, 0.51);

			vkvg_set_source(cr, sg);
			vkvg_rectangle(cr, 0, 0, 256, 256);
			vkvg_fill(cr);
			vkvg_pattern_destroy(sg);
			vkvg_translate(cr, 0, 260);
			sg = vkvg_pattern_create_sweep(128, 128, 0, 2);
			vkvg_pattern_add_color_stop(sg, 0.0, white, 1);
			vkvg_pattern_add_color_stop(sg, 0.25, red, 1);
			vkvg_pattern_add_color_stop(sg, 0.55, green, 1);
			vkvg_pattern_add_color_stop(sg, 0.70, blue, 0.51);
			vkvg_pattern_add_color_stop(sg, 1.00, white, 0.51);
			vkvg_set_source(cr, sg);
			//vkvg_rectangle(cr, 0, 0, 256, 256);
			vkvg_arc(cr, 128.0, 128.0, 80, 0, 2 * glm::pi<float>());
			vkvg_fill(cr);
			vkvg_pattern_destroy(sg);
		}
		vkvg_flush(cr);
		vkvg_surface_resolve(surf);//msaa采样转换输出
		vkvg_surface_write_to_png(surf, filename);
		vkvg_destroy(cr);
		vctx->free_surface(surf);
		//exit(1);
	}
#if 0
	{
		const char* filename = "temp/cairo_gradient.png";
		cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 256, 256);
		cairo_t* ctx = cairo_create(surface);
		//struct cairo_gradient_t* grad;
		{
			print_time ptt(filename);
			cairo_pattern_t* grad = cairo_pattern_create_linear(0.0, 0.0, 0.0, 256.0);
			cairo_pattern_add_color_stop_rgba(grad, 0, 1, 1, 1, 1);
			cairo_pattern_add_color_stop_rgba(grad, 1, 0, 0, 0, 1);
			cairo_set_source(ctx, grad);
			cairo_rectangle(ctx, 0, 0, 256, 256);
			cairo_fill(ctx);
			//cairo_pattern_t* rg = cairo_pattern_create_radial(15.2, 12.4, 25.6, 102.4, 102.4, 128.0);
			cairo_pattern_t* rg = cairo_pattern_create_radial(115.2, 102.4, 25.6, 102.4, 102.4, 128.0);
			cairo_pattern_add_color_stop_rgba(rg, 0, 1, 1, 0, 1);
			cairo_pattern_add_color_stop_rgba(rg, 0.2, 0, 1, 0, 1);
			cairo_pattern_add_color_stop_rgba(rg, 1, 1, 0, 0, 1);
			cairo_set_source(ctx, rg);
			//cairo_arc(ctx, 128.0, 128.0, 76.8, 0, 2 * glm::pi<double>());
			cairo_rectangle(ctx, 0, 0, 256, 256);
			cairo_fill(ctx);
			cairo_pattern_destroy(grad);
			cairo_pattern_destroy(rg);
		}
		cairo_surface_write_to_png(surface, filename);
		cairo_destroy(ctx);
		cairo_surface_destroy(surface);
	}
#endif
}
void draw_vgtest(VkvgSurface surf, VkvgSurface img, const glm::ivec2& surfsize) {

	const char* filename = "temp/vkvg_gradient.png";
	auto cr = vkvg_create(surf);
	vkvg_grid_fill(cr, surfsize, glm::ivec2(-1, 0xffdfdfdf), 20);
	//vkvg_grid_fill(cr, surfsize, glm::ivec2(-1, -1), 20);
	if (img) {
		//vkvg_rectangle(cr, 10, 10, surfsize.x * 0.5, surfsize.y * 0.5);
		//vkvg_clip(cr);
		auto pattern = vkvg_pattern_create_for_surface(img);
		vkvg_pattern_set_extend(pattern, VKVG_EXTEND_NONE);
		vkvg_matrix_t   matrix;
		float w = vkvg_surface_get_width(img), h = vkvg_surface_get_height(img);
		vkvg_matrix_init_scale(&matrix, w / 128.0, h / 128.0);
		vkvg_pattern_set_matrix(pattern, &matrix);
		vkvg_set_source(cr, pattern);
		vkvg_rectangle(cr, 0, 0, 128, 128);
		vkvg_set_opacity(cr, 0.2);
		//vkvg_paint(cr);
		vkvg_set_opacity(cr, 1.0);
		vkvg_pattern_destroy(pattern);
	}
	vkvg_translate(cr, 126, 0);
	vkvg_translate(cr, 0, 30);
	vkvg_set_source_color(cr, 0xff0020ff);
	vkvg_select_font_face(cr, "Consolas");
	vkvg_set_font_size(cr, 26);
	//vkvg_show_text(cr, "abcd0gyl");
	vkvg_translate(cr, 0, -30);
	VkvgPattern pat;
	pat = vkvg_pattern_create_linear(0.0, 0.0, 0.0, 256.0);
	vkvg_pattern_add_color_stop_rgba(pat, 0, 0, 0, 1, 0);// 蓝
	vkvg_pattern_add_color_stop_rgba(pat, 1, 1, 0, 0, 1);// 红
	vkvg_rectangle(cr, 0, 0, 128, 256);
	vkvg_set_source(cr, pat);
	vkvg_fill(cr);
	vkvg_pattern_destroy(pat);
	pat = vkvg_pattern_create_linear(0.0, 128.0, 256, 256.0);
	//vkvg_pattern_set_rotate(pat, 179);

	vkvg_pattern_add_color_stop_rgba(pat, 0, 0, 0, 0, 0);// 预乘后蓝
	vkvg_pattern_add_color_stop_rgba(pat, 1, 1, 0, 0, 1);// 红
	vkvg_rectangle(cr, 130, 0, 256, 256);
	vkvg_set_source(cr, pat);
	//vkvg_fill(cr); 
	vkvg_set_line_width(cr, 10);
	vkvg_stroke(cr);
	vkvg_pattern_destroy(pat);
	vkvg_translate(cr, 260, 0);
	//pat = vkvg_pattern_create_radial(128, 128, 25.6, 128, 128, 128.0, true);
	//vkvg_pattern_add_color_stop(pat, 0, 1, .51, .1, 0.81);
	//vkvg_pattern_add_color_stop(pat, 0.50, red, 1);
	//vkvg_pattern_add_color_stop(pat, 1, 1, 0.5, 0, 0.0);
	//vkvg_pattern_set_extend(pat, VKVG_EXTEND_REPEAT);
	pat = vkvg_pattern_create_radial(116, 102, 25, 102, 102, 128.0);
	vkvg_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, 1);
	vkvg_pattern_add_color_stop_rgba(pat, 1, 0, 0, 0, 1);
	vkvg_set_source(cr, pat);
	//vkvg_set_source_color(cr, 0xffff8000);
	vkvg_arc(cr, 128.0, 128.0, 76.8, 0, 2 * glm::pi<float>());
	//vkvg_rectangle(cr, 0, 0, 256, 256);
	vkvg_fill(cr);
	vkvg_pattern_destroy(pat);

	vkvg_translate(cr, 260, 0);
#define yellow 0.5,0.25,0
	int sgw = 3;
	VkvgPattern sg = vkvg_pattern_create_sweep(128, 128, 0, 2);
	vkvg_pattern_add_color_stop(sg, 0.0, yellow, 1);
	float xf = 1.0 / (sgw * 2.0);
	float txf = xf;
	for (size_t i = 0; i < sgw; i++)
	{
		vkvg_pattern_add_color_stop(sg, txf, 1, 0.5, 0, 1);
		txf += xf;
		vkvg_pattern_add_color_stop(sg, txf, yellow, 1);
		txf += xf;
	}
	//vkvg_pattern_add_color_stop(sg, 0.0, white, 1);
	//vkvg_pattern_add_color_stop(sg, 0.25, red, 1);
	//vkvg_pattern_add_color_stop(sg, 0.75, blue, 1);
	//vkvg_pattern_add_color_stop(sg, 0.50, green, 1);
	//vkvg_pattern_add_color_stop(sg, 1.00, white, 1);

	vkvg_set_source(cr, sg);
	//vkvg_rectangle(cr, 0, 0, 256, 256);
	vkvg_arc(cr, 128.0, 128.0, 80, 0, 2 * glm::pi<float>());
	vkvg_fill(cr);
	vkvg_pattern_destroy(sg);
	vkvg_translate(cr, -300, 0);

	sg = vkvg_pattern_create_sweep(128, 128, 0, 2);
	vkvg_pattern_add_color_stop(sg, 0.0, white, 1);
	vkvg_pattern_add_color_stop(sg, 0.25, red, 1);
	vkvg_pattern_add_color_stop(sg, 0.50, green, 1);
	vkvg_pattern_add_color_stop(sg, 0.75, blue, 1);
	vkvg_pattern_add_color_stop(sg, 1.00, white, 1);
	vkvg_set_source(cr, sg);
	//vkvg_rectangle(cr, 0, 0, 256, 256);
	vkvg_arc(cr, 128.0, 128.0, 80, 0, 2 * glm::pi<float>());
	vkvg_fill(cr);
	vkvg_pattern_destroy(sg);

	vkvg_translate(cr, -300, 0);
	vkvg_set_line_width(cr, 6);
	auto M_PI = glm::pi<float>();
	vkvg_rectangle(cr, 12, 12, 232, 70);
	vkvg_new_sub_path(cr); vkvg_arc(cr, 64, 64, 40, 0, 2 * M_PI);
	vkvg_new_sub_path(cr); vkvg_arc_negative(cr, 192, 64, 40, 0, -2 * M_PI);

	vkvg_set_fill_rule(cr, VKVG_FILL_RULE_EVEN_ODD);
	vkvg_set_source_rgb(cr, 0, 0.7, 0); vkvg_fill_preserve(cr);
	vkvg_set_source_rgb(cr, 0, 0, 0); vkvg_stroke(cr);

	vkvg_translate(cr, 0, 128);
	vkvg_rectangle(cr, 12, 12, 232, 70);
	vkvg_new_sub_path(cr); vkvg_arc(cr, 64, 64, 40, 0, 2 * M_PI);
	vkvg_new_sub_path(cr); vkvg_arc_negative(cr, 192, 64, 40, 0, -2 * M_PI);
	vkvg_set_glutess(cr, true);
	vkvg_set_fill_rule(cr, VKVG_FILL_RULE_NON_ZERO);
	vkvg_set_source_rgb(cr, 0, 0, 0.9); vkvg_fill_preserve(cr);
	vkvg_set_source_rgb(cr, 0, 0, 0); vkvg_stroke(cr);


	vkvg_flush(cr);
	vkvg_surface_resolve(surf);//msaa采样转换输出
	vkvg_surface_write_to_png(surf, filename);
	vkvg_destroy(cr);
}
void testgui() {
	//test_vkvg("temp/vgtest0618.png", 0);
	// 常用分辨率
	glm::ivec2 dpis[] = {
		{1024,768},
		{1152,864},
		{1280,720},
		{1280,768},
		{1280,800},
		{1280,960},
		{1280,1024},
		{1360,768},
		{1366,768},
		{1400,1050},
		{1440,900},
		{1600,900},
		{1680,1050},
		{1920,1080},
	};
	auto appx = new app_x();
	std::shared_ptr<app_x> spappx(appx);
	appx->init();
	adevice3_t* gd = new_gdev(0, 0);
	auto dctx = (vkg::cxDevice*)gd->new_device(gd->ctx, 0, 0, 0, 0);
	dev_info_cx devinfo = {};
	get_dev_info(dctx, &devinfo);
	// 准备使用3D渲染器的设备创建SDL渲染器
	appx->app->set_dev(devinfo.inst, devinfo.phy, devinfo.vkdev);
	auto fctx = appx->app->font_ctx;
	fctx->set_cache_size(512, 512);
	auto family = new_font_family(fctx, (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Consolas");
	glm::ivec2 ws = dpis[4];// { 1280, 860 };
	const char* wtitle = (char*)u8"窗口0";
	form_x* form0 = (form_x*)new_form(appx->app, wtitle, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable);
	appx->app->main = form0;

	auto view = new viewdev_cx();
	view->init_vgdev(&devinfo, 8);
	view->familys = family;
	view->app = appx->app;
	auto pcb = view->pcb;
	dom_cx* dom0 = view->get_dom(form0);
	glm::ivec2 surfsize = { 260 * 3,256 };
	VkvgSurface surf = view->_vgdev->new_surface(surfsize.x, surfsize.y);
	auto img = view->_vgdev->new_surface(R"(E:\za\noto-emoji-2.042\third_party\region-flags\png\GB-WLS.png)");
	if (surf) {
		draw_vgtest(surf, img, surfsize);
		//vctx->free_surface(surf);
	}
	auto dvv = new div_cx();
	dvv->set_size({ 500,400 });
	dvv->set_pos({ 100,60 });
	dvv->style.family = family;
	dvv->style.fontsize = 16;

	dvv->border = { 0xffacacac,1,5,0x9f666666 };	// 颜色，线粗，圆角，背景色
	dvv->flex.wrap = flex_wrap::WRAP;
	dvv->flex.direction = flex_direction::ROW;
	dvv->flex.justify_content = flex_align::ALIGN_START;	// x轴，主轴对齐
	dvv->flex.align_items = flex_align::ALIGN_CENTER;		// y轴，交叉轴对齐
	dvv->flex.align_content = flex_align::ALIGN_START;		// y轴，多行交叉轴对齐
	dvv->flex_child.margin_left = 2;		// 子元素外边距
	dvv->flex_child.margin_right = 2;
	dvv->flex_child.margin_top = 2;
	dvv->flex_child.margin_bottom = 2;
	dvv->draggable = true;
	//dvv->flex.direction = flex_direction::COLUMN;
	uint32_t colors[5] = { 0x905050fc,0x9050fc50,0x90fc5050,0x90ffffff,0x90282828 };
	//x=默认，y=鼠标进入，z=按下
	glm::uvec3 gradTop = { 0xff4a4a4a,0x80404040,0xff292929 }, gradBot = { 0xff3a3a3a,0x80303030,0xff1d1d1d };
	glm::uvec3 gradTop1 = { 0xff8a8a8a,0x80bebebe,0xff303030 }, gradBot1 = { 0xff5a5a5a,0x80303030,0xff1d1d1d };
	glm::uvec2 blackb = { 0x805c5c5c , 0x801d1d1d };
#if 1
	for (int i = 0; i < 5; i++) {
		auto btn = new gradient_btn();
		//auto btn = new color_btn();
		btn->rounding = 4;
		dvv->add_widget(btn);
		btn->set_size({ 200,36 });
		btn->back_color = colors[i];
		btn->borderLight = blackb.x;
		btn->borderDark = blackb.y;
		btn->gradTop = gradTop;
		btn->gradBot = gradBot;
		btn->style.fontsize = 16;
		btn->style.color = -1;
		btn->style.color_shadow = 0xcc000000;
		btn->str = (char*)u8"🔥按钮gbutton " + std::to_string(i);
		if (i == 4) {
			btn->gradTop = gradTop1;
			btn->gradBot = gradBot1;
		}
	}
	auto gr = new group_radio_t();
	for (int i = 0; i < 5; i++) {
		auto r = new radio_tl();
		r->set_group(gr);
		r->set_size({ 36,36 });
		r->style.line_col = 0xffff8020;
		r->style.thickness = 1;
		r->style.radius = 7;
		dvv->add_widget(r);
	}
	for (int i = 0; i < 7; i++) {
		auto btn = new color_btn();
		btn->rounding = 4;
		btn->set_btn_color_bgr(fmod(i, 5));
		dvv->add_widget(btn);
		btn->set_size({ 128,36 });
		btn->style.fontsize = 16;
		btn->style.color = -1;
		//btn->style.stroke = 1;
		btn->style.color_stroke = 0x80000000;
		//btn->style.shadow_pos = { 3, 3 };
		btn->style.color_shadow = 0xcc121212;
		//btn->style.mcolor_effect = false;
		btn->str = (char*)u8"🍕按钮 " + std::to_string(5 + i);
	}
	for (int i = 0; i < 4; i++) {
		auto r = new checkbox_tl();
		r->set_size({ 36,36 });
		r->style.line_col = 0xffff8020;
		r->style.thickness = 1;
		dvv->add_widget(r);
	}
	{
		auto r = new checkbox_tl();
		r->set_size({ 36,36 });
		r->style.line_col = 0xffff8020;
		r->style.thickness = 1;
		r->v.mixed = true;
		dvv->add_widget(r);
	}
#endif

	auto edit1 = new edit_cx();
	{
		auto r = edit1;
		r->set_size({ 236,32 });
		//r->set_single(false);
		r->placeholder = (char*)u8"输入文本";
		//dvv->add_widget(r);
		r->dindex = 0;

	}
	auto pro = new progress_tl();
	dvv->add_widget(pro);
	pro->set_size({ 120,26 });
	pro->right_inside = true;
	pro->rounding = 10;
	pro->height = 20;
	pro->width = 110;
	pro->color = { 0xffff9e40, 0x5f6c6c6c };
	pro->set_value(0.5);
	auto colorpick = new colorpick_tl();
	{
		auto c = colorpick;
		c->init(0xff282828, 300, 20, true);
		c->style.fontsize = 15;
		//dvv->add_widget(c);
	}
	static bool loadf = false;

	auto dvv2 = new div_cx();
	{
		auto dvv1 = new div_cx();
		dvv1->set_size({ 180,150 });
		dvv1->set_pos({ 100,60 });
		dvv1->style.family = family;
		dvv1->style.fontsize = 16;
		dvv1->border = { 0xffacacac,1,5,0xaf66f666 };	// 颜色，线粗，圆角，背景色
		dvv1->flex_child.margin_left = 2;		// 子元素外边距
		dvv1->flex_child.margin_right = 2;
		dvv1->flex_child.margin_top = 2;
		dvv1->flex_child.margin_bottom = 2;
		dvv1->flex.wrap = flex_wrap::WRAP;
		dvv1->draggable = true;
		dvv1->_absolute = true;
		{
			auto btn = new color_btn();
			btn->rounding = 4;
			btn->set_btn_color_bgr(fmod(4, 5));
			dvv1->add_widget(btn);
			btn->set_size({ 128,36 });
			btn->style.fontsize = 16;
			btn->style.color = -1;
			btn->str = (char*)u8"🍕按钮 ";
			btn->click_cb = [=](void* p, int clicks, const glm::vec2& mpos) {

				};
			{
				auto r = new checkbox_tl();
				r->set_size({ 36,36 });
				r->style.line_col = 0xffff8020;
				r->style.thickness = 1;
				r->v.mixed = true;
				dvv1->add_widget(r);
			}
			{
				auto r = new switch_tl();
				r->set_size({ 60,36 });
				r->set_value(true);
				r->v.mixed = true;
				dvv1->add_widget(r);
			}
		}
		dvv->add_widget(dvv1);
	}
	auto fpslab = new color_btn();
	{
		dvv2->set_size({ 500,400 });
		dvv2->set_pos({ 10,10 });
		dvv2->style.family = family;
		dvv2->style.fontsize = 16;
		dvv2->border = { 0xffffffff,1,5,0x50f66666 };	// 颜色，线粗，圆角，背景色
		dvv2->flex_child.margin_left = 4;		// 子元素外边距
		dvv2->flex_child.margin_right = 4;
		dvv2->flex_child.margin_top = 4;
		dvv2->flex_child.margin_bottom = 4;
		dvv2->draggable = true;
		dvv2->docking = true;
		dvv2->_absolute = true;
		{
			auto r = new switch_tl();
			r->set_size({ 60,36 });
			r->set_value(true);
			//r->v.mixed = true;
			dvv2->add_widget(r);
		}
		dom0->add_widget(dvv2);
		{
			auto btn = fpslab;
			btn->rounding = 4;
			btn->light = 0;
			btn->style.align = {};
			btn->set_btn_color_bgr(fmod(2, 5));
			btn->effect = uTheme::light;
			btn->_disabled_events = true;
			dvv2->add_widget(btn);
			btn->set_size({ 492,392 });
			btn->style.fontsize = 20;
			btn->style.color = 0xffffffff;
			//btn->style.stroke = 1;
			btn->style.color_stroke = 0x8f0012ff;
			btn->style.shadow_pos = { 1,1 };
			btn->style.color_shadow = 0x80ff2301;
		}
		dvv2->mevent_cb = [=](void* p, int type, const glm::vec2& mps)
			{
				return;
				auto dv = (div_cx*)p;
				if (!dv || !dv->form)return;

				//printf("ad %p\t%d %.2f %.2f\n", dvv2->form, type, mps.x, mps.y);
				if (!dv->form->viewports_enable) {
					glm::vec2 pos = dvv2->get_pos();
					glm::vec2 size = dvv2->get_size();
					glm::vec2 main_pos = appx->app->main->get_pos();
					glm::vec2 main_size = appx->app->main->get_size();


					if (type == (int)event_type2::on_drag) {

						if (pos.x < 0 || pos.y < 0 || pos.x + size.x >  main_size.x || pos.y + size.y >  main_size.y)
						{
						}
					}
					if (type == (int)event_type2::on_dragend) {
						if (pos.x < 0 || pos.y < 0 || pos.x + size.x >  main_size.x || pos.y + size.y >  main_size.y)
						{
							view->set_div(dvv2);
							view->remove_q();
						}
						else {
						}
					}
					return;
				}
				if (type == (int)event_type2::on_down) {
					if (appx->app->main != dvv2->form)
					{
						dvv2->fpos = dvv2->form->get_pos();
						dvv2->curpos = (glm::ivec2)mps - dvv2->fpos;
					}
				}
				if (type == (int)event_type2::on_drag) {
					glm::vec2 main_pos = appx->app->main->get_pos();
					glm::ivec2 ps = {};
					ps += dvv2->mmpos - dvv2->curpos;
					if (appx->app->main != dvv2->form)
					{
						printf("d %p\t%d %d\n", dvv2->form, ps.x, ps.y);
						dvv2->form->set_pos(ps);
						if (dvv2->form->get_visible())
							dvv2->fpos = ps;
					}
					else {
						dv->set_pos(ps);
					}
				}
				//view->get_div(dv->form);
			};
	}
	dom0->add_widget(dvv);
	auto colorpicker = new div_cx();
	{
		// 调色板

		auto dvv = colorpicker;
		dom0->add_widget(dvv);
		dvv->set_size({ 1200,400 });
		dvv->set_pos({ 200,160 });
		dvv->style.family = family;
		dvv->style.fontsize = 16;
		dvv->border = { 0xffacacac,1,5,0xf9666666 };	// 颜色，线粗，圆角，背景色
		dvv->flex.wrap = flex_wrap::NO_WRAP;
		dvv->flex.direction = flex_direction::ROW;
		dvv->flex.justify_content = flex_align::ALIGN_START;	// x轴，主轴对齐
		dvv->flex.align_items = flex_align::ALIGN_CENTER;		// y轴，交叉轴对齐
		dvv->flex.align_content = flex_align::ALIGN_START;		// y轴，多行交叉轴对齐
		dvv->flex_child.margin_left = 2;		// 子元素外边距
		dvv->flex_child.margin_right = 2;
		dvv->flex_child.margin_top = 2;
		dvv->flex_child.margin_bottom = 2;
		dvv->draggable = 1;

		auto r = new edit_cx();
		r->set_size({ 360,360 });
		//r->set_single(false);
		r->placeholder = (char*)u8"输入文本";
		r->dindex = 0;
		r->set_single(false);

		flex_data data;
		data.width = 100.0f;
		data.height = 200.0f;
		data.grow = 1.0f;
		data.justify_content = flex_align::ALIGN_CENTER;

		std::string json_str = save_flex_data(data);
		r->set_text(json_str.c_str(), json_str.length());
		//flex_data loaded = load_flex_data(json_str);

		dvv->add_widget(r);
		auto ibtn = new image_btn();
		if (ibtn) {
			ibtn->surf = surf;
			//ibtn->str = "abcc";
			ibtn->set_size(surfsize);
			dvv->add_widget(ibtn);
		}
	}
	glm::vec4 dcc[] = { glm::vec4(1, 0, 0, 0.5),
		glm::vec4(0, 1, 0, 0.5),
		glm::vec4(0, 0, 1, 0.5) };
	for (int i = 0; i < 0; i++)
	{
		auto dvv = new div_cx();
		dom0->add_widget(dvv);
		dvv->set_size({ 100,100 });
		dvv->set_pos({ 100,60 });
		dvv->style.family = family;
		dvv->style.fontsize = 16;
		dvv->border = { 0xffacacac,1,5, color2u(dcc[i]) };	// 颜色，线粗，圆角，背景色
		dvv->flex.wrap = flex_wrap::WRAP;
		dvv->flex.direction = flex_direction::ROW;
		dvv->flex.justify_content = flex_align::ALIGN_START;	// x轴，主轴对齐
		dvv->flex.align_items = flex_align::ALIGN_CENTER;		// y轴，交叉轴对齐
		dvv->flex.align_content = flex_align::ALIGN_START;		// y轴，多行交叉轴对齐
		dvv->flex_child.margin_left = 2;		// 子元素外边距
		dvv->flex_child.margin_right = 2;
		dvv->flex_child.margin_top = 2;
		dvv->flex_child.margin_bottom = 2;
		dvv->draggable = true;
		dom0->add_widget(dvv);
	}
	dom0->update(0.0f);
	size_t frame_count = 0;
	appx->app->set_fps(60);
	appx->view = view;
	appx->fpslab = fpslab;
	do {
		frame_count = appx->run();
		//draw_vgtest(surf, img, surfsize);
	} while (frame_count);
}
int main() {
	system(R"(rd /s /q C:\Users\hua\AppData\Local\Temp\SymbolCache\vgtest.pdb)");
	//auto rd = hz::shared_load(R"(E:\Program Files\RenderDoc_1.37_64\renderdoc.dll)");
	dev_info_cx devinfo = {};
	testgui();
	return 0;
}