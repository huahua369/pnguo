/*
2D渲染的一些实现
*/
#include "pch1.h"
#include "pnguo.h"
#include "print_time.h"
#include "event.h"

#ifndef no_cairo_ 
#ifdef __cplusplus
extern "C" {
#endif
#include <cairo/cairo.h>
#ifdef _WIN32
#include <cairo/cairo-win32.h>
#endif
#include <cairo/cairo-svg.h>
#include <cairo/cairo-pdf.h>

#ifndef NO_SVG
#include <librsvg/rsvg.h>
#endif 

#ifdef __cplusplus
}
#endif
#endif
//!no_cairo_


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // !M_PI

#ifndef THREE_SQRT
#define EPSILON 1e-8
#define EPSILON_NUMERIC 1e-4

#define THREE_SQRT sqrt(3)
#define ONE_THIRD (1.0 / 3.0)
#define INTMAX_2 INTMAX_MAX
#endif
#ifndef c_KAPPA90
#define c_KAPPA90 (0.5522847493f)	// Length proportional to radius of a cubic bezier handle for 90deg arcs.
#endif


#ifndef no_cairo_

// 画圆
void draw_circle(cairo_t* cr, const glm::vec2& pos, float r, const glm::ivec2& c)
{
	cairo_arc(cr, pos.x, pos.y, r, 0, 2 * M_PI);
	if (c.x != 0)
	{
		set_color(cr, c.x);
		if (c.y != 0)
			cairo_fill_preserve(cr);
		else
			cairo_fill(cr);
	}
	if (c.y != 0)
	{
		set_color(cr, c.y);
		cairo_stroke(cr);
	}
}
struct qv
{
	glm::vec2 p0, p1, p2;
};
struct q2
{
	glm::vec2 p1, p2;
};
struct cubic_v1
{
	glm::vec2 p0, p1, p2, p3;	// p1 p2是控制点
};
void c2to3(cubic_v1& c)
{
	static double dv = 2.0 / 3.0;
	glm::vec2 c1, c2;
	auto p0 = c.p0;
	auto p1 = c.p1;
	auto p2 = c.p3;
	c1 = p1 - p0; c1 *= dv; c1 += p0;
	c2 = p1 - p2; c2 *= dv; c2 += p2;
	c.p1 = c1;
	c.p2 = c2;
}
cubic_v1 q2c(qv* p)
{
	cubic_v1 cv = {};
	cv.p0 = p->p0;
	cv.p1 = p->p1;
	cv.p2 = p->p1;
	cv.p3 = p->p2;
	c2to3(cv);
	return cv;
}
void save_png_v(path_v* pv, int count, const std::string& fn, bool fy, float sc1)
{
	//std::vector<vertex_i32> vs;
	auto rc = pv->mkbox_npos();
	glm::ivec2 ss = { rc.z ,rc.w };
	//ss += glm::abs(pv->_pos);
	ss *= sc1 + 2;
	printf("save_png_v %d\t%d\n\n", ss.x, ss.y);
	cairo_surface_t* sur = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ss.x, ss.y);
	auto cr = cairo_create(sur);
	auto pxd = (uint32_t*)cairo_image_surface_get_data(sur);
	for (size_t i = 0; i < ss.x * ss.y; i++)
	{
		pxd[i] = 0xff000000;
	}
	cairo_matrix_t flip_y = {};
	cairo_matrix_init(&flip_y, 1, 0, 0, -1, 0, 0); // 垂直翻转
	float sc = sc1 > 0 ? sc1 : 1.0;
	for (size_t x = 0; x < count; x++)
	{
		auto kt = pv; pv++;
		if (!kt)continue;
		glm::vec2 pos = kt->_pos;
		pos *= sc;
		cairo_save(cr);
		//cairo_translate(cr, 0, kt->_baseline);
		cairo_translate(cr, 0, kt->_baseline * sc);
		cairo_translate(cr, pos.x, fy ? -pos.y : pos.y);
		if (fy)
			cairo_transform(cr, &flip_y);
		auto v1 = kt->_data;
		auto v = v1.data();
		auto ks = kt->_data.size();
		for (int i = 0; i < ks; i++)
		{
			auto& it = v[i];
			auto mt = (path_v::vertex_t*)&it;
			mt->p *= sc;
			mt->c *= sc;
			mt->c1 *= sc;
		}
		for (int i = 0; i < ks; i++)
		{
			auto& it = v[i];
			auto mt = (path_v::vertex_t*)&it;
			//vs.push_back(it);
			auto type = (path_v::vtype_e)v[i].type;
			switch (type)
			{
			case path_v::vtype_e::e_vmove:
				cairo_move_to(cr, it.p.x, it.p.y);
				//printf("m%.02f,%.02f ", it.x, it.y * -1.0);
				break;
			case path_v::vtype_e::e_vline:
				cairo_line_to(cr, it.p.x, it.p.y);
				//printf("l%.02f,%.02f ", it.x, it.y * -1.0);
				break;
			case path_v::vtype_e::e_vcurve:
			{
				qv q;
				q.p0 = { v[i - 1].p.x, v[i - 1].p.y };
				q.p1 = { v[i].c.x, v[i].c.y };
				q.p2 = { v[i].p.x, v[i].p.y };
				auto c = q2c(&q);
				cairo_curve_to(cr, c.p1.x, c.p1.y, c.p2.x, c.p2.y, c.p3.x, c.p3.y);
				//printf("q%.02f,%.02f %.02f,%.02f ", it.cx, it.cy * -1.0, it.x, it.y * -1.0);
			}
			break;
			case path_v::vtype_e::e_vcubic:
			{
				cubic_v1 c = {};
				c.p1 = { v[i].c.x, v[i].c.y };
				c.p2 = { v[i].c1.x, v[i].c1.y };
				c.p3 = { v[i].p.x, v[i].p.y };
				cairo_curve_to(cr, c.p1.x, c.p1.y, c.p2.x, c.p2.y, c.p3.x, c.p3.y);
			}
			break;
			default:
				break;
			}
		}
		if (ks > 2)
		{
			cairo_close_path(cr);
			cairo_set_source_rgb(cr, 1, 0.51, 0);
			//cairo_fill_preserve(cr);
		}
		cairo_set_line_width(cr, 1.0);
		cairo_set_source_rgba(cr, 0, 0.51, 1, 0.8);
		cairo_stroke(cr);

		uint32_t cc[2] = { 0xffff8000 ,0xff0080ff };
		int r = 2;
		for (int i = 0; i < ks; i++)
		{
			auto& it = v[i];
			auto mt = (path_v::vertex_t*)&it;
			//vs.push_back(it);
			auto type = (path_v::vtype_e)v[i].type;
			draw_circle(cr, mt->p, r, { cc[x], 0 });
			switch (type)
			{
			case path_v::vtype_e::e_vcurve:
			{
				draw_circle(cr, mt->c, r, { 0xff0080ff, 0 });
			}
			break;
			case path_v::vtype_e::e_vcubic:
			{
				draw_circle(cr, mt->c, r, { 0xff0080ff, 0 });
				draw_circle(cr, mt->c1, r, { 0xff0080ff, 0 });
			}
			break;
			default:
				break;
			}
		}
		cairo_restore(cr);
	}
	cairo_destroy(cr);
	cairo_surface_write_to_png(sur, fn.c_str());
	cairo_surface_destroy(sur);
}

glm::ivec4 get_text_extents(cairo_t* cr, const void* str, int len, font_xi* fx);
class Ruler
{
public:
	Ruler();
	void set_range(double lower, double upper);
	void set_page(double lower, double upper);
	void set_selection(double lower, double upper);

	//void add_track_widget(Gtk::Widget& widget);

	glm::ivec2 get_drawing_size();
	bool draw_scale(cairo_t* cr);
	void draw_marker(cairo_t* cr);
	glm::ivec4 marker_rect();
	bool on_drawing_area_draw(cairo_t* cr, const glm::vec2& pos);
	void on_style_updated();
	void on_prefs_changed();

	void on_motion(void* motion, double x, double y);
	int on_click_pressed(void* click, int n_press, double x, double y);

	void set_context_menu();
	cairo_surface_t* draw_label(cairo_t* cr_in, const std::string& label_value, const glm::ivec2& ts);

	glm::ivec2 drawingsize = {};
	glm::ivec2 oldsize = {};
	//Gtk::DrawingArea* _drawing_area;
	//Inkscape::PrefObserver _watch_prefs;
	//Gtk::Popover* _popover = nullptr;
	int    _orientation = 0;
	//Inkscape::Util::Unit const* _unit;
	double _lower = 0;
	double _upper = 100;
	double _position = 0;
	double _max_size = 100;

	// Page block
	double _page_lower = 0.0;
	double _page_upper = 0.0;

	// Selection block
	double _sel_lower = 0.0;
	double _sel_upper = 0.0;
	double _sel_visible = true;
	int is_yaxisdown = 0;
	bool   has_page = false;
	bool   _backing_store_valid = false;
	bool   ruler_in = 0;//英尺
	cairo_surface_t* _backing_store = 0;
	glm::ivec4 _rect = {};

	std::unordered_map<int, cairo_surface_t*> _label_cache;

	// Cached style properties
	glm::vec4 _shadow = { 0.0,0.00,0.0,0.5 };
	glm::vec4 _foreground = { 1.0,1.0,1.0,1.0 };
	glm::vec4 _cursor_color = { 0.87,0.384,0.384,1.0 };// 1.0, 0.50, 0.0, 1.0};//
	glm::vec4 _page_fill = { 0.12, 0.12, 0.12, 1.0 };
	glm::vec4 _select_fill = {  };
	glm::vec4 _select_stroke = { 0.4,0.4,1.0,1.0 };
	int _font_size = 8;
	uint32_t _back_color = 0xff353535;
	font_xi* fx = 0;
};

class canvas_dev;
class tinyviewcanvas_x
{
public:
	glm::ivec2 vpos = {}, size = {};	// 渲染偏移，大小
	std::vector<path_v*>* _data = 0;
	std::vector<path_v*> draw_data;
	std::vector<glm::vec4> ddrect, vr;
	// 填充颜色\线框颜色
	glm::ivec2 color = { 0x20ffffff, 0xffFF8050 };
	glm::vec4 bx = {};
	glm::mat3 mx = glm::mat3(1.0);
	glm::vec2 last_mouse = {}, eoffset = {};
	glm::ivec4 hover_bx = {};		//当前鼠标对象包围框
	int ckinc = 0;
	int scaleStep = 20;
	int scale = 100;
	int oldscale = 0;
	int minScale = 2, maxScale = 25600;
	int line_width = 1.0;
	cairo_surface_t* _backing_store = 0;

	bool   _backing_store_valid = false;
	bool   _mousezoom = true;
	bool   has_move = false;
	bool   has_scale = false;
public:
	tinyviewcanvas_x();
	~tinyviewcanvas_x();
	void set_size(const glm::ivec2& ss);
	void set_view_move(bool is);	// 鼠标移动视图
	void set_view_scale(bool is);	// 滚轮缩放视图
	void draw(canvas_dev* c);
	void on_button(int idx, int down, const glm::vec2& pos, int clicks);
	void on_motion(const glm::vec2& ps);
	void on_wheel(int deltaY);
	void reset_view();
	glm::ivec4 get_hbox();
	glm::vec4 get_bbox();
	glm::mat3 get_affine();
	void hit_test(const glm::vec2& ps);
private:
	void draw_back();

};



#if 1

void draw_round_rectangle(cairo_t* cr, double x, double y, double width, double height, double r)
{
#ifndef M_PI
	auto M_PI = glm::pi<double>();
#endif
	cairo_move_to(cr, x + r, y);
	cairo_line_to(cr, x + width - r, y);
	cairo_arc(cr, x + width - r, y + r, r, 3 * M_PI / 2, 2 * M_PI);
	//cairo_move_to(cr, x + width, y + r);
	cairo_line_to(cr, x + width, y + height - r);
	cairo_arc(cr, x + width - r, y + height - r, r, 0, M_PI / 2);
	//cairo_move_to(cr, x + width - r, y + height);
	cairo_line_to(cr, x + r, y + height);
	cairo_arc(cr, x + r, y + height - r, r, M_PI / 2, M_PI);
	//cairo_move_to(cr, x, y + height - r);
	cairo_line_to(cr, x, y + r);
	cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2.0);
	cairo_close_path(cr);
}
// r，左右下左
void draw_round_rectangle(cairo_t* cr, double x, double y, double width, double height, const glm::vec4& r)
{
#ifndef M_PI
	auto M_PI = glm::pi<double>();
#endif
	cairo_move_to(cr, x + r.x, y);
	cairo_line_to(cr, x + width - r.y, y);
	if (r.y)
		cairo_arc(cr, x + width - r.y, y + r.y, r.y, 3 * M_PI / 2, 2 * M_PI);
	//cairo_move_to(cr, x + width, y + r.y);
	cairo_line_to(cr, x + width, y + height - r.z);
	if (r.z)
		cairo_arc(cr, x + width - r.z, y + height - r.z, r.z, 0, M_PI / 2);
	//cairo_move_to(cr, x + width - r.z, y + height);
	cairo_line_to(cr, x + r.w, y + height);
	if (r.w)
		cairo_arc(cr, x + r.w, y + height - r.w, r.w, M_PI / 2, M_PI);
	//cairo_move_to(cr, x, y + height - r.w);
	cairo_line_to(cr, x, y + r.x);
	if (r.x > 0)
		cairo_arc(cr, x + r.x, y + r.x, r.x, M_PI, 3 * M_PI / 2.0);
	cairo_close_path(cr);
}
void draw_round_rectangle(cairo_t* cr, const glm::vec4& rc, const glm::vec4& r)
{
	draw_round_rectangle(cr, rc.x, rc.y, rc.z, rc.w, r);
}
// 圆角矩形
void draw_rectangle(cairo_t* cr, const glm::vec4& rc, double r)
{
	if (r > 0)
	{
		draw_round_rectangle(cr, rc.x, rc.y, rc.z, rc.w, r);
	}
	else {
		cairo_rectangle(cr, rc.x, rc.y, rc.z, rc.w);
	}
}
void draw_triangle(cairo_t* cr, const glm::vec2& pos, const glm::vec2& size, const glm::vec2& dirspos)
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
	cairo_move_to(cr, pos.x + tpos[0].x, pos.y + tpos[0].y);
	cairo_line_to(cr, pos.x + tpos[1].x, pos.y + tpos[1].y);
	cairo_line_to(cr, pos.x + tpos[2].x, pos.y + tpos[2].y);
	cairo_close_path(cr);
}

//vg_style_t
	//float* dash = 0;		// 虚线逗号/空格分隔的数字
	//int dashOffset = 0;

void fill_stroke(cairo_t* cr, vg_style_t* st) {
	bool stroke = st && st->thickness > 0 && st->color > 0;
	if (!cr || !st)return;
	if (st->fill)
	{
		set_color(cr, st->fill);
		if (stroke)
			cairo_fill_preserve(cr);
		else
			cairo_fill(cr);
	}
	if (stroke)
	{
		cairo_set_line_width(cr, st->thickness);
		if (st->cap > 0)
			cairo_set_line_cap(cr, (cairo_line_cap_t)st->cap);
		if (st->join > 0)
			cairo_set_line_join(cr, (cairo_line_join_t)st->join);
		if (st->dash.v > 0 || st->dash_p)
		{
			double dashes[64] = {};
			uint64_t x = 1;
			auto t = dashes;
			int num_dashes = st->dash_num;
			if (num_dashes > 64)num_dashes = 64;
			if (st->dash_p)
			{
				for (size_t i = 0; i < num_dashes; i++)
				{
					*t = st->dash_p[i]; t++;
				}
			}
			else
			{
				if (num_dashes > 8)num_dashes = 8;
				for (size_t i = 0; i < num_dashes; i++)
				{
					*t = st->dash.v8[i]; t++;
				}
			}
			if (num_dashes > 0)
				cairo_set_dash(cr, dashes, num_dashes, st->dash_offset);
		}
		set_color(cr, st->color);
		cairo_stroke(cr);
	}
}
void fill_stroke(cairo_t* cr, uint32_t fill, uint32_t color, int linewidth, bool isbgr) {
	bool stroke = linewidth > 0 && color;
	if (fill)
	{
		if (isbgr)
			set_color_bgr(cr, fill);
		else
			set_color(cr, fill);
		if (stroke)
			cairo_fill_preserve(cr);
		else
			cairo_fill(cr);
	}
	if (stroke)
	{
		cairo_set_line_width(cr, linewidth);
		if (isbgr)
			set_color_bgr(cr, color);
		else
			set_color(cr, color);
		cairo_stroke(cr);
	}
}
// 画圆
void draw_circle(cairo_t* cr, const glm::vec2& pos, float r)
{
	cairo_arc(cr, pos.x, pos.y, r, 0, 2 * M_PI);
}

void draw_ellipse(cairo_t* cr, const glm::vec2& c, const glm::vec2& r)
{
	double cx = c.x, cy = c.y, rx = r.x, ry = r.y;
	if (rx > 0.0f && ry > 0.0f) {
		cairo_move_to(cr, cx + rx, cy);
		cairo_curve_to(cr, cx + rx, cy + ry * c_KAPPA90, cx + rx * c_KAPPA90, cy + ry, cx, cy + ry);
		cairo_curve_to(cr, cx - rx * c_KAPPA90, cy + ry, cx - rx, cy + ry * c_KAPPA90, cx - rx, cy);
		cairo_curve_to(cr, cx - rx, cy - ry * c_KAPPA90, cx - rx * c_KAPPA90, cy - ry, cx, cy - ry);
		cairo_curve_to(cr, cx + rx * c_KAPPA90, cy - ry, cx + rx, cy - ry * c_KAPPA90, cx + rx, cy);
		cairo_close_path(cr);
	}
}

void draw_polyline(cairo_t* cr, const glm::vec2& pos, const glm::vec2* points, int points_count, unsigned int col, bool closed, float thickness)
{
	if (!cr || !points || points_count < 2 || !col)return;
	cairo_save(cr);
	cairo_translate(cr, pos.x, pos.y);
	cairo_move_to(cr, points->x, points->y);
	for (size_t i = 1; i < points_count; i++)
	{
		cairo_line_to(cr, points[i].x, points[i].y);
	}
	if (closed) { cairo_close_path(cr); }
	cairo_set_line_width(cr, thickness);
	if (col) {
		set_color(cr, col);
		cairo_stroke(cr);
	}
	cairo_restore(cr);
}
void draw_polyline(cairo_t* cr, const glm::vec2* points, int points_count, bool closed)
{
	if (!cr || !points || points_count < 2)return;
	cairo_move_to(cr, points->x, points->y);
	for (size_t i = 1; i < points_count; i++)
	{
		cairo_line_to(cr, points[i].x, points[i].y);
	}
	if (closed) { cairo_close_path(cr); }
}

void draw_polyline(cairo_t* cr, const PathsD* p, bool closed)
{
	if (!cr || !p || p->size() < 1)return;
	auto& d = *p;
	auto length = d.size();
	for (size_t i = 0; i < length; i++)
	{
		auto points = d[i];
		cairo_move_to(cr, points[0].x, points[0].y);
		auto points_count = points.size();
		for (size_t i = 1; i < points_count; i++)
		{
			cairo_line_to(cr, points[i].x, points[i].y);
		}
		if (closed) { cairo_close_path(cr); }
	}
}

void draw_polylines(cairo_t* cr, const glm::vec2& pos, const glm::vec2* points, int points_count, int* idx, int idx_count, unsigned int col, float thickness)
{
	if (!cr || !points || points_count < 2 || idx_count < 2 || !col)return;
	cairo_save(cr);
	cairo_translate(cr, pos.x, pos.y);
	cairo_set_line_width(cr, thickness);
	int nc = 0;
	for (size_t i = 0; i < idx_count; i++)
	{
		auto x = idx[i];
		if (x < 0)
		{
			if (nc > 1) {
				set_color(cr, col);
				cairo_stroke(cr);
			}
			nc = 0;
			continue;
		}
		if (nc == 0)
			cairo_move_to(cr, points[x].x, points[x].y);
		else
			cairo_line_to(cr, points[x].x, points[x].y);
		nc++;
	}
	if (nc > 1) {
		set_color(cr, col);
		cairo_stroke(cr);
	}
	cairo_restore(cr);
}
void draw_rect(cairo_t* cr, const glm::vec4& rc, uint32_t fill, uint32_t color, double r, int linewidth)
{
	cairo_as _ss_(cr);
	draw_rectangle(cr, rc, r);
	fill_stroke(cr, fill, color, linewidth, false);
}
//glm::ivec2 layout_text_x::get_text_rect(size_t idx, const void* str8, int len, int fontsize)

void draw_text(cairo_t* cr, layout_text_x* ltx, const void* str, int len, glm::vec4 text_rc, text_style_t* st, glm::ivec2* orc)
{
	glm::vec4 rc = text_rc;
	ltx->tem_rtv.clear();
	ltx->build_text(st->font, rc, st->text_align, str, len, st->font_size, ltx->tem_rtv);
	ltx->update_text();
	if (orc)
	{
		//if (orc->x > rc.z)
		orc->x = rc.z;
		//if (orc->y > rc.w)
		orc->y = rc.w;
	}
	cairo_as _ss_(cr);
	if (st->clip && text_rc.z > 0 && text_rc.w > 0) {
		draw_rectangle(cr, text_rc, 0);
		cairo_clip(cr);
	}
	if (st->text_color_shadow)
	{
		cairo_as _aa_(cr);
		cairo_translate(cr, st->shadow_pos.x, st->shadow_pos.y);
		ltx->draw_text(cr, ltx->tem_rtv, st->text_color_shadow);
	}
	//ltx->draw_rect_rc(cr, ltx->tem_rtv, 0xffff8000);
	ltx->draw_text(cr, ltx->tem_rtv, st->text_color);
}
void draw_rctext(cairo_t* cr, layout_text_x* ltx, text_tx* p, int count, text_style_tx* stv, int st_count, glm::ivec4* clip)
{
	if (!stv || st_count < 1 || !cr || !ltx || !p || count < 1)return;
	cairo_as _ss_(cr);
	if (clip && clip->z > 0 && clip->w > 0) {
		glm::vec4 cliprc = *clip;
		draw_rectangle(cr, cliprc, 0);
		cairo_clip(cr);
	}
	for (size_t i = 0; i < count; i++)
	{
		auto& it = p[i];
		auto idx = it.st_index < st_count ? it.st_index : 0;
		auto& st = stv[idx];
		float pad = st.rcst.thickness > 1 ? 0.0 : -0.5;
		auto ss = it.rc;
		auto draw_sbox = ss.z > 0 && ss.w > 0;
		if (draw_sbox)
		{
			ss.x += pad; ss.y += pad;
			auto r = ss.z < st.rcst.round * 2 || ss.w < st.rcst.round * 2 ? 0 : st.rcst.round;
			draw_rectangle(cr, ss, r);
			fill_stroke(cr, &st.rcst);
		}
		draw_text(cr, ltx, it.txt, it.len, it.trc, &st.st);
	}
}


dtext_cache::dtext_cache()
{
}
void dtext_cache::reset(cairo_t* crin, const glm::ivec2& vsize)
{
	if (vsize.x < 1 || vsize.y < 1)return;
	if (vsize != size || cr_in != crin || !cr || !surface)
	{
		cr_in = crin; size = vsize;
		free_surface();
	}
	else {
		return;
	}
	auto surface_in = cairo_get_target(cr_in);
	surface = cairo_surface_create_similar_image(surface_in, CAIRO_FORMAT_ARGB32, vsize.x, vsize.y);
	cr = cairo_create(surface);
}

dtext_cache::~dtext_cache()
{
	free_surface();
}
void dtext_cache::free_surface()
{
	if (cr) {
		cairo_destroy(cr);
	}
	if (surface)
		cairo_surface_destroy(surface);
	cr = 0;
	surface = 0;
}

void dtext_cache::save2png(const char* name)
{
	if (surface)
		cairo_surface_write_to_png(surface, name && *name ? name : "surface_label.png");
}

void dtext_cache::clear_color(uint32_t color)
{
	if (cr)
	{
		//set_color(cr, color);	// 设置蓝色 
		//cairo_paint(cr);		// 填充整个Surface 
		auto p = (uint32_t*)cairo_image_surface_get_data(surface);
		size_t n = size.x * size.y;
		if (color == 0) {
			memset(p, 0, n * sizeof(int));
		}
		else {
			for (size_t i = 0; i < n; i++)
			{
				p[i] = color;
			}
		}
	}
}


text_draw_t* new_text_drawable(const char* str, int len, layout_text_x* ltx, const glm::vec4& box_rc, text_style_t* st, glm::ivec2* orc)
{
	if (!str || len < 1 || !*str || !ltx || !st)return nullptr;
	glm::vec4 rc = box_rc;
	ltx->tem_rtv.clear();
	ltx->build_text(st->font, rc, st->text_align, str, len, st->font_size, ltx->tem_rtv);
	ltx->update_text();
	if (orc)
	{ 
		orc->x = rc.z; 
		orc->y = rc.w;
	}
	//cairo_as _ss_(cr);
	//if (st->clip && text_rc.z > 0 && text_rc.w > 0) {
	//	draw_rectangle(cr, text_rc, 0);
	//	cairo_clip(cr);
	//}
	//if (st->text_color_shadow)
	//{
	//	cairo_as _aa_(cr);
	//	cairo_translate(cr, st->shadow_pos.x, st->shadow_pos.y);
	//	ltx->draw_text(cr, ltx->tem_rtv, st->text_color_shadow);
	//}
	////ltx->draw_rect_rc(cr, ltx->tem_rtv, 0xffff8000);
	//ltx->draw_text(cr, ltx->tem_rtv, st->text_color);
	return nullptr;
}

void draw_draw_texts(text_draw_t* p)
{
	glm::ivec2 r = {}, r0 = {};
	if (!p || !p->cr || !p->st || !p->ltx || !p->color || !p->text || !p->text_rc || p->count < 1)return;
	p->box_rc.x = p->text_rc[0].x;
	p->box_rc.y = p->text_rc[0].y;
	cairo_as _ss_(p->cr);
	for (size_t i = 0; i < p->count; i++)
	{
		p->st->text_color = p->color[i];
		draw_text(p->cr, p->ltx, p->text[i].c_str(), -1, p->text_rc[i], p->st, &r0);
		auto x = p->text_rc[i].x;
		auto y = p->text_rc[i].y;
		if (x < p->box_rc.x)
			p->box_rc.x = x;
		if (y < p->box_rc.y)
			p->box_rc.y = y;
		r0.x += x;
		r0.y += y;
		if (p->box_rc.z < r0.x)
			p->box_rc.z = r0.x;
		if (p->box_rc.w < r0.y)
			p->box_rc.w = r0.y;

	}
	return;
}

void clip_cr(cairo_t* cr, const glm::ivec4& clip)
{
	glm::vec4 cliprc = clip;
	if (clip.z > 0 && clip.w > 0) {
		draw_rectangle(cr, cliprc, 0);
		cairo_clip(cr);
	}
}
void draw_ge(cairo_t* cr, void* p, int count)
{
	auto t = (char*)p;
	if (!(t && p && cr && count > 0))return;
	for (size_t i = 0; i < count; i++)
	{
		auto type = ((rect_b*)t)->type;
		switch (type)
		{
		case eg_e::e_rect:
		{
			auto dp = (rect_b*)t;
			if (dp->r.x > 0 && dp->r.y < 0)
			{
				draw_rect(cr, dp->rc, dp->fill, dp->color, dp->r.x, dp->thickness);
			}
			else {
				draw_round_rectangle(cr, dp->rc, dp->r);
				fill_stroke(cr, dp->fill, dp->color, dp->thickness);
			}
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_text:
		{
			auto dp = (text_b*)t;
			draw_text(cr, dp->ltx, dp->str, dp->len, dp->text_rc, dp->st);
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_circle:
		{
			auto dp = (circle_b*)t;
			draw_circle(cr, dp->pos, dp->r);
			fill_stroke(cr, dp->fill, dp->color, dp->thickness);
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_ellipse:
		{
			auto dp = (ellipse_b*)t;
			draw_ellipse(cr, dp->pos, dp->r);
			fill_stroke(cr, dp->fill, dp->color, dp->thickness);
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_triangle:
		{
			auto dp = (triangle_b*)t;
			draw_triangle(cr, dp->pos, dp->size, dp->dirspos);
			fill_stroke(cr, dp->fill, dp->color, dp->thickness);
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_polyline:
		{
			auto dp = (polyline_b*)t;
			{
				cairo_as _ss_(cr);
				cairo_translate(cr, dp->pos.x, dp->pos.y);
				draw_polyline(cr, dp->points, dp->points_count, dp->closed);
				fill_stroke(cr, dp->fill, dp->color, dp->thickness);
			}
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_polylines:
		{
			auto dp = (polylines_b*)t;
			draw_polylines(cr, dp->pos, dp->points, dp->points_count, dp->idx, dp->idx_count, dp->color, dp->thickness);
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_image:
		{
			auto dp = (image_b*)t;
			if (dp->sliced.x < 1)
			{
				draw_image(cr, (cairo_surface_t*)dp->image, dp->pos, dp->rc, dp->color, dp->dsize);
			}
			else
			{
				draw_image(cr, (cairo_surface_t*)dp->image, dp->pos, dp->rc, dp->color, dp->dsize, dp->sliced);
			}
			t += sizeof(dp[0]);
		}
		break;
		default:
			break;
		}
	}
}

cairo_surface_t* new_clip_rect(int r)
{
	glm::vec4 rc = { 0,0,r * 4,r * 4 };
	//auto d = new uint32_t[rc.z * rc.w];
	auto rrc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, rc.z, rc.w);
	//auto rrc = cairo_image_surface_create_for_data((unsigned char*)d, CAIRO_FORMAT_ARGB32, rc.z, rc.w, rc.z * 4);
	auto cr = cairo_create(rrc);
	set_color(cr, -1);
	draw_rectangle(cr, rc, r);
	cairo_fill(cr);
	cairo_destroy(cr);
	return rrc;
}
void clip_rect(cairo_t* cr, cairo_surface_t* r)
{
	if (!cr || !r)return;
	cairo_save(cr);
	auto a = cairo_get_target(cr);
	glm::vec2 aw = { cairo_image_surface_get_width(a), cairo_image_surface_get_height(a) };
	cairo_set_operator(cr, CAIRO_OPERATOR_DEST_IN);
	float w = cairo_image_surface_get_width(r);
	float h = cairo_image_surface_get_height(r);
	glm::vec2 w2 = { w * 0.5, h * 0.5 };

	cairo_save(cr);
	cairo_rectangle(cr, 0, 0, w2.x, w2.y);//左上角
	cairo_clip(cr);
	cairo_new_path(cr);
	cairo_set_source_surface(cr, r, 0, 0);
	cairo_paint(cr);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_rectangle(cr, aw.x - w2.x, 0, w2.x, w2.y);//右上角
	cairo_clip(cr);
	cairo_new_path(cr);
	cairo_set_source_surface(cr, r, aw.x - w, 0);
	cairo_paint(cr);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_rectangle(cr, 0, aw.y - w2.y, w2.x, w2.y);//左下角
	cairo_clip(cr);
	cairo_new_path(cr);
	cairo_set_source_surface(cr, r, 0, aw.y - h);
	cairo_paint(cr);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_rectangle(cr, aw.x - w2.x, aw.y - w2.y, w2.x, w2.y);//右下角
	cairo_clip(cr);
	cairo_new_path(cr);
	cairo_set_source_surface(cr, r, aw.x - w, aw.y - h);
	cairo_paint(cr);
	cairo_restore(cr);

	cairo_restore(cr);
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

void draw_rectangle_gradient(cairo_t* cr, int width, int height, const rect_shadow_t* pr)
{
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
		glm::vec4 color = mix_colors(rs.cfrom, rs.cto, ratio);
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
		cairo_pattern_t* rg = cairo_pattern_create_radial(point[0], point[1], 0, point[0], point[1], radius);
		for (auto& stop : color_stops)
		{
			cairo_pattern_add_color_stop_rgba(rg, stop.o, stop.x, stop.y, stop.z, stop.w);
		}
		cairo_move_to(cr, point[0], point[1]);
		double	angle1 = M_PI + 0.5 * M_PI * i, angle2 = angle1 + 0.5 * M_PI;
		cairo_arc(cr, point[0], point[1], radius, angle1, angle2);
		cairo_set_source(cr, rg);
		cairo_fill(cr);
		cairo_pattern_destroy(rg);
		i++;
	}

	//# draw four sides through linear gradient
	//# top side

	cairo_pattern_t* lg_top = cairo_pattern_create_linear(side_top[0], side_top[1] + radius, side_top[0], side_top[1]);
	for (auto& stop : color_stops)
	{
		cairo_pattern_add_color_stop_rgba(lg_top, stop.o, stop.x, stop.y, stop.z, stop.w);
	}
	draw_rectangle(cr, side_top, 0);
	cairo_set_source(cr, lg_top);
	cairo_fill(cr);

	//# bottom side
	cairo_pattern_t* lg_bottom = cairo_pattern_create_linear(side_bottom[0], side_bottom[1], side_bottom[0], side_bottom[1] + radius);
	for (auto& stop : color_stops)
	{
		cairo_pattern_add_color_stop_rgba(lg_bottom, stop.o, stop.x, stop.y, stop.z, stop.w);
	}
	draw_rectangle(cr, side_bottom, 0);
	cairo_set_source(cr, lg_bottom);
	cairo_fill(cr);

	//# left side
	cairo_pattern_t* lg_left = cairo_pattern_create_linear(side_left[0] + radius, side_left[1], side_left[0], side_left[1]);
	for (auto& stop : color_stops)
	{
		cairo_pattern_add_color_stop_rgba(lg_left, stop.o, stop.x, stop.y, stop.z, stop.w);
	}
	draw_rectangle(cr, side_left, 0);
	cairo_set_source(cr, lg_left);
	cairo_fill(cr);

	//# right side
	cairo_pattern_t* lg_right = cairo_pattern_create_linear(side_right[0], side_right[1], side_right[0] + radius, side_right[1]);
	for (auto& stop : color_stops)
	{
		cairo_pattern_add_color_stop_rgba(lg_right, stop.o, stop.x, stop.y, stop.z, stop.w);
	}
	draw_rectangle(cr, side_right, 0);
	cairo_set_source(cr, lg_right);
	cairo_fill(cr);

	cairo_pattern_destroy(lg_top);
	cairo_pattern_destroy(lg_bottom);
	cairo_pattern_destroy(lg_left);
	cairo_pattern_destroy(lg_right);
}

cairo_as::cairo_as(cairo_t* p) :cr(p)
{
	if (cr)
		cairo_save(cr);
}

cairo_as::~cairo_as()
{
	if (cr)
		cairo_restore(cr);
}

canvas_dev::canvas_dev()
{

}



canvas_dev::~canvas_dev()
{
	if (surface)
		cairo_surface_destroy(surface);
	if (cr)
		cairo_destroy(cr);
}
canvas_dev* new_cairo(cairo_surface_t* surface, canvas_dev* p = 0)
{
	if (surface)
	{
		auto cr = cairo_create(surface);
		if (cr)
		{
			if (!p)
				p = new canvas_dev();
			p->surface = surface;
			p->cr = cr;
			p->pixel = (uint32_t*)cairo_image_surface_get_data(surface);
			p->dsize.x = cairo_image_surface_get_width(surface);
			p->dsize.y = cairo_image_surface_get_height(surface);
		}
	}
	return p;
}
canvas_dev* canvas_dev::new_cdev(const glm::ivec2& size, uint32_t* data)
{
	cairo_surface_t* surface = 0;
	canvas_dev* p = 0;
	if (!data)
	{
		p = new canvas_dev();
		if (!p)return p;
		p->tdata.resize(size.x * size.y);
		data = p->tdata.data();
	}
	if (size.x > 0 && size.y > 0 && data)
	{
		surface = cairo_image_surface_create_for_data((unsigned char*)data, CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
	}
	return new_cairo(surface, p);
}

canvas_dev* canvas_dev::new_cdev_svg(const glm::ivec2& size, const char* svgfn)
{
	auto surface = cairo_svg_surface_create(svgfn, size.x, size.y);
	return new_cairo(surface);
}

canvas_dev* canvas_dev::new_cdev_dc(void* hdc)
{
#ifdef _WIN32
	auto surface = cairo_win32_surface_create((HDC)hdc);// 根据HDC创建表面
#else
	cairo_surface_t* surface = 0;
#endif 
	return new_cairo(surface);
}

void canvas_dev::free_cdev(canvas_dev* p)
{
	if (p)delete p;
}

void canvas_dev::save_png(const char* fn)
{
	if (surface && fn && *fn)
		cairo_surface_write_to_png(surface, fn);
}
void canvas_dev::save()
{
	cairo_save(cr);
}

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

glm::vec4 colorv4(uint32_t rgba) {
	return { SP_RGBA32_R_F(rgba), SP_RGBA32_G_F(rgba), SP_RGBA32_B_F(rgba), SP_RGBA32_A_F(rgba) };
}
glm::vec4 colorv4_bgr(uint32_t bgra) {
	return { SP_RGBA32_B_F(bgra), SP_RGBA32_G_F(bgra),SP_RGBA32_R_F(bgra), SP_RGBA32_A_F(bgra) };
}
void set_color(cairo_t* cr, uint32_t rgba)
{
	cairo_set_source_rgba(cr, SP_RGBA32_R_F(rgba),
		SP_RGBA32_G_F(rgba),
		SP_RGBA32_B_F(rgba),
		SP_RGBA32_A_F(rgba));
}
void set_color_bgr(cairo_t* cr, uint32_t c)
{
	cairo_set_source_rgba(cr,
		SP_RGBA32_B_F(c),
		SP_RGBA32_G_F(c),
		SP_RGBA32_R_F(c),
		SP_RGBA32_A_F(c));
}
void set_color_a(cairo_t* cr, uint32_t rgba, double a)
{
	cairo_set_source_rgba(cr, SP_RGBA32_R_F(rgba),
		SP_RGBA32_G_F(rgba),
		SP_RGBA32_B_F(rgba),
		SP_RGBA32_A_F(rgba) * a);
}
void set_source_rgba(cairo_t* cr, uint32_t rgba)
{
	cairo_set_source_rgba(cr, SP_RGBA32_R_F(rgba),
		SP_RGBA32_G_F(rgba),
		SP_RGBA32_B_F(rgba),
		SP_RGBA32_A_F(rgba));
}
void set_source_rgba(cairo_t* cr, glm::vec4 rgba)
{
	cairo_set_source_rgba(cr, rgba.x, rgba.y, rgba.z, rgba.w);
}
void set_source_rgba_x(cairo_t* cr, glm::vec4 rgba)
{
	cairo_set_source_rgba(cr, rgba.x * rgba.w, rgba.y * rgba.w, rgba.z * rgba.w, 1.0);
}

glm::vec2 v2xm3(const glm::vec2& v, const glm::mat3& m) {
	glm::vec3 ps(v, 1.0);
	return m * ps;
}

glm::vec4 get_boxm3(const glm::vec4& v, const glm::mat3& m) {
	glm::vec3 ps = { v.x,v.y,1.0 };
	glm::vec3 ps1 = { v.z,v.w ,1.0 };
	ps = m * ps;
	ps1 = m * ps1;
	return glm::vec4(ps.x, ps.y, ps1.x, ps1.y);
}


glm::ivec4 canvas_dev::get_text_extents(const void* str, int len, font_xi* fx)
{
	char* t = (char*)str;
	if (!str || !(*t))
	{
		return  glm::vec4();
	}
	if (len < 1)len = strlen(t);
	cairo_text_extents_t extents = {};
	cairo_select_font_face(cr, fx->family, (cairo_font_slant_t)fx->slant, (cairo_font_weight_t)fx->weight_bold);
	cairo_set_font_size(cr, fx->font_size);
	std::string str1((char*)str, len);
	cairo_text_extents(cr, (char*)str1.c_str(), &extents);
	return glm::vec4(extents.width, extents.height, extents.x_bearing, extents.y_bearing);
}

int fc1_f(float a)
{
	return a > 0 ? ceil(a) : floor(a);
}
glm::ivec4 fcv4(const glm::vec4& a)
{
	return { fc1_f(a.x), fc1_f(a.y), fc1_f(a.z), fc1_f(a.w) };
}
glm::ivec4 get_text_extents(cairo_t* cr, const void* str, int len, font_xi* fx)
{
	char* t = (char*)str;
	if (!str || !(*t))
	{
		return  glm::vec4();
	}
	if (len < 1)len = strlen(t);
	cairo_text_extents_t extents = {};
	cairo_select_font_face(cr, fx->family, (cairo_font_slant_t)fx->slant, (cairo_font_weight_t)fx->weight_bold);
	cairo_set_font_size(cr, fx->font_size);
	std::string str1((char*)str, len);
	cairo_text_extents(cr, (char*)str1.c_str(), &extents);
	auto r = glm::vec4((extents.width), extents.height, extents.x_bearing, extents.y_bearing);
	return fcv4(r);
}


void canvas_dev::begin(uint32_t color) {
	cairo_save(cr); set_clear(color);
}
void canvas_dev::end() {
	cairo_restore(cr);
}
void canvas_dev::set_pos(const glm::vec2& pos)
{
	cairo_translate(cr, pos.x, pos.y);
}
void canvas_dev::set_dscale(const glm::vec2& sc)
{
	cairo_surface_set_device_scale(surface, sc.x, sc.y);
}
void canvas_dev::set_dscale(double sc)
{
	cairo_surface_set_device_scale(surface, sc, sc);
}
void canvas_dev::set_dscalei(int& zoom)
{
	if (zoom < 1)zoom = 1;
	if (zoom > max_zoom)zoom = max_zoom;
	double sc = zoom / 100.0;
	cairo_surface_set_device_scale(surface, sc, sc);
}
void canvas_dev::set_scale(double sc)
{
	cairo_scale(cr, sc, sc);
}
void canvas_dev::set_scalei(int& zoom)
{
	//if (zoom < 1)zoom = 1;
	//if (zoom > max_zoom)zoom = max_zoom;
	double sc = zoom / 100.0;
	cairo_scale(cr, sc, sc);
}
uint32_t* canvas_dev::data()
{
	return surface ? (uint32_t*)cairo_image_surface_get_data(surface) : tdata.data();
}
void canvas_dev::set_clear(uint32_t color)
{
	if (pixel && cr)
	{
		if (color == 0) {
			memset(tdata.data(), 0, tdata.size() * sizeof(int));
		}
		else {
			for (auto& it : tdata) {
				it = color;
			}
		}
		// clear background
		//cairo_save(cr);
		//set_color(cr, color);
		//cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		//cairo_paint(cr);
		//cairo_restore(cr);
		//cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	}
}
/*
CAIRO_LINE_CAP_BUTT,	在起点（终点）开始（停止）直线
CAIRO_LINE_CAP_ROUND,	圆形结尾，圆心为终点
CAIRO_LINE_CAP_SQUARE	正方形结尾，正方形的中心是终点
CAIRO_LINE_JOIN_MITER,	尖角
CAIRO_LINE_JOIN_ROUND,	圆角
CAIRO_LINE_JOIN_BEVEL	截断			 */
template<class T>
void draw_path0(cairo_t* cr, T* p, style_path_t* st, glm::vec2 pos, glm::vec2 scale)
{
	if (!p || !st || (!st->fill_color && !st->stroke_color || !cr))return;
	auto t = p->v;
	bool stroke = false;
	cairo_save(cr);
	pos += st->pos;
	pos.x += p->x;
	pos.y += p->y;
	if (st->flip_y)
	{
		cairo_matrix_t flip_y = {};
		cairo_matrix_init(&flip_y, 1, 0, 0, -1, 0, 0); // 垂直翻转
		cairo_set_matrix(cr, &flip_y);
		pos.y = -pos.y;
	}
	cairo_translate(cr, pos.x, pos.y);
	if (st->scale.x > 0 && st->scale.y > 0)
	{
		cairo_scale(cr, st->scale.x, st->scale.y);
	}
	else if (scale.x > 0 && scale.y > 0)
	{
		cairo_scale(cr, scale.x, scale.y);
	}
	if (st->line_width > 0 && st->stroke_color)
	{
		stroke = true;
		cairo_set_line_width(cr, st->line_width);
		cairo_set_line_cap(cr, (cairo_line_cap_t)st->cap);
		cairo_set_line_join(cr, (cairo_line_join_t)st->join);
	}
	if (st->dash.dashes && st->dash.num_dashes)
	{
		cairo_set_dash(cr, st->dash.dashes, st->dash.num_dashes, st->dash.offset);
	}
#ifdef tsave__test
	std::vector<vertex_t16> tv16;
	std::vector<std::vector<vertex_t16>> v;
	tv16.reserve(p->count);
#endif
	auto mt = *t;
	auto xt = *t;
	for (size_t i = 0; i < p->count; i++, t++)
	{
		switch ((vte_e)t->type)
		{
		case vte_e::e_vmove:
		{
			if (i > 0)
			{
				if (xt.x == mt.x && xt.y == mt.y)
					cairo_close_path(cr);
#ifdef tsave__test
				v.push_back(tv16); tv16.clear();
#endif
			}
			mt = *t;
			cairo_move_to(cr, t->x, t->y);
		}break;
		case vte_e::e_vline:
		{
			cairo_line_to(cr, t->x, t->y);
		}break;
		case vte_e::e_vcurve:
		{
			static double dv = 2.0 / 3.0;
			auto p0 = glm::vec2(xt.x, xt.y);
			auto p1 = glm::vec2(t->cx, t->cy);
			auto p2 = glm::vec2(t->x, t->y);
			glm::vec2 c1, c2;
			{
#if 1
				c1 = p1 - p0; c1 *= dv; c1 += p0;
				c2 = p1 - p2; c2 *= dv; c2 += p2;
#else
				static double dv = 2.0 / 3.0;
				auto cx1 = p0.x + 2.0f / 3.0f * (p1.x - p0.x);
				auto cy1 = p0.y + 2.0f / 3.0f * (p1.y - p0.y);
				auto cx2 = p2.x + 2.0f / 3.0f * (p1.x - p2.x);
				auto cy2 = p2.y + 2.0f / 3.0f * (p1.y - p2.y);
				c1 = { cx1,cy1 }; c2 = { cx2,cy2 };
#endif
			}
			//	C0 = Q0
			//	C1 = Q0 + (2 / 3) (Q1 - Q0)
			//	C2 = Q2 + (2 / 3) (Q1 - Q2)
			//	C3 = Q2
			cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, t->x, t->y);
		}break;
		case vte_e::e_vcubic:
		{
			cairo_curve_to(cr, t->cx, t->cy, t->cx1, t->cy1, t->x, t->y);

		}break;
		default:
			break;
		}
#ifdef tsave__test
		tv16.push_back(*t);
#endif
		xt = *t;
	}
	if (p->count > 2)
	{
		if (xt.x == mt.x && xt.y == mt.y)
			cairo_close_path(cr);
	}

	if (st->fill_color)
	{
		set_color(cr, st->fill_color);
		if (stroke)
			cairo_fill_preserve(cr);
		else
			cairo_fill(cr);
	}
	if (stroke)
	{
		set_color(cr, st->stroke_color);
		cairo_stroke(cr);
	}
	cairo_restore(cr);
}


struct path_txf
{
	float x, y;	// 坐标
	int count;		// 数量
	path_v::vertex_tf* v;	// 端点 
};
void draw_path_v(cairo_t* cr, path_v* p, style_path_t* st)
{
	if (!p || !st || p->_data.empty())return;
	path_txf tf = {};
	tf.count = p->_data.size();
	tf.v = (path_v::vertex_tf*)p->data();
	tf.x = p->_pos.x;
	tf.y = p->_pos.y + p->_baseline;
	glm::vec2 pos = {}, sc = {};
	draw_path0(cr, &tf, st, pos, sc);
}
void canvas_dev::draw_path(path_v* p, style_path_t* st)
{
	if (!p || !st || p->_data.empty())return;
	draw_path_v(cr, p, st);
}

void canvas_dev::draw_circle(std::vector<std::vector<glm::vec2>>* ov, float r, style_path_t* st)
{
	if (!ov || ov->empty())return;
	path_v v;
	for (auto& it : *ov)
	{
		for (auto& ct : it)
		{
			v.addCircle(ct, r);
		}
	}
	draw_path(&v, st);
}
void canvas_dev::draw_rect(const glm::vec2& size, style_path_t* st)
{
	path_v v;
	v.addRect({ 0,0,size }, {});
	draw_path(&v, st);
}
void canvas_dev::draw_circle(std::vector<glm::vec2>* ov, float r, style_path_t* st)
{
	if (!ov || ov->empty())return;
	path_v v;
	for (auto& ct : *ov)
	{
		v.addCircle(ct, r);
	}
	draw_path(&v, st);
}
void canvas_dev::draw_line(std::vector<glm::vec2>* ov, style_path_t* st)
{
	if (!ov || ov->empty())return;
	bool stroke = st->line_width > 0 && st->stroke_color;
	if (stroke)
	{
		auto count = ov->size() / 2;
		cairo_save(cr);
		cairo_translate(cr, st->pos.x, st->pos.y);
		set_color(cr, st->stroke_color);

		cairo_set_line_width(cr, st->line_width);
		cairo_set_line_cap(cr, (cairo_line_cap_t)st->cap);
		cairo_set_line_join(cr, (cairo_line_join_t)st->join);

		auto t = ov->data();
		auto mt = *t;
		auto xt = *t;
		for (size_t i = 0; i < count; i++, t++)
		{
			mt = *t;
			cairo_move_to(cr, t->x, t->y);
			t++;
			cairo_line_to(cr, t->x, t->y);
			xt = *t;
		}

		cairo_stroke(cr);
		cairo_restore(cr);
	}
}
void canvas_dev::draw_line(const glm::vec4& t, uint32_t stroke_color, float linewidth)
{
	bool stroke = linewidth > 0 && stroke_color;
	if (stroke)
	{
		cairo_save(cr);
		set_color(cr, stroke_color);
		cairo_set_line_width(cr, linewidth);
		cairo_set_line_cap(cr, cairo_line_cap_t::CAIRO_LINE_CAP_SQUARE);
		cairo_set_line_join(cr, cairo_line_join_t::CAIRO_LINE_JOIN_BEVEL);
		cairo_move_to(cr, t.x, t.y);
		cairo_line_to(cr, t.z, t.w);
		cairo_stroke(cr);
		cairo_restore(cr);
	}
}
void canvas_dev::draw_line(const glm::vec2& t, const glm::vec2& t1, uint32_t stroke_color, float linewidth)
{
	bool stroke = linewidth > 0 && stroke_color;
	if (stroke)
	{
		cairo_save(cr);
		set_color(cr, stroke_color);
		cairo_set_line_width(cr, linewidth);
		cairo_set_line_cap(cr, cairo_line_cap_t::CAIRO_LINE_CAP_SQUARE);
		cairo_set_line_join(cr, cairo_line_join_t::CAIRO_LINE_JOIN_BEVEL);
		cairo_move_to(cr, t.x, t.y);
		cairo_line_to(cr, t1.x, t.y);
		cairo_stroke(cr);
		cairo_restore(cr);
	}
}
void canvas_dev::draw_line1(std::vector<glm::vec2>* ov, style_path_t* st)
{
	if (!ov || ov->empty())return;
	bool stroke = st->line_width > 0 && st->stroke_color;
	if (stroke)
	{
		auto count = ov->size();
		cairo_save(cr);
		cairo_translate(cr, st->pos.x, st->pos.y);
		set_color(cr, st->stroke_color);

		cairo_set_line_width(cr, st->line_width);
		cairo_set_line_cap(cr, (cairo_line_cap_t)st->cap);
		cairo_set_line_join(cr, (cairo_line_join_t)st->join);

		auto t = ov->data();
		auto mt = *t;
		auto xt = *t;
		cairo_move_to(cr, t->x, t->y);
		t++;
		for (size_t i = 1; i < count; i++, t++)
		{
			cairo_line_to(cr, t->x, t->y);
			xt = *t;
		}
		if (count > 2)
		{
			if (xt.x == mt.x && xt.y == mt.y)
				cairo_close_path(cr);
		}
		cairo_stroke(cr);
		cairo_restore(cr);
	}
}

void canvas_dev::draw_text(const void* str, const glm::vec2& pos, font_xi* fx)
{
	cairo_save(cr);
	cairo_select_font_face(cr, fx->family, (cairo_font_slant_t)fx->slant, fx->weight_bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
	auto opt = cairo_font_options_create();
	cairo_get_font_options(cr, opt);
	auto k = cairo_get_antialias(cr);
	auto k1 = cairo_font_options_get_antialias(opt);
	//cairo_set_antialias(cr, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_antialias(opt, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_subpixel_order(opt, CAIRO_SUBPIXEL_ORDER_RGB);
	cairo_font_options_set_color_mode(opt, CAIRO_COLOR_MODE_COLOR);
	cairo_set_font_options(cr, opt);
	cairo_move_to(cr, pos.x, pos.y);
	cairo_set_font_size(cr, fx->font_size);
	if (fx->color)
		set_color(cr, fx->color);
	cairo_show_text(cr, (char*)str);
	cairo_font_options_destroy(opt);
	cairo_restore(cr);
	//cairo_paint_with_alpha(cr, 0.83);
}


class cairo_layout_text
{
public:
	glm::ivec2 _size = {};	// 布局大小
	std::string _family;
	int _fontsize = 12;
	int _linewidth = 2;
	glm::ivec2 _color = {};
	glm::vec2 align = {};
	int baseline = 12;
	int baseline_raw = 12;
public:
	cairo_layout_text();
	~cairo_layout_text();
	void set_size(const glm::ivec2& ss);
	void set_style(const char* family, int fontsize, uint32_t color);
	void set_style(const char* family, int fontsize, const glm::ivec2& color, int linewidth);
	// x=0左，0.5中，1右。y也一样
	void set_align(const glm::vec2& a);
	// 渲染文本
	void draw_text(cairo_t* cr, const void* str, const glm::vec2& pos);
	// 渲染文字路径
	void draw_text_path(cairo_t* cr, const void* str, const glm::vec2& pos);

	glm::ivec4 get_text_ext(cairo_t* cr, const void* str);
private:

};
cairo_layout_text::cairo_layout_text()
{

}

cairo_layout_text::~cairo_layout_text()
{
}
void cairo_layout_text::set_size(const glm::ivec2& ss)
{
	_size = ss;
}
void cairo_layout_text::set_style(const char* family, int fontsize, uint32_t color) {
	if (family)_family = family;
	if (color)_color.y = color;
	if (fontsize > 0) _fontsize = fontsize;
}
void cairo_layout_text::set_style(const char* family, int fontsize, const glm::ivec2& color, int linewidth) {
	if (family)_family = family;
	_color = color;
	if (fontsize > 0) _fontsize = fontsize;
	if (linewidth > 0)_linewidth = linewidth;
}
void cairo_layout_text::set_align(const glm::vec2& a)
{
	align = a;
}
glm::ivec4 cairo_layout_text::get_text_ext(cairo_t* cr, const void* str)
{
	cairo_text_extents_t extents = {};
	cairo_font_extents_t fext = {};
	cairo_select_font_face(cr, _family.c_str(), cairo_font_slant_t::CAIRO_FONT_SLANT_NORMAL, /*fx->weight_bold ? CAIRO_FONT_WEIGHT_BOLD : */CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, _fontsize);
	cairo_text_extents(cr, (char*)str, &extents);
	cairo_font_extents(cr, &fext);
	glm::vec2 bearing = { extents.x_bearing, extents.y_bearing };
	glm::vec2 ext = { extents.width,extents.height };
	baseline_raw = fext.ascent;	// 原始基线
	glm::vec2 ss = _size;
	auto ps = ss * align - (ext * align + bearing);
	baseline = ps.y;  // 调整后基线
	return { ps, extents.width, extents.height };
}
void cairo_layout_text::draw_text(cairo_t* cr, const void* str, const glm::vec2& pos)
{
	auto bx = get_text_ext(cr, str);
	cairo_save(cr);
	cairo_move_to(cr, pos.x + bx.x, pos.y + bx.y);
	if (_color.y != 0)
		set_color(cr, _color.y);
	cairo_show_text(cr, (char*)str);
	cairo_restore(cr);
}
// 渲染文字路径
void cairo_layout_text::draw_text_path(cairo_t* cr, const void* str, const glm::vec2& pos)
{
	auto bx = get_text_ext(cr, str);
	cairo_save(cr);
	cairo_move_to(cr, pos.x + bx.x, pos.y + bx.y);
	cairo_text_path(cr, (char*)str);
	if (_color.x != 0)
	{
		set_color(cr, _color.x);
		if (_color.y != 0)
			cairo_fill_preserve(cr);
		else {
			cairo_fill(cr);
		}
	}
	if (_color.y != 0)
		set_color(cr, _color.y);
	cairo_set_line_width(cr, _linewidth);
	cairo_stroke(cr);
	cairo_restore(cr);
}

glm::ivec4 get_text_align(cairo_t* cr, const char* str, const glm::vec2& pos, const glm::vec2& boxsize, const glm::vec2& text_align, const char* family, int font_size)
{
	cairo_text_extents_t extents = {};
	if (family) {
		cairo_select_font_face(cr, family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	}
	if (font_size > 0)
		cairo_set_font_size(cr, font_size);
	cairo_text_extents(cr, str, &extents);
	glm::vec2 bearing = { extents.x_bearing, extents.y_bearing };
	glm::vec2 ext = { extents.width,extents.height };
	glm::vec2 ss = boxsize;
	auto ps = ss * text_align - (ext * text_align + bearing);
	ps += pos;
	auto r = glm::vec4(ps, extents.width, extents.height);
	return fcv4(r);
}

void draw_text(cairo_t* cr, const char* str, const glm::ivec4& et, uint32_t text_color)
{
	if (text_color)
	{
		set_color(cr, text_color);
		cairo_move_to(cr, et.x, et.y);
		cairo_show_text(cr, str); // 文本渲染 
	}
}

glm::ivec4 draw_text_align(cairo_t* cr, const char* str, const glm::vec2& pos, const glm::vec2& boxsize, const glm::vec2& text_align, uint32_t text_color, const char* family, int font_size)
{
	cairo_text_extents_t extents = {};
	if (family) {
		cairo_select_font_face(cr, family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	}
	if (font_size > 0)
		cairo_set_font_size(cr, font_size);
	cairo_text_extents(cr, str, &extents);
	glm::vec2 bearing = { extents.x_bearing, extents.y_bearing };
	glm::vec2 ext = { extents.width, extents.height };
	glm::vec2 ss = boxsize;
	auto ps = pos;
	if (ss.x > 0 || ss.y > 0)
		ps += ss * text_align - (ext * text_align + bearing);
	if (text_color)
	{
		set_color(cr, text_color);
		cairo_move_to(cr, ps.x, ps.y);
		cairo_show_text(cr, str); // 文本渲染 
	}
	return { ps, extents.x_advance, extents.y_advance };
}
inline int multiply_alpha(int alpha, int color)
{
	int temp = (alpha * color) + 0x80;
	return ((temp + (temp >> 8)) >> 8);
}
// 预乘输出bgra，type=0为原数据是rgba
void premultiply_data(int w, unsigned char* data, int type, bool multiply)
{
	for (size_t i = 0; i < w; i += 4) {
		uint8_t* base = &data[i];
		uint8_t  alpha = base[3];
		uint32_t p;

		if (alpha == 0) {
			p = 0;
		}
		else {
			uint8_t  red = base[0];
			uint8_t  green = base[1];
			uint8_t  blue = base[2];

			if (alpha != 0xff && multiply) {
				red = multiply_alpha(alpha, red);
				green = multiply_alpha(alpha, green);
				blue = multiply_alpha(alpha, blue);
			}
			if (type == 0)
				p = ((uint32_t)alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
			else
				p = ((uint32_t)alpha << 24) | (blue << 16) | (green << 8) | (red << 0);
		}
		memcpy(base, &p, sizeof(uint32_t));
	}
}
glm::ivec2 get_surface_size(cairo_surface_t* p) {
	return { cairo_image_surface_get_width(p), cairo_image_surface_get_height(p) };
}
glm::vec2 draw_image(cairo_t* cr, cairo_surface_t* image, const glm::vec2& pos, const glm::vec4& rc, uint32_t color, const glm::vec2& dsize)
{
	glm::vec2 ss = { rc.z, rc.w };
	if (ss.x < 0)
	{
		ss.x = cairo_image_surface_get_width(image);
	}
	if (ss.y < 0)
	{
		ss.y = cairo_image_surface_get_height(image);
	}
	if (ss.x > 0 && ss.y > 0)
	{
		glm::vec2 sc = { 1,1 };
		if (dsize.x > 0 && dsize.y > 0) {
			sc = dsize / ss;
		}
		if (color > 0 && color != -1)
		{
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_translate(cr, pos.x, pos.y);
			if (sc.x != 1 || sc.y != 1)
				cairo_scale(cr, sc.x, sc.y);
			cairo_rectangle(cr, 0, 0, ss.x, ss.y);
			cairo_clip(cr);
			set_color(cr, color);
			cairo_mask_surface(cr, image, -rc.x, -rc.y);
			cairo_restore(cr);
		}
		else {
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_translate(cr, pos.x, pos.y);
			if (sc.x != 1 || sc.y != 1)
				cairo_scale(cr, sc.x, sc.y);
			cairo_set_source_surface(cr, image, -rc.x, -rc.y);
			cairo_rectangle(cr, 0, 0, ss.x, ss.y);
			cairo_fill(cr);
			cairo_restore(cr);
		}
	}
	return ss;
}

glm::vec2 draw_image(cairo_t* cr, cairo_surface_t* image, const glm::vec2& pos, const glm::vec4& rc, uint32_t color, const glm::vec2& size, const glm::vec4& sliced)
{
	glm::vec2 ss = { rc.z, rc.w };
	if (ss.x < 0)
	{
		ss.x = cairo_image_surface_get_width(image);
	}
	if (ss.y < 0)
	{
		ss.y = cairo_image_surface_get_height(image);
	}
#if 0
	cairo_as _a(cr);
	//cairo_scale(cr, scale.x, scale.y);
	// 上层
	{
		glm::vec2 v[] = { {sliced.x,sliced.y}, {ss.x - (sliced.x + sliced.z),sliced.y }, { sliced.z,sliced.y } };
		glm::vec2 vpos[3] = { {0,0},{sliced.x,0},{sliced.x + v[1].x,0} };
		for (size_t i = 0; i < 3; i++)
		{
			glm::vec4 rc0 = { rc.x + vpos[i].x,rc.y + vpos[i].y,v[i].x,v[i].y };
			draw_image(cr, image, vpos[i] + pos, rc0, color);
		}
	}
	// 中层
	{
		glm::vec2 v[] = { {sliced.x,ss.y - (sliced.y + sliced.w)}, { ss.x - (sliced.x + sliced.z),ss.y - (sliced.y + sliced.w) }, { sliced.z,ss.y - (sliced.y + sliced.w) } };
		glm::vec2 vpos[3] = { {0,sliced.y},{sliced.x,sliced.y},{sliced.x + v[1].x,sliced.y} };
		for (size_t i = 0; i < 3; i++)
		{
			glm::vec4 rc0 = { rc.x + vpos[i].x,rc.y + vpos[i].y,v[i].x,v[i].y };
			draw_image(cr, image, vpos[i] + pos, rc0, color);
		}
	}
	// 下层
	{
		glm::vec2 v[] = { {sliced.x,sliced.w},{ss.x - (sliced.x + sliced.z),sliced.w},{sliced.z,sliced.w} };
		glm::vec2 vpos[3] = { {0,ss.y - sliced.w},{sliced.x,ss.y - sliced.w},{sliced.x + v[1].x,ss.y - sliced.w} };
		for (size_t i = 0; i < 3; i++)
		{
			glm::vec4 rc0 = { rc.x + vpos[i].x,rc.y + vpos[i].y,v[i].x,v[i].y };
			draw_image(cr, image, vpos[i] + pos, rc0, color);
		}
	}
#endif
	return ss;
}
int64_t get_rdev() {
	return std::chrono::system_clock::now().time_since_epoch().count();
}
int get_rand(int f, int s)
{
	static std::mt19937 gen(get_rdev()); //gen是一个使用rd()作种子初始化的标准梅森旋转算法的随机数发生器
	std::uniform_int_distribution<> distrib(f, s);
	return distrib(gen);
}
int64_t get_rand64(int64_t f, int64_t s)
{
	static std::mt19937_64 gen(get_rdev()); //gen是一个使用rd()作种子初始化的标准梅森旋转算法的随机数发生器
	auto d = gen();
	return f + d % (s - f + 1);
}
void destroy_image_data(void* d) {
	auto p = (uint32_t*)d;
	if (d)delete[]p;
}

cairo_surface_t* new_image_cr(image_ptr_t* img)
{
	cairo_surface_t* image = 0;
	if (img->stride < 1)img->stride = img->width * sizeof(uint32_t);
	auto px = new uint32_t[img->width * img->height];
	image = cairo_image_surface_create_for_data((unsigned char*)px, CAIRO_FORMAT_ARGB32, img->width, img->height, img->width * sizeof(int));
	if (image)
	{
		image_set_ud(image, key_def_data, px, destroy_image_data);
		memcpy(px, (unsigned char*)img->data, img->height * img->width * sizeof(int));
		if (img->multiply && img->type == 1)
		{
		}
		else {
			int stride = cairo_image_surface_get_stride(image);
			auto data = cairo_image_surface_get_data(image);
			auto t = data;
			auto ts = (unsigned char*)img->data;
			for (size_t i = 0; i < img->height; i++)
			{
				premultiply_data(img->width * 4, t, img->type, !img->multiply);
				t += stride;
				ts += img->stride;
			}
			//#define _DEBUG
			//			save_img_png(img, "update_text_img.png");
			//			cairo_surface_write_to_png(image, "update_text_surface.png");
			//#endif
		}
	}
	else {
		delete[]px;
	}
	return image;
}
cairo_surface_t* new_image_cr(const glm::ivec2& size, uint32_t* data)
{
	cairo_surface_t* image = 0;
	image_ptr_t img[1] = {};
	img->width = size.x;
	img->height = size.y;
	if (img->stride < 1)img->stride = img->width * sizeof(uint32_t);
	auto px = data ? data : new uint32_t[img->width * img->height];
	image = cairo_image_surface_create_for_data((unsigned char*)px, CAIRO_FORMAT_ARGB32, img->width, img->height, img->width * sizeof(int));
	if (image)
	{
		if (!data)
			image_set_ud(image, key_def_data, px, destroy_image_data);
		memset(px, 0, img->width * img->height * sizeof(int));
	}
	else {
		delete[]px;
	}
	return image;
}
void update_image_cr(cairo_surface_t* image, image_ptr_t* img)
{
	if (img->stride < 1)img->stride = img->width * sizeof(uint32_t);
	if (img->multiply && img->type == 1)
	{
		auto data = cairo_image_surface_get_data(image);
		memcpy(data, (unsigned char*)img->data, img->height * img->width * sizeof(int));
	}
	else {
		int stride = cairo_image_surface_get_stride(image);
		auto data = cairo_image_surface_get_data(image);
		memcpy(data, (unsigned char*)img->data, img->height * img->width * sizeof(int));
		auto t = data;
		auto ts = (unsigned char*)img->data;
		for (size_t i = 0; i < img->height; i++)
		{
			premultiply_data(img->width * 4, t, img->type, !img->multiply);
			t += stride;
			ts += img->stride;
		}
	}
}
void free_image_cr(cairo_surface_t* image)
{
	if (image)
	{
		cairo_surface_destroy(image);
	}
}

void image_set_ud(cairo_surface_t* p, uint64_t key, void* ud, void(*destroy_func)(void* data))
{
	if (p && key) {
		cairo_surface_set_user_data(p, (cairo_user_data_key_t*)key, ud, destroy_func);
	}
}

void* image_get_ud(cairo_surface_t* p, uint64_t key)
{
	return cairo_surface_get_user_data(p, (cairo_user_data_key_t*)key);
}

void image_save_png(cairo_surface_t* surface, const char* fn)
{
	if (surface && fn && *fn)
		cairo_surface_write_to_png(surface, fn);
}

void draw_text(cairo_t* cr, const void* str, const glm::vec2& pos, font_xi* fx)
{
	cairo_save(cr);
#if 0
	draw_text(cr, (char*)str, pos, fx->font_size);
#else
	cairo_select_font_face(cr, fx->family, (cairo_font_slant_t)fx->slant, fx->weight_bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
	auto opt = cairo_font_options_create();
	cairo_get_font_options(cr, opt);
	auto k = cairo_get_antialias(cr);
	auto k1 = cairo_font_options_get_antialias(opt);
	//cairo_set_antialias(cr, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_antialias(opt, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_subpixel_order(opt, CAIRO_SUBPIXEL_ORDER_RGB);
	cairo_set_font_options(cr, opt);
	cairo_move_to(cr, pos.x, pos.y);
	cairo_set_font_size(cr, fx->font_size);
	if (fx->color)
		set_color(cr, fx->color);
	cairo_show_text(cr, (char*)str);
	cairo_font_options_destroy(opt);
#endif
	cairo_restore(cr);
	//cairo_paint_with_alpha(cr, 0.83);
}
void canvas_dev::draw_circle(const glm::vec2& pos, float r, float linewidth, uint32_t color, uint32_t fill)
{
	cairo_save(cr);
	bool stroke = linewidth > 0 && color;
	if (fill)
	{
		set_color(cr, fill);
		cairo_arc(cr, pos.x, pos.y, r, 0, 2 * M_PI);
		cairo_fill(cr);
	}
	if (stroke)
	{
		set_color(cr, color);
		cairo_arc(cr, pos.x, pos.y, r, 0, 2 * M_PI);
		cairo_stroke(cr);
	}
	cairo_restore(cr);
}
void canvas_dev::draw_surface(cairo_surface_t* image, const glm::vec2& pos, double alpha)
{
	cairo_set_source_surface(cr, image, pos.x, pos.y);
	cairo_paint_with_alpha(cr, alpha);
}
#if 1
int canvas_dev::get_path(path_v* pt)
{
	int r = 0;
	cairo_path_t* path;
	cairo_path_data_t* data;

	path = cairo_copy_path(cr);
	if (!path)return 0;
	r = path->num_data;
	for (int i = 0; i < path->num_data; i += path->data[i].header.length) {
		data = &path->data[i];
		switch (data->header.type) {
		case CAIRO_PATH_MOVE_TO:
			pt->moveTo(data[1].point.x, data[1].point.y);
			break;
		case CAIRO_PATH_LINE_TO:
			pt->lineTo(data[1].point.x, data[1].point.y);
			break;
		case CAIRO_PATH_CURVE_TO:
			pt->curveTo(data[1].point.x, data[1].point.y,
				data[2].point.x, data[2].point.y,
				data[3].point.x, data[3].point.y);
			break;
		case CAIRO_PATH_CLOSE_PATH:
			pt->closePath();
			break;
		}
	}
	cairo_path_destroy(path);
	return 0;
}
#endif
glm::vec4 canvas_dev::get_path_extents()
{
	double tc[4] = {};
	auto t = tc;
	if (cr)
		cairo_path_extents(cr, t, t + 1, t + 2, t + 3);
	return glm::vec4(tc[0], tc[1], tc[2], tc[3]);
}

#if 1

glm::vec4 get_merged_rect(glm::vec4* rects, int length)
{
	if (length == 0) {
		return {};
	}
	glm::vec4 r = rects[0];
	for (size_t i = 1; i < length; i++)
	{
		auto it = rects[i];
		r.x = std::min(r.x, it.x);
		r.y = std::min(r.y, it.y);
		r.z = std::max(r.z, it.z);
		r.w = std::max(r.w, it.w);
	}
	return r;
}
glm::vec4 get_bboxs(std::vector<path_v*>& v)
{
	std::vector<glm::vec4> bboxs;
	bboxs.reserve(v.size());
	for (auto it : v)
	{
		auto v4 = it->mkbox();
		bboxs.push_back(v4);
	}
	return get_merged_rect(bboxs.data(), bboxs.size());
}
#endif // 1

tinyviewcanvas_x::tinyviewcanvas_x()
{
}

tinyviewcanvas_x::~tinyviewcanvas_x()
{
	if (_backing_store)
	{
		cairo_surface_destroy(_backing_store);
	}
	_backing_store = 0;
}

void tinyviewcanvas_x::draw(canvas_dev* c) {

	if (!_backing_store_valid)
		draw_back();
	auto cr = c->cr;
	if (_backing_store)
	{
		cairo_save(cr);
		cairo_set_source_surface(cr, _backing_store, 0, 0);
		cairo_paint(cr);
		cairo_restore(cr);
	}
}
void tinyviewcanvas_x::set_size(const glm::ivec2& ss)
{
	if (ss != size)
	{
		size = ss;
		if (_backing_store)
		{
			cairo_surface_destroy(_backing_store);
			_backing_store = 0;
		}
	}
}

void tinyviewcanvas_x::set_view_move(bool is)
{
	has_move = is;
}

void tinyviewcanvas_x::set_view_scale(bool is)
{
	has_scale = is;
}



void tinyviewcanvas_x::draw_back() {
	if (!_data)return;
	print_time a("draw_back");
	if (!_backing_store)
		_backing_store = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.x, size.y);
	if (!_backing_store)return;
	// Get context
	cairo_t* cr = cairo_create(_backing_store);
	{
		cairo_save(cr);
		set_color(cr, 0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint(cr);
		cairo_restore(cr);

		//cairo_translate(cr, vpos.x, vpos.y);
		cairo_matrix_t t = {};
		auto a0 = mx[0];
		auto a1 = mx[1];
		auto a2 = mx[2];
		//auto a3 = mx[3];
		//cairo_matrix_init(&t, a0.x, 0, 0, a1.y, a3.x, a3.y); // 垂直翻转
		//cairo_set_matrix(cr, &t);
		cairo_translate(cr, a2.x + vpos.x, vpos.y + a2.y);

		style_path_t st0 = {};
		st0.line_width = line_width;
		st0.fill_color = color.x;
		st0.stroke_color = color.y;
		st0.cap = 0;
		st0.join = 1;
		st0.pos = { 0.5,0.5 };
		style_path_t* spt = &st0;
		if (oldscale != scale)
		{
			print_time a("canvas draw");
			auto m = get_affine();
			ddrect.clear();
			double sc = scale / 100.0;
			glm::mat3 sx = glm::scale(glm::mat3(1.0), glm::vec2(sc, sc));
			oldscale = scale;
			for (auto it : draw_data)
			{
				delete it;
			}
			draw_data.clear();
			for (auto it : *_data)
			{
				auto v23 = glm::vec3(it->_pos, 1.0);
				auto v2s = glm::vec3(it->get_size(), 1.0);
				glm::vec2 ips = mx * v23;// get_v2m4(it->_pos, &mx);
				glm::vec2 ss = mx * v2s;// get_v2m4(it->get_size(), &mx);
				auto nx = ips + ss;
				if (ips.x > size.x || ips.y > size.y || nx.x < -vpos.x * 2 || nx.y < -vpos.y * 2)
				{
					//continue;
				}
				auto npv = new path_v(*it);
				draw_data.push_back(npv);
				npv->_pos.y += npv->_baseline;//基线
				npv->_baseline = 0;
				npv->set_mat(sx);
				auto v4 = it->mkbox();
				ddrect.push_back(v4);
			}
			bx = get_bboxs(draw_data);
		}
		{
			print_time a("draw_path_v");
			for (auto it : draw_data) {
				draw_path_v(cr, it, spt);
			}
		}
	}
	_backing_store_valid = true;
	cairo_destroy(cr);

}

void tinyviewcanvas_x::on_button(int idx, int down, const glm::vec2& pos1, int clicks)
{
	auto pos = pos1 - (glm::vec2)vpos;
	//idx=1左，3右，2中
	if (idx == 1)
	{
		if (down == 1 && ckinc == 0)
		{
			glm::vec2 a3 = mx[2];
			last_mouse = pos - a3;
		}
		ckinc++;
		if (down == 0)
		{
			ckinc = 0;
		}

	}
	else if (idx == 3) {
		if (down == 0)
		{
			reset_view();
		}
	}
}
void tinyviewcanvas_x::on_motion(const glm::vec2& pos1)
{
	auto pos = pos1 - (glm::vec2)vpos;
	if (ckinc > 0)
	{
		if (has_move) {
			auto mp = pos;
			mp = pos - last_mouse;
			auto t = glm::translate(glm::mat3(1.0), mp);
			double sc = scale / 100.0;
			auto s = glm::scale(glm::mat3(1.0), glm::vec2(sc, sc));
			mx = t * s;
			_backing_store_valid = false;
		}
	}
	eoffset = pos;
	hit_test(pos1);
}
void tinyviewcanvas_x::on_wheel(int deltaY)
{
	if (has_scale)
	{
		auto prevZoom = scale;
		auto scale1 = scale;
		auto zoom = (deltaY * scaleStep);
		scale1 += zoom;
		if (scale1 < minScale) {
			scale = minScale;
			return;
		}
		else if (scale1 > maxScale) {
			scale = maxScale;
			return;
		}
		double sc = scale1 / 100.0;
		double sc1 = prevZoom / 100.0;
		glm::vec2 nps = mx[2];
		auto s = glm::scale(glm::mat3(1.0), glm::vec2(sc, sc));
		auto t = glm::translate(glm::mat3(1.0), glm::vec2(nps));
		mx = t * s;
		scale = scale1;
	}
	hit_test(eoffset + (glm::vec2)vpos);
	_backing_store_valid = false;
}
void tinyviewcanvas_x::reset_view()
{
	ckinc = 0;
	scale = 100;
	mx = glm::mat3(1.0);
	_backing_store_valid = false;
}

glm::ivec4 tinyviewcanvas_x::get_hbox()
{
	return hover_bx;
}
glm::vec4 tinyviewcanvas_x::get_bbox()
{
	return bx;
}

glm::mat3 tinyviewcanvas_x::get_affine()
{
	glm::mat3 r = glm::translate(glm::mat3(1.0), glm::vec2(vpos));
	return r * mx;
}

// 测试鼠标坐标的矩形
void tinyviewcanvas_x::hit_test(const glm::vec2& ps)
{
	auto ae = get_affine();
	auto m = glm::inverse(ae);//逆矩阵
	auto p2 = v2xm3(ps, m);	 //坐标和矩阵相乘
	auto k2 = check_box_cr(p2, ddrect.data(), ddrect.size());
	hover_bx = {};
	if (k2.x)
	{
		auto v4 = get_boxm3(ddrect[k2.y], ae);
		v4.z -= v4.x;
		v4.w -= v4.y;
		hover_bx = { glm::floor(v4.x),glm::floor(v4.y),glm::round(v4.z),glm::round(v4.w) };
	}
}

struct SPRulerMetric
{
	gdouble ruler_scale[16];
	gint    subdivide[5];
};

// Ruler metric for general use.
static SPRulerMetric const ruler_metric_general = {
  { 1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000 },
  { 1, 5, 10, 50, 100 }
};

// Ruler metric for inch scales.
static SPRulerMetric const ruler_metric_inches = {
  { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 },
  { 1, 2, 4, 8, 16 }
};

// Half width of pointer triangle.
static double half_width = 5.0;
// 将像素转换为厘米
double pixels_to_centimeters(int pixels, int dpi) {
	return (pixels / dpi) * 2.54;
}

// 将像素转换为毫米
double pixels_to_millimeters(int pixels, int dpi) {
	return (pixels / dpi) * 25.4;
}
// 将毫米转换为像素
double smillimeters_to_pixel(int mm, int dpi) {
	return (dpi / 25.4) * mm;
}

// 将像素转换为英寸
double pixels_to_inches(int pixels, int dpi) {
	return pixels / dpi;
}

Ruler::Ruler()
{

}

void Ruler::set_range(double lower, double upper)
{
	if (_lower != lower || _upper != upper) {

		_lower = lower;
		_upper = upper;
		_max_size = _upper - _lower;
		if (_max_size == 0) {
			_max_size = 1;
		}

		_backing_store_valid = false;
		//_drawing_area->queue_draw();
	}
}

void Ruler::set_page(double lower, double upper)
{
	if (_page_lower != lower || _page_upper != upper) {
		_page_lower = lower;
		_page_upper = upper;

		_backing_store_valid = false;
		//_drawing_area->queue_draw();
	}
}

void Ruler::set_selection(double lower, double upper)
{
	if (_sel_lower != lower || _sel_upper != upper) {
		_sel_lower = lower;
		_sel_upper = upper;

		_backing_store_valid = false;
		//_drawing_area->queue_draw();
	}
}

glm::ivec2 Ruler::get_drawing_size()
{
	return  drawingsize;
}
#define orientation_horizontal 0


#define getrgba(a,b,sp) lerp(a.sp,b.sp,ratio)

glm::dvec4 mix_colors(glm::vec4 a, glm::vec4 b, float ratio)
{
	auto lerp = [](double v0, double v1, double t) { return (1.0 - t) * v0 + t * v1; };
	glm::dvec4 result = {};
	result.x = getrgba(a, b, x);
	result.y = getrgba(a, b, y);
	result.z = getrgba(a, b, z);
	result.w = getrgba(a, b, w);
	return result;
}
cairo_pattern_t* create_cubic_gradient(
	glm::vec4 rect,
	glm::vec4 from,
	glm::vec4 to,
	glm::vec2 ctrl1,
	glm::vec2 ctrl2,
	glm::vec2 p0 = {},
	glm::vec2 p1 = { 1,1 },
	int steps = 8
);
cairo_pattern_t* create_cubic_gradient(
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
	cairo_pattern_t* g = cairo_pattern_create_linear(rect.x, rect.y, rect.z, rect.w);

	//cairo_pattern_t* cairo_pattern_create_radial(double cx0, double cy0, double radius0, double cx1, double cy1, double radius1);

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

		auto color = mix_colors(from, to, ratio);
		cairo_pattern_add_color_stop_rgba(g, offset, color.x, color.y, color.z, color.w);
	}

	return g;
}
void paint_shadow(cairo_t* cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, bool rev, float r)
{
	auto trans = shadow;
	trans.w = 0;
	auto gr = rev ?
		create_cubic_gradient(glm::vec4(0, 0, size_x, size_y), trans, shadow, glm::vec2(0.5, 0.0), glm::vec2(1.0, 0.5))
		: create_cubic_gradient(glm::vec4(0, 0, size_x, size_y), shadow, trans, glm::vec2(0, 0.5), glm::vec2(0.5, 1));
	//cairo_rectangle(cr, 0, 0, width, height);
	draw_rectangle(cr, { 0,0,width,height }, r);
	cairo_set_source(cr, gr);
	cairo_fill(cr);
	cairo_pattern_destroy(gr);
}
void paint_shadow(cairo_t* cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, const glm::vec4& color_to, bool rev, float r)
{
	auto gr = rev ?
		create_cubic_gradient(glm::vec4(0, 0, size_x, size_y), color_to, shadow, glm::vec2(0.5, 0.0), glm::vec2(1.0, 0.5))
		: create_cubic_gradient(glm::vec4(0, 0, size_x, size_y), shadow, color_to, glm::vec2(0, 0.5), glm::vec2(0.5, 1));
	//cairo_rectangle(cr, 0, 0, width, height);
	draw_rectangle(cr, { 0,0,width,height }, r);
	cairo_set_source(cr, gr);
	cairo_fill(cr);
	cairo_pattern_destroy(gr);
}


bool Ruler::draw_scale(cairo_t* cr_in)
{
	auto a = get_drawing_size();
	int awidth = a.x, aheight = a.y;
	// Create backing store (need surface_in to get scale factor correct).
	if (oldsize != drawingsize)
	{
		oldsize = drawingsize;
		auto surface_in = cairo_get_target(cr_in);
		if (_backing_store)
		{
			cairo_surface_destroy(_backing_store);
		}
		_backing_store = cairo_surface_create_similar_image(surface_in, CAIRO_FORMAT_ARGB32, awidth, aheight);
		//_backing_store = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, awidth, aheight);
	}
	if (!_backing_store)return false;
	// Get context
	cairo_t* cr = cairo_create(_backing_store);
	cairo_save(cr);
	set_color(cr, _back_color);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_restore(cr);

	// Color in page indication box
	if (has_page)
	{
		double psize = std::abs(_page_upper - _page_lower);
		if (psize > 0)
		{

			set_source_rgba(cr, _page_fill);
			cairo_new_path(cr);

			if (_orientation == orientation_horizontal) {
				cairo_rectangle(cr, _page_lower, 0, psize, aheight);
			}
			else {
				cairo_rectangle(cr, 0, _page_lower, awidth, is_yaxisdown > 0 ? psize : -psize);
			}
			cairo_fill(cr);
		}
	}

	cairo_set_line_width(cr, 1.0);

	// aparallel is the longer, oriented dimension of the ruler; aperpendicular shorter.
	auto const [aparallel, aperpendicular] = _orientation == orientation_horizontal
		? std::pair{ awidth , aheight }
	: std::pair{ aheight, awidth };

	// Draw bottom/right line of ruler
	set_source_rgba(cr, _foreground);
	if (_orientation == orientation_horizontal) {
		cairo_move_to(cr, 0, aheight - 0.5);
		cairo_line_to(cr, awidth, aheight - 0.5);
	}
	else {
		cairo_move_to(cr, awidth - 0.5, 0);
		cairo_line_to(cr, awidth - 0.5, aheight);
	}
	cairo_stroke(cr);

	// Draw a shadow which overlaps any previously painted object. 
	int gradient_size = 4;
	if (_orientation == orientation_horizontal) {
		paint_shadow(cr, 0, gradient_size, awidth, gradient_size, _shadow, 0);// 垂直方向
	}
	else {
		paint_shadow(cr, gradient_size, 0, gradient_size, aheight, _shadow, 0);// 水平方向
	}
	// Figure out scale. Largest ticks must be far enough apart to fit largest text in vertical ruler.
	// We actually require twice the distance.
	uint32_t scale = std::ceil(abs(_max_size)); // Largest number
	std::string scale_text = std::to_string(scale);
	uint32_t digits = scale_text.length() + 1; // Add one for negative sign.
	uint32_t minimum = digits * _font_size * 2;

	auto const pixels_per_unit = aparallel / _max_size; // pixel per distance

	SPRulerMetric ruler_metric = ruler_metric_general;
	if (ruler_in) {
		ruler_metric = ruler_metric_inches;
	}

	unsigned scale_index;
	for (scale_index = 0; scale_index < G_N_ELEMENTS(ruler_metric.ruler_scale) - 1; ++scale_index) {
		if (ruler_metric.ruler_scale[scale_index] * std::abs(pixels_per_unit) > minimum) break;
	}

	// Now we find out what is the subdivide index for the closest ticks we can draw
	unsigned divide_index;
	for (divide_index = 0; divide_index < G_N_ELEMENTS(ruler_metric.subdivide) - 1; ++divide_index) {
		if (ruler_metric.ruler_scale[scale_index] * std::abs(pixels_per_unit) < 5 * ruler_metric.subdivide[divide_index + 1]) break;
	}

	// We'll loop over all ticks.
	double pixels_per_tick = pixels_per_unit *
		ruler_metric.ruler_scale[scale_index] / ruler_metric.subdivide[divide_index];

	double units_per_tick = pixels_per_tick / pixels_per_unit;
	double ticks_per_unit = 1.0 / units_per_tick;

	// Find first and last ticks
	int start = 0;
	int end = 0;
	if (_lower < _upper) {
		start = std::floor(_lower * ticks_per_unit);
		end = std::ceil(_upper * ticks_per_unit);
	}
	else {
		start = std::floor(_upper * ticks_per_unit);
		end = std::ceil(_lower * ticks_per_unit);
	}

	// Loop over all ticks
	set_source_rgba(cr, _foreground);
	for (int i = start; i < end + 1; ++i) {

		// Position of tick (add 0.5 to center tick on pixel).
		double position = std::floor(i * pixels_per_tick - _lower * pixels_per_unit) + 0.5;

		// Height of tick
		int size = aperpendicular - 7;
		for (int j = divide_index; j > 0; --j) {
			if (i % ruler_metric.subdivide[j] == 0) break;
			size = size / 2 + 1;
		}

		// Draw text for major ticks.
		if (i % ruler_metric.subdivide[divide_index] == 0) {
			cairo_save(cr);

			int label_value = std::round(i * units_per_tick);

			auto& label = _label_cache[label_value];
			auto lv = std::to_string(label_value);// +(char*)u8"图";
			fx->font_size = _font_size;
			//auto ws = get_text_extents(cr, lv.c_str(), lv.size(), fx);
			if (!label) {
				label = draw_label(cr, lv, { /*ws.x,ws.y*/ });
			}
			// Align text to pixel
			int x = 3;// +_font_size * 0.5;
			int y = position + 2.5;
			if (_orientation == orientation_horizontal) {
				x = position + 2.5;
				y = 3;// +_font_size * 0.5;
			}

			// We don't know the surface height/width, damn you cairo.
			//cairo_rectangle(cr, x, y, 100, 100);
			//cairo_clip(cr);
			cairo_set_source_surface(cr, label, x, y);
			cairo_paint(cr);
			cairo_restore(cr);

		}

		// Draw ticks
		set_source_rgba(cr, _foreground);
		if (_orientation == orientation_horizontal) {
			cairo_move_to(cr, position, aheight - size);
			cairo_line_to(cr, position, aheight);
		}
		else {
			cairo_move_to(cr, awidth - size, position);
			cairo_line_to(cr, awidth, position);
		}
		cairo_stroke(cr);
	}

	// Draw a selection bar
	if (_sel_lower != _sel_upper && _sel_visible) {
		const auto radius = 3.0;
		const auto delta = _sel_upper - _sel_lower;
		const auto dxy = delta > 0 ? radius : -radius;
		double sy0 = _sel_lower;
		double sy1 = _sel_upper;
		double sx0 = floor(aperpendicular * 0.7);
		double sx1 = sx0;

		if (_orientation == orientation_horizontal) {
			std::swap(sy0, sx0);
			std::swap(sy1, sx1);
		}

		cairo_set_line_width(cr, 2.0);

		if (fabs(delta) > 2 * radius) {
			set_source_rgba(cr, _select_stroke);
			if (_orientation == orientation_horizontal) {
				cairo_move_to(cr, sx0 + dxy, sy0);
				cairo_line_to(cr, sx1 - dxy, sy1);
			}
			else {
				cairo_move_to(cr, sx0, sy0 + dxy);
				cairo_line_to(cr, sx1, sy1 - dxy);
			}
			cairo_stroke(cr);
		}

		// Markers
		set_source_rgba(cr, _select_fill);
		cairo_new_path(cr);
		cairo_arc(cr, sx0, sy0, radius, 0, 2 * M_PI);
		cairo_arc(cr, sx1, sy1, radius, 0, 2 * M_PI);
		cairo_fill(cr);

		set_source_rgba(cr, _select_stroke);
		cairo_new_path(cr);
		cairo_arc(cr, sx0, sy0, radius, 0, 2 * M_PI);
		cairo_stroke(cr);
		cairo_new_path(cr);
		cairo_arc(cr, sx1, sy1, radius, 0, 2 * M_PI);
		cairo_stroke(cr);
	}

	_backing_store_valid = true;

	cairo_destroy(cr);
	return true;
}
#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923   // pi/2
#endif
cairo_surface_t* Ruler::draw_label(cairo_t* cr_in, const std::string& label_value, const glm::ivec2& ts)
{
	bool rotate = _orientation != orientation_horizontal;
	auto surface_in = cairo_get_target(cr_in);
#if 0
	auto ly = create_pango_layout(/*cr_in,*/ label_value.c_str(), _font_size);
	auto layout = &ly;
	int text_width = ts.x;
	int text_height = ts.y;
	layout->get_pixel_size(text_width, text_height);
	if (rotate) {
		std::swap(text_width, text_height);
	}
	auto surface = cairo_surface_create_similar_image(surface_in, CAIRO_FORMAT_ARGB32, text_width + 10, text_height + 10);
	cairo_t* cr = cairo_create(surface);

	cairo_save(cr);
	set_source_rgba(cr, _foreground);
	if (rotate) {
		cairo_translate(cr, text_width / 2, text_height / 2);
		cairo_rotate(cr, -M_PI_2);
		cairo_translate(cr, -text_height / 2, -text_width / 2);
	}
	ly.fgp = ly.layout;
	auto fx0 = *fx;
	fx0.color = 0;
	layout->draw(cr);
	cairo_restore(cr);

	cairo_surface_write_to_png(surface, "surface_label.png");
	cairo_destroy(cr);
	return surface;
#endif
	return 0;
}
void Ruler::draw_marker(cairo_t* cr)
{
	auto a = get_drawing_size();
	int awidth = a.x, aheight = a.y;
	set_source_rgba(cr, _cursor_color);
	if (_orientation == orientation_horizontal) {
		cairo_move_to(cr, _position, aheight);
		cairo_line_to(cr, _position - half_width, aheight - half_width);
		cairo_line_to(cr, _position + half_width, aheight - half_width);
		cairo_close_path(cr);
	}
	else {
		cairo_move_to(cr, awidth, _position);
		cairo_line_to(cr, awidth - half_width, _position - half_width);
		cairo_line_to(cr, awidth - half_width, _position + half_width);
		cairo_close_path(cr);
	}
	cairo_fill(cr);
}

glm::ivec4 Ruler::marker_rect()
{
	auto a = get_drawing_size();
	int awidth = a.x, aheight = a.y;
	glm::ivec4 rect;
	rect.x = 0;
	rect.y = 0;
	rect.z = 0;
	rect.w = 0;

	// Find size of rectangle to enclose triangle.
	if (_orientation == orientation_horizontal) {
		rect.x = std::floor(_position - half_width);
		rect.y = std::floor(aheight - half_width);
		rect.z = std::ceil(half_width * 2.0 + 1);
		rect.w = std::ceil(half_width);
	}
	else {
		rect.x = std::floor(awidth - half_width);
		rect.y = std::floor(_position - half_width);
		rect.z = std::ceil(half_width);
		rect.w = std::ceil(half_width * 2.0 + 1);
	}

	return rect;
}

bool Ruler::on_drawing_area_draw(cairo_t* cr, const glm::vec2& pos)
{
	if (!_backing_store_valid) {
		draw_scale(cr);

		cairo_surface_write_to_png(_backing_store, "ruler1904.png");
	}
	cairo_save(cr);

	cairo_translate(cr, pos.x, pos.y);
	auto a = get_drawing_size();
	int awidth = a.x, aheight = a.y;
	glm::vec2 c2 = { awidth, aheight };
	glm::vec2 c = {};
	if (_orientation == orientation_horizontal) {
		c.x = aheight;
	}
	else {
		c.y = awidth;
	}
	cairo_rectangle(cr, c.x, c.y, c2.x, c2.y);
	cairo_clip(cr);
	cairo_set_source_surface(cr, _backing_store, 0, 0);
	cairo_paint(cr);
	if (c.x == 0)c.x -= 1;
	if (c.y == 0)c.y -= 1;
	cairo_translate(cr, c.x, c.y);
	draw_marker(cr);
	cairo_restore(cr);
	return true;
}

void Ruler::on_style_updated()
{
	//Gtk::Box::on_style_updated();

	//Glib::RefPtr<Gtk::StyleContext> style_context = get_style_context();

	//// Cache all our colors to speed up rendering.

	//_foreground = get_foreground_color(style_context);
	//_font_size = get_font_size(*this);

	//_shadow = get_color_with_class(style_context, "shadow");
	//_page_fill = get_color_with_class(style_context, "page");

	//style_context->add_class("selection");
	//_select_fill = get_color_with_class(style_context, "background");
	//_select_stroke = get_color_with_class(style_context, "border");
	//style_context->remove_class("selection");

	_label_cache.clear();
	_backing_store_valid = false;

	//queue_resize();
	//_drawing_area->queue_draw();
}

void Ruler::on_prefs_changed()
{
	_sel_visible;
	_backing_store_valid = false;
}

void Ruler::on_motion(void* motion, double x, double y)
{
	int drawing_x = std::lround(x), drawing_y = std::lround(y);
	double const position = _orientation == orientation_horizontal ? drawing_x : drawing_y;
	if (position == _position) return;

	_position = position;
	auto new_rect = marker_rect();
	_rect = new_rect;
}

int Ruler::on_click_pressed(void* click, int n_press, double x, double y)
{
	return 0;
}

void Ruler::set_context_menu()
{

}


#endif // 1

#if 1


class svg_dp
{
public:
#ifndef NO_SVG
	RsvgHandle* handle = 0;
	RsvgDimensionData dimension = {};
#endif
public:
	svg_dp(int dpi = 0);
	~svg_dp();
	static svg_dp* new_from_file(const void* fn, size_t len);
	static svg_dp* new_from_data(const void* str, size_t len);
	glm::vec2 get_pos_id(const char* id);
	void get_bixbuf(const char* id);
	void draw(cairo_t* cr);
	void draw(cairo_t* cr, const glm::vec2& pos, const glm::vec2& scale, double angle, const char* id);
};

svg_dp::svg_dp(int dpi)
{
#ifndef NO_SVG
	rsvg_set_default_dpi(dpi < 72.0 ? 72.0 : dpi);
#endif
}

svg_dp::~svg_dp()
{
#ifndef NO_SVG
	// 释放资源  
	rsvg_handle_free(handle);
#endif
}
svg_dp* svg_dp::new_from_file(const void* fn, size_t len)
{
	svg_dp* p = 0;
#ifndef NO_SVG
	GError* er = NULL;
	auto handle = rsvg_handle_new_from_file((gchar*)fn, &er);
	if (er != NULL) {
		printf("Failed to load SVG file: %s\n", er->message);
		return 0;
	}
	p = new svg_dp();
	/* 获取SVG图像的尺寸 */
	rsvg_handle_get_dimensions(handle, &p->dimension);
	p->handle = handle;
#endif
	return p;
}
svg_dp* svg_dp::new_from_data(const void* str, size_t len)
{
	GError* er = NULL;
	svg_dp* p = 0;
#ifndef NO_SVG
	auto handle = rsvg_handle_new_from_data((guint8*)str, len, &er);
	if (er != NULL) {
		printf("Failed to load SVG file: %s\n", er->message);
		return 0;
	}
	p = new svg_dp();
	/* 获取SVG图像的尺寸 */
	rsvg_handle_get_dimensions(handle, &p->dimension);
	p->handle = handle;
#endif
	return p;
}
glm::vec2 svg_dp::get_pos_id(const char* id)
{
#ifndef NO_SVG
	RsvgPositionData pos = {};
	auto r = rsvg_handle_get_position_sub(handle, &pos, id);
	return glm::vec2(pos.x, pos.y);
#else
	return {};
#endif
}

void svg_dp::get_bixbuf(const char* id)
{
#ifndef NO_SVG
	auto pxb = id && *id ? rsvg_handle_get_pixbuf(handle) : rsvg_handle_get_pixbuf_sub(handle, id);
	uint32_t length = 0;
	auto px = gdk_pixbuf_get_pixels_with_length(pxb, &length);
	auto w = gdk_pixbuf_get_width(pxb);
	auto h = gdk_pixbuf_get_height(pxb);
	auto st = gdk_pixbuf_get_rowstride(pxb);
#endif
	//g_object_unref(pxb);
	return;
}


void svg_dp::draw(cairo_t* cr)
{
#ifndef NO_SVG
	rsvg_handle_render_cairo(handle, cr);//渲染到窗口，有刷新事件需要重新执行，这里只渲染一次 
#endif
}
void svg_dp::draw(cairo_t* cr, const glm::vec2& pos, const glm::vec2& scale, double angle, const char* id)
{
#ifndef NO_SVG
	cairo_save(cr);
	if (scale.x > 0 && scale.y > 0)
		cairo_scale(cr, scale.x, scale.y);
	if (angle > 0)
		cairo_rotate(cr, angle);
	cairo_translate(cr, pos.x, pos.y);
	if (id && *id)
	{
		RsvgDimensionData dim = {};
		RsvgPositionData poss = {};
		rsvg_handle_get_dimensions_sub(handle, &dim, id);
		if (!rsvg_handle_get_position_sub(handle, &poss, id)) {
			return;
		}
		/* Move the whole thing to 0, 0 so the object to export is at the origin */
		cairo_translate(cr, -poss.x, -poss.y);
		rsvg_handle_render_cairo_sub(handle, cr, id);
	}
	else
	{
		rsvg_handle_render_cairo(handle, cr);//渲染到窗口，有刷新事件需要重新执行，这里只渲染一次
	}
	//cairo_surface_flush(surface);
	cairo_restore(cr);
#endif
}

svg_cx* new_svg_file(const void* fn, size_t len, int dpi) {
	return (svg_cx*)svg_dp::new_from_file(fn, len);
}
svg_cx* new_svg_data(const void* str, size_t len, int dpi) {
	return (svg_cx*)svg_dp::new_from_data(str, len);
}
void free_svg(svg_cx* svg)
{
	if (svg)
	{
		auto t = (svg_dp*)svg;
		delete t;
	}
}
void render_svg(cairo_t* cr, svg_cx* svg)
{
	if (cr && svg)
	{
		auto t = (svg_dp*)svg;
		t->draw(cr);
	}
}
void render_svg(cairo_t* cr, svg_cx* svg, const glm::vec2& pos, const glm::vec2& scale, double angle, const char* id)
{
	if (cr && svg)
	{
		auto t = (svg_dp*)svg;
		t->draw(cr, pos, scale, angle, id);
	}
}
#endif // 1






view_g::view_g()
{
	fx.family = (char*)u8"Calibri";
	fx.font_size = fh;
	ruler = new Ruler[2];
	ruler[0].fx = &fx;
	ruler[1].fx = &fx;
	ruler[1]._orientation = 1;
	vcanvas = new tinyviewcanvas_x();
}

view_g::~view_g()
{
	if (_backing_store)
	{
		cairo_surface_destroy(_backing_store);
	}
	_backing_store = 0;
	if (rss)
	{
		cairo_surface_destroy(rss);
	}
	rss = 0;
	if (vcanvas) { delete vcanvas; vcanvas = 0; }
	if (ruler) { delete[]ruler; ruler = 0; }
}

view_g* view_g::new_view(const glm::vec2& size)
{
	view_g* p = 0;
	if (size.x > 1 && size.y > 1)
	{
		p = new view_g();
		p->set_view(size);
	}
	return p;
}
void view_g::free_view(view_g* p)
{
	if (p)delete p;
}
void view_g::set_view(const glm::ivec2& size)
{
	if (!(size.x > 1 && size.y > 1)) {
		assert(0);
		return;
	}
	auto _hruler = ruler;
	auto _vruler = _hruler + 1;
	auto ss = size;
	vsize = size;
	ss.y = rsize.y;
	_hruler->drawingsize = ss;
	ss = size;
	ss.x = rsize.x;
	_vruler->drawingsize = ss;
	vcanvas->set_size(size);
	if (cdv && cdv->dsize != size)
	{
		canvas_dev::free_cdev(cdv);
	}
	cdv = canvas_dev::new_cdev(size, 0);
	_backing_store_valid = false;
}

void view_g::on_motion(const glm::vec2& pos) {
	glm::ivec2 ps = pos - rsize;
	if (curpos != ps)
	{
		curpos = ps;

		auto _hruler = ruler;
		auto _vruler = _hruler + 1;
		vcanvas->on_motion(pos);
		_hruler->on_motion(0, ps.x, ps.y);
		_vruler->on_motion(0, ps.x, ps.y);
		update();
	}
}
void view_g::on_button(int idx, int down, const glm::vec2& pos, int clicks) {
	auto ps = pos - rsize;
	if (down == 0 && is_ck)
	{
		is_ck = false;
		glm::vec4 trc = { 0,0,rsize };
		auto k2 = check_box_cr(pos, &trc, 1);
		if (k2.x)
		{
			vcanvas->has_move = !vcanvas->has_move;
			vcanvas->has_scale = vcanvas->has_move;
		}
	}
	if (down == 1)
	{
		is_ck = true;
	}
	vcanvas->on_button(idx, down, pos, clicks);
	update();
	_draw_valid = true;
}
void view_g::on_wheel(double x, double y)
{
	vcanvas->on_wheel(y);
	update();
	_draw_valid = true;
}
void view_g::updateRulers()
{
	glm::vec4 viewbox = { 0,0, vsize };// _canvas->get_area_world();
	glm::vec4 startbox = viewbox;
	auto d2c_scalerot = vcanvas->get_affine();
	auto _hruler = ruler;
	auto _vruler = _hruler + 1;
	auto zoom1 = d2c_scalerot[0].x;
	glm::vec2 ps = vcanvas->vpos;
	auto xxm = glm::inverse(d2c_scalerot);		// 逆矩阵
	auto rulerbox = get_boxm3(startbox, xxm);
	_hruler->set_range(rulerbox.x, rulerbox.z);	//设置水平标尺读数范围
	_vruler->is_yaxisdown = is_yaxisdown;
	if (is_yaxisdown)
	{
		_vruler->set_range(rulerbox.y, rulerbox.w);	//设置垂直标尺范围
	}
	else {
		_vruler->set_range(-rulerbox.y, -rulerbox.w);
	}

	pagebox = { 0,0, wsize };
	pagebox = get_boxm3(pagebox, d2c_scalerot);
	_hruler->set_page(pagebox.x, pagebox.z);//水平
	_vruler->set_page(pagebox.y, pagebox.w);

	selbox = {};
	// todo
	selbox = get_boxm3(selbox, d2c_scalerot);
	if (selbox.z > 0 && selbox.w > 0) {
		_hruler->set_selection(selbox.x, selbox.z);//水平
		_vruler->set_selection(selbox.y, selbox.w);
	}
}
void view_g::mkpath()
{
	auto vss = vsize;
	glm::vec2 sw = step_width;
	sw *= step_width.z;
	sw *= step_width.z; sw *= 2;
	vss += sw;
	int rct = make_grid(vss, step_width, { 0.5,0.5 }, pvdata);
	draw_grid_back(vss);
}


void view_g::draw_grid_back(const glm::ivec2& vss)
{
	print_time a("draw_grid_back\n");
	if (!_backing_store)
		_backing_store = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, vss.x, vss.y);
	if (!_backing_store)return;
	// Get context
	cairo_t* cr = cairo_create(_backing_store);
	{
		style_path_t st0 = {};
		style_path_t st1 = {};
		st1.stroke_color = wcolor.z;
		st1.line_width = 1;
		st0.fill_color = color.x;
		//st0.box_color = color.y;// 0x80FF8050;
		st0.stroke_color = color.z;// 0x80353535;
		st0.line_width = 1.0f;
		st0.cap = 0;
		st0.join = 1;
		auto stbc = st0;

		stbc.stroke_color = color.w;// 0x80606060;

		style_path_t* spt = &st0, * spt1 = &stbc;

		if (spt && pvdata[0].size() > 0) {
			draw_path_v(cr, &pvdata[0], spt);
		}
		if (spt1 && pvdata[1].size() > 1) {
			draw_path_v(cr, &pvdata[1], spt1);
		}

	}
	_backing_store_valid = true;
	cairo_destroy(cr);
}

glm::mat3 view_g::get_mat()
{
	return vcanvas->get_affine();
}

glm::mat3x2 view_g::get_mat3x2()
{
	auto m = vcanvas->get_affine();
	return glm::mat3x2(m);
}
glm::dmat3x2 view_g::get_dmat3x2()
{
	auto m = vcanvas->get_affine();
	return glm::dmat3x2(m);
}

void view_g::on_event(uint32_t type, et_un_t* ep)
{
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	switch (t)
	{
	case devent_type_e::mouse_move_e:
	{
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y }; mps -= vgpos;
		on_motion(mps);
	}
	break;
	case devent_type_e::mouse_button_e:
	{
		auto p = e->b;
		glm::ivec2 mps = { p->x,p->y }; mps -= vgpos;
		on_button(p->button, p->down, mps, p->clicks);
	}
	break;
	case devent_type_e::mouse_wheel_e:
	{
		auto p = e->w;
		on_wheel(p->x, p->y);
	}
	break;
	case devent_type_e::keyboard_e:
	{
		//on_keyboard(ep);
	}
	break;
	}
}

void view_g::begin_draw() {
	if (!cdv || !_draw_valid)return;
	auto cr = cdv->cr;
	cdv->begin(clear_color);
	cairo_save(cr);
	//cairo_translate(cr, vpos.x, vpos.y);
	auto ae = vcanvas->get_affine();//视图矩阵
	glm::vec2 aps = ae[2];
	glm::vec2 sw = step_width;	// 单个网格宽、高
	sw *= step_width.z;			// 数量
	aps = glm::mod(aps, sw) - sw;
	cairo_set_source_surface(cr, _backing_store, aps.x, aps.y);
	cairo_paint(cr);
	cairo_restore(cr);
	cairo_save(cr);
	auto tr = glm::transpose(ae);
	auto ainv = glm::affineInverse(ae);
	auto inv = glm::inverse(ae);
	auto invtr = glm::inverseTranspose(ae);
	//set_mat3(cr, ae);
	set_color(cr, cross_color.x);
	cairo_set_line_width(cr, cross_width);
	glm::vec2 ds = { vsize };
	auto _hruler = ruler;
	auto _vruler = _hruler + 1;
	ds.x = _hruler->_page_lower;
	ds.y = _vruler->_page_lower;

	if (cross_width == 1)
		ds += 0.5;
	// 十字线
	cairo_move_to(cr, 0, ds.y);
	cairo_line_to(cr, vsize.x, ds.y);
	cairo_stroke(cr);
	set_color(cr, cross_color.y);
	cairo_move_to(cr, ds.x, 0);
	cairo_line_to(cr, ds.x, vsize.y);
	cairo_stroke(cr);
	cairo_restore(cr);

}
void view_g::end_draw() {
	if (!cdv || !_draw_valid)return;
	auto cr = cdv->cr;
	auto _hruler = ruler;
	auto _vruler = _hruler + 1;
	cairo_save(cr);
	// 左上角背景
	cairo_set_line_width(cr, 1);
	set_color(cr, ruler ? ruler->_back_color : color.z);
	cairo_rectangle(cr, 0, 0, rsize.x, rsize.y);
	cairo_fill(cr);
	//set_source_rgba(cr, ruler->_foreground);
	int gradient_size = 4;
	paint_shadow(cr, 0, gradient_size, rsize.x, gradient_size, _vruler->_shadow, 0);// 垂直方向  
	paint_shadow(cr, gradient_size, 0, gradient_size, rsize.y, _vruler->_shadow, 0);// 水平方向 
	cairo_restore(cr);

	cairo_save(cr);
	// 标尺
	_hruler->on_drawing_area_draw(cr, {});
	_vruler->on_drawing_area_draw(cr, {});
	cairo_restore(cr);
	//裁剪
	clip_rect(cr, rss);
	cdv->end();
}
void set_mat3(cairo_t* cr, const glm::mat3& m) {
	auto mm = (glm::dmat3x2)m;
	cairo_set_matrix(cr, (cairo_matrix_t*)&mm);
}
void view_g::draw()
{
	if (!cdv || !_draw_valid)return;
	auto cr = cdv->cr;
	cairo_save(cr);
	vcanvas->draw(cdv);
	cairo_restore(cr);
}
void view_g::update(float)
{
	if (!_backing_store_valid)
	{
		mkpath();
		_draw_valid = true;
	}
	vcanvas->vpos = vpos;
	updateRulers();
	auto _hruler = ruler;
	auto _vruler = _hruler + 1;
	if (!vcanvas->_backing_store_valid || !_hruler->_backing_store_valid || !_vruler->_backing_store_valid)
		_draw_valid = true;

}

cairo_t* new_cr(cairo_surface_t* sur) {
	return sur ? cairo_create(sur) : nullptr;
}
void free_cr(cairo_t* cr) {
	if (cr)cairo_destroy(cr);
}

cairo_surface_t* load_imagesvg(const std::string& fn, float scale)
{
	cairo_surface_t* p = 0;
	if (fn.rfind(".svg") != std::string::npos)
	{
		svg_cx* bl = new_svg_file(fn.c_str(), 0, 96);
		int xn = bl->width * bl->height;
		float sc = scale > 1 ? scale : 1;
		auto blsur = new_image_cr({ bl->width * sc ,bl->height * sc });
		auto pxd = (uint32_t*)cairo_image_surface_get_data(blsur);
		if (pxd)
		{
			p = blsur;
			auto stride = cairo_image_surface_get_stride(blsur) / 4;
			stride *= bl->height;
			for (size_t i = 0; i < xn; i++)
			{
				pxd[i] = 0;
			}
			image_set_ud(p, key_def_data_svgptr, bl, (cairo_destroy_func_t)free_svg);
			std::thread th([=]()
				{
					print_time a("load svg");
					cairo_t* cr = cairo_create(blsur);
					render_svg(cr, bl, {}, { scale, scale }, 0, 0);
					image_set_ud(blsur, key_def_data_done, (void*)1, 0);
					cairo_destroy(cr);
				});
			th.detach();
		}
	}
	else {
		image_ptr_t* ptr = stbimage_load::new_load(fn.c_str(), 0);
		if (ptr)
		{
			p = new_image_cr(ptr);
			image_set_ud(p, key_def_data_iptr, ptr, (cairo_destroy_func_t)stbimage_load::free_img);
		}
	}
	return p;
}

#endif
//!no_cairo_