
#include <pch1.h>
#include <stdint.h>
#include <stdio.h>
// 实现
//#define GLM_ENABLE_EXPERIMENTAL 
//#include <glm/glm.hpp>
//#include <glm/gtx/vector_angle.hpp>
//#include <glm/gtx/closest_point.hpp>
//#include <glm/gtc/type_ptr.hpp>

//#include <pnguo.h>
#include <vkvg/vkvg.h> 
#include "vkvgcx.h"
#include <stb_rect_pack.h> 
#include <print_time.h>
	// 线，二阶曲线，三阶曲线
enum class vtype_e1 :uint8_t
{
	e_vmove = 1,
	e_vline,
	e_vcurve,	// 二次曲线控制点[c]
	e_vcubic	// 三次曲线控制点[c,c1]
};
#ifndef path_v_dh
struct dash_t
{
	const double* dashes;
	int	      num_dashes;
	double	      offset;
};
struct style_path_t
{
	glm::vec2 pos = {};
	glm::vec2 scale = { 1.0f,1.0f };
	uint32_t fill_color = 0;
	uint32_t stroke_color = 0xffffffff;
	float line_width = 1.0f;
	dash_t dash = {};
	int8_t cap = 0;
	int8_t join = 0;
	int8_t flip_y = 0;
};
#endif
template<class T>
void draw_path_vkvg(VkvgContext cr, T* p, style_path_t* st)
{
	if (!p || !st || (!st->fill_color && !st->stroke_color || !cr))return;
	auto t = p->v;
	bool stroke = false;
	vkvg_save(cr);
	vkvg_set_fill_rule(cr, VKVG_FILL_RULE_EVEN_ODD);
	glm::vec2 pos, scale;
	pos += st->pos;
	pos.x += p->x;
	pos.y += p->y;
	if (st->flip_y)
	{
		vkvg_matrix_t flip_y = {};
		vkvg_matrix_init(&flip_y, 1, 0, 0, -1, 0, 0); // 垂直翻转
		vkvg_set_matrix(cr, &flip_y);
		pos.y = -pos.y;
	}
	vkvg_translate(cr, pos.x, pos.y);
	if (st->scale.x > 0 && st->scale.y > 0)
	{
		vkvg_scale(cr, st->scale.x, st->scale.y);
	}
	else if (scale.x > 0 && scale.y > 0)
	{
		vkvg_scale(cr, scale.x, scale.y);
	}
	if (st->line_width > 0 && st->stroke_color)
	{
		stroke = true;
		vkvg_set_line_width(cr, st->line_width);
		vkvg_set_line_cap(cr, (vkvg_line_cap_t)st->cap);
		vkvg_set_line_join(cr, (vkvg_line_join_t)st->join);
	}
	if (st->dash.dashes && st->dash.num_dashes)
	{
		std::vector<float> dashv;
		dashv.resize(st->dash.num_dashes);
		for (size_t i = 0; i < st->dash.num_dashes; i++)
		{
			dashv[i] = st->dash.dashes[i];
		}
		//vkvg_set_dash(cr, st->dash.dashes, st->dash.num_dashes, st->dash.offset);
		vkvg_set_dash(cr, dashv.data(), st->dash.num_dashes, st->dash.offset);
	}
	auto mt = *t;
	auto xt = *t;
	for (size_t i = 0; i < p->count; i++, t++)
	{
		switch ((vtype_e1)t->type)
		{
		case vtype_e1::e_vmove:
		{
			if (i > 0)
			{
				if (xt.x == mt.x && xt.y == mt.y)
					vkvg_close_path(cr);
			}
			mt = *t;
			vkvg_move_to(cr, t->x, t->y);
		}break;
		case vtype_e1::e_vline:
		{
			vkvg_line_to(cr, t->x, t->y);
		}break;
		case vtype_e1::e_vcurve:
		{
			static double dv = 2.0 / 3.0;
			auto p0 = glm::vec2(xt.x, xt.y);
			auto p1 = glm::vec2(t->cx, t->cy);
			auto p2 = glm::vec2(t->x, t->y);
			glm::vec2 c1, c2;
			c1 = p1 - p0; c1 *= dv; c1 += p0;
			c2 = p1 - p2; c2 *= dv; c2 += p2;
			//	C0 = Q0
			//	C1 = Q0 + (2 / 3) (Q1 - Q0)
			//	C2 = Q2 + (2 / 3) (Q1 - Q2)
			//	C3 = Q2
			vkvg_curve_to(cr, c1.x, c1.y, c2.x, c2.y, t->x, t->y);
		}break;
		case vtype_e1::e_vcubic:
		{
			vkvg_curve_to(cr, t->cx, t->cy, t->cx1, t->cy1, t->x, t->y);

		}break;
		default:
			break;
		}
		xt = *t;
	}
	if (p->count > 2)
	{
		if (xt.x == mt.x && xt.y == mt.y)
			vkvg_close_path(cr);
	}

	if (st->fill_color)
	{
		vkvg_set_source_color(cr, st->fill_color);
		if (stroke)
			vkvg_fill_preserve(cr);
		else
			vkvg_fill(cr);
	}
	if (stroke)
	{
		vkvg_set_source_color(cr, st->stroke_color);
		vkvg_stroke(cr);
	}
	vkvg_restore(cr);
}

void draw_path_vkvg(void* cr, path_v* p, style_path_t* st);
struct vertex_tf0
{
	// 24字节
	float x, y, cx, cy, cx1, cy1;
	// 4字节
	uint32_t type;
};
struct path_txf_vg
{
	float x, y;	// 坐标
	int count;		// 数量
	//path_v::vertex_tf* v;
	vertex_tf0* v;	// 端点 
};

void draw_path_vkvg(void* cr, path_v* p, style_path_t* st)
{
#if 0
	if (!p || !st || p->_data.empty())return;
	path_txf_vg tf = {};
	tf.count = p->_data.size();
	tf.v = (path_v::vertex_tf*)p->data();
	tf.x = p->_pos.x;
	tf.y = p->_pos.y + p->_baseline;
	draw_path_vkvg((VkvgContext)cr, &tf, st);
#endif
}

vkvg_ctx::vkvg_ctx(VkvgSurface surf)
{
	if (surf) {
		_surf = surf;
		ctx = vkvg_create(surf);
	}
}

vkvg_ctx::~vkvg_ctx()
{
	vkvg_destroy(ctx); ctx = 0;
}


void vkvg_ctx::transform(const glm::mat3* matrix)
{
	glm::mat3x2 m = *matrix;
	fun->vkvg_transform(ctx, (vkvg_matrix_t*)&m);
}
void vkvg_ctx::transform(const glm::mat3x2* matrix)
{
	fun->vkvg_transform(ctx, (vkvg_matrix_t*)matrix);
}

void vkvg_ctx::set_matrix(const glm::mat3* matrix)
{
	glm::mat3x2 m = *matrix;
	fun->vkvg_set_matrix(ctx, (vkvg_matrix_t*)&m);
}

void vkvg_ctx::set_matrix(const glm::mat3x2* matrix)
{
	fun->vkvg_set_matrix(ctx, (vkvg_matrix_t*)matrix);
}
glm::mat3x2 vkvg_ctx::get_matrix()
{
	vkvg_matrix_t mx = {};
	fun->vkvg_get_matrix(ctx, &mx);
	return *(glm::mat3x2*)&mx;
}

vkvg_dev::vkvg_dev()
{
}

vkvg_dev::~vkvg_dev()
{
	if (fun)delete fun; fun = 0;
	if (dev)
	{
		vkvg_device_destroy(dev); dev = 0;
	}
}
vkvg_func_t* vkvg_dev::get_fun()
{
	if (!fun)
	{
		fun = new vkvg_func_t();
#if 1
#define funcset(n) fun->##n=n
		funcset(vkvg_create);
		funcset(vkvg_destroy);
		funcset(vkvg_status);
		funcset(vkvg_status_to_string);
		funcset(vkvg_reference);
		funcset(vkvg_get_reference_count);
		funcset(vkvg_flush);
		funcset(vkvg_new_path);
		funcset(vkvg_close_path);
		funcset(vkvg_new_sub_path);
		funcset(vkvg_path_extents);
		funcset(vkvg_get_current_point);
		funcset(vkvg_line_to);
		funcset(vkvg_rel_line_to);
		funcset(vkvg_move_to);
		funcset(vkvg_rel_move_to);
		funcset(vkvg_arc);
		funcset(vkvg_arc_negative);
		funcset(vkvg_curve_to);
		funcset(vkvg_rel_curve_to);
		funcset(vkvg_quadratic_to);
		funcset(vkvg_rel_quadratic_to);
		funcset(vkvg_rectangle);
		funcset(vkvg_rounded_rectangle);
		funcset(vkvg_rounded_rectangle2);
		funcset(vkvg_ellipse);
		funcset(vkvg_elliptic_arc_to);
		funcset(vkvg_rel_elliptic_arc_to);
		funcset(vkvg_stroke);
		funcset(vkvg_stroke_preserve);
		funcset(vkvg_fill);
		funcset(vkvg_fill_preserve);
		funcset(vkvg_paint);
		funcset(vkvg_clear);
		funcset(vkvg_reset_clip);
		funcset(vkvg_clip);
		funcset(vkvg_clip_preserve);
		funcset(vkvg_set_opacity);
		funcset(vkvg_get_opacity);
		funcset(vkvg_set_source_color);
		funcset(vkvg_set_source_rgba);
		funcset(vkvg_set_source_rgb);
		funcset(vkvg_set_line_width);
		funcset(vkvg_set_miter_limit);
		//funcset(vkvg_get_miter_limit);
		funcset(vkvg_set_line_cap);
		funcset(vkvg_set_line_join);
		funcset(vkvg_set_source_surface);
		funcset(vkvg_set_source);
		funcset(vkvg_set_operator);
		funcset(vkvg_set_fill_rule);
		funcset(vkvg_set_dash);
		funcset(vkvg_get_dash);
		funcset(vkvg_get_line_width);
		funcset(vkvg_get_line_cap);
		funcset(vkvg_get_line_join);
		funcset(vkvg_get_operator);
		funcset(vkvg_get_fill_rule);
		funcset(vkvg_get_source);
		funcset(vkvg_get_target);
		funcset(vkvg_has_current_point);
		funcset(vkvg_save);
		funcset(vkvg_restore);
		funcset(vkvg_translate);
		funcset(vkvg_scale);
		funcset(vkvg_rotate);
		funcset(vkvg_transform);
		funcset(vkvg_set_matrix);
		funcset(vkvg_get_matrix);
		funcset(vkvg_identity_matrix);
#undef funcset
#endif
	}
	return fun;
}

VkvgSurface vkvg_dev::new_surface(int width, int height)
{
	return width > 0 && height > 0 ? vkvg_surface_create(dev, width, height) : nullptr;
}

VkvgSurface vkvg_dev::new_surface(uint32_t* data, int width, int height)
{
	return width > 0 && height > 0 && data ? vkvg_surface_create_from_bitmap(dev, (unsigned char*)data, width, height) : nullptr;
}

VkvgSurface vkvg_dev::new_surface(void* vkimg)
{
	return vkimg ? vkvg_surface_create_for_VkhImage(dev, vkimg) : nullptr;
}

VkvgSurface vkvg_dev::new_surface(const char* fn)
{
	return fn && *fn ? vkvg_surface_create_from_image(dev, fn) : nullptr;
}

void vkvg_dev::free_surface(VkvgSurface p)
{
	if (p)
	{
		vkvg_surface_destroy(p);
	}
}

vkvg_ctx* vkvg_dev::new_ctx(VkvgSurface p)
{
	auto r = p ? new vkvg_ctx(p) : nullptr;
	if (r)
	{
		r->fun = get_fun();
	}
	return r;
}

void vkvg_dev::free_ctx(vkvg_ctx* p)
{
	if (p)delete p;
}

void* vkvg_dev::get_vkimage(void* surface)
{
	return surface ? vkvg_surface_get_vk_image((VkvgSurface)surface) : nullptr;//获取vk image指针 
}

void vkvg_dev::image_save(void* surface, const char* fn)
{
	auto surf = (VkvgSurface)surface;
	if (surf && fn && *fn)
	{
		vkvg_surface_write_to_png(surf, fn);
	}
}

glm::ivec2 vkvg_dev::get_image_data(void* surface, std::vector<uint32_t>* opt)
{
	auto surf = (VkvgSurface)surface;
	glm::ivec2 rs = {};
	if (!surf)return rs;
	rs = { vkvg_surface_get_width(surf),vkvg_surface_get_height(surf) };
	if (opt)
	{
		std::vector<uint32_t>& buf = *opt;
		buf.resize(rs.x * rs.y);
		vkvg_surface_write_to_memory(surf, (unsigned char*)buf.data());
	}
	return rs;
}

vkvg_dev* new_vkvgdev(dev_info_c* c, int sc)
{
	vkvg_dev* p = 0;
	static std::vector<VkSampleCountFlags> sv = { VK_SAMPLE_COUNT_64_BIT  ,	VK_SAMPLE_COUNT_32_BIT ,VK_SAMPLE_COUNT_16_BIT
		, VK_SAMPLE_COUNT_8_BIT ,VK_SAMPLE_COUNT_4_BIT,VK_SAMPLE_COUNT_2_BIT,VK_SAMPLE_COUNT_1_BIT };
	VkvgDevice dev = {};
	bool hasdev = (c && c->inst && c->phy && c->vkdev);
	dev_info_c _c = {};
	if (!hasdev)c = &_c;
	VkSampleCountFlags cus = VK_SAMPLE_COUNT_1_BIT;
	int i = 0;
	for (i = 0; i < sv.size(); i++)
	{
		if (sc == sv[i])
		{
			break;
		}
	}
	for (; i < sv.size(); i++)
	{
		auto it = sv[i];
		vkvg_device_create_info_t info = { it, true ,c->inst, c->phy, c->vkdev, c->qFamIdx,c->qIndex,false };
		dev = vkvg_device_create(&info);
		if (dev)
		{
			cus = it;
			break;
		}
	}

	if (dev)
	{
		p = new vkvg_dev();
		p->dev = dev;
		p->samplecount = cus;
	}
	return p;
}
void free_vkvgdev(vkvg_dev* p)
{
	if (p)delete p;
}

#if 0
void vkvgtest(form_x* pw) {
	auto c = pw->get_dev();//64,32,16,8,4,2,1
	auto vdev = new_vkvgdev((dev_info_c*)&c, 8);
	if (1)
	{
		do {
			//auto tex = pw->new_texture(ws.x, ws.y, 1, 0, 0);
			auto ws = pw->get_size();
			auto sur = vdev->new_surface(ws.x, ws.y);
			//auto sur = vdev->new_surface(pw->get_texture_vk(tex));
			if (!sur)break;
			auto vx = vdev->new_ctx(sur);
			auto ctx = vx->ctx;
			auto kc = vx->fun;
			kc->vkvg_clear(ctx);
			kc->vkvg_save(ctx);
			kc->vkvg_set_fill_rule(ctx, VKVG_FILL_RULE_EVEN_ODD);
			kc->vkvg_move_to(ctx, 10.0, 10.0);
			kc->vkvg_line_to(ctx, 150.0, 120.0);
			kc->vkvg_line_to(ctx, 120.0, 220.0);
			kc->vkvg_close_path(ctx);
			kc->vkvg_set_line_width(ctx, 6.0);
			kc->vkvg_set_line_cap(ctx, vkvg_line_cap_t::VKVG_LINE_CAP_ROUND);
			kc->vkvg_set_line_join(ctx, vkvg_line_join_t::VKVG_LINE_JOIN_ROUND);
			kc->vkvg_set_source_color(ctx, 0xff0080ff);
			kc->vkvg_stroke(ctx);
			kc->vkvg_restore(ctx);
			vkvg_surface_resolve(sur);//msaa采样转换输出
			auto fn = "offscreen.png";
			vkvg_surface_write_to_png(sur, fn);
			//std::vector<uint32_t> pxd;
			vdev->free_ctx(vx);
			//auto sss = vdev->get_image_data(sur, &pxd);
			auto vkimg = vdev->get_vkimage(sur);
			if (!vkimg)break;
			auto tex = pw->new_texture(ws.x, ws.y, vkimg, 1);//vkvg的image是bgra
			if (tex)
			{
				//pw->update_texture(tex, pxd.data(), {}, 0);
				pw->set_texture_blend(tex, 0);
				pw->push_texture(tex, {}, { 300,300,ws.x, ws.y }, 10);//提交SDL渲染
			}

		} while (0);
	}
}
#endif
#if 1
void test_vkvg(const char* fn, dev_info_c* dc)
{
	vkvg_dev* vctx = new_vkvgdev(dc, 8);
	auto dev = vctx->dev;
	if (!dev)return;
	VkvgSurface surf = vkvg_surface_create(dev, 1024, 1024);
	VkvgContext ctx = vkvg_create(surf);
	vkvg_clear(ctx);
	//vkvg_save(ctx);
	if (1) {
		print_time ptt("vkvg");
		VkvgPattern pat;
		VkvgContext  cr = ctx;
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
		vkvg_move_to(cr, 20, 100);
		vkvg_set_source_color(cr, 0xff0cC616);
		vkvg_select_font_face(ctx, (char*)u8"微软雅黑");
		vkvg_set_font_size(cr, 26);
		vkvg_show_text(cr, (char*)u8"abc123\ng0加0123");
	}

	vkvg_flush(ctx);
	//vkvg_restore(ctx);
	if (!fn || !*fn)
		fn = "temp/offscreen_vkvg.png";
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
}
#endif