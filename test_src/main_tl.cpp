﻿
#include <pch1.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <pnguo/win_core.h>
#include "win32msg.h"
#include <pnguo/event.h>
#define GUI_STATIC_LIB
#include <pnguo/pnguo.h>
#include <pnguo/tinysdl3.h>
#include <pnguo/vkrenderer.h>
#include <pnguo/page.h>
#include <pnguo/mapView.h>
#include <pnguo/print_time.h>
#include <pnguo/mnet.h> 
#include <cairo/cairo.h>
#include "mshell.h"
#ifdef GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#endif 
#include <pnguo/editor_2d.h>
#include <spine/spine-sdl3/spinesdl3.h>
#include <stb_image_write.h>

#include "logic_gates.h"

auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman";// , Malgun Gothic";
auto fontn1 = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Malgun Gothic";
auto fontn2 = (char*)u8"Consolas,新宋体,Segoe UI Emoji,Times New Roman,Malgun Gothic";
text_style_t* get_defst(int idx)
{
	static text_style_t st[5] = {};
	st[0].font = 0;
	st[0].text_align = { 0.0,0.0 };
	st[0].font_size = 16;
	st[0].text_color = -1;

	st[1].font = 1;
	st[1].text_align = { 0.0,0.0 };
	st[1].font_size = 16;
	st[1].text_color = -1;
	st[2].font = 2;
	st[2].text_align = { 0.0,0.0 };
	st[2].font_size = 16;
	st[2].text_color = -1;
	return &st[idx];
}
extern "C" {
}
/*
	CLAY_RENDER_COMMAND_TYPE_NONE,
	CLAY_RENDER_COMMAND_TYPE_RECTANGLE,
	CLAY_RENDER_COMMAND_TYPE_BORDER,
	CLAY_RENDER_COMMAND_TYPE_TEXT,
	CLAY_RENDER_COMMAND_TYPE_IMAGE,
	CLAY_RENDER_COMMAND_TYPE_SCISSOR_START,
	CLAY_RENDER_COMMAND_TYPE_SCISSOR_END,
	CLAY_RENDER_COMMAND_TYPE_CUSTOM,
*/


/*
容器控件：子项渲染、事件处理、数据结构
*/
#include <memory>
#include <filesystem>

class vcpkg_cx
{
public:
	std::queue<std::string> cmds;
	std::mutex _lock;
	std::string rootdir; // vcpkg根目录
	njson vlist;
	njson vsearch;

public:
	vcpkg_cx();
	~vcpkg_cx();
	void do_clone(const std::string& dir, int depth);
	void do_bootstrap();
	void do_pull();
	void do_update(const std::string& t);
	void do_search();
	void do_list();
	void do_integrate_i();
	void do_integrate_r();
	void do_get_triplet();
	void do_install(const std::string& t, const std::string& triplet);
	void do_remove(const std::string& t, const std::string& triplet);
	void push_cmd(const std::string& c);
	void do_cmd();

private:
	bool is_valid_rootdir() const;
	void add_cmd(const std::string& cmd);
};

vcpkg_cx::vcpkg_cx() {}
vcpkg_cx::~vcpkg_cx() {}

bool vcpkg_cx::is_valid_rootdir() const
{
	return rootdir.size() >= 2;
}

void vcpkg_cx::add_cmd(const std::string& cmd)
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push(cmd);
}

void vcpkg_cx::do_clone(const std::string& dir, int depth)
{
	if (dir.size() < 2) return;
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
	if (!is_valid_rootdir()) return;
	add_cmd(
#ifdef _WIN32
		"bootstrap-vcpkg.bat"
#else
		"bootstrap-vcpkg.sh"
#endif
	);
}

void vcpkg_cx::do_pull()
{
	if (!is_valid_rootdir()) return;
	add_cmd("git pull");
}

void vcpkg_cx::do_update(const std::string& t)
{
	add_cmd("vcpkg update");
}

void vcpkg_cx::do_search()
{
	add_cmd("vcpkg search --x-full-desc --x-json");
}

void vcpkg_cx::do_list()
{
	add_cmd("vcpkg list --x-full-desc --x-json");
}

void vcpkg_cx::do_integrate_i()
{
	add_cmd("vcpkg integrate install");
}

void vcpkg_cx::do_integrate_r()
{
	add_cmd("vcpkg integrate remove");
}

void vcpkg_cx::do_get_triplet()
{
	add_cmd("vcpkg help triplet");
}

void vcpkg_cx::do_install(const std::string& t, const std::string& triplet)
{
	std::string c = "vcpkg install " + t;
	if (!triplet.empty())
	{
		c += ":" + triplet;
	}
	add_cmd(c);
}

void vcpkg_cx::do_remove(const std::string& t, const std::string& triplet)
{
	std::string c = "vcpkg remove " + t;
	if (!triplet.empty())
	{
		c += ":" + triplet;
	}
	add_cmd(c);
}

void vcpkg_cx::push_cmd(const std::string& c)
{
	add_cmd(c);
}

void vcpkg_cx::do_cmd()
{
	std::string c;
	while (!cmds.empty())
	{
		{
			std::lock_guard<std::mutex> lock(_lock);
			c.swap(cmds.front());
			cmds.pop();
		}
		if (c.empty()) continue;
		auto jstr = hz::cmdexe(c, rootdir.empty() ? nullptr : rootdir.c_str());
		try
		{
			njson k = njson::parse(jstr);
			if (!k.empty())
				vlist = k;
		}
		catch (const std::exception& e)
		{
			printf("json error:\t%s\n", e.what());
		}
		printf("%s\n", jstr.c_str());
	}
}

#if 1
// hexedit

#define HEX_EDITOR_STATIC_LIB
#include <hex_editor.h>

struct pack_hex_t
{
	hex_editor* hex = 0;
	scroll2_t hex_scroll = {};
	int fl = 0;
	glm::vec2 hex_size = { 900,600 };
	int width = 16;
	//int rcw = 14;
	plane_cx* p = 0;
	size_t dpx = 0;
};
void loadfile(pack_hex_t* ph, const std::string& str) {

	if (ph->hex->set_file(str.c_str(), true))
	{
		ph->hex->update_hex_editor();
		ph->hex->set_pos(0);
		ph->hex_scroll.v->set_viewsize(ph->hex_size.y, (ph->hex->acount + 5) * ph->fl, 0);
	}
}
void downmuose(pack_hex_t* ph, plane_ev* e) {
	auto dps = ph->p->get_dragpos(ph->dpx);//获取拖动时的坐标 
	auto dgp = ph->p->get_dragv6(ph->dpx);
	dgp->size.x = -1;
	glm::ivec2 scpos = e->mpos - ph->p->tpos;// 减去本面板坐标
	scpos -= (glm::ivec2)dps;
	//printf("old click:%d\t%d\n", scpos.x, scpos.y);
	auto hps = ph->hex_scroll.h->get_offset_ns();
	auto vps = ph->hex_scroll.v->get_offset_ns();
	scpos -= (glm::ivec2)ph->hex->text_rc[2];
	ph->hex->on_mouse(e->clicks, scpos, hps, vps);
	scpos.x += hps;
	scpos.y += vps;
}
void draw_hex(pack_hex_t* ph, cairo_t* cr)
{
	auto dps = ph->p->get_dragpos(ph->dpx);//获取拖动时的坐标 
	{
		cairo_as _ss_(cr);
		cairo_translate(cr, dps.x, dps.y);

		glm::ivec2 hps = dps;
		glm::vec4 chs = { 0,0,ph->hex_size };
		hps.y += ph->hex_size.y - ph->width;
		ph->hex_scroll.h->pos = hps;
		dps.x += ph->hex_size.x - ph->width;
		ph->hex_scroll.v->pos = dps;
		glm::vec4 bgrc = { -2.5,  -2.5,ph->hex_size.x + 3,ph->hex_size.y + 3 };
		auto phex = ph->hex;
		glm::vec4 rc = { 0,0,300,1000 };
		if (phex->acount == 0)
		{
			draw_rect(cr, bgrc, 0xf0121212, 0x80ffffff, 2, 1);
			return;
		}
		auto st = get_defst(1);
		auto fl = ph->p->ltx->get_lineheight(st->font, st->font_size);
		auto pxx = ph->hex_scroll.h->get_offset_ns();
		auto pyy = ph->hex_scroll.v->get_offset_ns();
		auto vps = pyy / fl;
		pyy = vps * fl;
		hex_style_t hst = {};
		hst.st = st;
		hst.ltx = ph->p->ltx;
		hst.pxx = pxx;
		hst.pyy = pyy;
		hst.fl = fl;
		hst.hex_size = ph->hex_size;
		hst.bgrc = bgrc;
		hst.view_size = { ph->hex_scroll.h->_view_size,ph->hex_scroll.v->_view_size };
		hst.cr = cr;
		phex->update_draw(&hst);
		auto dt = phex->get_drawt();
		if (dt->box_rc.z != ph->hex_scroll.h->_content_size)
			ph->hex_scroll.h->set_viewsize(ph->hex_size.x, dt->box_rc.z + fl, 0);
	}
}
pack_hex_t* testhex(plane_cx* p) {
	auto hex = new hex_editor();
	glm::vec2 hex_size = { 900,600 };
	int width = 16;
	int rcw = 14;
	auto dpx = p->push_dragpos({ 20,60 }, hex_size);// 增加一个拖动坐标
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
	static std::string kc = (char*)"ab 串口fad1\r\n231ffwfadfsfgdfgdfhjhg";
	auto g = md::gb_u8((char*)kc.c_str(), -1);
	auto nhs = hex_size;
	nhs.y -= width;
	auto st = get_defst(1);
	auto fl = p->ltx->get_lineheight(st->font, st->font_size);
	hex->set_linechar(fl, p->ltx->get_text_rect1(st->font, st->font_size, "x").x);
	hex->set_view_size(nhs);
	auto hex_scroll = p->add_scroll2(hex_size, width, rcw, { fl, fl }, { 2 * 0,0 }, { 0,2 * 0 });
	hex_scroll.v->set_viewsize(hex_size.y, (hex->acount + 5) * fl, 0);
	hex_scroll.h->set_viewsize(hex_size.x, hex_size.x, 0);
	hex_scroll.v->has_hover_sc = true;
	hex_scroll.v->hover_sc = true;
	auto ph = new pack_hex_t();
	ph->fl = fl;
	ph->p = p;
	ph->hex = hex;
	ph->hex_scroll = hex_scroll;
	ph->hex_size = hex_size;
	ph->dpx = dpx;
	ph->width = width;

	return ph;
}
#endif // 0



std::function<void(void* p, int type, int id)> mmcb;
void mcb(void* p, int type, int id) {
	if (mmcb)
		mmcb(p, type, id);
}

menu_cx* menu_m(form_x* form0)
{
	menumain_info m = {};
	m.form0 = form0;
	m.fontn = fontn;
	// 主菜单
	std::vector<std::string> mvs = { (char*)u8"文件",(char*)u8"编辑",(char*)u8"视图",(char*)u8"工具",(char*)u8"帮助" };
	m.mvs = &mvs;
	m.cb = [=](mitem_t* p, int type, int id) {
		//mcb(p, type, id);
		};
	m.mcb = [=](void* p, int clicks, int id) {
		mcb(p, clicks, id);
		};
	auto mc = mg::new_mm(&m);
	return mc;
}
void tohexstr(std::string& ot, const char* d, int len, int xlen, uint64_t line, bool n16) {
	auto hs = pg::to_string_hex(line, n16 ? 16 : 8, "x");
	hs += ": ";
	auto aa = std::string(d, len);
	bool isu8 = hz::is_utf8(d, len);
	if (!isu8)
	{
		auto u = hz::gbk_to_u8(aa);
		if (u.size())aa = u;
	}
	for (auto& it : aa) {
		if (it == 0)
		{
			it = '.';
		}
	}

	{
		unsigned int cp1 = 0;
		auto uc = md::get_utf8_count(aa.c_str(), aa.size());
		if (uc > 1)
		{
			auto laststr = aa.c_str();
			std::string c, c0;
			for (int k = 0; k < uc; k++)
			{
				auto t = laststr;
				laststr = md::utf8_next_char(laststr);
				c.assign(t, laststr);
				if (c.size() == 1)
				{
					if ((*t > 0 && !isprint(*t)) || *t < 0)
						c = ".";
				}
				c0 += c;
			}
			aa.swap(c0);
		}
	}
}
plane_cx* show_ui(form_x* form0, menu_cx* gm)
{
	if (!form0)return 0;
	glm::ivec2 size = { 1024,800 };
	glm::ivec2 cview = { 1580,1580 };
	auto p = new plane_cx();
	uint32_t pbc = 0xff2c2c2c;
	p->set_color({ 0x80ff802C,1,5,pbc });
	form0->bind(p);	// 绑定到窗口  
	//build_spr_ui(form0);
	p->set_rss(5);
	p->_lms = { 8,8 };
	p->add_familys(fontn, 0);
	p->add_familys(fontn1, 0);
	p->add_familys(fontn2, 0);
	p->draggable = false; //可拖动
	p->set_size(size);
	p->set_pos({ 1,30 });
	p->set_select_box(2 * 0, 0.012);	// 设置是否显示选择框

	p->fontsize = 16;
	int width = 16;
	int rcw = 14;
	{
		// 设置带滚动条
		p->set_scroll(width, rcw, { 4, 4 }, { 2 * 0,0 }, { 0,2 * 0 });
		//p->set_scroll_hide(0);
		p->set_view(size, cview);
		p->vertical->hover_sc = false;
		p->vertical->has_hover_sc = false;
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

	//form_x* form1 = (form_x*)new_form(form0->app, "", 600,500, -1, -1, ef_vulkan);
	//auto cp = p->add_gbutton((char*)u8"功能", { 160,30 }, color);
	//cp->click_cb = [=](void* ptr, int clicks)
	//	{
	//		form1->flash_window(2);
	//	};
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
	//for (auto& nt : mname)
	//{
	//	auto cp = p->add_cbutton(nt, { 180,30 }, 0);
	//	cp->effect = uTheme::light;
	//	cp->click_cb = [=](void* ptr, int clicks)
	//		{
	//			auto pb = (color_btn*)ptr;
	//			printf("%d\n", clicks);
	//			print_time aa("list");
	//			vx->do_list();
	//			vx->do_cmd();
	//		};
	//}
	//auto ce = p->add_input("", { 1000,30 }, 1);

	//for (auto& nt : btnname)
	//{
	//	auto cp = p->add_gbutton(nt, { 160,30 }, color);
	//	cp->click_cb = [=](void* ptr, int clicks)
	//		{
	//		};
	//}

#endif
	auto ce = p->add_input("", { 500,30 }, 1);
	int cs[] = { sizeof(glm::vec1),sizeof(glm::vec2),sizeof(glm::vec3),sizeof(glm::ivec3) };
	auto a4 = glm::translate(glm::vec3(1.2, 0.2, 1.3));
	auto a40 = glm::translate(glm::vec3(0.2, 0.2, 1.3));
	auto aa = a4 * a40;
	glm::vec4 a = glm::vec4(1.0, 2.2, 3.0, 1.0);
	auto b = glm::vec4(0.1, .2, .3, 1.0);
	auto c = a * b;
	size /= 2;
	glm::vec2 hex_size = { 900,600 };
	auto dpx = p->push_dragpos({ 20,60 }, hex_size);// 增加一个拖动坐标
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
	static std::string kc = (char*)"ab 串口fad1\r\n231ffwfadfsfgdfgdfhjhg";
	auto g = md::gb_u8((char*)kc.c_str(), -1);
	auto nhs = hex_size;
	nhs.y -= width;
	auto st = get_defst(1);
	auto fl = p->ltx->get_lineheight(st->font, st->font_size);

	//{
	//	print_time aak("load file");
	//	if (hex.set_file(R"(E:\迅雷下载\g\tenoke-romance.of.the.three.kingdoms.8.remake.iso)", true))
	//	{
	//		hex.update_hex_editor();
	//		hex.set_pos(0);
	//		//kc.assign((char*)hex.data(), hex.size());
	//	}
	//}
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
		}
	}
	auto ftff = ft->get_font("Source Han Sans SC", 0);
	if (k.size())
		ftns.push_back(k);
	//p->add_colorpick(0, 280, 30, true);
#if 0
	hz::mcurl_cx mc;
	{
		auto kv = hz::read_json("temp/aiv.json");
		mc.set_httpheader("Authorization", "Bearer " + kv["v"][0].get<std::string>());
		njson aa;
		aa["model"] = "deepseek-chat";
		aa["stream"] = false;
		aa["temperature"] = 0.7;
		std::string ct = (char*)u8"你是谁";
		aa["messages"] = { {{"role", "user"}, {"content",ct.c_str()}} };
		auto aad = aa.dump();
		mc.post("https://api.deepseek.com/chat/completions", aad.data(), aad.size());
		//mc.get("https://api.deepseek.com/user/balance");//查余额
		if (mc._data.size())
		{
			auto dd = (char*)mc._data.data();
			auto n = njson::parse(dd, dd + mc._data.size());
			//hz::save_json("temp/ai_get.json", n, 2);
		}
		printf("%s\n", mc._data.data());
	}
	if (0) {
		cmde_cx cmdx([](const char* str, int len)
			{
				auto s = md::gb_u8(str, len);
				printf("%s", s.c_str());
			});
		while (1) {
			static bool k = false;
			static bool c = false;
			if (k)
			{
				cmdx.write_str("cd E:\\aicpp\n"); k = false;
			}
			if (c)
			{
				//cmdx.write_str("exit\n");//退出cmd
				c = false;
				break;
			}
			Sleep(100);
		}
	}
#endif
	auto hex_edit = p->add_input("", { fl * 3,fl }, 1);
	hex_edit->_absolute = true;
	hex_edit->visible = false;
	pack_hex_t* ph = testhex(p);
	static int stt = 0;
	mmcb = [=](void* p, int type, int id) {
		if (id > 0)return;
		auto fn = hz::browse_openfile("选择文件", "", "所有文件\t*.*\tPNG格式(*.png)\t*.png\tJPEG(*.jpg)\t*.jpg\t", form0->get_nptr(), 0);
		if (fn.size()) {
			std::string str = fn[0];

			{
				print_time aak("load file");
				loadfile(ph, str);
			}
		}
		};
	p->add_mouse_cb([=](plane_ev* e)
		{
			if (e->down)
			{
				downmuose(ph, e);
			}
		});
	p->update_cb = [=](float delta)
		{
			return 0;
		};
	p->draw_front_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
		};
	p->draw_back_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
			auto f = ft->get_font("a", 0);
			uint32_t color = 0x80FF7373;// hz::get_themecolor(); 
			draw_hex(ph, cr);

		};

	return p;
}

void show_ui2(form_x* form0, menu_cx* gm)
{
	if (!form0)return;
}
void test_img() {

	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr.png", 0);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr60.jpg", 60);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr80.jpg", 80);
}
void clearpdb()
{
#ifdef _DEBUG 
	system("rd /s /q E:\\temcpp\\SymbolCache\\vkcmp.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\cedit.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\p86.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\mtl.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\mw.pdb");
#endif 
}

// 计算网格点
SDL_FPoint* generateGridPoints(int gridSize, int canvasSize, std::vector<SDL_FPoint>* opt) {
	int lines = static_cast<int>(std::ceil(static_cast<float>(canvasSize) / gridSize));
	int totalPoints = lines * 2 * 2 + 4; // 水平+垂直线，每线2个点
	if (opt->size() != totalPoints)
	{
		opt->resize(totalPoints);
	}
	SDL_FPoint* points = opt->data();
	int index = 0;
	float a = lines * gridSize;
	// 水平线 
	for (int y = 0; y <= canvasSize; y += gridSize) {
		points[index++] = { 0.0f, static_cast<float>(y) };
		points[index++] = { a, static_cast<float>(y) };
	}

	// 垂直线 
	for (int x = 0; x <= canvasSize; x += gridSize) {
		points[index++] = { static_cast<float>(x), 0.0f };
		points[index++] = { static_cast<float>(x), a };
	}

	return points;
}

// 绘制网格 
void genGrid(int gridSize, int canvasSize, std::vector<SDL_FPoint>* opt) {
	generateGridPoints(gridSize, canvasSize, opt);
}
void drawGrid(SDL_Renderer* renderer, const glm::vec2& pos, int gridSize, int canvasSize, std::vector<SDL_FPoint>* opt) {
	SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 1.0); // 设置线条颜色为黑色 
	SDL_FPoint* points = opt->data();
	auto length = opt->size() / 2;
	for (size_t i = 0; i < length; i++)
	{
		auto it = points[0];
		auto it1 = points[1];
		SDL_RenderLine(renderer, it.x + pos.x, it.y + pos.y, it1.x + pos.x, it1.y + pos.y);
		points += 2;
	}
}
int main()
{
	std::stack<int> st;
	clearpdb();
	const char* wtitle = (char*)u8"多功能管理工具";
	auto tstr = hz::u8_to_gbk(wtitle);
	auto app = new_app();
	cpuinfo_t cpuinfo = get_cpuinfo();
	glm::ivec2 ws = { 1280,860 };
	// ef_vulkan ef_gpu|ef_resizable
	form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, (ef_vulkan | ef_resizable));
	auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
	auto kd = sdldev.vkdev;
	sdldev.vkdev = 0;					// 清空使用独立创建逻辑设备
	std::vector<device_info_t> devs = get_devices(sdldev.inst); // 获取设备名称列表

	//vkdg_cx* vkd = new_vkdg(sdldev.inst, sdldev.phy);	// 创建vk渲染器 
	bs::test_timeline();
	auto gm = menu_m(form0);
	printf("%p\n", form0);
	auto pl = show_ui(form0, gm);
	show_ui2(form0, gm);
	//show_cpuinfo(form0);
	glm::mat3 m3x = glm::shearX(glm::mat3(1), 0.5f);
	glm::mat3 m3y = glm::shearY(glm::mat3(1), 0.5f);
	glm::vec3 v3 = { 1,2,3 };
	auto v3x = m3x * v3;
	auto v3x0 = v3 * m3x;
	float x = 0.2;
	auto v3y = m3y * x;
	auto v3y0 = x * m3y;
	auto d2 = new sp_drawable();
	d2->set_renderer(form0->renderer);
	/*d2->add(R"(E:\vsz\g3d\s2d\spine-runtimes\spine-sdl\data\spineboy-pma.atlas)"
		, R"(E:\vsz\g3d\s2d\spine-runtimes\spine-sdl\data\spineboy-pro.json)", 0.25, 0.2, "temp/spineboy-j.spt");
	d2->add(R"(E:\vsz\g3d\s2d\spine-runtimes\spine-glfw\data\spineboy-pma.atlas)"
		, R"(E:\vsz\g3d\s2d\spine-runtimes\spine-glfw\data\spineboy-pro.skel)", 0.25, 0.2, "temp/spineboy-skel.spt");
	*/

	d2->add_pkg("temp/spineboy-j.spt", 0.25, 0.2);
	d2->add_pkg("temp/spineboy-skel.spt", 0.25, 0.2);
	d2->set_pos(0, 600, 650);
	d2->set_pos(1, 300, 650);

	{
		njson j = hz::read_json(R"(E:\vsz\g3d\s2d\spine-runtimes\spine-sdl\data\spineboy-pro.json)");
		const char* kname[] = { "skeleton","bones","slots","ik","skins","transform","animations","events" };
		auto& skeleton = j["skeleton"];
		auto& bones = j["bones"];
		auto& slots = j["slots"];
		auto& ik = j["ik"];
		auto& skins = j["skins"];
		auto& transform = j["transform"];
		auto& animations = j["animations"];
		auto& events = j["events"];
		glm::vec4 size = { hz::toDouble(skeleton["x"]),hz::toDouble(skeleton["y"]),hz::toDouble(skeleton["width"]),hz::toDouble(skeleton["height"]), };
		std::string hash = hz::toStr(skeleton["hash"]);
		std::set<std::string> names;
		std::map<std::string, std::set<std::string>> mm;
		for (size_t i = 0; i < sizeof(kname) / sizeof(char*); i++)
		{
			auto& kk = j[kname[i]];
			if (kk.is_object())
			{
				for (auto& [k, v] : kk.items())
				{
					names.insert(k);
				}
			}
			else if (kk.is_array())
			{
				for (auto& it : kk)
				{
					for (auto& [k, v] : it.items())
					{
						names.insert(k);
					}
				}
			}
			mm[kname[i]] = (std::move(names));
			names.clear();
		}
		if (mm.size())
		{
			printf("\n");
		}
	}
	atlas_strinfo ass = get_atlas_strinfo();
	static std::vector<char*> nv;
	d2->get_anim_name(0, &nv);
	d2->animationstate_set_animationbyname(0, 0, "portal", 0);
	d2->animationstate_add_animationbyname(0, 0, "run", -1, 0);
	d2->animationstate_set_animationbyname(1, 0, "portal", 0);
	d2->animationstate_add_animationbyname(1, 0, "shoot", -1, 0);

	texture_cb tex_cb = get_texture_cb();
	{
		auto xh_tex = (SDL_Texture*)tex_cb.new_texture_file(form0->renderer, "data/xh1.png");
		page_obj_t ro = {};
		ro.renderer = form0->renderer;
		auto ptex = new texture_cb();
		*ptex = tex_cb;
		ro.cb = ptex;
		auto xha = new_atlas("data/xh1.atlas", &ro);

		logic_cx* logic = new_logic(xha);
		logic->_pos = { 20,90 };
		logic->add_gate(dType::AND_GATE, "and", { 60,60 }, 0);
		logic->add_gate(dType::AND_GATE, "and", { 60 ,150 }, 0);
		logic->add_gate(dType::AND_GATE, "and", { 150,60 }, 0);
		logic->add_gate(dType::AND_GATE, "and", { 150,150 }, 0);
		logic->add_gate(dType::AND_GATE, "and", { 230,60 }, 0);
		logic->gates[0].input = 0x00;
		logic->gates[1].input = 0x03;
		logic->gates[2].input = 0x01;
		logic->gates[3].input = 0x02;
		logic->gates[4].build = 1;
		glm::vec2 nnpos = { 280,0 };
		logic->add_gate(dType::XOR_GATE, "xor0", { nnpos.x + 60,nnpos.y + 60 }, 0);
		logic->add_gate(dType::XOR_GATE, "xor3", { nnpos.x + 60 ,nnpos.y + 150 }, 0);
		logic->add_gate(dType::XOR_GATE, "xor1", { nnpos.x + 150,nnpos.y + 60 }, 0);
		logic->add_gate(dType::XOR_GATE, "xor2", { nnpos.x + 150,nnpos.y + 150 }, 0);
		logic->add_gate(dType::XOR_GATE, "xor", { nnpos.x + 230,nnpos.y + 60 }, 0);
		int xx = 5;
		logic->gates[xx + 0].input = 0x00;
		logic->gates[xx + 1].input = 0x03;
		logic->gates[xx + 2].input = 0x01;
		logic->gates[xx + 3].input = 0x02;
		logic->gates[xx + 4].build = 1;
		nnpos += glm::vec2({ 0,180 });
		logic->add_gate(dType::OR_GATE, "or0", { nnpos.x + 60,nnpos.y + 60 }, 0);
		logic->add_gate(dType::OR_GATE, "or3", { nnpos.x + 60 ,nnpos.y + 150 }, 0);
		logic->add_gate(dType::OR_GATE, "or1", { nnpos.x + 150,nnpos.y + 60 }, 0);
		logic->add_gate(dType::OR_GATE, "or2", { nnpos.x + 150,nnpos.y + 150 }, 0);
		logic->add_gate(dType::OR_GATE, "or", { nnpos.x + 230,nnpos.y + 60 }, 0);
		xx += 5;
		logic->gates[xx + 0].input = 0x00;
		logic->gates[xx + 1].input = 0x03;
		logic->gates[xx + 2].input = 0x01;
		logic->gates[xx + 3].input = 0x02;
		logic->gates[xx + 4].build = 1;
		nnpos += glm::vec2({ -280,0 });
		logic->add_gate(dType::NOT_GATE, "not1", { nnpos.x + 150,nnpos.y + 60 }, 0);
		logic->add_gate(dType::NOT_GATE, "not2", { nnpos.x + 150,nnpos.y + 150 }, 0);
		logic->add_gate(dType::NOT_GATE, "not", { nnpos.x + 230,nnpos.y + 60 }, 0);
		xx += 5;
		logic->gates[xx + 0].input = 0x00; 
		logic->gates[xx + 1].input = 0x01; 
		logic->gates[xx + 2].build = 1;
		logic->_scale = 1.0;
		form0->add_event(0, [=](uint32_t type, et_un_t* e, void* ud)
			{
				auto btn = e->v.b;
				static int idx = -1;
				if (type != (uint32_t)devent_type_e::mouse_button_e || btn->down)return;
				idx++;
				if (idx < nv.size())
				{
					d2->animationstate_add_animationbyname(1, 0, nv[idx], -1, 0);
				}

			});

		// 随机数种子
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dis(0, 5);
		std::vector<int> lines;
		int lw = 26;
		int gh = 30;
		int yps = 0;
		lines.resize(26 * 16);
		for (size_t i = 0; i < lines.size(); i++)
		{
			int tt = dis(gen);
			lines[i] = tt;
		}
		std::vector<SDL_FPoint>* opt2 = new std::vector<SDL_FPoint>();
		genGrid(15, 70 * 15, opt2);
		form0->render_cb = [=](SDL_Renderer* renderer, double delta)
			{
				static double deltas = 0;
				deltas += delta;
				if (xh_tex) {
					glm::ivec4 notm = get_lgates_rc(2);
					glm::ivec4 andm = get_lgates_rc(3);
					glm::ivec4 orm = get_lgates_rc(4);
					glm::ivec4 xorm = get_lgates_rc(5);
					glm::ivec4 wline[6] = {};
					for (size_t i = 0; i < 6; i++)
					{
						wline[i] = get_lgates_rc(6 + i);
					}

					drawGrid(renderer, { 20,90 }, 15, 70 * 15, opt2);

					texture_dt adt = {   };
					adt.src_rect = { 0,0,512,512 };
					adt.dst_rect = { 106,106,adt.src_rect.z,adt.src_rect.w };
					//tex_cb.render_texture(renderer, xh_tex, &adt, 1); 



					auto tl = lines.data();
					glm::ivec2 npos = { 106,306 };
					texture_dt nadt = {};
					for (size_t i = 0; i < lines.size(); i++)
					{
						glm::ivec2 ps = { i % lw,i / lw };
						auto x = *tl;
						nadt.src_rect = wline[x];
						nadt.dst_rect = { npos.x + ps.x * gh,yps + npos.y + ps.y * gh,nadt.src_rect.z,nadt.src_rect.w };
						//tex_cb.render_texture(renderer, xh_tex, &nadt, 1);
						tl++;
					}
				}
				d2->update_draw(delta);
				logic->draw_update(delta);
			};
	}


	// 运行消息循环
	run_app(app, 0);
	delete d2;
	free_app(app);
	return 0;
}

void test_m(layout_text_x* ltx)
{
	auto lt = ltx;
	text_path_t opt = {};
	text_image_t opti = {};
	auto tp = lt->get_shape(0, 50, u8"富强", &opt);
	auto tpi = lt->get_glyph_item(0, 50, u8"富强", &opti);
	text_image_pt* tip = text_blur(&opt, 4, 2, 0xff111111, 0xff0000ff);
	uint32_t color = -1;
	auto image = opti.tv[0]._image;
	for (auto& it : opti.tv) {
		//ft, it._dwpos + it._apos, it._rect, it.color ? it.color : color

	}
	save_img_png(image, "temp/font_fw.png");
	// 保存到png\jpg
	textimage_file(tip, "temp/text_inflate.png");
	free_textimage(tip);

	maze_cx maze;
	astar_search as;
	int ww = 100;
	maze.set_seed(15687951215562);
	maze.init(ww, ww);
	as.init(ww, ww, (uint8_t*)maze.data(), false);
	// 大于0则是不通
	as.set_wallheight(0);
	glm::ivec2 pStart = maze.start, pEnd = maze.dest;//随机开始、结束坐标
	bool hok = as.FindPath(&pStart, &pEnd, 0);
	std::vector<uint32_t> mg;
	mg.resize(ww * ww, 0);
	auto nma = maze.data();
	for (size_t i = 0; i < mg.size(); i++)
	{
		mg[i] = nma[i] == 0 ? 0 : 0xff000000;
	}
	stbi_write_png("temp/maze.png", ww, ww, 4, mg.data(), 0);
	for (auto& it : as.path)
	{
		mg[it.x + it.y * ww] = 0xff0020ff;
	}
	mg[pStart.x + pStart.y * ww] = 0xff00ff00;
	mg[pEnd.x + pEnd.y * ww] = 0xffff0000;
	stbi_write_png("temp/maze2.png", ww, ww, 4, mg.data(), 0);
}