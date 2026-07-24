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
struct text_st;
struct image_r;
struct dblock_d;
struct geometry_d;
class vgdev_ctx;
class rvg_x
{
public:
	struct stack_item
	{
		glm::vec2 pos = {};				// 当前偏移
		glm::vec4 clip = {};			// 当前裁剪区域
		glm::vec2 scale = { 1.0f, 1.0f };
		float rotate_radians = 0.0f;
		glm::mat3x2 _transform = glm::mat3x2(1.0f);
	};
	glm::ivec2 pos = {};
private:
	vgdev_ctx* ctx = 0;
	std::stack<stack_item> _stk;
	std::stack<size_t> _st_view;
	stack_item _cur = {};				// 当前状态
	glm::ivec4 _tem_clip = {};
	glm::ivec4 _tview = {};				// 当前渲染区域 
	glm::ivec4 _clip = {};
	glm::vec2 _translate = {};
public:
	rvg_x();
	~rvg_x();
	void set_pos(const glm::ivec2& ps);
	void clear();
	void submit(fill_style_d* st);
	void submit(uint32_t fill, uint32_t color, int linewidth = 1);
	// 填充网格
	void grid_fill(const glm::vec2& size, const glm::ivec2& cols, int width);
	// 线性渐变填充
	void linear_fill(const glm::vec2& size, const glm::vec4* cols, int count);
	// 画2d箭头type=0线终点在三角形中间
	void add_arrow(const glm::vec2& p0, const glm::vec2& p1, float arrow_hwidth, float arrow_size, bool type);
	// 批量渲染同样式的块(圆或矩形)
	void draw_block(dblock_d* p, fill_style_d* st);
	// 描边或填充路径
	void draw_path(path_d* path, fill_style_d* style);
	// 批量画单条线
	void add_line(glm::vec4* p, size_t count);
	void add_line(const glm::vec2& ps0, const glm::vec2& ps1);

	void add_rect(const glm::vec4& rc, double r);
	void add_rect(const glm::vec4& rc, const glm::vec4& r);
	void add_circle(const glm::vec2& pos, float r);
	void add_ellipse(const glm::vec2& pos, const glm::vec2& r, float rotationAngle);
	// 三角形基于矩形内 
	//	 dir = 0;		// 尖角方向，0上，1右，2下，3左
	//	 spos = 50;		// 尖角点位置0-1，中间就是0.5
	void add_triangle(const glm::vec2& pos, const glm::vec2& size, const glm::vec2& dirspos);
	// 折线,polygon=closed
	void draw_polyline(const glm::vec2& pos, const glm::vec2* points, int points_count, uint32_t col, bool closed, float thickness);
	void add_polyline(const Clipper2Lib::PathsD* p, bool closed);
	void add_polyline(const glm::vec2* p, size_t count);
	// 渲染索引多段线，索引-1则跳过
	void draw_polylines(const glm::vec2& pos, const glm::vec2* points, int points_count, int* idx, int idx_count, uint32_t col, float thickness);
	// 文本渲染
	void set_text_style(text_style* ts);
	void add_text(text_st* p, text_style* ts);
	void add_image(image_ptr_t* img, const glm::ivec4& rc, const glm::ivec4& sliced, uint32_t color, const glm::ivec2& dsize, const glm::ivec2& pos);
	void add_image(image_r* r);
	// 添加gem指针的内容。顶点坐标不受状态影响
	void add_geometry(geometry_d* geo);
	void paint_shadow(double size_x, double size_y, double width, double height, const glm::vec4& shadow, const glm::vec4& color_to, bool rev, float r);
	// 路径裁剪
	void clip();
	// 矩形裁剪
	void clip(const glm::ivec4& c);
	glm::ivec4 get_clip();
	void save();
	void restore();
	void save0();
	void restore0();
	void fill();
	void stroke();
	void fill_preserve();
	void stroke_preserve();
	void fill_stroke(uint32_t fill, uint32_t color);
	void set_line_width(float w);
	void set_color(uint32_t color);
	void set_color(const glm::vec4& rgba);
	void translate(const glm::vec2& offset);
	void scale(float sx, float sy);
	void scale(const glm::vec2& sc);
	void rotate(float radians);
	void transform(const glm::mat3x2* matrix);
	void set_matrix(const glm::mat3x2* matrix);
	void get_matrix(glm::mat3x2* matrix);
	glm::vec2 get_translate();

	void push_ct(uint8_t op);
	/*
		rv->save();
		rv->push_view(glm::ivec4(0, 0, ss));// 设置渲染区域
		rv->set_line_width(border.y);
		glm::vec2 rc = ss;
		rc -= border.y;
		rv->add_rect({ 0.5,0.5,rc }, border.z);
		rv->fill_stroke(border.w, border.x);
		rv->pop_view();
		rv->restore();
	*/
	void push_view(const glm::ivec4& c, void* ptr = nullptr);
	void pop_view();
	void push_null(int v);
	uint32_t get_crc();
	uint32_t get_crc2index(size_t i);
public:
	void save_file(const char* fn);
	bool load_file(const char* fn);
private:

};



#ifdef __cplusplus
extern "C" {
#endif

	typedef struct state_save_t state_save_t;
	typedef struct vec2 vec2;
	struct paths_t {
		vec2* points = 0;			// 路径坐标点points array 
		uint32_t pointCount = 0;	// 数量effective points count
		uint32_t  pathPtr = 0;		// pointer in the path array
		uint32_t* pathes = 0;		// 每条路径的数量
		uint32_t  sizePathes = 0;	// 路径条数量
		uint32_t* color = 0;		// 独立颜色数组大小与sizePathes一致，0则用默认颜色curColor
		uint32_t curColor = 0xFFffffff;
		state_save_t* t = 0;
		uint32_t curVertOffset = 0;
	};
#ifdef __cplusplus 
}
#endif
struct drawctx_t {
	vgdev_ctx* ptr;
	void (*set_fence)(vgdev_ctx* ctx, bool enable);
	void* (*get_signal_sem)(vgdev_ctx* ctx);//signal_semaphore
	void (*set_glutess)(vgdev_ctx* ctx, bool enable);
	void (*clip_preserve)(vgdev_ctx* ctx, paths_t* p);
	void (*fill_preserve)(vgdev_ctx* ctx, paths_t* p);
	void (*stroke_preserve)(vgdev_ctx* ctx, paths_t* p);
	void (*clip)(vgdev_ctx* ctx, paths_t* p);
	void (*clip_rc)(vgdev_ctx* ctx, float* p);
	void (*clip0)(vgdev_ctx* ctx);
	void (*fill)(vgdev_ctx* ctx, paths_t* p);
	void (*stroke)(vgdev_ctx* ctx, paths_t* p);
	void (*clear_path)(paths_t* ctx);
	void* (*draw)(vgdev_ctx* ctx, VkvgContext ctxvg, void** waitSemaphore);
	void (*begin_frame)(vgdev_ctx* ctx);
	void (*end_frame)(vgdev_ctx* ctx);

	void (*set_line_cap)(vgdev_ctx* ctx, vkvg_line_cap_t lineCap);
	void (*set_line_join)(vgdev_ctx* ctx, vkvg_line_join_t lineJoin);
	void (*set_fill_rule)(vgdev_ctx* ctx, vkvg_fill_rule_t fillRule);
	void (*set_color)(vgdev_ctx* ctx, uint32_t color);
	void (*set_source_rgba)(vgdev_ctx* ctx, float r, float g, float b, float a);
	void (*set_source)(vgdev_ctx* ctx, VkvgPattern pat);
	void (*set_operator)(vgdev_ctx* ctx, vkvg_operator_t op);

	VkvgPattern(*new_pattern_linear)(vgdev_ctx* ctx, float x0, float y0, float x1, float y1);
	VkvgPattern(*new_pattern_radial)(vgdev_ctx* ctx, float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse);
	VkvgPattern(*new_pattern_sweep)(vgdev_ctx* ctx, float cx, float cy, float start_angle, float end_angle);
	vkvg_status_t(*pattern_set_color_stop)(VkvgPattern pat, int idx, float r, float g, float b, float a);

	paths_t* (*get_paths)(vgdev_ctx* ctx);
	paths_t* (*new_paths)(vgdev_ctx* ctx);
	void (*free_paths)(paths_t* ctx);
	void (*new_sub_path)(paths_t* ctx);
	void (*arc)(paths_t* ctx0, float xc, float yc, float radius, float a1, float a2);
	void (*arc_negative)(paths_t* ctx0, float xc, float yc, float radius, float a1, float a2);

	void (*grid_fill)(vgdev_ctx* cr, glm::vec2 size, glm::ivec2 cols, int width);

	void (*translate)(vgdev_ctx* ctx, float dx, float dy);

	void(*set_line_width)(vgdev_ctx* ctx, float width);
	int (*rectangle)(paths_t* ctx, float x, float y, float w, float h, float r);

};

vgdev_ctx* new_vgctx();
void free_vgctx(vgdev_ctx* p);
drawctx_t get_drawctx(vgdev_ctx* p);

