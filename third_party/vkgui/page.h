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
	glm::ivec2 color = {0xf0121212,0xaaffffff};	// 背景色，边框色
	float thickness = 1.0;	// 线宽
	float radius = 4;		// 矩形圆角 
};

class menu_cx;
class layout_text_x;
// 菜单项
class mitem_t
{
public:
	std::vector<std::string> v;
	std::function<void(mitem_t* p, int type, int id)> ckm_cb;
	int width = -1;		// 菜单宽度
	int height = 0;	// 菜单项高
	glm::ivec2 pos = {};
	menu_cx* m = 0;
	form_x* f = 0;
	mitem_t* parent = 0;	// 父级
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
	glm::ivec2 get_idx_pos(int idx);
};
// 菜单管理器
class menu_cx
{
public:
	form_x* form = 0;			// 主窗口 
	std::vector<std::string> familys;
public:
	menu_cx();
	~menu_cx();
	void set_main(form_x* f);
	void add_familys(const char* family);
	mitem_t* new_menu(int width, int height, const std::vector<std::string>& mvs, std::function<void(mitem_t* p, int type, int id)> cb);
	void show_item(mitem_t* it, const glm::vec2& pos);
	void free_item(mitem_t* p);
private:

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


