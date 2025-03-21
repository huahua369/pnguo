
#include <pch1.h>

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
#define HEX_EDITOR_STATIC_LIB
#include <hex_editor.h>
#include "mshell.h"
#ifdef GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#endif 
#include <pnguo/editor_2d.h>
#include <spine/spine-sdl3/spinesdl3.h>
#include <stb_image_write.h>
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
	static hex_editor hex;
	auto g = md::gb_u8((char*)kc.c_str(), -1);
	auto nhs = hex_size;
	nhs.y -= width;
	auto st = get_defst(1);
	auto fl = p->ltx->get_lineheight(st->font, st->font_size);
	hex.set_linechar(fl, p->ltx->get_text_rect1(st->font, st->font_size, "x").x);
	hex.set_view_size(nhs);
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
	auto hex_scroll = p->add_scroll2(hex_size, width, rcw, { fl, fl }, { 2 * 0,0 }, { 0,2 * 0 });
	hex_scroll.v->set_viewsize(hex_size.y, (hex.acount + 5) * fl, 0);
	hex_scroll.h->set_viewsize(hex_size.x, hex_size.x, 0);
	hex_scroll.v->has_hover_sc = true;
	hex_scroll.v->hover_sc = true;
	static int stt = 0;
	mmcb = [=](void* p, int type, int id) {
		if (id > 0)return;
		auto fn = hz::browse_openfile("选择文件", "", "所有文件\t*.*\tPNG格式(*.png)\t*.png\tJPEG(*.jpg)\t*.jpg\t", form0->get_nptr(), 0);
		if (fn.size()) {
			std::string str = fn[0];

			{
				print_time aak("load file");
				if (hex.set_file(str.c_str(), true))
				{
					hex.update_hex_editor();
					hex.set_pos(0);
					hex_scroll.v->set_viewsize(hex_size.y, (hex.acount + 5) * fl, 0);
					//kc.assign((char*)hex.data(), hex.size());
				}
			}
		}
		};
	p->add_mouse_cb([=](plane_ev* e)
		{
			if (e->down)
			{
				auto phex = &hex;
				auto dps = p->get_dragpos(dpx);//获取拖动时的坐标 
				auto dgp = p->get_dragv6(dpx);
				dgp->size.x = -1;
				glm::ivec2 scpos = e->mpos - p->tpos;// 减去本面板坐标
				scpos -= (glm::ivec2)dps;
				//printf("old click:%d\t%d\n", scpos.x, scpos.y);
				auto hps = hex_scroll.h->get_offset_ns();
				auto vps = hex_scroll.v->get_offset_ns();
				scpos -= (glm::ivec2)phex->text_rc[2];
				phex->on_mouse(e->clicks, scpos, hps, vps);
				scpos.x += hps;
				scpos.y += vps;
				//printf("click:%d\t%d\n", scpos.x, scpos.y);
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
			auto dps = p->get_dragpos(dpx);//获取拖动时的坐标 
			uint32_t color = 0x80FF7373;// hz::get_themecolor();
			{
				cairo_as _ss_(cr);
				cairo_translate(cr, dps.x, dps.y);
				glm::ivec2 hps = dps;
				glm::vec4 chs = { 0,0,hex_size };
				hps.y += hex_size.y - width;
				hex_scroll.h->pos = hps;
				dps.x += hex_size.x - width;
				hex_scroll.v->pos = dps;
				glm::vec4 bgrc = { -2.5,  -2.5,hex_size.x + 3,hex_size.y + 3 };
				auto phex = &hex;
				glm::vec4 rc = { 0,0,300,1000 };
				if (phex->acount == 0)
				{
					draw_rect(cr, bgrc, 0xf0121212, 0x80ffffff, 2, 1);
					//auto pf = &ftns;
					//for (auto& str : ftns)
					//{
						//draw_text(cr, p->ltx, str.c_str(), -1, rc, st);
						//rc.x += 300;
					//}
					return;
				}
				auto fl = p->ltx->get_lineheight(st->font, st->font_size);
				auto pxx = hex_scroll.h->get_offset_ns();
				auto pyy = hex_scroll.v->get_offset_ns();
				auto vps = pyy / fl;
				pyy = vps * fl;
				hex_style_t hst = {};
				hst.st = st;
				hst.ltx = p->ltx;
				hst.pxx = pxx;
				hst.pyy = pyy;
				hst.fl = fl;
				hst.hex_size = hex_size;
				hst.bgrc = bgrc;
				hst.view_size = { hex_scroll.h->_view_size,hex_scroll.v->_view_size };
				hst.cr = cr;
				phex->update_draw(&hst);
				auto dt = phex->get_drawt();
				if (dt->box_rc.z != hex_scroll.h->_content_size)
					hex_scroll.h->set_viewsize(hex_size.x, dt->box_rc.z + fl, 0);
			}

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

void build_audio_test(int seconds, std::vector<float>& data)
{
#define SAMPLE_RATE 48000
#define FREQUENCY   440
#define FRAME_SIZE  1024
	int channels = 2;
	int framenum = seconds * SAMPLE_RATE;// ((SAMPLE_RATE + FRAME_SIZE - 1) / FRAME_SIZE);
	auto M_PI = glm::pi<double>();
	data.resize(framenum * channels);
	auto dtt = data.data();
	float zeta = 0; //每一个frame 的初始角度
	int amp = 10000; //幅度
	for (int i = 0; i < channels; ++i) {
		auto dst = dtt + i;
		for (int j = 0; j < framenum; ++j) {
			*dst = amp * sin(2 * M_PI * FREQUENCY / SAMPLE_RATE * j + zeta); //每一个数据递进一个角度2*PI*FREQUENCY/SAMPLE_RATE,此值为角速度
			*dst *= 0.0001;
			dst += channels;
		}
	}
}
template<class T>
void build_audio_test(int seconds, std::vector<T>& data)
{
#define SAMPLE_RATE 48000
#define FREQUENCY   440
#define FRAME_SIZE  1024
	int channels = 2;
	int framenum = seconds * SAMPLE_RATE;// ((SAMPLE_RATE + FRAME_SIZE - 1) / FRAME_SIZE);
	auto M_PI = glm::pi<double>();
	data.resize(framenum * channels);
	auto dtt = data.data();
	float zeta = 0; //每一个frame 的初始角度
	int amp = 10000; //幅度
	for (int i = 0; i < channels; ++i) {
		auto dst = dtt + i;
		for (int j = 0; j < framenum; ++j) {
			*dst = amp * sinf(2 * M_PI * FREQUENCY / SAMPLE_RATE * j + zeta); //每一个数据递进一个角度2*PI*FREQUENCY/SAMPLE_RATE,此值为角速度 
			dst += channels;
		}
	}
}


int main()
{
#ifdef _DEBUG
	system("rd /s /q E:\\temcpp\\SymbolCache\\tcmp.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\vkcmp.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\cedit.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\p86.pdb");
#endif     
	const char* wtitle = (char*)u8"多功能管理工具";
	auto app = new_app();
	cpuinfo_t cpuinfo = get_cpuinfo();
	glm::ivec2 ws = { 1280,860 };
	// ef_vulkan ef_gpu|ef_resizable
	form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, (ef_vulkan));
	auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
	auto kd = sdldev.vkdev;
	sdldev.vkdev = 0;					// 清空使用独立创建逻辑设备
	std::vector<device_info_t> devs = get_devices(sdldev.inst); // 获取设备名称列表
	auto gm = menu_m(form0);
	printf("%p\n", form0);
	auto pl = show_ui(form0, gm);
	show_ui2(form0, gm);
	//show_cpuinfo(form0);
	auto d2 = new sp_drawable();
	d2->set_renderer(form0->renderer);
	d2->add(R"(E:\vsz\g3d\s2d\spine-runtimes\spine-sdl\data\spineboy-pma.atlas)"
		, R"(E:\vsz\g3d\s2d\spine-runtimes\spine-sdl\data\spineboy-pro.json)", 0.25, 0.2);
	d2->add(R"(E:\vsz\g3d\s2d\spine-runtimes\spine-glfw\data\spineboy-pma.atlas)"
		, R"(E:\vsz\g3d\s2d\spine-runtimes\spine-glfw\data\spineboy-pro.skel)", 0.25, 0.2);
	d2->set_pos(0, 300, 350);
	d2->set_pos(1, 300, 650);
	atlas_strinfo ass = get_atlas_strinfo();
	std::vector<char*> nv;
	d2->get_anim_name(0, &nv);
	d2->animationstate_set_animationbyname(0, 0, "portal", 0);
	d2->animationstate_add_animationbyname(0, 0, "run", -1, 0);
	d2->animationstate_set_animationbyname(1, 0, "portal", 0);
	d2->animationstate_add_animationbyname(1, 0, "shoot", -1, 0);
	std::vector<float>* adata = new std::vector<float>();
	std::vector<int32_t>* adata32 = new std::vector<int32_t>();
	std::vector<int16_t>* adata16 = new std::vector<int16_t>();
	build_audio_test(5, *adata);
	build_audio_test(5, *adata32);
	build_audio_test(5, *adata16);
	coders_t* cp = new_coders();
	audio_data_t* mad1 = new_audio_data(cp, R"(E:\song\平生不晚-难却.flac)");
	audio_data_t* mad12 = new_audio_data(cp, R"(E:\vsz\g3d\s2d\spine-runtimes\spine-unity\Assets\Spine Examples\Sound\Jump.ogg)");
	audio_data_t* mad21 = new_audio_data(cp, R"(E:\vsz\g3d\s2d\spine-runtimes\spine-unity\Assets\Spine Examples\Sound\Spineboygun.ogg)");
	audio_data_t* madogg = new_audio_data(cp, R"(E:\SteamLibrary\steamapps\common\Cities_Skylines\Files\Radio\Music\Cities\Europa Universalis IV - Battle of Lepanto.ogg)");
	audio_data_t* mad = new_audio_data(cp, R"(E:\song\陈奕迅-好久不见.flac)");
	audio_data_t* mad2 = new_audio_data(cp, R"(E:\song2\程响-是否.mp3)");
	auto st = app->new_audio_stream(mad->format, mad->channels, mad->freq);
	auto st1 = app->new_audio_stream(mad1->format, mad1->channels, mad1->freq);
	auto st32f = app->new_audio_stream(2, 2, 48000);
	auto st32 = app->new_audio_stream(1, 2, 48000);
	auto st16 = app->new_audio_stream(0, 2, 48000);

	{
		encoder_info_t e = {};
		while (1)
		{
			int rc1 = decoder_data(mad2);
			if (rc1 <= 0)
			{
				break;
			}
		}
		e.bits_per_sample = 16;
		e.channels = mad2->channels;
		e.sample_rate = mad2->freq;
		e.total_samples = mad2->total_samples;
		e.data = (char*)mad2->data;
		e.data_size = mad2->len;
		e.file_path = R"(E:\song2\程响-是否cf.flac)";
		auto pe = cp->codes[0];
		e.handle = pe->handle;
		int ret = pe->encoder(&e);
		printf("%d\n", ret);
	}

	/*
	open_audio
		new_audio_stream创建流（支持flac、ogg、mp3）
		decoder_data解码数据
		put_audio推送数据
		free_audio_stream
	close_audio
	*/

	{
		form0->render_cb = [=](SDL_Renderer* renderer, double delta)
			{
				auto dt = (char*)mad->data + mad->desize;
				auto dt1 = (char*)mad1->data + mad1->desize;
				int rc = decoder_data(mad);
				auto qn = app->get_audio_stream_queued(st);
				//if (rc > 0)
				//	app->put_audio(st, dt, rc);
				static double deltas = 0;
				deltas += delta;
				if (deltas > 0)
				{
					int rc1 = decoder_data(mad1);
					if (rc1 > 0)
						app->put_audio(st1, dt1, rc1);
				}
				if (adata->size())
				{
					//app->put_audio(st16, adata16->data(), adata16->size() * sizeof(int16_t));
					//app->put_audio(st32, adata32->data(), adata32->size() * sizeof(int32_t));//无法播放
					//app->put_audio(st32f, adata->data(), adata->size() * sizeof(float));
					//adata->clear();
				}
				else {}
				d2->update_draw(delta);
			};
	}
	auto lt = pl->ltx;
	text_path_t opt = {};
	text_image_t opti = {};
	auto tp = lt->get_shape(0, u8"富强", 50, &opt);
	auto tpi = lt->get_glyph_item(0, u8"富强", 50, &opti);
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
	//getchar(); 
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

	// 运行消息循环
	run_app(app, 0);
	delete d2;
	free_coders(cp);
	free_app(app);
	return 0;
}
