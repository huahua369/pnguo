
#include <pch1.h>
#include <pnguo.h>
#include <cairo/cairo.h>
#include "charts.h"
// K线视图
#if 1

charts_u1::charts_u1() {}
charts_u1::~charts_u1() {}
inline glm::vec2 pad5f(glm::vec2 p, float p1 = 0.5f)
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
		a[0] = pad5f(a[0]);
		a[1] = pad5f(a[1]);
		if (a[0].y > a[1].y)
			std::swap(a[0].y, a[1].y);
		cairo_move_to(cr, a1.x, a1.y);
		cairo_line_to(cr, a2.x, a[0].y);
		fill_stroke(cr, 0, c, ct->width.y, false);
		cairo_move_to(cr, a1.x, a[1].y);
		cairo_line_to(cr, a2.x, a2.y);
		fill_stroke(cr, 0, c, ct->width.y, false);
		a[0].x -= (ct->width.x - ct->width.y) * 0.5;
		a[0] = pad5f(a[0]);
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
std::string get_linestr(const char** p, int& len)
{
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
