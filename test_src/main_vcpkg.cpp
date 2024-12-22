
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

extern "C" {
}

struct librpspr_lib
{
	void* ptr = 0;
	// 创建渲染器，类型是rgba，可以提前设置data做缓冲区，uint32数组或uint8
	int (*new_rp)(int w, int h, int64_t* ret, int bgrtex) = 0;
	// 删除渲染器
	void (*free_rp)(int64_t p) = 0;
	// 渲染完可以获取渲染的数据，ret的大小要跟渲染器一样
	int (*get_image)(void* p, uint8_t* ret) = 0;
	int (*rp_clear)(void* p, uint32_t c, double depth) = 0;
	// 新api。以下文件名支持网址、本地文件
	// 添加spr和对应的调色板文件名，调色板偏移(有的可能要负数)。返回序号draw_spr用
	int (*add_spr)(void* p, const char* fn, const char* palfn, int pal_pos) = 0;
	// 获取spr帧数
	int (*get_spr_frame_count)(void* p, int idx) = 0;
	// 保存spr的图片到png文件
	int (*save_spr2png)(void* p, int idx, const char* fn) = 0;
	// 加载坐骑/附件/足迹光环等，文件名用分号分隔json;png;graypng;pal.bmp顺序不限。坐骑深度信息没有.pal.bmp就拼三种文件名即可
	int (*add_part)(void* p, const char* uris) = 0;
	int (*set_ride)(void* p, const char* fly, int d, const char* fnm, const char* fntcp) = 0;
	int (*set_addon)(void* p, const char* addons) = 0;

	// 渲染顺序排序ride\spr\part\addon
	int (*draw_ride)(void* p, int frame, int* pos) = 0;
	int (*draw_spr)(void* p, int idx, int frame, int* pos, int z) = 0;
	int (*draw_part)(void* p, int idx, int frame, int* pos, int z) = 0;
	int (*draw_addon_flr)(void* p, int frame, int* pos) = 0;
	double (*get_max_depth)(void* p) = 0;
	int (*draw_spr_ps)(void* p, int idx, int frame, int* dst, int z, int ps) = 0;
public:
	librpspr_lib();
	~librpspr_lib();
	bool load();
};
librpspr_lib::librpspr_lib() {
	load();
}
librpspr_lib::~librpspr_lib() {
	if (ptr)hz::shared_destroy(ptr);
	delete ptr; ptr = 0;
}
bool librpspr_lib::load()
{
	if (ptr)return true;
	auto k = hz::shared_load(R"(rpspr.dll)");
	static const char* ccfn[] = { "ptr_null",
	"new_rp","free_rp","get_image","rp_clear","add_spr","get_spr_frame_count","save_spr2png"
	,"add_part","set_ride","set_addon","draw_ride","draw_spr","draw_part","draw_addon_flr","get_max_depth","draw_spr_ps"
	};
	if (k)
	{
		ptr = k;
		hz::shared_get(k, ccfn, (void**)&ptr, sizeof(ccfn) / sizeof(char*));
	}
	return ptr && new_rp && free_rp;
}
class dspr_cx
{
public:
	librpspr_lib* so = 0;
	void* ctx = 0;
	std::vector<glm::ivec4> sprv;
	std::vector<uint32_t> imgdata;
	cairo_surface_t* img = 0;

	// 坐骑的渲染坐标
	glm::ivec2 ride_pos = {};
public:
	dspr_cx();
	~dspr_cx();

	void init(int w, int h);
	void add_spr(const std::string& sprfn, const std::string& palbmp, int pal_pos);
	//不同坐骑可能要设置不同值
	void set_spr_pos(uint32_t idx, int x, int y, int z = 950000);
	void set_ride(const std::string& fly, int d, const std::string& addon, const std::string& fnm_dir, const std::string& fntcp_dir);
	void draw(int i, int x, int y);
private:

};

dspr_cx::dspr_cx()
{
	so = new librpspr_lib();
}

dspr_cx::~dspr_cx()
{

}

void dspr_cx::init(int w, int h)
{
	bool tex_bgr = 1;
	so->new_rp(w, h, (int64_t*)&ctx, tex_bgr);
	glm::ivec2 csize = { w,h };
	imgdata.resize(w * h);
	img = new_image_cr(csize, imgdata.data());
}

//在__init__函数调用 add_spr\set_ride
void dspr_cx::add_spr(const std::string& sprfn, const std::string& palbmp, int pal_pos)
{
	auto w = so->add_spr(ctx, sprfn.c_str(), palbmp.c_str(), pal_pos);
	sprv.push_back({ 0,0,0,w });
}

void dspr_cx::set_ride(const std::string& fly, int d, const std::string& addon, const std::string& fnm_dir, const std::string& fntcp_dir)
{
	// 动作类型，方向，坐骑深度目录(需要有这三种文件json;png;graypng)，坐骑tcp目录（json;png;graypng;pal.bmp）
	//so->set_ride(ctx, "stand", 0, R"(https://xyq.gsf.netease.com/h5avtres/1.25/e135776/p722/)", R"(https://xyq.gsf.netease.com/static_h5/shape/char/0722/)");
	so->set_ride(ctx, fly.c_str(), d, fnm_dir.c_str(), fntcp_dir.c_str());
	// 附件，路径和坐骑tcp一样，（json;png;graypng;pal.bmp）保存在addon_f之类文件夹。支持fblr四种要有相应文件夹
	so->set_addon(ctx, addon.c_str());
}

void dspr_cx::set_spr_pos(uint32_t idx, int x, int y, int z)
{
	if (idx < sprv.size())
	{
		sprv[idx].x = x;
		sprv[idx].y = y;
		sprv[idx].z = z;
	}
}

void dspr_cx::draw(int i, int x, int y)
{
	glm::ivec2 pos = { x,y };
	auto rpos = pos + ride_pos;
	//渲染调用 
	so->rp_clear(ctx, 0xff000000, 0.0);
	so->draw_ride(ctx, i, (int*)&rpos);//写入坐骑原始深度\渲染坐骑和附件b
	auto maxd = so->get_max_depth(ctx);
	//渲染角色/装备等
	static int ps[10] = {};
	int q = 0;
	for (auto& it : sprv)
	{
		glm::ivec2 sp = it;
		sp += pos;
		if (it.w > 0)
			so->draw_spr_ps(ctx, it.w, i, (int*)&sp, it.z, ps[q]);
		q++;
	}
	auto maxd0 = so->get_max_depth(ctx);
	// 渲染坐骑挂件
	so->draw_addon_flr(ctx, i, (int*)&rpos);
	so->get_image(ctx, (uint8_t*)imgdata.data());
}

std::string get_temp_path() {
	std::string str; str.resize(MAX_PATH * 2);
	auto n = GetTempPathA(str.size(), str.data());
	if (n > 0)
		str.resize(n);
	else
		str.clear();
	return str;
}
void build_spr_ui(form_x* form0)
{
	std::string tp = get_temp_path();
	auto sp = new dspr_cx();
	glm::ivec2 csize = { 800,600 };
	sp->init(csize.x, csize.y);

	//sp->add_spr(R"(https://cloudrender-test.nie.netease.com/clouderrender/render/?q=%7B%22act%22%3A%22ride_usual_3%22%2C%22cdnpath%22%3A%22h5sdk_hash%2F3d%2F39086dd49b5305dd3fe299cb7b0977%22%2C%22dir%22%3A0%2C%22eid%22%3A1008%2C%22pid%22%3A2%2C%22render_type%22%3A%22h5sdk_dynamic%22%2C%22restype%22%3A%22json%22%2C%22scale%22%3A1.25%2C%22ver%22%3A%22shapeact21%22%7D)"
	//	, R"(https://xyq.gsf.netease.com/static_h5/pal/equip/1008.pal.bmp)", 0);
	//sp->add_spr(R"(https://cloudrender-test.nie.netease.com/clouderrender/render/?q=%7B%22act%22%3A%22ride_usual_3%22%2C%22cdnpath%22%3A%22h5sdk_hash%2F45%2Fbde2321b431fc85f59b0c6170a424d%22%2C%22dir%22%3A0%2C%22eid%22%3A24608%2C%22pid%22%3A2%2C%22render_type%22%3A%22h5sdk_dynamic%22%2C%22restype%22%3A%22json%22%2C%22scale%22%3A1.25%2C%22ver%22%3A%22shapeact21%22%7D)"
	//	, R"(https://xyq.gsf.netease.com/static_h5/pal/equip/24608.pal.bmp)", 192);
	//sp->add_spr(R"(https://cloudrender-test.nie.netease.com/clouderrender/render/?q=%7B%22act%22%3A%22ride_usual_3%22%2C%22cdnpath%22%3A%22h5sdk_hash%2F82%2F330a98b0d60d4846a16125d7cdb691%22%2C%22dir%22%3A0%2C%22eid%22%3A16416%2C%22pid%22%3A2%2C%22render_type%22%3A%22h5sdk_dynamic%22%2C%22restype%22%3A%22json%22%2C%22scale%22%3A1.25%2C%22ver%22%3A%22shapeact21%22%7D)"
	//	, R"(https://xyq.gsf.netease.com/static_h5/pal/equip/16416.pal.bmp)", 192);

	const void* sprs[] = { R"(https://cloudrender-test.nie.netease.com/clouderrender/render/?q=%7B%22act%22%3A%22ride_usual_3%22%2C%22cdnpath%22%3A%22h5sdk_hash%2F45%2Fbde2321b431fc85f59b0c6170a424d%22%2C%22dir%22%3A0%2C%22eid%22%3A24608%2C%22pid%22%3A2%2C%22render_type%22%3A%22h5sdk_dynamic%22%2C%22restype%22%3A%22json%22%2C%22scale%22%3A1.25%2C%22ver%22%3A%22shapeact21%22%7D)"
	,R"(https://cloudrender-test.nie.netease.com/clouderrender/render/?q=%7B%22act%22%3A%22ride_usual_3%22%2C%22cdnpath%22%3A%22h5sdk_hash%2F82%2F330a98b0d60d4846a16125d7cdb691%22%2C%22dir%22%3A0%2C%22eid%22%3A16416%2C%22pid%22%3A2%2C%22render_type%22%3A%22h5sdk_dynamic%22%2C%22restype%22%3A%22json%22%2C%22scale%22%3A1.25%2C%22ver%22%3A%22shapeact21%22%7D)"
	,R"(https://cloudrender-test.nie.netease.com/clouderrender/render/?q=%7B%22act%22%3A%22ride_usual_3%22%2C%22cdnpath%22%3A%22h5sdk_hash%2F3d%2F39086dd49b5305dd3fe299cb7b0977%22%2C%22dir%22%3A0%2C%22eid%22%3A1008%2C%22pid%22%3A2%2C%22render_type%22%3A%22h5sdk_dynamic%22%2C%22restype%22%3A%22json%22%2C%22scale%22%3A1.25%2C%22ver%22%3A%22shapeact21%22%7D)"
	,R"(https://cloudrender-test.nie.netease.com/clouderrender/render/?q=%7B%22act%22%3A%22ride_usual_3%22%2C%22cdnpath%22%3A%22h5sdk_hash%2Fb1%2F535bc51083a99bc5b936132c6be5f5%22%2C%22dir%22%3A0%2C%22eid%22%3A0%2C%22pid%22%3A2%2C%22render_type%22%3A%22h5sdk_dynamic%22%2C%22restype%22%3A%22json%22%2C%22scale%22%3A1.25%2C%22ver%22%3A%22shapeact21%22%7D)"
	,R"()"
	};
	int palv[] = { 24608, 16416,1008,2, };
	int palpos[] = { 192, 0,0,0 };
	for (int i = 0; i < 4; i++) {
		std::string palurl = R"(https://xyq.gsf.netease.com/static_h5/pal/equip/)";
		sp->add_spr((char*)sprs[i], palurl + std::to_string(palv[i]) + ".pal.bmp", palpos[i]);
		sp->set_spr_pos(i, 230, 210, 950000);
	}

	//sp->add_spr(R"(https://xyq.gsf.netease.com/cloud_hash/ed/f01d07161ea7701046c8a8a7d40ef3.spr)", R"(https://xyq.gsf.netease.com/static_h5/pal/hero/2.pal.bmp)", 0);
	//sp->add_spr(R"(https://xyq.gsf.netease.com/cloud_hash/fd/d7ea390e1462b6a096cd67b04a0c28.spr)", R"(https://xyq.gsf.netease.com/static_h5/pal/equip/16416.pal.bmp)", 192);

	sp->ride_pos = { 250,280 };
	sp->set_ride("stand", 0, "flr", R"(https://xyq.gsf.netease.com/h5avtres/1.25/e135776/p722/)", R"(https://xyq.gsf.netease.com/static_h5/shape/char/0722/)");

	glm::ivec2 size = { 1024,800 };
	glm::ivec2 cview = { 10240,10240 };
	auto p = new plane_cx();
	uint32_t pbc = 0xff2c2c2c;// 背景色
	p->set_color({ 0x80ff802C,1,5,pbc });
	p->set_rss(5);
	p->_lms = { 8,8 };
	p->add_familys(fontn, 0);
	p->add_familys(fontn1, 0);
	p->draggable = false; //可拖动
	p->fontsize = 16;
	p->set_size(size);
	p->set_pos({ 1,30 });
	//p->set_select_box(2, 0.012);
	form0->bind(p);	// 绑定到窗口  
	auto dpx1 = p->push_dragpos({ 0,0 });// , { 300,600 });// 增加一个拖动坐标
	p->on_click = [](plane_cx* p, int state, int clicks, const glm::ivec2& mpos) {};
	p->update_cb = [=](float delta)
		{
			static double xt = 0;
			xt += delta;
			if (xt > 0.1) { xt = 0; return 1; }
			return 0;
		};
	p->draw_front_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
		};
	p->draw_back_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
			auto dps = p->get_dragpos(dpx1);//获取拖动时的坐标

			uint32_t color = 0x80FF7373;// hz::get_themecolor();

			{
				static int ix = 0;
				sp->draw(ix, 0, 0);
				ix++;
				if (ix > 100000)ix = 0;
				cairo_as _ss_(cr);
				cairo_translate(cr, dps.x, dps.y);
				image_b pimg = {};
				pimg.image = sp->img;
				pimg.rc = { 0,0,csize };
				//struct image_b {
				//	eg_e type = eg_e::e_image;
				//	void* image; glm::vec2 pos; glm::vec4 rc; uint32_t color = -1; glm::vec2 dsize = { -1,-1 };
				//	// 九宫格图片
				//	glm::vec4 sliced = {};
				//};
				// 图形通用软渲染接口
				draw_ge(cr, &pimg, 1);

				//draw_rect(cr, { -2.5,  -2.5,200 + 6,300 + 6 }, 0xf0236F23, 0x80ffffff, 2, 1);
			}
		};
}


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

	build_spr_ui(form0);
	p->set_rss(5);
	p->_lms = { 8,8 };
	p->add_familys(fontn, 0);
	p->add_familys(fontn1, 0);
	p->draggable = false; //可拖动
	p->set_size(size);
	p->set_pos({ 1,30 });
	p->set_select_box(2, 0.012);
	p->on_click = [](plane_cx* p, int state, int clicks, const glm::ivec2& mpos) {};
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
	auto a4 = glm::translate(glm::vec3(1.2, 0.2, 1.3));
	auto a40 = glm::translate(glm::vec3(0.2, 0.2, 1.3));
	auto aa = a4 * a40;
	glm::vec4 a = glm::vec4(1.0, 2.2, 3.0, 1.0);
	auto b = glm::vec4(0.1, .2, .3, 1.0);
	auto c = a * b;
	auto dpx1 = p->push_dragpos({ 0,0 });// , { 300,600 });// 增加一个拖动坐标
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
		}
	}
	auto ftff = ft->get_font("Source Han Sans SC", 0);
	if (k.size())
		ftns.push_back(k);
	static int stt = 0;
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
			auto dps1 = p->get_dragpos(dpx1);//获取拖动时的坐标

			uint32_t color = 0x80FF7373;// hz::get_themecolor();

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
