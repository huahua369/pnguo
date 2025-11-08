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
	VG_CMD_CLIP_RECT,
	VG_CMD_CLIP_RECT_END,
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
typedef void (*vg_add_draw_cmd_fun)(void* ctx, uint8_t* cmds, size_t count, void* data, size_t size, int order);
// 填充风格，返回开始序号
typedef size_t(*vg_add_style_fun)(void* ctx, fill_style_d* st, size_t count);
// 文本风格在text_d的font_st用，返回开始序号
typedef size_t(*vg_add_text_style_fun)(void* ctx, text_style_d* st, size_t count);
// 清除风格数据，0全部，1只清除填充风格，2只清除文本风格
typedef void(*vg_clear_style_fun)(void* ctx, int t);

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
class flex_item
{
public:
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

	// size[0] == width, size[1] == height
	typedef void (*flex_self_sizing)(flex_item* item, float* size);
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

	void* managed_ptr = NULL;	// 用户数据指针
	flex_self_sizing self_sizing = NULL; // 运行时计算大小

	float frame[4] = {};	// 输出坐标、大小
	flex_item* parent = 0;	// 父级
	std::vector<flex_item*>* children = 0;	// 子级 
public:
	flex_item();
	~flex_item();
	// 指针为空就默认值，80字节
	void set_base(float* size, float* offset4, float* padding4, float* margin4, float* gsb, int order, uint8_t* align_pdw7);
	// 复制除宽高外的属性
	void copy_a(flex_item* p);
	flex_item* init();
	void update_should_order_children();	// 子元素属性改变时执行

	void item_add(flex_item* child);
	void item_insert(uint32_t index, flex_item* child);
	flex_item* item_delete(uint32_t index);
	flex_item* detach(flex_item* child);
	// 清空子元素
	void clear();
	// 执行布局计算
	void layout();
private:
	void layout_items(uint32_t child_begin, uint32_t child_end, uint32_t children_count, struct flex_layout* layout, uint32_t last_count);
	void layout_item(float width, float height);
};

#endif // NO_FLEX_CX



#ifdef FLEX_IMPLEMENTATION

flex_item::flex_item()
{
}

flex_item::~flex_item()
{
	if (children)
		delete children;
	children = 0;
}



void flex_item::update_should_order_children()
{
	if (order != 0 && parent != NULL) {
		parent->should_order_children = true;
	}
}


void flex_item::set_base(float* size, float* offset4, float* padding4, float* margin4, float* gsb, int order1, uint8_t* align_pdw7)
{
	if (size) {
		width = size[0];
		height = size[1];
	}
	if (offset4) {
		left = offset4[0];
		right = offset4[1];
		top = offset4[2];
		bottom = offset4[3];
	}
	if (padding4)
	{
		padding_left = padding4[0];
		padding_right = padding4[1];
		padding_top = padding4[2];
		padding_bottom = padding4[3];
	}
	if (margin4)
	{
		margin_left = margin4[0];
		margin_right = margin4[1];
		margin_top = margin4[2];
		margin_bottom = margin4[3];
	}
	auto p7 = align_pdw7;
	if (p7)
	{
		justify_content = (flex_align)p7[0];
		align_content = (flex_align)p7[1];
		align_items = (flex_align)p7[2];
		align_self = (flex_align)p7[3];

		position = (flex_position)p7[4];
		direction = (flex_direction)p7[5];
		wrap = (flex_wrap)p7[6];
		//bool should_order_children = false;
	}
	if (gsb) {
		grow = gsb[0];
		shrink = gsb[1];
		basis = gsb[2];
	}
	order = order1;
}

void flex_item::copy_a(flex_item* p)
{
	auto d0 = (char*)&left;
	auto d1 = (char*)&managed_ptr;
	auto d2 = (char*)&p->left;
	memcpy(d0, d2, d1 - d0);
}

flex_item* flex_item::init()
{
	flex_item* item = this;
	assert(item != NULL);
	*item = {};
	item->parent = NULL;
	if (item->children)
		item->children->clear();
	item->should_order_children = false;
	return item;
}


void flex_item::item_add(flex_item* child)
{
	flex_item* item = this;
	if (!item->children) {
		item->children = new std::vector<flex_item*>();
	}
	item->children->push_back(child);
	child->parent = item;
	child->update_should_order_children();
}


void flex_item::item_insert(uint32_t index, flex_item* child)
{
	flex_item* item = this;
	if (!item->children) {
		item->children = new std::vector<flex_item*>();
	}
	assert(index <= item->children->size());
	item->children->insert(item->children->begin() + index, child);
	child->parent = item;
	child->update_should_order_children();
}


flex_item* flex_item::item_delete(uint32_t index)
{
	flex_item* item = this;
	if (!children || children->empty())return nullptr;
	assert(index < item->children->size());
	assert(item->children->size() > 0);
	auto& v = *children;
	flex_item* child = v[index];
	children->erase(children->begin() + index);
	return child;
}

flex_item* flex_item::detach(flex_item* child)
{
	flex_item* item = this;
	if (!children || !child || !(item->children->size() > 0))return child;
	auto& v = *children;
	v.erase(std::remove(v.begin(), v.end(), child), v.end());
	child->parent = NULL;
	return child;
}

void flex_item::clear()
{
	if (children && children->size()) {
		children->clear();
	}
}


flex_item* flex_item_root(flex_item* item)
{
	while (item->parent != NULL) {
		item = item->parent;
	}
	return item;
}
//
//#define FRAME_GETTER(name, index) \
//     float flex_item_get_frame_##name(flex_item *item) { \
//        return item->frame[index]; \
//    }
//
//FRAME_GETTER(x, 0)
//FRAME_GETTER(y, 1)
//FRAME_GETTER(width, 2)
//FRAME_GETTER(height, 3)
//
//#undef FRAME_GETTER

struct flex_layout {
	// Set during init.
	bool wrap;
	bool reverse;               // whether main axis is reversed
	bool reverse2;              // whether cross axis is reversed (wrap only)
	bool vertical;
	float size_dim;             // main axis parent size
	float align_dim;            // cross axis parent size
	uint32_t frame_pos_i;   // main axis position
	uint32_t frame_pos2_i;  // cross axis position
	uint32_t frame_size_i;  // main axis size
	uint32_t frame_size2_i; // cross axis size
	uint32_t* ordered_indices;
	size_t ordered_count;
	// Set for each line layout.
	float line_dim;             // the cross axis size
	float flex_dim;             // the flexible part of the main axis size
	float extra_flex_dim;       // sizes of flexible items
	float flex_grows;
	float flex_shrinks;
	float pos2;                 // cross axis position
	float baseline;
	// Calculated layout lines - only tracked when needed:
	//   - if the root's align_content property isn't set to FLEX_ALIGN_START
	//   - or if any child item doesn't have a cross-axis size set
	bool need_lines;
	struct flex_layout_line {
		uint32_t child_begin;
		uint32_t child_end;
		float size;
	};
	flex_layout_line* lines;
	size_t lines_idx;
	//uint32_t lines_count;
	float lines_sizes;
	//uint32_t lines_cap;
};

flex_item::flex_align child_align(flex_item* child, flex_item* parent)
{
	auto align = child->align_self;
	if (align == flex_item::flex_align::ALIGN_AUTO && parent) {
		align = parent->align_items;
	}
	return align;
}

void layout_init(flex_item* item, float width, float height, struct flex_layout* layout)
{
	assert(item->padding_left >= 0);
	assert(item->padding_right >= 0);
	assert(item->padding_top >= 0);
	assert(item->padding_bottom >= 0);
	width -= item->padding_left + item->padding_right;
	height -= item->padding_top + item->padding_bottom;
	assert(width >= 0);
	assert(height >= 0);

	layout->reverse = false;
	layout->vertical = true;
	switch (item->direction) {
	case flex_item::flex_direction::ROW_REVERSE:
		layout->reverse = true;
	case flex_item::flex_direction::ROW:
		layout->vertical = false;
		layout->size_dim = width;
		layout->align_dim = height;
		layout->frame_pos_i = 0;
		layout->frame_pos2_i = 1;
		layout->frame_size_i = 2;
		layout->frame_size2_i = 3;
		break;

	case flex_item::flex_direction::COLUMN_REVERSE:
		layout->reverse = true;
	case flex_item::flex_direction::COLUMN:
		layout->size_dim = height;
		layout->align_dim = width;
		layout->frame_pos_i = 1;
		layout->frame_pos2_i = 0;
		layout->frame_size_i = 3;
		layout->frame_size2_i = 2;
		break;

	default:
		assert(false && "incorrect direction");
	}

	//layout->ordered_indices.clear();
	if (item->children && item->should_order_children && item->children->size() > 0) {
		layout->ordered_indices = new uint32_t[item->children->size()];// .resize(item->children->size());
		uint32_t* indices = layout->ordered_indices;// .data();
		assert(indices != NULL);
		// Creating a list of item indices sorted using the children's `order'
		// attribute values. We are using a simple insertion sort as we need
		// stability (insertion order must be preserved) and cross-platform
		// support. We should eventually switch to merge sort (or something
		// else) if the number of items becomes significant enough.
		auto& icv = *item->children;
		for (uint32_t i = 0; i < item->children->size(); i++) {
			indices[i] = i;
			for (uint32_t j = i; j > 0; j--) {
				uint32_t prev = indices[j - 1];
				uint32_t curr = indices[j];
				if (icv[prev]->order <= icv[curr]->order) {
					break;
				}
				indices[j - 1] = curr;
				indices[j] = prev;
			}
		}
	}

	layout->flex_dim = 0;
	layout->flex_grows = 0;
	layout->flex_shrinks = 0;

	layout->reverse2 = false;
	layout->wrap = item->wrap != flex_item::flex_wrap::NO_WRAP;
	if (layout->wrap) {
		if (item->wrap == flex_item::flex_wrap::WRAP_REVERSE) {
			layout->reverse2 = true;
			layout->pos2 = layout->align_dim;
		}
	}
	else {
		layout->pos2 = layout->vertical
			? item->padding_left : item->padding_top;
	}

	layout->need_lines = layout->wrap && item->align_content != flex_item::flex_align::ALIGN_START;
	layout->lines = new flex_layout::flex_layout_line[item->children->size()];
	layout->lines_idx = 0;
	layout->lines_sizes = 0;
	auto align_items = child_align(item, item->parent);
	if (align_items == flex_item::flex_align::ALIGN_BASELINE && item->children)
	{
		layout->baseline = 0;
		for (auto& it : *item->children) {
			layout->baseline = std::max(layout->baseline, it->baseline);
		}
	}
}

void layout_cleanup(struct flex_layout* layout)
{
	if (layout)
	{
		if (layout->ordered_indices)
		{
			delete[] layout->ordered_indices;
		}
		if (layout->lines)
		{
			delete[]layout->lines;
		}
		layout->ordered_indices = 0;
		layout->lines = 0;
	}
	//layout->ordered_indices.clear();
	//layout->lines.clear();
}

#define LAYOUT_RESET() \
    do { \
        layout->line_dim = layout->wrap ? 0 : layout->align_dim; \
        layout->flex_dim = layout->size_dim; \
        layout->extra_flex_dim = 0; \
        layout->flex_grows = 0; \
        layout->flex_shrinks = 0; \
    } \
    while (0)

#define LAYOUT_CHILD_AT(item, i) ((*item->children)[(layout->ordered_count ? layout->ordered_indices[i] : i)])
//#define LAYOUT_CHILD_AT(item, i) ((*item->children)[(layout->ordered_indices.size() ? layout->ordered_indices[i] : i)])
//#define LAYOUT_CHILD_AT(item, i) (item->children.ary[(layout->ordered_indices != NULL ? layout->ordered_indices[i] : i)])  

#define _LAYOUT_FRAME(child, name) child->frame[layout->frame_##name##_i]

#define CHILD_POS(child) _LAYOUT_FRAME(child, pos)
#define CHILD_POS2(child) _LAYOUT_FRAME(child, pos2)
#define CHILD_SIZE(child) _LAYOUT_FRAME(child, size)
#define CHILD_SIZE2(child) _LAYOUT_FRAME(child, size2)

#define CHILD_MARGIN(child, if_vertical, if_horizontal) \
    (layout->vertical \
     ? child->margin_##if_vertical \
     : child->margin_##if_horizontal)


bool layout_align(flex_item::flex_align align, float flex_dim, uint32_t children_count, float* pos_p, float* spacing_p, bool stretch_allowed)
{
	assert(flex_dim > 0);

	float pos = 0;
	float spacing = 0;
	switch (align) {
	case flex_item::flex_align::ALIGN_START:
		break;

	case flex_item::flex_align::ALIGN_END:
		pos = flex_dim;
		break;

	case flex_item::flex_align::ALIGN_CENTER:
		pos = flex_dim / 2;
		break;

	case flex_item::flex_align::ALIGN_SPACE_BETWEEN:
		if (children_count > 0) {
			spacing = flex_dim / (children_count - 1);
		}
		break;

	case flex_item::flex_align::ALIGN_SPACE_AROUND:
		if (children_count > 0) {
			spacing = flex_dim / children_count;
			pos = spacing / 2;
		}
		break;

	case flex_item::flex_align::ALIGN_SPACE_EVENLY:
		if (children_count > 0) {
			spacing = flex_dim / (children_count + 1);
			pos = spacing;
		}
		break;

	case flex_item::flex_align::ALIGN_AUTO:
	case flex_item::flex_align::ALIGN_STRETCH:
		if (stretch_allowed) {
			spacing = flex_dim / children_count;
			break;
		}
		// fall through
		break;
	default:
		return false;
	}

	*pos_p = pos;
	*spacing_p = spacing;
	return true;
}

void flex_item::layout_items(uint32_t child_begin, uint32_t child_end, uint32_t children_count, struct flex_layout* layout, uint32_t last_count)
{
	flex_item* item = this;
	assert(children_count <= (child_end - child_begin));
	if (children_count <= 0) {
		return;
	}
	if (last_count > 0 && last_count > children_count)
	{
		//children_count = last_count;
	}
	if (layout->flex_dim > 0 && layout->extra_flex_dim > 0) {
		// If the container has a positive flexible space, let's add to it
		// the sizes of all flexible children->
		layout->flex_dim += layout->extra_flex_dim;
	}

	// Determine the main axis initial position and optional spacing.
	float pos = 0;
	float spacing = 0;
	if (layout->flex_grows == 0 && layout->flex_dim > 0) {
		if (!layout_align(item->justify_content, layout->flex_dim,
			children_count, &pos, &spacing, false))
		{
			assert(0 && "incorrect justify_content");
		}
		if (layout->reverse) {
			pos = layout->size_dim - pos;
		}
	}

	if (layout->reverse) {
		pos -= layout->vertical ? item->padding_bottom : item->padding_right;
	}
	else {
		pos += layout->vertical ? item->padding_top : item->padding_left;
	}
	if (layout->wrap && layout->reverse2) {
		layout->pos2 -= layout->line_dim;
	}

	for (uint32_t i = child_begin; i < child_end; i++) {
		flex_item* child = LAYOUT_CHILD_AT(item, i);
		if (child->position == flex_position::POS_ABSOLUTE) {
			// Already positioned.
			continue;
		}

		// Grow or shrink the main axis item size if needed.
		float flex_size = 0;
		if (layout->flex_dim > 0) {
			if (child->grow != 0) {
				CHILD_SIZE(child) = 0; // Ignore previous size when growing.
				flex_size = (layout->flex_dim / layout->flex_grows)
					* child->grow;
			}
		}
		else if (layout->flex_dim < 0) {
			if (child->shrink != 0) {
				flex_size = (layout->flex_dim / layout->flex_shrinks)
					* child->shrink;
			}
		}
		CHILD_SIZE(child) += flex_size;

		// Set the cross axis position (and stretch the cross axis size if
		// needed).
		float align_size = CHILD_SIZE2(child);
		float align_pos = layout->pos2 + 0;
		switch (child_align(child, item)) {
		case flex_align::ALIGN_END:
			align_pos += layout->line_dim - align_size
				- CHILD_MARGIN(child, right, bottom);
			break;

		case flex_align::ALIGN_CENTER:
			align_pos += (layout->line_dim / 2) - (align_size / 2)
				+ (CHILD_MARGIN(child, left, top)
					- CHILD_MARGIN(child, right, bottom));
			break;

		case flex_align::ALIGN_STRETCH:
			if (align_size == 0) {
				CHILD_SIZE2(child) = layout->line_dim
					- (CHILD_MARGIN(child, left, top)
						+ CHILD_MARGIN(child, right, bottom));
			}
			// fall through
			align_pos += CHILD_MARGIN(child, left, top);
			break;
		case flex_align::ALIGN_START:
			align_pos += CHILD_MARGIN(child, left, top);
			break;
		case flex_align::ALIGN_BASELINE:
			align_pos += CHILD_MARGIN(child, left, top);
			if (child->baseline > 0) {
				align_pos += layout->baseline - child->baseline;
			}
			break;
		default:
			assert(false && "incorrect align_self");
		}
		CHILD_POS2(child) = align_pos;

		// Set the main axis position.
		if (layout->reverse) {
			pos -= CHILD_MARGIN(child, bottom, right);
			pos -= CHILD_SIZE(child);
			CHILD_POS(child) = pos;
			pos -= spacing;
			pos -= CHILD_MARGIN(child, top, left);
		}
		else {
			pos += CHILD_MARGIN(child, top, left);
			CHILD_POS(child) = pos;
			pos += CHILD_SIZE(child);
			pos += spacing;
			pos += CHILD_MARGIN(child, bottom, right);
		}

		// Now that the item has a frame, we can layout its children.
		child->layout_item(child->frame[2], child->frame[3]);
	}

	if (layout->wrap && !layout->reverse2) {
		layout->pos2 += layout->line_dim;
	}

	if (layout->need_lines) {
		flex_layout::flex_layout_line line[1] = {};
		line->child_begin = child_begin;
		line->child_end = child_end;
		line->size = layout->line_dim;
		layout->lines[layout->lines_idx] = (line[0]);
		layout->lines_idx++;
		//layout->lines.push_back(line[0]);
		layout->lines_sizes += line->size;
	}
}

void flex_item::layout_item(float width, float height)
{
	flex_item* item = this;
	if (!item->children || item->children->size() == 0) {
		return;
	}

	struct flex_layout layout_s = { 0 }, * layout = &layout_s;
	layout_init(item, width, height, &layout_s);

	LAYOUT_RESET();
	uint32_t last_count = 0;
	uint32_t last_layout_child = 0;
	uint32_t relative_children_count = 0;
	for (uint32_t i = 0; i < item->children->size(); i++) {
		flex_item* child = LAYOUT_CHILD_AT(item, i);

		// Items with an absolute position have their frames determined
		// directly and are skipped during layout.
		if (child->position == flex_position::POS_ABSOLUTE) {
#define ABSOLUTE_SIZE(val, pos1, pos2, dim) \
            (!isnan(val) \
             ? val \
             : (!isnan(pos1) && !isnan(pos2) \
                 ? dim - pos2 - pos1 \
                 : 0))

#define ABSOLUTE_POS(pos1, pos2, size, dim) \
            (!isnan(pos1) \
             ? pos1 \
             : (!isnan(pos2) \
                 ? dim - size - pos2 \
                 : 0))

			float child_width = ABSOLUTE_SIZE(child->width, child->left,
				child->right, width);

			float child_height = ABSOLUTE_SIZE(child->height, child->top,
				child->bottom, height);

			float child_x = ABSOLUTE_POS(child->left, child->right,
				child_width, width);

			float child_y = ABSOLUTE_POS(child->top, child->bottom,
				child_height, height);

			child->frame[0] = child_x;
			child->frame[1] = child_y;
			child->frame[2] = child_width;
			child->frame[3] = child_height;

			// Now that the item has a frame, we can layout its children.
			child->layout_item(child->frame[2], child->frame[3]);

#undef ABSOLUTE_POS
#undef ABSOLUTE_SIZE
			continue;
		}

		// Initialize frame.
		child->frame[0] = 0;
		child->frame[1] = 0;
		child->frame[2] = child->width;
		child->frame[3] = child->height;

		// Main axis size defaults to 0.
		if (isnan(CHILD_SIZE(child))) {
			CHILD_SIZE(child) = 0;
		}

		// Cross axis size defaults to the parent's size (or line size in wrap
		// mode, which is calculated later on).
		if (isnan(CHILD_SIZE2(child))) {
			if (layout->wrap) {
				layout->need_lines = true;
			}
			else {
				CHILD_SIZE2(child) = (layout->vertical ? width : height)
					- CHILD_MARGIN(child, left, top)
					- CHILD_MARGIN(child, right, bottom);
			}
		}

		// Call the self_sizing callback if provided. Only non-NAN values
		// are taken into account. If the item's cross-axis align property
		// is set to stretch, ignore the value returned by the callback.
		if (child->self_sizing != NULL) {
			float size[2] = { child->frame[2], child->frame[3] };

			child->self_sizing(child, size);

			for (uint32_t j = 0; j < 2; j++) {
				uint32_t size_off = j + 2;
				if (size_off == layout->frame_size2_i
					&& child_align(child, item) == flex_align::ALIGN_STRETCH) {
					continue;
				}
				float val = size[j];
				if (!isnan(val)) {
					child->frame[size_off] = val;
				}
			}
		}

		// Honor the `basis' property which overrides the main-axis size.
		if (!(isnan(child->basis) || child->basis < 0)) {
			assert(child->basis >= 0);
			CHILD_SIZE(child) = child->basis;
		}

		float child_size = CHILD_SIZE(child);
		if (layout->wrap) {
			if (layout->flex_dim < child_size) {
				// Not enough space for this child on this line, layout the
				// remaining items and move it to a new line.
				item->layout_items(last_layout_child, i, relative_children_count, layout, last_count);

				LAYOUT_RESET();
				last_layout_child = i;
				if (last_count < relative_children_count)
					last_count = relative_children_count;
				relative_children_count = 0;
			}

			float child_size2 = CHILD_SIZE2(child);
			if (!isnan(child_size2) && child_size2 > layout->line_dim) {
				layout->line_dim = child_size2;
			}
		}

		assert(child->grow >= 0);
		assert(child->shrink >= 0);

		layout->flex_grows += child->grow;
		layout->flex_shrinks += child->shrink;

		layout->flex_dim -= child_size
			+ (CHILD_MARGIN(child, top, left)
				+ CHILD_MARGIN(child, bottom, right));

		relative_children_count++;

		if (child_size > 0 && child->grow > 0) {
			layout->extra_flex_dim += child_size;
		}
	}

	// Layout remaining items in wrap mode, or everything otherwise.
	item->layout_items(last_layout_child, item->children ? item->children->size() : 0, relative_children_count, layout, last_count);

	// In wrap mode we may need to tweak the position of each line according to
	// the align_content property as well as the cross-axis size of items that
	// haven't been set yet.
	if (layout->need_lines && layout->lines_idx > 0) {
		float pos = 0;
		float spacing = 0;
		float flex_dim = layout->align_dim - layout->lines_sizes;
		if (flex_dim > 0) {
			if (!layout_align(item->align_content, flex_dim, layout->lines_idx, &pos, &spacing, true))
			{
				assert(0 && "incorrect align_content");
			}
		}

		float old_pos = 0;
		if (layout->reverse2) {
			pos = layout->align_dim - pos;
			old_pos = layout->align_dim;
		}

		for (uint32_t i = 0; i < layout->lines_idx; i++) {
			auto line = &layout->lines[i];

			if (layout->reverse2) {
				pos -= line->size;
				pos -= spacing;
				old_pos -= line->size;
			}

			// Re-position the children of this line, honoring any child
			// alignment previously set within the line.
			for (uint32_t j = line->child_begin; j < line->child_end;
				j++) {
				flex_item* child = LAYOUT_CHILD_AT(item, j);
				if (child->position == flex_position::POS_ABSOLUTE) {
					// Should not be re-positioned.
					continue;
				}
				if (isnan(CHILD_SIZE2(child))) {
					// If the child's cross axis size hasn't been set it, it
					// defaults to the line size.
					CHILD_SIZE2(child) = line->size
						+ (item->align_content == flex_align::ALIGN_STRETCH
							? spacing : 0);
				}
				CHILD_POS2(child) = pos + (CHILD_POS2(child) - old_pos);
			}

			if (!layout->reverse2) {
				pos += line->size;
				pos += spacing;
				old_pos += line->size;
			}
		}
	}

	layout_cleanup(layout);
}

#undef CHILD_MARGIN
#undef CHILD_POS
#undef CHILD_POS2
#undef CHILD_SIZE
#undef CHILD_SIZE2
#undef _LAYOUT_FRAME
#undef LAYOUT_CHILD_AT
#undef LAYOUT_RESET


void flex_item::layout()
{
	assert(parent == NULL);
	assert(!isnan(width));
	assert(!isnan(height));
	assert(self_sizing == NULL);
	layout_item(width, height);
}

#endif // !FLEX_IMPLEMENTATION



#endif // __cplusplus

#endif // !VG_H
