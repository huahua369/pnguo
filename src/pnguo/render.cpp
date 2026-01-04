/*

vkvg设备需要扩展scalarBlockLayout：VkPhysicalDeviceVulkan12Features或VkPhysicalDeviceScalarBlockLayoutFeaturesEXT
*/
#include <pch1.h>
#include <stdint.h>
#include <stdio.h>
// 实现
//#define GLM_ENABLE_EXPERIMENTAL 
//#include <glm/glm.hpp>
//#include <glm/gtx/vector_angle.hpp>
//#include <glm/gtx/closest_point.hpp>
//#include <glm/gtc/type_ptr.hpp>

#include <pnguo.h>

#define FLEX_IMPLEMENTATION
#include <vg.h>
#include <vkvg/vkvg.h> 

#include "render.h"
#include <stb_rect_pack.h> 
#include <print_time.h> 
#include <vkh.h>
#include "shaders/base3d.vert.h"
#include "shaders/base3d.frag.h"
#include "shaders/base3d2.vert.h"
#include "shaders/base3d2.frag.h"


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

void vkvg_dev::update_image(VkvgSurface image, uint32_t* data, int width, int height)
{

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
		VkPhysicalDeviceMemoryProperties m_memoryProperties = {};
		vkGetPhysicalDeviceMemoryProperties(c->phy, &m_memoryProperties);
		p->memoryProperties = new VkPhysicalDeviceMemoryProperties(m_memoryProperties);
		p->dev = dev;
		p->vkdev = vkdev;
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

const float arrow_size = 10;
const float arrow_hwidth = 4;
const float point_label_delta = -10;

glm::vec2 avec2_perp(glm::vec2 a) { return  glm::vec2(a.y, -a.x); }

void draw_arrow2(VkvgContext ctx, float x, float y)
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
		vkvg_save(cr);
		vkvg_set_source_color(cr, st->fill);
		if (stroke)
			vkvg_fill_preserve(cr);
		else
			vkvg_fill(cr);
		vkvg_restore(cr);
	}
	if (stroke)
	{
		vkvg_save(cr);
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
		vkvg_restore(cr);
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
// r，左右下左
void draw_rounded_rectangle(VkvgContext ctx, double x, double y, double width, double height, const glm::vec4& r1)
{
#ifndef M_PI
	auto M_PI = glm::pi<double>();
#endif
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
	vkvg_save(cr);
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
	vkvg_restore(cr);
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
void paint_shadow(VkvgContext cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, bool rev, float r)
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
void paint_shadow(VkvgContext cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, const glm::vec4& color_to, bool rev, float r)
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

void gen3data(image_ptr_t* img, const glm::ivec2& dst_pos, const glm::ivec4& rc, uint32_t color, std::vector<text_vx>* opt, std::vector<uint32_t>* idx)
{
	glm::ivec2 tex_size = { img->width,img->height };
	// 1. 计算矩形顶点（像素坐标）
	glm::ivec2 A = glm::ivec2(rc.x, rc.y); // 左下
	glm::ivec2 B = A + glm::ivec2(rc.z, 0); // 右下
	glm::ivec2 C = A + glm::ivec2(rc.z, rc.w); // 右上
	glm::ivec2 D = A + glm::ivec2(0, rc.w); // 左上
	glm::vec2 A1 = dst_pos;
	glm::vec2 B1 = dst_pos + glm::ivec2(rc.z, 0);
	glm::vec2 C1 = dst_pos + glm::ivec2(rc.z, rc.w);
	glm::vec2 D1 = dst_pos + glm::ivec2(0, rc.w);

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
	opt->insert(opt->end(), text_vx{ A1 ,A_norm , c });
	opt->insert(opt->end(), text_vx{ B1 ,B_norm , c });
	opt->insert(opt->end(), text_vx{ C1 ,C_norm , c });
	opt->insert(opt->end(), text_vx{ D1 ,D_norm , c });
	idx->insert(idx->end(), { ps + 0,ps + 1,ps + 2,ps + 0,ps + 2,ps + 3 });
	return;
}

void r_render_data(layout_tx* p, const glm::vec2& pos, sdl3_textdata* pt)
{
	void* renderer = pt->rptr;
	if (!p || !pt || !renderer)return;
	uint32_t color = -1;
	const int vsize = sizeof(text_vx);
	void* tex = 0;
	bool devrtex = (pt->rcb->set_texture_color4 && pt->rcb->render_texture);
	auto rect = p->box.rc;
	glm::ivec4 rc = {};
	glm::ivec2 pos0 = pos;
	pt->rcb->get_cliprect(renderer, &rc);
	pt->rcb->set_cliprect(renderer, &rect);
	for (auto& it : p->rd) {
		if (!it.img)continue;
		auto& tp = pt->vt[it.img];
		if (!tp)
		{
			tp = pt->rcb->make_tex(renderer, it.img);
		}
		if (!devrtex && tex != tp)
		{
			if (tex && pt->opt.size()) {
				auto nv = pt->opt.size();
				pt->rcb->draw_geometry(renderer, tex, (float*)pt->opt.data(), vsize, ((float*)pt->opt.data()) + 4, vsize, ((float*)pt->opt.data()) + 2, vsize, nv, pt->idx.data(), pt->idx.size(), sizeof(uint32_t));
				pt->opt.clear();
				pt->idx.clear();
			}
			tex = tp;
		}
		auto c = it.color ? it.color : color;
		if (it.sliced.x > 0 || it.sliced.y > 0 || it.sliced.z > 0 || it.sliced.w > 0)
		{
			texture_9grid_dt t9 = {};
			t9.src_rect = it.rc;
			t9.dst_rect = glm::vec4(it.pos + pos0, it.dsize);
			t9.scale = 0.0;
			t9.left_width = it.sliced.x, t9.right_width = it.sliced.y, t9.top_height = it.sliced.z, t9.bottom_height = it.sliced.w;
			t9.tileScale = -1; // 大于0则是9gridtiled中间平铺
			pt->rcb->render_texture_9grid(renderer, tex, &t9, 1);
		}
		else
		{
			if (devrtex)
			{
				texture_dt p = {};
				p.dst_rect = glm::vec4(it.pos + pos0, it.dsize);
				p.src_rect = it.rc;
				auto c4 = ucolor2f(c);
				pt->rcb->set_texture_color4(tex, &c4);
				pt->rcb->render_texture(renderer, tex, &p, 1);
			}
			else
			{
				gen3data(it.img, it.pos + pos0, it.rc, c, &pt->opt, &pt->idx);
				pt->tex = tp;
			}
		}
	}
	if (!devrtex && tex && pt->opt.size()) {
		auto nv = pt->opt.size();
		pt->rcb->draw_geometry(renderer, tex, (float*)pt->opt.data(), vsize, ((float*)pt->opt.data()) + 4, vsize, ((float*)pt->opt.data()) + 2, vsize, nv, pt->idx.data(), pt->idx.size(), sizeof(uint32_t));
		pt->opt.clear();
		pt->idx.clear();
	}
	pt->rcb->set_cliprect(renderer, &rc);
}

void r_update_data_text(text_render_o* p, sdl3_textdata* pt, float delta)
{
	std::vector<font_item_t>& tm = p->_vstr;
	for (auto& it : tm)
	{
		if (it._image) {
			auto& tp = pt->vt[it._image];
			if (!tp)
			{
				tp = pt->rcb->make_tex(pt->rptr, it._image);
			}
		}
	}
}
void r_render_data_text(text_render_o* p, const glm::vec2& pos, sdl3_textdata* pt)
{
	void* renderer = pt->rptr;
	std::vector<font_item_t>& tm = p->_vstr;
	uint32_t color = p->tb->style->color;
	if (color != pt->color) {
		pt->color = color;
		pt->opt.clear();
		pt->idx.clear();
	}
	void* tex = 0;
	const int vsize = sizeof(text_vx);
	if (pt->opt.empty())
	{
		size_t ct = 0;
		for (size_t i = 0; i < tm.size(); i++)
		{
			auto& git = tm[i];
			if (git._image) {
				auto& tp = pt->vt[git._image];
				if (tex != tp)
				{
					if (tex && pt->opt.size()) {
						auto nv = pt->opt.size();
						pt->rcb->draw_geometry(renderer, tex, (float*)&pt->opt.data()->pos, vsize, ((float*)&pt->opt.data()->color), vsize,
							((float*)&pt->opt.data()->uv), vsize, nv
							, pt->idx.data(), pt->idx.size(), sizeof(uint32_t));
						pt->opt.clear();
						pt->idx.clear();
						ct++;
					}
					tex = tp;
				}
				auto ps = git._dwpos + git._apos;
				ps += pos;
				gen3data(git._image, ps, git._rect, git.color ? git.color : color, &pt->opt, &pt->idx);
			}
		}
		if (tex && pt->opt.size()) {
			auto nv = pt->opt.size();
			pt->rcb->draw_geometry(renderer, tex, (float*)&pt->opt.data()->pos, vsize, ((float*)&pt->opt.data()->color), vsize,
				((float*)&pt->opt.data()->uv), vsize, nv, pt->idx.data(), pt->idx.size(), sizeof(uint32_t));
		}
		if (ct > 0)
		{
			pt->opt.clear();
			pt->idx.clear();
		}
		else {
			pt->tex = tex;
		}
	}
	else {
		auto nv = pt->opt.size();
		if (pt->tex && pt->opt.size())
		{
			pt->rcb->draw_geometry(renderer, pt->tex, (float*)&pt->opt.data()->pos, vsize, ((float*)&pt->opt.data()->color), vsize,
				((float*)&pt->opt.data()->uv), vsize, nv, pt->idx.data(), pt->idx.size(), sizeof(uint32_t));
		}
	}
}

void r_render_free_tex(sdl3_textdata* p)
{
	if (p)
	{
		for (auto& [k, v] : p->vt) {
			p->rcb->free_texture(v);
		}
		p->vt.clear();
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








//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------



#define white 1, 1, 1
#define red   1, 0, 0
#define green 0, 1, 0
#define blue  0, 0, 1

void test_vkvg(const char* fn, dev_info_c* dc)
{

	vkvg_dev* vctx = new_vkvgdev(dc, 8);
	auto dev = vctx->dev;
	if (!dev)return;
	new_spv_base(vctx->vkdev);
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
				draw_arrow2(ctx, (p[0] * scale) + 0.5, (p[1] * scale) + 0.5);
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
		vkvg_set_source_color(cr, 0xff0020ff);
		vkvg_select_font_face(ctx, "times");
		vkvg_move_to(ctx, 100.f, 100);
		vkvg_set_font_size(ctx, 30);
		static const char* txt = "The quick brown fox jumps over the lazy dog";
		//vkvg_show_text(ctx, txt);

	}

	vkvg_flush(ctx);
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

	{
		const char* filename = "temp/vkvg_gradient.png";
		VkvgSurface surf = vctx->new_surface(256, 256);
		auto ctx = vkvg_create(surf);
		{
			print_time ptt(filename);
			VkvgPattern grad = vkvg_pattern_create_linear(0.0, 0.0, 0.0, 256.0);
			vkvg_pattern_add_color_stop_rgba(grad, 0, 1, 1, 1, 1);
			vkvg_pattern_add_color_stop_rgba(grad, 1, 0, 0, 0, 1);
			vkvg_set_source(ctx, grad);
			vkvg_rectangle(ctx, 0, 0, 256, 256);
			vkvg_fill(ctx);
			VkvgPattern rg = vkvg_pattern_create_radial(115.2, 102.4, 25.6, 102.4, 102.4, 128.0);
			vkvg_pattern_add_color_stop_rgba(rg, 0, 1, 1, 0, 1);
			vkvg_pattern_add_color_stop_rgba(rg, 0.2, 0, 1, 0, 1);
			vkvg_pattern_add_color_stop_rgba(rg, 1, 1, 0, 0, 1);
			vkvg_set_source(ctx, rg);
			vkvg_rectangle(ctx, 0, 0, 256, 256);
			vkvg_fill(ctx);
			vkvg_pattern_destroy(grad);
			vkvg_pattern_destroy(rg);
		}
		vkvg_flush(ctx);
		vkvg_surface_resolve(surf);//msaa采样转换输出
		vkvg_surface_write_to_png(surf, filename);
		vkvg_destroy(ctx);
		vctx->free_surface(surf);
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



rDevice NewDevice(rLibrary library, const char* type/* = "default"*/) {
	return 0;
}
rArray1D NewArray1D(rDevice device, const void* appMemory, rMemoryDeleter deleter, const void* userData, rDataType dataType, uint64_t numElements1) {

	return 0;
}
rArray2D NewArray2D(rDevice device, const void* appMemory, rMemoryDeleter deleter, const void* userData, rDataType dataType, uint64_t numElements1, uint64_t numElements2) {

	return 0;
}
rArray3D NewArray3D(rDevice device, const void* appMemory, rMemoryDeleter deleter, const void* userData, rDataType dataType, uint64_t numElements1, uint64_t numElements2, uint64_t numElements3) {

	return 0;
}
void* MapArray(rDevice device, rArray array) {

	return 0;
}
void UnmapArray(rDevice device, rArray array) {
}
rLight NewLight(rDevice device, const char* type) {

	return 0;
}
rCamera NewCamera(rDevice device, const char* type) {

	return 0;
}
rGeometry NewGeometry(rDevice device, const char* type) {

	return 0;
}
rSpatialField NewSpatialField(rDevice device, const char* type) {

	return 0;
}
rVolume NewVolume(rDevice device, const char* type) {

	return 0;
}
rSurface NewSurface(rDevice device) {

	return 0;
}
rMaterial NewMaterial(rDevice device, const char* type) {

	return 0;
}
rSampler NewSampler(rDevice device, const char* type) {

	return 0;
}
rGroup NewGroup(rDevice device) {

	return 0;
}
rInstance NewInstance(rDevice device, const char* type) {

	return 0;
}
rWorld NewWorld(rDevice device) {

	return 0;
}
rObject NewObject(rDevice device, const char* objectType, const char* type) {

	return 0;
}
void SetParameter(rDevice device, rObject object, const char* name, rDataType dataType, const void* mem) {
}
void UnsetParameter(rDevice device, rObject object, const char* name) {
}
void UnsetAllParameters(rDevice device, rObject object) {
}
void* MapParameterArray1D(rDevice device, rObject object, const char* name, rDataType dataType, uint64_t numElements1, uint64_t* elementStride) {

	return 0;
}
void* MapParameterArray2D(rDevice device, rObject object, const char* name, rDataType dataType, uint64_t numElements1, uint64_t numElements2, uint64_t* elementStride) {

	return 0;
}
void* MapParameterArray3D(rDevice device, rObject object, const char* name, rDataType dataType, uint64_t numElements1, uint64_t numElements2, uint64_t numElements3, uint64_t* elementStride) {

	return 0;
}
void UnmapParameterArray(rDevice device, rObject object, const char* name) {
}
void CommitParameters(rDevice device, rObject object) {
}
void Release(rDevice device, rObject object) {
}
void Retain(rDevice device, rObject object) {
}
const char** GetDeviceSubtypes(rLibrary library) {

	return 0;
}
const char** GetDeviceExtensions(rLibrary library, const char* deviceSubtype) {

	return 0;
}
const char** GetObjectSubtypes(rDevice device, rDataType objectType) {

	return 0;
}
const void* GetObjectInfo(rDevice device, rDataType objectType, const char* objectSubtype, const char* infoName, rDataType infoType) {

	return 0;
}
const void* GetParameterInfo(rDevice device, rDataType objectType, const char* objectSubtype, const char* parameterName, rDataType parameterType, const char* infoName, rDataType infoType) {
	return 0;
}
int  GetProperty(rDevice device, rObject object, const char* name, rDataType type, void* mem, uint64_t size, rWaitMask mask) {

	return 0;
}
rFrame NewFrame(rDevice device) {
	return 0;
}
const void* MapFrame(rDevice device, rFrame frame, const char* channel, uint32_t* width, uint32_t* height, rDataType* pixelType) {

	return 0;
}
void UnmapFrame(rDevice device, rFrame frame, const char* channel) {
}
rRenderer NewRenderer(rDevice device, const char* type) {

	return 0;
}
void RenderFrame(rDevice device, rFrame frame) {
}
int  FrameReady(rDevice device, rFrame frame, rWaitMask mask) {

	return 0;
}
void DiscardFrame(rDevice device, rFrame frame) {
}
class rlib_t
{
public:
	rdev_impl impl;
public:
	rlib_t();
	~rlib_t();

private:

};

rlib_t::rlib_t()
{
	rdev_impl* opt = &impl;
	opt->library = this;
	opt->NewDevice = NewDevice;
	opt->NewArray1D = NewArray1D;
	opt->NewArray2D = NewArray2D;
	opt->NewArray3D = NewArray3D;
	opt->MapArray = MapArray;
	opt->UnmapArray = UnmapArray;
	opt->NewLight = NewLight;
	opt->NewCamera = NewCamera;
	opt->NewGeometry = NewGeometry;
	//opt->NewSpatialField = NewSpatialField;
	//opt->NewVolume = NewVolume;
	opt->NewSurface = NewSurface;
	opt->NewMaterial = NewMaterial;
	opt->NewSampler = NewSampler;
	opt->NewGroup = NewGroup;
	opt->NewInstance = NewInstance;
	opt->NewWorld = NewWorld;
	opt->NewObject = NewObject;
	opt->SetParameter = SetParameter;
	opt->UnsetParameter = UnsetParameter;
	opt->UnsetAllParameters = UnsetAllParameters;
	opt->MapParameterArray1D = MapParameterArray1D;
	opt->MapParameterArray2D = MapParameterArray2D;
	opt->MapParameterArray3D = MapParameterArray3D;
	opt->UnmapParameterArray = UnmapParameterArray;
	opt->CommitParameters = CommitParameters;
	opt->Release = Release;
	opt->Retain = Retain;
	opt->GetDeviceSubtypes = GetDeviceSubtypes;
	opt->GetDeviceExtensions = GetDeviceExtensions;
	opt->GetObjectSubtypes = GetObjectSubtypes;
	opt->GetObjectInfo = GetObjectInfo;
	opt->GetParameterInfo = GetParameterInfo;
	opt->GetProperty = GetProperty;
	opt->NewFrame = NewFrame;
	opt->MapFrame = MapFrame;
	opt->UnmapFrame = UnmapFrame;
	opt->NewRenderer = NewRenderer;
	opt->RenderFrame = RenderFrame;
	opt->FrameReady = FrameReady;
	opt->DiscardFrame = DiscardFrame;

}

rlib_t::~rlib_t()
{
}
rLibrary loadRLibrary(rdev_impl* opt)
{
	auto p = new rlib_t();
	if (opt)
		*opt = p->impl;
	return p;
}

void unloadRLibrary(rLibrary m) {
	auto p = (rlib_t*)m;
	if (m) {
		delete p;
	}
}
namespace vkr {
	vDevice::vDevice()
	{
	}
	vDevice::~vDevice()
	{
	}
	rDevice vDevice::init(rLibrary library, const char* type)
	{
		return rDevice();
	}
	rArray1D vDevice::newArray1D(const void* appMemory, rMemoryDeleter deleter, const void* userData, rDataType dataType, uint64_t numElements1)
	{
		return rArray1D();
	}
	rArray2D vDevice::newArray2D(const void* appMemory, rMemoryDeleter deleter, const void* userData, rDataType dataType, uint64_t numElements1, uint64_t numElements2)
	{
		return rArray2D();
	}
	rArray3D vDevice::newArray3D(const void* appMemory, rMemoryDeleter deleter, const void* userData, rDataType dataType, uint64_t numElements1, uint64_t numElements2, uint64_t numElements3)
	{
		return rArray3D();
	}
	void* vDevice::mapArray(rArray array)
	{
		return nullptr;
	}
	void vDevice::unmapArray(rArray array)
	{
	}
	rLight vDevice::newLight(const char* type)
	{
		return rLight();
	}
	rCamera vDevice::newCamera(const char* type)
	{
		return rCamera();
	}
	rGeometry vDevice::newGeometry(const char* type)
	{
		return rGeometry();
	}
	rSurface vDevice::newSurface()
	{
		return rSurface();
	}
	rMaterial vDevice::newMaterial(const char* type)
	{
		return rMaterial();
	}
	rSampler vDevice::newSampler(const char* type)
	{
		return rSampler();
	}
	rGroup vDevice::newGroup()
	{
		return rGroup();
	}
	rInstance vDevice::newInstance(const char* type)
	{
		return rInstance();
	}
	rWorld vDevice::newWorld()
	{
		return rWorld();
	}
	rObject vDevice::newObject(const char* objectType, const char* type)
	{
		return rObject();
	}
	void vDevice::setParameter(rObject object, const char* name, rDataType dataType, const void* mem)
	{
	}
	void vDevice::unsetParameter(rObject object, const char* name)
	{
	}
	void vDevice::unsetAllParameters(rObject object)
	{
	}
	void* vDevice::mapParameterArray1D(rObject object, const char* name, rDataType dataType, uint64_t numElements1, uint64_t* elementStride)
	{
		return nullptr;
	}
	void* vDevice::mapParameterArray2D(rObject object, const char* name, rDataType dataType, uint64_t numElements1, uint64_t numElements2, uint64_t* elementStride)
	{
		return nullptr;
	}
	void* vDevice::mapParameterArray3D(rObject object, const char* name, rDataType dataType, uint64_t numElements1, uint64_t numElements2, uint64_t numElements3, uint64_t* elementStride)
	{
		return nullptr;
	}
	void vDevice::unmapParameterArray(rObject object, const char* name)
	{
	}
	void vDevice::commitParameters(rObject object)
	{
	}
	void vDevice::release(rObject object)
	{
	}
	void vDevice::retain(rObject object)
	{
	}
	const char** vDevice::getObjectSubtypes(rDataType objectType)
	{
		return nullptr;
	}
	const void* vDevice::getObjectInfo(rDataType objectType, const char* objectSubtype, const char* infoName, rDataType infoType)
	{
		return nullptr;
	}
	const void* vDevice::getParameterInfo(rDataType objectType, const char* objectSubtype, const char* parameterName, rDataType parameterType, const char* infoName, rDataType infoType)
	{
		return nullptr;
	}
	int vDevice::getProperty(rObject object, const char* name, rDataType type, void* mem, uint64_t size, rWaitMask mask)
	{
		return 0;
	}

	rFrame vDevice::newFrame()
	{
		return rFrame();
	}

	const void* vDevice::mapFrame(rFrame frame, const char* channel, uint32_t* width, uint32_t* height, rDataType* pixelType)
	{
		return nullptr;
	}

	void vDevice::unmapFrame(rFrame frame, const char* channel)
	{
	}

	rRenderer vDevice::newRenderer(const char* type)
	{
		return rRenderer();
	}

	void vDevice::renderFrame(rFrame frame)
	{
	}

	int vDevice::frameReady(rFrame frame, rWaitMask mask)
	{
		return 0;
	}

	void vDevice::discardFrame(rFrame frame)
	{
	}

	vDevice* new_vdevice(const char* devname)
	{
		auto p = new vDevice();
		p->lib = loadRLibrary(nullptr);
		auto pl = (rlib_t*)p->lib;
		p->rcb = &pl->impl;
		return p;
	}

	void free_vdevice(vDevice* p)
	{
		if (p)
		{
			unloadRLibrary(p->lib);
			delete p;
		}
	}

}
//!vkr