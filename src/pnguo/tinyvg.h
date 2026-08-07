#pragma once
#ifndef VG_H
#define VG_H 

struct image_ptr_t
{
	int width = 0, height = 0;
	int type = 0;				// 0=rgba，1=bgra
	int stride = 0;
	uint32_t* data = 0;			// 像素数据
	//void* texid = 0;			// 纹理指针，由调用方自动生成管理
	void* ptr = 0;				// 用户数据
	int comp = 4;				// 通道数0单色位图，1灰度图，4rgba/bgra
	int  blendmode = 0;			// 混合模式
	bool static_tex = false;	// 静态纹理
	bool multiply = false;		// 预乘的纹理
	bool valid = false;			// 是否更新到纹理
};

struct quadratic_v
{
	glm::vec2 p0, p1, p2;
};
struct cubic_v
{
	glm::vec2 p0, p1, p2, p3;	// p1 p2是控制点
};
/*
用 bezier curve（贝塞尔曲线） 来设置 color stop（颜色渐变规则），
这里使用下面的曲线形式，其中
X轴为 offset（偏移量，取值范围为 0~1，0 代表阴影绘制起点），
Y轴为 alpha（颜 色透明度，取值范围为0~1，0 代表完全透明），
*/
struct rect_shadow_t
{
	float radius = 4;	// 半径
	int segment = 6;	// 细分段
	glm::vec4 cfrom = { 0,0,0,0.8 }, cto = { 0.5,0.5,0.5,0.5 };// 颜色从cf到ct
	/*	cubic
		X轴为 offset（偏移量，取值范围为 0~1，0 代表阴影绘制起点），
		Y轴为 alpha（颜 色透明度，取值范围为0~1，0 代表完全透明），
	*/
	cubic_v cubic = { {0.0,0.6},{0.5,0.39},{0.4,0.1},{1.0,0.0 } };
};

// 线，二阶曲线，三阶曲线
enum class path_type_e :uint32_t
{
	e_vmove = 1,// 移动
	e_vline,	// 直线
	e_vcurve,	// 二次曲线
	e_vcubic	// 三次曲线
};
struct path_vertex_t
{
	// 24字节
	glm::vec2 p, c, c1;
	// 4字节
	path_type_e type;
};
struct path_d {
	path_vertex_t* v = 0;
	size_t count = 0;
	glm::vec2 pos = {};
	float scale = 0;		// 整体缩放
	float scale_pos = 0.0;	// 缩放坐标
	int8_t flip_y = 0;
};

/*
	LINE_CAP_BUTT,0
	LINE_CAP_ROUND,1
	LINE_CAP_SQUARE2
	LINE_JOIN_MITER,0
	LINE_JOIN_ROUND,1
	LINE_JOIN_BEVEL2
*/

// 混合模式
enum class BLENDMODE_E :int {
	none = -1,
	normal = 0,	// 普通混合
	additive,
	multiply,
	modulate,
	screen
};
#ifndef d_doubleSided
#define d_doubleSided 0x01
#define d_depthTestEnable 0x02
#define d_depthWriteEnable 0x04
#define d_stencilTestEnable 0x08
#endif // !d_doubleSided
struct gem_info_s {
	uint8_t blendMode = 0;
	uint8_t topology = 0;
	uint8_t polygon = 0;
	uint8_t frontFace = 0;     // COUNTER_CLOCKWISE = 0, CLOCKWISE = 1,
	uint8_t cullMode = 0;      // NONE=0, FRONT=1, BACK=2, FRONT_AND_BACK=3
	uint8_t flags = 0;         // doubleSided, depthTestEnable, depthWriteEnable, stencilTestEnable
	uint8_t lineWidth = 1;
	uint8_t pad[1] = { 0 };
};


typedef void rvgctx_t;
typedef void rvg_path_t;
typedef void rvg_surface_t;
typedef void rvg_pattern_t;
struct text_style;
struct text_st;
struct image_r;

// 矢量接口
struct canvas_cb {
	rvgctx_t* ctx = 0;
	// ctx操作
	void (*set_dev)(rvgctx_t* ctx, void* vgdev);		// 绑定设备才能创建表面
	void (*set_fence)(rvgctx_t* ctx, bool enable);
	void (*set_glutess)(rvgctx_t* ctx, bool enable);
	void (*begin_frame)(rvgctx_t* ctx);
	void (*end_frame)(rvgctx_t* ctx);
	void (*draw)(rvgctx_t* ctx, void* ctxvg, void** waitSemaphore, void** signalSemaphore);
	// 路径操作
	rvg_path_t* (*new_path)(rvgctx_t* ctx);
	void(*path_destroy)(rvg_path_t* path);
	rvg_path_t* (*get_path)(rvgctx_t* ctx);
	void(*clear_path)(rvg_path_t* path);
	void(*close_path)(rvg_path_t* path);
	void(*new_sub_path)(rvg_path_t* path);
	void(*path_extents)(rvg_path_t* path, float* x1, float* y1, float* x2, float* y2);
	void(*get_current_point)(rvg_path_t* path, float* x, float* y);
	// 添加数据到当前路径，参考path_type_e
	void(*add_path)(rvg_path_t* path, float* data, size_t count);
	void(*move_to)(rvg_path_t* path, float x, float y);
	void(*rel_move_to)(rvg_path_t* path, float x, float y);
	void(*line_to)(rvg_path_t* path, float x, float y);
	void(*rel_line_to)(rvg_path_t* path, float dx, float dy);
	void(*arc)(rvg_path_t* path, float xc, float yc, float radius, float a1, float a2);
	void(*arc_negative)(rvg_path_t* path, float xc, float yc, float radius, float a1, float a2);
	void(*curve_to)(rvg_path_t* path, float x1, float y1, float x2, float y2, float x3, float y3);
	void(*rel_curve_to)(rvg_path_t* path, float x1, float y1, float x2, float y2, float x3, float y3);
	void(*quadratic_to)(rvg_path_t* path, float x1, float y1, float x2, float y2);
	void(*rel_quadratic_to)(rvg_path_t* path, float x1, float y1, float x2, float y2);
	void(*rectangle)(rvg_path_t* path, float x, float y, float w, float h);
	void(*rounded_rectangle)(rvg_path_t* path, float x, float y, float w, float h, float radius);
	void(*rounded_rectangle2)(rvg_path_t* path, float x, float y, float w, float h, float rx, float ry);
	void(*ellipse)(rvg_path_t* path, float radiusX, float radiusY, float x, float y, float rotationAngle);
	void(*elliptic_arc_to)(rvg_path_t* path, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);
	void(*rel_elliptic_arc_to)(rvg_path_t* path, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);

	// 渲染操作
	void(*set_path)(rvgctx_t* ctx, rvg_path_t* path);
	void(*stroke)(rvgctx_t* ctx);
	void(*stroke_preserve)(rvgctx_t* ctx);
	void(*fill)(rvgctx_t* ctx);
	void(*fill_preserve)(rvgctx_t* ctx);
	void(*paint)(rvgctx_t* ctx);			// 全屏渲染
	void(*clear)(rvgctx_t* ctx);			// 清空画布
	void(*reset_clip)(rvgctx_t* ctx);		// 重置裁剪
	void(*clip)(rvgctx_t* ctx);				// 路径裁剪，清空当前路径
	void(*clip_preserve)(rvgctx_t* ctx);	// 路径裁剪
	void(*scissor)(rvgctx_t* ctx, int x, int y, int width, int height);	// 矩形裁剪，不受状态影响
	// 配置
	void(*set_opacity)(rvgctx_t* ctx, float opacity);
	void(*set_source_color)(rvgctx_t* ctx, uint32_t c);
	void(*set_source_rgba)(rvgctx_t* ctx, float r, float g, float b, float a);
	void(*set_source_rgb)(rvgctx_t* ctx, float r, float g, float b);
	void(*set_line_width)(rvgctx_t* ctx, float width);
	void(*set_miter_limit)(rvgctx_t* ctx, float limit);
	void(*set_line_cap)(rvgctx_t* ctx, int cap);
	void(*set_line_join)(rvgctx_t* ctx, int join);
	void(*set_source_surface)(rvgctx_t* ctx, rvg_surface_t* surf, float x, float y);
	void(*set_source)(rvgctx_t* ctx, rvg_pattern_t* pat);
	void(*set_operator)(rvgctx_t* ctx, int op);
	void(*set_fill_rule)(rvgctx_t* ctx, int fr);
	void(*set_dash)(rvgctx_t* ctx, const float* dashes, uint32_t num_dashes, float offset);		// 虚线
	void(*set_dash8)(rvgctx_t* ctx, uint64_t dashes, uint32_t num_dashes, float offset);								// 虚线,用uint8_t v8[8]表示
	void(*save)(rvgctx_t* ctx);
	void(*restore)(rvgctx_t* ctx);
	void(*translate)(rvgctx_t* ctx, float dx, float dy);
	void(*scale)(rvgctx_t* ctx, float sx, float sy);
	void(*rotate)(rvgctx_t* ctx, float radians);
	void(*transform)(rvgctx_t* ctx, const void* matrix);
	void(*set_matrix)(rvgctx_t* ctx, const void* matrix);
	void(*get_matrix)(rvgctx_t* ctx, void* matrix);
	void(*identity_matrix)(rvgctx_t* ctx);

	// 图案：渐变/图片
	rvg_surface_t* (*new_surface)(rvgctx_t* ctx, int width, int height, uint32_t* data, int stride, int type);	// stride宽度，type: 0 rgba, 1 bgra
	rvg_surface_t* (*new_surface_vk)(rvgctx_t* ctx, int width, int height, void* vkimage, int format);						// 输入vkimage做源
	rvg_pattern_t* (*new_pattern_linear)(rvgctx_t* ctx, float x0, float y0, float x1, float y1);
	rvg_pattern_t* (*new_pattern_radial)(rvgctx_t* ctx, float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse);
	rvg_pattern_t* (*new_pattern_sweep)(rvgctx_t* ctx, float cx, float cy, float start_angle, float end_angle);
	int (*pattern_add_color_stop)(rvg_pattern_t* pat, float o, float r, float g, float b, float a);
	int (*pattern_set_color_stop)(rvg_pattern_t* pat, int idx, float o, float r, float g, float b, float a);
	void(*pattern_set_matrix)(rvg_pattern_t* pat, const void* matrix);	// mat3x2
	void(*pattern_set_extend)(rvg_pattern_t* pat, int extend);
	void(*pattern_set_filter)(rvg_pattern_t* pat, int filter);
	void(*surface_destroy)(rvg_surface_t* surf);
	void(*pattern_destroy)(rvg_pattern_t* pat);

	// 添加文本，风格
	void (*add_text)(void* ctx, text_st* p, text_style* ts);
	// 普通图片，支持九宫格、混合颜色
	void (*add_image)(void* ctx, image_r* r);
	// 原始三角形
	void (*geom_set_state)(void* ctx, gem_info_s* info);
	void (*geom_set_matrix)(void* ctx, const glm::mat4* matrix);
	// 添加几何数据到缓冲区，xy顶点坐标，color顶点颜色，uv顶点纹理坐标，indices索引数据，color_type=0表示float4，1表示uint32_t
	void (*geom_add_geometry)(void* ctx, void* texture, const float* xy, int xy_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type);
	// 添加3D几何数据到缓冲区，xyz顶点坐标，color顶点颜色（双面则要双倍），uv顶点纹理坐标，indices索引数据
	void (*geom_add_geometry3d)(void* ctx, void* texture, const float* xyz, int xyz_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type);

};


#endif // !VG_H




