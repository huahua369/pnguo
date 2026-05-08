
#include "pch1.h"
#ifdef _cairoh_ 
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
	//#include <pango/pango-layout.h>
	//#include <pango/pangocairo.h>

#ifdef __cplusplus
}
#endif
#endif

#include <SDL3/SDL_keycode.h>
#include <mapView.h>
#include <event.h>

#include <stb_image_write.h>

#include "pnguo.h"
#include "gui.h"  
#include "render.h"
#ifdef min
#undef min
#undef max
#endif // min

#if 1

// 图集数据

atlas_cx::atlas_cx()
{}

atlas_cx::~atlas_cx()
{}
void atlas_cx::add(image_rc_t* d, size_t count)
{
	if (d && count > 0)
	{
		_imgv.reserve(_imgv.size() + count);
		for (size_t i = 0; i < count; i++)
		{
			auto it = d[i];
			image_sliced_t dt = {};
			dt.img_rc = it.img_rc;
			dt.tex_rc = it.tex_rc;
			dt.color = it.color;
			_imgv.push_back(dt);
		}
	}
}
void atlas_cx::add(image_sliced_t* d, size_t count) {

	if (d && count > 0)
	{
		_imgv.reserve(_imgv.size() + count);
		for (size_t i = 0; i < count; i++)
		{
			auto dt = d[i];
			_imgv.push_back(dt);
		}
	}
}
void atlas_cx::add(const glm::ivec4& rc, const glm::ivec4& texrc, const glm::ivec4& sliced, uint32_t color) {

	image_sliced_t dt = {};
	dt.img_rc = rc;
	dt.tex_rc = texrc;
	dt.color = color;
	dt.sliced = sliced;
	_imgv.push_back(dt);
}
void atlas_cx::clear() {
	_imgv.clear();
}

#if 1
gshadow_cx::gshadow_cx()
{}

gshadow_cx::~gshadow_cx()
{}

image_sliced_t gshadow_cx::new_rect(const rect_shadow_t& rs)
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


#if 0
// todo 字体纹理缓存管理
class layout_text_x
{
public:
	font_rctx* ctx = 0;
	std::vector<std::vector<font_t*>> familyv;
	std::vector<glm::ivec3> cfb;
	int fdpi = 72;
	int heightline = 0;		// 固定行高
	text_path_t ctp = {};	// 临时缓存
	text_image_t cti = {};
	std::vector<d2_surface_t*> msu;
	std::vector<font_item_t> tv;
	std::vector<font_item_t> tem_rtv;	// 临时缓存用
	// todo
	std::vector<atlas_cx> tem_iptr;
	std::vector<float> tem_pv;
	glm::ivec2 ctrc = {}, oldrc = {};

	image_sliced_t sli = {};
	int sli_radius = 10;
	gshadow_cx* gs = 0;
	// 菜单边框填充：线颜色，线粗，圆角，背景色
	glm::ivec4 m_color = { 0xff606060,1,0,0xf0121212 };
	// 菜单项偏移
	glm::ivec2 m_cpos = { 3, 3 };

	bitmap_cache_cx* bc_ctx = 0;  //纹理缓存
	std::vector<font_t*> _t1;
	std::vector<uint32_t> s32;
public:
	layout_text_x();
	~layout_text_x();
	void set_ctx(font_rctx* p);
	// 添加字体,返回序号
	size_t add_familys(const char* familys, const char* style);
	void cpy_familys(layout_text_x* p);
	void clear_family();
	void clear_text();
	// 创建新缓存
	bitmap_cache_cx* new_cache(const glm::ivec2& vsize);
	void free_cache(bitmap_cache_cx* p);
	//text_dta* new_text_dta(size_t idx, int fontsize, const void* str8, int len, text_dta* old = 0);
	//text_dta* new_text_dta1(font_t* p, int fontsize, const void* str8, int len, text_dta* old = 0);
	// 获取基线
	int get_baseline(size_t idx, int fontsize);
	// 获取行高
	int get_lineheight(size_t idx, int fontsize);
	// 获取文本区域大小,z为基线
	glm::ivec3 get_text_rect(size_t idx, int fontsize, const void* str8, int len);
	glm::ivec3 get_text_rect1(size_t idx, int fontsize, const void* str8);
	int get_text_pos(size_t idx, int fontsize, const void* str8, int len, int xpos);
	int get_text_ipos(size_t idx, int fontsize, const void* str8, int len, int ipos);
	int get_text_posv(size_t idx, int fontsize, const void* str8, int len, std::vector<std::vector<int>>& ow);
	// 添加文本到渲染
	glm::ivec2 add_text(size_t idx, int fontsize, glm::vec4& rc, const glm::vec2& text_align, const void* str8, int len);
	glm::ivec2 build_text(size_t idx, int fontsize, glm::vec4& rc, const glm::vec2& text_align, const void* str8, int len, std::vector<font_item_t>& rtv);
	glm::ivec2 build_text1(font_t* p, int fontsize, glm::vec4& rc, const glm::vec2& text_align, const void* str8, int len, std::vector<font_item_t>& rtv);
	// 输出到图集
	void text2atlas(const glm::ivec2& r, uint32_t color, std::vector<atlas_cx>* opt);
	// 获取路径数据
	text_path_t* get_shape(size_t idx, int fontsize, const void* str8, text_path_t* opt, float scale = 1);
	// 获取渲染数据
	text_image_t* get_glyph_item(size_t idx, int fontsize, const void* str8, text_image_t* opt);
	text_image_t* get_glyph_item1(font_t* p, int fontsize, const void* str8, text_image_t* opt);
	// 渲染部分文本
#if 0
	void draw_text(rvg_cx* rv, const glm::ivec2& r, uint32_t color);
	void draw_text(rvg_cx* rv, const std::vector<font_item_t>& r, uint32_t color);

	void draw_rect_rc(rvg_cx* rv, const std::vector<font_item_t>& rtv, uint32_t color);
	// 渲染全部文本
	void draw_text(rvg_cx* rv, uint32_t color);
#endif
	// todo获取图集
	atlas_t* get_atlas();
	bool update_text();
	// 创建阴影
	atlas_cx* new_shadow(const glm::ivec2& ss, const glm::ivec2& pos);
	// 创建菜单
	pvm_t new_menu(int width, int height, const std::vector<std::string>& v, bool has_shadow, std::function<void(int type, int id)> cb);
	pvm_t new_menu(int width, int height, const char** v, size_t n, bool has_shadow, std::function<void(int type, int id)> cb);
	void free_menu(pvm_t pt);
private:
	void c_line_metrics(size_t idx, int fontsize);
};
layout_text_x::layout_text_x()
{
	gs = new gshadow_cx();
}

layout_text_x::~layout_text_x()
{
	if (gs)delete gs; gs = 0;
	for (auto it : msu) {
#ifdef _CR__
		free_image_cr(it);
#endif
	}
	free_cache(bc_ctx);
}

void layout_text_x::set_ctx(font_rctx* p)
{
	if (p)
	{
		ctx = p;
	}
}

size_t layout_text_x::add_familys(const char* familys, const char* style) {
	if (!ctx)return 0;
	assert(ctx);
	size_t rs = 0;
	if (ctx && familys && *familys)
	{
		std::vector<std::string> result;
		split_v(familys, ",", result);
		std::vector<font_t*> v;
		for (auto& it : result)
		{
			auto ft = ctx->get_font(it.c_str(), style);
			if (ft)
			{
				v.push_back(ft);
			}
		}
		if (v.size())
		{
			rs = familyv.size();
			familyv.push_back(v);
			cfb.push_back({});
		}
	}
	return rs;
}
void layout_text_x::cpy_familys(layout_text_x* p)
{
	if (p)
	{
		familyv = p->familyv;
		cfb = p->cfb;
	}
}
void layout_text_x::clear_family()
{
	familyv.clear();
	cfb.clear();
}
void layout_text_x::clear_text()
{
	tv.clear();
}

bitmap_cache_cx* layout_text_x::new_cache(const glm::ivec2& vsize)
{
	auto p = new bitmap_cache_cx();
	if (p)
	{
		p->resize(vsize.x, vsize.y);
		if (!bc_ctx)
			bc_ctx = p;
	}
	return p;
}

void layout_text_x::free_cache(bitmap_cache_cx* p)
{
	if (p)
	{
		delete p;
	}
}
//
//text_dta* layout_text_x::new_text_dta(size_t idx, int fontsize, const void* str8, int len, text_dta* old)
//{
//	if (!old)
//	{
//		old = new text_dta();
//	}
//	old->ltx = this;
//	auto str = (const char*)str8;
//	if (str8)
//	{
//		if (idx != old->idx)
//		{
//			old->idx = idx;
//		}
//		if (fontsize != old->fontsize)
//		{
//			old->fontsize = fontsize;
//		}
//		old->tv.clear();
//		auto nrc = build_text(idx, fontsize, old->rc, old->text_align, str8, len, old->tv);
//	}
//	return old;
//}
//
//text_dta* layout_text_x::new_text_dta1(font_t* p, int fontsize, const void* str8, int len, text_dta* old)
//{
//	if (!old)
//	{
//		old = new text_dta();
//	}
//	old->ltx = this;
//	auto str = (const char*)str8;
//	if (str8)
//	{
//		if (fontsize != old->fontsize)
//		{
//			old->fontsize = fontsize;
//		}
//		old->tv.clear();
//		auto nrc = build_text1(p, fontsize, old->rc, old->text_align, str8, len, old->tv);
//	}
//	return old;
//}

void layout_text_x::c_line_metrics(size_t idx, int fontsize) {
	if (idx >= familyv.size())idx = 0;
	if (fontsize == 0 || idx >= cfb.size() || familyv.empty())return;
	if (cfb[idx].z != fontsize)
	{
		glm::dvec2 r = {};
		auto& v = familyv[idx];
		for (auto it : v)
		{
			double scale = fontsize == 0 ? 1.0 : it->get_scale(fontsize);
			r.x = std::max(it->ascender * scale, r.x);
			r.y = std::max((it->ascender - it->descender + it->lineGap) * scale, r.y);
		}
		glm::ivec3 c = { r,fontsize };
		cfb[idx] = c;
	}
}
int layout_text_x::get_baseline(size_t idx, int fontsize)
{
	if (familyv.empty())return 0;
	if (idx >= familyv.size())idx = 0;
	c_line_metrics(idx, fontsize);
	return cfb[idx].x;
}

int layout_text_x::get_lineheight(size_t idx, int fontsize)
{
	if (familyv.empty())return 0;
	if (idx >= familyv.size())idx = 0;
	c_line_metrics(idx, fontsize);
	return heightline ? heightline : cfb[idx].y;
}

glm::ivec3 layout_text_x::get_text_rect(size_t idx, int fontsize, const void* str8, int len)
{
	glm::ivec3 ret = {};
	if (familyv.empty())
		return ret;
	auto str = (const char*)str8;
	if (idx >= familyv.size())idx = 0;
	auto font = familyv[idx][0];
	int x = 0;
	int y = 0;
	int n = 1;
	int lineheight = 0;// get_lineheight(idx, fontsize);

	font_t* oft0 = 0;
	do
	{
		if (!str || !(*str)) { break; }
		uint32_t ch = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
		//str = md::get_u8_last(str, &ch);
		if (ch == '\n')
		{
			ret.x = std::max(ret.x, x);
			x = 0;
			n++;
			continue;
		}
		font_t* oft = 0;
		auto rc = font->get_char_extent(ch, fontsize,/* fdpi,*/ &familyv[idx], &oft);
		if (oft != oft0 && oft) {
			oft0 = oft;
			double scale = fontsize == 0 ? 1.0 : oft->get_scale(fontsize);
			lineheight = std::max((int)((oft->ascender - oft->descender + oft->lineGap) * scale), lineheight);
		}

		x += rc.z;
		y = std::max(rc.y, y);
		ret.y = std::max(ret.y, y);
		ret.z = std::max(ret.z, rc.w);
	} while (str && *str);
	ret.x = std::max(ret.x, x);
	if (heightline > 0)
		lineheight = heightline;
	if (n > 1)
		ret.y = lineheight * n;
	else
		ret.y = get_lineheight(idx, fontsize);
	return ret;
}
glm::ivec3 layout_text_x::get_text_rect1(size_t idx, int fontsize, const void* str8)
{
	glm::ivec3 ret = {};
	if (familyv.empty())
		return ret;
	auto str = (const char*)str8;
	if (idx >= familyv.size())idx = 0;
	auto font = familyv[idx][0];
	int x = 0;
	int y = 0;
	int n = 1;
	int lineheight = 0;// get_lineheight(idx, fontsize);

	font_t* oft0 = 0;
	do
	{
		if (!str || !(*str)) { break; }
		uint32_t ch = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
		if (ch == '\n')
		{
			ret.x = std::max(ret.x, x);
			x = 0;
			n++;
			break;
		}
		font_t* oft = 0;
		auto rc = font->get_char_extent(ch, fontsize,/* fdpi,*/ &familyv[idx], &oft);
		if (oft != oft0) {
			oft0 = oft;
			double scale = fontsize == 0 ? 1.0 : oft->get_scale(fontsize);
			//lineheight = std::max((int)(oft->ascender * scale), lineheight);
			lineheight = std::max((int)((oft->ascender - oft->descender + oft->lineGap) * scale), lineheight);
			ret.z = oft->ascender * scale;
		}
		x = rc.x;
		ret.y = rc.y;
		break;
	} while (0);// str&&* str);
	ret.x = x;
	return ret;
}

int layout_text_x::get_text_pos(size_t idx, int fontsize, const void* str8, int len, int xpos)
{
	int ret = {};
	if (familyv.empty())
		return ret;
	auto str = (const char*)str8;
	auto str0 = (const char*)str8;
	if (idx >= familyv.size())idx = 0;
	auto font = familyv[idx][0];
	int x = 0;
	double ktt = 0.6;
	tem_pv.clear();
	do
	{
		if (!str || !(*str)) { break; }
		auto pstr = str;
		uint32_t ch = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
		if (ch == '\n')
		{
			break;
		}
		font_t* oft = 0;
		auto rc = font->get_char_extent(ch, fontsize,/* fdpi,*/ &familyv[idx], &oft);
		x += rc.z;
		tem_pv.push_back(x);
	} while (str && *str);
	if (tem_pv.size()) {
		auto rt = std::lower_bound(tem_pv.begin(), tem_pv.end(), xpos);
		ret = (rt != tem_pv.end()) ? *rt : *tem_pv.rbegin();
		if (xpos < tem_pv[0])
		{
			ret = tem_pv[0];
		}
		for (size_t i = 0; i < tem_pv.size(); i++)
		{
			if (ret == tem_pv[i])
			{
				ret = i; break;
			}
		}
		if (ret > tem_pv.size() || xpos > *tem_pv.rbegin())
		{
			ret = tem_pv.size();
		}
	}
	return ret;
}
int layout_text_x::get_text_ipos(size_t idx, int fontsize, const void* str8, int len, int ipos)
{
	int ret = {};
	if (familyv.empty())
		return ret;
	auto str = (const char*)str8;
	auto str0 = str + ipos;
	if (idx >= familyv.size())idx = 0;
	auto font = familyv[idx][0];
	int x = 0;
	double ktt = 0.6;
	do
	{
		if (!str || !(*str) || str >= str0) { break; }

		uint32_t ch = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
		if (ch == '\n')
		{
			x = 0;
			break;
		}
		font_t* oft = 0;
		auto rc = font->get_char_extent(ch, fontsize,/* fdpi,*/ &familyv[idx], &oft);
		x += rc.z;
	} while (str && *str);
	return x;
}
int layout_text_x::get_text_posv(size_t idx, int fontsize, const void* str8, int len, std::vector<std::vector<int>>& ow)
{
	int ret = {};
	if (familyv.empty())
		return ret;
	auto str = (const char*)str8;
	auto str0 = (const char*)str8;
	if (idx >= familyv.size())idx = 0;
	auto font = familyv[idx][0];
	int x = 0;
	double ktt = 0.6;
	ow.clear();
	std::vector<int> w0;
	w0.push_back(0);
	do
	{
		if (!str || !(*str)) { break; }

		uint32_t ch = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
		if (ch == '\n')
		{
			ow.push_back(w0);
			w0.clear();
			w0.push_back(0); x = 0;
			continue;
		}
		font_t* oft = 0;
		auto rc = font->get_char_extent(ch, fontsize,/* fdpi,*/ &familyv[idx], &oft);
		x += rc.z;
		w0.push_back(x);
	} while (str && *str);
	if (w0.size())ow.push_back(w0);
	return ret;
}

glm::ivec2 layout_text_x::add_text(size_t idx, int fontsize, glm::vec4& rc, const glm::vec2& text_align, const void* str8, int len)
{
	return build_text(idx, fontsize, rc, text_align, str8, len, tv);
}
glm::ivec2 layout_text_x::build_text(size_t idx, int fontsize, glm::vec4& rc, const glm::vec2& text_align, const void* str8, int len, std::vector<font_item_t>& rtv)
{
	assert(fontsize < 800);
	glm::ivec2 ret = { rtv.size(), 0 };
	if (!ctx) { return ret; }
	text_image_t* p = 0;
	cti.tv.clear();
	if (idx >= familyv.size())idx = 0;
	if (len > 0)
	{
		std::string str((char*)str8, len);
		p = get_glyph_item(idx, fontsize, str.c_str(), &cti);
	}
	else
	{
		p = get_glyph_item(idx, fontsize, str8, &cti);
	}
	if (p)
	{
		auto rct0 = get_text_rect(idx, fontsize, str8, len);
		glm::vec2 rct = { rct0.x,rct0.y };
		auto length = p->tv.size();
		auto baseline = rct0.z;
		baseline = get_baseline(idx, fontsize);
		int h = get_lineheight(idx, fontsize);
		if (rc.z < 1)rc.z = rct.x;
		if (rc.w < 1)rc.w = h;
		if (ctrc.y < h)ctrc.y = h;
		auto ta = text_align;
		if (ta.x < 0)ta.x = 0;
		if (ta.y < 0)ta.y = 0;
		glm::vec2 ss = { rc.z,rc.w }, bearing = { 0, baseline };
		// 区域大小 - 文本包围盒大小。居中就是text_align={0.5,0.5}
		auto ps = (ss - rct) * ta + bearing;
		ps.x += rc.x;
		ps.y += rc.y;
		glm::vec2 tps = {};
		rtv.reserve(rtv.size() + length);
		for (size_t i = 0; i < length; i++)
		{
			auto& it = p->tv[i];
			if (it.cpt == '\n')
			{
				tps.y += h;
				tps.x = 0;
			}
			if (it.cpt == '\t')
			{
				it.advance = fontsize;
			}
			it._apos = ps + tps;
			tps.x += it.advance;
			if (ctrc.x < it.advance)ctrc.x = it.advance;
			rtv.push_back(it);
		}
		ret.y = p->tv.size();
	}
	return ret;
}
glm::ivec2 layout_text_x::build_text1(font_t* fp, int fontsize, glm::vec4& rc, const glm::vec2& text_align, const void* str8, int len, std::vector<font_item_t>& rtv)
{
	assert(fontsize < 800);
	glm::ivec2 ret = { rtv.size(), 0 };
	if (!ctx) { return ret; }
	text_image_t* p = 0;
	cti.tv.clear();
	if (len > 0)
	{
		std::string str((char*)str8, len);
		p = get_glyph_item1(fp, fontsize, str.c_str(), &cti);
	}
	else
	{
		p = get_glyph_item1(fp, fontsize, str8, &cti);
	}
	if (p)
	{
		auto rct0 = fp->get_text_rect(fontsize, str8, len);
		glm::vec2 rct = { rct0.x,rct0.y };
		auto length = p->tv.size();
		auto baseline = rct0.z;
		baseline = fp->get_base_line(fontsize);// get_baseline(idx, fontsize);
		int h = fp->get_line_height(fontsize);
		if (rc.z < 1)rc.z = rct.x;
		if (rc.w < 1)rc.w = h;
		if (ctrc.y < h)ctrc.y = h;
		auto ta = text_align;
		if (ta.x < 0)ta.x = 0;
		if (ta.y < 0)ta.y = 0;
		glm::vec2 ss = { rc.z,rc.w }, bearing = { 0, baseline };
		// 区域大小 - 文本包围盒大小。居中就是text_align={0.5,0.5}
		auto ps = (ss - rct) * ta + bearing;
		ps.x += rc.x;
		ps.y += rc.y;
		glm::vec2 tps = {};
		rtv.reserve(rtv.size() + length);
		for (size_t i = 0; i < length; i++)
		{
			auto& it = p->tv[i];
			if (it.cpt == '\n')
			{
				tps.y += h;
				tps.x = 0;
			}
			if (it.cpt == '\t')
			{
				it.advance = fontsize;
			}
			it._apos = ps + tps;
			tps.x += it.advance;
			if (ctrc.x < it.advance)ctrc.x = it.advance;
			rtv.push_back(it);
		}
		ret.y = p->tv.size();
	}
	return ret;
}

atlas_t* layout_text_x::get_atlas()
{
	auto ft = ctx->bcc._data.data();
	auto n = ctx->bcc._data.size();
	tem_iptr.clear();
	tem_iptr.resize(n);
	for (size_t i = 0; i < n; i++)
	{
		auto& rt = tem_iptr[i];
		auto p = ft[i];
		rt.img = p;
	}
	for (auto& it : tv) {
		it._image;
	}
	{

		image_ptr_t* img = 0;
		glm::ivec4* img_rc = 0;	// 显示坐标、大小
		glm::ivec4* tex_rc = 0;	// 纹理区域
		glm::ivec4* sliced = 0;	// 九宫格渲染
		uint32_t* colors = 0;	// 颜色混合/透明度
		size_t count = 0;		// 数量
		glm::ivec4 clip = {};	// 裁剪区域
	};
	return nullptr;
}
bool layout_text_x::update_text()
{
	bool r = false;
	if (!ctx)return r;
	auto ft = ctx->bcc._data.data();
	auto n = ctx->bcc._data.size();
	for (size_t i = 0; i < n; i++)
	{
		auto p = ft[i];
		if (p->ptr)
		{
			if (p->valid)
			{
#ifdef _CR__
				update_image_cr((cairo_surface_t*)p->ptr, p);
#endif
				p->valid = 0; r = true;
			}
		}
		else {
#ifdef _CR__
			cairo_surface_t* su = new_image_cr(p);
			if (su)
			{
				msu.push_back(su);
				p->valid = 0; r = true;
				p->ptr = su;
			}
#endif
		}
	}
	return r;
}
atlas_cx* layout_text_x::new_shadow(const glm::ivec2& ss, const glm::ivec2& pos)
{
	if (sli.tex_rc.x < sli_radius)
	{
		rect_shadow_t rs = {};
		rs.cfrom = { 0.00,0.00,0.0,1 }, rs.cto = { 0.9,0.9,0.9,1 };
		rs.radius = sli_radius;
		rs.segment = 8;
		rs.cubic = { {0.0,0.66},{0.5,0.39},{0.4,0.1},{1.0,0.01 } };
		sli = gs->new_rect(rs);
	}
	auto rcs = sli;
	auto a = new atlas_cx();
	a->img = gs->img;
	a->img->type = 1;
	a->autofree = true;
	rcs.img_rc = { pos.x,pos.y,ss.x,ss.y };
	rcs.img_rc.x = rcs.img_rc.y = 0;
	a->add(&rcs, 1);
	//can->add_atlas(a);
	return a;
}

pvm_t layout_text_x::new_menu(int width, int height, const std::vector<std::string>& v, bool has_shadow, std::function<void(int type, int id)> cb) {
	std::vector<const char*> vs;
	vs.resize(v.size());
	for (size_t i = 0; i < v.size(); i++)
	{
		vs[i] = v[i].c_str();
	}
	return new_menu(width, height, vs.data(), v.size(), has_shadow, cb);
}
pvm_t layout_text_x::new_menu(int width, int height, const char** v, size_t n, bool has_shadow, std::function<void(int type, int id)> cb)
{
	pvm_t ret = {};
	auto ltx = this;
	//auto p = new plane_cx();
	//if (p && n > 0)
	//{
	//	p->fontsize = 16;
	//	int lheight = height > 0 ? height : ltx->get_lineheight(0, p->fontsize) * 1.5;
	//	if (width < 0)
	//	{
	//		int xw = lheight;
	//		for (size_t i = 0; i < n; i++)
	//		{
	//			auto it = v[i];
	//			auto rc = ltx->get_text_rect(0, p->fontsize, it, -1);
	//			if (xw < rc.x) {
	//				xw = rc.x;
	//			}
	//		}
	//		width = xw + lheight;
	//	}
	//	ret.w = width;
	//	ret.h = lheight;
	//	ret.cpos = m_cpos;
	//	glm::ivec2 iss = { width , lheight };
	//	p->set_color(m_color);
	//	glm::ivec2 ss = { width + p->border.y * 7, n * lheight + p->border.y * 7 };

	//	auto radius = ltx->sli_radius;
	//	glm::ivec2 sas = {};
	//	if (has_shadow)
	//	{
	//		sas += radius;
	//		auto ass = ss + radius;
	//		auto pa = ltx->new_shadow(ass, {});
	//		ret.back = pa;
	//	}
	//	p->_lpos = { 0,0 }; p->_lms = { 0,0 };
	//	p->custom_layout = true;
	//	p->set_fontctx(ltx->ctx);
	//	p->ltx->cpy_familys(ltx);
	//	for (size_t i = 0; i < n; i++)
	//	{
	//		auto it = v[i];
	//		auto pcb = p->add_cbutton(it, iss, 2);
	//		pcb->pos = { 0, i * lheight };
	//		pcb->pos += ret.cpos;
	//		pcb->light = 0.051;
	//		pcb->effect = uTheme::light;
	//		pcb->pdc.hover_border_color = pcb->pdc.border_color;
	//		pcb->pdc.border_color = 0;
	//		pcb->text_align = { 0.0,0.5 };
	//		if (pcb && cb)
	//		{
	//			pcb->click_cb = [=](void* pr, int)
	//				{
	//					cb(1, i);
	//					auto pc = (color_btn*)pr;
	//					if (pc)
	//						pc->_bst = (int)BTN_STATE::STATE_NOMAL;
	//				};
	//			pcb->mevent_cb = [=](void* p, int type, const glm::vec2& mps) {
	//				if (type == (int)event_type2::on_hover) {
	//					cb(0, i);
	//				}
	//				if (type == (int)event_type2::on_move) {
	//					cb(2, i);
	//				}
	//				};
	//		}
	//	}
	//	p->set_size(ss);
	//	p->set_pos({ radius * 0,radius * 0 });
	//	ret.p = p;
	//	ret.fsize = ss + sas;
	//}
	return ret;
}

void layout_text_x::free_menu(pvm_t pt)
{
	if (pt.p)
		delete pt.p;
	if (pt.back)
		delete pt.back;
}








#ifdef _CR__

void layout_text_x::draw_text(rvg_cx* rv, const glm::ivec2& r, uint32_t color)
{
	int mx = r.y + r.x;
	for (size_t i = r.x; i < mx; i++)
	{
		auto& it = tv[i];
		if (it._image)
		{
			auto ft = (cairo_surface_t*)it._image->ptr;
			if (ft)
			{
				draw_image(cr, ft, it._dwpos + it._apos, it._rect, it.color ? it.color : color);
			}
		}
	}
}
void layout_text_x::draw_text(rvg_cx* rv, const std::vector<font_item_t>& rtv, uint32_t color)
{
	auto mx = rtv.size();
	for (size_t i = 0; i < mx; i++)
	{
		auto& it = rtv[i];
		if (it._image)
		{
			auto ft = (cairo_surface_t*)it._image->ptr;
			if (ft)
			{
				draw_image(cr, ft, it._dwpos + it._apos, it._rect, it.color ? it.color : color);
			}
		}
	}
}
void layout_text_x::draw_rect_rc(rvg_cx* rv, const std::vector<font_item_t>& rtv, uint32_t color)
{
	auto mx = rtv.size();
	for (size_t i = 0; i < mx; i++)
	{
		auto& it = rtv[i];
		auto pos = it._dwpos + it._apos;
		glm::ivec4 rc = { pos.x,pos.y,it._rect.z,it._rect.w };
		draw_rect(cr, rc, color, 0, 0, 0);
	}
}
void layout_text_x::draw_text(rvg_cx* rv, uint32_t color)
{
	update_text();
	for (auto& it : tv)
	{
		if (it._image)
		{
			auto ft = (cairo_surface_t*)it._image->ptr;
			if (ft)
			{
				draw_image(cr, ft, it._dwpos + it._apos, it._rect, it.color ? it.color : color);
			}
		}
	}
}

#endif

void layout_text_x::text2atlas(const glm::ivec2& r, uint32_t color, std::vector<atlas_cx>* opt)
{
	if (opt && tv.size())
	{
		int mx = r.y + r.x;
		atlas_cx ac = {};
		ac.img = tv[r.x]._image;
		for (size_t i = r.x; i < mx; i++)
		{
			auto& it = tv[i];
			if (it._image == ac.img)
			{
				ac.add({ it._dwpos,it._rect.z,it._rect.w }, it._rect, {}, it.color ? it.color : color);
			}
			else {
				if (ac._imgv.size())
				{
					opt->push_back(ac);
					ac.clear();
				}
				ac.img = it._image;
				ac.add({ it._dwpos,it._rect.z,it._rect.w }, it._rect, {}, it.color ? it.color : color);
			}
		}
		if (ac._imgv.size())
		{
			opt->push_back(ac);
			ac.clear();
		}
	}

}


text_path_t* layout_text_x::get_shape(size_t idx, int fontsize, const void* str8, text_path_t* opt, float scale1)
{
	auto str = (const char*)str8;
	if (idx >= familyv.size())idx = 0;
	int adv = 0;
	do
	{
		if (!str || !(*str) || !opt) { opt = 0; break; }
		font_t* r = 0;
		int gidx = 0;
		auto ts = str;
		str = font_t::get_glyph_index_u8(str, &gidx, &r, &familyv[idx]);
		if (r && gidx >= 0)
		{
			auto k = r->get_shape(ts, fontsize, &opt->data, adv, scale1);
			adv += k.advance;
			if (k.count)
			{
				opt->tv.push_back(k);
				opt->baseline = k.baseline;
			}
		}
	} while (str && *str);
	if (opt) {
		auto pd = opt->data.data();
		for (size_t i = 0; i < opt->tv.size(); i++)
		{
			auto& it = opt->tv[i];
			it.v = pd + it.first;
		}
	}
	return opt;
}
// todo get_glyph_item

text_image_t* layout_text_x::get_glyph_item(size_t idx, int fontsize, const void* str8, text_image_t* opt)
{
	auto str = (const char*)str8;
	if (idx >= familyv.size())idx = 0;
	font_t* r = 0;
	do
	{
		if (!str || !(*str) || !opt) { opt = 0; break; }
		int gidx = 0;
		r = 0;
		if (*str == '\n')
			gidx = 0;
		auto ostr = str;

		uint32_t ch = 0;
		uint32_t ch1 = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
#if 0
		auto bstr = str;
		auto bstr1 = str;
		bstr1 += kk;
		int d2 = 0;
		s32.clear();
		s32.push_back(ch);
		for (int zwj = 1; zwj < 2; )
		{
			auto nk1 = md::utf8_to_unicode(bstr1, &ch1);
			bstr1 += nk1;
			if (ch1 != 0x200d)
			{
				zwj++;
				if (zwj == 2)break;
				s32.push_back(ch1);
			}
			else {
				zwj = 0; d2++;
			}
		}
#endif
		font_t::get_glyph_index_u8(ostr, &gidx, &r, &familyv[idx]);
		if (r && gidx >= 0)
		{
#if 0
			if (d2 > 0)
			{
				std::string nstr;
				font_t::GlyphPositions gp = {};
				auto nn0 = r->CollectGlyphsFromFont(s32.data(), s32.size(), 32, 0, 0, &gp);
				if (gp.len > 0) {
					printf("");
				}
			}
#endif
			auto k = r->get_glyph_item(gidx, ch, fontsize, bc_ctx);
			if (k._glyph_index)
			{
				k.cpt = ch;
				opt->tv.push_back(k);
			}
		}
		else {
			font_item_t k = {};
			k.cpt = ch;
			k.advance = 0;
			opt->tv.push_back(k);
		}
	} while (str && *str);
	return opt;
}

text_image_t* layout_text_x::get_glyph_item1(font_t* p, int fontsize, const void* str8, text_image_t* opt)
{
	auto str = (const char*)str8;
	do
	{
		if (!str || !(*str) || !opt || !p) { opt = 0; break; }
		_t1.clear(); _t1.push_back(p);
		font_t* r = 0;
		int gidx = 0;
		if (*str == '\n')
			gidx = 0;
		auto ostr = str;
		uint32_t ch = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
		font_t::get_glyph_index_u8(ostr, &gidx, &r, &_t1);
		if (r && gidx >= 0)
		{
			auto k = r->get_glyph_item(gidx, ch, fontsize, bc_ctx);
			if (k._glyph_index)
			{
				k.cpt = ch;
				opt->tv.push_back(k);
			}
		}
		else {
			font_item_t k = {};
			k.cpt = ch;
			k.advance = 0;
			opt->tv.push_back(k);
		}
	} while (str && *str);
	return opt;
}
#endif

void text_path2path_v(text_path_t* t, path_v* opt)
{
	if (!t || !opt || t->tv.empty() || t->data.empty())return;
	for (auto& it : t->tv)
	{
		opt->_data.insert(opt->_data.end(), (path_v::vertex_t*)it.v, (path_v::vertex_t*)it.v + it.count);
	}
}
text_image_pt* text_inflate(text_path_t* p, inflate_t* t)
{
	if (!p || !t)return 0;
	auto src = new path_v();
	auto dst = new path_v();
	text_path2path_v(p, src);
	inflate2flatten(src, dst, t);
	image_gray bmp = {};
	auto k = dst->get_size();
	bmp.width = k.x * 1.5;
	bmp.height = k.y * 1.5;
	auto ps = p->tv[0].bearing;
	ps.x -= t->width * 2;
	ps.y -= t->width * 2;
	ps.y -= p->tv[0].baseline;
	auto bmp1 = bmp;
	get_path_bitmap((vertex_32f*)src->data(), src->size(), &bmp1, { 1.0,1.0 }, ps, 1);
	get_path_bitmap((vertex_32f*)dst->data(), dst->size(), &bmp, { 1.0,1.0 }, ps, 1);
	text_image_pt* r = 0;
	do {
		if (bmp.width > 0)
			r = (text_image_pt*)malloc(sizeof(text_image_pt) + bmp.width * bmp.height * sizeof(uint32_t));
		if (!r)break;
		auto r1 = r + 1;
		r->img = (uint32_t*)r1;
		r->width = bmp.width;
		r->height = bmp.height;
		r->src = src;
		r->dst = dst;
		auto length = bmp._data.size();
		auto dt = bmp.data();
		uint32_t c = 0x00555555;
		for (size_t i = 0; i < length; i++)
		{
			//if (dt[i] > 0)
			//{
			//	auto c1 = c | (dt[i] << 24);
			//	r->img[i] = c1;
			//}
			//else 
			{
				r->img[i] = 0;
			}
		}
		dt = bmp1.data();
		float blur = t->width;
		auto bmp2 = bmp1;
		blur2gray((unsigned char*)dt, bmp.width, bmp.height, bmp.width, blur, 1);
		c = 0x0020ff20;
		for (size_t i = 0; i < length; i++)
		{
			if (dt[i] > 0)
			{
				auto c1 = c | (dt[i] << 24);
				r->img[i] = c1;
			}
		}
		c = 0x00ff8000; dt = bmp2.data();
		for (size_t i = 0; i < length; i++)
		{
			if (dt[i] > 0)
			{
				auto c1 = c | (dt[i] << 24);
				px_blend2c(&r->img[i], c1, -1);
			}
		}
	} while (0);
	return r;
}
text_image_pt* text_blur(text_path_t* p, float blur, int n, uint32_t color, uint32_t blur_color)
{
	if (!p || p->tv.empty())return 0;
	auto src = new path_v();
	text_path2path_v(p, src);
	image_gray bmp = {};
	auto k = src->get_size();
	bmp.width = k.x * 1.5;
	bmp.height = k.y * 1.5;
	auto ps = p->tv[0].bearing;
	ps.x -= blur * 2;
	ps.y -= blur * 2;
	ps.y -= p->tv[0].baseline;
	auto bmp1 = bmp;
	get_path_bitmap((vertex_32f*)src->data(), src->size(), &bmp, { 1.0,1.0 }, ps, 1);
	text_image_pt* r = 0;
	do {
		if (bmp.width > 0)
			r = (text_image_pt*)malloc(sizeof(text_image_pt) + bmp.width * bmp.height * sizeof(uint32_t));
		if (!r)break;
		auto r1 = r + 1;
		*r = {};
		r->img = (uint32_t*)r1;
		r->width = bmp.width;
		r->height = bmp.height;
		r->src = src;
		auto length = bmp._data.size();
		auto dt = bmp.data();
		uint32_t c = 0x00555555;
		memset(r->img, 0, sizeof(uint32_t) * length);
		dt = bmp.data();
		auto bmp2 = bmp;
		blur2gray((unsigned char*)dt, bmp.width, bmp.height, bmp.width, blur, n);
		c = blur_color;
		c &= 0x00ffffff;
		for (size_t i = 0; i < length; i++)
		{
			if (dt[i] > 0)
			{
				auto c1 = c | (dt[i] << 24);
				r->img[i] = c1;
			}
		}
		c = color;
		c &= 0x00ffffff; dt = bmp2.data();
		for (size_t i = 0; i < length; i++)
		{
			if (dt[i] > 0)
			{
				auto c1 = c | (dt[i] << 24);
				px_blend2c(&r->img[i], c1, -1);
			}
		}
	} while (0);
	return r;
}
void free_textimage(text_image_pt* p)
{
	if (p)
	{
		if (p->src)delete p->src;
		if (p->dst)delete p->dst;
		free(p);
	}
}
// 保存到png\jpg
void textimage_file(text_image_pt* p, const std::string& fn, int quality)
{
	if (p && p->img && fn.size())
	{
		if (fn.find(".png") != std::string::npos) {
			stbi_write_png(fn.c_str(), p->width, p->height, 4, p->img, 0);
		}
		else if (fn.find(".jpg") != std::string::npos) {
			stbi_write_jpg(fn.c_str(), p->width, p->height, 4, p->img, quality);
		}
	}
}
tiny_image_t* new_tiny_image(int width, int height, uint32_t* data)
{
	if (width < 2 || height < 2)return 0;
	tiny_image_t* p = (tiny_image_t*)malloc(width * height * sizeof(uint32_t) + sizeof(tiny_image_t));
	p->width = width;
	p->height = height;
	auto p1 = p + 1;
	p->img = (uint32_t*)(p + 1);
	if (data)
		memcpy(p->img, data, width * height * sizeof(uint32_t));
	else
		memset(p->img, 0, width * height * sizeof(uint32_t));
	return p;
}
tiny_image_t* gray2rgba(image_gray* g, uint32_t c)
{
	if (!g || g->width < 2 || g->_data.empty())return 0;
	auto p = new_tiny_image(g->width, g->height, 0);
	c &= 0x00ffffff;
	auto length = g->width * g->height;
	auto dt = g->data();
	for (size_t i = 0; i < length; i++)
	{
		if (dt[i] > 0)
		{
			auto c1 = c | (dt[i] << 24);
			p->img[i] = c1;
		}
	}
	return p;
}
void gray2rgba_blend(tiny_image_t* dst, const glm::ivec2& pos, image_gray* g, const glm::ivec4& rect, uint32_t c)
{
	if (!dst || !g || !dst->img)return;
	auto ps = pos.y * dst->width;
	auto data = dst->img + ps;
	int xx = rect.x + rect.z;
	xx = std::min(xx, g->width);
	c &= 0x00ffffff;
	auto dt = g->data() + rect.y * g->width;
	for (int y = 0; y < rect.w; y++)
	{
		for (int x = std::max(0, rect.x); x < xx; x++)
		{
			if (dt[x] > 0)
			{
				auto c1 = c | (dt[x] << 24);
				px_blend2c(&data[x], c1, -1);
			}
		}
		dt += g->width;
		data += dst->width;
	}
}
void free_tiny_image(tiny_image_t* p)
{
	if (p)
		free(p);
}
#endif // 1




#define PANGO_EDIT0
// todo 编辑框实现
#ifndef NO_EDIT

struct PLogAttr
{
	bool is_line_break : 1;
	bool is_mandatory_break : 1;
	bool is_char_break : 1;
	bool is_white : 1;
	bool is_cursor_position : 1;
	bool is_word_start : 1;
	bool is_word_end : 1;
	bool is_sentence_boundary : 1;
	bool is_sentence_start : 1;
	bool is_sentence_end : 1;
	bool backspace_deletes_character : 1;
	bool is_expandable_space : 1;
	bool is_word_boundary : 1;
	bool break_inserts_hyphen : 1;
	bool break_removes_preceding : 1;
};
enum dir_e {
	DIRECTION_LTR,
	DIRECTION_RTL,
	DIRECTION_TTB_LTR,
	DIRECTION_TTB_RTL,
	DIRECTION_WEAK_LTR,
	DIRECTION_WEAK_RTL,
	DIRECTION_NEUTRAL
};
struct PLayoutLine {
	text_ctx_cx* layout = 0;
	int start_index = 0;
	int length = 0;
	int resolved_dir = DIRECTION_LTR;
};
struct GlyphInfoLC {
	uint32_t glyph;
	uint32_t width;
	uint32_t x_offset;
	uint32_t y_offset;
	int log_clusters;
	bool is_cluster_start;
	bool is_color;
};
struct PGlyphString {
	int num_glyphs;
	std::vector<GlyphInfoLC> glyphs;
	//int* log_clusters;
	void init(const char* text, int n_chars, int shape_width);
};

void PGlyphString::init(const char* text, int n_chars, int shape_width)
{
	const char* p = text;
	if (n_chars < 0)n_chars = strlen(text);
	glyphs.resize(n_chars);
	for (int i = 0; i < n_chars; i++, p = md::utf8_next_char(p))
	{
		glyphs[i].glyph = -1;// PANGO_GLYPH_EMPTY;
		glyphs[i].x_offset = 0;//geometry.
		glyphs[i].y_offset = 0;//geometry.
		glyphs[i].width = shape_width;//geometry.
		glyphs[i].is_cluster_start = 1;//attr.
		glyphs[i].log_clusters = p - text;
	}
}


class text_ctx_cx
{
public:
	glm::ivec2 pos = {}, size = {};
#ifdef PANGO_EDIT
	PangoContext* context = 0;
	PangoLayout* layout = 0;
	PangoLayout* layout_editing = 0;
	std::string family = {};
#else

	std::vector<PLayoutLine> lines;
	std::vector<PLogAttr> log_attrs;
	std::vector<glm::ivec2> lvs;// 行开始结束
	std::vector<std::vector<int>> widths;// 字符偏移

#endif
	//image_ptr_t cacheimg = {};
	//d2_surface_t* sur = 0;
	//layout_text_x* ltx = 0;
	std::string text;			// 原文本
	std::string stext;			// 显示的文本
	std::string editingstr;

	font_family_t* family = 0;
	int fontid = 0;
	int fontsize = 8;
	uint32_t back_color = 0x06001020;		//背景色
	uint32_t text_color = 0xffffffff;
	//std::vector<uint32_t> dtimg;
	std::vector<glm::ivec4> rangerc;
	PathsD range_path;						// 圆角选区缓存
	path_v ptr_path;
	float round_path = 0.28;				// 圆角比例
	float _posy = -1;
	glm::ivec2 cpos = {};					// 当前鼠标坐标
	glm::ivec2 scroll_pos = {};				// 滚动坐标
	glm::ivec2 _align_pos = {};				// 对齐偏移坐标
	glm::vec2 text_align = { 0, 0.5 };		// 对齐
	//std::string text;
	glm::ivec3 cursor = { 1,-1,0 };				// 闪烁光标。宽度、颜色、毫秒
	int select_color = 0x80ffb399;
	int editing_text_color = -1;
	glm::vec4 _shadow = { 0.0,0.0,0.0,0.5 };
	glm::vec4 box_color = { 0.2,0.2,0.2,0.5 };
	int64_t ccursor = 0;	//当前光标
	int64_t ccursor8 = 0;	//当前光标字符
	int64_t caret_old = {};		//保存输入光标
	glm::ivec3 cursor_pos = {};
	glm::i64vec2 cur_select = {};
	int ckselect = 0;
	int lineheight = 10;//行高
	int clineidx = 0;	// 当前行
	int c_ct = 0;
	int c_d = 0;
	int _baseline = 0;
	int ascent = 0, descent = 0;
	int curx = 0;
	char pwd = 0;
	bool valid = true;
	bool autobr = false;
	bool is_scroll = true;
	bool is_hover = false;
	bool single_line = false;
	bool show_input_cursor = true;
	bool hover_text = false;
	bool upft = true;
	bool roundselect = true;	// 圆角选区
private:
	int64_t bounds[2] = {};	//当前选择
public:
	text_ctx_cx();
	~text_ctx_cx();

	void set_autobr(bool is);
	void set_size(const glm::ivec2& ss);
	void set_family(const char* family);
	void set_font_size(int fs);

	void set_text(const std::string& str);
	void set_editing(const std::string& str);
	void set_cursor(const glm::ivec3& c);
	glm::ivec4 get_extents();
	int get_baseline();
	int get_lineheight();
	size_t get_xy_to_index(int x, int y, const char* str);
	glm::ivec4 get_line_extents(int lidx, int idx, int dir);
	glm::ivec2 get_layout_size();
	glm::ivec2 get_line_info(int y);
	glm::i64vec2 get_bounds();
	glm::i64vec2 get_bounds0();
	void set_bounds0(const glm::i64vec2& v);
	std::vector<glm::ivec4> get_bounds_px();
	void up_caret();
	bool update(float delta);
	void draw(rvg_cx* rv);

	bool hit_test(const glm::ivec2& ps);
	void up_cursor(bool is);
	void set_single(bool is);
	glm::ivec4 get_cursor_posv(int idx);
#ifdef PANGO_EDIT
	glm::ivec2 get_pixel_size();
	void set_desc(const char* str);
	void set_markup(const std::string& str);
	glm::ivec2 get_layout_position(PangoLayout* layout);
	glm::ivec4 get_cursor_posv(PangoLayout* layout, int idx);
	glm::ivec2 get_line_length(int idx);
#else
	glm::ivec3 get_line_length(int idx);
	glm::ivec2 get_pixel_size(const char* str, int len);
#endif
private:
};


#ifndef PANGO_EDIT 






text_ctx_cx::text_ctx_cx()
{
	cursor.z = 500;
	//#ifdef _WIN32
	//	auto n = GetCaretBlinkTime();
	//	if (cursor.z < 10)
	//	{
	//		cursor.z = n;
	//	}
	//#else
	//#endif
}

text_ctx_cx::~text_ctx_cx()
{}

void text_ctx_cx::set_autobr(bool is)
{}
void text_ctx_cx::set_single(bool is) {
	single_line = is;
	//if (size.y > 0 && single_line)
	//	pango_layout_set_height(layout, size.y * PANGO_SCALE);
	//else
	//	pango_layout_set_height(layout, -1);
	//pango_layout_set_single_paragraph_mode(layout, is);
}
void text_ctx_cx::set_size(const glm::ivec2& ss)
{
	if (size != ss)
	{
		size = ss;
		valid = true;
	}
}


void text_ctx_cx::set_family(const char* familys)
{
	if (familys && *familys)
	{
		//family = familys;
	}
	//PangoFontDescription* desc = pango_font_description_new();
	//pango_font_description_set_family(desc, family.c_str());
	//pango_font_description_set_size(desc, fontsize * PANGO_SCALE);
	//pango_layout_set_font_description(layout, desc);
	//pango_layout_set_font_description(layout_editing, desc);
	//pango_font_description_free(desc);
	upft = true;
	valid = true;
}

void text_ctx_cx::set_font_size(int fs)
{
	if (fs > 2)
	{
		fontsize = fs;
		//PangoFontDescription* desc = pango_font_description_new();
		//pango_font_description_set_family(desc, family.c_str());
		//pango_font_description_set_size(desc, fontsize * PANGO_SCALE);
		//pango_layout_set_font_description(layout, desc);
		//pango_layout_set_font_description(layout_editing, desc);
		//pango_font_description_free(desc);
		//pango_layout_set_line_spacing(layout, 1.2);
		//auto kf = pango_layout_get_line_spacing(layout) / PANGO_SCALE * 1.0;
		valid = true;
	}
}

void text_ctx_cx::set_text(const std::string& str)
{
	std::string pws;
	auto length = str.size();
	if (length && pwd)
	{
		pws.resize(length);
		for (size_t i = 0; i < length; i++)
		{
			pws[i] = pwd;
		}
		stext = pws;
		//pango_layout_set_text(layout, pws.c_str(), pws.size());
	}
	else
	{
		stext = str;
		//pango_layout_set_text(layout, str.c_str(), str.size());
	}
	text = str;

	{
		lvs.clear();
		auto length = stext.size();
		auto p = stext.c_str();
		size_t f = 0, s = 0;
		for (size_t i = 0; i < length; i++)
		{
			if (p[i] == '\n' || i == length - 1)
			{
				lvs.push_back({ f,i - f });
				f = i + 1;
			}
		}
		log_attrs.clear();
		log_attrs.resize(text.size());
	}
	widths.clear();
	valid = true;
	upft = true;
}

void text_ctx_cx::set_editing(const std::string& str)
{
	editingstr = str;
	//pango_layout_set_text(layout_editing, str.c_str(), str.size());
	//pango_layout_set_markup(layout_editing, str.c_str(), str.size());
	valid = true;
}


void text_ctx_cx::set_cursor(const glm::ivec3& c)
{
	cursor = c;
}

glm::ivec4 text_ctx_cx::get_extents()
{
	// todo 获取像素大小区域
	return {};
}

glm::ivec2 text_ctx_cx::get_pixel_size(const char* str, int len)
{
	int w = 0, h = 0;
	if (str && *str)
	{
		auto rc = get_text_rect(family, fontsize, str, len, 0);
		w = rc.x; h = rc.y;
	}
	return glm::ivec2(w, h);
}

int text_ctx_cx::get_baseline()
{
	return font_get_baseline(family, fontsize);
	//glm::ceil((double)pango_layout_get_baseline(layout) / PANGO_SCALE);
}
int text_ctx_cx::get_lineheight()
{
	return font_get_lineheight(family, fontsize);
	//glm::ceil((double)pango_layout_get_baseline(layout) / PANGO_SCALE);
}
void glyph_string_x_to_index(PGlyphString* glyphs, const char* text, int length, bool r2l, int x_pos, int* index, bool* trailing)
{
	int i;
	int start_xpos = 0;
	int end_xpos = 0;
	int width = 0;

	int start_index = -1;
	int end_index = -1;

	int cluster_chars = 0;
	const char* p;

	bool found = false;

	/* Find the cluster containing the position */

	width = 0;

	if (r2l)//analysis->level % 2) /* Right to left */
	{
		for (i = glyphs->num_glyphs - 1; i >= 0; i--)
			width += glyphs->glyphs[i].width;

		for (i = glyphs->num_glyphs - 1; i >= 0; i--)
		{
			if (glyphs->glyphs[i].log_clusters != start_index)
			{
				if (found)
				{
					end_index = glyphs->glyphs[i].log_clusters;
					end_xpos = width;
					break;
				}
				else
				{
					start_index = glyphs->glyphs[i].log_clusters;
					start_xpos = width;
				}
			}

			width -= glyphs->glyphs[i].width;

			if (width <= x_pos && x_pos < width + glyphs->glyphs[i].width)
				found = true;
		}
	}
	else /* Left to right */
	{
		for (i = 0; i < glyphs->num_glyphs; i++)
		{
			if (glyphs->glyphs[i].log_clusters != start_index)
			{
				if (found)
				{
					end_index = glyphs->glyphs[i].log_clusters;
					end_xpos = width;
					break;
				}
				else
				{
					start_index = glyphs->glyphs[i].log_clusters;
					start_xpos = width;
				}
			}

			if (width <= x_pos && x_pos < width + glyphs->glyphs[i].width)
				found = true;

			width += glyphs->glyphs[i].width;
		}
	}

	if (end_index == -1)
	{
		end_index = length;
		end_xpos = (r2l) ? 0 : width;
	}

	/* Calculate number of chars within cluster */
	p = text + start_index;
	while (p < text + end_index)
	{
		p = md::utf8_next_char(p);
		cluster_chars++;
	}

	if (start_xpos == end_xpos)
	{
		if (index)
			*index = start_index;
		if (trailing)
			*trailing = false;
	}
	else
	{
		double cp = ((double)(x_pos - start_xpos) * cluster_chars) / (end_xpos - start_xpos);

		/* LTR and right-to-left have to be handled separately
		 * here because of the edge condition when we are exactly
		 * at a pixel boundary; end_xpos goes with the next
		 * character for LTR, with the previous character for RTL.
		 */
		if (start_xpos < end_xpos) /* Left-to-right */
		{
			if (index)
			{
				const char* p = text + start_index;
				int i = 0;

				while (i + 1 <= cp)
				{
					p = md::utf8_next_char(p);
					i++;
				}

				*index = (p - text);
			}

			if (trailing)
				*trailing = (cp - (int)cp >= 0.5);
		}
		else /* Right-to-left */
		{
			if (index)
			{
				const char* p = text + start_index;
				int i = 0;

				while (i + 1 < cp)
				{
					p = md::utf8_next_char(p);
					i++;
				}

				*index = (p - text);
			}

			if (trailing)
			{
				double cp_flip = cluster_chars - cp;
				*trailing = (cp_flip - (int)cp_flip >= 0.5);
			}
		}
	}
}

bool layout_line_x_to_index(PLayoutLine* line, int x_pos, int* index, int* trailing)
{
	int start_pos = 0;
	int first_index = 0; /* line->start_index */
	int first_offset;
	int last_index;      /* start of last grapheme in line */
	int last_offset;
	int end_index;       /* end iterator for line */
	int end_offset;      /* end iterator for line */
	text_ctx_cx* layout;
	int last_trailing;
	bool suppress_last_trailing;

	layout = line->layout;

	/* Find the last index in the line
	 */
	first_index = line->start_index;

	if (line->length == 0)
	{
		if (index)
			*index = first_index;
		if (trailing)
			*trailing = 0;

		return false;
	}

	assert(line->length > 0);
	auto text = layout->text.c_str();
	first_offset = md::utf8_pointer_to_offset(text, text + line->start_index);

	end_index = first_index + line->length;
	end_offset = first_offset + md::utf8_pointer_to_offset(text + first_index, text + end_index);

	last_index = end_index;
	last_offset = end_offset;
	last_trailing = 0;
	do
	{
		last_index = md::utf8_prev_char(text + last_index) - text;
		last_offset--;
		last_trailing++;
	} while (last_offset > first_offset && !layout->log_attrs[last_offset].is_cursor_position);

	/* This is a HACK. If a program only keeps track of cursor (etc)
	 * indices and not the trailing flag, then the trailing index of the
	 * last character on a wrapped line is identical to the leading
	 * index of the next line. So, we fake it and set the trailing flag
	 * to zero.
	 *
	 * That is, if the text is "now is the time", and is broken between
	 * 'now' and 'is'
	 *
	 * Then when the cursor is actually at:
	 *
	 * n|o|w| |i|s|
	 *              ^
	 * we lie and say it is at:
	 *
	 * n|o|w| |i|s|
	 *            ^
	 *
	 * So the cursor won't appear on the next line before 'the'.
	 *
	 * Actually, any program keeping cursor
	 * positions with wrapped lines should distinguish leading and
	 * trailing cursors.
	 */
	auto te = layout->lines.data() + layout->lines.size();
	auto tmp_list = layout->lines.data();
	while (tmp_list != line)
		tmp_list++;

	if (tmp_list != te && line->start_index + line->length == tmp_list->start_index)
		suppress_last_trailing = true;
	else
		suppress_last_trailing = false;

	if (x_pos < 0)
	{
		/* pick the leftmost char */
		if (index)
			*index = (line->resolved_dir == DIRECTION_LTR) ? first_index : last_index;
		/* and its leftmost edge */
		if (trailing)
			*trailing = (line->resolved_dir == DIRECTION_LTR || suppress_last_trailing) ? 0 : last_trailing;

		return false;
	}

	tmp_list = layout->lines.data();
	auto strc = text + tmp_list->start_index;
	while (tmp_list)
	{
		auto cw = font_get_text_rect1(layout->family, layout->fontsize, strc);
		int logical_width = cw.x;// pango_glyph_string_get_width(run->glyphs);

		if (x_pos >= start_pos && x_pos < start_pos + logical_width)
		{
			int offset;
			bool char_trailing = false;
			int grapheme_start_index;
			int grapheme_start_offset;
			int grapheme_end_offset;
			int pos;
			int char_index = tmp_list->start_index;

			//glyph_string_x_to_index(run->glyphs, text + run->item->offset, run->item->length, &run->item->analysis, x_pos - start_pos, &pos, &char_trailing);

			//char_index = run->item->offset + pos;

			/* Convert from characters to graphemes */
			// 返回字符偏移
			offset = md::utf8_pointer_to_offset(text, text + char_index);

			grapheme_start_offset = offset;
			grapheme_start_index = char_index;
			while (grapheme_start_offset > first_offset && !layout->log_attrs[grapheme_start_offset].is_cursor_position)
			{
				grapheme_start_index = md::utf8_prev_char(text + grapheme_start_index) - text;
				grapheme_start_offset--;
			}

			grapheme_end_offset = offset;
			do
			{
				grapheme_end_offset++;
			} while (grapheme_end_offset < end_offset && !layout->log_attrs[grapheme_end_offset].is_cursor_position);

			if (index)
				*index = grapheme_start_index;

			if (trailing)
			{
				if ((grapheme_end_offset == end_offset && suppress_last_trailing) || offset + char_trailing <= (grapheme_start_offset + grapheme_end_offset) / 2)
					*trailing = 0;
				else
					*trailing = grapheme_end_offset - grapheme_start_offset;
			}

			return true;
		}

		start_pos += logical_width;
		tmp_list++;
	}

	/* pick the rightmost char */
	if (index)
		*index = (line->resolved_dir == DIRECTION_LTR) ? last_index : first_index;

	/* and its rightmost edge */
	if (trailing)
		*trailing = (line->resolved_dir == DIRECTION_LTR && !suppress_last_trailing) ? last_trailing : 0;

	return false;
}

bool layout_xy_to_index(text_ctx_cx* layout, int x, int y, int* index, int* trailing) {
	PLayoutLine* prev_line = 0;
	PLayoutLine* found = 0;
	int found_line_x = 0;
	int prev_last = 0;
	int prev_line_x = 0;
	bool retval = false;
	bool outside = false;
	auto iter = layout->lines.data();
	auto h = layout->get_lineheight();
	for (size_t i = 0; i < layout->lines.size(); i++, iter++)
	{
		glm::ivec4 line_logical = {};
		int first_y = i * h, last_y = h * (i + 1);
		//pango_layout_iter_get_line_extents(&iter, NULL, &line_logical);
		//pango_layout_iter_get_line_yrange(&iter, &first_y, &last_y);
		if (y < first_y)
		{
			if (prev_line && y < (prev_last + (first_y - prev_last) / 2))
			{
				found = prev_line;
				found_line_x = prev_line_x;
			}
			else
			{
				if (prev_line == 0)
					outside = true; /* off the top */
				found = iter;
				found_line_x = x - line_logical.x;
			}
		}
		else if (y >= first_y && y < last_y)
		{
			found = iter;
			found_line_x = x - line_logical.x;
		}
		prev_line = iter;
		prev_last = last_y;
		prev_line_x = x - line_logical.x;
		if (found != 0)
			break;
	}
	if (found == 0)
	{
		/* Off the bottom of the layout */
		outside = true;
		found = prev_line;
		found_line_x = prev_line_x;
	}
	retval = layout_line_x_to_index(found, found_line_x, index, trailing);
	if (outside)
		retval = false;
	return retval;
}
#if 0
size_t text_ctx_cx::get_xy_to_index(int x, int y, const char* str)
{
	if (ltx && widths.empty())
	{
		auto pstr = stext.c_str();
		ltx->get_text_posv(fontid, fontsize, pstr, stext.size(), widths);
	}
	x += scroll_pos.x - _align_pos.x;
	y += scroll_pos.y - _align_pos.y;

	if (x < 0)
	{
		x = 0;
	}
	if (y < 0)
	{
		y = 0;
	}
	int lc = lvs.size();
	glm::ivec2 lps = {};
	//pango_layout_get_pixel_size(layout, &lps.x, &lps.y);
	if (y > lps.y)
		y = lps.y - 1;
	y /= get_lineheight();
	if (y >= lvs.size())y = lvs.size() - 1;

	int index = 0, trailing = 0;
	bool k = layout_xy_to_index(this, x, y, &index, &trailing);

	clineidx = y;// 当前行号
	lineheight = get_lineheight();
	auto cursor = index + trailing;
	if (!k)
	{
		auto nk = get_line_length(cursor);
		cursor = nk.x;
	}
	auto cp = str + cursor;
	auto chp = md::get_utf8_first(cp);
	int ps = chp - cp;
	cursor += ps;
	return cursor;
}
#endif
// 获取鼠标坐标的光标位置
size_t text_ctx_cx::get_xy_to_index(int x, int y, const char* str)
{
	auto pstr = text.c_str();
	if (widths.empty())
	{
		font_get_text_posv(family, fontsize, pstr, text.size(), widths);
	}
	if (widths.size() != lvs.size())
		return -1;
	x += scroll_pos.x - _align_pos.x;
	y += scroll_pos.y - _align_pos.y;
	int index = 0, trailing = 0;
	if (x < 0)
	{
		x = 0;
	}
	if (y < 0)
	{
		y = 0;
	}
	glm::ivec2 lps = get_pixel_size(pstr, text.size());
	if (y > lps.y)
		y = lps.y - 1;
	y /= get_lineheight();
	if (y >= lvs.size())y = lvs.size() - 1;
	auto ky = lvs[y];
	auto& ws = widths[y];
	int cw = 0;
	for (size_t i = 0; i < ws.size(); i++)
	{
		if (x < ws[i]) {
			index = i;  break;
		}
	}
	if (x > *(ws.rbegin())) {
		index = ws.size() - 1;
	}
	else if (index > 0) {
		int tr = (ws[index] - ws[index - 1]) * 0.5;
		int xx = x - ws[index - 1];
		if (xx >= tr)
		{
			cw = ws[index];
			index++;
		}
		else {
			cw = ws[index - 1];
		}
		index--;
	}
	auto newx = md::utf8_char_pos(str + ky.x, index, -1);
	newx -= (uint64_t)str;
	index = (uint64_t)newx;
	//index += ky.x;
	curx = cw;
	//printf("gxy:%d\n", (int)index);
	return (size_t)index;
	auto cursor = index + trailing;
	auto cp = str + cursor;
	auto chp = md::get_utf8_first(cp);
	int ps = chp - cp;
	cursor += ps;
	return cursor;
}

// 输入行号，索引号，方向，输出行号选择范围
glm::ivec4 text_ctx_cx::get_line_extents(int lidx, int idx, int dir)
{
	//pango_layout_get_line_count
	return {};
}
glm::ivec3 text_ctx_cx::get_line_length(int index)
{
	int x_pos = 0;
	int lidx = 0;
	int cu = -1;
	int cx = 0;
	for (size_t i = 0; i < lvs.size(); i++)
	{
		auto c = lvs[i];
		if (index >= c.x && index < c.x + c.y + 1)//因为有换行+1
		{
			x_pos = index - c.x;
			lidx = c.x;
			cu = i;// 行号
			break;
		}
	}
	if (cu < 0 && lvs.size()) {
		cu = lvs.size() - 1;
		auto c = lvs[cu];
		x_pos = index - c.x;
		lidx = c.x;
	}
	glm::ivec3 ret = { lidx,cu, x_pos };
	return ret;
}

glm::ivec2 text_ctx_cx::get_layout_size()
{
	return size;
}
// todo line
glm::ivec2 text_ctx_cx::get_line_info(int y)
{
	return y > 0 && y < lvs.size() ? lvs[y] : glm::ivec2();
}

glm::i64vec2 text_ctx_cx::get_bounds()
{
	glm::i64vec2 v = { bounds[0] , bounds[1] };
	if (v.x > v.y) { std::swap(v.x, v.y); }
	return v;
}
glm::i64vec2 text_ctx_cx::get_bounds0()
{
	glm::i64vec2 v = { bounds[0] , bounds[1] };
	return v;
}
void text_ctx_cx::set_bounds0(const glm::i64vec2& v)
{
	bounds[0] = v.x; bounds[1] = v.y;
	//printf("bounds:%d\t%d\n", (int)v.x, (int)v.y);
	//if (v.x == v.y && v.x == 0) {
	//	printf(" \n");
	//}
}
//glm::ivec2 geti2x(PangoLayout* layout, int x)
//{
//	int x_pos = 0;
//	int lidx = 0;
//	//pango_layout_index_to_line_x(layout, x, 0, &lidx, &x_pos);
//	//x_pos /= PANGO_SCALE;
//	return glm::ivec2(x_pos, lidx);
//}

glm::ivec4 text_ctx_cx::get_cursor_posv(int idx)
{
	glm::ivec4 r = { /*w1.x,w1.y,w1.width,w1.height*/ };
	return r;
}

//std::vector<glm::ivec4> get_caret_posv(PangoLayout* layout, int idx)
//{
//	std::vector<glm::ivec4> rv;
//	//glm::ivec4 r = { w0.x,w0.y,w0.width,w0.height };
//	//glm::ivec4 r1 = { w1.x,w1.y,w1.width,w1.height };
//	//rv.push_back(r);
//	//rv.push_back(r1); 
//	return rv;
//}

size_t char2pos(size_t ps, const char* str) {
	return md::get_utf8_count(str, ps);
}
// todo 获取范围的像素大小
std::vector<glm::ivec4> text_ctx_cx::get_bounds_px()
{
	float pwidth = fontsize * 0.5;// 补行尾宽度
	if (widths.empty())
	{
		auto pstr = text.c_str();
		font_get_text_posv(family, fontsize, pstr, text.size(), widths);
		pwidth = font_get_text_rect1(family, fontsize, "1").x;
	}
	std::vector<glm::ivec4> r;
	std::vector<glm::ivec4> rs, rss;
	auto pstr = text.c_str();
	auto tsize = text.size();
	auto v = get_bounds();
	if (v.x == v.y) { rangerc = rss; return rs; }
	if ((v.x >= tsize || v.y > tsize)) {
		rangerc = rss; return rs;
	}
	auto v1 = get_line_length(v.x);
	auto v2 = get_line_length(v.y);
	auto line_no = lvs.size();
	auto h = get_lineheight();
	// 计算选中范围的每行的坐标宽高
	if (v1 == v2) {}
	else {
		if (v1.y == v2.y)
		{
			auto ks = lvs[v1.y];
			auto w1 = widths[v1.y];
			auto xc = char2pos(v.x - ks.x, pstr + ks.x);
			auto yc = char2pos(v.y - ks.x, pstr + ks.x);
			int w = w1[xc];
			int ww = w1[yc] - w;
			rss.push_back({ w ,v1.y * h,ww,h });// 同一行时
		}
		else {
			auto ks = lvs[v1.y];
			auto w1 = widths[v1.y];
			auto w = w1[char2pos(v.x - ks.x, pstr + ks.x)];
			auto wd = *w1.rbegin() - w;
			rss.push_back({ w,v1.y * h,wd + pwidth,h });// 第一行
		}
		for (int i = v1.y + 1; i < line_no && i < v2.y; i++)
		{
			auto ks = lvs[i];
			auto w1 = widths[i];
			rss.push_back({ 0,i * h,*w1.rbegin() + pwidth, h });// 中间全行
		}
		if (v1.y < v2.y)
		{
			auto ks = lvs[v2.y];
			auto w1 = widths[v2.y];
			rss.push_back({ 0,v2.y * h,w1[char2pos(v.y - ks.x ,pstr + ks.x)],h });//最后一行
		}
	}
	if (roundselect)
	{
		PathsD subjects;
		PathD a;
		int py = _posy;
		for (size_t i = 0; i < rss.size(); i++)
		{
			auto& it = rss[i];
			if (it.z < 1)
				it.z = pwidth;
			a.push_back({ it.x,it.y + py });
			a.push_back({ it.x + it.z,it.y + py });
			a.push_back({ it.x + it.z,it.y + it.w + py });
			a.push_back({ it.x,it.y + it.w + py });
			subjects.push_back(a);
			a.clear();
		}
		subjects = Union(subjects, FillRule::NonZero, 6);
		//range_path = InflatePaths(subjects, 0.5, JoinType::Round, EndType::Polygon);
		auto sn = subjects.size();
		if (sn > 0)
		{
			path_v& ptr = ptr_path;
			ptr._data.clear();
			for (size_t i = 0; i < sn; i++)
			{
				auto& it = subjects[i];
				ptr.add_lines((glm::dvec2*)it.data(), it.size(), false);
			}
			range_path = gp::path_round(&ptr, -1, fontsize * round_path, 16, 0, 0);
		}
		else { range_path.clear(); }
	}
	rangerc = rss;
	return r;
}

void text_ctx_cx::up_caret()
{
	if (upft)
	{
		get_bounds_px();
		_baseline = get_baseline(); lineheight = get_lineheight();
		upft = false;
	}
	glm::ivec4 caret = {};
	auto v1 = get_line_length((int)ccursor);
	auto line_no = lvs.size();
	auto h = lineheight;
	// 计算选中范围的每行的坐标宽高 
	if (line_no > 0 && widths.size() > v1.y)
	{
		auto ks = lvs[v1.y];
		auto w1 = widths[v1.y];
		{
			auto pstr = text.c_str();
			caret.x = get_text_rect(family, fontsize, pstr + ks.x, ks.y, ccursor - ks.x).x;
		}
		caret.y = cursor_pos.z * v1.y;
		//printf("cursor:\t%d\n", cursor_pos.x);
	}
	cursor_pos = caret; cursor_pos.z = h;
}
bool text_ctx_cx::update(float delta)
{
	c_ct += delta * 1000.0 * c_d;
	if (c_ct > cursor.z)
	{
		c_d = -1;
		c_ct = cursor.z;
		valid = true;
	}
	if (c_ct < 0)
	{
		c_d = 1; c_ct = 0;
		valid = true;
	}
	if (!valid)return false;

	if (upft)
	{
		get_bounds_px();
		_baseline = get_baseline();
		lineheight = get_lineheight();
		upft = false;
	}
	if (single_line) {
		glm::vec2 ext = { 0,lineheight }, ss = size;
		auto ps = ss * text_align - (ext * text_align);
		_align_pos.y = ps.y;
	}
	bool ret = valid;
	valid = false;
	return true;
}
uint32_t get_reverse_color(uint32_t color) {
	uint8_t* c = (uint8_t*)&color;
	c[0] = 255 - c[0];
	c[1] = 255 - c[1];
	c[2] = 255 - c[2];
	return color;
}

void text_ctx_cx::draw(rvg_cx* rv)
{
	rv->save();
	rv->translate(pos);
	// 裁剪区域
	//rv->add_rect({ 0,0,size.x,size.y }, 0);
	//rv->clip();

	auto ps = _align_pos - scroll_pos;
	//auto oldop = cairo_get_operator(cr);
	//cairo_set_source_surface(cr, sur, pos.x, pos.y);
	//cairo_paint(cr);
	auto bbc = box_color;
	rv->set_color(bbc);
	rv->add_rect({ -0.5,   -0.5, size.x + 1, size.y + 1 }, 0);
	rv->set_line_width(1);
	//rv->stroke(); 
	rv->fill();
	{
		rv->save();
		// 裁剪区域
		rv->add_rect({ 1,1,size.x - 2,size.y - 2 }, 0);
		rv->clip();
		rv->translate({ -scroll_pos.x + _align_pos.x, -scroll_pos.y + _align_pos.y });

		auto v = get_bounds();
		if (v.x != v.y && rangerc.size()) {
			rv->set_color(select_color);
			if (roundselect)
			{
				if (range_path.size() && range_path[0].size() > 3) {
					rv->add_polyline(&range_path, true);
					rv->fill();
				}
			}
			else {
				for (auto& it : rangerc)
				{
					rv->add_rect(it, std::min(it.z, it.w) * 0.18);
					rv->fill();
				}
			}
		}
		rv->set_color(text_color);
		auto lhh = get_pixel_size(stext.c_str(), stext.size());
		// 渲染文本
		glm::vec4 rc = { 0,0,  lhh };
		text_style st = {};
		st.fontsize = fontsize;
		st.align = {};
		st.color = text_color;
		st.family = family;
		text_st tx = {};
		tx.pos = {};
		tx.size = size;
		tx.text = stext.c_str(); tx.text_len = stext.size();

		rv->add_text(&tx, &st);
		rv->restore();
	}
	double x = ps.x + cursor_pos.x, y = ps.y + cursor_pos.y;
	if (show_input_cursor && c_d == 1 && cursor.x > 0 && cursor_pos.z > 0)
	{
		rv->set_color(cursor.y);
		rv->add_rect({ x, y, cursor.x, cursor_pos.z }, 0);
		rv->fill();
	}
	// 编辑中的文本
	if (editingstr.size())
	{
		rv->translate({ x, y });
		// 渲染文本 
		text_style st = {};
		st.fontsize = fontsize;
		st.align = {};
		st.color = editing_text_color;
		st.family = family;
		glm::ivec2 lps = {};
		lps = get_pixel_size(editingstr.c_str(), editingstr.size());
		if (lps.y < lineheight)
			lps.y = lineheight;
		glm::vec4 lss = { 0,  lps.y + 0.5, lps.x, lps.y + 0.5 };
		glm::vec4 rc = { 1,1,lps.x + 2, lps.y + 2 };
		rv->set_color(get_reverse_color(editing_text_color));
		rv->add_rect({ 0, 0, lps.x + 2, lps.y + 2 }, 0);
		rv->fill();
		rv->set_color(editing_text_color);
		text_st tx = {};
		tx.pos = { rc.x,rc.y };
		tx.size = glm::ivec2(rc.z, rc.w);
		tx.text = editingstr.c_str(); tx.text_len = editingstr.size();
		rv->add_text(&tx, &st);
		rv->add_line({ lss.x + 1, lss.y }, { lss.z, lss.w });
		rv->set_line_width(1);
		rv->stroke();
	}
	rv->restore();
}

#else

text_ctx_cx::text_ctx_cx()
{
	PangoFontMap* fontMap = get_fmap;
	context = pango_font_map_create_context(fontMap);
	//pango_context_set_round_glyph_positions(context, 0);
	layout = pango_layout_new(context);
	layout_editing = pango_layout_copy(layout);
	pango_layout_set_wrap(layout, PANGO_WRAP_CHAR);
	pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
#ifdef _WIN32
	auto n = GetCaretBlinkTime();
	if (cursor.z < 10)
	{
		cursor.z = n;
	}
#else
	cursor.z = 500;
#endif
}

text_ctx_cx::~text_ctx_cx()
{
	if (context)
	{
		g_object_unref(context); context = 0;
	}
	if (layout)
	{
		g_object_unref(layout); layout = 0;
	}
	if (layout_editing)
	{
		g_object_unref(layout_editing); layout_editing = 0;
	}
	if (sur)
	{
		cairo_surface_destroy(sur); sur = 0;
	}
}

void text_ctx_cx::set_autobr(bool is)
{
	pango_layout_set_width(layout, is ? size.x * PANGO_SCALE : -1);
}
void text_ctx_cx::set_single(bool is) {
	single_line = is;
	if (size.y > 0 && single_line)
		pango_layout_set_height(layout, size.y * PANGO_SCALE);
	else
		pango_layout_set_height(layout, -1);
	pango_layout_set_single_paragraph_mode(layout, is);
}
void text_ctx_cx::set_size(const glm::ivec2& ss)
{
	if (size != ss)
	{
		size = ss;
		if (sur)
		{
			cairo_surface_destroy(sur);
		}
		dtimg.resize(size.x * size.y);
		sur = cairo_image_surface_create_for_data((unsigned char*)dtimg.data(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * sizeof(int));
		cacheimg = {};
		cacheimg.data = dtimg.data();
		cacheimg.width = size.x;
		cacheimg.height = size.y;
		cacheimg.type = 1;
		cacheimg.valid = 1;
		valid = true;
	}
}

void text_ctx_cx::set_desc(const char* str)
{
	auto desc = pango_font_description_from_string(str);// "Sans Bold Italic Condensed 22.5px");
	if (desc)
	{
		pango_layout_set_font_description(layout, desc);
		pango_font_description_free(desc);
		valid = true;
	}
}

void text_ctx_cx::set_family(const char* familys)
{
	if (familys && *familys)
	{
		family = familys;
	}
	PangoFontDescription* desc = pango_font_description_new();
	pango_font_description_set_family(desc, family.c_str());
	pango_font_description_set_size(desc, fontsize * PANGO_SCALE);
	pango_layout_set_font_description(layout, desc);
	pango_layout_set_font_description(layout_editing, desc);
	pango_font_description_free(desc);
	upft = true;
	valid = true;
}

void text_ctx_cx::set_font_size(int fs)
{
	if (fs > 2)
	{
		fontsize = fs;
		PangoFontDescription* desc = pango_font_description_new();
		pango_font_description_set_family(desc, family.c_str());
		pango_font_description_set_size(desc, fontsize * PANGO_SCALE);
		pango_layout_set_font_description(layout, desc);
		pango_layout_set_font_description(layout_editing, desc);
		pango_font_description_free(desc);
		pango_layout_set_line_spacing(layout, 1.2);
		auto kf = pango_layout_get_line_spacing(layout) / PANGO_SCALE * 1.0;
		valid = true;
	}
}

void text_ctx_cx::set_text(const std::string& str)
{
	std::string pws;
	auto length = str.size();
	if (length && pwd)
	{
		pws.resize(length);
		for (size_t i = 0; i < length; i++)
		{
			pws[i] = pwd;
		}
		pango_layout_set_text(layout, pws.c_str(), pws.size());
	}
	else
	{
		pango_layout_set_text(layout, str.c_str(), str.size());
	}
	int h = 0;
	auto line = pango_layout_get_line(layout, 0);
	pango_layout_line_get_height(line, &h);
	lineheight = h / PANGO_SCALE;
	get_bounds_px();
	valid = true;
}

void text_ctx_cx::set_editing(const std::string& str)
{
	editingstr = str;
	pango_layout_set_text(layout_editing, str.c_str(), str.size());
	//pango_layout_set_markup(layout_editing, str.c_str(), str.size());
	valid = true;
}

void text_ctx_cx::set_markup(const std::string& str)
{
	pango_layout_set_markup(layout, str.c_str(), str.size());
	int h = 0;
	auto line = pango_layout_get_line(layout, 0);
	pango_layout_line_get_height(line, &h);
	lineheight = h / PANGO_SCALE;
	valid = true;
}

void text_ctx_cx::set_cursor(const glm::ivec3& c)
{
	cursor = c;
}

glm::ivec4 text_ctx_cx::get_extents()
{
	PangoRectangle ink_rect = {};
	PangoRectangle logical_rect = {};
	pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);
	return glm::ivec4(ink_rect.x, ink_rect.y, ink_rect.width, ink_rect.height);
}

glm::ivec2 text_ctx_cx::get_pixel_size()
{
	int w = 0, h = 0;
	pango_layout_get_pixel_size(layout, &w, &h);
	return glm::ivec2(w, h);
}

int text_ctx_cx::get_baseline()
{
	return glm::ceil((double)pango_layout_get_baseline(layout) / PANGO_SCALE);
}
size_t text_ctx_cx::get_xy_to_index(int x, int y, const char* str)
{
	x += scroll_pos.x - _align_pos.x;
	y += scroll_pos.y - _align_pos.y;

	if (x < 0)
	{
		x = 0;
	}
	if (y < 0)
	{
		y = 0;
	}
	auto cc = pango_layout_get_character_count(layout);
	int lc = pango_layout_get_line_count(layout);
	glm::ivec2 lps = {};
	pango_layout_get_pixel_size(layout, &lps.x, &lps.y);
	if (y > lps.y)
		y = lps.y - 1;
	int index = 0, trailing = 0;
	auto ls = pango_layout_get_lines_readonly(layout);
	bool k = pango_layout_xy_to_index(layout, x * PANGO_SCALE, y * PANGO_SCALE, &index, &trailing);
	int x_pos = 0;
	int lidx = 0;
	pango_layout_index_to_line_x(layout, index, 0, &lidx, &x_pos);


	x_pos /= PANGO_SCALE;
	auto cursor = index + trailing;
	if (!k)
	{
		auto nk = get_line_length(cursor);
		cursor = nk.x;
	}
	auto cp = str + cursor;
	auto chp = md::get_utf8_first(cp);
	int ps = chp - cp;
	cursor += ps;
	cursor;
	clineidx = lidx;
	//printf("%d\t%d\n", ccursor, ps);
	PangoRectangle ink_rect = {};
	PangoRectangle logical_rect = {};
	pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);	//cairo_move_to(cr, 0 /*extents.x*/, ink_rect.y - (logical_rect.height - ink_rect.height) * 0.5);
	// 获取基线位置
	int y_pos = pango_layout_get_baseline(layout);
	int h = 0;
	auto line = pango_layout_get_line(layout, lidx);
	pango_layout_line_get_height(line, &h);
	lineheight = h / PANGO_SCALE;
	return cursor;
}

glm::ivec2 text_ctx_cx::get_layout_position(PangoLayout* layout)
{
	const int text_height = size.y;
	PangoRectangle logical_rect;
	int y_pos, area_height;
	PangoLayoutLine* line;


	area_height = PANGO_SCALE * text_height;

	line = (PangoLayoutLine*)pango_layout_get_lines_readonly(layout)->data;
	pango_layout_line_get_extents(line, NULL, &logical_rect);

	/* Align primarily for locale's ascent/descent */
	if (_baseline < 0)
		y_pos = ((area_height - ascent - descent) / 2 +
			ascent + logical_rect.y);
	else
		y_pos = PANGO_SCALE * _baseline - pango_layout_get_baseline(layout);

	/* Now see if we need to adjust to fit in actual drawn string */
	if (logical_rect.height > area_height)
		y_pos = (area_height - logical_rect.height) / 2;
	else if (y_pos < 0)
		y_pos = 0;
	else if (y_pos + logical_rect.height > area_height)
		y_pos = area_height - logical_rect.height;

	y_pos = y_pos / PANGO_SCALE;
	return { -scroll_pos.x, y_pos };
}
int layout_get_char_width(PangoLayout* layout)
{
	int width;
	PangoFontMetrics* metrics;
	const PangoFontDescription* font_desc;
	PangoContext* context = pango_layout_get_context(layout);

	font_desc = pango_layout_get_font_description(layout);
	if (!font_desc)
		font_desc = pango_context_get_font_description(context);

	metrics = pango_context_get_metrics(context, font_desc, NULL);
	width = pango_font_metrics_get_approximate_char_width(metrics);
	pango_font_metrics_unref(metrics);

	return width;
}
gboolean text_util_get_block_cursor_location(PangoLayout* layout, int index, PangoRectangle* pos, gboolean* at_line_end)
{
	PangoRectangle strong_pos, weak_pos;
	PangoLayoutLine* layout_line;
	gboolean rtl;
	int line_no;
	const char* text;

	g_return_val_if_fail(layout != NULL, FALSE);
	g_return_val_if_fail(index >= 0, FALSE);
	g_return_val_if_fail(pos != NULL, FALSE);

	pango_layout_index_to_pos(layout, index, pos);

	if (pos->width != 0)
	{
		/* cursor is at some visible character, good */
		if (at_line_end)
			*at_line_end = FALSE;
		if (pos->width < 0)
		{
			pos->x += pos->width;
			pos->width = -pos->width;
		}
		return TRUE;
	}

	pango_layout_index_to_line_x(layout, index, FALSE, &line_no, NULL);
	layout_line = pango_layout_get_line_readonly(layout, line_no);
	g_return_val_if_fail(layout_line != NULL, FALSE);

	text = pango_layout_get_text(layout);

	if (index < pango_layout_line_get_start_index(layout_line) + pango_layout_line_get_length(layout_line))
	{
		/* this may be a zero-width character in the middle of the line,
		 * or it could be a character where line is wrapped, we do want
		 * block cursor in latter case */
		if (g_utf8_next_char(text + index) - text !=
			pango_layout_line_get_start_index(layout_line) + pango_layout_line_get_length(layout_line))
		{
			/* zero-width character in the middle of the line, do not
			 * bother with block cursor */
			return FALSE;
		}
	}

	/* Cursor is at the line end. It may be an empty line, or it could
	 * be on the left or on the right depending on text direction, or it
	 * even could be in the middle of visual layout in bidi text. */

	pango_layout_get_cursor_pos(layout, index, &strong_pos, &weak_pos);

	if (strong_pos.x != weak_pos.x)
	{
		/* do not show block cursor in this case, since the character typed
		 * in may or may not appear at the cursor position */
		return FALSE;
	}

	/* In case when index points to the end of line, pos->x is always most right
	 * pixel of the layout line, so we need to correct it for RTL text. */
	if (pango_layout_line_get_length(layout_line))
	{
		if (pango_layout_line_get_resolved_direction(layout_line) == PANGO_DIRECTION_RTL)
		{
			PangoLayoutIter* iter;
			PangoRectangle line_rect;
			int i;
			int left, right;
			const char* p;

			p = g_utf8_prev_char(text + index);

			pango_layout_line_index_to_x(layout_line, p - text, FALSE, &left);
			pango_layout_line_index_to_x(layout_line, p - text, TRUE, &right);
			pos->x = MIN(left, right);

			iter = pango_layout_get_iter(layout);
			for (i = 0; i < line_no; i++)
				pango_layout_iter_next_line(iter);
			pango_layout_iter_get_line_extents(iter, NULL, &line_rect);
			pango_layout_iter_free(iter);

			rtl = TRUE;
			pos->x += line_rect.x;
		}
		else
			rtl = FALSE;
	}
	else
	{
		PangoContext* context = pango_layout_get_context(layout);
		rtl = pango_context_get_base_dir(context) == PANGO_DIRECTION_RTL;
	}

	pos->width = layout_get_char_width(layout);

	if (rtl)
		pos->x -= pos->width - 1;

	if (at_line_end)
		*at_line_end = TRUE;

	return pos->width != 0;
}

glm::ivec2 get_index2pos(PangoLayout* layout, int idx) {
	PangoRectangle pos = {};
	pango_layout_index_to_pos(layout, idx, &pos);
	return glm::ivec2(pos.x / PANGO_SCALE, pos.y / PANGO_SCALE);
}
// 输入行号，索引号，方向，输出行号选择范围
glm::ivec4 text_ctx_cx::get_line_extents(int lidx, int idx, int dir)
{
	//pango_layout_get_line_count
	auto line = pango_layout_get_line(layout, lidx);
	auto lst = pango_layout_get_lines(layout);
	PangoRectangle ink_rect = {};
	PangoRectangle logical_rect = {};
	glm::ivec4 rt = {};
	gboolean at_line_end = 0;
	PangoRectangle tpos = {};
	if (line)
	{
		int h = 0;
		pango_layout_line_get_height(line, &h);
		h = h / PANGO_SCALE;
		auto xi = pango_layout_line_get_start_index(line);
		{
			gboolean rb = text_util_get_block_cursor_location(layout, idx > 0 ? idx : xi, &tpos, &at_line_end);
			pango_layout_get_cursor_pos(layout, idx > 0 ? idx : xi, &ink_rect, &logical_rect);
			glm::ivec4 r = { logical_rect.x,logical_rect.y,logical_rect.width,logical_rect.height }, r1 = { tpos.x,tpos.y,tpos.width,tpos.height };
			r /= PANGO_SCALE;
			r1 /= PANGO_SCALE;
			rt.x = r.x;
			rt.y = r.y;
			rt.w = h;
		}
		if (dir)
		{
			pango_layout_get_cursor_pos(layout, xi, &ink_rect, &logical_rect);
			rt.z = rt.x;
			rt.x = logical_rect.x / PANGO_SCALE;
		}
		else {
			pango_layout_line_get_pixel_extents(line, &ink_rect, &logical_rect);
			rt.z = logical_rect.width - rt.x;
		}
		int lh = 0;
		pango_layout_line_get_height(line, &lh);
		lh /= PANGO_SCALE;
	}
	return rt;
}
glm::ivec2 text_ctx_cx::get_line_length(int index)
{
	int x_pos = 0;
	int lidx = 0;
	pango_layout_index_to_line_x(layout, index, 0, &lidx, &x_pos);
	x_pos /= PANGO_SCALE;
	auto line = pango_layout_get_line(layout, lidx);
	int len = pango_layout_line_get_length(line);
	pango_layout_index_to_line_x(layout, len, 0, &lidx, &x_pos);
	glm::ivec2 ret = { line->start_index + len, x_pos / PANGO_SCALE };
	return ret;
}

glm::i64vec2 text_ctx_cx::get_bounds()
{
	glm::i64vec2 v = { bounds[0] , bounds[1] };
	if (v.x > v.y) { std::swap(v.x, v.y); }
	return v;
}
glm::ivec2 geti2x(PangoLayout* layout, int x)
{
	int x_pos = 0;
	int lidx = 0;
	pango_layout_index_to_line_x(layout, x, 0, &lidx, &x_pos);
	x_pos /= PANGO_SCALE;
	return glm::ivec2(x_pos, lidx);
}

glm::ivec4 text_ctx_cx::get_cursor_posv(PangoLayout* layout, int idx)
{
	std::vector<glm::ivec4> rv;
	PangoRectangle sw[2] = {};
	pango_layout_get_cursor_pos(layout, idx, &sw[0], &sw[1]);
	auto& w1 = sw[1];
	glm::ivec4 r = { w1.x,w1.y,w1.width,w1.height };
	r /= PANGO_SCALE;
	//r->y = lineheight * ly;
	return r;
}

std::vector<glm::ivec4> get_caret_posv(PangoLayout* layout, int idx)
{
	std::vector<glm::ivec4> rv;
	PangoRectangle sw[2] = {};
	pango_layout_get_caret_pos(layout, idx, &sw[0], &sw[1]);
	auto& w0 = sw[0];
	auto& w1 = sw[1];
	glm::ivec4 r = { w0.x,w0.y,w0.width,w0.height };
	glm::ivec4 r1 = { w1.x,w1.y,w1.width,w1.height };
	rv.push_back(r);
	rv.push_back(r1);
	for (auto& it : rv) {
		it /= PANGO_SCALE;
	}
	return rv;
}
int get_line_height(PangoLayout* layout, int idx) {
	auto line = pango_layout_get_line(layout, idx);
	int h = 0;
	pango_layout_line_get_height(line, &h);
	h = h / PANGO_SCALE;
	return h;
}

glm::ivec2 get_layout_size_(PangoLayout* layout)
{
	PangoRectangle ink, logical;
	pango_layout_get_extents(layout, &ink, &logical);
	return { logical.width / PANGO_SCALE,logical.height / PANGO_SCALE };
}
glm::ivec2 text_ctx_cx::get_layout_size()
{
	return get_layout_size_(layout);
}
struct it_rect
{
	glm::ivec4 line_rect = {}, char_rect = {};
	glm::ivec2 yr = {};
	int baseline = 0;
};
it_rect get_iter(PangoLayoutIter* iter) {

	it_rect ret = {};
	PangoRectangle lr = {}, cr = {};
	pango_layout_iter_get_line_extents(iter, NULL, (PangoRectangle*)&lr);
	pango_layout_iter_get_char_extents(iter, (PangoRectangle*)&cr);
	pango_layout_iter_get_line_yrange(iter, &ret.yr.x, &ret.yr.y);
	ret.baseline = pango_layout_iter_get_baseline(iter);
	ret.line_rect = glm::ivec4(lr.x, lr.y, lr.width, lr.height);
	ret.char_rect = glm::ivec4(cr.x, cr.y, cr.width, cr.height);
	ret.line_rect /= PANGO_SCALE;
	ret.char_rect /= PANGO_SCALE;
	ret.yr /= PANGO_SCALE;
	ret.baseline /= PANGO_SCALE;
	return ret;
}
std::vector<glm::ivec4> text_ctx_cx::get_bounds_px()
{
	std::vector<glm::ivec4> r;
	int x_pos = 0;
	int lidx = 0;
	auto v = get_bounds();
	auto v1 = geti2x(layout, v.x);
	auto v2 = geti2x(layout, v.y);
	if (v.x != v.y)
	{
		v = v;
	}
	auto vp1 = get_index2pos(layout, v.x);
	auto vp2 = get_index2pos(layout, v.y);

	auto nk = get_line_length(v.x);
	auto nk1 = get_line_length(v.y);
	auto ss = get_layout_size();
	auto sw0 = get_cursor_posv(layout, v.x);
	auto sw1 = get_cursor_posv(layout, v.y);
	std::vector<glm::ivec4> rs, rss;
	int line_no = pango_layout_get_line_count(layout);
	auto iter = pango_layout_get_iter(layout);
	auto pwidth = layout_get_char_width(layout) / PANGO_SCALE;
	for (int i = 0; i < line_no; i++)
	{
		auto rc = get_iter(iter);
		if (i != v1.y)
		{
			pango_layout_iter_next_line(iter); continue;
		}
		if (v1.y == v2.y)
		{
			rss.push_back({ rc.line_rect.x + sw0.x,rc.yr.x,sw1.x - sw0.x,rc.yr.y - rc.yr.x });
			break;
		}
		else
		{
			glm::ivec4 f = { rc.line_rect.x + sw0.x,rc.yr.x,(rc.line_rect.z - sw0.x) + pwidth,rc.yr.y - rc.yr.x };
			rss.push_back(f);
			pango_layout_iter_next_line(iter);
			for (size_t x = v1.y + 1; x < v2.y; x++)
			{
				rc = get_iter(iter);
				glm::ivec4 c = { rc.line_rect.x,rc.yr.x,rc.line_rect.z + pwidth,rc.yr.y - rc.yr.x };
				rss.push_back(c);
				pango_layout_iter_next_line(iter);
			}
			rc = get_iter(iter);
			f = { rc.line_rect.x ,rc.yr.x, sw1.x ,rc.yr.y - rc.yr.x };
			rss.push_back(f);
			break;
		}
	}

	pango_layout_iter_free(iter);
	if (roundselect)
	{
		PathsD subjects;
		PathD a;
		for (size_t i = 0; i < rss.size(); i++)
		{
			auto& it = rss[i];
			if (it.z < 1)
				it.z = pwidth;
			a.push_back({ it.x,it.y });
			a.push_back({ it.x + it.z,it.y });
			a.push_back({ it.x + it.z,it.y + it.w });
			a.push_back({ it.x,it.y + it.w });
			subjects.push_back(a);
			a.clear();
		}
		subjects = Union(subjects, FillRule::NonZero, 6);
		//range_path = InflatePaths(subjects, 0.5, JoinType::Round, EndType::Polygon);
		auto sn = subjects.size();
		if (sn > 0)
		{
			path_v& ptr = ptr_path;
			ptr._data.clear();
			for (size_t i = 0; i < sn; i++)
			{
				auto& it = subjects[i];
				ptr.add_lines((glm::dvec2*)it.data(), it.size(), false);
			}
			range_path = gp::path_round(&ptr, -1, fontsize * round_path, 16, 0, 0);
		}
		else { range_path.clear(); }
	}
	rangerc = rss;
	return r;
}

void text_ctx_cx::up_caret()
{
	auto kc = pango_layout_get_line_count(layout);
	auto lps = get_layout_position(layout);
	PangoRectangle sw[4] = {};
	auto v1 = geti2x(layout, ccursor);
	auto f = get_line_extents(v1.y, ccursor, 0);

	pango_layout_get_cursor_pos(layout, ccursor, &sw[0], &sw[1]);
	pango_layout_get_caret_pos(layout, ccursor, &sw[2], &sw[3]);
	glm::ivec4 caret = { sw->x,sw->y,sw[2].x,sw[2].y };
	caret /= PANGO_SCALE;
	int h = sw->height;
	h /= PANGO_SCALE;
	cursor_pos = caret; cursor_pos.z = h;
}
glm::ivec2 get_line_info_(PangoLayout* layout, int line)
{
	glm::ivec2 ret = {};
	int ct = pango_layout_get_line_count(layout);
	if (line >= ct)line = ct - 1;
	if (line < 0)line = 0;
	PangoLayoutLine* pl = pango_layout_get_line(layout, line);
	if (pl)
	{
		ret = { pl->start_index , pl->length };
	}
	return ret;
}

glm::ivec2 text_ctx_cx::get_line_info(int y)
{
	return get_line_info_(layout, y);
}

void renderer_draw_layout(rvg_cx* rv, PangoLayout* layout, int x, int y, int baseline)
{
	PangoLayoutIter* iter;
	g_return_if_fail(PANGO_IS_LAYOUT(layout));
	iter = pango_layout_get_iter(layout);
	do
	{
		PangoRectangle   logical_rect;
		PangoLayoutLine* line;
		//int              baseline;

		line = pango_layout_iter_get_line_readonly(iter);
		glm::ivec2 yy = {};
		pango_layout_iter_get_line_yrange(iter, &yy.x, &yy.y);
		pango_layout_iter_get_line_extents(iter, NULL, &logical_rect);
		if (baseline == 0)
			baseline = pango_layout_iter_get_baseline(iter);
		cairo_save(cr);
		yy /= PANGO_SCALE;
		rv->translate(x + logical_rect.x, y + baseline + yy.x);
		pango_cairo_show_layout_line(cr, line);
		cairo_restore(cr);
	} while (pango_layout_iter_next_line(iter));

	pango_layout_iter_free(iter);

}
bool text_ctx_cx::update(float delta)
{
	c_ct += delta * 1000.0 * c_d;
	if (c_ct > cursor.z)
	{
		c_d = -1;
		c_ct = cursor.z;
		valid = true;
	}
	if (c_ct < 0)
	{
		c_d = 1; c_ct = 0;
		valid = true;
	}
	if (!valid)return false;
	auto cr = cairo_create(sur);
#if 1 
	size_t length = dtimg.size();
	auto dt = dtimg.data();
	for (size_t i = 0; i < length; i++)
	{
		*dt = back_color; dt++;
	}
#else
	set_color(cr, back_color);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
#endif
	cairo_save(cr);
	pango_cairo_update_layout(cr, layout);

	auto pwidth = layout_get_char_width(layout) / PANGO_SCALE;
	if (upft)
	{
		_baseline = get_baseline();
		upft = false;
	}
	if (single_line) {
		glm::vec2 ext = { 0,lineheight }, ss = size;
		auto ps = ss * text_align - (ext * text_align);
		_align_pos.y = ps.y;
	}
	rv->translate(-scroll_pos.x + _align_pos.x, -scroll_pos.y + _align_pos.y);

	auto v = get_bounds();
	if (v.x != v.y && rangerc.size()) {
		set_color(cr, select_color);
		if (roundselect)
		{
			if (range_path.size() && range_path[0].size() > 3) {
				draw_polyline(cr, &range_path, true);
				cairo_fill(cr);
			}
		}
		else {
			for (auto& it : rangerc)
			{
				rv->add_rect(it, std::min(it.z, it.w) * 0.18);
				cairo_fill(cr);
			}
		}
	}
	set_color(cr, text_color);
	auto b = pango_layout_get_ellipsize(layout);
	pango_cairo_show_layout(cr, layout);
	cairo_restore(cr);
	cairo_destroy(cr);
	bool ret = valid;
	valid = false;
	return true;
}
uint32_t get_reverse_color(uint32_t color) {
	uint8_t* c = (uint8_t*)&color;
	c[0] = 255 - c[0];
	c[1] = 255 - c[1];
	c[2] = 255 - c[2];
	return color;
}
void text_ctx_cx::draw(rvg_cx* rv)
{
	//printf("text_ctx_cx::draw\t%s\n",);
	cairo_save(cr);
	// 裁剪区域
	cairo_rectangle(cr, pos.x, pos.y, size.x, size.y);
	cairo_clip(cr);
	auto ps = pos - scroll_pos + _align_pos;
	auto oldop = cairo_get_operator(cr);
	cairo_set_source_surface(cr, sur, pos.x, pos.y);
	cairo_paint(cr);
	double x = ps.x + cursor_pos.x, y = ps.y + cursor_pos.y;
	if (show_input_cursor && c_d == 1 && cursor.x > 0 && cursor_pos.z > 0)
	{
		set_color(cr, cursor.y);
		cairo_rectangle(cr, x, y, cursor.x, cursor_pos.z);
		cairo_fill(cr);
	}
	cairo_restore(cr);
	auto bbc = box_color;
	set_source_rgba(cr, bbc);
	cairo_rectangle(cr, pos.x - 0.5, pos.y - 0.5, size.x + 1, size.y + 1);
	cairo_set_line_width(cr, 1);
	cairo_stroke(cr);
	// 编辑中的文本
	if (editingstr.size())
	{
		cairo_save(cr);
		rv->translate(x, y);

		pango_cairo_update_layout(cr, layout_editing);
		glm::ivec2 lps = {};
		pango_layout_get_pixel_size(layout_editing, &lps.x, &lps.y);
		if (lps.y < lineheight)
			lps.y = lineheight;
		glm::vec4 lss = { 0,  lps.y + 0.5, lps.x, lps.y + 0.5 };

		set_color(cr, get_reverse_color(editing_text_color));
		cairo_rectangle(cr, 0, 0, lps.x, lps.y + 2);
		cairo_fill(cr);
		set_color(cr, editing_text_color);
		pango_cairo_show_layout(cr, layout_editing);

		cairo_move_to(cr, lss.x, lss.y);
		cairo_line_to(cr, lss.z, lss.w);
		cairo_set_line_width(cr, 1);
		cairo_stroke(cr);
		cairo_restore(cr);
	}
}
#endif
bool text_ctx_cx::hit_test(const glm::ivec2& ps)
{
	auto p2 = ps;
	glm::vec4 rc = { 0,0,size };
	auto k2 = check_box_cr(p2, &rc, 1);
	return (k2.x);
}

double dcscroll(double cp, double isx, double scroll_increment_x, int& scrollx)
{
	double ret = .0;
	if (cp < scrollx)
	{
		ret = floor(std::max(0.0, cp - scroll_increment_x));
		scrollx = ret;
	}
	else if (cp - isx >= scrollx && isx > 0)
	{
		ret = floor(cp - isx + scroll_increment_x);
		scrollx = ret;
	}
	return ret;
}

//template <typename T>
//inline T clamp(T v, T mn, T mx) { return (v < mn) ? mn : (v > mx) ? mx : v; }
// 输入：视图大小、内容大小、滚动宽度
// 输出：x水平大小，y垂直大小，z为水平的滚动区域宽，w垂直滚动高
glm::vec4 calcu_scrollbar_size(const glm::vec2 vps, const glm::vec2 content_width, const glm::vec2 scroll_width, const glm::ivec4& count)
{
	auto scw = scroll_width;
	scw.x = scroll_width.x * count.x + count.z;
	scw.y = scroll_width.y * count.y + count.w;
	auto dif = vps - scw;
	bool isx = (dif.x < content_width.x) && vps.x < content_width.x;
	bool isy = (dif.y < content_width.y) && vps.y < content_width.y;
	int inc = (isx ? 1 : 0) + (isy ? 1 : 0);
	if (!isy)
		scw.x -= count.z;
	if (!isx)
		scw.y -= count.w;
	auto calc_cb = [](double vw, double cw, double scw, double sw)
		{
			double win_size_contents_v = cw,
				win_size_avail_v = vw - scw,
				scrollbar_size_v = win_size_avail_v;//scrollbar_size_v是实际大小
			auto win_size_v = std::max(std::max(win_size_contents_v, win_size_avail_v), 1.0);
			auto GrabMinSize = sw;			// std::clamp
			auto grab_h_pixels = glm::clamp(scrollbar_size_v * (win_size_avail_v / win_size_v), GrabMinSize, scrollbar_size_v);
			auto grab_h_norm = grab_h_pixels / scrollbar_size_v;
			return glm::vec2{ grab_h_pixels, scrollbar_size_v };
		};
	auto x = calc_cb(vps.x, content_width.x, scw.x, scroll_width.x);
	auto y = calc_cb(vps.y, content_width.y, scw.y, scroll_width.y);
	if (!(x.x < x.y) || !isx)
		x.x = 0;
	if (!(y.x < y.y) || !isy)
		y.x = 0;
	glm::vec4 ret = { x.x, y.x, x.y, y.y };
	return ret;
}

// 输入：视图大小、内容大小、滚动宽度 ,count
// 输出：x水平大小，y为水平的滚动区域宽，z是否显示滚动条
glm::vec3 calcu_scrollbar_size(int vps, int content_width, int scroll_width, int count)
{
	//计算去掉按钮时视图大小
	auto scw = scroll_width;
	scw = scroll_width * count;
	auto dif = vps - scw;
	bool isx = (dif < content_width) && vps < content_width;

	auto calc_cb = [](double vw, double cw, double scw, double sw)
		{
			double win_size_contents_v = cw,
				win_size_avail_v = vw - scw,
				scrollbar_size_v = win_size_avail_v;
			auto win_size_v = std::max(std::max(win_size_contents_v, win_size_avail_v), 1.0);
			auto GrabMinSize = sw;
			auto grab_h_pixels = std::clamp(scrollbar_size_v * (win_size_avail_v / win_size_v), GrabMinSize, scrollbar_size_v);
			auto grab_h_norm = grab_h_pixels / scrollbar_size_v;
			return glm::vec2{ grab_h_pixels, scrollbar_size_v };
		};
	auto x = calc_cb(vps, content_width, scw, scroll_width);
	if (!(x.x < x.y) || !isx)
		x.x = 0;
	return glm::vec3(x.x, x.y, isx);
}

void text_ctx_cx::up_cursor(bool is)
{
	if (is)
	{
		up_caret();
		glm::ivec2 cs = cursor_pos;
		auto evs = size;		// 视图大小
		auto h = cursor_pos.z;	// 行高
		if (h < 1)h = 1;
		evs.x -= _align_pos.x;
		int ey = evs.y - cursor_pos.z;
		//ey *= h;
		glm::ivec2 pos = {};
		if (is_scroll) {
			dcscroll(cs.x, evs.x, 2, scroll_pos.x);
			dcscroll(cs.y, ey, h, scroll_pos.y);
		}
		else
		{
			scroll_pos = { .0, .0 };
		}
		if (!(cs.x < 0 || cs.y < 0))
		{
			pos.x += cs.x;
			pos.y += cs.y;
		}
	}
}
edit_tl::edit_tl()
{
	widget_t::wtype = WIDGET_TYPE::WT_EDIT;
	ctx = new text_ctx_cx();
	//set_family("NSimSun", 12);
	set_family(0, 12);
	set_align_pos({ 4, 4 });
	set_color({ 0xff353535,-1,0xa0ff8000 ,0xff020202 });
	on_event_cb = [=](uint32_t type, et_un_t* e, const glm::vec2& pos) {
		this->ppos = pos;
		this->on_event_e(type, e);
		};
}

edit_tl::~edit_tl()
{
	if (ctx)delete ctx; ctx = 0;
}
void edit_tl::set_single(bool is) {
	ctx->set_single(is);
	single_line = is;
}
void edit_tl::set_pwd(char ch)
{
	if (ctx)ctx->pwd = ch;
}
void edit_tl::set_family(int fontid, int fontsize) {
	if (fontsize > 0)
		ctx->fontsize = fontsize;
	ctx->fontid = fontid;
	//ctx->set_family(f);
}
void edit_tl::set_show_input_cursor(bool ab)
{
	ctx->show_input_cursor = ab;
}
void edit_tl::set_autobr(bool ab)
{
	ctx->set_autobr(ab);
}
void edit_tl::set_round_path(float v)
{
	ctx->round_path = std::max(0.0f, std::min(1.0f, v));
}
void edit_tl::inputchar(const char* str)
{
	int sn = strlen(str);
	if (!str || !sn || ctx->ccursor < 0 || ctx->ccursor>_text.size())
	{
		return;
	}
	ipt_text = str;
	std::string& sstr = ipt_text;
	ctx->single_line = single_line;
	if (single_line)
	{
		auto& v = sstr;
		v.erase(std::remove(v.begin(), v.end(), '\r'), v.end());
		v.erase(std::remove(v.begin(), v.end(), '\n'), v.end());
	}
	if (input_cb)
		input_cb(this, sstr);
	_text.insert(ctx->ccursor, sstr);
	sn = sstr.size();
	ctx->ccursor += sn;
	ctx->set_text(_text);
	ctx->set_bounds0({ ctx->ccursor ,ctx->ccursor });
	ctx->up_cursor(true);
	if (changed_cb)
		changed_cb(this);
}
bool edit_tl::remove_bounds()
{
	bool r = 0;
	auto v = ctx->get_bounds();
	if (v.x != v.y) {
		ctx->ccursor = v.x;
		remove_char(v.x, v.y - v.x);//删除选择的字符	 
		r = true;
		ctx->widths.clear();
		ctx->set_bounds0({ 0,0 });
	}
	return r;
}
void edit_tl::remove_char(size_t idx, int count)
{
	if (idx < _text.size() && count > 0)
	{
		_text.erase(idx, count);
		ctx->set_text(_text);
		ctx->ccursor = idx;
	}
}
void edit_tl::set_cursor(const glm::ivec3& c)
{
	if (ctx && c.z > 0)ctx->set_cursor(c);
}
uint32_t rgb2bgr(uint32_t c) {
	uint32_t r = c;
	auto c8 = (uint8_t*)&r;
	std::swap(c8[0], c8[2]);
	return r;
}
void edit_tl::set_color(const glm::ivec4& c) {
	ctx->back_color = rgb2bgr(c.x);
	ctx->text_color = c.y;
	ctx->select_color = c.z;
	ctx->editing_text_color = c.w;
}
void edit_tl::set_text(const void* str0, int len)
{
	char* str = (char*)str0;
	if (str)
	{
		if (len < 0)len = strlen(str);
		std::string nstr(str, len);
		if (nstr != _text)
		{
			_text.swap(nstr);
			ctx->set_text(_text);
		}
	}
	else {
		_text.clear();
		ctx->set_text(_text);
	}
	ctx->set_bounds0({ 0,0 });  ctx->ccursor = _text.size();
}
void edit_tl::add_text(const void* str0, int len)
{
	char* str = (char*)str0;
	if (str && *str)
	{
		if (len < 0)len = strlen(str);
		std::string nstr(str, len);
		auto ps = _text.size();
		_text += (nstr);
		ctx->set_text(_text);
		ctx->set_bounds0({ 0,0 });
		ctx->ccursor = ps + len;
	}
}
void edit_tl::set_size(const glm::ivec2& ss)
{
	_size = ss;
	if (ss.x > 0 && ss.y > 0)
		ctx->set_size(ss);
}
void edit_tl::set_pos(const glm::ivec2& ps) {
	ctx->pos = _pos = ps;
}
void edit_tl::set_align_pos(const glm::vec2& ps)
{
	ctx->_align_pos = ps;
}
void edit_tl::set_align(const glm::vec2& a)
{
	ctx->text_align = a;
}
glm::ivec4 edit_tl::input_pos() {
	auto p = this;
	glm::ivec2 cpos = p->ctx->cursor_pos;
	return { p->ppos + p->ctx->pos + cpos - p->ctx->scroll_pos + p->ctx->_align_pos
		,2, p->ctx->cursor_pos.z + 2 };
}
std::string edit_tl::get_select_str()
{
	auto cx = ctx->get_bounds();
	if (cx.x == cx.y)return "";
	return std::string(_text.substr(cx.x, cx.y - cx.x));
}
std::wstring edit_tl::get_select_wstr()
{
	auto cx = ctx->get_bounds();
	if (cx.x == cx.y)return L"";
	return std::wstring(hz::u8_to_u16(_text.substr(cx.x, cx.y - cx.x)));
}
// 发送事件到本edit
void edit_tl::on_event_e(uint32_t type, et_un_t* ep) {

	auto e = &ep->v;
	auto t = (devent_type_e)type;
	//if (!ctx->ltx) { ctx->ltx = ltx; ctx->get_bounds_px(); }
	//if (!ltx)return;
	switch (t)
	{
	case devent_type_e::mouse_move_e:
	{
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y }; mps -= ctx->pos + ppos;
		if (ctx->hit_test(mps) || mdown)
		{
			ctx->is_hover = true;
			p->cursor = (int)cursor_st::cursor_ibeam;//设置输入光标

			auto cx = ctx->get_xy_to_index(mps.x, mps.y, _text.c_str());
			auto bp = ctx->cur_select;

			if (bp.x != bp.y && (cx >= bp.x && cx < bp.y))
			{
				p->cursor = (int)cursor_st::cursor_arrow;
				ctx->hover_text = true;
			}
			else {
				ctx->hover_text = false;
			}
			if (mdown)
			{
				if (ctx->ckselect == 2 && ep->form)
				{
					ctx->ckselect = 3;
					std::wstring ws = get_select_wstr();
					ws.push_back(0);
					_istate = 0;

					printf("drag begin:%p\n", this);
					bool ok = dragdrop_begin(ws.c_str(), ws.size());
					printf("drag end:%p\n", this);
					if (ok && !_read_only && !_istate) {

						auto ccr = ctx->get_bounds();
						auto d = bp.y - bp.x;

						if (ctx->ccursor < bp.x)
						{
							bp += d;
						}
						else { ccr += d; }

						ctx->set_bounds0(bp);
						remove_bounds();
						//printf("%p\t%d\t%d\n", this, ccr.x, bp.x);
						if (ctx->ckselect != 3)
						{
							ctx->set_bounds0(ccr);
							ctx->ccursor = ccr.y;
						}
						ctx->cur_select = ctx->get_bounds();
						ctx->get_bounds_px();
						ctx->up_cursor(true); mdown = false;
					}
					break;
				}
				if (ctx->ckselect == 0)
				{
					auto ob = ctx->get_bounds0();
					ctx->set_bounds0({ ob.x,cx });
					ctx->ccursor = cx;
				}
				ctx->get_bounds_px();
				if (ctx->c_d != 0)
				{
					ctx->c_d = -1; ctx->c_ct = -1;// 更新光标
				}
				ctx->up_cursor(true);
			}
		}
		else if (ctx->is_hover) {
			p->cursor = (int)cursor_st::cursor_arrow;
			ctx->is_hover = false;
		}
	}
	break;
	case devent_type_e::mouse_button_e:
	{
		auto p = e->b;
		glm::ivec2 mps = { p->x,p->y }; mps -= ctx->pos + ppos;
		bool isequal = ctx->cpos == mps;
		ctx->cpos = mps;
		if (ctx->hit_test(mps))
		{
			ep->ret = 1;
			auto cx = ctx->get_xy_to_index(mps.x, mps.y, _text.c_str());
			if (cx < 0)cx = 0;
			if (cx > _text.size())cx = _text.size();
			if (p->button == 1)
			{
				if (p->down == 0 && mdown && isequal && p->clicks == 1) //左键单击
				{
					auto bp = ctx->cur_select;
					auto ckse = ctx->ckselect;
					if (bp.x != bp.y && (cx >= bp.x && cx < bp.y))
					{
						ctx->hover_text = false;
					}
					ctx->ckselect = 0;
					ctx->ccursor = cx;
					ctx->set_bounds0({});
					ctx->up_cursor(true);
				}
				if (p->down)
				{
					auto bp = ctx->cur_select;
					if (ctx->hover_text)
					{
						ctx->ckselect = 2;
					}
					else
					{
						ctx->ckselect = 0;
						ctx->ccursor = cx;
						ctx->set_bounds0({ cx,cx });
						ctx->up_cursor(true);
					}
					if (ep->form)
					{
						form_set_input_ptr(ep->form, get_input_state(this, 1));
						ctx->c_d = -1; is_input = true;
					}
					else {
						ctx->c_d = 0; is_input = false;
					}
					mdown = true;
				}
			}
		}
		if (!p->down)
		{
			ctx->cur_select = ctx->get_bounds();
			ctx->ckselect = 1;
			mdown = false;
		}
	}
	break;
	case devent_type_e::mouse_wheel_e:
	{
		if (ctx->is_hover)
		{
			auto p = e->w;
			glm::ivec2 mps = { p->x,p->y };
			auto& sp = ctx->scroll_pos;
			sp.y -= mps.y * ctx->lineheight;
			auto lss = ctx->get_layout_size();
			lss.y -= ctx->lineheight;
			if (sp.y < 0)
			{
				sp.y = 0;
			}
			if (sp.y > lss.y)
			{
				sp.y = lss.y;
			}
		}
	}
	break;
	case devent_type_e::keyboard_e:
	{
		if (is_input)
			on_keyboard(ep);
	}
	break;
	case devent_type_e::text_editing_e:
	{
		if (!is_input)break;
		auto& p = e->e;
		bool setimepos = false;
		if (ctx->caret_old != ctx->ccursor)
		{
			ctx->caret_old = ctx->ccursor; setimepos = true;
		}
		std::string str;
		if (p->text) {
			str = p->text;
		}
		if (str.size())
		{
			setimepos = true;
		}
		if (setimepos)
		{
			auto ipos = input_pos();// 计算输入法坐标
			p->x = ipos.x;
			p->y = ipos.y;
			p->w = ipos.z;
			p->h = ipos.w;
			//printf("text_editing_e:%s\t%d\n", str.c_str(), ipos.x);
		}
		ctx->set_editing(str);
	}
	break;
	case devent_type_e::text_input_e:
	{
		auto p = e->t;
		remove_bounds();
		inputchar(p->text);
		auto ipos = input_pos();// 计算输入法坐标
		p->x = ipos.x;
		p->y = ipos.y + 3;
		p->w = ipos.z;
		p->h = ipos.w;
	}
	break;
	case devent_type_e::finger_e:
	{
		auto p = e->f;
		glm::ivec2 mps = ctx->cpos;
	}
	break;
	case devent_type_e::mgesture_e:
	{

	}
	break;
	case devent_type_e::ole_drop_e:
	{
		auto p = e->d;	// 接收ole拖放数据
		glm::ivec2 mps = { p->x,p->y }; mps -= ctx->pos + ppos;
		ctx->cpos = mps;
		if (ctx->hit_test(mps))
		{
			ep->ret = 1;
			auto cx = ctx->get_xy_to_index(mps.x, mps.y, _text.c_str());
			auto bp = ctx->cur_select;
			//printf("%d\n", ctx->c_ct);
			if (ctx->ckselect == 3 && bp.x != bp.y && (cx >= bp.x && cx < bp.y))
			{
				ctx->hover_text = true;
				//if (p->has) { *(p->has) = 0; }
				ctx->up_cursor(true);
				break;
			}
			else {
				ctx->hover_text = false;
			}
			if (p->has) { *(p->has) = 1; }
			if (ep->form) { form_set_input_ptr(ep->form, get_input_state(this, 1)); }
			auto lastc = ctx->ccursor;
			ctx->ccursor = cx;
			is_input = true;
			mdown = false;
			if (ctx->c_d != 0)
			{
				ctx->c_d = -1; ctx->c_ct = -1;// 更新光标
			}
			if (p->count && p->str) {
				printf("drag input:%p\n", this);
				/*
				输入光标c、选区xy

				*/
				auto b = ctx->get_bounds();
				glm::ivec2 b0 = ctx->get_bounds0();
				int kb = b0.y - cx;
				bool r1 = false;
				if (b.x != b.y) {
					if (b0.y > cx) {
						_istate = remove_bounds();// 选区大于输入位置直接删除
						ctx->ccursor = cx;
					}
					else { r1 = true; }
				}
				int dc = (b0.x > b0.y) ? b.y - b.x : 0;//0是后

				auto c1 = ctx->ccursor;
				for (size_t i = 0; i < p->count; i++)
				{
					if (p->fmt == 1 && i) // 1是文件添加分隔符
					{
						inputchar("; ");
					}
					inputchar(p->str[i]);
				}
				auto cc = ctx->ccursor;
				c1 = cc - c1;//输入的长度 
				if (r1) {
					ctx->set_bounds0(b);
					_istate = remove_bounds();// 选区小于输入位置后删除 
				}
				//printf("dc:\t%d\txy%d %d kb %d\n", dc, b0.x, b0.y, kb);
				ctx->ccursor = cc;
				glm::ivec2 nb = { cc,cc };
				ctx->ckselect = 1;
				if (b.x != b.y) {
					if (r1)//后删除
					{
						nb.x = nb.y = cc - (b.y - b.x);
						if (dc == 0) {
							nb.x -= c1;//后光标
						}
						else {
							nb.y -= c1;//前光标
						}
					}
					else
					{
						// 前删除
						if (dc == 0) {
							nb.x -= c1;//后光标
						}
						else {
							nb.y -= c1;//前光标
						}
					}
				}
				ctx->set_bounds0(nb);
				ctx->ccursor = nb.y;
				//printf("ole %p\t%d %d \n", this, cx, ctx->ccursor); 
			}
			else
			{
				//ctx->set_bounds0({ cx,cx });
			}
		}
		else {
			ctx->c_d = 0; is_input = false;
		}
		ctx->get_bounds_px();
		ctx->cur_select = ctx->get_bounds();
		ctx->up_cursor(true);
	}
	break;
	default:
		break;
	}

}
glm::ivec2 get_cl(char* str, int cursor)
{
	auto cp = str + cursor;
	auto chp = md::get_utf8_first(cp);
	int ps = chp - cp;
	cursor += ps;
	int n = 0;
	for (size_t i = 0; i < cursor; i++)
	{
		if (str[i] == '\n') { n++; }
	}
	return { cursor, n };
}
int get_cl_count(char* str, int c0, int c1)
{
	int n = 0;
	for (size_t i = c0; i < c1; i++)
	{
		auto cp = str + i;
		auto chp = md::get_utf8_first(cp);
		int ps = chp - cp;
		i += ps;
		n++;
	}
	return n;
}
void edit_tl::on_keyboard(et_un_t* ep)
{
	auto p = ep->v.k;
	if (!p->down)
	{
		do {
			if (!p->kmod & 1)break;
			switch (p->keycode) {
			case SDLK_A:
			{
				ctx->ccursor = _text.size();
				ctx->set_bounds0({ 0,ctx->ccursor });
				ctx->get_bounds_px();
				ctx->up_cursor(true);
			}
			break;
			case SDLK_X:
			case SDLK_C:
			{
				//auto cp1 = ext->get_cpos2(0);
				//auto cp2 = ext->get_cpos2(1);
				//auto str = _storage_buf->get_range(cp1, cp2);
				auto rb = ctx->get_bounds();
				if (rb.x != rb.y)
				{
					auto str = _text.substr(rb.x, rb.y - rb.x);
					if (str.size())
					{
						set_clipboard(str.c_str());
						if (p->keycode == SDLK_X && !_read_only)
						{
							remove_bounds();
							ctx->up_cursor(true);
						}
					}
				}
			}
			break;
			case SDLK_V:
			{
				auto str = get_clipboard();
				remove_bounds();
				inputchar(str.c_str());
			}
			break;
			case SDLK_Y:
				//is_redo = true;	//_storage_buf->redo();
				break;
			case SDLK_Z:
				//is_undo = true;	//_storage_buf->undo();
				break;
			}
		} while (0);
	}
	if (!p->down || ctx->editingstr.size())
	{
		return;
	}

	bool isupcursor = false;
	switch (p->keycode)
	{
	case SDLK_TAB:
	{
		inputchar("\t");
	}
	break;
	case SDLK_BACKSPACE:
	{
		if (!remove_bounds()) {
			const char* str = _text.c_str();
			auto p = md::get_utf8_prev(str + ctx->ccursor - 1);
			size_t idx = p - str;
			size_t ct = ctx->ccursor - idx;
			remove_char(idx, ct);//删除一个字符

			ctx->up_cursor(true);
		}
	}
	break;
	case SDLK_PRINTSCREEN:
	{}
	break;
	case SDLK_SCROLLLOCK:
	{}
	break;
	case SDLK_PAUSE:
	{}
	break;
	case SDLK_INSERT:
	{

	}
	break;
	case SDLK_PAGEDOWN:
	{

	}
	break;
	case SDLK_PAGEUP:
	{

	}
	break;
	case SDLK_DELETE:
	{
		if (!remove_bounds()) {
			const char* str = _text.c_str();
			auto p = md::get_utf8_first(str + ctx->ccursor + 1);
			size_t idx = p - str;
			size_t ct = idx - ctx->ccursor;
			remove_char(ctx->ccursor, ct);
			ctx->up_cursor(true);
		}
	}
	break;
	case SDLK_HOME:
	{
		auto ts = _text.size();
		auto c = get_cl(_text.data(), ctx->ccursor);
		auto lp2 = ctx->get_line_info(c.y);
		ctx->ccursor = lp2.x;
		ctx->set_bounds0({});
		ctx->up_cursor(true);
	}
	break;
	case SDLK_END:
	{
		auto ts = _text.size();
		auto c = get_cl(_text.data(), ctx->ccursor);
		auto lp2 = ctx->get_line_info(c.y);
		ctx->ccursor = lp2.x + lp2.y;
		ctx->set_bounds0({});
		ctx->up_cursor(true);
	}
	break;
	case SDLK_RIGHT:
	{
		auto v = ctx->get_bounds();
		auto ts = _text.size();
		if (v.x != v.y) {
			ctx->ccursor = v.y;
			ctx->set_bounds0({});
		}
		else
		{
			const char* str = _text.c_str();
			auto p = md::get_utf8_first(str + ctx->ccursor + 1);
			size_t idx = p - str;
			ctx->ccursor = idx;
		}
		if (ctx->ccursor > ts)ctx->ccursor = ts;
		ctx->up_cursor(true);
	}
	break;
	case SDLK_LEFT:
	{
		auto v = ctx->get_bounds();
		if (v.x != v.y) {
			ctx->ccursor = v.x;
			ctx->set_bounds0({});
			break;
		}
		else
		{
			const char* str = _text.c_str();
			auto p = md::get_utf8_prev(str + ctx->ccursor - 1);
			size_t idx = p - str;
			ctx->ccursor = idx;
		}
		if (ctx->ccursor < 0)ctx->ccursor = 0;
		ctx->up_cursor(true);
	}
	break;
	case SDLK_DOWN:
	{
		auto ts = _text.size();
		auto c = get_cl(_text.data(), ctx->ccursor);
		auto& idx = c.y;
		auto lpo2 = ctx->get_line_info(idx);
		auto lp2 = ctx->get_line_info(++idx);
		auto x = get_cl_count(_text.data(), lpo2.x, ctx->ccursor);
		if (x >= lp2.y)
		{
			x = lp2.x + lp2.y;
		}
		else
		{
			const char* str = _text.c_str() + lp2.x;
			auto p = md::utf8_char_pos(str, x, -1);
			x = p - _text.c_str();
		}
		ctx->ccursor = x;
		ctx->set_bounds0({});
		ctx->up_cursor(true);
	}
	break;
	case SDLK_UP:
	{
		auto ts = _text.size();
		auto c = get_cl(_text.data(), ctx->ccursor);
		auto& idx = c.y;
		auto lpo2 = ctx->get_line_info(idx);
		auto lp2 = ctx->get_line_info(--idx);
		auto x = get_cl_count(_text.data(), lpo2.x, ctx->ccursor);
		if (x >= lp2.y)
		{
			x = lp2.x + lp2.y;
		}
		else
		{
			const char* str = _text.c_str() + lp2.x;
			auto p = md::utf8_char_pos(str, x, -1);
			x = p - _text.c_str();
		}
		ctx->ccursor = x;
		ctx->set_bounds0({});
		ctx->up_cursor(true);
	}
	break;
	case SDLK_RETURN:
	{
		remove_bounds();
		if (!single_line)
		{
			inputchar("\n");
		}
		ctx->up_cursor(true);
	}
	break;
	default:
		break;
	}
}

// 更新渲染啥的
bool edit_tl::update(float delta) {
	if (!ctx->family)
	{
		ctx->family = family;
	}
	if (!is_input)
	{
		ctx->c_d = 0;
	}
	glm::ivec2 ss = _size;
	if (ctx->size != ss) {
		ctx->set_size(ss);
	}
	glm::ivec2 ps = _pos;
	if (ctx->pos != ps) {
		ctx->pos = ps;
	}
	return ctx->update(delta);
}
// 获取纹理/或者渲染到cairo
//image_ptr_t* edit_tl::get_render_data() {
//	return &ctx->cacheimg;
//}

void edit_tl::draw(rvg_cx* rv)
{
	ctx->draw(rv);
}


#ifdef INPUT_STATE_TH
input_state_t* get_input_state(void* ptr, int t)
{
	static input_state_t r = {};
	if (t)
	{
		if (r.ptr)
		{
			auto p = (edit_tl*)r.ptr;
			p->ctx->editingstr.clear();
			p->is_input = false;
		}
		r.ptr = ptr;
	}
	if (ptr && r.ptr)
	{
		auto p = (edit_tl*)r.ptr;
		if (p) {
			*((glm::ivec4*)&r.x) = p->input_pos();
			r.y += 3;
		}
		if (!r.cb)
			r.cb = [](uint32_t type, et_un_t* e, void* ud) { if (ud) { ((edit_tl*)ud)->on_event_e(type, e); }	};
	}
	return &r;
}

#endif // INPUT_STATE_TH

#endif // !NO_EDIT

#ifdef NO_TVIEW

class tview_x
{
public:
	glm::ivec2 vpos = {}, size = {};	// 渲染偏移，大小 
	std::vector<glm::vec4> ddrect;
	// 填充颜色 
	uint32_t clear_color = 0;
	d2_surface_t* _backing_store = 0;
	d2_surface_t* rss = 0;
	image_ptr_t img_rc = {};
	std::vector<uint32_t> _imgdata;
	glm::mat3 mx = glm::mat3(1.0);
	glm::vec2 last_mouse = {}, eoffset = {};
	glm::ivec4 hover_bx = {}, bx = {};		//当前鼠标对象包围框
	int ckinc = 0;
	int scaleStep = 2;
	int scale = 100;
	int oldscale = 0;
	int minScale = 2, maxScale = 25600;
	bool   _backing_store_valid = false;
	bool   has_move = false;
	bool   has_scale = false;
public:
	tview_x();
	~tview_x();
	void set_size(const glm::ivec2& ss);
	void set_view_move(bool is);	// 鼠标移动视图
	void set_view_scale(bool is);	// 滚轮缩放视图
	void set_rss(int r);
	void on_button(int idx, int down, const glm::vec2& pos, int clicks);
	void on_motion(const glm::vec2& ps);
	void on_wheel(int deltaY);
	void reset_view();
	image_ptr_t* get_ptr();
	glm::ivec4 get_hbox();
	glm::vec4 get_bbox();
	glm::mat3 get_affine();
	void hit_test(const glm::vec2& ps);
#ifdef _CR__
	cairo_t* begin_frame(bool redraw);
	void end_frame(cairo_t* cr);
#endif
	void set_draw_update()
	{
		_backing_store_valid = false;
	}
private:

};


tview_x::tview_x()
{
	//rss = new_clip_rect(5);
}

tview_x::~tview_x()
{
#ifdef _CR__
	if (rss)
	{
		cairo_surface_destroy(rss);
	}
	rss = 0;
	if (_backing_store)
	{
		cairo_surface_destroy(_backing_store);
	}
#endif
	_backing_store = 0;
}
image_ptr_t* tview_x::get_ptr()
{
	return  &img_rc;
}

#ifdef _CR__
cairo_t* tview_x::begin_frame(bool redraw)
{
	cairo_t* cr = 0;
	if (redraw || !_backing_store_valid && _backing_store)
	{
		cr = cairo_create(_backing_store);
		size_t length = size.x * size.y;
		auto img = (uint32_t*)cairo_image_surface_get_data(_backing_store);
		if (clear_color == 0) {
			memset(img, 0, length * sizeof(int));
		}
		else {
			for (size_t i = 0; i < length; i++)
			{
				img[i] = clear_color;
			}
		}

		//cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
		if (oldscale != scale)
		{
			oldscale = scale;
			//print_time a("canvas draw");
			auto m = get_affine();
		}

	}
	return cr;
}
void tview_x::end_frame(cairo_t* cr) {

	if (!_backing_store_valid)
	{
		_backing_store_valid = true;
		if (rss)
			clip_rect(cr, rss);
		cairo_destroy(cr);
	}
}
#endif


void tview_x::set_size(const glm::ivec2& ss)
{
	if (ss != size)
	{
		size = ss;
#ifdef _CR__
		if (_backing_store)
		{
			cairo_surface_destroy(_backing_store);
			_backing_store = 0;
		}
		_imgdata.resize(size.x * size.y);
		if (!_backing_store)
		{
			_backing_store = cairo_image_surface_create_for_data((unsigned char*)_imgdata.data(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
		}
		auto img = (uint32_t*)cairo_image_surface_get_data(_backing_store);
		img_rc.width = size.x;
		img_rc.height = size.y;
		img_rc.type = 1;
		img_rc.comp = 4;
		img_rc.data = img;
		img_rc.multiply = true;
#endif
	}
}

void tview_x::set_view_move(bool is)
{
	has_move = is;
}

void tview_x::set_view_scale(bool is)
{
	has_scale = is;
}

void tview_x::set_rss(int r)
{
	if (r > 0)
	{
#ifdef _CR__
		if (rss)
		{
			cairo_surface_destroy(rss);
		}
		rss = new_clip_rect(r);
#endif
	}
}



void tview_x::on_button(int idx, int down, const glm::vec2& pos1, int clicks)
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
			//reset_view();
		}
	}
}
void tview_x::on_motion(const glm::vec2& pos1)
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
void tview_x::on_wheel(int deltaY)
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
void tview_x::reset_view()
{
	ckinc = 0;
	scale = 100;
	mx = glm::mat3(1.0);
	_backing_store_valid = false;
}

glm::ivec4 tview_x::get_hbox()
{
	return hover_bx;
}
glm::vec4 tview_x::get_bbox()
{
	return bx;
}

glm::mat3 tview_x::get_affine()
{
	glm::mat3 r = glm::translate(glm::mat3(1.0), glm::vec2(vpos));
	return r * mx;
}

// 测试鼠标坐标的矩形
void tview_x::hit_test(const glm::vec2& ps)
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

#endif // !NO_TVIEW



// 杂算法
#if 1
namespace pn {
	void tobox(const glm::vec2& v, glm::vec4& t)
	{
		if (v.x < t.x)
		{
			t.x = v.x;
		}
		if (v.y < t.y)
		{
			t.y = v.y;
		}
		if (v.x > t.z)
		{
			t.z = v.x;
		}
		if (v.y > t.w)
		{
			t.w = v.y;
		}

	}

	/*
	*
	*  0中心
	*   1     2
		-------
		|     |
		-------
		4     3
	*/
	glm::vec2 getbox2t(std::vector<glm::vec2>& vt, int t)
	{
		glm::vec4 box = { INT_MAX,INT_MAX,INT_MIN,INT_MIN };
		for (auto& it : vt)
		{
			tobox(it, box);
		}
		glm::vec2 cp = { box.z - box.x,box.w - box.y };
		switch (t)
		{
		case 0:
			cp *= 0.5;
			break;
		case 1:
			cp = {};
			break;
		case 2:
			cp.y = 0;
			break;
		case 3:
			break;
		case 4:
			cp.x = 0;
			break;
		default:
			break;
		}
		cp.x += box.x;
		cp.y += box.y;
		return cp;
	}

	glm::vec2 getbox2t(const glm::vec4& box, int t)
	{
		glm::vec2 cp = { box.z - box.x,box.w - box.y };
		switch (t)
		{
		case 0:
			cp *= 0.5;
			break;
		case 1:
			cp = {};
			break;
		case 2:
			cp.y = 0;
			break;
		case 3:
			break;
		case 4:
			cp.x = 0;
			break;
		default:
			break;
		}
		cp.x += box.x;
		cp.y += box.y;
		return cp;
	}

	/*
	缩放模式
	xy从中心扩展
	固定1左上角，2右上角，3右下角，4左下角，来缩放xy
	单扩展 x左右、y上下
	获取图形包围盒，返回坐标t=0中心，1左上角，2右上角，3右下角，4左下角，
	*/
	glm::mat3 box_scale(const glm::vec4& box, int ddot, glm::vec2 sc, bool inve)
	{
		auto cp = getbox2t(box, ddot);
		glm::vec2 cp0 = {};
		glm::vec2 scale = sc;
		glm::mat3 m(1.0);
		// 缩放
		assert(scale.x > 0 && scale.y > 0);
		if (scale.x > 0 && scale.y > 0)
		{
			m = glm::translate(glm::mat3(1.0f), cp) * glm::scale(glm::mat3(1.0f), scale) * glm::translate(glm::mat3(1.0f), -cp);
			if (inve)
			{
				m = glm::inverse(m);	// 反向操作 
			}
		}
		return m;
	}
}
//!pn
#endif // 1

#if 1
/*
<表格、树形>
数据结构设计:
	网格线：		虚线/实线、颜色
	背景：		纯色
	内容store：	数值、文本、简单公式、图片/SVG

*/


#endif // 1


#if 1
// 默认按钮样式

image_btn::image_btn() :widget_t(WIDGET_TYPE::WT_IMAGE_BTN)
{

}
image_btn::~image_btn()
{}
color_btn::color_btn() :widget_t(WIDGET_TYPE::WT_COLOR_BTN)
{}
color_btn::~color_btn()
{}
gradient_btn::gradient_btn() :widget_t(WIDGET_TYPE::WT_GRADIENT_BTN)
{}
gradient_btn::~gradient_btn()
{}
radio_tl::radio_tl() :widget_t(WIDGET_TYPE::WT_RADIO)
{}
checkbox_tl::checkbox_tl() :widget_t(WIDGET_TYPE::WT_CHECKBOX)
{}
checkbox_tl::~checkbox_tl()
{}
switch_tl::switch_tl() :widget_t(WIDGET_TYPE::WT_SWITCH)
{}
switch_tl::~switch_tl()
{}
progress_tl::progress_tl() :widget_t(WIDGET_TYPE::WT_PROGRESS)
{}
progress_tl::~progress_tl()
{}
slider_tl::slider_tl() :widget_t(WIDGET_TYPE::WT_SLIDER)
{}
slider_tl::~slider_tl()
{}
colorpick_tl::colorpick_tl() :widget_t(WIDGET_TYPE::WT_COLORPICK)
{}
colorpick_tl::~colorpick_tl()
{}
scroll_bar::scroll_bar() :widget_t(WIDGET_TYPE::WT_SCROLL_BAR)
{}
scroll_bar::~scroll_bar()
{}


bool image_btn::on_mevent(int type, const glm::vec2& mps, void* e)
{
	return false;
}

bool image_btn::update(float) {
	return false;
}

class btn_colorlist
{
public:
	btn_colorlist();
	~btn_colorlist();
	btn_cols_t* get(size_t idx) {
		return (idx < bcs.size()) ? (btn_cols_t*)bcs[idx].data() : nullptr;
	}
private:
	std::vector<std::array<uint32_t, 8>> bcs = { { 0xFFffffff, 0xFF409eff, 0xFF409eff, 0xFF66b1ff, 0xFFe6e6e6, 0xFF0d84ff, 0xFF0d84ff ,0 },
			{ 0xFFffffff, 0xFF67c23a, 0xFF67c23a, 0xFF85ce61, 0xFFe6e6e6, 0xFF529b2e, 0xFF529b2e ,0},
			{ 0xFFffffff, 0xFF909399, 0xFF909399, 0xFFa6a9ad, 0xFFe6e6e6, 0xFF767980, 0xFF767980 ,0},
			{ 0xFFffffff, 0xFFe6a23c, 0xFFe6a23c, 0xFFebb563, 0xFFe6e6e6, 0xFFd48a1b, 0xFFd48a1b ,0 },
			{ 0xFFffffff, 0xFFf56c6c, 0xFFf56c6c, 0xFFf78989, 0xFFe6e6e6, 0xFFf23c3c, 0xFFf23c3c ,0 }
	};
};

btn_colorlist::btn_colorlist()
{
	for (auto& it : bcs)
	{
		for (size_t i = 0; i < 8; i++)
		{
			auto c = (uint8_t*)&it[i];
			std::swap(c[0], c[2]);
		}
	}
}

btn_colorlist::~btn_colorlist()
{}
btn_cols_t* color_btn::set_btn_color_bgr(size_t idx)
{
	static btn_colorlist clst;
	auto ret = clst.get(idx);
	if (ret)
	{
		pdc = *ret;
	}
	return ret;
}
#define UF_COLOR
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
inline uint32_t set_alpha_x(uint32_t c, uint32_t a0)
{
	u_col* t = (u_col*)&c;
	double af = a0 / FCV;
	double a = t->c.a * af + 0.5;
	if (a > 255)
		t->c.a = 255;
	else
		t->c.a = a;
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

const char* gradient_btn::c_str()
{
	return str.c_str();
}

void gradient_btn::init(glm::ivec4 rect, const std::string& text, uint32_t back_color, uint32_t text_color)
{
	auto p = this;
	auto& info = *p;
	info._pos = { rect.x, rect.y };
	info._size = { rect.z, rect.w };
	info.rounding = 4;
	info.back_color = back_color;
	info.text_color = text_color;
	info.opacity = 1;
	info.str = text.c_str();
	info.borderLight = 0xff5c5c5c;
	info.borderDark = 0xff1d1d1d;
	return;
}
bool gradient_btn::update(float delta)
{
	auto p = this;
	if (!p)return false;
	if (_bst == _old_bst)return false;
	_old_bst = _bst;
	auto& info = *p;

	// (sta & hz::BTN_STATE::STATE_FOCUS)
	uint32_t gradTop = info.gradTop.x;// 0xff4a4a4a; 
	uint32_t gradBot = info.gradBot.x;// 0xff3a3a3a;

	info.mPushed = (_bst & (int)BTN_STATE::STATE_ACTIVE);
	info.mMouseFocus = (_bst & (int)BTN_STATE::STATE_HOVER);
	if (_bst & (int)BTN_STATE::STATE_DISABLE)
		info.mEnabled = false;
	if (info.mPushed) {
		gradTop = info.gradTop.z;//0xff292929;
		gradBot = info.gradBot.z;//0xff1d1d1d;
	}
	else if (info.mMouseFocus && info.mEnabled) {
		gradTop = info.gradTop.y;// 0x80404040;
		gradBot = info.gradBot.y;//0x80303030;
	}
	info._gradTop = gradTop;
	info._gradBot = gradBot;
	return true;
}



bool color_btn::update(float delta)
{
	dtime += delta;
	auto dt = dtime;
	if (dt > 0.150) {
		if (str != text)
			text = str;
		dtime = 0;
	}
	else
	{
	}
	if (_bst == _old_bst)return false;
	_old_bst = _bst;
	auto p = this;
	btn_cols_t* pdc = &p->pdc;
	p->_disabled = (_bst & (int)BTN_STATE::STATE_DISABLE);
	if (p->_disabled)
	{
		p->hover = false;
		p->dfill = pdc->background_color;
		p->dcol = pdc->border_color;
		if (p->effect == uTheme::dark)
		{
			p->dcol = 0;
			p->dtext_color = (p->text_color) ? p->text_color : pdc->font_color;
		}
		if (p->effect == uTheme::light)
		{
			p->dcol = set_alpha_xf(p->dcol, p->light * 3);
			p->dfill = set_alpha_xf(p->dfill, p->light);
			p->dtext_color = (p->text_color) ? p->text_color : pdc->border_color;
		}
		if (p->effect == uTheme::plain)
		{
			p->dfill = 0;
			p->dtext_color = (p->text_color) ? p->text_color : pdc->border_color;
		}
		p->dcol = set_alpha_x(p->dcol, p->disabled_alpha);
		p->dfill = set_alpha_x(p->dfill, p->disabled_alpha);
		p->dtext_color = set_alpha_x(p->dtext_color, p->disabled_alpha);
	}
	else
	{
		bool isdown = mPushed = (_bst & (int)BTN_STATE::STATE_ACTIVE);
		p->hover = (_bst & (int)BTN_STATE::STATE_HOVER);
		if (isdown)
		{
			p->dfill = pdc->active_background_color;
			p->dcol = pdc->active_border_color;
			if (p->effect == uTheme::plain)
			{
				p->dfill = set_alpha_xf(p->dcol, p->light);
			}
			else {
				p->dtext_color = (p->text_color) ? p->text_color : pdc->active_font_color;
			}
			if (p->effect == uTheme::light)
			{
				p->dfill = set_alpha_f(p->dfill, p->light * 5);
				p->dcol = set_alpha_f(p->dcol, p->light * 6);
			}
			if (p->effect == uTheme::dark)
			{
				p->dcol = 0;
			}
		}
		else if (p->hover)
		{
			uint32_t ac = pdc->hover_color;
			p->dfill = pdc->hover_color;
			if (pdc->hover_border_color)
				p->dcol = pdc->hover_border_color;

			if (p->effect == uTheme::plain)
			{
				p->dcol = pdc->border_color;
				p->dfill = set_alpha_xf(p->dcol, p->light);
			}
			if (p->effect == uTheme::light)
			{
				p->dfill = set_alpha_f(p->dfill, p->light * 5);
				p->dtext_color = (p->text_color) ? p->text_color : pdc->font_color;
			}
		}
		else {
			p->dfill = pdc->background_color;
			p->dcol = pdc->border_color;
			if (p->effect == uTheme::dark)
			{
				p->dcol = 0;
				p->dtext_color = (p->text_color) ? p->text_color : pdc->font_color;
			}
			if (p->effect == uTheme::light)
			{
				p->dcol = set_alpha_xf(p->dcol, p->light * 3);
				p->dfill = set_alpha_xf(p->dfill, p->light);
				p->dtext_color = (p->text_color) ? p->text_color : pdc->border_color;
			}
			if (p->effect == uTheme::plain)
			{
				p->dfill = 0;
				p->dtext_color = (p->text_color) ? p->text_color : pdc->border_color;
			}
		}
	}
	return true;
}

#endif // 1



template<class T>
void free_obt(T*& p) {
	if (p) {
		delete p; p = 0;
	}
}





// todo ui



radio_tl::~radio_tl()
{
	if (gr && gr->ct > 0) {
		gr->ct--;
		if (gr->ct <= 0)
			delete gr;
	}
	gr = 0;
}

void radio_tl::bind_ptr(bool* p)
{}

void radio_tl::set_value(const std::string& str, bool bv)
{
	radio_info_t k = {};
	k.text = str;
	k.value = bv; k.value1 = !bv;
	v = k;
	if (bv)
	{
		set_value();
	}
	else {
		if (v.on_change_cb) { v.on_change_cb(this, v.value); }
	}
}

void radio_tl::set_value(bool bv)
{
	v.value = bv;
	if (bv)
	{
		set_value();
	}
	else {
		if (v.on_change_cb) { v.on_change_cb(this, v.value); }
	}
}

void radio_tl::set_value()
{
	v.value = true;
	if (gr && this != gr->active)
	{
		if (gr->active)
			gr->active->set_value(false);
		gr->active = this;
		if (v.on_change_cb) { v.on_change_cb(this, v.value); }
	}
}

bool radio_tl::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	if (et == event_type2::on_click)
	{
	}
	return false;
}

bool radio_tl::update(float delta)
{
	int ic = 0;
	{
		auto& it = v;
		if (_size.x <= 0) {
			_size.x = style.radius * 2;
		}
		if (_size.y <= 0) {
			_size.y = style.radius * 2;
		}
		if (cks > 0 && v.value == v.value1) {
			cks = 0; set_value();
		}
		if (it.value != it.value1)
		{
			auto dt = fmod(delta, style.duration);
			if (style.duration > 0) {
				it.dt += delta;
				if (it.dt >= style.duration) {
					it.value1 = it.value; it.dt = 0;
					dt = 1.0;

					_old_bst = _bst;
				}
				else
				{
					dt = it.dt / style.duration;
				}
			}
			else {
				dt = 1.0;
			}
			float t = it.value ? glm::mix(style.radius - style.thickness, style.thickness * 2.0f, dt) : glm::mix(style.thickness * 2.0f, style.radius - style.thickness, dt);
			it.swidth = t;
			if (!it.value && it.dt == 0)it.swidth = 0;
			ic++;
		}
	}
	return ic > 0;
}


void checkbox_tl::bind_ptr(bool* p)
{}

void checkbox_tl::set_value(const std::string& str, bool bv)
{
	checkbox_info_t k = {};
	k.text = str;
	k.value = bv; k.value1 = !bv;
	v = k;
	if (v.on_change_cb) { v.on_change_cb(this, v.value); }
}

void checkbox_tl::set_value(bool bv)
{
	v.value = bv;
	if (v.on_change_cb) { v.on_change_cb(this, v.value); }
}

void checkbox_tl::set_value()
{
	v.value1 = v.value;
	v.value = !v.value;
	if (v.on_change_cb) { v.on_change_cb(this, v.value); }
}

bool checkbox_tl::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	if (et == event_type2::on_click)
	{
	}
	return false;
}

bool checkbox_tl::update(float delta)
{
	int ic = 0;

	{
		auto& it = v;
		if (_size.x <= 0) {
			_size.x = style.square_sz;
		}
		if (_size.y <= 0) {
			_size.y = style.square_sz;
		}

		if (cks > 0 && v.value == v.value1) {
			cks = 0; set_value();
		}
		auto duration = it.duration > 0 ? it.duration : style.duration;
		if (it.value != it.value1)
		{
			auto dt = fmod(delta, duration);
			if (duration > 0)
			{
				it.dt += delta;
				if (it.dt >= duration) {
					it.value1 = it.value; it.dt = 0;
					dt = 1.0;
					_old_bst = _bst;
				}
				else
				{
					dt = it.dt / duration;
				}
			}
			else {
				dt = 1.0;
			}

			float t = it.value ? glm::mix(0.0f, 1.0f, dt) : glm::mix(1.0f, 0.0f, dt);
			it.new_alpha = t;
			ic++;
		}
	}
	return ic > 0;
}


void switch_tl::bind_ptr(bool* p)
{
	v.pv = p;
}

void switch_tl::set_value(bool b)
{
	v.value = b;
	v.value1 = !b;
	if (v.on_change_cb) { v.on_change_cb(this, v.value); }
}

void switch_tl::set_value()
{
	v.value1 = v.value;
	v.value = !v.value;
	if (v.on_change_cb) { v.on_change_cb(this, v.value); }
}

bool switch_tl::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	if (et == event_type2::on_click)
	{
	}
	return false;
}

bool switch_tl::update(float delta)
{
	int ic = 0;
	auto& it = v;
	if (cks > 0 && v.value == v.value1) {
		cks = 0; set_value();
	}
	if (it.value != it.value1)
	{
		auto dt = fmod(delta, it.duration);
		if (it.duration > 0) {
			it.dt += delta;
			if (it.dt >= it.duration) {
				it.value1 = it.value; it.dt = 0;
				dt = 1.0;
				_old_bst = _bst;
				if (v.pv) {
					*v.pv = it.value;
				}
			}
			else
			{
				dt = it.dt / it.duration;
			}
		}
		else {
			dt = 1.0;
		}
		dcol = it.value ? glm::mix(color.y, color.x, 1.0f) : glm::mix(color.x, color.y, 1.0f);
		cpos = it.value ? glm::mix(0.0f, 1.0f, dt) : glm::mix(1.0f, 0.0f, dt);
		ic++;
	}
	return ic > 0;
}

void progress_tl::set_value(double b)
{
	if (b < 0)b = 0;
	if (b > 1)b = 1;
	value = b;
	if (format.size())
	{
		double k = get_v();
		std::string vv;
		text = pg::to_string(k) + format;
		//width = _size.x;
		if (text.size() && !text_inside) {
			//todo auto rk = ltx->get_text_rect(0, font_size, text.c_str(), -1);
			//size.x = width + rk.x + rounding * 0.5;
			if (parent)parent->uplayout = true;
		}
	}
}

void progress_tl::set_vr(const glm::ivec2& r)
{
	vr = r;
}

double progress_tl::get_v()
{
	return glm::mix(vr.x, vr.y, value);
}

bool progress_tl::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	return false;
}

bool progress_tl::update(float delta)
{
	return false;
}


void slider_tl::bind_ptr(double* p)
{}

void slider_tl::set_value(double b)
{
	if (b < 0)b = 0;
	if (b > 1)b = 1;
	value = b;

}

void slider_tl::set_vr(const glm::ivec2& r)
{
	vr = r;
}

void slider_tl::set_cw(int cw)
{
	sl.x = cw;
}

double slider_tl::get_v()
{
	return glm::mix(vr.x, vr.y, value);
}

bool slider_tl::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	glm::ivec2 poss = mps - _pos;
	glm::ivec2 ss = _size;
	if (et == event_type2::on_down) {

	}
	if (et == event_type2::on_click)
	{
		if (poss[vertical] >= 0)
		{
			double xf = (double)poss[vertical] / ss[vertical];
			if (xf > 1)xf = 1;
			if (xf < 0)xf = 0;
			value = xf;
		}
	}
	if (et == event_type2::on_drag)
	{
		poss += curpos;
		if (poss[vertical] >= 0)
		{
			double xf = (double)poss[vertical] / ss[vertical];
			if (xf > 1)xf = 1;
			if (xf < 0)xf = 0;
			value = xf;
		}
	}
	return false;
}

bool slider_tl::update(float delta)
{
	return false;
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
glm::vec4 RGBtoHSV(glm::u8vec4* c)
{
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
void HSVtoRGB(const glm::vec4& hsv, glm::vec4& otc)
{
	float h = hsv.x, s = hsv.y, v = hsv.z;
	otc.w = hsv.w;
	if (s == 0.0f)
	{
		// gray
		otc.x = otc.y = otc.z = v;
		return;
	}
	h = fmod(h, 1.0f) / (60.0f / 360.0f);
	int   i = (int)h;
	float f = h - (float)i;
	float p = v * (1.0f - s);
	float q = v * (1.0f - s * f);
	float t = v * (1.0f - s * (1.0f - f));

	switch (i)
	{
	case 0: otc.x = v; otc.y = t; otc.z = p; break;
	case 1: otc.x = q; otc.y = v; otc.z = p; break;
	case 2: otc.x = p; otc.y = v; otc.z = t; break;
	case 3: otc.x = p; otc.y = q; otc.z = v; break;
	case 4: otc.x = t; otc.y = p; otc.z = v; break;
	case 5: default: otc.x = v; otc.y = p; otc.z = q; break;
	}
}

void colorpick_tl::init(uint32_t c, int w, int h, bool alpha)
{
	set_color2hsv(c);
	width = w;
	height = h;
	//if (height < font_size)
	//	height = ltx->get_lineheight(0, font_size);
	h = height + step;
	cpx = height * 2.5;
	int minw = cpx + step * 2;
	if (width < minw) {
		width = minw + h;
	}
	_size.x = width;
	int hn = 4;
	if (alpha)hn++;
	_size.y = h * hn;
}

uint32_t colorpick_tl::get_color()
{
	glm::vec4 hc = {};
	HSVtoRGB(hsv, hc);
	//auto hc1 = convertHSVtoRGB(hsv);
	glm::u8vec4 c = { (int)(hc.x * 255), (int)(hc.y * 255), (int)(hc.z * 255), (int)(hc.w * 255) };
	return *((uint32_t*)&c);
}

void colorpick_tl::set_color2hsv(uint32_t c)
{
	color.y = color.x;
	color.x = c;
	hsv = RGBtoHSV((glm::u8vec4*)&c);
}

void colorpick_tl::set_hsv(const glm::vec3& c)
{
	hsv.x = c.x;
	hsv.y = c.y;
	hsv.z = c.z;
	color.y = color.x;
	color.x = get_color();
}
void colorpick_tl::set_hsv(const glm::vec4& c)
{
	hsv = c;
	color.y = color.x;
	color.x = get_color();
}
void colorpick_tl::set_posv(const glm::ivec2& poss)
{
	double htp = height + step;
	double cw0 = colorw - step, x = poss.x;
	if (x < 0) { x = 0; }
	double xf = (double)poss.x / cw0;
	if (xf > 1)xf = 1;
	if (xf < 0)xf = 0;
	int x4 = alpha ? 4 : 3;
	if (dx >= 0 && dx < x4)
	{
		hsv[dx] = xf;
	}
}
bool colorpick_tl::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	glm::ivec2 poss = mps - _pos;
	glm::ivec2 ss = _size;
	poss.x -= cpx;
	poss.y -= height + step;
	double htp = height + step;
	if (poss.y < 0)poss.y = 0;
	if (et == event_type2::on_down) {
		dx = poss.y / htp;
	}
	if (et == event_type2::on_click)
	{
		set_posv(poss);
	}
	if (et == event_type2::on_drag)
	{
		poss += curpos;
		set_posv(poss);
	}


	return false;
}

bool colorpick_tl::update(float delta)
{
	if (hsv != oldhsv || hsvstr.empty())
	{
		int* k = nullptr;
		oldhsv = hsv;
		int h = hsv.x * 360;
		int s = hsv.y * 100;
		int v = hsv.z * 100;
		int a = hsv.w * 100;
		hsvstr = "H:" + std::to_string(h) + (char*)u8"°";
		hsvstr += "\nS:" + std::to_string(s) + "%";
		hsvstr += "\nV:" + std::to_string(v) + "%";
		if (alpha)
		{
			hsvstr += "\nA:" + std::to_string(a) + "%";
		}
		else { hsv.w = 1; }
		glm::vec4 hc = {};
		HSVtoRGB(hsv, hc);
		char buf[256] = {};
		glm::ivec4 c = { (int)(hc.x * 255), (int)(hc.y * 255), (int)(hc.z * 255), (int)(hc.w * 255) };
		sprintf(buf, "#%02X%02X%02X%02X %d,%d,%d,%d", c.x, c.y, c.z, c.w, c.x, c.y, c.z, c.w);
		colorstr = buf;
		glm::ivec2 ss = _size;
		colorw = ss.x - cpx - step * 2;
#if 0
		if (ltx)
		{
			glm::ivec2 ss = size;
			glm::vec2 ta = { 0.0, 0.0 };
			glm::vec4 rc = { step, height + step, ss };
			cw = ss.x - cpx - step * 2;
			rc.w -= rc.y;
			tem_rtv.clear();
			//auto rk = ltx->get_text_rect(1, "H:00%%"/* hsvstr.c_str()*/, -1, font_size);
			ltx->heightline = rc.y;//设置固定行高
			rc.y += step;
			ltx->build_text(1, font_size, rc, ta, hsvstr.c_str(), -1, tem_rtv);
			rc.y = 0; rc.x += cpx;
			rc.z = cw;
			rc.w = height; ta.y = 0.5;
			ltx->heightline = 0; // 使用默认行高
			ltx->build_text(1, font_size, rc, ta, colorstr.c_str(), -1, tem_rtv);
			ltx->heightline = 0;
			assert(cw > 0);
		}
#endif
		if (on_change_cb) {
			on_change_cb(this, get_color());
		}
	}
	return false;
}





#if 1
void scroll_bar::set_viewsize(int64_t vs, int64_t cs, int rcw)
{
	_view_size = vs;
	_content_size = cs;
	if (rcw > 0)
		_rc_width = rcw;
	valid = true;
}

bool scroll_bar::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	glm::ivec2 poss = mps - _pos;
	glm::ivec2 ss = _size;
	poss -= tps;
	int px = _dir ? 0 : 1;
	int tsm = thumb_size_m.x + tps[_dir] * 2.0;
	auto pts = poss[_dir];
	pts -= _offset;
	switch (et)
	{
	case event_type2::on_click:
	{
		if (!d_drag)
		{
			hover = true;
			if (pts < 0) {
				t_offset = 0;
			}
			if (pts > thumb_size_m.x) {
				t_offset = thumb_size_m.x;
			}
			t_offset = thumb_size_m.x * 0.5;
			set_posv(poss);
			hover = false;
		}
	}
	break;
	case event_type2::on_move:
	{
		auto pts = poss[_dir];
		pts -= _offset;
		if (pts < 0 || pts > thumb_size_m.x)
		{
			_tcc = _color.y;
		}
		else {
			_tcc = _color.z;
		}
		scale_s = scale_s0.y;
	}
	break;
	case event_type2::mouse_up:
		hover = false;
		break;
	case event_type2::on_down:
	{
		d_drag = false;
		t_offset = pts;
		if (pts < 0 || pts > thumb_size_m.x)
		{
			hover = false;
		}
		else {
			hover = true;
		}
	}
	break;
	case event_type2::on_scroll:
	{
		if (thumb_size_m.z > 0 && ((_bst & (int)BTN_STATE::STATE_HOVER) || hover_sc && (parent && parent->_hover)))
		{
			auto st = ss[_dir] - tsm;
#if 0
			auto pts = (-mps.y * _pos_width) + _offset;
#else
			int64_t pw = (-mps.y * _pos_width);
			c_offset += pw;//内容偏移
			int64_t mxst = st * scale_w;
			c_offset = std::max((int64_t)0, std::min(c_offset, mxst));
			pts = c_offset / scale_w;
#endif
			if (limit)
			{
				if (pts < 0)pts = 0;
				if (pts > st)pts = st;
			}
			_offset = pts;// 滚动滑块偏移
			return true;
		}
	}
	break;
	case event_type2::on_drag:
	{
		poss += curpos;
		if (hover)
		{
			set_posv(poss);
			d_drag = true;
		}
	}
	break;
	default:
		break;
	}
	return false;
}

bool scroll_bar::update(float delta)
{
	// 箭头、滑块按钮初始大小一样
	bool r = valid;
	if (valid)
	{
		int vrc = _view_size - _rc_width;
		auto vs = _view_size;
		int px = _dir ? 0 : 1;
		glm::ivec2 ss = _size;
		auto pxs = (ss[px] - _rc_width) * 0.5;
		auto ss1 = ss[_dir];
		auto ss2 = _pos[_dir];
		glm::ivec3 sbs = calcu_scrollbar_size(vs, _content_size, pxs, 2);
		tps = { pxs,pxs };
		sbs.x = ss1 - (_content_size - vs);

		int tsm = tps[_dir] * 2.0;
		if (sbs.z > 0) {
			if (sbs.x < _rc_width)
			{
				sbs.x = _rc_width * 2;
			}
			// 滚动条宽度-滑块宽度-边框偏移
			auto vci = ss1 - sbs.x - tsm;
			scale_w = abs((double)(_content_size - (vs)) / vci);
			//assert(!(scale_w < 1));
			if (scale_w < 1)
			{
				scale_w = 1;
			}
		}
		else {
			_offset = 0; c_offset = scale_w * _offset;
		}
		thumb_size_m = sbs;
		valid = false;
	}
	if (!(_bst & (int)BTN_STATE::STATE_HOVER)) {
		if (!(_bst & (int)BTN_STATE::STATE_ACTIVE))
		{
			_tcc = _color.y; scale_s = scale_s0.x;
		}
	}
	if ((_bst & (int)BTN_STATE::STATE_ACTIVE) && _color.w) {
		_tcc = _color.w;
	}
	return r;
}



int64_t scroll_bar::get_offset()
{
	return scale_w * _offset;
}
int64_t scroll_bar::get_offset_ns()
{
	return c_offset;
}

int scroll_bar::get_range()
{
	glm::ivec2 ss = _size;
	int tsm = thumb_size_m.x + tps[_dir] * 2.0;
	auto st = ss[_dir] - tsm;
	return st;
}

void scroll_bar::set_offset(int pts)
{
	glm::ivec2 ss = _size;
	int tsm = thumb_size_m.x + tps[_dir] * 2.0;
	auto st = ss[_dir] - tsm;
	if (limit)
	{
		if (pts < 0)pts = 0;
		if (pts > st)pts = st;
	}
	_offset = pts;
	c_offset = scale_w * _offset;
}
void scroll_bar::set_offset_inc(int inc)
{
	set_offset(_offset + inc);
}

void scroll_bar::set_posv(const glm::ivec2& poss)
{
	if (!hover || thumb_size_m.z <= 0)return;
	glm::ivec2 ss = _size;
	int px = _dir ? 0 : 1;
	int tsm = thumb_size_m.x + tps[_dir] * 2.0;
	auto pts = poss[_dir];
	pts -= t_offset;
	int mx = ss[_dir] - tsm;
	if (limit)
	{
		if (pts < 0)pts = 0;
		if (pts > mx)pts = mx;
	}
	_offset = pts;
	c_offset = scale_w * _offset;
}
#endif

// todo draw ct
#if 1

void widget_t::draw(rvg_cx* rv)
{}

void image_btn::draw(rvg_cx* rv) {

	auto psv = get_pos(false);
	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view);
	//image_ptr_t* img = 0;
	//image_sliced_t state_img[5] = {};
	text_style st = {};
	st.fontsize = font_size;
	st.align = text_align;
	st.color = text_color;
	st.family = family;
	text_st tx = {};
	tx.pos = {};
	tx.size = get_size();
	tx.text = str.c_str(); tx.text_len = str.size();
	rv->add_text(&tx, &st);

	rv->pop_view();
}

void color_btn::draw(rvg_cx* rv)
{
	auto p = this;
	auto ns = p->_size;
	static int bid = 1234;
	if (id == bid)
	{
		id = id;
	}
#if 1

	auto psv = get_pos(false);
	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view);

	rv->save();
	rv->translate(p->_pos);
	if (p->dfill)
	{
		if (p->_circle)
		{
			auto sp = p->_pos;
			auto r = lround(p->_size.y * 0.5);
			sp += r;
			rv->add_circle(sp, r);
		}
		else
		{
			rv->add_rect({ 0.5,0.5, p->_size }, p->rounding);
		}
		rv->submit(p->dfill, 0, 0);
	}
	// 渲染标签
	glm::vec2 ps = { thickness * 2, thickness * 2 };
	if (p->mPushed) {
		ps += pushedps;
	}

	ns -= thickness * 4;

	glm::vec4 rc = { ps, ns };


	//draw_text(g, ltx, p->str.c_str(), -1, rc, &st);

	/*
	text_style_t st = {};
	st.font = 0;
	st.text_align = p->text_align;
	st.font_size = p->font_size;
	st.text_color = p->text_color;	ltx->tem_rtv.clear();
		ltx->build_text(0, rc, text_align, p->str.c_str(), -1, p->font_size, ltx->tem_rtv);
		ltx->update_text();
		ltx->draw_text(g, ltx->tem_rtv, p->text_color);*/
		//auto rc = draw_text_align(g, p->str.c_str(), ps, ns, text_align, p->text_color, p->family.c_str(), p->font_size);

	if (p->dcol)
	{
		if (p->_circle)
		{
			auto sp = p->_pos;
			auto r = lround(p->_size.y * 0.5);
			sp += r;
			rv->add_circle(sp, r);
		}
		else
		{
			rv->add_rect({ 0.5,0.5, p->_size }, p->rounding);
		}
		rv->submit(0, p->dcol, p->thickness);
	}
	//if ((bst & (int)BTN_STATE::STATE_HOVER))
	//{
	//	rv->add_rect( { 0,0,size.x,size.y }, p->rounding);
	//	rv->submit( 0, 0x80ff8000, 1, 0);
	//} 
	text_style st = {};
	st.fontsize = p->font_size;
	st.align = p->text_align;
	st.color = p->text_color;
	st.family = p->family;
	text_st tx = {};
	tx.pos = {};
	tx.size = get_size();
	tx.text = p->str.c_str(); tx.text_len = p->str.size();
	rv->add_text(&tx, &st);
	rv->restore();

	rv->pop_view();
#endif
}

void gradient_btn::draw(rvg_cx* rv)
{
#if 1
	auto p = this;
	float x = p->_pos.x, y = p->_pos.y, w = p->_size.x, h = p->_size.y;
	int pushed = p->mPushed ? 0 : 1;
	uint32_t gradTop = p->_gradTop;
	uint32_t gradBot = p->_gradBot;
	uint32_t borderDark = p->borderDark;
	uint32_t borderLight = p->borderLight;
	double oa = p->opacity;
	auto ns = p->_size;

	auto bc = effect == uTheme::light ? p->back_color : set_alpha_xf2(p->back_color, get_alpha_f(p->back_color));
	double rounding = p->rounding;
	glm::vec2 ns1 = { w * 0.5, h * 0.5 };
	auto nr = (int)std::min(ns1.x, ns1.y);
	if (rounding > nr)
	{
		rounding = nr;
	}
	glm::vec2 tps = { 0.5,0.5 };

	auto psv = get_pos(false);
	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view);
	rv->save();
	rv->translate({ x, y });
	if (is_alpha(bc))
	{
		bc = set_alpha_f(bc, oa);
		rv->add_rect({ thickness, thickness, w - thickness * 2, h - thickness * 2 }, rounding);
		rv->set_color(bc);
		rv->fill();
	}
	if (p->mPushed) {
		gradTop = set_alpha_f(gradTop, 0.8f);
		gradBot = set_alpha_f(gradBot, 0.8f);
	}
	else {
		double v = 1.0 - get_alpha_f(p->back_color);
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
	glm::vec2 rct = { w - thickness, h - thickness * 2.0 };
	glm::vec4 gtop = to_c4(gradTop);
	glm::vec4 gbot = to_c4(gradBot);

	rv->save();
	if (effect == uTheme::dark || p->mPushed)
		rv->translate({ thickness, thickness });
	//else
	//	rv->translate({ 0, 0 });
	rv->paint_shadow(0, rct.y, rct.x, rct.y, gtop, gbot, 0, rounding);// 垂直方向
	rv->restore();
	// 渲染标签

	glm::vec2 ps = { thickness * 2,thickness * 2 };
	if (p->mPushed) {
		ps += thickness;
	}
	ns -= thickness * 4;
	glm::vec4 rc = { ps, ns };
	// 边框
	w -= 1;
	h -= 1;
	rv->set_line_width(thickness);
	rv->set_color(borderLight);
	rv->add_rect({ tps.x,tps.y + (p->mPushed ? 0.f : 1.0f), w, h - (p->mPushed ? 0.0f : 1.0f) }, rounding);
	rv->stroke();
	rv->set_color(borderDark);
	rv->add_rect({ tps.x,tps.y, w , h }, rounding);
	rv->stroke();
	text_style st = {};
	st.fontsize = p->font_size;
	st.align = p->text_align;
	st.color = p->text_color;
	st.family = p->family;
	text_st tx = {};
	tx.pos = { 0,thickness * .5 };
	tx.size = get_size();
	tx.text = p->str.c_str(); tx.text_len = p->str.size();
	rv->add_text(&tx, &st);

	rv->restore();

	rv->pop_view();

#endif
}

void draw_radios(rvg_cx* rv, radio_info_t* p, radio_style_t* ps)
{
	if (ps->radius > 0) {
		rv->add_circle(p->pos, ps->radius);
		if (p->value || p->swidth > 0)
			rv->submit(ps->col, 0, ps->thickness);
		else
			rv->submit(ps->innc * 0, ps->line_col, ps->thickness);
	}
	if (p->swidth > 0) {
		rv->add_circle(p->pos, p->swidth);
		rv->submit(ps->innc, 0, ps->thickness);
	}
}


// check打勾
void drawCheckMark(rvg_cx* rv, glm::vec2 pos, uint32_t col, float sz1, bool mixed)
{
	if (!col)
		return;
	float sz = sz1;
	float thickness = std::max(sz / 5.0f, 1.0f);
	sz -= thickness * 0.5f;
	pos += glm::vec2(thickness * 0.25f, thickness * 0.25f);

	float third = sz / 3.0f;
	float bx = pos.x + third;
	float td = sz - third * 0.5f;
	float by = pos.y + td;
	if (mixed)
	{
		td = thickness * 0.5f;
		auto ps = glm::vec2(pos.x + td, by - third);
		auto ps1 = ps;
		ps1.x += sz1 - thickness;
		rv->add_line(ps, ps1);
	}
	else {
		glm::vec2 ps[3];
		ps[0] = glm::vec2(bx - third, by - third);
		ps[1] = glm::vec2(bx, by);
		ps[2] = glm::vec2(bx + third * 2.0f, by - third * 2.0f);
		rv->add_polyline(ps, 3);
	}
	rv->submit(0, col, thickness);
}
void draw_checkbox(rvg_cx* rv, check_style_t* p, checkbox_info_t* pn)
{
	if (!p)return;
	auto cc = p->check_col;
	glm::ivec2 ps = pn->pos;
	rv->add_rect({ ps.x,ps.y,p->square_sz,p->square_sz }, p->rounding);
	if (pn->value || pn->new_alpha > 0)
		rv->submit(p->fill, p->col, p->thickness);
	else
		rv->submit(0, p->line_col, p->thickness);
	const float pad = std::max(1.0f, floor(p->square_sz / 6.0f));
	if (pn->new_alpha > 0)
	{
		cc = set_alpha_f(cc, pn->new_alpha);
		drawCheckMark(rv, (glm::vec2)ps + glm::vec2(pad, pad), cc, p->square_sz - pad * 2.0f, pn->mixed);
	}
}

void radio_tl::draw(rvg_cx* rv)
{
	auto p = this;
	if (rv && p) {
#if 1

		auto psv = get_pos(false);
		auto view = glm::ivec4(psv, get_size());
		rv->push_view(view);

		rv->save();
		glm::ivec2 poss = p->_pos;
		rv->translate((glm::vec2)poss);
		//rv->add_rect({ 0,0,get_size() }, 0);
		//rv->set_color(0);
		//rv->fill();
		rv->translate(glm::vec2(_size.x * 0.5 - style.radius, _size.y * 0.5 - style.radius));//+ glm::vec2(0.5f, 0.5f)
		int x = 0;
		{
			auto& it = v;
			it.pos = {};
			it.pos.x = x * style.radius * 0.5;
			it.pos += style.radius;
			draw_radios(rv, &it, &style);
			x++;
		}
		rv->restore();

		rv->pop_view();
#endif
	}
}

void checkbox_tl::draw(rvg_cx* rv)
{
	auto p = this;
	if (rv && p) {
#if 1

		auto psv = get_pos(false);
		auto view = glm::ivec4(psv, get_size());
		rv->push_view(view);

		rv->save();
		glm::ivec2 poss = p->_pos;
		poss.x += (_size.x - style.square_sz) * 0.5;
		poss.y += (_size.y - style.square_sz) * 0.5;
		rv->translate((glm::vec2)poss + glm::vec2(0.5f, 0.5f));
		int x = 0;
		{
			auto& it = v;
			it.pos.x = x * p->style.square_sz * 2.5;
			draw_checkbox(rv, &p->style, &it);
			x++;
		}
		rv->restore();

		rv->pop_view();
#endif
	}
}
void switch_tl::draw(rvg_cx* rv)
{
#if 1
	auto psv = get_pos(false);
	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view);

	glm::ivec2 poss = _pos;
	auto h = height;
	auto fc = h * cv * 0.5;
	auto ss = h * wf;
	if (_size.x <= 0) {
		_size.x = ss;
	}
	if (_size.y <= 0) {
		_size.y = h;
	}
	poss.x += (_size.x - ss) * 0.5;
	poss.y += (_size.y - height) * 0.5;
	rv->translate((glm::vec2)poss);
	rv->add_rect({ 0.5,0.5, ss, h }, h * 0.5);
	rv->submit(dcol, 0, 0);
	glm::vec2 cp = {};
	{
		auto ps = h * 0.5;
		cp.x += (ss - h) * cpos + h * 0.5;
		cp.y += ps;
		rv->add_circle(cp, fc);
		rv->submit(color.z, 0, 0);
	}
	rv->stroke();

	rv->pop_view();
#endif
}
void progress_tl::draw(rvg_cx* rv)
{

	glm::ivec2 poss = _pos;
	glm::ivec2 ss = _size;
	ss.x = width;
	ss.y = height;
#if 1
	auto psv = get_pos(false);
	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view);

	glm::vec2 npos = _size - (glm::vec2)ss;
	npos *= 0.5;
	rv->save();
	rv->translate(poss);
	//rv->add_rect({ 0,0, _size.x, _size.y }, 0);
	//rv->submit(0xff000000, 0, 0);
	rv->translate(npos);
	rv->add_rect({ 0.5,0.5, ss.x, ss.y }, rounding);
	rv->submit(color.y, 0, 0);
	double xx = ss.x * value;
	int kx = 0;
	int r = rounding;
	if (xx > 0)
	{
		rv->save();
		if (xx < rounding * 2)
		{
			rv->add_rect({ 0,0, xx, ss.y }, r);
			rv->clip();
			xx = r * 2;
			kx = 1;
		}
		rv->add_rect({ 0.5,0.5, xx, ss.y }, r);
		rv->submit(color.x, 0, 0);
		rv->restore();
	}
	if (text.size()) {
		glm::ivec2 rk = {};// ltx->get_text_rect(0, font_size, text.c_str(), -1);
		if (text_inside) {
			ss.x = xx;
			ss.x -= r + thickness;
		}
		else {
			ss.x = _size.x;
		}
		if (kx) {
			ss.x += rk.x;
		}
		if (right_inside) {
			ss.x = _size.x - r - thickness;
		}
		glm::vec2 ta = { 1,0.5 };
		glm::vec4 rc = { 0, 0, _size };
		//text_style_t st = {};
		//st.font = 0;
		//st.text_align = ta;
		//st.font_size = font_size;
		//st.text_color = text_color;
		//draw_text(cr, ltx, text.c_str(), -1, rc, &st);

		text_style st = {};
		st.fontsize = font_size;
		st.align = ta;// text_align;
		st.color = text_color;
		st.family = family;
		text_st tx = {};
		tx.pos = { 0,thickness };
		tx.size = ss;
		tx.text = text.c_str(); tx.text_len = text.size();
		rv->add_text(&tx, &st);
	}
	rv->restore();

	rv->pop_view();
#endif
}

void slider_tl::draw(rvg_cx* rv)
{
#if 1
	auto psv = get_pos(false);
	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view);

	rv->save();
	glm::ivec2 poss = _pos;
	glm::ivec2 ss = _size;
	rv->translate(poss);
	glm::vec4 brc = {}, cliprc, crc;
	int x = 0, y = 0;
	double xx = (ss[vertical]) * value;
	glm::vec2 spos = {};
	int kx = 0;
	int r = rounding;
	auto x1 = xx;
	if (xx < rounding * 2) {
		x1 = r * 2;
		kx = 1;
	}
	if (vertical)
	{
		x = (ss.x - wide) * 0.5;
		brc = { 0.5 + x ,0.5 + y , wide, ss.y };
		cliprc = { 0,0, wide, xx };
		xx = glm::clamp((float)xx, (float)0, (float)ss.y);
		spos = { ss.x * 0.5,xx };
		crc = { 0.5 + x,0.5 + y, wide, x1 };
		if (reverse_color) {
			x1 -= rounding;
			crc = { 0.5 + x,0.5 + y + x1 , wide, ss.y - x1 };
		}
	}
	else {
		y = (ss.y - wide) * 0.5;
		brc = { 0.5 + x ,0.5 + y, ss.x , wide };
		cliprc = { 0,0, xx, wide };
		xx = glm::clamp((float)xx, (float)0, (float)ss.x);
		spos = { xx ,ss.y * 0.5 };
		crc = { 0.5 + x,0.5 + y, x1, wide };
		if (reverse_color) {
			x1 -= rounding;
			crc = { 0.5 + x + x1 ,0.5 + y,ss.x - x1, wide };
		}
	}
	rv->add_rect(brc, rounding);
	rv->submit(color.y, 0, 0);
	if (xx >= 0)
	{
		{
			rv->add_rect(crc, r);
			rv->submit(color.x, 0, 0);
		}
		if (sl.x > 0) {
			rv->add_circle(spos, sl.x);
			rv->submit(sl.y, color.x, thickness);
		}
	}
	rv->restore();

	rv->pop_view();
#endif
}


void colorpick_tl::draw(rvg_cx* rv)
{
#if 1
	auto psv = get_pos(false);
	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view);

	glm::ivec2 poss = _pos;
	glm::ivec2 ss = _size;
	rv->translate(poss);
	rv->save();
	float style_alpha8 = 1;
	//uint32_t col_hues[] = { 0xff0000ff,0xff00ffff,0xff00ff00,0xffffff00,0xffff0000,0xffff00ff,0xff0000ff };
	const glm::vec4 col_hues[6 + 1] = { glm::vec4(1,0,0,style_alpha8), glm::vec4(1,1,0,style_alpha8), glm::vec4(0,1,0,style_alpha8)
		, glm::vec4(0,1,1,style_alpha8), glm::vec4(0,0,1,style_alpha8), glm::vec4(1,0,1,style_alpha8), glm::vec4(1,0,0,style_alpha8) };
	int yh = height + step;
	glm::vec4 hc = {};
	glm::vec2 tps = { cpx + step,yh };
	HSVtoRGB(hsv, hc);
	{
		glm::vec4 cc = {};
		rv->grid_fill({ cpx, height }, { -1,0xffdfdfdf }, height * 0.5);// 填充格子
		rv->add_rect({ 0,0,cpx, height }, 0);
		rv->set_color(hc);
		rv->fill();// 填充当前颜色
	}
	{
		rv->translate(tps);
		rv->linear_fill({ colorw,height }, col_hues, 7);	// H 
		glm::ivec4 rcc = { (colorw - step) * hsv.x,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		rv->add_rect(rcf, 0);
		rv->submit(-1, bc_color, thickness);
	}
	tps.x = 0;
	{
		rv->translate(tps);
		glm::vec4 cc[] = { {1,1,1,1}, hc };
		rv->grid_fill({ colorw, height }, { -1,-1 }, height * 0.5);// 背景色
		rv->linear_fill({ colorw, height }, cc, 2);	// S
		glm::ivec4 rcc = { (colorw - step) * hsv.y,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		rv->add_rect(rcf, 0);
		rv->submit(-1, bc_color, thickness);
	}
	{
		rv->translate(tps);
		glm::vec4 cc[] = { {0,0,0,1}, hc };
		rv->grid_fill({ colorw, height }, { -1,-1 }, height * 0.5);// 背景色
		rv->linear_fill({ colorw, height }, cc, 2);	// V
		glm::ivec4 rcc = { (colorw - step) * hsv.z,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		rv->add_rect(rcf, 0);
		rv->submit(-1, bc_color, thickness);
	}
	{
		rv->translate(tps);
		glm::vec4 cc[] = { {0,0,0,0}, {1,1,1,1} };
		rv->grid_fill({ colorw, height }, { -1,0xffdfdfdf }, height * 0.5);//背景色
		rv->linear_fill({ colorw, height }, cc, 2);	// A
		glm::ivec4 rcc = { (colorw - step) * hsv.w,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		rv->add_rect(rcf, 0);
		rv->submit(-1, bc_color, thickness);
	}
	rv->restore();
	{
		glm::ivec2 ss = _size;
		glm::vec2 ta = { 0.0, 0.0 };
		glm::vec4 rc = { step, height + step, ss };
		rc.w -= rc.y;
		text_style st = {};
		st.fontsize = font_size;
		st.align = ta;// text_align;
		st.color = text_color;
		st.family = family;
		st.lineheight = rc.y;//设置固定行高
		rc.y += step;
		text_st tx = {};
		tx.pos = { 0,thickness };
		tx.pos = { rc.x,rc.y };
		tx.size = { rc.z,rc.w };
		tx.text = hsvstr.c_str(); tx.text_len = hsvstr.size();
		rv->add_text(&tx, &st);
		rc.y = 0; rc.x += cpx;
		rc.z = colorw;
		rc.w = height; ta.y = 0.5;
		st.lineheight = 0; // 使用默认行高
		tx.pos = { rc.x,rc.y };
		tx.size = { rc.z,rc.w };
		tx.text = colorstr.c_str(); tx.text_len = colorstr.size();
		rv->add_text(&tx, &st);
	}

	rv->pop_view();
#endif
}
void scroll_bar::draw(rvg_cx* rv)
{
	glm::ivec2 poss = _pos;
	glm::ivec2 ss = _size;

	auto psv = get_pos(false);
	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view);

	// 背景
	if (!hideble || thumb_size_m.z) {
		rv->add_rect({ poss,ss }, rounding);
		rv->submit(_color.x, 0, 0);
	}
	// 滑块
	double rw = _rc_width * scale_s;
	glm::ivec4 trc = { poss.x,poss.y,rw,rw };
	int px = _dir ? 0 : 1;
	auto pxs = ceil((ss[px] - rw) * 0.5);
	trc.x += pxs;
	trc.y += pxs;
	trc[_dir] = tps[_dir] + _offset;
	trc[2 + _dir] = thumb_size_m.x;
	if (thumb_size_m.z)
	{
		rv->add_rect(trc, _rc_width * 0.5 * scale_s);
		rv->submit(_tcc, 0, 0);
	}
	rv->pop_view();
}


#endif // 1


#if 1
widget_t::widget_t()
{}
widget_t::widget_t(WIDGET_TYPE wt) :wtype(wt)
{}
widget_t::~widget_t()
{}
void widget_t::set_pos(const glm::ivec2& ps)
{
	_pos = ps; valid = true;
}
void widget_t::set_size(const glm::vec2& ss)
{
	_size = ss; valid = true;
}
glm::vec2 widget_t::get_size()
{
	return _size;
}
bool widget_t::on_mevent(int type, const glm::vec2& mps, void* e)
{
	return false;
}

bool widget_t::update(float delta)
{
	return false;
}

glm::ivec2 widget_t::get_pos(bool has_parent)
{
	glm::ivec2 ps = _pos;
	if (parent) {
		auto pss = parent->get_pos();
		auto ss = parent->get_spos();
		ss *= hscroll;
		ps += ss;
		if (has_parent) { ps += pss; }
	}
	return ps;
}

glm::ivec2 widget_t::get_spos()
{
	glm::ivec2 ps = {};
	if (parent) {
		auto ss = parent->get_spos();
		ss *= hscroll;
		ps += ss;
	}
	return ps;
}

void widget_t::set_family(font_family_t* family, int fontsize)
{
	this->family = family; this->font_size = fontsize;
}

void widget_t::set_editing(const std::string& str, const glm::ivec2& cpos, int lineheight)
{}


input_state_t* get_input_state_cx(void* ptr, int t);
// 通用控件鼠标事件处理 type有on_move/on_scroll/on_drag/on_down/on_up/on_click/on_dblclick/on_tripleclick
bool widget_on_move(widget_t* wp, uint32_t type, et_un_t* ep, const glm::vec2& pos) {
	bool hover = false;
	if (!wp)return hover;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	if (t == devent_type_e::mouse_move_e)
	{
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y }; mps -= pos;
		wp->mmpos = mps;
		// 判断是否鼠标进入 
		glm::vec4 trc = { wp->_pos  ,wp->_size };
		auto k = check_box_cr1(mps, &trc, 1, sizeof(glm::vec4));
		if (k.x) {
			bool hoverold = wp->_bst & (int)BTN_STATE::STATE_HOVER;
			wp->_bst |= (int)BTN_STATE::STATE_HOVER;   hover = true;
			if (!(wp->_bst & (int)BTN_STATE::STATE_ACTIVE))// 不是鼠标则独占
				ep->ret = 1;
			if (!hoverold)
			{
				// 鼠标进入
				wp->on_mevent((int)event_type2::on_enter, mps, p);
				if (wp->mevent_cb) {
					wp->mevent_cb(wp, (int)event_type2::on_enter, mps);
				}
			}
		}
		else {
			if (wp->_bst & (int)BTN_STATE::STATE_HOVER)
			{
				wp->_bst &= ~(int)BTN_STATE::STATE_HOVER;
				// 鼠标离开
				wp->on_mevent((int)event_type2::on_leave, mps, p);
				if (wp->mevent_cb) {
					wp->mevent_cb(wp, (int)event_type2::on_leave, mps);
				}
			}
		}

		{
			if (wp->_bst & (int)BTN_STATE::STATE_HOVER)
			{
				wp->on_mevent((int)event_type2::on_move, mps, p);
				if (wp->mevent_cb)
				{
					wp->mevent_cb(wp, (int)event_type2::on_move, mps);
				}
			}
			if (wp->_bst & (int)BTN_STATE::STATE_ACTIVE) {
				auto dps = mps - wp->curpos;
				wp->on_mevent((int)event_type2::on_drag, dps, p);		// 拖动事件
				if (wp->mevent_cb)
				{
					wp->mevent_cb(wp, (int)event_type2::on_drag, dps);		// 拖动事件
				}
			}
		}
	}
	return hover;
}

void widget_on_event(widget_t* wp, uint32_t type, et_un_t* ep, const glm::vec2& pos) {
	if (!wp)return;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	wp->form = ep->form;
	switch (t)
	{
	case devent_type_e::mouse_move_e:
		widget_on_move(wp, type, ep, pos);
		break;
	case devent_type_e::mouse_button_e:
	{
		auto p = e->b;
		glm::ivec2 mps = { p->x,p->y }; mps -= pos;
		bool isd = wp->cmpos == mps;
		wp->cmpos = mps;
		if (wp->_bst & (int)BTN_STATE::STATE_HOVER) {
			if (p->down == 1)
			{
				ep->ret = 1;
			}
			if (p->button == 1) {
				if (p->down == 1) {
					wp->_bst |= (int)BTN_STATE::STATE_ACTIVE;
					wp->curpos = mps - (glm::ivec2)wp->_pos;
					wp->cks = 0;
					wp->on_mevent((int)event_type2::on_down, mps, p);
					if (wp->mevent_cb) { wp->mevent_cb(wp, (int)event_type2::on_down, mps); }
				}
				else {
					if ((wp->_bst & (int)BTN_STATE::STATE_ACTIVE) && (isd || !wp->has_drag))
					{
						wp->cks = p->clicks;
						wp->on_mevent((int)event_type2::on_up, mps, p);
						if (wp->mevent_cb) {
							wp->mevent_cb(wp, (int)event_type2::on_up, mps);
						}
						int tc = (int)event_type2::on_click; //左键单击
						if (p->clicks == 2) { tc = (int)event_type2::on_dblclick; }
						else if (p->clicks == 3) { tc = (int)event_type2::on_tripleclick; }
						wp->_clicks = p->clicks;
						wp->on_mevent(tc, mps, p);
						if (wp->mevent_cb) {
							wp->mevent_cb(wp, tc, mps);
						}
						if (wp->click_cb)
						{
							wp->click_cb(wp, p->clicks);
						}
					}
					wp->_bst &= ~(int)BTN_STATE::STATE_ACTIVE;
				}
			}
		}
		if (p->down == 0) {
			wp->_bst &= ~(int)BTN_STATE::STATE_ACTIVE;
			wp->_bst |= (int)BTN_STATE::STATE_NOMAL;
			wp->on_mevent((int)event_type2::mouse_up, mps, p);
			if (wp->mevent_cb) {
				wp->mevent_cb(wp, (int)event_type2::mouse_up, mps);
			}
		}
	}
	break;
	case devent_type_e::mouse_wheel_e:
	{
		auto p = e->w;
		glm::vec2 mps = { p->x, p->y };
		if (wp->_bst & (int)BTN_STATE::STATE_HOVER || wp->has_hover_sc)
		{
			ep->ret = wp->on_mevent((int)event_type2::on_scroll, mps, p);
			if (wp->mevent_cb) {
				wp->mevent_cb(wp, (int)event_type2::on_scroll, mps);
			}
			ep->ret = 1;
		}
	}
	break;
	case devent_type_e::keyboard_e:
	{
		// todo
		//on_keyboard(ep);
	}
	break;
	default:
		break;
	}

}
void send_hover(widget_t* wp, const glm::vec2& mps) {

	wp->on_mevent((int)event_type2::on_hover, mps, nullptr);
	if (wp->mevent_cb)
	{
		wp->mevent_cb(wp, (int)event_type2::on_hover, mps);
	}
}

bool on_wpe(widget_t* pw, int type, et_un_t* ep, const glm::ivec2& ppos)
{
	bool r = false;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	widget_on_event(pw, type, ep, ppos);
	if (pw->on_event_cb)
	{
		pw->on_event_cb(type, ep, ppos);
	}
	else
	{
		if (ep->ret && t == devent_type_e::mouse_button_e)
		{
			auto p = e->b;
			if (p->down == 1)
			{
				get_input_state(0, 1);
				get_input_state_cx(0, 1);
			}
		}
	}
	return (ep->ret);
}

div_cx::div_cx()
{
	hscroll = {};
}

div_cx::~div_cx()
{}
void div_cx::add_widget(widget_t* p)
{
	if (p)
	{
		if (!p->family)
			p->set_family(family, font_size);
		p->parent = this;
		widgets.push_back(p); uplayout = true;
	}
}
void div_cx::remove_widget(widget_t* p)
{
	auto& v = widgets;
	auto ps = v.size();
	v.erase(std::remove(v.begin(), v.end(), p), v.end());
	if (v.size() != ps)
	{
		valid = true; uplayout = true;
	}
}
void div_cx::set_scroll(int width, int rcw, const glm::ivec2& pos_width, const glm::ivec2& vnpos, const glm::ivec2& hnpos)
{
	auto pss = _size;
	{
		auto cp = new_scroll_bar({ width,pss.y - width * 2 }, pss.y, pss.y, rcw, true, vnpos);
		bind_scroll_bar(cp, true); // 绑定垂直滚动条
		cp->_pos_width = pos_width.y > 0 ? pos_width.y : width * 2;//滚轮事件每次滚动量
		cp->hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->has_hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->hscroll = {};
		cp->_absolute = true;
		cp->rounding = std::max(2, (int)(width * 0.5));
	}
	{
		auto cp = new_scroll_bar({ pss.x - width * 2,width }, pss.x, pss.x, rcw, false, hnpos);
		bind_scroll_bar(cp, false); // 绑定水平滚动条
		cp->_pos_width = pos_width.x > 0 ? pos_width.x : width;
		cp->hscroll = {};
		cp->_absolute = true;
		cp->rounding = std::max(2, (int)(width * 0.5));
	}
}
void div_cx::set_scroll_hide(bool is)
{
	if (horizontal)
		horizontal->hideble = is;
	if (vertical)
		vertical->hideble = is;
}
void div_cx::set_scroll_pos(const glm::ivec2& ps, bool v)
{
	if (v)
	{
		if (vertical)
			vertical->_pos = ps;
	}
	else
	{
		if (horizontal)
			horizontal->_pos = ps;
	}
}
void div_cx::set_scroll_size(const glm::ivec2& ps, bool v)
{
	if (v)
	{
		if (vertical)
			vertical->_size = ps;
	}
	else
	{
		if (horizontal)
			horizontal->_size = ps;
	}
}
void div_cx::set_view(const glm::ivec2& view_size, const glm::ivec2& content_size)
{
	if (horizontal)
		horizontal->set_viewsize(view_size.x, content_size.x, 0);
	if (vertical)
		vertical->set_viewsize(view_size.y, content_size.y, 0);
}
void div_cx::set_scroll_visible(const glm::ivec2& hv)
{
	if (horizontal)//水平滚动条
	{
		horizontal->visible = hv.x;
	}
	//垂直滚动条
	if (vertical)
	{
		vertical->visible = hv.y;
	}
}
glm::ivec2 div_cx::get_scroll_range()
{
	glm::ivec2 r = {};
	if (horizontal)//水平滚动条
	{
		r.x = horizontal->get_range();
	}
	//垂直滚动条
	if (vertical)
	{
		r.y = vertical->get_range();
	}
	return r;
}
void div_cx::set_scroll_pts(const glm::ivec2& pts, int t)
{
	//水平滚动条
	if (horizontal)
	{
		if (t == 0)
			horizontal->set_offset(pts.x);
		else
			horizontal->set_offset_inc(pts.x);

	}
	//垂直滚动条
	if (vertical)
	{
		if (t == 0)
			vertical->set_offset(pts.y);
		else
			vertical->set_offset_inc(pts.y);
	}
}
void div_cx::sortdg()
{
	std::stable_sort(dragsp.begin(), dragsp.end(), [](const drag_v6* t1, const drag_v6* t2) { return t1->z < t2->z; });
}
void div_cx::bind_scroll_bar(scroll_bar* p, bool v)
{
	if (p)
	{
		if (v)
		{
			if (vertical)
				delete vertical;
			vertical = p;
		}
		else
		{
			if (horizontal)
				delete horizontal;
			horizontal = p;
		}
		remove_widget(p);
	}
}
void div_cx::on_motion(const glm::vec2& pos)
{
	glm::ivec2 ps = pos;
	if (ckinc > 0)
	{
		if (draggable)
			set_pos(ps - curpos);
		//else
		//	set_scroll_pts(ps - curpos, 1);

		div_ev e = {};
		e.p = this; e.down = 1; e.clicks = 0; e.mpos = ps;
		e.drag = true;
		if (on_click)
		{
			on_click(&e);	// 执行拖动事件
		}
		//for (auto& c : on_mouses)
		//{
		//	c(&e);
		//}
	}

	//tv->on_motion(ps - tpos);
	//update(0);
}
void div_cx::on_button(int idx, int down, const glm::vec2& pos, int clicks, int r)
{
	glm::ivec2 ps = pos - (glm::vec2)get_pos();
	if (idx == 1)
	{
		glm::vec4 trc = glm::vec4(0, 0, get_size());
		auto k2 = check_box_cr1(ps, &trc, 1, sizeof(glm::vec4));

		if (k2.x)
		{
			if (draggable && down == 1)
			{
				//todo form_move2end(form, this); // 移动窗口前面
			}
			if (!r)
			{
				if (down == 1 && ckinc == 0)
				{
					curpos = ps - tpos;
					ckinc++;
				}
				div_ev e = {};
				e.p = this; e.down = down; e.clicks = clicks; e.mpos = ps;
				if (on_click)
				{
					on_click(&e);	// 执行单击事件
				}
				//for (auto& c : on_mouses)
				//{
				//	c(&e);
				//}
			}
			ckup = 1;
		}
		else {
			ckup = 0;
			_hover = false;
		}
		if (down == 0)
		{
			if (ckinc)
				ckup = 1;
			ckinc = 0;
		}
	}
	//tv->on_button(idx, down, ps - tpos, clicks);
	//update(0);
	//_draw_valid = true;
}
void div_cx::on_wheel(double x, double y)
{
	//update(0);
}
scroll_bar* div_cx::new_scroll_bar(const glm::ivec2& size, int vs, int cs, int rcw, bool v, const glm::ivec2& npos)
{
	auto p = new scroll_bar();
	if (p)
	{
		add_widget(p);
		p->_absolute = true;
		p->_size = size;
		auto ss = get_size();
		glm::ivec2 dw = {};
		if (!v)
		{
			if (p->_size.x < 0)
				p->_size.x = ss.x - border.z * 2;
			p->_pos.y = ss.y - border.y;
			p->_pos.x = border.z;
			dw.y = 1;
			p->_dir = 0;
		}
		if (v)
		{
			if (p->_size.y < 0)
				p->_size.y = ss.y - border.z * 2;
			p->_pos.x = ss.x - (border.y);
			p->_pos.y = border.z;
			dw.x = 1;
			p->_dir = 1;
		}
		if (p->_size.x < rcw) {
			p->_size.x = rcw;
		}
		if (p->_size.y < rcw) {
			p->_size.y = rcw;
		}
		dw *= p->_size;
		p->_pos -= dw;
		p->_pos -= npos;
		p->set_viewsize(vs, cs, rcw);
	}
	return p;
}
scroll2_t div_cx::new_scroll2(const glm::ivec2& viewsize, int width, int rcw, const glm::ivec2& pos_width, const glm::ivec2& vnpos, const glm::ivec2& hnpos)
{
	scroll2_t r = {};
	auto pss = viewsize;
	{
		auto cp = new_scroll_bar({ width,pss.y - width * 2 }, pss.y, pss.y, rcw, true, vnpos);
		cp->_pos_width = pos_width.y > 0 ? pos_width.y : width * 2;//滚轮事件每次滚动量
		cp->hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->has_hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->hscroll = {};
		cp->rounding = std::max(2, (int)(width * 0.5));
		r.v = cp;
	}
	{
		auto cp = new_scroll_bar({ pss.x - width * 2,width }, pss.x, pss.x, rcw, false, hnpos);
		cp->_pos_width = pos_width.x > 0 ? pos_width.x : width;
		cp->hscroll = {};
		cp->rounding = std::max(2, (int)(width * 0.5));
		r.h = cp;
	}
	return r;
}
void div_cx::on_event(uint32_t type, et_un_t* ep)
{
	auto e = &ep->v;
	if (!visible)return;
	auto t = (devent_type_e)type;
	//glm::ivec2 vgpos = viewport;
	int r1 = 0;
	auto ppos = get_pos();
	auto sps = get_spos();
	_hover_eq.w = type;
	widget_t* hpw = 0;

	if (t == devent_type_e::mouse_move_e)
	{
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y };
		glm::vec4 trc = glm::vec4(0, 0, get_size());
		auto k2 = check_box_cr1(mps, &trc, 1, sizeof(glm::vec4));
		if (k2.x) {
			_bst |= (int)BTN_STATE::STATE_HOVER;
			r1 = 1;
			p->cursor = (int)cursor_st::cursor_arrow;
			_hover = true;
			if (_move_pos != mps)
			{
				_move_pos = mps;
				_hover_eq.x = 0;
			}
			//printf("_hover\n");
		}
		else {
			_move_pos = mps;
			_hover_eq.z = 0;
			_bst &= ~(int)BTN_STATE::STATE_HOVER;
			if (ckinc == 0)
				_hover = false;
			//printf("on_leave\n");
		}
		if (horizontal)
		{
			widget_on_event(horizontal, type, ep, ppos);// 水平滚动条
		}
		if (vertical) {
			widget_on_event(vertical, type, ep, ppos);// 垂直滚动条 
		}
		for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
			auto pw = *it;
			if (!pw || !pw->visible || pw->_disabled_events)continue;
			auto vpos = sps * pw->hscroll;
			on_wpe(pw, type, ep, ppos + vpos);
		}
		on_wpe(this, type, ep, {});

		event_wts.clear();
		event_wts1.clear();
		if (horizontal) {
			horizontal->_bst& (int)BTN_STATE::STATE_HOVER ? event_wts.push_back(horizontal) : event_wts1.push_back(horizontal);//水平滚动条
		}
		if (vertical) {
			vertical->_bst& (int)BTN_STATE::STATE_HOVER ? event_wts.push_back(vertical) : event_wts1.push_back(vertical);//垂直滚动条
		}
		for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
			if ((*it)->_bst & (int)BTN_STATE::STATE_HOVER)
				event_wts.push_back(*it);
			else
				event_wts1.push_back(*it);
		}
		if (this) {
			//this->_bst& (int)BTN_STATE::STATE_HOVER ? event_wts.push_back(this) : event_wts1.push_back(this);//本容器
		}
		auto length = event_wts.size();
		{
			// 生成鼠标离开消息
			for (size_t i = 1; i < length; i++) {
				auto pw = event_wts[i];
				if (!pw || !pw->visible || pw->_disabled_events)continue;
				auto vpos = sps * pw->hscroll;
				auto p = e->m;
				glm::ivec2 mps = { p->x,p->y };
				mps -= ppos + vpos;
				if (pw == this)mps -= ppos;
				bool isd = pw->cmpos == mps;
				pw->cmpos = mps;
				pw->_bst &= ~(int)BTN_STATE::STATE_HOVER;
				// 鼠标离开
				pw->on_mevent((int)event_type2::on_leave, mps, p);
				if (pw->mevent_cb) {
					pw->mevent_cb(pw, (int)event_type2::on_leave, mps);
				}
			}
		}

	}
	else
	{
		bool btn = !(t == devent_type_e::mouse_button_e && e->b->down == 0);// 弹起判断
		int icc = 0;
		auto length = event_wts.size();
		for (size_t i = 0; i < length; i++)
		{
			auto pw = event_wts[i];
			icc++;
			if (!pw || !pw->visible || pw->_disabled_events)continue;
			auto vpos = sps * pw->hscroll + ppos;
			if (pw == this)
				vpos = {};
			on_wpe(pw, type, ep, vpos);
			if (ep->ret && btn && pw != this) {
				hpw = pw;
				break;
			}
		}
		if (!hpw)
		{
			auto ln = event_wts1.size();
			for (size_t i = 0; i < ln; i++)
			{
				auto pw = event_wts1[i];
				if (!pw || !pw->visible || pw->_disabled_events)continue;
				auto vpos = sps * pw->hscroll + ppos;
				if (pw == this)
					vpos = {};
				on_wpe(pw, type, ep, vpos);
				if (ep->ret && btn) {
					hpw = pw; break;
				}
			}
		}
	}
	if (!ep->ret)
		ep->ret = r1;

	switch (t)
	{
	case devent_type_e::mouse_move_e:
	{
		auto length = event_wts.size();
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y };
		on_motion(mps);
		_hover_eq.z = (length > 0) ? 1 : 0;// 悬停准备
		if (ckinc > 0)
		{
			for (auto& it : drags)
			{
				if (it.ck > 0)
				{
					it.pos = mps - it.tp - ppos;	// 处理拖动坐标
					it.cp1 = mps - ppos;
				}
			}
		}
		else {
			for (auto& it : drags)
			{
				it.ck = 0;
			}
		}
	}
	break;
	case devent_type_e::mouse_button_e:
	{
		auto p = e->b;
		glm::ivec2 mps = { p->x,p->y };
		on_button(p->button, p->down, mps, p->clicks, ep->ret);

		if (p->button == 1) {
			if (p->down == 1) {
				mps -= ppos;
				drag_v6* dp = 0;

				for (auto vt = dragsp.rbegin(); vt != dragsp.rend(); vt++)
				{
					auto dp1 = *vt;
					auto& it = *dp1;
					it.z = 0;
					if (it.size.x > 0 && it.size.y > 0)
					{
						if (dp)continue;
						glm::vec4 trc = { it.pos + sps,it.size };
						auto k2 = check_box_cr1(mps, &trc, 1, sizeof(glm::vec4));
						if (k2.x)
						{
							it.tp = mps - it.pos;	// 记录当前拖动坐标
							it.ck = 1;
							it.cp1 = it.cp0 = mps;
							it.z = 1;
							dp = dp1;
						}
					}
				}
				if (!dp)
				{
					for (auto vt = dragsp.rbegin(); vt != dragsp.rend(); vt++)
					{
						auto dp1 = *vt;
						auto& it = *dp1;
						if (it.size.x == 0 || it.size.y == 0)
						{
							it.z = 0;
							it.cp1 = it.cp0 = mps;
							it.tp = mps - it.pos;	// 记录当前拖动坐标
							it.ck = 1;
						}
					}
				}
				else
				{
					for (auto vt = dragsp.rbegin(); vt != dragsp.rend(); vt++)
					{
						auto dp1 = *vt;
						auto& it = *dp1;
						if (it.size.x == 0 && it.size.y == 0)
						{
							it.z = 0;
							it.cp1 = it.cp0 = mps;
						}
					}
					sortdg();
				}
			}
		}

		_hover_eq.z = 0;
		//printf("ck:%d\t%p\n", ckinc, this);
		if (ckup > 0)
			ep->ret = 1;
	}
	break;
	case devent_type_e::mouse_wheel_e:
	{
		auto p = e->w;
		on_wheel(p->x, p->y);
		if (_hover)
		{
			ep->ret = 1;		// 滚轮独占本事件
		}
	}
	break;
	case devent_type_e::keyboard_e:
	{
		//on_keyboard(ep);
	}
	break;
	}
	evupdate++;
}
bool vht(widget_t** widgets, size_t count, const glm::ivec2& p, glm::ivec2 ips, const glm::ivec2& scroll_pos) {
	bool r = false;
	for (size_t i = count - 1; i > 0; i--) {
		auto pw = widgets[i];
		if (!pw || !pw->visible || pw->_disabled_events)continue;
		glm::vec2 mps = p; mps -= ips;
		mps -= pw->hscroll * scroll_pos;
		// 判断是否鼠标在控件上
		glm::vec4 ppos = { pw->_pos,pw->_size };
		auto k = check_box_cr1(mps, &ppos, 1, sizeof(glm::vec4));
		if (k.x) { r = true; }
	}
	return r;
}
bool div_cx::hittest(const glm::ivec2& pos)
{
	bool r = false;
	auto sps = get_spos();
	glm::ivec2 ips = get_pos(); auto ss = (glm::ivec2)_size;
	glm::vec4 rc = { ips ,ips + ss };
	if (rect_includes(rc, pos)) {
		if (draggable)
		{
			r = true;
		}
		else
		{
			r = vht(widgets.data(), widgets.size(), pos, ips, sps);
			if (!r) {
				widget_t* pws[2] = { horizontal, vertical };
				r = vht(pws, 2, pos, ips, {});
			}
		}
	}
	return r;
}

bool div_cx::press_test(const glm::ivec2& pos)
{
	bool r = false;
	auto sps = get_spos();
	glm::ivec2 ips = get_pos(); auto ss = (glm::ivec2)_size;
	glm::vec4 rc = { ips ,ips + ss };

	if (ckinc > 0 || _bst & (int)BTN_STATE::STATE_ACTIVE) {
		r = true;
	}
	else
	{
		// 按下鼠标时点中控件则捕获
		for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
			auto pw = (widget_t*)*it;
			if (!pw || !pw->visible || pw->_disabled_events)continue;
			if (pw->_bst & (int)BTN_STATE::STATE_ACTIVE) {
				r = true;
			}
		}
	}
	return r;
}

size_t div_cx::add_dragpos(const glm::ivec2& pos, const glm::ivec2& size)
{
	auto ps = drags.size();
	drag_v6 t = {};
	t.pos = pos;
	t.size = size;
	t.z = 0;
	drags.push_back(t);
	update_drag = true;
	return ps;
}

void div_cx::remove_dragpos(size_t idx)
{
	drags.erase(drags.begin() + idx); update_drag = true;
}

glm::ivec3 div_cx::get_dragpos(size_t idx)
{
	glm::ivec3 r = (idx < drags.size()) ? glm::ivec3(drags[idx].pos, drags[idx].z) : glm::ivec3();
	r += glm::ivec3(get_spos(), 0);
	return r;
}

drag_v6* div_cx::get_dragv6(size_t idx)
{
	return (idx < drags.size()) ? &drags[idx] : nullptr;
}

bool div_cx::update(float delta)
{
	int ic = 0;
	if (update_drag)
	{
		dragsp.clear();
		for (auto& it : drags) { dragsp.push_back(&it); }
		sortdg();
		update_drag = false; ic++;
	}
	for (auto& it : widgets) {
		ic += it->update(delta);
	}
	if (uplayout)
	{
		clayout(); ic++;
	}
	if (valid) {
		sort_draw.clear();
		sort_draw.reserve(widgets.size());
		for (auto p : widgets) {
			sort_draw.push_back(p);
		}
		std::stable_sort(sort_draw.begin(), sort_draw.end(), [](widget_t* p1, widget_t* p2) {return p1->dindex < p2->dindex; });
		valid = false;
		ic++;
	}
	return ic > 0;
}
void div_cx::draw(rvg_cx* rv)
{
	auto sps = get_spos();	// 获取滚动量
	auto ss = get_size();
	if (border.w || border.x) {
		rv->push_view(glm::ivec4(0, 0, ss));
		rv->save();
		rv->set_line_width(border.y);
		glm::vec2 rc = ss;
		rc -= border.y;
		rv->add_rect({ 0.5,0.5,rc }, border.z);
		rv->fill_stroke(border.w, border.x);
		rv->restore();
		rv->pop_view();
	}

	for (auto& it : sort_draw) {
		if (it->visible)
		{
			it->draw(rv);
		}
	}
	draw_last(rv);
}

void div_cx::draw_last(rvg_cx* rv)
{}
void div_cx::set_editing(const std::string& str, const glm::ivec2& cpos, int lineheight)
{
	editingstr = str;
	editpos = cpos;
	line_height = lineheight;
}

glm::vec2 get_margin_size(flex_data* c) {
	glm::vec2 m = {};
	if (c)
	{
		m.x = c->margin_left + c->margin_right;
		m.y = c->margin_top + c->margin_bottom;
	}
	return m;
}
// 计算一行布局，range为当前行的控件起始索引和数量，tempfv为临时使用的数组
glm::vec2 calc_line_layout(widget_t** p, const glm::ivec2& range, const glm::ivec2& box_size, flex_data* boxflex, flex_data* flex_child, const glm::ivec2& pos, std::vector<node_dt>& tempfv)
{
	int count = range.y;
	tempfv.resize(count + 1);
	auto cpos = pos;
	node_dt* fnode = tempfv.data();
	auto cp = fnode + 1;
	*fnode = {};
	flex_data tf[2] = {};
	if (boxflex)
	{
		tf[0] = *boxflex;
		fnode->size = box_size;
		fnode->child = cp;
		fnode->child_count = count;
		fnode->line_count = 0;
	}
	glm::vec2 margin_size = {};
	glm::vec2 margin_pos = {};
	if (flex_child) {
		tf[1] = *flex_child;
		if (tf[0].wrap > flex_wrap::NO_WRAP)
		{
			margin_size = get_margin_size(flex_child);
			margin_pos.x = flex_child->margin_left;
			margin_pos.y = flex_child->margin_top;
			cpos += margin_pos;
			tf[1].margin_left = 0;
			tf[1].margin_right = 0;
			tf[1].margin_top = 0;
			tf[1].margin_bottom = 0;
		}
	}
	for (int i = 0; i < count; i++)
	{
		auto it = p[range.x + i];
		cp[i] = {};
		cp[i].index = 1;
		cp[i].size = it->get_size() + margin_size;
		cp[i].position = it->_absolute ? 1 : 0;
	}
	auto nrc = flex_layout_calc(tf, 2, fnode, fnode->child_count + 1);
	for (size_t y = 0; y < fnode->child_count; y++)
	{
		auto t0 = fnode->child + y;
		auto& bt = p[range.x + y];
		if (bt->_absolute)
		{
			continue;
		}
		glm::vec2 vps2 = t0->frame;
		vps2 += cpos;
		bt->set_pos(vps2);
	}
	return glm::vec2(nrc.z, nrc.w);
}
void div_cx::clayout()
{
	if (!uplayout)return;
	uplayout = false;
	valid = true;
	widget_t** p = widgets.data();
	glm::vec2 pos = {};
	if (lines.empty())
	{
		// 竖排
		calc_line_layout(p, { 0,widgets.size() }, { _size.x,_size.y }, &flex, &flex_child, pos, tempfv);
	}
	else {
		for (auto& it : lines)
		{
			auto cs = calc_line_layout(p, it, { _size.x,_size.y }, &flex, &flex_child, pos, tempfv);
			pos.y += cs.y;
		}
	}
	return;
}


page_cx::page_cx()
{}

page_cx::~page_cx()
{}


#endif // 1




#if 1

// A cardinal direction
enum GuiDir : int
{
	GuiDir_None = -1,
	GuiDir_Left = 0,
	GuiDir_Right = 1,
	GuiDir_Up = 2,
	GuiDir_Down = 3,
	GuiDir_COUNT
};

#define STB_TEXTEDIT_CHARTYPE   char
#define STB_TEXTEDIT_STRING     text_control

// get the base type

#ifndef STB_TEXTEDIT_UNDOSTATECOUNT
#define STB_TEXTEDIT_UNDOSTATECOUNT   99
#endif
#ifndef STB_TEXTEDIT_UNDOCHARCOUNT
#define STB_TEXTEDIT_UNDOCHARCOUNT   999
#endif
#ifndef STB_TEXTEDIT_CHARTYPE
#define STB_TEXTEDIT_CHARTYPE        int
#endif
#ifndef STB_TEXTEDIT_POSITIONTYPE
#define STB_TEXTEDIT_POSITIONTYPE    int
#endif

struct StbUndoRecord
{
	// private data
	STB_TEXTEDIT_POSITIONTYPE  where;
	STB_TEXTEDIT_POSITIONTYPE  insert_length;
	STB_TEXTEDIT_POSITIONTYPE  delete_length;
	int                        char_storage;
};

struct StbUndoState
{
	// private data
	StbUndoRecord          undo_rec[STB_TEXTEDIT_UNDOSTATECOUNT];
	STB_TEXTEDIT_CHARTYPE  undo_char[STB_TEXTEDIT_UNDOCHARCOUNT];
	short undo_point, redo_point;
	int undo_char_point, redo_char_point;
};

struct STB_TexteditState
{
	/////////////////////
	//
	// public data
	//

	int cursor;
	// position of the text cursor within the string

	int select_start;          // selection start point
	int select_end;
	// selection start and end point in characters; if equal, no selection.
	// note that start may be less than or greater than end (e.g. when
	// dragging the mouse, start is where the initial click was, and you
	// can drag in either direction)

	bool insert_mode;
	// each textfield keeps its own insert mode state. to keep an app-wide
	// insert mode, copy this value in/out of the app state

	int row_count_per_page;
	// page size in number of row.
	// this value MUST be set to >0 for pageup or pagedown in multilines documents.

	/////////////////////
	//
	// private data
	//
	bool cursor_at_end_of_line; // not implemented yet
	bool initialized;
	bool has_preferred_x;
	bool single_line;
	bool padding1, padding2, padding3;
	float preferred_x; // this determines where the cursor up/down tries to seek to along x
	StbUndoState undostate;
};


////////////////////////////////////////////////////////////////////////
//
//     StbTexteditRow
//
// Result of layout query, used by stb_textedit to determine where
// the text in each row is.

// result of layout query
struct StbTexteditRow
{
	float x0, x1;             // starting x location, end x location (allows for align=right, etc)
	float baseline_y_delta;  // position of baseline relative to previous row's baseline
	float ymin, ymax;         // height of row above and below baseline
	int num_chars;
};

// define our editor structure
struct text_control
{
	STB_TexteditState state = {};
	std::string str;
	size_t curline = 0;				// 当前行号
	size_t curline_idx = 0;			// 当前行号偏移
	font_family_t* family = 0;
	int font_size = 16;
	int c_ct = 0;
	int c_d = 0;
	int64_t ccursor8 = 0;	//当前光标字符
	int64_t caret_old = {};		//保存输入光标
	glm::ivec3 cursor_pos = {};
	glm::ivec2 scroll_pos = {};				// 滚动坐标
	glm::ivec2 _align_pos = {};				// 对齐坐标
	glm::ivec2 _align_pos1 = {};			// 对齐坐标
	glm::vec2 view = {};	// 区域

	std::vector<glm::ivec2> lvs;// 行开始结束
	std::vector<std::vector<int>> widths;// 字符偏移
	std::vector<glm::ivec4> rangerc;
	PathsD range_path;						// 圆角选区缓存
	path_v ptr_path;
	float round_path = 0.28;				// 圆角比例
	float _r_posy = -1;						// 选区偏移

	int lineheight = 0;
	int8_t LastMoveDirectionLR = 0;
	bool is_scroll = true;
};

// define all the #defines needed 

#define KEYDOWN_BIT                    0x80000000

#define STB_TEXTEDIT_STRINGLEN(tc)     ((tc)->str.size())
#define STB_TEXTEDIT_LAYOUTROW         stb_textedit_layoutrow
#define STB_TEXTEDIT_GETWIDTH		   stb_textedit_getwidth
#define STB_TEXTEDIT_KEYTOTEXT(key)    (((key) & KEYDOWN_BIT) ? 0 : (key))
#define STB_TEXTEDIT_GETCHAR(tc,i)     ((tc)->str[i])
#define STB_TEXTEDIT_NEWLINE           '\n'
#define STB_TEXTEDIT_IS_SPACE(ch)      isspace(ch)
#define STB_TEXTEDIT_DELETECHARS       delete_chars
#define STB_TEXTEDIT_INSERTCHARS       insert_chars

#define STB_TEXTEDIT_K_SHIFT           0x40000000
#define STB_TEXTEDIT_K_CONTROL         0x20000000
#define STB_TEXTEDIT_K_LEFT            (KEYDOWN_BIT | 1) // actually use VK_LEFT, SDLK_LEFT, etc
#define STB_TEXTEDIT_K_RIGHT           (KEYDOWN_BIT | 2) // VK_RIGHT
#define STB_TEXTEDIT_K_UP              (KEYDOWN_BIT | 3) // VK_UP
#define STB_TEXTEDIT_K_DOWN            (KEYDOWN_BIT | 4) // VK_DOWN
#define STB_TEXTEDIT_K_LINESTART       (KEYDOWN_BIT | 5) // VK_HOME
#define STB_TEXTEDIT_K_LINEEND         (KEYDOWN_BIT | 6) // VK_END
#define STB_TEXTEDIT_K_TEXTSTART       (STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_TEXTEND         (STB_TEXTEDIT_K_LINEEND   | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_DELETE          (KEYDOWN_BIT | 7) // VK_DELETE
#define STB_TEXTEDIT_K_BACKSPACE       (KEYDOWN_BIT | 8) // VK_BACKSPACE
#define STB_TEXTEDIT_K_UNDO            (KEYDOWN_BIT | STB_TEXTEDIT_K_CONTROL | 'z')
#define STB_TEXTEDIT_K_REDO            (KEYDOWN_BIT | STB_TEXTEDIT_K_CONTROL | 'y')
#define STB_TEXTEDIT_K_INSERT          (KEYDOWN_BIT | 9) // VK_INSERT
#define STB_TEXTEDIT_K_WORDLEFT        (STB_TEXTEDIT_K_LEFT  | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_WORDRIGHT       (STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_PGUP            (KEYDOWN_BIT | 10) // VK_PGUP -- not implemented
#define STB_TEXTEDIT_K_PGDOWN          (KEYDOWN_BIT | 11) // VK_PGDOWN -- not implemented

#define STB_TEXTEDIT_GETWIDTH_NEWLINE -1

#ifndef STB_TEXTEDIT_memmove
#include <string.h>
#define STB_TEXTEDIT_memmove memmove
#endif

#ifndef STB_TEXTEDIT_GETPREVCHARINDEX
#define STB_TEXTEDIT_GETPREVCHARINDEX stb_textedit_getprevcharindex
#endif
#ifndef STB_TEXTEDIT_GETNEXTCHARINDEX
#define STB_TEXTEDIT_GETNEXTCHARINDEX stb_textedit_getnextcharindex
#endif


float stb_textedit_getwidth(STB_TEXTEDIT_STRING* str, int n, int idx);


// define the functions we need
void stb_textedit_layoutrow1(StbTexteditRow* row, STB_TEXTEDIT_STRING* str, int start_i)
{
	int remaining_chars = str->str.size() - start_i;
	row->num_chars = remaining_chars; // should do real word wrap here
	row->x0 = 0;
	row->ymin = 0;
	auto r = row;
	int i = start_i;
	float row_width = 0.0f;
	while (str->str[i] != '\n' && i < STB_TEXTEDIT_STRINGLEN(str)) {
		row_width += STB_TEXTEDIT_GETWIDTH(str, start_i, i);
		if (str->view.x > 0 && row_width > str->view.x) break; // 自动换行触发条件
		i++;
	}
	r->x1 = row_width;
	row->ymax = row->baseline_y_delta = str->lineheight;
	r->num_chars = (i - start_i) + 1;

}
void stb_textedit_layoutrow(StbTexteditRow* row, STB_TEXTEDIT_STRING* str, int start_i) {
	int remaining_chars = str->str.size() - start_i;
	row->num_chars = remaining_chars;
	row->x0 = 0;
	row->ymin = 0;
	auto r = row;
	int i = start_i;
	float row_width = 0.0f;
	int len = STB_TEXTEDIT_STRINGLEN(str);

	while (i < len) {
		if (str->str[i] == '\n') break;  // 遇到换行符退出

		float char_width = STB_TEXTEDIT_GETWIDTH(str, 0, i);
		float next_width = row_width + char_width;

		// 预判宽度是否超限
		if (str->view.x > 0 && next_width > str->view.x) break;

		row_width = next_width;
		i++;
	}

	// 显式处理换行符 
	if (i < len && str->str[i] == '\n') {
		i++;
	}

	r->x1 = row_width;
	row->ymax = row->baseline_y_delta = str->lineheight;
	r->num_chars = i - start_i;  // 修正字符数计算 
}

int delete_chars(STB_TEXTEDIT_STRING* str, int pos, int num)
{
	str->str.erase(pos, num);
	return num;
}

int insert_chars(STB_TEXTEDIT_STRING* str, int pos, const STB_TEXTEDIT_CHARTYPE* newtext, int num)
{
	str->str.insert(pos, newtext, num);
	return num;
}
//float STB_TEXTEDIT_GETWIDTH(ImGuiInputTextState* obj, int line_start_idx, int char_idx)
//{
//	unsigned int c; ImTextCharFromUtf8(&c, obj->TextSrc + line_start_idx + char_idx, obj->TextSrc + obj->TextLen);
//	if ((ImWchar)c == '\n') 
//		return IMSTB_TEXTEDIT_GETWIDTH_NEWLINE;
//	ImGuiContext& g = *obj->Ctx;
//	return g.FontBaked->GetCharAdvance((ImWchar)c) * g.FontBakedScale;
//}

// 行开始位置，字符位置
float stb_textedit_getwidth(STB_TEXTEDIT_STRING* str, int line_start_idx, int char_idx)
{
	if (str && str->family)
	{
		const char* p = str->str.c_str();
		uint32_t ch = 0;
		auto nn = md::utf8_to_unicode(p + line_start_idx + char_idx, &ch);
		if (ch == '\n') {
			return STB_TEXTEDIT_GETWIDTH_NEWLINE;
		}
		if (ch)
		{
			auto rc = font_get_char_extent(ch, str->font_size, str->family, 0);
			return rc.z;
		}
	}
	return 0;
}
// 返回上一个字符位置
int stb_textedit_getprevcharindex(STB_TEXTEDIT_STRING* str, int idx) {
	auto p = str->str.c_str();
	auto p1 = md::get_utf8_prev(p + idx);
	return (p1 - p);
}
// 返回下一个字符位置
int stb_textedit_getnextcharindex(STB_TEXTEDIT_STRING* str, int idx) {
	auto p = str->str.c_str() + idx;
	auto p1 = md::utf8_next_char(p);
	return (p1 - p) + idx;
}
/////////////////////////////////////////////////////////////////////////////
//
//      Mouse input handling
//

// traverse the layout to locate the nearest character to a display position
static int stb_text_locate_coord(STB_TEXTEDIT_STRING* str, float x, float y, int* out_side_on_line)
{
	StbTexteditRow r;
	int n = STB_TEXTEDIT_STRINGLEN(str);
	float base_y = 0, prev_x;
	int i = 0, k;

	r.x0 = r.x1 = 0;
	r.ymin = r.ymax = 0;
	r.num_chars = 0;
	*out_side_on_line = 0;

	// search rows to find one that straddles 'y'
	while (i < n) {
		STB_TEXTEDIT_LAYOUTROW(&r, str, i);
		if (r.num_chars <= 0)
			return n;

		if (i == 0 && y < base_y + r.ymin)
			return 0;

		if (y < base_y + r.ymax)
			break;

		i += r.num_chars;
		base_y += r.baseline_y_delta;
	}

	// below all text, return 'after' last character
	if (i >= n)
	{
		*out_side_on_line = 1;
		return n;
	}

	// check if it's before the beginning of the line
	if (x < r.x0)
		return i;

	// check if it's before the end of the line
	if (x < r.x1) {
		// search characters in row for one that straddles 'x'
		prev_x = r.x0;
		for (k = 0; k < r.num_chars; k = STB_TEXTEDIT_GETNEXTCHARINDEX(str, i + k) - i) {
			float w = STB_TEXTEDIT_GETWIDTH(str, i, k);
			if (x < prev_x + w) {
				*out_side_on_line = (k == 0) ? 0 : 1;
				if (x < prev_x + w / 2)
					return k + i;
				else
					return STB_TEXTEDIT_GETNEXTCHARINDEX(str, i + k);
			}
			prev_x += w;
		}
		// shouldn't happen, but if it does, fall through to end-of-line case
	}

	// if the last character is a newline, return that. otherwise return 'after' the last character
	*out_side_on_line = 1;
	if (STB_TEXTEDIT_GETCHAR(str, i + r.num_chars - 1) == STB_TEXTEDIT_NEWLINE)
		return i + r.num_chars - 1;
	else
		return i + r.num_chars;
}

// API click: on mouse down, move the cursor to the clicked location, and reset the selection
static void stb_textedit_click(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, float x, float y)
{
	// In single-line mode, just always make y = 0. This lets the drag keep working if the mouse
	// goes off the top or bottom of the text
	int side_on_line;
	if (state->single_line)
	{
		StbTexteditRow r;
		STB_TEXTEDIT_LAYOUTROW(&r, str, 0);
		y = r.ymin;
	}

	state->cursor = stb_text_locate_coord(str, x, y, &side_on_line);
	state->select_start = state->cursor;
	state->select_end = state->cursor;
	state->has_preferred_x = 0;
	str->LastMoveDirectionLR = (side_on_line ? GuiDir_Right : GuiDir_Left);
}

// API drag: on mouse drag, move the cursor and selection endpoint to the clicked location
static void stb_textedit_drag(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, float x, float y)
{
	int p = 0;
	int side_on_line;

	// In single-line mode, just always make y = 0. This lets the drag keep working if the mouse
	// goes off the top or bottom of the text
	if (state->single_line)
	{
		StbTexteditRow r;
		STB_TEXTEDIT_LAYOUTROW(&r, str, 0);
		y = r.ymin;
	}

	if (state->select_start == state->select_end)
		state->select_start = state->cursor;

	p = stb_text_locate_coord(str, x, y, &side_on_line);
	state->cursor = state->select_end = p;
	str->LastMoveDirectionLR = (side_on_line ? GuiDir_Right : GuiDir_Left);
}

/////////////////////////////////////////////////////////////////////////////
//
//      Keyboard input handling
//

// forward declarations
static void stb_text_undo(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
static void stb_text_redo(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
static void stb_text_makeundo_delete(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int length);
static void stb_text_makeundo_insert(STB_TexteditState* state, int where, int length);
static void stb_text_makeundo_replace(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int old_length, int new_length);

typedef struct
{
	float x, y;    // position of n'th character
	float height; // height of line
	int first_char, length; // first char of row, and length
	int prev_first;  // first char of previous row
} StbFindState;

// find the x/y location of a character, and remember info about the previous row in
// case we get a move-up event (for page up, we'll have to rescan)
static void stb_textedit_find_charpos(StbFindState* find, STB_TEXTEDIT_STRING* str, int n, int single_line)
{
	StbTexteditRow r;
	int prev_start = 0;
	int z = STB_TEXTEDIT_STRINGLEN(str);
	int i = 0, first;

	if (n == z && single_line) {
		// special case if it's at the end (may not be needed?)
		STB_TEXTEDIT_LAYOUTROW(&r, str, 0);
		find->y = 0;
		find->first_char = 0;
		find->length = z;
		find->height = r.ymax - r.ymin;
		find->x = r.x1;
		return;
	}

	// search rows to find the one that straddles character n
	find->y = 0;

	for (;;) {
		STB_TEXTEDIT_LAYOUTROW(&r, str, i);
		if (n < i + r.num_chars)
			break;
		if (str->LastMoveDirectionLR == GuiDir_Right && str->state.cursor > 0 && str->state.cursor == i + r.num_chars && STB_TEXTEDIT_GETCHAR(str, i + r.num_chars - 1) != STB_TEXTEDIT_NEWLINE) // [DEAR IMGUI] Wrapping point handling
			break;
		if (i + r.num_chars == z && z > 0 && STB_TEXTEDIT_GETCHAR(str, z - 1) != STB_TEXTEDIT_NEWLINE)  // [DEAR IMGUI] special handling for last line
			break;   // [DEAR IMGUI]
		prev_start = i;
		i += r.num_chars;
		find->y += r.baseline_y_delta;
		if (i == z) // [DEAR IMGUI]
		{
			r.num_chars = 0; // [DEAR IMGUI]
			break;   // [DEAR IMGUI]
		}
	}

	find->first_char = first = i;
	find->length = r.num_chars;
	find->height = r.ymax - r.ymin;
	find->prev_first = prev_start;

	// now scan to find xpos
	find->x = r.x0;
	for (i = 0; first + i < n; i = STB_TEXTEDIT_GETNEXTCHARINDEX(str, first + i) - first)
		find->x += STB_TEXTEDIT_GETWIDTH(str, first, i);
}

#define STB_TEXT_HAS_SELECTION(s)   ((s)->select_start != (s)->select_end)

// make the selection/cursor state valid if client altered the string
static void stb_textedit_clamp(STB_TEXTEDIT_STRING* str, STB_TexteditState* state)
{
	int n = STB_TEXTEDIT_STRINGLEN(str);
	if (STB_TEXT_HAS_SELECTION(state)) {
		if (state->select_start > n) state->select_start = n;
		if (state->select_end > n) state->select_end = n;
		// if clamping forced them to be equal, move the cursor to match
		if (state->select_start == state->select_end)
			state->cursor = state->select_start;
	}
	if (state->cursor > n) state->cursor = n;
}

// delete characters while updating undo
static void stb_textedit_delete(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int len)
{
	stb_text_makeundo_delete(str, state, where, len);
	STB_TEXTEDIT_DELETECHARS(str, where, len);
	state->has_preferred_x = 0;
}

// delete the section
static void stb_textedit_delete_selection(STB_TEXTEDIT_STRING* str, STB_TexteditState* state)
{
	stb_textedit_clamp(str, state);
	if (STB_TEXT_HAS_SELECTION(state)) {
		if (state->select_start < state->select_end) {
			stb_textedit_delete(str, state, state->select_start, state->select_end - state->select_start);
			state->select_end = state->cursor = state->select_start;
		}
		else {
			stb_textedit_delete(str, state, state->select_end, state->select_start - state->select_end);
			state->select_start = state->cursor = state->select_end;
		}
		state->has_preferred_x = 0;
	}
}

// canoncialize the selection so start <= end
static void stb_textedit_sortselection(STB_TexteditState* state)
{
	if (state->select_end < state->select_start) {
		int temp = state->select_end;
		state->select_end = state->select_start;
		state->select_start = temp;
	}
}

// move cursor to first character of selection
static void stb_textedit_move_to_first(STB_TexteditState* state)
{
	if (STB_TEXT_HAS_SELECTION(state)) {
		stb_textedit_sortselection(state);
		state->cursor = state->select_start;
		state->select_end = state->select_start;
		state->has_preferred_x = 0;
	}
}

// move cursor to last character of selection
static void stb_textedit_move_to_last(STB_TEXTEDIT_STRING* str, STB_TexteditState* state)
{
	if (STB_TEXT_HAS_SELECTION(state)) {
		stb_textedit_sortselection(state);
		stb_textedit_clamp(str, state);
		state->cursor = state->select_end;
		state->select_start = state->select_end;
		state->has_preferred_x = 0;
	}
}

// [DEAR IMGUI] Extracted this function so we can more easily add support for word-wrapping.
#ifndef STB_TEXTEDIT_MOVELINESTART
static int stb_textedit_move_line_start(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int cursor)
{
	if (state->single_line)
		return 0;
	while (cursor > 0) {
		int prev = STB_TEXTEDIT_GETPREVCHARINDEX(str, cursor);
		if (STB_TEXTEDIT_GETCHAR(str, prev) == STB_TEXTEDIT_NEWLINE)
			break;
		cursor = prev;
	}
	return cursor;
}
#define STB_TEXTEDIT_MOVELINESTART stb_textedit_move_line_start
#endif
#ifndef STB_TEXTEDIT_MOVELINEEND
static int stb_textedit_move_line_end(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int cursor)
{
	int n = STB_TEXTEDIT_STRINGLEN(str);
	if (state->single_line)
		return n;
	while (cursor < n && STB_TEXTEDIT_GETCHAR(str, cursor) != STB_TEXTEDIT_NEWLINE)
		cursor = STB_TEXTEDIT_GETNEXTCHARINDEX(str, cursor);
	return cursor;
}
#define STB_TEXTEDIT_MOVELINEEND stb_textedit_move_line_end
#endif

#ifdef STB_TEXTEDIT_IS_SPACE
static int is_word_boundary(STB_TEXTEDIT_STRING* str, int idx)
{
	return idx > 0 ? (STB_TEXTEDIT_IS_SPACE(STB_TEXTEDIT_GETCHAR(str, idx - 1)) && !STB_TEXTEDIT_IS_SPACE(STB_TEXTEDIT_GETCHAR(str, idx))) : 1;
}

#ifndef STB_TEXTEDIT_MOVEWORDLEFT
static int stb_textedit_move_to_word_previous(STB_TEXTEDIT_STRING* str, int c)
{
	c = STB_TEXTEDIT_GETPREVCHARINDEX(str, c); // always move at least one character
	while (c >= 0 && !is_word_boundary(str, c))
		c = STB_TEXTEDIT_GETPREVCHARINDEX(str, c);

	if (c < 0)
		c = 0;

	return c;
}
#define STB_TEXTEDIT_MOVEWORDLEFT stb_textedit_move_to_word_previous
#endif

#ifndef STB_TEXTEDIT_MOVEWORDRIGHT
static int stb_textedit_move_to_word_next(STB_TEXTEDIT_STRING* str, int c)
{
	const int len = STB_TEXTEDIT_STRINGLEN(str);
	c = STB_TEXTEDIT_GETNEXTCHARINDEX(str, c); // always move at least one character
	while (c < len && !is_word_boundary(str, c))
		c = STB_TEXTEDIT_GETNEXTCHARINDEX(str, c);

	if (c > len)
		c = len;

	return c;
}
#define STB_TEXTEDIT_MOVEWORDRIGHT stb_textedit_move_to_word_next
#endif

#endif

// update selection and cursor to match each other
static void stb_textedit_prep_selection_at_cursor(STB_TexteditState* state)
{
	if (!STB_TEXT_HAS_SELECTION(state))
		state->select_start = state->select_end = state->cursor;
	else
		state->cursor = state->select_end;
}

// API cut: delete selection
static int stb_textedit_cut(STB_TEXTEDIT_STRING* str, STB_TexteditState* state)
{
	if (STB_TEXT_HAS_SELECTION(state)) {
		stb_textedit_delete_selection(str, state); // implicitly clamps
		state->has_preferred_x = 0;
		return 1;
	}
	return 0;
}

// API paste: replace existing selection with passed-in text
static int stb_textedit_paste_internal(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, STB_TEXTEDIT_CHARTYPE* text, int len)
{
	// if there's a selection, the paste should delete it
	stb_textedit_clamp(str, state);
	stb_textedit_delete_selection(str, state);
	// try to insert the characters
	len = STB_TEXTEDIT_INSERTCHARS(str, state->cursor, text, len);
	if (len) {
		stb_text_makeundo_insert(state, state->cursor, len);
		state->cursor += len;
		state->has_preferred_x = 0;
		return 1;
	}
	// note: paste failure will leave deleted selection, may be restored with an undo (see https://github.com/nothings/stb/issues/734 for details)
	return 0;
}

#ifndef STB_TEXTEDIT_KEYTYPE
#define STB_TEXTEDIT_KEYTYPE int
#endif

// API key: process text input
// [DEAR IMGUI] Added stb_textedit_text(), extracted out and called by stb_textedit_key() for backward compatibility.
static void stb_textedit_text(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, const STB_TEXTEDIT_CHARTYPE* text, int text_len)
{
	// can't add newline in single-line mode
	if (text[0] == '\n' && state->single_line)
		return;

	if (state->insert_mode && !STB_TEXT_HAS_SELECTION(state) && state->cursor < STB_TEXTEDIT_STRINGLEN(str)) {
		stb_text_makeundo_replace(str, state, state->cursor, 1, 1);
		STB_TEXTEDIT_DELETECHARS(str, state->cursor, 1);
		text_len = STB_TEXTEDIT_INSERTCHARS(str, state->cursor, text, text_len);
		if (text_len) {
			state->cursor += text_len;
			state->has_preferred_x = 0;
		}
	}
	else {
		stb_textedit_delete_selection(str, state); // implicitly clamps
		text_len = STB_TEXTEDIT_INSERTCHARS(str, state->cursor, text, text_len);
		if (text_len) {
			stb_text_makeundo_insert(state, state->cursor, text_len);
			state->cursor += text_len;
			state->has_preferred_x = 0;
		}
	}
}

// API key: process a keyboard input
static void stb_textedit_key(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, STB_TEXTEDIT_KEYTYPE key)
{
retry:
	switch (key) {
	default: {
#ifdef STB_TEXTEDIT_KEYTOTEXT
		// This is not suitable for UTF-8 support.
		int c = STB_TEXTEDIT_KEYTOTEXT(key);
		if (c > 0) {
			STB_TEXTEDIT_CHARTYPE ch = (STB_TEXTEDIT_CHARTYPE)c;
			stb_textedit_text(str, state, &ch, 1);
		}
#endif
		break;
	}

#ifdef STB_TEXTEDIT_K_INSERT
	case STB_TEXTEDIT_K_INSERT:
		state->insert_mode = !state->insert_mode;
		break;
#endif

	case STB_TEXTEDIT_K_UNDO:
		stb_text_undo(str, state);
		state->has_preferred_x = 0;
		break;

	case STB_TEXTEDIT_K_REDO:
		stb_text_redo(str, state);
		state->has_preferred_x = 0;
		break;

	case STB_TEXTEDIT_K_LEFT:
		// if currently there's a selection, move cursor to start of selection
		if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_move_to_first(state);
		else
			if (state->cursor > 0)
				state->cursor = STB_TEXTEDIT_GETPREVCHARINDEX(str, state->cursor);
		state->has_preferred_x = 0;
		break;

	case STB_TEXTEDIT_K_RIGHT:
		// if currently there's a selection, move cursor to end of selection
		if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_move_to_last(str, state);
		else
			state->cursor = STB_TEXTEDIT_GETNEXTCHARINDEX(str, state->cursor);
		stb_textedit_clamp(str, state);
		state->has_preferred_x = 0;
		break;

	case STB_TEXTEDIT_K_LEFT | STB_TEXTEDIT_K_SHIFT:
		stb_textedit_clamp(str, state);
		stb_textedit_prep_selection_at_cursor(state);
		// move selection left
		if (state->select_end > 0)
			state->select_end = STB_TEXTEDIT_GETPREVCHARINDEX(str, state->select_end);
		state->cursor = state->select_end;
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_MOVEWORDLEFT
	case STB_TEXTEDIT_K_WORDLEFT:
		if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_move_to_first(state);
		else {
			state->cursor = STB_TEXTEDIT_MOVEWORDLEFT(str, state->cursor);
			stb_textedit_clamp(str, state);
		}
		break;

	case STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT:
		if (!STB_TEXT_HAS_SELECTION(state))
			stb_textedit_prep_selection_at_cursor(state);

		state->cursor = STB_TEXTEDIT_MOVEWORDLEFT(str, state->cursor);
		state->select_end = state->cursor;

		stb_textedit_clamp(str, state);
		break;
#endif

#ifdef STB_TEXTEDIT_MOVEWORDRIGHT
	case STB_TEXTEDIT_K_WORDRIGHT:
		if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_move_to_last(str, state);
		else {
			state->cursor = STB_TEXTEDIT_MOVEWORDRIGHT(str, state->cursor);
			stb_textedit_clamp(str, state);
		}
		break;

	case STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT:
		if (!STB_TEXT_HAS_SELECTION(state))
			stb_textedit_prep_selection_at_cursor(state);

		state->cursor = STB_TEXTEDIT_MOVEWORDRIGHT(str, state->cursor);
		state->select_end = state->cursor;

		stb_textedit_clamp(str, state);
		break;
#endif

	case STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_SHIFT:
		stb_textedit_prep_selection_at_cursor(state);
		// move selection right
		state->select_end = STB_TEXTEDIT_GETNEXTCHARINDEX(str, state->select_end);
		stb_textedit_clamp(str, state);
		state->cursor = state->select_end;
		state->has_preferred_x = 0;
		break;

	case STB_TEXTEDIT_K_DOWN:
	case STB_TEXTEDIT_K_DOWN | STB_TEXTEDIT_K_SHIFT:
	case STB_TEXTEDIT_K_PGDOWN:
	case STB_TEXTEDIT_K_PGDOWN | STB_TEXTEDIT_K_SHIFT: {
		StbFindState find;
		StbTexteditRow row;
		int i, j, sel = (key & STB_TEXTEDIT_K_SHIFT) != 0;
		int is_page = (key & ~STB_TEXTEDIT_K_SHIFT) == STB_TEXTEDIT_K_PGDOWN;
		int row_count = is_page ? state->row_count_per_page : 1;

		if (!is_page && state->single_line) {
			// on windows, up&down in single-line behave like left&right
			key = STB_TEXTEDIT_K_RIGHT | (key & STB_TEXTEDIT_K_SHIFT);
			goto retry;
		}

		if (sel)
			stb_textedit_prep_selection_at_cursor(state);
		else if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_move_to_last(str, state);

		// compute current position of cursor point
		stb_textedit_clamp(str, state);
		stb_textedit_find_charpos(&find, str, state->cursor, state->single_line);

		for (j = 0; j < row_count; ++j) {
			float x, goal_x = state->has_preferred_x ? state->preferred_x : find.x;
			int start = find.first_char + find.length;

			if (find.length == 0)
				break;

			// [DEAR IMGUI]
			// going down while being on the last line shouldn't bring us to that line end
			//if (STB_TEXTEDIT_GETCHAR(str, find.first_char + find.length - 1) != STB_TEXTEDIT_NEWLINE)
			//   break;

			// now find character position down a row
			state->cursor = start;
			stb_textedit_clamp(str, state);
			STB_TEXTEDIT_LAYOUTROW(&row, str, state->cursor);
			x = row.x0;
			for (i = 0; i < row.num_chars; ) {
				float dx = STB_TEXTEDIT_GETWIDTH(str, start, i);
				int next = STB_TEXTEDIT_GETNEXTCHARINDEX(str, state->cursor);
#ifdef STB_TEXTEDIT_GETWIDTH_NEWLINE
				if (dx == STB_TEXTEDIT_GETWIDTH_NEWLINE)
					break;
#endif
				x += dx;
				if (x > goal_x)
					break;
				i += next - state->cursor;
				state->cursor = next;
			}
			stb_textedit_clamp(str, state);

			if (state->cursor == find.first_char + find.length)
				str->LastMoveDirectionLR = GuiDir_Left;
			state->has_preferred_x = 1;
			state->preferred_x = goal_x;

			if (sel)
				state->select_end = state->cursor;

			// go to next line
			find.first_char = find.first_char + find.length;
			find.length = row.num_chars;
		}
		break;
	}

	case STB_TEXTEDIT_K_UP:
	case STB_TEXTEDIT_K_UP | STB_TEXTEDIT_K_SHIFT:
	case STB_TEXTEDIT_K_PGUP:
	case STB_TEXTEDIT_K_PGUP | STB_TEXTEDIT_K_SHIFT: {
		StbFindState find;
		StbTexteditRow row;
		int i, j, prev_scan, sel = (key & STB_TEXTEDIT_K_SHIFT) != 0;
		int is_page = (key & ~STB_TEXTEDIT_K_SHIFT) == STB_TEXTEDIT_K_PGUP;
		int row_count = is_page ? state->row_count_per_page : 1;

		if (!is_page && state->single_line) {
			// on windows, up&down become left&right
			key = STB_TEXTEDIT_K_LEFT | (key & STB_TEXTEDIT_K_SHIFT);
			goto retry;
		}

		if (sel)
			stb_textedit_prep_selection_at_cursor(state);
		else if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_move_to_first(state);

		// compute current position of cursor point
		stb_textedit_clamp(str, state);
		stb_textedit_find_charpos(&find, str, state->cursor, state->single_line);

		for (j = 0; j < row_count; ++j) {
			float  x, goal_x = state->has_preferred_x ? state->preferred_x : find.x;

			// can only go up if there's a previous row
			if (find.prev_first == find.first_char)
				break;

			// now find character position up a row
			state->cursor = find.prev_first;
			STB_TEXTEDIT_LAYOUTROW(&row, str, state->cursor);
			x = row.x0;
			for (i = 0; i < row.num_chars; ) {
				float dx = STB_TEXTEDIT_GETWIDTH(str, find.prev_first, i);
				int next = STB_TEXTEDIT_GETNEXTCHARINDEX(str, state->cursor);
#ifdef STB_TEXTEDIT_GETWIDTH_NEWLINE
				if (dx == STB_TEXTEDIT_GETWIDTH_NEWLINE)
					break;
#endif
				x += dx;
				if (x > goal_x)
					break;
				i += next - state->cursor;
				state->cursor = next;
			}
			stb_textedit_clamp(str, state);

			if (state->cursor == find.first_char)
				str->LastMoveDirectionLR = GuiDir_Right;
			else if (state->cursor == find.prev_first)
				str->LastMoveDirectionLR = GuiDir_Left;
			state->has_preferred_x = 1;
			state->preferred_x = goal_x;

			if (sel)
				state->select_end = state->cursor;

			// go to previous line
			// (we need to scan previous line the hard way. maybe we could expose this as a new API function?)
			prev_scan = find.prev_first > 0 ? find.prev_first - 1 : 0;
			while (prev_scan > 0)
			{
				int prev = STB_TEXTEDIT_GETPREVCHARINDEX(str, prev_scan);
				if (STB_TEXTEDIT_GETCHAR(str, prev) == STB_TEXTEDIT_NEWLINE)
					break;
				prev_scan = prev;
			}
			find.first_char = find.prev_first;
			find.prev_first = STB_TEXTEDIT_MOVELINESTART(str, state, prev_scan);
		}
		break;
	}

	case STB_TEXTEDIT_K_DELETE:
	case STB_TEXTEDIT_K_DELETE | STB_TEXTEDIT_K_SHIFT:
		if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_delete_selection(str, state);
		else {
			int n = STB_TEXTEDIT_STRINGLEN(str);
			if (state->cursor < n)
				stb_textedit_delete(str, state, state->cursor, STB_TEXTEDIT_GETNEXTCHARINDEX(str, state->cursor) - state->cursor);
		}
		state->has_preferred_x = 0;
		break;

	case STB_TEXTEDIT_K_BACKSPACE:
	case STB_TEXTEDIT_K_BACKSPACE | STB_TEXTEDIT_K_SHIFT:
		if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_delete_selection(str, state);
		else {
			stb_textedit_clamp(str, state);
			if (state->cursor > 0) {
				int prev = STB_TEXTEDIT_GETPREVCHARINDEX(str, state->cursor);
				stb_textedit_delete(str, state, prev, state->cursor - prev);
				state->cursor = prev;
			}
		}
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_K_TEXTSTART2
	case STB_TEXTEDIT_K_TEXTSTART2:
#endif
	case STB_TEXTEDIT_K_TEXTSTART:
		state->cursor = state->select_start = state->select_end = 0;
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_K_TEXTEND2
	case STB_TEXTEDIT_K_TEXTEND2:
#endif
	case STB_TEXTEDIT_K_TEXTEND:
		state->cursor = STB_TEXTEDIT_STRINGLEN(str);
		state->select_start = state->select_end = 0;
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_K_TEXTSTART2
	case STB_TEXTEDIT_K_TEXTSTART2 | STB_TEXTEDIT_K_SHIFT:
#endif
	case STB_TEXTEDIT_K_TEXTSTART | STB_TEXTEDIT_K_SHIFT:
		stb_textedit_prep_selection_at_cursor(state);
		state->cursor = state->select_end = 0;
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_K_TEXTEND2
	case STB_TEXTEDIT_K_TEXTEND2 | STB_TEXTEDIT_K_SHIFT:
#endif
	case STB_TEXTEDIT_K_TEXTEND | STB_TEXTEDIT_K_SHIFT:
		stb_textedit_prep_selection_at_cursor(state);
		state->cursor = state->select_end = STB_TEXTEDIT_STRINGLEN(str);
		state->has_preferred_x = 0;
		break;


#ifdef STB_TEXTEDIT_K_LINESTART2
	case STB_TEXTEDIT_K_LINESTART2:
#endif
	case STB_TEXTEDIT_K_LINESTART:
		stb_textedit_clamp(str, state);
		stb_textedit_move_to_first(state);
		state->cursor = STB_TEXTEDIT_MOVELINESTART(str, state, state->cursor);
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_K_LINEEND2
	case STB_TEXTEDIT_K_LINEEND2:
#endif
	case STB_TEXTEDIT_K_LINEEND: {
		stb_textedit_clamp(str, state);
		stb_textedit_move_to_last(str, state);
		state->cursor = STB_TEXTEDIT_MOVELINEEND(str, state, state->cursor);
		state->has_preferred_x = 0;
		break;
	}

#ifdef STB_TEXTEDIT_K_LINESTART2
	case STB_TEXTEDIT_K_LINESTART2 | STB_TEXTEDIT_K_SHIFT:
#endif
	case STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_SHIFT:
		stb_textedit_clamp(str, state);
		stb_textedit_prep_selection_at_cursor(state);
		state->cursor = STB_TEXTEDIT_MOVELINESTART(str, state, state->cursor);
		state->select_end = state->cursor;
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_K_LINEEND2
	case STB_TEXTEDIT_K_LINEEND2 | STB_TEXTEDIT_K_SHIFT:
#endif
	case STB_TEXTEDIT_K_LINEEND | STB_TEXTEDIT_K_SHIFT: {
		stb_textedit_clamp(str, state);
		stb_textedit_prep_selection_at_cursor(state);
		state->cursor = STB_TEXTEDIT_MOVELINEEND(str, state, state->cursor);
		state->select_end = state->cursor;
		state->has_preferred_x = 0;
		break;
	}
	}
}

/////////////////////////////////////////////////////////////////////////////
//
//      Undo processing
//
// @OPTIMIZE: the undo/redo buffer should be circular

static void stb_textedit_flush_redo(StbUndoState* state)
{
	state->redo_point = STB_TEXTEDIT_UNDOSTATECOUNT;
	state->redo_char_point = STB_TEXTEDIT_UNDOCHARCOUNT;
}

// discard the oldest entry in the undo list
static void stb_textedit_discard_undo(StbUndoState* state)
{
	if (state->undo_point > 0) {
		// if the 0th undo state has characters, clean those up
		if (state->undo_rec[0].char_storage >= 0) {
			int n = state->undo_rec[0].insert_length, i;
			// delete n characters from all other records
			state->undo_char_point -= n;
			STB_TEXTEDIT_memmove(state->undo_char, state->undo_char + n, (size_t)(state->undo_char_point * sizeof(STB_TEXTEDIT_CHARTYPE)));
			for (i = 0; i < state->undo_point; ++i)
				if (state->undo_rec[i].char_storage >= 0)
					state->undo_rec[i].char_storage -= n; // @OPTIMIZE: get rid of char_storage and infer it
		}
		--state->undo_point;
		STB_TEXTEDIT_memmove(state->undo_rec, state->undo_rec + 1, (size_t)(state->undo_point * sizeof(state->undo_rec[0])));
	}
}

// discard the oldest entry in the redo list--it's bad if this
// ever happens, but because undo & redo have to store the actual
// characters in different cases, the redo character buffer can
// fill up even though the undo buffer didn't
static void stb_textedit_discard_redo(StbUndoState* state)
{
	int k = STB_TEXTEDIT_UNDOSTATECOUNT - 1;

	if (state->redo_point <= k) {
		// if the k'th undo state has characters, clean those up
		if (state->undo_rec[k].char_storage >= 0) {
			int n = state->undo_rec[k].insert_length, i;
			// move the remaining redo character data to the end of the buffer
			state->redo_char_point += n;
			STB_TEXTEDIT_memmove(state->undo_char + state->redo_char_point, state->undo_char + state->redo_char_point - n, (size_t)((STB_TEXTEDIT_UNDOCHARCOUNT - state->redo_char_point) * sizeof(STB_TEXTEDIT_CHARTYPE)));
			// adjust the position of all the other records to account for above memmove
			for (i = state->redo_point; i < k; ++i)
				if (state->undo_rec[i].char_storage >= 0)
					state->undo_rec[i].char_storage += n;
		}
		// now move all the redo records towards the end of the buffer; the first one is at 'redo_point'
		// [DEAR IMGUI]
		size_t move_size = (size_t)((STB_TEXTEDIT_UNDOSTATECOUNT - state->redo_point - 1) * sizeof(state->undo_rec[0]));
		const char* buf_begin = (char*)state->undo_rec; (void)buf_begin;
		const char* buf_end = (char*)state->undo_rec + sizeof(state->undo_rec); (void)buf_end;
		assert(((char*)(state->undo_rec + state->redo_point)) >= buf_begin);
		assert(((char*)(state->undo_rec + state->redo_point + 1) + move_size) <= buf_end);
		STB_TEXTEDIT_memmove(state->undo_rec + state->redo_point + 1, state->undo_rec + state->redo_point, move_size);

		// now move redo_point to point to the new one
		++state->redo_point;
	}
}

static StbUndoRecord* stb_text_create_undo_record(StbUndoState* state, int numchars)
{
	// any time we create a new undo record, we discard redo
	stb_textedit_flush_redo(state);

	// if we have no free records, we have to make room, by sliding the
	// existing records down
	if (state->undo_point == STB_TEXTEDIT_UNDOSTATECOUNT)
		stb_textedit_discard_undo(state);

	// if the characters to store won't possibly fit in the buffer, we can't undo
	if (numchars > STB_TEXTEDIT_UNDOCHARCOUNT) {
		state->undo_point = 0;
		state->undo_char_point = 0;
		return NULL;
	}

	// if we don't have enough free characters in the buffer, we have to make room
	while (state->undo_char_point + numchars > STB_TEXTEDIT_UNDOCHARCOUNT)
		stb_textedit_discard_undo(state);

	return &state->undo_rec[state->undo_point++];
}

static STB_TEXTEDIT_CHARTYPE* stb_text_createundo(StbUndoState* state, int pos, int insert_len, int delete_len)
{
	StbUndoRecord* r = stb_text_create_undo_record(state, insert_len);
	if (r == NULL)
		return NULL;

	r->where = pos;
	r->insert_length = (STB_TEXTEDIT_POSITIONTYPE)insert_len;
	r->delete_length = (STB_TEXTEDIT_POSITIONTYPE)delete_len;

	if (insert_len == 0) {
		r->char_storage = -1;
		return NULL;
	}
	else {
		r->char_storage = state->undo_char_point;
		state->undo_char_point += insert_len;
		return &state->undo_char[r->char_storage];
	}
}

static void stb_text_undo(STB_TEXTEDIT_STRING* str, STB_TexteditState* state)
{
	StbUndoState* s = &state->undostate;
	StbUndoRecord u, * r;
	if (s->undo_point == 0)
		return;

	// we need to do two things: apply the undo record, and create a redo record
	u = s->undo_rec[s->undo_point - 1];
	r = &s->undo_rec[s->redo_point - 1];
	r->char_storage = -1;

	r->insert_length = u.delete_length;
	r->delete_length = u.insert_length;
	r->where = u.where;

	if (u.delete_length) {
		// if the undo record says to delete characters, then the redo record will
		// need to re-insert the characters that get deleted, so we need to store
		// them.

		// there are three cases:
		//    there's enough room to store the characters
		//    characters stored for *redoing* don't leave room for redo
		//    characters stored for *undoing* don't leave room for redo
		// if the last is true, we have to bail

		if (s->undo_char_point + u.delete_length >= STB_TEXTEDIT_UNDOCHARCOUNT) {
			// the undo records take up too much character space; there's no space to store the redo characters
			r->insert_length = 0;
		}
		else {
			int i;

			// there's definitely room to store the characters eventually
			while (s->undo_char_point + u.delete_length > s->redo_char_point) {
				// should never happen:
				if (s->redo_point == STB_TEXTEDIT_UNDOSTATECOUNT)
					return;
				// there's currently not enough room, so discard a redo record
				stb_textedit_discard_redo(s);
			}
			r = &s->undo_rec[s->redo_point - 1];

			r->char_storage = s->redo_char_point - u.delete_length;
			s->redo_char_point = s->redo_char_point - u.delete_length;

			// now save the characters
			for (i = 0; i < u.delete_length; ++i)
				s->undo_char[r->char_storage + i] = STB_TEXTEDIT_GETCHAR(str, u.where + i);
		}

		// now we can carry out the deletion
		STB_TEXTEDIT_DELETECHARS(str, u.where, u.delete_length);
	}

	// check type of recorded action:
	if (u.insert_length) {
		// easy case: was a deletion, so we need to insert n characters
		u.insert_length = STB_TEXTEDIT_INSERTCHARS(str, u.where, &s->undo_char[u.char_storage], u.insert_length);
		s->undo_char_point -= u.insert_length;
	}

	state->cursor = u.where + u.insert_length;

	s->undo_point--;
	s->redo_point--;
}

static void stb_text_redo(STB_TEXTEDIT_STRING* str, STB_TexteditState* state)
{
	StbUndoState* s = &state->undostate;
	StbUndoRecord* u, r;
	if (s->redo_point == STB_TEXTEDIT_UNDOSTATECOUNT)
		return;

	// we need to do two things: apply the redo record, and create an undo record
	u = &s->undo_rec[s->undo_point];
	r = s->undo_rec[s->redo_point];

	// we KNOW there must be room for the undo record, because the redo record
	// was derived from an undo record

	u->delete_length = r.insert_length;
	u->insert_length = r.delete_length;
	u->where = r.where;
	u->char_storage = -1;

	if (r.delete_length) {
		// the redo record requires us to delete characters, so the undo record
		// needs to store the characters

		if (s->undo_char_point + u->insert_length > s->redo_char_point) {
			u->insert_length = 0;
			u->delete_length = 0;
		}
		else {
			int i;
			u->char_storage = s->undo_char_point;
			s->undo_char_point = s->undo_char_point + u->insert_length;

			// now save the characters
			for (i = 0; i < u->insert_length; ++i)
				s->undo_char[u->char_storage + i] = STB_TEXTEDIT_GETCHAR(str, u->where + i);
		}

		STB_TEXTEDIT_DELETECHARS(str, r.where, r.delete_length);
	}

	if (r.insert_length) {
		// easy case: need to insert n characters
		r.insert_length = STB_TEXTEDIT_INSERTCHARS(str, r.where, &s->undo_char[r.char_storage], r.insert_length);
		s->redo_char_point += r.insert_length;
	}

	state->cursor = r.where + r.insert_length;

	s->undo_point++;
	s->redo_point++;
}

static void stb_text_makeundo_insert(STB_TexteditState* state, int where, int length)
{
	stb_text_createundo(&state->undostate, where, 0, length);
}

static void stb_text_makeundo_delete(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int length)
{
	int i;
	STB_TEXTEDIT_CHARTYPE* p = stb_text_createundo(&state->undostate, where, length, 0);
	if (p) {
		for (i = 0; i < length; ++i)
			p[i] = STB_TEXTEDIT_GETCHAR(str, where + i);
	}
}

static void stb_text_makeundo_replace(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int old_length, int new_length)
{
	int i;
	STB_TEXTEDIT_CHARTYPE* p = stb_text_createundo(&state->undostate, where, old_length, new_length);
	if (p) {
		for (i = 0; i < old_length; ++i)
			p[i] = STB_TEXTEDIT_GETCHAR(str, where + i);
	}
}

// reset the state to default
static void stb_textedit_clear_state(STB_TexteditState* state, int is_single_line)
{
	state->undostate.undo_point = 0;
	state->undostate.undo_char_point = 0;
	state->undostate.redo_point = STB_TEXTEDIT_UNDOSTATECOUNT;
	state->undostate.redo_char_point = STB_TEXTEDIT_UNDOCHARCOUNT;
	state->select_end = state->select_start = 0;
	state->cursor = 0;
	state->has_preferred_x = 0;
	state->preferred_x = 0;
	state->cursor_at_end_of_line = 0;
	state->initialized = 1;
	state->single_line = (bool)is_single_line;
	state->insert_mode = 0;
	state->row_count_per_page = 0;
}

// API initialize
static void stb_textedit_initialize_state(STB_TexteditState* state, int is_single_line)
{
	stb_textedit_clear_state(state, is_single_line);
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif

static int stb_textedit_paste(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, STB_TEXTEDIT_CHARTYPE const* ctext, int len)
{
	return stb_textedit_paste_internal(str, state, (STB_TEXTEDIT_CHARTYPE*)ctext, len);
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif





// traverse the layout to locate the nearest character to a display position
int stb_text_locate_coord(STB_TEXTEDIT_STRING* str, float x, float y, int* out_side_on_line);
// API click: on mouse down, move the cursor to the clicked location, and reset the selection
void stb_textedit_click(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, float x, float y);
// API drag: on mouse drag, move the cursor and selection endpoint to the clicked location
void stb_textedit_drag(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, float x, float y);
// forward declarations
void stb_text_undo(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
void stb_text_redo(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
void stb_text_makeundo_delete(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int length);
void stb_text_makeundo_insert(STB_TexteditState* state, int where, int length);
void stb_text_makeundo_replace(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int old_length, int new_length);

void stb_textedit_find_charpos(StbFindState* find, STB_TEXTEDIT_STRING* str, int n, int single_line);
// make the selection/cursor state valid if client altered the string
void stb_textedit_clamp(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
// delete characters while updating undo
void stb_textedit_delete(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int len);
// delete the section
void stb_textedit_delete_selection(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
// canoncialize the selection so start <= end
void stb_textedit_sortselection(STB_TexteditState* state);
// move cursor to first character of selection
void stb_textedit_move_to_first(STB_TexteditState* state);
// move cursor to last character of selection
void stb_textedit_move_to_last(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
#ifndef STB_TEXTEDIT_MOVELINESTART
int stb_textedit_move_line_start(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int cursor);
#endif
#ifndef STB_TEXTEDIT_MOVELINEEND
int stb_textedit_move_line_end(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int cursor);
#endif
int is_word_boundary(STB_TEXTEDIT_STRING* str, int idx);
#ifndef STB_TEXTEDIT_MOVEWORDLEFT
int stb_textedit_move_to_word_previous(STB_TEXTEDIT_STRING* str, int c);
#endif
#ifndef STB_TEXTEDIT_MOVEWORDRIGHT
int stb_textedit_move_to_word_next(STB_TEXTEDIT_STRING* str, int c);
#endif
// update selection and cursor to match each other
void stb_textedit_prep_selection_at_cursor(STB_TexteditState* state);
// API cut: delete selection
int stb_textedit_cut(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
// API paste: replace existing selection with passed-in text
int stb_textedit_paste_internal(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, STB_TEXTEDIT_CHARTYPE* text, int len);
// API key: process text input
void stb_textedit_text(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, const STB_TEXTEDIT_CHARTYPE* text, int text_len);
// API key: process a keyboard input
void stb_textedit_key(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, STB_TEXTEDIT_KEYTYPE key);
void stb_textedit_flush_redo(StbUndoState* state);
void stb_textedit_discard_undo(StbUndoState* state);
void stb_textedit_discard_redo(StbUndoState* state);
StbUndoRecord* stb_text_create_undo_record(StbUndoState* state, int numchars);
STB_TEXTEDIT_CHARTYPE* stb_text_createundo(StbUndoState* state, int pos, int insert_len, int delete_len);
// reset the state to default
void stb_textedit_clear_state(STB_TexteditState* state, int is_single_line);
// API initialize
void stb_textedit_initialize_state(STB_TexteditState* state, int is_single_line);
int stb_textedit_paste(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, STB_TEXTEDIT_CHARTYPE const* ctext, int len);



void testedit() {

	stb_textedit_click(0, 0, 0, 0);
	stb_textedit_drag(0, 0, 0, 0);
	stb_textedit_cut(0, 0);
	stb_textedit_key(0, 0, 0);
	stb_textedit_initialize_state(0, 0);
	stb_textedit_paste(0, 0, 0, 0);
}

#endif // 1

edit_cx::edit_cx() :widget_t(WIDGET_TYPE::WT_EDIT)
{
	ctx = new text_control();
	set_single(true);
	on_event_cb = [=](uint32_t type, et_un_t* e, const glm::vec2& pos)
		{
			on_event_e(type, e);
		};
}

edit_cx::~edit_cx()
{
	if (ctx)delete ctx; ctx = nullptr;
}

void edit_cx::set_single(bool is)
{
	stb_textedit_initialize_state(&ctx->state, is);
}

void edit_cx::set_pwd(char ch)
{
	pwdch = ch;
	//if (ch)
	//	md::unicode_to_utf8(pwdch, ch);
}

void edit_cx::set_text(const void* str, int len)
{
	stb_textedit_delete(ctx, &ctx->state, 0, ctx->str.size());
	stb_textedit_text(ctx, &ctx->state, (char*)str, len);
	if (pwdch) {
		stext.resize(ctx->str.size());
		for (auto& it : stext) {
			it = pwdch;
		}
	}
	up_text = true;
	up_cursor(true);
}

void edit_cx::add_text(const void* str, int len)
{
	stb_textedit_text(ctx, &ctx->state, (char*)str, len);
	if (pwdch) {
		stext.resize(ctx->str.size());
		for (auto& it : stext) {
			it = pwdch;
		}
	}
	up_text = true;
	up_cursor(true);
}

void edit_cx::set_size(const glm::ivec2& ss)
{
	widget_t::set_size(ss);
}

void edit_cx::set_pos(const glm::ivec2& pos)
{
	widget_t::set_pos(pos);
}

void edit_cx::set_align_pos(const glm::vec2& pos)
{}

void edit_cx::set_align(const glm::vec2& a)
{}

void edit_cx::set_cursor(const glm::ivec3& c)
{
	_cursor = c;
}

void edit_cx::set_color(const glm::ivec4& c)
{
	_color = c;
}

void edit_cx::set_family(font_family_t* family, int fontsize)
{
	widget_t::set_family(family, fontsize);
	ctx->family = family;
	ctx->font_size = fontsize;
}

void edit_cx::set_show_input_cursor(bool ab)
{
	show_input_cursor = ab;
}

void edit_cx::set_autobr(bool ab)
{}

void edit_cx::set_round_path(float v)
{}

void edit_cx::remove_char(size_t idx, int count)
{}

bool edit_cx::remove_bounds()
{
	return false;
}


void edit_cx::on_event_e(uint32_t type, et_un_t* ep)
{
	auto e = ep->v;
	auto t = (devent_type_e)type;
	switch (t) {
	case devent_type_e::text_input_e:
	{
		auto p = e.t;
		auto ipos = input_pos();// 计算输入法坐标
		p->x = ipos.x;
		p->y = ipos.y + 3;
		p->w = ipos.z;
		p->h = ipos.w;
		add_text(p->text, strlen(p->text));
		wstr = md::u8_w(ctx->str.c_str(), ctx->str.size());
	}break;
	case devent_type_e::text_editing_e: {

		if (!is_input)break;
		auto& p = e.e;
		bool setimepos = false;
		if (ctx->caret_old != ctx->state.cursor)
		{
			ctx->caret_old = ctx->state.cursor;
			setimepos = true;
		}
		std::string str;
		if (p->text) {
			str = p->text;
		}
		if (str.size())
		{
			setimepos = true;
		}
		if (setimepos)
		{
			auto ipos = input_pos();// 计算输入法坐标
			p->x = ipos.x;
			p->y = ipos.y;
			p->w = ipos.z;
			p->h = ipos.w;
			//printf("text_editing_e:%s\t%d\n", str.c_str(), ipos.x);
		}
		//if (str.size())
		editingstr = str;
	}break;
	case devent_type_e::keyboard_e:
	{
		on_keyboard(ep);
	}break;
	default:
		break;
	};

}

input_state_t* get_input_state_cx(void* ptr, int t)
{
	static input_state_t r = {};
	if (t)
	{
		if (r.ptr)
		{
			auto p = (edit_cx*)r.ptr;
			p->editingstr.clear();
			p->is_input = false;
		}
		r.ptr = ptr;
	}
	if (ptr && r.ptr)
	{
		auto p = (edit_cx*)r.ptr;
		if (p) {
			*((glm::ivec4*)&r.x) = p->input_pos();
			r.y += 3;
		}
		if (!r.cb)
			r.cb = [](uint32_t type, et_un_t* e, void* ud) { if (ud) { ((edit_cx*)ud)->on_event_e(type, e); }	};
	}
	return &r;
}
bool edit_cx::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto t = (event_type2)type;
	bool ret = false;
	auto p = (mouse_move_et*)e;

	auto mpos = mps;
	glm::vec2 tpos = ctx->_align_pos - ctx->scroll_pos;
	tpos += thickness;

	switch (t)
	{
	case event_type2::on_move:
	{
		p->cursor = (int)cursor_st::cursor_ibeam;//设置输入光标
	}break;
	case event_type2::on_leave:
	{
		p->cursor = (int)cursor_st::cursor_arrow;//设置光标
	}break;
	case event_type2::on_down:
	{
		mpos -= tpos + _pos;
		_cmpos = mpos;
		if (form)
		{
			form_set_input_ptr(form, get_input_state_cx(this, 1));
			ctx->c_d = -1; is_input = true;
		}
		else {
			ctx->c_d = 0; is_input = false;
		}
		stb_textedit_click(ctx, &ctx->state, mpos.x, mpos.y);
		ret = true;
	}break;
	case event_type2::on_drag:
	{
		mpos += curpos; mpos -= tpos + _pos;
		glm::ivec2 cm = mpos;
		if (_cmpos != cm)
			_cmpos = mpos;
		stb_textedit_drag(ctx, &ctx->state, mpos.x, mpos.y);
		ret = true;
	}break;
	case event_type2::mouse_wheel:
	{

	}break;
	default:
		break;
	};
	if (ret) {
		up_cursor(true);
	}
	return ret;
}

void edit_cx::on_keyboard(et_un_t* ep)
{
	auto p = ep->v.k;
	const bool is_osx = false;
	// Control=1 Shift=2 Alt=4 8Cmd/Super/Windows
	bool KeyShift = p->kmod == 2;
	bool KeyAlt = p->kmod == 4;
	bool KeyCtrl = p->kmod == 1;
	bool KeySuper = p->kmod == 8;
	const int k_mask = (KeyShift ? STB_TEXTEDIT_K_SHIFT : 0);
	const bool is_wordmove_key_down = is_osx ? KeyAlt : KeyCtrl;                     // OS X style: Text editing cursor movement using Alt instead of Ctrl
	const bool is_startend_key_down = is_osx && KeyCtrl && !KeySuper && !KeyAlt;  // OS X style: Line/Text Start and End using Cmd+Arrows instead of Home/End

	int key = 0;


	if (!p->down)
	{
		do {
			if (!KeyCtrl)break;
			switch (p->keycode) {
			case SDLK_A:
			{
				ctx->state.select_end = ctx->str.size();
				ctx->state.select_start = 0;
			}
			break;
			case SDLK_X:
			case SDLK_C:
			{
				std::string str;
				stb_textedit_sortselection(&ctx->state);
				str.assign(ctx->str.c_str() + ctx->state.select_start, ctx->state.select_end - ctx->state.select_start);
				set_clipboard(str.c_str());
				if (p->keycode == SDLK_X)
				{
					stb_textedit_cut(ctx, &ctx->state);
					up_text = true;
				}
			}
			break;
			case SDLK_V:
			{
				auto str = get_clipboard();
				stb_textedit_paste(ctx, &ctx->state, str.c_str(), str.size());
				up_text = true;
			}
			break;
			case SDLK_Y:
				key = STB_TEXTEDIT_K_REDO;
				up_text = true;
				//is_redo = true;	//_storage_buf->redo();
				break;
			case SDLK_Z:
				key = STB_TEXTEDIT_K_UNDO;
				up_text = true;
				//is_undo = true;	//_storage_buf->undo();
				break;
			}
		} while (0);
	}
	if (!key && (!p->down || editingstr.size()))
	{
		return;
	}

	bool isupcursor = false;
	switch (p->keycode)
	{
	case SDLK_PRINTSCREEN:
	{}
	break;
	case SDLK_SCROLLLOCK:
	{}
	break;
	case SDLK_PAUSE:
	{}
	break;
	case SDLK_TAB:
	{
		add_text("\t", 1);
	}
	break;
	case SDLK_BACKSPACE:
	{
		key = STB_TEXTEDIT_K_BACKSPACE;
		up_text = true;
	}
	break;
	case SDLK_INSERT:
	{
		key = STB_TEXTEDIT_K_INSERT;
		up_text = true;
	}
	break;
	case SDLK_PAGEDOWN:
	{
		key = STB_TEXTEDIT_K_PGDOWN;
	}
	break;
	case SDLK_PAGEUP:
	{
		key = STB_TEXTEDIT_K_PGUP;
	}
	break;
	case SDLK_DELETE:
	{
		key = STB_TEXTEDIT_K_DELETE;
	}
	break;
	case SDLK_HOME:
	{
		key = STB_TEXTEDIT_K_TEXTSTART;
	}
	break;
	case SDLK_END:
	{
		key = STB_TEXTEDIT_K_TEXTEND;
	}
	break;
	case SDLK_RIGHT:
	{
		key = STB_TEXTEDIT_K_RIGHT;
	}
	break;
	case SDLK_LEFT:
	{
		key = STB_TEXTEDIT_K_LEFT;
	}
	break;
	case SDLK_DOWN:
	{
		key = STB_TEXTEDIT_K_DOWN;
	}
	break;
	case SDLK_UP:
	{
		key = STB_TEXTEDIT_K_UP;
	}
	break;
	case SDLK_RETURN:
	{
		remove_bounds();
		if (!ctx->state.single_line)
		{
			add_text("\n", 1);
		}
	}
	break;
	default:
		break;
	}
	if (key)
	{
		/*STB_TEXTEDIT_K_SHIFT
		 STB_TEXTEDIT_K_CONTROL
		 STB_TEXTEDIT_K_LEFT
		 STB_TEXTEDIT_K_RIGHT
		 STB_TEXTEDIT_K_UP
		 STB_TEXTEDIT_K_DOWN
		 STB_TEXTEDIT_K_LINESTART
		 STB_TEXTEDIT_K_LINEEND
		 STB_TEXTEDIT_K_TEXTSTART
		 STB_TEXTEDIT_K_TEXTEND
		 STB_TEXTEDIT_K_DELETE
		 STB_TEXTEDIT_K_BACKSPACE
		 STB_TEXTEDIT_K_UNDO
		 STB_TEXTEDIT_K_REDO
		 STB_TEXTEDIT_K_INSERT
		 STB_TEXTEDIT_K_WORDLEFT
		  STB_TEXTEDIT_K_WORDRIGHT
		  STB_TEXTEDIT_K_PGUP
		  STB_TEXTEDIT_K_PGDOWN */
		if (KeyCtrl)
		{
			key |= STB_TEXTEDIT_K_CONTROL;
		}
		if (KeyShift)
		{
			key |= STB_TEXTEDIT_K_SHIFT;
		}
		stb_textedit_key(ctx, &ctx->state, key);
		up_cursor(true);
	}
}

bool edit_cx::update(float delta)
{
	ctx->c_ct += delta * 1000.0 * ctx->c_d;
	if (ctx->c_ct > _cursor.z)
	{
		ctx->c_d = -1;
		ctx->c_ct = _cursor.z;
		valid = true;
	}
	if (ctx->c_ct < 0)
	{
		ctx->c_d = 1; ctx->c_ct = 0;
		valid = true;
	}
	if (ctx->lineheight < 1)
		ctx->lineheight = font_get_lineheight(family, font_size);

	if (up_text)
	{
		if (ctx->state.single_line)
		{
			glm::vec2 ext = { 0,ctx->lineheight }, ext1 = { 0,font_size }, ss = _size;
			ss -= thickness * 2;
			auto ps = glm::ceil((ss - ext) * text_align);
			auto ps1 = glm::ceil((ss - ext1) * text_align);
			ctx->_align_pos.y = ps.y;
			ctx->_align_pos1.y = ps1.y;
		}
		up_text = false;
		ctx->widths.clear();
	}
	return false;
}

void edit_cx::draw(rvg_cx* rv)
{
	glm::ivec2 nposs = _pos;
	glm::ivec2 ss = _size;
	auto psv = get_pos(false);
	auto vsize = get_size();
	vsize += thickness * 2;
	auto view = glm::ivec4(psv, vsize);
	rv->push_view(view);
	rv->save();
	rv->translate(psv);
	rv->add_rect({ 0,0,ss.x ,ss.y }, rounding);
	rv->set_color(_color.x);
	rv->fill();
	psv += thickness;
	rv->translate({ thickness ,thickness });
	auto tpos = ctx->_align_pos - ctx->scroll_pos;
	auto tpos1 = ctx->_align_pos1 - ctx->scroll_pos;

	glm::vec2 ext = { 0,ctx->lineheight }, ext1 = { 0,font_size }, iss = _size;
	iss -= thickness * 2;
	auto ps = glm::ceil((iss - ext) * text_align);
	auto ps1 = glm::ceil((iss - ext1) * text_align);
	glm::ivec2 npos = {};
	glm::ivec2 srcpos = {};
	if (ctx->state.single_line)
	{
		npos.y += ps1.y;
		srcpos.y += ps.y;
	}
	{

		rv->save();
		// 裁剪区域渲染选中效果背景
		rv->add_rect({ 0,0,ss.x - thickness * 2,ss.y - thickness * 2 }, 0);
		rv->clip();
		tpos1 = tpos;
		tpos1.y += ceil((ctx->lineheight - font_size) * text_align.y);
		auto v = get_bounds();
		rv->translate({ 0,1 });
		if (v.x != v.y && ctx->rangerc.size()) {
			rv->save();
			rv->translate(tpos);
			rv->set_color(_color.z);
			if (roundselect)
			{
				if (ctx->range_path.size() && ctx->range_path[0].size() > 3) {
					rv->add_polyline(&ctx->range_path, true);
					rv->fill();
				}
			}
			else {
				for (auto& it : ctx->rangerc)
				{
					rv->add_rect(it, std::min(it.z, it.w) * 0.18);
					rv->fill();
				}
			}
			rv->restore();
		}

		// 渲染文本
		glm::vec4 rc = { 0,0,  ss };
		text_style st = {};
		st.fontsize = font_size;
		st.align = {};
		st.color = text_color;
		st.family = family;
		text_st tx = {};
		tx.pos = { tpos1 };
		tx.size = ss;
		if (pwdch)
		{
			tx.text = stext.c_str(); tx.text_len = stext.size();
		}
		else {
			tx.text = ctx->str.c_str();
			tx.text_len = ctx->str.size();
		}
		rv->add_text(&tx, &st);
		rv->restore();
	}
	rv->restore();
	rv->pop_view();

	glm::ivec2 cpos = ctx->cursor_pos;
	cpos += tpos + psv;// +(glm::ivec2)rv->_cur.pos;
	bool ccd = (show_input_cursor && ctx->c_d == 1 && _cursor.x > 0 && ctx->cursor_pos.z > 0);
	if (ccd)
	{
		auto rpos = cpos;
		rv->push_view({ rpos ,align_up(_cursor.x,4),align_up(ctx->cursor_pos.z,4) });
		rv->set_color(_cursor.y);
		rv->add_rect({ cpos.x,cpos.y, _cursor.x, ctx->cursor_pos.z }, 0);
		rv->fill();
		rv->pop_view();
	}
	// 编辑中的文本
	if (is_input)
	{
		auto epos = glm::ivec2(cpos);
		uint32_t editing_color = _color.w;
		if (editingstr.size())
		{
			// 渲染文本 
			text_style st = {};
			st.fontsize = font_size;
			st.align = {};
			st.color = editing_color;
			st.family = family;
			glm::ivec2 lps = get_text_rect(family, font_size, editingstr.c_str(), editingstr.size(), 0);

			if (lps.y < ctx->lineheight)
				lps.y = ctx->lineheight;
			glm::vec4 lss = { 0,  lps.y + 0.5, lps.x, lps.y + 0.5 };
			glm::vec4 rc = { 1,1,lps.x + 2, lps.y + 2 };
			auto clip0 = glm::ivec4(epos, rc.z, rc.w);

			rv->push_view(clip0);
			rv->save();
			rv->translate({ clip0.x, clip0.y });
			rv->set_color(get_reverse_color(editing_color));
			rv->add_rect({ 0, 0, lps.x + 2, lps.y + 2 }, 0);
			rv->fill();
			rv->set_color(editing_color);
			rv->add_line({ lss.x + 1, lss.y }, { lss.z, lss.w });
			rv->set_line_width(1);
			rv->stroke();

			text_st tx = {};
			tx.pos = { rc.x,rc.y };
			tx.size = glm::ivec2(rc.z, rc.w);
			tx.text = editingstr.c_str(); tx.text_len = editingstr.size();
			rv->add_text(&tx, &st);

			rv->restore();
			rv->pop_view();
		}
	}
}

glm::ivec4 edit_cx::input_pos()
{
	glm::ivec2 cpos = ctx->cursor_pos;
	auto pos = get_pos();
	return { pos + cpos - ctx->scroll_pos + ctx->_align_pos
		,2,  ctx->cursor_pos.z + 2 };
}

int edit_cx::get_cursor_idx()
{
	return ctx->state.cursor;
}

std::string edit_cx::get_select_str()
{
	return std::string();
}

std::wstring edit_cx::get_select_wstr()
{
	return std::wstring();
}

glm::ivec2 edit_cx::get_bounds()
{
	glm::ivec2 v = { ctx->state.select_start, ctx->state.select_end };
	if (v.x > v.y) { std::swap(v.x, v.y); }
	return v;
}
glm::ivec3 edit_cx::get_line_length(int index)
{
	int x_pos = 0;
	int lidx = 0;
	int cu = -1;
	int cx = 0;
	auto sp = ctx->str.c_str();
	for (size_t i = 0; i < ctx->lvs.size(); i++)
	{
		auto c = ctx->lvs[i];
		if (index >= c.x && index < c.x + c.y + 1)//因为有换行+1
		{
			x_pos = index - c.x;
			lidx = c.x;
			cu = i;// 行号
			break;
		}
	}
	if (cu < 0 && ctx->lvs.size()) {
		cu = ctx->lvs.size() - 1;
		auto c = ctx->lvs[cu];
		x_pos = index - c.x;
		lidx = c.x;
	}
	glm::ivec3 ret = { lidx,cu, x_pos };
	return ret;
}
// 获取范围的像素大小
std::vector<glm::ivec4> edit_cx::get_bounds_px()
{
	float pwidth = font_size * 0.5;// 补行尾宽度
	auto pstr = ctx->str.c_str();
	auto tsize = ctx->str.size();
	if (up_text || ctx->widths.empty())
	{
		up_text = false;
		ctx->widths.clear();
		ctx->lvs.clear();
		auto length = pwdch ? stext.size() : ctx->str.size();
		auto p = pwdch ? stext.c_str() : ctx->str.c_str();
		size_t f = 0, s = 0; size_t i = 0;
		for (; i < length; i++)
		{
			if (p[i] == '\n')
			{
				ctx->lvs.push_back({ f,i - f });
				f = i + 1;
			}
		}
		ctx->lvs.push_back({ f,i - f });
		font_get_text_posv(family, font_size, pstr, tsize, ctx->widths);
		pwidth = font_get_text_rect1(family, font_size, "1").x;
	}
	std::vector<glm::ivec4> r;
	std::vector<glm::ivec4> rs, rss;
	auto v = get_bounds();
	if (v.x == v.y) { ctx->rangerc = rss; return rs; }
	if ((v.x >= tsize || v.y > tsize)) {
		ctx->rangerc = rss; return rs;
	}
	auto v1 = get_line_length(v.x);
	auto v2 = get_line_length(v.y);
	auto line_no = ctx->lvs.size();
	auto h = fix_line_height > 0 ? fix_line_height : font_get_lineheight(family, font_size);
	// 计算选中范围的每行的坐标宽高
	if (v1 == v2) {}
	else {
		if (v1.y == v2.y)
		{
			auto ks = ctx->lvs[v1.y];
			auto w1 = ctx->widths[v1.y];
			auto xc = char2pos(v.x - ks.x, pstr + ks.x);
			auto yc = char2pos(v.y - ks.x, pstr + ks.x);
			int w = w1[xc];
			int ww = w1[yc] - w;
			rss.push_back({ w ,v1.y * h,ww,h });// 同一行时
		}
		else {
			auto ks = ctx->lvs[v1.y];
			auto w1 = ctx->widths[v1.y];
			auto w = w1[char2pos(v.x - ks.x, pstr + ks.x)];
			auto wd = *w1.rbegin() - w;
			rss.push_back({ w,v1.y * h,wd + pwidth,h });// 第一行
		}
		for (int i = v1.y + 1; i < line_no && i < v2.y; i++)
		{
			auto ks = ctx->lvs[i];
			auto w1 = ctx->widths[i];
			rss.push_back({ 0,i * h,*w1.rbegin() + pwidth, h });// 中间全行
		}
		if (v1.y < v2.y)
		{
			auto ks = ctx->lvs[v2.y];
			auto w1 = ctx->widths[v2.y];
			rss.push_back({ 0,v2.y * h,w1[char2pos(v.y - ks.x ,pstr + ks.x)],h });//最后一行
		}
	}
	if (roundselect)
	{
		PathsD subjects;
		PathD a;
		int py = ctx->_r_posy;
		for (size_t i = 0; i < rss.size(); i++)
		{
			auto& it = rss[i];
			if (it.z < 1)
				it.z = pwidth;
			a.push_back({ it.x,it.y + py });
			a.push_back({ it.x + it.z,it.y + py });
			a.push_back({ it.x + it.z,it.y + it.w + py });
			a.push_back({ it.x,it.y + it.w + py });
			subjects.push_back(a);
			a.clear();
		}
		subjects = Union(subjects, FillRule::NonZero, 6);
		//range_path = InflatePaths(subjects, 0.5, JoinType::Round, EndType::Polygon);
		auto sn = subjects.size();
		if (sn > 0)
		{
			path_v& ptr = ctx->ptr_path;
			ptr._data.clear();
			for (size_t i = 0; i < sn; i++)
			{
				auto& it = subjects[i];
				ptr.add_lines((glm::dvec2*)it.data(), it.size(), false);
			}
			ctx->range_path = gp::path_round(&ptr, -1, font_size * ctx->round_path, 16, 0, 0);
		}
		else { ctx->range_path.clear(); }
	}
	ctx->rangerc = rss;
	return r;
}

glm::ivec2 edit_cx::get_pixel_size(const char* str, int len)
{
	int w = 0, h = 0;
	if (str && *str)
	{
		auto rc = get_text_rect(family, font_size, str, len, 0);
		w = rc.x; h = rc.y;
	}
	return glm::ivec2(w, h);
}
size_t edit_cx::get_xy_to_index(int x, int y, const char* str)
{
	auto pstr = ctx->str.c_str();
	if (ctx->widths.empty())
	{
		font_get_text_posv(family, font_size, pstr, ctx->str.size(), ctx->widths);
	}
	if (ctx->widths.size() != ctx->lvs.size())
		return -1;
	x += ctx->scroll_pos.x - ctx->_align_pos.x;
	y += ctx->scroll_pos.y - ctx->_align_pos.y;
	int index = 0, trailing = 0;
	if (x < 0)
	{
		x = 0;
	}
	if (y < 0)
	{
		y = 0;
	}
	glm::ivec2 lps = get_pixel_size(pstr, text.size());
	if (y > lps.y)
		y = lps.y - 1;
	y /= ctx->lineheight;
	if (y >= ctx->lvs.size())y = ctx->lvs.size() - 1;
	auto ky = ctx->lvs[y];
	auto& ws = ctx->widths[y];
	int cw = 0;
	for (size_t i = 0; i < ws.size(); i++)
	{
		if (x < ws[i]) {
			index = i;  break;
		}
	}
	if (x > *(ws.rbegin())) {
		index = ws.size() - 1;
	}
	else if (index > 0) {
		int tr = (ws[index] - ws[index - 1]) * 0.5;
		int xx = x - ws[index - 1];
		if (xx >= tr)
		{
			cw = ws[index];
			index++;
		}
		else {
			cw = ws[index - 1];
		}
		index--;
	}
	auto newx = md::utf8_char_pos(str + ky.x, index, -1);
	newx -= (uint64_t)str;
	index = (uint64_t)newx;
	//curx = cw;
	return (size_t)index;
}

void edit_cx::up_caret()
{
	glm::ivec4 caret = {};
	get_bounds_px();
	auto v1 = get_line_length(ctx->state.cursor);
	auto line_no = ctx->lvs.size();
	auto h = fix_line_height > 0 ? fix_line_height : font_get_lineheight(family, font_size);
	// 计算选中范围的每行的坐标宽高 
	if (line_no > 0 && ctx->widths.size() > v1.y)
	{
		auto ks = ctx->lvs[v1.y];
		auto w1 = ctx->widths[v1.y];
		{
			auto pstr = ctx->str.c_str();
			caret.x = get_text_rect(family, font_size, pstr + ks.x, std::min(ks.y, ctx->state.cursor - ks.x), 0).x;
		}
		//caret.x = w1[ctx->state.cursor - ks.x];
		caret.y = ctx->cursor_pos.z * v1.y;
		//printf("cursor:\t%d\n", cursor_pos.x);
	}
	ctx->cursor_pos = caret; ctx->cursor_pos.z = h;
}

void edit_cx::up_cursor(bool is)
{
	if (is)
	{
		up_caret();
		glm::ivec2 cs = ctx->cursor_pos;
		auto evs = get_size();		// 视图大小
		auto h = ctx->cursor_pos.z;	// 行高
		if (h < 1)h = 1;
		evs.x -= ctx->_align_pos.x;
		int ey = evs.y - ctx->cursor_pos.z;
		//ey *= h;
		glm::ivec2 pos = {};
		if (ctx->is_scroll) {
			dcscroll(cs.x, evs.x, 2, ctx->scroll_pos.x);
			dcscroll(cs.y, ey, h, ctx->scroll_pos.y);
		}
		else
		{
			ctx->scroll_pos = { .0, .0 };
		}
		if (!(cs.x < 0 || cs.y < 0))
		{
			pos.x += cs.x;
			pos.y += cs.y;
		}
	}
}