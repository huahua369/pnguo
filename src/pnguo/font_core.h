/*
字体文本渲染管理实现
*/
#ifndef FONT_CORE_H
#define FONT_CORE_H


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


struct image_ptr_t;
struct hps_t;
struct font_impl;
class fd_info0;
union ft_key_s;
struct gcolors_t;
struct bitmap_ttinfo;
struct Bitmap_p;
class stb_packer;
class bitmap_cache_cx;

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

class plane_cx;

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


#endif // !FONT_CORE_H
