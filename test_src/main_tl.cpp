
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
#include <vkgui/print_time.h>

#include <cairo/cairo.h>

#include "mshell.h"
#ifdef GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#endif

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
void show_ui(form_x* form0, menu_cx* gm)
{
	if (!form0)return;
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
	p->set_select_box(2, 0.012);
	p->on_click = [](plane_ev* e) {};
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
	int cs[] = { sizeof(glm::vec1),sizeof(glm::vec2),sizeof(glm::vec3),sizeof(glm::ivec3) };
	auto a4 = glm::translate(glm::vec3(1.2, 0.2, 1.3));
	auto a40 = glm::translate(glm::vec3(0.2, 0.2, 1.3));
	auto aa = a4 * a40;
	glm::vec4 a = glm::vec4(1.0, 2.2, 3.0, 1.0);
	auto b = glm::vec4(0.1, .2, .3, 1.0);
	auto c = a * b;
	size /= 2;
	glm::vec2 hex_size = { 900,600 };
	auto dpx = p->push_dragpos({ 50,160 }, hex_size);// 增加一个拖动坐标
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
	hex.font_size = fl;
	hex.set_size(nhs);
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
	p->on_click = [=](plane_ev* e)
		{
			auto dps = p->get_dragpos(dpx);//获取拖动时的坐标 
			auto dgp = p->get_dragv6(dpx);
			dgp->size.x = -1;
			glm::ivec2 scpos = e->mpos - p->tpos;// 减去本面板坐标
			scpos -= (glm::ivec2)dps;
			printf("old click:%d\t%d\n", scpos.x, scpos.y);
			auto vps = hex_scroll.v->get_offset_ns();
			auto hps = hex_scroll.h->get_offset_ns();
			scpos.x += hps;
			scpos.y += vps;
			scpos -= (glm::ivec2)hex.text_rc[2];
			printf("click:%d\t%d\n", scpos.x, scpos.y);

		};
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
				draw_rect(cr, { -2.5,  -2.5,hex_size.x + 3,hex_size.y + 3 }, 0xf0121212, 0x80ffffff, 2, 1);
				auto pf = &ftns;
				auto phex = &hex;
				glm::vec4 rc = { 0,0,300,1000 };
				if (phex->acount == 0)
				{
					for (auto& str : ftns)
					{
						//draw_text(cr, p->ltx, str.c_str(), -1, rc, st);
						rc.x += 300;
					}
					return;
				}
				clip_cr(cr, chs);
				//auto rc1 = p->ltx->get_text_rect(st.font, str.c_str(), -1, st.font_size);0xf0236F23
				auto rc2 = rc;

				auto fl = p->ltx->get_lineheight(st->font, st->font_size);
				auto vps = hex_scroll.v->get_offset_ns() / fl;
				auto xx = hex_scroll.h->get_offset_ns();
				cairo_translate(cr, -xx, 0);
				phex->set_linepos(vps);
				phex->update_hex_editor();

				text_draw_t* dt = phex->get_drawt();
				dt->cr = cr; dt->ltx = p->ltx;
				dt->st = st;
				// 定义常量 
				const int MARGIN = 10;
				const int DATA_INSPECTOR_TITLE_WIDTH = 80;
				const int DECODED_TEXT_WIDTH = 150;
				const int RULER_DI_WIDTH = 200;
				// 主绘制代码 
				auto nn = phex->line_number_n;
				int cw = st->font_size * 0.5;
				int line_number_width = nn * cw;
				int data_width = phex->bytes_per_line * cw * 3;
				auto height = hex_scroll.v->_view_size;
				phex->dititle = (char*)u8"数据检查器";
				// 设置各个区域的绘制位置 
				int ruler_w = phex->ruler.size() * cw;
				phex->text_rc[0] = { MARGIN + line_number_width, MARGIN, ruler_w, height }; // ruler 
				phex->text_rc[1] = { MARGIN, MARGIN + fl ,line_number_width, height }; // line_number 
				phex->text_rc[2] = { MARGIN + line_number_width, MARGIN + fl, data_width, height }; // data_hex 
				int dataps = MARGIN + line_number_width + data_width + fl;
				phex->text_rc[3] = { dataps, MARGIN + fl ,DECODED_TEXT_WIDTH, height }; // decoded_text 
				dataps += phex->bytes_per_line * cw + fl;
				phex->text_rc[4] = { dataps, MARGIN + fl, RULER_DI_WIDTH, height }; // ruler_di 
				phex->text_rc[5] = { dataps, MARGIN ,RULER_DI_WIDTH, height }; // dititle 
				dataps += DATA_INSPECTOR_TITLE_WIDTH;
				phex->text_rc[6] = { dataps, MARGIN + fl, RULER_DI_WIDTH, height }; // data_inspector 
				phex->color[0] = 0xffff783b;	//0xfffb8f71;	// 标尺
				phex->color[1] = 0xff999999;	// 行号
				phex->color[2] = 0xffeeeeee;	// 16进制数据
				phex->color[3] = 0xffeeeeee;	// 解码文件
				phex->color[4] = 0xff999999;	// 数据检查器字段头
				phex->color[5] = -1;			// 数据检查器标题
				phex->color[6] = 0xff0cc616;	// 0xff107c10;	// 数据检查器相关信息
				//hex_edit->set_pos({ { phex->text_rc[2].x,  phex->text_rc[2].y  } }); 
				draw_draw_texts(dt);

				if (dt->box_rc.z != hex_scroll.h->_content_size)
					hex_scroll.h->set_viewsize(hex_size.x, dt->box_rc.z + fl, 0);
			}

		};
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
	//do_text(fontn, 0, strlen(fontn)); 
	glm::ivec2 ws = { 1280,860 };
	// ef_vulkan ef_gpu
	form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, (ef_vulkan | ef_resizable));//ef_gpuef_dx11| ef_vsync
	//form_x* form1 = (form_x*)new_form(app, wtitle1, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable);
	auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
	auto kd = sdldev.vkdev;
	sdldev.vkdev = 0;								// 清空使用独立创建逻辑设备
	std::vector<device_info_t> devs = get_devices(sdldev.inst); // 获取设备名称列表
	auto gm = menu_m(form0);
	printf("%p\n", form0);
	show_ui(form0, gm);
	show_ui2(form0, gm);
	//show_cpuinfo(form0);
	// 运行消息循环
	run_app(app, 0);
	free_app(app);
	return 0;
}
