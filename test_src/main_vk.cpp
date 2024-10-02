
#include <pch1.h>
#include <vkgui/pnguo.h>
#include <vkgui/tinysdl3.h>
#include <vkgui/vkrenderer.h>

void new_ui(form_x* form0, vkdg_cx* vkd) {

	auto pl1 = new plane_cx();
	uint32_t pbc = 0xc02c2c2c;
	pl1->set_border({ 0x80ff802C,1,5,pbc });
	form0->bind(pl1);	// 绑定到窗口  
	pl1->set_rss(5);
	pl1->_lms = { 6,6 };
	auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Malgun Gothic";
	pl1->add_familys(fontn, 0);
	pl1->draggable = true; //可拖动
	pl1->set_size({ 320,600 });
	pl1->set_pos({ 1000,100 });
	pl1->on_click = [](plane_cx* p, int state, int clicks) {};
	pl1->fontsize = 16;
	glm::vec2 bs = { 150,22 };
	glm::vec2 bs1 = { 50,22 };
	std::vector<std::string> boolstr = { "TAA", "LightFrustum",	"BoundingBoxes","ShowMilliseconds" };
	std::vector<bool*> bps = { &vkd->state.bUseTAA, &vkd->state.bDrawLightFrustum, &vkd->state.bDrawBoundingBoxes, &vkd->state.bShowMilliseconds };
	for (size_t i = 0; i < boolstr.size(); i++)
	{
		auto& it = boolstr[i];
		auto kcb = pl1->add_label(it.c_str(), bs, 0);
		{
			kcb->_disabled_events = true;
			kcb->text_color = 0xff7373ff;
			auto sw1 = (switch_tl*)pl1->add_switch(bs1, it.c_str(), *(bps[i]));
			sw1->get_pos();
			sw1->bind_ptr(bps[i]);
		}
	}
	static std::vector<color_btn*> lbs;
	bs.x = 300;
	for (size_t i = 0; i < 18; i++)
	{
		auto kcb = pl1->add_label("", bs, 0);
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
#include <random>
#include <vkgui/win_core.h>
#include "mshell.h"
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
	const char* wtitle = (char*)u8"窗口1";
	form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, 0);
	auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
	vkdg_cx* vkd = new_vkdg(&sdldev);	// 创建vk渲染器 
	SDL_Texture* d3tex = 0;
	//SetWindowDisplayAffinity((HWND)form0->get_nptr(), WDA_MONITOR);
	if (vkd) {
		{
			vkd->load_gltf(R"(E:\model\sharp2.glb)", {}, 1.0);// 加载gltf
			vkd->load_gltf(R"(E:\model\realistic_palm_tree_10_free.glb)", { 2,0,0 }, 1.0);
			vkd->load_gltf(R"(E:\model\bc22.glb)", { 10,0,50 }, 1.0);
			vkd->load_gltf(R"(E:\app\tools\pnguo\out\bin\media\Cauldron-Media\buster_drone\busterDrone.gltf)", { 12,1,1 }, 1.0);
			vkd->load_gltf(R"(E:\model\lets_go_to_the_beach_-_beach_themed_diorama.glb)", { 0,0,20 }, 1.0);
			//load_gltf(vkd, R"(E:\model\hero_alice_lobby.glb)");
			//load_gltf(vkd, R"(E:\model\spaceship.glb)");
			vkd->load_gltf(R"(E:\model\helicopter_space_ship.glb)", { 5,6,-8 }, 1.0);
			//load_gltf(vkd, R"(E:\model\maple_trees.glb)");
			//load_gltf(vkd, R"(E:\model\psx_houses.glb)");
			//load_gltf(vkd, R"(E:\model\psx_old_house.glb)");
			//load_gltf(vkd, R"(E:\model\space_station_4.glb)");
			//load_gltf(vkd, R"(E:\model\sexy_guardian_woman_model_18.glb)");
			//load_gltf(vkd, R"(E:\code\hub\cpp\vulkanFrame\vulkanFrame\DamagedHelmet.glb)");
			//load_gltf(vkd, R"(E:\model\DragonAttenuation.glb)");
		}
		vkd->resize(1024, 800);				// 设置fbo缓冲区大小
		auto vr = vkd->get_vkimage(0);	// 获取fbo纹理弄到窗口显示
		auto texok = form0->add_vkimage(vr.size, vr.vkimageptr, { 20,20 }, 1);// 创建SDL的bgra纹理 
		vkd->state.SelectedTonemapperIndex = 1;
		vkd->state.Exposure = 0.1;
		vkd->state.EmissiveFactor = 250;
		new_ui(form0, vkd);
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
	run_app(app, 0);
	free_app(app);
	return 0;
}
