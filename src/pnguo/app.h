#pragma once
#include "tinysdl3.h"
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
class vkdg_cx;
class drawable_cx;
struct image_btn;
struct color_btn;
struct gradient_btn;
struct radio_tl;
struct checkbox_tl;
struct switch_tl;
struct progress_tl;
struct slider_tl;
struct colorpick_tl;
struct scroll_bar;
class edit_cx;
class dom_cx;

class viewdev_cx
{
public:
	app_cx* app = 0;
	texture_cb* pcb = 0;
	std::unordered_map<form_x*, div_cx*> mfd;
	std::vector<drawable_cx*> cavs;
	std::queue<form_x*> cform;	// 缓存窗口
	struct rem_div {
		dom_cx* d = 0;
		div_cx* p = 0;
	};
	std::queue<rem_div> _rd;	// 缓存删除
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
	dom_cx* get_dom(form_x* f);
	void wait_dev();
	void remove_q();
private:

};

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
	int cmd_draw();
private:

};
class app_x
{
public:
	app_cx* app = 0;
	hz::audio_cx* audio_ctx = 0;
	viewdev_cx* view = 0;
	vkdg_cx* vkd = 0;	// 3d渲染器
	color_btn* fpslab = 0;
	std::string gpustr;
	int uims = 0, ms3d = 0, SDLms = 0, cpums = 0;
	int iec = 0;
	bool r3d = 0;
public:
	app_x();
	~app_x();
	void init();
	size_t run();
private:

};

// 保存：将结构体转为 JSON 字符串 
std::string save_flex_data(const flex_data& data);
std::string save_flex_data1(const flex_data1& data);
// 加载：从 JSON 字符串解析结构体 
flex_data load_flex_data(const std::string& json_str);
