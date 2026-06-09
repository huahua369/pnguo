#pragma once
/*
	管理控件等信息
*/
namespace hz {
	class audio_cx;
}
class app_cx;
class div_cx;
class form_x;
class vkvg_dev;
class drawable_cx;
struct vkvg_func_t;
struct dev_info_cx;
struct texture_cb;
struct font_family_t;

class viewdev_cx
{
public:
	app_cx* app = 0;
	texture_cb* pcb = 0;
	std::unordered_map<form_x*, div_cx*> mfd;
	std::vector<drawable_cx*> cavs;
	std::queue<form_x*> cform;	// 缓存窗口
	// 矢量图渲染用
	vkvg_dev* _vgdev = 0;
	vkvg_func_t* vgcb = 0;
	font_family_t* familys = 0;				// 默认字体
	glm::ivec2 cache_size = { 1024,1024 };		// 矢量图渲染缓存大小
	int max_form_cache = 2;
public:
	viewdev_cx();
	~viewdev_cx();
public:
	// 初始化矢量图渲染器，输入vk设备
	void init_vgdev(dev_info_cx* d, int sample = 8);
	div_cx* get_div(form_x* f);
	form_x* set_div(div_cx* d);
	void wait_dev();
private:

};

class drawable_cx;
class dom_cx
{
public:
	drawable_cx* dc = 0;
	// 控件列表
	std::vector<div_cx*> widgets, tdrawlist, tadd, tremove;
	form_x* form0 = 0;
public:
	dom_cx();
	~dom_cx();
	void init(form_x* f, texture_cb* cb, const glm::ivec4& view, vkvg_dev* vgdev, font_family_t* familys);

	void add_widget(div_cx* w);
	void remove_widget(div_cx* w);
	bool press_test();
	bool hittest(const glm::ivec2& mpos);

	int update(float delta);
	int build();
	// 暂停渲染
	void pause();
	void cmd_draw();
private:

};
class app_x
{
public:
	app_cx* app = 0;
	hz::audio_cx* audio_ctx = 0;
public:
	app_x();
	~app_x();
	void init(app_cx* a);
private:

};
