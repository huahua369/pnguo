/*
	Copyright (c) 华仔
	188665600@qq.com
	创建时间2024-07-10

	页面管理
*/
#include "pch1.h"
#include "pnguo.h"
#include "tinysdl3.h"
#include "page.h"

#include "buffer.h"

#if 1
void show_tooltip(form_x* form, const std::string& str, const glm::ivec2& pos, style_tooltip* bc)
{
	if (!form || !bc || str.empty())return;
	auto div = new plane_cx();
	div->set_fontctx(form->app->font_ctx);
	div->add_familys(bc->family, 0);
	div->fontsize = bc->fonst_size;
	div->_css.align_content = flex_item::flex_align::ALIGN_CENTER;
	div->_css.justify_content = flex_item::flex_align::ALIGN_CENTER;
	div->_css.align_items = flex_item::flex_align::ALIGN_CENTER;

	div->_lpos = { 0,0 }; div->_lms = { 0,0 };
	//div->_lms = { 6,6 };
	div->border = { bc->color.y,bc->thickness,bc->radius,bc->color.x };
	auto ft = div->ltx;
	auto rc = ft->get_text_rect(0, str.c_str(), str.size(), bc->fonst_size);
	auto h = ft->get_lineheight(0, bc->fonst_size);
	div->set_clear_color(0);
	auto drc = rc;

	drc += h;
	div->set_size(drc);
	drc += h;
	auto lp1 = div->add_label(str, rc, 0);
	//auto lp = div->add_cbutton(str, rc, 0);
	if (!form->tooltip)
		form->tooltip = new_form_tooltip(form, drc.x, drc.y);
	else {
		form->tooltip->clear_wt();
		form->tooltip->set_size(drc);
	}
	form->tooltip->bind(div);
	form->tooltip->show();
	form->tooltip->set_pos(pos);
}

void hide_tooltip(form_x* form)
{
	if (form && form->tooltip)form->tooltip->hide();
}



mitem_t::mitem_t()
{
	backgs = new canvas_atlas();
	fronts = new canvas_atlas();
	ltx = new layout_text_x();
}

mitem_t::~mitem_t()
{
	if (f)
		f->close();
	f = 0;
	if (backgs)delete backgs; backgs = 0;
	if (fronts)delete fronts; fronts = 0;
	if (ltx)delete ltx; ltx = 0;
}

void mitem_t::show(const glm::vec2& ps)
{
	if (pv.p) {
		auto cv = pv.p->widgets;
		auto length = cv.size();
		for (size_t i = 0; i < length; i++)
		{
			auto it = cv[i];
			if (it)
				it->bst = (int)BTN_STATE::STATE_NOMAL;
		}
	}
	pos = ps;
	if (f) {
		f->set_pos(pos);
		f->show();
	}
	else if (m) {
		m->show_item(this, pos);
	}

}

void mitem_t::hide(bool hp)
{
	if (f)
		f->hide();
	else {

	}
	if (parent && hp)
		parent->hide(1);
}

void mitem_t::close()
{
	if (f)
		f->close();
	f = 0;
}

void mitem_t::set_data(int w, int h, const std::vector<std::string>& mvs)
{
	width = w;
	height = h;
	v = mvs;
	backgs->remove_atlas(pv.back);
	fronts->remove_atlas(pv.front);
	ltx->free_menu(pv);
	auto p = this;
	pv = ltx->new_menu(width, height, mvs, [=](int type, int idx)
		{
			if (ckm_cb)
				ckm_cb(p, type, idx);
		});
	backgs->add_atlas(pv.back);
	fronts->add_atlas(pv.front);
}

glm::ivec2 mitem_t::get_idx_pos(int idx)
{
	auto ps = pos;
	ps.x += pv.w + pv.cpos.x + pv.p->border.y;
	ps.y += idx * pv.h;
	return ps;
}

menu_cx::menu_cx()
{
}

menu_cx::~menu_cx()
{
}
void menu_cx::set_main(form_x* f)
{
	form = f;
}
void menu_cx::add_familys(const char* family)
{
	if (family && *family)
		familys.push_back(family);
}
mitem_t* menu_cx::new_menu(int width, int height, const std::vector<std::string>& mvs, std::function<void(mitem_t* p, int type, int id)> cb)
{
	auto p = new mitem_t();
	if (p && form) {
		p->m = this;
		p->ckm_cb = cb;
		p->ltx->set_ctx(form->app->font_ctx);
		for (auto it : familys) {
			p->ltx->add_familys(it.c_str(), 0);
		}
		p->set_data(width, height, mvs);
	}
	return p;
}
void menu_cx::show_item(mitem_t* it, const glm::vec2& pos)
{
	if (!form)return;
	auto mf1 = it->f ? it->f : new_form_popup(form, it->pv.fsize.x, it->pv.fsize.y);
	if (it->f)
	{
		it->f->remove(it->backgs);
		it->f->remove(it->fronts);
		it->f->unbind(it->pv.p);
	}
	it->f = mf1;
	mf1->add_canvas_atlas(it->backgs);
	mf1->bind(it->pv.p);
	mf1->add_canvas_atlas(it->fronts);
	mf1->set_size(it->pv.fsize);
	mf1->set_pos(pos);
	mf1->show();
}

void menu_cx::free_item(mitem_t* p)
{
	if (p)
		delete p;
}
#endif

// todo 列表视图
#if 1

list_box_cx::list_box_cx()
{
}

list_box_cx::~list_box_cx()
{
}

dialog_cx::dialog_cx()
{
}

dialog_cx::~dialog_cx()
{
}

#endif // 1

#if 1

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
struct input_info_t
{
	glm::ivec2 pos;
	glm::ivec2 pos1;
	std::string str;
};
// set_data、remove、insert、clear_redo、undo、redo
void test()
{
	auto buf = new hz::buffer_t();
	std::vector<input_info_t> _iit;// 操作列表
	buf->set_data("abc", -1);
	int cuinc = 0;
	auto ic = cuinc;
	bool single_line = false;
	glm::ivec2 r = {};
	for (auto& it : _iit)
	{
		if (it.pos > it.pos1)
		{
			std::swap(it.pos, it.pos1);
		}
		if (it.str.empty())
		{
			// 执行删除
			buf->remove(it.pos, it.pos1);
			r = it.pos; cuinc++;
			//printf("\t光标d:%d\t%d\t%d\n", (int)it.str.size(), r.x, r.y);
		}
		else {
			// 插入文本
			if (single_line)
			{
				std::remove(it.str.begin(), it.str.end(), '\n');
			}
			//if (tvt && tvt->on_input)
			//	tvt->on_input(&it.str);// 执行回调函数

			if (it.str.empty())
			{
				r = it.pos;
			}
			else {
				r = buf->insert(it.pos, it.str.c_str(), it.str.size(), &it.pos1);// 插入文本
			}
			cuinc++;
			//printf("\t光标:%d\t%d\t%d\n", (int)it.str.size(), r.x, r.y);
		}
	}
	if (ic != cuinc)
	{
		buf->clear_redo(); // 清空重做栈
	}
	glm::ivec2 cp1 = { 0,0 }, cp2 = { 0,2 };
	// 获取选中文本
	auto str = buf->get_range(cp1, cp2);
}
#endif // 1
