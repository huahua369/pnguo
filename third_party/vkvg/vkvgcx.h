#pragma once

#include "vkvg.h"
struct style_path_t;
class path_v;

void draw_path_vkvg(void* cr, path_v* p, style_path_t* st);
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
	void set_matrix(const  glm::mat3* matrix);
	void set_matrix(const  glm::mat3x2* matrix);
	glm::mat3x2 get_matrix();
private:

};


class vkvg_dev
{
public:
	VkvgDevice dev = 0;
	vkvg_func_t* fun = {};
	VkSampleCountFlags samplecount = {};
public:
	vkvg_dev();
	~vkvg_dev();
	vkvg_func_t* get_fun();

	VkvgSurface new_surface(int width, int height);
	VkvgSurface new_surface(uint32_t* data, int width, int height);
	VkvgSurface new_surface(void* vkimg);
	VkvgSurface new_surface(const char* fn);
	void free_surface(VkvgSurface p);
	vkvg_ctx* new_ctx(VkvgSurface p);
	void free_ctx(vkvg_ctx* p);

	void* get_vkimage(void* surface);
	void image_save(void* surface, const char* fn);
	glm::ivec2 get_image_data(void* surface, std::vector<uint32_t>* opt);
private:

};

struct dev_info_c
{
	VkInstance inst;
	VkPhysicalDevice phy;
	VkDevice vkdev;
	uint32_t qFamIdx;		// familyIndex
	uint32_t qIndex = 0;
};
// 64,32,16,8,4,2,1
vkvg_dev* new_vkvgdev(dev_info_c* c = 0, int sample = 8);
void free_vkvgdev(vkvg_dev* p);
struct fill_style_d;
struct text_style_d;


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
 