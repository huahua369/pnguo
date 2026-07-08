
/*
应用管理
创建：2026-05-10
https://github.com/huahua369/pnguo

struct ctrl_t {
	std::string name;
	std::string tips;
};
struct elem_t {
	int id = -1;		// 控件id
	int parent = -1;	// 父级id
};
json结构，css保存文本样式和布局，ctrl保存控件信息，panel保存面板信息
{
css:[{text_style},{flex_data},{text_style},{flex_data}],
ctrl:[{ctrl_t},{ctrl_t}],
panel:[],
}


*/


#include <pch1.h>
#include "vg.h"
#include "app.h"
#include "event.h"
#include "render.h"
#include "tinysdl3.h"
#include "font_core.h"
#include "mapView.h"
#include "pnguo.h"
#include "gui.h"
#include "audio.h"
#include <pnguo/print_time.h> 
#include "vkrenderer.h"
#include "gpu_vk.h"

#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif



namespace nlohmann {
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(glm::ivec2, x, y)
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(glm::vec2, x, y)
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(glm::ivec3, x, y, z)
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(glm::vec3, x, y, z)
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(glm::ivec4, x, y, z, w)
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(glm::vec4, x, y, z, w)
}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(flex_data,
	width, height,
	left, right, top, bottom,
	padding_left, padding_right, padding_top, padding_bottom,
	margin_left, margin_right, margin_top, margin_bottom,
	grow, shrink, order, basis, baseline,
	justify_content, align_content, align_items, align_self,
	position, direction, wrap,
	should_order_children
)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(flex_data1,
	width, height,
	offset,
	margin, padding,
	grow, shrink, order, basis, baseline,
	justify_content, align_content, align_items, align_self,
	position, direction, wrap,
	should_order_children
)

// 文本样式
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(text_style_str,
	family, styles,
	fontsize,
	lineheight,
	align,
	shadow_pos,
	stroke,
	color,
	color_stroke,
	color_shadow,
	mcolor_effect
)



std::string save_text_style_str(font_rctx* ctx, const text_style* t, int indent)
{
	text_style_str data;
	if (!ctx || !t) return "";
	auto fs = get_font_family_str(ctx, t->family);
	data.family = fs.family;
	data.styles = fs.styles;
	data.fontsize = t->fontsize;
	data.lineheight = t->lineheight;
	data.align = t->align;
	data.shadow_pos = t->shadow_pos;
	data.stroke = t->stroke;
	data.color = t->color;
	data.color_stroke = t->color_stroke;
	data.color_shadow = t->color_shadow;
	data.mcolor_effect = t->mcolor_effect;
	njson0 j = data;
	return j.dump(indent);
}

bool load_text_style_str(font_rctx* ctx, const std::string& json_str, text_style* t)
{
	if (!ctx || !t || json_str.empty())return false;
	njson0 j = njson0::parse(json_str);
	auto ss = j.get<text_style_str>();
	text_style ts;
	if (ctx)
		ts.family = new_font_family(ctx, ss.family.c_str(), ss.styles.c_str());
	ts.fontsize = ss.fontsize;
	ts.lineheight = ss.lineheight;
	ts.align = ss.align;
	ts.shadow_pos = ss.shadow_pos;
	ts.stroke = ss.stroke;
	ts.color = ss.color;
	ts.color_stroke = ss.color_stroke;
	ts.color_shadow = ss.color_shadow;
	ts.mcolor_effect = ss.mcolor_effect;
	if (t) *t = ts;
	return true;
}

// 保存：将结构体转为 JSON 字符串 
std::string save_flex_data(const flex_data& data, int indent) {
	njson0 j = data;
	return j.dump(indent);  // 缩进2 
}

std::string save_flex_data1(const flex_data1& data, int indent) {
	njson0 j = data;
	return j.dump(indent);  // 缩进2 
}

// 加载：从 JSON 字符串解析结构体 
flex_data load_flex_data(const std::string& json_str) {
	njson0 j = njson0::parse(json_str);
	return j.get<flex_data>();
}
void testnj() {
	//kaa_t c = { {1,2,3,4}, 5.0f };
	//njson0 j = c;
	//std::string json_str = j.dump(2);
	//printf("json_str:\n%s\n", json_str.c_str());
}
//-------------------------------------------------------------------------------------------------------------------------------------
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
		f = dv->form;
		glm::vec4 mrc = glm::ivec4(app->main->get_pos(), app->main->get_size());
		glm::vec4 frc = glm::ivec4(f->get_pos(), f->get_size());
		mrc.z += mrc.x; mrc.w += mrc.y;
		frc.z += frc.x; frc.w += frc.y;
		if (rect_includes(mrc, frc))
		{
			dom_cx* td3 = app->main->_dom;
			if (f->get_visible()) {
				dv->remove_widget(dv);
				td3->add_widget(dv);
				f->hide();
				if (cform.size() < max_form_cache)
					cform.push(f);
				else {
					app->remove(f);
				}
			}
			dv->set_pos(f->get_pos() - app->main->get_pos());
			f->viewports_enable = false;
			dv->form = app->main;
			dv->fpos = {};

			//dvv2->dindex = 1;
			//dvv2->fpos = {};
			//if (dvv2->form)
			//{
			//	dvv2->form->viewports_enable = false;
			//	dvv2->draggable = true;
			//}
			//dv->form = appx->app->main;
		}
		else {
			dv = 0;
		}
	}
	return dv;
}
void afree_dom(dom_cx* p) {
	if (p)delete p;
}
form_x* viewdev_cx::set_div(div_cx* d)
{
	if (!d)
		return nullptr;
	form_x* f1 = d->form;
	if (f1->viewports_enable) {

		return 0;
	}
	bool rm0 = (f1 == app->main) || !f1;
	if (rm0)
	{
		f1 = 0;
		if (cform.size())
		{
			f1 = cform.front(); cform.pop();
		}
	}
	dom_cx* td2 = 0;
	dom_cx* td3 = 0;
	if (!f1)
	{
		f1 = (form_x*)new_form(app, "", 500, 500, -1, -1, ef_utility | ef_resizable | ef_borderless | ef_vulkan/*ef_dx11 | ef_transparent*/);
		if (f1) {
			td2 = new dom_cx();
			if (!td2)
				app->remove(f1);
			else
				f1->_dom = td2;
		}
	}
	if (!f1)
		return nullptr;
	td2 = f1->_dom;
	if (td2)
		td2->init(f1, pcb, { 0,0,cache_size }, _vgdev, familys);
	auto pos = d->get_pos();
	auto size = d->get_size();
	f1->viewports_enable = true;
	f1->set_size(size);
	if (d->form && rm0) {
		pos += d->form->get_pos();
		td3 = d->form->_dom;
		//_rd.push({ td3,d });
		td3->remove_widget(d);
	}
	if (d->form != f1)
	{
		mfd[f1] = d;
		td2->add_widget(d);
		d->set_pos({});
		f1->show();
		//f1->raise();
		d->form = f1;
	}
	d->draggable = false;
	d->fpos = pos;
	f1->set_pos(pos);
	return f1;
}

dom_cx* viewdev_cx::get_dom(form_x* f)
{
	dom_cx* r = 0;
	if (f)
	{
		if (!f->_dom)
		{
			r = new dom_cx();
			if (r)
				r->init(f, pcb, { 0,0,cache_size }, _vgdev, familys);
			f->_dom = r;
		}
		else
		{
			r = f->_dom;
		}
	}
	return r;
}

void viewdev_cx::push_m(div_cx* d)
{
	auto dom = get_dom(app->main);
	if (dom)
		dom->add_widget(d);
}

void viewdev_cx::wait_dev()
{
	if (_vgdev)
		_vgdev->wait_dev();
}

void viewdev_cx::remove_q()
{
	for (; _rd.size();) {
		auto it = _rd.front();
		_rd.pop();
		if (it.d && it.p)
			it.d->remove_widget(it.p);
	}
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
			form0->_dom = 0;
		}
		form0 = f;
		if (form0)
		{
			form0->free_dom = afree_dom;
			form0->_dom = this;
			form0->add_event(this, [=](uint32_t type, et_un_t* e, void* ud) {
				bool btn = !((devent_type_e)type == devent_type_e::mouse_button_e && e->v.b->down == 0);
				auto dom = (dom_cx*)ud;
				if (dom) {
					for (auto it = dom->widgets.rbegin(); it != dom->widgets.rend(); it++)
					{
						(*it)->on_event(type, e);
						if (e->ret && btn)
							break;
					}
				}
				});
		}
		if (dc) {
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
		std::stable_sort(widgets.begin(), widgets.end(), [](div_cx* a, div_cx* b) {
		glm::ivec2 a0 = { a->order,a->dindex }, b0 = { b->order,b->dindex };
		return  a0 < b0;
			});

	ret += build();

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
	//printf("begin\n");
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
	//printf("end\n");
	int ms1 = rtc.end();
	static int oms = -1;
	//if (ms != oms)
	//{
	//	oms = ms;
	if (ms1 > 0)
	{
		ms1 = ms1;
		//printf("all draw %d ms\n", ms1);
	}
	//}
	return ret;
}

void dom_cx::pause()
{
	if (dc)	dc->pause();
}

int dom_cx::cmd_draw()
{
	if (dc)dc->cmd_draw();
	return 0;
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
	delete_font_family(family); family = 0;
	if (view)delete view; view = 0;
	free_vkdg(vkd);
	gd->release(dctx);
	free_gdev(gd); gd = 0;
	free_app(app);
}

void app_x::init(bool has3d)
{
	if (app)return;
	app = new_app();
	if (!app)return;
	hz::audio_backend_t abc = { app->get_audio_device(),app_cx::new_audio_stream,app_cx::free_audio_stream,app_cx::bindaudio,app_cx::unbindaudio,app_cx::unbindaudios
		,app_cx::get_audio_stream_queued,app_cx::get_audio_stream_available,app_cx::get_audio_dst_framesize
		,app_cx::put_audio,app_cx::pause_audio,app_cx::mix_audio,app_cx::clear_audio,app_cx::sleep_ms,app_cx::get_ticks };
	audio_ctx = new hz::audio_cx();
	audio_ctx->init(&abc, "data/config_music.json");
	audio_ctx->play_thread = false;
	audio_ctx->run_thread();
	gd = new_gdev(0, 0);
	dctx = (vkg::cxDevice*)gd->new_device(gd->ctx, 0, 0, 0, 0);
	dev_info_cx devinfo = {};
	get_dev_info(dctx, &devinfo);

	auto fctx = app->font_ctx;
	fctx->set_cache_size(font_csize.x, font_csize.y);
	family = new_font_family(fctx, (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Consolas");
	// 准备使用3D渲染器的设备创建SDL渲染器
	app->set_dev(devinfo.inst, devinfo.phy, devinfo.vkdev);
	if (has3d) {
		vkd = new_vkdg(devinfo.inst, devinfo.phy, devinfo.vkdev, 0, 0, 0);	// 创建vk渲染器 
		r3d = true;
	}
	view = new viewdev_cx();
	view->init_vgdev(&devinfo, 8);
	view->familys = family;
	view->app = app;
}

size_t app_x::run()
{
	size_t fc = 0;
	c_runtime_cx rtc; // 高精度计时器
	c_runtime_cx rt_cpu; // cpu计时器
	do {
		int ct = 0;
		auto delta = app->update_event();
		fc = app->form_count();
		if (!fc)break;
		if (audio_ctx)
			ct += audio_ctx->update(delta);

		cpums = rt_cpu.end();
		rt_cpu.begin();
		auto fps = app->get_fps();
		for (auto p : app->forms)
		{
			if (p && p->_dom)
			{
				auto dom0 = p->_dom;
				p->update(delta);
				if (p->is_minimized())
				{
					dom0->pause();
					continue;
				}
				auto io = p->get_io();
				// 判断鼠标是否点中控件，点中了就设置io->WantCaptureMouse = true，让3d渲染器不处理鼠标事件，交给控件处理
				if (dom0->press_test() && io)
					io->WantCaptureMouse = true;

			}
		}
		if (r3d && vkd)
		{
			auto io = app->main->get_io();
			vkd->update(io);	// 更新事件
			gpustr = vkd->get_label();
		}
		static double upt = 0.0;
		upt += delta;
		if (upt > 1.0)
		{
			upt = 0.0;
			if (fpslab)
				fpslab->str = vkr::format("CPU FPS\t\t: %d\nUIcmd\t\t\t: %d ms\n3Dcmd\t\t\t: %d ms\nSDLms\t\t\t: %d ms\nCPUms\t\t\t: %d ms\n", fps, uims, ms3d, SDLms, cpums) + gpustr;
		}
		rtc.begin();
		for (auto p : app->forms)
		{
			if (p && p->_dom && p->_size.x > 0)
			{
				auto dom0 = p->_dom;
				p->uct = dom0->update(delta) + ct;
			}
		}
		uims = rtc.end();
		int64_t sem3d = 0;
		if (r3d && vkd && app->main->get_visible())
		{
			rtc.begin();
			vkd->on_render();		// 渲染到fbo纹理tex3d
			if (!vkd->_state.has_fence) {
				sem3d = vkd->get_fbo_semaphore();
			}
			ms3d = rtc.end();
			app->main->uct++;
		}
		app->main->add_vk_semaphores(sem3d, (int64_t)0, 0);
		{
			rtc.begin();		// 开始计时录制SDL渲染命令
			for (auto p : app->forms)
			{
				if (p && p->uct > 0)
				{
					p->set_state();	// 清空/设置交换链接状态 
					if (p->is_render) {
						if (dtex3d) {
							texture_dt tdt = {};
							tdt.src_rect = { 0,0,vkd->width,vkd->height };
							tdt.dst_rect = tdt.src_rect;
							if (rc3d.z > 0 && rc3d.w > 0) { tdt.dst_rect = rc3d; }
							if (dtex3d) view->pcb->render_texture(app->main->renderer, dtex3d, &tdt, 1);//3d
						}
						p->_dom ? p->_dom->cmd_draw() : 0;
						p->present();
					}
				}
			}
			if (!r3d && view)
				view->wait_dev();
			SDLms = rtc.end();
		}
	} while (0);
	return app->form_count();
}
