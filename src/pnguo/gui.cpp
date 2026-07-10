
#include "pch1.h"

#include <SDL3/SDL_keycode.h>
#include <stb_image_write.h>

#include <mapView.h>
#include <event.h>
#include "font_core.h"
#include "pnguo.h"
#include "gui.h"
#include "render.h"
#ifdef min
#undef min
#undef max
#endif // min

#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif


// 杂算法
#if 1
namespace pn {
	void tobox(const glm::vec2& v, glm::vec4& t)
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
	glm::vec2 getbox2t(std::vector<glm::vec2>& vt, int t)
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

	glm::vec2 getbox2t(const glm::vec4& box, int t)
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

	/*
	缩放模式
	xy从中心扩展
	固定1左上角，2右上角，3右下角，4左下角，来缩放xy
	单扩展 x左右、y上下
	获取图形包围盒，返回坐标t=0中心，1左上角，2右上角，3右下角，4左下角，
	*/
	glm::mat3 box_scale(const glm::vec4& box, int ddot, glm::vec2 sc, bool inve)
	{
		auto cp = getbox2t(box, ddot);
		glm::vec2 cp0 = {};
		glm::vec2 scale = sc;
		glm::mat3 m(1.0);
		// 缩放
		assert(scale.x > 0 && scale.y > 0);
		if (scale.x > 0 && scale.y > 0)
		{
			m = glm::translate(glm::mat3(1.0f), cp) * glm::scale(glm::mat3(1.0f), scale) * glm::translate(glm::mat3(1.0f), -cp);
			if (inve)
			{
				m = glm::inverse(m);	// 反向操作 
			}
		}
		return m;
	}
}
//!pn
#endif // 1

#if 1
/*
<表格、树形>
数据结构设计:
	网格线：		虚线/实线、颜色
	背景：		纯色
	内容store：	数值、文本、简单公式、图片/SVG

*/


#endif // 1


#if 1
// 默认按钮样式

image_btn::image_btn() :widget_t(WIDGET_TYPE::WT_IMAGE_BTN)
{

}
image_btn::~image_btn()
{}
void image_btn::set_size(const glm::vec2& ss)
{
	widget_t::set_size(ss);
	if (multi == 0) {
		auto p = &state_img[0];
		p->tex_rc = glm::ivec4(0, 0, ss);
	}
}
void image_btn::set_state(BTN_STATE m, const glm::ivec2& rc)
{
	int width = st.width;
	int n = 0;
	if (_bst & (int)BTN_STATE::STATE_NOMAL) {
		n++;
	}
	if (_bst & (int)BTN_STATE::STATE_HOVER) {
		n++;
	}
	if (_bst & (int)BTN_STATE::STATE_ACTIVE) {
		n++;
	}
	if (_bst & (int)BTN_STATE::STATE_FOCUS) {
		n++;
	}
	if (_bst & (int)BTN_STATE::STATE_DISABLE) {
		n++;
	}
	if (n > 0)
	{
		width /= n;
		glm::ivec4 rc2 = { 0,0,width,st.height };
		if (rc.x > 0)
			rc2.z = rc.x;
		if (rc.y > 0)
			rc2.w = rc.y;
		if (_bst & (int)BTN_STATE::STATE_NOMAL) {
			state_img[0].tex_rc = rc2;
		}
		if (_bst & (int)BTN_STATE::STATE_HOVER) {
			rc2.x += width;
			state_img[1].tex_rc = rc2;
		}
		if (_bst & (int)BTN_STATE::STATE_ACTIVE) {
			rc2.x += width;
			state_img[2].tex_rc = rc2;
		}
		if (_bst & (int)BTN_STATE::STATE_FOCUS) {
			rc2.x += width;
			state_img[3].tex_rc = rc2;
		}
		if (_bst & (int)BTN_STATE::STATE_DISABLE) {
			rc2.x += width;
			state_img[4].tex_rc = rc2;
		}
	}
}
void image_btn::set_state1(BTN_STATE m, const glm::ivec4& rc)
{
	if (_bst & (int)BTN_STATE::STATE_NOMAL) {
		state_img[0].tex_rc = rc;
	}
	if (_bst & (int)BTN_STATE::STATE_HOVER) {
		state_img[1].tex_rc = rc;
	}
	if (_bst & (int)BTN_STATE::STATE_ACTIVE) {
		state_img[2].tex_rc = rc;
	}
	if (_bst & (int)BTN_STATE::STATE_FOCUS) {
		state_img[3].tex_rc = rc;
	}
	if (_bst & (int)BTN_STATE::STATE_DISABLE) {
		state_img[4].tex_rc = rc;
	}
}
void image_btn::set_image(image_ptr_t* img)
{
	img_type = 0;
	st = *img;
	imgptr.img = &st;
}
void image_btn::set_vkimage(void* vkimage, int width, int height, int type)
{
	st = {};
	st.width = width;
	st.height = height;
	st.type = type;
	st.ptr = vkimage;
	imgptr.img = &st;
	img_type = 2;
}
void image_btn::set_surface(void* surf, int width, int height)
{
	st = {};
	st.width = width;
	st.height = height;
	imgptr.surf = surf;
	img_type = 1;
}
color_btn::color_btn() :widget_t(WIDGET_TYPE::WT_COLOR_BTN)
{}
color_btn::~color_btn()
{}
gradient_btn::gradient_btn() :widget_t(WIDGET_TYPE::WT_GRADIENT_BTN)
{}
gradient_btn::~gradient_btn()
{}
menu_btn::menu_btn() :widget_t(WIDGET_TYPE::WT_MENU_BTN)
{}

menu_btn::~menu_btn()
{}
radio_tl::radio_tl() :widget_t(WIDGET_TYPE::WT_RADIO)
{}
checkbox_tl::checkbox_tl() :widget_t(WIDGET_TYPE::WT_CHECKBOX)
{}
checkbox_tl::~checkbox_tl()
{}
switch_tl::switch_tl() :widget_t(WIDGET_TYPE::WT_SWITCH)
{}
switch_tl::~switch_tl()
{}
progress_tl::progress_tl() :widget_t(WIDGET_TYPE::WT_PROGRESS)
{}
progress_tl::~progress_tl()
{}
slider_tl::slider_tl() :widget_t(WIDGET_TYPE::WT_SLIDER)
{}
slider_tl::~slider_tl()
{}
colorpick_tl::colorpick_tl() :widget_t(WIDGET_TYPE::WT_COLORPICK)
{}
colorpick_tl::~colorpick_tl()
{}
scroll_bar::scroll_bar() :widget_t(WIDGET_TYPE::WT_SCROLL_BAR)
{}
scroll_bar::~scroll_bar()
{}


bool image_btn::on_mevent(int type, const glm::vec2& mps, void* e)
{
	return false;
}

bool image_btn::update(float) {
	show_idx = 0;
	if (_bst & (int)BTN_STATE::STATE_HOVER) {
		show_idx = 1;
	}
	if (_bst & (int)BTN_STATE::STATE_ACTIVE) {
		show_idx = 2;
	}
	if (_bst & (int)BTN_STATE::STATE_FOCUS) {
		show_idx = 3;
	}
	if (_bst & (int)BTN_STATE::STATE_DISABLE) {
		show_idx = 4;
	}
	if (multi == 0)
	{
		show_idx = 0;
	}
	//  STATE_NOMAL = BIT_INC(0),
	//	STATE_HOVER = BIT_INC(1),
	//	STATE_ACTIVE = BIT_INC(2),
	//	STATE_FOCUS = BIT_INC(3),
	//	STATE_DISABLE = BIT_INC(4),
	return false;
}

class btn_colorlist
{
public:
	btn_colorlist();
	~btn_colorlist();
	btn_cols_t* get(size_t idx) {
		return (idx < bcs.size()) ? (btn_cols_t*)bcs[idx].data() : nullptr;
	}
private:
	std::vector<std::array<uint32_t, 8>> bcs = { { 0xFFffffff, 0xFF409eff, 0xFF409eff, 0xFF66b1ff, 0xFFe6e6e6, 0xFF0d84ff, 0xFF0d84ff ,0 },
			{ 0xFFffffff, 0xFF67c23a, 0xFF67c23a, 0xFF85ce61, 0xFFe6e6e6, 0xFF529b2e, 0xFF529b2e ,0},
			{ 0xFFffffff, 0xFF909399, 0xFF909399, 0xFFa6a9ad, 0xFFe6e6e6, 0xFF767980, 0xFF767980 ,0},
			{ 0xFFffffff, 0xFFe6a23c, 0xFFe6a23c, 0xFFebb563, 0xFFe6e6e6, 0xFFd48a1b, 0xFFd48a1b ,0 },
			{ 0xFFffffff, 0xFFf56c6c, 0xFFf56c6c, 0xFFf78989, 0xFFe6e6e6, 0xFFf23c3c, 0xFFf23c3c ,0 }
	};
};

btn_colorlist::btn_colorlist()
{
	for (auto& it : bcs)
	{
		for (size_t i = 0; i < 8; i++)
		{
			auto c = (uint8_t*)&it[i];
			std::swap(c[0], c[2]);
		}
	}
}

btn_colorlist::~btn_colorlist()
{}
btn_cols_t* color_btn::set_btn_color_bgr(size_t idx)
{
	static btn_colorlist clst;
	auto ret = clst.get(idx);
	if (ret)
	{
		cs.pdc = *ret;
	}
	return ret;
}
#define UF_COLOR
union u_col
{
	uint32_t uc;
	unsigned char u[4];
	struct urgba
	{
		unsigned char r, g, b, a;
	}c;
};
#define FCV 255.0
#define FCV1 256.0
inline uint32_t set_alpha_xf(uint32_t c, double af)
{
	if (af < 0)af = 0;
	//uint32_t a = af * FCV;
	u_col* t = (u_col*)&c;
	double a = t->c.a * af + 0.5;
	if (a > 255)
		t->c.a = 255;
	else
		t->c.a = a;
	return c;
}
inline uint32_t set_alpha_xf2(uint32_t c, double af)
{
	if (af < 0)af = 0;
	u_col* t = (u_col*)&c;
	t->c.r *= af;
	t->c.g *= af;
	t->c.b *= af;
	return c;
}
inline uint32_t set_alpha_x(uint32_t c, uint32_t a0)
{
	u_col* t = (u_col*)&c;
	double af = a0 / FCV;
	double a = t->c.a * af + 0.5;
	if (a > 255)
		t->c.a = 255;
	else
		t->c.a = a;
	return c;
}
inline uint32_t set_alpha_f(uint32_t c, double af)
{
	if (af > 1)af = 1;
	if (af < 0)af = 0;
	uint32_t a = af * FCV + 0.5;
	u_col* t = (u_col*)&c;
	t->c.a = a;
	return c;
}

inline glm::vec4 to_c4(uint32_t c)
{
	u_col* t = (u_col*)&c;
	glm::vec4 fc;
	float* f = &fc.x;
	for (int i = 0; i < 4; i++)
	{
		*f++ = t->u[i] / FCV;
	}
	return  fc;
}

bool update_color_btn(color_style* p, float delta)
{
	p->dtime += delta;
	auto dt = p->dtime;
	if (dt > 0.150) {
		p->dtime = 0;
	}
	if (p->_bst == p->_old_bst)return false;
	p->_old_bst = p->_bst;
	btn_cols_t* pdc = &p->pdc;
	p->_disabled = (p->_bst & (int)BTN_STATE::STATE_DISABLE);
	auto text_color = p->ptext_style ? p->ptext_style->color : 0;
	if (p->_disabled)
	{
		p->hover = false;
		p->dfill = pdc->background_color;
		p->dcol = pdc->border_color;
		if (p->effect == uTheme::dark)
		{
			p->dcol = 0;
			p->dtext_color = (text_color) ? text_color : pdc->font_color;
		}
		if (p->effect == uTheme::light)
		{
			p->dcol = set_alpha_xf(p->dcol, p->light * 3);
			p->dfill = set_alpha_xf(p->dfill, p->light);
			p->dtext_color = (text_color) ? text_color : pdc->border_color;
		}
		if (p->effect == uTheme::plain)
		{
			p->dfill = 0;
			p->dtext_color = (text_color) ? text_color : pdc->border_color;
		}
		p->dcol = set_alpha_x(p->dcol, p->disabled_alpha);
		p->dfill = set_alpha_x(p->dfill, p->disabled_alpha);
		p->dtext_color = set_alpha_x(p->dtext_color, p->disabled_alpha);
	}
	else
	{
		bool isdown = p->mPushed = (p->_bst & (int)BTN_STATE::STATE_ACTIVE);
		p->hover = (p->_bst & (int)BTN_STATE::STATE_HOVER);
		if (isdown)
		{
			p->dfill = pdc->active_background_color;
			p->dcol = pdc->active_border_color;
			if (p->effect == uTheme::plain)
			{
				p->dfill = set_alpha_xf(p->dcol, p->light);
			}
			else {
				p->dtext_color = (text_color) ? text_color : pdc->active_font_color;
			}
			if (p->effect == uTheme::light)
			{
				p->dfill = set_alpha_f(p->dfill, p->light * 5);
				p->dcol = set_alpha_f(p->dcol, p->light * 6);
			}
			if (p->effect == uTheme::dark)
			{
				p->dcol = 0;
			}
		}
		else if (p->hover)
		{
			uint32_t ac = pdc->hover_color;
			p->dfill = pdc->hover_color;
			if (pdc->hover_border_color)
				p->dcol = pdc->hover_border_color;

			if (p->effect == uTheme::plain)
			{
				p->dcol = pdc->border_color;
				p->dfill = set_alpha_xf(p->dcol, p->light);
			}
			if (p->effect == uTheme::light)
			{
				p->dfill = set_alpha_f(p->dfill, p->light * 5);
				p->dtext_color = (text_color) ? text_color : pdc->font_color;
			}
		}
		else {
			p->dfill = pdc->background_color;
			p->dcol = pdc->border_color;
			if (p->effect == uTheme::dark)
			{
				p->dcol = 0;
				p->dtext_color = (text_color) ? text_color : pdc->font_color;
			}
			if (p->effect == uTheme::light)
			{
				p->dcol = set_alpha_xf(p->dcol, p->light * 3);
				p->dfill = set_alpha_xf(p->dfill, p->light);
				p->dtext_color = (text_color) ? text_color : pdc->border_color;
			}
			if (p->effect == uTheme::plain)
			{
				p->dfill = 0;
				p->dtext_color = (text_color) ? text_color : pdc->border_color;
			}
		}
	}
	return true;
}


const char* gradient_btn::c_str()
{
	return str.c_str();
}

void gradient_btn::init(glm::ivec4 rect, const std::string& text, uint32_t back_color, uint32_t text_color)
{
	auto p = this;
	auto& info = *p;
	info._pos = { rect.x, rect.y };
	info._size = { rect.z, rect.w };
	info.rounding = 4;
	info.back_color = back_color;
	info.style.color = text_color;
	info.opacity = 1;
	info.str = text.c_str();
	info.borderLight = 0xff5c5c5c;
	info.borderDark = 0xff1d1d1d;
	return;
}
bool gradient_btn::update(float delta)
{
	auto p = this;
	if (!p)return false;
	if (_bst == _old_bst)return false;
	_old_bst = _bst;
	auto& info = *p;

	// (sta & hz::BTN_STATE::STATE_FOCUS)
	uint32_t gradTop = info.gradTop.x;// 0xff4a4a4a; 
	uint32_t gradBot = info.gradBot.x;// 0xff3a3a3a;

	info.mPushed = (_bst & (int)BTN_STATE::STATE_ACTIVE);
	info.mMouseFocus = (_bst & (int)BTN_STATE::STATE_HOVER);
	if (_bst & (int)BTN_STATE::STATE_DISABLE)
		info.mEnabled = false;
	if (info.mPushed) {
		gradTop = info.gradTop.z;//0xff292929;
		gradBot = info.gradBot.z;//0xff1d1d1d;
	}
	else if (info.mMouseFocus && info.mEnabled) {
		gradTop = info.gradTop.y;// 0x80404040;
		gradBot = info.gradBot.y;//0x80303030;
	}
	info._gradTop = gradTop;
	info._gradBot = gradBot;
	return true;
}



bool color_btn::update(float delta) {
	cs.str = str.c_str();
	cs.str_len = str.size();
	cs.ptext_style = &style;
	cs._bst = _bst;
	cs.rounding = rounding;
	return update_color_btn(&cs, delta);
}
bool menu_btn::update(float delta)
{
	return false;
}

#endif // 1



template<class T>
void free_obt(T*& p) {
	if (p) {
		delete p; p = 0;
	}
}





// todo ui



radio_tl::~radio_tl()
{
	if (gr) {
		gr->ct--;
		if (gr->ct <= 0)
			delete gr;
	}
	gr = 0;
}

void radio_tl::set_group(group_radio_t* p)
{
	if (p) {
		if (gr)gr->ct--;
		gr = p;
		p->ct++;
	}
}

void radio_tl::bind_ptr(bool* p)
{}

void radio_tl::set_value(const std::string& str, bool bv)
{
	radio_info_t k = {};
	k.text = str;
	k.value = bv; k.value1 = !bv;
	v = k;
	if (bv)
	{
		set_value();
	}
	else {
		if (v.on_change_cb) { v.on_change_cb(this, v.value); }
	}
}

void radio_tl::set_value(bool bv)
{
	v.value = bv;
	if (bv)
	{
		set_value();
	}
	else {
		if (v.on_change_cb) { v.on_change_cb(this, v.value); }
	}
}

void radio_tl::set_value()
{
	v.value = true;
	if (gr && this != gr->active)
	{
		if (gr->active)
			gr->active->set_value(false);
		gr->active = this;
		if (v.on_change_cb) { v.on_change_cb(this, v.value); }
	}
}

bool radio_tl::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	if (et == event_type2::on_click)
	{
	}
	return false;
}

bool radio_tl::update(float delta)
{
	int ic = 0;
	{
		auto& it = v;
		if (_size.x <= 0) {
			_size.x = style.radius * 2;
		}
		if (_size.y <= 0) {
			_size.y = style.radius * 2;
		}
		if (cks > 0 && v.value == v.value1) {
			cks = 0; set_value();
		}
		if (it.value != it.value1)
		{
			auto dt = fmod(delta, style.duration);
			if (style.duration > 0) {
				it.dt += delta;
				if (it.dt >= style.duration) {
					it.value1 = it.value; it.dt = 0;
					dt = 1.0;

					_old_bst = _bst;
				}
				else
				{
					dt = it.dt / style.duration;
				}
			}
			else {
				dt = 1.0;
			}
			float t = it.value ? glm::mix(style.radius - style.thickness, style.thickness * 2.0f, dt) : glm::mix(style.thickness * 2.0f, style.radius - style.thickness, dt);
			it.swidth = t;
			if (!it.value && it.dt == 0)it.swidth = 0;
			ic++;
		}
	}
	return ic > 0;
}


void checkbox_tl::bind_ptr(bool* p)
{}

void checkbox_tl::set_value(const std::string& str, bool bv)
{
	checkbox_info_t k = {};
	k.text = str;
	k.value = bv; k.value1 = !bv;
	v = k;
	if (v.on_change_cb) { v.on_change_cb(this, v.value); }
}

void checkbox_tl::set_value(bool bv)
{
	v.value = bv;
	if (v.on_change_cb) { v.on_change_cb(this, v.value); }
}

void checkbox_tl::set_value()
{
	v.value1 = v.value;
	v.value = !v.value;
	if (v.on_change_cb) { v.on_change_cb(this, v.value); }
}

bool checkbox_tl::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	if (et == event_type2::on_click)
	{
	}
	return false;
}

bool checkbox_tl::update(float delta)
{
	int ic = 0;

	{
		auto& it = v;
		if (_size.x <= 0) {
			_size.x = style.square_sz;
		}
		if (_size.y <= 0) {
			_size.y = style.square_sz;
		}

		if (cks > 0 && v.value == v.value1) {
			cks = 0; set_value();
		}
		auto duration = it.duration > 0 ? it.duration : style.duration;
		if (it.value != it.value1)
		{
			auto dt = fmod(delta, duration);
			if (duration > 0)
			{
				it.dt += delta;
				if (it.dt >= duration) {
					it.value1 = it.value; it.dt = 0;
					dt = 1.0;
					_old_bst = _bst;
				}
				else
				{
					dt = it.dt / duration;
				}
			}
			else {
				dt = 1.0;
			}

			float t = it.value ? glm::mix(0.0f, 1.0f, dt) : glm::mix(1.0f, 0.0f, dt);
			it.new_alpha = t;
			ic++;
		}
	}
	return ic > 0;
}


void switch_tl::bind_ptr(bool* p)
{
	v.pv = p;
}

void switch_tl::set_value(bool b)
{
	v.value = b;
	v.value1 = !b;
	if (v.on_change_cb) { v.on_change_cb(this, v.value); }
}

void switch_tl::set_value()
{
	v.value1 = v.value;
	v.value = !v.value;
	if (v.on_change_cb) { v.on_change_cb(this, v.value); }
}

bool switch_tl::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	if (et == event_type2::on_click)
	{
	}
	return false;
}

bool switch_tl::update(float delta)
{
	int ic = 0;
	auto& it = v;
	if (cks > 0 && v.value == v.value1) {
		cks = 0; set_value();
	}
	if (it.value != it.value1)
	{
		auto dt = fmod(delta, it.duration);
		if (it.duration > 0) {
			it.dt += delta;
			if (it.dt >= it.duration) {
				it.value1 = it.value; it.dt = 0;
				dt = 1.0;
				_old_bst = _bst;
				if (v.pv) {
					*v.pv = it.value;
				}
			}
			else
			{
				dt = it.dt / it.duration;
			}
		}
		else {
			dt = 1.0;
		}
		dcol = it.value ? glm::mix(color.y, color.x, 1.0f) : glm::mix(color.x, color.y, 1.0f);
		cpos = it.value ? glm::mix(0.0f, 1.0f, dt) : glm::mix(1.0f, 0.0f, dt);
		ic++;
	}
	return ic > 0;
}

void progress_tl::set_value(double b)
{
	if (b < 0)b = 0;
	if (b > 1)b = 1;
	value = b;
	if (format.size())
	{
		double k = get_v();
		std::string vv;
		text = pg::to_string(k) + format;
		//width = _size.x;
		if (text.size() && !text_inside) {
			//todo auto rk = ltx->get_text_rect(0, font_size, text.c_str(), -1);
			//size.x = width + rk.x + rounding * 0.5;
			if (parent)parent->uplayout = true;
		}
	}
}

void progress_tl::set_vr(const glm::ivec2& r)
{
	vr = r;
}

double progress_tl::get_v()
{
	return glm::mix(vr.x, vr.y, value);
}

bool progress_tl::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	return false;
}

bool progress_tl::update(float delta)
{
	return false;
}


void slider_tl::bind_ptr(double* p)
{}

void slider_tl::set_value(double b)
{
	if (b < 0)b = 0;
	if (b > 1)b = 1;
	value = b;

}

void slider_tl::set_vr(const glm::ivec2& r)
{
	vr = r;
}

void slider_tl::set_cw(int cw)
{
	sl.x = cw;
}

double slider_tl::get_v()
{
	return glm::mix(vr.x, vr.y, value);
}

bool slider_tl::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	glm::ivec2 poss = mps - _pos;
	glm::ivec2 ss = _size;
	if (et == event_type2::on_down) {

	}
	if (et == event_type2::on_click)
	{
		if (poss[vertical] >= 0)
		{
			double xf = (double)poss[vertical] / ss[vertical];
			if (xf > 1)xf = 1;
			if (xf < 0)xf = 0;
			value = xf;
		}
	}
	if (et == event_type2::on_drag)
	{
		poss += curpos;
		if (poss[vertical] >= 0)
		{
			double xf = (double)poss[vertical] / ss[vertical];
			if (xf > 1)xf = 1;
			if (xf < 0)xf = 0;
			value = xf;
		}
	}
	return false;
}

bool slider_tl::update(float delta)
{
	return false;
}




void colorpick_tl::init(uint32_t c, int w, int h, bool alpha)
{
	set_color2hsv(c);
	width = w;
	height = h;
	//if (height < font_size)
	//	height = ltx->get_lineheight(0, font_size);
	h = height + step;
	cpx = height * 2.5;
	int minw = cpx + step * 2;
	if (width < minw) {
		width = minw + h;
	}
	_size.x = width;
	int hn = 4;
	if (alpha)hn++;
	_size.y = h * hn;
}

uint32_t colorpick_tl::get_color()
{
	glm::vec4 hc = HSVtoRGB(hsv);
	glm::u8vec4 c = { (int)(hc.x * 255), (int)(hc.y * 255), (int)(hc.z * 255), (int)(hc.w * 255) };
	return *((uint32_t*)&c);
}

void colorpick_tl::set_color2hsv(uint32_t c)
{
	color.y = color.x;
	color.x = c;
	hsv = RGBtoHSV(c);
}

void colorpick_tl::set_hsv(const glm::vec3& c)
{
	hsv.x = c.x;
	hsv.y = c.y;
	hsv.z = c.z;
	color.y = color.x;
	color.x = get_color();
}
void colorpick_tl::set_hsv(const glm::vec4& c)
{
	hsv = c;
	color.y = color.x;
	color.x = get_color();
}
void colorpick_tl::set_posv(int poss_x)
{
	double cw0 = colorw - step, x = poss_x;
	if (x < 0) { x = 0; }
	double xf = glm::clamp((double)poss_x / cw0, 0.0, 1.0);
	int x4 = alpha ? 4 : 3;
	if (dx >= 0 && dx < x4)
	{
		hsv[dx] = xf;
	}
}
bool colorpick_tl::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	glm::ivec2 poss = mps - _pos;
	glm::ivec2 ss = _size;
	poss.x -= cpx + step;
	poss.y -= height + step;
	double htp = height + step;
	if (et == event_type2::on_click && (poss.x > 0) && poss.y > 0)
	{
		set_posv(poss.x);
	}
	if (et == event_type2::on_down) {
		if (poss.y > 0 && poss.x > 0)
			dx = poss.y / htp;
		else
			dx = -1;
	}
	if (poss.y < 0)
		poss.y = 0;
	if (et == event_type2::on_drag)
	{
		auto oldps = poss;
		poss += curpos;
		set_posv(poss.x);
	}
	return false;
}

bool colorpick_tl::update(float delta)
{
	if (hsv != oldhsv || hsvstr.empty())
	{
		int* k = nullptr;
		oldhsv = hsv;
		int h = hsv.x * 360;
		int s = hsv.y * 100;
		int v = hsv.z * 100;
		int a = hsv.w * 100;
		std::string th = std::to_string(h) + (char*)u8"°";
		std::string ts = std::to_string(s) + "%";
		std::string tv = std::to_string(v) + "%";
		std::string ta = std::to_string(a) + "%";

		hsvstr = "H:" + th;
		hsvstr += "\nS:" + ts;
		hsvstr += "\nV:" + tv;
		if (alpha)
		{
			hsvstr += "\nA:" + ta;
		}
		else { hsv.w = 1; }
		glm::vec4 hc = HSVtoRGB(hsv);
		char buf[256] = {};
		glm::ivec4 c = { (int)(hc.x * 255), (int)(hc.y * 255), (int)(hc.z * 255), (int)(hc.w * 255) };
		sprintf(buf, "#%02X%02X%02X%02X %d,%d,%d,%d", c.x, c.y, c.z, c.w, c.x, c.y, c.z, c.w);
		colorstr = buf;
		glm::ivec2 ss = _size;
		colorw = ss.x - cpx - step * 2;
		if (on_change_cb) {
			on_change_cb(this, get_color());
		}
	}
	return false;
}


colorpick_cx::colorpick_cx()
{}

colorpick_cx::~colorpick_cx()
{}

void colorpick_cx::init(uint32_t c, int w, int h)
{
	set_color2hsv(c);
	width = w;
	height = h;
	//if (height < font_size)
	//	height = ltx->get_lineheight(0, font_size);
	h = height + step;
	cpx = height * 2.5;
	int minw = cpx + step * 2;
	if (width < minw) {
		width = minw + h;
	}
	_size.x = width;
	int hn = 5;
	_size.y = h * hn;
}

uint32_t colorpick_cx::get_color()
{
	glm::vec4 hc = HSVtoRGB(hsv);
	glm::u8vec4 c = { (int)(hc.x * 255), (int)(hc.y * 255), (int)(hc.z * 255), (int)(hc.w * 255) };
	return *((uint32_t*)&c);
}

void colorpick_cx::set_color2hsv(uint32_t c)
{
	color.y = color.x;
	color.x = c;
	hsv = RGBtoHSV(c);
}

void colorpick_cx::set_hsv(const glm::vec3& c)
{
	hsv.x = c.x;
	hsv.y = c.y;
	hsv.z = c.z;
	color.y = color.x;
	color.x = get_color();
}
void colorpick_cx::set_hsv(const glm::vec4& c)
{
	hsv = c;
	color.y = color.x;
	color.x = get_color();
}
void colorpick_cx::set_posv(const glm::ivec2& poss)
{
	double cw0 = colorw - step, x = poss.x;
	if (x < 0) { x = 0; }
	double xf = glm::clamp((double)poss.x / cw0, 0.0, 1.0);
	int x4 = 4;// alpha ? 4 : 3;
	if (dx >= 0 && dx < x4)
	{
		hsv[dx] = xf;
	}
}

bool colorpick_cx::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	glm::ivec2 poss = mps - _pos;
	glm::ivec2 ss = _size;
	poss.x -= cpx + step;
	poss.y -= height + step;
	double htp = height + step;
	if (poss.y < 0)poss.y = 0;
	if (et == event_type2::on_down) {
		dx = poss.y / htp;
	}
	if (et == event_type2::on_click)
	{
		set_posv(poss);
	}
	if (et == event_type2::on_drag)
	{
		poss += curpos;
		set_posv(poss);
	}


	return false;
}

bool colorpick_cx::update(float delta)
{
	if (hsv != oldhsv || hsvstr.empty())
	{
		int* k = nullptr;
		oldhsv = hsv;
		int h = hsv.x * 360;
		int s = hsv.y * 100;
		int v = hsv.z * 100;
		int a = hsv.w * 100;
		std::string th = std::to_string(h) + (char*)u8"°";
		std::string ts = std::to_string(s) + "%";
		std::string tv = std::to_string(v) + "%";
		std::string ta = std::to_string(a) + "%";

		hsvstr = "H:" + th;
		hsvstr += "\nS:" + ts;
		hsvstr += "\nV:" + tv;
		hsvstr += "\nA:" + ta;
		glm::vec4 hc = HSVtoRGB(hsv);
		char buf[256] = {};
		glm::ivec4 c = { (int)(hc.x * 255), (int)(hc.y * 255), (int)(hc.z * 255), (int)(hc.w * 255) };
		sprintf(buf, "#%02X%02X%02X%02X %d,%d,%d,%d", c.x, c.y, c.z, c.w, c.x, c.y, c.z, c.w);
		colorstr = buf;
		glm::ivec2 ss = _size;
		colorw = ss.x - cpx - step * 2;
		if (on_change_cb) {
			on_change_cb(this, get_color());
		}
	}
	return false;
}



#if 1
void scroll_bar::set_viewsize(int64_t vs, int64_t cs, int rcw)
{
	_view_size = vs;
	_content_size = cs;
	if (rcw > 0)
		_rc_width = rcw;
	valid = true;
}

bool scroll_bar::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto et = (event_type2)type;
	glm::ivec2 poss = mps - _pos;
	glm::ivec2 ss = _size;
	poss -= tps;
	int px = _dir ? 0 : 1;
	int tsm = thumb_size_m.x + tps[_dir] * 2.0;
	auto pts = poss[_dir];
	pts -= _offset;
	switch (et)
	{
	case event_type2::on_click:
	{
		if (!d_drag)
		{
			hover = true;
			if (pts < 0) {
				t_offset = 0;
			}
			if (pts > thumb_size_m.x) {
				t_offset = thumb_size_m.x;
			}
			t_offset = thumb_size_m.x * 0.5;
			set_posv(poss);
			hover = false;
		}
	}
	break;
	case event_type2::on_move:
	{
		auto pts = poss[_dir];
		pts -= _offset;
		if (pts < 0 || pts > thumb_size_m.x)
		{
			_tcc = _color.y;
		}
		else {
			_tcc = _color.z;
		}
		scale_s = scale_s0.y;
	}
	break;
	case event_type2::mouse_up:
		hover = false;
		break;
	case event_type2::on_down:
	{
		d_drag = false;
		t_offset = pts;
		if (pts < 0 || pts > thumb_size_m.x)
		{
			hover = false;
		}
		else {
			hover = true;
		}
	}
	break;
	case event_type2::on_scroll:
	{
		if (thumb_size_m.z > 0 && ((_bst & (int)BTN_STATE::STATE_HOVER) || hover_sc && (parent && parent->_hover)))
		{
			auto st = ss[_dir] - tsm;
#if 0
			auto pts = (-mps.y * _pos_width) + _offset;
#else
			int64_t pw = (-mps.y * _pos_width);
			c_offset += pw;//内容偏移
			int64_t mxst = st * scale_w;
			c_offset = std::max((int64_t)0, std::min(c_offset, mxst));
			pts = c_offset / scale_w;
#endif
			if (limit)
			{
				if (pts < 0)pts = 0;
				if (pts > st)pts = st;
			}
			_offset = pts;// 滚动滑块偏移
			return true;
		}
	}
	break;
	case event_type2::on_drag:
	{
		poss += curpos;
		if (hover)
		{
			set_posv(poss);
			d_drag = true;
		}
	}
	break;
	default:
		break;
	}
	return false;
}

double dcscroll(double cp, double isx, double scroll_increment_x, int& scrollx)
{
	double ret = .0;
	if (cp < scrollx)
	{
		ret = floor(std::max(0.0, cp - scroll_increment_x));
		scrollx = ret;
	}
	else if (cp - isx >= scrollx && isx > 0)
	{
		ret = floor(cp - isx + scroll_increment_x);
		scrollx = ret;
	}
	return ret;
}

//template <typename T>
//inline T clamp(T v, T mn, T mx) { return (v < mn) ? mn : (v > mx) ? mx : v; }
// 输入：视图大小、内容大小、滚动宽度
// 输出：x水平大小，y垂直大小，z为水平的滚动区域宽，w垂直滚动高
glm::vec4 calcu_scrollbar_size(const glm::vec2 vps, const glm::vec2 content_width, const glm::vec2 scroll_width, const glm::ivec4& count)
{
	auto scw = scroll_width;
	scw.x = scroll_width.x * count.x + count.z;
	scw.y = scroll_width.y * count.y + count.w;
	auto dif = vps - scw;
	bool isx = (dif.x < content_width.x) && vps.x < content_width.x;
	bool isy = (dif.y < content_width.y) && vps.y < content_width.y;
	int inc = (isx ? 1 : 0) + (isy ? 1 : 0);
	if (!isy)
		scw.x -= count.z;
	if (!isx)
		scw.y -= count.w;
	auto calc_cb = [](double vw, double cw, double scw, double sw)
		{
			double win_size_contents_v = cw,
				win_size_avail_v = vw - scw,
				scrollbar_size_v = win_size_avail_v;//scrollbar_size_v是实际大小
			auto win_size_v = std::max(std::max(win_size_contents_v, win_size_avail_v), 1.0);
			auto GrabMinSize = sw;			// std::clamp
			auto grab_h_pixels = glm::clamp(scrollbar_size_v * (win_size_avail_v / win_size_v), GrabMinSize, scrollbar_size_v);
			auto grab_h_norm = grab_h_pixels / scrollbar_size_v;
			return glm::vec2{ grab_h_pixels, scrollbar_size_v };
		};
	auto x = calc_cb(vps.x, content_width.x, scw.x, scroll_width.x);
	auto y = calc_cb(vps.y, content_width.y, scw.y, scroll_width.y);
	if (!(x.x < x.y) || !isx)
		x.x = 0;
	if (!(y.x < y.y) || !isy)
		y.x = 0;
	glm::vec4 ret = { x.x, y.x, x.y, y.y };
	return ret;
}

// 输入：视图大小、内容大小、滚动宽度 ,count
// 输出：x水平大小，y为水平的滚动区域宽，z是否显示滚动条
glm::vec3 calcu_scrollbar_size(int vps, int content_width, int scroll_width, int count)
{
	//计算去掉按钮时视图大小
	auto scw = scroll_width;
	scw = scroll_width * count;
	auto dif = vps - scw;
	bool isx = (dif < content_width) && vps < content_width;

	auto calc_cb = [](double vw, double cw, double scw, double sw)
		{
			double win_size_contents_v = cw,
				win_size_avail_v = vw - scw,
				scrollbar_size_v = win_size_avail_v;
			auto win_size_v = std::max(std::max(win_size_contents_v, win_size_avail_v), 1.0);
			auto GrabMinSize = sw;
			auto grab_h_pixels = std::clamp(scrollbar_size_v * (win_size_avail_v / win_size_v), GrabMinSize, scrollbar_size_v);
			auto grab_h_norm = grab_h_pixels / scrollbar_size_v;
			return glm::vec2{ grab_h_pixels, scrollbar_size_v };
		};
	auto x = calc_cb(vps, content_width, scw, scroll_width);
	if (!(x.x < x.y) || !isx)
		x.x = 0;
	return glm::vec3(x.x, x.y, isx);
}

uint32_t get_reverse_color(uint32_t color) {
	uint8_t* c = (uint8_t*)&color;
	c[0] = 255 - c[0];
	c[1] = 255 - c[1];
	c[2] = 255 - c[2];
	return color;
}

size_t char2pos(size_t ps, const char* str) {
	return md::get_utf8_count(str, ps);
}


bool scroll_bar::update(float delta)
{
	// 箭头、滑块按钮初始大小一样
	bool r = valid;
	if (valid)
	{
		int vrc = _view_size - _rc_width;
		auto vs = _view_size;
		int px = _dir ? 0 : 1;
		glm::ivec2 ss = _size;
		auto pxs = (ss[px] - _rc_width) * 0.5;
		auto ss1 = ss[_dir];
		auto ss2 = _pos[_dir];
		glm::ivec3 sbs = calcu_scrollbar_size(vs, _content_size, pxs, 2);
		tps = { pxs,pxs };
		sbs.x = ss1 - (_content_size - vs);

		int tsm = tps[_dir] * 2.0;
		if (sbs.z > 0) {
			if (sbs.x < _rc_width)
			{
				sbs.x = _rc_width * 2;
			}
			// 滚动条宽度-滑块宽度-边框偏移
			auto vci = ss1 - sbs.x - tsm;
			scale_w = abs((double)(_content_size - (vs)) / vci);
			//assert(!(scale_w < 1));
			if (scale_w < 1)
			{
				scale_w = 1;
			}
		}
		else {
			_offset = 0; c_offset = scale_w * _offset;
		}
		thumb_size_m = sbs;
		valid = false;
	}
	if (!(_bst & (int)BTN_STATE::STATE_HOVER)) {
		if (!(_bst & (int)BTN_STATE::STATE_ACTIVE))
		{
			_tcc = _color.y; scale_s = scale_s0.x;
		}
	}
	if ((_bst & (int)BTN_STATE::STATE_ACTIVE) && _color.w) {
		_tcc = _color.w;
	}
	return r;
}



int64_t scroll_bar::get_offset()
{
	return scale_w * _offset;
}
int64_t scroll_bar::get_offset_ns()
{
	return c_offset;
}

int scroll_bar::get_range()
{
	glm::ivec2 ss = _size;
	int tsm = thumb_size_m.x + tps[_dir] * 2.0;
	auto st = ss[_dir] - tsm;
	return st;
}

void scroll_bar::set_offset(int pts)
{
	glm::ivec2 ss = _size;
	int tsm = thumb_size_m.x + tps[_dir] * 2.0;
	auto st = ss[_dir] - tsm;
	if (limit)
	{
		if (pts < 0)pts = 0;
		if (pts > st)pts = st;
	}
	_offset = pts;
	c_offset = scale_w * _offset;
}
void scroll_bar::set_offset_inc(int inc)
{
	set_offset(_offset + inc);
}

void scroll_bar::set_posv(const glm::ivec2& poss)
{
	if (!hover || thumb_size_m.z <= 0)return;
	glm::ivec2 ss = _size;
	int px = _dir ? 0 : 1;
	int tsm = thumb_size_m.x + tps[_dir] * 2.0;
	auto pts = poss[_dir];
	pts -= t_offset;
	int mx = ss[_dir] - tsm;
	if (limit)
	{
		if (pts < 0)pts = 0;
		if (pts > mx)pts = mx;
	}
	_offset = pts;
	c_offset = scale_w * _offset;
}
#endif

// todo draw ct
#if 1

void draw_color_btn(rvg_cx* rv, color_style* t, const glm::ivec2& pos, const glm::ivec2& size)
{
	auto ns = size;
	int thickness = t->thickness;
	auto psv = pos;
	auto view = glm::ivec4(psv, ns + thickness);
	rv->push_view(view, t);
	rv->translate(psv);
	if (t->dfill)
	{
		if (t->circle)
		{
			glm::vec2 sp = {};
			auto r = lround(ns.y * 0.5);
			sp += r;
			rv->add_circle(sp, r);
		}
		else
		{
			rv->add_rect({ 0.,0., ns }, t->rounding);
		}
		rv->submit(t->dfill, 0, 0);
	}
	// 渲染标签
	glm::vec2 ps = { thickness * 2, thickness * 2 };
	if (t->mPushed) {
		ps += t->pushedps;
	}
	ns -= thickness * 4;
	glm::vec4 rc = { ps, ns };
	if (t->dcol)
	{
		if (t->circle)
		{
			glm::vec2 sp = {};
			auto r = lround(ns.y * 0.5);
			sp += r;
			rv->add_circle(sp, r);
		}
		else
		{
			rv->add_rect({ 0.5,0.5, ns }, t->rounding);
		}
		rv->submit(0, t->dcol, thickness);
	}
	text_st tx = {};
	tx.pos = ps;
	tx.size = ns;
	tx.text = t->str; tx.text_len = t->str_len;
	rv->add_text(&tx, t->ptext_style);
	rv->pop_view();
}


void widget_t::draw(rvg_cx* rv)
{}

void image_btn::draw(rvg_cx* rv) {
	auto ss = get_size();
	auto psv = get_spos(); psv += _pos; //auto psv = get_ppos();
	auto view = glm::ivec4(psv, ss);
	rv->push_view(view, this);
	rv->translate(psv);
	//rv->add_rect({ 0.,0., ss }, rounding);
	//rv->submit(0x80ffffff, 0, 0); 
	auto p = &state_img[show_idx];
	image_r r = {};
	r.img = (image_ptr_t*)(imgptr.img);
	r.rc = p->tex_rc;// glm::ivec4(0, 0, ss);		// 所在纹理区域
	r.sliced = p->sliced;	// 九宫格
	r.dsize = { p->img_rc.z, p->img_rc.w };	// 渲染大小
	r.pos = psv + glm::ivec2(p->img_rc);		// 渲染坐标
	r.color = -1;		// 混合颜色
	r.type = img_type;
	rv->add_image(&r);

	text_st tx = {};
	tx.pos = {};
	tx.size = get_size();
	tx.text_len = str.size();
	if (tx.text_len > 0) {
		tx.text = str.c_str();
		rv->add_text(&tx, &style);
	}

	rv->pop_view();
}

void color_btn::draw(rvg_cx* rv)
{
	draw_color_btn(rv, &cs, get_spos() + glm::ivec2(_pos), get_size());
}

void gradient_btn::draw(rvg_cx* rv)
{
#if 1
	auto p = this;
	auto psv = get_spos(); psv += _pos; //auto psv = get_ppos();
	float x = psv.x, y = psv.y, w = p->_size.x, h = p->_size.y;
	int pushed = p->mPushed ? 0 : 1;
	uint32_t gradTop = p->_gradTop;
	uint32_t gradBot = p->_gradBot;
	uint32_t borderDark = p->borderDark;
	uint32_t borderLight = p->borderLight;
	double oa = p->opacity;
	auto ns = p->_size;

	auto bc = effect == uTheme::light ? p->back_color : set_alpha_xf2(p->back_color, get_alpha_f(p->back_color));
	double rounding = p->rounding;
	glm::vec2 ns1 = { w * 0.5, h * 0.5 };
	auto nr = (int)std::min(ns1.x, ns1.y);
	if (rounding > nr)
	{
		rounding = nr;
	}
	glm::vec2 tps = { 0.5,0.5 };

	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view, this);
	//rv->save();
	rv->translate({ x, y });
	if (is_alpha(bc))
	{
		bc = set_alpha_f(bc, oa);
		rv->add_rect({ thickness, thickness, w - thickness * 2, h - thickness * 2 }, rounding);
		rv->set_color(bc);
		rv->fill();
	}
	if (p->mPushed) {
		gradTop = set_alpha_f(gradTop, 0.8f);
		gradBot = set_alpha_f(gradBot, 0.8f);
	}
	else {
		double v = 1.0 - get_alpha_f(p->back_color);
		auto gv = p->mEnabled ? v : v * .5f + .5f;
		gradTop = set_alpha_xf(gradTop, gv);
		gradBot = set_alpha_xf(gradBot, gv);
	}
	auto gt = to_c4(gradTop);
	auto gt1 = to_c4(gradBot);
	gradTop = set_alpha_xf(gradTop, oa);
	gradBot = set_alpha_xf(gradBot, oa);
	borderLight = set_alpha_xf(borderLight, oa);
	borderDark = set_alpha_xf(borderDark, oa);
	// 渐变
	glm::vec4 r;
	if (rounding > 0)
	{
		r = { rounding, rounding, rounding, rounding };
	}
	glm::vec2 rct = { w - thickness, h - thickness * 2.0 };
	glm::vec4 gtop = to_c4(gradTop);
	glm::vec4 gbot = to_c4(gradBot);

	//rv->save();
	if (effect == uTheme::dark || p->mPushed)
		rv->translate({ thickness, thickness });
	//else
	//	rv->translate({ 0, 0 });
	rv->paint_shadow(0, rct.y, rct.x, rct.y, gtop, gbot, 0, rounding);// 垂直方向
	if (effect == uTheme::dark || p->mPushed)
		rv->translate({ -thickness, -thickness });
	//rv->restore();
	// 渲染标签

	glm::vec2 ps = { thickness * 2,thickness * 2 };
	if (p->mPushed) {
		ps += thickness;
	}
	ns -= thickness * 4;
	glm::vec4 rc = { ps, ns };
	// 边框
	w -= 1;
	h -= 1;
	rv->set_line_width(thickness);
	rv->set_color(borderLight);
	rv->add_rect({ tps.x,tps.y + (p->mPushed ? 0.f : 1.0f), w, h - (p->mPushed ? 0.0f : 1.0f) }, rounding);
	rv->stroke();
	rv->set_color(borderDark);
	rv->add_rect({ tps.x,tps.y, w , h }, rounding);
	rv->stroke();
	text_st tx = {};
	tx.pos = { 0,thickness * .5 };
	tx.size = get_size();
	tx.text = p->str.c_str(); tx.text_len = p->str.size();
	rv->add_text(&tx, &style);

	//rv->restore();

	rv->pop_view();

#endif
}


void menu_btn::draw(rvg_cx* rv)
{

}



void draw_radios(rvg_cx* rv, radio_info_t* p, radio_style_t* ps)
{
	if (ps->radius > 0) {
		rv->add_circle(p->pos, ps->radius);
		if (p->value || p->swidth > 0)
			rv->submit(ps->col, 0, ps->thickness);
		else
			rv->submit(ps->innc * 0, ps->line_col, ps->thickness);
	}
	if (p->swidth > 0) {
		rv->add_circle(p->pos, p->swidth);
		rv->submit(ps->innc, 0, ps->thickness);
	}
}


// check打勾
void drawCheckMark(rvg_cx* rv, glm::vec2 pos, uint32_t col, float sz1, bool mixed)
{
	if (!col)
		return;
	float sz = sz1;
	float thickness = std::max(sz / 5.0f, 1.0f);
	sz -= thickness * 0.5f;
	pos += glm::vec2(thickness * 0.25f, thickness * 0.25f);

	float third = sz / 3.0f;
	float bx = pos.x + third;
	float td = sz - third * 0.5f;
	float by = pos.y + td;
	if (mixed)
	{
		td = thickness * 0.5f;
		auto ps = glm::vec2(pos.x + td, by - third);
		auto ps1 = ps;
		ps1.x += sz1 - thickness;
		rv->add_line(ps, ps1);
	}
	else {
		glm::vec2 ps[3];
		ps[0] = glm::vec2(bx - third, by - third);
		ps[1] = glm::vec2(bx, by);
		ps[2] = glm::vec2(bx + third * 2.0f, by - third * 2.0f);
		rv->add_polyline(ps, 3);
	}
	rv->submit(0, col, thickness);
}
void draw_checkbox(rvg_cx* rv, check_style_t* p, checkbox_info_t* pn)
{
	if (!p)return;
	auto cc = p->check_col;
	glm::ivec2 ps = pn->pos;
	rv->add_rect({ ps.x,ps.y,p->square_sz,p->square_sz }, p->rounding);
	if (pn->value || pn->new_alpha > 0)
		rv->submit(p->fill, p->col, p->thickness);
	else
		rv->submit(0, p->line_col, p->thickness);
	const float pad = std::max(1.0f, floor(p->square_sz / 6.0f));
	if (pn->new_alpha > 0)
	{
		cc = set_alpha_f(cc, pn->new_alpha);
		drawCheckMark(rv, (glm::vec2)ps + glm::vec2(pad, pad), cc, p->square_sz - pad * 2.0f, pn->mixed);
	}
}

void radio_tl::draw(rvg_cx* rv)
{
	auto p = this;
	if (rv && p) {
#if 1

		auto psv = get_spos(); psv += _pos; //auto psv = get_ppos();
		auto view = glm::ivec4(psv, get_size());
		rv->push_view(view, this);

		//rv->save();
		rv->translate((glm::vec2)psv);
		//rv->add_rect({ 0,0,get_size() }, 0);
		//rv->set_color(0);
		//rv->fill();
		rv->translate(glm::vec2(_size.x * 0.5 - style.radius, _size.y * 0.5 - style.radius));//+ glm::vec2(0.5f, 0.5f)
		int x = 0;
		{
			auto& it = v;
			it.pos = {};
			it.pos.x = x * style.radius * 0.5;
			it.pos += style.radius;
			draw_radios(rv, &it, &style);
			x++;
		}
		//rv->restore();

		rv->pop_view();
#endif
	}
}

void checkbox_tl::draw(rvg_cx* rv)
{
	auto p = this;
	if (rv && p) {
#if 1

		auto psv = get_spos(); psv += _pos; //auto psv = get_ppos();
		auto view = glm::ivec4(psv, get_size());
		rv->push_view(view, this);

		//rv->save();
		glm::ivec2 poss = psv;
		poss.x += (_size.x - style.square_sz) * 0.5;
		poss.y += (_size.y - style.square_sz) * 0.5;
		rv->translate((glm::vec2)poss + glm::vec2(0.5f, 0.5f));
		int x = 0;
		{
			auto& it = v;
			it.pos.x = x * p->style.square_sz * 2.5;
			draw_checkbox(rv, &p->style, &it);
			x++;
		}
		//rv->restore();

		rv->pop_view();
#endif
	}
}
void switch_tl::draw(rvg_cx* rv)
{
#if 1
	auto psv = get_spos(); psv += _pos; //auto psv = get_ppos();
	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view, this);

	glm::ivec2 poss = psv;
	auto h = height;
	auto fc = h * cv * 0.5;
	auto ss = h * wf;
	if (_size.x <= 0) {
		_size.x = ss;
	}
	if (_size.y <= 0) {
		_size.y = h;
	}
	poss.x += (_size.x - ss) * 0.5;
	poss.y += (_size.y - height) * 0.5;
	rv->translate((glm::vec2)poss);
	rv->add_rect({ 0.5,0.5, ss, h }, h * 0.5);
	rv->submit(dcol, 0, 0);
	glm::vec2 cp = {};
	{
		auto ps = h * 0.5;
		cp.x += (ss - h) * cpos + h * 0.5;
		cp.y += ps;
		rv->add_circle(cp, fc);
		rv->submit(color.z, 0, 0);
	}
	rv->stroke();

	rv->pop_view();
#endif
}
void progress_tl::draw(rvg_cx* rv)
{
	glm::ivec2 ss = _size;
	ss.x = width;
	ss.y = height;
#if 1
	auto psv = get_spos(); psv += _pos; //auto psv = get_ppos();
	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view, this);
	glm::vec2 npos = _size - (glm::vec2)ss;
	npos *= 0.5;
	//rv->save();
	rv->translate(psv);
	//rv->add_rect({ 0,0, _size.x, _size.y }, 0);
	//rv->submit(0xff000000, 0, 0);
	rv->translate(npos);
	rv->add_rect({ 0.5,0.5, ss.x, ss.y }, rounding);
	rv->submit(color.y, 0, 0);
	double xx = ss.x * value;
	int kx = 0;
	int r = rounding;
	if (xx > 0)
	{
		//rv->save();
		glm::ivec4 oclip = {};
		if (xx < rounding * 2)
		{
			rv->add_rect({ 0,0, xx, ss.y }, r);
			rv->clip();
			oclip = rv->get_clip();
			xx = r * 2;
			kx = 1;
		}
		rv->add_rect({ 0.5,0.5, xx, ss.y }, r);
		rv->submit(color.x, 0, 0);
		if (oclip.z > 0)
			rv->clip(oclip);
		//rv->restore();
	}
	if (text.size()) {
		glm::ivec2 rk = {};// ltx->get_text_rect(0, font_size, text.c_str(), -1);
		if (text_inside) {
			ss.x = xx;
			ss.x -= r + thickness;
		}
		else {
			ss.x = _size.x;
		}
		if (kx) {
			ss.x += rk.x;
		}
		if (right_inside) {
			ss.x = _size.x - r - thickness;
		}
		glm::vec2 ta = { 1,0.5 };
		glm::vec4 rc = { 0, 0, _size };
		//text_style_t st = {};
		//st.font = 0;
		//st.text_align = ta;
		//st.font_size = font_size;
		//st.text_color = text_color;
		//draw_text(cr, ltx, text.c_str(), -1, rc, &st);

		text_style st = style;
		st.align = ta;
		text_st tx = {};
		tx.pos = { 0,thickness };
		tx.size = ss;
		tx.text = text.c_str(); tx.text_len = text.size();
		rv->add_text(&tx, &st);
	}
	//rv->restore();

	rv->pop_view();
#endif
}

void slider_tl::draw(rvg_cx* rv)
{
#if 1
	auto psv = get_spos(); psv += _pos; //auto psv = get_ppos();
	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view, this);
	//rv->save();
	glm::ivec2 ss = _size;
	rv->translate(psv);
	glm::vec4 brc = {}, cliprc, crc;
	int x = 0, y = 0;
	double xx = (ss[vertical]) * value;
	glm::vec2 spos = {};
	int kx = 0;
	int r = rounding;
	auto x1 = xx;
	if (xx < rounding * 2) {
		x1 = r * 2;
		kx = 1;
	}
	if (vertical)
	{
		x = (ss.x - wide) * 0.5;
		brc = { 0.5 + x ,0.5 + y , wide, ss.y };
		cliprc = { 0,0, wide, xx };
		xx = glm::clamp((float)xx, (float)0, (float)ss.y);
		spos = { ss.x * 0.5,xx };
		crc = { 0.5 + x,0.5 + y, wide, x1 };
		if (reverse_color) {
			x1 -= rounding;
			crc = { 0.5 + x,0.5 + y + x1 , wide, ss.y - x1 };
		}
	}
	else {
		y = (ss.y - wide) * 0.5;
		brc = { 0.5 + x ,0.5 + y, ss.x , wide };
		cliprc = { 0,0, xx, wide };
		xx = glm::clamp((float)xx, (float)0, (float)ss.x);
		spos = { xx ,ss.y * 0.5 };
		crc = { 0.5 + x,0.5 + y, x1, wide };
		if (reverse_color) {
			x1 -= rounding;
			crc = { 0.5 + x + x1 ,0.5 + y,ss.x - x1, wide };
		}
	}
	rv->add_rect(brc, rounding);
	rv->submit(color.y, 0, 0);
	if (xx >= 0)
	{
		{
			rv->add_rect(crc, r);
			rv->submit(color.x, 0, 0);
		}
		if (sl.x > 0) {
			rv->add_circle(spos, sl.x);
			rv->submit(sl.y, color.x, thickness);
		}
	}
	//rv->restore();

	rv->pop_view();
#endif
}


void colorpick_tl::draw(rvg_cx* rv)
{
#if 1
	auto psv = get_spos(); psv += _pos; //auto psv = get_ppos();
	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view, this);
	glm::ivec2 ss = _size;
	rv->translate(psv);
	auto oldpos = rv->get_translate();
	//rv->save();
	//uint32_t col_hues[] = { 0xff0000ff,0xff00ffff,0xff00ff00,0xffffff00,0xffff0000,0xffff00ff,0xff0000ff };
	const glm::vec4 col_hues[6 + 1] = { glm::vec4(1,0,0,1), glm::vec4(1,1,0,1), glm::vec4(0,1,0,1)
		, glm::vec4(0,1,1,1), glm::vec4(0,0,1,1), glm::vec4(1,0,1,1), glm::vec4(1,0,0,1) };
	int yh = height + step;
	glm::vec2 tps = { cpx + step,yh };
	glm::vec4 hc = HSVtoRGB(hsv);
	{
		glm::vec4 cc = {};
		rv->grid_fill({ cpx, height }, { -1,0xffdfdfdf }, height * 0.5);// 填充格子
		rv->add_rect({ 0,0,cpx, height }, 0);
		rv->set_color(hc);
		rv->fill();// 填充当前颜色
	}
	{
		rv->translate(tps);
		rv->linear_fill({ colorw,height }, col_hues, 7);	// H 
		glm::ivec4 rcc = { (colorw - step) * hsv.x,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		rv->add_rect(rcf, 0);
		rv->submit(-1, bc_color, thickness);
	}
	tps.x = 0;
	{
		rv->translate(tps);
		glm::vec4 cc[] = { {1,1,1,1}, hc };
		rv->grid_fill({ colorw, height }, { -1,-1 }, height * 0.5);// 背景色
		rv->linear_fill({ colorw, height }, cc, 2);	// S
		glm::ivec4 rcc = { (colorw - step) * hsv.y,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		rv->add_rect(rcf, 0);
		rv->submit(-1, bc_color, thickness);
	}
	{
		rv->translate(tps);
		glm::vec4 cc[] = { {0,0,0,1}, hc };
		rv->grid_fill({ colorw, height }, { -1,-1 }, height * 0.5);// 背景色
		rv->linear_fill({ colorw, height }, cc, 2);	// V
		glm::ivec4 rcc = { (colorw - step) * hsv.z,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		rv->add_rect(rcf, 0);
		rv->submit(-1, bc_color, thickness);
	}
	{
		rv->translate(tps);
		glm::vec4 cc[] = { {0,0,0,0}, {1,1,1,1} };
		rv->grid_fill({ colorw, height }, { -1,0xffdfdfdf }, height * 0.5);//背景色
		rv->linear_fill({ colorw, height }, cc, 2);	// A
		glm::ivec4 rcc = { (colorw - step) * hsv.w,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		rv->add_rect(rcf, 0);
		rv->submit(-1, bc_color, thickness);
	}
	auto oldpos1 = rv->get_translate();
	rv->translate(oldpos - oldpos1);
	//rv->restore();
	{
		glm::ivec2 ss = _size;
		glm::vec2 ta = { 0.0, 0.0 };
		glm::vec4 rc = { step, height + step, ss };
		rc.w -= rc.y;
		text_style st = style;
		st.align = ta;// text_align; 
		st.lineheight = rc.y;//设置固定行高
		rc.y += step;
		text_st tx = {};
		tx.pos = { 0,thickness };
		tx.pos = { rc.x,rc.y };
		tx.size = { rc.z,rc.w };
		tx.text = hsvstr.c_str(); tx.text_len = hsvstr.size();
		rv->add_text(&tx, &st);
		rc.y = 0; rc.x += cpx;
		rc.z = colorw;
		rc.w = height; ta.y = 0.5;
		st.lineheight = 0; // 使用默认行高
		tx.pos = { rc.x,rc.y };
		tx.size = { rc.z,rc.w };
		tx.text = colorstr.c_str(); tx.text_len = colorstr.size();
		rv->add_text(&tx, &st);
	}

	rv->pop_view();
#endif
}
void scroll_bar::draw(rvg_cx* rv)
{
	glm::ivec2 ss = _size;
	auto psv = _pos; //auto psv = get_ppos();
	auto view = glm::ivec4(psv, get_size());
	rv->push_view(view, this);
	rv->translate(psv);
	// 背景
	if (!hideble || thumb_size_m.z) {
		rv->add_rect({ 0,0,ss }, rounding);
		rv->submit(_color.x, 0, 0);
	}
	// 滑块
	double rw = _rc_width * scale_s;
	glm::ivec4 trc = { 0,0,rw,rw };
	int px = _dir ? 0 : 1;
	auto pxs = ceil((ss[px] - rw) * 0.5);
	trc.x += pxs;
	trc.y += pxs;
	trc[_dir] = tps[_dir] + _offset;
	trc[2 + _dir] = thumb_size_m.x;
	if (thumb_size_m.z)
	{
		rv->add_rect(trc, _rc_width * 0.5 * scale_s);
		rv->submit(_tcc, 0, 0);
	}
	rv->pop_view();
}


#endif // 1


#if 1
widget_t::widget_t()
{}
widget_t::widget_t(WIDGET_TYPE wt) :wtype(wt)
{}
widget_t::~widget_t()
{}
void widget_t::set_pos(const glm::ivec2& ps)
{
	_pos = ps; valid = true;
}
void widget_t::set_size(const glm::vec2& ss)
{
	_size = ss; valid = true;
}
glm::vec2 widget_t::get_size()
{
	return _size;
}
bool widget_t::on_mevent(int type, const glm::vec2& mps, void* e)
{
	return false;
}
void widget_t::on_event(uint32_t type, et_un_t* ep) {

}
bool widget_t::update(float delta)
{
	return false;
}

glm::ivec2 widget_t::get_pos(bool has_parent)
{
	glm::ivec2 ps = _pos;
	if (parent) {
		auto pss = parent->get_pos();
		auto ss = parent->get_spos();
		ss *= hscroll;
		ps += ss;
		if (has_parent) { ps += pss; }
	}
	return ps;
}

glm::ivec2 widget_t::get_ppos()
{
	glm::ivec2 ps = {};
	if (parent) {
		auto pss = parent->get_ppos();
		auto ss = parent->get_spos();
		ss *= hscroll;
		ps += _pos;
		ps += ss + pss;
	}
	return ps;
}

glm::ivec2 widget_t::get_spos()
{
	glm::ivec2 ps = {};
	if (parent) {
		auto ss = parent->get_spos();
		ss *= hscroll;
		ps += ss;
	}
	return ps;
}

void widget_t::set_family(font_family_t* family, int fontsize)
{
	if (family)
		style.family = family;
	if (style.fontsize == 0 && fontsize != 0)
		style.fontsize = fontsize;
}

void widget_t::set_editing(const std::string& str, const glm::ivec2& cpos, int lineheight)
{}


input_state_t* get_input_state_cx(void* ptr, int t);
// 通用控件鼠标事件处理 type有on_move/on_scroll/on_drag/on_down/on_up/on_click/on_dblclick/on_tripleclick
bool widget_on_move(widget_t* wp, uint32_t type, et_un_t* ep, const glm::vec2& pos) {
	bool hover = false;
	if (!wp)return hover;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	if (t == devent_type_e::mouse_move_e)
	{
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y }; mps -= pos;
		wp->mmpos = mps;
		auto gpos = wp->get_pos(false);
		// 判断是否鼠标进入 
		glm::vec4 trc = { gpos + wp->fpos, wp->_size };
		auto k = check_box_cr1(mps, &trc, 1, sizeof(glm::vec4));
		if (k.x) {
			bool hoverold = wp->_bst & (int)BTN_STATE::STATE_HOVER;
			wp->_bst |= (int)BTN_STATE::STATE_HOVER;   hover = true;
			if (!(wp->_bst & (int)BTN_STATE::STATE_ACTIVE))// 不是鼠标则独占
				ep->ret = 1;
			if (!hoverold)
			{
				// 鼠标进入
				wp->on_mevent((int)event_type2::on_enter, mps, p);
				if (wp->mevent_cb) {
					wp->mevent_cb(wp, (int)event_type2::on_enter, mps);
				}
			}
		}
		else {
			if (wp->_bst & (int)BTN_STATE::STATE_HOVER)
			{
				wp->_bst &= ~(int)BTN_STATE::STATE_HOVER;
				// 鼠标离开
				wp->on_mevent((int)event_type2::on_leave, mps, p);
				if (wp->mevent_cb) {
					wp->mevent_cb(wp, (int)event_type2::on_leave, mps);
				}
			}
		}

		{
			if (wp->_bst & (int)BTN_STATE::STATE_HOVER)
			{
				wp->on_mevent((int)event_type2::on_move, mps, p);
				if (wp->mevent_cb)
				{
					wp->mevent_cb(wp, (int)event_type2::on_move, mps);
				}
			}
			if (wp->_bst & (int)BTN_STATE::STATE_ACTIVE) {
				auto dps = mps - wp->curpos;
				bool first = !wp->has_drag;
				wp->has_drag = true;
				if (first) {
					wp->on_mevent((int)event_type2::on_dragstart, dps, p);
					if (wp->mevent_cb)
					{
						wp->mevent_cb(wp, (int)event_type2::on_dragstart, dps);
					}
				}
				else
				{
					wp->on_mevent((int)event_type2::on_drag, dps, p);		// 拖动事件
					if (wp->mevent_cb)
					{
						wp->mevent_cb(wp, (int)event_type2::on_drag, dps);		// 拖动事件
					}
				}
			}
		}
	}
	return hover;
}

void widget_on_event(widget_t* wp, uint32_t type, et_un_t* ep, const glm::vec2& pos) {
	if (!wp)return;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	wp->form = ep->form;
	switch (t)
	{
	case devent_type_e::mouse_move_e:
		widget_on_move(wp, type, ep, pos);
		break;
	case devent_type_e::mouse_button_e:
	{
		auto p = e->b;
		glm::ivec2 mps = { p->x,p->y }; mps -= pos;
		bool isd = wp->cmpos == mps; // 判断坐标是否改变
		wp->cmpos = mps;
		auto dv = dynamic_cast<div_cx*>(wp);
		bool cs = false;
		if (dv) {
			cs = dv->hittest(mps + (glm::ivec2)pos);
		}
		if (!cs && wp->_bst & (int)BTN_STATE::STATE_HOVER) {
			if (p->down == 1)
			{
				ep->ret = 1;
			}
			if (p->button == 1) {
				if (p->down == 1) {
					wp->_bst |= (int)BTN_STATE::STATE_ACTIVE;
					wp->curpos = mps - (glm::ivec2)wp->_pos;
					wp->cks = 0;
					wp->on_mevent((int)event_type2::on_down, mps, p);
					if (wp->mevent_cb) { wp->mevent_cb(wp, (int)event_type2::on_down, mps); }
				}
				else {
					if (wp->_bst & (int)BTN_STATE::STATE_ACTIVE)
					{
						wp->cks = p->clicks;
						if (wp->has_drag)
						{
						}
						else
						{
							wp->on_mevent((int)event_type2::on_up, mps, p);
							if (wp->mevent_cb) {
								wp->mevent_cb(wp, (int)event_type2::on_up, mps);
							}
							int tc = (int)event_type2::on_click; //左键单击
							if (p->clicks == 2) { tc = (int)event_type2::on_dblclick; }
							else if (p->clicks == 3) { tc = (int)event_type2::on_tripleclick; }
							wp->_clicks = p->clicks;
							wp->on_mevent(tc, mps, p);
							if (wp->mevent_cb) {
								wp->mevent_cb(wp, tc, mps);
							}
							if (wp->click_cb)
							{
								wp->click_cb(wp, p->clicks, mps);
							}
						}
					}
					wp->_bst &= ~(int)BTN_STATE::STATE_ACTIVE;
				}
			}
		}
		if (p->down == 0) {
			wp->_bst &= ~(int)BTN_STATE::STATE_ACTIVE;
			wp->_bst |= (int)BTN_STATE::STATE_NOMAL;
			if (wp->has_drag)
			{
				wp->on_mevent((int)event_type2::on_dragend, mps, p);
				if (wp->mevent_cb) {
					wp->mevent_cb(wp, (int)event_type2::on_dragend, mps);
				}
			}
			wp->has_drag = false;
			wp->on_mevent((int)event_type2::mouse_up, mps, p);
			if (wp->mevent_cb) {
				wp->mevent_cb(wp, (int)event_type2::mouse_up, mps);
			}
		}
	}
	break;
	case devent_type_e::mouse_wheel_e:
	{
		auto p = e->w;
		glm::vec2 mps = { p->x, p->y };
		if (wp->_bst & (int)BTN_STATE::STATE_HOVER || wp->has_hover_sc)
		{
			ep->ret = wp->on_mevent((int)event_type2::on_scroll, mps, p);
			if (wp->mevent_cb) {
				wp->mevent_cb(wp, (int)event_type2::on_scroll, mps);
			}
			ep->ret = 1;
		}
	}
	break;
	case devent_type_e::keyboard_e:
	{
		// todo
		//on_keyboard(ep);
	}
	break;
	default:
		break;
	}

}
void send_hover(widget_t* wp, const glm::vec2& mps) {

	wp->on_mevent((int)event_type2::on_hover, mps, nullptr);
	if (wp->mevent_cb)
	{
		wp->mevent_cb(wp, (int)event_type2::on_hover, mps);
	}
}

bool on_wpe(widget_t* pw, int type, et_un_t* ep, const glm::ivec2& ppos)
{
	bool r = false;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	widget_on_event(pw, type, ep, ppos);
	if (pw->on_event_cb)
	{
		pw->on_event_cb(type, ep, ppos);
	}
	else
	{
		if (ep->ret && t == devent_type_e::mouse_button_e)
		{
			auto p = e->b;
			if (p->down == 1 && pw->_bst & (int)event_type2::on_down)
			{
				get_input_state_cx(0, 1);
			}
		}
	}
	return (ep->ret);
}

div_cx::div_cx()
{
	hscroll = {};
}

div_cx::~div_cx()
{
	auto ac = (hz::usp_ac*)flex_ctx_ac(lctx);
	free_flex_ctx(lctx);
	if (ac)
		delete ac;
	for (auto p : widgets)
	{
		if (p && p->_autofree)
			delete p;
	}
}
void div_cx::add_widget(widget_t* p)
{
	if (p)
	{
		p->set_family(style.family, style.fontsize);
		auto par = dynamic_cast<div_cx*>(p->parent);
		if (par) {
			par->remove_widget(p);
		}
		p->parent = this;
		auto it = std::find(widgets.begin(), widgets.end(), p);
		if (it == widgets.end()) {
			tadd.push_back(p);
		}
		uplayout = true;
	}
}
void div_cx::remove_widget(widget_t* p)
{
	if (p)
	{
		tremove.push_back(p);
		uplayout = true;
	}
}
void div_cx::set_scroll(int width, int rcw, const glm::ivec2& pos_width, const glm::ivec2& vnpos, const glm::ivec2& hnpos)
{
	auto pss = _size;
	{
		auto cp = new_scroll_bar({ width,pss.y - width * 2 }, pss.y, pss.y, rcw, true, vnpos);
		bind_scroll_bar(cp, true); // 绑定垂直滚动条
		cp->_pos_width = pos_width.y > 0 ? pos_width.y : width * 2;//滚轮事件每次滚动量
		cp->hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->has_hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->hscroll = {};
		cp->_absolute = true;
		cp->rounding = std::max(2, (int)(width * 0.5));
	}
	{
		auto cp = new_scroll_bar({ pss.x - width * 2,width }, pss.x, pss.x, rcw, false, hnpos);
		bind_scroll_bar(cp, false); // 绑定水平滚动条
		cp->_pos_width = pos_width.x > 0 ? pos_width.x : width;
		cp->hscroll = {};
		cp->_absolute = true;
		cp->rounding = std::max(2, (int)(width * 0.5));
	}
}
void div_cx::set_scroll_hide(bool is)
{
	if (horizontal)
		horizontal->hideble = is;
	if (vertical)
		vertical->hideble = is;
}
void div_cx::set_scroll_pos(const glm::ivec2& ps, bool v)
{
	if (v)
	{
		if (vertical)
			vertical->_pos = ps;
	}
	else
	{
		if (horizontal)
			horizontal->_pos = ps;
	}
}
void div_cx::set_scroll_size(const glm::ivec2& ps, bool v)
{
	if (v)
	{
		if (vertical)
			vertical->_size = ps;
	}
	else
	{
		if (horizontal)
			horizontal->_size = ps;
	}
}
void div_cx::set_view(const glm::ivec2& view_size, const glm::ivec2& content_size)
{
	if (horizontal)
		horizontal->set_viewsize(view_size.x, content_size.x, 0);
	if (vertical)
		vertical->set_viewsize(view_size.y, content_size.y, 0);
}
void div_cx::set_scroll_visible(const glm::ivec2& hv)
{
	if (horizontal)//水平滚动条
	{
		horizontal->visible = hv.x;
	}
	//垂直滚动条
	if (vertical)
	{
		vertical->visible = hv.y;
	}
}
glm::ivec2 div_cx::get_scroll_range()
{
	glm::ivec2 r = {};
	if (horizontal)//水平滚动条
	{
		r.x = horizontal->get_range();
	}
	//垂直滚动条
	if (vertical)
	{
		r.y = vertical->get_range();
	}
	return r;
}
void div_cx::set_scroll_pts(const glm::ivec2& pts, int t)
{
	//水平滚动条
	if (horizontal)
	{
		if (t == 0)
			horizontal->set_offset(pts.x);
		else
			horizontal->set_offset_inc(pts.x);

	}
	//垂直滚动条
	if (vertical)
	{
		if (t == 0)
			vertical->set_offset(pts.y);
		else
			vertical->set_offset_inc(pts.y);
	}
}
void div_cx::sortdg()
{
	std::stable_sort(dragsp.begin(), dragsp.end(), [](const drag_v6* t1, const drag_v6* t2) { return t1->z < t2->z; });
}
void div_cx::bind_scroll_bar(scroll_bar* p, bool v)
{
	if (p)
	{
		if (v)
		{
			if (vertical)
				delete vertical;
			vertical = p;
		}
		else
		{
			if (horizontal)
				delete horizontal;
			horizontal = p;
		}
		remove_widget(p);
	}
}
void div_cx::on_motion(const glm::vec2& pos)
{
	glm::ivec2 ps = pos;
	if (ckinc > 0)
	{
		//if (draggable)
		//	set_pos(ps - curpos);
		//else
		//	set_scroll_pts(ps - curpos, 1);

		div_ev e = {};
		e.p = this; e.down = 1; e.clicks = 0; e.mpos = ps;
		e.drag = true;
		if (on_click)
		{
			on_click(&e);	// 执行拖动事件
		}
		//for (auto& c : on_mouses)
		//{
		//	c(&e);
		//}
	}

	//tv->on_motion(ps - tpos);
	//update(0);
}
void div_cx::on_button(int idx, int down, const glm::vec2& pos, int clicks, int r)
{
	glm::ivec2 ps = pos;
	if (idx == 1)
	{
		glm::vec4 trc = glm::vec4(0, 0, get_size());
		auto k2 = check_box_cr1(ps, &trc, 1, sizeof(glm::vec4));

		if (k2.x)
		{
			if (draggable && down == 1)
			{
				//todo form_move2end(form, this); // 移动窗口前面
			}
			if (!r)
			{
				if (down == 1 && ckinc == 0)
				{
					//curpos = ps - tpos;
					ckinc++;
				}
				div_ev e = {};
				e.p = this; e.down = down; e.clicks = clicks; e.mpos = ps;
				if (on_click)
				{
					on_click(&e);	// 执行单击事件
				}
				//for (auto& c : on_mouses)
				//{
				//	c(&e);
				//}
			}
			ckup = 1;
		}
		else {
			ckup = 0;
			_hover = false;
		}
		if (down == 0)
		{
			if (ckinc)
				ckup = 1;
			ckinc = 0;
		}
	}
	//tv->on_button(idx, down, ps - tpos, clicks);
	//update(0);
	//_draw_valid = true;
}
void div_cx::on_wheel(double x, double y)
{
	//update(0);
}
scroll_bar* div_cx::new_scroll_bar(const glm::ivec2& size, int vs, int cs, int rcw, bool v, const glm::ivec2& npos)
{
	auto p = new scroll_bar();
	if (p)
	{
		add_widget(p);
		p->_absolute = true;
		p->_size = size;
		auto ss = get_size();
		glm::ivec2 dw = {};
		if (!v)
		{
			if (p->_size.x < 0)
				p->_size.x = ss.x - border.z * 2;
			p->_pos.y = ss.y - border.y;
			p->_pos.x = border.z;
			dw.y = 1;
			p->_dir = 0;
		}
		if (v)
		{
			if (p->_size.y < 0)
				p->_size.y = ss.y - border.z * 2;
			p->_pos.x = ss.x - (border.y);
			p->_pos.y = border.z;
			dw.x = 1;
			p->_dir = 1;
		}
		if (p->_size.x < rcw) {
			p->_size.x = rcw;
		}
		if (p->_size.y < rcw) {
			p->_size.y = rcw;
		}
		dw *= p->_size;
		p->_pos -= dw;
		p->_pos -= npos;
		p->set_viewsize(vs, cs, rcw);
	}
	return p;
}
scroll2_t div_cx::new_scroll2(const glm::ivec2& viewsize, int width, int rcw, const glm::ivec2& pos_width, const glm::ivec2& vnpos, const glm::ivec2& hnpos)
{
	scroll2_t r = {};
	auto pss = viewsize;
	{
		auto cp = new_scroll_bar({ width,pss.y - width * 2 }, pss.y, pss.y, rcw, true, vnpos);
		cp->_pos_width = pos_width.y > 0 ? pos_width.y : width * 2;//滚轮事件每次滚动量
		cp->hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->has_hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->hscroll = {};
		cp->rounding = std::max(2, (int)(width * 0.5));
		r.v = cp;
	}
	{
		auto cp = new_scroll_bar({ pss.x - width * 2,width }, pss.x, pss.x, rcw, false, hnpos);
		cp->_pos_width = pos_width.x > 0 ? pos_width.x : width;
		cp->hscroll = {};
		cp->rounding = std::max(2, (int)(width * 0.5));
		r.h = cp;
	}
	return r;
}
void div_cx::on_event(uint32_t type, et_un_t* ep)
{
	auto e = &ep->v;
	if (!visible)return;
	auto t = (devent_type_e)type;
	//glm::ivec2 vgpos = viewport;
	int r1 = 0;
	auto ppos = get_pos() + fpos;
	auto sps = get_spos();
	_hover_eq.w = type;
	widget_t* hpw = 0;

	if (t == devent_type_e::mouse_move_e)
	{
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y };
		glm::vec4 trc = glm::vec4(ppos, get_size());
		if (!parent)
		{
			auto k2 = check_box_cr1(mps, &trc, 1, sizeof(glm::vec4));
			if (k2.x) {
				_bst |= (int)BTN_STATE::STATE_HOVER;
				r1 = 1;
				p->cursor = (int)cursor_st::cursor_arrow;
				_hover = true;
				if (_move_pos != mps)
				{
					_move_pos = mps;
					_hover_eq.x = 0;
				}
				//printf("_hover\n");
			}
			else {
				_move_pos = mps;
				_hover_eq.z = 0;
				_bst &= ~(int)BTN_STATE::STATE_HOVER;
				if (ckinc == 0)
					_hover = false;
				//printf("on_leave\n");
			}
		}
		if (horizontal)
		{
			widget_on_event(horizontal, type, ep, ppos);// 水平滚动条
		}
		if (vertical) {
			widget_on_event(vertical, type, ep, ppos);// 垂直滚动条 
		}
		for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
			auto pw = *it;
			if (!pw || !pw->visible || pw->_disabled_events)continue;
			auto vpos = sps * pw->hscroll;
			on_wpe(pw, type, ep, ppos + vpos);
		}
		if (!parent)
			on_wpe(this, type, ep, {});

		event_wts.clear();
		event_wts1.clear();
		if (horizontal) {
			horizontal->_bst& (int)BTN_STATE::STATE_HOVER ? event_wts.push_back(horizontal) : event_wts1.push_back(horizontal);//水平滚动条
		}
		if (vertical) {
			vertical->_bst& (int)BTN_STATE::STATE_HOVER ? event_wts.push_back(vertical) : event_wts1.push_back(vertical);//垂直滚动条
		}
		for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
			if ((*it)->_bst & (int)BTN_STATE::STATE_HOVER)
				event_wts.push_back(*it);
			else
				event_wts1.push_back(*it);
		}
		if (this && !parent) {
			if (this->_bst & (int)BTN_STATE::STATE_HOVER) {
				event_wts.push_back(this);
			}
			else {
				event_wts1.push_back(this);//本容器
			}
		}
		auto length = event_wts.size();
		{
			// 生成鼠标离开消息
			for (size_t i = 1; i < length; i++) {
				auto pw = event_wts[i];
				if (!pw || !pw->visible || pw->_disabled_events)continue;
				auto vpos = sps * pw->hscroll;
				auto p = e->m;
				glm::ivec2 mps = { p->x,p->y };
				mps -= ppos + vpos;
				if (pw == this)mps -= ppos;
				bool isd = pw->cmpos == mps;
				pw->cmpos = mps;
				pw->_bst &= ~(int)BTN_STATE::STATE_HOVER;
				// 鼠标离开
				pw->on_mevent((int)event_type2::on_leave, mps, p);
				if (pw->mevent_cb) {
					pw->mevent_cb(pw, (int)event_type2::on_leave, mps);
				}
			}
		}

	}
	else
	{
		bool btn = !(t == devent_type_e::mouse_button_e && e->b->down == 0);// 弹起判断
		int icc = 0;
		auto length = event_wts.size();
		for (size_t i = 0; i < length; i++)
		{
			auto pw = event_wts[i];
			icc++;
			if (!pw || !pw->visible || pw->_disabled_events)continue;
			auto vpos = sps * pw->hscroll + ppos;
			if (pw == this)
				vpos = {};
			on_wpe(pw, type, ep, vpos);
			if (ep->ret && btn && pw != this) {
				hpw = pw;
				break;
			}
		}
		if (!hpw)
		{
			auto ln = event_wts1.size();
			for (size_t i = 0; i < ln; i++)
			{
				auto pw = event_wts1[i];
				if (!pw || !pw->visible || pw->_disabled_events)continue;
				auto vpos = sps * pw->hscroll + ppos;
				if (pw == this)
					vpos = {};
				on_wpe(pw, type, ep, vpos);
				if (ep->ret && btn) {
					hpw = pw; break;
				}
			}
		}
	}
	if (!ep->ret)
		ep->ret = r1;
	for (auto pt : widgets) {
		if (pt)
			pt->on_event(type, ep);
	}
	switch (t)
	{
	case devent_type_e::mouse_move_e:
	{
		auto length = event_wts.size();
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y };
		mps -= ppos;
		on_motion(mps);
		_hover_eq.z = (length > 0) ? 1 : 0;// 悬停准备
		if (ckinc > 0)
		{
			for (auto& it : drags)
			{
				if (it.ck > 0)
				{
					it.pos = mps - it.tp;	// 处理拖动坐标
					it.cp1 = mps;
				}
			}
		}
		else {
			for (auto& it : drags)
			{
				it.ck = 0;
			}
		}
	}
	break;
	case devent_type_e::mouse_button_e:
	{
		auto p = e->b;
		glm::ivec2 mps = { p->x,p->y };
		mps -= ppos;
		on_button(p->button, p->down, mps, p->clicks, ep->ret);

		if (p->button == 1) {
			if (p->down == 1) {
				drag_v6* dp = 0;

				for (auto vt = dragsp.rbegin(); vt != dragsp.rend(); vt++)
				{
					auto dp1 = *vt;
					auto& it = *dp1;
					it.z = 0;
					if (it.size.x > 0 && it.size.y > 0)
					{
						if (dp)continue;
						glm::vec4 trc = { it.pos + sps,it.size };
						auto k2 = check_box_cr1(mps, &trc, 1, sizeof(glm::vec4));
						if (k2.x)
						{
							it.tp = mps - it.pos;	// 记录当前拖动坐标
							it.ck = 1;
							it.cp1 = it.cp0 = mps;
							it.z = 1;
							dp = dp1;
						}
					}
				}
				if (!dp)
				{
					for (auto vt = dragsp.rbegin(); vt != dragsp.rend(); vt++)
					{
						auto dp1 = *vt;
						auto& it = *dp1;
						if (it.size.x == 0 || it.size.y == 0)
						{
							it.z = 0;
							it.cp1 = it.cp0 = mps;
							it.tp = mps - it.pos;	// 记录当前拖动坐标
							it.ck = 1;
						}
					}
				}
				else
				{
					for (auto vt = dragsp.rbegin(); vt != dragsp.rend(); vt++)
					{
						auto dp1 = *vt;
						auto& it = *dp1;
						if (it.size.x == 0 && it.size.y == 0)
						{
							it.z = 0;
							it.cp1 = it.cp0 = mps;
						}
					}
					sortdg();
				}
			}
		}

		_hover_eq.z = 0;
		//printf("ck:%d\t%p\n", ckinc, this);
		if (ckup > 0)
			ep->ret = 1;
	}
	break;
	case devent_type_e::mouse_wheel_e:
	{
		auto p = e->w;
		on_wheel(p->x, p->y);
		if (_hover)
		{
			ep->ret = 1;		// 滚轮独占本事件
		}
	}
	break;
	case devent_type_e::keyboard_e:
	{
		//on_keyboard(ep);
	}
	break;
	}
	evupdate++;
}
bool div_cx::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto t = (event_type2)type;
	bool ret = false;
	auto p = (mouse_move_et*)e;
	auto mpos = mps;
	glm::vec2 tpos = {};
	tpos += thickness;
	switch (t)
	{
	case event_type2::on_move:
	{
	}break;
	case event_type2::on_leave:
	{
	}break;
	case event_type2::on_down:
	{
		mpos -= tpos + _pos;
		if (draggable)
			dindex = 1;
		ret = true;
		valid = true;
	}break;
	case event_type2::on_drag:
	{
		if (draggable)
		{
			//printf("draggable %.1f %.1f\n", mps.x, mps.y);
			set_pos(mps);
		}
		ret = true;
	}break;
	case event_type2::mouse_up:
	{
		if (draggable)
		{
			dindex = 0;
		}
		valid = true;
		ret = true;
	}break;
	};
	return false;
}
bool vht(widget_t** widgets, size_t count, const glm::ivec2& p) {
	bool r = false;
	for (size_t i = 0; i < count; i++) {
		auto pw = widgets[i];
		if (!pw || !pw->visible || pw->_disabled_events)continue;
		glm::vec2 mps = p;
		mps -= pw->get_pos();
		// 判断是否鼠标在控件上
		glm::vec4 ppos = { 0,0,pw->_size };
		auto k = check_box_cr1(mps, &ppos, 1, sizeof(glm::vec4));
		if (k.x) { r = true; break; }
	}
	return r;
}
bool div_cx::hittest(const glm::ivec2& pos)
{
	bool r = false;
	auto sps = get_spos();
	glm::ivec2 ips = get_pos() + fpos;
	auto ss = (glm::ivec2)_size;
	glm::vec4 rc = { ips ,ips + ss };
	if (rect_includes(rc, pos)) {
		r = vht(widgets.data(), widgets.size(), pos);
		if (!r) {
			widget_t* pws[2] = { horizontal, vertical };
			r = vht(pws, 2, pos);
		}
	}
	return r;
}

bool div_cx::press_test()
{
	bool r = false;
	auto sps = get_spos();
	glm::ivec2 ips = get_pos(); auto ss = (glm::ivec2)_size;
	glm::vec4 rc = { ips ,ips + ss };

	if (ckinc > 0 || _bst & (int)BTN_STATE::STATE_ACTIVE) {
		r = true;
	}
	else
	{
		// 按下鼠标时点中控件则捕获
		for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
			auto pw = (widget_t*)*it;
			if (!pw || !pw->visible || pw->_disabled_events)continue;
			if (pw->_bst & (int)BTN_STATE::STATE_ACTIVE) {
				r = true;
			}
		}
	}
	return r;
}

size_t div_cx::add_dragpos(const glm::ivec2& pos, const glm::ivec2& size)
{
	auto ps = drags.size();
	drag_v6 t = {};
	t.pos = pos;
	t.size = size;
	t.z = 0;
	drags.push_back(t);
	update_drag = true;
	return ps;
}

void div_cx::remove_dragpos(size_t idx)
{
	drags.erase(drags.begin() + idx); update_drag = true;
}

glm::ivec3 div_cx::get_dragpos(size_t idx)
{
	glm::ivec3 r = (idx < drags.size()) ? glm::ivec3(drags[idx].pos, drags[idx].z) : glm::ivec3();
	r += glm::ivec3(get_spos(), 0);
	return r;
}

drag_v6* div_cx::get_dragv6(size_t idx)
{
	return (idx < drags.size()) ? &drags[idx] : nullptr;
}

bool div_cx::update(float delta)
{
	int ic = 0;

	if (tadd.size()) {
		for (auto p : tadd)
			widgets.push_back(p);
		tadd.clear();
	}
	if (tremove.size())
	{
		auto& v = widgets;
		auto ps = v.size();
		for (auto p : tremove) {
			v.erase(std::remove(v.begin(), v.end(), p), v.end());
		}
		tremove.clear();
	}
	if (update_drag)
	{
		dragsp.clear();
		for (auto& it : drags) { dragsp.push_back(&it); }
		sortdg();
		update_drag = false; ic++;
	}
	if (delta > 0)
	{
		for (auto& it : widgets) {
			ic += it->update(delta);
			if (ic > 0)
				ic = ic;
		}
	}
	if (uplayout)
	{
		clayout(); ic++;
	}
	if (valid) {
		sort_draw.clear();
		sort_draw.reserve(widgets.size());
		for (auto p : widgets) {
			sort_draw.push_back(p);
		}
		std::stable_sort(sort_draw.begin(), sort_draw.end(), [](widget_t* p1, widget_t* p2) {return p1->dindex < p2->dindex; });
		valid = false;
		ic++;
	}
	if (dindex)ic++;
	return ic > 0;
}
void div_cx::draw(rvg_cx* rv)
{
	auto pos = get_ppos();
	auto sps = get_spos();	// 获取滚动量
	auto ss = get_size();
	if (border.w || border.x) {
		rv->push_view(glm::ivec4(0, 0, ss), this);
		rv->set_line_width(border.y);
		//rv->translate(pos);
		glm::vec2 rc = ss;
		rc -= border.y;
		rv->add_rect({ 0.5,0.5,rc }, border.z);
		rv->fill_stroke(border.w, border.x);
		if (text.size()) {
			glm::ivec2 rk = {};
			glm::vec2 ta = { 0.5,0.5 };
			glm::vec4 rc = { 0, 0, _size };
			text_style st = style;
			st.align = ta;
			text_st tx = {};
			tx.pos = { 0,thickness };
			tx.size = ss;
			tx.text = text.c_str(); tx.text_len = text.size();
			rv->add_text(&tx, &st);
		}
		rv->pop_view();
	}

	for (auto& it : sort_draw) {
		auto dp = dynamic_cast<div_cx*>(it);
		if (it->visible && !dp)
		{
			it->draw(rv);
		}
	}
	draw_last(rv);
}

void div_cx::draw_last(rvg_cx* rv)
{}
void div_cx::set_editing(const std::string& str, const glm::ivec2& cpos, int lineheight)
{
	editingstr = str;
	editpos = cpos;
	line_height = lineheight;
}

glm::vec2 get_margin_size(flex_data* c) {
	glm::vec2 m = {};
	if (c)
	{
		m.x = c->margin_left + c->margin_right;
		m.y = c->margin_top + c->margin_bottom;
	}
	return m;
}
// 计算一行布局，range为当前行的控件起始索引和数量，tempfv为临时使用的数组
glm::vec2 calc_line_layout(widget_t** p, const glm::ivec2& range, const glm::ivec2& box_size, flex_data* boxflex, flex_data* flex_child
	, const glm::ivec2& pos, std::vector<node_dt>& tempfv, flex_ctx* ctx)
{
	int count = range.y;
	tempfv.resize(count + 1);
	auto cpos = pos;
	node_dt* fnode = tempfv.data();
	auto cp = fnode + 1;
	*fnode = {};
	flex_data tf[2] = {};
	if (boxflex)
	{
		tf[0] = *boxflex;
		fnode->size = box_size;
		fnode->child = cp;
		fnode->child_count = count;
		fnode->line_count = 0;
	}
	glm::vec2 margin_size = {};
	glm::vec2 margin_pos = {};
	if (flex_child) {
		tf[1] = *flex_child;
		if (tf[0].wrap > flex_wrap::NO_WRAP)
		{
			margin_size = get_margin_size(flex_child);
			margin_pos.x = flex_child->margin_left;
			margin_pos.y = flex_child->margin_top;
			cpos += margin_pos;
			tf[1].margin_left = 0;
			tf[1].margin_right = 0;
			tf[1].margin_top = 0;
			tf[1].margin_bottom = 0;
		}
	}
	for (int i = 0; i < count; i++)
	{
		auto it = p[range.x + i];
		cp[i] = {};
		cp[i].index = 1;
		cp[i].size = it->get_size() + margin_size;
		cp[i].position = it->_absolute ? 1 : 0;
	}
	auto nrc = flex_layout_calc(tf, 2, fnode, fnode->child_count + 1, ctx);
	for (size_t y = 0; y < fnode->child_count; y++)
	{
		auto t0 = fnode->child + y;
		auto& bt = p[range.x + y];
		if (bt->_absolute)
		{
			continue;
		}
		glm::vec2 vps2 = t0->frame;
		vps2 += cpos;
		bt->set_pos(vps2);
	}
	return glm::vec2(nrc.z, nrc.w);
}
void div_cx::clayout()
{
	if (!uplayout)return;
	uplayout = false;
	valid = true;
	widget_t** p = widgets.data();
	glm::vec2 pos = {};
	if (!lctx) {
		lctx = new_flex_ctx();
		flex_ctx_set_ac(lctx, new hz::usp_ac());
	}
	if (lines.empty())
	{
		// 竖排
		calc_line_layout(p, { 0,widgets.size() }, { _size.x,_size.y }, &flex, &flex_child, pos, tempfv, lctx);
	}
	else {
		for (auto& it : lines)
		{
			auto cs = calc_line_layout(p, it, { _size.x,_size.y }, &flex, &flex_child, pos, tempfv, lctx);
			pos.y += cs.y;
		}
	}
	return;
}


page_cx::page_cx()
{}

page_cx::~page_cx()
{}


#endif // 1




#if 1

// A cardinal direction
enum GuiDir : int
{
	GuiDir_None = -1,
	GuiDir_Left = 0,
	GuiDir_Right = 1,
	GuiDir_Up = 2,
	GuiDir_Down = 3,
	GuiDir_COUNT
};

#define STB_TEXTEDIT_CHARTYPE   char
#define STB_TEXTEDIT_STRING     text_control

// get the base type

#ifndef STB_TEXTEDIT_UNDOSTATECOUNT
#define STB_TEXTEDIT_UNDOSTATECOUNT   99
#endif
#ifndef STB_TEXTEDIT_UNDOCHARCOUNT
#define STB_TEXTEDIT_UNDOCHARCOUNT   999
#endif
#ifndef STB_TEXTEDIT_CHARTYPE
#define STB_TEXTEDIT_CHARTYPE        int
#endif
#ifndef STB_TEXTEDIT_POSITIONTYPE
#define STB_TEXTEDIT_POSITIONTYPE    int
#endif

struct StbUndoRecord
{
	// private data
	STB_TEXTEDIT_POSITIONTYPE  where;
	STB_TEXTEDIT_POSITIONTYPE  insert_length;
	STB_TEXTEDIT_POSITIONTYPE  delete_length;
	int                        char_storage;
};

struct StbUndoState
{
	// private data
	StbUndoRecord          undo_rec[STB_TEXTEDIT_UNDOSTATECOUNT];
	STB_TEXTEDIT_CHARTYPE  undo_char[STB_TEXTEDIT_UNDOCHARCOUNT];
	short undo_point, redo_point;
	int undo_char_point, redo_char_point;
};

struct STB_TexteditState
{
	/////////////////////
	//
	// public data
	//

	int cursor;
	// position of the text cursor within the string

	int select_start;          // selection start point
	int select_end;
	// selection start and end point in characters; if equal, no selection.
	// note that start may be less than or greater than end (e.g. when
	// dragging the mouse, start is where the initial click was, and you
	// can drag in either direction)

	bool insert_mode;
	// each textfield keeps its own insert mode state. to keep an app-wide
	// insert mode, copy this value in/out of the app state

	int row_count_per_page;
	// page size in number of row.
	// this value MUST be set to >0 for pageup or pagedown in multilines documents.

	/////////////////////
	//
	// private data
	//
	bool cursor_at_end_of_line; // not implemented yet
	bool initialized;
	bool has_preferred_x;
	bool single_line;
	bool padding1, padding2, padding3;
	float preferred_x; // this determines where the cursor up/down tries to seek to along x
	StbUndoState undostate;
};


////////////////////////////////////////////////////////////////////////
//
//     StbTexteditRow
//
// Result of layout query, used by stb_textedit to determine where
// the text in each row is.

// result of layout query
struct StbTexteditRow
{
	float x0, x1;             // starting x location, end x location (allows for align=right, etc)
	float baseline_y_delta;  // position of baseline relative to previous row's baseline
	float ymin, ymax;         // height of row above and below baseline
	int num_chars;
};

// define our editor structure
struct text_control
{
	STB_TexteditState state = {};
	std::string str;
	size_t curline = 0;				// 当前行号
	size_t curline_idx = 0;			// 当前行号偏移
	font_family_t* family = 0;
	int font_size = 16;
	int c_ct = 0;
	int c_d = 0;
	int64_t ccursor8 = 0;	//当前光标字符
	int64_t caret_old = {};		//保存输入光标
	glm::ivec3 cursor_pos = {};
	glm::ivec2 scroll_pos = {};				// 滚动坐标
	glm::ivec2 _align_pos = {};				// 对齐坐标
	glm::ivec2 _align_pos1 = {};			// 对齐坐标
	glm::vec2 view = {};	// 区域

	std::vector<glm::ivec2> lvs;// 行开始结束
	std::vector<std::vector<int>> widths;// 字符偏移
	std::vector<glm::ivec4> rangerc;
	PathsD range_path;						// 圆角选区缓存
	path_v ptr_path;
	float round_path = 0.28;				// 圆角比例
	float _r_posy = -1;						// 选区偏移

	int lineheight = 0;
	int8_t LastMoveDirectionLR = 0;
	bool is_scroll = true;
};

// define all the #defines needed 

#define KEYDOWN_BIT                    0x80000000

#define STB_TEXTEDIT_STRINGLEN(tc)     ((tc)->str.size())
#define STB_TEXTEDIT_LAYOUTROW         stb_textedit_layoutrow
#define STB_TEXTEDIT_GETWIDTH		   stb_textedit_getwidth
#define STB_TEXTEDIT_KEYTOTEXT(key)    (((key) & KEYDOWN_BIT) ? 0 : (key))
#define STB_TEXTEDIT_GETCHAR(tc,i)     ((tc)->str[i])
#define STB_TEXTEDIT_NEWLINE           '\n'
#define STB_TEXTEDIT_IS_SPACE(ch)      isspace(ch)
#define STB_TEXTEDIT_DELETECHARS       delete_chars
#define STB_TEXTEDIT_INSERTCHARS       insert_chars

#define STB_TEXTEDIT_K_SHIFT           0x40000000
#define STB_TEXTEDIT_K_CONTROL         0x20000000
#define STB_TEXTEDIT_K_LEFT            (KEYDOWN_BIT | 1) // actually use VK_LEFT, SDLK_LEFT, etc
#define STB_TEXTEDIT_K_RIGHT           (KEYDOWN_BIT | 2) // VK_RIGHT
#define STB_TEXTEDIT_K_UP              (KEYDOWN_BIT | 3) // VK_UP
#define STB_TEXTEDIT_K_DOWN            (KEYDOWN_BIT | 4) // VK_DOWN
#define STB_TEXTEDIT_K_LINESTART       (KEYDOWN_BIT | 5) // VK_HOME
#define STB_TEXTEDIT_K_LINEEND         (KEYDOWN_BIT | 6) // VK_END
#define STB_TEXTEDIT_K_TEXTSTART       (STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_TEXTEND         (STB_TEXTEDIT_K_LINEEND   | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_DELETE          (KEYDOWN_BIT | 7) // VK_DELETE
#define STB_TEXTEDIT_K_BACKSPACE       (KEYDOWN_BIT | 8) // VK_BACKSPACE
#define STB_TEXTEDIT_K_UNDO            (KEYDOWN_BIT | STB_TEXTEDIT_K_CONTROL | 'z')
#define STB_TEXTEDIT_K_REDO            (KEYDOWN_BIT | STB_TEXTEDIT_K_CONTROL | 'y')
#define STB_TEXTEDIT_K_INSERT          (KEYDOWN_BIT | 9) // VK_INSERT
#define STB_TEXTEDIT_K_WORDLEFT        (STB_TEXTEDIT_K_LEFT  | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_WORDRIGHT       (STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_PGUP            (KEYDOWN_BIT | 10) // VK_PGUP -- not implemented
#define STB_TEXTEDIT_K_PGDOWN          (KEYDOWN_BIT | 11) // VK_PGDOWN -- not implemented

#define STB_TEXTEDIT_GETWIDTH_NEWLINE -1

#ifndef STB_TEXTEDIT_memmove
#include <string.h>
#define STB_TEXTEDIT_memmove memmove
#endif

#ifndef STB_TEXTEDIT_GETPREVCHARINDEX
#define STB_TEXTEDIT_GETPREVCHARINDEX stb_textedit_getprevcharindex
#endif
#ifndef STB_TEXTEDIT_GETNEXTCHARINDEX
#define STB_TEXTEDIT_GETNEXTCHARINDEX stb_textedit_getnextcharindex
#endif


float stb_textedit_getwidth(STB_TEXTEDIT_STRING* str, int n, int idx);


// define the functions we need
void stb_textedit_layoutrow1(StbTexteditRow* row, STB_TEXTEDIT_STRING* str, int start_i)
{
	int remaining_chars = str->str.size() - start_i;
	row->num_chars = remaining_chars; // should do real word wrap here
	row->x0 = 0;
	row->ymin = 0;
	auto r = row;
	int i = start_i;
	float row_width = 0.0f;
	while (str->str[i] != '\n' && i < STB_TEXTEDIT_STRINGLEN(str)) {
		row_width += STB_TEXTEDIT_GETWIDTH(str, start_i, i);
		if (str->view.x > 0 && row_width > str->view.x) break; // 自动换行触发条件
		i++;
	}
	r->x1 = row_width;
	row->ymax = row->baseline_y_delta = str->lineheight;
	r->num_chars = (i - start_i) + 1;

}
void stb_textedit_layoutrow(StbTexteditRow* row, STB_TEXTEDIT_STRING* str, int start_i) {
	int remaining_chars = str->str.size() - start_i;
	row->num_chars = remaining_chars;
	row->x0 = 0;
	row->ymin = 0;
	auto r = row;
	int i = start_i;
	float row_width = 0.0f;
	int len = STB_TEXTEDIT_STRINGLEN(str);

	while (i < len) {
		if (str->str[i] == '\n') break;  // 遇到换行符退出

		float char_width = STB_TEXTEDIT_GETWIDTH(str, 0, i);
		float next_width = row_width + char_width;

		// 预判宽度是否超限
		if (str->view.x > 0 && next_width > str->view.x) break;

		row_width = next_width;
		i++;
	}

	// 显式处理换行符 
	if (i < len && str->str[i] == '\n') {
		i++;
	}

	r->x1 = row_width;
	row->ymax = row->baseline_y_delta = str->lineheight;
	r->num_chars = i - start_i;  // 修正字符数计算 
}

int delete_chars(STB_TEXTEDIT_STRING* str, int pos, int num)
{
	str->str.erase(pos, num);
	return num;
}

int insert_chars(STB_TEXTEDIT_STRING* str, int pos, const STB_TEXTEDIT_CHARTYPE* newtext, int num)
{
	str->str.insert(pos, newtext, num);
	return num;
}
//float STB_TEXTEDIT_GETWIDTH(ImGuiInputTextState* obj, int line_start_idx, int char_idx)
//{
//	unsigned int c; ImTextCharFromUtf8(&c, obj->TextSrc + line_start_idx + char_idx, obj->TextSrc + obj->TextLen);
//	if ((ImWchar)c == '\n') 
//		return IMSTB_TEXTEDIT_GETWIDTH_NEWLINE;
//	ImGuiContext& g = *obj->Ctx;
//	return g.FontBaked->GetCharAdvance((ImWchar)c) * g.FontBakedScale;
//}

// 行开始位置，字符位置
float stb_textedit_getwidth(STB_TEXTEDIT_STRING* str, int line_start_idx, int char_idx)
{
	if (str && str->family)
	{
		const char* p = str->str.c_str();
		uint32_t ch = 0;
		auto nn = md::utf8_to_unicode(p + line_start_idx + char_idx, &ch);
		if (ch == '\n') {
			return STB_TEXTEDIT_GETWIDTH_NEWLINE;
		}
		if (ch)
		{
			auto rc = font_get_char_extent(ch, str->font_size, str->family, 0);
			return rc.z;
		}
	}
	return 0;
}
// 返回上一个字符位置
int stb_textedit_getprevcharindex(STB_TEXTEDIT_STRING* str, int idx) {
	auto p = str->str.c_str();
	auto p1 = md::get_utf8_prev(p + idx);
	return (p1 - p);
}
// 返回下一个字符位置
int stb_textedit_getnextcharindex(STB_TEXTEDIT_STRING* str, int idx) {
	auto p = str->str.c_str() + idx;
	auto p1 = md::utf8_next_char(p);
	return (p1 - p) + idx;
}
/////////////////////////////////////////////////////////////////////////////
//
//      Mouse input handling
//

// traverse the layout to locate the nearest character to a display position
static int stb_text_locate_coord(STB_TEXTEDIT_STRING* str, float x, float y, int* out_side_on_line)
{
	StbTexteditRow r;
	int n = STB_TEXTEDIT_STRINGLEN(str);
	float base_y = 0, prev_x;
	int i = 0, k;

	r.x0 = r.x1 = 0;
	r.ymin = r.ymax = 0;
	r.num_chars = 0;
	*out_side_on_line = 0;

	// search rows to find one that straddles 'y'
	while (i < n) {
		STB_TEXTEDIT_LAYOUTROW(&r, str, i);
		if (r.num_chars <= 0)
			return n;

		if (i == 0 && y < base_y + r.ymin)
			return 0;

		if (y < base_y + r.ymax)
			break;

		i += r.num_chars;
		base_y += r.baseline_y_delta;
	}

	// below all text, return 'after' last character
	if (i >= n)
	{
		*out_side_on_line = 1;
		return n;
	}

	// check if it's before the beginning of the line
	if (x < r.x0)
		return i;

	// check if it's before the end of the line
	if (x < r.x1) {
		// search characters in row for one that straddles 'x'
		prev_x = r.x0;
		for (k = 0; k < r.num_chars; k = STB_TEXTEDIT_GETNEXTCHARINDEX(str, i + k) - i) {
			float w = STB_TEXTEDIT_GETWIDTH(str, i, k);
			if (x < prev_x + w) {
				*out_side_on_line = (k == 0) ? 0 : 1;
				if (x < prev_x + w / 2)
					return k + i;
				else
					return STB_TEXTEDIT_GETNEXTCHARINDEX(str, i + k);
			}
			prev_x += w;
		}
		// shouldn't happen, but if it does, fall through to end-of-line case
	}

	// if the last character is a newline, return that. otherwise return 'after' the last character
	*out_side_on_line = 1;
	if (STB_TEXTEDIT_GETCHAR(str, i + r.num_chars - 1) == STB_TEXTEDIT_NEWLINE)
		return i + r.num_chars - 1;
	else
		return i + r.num_chars;
}

// API click: on mouse down, move the cursor to the clicked location, and reset the selection
static void stb_textedit_click(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, float x, float y)
{
	// In single-line mode, just always make y = 0. This lets the drag keep working if the mouse
	// goes off the top or bottom of the text
	int side_on_line;
	if (state->single_line)
	{
		StbTexteditRow r;
		STB_TEXTEDIT_LAYOUTROW(&r, str, 0);
		y = r.ymin;
	}

	state->cursor = stb_text_locate_coord(str, x, y, &side_on_line);
	state->select_start = state->cursor;
	state->select_end = state->cursor;
	state->has_preferred_x = 0;
	str->LastMoveDirectionLR = (side_on_line ? GuiDir_Right : GuiDir_Left);
}

// API drag: on mouse drag, move the cursor and selection endpoint to the clicked location
static void stb_textedit_drag(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, float x, float y)
{
	int p = 0;
	int side_on_line;

	// In single-line mode, just always make y = 0. This lets the drag keep working if the mouse
	// goes off the top or bottom of the text
	if (state->single_line)
	{
		StbTexteditRow r;
		STB_TEXTEDIT_LAYOUTROW(&r, str, 0);
		y = r.ymin;
	}

	if (state->select_start == state->select_end)
		state->select_start = state->cursor;

	p = stb_text_locate_coord(str, x, y, &side_on_line);
	state->cursor = state->select_end = p;
	str->LastMoveDirectionLR = (side_on_line ? GuiDir_Right : GuiDir_Left);
}

/////////////////////////////////////////////////////////////////////////////
//
//      Keyboard input handling
//

// forward declarations
static void stb_text_undo(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
static void stb_text_redo(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
static void stb_text_makeundo_delete(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int length);
static void stb_text_makeundo_insert(STB_TexteditState* state, int where, int length);
static void stb_text_makeundo_replace(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int old_length, int new_length);

typedef struct
{
	float x, y;    // position of n'th character
	float height; // height of line
	int first_char, length; // first char of row, and length
	int prev_first;  // first char of previous row
} StbFindState;

// find the x/y location of a character, and remember info about the previous row in
// case we get a move-up event (for page up, we'll have to rescan)
static void stb_textedit_find_charpos(StbFindState* find, STB_TEXTEDIT_STRING* str, int n, int single_line)
{
	StbTexteditRow r;
	int prev_start = 0;
	int z = STB_TEXTEDIT_STRINGLEN(str);
	int i = 0, first;

	if (n == z && single_line) {
		// special case if it's at the end (may not be needed?)
		STB_TEXTEDIT_LAYOUTROW(&r, str, 0);
		find->y = 0;
		find->first_char = 0;
		find->length = z;
		find->height = r.ymax - r.ymin;
		find->x = r.x1;
		return;
	}

	// search rows to find the one that straddles character n
	find->y = 0;

	for (;;) {
		STB_TEXTEDIT_LAYOUTROW(&r, str, i);
		if (n < i + r.num_chars)
			break;
		if (str->LastMoveDirectionLR == GuiDir_Right && str->state.cursor > 0 && str->state.cursor == i + r.num_chars && STB_TEXTEDIT_GETCHAR(str, i + r.num_chars - 1) != STB_TEXTEDIT_NEWLINE) // [DEAR IMGUI] Wrapping point handling
			break;
		if (i + r.num_chars == z && z > 0 && STB_TEXTEDIT_GETCHAR(str, z - 1) != STB_TEXTEDIT_NEWLINE)  // [DEAR IMGUI] special handling for last line
			break;   // [DEAR IMGUI]
		prev_start = i;
		i += r.num_chars;
		find->y += r.baseline_y_delta;
		if (i == z) // [DEAR IMGUI]
		{
			r.num_chars = 0; // [DEAR IMGUI]
			break;   // [DEAR IMGUI]
		}
	}

	find->first_char = first = i;
	find->length = r.num_chars;
	find->height = r.ymax - r.ymin;
	find->prev_first = prev_start;

	// now scan to find xpos
	find->x = r.x0;
	for (i = 0; first + i < n; i = STB_TEXTEDIT_GETNEXTCHARINDEX(str, first + i) - first)
		find->x += STB_TEXTEDIT_GETWIDTH(str, first, i);
}

#define STB_TEXT_HAS_SELECTION(s)   ((s)->select_start != (s)->select_end)

// make the selection/cursor state valid if client altered the string
static void stb_textedit_clamp(STB_TEXTEDIT_STRING* str, STB_TexteditState* state)
{
	int n = STB_TEXTEDIT_STRINGLEN(str);
	if (STB_TEXT_HAS_SELECTION(state)) {
		if (state->select_start > n) state->select_start = n;
		if (state->select_end > n) state->select_end = n;
		// if clamping forced them to be equal, move the cursor to match
		if (state->select_start == state->select_end)
			state->cursor = state->select_start;
	}
	if (state->cursor > n) state->cursor = n;
}

// delete characters while updating undo
static void stb_textedit_delete(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int len)
{
	stb_text_makeundo_delete(str, state, where, len);
	STB_TEXTEDIT_DELETECHARS(str, where, len);
	state->has_preferred_x = 0;
}

// delete the section
static void stb_textedit_delete_selection(STB_TEXTEDIT_STRING* str, STB_TexteditState* state)
{
	stb_textedit_clamp(str, state);
	if (STB_TEXT_HAS_SELECTION(state)) {
		if (state->select_start < state->select_end) {
			stb_textedit_delete(str, state, state->select_start, state->select_end - state->select_start);
			state->select_end = state->cursor = state->select_start;
		}
		else {
			stb_textedit_delete(str, state, state->select_end, state->select_start - state->select_end);
			state->select_start = state->cursor = state->select_end;
		}
		state->has_preferred_x = 0;
	}
}

// canoncialize the selection so start <= end
static void stb_textedit_sortselection(STB_TexteditState* state)
{
	if (state->select_end < state->select_start) {
		int temp = state->select_end;
		state->select_end = state->select_start;
		state->select_start = temp;
	}
}

// move cursor to first character of selection
static void stb_textedit_move_to_first(STB_TexteditState* state)
{
	if (STB_TEXT_HAS_SELECTION(state)) {
		stb_textedit_sortselection(state);
		state->cursor = state->select_start;
		state->select_end = state->select_start;
		state->has_preferred_x = 0;
	}
}

// move cursor to last character of selection
static void stb_textedit_move_to_last(STB_TEXTEDIT_STRING* str, STB_TexteditState* state)
{
	if (STB_TEXT_HAS_SELECTION(state)) {
		stb_textedit_sortselection(state);
		stb_textedit_clamp(str, state);
		state->cursor = state->select_end;
		state->select_start = state->select_end;
		state->has_preferred_x = 0;
	}
}

// [DEAR IMGUI] Extracted this function so we can more easily add support for word-wrapping.
#ifndef STB_TEXTEDIT_MOVELINESTART
static int stb_textedit_move_line_start(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int cursor)
{
	if (state->single_line)
		return 0;
	while (cursor > 0) {
		int prev = STB_TEXTEDIT_GETPREVCHARINDEX(str, cursor);
		if (STB_TEXTEDIT_GETCHAR(str, prev) == STB_TEXTEDIT_NEWLINE)
			break;
		cursor = prev;
	}
	return cursor;
}
#define STB_TEXTEDIT_MOVELINESTART stb_textedit_move_line_start
#endif
#ifndef STB_TEXTEDIT_MOVELINEEND
static int stb_textedit_move_line_end(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int cursor)
{
	int n = STB_TEXTEDIT_STRINGLEN(str);
	if (state->single_line)
		return n;
	while (cursor < n && STB_TEXTEDIT_GETCHAR(str, cursor) != STB_TEXTEDIT_NEWLINE)
		cursor = STB_TEXTEDIT_GETNEXTCHARINDEX(str, cursor);
	return cursor;
}
#define STB_TEXTEDIT_MOVELINEEND stb_textedit_move_line_end
#endif

#ifdef STB_TEXTEDIT_IS_SPACE
static int is_word_boundary(STB_TEXTEDIT_STRING* str, int idx)
{
	return idx > 0 ? (STB_TEXTEDIT_IS_SPACE(STB_TEXTEDIT_GETCHAR(str, idx - 1)) && !STB_TEXTEDIT_IS_SPACE(STB_TEXTEDIT_GETCHAR(str, idx))) : 1;
}

#ifndef STB_TEXTEDIT_MOVEWORDLEFT
static int stb_textedit_move_to_word_previous(STB_TEXTEDIT_STRING* str, int c)
{
	c = STB_TEXTEDIT_GETPREVCHARINDEX(str, c); // always move at least one character
	while (c >= 0 && !is_word_boundary(str, c))
		c = STB_TEXTEDIT_GETPREVCHARINDEX(str, c);

	if (c < 0)
		c = 0;

	return c;
}
#define STB_TEXTEDIT_MOVEWORDLEFT stb_textedit_move_to_word_previous
#endif

#ifndef STB_TEXTEDIT_MOVEWORDRIGHT
static int stb_textedit_move_to_word_next(STB_TEXTEDIT_STRING* str, int c)
{
	const int len = STB_TEXTEDIT_STRINGLEN(str);
	c = STB_TEXTEDIT_GETNEXTCHARINDEX(str, c); // always move at least one character
	while (c < len && !is_word_boundary(str, c))
		c = STB_TEXTEDIT_GETNEXTCHARINDEX(str, c);

	if (c > len)
		c = len;

	return c;
}
#define STB_TEXTEDIT_MOVEWORDRIGHT stb_textedit_move_to_word_next
#endif

#endif

// update selection and cursor to match each other
static void stb_textedit_prep_selection_at_cursor(STB_TexteditState* state)
{
	if (!STB_TEXT_HAS_SELECTION(state))
		state->select_start = state->select_end = state->cursor;
	else
		state->cursor = state->select_end;
}

// API cut: delete selection
static int stb_textedit_cut(STB_TEXTEDIT_STRING* str, STB_TexteditState* state)
{
	if (STB_TEXT_HAS_SELECTION(state)) {
		stb_textedit_delete_selection(str, state); // implicitly clamps
		state->has_preferred_x = 0;
		return 1;
	}
	return 0;
}

// API paste: replace existing selection with passed-in text
static int stb_textedit_paste_internal(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, STB_TEXTEDIT_CHARTYPE* text, int len)
{
	// if there's a selection, the paste should delete it
	stb_textedit_clamp(str, state);
	stb_textedit_delete_selection(str, state);
	// try to insert the characters
	len = STB_TEXTEDIT_INSERTCHARS(str, state->cursor, text, len);
	if (len) {
		stb_text_makeundo_insert(state, state->cursor, len);
		state->cursor += len;
		state->has_preferred_x = 0;
		return 1;
	}
	// note: paste failure will leave deleted selection, may be restored with an undo (see https://github.com/nothings/stb/issues/734 for details)
	return 0;
}

#ifndef STB_TEXTEDIT_KEYTYPE
#define STB_TEXTEDIT_KEYTYPE int
#endif

// API key: process text input
// [DEAR IMGUI] Added stb_textedit_text(), extracted out and called by stb_textedit_key() for backward compatibility.
static void stb_textedit_text(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, const STB_TEXTEDIT_CHARTYPE* text, int text_len)
{
	// can't add newline in single-line mode
	if (text[0] == '\n' && state->single_line)
		return;

	if (state->insert_mode && !STB_TEXT_HAS_SELECTION(state) && state->cursor < STB_TEXTEDIT_STRINGLEN(str)) {
		stb_text_makeundo_replace(str, state, state->cursor, 1, 1);
		STB_TEXTEDIT_DELETECHARS(str, state->cursor, 1);
		text_len = STB_TEXTEDIT_INSERTCHARS(str, state->cursor, text, text_len);
		if (text_len) {
			state->cursor += text_len;
			state->has_preferred_x = 0;
		}
	}
	else {
		stb_textedit_delete_selection(str, state); // implicitly clamps
		text_len = STB_TEXTEDIT_INSERTCHARS(str, state->cursor, text, text_len);
		if (text_len) {
			stb_text_makeundo_insert(state, state->cursor, text_len);
			state->cursor += text_len;
			state->has_preferred_x = 0;
		}
	}
}

// API key: process a keyboard input
static void stb_textedit_key(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, STB_TEXTEDIT_KEYTYPE key)
{
retry:
	switch (key) {
	default: {
#ifdef STB_TEXTEDIT_KEYTOTEXT
		// This is not suitable for UTF-8 support.
		int c = STB_TEXTEDIT_KEYTOTEXT(key);
		if (c > 0) {
			STB_TEXTEDIT_CHARTYPE ch = (STB_TEXTEDIT_CHARTYPE)c;
			stb_textedit_text(str, state, &ch, 1);
		}
#endif
		break;
	}

#ifdef STB_TEXTEDIT_K_INSERT
	case STB_TEXTEDIT_K_INSERT:
		state->insert_mode = !state->insert_mode;
		break;
#endif

	case STB_TEXTEDIT_K_UNDO:
		stb_text_undo(str, state);
		state->has_preferred_x = 0;
		break;

	case STB_TEXTEDIT_K_REDO:
		stb_text_redo(str, state);
		state->has_preferred_x = 0;
		break;

	case STB_TEXTEDIT_K_LEFT:
		// if currently there's a selection, move cursor to start of selection
		if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_move_to_first(state);
		else
			if (state->cursor > 0)
				state->cursor = STB_TEXTEDIT_GETPREVCHARINDEX(str, state->cursor);
		state->has_preferred_x = 0;
		break;

	case STB_TEXTEDIT_K_RIGHT:
		// if currently there's a selection, move cursor to end of selection
		if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_move_to_last(str, state);
		else
			state->cursor = STB_TEXTEDIT_GETNEXTCHARINDEX(str, state->cursor);
		stb_textedit_clamp(str, state);
		state->has_preferred_x = 0;
		break;

	case STB_TEXTEDIT_K_LEFT | STB_TEXTEDIT_K_SHIFT:
		stb_textedit_clamp(str, state);
		stb_textedit_prep_selection_at_cursor(state);
		// move selection left
		if (state->select_end > 0)
			state->select_end = STB_TEXTEDIT_GETPREVCHARINDEX(str, state->select_end);
		state->cursor = state->select_end;
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_MOVEWORDLEFT
	case STB_TEXTEDIT_K_WORDLEFT:
		if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_move_to_first(state);
		else {
			state->cursor = STB_TEXTEDIT_MOVEWORDLEFT(str, state->cursor);
			stb_textedit_clamp(str, state);
		}
		break;

	case STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT:
		if (!STB_TEXT_HAS_SELECTION(state))
			stb_textedit_prep_selection_at_cursor(state);

		state->cursor = STB_TEXTEDIT_MOVEWORDLEFT(str, state->cursor);
		state->select_end = state->cursor;

		stb_textedit_clamp(str, state);
		break;
#endif

#ifdef STB_TEXTEDIT_MOVEWORDRIGHT
	case STB_TEXTEDIT_K_WORDRIGHT:
		if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_move_to_last(str, state);
		else {
			state->cursor = STB_TEXTEDIT_MOVEWORDRIGHT(str, state->cursor);
			stb_textedit_clamp(str, state);
		}
		break;

	case STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT:
		if (!STB_TEXT_HAS_SELECTION(state))
			stb_textedit_prep_selection_at_cursor(state);

		state->cursor = STB_TEXTEDIT_MOVEWORDRIGHT(str, state->cursor);
		state->select_end = state->cursor;

		stb_textedit_clamp(str, state);
		break;
#endif

	case STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_SHIFT:
		stb_textedit_prep_selection_at_cursor(state);
		// move selection right
		state->select_end = STB_TEXTEDIT_GETNEXTCHARINDEX(str, state->select_end);
		stb_textedit_clamp(str, state);
		state->cursor = state->select_end;
		state->has_preferred_x = 0;
		break;

	case STB_TEXTEDIT_K_DOWN:
	case STB_TEXTEDIT_K_DOWN | STB_TEXTEDIT_K_SHIFT:
	case STB_TEXTEDIT_K_PGDOWN:
	case STB_TEXTEDIT_K_PGDOWN | STB_TEXTEDIT_K_SHIFT: {
		StbFindState find;
		StbTexteditRow row;
		int i, j, sel = (key & STB_TEXTEDIT_K_SHIFT) != 0;
		int is_page = (key & ~STB_TEXTEDIT_K_SHIFT) == STB_TEXTEDIT_K_PGDOWN;
		int row_count = is_page ? state->row_count_per_page : 1;

		if (!is_page && state->single_line) {
			// on windows, up&down in single-line behave like left&right
			key = STB_TEXTEDIT_K_RIGHT | (key & STB_TEXTEDIT_K_SHIFT);
			goto retry;
		}

		if (sel)
			stb_textedit_prep_selection_at_cursor(state);
		else if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_move_to_last(str, state);

		// compute current position of cursor point
		stb_textedit_clamp(str, state);
		stb_textedit_find_charpos(&find, str, state->cursor, state->single_line);

		for (j = 0; j < row_count; ++j) {
			float x, goal_x = state->has_preferred_x ? state->preferred_x : find.x;
			int start = find.first_char + find.length;

			if (find.length == 0)
				break;

			// [DEAR IMGUI]
			// going down while being on the last line shouldn't bring us to that line end
			//if (STB_TEXTEDIT_GETCHAR(str, find.first_char + find.length - 1) != STB_TEXTEDIT_NEWLINE)
			//   break;

			// now find character position down a row
			state->cursor = start;
			stb_textedit_clamp(str, state);
			STB_TEXTEDIT_LAYOUTROW(&row, str, state->cursor);
			x = row.x0;
			for (i = 0; i < row.num_chars; ) {
				float dx = STB_TEXTEDIT_GETWIDTH(str, start, i);
				int next = STB_TEXTEDIT_GETNEXTCHARINDEX(str, state->cursor);
#ifdef STB_TEXTEDIT_GETWIDTH_NEWLINE
				if (dx == STB_TEXTEDIT_GETWIDTH_NEWLINE)
					break;
#endif
				x += dx;
				if (x > goal_x)
					break;
				i += next - state->cursor;
				state->cursor = next;
			}
			stb_textedit_clamp(str, state);

			if (state->cursor == find.first_char + find.length)
				str->LastMoveDirectionLR = GuiDir_Left;
			state->has_preferred_x = 1;
			state->preferred_x = goal_x;

			if (sel)
				state->select_end = state->cursor;

			// go to next line
			find.first_char = find.first_char + find.length;
			find.length = row.num_chars;
		}
		break;
	}

	case STB_TEXTEDIT_K_UP:
	case STB_TEXTEDIT_K_UP | STB_TEXTEDIT_K_SHIFT:
	case STB_TEXTEDIT_K_PGUP:
	case STB_TEXTEDIT_K_PGUP | STB_TEXTEDIT_K_SHIFT: {
		StbFindState find;
		StbTexteditRow row;
		int i, j, prev_scan, sel = (key & STB_TEXTEDIT_K_SHIFT) != 0;
		int is_page = (key & ~STB_TEXTEDIT_K_SHIFT) == STB_TEXTEDIT_K_PGUP;
		int row_count = is_page ? state->row_count_per_page : 1;

		if (!is_page && state->single_line) {
			// on windows, up&down become left&right
			key = STB_TEXTEDIT_K_LEFT | (key & STB_TEXTEDIT_K_SHIFT);
			goto retry;
		}

		if (sel)
			stb_textedit_prep_selection_at_cursor(state);
		else if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_move_to_first(state);

		// compute current position of cursor point
		stb_textedit_clamp(str, state);
		stb_textedit_find_charpos(&find, str, state->cursor, state->single_line);

		for (j = 0; j < row_count; ++j) {
			float  x, goal_x = state->has_preferred_x ? state->preferred_x : find.x;

			// can only go up if there's a previous row
			if (find.prev_first == find.first_char)
				break;

			// now find character position up a row
			state->cursor = find.prev_first;
			STB_TEXTEDIT_LAYOUTROW(&row, str, state->cursor);
			x = row.x0;
			for (i = 0; i < row.num_chars; ) {
				float dx = STB_TEXTEDIT_GETWIDTH(str, find.prev_first, i);
				int next = STB_TEXTEDIT_GETNEXTCHARINDEX(str, state->cursor);
#ifdef STB_TEXTEDIT_GETWIDTH_NEWLINE
				if (dx == STB_TEXTEDIT_GETWIDTH_NEWLINE)
					break;
#endif
				x += dx;
				if (x > goal_x)
					break;
				i += next - state->cursor;
				state->cursor = next;
			}
			stb_textedit_clamp(str, state);

			if (state->cursor == find.first_char)
				str->LastMoveDirectionLR = GuiDir_Right;
			else if (state->cursor == find.prev_first)
				str->LastMoveDirectionLR = GuiDir_Left;
			state->has_preferred_x = 1;
			state->preferred_x = goal_x;

			if (sel)
				state->select_end = state->cursor;

			// go to previous line
			// (we need to scan previous line the hard way. maybe we could expose this as a new API function?)
			prev_scan = find.prev_first > 0 ? find.prev_first - 1 : 0;
			while (prev_scan > 0)
			{
				int prev = STB_TEXTEDIT_GETPREVCHARINDEX(str, prev_scan);
				if (STB_TEXTEDIT_GETCHAR(str, prev) == STB_TEXTEDIT_NEWLINE)
					break;
				prev_scan = prev;
			}
			find.first_char = find.prev_first;
			find.prev_first = STB_TEXTEDIT_MOVELINESTART(str, state, prev_scan);
		}
		break;
	}

	case STB_TEXTEDIT_K_DELETE:
	case STB_TEXTEDIT_K_DELETE | STB_TEXTEDIT_K_SHIFT:
		if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_delete_selection(str, state);
		else {
			int n = STB_TEXTEDIT_STRINGLEN(str);
			if (state->cursor < n)
				stb_textedit_delete(str, state, state->cursor, STB_TEXTEDIT_GETNEXTCHARINDEX(str, state->cursor) - state->cursor);
		}
		state->has_preferred_x = 0;
		break;

	case STB_TEXTEDIT_K_BACKSPACE:
	case STB_TEXTEDIT_K_BACKSPACE | STB_TEXTEDIT_K_SHIFT:
		if (STB_TEXT_HAS_SELECTION(state))
			stb_textedit_delete_selection(str, state);
		else {
			stb_textedit_clamp(str, state);
			if (state->cursor > 0) {
				int prev = STB_TEXTEDIT_GETPREVCHARINDEX(str, state->cursor);
				stb_textedit_delete(str, state, prev, state->cursor - prev);
				state->cursor = prev;
			}
		}
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_K_TEXTSTART2
	case STB_TEXTEDIT_K_TEXTSTART2:
#endif
	case STB_TEXTEDIT_K_TEXTSTART:
		state->cursor = state->select_start = state->select_end = 0;
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_K_TEXTEND2
	case STB_TEXTEDIT_K_TEXTEND2:
#endif
	case STB_TEXTEDIT_K_TEXTEND:
		state->cursor = STB_TEXTEDIT_STRINGLEN(str);
		state->select_start = state->select_end = 0;
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_K_TEXTSTART2
	case STB_TEXTEDIT_K_TEXTSTART2 | STB_TEXTEDIT_K_SHIFT:
#endif
	case STB_TEXTEDIT_K_TEXTSTART | STB_TEXTEDIT_K_SHIFT:
		stb_textedit_prep_selection_at_cursor(state);
		state->cursor = state->select_end = 0;
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_K_TEXTEND2
	case STB_TEXTEDIT_K_TEXTEND2 | STB_TEXTEDIT_K_SHIFT:
#endif
	case STB_TEXTEDIT_K_TEXTEND | STB_TEXTEDIT_K_SHIFT:
		stb_textedit_prep_selection_at_cursor(state);
		state->cursor = state->select_end = STB_TEXTEDIT_STRINGLEN(str);
		state->has_preferred_x = 0;
		break;


#ifdef STB_TEXTEDIT_K_LINESTART2
	case STB_TEXTEDIT_K_LINESTART2:
#endif
	case STB_TEXTEDIT_K_LINESTART:
		stb_textedit_clamp(str, state);
		stb_textedit_move_to_first(state);
		state->cursor = STB_TEXTEDIT_MOVELINESTART(str, state, state->cursor);
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_K_LINEEND2
	case STB_TEXTEDIT_K_LINEEND2:
#endif
	case STB_TEXTEDIT_K_LINEEND: {
		stb_textedit_clamp(str, state);
		stb_textedit_move_to_last(str, state);
		state->cursor = STB_TEXTEDIT_MOVELINEEND(str, state, state->cursor);
		state->has_preferred_x = 0;
		break;
	}

#ifdef STB_TEXTEDIT_K_LINESTART2
	case STB_TEXTEDIT_K_LINESTART2 | STB_TEXTEDIT_K_SHIFT:
#endif
	case STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_SHIFT:
		stb_textedit_clamp(str, state);
		stb_textedit_prep_selection_at_cursor(state);
		state->cursor = STB_TEXTEDIT_MOVELINESTART(str, state, state->cursor);
		state->select_end = state->cursor;
		state->has_preferred_x = 0;
		break;

#ifdef STB_TEXTEDIT_K_LINEEND2
	case STB_TEXTEDIT_K_LINEEND2 | STB_TEXTEDIT_K_SHIFT:
#endif
	case STB_TEXTEDIT_K_LINEEND | STB_TEXTEDIT_K_SHIFT: {
		stb_textedit_clamp(str, state);
		stb_textedit_prep_selection_at_cursor(state);
		state->cursor = STB_TEXTEDIT_MOVELINEEND(str, state, state->cursor);
		state->select_end = state->cursor;
		state->has_preferred_x = 0;
		break;
	}
	}
}

/////////////////////////////////////////////////////////////////////////////
//
//      Undo processing
//
// @OPTIMIZE: the undo/redo buffer should be circular

static void stb_textedit_flush_redo(StbUndoState* state)
{
	state->redo_point = STB_TEXTEDIT_UNDOSTATECOUNT;
	state->redo_char_point = STB_TEXTEDIT_UNDOCHARCOUNT;
}

// discard the oldest entry in the undo list
static void stb_textedit_discard_undo(StbUndoState* state)
{
	if (state->undo_point > 0) {
		// if the 0th undo state has characters, clean those up
		if (state->undo_rec[0].char_storage >= 0) {
			int n = state->undo_rec[0].insert_length, i;
			// delete n characters from all other records
			state->undo_char_point -= n;
			STB_TEXTEDIT_memmove(state->undo_char, state->undo_char + n, (size_t)(state->undo_char_point * sizeof(STB_TEXTEDIT_CHARTYPE)));
			for (i = 0; i < state->undo_point; ++i)
				if (state->undo_rec[i].char_storage >= 0)
					state->undo_rec[i].char_storage -= n; // @OPTIMIZE: get rid of char_storage and infer it
		}
		--state->undo_point;
		STB_TEXTEDIT_memmove(state->undo_rec, state->undo_rec + 1, (size_t)(state->undo_point * sizeof(state->undo_rec[0])));
	}
}

// discard the oldest entry in the redo list--it's bad if this
// ever happens, but because undo & redo have to store the actual
// characters in different cases, the redo character buffer can
// fill up even though the undo buffer didn't
static void stb_textedit_discard_redo(StbUndoState* state)
{
	int k = STB_TEXTEDIT_UNDOSTATECOUNT - 1;

	if (state->redo_point <= k) {
		// if the k'th undo state has characters, clean those up
		if (state->undo_rec[k].char_storage >= 0) {
			int n = state->undo_rec[k].insert_length, i;
			// move the remaining redo character data to the end of the buffer
			state->redo_char_point += n;
			STB_TEXTEDIT_memmove(state->undo_char + state->redo_char_point, state->undo_char + state->redo_char_point - n, (size_t)((STB_TEXTEDIT_UNDOCHARCOUNT - state->redo_char_point) * sizeof(STB_TEXTEDIT_CHARTYPE)));
			// adjust the position of all the other records to account for above memmove
			for (i = state->redo_point; i < k; ++i)
				if (state->undo_rec[i].char_storage >= 0)
					state->undo_rec[i].char_storage += n;
		}
		// now move all the redo records towards the end of the buffer; the first one is at 'redo_point'
		// [DEAR IMGUI]
		size_t move_size = (size_t)((STB_TEXTEDIT_UNDOSTATECOUNT - state->redo_point - 1) * sizeof(state->undo_rec[0]));
		const char* buf_begin = (char*)state->undo_rec; (void)buf_begin;
		const char* buf_end = (char*)state->undo_rec + sizeof(state->undo_rec); (void)buf_end;
		assert(((char*)(state->undo_rec + state->redo_point)) >= buf_begin);
		assert(((char*)(state->undo_rec + state->redo_point + 1) + move_size) <= buf_end);
		STB_TEXTEDIT_memmove(state->undo_rec + state->redo_point + 1, state->undo_rec + state->redo_point, move_size);

		// now move redo_point to point to the new one
		++state->redo_point;
	}
}

static StbUndoRecord* stb_text_create_undo_record(StbUndoState* state, int numchars)
{
	// any time we create a new undo record, we discard redo
	stb_textedit_flush_redo(state);

	// if we have no free records, we have to make room, by sliding the
	// existing records down
	if (state->undo_point == STB_TEXTEDIT_UNDOSTATECOUNT)
		stb_textedit_discard_undo(state);

	// if the characters to store won't possibly fit in the buffer, we can't undo
	if (numchars > STB_TEXTEDIT_UNDOCHARCOUNT) {
		state->undo_point = 0;
		state->undo_char_point = 0;
		return NULL;
	}

	// if we don't have enough free characters in the buffer, we have to make room
	while (state->undo_char_point + numchars > STB_TEXTEDIT_UNDOCHARCOUNT)
		stb_textedit_discard_undo(state);

	return &state->undo_rec[state->undo_point++];
}

static STB_TEXTEDIT_CHARTYPE* stb_text_createundo(StbUndoState* state, int pos, int insert_len, int delete_len)
{
	StbUndoRecord* r = stb_text_create_undo_record(state, insert_len);
	if (r == NULL)
		return NULL;

	r->where = pos;
	r->insert_length = (STB_TEXTEDIT_POSITIONTYPE)insert_len;
	r->delete_length = (STB_TEXTEDIT_POSITIONTYPE)delete_len;

	if (insert_len == 0) {
		r->char_storage = -1;
		return NULL;
	}
	else {
		r->char_storage = state->undo_char_point;
		state->undo_char_point += insert_len;
		return &state->undo_char[r->char_storage];
	}
}

static void stb_text_undo(STB_TEXTEDIT_STRING* str, STB_TexteditState* state)
{
	StbUndoState* s = &state->undostate;
	StbUndoRecord u, * r;
	if (s->undo_point == 0)
		return;

	// we need to do two things: apply the undo record, and create a redo record
	u = s->undo_rec[s->undo_point - 1];
	r = &s->undo_rec[s->redo_point - 1];
	r->char_storage = -1;

	r->insert_length = u.delete_length;
	r->delete_length = u.insert_length;
	r->where = u.where;

	if (u.delete_length) {
		// if the undo record says to delete characters, then the redo record will
		// need to re-insert the characters that get deleted, so we need to store
		// them.

		// there are three cases:
		//    there's enough room to store the characters
		//    characters stored for *redoing* don't leave room for redo
		//    characters stored for *undoing* don't leave room for redo
		// if the last is true, we have to bail

		if (s->undo_char_point + u.delete_length >= STB_TEXTEDIT_UNDOCHARCOUNT) {
			// the undo records take up too much character space; there's no space to store the redo characters
			r->insert_length = 0;
		}
		else {
			int i;

			// there's definitely room to store the characters eventually
			while (s->undo_char_point + u.delete_length > s->redo_char_point) {
				// should never happen:
				if (s->redo_point == STB_TEXTEDIT_UNDOSTATECOUNT)
					return;
				// there's currently not enough room, so discard a redo record
				stb_textedit_discard_redo(s);
			}
			r = &s->undo_rec[s->redo_point - 1];

			r->char_storage = s->redo_char_point - u.delete_length;
			s->redo_char_point = s->redo_char_point - u.delete_length;

			// now save the characters
			for (i = 0; i < u.delete_length; ++i)
				s->undo_char[r->char_storage + i] = STB_TEXTEDIT_GETCHAR(str, u.where + i);
		}

		// now we can carry out the deletion
		STB_TEXTEDIT_DELETECHARS(str, u.where, u.delete_length);
	}

	// check type of recorded action:
	if (u.insert_length) {
		// easy case: was a deletion, so we need to insert n characters
		u.insert_length = STB_TEXTEDIT_INSERTCHARS(str, u.where, &s->undo_char[u.char_storage], u.insert_length);
		s->undo_char_point -= u.insert_length;
	}

	state->cursor = u.where + u.insert_length;

	s->undo_point--;
	s->redo_point--;
}

static void stb_text_redo(STB_TEXTEDIT_STRING* str, STB_TexteditState* state)
{
	StbUndoState* s = &state->undostate;
	StbUndoRecord* u, r;
	if (s->redo_point == STB_TEXTEDIT_UNDOSTATECOUNT)
		return;

	// we need to do two things: apply the redo record, and create an undo record
	u = &s->undo_rec[s->undo_point];
	r = s->undo_rec[s->redo_point];

	// we KNOW there must be room for the undo record, because the redo record
	// was derived from an undo record

	u->delete_length = r.insert_length;
	u->insert_length = r.delete_length;
	u->where = r.where;
	u->char_storage = -1;

	if (r.delete_length) {
		// the redo record requires us to delete characters, so the undo record
		// needs to store the characters

		if (s->undo_char_point + u->insert_length > s->redo_char_point) {
			u->insert_length = 0;
			u->delete_length = 0;
		}
		else {
			int i;
			u->char_storage = s->undo_char_point;
			s->undo_char_point = s->undo_char_point + u->insert_length;

			// now save the characters
			for (i = 0; i < u->insert_length; ++i)
				s->undo_char[u->char_storage + i] = STB_TEXTEDIT_GETCHAR(str, u->where + i);
		}

		STB_TEXTEDIT_DELETECHARS(str, r.where, r.delete_length);
	}

	if (r.insert_length) {
		// easy case: need to insert n characters
		r.insert_length = STB_TEXTEDIT_INSERTCHARS(str, r.where, &s->undo_char[r.char_storage], r.insert_length);
		s->redo_char_point += r.insert_length;
	}

	state->cursor = r.where + r.insert_length;

	s->undo_point++;
	s->redo_point++;
}

static void stb_text_makeundo_insert(STB_TexteditState* state, int where, int length)
{
	stb_text_createundo(&state->undostate, where, 0, length);
}

static void stb_text_makeundo_delete(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int length)
{
	int i;
	STB_TEXTEDIT_CHARTYPE* p = stb_text_createundo(&state->undostate, where, length, 0);
	if (p) {
		for (i = 0; i < length; ++i)
			p[i] = STB_TEXTEDIT_GETCHAR(str, where + i);
	}
}

static void stb_text_makeundo_replace(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int old_length, int new_length)
{
	int i;
	STB_TEXTEDIT_CHARTYPE* p = stb_text_createundo(&state->undostate, where, old_length, new_length);
	if (p) {
		for (i = 0; i < old_length; ++i)
			p[i] = STB_TEXTEDIT_GETCHAR(str, where + i);
	}
}

// reset the state to default
static void stb_textedit_clear_state(STB_TexteditState* state, int is_single_line)
{
	state->undostate.undo_point = 0;
	state->undostate.undo_char_point = 0;
	state->undostate.redo_point = STB_TEXTEDIT_UNDOSTATECOUNT;
	state->undostate.redo_char_point = STB_TEXTEDIT_UNDOCHARCOUNT;
	state->select_end = state->select_start = 0;
	state->cursor = 0;
	state->has_preferred_x = 0;
	state->preferred_x = 0;
	state->cursor_at_end_of_line = 0;
	state->initialized = 1;
	state->single_line = (bool)is_single_line;
	state->insert_mode = 0;
	state->row_count_per_page = 0;
}

// API initialize
static void stb_textedit_initialize_state(STB_TexteditState* state, int is_single_line)
{
	stb_textedit_clear_state(state, is_single_line);
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif

static int stb_textedit_paste(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, STB_TEXTEDIT_CHARTYPE const* ctext, int len)
{
	return stb_textedit_paste_internal(str, state, (STB_TEXTEDIT_CHARTYPE*)ctext, len);
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif





// traverse the layout to locate the nearest character to a display position
int stb_text_locate_coord(STB_TEXTEDIT_STRING* str, float x, float y, int* out_side_on_line);
// API click: on mouse down, move the cursor to the clicked location, and reset the selection
void stb_textedit_click(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, float x, float y);
// API drag: on mouse drag, move the cursor and selection endpoint to the clicked location
void stb_textedit_drag(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, float x, float y);
// forward declarations
void stb_text_undo(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
void stb_text_redo(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
void stb_text_makeundo_delete(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int length);
void stb_text_makeundo_insert(STB_TexteditState* state, int where, int length);
void stb_text_makeundo_replace(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int old_length, int new_length);

void stb_textedit_find_charpos(StbFindState* find, STB_TEXTEDIT_STRING* str, int n, int single_line);
// make the selection/cursor state valid if client altered the string
void stb_textedit_clamp(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
// delete characters while updating undo
void stb_textedit_delete(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int where, int len);
// delete the section
void stb_textedit_delete_selection(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
// canoncialize the selection so start <= end
void stb_textedit_sortselection(STB_TexteditState* state);
// move cursor to first character of selection
void stb_textedit_move_to_first(STB_TexteditState* state);
// move cursor to last character of selection
void stb_textedit_move_to_last(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
#ifndef STB_TEXTEDIT_MOVELINESTART
int stb_textedit_move_line_start(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int cursor);
#endif
#ifndef STB_TEXTEDIT_MOVELINEEND
int stb_textedit_move_line_end(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, int cursor);
#endif
int is_word_boundary(STB_TEXTEDIT_STRING* str, int idx);
#ifndef STB_TEXTEDIT_MOVEWORDLEFT
int stb_textedit_move_to_word_previous(STB_TEXTEDIT_STRING* str, int c);
#endif
#ifndef STB_TEXTEDIT_MOVEWORDRIGHT
int stb_textedit_move_to_word_next(STB_TEXTEDIT_STRING* str, int c);
#endif
// update selection and cursor to match each other
void stb_textedit_prep_selection_at_cursor(STB_TexteditState* state);
// API cut: delete selection
int stb_textedit_cut(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
// API paste: replace existing selection with passed-in text
int stb_textedit_paste_internal(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, STB_TEXTEDIT_CHARTYPE* text, int len);
// API key: process text input
void stb_textedit_text(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, const STB_TEXTEDIT_CHARTYPE* text, int text_len);
// API key: process a keyboard input
void stb_textedit_key(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, STB_TEXTEDIT_KEYTYPE key);
void stb_textedit_flush_redo(StbUndoState* state);
void stb_textedit_discard_undo(StbUndoState* state);
void stb_textedit_discard_redo(StbUndoState* state);
StbUndoRecord* stb_text_create_undo_record(StbUndoState* state, int numchars);
STB_TEXTEDIT_CHARTYPE* stb_text_createundo(StbUndoState* state, int pos, int insert_len, int delete_len);
// reset the state to default
void stb_textedit_clear_state(STB_TexteditState* state, int is_single_line);
// API initialize
void stb_textedit_initialize_state(STB_TexteditState* state, int is_single_line);
int stb_textedit_paste(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, STB_TEXTEDIT_CHARTYPE const* ctext, int len);



void testedit() {

	stb_textedit_click(0, 0, 0, 0);
	stb_textedit_drag(0, 0, 0, 0);
	stb_textedit_cut(0, 0);
	stb_textedit_key(0, 0, 0);
	stb_textedit_initialize_state(0, 0);
	stb_textedit_paste(0, 0, 0, 0);
}

#endif // 1

edit_cx::edit_cx() :widget_t(WIDGET_TYPE::WT_EDIT)
{
	ctx = new text_control();
	set_single(true);
	on_event_cb = [=](uint32_t type, et_un_t* e, const glm::vec2& pos)
		{
			on_event_e(type, e);
		};
}

edit_cx::~edit_cx()
{
	if (ctx)delete ctx; ctx = nullptr;
}

void edit_cx::set_single(bool is)
{
	stb_textedit_initialize_state(&ctx->state, is);
}

void edit_cx::set_pwd(char ch)
{
	pwdch = ch;
	//if (ch)
	//	md::unicode_to_utf8(pwdch, ch);
}

void edit_cx::set_text(const void* str, int len)
{
	stb_textedit_delete(ctx, &ctx->state, 0, ctx->str.size());
	stb_textedit_text(ctx, &ctx->state, (char*)str, len);
	if (pwdch) {
		stext.resize(ctx->str.size());
		for (auto& it : stext) {
			it = pwdch;
		}
	}
	up_text = true;
	up_cursor(true);
}

void edit_cx::add_text(const void* str, int len)
{
	stb_textedit_text(ctx, &ctx->state, (char*)str, len);
	if (pwdch) {
		stext.resize(ctx->str.size());
		for (auto& it : stext) {
			it = pwdch;
		}
	}
	up_text = true;
	up_cursor(true);
}

void edit_cx::set_size(const glm::ivec2& ss)
{
	widget_t::set_size(ss);
}

void edit_cx::set_pos(const glm::ivec2& pos)
{
	widget_t::set_pos(pos);
}

void edit_cx::set_align_pos(const glm::vec2& pos)
{}

void edit_cx::set_align(const glm::vec2& a)
{}

void edit_cx::set_cursor(const glm::ivec3& c)
{
	_cursor = c;
}

void edit_cx::set_color(const glm::ivec4& c)
{
	_color = c;
}

void edit_cx::set_family(font_family_t* family, int fontsize)
{
	widget_t::set_family(family, fontsize);
	ctx->family = family;
	ctx->font_size = fontsize;
}

void edit_cx::set_show_input_cursor(bool ab)
{
	show_input_cursor = ab;
}

void edit_cx::set_autobr(bool ab)
{}

void edit_cx::set_round_path(float v)
{}

void edit_cx::remove_char(size_t idx, int count)
{}

bool edit_cx::remove_bounds()
{
	return false;
}


void edit_cx::on_event_e(uint32_t type, et_un_t* ep)
{
	auto e = ep->v;
	auto t = (devent_type_e)type;
	switch (t) {
	case devent_type_e::text_input_e:
	{
		auto p = e.t;
		auto ipos = input_pos();// 计算输入法坐标
		p->x = ipos.x;
		p->y = ipos.y + 3;
		p->w = ipos.z;
		p->h = ipos.w;
		add_text(p->text, strlen(p->text));
		wstr = md::u8_w(ctx->str.c_str(), ctx->str.size());
	}break;
	case devent_type_e::text_editing_e: {

		if (!is_input)break;
		auto& p = e.e;
		bool setimepos = false;
		if (ctx->caret_old != ctx->state.cursor)
		{
			ctx->caret_old = ctx->state.cursor;
			setimepos = true;
		}
		std::string str;
		if (p->text) {
			str = p->text;
		}
		if (str.size())
		{
			setimepos = true;
		}
		if (setimepos)
		{
			auto ipos = input_pos();// 计算输入法坐标
			p->x = ipos.x;
			p->y = ipos.y;
			p->w = ipos.z;
			p->h = ipos.w;
			//printf("text_editing_e:%s\t%d\n", str.c_str(), ipos.x);
		}
		//if (str.size())
		editingstr = str;
	}break;
	case devent_type_e::keyboard_e:
	{
		on_keyboard(ep);
	}break;
	default:
		break;
	};

}

input_state_t* get_input_state_cx(void* ptr, int t)
{
	static input_state_t r = {};
	if (t)
	{
		if (r.ptr)
		{
			auto p = (edit_cx*)r.ptr;
			p->editingstr.clear();
			p->is_input = false;
			//p->dindex--;
			//if (p->parent)
			//	p->parent->valid = true;
		}
		r.ptr = ptr;
	}
	if (ptr && r.ptr)
	{
		auto p = (edit_cx*)r.ptr;
		if (p) {
			//p->dindex++;
			//if (p->parent)
			//	p->parent->valid = true;
			*((glm::ivec4*)&r.x) = p->input_pos();
			r.y += 3;
		}
		if (!r.cb)
			r.cb = [](uint32_t type, et_un_t* e, void* ud) {
			if (ud) {
				((edit_cx*)ud)->on_event_e(type, e);
			}
			};
	}
	return &r;
}
bool edit_cx::on_mevent(int type, const glm::vec2& mps, void* e)
{
	auto t = (event_type2)type;
	bool ret = false;
	auto p = (mouse_move_et*)e;

	auto mpos = mps;
	glm::vec2 tpos = ctx->_align_pos - ctx->scroll_pos;
	tpos += thickness;

	switch (t)
	{
	case event_type2::on_move:
	{
		p->cursor = (int)cursor_st::cursor_ibeam;//设置输入光标
	}break;
	case event_type2::on_leave:
	{
		p->cursor = (int)cursor_st::cursor_arrow;//设置光标
	}break;
	case event_type2::on_down:
	{
		mpos -= tpos + _pos;
		_cmpos = mpos;
		if (form)
		{
			form_set_input_ptr(form, get_input_state_cx(this, 1));
			ctx->c_d = -1; is_input = true;
		}
		else {
			ctx->c_d = 0; is_input = false;
		}
		stb_textedit_click(ctx, &ctx->state, mpos.x, mpos.y);
		ret = true;
	}break;
	case event_type2::on_drag:
	{
		mpos += curpos; mpos -= tpos + _pos;
		glm::ivec2 cm = mpos;
		if (_cmpos != cm)
			_cmpos = mpos;
		stb_textedit_drag(ctx, &ctx->state, mpos.x, mpos.y);
		ret = true;
	}break;
	case event_type2::on_scroll:
	{
		ctx->scroll_pos.y -= mps.y * ctx->lineheight;
		int lc = ctx->lvs.size();
		lc--;
		if (lc >= 0)
			ctx->scroll_pos.y = glm::clamp(ctx->scroll_pos.y, 0, lc * ctx->lineheight);
	}break;
	default:
		break;
	};
	if (ret) {
		up_cursor(true);
	}
	return ret;
}

void edit_cx::on_keyboard(et_un_t* ep)
{
	auto p = ep->v.k;
	const bool is_osx = false;
	// Control=1 Shift=2 Alt=4 8Cmd/Super/Windows
	bool KeyShift = p->kmod == 2;
	bool KeyAlt = p->kmod == 4;
	bool KeyCtrl = p->kmod == 1;
	bool KeySuper = p->kmod == 8;
	const int k_mask = (KeyShift ? STB_TEXTEDIT_K_SHIFT : 0);
	const bool is_wordmove_key_down = is_osx ? KeyAlt : KeyCtrl;                     // OS X style: Text editing cursor movement using Alt instead of Ctrl
	const bool is_startend_key_down = is_osx && KeyCtrl && !KeySuper && !KeyAlt;  // OS X style: Line/Text Start and End using Cmd+Arrows instead of Home/End

	int key = 0;


	if (!p->down)
	{
		do {
			if (!KeyCtrl)break;
			switch (p->keycode) {
			case SDLK_A:
			{
				ctx->state.select_end = ctx->str.size();
				ctx->state.select_start = 0;
			}
			break;
			case SDLK_X:
			case SDLK_C:
			{
				std::string str;
				stb_textedit_sortselection(&ctx->state);
				str.assign(ctx->str.c_str() + ctx->state.select_start, ctx->state.select_end - ctx->state.select_start);
				set_clipboard(str.c_str());
				if (p->keycode == SDLK_X)
				{
					stb_textedit_cut(ctx, &ctx->state);
					up_text = true;
				}
			}
			break;
			case SDLK_V:
			{
				auto str = get_clipboard();
				stb_textedit_paste(ctx, &ctx->state, str.c_str(), str.size());
				up_text = true;
			}
			break;
			case SDLK_Y:
				key = STB_TEXTEDIT_K_REDO;
				up_text = true;
				//is_redo = true;	//_storage_buf->redo();
				break;
			case SDLK_Z:
				key = STB_TEXTEDIT_K_UNDO;
				up_text = true;
				//is_undo = true;	//_storage_buf->undo();
				break;
			}
		} while (0);
	}
	if (!key && (!p->down || editingstr.size()))
	{
		return;
	}

	bool isupcursor = false;
	switch (p->keycode)
	{
	case SDLK_PRINTSCREEN:
	{}
	break;
	case SDLK_SCROLLLOCK:
	{}
	break;
	case SDLK_PAUSE:
	{}
	break;
	case SDLK_TAB:
	{
		add_text("\t", 1);
	}
	break;
	case SDLK_BACKSPACE:
	{
		key = STB_TEXTEDIT_K_BACKSPACE;
		up_text = true;
	}
	break;
	case SDLK_INSERT:
	{
		key = STB_TEXTEDIT_K_INSERT;
		up_text = true;
	}
	break;
	case SDLK_PAGEDOWN:
	{
		key = STB_TEXTEDIT_K_PGDOWN;
	}
	break;
	case SDLK_PAGEUP:
	{
		key = STB_TEXTEDIT_K_PGUP;
	}
	break;
	case SDLK_DELETE:
	{
		key = STB_TEXTEDIT_K_DELETE;
	}
	break;
	case SDLK_HOME:
	{
		key = STB_TEXTEDIT_K_TEXTSTART;
	}
	break;
	case SDLK_END:
	{
		key = STB_TEXTEDIT_K_TEXTEND;
	}
	break;
	case SDLK_RIGHT:
	{
		key = STB_TEXTEDIT_K_RIGHT;
	}
	break;
	case SDLK_LEFT:
	{
		key = STB_TEXTEDIT_K_LEFT;
	}
	break;
	case SDLK_DOWN:
	{
		key = STB_TEXTEDIT_K_DOWN;
	}
	break;
	case SDLK_UP:
	{
		key = STB_TEXTEDIT_K_UP;
	}
	break;
	case SDLK_RETURN:
	{
		remove_bounds();
		if (!ctx->state.single_line)
		{
			add_text("\n", 1);
		}
	}
	break;
	default:
		break;
	}
	if (key)
	{
		/*STB_TEXTEDIT_K_SHIFT
		 STB_TEXTEDIT_K_CONTROL
		 STB_TEXTEDIT_K_LEFT
		 STB_TEXTEDIT_K_RIGHT
		 STB_TEXTEDIT_K_UP
		 STB_TEXTEDIT_K_DOWN
		 STB_TEXTEDIT_K_LINESTART
		 STB_TEXTEDIT_K_LINEEND
		 STB_TEXTEDIT_K_TEXTSTART
		 STB_TEXTEDIT_K_TEXTEND
		 STB_TEXTEDIT_K_DELETE
		 STB_TEXTEDIT_K_BACKSPACE
		 STB_TEXTEDIT_K_UNDO
		 STB_TEXTEDIT_K_REDO
		 STB_TEXTEDIT_K_INSERT
		 STB_TEXTEDIT_K_WORDLEFT
		  STB_TEXTEDIT_K_WORDRIGHT
		  STB_TEXTEDIT_K_PGUP
		  STB_TEXTEDIT_K_PGDOWN */
		if (KeyCtrl)
		{
			key |= STB_TEXTEDIT_K_CONTROL;
		}
		if (KeyShift)
		{
			key |= STB_TEXTEDIT_K_SHIFT;
		}
		stb_textedit_key(ctx, &ctx->state, key);
		up_cursor(true);
	}
}

bool edit_cx::update(float delta)
{
	int ret = 0;
	if (is_input) {
		ctx->c_ct += delta * 1000.0 * ctx->c_d;
		if (ctx->c_ct > _cursor.z)
		{
			ctx->c_d = -1;
			ctx->c_ct = _cursor.z;
			valid = true;
		}
		if (ctx->c_ct < 0)
		{
			ctx->c_d = 1; ctx->c_ct = 0;
			valid = true;
		}
	}
	if (ctx->lineheight < 1)
		ctx->lineheight = font_get_lineheight(style.family, style.fontsize, true);

	if (up_text)
	{
		if (ctx->state.single_line)
		{
			glm::vec2 ext = { 0,ctx->lineheight }, ext1 = { 0,style.fontsize }, ss = _size;
			ss -= thickness * 2;
			auto ps = glm::ceil((ss - ext) * style.align);
			auto ps1 = glm::ceil((ss - ext1) * style.align);
			ctx->_align_pos.y = ps.y;
			ctx->_align_pos1.y = ps1.y;
		}
		up_text = false;
		ctx->widths.clear();
		valid = true;
	}
	if (valid) {
		ret++; valid = false;
	}
	return ret > 0;
}

void edit_cx::draw(rvg_cx* rv)
{
	glm::ivec2 nposs = _pos;
	glm::ivec2 ss = _size;
	auto psv = get_spos(); psv += _pos; //auto psv = get_ppos();
	auto vsize = get_size();
	vsize += thickness * 2;
	auto view = glm::ivec4(psv, vsize);
	rv->push_view(view, this);
	//rv->save();
	rv->translate(psv);
	rv->add_rect({ 0,0,ss.x ,ss.y }, rounding);
	rv->set_color(_color.x);
	rv->fill();
	psv += thickness;
	rv->translate({ thickness ,thickness });
	auto tpos = ctx->_align_pos - ctx->scroll_pos;
	auto tpos1 = ctx->_align_pos1 - ctx->scroll_pos;

	glm::vec2 ext = { 0,ctx->lineheight }, ext1 = { 0,style.fontsize }, iss = _size;
	iss -= thickness * 2;
	auto ps = glm::ceil((iss - ext) * style.align);
	auto ps1 = glm::ceil((iss - ext1) * style.align);
	glm::ivec2 npos = {};
	glm::ivec2 srcpos = {};
	glm::ivec2 cpos = ctx->cursor_pos;
	if (ctx->state.single_line)
	{
		npos.y += ps1.y;
		srcpos.y += ps.y;
	}
	{
		//rv->save();
		// 裁剪区域渲染选中效果背景
		rv->add_rect({ 0,0,ss.x - thickness * 2,ss.y - thickness * 2 }, 0);
		rv->clip();
		auto oclip = rv->get_clip();
		tpos1 = tpos;
		tpos1.y += ceil((ctx->lineheight - style.fontsize) * style.align.y);
		auto v = get_bounds();
		rv->translate({ 0,1 });
		if (v.x != v.y && ctx->rangerc.size()) {
			//rv->save();
			rv->translate(tpos);
			rv->set_color(_color.z);
			if (roundselect)
			{
				if (ctx->range_path.size() && ctx->range_path[0].size() > 3) {
					rv->add_polyline(&ctx->range_path, true);
					rv->fill();
				}
			}
			else {
				for (auto& it : ctx->rangerc)
				{
					rv->add_rect(it, std::min(it.z, it.w) * 0.18);
					rv->fill();
				}
			}
			rv->translate(-tpos);
			//rv->restore();
		}

		// 渲染文本
		glm::vec4 rc = { 0,0,  ss };
		text_style st = style;
		st.align = {};
		text_st tx = {};
		tx.pos = { tpos1 };
		tx.size = ss;
		auto ptxt = placeholder.c_str();
		size_t plen = placeholder.size();
		do {
			if (pwdch)
			{
				if (stext.size() > 0)
				{
					ptxt = stext.c_str();
					plen = stext.size(); break;
				}
			}
			else {
				if (ctx->str.size() > 0)
				{
					ptxt = ctx->str.c_str();
					plen = ctx->str.size(); break;
				}
			}
			st.color = set_alpha_x(style.color, 200);
		} while (0);
		tx.text = ptxt;
		tx.text_len = plen;
		if (*ptxt && plen > 0)
			rv->add_text(&tx, &st);


		cpos += tpos + psv;
		bool ccd = (show_input_cursor && ctx->c_d == 1 && _cursor.x > 0 && ctx->cursor_pos.z > 0);
		if (ccd && is_input)
		{
			auto rpos = cpos;
			rpos -= rv->get_translate();
			//rv->push_view({ rpos ,align_up(_cursor.x,4),align_up(ctx->cursor_pos.z,4) });
			rv->set_color(_cursor.y);
			rv->add_rect(glm::ivec4(rpos, _cursor.x, ctx->cursor_pos.z), 0);
			rv->fill();
			//rv->pop_view();
		}
		//rv->restore();
		rv->clip(oclip);// 恢复裁剪区域
	}
	//rv->restore();
	rv->pop_view();

	// 编辑中的文本
	if (is_input)
	{
		auto epos = glm::ivec2(cpos);
		uint32_t editing_color = _color.w;
		if (editingstr.size())
		{
			// 渲染文本 
			text_style st = style;
			st.align = {};
			st.color = editing_color;
			glm::ivec2 lps = get_text_rect(style.family, style.fontsize, editingstr.c_str(), editingstr.size(), 0);

			if (lps.y < ctx->lineheight)
				lps.y = ctx->lineheight;
			glm::vec4 lss = { 0,  lps.y + 0.5, lps.x, lps.y + 0.5 };
			glm::vec4 rc = { 1,1,lps.x + 2, lps.y + 2 };
			auto clip0 = glm::ivec4(epos, rc.z, rc.w);

			rv->push_view(clip0);
			rv->translate({ clip0.x, clip0.y });
			rv->set_color(get_reverse_color(editing_color));
			rv->add_rect({ 0, 0, lps.x + 2, lps.y + 2 }, 0);
			rv->fill();
			rv->set_color(editing_color);
			rv->add_line({ lss.x + 1, lss.y }, { lss.z, lss.w });
			rv->set_line_width(1);
			rv->stroke();
			text_st tx = {};
			tx.pos = { rc.x,rc.y };
			tx.size = glm::ivec2(rc.z, rc.w);
			tx.text = editingstr.c_str(); tx.text_len = editingstr.size();
			rv->add_text(&tx, &st);
			rv->pop_view();
		}
	}
}

glm::ivec4 edit_cx::input_pos()
{
	glm::ivec2 cpos = ctx->cursor_pos;
	auto pos = get_pos();
	return { pos + cpos - ctx->scroll_pos + ctx->_align_pos
		,2,  ctx->cursor_pos.z + 2 };
}

int edit_cx::get_cursor_idx()
{
	return ctx->state.cursor;
}

std::string edit_cx::get_select_str()
{
	return std::string();
}

std::wstring edit_cx::get_select_wstr()
{
	return std::wstring();
}

glm::ivec2 edit_cx::get_bounds()
{
	glm::ivec2 v = { ctx->state.select_start, ctx->state.select_end };
	if (v.x > v.y) { std::swap(v.x, v.y); }
	return v;
}
glm::ivec3 edit_cx::get_line_length(int index)
{
	int x_pos = 0;
	int lidx = 0;
	int cu = -1;
	int cx = 0;
	auto sp = ctx->str.c_str();
	for (size_t i = 0; i < ctx->lvs.size(); i++)
	{
		auto c = ctx->lvs[i];
		if (index >= c.x && index < c.x + c.y + 1)//因为有换行+1
		{
			x_pos = index - c.x;
			lidx = c.x;
			cu = i;// 行号
			break;
		}
	}
	if (cu < 0 && ctx->lvs.size()) {
		cu = ctx->lvs.size() - 1;
		auto c = ctx->lvs[cu];
		x_pos = index - c.x;
		lidx = c.x;
	}
	glm::ivec3 ret = { lidx,cu, x_pos };
	return ret;
}
// 获取范围的像素大小
std::vector<glm::ivec4> edit_cx::get_bounds_px()
{
	bool font_hfirst = true;//取第一字体行高
	float pwidth = style.fontsize * 0.5;// 补行尾宽度
	auto pstr = ctx->str.c_str();
	auto tsize = ctx->str.size();
	if (up_text || ctx->widths.empty())
	{
		up_text = false;
		ctx->widths.clear();
		ctx->lvs.clear();
		auto length = pwdch ? stext.size() : ctx->str.size();
		auto p = pwdch ? stext.c_str() : ctx->str.c_str();
		size_t f = 0, s = 0; size_t i = 0;
		for (; i < length; i++)
		{
			if (p[i] == '\n')
			{
				ctx->lvs.push_back({ f,i - f });
				f = i + 1;
			}
		}
		ctx->lvs.push_back({ f,i - f });
		font_get_text_posv(style.family, style.fontsize, pstr, tsize, ctx->widths);
		pwidth = font_get_text_rect1(style.family, style.fontsize, "1").x;
	}
	std::vector<glm::ivec4> r;
	std::vector<glm::ivec4> rs, rss;
	auto v = get_bounds();
	if (v.x == v.y) { ctx->rangerc = rss; return rs; }
	if ((v.x >= tsize || v.y > tsize)) {
		ctx->rangerc = rss; return rs;
	}
	auto v1 = get_line_length(v.x);
	auto v2 = get_line_length(v.y);
	auto line_no = ctx->lvs.size();
	auto h = fix_line_height > 0 ? fix_line_height : font_get_lineheight(style.family, style.fontsize, font_hfirst);
	// 计算选中范围的每行的坐标宽高
	if (v1 == v2) {}
	else {
		if (v1.y == v2.y)
		{
			auto ks = ctx->lvs[v1.y];
			auto w1 = ctx->widths[v1.y];
			auto xc = char2pos(v.x - ks.x, pstr + ks.x);
			auto yc = char2pos(v.y - ks.x, pstr + ks.x);
			int w = w1[xc];
			int ww = w1[yc] - w;
			rss.push_back({ w ,v1.y * h,ww,h });// 同一行时
		}
		else {
			auto ks = ctx->lvs[v1.y];
			auto w1 = ctx->widths[v1.y];
			auto w = w1[char2pos(v.x - ks.x, pstr + ks.x)];
			auto wd = *w1.rbegin() - w;
			rss.push_back({ w,v1.y * h,wd + pwidth,h });// 第一行
		}
		for (int i = v1.y + 1; i < line_no && i < v2.y; i++)
		{
			auto ks = ctx->lvs[i];
			auto w1 = ctx->widths[i];
			rss.push_back({ 0,i * h,*w1.rbegin() + pwidth, h });// 中间全行
		}
		if (v1.y < v2.y)
		{
			auto ks = ctx->lvs[v2.y];
			auto w1 = ctx->widths[v2.y];
			rss.push_back({ 0,v2.y * h,w1[char2pos(v.y - ks.x ,pstr + ks.x)],h });//最后一行
		}
	}
	if (roundselect)
	{
		PathsD subjects;
		PathD a;
		int py = ctx->_r_posy;
		for (size_t i = 0; i < rss.size(); i++)
		{
			auto& it = rss[i];
			if (it.z < 1)
				it.z = pwidth;
			a.push_back({ it.x,it.y + py });
			a.push_back({ it.x + it.z,it.y + py });
			a.push_back({ it.x + it.z,it.y + it.w + py });
			a.push_back({ it.x,it.y + it.w + py });
			subjects.push_back(a);
			a.clear();
		}
		subjects = Union(subjects, FillRule::NonZero, 6);
		//range_path = InflatePaths(subjects, 0.5, JoinType::Round, EndType::Polygon);
		auto sn = subjects.size();
		if (sn > 0)
		{
			path_v& ptr = ctx->ptr_path;
			ptr._data.clear();
			for (size_t i = 0; i < sn; i++)
			{
				auto& it = subjects[i];
				ptr.add_lines((glm::dvec2*)it.data(), it.size(), false);
			}
			ctx->range_path = gp::path_round(&ptr, -1, style.fontsize * ctx->round_path, 16, 0, 0);
		}
		else { ctx->range_path.clear(); }
	}
	ctx->rangerc = rss;
	return r;
}

glm::ivec2 edit_cx::get_pixel_size(const char* str, int len)
{
	int w = 0, h = 0;
	if (str && *str)
	{
		auto rc = get_text_rect(style.family, style.fontsize, str, len, 0);
		w = rc.x; h = rc.y;
	}
	return glm::ivec2(w, h);
}
size_t edit_cx::get_xy_to_index(int x, int y, const char* str)
{
	auto pstr = ctx->str.c_str();
	if (ctx->widths.empty())
	{
		font_get_text_posv(style.family, style.fontsize, pstr, ctx->str.size(), ctx->widths);
	}
	if (ctx->widths.size() != ctx->lvs.size())
		return -1;
	x += ctx->scroll_pos.x - ctx->_align_pos.x;
	y += ctx->scroll_pos.y - ctx->_align_pos.y;
	int index = 0, trailing = 0;
	if (x < 0)
	{
		x = 0;
	}
	if (y < 0)
	{
		y = 0;
	}
	glm::ivec2 lps = get_pixel_size(pstr, text.size());
	if (y > lps.y)
		y = lps.y - 1;
	y /= ctx->lineheight;
	if (y >= ctx->lvs.size())y = ctx->lvs.size() - 1;
	auto ky = ctx->lvs[y];
	auto& ws = ctx->widths[y];
	int cw = 0;
	for (size_t i = 0; i < ws.size(); i++)
	{
		if (x < ws[i]) {
			index = i;  break;
		}
	}
	if (x > *(ws.rbegin())) {
		index = ws.size() - 1;
	}
	else if (index > 0) {
		int tr = (ws[index] - ws[index - 1]) * 0.5;
		int xx = x - ws[index - 1];
		if (xx >= tr)
		{
			cw = ws[index];
			index++;
		}
		else {
			cw = ws[index - 1];
		}
		index--;
	}
	auto newx = md::utf8_char_pos(str + ky.x, index, -1);
	newx -= (uint64_t)str;
	index = (uint64_t)newx;
	//curx = cw;
	return (size_t)index;
}

void edit_cx::up_caret()
{
	glm::ivec4 caret = {};
	get_bounds_px();
	auto v1 = get_line_length(ctx->state.cursor);
	auto line_no = ctx->lvs.size();
	auto h = fix_line_height > 0 ? fix_line_height : font_get_lineheight(style.family, style.fontsize, true);
	// 计算选中范围的每行的坐标宽高 
	if (line_no > 0 && ctx->widths.size() > v1.y)
	{
		auto ks = ctx->lvs[v1.y];
		auto w1 = ctx->widths[v1.y];
		{
			auto pstr = ctx->str.c_str();
			caret.x = get_text_rect(style.family, style.fontsize, pstr + ks.x, std::min(ks.y, ctx->state.cursor - ks.x), 0).x;
		}
		//caret.x = w1[ctx->state.cursor - ks.x];
		caret.y = h * v1.y;
		//printf("cursor:\t%d\n", cursor_pos.x);
	}
	ctx->cursor_pos = caret; ctx->cursor_pos.z = h;
}

void edit_cx::up_cursor(bool is)
{
	if (is)
	{
		up_caret();
		glm::ivec2 cs = ctx->cursor_pos;
		auto evs = get_size();		// 视图大小
		auto h = ctx->cursor_pos.z;	// 行高
		if (h < 1)h = 1;
		evs.x -= ctx->_align_pos.x;
		int ey = evs.y - ctx->cursor_pos.z;
		//ey *= h;
		glm::ivec2 pos = {};
		if (ctx->is_scroll) {
			dcscroll(cs.x, evs.x, 2, ctx->scroll_pos.x);
			dcscroll(cs.y, ey, h, ctx->scroll_pos.y);
		}
		else
		{
			ctx->scroll_pos = { .0, .0 };
		}
		if (!(cs.x < 0 || cs.y < 0))
		{
			pos.x += cs.x;
			pos.y += cs.y;
		}
	}
}
