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

// todo 树
#if 0
		// 去掉头尾空格
std::string trim(const std::string& str, const char* pch)
{
	auto s = str;
	if (s.empty())
	{
		return s;
	}
	s.erase(0, s.find_first_not_of(pch));
	s.erase(s.find_last_not_of(pch) + 1);
	return s;
}
std::string trim_ch(const std::string& str, const std::string& pch)
{
	std::string r = str;
	if (pch.size() && str.size())
	{
		size_t p1 = 0, p2 = str.size();
		for (int i = 0; i < str.size(); i++)
		{
			auto ch = str[i];
			if (pch.find(ch) == std::string::npos)
			{
				p1 = i;
				break;
			}
		}
		for (int i = str.size() - 1; i > 0; i--)
		{
			auto ch = str[i];
			if (pch.find(ch) == std::string::npos)
			{
				p2 = i + 1;
				break;
			}
		}
		r = str.substr(p1, (p2 - p1));
	}
	return r;
}
static uint64_t toUInt(const njson& v, uint64_t de = 0)
{
	uint64_t ret = de;
	if (v.is_number())
	{
		ret = v.get<uint64_t>();
	}
	else if (!v.is_null())
	{
		ret = std::atoll(trim(v.dump(), "\"").c_str());
	}
	return ret;
}

njson& push_button(const void* str, int cidx, int eid, njson& btn)
{
	njson it;
	it["s"] = (char*)(str ? str : "");
	it["cidx"] = cidx;
	it["eid"] = eid;
	btn.push_back(it);
	return *btn.rbegin();
}
njson& push_button(const void* str, int cidx, int eid, njson& btn, glm::ivec2 size, int fh)
{
	njson it;
	it["s"] = (char*)(str ? str : "");
	it["cidx"] = cidx;
	it["eid"] = eid;
	it["fontheight"] = fh;
	it["size"] = { size.x,size.y };
	btn.push_back(it);
	return *btn.rbegin();
}


void set_moveto(njson& v, glm::vec2 target, glm::vec2 from, float mt, float wait)
{
	njson& t = v;
	t["mt"] = mt;
	t["wait"] = wait;
	t["from"] = { from.x, from.y };
	t["target"] = { target.x,target.y };
	t.erase("pad");
}
void set_moveto(njson& v, float mt, glm::vec2 target, float wait)
{
	njson& t = v;
	t["mt"] = mt;
	t["wait"] = wait;
	t["target"] = { target.x,target.y };
	t.erase("pad");
}
void set_moveto(njson& v, glm::vec2 pad, float mt, float wait)
{
	njson& t = v;
	t["mt"] = mt;
	t["wait"] = wait;
	t["pad"] = { pad.x,pad.y };
	t.erase("target");

}
void set_ps(njson& v, glm::vec2 size, glm::vec2 pos, bool is)
{
	v["pos"] = { pos.x,pos.y };
	v["size"] = { size.x,size.y };
	if (is)
		v["on"] = is;
	else
		v.erase("on");
}
void freecb(njson* p)
{
	if (p)
	{
		njson& c = *p;
		auto cb = (std::function<void(int, njson*)>*)toUInt(c[".click"]);
		if (cb)
		{
			delete cb;
		}
		delete p;
	}
}

template<class T>
T* get_ptr0(world_cx* ctx, const char* gid, int idx, int type)
{
	T* p = 0;
	if (ctx && gid && *gid)
	{
		auto u = ctx->get_ptr2(gid, type);
		if (u.n)
		{
			if (idx < 0 || idx > u.n)idx = 0;
			p = (T*)u.p[idx];
		}
	}
	return p;
}
inline ui::scroll_view_u* get_svptr(world_cx* c, const char* gid, int idx)
{
	return get_ptr0<ui::scroll_view_u>(c, gid, idx, 1);
}
inline ui::div_u* get_divptr(world_cx* c, const char* gid, int idx)
{
	return get_ptr0<ui::div_u>(c, gid, idx, 0);
}

ui::scroll_view_u* world_cx::get_svptr(const char* gid, int idx) {
	return get_ptr0<ui::scroll_view_u>(this, gid, idx, 1);
}
ui::div_u* world_cx::get_divptr(const char* gid, int idx) {
	return get_ptr0<ui::div_u>(this, gid, idx, 0);
}

#define merge_j(dst,src) 	do{for (auto& [k, v] : src.items())	{ dst[k] = v;	}}while(0)
/*
	菜单开关动画实现，通过gid操作坐标，修改g的参数返回到动画处理系统执行
	idx=操作的index，sp为间隔{x子项，y父级}，mt为移动到目标毫秒数，one=true则关闭所有，只展开一个父项。
	vsize返回展开后大小，方便设置滚动视图
*/
bool get_switch(njson& g, int idx, glm::vec3 sp, float mt, bool one, glm::vec2* vsize)
{
	bool ret = false;
	float wait = 0.0, sp3 = sp.z;
	glm::vec2 t = { sp.x,sp.y }, vsize0 = {};
	if (!vsize)vsize = &vsize0;
	do {
		if (idx >= g.size() && idx >= 0) { break; }
		glm::vec2 pos = toVec2(g[0]["pos"]);
		for (size_t x = 0; x < g.size(); x++) {
			auto& vt = g[x]; auto& v1 = vt["v"];
			bool oc = vt.find("on") != vt.end() ? toBool(vt["on"]) : false;
			if (one && x != idx) { vt["on"] = false; oc = false; }
			if (x == idx) { oc = !oc; vt["on"] = oc; }
			for (size_t i = 0; i < v1.size(); i++) {
				auto& kt = v1[i];
				glm::vec2 c0 = toVec2(kt["size"]);
				if (i > 0) {
					if (!oc) {
						bool visi = toBool(kt["visible"]);
						if (visi) {
							kt["wait0"] = 0; kt["visible"] = false;	// 隐藏子项
							glm::vec2 ps1 = { -c0.x * 3, 0 };
							set_moveto(kt, ps1, mt, wait);	// 移动子项到外面
						}
						continue;
					}
					if (one || x == idx) { if (oc) { kt["wait0"] = 0; kt["visible"] = true; } }
				}
				vsize->y += c0.y + t.x;
				pos.y += t.x;
				auto ps1 = pos;
				if (i > 0) { ps1.x += sp3; }
				vsize->x = std::max(vsize->x, c0.x + ps1.x);
				set_moveto(kt, mt, ps1, wait);	// 移动到目标
				pos.y += c0.y;
			}
			pos.y += t.y;
			vsize->y += t.y;
		}
		ret = true;
	} while (0);
	save_json(g, "temp/tr1758.json", 2);
	return ret;
}

void on_navclick(uint64_t idx, void* p)
{
	auto d = (njson*)p;
	if (d)
	{
		auto& n = *d;
		auto ctx = (world_cx*)toUInt(n["$ctx"]);
		auto& at = n["$at"];
		auto& b = n["$base"];
		glm::vec2 vsize;
		auto gid = toStr(b["gid"]);
		auto asize = toVec4(b["asize"]);
		auto sp = toVec4(b["sp"]);
		auto emt = toFloat(b["expmt"]);
		auto xtype = toBool(b["xtype"]); // 单个展开
		// 获取开关动画参数
		if (get_switch(at, idx, sp, sp.w, xtype, &vsize))
		{
			if (ctx)
			{
				auto sv = get_svptr(ctx, gid.c_str(), 0);
				auto dp = get_divptr(ctx, gid.c_str(), 0);
				if (ctx)
					ctx->push_action(at);	// 提交动画执行

				if (idx == 0 && dp)
				{
					bool on1 = toBool(at[idx]["on"]);
					glm::vec2 ss = {};
					if (on1)
					{
						ss = { asize.z,asize.w };
					}
					else {
						ss = { asize.x,asize.y };
					}
					dp->at_size(ss, emt);
					if (sv)
						sv->at_size(ss, emt);
				}
				if (sv)
				{
					sv->set_content_size(vsize);  // 设置滚动视图内容大小
				}
			}
		}
		auto cb = (std::function<void(int, njson*)>*)toUInt((*d)[".click"]);
		if (cb && *cb)
		{
			(*cb)(idx, d);
		}
	}
}


// 创建导航菜单, cb单击回调
void new_tree2(njson d, njson info, std::function<void(int, njson*)> cb)
{
	glm::ivec2 size = { 220, 680 }, pos = { 10,10 };
	glm::ivec2 ps = { 10,10 };
	glm::ivec2 ss = { 180,50 };
	glm::ivec2 fh = { 22, 18 };
	glm::ivec2 space = { 2, 6 };
	glm::ivec2 effect = { 0, 1 };
	glm::ivec2 color_idx = { 0, 4 };
	glm::ivec2 scroll = { 10, 10 };
	glm::ivec2 crow_size = { 10, 10 };
	float padx = 20;
	float fr = toFloat(info["round"], 0.1);
	float mt = toFloat(info["mt"], 0.2);
	bool xtype = toBool(info["xtype"]);
	njson btn, r;
	int i = 0, i1 = d.size();
	njson tr;
	int y = 0;
	std::string gid = toStr(info["gid"], "list_nav");
	toiVec2(info["pos"], pos);		// 整体坐标
	toiVec2(info["cpos"], ps);		// 坐标
	toiVec2(info["size"], size);		// 大小
	toiVec2(info["fontheight"], fh);		// 字高
	toiVec2(info["space"], space);		// 间隔
	toiVec2(info["row_size"], ss);		// 行宽高
	toiVec2(info["color_idx"], color_idx);		// 风格颜色
	toiVec2(info["effect"], effect);		// 风格
	toiVec2(info["scroll"], scroll);		// 滚动量
	toiVec2(info["crow_size"], crow_size);		// 子元素大小
	glm::vec2 ips;
	toVec2(info["ips"], ips);		// 偏移
	uint32_t divc[2] = {};
	auto fillc = info["btn_fillcolor"];
	auto bc = info["btn_bordercolor"];
	padx = toInt(info["indent"], padx);
	njson kstr0;
	for (auto it : d)
	{
		auto s2 = it.begin();
		auto str = toStr(s2.key());
		auto& t0 = push_button(str.c_str(), color_idx.x, i, btn, ss, fh.x);
		auto& kt = s2.value();
		njson atn;
		atn["gid"] = gid + "_" + std::to_string(i);
		t0["gid"] = atn["gid"];
		t0["effect"] = effect.x;
		if (bc.size())
			t0["border_color"] = bc[0];//? bc : 0xccff9e40;
		if (fillc.size())
			t0["fill_color"] = fillc[0];//? bc : 0xccff9e40;
		auto ps1 = ps;
		ps1.y += y;
		t0["pos"] = { ps1.x,  ps1.y };
		t0["sps"] = { ips.x,0.5 };
		//t0["ips"] = { ips.x, 0 };
		atn["pos"] = { ps1.x,  ps1.y };
		y += ss.y + space.x;
		{
			auto& t = atn["v"][0];
			set_ps(t, ss, ps1, false);
			t["s"] = str;
		}
		for (size_t x = 0; x < kt.size(); x++)
		{
			auto nt = toStr(kt[x]);
			glm::vec2 c2 = { crow_size.x - padx,crow_size.y };
			ps1 = ps;
			ps1.y += y;
			auto& t = atn["v"][x + 1];
			set_ps(t, c2, ps1, false);
			t["s"] = nt;
			kstr0.push_back(nt);
			auto& c0 = push_button(nt.c_str(), color_idx.y, i1++, btn, c2, fh.y);
			c0["gid"] = atn["gid"];
			c0["effect"] = effect.y;
			//c0["has_drag_pos"] = 1;
			c0["pos"] = { ps1.x,  ps1.y };
			if (bc.size() > 1)
				c0["border_color"] = bc[1];
			if (fillc.size() > 1)
				c0["fill_color"] = fillc[1];

			c0["sps"] = { ips.y,0.5 };
			//c0["ips"] = { ips.y,0 };
			y += c2.y + space.x;
		}
		y += space.y;
		atn["size"] = kt.size();
		tr.push_back(atn);
		i++;
	}
	njson* od = new njson();
	(*od)["$count"] = d.size();
	(*od)["$at"] = tr;
	(*od)["$ctx"] = (uint64_t)this;
	njson base;
	base["sp"] = { space.x,space.y,padx,mt };
	base["xtype"] = xtype;
	base["gid"] = gid;
	base["asize"] = info["asize"];
	base["expmt"] = info["expmt"];
	(*od)["$base"] = base;
	{
		auto& kn = btn;
		auto fontsize = fh.x;
		int m = 0;
		njson b1;
		for (int i = 0; i < kn.size(); i++)
		{
			b1.clear();
			auto it = kn[i];
			b1["t"] = "button";
			b1["fround"] = fr;
			b1["abs"] = true;// 使用动画控制坐标，所以设置绝对坐标
			for (auto& [k, v] : it.items())
				b1[k] = v;
			b1[".e"] = (uint64_t)on_navclick;
			b1[".eid"] = it["eid"];
			b1[".eud"] = (uint64_t)od;
			r.push_back(b1);
			m++;
		}
	}

	if (cb)
	{
		auto ncb = new std::function<void(int, njson*)>(cb);
		(*od)[".click"] = (uint64_t)ncb;
	}
	njson dv;
	dv["pid"] = 0;	// pid==0是根节点
	dv["dragable"] = 0;	// 可拖动
	dv["front"] = 0 * 1;	// 点击前置显示
	dv["rounding"] = info["rounding"];	// 圆角
	glm::ivec2 s = { size.x, size.y };
	dv["size"] = { s.x,s.y };
	dv["pos"] = { pos.x,pos.y };
	merge_j(dv, info);
	dv["t"] = "div";
	dv[".dp"] = (uint64_t)od;
	dv[".onfree"] = (uint64_t)freecb;
	njson rv;
	rv["t"] = "view";
	dv["fill"] = 0;	dv["border"] = 0; dv.erase("color");
	//rv["border"] = 0x80a05000;	rv["fill"] = 0xff505050; // 边框背景颜色
	rv["color"] = info["color"];
	rv["rounding"] = info["rounding"];	// 圆角
	// auto_size{xy一般为负数(直接和父级相加)，zw要大于0}
	rv["auto_size"] = { 0,0,1,1 };
	rv["abs"] = 1;
	rv["size"] = { size.x,size.y };
	rv["vss"] = { size.x - 100,y };
	rv["step"] = { scroll.x, scroll.y };
	rv["sc_visible"] = info["sc_visible"];
	rv["gid"] = gid;
	rv[".c"] = r;
	dv[".c"].push_back(rv);
	new_widget(dv, 0);
	glm::vec2 vsize;
	if (get_switch(tr, -1, { space.x,space.y,padx }, 0, xtype, &vsize))
	{
		auto sv = get_svptr(gid.c_str(), 0);
		push_action(tr);
		if (sv)
			sv->set_content_size(vsize);
	}
}
#endif