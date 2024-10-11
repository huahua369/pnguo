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
#include <mapView.h>
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
	if (pv.back)
		backgs->remove_atlas(pv.back);
	fronts->remove_atlas(pv.front);
	ltx->free_menu(pv);
	auto p = this;
	pv = ltx->new_menu(width, height, mvs, false, [=](int type, int idx)
		{
			if (ckm_cb)
				ckm_cb(p, type, idx);
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
		if (it->backgs->count())
			it->f->remove(it->backgs);
		it->f->remove(it->fronts);
		it->f->unbind(it->pv.p);
	}
	it->f = mf1;
	if (it->backgs->count())
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
#endif // 1

// todo 树
#if 1
static uint64_t toUInt(const njson& v, uint64_t de = 0)
{
	uint64_t ret = de;
	if (v.is_number())
	{
		ret = v.get<uint64_t>();
	}
	else if (!v.is_null())
	{
		ret = std::atoll(md::trim(v.dump(), "\"").c_str());
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