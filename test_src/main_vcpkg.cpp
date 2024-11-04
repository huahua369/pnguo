
#include <pch1.h>

#include <random>
#include <vkgui/win_core.h>
#include "win32msg.h"
#include <vkgui/event.h>
#include <vkgui/pnguo.h>
#include <vkgui/tinysdl3.h>
#include <vkgui/vkrenderer.h>
#include <vkgui/page.h>
#include <vkgui/mapView.h>

#include <cairo/cairo.h>

#include "mshell.h"


auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman";// , Malgun Gothic";

class vcpkg_Cx
{
public:
	std::queue<std::string> cmds;
	std::mutex _lock;
	std::string rootdir;// vcpkg根目录
public:
	vcpkg_Cx();
	~vcpkg_Cx();
	void do_clone(const std::string& dir);
	void do_bootstrap();
	// 拉取
	void do_pull();
	// 比较更新了哪些库
	void do_update(const std::string& t);
	// 搜索库
	void do_search();
	// 列出安装库
	void do_list();
	// 集成到全局
	void do_integrate_i();
	// 移除全局
	void do_integrate_r();
	// 查看支持的架构
	void do_get_triplet();
	// 安装库
	void do_install(const std::string& t, const std::string& triplet);
	// 删除库
	void do_remove(const std::string& t, const std::string& triplet);
	// 执行自定义命令
	void push_cmd(const std::string& c);
	// 执行命令
	void do_cmd();
private:

};
vcpkg_Cx::vcpkg_Cx() {}
vcpkg_Cx::~vcpkg_Cx() {}
void vcpkg_Cx::do_clone(const std::string& dir)
{
	if (dir.size() < 2)return;
	std::lock_guard<std::mutex> lock(_lock);
	//cmds.push(dir.substr(0, 2));
	//cmds.push("cd \"" + dir + "\"");
	rootdir = dir;
	cmds.push("git clone https://github.com/microsoft/vcpkg.git");
}
void vcpkg_Cx::do_bootstrap()
{
	if (rootdir.size() < 2)return;
	std::lock_guard<std::mutex> lock(_lock);
	//cmds.push(rootdir.substr(0, 2));
	//cmds.push("cd \"" + rootdir + "\"");
#ifdef _WIN32
	cmds.push("bootstrap-vcpkg.bat");
#else
	cmds.push("bootstrap-vcpkg.sh");
#endif // _WIN32

}
void vcpkg_Cx::do_pull()
{
	if (rootdir.size() < 2)return;
	std::lock_guard<std::mutex> lock(_lock);
	//cmds.push(rootdir.substr(0, 2));
	//cmds.push("cd \"" + rootdir + "\"");
	cmds.push("git pull");
}
void vcpkg_Cx::do_update(const std::string& t)
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg update");
}
void vcpkg_Cx::do_search()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg search --x-full-desc --x-json");
}
void vcpkg_Cx::do_list()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg list --x-full-desc --x-json");

}

void vcpkg_Cx::do_integrate_i()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg integrate install");
}

void vcpkg_Cx::do_integrate_r()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg integrate remove");
}

void vcpkg_Cx::do_get_triplet()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg help triplet");
}

void vcpkg_Cx::do_install(const std::string& t, const std::string& triplet)
{
	std::lock_guard<std::mutex> lock(_lock);
	auto c = "vcpkg install " + t;
	if (triplet.size())
	{
		c += ":" + triplet;
	}
	cmds.push(c);
}

void vcpkg_Cx::do_remove(const std::string& t, const std::string& triplet)
{
	std::lock_guard<std::mutex> lock(_lock);
	auto c = "vcpkg remove " + t;
	if (triplet.size())
	{
		c += ":" + triplet;
	}
	cmds.push(c);
}

void vcpkg_Cx::push_cmd(const std::string& c)
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push(c);
}



void vcpkg_Cx::do_cmd()
{
	std::string c;
	for (; cmds.size();) {
		{
			std::lock_guard<std::mutex> lock(_lock);
			c.swap(cmds.front()); cmds.pop();
		}
		if (c.empty())continue;
		auto jstr = hz::cmdexe(c, rootdir.empty() ? nullptr : rootdir.c_str());
		// todo 处理返回结果
	}
}



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
				//auto jstr = hz::cmdexe("c:");
				auto jstra = hz::cmdexe("echo %cd%", "c:\\sdk");
				auto jstraa = hz::cmdexe("echo %cd%", "e:\\sdk");
				printf("");
				//hz::opencmd([](const char* str) {printf(str); }, [](std::string& str) {
				//	str = "cd e:\n";

				//	});
				//auto jstr = hz::cmdexe("vcpkg integrate install");
				//auto jstr = hz::cmdexe("vcpkg search --x-full-desc --x-json");
				//auto jstr = hz::cmdexe("vcpkg list --x-full-desc --x-json");
				//hz::mfile_t mm;
				//auto kd = mm.new_m("temp/vlist.json", jstr.size());
				//if (kd)
				//{
				//	memcpy(kd, jstr.c_str(), mm.size());
				//	return;
				//}
				//jstr.push_back(0);
				//njson nn = njson::parse(jstr.c_str());
				//hz::save_json("temp/vlist.json", nn, 2);
				//printf("%s\n", cbt->str.c_str());
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
	p->on_click = [](plane_cx* p, int state, int clicks) {};
	p->fontsize = 16;

	size.y -= 50;
	size.x -= 50 * 10;
	size /= 2;

	auto dpx = p->push_dragpos(size);// 增加一个拖动坐标

	p->update_cb = [=](float delta)
		{
			static double kt = 0;
			kt += delta;
			if (kt > 0.5)
			{
				kt = 0.0;
				return 1;
			}
			return 0;
		};
	p->draw_back_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
			auto dps = p->get_dragpos(dpx);//获取拖动时的坐标
			cairo_translate(cr, dps.x, dps.y);
			{
				cairo_as _ss_(cr);
			}
			std::string str = (char*)u8"🍉";
			glm::vec4 rc = { 0,0,200,56 };
			text_style_t st = {};
			st.font = 0;
			st.text_align = { 0.0,0.5 };
			st.font_size = 39;
			st.text_color = -1;
			auto rc1 = p->ltx->get_text_rect(st.font, str.c_str(), -1, st.font_size);
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

int main()
{
#ifdef _DEBUG
	system("rd /s /q E:\\temcpp\\SymbolCache\\tcmp.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\vkcmp.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\cedit.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\p86.pdb");
#endif  
	auto app = new_app();

	glm::ivec2 ws = { 1280,860 };
	const char* wtitle = (char*)u8"vcpkg管理工具";

	form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable /*| ef_borderless*/ | ef_transparent);
	//form_x* form1 = (form_x*)new_form(app, wtitle1, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable);
	auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
	auto kd = sdldev.vkdev;
	sdldev.vkdev = 0;								// 清空使用独立创建逻辑设备
	std::vector<device_info_t> devs = get_devices(sdldev.inst); // 获取设备名称列表

	menu_m(form0);

	show_belt(form0);
	//show_cpuinfo(form0);
	// 运行消息循环
	run_app(app, 0);
	free_app(app);
	return 0;
}
