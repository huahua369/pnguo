#pragma once
#include <list>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <functional>

typedef struct _PangoContext PangoContext;
struct input_state_t;

std::string to_string(double _Val);

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
	void* texid = 0;			// 纹理指针
	void* ptr = 0;				// 用户数据
	int comp = 4;				// 通道数
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
	glm::vec4 color = { 1.0,1.0,1.0,1.0 };	// 顶点颜色
	glm::vec2 tex_coord = {};		// 纹理uv
public:
	vertex_v2();
	vertex_v2(glm::vec3 p, glm::vec2 u, uint32_t  c);
	vertex_v2(glm::vec2 p, glm::vec2 u, uint32_t  c);
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
public:
	atlas_cx();
	~atlas_cx();
	void add(image_rc_t* d, size_t count);
	void add(image_sliced_t* d, size_t count);
	void add(const glm::ivec4& rc, const glm::ivec4& texrc, const glm::ivec4& sliced, uint32_t color = -1);
	void clear();
private:

};

// 图集画布
class canvas_atlas
{
public:
	struct draw_cmd_c
	{
		glm::ivec4 clip_rect = {};
		void* texid = 0;
		uint32_t vtxOffset = 0;
		uint32_t idxOffset = 0;
		uint32_t elemCount = 0;
	};
	struct image_rs
	{
		image_ptr_t* img = 0;
		glm::ivec2 size = { -1,-1 };		// *大小	
		glm::ivec4 rect = { 0,0,-1,-1 };	// *区域/坐标大小
		glm::ivec4 sliced = {};				// 九宫格渲染 left top right bottom 
	};
	std::vector<draw_cmd_c> cmd_data;
	std::vector<vertex_v2> vtxs;
	std::vector<int> idxs;

	std::vector<atlas_cx*> _atlas_cx;	// 显示的图集
	std::vector<atlas_t*> _atlas_t;		// 显示的图集
	std::vector<void* > _texs_t;		// 图集的纹理
	glm::ivec4 viewport = { 0,0,0,0 };  // maxint
	glm::ivec4 _clip_rect = { 0,0,200000,200000 };// 当前裁剪 
	void* _renderer = 0;	// 当前渲染器
	void(*destroy_texture_cb)(void* tex) = 0;//删除纹理回调 
	bool valid = false;
	bool visible = true;
public:
	canvas_atlas();
	~canvas_atlas();
	void add_atlas(atlas_cx* p);		// 添加显示的图集
	void remove_atlas(atlas_cx* p);	// 删除显示的图集
	void add_atlas(atlas_t* p);		// 添加显示的图集
	void remove_atlas(atlas_t* p);	// 删除显示的图集
	// 加载图片
	image_ptr_t* new_image2(const void* file);
	image_ptr_t* new_image2mem(const void* d, size_t s);
	void free_image(image_ptr_t* p);
	// 转换成bgra预乘
	void convert_bgr_multiply(image_ptr_t* p);
public:
	// 窗口调用 
	void apply();	// 创建顶点啥的
private:
	void add_image(image_rs* r, const glm::vec2& pos, uint32_t color);
	void clear();	//清除渲染数据
	bool nohas_clip(glm::ivec4 a);
	// 创建九宫格顶点
	void make_image_sliced(void* img, const glm::ivec4& a, glm::ivec2 texsize, const glm::ivec4& sliced, const glm::ivec4& rect, uint32_t col);
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
	int dim = 0;	// dimension 2/3/4
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
	std::string family, fullname;
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
	int last = 0;

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
//struct tinyimage_cx {
//	int width = 0, height = 0;
//	int isupdate = 1;
//	int format = 0;		// 0rgba, 1bgra
//	uint32_t* data = 0;  //rgba
//	void* ptr = 0;
//};
//struct tinyimagef_cx {
//	float width = 0, height = 0;// count=width*height*4
//	int isupdate = 1;
//	int format = 0;
//	float data[0];
//};
class svg_cx {
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

	image_ptr_t* push_cache_bitmap(Bitmap_p* bitmap, glm::ivec2* pos, int linegap = 0, uint32_t col = -1);
	image_ptr_t* push_cache_bitmap_old(Bitmap_p* bitmap, glm::ivec2* pos, uint32_t col, image_ptr_t* ret, int linegap = 0);
	// 清空所有缓存
	void clear();
private:
	stb_packer* get_last_packer(bool isnew);

};
//image_ptr_t to_imgptr(tinyimage_cx* p);

struct hps_t;
class fd_info0;

class font_t
{
public:
	font_impl* font = 0;
	std::string _name, fullname;
	std::string _style;
	// 支持的语言
	std::string _slng;
	int64_t num_glyphs = 0;
	uint32_t  _index = 0;

	float ascender = .0;
	float descender = .0;
	float lineh = .0;
	double  xMaxExtent = 0.0, lineGap = 0.0;
	std::map<uint64_t, font_item_t*> _cache_glyphidx;
	std::vector<font_item_t*> cache_data;
	int cache_count = 0;
	int ccount = 84;

	hps_t* hp = 0;

	bitmap_ttinfo* bitinfo = 0;
	gcolors_t* colorinfo = 0;
	bitmap_cache_cx* ctx = 0;  //纹理缓存

	// 宽度缓存表
	//std::unordered_map<uint64_t, glm::ivec3> _char_lut;

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
	tinypath_t get_shape(const void* str8, int height, std::vector<vertex_f>* opt);
	glm::ivec2 get_shape_box(const void* str8, int height);
	tinypath_t get_shape(int cp, int height, std::vector<vertex_f>* opt);
public:
	// 获取字符大小
	glm::ivec3 get_char_extent(char32_t ch, unsigned char font_size, unsigned short font_dpi, std::vector<font_t*>* fallbacks);
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
	font_item_t* push_gcache(uint32_t glyph_index, uint16_t height, image_ptr_t* img, const glm::ivec4& rect, const glm::ivec2& pos);
	font_item_t* get_gcache(uint32_t glyph_index, uint16_t height);
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

struct text_layout_t
{
	PangoLayout* layout = 0;
	font_rctx* ctx = 0;
	glm::ivec2 pos = {};
	glm::ivec2 rc = {};
	int lineheight = 0;
	int baseline = 0;
	int text_color = -1;
	bool once = false;
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
	std::map<std::string, font_t*> fzv;		// 自定义加载字体表
	font_imp* imp = 0;
	bitmap_cache_cx bcc = {};				// 纹理缓存
	font_t* current = 0;

	PangoContext* pcontext = 0;
	PangoLayout* layout = 0;
	std::set<PangoLayout*> gclt;
	std::string temfamily = "";
	std::string family = "NSimSun";
	int fontsize = 12;
public:
	font_rctx();
	~font_rctx();
	int get_count();						// 获取注册的字体数量
	int get_count_style(int idx);			// 获取idx字体风格数量
	const char* get_family(int idx);		// 获取字体名称
	const char* get_family_en(const char* family);		// 获取字体名称
	const char* get_family_full(int idx);	// 获取字体全名
	const char* get_family_style(int idx, int stidx);	// 获取字体风格名
	font_t* get_font(int idx, int styleidx);// 通过索引号获取字体对象
	font_t* get_font(const char* family, const char* style);//通过字符串名获取
	font_t* get_font_cur();					//获取当前字体对象

	void set_family_size(const std::string& fam, int fs);
	text_layout_t get_text_layout(const std::string& str, text_layout_t* lt);
	void draw_text(cairo_t* cr, text_layout_t* lt);
	void free_textlayout(text_layout_t* lt);
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
	std::vector<vertex_t> _data;
	// xy最小，zw最大
	glm::vec4 _box = { INT_MAX,INT_MAX,INT_MIN,INT_MIN };
	// 坐标
	glm::vec2 _pos = {};
	uint64_t ud = 0;
	int _baseline = 0;
	wchar_t text = 0;
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
	void add(const void* d, size_t size);
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
	double cross_v2(const glm::vec2& a, const glm::vec2& b);
	// 扩展多边形,会有自相交bug，但顶点数相同
	void expand_polygon(glm::vec2* polygon, int count, float expand, std::vector<glm::vec2>& ots);

	//type取{Square=0, Round=1, Miter=2}顶数会不同
	void expand_polygon_c(glm::vec2* polygon, int count, float expand, int type, std::vector<glm::vec2>& ots);
public:
	// 获取整体大小
	glm::vec2 get_size();
	glm::vec4 mkbox();
	void add(path_v* p);
	void incpos(const glm::vec2& p);
	void mxfy(double fy);
	// 曲线序号，段数，输出std::vector<glm::vec2/glm::dvec2>，type=0，1是double
	int get_flatten(size_t idx, size_t count, int m, void* flatten, int type);

	// 获取扩展路径,输出到opt，空则修改自身。顶点数相同，注意自相交
	int get_expand(float width, path_v* opt);

	// type取{Square=0, Round=1, Miter=2} ，不同width会使顶点数不同
	int get_expand_flatten(size_t idx, float width, int segments, int type, std::vector<std::vector<glm::vec2>>* ots, std::vector<int>* ccwv);
	int get_expand_flatten(float width, int segments, int type, std::vector<std::vector<glm::vec2>>* iod);

	int get_expand_flatten2(float expand, float scale, int segments, int type, std::vector<std::vector<glm::vec2>>* ots, bool is_round);

	bool is_ccw(int idx);
private:

};

struct text_path_t
{
	std::vector<tinypath_t> tv;
	std::vector<vertex_f> data;
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
class layout_text_x
{
public:
	font_rctx* ctx = 0;
	std::vector<std::vector<font_t*>> familyv;
	std::vector<glm::ivec3> cfb;
	int fdpi = 72;
	text_path_t ctp = {};	// 临时缓存
	text_image_t cti = {};
	std::vector<cairo_surface_t*> msu;
	std::vector<font_item_t> tv;
	std::vector<font_item_t> tem_rtv;	// 临时缓存用
	// todo
	std::vector<atlas_cx> tem_iptr;
	// 渲染缓存
	cairo_surface_t* ctemp = 0;
	glm::ivec2 ctrc = {}, oldrc = {};
public:
	layout_text_x();
	~layout_text_x();
	void set_ctx(font_rctx* p);
	// 添加字体,返回序号
	size_t add_familys(const char* familys, const char* style);
	void clear_family();
	void clear_text();

	// 获取基线
	int get_baseline(size_t idx, int fontsize);
	// 获取行高
	int get_lineheight(size_t idx, int fontsize);
	// 获取文本区域大小
	glm::ivec4 get_text_rect(size_t idx, const void* str8, int len, int fontsize);
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

	// 渲染全部文本
	void draw_text(cairo_t* cr, uint32_t color);
	// 获取图集
	atlas_t* get_atlas();
	void update_text();
private:
	void c_line_metrics(size_t idx, int fontsize);
};

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
	void on_button(int idx, int state, const glm::vec2& pos, int clicks);
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
	typedef void (*flex_self_sizing)(flex_item* item, float size[2]);
	float	width = NAN;
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

	flex_align justify_content = flex_align::ALIGN_START;	// 父元素:主轴上的元素的排列方式 start\end\center\space-between\space-around
	flex_align align_content = flex_align::ALIGN_STRETCH;	// 父元素:适用多行的flex容器 start\end\center\space-between\space-around\stretch 
	flex_align align_items = flex_align::ALIGN_STRETCH;		// 父元素:副轴上的元素的排列方式 start\end\center\stretch 
	flex_align align_self = flex_align::ALIGN_AUTO;			// 子元素:覆盖父容器align-items的设置

	flex_position position = flex_position::POS_RELATIVE;	// 子元素:
	flex_direction direction = flex_direction::ROW;			// 父元素:
	flex_wrap wrap = flex_wrap::WRAP;						// 父元素:是否换行
	bool should_order_children = false;

	float grow = 0;		// 子元素:自身放大比例，默认为0不放大
	float shrink = 0;	// 子元素:空间不足时自身缩小比例，默认为1自动缩小，0不缩小
	int	  order = 0;	// 子元素:自身排列顺序。数值越小，越靠前
	float basis = NAN;	// 子元素:定义最小空间

	void* managed_ptr = NULL;
	flex_self_sizing self_sizing = NULL;

	float frame[4] = {};	// 输出坐标、大小
	flex_item* parent = 0;
	std::vector<flex_item*>* children = 0;
public:
	flex_item();
	~flex_item();

	flex_item* init();
	void update_should_order_children();	// 子元素属性改变时执行

	void item_add(flex_item* child);
	void item_insert(uint32_t index, flex_item* child);
	flex_item* item_delete(uint32_t index);
	flex_item* detach(flex_item* child);
	// 执行布局计算
	void layout();
private:
	void layout_items(uint32_t child_begin, uint32_t child_end, uint32_t children_count, struct flex_layout* layout, uint32_t last_count);
	void layout_item(float width, float height);
};

#endif // NO_FLEX_CX

//  cb;支持的type有on_move/on_scroll/on_drag/on_down/on_up/on_click/on_dblclick/on_tripleclick
struct widget_base
{
	int id = 0;
	int bst = 1;			// 鼠标状态
	glm::vec2 pos = {};		// 控件坐标
	glm::vec2 size = {};	// 控件大小
	glm::ivec2 curpos = {};	// 当前拖动鼠标坐标
	glm::ivec2 cmpos = {};	// 当前鼠标坐标
	std::string text;
	std::string family;
	float font_size = 16;
	glm::vec2 text_align = { 0.5,.5 }; // 文本对齐
	int rounding = 4;		// 圆角
	float thickness = 1.0;	// 边框线粗
	std::function<void(void* p, int type, const glm::vec2& mps)> cb;	//通用事件处理
	std::function<void(void* p, int clicks)> click_cb;					//点击事件
	layout_text_x* ltx = 0;
	glm::ivec2 txtps = {};
	glm::ivec2 txtps2 = {};
	int _old_bst = 0;			// 鼠标状态
	int cks = 0;
	bool _disabled_events = false;
	bool visible = true;
	bool has_drag = false;	// 是否有拖动事件
	bool _autofree = false;
	virtual bool update(float delta);
	virtual void draw(cairo_t* cr);
};


// 输入布局样式等信息+文字
#ifndef NO_EDIT

class text_ctx_cx;

class edit_tl :public widget_base
{
public:
	text_ctx_cx* ctx = 0;							// 布局/渲染/事件处理
	std::string _text;								// 保存行文本
	std::function<void(edit_tl* ptr)> changed_cb;	// 文本改变时执行回调函数

	glm::ivec2 ppos = {};	// 父级坐标 
	bool single_line = false;
	bool mdown = false;
	bool _read_only = false;
	bool is_input = false;
public:
	edit_tl();
	~edit_tl();
	void set_single(bool is);
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
	void set_family(const char* familys, int fontsize);
	// 设置是否显示输入光标
	void set_show_input_cursor(bool ab);

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
	STATE_NOMAL = 1,
	STATE_HOVER = 2,
	STATE_ACTIVE = 4,
	STATE_FOCUS = 8,
	STATE_DISABLE = 0x10,
};

// todo图片按钮
struct image_btn :public widget_base {
	std::string str;

	atlas_t dsi = {};
	std::vector<glm::ivec4> data;
	int show_idx = 0;	// 参考BTN_STATE
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
	bool _circle = false;			// 圆形按钮
	bool _disabled = false;
	bool mPushed = false;
	bool hover = false;
	bool bgr = 0;
public:
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
	float swidth = 0.;
	float dt = 0;	// 动画进度
	bool value = 0; // 选中值
	bool value1 = 1; // 选中值动画
};
struct checkbox_info_t
{
	glm::vec2 pos;	// 坐标
	std::string text;
	float dt = 0;	// 动画进度
	float duration = 0.28;	// 动画时间
	float new_alpha = -1;		// 动画控制
	bool mixed = false;			// 是否满
	bool value = 0; // 选中值
	bool value1 = 1; // 选中值动画
};
struct radio_tl;
struct group_radio_t
{
	radio_tl* active = 0;
};
// 单选
struct radio_tl :public widget_base
{
	radio_style_t style = {};	// 风格id
	radio_info_t v;
	group_radio_t* gr = 0;		// 组 
public:
	void set_value(const std::string& str, bool v);
	void set_value(bool v);
	void set_value();
	bool update(float delta);
	void draw(cairo_t* cr);
};
// 复选
struct checkbox_tl :public widget_base
{
	check_style_t style = {};	// 风格id
	checkbox_info_t v;
public:
	void set_value(const std::string& str, bool v);
	void set_value(bool v);
	void set_value();
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
	void set_value(bool b);
	void set_value();
	bool update(float delta);
	void draw(cairo_t* cr);
};



#endif // 1

class form_x;
class tview_x;
struct text_item_t
{
	std::string familys, text;
	int fontsize;
	uint32_t color;
	text_layout_t layout;
	bool bd_valid = true;
};
// 容器用
struct layout_info_x {
	glm::vec2 pos_align = { 0.5,0.5 };	// 默认的子元素偏移对齐。左中右上下布局计算auto ps = size * align - (ext * align + bearing);
	flex_item::flex_align justify_content = flex_item::flex_align::ALIGN_START;
	flex_item::flex_align align_content = flex_item::flex_align::ALIGN_START;
	flex_item::flex_align align_items = flex_item::flex_align::ALIGN_START;
	flex_item::flex_direction direction = flex_item::flex_direction::ROW;		// 行/列
	flex_item::flex_wrap wrap = flex_item::flex_wrap::WRAP;						// 是否换行
};

// 面板，继承图集
class plane_cx :public canvas_atlas
{
public:
	tview_x* tv = {};			// 视图管理
	atlas_t* _pat = 0;			// 渲染面板用
	form_x* form = 0;			// 绑定的窗口 
	layout_text_x* ltx = 0;		// 文本渲染管理
	std::function<void(plane_cx* p, int state, int clicks)> on_click;
	std::function<void(cairo_t* cr)> draw_cb;
	std::function<bool(float delta)> update_cb;
	//std::vector<text_item_t> txtv;
	std::vector<widget_base*> widgets;
	std::vector<image_ptr_t*> images;
	std::vector<svg_cx*> svgs;
	layout_info_x _css = {};		// 布局样式
	glm::vec2 _lpos = { 10,10 }, _lms = { 2,2 };// 偏移，加宽
	std::string familys = "Arial,NSimSun";
	int fontsize = 12;
	uint32_t text_color = -1;
	glm::ivec3 border = {};	// 颜色，线粗，圆角
	glm::ivec2 curpos = {}, tpos = {};// 鼠标状态
	int ckinc = 0;
	int ckup = 0;
	int _bst = {};
	int evupdate = 0;
	int dms = 16, dmsset = 16;	// 渲染间隔ms
	bool _draw_valid = true;
	bool draggable = false;
public:
	plane_cx();
	~plane_cx();
	void set_fontctx(font_rctx* p);
	glm::ivec2 get_pos();
	void set_pos(const glm::ivec2& ps);
	void set_size(const glm::ivec2& ss);
	glm::vec2 get_size();
	void set_colors(const glm::ivec4& c);
	size_t add_res(const std::string& fn);
	size_t add_res(const char* data, int len);
	void move2end(widget_base* p);
	void set_family_size(const std::string& fam, int fs, uint32_t color);
	// 添加字体,返回序号
	size_t add_familys(const char* familys, const char* style);
	void add_widget(widget_base* p);
	// 新增控件：开关、复选、单选
	switch_tl* add_switch(const glm::ivec2& size, const std::string& label, bool v, bool inlinetxt = false);
	checkbox_tl* add_checkbox(const glm::ivec2& size, const std::string& label, bool v);
	radio_tl* add_radio(const glm::ivec2& size, const std::string& label, bool v, group_radio_t* gp);
	edit_tl* add_input(const std::string& label, const glm::ivec2& size, bool single_line);
	gradient_btn* add_gbutton(const std::string& label, const glm::ivec2& size, uint32_t bcolor);
	color_btn* add_cbutton(const std::string& label, const glm::ivec2& size, int idx);
	color_btn* add_label(const std::string& label, const glm::ivec2& size, int idx);
	// 窗口执行事件
	void on_event(uint32_t type, et_un_t* e);
	void update(float delta);
	// 更新布局
	void mk_layout();
	// 返回是否命中ui
	bool hittest(const glm::ivec2& pos);
private:
	void on_motion(const glm::vec2& pos);
	// 	idx=1左，3右，2中
	void on_button(int idx, int state, const glm::vec2& pos, int clicks, int r);
	void on_wheel(double x, double y);
};

flex_item* flexlayout(flex_item* r, std::vector<glm::vec4>& v, const glm::vec2& pos, const glm::vec2& ms);

svg_cx* new_svg_file(const void* fn, size_t len, int dpi);
svg_cx* new_svg_data(const void* str, size_t len, int dpi);
void free_svg(svg_cx* svg);
#if 1
void render_svg(cairo_t* cr, svg_cx* svg);
void render_svg(cairo_t* cr, svg_cx* svg, const glm::vec2& pos, const glm::vec2& scale, double angle);

void set_color(cairo_t* cr, uint32_t rgba);
void set_color_bgr(cairo_t* cr, uint32_t c);
void set_color_a(cairo_t* cr, uint32_t rgba, double a);
void draw_rectangle(cairo_t* cr, const glm::vec4& rc, double r);
void draw_round_rectangle(cairo_t* cr, double x, double y, double width, double height, double r);
void draw_round_rectangle(cairo_t* cr, double x, double y, double width, double height, const glm::vec4& r);
void draw_circle(cairo_t* cr, const glm::vec2& pos, float r);
void fill_stroke(cairo_t* cr, uint32_t fill, uint32_t color, int linewidth, bool isbgr);
void draw_polyline(cairo_t* cr, const glm::vec2& pos, const glm::vec2* points, int points_count, unsigned int col, bool closed, float thickness);

cairo_surface_t* new_clip_rect(int r);
void clip_rect(cairo_t* cr, cairo_surface_t* r);
void paint_shadow(cairo_t* cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, bool rev, float r = 0);//color_to=shadow;shadow.w=0;
void paint_shadow(cairo_t* cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, const glm::vec4& color_to, bool rev, float r = 0);
// 获取对齐坐标
glm::ivec4 get_text_align(cairo_t* cr, const char* str, const glm::vec2& pos, const glm::vec2& boxsize, const glm::vec2& text_align, const char* family, int font_size);
void draw_text(cairo_t* cr, const char* str, const glm::ivec4& et, uint32_t text_color);
// box渲染对齐文本
glm::ivec4 draw_text_align(cairo_t* cr, const char* str, const glm::vec2& pos, const glm::vec2& boxsize, const glm::vec2& text_align, uint32_t text_color, const char* family, int font_size);

cairo_surface_t* new_image_cr(image_ptr_t* img);
void update_image_cr(cairo_surface_t* image, image_ptr_t* img);
void free_image_cr(cairo_surface_t* image);
glm::ivec2 get_surface_size(cairo_surface_t* p);
glm::vec2 draw_image(cairo_t* cr, cairo_surface_t* image, const glm::vec2& pos, const glm::vec4& rc, uint32_t color = -1, cairo_surface_t* tp = 0);
#endif
/*
<表格、树形>
数据结构设计:
	网格线：		虚线/实线、颜色
	背景：		纯色、图片
	内容store：	数值、文本、简单公式、图片/SVG

*/

struct cell_line_style {
	uint32_t color = 0xffd4d4d4;
	float thickness = 1;
	float* dash = 0;		// 虚线逗号/空格分隔的数字
	int dashOffset = 0;
};
// 线样式
struct cell_line_pos
{
	cell_line_style* style = 0;//引用线风格
	glm::ivec2 first = {};	// 开始格坐标
	glm::ivec2 second = {};	// 结束格坐标
};
struct cell_line_pos64
{
	cell_line_style* style = 0;
	glm::i64vec2 first = {};
	glm::i64vec2 second = {};
};
// 单元格填充颜色
struct cell_fill_style {
	uint32_t color = 0xffffffff;
	glm::ivec2 first = {};	// 开始格坐标
	glm::ivec2 second = {};	// 结束格坐标，-1全部
};
struct cell_fill_style64 {
	uint32_t color = 0xffffffff;
	glm::i64vec2 first = {};
	glm::i64vec2 second = {};
};
// 单元格内容
struct cell_store
{
	int type = 0;		// 0文本，1整数，2浮点数，3公式
	std::string text;	// 显示文本
	union {
		uint64_t iv = 0;
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
	glm::ivec2 a;	// x左中右，y上中下。0/1/2
	bool autobr = false;// 自动换行
};
//图片/svg
struct cell_image_store
{
	std::string uri;			// uri、base64（图片数据或svg字符串）
	glm::ivec2 size;			// 指定图片显示大小，0则填充单元格大小，-1则原始大小，其它则缩放数值
	glm::i64vec2 pos = {};		// 单元格坐标
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


// 固定表格  

struct column_ht
{
	std::string title;	// 文本
	std::string format;	// 数字格式化
	int width = 0;		// 宽
	int align = 0;		// 0左，1中，2右
	int idx = 0;		// 初始序号
};
class listview_cx
{
public:
	std::vector<column_ht> titles;					// 标题信息
	std::vector<std::vector<std::string>> stores;	// 保存数据
	bool data_valid = true;	// 更新了数据
public:
	listview_cx();
	~listview_cx();
	void set_title(column_ht* p, int count);
	// 获取行数
	size_t get_count();
	// 列数
	size_t get_column_count();
	// 添加一行到后面
	void push_back(const std::vector<std::string>& t);
	// 插入一行
	void insert(size_t pos, const std::vector<std::string>& t);
	void update(size_t pos, const std::vector<std::string>& t);	// 更新整行
	void update(size_t pos, size_t idx, const std::string& str);	// 更新一格
	std::vector<std::string>* get_line(size_t pos);
	// 移动列位置，移动的列，移到pos。移动不影响上面输入数据的排序
	void move_column(size_t idx, size_t pos);
	// 排序的列，asc=0小到大，1大到小
	void sort_column(size_t idx, int asc);
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

namespace md {
	uint32_t get_u8_idx(const char* str, int64_t idx);
	const char* get_u8_last(const char* str, uint32_t* codepoint);
	std::string u16to_u8(uint16_t* str, size_t len);
	std::wstring u8to_w(const char* str, size_t len);
}
// 实现在tinysdl3.cpp
void form_move2end(form_x* f, plane_cx* ud);
// 设置接收输入的控件
void form_set_input_ptr(form_x* f, void* ud);
// OLO拖放文本
bool dragdrop_begin(const wchar_t* str, size_t size);
