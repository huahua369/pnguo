
/*
应用管理
创建：2026-05-10
https://github.com/huahua369/pnguo
*/


#include <pch1.h>
#include "app.h"
#include "render.h"
#include "tinysdl3.h"
#include "vg.h"
#include "font_core.h"
#include "mapView.h"
#include "pnguo.h"
#include "gui.h"

viewdev_cx::viewdev_cx()
{}

viewdev_cx::~viewdev_cx()
{
	free_vkvgdev(_vgdev); _vgdev = nullptr;
}

void viewdev_cx::init_vgdev(dev_info_cx* d, int sample)
{
	if (!pcb)
		pcb = new texture_cb();
	assert(pcb);
	if (pcb)
		get_sdl_texture_cb(pcb);
	_vgdev = new_vgdev_cx(d, sample);
}

div_cx* viewdev_cx::get_div(form_x* f)
{
	auto dv = mfd[f];
	if (dv && app->main && f)
	{
		glm::vec4 mrc = glm::ivec4(app->main->get_pos(), app->main->get_size());
		glm::vec4 frc = glm::ivec4(f->get_pos(), f->get_size());
		mrc.z += mrc.x; mrc.w += mrc.y;
		frc.z += frc.x; frc.w += frc.y;
		if (rect_includes(mrc, frc))
		{
			drawable_cx* td2 = 0;
			drawable_cx* td3 = 0;
			td2->remove_widget(dv);
			td3->add_widget(dv);
			dv->set_pos(f->get_pos() - app->main->get_pos());
			f->hide();
			if (cform.size() < max_form_cache)
				cform.push(f);
			else {
				app->remove(f);
			}
		}
		else {
			dv = 0;
		}
	}
	return dv;
}

form_x* viewdev_cx::set_div(div_cx* d)
{
	if (!d)
		return nullptr;
	form_x* f1 = 0;
	if (cform.size())
	{
		f1 = cform.front(); cform.pop();
	}
	drawable_cx* td2 = 0;
	drawable_cx* td3 = 0;
	if (!f1)
	{
		f1 = (form_x*)new_form(app, "", 500, 500, -1, -1, ef_utility | ef_resizable | ef_borderless | ef_vulkan/*ef_dx11 | ef_transparent*/);
		if (f1) {
			td2 = new drawable_cx();
			if (td2)
				td2->init(f1, pcb, { 0,0,cache_size }, _vgdev);
			else
				app->remove(f1);
		}
	}
	if (!f1)
		return nullptr;
	//td2 = f1->_draw_data[0];
	//auto pos = d->get_pos();
	//auto size = d->get_size();
	//f1->set_size(size);
	//if (d->form && d->form != f1) {
	//	pos += d->form->get_pos();
	//	td3 = d->form->_draw_data[0];
	//	td3->remove_widget(d);
	//}
	//if (d->form != f1)
	//{
	//	mfd[f1] = d;
	//	td2->add_widget(d);
	//	d->set_pos({});
	//	f1->show();
	//	//f1->raise();
	//	d->form = f1;
	//}
	//f1->set_pos(pos);
	return f1;
}

void* viewdev_cx::new_semaphore()
{
	return nullptr;
}

