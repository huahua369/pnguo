
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

#ifdef GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#endif

auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman";// , Malgun Gothic";
auto fontn1 = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Malgun Gothic";

class vcpkg_cx
{
public:
	std::queue<std::string> cmds;
	std::mutex _lock;
	std::string rootdir;// vcpkg根目录
	njson vlist;
	njson vsearch;
public:
	vcpkg_cx();
	~vcpkg_cx();
	void do_clone(const std::string& dir, int depth);
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
vcpkg_cx::vcpkg_cx() {}
vcpkg_cx::~vcpkg_cx() {}
void vcpkg_cx::do_clone(const std::string& dir, int depth)
{
	if (dir.size() < 2)return;
	std::lock_guard<std::mutex> lock(_lock);
	rootdir = dir;
	std::string c = "git clone https://github.com/microsoft/vcpkg.git";
	if (depth > 0)
	{
		c += " --depth=" + std::to_string(depth);
	}
	cmds.push(c);
}
void vcpkg_cx::do_bootstrap()
{
	if (rootdir.size() < 2)return;
	std::lock_guard<std::mutex> lock(_lock);
#ifdef _WIN32
	cmds.push("bootstrap-vcpkg.bat");
#else
	cmds.push("bootstrap-vcpkg.sh");
#endif // _WIN32

}
void vcpkg_cx::do_pull()
{
	if (rootdir.size() < 2)return;
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("git pull");
}
void vcpkg_cx::do_update(const std::string& t)
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg update");
}
void vcpkg_cx::do_search()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg search --x-full-desc --x-json");
}
void vcpkg_cx::do_list()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg list --x-full-desc --x-json");

}

void vcpkg_cx::do_integrate_i()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg integrate install");
}

void vcpkg_cx::do_integrate_r()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg integrate remove");
}

void vcpkg_cx::do_get_triplet()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg help triplet");
}

void vcpkg_cx::do_install(const std::string& t, const std::string& triplet)
{
	std::lock_guard<std::mutex> lock(_lock);
	auto c = "vcpkg install " + t;
	if (triplet.size())
	{
		c += ":" + triplet;
	}
	cmds.push(c);
}

void vcpkg_cx::do_remove(const std::string& t, const std::string& triplet)
{
	std::lock_guard<std::mutex> lock(_lock);
	auto c = "vcpkg remove " + t;
	if (triplet.size())
	{
		c += ":" + triplet;
	}
	cmds.push(c);
}

void vcpkg_cx::push_cmd(const std::string& c)
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push(c);
}


void vcpkg_cx::do_cmd()
{
	std::string c;
	for (; cmds.size();) {
		{
			std::lock_guard<std::mutex> lock(_lock);
			c.swap(cmds.front()); cmds.pop();
		}
		if (c.empty())continue;
		auto jstr = hz::cmdexe(c, rootdir.empty() ? nullptr : rootdir.c_str());
		try
		{
			njson k = njson::parse(jstr);
			if (k.size())
				vlist = k;
		}
		catch (const std::exception& e)
		{
			printf("json error:\t%s\n", e.what());
		}
		printf("%s\n", jstr.c_str());
		// todo 处理返回结果
	}
}


menu_cx* menu_m(form_x* form0)
{
	menumain_info m = {};
	m.form0 = form0;
	m.fontn = fontn;
	// 主菜单
	std::vector<std::string> mvs = { (char*)u8"文件",(char*)u8"编辑",(char*)u8"视图",(char*)u8"工具",(char*)u8"帮助" };
	m.mvs = &mvs;
	auto mc = mg::new_mm(&m);
	return mc;
}


void show_ui(form_x* form0, menu_cx* gm)
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
	p->add_familys(fontn1, 0);
	p->draggable = false; //可拖动
	p->set_size(size);
	p->set_pos({ 1,30 });
	p->on_click = [](plane_cx* p, int state, int clicks) {};
	p->fontsize = 16;
	int width = 16;
	int rcw = 14;
	{
		// 设置带滚动条
		p->set_scroll(width, rcw, { 4, 4 }, { 2,0 }, { 0,2 });
		p->set_scroll_hide(1);
		p->set_view(size, cview);
	}
	size.y -= 50;
	size.x -= 50 * 10;
	size /= 2;
	vcpkg_cx* vx = new vcpkg_cx();
	std::vector<const char*> btnname = {
	"install",//安装库(const std::string & t, const std::string & triplet); 
	"remove",//删除库(const std::string & t, const std::string & triplet); 
	"clone",//(const std::string & dir, int depth); 
	"bootstrap",// 
	"update",//比较更新了哪些库(const std::string & t);	 
	"ecmd",//执行自定义命令(const std::string & c); 
	};
	std::vector<const char*> mname = {
	"list",//列出安装库 
	"search",//搜索库 
	"pull",//拉取  		 
	"get triplet",//查看支持的架构  
	"integrate install",//集成到全局 
	"integrate remove",//移除全局 
	};
	auto color = 0x2c2c2c;// hz::get_themecolor();
	((uint8_t*)(&color))[3] = 0x80;
#if 0
	std::vector<menu_info> vm;
	vm.push_back({ mname.data(),mname.size() });
	auto mms = gm->new_menu_g(vm.data(), vm.size(), { 200,30 }, [=](mitem_t* pm, int type, int idx)
		{
			if (type == 1)
			{
				printf("click:%d\t%d\n", type, idx);
			}
		});
	//for (auto& nt : btnname)
	{
		auto cp = p->add_gbutton((char*)u8"功能", { 160,30 }, color);
		cp->click_cb = [=](void* ptr, int clicks)
			{
				auto cps = cp->get_pos();
				cps.y += cp->size.y + cp->thickness;
				gm->show_mg(mms, 0, cps);
			};
	}
#else
	for (auto& nt : mname)
	{
		//auto cp = p->add_gbutton(nt, { 180,30 }, color);
		//cp->text_align.x = 0;
		auto cp = p->add_cbutton(nt, { 180,30 }, 0);
		cp->effect = uTheme::light;
		cp->click_cb = [=](void* ptr, int clicks)
			{
				printf("%d\n", clicks);
				vx->do_list();
				vx->do_cmd();
			};
	}
	auto ce = p->add_input("", { 1000,30 }, 1);
	ce->set_family(0, 12);
	for (auto& nt : btnname)
	{
		auto cp = p->add_gbutton(nt, { 160,30 }, color);
		cp->click_cb = [=](void* ptr, int clicks)
			{
			};
	}

#endif
	int cs[] = { sizeof(glm::vec1),sizeof(glm::vec2),sizeof(glm::vec3),sizeof(glm::ivec3) };
	auto a4 = glm::translate(glm::vec3(1.2,0.2,1.3));
	auto a40 = glm::translate(glm::vec3(0.2,0.2,1.3));
	auto aa = a4 * a40;
	glm::vec4 a = glm::vec4(1.0, 2.2, 3.0, 1.0);
	auto b = glm::vec4(0.1, .2, .3, 1.0);
	auto c = a * b;
	auto dpx1 = p->push_dragpos(size, { 300,600 });// 增加一个拖动坐标
	size /= 2;
	auto dpx = p->push_dragpos(size, { 900 ,630 });// 增加一个拖动坐标
	auto rint = get_rand64(0, (uint32_t)-1);

	auto ft = p->ltx->ctx;
	int fc = ft->get_count();						// 获取注册的字体数量
	const char* get_family(int idx);		// 获取字体名称
	const char* get_family_en(const char* family);		// 获取字体名称
	const char* get_family_full(int idx);	// 获取字体全名
	struct ft_info
	{
		std::string name, full, style;
	};
	static std::vector<ft_info> fvs;
	fvs.resize(fc);
	static std::vector<std::string> ftns;
	std::string k;
	for (size_t i = 0; i < fc; i++)
	{
		int cs = ft->get_count_style(i);			// 获取idx字体风格数量
		auto& it = fvs[i];
		it.name = ft->get_family_cn(i);
		it.full = ft->get_family_full(i);
		for (size_t c = 0; c < cs; c++)
		{
			auto sty = ft->get_family_style(i, c);
			it.style += sty; it.style.push_back(';');
		}
		{
			if (i > 0 && (i % 30 == 0))
			{
				ftns.push_back(k); k.clear();
			}
			k += it.name + "\n";
			//auto cp = p->add_cbutton(it.name, { 280,30 }, 3);
			//cp->effect = uTheme::light;
			//cp->click_cb = [=](void* ptr, int clicks)
			//	{
			//	};
		}
	}
	auto ftff = ft->get_font("Source Han Sans SC", 0);
	if (k.size())
		ftns.push_back(k);
	static int stt = 0;
	p->update_cb = [=](float delta)
		{
			static double kt = 0;
			kt += delta;
			if (kt > 0.2)
			{
				stt++;
				if (stt > 9)stt = 0;
				kt = 0.0;
				return 1;
			}
			return 0;
		};
	p->draw_front_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
			auto dps1 = p->get_dragpos(dpx1);//获取拖动时的坐标
			uint32_t color = 0x80FF7373;// hz::get_themecolor();
			vg_style_t st[1] = {};
			st->dash = 0xF83E0;
			st->num_bit = 20;
			st->dash_offset = -stt;
			st->thickness = 1;
			st->join = 1;
			st->cap = 1;
			st->fill = 0x80FF7373;
			st->color = 0xffffffff;
			float pad = st->thickness > 1 ? 0.0 : -0.5;
			//if (dps1.z == 0)
			{
				cairo_as _ss_(cr);
				//	draw_rect(cr, { dps1.x + 0.5,dps1.y + 0.5,300,600 }, color, 0x80ffffff, 2, 1); 

				draw_rectangle(cr, { dps1.x + pad ,dps1.y + pad ,300,600 }, 0);
				fill_stroke(cr, st);
			}
		};
	p->draw_back_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
			auto f = ft->get_font("a", 0);
			auto dps = p->get_dragpos(dpx);//获取拖动时的坐标
			auto dps1 = p->get_dragpos(dpx1);//获取拖动时的坐标

			uint32_t color = 0x80FF7373;// hz::get_themecolor();
			vg_style_t st[1] = {};
			st->dash = 0xF83E0;
			st->num_bit = 20;
			st->dash_offset = -stt;
			st->thickness = 1;
			st->join = 1;
			st->cap = 1;
			st->fill = 0x80FF7373;
			st->color = 0xffffffff;
			float pad = st->thickness > 1 ? 0.0 : -0.5;
			if (dps1.z == 0)
			{
				cairo_as _ss_(cr);
				//	draw_rect(cr, { dps1.x + 0.5,dps1.y + 0.5,300,600 }, color, 0x80ffffff, 2, 1); 

				//draw_rectangle(cr, { dps1.x + pad ,dps1.y + pad ,300,600 }, 0);
				//fill_stroke(cr, st);
			}
			{
				cairo_as _ss_(cr);
				cairo_translate(cr, dps.x, dps.y);
				glm::vec4 rc = { 0,0,300,1000 };
				text_style_t st = {};
				st.font = 1;
				st.text_align = { 0.0,0.0 };
				st.font_size = 16;
				st.text_color = -1;
				//auto rc1 = p->ltx->get_text_rect(st.font, str.c_str(), -1, st.font_size);
				auto rc2 = rc;
				draw_rect(cr, { -2.5,  -2.5,900 + 6,630 + 6 }, 0xf0236F23, 0x80ffffff, 2, 1);
				for (auto& str : ftns)
				{
					draw_text(cr, p->ltx, str.c_str(), -1, rc, &st);
					rc.x += 300;
				}
			}
			if (dps1.z > 0)
			{
				cairo_as _ss_(cr);
				//draw_rect(cr, { dps1.x + 0.5,dps1.y + 0.5,300,600 }, color, 0x80ffffff, 2, 1);
				//draw_rectangle(cr, { dps1.x + pad ,dps1.y + pad ,300,600 }, 0);
				//fill_stroke(cr, st);
			}
		};
}


void test_img() {

	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr.png", 0);
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
	const char* wtitle = (char*)u8"vcpkg管理工具";
	auto app = new_app();

	do_text(fontn, 0, strlen(fontn));
	glm::ivec2 ws = { 1280,860 };
	// ef_vulkan ef_gpu
	form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, (ef_vulkan | ef_resizable));//ef_gpuef_dx11| ef_vsync
	//form_x* form1 = (form_x*)new_form(app, wtitle1, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable);
	auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
	auto kd = sdldev.vkdev;
	sdldev.vkdev = 0;								// 清空使用独立创建逻辑设备
	std::vector<device_info_t> devs = get_devices(sdldev.inst); // 获取设备名称列表

	auto gm = menu_m(form0);

	show_ui(form0, gm);
	//show_cpuinfo(form0);
	// 运行消息循环
	run_app(app, 0);
	free_app(app);
	return 0;
}
