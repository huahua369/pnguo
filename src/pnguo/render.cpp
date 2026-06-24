/*

vkvg设备需要扩展scalarBlockLayout：VkPhysicalDeviceVulkan12Features或VkPhysicalDeviceScalarBlockLayoutFeaturesEXT
*/
#include <pch1.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <complex.h>
#ifdef __cplusplus
}
#endif
// 实现
//#define GLM_ENABLE_EXPERIMENTAL 
//#include <glm/glm.hpp>
//#include <glm/gtx/vector_angle.hpp>
//#include <glm/gtx/closest_point.hpp>
//#include <glm/gtc/type_ptr.hpp>

#include <pnguo.h>

#include <zlib.h>

#define FLEX_IMPLEMENTATION
#include <vg.h>

#include <vkvg_cx.h>

#include <stb_rect_pack.h> 
#include <print_time.h> 
#include <vkh.h>
#include "shaders/base3d.vert.h"
#include "shaders/base3d.frag.h"
#include "shaders/base3d2.vert.h"
#include "shaders/base3d2.frag.h"

#include "tinysdl3.h"

#include "render.h"
#include "font_core.h"

#include "ecc_sv.h"
#include "mapView.h"

#include "ntype.h"

#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

#define SP_RGBA32_R_U(v) ((v) & 0xff)
#define SP_RGBA32_G_U(v) (((v) >> 8) & 0xff)
#define SP_RGBA32_B_U(v) (((v) >> 16) & 0xff)
#define SP_RGBA32_A_U(v) (((v) >> 24) & 0xff)
#define SP_COLOR_U_TO_F(v) ((v) / 255.0)
#define SP_COLOR_F_TO_U(v) ((uint32_t) ((v) * 255. + .5))
#define SP_RGBA32_R_F(v) SP_COLOR_U_TO_F (SP_RGBA32_R_U (v))
#define SP_RGBA32_G_F(v) SP_COLOR_U_TO_F (SP_RGBA32_G_U (v))
#define SP_RGBA32_B_F(v) SP_COLOR_U_TO_F (SP_RGBA32_B_U (v))
#define SP_RGBA32_A_F(v) SP_COLOR_U_TO_F (SP_RGBA32_A_U (v))

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


struct vkvgctx {
	VkvgDevice dev = 0;
	VkhDevice vkhdev = 0;
};

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
	ctx = new vkvgctx();
	ctx1 = new vkvgctx();
}
void free_gctx(vkvgctx* p) {
	if (p)
	{
		if (p->dev)
		{
			vkvg_device_destroy(p->dev); p->dev = 0;
		}
		if (p->vkhdev) {
			vkh_device_destroy(p->vkhdev); p->vkhdev = 0;
		}
		delete p; p = 0;
	}
}
vkvg_dev::~vkvg_dev()
{
	if (fun)delete fun; fun = 0;
	if (memoryProperties)delete memoryProperties; memoryProperties = 0;
	free_gctx(ctx);
	free_gctx(ctx1);

}
vkvg_func_t* vkvg_dev::get_fun()
{
	if (!fun)
	{
		fun = new vkvg_func_t();
#if 1
#define funcset(n) fun->##n=n

		funcset(vkvg_matrix_init_identity);
		funcset(vkvg_matrix_init);
		funcset(vkvg_matrix_init_translate);
		funcset(vkvg_matrix_init_scale);
		funcset(vkvg_matrix_init_rotate);
		funcset(vkvg_matrix_translate);
		funcset(vkvg_matrix_scale);
		funcset(vkvg_matrix_rotate);
		funcset(vkvg_matrix_multiply);
		funcset(vkvg_matrix_transform_distance);
		funcset(vkvg_matrix_transform_point);
		funcset(vkvg_matrix_invert);
		funcset(vkvg_matrix_get_scale);
		funcset(vkvg_device_set_context_cache_size);
		funcset(vkvg_device_create);
		funcset(vkvg_device_destroy);
		funcset(vkvg_device_status);
		funcset(vkvg_device_reference);
		funcset(vkvg_device_get_reference_count);
		funcset(vkvg_device_set_dpy);
		funcset(vkvg_device_get_dpy);
		funcset(vkvg_get_required_instance_extensions);
		funcset(vkvg_get_required_device_extensions);
		funcset(vkvg_get_device_requirements);
		funcset(vkvg_surface_create);
		funcset(vkvg_surface_create_from_image);
		funcset(vkvg_surface_create_for_VkhImage);
		funcset(vkvg_surface_create_from_bitmap);
		funcset(vkvg_surface_status);
		funcset(vkvg_surface_reference);
		funcset(vkvg_surface_get_reference_count);
		funcset(vkvg_surface_destroy);

		funcset(vkvg_surface_clear);
		funcset(vkvg_surface_get_vk_image);
		funcset(vkvg_surface_get_vk_format);
		funcset(vkvg_surface_get_width);
		funcset(vkvg_surface_get_height);
		funcset(vkvg_surface_write_to_png);
		funcset(vkvg_surface_write_to_memory);
		funcset(vkvg_surface_resolve);

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
		funcset(vkvg_get_miter_limit);
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

		funcset(vkvg_select_font_face);
		funcset(vkvg_load_font_from_path);
		funcset(vkvg_load_font_from_memory);
		funcset(vkvg_set_font_size);
		funcset(vkvg_show_text);
		funcset(vkvg_text_extents);
		funcset(vkvg_font_extents);
		funcset(vkvg_text_run_create);
		funcset(vkvg_text_run_create_with_length);
		funcset(vkvg_text_run_destroy);
		funcset(vkvg_show_text_run);
		funcset(vkvg_text_run_get_extents);
		funcset(vkvg_text_run_get_glyph_count);
		funcset(vkvg_text_run_get_glyph_position);
		funcset(vkvg_pattern_status);
		funcset(vkvg_pattern_reference);
		funcset(vkvg_pattern_get_reference_count);
		funcset(vkvg_pattern_create_for_surface);
		funcset(vkvg_pattern_create_linear);
		funcset(vkvg_pattern_edit_linear);
		funcset(vkvg_pattern_get_linear_points);
		funcset(vkvg_pattern_create_radial);
		funcset(vkvg_pattern_edit_radial);
		funcset(vkvg_pattern_get_color_stop_count);
		funcset(vkvg_pattern_get_color_stop_rgba);
		funcset(vkvg_pattern_destroy);
		funcset(vkvg_pattern_add_color_stop);
		funcset(vkvg_pattern_set_extend);
		funcset(vkvg_pattern_set_filter);
		funcset(vkvg_pattern_get_extend);
		funcset(vkvg_pattern_get_filter);
		funcset(vkvg_pattern_get_type);
		funcset(vkvg_pattern_set_matrix);
		funcset(vkvg_pattern_get_matrix);
		//funcset(vkvg_set_source_color_name);
		funcset(vkvg_start_recording);
		funcset(vkvg_stop_recording);
		funcset(vkvg_replay);
		funcset(vkvg_replay_command);
		funcset(vkvg_recording_get_command);
		funcset(vkvg_recording_get_count);
		funcset(vkvg_recording_get_data);
		funcset(vkvg_recording_destroy);

#undef funcset
#endif
	}
	return fun;
}

VkvgSurface vkvg_dev::new_surface(int width, int height)
{
	return width > 0 && height > 0 ? vkvg_surface_create(ctx->dev, width, height) : nullptr;
}

VkvgSurface vkvg_dev::new_surface(uint32_t* data, int width, int height)
{
	return width > 0 && height > 0 && data ? vkvg_surface_create_from_bitmap(ctx->dev, (unsigned char*)data, width, height) : nullptr;
}

//VkvgSurface vkvg_dev::new_surface(void* vkimg, int type, int width, int height)
//{
//	if (!vkimg || width < 2 || height < 2 || !ctx || !ctx->vkhdev)
//	{
//		return nullptr;
//	}
//	VkhImage p = vkh_image_import(ctx->vkhdev, (VkImage)vkimg, type ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM, width, height);
//	return  vkvg_surface_create_for_VkhImage(ctx->dev, p);
//}

VkvgSurface vkvg_dev::new_surface(const char* fn)
{
	return fn && *fn ? vkvg_surface_create_from_image(ctx->dev, fn) : nullptr;
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

vkvg_ctx* vkvg_dev::new_context(VkvgSurface p)
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

void vkvg_dev::wait_dev()
{
	if (vkdev)
		vkDeviceWaitIdle(vkdev);
}


vkvg_dev* new_vkvgdev(dev_info_c* c, int sc)
{
	vkvg_dev* p = 0;
	static std::vector<VkSampleCountFlags> sv = { VK_SAMPLE_COUNT_64_BIT  ,	VK_SAMPLE_COUNT_32_BIT ,VK_SAMPLE_COUNT_16_BIT
		, VK_SAMPLE_COUNT_8_BIT ,VK_SAMPLE_COUNT_4_BIT,VK_SAMPLE_COUNT_2_BIT,VK_SAMPLE_COUNT_1_BIT };
	VkvgDevice dev = {};
	VkvgDevice dev1 = {};
	bool hasdev = (c && (c->inst && c->phy && c->vkdev));
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
	VkDevice vkdev = c->vkdev;
	for (; i < sv.size(); i++)
	{
		auto it = sv[i];
		vkvg_device_create_info_t info = { it, true ,c->inst, c->phy, c->vkdev, c->qFamIdx,c->qIndex,false };
		dev = vkvg_device_create(&info);

		if (dev)
		{
			vkdev = c->vkdev;
			cus = it;
			break;
		}
	}
	if (dev)
	{
		p = new vkvg_dev();
		if (cus != VK_SAMPLE_COUNT_1_BIT)
		{
			vkvg_device_create_info_t info = { VK_SAMPLE_COUNT_1_BIT, true ,c->inst, c->phy, c->vkdev, c->qFamIdx,c->qIndex,false };
			dev1 = vkvg_device_create(&info);
			c->phy = info.phy;
		}
		VkPhysicalDeviceMemoryProperties m_memoryProperties = {};
		vkGetPhysicalDeviceMemoryProperties(c->phy, &m_memoryProperties);
		p->memoryProperties = new VkPhysicalDeviceMemoryProperties(m_memoryProperties);
		p->ctx->dev = dev;
		p->vkdev = vkdev;
		p->ctx1->dev = dev1;
		p->samplecount = cus;
		//p->ctx->vkhdev = vkh_device_import(c->inst, c->phy, c->vkdev);
	}
	return p;
}
void free_vkvgdev(vkvg_dev* p)
{
	if (p)delete p;
}

vkvg_dev* new_vgdev_cx(dev_info_cx* d, int sample)
{
	vkvg_dev* ret = 0;
	dev_info_c cc = {};
	if (d)
	{
		cc.inst = (VkInstance)d->inst; cc.phy = (VkPhysicalDevice)d->phy; cc.vkdev = (VkDevice)d->vkdev;
		cc.qFamIdx = d->qFamIdx;
		if (d->qCount > 2)
			cc.qIndex = 2;
	}
	auto p = new_vkvgdev(&cc, sample);
	//if (_vgdev && p)
	//{
	//	free_vkvgdev(_vgdev); _vgdev = nullptr;
	//}
	if (p)
	{
		p->qindex = cc.qIndex;
		ret = p;
		p->get_fun();
	}
	return ret;
}

void free_surface(void* c)
{
	if (c) {
		vkvg_surface_destroy((VkvgSurface)c);
	}
}

void* ctx_begin(void* surface)
{
	if (!surface)return nullptr;
	auto c = vkvg_create((VkvgSurface)surface);
	return c;
}

void ctx_end(void* ctx)
{
	if (ctx)
	{
		vkvg_destroy((VkvgContext)ctx);
	}
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

const float arrow_size = 10;
const float arrow_hwidth = 4;
const float point_label_delta = -10;

glm::vec2 avec2_perp(glm::vec2 a) { return  glm::vec2(a.y, -a.x); }

void draw_arrow_to(VkvgContext ctx, float x, float y)
{
	glm::vec2 p0;
	glm::vec2 p1 = { x, y };
	vkvg_get_current_point(ctx, &p0.x, &p0.y);

	glm::vec2 dir = (p0 - p1);
	glm::vec2 n = glm::normalize(dir);
	glm::vec2 perp = avec2_perp(n) * arrow_hwidth;

	vkvg_line_to(ctx, x, y);
	vkvg_stroke(ctx);
	vkvg_move_to(ctx, x, y);
	glm::vec2 p = (p1 + (n * arrow_size));
	glm::vec2 a = (p + perp);
	vkvg_line_to(ctx, a.x, a.y);
	a = (p + (perp * -1));
	vkvg_line_to(ctx, a.x, a.y);
	vkvg_close_path(ctx);
	vkvg_fill(ctx);
	vkvg_move_to(ctx, x, y);
}

void draw_arrow(VkvgContext ctx, const glm::vec2& p0, const glm::vec2& p1, float arrow_hwidth, float arrow_size, bool type)
{
	glm::vec2 dir = (p0 - p1);
	glm::vec2 n = glm::normalize(dir);
	glm::vec2 perp = avec2_perp(n) * arrow_hwidth;
	auto dir1 = p1 - p0;
	float len = glm::length(dir1);
	vkvg_move_to(ctx, p0.x, p0.y);
	float e = arrow_size * 0.5;
	if (len > e && type == 0) {
		dir1 = glm::normalize(dir1);
		glm::vec2 e2 = { e, arrow_hwidth };
		float newLength = len - e;
		glm::vec2 p1n = p0 + dir1 * newLength;
		vkvg_line_to(ctx, p1n.x, p1n.y);
	}
	else {
		vkvg_line_to(ctx, p1.x, p1.y);
	}
	vkvg_stroke(ctx);
	vkvg_move_to(ctx, p1.x, p1.y);
	glm::vec2 p = (p1 + (n * arrow_size));
	glm::vec2 a = (p + perp);
	vkvg_line_to(ctx, a.x, a.y);
	a = (p + (perp * -1));
	vkvg_line_to(ctx, a.x, a.y);
	vkvg_close_path(ctx);
	vkvg_fill(ctx);
}

#endif
// __has_include(<vg.h>)


void submit_style(VkvgContext cr, fill_style_d* st) {
	bool stroke = st && st->thickness > 0 && st->color > 0;
	if (!cr || !st)return;
	if (st->fill)
	{
		vkvg_set_source_color(cr, st->fill);
		if (stroke)
			vkvg_fill_preserve(cr);
		else
			vkvg_fill(cr);
	}
	if (stroke)
	{
		vkvg_set_line_width(cr, st->thickness);
		if (st->cap > 0)
			vkvg_set_line_cap(cr, (vkvg_line_cap_t)st->cap);
		if (st->join > 0)
			vkvg_set_line_join(cr, (vkvg_line_join_t)st->join);
		if (st->dash.v > 0 || st->dash_p)
		{
			float dashes[64] = {};
			uint64_t x = 1;
			auto t = dashes;
			int num_dashes = st->dash_num;
			if (num_dashes > 64)num_dashes = 64;
			if (st->dash_p)
			{
				vkvg_set_dash(cr, st->dash_p, st->dash_num, st->dash_offset);
			}
			else
			{
				if (num_dashes > 8)num_dashes = 8;
				for (size_t i = 0; i < num_dashes; i++)
				{
					*t = st->dash.v8[i]; t++;
				}
				if (num_dashes > 0)
					vkvg_set_dash(cr, dashes, num_dashes, st->dash_offset);
			}
		}
		vkvg_set_source_color(cr, st->color);
		vkvg_stroke(cr);
	}

}

inline float get_radius(float& radius, float width, float height) {
	if (radius > 0)
	{
		if ((radius > width / 2.0f) || (radius > height / 2.0f))
			radius = fmin(width / 2.0f, width / 2.0f);
	}
	return radius;
}
#ifndef M_PI
auto M_PI = glm::pi<double>();
#endif

// r，左右下左
void draw_rounded_rectangle(VkvgContext ctx, double x, double y, double width, double height, const glm::vec4& r1)
{
	if (width <= 0 || height <= 0)
		return;
	auto r = r1;
	get_radius(r.x, width, height);
	get_radius(r.y, width, height);
	get_radius(r.z, width, height);
	get_radius(r.w, width, height);
	auto w = width;
	auto h = height;
	vkvg_move_to(ctx, x + r.x, y);
	vkvg_line_to(ctx, x + w - r.y, y);// 上
	if (r.y > 0.0f)
		vkvg_elliptic_arc_to(ctx, x + w, y + r.y, false, true, r.y, r.y, 0);
	vkvg_line_to(ctx, x + w, y + h - r.z);// 右
	if (r.z > 0.0f)
		vkvg_elliptic_arc_to(ctx, x + w - r.z, y + h, false, true, r.z, r.z, 0);
	vkvg_line_to(ctx, x + r.w, y + h);// 下
	if (r.w > 0.0f)
		vkvg_elliptic_arc_to(ctx, x, y + h - r.w, false, true, r.w, r.w, 0);
	vkvg_line_to(ctx, x, y + r.x);// 左
	if (r.x > 0.0f)
		vkvg_elliptic_arc_to(ctx, x + r.x, y, false, true, r.x, r.x, 0);
	vkvg_close_path(ctx);
}
void draw_triangle(VkvgContext cr, const glm::vec2& pos, const glm::vec2& size, const glm::vec2& dirspos)
{
	glm::vec2 tpos[3] = {};
	float df = dirspos.y;
	switch ((int)dirspos.x)
	{
	case 0:
	{
		tpos[0] = { size.x * df, 0 };
		tpos[1] = { size.x, size.y };
		tpos[2] = { 0, size.y };
	}
	break;
	case 1:
	{
		tpos[0] = { size.x, size.y * df };
		tpos[1] = { 0, size.y };
		tpos[2] = { 0, 0 };
	}
	break;
	case 2:
	{
		tpos[0] = { size.x * df, size.y };
		tpos[1] = { 0, 0 };
		tpos[2] = { size.x, 0 };
	}
	break;
	case 3:
	{
		tpos[0] = { 0, size.y * df };
		tpos[1] = { size.x, 0 };
		tpos[2] = { size.x, size.y };
	}
	break;
	default:
		break;
	}
	vkvg_move_to(cr, pos.x + tpos[0].x, pos.y + tpos[0].y);
	vkvg_line_to(cr, pos.x + tpos[1].x, pos.y + tpos[1].y);
	vkvg_line_to(cr, pos.x + tpos[2].x, pos.y + tpos[2].y);
	vkvg_close_path(cr);
}

void draw_grid_fill(VkvgContext cr, const glm::vec2& ss, const glm::ivec2& cols, int width)
{
	int x = fmod(ss.x, width);
	int y = fmod(ss.y, width);
	int xn = ss.x / width;
	int yn = ss.y / width;
	if (x > 0)xn++;
	if (y > 0)yn++;
	vkvg_save(cr);
	vkvg_rectangle(cr, 0, 0, ss.x, ss.y);
	vkvg_clip(cr);
	for (size_t i = 0; i < yn; i++)
	{
		for (size_t j = 0; j < xn; j++)
		{
			bool k = (j & 1);
			if (!(i & 1))
				k = !k;
			auto c = cols[k];
			vkvg_rectangle(cr, j * width, i * width, width, width);
			vkvg_set_source_color(cr, c);
			vkvg_fill(cr);
		}
	}
	vkvg_restore(cr);
}
void draw_linear(VkvgContext cr, const glm::vec2& ss, const glm::vec4* cols, int count)
{
	VkvgPattern gr = vkvg_pattern_create_linear(0, 0, ss.x, ss.y);
	double n = count - 1;
	for (size_t i = 0; i < count; i++)
	{
		auto color = cols[i];
		vkvg_pattern_add_color_stop(gr, i / n, color.x, color.y, color.z, color.w);
	}
	vkvg_rectangle(cr, 0, 0, ss.x, ss.y);
	vkvg_set_source(cr, gr);
	vkvg_fill(cr);
	vkvg_pattern_destroy(gr);
}

#define getrgba(a,b,sp) lerp(a.sp,b.sp,ratio)

glm::dvec4 mix_colors0(glm::vec4 a, glm::vec4 b, float ratio)
{
	auto lerp = [](double v0, double v1, double t) { return (1.0 - t) * v0 + t * v1; };
	glm::dvec4 result = {};
	result.x = getrgba(a, b, x);
	result.y = getrgba(a, b, y);
	result.z = getrgba(a, b, z);
	result.w = getrgba(a, b, w);
	return result;
}
VkvgPattern new_cubic_gradient(
	glm::vec4 rect,
	glm::vec4 from,
	glm::vec4 to,
	glm::vec2 ctrl1,
	glm::vec2 ctrl2,
	glm::vec2 p0 = {},
	glm::vec2 p1 = { 1,1 },
	int steps = 8
);
VkvgPattern new_cubic_gradient(
	glm::vec4 rect,
	glm::vec4 from,
	glm::vec4 to,
	glm::vec2 ctrl1,
	glm::vec2 ctrl2,
	glm::vec2 p0,
	glm::vec2 p1,
	int steps
) {
	// validate input points
	for (auto&& pt : { p0, ctrl1, ctrl2, p1 }) {
		if (pt.x < 0 || pt.x > 1 ||
			pt.y < 0 || pt.y > 1) {
			throw std::invalid_argument("Invalid points for cubic gradient; 0..1 coordinates expected.");
		}
	}
	if (steps < 2 || steps > 999) {
		throw std::invalid_argument("Invalid number of steps for cubic gradient; 2 to 999 steps expected.");
	}
	if (rect.x > rect.z)
	{
		std::swap(rect.x, rect.z);
	}
	if (rect.y > rect.w)
	{
		std::swap(rect.y, rect.w);
	}
	VkvgPattern g = vkvg_pattern_create_linear(rect.x, rect.y, rect.z, rect.w);
	//VkvgPattern vkvg_pattern_create_radial(double cx0, double cy0, double radius0, double cx1, double cy1, double radius1);
	--steps;
	for (int step = 0; step <= steps; ++step) {
		auto t = 1.0 * step / steps;
		auto s = 1.0 - t;
		auto p0t = p0;
		auto p1t = p1;
		auto c1 = ctrl1;
		auto c2 = ctrl2;
		p0t *= (t * t * t);
		c1 *= (3 * t * t * s);
		c2 *= (3 * t * s * s);
		p1t *= (s * s * s);
		//auto p = (t * t * t) * p0 + (3 * t * t * s) * ctrl1 + (3 * t * s * s) * ctrl2 + (s * s * s) * p1;
		auto p = p0t + c1 + c2 + p1t;
		auto offset = p.x;
		auto ratio = p.y;
		auto color = mix_colors0(from, to, ratio);
		vkvg_pattern_add_color_stop(g, offset, color.x, color.y, color.z, color.w);
	}

	return g;
}
void paint_shadow_v(VkvgContext cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, bool rev, float r)
{
	auto trans = shadow;
	trans.w = 0;
	auto gr = rev ?
		new_cubic_gradient(glm::vec4(0, 0, size_x, size_y), trans, shadow, glm::vec2(0.5, 0.0), glm::vec2(1.0, 0.5))
		: new_cubic_gradient(glm::vec4(0, 0, size_x, size_y), shadow, trans, glm::vec2(0, 0.5), glm::vec2(0.5, 1));
	if (r > 0)
	{
		vkvg_rounded_rectangle(cr, 0, 0, width, height, r);
	}
	else
	{
		vkvg_rectangle(cr, 0, 0, width, height);
	}
	vkvg_set_source(cr, gr);
	vkvg_fill(cr);
	vkvg_pattern_destroy(gr);
}
void paint_shadow_v(VkvgContext cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, const glm::vec4& color_to, bool rev, float r)
{
	auto gr = rev ?
		new_cubic_gradient(glm::vec4(0, 0, size_x, size_y), color_to, shadow, glm::vec2(0.5, 0.0), glm::vec2(1.0, 0.5))
		: new_cubic_gradient(glm::vec4(0, 0, size_x, size_y), shadow, color_to, glm::vec2(0, 0.5), glm::vec2(0.5, 1));
	if (r > 0)
	{
		vkvg_rounded_rectangle(cr, 0, 0, width, height, r);
	}
	else
	{
		vkvg_rectangle(cr, 0, 0, width, height);
	}
	vkvg_set_source(cr, gr);
	vkvg_fill(cr);
	vkvg_pattern_destroy(gr);
}

void vgc_draw_cmds(void* ctx, uint8_t* cmds, size_t count, void* data, size_t size, vg_style_data* style)
{
	if (!ctx || !cmds || count == 0 || !data || !size || !style)return;
	auto cr = (VkvgContext)ctx;
	auto pd = (uint8_t*)data;
	for (size_t i = 0; i < count; i++)
	{
		auto t = (VG_DRAW_CMD)cmds[i];
		switch (t)
		{
		case VG_DRAW_CMD::VG_CMD_CLEAR:
			vkvg_clear(cr);
			break;
		case VG_DRAW_CMD::VG_CMD_SAVE:
			vkvg_save(cr);
			break;
		case VG_DRAW_CMD::VG_CMD_RESTORE:
			vkvg_restore(cr);
			break;
		case VG_DRAW_CMD::VG_CMD_CLIP_RECT:
		{
			auto p = (clip_rect_d*)pd;
			vkvg_rectangle(cr, p->pos.x, p->pos.y, p->size.x, p->size.y);
			vkvg_clip(cr);
			pd += sizeof(clip_rect_d);
		}
		break;
		case VG_DRAW_CMD::VG_CMD_RECT:
		{
			auto p = (rect_d*)pd;
			if (p->r > 0)
			{
				vkvg_rounded_rectangle(cr, p->pos.x, p->pos.y, p->size.x, p->size.y, p->r);
			}
			else {
				vkvg_rectangle(cr, p->pos.x, p->pos.y, p->size.x, p->size.y);
			}
			if (style->fs && p->st >= 0 && p->st < style->fs_count)
				submit_style(cr, style->fs + p->st);
			pd += sizeof(rect_d);
		}
		break;
		case VG_DRAW_CMD::VG_CMD_RECT4R:
		{
			auto p = (rect4r_d*)pd;
			draw_rounded_rectangle(cr, p->pos.x, p->pos.y, p->size.x, p->size.y, p->r);
			if (style->fs && p->st >= 0 && p->st < style->fs_count)
				submit_style(cr, style->fs + p->st);
			pd += sizeof(rect4r_d);
		}
		break;
		case VG_DRAW_CMD::VG_CMD_CIRCLE:
		{
			auto p = (circle_d*)pd;
			vkvg_ellipse(cr, p->radius, p->radius, p->pos.x, p->pos.y, 0);
			if (style->fs && p->st >= 0 && p->st < style->fs_count)
				submit_style(cr, style->fs + p->st);
			pd += sizeof(circle_d);
		}
		break;
		case VG_DRAW_CMD::VG_CMD_ELLIPSE:
		{
			auto p = (ellipse_d*)pd;
			vkvg_ellipse(cr, p->radius.x, p->radius.y, p->pos.x, p->pos.y, p->rotationAngle);
			if (style->fs && p->st >= 0 && p->st < style->fs_count)
				submit_style(cr, style->fs + p->st);
			pd += sizeof(ellipse_d);
		}
		break;
		case VG_DRAW_CMD::VG_CMD_TRIANGLE:
		{
			auto p = (triangle_d*)pd;
			draw_triangle(cr, p->pos, p->size, p->spos);
			if (style->fs && p->st >= 0 && p->st < style->fs_count)
				submit_style(cr, style->fs + p->st);
			pd += sizeof(triangle_d);
		}
		break;
		case VG_DRAW_CMD::VG_CMD_FILL_GRID:
		{
			auto p = (grid_fill_d*)pd;
			draw_grid_fill(cr, p->size, p->cols, p->width);
			pd += sizeof(grid_fill_d);
		}
		break;
		case VG_DRAW_CMD::VG_CMD_FILL_LINEAR:
		{
			auto p = (linear_fill_d*)pd;
			draw_linear(cr, p->size, p->cols, p->count);
			pd += sizeof(linear_fill_d);
		}
		break;
		case VG_DRAW_CMD::VG_CMD_STROKE_LINE:
		{
			auto p = (line_d*)pd;
			vkvg_move_to(cr, p->p1.x, p->p1.y);
			vkvg_line_to(cr, p->p2.x, p->p2.y);
			if (style->fs && p->st >= 0 && p->st < style->fs_count)
				submit_style(cr, style->fs + p->st);
		}
		break;
		case VG_DRAW_CMD::VG_CMD_STROKE_POLYLINE:
		{
			auto p = (polyline_d*)pd;
			if (p->count > 0)
			{
				vkvg_move_to(cr, p->points[0].x, p->points[0].y);
				for (size_t j = 1; j < p->count; j++)
				{
					vkvg_line_to(cr, p->points[j].x, p->points[j].y);
				}
				if (p->closed)
					vkvg_close_path(cr);
				if (style->fs && p->st >= 0 && p->st < style->fs_count)
					submit_style(cr, style->fs + p->st);
			}
			pd += sizeof(polyline_d) + sizeof(glm::vec2) * (p->count - 1);
		}
		break;
		//case VG_DRAW_CMD::VG_CMD_TEXT:
		//{
		//	auto p = (text_d*)pd;
		//	if (style->ts && p->font_st >= 0 && p->font_st < style->ts_count)
		//	{
		//		auto& ts = style->ts[p->font_st];
		//	}
		//	pd += sizeof(text_d) + p->len + 1;
		//}
		//break;
		default:
			break;
		}
	}

}
void vgc_draw_path(void* ctx, path_d* path, fill_style_d* style)
{
	if (!ctx || !path || path->count == 0 || !path->v || !style)return;
	auto cr = (VkvgContext)ctx;
	auto t = path->v;
	bool stroke = false;
	vkvg_save(cr);
	vkvg_set_fill_rule(cr, VKVG_FILL_RULE_EVEN_ODD);
	glm::vec2 pos, scale = {};
	pos += path->pos;
	if (path->flip_y)
	{
		vkvg_matrix_t flip_y = {};
		vkvg_matrix_init(&flip_y, 1, 0, 0, -1, 0, 0); // 垂直翻转
		vkvg_set_matrix(cr, &flip_y);
		pos.y = -pos.y;
	}
	vkvg_translate(cr, pos.x, pos.y);
	if (path->scale > 0)
	{
		vkvg_scale(cr, path->scale, path->scale);
	}
	float scale_pos = 1.0;
	auto mt = *t;
	auto xt = *t;
	if (path->scale_pos > 0)
	{
		scale_pos = path->scale_pos;
		for (size_t i = 0; i < path->count; i++, t++)
		{
			switch ((vtype_e1)t->type)
			{
			case vtype_e1::e_vmove:
			{
				if (i > 0)
				{
					if (xt.p.x == mt.p.x && xt.p.y == mt.p.y)
						vkvg_close_path(cr);
				}
				mt = *t;
				vkvg_move_to(cr, t->p.x * scale_pos, t->p.y * scale_pos);
			}break;
			case vtype_e1::e_vline:
			{
				vkvg_line_to(cr, t->p.x * scale_pos, t->p.y * scale_pos);
			}break;
			case vtype_e1::e_vcurve:
			{
				vkvg_quadratic_to(cr, t->c.x * scale_pos, t->c.y * scale_pos, t->p.x * scale_pos, t->p.y * scale_pos);
			}break;
			case vtype_e1::e_vcubic:
			{
				vkvg_curve_to(cr, t->c.x * scale_pos, t->c.y * scale_pos, t->c1.x * scale_pos, t->c1.y * scale_pos, t->p.x * scale_pos, t->p.y * scale_pos);

			}break;
			default:
				break;
			}
			xt = *t;
		}
	}
	else
	{
		for (size_t i = 0; i < path->count; i++, t++)
		{
			switch ((vtype_e1)t->type)
			{
			case vtype_e1::e_vmove:
			{
				if (i > 0)
				{
					if (xt.p.x == mt.p.x && xt.p.y == mt.p.y)
						vkvg_close_path(cr);
				}
				mt = *t;
				vkvg_move_to(cr, t->p.x, t->p.y);
			}break;
			case vtype_e1::e_vline:
			{
				vkvg_line_to(cr, t->p.x, t->p.y);
			}break;
			case vtype_e1::e_vcurve:
			{
				//static double dv = 2.0 / 3.0;
				//auto p0 = glm::vec2(xt.p.x, xt.p.y);
				//auto p1 = glm::vec2(t->c.x, t->c.y);
				//auto p2 = glm::vec2(t->p.x, t->p.y);
				//glm::vec2 c1, c2;
				//c1 = p1 - p0; c1 *= dv; c1 += p0;
				//c2 = p1 - p2; c2 *= dv; c2 += p2;
				////	C0 = Q0
				////	C1 = Q0 + (2 / 3) (Q1 - Q0)
				////	C2 = Q2 + (2 / 3) (Q1 - Q2)
				////	C3 = Q2
				//vkvg_curve_to(cr, c1.x, c1.y, c2.x, c2.y, t->p.x, t->p.y);
				vkvg_quadratic_to(cr, t->c.x, t->c.y, t->p.x, t->p.y);
			}break;
			case vtype_e1::e_vcubic:
			{
				vkvg_curve_to(cr, t->c.x, t->c.y, t->c1.x, t->c1.y, t->p.x, t->p.y);

			}break;
			default:
				break;
			}
			xt = *t;
		}
	}
	if (path->count > 2)
	{
		if (xt.p.x == mt.p.x && xt.p.y == mt.p.y)
			vkvg_close_path(cr);
	}
	submit_style(cr, style);
	vkvg_restore(cr);
}

void vgc_draw_block(void* ctx, dblock_d* p, fill_style_d* style)
{
	if (!ctx || !p || !p->points || p->count == 0 || p->rc.x <= 0 || p->rc.y < 0 || !style)return;
	auto cr = (VkvgContext)ctx;
	vkvg_save(cr);
	vkvg_set_fill_rule(cr, VKVG_FILL_RULE_EVEN_ODD);
	if (p->scale_pos > 0)
	{
		auto sp = p->scale_pos;
		vkvg_translate(cr, p->pos.x * sp, p->pos.y * sp);
		vkvg_translate(cr, p->view_pos.x, p->view_pos.y);
		for (size_t i = 0; i < p->count; i++)
		{
			auto pt = p->points[i];
			if (pt.y > 0)
			{
				vkvg_rectangle(cr, pt.x * sp, pt.y * sp, p->rc.x * sp, p->rc.y * sp);
			}
			else {
				vkvg_ellipse(cr, p->rc.x * sp, p->rc.x * sp, pt.x * sp, pt.y * sp, 0);
			}
		}
	}
	else
	{
		vkvg_translate(cr, p->pos.x, p->pos.y);
		vkvg_translate(cr, p->view_pos.x, p->view_pos.y);
		for (size_t i = 0; i < p->count; i++)
		{
			auto pt = p->points[i];
			if (pt.y > 0)
			{
				vkvg_rectangle(cr, pt.x, pt.y, p->rc.x, p->rc.y);
			}
			else {
				vkvg_ellipse(cr, p->rc.x, p->rc.x, pt.x, pt.y, 0);
			}
		}
	}
	submit_style(cr, style);
	vkvg_restore(cr);
}
void vkvg_rect(VkvgContext ctx, const glm::vec4& rc) {
	vkvg_rectangle(ctx, rc.x, rc.y, rc.z, rc.w);
}
struct stops_t {
	float o;
	float x, y, z, w;
};
/*
用 bezier curve（贝塞尔曲线） 来设置 color stop（颜色渐变规则），
这里使用下面的曲线形式，其中
X轴为 offset（偏移量，取值范围为 0~1，0 代表阴影绘制起点），
Y轴为 alpha（颜 色透明度，取值范围为0~1，0 代表完全透明），
*/

void draw_rectangle_gradient(void* ctx, int width, int height, const rect_shadow_t* pr)
{
	if (!ctx || !pr || width < 4 || height < 4)return;
	auto cr = (VkvgContext)ctx;
	auto& rs = *pr;
	std::vector<stops_t> color_stops;
	float radius = rs.radius;
	if (radius < 1 || width < 2 || height < 2)
		return;
	auto bv = get_bezier(&rs.cubic, 1, rs.segment > 3 ? rs.segment : 3);
	for (size_t i = 0; i < bv.size(); i++)
	{
		float ratio = bv[i].x;
		float a = bv[i].y;
		glm::vec4 color = mix_colors0(rs.cfrom, rs.cto, ratio);
		color_stops.push_back({ ratio,color.x, color.y, color.z, color.w * a });
	}
	if (width < 2 * radius)
	{
		width = 2 * radius;
	}
	if (height < 2 * radius)
	{
		height = 2 * radius;
	}

	//# radial gradient center points for four corners, top - left, top - right, etc
	glm::vec2 corner_tl = glm::vec2(radius, radius);
	glm::vec2 corner_tr = glm::vec2(width - radius, radius);
	glm::vec2 corner_bl = glm::vec2(radius, height - radius);
	glm::vec2 corner_br = glm::vec2(width - radius, height - radius);
	std::vector<glm::vec2> corner_points = { corner_tl, corner_tr, corner_br, corner_bl };

	//# linear gradient rectangle info for four sides
	glm::vec4 side_top = glm::vec4(radius, 0, width - 2 * radius, radius);
	glm::vec4 side_bottom = glm::vec4(radius, height - radius, width - 2 * radius, radius);
	glm::vec4 side_left = glm::vec4(0, radius, radius, height - 2 * radius);
	glm::vec4 side_right = glm::vec4(width - radius, radius, radius, height - 2 * radius);

	//# draw four corners through radial gradient
	int i = 0;
	for (auto& point : corner_points)
	{
		auto rg = vkvg_pattern_create_radial(point[0], point[1], 0, point[0], point[1], radius);
		for (auto& stop : color_stops)
		{
			vkvg_pattern_add_color_stop_rgba(rg, stop.o, stop.x, stop.y, stop.z, stop.w);
		}
		vkvg_move_to(cr, point[0], point[1]);
		double	angle1 = M_PI + 0.5 * M_PI * i, angle2 = angle1 + 0.5 * M_PI;
		vkvg_arc(cr, point[0], point[1], radius, angle1, angle2);
		vkvg_set_source(cr, rg);
		vkvg_fill(cr);
		vkvg_pattern_destroy(rg);
		i++;
	}

	//# draw four sides through linear gradient
	//# top side

	auto lg_top = vkvg_pattern_create_linear(side_top[0], side_top[1] + radius, side_top[0], side_top[1]);
	for (auto& stop : color_stops)
	{
		vkvg_pattern_add_color_stop_rgba(lg_top, stop.o, stop.x, stop.y, stop.z, stop.w);
	}
	vkvg_rect(cr, side_top);
	vkvg_set_source(cr, lg_top);
	vkvg_fill(cr);

	//# bottom side
	auto lg_bottom = vkvg_pattern_create_linear(side_bottom[0], side_bottom[1], side_bottom[0], side_bottom[1] + radius);
	for (auto& stop : color_stops)
	{
		vkvg_pattern_add_color_stop_rgba(lg_bottom, stop.o, stop.x, stop.y, stop.z, stop.w);
	}
	vkvg_rect(cr, side_bottom);
	vkvg_set_source(cr, lg_bottom);
	vkvg_fill(cr);

	//# left side
	auto lg_left = vkvg_pattern_create_linear(side_left[0] + radius, side_left[1], side_left[0], side_left[1]);
	for (auto& stop : color_stops)
	{
		vkvg_pattern_add_color_stop_rgba(lg_left, stop.o, stop.x, stop.y, stop.z, stop.w);
	}
	vkvg_rect(cr, side_left);
	vkvg_set_source(cr, lg_left);
	vkvg_fill(cr);

	//# right side
	auto lg_right = vkvg_pattern_create_linear(side_right[0], side_right[1], side_right[0] + radius, side_right[1]);
	for (auto& stop : color_stops)
	{
		vkvg_pattern_add_color_stop_rgba(lg_right, stop.o, stop.x, stop.y, stop.z, stop.w);
	}
	vkvg_rect(cr, side_right);
	vkvg_set_source(cr, lg_right);
	vkvg_fill(cr);

	vkvg_flush(cr);
	//vkvg_surface_resolve(surf);//msaa采样转换输出

	vkvg_pattern_destroy(lg_top);
	vkvg_pattern_destroy(lg_bottom);
	vkvg_pattern_destroy(lg_left);
	vkvg_pattern_destroy(lg_right);
}


void draw_grid(void* ctx, grid_info_t* t, vkvg_func_t* func)
{
	auto cr = (VkvgContext)ctx;
	if (!cr || !func || !t || t->count < 2 || t->width < t->count)return;
	func->vkvg_save(cr);
	func->vkvg_rectangle(cr, 0, 0, t->width, t->width);
	func->vkvg_clip_preserve(cr);
	func->vkvg_set_source_color(cr, t->back_color);
	func->vkvg_fill(cr);
	func->vkvg_translate(cr, 0.5, 0.5);
	int inw = t->width / t->count;
	// 内线
	for (size_t i = 1; i < t->count; i++)
	{
		func->vkvg_move_to(cr, inw * i, 0);
		func->vkvg_line_to(cr, inw * i, t->width);
		func->vkvg_move_to(cr, 0, inw * i);
		func->vkvg_line_to(cr, t->width, inw * i);
	}
	func->vkvg_set_source_color(cr, t->inline_color);
	func->vkvg_stroke(cr);
	// 外框线
	func->vkvg_move_to(cr, t->width, 0);
	func->vkvg_line_to(cr, 0, 0);
	func->vkvg_line_to(cr, 0, t->width);
	func->vkvg_set_source_color(cr, t->line_color);
	func->vkvg_stroke(cr);
	func->vkvg_restore(cr);
}


#if 0
image_sliced_t new_rect(const rect_shadow_t& rs)
{
	image_sliced_t r = {};
	glm::ivec2 ss = { rs.radius * 3, rs.radius * 3 };
	timg.resize(ss.x * ss.y);
#ifdef _CR__
	auto sur = new_image_cr(ss, timg.data());
	cairo_t* cr = new_cr(sur);
	// 边框阴影
	draw_rectangle_gradient(cr, ss.x, ss.y, &rs);

	image_ptr_t px = {};
	px.width = ss.x;
	px.height = ss.y;
	px.type = 1;
	px.stride = ss.x * sizeof(int);
	px.data = timg.data();
	px.comp = 4;
	glm::ivec2 ps = {};
	auto px0 = bcc.push_cache_bitmap(&px, &ps);
	if (px0) {
		r.img_rc = { 0,0,ss.x,ss.y };
		r.tex_rc = { ps.x,ps.y,ss.x,ss.y };
		r.sliced.x = r.sliced.y = r.sliced.z = r.sliced.w = rs.radius + 1;
		r.color = -1;
		img = px0;
	}
#ifdef _DEBUG
	image_save_png(sur, "temp/gshadow.png");
#endif
	free_image_cr(sur);
	free_cr(cr);
#endif
	return r;
}
#endif

void gen3data(const glm::ivec2& tex_size, const glm::ivec2& dst_pos, const glm::ivec4& rc, const glm::ivec2& dsize, uint32_t color, t_vector<text_vx>* opt, t_vector<uint32_t>* idx)
{
	// 1. 计算矩形顶点（像素坐标）
	glm::ivec2 A = glm::ivec2(rc.x, rc.y); // 左下
	glm::ivec2 B = A + glm::ivec2(rc.z, 0); // 右下
	glm::ivec2 C = A + glm::ivec2(rc.z, rc.w); // 右上
	glm::ivec2 D = A + glm::ivec2(0, rc.w); // 左上
	auto ds = dsize;
	if (!(ds.x > 0 && ds.y > 0))
	{
		ds.x = rc.z;
		ds.y = rc.w;
	}
	glm::vec2 A1 = dst_pos;
	glm::vec2 B1 = dst_pos + glm::ivec2(ds.x, 0);
	glm::vec2 C1 = dst_pos + glm::ivec2(ds.x, ds.y);
	glm::vec2 D1 = dst_pos + glm::ivec2(0, ds.y);

	// 2. 归一化因子
	float inv_w = 1.0f / static_cast<float>(tex_size.x);
	float inv_h = 1.0f / static_cast<float>(tex_size.y);

	// 3. 归一化顶点坐标
	auto normalize2 = [&](const glm::ivec2& p) -> glm::vec2 {
		return glm::vec2(p.x * inv_w, p.y * inv_h);
		};
	glm::vec2 A_norm = normalize2(A);
	glm::vec2 B_norm = normalize2(B);
	glm::vec2 C_norm = normalize2(C);
	glm::vec2 D_norm = normalize2(D);

	// 4. 解析颜色
	glm::vec4 c = ucolor2f(color);
	// 5. 生成顶点数组（4顶点，32个浮点数）
	uint32_t ps = opt->size();
	opt->reserve(ps + 4);
	idx->reserve(idx->size() + 6);
	opt->push_back(text_vx{ A1 ,A_norm , c });
	opt->push_back(text_vx{ B1 ,B_norm , c });
	opt->push_back(text_vx{ C1 ,C_norm , c });
	opt->push_back(text_vx{ D1 ,D_norm , c });
	idx->insert(idx->end(), { ps + 0,ps + 1,ps + 2,ps + 0,ps + 2,ps + 3 });
	return;
}


clicprect_cx::clicprect_cx(void* renderer, texture_cb* cb, const glm::ivec4& rc) :rcb(cb), _renderer(renderer)
{
	if (rcb && renderer && rc.z > 0 && rc.w > 0)
	{
		rcb->get_cliprect(renderer, &oldrc);
		rcb->set_cliprect(renderer, &rc);
	}
	else {
		rcb = nullptr;
	}
}

clicprect_cx::~clicprect_cx()
{
	if (rcb && _renderer)
	{
		rcb->set_cliprect(_renderer, &oldrc);
	}
}



vgcache_cx::vgcache_cx()
{}

vgcache_cx::~vgcache_cx()
{}


void draw_rectangle(VkvgContext cr, const glm::vec4& rc, int r);

union u_col
{
	uint32_t uc;
	unsigned char u[4];
	struct urgba
	{
		unsigned char r, g, b, a;
	}c;
};
#define FCV 255.0
#define FCV1 256.0
inline uint32_t set_alpha_xf(uint32_t c, double af)
{
	if (af < 0)af = 0;
	//uint32_t a = af * FCV;
	u_col* t = (u_col*)&c;
	double a = t->c.a * af + 0.5;
	if (a > 255)
		t->c.a = 255;
	else
		t->c.a = a;
	return c;
}
inline uint32_t set_alpha_xf2(uint32_t c, double af)
{
	if (af < 0)af = 0;
	u_col* t = (u_col*)&c;
	t->c.r *= af;
	t->c.g *= af;
	t->c.b *= af;
	return c;
}
inline uint32_t set_alpha_f(uint32_t c, double af)
{
	if (af > 1)af = 1;
	if (af < 0)af = 0;
	uint32_t a = af * FCV + 0.5;
	u_col* t = (u_col*)&c;
	t->c.a = a;
	return c;
}
void set_color(VkvgContext cr, uint32_t rgba)
{
	vkvg_set_source_rgba(cr, SP_RGBA32_R_F(rgba),
		SP_RGBA32_G_F(rgba),
		SP_RGBA32_B_F(rgba),
		SP_RGBA32_A_F(rgba));
}
inline glm::vec4 to_c4(uint32_t c)
{
	u_col* t = (u_col*)&c;
	glm::vec4 fc;
	float* f = &fc.x;
	for (int i = 0; i < 4; i++)
	{
		*f++ = t->u[i] / FCV;
	}
	return  fc;
}

void draw_rectangle(VkvgContext cr, const glm::vec4& rc, int r)
{
	if (r > 0)
	{
		vkvg_rounded_rectangle(cr, rc.x, rc.y, rc.z, rc.w, r);
	}
	else {
		vkvg_rectangle(cr, rc.x, rc.y, rc.z, rc.w);
	}
}

void vkvg_grid_fill(VkvgContext cr, glm::vec2  size, glm::ivec2 cols, int width)
{
	int x = fmod(size.x, width);
	int y = fmod(size.y, width);
	int xn = size.x / width;
	int yn = size.y / width;
	if (x > 0)xn++;
	if (y > 0)yn++;
	vkvg_save(cr);
	draw_rectangle(cr, { 0,0, size.x, size.y }, 0);
	vkvg_clip(cr);
	for (size_t i = 0; i < yn; i++)
	{
		for (size_t j = 0; j < xn; j++)
		{
			bool k = (j & 1);
			if (!(i & 1))
				k = !k;
			auto c = cols[k];
			draw_rectangle(cr, { j * width,i * width,width,width }, 0);
			set_color(cr, c);
			vkvg_fill(cr);
		}
	}
	vkvg_restore(cr);
}


#if 1

void mr_v4(glm::vec4& rc, const glm::vec2& it) {
	rc.x = std::min(rc.x, it.x);
	rc.y = std::min(rc.y, it.y);
	rc.z = std::max(rc.z, it.x);
	rc.w = std::max(rc.w, it.y);
}

rvg_cx::rvg_cx()
{}

rvg_cx::~rvg_cx()
{}

void rvg_cx::set_pos(const glm::ivec2& ps)
{
	if (pos != ps)
	{
		pos = ps;
	}
}

void rvg_cx::clear()
{
	_cmdtype.clear();		// 操作类型
	_cmd.clear();			// 命令数据
	_cmd_pos.clear();		// 命令的数据偏移
	_view.clear();		// 视图列表
	pos = {};
	while (_stk.size()) {
		_stk.pop();
	}
	while (_st_view.size()) {
		_st_view.pop();
	}
	_cur = {};				// 当前状态
	_tem_clip = {};
	_tview = {};				// 当前渲染区域 
}

#if 1

void rvg_cx::submit(fill_style_d* st)
{
	if (!st)return;
	push_ct(OP_SUBMIT_STYLE);
	auto t = *st;
	if (t.dash_p) {
		t.dash_p = (float*)(sizeof(t));
	}
	_cmd.insert(_cmd.end(), (char*)&t, (char*)&t + sizeof(t));
	if (t.dash_p) {
		_cmd.insert(_cmd.end(), (char*)st->dash_p, (char*)st->dash_p + sizeof(float) * st->dash_num);
	}
}
void rvg_cx::submit(uint32_t fill, uint32_t color, int linewidth)
{
	push_ct(OP_SUBMIT_COLOR);
	submit_color_d t = { fill,color,linewidth };
	_cmd.insert(_cmd.end(), (char*)&t, (char*)&t + sizeof(t));
}
void rvg_cx::grid_fill(const glm::vec2& size, const glm::ivec2& cols, int width)
{
	push_ct(OP_GRID_FILL);
	grid_fill_r t = { size,cols,width };
	_cmd.insert(_cmd.end(), (char*)&t, (char*)&t + sizeof(t));
}

void rvg_cx::linear_fill(const glm::vec2& size, const glm::vec4* cols, int count)
{
	push_ct(OP_LINEAR_FILL);
	linear_fill_r t = { size,count };
	_cmd.insert(_cmd.end(), (char*)&t, (char*)&t + sizeof(t));
	if (count > 0)
	{
		_cmd.insert(_cmd.end(), (char*)(cols), (char*)(cols + count));
	}
}
void rvg_cx::add_arrow(const glm::vec2& p0, const glm::vec2& p1, float arrow_hwidth, float arrow_size, bool type)
{
	push_ct(OP_ADD_ARROW);
	arrow_d t = { p0,p1,arrow_hwidth,arrow_size,type };
	_cmd.insert(_cmd.end(), (char*)&t, (char*)&t + sizeof(t));
}
void rvg_cx::draw_block(dblock_d* p, fill_style_d* st)
{
	if (!p || !p->points || p->count == 0 || p->rc.x <= 0 || p->rc.y < 0 || !st)return;
	save();
	push_ct(OP_DRAW_BLOCK);
	auto t = *p;
	t.points = (glm::vec2*)(_cmd.size() + sizeof(t));
	_cmd.insert(_cmd.end(), (char*)&t, (char*)&t + sizeof(dblock_d));
	_cmd.insert(_cmd.end(), (char*)p->points, (char*)p->points + sizeof(glm::vec2) * p->count);
	submit(st);
	restore();
}

void rvg_cx::draw_path(path_d* path, fill_style_d* style)
{
	if (!path || path->count == 0 || !path->v || !style)return;
	save();
	push_ct(OP_DRAW_PATH);
	auto t = *path;
	t.v = (path_vertex_t*)(_cmd.size() + sizeof(t));
	_cmd.insert(_cmd.end(), (char*)&t, (char*)&t + sizeof(path_d));
	_cmd.insert(_cmd.end(), (char*)path->v, (char*)path->v + sizeof(path_vertex_t) * path->count);
	submit(style);
	restore();
}

void rvg_cx::add_line(glm::vec4* p, size_t count)
{
	push_ct(OP_ADD_LINE_PTR);
	_cmd.insert(_cmd.end(), (char*)&count, (char*)&count + sizeof(size_t));
	_cmd.insert(_cmd.end(), (char*)p, (char*)p + sizeof(glm::vec4) * count);
}

void rvg_cx::add_line(const glm::vec2& ps0, const glm::vec2& ps1)
{
	glm::vec2 v[2] = { ps0,ps1 };
	add_polyline(v, 2);
}

void rvg_cx::add_rect(const glm::vec4& rc, double r)
{
	push_ct(OP_ADD_RECT_DOUBLE);
	_cmd.insert(_cmd.end(), (char*)&rc, (char*)&rc + sizeof(rc));
	_cmd.insert(_cmd.end(), (char*)&r, (char*)&r + sizeof(r));
	_tem_clip = rc;
	_tem_clip.x += _cur.pos.x;
	_tem_clip.y += _cur.pos.y;
}
void rvg_cx::add_rect(const glm::vec4& rc, const glm::vec4& r)
{
	push_ct(OP_ADD_RECT_VEC4);
	rect_v4 t = { rc, r };
	_cmd.insert(_cmd.end(), (char*)&t, (char*)&t + sizeof(t));
	_tem_clip = rc;
	_tem_clip.x += _cur.pos.x;
	_tem_clip.y += _cur.pos.y;
}

void rvg_cx::add_circle(const glm::vec2& pos, float r)
{
	if (!(r > 0))return;
	push_ct(OP_ADD_CIRCLE);
	circle_r t = { pos,r };
	_cmd.insert(_cmd.end(), (char*)&t, (char*)&t + sizeof(t));
}

void rvg_cx::add_ellipse(const glm::vec2& pos, const glm::vec2& r, float rotationAngle)
{
	push_ct(OP_ADD_ELLIPSE);
	ellipse_r t = { pos, r, rotationAngle };
	_cmd.insert(_cmd.end(), (char*)&t, (char*)&t + sizeof(t));
}
void rvg_cx::add_triangle(const glm::vec2& pos, const glm::vec2& size, const glm::vec2& dirspos)
{
	push_ct(OP_ADD_TRIANGLE);
	triangle_r t = { pos,size,dirspos };
	_cmd.insert(_cmd.end(), (char*)&t, (char*)&t + sizeof(t));
}

void rvg_cx::draw_polyline(const glm::vec2& pos, const glm::vec2* points, int points_count, uint32_t col, bool closed, float thickness)
{
	push_ct(OP_POLYLINE_VEC2);
	polyline_r t = { pos,points_count,col,thickness,closed };
	_cmd.insert(_cmd.end(), (char*)&t, (char*)&t + sizeof(t));
	_cmd.insert(_cmd.end(), (char*)points, (char*)points + sizeof(glm::vec2) * points_count);
}

void rvg_cx::add_polyline(const PathsD* p, bool closed)
{
	if (!p || p->size() < 1)return;
	push_ct(OP_ADD_POLYLINE_PATH);
	auto& d = *p;
	auto length = d.size();

	polyline_pd pd = {};
	pd.count = d.size();
	pd.closed = closed;
	_cmd.insert(_cmd.end(), (char*)&pd, (char*)&pd + sizeof(polyline_pd));
	for (size_t i = 0; i < length; i++)
	{
		auto points = d[i];
		glm::vec2 pos = { points[0].x, points[0].y };
		auto points_count = points.size();
		_cmd.insert(_cmd.end(), (char*)&points_count, (char*)&points_count + sizeof(points_count));
		_cmd.insert(_cmd.end(), (char*)&pos, (char*)&pos + sizeof(glm::vec2));
		for (size_t i = 1; i < points_count; i++)
		{
			pos = { points[i].x, points[i].y };
			_cmd.insert(_cmd.end(), (char*)&pos, (char*)&pos + sizeof(glm::vec2));
		}
	}
}
#define PUSH_VALUE(v) _cmd.insert(_cmd.end(), (char*)&v, (char*)&v + sizeof(v))
#define PUSH_VALUE_P(v,c) _cmd.insert(_cmd.end(), (char*)v, (char*)v + sizeof(*v)*c)

void rvg_cx::add_polyline(const glm::vec2* p, size_t count)
{
	if (!p || count < 2)return;
	push_ct(OP_ADD_POLYLINE_VEC2_PTR);
	PUSH_VALUE(count);
	PUSH_VALUE_P(p, count);
}
void rvg_cx::draw_polylines(const glm::vec2& pos, const glm::vec2* points, int points_count, int* idx, int idx_count, uint32_t col, float thickness)
{
	if (!points || points_count < 2 || !idx || idx_count < 2)return;
	push_ct(OP_POLYLINES);
	polyline_index_r t = { pos,points_count,idx_count,col,thickness };
	_cmd.insert(_cmd.end(), (char*)&t, (char*)&t + sizeof(t));
	_cmd.insert(_cmd.end(), (char*)points, (char*)points + sizeof(glm::vec2) * points_count);
	_cmd.insert(_cmd.end(), (char*)idx, (char*)idx + sizeof(int) * idx_count);
}

void rvg_cx::set_text_style(text_style* ts)
{
	if (!ts)return;
	push_ct(OP_TEXT_STYLE);
	_cmd.insert(_cmd.end(), (char*)ts, (char*)ts + sizeof(text_style));
}

void rvg_cx::add_text(text_st* p, text_style* ts)
{
	if (!p || p->text == 0 || p->text_len < 1)return;
	auto pos = p->pos + _cur.pos;
	glm::vec4 rc = { pos , 0,0 };//p->size
	//push_view(rc);
	set_text_style(ts);
	push_ct(OP_ADD_TEXT);
	auto idx = _cmd.size();
	_cmd.insert(_cmd.end(), (char*)p, (char*)p + sizeof(text_st));
	auto pd = (text_st*)(_cmd.data() + idx);
	auto tps = _cmd.size();
	pd->pos = pos;
	pd->clip = _cur.clip;
	_cmd.insert(_cmd.end(), pd->text, pd->text + pd->text_len);
	pd->text = (char*)tps;
	//pop_view();
}

void rvg_cx::add_image(image_ptr_t* img, const glm::ivec4& rc, const glm::ivec4& sliced, uint32_t color, const glm::ivec2& dsize, const glm::ivec2& pos)
{
	image_r r = { img, rc, sliced, dsize, pos, color };
	add_image(&r);
}

void rvg_cx::add_image(image_r* r)
{
	if (!r || !r->img)return;
	glm::vec4 rc = { _cur.pos + (glm::vec2)r->pos , 0,0 };
	//push_view(rc);
	push_ct(OP_ADD_IMAGE);
	_cmd.insert(_cmd.end(), (char*)r, (char*)(r + 1));
	//pop_view();
}

void rvg_cx::add_geometry(geometry_d* geo)
{
	//push_view(glm::ivec4(_cur.pos, 0, 0));
	push_ct(OP_ADD_GEOMETRY);
	_cmd.insert(_cmd.end(), (char*)geo, (char*)(geo + 1));
	//pop_view();
}

void rvg_cx::paint_shadow(double size_x, double size_y, double width, double height, const glm::vec4& shadow, const glm::vec4& color_to, bool rev, float r)
{
	push_ct(OP_PAINT_SHADOW);
	paint_shadow_d d = {};
	d.size = glm::vec2(size_x, size_y);
	d.dst_size = glm::vec2(width, height);
	d.shadow = shadow;
	d.color_to = color_to;
	d.rev = rev;
	d.r = r;
	_cmd.insert(_cmd.end(), (char*)&d, (char*)&d + sizeof(d));
}

void rvg_cx::translate(const glm::vec2& offset)
{
	push_ct(OP_TRANSLATE);
	_cmd.insert(_cmd.end(), (char*)&offset, (char*)&offset + sizeof(offset));
	_cur.pos += offset;
}

void rvg_cx::scale(float sx, float sy)
{
	push_ct(OP_SCALE);
	glm::vec2 sc = { sx,sy };
	_cmd.insert(_cmd.end(), (char*)&sc, (char*)&sc + sizeof(sc));
	_cur.scale *= sc;
}

void rvg_cx::scale(const glm::vec2& sc)
{
	push_ct(OP_SCALE);
	_cmd.insert(_cmd.end(), (char*)&sc, (char*)&sc + sizeof(sc));
	_cur.scale *= sc;
}

void rvg_cx::rotate(float radians)
{
	push_ct(OP_ROTATE);
	_cmd.insert(_cmd.end(), (char*)&radians, (char*)&radians + sizeof(radians));
	_cur.rotate_radians += radians;
}

void rvg_cx::transform(const glm::mat3x2* matrix)
{
	push_ct(OP_TRANSFORM);
	_cmd.insert(_cmd.end(), (char*)matrix, (char*)matrix + sizeof(glm::mat3x2));
	_cur._transform = glm::matrixCompMult(_cur._transform, *matrix);
}

void rvg_cx::set_matrix(const glm::mat3x2* matrix)
{
	push_ct(OP_SET_MATRIX);
	_cmd.insert(_cmd.end(), (char*)matrix, (char*)matrix + sizeof(glm::mat3x2));
	_cur._transform = *matrix;
}

void rvg_cx::get_matrix(glm::mat3x2* matrix)
{
	if (matrix)
	{
		*matrix = _cur._transform;
	}
}

glm::vec2 rvg_cx::get_translate()
{
	return _cur.pos;
}


void rvg_cx::clip()
{
	push_ct(OP_CLIP);
	_cur.clip = _tem_clip;
}

void rvg_cx::clip(const glm::ivec4& c)
{
	add_rect(c, 0);
	push_ct(OP_CLIP);
	_cur.clip = _tem_clip;
}

glm::ivec4 rvg_cx::get_clip()
{
	return _cur.clip;
}

void rvg_cx::save()
{
	push_ct(OP_SAVE);
	_stk.push(_cur);
}

void rvg_cx::restore()
{
	push_ct(OP_RESTORE);
	if (_stk.size())
	{
		_cur = _stk.top();
		_stk.pop();
	}
}

void rvg_cx::save0()
{
	_stk.push(_cur);
}

void rvg_cx::restore0()
{
	if (_stk.size())
	{
		_cur = _stk.top();
		_stk.pop();
	}
}

void rvg_cx::fill()
{
	push_ct(OP_FILL);
}

void rvg_cx::stroke()
{
	push_ct(OP_STROKE);
}

void rvg_cx::fill_preserve()
{
	push_ct(OP_FILL_PRESERVE);
}

void rvg_cx::stroke_preserve()
{
	push_ct(OP_STROKE_PRESERVE);
}

void rvg_cx::fill_stroke(uint32_t fill_color, uint32_t color)
{
	if (fill_color)
	{
		set_color(fill_color);
		if (color)
			fill_preserve();
		else
			fill();
	}
	if (color)
	{
		set_color(color);
		stroke();
	}
}

void rvg_cx::set_line_width(float w)
{
	push_ct(OP_SET_LINE_WIDTH);
	_cmd.insert(_cmd.end(), (char*)&w, (char*)&w + sizeof(w));
}

void rvg_cx::set_color(uint32_t color)
{
	push_ct(OP_SET_COLOR_UINT);
	_cmd.insert(_cmd.end(), (char*)&color, (char*)&color + sizeof(color));
}

void rvg_cx::set_color(const glm::vec4& rgba)
{
	push_ct(OP_SET_COLOR_VEC4);
	_cmd.insert(_cmd.end(), (char*)&rgba, (char*)&rgba + sizeof(rgba));
}


// 区域是否相交
bool check_rect_cross(const glm::vec4& r1, const glm::vec4& r2)
{
	if (glm::max(r1.x, r2.x) <= glm::min(r1.z, r2.z) && glm::max(r1.y, r2.y) <= glm::min(r1.w, r2.w))
	{
		return true;   //有交集
	}
	else {
		return false;   //无交集
	}
}

inline bool is_textimage(uint8_t c) {
	return c >= rvg_cx::OP_TEXT_STYLE;
}

void rvg_cx::push_view(const glm::ivec4& c, void* ptr)
{
	_tview = c;
	if (!ptr && _view.size()) {
		ptr = _view.back().ptr;
	}
	cmdview_v cv = {};
	cv.rc = c;
	cv.first = _cmdtype.size();
	cv.dcv_index = 0;
	cv.ptr = ptr;
	cv.pos = pos;
	_st_view.push(_view.size());
	_view.push_back(cv);
	push_ct(OP_VIEW);
	save0();
}

void rvg_cx::pop_view()
{
	restore0();
	push_ct(OP_VIEW_POP);
	size_t cidx = -1;
	if (_st_view.size())
	{
		cidx = _st_view.top();
		_st_view.pop();
	}
	if (cidx < _view.size())
	{
		auto& v = _view[cidx];
		v.count = _cmdtype.size() - v.first;
	}
	else {
		assert(0);// 没有显示区
	}
}

bool rvg_cx::is_image()
{
	auto t = _cmdtype.back();
	return t > OP_TEXT_STYLE;
}
void rvg_cx::push_null(int v)
{
	push_ct(v);
}
inline uint32_t r_crc32(const void* d, uint32_t l) {
	return  crc32(0, (Bytef*)d, l);
}
uint32_t rvg_cx::get_crc()
{
	return r_crc32(_view.data(), _view.size());
	//return ecc_crc32u(_view.data(), _view.size()); 
}
uint32_t rvg_cx::get_crc2index(size_t i)
{
	uint32_t r = 0;
	if (i < _view.size() && _cmd.size() && _cmd_pos.size()) {
		auto& it = _view[i];
		auto ct1 = r_crc32(&it, sizeof(it));
		auto ct2 = r_crc32(_cmdtype.data() + it.first, std::min(it.count, _cmdtype.size() - it.first));
		auto dpos = _cmd_pos[it.first];
		auto dpos1 = _cmd_pos[it.first + it.count];
		auto ct3 = r_crc32(_cmd.data() + dpos, std::min(dpos1 - dpos, _cmd.size() - dpos));
		r = ct1 ^ ct2 ^ ct3;
	}
	return r;
}
uint32_t get_crc(cmdview_v* cv, const char* cmd, size_t cmdc, const char* dt, size_t dtc, const size_t* cpos)
{
	uint32_t r = 0;
	auto& it = *cv;
	auto ct1 = r_crc32(&it, sizeof(it));
	auto ct2 = r_crc32(dt + it.first, std::min(it.count, dtc - it.first));
	auto dpos = cpos[it.first];
	auto dpos1 = cpos[it.first + it.count];
	auto ct3 = r_crc32(cmd + dpos, std::min(dpos1 - dpos, cmdc - dpos));
	r = ct1 ^ ct2 ^ ct3;
	return r;
}
void rvg_cx::push_ct(uint8_t op)
{
	_cmdtype.push_back((Opcode)op);
	_cmd_pos.push_back(_cmd.size());
}

void rvg_cx::save_file(const char* fn)
{
	hz::mfile_t f;
	if (f.open_m(fn, false)) {
		njson0 h;
		h["pos"] = { pos.x,pos.y };
		std::vector<size_t> ps = { _view.size() * sizeof(_view[0])
			,_cmd_pos.size() * sizeof(_cmd_pos[0])
			, _cmdtype.size() * sizeof(_cmdtype[0])
			, _cmd.size() * sizeof(_cmd[0])
		};
		h["offset"] = ps;
		size_t a = 0;
		for (auto it : ps) {
			a += it;
		}
		h["data_size"] = a;
		auto crc = get_crc();
		h["crc"] = crc;
		auto dumstr = h.dump(2);
		a += dumstr.size() + 1;
		f.ftruncate_m(a);
		auto d = f.map(a, 0);
		if (d)
		{
			f.write(dumstr.c_str(), dumstr.size() + 1);
			f.write(_view.data(), ps[0]);
			f.write(_cmd_pos.data(), ps[1]);
			f.write(_cmdtype.data(), ps[2]);
			f.write(_cmd.data(), ps[3]);
			f.flush();
		}
	}
}

namespace gp
{
	glm::vec2 get_vec2(njson& n, const char* k);
	glm::vec2 get_vec2(njson0& n, const char* k);
}
bool rvg_cx::load_file(const char* fn)
{
	hz::mfile_t f;
	bool ret = false;
	auto d = f.open_d(fn, true);
	do {
		if (!d || d[0] != '{')break;
		clear();
		auto ss = strlen(d);
		njson0 h = njson0::parse(d, d + f.size());
		if (h.empty() || !h.is_object())break;
		pos = gp::get_vec2(h, "pos");
		auto crc = h["crc"].get<uint32_t>();
		std::vector<size_t> ps = gp::get_vsize(h, "offset");
		if (ps.empty())break;
		size_t a0 = h["data_size"].get<size_t>();
		size_t a = 0;
		for (auto it : ps) {
			a += it;
		}
		if (a != a0 || a < (f.size() - ss - 1))break;
		f.seek(ss + 1);
		_view.resize(ps[0] / sizeof(_view[0]));
		_cmd_pos.resize(ps[1] / sizeof(_cmd_pos[0]));
		_cmdtype.resize(ps[2] / sizeof(_cmdtype[0]));
		_cmd.resize(ps[3] / sizeof(_cmd[0]));
		f.read(_view.data(), ps[0]);
		f.read(_cmd_pos.data(), ps[1]);
		f.read(_cmdtype.data(), ps[2]);
		f.read(_cmd.data(), ps[3]);
		auto crc1 = get_crc();
		if (crc != crc1)
		{
			assert(0);
		}
		else {
			ret = true;
		}
	} while (0);
	return ret;
}



size_t cmd_op_submit_style(uint8_t* d, VkvgContext ctx)
{
	size_t n = sizeof(fill_style_d);
	fill_style_d* st = (fill_style_d*)d;
	if (st->dash_p)
	{
		st->dash_p = (float*)(d + n);
		n += sizeof(float) * st->dash_num;
	}

	bool stroke = st && st->thickness > 0 && st->color > 0;
	if (!ctx || !st)return n;
	auto cr = ctx;
	if (st->fill)
	{
		set_color(cr, st->fill);
		if (stroke)
			vkvg_fill_preserve(cr);
		else
			vkvg_fill(cr);
	}
	if (stroke)
	{
		vkvg_set_line_width(cr, st->thickness);
		if (st->cap > 0)
			vkvg_set_line_cap(cr, (vkvg_line_cap_t)st->cap);
		if (st->join > 0)
			vkvg_set_line_join(cr, (vkvg_line_join_t)st->join);
		if (st->dash.v > 0 || st->dash_p)
		{
			float dashes[64] = {};
			uint64_t x = 1;
			auto t = dashes;
			int num_dashes = st->dash_num;
			if (st->dash_p)
			{
				vkvg_set_dash(cr, st->dash_p, st->dash_num, st->dash_offset);
			}
			else
			{
				if (num_dashes > 8)num_dashes = 8;
				for (size_t i = 0; i < num_dashes; i++)
				{
					*t = st->dash.v8[i]; t++;
				}
				if (num_dashes > 0)
					vkvg_set_dash(cr, dashes, num_dashes, st->dash_offset);
			}
		}
		set_color(cr, st->color);
		vkvg_stroke(cr);
	}
	return n;
}

size_t cmd_op_submit_color(uint8_t* d, VkvgContext ctx)
{
	submit_color_d* p = (submit_color_d*)d;
	bool stroke = p->linewidth > 0 && p->color;
	auto cr = ctx;
	if (p->fill)
	{
		//if (isbgr)
		//	set_color_bgr(cr, fill);
		//else
		set_color(cr, p->fill);
		if (stroke)
			vkvg_fill_preserve(cr);
		else
			vkvg_fill(cr);
	}
	if (stroke)
	{
		vkvg_set_line_width(cr, p->linewidth);
		set_color(cr, p->color);
		vkvg_stroke(cr);
	}
	return sizeof(submit_color_d);
}

size_t cmd_op_grid_fill(uint8_t* d, VkvgContext cr)
{
	auto p = (grid_fill_r*)d;
	auto width = p->width;
	int x = fmod(p->size.x, width);
	int y = fmod(p->size.y, width);
	int xn = p->size.x / width;
	int yn = p->size.y / width;
	if (x > 0)xn++;
	if (y > 0)yn++;
	vkvg_save(cr);
	draw_rectangle(cr, { 0,0,p->size.x,p->size.y }, 0);
	vkvg_clip(cr);
	for (size_t i = 0; i < yn; i++)
	{
		for (size_t j = 0; j < xn; j++)
		{
			bool k = (j & 1);
			if (!(i & 1))
				k = !k;
			auto c = p->cols[k];
			draw_rectangle(cr, { j * width,i * width,width,width }, 0);
			set_color(cr, c);
			vkvg_fill(cr);
		}
	}
	vkvg_restore(cr);
	return sizeof(grid_fill_r);
}

size_t cmd_op_linear_fill(uint8_t* d, VkvgContext cr)
{
	auto p = (linear_fill_r*)d;
	glm::vec2& size = p->size;
	glm::vec4* cols = (glm::vec4*)(d + sizeof(linear_fill_r));
	int count = p->count;
	if (count <= 0)
	{
		return sizeof(linear_fill_r);
	}
	VkvgPattern gr = vkvg_pattern_create_linear(0, 0, size.x, size.y);
	double n = count - 1;
	for (size_t i = 0; i < count; i++)
	{
		auto color = cols[i];
		vkvg_pattern_add_color_stop_rgba(gr, i / n, color.x, color.y, color.z, color.w);
	}
	draw_rectangle(cr, { 0,0,size.x,size.y }, 0);
	vkvg_set_source(cr, gr);
	vkvg_fill(cr);
	vkvg_pattern_destroy(gr);
	return sizeof(linear_fill_r) + sizeof(glm::vec4) * count;
}
size_t cmd_op_add_arrow(uint8_t* d, VkvgContext cr)
{
	auto p = (arrow_d*)d;
	draw_arrow(cr, p->p0, p->p1, p->arrow_hwidth, p->arrow_size, p->type);
	return sizeof(arrow_d);
}
size_t cmd_op_draw_block(uint8_t* d, VkvgContext ctx) {

	auto pd = *(dblock_d*)d;
	pd.points = (glm::vec2*)(d + sizeof(dblock_d));
	auto p = &pd;
	auto n = sizeof(dblock_d) + sizeof(glm::vec2) * p->count;
	if (!ctx || !p->points || p->count == 0 || p->rc.x <= 0 || p->rc.y < 0)return n;
	auto cr = (VkvgContext)ctx;
	vkvg_set_fill_rule(cr, VKVG_FILL_RULE_EVEN_ODD);
	if (p->scale_pos > 0)
	{
		auto sp = p->scale_pos;
		vkvg_translate(cr, p->pos.x * sp, p->pos.y * sp);
		vkvg_translate(cr, p->view_pos.x, p->view_pos.y);
		for (size_t i = 0; i < p->count; i++)
		{
			auto pt = p->points[i];
			if (pt.y > 0)
			{
				vkvg_rectangle(cr, pt.x * sp, pt.y * sp, p->rc.x * sp, p->rc.y * sp);
			}
			else {
				vkvg_ellipse(cr, p->rc.x * sp, p->rc.x * sp, pt.x * sp, pt.y * sp, 0);
			}
		}
	}
	else
	{
		vkvg_translate(cr, p->pos.x, p->pos.y);
		vkvg_translate(cr, p->view_pos.x, p->view_pos.y);
		for (size_t i = 0; i < p->count; i++)
		{
			auto pt = p->points[i];
			if (pt.y > 0)
			{
				vkvg_rectangle(cr, pt.x, pt.y, p->rc.x, p->rc.y);
			}
			else {
				vkvg_ellipse(cr, p->rc.x, p->rc.x, pt.x, pt.y, 0);
			}
		}
	}
	//submit_style(cr, style);
	//vkvg_restore(cr);
	return n;
}

size_t cmd_op_draw_path(uint8_t* d, VkvgContext ctx)
{
	path_d p = *(path_d*)d;
	auto path = (path_d*)&p;
	p.v = (path_vertex_t*)(d + sizeof(path_d));
	size_t n = sizeof(path_d) + sizeof(path_vertex_t) * path->count;
	if (!ctx || !path || path->count == 0 || !path->v)return n;
	auto cr = (VkvgContext)ctx;
	auto t = path->v;
	bool stroke = false;
	//vkvg_save(cr);
	vkvg_set_fill_rule(cr, VKVG_FILL_RULE_EVEN_ODD);
	glm::vec2 pos, scale = {};
	pos += path->pos;
	if (path->flip_y)
	{
		vkvg_matrix_t flip_y = {};
		vkvg_matrix_init(&flip_y, 1, 0, 0, -1, 0, 0); // 垂直翻转
		vkvg_set_matrix(cr, &flip_y);
		pos.y = -pos.y;
	}
	vkvg_translate(cr, pos.x, pos.y);
	if (path->scale > 0)
	{
		vkvg_scale(cr, path->scale, path->scale);
	}
	float scale_pos = 1.0;
	auto mt = *t;
	auto xt = *t;
	if (path->scale_pos > 0)
	{
		scale_pos = path->scale_pos;
		for (size_t i = 0; i < path->count; i++, t++)
		{
			switch ((vtype_e1)t->type)
			{
			case vtype_e1::e_vmove:
			{
				if (i > 0)
				{
					if (xt.p.x == mt.p.x && xt.p.y == mt.p.y)
						vkvg_close_path(cr);
				}
				mt = *t;
				vkvg_move_to(cr, t->p.x * scale_pos, t->p.y * scale_pos);
			}break;
			case vtype_e1::e_vline:
			{
				vkvg_line_to(cr, t->p.x * scale_pos, t->p.y * scale_pos);
			}break;
			case vtype_e1::e_vcurve:
			{
				vkvg_quadratic_to(cr, t->c.x * scale_pos, t->c.y * scale_pos, t->p.x * scale_pos, t->p.y * scale_pos);
			}break;
			case vtype_e1::e_vcubic:
			{
				vkvg_curve_to(cr, t->c.x * scale_pos, t->c.y * scale_pos, t->c1.x * scale_pos, t->c1.y * scale_pos, t->p.x * scale_pos, t->p.y * scale_pos);

			}break;
			default:
				break;
			}
			xt = *t;
		}
	}
	else
	{
		for (size_t i = 0; i < path->count; i++, t++)
		{
			switch ((vtype_e1)t->type)
			{
			case vtype_e1::e_vmove:
			{
				if (i > 0)
				{
					if (xt.p.x == mt.p.x && xt.p.y == mt.p.y)
						vkvg_close_path(cr);
				}
				mt = *t;
				vkvg_move_to(cr, t->p.x, t->p.y);
			}break;
			case vtype_e1::e_vline:
			{
				vkvg_line_to(cr, t->p.x, t->p.y);
			}break;
			case vtype_e1::e_vcurve:
			{
				//static double dv = 2.0 / 3.0;
				//auto p0 = glm::vec2(xt.p.x, xt.p.y);
				//auto p1 = glm::vec2(t->c.x, t->c.y);
				//auto p2 = glm::vec2(t->p.x, t->p.y);
				//glm::vec2 c1, c2;
				//c1 = p1 - p0; c1 *= dv; c1 += p0;
				//c2 = p1 - p2; c2 *= dv; c2 += p2;
				////	C0 = Q0
				////	C1 = Q0 + (2 / 3) (Q1 - Q0)
				////	C2 = Q2 + (2 / 3) (Q1 - Q2)
				////	C3 = Q2
				//vkvg_curve_to(cr, c1.x, c1.y, c2.x, c2.y, t->p.x, t->p.y);
				vkvg_quadratic_to(cr, t->c.x, t->c.y, t->p.x, t->p.y);
			}break;
			case vtype_e1::e_vcubic:
			{
				vkvg_curve_to(cr, t->c.x, t->c.y, t->c1.x, t->c1.y, t->p.x, t->p.y);

			}break;
			default:
				break;
			}
			xt = *t;
		}
	}
	if (path->count > 2)
	{
		if (xt.p.x == mt.p.x && xt.p.y == mt.p.y)
			vkvg_close_path(cr);
	}
	//submit_style(cr, style);
	//vkvg_restore(cr);
	return n;
}

size_t cmd_op_add_line_ptr(uint8_t* d, VkvgContext ctx)
{
	glm::vec4* p = (glm::vec4*)(d + sizeof(size_t));
	size_t count = *((size_t*)d);
	size_t n = sizeof(glm::vec4) * count + sizeof(size_t);
	if (!p || count == 0)return n;
	auto t = p;
	for (size_t i = 0; i < count; i++)
	{
		vkvg_move_to(ctx, t->x, t->y);
		vkvg_line_to(ctx, t->z, t->w);
		t++;
	}
	return n;
}

//void rvg_cx::add_line(const glm::vec2& ps0, const glm::vec2& ps1)
//{
//	vkvg_move_to(ctx, ps0.x, ps0.y);
//	vkvg_line_to(ctx, ps1.x, ps1.y);
//}

size_t cmd_op_add_rect_double(uint8_t* d, VkvgContext ctx)
{
	glm::vec4* rc = (glm::vec4*)d;
	double r = *(double*)(d + sizeof(glm::vec4));
	draw_rectangle(ctx, *rc, r);
	return sizeof(glm::vec4) + sizeof(double);
}

// r，左右下左
size_t cmd_op_add_rect_vec4(uint8_t* d, VkvgContext ctx) {

	glm::vec4 rc = *(glm::vec4*)d;
	glm::vec4 r = *(glm::vec4*)(d + sizeof(glm::vec4));
	auto cr = ctx;
	auto x = rc.x;
	auto y = rc.y;
	auto width = rc.z;
	auto height = rc.w;
	vkvg_move_to(cr, x + r.x, y);
	vkvg_line_to(cr, x + width - r.y, y);
	if (r.y)
		vkvg_arc(cr, x + width - r.y, y + r.y, r.y, 3 * M_PI / 2, 2 * M_PI);
	vkvg_line_to(cr, x + width, y + height - r.z);
	if (r.z)
		vkvg_arc(cr, x + width - r.z, y + height - r.z, r.z, 0, M_PI / 2);
	vkvg_line_to(cr, x + r.w, y + height);
	if (r.w)
		vkvg_arc(cr, x + r.w, y + height - r.w, r.w, M_PI / 2, M_PI);
	vkvg_line_to(cr, x, y + r.x);
	if (r.x > 0)
		vkvg_arc(cr, x + r.x, y + r.x, r.x, M_PI, 3 * M_PI / 2.0);
	vkvg_close_path(cr);
	return sizeof(glm::vec4) * 2;
}
size_t cmd_op_add_circle(uint8_t* d, VkvgContext ctx)
{
	glm::vec2* pos = (glm::vec2*)d;
	float r = *(float*)(pos + 1);
	vkvg_ellipse(ctx, r, r, pos->x, pos->y, 0);
	//vkvg_arc(ctx, pos->x, pos->y, r, 0, 2 * M_PI);
	return sizeof(glm::vec2) + sizeof(float);
}
size_t cmd_op_add_ellipse(uint8_t* d, VkvgContext ctx)
{
	glm::vec2* pos = (glm::vec2*)d;
	glm::vec2* r = (glm::vec2*)(pos + 1);
	float rotationAngle = *(float*)(r + 1);
	vkvg_ellipse(ctx, r->x, r->y, pos->x, pos->y, rotationAngle);
	return sizeof(glm::vec2) * 2 + sizeof(float);
}



template<class T>
T next_value(uint8_t*& d) {
	auto p = (T*)d;
	d += sizeof(T);
	return *p;
}

template<class T>
T* next_value_p(uint8_t*& d) {
	T* p = (T*)d;
	d += sizeof(T);
	return p;
}
size_t cmd_op_add_triangle(uint8_t* d, VkvgContext ctx)
{
	glm::vec2 pos = next_value<glm::vec2>(d);
	glm::vec2 size = next_value<glm::vec2>(d);
	glm::vec2 dirspos = next_value<glm::vec2>(d);

	glm::vec2 tpos[3] = {};
	float df = dirspos.y;
	switch ((int)dirspos.x)
	{
	case 0:
	{
		tpos[0] = { size.x * df, 0 };
		tpos[1] = { size.x, size.y };
		tpos[2] = { 0, size.y };
	}
	break;
	case 1:
	{
		tpos[0] = { size.x, size.y * df };
		tpos[1] = { 0, size.y };
		tpos[2] = { 0, 0 };
	}
	break;
	case 2:
	{
		tpos[0] = { size.x * df, size.y };
		tpos[1] = { 0, 0 };
		tpos[2] = { size.x, 0 };
	}
	break;
	case 3:
	{
		tpos[0] = { 0, size.y * df };
		tpos[1] = { size.x, 0 };
		tpos[2] = { size.x, size.y };
	}
	break;
	default:
		break;
	}
	vkvg_move_to(ctx, pos.x + tpos[0].x, pos.y + tpos[0].y);
	vkvg_line_to(ctx, pos.x + tpos[1].x, pos.y + tpos[1].y);
	vkvg_line_to(ctx, pos.x + tpos[2].x, pos.y + tpos[2].y);
	vkvg_close_path(ctx);
	return sizeof(glm::vec2) * 3;
}



size_t cmd_op_polyline_vec2(uint8_t* d, VkvgContext ctx)
{
	polyline_r t = next_value<polyline_r>(d);
	glm::vec2* points = (glm::vec2*)d;
	int points_count = t.points_count;
	uint32_t col = t.col;
	auto cr = ctx;
	size_t n = sizeof(polyline_r) + sizeof(glm::vec2) * points_count;
	if (!cr || !points || points_count < 2 || !col)return n;
	vkvg_save(cr);
	vkvg_translate(cr, t.pos.x, t.pos.y);
	vkvg_move_to(cr, points->x, points->y);
	for (size_t i = 1; i < points_count; i++)
	{
		vkvg_line_to(cr, points[i].x, points[i].y);
	}
	if (t.closed) { vkvg_close_path(cr); }
	vkvg_set_line_width(cr, t.thickness);
	if (col) {
		set_color(cr, col);
		vkvg_stroke(cr);
	}
	vkvg_restore(cr);
	return n;
}

size_t cmd_op_add_polyline_path(uint8_t* d, VkvgContext ctx)
{
	size_t n = 0;
	auto f = d;
	auto t = next_value<polyline_pd>(d);
	auto cr = ctx;
	if (!cr || t.count < 1)return d - f;
	for (size_t i = 0; i < t.count; i++)
	{
		auto points_count = next_value<size_t>(d);
		auto points = (glm::vec2*)d;
		vkvg_move_to(cr, points[0].x, points[0].y);
		for (size_t i = 1; i < points_count; i++)
		{
			vkvg_line_to(cr, points[i].x, points[i].y);
		}
		if (t.closed) { vkvg_close_path(cr); }
		d = (uint8_t*)(points + points_count);
	}
	return d - f;
}


size_t cmd_op_add_polyline_vec2_ptr(uint8_t* d, VkvgContext ctx)
{
	auto f = d;
	int count = next_value<size_t>(d);
	glm::vec2* p = (glm::vec2*)d;
	if (!p || count == 0)return d - f;
	auto t = p;
	vkvg_move_to(ctx, t->x, t->y);
	t++;
	for (size_t i = 1; i < count; i++)
	{
		vkvg_line_to(ctx, t->x, t->y);
		t++;
	}
	d = (uint8_t*)t;
	return d - f;
}

size_t cmd_op_polylines(uint8_t* d, VkvgContext ctx)
//void rvg_cx::draw_polylines(const glm::vec2& pos, const , int points_count, , int idx_count, uint32_t col, float thickness)
{
	auto f = d;
	auto t = next_value<polyline_index_r>(d);
	glm::vec2* points = (glm::vec2*)d;
	int* idx = (int*)(points + t.points_count);
	d += sizeof(glm::vec2) * t.points_count + sizeof(int) * t.idx_count;
	auto cr = ctx;
	if (!cr || !points || t.points_count < 2 || t.idx_count < 2 || !t.col)return d - f;
	vkvg_save(cr);
	vkvg_translate(cr, t.pos.x, t.pos.y);
	vkvg_set_line_width(cr, t.thickness);
	int nc = 0;
	for (size_t i = 0; i < t.idx_count; i++)
	{
		auto x = idx[i];
		if (x < 0)
		{
			if (nc > 1) {
				set_color(cr, t.col);
				vkvg_stroke(cr);
			}
			nc = 0;
			continue;
		}
		if (nc == 0)
			vkvg_move_to(cr, points[x].x, points[x].y);
		else
			vkvg_line_to(cr, points[x].x, points[x].y);
		nc++;
	}
	if (nc > 1) {
		set_color(cr, t.col);
		vkvg_stroke(cr);
	}
	vkvg_restore(cr);
	return d - f;
}

size_t cmd_op_paint_shadow(uint8_t* d, VkvgContext ctx)
{
	auto p = next_value_p<paint_shadow_d>(d);
	paint_shadow_v(ctx, p->size.x, p->size.y, p->dst_size.x, p->dst_size.y, p->shadow, p->color_to, p->rev, p->r);
	return sizeof(paint_shadow_d);
}
size_t cmd_op_translate(uint8_t* d, VkvgContext ctx)
{
	auto offset = next_value<glm::vec2>(d);
	vkvg_translate(ctx, offset.x, offset.y);
	return sizeof(glm::vec2);
}
size_t cmd_op_scale(uint8_t* d, VkvgContext ctx)//; (float sx, float sy);
{
	auto sc = next_value<glm::vec2>(d);
	vkvg_scale(ctx, sc.x, sc.y);
	return sizeof(glm::vec2);
}
size_t cmd_op_rotate(uint8_t* d, VkvgContext ctx)//(float radians);
{
	auto r = next_value<float>(d);
	vkvg_rotate(ctx, r);
	return sizeof(float);
}
size_t cmd_op_transform(uint8_t* d, VkvgContext ctx)//(const glm::mat3x2* matrix);
{
	vkvg_transform(ctx, (vkvg_matrix_t*)d);
	return sizeof(glm::mat3x2);
}
size_t cmd_op_set_matrix(uint8_t* d, VkvgContext ctx)//(const glm::mat3x2* matrix);
{
	vkvg_set_matrix(ctx, (vkvg_matrix_t*)d);
	return sizeof(glm::mat3x2);
}


size_t cmd_op_clip(uint8_t* d, VkvgContext ctx)
{
	vkvg_clip(ctx);
	return 0;
}
size_t cmd_op_save(uint8_t* d, VkvgContext ctx)
{
	vkvg_save(ctx);
	return 0;
}
size_t cmd_op_restore(uint8_t* d, VkvgContext ctx)
{
	vkvg_restore(ctx);
	return 0;
}
size_t cmd_op_fill(uint8_t* d, VkvgContext ctx)
{
	vkvg_fill(ctx);
	return 0;
}
size_t cmd_op_stroke(uint8_t* d, VkvgContext ctx)
{
	vkvg_stroke(ctx);
	return 0;
}
size_t cmd_op_fill_preserve(uint8_t* d, VkvgContext ctx)
{
	vkvg_fill_preserve(ctx);
	return 0;
}
size_t cmd_op_stroke_preserve(uint8_t* d, VkvgContext ctx)
{
	vkvg_stroke_preserve(ctx);
	return 0;
}
size_t cmd_op_set_line_width(uint8_t* d, VkvgContext ctx)
{
	auto w = next_value<float>(d);
	vkvg_set_line_width(ctx, w);
	return sizeof(float);
}
size_t cmd_op_set_color_uint(uint8_t* d, VkvgContext ctx)
{
	uint32_t color = next_value<uint32_t>(d);
	vkvg_set_source_rgba(ctx, SP_RGBA32_R_F(color),
		SP_RGBA32_G_F(color),
		SP_RGBA32_B_F(color),
		SP_RGBA32_A_F(color));
	return sizeof(uint32_t);
}
size_t cmd_op_set_color_vec4(uint8_t* d, VkvgContext ctx)
{
	auto rgba = next_value<glm::vec4>(d);
	vkvg_set_source_rgba(ctx, rgba.x, rgba.y, rgba.z, rgba.w);
	return sizeof(glm::vec4);
}
size_t cmd_op_text_style(uint8_t* d, void* ctx)
{
	auto pc = (multi_rich_text_t*)ctx;
	text_style t = next_value<text_style>(d);
	rt_text_style_ts((rich_text_t*)ctx, &t);
	return sizeof(text_style);
}
//void rvg_cx::add_text(text_st* p, size_t count, text_style* ts)
size_t cmd_op_add_text(uint8_t* d, void* ctx)
{
	auto pc = (multi_rich_text_t*)ctx;
	auto f = d;
	text_st t = next_value<text_st>(d);
	auto idx = mrt_add_box(pc, t.pos, t.size, t.clip);
	char* str = 0;
	auto pb = mrt_get_boxinfo(pc, idx);

	if (t.text)
		str = (char*)d;
	mrt_add_text(pc, idx, str, t.text_len, 0, 0);
	//std::string kk(str, t.text_len);
	//printf("%s\t%p\n", kk.c_str(), (void*)pc->rich._ct_style.color);
	return (d - f) + t.text_len;
}
// todo clip
size_t cmd_op_add_image(uint8_t* d, void* ctx)
{
	auto pc = (multi_rich_text_t*)ctx;
	auto f = d;
	image_r t = next_value<image_r>(d);
	auto idx = mrt_add_box(pc, {}, t.dsize, {});
	if (t.is_surf) {
		mrt_add_image_vg(pc, idx, t.img, t.rc, t.sliced, t.color, t.dsize, t.pos, true);
	}
	else
	{
		mrt_add_image(pc, idx, t.img, t.rc, t.sliced, t.color, t.dsize, t.pos, true);
	}
	return d - f;
}
size_t cmd_op_add_geometry(uint8_t* d, void* ctx)
{
	auto f = (geometry_d*)d;

	return sizeof(geometry_d);
}

size_t cmd_op_view(uint8_t* d, VkvgContext ctx)
{

	return 0;
}

size_t cmd_op_view_pop(uint8_t* d, VkvgContext ctx)
{

	return 0;
}

typedef size_t(*cmd_func_type)(uint8_t* d, VkvgContext ctx);
size_t call_cmd_func(uint8_t c, uint8_t* d, void* ctx)
{
	//OP_SUBMIT_STYLE, OP_SUBMIT_COLOR, OP_GRID_FILL, OP_LINEAR_FILL, OP_ADD_ARROW,
	//	OP_DRAW_BLOCK, OP_DRAW_PATH, OP_ADD_LINE_PTR, OP_ADD_RECT_DOUBLE,
	//	OP_ADD_RECT_VEC4, OP_ADD_CIRCLE, OP_ADD_ELLIPSE, OP_ADD_TRIANGLE, OP_POLYLINE_VEC2,
	//	OP_ADD_POLYLINE_PATH, OP_ADD_POLYLINE_VEC2_PTR, OP_POLYLINES,
	//	OP_PAINT_SHADOW, OP_TRANSLATE, OP_CLIP, OP_SAVE, OP_RESTORE, OP_FILL, OP_STROKE, OP_FILL_PRESERVE, OP_STROKE_PRESERVE,
	//	OP_SET_LINE_WIDTH, OP_SET_COLOR_UINT, OP_SET_COLOR_VEC4,
	//	OP_TEXT_STYLE, OP_ADD_TEXT, OP_ADD_IMAGE,OP_ADD_GEOMETRY
	static cmd_func_type cbs[] = { nullptr,cmd_op_view,cmd_op_view_pop, cmd_op_submit_style, cmd_op_submit_color, cmd_op_grid_fill, cmd_op_linear_fill, cmd_op_add_arrow,
		cmd_op_draw_block, cmd_op_draw_path, cmd_op_add_line_ptr, cmd_op_add_rect_double,
		cmd_op_add_rect_vec4, cmd_op_add_circle, cmd_op_add_ellipse, cmd_op_add_triangle, cmd_op_polyline_vec2,
		cmd_op_add_polyline_path, cmd_op_add_polyline_vec2_ptr, cmd_op_polylines,
		cmd_op_paint_shadow, cmd_op_translate,cmd_op_scale, cmd_op_rotate, cmd_op_transform, cmd_op_set_matrix,
		cmd_op_clip, cmd_op_save, cmd_op_restore, cmd_op_fill, cmd_op_stroke, cmd_op_fill_preserve, cmd_op_stroke_preserve,
		cmd_op_set_line_width, cmd_op_set_color_uint, cmd_op_set_color_vec4 ,
		 (cmd_func_type)cmd_op_text_style, (cmd_func_type)cmd_op_add_text,(cmd_func_type)cmd_op_add_image, (cmd_func_type)cmd_op_add_geometry, };
	size_t r = 0;
	if (c > 0 && c < rvg_cx::OP_MAX_COUNT)
	{
		auto cb = cbs[c];
		r = cb ? cb(d, (VkvgContext)ctx) : 0;
	}
	return r;
}

#endif

#if 0
struct gradient_btn_t
{
	glm::ivec2 pos = {}, size = {};
	std::string str;
	uint32_t back_color = 0xff000000;
	uint32_t text_color = -1;
	uint32_t text_color_shadow = 0x88111111;
	double opacity = 1.0;
	// private
	uint32_t gradTop = 0;
	uint32_t gradBot = 0;
	uint32_t borderLight = 0xff5c5c5c;
	uint32_t borderDark = 0xff1d1d1d;
	uTheme effect = uTheme::light;	// dark
	int rounding = 4;
	int thickness = 1;

	int bst = 1;				// 鼠标状态
	int _old_bst = 0;			// 鼠标状态
	int cks = 0;				// 鼠标点击状态

	bool mPushed = false;
	bool mChecked = false;
	bool mMouseFocus = false;
	bool mEnabled = true;
	bool is_muilt = true;
};

bool gradient_btn_update(gradient_btn_t* p, float delta);
void gradient_btn_draw(VkvgContext cr, gradient_btn_t* p);
bool gradient_btn_update(gradient_btn_t* p, float delta)
{
	if (!p)return false;
	if (p->bst == p->_old_bst)return false;
	p->_old_bst = p->bst;
	auto& info = *p;

	// (sta & hz::BTN_STATE::STATE_FOCUS)
	uint32_t gradTop = 0xff4a4a4a;
	uint32_t gradBot = 0xff3a3a3a;

	info.mPushed = (p->bst & (int)BTN_STATE::STATE_ACTIVE);
	info.mMouseFocus = (p->bst & (int)BTN_STATE::STATE_HOVER);
	if (p->bst & (int)BTN_STATE::STATE_DISABLE)
		info.mEnabled = false;
	if (info.mPushed) {
		gradTop = 0xff292929;
		gradBot = 0xff1d1d1d;
	}
	else if (info.mMouseFocus && info.mEnabled) {
		gradTop = 0x80404040;
		gradBot = 0x80303030;
	}
	info.gradTop = gradTop;
	info.gradBot = gradBot;
	return true;
}
void gradient_btn_draw(VkvgContext cr, gradient_btn_t* p)
{
	float x = p->pos.x, y = p->pos.y, w = p->size.x, h = p->size.y;
	int pushed = p->mPushed ? 0 : 1;
	uint32_t gradTop = p->gradTop;
	uint32_t gradBot = p->gradBot;
	uint32_t borderDark = p->borderDark;
	uint32_t borderLight = p->borderLight;
	double oa = p->opacity;
	auto ns = p->size;
	auto bc = p->effect == uTheme::light ? p->back_color : set_alpha_xf2(p->back_color, get_alpha_f(p->back_color));
	double rounding = p->rounding;
	glm::vec2 ns1 = { w * 0.5, h * 0.5 };
	auto nr = (int)std::min(ns1.x, ns1.y);
	if (rounding > nr)
	{
		rounding = nr;
	}
	vkvg_save(cr);
	vkvg_translate(cr, x, y);
	if (is_alpha(bc))
	{
		bc = set_alpha_f(bc, oa);
		glm::vec4 rcc = { p->thickness,p->thickness, w - p->thickness, h - p->thickness * 2 };
		draw_rectangle(cr, rcc, rounding);
		set_color(cr, bc);
		vkvg_fill(cr);
	}
	if (p->mPushed) {
		gradTop = set_alpha_f(gradTop, 0.8f);
		gradBot = set_alpha_f(gradBot, 0.8f);
	}
	else {
		double v = 1 - get_alpha_f(p->back_color);
		auto gv = p->mEnabled ? v : v * .5f + .5f;
		gradTop = set_alpha_xf(gradTop, gv);
		gradBot = set_alpha_xf(gradBot, gv);
	}
	auto gt = to_c4(gradTop);
	auto gt1 = to_c4(gradBot);
	gradTop = set_alpha_xf(gradTop, oa);
	gradBot = set_alpha_xf(gradBot, oa);
	borderLight = set_alpha_xf(borderLight, oa);
	borderDark = set_alpha_xf(borderDark, oa);
	// 渐变
	glm::vec4 r;
	if (rounding > 0)
	{
		r = { rounding, rounding, rounding, rounding };
	}
	glm::vec2 rct = { w - p->thickness, h - p->thickness * 2 };
	glm::vec4 gtop = to_c4(gradTop);
	glm::vec4 gbot = to_c4(gradBot);

	vkvg_save(cr);
	if (p->effect == uTheme::dark)
		vkvg_translate(cr, p->thickness, p->thickness);
	else if (p->mPushed)
		vkvg_translate(cr, p->thickness, p->thickness);
	paint_shadow_v(cr, 0, rct.y, rct.x, rct.y, gtop, gbot, 0, rounding);// 垂直方向
	vkvg_restore(cr);
	// 渲染标签
	glm::vec2 ps = { p->thickness * 2, p->thickness * 2 };
	if (p->mPushed) {
		ps += p->thickness;
	}
	ns -= p->thickness * 4;
	glm::vec4 rc = { ps, ns };

#if 0
	text_style_t st = {};
	st.font = 0;
	st.text_align = p->text_align;
	st.font_size = p->font_size;
	st.text_color = p->text_color;
	st.shadow_pos = { thickness, thickness };
	st.text_color_shadow = text_color_shadow;
	{
		int font = 0;
		glm::vec2 text_align = { 0.0,0.5 };
		glm::vec2 shadow_pos = { 1.0,1.0 };
		int font_size = 18;
		uint32_t text_color = 0xffffffff;
		uint32_t text_color_shadow = 0;
	};
	draw_text(g, ltx, p->str.c_str(), -1, rc, &st);



	ltx->tem_rtv.clear();
	ltx->build_text(0, rc, text_align, p->str.c_str(), -1, p->font_size, ltx->tem_rtv);
	ltx->update_text();
	if (text_color_shadow)
	{
		vkvg_as _aa_(g);
		vkvg_translate(g, thickness, thickness);
		ltx->draw_text(g, ltx->tem_rtv, text_color_shadow);
	}
	ltx->draw_text(g, ltx->tem_rtv, p->text_color);

#endif // 1
	// 边框
	vkvg_set_line_width(cr, p->thickness);
	set_color(cr, borderLight);
	w -= 1;
	h -= 1;
	glm::vec2 tps = { 0.5,0.5 };
	draw_rectangle(cr, { tps.x,tps.y + (p->mPushed ? 0.f : 1.0f), w, h - (p->mPushed ? 0.0f : 1.0f) }, rounding);
	vkvg_stroke(cr);
	set_color(cr, borderDark);
	draw_rectangle(cr, { tps.x,tps.y, w , h }, rounding);
	vkvg_stroke(cr);
	vkvg_restore(cr);
}
#endif

rvg_data_cx::rvg_data_cx()
{
	ac = new hz::usp_ac();
	packer = new_packer(10, 10);
	mrt = new multi_rich_text_t();
	d = new rvg_cx();
}

rvg_data_cx::~rvg_data_cx()
{
	if (mrt)
	{
		delete mrt;
		mrt = 0;
	}
	if (packer)
	{
		delete packer;
		packer = 0;
	}
	if (d)delete d;
	d = 0;
	if (ac)
	{
		delete ac;
		ac = 0;
	}
}

rvg_cx* rvg_data_cx::get()
{
	return d;
}
//	if (it.type == 0 && it.type == f.type && check_rect_cross(f.rc, it.rc))
//	{
//		f.rc.x = std::min(f.rc.x, c.x);
//		f.rc.y = std::min(f.rc.y, c.y);
//		f.rc.z = std::max(f.rc.z, c.z);
//		f.rc.w = std::max(f.rc.w, c.w);
//		f.second = it.second;
//	} 
int rvg_data_cx::update()
{
	auto rvg = d;
	int sf = 0;
	dcv.clear();
	auto cmd_count = rvg->_cmdtype.size();
	size_t gdx = 0;
	auto cmd = rvg->_cmd.data();
	auto cmdc = rvg->_cmd.size();
	auto dt = rvg->_cmdtype.data();
	auto dtc = rvg->_cmdtype.size();
	auto cpos = rvg->_cmd_pos.data();
	size_t upinc = 0;
	auto rcrc = rvg->get_crc();
	if (cmd_crc != rcrc)
	{
		upinc++;
		cmd_crc = rcrc;
	}
	//printf("crc\t%d\n", rcrc);
	// 计算每个渲染区，取偶数大小
	for (auto& it : rvg->_view)
	{
		auto rc = it.rc;
		uint32_t c = get_crc(&it, (char*)cmd, cmdc, (char*)dt, dtc, cpos);
		it.dcv_index = -1;
		if (rc.z < 1 || rc.w < 1)continue;
		d2_rt d = {};
		d.size = { rc.z, rc.w };
		d.size += stwidth * 2;
		d.size.x = align_up(d.size.x, 4);
		d.size.y = align_up(d.size.y, 4);
		d.size.x = std::min(_view.z, d.size.x);
		d.size.y = std::min(_view.w, d.size.y);
		d.offset = glm::ivec2(rc.x, rc.y);
		d.crc = c;
		d.up = 1;
		max_rect.x = std::max(max_rect.x, d.size.x);
		max_rect.y = std::max(max_rect.y, d.size.y);
		it.dcv_index = dcv.size();
		if (dcv_p) {
			if (d.up = dcv_p[it.dcv_index].crc != c);
		}
		if (d.up)upinc++;
		dcv.push_back(d);
	}
	//upinc++;
	if (dcv_p && upinc == 0)
	{
		return 0;
	}
	auto dcp = dcv.data();
	size_t dcc = dcv.size();
	if (dcc > dcv_ac)
	{
		auto np = ac->new_mem<d2_rt>(dcc);
		if (np) {
			ac->free_mem(dcv_p, dcv_ac);
			dcv_p = np;
			dcv_ac = dcc;
		}
		else {
			assert(0);
			return 0;
		}
	}
	if (!dcv_p)
		return 0;
	//auto ddp = ac->new_mem<gdata_ptr>(dcc);
	dcv_c = dcc;
	up = true;
	// 清空所有画布装箱
	packer->init_target(_view.z, _view.w, 0);
	do
	{
		// 渲染区装箱
		auto ab = packer->push_rect((glm::ivec4*)dcp, 1, sizeof(d2_rt));
		if (ab)
		{
			packer->clear();
			sf++; // 满了重新用一张新纹理
		}
		else {
			dcp->surface = sf;
			dcp++;
			dcc--;
		}
	} while (dcc > 0);
	memcpy(dcv_p, dcv.data(), sizeof(d2_rt) * dcv.size());
	surfaces.resize(sf + 1);
	return 1;
}
translate_cc rvg_data_cx::get_ctx(size_t idx, const glm::ivec4& rc)
{
	translate_cc r = {};
	if (idx < dcv.size())
	{
		auto& rcc = dcv[idx];
		if (rc.z > 0 && rc.w > 0)
		{
			dst_data.push_back({});
			auto& v = dst_data.back();
			if (!v.v.d2)
			{
				v.v.d2 = dcv_p;
				v.raw_index = idx;
				v.type = 1;
			}
		}
		r.clips = rcc.size;
		r.apos = rcc.pos - rcc.offset;
		r.apos += stwidth;
		r.surface = surfaces[rcc.surface].surface;
		r.ctx = surfaces[rcc.surface].ctx;
	}
	return r;
}

// 组装渲染数据
void build_vg(rvg_data_cx* dst, drawable_cx* dra)
{
	auto rvg = dst->get();
	if (!dst->up)return;

	dst->mrt->rich._ct_style.family = dra->familys;
	dst->up = false;// 全部更新	
	mrt_clear(dst->mrt);
	dst->dst_data.clear();
	bool first = false;
	for (auto& it : dst->surfaces)
	{
		if (!it.surface)
		{
			it.surface = dra->new_surface(dra->_view.z, dra->_view.w);
			first = true;
		}
		auto ctx = ctx_begin(it.surface);
		if (ctx)
		{
			it.ctx = ctx;
			vkvg_clear((VkvgContext)ctx);
			//ctx_end(ctx);
		}
	}
	auto d = rvg->_cmd.data();
	auto didx = rvg->_cmd_pos.data();
	size_t ps = 0;
	size_t dd_count = 0;
	size_t rs = 0;
	std::vector<size_t> rsv;
	//glm::ivec3 tcount = {};
	size_t xk = 0;
	for (auto ct : rvg->_cmdtype) {
		if (ct == rvg_cx::OP_RESTORE)
		{
			rs++; rsv.push_back(xk);
		}
		if (ct == rvg_cx::OP_VIEW || ct == rvg_cx::OP_ADD_TEXT || ct == rvg_cx::OP_ADD_IMAGE || ct == rvg_cx::OP_ADD_GEOMETRY) {
			dd_count++;
		}
		xk++;
	}
	auto pct = rvg->_cmdtype.data();
	auto length = rvg->_cmdtype.size();

	size_t vidx = 0;
	std::stack<translate_cc> stt = {};
	translate_cc tcc = {};
	glm::ivec2 lpos = {};
	for (size_t i = 0; i < length; i++)
	{
		auto ct = pct[i];
		if (ct == rvg_cx::OP_VIEW) {
			auto vv = rvg->_view[vidx];
			if (vv.dcv_index < dst->dcv.size()) {
				tcc = dst->get_ctx(vv.dcv_index, vv.rc);
				auto& dstv = dst->dst_data.back();
				dstv.view = vv;
				lpos = vv.pos;
				//tcc.ctx = ctx_begin(tcc.surface);
				tcc.ptr = vv.ptr;
				if (tcc.ctx)
				{
					auto ctx = (VkvgContext)tcc.ctx;
					vkvg_save(ctx);
					vkvg_translate(ctx, tcc.apos.x, tcc.apos.y);
				}
				stt.push(tcc);
			}
			else {
			}
			vidx++;
			continue;
		}
		if (ct == rvg_cx::OP_VIEW_POP)
		{
			if (stt.size())
			{
				tcc = stt.top();
				stt.pop();
				if (tcc.ctx)
				{
					auto ctx = (VkvgContext)tcc.ctx;
					//vkvg_translate(ctx, -tcc.apos.x, -tcc.apos.y);
					vkvg_restore(ctx);
					//ctx_end(tcc.ctx);
				}
			}
			continue;
		}
		if (ct == rvg_cx::OP_ADD_TEXT || ct == rvg_cx::OP_ADD_IMAGE) {
			gdata_ptr vt = {};
			if (ct == rvg_cx::OP_ADD_IMAGE)
				vt.view.pos = lpos;
			vt.raw_index = mrt_box_count(dst->mrt);
			vt.v.t = (box_text_d*)1;
			vt.type = 0;
			vt.view.ptr = tcc.ptr;
			vt.view.pos = lpos;
			dst->dst_data.push_back(vt);
		}
		auto pss = didx[i];
		if (ct == rvg_cx::OP_ADD_GEOMETRY) {
			gdata_ptr vt = {};
			vt.raw_index = 0;
			vt.v.geo = (geometry_d*)(d + pss);
			vt.type = 2;
			dst->dst_data.push_back(vt);
			continue;
		}
		size_t n = call_cmd_func(ct, d + pss, ct < rvg_cx::OP_TEXT_STYLE ? tcc.ctx : dst->mrt);
	}
	assert(stt.empty());// 命令异常
	for (; stt.size();) {

		tcc = stt.top();
		stt.pop();
		if (tcc.ctx)
		{
			auto ctx = (VkvgContext)tcc.ctx;
			//vkvg_translate(ctx, -tcc.apos.x, -tcc.apos.y);
			vkvg_restore(ctx);
			//ctx_end(tcc.ctx);
		}
	}
	int ki = 0;
	for (auto& it : dst->surfaces)
	{
		if (it.surface)
		{
			ctx_end(it.ctx);
			vkvg_surface_resolve((VkvgSurface)it.surface);
			//if (first)
			//{
			//	std::string str = "temp/vgtest-" + std::to_string(ki++) + ".png";
			//	vkvg_surface_write_to_png((VkvgSurface)it.surface, str.c_str());
			//}
		}
		dra->update_surface((VkvgSurface)it.surface);
	}

	mrt_build(dst->mrt);

	dra->update_text((rich_text_t*)dst->mrt, 0);
	for (auto& it : dst->dst_data) {
		if (it.type == 0)
		{
			it.v.t = mrt_get_box_index(dst->mrt, (size_t)it.raw_index);
		}
	}
}

drawable_cx::drawable_cx()
{}

drawable_cx::~drawable_cx()
{
	free_tex();
	for (auto& [k, v] : _dobj) {
		free_rvg(v);
		delete v;
	}
	_dobj.clear();
}

void drawable_cx::init(texture_cb* cb, const glm::ivec4& view, vkvg_dev* vgdev)
{
	rcb = cb;
	if (view.z > 0 && view.w > 0)
	{
		_view = view;
	}
	if (vgdev) {
		_vgdev = vgdev;
		vgcb = vgdev->get_fun();
	}
}

void* drawable_cx::new_surface(int width, int height)
{
	auto dev = _vgdev;
	if (width > 1 && height > 1 && dev && dev->ctx && dev->ctx->dev)
	{
		auto surf = vkvg_surface_create(dev->ctx->dev, width, height);
		return (void*)surf;
	}
	return nullptr;
}

void* drawable_cx::new_surface1(int width, int height)
{
	auto dev = _vgdev;
	if (width > 1 && height > 1 && dev && dev->ctx1 && dev->ctx1->dev)
	{
		auto surf = vkvg_surface_create(dev->ctx1->dev, width, height);
		return (void*)surf;
	}
	return nullptr;
}

void drawable_cx::update_surface(void* surface)
{
	void* renderer = rptr;
	auto surf = (VkvgSurface)surface;
	if (!surface || !rcb || !renderer)return;
	auto& tp = _vgt[surface];
	if (!tp)
	{
		VkImage img = vkvg_surface_get_vk_image(surf);
		VkFormat fmt = vkvg_surface_get_vk_format(surf);
		uint32_t w = vkvg_surface_get_width(surf);
		uint32_t h = vkvg_surface_get_height(surf);
		auto t = rcb->new_texture_vk(renderer, w, h, img, fmt == VK_FORMAT_B8G8R8A8_UNORM ? 1 : 0);
		if (t && t != tp)
		{
			rcb->set_texture_blend(t, 0, true);
			tp = t;
		}
	}
}

void drawable_cx::update_image(image_ptr_t* img)
{
	if (img) {
		auto& tp = _vt[img];
		if (!tp)
		{
			tp = rcb->new_tex(rptr, img);
		}
		else if (img->valid)
		{
			rcb->update_texture(tp, 0, img->data, img->stride);
		}
	}
}

void drawable_cx::draw_surface(void* surface, const glm::vec2& pos, const glm::ivec4& rc, const glm::ivec2& size)
{
	void* renderer = rptr;
	if (!surface || !rcb || !renderer)return;
	auto& tp = _vgt[surface];
	if (tp)
	{
		texture_dt dt = {};
		dt.dst_rect = glm::vec4(pos, size);
		dt.src_rect = rc;
		rcb->render_texture(renderer, tp, &dt, 1);
	}
}

int drawable_cx::draw_rvg(rvg_data_cx* dst)
{
	int c = 0;
	void* renderer = rptr;
	if (!rcb || !renderer)return c;
	if (!dst || dst->dst_data.empty())return c;
	glm::vec2 pos0 = {};// dst->_pos;
	auto length = dst->dst_data.size();
	//int kc = 0;
	for (size_t m = 0; m < length; m++)
	{
		auto& vt = dst->dst_data[m];
		pos0 = vt.view.pos;
		switch (vt.type)
		{
		case 0:
			if (vt.v.t)
			{
				draw_boxtext(vt.v.t, pos0);
				c++;
				//kc++;
			}
			break;
		case 1:
			if (vt.v.d2)
			{
				auto it = vt.v.d2 + vt.raw_index;
				auto surface = dst->surfaces[it->surface].surface;
				auto& tp = _vgt[surface];
				if (tp)
				{
					texture_dt dt = {};
					auto ost = it->offset;
					ost += pos0;// dst->_pos;
					ost -= stwidth;
					auto ss = it->size;
					ss -= stwidth;
					dt.dst_rect = glm::vec4(ost, ss);
					auto pos = it->pos;
					pos += stwidth;
					dt.src_rect = glm::vec4(it->pos, ss);
					rcb->render_texture(renderer, tp, &dt, 1);
					c++;
				}
			}break;
		case 2:
			if (vt.v.geo) {
				c += draw_geometry(vt.v.geo);
			}
			break;
		};
	}
	//printf("kc\t%d\tdc:%d\n", kc, (int)length);
	return c;
}



void drawable_cx::update_text(rich_text_t* p, float delta)
{
	void* renderer = rptr;
	if (!p || !rcb || !renderer)return;
	auto& ov = p->layout.ov;
	for (auto& it : ov)
	{
		update_image(it);
	}
	for (auto t : p->layout.gv) {
		auto surface = (VkvgSurface)t;
		update_surface(surface);
	}
}

void drawable_cx::draw_textdata(rich_text_t* p, const glm::vec2& pos)
{
	void* renderer = rptr;
	if (!p || !rcb || !renderer)return;
	uint32_t color = -1;
	const int vsize = sizeof(text_vx);
	bool devrtex = !tex_batch && (rcb->set_texture_color4 && rcb->render_texture);
	auto rect = p->box.tbox.rc;
	glm::ivec4 rc = {};
	glm::ivec2 pos0 = {};
	void* tex = 0;
	rect.x += pos.x; rect.y += pos.y;
	pos0.x += rect.x; pos0.y += rect.y;
	auto& tm = p->layout.dst_vstr;
	auto tbp = p->tbs.data();
	clicprect_cx cp(renderer, rcb, rect); // 设置裁剪区域
	for (auto& vt : tm) {
		if (vt.i.ctype)
		{
			auto& it = *vt.i.ib;
			if (!it.img)continue;
			void* tp = get_texture(it.img);
			if (tex != tp)
			{
				submit_data(tex);
				tex = tp;
			}
			auto lpos = it.layout_pos + it.pos + pos0;
			auto c = it.color ? it.color : color;
			if (it.sliced.x > 0 || it.sliced.y > 0 || it.sliced.z > 0 || it.sliced.w > 0)
			{
				texture_9grid_dt t9 = {};
				t9.src_rect = it.rc;
				t9.dst_rect = glm::vec4(lpos, it.dsize);
				t9.scale = 0.0;
				t9.left_width = it.sliced.x, t9.right_width = it.sliced.y, t9.top_height = it.sliced.z, t9.bottom_height = it.sliced.w;
				t9.tileScale = -1; // 大于0则是9gridtiled中间平铺
				rcb->render_texture_9grid(renderer, tex, &t9, 1);
			}
			else
			{
				if (devrtex)
				{
					texture_dt p = {};
					p.dst_rect = glm::vec4(lpos, it.dsize);
					p.src_rect = it.rc;
					auto c4 = ucolor2fx(c);
					rcb->set_texture_color4(tex, &c4);
					rcb->render_texture(renderer, tex, &p, 1);
				}
				else
				{
					glm::ivec2 tex_size = {};
					if (it.is_surface)
					{
						tex_size.x = vkvg_surface_get_width((VkvgSurface)it.img);
						tex_size.y = vkvg_surface_get_height((VkvgSurface)it.img);
					}
					else { tex_size.x = it.img->width; tex_size.y = it.img->height; };
					gen3data(tex_size, lpos, it.rc, it.dsize, c, &opt, &idx);
					tex = tp;
				}
			}
		}
		else if (vt.f.ctype == 0) {
			auto& git = vt.f;
			if (git._image) {
				auto& tp = _vt[git._image];
				if (tex != tp)
				{
					submit_data(tex);
					tex = tp;
				}
				auto ps = git._dwpos + git._apos;
				ps += pos0;
				auto tstyle = tbp[git.tb_idx].style;
				color = tstyle.color;
				auto img = git._image;
				glm::ivec2 tex_size = { img->width,  img->height };
				if (!git.color || tstyle.mcolor_effect)
				{
					if (tstyle.color_shadow)
					{
						auto ps1 = ps;
						ps1 += tstyle.shadow_pos;	// 生成阴影数据
						gen3data(tex_size, ps1, git._rect, {}, tstyle.color_shadow, &opt, &idx);
					}
					if (tstyle.stroke && tstyle.color_stroke)
					{
						int pxx[4] = { -tstyle.stroke, 0, tstyle.stroke, 0 };
						int pyy[4] = { 0, -tstyle.stroke, 0, tstyle.stroke };
						for (int e = 0; e < 4; e++)
						{
							auto ps1 = ps;
							ps1.x += pxx[e];
							ps1.y += pyy[e];	// 生成描边数据
							gen3data(tex_size, ps1, git._rect, {}, tstyle.color_stroke, &opt, &idx);
						}
					}
				}
				gen3data(tex_size, ps, git._rect, {}, git.color ? git.color : color, &opt, &idx);
			}
		}
	}
	submit_data(tex);
}

void drawable_cx::draw_boxtext(box_text_d* p, const glm::vec2& pos)
{
	void* renderer = rptr;
	if (!p || !rcb || !renderer)return;
	uint32_t color = -1;
	const int vsize = sizeof(text_vx);
	bool devrtex = !tex_batch && (rcb->set_texture_color4 && rcb->render_texture);
	auto rect = p->view;
	glm::ivec4 rc = {};
	glm::ivec2 pos0 = {};
	void* tex = 0;
	rect.x += pos.x; rect.y += pos.y;
	pos0.x += rect.x; pos0.y += rect.y;
	auto clip = p->clip;
	if (clip.z > 0 && clip.w > 0) {
		clip.x += pos.x; clip.y += pos.y;
	}
	auto tm = p->d + p->first;
	auto tbp = p->tbs->data();
	clicprect_cx cp(renderer, rcb, clip); // 设置裁剪区域

	for (size_t i = 0; i < p->count; i++)
	{
		auto& vt = *(tm + i);
		if (vt.i.ctype)
		{
			auto& it = *vt.i.ib;
			if (!it.img)continue;
			void* tp = get_texture(it.img);
			if (tex != tp)
			{
				submit_data(tex);
				tex = tp;
			}
			auto lpos = it.layout_pos + it.pos + pos0;
			auto c = it.color ? it.color : color;
			if (it.sliced.x > 0 || it.sliced.y > 0 || it.sliced.z > 0 || it.sliced.w > 0)
			{
				texture_9grid_dt t9 = {};
				t9.src_rect = it.rc;
				t9.dst_rect = glm::vec4(lpos, it.dsize);
				t9.scale = 0.0;
				t9.left_width = it.sliced.x, t9.right_width = it.sliced.y, t9.top_height = it.sliced.z, t9.bottom_height = it.sliced.w;
				t9.tileScale = -1; // 大于0则是9gridtiled中间平铺
				rcb->render_texture_9grid(renderer, tex, &t9, 1);
			}
			else
			{
				if (devrtex)
				{
					texture_dt p = {};
					p.dst_rect = glm::vec4(lpos, it.dsize);
					p.src_rect = it.rc;
					auto c4 = ucolor2fx(c);
					rcb->set_texture_color4(tex, &c4);
					rcb->render_texture(renderer, tex, &p, 1);
				}
				else
				{
					glm::ivec2 tex_size = {};
					if (it.is_surface)
					{
						tex_size.x = vkvg_surface_get_width((VkvgSurface)it.img);
						tex_size.y = vkvg_surface_get_height((VkvgSurface)it.img);
					}
					else { tex_size.x = it.img->width; tex_size.y = it.img->height; };
					gen3data(tex_size, lpos, it.rc, it.dsize, c, &opt, &idx);
					tex = tp;
				}
			}
		}
		else if (vt.f.ctype == 0) {
			auto& git = vt.f;
			if (git._image) {
				auto& tp = _vt[git._image];
				if (tex != tp)
				{
					submit_data(tex);	// 提交合并的渲染数据
					tex = tp;
				}
				auto ps = git._dwpos + git._apos;
				ps += pos0;
				auto tstyle = tbp[git.tb_idx].style;
				color = tstyle.color;
				auto img = git._image;
				glm::ivec2 tex_size = { img->width,  img->height };
				if (!git.color || tstyle.mcolor_effect)
				{
					if (tstyle.color_shadow)
					{
						auto ps1 = ps;
						ps1 += tstyle.shadow_pos;	// 生成阴影数据
						gen3data(tex_size, ps1, git._rect, {}, tstyle.color_shadow, &opt, &idx);
					}
					if (tstyle.stroke && tstyle.color_stroke)
					{
						int pxx[4] = { -tstyle.stroke, 0, tstyle.stroke, 0 };
						int pyy[4] = { 0, -tstyle.stroke, 0, tstyle.stroke };
						for (int e = 0; e < 4; e++)
						{
							auto ps1 = ps;
							ps1.x += pxx[e];
							ps1.y += pyy[e];	// 生成描边数据
							gen3data(tex_size, ps1, git._rect, {}, tstyle.color_stroke, &opt, &idx);
						}
					}
				}
				// 生成文本数据
				gen3data(tex_size, ps, git._rect, {}, git.color ? git.color : color, &opt, &idx);
			}
		}
	}
	submit_data(tex);
}

int drawable_cx::draw_geometry(void* image, const glm::ivec4& clip, std::vector<text_vx>* v, std::vector<uint32_t>* index)
{
	const int vsize = sizeof(text_vx);
	auto nv = v->size();
	if (!rcb || !v || !index || v->empty() || index->empty())return 0;
	void* texp = get_texture(image);
	//rcb->set_viewport(rptr, &viewport);
	rcb->set_cliprect(rptr, &clip);
	rcb->draw_geometry(rptr, texp, (float*)&v->data()->pos, vsize, ((float*)&v->data()->color), vsize,
		((float*)&v->data()->uv), vsize, nv, index->data(), index->size(), sizeof(uint32_t));
	return 1;
}

int drawable_cx::draw_geometry(geometry_d* geo)
{
	const int vsize = sizeof(text_vx);
	auto nv = geo->vcount;
	if (!rcb || !geo->vdata || !geo->idx || geo->vcount < 3 || geo->icount < 3)return 0;
	void* texp = get_texture(geo->image);
	clicprect_cx cp(rptr, rcb, geo->clip);
	if (!geo->cmds || geo->cmd_count < 1)
	{
		rcb->draw_geometry(rptr, texp, (float*)geo->vdata, vsize, ((float*)&geo->vdata->color), vsize,
			((float*)&geo->vdata->uv), vsize, nv, geo->idx, geo->icount, sizeof(uint32_t));
	}
	else if (geo->cmds && geo->cmd_count > 0) {
		auto ct = geo->cmds;
		for (size_t i = 0; i < geo->cmd_count; i++)
		{
			//rcb->set_viewport(rptr, (ct->viewport.z > 0 && ct->viewport.w > 0) ? &ct->viewport : nullptr);
			rcb->set_cliprect(rptr, &ct->clip_rect);
			auto d = geo->vdata + ct->vtx_first;
			rcb->draw_geometry(rptr, ct->tex_ref, (float*)(d), vsize, (float*)(&d->color), vsize, (float*)(&d->uv), vsize, geo->vcount - ct->vtx_first, geo->idx + ct->idx_first, ct->elemCount, sizeof(uint32_t));
		}
	}
	return 1;
}


// todo 图集渲染
void drawable_cx::draw_mesh2ddata(mesh2d_cx* dc, const glm::vec2& render_scale)
{
	glm::vec2 clip_off = {};
	glm::vec2 clip_scale = render_scale;
	glm::ivec4 vp = { 0,0,-1,-1 };
	if (dc->viewport.z > 0 && dc->viewport.w > 0)
	{
		vp.x = dc->viewport.x;
		vp.y = dc->viewport.y;
		vp.z = dc->viewport.z;
		vp.w = dc->viewport.w;
		rcb->set_viewport(rptr, &vp);
	}
	auto av = dc;
	auto vd = av->vtxs.data();
	auto vdt = av->vtxs.data();
	auto idv = av->idxs.data();
	auto vbs = av->vtxs.size();
	auto ibs = av->idxs.size();
	std::vector<int> idxs;
	struct { void* texture; uint32_t blendMode; bool multiply = false; } states = {};
	glm::ivec4 oldclip = {};
	rcb->get_cliprect(rptr, &oldclip);
	size_t cclip = 0;
	for (auto& pcmd : av->cmd_data)
	{
		glm::vec2 clip_min((pcmd.clip_rect.x - clip_off.x) * clip_scale.x, (pcmd.clip_rect.y - clip_off.y) * clip_scale.y);
		glm::vec2 clip_max((pcmd.clip_rect.z - clip_off.x) * clip_scale.x, (pcmd.clip_rect.w - clip_off.y) * clip_scale.y);
		if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
		{
			rcb->set_cliprect(rptr, 0); cclip++;
		}
		else
		{
			glm::ivec4 r = { (int)(clip_min.x), (int)(clip_min.y), (int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y) };
			rcb->set_cliprect(rptr, &r); cclip++;
		}
		auto texture = pcmd.texid;
		auto vertices = vdt + pcmd.vtxOffset;
		const float* xy = &vertices->position.x;
		int stride = sizeof(vertex_v2);
		const float* color = (float*)&vertices->color;
		const float* uv = &vertices->tex_coord.x;
		int size_indices = 4;
		auto indices = ibs ? idv + pcmd.idxOffset : nullptr;
		auto num_indices = pcmd.elemCount;
		if ((BlendMode_e)pcmd.blend_mode != BlendMode_e::none) {
			uint32_t blend = pcmd.blend_mode;
			if (states.blendMode != blend || states.texture != texture || states.multiply != pcmd.multiply) {
				states.texture = texture; states.blendMode = blend; states.multiply = pcmd.multiply;
				rcb->set_texture_blend(states.texture, states.blendMode, states.multiply);
			}
		}
		rcb->draw_geometry(rptr, texture, xy, stride, color, stride, uv, stride, pcmd.vCount, indices, num_indices, size_indices);
	}
	if (cclip > 0) {
		rcb->set_cliprect(rptr, &oldclip);
	}
}


bool drawable_cx::update_rvgdata(rvg_data_cx* dst)
{
	assert(_view.z > 0 && _view.w > 0 && dst);
	auto rvg = dst->get();
	// dst->_pos != rvg->pos;	dst->_pos = rvg->pos;
	_drawv.push_back(dst);
	dst->_view = _view;
	auto ret = dst->update();
	build_vg(dst, this);
	return ret;
}


void drawable_cx::free_tex()
{
	if (rcb)
	{
		if (_vt.size())
		{
			for (auto& [k, v] : _vt) {
				rcb->free_texture(v);
			}
			_vt.clear();
		}
		if (_vgt.size()) {
			for (auto& [k, v] : _vgt) {
				rcb->free_texture(v);
			}
			_vgt.clear();
		}
	}
}

void drawable_cx::free_rvg(rvg_data_cx* p)
{
	if (!p || p->surfaces.empty())return;
	for (auto& it : p->surfaces)
	{
		if (it.surface)
		{
			free_surface(it.surface);
		}
	}
}

glm::ivec2 drawable_cx::get_size()
{
	return glm::ivec2(_view.z, _view.w);
}

void* drawable_cx::get_texture(void* surface)
{
	auto ipt = _vt.find((image_ptr_t*)surface);
	void* tp = 0;
	if (ipt != _vt.end())
	{
		tp = ipt->second;
	}
	else {
		auto kpt = _vgt.find(surface);
		if (kpt != _vgt.end())
			tp = kpt->second;
	}
	return tp;
}

void drawable_cx::pause()
{
	_pause = true;
}

void drawable_cx::submit_data(void* tex)
{
	void* renderer = rptr;
	if (!rcb || !renderer)return;
	const int vsize = sizeof(text_vx);
	if (opt.size()) {
		auto nv = opt.size();
		rcb->draw_geometry(renderer, tex, (float*)&opt.data()->pos, vsize, ((float*)&opt.data()->color), vsize,
			((float*)&opt.data()->uv), vsize, nv
			, idx.data(), idx.size(), sizeof(uint32_t));
		opt.clear();
		idx.clear();
	}
}

int drawable_cx::cmd_draw()
{
	int c = 0;
	for (auto& p : _drawv) {
		c += draw_rvg(p);
	}
	return c;
}

void drawable_cx::clear_draw()
{
	_drawv.clear();
}

void render_drawable(Drawable auto& drawable)
{
	drawable.cmd_draw();
}

int render_update(Drawable auto& drawable, float delta)
{
	return drawable.update(delta);
}




// H in [0,360)
// S, V, R, G, B in [0,1]
glm::vec4 convertHSVtoRGB(const glm::vec4& hsv)
{
	double H = hsv.x * 360.0, S = hsv.y, V = hsv.z;
	double R = 0.0, G = 0.0, B = 0.0;
	int Hi = int(floor(H / 60.)) % 6;
	double f = H / 60. - Hi;
	double p = V * (1 - S);
	double q = V * (1 - f * S);
	double t = V * (1 - (1 - f) * S);
	switch (Hi) {
	case 0: R = V, G = t, B = p; break;
	case 1: R = q, G = V, B = p; break;
	case 2: R = p, G = V, B = t; break;
	case 3: R = p, G = q, B = V; break;
	case 4: R = t, G = p, B = V; break;
	case 5: R = V, G = p, B = q; break;
	}
	return { R,G,B,hsv.w };
}



// Convert rgb floats ([0-1],[0-1],[0-1]) to hsv floats ([0-1],[0-1],[0-1]), from Foley & van Dam p592
// Optimized http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
glm::vec4 RGBtoHSV(uint32_t col)
{
	glm::u8vec4* c = (glm::u8vec4*)&col;
	double r = c->x / 255.0, g = c->y / 255.0, b = c->z / 255.0, a = c->w / 255.0;
	double K = 0.;
	if (g < b)
	{
		std::swap(g, b);
		K = -1.;
	}
	if (r < g)
	{
		std::swap(r, g);
		K = -2. / 6. - K;
	}
	const float chroma = r - (g < b ? g : b);
	glm::vec4 hsv = { fabs(K + (g - b) / (6.0 * chroma + 1e-20f)), chroma / (r + 1e-20f), r ,a };
	return hsv;
}
// Convert hsv floats ([0-1],[0-1],[0-1]) to rgb floats ([0-1],[0-1],[0-1]), from Foley & van Dam p593
// also http://en.wikipedia.org/wiki/HSL_and_HSV
glm::vec4 HSVtoRGB(const glm::vec4& hsv)
{
	glm::vec4 c = {};
	float h = hsv.x, s = hsv.y, v = hsv.z;
	c.w = hsv.w;
	if (s == 0.0f)
	{
		c.x = c.y = c.z = v;	// gray
		return c;
	}
	h = fmod(h, 1.0f) / (60.0f / 360.0f);
	int   i = (int)h;
	float f = h - (float)i;
	float p = v * (1.0f - s);
	float q = v * (1.0f - s * f);
	float t = v * (1.0f - s * (1.0f - f));
	switch (i)
	{
	case 0: c.x = v; c.y = t; c.z = p; break;
	case 1: c.x = q; c.y = v; c.z = p; break;
	case 2: c.x = p; c.y = v; c.z = t; break;
	case 3: c.x = p; c.y = q; c.z = v; break;
	case 4: c.x = t; c.y = p; c.z = v; break;
	case 5: default: c.x = v; c.y = p; c.z = q; break;
	}
	return c;
}



// todo test
void ttbr()
{
	drawable_cx* it = 0;
	//render_update(*it, 0.0f);
	//render_drawable(*it);
}

#endif


void r_update_data_text(text_render_o* p, drawable_cx* pt, float delta)
{
	std::vector<font_item_t>& tm = p->dst_vstr;
	for (auto& img : p->ov)
	{
		if (img) {
			auto& tp = pt->_vt[img];
			if (!tp)
			{
				tp = pt->rcb->new_tex(pt->rptr, img);
				pt->rcb->set_texture_blend(tp, 0, true);
			}
			else if (img->valid)
			{
				pt->rcb->update_texture(tp, 0, img->data, img->stride);
			}
		}
	}
}



void test_drawvkvg(VkvgContext ctx, VkvgSurface surf, bspline_ct* bs, const char* filename)
{
	vkvg_clear(ctx);
	vkvg_save(ctx);
	VkvgPattern pat;
	VkvgContext  cr = ctx;
	//pat = vkvg_pattern_create_linear(0.0, 0.0, 0.0, 256.0);
	//vkvg_pattern_add_color_stop(pat, 1, 0, 0, 0, 1);
	//vkvg_pattern_add_color_stop(pat, 0, 1, 1, 1, 1);
	//vkvg_rectangle(cr, 0, 0, 256, 256);
	//vkvg_set_source(cr, pat);
	//vkvg_fill(cr);
	//vkvg_pattern_destroy(pat);
	auto v = bs->sample2(64);	// 样条线转线段，细分64段
	vkvg_translate(ctx, 10, 20);
	vkvg_move_to(ctx, v[0].x, v[0].y);
	for (size_t i = 1; i < v.size(); i++)
	{
		vkvg_line_to(ctx, v[i].x, v[i].y);
	}
	vkvg_set_source_color(ctx, 0x800080ff);
	vkvg_set_line_width(ctx, 12.0);
	vkvg_stroke(ctx);
	vkvg_restore(ctx);
	fill_style_d st = {};
	glm::vec4 rect = { 195.5,95.5,510,210 };
	st.fill = 0xc0121212;
	st.color = 0x801152cc;
	st.thickness = 12;
	static glm::vec4 rr = { 6,6,6,6 };
	draw_rounded_rectangle(cr, rect.x, rect.y, rect.z, rect.w, rr);
	submit_style(cr, &st);
	vkvg_set_line_width(ctx, 6.0);
	vkvg_set_source_color(ctx, 0xffE97B5F);
	draw_arrow(ctx, glm::vec2(300, 350), glm::vec2(500, 350), 15, 50);
	draw_arrow(ctx, glm::vec2(300, 380), glm::vec2(500, 550), 15, 40);
	vkvg_flush(ctx);
	vkvg_surface_resolve(surf);	// msaa采样转换输出  
	if (filename && *filename)
	{
		vkvg_surface_write_to_png(surf, filename);
	}
}

VkShaderModule newModule(VkDevice device, const uint32_t* SpvData, size_t SpvSize, VkResult* r)
{
	VkShaderModule pShaderModule = {};
	VkShaderModuleCreateInfo moduleCreateInfo = {};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.pCode = (uint32_t*)SpvData;
	moduleCreateInfo.codeSize = SpvSize;
	auto hr = vkCreateShaderModule(device, &moduleCreateInfo, NULL, &pShaderModule);
	if (r)*r = hr;
	return pShaderModule;
}
#define new_module(device,spvdata,r) newModule(device,spvdata,sizeof(spvdata),r)

#define BLENDMODE_NONE                  0x00000000u /**< no blending: dstRGBA = srcRGBA */
#define BLENDMODE_BLEND                 0x00000001u /**< alpha blending: dstRGB = (srcRGB * srcA) + (dstRGB * (1-srcA)), dstA = srcA + (dstA * (1-srcA)) */
#define BLENDMODE_BLEND_PREMULTIPLIED   0x00000010u /**< pre-multiplied alpha blending: dstRGBA = srcRGBA + (dstRGBA * (1-srcA)) */
#define BLENDMODE_ADD                   0x00000002u /**< additive blending: dstRGB = (srcRGB * srcA) + dstRGB, dstA = dstA */
#define BLENDMODE_ADD_PREMULTIPLIED     0x00000020u /**< pre-multiplied additive blending: dstRGB = srcRGB + dstRGB, dstA = dstA */
#define BLENDMODE_MOD                   0x00000004u /**< color modulate: dstRGB = srcRGB * dstRGB, dstA = dstA */
#define BLENDMODE_MUL                   0x00000008u /**< color multiply: dstRGB = (srcRGB * dstRGB) + (dstRGB * (1-srcA)), dstA = dstA */

void set_cblend(VkPipelineColorBlendAttachmentState& opt, const std::array<uint32_t, 6>& b) {
	opt.srcColorBlendFactor = (VkBlendFactor)b[0];
	opt.srcAlphaBlendFactor = (VkBlendFactor)b[1];
	opt.colorBlendOp = (VkBlendOp)b[2];
	opt.dstColorBlendFactor = (VkBlendFactor)b[3];
	opt.dstAlphaBlendFactor = (VkBlendFactor)b[4];
	opt.alphaBlendOp = (VkBlendOp)b[5];
}
#if 1
#define BLENDMODE_NONE_FULL set_cblend(colorBlendAttachment,{VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD})
#define BLENDMODE_BLEND_FULL set_cblend(colorBlendAttachment,{VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD})
#define BLENDMODE_BLEND_PREMULTIPLIED_FULL set_cblend(colorBlendAttachment,{VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD})
#define BLENDMODE_ADD_FULL set_cblend(colorBlendAttachment,{VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD})
#define BLENDMODE_ADD_PREMULTIPLIED_FULL set_cblend(colorBlendAttachment,{VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD})
#define BLENDMODE_MOD_FULL set_cblend(colorBlendAttachment,{VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD})
#define BLENDMODE_MUL_FULL set_cblend(colorBlendAttachment,{VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD})
#define BLENDMODE_SCREEN_FULL set_cblend(colorBlendAttachment,{VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, VK_BLEND_OP_ADD})
#endif

// Color blend
void set_blend(VkPipelineColorBlendAttachmentState& colorBlendAttachment, uint32_t blendMode)
{
	colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = VK_TRUE;
	auto bm = (blendmode_e)blendMode;
	switch (bm)
	{
	case blendmode_e::none:
		BLENDMODE_NONE_FULL;
		break;
	case blendmode_e::normal:
		BLENDMODE_BLEND_FULL;
		break;
	case blendmode_e::additive:
		BLENDMODE_ADD_FULL;
		break;
	case blendmode_e::normal_prem:
		BLENDMODE_BLEND_PREMULTIPLIED_FULL;
		break;
	case blendmode_e::additive_prem:
		BLENDMODE_ADD_PREMULTIPLIED_FULL;
		break;
	case blendmode_e::multiply:
		BLENDMODE_MUL_FULL;
		break;
	case blendmode_e::modulate:
		BLENDMODE_MOD_FULL;
		break;
	case blendmode_e::screen:
		BLENDMODE_SCREEN_FULL;
		break;
	default:
		break;
	}
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
}
struct pipelinestate_p
{
	int shader;			// 0: base3d, 1: base3d2
	uint32_t blendMode;
	VkPrimitiveTopology topology;
	VkFormat format;
	VkPipelineLayout pipelineLayout;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipeline pipeline;
};
struct pipe_data {
	VkDevice dev = {};
	VkShaderModule vert[2] = {};
	VkShaderModule frag[2] = {};
	VkRenderPass renderPass = {};
	std::vector<pipelinestate_p> pipelineStates;
};
static pipelinestate_p* newPipelineState(pipe_data* pd, int shader, uint32_t blendMode, VkPrimitiveTopology topology)
{
	pipelinestate_p pipelineStates = {};
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkResult result = VK_SUCCESS;
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	VkVertexInputAttributeDescription attributeDescriptions[3];
	VkVertexInputBindingDescription bindingDescriptions[1];
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2];
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.pStages = shaderStageCreateInfo;
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

	// Shaders
	const char* name = "main";
	for (uint32_t i = 0; i < 2; i++) {
		shaderStageCreateInfo[i] = {};
		shaderStageCreateInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCreateInfo[i].module = (i == 0) ? pd->vert[shader] : pd->frag[shader];
		shaderStageCreateInfo[i].stage = (i == 0) ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStageCreateInfo[i].pName = name;
	}
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = &shaderStageCreateInfo[0];


	VkPipelineLayout pipelineLayout = {};
	VkDescriptorSetLayout descriptorSetLayout = {};
	VkPushConstantRange pushConstantRange[] = {
		{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)},
		//{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(push_constants)}
	};
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings(1);
	layoutBindings[0].binding = 1;
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindings[0].descriptorCount = 1;
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	layoutBindings[0].pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
	descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_layout.pNext = NULL;
	descriptor_layout.bindingCount = (uint32_t)layoutBindings.size();
	descriptor_layout.pBindings = layoutBindings.data();
	result = vkCreateDescriptorSetLayout(pd->dev, &descriptor_layout, NULL, &descriptorSetLayout);
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = (VkPushConstantRange*)&pushConstantRange;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	result = vkCreatePipelineLayout(pd->dev, &pipelineLayoutCreateInfo, NULL, &pipelineLayout);
	/*
	layout(location = 0) in vec3 pos;
	layout(location = 1) in vec2 uv;
	layout(location = 2) in vec4 col;
	layout(location = 3) in vec4 col1;
	*/
	VkVertexInputAttributeDescription vi_attrs[] =
	{
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
		{ 1, 0, VK_FORMAT_R32G32_SFLOAT, 12 },
		{ 2, 0, VK_FORMAT_R8G8B8A8_UNORM, 20 },
		{ 3, 0, VK_FORMAT_R8G8B8A8_UNORM, 24 },
	};
	//uint32_t    location;
	//uint32_t    binding;
	//VkFormat    format;
	//uint32_t    offset;
	//_countof(vi_attrs); 
	// Vertex input
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 3 + shader;
	vertexInputCreateInfo.pVertexAttributeDescriptions = vi_attrs;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescriptions[0];

	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	bindingDescriptions[0].stride = sizeof(float) * 5 + sizeof(int) * (shader + 1);

	// Input assembly
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = topology;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.viewportCount = 1;

	// Dynamic states
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	VkDynamicState dynamicStates[2] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	dynamicStateCreateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
	dynamicStateCreateInfo.pDynamicStates = dynamicStates;

	// Rasterization state
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationStateCreateInfo.lineWidth = 1.0f;

	// MSAA state
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	VkSampleMask multiSampleMask = 0xFFFFFFFF;
	multisampleStateCreateInfo.pSampleMask = &multiSampleMask;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Depth Stencil
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	// Color blend
	VkPipelineColorBlendAttachmentState colorBlendAttachment = { 0 };
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachment;
	colorBlendAttachment.blendEnable = VK_TRUE;
	set_blend(colorBlendAttachment, blendMode);
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	// Renderpass / layout
	pipelineCreateInfo.renderPass = pd->renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.layout = pipelineLayout;

	result = vkCreateGraphicsPipelines(pd->dev, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &pipeline);
	if (result != VK_SUCCESS) {
		return NULL;
	}

	pipelineStates.shader = shader;
	pipelineStates.blendMode = blendMode;
	pipelineStates.topology = topology;
	pipelineStates.pipeline = pipeline;
	pipelineStates.descriptorSetLayout = descriptorSetLayout;
	pipelineStates.pipelineLayout = pipelineCreateInfo.layout;
	pd->pipelineStates.push_back(pipelineStates);
	auto pps = &pd->pipelineStates.back();
	return pps;
}

pipelinestate_p* new_spv_base(VkDevice device)
{
	auto base3d_frag_m = new_module(device, base3d_frag, 0);
	auto base3d_vert_m = new_module(device, base3d_vert, 0);
	auto base3d2_frag_m = new_module(device, base3d2_frag, 0);
	auto base3d2_vert_m = new_module(device, base3d2_vert, 0);
	auto pd = new pipe_data();
	pd->dev = device;
	pd->frag[0] = base3d_frag_m;
	pd->vert[0] = base3d_vert_m;
	pd->frag[1] = base3d2_frag_m;
	pd->vert[1] = base3d2_vert_m;
	int blendMode = 0; VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelinestate_p* p = newPipelineState(pd, 0, blendMode, topology);
	pipelinestate_p* p1 = newPipelineState(pd, 1, blendMode, topology);
	return p;
}


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------

size_t gen_rect_color(const glm::ivec4& rc, const glm::vec4& color, std::vector<text_vx>& vtx, std::vector<uint32_t>& idx)
{
	glm::vec2 uv = {};
	glm::vec2 a = { rc.x,rc.y }, c = { rc.x + rc.z,rc.y + rc.w };
	uint32_t vps = vtx.size();
	uint32_t ips = idx.size();
	vtx.resize(vps + 4);
	idx.insert(idx.end(), { vps + 0,vps + 1,vps + 2,vps + 0,vps + 2,vps + 3 });
	auto t = vtx.data() + vps;
	t->pos = a; t->uv = uv; t->color = color; t++;
	t->pos = glm::vec2(c.x, a.y); t->uv = uv; t->color = color; t++;
	t->pos = c; t->uv = uv; t->color = color; t++;
	t->pos = glm::vec2(a.x, c.y); t->uv = uv; t->color = color; t++;
	return vps;
}

size_t gen_rect_mcolor(const glm::ivec4& rc, const glm::ivec4& color, std::vector<text_vx>& vtx, std::vector<uint32_t>& idx)
{
	glm::vec2 uv = {};
	glm::vec2 a = { rc.x,rc.y }, c = { rc.x + rc.z,rc.y + rc.w };
	uint32_t vps = vtx.size();
	uint32_t ips = idx.size();
	vtx.resize(vps + 4);
	idx.insert(idx.end(), { vps + 0,vps + 1,vps + 2,vps + 0,vps + 2,vps + 3 });
	auto t = vtx.data() + vps;
	t->pos = a; t->uv = uv; t->color = ucolor2fx(color.x); t++;
	t->pos = glm::vec2(c.x, a.y); t->uv = uv; t->color = ucolor2fx(color.y); t++;
	t->pos = c; t->uv = uv; t->color = ucolor2fx(color.z); t++;
	t->pos = glm::vec2(a.x, c.y); t->uv = uv; t->color = ucolor2fx(color.w); t++;
	return vps;
}
size_t gen_rect_mcolor(const glm::ivec4& rc, const glm::vec4* color, std::vector<text_vx>& vtx, std::vector<uint32_t>& idx)
{
	glm::vec2 uv = {};
	glm::vec2 a = { rc.x,rc.y }, c = { rc.x + rc.z,rc.y + rc.w };
	uint32_t vps = vtx.size();
	uint32_t ips = idx.size();
	vtx.resize(vps + 4);
	idx.insert(idx.end(), { vps + 0,vps + 1,vps + 2,vps + 0,vps + 2,vps + 3 });
	auto t = vtx.data() + vps;
	t->pos = a; t->uv = uv; t->color = (color[0]); t++;
	t->pos = glm::vec2(c.x, a.y); t->uv = uv; t->color = (color[1]); t++;
	t->pos = c; t->uv = uv; t->color = (color[2]); t++;
	t->pos = glm::vec2(a.x, c.y); t->uv = uv; t->color = (color[3]); t++;
	return vps;
}


size_t build_huedata(const glm::vec4& incolor, const glm::ivec4& rc0, std::vector<text_vx>& vtx, std::vector<uint32_t>& idx) {
	std::vector<glm::vec4> color = { glm::vec4(1.0) ,incolor,incolor,glm::vec4(1.0) };
	auto rc = rc0;
	gen_rect_mcolor(rc, color.data(), vtx, idx);	//白色->颜色
	color = { glm::vec4(0.0),glm::vec4(0.0),glm::vec4(0,0,0,1) ,glm::vec4(0,0,0,1) };
	gen_rect_mcolor(rc, color.data(), vtx, idx);	//透明->黑
	static const glm::vec4 col_hues[6 + 1] = { glm::vec4(1,0,0,1), glm::vec4(1,1,0,1), glm::vec4(0,1,0,1),
		glm::vec4(0,1,1,1), glm::vec4(0,0,1,1), glm::vec4(1,0,1,1), glm::vec4(1,0,0,1) };
	rc.x += rc.z + 4;
	auto rc1 = rc;
	auto fx = rc.z / 6.0;
	rc.z = fx;
	for (size_t i = 0; i < 6; i++)
	{
		glm::vec4 c4f[4] = { col_hues[i],col_hues[i + 1] };
		c4f[2] = c4f[1];
		c4f[3] = c4f[0];
		gen_rect_mcolor(rc, c4f, vtx, idx);
		rc.x += fx;
	}
	gen_rect_mcolor(rc1, color.data(), vtx, idx);	//透明->黑
	return vtx.size();

}
// 获取色盘颜色
glm::vec4 get_color_cb(const glm::ivec2& pos, const glm::ivec2& size, float h)
{
	glm::vec2 n = (glm::vec2)pos / (glm::vec2)size;
	glm::vec4 hc = {};
	glm::vec4 hsv = { h,0,0,1 };
	hsv.y = n.x;
	hsv.z = 1.0 - n.y;
	hc = HSVtoRGB(hsv);
	return hc;
}
// 获取色调颜色
glm::vec4 get_hue_color_cb(const glm::ivec2& pos, const glm::ivec2& size)
{
	glm::vec2 n = (glm::vec2)pos / (glm::vec2)size;
	glm::vec4 hc = {};
	glm::vec4 hsv = { n.x,0,0,1 };
	hsv.y = 1;
	hsv.z = 1.0 - n.y;
	hc = HSVtoRGB(hsv);
	return hc;
}



//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------


/*
		int texwidth = 1024;
		VkvgSurface surf = vctx ? vctx->new_surface(texwidth, texwidth) : 0;
		auto ctx = vkvg_create(surf);
	{
				//vkvg_clear(ctx);
				//vkvg_flush(ctx);
				//vkvg_surface_resolve(surf);//msaa采样转换输出
				//vkvg_surface_write_to_png(surf, filename);
				//vkvg_destroy(ctx);

				VkImage image = vkvg_surface_get_vk_image(surf);
				VkFormat format = vkvg_surface_get_vk_format(surf);
				//vctx->free_surface(surf);
				if (image)
				{
					vg2dtex = pcb->new_texture_vk(form0->renderer, texwidth, texwidth, image, format == VK_FORMAT_B8G8R8A8_UNORM ? 1 : 0);// 创建SDL的rgba纹理
					pcb->set_texture_blend(vg2dtex, (int)BLENDMODE_E::normal, true);
				}
			}
		vkvg_flush(ctx);
		vkvg_destroy(ctx);
		vctx->free_surface(surf);
		free_vkvgdev(vctx);
*/



rdg_cx::rdg_cx()
{}

rdg_cx::~rdg_cx()
{}

GraphResource::GraphResource()
{}

GraphResource::~GraphResource()
{}



mesh2d_cx::mesh2d_cx()
{}

mesh2d_cx::~mesh2d_cx()
{}

void mesh2d_cx::clear()
{
	vtxs.clear();
	idxs.clear();
	cmd_data.clear();
	_clip_rect = viewport;
	_clip_rect.x = _clip_rect.y = 0;
}

inline uint8_t is_rect_intersect0(int x01, int x02, int y01, int y02,
	int x11, int x12, int y11, int y12)
{
	int zx = abs(x01 + x02 - x11 - x12);
	int x = abs(x01 - x02) + abs(x11 - x12);
	int zy = abs(y01 + y02 - y11 - y12);
	int y = abs(y01 - y02) + abs(y11 - y12);
	if (zx <= x && zy <= y)
		return 1;
	else
		return 0;
}
inline bool is_rect_intersect(glm::vec4 r1, glm::vec4 r2)
{
	//第一种情况：如果b.x > a.x + a.w，则a和b一定不相交，
	//第二种情况：如果a.y > b.y + b.h，则a和b一定不相交，
	//第三种情况：如果b.y > a.y + a.h，则a和b一定不相交，
	//第四种情况：如果a.x > b.x + b.w，则a和b一定不相交
	auto& a = r1; auto& b = r2;
	if (a.x > b.x + b.z || b.x > a.x + a.z || a.y > b.y + b.w || b.y > a.y + a.w) {
		return false;
	}
	else {
		return true;
	}
	return is_rect_intersect0(r1.x, r1.y, r1.z, r1.w, r2.x, r2.y, r2.z, r2.w);
}
bool mesh2d_cx::nohas_clip(glm::ivec4 a)
{
	auto clip = _clip_rect;
	if (clip.z > viewport.z || clip.z < 0)clip.z = viewport.z;
	if (clip.w > viewport.w || clip.w < 0)clip.w = viewport.w;
	if (clip.z < 0 || clip.w < 0)
	{
		return false;
	}
	return (!is_rect_intersect(clip, a));
}
void mesh2d_cx::add(void* user_image, std::vector<vertex_t>& vertex, std::vector<int>& vt_index, const glm::ivec4& clip)
{
	add(user_image, vertex.data(), vertex.size(), vt_index.data(), vt_index.size(), clip);
}

void mesh2d_cx::add(void* user_image, vertex_t* vertex, size_t vcount, int* vt_index, size_t icount, const glm::ivec4& clip)
{
	auto ps0 = vcount;
	auto ps = vtxs.size();
	auto ix = idxs.size();
	auto ic = icount;
	vtxs.resize(ps + vcount);
	idxs.resize(ix + icount);
	auto& cd = cmd_data;
	if (cd.empty())
	{
		cd.push_back({});
	}
	auto dt = &cd.back();
	auto pidx = idxs.data() + ix;
	if (dt->texid != user_image || dt->clip_rect != clip)
	{
		if (dt->elemCount > 0)
			cd.push_back({});
		dt = &cd.back();
		dt->texid = user_image;
		dt->clip_rect = clip;
		dt->vtxOffset = ps;
		dt->idxOffset = ix;
		dt->elemCount = ic;
		dt->vCount = ps0;
	}
	else
	{
		// 合批
		dt->elemCount += ic;
		dt->vCount += ps0;
		auto idt = vt_index;
		for (size_t i = 0; i < ic; i++)
		{
			idt[i] += ix;
		}
	}
	memcpy(vtxs.data() + ps, vertex, vcount * sizeof(vertex[0]));
	memcpy(pidx, vt_index, icount * sizeof(vt_index[0]));
}


void mesh2d_cx::add_image(image_ptr_t* img, const glm::ivec4& clip, const glm::ivec4& dst, const glm::ivec4& src, const glm::ivec4& sliced, uint32_t color)
{
	auto a = glm::vec4(dst);
	glm::ivec2 pos = { a.x, a.y }, size = { a.z, a.w };
	glm::vec4 v4 = { 0, 0, 1, 1 };
	glm::vec4 uv = v4;
	glm::vec2 s = size;
	glm::ivec2 texsize = { img->width, img->height };
	if (a.z < 0)
		a.z *= -std::min(src.z, texsize.x);
	if (a.w < 0)
		a.w *= -std::min(src.w, texsize.y);
	if (nohas_clip(a))
		return;
	glm::vec4 color3 = ucolor2f(color);
	if (sliced.x > 0)
	{
		add_image_sliced(img, a, texsize, sliced, src, color, clip);// 生成九宫格到mesh
	}
	else
	{
		if (!(src.x < 0))
		{
			v4 = src;
			v4.z += v4.x; v4.w += v4.y;//加上原点坐标
			v4.z = glm::min(v4.z, (float)texsize.x);
			v4.w = glm::min(v4.w, (float)texsize.y);
			uv = { v4.x / texsize.x, v4.y / texsize.y, v4.z / texsize.x, v4.w / texsize.y };
			if (uv.x < 0) { uv.x = 0; }
			if (uv.y < 0) { uv.y = 0; }
		}
		glm::vec2 av = pos, cv = { pos.x + s.x, pos.y + s.y }, uv_a = { uv.x, uv.y }, uv_c{ uv.z, uv.w };
		auto& col = color3;
		glm::vec2 bv(cv.x, av.y), dv(av.x, cv.y), uv_b(uv_c.x, uv_a.y), uv_d(uv_a.x, uv_c.y);

		vertex_t vertex[] = {
		   {av, uv_a, col},
		   {bv, uv_b, col},
		   {cv, uv_c, col},
		   {dv, uv_d, col},
		};
		int rect_index_order[] = { 0, 1, 2, 0, 2, 3 };
		add(img, vertex, 4, rect_index_order, 6, clip);// 添加矩形(两个三角形)到mesh
	}
}

/*


九宫格渲染:
+--+---------------+--+
|0 |       1       |2 |
+--+---------------+--+
|  |               |  |
|  |               |  |
|3 |    center     |4 |
|  |               |  |
+--+---------------+--+
|5 |       6       |7 |
+--+---------------+--+

九宫格:索引
0  12                     14  2
8  4                      6   10

9  5                      7   11
1  13                     15  3
+--+-------------------------+--+
|  |                         |  |
+--+-------------------------+--+
|  |                         |  |
|  |                         |  |
+--+-------------------------+--+
|  |                         |  |
+--+-------------------------+--+
sliced.x=左宽，y上高，z右宽，w下高

*/
void mesh2d_cx::add_image_sliced(void* user_image, const glm::ivec4& a, const glm::ivec2& texsize, const glm::ivec4& sliced, const glm::ivec4& rect, uint32_t col, const glm::ivec4& clip)
{
	static std::vector<int> vt_index =// { 0,8,12,4,14,6,2,10,11,6,7,4,5,8,9,1,5,13,7,15,11,3 };//E_TRIANGLE_STRIP
	{ 0, 8, 12, 8, 12, 4, 12, 4, 14, 4, 14, 6, 14, 6, 2, 6, 2, 10,
		6, 7, 10, 7, 10, 11, 4, 5, 6, 5, 6, 7, 8, 9, 4, 9, 4, 5,
		9, 1, 5, 1, 5, 13, 5, 13, 7, 13, 7, 15, 7, 15, 11, 15, 11, 3 };//E_TRIANGLE_LIST

	glm::ivec2 pos = { a.x, a.y }, size = { a.z, a.w };
	glm::vec4 uv = { 0, 0, 1, 1 };
	glm::vec4 v4 = { 0, 0, texsize.x, texsize.y };
	if (!(rect.x < 0))
	{
		v4 = rect;
		v4.z += v4.x; v4.w += v4.y;//加上原点坐标
		uv = { v4.x / texsize.x, v4.y / texsize.y, v4.z / texsize.x, v4.w / texsize.y, };
	}
	float left = sliced.x,
		top = sliced.y,
		right = sliced.z,
		bottom = sliced.w;
	float x = pos.x, y = pos.y, width = size.x, height = size.y;
	glm::vec4 suv = { (left + v4.x) / texsize.x, (top + v4.y) / texsize.y,
		(v4.z - right) / texsize.x, (v4.w - bottom) / texsize.y };

	std::vector<vertex_t> vertex = {
		//0
		{{x, y}, {uv.x, uv.y}, col},
		//1
		{{x, y + height}, {uv.x, uv.w}, col},
		//2
		{{x + width, y}, {uv.z, uv.y}, col},
		//3
		{{x + width, y + height}, {uv.z, uv.w}, col},
		//4
		{{x + left, y + top}, {suv.x, suv.y}, col},
		//5
		{{x + left, y + height - bottom}, {suv.x, suv.w}, col},
		//6
		{{x + width - right, y + top}, {suv.z, suv.y}, col},
		//7
		{{x + width - right, y + height - bottom}, {suv.z, suv.w}, col},
		//8
		{{x, y + top}, {uv.x, suv.y}, col},
		//9
		{{x, y + height - bottom}, {uv.x, suv.w}, col},
		//10
		{{x + width, y + top}, {uv.z, suv.y}, col},
		//11
		{{x + width, y + height - bottom}, {uv.z, suv.w}, col},
		//12
		{{x + left, y}, {suv.x, uv.y}, col},
		//13
		{{x + left, y + height}, {suv.x, uv.w}, col},
		//14
		{{x + width - right, y}, {suv.z, uv.y}, col},
		//15
		{{x + width - right, y + height}, {suv.z, uv.w}, col}
	};

	add(user_image, vertex, vt_index, clip);

	return;
}
void mesh2d_cx::add_image_angle(image_ptr_t* img, const glm::ivec4& srcrect, const glm::ivec4& dstrect, float angle, const glm::vec2* center, uint32_t col, const glm::ivec4& clip, int flip)
{
	int rect_index_order[] = { 0, 1, 2, 0, 2, 3 };
	glm::ivec4 real_srcrect = {};
	glm::vec2 real_center = {};
	if (flip == FLIP_NONE && (int)(angle / 360) == angle / 360) { // fast path when we don't need rotation or flipping
		add_image(img, clip, srcrect, dstrect, {}, col);
		return;
	}
	real_srcrect.x = 0.0f;
	real_srcrect.y = 0.0f;
	real_srcrect.z = (float)img->width;
	real_srcrect.w = (float)img->height;
	if (center) {
		real_center = *center;
	}
	else {
		real_center.x = dstrect.z / 2.0f;
		real_center.y = dstrect.w / 2.0f;
	}
	vertex_t v[4];
	//float xy[8];
	const int xy_stride = 2 * sizeof(float);
	//float uv[8];
	const int uv_stride = 2 * sizeof(float);
	const int num_vertices = 4;
	const int* indices = rect_index_order;
	const int num_indices = 6;
	const int size_indices = 4;
	glm::vec2 minuv, maxuv;
	glm::vec2 minxy, maxxy;
	float centerx, centery;

	float s_minx, s_miny, s_maxx, s_maxy;
	float c_minx, c_miny, c_maxx, c_maxy;

	const float radian_angle = glm::radians(angle);
	const float s = glm::sin(radian_angle);
	const float c = glm::cos(radian_angle);

	minuv.x = real_srcrect.x / img->width;
	minuv.y = real_srcrect.y / img->height;
	maxuv.x = (real_srcrect.x + real_srcrect.z) / img->width;
	maxuv.y = (real_srcrect.y + real_srcrect.w) / img->height;

	centerx = real_center.x + dstrect.x;
	centery = real_center.y + dstrect.y;

	if (flip & FLIP_HORIZONTAL) {
		minxy.x = dstrect.x + dstrect.z;
		maxxy.x = dstrect.x;
	}
	else {
		minxy.x = dstrect.x;
		maxxy.x = dstrect.x + dstrect.z;
	}

	if (flip & FLIP_VERTICAL) {
		minxy.y = dstrect.y + dstrect.w;
		maxxy.y = dstrect.y;
	}
	else {
		minxy.y = dstrect.y;
		maxxy.y = dstrect.y + dstrect.w;
	}

	v[0].tex_coord = minuv;
	v[1].tex_coord = maxuv;
	v[2].tex_coord = maxuv;
	v[3].tex_coord = minuv;

	/* apply rotation with 2x2 matrix ( c -s )
	 *                                ( s  c ) */
	s_minx = s * (minxy.x - centerx);
	s_miny = s * (minxy.y - centery);
	s_maxx = s * (maxxy.x - centerx);
	s_maxy = s * (maxxy.y - centery);
	c_minx = c * (minxy.x - centerx);
	c_miny = c * (minxy.y - centery);
	c_maxx = c * (maxxy.x - centerx);
	c_maxy = c * (maxxy.y - centery);

	// (minx, miny)
	v[0].position = glm::vec2((c_minx - s_miny) + centerx, (s_minx + c_miny) + centery);
	// (maxx, miny)
	v[1].position = glm::vec2((c_maxx - s_miny) + centerx, (s_maxx + c_miny) + centery);
	// (maxx, maxy)
	v[2].position = glm::vec2((c_maxx - s_maxy) + centerx, (s_maxx + c_maxy) + centery);
	// (minx, maxy)
	v[3].position = glm::vec2((c_minx - s_maxy) + centerx, (s_minx + c_maxy) + centery);
	auto c4 = ucolor2fx(col);
	v[0].color = c4;
	v[1].color = c4;
	v[2].color = c4;
	v[3].color = c4;
	add(img, v, 4, rect_index_order, 6, clip);
}


mesh2d_cx::vertex_t::vertex_t()
{}

mesh2d_cx::vertex_t::vertex_t(glm::vec3 p, glm::vec2 u, uint32_t c) :position(p.x, p.y), tex_coord(u) {
	color = ucolor2f(c);
}

mesh2d_cx::vertex_t::vertex_t(glm::vec2 p, glm::vec2 u, uint32_t c) :position(p), tex_coord(u) {
	color = ucolor2f(c);
}

mesh2d_cx::vertex_t::vertex_t(glm::vec2 p, glm::vec2 u, glm::vec4 c) :position(p), tex_coord(u), color(c)
{}
