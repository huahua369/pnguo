
/*
应用管理
创建：2026-05-10
https://github.com/huahua369/pnguo
*/


#include <pch1.h>
#include "app.h"
#include "render.h"

viewdev_cx::viewdev_cx()
{}

viewdev_cx::~viewdev_cx()
{
	free_vkvgdev(_vgdev); _vgdev = nullptr;
}

void viewdev_cx::init_vgdev(dev_info_cx* d, int sample)
{
	_vgdev = new_vgdev_cx(d, sample);
}


