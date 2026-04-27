#pragma once
/*
*
*	vkvg渲染矢量图到VkvgSurface表面
*
*/

#include <vector>
#include <map>
#include <string>
#include <stack>
#include "vkvg.h"

#ifndef GLM_FORCE_XYZW_ONLY
#define GLM_ENABLE_EXPERIMENTAL 
#define GLM_FORCE_XYZW_ONLY
#include <glm/glm.hpp>   
#endif
#ifndef vkvg_pattern_add_color_stop_rgba
#define vkvg_pattern_add_color_stop_rgba vkvg_pattern_add_color_stop
#endif 

#ifndef VK_DEFINE_HANDLE
#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_HANDLE(VkPhysicalDevice)
VK_DEFINE_HANDLE(VkDevice)
VK_DEFINE_HANDLE(VkQueue)
#endif // !VK_DEFINE_HANDLE 

#ifdef __cplusplus
extern "C" {
#endif
#ifndef VKVG_H
	typedef struct _vkvg_device_t* VkvgDevice;
	typedef struct _vkvg_surface_t* VkvgSurface;
	typedef struct _vkvg_context_t* VkvgContext;
	typedef struct _vkvg_pattern_t* VkvgPattern;
	struct vkvg_matrix_t;
	enum vkvg_status_t;
	enum vkvg_line_cap_t;
	enum vkvg_line_join_t;
	enum vkvg_operator_t;
	enum vkvg_fill_rule_t;
#endif
	struct vkvg_func_t
	{
		VkvgContext(*vkvg_create)(VkvgSurface surf);
		void(*vkvg_destroy)(VkvgContext ctx);
		vkvg_status_t(*vkvg_status)(VkvgContext ctx);
		const char* (*vkvg_status_to_string)(vkvg_status_t status);
		VkvgContext(*vkvg_reference)(VkvgContext ctx);
		uint32_t(*vkvg_get_reference_count)(VkvgContext ctx);
		void(*vkvg_flush)(VkvgContext ctx);
		void(*vkvg_new_path)(VkvgContext ctx);
		void(*vkvg_close_path)(VkvgContext ctx);
		void(*vkvg_new_sub_path)(VkvgContext ctx);
		void(*vkvg_path_extents)(VkvgContext ctx, float* x1, float* y1, float* x2, float* y2);
		void(*vkvg_get_current_point)(VkvgContext ctx, float* x, float* y);
		void(*vkvg_line_to)(VkvgContext ctx, float x, float y);
		void(*vkvg_rel_line_to)(VkvgContext ctx, float dx, float dy);
		void(*vkvg_move_to)(VkvgContext ctx, float x, float y);
		void(*vkvg_rel_move_to)(VkvgContext ctx, float x, float y);
		void(*vkvg_arc)(VkvgContext ctx, float xc, float yc, float radius, float a1, float a2);
		void(*vkvg_arc_negative)(VkvgContext ctx, float xc, float yc, float radius, float a1, float a2);
		void(*vkvg_curve_to)(VkvgContext ctx, float x1, float y1, float x2, float y2, float x3, float y3);
		void(*vkvg_rel_curve_to)(VkvgContext ctx, float x1, float y1, float x2, float y2, float x3, float y3);
		void(*vkvg_quadratic_to)(VkvgContext ctx, float x1, float y1, float x2, float y2);
		void(*vkvg_rel_quadratic_to)(VkvgContext ctx, float x1, float y1, float x2, float y2);
		vkvg_status_t(*vkvg_rectangle)(VkvgContext ctx, float x, float y, float w, float h);
		vkvg_status_t(*vkvg_rounded_rectangle)(VkvgContext ctx, float x, float y, float w, float h, float radius);
		void(*vkvg_rounded_rectangle2)(VkvgContext ctx, float x, float y, float w, float h, float rx, float ry);
		void(*vkvg_ellipse)(VkvgContext ctx, float radiusX, float radiusY, float x, float y, float rotationAngle);
		void(*vkvg_elliptic_arc_to)(VkvgContext ctx, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);
		void(*vkvg_rel_elliptic_arc_to)(VkvgContext ctx, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);
		void(*vkvg_stroke)(VkvgContext ctx);
		void(*vkvg_stroke_preserve)(VkvgContext ctx);
		void(*vkvg_fill)(VkvgContext ctx);
		void(*vkvg_fill_preserve)(VkvgContext ctx);
		void(*vkvg_paint)(VkvgContext ctx);
		void(*vkvg_clear)(VkvgContext ctx);
		void(*vkvg_reset_clip)(VkvgContext ctx);
		void(*vkvg_clip)(VkvgContext ctx);
		void(*vkvg_clip_preserve)(VkvgContext ctx);
		void(*vkvg_set_opacity)(VkvgContext ctx, float opacity);
		float(*vkvg_get_opacity)(VkvgContext ctx);
		void(*vkvg_set_source_color)(VkvgContext ctx, uint32_t c);
		void(*vkvg_set_source_rgba)(VkvgContext ctx, float r, float g, float b, float a);
		void(*vkvg_set_source_rgb)(VkvgContext ctx, float r, float g, float b);
		void(*vkvg_set_line_width)(VkvgContext ctx, float width);
		void(*vkvg_set_miter_limit)(VkvgContext ctx, float limit);
		//float(*vkvg_get_miter_limit)(VkvgContext ctx);
		void(*vkvg_set_line_cap)(VkvgContext ctx, vkvg_line_cap_t cap);
		void(*vkvg_set_line_join)(VkvgContext ctx, vkvg_line_join_t join);
		void(*vkvg_set_source_surface)(VkvgContext ctx, VkvgSurface surf, float x, float y);
		void(*vkvg_set_source)(VkvgContext ctx, VkvgPattern pat);
		void(*vkvg_set_operator)(VkvgContext ctx, vkvg_operator_t op);
		void(*vkvg_set_fill_rule)(VkvgContext ctx, vkvg_fill_rule_t fr);
		void(*vkvg_set_dash)(VkvgContext ctx, const float* dashes, uint32_t num_dashes, float offset);		// 虚线
		void(*vkvg_get_dash)(VkvgContext ctx, const float* dashes, uint32_t* num_dashes, float* offset);
		float(*vkvg_get_line_width)(VkvgContext ctx);
		vkvg_line_cap_t(*vkvg_get_line_cap)(VkvgContext ctx);
		vkvg_line_join_t(*vkvg_get_line_join)(VkvgContext ctx);
		vkvg_operator_t(*vkvg_get_operator)(VkvgContext ctx);
		vkvg_fill_rule_t(*vkvg_get_fill_rule)(VkvgContext ctx);
		VkvgPattern(*vkvg_get_source)(VkvgContext ctx);
		VkvgSurface(*vkvg_get_target)(VkvgContext ctx);
		bool(*vkvg_has_current_point)(VkvgContext ctx);
		void(*vkvg_save)(VkvgContext ctx);
		void(*vkvg_restore)(VkvgContext ctx);
		void(*vkvg_translate)(VkvgContext ctx, float dx, float dy);
		void(*vkvg_scale)(VkvgContext ctx, float sx, float sy);
		void(*vkvg_rotate)(VkvgContext ctx, float radians);
		void(*vkvg_transform)(VkvgContext ctx, const vkvg_matrix_t* matrix);
		void(*vkvg_set_matrix)(VkvgContext ctx, const vkvg_matrix_t* matrix);
		void(*vkvg_get_matrix)(VkvgContext ctx, vkvg_matrix_t* const matrix);
		void(*vkvg_identity_matrix)(VkvgContext ctx);
	};

#ifdef __cplusplus
}
#endif



// 混合模式
enum class blendmode_e :int {
	none = -1,
	normal = 0,	// 普通混合
	additive,
	multiply,
	modulate,
	screen,
	normal_prem,	// 预乘alpha
	additive_prem,
};
#ifndef CLIPPER_CORE_H
using PathsD = std::vector<std::vector<double>>;
#endif // !CLIPPER_CORE_H

struct path_d;
struct image_block;
struct image_ptr_t;
struct text_style;
union fitem_t;
struct dev_info_cx;
struct vg_style_data;
struct rich_text_t;
class packer_base;

// SDL渲染器专用
typedef struct texture_cb texture_cb;
struct text_vx
{
	glm::vec2 pos;
	glm::vec2 uv;
	glm::vec4 color;
};
struct submit_color_d
{
	uint32_t fill, color; int linewidth;
};
struct paint_shadow_d
{
	glm::vec2 size;
	glm::vec2 dst_size;
	glm::vec4 shadow;
	glm::vec4 color_to;
	float r = 0;
	bool rev = false;
};
struct grid_fill_r
{
	glm::vec2  size; glm::ivec2 cols; int width;
};
struct linear_fill_r
{
	glm::vec2 size; int count; glm::vec4 cols[0];
};
struct arrow_d
{
	glm::vec2 p0, p1; float arrow_hwidth, arrow_size; bool type;
};
struct rect_v4
{
	glm::vec4 rc;
	glm::vec4 r;
};
struct triangle_r
{
	glm::vec2 pos;
	glm::vec2 size;
	glm::vec2 spos;
};
struct circle_r
{
	glm::vec2 pos;
	float r;
};
struct ellipse_r
{
	glm::vec2 pos;
	glm::vec2 radius;
	float rotationAngle;
};
struct polyline_r
{
	glm::vec2 pos;
	int points_count;
	uint32_t col;
	float thickness;
	bool closed;
};
struct polyline_index_r
{
	glm::vec2 pos;
	int points_count;
	int idx_count;
	uint32_t col;
	float thickness;
};
struct image_r
{
	image_ptr_t* img;
	glm::ivec4 rc;
	glm::ivec4 sliced;
	glm::ivec2 dsize;
	glm::ivec2 pos;
	uint32_t color;
};
struct polyline_pd
{
	size_t count;
	bool closed;
};

struct d2_rt {
	glm::ivec2 pos;		// 纹理偏移
	glm::ivec2 size;	// 区域大小
	glm::ivec2 offset;	// 渲染偏移
	int surface = 0;
	int index = 0;
};
struct surface_ctx {
	void* surface;
	void* ctx;
};

struct vitext_t
{
	fitem_t* t = 0;		// 位图或文本
	d2_rt* d2 = 0;		// 矢量图
	size_t first = 0;	// 第一个位置
	size_t count = 0;	// 渲染数量
};
struct cmdrect_v {
	glm::ivec4 rc = {};	// 渲染区域
	glm::ivec2 offset;	// 偏移
	int type = 0;		// 0矢量图，1文本/位图
	int first = 0, count = 0;// 命令索引
};

class vkvg_ctx
{
public:
	VkvgContext ctx = 0;
	VkvgSurface _surf = 0;
	vkvg_func_t* fun = {};

public:
	vkvg_ctx(VkvgSurface surf);
	~vkvg_ctx();

	void transform(const glm::mat3* matrix);
	void transform(const glm::mat3x2* matrix);
	void set_matrix(const glm::mat3* matrix);
	void set_matrix(const glm::mat3x2* matrix);
	glm::mat3x2 get_matrix();
private:

};
struct vkvgctx;

class vkvg_dev
{
public:
	VkDevice vkdev = 0;
	vkvgctx* ctx = 0;
	vkvg_func_t* fun = {};
	void* memoryProperties = 0;
	VkSampleCountFlags samplecount = {};
	int qindex = 0;
public:
	vkvg_dev();
	~vkvg_dev();
	vkvg_func_t* get_fun();

	VkvgSurface new_surface(int width, int height);
	VkvgSurface new_surface(uint32_t* data, int width, int height);
	VkvgSurface new_surface(const char* fn);
	void free_surface(VkvgSurface p);
	vkvg_ctx* new_ctx(VkvgSurface p);
	vkvg_ctx* new_context(VkvgSurface p);
	void free_ctx(vkvg_ctx* p);

	// 直接获取VkImage指针
	void* get_vkimage(void* surface);
	void image_save(void* surface, const char* fn);
	glm::ivec2 get_image_data(void* surface, std::vector<uint32_t>* opt);
private:

};

struct fill_style_d;
struct dev_info_c
{
	VkInstance inst;
	VkPhysicalDevice phy;
	VkDevice vkdev;
	uint32_t qFamIdx;		// familyIndex
	uint32_t qIndex = 0;
	uint32_t qCount = 0;
};
// 64,32,16,8,4,2,1
vkvg_dev* new_vkvgdev(dev_info_c* c = 0, int sample = 8);
void free_vkvgdev(vkvg_dev* p);

void submit_style(VkvgContext cr, fill_style_d* st);
void draw_rounded_rectangle(VkvgContext cr, double x, double y, double width, double height, const glm::vec4& r);
void draw_triangle(VkvgContext cr, const glm::vec2& pos, const glm::vec2& size, const glm::vec2& dirspos);
void draw_arrow_to(VkvgContext ctx, float x, float y);
// type=0线终点在三角形中间
void draw_arrow(VkvgContext ctx, const glm::vec2& p0, const glm::vec2& p1, float arrow_hwidth, float arrow_size, bool type = 0);

// 画网格填充
void draw_grid_fill(VkvgContext cr, const glm::vec2& ss, const glm::ivec2& cols, int width);
// 画线性渐变填充
void draw_linear(VkvgContext cr, const glm::vec2& ss, const glm::vec4* cols, int count);



/*
渲染命令函数，
ctx：渲染上下文指针VkvgContext
cmds：命令数组
count：命令数量
data：命令数据指针
size：数据大小
order：渲染顺序，0=默认，数值越小，越靠前
*/
void vgc_draw_cmds(void* ctx, uint8_t* cmds, size_t count, void* data, size_t size, vg_style_data* style);
// 绘制路径函数
void vgc_draw_path(void* ctx, path_d* path, fill_style_d* style);
/*
批量渲染块(圆或矩形)
坐标数组point，count数量
rc的xy宽高。如果y为0，则表示绘制圆形，x为直径
*/
struct dblock_d {
	glm::vec2* points = 0; int count = 0; glm::vec2 rc = {};
	glm::vec2 pos = {};			// 块偏移，受scale_pos影响
	glm::vec2 view_pos = {};	// 视图偏移，不受scale_pos影响
	float scale_pos = 0;		// 视图缩放，不缩放线宽
};
void vgc_draw_block(void* ctx, dblock_d* p, fill_style_d* style);
// ui基本结构
// 文本渲染结构
struct text_st {
	glm::vec2 pos;
	glm::vec2 size;
	const char* text;
	int text_len;
};
// 矢量图渲染缓存
class vgcache_cx
{
public:

public:
	vgcache_cx();
	~vgcache_cx();

private:

};



class rvg_cx
{
public:
	enum Opcode : uint8_t {
		OP_NULL,
		OP_SUBMIT_STYLE, OP_SUBMIT_COLOR, OP_GRID_FILL, OP_LINEAR_FILL, OP_ADD_ARROW,
		OP_DRAW_BLOCK, OP_DRAW_PATH, OP_ADD_LINE_PTR, OP_ADD_RECT_DOUBLE,
		OP_ADD_RECT_VEC4, OP_ADD_CIRCLE, OP_ADD_ELLIPSE, OP_ADD_TRIANGLE, OP_POLYLINE_VEC2,
		OP_ADD_POLYLINE_PATH, OP_ADD_POLYLINE_VEC2_PTR, OP_POLYLINES,
		OP_TEXT_STYLE, OP_ADD_TEXT, OP_ADD_IMAGE, OP_PAINT_SHADOW, OP_TRANSLATE,
		OP_CLIP, OP_SAVE, OP_RESTORE, OP_FILL, OP_STROKE, OP_FILL_PRESERVE, OP_STROKE_PRESERVE,
		OP_SET_LINE_WIDTH, OP_SET_COLOR_UINT, OP_SET_COLOR_VEC4
		, OP_MAX_COUNT
	};
	std::vector<uint8_t> _cmdtype;		// 操作类型
	std::vector<uint8_t> _cmd;			// 命令数据
	std::vector<glm::ivec4> _vg_rect;	// 统计矢量图批次渲染的区域
	std::vector<glm::ivec3> _vg_bs;		// 统计矢量图批次渲染的索引
	std::stack<glm::vec2> translate_pos;
	std::vector<cmdrect_v> _data;		// 渲染数据列表
	glm::vec2 tpos = {};				// 当前偏移
	float _thickness = 1.0;
	glm::ivec4* _prc = 0;				// 当前批次渲染区域 
	glm::ivec2 pos = {};
public:
	rvg_cx();
	~rvg_cx();

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
	// 折线
	void draw_polyline(const glm::vec2& pos, const glm::vec2* points, int points_count, uint32_t col, bool closed, float thickness);
	void add_polyline(const PathsD* p, bool closed);
	void add_polyline(const glm::vec2* p, size_t count);
	// 渲染索引多段线，索引-1则跳过
	void draw_polylines(const glm::vec2& pos, const glm::vec2* points, int points_count, int* idx, int idx_count, uint32_t col, float thickness);
	// 文本渲染
	void set_text_style(text_style* ts);
	void add_text(text_st* p, size_t count, text_style* ts);
	void add_image(image_ptr_t* img, const glm::ivec4& rc, const glm::ivec4& sliced, uint32_t color, const glm::ivec2& dsize, const glm::ivec2& pos);
	void add_image(image_r* r);
	void paint_shadow(double size_x, double size_y, double width, double height, const glm::vec4& shadow, const glm::vec4& color_to, bool rev, float r);

	void translate(const glm::vec2& offset);
	void clip();
	void save();
	void restore();
	void fill();
	void stroke();
	void fill_preserve();
	void stroke_preserve();
	void set_line_width(float w);
	void set_color(uint32_t color);
	void set_color(const glm::vec4& rgba);
	void push_vrc();
	void merge_vrc(const glm::ivec4& c);
	void push_ct(uint8_t op);
	void nk_bs();
	bool is_image();
	void push_null(int v);
private:

};

class rvg_data_cx
{
public:
	packer_base* packer = 0;			// 矩形打包器
	glm::ivec2 max_rect = {};			// 最大的区域
	std::vector<d2_rt> dcv;				// 矢量缓存信息 
	std::vector<surface_ctx> surfaces;
	std::vector<vitext_t> dst_data;		// 渲染数据列表
	glm::ivec4 _view = {};
	glm::ivec2 pos = {};
	float stwidth = 2.0;
public:
	rvg_data_cx();
	~rvg_data_cx();
	void update(rvg_cx* rvg);
private:

};

// 输入矢量图返回渲染数据
class canvas2d_t
{
public:
	// 矢量图渲染用
	vkvg_dev* vgdev = 0;
	// 文本渲染用
	std::map<image_ptr_t*, void*> _vt;
	std::map<void*, void*> _vgt;
	std::vector<text_vx> opt; std::vector<uint32_t> idx;
	texture_cb* rcb = 0;
	void* tex = 0;
	void* rptr = 0;
	glm::ivec4 _view = { 0,0,1024,1024 };	// 视口，超出范围部分不会渲染
	void* cctx = 0;
	uint32_t color = 0;
	float stwidth = 2.0;
	bool tex_batch = true;
public:
	canvas2d_t();
	~canvas2d_t();
	void set_renderer(void* renderer, texture_cb* cb, const glm::ivec4& view);
	// 初始化矢量图渲染器，输入vk设备
	void init_vgdev(dev_info_cx* d, int sample = 8);
	// 创建vkvg surface，输入宽高
	void* new_surface(int width, int height);
	void free_surface(void* surface);
	// begin/end渲染，输入surface指针，返回渲染上下文指针，中间可以调用vkvg的渲染函数
	void* ctx_begin(void* surface);
	void ctx_end(void* ctx);

	void update(void* surface, float delta);
	void draw_surface(void* surface, const glm::vec2& pos, const glm::ivec4& rc, const glm::ivec2& size);
	void draw_rvg(rvg_data_cx* dst);
	// 富文本渲染
	void update(rich_text_t* p, float delta);
	void draw_textdata(rich_text_t* p, const glm::vec2& pos);
	// 批量渲染矢量图、位图、文本
	void update_rvg(rvg_cx* rvg, rvg_data_cx* dst);
	// 释放渲染器的纹理
	void free_tex();
};

class clicprect_cx
{
public:
	texture_cb* rcb = 0;
	void* _renderer = 0;
	glm::ivec4 oldrc = {};
public:
	clicprect_cx(void* renderer, texture_cb* cb, const glm::ivec4& rc);
	~clicprect_cx();
};


//void r_update_data_text(text_render_o* p, canvas2d_t* pt, float delta);
//// 渲染一段文本
//void r_render_data_text(text_render_o* p, const glm::vec2& pos, canvas2d_t* pt);
//void test_drawvkvg(VkvgContext ctx, VkvgSurface surf, bspline_ct* bs, const char* filename);

// 区域是否相交
bool check_rect_cross(const glm::vec4& r1, const glm::vec4& r2);

