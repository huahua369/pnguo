#pragma once
#ifndef VG_H
#define VG_H

struct image_ptr_t
{
	int width = 0, height = 0;
	int type = 0;				// 0=rgba，1=bgra
	int stride = 0;
	uint32_t* data = 0;			// 像素数据
	void* texid = 0;			// 纹理指针，由调用方自动生成管理
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
struct image_d {
	void* image; glm::vec2 pos; glm::vec4 rc; uint32_t color = -1; glm::vec2 dsize = { -1,-1 };
	// 九宫格图片
	glm::vec4 sliced = {};
	size_t res_idx;		// 资源序号
};

struct clip_rect_d {
	glm::vec2 pos;
	glm::vec2 size;
};
enum class VG_DRAW_CMD :uint8_t {
	VG_CMD_CLEAR = 0,
	VG_CMD_SAVE,
	VG_CMD_RESTORE,
	VG_CMD_CLIP_RECT,
	VG_CMD_RECT,
	VG_CMD_RECT4R,
	VG_CMD_CIRCLE,
	VG_CMD_ELLIPSE,
	VG_CMD_TRIANGLE,
	VG_CMD_FILL_GRID,
	VG_CMD_FILL_LINEAR,
	VG_CMD_STROKE_LINE,
	VG_CMD_STROKE_POLYLINE,
	VG_CMD_IMAGE,
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
struct vg_style_data {
	fill_style_d* fs;
	size_t fs_count;
	text_style_d* ts;
	size_t ts_count;
};
typedef void (*vg_draw_cmd_fun)(void* ctx, uint8_t* cmds, size_t count, void* data, size_t size, vg_style_data* style);
typedef void (*vg_draw_path_fun)(void* ctx, path_d* path, fill_style_d* style);


#ifdef __cplusplus
// 简易stb_image加载
class stbimage_load :public image_ptr_t
{
public:
	int rcomp = 4;	// 目标通道
public:
	stbimage_load();
	stbimage_load(const char* fn);

	~stbimage_load();
	bool load(const char* fn);

	bool load_mem(const char* d, size_t s);
	void tobgr();
	static stbimage_load* new_load(const void* fnd, size_t len);
	static void free_img(stbimage_load* p);
};
/*
	根元素要求
	assert(parent == NULL);
	assert(!isnan(width));
	assert(!isnan(height));
	assert(self_sizing == NULL);

	FLEX_ALIGN_SPACE_BETWEEN,	//两端对齐，两端间隔0，中间间隔1
	FLEX_ALIGN_SPACE_AROUND,	//分散居中,两端间隔0.5，中间间隔1
	FLEX_ALIGN_SPACE_EVENLY,	//分散居中,每个间隔1
*/
enum class flex_align :uint8_t {
	ALIGN_AUTO = 0,
	ALIGN_STRETCH,
	ALIGN_CENTER,
	ALIGN_START,
	ALIGN_END,
	ALIGN_SPACE_BETWEEN,
	ALIGN_SPACE_AROUND,
	ALIGN_SPACE_EVENLY,
	ALIGN_BASELINE
};

enum class flex_position :uint8_t {
	POS_RELATIVE = 0,
	POS_ABSOLUTE
};
// row行，reverse反向，column列
enum flex_direction :uint8_t {
	ROW = 0,
	ROW_REVERSE,
	COLUMN,
	COLUMN_REVERSE
};

enum class flex_wrap :uint8_t {
	NO_WRAP = 0,
	WRAP,
	WRAP_REVERSE
};
#else
typedef uint8_t flex_align;
typedef uint8_t flex_position;
typedef uint8_t flex_direction;
typedef uint8_t flex_wrap;
#endif // __cplusplus
struct flex_data {
	float width = NAN, height = NAN;	// 大小
	float left = NAN, right = NAN, top = NAN, bottom = NAN;	// 偏移
	float padding_left = 0;		// 本元素内边距
	float padding_right = 0;
	float padding_top = 0;
	float padding_bottom = 0;
	float margin_left = 0;		// 本元素外边距
	float margin_right = 0;
	float margin_top = 0;
	float margin_bottom = 0;
	float grow = 0;		// 子元素:自身放大比例，默认为0不放大
	float shrink = 0;	// 子元素:空间不足时自身缩小比例，默认为1自动缩小，0不缩小
	int	  order = 0;	// 子元素:自身排列顺序。数值越小，越靠前
	float basis = NAN;	// 子元素:定义最小空间
	float baseline = 0.0; // 基线位置
	flex_align justify_content = flex_align::ALIGN_START;	// 父元素:主轴上的元素的排列方式 start\end\center\space-between\space-around\space-evenly
	flex_align align_content = flex_align::ALIGN_STRETCH;	// 父元素:适用多行的flex容器 start\end\center\space-between\space-around\space-evenly\stretch 
	flex_align align_items = flex_align::ALIGN_STRETCH;		// 父元素:副轴上的元素的排列方式 start\end\center\stretch\baseline
	flex_align align_self = flex_align::ALIGN_AUTO;			// 子元素:覆盖父容器align-items的设置
	flex_position position = flex_position::POS_RELATIVE;	// 子元素:
	flex_direction direction = flex_direction::ROW;			// 父元素:
	flex_wrap wrap = flex_wrap::NO_WRAP;						// 父元素:是否换行，超出宽度自动换行
	bool should_order_children = false;
};

struct node_dt
{
	glm::vec2 size = {};	// in 原大小
	glm::vec4 offset = {};	// in 偏移位置
	glm::vec4 frame = {};	// out 输出位置大小
	size_t index = 0;		// in 样式序号
	float baseline = 0.0;	// in 基线位置
	node_dt* child = 0;		// in 子元素指针
	size_t child_count = 0;
	size_t tidx = -1;		// out 自动计算节点索引
	size_t parent = -1;		// out 自动计算父节点索引
	size_t line_count = 0;	// out 行数量
};
// 输入样式数据，根节点指针，所有节点数量
glm::vec4 flex_layout_calc(flex_data* fd, size_t count, node_dt* p, size_t node_count);



struct texture_dt
{
	// 源区域、目标区域 
	glm::vec4 src_rect = {};
	glm::vec4 dst_rect = {};
};
struct texture_angle_dt
{
	// 源区域、目标区域 
	glm::vec4 src_rect = {};
	glm::vec4 dst_rect = {};
	glm::vec2 center = {};	//旋转中心
	double angle = 0;		//旋转角度
	int flip = 0;			//翻转模式，0=SDL_FLIP_NONE，1=SDL_FLIP_HORIZONTAL，2=SDL_FLIP_VERTICAL
};
// 渲染重复平铺
struct texture_tiled_dt
{
	glm::vec4 src_rect = {};
	glm::vec4 dst_rect = {};
	float scale = 1.0;
};
// 九宫格渲染
struct texture_9grid_dt
{
	glm::vec4 src_rect = {};
	glm::vec4 dst_rect = {};
	float scale = 1.0;
	float left_width = 1.0, right_width = 1.0, top_height = 1.0, bottom_height = 1.0;
	float tileScale = -1; // 大于0则是9gridtiled中间平铺
};

// 渲染三角网，indices支持uint8_t uint16_t uint(实际最大值int_max)
struct geometryraw_dt
{
	const float* xy; int xy_stride;
	const glm::vec4* color; int color_stride;// color_stride等于0时，用一个颜色
	const float* uv; int uv_stride;
	int num_vertices;
	const void* indices; int num_indices; int size_indices;
};
// 混合模式
enum class BLENDMODE_E :int {
	none = -1,
	normal = 0,	// 普通混合
	additive,
	multiply,
	modulate,
	screen
};
#ifndef TEX_CB
#define TEX_CB
struct texture_cb
{
	// 创建纹理
	void* (*new_texture)(void* renderer, int width, int height, int type, void* data, int stride, int bm, bool static_tex, bool multiply);
	// 更新纹理数据
	void (*update_texture)(void* texture, const glm::ivec4* rect, const void* pixels, int pitch);
	// 设置纹理混合模式
	void (*set_texture_blend)(void* texture, uint32_t b, bool multiply);
	void (*set_texture_color)(void* texture, float r, float g, float b, float a);
	void (*set_texture_color4)(void* texture, const glm::vec4* c);
	void (*set_texture_color32)(void* texture, const uint32_t* c);
	// 删除纹理
	void (*free_texture)(void* texture);
	// 创建或更新纹理
	void* (*make_tex)(void* renderer, image_ptr_t* img);
	// 从图片文件创建纹理
	void* (*new_texture_file)(void* renderer, const char* fn);
	// 纹理渲染
	// 批量区域渲染
	int (*render_texture)(void* renderer, void* texture, texture_dt* p, int count);
	// 批量区域渲染,颜色可选，uint32_t和vec4两种颜色指针，color_count等于1或和区域count相同
	int (*render_texture_color)(void* renderer, void* texture, texture_dt* p, int count, const void* color, int color_count, int color_type);
	// 单个区域支持旋转
	bool (*render_texture_rotated)(void* renderer, void* texture, texture_angle_dt* p, int count);
	// 平铺渲染
	bool (*render_texture_tiled)(void* renderer, void* texture, texture_tiled_dt* p, int count);
	// 九宫格渲染
	bool (*render_texture_9grid)(void* renderer, void* texture, texture_9grid_dt* p, int count);
	// 渲染2d三角网,支持顶点色、纹理
	bool (*render_geometryraw)(void* renderer, void* texture, geometryraw_dt* p, int count);
	// 创建一个指定宽高的rgba纹理
	void* (*new_texture_0)(void* renderer, int width, int height);
	// 通过vk image创建纹理
	void* (*new_texture_vk)(void* renderer, int width, int height, void* vkptr, int format);
	bool (*draw_geometry)(void* renderer, void* texture, const float* xy, int xy_stride
		, const float* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices);
	bool (*set_viewport)(void* renderer, const glm::ivec4* rect);
	bool (*set_cliprect)(void* renderer, const glm::ivec4* rect);
	bool (*get_viewport)(void* renderer, glm::ivec4* rect);
	bool (*get_cliprect)(void* renderer, glm::ivec4* rect);
};
#else
typedef struct texture_cb texture_cb;
#endif

#endif // !VG_H




