
#include <pch1.h>
#include <vkgui/pnguo.h>
#include <vkgui/tinysdl3.h>
#include <vkgui/mapView.h>
#include <vkgui/print_time.h>

#include <iostream>

#include <cairo/cairo.h>
#include <stdfloat>
#include "charts.h"
#include <vulkan/vulkan.h>
#include <vkvg/vkvgcx.h>

/*
	todo
	样式：
		填充色、边框颜色、线粗、段数、段长
		字体名、字号、颜色
	文本渲染
		矩形：坐标、大小、圆角、(裁剪、区域)、对齐方式
		文本
	图形渲染
		线、方块、圆、三角形、路径线、位图
*/

#define DVC_EXPORT extern "C" __declspec(dllimport)

DVC_EXPORT int rvk(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow, LPCSTR Name);
DVC_EXPORT int rdx12(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow, LPCSTR Name);
struct v22
{
	glm::vec2 a, b;
};
glm::vec2 draw_r(cairo_t* cr, glm::vec4 t, float aj) {
	t.w = 1;
	glm::mat4 a = glm::rotate(glm::radians(aj), glm::vec3(1.0, 0.0, 0.0));//沿Z轴旋转90度
	auto t2 = a * t;
	auto tt = t;
	tt *= 6;
	draw_circle(cr, tt, 2);
	fill_stroke(cr, 0xffffffff, 0, 1, false);
	tt = t2;
	tt *= 6;
	draw_circle(cr, tt, 2);
	fill_stroke(cr, 0xff00ff00, 0, 1, false);
	return t2;
}



#include <clipper2/clipper.h> 
using namespace Clipper2Lib;
void tobox(const glm::vec2& v, glm::vec4& t);
void tobox0(const glm::vec2& v, glm::vec4& t)
{
	if (v.x < t.x)
	{
		t.x = v.x;
	}
	if (v.y < t.y)
	{
		t.y = v.y;
	}
	if (v.x > t.z)
	{
		t.z = v.x;
	}
	if (v.y > t.w)
	{
		t.w = v.y;
	}

}

/*
*
*  0中心
*   1     2
	-------
	|     |
	-------
	4     3
*/
glm::vec2 getbox2t_(std::vector<glm::vec2>& vt, int t)
{
	glm::vec4 box = { INT_MAX,INT_MAX,INT_MIN,INT_MIN };
	for (auto& it : vt)
	{
		tobox(it, box);
	}
	glm::vec2 cp = { box.z - box.x,box.w - box.y };
	switch (t)
	{
	case 0:
		cp *= 0.5;
		break;
	case 1:
		cp = {};
		break;
	case 2:
		cp.y = 0;
		break;
	case 3:
		break;
	case 4:
		cp.x = 0;
		break;
	default:
		break;
	}
	cp.x += box.x;
	cp.y += box.y;
	return cp;
}
glm::vec2 getbox2t_(const glm::vec2* vt, int count, int t)
{
	glm::vec4 box = { INT_MAX,INT_MAX,INT_MIN,INT_MIN };
	for (size_t i = 0; i < count; i++)
	{
		auto it = vt[i];
		tobox(it, box);
	}
	glm::vec2 cp = { box.z - box.x,box.w - box.y };
	switch (t)
	{
	case 0:
		cp *= 0.5;
		break;
	case 1:
		cp = {};
		break;
	case 2:
		cp.y = 0;
		break;
	case 3:
		break;
	case 4:
		cp.x = 0;
		break;
	default:
		break;
	}
	cp.x += box.x;
	cp.y += box.y;
	return cp;
}

glm::vec2 getbox2t_(glm::vec4 box, int t)
{
	glm::vec2 cp = { box.z - box.x,box.w - box.y };
	switch (t)
	{
	case 0:
		cp *= 0.5;
		break;
	case 1:
		cp = {};
		break;
	case 2:
		cp.y = 0;
		break;
	case 3:
		break;
	case 4:
		cp.x = 0;
		break;
	default:
		break;
	}
	cp.x += box.x;
	cp.y += box.y;
	return cp;
}
// 缩放基于获取图形包围盒，返回坐标t=0中心，1左上角，2右上角，3右下角，4左下角，
void scale_pts(glm::vec2* pts, int count, const glm::vec2& sc, int ddot, bool inv)
{
	auto cp = getbox2t_(pts, count, ddot);//获取包围盒
	glm::vec2 cp0 = {};
	glm::vec2 scale = sc;
	// 缩放
	assert(scale.x > 0 && scale.y > 0);
	if (scale.x > 0 && scale.y > 0)
	{
		auto m = glm::translate(glm::mat3(1.0f), glm::vec2(cp)) * glm::scale(glm::mat3(1.0f), scale)
			* glm::translate(glm::mat3(1.0f), glm::vec2(-cp));
		if (inv)
		{
			m = glm::inverse(m);// 撤销执行反向操作 
		}
		auto v = pts;
		for (size_t i = 0; i < count; i++)
		{
			glm::vec3 v3 = { v[i] ,1.0 };
			v[i] = glm::vec2(m * v3);
		}
	}
}
void draw_pts(cairo_t* cr, std::vector<glm::vec2>& ptv, uint32_t c) {

	cairo_move_to(cr, (int)ptv[0].x, (int)ptv[0].y);
	for (size_t i = 1; i < ptv.size(); i++)
	{
		auto v2 = ptv[i];
		cairo_line_to(cr, (int)v2.x, (int)v2.y);
	}
	cairo_close_path(cr);
	fill_stroke(cr, 0, c, 1, false);
}
struct vkdinfo {
	VkDevice				vkDev;					/**< Vulkan Logical Device */
	VkPhysicalDeviceMemoryProperties phyMemProps;	/**< Vulkan Physical device memory properties */
	VkPhysicalDevice		phy;					/**< Vulkan Physical device */
	VkInstance				instance;				/**< Vulkan instance */
};

class div2_t
{
public:
	struct v6_t :public glm::vec4
	{
		float px, py;
	};
	std::vector<glm::vec4> rcs;
	std::vector<flex_item> rcs_st, c2;
	std::vector<std::vector<v6_t>> childs;
public:
	div2_t();
	~div2_t();
	void set_root(const std::vector<int>& r);
	void set_root_style(size_t idx, const flex_item& it);
	void add_child(size_t idx, const glm::vec2& ss);
	void layout();
	void draw(cairo_t* cr);
private:

};

div2_t::div2_t()
{
}

div2_t::~div2_t()
{
}

void div2_t::set_root(const std::vector<int>& r)
{
	rcs.clear();
	for (auto it : r)
		rcs.push_back({ 0,0,it,0 });
	childs.resize(rcs.size());
	rcs_st.resize(rcs.size());
}

void div2_t::set_root_style(size_t idx, const flex_item& it)
{
	rcs_st[idx] = it;
}

void div2_t::add_child(size_t idx, const glm::vec2& ss)
{
	childs[idx].push_back({ { 0,0, ss},0,0 });
}

void div2_t::layout()
{
	auto length = rcs.size();
	flex_item r1;
	r1.width = 1024;
	r1.height = 0;
	for (size_t x = 0; x < length; x++)
	{
		auto r = &rcs_st[x];
		auto it = rcs[x];
		r->width = it.z;
		r->height = it.w;
		r1.item_add(r);
	}
	std::vector<std::vector<flex_item>> c;
	c.resize(length);
	for (size_t x = 0; x < length; x++)
	{
		auto& v = childs[x];
		auto r = &rcs_st[x];
		r->clear();
		// todo 子元素可以设置更多属性，这里用默认值
		c[x].resize(v.size());
		flex_item* p = c[x].data();
		if (p)
		{
			for (size_t i = 0; i < v.size(); i++)
			{
				//v[i].z += ms.x; v[i].w += ms.y;
				p[i].width = v[i].z;
				p[i].height = v[i].w;
				if (x == 2)
					p[i].grow = 1;
				r->item_add(p + i);
			}
		}
	}
	r1.layout();
	for (size_t x = 0; x < length; x++)
	{
		auto rt = &rcs_st[x];
		auto& it = rcs[x];
		if (rt->position != flex_item::flex_position::POS_ABSOLUTE) {
			it.x = rt->frame[0];
			it.y = rt->frame[1];
		}
		auto& v = childs[x];
		flex_item* p = c[x].data();
		for (size_t i = 0; i < v.size(); i++)
		{
			if (p[i].position != flex_item::flex_position::POS_ABSOLUTE) {
				v[i].x = p[i].frame[0];
				v[i].y = p[i].frame[1];
				v[i].px = p[i].frame[2];
				v[i].py = p[i].frame[3];
			}
		}
	}

}

void div2_t::draw(cairo_t* cr)
{
	auto length = rcs.size();
	for (size_t x = 0; x < length; x++)
	{
		auto it = rcs[x];
		if (it.w <= 0)it.w = 1024;
		//draw_rectangle(cr, { 0.5 + it.x,0.5 + it.y,it.z,it.w }, 4);
		//fill_stroke(cr, 0x10805c42, 0xff0020cC, 1, false);
		auto& v = childs[x];
		auto n = v.size();
		for (size_t i = 0; i < n; i++)
		{
			auto vt = v[i];
			draw_rectangle(cr, { 0.5 + vt.x + it.x,0.5 + vt.y + it.y,vt.px,vt.py }, 4);
			fill_stroke(cr, 0xff805c42, 0xff2C80ff, 1, false);
		}
	}
}
// 渲染树节点
void draw_treenode(cairo_t* cr, layout_text_x* ltx)
{
	std::string text;
	int font_size = 16;
	int text_color = -1;
	auto rk = ltx->get_text_rect(0, text.c_str(), -1, font_size);
	glm::ivec2 ss = { 100,100 };
	glm::vec2 align = { 1,0.5 };
	glm::vec4 rc = { 0, 0, ss };
	ltx->tem_rtv.clear();
	ltx->build_text(0, rc, align, text.c_str(), -1, font_size, ltx->tem_rtv);
	ltx->update_text();
	ltx->draw_text(cr, ltx->tem_rtv, text_color);
}
struct text_render_t
{
	glm::vec4 rc = {};		// 渲染区域
	glm::vec2 align = {};	// 对齐区域
	int font_size = 16;		// 字号大小
	int font = 0;			// 字体集序号
	const char* str = 0;
	int len = 0;			// 字符串长度
	bool autobr = false;	// 自动换行
	bool clip = true;		// 启用裁剪
};

struct node_ts
{
	std::string str;
	tree_node_t* parent = 0;				// 父级
	std::vector<tree_node_t*>* child = 0;	// 孩子 
	int _level = 0;
	bool _expand = 0;
};
void loadtestdata()
{
	auto ed = hz::read_json("ed.json");
	for (auto& [k, v] : ed.items()) {
		printf("%s\n", k.c_str());
	}
}
int main()
{
	// 一格一物：		固体块、墙、气体、液体。种类不到200种
	// 可在气液体重叠：	固体、物件、建筑
	loadtestdata();
	auto qyt = new	uint16_t[256 * 384];
	qyt[0] = -1;
	//return rdx12((HINSTANCE)GetModuleHandle(0), (char*)"", SW_SHOW, "abc");
	//return rvk((HINSTANCE)GetModuleHandle(0), (char*)"", SW_SHOW, "abc");
	glm::ivec2 ws = { 1280,800 };
	auto app = new_app();
	form_newinfo_t ptf = {};
	ptf.app = app; ptf.title = (char*)u8"窗口1";
	ptf.size = ws;
	ptf.flags = ef_vulkan | ef_resizable;
	ptf.has_renderer = true;
	//form_x* form1 = (form_x*)call_data((int)cdtype_e::new_form, &ptf);
	form_x* form0 = (form_x*)call_data((int)cdtype_e::new_form, &ptf);
	ptf.flags = ef_vulkan | ef_transparent | ef_borderless | ef_popup;// | ef_utility;
	ptf.size = { 820,620 };
	ptf.parent = form0;
	form_x* form1 = (form_x*)call_data((int)cdtype_e::new_form, &ptf);
	//form_x* form0 =  app->new_form_renderer(ptf.title, ptf.size, ptf.flags,ptf.has_renderer);
	form1->set_alpha(true);
	//form0->set_alpha(false);
	form0->up_cb = [](float dt, int* rms) {
		//	*rms = 8;
		};
	auto vkdev = form0->get_dev();
	bool bindless = form0->has_variable();
	int cpun = call_data((int)cdtype_e::cpu_count, 0);
	auto pw = form0;
	printf((char*)u8"启动\n cpu核心数量:%d\n", cpun);
	auto pl1 = new plane_cx();
	auto pl2 = new plane_cx();
	auto pl3 = new plane_cx();
	auto pl4 = new plane_cx();
	pl2->_lms = { 6,6 };
	pl1->border = { 0x80ff802C,1,5 };
	pl2->border = { 0x80ff802C,1,5 };
	pl3->border = { 0x80ff802C,1,5 };
	uint32_t pbc = 0x80121212;
	pw->bind(pl3);	// 绑定到窗口
	pw->bind(pl2);	// 绑定到窗口
	pw->bind(pl1);	// 绑定到窗口
	pw->bind(pl4);
	auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman";
	auto fontn2 = (char*)u8"Consolas,新宋体,Times New Roman";
	//fontn = (char*)u8"黑体,Segoe UI Emoji";
	//size_t add_familys(const char* familys, const char* style)
	pl1->add_familys(fontn, 0);
	pl1->add_familys(fontn2, 0);
	pl2->add_familys(fontn, 0);
	pl3->add_familys(fontn, 0);
	//pl2->_css.justify_content = flex_item::flex_align::ALIGN_SPACE_EVENLY;
	//pl3->visible = false;
	//pl2->visible = false;
	//pl1->visible = false; 

	{
		auto p = pl4;
		p->draggable = true; //可拖动
		p->add_familys(fontn, 0);
		p->_lms = { 6,6 };
		p->border = { 0x80ff802C,1,5 };
		p->on_click_outer = [=](plane_cx* p, int state, int clicks) {p->visible = false; };

		p->set_size({ 800,600 });
		p->set_pos({ 0,0 });
		p->set_colors({ pbc,-1,0,0 });
		auto pss = p->get_size();
		int width = 10;
		int rcw = 8;
		{
			// 设置带滚动条
			p->set_scroll(width, rcw, { 0,0 });
		}
		glm::vec2 cs = { 1500,1600 };
		auto vs = p->get_size();
		vs -= 22;
		p->set_view(vs, cs);


		flex_item root;
		auto ss = p->get_size();
		root.width = ss.x - 100;
		root.height = ss.y;
		root.justify_content = flex_item::flex_align::ALIGN_START;
		root.align_content = flex_item::flex_align::ALIGN_START;
		root.align_items = flex_item::flex_align::ALIGN_CENTER;
		root.wrap = flex_item::flex_wrap::WRAP;
		root.direction = flex_item::flex_direction::ROW;

		std::vector<glm::vec4> layouts = { 		 };
		glm::vec2 rsize = { root.width,root.height };
		flex_item* c = flexlayout(&root, layouts, {}, { 6,16 });
		div2_t* div = new div2_t();
		div->set_root({ 200,200,66,500 });
		div->set_root_style(0, root);
		//div->set_root_style(1, root);
		root.justify_content = flex_item::flex_align::ALIGN_END;
		root.direction = flex_item::flex_direction::COLUMN;
		root.align_content = flex_item::flex_align::ALIGN_START;
		div->set_root_style(2, root);
		root.align_content = flex_item::flex_align::ALIGN_START;
		div->set_root_style(3, root);
		div->add_child(2, { 60,60 });
		div->add_child(2, { 60,160 });
		div->add_child(2, { 60,60 });
		div->rcs[2].w = 500;
		div->rcs[3].w = 500;
		for (size_t i = 0; i < 8; i++)
		{
			div->add_child(3, { 60,20 });
		}
		{
			auto& cc = div->childs[2];
			div->c2.resize(cc.size());
			for (size_t i = 0; i < cc.size(); i++)
			{
				div->c2[i].grow = 1;// 320.0 / cc[i].w;
			}
		}
		div->layout();
		{
			auto gb2 = p->add_cbutton((char*)u8"🍑add", { 80,30 }, 0);
			gb2->effect = uTheme::light;
			gb2->hscroll = {};
			gb2->click_cb = [=](void* ptr, int clicks)
				{
					form1->hide();
					div->add_child(0, { 60,60 });
					div->layout();
					auto cbt = p->add_cbutton((char*)u8"🍑new", { 80,30 }, 0);
					cbt->font_size = 16;
					cbt->effect = uTheme::light;
					cbt->pdc;
					cbt->hscroll = {};
					cbt->light = 1;
					cbt->click_cb = [=](void* ptr, int clicks)
						{
							form1->show();
						};
				};
		}
		p->draw_back_cb = [=](cairo_t* cr)
			{
				cairo_as _cas(cr);
				cairo_translate(cr, 6, 50);

				div->draw(cr);
				return;
			};
	}
	// 创建列表视图
	auto listp = new listview_cx();
	listp->border = { 0x80ff802C,1,5 };
	form1->bind(listp);	// 绑定到窗口	
	listp->add_familys(fontn, 0);
	listp->add_familys(fontn2, 0);
	{
		//listp->draggable = true; //可拖动
		listp->set_size({ 800,600 });
		listp->set_pos({ 10,10 });
		listp->set_colors({ 0xff333333,-1,0,0 });
		auto pss = listp->get_size();
		int width = 10;
		int rcw = 8;
		{
			// 设置带滚动条
			listp->set_scroll(width, rcw, { 0,0 });
		}
		column_lv c = {};
		c.width = 100;
		c.title = (char*)u8"名称";
		listp->add_title(c);
		c.title = (char*)u8"状态";
		listp->add_title(c);
		c.title = (char*)u8"描述";
		listp->add_title(c);

		plane_cx* p = listp;
		p->custom_layout = true;
		p->fontsize = 16;

		std::vector<std::string> cstr = { (char*)u8"名 称" ,(char*)u8"状\t态",(char*)u8"描述" };
		std::vector<std::string> cstr1 = { (char*)u8"checkbox 🍇测试1" ,(char*)u8"checkbox 测试2",(char*)u8"checkbox 测试3" };
		std::vector<std::string> cstr2 = { (char*)u8"radio 🍍测试1" ,(char*)u8"radio 测试2",(char*)u8"radio 测试3" };
		width = 150;
		std::vector<color_btn*> cbv = new_label(p, cstr, width, [](void* ptr, int clicks)
			{
				auto pr = (color_btn*)ptr;

			});
		std::vector<checkbox_com> ckv = new_checkbox(p, cstr1, width, [=](void* ptr, bool v)
			{
				auto pr = (checkbox_tl*)ptr;

			});
		std::vector<radio_com> rcv = new_radio(p, cstr2, width, [=](void* ptr, bool v)
			{
				auto pr = (radio_tl*)ptr;
			});
		auto gv = new grid_view();
		gv->set_size(3, 10);
		gv->_pos = { 6,6 };
		for (size_t i = 0; i < 3; i++)
		{
			gv->set_width(i, width + 30);
		}
		for (size_t i = 0; i < 10; i++)
		{
			gv->set_height(i, 30);
		}
		auto cs = gv->get_size();
		auto vs = p->get_size();
		vs -= 22;
		p->set_view(vs, { 1500,1620 });
		for (size_t i = 0; i < cbv.size(); i++)
		{
			auto rc = gv->get({ i,0 });
			auto it = cbv[i];
			it->pos = rc;
			it->pos += 4;
		}
		for (size_t i = 0; i < ckv.size(); i++)
		{
			auto rc = gv->get({ i,1 });
			auto& it = ckv[i];
			it.c->pos = rc;
			it.b->pos = rc;
			it.b->pos.x += it.c->size.x;
			it.c->pos += 4;
			it.b->pos += 4;
		}
		for (size_t i = 0; i < rcv.size(); i++)
		{
			auto rc = gv->get({ i,2 });
			auto& it = rcv[i];
			it.c->pos = rc;
			it.b->pos = rc;
			it.b->pos.x += it.c->size.x;
			it.c->pos += 4;
			it.b->pos += 4;
		}
		svg_cx* bl = new_svg_file("blender_icons.svg", 0, 96);
		svg_cx* bl1 = new_svg_file("button20.svg", 0, 96);
		svg_cx* bl2 = new_svg_file("button21.svg", 0, 96);
		int xn = bl->width * 2 * bl->height * 2;
		auto blsur = new_image_cr({ bl->width * 2,bl->height * 2 });
		auto pxd = (uint32_t*)cairo_image_surface_get_data(blsur);
		if (pxd)
		{
			auto stride = cairo_image_surface_get_stride(blsur) / 4;
			stride *= bl->height * 2;
			for (size_t i = 0; i < xn; i++)
			{
				pxd[i] = 0;
			}
		}
		static int svginc = 0;
		std::thread th([=]()
			{
				print_time a("load svg");
				cairo_t* cr = cairo_create(blsur);
				//render_svg(cr, bl, {}, { 1.0,1.0 }, 0);
				render_svg(cr, bl1, { 0,bl->height + 100 }, { 1.0,1.0 }, 0);
				render_svg(cr, bl2, { 400,bl->height + 100 }, { 1.0,1.0 }, 0);
				svginc = 1;
				p->set_update();
			});
		th.detach();
		p->draw_back_cb = [=](cairo_t* cr)
			{
				cairo_as _cas(cr);
				cairo_translate(cr, 6, 6);
				draw_rectangle(cr, { 0,0,cs.x,cs.y }, 4);
				fill_stroke(cr, 0xf05c8042, 0xffff802C, 2, false);
				draw_ellipse(cr, { 200,200 }, { 120,20 });
				fill_stroke(cr, 0xf0805c42, 0xff0080ff, 2, false);
				draw_ellipse(cr, { 400,200 }, { 120,20 });
				fill_stroke(cr, 0x8ffa2000, 0xff0000ff, 2, false);
				auto ltx = p->ltx;
				if (svginc)
					draw_image(cr, blsur, { 10,320 }, { 0,0,-1,-1 });
				return;
			};
	}





	charts_u1* pcu = new charts_u1();
	pcu->set_color({ 0xff000000,0xffff9e40,1,4 });
	pcu->set_rect({ 0,150,800,300 });
	pcu->set_filltype({ 0,1 });
	auto ct = pcu->get_ct();
	int tk = 1;
	ct->tks = 2;
	ct->width = { tk + 4,tk };
	std::string title;
	{
		hz::mfile_t msh;
		auto tp = msh.open_d("sh938.txt", true);// 读取从通达信导出的历史k线数据

		if (tp)
		{
			const char* t = tp;
			int len = msh.size();
			int nc = 0;
			for (;;)
			{
				auto str = get_linestr(&t, len);
				if (str.empty())break;
				if (nc == 0)title = hz::gbk_to_u8(str);
				if (2 > nc++)
				{
					continue;
				}
				pcu->push_line(str.c_str(), str.size());
			}

		}
	}
	pcu->update();
	{
		pl1->draggable = true; //可拖动
		pl1->set_size({ 830,600 });
		pl1->set_pos({ 100,100 });
		pl1->set_colors({ pbc,-1,0,0 });
		pl1->on_click = [](plane_cx* p, int state, int clicks) {};
		pl1->fontsize = 16;
		int fontsize = 26;
		glm::vec2 text_align = { 0.1,0.1 };
		//txt->add_family("Consolas", 0);
		//txt->add_family((char*)u8"XITS Math", 0);
		//auto xps = txt->add_familys((char*)u8"楷体,新宋体", 0);

		//txt->add_text({ 0 + 2,0 + 2,200,90 }, text_align, u8"↣ ↠ ↦ ↤ → ← ↔ ⇒ ⇐ ⇔\n 𝔹 ℂ 𝔽 ℕ ℙ ℚ ℝ 𝕋 ℤ \nα β χ δ Δ γ Γ ϵ ɛ η \nκ λ Λ μ ν ω Ω ϕ φ Φ \nπ Π ψ Ψ ρ σ Σ τ θ ϑ Θ υ \nξ Ξ ζ 𝔸 𝐀 𝔄 𝕬 𝐴 𝑨", -1, 18);
		//txt->clear_family();
		//txt->add_family("Consolas", 0);
		text_align = { 0.0,0.1 };
		auto txt = pl1->ltx;
		glm::vec4 rcc1 = { 0 + 2,0 + 2,260,90 };
		txt->add_text(0, rcc1, { 0,0.5 }, (char*)u8"🍑🍍🌶🍆abcg", -1, 60);
		txt->update_text();
		auto et1 = pl1->add_input("", { 100,22 }, true);
		glm::vec2 bs = { 50,22 };
		et1->set_pos({ 10,10 });


		pl1->add_switch(bs, "", true);
		pl1->add_switch(bs, "", false);
		auto sw1 = (switch_tl*)pl1->add_switch(bs, (char*)u8"开", true);
		auto sw2 = (switch_tl*)pl1->add_switch(bs, "k", false);
		bs.x = 16;
		bs.y = 16;
		{
			auto ck1 = pl1->add_checkbox(bs, "cc", false);
			bs.x = 90;
			bs.y = 30;
			auto kcb = pl1->add_label((char*)u8"🌽玉米", bs, 0);
			kcb->click_cb = [=](void* ptr, int clicks) {
				ck1->set_value();
				};
			ck1->v.on_change_cb = [=](void* p, bool v) {
				//pmodal->visible = v;
				};
		}
		bs.x = 16;
		bs.y = 16;
		{
			auto ck1 = pl1->add_checkbox(bs, "cc1", false);
			bs.x = 90;
			bs.y = 30;
			auto kcb = pl1->add_label((char*)u8"g🍖🌶🌶", bs, 0);
			kcb->click_cb = [=](void* ptr, int clicks) {
				ck1->set_value();
				};
		}
		auto gr = new group_radio_t();
		bs.x = 16;
		bs.y = 16;
		auto r1 = pl1->add_radio(bs, "cc2", true, gr);
		{
			bs.x = 90;
			bs.y = 30;
			auto kcb = pl1->add_label((char*)u8"🍇葡萄", bs, 0);
			kcb->click_cb = [=](void* ptr, int clicks) {
				r1->set_value();
				};

		}
		bs.x = 16;
		bs.y = 16;
		auto r2 = pl1->add_radio(bs, "cc3", true, gr);
		{
			bs.x = 90;
			bs.y = 30;
			auto kcb = pl1->add_label((char*)u8"🥝猕猴桃", bs, 0);
			kcb->click_cb = [=](void* ptr, int clicks) {
				r2->set_value();
				};

		}
		std::vector<progress_tl*> prv;
		{
			auto pro = pl1->add_progress("%", { 100,30 }, 0.06);
			pro->rounding = 10;
			prv.push_back(pro);
		}
		{
			auto pro = pl1->add_progress("%", { 100,20 }, 0.125);
			pro->rounding = 10;
			prv.push_back(pro);
		}
		{
			auto pro = pl1->add_progress("%", { 100,20 }, 0.5);
			pro->rounding = 10;
			prv.push_back(pro);
		}
		{
			auto pro = pl1->add_progress("%", { 100,20 }, 1);
			pro->rounding = 10;
			prv.push_back(pro);
		}
		{
			auto pro = pl1->add_progress("%", { 160,20 }, 0);
			pro->rounding = 10;
			pro->text_inside = false;
			pro->set_value(0.8);
			prv.push_back(pro);
		}
		{
			auto pro = pl1->add_progress("%", { 160,20 }, 0);
			pro->rounding = 10;
			pro->text_inside = false;
			pro->set_value(0.98);
			prv.push_back(pro);
		}
		{
			auto pro = pl1->add_progress("%", { 160,20 }, 0);
			pro->rounding = 10;
			pro->right_inside = true;
			pro->set_value(0.68);
			prv.push_back(pro);
		}
		{
			auto pro = pl1->add_progress("%", { 160,26 }, 0);
			pro->rounding = 12;
			pro->right_inside = true;
			pro->set_value(1);
			prv.push_back(pro);
		}
		{
			auto cp = pl1->add_colorpick(0, 250, 20, true);
			cp->font_size = 16;
			cp->on_change_cb = [=](colorpick_tl* p, uint32_t col)
				{
					for (auto it : prv) {
						it->color.x = col;
					}
				};
			cp->init(0, 250, 20, true);
			cp->set_hsv({ 0.62,1,0.91,0.68 });
		}
		bs = { 200, 12 };
		{
			auto slider = pl1->add_slider(bs, 7, 0.2);
			slider->sl.y = -1;
			slider->thickness = 2;
			slider->rounding = 3;
		}
		{
			bs = { 12, 200 };
			auto slider = pl1->add_slider(bs, 7, 0.2);
			slider->sl.y = 0xff3030f8;
			slider->thickness = 2;
			slider->rounding = 3;
		}
		{
			bs = { 200, 12 };
			auto slider = pl1->add_slider(bs, 7, 0.2);
			slider->sl.y = 0xff3030f8;
			slider->thickness = 2;
			slider->rounding = 3;
			slider->reverse_color = 1;
		}
		{
			bs = { 12, 200 };
			auto slider = pl1->add_slider(bs, 7, 0.2);
			slider->sl.y = 0xff3030f8;
			slider->thickness = 2;
			slider->rounding = 3;
			slider->reverse_color = 1;
		}
		auto pss = pl1->get_size();

		int width = 10;
		int rcw = 8;
		{
			// 设置带滚动条
			pl1->set_scroll(width, rcw, { 0,0 });
		}

		sw1->color = { 0xff66ce13, 0xff4949ff ,-1 };
		sw2->color = { 0xff66ce13, 0xff4949ff ,-1 };
		//sw2->text_color = {};
		pl1->_css.align_items = flex_item::flex_align::ALIGN_CENTER;

		pl1->update_cb = [=](float delta)
			{

				return 0;
			};
		pl1->draw_back_cb = [=](cairo_t* cr)
			{
				cairo_as _cas(cr);
				//cairo_translate(cr, 0, 300);
				//txt->draw_text(cr, 0xff0080ff);
				return;
				if (0) {
					cairo_as _cas(cr);
					cairo_translate(cr, 50.5, 40.5);
					static std::vector<glm::vec2> ptv = {};
					ptv = { {0,0},{0,150},{150,150},{150,0} };
					glm::vec2 sc = { 0.264583319 ,0.264583319 }, bs = { 1.0,1.0 };
					int ddot = 1;
					scale_pts(ptv.data(), ptv.size(), sc, ddot, 0);
					draw_pts(cr, ptv, 0xffff802C);// 渲染路径线

					sc = bs / sc;
					scale_pts(ptv.data(), ptv.size(), sc, ddot, 0);
					draw_pts(cr, ptv, 0xff80ff2C);

					sc = bs / sc;
					scale_pts(ptv.data(), ptv.size(), sc, ddot, 0);
					draw_pts(cr, ptv, 0xff0080ff);
				}

				//return;
				if (0) {
					cairo_as _cas(cr);
					cairo_translate(cr, 150, 150);
					glm::vec4 t = { 1,1,1,1 };
					glm::vec4 t1 = { 3,2,1,1 };
					auto v1 = draw_r(cr, t, 90);
					auto v2 = draw_r(cr, t1, 90);
					v1 *= 6;
					v2 *= 6;
					cairo_move_to(cr, v1.x, v1.y);
					cairo_line_to(cr, v2.x, v2.y);
					fill_stroke(cr, 0, -1, 1, false);
					auto tk = t;
					auto tk1 = t1;
					tk *= 6;
					tk1 *= 6;
					cairo_move_to(cr, tk.x, tk.y);
					cairo_line_to(cr, tk1.x, tk1.y);
					fill_stroke(cr, 0, 0xff0000ff, 1, false);

					draw_r(cr, t, 90);
					draw_r(cr, t1, 90);
					glm::vec2 tt = { 0,0 };
					draw_circle(cr, tt, 2);
					fill_stroke(cr, 0xff0080ff, 0, 1, false);
				}
				{
					cairo_as _cas(cr);
					cairo_translate(cr, 10, 50);
					draw_rectangle(cr, { 0.5,0.5,260 + 4,90 + 4 }, 4);
					fill_stroke(cr, 0x80805c42, 0xffff802C, 1, false);
					draw_rectangle(cr, { 330.5,0.5,400 + 4,90 + 4 }, 4);
					fill_stroke(cr, 0xff422e21, 0xffff802C, 1, false);

					draw_rectangle(cr, { 0,0,260 ,90 }, 4);
					draw_rectangle(cr, { 330.5,0.5,400 + 4,90 + 4 }, 4);
					draw_rectangle(cr, { 0,150,800,300 }, 4);
					cairo_clip(cr);
					cairo_new_path(cr);
					txt->draw_text(cr, -1);
					pcu->cidx = std::atoi(et1->_text.c_str());
					pcu->update_draw(cr);
				}
			};
	}
	{
		pl3->draggable = true; //可拖动
		pl3->set_size({ 1400,600 });
		pl3->set_pos({ 10,10 });
		pl3->set_colors({ 0xff000000,-1,0,0 });
		pl3->on_click = [](plane_cx* p, int state, int clicks) {};
		pl3->update_cb = [=](float delta)
			{

				return 0;
			};
		pl3->draw_back_cb = [=](cairo_t* cr)
			{
				int y1 = 10;
				cairo_as _ss_(cr);

				draw_triangle(cr, { 100,100 }, { 8,8 }, { 0,1 });
				fill_stroke(cr, -1, 0);
				draw_triangle(cr, { 100.5,120.5 }, { 4.5,9 }, { 1,0.5 });
				fill_stroke(cr, 0, -1, 1);


				cairo_translate(cr, 10, 200);
				auto txt = pl1->ltx;
				//txt->update_text();
				for (auto it : txt->msu)
				{
					auto ss = draw_image(cr, it, { 10, y1 }, { 0,0,1024,512 });
					y1 += ss.y + 10;
				}

			};
	}
	{
		pl2->draggable = true; //可拖动*
		pl2->set_size({ 830,600 });
		pl2->set_pos({ 10,10 });
		pl2->set_colors({ pbc,-1,0,0 });
		pl2->on_click = [](plane_cx* p, int state, int clicks) {};
		pl2->draw_back_cb = [=](cairo_t* cr)
			{
			};
		int cc = 1;
		int bc = 0x80ff8222;
		edit_tl* et1, * et2;
		{
			pl2->set_family_size((char*)u8"NSimSun", 16, -1);// 按钮和edit字号标准不同
			auto gb2 = pl2->add_cbutton((char*)u8"🍑图库目录", { 80,30 }, 0);
			gb2->effect = uTheme::light;
			gb2->light = 0.2 * 0;
			gb2->_disabled_events = true;
			gb2->pdc;
			pl2->set_family_size((char*)u8"NSimSun,Segoe UI Emoji", 12, -1);
			et1 = pl2->add_input("", { 400,30 }, true);
			et2 = pl2->add_input("", { 620,300 }, false);
			et2->set_autobr(true);
		}
		pl2->set_family_size((char*)u8"NSimSun", 16, -1);// 按钮和edit字号标准不同
		{
			{
				njson bf = hz::read_json("temp/bfinfo.json");
				std::string ph;
				if (bf.is_object() && bf.find("folder") != bf.end())
				{
					ph = (bf["folder"]);
					et1->set_text(ph.c_str(), ph.size());
				}
			}
			auto gb2 = pl2->add_gbutton((char*)u8"生成索引", { 100,30 }, 0);

			gb2->click_cb = [=](void* ptr, int clicks)
				{
					njson bf = hz::read_json("temp/bfinfo.json");
					std::string ph;
					if (bf.is_object() && bf.find("folder") != bf.end())
					{
						ph = hz::u8_to_gbk(bf["folder"]);
					}

					auto p = (gradient_btn*)ptr;

					auto fpath = et1->_text.size() ? hz::u8_to_gbk(et1->_text) : hz::browse_folder(ph, "选择图片文件夹");
					if (fpath.size())
					{
						double ss = 0.0;
						int cn = 0;
						// todo
						std::string str = (char*)u8"成功添加" + std::to_string(cn) + (char*)u8"张图片，耗时(秒):" + pg::to_string(ss) + "\n";
						et2->add_text(str.c_str(), str.size());
						fpath += "\n";
						et2->add_text(fpath.c_str(), fpath.size());
					}
					else {
						std::string str = (char*)u8"路径打开失败！请重新输入路径。\n";
						et2->add_text(str.c_str(), str.size());
					}
				};
		}
		{
			auto gb2 = pl2->add_gbutton((char*)u8"清空索引", { 100,30 }, bc);

			gb2->click_cb = [=](void* ptr, int clicks)
				{
					auto p = (gradient_btn*)ptr;
					if (p)
					{

					}
				};
		}
		{
			auto gb2 = pl2->add_gbutton((char*)u8"清空输入框1", { 100,30 }, 0x802282ff);

			gb2->click_cb = [=](void* ptr, int clicks)
				{
					auto p = (gradient_btn*)ptr;
					if (p)
					{
						et1->set_text(0, 0);
						printf((char*)u8"点击数量：%d\n", clicks);
					}
				};
			//	gb2->text_color = 0xff2222ff;
		}
		{
			auto gb2 = pl2->add_gbutton((char*)u8"搜索", { 100,30 }, bc);

			//	gb2->text_color = 0xffff0022;
			gb2->click_cb = [=](void* ptr, int clicks)
				{
					auto p = (gradient_btn*)ptr;
					if (p)
					{
						njson bf = hz::read_json("temp/bfinfo.json");
						std::string ph;
						if (bf.is_object() && bf.find("imagefn") != bf.end())
						{
							ph = hz::u8_to_gbk(bf["imagefn"]);
						}

						std::string filter;
						auto fn = hz::browse_openfile("选择图像文件", "", "所有文件\t*.*\tPNG格式(*.png)\t*.png\tJPEG(*.jpg)\t*.jpg\t", 0, 0);
						if (fn.size()) {
							std::string str;
							for (auto& it : fn)
							{
								str += it + "\n";
							}
							et2->add_text(str.c_str(), str.size());
						}
					}
				};
		}
		{
			auto gb2 = pl2->add_gbutton((char*)u8"清空输入框2", { 100,30 }, 0x802222ff);

			gb2->click_cb = [=](void* ptr, int clicks)
				{
					auto p = (gradient_btn*)ptr;
					if (p)
					{
						et2->set_text(0, 0);
						printf((char*)u8"点击数量：%d\n", clicks);
					}
				};
		}
		pl2->move2end(et2);
		pl2->mk_layout();
	}
	run_app(app, 0);
	free_app(app);
	return 0;
}
