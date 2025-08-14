
#include <pch1.h>

#include <random>
#include <pnguo/win_core.h>
#include "win32msg.h"
#include <pnguo/event.h>
#define GUI_STATIC_LIB
#include <pnguo/pnguo.h>
#include <pnguo/tinysdl3.h>
#include <pnguo/vkrenderer.h>
#include <pnguo/page.h>
#include <pnguo/mapView.h>

#include <cairo/cairo.h>

#include "mshell.h"
//#include <cgltf.h>

auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman";// , Malgun Gothic";

void new_ui(form_x* form0, vkdg_cx* vkd) {
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
		std::vector<bool*> bps = { &vkd->_state.bUseTAA, &vkd->_state.bDrawLightFrustum, &vkd->_state.bDrawBoundingBoxes, &vkd->_state.bShowMilliseconds };
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
				txt = it + ": " + cpuinfo.name;
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
#include <entt/entt.hpp>
struct node_t
{
	int16_t type, count;//种类、数量
};
struct belt_t
{
	node_t* p = 0;	// 节点
	size_t n = 0;	// 节点数量
	glm::ivec2 pos[3] = {};// 曲线
	belt_t* link_ptr = 0;	// 尾部连接
	size_t link_idx = 0;		// 连接位置
};
// 传送带
class belt_cx
{
public:
	std::vector<entt::entity> lines;
	glm::vec2 pos = { 0.5,0 };
	int w = 56;
	uint32_t linecolor = 0xcc222222;
	std::vector<glm::vec2> bline;
	std::vector<int> bline_idx;
	int dxx = 0;
	double ts = 0.0;
public:
	belt_cx();
	~belt_cx();

	void update(float delta);
	void draw(cairo_t* cr);
private:

};

belt_cx::belt_cx()
{
	int n = w * 10 / 6.0; size_t nc = 0;
	int nx = w / 6;
	int xx = 0;

	{
		for (size_t i = 0; i < n; i++, nc++)
		{
			bline.push_back({ xx + i * nx,1 });
			bline.push_back({ xx + i * nx,w - 2 });
			bline_idx.push_back(i + nc);
			bline_idx.push_back(i + nc + 1);
			bline_idx.push_back(-1);
		}
	}

}

belt_cx::~belt_cx()
{
}
void belt_cx::update(float delta) {
	ts += delta;
	if (ts > 0.12 * 0.5)
	{
		dxx++;
		ts = 0;
	}
	if (dxx > 7)dxx = 0;
}
void belt_cx::draw(cairo_t* cr) {

	for (size_t i = 0; i < 10; i++)
	{
		draw_rect(cr, { i * w,0,w,w }, 0xf0cccccc, 0xffff802C, 2, 1);
	}
	auto ps = pos;
	ps.x += dxx;
	// 裁剪区域
	cairo_rectangle(cr, 0, 0, 10 * w, w);
	cairo_clip(cr);
	draw_polylines(cr, ps, bline.data(), bline.size(), bline_idx.data(), bline_idx.size(), linecolor, 1);
}

void show_belt(form_x* form0)
{
	if (!form0)return;
	glm::ivec2 size = { 1024,800 };
	glm::ivec2 cview = { 10240,10240 };
	auto p = new plane_cx();
	uint32_t pbc = 0xf02c2c2c;
	p->set_color({ 0x80ff802C,1,5,pbc });
	form0->bind(p);	// 绑定到窗口  
	p->set_rss(5);
	p->_lms = { 8,8 };
	p->add_familys(fontn, 0);
	p->draggable = false; //可拖动
	p->set_size(size);
	p->set_pos({ 30,50 });
	p->on_click = [](plane_ev* e) {};
	p->fontsize = 16;

	size.y -= 50;
	size.x -= 50 * 10;
	size /= 2;

	auto dpx = p->push_dragpos(size);// 增加一个拖动坐标

	auto bp = new belt_cx();
	p->update_cb = [=](float delta)
		{

			bp->update(delta);
			return 1;
		};
	p->draw_back_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
			auto dps = p->get_dragpos(dpx);//获取拖动时的坐标
			cairo_translate(cr, dps.x, dps.y);
			{
				cairo_as _ss_(cr);
				bp->draw(cr);
			}
			std::string str = (char*)u8"🍉";
			glm::vec4 rc = { 0,0,200,56 };
			text_style_t st = {};
			st.font = 0;
			st.text_align = { 0.0,0.5 };
			st.font_size = 39;
			st.text_color = -1;
			auto rc1 = p->ltx->get_text_rect(st.font, st.font_size, str.c_str(), -1);
			auto rc2 = rc;
			//rc2.z = rc1.x; rc2.w = rc1.y;
			//rc2.y = (rc.w - rc1.y) * st.text_align.y;
			//draw_rect(cr, rc2, 0xf0222222, 0xff802Cff, 2, 1);
			draw_text(cr, p->ltx, str.c_str(), -1, rc, &st);
		};
}


void test_img() {

	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr.png", 0);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr10.jpg", 10);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr30.jpg", 30);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr60.jpg", 60);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr80.jpg", 80);
}
glm::vec4 p2v(float yaw, float pitch)
{
	return glm::vec4(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch), 0);
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

	auto rd = hz::shared_load(R"(E:\Program Files\RenderDoc_1.37_64\renderdoc.dll)");

	{;

#if 0

		glm::quat qk = glm::quat(1, 0, 0, 0);
		static constexpr float AMD_PI = 3.1415926535897932384626433832795f;
		static constexpr float AMD_PI_OVER_2 = 1.5707963267948966192313216916398f;
		static constexpr float AMD_PI_OVER_4 = 0.78539816339744830961566084581988f;
		auto a = glm::radians(75.0f);
		auto ak = AMD_PI_OVER_4;
		auto ac = cosf(a);
		auto ac0 = cosf(ak);
		printf("\n")
		vkr::Transform transform = {};
		transform.LookAt(p2v(AMD_PI_OVER_2, 0.58f) * 3.5f, glm::vec4(0, 0, 0, 0), false);
		vkr::light_t l = {};
		l._type = vkr::light_t::LIGHT_SPOTLIGHT;
		l._intensity = 50.0;
		l._color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		l._range = 15.0;
		l._outerConeAngle = AMD_PI_OVER_4;// glm::radians(20.0);
		l._innerConeAngle = AMD_PI_OVER_4 * 0.9f;
		glm::mat4 lightMat = transform.GetWorldMat();
		glm::mat4 lightView = glm::affineInverse(lightMat);
		glm::mat4 mLightViewProj;
		glm::mat4 per = glm::perspective(l._outerConeAngle * 2.0f, 1.0f, .1f, 100.0f);
		auto mLightView = lightView;
		if (l._type == vkr::light_t::LIGHT_SPOTLIGHT)
			mLightViewProj = per * lightView;
		//transpose
		auto direction = glm::transpose(lightView) * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		auto position = lightMat[3];
		auto outerConeCos = cosf(l._outerConeAngle);
		auto innerConeCos = cosf(l._innerConeAngle);

		glm::vec4 vCenter = glm::vec4(0.0f, 0.0f, 0.5f, 0.0f);
		glm::vec4 vRadius = glm::vec4(1.0f, 1.0f, 0.5f, 0.0f);
		glm::vec4 vColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glm::mat4 spotlightMatrix = glm::inverse(mLightViewProj);

		printf("Spotlight Matrix:\n");
#endif
	}



	//{
	//	hz::mfile_t m;
	//	auto pd = m.open_d(R"(E:\zt\c00951ea.spr)", 0);
	//	std::vector<int> fv;
	//	if (pd)
	//	{
	//		fv.resize(20);
	//		memcpy(fv.data(), pd, 20 * 4);
	//	}
	//}
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
	auto kd = sdldev.vkdev;
	//sdldev.vkdev = 0;	// 清空使用独立创建逻辑设备
	std::vector<device_info_t> devs = get_devices(sdldev.inst); // 获取设备名称列表
	bool grab_enable = false;	// 设置鼠标范围在窗口内
	bool rmode = true;			// 设置窗口的相对鼠标模式。
	//form0->set_mouse_mode(grab_enable, rmode);
	get_queue_info(sdldev.phy);

	vkdg_cx* vkd = new_vkdg(sdldev.inst, sdldev.phy, kd);	// 创建vk渲染器 
	//vkdg_cx* vkd1 = new_vkdg(&sdldev);	// 创建vk渲染器  
	//SetWindowDisplayAffinity((HWND)form0->get_nptr(), WDA_MONITOR);// 反截图
	//if (vkd1) {
	//	vkd1->add_gltf(R"(E:\model\helicopter_space_ship.glb)", { 15,0,8 }, 1.0);
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
			int kadf[] = { sizeof(std::string),sizeof(std::vector<char>) };
			//vkd->add_gltf(R"(E:\model\sharp2.glb)", { 0,0,0 }, 1.0);// 地板
			//vkd->add_gltf(R"(E:\model\zw\fantasy_church_ruins.glb)", { -5,0,-6 }, 1.0);// 加载gltf
			//vkd->add_gltf(R"(E:\model\zw\autumnal_forest.glb)", { -15,0,-6 }, 1.0);// 加载gltf
			//vkd->add_gltf(R"(E:\model\realistic_palm_tree_10_free.glb)", { 2,0,0 }, 1.0);
			//vkd->add_gltf(R"(E:\model\bc22.glb)", { 0,0,5 }, 0.52);
			//vkd->add_gltf(R"(E:\vsz\h\avl\av\Bee.glb)", { 0,0,0 }, 10.0); 
			//vkd->add_gltf(R"(E:\code\c\assimp\test\models\glTF2\textureTransform\TextureTransformTest.gltf)", { 0,0,0 }, 1.0);
			vkd->add_gltf(R"(E:\ag\glTFSample\media\Cauldron-Media\buster_drone\busterDrone.gltf)", { 0,0.8 * 0,0 }, 1.0);
			//vkd->add_gltf(R"(E:\model\lets_go_to_the_beach_-_beach_themed_diorama.glb)", { 0,0,20 }, 1.0);
			//vkd->add_gltf( R"(E:\model\hero_alice_lobby.glb)");
			//vkd->add_gltf(R"(E:\model\pale_radiance_tree.glb)", { }, 1.0);
			//vkd->add_gltf(R"(E:\model\ka-2000__scx2800-2_cranes (1).glb)", { 5,0,-8 }, 1.0);
			//vkd->add_gltf(R"(E:\model\maple_trees.glb)", { 20,0,10 }, 0.10);

			//vkd->add_gltf(R"(E:\model\rock_monster.glb)", { 5,0,10 }, 0.5);
			/*vkd->add_gltf(R"(E:\model\helicopter_space_ship.glb)", {}, 1.0);
			vkd->add_gltf(R"(E:\zmodel\cr123.glb)", { 0,0,10 }, 10.0);
			vkd->add_gltf(R"(E:\model\spaceship.glb)", { 0 * 5,10,-8 * 0 }, 1.0);*/

			//vkd->add_gltf(R"(E:\zmodel\glTF-Sample-Models-main\2.0\ClearcoatRing\glTF\ClearcoatRing.gltf)", {  }, 1.0);
			//vkd->add_gltf(R"(E:\zmodel\glTF-Sample-Models-main\2.0\MorphStressTest\glTF-Binary\MorphStressTest.glb)", {  }, 1.0);
			//vkd->add_gltf(R"(E:\zmodel\MorphStressTest.glb)", { }, 1.0);
			//vkd->add_gltf(R"(E:\model\psx_houses.glb)", { 15,0,-8 }, 1.0);
			//vkd->add_gltf(R"(E:\model\psx_old_house.glb)", { 0 * 5,0,-8 * 0 }, 1.0);
			//vkd->add_gltf(R"(E:\model\o-tech_reaper-4k-materialtest.glb)", { 5,10,-8 }, 10.0);
			//vkd->add_gltf(R"(E:\zmodel\sofa.glb)", { 0,0,0 }, 1.0);
			//vkd->add_gltf(R"(E:\tx\parlahti.glb)", { 0,0,0 }, 1.0);
			//vkd->add_gltf(R"(E:\model\black_hole.glb)", { 0,0,0 }, 0.0010);

			//vkd->add_gltf( R"(E:\model\space_station_4.glb)");
			//vkd->add_gltf( R"(E:\model\sexy_guardian_woman_model_18.glb)");
			//vkd->add_gltf( R"(E:\code\hub\cpp\vulkanFrame\vulkanFrame\DamagedHelmet.glb)");
			//vkd->add_gltf( R"(E:\model\DragonAttenuation.glb)", { 0,0,0 }, 1.0);
		}
		vkd->resize(1024, 800);				// 设置fbo缓冲区大小
		auto vr = vkd->get_vkimage(0);	// 获取fbo纹理弄到窗口显示 nullptr;//
		auto texok = form0->add_vkimage(vr.size, vr.vkimage, { 20,36 }, 1);// 创建SDL的bgra纹理 
		/*
		case 0: return AMDTonemapper(color);
		case 1: return DX11DSK(color);
		case 2: return Reinhard(color);
		case 3: return Uncharted2Tonemap(color);
		case 4: return tonemapACES( color );
		case 5: return color;
		*/
		vkd->_state.SelectedTonemapperIndex = 0;
		vkd->_state.Exposure = 1.0;
		vkd->_state.EmissiveFactor = 30;
		vkd->_state.IBLFactor = 1.0;
		new_ui(form0, vkd);
		if (texok)
		{
			form0->up_cb = [=](float delta, int* ret)
				{
					auto light = vkd->get_light(0);
					vkd->_state.SelectedTonemapperIndex;	// 0-5: Tonemapper算法选择
					vkd->_state.Exposure;				// 曝光度：默认1.0
					vkd->_state.bUseTAA;
					vkd->update(form0->io);	// 更新事件
					vkd->on_render();		// 执行渲染
					static bool sa = false;
					if (sa)
					{
						auto img = vkd->save_shadow(0);	// 保存阴影贴图
						auto pf = (float*)img.data;
						for (size_t i = 0; i < img.size.x * img.size.y; i++)
						{
							img.data[i] = gray_float_to_rgba(*pf);
							pf++;
						}
						stbi_write_png("temp/shadow.png", img.size.x, img.size.y, 4, img.data, img.size.x * 4);
						sa = false;
					}
				};
		}

	}

	//show_belt(form0);
	//show_cpuinfo(form0);
	// 运行消息循环
	run_app(app, 0);
	free_app(app);
#ifdef _WIN32
	ExitProcess(0);
#endif
	return 0;
}
