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
	div->border = { 0x80aaaaaa,1,0 };
	auto ft = div->ltx;
	auto rc = ft->get_text_rect(0, str.c_str(), str.size(), bc->fonst_size);
	auto h = ft->get_lineheight(0, bc->fonst_size);
	div->set_colors({ 0xff000000,-1,0,0 });
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
	else {
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
	if (p) {
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
