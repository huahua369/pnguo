
#include <pch1.h>
#include <vkgui/pnguo.h>
#include <vkgui/tinysdl3.h>
#include <vkgui/mapView.h>

#include <iostream>

#include <cairo/cairo.h>
#include <stdfloat>


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

// K线视图
#if 1

// 蜡烛台
struct candlestick_css {
	glm::ivec2 color = { 0xff0020ff, 0xff00ff20 };		// 显示颜色：阳、阴
	glm::vec2 width = { 5, 1 };	// 宽度
	float height;			// 图表高
	float space = 1;		// 间隔
	float tks = 1;			// 线粗 thickness
	glm::i16vec2 rc = { 0,1 };// 0空心，1填充
	uint32_t macol[4] = { 0xffC67054, 0xff75CC91, 0xff58C8FA, 0xff6666EE };
};
// 蜡烛台图表
class charts_u1
{
public:
	glm::vec2 _pos = {};
	size_t cidx = 0;	//显示下标
private:
	std::vector<glm::vec4> kdata;
	glm::vec2 _size = {};
	candlestick_css _ct = {};
	glm::ivec4 bc = { 0x80ff9e40, 0x20000000,1,4 };
	std::map<int, std::vector<glm::vec2>> ma;
public:
	charts_u1();
	~charts_u1();
	void set_rect(const glm::vec4& r);
	void set_color(const glm::ivec4& c);
	void set_filltype(const glm::ivec2& c);
	candlestick_css* get_ct();
	void push_v4(glm::vec4* v, int n);
	void push_line(const char* str, int n);
	void update(const glm::ivec4& id = { 0, 1, 2, 3 });
	void make_ma(int n);
public:

	void update_draw(cairo_t* cr);
private:

};
charts_u1::charts_u1() {}
charts_u1::~charts_u1() {}
inline glm::vec2 pad5fxy(glm::vec2 p, float p1 = 0.5f)
{
	auto kk = glm::modf(p, p);
	p += p1;
	return p;
}
inline glm::vec2 pad5fx(glm::vec2 p, float p1 = 0.5f)
{
	auto kk = glm::modf(p, p);
	p.x += p1;
	return p;
}
// 绘制单个glm::vec4 v;			// 开盘	    最高	    最低	    收盘
		// pos;			// 中线坐标,
void draw_candlestick(cairo_t* cr, glm::vec2 pos, glm::vec4 v, candlestick_css* ct)
{
	pos.y += ct->height;
	auto c = v.x < v.w ? ct->color.x : ct->color.y;
	glm::vec2 a[2] = { {pos.x, pos.y - v.x}, {pos.x, pos.y - v.w} };
	glm::vec2 a1 = pos, a2 = pos;
	a1.y -= v.y; a2.y -= v.z;
	auto cy = ct->width.y - abs(a[0].y - a[1].y);
	if (cy > 0)
	{
		a[(v.x < v.w) ? 0 : 1].y += cy;
	}
	a1 = pad5fx(a1);
	a2 = pad5fx(a2);
	auto rx = v.x < v.w ? ct->rc.x : ct->rc.y;
#if 0
	a[0] = pad5fx(a[0]);
	a[1] = pad5fx(a[1]);
	cairo_move_to(cr, a[0].x, a[0].y);
	cairo_line_to(cr, a[1].x, a[1].y);
	fill_stroke(cr, 0, c, ct->width.x, false);
#else
	if (rx)
	{
		cairo_move_to(cr, a1.x, a1.y);
		cairo_line_to(cr, a2.x, a2.y);
		fill_stroke(cr, 0, c, ct->width.y, false);
		if (a[0].y > a[1].y)
			std::swap(a[0].y, a[1].y);
		a[0] = pad5fx(a[0]);
		a[1] = pad5fx(a[1]);
		cairo_move_to(cr, a[0].x, a[0].y);
		cairo_line_to(cr, a[1].x, a[1].y);
		fill_stroke(cr, 0, c, ct->width.x, false);
	}
	else
	{
		a[0] = pad5fx(a[0]);
		a[1] = pad5fx(a[1]);
		if (a[0].y > a[1].y)
			std::swap(a[0].y, a[1].y);
		cairo_move_to(cr, a1.x, a1.y);
		cairo_line_to(cr, a2.x, a[0].y);
		fill_stroke(cr, 0, c, ct->width.y, false);
		cairo_move_to(cr, a1.x, a[1].y);
		cairo_line_to(cr, a2.x, a2.y);
		fill_stroke(cr, 0, c, ct->width.y, false);
		a[0].x -= (ct->width.x - ct->width.y) * 0.5;
		a[0] = pad5fx(a[0]);
		draw_rectangle(cr, { a[0] , ct->width.x - ct->width.y,abs(a[1].y - a[0].y) }, 0);
		fill_stroke(cr, 0, c, ct->width.y, false);
	}
#endif
}
void charts_u1::set_rect(const glm::vec4& r)
{
	_pos = { r.x, r.y };
	_size = { r.z, r.w };
	_ct.height = _size.y - _ct.width.x;
}
void charts_u1::set_color(const glm::ivec4& c)
{
	bc = c;
}
void charts_u1::set_filltype(const glm::ivec2& c)
{
	_ct.rc = c;
}
candlestick_css* charts_u1::get_ct()
{
	return &_ct;
}
double cdif(glm::vec2 src, glm::vec2 dst) {
	return  (dst.x - dst.y) / (src.x - src.y);
}
double between0_1(double x, glm::vec2 m, double dif) {
	return m.y + dif * (x - m.x);
}
// 映射到范围
void between0_1(glm::vec4& xx, glm::vec2 m, double dif) {
	xx.x = m.y + dif * (xx.x - m.x);
	xx.y = m.y + dif * (xx.y - m.x);
	xx.z = m.y + dif * (xx.z - m.x);
	xx.w = m.y + dif * (xx.w - m.x);
}

// 计算MA均线
void calculateMA(int dayCount, glm::vec4* v, int count, float space, float ic, std::vector<glm::vec2>& result)
{
	result.clear();
	int rc = count / dayCount;
	result.reserve(rc);
	double sp = 0;
	double sum = 0;
	for (int i = 0; i < dayCount; i++) {
		sum += v[i].w;// +收盘价
		sp += space;
	}
	result.push_back({ sp - space, ic - sum / dayCount });
	for (int i = dayCount; i < count; i++, sp += space) {
		sum += v[i].w;
		sum -= v[i - dayCount].w;
		result.push_back({ sp, ic - sum / dayCount });
	}
	return;
}
void charts_u1::push_v4(glm::vec4* v, int n)
{
	if (v && n > 0) {
		auto ps = kdata.size();
		kdata.resize(n + ps);
		memcpy(kdata.data() + ps, v, n * sizeof(glm::vec4));
	}
}
void charts_u1::push_line(const char* str, int n)
{
	if (str && n)
	{
		std::string buf;
		buf.assign(str, n);
		//日期	    开盘	    最高	    最低	    收盘	    成交量	    成交额
		char dt[20] = {};
		glm::vec4 v = {};
		int cj0 = 0;
		double cj1 = 0.0;
		auto hr = sscanf(buf.c_str(), "%s	%f	%f	%f	%f	%d	%lf", dt, &v.x, &v.y, &v.z, &v.w, &cj0, &cj1);
		if (hr == 7)
			kdata.push_back(v);
	}
}
void charts_u1::update(const glm::ivec4& idx)
{
	glm::ivec4 defidx = { 0, 1, 2, 3 };
	if (kdata.empty())return;
	auto v = kdata.data();
	auto nc = kdata.size();
	float mi = v->z, ma = v->y;
	if (idx != defidx)
	{
		for (size_t i = 0; i < nc; i++)
		{
			float* p = (float*)&v[i];
			glm::vec4 tv4;
			tv4.x = p[idx.x];
			tv4.y = p[idx.y];
			tv4.z = p[idx.z];
			tv4.w = p[idx.w];
			*((glm::vec4*)p) = tv4;
			if (mi > v[i].z)mi = v[i].z;
			if (ma < v[i].y)ma = v[i].y;
		}
	}
	else {
		for (size_t i = 0; i < nc; i++)
		{
			if (mi > v[i].z)mi = v[i].z;
			if (ma < v[i].y)ma = v[i].y;
		}
	}
	// 数据最大值，最小值 ， 显示范围高度、最低点
	auto dif = cdif({ ma, mi }, { _ct.height, _ct.width.x });
	// 归一化
	for (size_t i = 0; i < nc; i++)
	{
		between0_1(v[i], { mi, _ct.width.x }, dif);
	}
	std::array<int, 4> mas = { 5, 10, 20, 30 };
	for (auto it : mas) {
		make_ma(it);
	}
	printf("min max:%f\t%f\n", mi, ma);
}
void charts_u1::make_ma(int n)
{
	calculateMA(n, kdata.data(), kdata.size(), _ct.space + _ct.width.x, _ct.height, ma[n]);
}

void charts_u1::update_draw(cairo_t* cr)
{
	auto pos = _pos;
	pos += _ct.width.x;
	int kx = pos.x + _size.x;
	draw_rectangle(cr, { _pos, _size }, bc.w);
	fill_stroke(cr, bc.x, 0, 0, false);
	auto p = kdata.data();
	int psx = _ct.space + _ct.width.x;
	psx *= cidx;
	for (size_t i = cidx; i < kdata.size(); i++)
	{
		auto it = p[i];
		draw_candlestick(cr, pos, it, &_ct);
		pos.x += _ct.space + _ct.width.x;
		if (pos.x > kx)
		{
			break;
		}
	}
	int i = 0;
	pos = _pos;
	pos += _ct.width.x;
	cairo_save(cr);
	cairo_translate(cr, -psx, 0);
	for (auto& [k, v] : ma)
	{
		draw_polyline(cr, pos, v.data(), v.size(), _ct.macol[i++], false, _ct.tks);
		if (i > 4)i = 0;
	}
	cairo_restore(cr);
	draw_rectangle(cr, { _pos, _size }, bc.w);
	fill_stroke(cr, 0, bc.y, bc.z, false);
}
std::string get_linestr(const char** p, int& len) {
	std::string r;
	auto str = *p;
	auto t = str;
	for (; *str && len > 0; str++) {
		len--;
		if (*str == '\r')
		{
			str++; len--;
		}
		if (*str == '\n')
		{
			r.append(t, str); str++;
			t = str;
			break;
		}
	}
	*p = str;
	return r;
}
#endif // 1

int main()
{
	glm::ivec2 ws = { 1280,800 };
	auto app = new_app();
	form_newinfo_t ptf = { app,(char*)u8"窗口1",ws, ef_vulkan /*| ef_resizable*/,true };
	form_x* form0 = (form_x*)call_data((int)cdtype_e::new_form, &ptf);
	//form_x* form0 = ptf.has_renderer ? app->new_form_renderer(ptf.title, ptf.size, ptf.flags) : app->new_form(ptf.title, ptf.size, ptf.flags));

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
	ct->width = { tk + ct->tks * 3,tk };
	hz::mfile_t msh;
	auto tp = msh.open_d("SH_600938.txt", true);// 读取从通达信导出的历史k线数据
	std::string title;
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
	pcu->update();
	auto txt = new layout_text_x();
	{
		pl1->draggable = true; //可拖动
		pl1->set_size({ 830,600 });
		pl1->set_pos({ 100,100 });
		pl1->set_colors({ 0xff121212,-1,0,0 });
		pl1->on_click = [](plane_cx* p, int state, int clicks) {};

		txt->set_ctx(app->font_ctx);
		int fontsize = 26;
		glm::vec2 text_align = { 0.1,0.1 };
		//txt->add_family("Consolas", 0);
		//txt->add_family((char*)u8"XITS Math", 0);
		txt->add_family((char*)u8"新宋体", 0);
		txt->add_text({ 0 + 2,0 + 2,260,90 }, text_align, title.c_str(), -1, 16);
		//txt->add_text({ 0 + 2,0 + 2,200,90 }, text_align, u8"↣ ↠ ↦ ↤ → ← ↔ ⇒ ⇐ ⇔\n 𝔹 ℂ 𝔽 ℕ ℙ ℚ ℝ 𝕋 ℤ \nα β χ δ Δ γ Γ ϵ ɛ η \nκ λ Λ μ ν ω Ω ϕ φ Φ \nπ Π ψ Ψ ρ σ Σ τ θ ϑ Θ υ \nξ Ξ ζ 𝔸 𝐀 𝔄 𝕬 𝐴 𝑨", -1, 18);
		txt->clear_family();
		//txt->add_family("Consolas", 0);
		txt->add_family((char*)u8"楷体", 0);
		txt->add_family("Segoe UI Emoji", 0); text_align = { 0.0,0.1 };
		txt->add_text({ 330 + 2 ,0 + 2 ,400,90 }, text_align, u8"🍊🍇 \n5日线, 10日线, 20日线, 30日线", -1, fontsize);

		auto et1 = pl1->add_input("", { 100,30 }, true);
		et1->set_pos({ 10,10 });
		pl1->draw_cb = [=](cairo_t* cr)
			{
				cairo_translate(cr, 10, 100);
				draw_rectangle(cr, { 0.5,0.5,260 + 4,90 + 4 }, 4);
				fill_stroke(cr, 0x80805c42, 0xffff802C, 1, false);
				draw_rectangle(cr, { 330.5,0.5,400 + 4,90 + 4 }, 4);
				fill_stroke(cr, 0xff422e21, 0xffff802C, 1, false);

				draw_rectangle(cr, { 0,0,260 ,90 }, 4);
				draw_rectangle(cr, { 330.5,0.5,400 + 4,90 + 4 }, 4);
				draw_rectangle(cr, { 0,150,800,300 }, 4);
				cairo_clip(cr);
				cairo_new_path(cr);
				txt->draw_text(cr);
				pcu->cidx = std::atoi(et1->_text.c_str());
				pcu->update_draw(cr);
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
				int y = 10;

				cairo_save(cr);
				for (auto it : txt->msu)
				{
					auto ss = draw_image(cr, it, { 10, y }, { 0,0,1024,512 });
					y += ss.y + 10;
				}
				cairo_restore(cr);
			};
	}
	{
		pl2->draggable = true; //可拖动
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
