﻿
#include <pch1.h>

#include <random>
#include <vkgui/win_core.h>
#include "win32msg.h"
#include <vkgui/event.h>
#include <vkgui/pnguo.h>
#include <vkgui/tinysdl3.h>
#include <vkgui/vkrenderer.h>
#include <vkgui/page.h>

#include "mshell.h"


auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Malgun Gothic";

void menu_m(form_x* form0)
{
	auto mainmenu = new plane_cx();
	form0->bind(mainmenu, 1);	// 绑定主菜单到窗口
	auto p = mainmenu;
	//p->set_rss(5);
	p->add_familys(fontn, 0);
	//p->draggable = true; //可拖动✔
	p->set_color({ 0,1,0,0xff000000 });
	// 主菜单
	std::vector<std::string> mvs = { (char*)u8"文件",(char*)u8"编辑",(char*)u8"视图",(char*)u8"工具",(char*)u8"帮助" };
	p->fontsize = 16;

	{
		glm::ivec2  fs = form0->get_size();
		if (fs.x & 1)
			fs.x++;
		if (fs.y & 1)
			fs.y++;
		p->set_size({ fs.x, p->fontsize * 2 });
	}
	p->set_pos({});
	glm::vec2 cs = { 1500,1600 };
	auto vs = p->get_size();

	p->set_view(vs, cs);
	p->update_cb = [=](float dt)
		{
			bool r = false;
			if (form0)
			{
				glm::ivec2 ps = p->get_size(), fs = form0->get_size();
				if (fs.x & 1)
					fs.x++;
				if (fs.y & 1)
					fs.y++;
				if (ps.x != fs.x)
				{
					p->set_size({ fs.x, p->fontsize * 2 });
					r = true;
				}
			}
			return r;
		};

	menu_cx* mc = new menu_cx();	// 菜单管理
	mc->set_main(form0);
	mc->add_familys(fontn);

	std::vector<std::string> mvs0 = { (char*)u8"🍇测试菜单1g",(char*)u8"🍑菜单",(char*)u8"🍍菜单1" };
	std::vector<std::string> mvs1 = { (char*)u8"🍇子菜单",(char*)u8"🍑菜单2",(char*)u8"🍍菜单12" };
	int cidx = 1;
	// 创建菜单
	auto pm31 = mc->new_menu(-1, 30, mvs1, [=](mitem_t* pm, int type, int idx)
		{
			if (type == 1)
			{
				printf("click:%d\t%d\n", type, idx);
			}
			//else
			//	printf("move:%d\t%d\n", type, idx);
		});
	auto pm3 = mc->new_menu(-1, 30, mvs0, [=](mitem_t* pm, int type, int idx)
		{
			if (type == 1)
			{
				printf("click:%d\t%d\n", type, idx);
			}
			else
			{
				//printf("move:%d\t%d\n", type, idx);
			}
		});
	pm3->set_child(pm31, 2);
	//pm3->show({ 100,100 }); // 显示菜单

	for (auto& it : mvs)
	{
		auto cbt = p->add_cbutton(it.c_str(), { 60,26 }, (int)uType::info);
		cbt->effect = uTheme::light;
		cbt->hscroll = {};
		cbt->rounding = 0;
		cbt->light = 0.1;

		cbt->click_cb = [=](void* ptr, int clicks)
			{
				printf("%s\n", cbt->str.c_str());
			};
		cbt->mevent_cb = [=](void* pt, int type, const glm::vec2& mps)
			{
				static void* enterst = 0;
				auto cp = (color_btn*)pt;
				auto t = (event_type2)type;
				switch (t)
				{
				case event_type2::on_down:
				{
					auto cps = cp->get_pos();
					cps.y += cp->size.y + cp->thickness;
					pm3->hide(true);
					pm3->show(cps);
					hide_tooltip(form0);
					form0->uptr = 0;
				}
				break;
				case event_type2::on_enter:
				{
					enterst = pt;
					if (pm3->get_visible()) {
						auto cps = cp->get_pos();
						cps.y += cp->size.y + cp->thickness;
						pm3->hide(true);
						pm3->show(cps);
					}
				}
				break;
				case event_type2::on_hover:
				{
					// 0.5秒触发悬停事件
					style_tooltip stp = {};
					stp.family = fontn;
					stp.fonst_size = 14;
					glm::vec2 cps = mps;
					cps.y += 20;
					if (enterst == pt) {
						if (form0->uptr != pt)
						{
							//show_tooltip(form0, (char*)u8"提示信息！", cps, &stp);
							form0->uptr = pt;
						}
					}
				}
				break;
				case event_type2::on_leave:
				{
					if (enterst == pt) {
						hide_tooltip(form0);
						form0->uptr = 0;
					}
				}
				break;
				default:
					break;
				}
			};
	}
}

void new_ui(form_x* form0, vkdg_cx* vkd) {
	menu_m(form0);
	auto p = new plane_cx();
	uint32_t pbc = 0xc02c2c2c;
	p->set_color({ 0x80ff802C,1,5,pbc });
	form0->bind(p);	// 绑定到窗口  
	p->set_rss(5);
	p->_lms = { 6,6 };
	p->add_familys(fontn, 0);
	p->draggable = true; //可拖动
	p->set_size({ 320,660 });
	p->set_pos({ 1000,100 });
	p->on_click = [](plane_cx* p, int state, int clicks) {};
	p->fontsize = 16;
	int width = 16;
	int rcw = 14;
	{
		// 设置带滚动条
		p->set_scroll(width, rcw, { 0, 0 }, { 2,0 }, { 0,2 });
		p->set_scroll_hide(1);
	}
	p->set_view({ 320,660 }, { 600, 660 });
	glm::vec2 bs = { 150,22 };
	glm::vec2 bs1 = { 50,22 };
	std::vector<std::string> boolstr = { "TAA", "LightFrustum",	"BoundingBoxes","ShowMilliseconds" };
	if (vkd)
	{
		std::vector<bool*> bps = { &vkd->state.bUseTAA, &vkd->state.bDrawLightFrustum, &vkd->state.bDrawBoundingBoxes, &vkd->state.bShowMilliseconds };
		for (size_t i = 0; i < boolstr.size(); i++)
		{
			auto& it = boolstr[i];
			auto kcb = p->add_label(it.c_str(), bs, 0);
			{
				kcb->_disabled_events = true;
				kcb->text_color = 0xff7373ff;
				auto sw1 = (switch_tl*)p->add_switch(bs1, it.c_str(), *(bps[i]));
				//sw1->_disabled_events = true;
				sw1->get_pos();
				sw1->bind_ptr(bps[i]);
			}
		}
		std::vector<color_btn*>* pbs = new std::vector<color_btn*>();
		bs.x = 300;
		auto& lbs = *pbs;
		for (size_t i = 0; i < 18; i++)
		{
			auto kcb = p->add_label("", bs, 0);
			kcb->text_color = 0xff80F61F;
			kcb->_disabled_events = true;
			lbs.push_back(kcb);
		}
		vkd->set_label_cb([=](int count, int idx, const char* str)
			{
				if (idx < 0) {
					for (size_t i = 0; i < 18; i++)
						lbs[i]->str.clear();
				}
				else
				{
					lbs[idx]->str = str;
				}
			});
	}


	uint32_t* cc = get_wcolor() + 8;
	for (int i = 0; i < 0 * 8; i++) {
		auto p = new plane_cx();
		uint32_t tc = cc[i];
		p->set_color({ 0x80ff802C,1,5,0xff2c2c2c });
		form0->bind(p);	// 绑定到窗口  
		p->set_rss(5);
		p->_lms = { 6,6 };
		p->add_familys(fontn, 0);
		p->draggable = true; //可拖动
		p->set_size({ 500,300 });
		p->set_pos({ 100 + i * 20,100 + i * 20 });
		for (size_t j = 0; j < boolstr.size(); j++)
		{
			auto& it = boolstr[j];
			auto kcb = p->add_label(it.c_str(), bs, 0);
			{
				kcb->_disabled_events = true;
				kcb->text_color = tc;
				auto sw1 = (switch_tl*)p->add_switch(bs1, it.c_str(), 0);
				sw1->get_pos();
				//sw1->bind_ptr(bps[j]);
			}
		}
	}
}

#ifdef _WIN32

static std::string GetCPUNameString()
{
	int nIDs = 0;
	int nExIDs = 0;

	char strCPUName[0x40] = { };

	std::array<int, 4> cpuInfo;
	std::vector<std::array<int, 4>> extData;

	__cpuid(cpuInfo.data(), 0);

	// Calling __cpuid with 0x80000000 as the function_id argument
	// gets the number of the highest valid extended ID.
	__cpuid(cpuInfo.data(), 0x80000000);

	nExIDs = cpuInfo[0];
	for (int i = 0x80000000; i <= nExIDs; ++i)
	{
		__cpuidex(cpuInfo.data(), i, 0);
		extData.push_back(cpuInfo);
	}

	// Interpret CPU strCPUName string if reported
	if (nExIDs >= 0x80000004)
	{
		memcpy(strCPUName, extData[2].data(), sizeof(cpuInfo));
		memcpy(strCPUName + 16, extData[3].data(), sizeof(cpuInfo));
		memcpy(strCPUName + 32, extData[4].data(), sizeof(cpuInfo));
	}

	return strlen(strCPUName) != 0 ? strCPUName : "UNAVAILABLE";
}
#else
static std::string GetCPUNameString()
{
	return "UNAVAILABLE";
}

#endif
void show_cpuinfo(form_x* form0)
{
	cpuinfo_t cpuinfo = get_cpuinfo();
	if (!form0)return;
	auto p = new plane_cx();
	uint32_t pbc = 0xf02c2c2c;
	p->set_color({ 0x80ff802C,1,5,pbc });
	form0->bind(p);	// 绑定到窗口  
	p->set_rss(5);
	p->_lms = { 8,8 };
	p->add_familys(fontn, 0);
	p->draggable = true; //可拖动
	p->set_size({ 420,660 });
	p->set_pos({ 100,100 });
	p->on_click = [](plane_cx* p, int state, int clicks) {};
	p->fontsize = 16;
	int width = 16;
	int rcw = 14;
	{
		// 设置带滚动条
		p->set_scroll(width, rcw, { 0, 0 }, { 2,0 }, { 0,2 });
		p->set_scroll_hide(1);
	}
	// 视图大小，内容大小
	p->set_view({ 420,660 }, { 420, 660 });
	glm::vec2 bs = { 150,22 };
	glm::vec2 bs1 = { 50,22 };
	std::vector<std::string> ncsstr = { (char*)u8"CPU信息","NumLogicalCPUCores","CPUCacheLineSize","SystemRAM","SIMDAlignment" };
	std::vector<std::string> boolstr = { "AltiVec","MMX","SSE","SSE2","SSE3","SSE41","SSE42","AVX","AVX2","AVX512F","ARMSIMD","NEON","LSX","LASX" };

	static std::vector<color_btn*> lbs;
	bs.x = 300;
	uint32_t txtcolor = 0xfff2f2f2;// 0xff7373ff;
	int64_t ds[] = { 0, cpuinfo.NumLogicalCPUCores,cpuinfo.CPUCacheLineSize,cpuinfo.SystemRAM ,cpuinfo.SIMDAlignment };
	{
		int i = 0;
		for (auto& it : ncsstr)
		{
			std::string txt = vkr::format("%-20s: %lld", it.c_str(), ds[i]);
			auto tc = txtcolor;
			if (i == 0) {
				txt = it + ": " + GetCPUNameString();
				tc = 0xffF6801F;
			}
			i++;
			auto kcb = p->add_label(txt, bs, 0);
			kcb->text_color = tc;
			kcb->_disabled_events = true;
			lbs.push_back(kcb);
		}
	}
	bool* bps = &cpuinfo.AltiVec;
	bs.x = 160;
	for (size_t i = 0; i < boolstr.size(); i++)
	{
		auto& it = boolstr[i];
		auto kcb = p->add_label(it.c_str(), bs, 0);
		{
			kcb->_disabled_events = true;
			kcb->text_color = txtcolor;
			auto sw1 = (switch_tl*)p->add_switch(bs1, it.c_str(), bps[i]);
			sw1->_disabled_events = true;
			sw1->get_pos();
			kcb = p->add_label("", bs, 0);
			kcb->_disabled_events = true;
		}
	}
}



void test_img() {

	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr.png", 0);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr10.jpg", 10);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr30.jpg", 30);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr60.jpg", 60);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr80.jpg", 80);
}

int main()
{
#ifdef _DEBUG
	system("rd /s /q E:\\temcpp\\SymbolCache\\tcmp.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\vkcmp.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\cedit.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\p86.pdb");
#endif 
	//hz::main_ssh2();

	//return 0;
	//test_img();
	auto app = new_app();
	uint32_t* cc = get_wcolor();
	for (size_t i = 0; i < 16; i++)
	{
		auto str = get_wcname(i, 0);
		printf("\x1b[01;3%dm%s\x1b[0m\n", (int)i % 8, str);
	}
	//testnjson();
	glm::ivec2 ws = { 1280,860 };
	const char* wtitle = (char*)u8"窗口0";
	const char* wtitle1 = (char*)u8"窗口1";

	form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable /*| ef_borderless*/ | ef_transparent);
	//form_x* form1 = (form_x*)new_form(app, wtitle1, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable);

	auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
	vkdg_cx* vkd = new_vkdg(&sdldev);	// 创建vk渲染器 
	//vkdg_cx* vkd1 = new_vkdg(&sdldev);	// 创建vk渲染器  
	//SetWindowDisplayAffinity((HWND)form0->get_nptr(), WDA_MONITOR);// 反截图
	//if (vkd1) {
	//	vkd1->load_gltf(R"(E:\model\helicopter_space_ship.glb)", { 15,0,8 }, 1.0);
	//	vkd1->resize(1024, 800);				// 设置fbo缓冲区大小
	//	auto vr = vkd1->get_vkimage(0);	// 获取fbo纹理弄到窗口显示 nullptr;//
	//	auto texok = form1->add_vkimage(vr.size, vr.vkimageptr, { 20,36 }, 1);// 创建SDL的bgra纹理 
	//	//vkd->state.SelectedTonemapperIndex = 1;
	//	vkd1->state.Exposure = 0.9928;
	//	vkd1->state.EmissiveFactor = 250;
	//	new_ui(form1, vkd1);
	//	if (texok)
	//	{
	//		form1->up_cb = [=](float delta, int* ret)
	//			{
	//				auto light = vkd1->get_light(0);
	//				vkd1->state.SelectedTonemapperIndex;	// 0-5: Tonemapper算法选择
	//				vkd1->state.Exposure;				// 曝光度：默认1.0
	//				vkd1->state.bUseTAA;
	//				vkd1->update(form1->io);	// 更新事件
	//				vkd1->on_render();		// 执行渲染
	//			};
	//	}
	//}
	if (vkd) {
		{
			//vkd->load_gltf(R"(E:\model\sharp2.glb)", {}, 1.0);// 加载gltf
			//vkd->load_gltf(R"(E:\model\realistic_palm_tree_10_free.glb)", { 2,0,0 }, 1.0);
			//vkd->load_gltf(R"(E:\model\bc22.glb)", { 10,0,50 }, 1.0);
			//vkd->load_gltf(R"(E:\app\tools\pnguo\out\bin\media\Cauldron-Media\buster_drone\busterDrone.gltf)", { 12,1,1 }, 1.0);
			//vkd->load_gltf(R"(E:\model\lets_go_to_the_beach_-_beach_themed_diorama.glb)", { 0,0,20 }, 1.0);
			//vkd->load_gltf( R"(E:\model\hero_alice_lobby.glb)");
			//vkd->load_gltf(R"(E:\model\pale_radiance_tree.glb)", { 15,0,-8 }, 1.0);
			//vkd->load_gltf(R"(E:\model\ka-2000__scx2800-2_cranes (1).glb)", { 5,0,-8 }, 1.0);
			//load_gltf(vkd, R"(E:\model\maple_trees.glb)");

			//vkd->load_gltf(R"(E:\model\rock_monster.glb)", { 5,0,10 }, 0.5);
			vkd->load_gltf(R"(E:\model\helicopter_space_ship.glb)", { 5,5,8 }, 1.0);
			vkd->load_gltf(R"(E:\model\psx_houses.glb)", { 15,0,-8 }, 1.0);
			vkd->load_gltf(R"(E:\model\psx_old_house.glb)", { 0 * 5,0,-8 * 0 }, 1.0);
			vkd->load_gltf(R"(E:\model\spaceship.glb)", { 0 * 5,10,-8 * 0 }, 1.0);

			//vkd->load_gltf( R"(E:\model\space_station_4.glb)");
			//vkd->load_gltf( R"(E:\model\sexy_guardian_woman_model_18.glb)");
			//vkd->load_gltf( R"(E:\code\hub\cpp\vulkanFrame\vulkanFrame\DamagedHelmet.glb)");
			//vkd->load_gltf( R"(E:\model\DragonAttenuation.glb)");
		}
		vkd->resize(1024, 800);				// 设置fbo缓冲区大小
		auto vr = vkd->get_vkimage(0);	// 获取fbo纹理弄到窗口显示 nullptr;//
		auto texok = form0->add_vkimage(vr.size, vr.vkimageptr, { 20,36 }, 1);// 创建SDL的bgra纹理 
		//vkd->state.SelectedTonemapperIndex = 1;
		vkd->state.Exposure = 0.9928;
		vkd->state.EmissiveFactor = 250;
		new_ui(form0, vkd);
		if (texok)
		{
			form0->up_cb = [=](float delta, int* ret)
				{
					auto light = vkd->get_light(0);
					vkd->state.SelectedTonemapperIndex;	// 0-5: Tonemapper算法选择
					vkd->state.Exposure;				// 曝光度：默认1.0
					vkd->state.bUseTAA;
					vkd->update(form0->io);	// 更新事件
					vkd->on_render();		// 执行渲染
				};
		}

	}
	else
	{
		new_ui(form0, 0);
	}
	show_cpuinfo(form0);
	// 运行消息循环
	run_app(app, 0);
	free_app(app);
	return 0;
}
