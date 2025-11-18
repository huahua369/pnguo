
#ifndef VG_H
#define VG_H


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

#ifndef NO_FLEX_CX
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
struct flex_data {
	float width = NAN;
	float height = NAN;

	float left = NAN;			// 左偏移坐标
	float right = NAN;
	float top = NAN;
	float bottom = NAN;

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
	flex_wrap wrap = flex_wrap::WRAP;						// 父元素:是否换行，超出宽度自动换行

	bool should_order_children = false;
};

#endif // NO_FLEX_CX



#endif // __cplusplus

#endif // !VG_H




