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

class viewdev_cx
{
public:
	std::vector<div_cx*> divs;
	std::vector<form_x*> forms;
	std::vector<drawable_cx*> cavs;
	// 矢量图渲染用
	vkvg_dev* _vgdev = 0;
	vkvg_func_t* vgcb = 0;
public:
	viewdev_cx();
	~viewdev_cx();
public:
// 初始化矢量图渲染器，输入vk设备
	void init_vgdev(dev_info_cx* d, int sample = 8);
	void update(float delta);
private:

};

