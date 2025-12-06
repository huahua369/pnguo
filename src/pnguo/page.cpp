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
#include "event.h"

#include "buffer.h"
#include <mapView.h>
#if 1
void show_tooltip(form_x* form, const std::string& str, const glm::ivec2& pos, style_tooltip* bc)
{
	if (!form || !bc || str.empty())return;
	auto div = new plane_cx();
	div->set_fontctx(form->app->font_ctx);
	div->add_familys(bc->family, 0);
	div->fontsize = bc->fonst_size;
	div->_css.align_content = flex_align::ALIGN_CENTER;
	div->_css.justify_content = flex_align::ALIGN_CENTER;
	div->_css.align_items = flex_align::ALIGN_CENTER;

	div->_lpos = { 0,0 }; div->_lms = { 0,0 };
	div->border = { bc->color.y,bc->thickness,bc->radius,bc->color.x };
	auto ft = div->ltx;
	auto rc = ft->get_text_rect(0, bc->fonst_size, str.c_str(), str.size());
	auto h = ft->get_lineheight(0, bc->fonst_size);
	div->set_clear_color(0);
	auto drc = rc;
	rc.y = h;
	drc += h;
	div->set_size(drc);
	drc += h;
	auto lp1 = div->add_label(str, rc, 0);
	lp1->id = 1234;
	lp1->text_align = {};
	//auto lp = div->add_cbutton(str, rc, 0);
	if (!form->tooltip)
		form->tooltip = new_form_tooltip(form, drc.x, drc.y);
	else {
		form->tooltip->clear_wt();
		form->tooltip->set_size(drc);
	}
	//form->tooltip->bind(div);
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
	//visible = true;
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
	//visible = false;
	if (f && f->get_visible())
		f->hide();
	for (auto& it : v) {
		if (it.child) {
			it.child->hide(0);
		}
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

void mitem_t::set_data(int w, int h, const std::vector<std::string>& v) {
	std::vector<const char*> vs;
	vs.resize(v.size());
	for (size_t i = 0; i < v.size(); i++)
	{
		vs[i] = v[i].c_str();
	}
	set_data(w, h, vs.data(), vs.size());
}
void mitem_t::set_data(int w, int h, const char** mvs, size_t n)
{
	if (!mvs || n < 1)return;
	width = w;
	height = h;
	v.resize(n);
	for (size_t i = 0; i < n; i++)
	{
		v[i].str = mvs[i];
	}
	if (pv.back)
		backgs->remove_atlas(pv.back);
	fronts->remove_atlas(pv.front);
	ltx->free_menu(pv);
	auto p = this;
	pv = ltx->new_menu(width, height, mvs, n, true, [=](int type, int idx)
		{
			if (ckm_cb)
				ckm_cb(p, type, idx);
			auto pc = v[idx].child;
			switch (type)
			{
			case 0:
			{
				if (pc) {
					cct = pc;
					pc->show(get_idx_pos(idx));// 显示子菜单
				}
			}
			break;
			case 1:
			{
				if (!pc)
					hide(true);	// 点击隐藏
			}
			break;
			case 2:
			{
				if (!pc && cct) {
					cct->hide(false); cct = 0;
				}
			}
			break;
			default:
				break;
			}

		});
	if (pv.back)
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

void mitem_t::set_child(mitem_t* cp, int idx)
{
	if (idx < 0 || idx >= v.size())return;
	v[idx].child = cp; cp->parent = this;
}

bool mitem_t::get_visible()
{
	return f ? f->get_visible() : false;
}

menu_cx::menu_cx()
{
}

menu_cx::~menu_cx()
{
	if (u)delete u; u = nullptr;
}
void menu_cx::set_main(form_x* f)
{
	form = f;
	form->push_menu(this);
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
	//auto mf1 = it->f ? it->f : new_form_popup(form, it->pv.fsize.x, it->pv.fsize.y);
	//if (it->f)
	//{
	//	if (it->backgs->count())
	//		it->f->remove(it->backgs);
	//	it->f->remove(it->fronts);
	//	//it->f->unbind(it->pv.p);
	//}
	//it->f = mf1;
	//if (it->backgs->count())
	//	mf1->add_canvas_atlas(it->backgs);
	////mf1->bind(it->pv.p);
	//mf1->add_canvas_atlas(it->fronts);
	//mf1->set_size(it->pv.fsize);
	//mf1->set_pos(pos);
	//mf1->show();
}

void menu_cx::free_item(mitem_t* p)
{
	if (p)
		delete p;
}
mitem_g* menu_cx::new_menu_g(menu_info* pn, int count, const glm::vec2& msize, std::function<void(mitem_t* p, int type, int id)> cb)
{
	mitem_g* r = {};
	if (pn && count > 0)
	{
		auto mss = msize;
		if (mss.x < 2)
		{
			mss.x = 100;
		}
		if (mss.y < 2)
		{
			mss.y = 30;
		}
		auto ptr = new mitem_t[count];
		auto p = ptr;
		for (size_t i = 0; i < count; i++, p++)
		{
			auto& it = pn[i];
			if (!it.mstr || it.count < 1)continue;
			p->m = this;
			p->ckm_cb = cb;
			p->ltx->set_ctx(form->app->font_ctx);
			for (auto it : familys) {
				p->ltx->add_familys(it.c_str(), 0);
			}
			p->set_data(mss.x, mss.y, it.mstr, it.count);
			pn->ptr = p;
		}
		for (size_t i = 0; i < count; i++, p++)
		{
			auto& it = pn[i];
			if (!it.ptr || it.parent < 0 || it.parent == i || it.parent_idx < 0)continue;
			auto& par = pn[it.parent];
			par.ptr->set_child(it.ptr, it.parent_idx);
		}
		r = new mitem_g();
		r->ptr = ptr;
		r->count = count;
	}
	return r;
}
void menu_cx::show_mg(mitem_g* p, int idx, const glm::vec2& pos)
{
	if (p && p->ptr && idx >= 0 && idx < p->count)
	{
		p->ptr[idx].hide(true);
		p->ptr[idx].show(pos);
		p->cx = idx;
	}
}
void menu_cx::free_menu_g(mitem_g* p)
{
	if (p && p->ptr) {
		delete[] p->ptr;
		delete p;
	}
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
 
// 渲染树节点
//void draw_treenode(cairo_t* cr, layout_text_x* ltx)
//{
//	std::string text;
//	int font_size = 16;
//	int text_color = -1;
//	auto rk = ltx->get_text_rect(0, font_size, text.c_str(), -1);
//	glm::ivec2 ss = { 100,100 };
//	glm::vec2 align = { 1,0.5 };
//	glm::vec4 rc = { 0, 0, ss };
//	ltx->tem_rtv.clear();
//	ltx->build_text(0, font_size, rc, align, text.c_str(), -1, ltx->tem_rtv);
//	ltx->update_text();
//	ltx->draw_text(cr, ltx->tem_rtv, text_color);
//}
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
				auto rt = std::remove(it.str.begin(), it.str.end(), '\n');
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
#if 1
enum InterpolationPath_e :uint8_t
{
	TRANSLATION,
	ROTATION,
	SCALE,
	WEIGHTS
};
enum InterpolationModes_e :uint8_t
{
	LINEAR,
	STEP,
	CUBICSPLINE
};
struct sampler_t
{
	size_t input;
	size_t output;
	InterpolationModes_e interpolation;
};
struct accessor_t
{
	float* _data;	// 数量
	size_t _stride;	// float=1、vec2、vec3、vec4\quat=4
	size_t _count;		// 数量
	size_t size() {
		return _count;
	}
	float* data() {
		return _data;
	}
	const void* get(int i) const
	{
		if (i >= _count)
			i = _count - 1;

		return _data + _stride * i;
	}

	int FindClosestFloatIndex(float val) const
	{
		int ini = 0;
		int fin = _count - 1;

		while (ini <= fin)
		{
			int mid = (ini + fin) / 2;
			float v = *(const float*)get(mid);

			if (val < v)
				fin = mid - 1;
			else if (val > v)
				ini = mid + 1;
			else
				return mid;
		}

		return fin;
	}
	float operator[](size_t _Pos) {
		return _data[_Pos];
	}
};
// todo 动画插值算法
class interpolator_cx
{
public:

	// 获取插值
	//glm::vec4 interpolate(accessor_t* accessors, int target_path, sampler_t& sampler, float t, int stride, float maxTime)
public:
	size_t prevKey = 0;	// 当前帧
	float prevT = 0.0;	// 当前时间
public:
	glm::vec4 slerpQuat(float* q1, float* q2, float t)
	{
		glm::quat r = glm::normalize(glm::slerp(glm::make_quat(q1), glm::make_quat(q2), t));
		return { r.x,r.y,r.z,r.w };
	}

	glm::vec4 step(int prevKey, float* output, int stride)
	{
		glm::vec4 result = {};
		for (auto i = 0; i < stride; ++i)
		{
			result[i] = output[prevKey * stride + i];
		}
		return result;
	}

	glm::vec4 linear(int prevKey, int nextKey, float* output, float t, int stride)
	{
		glm::vec4 result = {};
		for (auto i = 0; i < stride; ++i)
		{
			result[i] = output[prevKey * stride + i] * (1 - t) + output[nextKey * stride + i] * t;
		}
		return result;
	}
	glm::vec4 linear_v3(float* prev, float* next, float t)
	{
		return glm::mix(glm::vec4(prev[0], prev[1], prev[2], 0), glm::vec4(next[0], next[1], next[2], 0), t);
	}

	glm::vec4 cubicSpline(int prevKey, int nextKey, float* output, float keyDelta, float t, int stride)
	{
		// stride: Count of components (4 in a quaternion).
		// Scale by 3, because each output entry consist of two tangents and one data-point.
		auto prevIndex = prevKey * stride * 3;
		auto nextIndex = nextKey * stride * 3;
		auto A = 0;
		auto V = 1 * stride;
		auto B = 2 * stride;

		glm::vec4 result = {};// (stride);
		auto tSq = pow(t, 2);
		auto tCub = pow(t, 3);

		// We assume that the components in output are laid out like this: in-tangent, point, out-tangent.
		// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-c-spline-interpolation
		for (size_t i = 0; i < stride; ++i)
		{
			auto v0 = output[prevIndex + i + V];
			auto a = keyDelta * output[nextIndex + i + A];
			auto b = keyDelta * output[prevIndex + i + B];
			auto v1 = output[nextIndex + i + V];

			result[i] = ((2 * tCub - 3 * tSq + 1) * v0) + ((tCub - 2 * tSq + t) * b) + ((-2 * tCub + 3 * tSq) * v1) + ((tCub - tSq) * a);
		}

		return result;
	}

	void resetKey()
	{
		prevKey = 0;
	}

	glm::vec4 getv4x(float* a, int offset, int stride) {
		glm::vec4 tensor = {};
		for (int i = 0; i < stride; ++i) {
			tensor[i] = a[offset + i];
		}
		return tensor;
	}
	// 获取插值
	glm::vec4 interpolate(accessor_t* accessors, int target_path, sampler_t& sampler, float t, int stride, float maxTime)
	{
		if (t < 0)
		{
			return {};
		}

		auto& input = accessors[sampler.input];
		auto& output = accessors[sampler.output];

		if (output.size() == stride) // no interpolation for single keyFrame animations
		{
			return getv4x(output.data(), 0, stride);
		}

		// Wrap t around, so the animation loops.
		// Make sure that t is never earlier than the first keyframe and never later then the last keyframe.
		t = fmod(t, maxTime);
		t = glm::clamp(t, input[0], input[input.size() - 1]);

		if (prevT > t)
		{
			prevKey = 0;
		}

		prevT = t;

		// Find next keyframe: min{ t of input | t > prevKey }
		size_t nextKey = 0;
		for (size_t i = prevKey; i < input.size(); ++i)
		{
			if (t <= input[i])
			{
				nextKey = glm::clamp(i, (size_t)1, input.size() - 1);
				break;
			}
		}
		prevKey = glm::clamp(nextKey - 1, (size_t)0, nextKey);

		auto keyDelta = input[nextKey] - input[prevKey];

		// Normalize t: [t0, t1] -> [0, 1]
		auto tn = (t - input[prevKey]) / keyDelta;
		// channel.target.path
		if (target_path == (int)InterpolationPath_e::ROTATION)
		{

			if (InterpolationModes_e::CUBICSPLINE == sampler.interpolation)
			{
				// GLTF requires cubic spline interpolation for quaternions.
				// https://github.com/KhronosGroup/glTF/issues/1386
				auto result = cubicSpline(prevKey, nextKey, output.data(), keyDelta, tn, 4);
				result = glm::normalize(result);
				return result;
			}
			else if (sampler.interpolation == InterpolationModes_e::LINEAR)
			{
				auto q0 = getQuat(output.data(), prevKey);
				auto q1 = getQuat(output.data(), nextKey);
				return slerpQuat(&q0.x, &q1.x, tn);
			}
			else if (sampler.interpolation == InterpolationModes_e::STEP)
			{
				return getQuat(output.data(), prevKey);
			}

		}

		switch (sampler.interpolation)
		{
		case InterpolationModes_e::STEP:
			return step(prevKey, output.data(), stride);
		case InterpolationModes_e::CUBICSPLINE:
			return cubicSpline(prevKey, nextKey, output.data(), keyDelta, tn, stride);
		default:
			return linear(prevKey, nextKey, output.data(), tn, stride);
		}
	}

	glm::vec4 getQuat(float* output, int index)
	{
		auto x = output[4 * index];
		auto y = output[4 * index + 1];
		auto z = output[4 * index + 2];
		auto w = output[4 * index + 3];
		return glm::vec4(x, y, z, w);
	}
};

/* 路径动画命令。支持1/2/3维路径运动
	等待(秒)e_wait=0,
	跳到e_vmove = 1,
	直线到e_vline=2,
	2阶曲线到e_vcurve=3,
	3阶曲线到e_vcubic=4
*/
action_t::action_t() {}
action_t::~action_t() {}
// 设置接收结果指针
void action_t::set_dst(float* p) {
	dst = p; _dim = 1;
}
void action_t::set_dst(glm::vec2* p) {
	dst = (float*)p; _dim = 2;
}
void action_t::set_dst(glm::vec3* p) {
	dst = (float*)p; _dim = 3;
}
void action_t::set_dst(glm::vec4* p) {
	dst = (float*)p; _dim = 4;
}
// 添加等待n秒后执行后续命令
void action_t::add_wait(float st) {
	cmds.push_back({ 0, st,1,0 });
	_count++;
}
// 添加路径动画命令,	mt移动所需时间(秒)，dim类型{1=float，2=vec2，3=vec3，4=vec4}，
// target为路径数据，第一个值e_vmove = 1, e_vline=2, e_vcurve=3, e_vcubic=4，后面跟着坐标，count为坐标数量
// 曲线glm::vec2 p, c, c1;
int action_t::add(float mt, int dim, float* target, int count) {
	if (!target || count < 1 || dim < 1 || dim != _dim || !(mt > 0))return -1;
	cmds.reserve(cmds.size() + count * 3);
	cmds.push_back({ 1, mt , count, 0 });//1是路径，mt为这条路径执行总时间，count路径点数量
	_count++;
	if (dim == 1)
	{
		for (size_t i = 0; i < count; i++)
		{
			int t = *target; target++;
			auto pt = target;
			cmds.push_back({ t, 1,0,0 });
			switch (t)
			{
			case 1:
			case 2:
			{
				cmds.push_back({ *pt,0,0,0 });
				target++;
			}
			break;
			case 3:
			{
				cmds.push_back({ pt[0],pt[1], 0,0 });
				target += dim * 2;
			}
			break;
			case 4:
			{
				cmds.push_back({ pt[0],pt[1],pt[2],0 });
				target += dim * 3;
			}
			break;
			default:
				break;
			}
		}
	}
	else if (dim == 2)
	{
		for (size_t i = 0; i < count; i++)
		{
			int t = *target; target++;
			auto pt = (glm::vec2*)target;
			int ks[] = { 0,1,1,1,2 };
			cmds.push_back({ t, ks[t],0,0 });
			switch (t)
			{
			case 1:
			case 2:
			{
				cmds.push_back({ *pt,0,0 });
				target += dim;
			}
			break;
			case 3:
			{
				cmds.push_back({ pt[0], pt[1] });
				target += dim * 2;
			}
			break;
			case 4:
			{
				cmds.push_back({ pt[0],pt[1] });
				cmds.push_back({ pt[2],0,0 });
				target += dim * 3;
			}
			break;
			default:
				break;
			}
		}
	}
	else if (dim == 3)
	{
		for (size_t i = 0; i < count; i++)
		{
			int t = *target; target++;
			auto pt = (glm::vec3*)target;
			int ks[] = { 0,1,1,2,3 };
			cmds.push_back({ t, ks[t],0,0 });
			switch (t)
			{
			case 1:
			case 2:
			{
				glm::vec4 k = { *pt, 0 };
				cmds.push_back(k);
				target += dim;
			}
			break;
			case 3:
			{
				cmds.push_back({ *pt, 0 });
				pt++;
				cmds.push_back({ *pt, 0 });
				target += dim * 2;
			}
			break;
			case 4:
			{
				cmds.push_back({ *pt, 0 });
				pt++;
				cmds.push_back({ *pt, 0 });
				pt++;
				cmds.push_back({ *pt, 0 });
				target += dim * 3;
			}
			break;
			default:
				break;
			}
		}
	}
	else if (dim == 4)
	{
		for (size_t i = 0; i < count; i++)
		{
			int t = *target; target++;
			auto pt = (glm::vec4*)target;
			int ks[] = { 0,1,1,2,3 };
			cmds.push_back({ t, ks[t],0,0 });
			switch (t)
			{
			case 1:
			case 2:
			{
				cmds.push_back(*pt);
				target += dim;
			}
			break;
			case 3:
			{
				cmds.push_back(*pt);
				pt++;
				cmds.push_back(*pt);
				target += dim * 2;
			}
			break;
			case 4:
			{
				cmds.push_back(*pt);
				pt++;
				cmds.push_back(*pt);
				pt++;
				cmds.push_back(*pt);
				target += dim * 3;
			}
			break;
			default:
				break;
			}
		}
	}
	return 0;
}
// 执行
void action_t::play() {
	_pause = false;
}
// 暂停
void action_t::pause() {
	_pause = true;
}
// 返回值：0不执行，1执行结束，2执行中
int action_t::updata_t(float deltaTime) {
	if (_pause)return 0;
	ctime += deltaTime;


	return 0;
}
// 清空命令
void action_t::clear()
{
	_count = 0;
	ctime = 0;
	cmds.clear();
}

// 直线移动，时间秒，目标，原坐标可选
action_t* move2w(float mt, const glm::vec2& target, glm::vec2* src, float wait)
{
	return 0;
}
// 在原坐标为原点增加移动
action_t* move2inc(const glm::vec2& pad, float mt, float wait)
{
	return 0;
}
action_t* at_size(const glm::vec2& dst, float mt)
{
	return 0;
}
action_show_t* wait_show(bool visible, float wait)
{
	return 0;
}

// 更新动画
void update_ecs(float deltaTime)
{
#if 0
	{
		auto view = reg->view<action_t>();
		for (auto e : view) {
			auto& d = view.get<action_t>(e);
			float ct = 0;
			int hr = d.updata_t(deltaTime, ct);
			auto ptr = d.ptr;
			if (ptr->aidx.x >= 0)
			{
				auto ps = d.get<glm::vec2>(ptr->aidx.x);
				if (ps)
					ptr->set_pos(*ps);// 移动
			}
			if (ptr->aidx.y >= 0)
			{
				auto ss = d.get<glm::vec2>(ptr->aidx.y);
				if (ss)
				{
					ptr->set_size(*ss);//变形
				}
			}
			if (hr == 1) {
				d.clear();
				reg->remove<action_t>(e);// 结束动画
			}


		}
	}
	{
		// 显示隐藏
		auto view = reg->view<ui_show_t>();
		for (auto e : view) {
			auto& d = view.get<ui_show_t>(e);
			if (d.ptr)
			{
				if (d.wait > 0)
				{
					d.wait -= deltaTime;
					continue;
				}
				else
				{
					d.ptr->set_visible(d.v);
				}
			}
			reg->remove<ui_show_t>(e);
		}
	}
	// 更新auto大小
	//if (isupauto)
	do {
		auto view0p = reg->view<csize_empty>();
		int inc = 0;
		for (auto e : view0p) {
			auto& v = view0p.get<csize_empty>(e);
			reg->remove<csize_empty>(e);
			inc++;
		}
		if (inc == 0)
		{
			break;
		}
		auto view = reg->view<setsize_t>();
		for (auto e : view) {
			auto& v = view.get<setsize_t>(e);
			auto ns = v.pad;
			auto pr = v.ptr->parent;
			if (!pr || v.ptr->aidx.y >= 0)
			{
				continue;
			}
			auto pss = pr->get_size();
			auto olds = v.ptr->get_size();
			auto nx = (ns.x + pss.x) * ns.z;
			auto ny = (ns.y + pss.y) * ns.w;
			if (nx > 0 && ns.z > 0)
			{
				olds.x = nx;
			}
			if (ny > 0 && ns.w > 0)
			{
				olds.y = ny;
			}
			v.ptr->set_size(olds);
		}
	} while (0);
#endif
}
#endif // 1

// todo 树
#if 1

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
		auto cb = (std::function<void(int, njson*)>*)hz::toUInt(c[".click"]);
		if (cb)
		{
			delete cb;
		}
		delete p;
	}
}


/*
	菜单开关动画实现，通过gid操作坐标，修改g的参数返回到动画处理系统执行
	idx=操作的index，sp为间隔{x子项，y父级}，mt为移动到目标毫秒数，one=true则关闭所有，只展开一个父项。
	vsize返回展开后大小，方便设置滚动视图
*/
struct sw_item_t {
	glm::vec2 size;
	// 执行
	float mt = 0.0;
	float wait = 0.0;
	float wait0 = -1.0;
	glm::vec2 target, from;
	glm::vec2 pad;
	uint8_t type = 0;		// 0从from坐标到目标坐标，1当前位置移到目标，2在原坐标为原点增加移动。隐藏wait0>=0，
	bool visible = true;
};
struct swinfo_t
{
	glm::vec2 pos;
	std::vector<sw_item_t> v;
	bool on = false;
};
void set_moveto(sw_item_t& v, glm::vec2 target, glm::vec2 from, float mt, float wait)
{
	v.type = 0;
	v.mt = mt;
	v.wait = wait;
	v.target = target;
	v.from = from;
	v.pad = {};
}
void set_moveto(sw_item_t& v, float mt, glm::vec2 target, float wait)
{
	v.type = 1;
	v.mt = mt;
	v.wait = wait;
	v.target = target;
	v.pad = {};
}
void set_moveto(sw_item_t& v, glm::vec2 pad, float mt, float wait)
{
	v.type = 2;
	v.mt = mt;
	v.wait = wait;
	v.pad = pad;
}
bool get_switch(std::vector<swinfo_t>& g, int idx, glm::vec3 sp, float mt, bool one, glm::vec2* vsize)
{
	bool ret = false;
	float wait = 0.0, sp3 = sp.z;
	glm::vec2 t = { sp.x,sp.y }, vsize0 = {};
	if (!vsize)vsize = &vsize0;
#if 1
	do {
		if (idx >= g.size() && idx >= 0) { break; }
		glm::vec2 pos = g[0].pos;
		for (size_t x = 0; x < g.size(); x++) {
			auto& vt = g[x];
			auto& v1 = vt.v;
			bool oc = vt.on;
			if (one && x != idx) { vt.on = false; oc = false; }
			if (x == idx) { oc = !oc; vt.on = oc; }
			for (size_t i = 0; i < v1.size(); i++) {
				auto& kt = v1[i];
				glm::vec2 c0 = kt.size;
				kt.wait0 = -1.0;
				if (i > 0) {
					if (!oc) {
						bool visi = kt.visible;
						if (visi) {
							kt.wait0 = 0.0; kt.visible = false;	// 隐藏子项
							glm::vec2 ps1 = { -c0.x * 3, 0 };
							set_moveto(kt, ps1, mt, wait);	// 移动子项到外面
						}
						continue;
					}
					if (one || x == idx) { if (oc) { kt.wait0 = 0.0; kt.visible = true; } }
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
#endif
	//hz::save_json(g, "temp/tr1758.json", 2);
	return ret;
}

void on_navclick(uint64_t idx, void* p)
{
#if 0
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
#endif
}

struct tree2_info_t {
	float round = 0.1;
	float mt = 0.2;
	bool xtype = false;			// 单个展开
	bool sc_visible = true;		// 是否显示滚动条
	std::string gid;
	glm::ivec2 pos = {};		// 整体坐标
	glm::ivec2 cpos = {};		// 坐标
	glm::ivec2 size = {};		// 大小
	glm::ivec2 fontheight = {};		// 字高
	glm::ivec2 space = {};		// 间隔
	glm::ivec2 row_size = {};		// 行宽高
	glm::ivec2 color_idx = {};		// 风格颜色
	glm::ivec2 effect = {};		// 风格
	glm::ivec2 scroll = {};		// 滚动量
	glm::ivec2 crow_size = {};		// 子元素大小
	glm::vec4 asize = {};
	float expmt = {};
	glm::vec2 ips;		// 偏移 
	uint32_t btn_fillcolor[2] = {};
	uint32_t btn_bordercolor[2] = { 0xff000000,0xff000000 };
	float indent = 0.0;
};
struct dv_t
{
	std::string key;
	std::vector<std::string> v;
};
// 创建导航菜单, cb单击回调
void new_tree2(const std::vector<dv_t>& d, tree2_info_t* info, std::function<void(int, njson*)> cb)
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
	float fr = info->round;// toFloat(info["round"], 0.1);

	njson btn, r;
	int i = 0, i1 = d.size();
	njson tr;
	int y = 0;
	std::string gid = info->gid;

	uint32_t divc[2] = {};
	auto& fillc = info->btn_fillcolor;
	auto& bc = info->btn_bordercolor;
	padx = info->indent;
	njson kstr0;
	for (auto& it : d)
	{
		auto str = it.key;
		auto& t0 = push_button(str.c_str(), color_idx.x, i, btn, ss, fh.x);
		auto& kt = it.v;
		njson atn;
		atn["gid"] = gid + "_" + std::to_string(i);
		t0["gid"] = atn["gid"];
		t0["effect"] = effect.x;
		if (bc[0] != 0)
			t0["border_color"] = bc[0];//? bc : 0xccff9e40;
		if (fillc[0] != 0)
			t0["fill_color"] = fillc[0];//? bc : 0xccff9e40;
		auto ps1 = ps;
		ps1.y += y;
		t0["pos"] = { ps1.x,  ps1.y };
		t0["sps"] = { info->ips.x,0.5 };
		atn["pos"] = { ps1.x,  ps1.y };
		y += ss.y + space.x;
		{
			auto& t = atn["v"][0];
			set_ps(t, ss, ps1, false);
			t["s"] = str;
		}
		for (size_t x = 0; x < kt.size(); x++)
		{
			auto nt = kt[x];
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
			c0["pos"] = { ps1.x,  ps1.y };
			if (bc[1] != 0)
				c0["border_color"] = bc[1];
			if (fillc[1])
				c0["fill_color"] = fillc[1];

			c0["sps"] = { info->ips.y,0.5 };
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
	njson base;
	base["sp"] = { space.x,space.y,padx,info->mt };
	base["xtype"] = info->xtype;
	base["gid"] = gid;
	//base["asize"] = info["asize"];
	//base["expmt"] = info["expmt"];
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
			b1["fround"] = info->round;
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
	//dv["rounding"] = info["rounding"];	// 圆角
	glm::ivec2 s = { size.x, size.y };
	dv["size"] = { s.x,s.y };
	dv["pos"] = { pos.x,pos.y };
	//merge_j(dv, info);
	dv["t"] = "div";
	dv[".dp"] = (uint64_t)od;
	dv[".onfree"] = (uint64_t)freecb;
	njson rv;
	rv["t"] = "view";
	dv["fill"] = 0;	dv["border"] = 0; dv.erase("color");
	//rv["border"] = 0x80a05000;	rv["fill"] = 0xff505050; // 边框背景颜色
	//rv["color"] = info["color"];
	//rv["rounding"] = info["rounding"];	// 圆角
	// auto_size{xy一般为负数(直接和父级相加)，zw要大于0}
	rv["auto_size"] = { 0,0,1,1 };
	rv["abs"] = 1;
	rv["size"] = { size.x,size.y };
	rv["vss"] = { size.x - 100,y };
	rv["step"] = { scroll.x, scroll.y };
	/*rv["sc_visible"] = info["sc_visible"];*/
	rv["gid"] = gid;
	rv[".c"] = r;
	dv[".c"].push_back(rv);
	//new_widget(dv, 0);
	//glm::vec2 vsize;
	//if (get_switch(tr, -1, { space.x,space.y,padx }, 0, xtype, &vsize))
	//{
	//	auto sv = get_svptr(gid.c_str(), 0);
	//	push_action(tr);
	//	if (sv)
	//		sv->set_content_size(vsize);
	//}
}
#endif

// 主菜单
namespace mg {
	menu_cx* new_mm(menumain_info* mm)
	{
		if (!mm || !mm->form0 || !mm->fontn || !mm->mvs)return 0;
		auto mainmenu = new plane_cx();
		//mm->form0->bind(mainmenu, 1);	// 绑定主菜单到窗口
		auto p = mainmenu;
		p->add_familys(mm->fontn, 0);
		p->set_color({ 0,1,0,mm->bc_color });
		p->fontsize = 16;

		{
			glm::ivec2  fs = mm->form0->get_size();
			if (fs.x & 1)
				fs.x++;
			if (fs.y & 1)
				fs.y++;
			p->set_size({ fs.x, 30 });
		}
		p->set_pos({});
		glm::vec2 cs = { 1500,1600 };
		auto vs = p->get_size();
		auto mmd = *mm;
		p->set_view(vs, cs);
		p->update_cb = [=](float dt)
			{
				bool r = false;
				if (mmd.form0)
				{
					glm::ivec2 ps = p->get_size(), fs = mmd.form0->get_size();
					if (fs.x & 1)
						fs.x++;
					if (fs.y & 1)
						fs.y++;
					if (ps.x != fs.x)
					{
						p->set_size({ fs.x, 30 });
						r = true;
					}
				}
				return r;
			};

		menu_cx* mc = new menu_cx();	// 菜单管理
		mc->set_main(mmd.form0);
		mc->add_familys(mmd.fontn);
		auto mgp = (mmd.pm && mmd.count && mmd.count == mmd.mvs->size()) ? mc->new_menu_g(mmd.pm, mmd.count, mmd.msize, mmd.cb) : nullptr;

		int xc = 0;
		mc->u = p;
		int ix = 0;
		for (auto& it : mmd.mvs[0])
		{
			auto cbt = p->add_cbutton(it.c_str(), { 60,26 }, (int)uType::info);
			cbt->effect = uTheme::light;
			cbt->hscroll = {};
			cbt->rounding = 0;
			cbt->light = 0.1;

			cbt->click_cb = [=](void* ptr, int clicks)
				{
					if (mmd.mcb)mmd.mcb(ptr, clicks, ix);
				};
			ix++;
			cbt->mevent_cb = [=](void* pt, int type, const glm::vec2& mps)
				{
					static void* enterst = 0;
					auto cp = (color_btn*)pt;
					auto t = (event_type2)type;
					switch (t)
					{
					case event_type2::on_down:
					{
						auto cps = cp->get_pos();
						cps.y += cp->size.y + cp->thickness;
						if (mgp)
							mc->show_mg(mgp, xc, cps);
						hide_tooltip(mmd.form0);
						mmd.form0->uptr = 0;
					}
					break;
					case event_type2::on_enter:
					{
						enterst = pt;
						if (mgp && mgp->cx >= 0)
						{
							auto pmx = &mgp->ptr[mgp->cx];
							if (pmx->get_visible()) {
								auto cps = cp->get_pos();
								cps.y += cp->size.y + cp->thickness;
								pmx->hide(true);
								mc->show_mg(mgp, xc, cps);
							}
						}
					}
					break;
					case event_type2::on_hover:
					{
						// 0.5秒触发悬停事件
						style_tooltip stp = {};
						stp.family = mmd.fontn;
						stp.fonst_size = 14;
						glm::vec2 cps = mps;
						cps.y += 20;
						if (enterst == pt) {
							if (mmd.form0->uptr != pt)
							{
								//show_tooltip(form0, (char*)u8"提示信息！", cps, &stp);
								mmd.form0->uptr = pt;
							}
						}
					}
					break;
					case event_type2::on_leave:
					{
						if (enterst == pt) {
							hide_tooltip(mmd.form0);
							mmd.form0->uptr = 0;
						}
					}
					break;
					default:
						break;
					}
				};
		}
		return mc;
	}

	void free_mm(menu_cx* p)
	{
		if (p)delete p;
	}

}
//!mg

listview_x::listview_x()
{
}

listview_x::~listview_x()
{
}
void listview_x::set_style(text_style_t* t)
{
}

#if 1
#include <regex>

namespace hz {
	// LinearRGB转换为sRGB：0-1的数值
	float rgb2srgbf(float linear)
	{
		float s;
		if (linear <= 0.0031308) {
			s = linear * 12.92;
		}
		else {
			s = 1.055 * pow(linear, 1.0 / 2.4) - 0.055;
		}
		return s;
	}
	//	sRGB转换为LinearRGB 
	float srgb2rgbf(float s)
	{
		float linear;
		if (s <= 0.04045) {
			linear = s / 12.92;
		}
		else {
			linear = pow((s + 0.055) / 1.055, 2.4);
		}
		return linear;
	}
	uint32_t rgb2srgb(uint32_t rgb) {
		uint32_t& c = rgb;
		auto p = (uint8_t*)&rgb;
		for (size_t i = 0; i < 3; i++)
		{
			float k = p[i];
			k /= 255.0;
			k = rgb2srgbf(k);
			k *= 255.0;
			p[i] = k;
		}
		return c;
	}
	uint32_t srgb2rgb(uint32_t s) {
		uint32_t& c = s;
		auto p = (uint8_t*)&s;
		for (size_t i = 0; i < 3; i++)
		{
			float k = p[i];
			k /= 255.0;
			k = rgb2srgbf(k);
			k *= 255.0;
			p[i] = k;
		}
		return c;
	}
	inline bool isse(int ch)
	{
		return ch == 0 || (ch > 0 && isspace(ch));
	}
	inline bool is_json(const char* str, size_t len)
	{
		bool ret = false;
		char* te = (char*)str + len - 1;
		char* t = (char*)str;
		if (str && len > 1)
		{
			do
			{
				while (isse(*t))
				{
					t++;
				}
				if (*t != '[' && *t != '{')
				{
					break;
				}
				while (isse(*te))
				{
					te--;
				}
				if ((*t == '[' && *te == ']') || (*t == '{' && *te == '}'))
				{
					ret = true;
				}
			} while (0);
		}
		return ret;
	}
#ifndef FCV
	union u_col
	{
		unsigned int uc;
		unsigned char u[4];
		struct urgba
		{
			unsigned char r, g, b, a;
		}c;
	};
#define FCV 255.0
#define FCV1 256.0
	// rgba
	inline unsigned int to_uint(glm::vec4 col)
	{
		u_col t;
		t.u[0] = col.x * FCV + 0.5;
		t.u[1] = col.y * FCV + 0.5;
		t.u[2] = col.z * FCV + 0.5;
		t.u[3] = col.w * FCV + 0.5;
		return t.uc;
	}
#endif
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

	//uint64_t toUInt(const njson& v, uint64_t de)
	//{
	//	uint64_t ret = de;
	//	if (v.is_number())
	//	{
	//		ret = v.get<uint64_t>();
	//	}
	//	else if (!v.is_null())
	//	{
	//		ret = std::atoll(md::trim(v.dump(), "\"").c_str());
	//	}
	//	return ret;
	//}

	uint64_t toUInt(const njson& v, uint64_t de)
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
	int64_t toInt(const njson& v, const char* k, int64_t de)
	{
		int64_t ret = de;
		if (v.find(k) == v.end())
		{
			return ret;
		}
		if (v[k].is_number())
		{
			ret = v[k].get<int64_t>();
		}
		else if (!v[k].is_null())
		{
			ret = std::atoll(trim(v[k].dump(), "\"").c_str());
		}
		return ret;
	}
	int64_t toInt(const njson& v, int64_t de)
	{
		int64_t ret = de;
		if (v.is_number())
		{
			ret = v.get<int64_t>();
		}
		else if (!v.is_null())
		{
			ret = std::atoll(trim(v.dump(), "\"").c_str());
		}
		return ret;
	}
	double toDouble(const njson& v, double de)
	{
		double ret = de;
		if (v.is_number())
		{
			ret = v.get<double>();
		}
		else if (!v.is_null())
		{
			ret = std::atof(trim(v.dump(), "\"").c_str());
		}
		return ret;
	}
#define toFloat toDouble

	bool toBool(const njson& v, bool def)
	{
		bool ret = def;
		if (v.is_boolean())
		{
			ret = v.get<bool>();
		}
		else if (v.is_number())
		{
			ret = v.get<int>();
		}
		//else
		//{
		//	ret = trim(v.dump(), "\"") != "null";
		//}
		return ret;
	}
	bool toBool(const njson& v, const char* k, bool def)
	{
		bool ret = def;
		if (v.find(k) != v.end())
		{
			auto& t = v[k];
			if (t.is_boolean())
			{
				ret = t.get<bool>();
			}
			else if (t.is_number())
			{
				ret = t.get<int>();
			}
		}
		//else
		//{
		//	ret = trim(v.dump(), "\"") != "null";
		//}
		return ret;
	}
	std::string toStr(const njson& v, const char* k, const std::string& des)
	{
		std::string ret = des;
		if (v.find(k) == v.end())
		{
			return ret;
		}
		if (v[k].is_string())
		{
			ret = v[k].get<std::string>();
		}
		else
		{
			ret = trim(v[k].dump(), "\"");
		}
		return ret;
	}
	std::string toStr(const njson& v, const std::string& des)
	{
		std::string ret = des;
		if (v.is_null())
		{
			//ret = "";
		}
		else if (v.is_string())
		{
			ret = v.get<std::string>();
		}
		else
		{
			ret = trim(v.dump(), "\"");
		}
		return ret;
	}
	std::string getStr(const njson& v, const std::string& key)
	{
		std::string ret;
		if (v.is_null())
		{
			//ret = "";
		}
		else if (v.is_string())
		{
			ret = v.get<std::string>();
		}
		else if (v.is_object() && key != "" && v.find(key) != v.end())
		{
			ret = v[key].get<std::string>();
		}
		else
		{
			ret = trim(v.dump(), "\"");
		}
		return ret;
	}
	template<class T>
	std::vector<T> toVector(const njson& n)
	{
		std::vector<T> ret;
		if (n.is_array())
		{
			ret = n.get<std::vector<T>>();
		}
		return ret;
	}

	std::string toStr(double price)
	{
		auto res = /*n > 0 ? std::_Floating_to_string(("%." + std::to_string(n) + "f").c_str(), price) :*/ std::to_string(price);
		const std::string format("$1");
		try {
			std::regex r("(\\d*)\\.0{6}|");
			std::regex r2("(\\d*\\.{1}0*[^0]+)0*");
			res = std::regex_replace(res, r2, format);
			res = std::regex_replace(res, r, format);
		}
		catch (const std::exception& e) {
			return res;
		}
		return res;
	}
	std::string toStr(double price, int n)
	{
		auto ret = std::to_string(price);
		//return n > 0 ? std::_Floating_to_string(("%." + std::to_string(n) + "f").c_str(), price) : std::to_string(price);
		if (n > 0)
		{
			auto pos = ret.find('.');
			if (pos != std::string::npos)
			{
				auto c = pos + n + 1;
				if (c < ret.size())
				{
					ret.resize(c);
				}
			}
		}
		return ret;
	}
	std::string toStr(int64_t n)
	{
		return std::to_string(n);
	}
	uint64_t toHex(const njson& v, uint64_t d)
	{
		uint64_t ret = d;
		do {
			if (v.is_string())
			{
				std::string buf = md::replace_s(toStr(v), " ", "");

				char* str = 0;
				//buf.resize(10);
				if (buf[0] == '#')
				{
					buf.erase(buf.begin());
					int k = buf.size();
					if (k > 8)
					{
						buf.resize(8);
					}
					if (k >= 6)
					{
						std::swap(*((short*)&buf[0]), *((short*)&buf[4]));
					}
					else if (k == 3)
					{
						std::string nc;
						for (auto it : buf)
						{
							nc.insert(nc.begin(), { it, it });
						}
						buf = nc;
					}
					if (buf.size() == 8)
					{
						buf = buf.substr(6, 2) + buf.substr(0, 6);
					}
					else if (buf.size() == 6)
					{
						buf.insert(buf.begin(), { 'f','f' });
					}
					buf.insert(buf.begin(), { '0','x' });
				}
				int cs = 10;
				if (buf.find("0x") == 0)
				{
					cs = 16;
				}
				ret = std::strtoll(buf.c_str(), &str, cs);
			}
			else if (v.is_number())
			{
				ret = toUInt(v);
			}
			else if (v.is_array())
			{
				bool isr = false;
				for (auto& it : v)
				{
					if (!it.is_number())
					{
						isr = true;
						break;
						assert(0);
					}
				}
				if (isr)break;
				// 1 3 4数量
				std::vector<double> trv = v;
				trv.resize(4);
				if (v.size() < 4)
				{
					trv[3] = 1.0;
					if (v.size() == 1)
					{
						trv[1] = trv[2] = trv[0];
					}
				}
				glm::vec4 v1;
				for (int i = 0; i < 4; i++) v1[i] = trv[i];
				ret = to_uint(v1);
			}
		} while (0);
		return ret;
	}
	unsigned int toColor(const njson& v, unsigned int d)
	{
		return toHex(v, d);
	}
	std::string ptHex(const std::string& str, char ch, char step)
	{
		char spch[8] = { '%', '0', '2', ch, step, 0 };
		std::string chBuffer;
		char chEach[10];
		int nCount;
		memset(chEach, 0, 10);
		unsigned char* d = (unsigned char*)str.c_str();
		auto len = str.size();
		for (nCount = 0; nCount < len /*&& d[nCount]>0*/; nCount++)
		{
			sprintf(chEach, spch, d[nCount]);
			chBuffer += chEach;
		}
		return chBuffer;
	}
	std::string ptHex(const void* data, size_t size, char ch, char step)
	{
		char spch[8] = { '%', '0', '2', ch, step, 0 };
		std::string chBuffer;
		char chEach[10];
		int nCount;
		memset(chEach, 0, 10);
		unsigned char* d = (unsigned char*)data;
		auto len = size;
		for (nCount = 0; nCount < len /*&& d[nCount]>0*/; nCount++)
		{
			sprintf(chEach, spch, d[nCount]);
			chBuffer += chEach;
		}
		return chBuffer;
	}

	std::string toColor2(unsigned int d)
	{
		std::string t;
		t.resize(sizeof(unsigned int));
		memcpy((void*)t.data(), &d, t.size());
		std::reverse(t.begin(), t.end());
		t = t.empty() ? "0x00" : "0x" + ptHex(t);
		return t;
	}

#if (defined(GLM_VERSION))
	glm::ivec2 toiVec2(const njson& v, int d /*= -1*/)
	{
		glm::ivec2 rv = { d, d };

		if (v.is_number())
		{
			rv[0] = rv[1] = rv[2] = rv[3] = v.get<int64_t>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 2 && i < v.size(); i++)
			{
				rv[i] = v[i].get<int64_t>();
			}
		}
		return rv;
	}
	glm::ivec2 toiVec2(const njson& v, glm::ivec2& ot)
	{
		glm::ivec2& rv = ot;

		if (v.is_number())
		{
			rv[0] = rv[1] = rv[2] = rv[3] = v.get<int64_t>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 2 && i < v.size(); i++)
			{
				rv[i] = v[i].get<int64_t>();
			}
		}
		return rv;
	}
	glm::vec2 toVec2(const njson& v, double d)
	{
		glm::vec2 rv = { d, d };

		if (v.is_number())
		{
			rv[0] = rv[1] = v.get<double>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 2 && i < v.size(); i++)
			{
				rv[i] = v[i].get<double>();
			}
		}
		return rv;
	}
	glm::vec2 toVec2(const njson& v, glm::vec2& ot)
	{
		glm::vec2& rv = ot;

		if (v.is_number())
		{
			rv[0] = rv[1] = v.get<double>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 2 && i < v.size(); i++)
			{
				rv[i] = v[i].get<double>();
			}
		}
		return rv;
	}
	glm::vec3 toVec3(const njson& v, double d)
	{
		glm::vec3 rv = { d, d, d };

		if (v.is_number())
		{
			rv[0] = rv[1] = rv[2] = v.get<double>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 3 && i < v.size(); i++)
			{
				rv[i] = v[i].get<double>();
			}
		}
		return rv;
	}
	glm::vec4 toVec4(const std::vector<double>& v, bool one1)
	{
		glm::vec4 rv;
		{
			std::vector<double> trv = v;
			if (one1 && trv.size() == 1) {
				rv[0] = rv[1] = rv[2] = rv[3] = trv[0];
			}
			else {
				for (size_t i = 0; i < v.size() && i < 4; i++)
				{
					rv[i] = v[i];
				}
			}
		}
		return rv;
	}
	glm::vec4 toVec4(const njson& v, double d)
	{
		glm::vec4 rv = { d, d, d, d };
		if (v.is_number())
		{
			rv[0] = rv[1] = rv[2] = rv[3] = v.get<double>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 4 && i < v.size(); i++)
			{
				rv[i] = v[i].get<double>();
			}
		}
		return rv;
	}
	glm::ivec4 toiVec4(const njson& v, int d)
	{
		glm::ivec4 rv = { d, d, d, d };
		if (v.is_number())
		{
			rv[0] = rv[1] = rv[2] = rv[3] = v.get<int64_t>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 4 && i < v.size(); i++)
			{
				rv[i] = v[i].get<int64_t>();
			}
		}
		return rv;
	}
	glm::ivec3 toiVec3(const njson& v, int d)
	{
		glm::ivec3 rv = { d, d, d };
		if (v.is_number())
		{
			rv[0] = rv[1] = rv[2] = v.get<int64_t>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 3 && i < v.size(); i++)
			{
				rv[i] = v[i].get<int64_t>();
			}
		}
		return rv;
	}
	njson v2to(const glm::vec2& v)
	{
		std::vector<double> rv = { v.x, v.y };
		return rv;
	}
	njson v3to(const glm::vec3& v)
	{
		std::vector<double> rv = { v.x, v.y, v.z };
		return rv;
	}
	njson v4to(const glm::vec4& v)
	{
		std::vector<double> rv = { v.x, v.y, v.z, v.w };
		return rv;
	}

	template <typename T>
	void v2to_a(const T& v, njson& a)
	{
		a.push_back(v.x);
		a.push_back(v.y);
	}
	template <typename T>
	void v3to_a(const T& v, njson& a)
	{
		a.push_back(v.x);
		a.push_back(v.y);
		a.push_back(v.z);
	}
	template <typename T>
	void v4to_a(const T& v, njson& a)
	{
		a.push_back(v.x);
		a.push_back(v.y);
		a.push_back(v.z);
		a.push_back(v.w);
	}
#endif
	template <typename T>
	T* toPtr(const njson& v)
	{
#if 0
		std::string buf = toStr(v);
		unsigned long long ret = 0;
		char* str = 0;
		buf.resize(18);
		ret = std::strtoll(buf.c_str(), &str, buf.find("0x") == 0 ? 16 : 10);
		return (T*)ret;
#else
		return (T*)toHex(v);
#endif
	}
#ifndef _ui64toa_s
	char* _ui64toa_s(uint64_t num, char* str, int size, int radix)
	{/*索引表*/
		char index[] = "0123456789ABCDEF";
		uint64_t unum = 0;/*中间变量*/
		int i = 0, j, k;
		/*确定unum的值*/
		if (radix == 10 && num < 0)/*十进制负数*/
		{
			unum = (uint64_t)-num;
			str[i++] = '-';
		}
		else unum = (uint64_t)num;/*其他情况*/
		/*转换*/
		do {
			str[i++] = index[unum % (uint64_t)radix];
			unum /= radix;
		} while (unum);
		str[i] = '\0';
		/*逆序*/
		if (str[0] == '-')
			k = 1;/*十进制负数*/
		else
			k = 0;

		for (j = k; j <= (i - 1) / 2; j++)
		{
			char temp;
			temp = str[j];
			str[j] = str[i - 1 + k - j];
			str[i - 1 + k - j] = temp;
		}
		return str;
	}
#endif // !_ui64toa_s
	//template <typename T>
	std::string fromPtr(void* pv)
	{
		std::string buf = "0x";
		buf.resize(sizeof(void*) * 4);
		_ui64toa_s((uint64_t)pv, (char*)(buf.data() + 2), buf.size() - 2, 16);
		return buf.c_str();
	}
}
#endif