
#include <pch1.h>
#include <vkgui/pnguo.h>
#include <vkgui/tinysdl3.h>
#include <vkgui/mapView.h>

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
int main()
{
	// 一格一物：		固体块、墙、气体、液体。种类不到200种
	// 可在气液体重叠：	固体、物件、建筑

	auto qyt = new	uint16_t[256 * 384];
	qyt[0] = -1;
	//return rdx12((HINSTANCE)GetModuleHandle(0), (char*)"", SW_SHOW, "abc");
	//return rvk((HINSTANCE)GetModuleHandle(0), (char*)"", SW_SHOW, "abc");
	glm::ivec2 ws = { 1280,800 };
	auto app = new_app();
	form_newinfo_t ptf = { app,(char*)u8"窗口1",ws, ef_vulkan | ef_resizable/* | ef_borderless*/,true };
	//form_x* form1 = (form_x*)call_data((int)cdtype_e::new_form, &ptf);
	form_x* form0 = (form_x*)call_data((int)cdtype_e::new_form, &ptf);
	//form_x* form0 =  app->new_form_renderer(ptf.title, ptf.size, ptf.flags,ptf.has_renderer);
	//form0->set_alpha(true);
	//form0->set_alpha(false);
	auto vkdev = form0->get_dev();
	bool bindless = form0->has_variable();
	int cpun = call_data((int)cdtype_e::cpu_count, 0);
	auto pw = form0;
	printf((char*)u8"启动\n cpu核心数量:%d\n", cpun);
	auto pl1 = new plane_cx();
	auto pl2 = new plane_cx();
	auto pl3 = new plane_cx();
	pl1->border = { 0x80ff802C,1,5 };
	pl2->border = { 0x80ff802C,1,5 };
	pl3->border = { 0x80ff802C,1,5 };
	pw->bind(pl3);	// 绑定到窗口
	pw->bind(pl2);	// 绑定到窗口
	pw->bind(pl1);	// 绑定到窗口
	//pl2->_css.justify_content = flex_item::flex_align::ALIGN_SPACE_EVENLY;
	pl3->visible = false;
	//pl1->visible = false;


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
		pl1->set_colors({ 0xff121212,-1,0,0 });
		pl1->on_click = [](plane_cx* p, int state, int clicks) {};

		int fontsize = 26;
		glm::vec2 text_align = { 0.1,0.1 };
		//txt->add_family("Consolas", 0);
		//txt->add_family((char*)u8"XITS Math", 0);
		//auto xps = txt->add_familys((char*)u8"楷体,新宋体", 0);
		glm::vec4 rcc1 = { 0 + 2,0 + 2,260,90 };
		//txt->add_text({ 0 + 2,0 + 2,200,90 }, text_align, u8"↣ ↠ ↦ ↤ → ← ↔ ⇒ ⇐ ⇔\n 𝔹 ℂ 𝔽 ℕ ℙ ℚ ℝ 𝕋 ℤ \nα β χ δ Δ γ Γ ϵ ɛ η \nκ λ Λ μ ν ω Ω ϕ φ Φ \nπ Π ψ Ψ ρ σ Σ τ θ ϑ Θ υ \nξ Ξ ζ 𝔸 𝐀 𝔄 𝕬 𝐴 𝑨", -1, 18);
		//txt->clear_family();
		//txt->add_family("Consolas", 0);
		auto pxps = pl1->add_familys((char*)u8"新宋体,Segoe UI Emoji", 0);
		text_align = { 0.0,0.1 };
		auto txt = pl1->ltx;
		txt->add_text(0, rcc1, { 0,0.5 }, (char*)u8"🍑🍍🍆abcg", -1, 60);
		auto et1 = pl1->add_input("", { 100,22 }, true);
		glm::vec2 bs = { 50,22 };
		et1->set_pos({ 10,10 });
		pl1->add_switch(bs, "", true);
		pl1->add_switch(bs, "", false);
		auto sw1 = (switch_tl*)pl1->add_switch(bs, (char*)u8"开", true);
		auto sw2 = (switch_tl*)pl1->add_switch(bs, "k", false);
		pl1->add_checkbox(bs, "cc", false);
		pl1->add_checkbox(bs, "cc1", true);
		pl1->add_radio(bs, "cc2", false);
		pl1->add_radio(bs, "cc3", true);
		sw1->color = { 0xff66ce13, 0xff4949ff ,-1 };
		sw2->color = { 0xff66ce13, 0xff4949ff ,-1 };
		//sw2->text_color = {};
		pl1->_css.align_items = flex_item::flex_align::ALIGN_CENTER;
		pl1->mk_layout();
		pl1->update_cb = [=](float delta)
			{

				return 0;
			};
		pl1->draw_cb = [=](cairo_t* cr)
			{
				cairo_as _cas(cr);
				cairo_translate(cr, 0, 10);
				txt->draw_text(cr, -1);
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
		pl3->set_colors({ 0xff121212,-1,0,0 });
		pl3->on_click = [](plane_cx* p, int state, int clicks) {};
		pl3->draw_cb = [=](cairo_t* cr)
			{
				int y1 = 10;

				cairo_save(cr);
				//for (auto it : txt->msu)
				//{
				//	auto ss = draw_image(cr, it, { 10, y1 }, { 0,0,1024,512 });
				//	y1 += ss.y + 10;
				//}
				cairo_restore(cr);
			};
	}
	{
		pl2->draggable = true; //可拖动*
		pl2->set_size({ 830,600 });
		pl2->set_pos({ 10,10 });
		pl2->set_colors({ 0xff121212,-1,0,0 });
		pl2->on_click = [](plane_cx* p, int state, int clicks) {};
		pl2->draw_cb = [=](cairo_t* cr)
			{
			};
		int cc = 1;
		int bc = 0x80ff8222;
		edit_tl* et1, * et2;
		{
			pl2->set_family_size((char*)u8"NSimSun", 16, -1);// 按钮和edit字号标准不同
			auto gb2 = pl2->add_cbutton((char*)u8"图库目录", { 80,30 }, 0);
			gb2->effect = uTheme::light;
			gb2->light = 0.2 * 0;
			gb2->_disabled_events = true;
			gb2->pdc;
			pl2->set_family_size((char*)u8"NSimSun,Segoe UI Emoji", 12, -1);
			et1 = pl2->add_input("", { 400,30 }, true);
			et2 = pl2->add_input("", { 620,300 }, false);
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
						std::string str = (char*)u8"成功添加" + std::to_string(cn) + (char*)u8"张图片，耗时(秒):" + to_string(ss) + "\n";
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
