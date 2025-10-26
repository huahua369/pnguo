#ifndef GUI_H
#define GUI_H


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
class text_dta
{
public:
	std::vector<font_item_t> tv;
	glm::vec4 rc = {};
	glm::vec2 text_align = {};
	size_t idx = 0;
	int fontsize = 16;
	layout_text_x* ltx = 0;
public:
	text_dta();
	~text_dta();

private:

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
	std::vector<d2_surface_t*> msu;
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

	bitmap_cache_cx* bc_ctx = 0;  //纹理缓存
	std::vector<font_t*> _t1;
	std::vector<uint32_t> s32;
public:
	layout_text_x();
	~layout_text_x();
	void set_ctx(font_rctx* p);
	// 添加字体,返回序号
	size_t add_familys(const char* familys, const char* style);
	void cpy_familys(layout_text_x* p);
	void clear_family();
	void clear_text();
	// 创建新缓存
	bitmap_cache_cx* new_cache(const glm::ivec2& vsize);
	void free_cache(bitmap_cache_cx* p);
	text_dta* new_text_dta(size_t idx, int fontsize, const void* str8, int len, text_dta* old = 0);
	text_dta* new_text_dta1(font_t* p, int fontsize, const void* str8, int len, text_dta* old = 0);
	// 获取基线
	int get_baseline(size_t idx, int fontsize);
	// 获取行高
	int get_lineheight(size_t idx, int fontsize);
	// 获取文本区域大小,z为基线
	glm::ivec3 get_text_rect(size_t idx, int fontsize, const void* str8, int len);
	glm::ivec3 get_text_rect1(size_t idx, int fontsize, const void* str8);
	int get_text_pos(size_t idx, int fontsize, const void* str8, int len, int xpos);
	int get_text_ipos(size_t idx, int fontsize, const void* str8, int len, int ipos);
	int get_text_posv(size_t idx, int fontsize, const void* str8, int len, std::vector<std::vector<int>>& ow);
	// 添加文本到渲染
	glm::ivec2 add_text(size_t idx, int fontsize, glm::vec4& rc, const glm::vec2& text_align, const void* str8, int len);
	glm::ivec2 build_text(size_t idx, int fontsize, glm::vec4& rc, const glm::vec2& text_align, const void* str8, int len, std::vector<font_item_t>& rtv);
	glm::ivec2 build_text1(font_t* p, int fontsize, glm::vec4& rc, const glm::vec2& text_align, const void* str8, int len, std::vector<font_item_t>& rtv);
	// 输出到图集
	void text2atlas(const glm::ivec2& r, uint32_t color, std::vector<atlas_cx>* opt);
	// 获取路径数据
	text_path_t* get_shape(size_t idx, int fontsize, const void* str8, text_path_t* opt, float scale = 1);
	// 获取渲染数据
	text_image_t* get_glyph_item(size_t idx, int fontsize, const void* str8, text_image_t* opt);
	text_image_t* get_glyph_item1(font_t* p, int fontsize, const void* str8, text_image_t* opt);
	// 渲染部分文本
#if 0
	void draw_text(cairo_t* cr, const glm::ivec2& r, uint32_t color);
	void draw_text(cairo_t* cr, const std::vector<font_item_t>& r, uint32_t color);

	void draw_rect_rc(cairo_t* cr, const std::vector<font_item_t>& rtv, uint32_t color);
	// 渲染全部文本
	void draw_text(cairo_t* cr, uint32_t color);
#endif
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

// 文字路径转path_v对象
void text_path2path_v(text_path_t* t, path_v* opt);
// rgba图像数据
struct tiny_image_t
{
	int width = 0, height = 0;
	uint32_t* img = 0;
};
struct text_image_pt
{
	int width = 0, height = 0;
	uint32_t* img = 0;
	path_v* src = 0;
	path_v* dst = 0;
};
// 文字加blur
text_image_pt* text_blur(text_path_t* p, float blur, int n, uint32_t color, uint32_t blur_color);
void free_textimage(text_image_pt* p);
// 保存到png\jpg
void textimage_file(text_image_pt* p, const std::string& fn, int quality = 90);

tiny_image_t* new_tiny_image(int width, int height, uint32_t* data);
tiny_image_t* gray2rgba(image_gray* g, uint32_t c);
void gray2rgba_blend(tiny_image_t* dst, const glm::ivec2& pos, image_gray* g, const glm::ivec4& rect, uint32_t c);
void free_tiny_image(tiny_image_t* p);

#if 1
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
	//virtual void draw(cairo_t* cr);
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
	// void draw(cairo_t* cr);

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
	// void draw(cairo_t* cr);
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
	// void draw(cairo_t* cr);
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
	// void draw(cairo_t* cr);
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
	// void draw(cairo_t* cr);
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
	// void draw(cairo_t* cr);
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
	// void draw(cairo_t* cr);
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
	// void draw(cairo_t* cr);
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
	// void draw(cairo_t* cr);
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
	// void draw(cairo_t* cr);
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
	// void draw(cairo_t* cr);
	int64_t get_offset();			// 获取滚动偏移
	int64_t get_offset_ns();			// 获取滚动偏移
	int get_range();			// 获取滚动偏移最大范围
	void set_offset(int pts);			// 设置滚动偏移
	void set_offset_inc(int inc);			// 增加滚动偏移
	void set_posv(const glm::ivec2& poss);
};


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
	std::function<void(void* cr, const glm::vec2& scroll)> draw_back_cb, draw_front_cb;
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
	std::vector<std::function<void(plane_ev* e)>> on_mouses;

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
	// 添加单击事件处理
	void add_mouse_cb(std::function<void(plane_ev* e)> cb);
public:
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



#endif
// 定义实现
#ifdef GUI_STATIC_LIB
#include "gui.cpp"
#endif
#endif // !GUI_H
