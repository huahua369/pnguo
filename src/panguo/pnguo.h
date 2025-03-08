/*
* pnguo
	本文件的实现：2D骨骼动画、图集、(基础控件、面板)、字体管理、字体软光栅渲染、flex布局算法、简易路径path_v、样条线、stb加载图片、svg加载、cairo画布输出svg等
	多边形三角化（CDT、割耳法）、模型布尔运算、stl模型读写

	font_rctx、font_t
   字体纹理缓存管理
  layout_text_x

	关系：窗口->面板->控件/图形
	面板持有控件，窗口分发事件给面板。

	组件支持内容：文本、图片、矢量图、基础控件
todo 基础组件
- [ ] 表格视图：
- [ ] 树形视图：
- [ ] 属性视图：
- [ ] 对话框：
- [ ] 大文本编辑：
- [ ] 16进制文件编辑：
*/

#pragma once
#include <list>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <functional>

//typedef struct _PangoContext PangoContext;
struct input_state_t;

#ifndef BIT_INC
#define BIT_INC(x) (1<<x)
#endif

struct font_config_t {
	int id = 0, size = 16;
	uint32_t color = 0xffffffff;
	uint32_t back_color = 0xff000000;
};


namespace md {
	int64_t get_utf8_count(const char* buffer, int64_t len);
	const char* utf8_char_pos(const char* buffer, int64_t pos, uint64_t len);
	const char* utf8_prev_char(const char* str);
	const char* utf8_next_char(const char* p);
	int utf8_pointer_to_offset(const char* str, const char* pos);
	char* utf8_offset_to_pointer(const char* str, int offset);
	uint32_t get_u8_idx(const char* str, int64_t idx);
	const char* get_u8_last(const char* str, uint32_t* codepoint);
	std::string u16to_u8(uint16_t* str, size_t len);
	std::wstring u8to_w(const char* str, size_t len);
}
namespace pg
{
	std::string to_string(double _Val);
	std::string to_string(double _Val, const char* fmt);
	std::string to_string_p(uint32_t _Val);
	std::string to_string_p(uint64_t _Val);
	std::string to_string_hex(uint32_t _Val);
	std::string to_string_hex(uint64_t _Val, int n, const char* x);
	std::string to_string_hex2(uint32_t _Val);
}
/*
图集画布
class canvas_atlas
2D骨骼动画
struct skeleton_t
*/

// 骨骼动画矩阵
struct bone_t {
	glm::mat3* m = 0;			// 计算好的动画矩阵animated_mats
	int count = 0;
};
struct attachment_t
{
	void* tex = 0;			// 纹理，无纹理则显示纯色
	glm::vec2* pos = 0;		// 坐标
	glm::vec2* uvs = 0;		// UV
	glm::vec4* colors = 0;	// 顶点颜色(可选)
	size_t vs = 0;			// 顶点数量
	int* idxs = 0;			// 索引
	size_t is = 0;			// 索引数量
	glm::vec4 color = {};	// 无单独顶点颜色时，skeleton->color * slot->color * attachment->color;
	bool isActive = 0;		// 是否激活
};
struct slot_t
{
	glm::vec4 color = {};
	int blend_mode = 0;				// 混合模式
	attachment_t* attachment = 0;	// 激活的附件 
	bone_t* bone = 0;				// 骨头
	glm::vec4 weights = {};			// 权重
	glm::uvec4 joints = {};			// 骨骼id
};
class component2d_t;
struct skeleton_t
{
	glm::vec4 color = {};
	slot_t* solt_data = 0;
	size_t slots_size = 0;
	component2d_t* anim = 0;			// 组件，私有数据结构
	uint32_t active_idx = 0;		// 当前执行的动画
	uint32_t active_aidx = 0;		// 当前执行的动画
	bool usePremultipliedAlpha = false;//纹理预乘
	bool visible = 0;				// 是否可见
	// 切换动画id,或附件号
	void set_active(uint32_t idx, uint32_t aidx);
	void update(float delta);
};
// todo 未实现
skeleton_t* load_skeleton_file(const void* filepath);
skeleton_t* load_skeleton_mem(const void* d, size_t len);
// !end 2d skeleton 


// 线，二阶曲线，三阶曲线
enum class vte_e :uint32_t
{
	e_vmove = 1,
	e_vline,
	e_vcurve,
	e_vcubic
};

// 混合模式
enum class BlendMode_e :int {
	none = -1,
	normal = 0,	// 普通混合
	additive,
	multiply,
	screen
};


// 白色
#ifndef col_white
#define col_white 0xffffffff
//uint32_t color = col_white;			// 混合颜色，默认白色不混合
#endif

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
struct image_rc_t
{
	glm::ivec4 img_rc = {};	// 显示坐标、大小
	glm::ivec4 tex_rc = {};	// 纹理区域  
	uint32_t color = 0;	// 颜色混合/透明度
};
struct image_sliced_t
{
	glm::ivec4 img_rc = {};	// 显示坐标、大小
	glm::ivec4 tex_rc = {};	// 纹理区域
	glm::ivec4 sliced = {};	// 九宫格渲染0则不启用 
	uint32_t color = 0;	// 颜色混合/透明度
};
struct atlas_t
{
	image_ptr_t* img = 0;
	glm::ivec4* img_rc = 0;	// 显示坐标、大小
	glm::ivec4* tex_rc = 0;	// 纹理区域
	glm::ivec4* sliced = 0;	// 九宫格渲染
	uint32_t* colors = 0;	// 颜色混合/透明度
	size_t count = 0;		// 数量
	glm::ivec4 clip = {};	// 裁剪区域
};

struct vertex_v2
{
	glm::vec2 position = {};		// 坐标	
	glm::vec2 tex_coord = {};		// 纹理uv
	glm::vec4 color = { 1.0,1.0,1.0,1.0 };	// 顶点颜色
public:
	vertex_v2();
	vertex_v2(glm::vec3 p, glm::vec2 u, uint32_t  c);
	vertex_v2(glm::vec2 p, glm::vec2 u, uint32_t  c);
	vertex_v2(glm::vec2 p, glm::vec2 u, glm::vec4  c);
};
struct tex9Grid {
	glm::vec4 srcrect, dstrect;
	float left_width, right_width, top_height, bottom_height, scale;
};



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

void gray_copy2rgba(image_ptr_t* dst, image_ptr_t* src, const glm::ivec2& dst_pos, const glm::ivec4& rc, uint32_t col, bool b);
void save_img_png(image_ptr_t* p, const char* str);

class atlas_cx
{
public:
	image_ptr_t* img = 0;
	glm::ivec4 clip = {};	// 裁剪区域
	std::vector<image_sliced_t> _imgv;
	bool autofree = 0;
public:
	atlas_cx();
	virtual ~atlas_cx();
	void add(image_rc_t* d, size_t count);
	void add(image_sliced_t* d, size_t count);
	void add(const glm::ivec4& rc, const glm::ivec4& texrc, const glm::ivec4& sliced, uint32_t color = -1);
	void clear();
private:

};
struct image_rect {
	void* tex = 0;
	glm::vec4 src = {}, dst = {};
	int blend_mode = 0;					// 混合模式	
};
struct image_tiled {
	void* tex = 0;
	glm::vec4 src = {}, dst = {};
	int blend_mode = 0;					// 混合模式	
	float scale = 0.0f;					// Tiled 
};
struct image_9grid {
	void* tex = 0;
	glm::vec4 src = {}, dst = {};
	int blend_mode = 0;					// 混合模式	
	float scale = 0.0f;					// 9Grid
	float left_width = 0.0f, right_width = 0.0f, top_height = 0.0f, bottom_height = 0.0f;
};
class mesh2d_cx
{
public:
	struct tex_rs {
		void* tex = 0;
		glm::vec4 src = {}, dst = {};
		float scale = 0.0f;					// Tiled、9Grid
		float left_width = 0.0f, right_width = 0.0f, top_height = 0.0f, bottom_height = 0.0f;
		int blend_mode = 0;					// 混合模式	
	};
	struct draw_cmd_c
	{
		glm::ivec4 clip_rect = {};
		void* texid = 0;
		uint32_t vtxOffset = 0;
		uint32_t idxOffset = 0;
		uint32_t elemCount = 0;
		uint32_t vCount = 0;
		int blend_mode = 0;				// 混合模式	
	};
	std::vector<draw_cmd_c> cmd_data;	// 渲染命令
	std::vector<vertex_v2> vtxs;		// 顶点数据
	std::vector<int> idxs;				// 索引
public:
	mesh2d_cx();
	virtual ~mesh2d_cx();
	// 添加相同纹理/裁剪区域则自动合批
	void add(std::vector<vertex_v2>& vertex, std::vector<int>& vt_index, void* user_image, const glm::ivec4& clip);
private:

};
class gshadow_cx;
// 图集画布
class canvas_atlas
{
public:
	struct image_rs
	{
		image_ptr_t* img = 0;
		glm::ivec2 size = { -1,-1 };		// *大小	
		glm::ivec4 rect = { 0,0,-1,-1 };	// *区域/坐标大小
		glm::ivec4 sliced = {};				// 九宫格渲染 left top right bottom 
	};
	mesh2d_cx _mesh;

	std::vector<gshadow_cx*> _gs;		// 阴影管理
	std::vector<atlas_cx*> _atlas_cx;	// 显示的图集
	std::vector<atlas_t*> _atlas_t;		// 显示的图集
	std::vector<void* > _texs_t;		// 图集的纹理
	glm::ivec4 viewport = { 0,0,0,0 };  // maxint
	glm::ivec4 _clip_rect = { 0,0,200000,200000 };// 当前裁剪 
	void* _renderer = 0;	// 当前渲染器
	void(*destroy_texture_cb)(void* tex) = 0;//删除纹理回调 
	bool valid = false;
	bool visible = true;
	bool autofree = 0;
public:
	canvas_atlas();
	virtual ~canvas_atlas();
	gshadow_cx* new_gs();
	void add_atlas(atlas_cx* p);	// 添加显示的图集
	void remove_atlas(atlas_cx* p);	// 删除显示的图集
	void add_atlas(atlas_t* p);		// 添加显示的图集
	void remove_atlas(atlas_t* p);	// 删除显示的图集
	// 加载图片
	image_ptr_t* new_image2(const void* file);
	image_ptr_t* new_image2mem(const void* d, size_t s);
	void free_image(image_ptr_t* p);
	// 转换成bgra预乘
	void convert_bgr_multiply(image_ptr_t* p);
	size_t count();
public:
	// 窗口调用 
	void apply();	// 创建顶点啥的
private:
	void add_image(image_rs* r, const glm::vec2& pos, uint32_t color, const glm::ivec4& clip);
	void clear();	//清除渲染数据
	bool nohas_clip(glm::ivec4 a);
	// 创建九宫格顶点
	void make_image_sliced(void* img, const glm::ivec4& a, glm::ivec2 texsize, const glm::ivec4& sliced, const glm::ivec4& rect, uint32_t col, const glm::ivec4& clip);
};

/*
bspline
阶数degree\曲线类型
1直线
2二次曲线 例如圆，抛物线
3三次曲线 最常用的nurbs曲线
5连续度更高的曲线
7连续度极高的曲线
*/
namespace tinyspline {
	class BSpline;
}
class bspline_ct
{
public:
	void* ptr = 0;	// tinyspline::BSpline*
	int dim = 0;	// dimension vec2/3/4
public:
	bspline_ct();
	~bspline_ct();
	tinyspline::BSpline* get();	// 获取原始指针
	// 创建B样条线，返回ptr
	void* new_bspline(glm::vec2* cp, int n, size_t degree = 3);
	void* new_bspline(glm::dvec2* cp, int n, size_t degree = 3);
	void* new_bspline(glm::vec3* cp, int n, size_t degree = 3);
	void* new_bspline(glm::dvec3* cp, int n, size_t degree = 3);
	// nurbs
	void* new_bspline(glm::vec4* cp, int n, size_t degree = 2);
	void* new_bspline(glm::dvec4* cp, int n, size_t degree = 2);
	std::vector<glm::vec2> get_cp2();		// 获取控制点
	std::vector<glm::vec2> sample2(int m);	// 获取细分坐标
	std::vector<glm::vec3> sample3(int m);
	std::vector<glm::vec4> sample4(int m);
private:
	void free_bspline(void* p);

};

// 分割3次贝塞尔曲线
bool tessbc(glm::vec2* c, float t, std::vector<glm::vec2>* opt);


struct dash_t
{
	const double* dashes;
	size_t	      num_dashes;
	double	      offset;
};
struct dashf_t
{
	const float* dashes;
	int			 num_dashes;
	float	     offset;
};
/*
CAIRO_LINE_CAP_BUTT,	0在起点（终点）开始（停止）直线
CAIRO_LINE_CAP_ROUND,	1圆形结尾，圆心为终点
CAIRO_LINE_CAP_SQUARE	2正方形结尾，正方形的中心是终点
CAIRO_LINE_JOIN_MITER,	0尖角
CAIRO_LINE_JOIN_ROUND,	1圆角
CAIRO_LINE_JOIN_BEVEL	2截断
*/
struct style_path_t
{
	glm::vec2 pos = {};
	glm::vec2 scale = { 1.0f,1.0f };
	uint32_t fill_color = 0;
	uint32_t stroke_color = 0xffffffff;
	float line_width = 1.0f;
	dash_t dash = {};
	int8_t cap = 0;
	int8_t join = 0;
	int8_t flip_y = 0;
};



struct font_xi {
	const char* family = 0;
	float font_size = 16;
	uint32_t color = 0xffffffff;
	uint8_t slant = 0;//NORMAL=0，ITALIC=1，OBLIQUE=2
	bool weight_bold = false;
};

struct fontns
{
	std::string family, fullname, namecn;
	std::vector<std::string> style, fpath;
	std::set<std::string> alias;
	std::vector<void*> vptr;
};
// 获取系统字体列表
std::map<std::string, fontns> get_allfont();



struct vertex_f
{
	int type;
	float x, y, cx, cy, cx1, cy1;
};
struct vertex_v2f
{
	int type;
	glm::vec2 p, c, c1;
};
struct vertex_d
{
	int64_t type;
	double x, y, cx, cy, cx1, cy1;
};

struct tinypath_t
{
	vertex_f* v = 0;
	int count = 0;
	int baseline = 0;
	int first = 0;
	int advance = 0;
	glm::ivec2 bearing = {};
};

struct tinyflatten_t
{
	vertex_v2f* first = 0;
	size_t n = 0;
	int mc; float mlen;
	std::vector<glm::vec2>* flatten;
	float dist = 0;			// 距离剔除
	int angle = 100;		// 优化小于100度的
	bool oldfp = false;
};

struct font_impl;
class info_one
{
public:
	std::wstring name_;
	std::string name_a;
	std::string ndata;
	int platform_id = 0;
	int encoding_id = 0;
	int language_id = 0;
	int name_id = 0;
	int length_ = 0;
	info_one(int platform, int encoding, int language, int nameid, const char* name, int len);
	~info_one();
	std::string get_name();
};

class svg_cx
{
public:
	void* ptr;
	int width;
	int height;
	double em;
	double ex;
};
// 字体缓存
struct font_item_t
{
public:
	uint32_t _glyph_index = 0;
	// 原始的advance
	int advance = 0;
	// 渲染偏移坐标
	glm::ivec2 _dwpos = {};
	image_ptr_t* _image = nullptr;		// 排列int width,height,v2pad,data  
	// 缓存位置xy, 字符图像zw(width,height)
	glm::ivec4 _rect = {};
	glm::ivec2 _apos = {};// 布局坐标
	// 彩色字体时用
	uint32_t color = 0;
	int cpt = 0;
};

union ft_key_s;
struct gcolors_t;
struct bitmap_ttinfo;
struct Bitmap_p;
class stb_packer;

class bitmap_cache_cx
{
public:
	std::vector<stb_packer*> _packer;
	std::vector<image_ptr_t*> _data;
	int width = 1024;					// 纹理宽高
public:
	bitmap_cache_cx();
	~bitmap_cache_cx();
	// 复制像素到装箱，从pos返回坐标
	image_ptr_t* push_cache_bitmap(Bitmap_p* bitmap, glm::ivec2* pos, int linegap = 0, uint32_t col = -1);
	image_ptr_t* push_cache_bitmap(image_ptr_t* bitmap, glm::ivec2* pos, int linegap = 0, uint32_t col = -1);
	// 叠加到已有位置
	image_ptr_t* push_cache_bitmap_old(Bitmap_p* bitmap, glm::ivec2* pos, uint32_t col, image_ptr_t* ret, int linegap = 0);
	// 清空所有缓存
	void clear();
private:
	stb_packer* get_last_packer(bool isnew);

};
//image_ptr_t to_imgptr(tinyimage_cx* p);

struct hps_t;
class fd_info0;
// 单个字体
class font_t
{
public:
	font_impl* font = 0;
	uint32_t dataSize = 0;
	std::string _name, fullname;
	std::string _style;
	std::wstring _namew;
	// 支持的语言
	std::string _slng;
	int64_t num_glyphs = 0;
	uint32_t  _index = 0;

	float ascender = 0.0;
	float descender = 0.0;
	float lineh = 0.0;
	double  xMaxExtent = 0.0, lineGap = 0.0;
	std::map<uint64_t, font_item_t*> _cache_glyphidx;	// 缓存字形
	std::vector<font_item_t*> cache_data;
	int cache_count = 0;
	int ccount = 84;

	hps_t* hp = 0;

	bitmap_ttinfo* bitinfo = 0;
	gcolors_t* colorinfo = 0;
	bitmap_cache_cx* ctx = 0;  //纹理缓存，多个字体共用

	bool first_bitmap = false;
public:
	font_t();
	~font_t();
	float get_scale(int px);
	double get_base_line(double height);
	int get_xmax_extent(double height, int* line_gap);
	// 获取字体最大box
	glm::ivec4 get_bounding_box(double scale, bool is_align0);
	// 输入utf8获取轮廓，height=0则获取原始大小。提供opt
	tinypath_t get_shape(const void* str8, int height, std::vector<vertex_f>* opt, int adv);
	glm::ivec2 get_shape_box(const void* str8, int height);
	glm::ivec2 get_shape_box(uint32_t ch, int height);
	tinypath_t get_shape(int cp, int height, std::vector<vertex_f>* opt, int adv);
public:
	// 获取字符大小{xy大小，z=advance,w=基线}
	glm::ivec4 get_char_extent(char32_t ch, unsigned char font_size, /*unsigned short font_dpi,*/ std::vector<font_t*>* fallbacks, font_t** oft);
	void clear_char_lut();
	void clear_gcache();
	// todo 获取文字渲染信息。glyph_index=-1时则用unicode_codepoint；
	font_item_t get_glyph_item(uint32_t glyph_index, uint32_t unicode_codepoint, int fontsize);

	// 获取文字在哪个字体
	static const char* get_glyph_index_u8(const char* u8str, int* oidx, font_t** renderFont, std::vector<font_t*>* fallbacks);
	int get_glyph_index(uint32_t codepoint, font_t** renderFont, std::vector<font_t*>* fallbacks);

	std::map<int, std::vector<info_one>> get_detail();
public:
	void init_post_table();
	int init_color();
	int init_sbit();
private:

	// 字形索引缓存
	font_item_t* push_gcache(uint32_t glyph_index, uint32_t height, image_ptr_t* img, const glm::ivec4& rect, const glm::ivec2& pos);
	font_item_t* get_gcache(uint32_t glyph_index, uint32_t height);
	int get_glyph_image(int gidx, double height, glm::ivec4* ot, Bitmap_p* bitmap, std::vector<char>* out, int lcd_type, uint32_t unicode_codepoint, int xf = 0);
	int get_gcolor(uint32_t base_glyph, std::vector<uint32_t>& ag, std::vector<uint32_t>& col);

	int get_glyph_bitmap(int gidx, int height, glm::ivec4* ot, Bitmap_p* out_bitmap, std::vector<char>* out);
	int get_custom_decoder_bitmap(uint32_t unicode_codepoint, int height, glm::ivec4* ot, Bitmap_p* out_bitmap, std::vector<char>* out);

};

typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
typedef struct _PangoLayout      PangoLayout;

class font_rctx;
class font_imp;
/*
	CAIRO_LINE_CAP_BUTT,0
	CAIRO_LINE_CAP_ROUND,1
	CAIRO_LINE_CAP_SQUARE2
	CAIRO_LINE_JOIN_MITER,0
	CAIRO_LINE_JOIN_ROUND,1
	CAIRO_LINE_JOIN_BEVEL2
	*/
struct vg_style_t {
	uint32_t fill = 0;// 填充颜色
	uint32_t color = 0;		// 线颜色
	float thickness = 1;	// 线宽
	int round = 0;			// 圆角
	int cap = -1, join = -1;
	int dash_offset = 0;
	int dash_num = 0;		// 虚线计数 dash最大8、dash_p最大64
	union {
		uint64_t v = 0;
		uint8_t v8[8];
	}dash = {};		// ink  skip  ink  skip
	float* dash_p = 0;		// 虚线逗号/空格分隔的数字
};
/*	vg_style_t st[1] = {};
	st->dash = 0xF83E0;//0b11111000001111100000
	st->dash_num = 20;
	st->thickness = 1;
	st->join = 1;
	st->cap = 1;
	st->fill = 0x80FF7373;
	st->color = 0xffffffff;
	*/
	//struct text_layout_t
	//{
	//	PangoLayout* layout = 0;
	//	font_rctx* ctx = 0;
	//	glm::ivec2 pos = {};
	//	glm::ivec2 rc = {};
	//	int lineheight = 0;
	//	int baseline = 0;
	//	int text_color = -1;
	//	bool once = false;
	//};

struct text_style_t
{
	int font = 0;
	int font_size = 18;
	glm::vec2 text_align = { 0.0,0.5 };
	glm::vec2 shadow_pos = { 1.0,1.0 };
	uint32_t text_color = 0xffffffff;
	uint32_t text_color_shadow = 0;
	bool clip = true;
};

struct text_style_tx
{
	text_style_t st = {};	// 文本样式
	vg_style_t rcst = {};	// 背景矩形样式
};
struct text_tx
{
	glm::ivec4 rc = {}, trc = {};// 背景矩形，文本矩形
	const char* txt = 0;
	int len = 0;
	int st_index = 0;
};

//typedef enum TTF_Direction
//{
//	TTF_DIRECTION_INVALID = 0,
//	TTF_DIRECTION_LTR = 4,        /**< Left to Right */
//	TTF_DIRECTION_RTL,            /**< Right to Left */
//	TTF_DIRECTION_TTB,            /**< Top to Bottom */
//	TTF_DIRECTION_BTT             /**< Bottom to Top */
//} TTF_Direction;

struct text_ayout_t
{
	int direction;
	uint32_t script;
	int font_height;
	int* lines;
	int wrap_length;
	bool wrap_whitespace_visible;
};
struct text_data
{
	font_t* font = 0;
	text_style_t st = {};
	text_ayout_t* layout;
	int x, y, w, h;
	uint32_t props = 0;
	bool needs_engine_update;
	bool needs_layout_update;
};
// 文件对象
struct text_t
{
	char* text = 0;
	int num_lines = 0;
	text_data* internal = 0;
};

class internal_text_cx
{
public:
	text_t text = {};
	text_data internal = {};
	text_ayout_t layout = {};
public:
	internal_text_cx();
	~internal_text_cx();

private:

};


class font_rctx
{
public:
	/*
	struct fontns
	{
		std::string family;
		std::vector<std::string> alias, style, fpath;//别名，风格、文件路径
		std::vector<void*> vptr;//font_t对象
	};
	*/
	std::map<std::string, fontns> fyv;		// 系统字体表
	std::vector<fontns*> fyvs;				// 系统字体表
	std::map<std::string, font_t*> fzv;		// 自定义加载字体表
	font_imp* imp = 0;
	bitmap_cache_cx bcc = {};				// 纹理缓存
	font_t* current = 0;

	//PangoContext* pcontext = 0;
	//PangoLayout* layout = 0;
	//std::set<PangoLayout*> gclt;
	std::string temfamily = "";
	//std::string family = "NSimSun";
	//int fontsize = 12;

public:
	font_rctx();
	~font_rctx();
	int get_count();						// 获取注册的字体数量
	int get_count_style(int idx);			// 获取idx字体风格数量
	const char* get_family(int idx);		// 获取字体名称
	const char* get_family_en(const char* family);		// 获取字体名称
	const char* get_family_cn(int family);		// 获取字体中文名称
	const char* get_family_alias(int family);		// 获取字体中文名称
	const char* get_family_full(int idx);	// 获取字体全名
	const char* get_family_style(int idx, int stidx);	// 获取字体风格名
	font_t* get_font(int idx, int styleidx);// 通过索引号获取字体对象
	font_t* get_font(const char* family, const char* style);//通过字符串名获取
	font_t* get_font_cur();					//获取当前字体对象

	//void set_family_size(const std::string& fam, int fs);
	//text_layout_t get_text_layout(const std::string& str, text_layout_t* lt);
	//void draw_text(cairo_t* cr, text_layout_t* lt);
	//void free_textlayout(text_layout_t* lt);
public:
	font_t* get_mfont(const std::string& name);
	// 手动添加字体使用。纹理缓存同一个，但不能用上面函数查询
	std::vector<font_t*> add2file(const std::string& fn, std::vector<std::string>* pname);
	std::vector<font_t*> add2mem(const char* data, size_t len, bool iscp, std::vector<std::string>* pname);
	void free_font(const std::string& name);
private:
	font_t* get_mk(fontns& v, size_t st);	// 获取生成stb字体对象
};
font_rctx* new_fonts_ctx();
void free_fonts_ctx(font_rctx* p);

// 获取粘贴板文本
std::string get_clipboard();
// 设置粘贴板文本
void set_clipboard(const char* str);

struct quadratic_v
{
	glm::vec2 p0, p1, p2;
};
struct cubic_v
{
	glm::vec2 p0, p1, p2, p3;	// p1 p2是控制点
};
#ifndef path_v_dh
#define path_v_dh
#endif // !path_v_dh

class path_v;

namespace gpv {
	using Path64 = std::vector<glm::i64vec2>;
	using PathD = std::vector<glm::dvec2>;
	using Path = std::vector<glm::vec2>;
	using Paths64 = std::vector<Path64>;
	using Paths = std::vector<Path>;
	using PathsD = std::vector<PathD>;
	// 输入路径，输出细分圆角路径线，ccw=0,1,全部角度-1
	Paths path_round_0(path_v* ptr, int ccw, float radius, int num_segments, int ml, int ds);
	PathsD path_round_1(path_v* ptr, int ccw, float radius, int num_segments, int ml, int ds);
}

namespace gp {
	using plane3_t = std::vector<glm::vec3>;

	// 获取圆角弧度a的坐标，圆心x=0,y=0
	glm::vec2 get_circle(float a, float r);
	glm::vec3 get_circle(float a, const glm::vec3& c);

	// 输入三点，角点ptr2，输出圆
	glm::vec3 get_circle3(glm::vec2 pt1, glm::vec2 pt2, glm::vec2 pt3, float radius);
	// 获取点p到圆的两条切线
	glm::vec4 gtan(const glm::vec2& P, const glm::vec3& c);

	struct cmd_plane_t
	{
		path_v* pv = 0;					// 路径，优先级0
		gpv::Paths* fv = 0;				// 自定义折线路径，1
		gpv::PathsD* dv = 0;			// 自定义折线路径，2
		plane3_t* opt = 0;				// *输出三角形
		int type = 2;					// 扩展大小可选类型Square=0, Round=1, Miter=2
		int radius = 0;					// 圆角半径
		glm::ivec2 rccw = {}; 			// 圆角顺序
		int pccw = 0;					// 面的顺序
		int segments = 8;				// 段数
		float segments_len = 0.0;		// 细分长度
		float dist = 0.0;				// 细分裁剪距离
		float thickness = 1.0;			// 厚度
		int radius_a = 150;				// 小于角度执行圆角
		bool is_expand = 0;				// 1扩展（顶点数不一致）、0比例缩放（有自相交问题）
	};

	int get_flatten(tinypath_t* p, int m, float ml, float ds, std::vector<glm::vec2>* flatten);
	void constrained_delaunay_triangulation_v(std::vector<std::vector<glm::vec2>>* paths, std::vector<glm::vec3>& ms, bool pccw, double z);
	void constrained_delaunay_triangulation_v(std::vector<std::vector<glm::vec2>>* paths, std::vector<glm::vec3>& vd, std::vector<glm::ivec3>& idxs, bool pccw, double z);

	void cdt_pt(glm::vec3* pt, int n, /*double tolerance,*/ std::vector<glm::vec3>& ms, bool pccw);
	void cdt_pt(glm::vec3* pt, int n,/* double tolerance,*/ std::vector<glm::vec3>* ms, bool pccw);
	void cdt_pt(glm::vec2* pt, int n, /*double tolerance,*/ std::vector<glm::vec3>* msp, bool pccw);

	// 单线面，先扩展后比例
	int build_plane1(cmd_plane_t* c, float expand, float scale, float z);
	// 单面打孔，tr.x=半径，tr.y=间隔
	int build_plane1hole(cmd_plane_t* c, float scale, float z, const glm::vec2& tr, std::vector<glm::vec2>* cshape, std::vector<glm::ivec2>* idx, std::vector<glm::vec3>* vpos);
	// 打孔双路径面
	int build_plane2hole(cmd_plane_t* c, float scale, float expand, float z, const glm::vec2& tr, bool rv, std::vector<glm::vec2>* cshape, std::vector<glm::ivec2>* idx, std::vector<glm::vec3>* vpos);
	// 缩放2裁剪面
	int build_plane2sc(cmd_plane_t* c, float scale, float scale1, float z, bool rv);
	// 双路径面， 比例扩展两条路径成竖面
	int build_plane3(cmd_plane_t* c, const glm::vec2& scale, const glm::vec2& z);
	int build_plane3(cmd_plane_t* c, float scale, const glm::vec2& z);
	// 生成竖面，单线
	int build_plane0(cmd_plane_t* c, const glm::vec2& scale, const glm::vec2& z);

	// 获取细分后的曲线
	struct pv_st {
		path_v* v = 0;
		int count = 0;
	};
	pv_st build_plane3_v(cmd_plane_t* c, const glm::vec2& scale, const glm::vec2& z, const std::string& fn, void (*cb)(path_v* pv, int count, const std::string& fn, bool fy));

	// 命令
	struct dv_cmd_t
	{
		glm::vec2 expand;		// 扩展大小
		glm::ivec2 ccw;			// 圆角顺序
		glm::vec2 plane_z;		// 面高度
		glm::vec2 radius;		// 圆角半径大小 
		glm::ivec2 plane_ccw;	// 面的顺序
		int plane_type;			// 面类型，1是单路径面，2是双路径面
		void set_plane1(float expand, bool ccw, float z, float radius, int v_ccw1, int ptype);
		void set_plane2(float expand, bool ccw, float z, float radius, int v_ccw1);
	};
	// 墙
	struct dv_wall_t
	{
		glm::vec2 expand;
		glm::vec2 height;
		glm::vec2 r;			// 圆角
		glm::ivec2 ccw;			// 圆角顺序
		int plane_ccw = 0;		//0=外墙outer,1=内墙inner
	};
	typedef std::vector<dv_cmd_t> vec_cmd;
	typedef std::vector<dv_wall_t> vec_wall;
	// 上下封闭输入结构
	struct closed_t
	{
		int type = 0;		// 0为顶，1为底
		// 封闭厚度
		float thickness = 0;
		// 台阶 宽、高、厚度、方向(0=上平，1下平)
		glm::vec4 step = {};
	};
	struct mkcustom_dt
	{
		// 壁曲线参数
		int m = 20;
		int ct = 0;
		std::vector<glm::vec2> v, v1;

		// 台阶参数
		glm::vec2 step_expand = {};
		glm::vec2 step_expand0 = {};
		glm::vec4 step = {};				// 上台阶, 端面厚度x，-1则无封面。4个参数则台阶
		glm::vec4 step1 = {};				// 下台阶
		glm::vec2 step2 = {};
	};
	struct base_mv_t
	{
		// 大小，xy宽深度为0则不改，z高度
		glm::vec3 box_size = { 0, 0, 20 };
		// 尺寸缩放比例，默认1不缩放
		glm::vec3 box_scale = { 1.0,1.0,1.0 };
		// 分段数量
		int segments = 12;
		float ds = 1.0;
		// 轮廓厚度默认1
		float thickness = 1.0;
		// 0不倒角, 圆角半径
		int radius = 0;
	};
	// 生成B样条线约束的竖三角面
	glm::vec4 mkcustom(mkcustom_dt* np, glm::vec2 k, base_mv_t& bm, cmd_plane_t* c, const glm::uvec2& bcount = { -1,-1 });

	// 多边形
	struct mesh_mt
	{
		std::vector<uint32_t> face_size;		// 面的边数3\4、多边形
		std::vector<uint32_t> face_indice;		// 索引
		std::vector<double> vertex_coord;		// 顶点坐标
	public:
		// 查找循环边，输入一个边序号，返回边
		std::vector<glm::uvec2> find_edge_loop(uint32_t idx);
		// 查找循环面，输入一个面序号，返回面
		std::vector<glm::uvec4> find_face_loop(uint32_t idx);

		void add_vertex(const glm::dvec3* v, size_t n);
		void add_vertex(const glm::vec3* v, size_t n);
	};
	// 面编辑结构
	struct edit_mesh_t
	{
		struct halfedge_t
		{
			glm::uvec2 indice = {};					//顶点索引，x=开头，y下一点索引
			size_t front_half, back_half, rev_half;	// 前后反半边
			size_t face_idx;						// 面集合序号
		};
		struct face_t {
			glm::uvec2 half[4]; // 半边集合索引，最多支持四边形

		};
		std::map<glm::uvec2, size_t> _m;		// 半边集合，key{点索引，下一点索引}，数据索引
		std::vector<halfedge_t> _data;			// 半边集合数据
		std::vector<face_t> _face;				// 面集合
		mesh_mt* mesh = 0;						// 网格数据
		// 生成半边
		void build_halfedge();
	};
	// 三角形
	struct mesh3_mt
	{
		std::vector<glm::ivec3>	indices;	// 三角形索引
		std::vector<glm::vec3>	vertices;	// 顶点坐标
	};


	// 挤出、倒角
	struct extrude_bevel_t {
		float bevel_width = 0;	// 挤出宽度
		float depth = 0;		// 深度
		float count = 5;		// 分辨率
		float thickness = 1.0;	// 厚度
		glm::ivec2 type = { 0,1 };	//样式  x.0=v，1=U，2=|_|，y=-1倒过来
	};
	struct line_style_t {
		float depth = 0;		// 深度
		float count = 5;		// 分辨率
		float thickness = 1.0;	// 厚度
		float bottom_thickness = 0.0;//封底厚度
		glm::ivec2 type = { 0,1 };	//样式 x.0=v，1=U，2=|_|，y=-1倒过来，
	};
	// 生成3D扩展线模型
	void build_line3d(const glm::vec3& pos1, const glm::vec3& pos2, const glm::ivec2& size, line_style_t* style, mesh_mt* opt);

	// 多边形三角化，输出到3角面网络opt，自动判断洞，，pccw是否置反三角面，type 0=CDT,1=割耳法
	int triangulates(std::vector<glm::vec2>* poly, int n, bool pccw, mesh_mt* opt, int type);

}
// todo path_v
class path_v
{
public:
	// 线，二阶曲线，三阶曲线
	enum class vtype_e :uint8_t
	{
		e_vmove = 1,
		e_vline,
		e_vcurve,
		e_vcubic
	};
	struct vertex_t
	{
		// 24字节
		glm::vec2 p, c, c1;
		// 4字节
		vtype_e type;
		uint8_t padding;
		uint16_t padding1;
	};
	struct vertex_tf
	{
		// 24字节
		float x, y, cx, cy, cx1, cy1;
		// 4字节
		vtype_e type;
		uint8_t padding;
		uint16_t padding1;
	};
	struct flatten_t
	{
		path_v::vertex_t* first = 0;
		size_t n = 0;
		int mc; float mlen;
		std::vector<glm::vec2>* flatten;
		float dist = 0;			// 距离剔除
		int angle = 100;		// 优化小于100度的
		bool oldfp = false;
	};
	std::vector<vertex_t> _data;
	// xy最小，zw最大
	glm::vec4 _box = { INT_MAX,INT_MAX,INT_MIN,INT_MIN };
	// 坐标
	glm::vec2 _pos = {};
	uint64_t ud = 0;
	int _baseline = 0;
	wchar_t text = 0;
	int angle = 100;
	bool oldexp = false;
	bool flip_y = false;	//生成模型时翻转y
	bool cbox = false;
public:
	path_v();
	~path_v();
	void swap(path_v& v);
	vertex_t* data();
	// 端点数量
	size_t size();
	// 数据长度
	size_t dsize();
	// 曲线条数
	size_t mcount();
	// 开始一条路径
	void moveTo(const glm::vec2& p);
	// 二次
	void quadraticCurveTo(const glm::vec2& cp0, const glm::vec2& p);
	// 三次
	void bezierCurveTo(const glm::vec2& cp0, const glm::vec2& cp1, const glm::vec2& p);
	void curveTo(const glm::vec2& cp0, const glm::vec2& p);
	void cubicTo(const glm::vec2& cp0, const glm::vec2& cp1, const glm::vec2& p);
	void lineTo(const glm::vec2& p);
	// 闭合一条曲线
	void closePath();

	//#define c_KAPPA90 (0.5522847493f)	// Length proportional to radius of a cubic bezier handle for 90deg arcs.

	void addCircle(const glm::vec2& c, double r);
	void addCircle(const glm::vec3* c, size_t n);
	void addCircle(const glm::vec2* c, size_t n, double r);
	void addEllipse(const glm::vec2& c, const glm::vec2& r);
	void addRect(const glm::vec4& a, const glm::vec2& r);
	void add(const vertex_t& v);
	void set_data(const vertex_t* d, size_t size);
	void set_data(const tinypath_t* d, int count);
	void add_lines(const glm::vec2* d, size_t size, bool isclose = false);
	void add_lines(const glm::dvec2* d, size_t size, bool isclose = false);

	void insert(size_t idx, const vertex_t& v);
	void erase(size_t idx);

	void moveTo(double x, double y);
	void lineTo(double x, double y);
	void curveTo(double x, double y, double cx, double cy, double c1x, double c1y);
	// 转成曲线
	void lineTo2(double x, double y);
	void lineTo2(const glm::vec2& p);
	void cubicBezTo(double cpx1, double cpy1, double cpx2, double cpy2, double x, double y);
	// 获取指定idx的一条线首指针
	vertex_t* getline2(int x, size_t* px);
	// 反转指定一条线
	void reverse1(int idx);
	void reverse_all();
	// 设置矩阵
	void set_mat(const glm::mat3& m);
public:
	static glm::vec2 rotate_pos(const glm::vec2& pos, const glm::vec2& center, double angle);
	//向量外积
	static double cross_v2(const glm::vec2& a, const glm::vec2& b);
	// 扩展多边形,会有自相交bug，但顶点数相同
	static void expand_polygon(glm::vec2* polygon, int count, float expand, std::vector<glm::vec2>& ots);

	//type取{Square=0, Round=1, Miter=2}顶数会不同
	static void expand_polygon_c(glm::vec2* polygon, int count, float expand, int type, std::vector<glm::vec2>& ots);
public:
	// 获取整体大小
	glm::vec2 get_size();
	glm::vec4 mkbox();
	glm::vec4 mkbox_npos();
	void add(path_v* p);
	void incpos(const glm::vec2& p);
	void mxfy(double fy);
	// 曲线序号，段数，输出std::vector<glm::vec2/glm::dvec2>，type=0，1是double
	//int get_flatten(size_t idx, size_t count, int m, void* flatten, int type);
	int get_flatten(size_t idx, size_t count, int m, float ml, float ds, std::vector<glm::vec2>* flatten);

	// 获取扩展路径,输出到opt，空则修改自身。顶点数相同，注意自相交
	int get_expand(float width, path_v* opt);

	// type取{Square=0, Round=1, Miter=2} ，不同width会使顶点数不同
	int get_expand_flatten(size_t idx, float width, int segments, float ml, float ds, int type, std::vector<std::vector<glm::vec2>>* ots, std::vector<int>* ccwv);
	int get_expand_flatten(float width, int segments, int type, std::vector<std::vector<glm::vec2>>* iod);

	int get_expand_flatten2(float expand, float scale, int segments, float ml, float ds, int type, std::vector<std::vector<glm::vec2>>* ots, bool is_round, bool is_close = 1);

	int get_expand_flatten3(float expand, float scale, int segments, float ml, float ds, int type, int etype, std::vector<std::vector<glm::vec2>>* ots, bool is_close);

	// 三角化，曲线细分段，是否转置成反面0/1，输出到ms数组。type=0cdt，1=割耳
	int triangulate(int segments, float ml, float ds, bool pccw, std::vector<glm::vec2>* ms, int type = 0);
	// 获取每条边的中心点拉高z
	int get_triangulate_center_line(int segments, float ml, float ds, int is_reverse, const glm::vec2& z, std::vector<glm::vec3>* ms);

	bool is_ccw(int idx);
private:

};
void save_png_v(path_v* pv, int count, const std::string& fn, bool fy, float sc);

struct text_path_t
{
	std::vector<tinypath_t> tv;	// 每个字的路径
	std::vector<vertex_f> data;	// 所有路径数据
};
struct text_image_t
{
	std::vector<font_item_t> tv;

};
struct text_atlas_t
{
	atlas_t atlas = {};
	image_ptr_t ipt = {};
};
class gshadow_cx;
class plane_cx;

struct pvm_t
{
	atlas_cx* back = 0;	// 背景
	atlas_cx* front = 0;	// 前景
	plane_cx* p = 0;		// 控件
	glm::vec2 fsize = {};
	glm::vec2 cpos = {};
	int w, h;
};
// todo 字体纹理缓存管理
class layout_text_x
{
public:
	font_rctx* ctx = 0;
	std::vector<std::vector<font_t*>> familyv;
	std::vector<glm::ivec3> cfb;
	int fdpi = 72;
	int heightline = 0;		// 固定行高
	text_path_t ctp = {};	// 临时缓存
	text_image_t cti = {};
	std::vector<cairo_surface_t*> msu;
	std::vector<font_item_t> tv;
	std::vector<font_item_t> tem_rtv;	// 临时缓存用
	// todo
	std::vector<atlas_cx> tem_iptr;
	std::vector<float> tem_pv;
	glm::ivec2 ctrc = {}, oldrc = {};

	image_sliced_t sli = {};
	int sli_radius = 10;
	gshadow_cx* gs = 0;
	// 菜单边框填充：线颜色，线粗，圆角，背景色
	glm::ivec4 m_color = { 0xff606060,1,0,0xf0121212 };
	// 菜单项偏移
	glm::ivec2 m_cpos = { 3, 3 };
public:
	layout_text_x();
	~layout_text_x();
	void set_ctx(font_rctx* p);
	// 添加字体,返回序号
	size_t add_familys(const char* familys, const char* style);
	void cpy_familys(layout_text_x* p);
	void clear_family();
	void clear_text();

	// 获取基线
	int get_baseline(size_t idx, int fontsize);
	// 获取行高
	int get_lineheight(size_t idx, int fontsize);
	// 获取文本区域大小,z为基线
	glm::ivec3 get_text_rect(size_t idx, const void* str8, int len, int fontsize);
	glm::ivec3 get_text_rect1(size_t idx, int fontsize, const void* str8);
	int get_text_pos(size_t idx, int fontsize, const void* str8, int len, int xpos);
	int get_text_ipos(size_t idx, int fontsize, const void* str8, int len, int ipos);
	int get_text_posv(size_t idx, int fontsize, const void* str8, int len, std::vector<std::vector<int>>& ow);
	// 添加文本到渲染
	glm::ivec2 add_text(size_t idx, glm::vec4& rc, const glm::vec2& text_align, const void* str8, int len, int fontsize);
	glm::ivec2 build_text(size_t idx, glm::vec4& rc, const glm::vec2& text_align, const void* str8, int len, int fontsize, std::vector<font_item_t>& rtv);
	// 输出到图集
	void text2atlas(const glm::ivec2& r, uint32_t color, std::vector<atlas_cx>* opt);
	// 获取路径数据
	text_path_t* get_shape(size_t idx, const void* str8, int fontsize, text_path_t* opt);
	// 获取渲染数据
	text_image_t* get_glyph_item(size_t idx, const void* str8, int fontsize, text_image_t* opt);
	// 渲染部分文本
	void draw_text(cairo_t* cr, const glm::ivec2& r, uint32_t color);
	void draw_text(cairo_t* cr, const std::vector<font_item_t>& r, uint32_t color);

	void draw_rect_rc(cairo_t* cr, const std::vector<font_item_t>& rtv, uint32_t color);
	// 渲染全部文本
	void draw_text(cairo_t* cr, uint32_t color);
	// todo获取图集
	atlas_t* get_atlas();
	bool update_text();
	// 创建阴影
	atlas_cx* new_shadow(const glm::ivec2& ss, const glm::ivec2& pos);
	// 创建菜单
	pvm_t new_menu(int width, int height, const std::vector<std::string>& v, bool has_shadow, std::function<void(int type, int id)> cb);
	pvm_t new_menu(int width, int height, const char** v, size_t n, bool has_shadow, std::function<void(int type, int id)> cb);
	void free_menu(pvm_t pt);
private:
	void c_line_metrics(size_t idx, int fontsize);
};
// auto save 
class cairo_as
{
public:
	cairo_as(cairo_t* p);
	~cairo_as();

private:
	cairo_t* cr = 0;
};

class canvas_dev
{
public:
	cairo_surface_t* surface = 0;
	cairo_t* cr = 0;
	uint32_t* pixel = 0;
	glm::ivec2 dsize = {};
	// 全局偏移、缩放
	glm::vec2 translate = { 0.0f,0.0f };
	glm::vec2 scale = { 1.0f,1.0f };
	double angle = 0;
	int max_zoom = 25600;//最大百分比
	//std::vector<vertex_t32>* tv = 0;
	std::vector<uint32_t> tdata;
public:
	canvas_dev();
	~canvas_dev();

	static canvas_dev* new_cdev(const glm::ivec2& size, uint32_t* data);
	static canvas_dev* new_cdev_svg(const glm::ivec2& size, const char* svgfn);
	static canvas_dev* new_cdev_dc(void* hdc);
	static void free_cdev(canvas_dev* p);
	// 保存图像到文件 
	void save_png(const char* fn);
	// 使用颜色清屏
	void set_clear(uint32_t color);
	// 渲染一条路径 


	void draw_path(path_v* p, style_path_t* st);

	void draw_circle(std::vector<std::vector<glm::vec2>>* ov, float r, style_path_t* st);
	void draw_rect(const glm::vec2&, style_path_t* st);
	// 画多个圆
	void draw_circle(std::vector<glm::vec2>* ov, float r, style_path_t* st);
	// 两个坐标一条线的多条线
	void draw_line(std::vector<glm::vec2>* ov, style_path_t* st);
	void draw_line(const glm::vec4& ov, uint32_t color, float linewidth);
	void draw_line(const glm::vec2& t, const glm::vec2& t1, uint32_t stroke_color, float linewidth);
	// 一条线
	void draw_line1(std::vector<glm::vec2>* ov, style_path_t* st);
	void draw_text(const void* str, const glm::vec2& pos, font_xi* fx);
	void draw_circle(const glm::vec2& pos, float r, float linewidth, uint32_t color, uint32_t fill);

	void draw_surface(cairo_surface_t* image, const glm::vec2& pos, double alpha);


	int get_path(path_v* pt);

	glm::vec4 get_path_extents();
	glm::ivec4 get_text_extents(const void* str, int len, font_xi* fx);
	void begin(uint32_t color);
	void end();
	void save();
	void set_pos(const glm::vec2& pos);
	void set_scale(double sc);
	void set_scalei(int& zoom);
	void set_dscale(const glm::vec2& sc);
	void set_dscale(double sc);
	void set_dscalei(int& zoom);
	uint32_t* data();
private:

};
struct et_un_t;
class tinyviewcanvas_x;
class Ruler;
class view_g
{
public:
	glm::ivec2 vsize = { 1000,800 };	//视图大小
	glm::ivec2 vgpos = { 0,0 };			//视图全局坐标
	tinyviewcanvas_x* vcanvas = 0;		//画板
	canvas_dev* cdv = 0;
	font_xi fx = {};
	Ruler* ruler = {};					// 标尺
	glm::vec4 pagebox = {}, selbox = {};	// 页面框、选中框
	// 方框大小、坐标
	glm::vec2 wsize = { 800,600 };
	glm::vec2 vpos = { 500,400 };
	// 标尺大小
	glm::vec2 rsize = { 20, 20 };
	int fh = 12;	//字高
	// 背景格宽
	int bwidth = 8;
	// 1单位宽度
	glm::ivec3 step_width = { 20,20,5 };
	// 背景色x、线框颜色y、背景格颜色z
	glm::ivec4 color = { 0xff505050, 0x80FF8050,0x80353535,0x80606060 };
	glm::ivec4 wcolor = { -1, 0xff000000,-1 ,0 };//填充、线、方框线
	uint32_t clear_color = 0xff1b1b1b;
	uint32_t hover_color = 0x800000ff;
	glm::ivec2 cross_color = { 0xff503d85, 0xff327660 };
	int cross_width = 2;
	// 当前鼠标坐标
	glm::ivec2 curpos = {};
	//btype_e btype = {};		// 按钮状态
	path_v pvdata[2] = {};
	cairo_surface_t* _backing_store = 0;
	cairo_surface_t* rss = 0;
	bool _backing_store_valid = false;
	bool _draw_valid = true;
	bool is_yaxisdown = 1;		// 1=y向下
	bool is_ck = false;
public:
	view_g();
	~view_g();
	static view_g* new_view(const glm::vec2& size);
	static void free_view(view_g* p);
	void set_view(const glm::ivec2& size);

	void update(float a = 0.0);
	void begin_draw();
	void draw();
	void end_draw();
	glm::mat3 get_mat();
	glm::mat3x2 get_mat3x2();
	glm::dmat3x2 get_dmat3x2();
	// 发送事件到
	void on_event(uint32_t type, et_un_t* ep);
private:
	void on_motion(const glm::vec2& pos);
	void on_button(int idx, int down, const glm::vec2& pos, int clicks);
	void on_wheel(double x, double y);
	void updateRulers();
	void mkpath();
	void draw_grid_back(const glm::ivec2& vss);
};

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
		ALIGN_SPACE_EVENLY
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

	flex_align justify_content = flex_align::ALIGN_START;	// 父元素:主轴上的元素的排列方式 start\end\center\space-between\space-around
	flex_align align_content = flex_align::ALIGN_STRETCH;	// 父元素:适用多行的flex容器 start\end\center\space-between\space-around\stretch 
	flex_align align_items = flex_align::ALIGN_STRETCH;		// 父元素:副轴上的元素的排列方式 start\end\center\stretch 
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

class grid_view
{
public:
	// 行列的宽高
	std::vector<glm::vec2> _column_width;	// 列宽，x宽，y为本列坐标缓存
	std::vector<glm::vec2> _row_height;		// 行高，x高，y为本行坐标缓存
	glm::vec2 _pos = {};
	glm::vec2 _size = {};
	bool valid = true;
public:
	grid_view();
	~grid_view();
	// 设置列行数量
	void set_size(size_t x, size_t y);
	void add_col(int width);
	void add_row(int height);
	// 设置列宽，idx为列索引，v宽度
	void set_width(size_t idx, float v);
	// 设置行高，idx为列索引，v行高
	void set_height(size_t idx, float v);
	// 获取总大小
	glm::vec2 get_size();
	// 获取指定坐标单元格的坐标/大小。
	glm::vec4 get(const glm::ivec2& pos);
private:

};


enum class WIDGET_TYPE :uint32_t {
	WT_NULL,
	WT_EDIT,
	WT_COLOR_BTN, WT_IMAGE_BTN, WT_GRADIENT_BTN,
	WT_RADIO,
	WT_CHECKBOX,
	WT_SWITCH,
	WT_PROGRESS,
	WT_SLIDER,
	WT_COLORPICK,
	WT_SCROLL_BAR,
};

// 判断拾取
struct pickup_t
{
	glm::vec2 pos = {};		// 坐标
	glm::vec3 angle = {};	// 距离x-y，角度z

};
//  cb;支持的type有on_move/on_scroll/on_drag/on_down/on_up/on_click/on_dblclick/on_tripleclick
struct widget_base
{
public:
	int id = 0;
	WIDGET_TYPE wtype = WIDGET_TYPE::WT_NULL;
	int bst = 1;			// 鼠标状态
	glm::vec2 pos = {};		// 控件坐标
	glm::vec2 size = {};	// 控件大小
	glm::ivec2 curpos = {};	// 当前拖动鼠标坐标
	glm::ivec2 cmpos = {};	// 当前鼠标坐标
	glm::ivec2 mmpos = {};	// 当前鼠标坐标
	glm::ivec2 ppos = {};	// 父级坐标 
	std::string text;		// 内部显示用字符串
	std::string family;
	float font_size = 16;
	glm::vec2 text_align = { 0.5,.5 }; // 文本对齐
	int rounding = 4;		// 圆角
	float thickness = 1.0;	// 边框线粗
	std::function<void(void* p, int type, const glm::vec2& mps)> mevent_cb;	//通用事件处理
	std::function<void(void* p, int clicks)> click_cb;						//左键点击事件
	int _clicks = 0;		// 点击数量
	layout_text_x* ltx = 0;

	glm::ivec2 hscroll = { 1,1 };// x=1则受水平滚动条影响，y=1则受垂直滚动条影响
	int _old_bst = 0;			// 鼠标状态
	int cks = 0;				// 鼠标点击状态
	plane_cx* parent = 0;
	double dtime = 0.0;
	bool _disabled_events = false;
	bool visible = true;
	bool _absolute = false;		// true绝对坐标，false布局计算
	bool has_drag = false;	// 是否有拖动事件
	bool _autofree = false;
	bool has_hover_sc = 0;	// 滚动在父级接收
public:
	widget_base();
	widget_base(WIDGET_TYPE wt);
	virtual ~widget_base();
	//event_type2
	virtual bool on_mevent(int type, const glm::vec2& mps);
	virtual bool update(float delta);
	virtual void draw(cairo_t* cr);
	virtual glm::ivec2 get_pos(bool has_parent = true);
};


// 输入布局样式等信息+文字
#ifndef NO_EDIT

class text_ctx_cx;

class edit_tl :public widget_base
{
public:
	text_ctx_cx* ctx = 0;							// 布局/渲染/事件处理
	std::string _text;								// 保存行文本
	std::string ipt_text;								// 输入缓存文件
	std::function<void(edit_tl* ptr)> changed_cb;	// 文本改变时执行回调函数 
	std::function<void(edit_tl* ptr, std::string& str)> input_cb;	// 文本输入时执行回调函数，可修改此字符串返回
	int _istate = 0;
	bool single_line = false;
	bool mdown = false;
	bool _read_only = false;
	bool is_input = false;
public:
	edit_tl();
	~edit_tl();
	void set_single(bool is);
	// 设置为密码框比如'*'
	void set_pwd(char ch);
	// 设置utf8文本
	void set_text(const void* str, int len);
	void add_text(const void* str, int len);
	// 设置文本框大小
	void set_size(const glm::ivec2& ss);
	// 设置文本框坐标
	void set_pos(const glm::ivec2& pos);
	void set_align_pos(const glm::vec2& pos);
	void set_align(const glm::vec2& a);
	// 闪烁光标。宽度、颜色、毫秒
	void set_cursor(const glm::ivec3& c);
	// 背景色、文本颜色、选择背景色、输入法编辑文本颜色
	void set_color(const glm::ivec4& c);
	void set_family(int fontid, int fontsize);
	// 设置是否显示输入光标
	void set_show_input_cursor(bool ab);
	// 设置自动换行
	void set_autobr(bool ab);
	void set_round_path(float v);
	// 删除位置，字符数量
	void remove_char(size_t idx, int count);
	// 删除选择的文本
	bool remove_bounds();
	// 发送事件到本edit
	void on_event_e(uint32_t type, et_un_t* e);
	// 更新渲染啥的
	bool update(float delta);
	// 获取纹理/或者渲染到cairo
	image_ptr_t* get_render_data();
	void draw(cairo_t* cr);

	glm::ivec4 input_pos();
	std::string get_select_str();
	std::wstring get_select_wstr();
private:
	void on_keyboard(et_un_t* ep);
	void inputchar(const char* str);
};

input_state_t* get_input_state(void* ptr, int t);

#endif // !NO_EDIT
#if 1


// 控件相关

struct btn_cols_t {
	uint32_t font_color = 0xFF222222;
	uint32_t background_color = 0xFFffffff;
	uint32_t border_color = 0xFF222222;
	uint32_t hover_color = 0xFFf78989;
	uint32_t active_font_color = 0xFFe6e6e6;
	uint32_t active_background_color = 0xFFf2f2f2;
	uint32_t active_border_color = 0;// 0xFF3c3c3c;
	uint32_t hover_border_color = 0;
};
// todo tag 标签,用于标记和选择。0蓝，1青，2灰，3橙，4红
enum class uType :uint8_t {
	primary, success, info, warning, danger
};
// 填充颜色字白色，边框字颜色中间半透明，边框字颜色
enum class uTheme :uint8_t {
	dark, light, plain,
};
/*
1.普通状态2，鼠标hover状态  3.active 点击状态  4.focus 取得焦点状态  4.disable禁用状态
*/
enum class BTN_STATE :uint8_t
{
	STATE_NOMAL = BIT_INC(0),
	STATE_HOVER = BIT_INC(1),
	STATE_ACTIVE = BIT_INC(2),
	STATE_FOCUS = BIT_INC(3),
	STATE_DISABLE = BIT_INC(4),
};

// todo图片按钮
struct image_btn :public widget_base {
	std::string str;
	image_ptr_t* img = 0;
	image_sliced_t state_img[5] = {};
	std::vector<glm::ivec4> data;
	int show_idx = 0;	// 参考BTN_STATE
public:
	image_btn();
	~image_btn();
	bool on_mevent(int type, const glm::vec2& mps);
	bool update(float delta);
	void draw(cairo_t* cr);
};
// 纯色按钮
struct color_btn :public widget_base
{
	std::string str;
	float light = 0.512;
	btn_cols_t pdc = {};			// 颜色配置

	uint32_t dfill = 0, dcol = 0;	// 渲染用
	uint32_t dtext_color = 0;		// 渲染用
	uint32_t text_color = 0xffffffff;
	int disabled_alpha = 0x30;
	uTheme effect = uTheme::dark;
	glm::vec2 pushedps = {};

	bool _circle = false;			// 圆形按钮
	bool _disabled = false;
	bool mPushed = false;
	bool hover = false;
	bool bgr = 0;
public:
	color_btn();
	~color_btn();
	btn_cols_t* set_btn_color_bgr(size_t idx);

	bool update(float delta);
	void draw(cairo_t* cr);
};

// 渐变按钮

struct gradient_btn :public widget_base
{
	std::string str;
	//const char* icon = nullptr;
	//icon_rt icon = {};
	//size_t len = 0; 
	uint32_t back_color = 0;
	uint32_t text_color = 0;
	uint32_t text_color_shadow = 0x88111111;
	double opacity = 1.0;
	// private
	uint32_t gradTop = 0;
	uint32_t gradBot = 0;
	uint32_t borderLight = 0;
	uint32_t borderDark = 0;
	uTheme effect = uTheme::light;	// dark
	bool mPushed = false;
	bool mChecked = false;
	bool mMouseFocus = false;
	bool mEnabled = true;
	bool is_muilt = true;
public:
	gradient_btn();
	~gradient_btn();
	const char* c_str();
	void init(glm::ivec4 rect, const std::string& text, uint32_t back_color = 0, uint32_t text_color = -1);

	bool update(float delta);
	void draw(cairo_t* cr);
};

struct radio_style_t
{
	uint32_t col = 0xffFF9E40, innc = 0xffffffff, text_col = 0xff666666;
	uint32_t line_col = 0xff4c4c4c;
	float radius = 7;
	float thickness = 1.0;
	float duration = 0.28;	// 动画时间 
};

struct check_style_t {
	uint32_t col = 0xffFF9E40;
	uint32_t fill = 0xffff8020;
	uint32_t check_col = 0xffffffff;
	uint32_t line_col = 0xff4c4c4c;
	uint32_t text_col = 0xff666666;
	float rounding = 2;
	float square_sz = 14;
	float thickness = 1.0;
	float duration = 0.28;	// 动画时间 
};
struct radio_info_t
{
	glm::vec2 pos;	// 坐标
	std::string text;
	std::function<void(void* p, bool v)> on_change_cb;
	float swidth = 0.;
	float dt = 0;	// 动画进度
	bool* pv = 0;
	bool value = 0; // 选中值
	bool value1 = 1; // 选中值动画
};
struct checkbox_info_t
{
	glm::vec2 pos;	// 坐标
	std::string text;
	std::function<void(void* p, bool v)> on_change_cb;
	float dt = 0;	// 动画进度
	float duration = 0.28;	// 动画时间
	float new_alpha = -1;		// 动画控制	
	bool* pv = 0;
	bool mixed = false;			// 是否满
	bool value = 0; // 选中值
	bool value1 = 1; // 选中值动画
};
struct radio_tl;
struct group_radio_t
{
	radio_tl* active = 0;	// 激活的radio
	int ct = 0;				// 引用计数
};
// 单选
struct radio_tl :public widget_base
{
	radio_style_t style = {};	// 风格id
	radio_info_t v;
	group_radio_t* gr = 0;		// 组 
public:
	radio_tl();
	~radio_tl();
public:
	void bind_ptr(bool* p);
	void set_value(const std::string& str, bool v);
	void set_value(bool v);
	void set_value();

	bool on_mevent(int type, const glm::vec2& mps);
	bool update(float delta);
	void draw(cairo_t* cr);
};
// 复选
struct checkbox_tl :public widget_base
{
	check_style_t style = {};	// 风格id
	checkbox_info_t v;
public:
	checkbox_tl();
	~checkbox_tl();
	void bind_ptr(bool* p);
	void set_value(const std::string& str, bool v);
	void set_value(bool v);
	void set_value();

	bool on_mevent(int type, const glm::vec2& mps);
	bool update(float delta);
	void draw(cairo_t* cr);
};

// 开关
struct switch_tl :public widget_base
{
	glm::ivec3 color = { 0xffff9e40, 0xff4c4c4c,-1 }; // 开/关/圆点颜色 { 0xff66ce13, 0xff4949ff };
	glm::ivec2 text_color = { 0xffff9e40, 0xff4c4c4c }; // 文本颜色;
	uint32_t dcol = 0;	// 渲染的颜色 
	float cpos = 0;		// 动画坐标
	float cv = 0.7;		// 圆点大小 
	float height = 20;
	float wf = 2.1;		// 宽比例 
	checkbox_info_t v = {};
	bool inline_prompt = false;
public:
	switch_tl();
	~switch_tl();
	void bind_ptr(bool* p);
	void set_value(bool b);
	void set_value();

	bool on_mevent(int type, const glm::vec2& mps);
	bool update(float delta);
	void draw(cairo_t* cr);
};
// 进度条
struct progress_tl :public widget_base
{
	std::string format;				// 格式
	glm::vec2 vr = { 0, 100 };		// 范围
	glm::ivec2 color = { 0xffff9e40, 0x806c6c6c };//前景色，背景色
	uint32_t text_color = 0xffffffff;
	double value = 0.0;				// 当前进度
	int width = 0;					// 宽度
	int right_inside = 0;			// 右对齐
	bool text_inside = true;
public:
	progress_tl();
	~progress_tl();
	void set_value(double b);
	void set_vr(const glm::ivec2& r);
	double get_v();

	bool on_mevent(int type, const glm::vec2& mps);
	bool update(float delta);
	void draw(cairo_t* cr);
};
// 滑块
struct slider_tl :public widget_base
{
	glm::vec2 vr = { 0, 100 };		// 范围
	glm::ivec2 color = { 0xffff9e40, 0x806c6c6c };//前景色，背景色 
	glm::ivec2 sl = { 6,0xff363636 };	// 滑块半径颜色
	int wide = 0;
	double value = 0.0;				// 当前进度
	double* pv = 0;
	int vertical = 0;				// 垂直模式1
	bool reverse_color = 0;
public:
	slider_tl();
	~slider_tl();
	void bind_ptr(double* p);
	void set_value(double b);
	void set_vr(const glm::ivec2& r);
	// 设置圆大小
	void set_cw(int cw);
	double get_v();

	bool on_mevent(int type, const glm::vec2& mps);
	bool update(float delta);
	void draw(cairo_t* cr);
};
// 颜色控件
struct colorpick_tl :public widget_base
{
	glm::ivec2 color = { -1, -1 };	//当前颜色，旧颜色
	glm::vec4 hsv = {}, oldhsv = {};	// 0-1保存hsv
	uint32_t text_color = 0xffeeeeee;//文本颜色 
	uint32_t bc_color = 0xff232323;//边框 
	int width = 0;					// 宽度
	int height = 0;					// 单行高度 
	int step = 4;					// 行间隔 
	int cw = 0;						// 颜色宽
	int cpx = 0;					// 颜色x坐标
	int dx = -1;
	std::string hsvstr, colorstr;
	std::vector<font_item_t> tem_rtv;
	std::function<void(colorpick_tl* p, uint32_t col)> on_change_cb;
	bool alpha = true;				// 显示透明通道
public:
	colorpick_tl();
	~colorpick_tl();
	void init(uint32_t c, int w, int h, bool alpha);
	uint32_t get_color();	// 获取颜色
	void set_color2hsv(uint32_t c);
	void set_hsv(const glm::vec3& c);
	void set_hsv(const glm::vec4& c);
	void set_posv(const glm::ivec2& poss);

	bool on_mevent(int type, const glm::vec2& mps);
	bool update(float delta);
	void draw(cairo_t* cr);
};

// 滚动条
struct scroll_bar :public widget_base
{
	int64_t _view_size = 0;			// 视图大小
	int64_t _content_size = 0;		// 内容大小 
	int _rc_width = 0;			// 滑块宽度
	int _dir = 0;				// 方向，0=水平，1=垂直
	glm::ivec3 thumb_size_m = {};// 滚动范围
	glm::vec2 tps = {};
	glm::ivec4 _color = { 0xff363636,0xffcccccc,0xffffffff,0xffC8641E };		// 背景色，滑块颜色，滑块高亮颜色，激活颜色
	uint32_t _tcc = 0;			// 滑块当前颜色
	float _pos_width = 1;		// 滚动宽度
	int t_offset = 0;			// 偏移量
	float scale_w = 1.0;		// 滚动比例
	float scale_s = 0.6;		// 显示比例
	glm::vec2 scale_s0 = { 0.8,0.8 };	// 显示比例，用于鼠标进入变形
	bool hover = 0;				// 保存鼠标进入状态
	bool hover_sc = 0;
	bool hideble = 0;			// 隐藏滚动条
	bool limit = 1;				// 是否限制在滚动范围
	bool valid = 1;				// 是否重新渲染
	bool d_drag = 0;
private:
	int64_t _offset = 0;			// 偏移量
	int64_t c_offset = 0;			// 内容偏移量
public:
	scroll_bar();
	~scroll_bar();
	void set_viewsize(int64_t vs, int64_t cs, int rcw);
	bool on_mevent(int type, const glm::vec2& mps);
	bool update(float delta);
	void draw(cairo_t* cr);
	int64_t get_offset();			// 获取滚动偏移
	int64_t get_offset_ns();			// 获取滚动偏移
	int get_range();			// 获取滚动偏移最大范围
	void set_offset(int pts);			// 设置滚动偏移
	void set_offset_inc(int inc);			// 增加滚动偏移
	void set_posv(const glm::ivec2& poss);
};

struct text_draw_t
{
	cairo_t* cr; layout_text_x* ltx;
	std::string* text = 0;
	glm::vec4* text_rc;
	uint32_t* color;
	int count = 0;
	text_style_t* st;
	glm::vec4 box_rc;
};
struct rect_select_x;

class dtext_cache
{
public:
	cairo_t* cr = 0;
	cairo_surface_t* surface = 0;
	cairo_t* cr_in = 0;
	glm::ivec2 size = {};
public:
	dtext_cache();
	~dtext_cache();
	void reset(cairo_t* cr_in, const glm::ivec2& vsize);
	void free_surface();
	void save2png(const char* name);
	void clear_color(uint32_t color);
private:

};

struct hex_style_t {
	cairo_t* cr = 0;		// 0不渲染
	text_style_t* st = 0;
	layout_text_x* ltx = 0;
	int64_t fl = 0;		// 行高
	int64_t pxx = 0;
	int64_t pyy = 0;
	glm::vec2 hex_size = { 900,600 };
	glm::ivec2 view_size = {};
	glm::vec4 bgrc = {};
	// 定义常量 
	const int MARGIN = 10;
	const int DATA_INSPECTOR_TITLE_WIDTH = 80;
	const int DECODED_TEXT_WIDTH = 150;
	const int RULER_DI_WIDTH = 200;
	uint32_t bg_fill = 0xff121212, bg_color = 0x80ffffff; double bg_r = 2; int bg_linewidth = 1;
};
// 16进制编辑
struct hex_editor
{
public:
	std::string ruler, line_number;	// 行号，标尺
	std::string data_hex;			// 数据hex
	std::string decoded_text;		// 解码文本
	std::string ruler_di;			// 数据检查器头
	std::string dititle;			// 数据检查器标题
	std::string data_inspector;		// 数据检查
	int line_height = 20;
	int char_width = 10;
	int line_number_n = 0;
	int bytes_per_line = 16;		// 第行显示字节数4-256
	int select_border = 2;			// 选中边框宽度
	int64_t acount = 0;				// 行数量 
	glm::i64vec2 range = {};		// 选中范围
	glm::vec4 text_rc[7] = {};		// 渲染区域
	uint32_t color[7] = {};			// 渲染颜色
	uint32_t select_color = 0xfff77d5a;
	float round_path = 0.2;			// 圆角选区
	dtext_cache dtc = {};
private:
	unsigned char* _data = 0;
	size_t _size = 0;
	int64_t count = 0;				// 当前显示行数量
	size_t line_offset = 0;			// 当前行的偏移
	glm::ivec2 view_size = { 600,1080 }; // 视图高 
	text_draw_t tdt = {};
private:
	glm::i64vec2 range2 = {};		// 选中范围,从小到大
	glm::i64vec2 range_c1 = {};		// 选中范围c
	int64_t ctpos = -1;
	rect_select_x* rsx = 0;
	std::string bpline;				// 缓存用
	std::vector<char> tempstr;		// 缓存用
	void* mapfile = 0;				// 映射文件
	bool is_update = true;
	bool _rdonly = false;
	bool d_update = false;
public:
	hex_editor();
	~hex_editor();
	// 设置行高、字宽
	void set_linechar(int height, int charwidth);
	// 设置范围计算选中矩形
	void set_range(int64_t r, int64_t r1);
	void set_view_size(const glm::ivec2& s);
	// 设置数据只显示
	bool set_data(const char* d, size_t len, bool is_copy);
	// 设置文件，是否只读打开
	bool set_file(const char* fn, bool is_rdonly);
	void save_data(size_t pos, size_t len);
	size_t write_data(const void* d, size_t len, size_t pos, bool save);	// 写入数据，是否保存到文件
	// 设置当前光标，计算data_inspector
	void set_pos(size_t pos);
	// 设置显示偏移
	void set_linepos(size_t pos);
	// 鼠标事件，0鼠标移动
	void on_mouse(int clicks, const glm::ivec2& mpos, int64_t hpos, int64_t vpos);
	std::string get_ruler_di();
	char* data();
	size_t size();
	bool update_hex_editor();
	text_draw_t* get_drawt();
	glm::i64vec2 get_range2();
	// 获取渲染大小
	glm::ivec2 get_draw_rect();
	bool get_draw_update();
	void draw_rc(cairo_t* cr);
	// 更新、使用cairo渲染
	void update_draw(hex_style_t* hst);
private:
	// 鼠标坐标转偏移
	int64_t get_mpos2offset(const glm::i64vec2& mpos);
	void make_rc();
};
void draw_draw_texts(text_draw_t* p);
/*
todo Data Inspector
binary(1)读一个字节
octal(1)
uint8(1)
int8(1)
uint16(2)
int16(2)
uint24(3)
int24(3)
uint32(4)
int32
uint64
int64
ULEB128
SLEB128
float16
bfloat16
float32
float64
ASCII
UTF-8读整个字符
UTF-16读整个字符
GB18030
BIG5
SHIFT-JIS
*/


#endif // 1

class form_x;
class tview_x;
//struct text_item_t
//{
//	std::string familys, text;
//	int fontsize;
//	uint32_t color;
//	text_layout_t layout;
//	bool bd_valid = true;
//};
// 容器用
struct layout_info_x {
	glm::vec2 pos_align = { 0.5,0.5 };	// 默认的子元素偏移对齐。左中右上下布局计算auto ps = size * align - (ext * align + bearing);
	flex_item::flex_align justify_content = flex_item::flex_align::ALIGN_START;
	flex_item::flex_align align_content = flex_item::flex_align::ALIGN_START;
	flex_item::flex_align align_items = flex_item::flex_align::ALIGN_START;
	flex_item::flex_direction direction = flex_item::flex_direction::ROW;		// 行/列
	flex_item::flex_wrap wrap = flex_item::flex_wrap::WRAP;						// 是否换行
};
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
// 阴影管理
class gshadow_cx
{
public:
	bitmap_cache_cx bcc = {};				// 纹理缓存
	atlas_cx atc = {};
	std::vector<uint32_t> timg;
	image_ptr_t* img = 0;
	bool autofree = 0;
public:
	gshadow_cx();
	~gshadow_cx();
	image_sliced_t new_rect(const rect_shadow_t& rs);
private:

};
struct drag_v6
{
	glm::ivec2 pos;
	glm::ivec2 size;
	glm::ivec2 tp, cp0, cp1;
	int ck = 0;
	int z = 0;
};
struct scroll2_t
{
	scroll_bar* h = 0,		// 水平
		* v = 0;			// 垂直
};
struct plane_ev
{
	plane_cx* p;
	int down;		// 是否按下
	int clicks;		// 单击次数
	glm::ivec2 mpos;// 鼠标坐标
	bool drag;		// 是否拖动
};
// 面板，继承图集
class plane_cx :public canvas_atlas
{
public:
	tview_x* tv = {};			// 视图管理
	atlas_t* _pat = 0;			// 渲染面板背景
	form_x* form = 0;			// 绑定的窗口 
	layout_text_x* ltx = 0;		// 文本渲染管理
	std::function<void(plane_ev* e)> on_click;
	std::function<void(plane_ev* e)> on_click_outer;//模态窗口点中外围时
	std::function<void(cairo_t* cr, const glm::vec2& scroll)> draw_back_cb, draw_front_cb;
	std::function<bool(float delta)> update_cb;
	std::vector<widget_base*> widgets, event_wts, event_wts1;
	widget_base me = {};// 本体事件处理
	glm::dvec4 _hover_eq = { 0,0.5,0,0 };	// 时间
	glm::ivec2 _move_pos = {};
	// 移到最后
	void (*form_move2end)(form_x* f, plane_cx* ud) = 0;
	// 设置接收输入的控件
	void (*form_set_input_ptr)(form_x* f, void* ud) = 0;
	// OLO拖放文本
	bool (*dragdrop_begin)(const wchar_t* str, size_t size) = 0;

	scroll_bar* horizontal = 0, * vertical = 0;//水平滚动条 ，垂直滚动条 
	std::vector<drag_v6> drags;	// 拖动坐标
	std::vector<drag_v6*> dragsp;	// 拖动区域
	layout_info_x _css = {};		// 布局样式
	glm::vec2 _lpos = { 0,0 }, _lms = { 2,2 };// 布局偏移，子元素加宽
	std::string familys = "NSimSun";
	int fontsize = 16;
	uint32_t text_color = -1;
	glm::ivec4 border = {};	// 颜色，线粗，圆角，背景色
	glm::ivec2 curpos = {}, tpos = {};// 鼠标状态
	int ckinc = 0;
	int ckup = 0;
	int _bst = {};
	int evupdate = 0;
	int dms = 16, dmsset = 16;	// 渲染间隔ms
	vg_style_t vgs = {};
	glm::vec4 vgtms = { 0.2, 0.0, 0.0, 0.0f };// 间隔时间，最大值，当前时间, 选择框用 
	bool _draw_sbox = false;
	bool _draw_valid = true;
	bool _hover = false;
	bool draggable = false;
	bool uplayout = true;
	bool custom_layout = false;	// 使用自定义布局计算 
	bool autofree = true;
public:
	plane_cx();
	~plane_cx();
	void set_fontctx(font_rctx* p);
	glm::ivec2 get_pos();
	glm::ivec2 get_spos();
	void set_pos(const glm::ivec2& ps);
	void set_size(const glm::ivec2& ss);
	glm::vec2 get_size();
	void set_clear_color(uint32_t c);
	// 线颜色，线粗，圆角，背景色
	void set_color(const glm::ivec4& c);
	// 设置选择框范围，时间
	void set_select_box(int width, float s);
	size_t add_res(const std::string& fn);
	size_t add_res(const char* data, int len);
	// 设置本面板滚动条，pos_width每次滚动量,垂直vnpos,水平hnpos为滚动条容器内偏移
	void set_scroll(int width, int rcw, const glm::ivec2& pos_width, const glm::ivec2& vnpos = {}, const glm::ivec2& hnpos = {});
	void set_scroll_hide(bool is);// 是否隐藏滚动条
	void set_scroll_pos(const glm::ivec2& ps, bool v);
	void set_scroll_size(const glm::ivec2& ps, bool v);
	void set_view(const glm::ivec2& view_size, const glm::ivec2& content_size);
	void set_scroll_visible(const glm::ivec2& hv);
	glm::ivec2 get_scroll_range();
	// 设置位置，t=0设置，1加减
	void set_scroll_pts(const glm::ivec2& pts, int t);
	void move2end(widget_base* p);
	void set_family_size(const std::string& fam, int fs, uint32_t color);
	// 添加字体,返回序号
	size_t add_familys(const char* familys, const char* style);
	// 添加滚动条
	scroll_bar* add_scroll_bar(const glm::ivec2& size, int vs, int cs, int rcw, bool v, const glm::ivec2& npos = {});
	scroll2_t add_scroll2(const glm::ivec2& viewsize, int width, int rcw, const glm::ivec2& pos_width, const glm::ivec2& vnpos, const glm::ivec2& hnpos);
	void add_widget(widget_base* p);
	void remove_widget(widget_base* p);
	// 新增控件：开关、复选、单选
	switch_tl* add_switch(const glm::ivec2& size, const std::string& label, bool v, bool inlinetxt = false);
	checkbox_tl* add_checkbox(const glm::ivec2& size, const std::string& label, bool v);
	radio_tl* add_radio(const glm::ivec2& size, const std::string& label, bool v, group_radio_t* gp);
	edit_tl* add_input(const std::string& label, const glm::ivec2& size, bool single_line);
	gradient_btn* add_gbutton(const std::string& label, const glm::ivec2& size, uint32_t bcolor);
	color_btn* add_cbutton(const std::string& label, const glm::ivec2& size, int idx);
	color_btn* add_label(const std::string& label, const glm::ivec2& size, int idx);
	progress_tl* add_progress(const std::string& format, const glm::ivec2& size, double v);
	slider_tl* add_slider(const glm::ivec2& size, int h, double v);
	colorpick_tl* add_colorpick(uint32_t c, int width, int h, bool alpha);
	void set_update();
	// 窗口执行事件
	virtual void on_event(uint32_t type, et_un_t* e);
	virtual void update(float delta);
	// 更新布局
	void mk_layout();
	// 返回是否命中ui
	bool hittest(const glm::ivec2& pos);
	size_t push_dragpos(const glm::ivec2& pos, const glm::ivec2& size = {});
	glm::ivec3 get_dragpos(size_t idx);
	drag_v6* get_dragv6(size_t idx);
	void sortdg();
public:
	gshadow_cx* get_gs();
	// 设置边框阴影
	void set_shadow(const rect_shadow_t& rs);
	// 圆角裁剪
	void set_rss(int r);
private:
	void bind_scroll_bar(scroll_bar* p, bool v);	// 绑定到面板
	void on_motion(const glm::vec2& pos);
	// 	idx=1左，3右，2中
	void on_button(int idx, int down, const glm::vec2& pos, int clicks, int r);
	void on_wheel(double x, double y);
};



struct column_lv
{
	std::string title;	// 文本 
	glm::vec2 align = { 0.5,0.5 };	// 0左，0.5中，1右
	int type = 0;		// 类型
	int width = 0;		// 宽
	int idx = 0;		// 初始序号
	bool visible = true;//是否显示列
};
/*
switch_tl* add_switch(const glm::ivec2& size, const std::string& label, bool v, bool inlinetxt = false);
checkbox_tl* add_checkbox(const glm::ivec2& size, const std::string& label, bool v);
radio_tl* add_radio(const glm::ivec2& size, const std::string& label, bool v, group_radio_t* gp);
edit_tl* add_input(const std::string& label, const glm::ivec2& size, bool single_line);
gradient_btn* add_gbutton(const std::string& label, const glm::ivec2& size, uint32_t bcolor);
color_btn* add_cbutton(const std::string& label, const glm::ivec2& size, int idx);
color_btn* add_label(const std::string& label, const glm::ivec2& size, int idx);
progress_tl* add_progress(const std::string& format, const glm::ivec2& size, double v);
colorpick_tl* add_colorpick(uint32_t c, int width, int h, bool alpha);
*/


// 列表视图控件。显示文本、image_ptr_t、svg_cx
class listview_cx :public plane_cx
{
public:
	std::vector<column_lv> _title;	// 标题信息
	// 表格要显示的字符串 
	std::list<std::vector<std::string>> _data;

	grid_view* gv = 0;
	std::vector<font_item_t> tem_rtv;
	glm::ivec2 rc = {};	// 默认行列的宽高
	bool data_valid = false;
public:
	listview_cx();
	~listview_cx();

public:
	void add_title(const column_lv& p);
	// 设置视图大小
	void set_view_size(const glm::ivec2& sv, const glm::ivec2& crsize);

	size_t get_count();
	size_t get_column_count();
public:
	virtual void on_event(uint32_t type, et_un_t* e);
	virtual void update(float delta);
private:

};
struct tree_node_t
{
	std::string title;					// 显示的标题文本
	void* raw = 0;						// 自定义数据，比如用于显示图标啥的
	tree_node_t* parent = 0;			// 父级
	std::vector<tree_node_t*>* child = 0;// 孩子 
	int level = 0;						// 层级，自动计算 
	bool _expand = false;				// 是否展开 
	bool _autofree = false;				// 是否内部释放 
};

/*
添加节点、删除节点
*/
class tree_view_cx
{
public:
	tree_node_t _root = {};				// 根节点
	std::vector<tree_node_t*> showlist;	// 显示列表
	std::vector<font_item_t> tem_rtv;	// 文字渲染缓存
	glm::ivec2 scroll_pos, scroll_pos_old;// 滚动坐标
	std::function<void(tree_node_t* p)> on_click;	// 点中节点
	std::function<void(tree_node_t* p)> on_hover;	// 鼠标在节点上
	plane_cx* plane = 0;
public:
	tree_view_cx();
	~tree_view_cx();
	void binding(plane_cx* p);
	tree_node_t* insert(tree_node_t* parent, const std::string& str, void* data);
	tree_node_t* insert(tree_node_t* c, tree_node_t* parent);
	void remove(tree_node_t* p);
	void set_scroll_pos(const glm::ivec2& scp);
public:
	// 鼠标事件
	void on_mevent(int type, const glm::vec2& mps);
	void update(float delta);
	// 渲染树 
	void draw(cairo_t* cr);
private:

};

class valueview_cx :public plane_cx
{
public:
	valueview_cx();
	~valueview_cx();

public:
	virtual void on_event(uint32_t type, et_un_t* e);
	virtual void update(float delta);
private:

};







flex_item* flexlayout(flex_item* r, std::vector<glm::vec4>& v, const glm::vec2& pos, const glm::vec2& gap);

#if 1
svg_cx* new_svg_file(const void* fn, size_t len, int dpi);
svg_cx* new_svg_data(const void* str, size_t len, int dpi);
void free_svg(svg_cx* svg);
void render_svg(cairo_t* cr, svg_cx* svg);
void render_svg(cairo_t* cr, svg_cx* svg, const glm::vec2& pos, const glm::vec2& scale, double angle, const char* id = 0);

// 加载svg或图片
cairo_surface_t* load_imagesvg(const std::string& fn, float scale);

glm::vec4 colorv4(uint32_t rgba);
glm::vec4 colorv4_bgr(uint32_t bgra);
void set_color(cairo_t* cr, uint32_t rgba);
void set_color_bgr(cairo_t* cr, uint32_t c);
void set_color_a(cairo_t* cr, uint32_t rgba, double a);
void draw_rectangle(cairo_t* cr, const glm::vec4& rc, double r);
void draw_round_rectangle(cairo_t* cr, double x, double y, double width, double height, double r);
void draw_round_rectangle(cairo_t* cr, double x, double y, double width, double height, const glm::vec4& r);
void draw_round_rectangle(cairo_t* cr, const glm::vec4& rc, const glm::vec4& r);
void draw_circle(cairo_t* cr, const glm::vec2& pos, float r);
void draw_ellipse(cairo_t* cr, const glm::vec2& pos, const glm::vec2& r);
// 三角形基于矩形内 
//	 dir = 0;		// 尖角方向，0上，1右，2下，3左
//	 spos = 50;		// 尖角点位置0-1，中间就是0.5
void draw_triangle(cairo_t* cr, const glm::vec2& pos, const glm::vec2& size, const glm::vec2& dirspos);
void fill_stroke(cairo_t* cr, vg_style_t* st);
void fill_stroke(cairo_t* cr, uint32_t fill, uint32_t color, int linewidth = 1, bool isbgr = 0);
void draw_polyline(cairo_t* cr, const glm::vec2& pos, const glm::vec2* points, int points_count, unsigned int col, bool closed, float thickness);
// 渲染索引多段线，索引-1则跳过
void draw_polylines(cairo_t* cr, const glm::vec2& pos, const glm::vec2* points, int points_count, int* idx, int idx_count, unsigned int col, float thickness);

void draw_rect(cairo_t* cr, const glm::vec4& rc, uint32_t fill, uint32_t color, double r, int linewidth);
//glm::ivec2 layout_text_x::get_text_rect(size_t idx, const void* str8, int len, int fontsize)
void draw_text(cairo_t* cr, layout_text_x* ltx, const void* str, int len, glm::vec4 text_rc, text_style_t* st, glm::ivec2* orc = 0);


enum class eg_e :uint32_t {
	enull,
	e_rect, e_text, e_circle, e_ellipse, e_triangle, e_polyline, e_polylines, e_image
};
struct rect_b {
	eg_e type = eg_e::e_rect;
	uint32_t fill, color; int thickness;
	glm::vec4 rc;	// 坐标，大小
	glm::ivec4 r;	// 圆角
};
struct text_b
{
	eg_e type = eg_e::e_text;
	const void* str;
	int len;
	glm::vec4 text_rc;
	text_style_t* st;
	layout_text_x* ltx;
};
struct circle_b {
	eg_e type = eg_e::e_circle;
	uint32_t fill, color; int thickness; float r;
	glm::vec2 pos;
};
struct ellipse_b {
	eg_e type = eg_e::e_ellipse;
	uint32_t fill, color; int thickness;
	glm::vec2 pos, r;
};
struct triangle_b {
	eg_e type = eg_e::e_triangle;
	uint32_t fill, color; int thickness;
	glm::vec2 pos, size, dirspos;
};
struct polyline_b
{
	eg_e type = eg_e::e_polyline;
	glm::vec2& pos; const glm::vec2* points; int points_count; unsigned int color, fill; bool closed; float thickness;
};
struct polylines_b
{
	eg_e type = eg_e::e_polylines;
	glm::vec2 pos;
	const glm::vec2* points; int points_count; int* idx; int idx_count; unsigned int color, fill; float thickness;
};
struct image_b {
	eg_e type = eg_e::e_image;
	void* image; glm::vec2 pos; glm::vec4 rc; uint32_t color = -1; glm::vec2 dsize = { -1,-1 };
	// 九宫格图片
	glm::vec4 sliced = {};
};
// 图形通用软渲染接口
void draw_ge(cairo_t* cr, void* p, int count);
// 批量渲染文本+矩形背景
void draw_rctext(cairo_t* cr, layout_text_x* ltx, text_tx* p, int count, text_style_tx* stv, int st_count, glm::ivec4* clip);

void clip_cr(cairo_t* cr, const glm::ivec4& clip);


cairo_surface_t* new_clip_rect(int r);
void clip_rect(cairo_t* cr, cairo_surface_t* r);
void paint_shadow(cairo_t* cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, bool rev, float r = 0);//color_to=shadow;shadow.w=0;
void paint_shadow(cairo_t* cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, const glm::vec4& color_to, bool rev, float r = 0);

// 采样颜色
glm::dvec4 mix_colors(glm::vec4 a, glm::vec4 b, float ratio);
/*
用 bezier curve（贝塞尔曲线） 来设置 color stop（颜色渐变规则），
这里使用下面的曲线形式，其中
X轴为 offset（偏移量，取值范围为 0~1，0 代表阴影绘制起点），
Y轴为 alpha（颜 色透明度，取值范围为0~1，0 代表完全透明），
*/
void draw_rectangle_gradient(cairo_t* cr, int width, int height, const rect_shadow_t& rs);

// 获取对齐坐标
glm::ivec4 get_text_align(cairo_t* cr, const char* str, const glm::vec2& pos, const glm::vec2& boxsize, const glm::vec2& text_align, const char* family, int font_size);
void draw_text(cairo_t* cr, const char* str, const glm::ivec4& et, uint32_t text_color);
// box渲染对齐文本
glm::ivec4 draw_text_align(cairo_t* cr, const char* str, const glm::vec2& pos, const glm::vec2& boxsize, const glm::vec2& text_align, uint32_t text_color, const char* family, int font_size);
#ifndef key_def_data
#define key_def_data 1024
#define key_def_data_iptr 1025
#define key_def_data_svgptr 1026
#define key_def_data_done 1000
#endif
cairo_surface_t* new_image_cr(image_ptr_t* img);
cairo_surface_t* new_image_cr(const glm::ivec2& size, uint32_t* data = 0);
void update_image_cr(cairo_surface_t* image, image_ptr_t* img);
void free_image_cr(cairo_surface_t* image);
void image_set_ud(cairo_surface_t* p, uint64_t key, void* ud, void (*destroy_func)(void* data));
void* image_get_ud(cairo_surface_t* p, uint64_t key);
void image_save_png(cairo_surface_t* cr, const char* fn);
glm::ivec2 get_surface_size(cairo_surface_t* p);
// 渲染图片到cr
glm::vec2 draw_image(cairo_t* cr, cairo_surface_t* image, const glm::vec2& pos, const glm::vec4& rc, uint32_t color = -1, const glm::vec2& dsize = { -1,-1 });
// 渲染九宫格图片
glm::vec2 draw_image(cairo_t* cr, cairo_surface_t* image, const glm::vec2& pos, const glm::vec4& rc, uint32_t color, const glm::vec2& size, const glm::vec4& sliced);
#endif

int get_rand(int f, int s);
int64_t get_rand64(int64_t f, int64_t s);



/*
<表格、树形>
数据结构设计:
	网格线：		虚线/实线、颜色
	背景：		纯色、图片
	内容store：	数值、文本、简单公式、图片/SVG

*/
//vg_style_t
using cell_line_style = vg_style_t;
// 文本、填充颜色
struct cell_color_style {
	uint32_t text_color = 0xff282828;
	uint32_t color = 0xffffffff;
};
// 单元格内容
struct cell_store
{
	int type = 0;		// 0文本，1整数，2无符号整数，3浮点数，4公式
	std::string text;	// 显示文本
	union {
		int64_t iv;
		uint64_t uv;
		double fv;
		const char* expression;
	}v = {};
};
// 格式
struct format_style {
	const char* format = 0;		// 数字格式化
	const char* familys = 0;	// 字体名，逗号分隔
	int fontsize = 12;			// 字体大小
};
// 对齐格式
struct align_style {
	glm::vec3 a;	// x水平，y垂直
	//bool autobr = false;// z自动换行
};
//图片/svg
struct cell_image_store
{
	std::string uri;			// uri、base64（图片数据或svg字符串）
	glm::ivec2 size;			// 指定图片显示大小，0则填充单元格大小，-1则原始大小，其它则缩放数值
	glm::i64vec2 pos = {};		// 单元格坐标
};


// 线样式
struct cell_line_pos
{
	int style = 0;//线风格
	glm::ivec2 first = {};	// 开始格坐标
	glm::ivec2 second = {};	// 结束格坐标
};
struct cell_line_pos64
{
	int style = 0;
	glm::i64vec2 first = {};
	glm::i64vec2 second = {};
};
// 单元格 文本、填充颜色
struct cell_fill_style {
	int style = 0;
	glm::ivec2 first = {};	// 开始格坐标
	glm::ivec2 second = {};	// 结束格坐标，-1全部
};
struct cell_fill_style64 {
	int style = 0;
	glm::i64vec2 first = {};
	glm::i64vec2 second = {};
};
// 单元格内容坐标
struct cell_store_pos
{
	cell_store* value = 0;
	format_style* format = 0;
	align_style* align = 0;
	glm::ivec2 pos = {};			// 单元格坐标 
};
struct cell_store_pos64
{
	cell_store* value = 0;
	glm::i64vec2 pos = {};			// 64位坐标
};






// 表格填充支持的控件类型
enum class widget_type :uint32_t {
	wt_null,
	wt_text,
	wt_image,
	wt_edit,
	wt_color_btn,
	wt_gradient_btn,
	wt_radio_btn,
	wt_checkbox_btn,
	wt_switch_btn,
	wt_progress,
	wt_slider
};
// 列表渲染
class rlistview_cx
{
public:
	glm::ivec2 pos = {}, size = { -1,-1 };	// 坐标、视图大小

	grid_view gv;							// 网格
	std::vector<cairo_surface_t*> imagelist;// 图标列表
	void* _data = 0;						// 渲染数据 
	layout_text_x* ltx = 0;					// 文本渲染管理
public:
	rlistview_cx();
	~rlistview_cx();
	void set_data(void* d);
	// 图片/svg
	size_t add_image(const std::string& fn);
	// 渲染输入滚动条坐标
	void draw(cairo_t* cr, const glm::ivec2& scroll_pos);
	// 清空图片
	void clear_image();
private:

};


// 列表渲染
class render_lv
{
public:
	std::string familys;
	int fontsize = 12;
	float thickness = 1.0;	// 线粗
	uint32_t text_color = -1;//z文本颜色
	glm::ivec3 back_color = { 0,0,0x50ff8000 };//xy背景色（偶奇数），z鼠标经过增加颜色背景，
	glm::ivec4 title_color = {};// 背景色，鼠标经过时颜色,按下颜色。边框颜色
	glm::ivec2 line_color = { 0,0x80ff808080 };//x横，y竖
	plane_cx* plane = 0;
public:
	render_lv();
	~render_lv();

	void set_style(const std::string& family, int fs, uint32_t tcolor);
	// 绑定到面板
	void binding(plane_cx* p);
private:

};



// 创建控件
struct checkbox_com
{
	checkbox_tl* c = 0;
	color_btn* b = 0;
};
struct radio_com
{
	radio_tl* c = 0;
	color_btn* b = 0;
};
std::vector<color_btn*> new_label(plane_cx* p, const std::vector<std::string>& t, int width, std::function<void(void* p, int clicks)> cb);
std::vector<checkbox_com> new_checkbox(plane_cx* p, const std::vector<std::string>& t, int width, std::function<void(void* ptr, bool v)> cb);
std::vector<radio_com> new_radio(plane_cx* p, const std::vector<std::string>& t, int width, std::function<void(void* ptr, bool v)> cb);


std::string icu_u16_gbk(const void* str, size_t size);
std::string icu_gbk_u8(const char* str, size_t size);
std::string icu_u8_gbk(const char* str, size_t size);
std::string icu_u16_u8(const void* str, size_t size);
std::u16string icu_u8_u16(const char* str, size_t size);

void do_text(const char* str, size_t first, size_t count);


