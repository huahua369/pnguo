#pragma once
#ifndef VG_H
#define VG_H


/*
	LINE_CAP_BUTT,0
	LINE_CAP_ROUND,1
	LINE_CAP_SQUARE2
	LINE_JOIN_MITER,0
	LINE_JOIN_ROUND,1
	LINE_JOIN_BEVEL2
*/
struct fill_style_d {
	uint32_t fill = 0;		// 填充颜色
	uint32_t color = 0;		// 线颜色stroke
	float thickness = 1;	// 线宽
	int round = 0;			// 圆角
	int cap = -1, join = -1;
	int dash_offset = 0;
	int dash_num = 0;		// 虚线计数 dash最大8、dash_p建议最大64
	union {
		uint64_t v = 0;
		uint8_t v8[8];
	}dash = {};		// ink  skip  ink  skip
	float* dash_p = 0;		// 虚线逗号/空格分隔的数字
};
/*	bg_style_d st[1] = {};
	st->dash = 0xF83E0;//0b11111000001111100000
	st->dash_num = 20;
	st->thickness = 1;
	st->join = 1;
	st->cap = 1;
	st->fill = 0x80FF7373;
	st->color = 0xffffffff;
*/
struct text_style_d
{
	int font = 0;
	int font_size = 18;
	glm::vec2 text_align = { 0.0,0.5 };
	glm::vec2 shadow_pos = { 1.0,1.0 };
	uint32_t text_color = 0xffffffff;
	uint32_t text_color_shadow = 0;
	bool clip = true;
};

struct rect_d {
	glm::vec2 pos;
	glm::vec2 size;
	float r;
	int st;
};
struct rect4r_d {
	glm::vec2 pos;
	glm::vec2 size;
	glm::vec4 r;
	int st;
};
struct circle_d {
	glm::vec2 pos;
	float radius;
	int st;
};
struct ellipse_d {
	glm::vec2 pos;
	glm::vec2 radius; 
	float rotationAngle;
	int st;
};
// 三角形基于矩形内 
//	 dir = 0;		// 尖角方向，0上，1右，2下，3左
//	 spos = 50;		// 尖角点位置0-1，中间就是0.5
struct triangle_d {
	glm::vec2 pos; glm::vec2 size, spos;
	int st;
};
struct grid_fill_d {
	glm::vec2 pos; glm::vec2 size; glm::ivec2 cols; int width;
	int st;
};

struct linear_fill_d {
	glm::vec2 pos; glm::vec2 size;
	const glm::vec4* cols; int count;
	int st;
};
struct line_d {
	glm::vec2 p1;
	glm::vec2 p2;
	int st;
};
// 折线只有stroke
struct polyline_d {
	glm::vec2* points;	// 点数据跟在结构体后面
	size_t count;
	int st;
	bool closed = false;
};

struct text_d {
	glm::vec2 pos;
	const char* text;	// 文本数据跟在结构体后面
	size_t len;
	int font_st; // 字体样式
	glm::vec2 rect;
};
enum class VG_DRAW_CMD :uint8_t {
	VG_CMD_CLEAR = 0,
	VG_CMD_RECT,
	VG_CMD_RECT4R,
	VG_CMD_CIRCLE,
	VG_CMD_ELLIPSE,
	VG_CMD_TRIANGLE,
	VG_CMD_FILL_GRID,
	VG_CMD_FILL_LINEAR,
	VG_CMD_STROKE_LINE,
	VG_CMD_STROKE_POLYLINE,
	VG_CMD_TEXT,
};
/*
渲染命令函数，
ctx：渲染上下文指针
cmds：命令数组
count：命令数量
data：命令数据指针
size：数据大小
order：渲染顺序，0=默认，数值越小，越靠前
*/
typedef void (*vg_add_draw_cmd_fun)(void* ctx, uint8_t* cmds, size_t count, void* data, size_t size, int order);
// 填充风格，返回开始序号
typedef size_t(*vg_add_style_fun)(void* ctx, fill_style_d* st, size_t count);
// 文本风格在text_d的font_st用，返回开始序号
typedef size_t(*vg_add_text_style_fun)(void* ctx, text_style_d* st, size_t count);
// 清除风格数据，0全部，1只清除填充风格，2只清除文本风格
typedef void(*vg_clear_style_fun)(void* ctx, int t);

#endif // !VG_H
