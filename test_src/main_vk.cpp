/*
文本渲染：fontconfig、stb_truetype、harfBuzz、icu
矢量图渲染： vkvg
窗口：SDL3

文本渲染数据：图集+三角形
视图纹理：前景、背景各一张。文本渲染在中间

*/
#include <pch1.h>
#include <random>
#include <pnguo/win_core.h>
#include "win32msg.h"
#include <SDL3/SDL.h>
#include <pnguo/event.h>
//#define GUI_STATIC_LIB
#include <pnguo/pnguo.h>
#include <pnguo/tinysdl3.h>
#include <pnguo/app.h>
#include <pnguo/vkrenderer.h>
#include <pnguo/gpu_vk.h>
#include <pnguo/page.h>
#include <pnguo/mapView.h>
#include <pnguo/print_time.h> 
#include <pnguo/mesh3d.h>
#include "mshell.h"
//#include <cgltf.h>
#include <mcut/stlrw.h> 
#include <spine/spine-sdl3/spinesdl3.h>
#include <pnguo/render.h>
#include <pnguo/win_core.h>

#include <pnguo/plot.h>
auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman";// , Malgun Gothic";

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <entt/entt.hpp>

hz::audio_cx* audiofft(app_cx* app) {

	static hz::audio_backend_t abc = { app->get_audio_device(),app_cx::new_audio_stream,app_cx::free_audio_stream,app_cx::bindaudio,app_cx::unbindaudio,app_cx::unbindaudios
,app_cx::get_audio_stream_queued,app_cx::get_audio_stream_available,app_cx::get_audio_dst_framesize
,app_cx::put_audio,app_cx::pause_audio,app_cx::mix_audio,app_cx::clear_audio,app_cx::sleep_ms,app_cx::get_ticks };
	auto audio_ctx = new hz::audio_cx();
	audio_ctx->init(&abc, "data/config_music.json");
	audio_ctx->play_thread = false;
	audio_ctx->run_thread();
	audio_ctx->add_song(0, R"(E:\song\G.E.M.邓紫棋-桃花诺.flac)");
	// 设置播放歌单，只有一个歌单，所以设置0
	audio_ctx->set_gd(0);
	// 设置播放类型: 0单曲播放，1单曲循环，2顺序播放，3循环播放，4随机播放
	audio_ctx->set_type(3);
	// 播放当前歌单指定索引开始播放
	audio_ctx->play(0);
	return audio_ctx;
	auto mad1 = audio_ctx->get_list_it(0)->v[0]->data;
	hz::fft_cx* fft = new hz::fft_cx();
	hz::fft_cx* fft1 = new hz::fft_cx();
	double dtime = 0.06;
	int fs = mad1->sample_rate * dtime;
	{
		while (1)
		{
			int rc1 = decoder_data(mad1);
			if (rc1 <= 0)
			{
				break;
			}
		}
		fft->init(mad1->sample_rate, mad1->bits_per_sample, 0, 0);
		fft->draw_pos;
		fft->is_raw = true;
		fft1->init(mad1->sample_rate, mad1->bits_per_sample, 0, 0);
		fft1->draw_pos.y += 110;
		fft1->bar_width = 6;
		fft1->bar_step = 0;
		fft->bar_width = 6;
		fft->bar_step = 3;
	}
	{
		// 渲染回调
		SDL_Renderer* renderer = 0; double delta = 0.0;
		static double deltas = 0;
		deltas += delta;
		if (deltas > dtime)
		{
			static int64_t kn = 0;
			static int64_t kn1 = 0;
			static int64_t sn = 256;
			static int64_t sn1 = 2048;
			deltas = 0;
			fft->calculate_heights((short*)mad1->data + kn, sn * 2, 100);
			fft1->calculate_heights((short*)mad1->data + kn1, sn1 * 2, 100);
			kn += fs;
			kn1 += fs;
			if (kn >= mad1->total_samples)
			{
				kn = 0;
			}
			if (kn1 >= mad1->total_samples)
			{
				kn1 = 0;
			}
		}
		glm::vec4 color = { 0,0.5,1.0,0.8 };
		static std::vector<SDL_Vertex> vertices;
		gen_rects(fft->_rects, vertices, { 0.1, 1, 0.1, 0.9 }, { 1, 0.5, 0, 1 });
		SDL_RenderGeometry(renderer, nullptr, vertices.data(), vertices.size(), nullptr, 0);
		gen_rects(fft1->_rects, vertices, { 0.5, 0.0, 0, 0.00 }, { 1, 0.15, 0, 0.9 });
		SDL_RenderGeometry(renderer, nullptr, vertices.data(), vertices.size(), nullptr, 0);
	}
}
int main()
{
	auto k = time(0);
	main_heightmap(k);
	Sleep(1000);
	if (1) {
		//hz::main_ssh2();
		//return 0;
		//test_img();

		auto app = new_app();
		// 常用分辨率
		glm::ivec2 dpis[] = {
			{1024,768},
			{1152,864},
			{1280,720},
			{1280,768},
			{1280,800},
			{1280,960},
			{1280,1024},
			{1360,768},
			{1366,768},
			{1400,1050},
			{1440,900},
			{1600,900},
			{1680,1050},
			{1920,1080},
		};
		// 渲染fbo尺寸比例从50%到200%,步长5%
		int fbo_scale[3] = { 50,200,5 };
#ifdef _WIN32
#ifdef _DEBUG
		system("rd /s /q E:\\temcpp\\SymbolCache\\tcmp.pdb");
		system("rd /s /q E:\\temcpp\\SymbolCache\\vkcmp.pdb");
		system("rd /s /q E:\\temcpp\\SymbolCache\\cedit.pdb");
		system("rd /s /q E:\\temcpp\\SymbolCache\\p86.pdb");
		//auto rd = hz::shared_load(R"(E:\Program Files\RenderDoc_1.37_64\renderdoc.dll)");

#endif 
#endif // _WIN32


		auto fctx = app->font_ctx;
		fctx->set_cache_size(512, 512);
		auto family = new_font_family(fctx, (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Consolas");

		glm::ivec2 ws = dpis[4];// { 1280, 860 };
		const char* wtitle = (char*)u8"窗口0";
		const char* wtitle1 = (char*)u8"窗口1";
		auto devname = "Intel(R)";
		devname = 0;
		adevice3_t* gd = new_gdev(0, 0);
		auto dctx = (vkg::cxDevice*)gd->new_device(gd->ctx, 0, 0, 0, 0);
		dev_info_cx devinfo = {};
		get_dev_info(dctx, &devinfo);

		vkdg_cx* vkd = new_vkdg(devinfo.inst, devinfo.phy, devinfo.vkdev, 0, 0, devname);	// 创建vk渲染器 

		auto sampler = gd->new_object((aDevice*)dctx, (int)obj_type_e::OBJ_SAMPLER, "sampler");	// 创建vk渲染器对象

		// 准备使用3D渲染器的设备创建SDL渲染器
		app->set_dev(vkd->_dev_info.inst, vkd->_dev_info.phy, vkd->_dev_info.vkdev);

		//vkr::new_ms_pipe(vkd->_dev_info.vkdev, vkd->renderpass_opaque);
		form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable /*| ef_vsync| ef_borderless*/);
		//form_x* form1 = (form_x*)new_form(app, wtitle1, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable);
		auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
		app->main = form0;
		auto kd = sdldev.vkdev;

		//void* aaaa = vkr::new_ms_pipe(vkd->dev, vkd->renderpass_fbo);

		//sdldev.vkdev = 0;	// 清空使用独立创建逻辑设备
		auto devs = vkr::get_devices(sdldev.inst); // 获取设备名称列表
		bool grab_enable = false;	// 设置鼠标范围在窗口内
		bool rmode = true;			// 设置窗口的相对鼠标模式。
		//form0->set_mouse_mode(grab_enable, rmode);
		get_queue_info(sdldev.phy);
		// 菜单窗口
		//form_x* popw = new_form_popup(form0, 200, 600);

		//form_x* ttw = new_form1(app, 200, 600, 0);
		//popw->set_pos({ -100,00 });
		//ttw->set_pos({ 50,100 });
		//popw->render_cb = [=](SDL_Renderer* renderer, double delta)
		//	{
		//		SDL_FRect rc = { 0,0,180,580 };
		//		SDL_SetRenderDrawColorFloat(renderer, 0.90f, 0.10f, 0.0f, 0.8f);
		//		SDL_RenderFillRect(renderer, &rc);
		//	};

		//ttw->render_cb = [=](SDL_Renderer* renderer, double delta)
		//	{
		//		SDL_FRect rc = { 0,0,180,580 };
		//		SDL_SetRenderDrawColorFloat(renderer, 0.10f, 0.90f, 0.0f, 0.8f);
		//		SDL_RenderFillRect(renderer, &rc);
		//	};
		//ttw->mmove_type = 1;
		//popw->mmove_type = 1;
		//popw->_focus_lost_hide = 0; 
		bool shadowMap = true;
		if (vkd) {
			auto rmd = hz::read_json("data/vksample.json");
			auto& scenes = rmd["scenes"];
			if (scenes.empty())
			{
				{
					njson item;
					item["name"] = "busterDrone";
					item["dir"] = R"(E:\model\testgltf\)";
					item["filename"] = "busterDrone.gltf";
					item["pos"] = { 0.0,1.8,0.0 };
					item["scale"] = 1.0;
					item["shadowMap"] = true;
					item["visible"] = true;
					scenes.push_back(item);
				}
				{
					njson item;
					item["name"] = "droide";
					item["dir"] = R"(E:\model\testgltf\)";
					item["filename"] = "free_droide_de_seguridad_k-2so_by_oscar_creativo.glb";
					item["pos"] = { 0.0,0.0,0.0 };
					item["scale"] = 10.0;
					item["shadowMap"] = true;
					item["visible"] = true;
					scenes.push_back(item);
				}
				hz::save_json("data/vksample.json", rmd, 2);
			}
			for (auto& it : scenes)
			{
				std::string path = it["dir"];
				std::string fn = it["filename"];
				if (fn.empty() || !hz::toBool(it["visible"]))continue;
				if (path.size()) {
					char xx = *path.rbegin();
					char fx = fn[0];
					if (!(xx == '\\' || xx == '/') && !(fx == '\\' || fx == '/')) {
						path += "/";
					}
				}
				path += fn;
				if (hz::access_2(path.c_str()))
				{
					auto pos = hz::toVec3(it["pos"]);
					vkd->add_gltf(path.c_str(), pos, hz::toDouble(it["scale"], 1.0), hz::toUInt(it["instanceCount"], 1.0), hz::toBool(it["shadowMap"]));
				}
			}
			vkd->resize(ws);				// 设置fbo缓冲区大小
			auto vki = vkd->get_vkimage(0);	// 获取fbo纹理弄到窗口显示 nullptr;//
			/*
			case 0: return AMDTonemapper(color);
			case 1: return DX11DSK(color);
			case 2: return Reinhard(color);
			case 3: return Uncharted2Tonemap(color);
			case 4: return tonemapACES( color );
			case 5: return color;
			*/
			vkd->_state.SelectedTonemapperIndex = 0;
			vkd->_state.Exposure = 0.8;
			vkd->_state.EmissiveFactor = 1;
			vkd->_state.IBLFactor = 0.5;
			//new_ui(form0, vkd); 
			//void* vg2dtex = nullptr;
			const char* filename = "temp/vkvg_gb.png";
			bspline_ct* bs = new bspline_ct();
			std::vector<glm::vec2> pts = { {100,500},{200,600},{300,400},{400,700},{500,500} };
			auto bptr = bs->new_bspline(pts.data(), pts.size());

			auto view = new viewdev_cx();
			view->init_vgdev(&devinfo, 8);
			view->familys = family;
			view->app = app;
			auto pcb = view->pcb;
			dom_cx* dom0 = new dom_cx();
			dom0->init(form0, pcb, { 0,0,ws.x, ws.y }, view->_vgdev, family);

			//auto ptm = plot_main(dom0, 1200, 1200);


			void* tex3d = pcb->new_texture_vk(form0->renderer, vki.size.x, vki.size.y, vki.vkimage, 0);// 创建SDL的rgba纹理 
			pcb->set_texture_blend(tex3d, 0, 0);


			auto dvv = new div_cx();
			dvv->set_size({ 500,400 });
			dvv->set_pos({ 100,60 });
			dvv->family = family;
			dvv->border = { 0xffacacac,1,5,0x9f666666 };	// 颜色，线粗，圆角，背景色
			dvv->flex.wrap = flex_wrap::WRAP;
			dvv->flex.direction = flex_direction::ROW;
			dvv->flex.justify_content = flex_align::ALIGN_START;	// x轴，主轴对齐
			dvv->flex.align_items = flex_align::ALIGN_CENTER;		// y轴，交叉轴对齐
			dvv->flex.align_content = flex_align::ALIGN_START;		// y轴，多行交叉轴对齐
			dvv->flex_child.margin_left = 2;		// 子元素外边距
			dvv->flex_child.margin_right = 2;
			dvv->flex_child.margin_top = 2;
			dvv->flex_child.margin_bottom = 2;
			dvv->draggable = true;
			//dvv->flex.direction = flex_direction::COLUMN;
			uint32_t colors[5] = { 0x905050fc,0x9050fc50,0x90fc5050,0x90ffffff,0x90282828 };
			//x=默认，y=鼠标进入，z=按下
			glm::uvec3 gradTop = { 0xff4a4a4a,0x80404040,0xff292929 }, gradBot = { 0xff3a3a3a,0x80303030,0xff1d1d1d };
			glm::uvec3 gradTop1 = { 0xff8a8a8a,0x80bebebe,0xff303030 }, gradBot1 = { 0xff5a5a5a,0x80303030,0xff1d1d1d };
			glm::uvec2 blackb = { 0x805c5c5c , 0x801d1d1d };
#if 1
			for (int i = 0; i < 5; i++) {
				auto btn = new gradient_btn();
				btn->rounding = 4;
				dvv->add_widget(btn);
				btn->set_size({ 200,36 });
				btn->back_color = colors[i];
				btn->borderLight = blackb.x;
				btn->borderDark = blackb.y;
				btn->gradTop = gradTop;
				btn->gradBot = gradBot;
				btn->font_size = 16;
				btn->text_color = -1;
				btn->str = (char*)u8"🔥按钮gbutton " + std::to_string(i);
				if (i == 4) {
					//btn->borderLight = blackb.y;
					//btn->borderDark = blackb.x;
					btn->gradTop = gradTop1;
					btn->gradBot = gradBot1;
				}
			}
			auto gr = new group_radio_t();
			for (int i = 0; i < 5; i++) {
				auto r = new radio_tl();
				r->gr = gr;
				r->set_size({ 36,36 });
				r->style.line_col = 0xffff8020;
				r->style.thickness = 1;
				r->style.radius = 7;
				dvv->add_widget(r);
			}
			for (int i = 0; i < 7; i++) {
				auto btn = new color_btn();
				btn->rounding = 4;
				btn->set_btn_color_bgr(fmod(i, 5));
				dvv->add_widget(btn);
				btn->set_size({ 128,36 });
				btn->font_size = 16;
				btn->text_color = 0;
				btn->str = (char*)u8"🍕按钮 " + std::to_string(5 + i);
			}
			for (int i = 0; i < 4; i++) {
				auto r = new checkbox_tl();
				r->set_size({ 36,36 });
				r->style.line_col = 0xffff8020;
				r->style.thickness = 1;
				dvv->add_widget(r);
			}
			{
				auto r = new checkbox_tl();
				r->set_size({ 36,36 });
				r->style.line_col = 0xffff8020;
				r->style.thickness = 1;
				r->v.mixed = true;
				dvv->add_widget(r);
			}
#endif

			auto edit1 = new edit_cx();
			{
				auto r = edit1;
				r->set_size({ 236,32 });
				//r->set_single(false);
				r->placeholder = (char*)u8"输入文本";
				dvv->add_widget(r);
				r->dindex = 0;
			}
			auto pro = new progress_tl();
			dvv->add_widget(pro);
			pro->set_size({ 120,26 });
			pro->right_inside = true;
			pro->rounding = 10;
			pro->height = 20;
			pro->width = 110;
			pro->color = { 0xffff9e40, 0x5f6c6c6c };
			pro->set_value(0.5);
			auto colorpick = new colorpick_tl();
			{
				auto c = colorpick;
				c->init(0xff282828, 300, 20, true);
				c->font_size = 15;
				dvv->add_widget(c);
			}
			static bool loadf = false;

			auto dvv2 = new div_cx();
			{
				auto dvv1 = new div_cx();
				dvv1->set_size({ 180,150 });
				dvv1->set_pos({ 100,60 });
				dvv1->family = family;
				dvv1->border = { 0xffacacac,1,5,0xaf66f666 };	// 颜色，线粗，圆角，背景色
				dvv1->flex_child.margin_left = 2;		// 子元素外边距
				dvv1->flex_child.margin_right = 2;
				dvv1->flex_child.margin_top = 2;
				dvv1->flex_child.margin_bottom = 2;
				dvv1->flex.wrap = flex_wrap::WRAP;
				dvv1->draggable = true;
				dvv1->_absolute = true;
				{
					auto btn = new color_btn();
					btn->rounding = 4;
					btn->set_btn_color_bgr(fmod(4, 5));
					dvv1->add_widget(btn);
					btn->set_size({ 128,36 });
					btn->font_size = 16;
					btn->text_color = -1;
					btn->str = (char*)u8"🍕按钮 ";
					btn->click_cb = [=](void* p, int clicks, const glm::vec2& mpos) {

						};
					{
						auto r = new checkbox_tl();
						r->set_size({ 36,36 });
						r->style.line_col = 0xffff8020;
						r->style.thickness = 1;
						r->v.mixed = true;
						dvv1->add_widget(r);
					}
					{
						auto r = new switch_tl();
						r->set_size({ 60,36 });
						r->set_value(true);
						r->v.mixed = true;
						dvv1->add_widget(r);
					}
				}
				dvv->add_widget(dvv1);
			}
			auto fpslab = new color_btn();
			{
				dvv2->set_size({ 500,400 });
				dvv2->set_pos({ 10,10 });
				dvv2->family = family;
				dvv2->border = { 0xF11212ac,1,5,0x50f66666 };	// 颜色，线粗，圆角，背景色
				dvv2->flex_child.margin_left = 4;		// 子元素外边距
				dvv2->flex_child.margin_right = 4;
				dvv2->flex_child.margin_top = 4;
				dvv2->flex_child.margin_bottom = 4;
				dvv2->draggable = true;
				dvv2->docking = true;
				dvv2->_absolute = true;
				dom0->add_widget(dvv2);
				{
					auto btn = fpslab;
					btn->rounding = 4;
					btn->text_align = {};
					btn->set_btn_color_bgr(fmod(2, 5));
					btn->effect = uTheme::light;
					btn->_disabled_events = true;
					dvv2->add_widget(btn);
					btn->set_size({ 492,392 });
					btn->font_size = 16;
					btn->text_color = -1;
					/*btn->str = (char*)u8"🍕透明按钮 ";*/
				}
				dvv2->mevent_cb = [=](void* p, int type, const glm::vec2& mps)
					{
						if (type == (int)event_type2::on_dragend) {
							glm::vec2 pos = dvv2->get_pos();
							glm::vec2 size = dvv2->get_size();
							glm::vec2 main_pos = app->main->get_pos();
							glm::vec2 main_size = app->main->get_size();
							if (pos.x < 0 || pos.y < 0 || pos.x + size.x >  main_size.x || pos.y + size.y >  main_size.y)
							{
								//view->set_div(dvv2);
							}
						}
					};
			}
			dom0->add_widget(dvv);
			auto colorpicker = new div_cx();
			{
				// 调色板

				auto dvv = colorpicker;
				dom0->add_widget(dvv);
				dvv->set_size({ 1000,400 });
				dvv->set_pos({ 100,60 });
				dvv->family = family;
				dvv->border = { 0xffacacac,1,5,0xf9666666 };	// 颜色，线粗，圆角，背景色
				dvv->flex.wrap = flex_wrap::WRAP;
				dvv->flex.direction = flex_direction::ROW;
				dvv->flex.justify_content = flex_align::ALIGN_START;	// x轴，主轴对齐
				dvv->flex.align_items = flex_align::ALIGN_CENTER;		// y轴，交叉轴对齐
				dvv->flex.align_content = flex_align::ALIGN_START;		// y轴，多行交叉轴对齐
				dvv->flex_child.margin_left = 2;		// 子元素外边距
				dvv->flex_child.margin_right = 2;
				dvv->flex_child.margin_top = 2;
				dvv->flex_child.margin_bottom = 2;
				dvv->draggable = true;
			}
			glm::vec4 dcc[] = { glm::vec4(1, 0, 0, 0.5),
				glm::vec4(0, 1, 0, 0.5),
				glm::vec4(0, 0, 1, 0.5) };
			for (int i = 0; i < 3; i++)
			{
				auto dvv = new div_cx();
				dom0->add_widget(dvv);
				dvv->set_size({ 100,100 });
				dvv->set_pos({ 100,60 });
				dvv->family = family;
				dvv->border = { 0xffacacac,1,5, color2u(dcc[i]) };	// 颜色，线粗，圆角，背景色
				dvv->flex.wrap = flex_wrap::WRAP;
				dvv->flex.direction = flex_direction::ROW;
				dvv->flex.justify_content = flex_align::ALIGN_START;	// x轴，主轴对齐
				dvv->flex.align_items = flex_align::ALIGN_CENTER;		// y轴，交叉轴对齐
				dvv->flex.align_content = flex_align::ALIGN_START;		// y轴，多行交叉轴对齐
				dvv->flex_child.margin_left = 2;		// 子元素外边距
				dvv->flex_child.margin_right = 2;
				dvv->flex_child.margin_top = 2;
				dvv->flex_child.margin_bottom = 2;
				dvv->draggable = true;
				dom0->add_widget(dvv);
			}
			dom0->update(0.0f);
			{
				app->e_window_cb = [=](form_x* fw, int type)
					{
						if (type == SDL_EVENT_WINDOW_MOVED)
						{
							//view->get_div(fw);
						}
					};
			}
			app->set_fps(60);
			vkd->_state.has_fence = false;
			auto actx = audiofft(app);
			hz::fft_data fft = {};
			int fft_size = 1024;
			ftd_init(&fft, fft_size, 5.0);
			//ftd_free(&fft);
			actx->pause(1);
			std::string gpustr;
			bool r3d = 0;
			c_runtime_cx rtc; // 高精度计时器
			c_runtime_cx rt_cpu; // cpu计时器
			int uims = 0, ms3d = 0, SDLms = 0, cpums = 0;
			// 运行消息循环			
			do {
				int ct = 0;
				auto delta = app->update_event();
				if (!app->form_count())break;
				actx->run_play();
				auto avd = actx->_current;
				if (avd && /*actx->has_play &&*/ avd->cpos > fft_size)
				{
					auto add = (short*)avd->data->data;
					size_t apos = (avd->ctime / avd->atime) * avd->data->total_samples;
					fft.bits_per_sample = avd->data->bits_per_sample;
					ftd_update(&fft, add + apos, fft_size * 2, 100);
					ct++;
				}
				cpums = rt_cpu.end();
				rt_cpu.begin();
				auto fps = app->get_fps();
				form0->update(delta);
				if (form0->is_minimized())
				{
					dom0->pause();
					continue;
				}
				void* dtex3d = 0;
				if (r3d)
				{
					dtex3d = tex3d;
					auto io = form0->get_io();
					// 判断鼠标是否点中控件，点中了就设置io->WantCaptureMouse = true，让3d渲染器不处理鼠标事件，交给控件处理
					if (dom0->press_test() && io)
						io->WantCaptureMouse = true;

					edit1->_color.x = colorpick->get_color();
					vkd->update(form0->io);	// 更新事件
					gpustr = vkd->get_label();
				}
				static double upt = 0.0;
				upt += delta;
				if (upt > 1.0)
				{
					upt = 0.0;
					fpslab->str = vkr::format("CPU FPS\t\t: %d\nUIcmd\t\t\t: %d ms\n3Dcmd\t\t\t: %d ms\nSDLms\t\t\t: %d ms\nCPUms\t\t\t: %d ms\n", fps, uims, ms3d, SDLms, cpums) + gpustr;
				}
				rtc.begin();
				ct += dom0->update(delta);
				uims = rtc.end();
				int64_t sem3d = 0;
				if (r3d)
				{
					rtc.begin();
					vkd->on_render();		// 渲染到fbo纹理tex3d
					if (!vkd->_state.has_fence) {
						sem3d = vkd->get_fbo_semaphore();
					}
					ms3d = rtc.end();
					ct++;
				}
				form0->add_vk_semaphores(sem3d, (int64_t)0, 0);
				if (ct) {
					rtc.begin();		// 开始计时录制SDL渲染命令
					form0->set_state();	// 清空/设置交换链接状态 
					texture_dt tdt = {};
					tdt.src_rect = { 0,0,vki.size.x,vki.size.y };
					tdt.dst_rect = { 0,0,vki.size.x,vki.size.y };
					if (dtex3d) pcb->render_texture(form0->renderer, dtex3d, &tdt, 1);//3d
					dom0->cmd_draw();
					{
						glm::vec4 color = { 0,0.5,1.0,0.8 };
						static std::vector<SDL_Vertex> vertices;
						if (fft._rects.size())
						{
							gen_rects(fft._rects, vertices, { 0.1, 1, 0.1, 0.9 }, { 1, 0.5, 0, 1 });
							SDL_RenderGeometry(form0->renderer, nullptr, vertices.data(), vertices.size(), nullptr, 0);
						}
					}
					form0->present();
					if (!r3d)
						view->wait_dev();
					SDLms = rtc.end();
				}
			} while (app->form_count());
			delete dom0;
			delete view;

		}
		free_vkdg(vkd);
		gd->release(sampler);
		gd->release(dctx);
		free_gdev(gd);
		free_app(app);
	}
	return 0;
}
