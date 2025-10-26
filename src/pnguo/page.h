/*
	页面管理
	Copyright (c) 华仔
	188665600@qq.com
	创建时间2024-07-10

	用于创建控件之类页面元素。
*/
class form_x;
class plane_cx;
struct style_plane_t;
struct style_tooltip
{
	const char* family = 0;
	int fonst_size = 16;
	int text_color = -1;	// 文本颜色
	glm::ivec2 color = { 0xf0121212,0xaaffffff };	// 背景色，边框色
	float thickness = 1.0;	// 线宽
	float radius = 0;		// 矩形圆角 
};
class mitem_t;
class menu_cx;
class layout_text_x;
struct itd_t
{
	std::string str;
	mitem_t* child = 0;
};
// 菜单项
class mitem_t
{
public:
	std::vector<itd_t> v;
	std::function<void(mitem_t* p, int type, int id)> ckm_cb;
	int width = -1;		// 菜单宽度
	int height = 0;	// 菜单项高
	glm::ivec2 pos = {};
	menu_cx* m = 0;
	form_x* f = 0;
	mitem_t* parent = 0;	// 父级
	mitem_t* cct = 0;		// 当前子菜单
	pvm_t pv = {};
	canvas_atlas* backgs = 0, * fronts = 0;// 背景和前景
	layout_text_x* ltx = 0;
public:
	mitem_t();
	~mitem_t();
	void show(const glm::vec2& pos);
	void hide(bool hp);
	void close();
	void set_data(int width, int height, const std::vector<std::string>& mvs);
	void set_data(int width, int height, const char** mvs, size_t n);
	glm::ivec2 get_idx_pos(int idx);
	// 设置子菜单
	void set_child(mitem_t* cp, int idx);
	bool get_visible();
};
struct menu_info
{
	const char** mstr = 0;	// 菜单字符串
	size_t count = 0;		// 菜单项数量 
	mitem_t* ptr = 0;		// 返回的菜单指针
	int parent = -1;		// 父级id
	int parent_idx = -1;	// 父级菜单项索引
};
// 菜单组
struct mitem_g {
	mitem_t* ptr = 0;
	int count = 0;			// 菜单数量
	int cx = -1;				// 当前显示
};
// 菜单管理器
class menu_cx
{
public:
	form_x* form = 0;			// 主窗口 
	plane_cx* u = 0;
	std::vector<std::string> familys;
public:
	menu_cx();
	~menu_cx();
	void set_main(form_x* f);
	void add_familys(const char* family);
	// 创建菜单组
	mitem_g* new_menu_g(menu_info* p, int count, const glm::vec2& msize, std::function<void(mitem_t* p, int type, int id)> cb);
	void show_mg(mitem_g* p, int idx, const glm::vec2& pos);
	void free_menu_g(mitem_g* p);
	void show_item(mitem_t* it, const glm::vec2& pos);
private:
	mitem_t* new_menu(int width, int height, const std::vector<std::string>& mvs, std::function<void(mitem_t* p, int type, int id)> cb);
	void free_item(mitem_t* p);

};
// 显示工具提示面板 
void show_tooltip(form_x* form, const std::string& str, const glm::ivec2& pos, style_tooltip* bc);
void hide_tooltip(form_x* form);

struct style_plane_t {
	image_ptr_t* img = 0;	// 图标纹理
	glm::ivec2 color = {};	// 背景色，边框色
	int text_color = -1;	// 文本颜色
	float thickness = 1.0;	// 线宽
	float radius = 4;		// 矩形圆角
};


struct column_lvt
{
	std::string title;	// 文本 
	glm::vec2 align = { 0.5,0.5 };	// 0左，0.5中，1右
	int width = 0;		// 宽
	int idx = 0;		// 初始序号
	bool visible = true;//是否显示列
};
// 列表框
class list_box_cx
{
public:
	int font_size = 16;
public:
	list_box_cx();
	~list_box_cx();

private:

};


// 对话框
class dialog_cx
{
public:

public:
	dialog_cx();
	~dialog_cx();

private:

};



class div2_t
{
public:
	struct v6_t :public glm::vec4
	{
		float px, py;
	};
	std::vector<glm::vec4> rcs;
	std::vector<flex_item> rcs_st, c2;
	std::vector<std::vector<v6_t>> childs;
public:
	div2_t();
	~div2_t();
	void set_root(const std::vector<int>& r);
	void set_root_style(size_t idx, const flex_item& it);
	void add_child(size_t idx, const glm::vec2& ss);
	void layout();
	void draw(void* cr);
private:

};

struct action_show_t
{
	void* ptr = 0;
	float wait = 0.0f;
	bool v = false;
};

// 动画到目标
template<class T>
class vat_t
{
public:
	int t = 0;
	float mt = 1;
	// 开始、目标
	T first = {}, target = {};
	// ct当前时间，mt设置时间
	float ct = 0;
	// 返回值：0不执行，1执行结束，2执行中
	int updata_t(float deltaTime, T& dst)
	{
		if ((deltaTime > mt && mt > 0)) return 0;
		double kct = ct;
		ct += deltaTime;
		if (ct >= mt) {
			dst = target;
			return 1;
		}
		dst = glm::mix(first, target, ct / mt);
		return 2;
	}
};
/* 路径动画命令。支持1/2/3维路径运动
	等待(秒)e_wait=0,	vec2类型、等待时间
	路径=1，				vec2类型、数量
		跳到e_vmove = 1,
		直线到e_vline=2,
		2阶曲线到e_vcurve=3,
		3阶曲线到e_vcubic=4
*/
struct action_t
{
public:
	std::vector<glm::vec4> cmds;
	std::vector<glm::vec4> v_time;	// 时间
	void* ptr = 0;		// 用户指针
	int16_t _dim = 2;	// 1=float，2=vec2，3=vec3，4=vec4
	int16_t type = 0;	// 类型，0顺序，1同时执行
	int cidx = 0;		// 当前执行的命令序号
	float ctime = 0;	// 运行时间
	float speed = 1.0;	// 运行时间
	int _count = 0;		// 动作数量
	float* dst = 0;		// 结果 
private:
	bool _pause = false;
public:
	action_t();
	~action_t();
	// 设置接收结果指针,影响_dim
	void set_dst(float* p);
	void set_dst(glm::vec2* p);
	void set_dst(glm::vec3* p);
	void set_dst(glm::vec4* p);
	// 添加等待n秒后执行
	void add_wait(float st);
	// 添加路径动画命令,	mt移动所需时间(秒)，dim类型{1=float，2=vec2，3=vec3，4=vec4}，
	// target为路径数据，第一个值e_vmove = 1, e_vline=2, e_vcurve=3, e_vcubic=4，后面跟着坐标，count为坐标数量
	int add(float mt, int dim, float* target, int count);

	void play();	// 执行
	void pause();	// 暂停
	// 返回值：0不执行，1执行结束，2执行中
	int updata_t(float deltaTime);
	// 清空命令
	void clear();
};

// 直线移动，时间秒，目标，原坐标可选
action_t* move2w(float mt, const glm::vec2& target, glm::vec2* src, float wait);
// 在原坐标为原点增加移动
action_t* move2inc(const glm::vec2& pad, float mt, float wait);
action_t* at_size(const glm::vec2& dst, float mt);
action_show_t* wait_show(bool visible, float wait);

struct menumain_info
{
	form_x* form0; const char* fontn;
	uint32_t bc_color = 0xff000000;
	std::vector<std::string>* mvs;
	menu_info* pm = 0; int count = 0;   glm::vec2 msize = {};
	std::function<void(mitem_t* p, int type, int id)> cb;
	std::function<void(void* p, int clicks, int id)> mcb;
};
class listview_x
{
public:
	listview_x();
	~listview_x();
	void set_style(text_style_t* t);
private:
};

namespace mg {
	//创建主菜单
	menu_cx* new_mm(menumain_info* mm);
	void free_mm(menu_cx* p);


}
//!mg
namespace hz {
	// LinearRGB转换为sRGB：0-1的数值
	float rgb2srgbf(float linear);
	//	sRGB转换为LinearRGB 
	float srgb2rgbf(float s);
	uint32_t rgb2srgb(uint32_t rgb);
	uint32_t srgb2rgb(uint32_t s);
	bool is_json(const char* str, size_t len);
	// 去掉头尾空格
	std::string trim(const std::string& str, const char* pch);
	std::string trim_ch(const std::string& str, const std::string& pch);
	uint64_t toUInt(const njson& v, uint64_t de = 0);
	int64_t toInt(const njson& v, const char* k, int64_t de);
	int64_t toInt(const njson& v, int64_t de = 0);
	double toDouble(const njson& v, double de = 0);

	static bool toBool(const njson& v, bool def = false);
	bool toBool(const njson& v, const char* k, bool def);
	std::string toStr(const njson& v, const char* k, const std::string& des);
	std::string toStr(const njson& v, const std::string& des = "");
	std::string getStr(const njson& v, const std::string& key);
	std::string toStr(double price);
	std::string toStr(double price, int n);
	std::string toStr(int64_t n);
	uint64_t toHex(const njson& v, uint64_t d = 0);
	unsigned int toColor(const njson& v, unsigned int d = 0);
	std::string toColor2(unsigned int d);
	std::string ptHex(const std::string& str, char ch = 'x', char step = '\0');
	std::string ptHex(const void* data, size_t size, char ch = 'x', char step = '\0');
	glm::ivec2 toiVec2(const njson& v, int d = -1);
	glm::ivec2 toiVec2(const njson& v, glm::ivec2& ot);
	glm::vec2 toVec2(const njson& v, double d = 0);
	glm::vec2 toVec2(const njson& v, glm::vec2& ot);
	glm::vec3 toVec3(const njson& v, double d = 0);
	glm::vec4 toVec4(const std::vector<double>& v, bool one1 = false);
	glm::vec4 toVec4(const njson& v, double d = 0);
	glm::ivec4 toiVec4(const njson& v, int d = 0);
	glm::ivec3 toiVec3(const njson& v, int d = 0);
}
//!HZ