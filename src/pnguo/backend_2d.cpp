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

#ifdef min
#undef min
#undef max
#endif // min

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
	for (size_t i = 0; i < ss.x* ss.y; i++)
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
#endif

#if 1
//glm::ivec2 layout_text_x::get_text_rect(size_t idx, const void* str8, int len, int fontsize)

void draw_text(cairo_t* cr, layout_text_x* ltx, const void* str, int len, glm::vec4 text_rc, text_style_t* st, glm::ivec2* orc)
{
	glm::vec4 rc = text_rc;
	ltx->tem_rtv.clear();
	ltx->build_text(st->font, st->font_size, rc, st->text_align, str, len, ltx->tem_rtv);
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
	ltx->build_text(st->font, st->font_size, rc, st->text_align, str, len, ltx->tem_rtv);
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


#ifndef no_stb2d 

 /*
  * type
  */
typedef int64_t			XCG_FT_Int64;
typedef uint64_t		XCG_FT_UInt64;
typedef int32_t			XCG_FT_Int32;
typedef uint32_t		XCG_FT_UInt32;

#define XCG_FT_BOOL(x)	((XCG_FT_Bool)(x))
#ifndef TRUE
#define TRUE			1
#endif
#ifndef FALSE
#define FALSE			0
#endif
int builtin_clz(unsigned int x) {
	if (x == 0) return 32;
	int count = 0;
	while ((x & (1U << 31)) == 0) {
		count++;
		x <<= 1;
	}
	return count;
}
/*
 * math
 */
#define XCG_FT_MIN(a, b)		((a) < (b) ? (a) : (b))
#define XCG_FT_MAX(a, b)		((a) > (b) ? (a) : (b))
#define XCG_FT_ABS(a)			((a) < 0 ? -(a) : (a))
#define XCG_FT_HYPOT(x, y)		(x = XCG_FT_ABS(x), y = XCG_FT_ABS(y), x > y ? x + (3 * y >> 3) : y + (3 * x >> 3))
#define XCG_FT_ANGLE_PI			(180L << 16)
#define XCG_FT_ANGLE_2PI		(XCG_FT_ANGLE_PI * 2)
#define XCG_FT_ANGLE_PI2		(XCG_FT_ANGLE_PI / 2)
#define XCG_FT_ANGLE_PI4		(XCG_FT_ANGLE_PI / 4)

#define XCG_FT_MSB(x)			(31 - builtin_clz(x))
#define XCG_FT_PAD_FLOOR(x, n)	((x) & ~((n)-1))
#define XCG_FT_PAD_ROUND(x, n)	XCG_FT_PAD_FLOOR((x) + ((n) / 2), n)
#define XCG_FT_PAD_CEIL(x, n)	XCG_FT_PAD_FLOOR((x) + ((n) - 1), n)

#define XCG_FT_MOVE_SIGN(x, s) \
	do { \
	if(x < 0) { \
			x = -x; \
			s = -s; \
		} \
	} while(0)

XCG_FT_Long XCG_FT_MulFix(XCG_FT_Long a, XCG_FT_Long b)
{
	XCG_FT_Int s = 1;
	XCG_FT_Long c;

	XCG_FT_MOVE_SIGN(a, s);
	XCG_FT_MOVE_SIGN(b, s);
	c = (XCG_FT_Long)(((XCG_FT_Int64)a * b + 0x8000L) >> 16);

	return (s > 0) ? c : -c;
}

XCG_FT_Long XCG_FT_MulDiv(XCG_FT_Long a, XCG_FT_Long b, XCG_FT_Long c)
{
	XCG_FT_Int s = 1;
	XCG_FT_Long d;

	XCG_FT_MOVE_SIGN(a, s);
	XCG_FT_MOVE_SIGN(b, s);
	XCG_FT_MOVE_SIGN(c, s);
	d = (XCG_FT_Long)(c > 0 ? ((XCG_FT_Int64)a * b + (c >> 1)) / c : 0x7FFFFFFFL);
	return (s > 0) ? d : -d;
}

XCG_FT_Long XCG_FT_DivFix(XCG_FT_Long a, XCG_FT_Long b)
{
	XCG_FT_Int s = 1;
	XCG_FT_Long q;

	XCG_FT_MOVE_SIGN(a, s);
	XCG_FT_MOVE_SIGN(b, s);
	q = (XCG_FT_Long)(b > 0 ? (((XCG_FT_UInt64)a << 16) + (b >> 1)) / b : 0x7FFFFFFFL);
	return (s < 0 ? -q : q);
}

#define XCG_FT_TRIG_SCALE 		(0xDBD95B16UL)
#define XCG_FT_TRIG_SAFE_MSB	(29)
#define XCG_FT_TRIG_MAX_ITERS	(23)

static const XCG_FT_Fixed ft_trig_arctan_table[] = {
	1740967L, 919879L, 466945L, 234379L, 117304L, 58666L, 29335L, 14668L,
	7334L,    3667L,   1833L,   917L,    458L,    229L,   115L,   57L,
	29L,      14L,     7L,      4L,      2L,      1L
};

static XCG_FT_Fixed ft_trig_downscale(XCG_FT_Fixed val)
{
	XCG_FT_Fixed s;
	XCG_FT_Int64 v;

	s = val;
	val = XCG_FT_ABS(val);
	v = (val * (XCG_FT_Int64)XCG_FT_TRIG_SCALE) + 0x100000000UL;
	val = (XCG_FT_Fixed)(v >> 32);
	return (s >= 0) ? val : -val;
}

static XCG_FT_Int ft_trig_prenorm(XCG_FT_Vector* vec)
{
	XCG_FT_Pos x, y;
	XCG_FT_Int shift;

	x = vec->x;
	y = vec->y;

	shift = XCG_FT_MSB(XCG_FT_ABS(x) | XCG_FT_ABS(y));

	if (shift <= XCG_FT_TRIG_SAFE_MSB)
	{
		shift = XCG_FT_TRIG_SAFE_MSB - shift;
		vec->x = (XCG_FT_Pos)((XCG_FT_ULong)x << shift);
		vec->y = (XCG_FT_Pos)((XCG_FT_ULong)y << shift);
	}
	else
	{
		shift -= XCG_FT_TRIG_SAFE_MSB;
		vec->x = x >> shift;
		vec->y = y >> shift;
		shift = -shift;
	}
	return shift;
}

static void ft_trig_pseudo_rotate(XCG_FT_Vector* vec, XCG_FT_Angle theta)
{
	XCG_FT_Int i;
	XCG_FT_Fixed x, y, xtemp, b;
	const XCG_FT_Fixed* arctanptr;

	x = vec->x;
	y = vec->y;
	while (theta < -XCG_FT_ANGLE_PI4)
	{
		xtemp = y;
		y = -x;
		x = xtemp;
		theta += XCG_FT_ANGLE_PI2;
	}
	while (theta > XCG_FT_ANGLE_PI4)
	{
		xtemp = -y;
		y = x;
		x = xtemp;
		theta -= XCG_FT_ANGLE_PI2;
	}
	arctanptr = ft_trig_arctan_table;
	for (i = 1, b = 1; i < XCG_FT_TRIG_MAX_ITERS; b <<= 1, i++)
	{
		XCG_FT_Fixed v1 = ((y + b) >> i);
		XCG_FT_Fixed v2 = ((x + b) >> i);
		if (theta < 0)
		{
			xtemp = x + v1;
			y = y - v2;
			x = xtemp;
			theta += *arctanptr++;
		}
		else
		{
			xtemp = x - v1;
			y = y + v2;
			x = xtemp;
			theta -= *arctanptr++;
		}
	}
	vec->x = x;
	vec->y = y;
}

static void ft_trig_pseudo_polarize(XCG_FT_Vector* vec)
{
	XCG_FT_Angle theta;
	XCG_FT_Int i;
	XCG_FT_Fixed x, y, xtemp, b;
	const XCG_FT_Fixed* arctanptr;

	x = vec->x;
	y = vec->y;
	if (y > x)
	{
		if (y > -x)
		{
			theta = XCG_FT_ANGLE_PI2;
			xtemp = y;
			y = -x;
			x = xtemp;
		}
		else
		{
			theta = y > 0 ? XCG_FT_ANGLE_PI : -XCG_FT_ANGLE_PI;
			x = -x;
			y = -y;
		}
	}
	else
	{
		if (y < -x)
		{
			theta = -XCG_FT_ANGLE_PI2;
			xtemp = -y;
			y = x;
			x = xtemp;
		}
		else
		{
			theta = 0;
		}
	}
	arctanptr = ft_trig_arctan_table;
	for (i = 1, b = 1; i < XCG_FT_TRIG_MAX_ITERS; b <<= 1, i++)
	{
		XCG_FT_Fixed v1 = ((y + b) >> i);
		XCG_FT_Fixed v2 = ((x + b) >> i);
		if (y > 0)
		{
			xtemp = x + v1;
			y = y - v2;
			x = xtemp;
			theta += *arctanptr++;
		}
		else
		{
			xtemp = x - v1;
			y = y + v2;
			x = xtemp;
			theta -= *arctanptr++;
		}
	}
	if (theta >= 0)
		theta = XCG_FT_PAD_ROUND(theta, 32);
	else
		theta = -XCG_FT_PAD_ROUND(-theta, 32);
	vec->x = x;
	vec->y = theta;
}

XCG_FT_Fixed XCG_FT_Cos(XCG_FT_Angle angle)
{
	XCG_FT_Vector v;

	v.x = XCG_FT_TRIG_SCALE >> 8;
	v.y = 0;
	ft_trig_pseudo_rotate(&v, angle);
	return (v.x + 0x80L) >> 8;
}

XCG_FT_Fixed XCG_FT_Sin(XCG_FT_Angle angle)
{
	return XCG_FT_Cos(XCG_FT_ANGLE_PI2 - angle);
}

XCG_FT_Fixed XCG_FT_Tan(XCG_FT_Angle angle)
{
	XCG_FT_Vector v;

	v.x = XCG_FT_TRIG_SCALE >> 8;
	v.y = 0;
	ft_trig_pseudo_rotate(&v, angle);
	return XCG_FT_DivFix(v.y, v.x);
}

XCG_FT_Angle XCG_FT_Atan2(XCG_FT_Fixed dx, XCG_FT_Fixed dy)
{
	XCG_FT_Vector v;

	if (dx == 0 && dy == 0)
		return 0;
	v.x = dx;
	v.y = dy;
	ft_trig_prenorm(&v);
	ft_trig_pseudo_polarize(&v);
	return v.y;
}

void XCG_FT_Vector_Unit(XCG_FT_Vector* vec, XCG_FT_Angle angle)
{
	vec->x = XCG_FT_TRIG_SCALE >> 8;
	vec->y = 0;
	ft_trig_pseudo_rotate(vec, angle);
	vec->x = (vec->x + 0x80L) >> 8;
	vec->y = (vec->y + 0x80L) >> 8;
}

void XCG_FT_Vector_Rotate(XCG_FT_Vector* vec, XCG_FT_Angle angle)
{
	XCG_FT_Int shift;
	XCG_FT_Vector v = *vec;

	if (v.x == 0 && v.y == 0)
		return;
	shift = ft_trig_prenorm(&v);
	ft_trig_pseudo_rotate(&v, angle);
	v.x = ft_trig_downscale(v.x);
	v.y = ft_trig_downscale(v.y);

	if (shift > 0)
	{
		XCG_FT_Int32 half = (XCG_FT_Int32)1L << (shift - 1);

		vec->x = (v.x + half - (v.x < 0)) >> shift;
		vec->y = (v.y + half - (v.y < 0)) >> shift;
	}
	else
	{
		shift = -shift;
		vec->x = (XCG_FT_Pos)((XCG_FT_ULong)v.x << shift);
		vec->y = (XCG_FT_Pos)((XCG_FT_ULong)v.y << shift);
	}
}

XCG_FT_Fixed XCG_FT_Vector_Length(XCG_FT_Vector* vec)
{
	XCG_FT_Int shift;
	XCG_FT_Vector v;

	v = *vec;
	if (v.x == 0)
	{
		return XCG_FT_ABS(v.y);
	}
	else if (v.y == 0)
	{
		return XCG_FT_ABS(v.x);
	}

	shift = ft_trig_prenorm(&v);
	ft_trig_pseudo_polarize(&v);
	v.x = ft_trig_downscale(v.x);
	if (shift > 0)
		return (v.x + (1 << (shift - 1))) >> shift;

	return (XCG_FT_Fixed)((XCG_FT_UInt32)v.x << -shift);
}

void XCG_FT_Vector_Polarize(XCG_FT_Vector* vec, XCG_FT_Fixed* length, XCG_FT_Angle* angle)
{
	XCG_FT_Int shift;
	XCG_FT_Vector v;

	v = *vec;
	if (v.x == 0 && v.y == 0)
		return;
	shift = ft_trig_prenorm(&v);
	ft_trig_pseudo_polarize(&v);
	v.x = ft_trig_downscale(v.x);

	*length = (shift >= 0) ? (v.x >> shift) : (XCG_FT_Fixed)((XCG_FT_UInt32)v.x << -shift);
	*angle = v.y;
}

void XCG_FT_Vector_From_Polar(XCG_FT_Vector* vec, XCG_FT_Fixed length, XCG_FT_Angle angle)
{
	vec->x = length;
	vec->y = 0;

	XCG_FT_Vector_Rotate(vec, angle);
}

XCG_FT_Angle XCG_FT_Angle_Diff(XCG_FT_Angle angle1, XCG_FT_Angle angle2)
{
	XCG_FT_Angle delta = angle2 - angle1;

	while (delta <= -XCG_FT_ANGLE_PI)
		delta += XCG_FT_ANGLE_2PI;

	while (delta > XCG_FT_ANGLE_PI)
		delta -= XCG_FT_ANGLE_2PI;

	return delta;
}

/*
 * raster
 */
typedef long			TCoord;
typedef long			TPos;
typedef long			TArea;
typedef ptrdiff_t		XCG_FT_PtrDist;

#define xcg_ft_setjmp	setjmp
#define xcg_ft_longjmp 	longjmp
#define xcg_ft_jmp_buf	jmp_buf

#define ErrRaster_Invalid_Mode      -2
#define ErrRaster_Invalid_Outline   -1
#define ErrRaster_Invalid_Argument  -3
#define ErrRaster_Memory_Overflow   -4
#define ErrRaster_OutOfMemory       -6

#define XCG_FT_MINIMUM_POOL_SIZE 	8192
#define XCG_FT_MAX_GRAY_SPANS		256

#define RAS_ARG   					PWorker worker
#define RAS_ARG_					PWorker worker,
#define RAS_VAR 					worker
#define RAS_VAR_					worker,
#define ras							(*worker)

#define PIXEL_BITS		8
#define ONE_PIXEL		(1L << PIXEL_BITS)
#define TRUNC(x)		(TCoord)((x) >> PIXEL_BITS)
#define FRACT(x)		(TCoord)((x) & (ONE_PIXEL - 1))
#if PIXEL_BITS >= 6
#define UPSCALE(x)		((x) * (ONE_PIXEL >> 6))
#define DOWNSCALE(x)	((x) >> (PIXEL_BITS - 6))
#else
#define UPSCALE(x)		((x) >> (6 - PIXEL_BITS))
#define DOWNSCALE(x)	((x) * (64 >> PIXEL_BITS))
#endif

#define XCG_FT_DIV_MOD(type, dividend, divisor, quotient, remainder) \
	do { \
		(quotient) = (type)((dividend) / (divisor)); \
		(remainder) = (type)((dividend) % (divisor)); \
		if((remainder) < 0) \
		{ \
			(quotient)--; \
			(remainder) += (type)(divisor); \
		} \
	} while(0)

typedef struct TCell_* PCell;
typedef struct TCell_ {
	int x;
	int cover;
	TArea area;
	PCell next;
} TCell;

typedef struct TWorker_ {
	TCoord ex, ey;
	TPos min_ex, max_ex;
	TPos min_ey, max_ey;
	TPos count_ex, count_ey;
	TArea area;
	int cover;
	int invalid;
	PCell cells;
	XCG_FT_PtrDist max_cells;
	XCG_FT_PtrDist num_cells;
	TPos x, y;
	XCG_FT_Outline outline;
	XCG_FT_BBox clip_box;
	XCG_FT_Span gray_spans[XCG_FT_MAX_GRAY_SPANS];
	int num_gray_spans;
	int skip_spans;
	XCG_FT_Raster_Span_Func render_span;
	void* render_span_data;
	int band_size;
	int band_shoot;
	xcg_ft_jmp_buf jump_buffer;
	void* buffer;
	long buffer_size;
	PCell* ycells;
	TPos ycount;
} TWorker, * PWorker;

static void gray_init_cells(RAS_ARG_ void* buffer, long byte_size)
{
	ras.buffer = buffer;
	ras.buffer_size = byte_size;
	ras.ycells = (PCell*)buffer;
	ras.cells = NULL;
	ras.max_cells = 0;
	ras.num_cells = 0;
	ras.area = 0;
	ras.cover = 0;
	ras.invalid = 1;
}

static void gray_compute_cbox(RAS_ARG)
{
	XCG_FT_Outline* outline = &ras.outline;
	XCG_FT_Vector* vec = outline->points;
	XCG_FT_Vector* limit = vec + outline->n_points;

	if (outline->n_points <= 0)
	{
		ras.min_ex = ras.max_ex = 0;
		ras.min_ey = ras.max_ey = 0;
		return;
	}
	ras.min_ex = ras.max_ex = vec->x;
	ras.min_ey = ras.max_ey = vec->y;
	vec++;
	for (; vec < limit; vec++)
	{
		TPos x = vec->x;
		TPos y = vec->y;
		if (x < ras.min_ex)
			ras.min_ex = x;
		if (x > ras.max_ex)
			ras.max_ex = x;
		if (y < ras.min_ey)
			ras.min_ey = y;
		if (y > ras.max_ey)
			ras.max_ey = y;
	}
	ras.min_ex = ras.min_ex >> 6;
	ras.min_ey = ras.min_ey >> 6;
	ras.max_ex = (ras.max_ex + 63) >> 6;
	ras.max_ey = (ras.max_ey + 63) >> 6;
}

static PCell gray_find_cell(RAS_ARG)
{
	PCell* pcell, cell;
	TPos x = ras.ex;

	if (x > ras.count_ex)
		x = ras.count_ex;
	pcell = &ras.ycells[ras.ey];
	for (;;)
	{
		cell = *pcell;
		if (cell == NULL || cell->x > x)
			break;
		if (cell->x == x)
			goto Exit;
		pcell = &cell->next;
	}
	if (ras.num_cells >= ras.max_cells)
		xcg_ft_longjmp(ras.jump_buffer, 1);
	cell = ras.cells + ras.num_cells++;
	cell->x = x;
	cell->area = 0;
	cell->cover = 0;
	cell->next = *pcell;
	*pcell = cell;
Exit:
	return cell;
}

static void gray_record_cell(RAS_ARG)
{
	if (ras.area | ras.cover)
	{
		PCell cell = gray_find_cell(RAS_VAR);
		cell->area += ras.area;
		cell->cover += ras.cover;
	}
}

static void gray_set_cell(RAS_ARG_ TCoord ex, TCoord ey)
{
	ey -= ras.min_ey;
	if (ex > ras.max_ex)
		ex = ras.max_ex;
	ex -= ras.min_ex;
	if (ex < 0)
		ex = -1;
	if (ex != ras.ex || ey != ras.ey)
	{
		if (!ras.invalid)
			gray_record_cell(RAS_VAR);
		ras.area = 0;
		ras.cover = 0;
		ras.ex = ex;
		ras.ey = ey;
	}
	ras.invalid = ((unsigned int)ey >= (unsigned int)ras.count_ey || ex >= ras.count_ex);
}

static void gray_start_cell(RAS_ARG_ TCoord ex, TCoord ey)
{
	if (ex > ras.max_ex)
		ex = (TCoord)(ras.max_ex);
	if (ex < ras.min_ex)
		ex = (TCoord)(ras.min_ex - 1);
	ras.area = 0;
	ras.cover = 0;
	ras.ex = ex - ras.min_ex;
	ras.ey = ey - ras.min_ey;
	ras.invalid = 0;
	gray_set_cell(RAS_VAR_ ex, ey);
}

static void gray_render_scanline(RAS_ARG_ TCoord ey, TPos x1, TCoord y1, TPos x2, TCoord y2)
{
	TCoord ex1, ex2, fx1, fx2, first, dy, delta, mod;
	TPos p, dx;
	int incr;

	ex1 = TRUNC(x1);
	ex2 = TRUNC(x2);
	if (y1 == y2)
	{
		gray_set_cell(RAS_VAR_ ex2, ey);
		return;
	}
	fx1 = FRACT(x1);
	fx2 = FRACT(x2);
	if (ex1 == ex2)
		goto End;
	dx = x2 - x1;
	dy = y2 - y1;
	if (dx > 0)
	{
		p = (ONE_PIXEL - fx1) * dy;
		first = ONE_PIXEL;
		incr = 1;
	}
	else
	{
		p = fx1 * dy;
		first = 0;
		incr = -1;
		dx = -dx;
	}
	XCG_FT_DIV_MOD(TCoord, p, dx, delta, mod);
	ras.area += (TArea)(fx1 + first) * delta;
	ras.cover += delta;
	y1 += delta;
	ex1 += incr;
	gray_set_cell(RAS_VAR_ ex1, ey);
	if (ex1 != ex2)
	{
		TCoord lift, rem;
		p = ONE_PIXEL * dy;
		XCG_FT_DIV_MOD(TCoord, p, dx, lift, rem);
		do {
			delta = lift;
			mod += rem;
			if (mod >= (TCoord)dx)
			{
				mod -= (TCoord)dx;
				delta++;
			}
			ras.area += (TArea)(ONE_PIXEL * delta);
			ras.cover += delta;
			y1 += delta;
			ex1 += incr;
			gray_set_cell(RAS_VAR_ ex1, ey);
		} while (ex1 != ex2);
	}
	fx1 = ONE_PIXEL - first;
End:
	dy = y2 - y1;
	ras.area += (TArea)((fx1 + fx2) * dy);
	ras.cover += dy;
}

static void gray_render_line(RAS_ARG_ TPos to_x, TPos to_y)
{
	TCoord ey1, ey2, fy1, fy2, first, delta, mod;
	TPos p, dx, dy, x, x2;
	int incr;

	ey1 = TRUNC(ras.y);
	ey2 = TRUNC(to_y);
	if ((ey1 >= ras.max_ey && ey2 >= ras.max_ey) || (ey1 < ras.min_ey && ey2 < ras.min_ey))
		goto End;
	fy1 = FRACT(ras.y);
	fy2 = FRACT(to_y);
	if (ey1 == ey2)
	{
		gray_render_scanline(RAS_VAR_ ey1, ras.x, fy1, to_x, fy2);
		goto End;
	}
	dx = to_x - ras.x;
	dy = to_y - ras.y;
	if (dx == 0)
	{
		TCoord ex = TRUNC(ras.x);
		TCoord two_fx = FRACT(ras.x) << 1;
		TPos area, max_ey1;

		if (dy > 0)
		{
			first = ONE_PIXEL;
		}
		else
		{
			first = 0;
		}
		delta = first - fy1;
		ras.area += (TArea)two_fx * delta;
		ras.cover += delta;
		delta = first + first - ONE_PIXEL;
		area = (TArea)two_fx * delta;
		max_ey1 = ras.count_ey + ras.min_ey;
		if (dy < 0)
		{
			if (ey1 > max_ey1)
			{
				ey1 = (max_ey1 > ey2) ? max_ey1 : ey2;
				gray_set_cell(&ras, ex, ey1);
			}
			else
			{
				ey1--;
				gray_set_cell(&ras, ex, ey1);
			}
			while (ey1 > ey2 && ey1 >= ras.min_ey)
			{
				ras.area += area;
				ras.cover += delta;
				ey1--;

				gray_set_cell(&ras, ex, ey1);
			}
			if (ey1 != ey2)
			{
				ey1 = ey2;
				gray_set_cell(&ras, ex, ey1);
			}
		}
		else
		{
			if (ey1 < ras.min_ey)
			{
				ey1 = (ras.min_ey < ey2) ? ras.min_ey : ey2;
				gray_set_cell(&ras, ex, ey1);
			}
			else
			{
				ey1++;
				gray_set_cell(&ras, ex, ey1);
			}
			while (ey1 < ey2 && ey1 < max_ey1)
			{
				ras.area += area;
				ras.cover += delta;
				ey1++;

				gray_set_cell(&ras, ex, ey1);
			}
			if (ey1 != ey2)
			{
				ey1 = ey2;
				gray_set_cell(&ras, ex, ey1);
			}
		}
		delta = (int)(fy2 - ONE_PIXEL + first);
		ras.area += (TArea)two_fx * delta;
		ras.cover += delta;
		goto End;
	}
	if (dy > 0)
	{
		p = (ONE_PIXEL - fy1) * dx;
		first = ONE_PIXEL;
		incr = 1;
	}
	else
	{
		p = fy1 * dx;
		first = 0;
		incr = -1;
		dy = -dy;
	}
	XCG_FT_DIV_MOD(TCoord, p, dy, delta, mod);
	x = ras.x + delta;
	gray_render_scanline(RAS_VAR_ ey1, ras.x, fy1, x, (TCoord)first);
	ey1 += incr;
	gray_set_cell(RAS_VAR_ TRUNC(x), ey1);
	if (ey1 != ey2)
	{
		TCoord lift, rem;
		p = ONE_PIXEL * dx;
		XCG_FT_DIV_MOD(TCoord, p, dy, lift, rem);
		do {
			delta = lift;
			mod += rem;
			if (mod >= (TCoord)dy)
			{
				mod -= (TCoord)dy;
				delta++;
			}
			x2 = x + delta;
			gray_render_scanline(RAS_VAR_ ey1, x, ONE_PIXEL - first, x2, first);
			x = x2;
			ey1 += incr;
			gray_set_cell(RAS_VAR_ TRUNC(x), ey1);
		} while (ey1 != ey2);
	}
	gray_render_scanline(RAS_VAR_ ey1, x, ONE_PIXEL - first, to_x, fy2);
End:
	ras.x = to_x;
	ras.y = to_y;
}

static void gray_split_conic(XCG_FT_Vector* base)
{
	TPos a, b;

	base[4].x = base[2].x;
	b = base[1].x;
	a = base[3].x = (base[2].x + b) / 2;
	b = base[1].x = (base[0].x + b) / 2;
	base[2].x = (a + b) / 2;

	base[4].y = base[2].y;
	b = base[1].y;
	a = base[3].y = (base[2].y + b) / 2;
	b = base[1].y = (base[0].y + b) / 2;
	base[2].y = (a + b) / 2;
}

static void gray_render_conic(RAS_ARG_ const XCG_FT_Vector* control, const XCG_FT_Vector* to)
{
	XCG_FT_Vector bez_stack[16 * 2 + 1];
	XCG_FT_Vector* arc = bez_stack;
	TPos dx, dy;
	int draw, split;

	arc[0].x = UPSCALE(to->x);
	arc[0].y = UPSCALE(to->y);
	arc[1].x = UPSCALE(control->x);
	arc[1].y = UPSCALE(control->y);
	arc[2].x = ras.x;
	arc[2].y = ras.y;

	if ((TRUNC(arc[0].y) >= ras.max_ey &&
		TRUNC(arc[1].y) >= ras.max_ey &&
		TRUNC(arc[2].y) >= ras.max_ey) || (TRUNC(arc[0].y) < ras.min_ey &&
			TRUNC(arc[1].y) < ras.min_ey &&
			TRUNC(arc[2].y) < ras.min_ey))
	{
		ras.x = arc[0].x;
		ras.y = arc[0].y;
		return;
	}
	dx = XCG_FT_ABS(arc[2].x + arc[0].x - 2 * arc[1].x);
	dy = XCG_FT_ABS(arc[2].y + arc[0].y - 2 * arc[1].y);
	if (dx < dy)
		dx = dy;
	draw = 1;
	while (dx > ONE_PIXEL / 4)
	{
		dx >>= 2;
		draw <<= 1;
	}
	do {
		split = 1;
		while ((draw & split) == 0)
		{
			gray_split_conic(arc);
			arc += 2;
			split <<= 1;
		}
		gray_render_line(RAS_VAR_ arc[0].x, arc[0].y);
		arc -= 2;
	} while (--draw);
}

static void gray_split_cubic(XCG_FT_Vector* base)
{
	TPos a, b, c, d;

	base[6].x = base[3].x;
	c = base[1].x;
	d = base[2].x;
	base[1].x = a = (base[0].x + c) / 2;
	base[5].x = b = (base[3].x + d) / 2;
	c = (c + d) / 2;
	base[2].x = a = (a + c) / 2;
	base[4].x = b = (b + c) / 2;
	base[3].x = (a + b) / 2;

	base[6].y = base[3].y;
	c = base[1].y;
	d = base[2].y;
	base[1].y = a = (base[0].y + c) / 2;
	base[5].y = b = (base[3].y + d) / 2;
	c = (c + d) / 2;
	base[2].y = a = (a + c) / 2;
	base[4].y = b = (b + c) / 2;
	base[3].y = (a + b) / 2;
}

static void gray_render_cubic(RAS_ARG_ const XCG_FT_Vector* control1, const XCG_FT_Vector* control2, const XCG_FT_Vector* to)
{
	XCG_FT_Vector bez_stack[16 * 3 + 1];
	XCG_FT_Vector* arc = bez_stack;
	TPos dx, dy, dx_, dy_;
	TPos dx1, dy1, dx2, dy2;
	TPos L, s, s_limit;

	arc[0].x = UPSCALE(to->x);
	arc[0].y = UPSCALE(to->y);
	arc[1].x = UPSCALE(control2->x);
	arc[1].y = UPSCALE(control2->y);
	arc[2].x = UPSCALE(control1->x);
	arc[2].y = UPSCALE(control1->y);
	arc[3].x = ras.x;
	arc[3].y = ras.y;

	if ((TRUNC(arc[0].y) >= ras.max_ey &&
		TRUNC(arc[1].y) >= ras.max_ey &&
		TRUNC(arc[2].y) >= ras.max_ey &&
		TRUNC(arc[3].y) >= ras.max_ey) || (TRUNC(arc[0].y) < ras.min_ey &&
			TRUNC(arc[1].y) < ras.min_ey &&
			TRUNC(arc[2].y) < ras.min_ey &&
			TRUNC(arc[3].y) < ras.min_ey))
	{
		ras.x = arc[0].x;
		ras.y = arc[0].y;
		return;
	}
	for (;;)
	{
		dx = dx_ = arc[3].x - arc[0].x;
		dy = dy_ = arc[3].y - arc[0].y;
		L = XCG_FT_HYPOT(dx_, dy_);
		if (L >= (1 << 23))
			goto Split;
		s_limit = L * (TPos)(ONE_PIXEL / 6);
		dx1 = arc[1].x - arc[0].x;
		dy1 = arc[1].y - arc[0].y;
		s = XCG_FT_ABS(dy * dx1 - dx * dy1);
		if (s > s_limit)
			goto Split;
		dx2 = arc[2].x - arc[0].x;
		dy2 = arc[2].y - arc[0].y;
		s = XCG_FT_ABS(dy * dx2 - dx * dy2);
		if (s > s_limit)
			goto Split;
		if (dx1 * (dx1 - dx) + dy1 * (dy1 - dy) > 0 || dx2 * (dx2 - dx) + dy2 * (dy2 - dy) > 0)
			goto Split;
		gray_render_line(RAS_VAR_ arc[0].x, arc[0].y);
		if (arc == bez_stack)
			return;
		arc -= 3;
		continue;
	Split:
		gray_split_cubic(arc);
		arc += 3;
	}
}

static int gray_move_to(const XCG_FT_Vector* to, PWorker worker)
{
	TPos x, y;

	if (!ras.invalid)
		gray_record_cell(worker);
	x = UPSCALE(to->x);
	y = UPSCALE(to->y);
	gray_start_cell(worker, TRUNC(x), TRUNC(y));
	ras.x = x;
	ras.y = y;
	return 0;
}

static void gray_hline(RAS_ARG_ TCoord x, TCoord y, TPos area, int acount)
{
	int coverage;

	coverage = (int)(area >> (PIXEL_BITS * 2 + 1 - 8));
	if (coverage < 0)
		coverage = -coverage;
	if (ras.outline.flags & XCG_FT_OUTLINE_EVEN_ODD_FILL)
	{
		coverage &= 511;
		if (coverage > 256)
			coverage = 512 - coverage;
		else if (coverage == 256)
			coverage = 255;
	}
	else
	{
		if (coverage >= 256)
			coverage = 255;
	}
	y += (TCoord)ras.min_ey;
	x += (TCoord)ras.min_ex;
	if (x >= (1 << 23))
		x = (1 << 23) - 1;
	if (y >= (1 << 23))
		y = (1 << 23) - 1;
	if (coverage)
	{
		XCG_FT_Span* span;
		int count;
		int skip;
		count = ras.num_gray_spans;
		span = ras.gray_spans + count - 1;
		if (count > 0 && span->y == y && span->x + span->len == x && span->coverage == coverage)
		{
			span->len = span->len + acount;
			return;
		}
		if (count >= XCG_FT_MAX_GRAY_SPANS)
		{
			if (ras.render_span && count > ras.skip_spans)
			{
				skip = ras.skip_spans > 0 ? ras.skip_spans : 0;
				ras.render_span(ras.num_gray_spans - skip,
					ras.gray_spans + skip,
					ras.render_span_data);
			}
			ras.skip_spans -= ras.num_gray_spans;
			ras.num_gray_spans = 0;
			span = ras.gray_spans;
		}
		else
			span++;
		span->x = x;
		span->len = acount;
		span->y = y;
		span->coverage = (unsigned char)coverage;
		ras.num_gray_spans++;
	}
}

static void gray_sweep(RAS_ARG)
{
	int yindex;

	if (ras.num_cells == 0)
		return;
	for (yindex = 0; yindex < ras.ycount; yindex++)
	{
		PCell cell = ras.ycells[yindex];
		TCoord cover = 0;
		TCoord x = 0;

		for (; cell != NULL; cell = cell->next)
		{
			TArea area;
			if (cell->x > x && cover != 0)
				gray_hline(RAS_VAR_ x, yindex, cover * (ONE_PIXEL * 2), cell->x - x);
			cover += cell->cover;
			area = cover * (ONE_PIXEL * 2) - cell->area;
			if (area != 0 && cell->x >= 0)
				gray_hline(RAS_VAR_ cell->x, yindex, area, 1);
			x = cell->x + 1;
		}
		if (ras.count_ex > x && cover != 0)
			gray_hline(RAS_VAR_ x, yindex, cover * (ONE_PIXEL * 2), ras.count_ex - x);
	}
}

static int XCG_FT_Outline_Decompose(const XCG_FT_Outline* outline, void* user)
{
#undef SCALED
#define SCALED( x )  (x)
	XCG_FT_Vector v_last;
	XCG_FT_Vector v_control;
	XCG_FT_Vector v_start;
	XCG_FT_Vector* point;
	XCG_FT_Vector* limit;
	char* tags;
	int n;
	int first;
	int error;
	char tag;

	if (!outline)
		return ErrRaster_Invalid_Outline;
	first = 0;
	for (n = 0; n < outline->n_contours; n++)
	{
		int last;
		last = outline->contours[n];
		if (last < 0)
			goto Invalid_Outline;
		limit = outline->points + last;
		v_start = outline->points[first];
		v_start.x = SCALED(v_start.x);
		v_start.y = SCALED(v_start.y);
		v_last = outline->points[last];
		v_last.x = SCALED(v_last.x);
		v_last.y = SCALED(v_last.y);
		v_control = v_start;
		point = outline->points + first;
		tags = outline->tags + first;
		tag = XCG_FT_CURVE_TAG(tags[0]);
		if (tag == XCG_FT_CURVE_TAG_CUBIC)
			goto Invalid_Outline;
		if (tag == XCG_FT_CURVE_TAG_CONIC)
		{
			if (XCG_FT_CURVE_TAG(outline->tags[last]) == XCG_FT_CURVE_TAG_ON)
			{
				v_start = v_last;
				limit--;
			}
			else
			{
				v_start.x = (v_start.x + v_last.x) / 2;
				v_start.y = (v_start.y + v_last.y) / 2;
				v_last = v_start;
			}
			point--;
			tags--;
		}
		error = gray_move_to(&v_start, (PWorker)user);
		if (error)
			goto Exit;
		while (point < limit)
		{
			point++;
			tags++;
			tag = XCG_FT_CURVE_TAG(tags[0]);
			switch (tag)
			{
			case XCG_FT_CURVE_TAG_ON:
			{
				XCG_FT_Vector vec;
				vec.x = SCALED(point->x);
				vec.y = SCALED(point->y);
				gray_render_line((PWorker)user, UPSCALE(vec.x), UPSCALE(vec.y));
				continue;
			}
			case XCG_FT_CURVE_TAG_CONIC:
			{
				v_control.x = SCALED(point->x);
				v_control.y = SCALED(point->y);
			Do_Conic:
				if (point < limit)
				{
					XCG_FT_Vector vec;
					XCG_FT_Vector v_middle;
					point++;
					tags++;
					tag = XCG_FT_CURVE_TAG(tags[0]);
					vec.x = SCALED(point->x);
					vec.y = SCALED(point->y);
					if (tag == XCG_FT_CURVE_TAG_ON)
					{
						gray_render_conic((PWorker)user, &v_control, &vec);
						continue;
					}
					if (tag != XCG_FT_CURVE_TAG_CONIC)
						goto Invalid_Outline;
					v_middle.x = (v_control.x + vec.x) / 2;
					v_middle.y = (v_control.y + vec.y) / 2;
					gray_render_conic((PWorker)user, &v_control, &v_middle);
					v_control = vec;
					goto Do_Conic;
				}
				gray_render_conic((PWorker)user, &v_control, &v_start);
				goto Close;
			}
			default:
			{
				XCG_FT_Vector vec1, vec2;
				if (point + 1 > limit ||
					XCG_FT_CURVE_TAG(tags[1]) != XCG_FT_CURVE_TAG_CUBIC)
					goto Invalid_Outline;
				point += 2;
				tags += 2;
				vec1.x = SCALED(point[-2].x);
				vec1.y = SCALED(point[-2].y);
				vec2.x = SCALED(point[-1].x);
				vec2.y = SCALED(point[-1].y);
				if (point <= limit)
				{
					XCG_FT_Vector vec;
					vec.x = SCALED(point->x);
					vec.y = SCALED(point->y);
					gray_render_cubic((PWorker)user, &vec1, &vec2, &vec);
					continue;
				}
				gray_render_cubic((PWorker)user, &vec1, &vec2, &v_start);
				goto Close;
			}
			}
		}
		gray_render_line((PWorker)user, UPSCALE(v_start.x), UPSCALE(v_start.y));
	Close:
		first = last + 1;
	}
	return 0;
Exit:
	return error;
Invalid_Outline:
	return ErrRaster_Invalid_Outline;
}

typedef struct TBand_ {
	TPos min, max;
} TBand;

static int gray_convert_glyph_inner(RAS_ARG)
{
	volatile int error = 0;

	if (xcg_ft_setjmp(ras.jump_buffer) == 0)
	{
		error = XCG_FT_Outline_Decompose(&ras.outline, &ras);
		if (!ras.invalid)
			gray_record_cell(RAS_VAR);
	}
	else
	{
		error = ErrRaster_Memory_Overflow;
	}
	return error;
}

static int gray_convert_glyph(RAS_ARG)
{
	TBand bands[40];
	TBand* volatile band;
	int volatile n, num_bands;
	TPos volatile min, max, max_y;
	XCG_FT_BBox* clip;
	int skip;

	ras.num_gray_spans = 0;
	gray_compute_cbox(RAS_VAR);
	clip = &ras.clip_box;
	if (ras.max_ex <= clip->xMin || ras.min_ex >= clip->xMax || ras.max_ey <= clip->yMin || ras.min_ey >= clip->yMax)
		return 0;
	if (ras.min_ex < clip->xMin)
		ras.min_ex = clip->xMin;
	if (ras.min_ey < clip->yMin)
		ras.min_ey = clip->yMin;
	if (ras.max_ex > clip->xMax)
		ras.max_ex = clip->xMax;
	if (ras.max_ey > clip->yMax)
		ras.max_ey = clip->yMax;
	ras.count_ex = ras.max_ex - ras.min_ex;
	ras.count_ey = ras.max_ey - ras.min_ey;
	num_bands = (int)((ras.max_ey - ras.min_ey) / ras.band_size);
	if (num_bands == 0)
		num_bands = 1;
	if (num_bands >= 39)
		num_bands = 39;
	ras.band_shoot = 0;
	min = ras.min_ey;
	max_y = ras.max_ey;
	for (n = 0; n < num_bands; n++, min = max)
	{
		max = min + ras.band_size;
		if (n == num_bands - 1 || max > max_y)
			max = max_y;
		bands[0].min = min;
		bands[0].max = max;
		band = bands;
		while (band >= bands)
		{
			TPos bottom, top, middle;
			int error;
			{
				PCell cells_max;
				int yindex;
				int cell_start, cell_end, cell_mod;
				ras.ycells = (PCell*)ras.buffer;
				ras.ycount = band->max - band->min;
				cell_start = sizeof(PCell) * ras.ycount;
				cell_mod = cell_start % sizeof(TCell);
				if (cell_mod > 0)
					cell_start += sizeof(TCell) - cell_mod;
				cell_end = ras.buffer_size;
				cell_end -= cell_end % sizeof(TCell);
				cells_max = (PCell)((char*)ras.buffer + cell_end);
				ras.cells = (PCell)((char*)ras.buffer + cell_start);
				if (ras.cells >= cells_max)
					goto ReduceBands;
				ras.max_cells = (int)(cells_max - ras.cells);
				if (ras.max_cells < 2)
					goto ReduceBands;
				for (yindex = 0; yindex < ras.ycount; yindex++)
					ras.ycells[yindex] = NULL;
			}
			ras.num_cells = 0;
			ras.invalid = 1;
			ras.min_ey = band->min;
			ras.max_ey = band->max;
			ras.count_ey = band->max - band->min;
			error = gray_convert_glyph_inner(RAS_VAR);
			if (!error)
			{
				gray_sweep(RAS_VAR);
				band--;
				continue;
			}
			else if (error != ErrRaster_Memory_Overflow)
				return 1;
		ReduceBands:
			bottom = band->min;
			top = band->max;
			middle = bottom + ((top - bottom) >> 1);
			if (middle == bottom)
			{
				return ErrRaster_OutOfMemory;
			}
			if (bottom - top >= ras.band_size)
				ras.band_shoot++;
			band[1].min = bottom;
			band[1].max = middle;
			band[0].min = middle;
			band[0].max = top;
			band++;
		}
	}
	if (ras.render_span && ras.num_gray_spans > ras.skip_spans)
	{
		skip = ras.skip_spans > 0 ? ras.skip_spans : 0;
		ras.render_span(ras.num_gray_spans - skip,
			ras.gray_spans + skip,
			ras.render_span_data);
	}
	ras.skip_spans -= ras.num_gray_spans;
	if (ras.band_shoot > 8 && ras.band_size > 16)
		ras.band_size = ras.band_size / 2;
	return 0;
}

static int gray_raster_render(RAS_ARG_ void* buffer, long buffer_size, const XCG_FT_Raster_Params* params)
{
	const XCG_FT_Outline* outline = (const XCG_FT_Outline*)params->source;
	if (outline == NULL)
		return ErrRaster_Invalid_Outline;
	if (outline->n_points == 0 || outline->n_contours <= 0)
		return 0;
	if (!outline->contours || !outline->points)
		return ErrRaster_Invalid_Outline;
	if (outline->n_points != outline->contours[outline->n_contours - 1] + 1)
		return ErrRaster_Invalid_Outline;
	if (!(params->flags & XCG_FT_RASTER_FLAG_AA))
		return ErrRaster_Invalid_Mode;
	if (!(params->flags & XCG_FT_RASTER_FLAG_DIRECT))
		return ErrRaster_Invalid_Mode;
	if (params->flags & XCG_FT_RASTER_FLAG_CLIP)
	{
		ras.clip_box = params->clip_box;
	}
	else
	{
		ras.clip_box.xMin = -(1 << 23);
		ras.clip_box.yMin = -(1 << 23);
		ras.clip_box.xMax = (1 << 23) - 1;
		ras.clip_box.yMax = (1 << 23) - 1;
	}
	gray_init_cells(RAS_VAR_ buffer, buffer_size);
	ras.outline = *outline;
	ras.num_cells = 0;
	ras.invalid = 1;
	ras.band_size = (int)(buffer_size / (long)(sizeof(TCell) * 8));
	ras.render_span = (XCG_FT_Raster_Span_Func)params->gray_spans;
	ras.render_span_data = params->user;
	return gray_convert_glyph(RAS_VAR);
}

void XCG_FT_Raster_Render(const XCG_FT_Raster_Params* params)
{
	char stack[XCG_FT_MINIMUM_POOL_SIZE];
	size_t length = XCG_FT_MINIMUM_POOL_SIZE;

	TWorker worker;
	worker.skip_spans = 0;
	int rendered_spans = 0;
	int error = gray_raster_render(&worker, stack, length, params);
	while (error == ErrRaster_OutOfMemory)
	{
		if (worker.skip_spans < 0)
			rendered_spans += -worker.skip_spans;
		worker.skip_spans = rendered_spans;
		length *= 2;
		void* heap = malloc(length);
		error = gray_raster_render(&worker, heap, length, params);
		free(heap);
	}
}

/*
 * stroker
 */
#define XCG_FT_SMALL_CONIC_THRESHOLD	(XCG_FT_ANGLE_PI / 6)
#define XCG_FT_SMALL_CUBIC_THRESHOLD	(XCG_FT_ANGLE_PI / 8)
#define XCG_FT_IS_SMALL(x)				((x) > -2 && (x) < 2)

static XCG_FT_Pos ft_pos_abs(XCG_FT_Pos x)
{
	return x >= 0 ? x : -x;
}

static void ft_conic_split(XCG_FT_Vector* base)
{
	XCG_FT_Pos a, b;

	base[4].x = base[2].x;
	a = base[0].x + base[1].x;
	b = base[1].x + base[2].x;
	base[3].x = b >> 1;
	base[2].x = (a + b) >> 2;
	base[1].x = a >> 1;

	base[4].y = base[2].y;
	a = base[0].y + base[1].y;
	b = base[1].y + base[2].y;
	base[3].y = b >> 1;
	base[2].y = (a + b) >> 2;
	base[1].y = a >> 1;
}

static XCG_FT_Bool ft_conic_is_small_enough(XCG_FT_Vector* base, XCG_FT_Angle* angle_in, XCG_FT_Angle* angle_out)
{
	XCG_FT_Vector d1, d2;
	XCG_FT_Angle theta;
	XCG_FT_Int close1, close2;

	d1.x = base[1].x - base[2].x;
	d1.y = base[1].y - base[2].y;
	d2.x = base[0].x - base[1].x;
	d2.y = base[0].y - base[1].y;
	close1 = XCG_FT_IS_SMALL(d1.x) && XCG_FT_IS_SMALL(d1.y);
	close2 = XCG_FT_IS_SMALL(d2.x) && XCG_FT_IS_SMALL(d2.y);
	if (close1)
	{
		if (close2)
		{
		}
		else
		{
			*angle_in = *angle_out = XCG_FT_Atan2(d2.x, d2.y);
		}
	}
	else
	{
		if (close2)
		{
			*angle_in = *angle_out = XCG_FT_Atan2(d1.x, d1.y);
		}
		else
		{
			*angle_in = XCG_FT_Atan2(d1.x, d1.y);
			*angle_out = XCG_FT_Atan2(d2.x, d2.y);
		}
	}
	theta = ft_pos_abs(XCG_FT_Angle_Diff(*angle_in, *angle_out));
	return XCG_FT_BOOL(theta < XCG_FT_SMALL_CONIC_THRESHOLD);
}

static void ft_cubic_split(XCG_FT_Vector* base)
{
	XCG_FT_Pos a, b, c;

	base[6].x = base[3].x;
	a = base[0].x + base[1].x;
	b = base[1].x + base[2].x;
	c = base[2].x + base[3].x;
	base[5].x = c >> 1;
	c += b;
	base[4].x = c >> 2;
	base[1].x = a >> 1;
	a += b;
	base[2].x = a >> 2;
	base[3].x = (a + c) >> 3;

	base[6].y = base[3].y;
	a = base[0].y + base[1].y;
	b = base[1].y + base[2].y;
	c = base[2].y + base[3].y;
	base[5].y = c >> 1;
	c += b;
	base[4].y = c >> 2;
	base[1].y = a >> 1;
	a += b;
	base[2].y = a >> 2;
	base[3].y = (a + c) >> 3;
}

static XCG_FT_Angle ft_angle_mean(XCG_FT_Angle angle1, XCG_FT_Angle angle2)
{
	return angle1 + XCG_FT_Angle_Diff(angle1, angle2) / 2;
}

static XCG_FT_Bool ft_cubic_is_small_enough(XCG_FT_Vector* base, XCG_FT_Angle* angle_in, XCG_FT_Angle* angle_mid, XCG_FT_Angle* angle_out)
{
	XCG_FT_Vector d1, d2, d3;
	XCG_FT_Angle theta1, theta2;
	XCG_FT_Int close1, close2, close3;

	d1.x = base[2].x - base[3].x;
	d1.y = base[2].y - base[3].y;
	d2.x = base[1].x - base[2].x;
	d2.y = base[1].y - base[2].y;
	d3.x = base[0].x - base[1].x;
	d3.y = base[0].y - base[1].y;
	close1 = XCG_FT_IS_SMALL(d1.x) && XCG_FT_IS_SMALL(d1.y);
	close2 = XCG_FT_IS_SMALL(d2.x) && XCG_FT_IS_SMALL(d2.y);
	close3 = XCG_FT_IS_SMALL(d3.x) && XCG_FT_IS_SMALL(d3.y);

	if (close1)
	{
		if (close2)
		{
			if (close3)
			{
			}
			else
			{
				*angle_in = *angle_mid = *angle_out = XCG_FT_Atan2(d3.x, d3.y);
			}
		}
		else
		{
			if (close3)
			{
				*angle_in = *angle_mid = *angle_out = XCG_FT_Atan2(d2.x, d2.y);
			}
			else
			{
				*angle_in = *angle_mid = XCG_FT_Atan2(d2.x, d2.y);
				*angle_out = XCG_FT_Atan2(d3.x, d3.y);
			}
		}
	}
	else
	{
		if (close2)
		{
			if (close3)
			{
				*angle_in = *angle_mid = *angle_out = XCG_FT_Atan2(d1.x, d1.y);
			}
			else
			{
				*angle_in = XCG_FT_Atan2(d1.x, d1.y);
				*angle_out = XCG_FT_Atan2(d3.x, d3.y);
				*angle_mid = ft_angle_mean(*angle_in, *angle_out);
			}
		}
		else
		{
			if (close3)
			{
				*angle_in = XCG_FT_Atan2(d1.x, d1.y);
				*angle_mid = *angle_out = XCG_FT_Atan2(d2.x, d2.y);
			}
			else
			{
				*angle_in = XCG_FT_Atan2(d1.x, d1.y);
				*angle_mid = XCG_FT_Atan2(d2.x, d2.y);
				*angle_out = XCG_FT_Atan2(d3.x, d3.y);
			}
		}
	}
	theta1 = ft_pos_abs(XCG_FT_Angle_Diff(*angle_in, *angle_mid));
	theta2 = ft_pos_abs(XCG_FT_Angle_Diff(*angle_mid, *angle_out));
	return XCG_FT_BOOL(theta1 < XCG_FT_SMALL_CUBIC_THRESHOLD && theta2 < XCG_FT_SMALL_CUBIC_THRESHOLD);
}

typedef enum XCG_FT_StrokeTags_ {
	XCG_FT_STROKE_TAG_ON = 1,
	XCG_FT_STROKE_TAG_CUBIC = 2,
	XCG_FT_STROKE_TAG_BEGIN = 4,
	XCG_FT_STROKE_TAG_END = 8,
} XCG_FT_StrokeTags;

#define XCG_FT_STROKE_TAG_BEGIN_END		(XCG_FT_STROKE_TAG_BEGIN | XCG_FT_STROKE_TAG_END)

typedef struct XCG_FT_StrokeBorderRec_ {
	XCG_FT_UInt num_points;
	XCG_FT_UInt max_points;
	XCG_FT_Vector* points;
	XCG_FT_Byte* tags;
	XCG_FT_Bool movable;
	XCG_FT_Int start;
	XCG_FT_Bool valid;
} XCG_FT_StrokeBorderRec, * XCG_FT_StrokeBorder;

XCG_FT_Error XCG_FT_Outline_Check(XCG_FT_Outline* outline)
{
	if (outline)
	{
		XCG_FT_Int n_points = outline->n_points;
		XCG_FT_Int n_contours = outline->n_contours;
		XCG_FT_Int end0, end;
		XCG_FT_Int n;

		if (n_points == 0 && n_contours == 0)
			return 0;
		if (n_points <= 0 || n_contours <= 0)
			goto Bad;
		end0 = end = -1;
		for (n = 0; n < n_contours; n++)
		{
			end = outline->contours[n];
			if (end <= end0 || end >= n_points)
				goto Bad;
			end0 = end;
		}
		if (end != n_points - 1)
			goto Bad;
		return 0;
	}
Bad:
	return -1;
}

void XCG_FT_Outline_Get_CBox(const XCG_FT_Outline* outline, XCG_FT_BBox* acbox)
{
	XCG_FT_Pos xMin, yMin, xMax, yMax;

	if (outline && acbox)
	{
		if (outline->n_points == 0)
		{
			xMin = 0;
			yMin = 0;
			xMax = 0;
			yMax = 0;
		}
		else
		{
			XCG_FT_Vector* vec = outline->points;
			XCG_FT_Vector* limit = vec + outline->n_points;
			xMin = xMax = vec->x;
			yMin = yMax = vec->y;
			vec++;
			for (; vec < limit; vec++)
			{
				XCG_FT_Pos x, y;
				x = vec->x;
				if (x < xMin)
					xMin = x;
				if (x > xMax)
					xMax = x;
				y = vec->y;
				if (y < yMin)
					yMin = y;
				if (y > yMax)
					yMax = y;
			}
		}
		acbox->xMin = xMin;
		acbox->xMax = xMax;
		acbox->yMin = yMin;
		acbox->yMax = yMax;
	}
}

static XCG_FT_Error ft_stroke_border_grow(XCG_FT_StrokeBorder border, XCG_FT_UInt new_points)
{
	XCG_FT_UInt old_max = border->max_points;
	XCG_FT_UInt new_max = border->num_points + new_points;
	XCG_FT_Error error = 0;

	if (new_max > old_max)
	{
		XCG_FT_UInt cur_max = old_max;
		while (cur_max < new_max)
			cur_max += (cur_max >> 1) + 16;
		border->points = (XCG_FT_Vector*)realloc(border->points, cur_max * sizeof(XCG_FT_Vector));
		border->tags = (XCG_FT_Byte*)realloc(border->tags, cur_max * sizeof(XCG_FT_Byte));
		if (!border->points || !border->tags)
			goto Exit;
		border->max_points = cur_max;
	}
Exit:
	return error;
}

static void ft_stroke_border_close(XCG_FT_StrokeBorder border, XCG_FT_Bool reverse)
{
	XCG_FT_UInt start = border->start;
	XCG_FT_UInt count = border->num_points;

	if (count <= start + 1U)
		border->num_points = start;
	else
	{
		border->num_points = --count;
		border->points[start] = border->points[count];
		border->tags[start] = border->tags[count];
		if (reverse)
		{
			{
				XCG_FT_Vector* vec1 = border->points + start + 1;
				XCG_FT_Vector* vec2 = border->points + count - 1;
				for (; vec1 < vec2; vec1++, vec2--)
				{
					XCG_FT_Vector tmp;
					tmp = *vec1;
					*vec1 = *vec2;
					*vec2 = tmp;
				}
			}
			{
				XCG_FT_Byte* tag1 = border->tags + start + 1;
				XCG_FT_Byte* tag2 = border->tags + count - 1;

				for (; tag1 < tag2; tag1++, tag2--)
				{
					XCG_FT_Byte tmp;
					tmp = *tag1;
					*tag1 = *tag2;
					*tag2 = tmp;
				}
			}
		}
		border->tags[start] |= XCG_FT_STROKE_TAG_BEGIN;
		border->tags[count - 1] |= XCG_FT_STROKE_TAG_END;
	}
	border->start = -1;
	border->movable = FALSE;
}

static XCG_FT_Error ft_stroke_border_lineto(XCG_FT_StrokeBorder border, XCG_FT_Vector* to, XCG_FT_Bool movable)
{
	XCG_FT_Error error = 0;

	if (border->movable)
	{
		border->points[border->num_points - 1] = *to;
	}
	else
	{
		if (border->num_points > 0 &&
			XCG_FT_IS_SMALL(border->points[border->num_points - 1].x - to->x) &&
			XCG_FT_IS_SMALL(border->points[border->num_points - 1].y - to->y))
			return error;
		error = ft_stroke_border_grow(border, 1);
		if (!error)
		{
			XCG_FT_Vector* vec = border->points + border->num_points;
			XCG_FT_Byte* tag = border->tags + border->num_points;
			vec[0] = *to;
			tag[0] = XCG_FT_STROKE_TAG_ON;
			border->num_points += 1;
		}
	}
	border->movable = movable;
	return error;
}

static XCG_FT_Error ft_stroke_border_conicto(XCG_FT_StrokeBorder border, XCG_FT_Vector* control, XCG_FT_Vector* to)
{
	XCG_FT_Error error;

	error = ft_stroke_border_grow(border, 2);
	if (!error)
	{
		XCG_FT_Vector* vec = border->points + border->num_points;
		XCG_FT_Byte* tag = border->tags + border->num_points;

		vec[0] = *control;
		vec[1] = *to;
		tag[0] = 0;
		tag[1] = XCG_FT_STROKE_TAG_ON;
		border->num_points += 2;
	}
	border->movable = FALSE;
	return error;
}

static XCG_FT_Error ft_stroke_border_cubicto(XCG_FT_StrokeBorder border, XCG_FT_Vector* control1, XCG_FT_Vector* control2, XCG_FT_Vector* to)
{
	XCG_FT_Error error;

	error = ft_stroke_border_grow(border, 3);
	if (!error)
	{
		XCG_FT_Vector* vec = border->points + border->num_points;
		XCG_FT_Byte* tag = border->tags + border->num_points;

		vec[0] = *control1;
		vec[1] = *control2;
		vec[2] = *to;
		tag[0] = XCG_FT_STROKE_TAG_CUBIC;
		tag[1] = XCG_FT_STROKE_TAG_CUBIC;
		tag[2] = XCG_FT_STROKE_TAG_ON;
		border->num_points += 3;
	}
	border->movable = FALSE;
	return error;
}

#define XCG_FT_ARC_CUBIC_ANGLE		(XCG_FT_ANGLE_PI / 2)

static XCG_FT_Error ft_stroke_border_arcto(XCG_FT_StrokeBorder border, XCG_FT_Vector* center, XCG_FT_Fixed radius, XCG_FT_Angle angle_start, XCG_FT_Angle angle_diff)
{
	XCG_FT_Fixed coef;
	XCG_FT_Vector a0, a1, a2, a3;
	XCG_FT_Int i, arcs = 1;
	XCG_FT_Error error = 0;

	while (angle_diff > XCG_FT_ARC_CUBIC_ANGLE * arcs || -angle_diff > XCG_FT_ARC_CUBIC_ANGLE * arcs)
		arcs++;
	coef = XCG_FT_Tan(angle_diff / (4 * arcs));
	coef += coef / 3;
	XCG_FT_Vector_From_Polar(&a0, radius, angle_start);
	a1.x = XCG_FT_MulFix(-a0.y, coef);
	a1.y = XCG_FT_MulFix(a0.x, coef);
	a0.x += center->x;
	a0.y += center->y;
	a1.x += a0.x;
	a1.y += a0.y;
	for (i = 1; i <= arcs; i++)
	{
		XCG_FT_Vector_From_Polar(&a3, radius, angle_start + i * angle_diff / arcs);
		a2.x = XCG_FT_MulFix(a3.y, coef);
		a2.y = XCG_FT_MulFix(-a3.x, coef);
		a3.x += center->x;
		a3.y += center->y;
		a2.x += a3.x;
		a2.y += a3.y;
		error = ft_stroke_border_cubicto(border, &a1, &a2, &a3);
		if (error)
			break;
		a1.x = a3.x - a2.x + a3.x;
		a1.y = a3.y - a2.y + a3.y;
	}
	return error;
}

static XCG_FT_Error ft_stroke_border_moveto(XCG_FT_StrokeBorder border, XCG_FT_Vector* to)
{
	if (border->start >= 0)
		ft_stroke_border_close(border, FALSE);
	border->start = border->num_points;
	border->movable = FALSE;
	return ft_stroke_border_lineto(border, to, FALSE);
}

static void ft_stroke_border_init(XCG_FT_StrokeBorder border)
{
	border->points = NULL;
	border->tags = NULL;
	border->num_points = 0;
	border->max_points = 0;
	border->start = -1;
	border->valid = FALSE;
}

static void ft_stroke_border_reset(XCG_FT_StrokeBorder border)
{
	border->num_points = 0;
	border->start = -1;
	border->valid = FALSE;
}

static void ft_stroke_border_done(XCG_FT_StrokeBorder border)
{
	free(border->points);
	free(border->tags);

	border->num_points = 0;
	border->max_points = 0;
	border->start = -1;
	border->valid = FALSE;
}

static XCG_FT_Error ft_stroke_border_get_counts(XCG_FT_StrokeBorder border, XCG_FT_UInt* anum_points, XCG_FT_UInt* anum_contours)
{
	XCG_FT_Error error = 0;
	XCG_FT_UInt num_points = 0;
	XCG_FT_UInt num_contours = 0;
	XCG_FT_UInt count = border->num_points;
	XCG_FT_Vector* point = border->points;
	XCG_FT_Byte* tags = border->tags;
	XCG_FT_Int in_contour = 0;

	for (; count > 0; count--, num_points++, point++, tags++)
	{
		if (tags[0] & XCG_FT_STROKE_TAG_BEGIN)
		{
			if (in_contour != 0)
				goto Fail;
			in_contour = 1;
		}
		else if (in_contour == 0)
			goto Fail;
		if (tags[0] & XCG_FT_STROKE_TAG_END)
		{
			in_contour = 0;
			num_contours++;
		}
	}
	if (in_contour != 0)
		goto Fail;
	border->valid = TRUE;
Exit:
	*anum_points = num_points;
	*anum_contours = num_contours;
	return error;
Fail:
	num_points = 0;
	num_contours = 0;
	goto Exit;
}

static void ft_stroke_border_export(XCG_FT_StrokeBorder border, XCG_FT_Outline* outline)
{
	if (outline->points != NULL && border->points != NULL)
		memcpy(outline->points + outline->n_points, border->points, border->num_points * sizeof(XCG_FT_Vector));
	if (outline->tags)
	{
		XCG_FT_UInt count = border->num_points;
		XCG_FT_Byte* read = border->tags;
		XCG_FT_Byte* write = (XCG_FT_Byte*)outline->tags + outline->n_points;
		for (; count > 0; count--, read++, write++)
		{
			if (*read & XCG_FT_STROKE_TAG_ON)
				*write = XCG_FT_CURVE_TAG_ON;
			else if (*read & XCG_FT_STROKE_TAG_CUBIC)
				*write = XCG_FT_CURVE_TAG_CUBIC;
			else
				*write = XCG_FT_CURVE_TAG_CONIC;
		}
	}
	if (outline->contours)
	{
		XCG_FT_UInt count = border->num_points;
		XCG_FT_Byte* tags = border->tags;
		XCG_FT_Int* write = outline->contours + outline->n_contours;
		XCG_FT_Int idx = (XCG_FT_Int)outline->n_points;
		for (; count > 0; count--, tags++, idx++)
		{
			if (*tags & XCG_FT_STROKE_TAG_END)
			{
				*write++ = idx;
				outline->n_contours++;
			}
		}
	}
	outline->n_points = (int)(outline->n_points + border->num_points);
	XCG_FT_Outline_Check(outline);
}

#define XCG_FT_SIDE_TO_ROTATE(s)	(XCG_FT_ANGLE_PI2 - (s) * XCG_FT_ANGLE_PI)

typedef struct XCG_FT_StrokerRec_ {
	XCG_FT_Angle angle_in;
	XCG_FT_Angle angle_out;
	XCG_FT_Vector center;
	XCG_FT_Fixed line_length;
	XCG_FT_Bool first_point;
	XCG_FT_Bool subpath_open;
	XCG_FT_Angle subpath_angle;
	XCG_FT_Vector subpath_start;
	XCG_FT_Fixed subpath_line_length;
	XCG_FT_Bool handle_wide_strokes;
	XCG_FT_Stroker_LineCap line_cap;
	XCG_FT_Stroker_LineJoin line_join;
	XCG_FT_Stroker_LineJoin line_join_saved;
	XCG_FT_Fixed miter_limit;
	XCG_FT_Fixed radius;
	XCG_FT_StrokeBorderRec borders[2];
} XCG_FT_StrokerRec;

XCG_FT_Error XCG_FT_Stroker_New(XCG_FT_Stroker* astroker)
{
	XCG_FT_Error error = 0;
	XCG_FT_Stroker stroker = NULL;
	stroker = (XCG_FT_StrokerRec*)calloc(1, sizeof(XCG_FT_StrokerRec));
	if (stroker)
	{
		ft_stroke_border_init(&stroker->borders[0]);
		ft_stroke_border_init(&stroker->borders[1]);
	}
	*astroker = stroker;
	return error;
}

void XCG_FT_Stroker_Rewind(XCG_FT_Stroker stroker)
{
	if (stroker)
	{
		ft_stroke_border_reset(&stroker->borders[0]);
		ft_stroke_border_reset(&stroker->borders[1]);
	}
}

void XCG_FT_Stroker_Set(XCG_FT_Stroker stroker, XCG_FT_Fixed radius, XCG_FT_Stroker_LineCap line_cap, XCG_FT_Stroker_LineJoin line_join, XCG_FT_Fixed miter_limit)
{
	stroker->radius = radius;
	stroker->line_cap = line_cap;
	stroker->line_join = line_join;
	stroker->miter_limit = miter_limit;
	if (stroker->miter_limit < 0x10000)
		stroker->miter_limit = 0x10000;
	stroker->line_join_saved = line_join;
	XCG_FT_Stroker_Rewind(stroker);
}

void XCG_FT_Stroker_Done(XCG_FT_Stroker stroker)
{
	if (stroker)
	{
		ft_stroke_border_done(&stroker->borders[0]);
		ft_stroke_border_done(&stroker->borders[1]);
		free(stroker);
	}
}

static XCG_FT_Error ft_stroker_arcto(XCG_FT_Stroker stroker, XCG_FT_Int side)
{
	XCG_FT_Angle total, rotate;
	XCG_FT_Fixed radius = stroker->radius;
	XCG_FT_Error error = 0;
	XCG_FT_StrokeBorder border = stroker->borders + side;

	rotate = XCG_FT_SIDE_TO_ROTATE(side);
	total = XCG_FT_Angle_Diff(stroker->angle_in, stroker->angle_out);
	if (total == XCG_FT_ANGLE_PI)
		total = -rotate * 2;
	error = ft_stroke_border_arcto(border, &stroker->center, radius, stroker->angle_in + rotate, total);
	border->movable = FALSE;
	return error;
}

static XCG_FT_Error ft_stroker_cap(XCG_FT_Stroker stroker, XCG_FT_Angle angle, XCG_FT_Int side)
{
	XCG_FT_Error error = 0;

	if (stroker->line_cap == XCG_FT_STROKER_LINECAP_ROUND)
	{
		stroker->angle_in = angle;
		stroker->angle_out = angle + XCG_FT_ANGLE_PI;
		error = ft_stroker_arcto(stroker, side);
	}
	else
	{
		XCG_FT_Vector middle, delta;
		XCG_FT_Fixed radius = stroker->radius;
		XCG_FT_StrokeBorder border = stroker->borders + side;
		XCG_FT_Vector_From_Polar(&middle, radius, angle);
		delta.x = side ? middle.y : -middle.y;
		delta.y = side ? -middle.x : middle.x;
		if (stroker->line_cap == XCG_FT_STROKER_LINECAP_SQUARE)
		{
			middle.x += stroker->center.x;
			middle.y += stroker->center.y;
		}
		else
		{
			middle.x = stroker->center.x;
			middle.y = stroker->center.y;
		}
		delta.x += middle.x;
		delta.y += middle.y;
		error = ft_stroke_border_lineto(border, &delta, FALSE);
		if (error)
			goto Exit;
		delta.x = middle.x - delta.x + middle.x;
		delta.y = middle.y - delta.y + middle.y;
		error = ft_stroke_border_lineto(border, &delta, FALSE);
	}
Exit:
	return error;
}

static XCG_FT_Error ft_stroker_inside(XCG_FT_Stroker stroker, XCG_FT_Int side, XCG_FT_Fixed line_length)
{
	XCG_FT_StrokeBorder border = stroker->borders + side;
	XCG_FT_Angle phi, theta, rotate;
	XCG_FT_Fixed length;
	XCG_FT_Vector sigma = { 0, 0 };
	XCG_FT_Vector delta;
	XCG_FT_Error error = 0;
	XCG_FT_Bool intersect;

	rotate = XCG_FT_SIDE_TO_ROTATE(side);
	theta = XCG_FT_Angle_Diff(stroker->angle_in, stroker->angle_out) / 2;
	if (!border->movable || line_length == 0 || theta > 0x59C000 || theta < -0x59C000)
		intersect = FALSE;
	else
	{
		XCG_FT_Fixed min_length;
		XCG_FT_Vector_Unit(&sigma, theta);
		min_length = ft_pos_abs(XCG_FT_MulDiv(stroker->radius, sigma.y, sigma.x));
		intersect = XCG_FT_BOOL(min_length && stroker->line_length >= min_length && line_length >= min_length);
	}
	if (!intersect)
	{
		XCG_FT_Vector_From_Polar(&delta, stroker->radius, stroker->angle_out + rotate);
		delta.x += stroker->center.x;
		delta.y += stroker->center.y;
		border->movable = FALSE;
	}
	else
	{
		phi = stroker->angle_in + theta + rotate;
		length = XCG_FT_DivFix(stroker->radius, sigma.x);
		XCG_FT_Vector_From_Polar(&delta, length, phi);
		delta.x += stroker->center.x;
		delta.y += stroker->center.y;
	}
	error = ft_stroke_border_lineto(border, &delta, FALSE);
	return error;
}

static XCG_FT_Error ft_stroker_outside(XCG_FT_Stroker stroker, XCG_FT_Int side, XCG_FT_Fixed line_length)
{
	XCG_FT_StrokeBorder border = stroker->borders + side;
	XCG_FT_Error error;
	XCG_FT_Angle rotate;

	if (stroker->line_join == XCG_FT_STROKER_LINEJOIN_ROUND)
		error = ft_stroker_arcto(stroker, side);
	else
	{
		XCG_FT_Fixed radius = stroker->radius;
		XCG_FT_Vector sigma = { 0, 0 };
		XCG_FT_Angle theta = 0, phi = 0;
		XCG_FT_Bool bevel, fixed_bevel;

		rotate = XCG_FT_SIDE_TO_ROTATE(side);
		bevel = XCG_FT_BOOL(stroker->line_join == XCG_FT_STROKER_LINEJOIN_BEVEL);
		fixed_bevel = XCG_FT_BOOL(stroker->line_join != XCG_FT_STROKER_LINEJOIN_MITER_VARIABLE);

		if (!bevel)
		{
			theta = XCG_FT_Angle_Diff(stroker->angle_in, stroker->angle_out) / 2;
			if (theta == XCG_FT_ANGLE_PI2)
				theta = -rotate;
			phi = stroker->angle_in + theta + rotate;
			XCG_FT_Vector_From_Polar(&sigma, stroker->miter_limit, theta);
			if (sigma.x < 0x10000L)
			{
				if (fixed_bevel || ft_pos_abs(theta) > 57)
					bevel = TRUE;
			}
		}
		if (bevel)
		{
			if (fixed_bevel)
			{
				XCG_FT_Vector delta;
				XCG_FT_Vector_From_Polar(&delta, radius, stroker->angle_out + rotate);
				delta.x += stroker->center.x;
				delta.y += stroker->center.y;
				border->movable = FALSE;
				error = ft_stroke_border_lineto(border, &delta, FALSE);
			}
			else
			{
				XCG_FT_Vector middle, delta;
				XCG_FT_Fixed coef;

				XCG_FT_Vector_From_Polar(&middle, XCG_FT_MulFix(radius, stroker->miter_limit), phi);
				coef = XCG_FT_DivFix(0x10000L - sigma.x, sigma.y);
				delta.x = XCG_FT_MulFix(middle.y, coef);
				delta.y = XCG_FT_MulFix(-middle.x, coef);
				middle.x += stroker->center.x;
				middle.y += stroker->center.y;
				delta.x += middle.x;
				delta.y += middle.y;
				error = ft_stroke_border_lineto(border, &delta, FALSE);
				if (error)
					goto Exit;
				delta.x = middle.x - delta.x + middle.x;
				delta.y = middle.y - delta.y + middle.y;
				error = ft_stroke_border_lineto(border, &delta, FALSE);
				if (error)
					goto Exit;
				if (line_length == 0)
				{
					XCG_FT_Vector_From_Polar(&delta, radius, stroker->angle_out + rotate);
					delta.x += stroker->center.x;
					delta.y += stroker->center.y;
					error = ft_stroke_border_lineto(border, &delta, FALSE);
				}
			}
		}
		else
		{
			XCG_FT_Fixed length;
			XCG_FT_Vector delta;
			length = XCG_FT_MulDiv(stroker->radius, stroker->miter_limit, sigma.x);
			XCG_FT_Vector_From_Polar(&delta, length, phi);
			delta.x += stroker->center.x;
			delta.y += stroker->center.y;
			error = ft_stroke_border_lineto(border, &delta, FALSE);
			if (error)
				goto Exit;
			if (line_length == 0)
			{
				XCG_FT_Vector_From_Polar(&delta, stroker->radius, stroker->angle_out + rotate);
				delta.x += stroker->center.x;
				delta.y += stroker->center.y;
				error = ft_stroke_border_lineto(border, &delta, FALSE);
			}
		}
	}
Exit:
	return error;
}

static XCG_FT_Error ft_stroker_process_corner(XCG_FT_Stroker stroker, XCG_FT_Fixed line_length)
{
	XCG_FT_Error error = 0;
	XCG_FT_Angle turn;
	XCG_FT_Int inside_side;

	turn = XCG_FT_Angle_Diff(stroker->angle_in, stroker->angle_out);
	if (turn == 0)
		goto Exit;
	inside_side = 0;
	if (turn < 0)
		inside_side = 1;
	error = ft_stroker_inside(stroker, inside_side, line_length);
	if (error)
		goto Exit;
	error = ft_stroker_outside(stroker, 1 - inside_side, line_length);
Exit:
	return error;
}

static XCG_FT_Error ft_stroker_subpath_start(XCG_FT_Stroker stroker, XCG_FT_Angle start_angle, XCG_FT_Fixed line_length)
{
	XCG_FT_Vector delta;
	XCG_FT_Vector point;
	XCG_FT_Error error;
	XCG_FT_StrokeBorder border;

	XCG_FT_Vector_From_Polar(&delta, stroker->radius, start_angle + XCG_FT_ANGLE_PI2);
	point.x = stroker->center.x + delta.x;
	point.y = stroker->center.y + delta.y;
	border = stroker->borders;
	error = ft_stroke_border_moveto(border, &point);
	if (error)
		goto Exit;
	point.x = stroker->center.x - delta.x;
	point.y = stroker->center.y - delta.y;
	border++;
	error = ft_stroke_border_moveto(border, &point);
	stroker->subpath_angle = start_angle;
	stroker->first_point = FALSE;
	stroker->subpath_line_length = line_length;
Exit:
	return error;
}

XCG_FT_Error XCG_FT_Stroker_LineTo(XCG_FT_Stroker stroker, XCG_FT_Vector* to)
{
	XCG_FT_Error error = 0;
	XCG_FT_StrokeBorder border;
	XCG_FT_Vector delta;
	XCG_FT_Angle angle;
	XCG_FT_Int side;
	XCG_FT_Fixed line_length;

	delta.x = to->x - stroker->center.x;
	delta.y = to->y - stroker->center.y;
	if (delta.x == 0 && delta.y == 0)
		goto Exit;
	line_length = XCG_FT_Vector_Length(&delta);
	angle = XCG_FT_Atan2(delta.x, delta.y);
	XCG_FT_Vector_From_Polar(&delta, stroker->radius, angle + XCG_FT_ANGLE_PI2);
	if (stroker->first_point)
	{
		error = ft_stroker_subpath_start(stroker, angle, line_length);
		if (error)
			goto Exit;
	}
	else
	{
		stroker->angle_out = angle;
		error = ft_stroker_process_corner(stroker, line_length);
		if (error)
			goto Exit;
	}
	for (border = stroker->borders, side = 1; side >= 0; side--, border++)
	{
		XCG_FT_Vector point;
		point.x = to->x + delta.x;
		point.y = to->y + delta.y;
		error = ft_stroke_border_lineto(border, &point, TRUE);
		if (error)
			goto Exit;
		delta.x = -delta.x;
		delta.y = -delta.y;
	}
	stroker->angle_in = angle;
	stroker->center = *to;
	stroker->line_length = line_length;
Exit:
	return error;
}

XCG_FT_Error XCG_FT_Stroker_ConicTo(XCG_FT_Stroker stroker, XCG_FT_Vector* control, XCG_FT_Vector* to)
{
	XCG_FT_Error error = 0;
	XCG_FT_Vector bez_stack[34];
	XCG_FT_Vector* arc;
	XCG_FT_Vector* limit = bez_stack + 30;
	XCG_FT_Bool first_arc = TRUE;

	if (XCG_FT_IS_SMALL(stroker->center.x - control->x) &&
		XCG_FT_IS_SMALL(stroker->center.y - control->y) &&
		XCG_FT_IS_SMALL(control->x - to->x) &&
		XCG_FT_IS_SMALL(control->y - to->y))
	{
		stroker->center = *to;
		goto Exit;
	}
	arc = bez_stack;
	arc[0] = *to;
	arc[1] = *control;
	arc[2] = stroker->center;
	while (arc >= bez_stack)
	{
		XCG_FT_Angle angle_in, angle_out;
		angle_in = angle_out = stroker->angle_in;
		if (arc < limit && !ft_conic_is_small_enough(arc, &angle_in, &angle_out))
		{
			if (stroker->first_point)
				stroker->angle_in = angle_in;
			ft_conic_split(arc);
			arc += 2;
			continue;
		}
		if (first_arc)
		{
			first_arc = FALSE;
			if (stroker->first_point)
				error = ft_stroker_subpath_start(stroker, angle_in, 0);
			else
			{
				stroker->angle_out = angle_in;
				error = ft_stroker_process_corner(stroker, 0);
			}
		}
		else if (ft_pos_abs(XCG_FT_Angle_Diff(stroker->angle_in, angle_in)) > XCG_FT_SMALL_CONIC_THRESHOLD / 4)
		{
			stroker->center = arc[2];
			stroker->angle_out = angle_in;
			stroker->line_join = XCG_FT_STROKER_LINEJOIN_ROUND;
			error = ft_stroker_process_corner(stroker, 0);
			stroker->line_join = stroker->line_join_saved;
		}
		if (error)
			goto Exit;
		{
			XCG_FT_Vector ctrl, end;
			XCG_FT_Angle theta, phi, rotate, alpha0 = 0;
			XCG_FT_Fixed length;
			XCG_FT_StrokeBorder border;
			XCG_FT_Int side;

			theta = XCG_FT_Angle_Diff(angle_in, angle_out) / 2;
			phi = angle_in + theta;
			length = XCG_FT_DivFix(stroker->radius, XCG_FT_Cos(theta));
			if (stroker->handle_wide_strokes)
				alpha0 = XCG_FT_Atan2(arc[0].x - arc[2].x, arc[0].y - arc[2].y);
			for (border = stroker->borders, side = 0; side <= 1; side++, border++)
			{
				rotate = XCG_FT_SIDE_TO_ROTATE(side);
				XCG_FT_Vector_From_Polar(&ctrl, length, phi + rotate);
				ctrl.x += arc[1].x;
				ctrl.y += arc[1].y;
				XCG_FT_Vector_From_Polar(&end, stroker->radius, angle_out + rotate);
				end.x += arc[0].x;
				end.y += arc[0].y;

				if (stroker->handle_wide_strokes)
				{
					XCG_FT_Vector start;
					XCG_FT_Angle alpha1;

					start = border->points[border->num_points - 1];
					alpha1 = XCG_FT_Atan2(end.x - start.x, end.y - start.y);
					if (ft_pos_abs(XCG_FT_Angle_Diff(alpha0, alpha1)) >
						XCG_FT_ANGLE_PI / 2)
					{
						XCG_FT_Angle beta, gamma;
						XCG_FT_Vector bvec, delta;
						XCG_FT_Fixed blen, sinA, sinB, alen;

						beta = XCG_FT_Atan2(arc[2].x - start.x, arc[2].y - start.y);
						gamma = XCG_FT_Atan2(arc[0].x - end.x, arc[0].y - end.y);

						bvec.x = end.x - start.x;
						bvec.y = end.y - start.y;
						blen = XCG_FT_Vector_Length(&bvec);
						sinA = ft_pos_abs(XCG_FT_Sin(alpha1 - gamma));
						sinB = ft_pos_abs(XCG_FT_Sin(beta - gamma));
						alen = XCG_FT_MulDiv(blen, sinA, sinB);
						XCG_FT_Vector_From_Polar(&delta, alen, beta);
						delta.x += start.x;
						delta.y += start.y;
						border->movable = FALSE;
						error = ft_stroke_border_lineto(border, &delta, FALSE);
						if (error)
							goto Exit;
						error = ft_stroke_border_lineto(border, &end, FALSE);
						if (error)
							goto Exit;
						error = ft_stroke_border_conicto(border, &ctrl, &start);
						if (error)
							goto Exit;
						error = ft_stroke_border_lineto(border, &end, FALSE);
						if (error)
							goto Exit;
						continue;
					}
				}
				error = ft_stroke_border_conicto(border, &ctrl, &end);
				if (error)
					goto Exit;
			}
		}
		arc -= 2;
		stroker->angle_in = angle_out;
	}
	stroker->center = *to;
	stroker->line_length = 0;
Exit:
	return error;
}

XCG_FT_Error XCG_FT_Stroker_CubicTo(XCG_FT_Stroker stroker, XCG_FT_Vector* control1, XCG_FT_Vector* control2, XCG_FT_Vector* to)
{
	XCG_FT_Error error = 0;
	XCG_FT_Vector bez_stack[37];
	XCG_FT_Vector* arc;
	XCG_FT_Vector* limit = bez_stack + 32;
	XCG_FT_Bool first_arc = TRUE;

	if (XCG_FT_IS_SMALL(stroker->center.x - control1->x) &&
		XCG_FT_IS_SMALL(stroker->center.y - control1->y) &&
		XCG_FT_IS_SMALL(control1->x - control2->x) &&
		XCG_FT_IS_SMALL(control1->y - control2->y) &&
		XCG_FT_IS_SMALL(control2->x - to->x) &&
		XCG_FT_IS_SMALL(control2->y - to->y))
	{
		stroker->center = *to;
		goto Exit;
	}
	arc = bez_stack;
	arc[0] = *to;
	arc[1] = *control2;
	arc[2] = *control1;
	arc[3] = stroker->center;
	while (arc >= bez_stack)
	{
		XCG_FT_Angle angle_in, angle_mid, angle_out;
		angle_in = angle_out = angle_mid = stroker->angle_in;
		if (arc < limit && !ft_cubic_is_small_enough(arc, &angle_in, &angle_mid, &angle_out))
		{
			if (stroker->first_point)
				stroker->angle_in = angle_in;
			ft_cubic_split(arc);
			arc += 3;
			continue;
		}
		if (first_arc)
		{
			first_arc = FALSE;
			if (stroker->first_point)
				error = ft_stroker_subpath_start(stroker, angle_in, 0);
			else
			{
				stroker->angle_out = angle_in;
				error = ft_stroker_process_corner(stroker, 0);
			}
		}
		else if (ft_pos_abs(XCG_FT_Angle_Diff(stroker->angle_in, angle_in)) > XCG_FT_SMALL_CUBIC_THRESHOLD / 4)
		{
			stroker->center = arc[3];
			stroker->angle_out = angle_in;
			stroker->line_join = XCG_FT_STROKER_LINEJOIN_ROUND;
			error = ft_stroker_process_corner(stroker, 0);
			stroker->line_join = stroker->line_join_saved;
		}
		if (error)
			goto Exit;
		{
			XCG_FT_Vector ctrl1, ctrl2, end;
			XCG_FT_Angle theta1, phi1, theta2, phi2, rotate, alpha0 = 0;
			XCG_FT_Fixed length1, length2;
			XCG_FT_StrokeBorder border;
			XCG_FT_Int side;

			theta1 = XCG_FT_Angle_Diff(angle_in, angle_mid) / 2;
			theta2 = XCG_FT_Angle_Diff(angle_mid, angle_out) / 2;
			phi1 = ft_angle_mean(angle_in, angle_mid);
			phi2 = ft_angle_mean(angle_mid, angle_out);
			length1 = XCG_FT_DivFix(stroker->radius, XCG_FT_Cos(theta1));
			length2 = XCG_FT_DivFix(stroker->radius, XCG_FT_Cos(theta2));
			if (stroker->handle_wide_strokes)
				alpha0 = XCG_FT_Atan2(arc[0].x - arc[3].x, arc[0].y - arc[3].y);
			for (border = stroker->borders, side = 0; side <= 1; side++, border++)
			{
				rotate = XCG_FT_SIDE_TO_ROTATE(side);
				XCG_FT_Vector_From_Polar(&ctrl1, length1, phi1 + rotate);
				ctrl1.x += arc[2].x;
				ctrl1.y += arc[2].y;
				XCG_FT_Vector_From_Polar(&ctrl2, length2, phi2 + rotate);
				ctrl2.x += arc[1].x;
				ctrl2.y += arc[1].y;
				XCG_FT_Vector_From_Polar(&end, stroker->radius, angle_out + rotate);
				end.x += arc[0].x;
				end.y += arc[0].y;
				if (stroker->handle_wide_strokes)
				{
					XCG_FT_Vector start;
					XCG_FT_Angle alpha1;
					start = border->points[border->num_points - 1];
					alpha1 = XCG_FT_Atan2(end.x - start.x, end.y - start.y);
					if (ft_pos_abs(XCG_FT_Angle_Diff(alpha0, alpha1)) >
						XCG_FT_ANGLE_PI / 2)
					{
						XCG_FT_Angle beta, gamma;
						XCG_FT_Vector bvec, delta;
						XCG_FT_Fixed blen, sinA, sinB, alen;
						beta = XCG_FT_Atan2(arc[3].x - start.x, arc[3].y - start.y);
						gamma = XCG_FT_Atan2(arc[0].x - end.x, arc[0].y - end.y);
						bvec.x = end.x - start.x;
						bvec.y = end.y - start.y;
						blen = XCG_FT_Vector_Length(&bvec);
						sinA = ft_pos_abs(XCG_FT_Sin(alpha1 - gamma));
						sinB = ft_pos_abs(XCG_FT_Sin(beta - gamma));
						alen = XCG_FT_MulDiv(blen, sinA, sinB);
						XCG_FT_Vector_From_Polar(&delta, alen, beta);
						delta.x += start.x;
						delta.y += start.y;
						border->movable = FALSE;
						error = ft_stroke_border_lineto(border, &delta, FALSE);
						if (error)
							goto Exit;
						error = ft_stroke_border_lineto(border, &end, FALSE);
						if (error)
							goto Exit;
						error = ft_stroke_border_cubicto(border, &ctrl2, &ctrl1, &start);
						if (error)
							goto Exit;
						error = ft_stroke_border_lineto(border, &end, FALSE);
						if (error)
							goto Exit;
						continue;
					}
				}
				error = ft_stroke_border_cubicto(border, &ctrl1, &ctrl2, &end);
				if (error)
					goto Exit;
			}
		}
		arc -= 3;
		stroker->angle_in = angle_out;
	}
	stroker->center = *to;
	stroker->line_length = 0;
Exit:
	return error;
}

XCG_FT_Error XCG_FT_Stroker_BeginSubPath(XCG_FT_Stroker stroker, XCG_FT_Vector* to, XCG_FT_Bool open)
{
	stroker->first_point = TRUE;
	stroker->center = *to;
	stroker->subpath_open = open;
	stroker->handle_wide_strokes = XCG_FT_BOOL(stroker->line_join != XCG_FT_STROKER_LINEJOIN_ROUND || (stroker->subpath_open && stroker->line_cap == XCG_FT_STROKER_LINECAP_BUTT));
	stroker->subpath_start = *to;
	stroker->angle_in = 0;
	return 0;
}

static XCG_FT_Error ft_stroker_add_reverse_left(XCG_FT_Stroker stroker, XCG_FT_Bool open)
{
	XCG_FT_StrokeBorder right = stroker->borders + 0;
	XCG_FT_StrokeBorder left = stroker->borders + 1;
	XCG_FT_Int new_points;
	XCG_FT_Error error = 0;

	new_points = left->num_points - left->start;
	if (new_points > 0)
	{
		error = ft_stroke_border_grow(right, (XCG_FT_UInt)new_points);
		if (error)
			goto Exit;
		{
			XCG_FT_Vector* dst_point = right->points + right->num_points;
			XCG_FT_Byte* dst_tag = right->tags + right->num_points;
			XCG_FT_Vector* src_point = left->points + left->num_points - 1;
			XCG_FT_Byte* src_tag = left->tags + left->num_points - 1;
			while (src_point >= left->points + left->start)
			{
				*dst_point = *src_point;
				*dst_tag = *src_tag;
				if (open)
					dst_tag[0] &= ~XCG_FT_STROKE_TAG_BEGIN_END;
				else
				{
					XCG_FT_Byte ttag = (XCG_FT_Byte)(dst_tag[0] & XCG_FT_STROKE_TAG_BEGIN_END);
					if (ttag == XCG_FT_STROKE_TAG_BEGIN || ttag == XCG_FT_STROKE_TAG_END)
						dst_tag[0] ^= XCG_FT_STROKE_TAG_BEGIN_END;
				}
				src_point--;
				src_tag--;
				dst_point++;
				dst_tag++;
			}
		}
		left->num_points = left->start;
		right->num_points += new_points;
		right->movable = FALSE;
		left->movable = FALSE;
	}
Exit:
	return error;
}

XCG_FT_Error XCG_FT_Stroker_EndSubPath(XCG_FT_Stroker stroker)
{
	XCG_FT_Error error = 0;

	if (stroker->subpath_open)
	{
		XCG_FT_StrokeBorder right = stroker->borders;
		error = ft_stroker_cap(stroker, stroker->angle_in, 0);
		if (error)
			goto Exit;
		error = ft_stroker_add_reverse_left(stroker, TRUE);
		if (error)
			goto Exit;
		stroker->center = stroker->subpath_start;
		error = ft_stroker_cap(stroker, stroker->subpath_angle + XCG_FT_ANGLE_PI, 0);
		if (error)
			goto Exit;
		ft_stroke_border_close(right, FALSE);
	}
	else
	{
		XCG_FT_Angle turn;
		XCG_FT_Int inside_side;
		if (stroker->center.x != stroker->subpath_start.x || stroker->center.y != stroker->subpath_start.y)
		{
			error = XCG_FT_Stroker_LineTo(stroker, &stroker->subpath_start);
			if (error)
				goto Exit;
		}
		stroker->angle_out = stroker->subpath_angle;
		turn = XCG_FT_Angle_Diff(stroker->angle_in, stroker->angle_out);
		if (turn != 0)
		{
			inside_side = 0;
			if (turn < 0)
				inside_side = 1;
			error = ft_stroker_inside(stroker, inside_side, stroker->subpath_line_length);
			if (error)
				goto Exit;
			error = ft_stroker_outside(stroker, 1 - inside_side, stroker->subpath_line_length);
			if (error)
				goto Exit;
		}
		ft_stroke_border_close(stroker->borders + 0, FALSE);
		ft_stroke_border_close(stroker->borders + 1, TRUE);
	}
Exit:
	return error;
}

XCG_FT_Error XCG_FT_Stroker_GetBorderCounts(XCG_FT_Stroker stroker, XCG_FT_StrokerBorder border, XCG_FT_UInt* anum_points, XCG_FT_UInt* anum_contours)
{
	XCG_FT_UInt num_points = 0, num_contours = 0;
	XCG_FT_Error error;

	if (!stroker || border > 1)
	{
		error = -1;
		goto Exit;
	}
	error = ft_stroke_border_get_counts(stroker->borders + border, &num_points, &num_contours);
Exit:
	if (anum_points)
		*anum_points = num_points;
	if (anum_contours)
		*anum_contours = num_contours;
	return error;
}

XCG_FT_Error XCG_FT_Stroker_GetCounts(XCG_FT_Stroker stroker, XCG_FT_UInt* anum_points, XCG_FT_UInt* anum_contours)
{
	XCG_FT_UInt count1, count2, num_points = 0;
	XCG_FT_UInt count3, count4, num_contours = 0;
	XCG_FT_Error error;

	error = ft_stroke_border_get_counts(stroker->borders + 0, &count1, &count2);
	if (error)
		goto Exit;
	error = ft_stroke_border_get_counts(stroker->borders + 1, &count3, &count4);
	if (error)
		goto Exit;
	num_points = count1 + count3;
	num_contours = count2 + count4;
Exit:
	*anum_points = num_points;
	*anum_contours = num_contours;
	return error;
}

void XCG_FT_Stroker_ExportBorder(XCG_FT_Stroker stroker, XCG_FT_StrokerBorder border, XCG_FT_Outline* outline)
{
	if (border == XCG_FT_STROKER_BORDER_LEFT || border == XCG_FT_STROKER_BORDER_RIGHT)
	{
		XCG_FT_StrokeBorder sborder = &stroker->borders[border];
		if (sborder->valid)
			ft_stroke_border_export(sborder, outline);
	}
}

void XCG_FT_Stroker_Export(XCG_FT_Stroker stroker, XCG_FT_Outline* outline)
{
	XCG_FT_Stroker_ExportBorder(stroker, XCG_FT_STROKER_BORDER_LEFT, outline);
	XCG_FT_Stroker_ExportBorder(stroker, XCG_FT_STROKER_BORDER_RIGHT, outline);
}

XCG_FT_Error XCG_FT_Stroker_ParseOutline(XCG_FT_Stroker stroker, const XCG_FT_Outline* outline)
{
	XCG_FT_Vector v_last;
	XCG_FT_Vector v_control;
	XCG_FT_Vector v_start;
	XCG_FT_Vector* point;
	XCG_FT_Vector* limit;
	char* tags;
	XCG_FT_Error error;
	XCG_FT_Int n;
	XCG_FT_UInt first;
	XCG_FT_Int tag;

	if (!outline || !stroker)
		return -1;
	XCG_FT_Stroker_Rewind(stroker);
	first = 0;
	for (n = 0; n < outline->n_contours; n++)
	{
		XCG_FT_UInt last;
		last = outline->contours[n];
		limit = outline->points + last;
		if (last <= first)
		{
			first = last + 1;
			continue;
		}
		v_start = outline->points[first];
		v_last = outline->points[last];
		v_control = v_start;
		point = outline->points + first;
		tags = outline->tags + first;
		tag = XCG_FT_CURVE_TAG(tags[0]);
		if (tag == XCG_FT_CURVE_TAG_CUBIC)
			goto Invalid_Outline;
		if (tag == XCG_FT_CURVE_TAG_CONIC)
		{
			if (XCG_FT_CURVE_TAG(outline->tags[last]) == XCG_FT_CURVE_TAG_ON)
			{
				v_start = v_last;
				limit--;
			}
			else
			{
				v_start.x = (v_start.x + v_last.x) / 2;
				v_start.y = (v_start.y + v_last.y) / 2;
			}
			point--;
			tags--;
		}
		error = XCG_FT_Stroker_BeginSubPath(stroker, &v_start, outline->contours_flag[n]);
		if (error)
			goto Exit;
		while (point < limit)
		{
			point++;
			tags++;
			tag = XCG_FT_CURVE_TAG(tags[0]);
			switch (tag)
			{
			case XCG_FT_CURVE_TAG_ON:
			{
				XCG_FT_Vector vec;
				vec.x = point->x;
				vec.y = point->y;
				error = XCG_FT_Stroker_LineTo(stroker, &vec);
				if (error)
					goto Exit;
				continue;
			}

			case XCG_FT_CURVE_TAG_CONIC:
				v_control.x = point->x;
				v_control.y = point->y;
			Do_Conic:
				if (point < limit)
				{
					XCG_FT_Vector vec;
					XCG_FT_Vector v_middle;
					point++;
					tags++;
					tag = XCG_FT_CURVE_TAG(tags[0]);
					vec = point[0];
					if (tag == XCG_FT_CURVE_TAG_ON)
					{
						error = XCG_FT_Stroker_ConicTo(stroker, &v_control, &vec);
						if (error)
							goto Exit;
						continue;
					}
					if (tag != XCG_FT_CURVE_TAG_CONIC)
						goto Invalid_Outline;
					v_middle.x = (v_control.x + vec.x) / 2;
					v_middle.y = (v_control.y + vec.y) / 2;
					error = XCG_FT_Stroker_ConicTo(stroker, &v_control, &v_middle);
					if (error)
						goto Exit;
					v_control = vec;
					goto Do_Conic;
				}
				error = XCG_FT_Stroker_ConicTo(stroker, &v_control, &v_start);
				goto Close;

			default:
			{
				XCG_FT_Vector vec1, vec2;
				if (point + 1 > limit ||
					XCG_FT_CURVE_TAG(tags[1]) != XCG_FT_CURVE_TAG_CUBIC)
					goto Invalid_Outline;
				point += 2;
				tags += 2;
				vec1 = point[-2];
				vec2 = point[-1];
				if (point <= limit)
				{
					XCG_FT_Vector vec;
					vec = point[0];
					error = XCG_FT_Stroker_CubicTo(stroker, &vec1, &vec2, &vec);
					if (error)
						goto Exit;
					continue;
				}
				error = XCG_FT_Stroker_CubicTo(stroker, &vec1, &vec2, &v_start);
				goto Close;
			}
			}
		}
	Close:
		if (error)
			goto Exit;
		if (stroker->first_point)
		{
			stroker->subpath_open = TRUE;
			error = ft_stroker_subpath_start(stroker, 0, 0);
			if (error)
				goto Exit;
		}
		error = XCG_FT_Stroker_EndSubPath(stroker);
		if (error)
			goto Exit;
		first = last + 1;
	}
	return 0;
Exit:
	return error;
Invalid_Outline:
	return -2;
}

#define cg_array_init(array) \
	do { \
		array.data = NULL; \
		array.size = 0; \
		array.capacity = 0; \
	} while(0)

template<typename T>
void cg_array_ensure(T& array, size_t count) {
	do {
		if (array.size + count > array.capacity) {
			int capacity = array.size + count;
			int newcapacity = (array.capacity == 0) ? 8 : array.capacity;
			while (newcapacity < capacity) { newcapacity <<= 1; }
			array.data = (decltype(array.data))realloc(array.data, (size_t)newcapacity * sizeof(array.data[0]));
			array.capacity = newcapacity;
		}
	} while (0);
}

static inline void cg_color_init_rgba(struct cg_color_t* color, double r, double g, double b, double a)
{
	color->r = r;
	color->g = g;
	color->b = b;
	color->a = a;
}

static inline void cg_rect_init(struct cg_rect_t* rect, double x, double y, double w, double h)
{
	rect->x = x;
	rect->y = y;
	rect->w = w;
	rect->h = h;
}

void cg_matrix_init(struct cg_matrix_t* m, double a, double b, double c, double d, double tx, double ty)
{
	m->a = a;   m->b = b;
	m->c = c;   m->d = d;
	m->tx = tx; m->ty = ty;
}

void cg_matrix_init_identity(struct cg_matrix_t* m)
{
	m->a = 1;  m->b = 0;
	m->c = 0;  m->d = 1;
	m->tx = 0; m->ty = 0;
}

void cg_matrix_init_translate(struct cg_matrix_t* m, double tx, double ty)
{
	m->a = 1;   m->b = 0;
	m->c = 0;   m->d = 1;
	m->tx = tx; m->ty = ty;
}

void cg_matrix_init_scale(struct cg_matrix_t* m, double sx, double sy)
{
	m->a = sx; m->b = 0;
	m->c = 0;  m->d = sy;
	m->tx = 0; m->ty = 0;
}

void cg_matrix_init_rotate(struct cg_matrix_t* m, double r)
{
	double s = sin(r);
	double c = cos(r);

	m->a = c;   m->b = s;
	m->c = -s;  m->d = c;
	m->tx = 0;  m->ty = 0;
}

void cg_matrix_translate(struct cg_matrix_t* m, double tx, double ty)
{
	m->tx += m->a * tx + m->c * ty;
	m->ty += m->b * tx + m->d * ty;
}

void cg_matrix_scale(struct cg_matrix_t* m, double sx, double sy)
{
	m->a *= sx;
	m->b *= sx;
	m->c *= sy;
	m->d *= sy;
}

void cg_matrix_rotate(struct cg_matrix_t* m, double r)
{
	double s = sin(r);
	double c = cos(r);
	double ca = c * m->a;
	double cb = c * m->b;
	double cc = c * m->c;
	double cd = c * m->d;
	double sa = s * m->a;
	double sb = s * m->b;
	double sc = s * m->c;
	double sd = s * m->d;

	m->a = ca + sc;
	m->b = cb + sd;
	m->c = cc - sa;
	m->d = cd - sb;
}

void cg_matrix_multiply(struct cg_matrix_t* m, struct cg_matrix_t* m1, struct cg_matrix_t* m2)
{
	struct cg_matrix_t t;

	t.a = m1->a * m2->a;
	t.b = 0.0;
	t.c = 0.0;
	t.d = m1->d * m2->d;
	t.tx = m1->tx * m2->a + m2->tx;
	t.ty = m1->ty * m2->d + m2->ty;
	if (m1->b != 0.0 || m1->c != 0.0 || m2->b != 0.0 || m2->c != 0.0)
	{
		t.a += m1->b * m2->c;
		t.b += m1->a * m2->b + m1->b * m2->d;
		t.c += m1->c * m2->a + m1->d * m2->c;
		t.d += m1->c * m2->b;
		t.tx += m1->ty * m2->c;
		t.ty += m1->tx * m2->b;
	}
	memcpy(m, &t, sizeof(struct cg_matrix_t));
}

void cg_matrix_invert(struct cg_matrix_t* m)
{
	double a, b, c, d, tx, ty;
	double det;

	if ((m->c == 0.0) && (m->b == 0.0))
	{
		m->tx = -m->tx;
		m->ty = -m->ty;
		if (m->a != 1.0)
		{
			if (m->a == 0.0)
				return;
			m->a = 1.0 / m->a;
			m->tx *= m->a;
		}
		if (m->d != 1.0)
		{
			if (m->d == 0.0)
				return;
			m->d = 1.0 / m->d;
			m->ty *= m->d;
		}
	}
	else
	{
		det = m->a * m->d - m->b * m->c;
		if (det != 0.0)
		{
			a = m->a;
			b = m->b;
			c = m->c;
			d = m->d;
			tx = m->tx;
			ty = m->ty;
			m->a = d / det;
			m->b = -b / det;
			m->c = -c / det;
			m->d = a / det;
			m->tx = (c * ty - d * tx) / det;
			m->ty = (b * tx - a * ty) / det;
		}
	}
}

void cg_matrix_map_point(struct cg_matrix_t* m, struct cg_point_t* p1, struct cg_point_t* p2)
{
	p2->x = p1->x * m->a + p1->y * m->c + m->tx;
	p2->y = p1->x * m->b + p1->y * m->d + m->ty;
}

struct cg_surface_t* cg_surface_create(int width, int height)
{
	struct cg_surface_t* surface = (cg_surface_t*)malloc(sizeof(struct cg_surface_t));
	surface->ref = 1;
	surface->width = width;
	surface->height = height;
	surface->stride = width << 2;
	surface->owndata = 1;
	surface->pixels = (uint8_t*)calloc(1, (size_t)(height * surface->stride));
	return surface;
}

struct cg_surface_t* cg_surface_create_for_data(int width, int height, void* pixels)
{
	struct cg_surface_t* surface = (cg_surface_t*)malloc(sizeof(struct cg_surface_t));
	surface->ref = 1;
	surface->width = width;
	surface->height = height;
	surface->stride = width << 2;
	surface->owndata = 0;
	surface->pixels = (uint8_t*)pixels;
	return surface;
}

void cg_surface_destroy(struct cg_surface_t* surface)
{
	if (surface)
	{
		if (--surface->ref == 0)
		{
			if (surface->owndata)
				free(surface->pixels);
			free(surface);
		}
	}
}

struct cg_surface_t* cg_surface_reference(struct cg_surface_t* surface)
{
	if (surface)
	{
		++surface->ref;
		return surface;
	}
	return NULL;
}

static struct cg_path_t* cg_path_create(void)
{
	struct cg_path_t* path = (cg_path_t*)malloc(sizeof(struct cg_path_t));
	path->contours = 0;
	path->start.x = 0.0;
	path->start.y = 0.0;
	cg_array_init(path->elements);
	cg_array_init(path->points);
	return path;
}

static void cg_path_destroy(struct cg_path_t* path)
{
	if (path)
	{
		if (path->elements.data)
			free(path->elements.data);
		if (path->points.data)
			free(path->points.data);
		free(path);
	}
}

static inline void cg_path_get_current_point(struct cg_path_t* path, double* x, double* y)
{
	if (path->points.size == 0)
	{
		*x = 0.0;
		*y = 0.0;
	}
	else
	{
		*x = path->points.data[path->points.size - 1].x;
		*y = path->points.data[path->points.size - 1].y;
	}
}

static void cg_path_move_to(struct cg_path_t* path, double x, double y)
{
	cg_array_ensure(path->elements, 1);
	cg_array_ensure(path->points, 1);

	path->elements.data[path->elements.size] = CG_PATH_ELEMENT_MOVE_TO;
	path->elements.size += 1;
	path->contours += 1;
	path->points.data[path->points.size].x = x;
	path->points.data[path->points.size].y = y;
	path->points.size += 1;
	path->start.x = x;
	path->start.y = y;
}

static void cg_path_line_to(struct cg_path_t* path, double x, double y)
{
	cg_array_ensure(path->elements, 1);
	cg_array_ensure(path->points, 1);

	path->elements.data[path->elements.size] = CG_PATH_ELEMENT_LINE_TO;
	path->elements.size += 1;
	path->points.data[path->points.size].x = x;
	path->points.data[path->points.size].y = y;
	path->points.size += 1;
}

static void cg_path_curve_to(struct cg_path_t* path, double x1, double y1, double x2, double y2, double x3, double y3)
{
	cg_array_ensure(path->elements, 1);
	cg_array_ensure(path->points, 3);

	path->elements.data[path->elements.size] = CG_PATH_ELEMENT_CURVE_TO;
	path->elements.size += 1;
	struct cg_point_t* points = path->points.data + path->points.size;
	points[0].x = x1;
	points[0].y = y1;
	points[1].x = x2;
	points[1].y = y2;
	points[2].x = x3;
	points[2].y = y3;
	path->points.size += 3;
}

static void cg_path_quad_to(struct cg_path_t* path, double x1, double y1, double x2, double y2)
{
	double x, y;
	cg_path_get_current_point(path, &x, &y);

	double cx = 2.0 / 3.0 * x1 + 1.0 / 3.0 * x;
	double cy = 2.0 / 3.0 * y1 + 1.0 / 3.0 * y;
	double cx1 = 2.0 / 3.0 * x1 + 1.0 / 3.0 * x2;
	double cy1 = 2.0 / 3.0 * y1 + 1.0 / 3.0 * y2;
	cg_path_curve_to(path, cx, cy, cx1, cy1, x2, y2);
}

static void cg_path_close(struct cg_path_t* path)
{
	if (path->elements.size == 0)
		return;
	if (path->elements.data[path->elements.size - 1] == CG_PATH_ELEMENT_CLOSE)
		return;
	cg_array_ensure(path->elements, 1);
	cg_array_ensure(path->points, 1);
	path->elements.data[path->elements.size] = CG_PATH_ELEMENT_CLOSE;
	path->elements.size += 1;
	path->points.data[path->points.size].x = path->start.x;
	path->points.data[path->points.size].y = path->start.y;
	path->points.size += 1;
}

static void cg_path_rel_move_to(struct cg_path_t* path, double dx, double dy)
{
	double x, y;
	cg_path_get_current_point(path, &x, &y);
	cg_path_move_to(path, dx + x, dy + y);
}

static void cg_path_rel_line_to(struct cg_path_t* path, double dx, double dy)
{
	double x, y;
	cg_path_get_current_point(path, &x, &y);
	cg_path_line_to(path, dx + x, dy + y);
}

static void cg_path_rel_curve_to(struct cg_path_t* path, double dx1, double dy1, double dx2, double dy2, double dx3, double dy3)
{
	double x, y;
	cg_path_get_current_point(path, &x, &y);
	cg_path_curve_to(path, dx1 + x, dy1 + y, dx2 + x, dy2 + y, dx3 + x, dy3 + y);
}

static void cg_path_rel_quad_to(struct cg_path_t* path, double dx1, double dy1, double dx2, double dy2)
{
	double x, y;
	cg_path_get_current_point(path, &x, &y);
	cg_path_quad_to(path, dx1 + x, dy1 + y, dx2 + x, dy2 + y);
}

static inline void cg_path_add_rectangle(struct cg_path_t* path, double x, double y, double w, double h)
{
	cg_path_move_to(path, x, y);
	cg_path_line_to(path, x + w, y);
	cg_path_line_to(path, x + w, y + h);
	cg_path_line_to(path, x, y + h);
	cg_path_line_to(path, x, y);
	cg_path_close(path);
}

static inline void cg_path_add_round_rectangle(struct cg_path_t* path, double x, double y, double w, double h, double rx, double ry)
{
	rx = std::min(rx, w * 0.5);
	ry = std::min(ry, h * 0.5);

	double right = x + w;
	double bottom = y + h;
	double cpx = rx * 0.55228474983079339840;
	double cpy = ry * 0.55228474983079339840;

	cg_path_move_to(path, x, y + ry);
	cg_path_curve_to(path, x, y + ry - cpy, x + rx - cpx, y, x + rx, y);
	cg_path_line_to(path, right - rx, y);
	cg_path_curve_to(path, right - rx + cpx, y, right, y + ry - cpy, right, y + ry);
	cg_path_line_to(path, right, bottom - ry);
	cg_path_curve_to(path, right, bottom - ry + cpy, right - rx + cpx, bottom, right - rx, bottom);
	cg_path_line_to(path, x + rx, bottom);
	cg_path_curve_to(path, x + rx - cpx, bottom, x, bottom - ry + cpy, x, bottom - ry);
	cg_path_line_to(path, x, y + ry);
	cg_path_close(path);
}

static void cg_path_add_ellipse(struct cg_path_t* path, double cx, double cy, double rx, double ry)
{
	double left = cx - rx;
	double top = cy - ry;
	double right = cx + rx;
	double bottom = cy + ry;
	double cpx = rx * 0.55228474983079339840;
	double cpy = ry * 0.55228474983079339840;

	cg_path_move_to(path, cx, top);
	cg_path_curve_to(path, cx + cpx, top, right, cy - cpy, right, cy);
	cg_path_curve_to(path, right, cy + cpy, cx + cpx, bottom, cx, bottom);
	cg_path_curve_to(path, cx - cpx, bottom, left, cy + cpy, left, cy);
	cg_path_curve_to(path, left, cy - cpy, cx - cpx, top, cx, top);
	cg_path_close(path);
}

static void cg_path_add_arc(struct cg_path_t* path, double cx, double cy, double r, double a0, double a1, int ccw)
{
	double da = a1 - a0;
	if (fabs(da) > 6.28318530717958647693)
	{
		da = 6.28318530717958647693;
	}
	else if (da != 0.0 && ccw != (da < 0.0))
	{
		da += 6.28318530717958647693 * (ccw ? -1 : 1);
	}
	int seg_n = (int)(ceil(fabs(da) / 1.57079632679489661923));
	double seg_a = da / seg_n;
	double d = (seg_a / 1.57079632679489661923) * 0.55228474983079339840 * r;
	double a = a0;
	double ax = cx + cos(a) * r;
	double ay = cy + sin(a) * r;
	double dx = -sin(a) * d;
	double dy = cos(a) * d;
	if (path->points.size == 0)
		cg_path_move_to(path, ax, ay);
	else
		cg_path_line_to(path, ax, ay);
	for (int i = 0; i < seg_n; i++)
	{
		double cp1x = ax + dx;
		double cp1y = ay + dy;
		a += seg_a;
		ax = cx + cos(a) * r;
		ay = cy + sin(a) * r;
		dx = -sin(a) * d;
		dy = cos(a) * d;
		double cp2x = ax - dx;
		double cp2y = ay - dy;
		cg_path_curve_to(path, cp1x, cp1y, cp2x, cp2y, ax, ay);
	}
}

static inline void cg_path_clear(struct cg_path_t* path)
{
	path->elements.size = 0;
	path->points.size = 0;
	path->contours = 0;
	path->start.x = 0.0;
	path->start.y = 0.0;
}

struct cg_bezier_t {
	double x1; double y1;
	double x2; double y2;
	double x3; double y3;
	double x4; double y4;
};

static inline void split(struct cg_bezier_t* b, struct cg_bezier_t* first, struct cg_bezier_t* second)
{
	double c = (b->x2 + b->x3) * 0.5;
	first->x2 = (b->x1 + b->x2) * 0.5;
	second->x3 = (b->x3 + b->x4) * 0.5;
	first->x1 = b->x1;
	second->x4 = b->x4;
	first->x3 = (first->x2 + c) * 0.5;
	second->x2 = (second->x3 + c) * 0.5;
	first->x4 = second->x1 = (first->x3 + second->x2) * 0.5;

	c = (b->y2 + b->y3) * 0.5;
	first->y2 = (b->y1 + b->y2) * 0.5;
	second->y3 = (b->y3 + b->y4) * 0.5;
	first->y1 = b->y1;
	second->y4 = b->y4;
	first->y3 = (first->y2 + c) * 0.5;
	second->y2 = (second->y3 + c) * 0.5;
	first->y4 = second->y1 = (first->y3 + second->y2) * 0.5;
}

static inline void flatten(struct cg_path_t* path, struct cg_point_t* p0, struct cg_point_t* p1, struct cg_point_t* p2, struct cg_point_t* p3)
{
	struct cg_bezier_t beziers[32];
	struct cg_bezier_t* b = beziers;

	beziers[0].x1 = p0->x;
	beziers[0].y1 = p0->y;
	beziers[0].x2 = p1->x;
	beziers[0].y2 = p1->y;
	beziers[0].x3 = p2->x;
	beziers[0].y3 = p2->y;
	beziers[0].x4 = p3->x;
	beziers[0].y4 = p3->y;
	while (b >= beziers)
	{
		double y4y1 = b->y4 - b->y1;
		double x4x1 = b->x4 - b->x1;
		double l = fabs(x4x1) + fabs(y4y1);
		double d;
		if (l > 1.0)
		{
			d = fabs((x4x1) * (b->y1 - b->y2) - (y4y1) * (b->x1 - b->x2)) + fabs((x4x1) * (b->y1 - b->y3) - (y4y1) * (b->x1 - b->x3));
		}
		else
		{
			d = fabs(b->x1 - b->x2) + fabs(b->y1 - b->y2) + fabs(b->x1 - b->x3) + fabs(b->y1 - b->y3);
			l = 1.0;
		}
		if ((d < l * 0.25) || (b == beziers + 31))
		{
			cg_path_line_to(path, b->x4, b->y4);
			--b;
		}
		else
		{
			split(b, b + 1, b);
			++b;
		}
	}
}

static inline struct cg_path_t* cg_path_clone(const struct cg_path_t* path)
{
	struct cg_path_t* result = cg_path_create();
	cg_array_ensure(result->elements, path->elements.size);
	cg_array_ensure(result->points, path->points.size);

	memcpy(result->elements.data, path->elements.data, (size_t)path->elements.size * sizeof(enum cg_path_element_t));
	memcpy(result->points.data, path->points.data, (size_t)path->points.size * sizeof(struct cg_point_t));

	result->elements.size = path->elements.size;
	result->points.size = path->points.size;
	result->contours = path->contours;
	result->start = path->start;
	return result;
}

static inline struct cg_path_t* cg_path_clone_flat(struct cg_path_t* path)
{
	struct cg_point_t* points = path->points.data;
	struct cg_path_t* result = cg_path_create();
	struct cg_point_t p0;

	cg_array_ensure(result->elements, path->elements.size);
	cg_array_ensure(result->points, path->points.size);
	for (int i = 0; i < path->elements.size; i++)
	{
		switch (path->elements.data[i])
		{
		case CG_PATH_ELEMENT_MOVE_TO:
			cg_path_move_to(result, points[0].x, points[0].y);
			points += 1;
			break;
		case CG_PATH_ELEMENT_LINE_TO:
			cg_path_line_to(result, points[0].x, points[0].y);
			points += 1;
			break;
		case CG_PATH_ELEMENT_CURVE_TO:
			cg_path_get_current_point(result, &p0.x, &p0.y);
			flatten(result, &p0, points, points + 1, points + 2);
			points += 3;
			break;
		case CG_PATH_ELEMENT_CLOSE:
			cg_path_line_to(result, points[0].x, points[0].y);
			points += 1;
			break;
		default:
			break;
		}
	}
	return result;
}

static struct cg_dash_t* cg_dash_create(double* dashes, int ndash, double offset)
{
	if (dashes && (ndash > 0))
	{
		struct cg_dash_t* dash = (cg_dash_t*)malloc(sizeof(struct cg_dash_t));
		dash->offset = offset;
		dash->data = (double*)malloc((size_t)ndash * sizeof(double));
		dash->size = ndash;
		memcpy(dash->data, dashes, (size_t)ndash * sizeof(double));
		return dash;
	}
	return NULL;
}

static struct cg_dash_t* cg_dash_clone(struct cg_dash_t* dash)
{
	if (dash)
		return cg_dash_create(dash->data, dash->size, dash->offset);
	return NULL;
}

static void cg_dash_destroy(struct cg_dash_t* dash)
{
	if (dash)
	{
		free(dash->data);
		free(dash);
	}
}

static inline struct cg_path_t* cg_dash_path(struct cg_dash_t* dash, struct cg_path_t* path)
{
	if ((dash->data == NULL) || (dash->size <= 0))
		return cg_path_clone(path);

	int toggle = 1;
	int offset = 0;
	double phase = dash->offset;
	while (phase >= dash->data[offset])
	{
		toggle = !toggle;
		phase -= dash->data[offset];
		offset += 1;
		if (offset == dash->size)
			offset = 0;
	}

	struct cg_path_t* flat = cg_path_clone_flat(path);
	struct cg_path_t* result = cg_path_create();
	cg_array_ensure(result->elements, flat->elements.size);
	cg_array_ensure(result->points, flat->points.size);

	enum cg_path_element_t* elements = flat->elements.data;
	enum cg_path_element_t* end = elements + flat->elements.size;
	struct cg_point_t* points = flat->points.data;
	while (elements < end)
	{
		int itoggle = toggle;
		int ioffset = offset;
		double iphase = phase;
		double x0 = points->x;
		double y0 = points->y;
		if (itoggle)
			cg_path_move_to(result, x0, y0);
		++elements;
		++points;
		while ((elements < end) && (*elements == CG_PATH_ELEMENT_LINE_TO))
		{
			double dx = points->x - x0;
			double dy = points->y - y0;
			double dist0 = sqrt(dx * dx + dy * dy);
			double dist1 = 0;
			while (dist0 - dist1 > dash->data[ioffset] - iphase)
			{
				dist1 += dash->data[ioffset] - iphase;
				double a = dist1 / dist0;
				double x = x0 + a * dx;
				double y = y0 + a * dy;
				if (itoggle)
					cg_path_line_to(result, x, y);
				else
					cg_path_move_to(result, x, y);
				itoggle = !itoggle;
				iphase = 0;
				ioffset += 1;
				if (ioffset == dash->size)
					ioffset = 0;
			}
			iphase += dist0 - dist1;
			x0 = points->x;
			y0 = points->y;
			if (itoggle)
				cg_path_line_to(result, x0, y0);
			++elements;
			++points;
		}
	}
	cg_path_destroy(flat);
	return result;
}

#define ALIGN_SIZE(size)	(((size) + 7ul) & ~7ul)
static void ft_outline_init(XCG_FT_Outline* outline, struct cg_ctx_t* ctx, int points, int contours)
{
	size_t size_a = ALIGN_SIZE((points + contours) * sizeof(XCG_FT_Vector));
	size_t size_b = ALIGN_SIZE((points + contours) * sizeof(char));
	size_t size_c = ALIGN_SIZE(contours * sizeof(int));
	size_t size_d = ALIGN_SIZE(contours * sizeof(char));
	size_t size_n = size_a + size_b + size_c + size_d;
	if (size_n > ctx->outline_size)
	{
		ctx->outline_data = realloc(ctx->outline_data, size_n);
		ctx->outline_size = size_n;
	}

	XCG_FT_Byte* data = (XCG_FT_Byte*)ctx->outline_data;
	outline->points = (XCG_FT_Vector*)(data);
	outline->tags = outline->contours_flag = NULL;
	outline->contours = NULL;
	if (data)
	{
		outline->tags = (char*)(data + size_a);
		outline->contours = (int*)(data + size_a + size_b);
		outline->contours_flag = (char*)(data + size_a + size_b + size_c);
	}
	outline->n_points = 0;
	outline->n_contours = 0;
	outline->flags = 0x0;
}

#define FT_COORD(x)			(XCG_FT_Pos)((x) * 64)
static void ft_outline_move_to(XCG_FT_Outline* ft, double x, double y)
{
	ft->points[ft->n_points].x = FT_COORD(x);
	ft->points[ft->n_points].y = FT_COORD(y);
	ft->tags[ft->n_points] = XCG_FT_CURVE_TAG_ON;
	if (ft->n_points)
	{
		ft->contours[ft->n_contours] = ft->n_points - 1;
		ft->n_contours++;
	}
	ft->contours_flag[ft->n_contours] = 1;
	ft->n_points++;
}

static void ft_outline_line_to(XCG_FT_Outline* ft, double x, double y)
{
	ft->points[ft->n_points].x = FT_COORD(x);
	ft->points[ft->n_points].y = FT_COORD(y);
	ft->tags[ft->n_points] = XCG_FT_CURVE_TAG_ON;
	ft->n_points++;
}

static void ft_outline_curve_to(XCG_FT_Outline* ft, double x1, double y1, double x2, double y2, double x3, double y3)
{
	ft->points[ft->n_points].x = FT_COORD(x1);
	ft->points[ft->n_points].y = FT_COORD(y1);
	ft->tags[ft->n_points] = XCG_FT_CURVE_TAG_CUBIC;
	ft->n_points++;

	ft->points[ft->n_points].x = FT_COORD(x2);
	ft->points[ft->n_points].y = FT_COORD(y2);
	ft->tags[ft->n_points] = XCG_FT_CURVE_TAG_CUBIC;
	ft->n_points++;

	ft->points[ft->n_points].x = FT_COORD(x3);
	ft->points[ft->n_points].y = FT_COORD(y3);
	ft->tags[ft->n_points] = XCG_FT_CURVE_TAG_ON;
	ft->n_points++;
}

static void ft_outline_close(XCG_FT_Outline* ft)
{
	ft->contours_flag[ft->n_contours] = 0;
	int index = ft->n_contours ? ft->contours[ft->n_contours - 1] + 1 : 0;
	if (index == ft->n_points)
		return;

	ft->points[ft->n_points].x = ft->points[index].x;
	ft->points[ft->n_points].y = ft->points[index].y;
	ft->tags[ft->n_points] = XCG_FT_CURVE_TAG_ON;
	ft->n_points++;
}

static void ft_outline_end(XCG_FT_Outline* ft)
{
	if (ft->n_points)
	{
		ft->contours[ft->n_contours] = ft->n_points - 1;
		ft->n_contours++;
	}
}

static void ft_outline_convert(XCG_FT_Outline* outline, struct cg_ctx_t* ctx, struct cg_path_t* path, struct cg_matrix_t* matrix)
{
	ft_outline_init(outline, ctx, path->points.size, path->contours);
	enum cg_path_element_t* elements = path->elements.data;
	struct cg_point_t* points = path->points.data;
	struct cg_point_t p[3];
	for (int i = 0; i < path->elements.size; i++)
	{
		switch (elements[i])
		{
		case CG_PATH_ELEMENT_MOVE_TO:
			cg_matrix_map_point(matrix, &points[0], &p[0]);
			ft_outline_move_to(outline, p[0].x, p[0].y);
			points += 1;
			break;
		case CG_PATH_ELEMENT_LINE_TO:
			cg_matrix_map_point(matrix, &points[0], &p[0]);
			ft_outline_line_to(outline, p[0].x, p[0].y);
			points += 1;
			break;
		case CG_PATH_ELEMENT_CURVE_TO:
			cg_matrix_map_point(matrix, &points[0], &p[0]);
			cg_matrix_map_point(matrix, &points[1], &p[1]);
			cg_matrix_map_point(matrix, &points[2], &p[2]);
			ft_outline_curve_to(outline, p[0].x, p[0].y, p[1].x, p[1].y, p[2].x, p[2].y);
			points += 3;
			break;
		case CG_PATH_ELEMENT_CLOSE:
			ft_outline_close(outline);
			points += 1;
			break;
		}
	}
	ft_outline_end(outline);
}

static void ft_outline_convert_dash(XCG_FT_Outline* outline, struct cg_ctx_t* ctx, struct cg_path_t* path, struct cg_matrix_t* matrix, struct cg_dash_t* dash)
{
	struct cg_path_t* dashed = cg_dash_path(dash, path);
	ft_outline_convert(outline, ctx, dashed, matrix);
	cg_path_destroy(dashed);
}

static void generation_callback(int count, const XCG_FT_Span* spans, void* user)
{
	struct cg_rle_t* rle = (cg_rle_t*)user;
	cg_array_ensure(rle->spans, count);
	struct cg_span_t* data = rle->spans.data + rle->spans.size;
	memcpy(data, spans, (size_t)count * sizeof(struct cg_span_t));
	rle->spans.size += count;
}

static struct cg_rle_t* cg_rle_create(void)
{
	struct cg_rle_t* rle = (cg_rle_t*)malloc(sizeof(struct cg_rle_t));
	cg_array_init(rle->spans);
	rle->x = 0;
	rle->y = 0;
	rle->w = 0;
	rle->h = 0;
	return rle;
}

static void cg_rle_destroy(struct cg_rle_t* rle)
{
	if (rle)
	{
		free(rle->spans.data);
		free(rle);
	}
}

static void cg_rle_rasterize(struct cg_ctx_t* ctx, struct cg_rle_t* rle, struct cg_path_t* path, struct cg_matrix_t* m, struct cg_rect_t* clip, struct cg_stroke_data_t* stroke, enum cg_fill_rule_t winding)
{
	XCG_FT_Raster_Params params;
	params.flags = XCG_FT_RASTER_FLAG_DIRECT | XCG_FT_RASTER_FLAG_AA;
	params.gray_spans = generation_callback;
	params.user = rle;
	if (clip)
	{
		params.flags |= XCG_FT_RASTER_FLAG_CLIP;
		params.clip_box.xMin = (XCG_FT_Pos)(clip->x);
		params.clip_box.yMin = (XCG_FT_Pos)(clip->y);
		params.clip_box.xMax = (XCG_FT_Pos)(clip->x + clip->w);
		params.clip_box.yMax = (XCG_FT_Pos)(clip->y + clip->h);
	}
	if (stroke)
	{
		XCG_FT_Outline outline;
		if (stroke->dash == NULL)
			ft_outline_convert(&outline, ctx, path, m);
		else
			ft_outline_convert_dash(&outline, ctx, path, m, stroke->dash);
		XCG_FT_Stroker_LineCap ftCap;
		XCG_FT_Stroker_LineJoin ftJoin;
		XCG_FT_Fixed ftWidth;
		XCG_FT_Fixed ftMiterLimit;

		struct cg_point_t p1 = { 0, 0 };
		struct cg_point_t p2 = { 1.41421356237309504880, 1.41421356237309504880 };
		struct cg_point_t p3;

		cg_matrix_map_point(m, &p1, &p1);
		cg_matrix_map_point(m, &p2, &p2);

		p3.x = p2.x - p1.x;
		p3.y = p2.y - p1.y;

		double scale = sqrt(p3.x * p3.x + p3.y * p3.y) / 2.0;

		ftWidth = (XCG_FT_Fixed)(stroke->width * scale * 0.5 * (1 << 6));
		ftMiterLimit = (XCG_FT_Fixed)(stroke->miterlimit * (1 << 16));

		switch (stroke->cap)
		{
		case CG_LINE_CAP_SQUARE:
			ftCap = XCG_FT_STROKER_LINECAP_SQUARE;
			break;
		case CG_LINE_CAP_ROUND:
			ftCap = XCG_FT_STROKER_LINECAP_ROUND;
			break;
		default:
			ftCap = XCG_FT_STROKER_LINECAP_BUTT;
			break;
		}
		switch (stroke->join)
		{
		case CG_LINE_JOIN_BEVEL:
			ftJoin = XCG_FT_STROKER_LINEJOIN_BEVEL;
			break;
		case CG_LINE_JOIN_ROUND:
			ftJoin = XCG_FT_STROKER_LINEJOIN_ROUND;
			break;
		default:
			ftJoin = XCG_FT_STROKER_LINEJOIN_MITER_FIXED;
			break;
		}
		XCG_FT_Stroker stroker;
		XCG_FT_Stroker_New(&stroker);
		XCG_FT_Stroker_Set(stroker, ftWidth, ftCap, ftJoin, ftMiterLimit);
		XCG_FT_Stroker_ParseOutline(stroker, &outline);

		XCG_FT_UInt points;
		XCG_FT_UInt contours;
		XCG_FT_Stroker_GetCounts(stroker, &points, &contours);

		ft_outline_init(&outline, ctx, points, contours);
		XCG_FT_Stroker_Export(stroker, &outline);
		XCG_FT_Stroker_Done(stroker);

		outline.flags = XCG_FT_OUTLINE_NONE;
		params.source = &outline;
		XCG_FT_Raster_Render(&params);
	}
	else
	{
		XCG_FT_Outline outline;
		ft_outline_convert(&outline, ctx, path, m);
		switch (winding)
		{
		case CG_FILL_RULE_EVEN_ODD:
			outline.flags = XCG_FT_OUTLINE_EVEN_ODD_FILL;
			break;
		default:
			outline.flags = XCG_FT_OUTLINE_NONE;
			break;
		}

		params.source = &outline;
		XCG_FT_Raster_Render(&params);
	}

	if (rle->spans.size == 0)
	{
		rle->x = 0;
		rle->y = 0;
		rle->w = 0;
		rle->h = 0;
		return;
	}

	struct cg_span_t* spans = rle->spans.data;
	int x1 = INT_MAX;
	int y1 = spans[0].y;
	int x2 = 0;
	int y2 = spans[rle->spans.size - 1].y;
	for (int i = 0; i < rle->spans.size; i++)
	{
		if (spans[i].x < x1)
			x1 = spans[i].x;
		if (spans[i].x + spans[i].len > x2)
			x2 = spans[i].x + spans[i].len;
	}

	rle->x = x1;
	rle->y = y1;
	rle->w = x2 - x1;
	rle->h = y2 - y1 + 1;
}

static struct cg_rle_t* cg_rle_intersection(struct cg_rle_t* a, struct cg_rle_t* b)
{
	struct cg_rle_t* result = (cg_rle_t*)malloc(sizeof(struct cg_rle_t));
	cg_array_init(result->spans);
	cg_array_ensure(result->spans, std::max(a->spans.size, b->spans.size));

	struct cg_span_t* a_spans = a->spans.data;
	struct cg_span_t* a_end = a_spans + a->spans.size;
	struct cg_span_t* b_spans = b->spans.data;
	struct cg_span_t* b_end = b_spans + b->spans.size;
	while ((a_spans < a_end) && (b_spans < b_end))
	{
		if (b_spans->y > a_spans->y)
		{
			++a_spans;
			continue;
		}
		if (a_spans->y != b_spans->y)
		{
			++b_spans;
			continue;
		}
		int ax1 = a_spans->x;
		int ax2 = ax1 + a_spans->len;
		int bx1 = b_spans->x;
		int bx2 = bx1 + b_spans->len;
		if (bx1 < ax1 && bx2 < ax1)
		{
			++b_spans;
			continue;
		}
		else if (ax1 < bx1 && ax2 < bx1)
		{
			++a_spans;
			continue;
		}
		int x = std::max(ax1, bx1);
		int len = std::min(ax2, bx2) - x;
		if (len)
		{
			struct cg_span_t* span = result->spans.data + result->spans.size;
			span->x = x;
			span->len = len;
			span->y = a_spans->y;
			span->coverage = CG_DIV255(a_spans->coverage * b_spans->coverage);
			result->spans.size += 1;
		}
		if (ax2 < bx2)
		{
			++a_spans;
		}
		else
		{
			++b_spans;
		}
	}
	if (result->spans.size == 0)
	{
		result->x = 0;
		result->y = 0;
		result->w = 0;
		result->h = 0;
		return result;
	}
	struct cg_span_t* spans = result->spans.data;
	int x1 = INT_MAX;
	int y1 = spans[0].y;
	int x2 = 0;
	int y2 = spans[result->spans.size - 1].y;
	for (int i = 0; i < result->spans.size; i++)
	{
		if (spans[i].x < x1)
			x1 = spans[i].x;
		if (spans[i].x + spans[i].len > x2)
			x2 = spans[i].x + spans[i].len;
	}
	result->x = x1;
	result->y = y1;
	result->w = x2 - x1;
	result->h = y2 - y1 + 1;

	return result;
}

static void cg_rle_clip_path(struct cg_rle_t* rle, struct cg_rle_t* clip)
{
	if (rle && clip)
	{
		struct cg_rle_t* result = cg_rle_intersection(rle, clip);
		cg_array_ensure(rle->spans, result->spans.size);
		memcpy(rle->spans.data, result->spans.data, (size_t)result->spans.size * sizeof(struct cg_span_t));
		rle->spans.size = result->spans.size;
		rle->x = result->x;
		rle->y = result->y;
		rle->w = result->w;
		rle->h = result->h;
		cg_rle_destroy(result);
	}
}

static struct cg_rle_t* cg_rle_clone(struct cg_rle_t* rle)
{
	if (rle)
	{
		struct cg_rle_t* result = (cg_rle_t*)malloc(sizeof(struct cg_rle_t));
		cg_array_init(result->spans);
		cg_array_ensure(result->spans, rle->spans.size);
		memcpy(result->spans.data, rle->spans.data, (size_t)rle->spans.size * sizeof(struct cg_span_t));
		result->spans.size = rle->spans.size;
		result->x = rle->x;
		result->y = rle->y;
		result->w = rle->w;
		result->h = rle->h;
		return result;
	}
	return NULL;
}

static inline void cg_rle_clear(struct cg_rle_t* rle)
{
	rle->spans.size = 0;
	rle->x = 0;
	rle->y = 0;
	rle->w = 0;
	rle->h = 0;
}

static void cg_gradient_init_linear(struct cg_gradient_t* gradient, double x1, double y1, double x2, double y2)
{
	gradient->type = CG_GRADIENT_TYPE_LINEAR;
	gradient->spread = CG_SPREAD_METHOD_PAD;
	gradient->opacity = 1.0;
	gradient->stops.size = 0;
	cg_matrix_init_identity(&gradient->matrix);
	cg_gradient_set_values_linear(gradient, x1, y1, x2, y2);
}

static void cg_gradient_init_radial(struct cg_gradient_t* gradient, double cx, double cy, double cr, double fx, double fy, double fr)
{
	gradient->type = CG_GRADIENT_TYPE_RADIAL;
	gradient->spread = CG_SPREAD_METHOD_PAD;
	gradient->opacity = 1.0;
	gradient->stops.size = 0;
	cg_matrix_init_identity(&gradient->matrix);
	cg_gradient_set_values_radial(gradient, cx, cy, cr, fx, fy, fr);
}

void cg_gradient_set_values_linear(struct cg_gradient_t* gradient, double x1, double y1, double x2, double y2)
{
	gradient->values[0] = x1;
	gradient->values[1] = y1;
	gradient->values[2] = x2;
	gradient->values[3] = y2;
}

void cg_gradient_set_values_radial(struct cg_gradient_t* gradient, double cx, double cy, double cr, double fx, double fy, double fr)
{
	gradient->values[0] = cx;
	gradient->values[1] = cy;
	gradient->values[2] = cr;
	gradient->values[3] = fx;
	gradient->values[4] = fy;
	gradient->values[5] = fr;
}

void cg_gradient_set_spread(struct cg_gradient_t* gradient, enum cg_spread_method_t spread)
{
	gradient->spread = spread;
}

void cg_gradient_set_matrix(struct cg_gradient_t* gradient, struct cg_matrix_t* m)
{
	memcpy(&gradient->matrix, m, sizeof(struct cg_matrix_t));
}

void cg_gradient_set_opacity(struct cg_gradient_t* gradient, double opacity)
{
	gradient->opacity = glm::clamp(opacity, 0.0, 1.0);
}

void cg_gradient_add_stop_rgb(struct cg_gradient_t* gradient, double offset, double r, double g, double b)
{
	cg_gradient_add_stop_rgba(gradient, offset, r, g, b, 1.0);
}

void cg_gradient_add_stop_rgba(struct cg_gradient_t* gradient, double offset, double r, double g, double b, double a)
{
	if (offset < 0.0)
		offset = 0.0;
	if (offset > 1.0)
		offset = 1.0;
	cg_array_ensure(gradient->stops, 1);
	struct cg_gradient_stop_t* stops = gradient->stops.data;
	int nstops = gradient->stops.size;
	int i;
	for (i = 0; i < nstops; i++)
	{
		if (offset < stops[i].offset)
		{
			memmove(&stops[i + 1], &stops[i], (size_t)(nstops - i) * sizeof(struct cg_gradient_stop_t));
			break;
		}
	}
	struct cg_gradient_stop_t* stop = &stops[i];
	stop->offset = offset;
	cg_color_init_rgba(&stop->color, r, g, b, a);
	gradient->stops.size += 1;
}

void cg_gradient_add_stop_color(struct cg_gradient_t* gradient, double offset, struct cg_color_t* color)
{
	cg_gradient_add_stop_rgba(gradient, offset, color->r, color->g, color->b, color->a);
}

void cg_gradient_add_stop(struct cg_gradient_t* gradient, struct cg_gradient_stop_t* stop)
{
	cg_gradient_add_stop_rgba(gradient, stop->offset, stop->color.r, stop->color.g, stop->color.b, stop->color.a);
}

void cg_gradient_clear_stops(struct cg_gradient_t* gradient)
{
	gradient->stops.size = 0;
}

static void cg_gradient_copy(struct cg_gradient_t* gradient, struct cg_gradient_t* source)
{
	gradient->type = source->type;
	gradient->spread = source->spread;
	gradient->matrix = source->matrix;
	gradient->opacity = source->opacity;
	cg_array_ensure(gradient->stops, source->stops.size);
	memcpy(gradient->values, source->values, sizeof(source->values));
	memcpy(gradient->stops.data, source->stops.data, source->stops.size * sizeof(struct cg_gradient_stop_t));
}

static void cg_gradient_destroy(struct cg_gradient_t* gradient)
{
	if (gradient->stops.data)
		free(gradient->stops.data);
}

static void cg_texture_init(struct cg_texture_t* texture, struct cg_surface_t* surface, enum cg_texture_type_t type)
{
	surface = cg_surface_reference(surface);
	cg_surface_destroy(texture->surface);
	texture->type = type;
	texture->surface = surface;
	texture->opacity = 1.0;
	cg_matrix_init_identity(&texture->matrix);
}

void cg_texture_set_type(struct cg_texture_t* texture, enum cg_texture_type_t type)
{
	texture->type = type;
}

void cg_texture_set_matrix(struct cg_texture_t* texture, struct cg_matrix_t* m)
{
	memcpy(&texture->matrix, m, sizeof(struct cg_matrix_t));
}

void cg_texture_set_surface(struct cg_texture_t* texture, struct cg_surface_t* surface)
{
	surface = cg_surface_reference(surface);
	cg_surface_destroy(texture->surface);
	texture->surface = surface;
}

void cg_texture_set_opacity(struct cg_texture_t* texture, double opacity)
{
	texture->opacity = glm::clamp(opacity, 0.0, 1.0);
}

static void cg_texture_copy(struct cg_texture_t* texture, struct cg_texture_t* source)
{
	struct cg_surface_t* surface = cg_surface_reference(source->surface);
	cg_surface_destroy(texture->surface);
	texture->type = source->type;
	texture->surface = surface;
	texture->opacity = source->opacity;
	texture->matrix = source->matrix;
}

static void cg_texture_destroy(struct cg_texture_t* texture)
{
	cg_surface_destroy(texture->surface);
}

static void cg_paint_init(struct cg_paint_t* paint)
{
	paint->type = CG_PAINT_TYPE_COLOR;
	paint->texture.surface = NULL;
	cg_array_init(paint->gradient.stops);
	cg_color_init_rgba(&paint->color, 0, 0, 0, 1.0);
}

static void cg_paint_destroy(struct cg_paint_t* paint)
{
	cg_texture_destroy(&paint->texture);
	cg_gradient_destroy(&paint->gradient);
}

static void cg_paint_copy(struct cg_paint_t* paint, struct cg_paint_t* source)
{
	paint->type = source->type;
	switch (source->type)
	{
	case CG_PAINT_TYPE_COLOR:
		paint->color = source->color;
		break;
	case CG_PAINT_TYPE_GRADIENT:
		cg_gradient_copy(&paint->gradient, &source->gradient);
		break;
	case CG_PAINT_TYPE_TEXTURE:
		cg_texture_copy(&paint->texture, &source->texture);
		break;
	default:
		break;
	}
}

struct cg_gradient_data_t {
	enum cg_spread_method_t spread;
	struct cg_matrix_t matrix;
	uint32_t colortable[1024];
	union {
		struct {
			double x1, y1;
			double x2, y2;
		} linear;
		struct {
			double cx, cy, cr;
			double fx, fy, fr;
		} radial;
	};
};

struct cg_texture_data_t {
	struct cg_matrix_t matrix;
	int width;
	int height;
	int stride;
	int alpha;
	uint8_t* pixels;
};

struct cg_linear_gradient_values_t {
	double dx;
	double dy;
	double l;
	double off;
};

struct cg_radial_gradient_values_t {
	double dx;
	double dy;
	double dr;
	double sqrfr;
	double a;
	double inv2a;
	int extended;
};

static inline uint32_t premultiply_color(struct cg_color_t* color, double opacity)
{
	uint32_t alpha = (uint8_t)(color->a * opacity * 255);
	uint32_t pr = (uint8_t)(color->r * alpha);
	uint32_t pg = (uint8_t)(color->g * alpha);
	uint32_t pb = (uint8_t)(color->b * alpha);
	return (alpha << 24) | (pr << 16) | (pg << 8) | (pb);
}

static inline uint32_t combine_opacity(struct cg_color_t* color, double opacity)
{
	uint32_t a = (uint8_t)(color->a * opacity * 255);
	uint32_t r = (uint8_t)(color->r * 255);
	uint32_t g = (uint8_t)(color->g * 255);
	uint32_t b = (uint8_t)(color->b * 255);
	return (a << 24) | (r << 16) | (g << 8) | (b);
}

static inline uint32_t premultiply_pixel(uint32_t color)
{
	uint32_t a = CG_ALPHA(color);
	uint32_t r = (color >> 16) & 0xff;
	uint32_t g = (color >> 8) & 0xff;
	uint32_t b = (color >> 0) & 0xff;
	uint32_t pr = (r * a) / 255;
	uint32_t pg = (g * a) / 255;
	uint32_t pb = (b * a) / 255;
	return (a << 24) | (pr << 16) | (pg << 8) | (pb);
}

static inline uint32_t interpolate_pixel(uint32_t x, uint32_t a, uint32_t y, uint32_t b)
{
	uint32_t t = (x & 0xff00ff) * a + (y & 0xff00ff) * b;
	t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
	t &= 0xff00ff;
	x = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b;
	x = (x + ((x >> 8) & 0xff00ff) + 0x800080);
	x &= 0xff00ff00;
	x |= t;
	return x;
}

static void cg_memfill32(uint32_t* dst, uint32_t val, int len)
{
	for (int i = 0; i < len; i++)
		dst[i] = val;
}
//extern __typeof(__cg_memfill32) cg_memfill32 __attribute__((weak, alias("__cg_memfill32")));

static inline int gradient_clamp(struct cg_gradient_data_t* gradient, int ipos)
{
	switch (gradient->spread)
	{
	case CG_SPREAD_METHOD_PAD:
		if (ipos < 0)
			ipos = 0;
		else if (ipos >= 1024)
			ipos = 1024 - 1;
		break;
	case CG_SPREAD_METHOD_REFLECT:
		ipos = ipos % 2048;
		ipos = ipos < 0 ? 2048 + ipos : ipos;
		ipos = ipos >= 1024 ? 2048 - 1 - ipos : ipos;
		break;
	case CG_SPREAD_METHOD_REPEAT:
		ipos = ipos % 1024;
		ipos = ipos < 0 ? 1024 + ipos : ipos;
		break;
	default:
		break;
	}
	return ipos;
}

#define FIXPT_BITS	(8)
#define FIXPT_SIZE	(1 << FIXPT_BITS)

static inline uint32_t gradient_pixel_fixed(struct cg_gradient_data_t* gradient, int fixed_pos)
{
	int ipos = (fixed_pos + (FIXPT_SIZE / 2)) >> FIXPT_BITS;
	return gradient->colortable[gradient_clamp(gradient, ipos)];
}

static inline uint32_t gradient_pixel(struct cg_gradient_data_t* gradient, double pos)
{
	int ipos = (int)(pos * (1024 - 1) + 0.5);
	return gradient->colortable[gradient_clamp(gradient, ipos)];
}

static inline void fetch_linear_gradient(uint32_t* buffer, struct cg_linear_gradient_values_t* v, struct cg_gradient_data_t* gradient, int y, int x, int length)
{
	double t, inc;
	double rx = 0, ry = 0;

	if (v->l == 0.0)
	{
		t = inc = 0;
	}
	else
	{
		rx = gradient->matrix.c * (y + 0.5) + gradient->matrix.a * (x + 0.5) + gradient->matrix.tx;
		ry = gradient->matrix.d * (y + 0.5) + gradient->matrix.b * (x + 0.5) + gradient->matrix.ty;
		t = v->dx * rx + v->dy * ry + v->off;
		inc = v->dx * gradient->matrix.a + v->dy * gradient->matrix.b;
		t *= (1024 - 1);
		inc *= (1024 - 1);
	}
	uint32_t* end = buffer + length;
	if ((inc > -1e-5) && (inc < 1e-5))
	{
		cg_memfill32(buffer, gradient_pixel_fixed(gradient, (int)(t * FIXPT_SIZE)), length);
	}
	else
	{
		if (t + inc * length < (double)(INT_MAX >> (FIXPT_BITS + 1)) && t + inc * length >(double)(INT_MIN >> (FIXPT_BITS + 1)))
		{
			int t_fixed = (int)(t * FIXPT_SIZE);
			int inc_fixed = (int)(inc * FIXPT_SIZE);
			while (buffer < end)
			{
				*buffer = gradient_pixel_fixed(gradient, t_fixed);
				t_fixed += inc_fixed;
				++buffer;
			}
		}
		else
		{
			while (buffer < end)
			{
				*buffer = gradient_pixel(gradient, t / 1024);
				t += inc;
				++buffer;
			}
		}
	}
}

static inline void fetch_radial_gradient(uint32_t* buffer, struct cg_radial_gradient_values_t* v, struct cg_gradient_data_t* gradient, int y, int x, int length)
{
	if (v->a == 0.0)
	{
		cg_memfill32(buffer, 0, length);
		return;
	}

	double rx = gradient->matrix.c * (y + 0.5) + gradient->matrix.tx + gradient->matrix.a * (x + 0.5);
	double ry = gradient->matrix.d * (y + 0.5) + gradient->matrix.ty + gradient->matrix.b * (x + 0.5);
	rx -= gradient->radial.fx;
	ry -= gradient->radial.fy;

	double inv_a = 1.0 / (2.0 * v->a);
	double delta_rx = gradient->matrix.a;
	double delta_ry = gradient->matrix.b;

	double b = 2 * (v->dr * gradient->radial.fr + rx * v->dx + ry * v->dy);
	double delta_b = 2 * (delta_rx * v->dx + delta_ry * v->dy);
	double b_delta_b = 2 * b * delta_b;
	double delta_b_delta_b = 2 * delta_b * delta_b;

	double bb = b * b;
	double delta_bb = delta_b * delta_b;

	b *= inv_a;
	delta_b *= inv_a;

	double rxrxryry = rx * rx + ry * ry;
	double delta_rxrxryry = delta_rx * delta_rx + delta_ry * delta_ry;
	double rx_plus_ry = 2 * (rx * delta_rx + ry * delta_ry);
	double delta_rx_plus_ry = 2 * delta_rxrxryry;

	inv_a *= inv_a;

	double det = (bb - 4 * v->a * (v->sqrfr - rxrxryry)) * inv_a;
	double delta_det = (b_delta_b + delta_bb + 4 * v->a * (rx_plus_ry + delta_rxrxryry)) * inv_a;
	double delta_delta_det = (delta_b_delta_b + 4 * v->a * delta_rx_plus_ry) * inv_a;

	uint32_t* end = buffer + length;
	if (v->extended)
	{
		while (buffer < end)
		{
			uint32_t result = 0;
			det = fabs(det) < DBL_EPSILON ? 0.0 : det;
			if (det >= 0)
			{
				double w = sqrt(det) - b;
				if (gradient->radial.fr + v->dr * w >= 0)
					result = gradient_pixel(gradient, w);
			}
			*buffer = result;
			det += delta_det;
			delta_det += delta_delta_det;
			b += delta_b;
			++buffer;
		}
	}
	else
	{
		while (buffer < end)
		{
			det = fabs(det) < DBL_EPSILON ? 0.0 : det;
			uint32_t result = 0;
			if (det >= 0)
				result = gradient_pixel(gradient, sqrt(det) - b);
			*buffer++ = result;
			det += delta_det;
			delta_det += delta_delta_det;
			b += delta_b;
		}
	}
}

static void cg_comp_solid_source(uint32_t* dst, int len, uint32_t color, uint32_t alpha)
{
	if (alpha == 255)
	{
		cg_memfill32(dst, color, len);
	}
	else
	{
		uint32_t ialpha = 255 - alpha;
		color = CG_BYTE_MUL(color, alpha);
		for (int i = 0; i < len; i++)
			dst[i] = color + CG_BYTE_MUL(dst[i], ialpha);
	}
}
//extern __typeof(__cg_comp_solid_source) cg_comp_solid_source __attribute__((weak, alias("__cg_comp_solid_source")));

static void cg_comp_solid_source_over(uint32_t* dst, int len, uint32_t color, uint32_t alpha)
{
	if ((alpha & CG_ALPHA(color)) == 255)
	{
		cg_memfill32(dst, color, len);
	}
	else
	{
		if (alpha != 255)
			color = CG_BYTE_MUL(color, alpha);
		uint32_t ialpha = 255 - CG_ALPHA(color);
		for (int i = 0; i < len; i++)
			dst[i] = color + CG_BYTE_MUL(dst[i], ialpha);
	}
}
//extern __typeof(__cg_comp_solid_source_over) cg_comp_solid_source_over __attribute__((weak, alias("__cg_comp_solid_source_over")));

static void cg_comp_solid_destination_in(uint32_t* dst, int len, uint32_t color, uint32_t alpha)
{
	uint32_t a = CG_ALPHA(color);
	if (alpha != 255)
		a = CG_BYTE_MUL(a, alpha) + 255 - alpha;
	for (int i = 0; i < len; i++)
		dst[i] = CG_BYTE_MUL(dst[i], a);
}
//extern __typeof(__cg_comp_solid_destination_in) cg_comp_solid_destination_in __attribute__((weak, alias("__cg_comp_solid_destination_in")));

static void cg_comp_solid_destination_out(uint32_t* dst, int len, uint32_t color, uint32_t alpha)
{
	uint32_t a = CG_ALPHA(~color);
	if (alpha != 255)
		a = CG_BYTE_MUL(a, alpha) + 255 - alpha;
	for (int i = 0; i < len; i++)
		dst[i] = CG_BYTE_MUL(dst[i], a);
}
//extern __typeof(__cg_comp_solid_destination_out) cg_comp_solid_destination_out __attribute__((weak, alias("__cg_comp_solid_destination_out")));

static void cg_comp_source(uint32_t* dst, int len, uint32_t* src, uint32_t alpha)
{
	if (alpha == 255)
	{
		memcpy(dst, src, (size_t)(len) * sizeof(uint32_t));
	}
	else
	{
		uint32_t ialpha = 255 - alpha;
		for (int i = 0; i < len; i++)
			dst[i] = interpolate_pixel(src[i], alpha, dst[i], ialpha);
	}
}
//extern __typeof(__cg_comp_source) cg_comp_source __attribute__((weak, alias("__cg_comp_source")));

static void cg_comp_source_over(uint32_t* dst, int len, uint32_t* src, uint32_t alpha)
{
	uint32_t s, sia;
	if (alpha == 255)
	{
		for (int i = 0; i < len; i++)
		{
			s = src[i];
			if (s >= 0xff000000)
				dst[i] = s;
			else if (s != 0)
			{
				sia = CG_ALPHA(~s);
				dst[i] = s + CG_BYTE_MUL(dst[i], sia);
			}
		}
	}
	else
	{
		for (int i = 0; i < len; i++)
		{
			s = CG_BYTE_MUL(src[i], alpha);
			sia = CG_ALPHA(~s);
			dst[i] = s + CG_BYTE_MUL(dst[i], sia);
		}
	}
}
//extern __typeof(__cg_comp_source_over) cg_comp_source_over __attribute__((weak, alias("__cg_comp_source_over")));

static void cg_comp_destination_in(uint32_t* dst, int len, uint32_t* src, uint32_t alpha)
{
	if (alpha == 255)
	{
		for (int i = 0; i < len; i++)
			dst[i] = CG_BYTE_MUL(dst[i], CG_ALPHA(src[i]));
	}
	else
	{
		uint32_t cia = 255 - alpha;
		uint32_t a;
		for (int i = 0; i < len; i++)
		{
			a = CG_BYTE_MUL(CG_ALPHA(src[i]), alpha) + cia;
			dst[i] = CG_BYTE_MUL(dst[i], a);
		}
	}
}
//extern __typeof(__cg_comp_destination_in) cg_comp_destination_in __attribute__((weak, alias("__cg_comp_destination_in")));

static void cg_comp_destination_out(uint32_t* dst, int len, uint32_t* src, uint32_t alpha)
{
	if (alpha == 255)
	{
		for (int i = 0; i < len; i++)
			dst[i] = CG_BYTE_MUL(dst[i], CG_ALPHA(~src[i]));
	}
	else
	{
		uint32_t cia = 255 - alpha;
		uint32_t sia;
		for (int i = 0; i < len; i++)
		{
			sia = CG_BYTE_MUL(CG_ALPHA(~src[i]), alpha) + cia;
			dst[i] = CG_BYTE_MUL(dst[i], sia);
		}
	}
}
//extern __typeof(__cg_comp_destination_out) cg_comp_destination_out __attribute__((weak, alias("__cg_comp_destination_out")));

typedef void (*cg_comp_solid_function_t)(uint32_t* dst, int len, uint32_t color, uint32_t alpha);
static const cg_comp_solid_function_t cg_comp_solid_map[] = {
	cg_comp_solid_source,
	cg_comp_solid_source_over,
	cg_comp_solid_destination_in,
	cg_comp_solid_destination_out,
};

typedef void (*cg_comp_function_t)(uint32_t* dst, int len, uint32_t* src, uint32_t alpha);
static const cg_comp_function_t cg_comp_map[] = {
	cg_comp_source,
	cg_comp_source_over,
	cg_comp_destination_in,
	cg_comp_destination_out,
};

static inline void blend_solid(struct cg_surface_t* surface, enum cg_operator_t op, struct cg_rle_t* rle, uint32_t solid)
{
	cg_comp_solid_function_t func = cg_comp_solid_map[op];
	int count = rle->spans.size;
	struct cg_span_t* spans = rle->spans.data;
	while (count--)
	{
		uint32_t* target = (uint32_t*)(surface->pixels + spans->y * surface->stride) + spans->x;
		func(target, spans->len, solid, spans->coverage);
		++spans;
	}
}

static inline void blend_linear_gradient(struct cg_surface_t* surface, enum cg_operator_t op, struct cg_rle_t* rle, struct cg_gradient_data_t* gradient)
{
	cg_comp_function_t func = cg_comp_map[op];
	unsigned int buffer[1024];

	struct cg_linear_gradient_values_t v;
	v.dx = gradient->linear.x2 - gradient->linear.x1;
	v.dy = gradient->linear.y2 - gradient->linear.y1;
	v.l = v.dx * v.dx + v.dy * v.dy;
	v.off = 0.0;
	if (v.l != 0.0)
	{
		v.dx /= v.l;
		v.dy /= v.l;
		v.off = -v.dx * gradient->linear.x1 - v.dy * gradient->linear.y1;
	}

	int count = rle->spans.size;
	struct cg_span_t* spans = rle->spans.data;
	while (count--)
	{
		int length = spans->len;
		int x = spans->x;
		while (length)
		{
			int l = std::min(length, 1024);
			fetch_linear_gradient(buffer, &v, gradient, spans->y, x, l);
			uint32_t* target = (uint32_t*)(surface->pixels + spans->y * surface->stride) + x;
			func(target, l, buffer, spans->coverage);
			x += l;
			length -= l;
		}
		++spans;
	}
}

static inline void blend_radial_gradient(struct cg_surface_t* surface, enum cg_operator_t op, struct cg_rle_t* rle, struct cg_gradient_data_t* gradient)
{
	cg_comp_function_t func = cg_comp_map[op];
	unsigned int buffer[1024];

	struct cg_radial_gradient_values_t v;
	v.dx = gradient->radial.cx - gradient->radial.fx;
	v.dy = gradient->radial.cy - gradient->radial.fy;
	v.dr = gradient->radial.cr - gradient->radial.fr;
	v.sqrfr = gradient->radial.fr * gradient->radial.fr;
	v.a = v.dr * v.dr - v.dx * v.dx - v.dy * v.dy;
	v.inv2a = 1.0 / (2.0 * v.a);
	v.extended = gradient->radial.fr != 0.0 || v.a <= 0.0;

	int count = rle->spans.size;
	struct cg_span_t* spans = rle->spans.data;
	while (count--)
	{
		int length = spans->len;
		int x = spans->x;
		while (length)
		{
			int l = std::min(length, 1024);
			fetch_radial_gradient(buffer, &v, gradient, spans->y, x, l);
			uint32_t* target = (uint32_t*)(surface->pixels + spans->y * surface->stride) + x;
			func(target, l, buffer, spans->coverage);
			x += l;
			length -= l;
		}
		++spans;
	}
}

#define FIXED_SCALE (1 << 16)
static inline void blend_untransformed_argb(struct cg_surface_t* surface, enum cg_operator_t op, struct cg_rle_t* rle, struct cg_texture_data_t* texture)
{
	cg_comp_function_t func = cg_comp_map[op];

	int image_width = texture->width;
	int image_height = texture->height;
	int xoff = (int)(texture->matrix.tx);
	int yoff = (int)(texture->matrix.ty);
	int count = rle->spans.size;
	struct cg_span_t* spans = rle->spans.data;
	while (count--)
	{
		int x = spans->x;
		int length = spans->len;
		int sx = xoff + x;
		int sy = yoff + spans->y;
		if (sy >= 0 && sy < image_height && sx < image_width)
		{
			if (sx < 0)
			{
				x -= sx;
				length += sx;
				sx = 0;
			}
			if (sx + length > image_width)
				length = image_width - sx;
			if (length > 0)
			{
				int coverage = (spans->coverage * texture->alpha) >> 8;
				uint32_t* src = (uint32_t*)(texture->pixels + sy * texture->stride) + sx;
				uint32_t* dst = (uint32_t*)(surface->pixels + spans->y * surface->stride) + x;
				func(dst, length, src, coverage);
			}
		}
		++spans;
	}
}

static inline void blend_transformed_argb(struct cg_surface_t* surface, enum cg_operator_t op, struct cg_rle_t* rle, struct cg_texture_data_t* texture)
{
	cg_comp_function_t func = cg_comp_map[op];
	uint32_t buffer[1024];

	int image_width = texture->width;
	int image_height = texture->height;
	int fdx = (int)(texture->matrix.a * FIXED_SCALE);
	int fdy = (int)(texture->matrix.b * FIXED_SCALE);
	int count = rle->spans.size;
	struct cg_span_t* spans = rle->spans.data;
	while (count--)
	{
		uint32_t* target = (uint32_t*)(surface->pixels + spans->y * surface->stride) + spans->x;
		double cx = spans->x + 0.5;
		double cy = spans->y + 0.5;
		int x = (int)((texture->matrix.c * cy + texture->matrix.a * cx + texture->matrix.tx) * FIXED_SCALE);
		int y = (int)((texture->matrix.d * cy + texture->matrix.b * cx + texture->matrix.ty) * FIXED_SCALE);
		int length = spans->len;
		int coverage = (spans->coverage * texture->alpha) >> 8;
		while (length)
		{
			int l = std::min(length, 1024);
			uint32_t* end = buffer + l;
			uint32_t* b = buffer;
			int start = 0;
			int clen = 0;
			while (b < end)
			{
				int px = x >> 16;
				int py = y >> 16;
				if (((unsigned int)px < (unsigned int)image_width) && ((unsigned int)py < (unsigned int)image_height))
				{
					*b = ((uint32_t*)(texture->pixels + py * texture->stride))[px];
					clen++;
				}
				x += fdx;
				y += fdy;
				++b;
				if (clen == 0)
					start++;
			}
			func(target + start, clen, buffer + start, coverage);
			target += l;
			length -= l;
		}
		++spans;
	}
}

static inline void blend_untransformed_tiled_argb(struct cg_surface_t* surface, enum cg_operator_t op, struct cg_rle_t* rle, struct cg_texture_data_t* texture)
{
	cg_comp_function_t func = cg_comp_map[op];

	int image_width = texture->width;
	int image_height = texture->height;
	int xoff = (int)(texture->matrix.tx) % image_width;
	int yoff = (int)(texture->matrix.ty) % image_height;
	if (xoff < 0)
		xoff += image_width;
	if (yoff < 0)
		yoff += image_height;
	int count = rle->spans.size;
	struct cg_span_t* spans = rle->spans.data;
	while (count--)
	{
		int x = spans->x;
		int length = spans->len;
		int sx = (xoff + spans->x) % image_width;
		int sy = (spans->y + yoff) % image_height;
		if (sx < 0)
			sx += image_width;
		if (sy < 0)
			sy += image_height;
		int coverage = (spans->coverage * texture->alpha) >> 8;
		while (length)
		{
			int l = std::min(image_width - sx, length);
			if (1024 < l)
				l = 1024;
			uint32_t* src = (uint32_t*)(texture->pixels + sy * texture->stride) + sx;
			uint32_t* dst = (uint32_t*)(surface->pixels + spans->y * surface->stride) + x;
			func(dst, l, src, coverage);
			x += l;
			length -= l;
			sx = 0;
		}
		++spans;
	}
}

static inline void blend_transformed_tiled_argb(struct cg_surface_t* surface, enum cg_operator_t op, struct cg_rle_t* rle, struct cg_texture_data_t* texture)
{
	cg_comp_function_t func = cg_comp_map[op];
	uint32_t buffer[1024];

	int image_width = texture->width;
	int image_height = texture->height;
	int scanline_offset = texture->stride / 4;
	int fdx = (int)(texture->matrix.a * FIXED_SCALE);
	int fdy = (int)(texture->matrix.b * FIXED_SCALE);
	int count = rle->spans.size;
	struct cg_span_t* spans = rle->spans.data;
	while (count--)
	{
		uint32_t* target = (uint32_t*)(surface->pixels + spans->y * surface->stride) + spans->x;
		uint32_t* image_bits = (uint32_t*)texture->pixels;
		double cx = spans->x + 0.5;
		double cy = spans->y + 0.5;
		int x = (int)((texture->matrix.c * cy + texture->matrix.a * cx + texture->matrix.tx) * FIXED_SCALE);
		int y = (int)((texture->matrix.d * cy + texture->matrix.b * cx + texture->matrix.ty) * FIXED_SCALE);
		int coverage = (spans->coverage * texture->alpha) >> 8;
		int length = spans->len;
		while (length)
		{
			int l = std::min(length, 1024);
			uint32_t* end = buffer + l;
			uint32_t* b = buffer;
			int px16 = x % (image_width << 16);
			int py16 = y % (image_height << 16);
			int px_delta = fdx % (image_width << 16);
			int py_delta = fdy % (image_height << 16);
			while (b < end)
			{
				if (px16 < 0)
					px16 += image_width << 16;
				if (py16 < 0)
					py16 += image_height << 16;
				int px = px16 >> 16;
				int py = py16 >> 16;
				int y_offset = py * scanline_offset;

				*b = image_bits[y_offset + px];
				x += fdx;
				y += fdy;
				px16 += px_delta;
				if (px16 >= image_width << 16)
					px16 -= image_width << 16;
				py16 += py_delta;
				if (py16 >= image_height << 16)
					py16 -= image_height << 16;
				++b;
			}
			func(target, l, buffer, coverage);
			target += l;
			length -= l;
		}
		++spans;
	}
}

static inline void cg_blend_color(struct cg_ctx_t* ctx, struct cg_rle_t* rle, struct cg_color_t* color)
{
	if (color)
	{
		struct cg_state_t* state = ctx->state;
		uint32_t solid = premultiply_color(color, state->opacity);
		if ((CG_ALPHA(solid) == 255) && (state->op == CG_OPERATOR_SRC_OVER))
			blend_solid(ctx->surface, CG_OPERATOR_SRC, rle, solid);
		else
			blend_solid(ctx->surface, state->op, rle, solid);
	}
}

static inline void cg_blend_gradient(struct cg_ctx_t* ctx, struct cg_rle_t* rle, struct cg_gradient_t* gradient)
{
	if (gradient && (gradient->stops.size > 0))
	{
		struct cg_state_t* state = ctx->state;
		struct cg_gradient_data_t data;
		int i, pos = 0, nstop = gradient->stops.size;
		struct cg_gradient_stop_t* curr, * next, * start, * last;
		uint32_t curr_color, next_color, last_color;
		uint32_t dist, idist;
		double delta, t, incr, fpos;
		double opacity = state->opacity * gradient->opacity;

		start = gradient->stops.data;
		curr = start;
		curr_color = combine_opacity(&curr->color, opacity);

		data.colortable[pos] = premultiply_pixel(curr_color);
		++pos;
		incr = 1.0 / 1024;
		fpos = 1.5 * incr;

		while (fpos <= curr->offset)
		{
			data.colortable[pos] = data.colortable[pos - 1];
			++pos;
			fpos += incr;
		}
		for (i = 0; i < nstop - 1; i++)
		{
			curr = (start + i);
			next = (start + i + 1);
			delta = 1.0 / (next->offset - curr->offset);
			next_color = combine_opacity(&next->color, opacity);
			while (fpos < next->offset && pos < 1024)
			{
				t = (fpos - curr->offset) * delta;
				dist = (uint32_t)(255 * t);
				idist = 255 - dist;
				data.colortable[pos] = premultiply_pixel(interpolate_pixel(curr_color, idist, next_color, dist));
				++pos;
				fpos += incr;
			}
			curr_color = next_color;
		}

		last = start + nstop - 1;
		last_color = premultiply_color(&last->color, opacity);
		for (; pos < 1024; ++pos)
			data.colortable[pos] = last_color;

		data.spread = gradient->spread;
		data.matrix = gradient->matrix;
		cg_matrix_multiply(&data.matrix, &data.matrix, &state->matrix);
		cg_matrix_invert(&data.matrix);

		if (gradient->type == CG_GRADIENT_TYPE_LINEAR)
		{
			data.linear.x1 = gradient->values[0];
			data.linear.y1 = gradient->values[1];
			data.linear.x2 = gradient->values[2];
			data.linear.y2 = gradient->values[3];
			blend_linear_gradient(ctx->surface, state->op, rle, &data);
		}
		else
		{
			data.radial.cx = gradient->values[0];
			data.radial.cy = gradient->values[1];
			data.radial.cr = gradient->values[2];
			data.radial.fx = gradient->values[3];
			data.radial.fy = gradient->values[4];
			data.radial.fr = gradient->values[5];
			blend_radial_gradient(ctx->surface, state->op, rle, &data);
		}
	}
}

static inline void cg_blend_texture(struct cg_ctx_t* ctx, struct cg_rle_t* rle, struct cg_texture_t* texture)
{
	if (texture)
	{
		struct cg_state_t* state = ctx->state;
		struct cg_texture_data_t data;
		data.width = texture->surface->width;
		data.height = texture->surface->height;
		data.stride = texture->surface->stride;
		data.alpha = (int)(state->opacity * texture->opacity * 256.0);
		data.pixels = texture->surface->pixels;
		data.matrix = texture->matrix;
		cg_matrix_multiply(&data.matrix, &data.matrix, &state->matrix);
		cg_matrix_invert(&data.matrix);
		struct cg_matrix_t* m = &data.matrix;
		if ((m->a == 1.0) && (m->b == 0.0) && (m->c == 0.0) && (m->d == 1.0))
		{
			if (texture->type == CG_TEXTURE_TYPE_PLAIN)
				blend_untransformed_argb(ctx->surface, state->op, rle, &data);
			else
				blend_untransformed_tiled_argb(ctx->surface, state->op, rle, &data);
		}
		else
		{
			if (texture->type == CG_TEXTURE_TYPE_PLAIN)
				blend_transformed_argb(ctx->surface, state->op, rle, &data);
			else
				blend_transformed_tiled_argb(ctx->surface, state->op, rle, &data);
		}
	}
}

static void cg_blend(struct cg_ctx_t* ctx, struct cg_rle_t* rle)
{
	if (rle && (rle->spans.size > 0))
	{
		struct cg_paint_t* source = &ctx->state->paint;
		switch (source->type)
		{
		case CG_PAINT_TYPE_COLOR:
			cg_blend_color(ctx, rle, &source->color);
			break;
		case CG_PAINT_TYPE_GRADIENT:
			cg_blend_gradient(ctx, rle, &source->gradient);
			break;
		case CG_PAINT_TYPE_TEXTURE:
			cg_blend_texture(ctx, rle, &source->texture);
		default:
			break;
		}
	}
}

static struct cg_state_t* cg_state_create(void)
{
	struct cg_state_t* state = (cg_state_t*)malloc(sizeof(struct cg_state_t));
	state->clippath = NULL;
	cg_paint_init(&state->paint);
	cg_matrix_init_identity(&state->matrix);
	state->winding = CG_FILL_RULE_NON_ZERO;
	state->stroke.width = 1.0;
	state->stroke.miterlimit = 10.0;
	state->stroke.cap = CG_LINE_CAP_BUTT;
	state->stroke.join = CG_LINE_JOIN_MITER;
	state->stroke.dash = NULL;
	state->op = CG_OPERATOR_SRC_OVER;
	state->opacity = 1.0;
	state->next = NULL;
	return state;
}

static struct cg_state_t* cg_state_clone(struct cg_state_t* state)
{
	struct cg_state_t* newstate = cg_state_create();
	newstate->clippath = cg_rle_clone(state->clippath);
	cg_paint_copy(&newstate->paint, &state->paint);
	newstate->matrix = state->matrix;
	newstate->winding = state->winding;
	newstate->stroke.width = state->stroke.width;
	newstate->stroke.miterlimit = state->stroke.miterlimit;
	newstate->stroke.cap = state->stroke.cap;
	newstate->stroke.join = state->stroke.join;
	newstate->stroke.dash = cg_dash_clone(state->stroke.dash);
	newstate->op = state->op;
	newstate->opacity = state->opacity;
	newstate->next = NULL;
	return newstate;
}

static void cg_state_destroy(struct cg_state_t* state)
{
	cg_rle_destroy(state->clippath);
	cg_paint_destroy(&state->paint);
	cg_dash_destroy(state->stroke.dash);
	free(state);
}

struct cg_ctx_t* cg_create(struct cg_surface_t* surface)
{
	struct cg_ctx_t* ctx = (cg_ctx_t*)malloc(sizeof(struct cg_ctx_t));
	ctx->surface = cg_surface_reference(surface);
	ctx->state = cg_state_create();
	ctx->path = cg_path_create();
	ctx->rle = cg_rle_create();
	ctx->clippath = NULL;
	ctx->clip.x = 0.0;
	ctx->clip.y = 0.0;
	ctx->clip.w = surface->width;
	ctx->clip.h = surface->height;
	ctx->outline_data = NULL;
	ctx->outline_size = 0;
	return ctx;
}

void cg_destroy(struct cg_ctx_t* ctx)
{
	if (ctx)
	{
		while (ctx->state)
		{
			struct cg_state_t* state = ctx->state;
			ctx->state = state->next;
			cg_state_destroy(state);
		}
		cg_surface_destroy(ctx->surface);
		cg_path_destroy(ctx->path);
		cg_rle_destroy(ctx->rle);
		cg_rle_destroy(ctx->clippath);
		if (ctx->outline_data)
			free(ctx->outline_data);
		free(ctx);
	}
}

void cg_save(struct cg_ctx_t* ctx)
{
	struct cg_state_t* state = cg_state_clone(ctx->state);
	state->next = ctx->state;
	ctx->state = state;
}

void cg_restore(struct cg_ctx_t* ctx)
{
	struct cg_state_t* state = ctx->state;
	ctx->state = state->next;
	cg_state_destroy(state);
}

struct cg_color_t* cg_set_source_rgb(struct cg_ctx_t* ctx, double r, double g, double b)
{
	return cg_set_source_rgba(ctx, r, g, b, 1.0);
}

struct cg_color_t* cg_set_source_rgba(struct cg_ctx_t* ctx, double r, double g, double b, double a)
{
	struct cg_paint_t* paint = &ctx->state->paint;
	paint->type = CG_PAINT_TYPE_COLOR;
	cg_color_init_rgba(&paint->color, r, g, b, a);
	return &paint->color;
}

struct cg_color_t* cg_set_source_color(struct cg_ctx_t* ctx, struct cg_color_t* color)
{
	return cg_set_source_rgba(ctx, color->r, color->g, color->b, color->a);
}

struct cg_gradient_t* cg_set_source_linear_gradient(struct cg_ctx_t* ctx, double x1, double y1, double x2, double y2)
{
	struct cg_paint_t* paint = &ctx->state->paint;
	paint->type = CG_PAINT_TYPE_GRADIENT;
	cg_gradient_init_linear(&paint->gradient, x1, y1, x2, y2);
	return &paint->gradient;
}

struct cg_gradient_t* cg_set_source_radial_gradient(struct cg_ctx_t* ctx, double cx, double cy, double cr, double fx, double fy, double fr)
{
	struct cg_paint_t* paint = &ctx->state->paint;
	paint->type = CG_PAINT_TYPE_GRADIENT;
	cg_gradient_init_radial(&paint->gradient, cx, cy, cr, fx, fy, fr);
	return &paint->gradient;
}

static inline struct cg_texture_t* cg_set_texture(struct cg_ctx_t* ctx, struct cg_surface_t* surface, enum cg_texture_type_t type)
{
	struct cg_paint_t* paint = &ctx->state->paint;
	paint->type = CG_PAINT_TYPE_TEXTURE;
	cg_texture_init(&paint->texture, surface, type);
	return &paint->texture;
}

struct cg_texture_t* cg_set_source_surface(struct cg_ctx_t* ctx, struct cg_surface_t* surface, double x, double y)
{
	struct cg_texture_t* texture = cg_set_texture(ctx, surface, CG_TEXTURE_TYPE_PLAIN);
	cg_matrix_init_translate(&texture->matrix, x, y);
	return texture;
}

void cg_set_operator(struct cg_ctx_t* ctx, enum cg_operator_t op)
{
	ctx->state->op = op;
}

void cg_set_opacity(struct cg_ctx_t* ctx, double opacity)
{
	ctx->state->opacity = glm::clamp(opacity, 0.0, 1.0);
}

void cg_set_fill_rule(struct cg_ctx_t* ctx, enum cg_fill_rule_t winding)
{
	ctx->state->winding = winding;
}

void cg_set_line_width(struct cg_ctx_t* ctx, double width)
{
	ctx->state->stroke.width = width;
}

void cg_set_line_cap(struct cg_ctx_t* ctx, enum cg_line_cap_t cap)
{
	ctx->state->stroke.cap = cap;
}

void cg_set_line_join(struct cg_ctx_t* ctx, enum cg_line_join_t join)
{
	ctx->state->stroke.join = join;
}

void cg_set_miter_limit(struct cg_ctx_t* ctx, double limit)
{
	ctx->state->stroke.miterlimit = limit;
}

void cg_set_dash(struct cg_ctx_t* ctx, double* dashes, int ndash, double offset)
{
	cg_dash_destroy(ctx->state->stroke.dash);
	ctx->state->stroke.dash = cg_dash_create(dashes, ndash, offset);
}

void cg_translate(struct cg_ctx_t* ctx, double tx, double ty)
{
	cg_matrix_translate(&ctx->state->matrix, tx, ty);
}

void cg_scale(struct cg_ctx_t* ctx, double sx, double sy)
{
	cg_matrix_scale(&ctx->state->matrix, sx, sy);
}

void cg_rotate(struct cg_ctx_t* ctx, double r)
{
	cg_matrix_rotate(&ctx->state->matrix, r);
}

void cg_transform(struct cg_ctx_t* ctx, struct cg_matrix_t* m)
{
	cg_matrix_multiply(&ctx->state->matrix, m, &ctx->state->matrix);
}

void cg_set_matrix(struct cg_ctx_t* ctx, struct cg_matrix_t* m)
{
	memcpy(&ctx->state->matrix, m, sizeof(struct cg_matrix_t));
}

void cg_identity_matrix(struct cg_ctx_t* ctx)
{
	cg_matrix_init_identity(&ctx->state->matrix);
}

void cg_move_to(struct cg_ctx_t* ctx, double x, double y)
{
	cg_path_move_to(ctx->path, x, y);
}

void cg_line_to(struct cg_ctx_t* ctx, double x, double y)
{
	cg_path_line_to(ctx->path, x, y);
}

void cg_curve_to(struct cg_ctx_t* ctx, double x1, double y1, double x2, double y2, double x3, double y3)
{
	cg_path_curve_to(ctx->path, x1, y1, x2, y2, x3, y3);
}

void cg_quad_to(struct cg_ctx_t* ctx, double x1, double y1, double x2, double y2)
{
	cg_path_quad_to(ctx->path, x1, y1, x2, y2);
}

void cg_rel_move_to(struct cg_ctx_t* ctx, double dx, double dy)
{
	cg_path_rel_move_to(ctx->path, dx, dy);
}

void cg_rel_line_to(struct cg_ctx_t* ctx, double dx, double dy)
{
	cg_path_rel_line_to(ctx->path, dx, dy);
}

void cg_rel_curve_to(struct cg_ctx_t* ctx, double dx1, double dy1, double dx2, double dy2, double dx3, double dy3)
{
	cg_path_rel_curve_to(ctx->path, dx1, dy1, dx2, dy2, dx3, dy3);
}

void cg_rel_quad_to(struct cg_ctx_t* ctx, double dx1, double dy1, double dx2, double dy2)
{
	cg_path_rel_quad_to(ctx->path, dx1, dy1, dx2, dy2);
}

void cg_rectangle(struct cg_ctx_t* ctx, double x, double y, double w, double h)
{
	cg_path_add_rectangle(ctx->path, x, y, w, h);
}

void cg_round_rectangle(struct cg_ctx_t* ctx, double x, double y, double w, double h, double rx, double ry)
{
	cg_path_add_round_rectangle(ctx->path, x, y, w, h, rx, ry);
}

void cg_ellipse(struct cg_ctx_t* ctx, double cx, double cy, double rx, double ry)
{
	cg_path_add_ellipse(ctx->path, cx, cy, rx, ry);
}

void cg_circle(struct cg_ctx_t* ctx, double cx, double cy, double r)
{
	cg_path_add_ellipse(ctx->path, cx, cy, r, r);
}

void cg_arc(struct cg_ctx_t* ctx, double cx, double cy, double r, double a0, double a1)
{
	cg_path_add_arc(ctx->path, cx, cy, r, a0, a1, 0);
}

void cg_arc_negative(struct cg_ctx_t* ctx, double cx, double cy, double r, double a0, double a1)
{
	cg_path_add_arc(ctx->path, cx, cy, r, a0, a1, 1);
}

void cg_new_path(struct cg_ctx_t* ctx)
{
	cg_path_clear(ctx->path);
}

void cg_close_path(struct cg_ctx_t* ctx)
{
	cg_path_close(ctx->path);
}

void cg_reset_clip(struct cg_ctx_t* ctx)
{
	cg_rle_destroy(ctx->state->clippath);
	ctx->state->clippath = NULL;
}

void cg_clip(struct cg_ctx_t* ctx)
{
	cg_clip_preserve(ctx);
	cg_new_path(ctx);
}

void cg_clip_preserve(struct cg_ctx_t* ctx)
{
	struct cg_state_t* state = ctx->state;
	if (state->clippath)
	{
		cg_rle_clear(ctx->rle);
		cg_rle_rasterize(ctx, ctx->rle, ctx->path, &state->matrix, &ctx->clip, NULL, state->winding);
		cg_rle_clip_path(state->clippath, ctx->rle);
	}
	else
	{
		state->clippath = cg_rle_create();
		cg_rle_rasterize(ctx, state->clippath, ctx->path, &state->matrix, &ctx->clip, NULL, state->winding);
	}
}

void cg_fill(struct cg_ctx_t* ctx)
{
	cg_fill_preserve(ctx);
	cg_new_path(ctx);
}

void cg_fill_preserve(struct cg_ctx_t* ctx)
{
	struct cg_state_t* state = ctx->state;
	cg_rle_clear(ctx->rle);
	cg_rle_rasterize(ctx, ctx->rle, ctx->path, &state->matrix, &ctx->clip, NULL, state->winding);
	cg_rle_clip_path(ctx->rle, state->clippath);
	cg_blend(ctx, ctx->rle);
}

void cg_stroke(struct cg_ctx_t* ctx)
{
	cg_stroke_preserve(ctx);
	cg_new_path(ctx);
}

void cg_stroke_preserve(struct cg_ctx_t* ctx)
{
	struct cg_state_t* state = ctx->state;
	cg_rle_clear(ctx->rle);
	cg_rle_rasterize(ctx, ctx->rle, ctx->path, &state->matrix, &ctx->clip, &state->stroke, CG_FILL_RULE_NON_ZERO);
	cg_rle_clip_path(ctx->rle, state->clippath);
	cg_blend(ctx, ctx->rle);
}

void cg_paint(struct cg_ctx_t* ctx)
{
	struct cg_state_t* state = ctx->state;
	if ((state->clippath == NULL) && (ctx->clippath == NULL))
	{
		struct cg_path_t* path = cg_path_create();
		cg_path_add_rectangle(path, ctx->clip.x, ctx->clip.y, ctx->clip.w, ctx->clip.h);
		struct cg_matrix_t m;
		cg_matrix_init_identity(&m);
		ctx->clippath = cg_rle_create();
		cg_rle_rasterize(ctx, ctx->clippath, path, &m, &ctx->clip, NULL, CG_FILL_RULE_NON_ZERO);
		cg_path_destroy(path);
	}
	struct cg_rle_t* rle = state->clippath ? state->clippath : ctx->clippath;
	cg_blend(ctx, rle);
}

#endif // no_stb2d


