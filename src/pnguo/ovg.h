#pragma once
/*
矢量/图片/文本/三角形录制到rvg_t对象
*/
#include <cstdint>


struct quadratic_v_t
{
	glm::vec2 p0, p1, p2;
};
struct cubic_v_t
{
	glm::vec2 p0, p1, p2, p3;	// p1 p2是控制点
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
	glm::vec4 cfrom = { 0,0,0,0.8 }, cto = { 0.5,0.5,0.5,0.5 };// 颜色从cf到ct
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

enum vg_line_cap_t :uint8_t {
	VG_LINE_CAP_BUTT,
	VG_LINE_CAP_ROUND,
	VG_LINE_CAP_SQUARE
};

enum vg_line_join_t :uint8_t {
	VG_LINE_JOIN_MITER,
	VG_LINE_JOIN_ROUND,
	VG_LINE_JOIN_BEVEL
};
enum vg_fill_rule_t {
	VG_FILL_RULE_EVEN_ODD,
	VG_FILL_RULE_NON_ZERO
};
enum vg_extend_t :uint8_t {
	VG_EXTEND_NONE,
	VG_EXTEND_REPEAT,
	VG_EXTEND_REFLECT,
	VG_EXTEND_PAD
};

enum vg_filter_t :uint8_t {
	VG_FILTER_FAST,
	VG_FILTER_GOOD,
	VG_FILTER_BEST,
	VG_FILTER_NEAREST,
	VG_FILTER_BILINEAR,
	VG_FILTER_GAUSSIAN,
};
enum vg_pattern_type_t :uint8_t {
	VG_PATTERN_TYPE_SOLID,        // 单色
	VG_PATTERN_TYPE_SURFACE,      // 纹理填充
	VG_PATTERN_TYPE_LINEAR,       // 线性渐变 /*!< linear gradient pattern */
	VG_PATTERN_TYPE_RADIAL,       // 径向渐变 /*!< radial gradient pattern */
	VG_PATTERN_TYPE_MESH,         // 网格渐变 /*!< not implemented */
	VG_PATTERN_TYPE_RASTER_SOURCE, //
	VG_PATTERN_TYPE_SWEEP,			// 锥形渐变 
};
enum vg_clip_state_t :uint8_t {
	vg_clip_state_none = 0x00,
	vg_clip_state_clear = 0x01,
	vg_clip_state_clip = 0x02,
	vg_clip_state_clip_saved = 0x06,
};

enum vg_operator_t :uint8_t {
	VG_OPERATOR_CLEAR,

	VG_OPERATOR_SOURCE,
	VG_OPERATOR_OVER,
	VG_OPERATOR_DIFFERENCE,
	VG_OPERATOR_MAX,
};

// 渲染命令
#if 1
struct push_constants_t {
	glm::mat3x2 mat;
	glm::mat3x2 matInv;
	glm::vec4 source;
	glm::vec2 size;
	uint32_t fsq_patternType;
	float opacity;
};
#define MAX_STOPS 32
struct vg_gradient_t {
	glm::vec4 colors[MAX_STOPS];
	float stops[MAX_STOPS];
	glm::vec4 cp[2];
	glm::ivec4 m;
	glm::vec2 scale;	// 缩放目标
	uint32_t count;
	int extend;
};
// 纹理表面，由后端提供
struct vg_surface_t;
struct font_family_t;

struct vg_pattern_t {
	int					status;
	uint32_t            references;
	vg_extend_t       extend;
	vg_filter_t       filter;
	vg_pattern_type_t	type;
	bool                hasMatrix;
	glm::mat3x2			matrix;
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
	vg_pattern_t* pattern;
	vg_clip_state_t		clippingState;
	uint32_t			references = 1;
	bool aa = true;
	bool glutessEnable = false;
};
enum class depth_stencil_State :uint8_t {
	d_depthtest_enable = 0x01,
	d_depthwrite_enable = 0x02,
	d_stenciltest_enable = 0x04
};
// 混合模式 
enum class blendMode_e :int {
	none = -1,	// 不混合
	normal = 0,	// 普通混合
	additive,
	multiply,
	modulate,
	screen,
	normal_prem,	// 预乘alpha
	additive_prem,
};
enum shader_type_e :uint8_t {
	ST_NONE,
	ST_MASK,
	ST_DOUBLESIDED,
	ST_INSTANCE,
	ST_INSTANCE_DOUBLESIDED,
};
// 0矢量图管线						2d
// 0普通三角形(纹理)					2d/3d		tex0
// 1普通三角形+遮罩纹理				2d/3d		tex0、tex1
// 2双面三角形(两种颜色/纹理)			doubleSided	tex0
// 3三角形(纹理)实例化							ubo0、tex1
// 4双面三角形(两种颜色/纹理)实例化				ubo0、tex1
struct gem_info_t {
	uint8_t blendMode = 0;
	uint8_t topology = 0;
	uint8_t polygon = 0;
	uint8_t frontFace = 0;	// COUNTER_CLOCKWISE = 0, CLOCKWISE = 1,
	uint8_t cullMode = 0;	// NONE=0, FRONT=1, BACK=2, FRONT_AND_BACK=3
	uint8_t flags = 0;		// depthTestEnable, depthWriteEnable, stencilTestEnable
	uint8_t lineWidth = 1;
	uint8_t shader = 0;		// shader_type_e
};

// 普通三角形命令
struct geom_cmd_t {
	int stype = 1;
	gem_info_t state = {};
	void* texture = nullptr;
	glm::mat4 mat = glm::mat4(1.0f);	// 矩阵
	float mask_time = 1.0;				// 遮罩时间
	uint32_t elemCount = 0;				// 元素计数，索引数量或顶点数量
	uint32_t firstIndex = 0;			// -1则非索引渲染
	int32_t  vertexOffset = 0;
	size_t v_offset = 0;				// vbo绑定偏移：0单面，1双面
};

// 矢量命令
struct vgcmd_t {
	int stype = 0;
	int full_screen_quad = 0;
	glm::ivec2 vertex = {};			// 顶点开始、数量
	glm::ivec2 index = {};			// 索引开始、数量
	vg_state_save_t* state = {};	// 渲染参数
	glm::vec4 bounds = {};			// 全屏填充,odd/clip专用
	int8_t type = 0;				// 类型：填充0、描边1、裁剪2、全屏3、清屏4
};
union gcmd_t {
	vgcmd_t vg;
	geom_cmd_t g;
};
struct ovgVertex {
	glm::vec2	pos;
	glm::vec2	uv;
	uint32_t	color;
};
struct geomVertex1 {
	glm::vec3 pos;
	glm::vec2 uv;
	uint32_t color;
};
struct geomVertex2 {
	glm::vec3 pos;
	glm::vec2 uv;
	uint32_t color;
	uint32_t color1;
};
struct ovg_draw_data {
	gcmd_t* d;				// 渲染命令列表
	size_t count;
	ovgVertex* vg_vertex;	// 矢量顶点
	size_t v_count;
	uint32_t* vg_indices;	// 矢量索引
	size_t i_count;
	size_t uboCount;		// 渐变ubo结构数量
	geomVertex1* vertex1;	// 单面顶点
	size_t v1_count;
	geomVertex2* vertex2;	// 双面顶点
	size_t v2_count;
	uint32_t* geom_indices;	// 索引 
	size_t g_count;
};

#endif

enum ImageFlipMode
{
	FLIP_NONE,			// 不翻转
	FLIP_HORIZONTAL,	// 水平翻转 
	FLIP_VERTICAL,		// 垂直翻转
	FLIP_HORIZONTAL_AND_VERTICAL = (FLIP_HORIZONTAL | FLIP_VERTICAL)    // 水平和垂直翻转（不是对角翻转）
};
struct ovg_image_r
{
	void* img;
	glm::ivec4 rc;		// 所在纹理区域
	glm::ivec4 sliced;	// 九宫格
	glm::ivec2 texsize;	// 纹理大小
	glm::ivec4 dst;		// 渲染坐标大小 
	uint32_t color;		// 混合颜色
	int8_t type;		// img的类型
};
struct text_st_t {
	glm::vec2 pos;
	glm::vec2 size;
	glm::vec4 clip;		// 裁剪区域
	const char* text;
	int text_len;
};

// 文本样式
struct text_style_t
{
	font_family_t* family = 0;
	float fontsize = 0;
	float lineheight = 0;
	glm::vec2 align = { 0.50,0.50 };	// 文本对齐
	glm::vec2 shadow_pos = { 1.0,1.0 };
	int stroke = 0;						// 描边宽度
	uint32_t color = 0xffc2c2c2;		// 文本颜色
	uint32_t color_stroke = 0xff000000;	// 描边颜色
	uint32_t color_shadow = 0;			// 阴影颜色	0xcc121212;
	bool mcolor_effect = true;			// 是否启用彩色字体参与阴影描边效果
};
// 文本区域
struct text_box_rt {
	glm::ivec4 rc = {};		// 设置文本渲染区域，偏移/大小
	glm::vec2 text_align = { 0.0,0.0 };// 文本对齐
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
// 录制
struct ovg_recording_t;

// 接口
struct ovg_canvas_cb {
	mem_resource_t* ac;
	// 路径操作
	ovg_path_t* (*new_path)(mem_resource_t* ac);
	void(*path_destroy)(ovg_path_t* path);
	void(*clear_path)(ovg_path_t* path);
	void(*close_path)(ovg_path_t* path);
	void(*new_sub_path)(ovg_path_t* path);
	void(*path_extents)(ovg_path_t* path, float* x1, float* y1, float* x2, float* y2);
	void(*get_current_point)(ovg_path_t* path, float* x, float* y);
	size_t(*get_segment_count)(ovg_path_t* path);
	void(*set_segment_color)(ovg_path_t* path, size_t idx, uint32_t color);
	// 添加数据到当前路径，参考path_type_e
	void(*add_path)(ovg_path_t* path, float* data, size_t count);
	void(*move_to)(ovg_path_t* path, float x, float y);
	void(*rel_move_to)(ovg_path_t* path, float x, float y);
	void(*line_to)(ovg_path_t* path, float x, float y);
	void(*rel_line_to)(ovg_path_t* path, float dx, float dy);
	void(*arc)(ovg_path_t* path, float xc, float yc, float radius, float a1, float a2);
	void(*arc_negative)(ovg_path_t* path, float xc, float yc, float radius, float a1, float a2);
	// 有缩放时，先执行set_path一次再执行curve_to
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
	void(*circle)(ovg_path_t* path, float x, float y, float radius);
	// 配置
	vg_state_save_t* (*new_state)(mem_resource_t* ac);
	void (*state_destroy)(vg_state_save_t* p);
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
	void (*destroy_rvg)(rvg_t* p);
	void(*clear)(rvg_t* v);			// 清空画布
	void(*set_path)(rvg_t* v, ovg_path_t* path, vg_state_save_t* st);// 绑定路径和状态
	void(*stroke)(rvg_t* v);
	void(*stroke_preserve)(rvg_t* v);
	void(*fill)(rvg_t* v);
	void(*fill_preserve)(rvg_t* v);
	void(*paint)(rvg_t* v);			// 全屏渲染
	void(*reset_clip)(rvg_t* v);	// 重置裁剪
	void(*clip)(rvg_t* v);			// 路径裁剪，清空当前路径
	void(*clip_preserve)(rvg_t* v);	// 路径裁剪
	void(*clip_rect)(rvg_t* v, int x, int y, int width, int height);	// 矩形裁剪
	void(*set_clip_rect)(rvg_t* v, void* rc);	// 矩形裁剪,int[4]
	void(*get_clip_rect)(rvg_t* v, void* rc);	// 获取矩形裁剪

	// 添加文本，风格，渲染区可选
	void (*add_text)(rvg_t* dc, text_st_t* p, text_style_t* ts, text_box_rt* box);
	// 普通图片，支持九宫格、混合颜色
	void (*add_image)(rvg_t* dc, ovg_image_r* r);
	// 原始三角形，输入0则不修改
	void (*set_geom_state)(rvg_t* dc, gem_info_t* info, const void* matrix4x4);
	// 添加几何数据到缓冲区，xy顶点坐标，color顶点颜色，uv顶点纹理坐标，indices索引数据，color_type=0表示float4，1表示uint32_t
	void (*add_geometry)(rvg_t* dc, vg_surface_t* texture, const float* xy, int xy_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type);
	// 添加3D几何数据到缓冲区，xyz顶点坐标，color顶点颜色（双面则要双倍），uv顶点纹理坐标，indices索引数据
	void (*add_geometry3d)(rvg_t* dc, vg_surface_t* texture, const float* xyz, int xyz_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type);

};

// 命令模式，没有destroy函数的对象都不需要手动释放
struct ovg_ctx_cb {
	mem_resource_t* ac;	// 内存分配器，由new_ctx_cb自己创建 	
	// 渲染操作，rvg_t可以多次执行fill或stroke/clip
	rvg_t* (*new_rvg)(mem_resource_t* ac);
	void (*destroy_rvg)(rvg_t* p);
	void(*clear)(rvg_t* v);			// 清空画布 
	// 路径操作
	ovg_path_t* (*get_path)(rvg_t* ctx);
	void (*new_path)(rvg_t* ctx);
	void(*clear_path)(rvg_t* ctx);
	void(*close_path)(rvg_t* ctx);
	void(*new_sub_path)(rvg_t* ctx);
	void(*path_extents)(rvg_t* ctx, float* x1, float* y1, float* x2, float* y2);
	void(*get_current_point)(rvg_t* ctx, float* x, float* y);
	size_t(*get_segment_count)(rvg_t* ctx);
	void(*set_segment_color)(rvg_t* ctx, size_t idx, uint32_t color);
	// 添加数据到当前路径，参考path_type_e
	void(*add_path)(rvg_t* ctx, float* data, size_t count);
	void(*move_to)(rvg_t* ctx, float x, float y);
	void(*rel_move_to)(rvg_t* ctx, float x, float y);
	void(*line_to)(rvg_t* ctx, float x, float y);
	void(*rel_line_to)(rvg_t* ctx, float dx, float dy);
	void(*arc)(rvg_t* ctx, float xc, float yc, float radius, float a1, float a2);
	void(*arc_negative)(rvg_t* ctx, float xc, float yc, float radius, float a1, float a2);
	// 有缩放时，先执行set_path一次再执行curve_to
	void(*curve_to)(rvg_t* ctx, float x1, float y1, float x2, float y2, float x3, float y3);
	void(*rel_curve_to)(rvg_t* ctx, float x1, float y1, float x2, float y2, float x3, float y3);
	void(*quadratic_to)(rvg_t* ctx, float x1, float y1, float x2, float y2);
	void(*rel_quadratic_to)(rvg_t* ctx, float x1, float y1, float x2, float y2);
	void(*rectangle)(rvg_t* ctx, float x, float y, float w, float h);
	void(*rounded_rectangle)(rvg_t* ctx, float x, float y, float w, float h, float radius);
	void(*rounded_rectangle2)(rvg_t* ctx, float x, float y, float w, float h, float rx, float ry);
	void(*ellipse)(rvg_t* ctx, float radiusX, float radiusY, float x, float y, float rotationAngle);
	void(*elliptic_arc_to)(rvg_t* ctx, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);
	void(*rel_elliptic_arc_to)(rvg_t* ctx, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);
	void(*circle)(rvg_t* ctx, float x, float y, float radius);
	// 配置状态
	void(*set_opacity)(rvg_t* ctx, float opacity);
	void(*set_source_color)(rvg_t* ctx, uint32_t c);
	void(*set_source_rgba)(rvg_t* ctx, float r, float g, float b, float a);
	void(*set_source_rgb)(rvg_t* ctx, float r, float g, float b);
	void(*set_line_width)(rvg_t* ctx, float width);
	void(*set_miter_limit)(rvg_t* ctx, float limit);
	void(*set_line_cap)(rvg_t* ctx, int cap);
	void(*set_line_join)(rvg_t* ctx, int join);
	void(*set_source_surface)(rvg_t* ctx, vg_surface_t* surf, float x, float y);
	void(*set_source)(rvg_t* ctx, vg_pattern_t* pat);
	void(*set_operator)(rvg_t* ctx, int op);
	void(*set_fill_rule)(rvg_t* ctx, int fr);
	void(*set_dash)(rvg_t* ctx, const float* dashes, uint32_t num_dashes, float offset);		// 虚线
	void(*set_dash8)(rvg_t* ctx, uint64_t dashes, uint32_t num_dashes, float offset);								// 虚线,用uint8_t v8[8]表示
	void(*translate)(rvg_t* ctx, float dx, float dy);
	void(*scale)(rvg_t* ctx, float sx, float sy);
	void(*rotate)(rvg_t* ctx, float radians);
	void(*transform)(rvg_t* ctx, const void* matrix);
	void(*set_matrix)(rvg_t* ctx, const void* matrix);
	void(*get_matrix)(rvg_t* ctx, void* matrix);
	void(*identity_matrix)(rvg_t* ctx);

	// 图案：渐变/图片 
	vg_pattern_t* (*new_pattern_linear)(rvg_t* ctx, float x0, float y0, float x1, float y1);
	vg_pattern_t* (*new_pattern_radial)(rvg_t* ctx, float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse);
	vg_pattern_t* (*new_pattern_sweep)(rvg_t* ctx, float cx, float cy, float start_angle, float end_angle);
	int (*pattern_add_color_stop)(vg_pattern_t* pat, float o, float r, float g, float b, float a);
	int (*pattern_set_color_stop)(vg_pattern_t* pat, int idx, float o, float r, float g, float b, float a);
	void(*pattern_set_matrix)(vg_pattern_t* pat, const void* matrix);	// mat3x2
	void(*pattern_set_extend)(vg_pattern_t* pat, int extend);
	void(*pattern_set_filter)(vg_pattern_t* pat, int filter);

	void(*save)(rvg_t* v);		// 保存状态，（裁剪状态暂不实现）
	void(*restore)(rvg_t* v);	// 恢复状态
	void(*stroke)(rvg_t* v);
	void(*stroke_preserve)(rvg_t* v);
	void(*fill)(rvg_t* v);
	void(*fill_preserve)(rvg_t* v);
	void(*paint)(rvg_t* v);			// 全屏渲染
	void(*reset_clip)(rvg_t* v);	// 重置裁剪
	void(*clip)(rvg_t* v);			// 路径裁剪，清空当前路径
	void(*clip_preserve)(rvg_t* v);	// 路径裁剪
	void(*clip_rect)(rvg_t* v, int x, int y, int width, int height);	// 矩形裁剪
	void(*set_clip_rect)(rvg_t* v, void* rc);	// 矩形裁剪,int[4]
	void(*get_clip_rect)(rvg_t* v, void* rc);	// 获取矩形裁剪

	// 添加文本，风格，渲染区可选
	void (*add_text)(rvg_t* dc, text_st_t* p, text_style_t* ts, text_box_rt* box);
	// 普通图片，支持九宫格、混合颜色
	void (*add_image)(rvg_t* dc, ovg_image_r* r);
	// 原始三角形，输入0则不修改
	void (*set_geom_state)(rvg_t* dc, gem_info_t* info, const void* matrix4x4);
	// 添加几何数据到缓冲区，xy顶点坐标，color顶点颜色，uv顶点纹理坐标，indices索引数据，color_type=0表示float4，1表示uint32_t
	void (*add_geometry)(rvg_t* dc, vg_surface_t* texture, const float* xy, int xy_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type);
	// 添加3D几何数据到缓冲区，xyz顶点坐标，color顶点颜色（双面则要双倍），uv顶点纹理坐标，indices索引数据
	void (*add_geometry3d)(rvg_t* dc, vg_surface_t* texture, const float* xyz, int xyz_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type);
	// todo 录制
	void (*start_recording)(rvg_t* ctx);
	ovg_recording_t* (*stop_recording)(rvg_t* ctx);
	void (*replay)(rvg_t* ctx, ovg_recording_t* rec);
	void (*replay_command)(rvg_t* ctx, ovg_recording_t* rec, uint32_t cmdIndex);
	uint32_t(*recording_get_count)(ovg_recording_t* rec);
	void* (*recording_get_data)(ovg_recording_t* rec);
	void  (*recording_destroy)(ovg_recording_t* rec);

};

ovg_canvas_cb* new_canvas_cb();
void free_canvas_cb(ovg_canvas_cb*);

ovg_ctx_cb* new_ctx_cb();
void free_ctx_cb(ovg_ctx_cb*);

ovg_draw_data get_draw_list(rvg_t* p);
