
#include "pch1.h"
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
	//#include <pango/pango-layout.h>
	//#include <pango/pangocairo.h>

#ifdef __cplusplus
}
#endif
#endif

#include <SDL3/SDL_keycode.h>
#include <mapView.h>
#include <event.h>

#include "pnguo.h"
#include "gui.h" 


widget_base::widget_base()
{
}
widget_base::widget_base(WIDGET_TYPE wt) :wtype(wt)
{
}
widget_base::~widget_base()
{
}
bool widget_base::on_mevent(int type, const glm::vec2& mps)
{
	return false;
}

bool widget_base::update(float delta)
{
	return false;
}

void widget_base::draw(cairo_t* cr)
{
}

glm::ivec2 widget_base::get_pos(bool has_parent)
{
	glm::ivec2 ps = pos;
	if (parent) {
		auto pss = parent->get_pos();
		auto ss = parent->get_spos();
		ss *= hscroll;
		ps += ss;
		if (has_parent) { ps += pss; }
	}
	return ps;
}



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
	image_ptr_t cacheimg = {};
	cairo_surface_t* sur = 0;
	layout_text_x* ltx = 0;
	std::string text;			// 原文本
	std::string stext;			// 显示的文本
	std::string editingstr;

	int fontid = 0;
	int fontsize = 8;
	uint32_t back_color = 0x06001020;		//背景色
	uint32_t text_color = 0xffffffff;
	std::vector<uint32_t> dtimg;
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
	void draw(cairo_t* cr);

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
	if (sur)
	{
		cairo_surface_destroy(sur); sur = 0;
	}
}

void text_ctx_cx::set_autobr(bool is)
{
}
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
	if (ltx && str && *str)
	{
		auto rc = ltx->get_text_rect(fontid, str, len, fontsize);
		w = rc.x; h = rc.y;
	}
	return glm::ivec2(w, h);
}

int text_ctx_cx::get_baseline()
{
	return ltx ? ltx->get_baseline(fontid, fontsize) : 0;
	//glm::ceil((double)pango_layout_get_baseline(layout) / PANGO_SCALE);
}
int text_ctx_cx::get_lineheight()
{
	return ltx ? ltx->get_lineheight(fontid, fontsize) : 0;
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
	auto ltx = layout->ltx;
	auto strc = text + tmp_list->start_index;
	while (tmp_list)
	{
		auto cw = ltx->get_text_rect1(layout->fontid, layout->fontsize, strc);
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
	if (ltx && widths.empty())
	{
		ltx->get_text_posv(fontid, fontsize, pstr, text.size(), widths);
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
glm::ivec2 geti2x(PangoLayout* layout, int x)
{
	int x_pos = 0;
	int lidx = 0;
	//pango_layout_index_to_line_x(layout, x, 0, &lidx, &x_pos);
	//x_pos /= PANGO_SCALE;
	return glm::ivec2(x_pos, lidx);
}

glm::ivec4 text_ctx_cx::get_cursor_posv(int idx)
{
	glm::ivec4 r = { /*w1.x,w1.y,w1.width,w1.height*/ };
	return r;
}

std::vector<glm::ivec4> get_caret_posv(PangoLayout* layout, int idx)
{
	std::vector<glm::ivec4> rv;
	//glm::ivec4 r = { w0.x,w0.y,w0.width,w0.height };
	//glm::ivec4 r1 = { w1.x,w1.y,w1.width,w1.height };
	//rv.push_back(r);
	//rv.push_back(r1); 
	return rv;
}

size_t char2pos(size_t ps, const char* str) {
	return md::get_utf8_count(str, ps);
}
// todo 获取范围的像素大小
std::vector<glm::ivec4> text_ctx_cx::get_bounds_px()
{
	float pwidth = fontsize * 0.5;// 补行尾宽度
	if (ltx && widths.empty())
	{
		auto pstr = text.c_str();
		ltx->get_text_posv(fontid, fontsize, pstr, text.size(), widths);
		pwidth = ltx->get_text_rect1(fontid, fontsize, "1").x;
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
		if (ltx) {
			get_bounds_px();
			_baseline = get_baseline(); lineheight = get_lineheight();
			upft = false;
		}
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
		if (ltx) {
			auto pstr = text.c_str();
			caret.x = ltx->get_text_ipos(fontid, fontsize, pstr + ks.x, ks.y, ccursor - ks.x);
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

	cairo_as _ss_(cr);


	if (upft)
	{
		if (ltx) {
			get_bounds_px();
			_baseline = get_baseline(); lineheight = get_lineheight();
			upft = false;
		}
	}
	if (single_line) {
		glm::vec2 ext = { 0,lineheight }, ss = size;
		auto ps = ss * text_align - (ext * text_align);
		_align_pos.y = ps.y;
	}
	cairo_translate(cr, -scroll_pos.x + _align_pos.x, -scroll_pos.y + _align_pos.y);

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
				draw_rectangle(cr, it, std::min(it.z, it.w) * 0.18);
				cairo_fill(cr);
			}
		}
	}
	set_color(cr, text_color);
	auto lhh = get_pixel_size(stext.c_str(), stext.size());
	// 渲染文本
	glm::vec4 rc = { 0,0,  lhh };
	text_style_t st = {};
	st.font = 0;
	st.text_align = { 0.0,0.0 };
	st.font_size = fontsize;
	st.text_color = text_color;
	if (ltx)
		draw_text(cr, ltx, stext.c_str(), -1, rc, &st);

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
void text_ctx_cx::draw(cairo_t* cr)
{
	cairo_as _ss_(cr);
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
	auto bbc = box_color;
	set_source_rgba(cr, bbc);
	cairo_rectangle(cr, pos.x - 0.5, pos.y - 0.5, size.x + 1, size.y + 1);
	cairo_set_line_width(cr, 1);
	cairo_stroke(cr);
	// 编辑中的文本
	if (editingstr.size())
	{
		cairo_translate(cr, x, y);
		// 渲染文本
		text_style_t st = {};
		st.font = 0;
		st.text_align = { 0.0,0.0 };
		st.font_size = fontsize;
		st.text_color = editing_text_color;

		glm::ivec2 lps = {};
		lps = get_pixel_size(editingstr.c_str(), editingstr.size());
		if (lps.y < lineheight)
			lps.y = lineheight;
		glm::vec4 lss = { 0,  lps.y + 0.5, lps.x, lps.y + 0.5 };
		glm::vec4 rc = { 1,1,lps.x + 2, lps.y + 2 };

		set_color(cr, get_reverse_color(editing_text_color));
		cairo_rectangle(cr, 0, 0, lps.x + 2, lps.y + 2);
		cairo_fill(cr);
		set_color(cr, editing_text_color);
		if (ltx)
			draw_text(cr, ltx, editingstr.c_str(), -1, rc, &st);
		cairo_move_to(cr, lss.x + 1, lss.y);
		cairo_line_to(cr, lss.z, lss.w);
		cairo_set_line_width(cr, 1);
		cairo_stroke(cr);
	}
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

void renderer_draw_layout(cairo_t* cr, PangoLayout* layout, int x, int y, int baseline)
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
		cairo_translate(cr, x + logical_rect.x, y + baseline + yy.x);
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
	cairo_translate(cr, -scroll_pos.x + _align_pos.x, -scroll_pos.y + _align_pos.y);

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
				draw_rectangle(cr, it, std::min(it.z, it.w) * 0.18);
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
void text_ctx_cx::draw(cairo_t* cr)
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
		cairo_translate(cr, x, y);

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
	widget_base::wtype = WIDGET_TYPE::WT_EDIT;
	ctx = new text_ctx_cx();
	//set_family("NSimSun", 12);
	set_family(0, 12);
	set_align_pos({ 4, 4 });
	set_color({ 0xff353535,-1,0xa0ff8000 ,0xff020202 });
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
	size = ss;
	if (ss.x > 0 && ss.y > 0)
		ctx->set_size(ss);
}
void edit_tl::set_pos(const glm::ivec2& ps) {
	ctx->pos = pos = ps;
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
	if (!ctx->ltx) { ctx->ltx = ltx; ctx->get_bounds_px(); }
	if (!ltx)return;
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
					bool ok = parent && parent->dragdrop_begin ? parent->dragdrop_begin(ws.c_str(), ws.size()) : false;
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
						if (parent && parent->form_set_input_ptr) { parent->form_set_input_ptr(ep->form, get_input_state(this, 1)); };
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
			if (ep->form && parent && parent->form_set_input_ptr) { parent->form_set_input_ptr(ep->form, get_input_state(this, 1)); }
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
	if (!ctx->ltx)
	{
		ctx->ltx = ltx;
	}
	if (!is_input)
	{
		ctx->c_d = 0;
	}
	glm::ivec2 ss = size;
	if (ctx->size != ss) {
		ctx->set_size(ss);
	}
	glm::ivec2 ps = pos;
	if (ctx->pos != ps) {
		ctx->pos = ps;
	}
	return ctx->update(delta);
}
// 获取纹理/或者渲染到cairo
image_ptr_t* edit_tl::get_render_data() {
	return &ctx->cacheimg;
}
void edit_tl::draw(cairo_t* cr)
{
	ctx->draw(cr);
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

#ifndef NO_TVIEW

class tview_x
{
public:
	glm::ivec2 vpos = {}, size = {};	// 渲染偏移，大小 
	std::vector<glm::vec4> ddrect;
	// 填充颜色 
	uint32_t clear_color = 0;
	cairo_surface_t* _backing_store = 0;
	cairo_surface_t* rss = 0;
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
	cairo_t* begin_frame(bool redraw);
	void end_frame(cairo_t* cr);
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
	if (rss)
	{
		cairo_surface_destroy(rss);
	}
	rss = 0;
	if (_backing_store)
	{
		cairo_surface_destroy(_backing_store);
	}
	_backing_store = 0;
}
image_ptr_t* tview_x::get_ptr()
{
	return  &img_rc;
}

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
void tview_x::set_size(const glm::ivec2& ss)
{
	if (ss != size)
	{
		size = ss;
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
		if (rss)
		{
			cairo_surface_destroy(rss);
		}
		rss = new_clip_rect(r);
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


#if 1
void widget_on_event(widget_base* p, uint32_t type, et_un_t* e, const glm::vec2& pos);
void send_hover(widget_base* wp, const glm::vec2& mps);

plane_cx::plane_cx()
{
	tv = new tview_x();
	ltx = new layout_text_x();
	auto st = &vgs;
	st->dash.v = 0x05050505;
	st->dash_num = 4;
	st->thickness = 0;
	st->join = 1;
	st->cap = 1;
	st->fill = 0x80FF7373;
	st->color = 0xffffffff;
	st->round = 6;
	vgtms.y = 20;

	push_dragpos({ 0,0 });
}

plane_cx::~plane_cx()
{
	remove_widget(horizontal);
	remove_widget(vertical);
	if (horizontal)
		delete horizontal;
	horizontal = 0;
	if (vertical)
		delete vertical;
	vertical = 0;
	for (auto it : widgets) {
		if (it && it->_autofree)delete it;
	}
	widgets.clear();
	if (tv)
	{
		delete tv; tv = 0;
	}
	if (_pat)
	{
		delete _pat; _pat = 0;
	}
	if (ltx)
	{
		delete ltx; ltx = 0;
	}
}
void plane_cx::set_fontctx(font_rctx* p)
{
	if (ltx && p)
		ltx->set_ctx(p);
}
glm::ivec2 plane_cx::get_pos() {
	return glm::ivec2(viewport.x, viewport.y);
}
glm::ivec2 plane_cx::get_spos()
{
	glm::vec2 sps = {};

	if (horizontal)
		sps.x = -horizontal->get_offset();
	if (vertical)
		sps.y = -vertical->get_offset();
	return sps;
}
void plane_cx::set_pos(const glm::ivec2& ps) {
	tpos = ps;
	viewport.x = ps.x; viewport.y = ps.y;
}
void plane_cx::set_size(const glm::ivec2& ss) {
	if (ss.x > 0 && ss.y > 0)
	{
		viewport.z = ss.x; viewport.w = ss.y;
		_clip_rect = { 0,0,ss.x,ss.y };
		uplayout = true;
		me.size = ss;
		if (tv)
			tv->set_size(ss);
		else
			return;
		if (!_pat)
		{
			_pat = new atlas_t();
			add_atlas(_pat);
		}
		_pat->img = tv->get_ptr();
		_pat->img->valid = true;
		//static uint32_t cc = 0x8f0080ff;
		//_pat->colors = &cc;
		_pat->img_rc = &_clip_rect;
		_pat->tex_rc = (glm::ivec4*)&tv->vpos;
		_pat->tex_rc->x = 0; _pat->tex_rc->y = 0;
		_pat->count = 1;
		_pat->clip = _clip_rect;
	}
}
glm::vec2 plane_cx::get_size()
{
	return glm::vec2(tv->size);
}
void plane_cx::set_select_box(int w, float s)
{
	vgs.thickness = w;
	vgtms.x = s;
}
void plane_cx::add_mouse_cb(std::function<void(plane_ev* e)> cb)
{
	if (cb) {
		on_mouses.push_back(cb);
	}
}

size_t plane_cx::add_res(const std::string& fn)
{

	return 0;
}
size_t plane_cx::add_res(const char* data, int len)
{

	return 0;
}
//pos_width每次滚动量,垂直vnpos,水平hnpos为滚动条容器内偏移
void plane_cx::set_scroll(int width, int rcw, const glm::ivec2& pos_width, const glm::ivec2& vnpos, const glm::ivec2& hnpos)
{
	auto pss = get_size();
	{
		auto cp = add_scroll_bar({ width,pss.y - width * 2 }, pss.y, pss.y, rcw, true, vnpos);
		bind_scroll_bar(cp, true); // 绑定垂直滚动条
		cp->_pos_width = pos_width.y > 0 ? pos_width.y : width * 2;//滚轮事件每次滚动量
		cp->hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->has_hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->hscroll = {};
		cp->_absolute = true;
		cp->rounding = std::max(2, (int)(width * 0.5));
	}
	{
		auto cp = add_scroll_bar({ pss.x - width * 2,width }, pss.x, pss.x, rcw, false, hnpos);
		bind_scroll_bar(cp, false); // 绑定水平滚动条
		cp->_pos_width = pos_width.x > 0 ? pos_width.x : width;
		cp->hscroll = {};
		cp->_absolute = true;
		cp->rounding = std::max(2, (int)(width * 0.5));
	}
}
scroll2_t plane_cx::add_scroll2(const glm::ivec2& viewsize, int width, int rcw, const glm::ivec2& pos_width, const glm::ivec2& vnpos, const glm::ivec2& hnpos)
{
	scroll2_t r = {};
	auto pss = viewsize;
	{
		auto cp = add_scroll_bar({ width,pss.y - width * 2 }, pss.y, pss.y, rcw, true, vnpos);
		cp->_pos_width = pos_width.y > 0 ? pos_width.y : width * 2;//滚轮事件每次滚动量
		cp->hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->has_hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->hscroll = {};
		cp->rounding = std::max(2, (int)(width * 0.5));
		r.v = cp;
	}
	{
		auto cp = add_scroll_bar({ pss.x - width * 2,width }, pss.x, pss.x, rcw, false, hnpos);
		cp->_pos_width = pos_width.x > 0 ? pos_width.x : width;
		cp->hscroll = {};
		cp->rounding = std::max(2, (int)(width * 0.5));
		r.h = cp;
	}
	return r;
}

void plane_cx::set_scroll_hide(bool is)
{
	if (horizontal)
		horizontal->hideble = is;
	if (vertical)
		vertical->hideble = is;
}

void plane_cx::set_scroll_pos(const glm::ivec2& ps, bool v)
{
	if (v)
	{
		if (vertical)
			vertical->pos = ps;
	}
	else
	{
		if (horizontal)
			horizontal->pos = ps;
	}
}

void plane_cx::set_scroll_size(const glm::ivec2& ps, bool v)
{
	if (v)
	{
		if (vertical)
			vertical->size = ps;
	}
	else
	{
		if (horizontal)
			horizontal->size = ps;
	}
}

void plane_cx::set_view(const glm::ivec2& view_size, const glm::ivec2& content_size)
{
	if (horizontal)
		horizontal->set_viewsize(view_size.x, content_size.x, 0);
	if (vertical)
		vertical->set_viewsize(view_size.y, content_size.y, 0);
}

void plane_cx::set_scroll_visible(const glm::ivec2& hv)
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

glm::ivec2 plane_cx::get_scroll_range()
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

void plane_cx::set_scroll_pts(const glm::ivec2& pts, int t)
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
void plane_cx::set_clear_color(uint32_t c)
{
	tv->clear_color = rgb2bgr(c);
}
void plane_cx::set_color(const glm::ivec4& c) {
	border = c;
}
void plane_cx::move2end(widget_base* wp)
{
	if (wp)
	{
		auto& v = widgets;
		v.erase(std::remove_if(v.begin(), v.end(), [=](widget_base* pr) {return pr == wp; }), v.end());
		v.push_back(wp);
	}
}

size_t plane_cx::add_familys(const char* familys, const char* style)
{
	return ltx ? ltx->add_familys(familys, style) : 0;
}

scroll_bar* plane_cx::add_scroll_bar(const glm::ivec2& size, int vs, int cs, int rcw, bool v, const glm::ivec2& npos)
{
	auto p = new scroll_bar();
	if (p)
	{
		add_widget(p);
		p->_absolute = true;
		p->size = size;
		auto ss = get_size();
		glm::ivec2 dw = {};
		if (!v)
		{
			if (p->size.x < 0)
				p->size.x = ss.x - border.z * 2;
			p->pos.y = ss.y - border.y;
			p->pos.x = border.z;
			dw.y = 1;
			p->_dir = 0;
		}
		if (v)
		{
			if (p->size.y < 0)
				p->size.y = ss.y - border.z * 2;
			p->pos.x = ss.x - (border.y);
			p->pos.y = border.z;
			dw.x = 1;
			p->_dir = 1;
		}
		if (p->size.x < rcw) {
			p->size.x = rcw;
		}
		if (p->size.y < rcw) {
			p->size.y = rcw;
		}
		dw *= p->size;
		p->pos -= dw;
		p->pos -= npos;
		p->set_viewsize(vs, cs, rcw);
	}
	return p;
}

void plane_cx::bind_scroll_bar(scroll_bar* p, bool v)
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


void plane_cx::add_widget(widget_base* p)
{
	if (p)
	{
		p->ltx = ltx;
		p->parent = this;
		widgets.push_back(p); uplayout = true;
	}
}
void plane_cx::remove_widget(widget_base* p)
{
	auto& v = widgets;
	auto ps = v.size();
	v.erase(std::remove(v.begin(), v.end(), p), v.end());
	if (v.size() != ps)
	{
		valid = true; uplayout = true;
	}
}
switch_tl* plane_cx::add_switch(const glm::ivec2& size, const std::string& label, bool v, bool inlinetxt)
{
	auto p = new switch_tl();
	if (p) {
		add_widget(p);
		p->set_value(v);
		p->size = size;
		p->text = label;
		if (ltx && ltx->ctx) {
			glm::vec4 rc = { 0, 0, 0, 0 };
			glm::vec2 talign = { 0,0.5 };
			if (label.size())
			{
				//	p->txtps = ltx->add_text(0, rc, talign, label.c_str(), label.size(), fontsize);
			}
		}
		p->_autofree = true;
	}
	return p;
}
checkbox_tl* plane_cx::add_checkbox(const glm::ivec2& size, const std::string& label, bool v)
{
	auto p = new checkbox_tl();
	if (p) {
		add_widget(p);
		p->set_value(label, v);
		p->text = label;
		p->size = size;
		glm::vec4 rc = { 0, 0, 0, 0 };
		glm::vec2 talign = { 0,0.5 };
		if (label.size())
		{
		}
		p->_autofree = true;
	}
	return p;
}
radio_tl* plane_cx::add_radio(const glm::ivec2& size, const std::string& label, bool v, group_radio_t* gp)
{
	auto p = new radio_tl();
	if (p) {
		p->gr = gp;
		if (gp)
			gp->ct++;
		add_widget(p);
		p->set_value(label, v);
		p->text = label;
		p->size = size;
		glm::vec4 rc = { 0, 0, 0, 0 };
		glm::vec2 talign = { 0,0.5 };
		if (label.size())
		{
		}
		p->_autofree = true;
	}
	return p;
}

edit_tl* plane_cx::add_input(const std::string& label, const glm::ivec2& size, bool single_line) {
	edit_tl* edit1 = new edit_tl();
	if (edit1) {
		auto ss = size;
		edit1->set_size(ss);
		edit1->set_single(single_line);
		//edit1->set_family(familys.c_str(), fontsize); // 多字体混合无法对齐高度。英文行和中文行高度不同		 
		edit1->set_family(0, fontsize); // 多字体混合无法对齐高度。英文行和中文行高度不同		 
		edit1->ppos = get_pos();
		add_widget(edit1);
		edit1->_autofree = true;
	}
	return edit1;
}
gradient_btn* plane_cx::add_gbutton(const std::string& label, const glm::ivec2& size, uint32_t bcolor)
{
	gradient_btn* gb = new gradient_btn();
	if (gb) {
		gb->init({ 0,0,size }, label, bcolor);
		gb->family = familys;
		gb->font_size = fontsize;
		add_widget(gb);
		gb->_autofree = true;
	}
	return gb;
}
color_btn* plane_cx::add_cbutton(const std::string& label, const glm::ivec2& size, int idx)
{
	color_btn* gb = new color_btn();
	if (gb)
	{
		gb->str = label;
		gb->family = familys;
		gb->font_size = fontsize;
		gb->set_btn_color_bgr(idx);
		gb->size = size;
		add_widget(gb);
		gb->_autofree = true;
	}
	return gb;
}

color_btn* plane_cx::add_label(const std::string& label, const glm::ivec2& size, int idx)
{
	auto p = add_cbutton(label, size, idx);
	if (p)
	{
		p->effect = uTheme::light;
		p->light = 0;
		p->text_align.x = 0;
	}
	return p;
}

progress_tl* plane_cx::add_progress(const std::string& format, const glm::ivec2& size, double v)
{
	auto p = new progress_tl();
	if (p)
	{
		p->size = size;
		p->format = format;
		add_widget(p);
		p->set_value(v);
		p->_autofree = true;
	}
	return p;
}

slider_tl* plane_cx::add_slider(const glm::ivec2& size, int h, double v)
{
	auto p = new slider_tl();
	if (p)
	{
		p->size = size;
		p->wide = h;
		p->vertical = size.y > size.x;
		if (p->vertical)
			p->sl.x = size.x * 0.5;
		else
			p->sl.x = size.y * 0.5;
		add_widget(p);
		p->set_value(v);
		p->_autofree = true;
	}
	return p;
}

colorpick_tl* plane_cx::add_colorpick(uint32_t c, int w, int h, bool alpha)
{
	auto p = new colorpick_tl();
	if (p) {
		add_widget(p);
		p->init(c, w, h, alpha);
		p->_autofree = true;
	}
	return p;
}

void plane_cx::set_family_size(const std::string& fam, int fs, uint32_t color)
{
	if (fam.size())
		familys = fam;
	if (fs > 0)
		fontsize = fs;
	text_color = color;
}



void plane_cx::set_update()
{
	evupdate++;
}
void plane_cx::update(float delta)
{
	//print_time a("plane_cx::update");
	int ic = 0;
	if (horizontal)
	{
		horizontal->update(delta);
	}
	if (vertical) {
		vertical->update(delta);
	}
	auto sps = get_spos();	// 获取滚动量

	// 
	//printf("type\t%d\n", _hover_eq.w);
	if (_hover_eq.z > 0 && (devent_type_e)_hover_eq.w == devent_type_e::mouse_move_e)
	{
		_hover_eq.x += delta;
		// 大于设置时间触发hover事件
		if (_hover_eq.x > _hover_eq.y) {
			auto length = event_wts.size();
			for (size_t i = 0; i < length; i++)
			{
				auto pw = event_wts[i];
				if (!pw || !pw->visible || pw->_disabled_events || !(pw->bst & (int)BTN_STATE::STATE_HOVER) || pw->bst & (int)BTN_STATE::STATE_ACTIVE)continue;
				auto vpos = sps * pw->hscroll;
				send_hover(pw, _move_pos);
			}
			_hover_eq.x = _hover_eq.z = 0;//重置
		}
	}
	for (auto& it : widgets) {
		ic += it->update(delta);
	}
	if (update_cb)
		ic += update_cb(delta);
	if (uplayout) {
		mk_layout();
	}
	ic += ltx->update_text();
	if (_draw_sbox && vgs.thickness > 0 && vgtms.x > 0 && vgtms.y > 0)
	{
		auto& kt = vgtms.z;
		kt += delta;
		if (kt > vgtms.x)
		{
			vgtms.w += 1;
			if (vgtms.w > vgtms.y)vgtms.w = 0;
			kt = 0.0;
			ic++;
		}
	}
	else {
		vgtms.w = 0;
	}
	if (ic > 0 || evupdate > 0)tv->set_draw_update();
	auto kms = delta * 1000;
	dms -= kms;
	if (dms > 0 || kms <= 0)
		return;
	dms = dmsset;
	auto cr = tv->begin_frame(0);
	if (cr)
	{
		evupdate = 0;
		auto ls = get_size();
		bool has_border = (border.y > 0 && border.x != 0);
		{
			// 背景 
			if (border.w) {
				glm::vec4 rf = { 0.,0.,ls };
				if (has_border) {
					rf.x = rf.y = 0.5;
				}
				draw_rectangle(cr, rf, border.z);
				fill_stroke(cr, border.w, 0, 0, false);
			}
			if (draw_back_cb)
			{
				cairo_as _aa_(cr);
				draw_back_cb(cr, sps);
			}
			for (auto& it : widgets) {
				if (it->visible)
				{
					cairo_as __cas_(cr);
					auto scp = sps * it->hscroll;
					if (scp.x != 0 || scp.y != 0)
						cairo_translate(cr, scp.x, scp.y);// 滚动条影响
					it->draw(cr);
				}
			}
			if (draw_front_cb)
			{
				cairo_as _aa_(cr);
				draw_front_cb(cr, sps);
			}
			if (vgs.thickness > 0) {
				auto dps1 = get_dragpos(0);//获取拖动时的坐标
				auto v6 = get_dragv6(0);
				auto st = &vgs;
				st->dash_offset = -vgtms.w;
				float pad = st->thickness > 1 ? 0.0 : -0.5;
				cairo_as _ss_(cr);
				auto pos = v6->cp0;
				auto pos1 = v6->cp1;
				auto w = pos1;
				auto ss = glm::abs(pos - w);
				if (w.x < pos.x)
				{
					pos.x = w.x;
				}
				if (w.y < pos.y)
				{
					pos.y = w.y;
				}
				_draw_sbox = ss.x > 0 && ss.y > 0;
				if (_draw_sbox)
				{
					auto r = ss.x < st->round * 2 || ss.y < st->round * 2 ? 0 : st->round;
					draw_rectangle(cr, { pos.x + pad ,pos.y + pad ,ss.x,ss.y }, r);
					fill_stroke(cr, st);
				}
			}
			if (horizontal)
			{
				horizontal->draw(cr);
			}
			if (vertical) {
				vertical->draw(cr);
			}
		}
		// 边框线  
		if (has_border)
		{
			ls -= 1;
			draw_rectangle(cr, { 0.5,0.5,ls }, border.z);
			fill_stroke(cr, 0, border.x, border.y, false);
		}
		tv->end_frame(cr);
		_pat->img->valid = true;
	}
	bool savetext = false;
	if (savetext)
	{
		int i = 0;
		for (auto img : ltx->msu)
		{
			std::string fn = "cache/update_text_img_" + std::to_string(i++) + ".png";
			cairo_surface_write_to_png(img, fn.c_str());
		}
	}
}


flex_item* flexlayout(flex_item* r, std::vector<glm::vec4>& v, const glm::vec2& pos, const glm::vec2& gap)
{
	flex_item* p = 0;
	auto length = v.size();
	if (r && length)
	{
		p = new flex_item[length];
		if (p)
		{
			for (size_t i = 0; i < length; i++)
			{
				v[i].z += gap.x; v[i].w += gap.y;
				p[i].width = v[i].z;
				p[i].height = v[i].w;
				r->item_add(p + i);
			}
			r->layout();

			for (size_t i = 0; i < length; i++)
			{
				if (p[i].position != flex_item::flex_position::POS_ABSOLUTE) {
					v[i].x = p[i].frame[0] + pos.x;
					v[i].y = p[i].frame[1] + pos.y;
				}
			}
		}
	}
	return p;
}

void plane_cx::mk_layout()
{
	uplayout = false;
	if (custom_layout)return;// 自定义布局计算则退出默认而已计算
	flex_item root;
	auto ss = get_size();
	root.width = ss.x;
	root.height = ss.y;
	root.justify_content = _css.justify_content;
	root.align_content = _css.align_content;
	root.align_items = _css.align_items;
	root.wrap = _css.wrap;
	root.direction = _css.direction;

	std::vector<glm::vec4> layouts;
	std::vector<widget_base*> wbs;
	layouts.reserve(widgets.size());
	wbs.reserve(widgets.size());
	for (auto& it : widgets) {
		auto p = (widget_base*)it;
		if (p->_absolute)continue;
		layouts.push_back({ p->pos, p->size });
		wbs.push_back(p);
	}
	flex_item* c = flexlayout(&root, layouts, _lpos, _lms);
	if (c)
	{
		auto length = wbs.size();
		for (size_t i = 0; i < length; i++)
		{
			auto p = (widget_base*)wbs[i];
			auto it = layouts[i];
			glm::vec2 itss = { it.z,it.w };
			p->pos = it;
			p->pos += (itss - p->size) * _css.pos_align;
		}
		delete[] c;
	}
}
bool vht(const std::vector<widget_base*>& widgets, const glm::ivec2& p, glm::ivec2 ips, const glm::ivec2& scroll_pos) {
	bool r = false;
	for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
		auto pw = (widget_base*)*it;
		if (!pw || !pw->visible || pw->_disabled_events)continue;
		glm::vec2 mps = p; mps -= ips;
		mps -= pw->hscroll * scroll_pos;
		// 判断是否鼠标在控件上
		glm::vec4 ppos = { pw->pos,pw->size };
		auto k = check_box_cr1(mps, &ppos, 1, sizeof(glm::vec4));
		if (k.x) { r = true; }
	}
	return r;
}
bool plane_cx::hittest(const glm::ivec2& p)
{
	bool r = false;
	auto sps = get_spos();
	glm::ivec2 ips = get_pos(); auto ss = (glm::ivec2)get_size();
	glm::vec4 rc = { ips ,ips + ss };
	if (rect_includes(rc, p)) {
		if (draggable)
		{
			r = true;
		}
		else
		{
			r = vht(widgets, p, ips, sps);
			if (!r) {
				r = vht({ vertical ,horizontal }, p, ips, {});
			}
		}
	}
	//printf("%p\t%d\n", this, (int)r);
	return r;
}

size_t plane_cx::push_dragpos(const glm::ivec2& pos, const glm::ivec2& size)
{
	auto ps = drags.size();
	drag_v6 t = {};
	t.pos = pos;
	t.size = size;
	t.z = 0;
	drags.push_back(t);
	dragsp.clear();
	for (auto& it : drags) { dragsp.push_back(&it); }
	sortdg();
	return ps;
}

glm::ivec3 plane_cx::get_dragpos(size_t idx)
{
	glm::ivec3 r = (idx < drags.size()) ? glm::ivec3(drags[idx].pos, drags[idx].z) : glm::ivec3();
	r += glm::ivec3(get_spos(), 0);
	return r;
}

drag_v6* plane_cx::get_dragv6(size_t idx)
{
	return (idx < drags.size()) ? &drags[idx] : nullptr;
}

void plane_cx::sortdg()
{
	std::stable_sort(dragsp.begin(), dragsp.end(), [](const drag_v6* t1, const drag_v6* t2) { return t1->z < t2->z; });
}

gshadow_cx* plane_cx::get_gs()
{
	return ltx ? ltx->gs : nullptr;
}

void plane_cx::set_shadow(const rect_shadow_t& rs)
{
	auto gs = get_gs();
	auto rcs = gs->new_rect(rs);
	auto a = new atlas_cx();
	a->img = gs->img;
	a->img->type = 1;
	auto ss = get_size();
	ss -= rs.radius * 2;
	rcs.img_rc = { -rs.radius,-rs.radius,ss.x,ss.y };
	rcs.img_rc.x = rcs.img_rc.y = rs.radius;
	a->add(&rcs, 1);
	add_atlas(a);
}

void plane_cx::set_rss(int r)
{
	if (tv)
		tv->set_rss(r);
}

void plane_cx::on_motion(const glm::vec2& pos) {
	glm::ivec2 ps = pos;
	if (ckinc > 0)
	{
		if (draggable)
			set_pos(ps - curpos);
		//else
		//	set_scroll_pts(ps - curpos, 1);

		plane_ev e = {};
		e.p = this; e.down = 1; e.clicks = 0; e.mpos = ps;
		e.drag = true;
		if (on_click)
		{
			on_click(&e);	// 执行拖动事件
		}
		for (auto& c : on_mouses)
		{
			c(&e);
		}
	}

	tv->on_motion(ps - tpos);
	update(0);

}
void plane_cx::on_button(int idx, int down, const glm::vec2& pos, int clicks, int r) {
	glm::ivec2 ps = pos;
	if (idx == 1)
	{
		glm::vec4 trc = viewport;
		auto k2 = check_box_cr1(pos, &trc, 1, sizeof(glm::vec4));

		if (k2.x)
		{
			if (draggable && down == 1)
				form_move2end(form, this); // 移动窗口前面
			if (!r)
			{
				if (down == 1 && ckinc == 0)
				{
					curpos = ps - tpos;
					ckinc++;
				}
				plane_ev e = {};
				e.p = this; e.down = down; e.clicks = clicks; e.mpos = ps;
				if (on_click)
				{
					on_click(&e);	// 执行单击事件
				}
				for (auto& c : on_mouses)
				{
					c(&e);
				}
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
	tv->on_button(idx, down, ps - tpos, clicks);
	update(0);
	_draw_valid = true;
}
void plane_cx::on_wheel(double x, double y)
{
	update(0);
	_draw_valid = true;
}

bool on_wpe(widget_base* pw, int type, et_un_t* ep, const glm::ivec2& ppos)
{
	bool r = false;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	auto pt = dynamic_cast<edit_tl*>(pw);
	if (!pt) {
		widget_on_event(pw, type, ep, ppos);
		if (ep->ret && t == devent_type_e::mouse_button_e)
		{
			auto p = e->b;
			if (p->down == 1)
				get_input_state(0, 1);
		}
	}
	else
	{
		pt->ppos = ppos;
		pt->on_event_e(type, ep);
	}
	return (ep->ret);
}
void plane_cx::on_event(uint32_t type, et_un_t* ep)
{
	auto e = &ep->v;
	if (!visible)return;
	auto t = (devent_type_e)type;
	glm::ivec2 vgpos = viewport;
	int r1 = 0;
	auto ppos = get_pos();
	auto sps = get_spos();
	_hover_eq.w = type;
	widget_base* hpw = 0;
	if (t == devent_type_e::mouse_move_e)
	{
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y };
		glm::vec4 trc = viewport;
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

		event_wts.clear();
		event_wts1.clear();
		if (horizontal) {
			horizontal->bst& (int)BTN_STATE::STATE_HOVER ? event_wts.push_back(horizontal) : event_wts1.push_back(horizontal);//水平滚动条
		}
		if (vertical) {
			vertical->bst& (int)BTN_STATE::STATE_HOVER ? event_wts.push_back(vertical) : event_wts1.push_back(vertical);//垂直滚动条
		}
		for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
			if ((*it)->bst & (int)BTN_STATE::STATE_HOVER)
				event_wts.push_back(*it);
			else
				event_wts1.push_back(*it);
		}
		auto length = event_wts.size();
		{
			// 生成鼠标离开消息
			for (size_t i = 1; i < length; i++) {
				auto pw = event_wts[i];
				if (!pw || !pw->visible || pw->_disabled_events)continue;
				auto vpos = sps * pw->hscroll;
				auto p = e->m;
				glm::ivec2 mps = { p->x,p->y }; mps -= ppos + vpos;
				bool isd = pw->cmpos == mps;
				pw->cmpos = mps;
				pw->bst &= ~(int)BTN_STATE::STATE_HOVER;
				// 鼠标离开
				pw->on_mevent((int)event_type2::on_leave, mps);
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
			auto vpos = sps * pw->hscroll;
			on_wpe(pw, type, ep, ppos + vpos);
			if (ep->ret && btn) {
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
				auto vpos = sps * pw->hscroll;
				on_wpe(pw, type, ep, ppos + vpos);
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

image_btn::image_btn() :widget_base(WIDGET_TYPE::WT_IMAGE_BTN)
{

}
image_btn::~image_btn()
{
}
color_btn::color_btn() :widget_base(WIDGET_TYPE::WT_COLOR_BTN)
{
}
color_btn::~color_btn()
{
}
gradient_btn::gradient_btn() :widget_base(WIDGET_TYPE::WT_GRADIENT_BTN)
{
}
gradient_btn::~gradient_btn()
{
}
radio_tl::radio_tl() :widget_base(WIDGET_TYPE::WT_RADIO)
{
}
checkbox_tl::checkbox_tl() :widget_base(WIDGET_TYPE::WT_CHECKBOX)
{
}
checkbox_tl::~checkbox_tl()
{
}
switch_tl::switch_tl() :widget_base(WIDGET_TYPE::WT_SWITCH)
{
}
switch_tl::~switch_tl()
{
}
progress_tl::progress_tl() :widget_base(WIDGET_TYPE::WT_PROGRESS)
{
}
progress_tl::~progress_tl()
{
}
slider_tl::slider_tl() :widget_base(WIDGET_TYPE::WT_SLIDER)
{
}
slider_tl::~slider_tl()
{
}
colorpick_tl::colorpick_tl() :widget_base(WIDGET_TYPE::WT_COLORPICK)
{
}
colorpick_tl::~colorpick_tl()
{
}
scroll_bar::scroll_bar() :widget_base(WIDGET_TYPE::WT_SCROLL_BAR)
{
}
scroll_bar::~scroll_bar()
{
}


bool image_btn::on_mevent(int type, const glm::vec2& mps)
{
	return false;
}

bool image_btn::update(float) {
	return false;
}
void image_btn::draw(cairo_t* g) {

}

btn_cols_t* color_btn::set_btn_color_bgr(size_t idx)
{
	static std::vector<std::array<uint32_t, 8>> bcs = { { 0xFFffffff, 0xFF409eff, 0xFF409eff, 0xFF66b1ff, 0xFFe6e6e6, 0xFF0d84ff, 0xFF0d84ff ,0 },
		{ 0xFFffffff, 0xFF67c23a, 0xFF67c23a, 0xFF85ce61, 0xFFe6e6e6, 0xFF529b2e, 0xFF529b2e ,0},
		{ 0xFFffffff, 0xFF909399, 0xFF909399, 0xFFa6a9ad, 0xFFe6e6e6, 0xFF767980, 0xFF767980 ,0},
		{ 0xFFffffff, 0xFFe6a23c, 0xFFe6a23c, 0xFFebb563, 0xFFe6e6e6, 0xFFd48a1b, 0xFFd48a1b ,0 },
		{ 0xFFffffff, 0xFFf56c6c, 0xFFf56c6c, 0xFFf78989, 0xFFe6e6e6, 0xFFf23c3c, 0xFFf23c3c ,0 }
	};
	auto ret = (btn_cols_t*)((idx < bcs.size()) ? bcs[idx].data() : nullptr);
	if (ret)
	{
		bgr = 1;
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

void gradient_btn::draw(cairo_t* g)
{
	auto p = this;
	float x = p->pos.x, y = p->pos.y, w = p->size.x, h = p->size.y;
	int pushed = p->mPushed ? 0 : 1;
	uint32_t gradTop = p->gradTop;
	uint32_t gradBot = p->gradBot;
	uint32_t borderDark = p->borderDark;
	uint32_t borderLight = p->borderLight;
	double oa = p->opacity;
	auto ns = p->size;

	auto bc = effect == uTheme::light ? p->back_color : set_alpha_xf2(p->back_color, get_alpha_f(p->back_color));
	double rounding = p->rounding;
	glm::vec2 ns1 = { w * 0.5, h * 0.5 };
	auto nr = (int)std::min(ns1.x, ns1.y);
	if (rounding > nr)
	{
		rounding = nr;
	}
	cairo_save(g);
	cairo_translate(g, x, y);
	if (is_alpha(bc))
	{
		bc = set_alpha_f(bc, oa);
		draw_rectangle(g, { thickness,thickness, w - thickness, h - thickness * 2 }, rounding);
		set_color(g, bc);
		cairo_fill(g);
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
	glm::vec2 rct = { w - thickness, h - thickness * 2 };
	glm::vec4 gtop = to_c4(gradTop);
	glm::vec4 gbot = to_c4(gradBot);

	cairo_save(g);
	if (effect == uTheme::dark)
		cairo_translate(g, thickness, thickness);
	else if (p->mPushed)
		cairo_translate(g, thickness, thickness);
	paint_shadow(g, 0, rct.y, rct.x, rct.y, gtop, gbot, 0, rounding);// 垂直方向
	cairo_restore(g);
	// 渲染标签

	glm::vec2 ps = { thickness * 2,thickness * 2 };
	if (p->mPushed) {
		ps += thickness;
	}
	ns -= thickness * 4;
	glm::vec4 rc = { ps, ns };

#if 1
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

#else

	ltx->tem_rtv.clear();
	ltx->build_text(0, rc, text_align, p->str.c_str(), -1, p->font_size, ltx->tem_rtv);
	ltx->update_text();
	if (text_color_shadow)
	{
		cairo_as _aa_(g);
		cairo_translate(g, thickness, thickness);
		ltx->draw_text(g, ltx->tem_rtv, text_color_shadow);
	}
	ltx->draw_text(g, ltx->tem_rtv, p->text_color);

#endif // 1
	// 边框
	cairo_set_line_width(g, thickness);
	set_color(g, borderLight);
	draw_rectangle(g, { 0.5f,  (p->mPushed ? 0.5f : 1.5f), w, h - (p->mPushed ? 0.0f : 1.0f) }, rounding);
	cairo_stroke(g);
	set_color(g, borderDark);
	draw_rectangle(g, { 0.5f,  0.5f, w, h - 0.5 }, rounding);
	cairo_stroke(g);
	cairo_restore(g);
}


const char* gradient_btn::c_str()
{
	return str.c_str();
}

void gradient_btn::init(glm::ivec4 rect, const std::string& text, uint32_t back_color, uint32_t text_color)
{
	auto p = this;
	auto& info = *p;
	info.pos = { rect.x, rect.y };
	info.size = { rect.z, rect.w };
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
	if (bst == _old_bst)return false;
	_old_bst = bst;
	auto& info = *p;

	// (sta & hz::BTN_STATE::STATE_FOCUS)
	uint32_t gradTop = 0xff4a4a4a;
	uint32_t gradBot = 0xff3a3a3a;

	info.mPushed = (bst & (int)BTN_STATE::STATE_ACTIVE);
	info.mMouseFocus = (bst & (int)BTN_STATE::STATE_HOVER);
	if (bst & (int)BTN_STATE::STATE_DISABLE)
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
		if (bst == _old_bst)return false;
	}
	_old_bst = bst;
	auto p = this;
	btn_cols_t* pdc = &p->pdc;
	p->_disabled = (bst & (int)BTN_STATE::STATE_DISABLE);
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
		bool isdown = mPushed = (bst & (int)BTN_STATE::STATE_ACTIVE);
		p->hover = (bst & (int)BTN_STATE::STATE_HOVER);
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

void color_btn::draw(cairo_t* g)
{
	auto p = this;
	auto ns = p->size;
	static int bid = 1234;
	if (id == bid)
	{
		id = id;
	}
	cairo_as _as_(g);
	cairo_translate(g, p->pos.x, p->pos.y);
	if (p->dfill)
	{
		if (p->_circle)
		{
			auto sp = p->pos;
			auto r = lround(p->size.y * 0.5);
			sp += r;
			draw_circle(g, sp, r);
		}
		else
		{
			draw_rectangle(g, { 0.5,0.5, p->size }, p->rounding);
		}
		fill_stroke(g, p->dfill, 0, 0, bgr);
	}
	// 渲染标签
	glm::vec2 ps = { thickness * 2, thickness * 2 };
	if (p->mPushed) {
		ps += pushedps;
	}

	ns -= thickness * 4;

	glm::vec4 rc = { ps, ns };

	text_style_t st = {};
	st.font = 0;
	st.text_align = p->text_align;
	st.font_size = p->font_size;
	st.text_color = p->text_color;

	draw_text(g, ltx, p->str.c_str(), -1, rc, &st);

	/*	ltx->tem_rtv.clear();
		ltx->build_text(0, rc, text_align, p->str.c_str(), -1, p->font_size, ltx->tem_rtv);
		ltx->update_text();
		ltx->draw_text(g, ltx->tem_rtv, p->text_color);*/
		//auto rc = draw_text_align(g, p->str.c_str(), ps, ns, text_align, p->text_color, p->family.c_str(), p->font_size);

	if (p->dcol)
	{
		if (p->_circle)
		{
			auto sp = p->pos;
			auto r = lround(p->size.y * 0.5);
			sp += r;
			draw_circle(g, sp, r);
		}
		else
		{
			draw_rectangle(g, { 0.5,0.5, p->size }, p->rounding);
		}
		fill_stroke(g, 0, p->dcol, p->thickness, bgr);
	}

	//if ((bst & (int)BTN_STATE::STATE_HOVER))
	//{
	//	draw_rectangle(g, { 0,0,size.x,size.y }, p->rounding);
	//	fill_stroke(g, 0, 0x80ff8000, 1, 0);
	//}
}









#if 1
// 通用控件鼠标事件处理 type有on_move/on_scroll/on_drag/on_down/on_up/on_click/on_dblclick/on_tripleclick
bool widget_on_move(widget_base* wp, uint32_t type, et_un_t* ep, const glm::vec2& pos) {
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
		glm::vec4 trc = { wp->pos  ,wp->size };
		auto k = check_box_cr1(mps, &trc, 1, sizeof(glm::vec4));
		if (k.x) {
			bool hoverold = wp->bst & (int)BTN_STATE::STATE_HOVER;
			wp->bst |= (int)BTN_STATE::STATE_HOVER;   hover = true;
			if (!(wp->bst & (int)BTN_STATE::STATE_ACTIVE))// 不是鼠标则独占
				ep->ret = 1;
			if (!hoverold)
			{
				// 鼠标进入
				wp->on_mevent((int)event_type2::on_enter, mps);
				if (wp->mevent_cb) {
					wp->mevent_cb(wp, (int)event_type2::on_enter, mps);
				}
			}
		}
		else {
			if (wp->bst & (int)BTN_STATE::STATE_HOVER)
			{
				wp->bst &= ~(int)BTN_STATE::STATE_HOVER;
				// 鼠标离开
				wp->on_mevent((int)event_type2::on_leave, mps);
				if (wp->mevent_cb) {
					wp->mevent_cb(wp, (int)event_type2::on_leave, mps);
				}
			}
		}

		{
			if (wp->bst & (int)BTN_STATE::STATE_HOVER)
			{
				wp->on_mevent((int)event_type2::on_move, mps);
				if (wp->mevent_cb)
				{
					wp->mevent_cb(wp, (int)event_type2::on_move, mps);
				}
			}
			if (wp->bst & (int)BTN_STATE::STATE_ACTIVE) {
				auto dps = mps - wp->curpos;
				wp->on_mevent((int)event_type2::on_drag, dps);		// 拖动事件
				if (wp->mevent_cb)
				{
					wp->mevent_cb(wp, (int)event_type2::on_drag, dps);		// 拖动事件
				}
			}
		}
	}
	return hover;
}

void widget_on_event(widget_base* wp, uint32_t type, et_un_t* ep, const glm::vec2& pos) {
	if (!wp)return;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
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
		if (wp->bst & (int)BTN_STATE::STATE_HOVER) {
			if (p->down == 1)
			{
				ep->ret = 1;
			}
			if (p->button == 1) {
				if (p->down == 1) {
					wp->bst |= (int)BTN_STATE::STATE_ACTIVE;
					wp->curpos = mps - (glm::ivec2)wp->pos;
					wp->cks = 0;
					wp->on_mevent((int)event_type2::on_down, mps);
					if (wp->mevent_cb) { wp->mevent_cb(wp, (int)event_type2::on_down, mps); }
				}
				else {
					if ((wp->bst & (int)BTN_STATE::STATE_ACTIVE) && (isd || !wp->has_drag))
					{
						wp->cks = p->clicks;
						wp->on_mevent((int)event_type2::on_up, mps);
						if (wp->mevent_cb) {
							wp->mevent_cb(wp, (int)event_type2::on_up, mps);
						}
						int tc = (int)event_type2::on_click; //左键单击
						if (p->clicks == 2) { tc = (int)event_type2::on_dblclick; }
						else if (p->clicks == 3) { tc = (int)event_type2::on_tripleclick; }
						wp->_clicks = p->clicks;
						wp->on_mevent(tc, mps);
						if (wp->mevent_cb) {
							wp->mevent_cb(wp, tc, mps);
						}
						if (wp->click_cb)
						{
							wp->click_cb(wp, p->clicks);
						}
					}
					wp->bst &= ~(int)BTN_STATE::STATE_ACTIVE;
				}
			}
		}
		if (p->down == 0) {
			wp->bst &= ~(int)BTN_STATE::STATE_ACTIVE;
			wp->bst |= (int)BTN_STATE::STATE_NOMAL;
			wp->on_mevent((int)event_type2::mouse_up, mps);
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
		if (wp->bst & (int)BTN_STATE::STATE_HOVER || wp->has_hover_sc)
		{
			ep->ret = wp->on_mevent((int)event_type2::on_scroll, mps);
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
void send_hover(widget_base* wp, const glm::vec2& mps) {

	wp->on_mevent((int)event_type2::on_hover, mps);
	if (wp->mevent_cb)
	{
		wp->mevent_cb(wp, (int)event_type2::on_hover, mps);
	}
}


#endif // 1




#endif // 1



template<class T>
void free_obt(T*& p) {
	if (p) {
		delete p; p = 0;
	}
}





// todo ui



void draw_radios(cairo_t* cr, radio_info_t* p, radio_style_t* ps)
{
	if (ps->radius > 0) {
		draw_circle(cr, p->pos, ps->radius);
		if (p->value || p->swidth > 0)
			fill_stroke(cr, ps->col, 0, ps->thickness, 0);
		else
			fill_stroke(cr, 0, ps->line_col, ps->thickness, 0);
	}
	if (p->swidth > 0) {
		draw_circle(cr, p->pos, p->swidth);
		fill_stroke(cr, ps->innc, 0, ps->thickness, 0);
	}
}


// check打勾
void drawCheckMark(cairo_t* cr, glm::vec2 pos, uint32_t col, float sz1, bool mixed)
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
		cairo_move_to(cr, ps.x, ps.y);
		ps.x += sz1 - thickness;
		cairo_line_to(cr, ps.x, ps.y);
	}
	else {
		cairo_move_to(cr, bx - third, by - third);
		cairo_line_to(cr, bx, by);
		cairo_line_to(cr, bx + third * 2.0f, by - third * 2.0f);
	}
	fill_stroke(cr, 0, col, thickness, 0);
}
void draw_checkbox(cairo_t* cr, check_style_t* p, checkbox_info_t* pn)
{
	if (!p)return;
	auto cc = p->check_col;
	glm::ivec2 ps = pn->pos;
	draw_rectangle(cr, { ps.x,ps.y,p->square_sz,p->square_sz }, p->rounding);
	if (pn->value || pn->new_alpha > 0)
		fill_stroke(cr, p->fill, p->col, p->thickness, 0);
	else
		fill_stroke(cr, 0, p->line_col, p->thickness, 0);
	const float pad = std::max(1.0f, floor(p->square_sz / 6.0f));
	if (pn->new_alpha > 0)
	{
		cc = set_alpha_f(cc, pn->new_alpha);
		drawCheckMark(cr, (glm::vec2)ps + glm::vec2(pad, pad), cc, p->square_sz - pad * 2.0f, pn->mixed);
	}
}



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
{
}

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

bool radio_tl::on_mevent(int type, const glm::vec2& mps)
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
		if (size.x <= 0) {
			size.x = style.radius * 2;
		}
		if (size.y <= 0) {
			size.y = style.radius * 2;
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

					_old_bst = bst;
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

void radio_tl::draw(cairo_t* cr)
{
	auto p = this;
	if (cr && p) {
		cairo_as _as_(cr);
		glm::ivec2 poss = p->pos;
		poss.y += size.y * 0.5 - style.radius;
		cairo_translate(cr, poss.x + 0.5, poss.y + 0.5);
		int x = 0;
		{
			auto& it = v;
			it.pos = {};
			it.pos.x = x * style.radius * 5;
			it.pos += style.radius;
			draw_radios(cr, &it, &style);
			x++;
		}
	}
}

void checkbox_tl::bind_ptr(bool* p)
{
}

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

bool checkbox_tl::on_mevent(int type, const glm::vec2& mps)
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
		if (size.x <= 0) {
			size.x = style.square_sz;
		}
		if (size.y <= 0) {
			size.y = style.square_sz;
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
					_old_bst = bst;
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

void checkbox_tl::draw(cairo_t* cr)
{
	auto p = this;
	if (cr && p) {
		cairo_as _as_(cr);
		glm::ivec2 poss = p->pos;
		poss.y += (size.y - style.square_sz) * 0.5;
		cairo_translate(cr, poss.x + 0.5, poss.y + 0.5);
		int x = 0;
		{
			auto& it = v;
			it.pos.x = x * p->style.square_sz * 2.5;
			draw_checkbox(cr, &p->style, &it);
			x++;
		}
	}
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

bool switch_tl::on_mevent(int type, const glm::vec2& mps)
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
				_old_bst = bst;
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

void switch_tl::draw(cairo_t* cr)
{
	cairo_as _as_(cr);
	glm::ivec2 poss = pos;
	auto h = height;
	auto fc = h * cv * 0.5;
	auto ss = h * wf;
	if (size.x <= 0) {
		size.x = ss;
	}
	if (size.y <= 0) {
		size.y = h;
	}
	poss.x += (size.x - ss) * 0.5;
	poss.y += (size.y - height) * 0.5;
	cairo_translate(cr, poss.x, poss.y);
	draw_rectangle(cr, { 0.5,0.5, ss, h }, h * 0.5);
	fill_stroke(cr, dcol, 0, 0, 0);
	glm::vec2 cp = {};
	{
		auto ps = h * 0.5;
		cp.x += (ss - h) * cpos + h * 0.5;
		cp.y += ps;
		draw_circle(cr, cp, fc);
		fill_stroke(cr, color.z, 0, 0, 0);
	}
}

void progress_tl::set_value(double b)
{
	if (b < 0)b = 0;
	if (b > 1)b = 1;
	value = b;
	if (format.size() && ltx)
	{
		double k = get_v();
		std::string vv;
		text = pg::to_string(k) + format;
		width = size.x;
		if (text.size() && !text_inside) {
			auto rk = ltx->get_text_rect(0, text.c_str(), -1, font_size);
			size.x = width + rk.x + rounding * 0.5;
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

bool progress_tl::on_mevent(int type, const glm::vec2& mps)
{
	auto et = (event_type2)type;
	return false;
}

bool progress_tl::update(float delta)
{
	return false;
}

void progress_tl::draw(cairo_t* cr)
{
	cairo_as _as_(cr);
	glm::ivec2 poss = pos;
	glm::ivec2 ss = size;
	ss.x = width;
	cairo_translate(cr, poss.x, poss.y);
	draw_rectangle(cr, { 0.5,0.5, ss.x, ss.y }, rounding);
	fill_stroke(cr, color.y, 0, 0, 0);
	double xx = ss.x * value;
	int kx = 0;
	int r = rounding;
	if (xx > 0)
	{
		cairo_as _as_a(cr);
		if (xx < rounding * 2)
		{
			draw_rectangle(cr, { 0,0, xx, ss.y }, r);
			cairo_clip(cr);
			xx = r * 2;
			kx = 1;
		}
		draw_rectangle(cr, { 0.5,0.5, xx, ss.y }, r);
		fill_stroke(cr, color.x, 0, 0, 0);
	}
	if (text.size()) {
		auto rk = ltx->get_text_rect(0, text.c_str(), -1, font_size);
		if (text_inside) {
			ss.x = xx;
			ss.x -= r * 0.5;
		}
		else {
			ss.x = size.x;
		}
		if (kx) {

			ss.x += rk.x;
		}
		if (right_inside) {
			ss.x = size.x - r * 0.5;
		}
		glm::vec2 ta = { 1,0.5 };
		glm::vec4 rc = { 0, 0, ss };
		text_style_t st = {};
		st.font = 0;
		st.text_align = ta;
		st.font_size = font_size;
		st.text_color = text_color;
		draw_text(cr, ltx, text.c_str(), -1, rc, &st);

	}
}



void slider_tl::bind_ptr(double* p)
{
}

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

bool slider_tl::on_mevent(int type, const glm::vec2& mps)
{
	auto et = (event_type2)type;
	glm::ivec2 poss = mps - pos;
	glm::ivec2 ss = size;
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

void slider_tl::draw(cairo_t* cr)
{
	cairo_as _as_(cr);
	glm::ivec2 poss = pos;
	glm::ivec2 ss = size;
	cairo_translate(cr, poss.x, poss.y);
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
	draw_rectangle(cr, brc, rounding);
	fill_stroke(cr, color.y, 0, 0, 0);
	if (xx >= 0)
	{
		{
			cairo_as _as_a(cr);
			draw_rectangle(cr, crc, r);
			fill_stroke(cr, color.x, 0, 0, 0);
		}
		if (sl.x > 0) {
			draw_circle(cr, spos, sl.x);
			fill_stroke(cr, sl.y, color.x, thickness, 0);
		}
	}
}







// H in [0,360)
// S, V, R, G, B in [0,1]
glm::vec4 convertHSVtoRGB(const glm::vec4& hsv)
{
	double H = hsv.x * 360.0, S = hsv.y, V = hsv.z;
	double R, G, B;
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
	if (height < font_size)
		height = ltx->get_lineheight(0, font_size);
	h = height + step;
	cpx = height * 2.5;
	int minw = cpx + step * 2;
	if (width < minw) {
		width = minw + h;
	}
	size.x = width;
	int hn = 4;
	if (alpha)hn++;
	size.y = h * hn;
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
	double cw0 = cw - step, x = poss.x;
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
bool colorpick_tl::on_mevent(int type, const glm::vec2& mps)
{
	auto et = (event_type2)type;
	glm::ivec2 poss = mps - pos;
	glm::ivec2 ss = size;
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
			ltx->build_text(1, rc, ta, hsvstr.c_str(), -1, font_size, tem_rtv);
			rc.y = 0; rc.x += cpx;
			rc.z = cw;
			rc.w = height; ta.y = 0.5;
			ltx->heightline = 0; // 使用默认行高
			ltx->build_text(1, rc, ta, colorstr.c_str(), -1, font_size, tem_rtv);
			ltx->heightline = 0;
			assert(cw > 0);
		}
		if (on_change_cb) {
			on_change_cb(this, get_color());
		}
	}
	return false;
}
void draw_grid_fill(cairo_t* cr, const glm::vec2& ss, const glm::ivec2& cols, int width)
{
	int x = fmod(ss.x, width);
	int y = fmod(ss.y, width);
	int xn = ss.x / width;
	int yn = ss.y / width;
	if (x > 0)xn++;
	if (y > 0)yn++;

	cairo_as _as_a(cr);
	draw_rectangle(cr, { 0,0,ss.x,ss.y }, 0);
	cairo_clip(cr);
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
			cairo_fill(cr);
		}
	}
}
void draw_linear(cairo_t* cr, const glm::vec2& ss, const glm::vec4* cols, int count)
{
	cairo_pattern_t* gr = cairo_pattern_create_linear(0, 0, ss.x, ss.y);
	double n = count - 1;
	for (size_t i = 0; i < count; i++)
	{
		auto color = cols[i];
		cairo_pattern_add_color_stop_rgba(gr, i / n, color.x, color.y, color.z, color.w);
	}
	draw_rectangle(cr, { 0,0,ss.x,ss.y }, 0);
	cairo_set_source(cr, gr);
	cairo_fill(cr);
	cairo_pattern_destroy(gr);
}
void colorpick_tl::draw(cairo_t* cr)
{
	cairo_as _as_a(cr);
	glm::ivec2 poss = pos;
	glm::ivec2 ss = size;
	cairo_translate(cr, poss.x, poss.y);
	if (ltx)
	{
		glm::vec2 ta = { 0, 0.5 };
		glm::vec4 rc = { 0, height + step, ss };
		rc.w -= rc.y;
		ltx->draw_text(cr, tem_rtv, text_color);
	}
	float style_alpha8 = 1;
	//uint32_t col_hues[] = { 0xff0000ff,0xff00ffff,0xff00ff00,0xffffff00,0xffff0000,0xffff00ff,0xff0000ff };
	const glm::vec4 col_hues[6 + 1] = { glm::vec4(1,0,0,style_alpha8), glm::vec4(1,1,0,style_alpha8), glm::vec4(0,1,0,style_alpha8)
		, glm::vec4(0,1,1,style_alpha8), glm::vec4(0,0,1,style_alpha8), glm::vec4(1,0,1,style_alpha8), glm::vec4(1,0,0,style_alpha8) };
	int yh = height + step;
	glm::vec4 hc = {};
	HSVtoRGB(hsv, hc);
	{
		glm::vec4 cc = {};
		draw_grid_fill(cr, { cpx, height }, { -1,0xffdfdfdf }, height * 0.5);// 填充格子
		draw_rectangle(cr, { 0,0,cpx, height }, 0);
		set_source_rgba(cr, hc);
		cairo_fill(cr);// 填充当前颜色
	}
	{
		cairo_translate(cr, cpx, yh);
		draw_linear(cr, { cw,height }, col_hues, 7);	// H 
		glm::ivec4 rcc = { (cw - step) * hsv.x,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		draw_rectangle(cr, rcf, 0);
		fill_stroke(cr, -1, bc_color, thickness, false);
	}
	{
		cairo_translate(cr, 0, yh);
		glm::vec4 cc[] = { {1,1,1,1}, hc };
		draw_grid_fill(cr, { cw, height }, { -1,-1 }, height * 0.5);// 背景色
		draw_linear(cr, { cw, height }, cc, 2);	// S
		glm::ivec4 rcc = { (cw - step) * hsv.y,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		draw_rectangle(cr, rcf, 0);
		fill_stroke(cr, -1, bc_color, thickness, false);
	}
	{
		cairo_translate(cr, 0, yh);
		glm::vec4 cc[] = { {0,0,0,1}, hc };
		draw_grid_fill(cr, { cw, height }, { -1,-1 }, height * 0.5);// 背景色
		draw_linear(cr, { cw, height }, cc, 2);	// V
		glm::ivec4 rcc = { (cw - step) * hsv.z,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		draw_rectangle(cr, rcf, 0);
		fill_stroke(cr, -1, bc_color, thickness, false);
	}
	{
		cairo_translate(cr, 0, yh);
		glm::vec4 cc[] = { {0,0,0,0}, {1,1,1,1} };
		draw_grid_fill(cr, { cw, height }, { -1,0xffdfdfdf }, height * 0.5);//背景色
		draw_linear(cr, { cw, height }, cc, 2);	// A
		glm::ivec4 rcc = { (cw - step) * hsv.w,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		draw_rectangle(cr, rcf, 0);
		fill_stroke(cr, -1, bc_color, thickness, false);
	}

}
#endif


#if 1
void scroll_bar::set_viewsize(int64_t vs, int64_t cs, int rcw)
{
	_view_size = vs;
	_content_size = cs;
	if (rcw > 0)
		_rc_width = rcw;
	valid = true;
}

bool scroll_bar::on_mevent(int type, const glm::vec2& mps)
{
	auto et = (event_type2)type;
	glm::ivec2 poss = mps - pos;
	glm::ivec2 ss = size;
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
		if (thumb_size_m.z > 0 && ((bst & (int)BTN_STATE::STATE_HOVER) || hover_sc && (parent && parent->_hover)))
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
		glm::ivec2 ss = size;
		auto pxs = (ss[px] - _rc_width) * 0.5;
		auto ss1 = ss[_dir];
		auto ss2 = pos[_dir];
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
	if (!(bst & (int)BTN_STATE::STATE_HOVER)) {
		if (!(bst & (int)BTN_STATE::STATE_ACTIVE))
		{
			_tcc = _color.y; scale_s = scale_s0.x;
		}
	}
	if ((bst & (int)BTN_STATE::STATE_ACTIVE) && _color.w) {
		_tcc = _color.w;
	}
	return r;
}

void scroll_bar::draw(cairo_t* cr)
{
	cairo_as _as_a(cr);
	glm::ivec2 poss = pos;
	glm::ivec2 ss = size;

	{
		cairo_translate(cr, poss.x, poss.y);
		// 背景
		if (!hideble || thumb_size_m.z) {
			draw_rectangle(cr, { 0,0,ss }, rounding);
			fill_stroke(cr, _color.x, 0, 0, 0);
		}
		// 滑块
		double rw = _rc_width * scale_s;
		glm::ivec4 trc = { 0,0,rw,rw };
		int px = _dir ? 0 : 1;
		auto pxs = ceil((ss[px] - rw) * 0.5);
		trc.x = pxs;
		trc.y = pxs;
		trc[_dir] = tps[_dir] + _offset;
		trc[2 + _dir] = thumb_size_m.x;
		if (thumb_size_m.z)
		{
			draw_rectangle(cr, trc, _rc_width * 0.5 * scale_s);
			fill_stroke(cr, _tcc, 0, 0, 0);
		}
	}
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
	glm::ivec2 ss = size;
	int tsm = thumb_size_m.x + tps[_dir] * 2.0;
	auto st = ss[_dir] - tsm;
	return st;
}

void scroll_bar::set_offset(int pts)
{
	glm::ivec2 ss = size;
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
	glm::ivec2 ss = size;
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


std::vector<color_btn*> new_label(plane_cx* p, const std::vector<std::string>& t, int width, std::function<void(void* p, int clicks)> cb)
{
	std::vector<color_btn*> rv;
	if (p && p->ltx) {
		for (auto& it : t)
		{
			glm::vec2 bs;
			bs.x = bs.y = p->fontsize;
			bs.x = width;
			bs.y = p->ltx->get_lineheight(0, bs.y);
			auto kcb = p->add_label(it, bs, 0);
			rv.push_back(kcb);
		}
	}
	return rv;
}

std::vector<checkbox_com> new_checkbox(plane_cx* p, const std::vector<std::string>& t, int width, std::function<void(void* ptr, bool v)> cb)
{
	std::vector<checkbox_com> rv;
	if (p && p->ltx) {
		for (auto& it : t)
		{
			glm::vec2 bs;
			bs.x = bs.y = p->ltx->get_lineheight(0, p->fontsize);
			bs.x *= 1.5;
			auto c = p->add_checkbox(bs, it, false);
			c->v.on_change_cb = cb;
			bs.x = width;
			auto kcb = p->add_label(it, bs, 0);
			kcb->click_cb = [=](void* ptr, int clicks)
				{
					c->set_value();
				};
			rv.push_back({ c,kcb });
		}
	}
	return rv;
}
std::vector<radio_com> new_radio(plane_cx* p, const std::vector<std::string>& t, int width, std::function<void(void* ptr, bool v)> cb)
{
	std::vector<radio_com> rv;
	if (p && p->ltx) {
		auto gr = new group_radio_t();
		for (auto& it : t)
		{
			glm::vec2 bs;
			bs.x = bs.y = p->ltx->get_lineheight(0, p->fontsize);
			bs.x *= 1.5;
			auto c = p->add_radio(bs, it, false, gr);
			c->v.on_change_cb = cb;
			bs.x = width;
			auto kcb = p->add_label(it, bs, 0);
			kcb->click_cb = [=](void* ptr, int clicks)
				{
					c->set_value();
				};
			rv.push_back({ c,kcb });
		}
	}
	return rv;
}

