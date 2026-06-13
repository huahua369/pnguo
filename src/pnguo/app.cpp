
/*
应用管理
创建：2026-05-10
https://github.com/huahua369/pnguo
*/


#include <pch1.h>
#include "app.h"
#include "event.h"
#include "render.h"
#include "tinysdl3.h"
#include "vg.h"
#include "font_core.h"
#include "mapView.h"
#include "pnguo.h"
#include "gui.h"
#include "audio.h"
#include <pnguo/print_time.h> 
#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

viewdev_cx::viewdev_cx()
{}

viewdev_cx::~viewdev_cx()
{
	free_vkvgdev(_vgdev); _vgdev = nullptr;
	if (pcb)
		delete pcb; pcb = nullptr;
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
			dom_cx* td2 = 0;
			dom_cx* td3 = 0;
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
	dom_cx* td2 = 0;
	dom_cx* td3 = 0;
	if (!f1)
	{
		f1 = (form_x*)new_form(app, "", 500, 500, -1, -1, ef_utility | ef_resizable | ef_borderless | ef_vulkan/*ef_dx11 | ef_transparent*/);
		if (f1) {
			td2 = new dom_cx();
			if (td2)
				td2->init(f1, pcb, { 0,0,cache_size }, _vgdev, familys);
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

void viewdev_cx::wait_dev()
{
	if (_vgdev)
		_vgdev->wait_dev();
}



dom_cx::dom_cx()
{}

dom_cx::~dom_cx()
{
	for (auto pt : widgets)
	{
		if (pt)
			delete pt;
	}
	if (dc)delete dc; dc = 0;
}


void dom_cx::init(form_x* f, texture_cb* cb, const glm::ivec4& view, vkvg_dev* vgdev, font_family_t* familys)
{
	if (!dc)
		dc = new drawable_cx();
	if (f)
	{
		if (form0 && form0 != f) {
			form0->remove_event(this);
		}
		form0 = f;
		//f->add(this);
		if (form0)
		{
			form0->add_event(this, [=](uint32_t type, et_un_t* e, void* ud) {
				bool btn = !((devent_type_e)type == devent_type_e::mouse_button_e && e->v.b->down == 0);

				for (auto it = widgets.rbegin(); it != widgets.rend(); it++)
				{
					(*it)->on_event(type, e);
					if (e->ret && btn)
						break;
				}
				});
		}
		if (dc) {
			if (dc->rptr && dc->rptr != f->renderer) {
				dc->free_tex();
			}
			dc->rptr = f->renderer;
		}
	}
	if (dc) {
		dc->init(cb, view, vgdev);
		dc->familys = familys;
	}
}


void dom_cx::add_widget(div_cx* w)
{
	if (w)
	{
		tadd.push_back(w);
	}
}

void dom_cx::remove_widget(div_cx* w)
{
	if (w)
	{
		tremove.push_back(w);
	}
}

bool dom_cx::press_test()
{
	for (auto& p : tdrawlist) {
		if (p && p->visible && p->press_test())
		{
			return true;
		}
	}
	return false;
}

bool dom_cx::hittest(const glm::ivec2& mpos)
{
	for (auto& p : tdrawlist) {
		if (p && p->visible && p->hittest(mpos))
		{
			return true;
		}
	}
	return false;
}

int dom_cx::update(float delta)
{
	int ret = 0;
	if (!dc)return ret;
	dc->_pause;
	dc->_pause = false;
	if (!tremove.empty())
	{
		for (auto w : tremove) {
			auto it = std::find(widgets.begin(), widgets.end(), w);
			if (it != widgets.end()) {
				widgets.erase(it); ret++;
			}
		}
		tremove.clear();
	}
	if (!tadd.empty())
	{
		for (auto w : tadd) {
			auto it = std::find(widgets.begin(), widgets.end(), w);
			if (it == widgets.end()) {
				widgets.push_back(w); ret++;
			}
		}
		tadd.clear();
	}
	for (auto& p : widgets) {
		ret += p->update(delta);
	}
	if (ret > 0)
		std::stable_sort(widgets.begin(), widgets.end(), [](div_cx* a, div_cx* b) { return a->dindex < b->dindex; });
 
	ret += build();
	if (ret > 0)
		dc->cmd_draw();
	return ret;
}
void get_div(widget_t* p, std::vector<div_cx*>* v)
{
	auto d = dynamic_cast<div_cx*>(p);
	if (d && d->visible)
	{
		v->push_back(d);
		for (auto it : d->widgets)
		{
			get_div(it, v);
		}
	}
}
int dom_cx::build()
{
	int ret = 0;
	if (!dc)return ret;
	dc->clear_draw();
	tdrawlist.clear();
	for (auto& p : widgets) {
		if (p && p->visible)
		{
			get_div(p, &tdrawlist);
		}
	}
	//printf("draw 0\n");
	c_runtime_cx rtc;
	rtc.begin();
#if 0
	auto& rvgd = dc->_dobj[0];
	if (!rvgd)
		rvgd = new rvg_data_cx();
	rvg_cx* rvg = rvgd->get();
	rvg->clear();
	for (auto& p : tdrawlist) {
		if (p)
		{
			glm::ivec2 dpos = {};
			if (p->parent)
				dpos = p->_pos;
			if (rvg && rvgd)
			{
				//print_time _bb("build");
				rvg->set_pos(p->get_pos());
				p->draw(rvg);	// 录制渲染
			}
		}
	}
	// 资源绑定窗口
	int ms = rtc.end();
	rtc.begin();
	ret += dc->update_rvgdata(rvgd);
#else

	for (auto& p : tdrawlist) {
		if (p)
		{
			auto& rvgd = dc->_dobj[p];
			if (!rvgd)
				rvgd = new rvg_data_cx();
			rvg_cx* rvg = rvgd->get();
			rvg->clear();
			glm::ivec2 dpos = {};
			if (p->parent)
				dpos = p->_pos;
			if (rvg && rvgd)
			{
				//print_time _bb("build");
				rvg->set_pos(p->get_pos());
				p->draw(rvg);	// 录制渲染
			}
		}
	}

	int ms = rtc.end();
	rtc.begin();
	for (auto& p : tdrawlist) {
		if (p)
		{
			auto& rvgd = dc->_dobj[p];
			if (!rvgd)
				continue;
			// 生成资源
			ret += dc->update_rvgdata(rvgd);
		}
	}
#endif
	int ms1 = rtc.end();
	static int oms = -1;
	//if (ms != oms)
	//{
	//	oms = ms;
	//	printf("draw %d ms\n", ms);
	//}
	return ret;
}

void dom_cx::pause()
{
	if (dc)	dc->pause();
}

void dom_cx::cmd_draw()
{
	if (dc)dc->cmd_draw();
}


void test_hueui()
{
	div_cx* colorpicker = 0; drawable_cx* td3 = 0;
	std::vector<text_vx> vtx;
	std::vector<uint32_t> idx;
	glm::ivec2 psize = { 360,360 };
	glm::vec4 c1 = glm::vec4(0, 0, 0.8, 1.0);// 0xffcc0000;
	glm::vec4 ohsv = { 0.7,1,1,1 };
	glm::vec4 pick0 = c1, pick1 = c1;
	c1 = HSVtoRGB(ohsv);
	auto vtx_pos = vtx.size();
	auto vtxd = vtx.data();
	// 获取色盘颜色
	static auto get_color_cb = [](const glm::ivec2& pos, const glm::ivec2& size, float h) {
		glm::vec2 n = (glm::vec2)pos / (glm::vec2)size;
		glm::vec4 hc = {};
		glm::vec4 hsv = { h,0,0,1 };
		hsv.y = n.x;
		hsv.z = 1.0 - n.y;
		hc = HSVtoRGB(hsv);
		return hc;
		};
	// 获取色调颜色
	static auto get_hue_color_cb = [](const glm::ivec2& pos, const glm::ivec2& size) {
		glm::vec2 n = (glm::vec2)pos / (glm::vec2)size;
		glm::vec4 hc = {};
		glm::vec4 hsv = { n.x,0,0,1 };
		hsv.y = 1;
		hsv.z = 1.0 - n.y;
		hc = HSVtoRGB(hsv);
		return hc;
		};

	colorpicker->mevent_cb = [=, &pick0, &pick1, &vtx_pos, &vtx, &idx](void* p, int type, const glm::vec2& mpos)
		{
			if (type == (int)event_type2::on_drag) {
				glm::vec2 pos = colorpicker->get_pos() + 6;
				glm::ivec4 rc = { 0,0,psize };
				rc.x = pos.x;
				rc.y = pos.y;
				vtx.clear();
				idx.clear();
				vtx_pos = build_huedata(c1, rc, vtx, idx);
				rc.x += rc.z + 4;
				auto rc1 = rc;
				rc1.x += rc1.z + 6;
				rc1.z = rc1.w = 50;
				gen_rect_color(rc1, pick0, vtx, idx);	//颜色块
				rc1.x += 56;
				gen_rect_color(rc1, pick1, vtx, idx);	//颜色块
				colorpicker->valid = true;
			}
			else if (type == (int)event_type2::on_down) {
				auto mps = mpos - (glm::vec2)colorpicker->get_pos();
				mps -= 6;
				if (mps.y > psize.y)
					return;
				auto d = vtx.data() + vtx_pos;
				if (mps.x < psize.x)
				{
					pick0 = get_color_cb(mps, psize, ohsv.x);
					for (size_t i = 0; i < 4; i++)
					{
						d[i].color = pick0;
					}
				}
				d += 4;
				mps.x -= 4 + psize.x;
				if (mps.x > 0 && mps.x < psize.x)
				{
					pick1 = get_hue_color_cb(mps, psize);
					for (size_t i = 0; i < 4; i++)
					{
						d[i].color = pick1;
					}
				}
				colorpicker->valid = true;
			}
		};

	glm::vec2 pos = colorpicker->get_pos() + 6;
	glm::ivec4 rc = { 0,0,psize };
	rc.x = pos.x;
	rc.y = pos.y;
	vtx.clear();
	idx.clear();
	vtx_pos = build_huedata(c1, rc, vtx, idx);
	rc.x += rc.z + 4;
	auto rc1 = rc;
	rc1.x += rc1.z + 6;
	rc1.z = rc1.w = 50;
	gen_rect_color(rc1, pick0, vtx, idx);	//颜色块
	rc1.x += 56;
	gen_rect_color(rc1, pick1, vtx, idx);	//颜色块
	auto vd = vtx.data();
	td3->draw_geometry(0, glm::ivec4(colorpicker->get_pos(), colorpicker->get_size()), &vtx, &idx);
}


app_x::app_x()
{}

app_x::~app_x()
{
	if (audio_ctx)
		delete audio_ctx;
	audio_ctx = 0;
	free_app(app);
}

void app_x::init()
{
	if (app)return;
	app = new_app();
	hz::audio_backend_t abc = { app->get_audio_device(),app_cx::new_audio_stream,app_cx::free_audio_stream,app_cx::bindaudio,app_cx::unbindaudio,app_cx::unbindaudios
,app_cx::get_audio_stream_queued,app_cx::get_audio_stream_available,app_cx::get_audio_dst_framesize
,app_cx::put_audio,app_cx::pause_audio,app_cx::mix_audio,app_cx::clear_audio,app_cx::sleep_ms,app_cx::get_ticks };
	audio_ctx = new hz::audio_cx();
	audio_ctx->init(&abc, "data/config_music.json");
	audio_ctx->play_thread = false;
	audio_ctx->run_thread();
}
