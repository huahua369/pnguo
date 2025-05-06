/*
字体文本渲染管理实现
*/
#ifndef FONT_CORE_H
#define FONT_CORE_H


struct vertex_f
{
	float x, y, cx, cy, cx1, cy1;
	int type;
};
struct vertex_v2f
{
	glm::vec2 p, c, c1;
	int type;
};
struct vertex_d
{
	double x, y, cx, cy, cx1, cy1;
	int64_t type;
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


struct image_ptr_t;
struct hps_t;
struct font_impl;
struct font_item_t;
class font_imp;
class fd_info0;
union ft_key_s;
struct gcolors_t;
struct bitmap_ttinfo;
struct Bitmap_p;
class stb_packer;
class bitmap_cache_cx;

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
	bitmap_cache_cx* bc_ctx = 0;  //纹理缓存，多个字体共用 

	bool first_bitmap = false;
	// Whether kerning is desired
	bool enable_kerning = false;
	bool use_no_color = false;

	struct c_glyph {
		int stored;
		uint32_t index;
		//TTF_Image bitmap;
		//TTF_Image pixmap;
		int sz_left;
		int sz_top;
		int sz_width;
		int sz_rows;
		int advance;
		union {
			// TTF_HINTING_LIGHT_SUBPIXEL (only pixmap)
			struct {
				int lsb_minus_rsb;
				int translation;
			} subpixel;
			// Other hinting
			struct {
				int rsb_delta;
				int lsb_delta;
			} kerning_smart;
		};
	};
	struct GlyphPosition {
		font_t* font;
		uint32_t index;
		uint32_t codepoint;
		c_glyph* glyph;
		int x_offset;
		int y_offset;
		int x_advance;
		int y_advance;
		int x;
		int y;
		int offset;
	};
	struct GlyphPositions {
		GlyphPosition* pos;
		int len;
		int width26dot6;
		int height26dot6;
		int num_clusters;
		int maxlen;
	};
public:
	font_t();
	~font_t();
	float get_scale(int px);
	double get_base_line(double height);
	double get_line_height(double height);
	int get_xmax_extent(double height, int* line_gap);
	glm::ivec3 get_text_rect(int fontsize, const void* str8, int len);
	// 获取字体最大box
	glm::ivec4 get_bounding_box(double scale, bool is_align0);
	// 输入utf8获取轮廓，height=0则获取原始大小。提供opt
	tinypath_t get_shape(const void* str8, int height, std::vector<vertex_f>* opt, int adv, float scale1 = 1);
	glm::ivec2 get_shape_box(const void* str8, int height);
	glm::ivec2 get_shape_box(uint32_t ch, int height);
	tinypath_t get_shape(int cp, int height, std::vector<vertex_f>* opt, int adv, float scale1 = 1);
public:
	// 获取字符大小{xy大小，z=advance,w=基线}
	glm::ivec4 get_char_extent(char32_t ch, unsigned char font_size, /*unsigned short font_dpi,*/ std::vector<font_t*>* fallbacks, font_t** oft);
	void clear_char_lut();
	void clear_gcache();
	// todo 获取文字渲染信息。glyph_index=-1时则用unicode_codepoint；
	font_item_t get_glyph_item(uint32_t glyph_index, uint32_t unicode_codepoint, int fontsize, bitmap_cache_cx* use_ctx = nullptr);

	// 获取文字在哪个字体
	static const char* get_glyph_index_u8(const char* u8str, int* oidx, font_t** renderFont, std::vector<font_t*>* fallbacks);
	int get_glyph_index(uint32_t codepoint, font_t** renderFont, std::vector<font_t*>* fallbacks);

	std::map<int, std::vector<info_one>> get_detail();

	bool CollectGlyphsFromFont(const char* text, size_t length, int direction, uint32_t script, GlyphPositions* positions);
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


struct fontns
{
	std::string family, fullname, namecn;
	std::vector<std::string> style, fpath;
	std::set<std::string> alias;
	std::vector<void*> vptr;
};

// 获取系统字体列表
std::map<std::string, fontns> get_allfont();

class bitmap_cache_cx
{
public:
	std::vector<stb_packer*> _packer;
	std::vector<image_ptr_t*> _data;
	int width = 1024;					// 纹理宽高
	int height = 1024;
public:
	bitmap_cache_cx();
	~bitmap_cache_cx();
	void resize(int w, int h);
	glm::ivec2 fill_color(int w, int h, uint32_t color);
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
/*
	LINE_CAP_BUTT,0
	LINE_CAP_ROUND,1
	LINE_CAP_SQUARE2
	LINE_JOIN_MITER,0
	LINE_JOIN_ROUND,1
	LINE_JOIN_BEVEL2
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

struct text_path_t
{
	std::vector<tinypath_t> tv;	// 每个字的路径
	std::vector<vertex_f> data;	// 所有路径数据
	int baseline = 0;
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

// Histogram
class yHist
{
public:
	yHist()
	{
	}

	~yHist()
	{
	}
	void init(int c = 256);
	void Hist(unsigned char c);

	//比较两个直方图
	bool diff(yHist* p, int dmin = 1500, int dmax = 2000);
	double calculate_relation(yHist* p);
public:
	//创建直方图
	std::vector<int> hists;
	std::set<int> mset;
	int hmin = 0, hmax = 0;
};
// 灰度图
class image_gray
{
public:
	int width = 0, height = 0;
	std::vector<unsigned char> _data;
public:
	image_gray();
	~image_gray();

public:
	unsigned char* data();
	size_t size();
	void resize(size_t w, size_t h);
	void clear_color(unsigned char fill);
	void draw_rect(const glm::ivec4& rect, unsigned char col, unsigned char fill, glm::ivec4 rounding = {});
	void draw_circle_fill(const glm::vec3& c, unsigned char fill);
private:

};
struct vertex_32f
{
	// 24字节
	glm::vec2 p, c, c1;
	// 4字节
	uint32_t type;
};
#ifdef stbtt_vertex_type
unsigned char* get_glyph_bitmap_subpixel(stbtt_vertex* vertices, int num_verts, glm::vec2 scale, glm::vec2 shift, glm::vec2 xy_off, std::vector<unsigned char>* out, glm::ivec3 bpm_size, int invert);
#endif
/*
todo 路径填充光栅化-function
输入路径path，缩放比例scale、bpm_size{宽高xy,每行步长z}、invert 1倒置, 0正常，
xy_off偏移默认0
输出灰度图
支持移动、直线、2/3次贝塞尔曲线，构成的路径。
   vmove=1, vline=2, vcurve=3, vcubic=4
*/
// 路径填充光栅化
void get_path_bitmap(vertex_32f* vertices, size_t num_verts, image_gray* bmp, glm::vec2 scale, glm::vec2 xy_off, int invert);
// 模糊灰度图
void blur2gray(unsigned char* dst, int w, int h, int dstStride, float blur, int n, int mode = 0x01 | 0x02);

void px_blend2c(uint32_t* pDstBmp, uint32_t src, uint32_t col);

void save_img_png(image_ptr_t* p, const char* str);
// 灰度图转rgba
void gray_copy2rgba(image_ptr_t* dst, image_ptr_t* src, const glm::ivec2& dst_pos, const glm::ivec4& rc, uint32_t col, bool isblend);
//单色位图1位
void bit_copy2rgba(image_ptr_t* dst, image_ptr_t* src, const glm::ivec2& dst_pos, const glm::ivec4& rc, uint32_t color);

void rgba_copy2rgba(image_ptr_t* dst, image_ptr_t* src, const glm::ivec2& dst_pos, const glm::ivec4& rc, bool isblend);

void split_v(std::string str, const std::string& pattern, std::vector<std::string>& result);


#endif // !FONT_CORE_H
