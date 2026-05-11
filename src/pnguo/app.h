#pragma once
/*
	管理控件等信息
*/
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
private:

};

