/*
	Copyright (c) 华仔
	188665600@qq.com
	创建时间2024-07-10

	页面管理
*/
#include "pch1.h"
#include "page.h"
#include "pnguo.h"
#include "tinysdl3.h"

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

	auto ft = div->ltx;
	auto rc = ft->get_text_rect(0, str.c_str(), str.size(), bc->fonst_size);
	auto h = ft->get_lineheight(0, bc->fonst_size);
	div->set_colors({ 0xaa000000,-1,0,0 });
	auto drc = rc;
	drc += h;
	div->set_size(drc);
	auto lp = div->add_label(str, rc, 0);
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
