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

#include <clipper2/clipper.h> 
using namespace Clipper2Lib;

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
		void(*vkvg_matrix_init_identity)(vkvg_matrix_t* matrix);
		void(*vkvg_matrix_init)(vkvg_matrix_t* matrix, float xx, float yx, float xy, float yy, float x0, float y0);
		void(*vkvg_matrix_init_translate)(vkvg_matrix_t* matrix, float tx, float ty);
		void(*vkvg_matrix_init_scale)(vkvg_matrix_t* matrix, float sx, float sy);
		void(*vkvg_matrix_init_rotate)(vkvg_matrix_t* matrix, float radians);
		void(*vkvg_matrix_translate)(vkvg_matrix_t* matrix, float tx, float ty);
		void(*vkvg_matrix_scale)(vkvg_matrix_t* matrix, float sx, float sy);
		void(*vkvg_matrix_rotate)(vkvg_matrix_t* matrix, float radians);
		void(*vkvg_matrix_multiply)(vkvg_matrix_t* result, const vkvg_matrix_t* a, const vkvg_matrix_t* b);
		void(*vkvg_matrix_transform_distance)(const vkvg_matrix_t* matrix, float* dx, float* dy);
		void(*vkvg_matrix_transform_point)(const vkvg_matrix_t* matrix, float* x, float* y);
		vkvg_status_t(*vkvg_matrix_invert)(vkvg_matrix_t* matrix);
		void(*vkvg_matrix_get_scale)(const vkvg_matrix_t* matrix, float* sx, float* sy);
		void(*vkvg_device_set_context_cache_size)(VkvgDevice dev, uint32_t maxCount);
		VkvgDevice(*vkvg_device_create)(vkvg_device_create_info_t* info);
		void(*vkvg_device_destroy)(VkvgDevice dev);
		vkvg_status_t(*vkvg_device_status)(VkvgDevice dev);
		VkvgDevice(*vkvg_device_reference)(VkvgDevice dev);
		uint32_t(*vkvg_device_get_reference_count)(VkvgDevice dev);
		void(*vkvg_device_set_dpy)(VkvgDevice dev, int hdpy, int vdpy);
		void(*vkvg_device_get_dpy)(VkvgDevice dev, int* hdpy, int* vdpy);
		void(*vkvg_get_required_instance_extensions)(const char** pExtensions, uint32_t* pExtCount);
		vkvg_status_t(*vkvg_get_required_device_extensions)(VkPhysicalDevice phy, const char** pExtensions, uint32_t* pExtCount);
		const void* (*vkvg_get_device_requirements)(VkPhysicalDeviceFeatures* pEnabledFeatures);
		VkvgSurface(*vkvg_surface_create)(VkvgDevice dev, uint32_t width, uint32_t height);
		VkvgSurface(*vkvg_surface_create_from_image)(VkvgDevice dev, const char* filePath);
		VkvgSurface(*vkvg_surface_create_for_VkhImage)(VkvgDevice dev, void* vkhImg);
		VkvgSurface(*vkvg_surface_create_from_bitmap)(VkvgDevice dev, unsigned char* img, uint32_t width, uint32_t height);
		vkvg_status_t(*vkvg_surface_status)(VkvgSurface surf);
		VkvgSurface(*vkvg_surface_reference)(VkvgSurface surf);
		uint32_t(*vkvg_surface_get_reference_count)(VkvgSurface surf);
		void(*vkvg_surface_destroy)(VkvgSurface surf);

		void(*vkvg_surface_clear)(VkvgSurface surf);
		VkImage(*vkvg_surface_get_vk_image)(VkvgSurface surf);
		VkFormat(*vkvg_surface_get_vk_format)(VkvgSurface surf);
		uint32_t(*vkvg_surface_get_width)(VkvgSurface surf);
		uint32_t(*vkvg_surface_get_height)(VkvgSurface surf);
		vkvg_status_t(*vkvg_surface_write_to_png)(VkvgSurface surf, const char* path);
		vkvg_status_t(*vkvg_surface_write_to_memory)(VkvgSurface surf, unsigned char* const bitmap);
		void(*vkvg_surface_resolve)(VkvgSurface surf);

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
		float(*vkvg_get_miter_limit)(VkvgContext ctx);
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

		void(*vkvg_select_font_face)(VkvgContext ctx, const char* name);
		void(*vkvg_load_font_from_path)(VkvgContext ctx, const char* path, const char* name);
		void(*vkvg_load_font_from_memory)(VkvgContext ctx, unsigned char* fontBuffer, long fontBufferByteSize, const char* name);
		void(*vkvg_set_font_size)(VkvgContext ctx, uint32_t size);
		void(*vkvg_show_text)(VkvgContext ctx, const char* utf8);
		void(*vkvg_text_extents)(VkvgContext ctx, const char* utf8, vkvg_text_extents_t* extents);
		void(*vkvg_font_extents)(VkvgContext ctx, vkvg_font_extents_t* extents);
		VkvgText(*vkvg_text_run_create)(VkvgContext ctx, const char* text);
		VkvgText(*vkvg_text_run_create_with_length)(VkvgContext ctx, const char* text, uint32_t length);
		void(*vkvg_text_run_destroy)(VkvgText textRun);
		void(*vkvg_show_text_run)(VkvgContext ctx, VkvgText textRun);
		void(*vkvg_text_run_get_extents)(VkvgText textRun, vkvg_text_extents_t* extents);
		uint32_t(*vkvg_text_run_get_glyph_count)(VkvgText textRun);
		void(*vkvg_text_run_get_glyph_position)(VkvgText textRun, uint32_t index, vkvg_glyph_info_t* pGlyphInfo);
		vkvg_status_t(*vkvg_pattern_status)(VkvgPattern pat);
		VkvgPattern(*vkvg_pattern_reference)(VkvgPattern pat);
		uint32_t(*vkvg_pattern_get_reference_count)(VkvgPattern pat);
		VkvgPattern(*vkvg_pattern_create_for_surface)(VkvgSurface surf);
		VkvgPattern(*vkvg_pattern_create_linear)(float x0, float y0, float x1, float y1);
		vkvg_status_t(*vkvg_pattern_edit_linear)(VkvgPattern pat, float x0, float y0, float x1, float y1);
		vkvg_status_t(*vkvg_pattern_get_linear_points)(VkvgPattern pat, float* x0, float* y0, float* x1, float* y1);
		VkvgPattern(*vkvg_pattern_create_radial)(float cx0, float cy0, float radius0, float cx1, float cy1, float radius1);
		vkvg_status_t(*vkvg_pattern_edit_radial)(VkvgPattern pat, float cx0, float cy0, float radius0, float cx1, float cy1, float radius1);
		vkvg_status_t(*vkvg_pattern_get_color_stop_count)(VkvgPattern pat, uint32_t* count);
		vkvg_status_t(*vkvg_pattern_get_color_stop_rgba)(VkvgPattern pat, uint32_t index, float* offset, float* r, float* g, float* b, float* a);
		void(*vkvg_pattern_destroy)(VkvgPattern pat);
		vkvg_status_t(*vkvg_pattern_add_color_stop)(VkvgPattern pat, float offset, float r, float g, float b, float a);
		void(*vkvg_pattern_set_extend)(VkvgPattern pat, vkvg_extend_t extend);
		void(*vkvg_pattern_set_filter)(VkvgPattern pat, vkvg_filter_t filter);
		vkvg_extend_t(*vkvg_pattern_get_extend)(VkvgPattern pat);
		vkvg_filter_t(*vkvg_pattern_get_filter)(VkvgPattern pat);
		vkvg_pattern_type_t(*vkvg_pattern_get_type)(VkvgPattern pat);
		void(*vkvg_pattern_set_matrix)(VkvgPattern pat, const vkvg_matrix_t* matrix);
		void(*vkvg_pattern_get_matrix)(VkvgPattern pat, vkvg_matrix_t* matrix);
		void(*vkvg_set_source_color_name)(VkvgContext ctx, const char* color);
		void(*vkvg_start_recording)(VkvgContext ctx);
		VkvgRecording(*vkvg_stop_recording)(VkvgContext ctx);
		void(*vkvg_replay)(VkvgContext ctx, VkvgRecording rec);
		void(*vkvg_replay_command)(VkvgContext ctx, VkvgRecording rec, uint32_t cmdIndex);
		void(*vkvg_recording_get_command)(VkvgRecording rec, uint32_t cmdIndex, uint32_t* cmd, void** dataOffset);
		uint32_t(*vkvg_recording_get_count)(VkvgRecording rec);
		void* (*vkvg_recording_get_data)(VkvgRecording rec);
		void(*vkvg_recording_destroy)(VkvgRecording rec);
	};

#ifdef __cplusplus
}
#endif


template<typename T>
concept Drawable = requires(T obj) {
	{ obj.cmd_draw() } -> std::same_as<void>; // 要求有 draw() 方法 
	{ obj.update(float{}) } -> std::same_as<int>;
	{ obj.press_test() } -> std::same_as<bool>;
	{ obj.hittest(glm::ivec2{}) } -> std::same_as<bool>;
};
void render_drawable(Drawable auto& drawable);
int render_update(Drawable auto& drawable, float delta);
int render_build(Drawable auto& drawable);


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
//using PathsD = std::vector<std::vector<double>>;
#endif // !CLIPPER_CORE_H

struct path_d;
struct image_block;
struct image_ptr_t;
struct text_style;
union fitem_t;
struct dev_info_cx;
struct vg_style_data;
struct font_family_t;
struct rich_text_t;
struct multi_rich_text_t;
struct box_text_d;
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
	int type = 0;		// 0矢量图，1文本/位图
	int index = 0;
};
struct surface_ctx {
	void* surface;
	void* ctx;
};
struct draw_cmd
{
	glm::ivec4 clip_rect;
	void* tex_ref;			// image_ptr_t*或vgsurface
	unsigned int vtx_first;
	unsigned int idx_first;
	unsigned int elemCount;
};
// 原始三角形数据
struct geometry_d {
	void* image = 0;		// 可选
	glm::ivec4 clip = {};	// 裁剪区域
	text_vx* vdata = 0;		// 顶点数据
	uint32_t* idx = 0;		// 索引数据
	int vcount = 0;			// 顶点数量
	int icount = 0;			// 索引数量
	draw_cmd* cmds = 0;		// 命令（可选）
	int cmd_count = 0;		// 命令数量
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
	vkvgctx* ctx1 = 0;
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
	void wait_dev();
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

vkvg_dev* new_vgdev_cx(dev_info_cx* d, int sample = 8);
void free_surface(void* surface);
// begin/end渲染，输入surface指针，返回渲染上下文指针，中间可以调用vkvg的渲染函数
void* ctx_begin(void* surface);
void ctx_end(void* ctx);

struct grid_info_t
{
	int width = 200;
	int count = 10;
	int linewidth = 1;
	uint32_t back_color = 0xff353535;
	uint32_t inline_color = 0xff404040;
	uint32_t line_color = 0xff181818;
};
void draw_grid(void* ctx, grid_info_t* t, vkvg_func_t* func);


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
	glm::vec4 clip;		// 裁剪区域
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

struct stack_item
{
	glm::vec2 pos = {};				// 当前偏移
	glm::vec4 clip = {};			// 当前裁剪区域
	glm::vec2 scale = { 1.0f, 1.0f };
	float rotate_radians = 0.0f;
	glm::mat3x2 _transform = glm::mat3x2(1.0f);
};

struct cmdview_v {
	glm::ivec4 rc = {};	// 渲染区域
	size_t inc = 0;
	size_t dcv_index = 0;
};
struct rvgraw_t {
	glm::ivec2 pos = {};
	size_t cmd_count = 0;
	size_t view_count = 0;
	size_t datasize = 0;
	uint8_t* cmdtype = 0;	// cmd_count
	size_t* cmdpos = 0;		// cmd_count
	uint8_t* cmddata = 0;
	cmdview_v* view = 0;
};

class rvg_cx
{
public:
	enum Opcode : uint8_t {
		OP_NULL, OP_VIEW, OP_VIEW_POP,
		OP_SUBMIT_STYLE, OP_SUBMIT_COLOR, OP_GRID_FILL, OP_LINEAR_FILL, OP_ADD_ARROW,
		OP_DRAW_BLOCK, OP_DRAW_PATH, OP_ADD_LINE_PTR, OP_ADD_RECT_DOUBLE,
		OP_ADD_RECT_VEC4, OP_ADD_CIRCLE, OP_ADD_ELLIPSE, OP_ADD_TRIANGLE, OP_POLYLINE_VEC2,
		OP_ADD_POLYLINE_PATH, OP_ADD_POLYLINE_VEC2_PTR, OP_POLYLINES,
		OP_PAINT_SHADOW, OP_TRANSLATE, OP_SCALE, OP_ROTATE, OP_TRANSFORM, OP_SET_MATRIX, OP_CLIP, OP_SAVE, OP_RESTORE, OP_FILL, OP_STROKE, OP_FILL_PRESERVE, OP_STROKE_PRESERVE,
		OP_SET_LINE_WIDTH, OP_SET_COLOR_UINT, OP_SET_COLOR_VEC4,
		OP_TEXT_STYLE, OP_ADD_TEXT, OP_ADD_IMAGE, OP_ADD_GEOMETRY,
		OP_MAX_COUNT
	};

	std::vector<Opcode> _cmdtype;		// 操作类型
	std::vector<uint8_t> _cmd;			// 命令数据
	std::vector<size_t> _cmd_pos;		// 命令的数据偏移
	std::vector<cmdview_v> _view;		// 视图列表
	glm::ivec2 pos = {};
private:
	std::stack<stack_item> _stk;
	stack_item _cur = {};				// 当前状态
	glm::ivec4 _tem_clip = {};
	glm::ivec4 _tview = {};				// 当前渲染区域 
public:
	rvg_cx();
	~rvg_cx();
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
	void add_polyline(const PathsD* p, bool closed);
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

	void clip();
	void clip(const glm::ivec4& c);
	void save();
	void restore();
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

	void push_ct(uint8_t op);
	bool is_image();
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
	void push_view(const glm::ivec4& c);
	void pop_view();
	void push_null(int v);
	uint32_t get_crc();
public:
	void save_file(const char* fn);
	bool load_file(const char* fn);
private:

};

struct translate_cc
{
	void* surface = 0;
	void* ctx = 0;
	glm::vec2 apos = {};
	glm::vec2 clips = {};
};
struct gdata_ptr
{
	union
	{
		box_text_d* t = 0;	// 位图或文本
		d2_rt* d2;			// 矢量图
		geometry_d* geo;	// 三角形数据
	}v;
	size_t first = 0;	// 第一个位置
	size_t count = 0;	// 渲染数量
	int type = 0;		// 0文本/位图，1矢量图，2三角形
};
class rvg_data_cx
{
public:
	rvg_cx* d = 0;						// 矢量图/文本/位图渲染命令
	multi_rich_text_t* mrt = 0;			// 文本渲染管理器
	packer_base* packer = 0;			// 矩形打包器
	glm::ivec2 max_rect = {};			// 最大的区域
	std::vector<d2_rt> dcv;				// 矢量缓存信息 
	std::vector<surface_ctx> surfaces;
	std::vector<gdata_ptr> dst_data;	// 使用rvg_cx生成渲染数据列表
	glm::ivec4 _view = {};
	glm::ivec2 _pos = {};
	float stwidth = 2.0;
	uint32_t cmd_crc = 0;
	bool mix_text = true;
public:
	rvg_data_cx();
	~rvg_data_cx();
	rvg_cx* get();
	void update();
	translate_cc get_ctx(size_t idx, const glm::ivec4& rc);
private:

};
class form_x;
class div_cx;
// 输入矢量图返回渲染数据
class drawable_cx
{
public:
	// 渲染的数据列表
	std::vector<rvg_data_cx*> _drawv;
	std::unordered_map<div_cx*, rvg_data_cx*> _dobj;
	// 文本渲染用
	std::map<image_ptr_t*, void*> _vt;
	std::map<void*, void*> _vgt;
	std::vector<text_vx> opt; std::vector<uint32_t> idx;
	vkvg_dev* _vgdev = 0;
	vkvg_func_t* vgcb = 0;
	texture_cb* rcb = 0;	// 渲染器接口
	void* rptr = 0;			// 渲染器指针
	void* tex = 0;
	font_family_t* familys = 0;				// 默认字体
	glm::ivec4 _view = { 0,0,1024,1024 };	// 视口，超出范围部分不会渲染
	void* cctx = 0;
	uint32_t color = 0;
	float stwidth = 2.0;
	bool tex_batch = true;
	bool _pause = false;
public:
	drawable_cx();
	~drawable_cx();
	void init(texture_cb* cb, const glm::ivec4& view, vkvg_dev* vgdev);
	void* new_surface(int width, int height);
	void* new_surface1(int width, int height);
	void update_surface(void* surface);
	void update_image(image_ptr_t* img);
	void draw_surface(void* surface, const glm::vec2& pos, const glm::ivec4& rc, const glm::ivec2& size);
	void draw_rvg(rvg_data_cx* dst);
	// 富文本渲染
	void update_text(rich_text_t* p, float delta);
	void draw_textdata(rich_text_t* p, const glm::vec2& pos);
	void draw_boxtext(box_text_d* p, const glm::vec2& pos);
	// 渲染原始三角形数据，opt顶点数据，idx索引数据。image{image_ptr_t*或surface}。提前创建对应纹理
	void draw_geometry(void* image, const glm::ivec4& clip, std::vector<text_vx>* v, std::vector<uint32_t>* idx);
	void draw_geometry(geometry_d* geo);
	// 批量生成渲染矢量图、位图、文本
	bool update_rvgdata(rvg_data_cx* dst);
	// 释放渲染器的纹理
	void free_tex();
	void free_rvg(rvg_data_cx* p);
	glm::ivec2 get_size();
	void* get_texture(void* surface);
	// 暂停渲染
	void pause();
public:
	void cmd_draw();
	void clear_draw();
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
class dom_cx
{
public:
	drawable_cx dc = {};
	// 控件列表
	std::vector<div_cx*> widgets, tdrawlist, tadd, tremove;
	form_x* form0 = 0;
public:
	dom_cx();
	~dom_cx();
	void init(form_x* f, texture_cb* cb, const glm::ivec4& view, vkvg_dev* vgdev, font_family_t* familys);

	void add_widget(div_cx* w);
	void remove_widget(div_cx* w);
	bool press_test();
	bool hittest(const glm::ivec2& mpos);

	int update(float delta);
	int build();
	// 暂停渲染
	void pause();
	void cmd_draw();
private:

};


//void r_update_data_text(text_render_o* p, drawable_cx* pt, float delta);
//// 渲染一段文本
//void r_render_data_text(text_render_o* p, const glm::vec2& pos, drawable_cx* pt);
//void test_drawvkvg(VkvgContext ctx, VkvgSurface surf, bspline_ct* bs, const char* filename);

// 区域是否相交
bool check_rect_cross(const glm::vec4& r1, const glm::vec4& r2);

// 单色矩形
size_t gen_rect_color(const glm::ivec4& rc, const glm::vec4& color, std::vector<text_vx>& vtx, std::vector<uint32_t>& idx);
// 四个角不同颜色
size_t gen_rect_mcolor(const glm::ivec4& rc, const glm::ivec4& color, std::vector<text_vx>& vtx, std::vector<uint32_t>& idx);
size_t gen_rect_mcolor(const glm::ivec4& rc, const glm::vec4* color, std::vector<text_vx>& vtx, std::vector<uint32_t>& idx);

// 获取色盘颜色
glm::vec4 get_color_cb(const glm::ivec2& pos, const glm::ivec2& size, float h);
// 获取色调颜色
glm::vec4 get_hue_color_cb(const glm::ivec2& pos, const glm::ivec2& size);
size_t build_huedata(const glm::vec4& incolor, const glm::ivec4& rc0, std::vector<text_vx>& vtx, std::vector<uint32_t>& idx);
