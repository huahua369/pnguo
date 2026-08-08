#pragma once
#include <cstdint>


struct quadratic_v_t
{
	vec2 p0, p1, p2;
};
struct cubic_v_t
{
	vec2 p0, p1, p2, p3;	// p1 p2是控制点
};
/*
用 bezier curve（贝塞尔曲线） 来设置 color stop（颜色渐变规则），
这里使用下面的曲线形式，其中
X轴为 offset（偏移量，取值范围为 0~1，0 代表阴影绘制起点），
Y轴为 alpha（颜 色透明度，取值范围为0~1，0 代表完全透明），
*/
struct rect_shadow_vt
{
	float radius = 4;	// 半径
	int segment = 6;	// 细分段
	vec4 cfrom = { 0,0,0,0.8 }, cto = { 0.5,0.5,0.5,0.5 };// 颜色从cf到ct
	/*	cubic
		X轴为 offset（偏移量，取值范围为 0~1，0 代表阴影绘制起点），
		Y轴为 alpha（颜 色透明度，取值范围为0~1，0 代表完全透明），
	*/
	cubic_v_t cubic = { {0.0,0.6},{0.5,0.39},{0.4,0.1},{1.0,0.0 } };
};

// 线，二阶曲线，三阶曲线
enum class path_type_et :uint32_t
{
	e_vmove = 1,// 移动
	e_vline,	// 直线
	e_vcurve,	// 二次曲线
	e_vcubic	// 三次曲线
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
enum class blendMode_e :int8_t {
	none = -1,
	normal = 0,	// 普通混合
	additive,
	multiply,
	modulate,
	screen
};
enum vg_extend_t :uint8_t {
	VG_EXTEND_NONE,    /*!< nothing will be outputed outside the pattern original bounds */
	VG_EXTEND_REPEAT,  /*!< pattern will be repeated to fill all the target bounds */
	VG_EXTEND_REFLECT, /*!< pattern will be repeated but mirrored on each repeat */
	VG_EXTEND_PAD      /*!< the last pixels making the borders of the pattern will be extended to the whole target */
};

enum vg_filter_t :uint8_t {
	VG_FILTER_FAST,
	VG_FILTER_GOOD,
	VG_FILTER_BEST,
	VG_FILTER_NEAREST,
	VG_FILTER_BILINEAR,
	VG_FILTER_GAUSSIAN,
};
enum class vg_pattern_type_t :uint8_t {
	VG_PATTERN_TYPE_SOLID,         /*!< single color pattern */
	VG_PATTERN_TYPE_SURFACE,       /*!< vg surface pattern */
	VG_PATTERN_TYPE_LINEAR,        /*!< linear gradient pattern */
	VG_PATTERN_TYPE_RADIAL,        /*!< radial gradient pattern */
	VG_PATTERN_TYPE_MESH,          /*!< not implemented */
	VG_PATTERN_TYPE_RASTER_SOURCE, /*!< not implemented */
	VG_PATTERN_TYPE_SWEEP, /*!< 锥形渐变 */
};
enum vg_clip_state_t :uint8_t {
	vg_clip_state_none = 0x00,
	vg_clip_state_clear = 0x01,
	vg_clip_state_clip = 0x02,
	vg_clip_state_clip_saved = 0x06,
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


struct push_constants_t {
	vec4          source;
	vec2          size;
	uint32_t      fsq_patternType;
	float         opacity;
	mat3x2 mat;
	mat3x2 matInv;
};
#define MAX_STOPS 32
struct vg_gradient_t {
	vec4 colors[MAX_STOPS];
	float stops[MAX_STOPS];
	vec4 cp[2];
	ivec4 m;
	vec2 scale;	// 缩放目标
	uint32_t count;
	int extend;
};
struct vg_surface_t;
struct font_family_t;

struct vg_pattern_t {
	int					status;
	uint32_t            references;
	vg_extend_t       extend;
	vg_filter_t       filter;
	vg_pattern_type_t	type;
	bool                hasMatrix;
	mat3x2			matrix;
	void* data;	// Surface指针或vg_gradient_t
};

struct vg_state_save_t {
	float		lineWidth;
	float		miterLimit;
	uint32_t	dashCount;  // value count in dash array, 0 if dash not set.
	float		dashOffset; // an offset for dash
	float* dashes;     // an array of alternate lengths of on and off stroke.
	uint8_t		curOperator;
	uint8_t		lineCap;
	uint8_t		lineJoin;
	uint8_t		curFillRule;
	push_constants_t	pushConsts;
	uint32_t			color;
	vg_pattern_t		pattern;
	vg_clip_state_t		clippingState;
	uint32_t			references = 1;
	bool aa = true;
};

struct ovg_image_r
{
	void* img;
	ivec4 rc;		// 所在纹理区域
	ivec4 sliced;	// 九宫格
	ivec2 dsize;	// 渲染大小
	ivec2 pos;		// 渲染坐标
	uint32_t color;		// 混合颜色
	int8_t type;		// img的类型
};
struct text_st_t {
	vec2 pos;
	vec2 size;
	vec4 clip;		// 裁剪区域
	const char* text;
	int text_len;
};

// 文本样式
struct text_style_t
{
	font_family_t* family = 0;
	float fontsize = 0;
	float lineheight = 0;
	vec2 align = { 0.50,0.50 };	// 文本对齐
	vec2 shadow_pos = { 1.0,1.0 };
	int stroke = 0;						// 描边宽度
	uint32_t color = 0xffc2c2c2;		// 文本颜色
	uint32_t color_stroke = 0xff000000;	// 描边颜色
	uint32_t color_shadow = 0;			// 阴影颜色	0xcc121212;
	bool mcolor_effect = true;			// 是否启用彩色字体参与阴影描边效果
};
// 文本区域
struct text_box_rt {
	ivec4 rc = {};		// 设置文本渲染区域，偏移/大小
	vec2 text_align = { 0.0,0.0 };// 文本对齐
	int8_t auto_break = 0;	// 是否自动换行
	int8_t word_wrap = 0;	// 0字符换行，1单词换行，2换行点，3句子断开，4标题大小写断点
	int8_t ellipsis = 0;	// 省略号
};

// 内存资源分配器
struct mem_resource_t;
// 路径对象
struct ovg_path_t;
// 矢量对象
struct rvg_t;
// 渲染列表
struct drawlist_t;
// 接口
struct ovg_canvas_cb {
	mem_resource_t* ac = 0;
	// 路径操作
	ovg_path_t* (*new_path)(mem_resource_t* ac);
	void(*path_destroy)(ovg_path_t* path);
	void(*clear_path)(ovg_path_t* path);
	void(*close_path)(ovg_path_t* path);
	void(*new_sub_path)(ovg_path_t* path);
	void(*path_extents)(ovg_path_t* path, float* x1, float* y1, float* x2, float* y2);
	void(*get_current_point)(ovg_path_t* path, float* x, float* y);
	// 添加数据到当前路径，参考path_type_e
	void(*add_path)(ovg_path_t* path, float* data, size_t count);
	void(*add_path0)(ovg_path_t* path, ovg_path_t* src);
	void(*move_to)(ovg_path_t* path, float x, float y);
	void(*rel_move_to)(ovg_path_t* path, float x, float y);
	void(*line_to)(ovg_path_t* path, float x, float y);
	void(*rel_line_to)(ovg_path_t* path, float dx, float dy);
	void(*arc)(ovg_path_t* path, float xc, float yc, float radius, float a1, float a2);
	void(*arc_negative)(ovg_path_t* path, float xc, float yc, float radius, float a1, float a2);
	void(*curve_to)(ovg_path_t* path, float x1, float y1, float x2, float y2, float x3, float y3);
	void(*rel_curve_to)(ovg_path_t* path, float x1, float y1, float x2, float y2, float x3, float y3);
	void(*quadratic_to)(ovg_path_t* path, float x1, float y1, float x2, float y2);
	void(*rel_quadratic_to)(ovg_path_t* path, float x1, float y1, float x2, float y2);
	void(*rectangle)(ovg_path_t* path, float x, float y, float w, float h);
	void(*rounded_rectangle)(ovg_path_t* path, float x, float y, float w, float h, float radius);
	void(*rounded_rectangle2)(ovg_path_t* path, float x, float y, float w, float h, float rx, float ry);
	void(*ellipse)(ovg_path_t* path, float radiusX, float radiusY, float x, float y, float rotationAngle);
	void(*elliptic_arc_to)(ovg_path_t* path, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);
	void(*rel_elliptic_arc_to)(ovg_path_t* path, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);
	// 配置
	vg_state_save_t* (*new_state)(mem_resource_t* ac);
	void(*set_opacity)(vg_state_save_t* ctx, float opacity);
	void(*set_source_color)(vg_state_save_t* ctx, uint32_t c);
	void(*set_source_rgba)(vg_state_save_t* ctx, float r, float g, float b, float a);
	void(*set_source_rgb)(vg_state_save_t* ctx, float r, float g, float b);
	void(*set_line_width)(vg_state_save_t* ctx, float width);
	void(*set_miter_limit)(vg_state_save_t* ctx, float limit);
	void(*set_line_cap)(vg_state_save_t* ctx, int cap);
	void(*set_line_join)(vg_state_save_t* ctx, int join);
	void(*set_source_surface)(vg_state_save_t* ctx, vg_surface_t* surf, float x, float y);
	void(*set_source)(vg_state_save_t* ctx, vg_pattern_t* pat);
	void(*set_operator)(vg_state_save_t* ctx, int op);
	void(*set_fill_rule)(vg_state_save_t* ctx, int fr);
	void(*set_dash)(vg_state_save_t* ctx, const float* dashes, uint32_t num_dashes, float offset);		// 虚线
	void(*set_dash8)(vg_state_save_t* ctx, uint64_t dashes, uint32_t num_dashes, float offset);								// 虚线,用uint8_t v8[8]表示
	void(*translate)(vg_state_save_t* ctx, float dx, float dy);
	void(*scale)(vg_state_save_t* ctx, float sx, float sy);
	void(*rotate)(vg_state_save_t* ctx, float radians);
	void(*transform)(vg_state_save_t* ctx, const void* matrix);
	void(*set_matrix)(vg_state_save_t* ctx, const void* matrix);
	void(*get_matrix)(vg_state_save_t* ctx, void* matrix);
	void(*identity_matrix)(vg_state_save_t* ctx);

	// 图案：渐变/图片 
	vg_pattern_t* (*new_pattern_linear)(mem_resource_t* ac, float x0, float y0, float x1, float y1);
	vg_pattern_t* (*new_pattern_radial)(mem_resource_t* ac, float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse);
	vg_pattern_t* (*new_pattern_sweep)(mem_resource_t* ac, float cx, float cy, float start_angle, float end_angle);
	int (*pattern_add_color_stop)(vg_pattern_t* pat, float o, float r, float g, float b, float a);
	int (*pattern_set_color_stop)(vg_pattern_t* pat, int idx, float o, float r, float g, float b, float a);
	void(*pattern_set_matrix)(vg_pattern_t* pat, const void* matrix);	// mat3x2
	void(*pattern_set_extend)(vg_pattern_t* pat, int extend);
	void(*pattern_set_filter)(vg_pattern_t* pat, int filter);
	void(*pattern_destroy)(vg_pattern_t* pat);

	// 渲染操作，rvg_t可以多次执行fill或stroke/clip
	rvg_t* (*new_rvg)(mem_resource_t* ac);
	void(*set_path)(rvg_t* v, ovg_path_t* path, vg_state_save_t* st);
	void(*stroke)(rvg_t* v);
	void(*stroke_preserve)(rvg_t* v);
	void(*fill)(rvg_t* v);
	void(*fill_preserve)(rvg_t* v);
	void(*paint)(rvg_t* v);			// 全屏渲染
	void(*clear)(rvg_t* v);			// 清空画布
	void(*reset_clip)(rvg_t* v);	// 重置裁剪
	void(*clip)(rvg_t* v);			// 路径裁剪，清空当前路径
	void(*clip_preserve)(rvg_t* v);	// 路径裁剪
	void(*scissor)(rvg_t* v, int x, int y, int width, int height);	// 矩形裁剪，不受状态影响

	// 渲染列表
	drawlist_t* (*new_drawlist)(mem_resource_t* ac);
	// 添加矢量对象，dst渲染的坐标/宽高，rect为对象的区域坐标/宽高
	void (*add_vg)(drawlist_t* dc, rvg_t* v, const vec4* dst, const ivec4* rect);
	// 添加文本，风格，渲染区可选
	void (*add_text)(drawlist_t* dc, text_st_t* p, text_style_t* ts, text_box_rt* box);
	// 普通图片，支持九宫格、混合颜色
	void (*add_image)(drawlist_t* dc, ovg_image_r* r);
	// 原始三角形，输入0则不修改
	void (*geom_set_state)(drawlist_t* dc, gem_info_s* info, const mat4* matrix);
	// 添加几何数据到缓冲区，xy顶点坐标，color顶点颜色，uv顶点纹理坐标，indices索引数据，color_type=0表示float4，1表示uint32_t
	void (*geom_add_geometry)(drawlist_t* dc, void* texture, const float* xy, int xy_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type);
	// 添加3D几何数据到缓冲区，xyz顶点坐标，color顶点颜色（双面则要双倍），uv顶点纹理坐标，indices索引数据
	void (*geom_add_geometry3d)(drawlist_t* dc, void* texture, const float* xyz, int xyz_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type);

};

// 测试
void* new_gpu();

struct ovg_device_t;
struct ovg_ctx_t;
ovg_device_t* new_vkdevctx(VkDevice vkdev, VkPhysicalDevice phy, VkInstance instance);
void free_vkdevctx(ovg_device_t* dev);
ovg_ctx_t* new_ovgctx(ovg_device_t* dev, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlags samples);

ovg_canvas_cb* get_canvas_cb(ovg_ctx_t* ctx);
