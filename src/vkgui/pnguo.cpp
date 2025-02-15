
/*
	Copyright (c) 华仔
	188665600@qq.com

	本文件的实现：骨骼动画、图集、控件、面板、字体管理、字体软光栅渲染、flex布局算法、简易路径path_v、样条线、stb加载图片、svg加载、cairo画布输出svg等

	关系：窗口->面板->控件/图形
	面板持有控件，窗口分发事件给面板。

	2024-07-03	增加菜单实现
	2024-07-01	增加矩形阴影生成。修复九宫格mesh渲染。
	2024-06-19	增加slider_tl滑块控件
	2024-06-18	增加网格视图算法初步实现grid_view
	2024-05-10 添加按钮控件
	2024-05-07 添加输入框控件
	2024-04-07 创建本文件，实现2d骨骼动画

	xatlas模型uv展开库
	todo菜单：独立窗口(可选)-面板（单选、多选、图文按钮）
*/



#include <pch1.h>

#include <map>
#include <random>
#include <vector>

#include <algorithm>
#include <array>
#include <fstream>
#include <stack>
#include <memory>
#include <numeric> // iota
#include <stdio.h>
#include <string.h>
#include <unordered_map>


#include <pnguo.h>


#include <stb_image.h>
#include <stb_image_write.h>
#include <stb_rect_pack.h>
// 样条线算法
#include <tinyspline/tinysplinecxx.h>
// 三角化算法
//#include <earcut.hpp>
// 多边形算法
#include <clipper2/clipper.h> 
using namespace Clipper2Lib;
#include <SDL3/SDL_keycode.h>
#include <print_time.h>
#include <mapView.h>
#include <event.h>
#include <thread>
#ifndef no_cairo_ 
#ifdef __cplusplus
extern "C" {
#endif
#include <cairo/cairo.h>
#ifdef _WIN32
#include <cairo/cairo-win32.h>
#endif
#include <cairo/cairo-svg.h>
#include <cairo/cairo-pdf.h>

#ifndef NO_SVG
#include <librsvg/rsvg.h>
#endif
#include <pango/pango-layout.h>
#include <pango/pangocairo.h>

#ifdef __cplusplus
}
#endif
#endif

#ifndef NO_FONT_CX
#include <hb.h>
#include <hb-ot.h>
#include <fontconfig/fontconfig.h> 
#endif


#include <unicode/uchar.h>
#include <unicode/ucnv.h>
#include <unicode/utypes.h>
#include <unicode/ucsdet.h>
#include <unicode/unistr.h>
#include <unicode/ustring.h>
#include <unicode/ubidi.h> 
#include <unicode/uscript.h>  

#include <stb_truetype.h>

#ifdef max
#undef max
#undef min
#endif // max

#if 1 



#include <algorithm>
#include <array>
#include <fstream>
#include <stack>

#include <memory>

#include <numeric> // iota
#include <stdio.h>
#include <string.h>
#include <unordered_map> 

#include <earcut.hpp> 
namespace mapbox {
	namespace util {

		template <>
		struct nth<0, glm::vec2> {
			inline static auto get(const glm::vec2& t) {
				return t.x;
			};
		};
		template <>
		struct nth<1, glm::vec2> {
			inline static auto get(const glm::vec2& t) {
				return t.y;
			};
		};

	} // namespace util
} // namespace mapbox
#endif


namespace gp {
	uint64_t toUInt(const njson& v, uint64_t de = 0);
	int64_t toInt(const njson& v, const char* k, int64_t de);
	int64_t toInt(const njson& v, int64_t de = 0);
	double toDouble(const njson& v, double de = 0);
	std::string toStr(const njson& v, const char* k, const std::string& des = "");
	std::string toStr(const njson& v, const std::string& des = "");
	int64_t str2int(const char* str, int64_t de = 0);
	njson str2ints(const std::string& s);
}

namespace pg
{
	std::string to_string(double _Val)
	{
		const auto _Len = static_cast<size_t>(_scprintf("%.16g", _Val));
		std::string _Str(_Len, '\0');
		sprintf_s(&_Str[0], _Len + 1, "%.16g", _Val);
		return _Str;
	}
	std::string to_string_p(uint32_t _Val)
	{
		void* p = (void*)_Val;
		const auto _Len = static_cast<size_t>(_scprintf("%p", p));
		std::string _Str(_Len, '\0');
		sprintf_s(&_Str[0], _Len + 1, "%p", p);
		return _Str;
	}
	std::string to_string_p(uint64_t _Val)
	{
		void* p = (void*)_Val;
		const auto _Len = static_cast<size_t>(_scprintf("%p", p));
		std::string _Str(_Len, '\0');
		sprintf_s(&_Str[0], _Len + 1, "%p", p);
		return _Str;
	}
	std::string to_string_hex(uint32_t _Val)
	{
		const auto _Len = static_cast<size_t>(_scprintf("%x", _Val));
		std::string _Str(_Len, '\0');
		sprintf_s(&_Str[0], _Len + 1, "%x", _Val);
		return _Str;
	}
	std::string to_string_hex(uint64_t _Val, int n, const char* x)
	{
		std::string fmt = "%";
		if (n > 0)
		{
			fmt += "0" + std::to_string(n);
		}
		fmt += x ? x : "x";
		const auto _Len = static_cast<size_t>(_scprintf(fmt.c_str(), _Val));
		std::string _Str(_Len, '\0');
		sprintf_s(&_Str[0], _Len + 1, fmt.c_str(), _Val);
		return _Str;
	}
	std::string to_string_hex2(uint32_t _Val)
	{
		const auto _Len = static_cast<size_t>(_scprintf("%02x", _Val));
		std::string _Str(_Len, '\0');
		sprintf_s(&_Str[0], _Len + 1, "%02x", _Val);
		return _Str;
	}
	std::string to_string(double _Val, const char* fmt)
	{
		if (!fmt || !*fmt)fmt = "%.16g";
		const auto _Len = static_cast<size_t>(_scprintf(fmt, _Val));
		std::string _Str(_Len, '\0');
		sprintf_s(&_Str[0], _Len + 1, fmt, _Val);
		return _Str;
	}
}

// 2d骨骼动画
#if 1

struct Transform_t
{
	glm::quat _rotation = {};
	glm::vec3 _translation = glm::vec3(0, 0, 0);
	glm::vec3 _scale = glm::vec3(1, 1, 1);

	glm::mat4 LookAt(glm::vec4 source, glm::vec4 target, bool flipY)
	{
		auto mat = glm::inverse(glm::lookAt(glm::vec3(source), glm::vec3(target), glm::vec3(0, flipY ? -1 : 1, 0)));
		return mat;
	}

	glm::mat4 GetWorldMat() const
	{
		return glm::translate(glm::vec3(_translation)) * glm::mat4(_rotation) * glm::scale(glm::vec3(_scale));
	}
};
class transform2d_t
{
public:
	//平移、缩放、 变形
	glm::vec2 pos = {}, scale = {}, shear = {};
	glm::vec2 anchor = {};//锚点
	// 旋转,弧度
	float rotation = 0;
	glm::mat3 mt = glm::mat3(1.0);
public:
	transform2d_t();
	~transform2d_t();
	void set_degrees(float a) { rotation = glm::radians(a); }
	glm::mat3 get() {
		auto romt = glm::translate(glm::mat3(1.0), anchor) * glm::rotate(glm::mat3(1.0), rotation) * glm::translate(glm::mat3(1.0), -anchor);
		mt = glm::translate(glm::mat3(1.0), pos) * romt * glm::scale(glm::mat3(1.0), scale) * glm::shearY(glm::mat3(1.0), shear.y) * glm::shearX(glm::mat3(1.0), shear.x);
		return mt;
	}
	glm::vec2 getv2(const glm::vec2& ps)
	{
		glm::vec3 v2 = { ps, 1.0 };
		v2 = mt * v2;
		return v2;
	}
private:

};

transform2d_t::transform2d_t()
{
}

transform2d_t::~transform2d_t()
{
}

class tfAccessor
{
public:
	const void* _data = NULL;
	int _count = 0;
	int _stride = 0;
	int _dimension = 0;
	int _type = 0;

	glm::vec4 _min = {};
	glm::vec4 _max = {};
	tfAccessor() {}
	~tfAccessor() {}
	const void* Get(int i) const
	{
		if (i >= _count)
			i = _count - 1;

		return (const char*)_data + _stride * i;
	}

	int FindClosestFloatIndex(float val) const
	{
		int ini = 0;
		int fin = _count - 1;

		while (ini <= fin)
		{
			int mid = (ini + fin) / 2;
			float v = *(const float*)Get(mid);

			if (val < v)
				fin = mid - 1;
			else if (val > v)
				ini = mid + 1;
			else
				return mid;
		}

		return fin;
	}
};
class tfSampler
{
public:
	tfAccessor _time;	// input
	tfAccessor _value;	// output
	void SampleLinear(float time, float* frac, float** pCurr, float** pNext) const
	{
		int curr_index = _time.FindClosestFloatIndex(time);
		int next_index = std::min<int>(curr_index + 1, _time._count - 1);

		if (curr_index < 0) curr_index++;

		if (curr_index == next_index)
		{
			*frac = 0;
			*pCurr = (float*)_value.Get(curr_index);
			*pNext = (float*)_value.Get(next_index);
			return;
		}

		float curr_time = *(float*)_time.Get(curr_index);
		float next_time = *(float*)_time.Get(next_index);

		*pCurr = (float*)_value.Get(curr_index);
		*pNext = (float*)_value.Get(next_index);
		*frac = (time - curr_time) / (next_time - curr_time);
		assert(*frac >= 0 && *frac <= 1.0);
	}
};
class tfChannel
{
public:
	tfSampler* sampler = 0;
	enum { TRANSLATION, ROTATION, SCALE, SHEAR/*, WEIGHTS*/ } transformType = {};
	/* LINEAR\STEP:vec2 float vec2 vec2						值p
	*  CUBIC : vec2[3] float[3] vec2[3] vec2[3]		值p,c,c1
	*  bezier、bspline导出时转换成linear
	 */
	enum { LINEAR, STEP, CUBIC/*, BEZIER, BSPLINE*/ } interpolation = LINEAR;
public:
	tfChannel() {}
	~tfChannel()
	{
	}
};

struct tfAnimation
{
	float duration;//动画持续时间
	std::map<int, tfChannel> _channels;//节点id, 通道
	std::string name;
};
struct tfNode
{
	std::vector<int> _children;
	int skinIndex = -1;
	int meshIndex = -1;
	int channel = -1;			// 一节点只有一个通道
	bool bIsJoint = false;
	std::string _name;
	transform2d_t _tranform;
};
struct tfScene
{
	std::vector<uint32_t> _nodes;
};

struct tfSkins
{
	tfAccessor _InverseBindMatrices;
	tfNode* _skeleton = NULL;
	std::vector<int> _jointsNodeIdx;
};
struct tfPrimitives
{
	glm::vec4 _center;
	glm::vec4 _radius;
};

struct tfMesh
{
	std::vector<tfPrimitives> _primitives;	// 包围盒
};
// 渲染结构
class component2d_t
{
public:
	std::vector<tfScene> _scenes;
	std::vector<tfSkins> _skins;
	//std::vector<tfMesh> _meshs;
	std::vector<tfSampler> samplers;		// 动画采样列表
	std::vector<tfAnimation> animations;	// 动画列表
	std::vector<tfNode> nodes;				// 节点列表
	std::vector<glm::mat3> animatedMats;	// 缓存矩阵
	std::vector<glm::mat3> worldSpaceMats;  // 处理层次结构后每个节点的世界空间矩阵
	std::map<int, std::vector<glm::mat3>> worldSpaceSkeletonMats;// 蒙皮矩阵，遵循jointsNodeIdx顺序
	std::vector<char*> buffers;				// 数据
	uint32_t idx = 0;						// 当前执行的动画
	component2d_t() {}
	~component2d_t() {}
	// translation，rotation，scale，shear，，，	
	void update(float delta);

private:
	void SetAnimationTime(uint32_t animationIndex, float time);

};
// 关键帧
struct keyframe_t
{
	float ktime;
	// 旋转
	float rotation = 0;
	//平移、缩放、 变形
	glm::vec2 pos = {}, scale = {}, shear = {};
	int interpolation = 0;// linear=0，step=1，cubicspline=2
};
struct attachment_it
{
	void* tex = 0;			// 纹理
	glm::vec4 color = {};	// 无单独顶点颜色时，skeleton->color * slot->color * attachment->color;
	bool isActive = 0;		// 是否激活
};
struct slot_it
{
	glm::vec4 color = {};
	int blend_mode = 0;				// 混合模式
	attachment_t* attachment = 0;	// 附件
};

struct node_et {
	std::vector<node_et*> children;
};

struct timeline_t
{
	std::string name;	//动画名称
	float duration;		//动画持续时间
	std::map<node_et*, std::vector<keyframe_t>> timeline;
};

// 编辑结构
class component2d_editer_t
{
public:
	std::vector<timeline_t*> animations;
public:
	component2d_editer_t();
	~component2d_editer_t();

private:

};

component2d_editer_t::component2d_editer_t()
{
}

component2d_editer_t::~component2d_editer_t()
{
}

void component2d_t::update(float delta)
{
	SetAnimationTime(idx, delta);
}
/*
* https://github.khronos.org/glTF-Tutorials/gltfTutorial/gltfTutorial_007_Animations.html
*  deltaTime = nextTime - previousTime
 previousTangent = deltaTime * previousOutputTangent
	nextTangent = deltaTime * nextInputTangent
*/
template<class T, class Tv>
T cubicSpline1(const T& previousPoint, const T& previousTangent, const T& nextPoint, const T& nextTangent, const Tv& interpolationValue)
{
	auto t = interpolationValue;
	auto t2 = t * t;
	auto t3 = t2 * t;
	T v1 = previousPoint; v1 *= (2.0 * t3 - 3.0 * t2 + 1.0);
	T v2 = previousTangent; v2 *= (t3 - 2.0 * t2 + t);
	T v3 = nextPoint; v3 *= (-2.0 * t3 + 3.0 * t2);
	T v4 = nextTangent; v4 *= (t3 - t2);
	return v1 + v2 + v3 + v4;
}
template <typename T>
T cubicSpline(const T& vert0, const T& tang0, const T& vert1, const T& tang1, float t) {
	float tt = t * t, ttt = tt * t;
	float s2 = -2 * ttt + 3 * tt, s3 = ttt - tt;
	float s0 = 1 - s2, s1 = s3 - tt + t;
	T p0 = vert0;
	T m0 = tang0;
	T p1 = vert1;
	T m1 = tang1;
	return s0 * p0 + s1 * m0 * t + s2 * p1 + s3 * m1 * t;
}
void component2d_t::SetAnimationTime(uint32_t animationIndex, float dtime)
{
	if (animationIndex < animations.size())
	{
		tfAnimation* anim = &animations[animationIndex];
		//loop animation
		dtime = fmod(dtime, anim->duration);
		for (auto it = anim->_channels.begin(); it != anim->_channels.end(); it++)
		{
			transform2d_t* pSourceTrans = &nodes[it->first]._tranform;
			transform2d_t animated = *pSourceTrans;
			float frac = 0.0, * pCurr = 0, * pNext = 0;
			auto itn = it->second.interpolation;
			if (it->second.interpolation == tfChannel::STEP) {
				dtime = 0.0f;
			}
			it->second.sampler->SampleLinear(dtime, &frac, &pCurr, &pNext);
			auto prev = (glm::vec2*)pCurr;
			auto next = (glm::vec2*)pNext;
			switch (it->second.transformType)
			{
			case tfChannel::TRANSLATION: {
				// Animate translation
				if (it->second.interpolation == tfChannel::CUBIC) {
					auto vert0 = prev + 1;
					auto tang0 = prev + 2;
					auto tang1 = next;
					auto vert1 = next + 1;
					animated.pos = cubicSpline(*vert0, *tang0, *vert1, *tang1, frac);
				}
				else {
					animated.pos = glm::mix(*prev, *next, frac); //linear((1.0f - frac) * prevpoint) + ((frac)*nextpoint);
				}
			}break;
			case tfChannel::ROTATION: {
				// Animate rotation 
				auto prev = pCurr;
				auto next = pNext;
				glm::quat q;
				if (it->second.interpolation == tfChannel::CUBIC) {
					auto vert0 = prev + 1;
					auto tang0 = prev + 2;
					auto tang1 = next;
					auto vert1 = next + 1;
					q = glm::normalize(glm::quat(1.0, 0.0, 0.0, cubicSpline(*vert0, *tang0, *vert1, *tang1, frac)));
				}
				else {
					glm::quat q1(1.0f, 0.0, 0.0, *prev);
					glm::quat q2(1.0f, 0.0, 0.0, *next);
					q = glm::slerp(q1, q2, frac);
				}
				animated.rotation = std::atan2(q.z, q.w);
			}break;
			case tfChannel::SCALE: {
				// Animate scale 
				if (it->second.interpolation == tfChannel::CUBIC) {
					auto vert0 = prev + 1;
					auto tang0 = prev + 2;
					auto tang1 = next;
					auto vert1 = next + 1;
					animated.pos = cubicSpline(*vert0, *tang0, *vert1, *tang1, frac);
				}
				else {
					animated.pos = glm::mix(*prev, *next, frac); //linear((1.0f - frac) * prevpoint) + ((frac)*nextpoint);
				}
			}break;
			case tfChannel::SHEAR: {
				// 斜切动画
				if (it->second.interpolation == tfChannel::CUBIC) {
					auto vert0 = prev + 1;
					auto tang0 = prev + 2;
					auto tang1 = next;
					auto vert1 = next + 1;
					animated.pos = cubicSpline(*vert0, *tang0, *vert1, *tang1, frac);
				}
				else {
					animated.pos = glm::mix(*prev, *next, frac); //linear((1.0f - frac) * prevpoint) + ((frac)*nextpoint);
				}
			}break;
			default:
				break;
			}
			animatedMats[it->first] = animated.get();
		}
	}
}
void skeleton_t::update(float delta)
{
	anim->update(delta);
}
void skeleton_t::set_active(uint32_t idx, uint32_t aidx)
{
	active_idx = idx;
	active_aidx = aidx;
	anim->idx = active_idx;

}
class packget2d_t :public skeleton_t
{
public:
	component2d_t cpd = {};
public:
	packget2d_t();
	~packget2d_t();

private:

};

packget2d_t::packget2d_t()
{
}

packget2d_t::~packget2d_t()
{
}
// 加载骨骼动画
skeleton_t* load_skeleton_file(const void* filepath)
{
	return 0;
}
skeleton_t* load_skeleton_mem(const void* d, size_t len)
{
	return 0;
}








#endif

// 普通图集
#if 1

#if 0
ndef STB_RECT_PACK_VERSION

struct stbrp_rect
{
	// reserved for your use:
	int            id;

	// input:
	uint16_t    w, h;

	// output:
	uint16_t    x, y;
	int            was_packed;  // non-zero if valid packing

}; // 16 bytes, nominally
#endif
#ifdef STB_RECT_PACK_VERSION
class stb_packer
{
public:
	stbrp_context _ctx = {};
	std::vector<uint32_t> ptr;
	image_ptr_t img = {};
	std::vector<stbrp_node> _rpns;
public:
	stb_packer() {}
	~stb_packer() {}
	image_ptr_t* get() {
		return (image_ptr_t*)&img;
	}
	void init_target(int width, int height, bool newptr = true) {
		assert(!(width < 10 || height < 10));
		if (width < 10 || height < 10)return;
		if (newptr) { ptr.resize(width * height); }
		auto img = get();
		img->width = width;
		img->height = height;
		img->valid = 1;
		img->data = ptr.data();
		_rpns.resize(width);
		memset(_rpns.data(), 0, _rpns.size() * sizeof(stbrp_node));
		stbrp_init_target(&_ctx, width, height, _rpns.data(), _rpns.size());
		stbrp_setup_allow_out_of_mem(&_ctx, 0);
	}
	void clear() {
		memset(_rpns.data(), 0, _rpns.size() * sizeof(stbrp_node));
		init_target(_ctx.width, _ctx.height);
	}
	int push_rect(glm::ivec4* rc, int n)
	{
		if (!rc || n < 1)return 0;
		std::vector<stbrp_rect> rct(n);
		auto r = rc;
		for (auto& it : rct)
		{
			it.w = r->z; it.h = r->w; r++;
		}
		int ret = stbrp_pack_rects(&_ctx, rct.data(), n);
		r = rc;
		for (auto& it : rct)
		{
			r->x = it.x; r->y = it.y; r++;
		}
		auto img = get();
		img->valid = 1;
		return ret;
	}
	int push_rect(glm::ivec2 rc, glm::ivec2* pos)
	{
		stbrp_rect rct[2] = {};
		rct->w = rc.x;
		rct->h = rc.y;
		int ret = stbrp_pack_rects(&_ctx, rct, 1);
		if (pos)
		{
			*pos = { rct->x,rct->y };
		}
		auto img = get();
		img->valid = 1;
		return ret;
	}
public:
	// todo stb结构
	int pack_rects(stbrp_rect* rects, int num_rects)
	{
		return stbrp_pack_rects(&_ctx, rects, num_rects);
	}
	void setup_allow_out_of_mem(int allow_out_of_mem)
	{
		stbrp_setup_allow_out_of_mem(&_ctx, allow_out_of_mem);
	}
	//可以选择库应该使用哪个打包启发式方法。不同启发式方法将为不同的数据集生成更好/更差的结果。 如果再次调用init，将重置为默认值。	
	void setup_heuristic(int heuristic = 1)
	{
		stbrp_setup_heuristic(&_ctx, heuristic);
	}
private:

};

void test_rect()
{
	glm::ivec2 r = { 730,1000 };
	stb_packer pack;
	pack.init_target(r.x, r.y);
	//hz::Image tespack;
	//tespack.resize(r.x, r.y);
	std::vector<uint32_t> colors = {
	0xff0000ff,
	0xff00ff00,
	0xffff0000,
	0xff0080ff,
	0xff8000ff,
	0xff00ff80,
	0xff80ff00,
	0xffff0080,
	0xffff8000,

	};
	std::vector<glm::ivec2> rects = {
		//{1120,800},
		{700,300},
		{470,300},
		{230,140},
		{230,140},
		{350,140},
		{360,300},
		{350,140},
		{350,140},
		{350,140},
		{350,140},
		{350,140},
		{350,140},
	};
	{
		print_time at("stb pack");
		int i = 0;
		for (auto& it : rects)
		{
			glm::ivec2 prs;
			int k = pack.push_rect({ it.x,it.y }, &prs);
			if (k)
			{
				glm::ivec4 trc = { prs.x,prs.y,it.x,it.y };
				auto c = colors[i++];
				c |= 0x80000000;
				//tespack.draw_rect(trc, 0, c);
			}
		}
	}
	std::string fn = "test_packer_stb.png";
	//tespack.saveImage(fn);
	exit(0);
}

#endif // STB_RECT_PACK_VERSION

stbimage_load::stbimage_load()
{
}

stbimage_load::stbimage_load(const char* fn)
{
	load(fn);
}

stbimage_load::~stbimage_load()
{
	stbi_image_free(data);
}
void stbimage_load::free_img(stbimage_load* p)
{
	if (p)
		delete p;
}
stbimage_load* stbimage_load::new_load(const void* fnd, size_t len)
{
	stbimage_load t;
	if (fnd)
	{
		if (len)
			t.load_mem((char*)fnd, len);
		else
			t.load((char*)fnd);
	}
	if (t.data && t.width && t.height)
	{
		auto p = new stbimage_load();
		if (p)
		{
			*p = t;
			t.data = 0;
		}
		return p;
	}
	return nullptr;
}
void stbimage_load::tobgr()
{
	auto n = width * height;
	auto t = (char*)data;
	for (size_t i = 0; i < n; i++)
	{
		std::swap(*(t + 0), *(t + 2));
		t += 4;
	}
}
bool stbimage_load::load(const char* fn)
{
	hz::mfile_t mf;
	if (!fn || !*fn)return false;
	auto rawd = mf.open_d(fn, true);
	if (!rawd)
	{
		return false;
	}
	data = (uint32_t*)stbi_load_from_memory((stbi_uc*)rawd, mf.size(), &width, &height, &rcomp, comp);
	type = 0;
	return (data ? true : false);
}
bool stbimage_load::load_mem(const char* d, size_t s)
{
	data = (uint32_t*)stbi_load_from_memory((stbi_uc*)d, s, &width, &height, &rcomp, comp);
	type = 0;
	return (data ? true : false);
}
void save_img_png(image_ptr_t* p, const char* str)
{
	if (p && str && *str) {
		stbi_write_png(str, p->width, p->height, p->comp, p->data, p->stride);
	}
}

// 预乘输出bgra，type=0为原数据是rgba
void premultiply_data(int w, unsigned char* data, int type, bool multiply);


struct u84
{
	uint8_t r, g, b, a;
};
inline double get_alpha_f(uint32_t c)
{
	auto* t = (u84*)&c;
	return  t->a / 255.0;
}
inline bool is_alpha(uint32_t c)
{
	return (c & 0xFF000000);
}
inline uint32_t set_alpha(uint32_t c, uint32_t a)
{
	auto* t = (u84*)&c;
	t->a = std::min(a, (uint32_t)255);
	return c;
}
#ifdef USE_BGRA_PACKED_COLOR
#define COL32_R_SHIFT    16
#define COL32_G_SHIFT    8
#define COL32_B_SHIFT    0
#define COL32_A_SHIFT    24
#define COL32_A_MASK     0xFF000000
#else
#define COL32_R_SHIFT    0
#define COL32_G_SHIFT    8
#define COL32_B_SHIFT    16
#define COL32_A_SHIFT    24
#define COL32_A_MASK     0xFF000000
#endif
glm::vec4 ColorConvertU32ToFloat4(uint32_t in)
{
	float s = 1.0f / 255.0f;
	return glm::vec4(
		((in >> COL32_R_SHIFT) & 0xFF) * s,
		((in >> COL32_G_SHIFT) & 0xFF) * s,
		((in >> COL32_B_SHIFT) & 0xFF) * s,
		((in >> COL32_A_SHIFT) & 0xFF) * s);
}
void px_blend2c(uint32_t* pDstBmp, uint32_t src, uint32_t col)
{
	// C实现
	unsigned char* pSrc = (unsigned char*)&src;
	unsigned char* pDst = (unsigned char*)pDstBmp;
	uint32_t below_A, below_R, below_G, below_B;
	uint32_t above_A, above_R, above_G, above_B;
	glm::vec4 cf;

	above_B = *pSrc++;
	above_G = *pSrc++;
	above_R = *pSrc++;
	above_A = *pSrc++;
	if (col != -1 && above_A > 0)
	{
		cf = ColorConvertU32ToFloat4(col);
		above_B = cf.x * above_B;
		above_G = cf.y * above_G;
		above_R = cf.z * above_B;
		above_A = cf.w * above_A;
	}
	if (above_A == 0)
	{
		pDst += 4;
		return;
	}
	below_B = *pDst;
	below_G = *(pDst + 1);
	below_R = *(pDst + 2);
	below_A = *(pDst + 3);
	if (below_A == 0)
	{
		*pDstBmp = src;
		return;
	}
	uint32_t uc[] = { below_B - (below_B - above_B) * above_A / 255,
		below_G - (below_G - above_G) * above_A / 255,
		below_R - (below_R - above_R) * above_A / 255,
	};
	unsigned char d[4];
	d[0] = below_B - (below_B - above_B) * above_A / 255;
	d[1] = below_G - (below_G - above_G) * above_A / 255;
	d[2] = below_R - (below_R - above_R) * above_A / 255;
	auto lsa = pDst;
	if (below_A == 255)
		d[3] = 255;
	else
		d[3] = below_A - (below_A - above_A) * above_A / 255;
	*pDst++ = d[0];
	*pDst++ = d[1];
	*pDst++ = d[2];
	*pDst++ = d[3];
	return;
}
// 灰度图转rgba
void gray_copy2rgba(image_ptr_t* dst, image_ptr_t* src, const glm::ivec2& dst_pos, const glm::ivec4& rc, uint32_t col, bool isblend)
{
	if (src && src->data && src->comp == 1 && dst && dst->width > 0 && dst->height > 0 && dst->data && dst->comp == 4)
	{
		float brightness = 0;
		int w = rc.z, h = rc.w, posx = rc.x, posy = rc.y;
		glm::ivec2 ts = { dst->width,dst->height };
		auto bdata = (uint32_t*)dst->data;
		auto bit = (unsigned char*)(src->data) + (rc.y * src->width);

		int pitch = src->width;
		int ic = 0;
		auto ca = get_alpha_f(col);
		for (int j = 0; j < h && (j + dst_pos.y) < ts.y; j++)
		{
			auto pj = pitch * j;
			unsigned char* pixel = bit + pj;
			auto jp = j + dst_pos.y;
			int64_t psy = (jp * ts.x);
			if (psy < 0 || jp >= dst->height)
			{
				continue;
			}
			auto expanded_data = bdata + psy;
			uint32_t* dc = (uint32_t*)expanded_data;
			for (int i = 0; (i < w) && ((i + dst_pos.x) < ts.x); i++)
			{
				unsigned char c = pixel[i];
				if (c)
				{
					uint32_t uc = 0, ut = std::min(255.0f, brightness * c + c);
					if (ut > 255)ut = 255;
					ut *= ca;
					uc = set_alpha(col, ut);
					if (isblend)
					{
						px_blend2c(&dc[i + dst_pos.x], uc, -1);
					}
					else {
						dc[i + dst_pos.x] = uc;
					}
					ic++;
				}
			}
		}
		if (ic > 0)
			dst->valid = true;
	}
}
//单色位图1位
void bit_copy2rgba(image_ptr_t* dst, image_ptr_t* src, const glm::ivec2& dst_pos, const glm::ivec4& rc, uint32_t color)
{
	int posx = dst_pos.x, posy = dst_pos.y;
	int w = rc.z, h = rc.w;
	glm::ivec2 outsize = { dst->width,dst->height };
	unsigned char* bit = (unsigned char*)src->data;
	auto bdata = (uint32_t*)dst->data;
	for (int j = 0; j < h && (j + posy) < outsize.y; j++)
	{
		auto jp = j + posy;
		int64_t psy = (jp * outsize.x);
		if (psy < 0 || jp >= dst->height)
		{
			continue;
		}
		unsigned char* pixel = bit + src->stride * j;
		auto expanded_data = bdata + psy;
		unsigned int* dc = (unsigned int*)expanded_data;
		for (int i = 0; (i < w) && ((i + posx) < outsize.x); i++)
		{
			unsigned char c = (pixel[i / 8] & (0x80 >> (i & 7))) ? 255 : 0;
			if (c)
			{
				dc[i + posx] = color;
			}
		}
	}
}
void rgba_copy2rgba(image_ptr_t* dst, image_ptr_t* src, const glm::ivec2& dst_pos, const glm::ivec4& rc, bool isblend)
{
	if (src && src->data && src->comp == 4 && dst && dst->width > 0 && dst->height > 0 && dst->data && dst->comp == 4)
	{
		int w = rc.z, h = rc.w, posx = rc.x, posy = rc.y;
		glm::ivec2 ts = { dst->width,dst->height };
		auto bdata = (uint32_t*)dst->data;
		auto bit = (uint32_t*)(src->data) + (rc.y * src->width);

		int pitch = src->width;
		int ic = 0;
		for (int j = 0; j < h && (j + dst_pos.y) < ts.y; j++)
		{
			auto pj = pitch * j;
			auto pixel = bit + pj;
			auto jp = j + dst_pos.y;
			int64_t psy = (jp * ts.x);
			if (psy < 0 || jp >= dst->height)
			{
				continue;
			}
			auto expanded_data = bdata + psy;
			uint32_t* dc = (uint32_t*)expanded_data;
			for (int i = 0; (i < w) && ((i + dst_pos.x) < ts.x); i++)
			{
				auto c = pixel[i];
				if (c)
				{
					if (isblend)
					{
						px_blend2c(&dc[i + dst_pos.x], c, -1);
					}
					else {
						dc[i + dst_pos.x] = c;
					}
					ic++;
				}
			}
		}
		if (ic > 0)
			dst->valid = true;
	}
}






vertex_v2::vertex_v2() {}
vertex_v2::vertex_v2(glm::vec3 p, glm::vec2 u, uint32_t  c) :position(p.x, p.y), tex_coord(u) {
	color = colorv4(c);
}
vertex_v2::vertex_v2(glm::vec2 p, glm::vec2 u, uint32_t  c) :position(p), tex_coord(u) {
	color = colorv4(c);
}
vertex_v2::vertex_v2(glm::vec2 p, glm::vec2 u, glm::vec4 c) :position(p), tex_coord(u), color(c) {}

// 图集数据

atlas_cx::atlas_cx()
{
}

atlas_cx::~atlas_cx()
{
}
void atlas_cx::add(image_rc_t* d, size_t count)
{
	if (d && count > 0)
	{
		_imgv.reserve(_imgv.size() + count);
		for (size_t i = 0; i < count; i++)
		{
			auto it = d[i];
			image_sliced_t dt = {};
			dt.img_rc = it.img_rc;
			dt.tex_rc = it.tex_rc;
			dt.color = it.color;
			_imgv.push_back(dt);
		}
	}
}
void atlas_cx::add(image_sliced_t* d, size_t count) {

	if (d && count > 0)
	{
		_imgv.reserve(_imgv.size() + count);
		for (size_t i = 0; i < count; i++)
		{
			auto dt = d[i];
			_imgv.push_back(dt);
		}
	}
}
void atlas_cx::add(const glm::ivec4& rc, const glm::ivec4& texrc, const glm::ivec4& sliced, uint32_t color) {

	image_sliced_t dt = {};
	dt.img_rc = rc;
	dt.tex_rc = texrc;
	dt.color = color;
	dt.sliced = sliced;
	_imgv.push_back(dt);
}
void atlas_cx::clear() {
	_imgv.clear();
}


mesh2d_cx::mesh2d_cx()
{
}

mesh2d_cx::~mesh2d_cx()
{
}

void mesh2d_cx::add(std::vector<vertex_v2>& vertex, std::vector<int>& vt_index, void* user_image, const glm::ivec4& clip)
{
	auto ps0 = vertex.size();
	auto ix0 = vt_index.size();
	auto ps = vtxs.size();
	auto ix = idxs.size();
	auto ic = vt_index.size();
	vtxs.resize(ps + vertex.size());
	idxs.resize(ix + vt_index.size());
	auto& cd = cmd_data;
	if (cd.empty())
	{
		cd.push_back({});
	}
	auto dt = &cd[cd.size() - 1];
	auto pidx = idxs.data() + ix;
	if (dt->texid != user_image || dt->clip_rect != clip)
	{
		if (dt->elemCount > 0)
			cd.push_back({});
		dt = &cd[cd.size() - 1];
		dt->texid = user_image;
		dt->clip_rect = clip;
		dt->vtxOffset = ps;
		dt->idxOffset = ix;
		dt->elemCount = ic;
		dt->vCount = ps0;
	}
	else
	{
		// 合批
		dt->elemCount += ic;
		dt->vCount += ps0;
		auto idt = vt_index.data();
		for (size_t i = 0; i < ic; i++)
		{
			idt[i] += ix;
		}
	}
	memcpy(vtxs.data() + ps, vertex.data(), vertex.size() * sizeof(vertex[0]));
	memcpy(pidx, vt_index.data(), vt_index.size() * sizeof(vt_index[0]));

}

canvas_atlas::canvas_atlas()
{
}

canvas_atlas::~canvas_atlas()
{
	if (destroy_texture_cb)
	{
		for (auto it : _texs_t)
		{
			if (it)
			{
				auto p = it;
				destroy_texture_cb(p);
			}
		}
	}
	for (auto it : _gs) {
		if (it && it->autofree)delete it;
	}
	_gs.clear();
	for (auto it : _atlas_cx) {
		if (it && it->autofree)delete it;
	}
	_atlas_cx.clear();
}
gshadow_cx* canvas_atlas::new_gs()
{
	auto p = new gshadow_cx();
	p->autofree = 1;
	_gs.push_back(p);
	return p;
}
void canvas_atlas::add_atlas(atlas_cx* p)
{
	if (p)
	{
		_atlas_cx.push_back(p); valid = true;
	}
}
void canvas_atlas::remove_atlas(atlas_cx* p)
{
	if (p)
	{
		auto& v = _atlas_cx;
		v.erase(std::remove(v.begin(), v.end(), p), v.end()); valid = true;
	}
}
void canvas_atlas::add_atlas(atlas_t* p) {
	if (p)
	{
		_atlas_t.push_back(p); valid = true;
	}
}
void canvas_atlas::remove_atlas(atlas_t* p) {
	if (p)
	{
		auto& v = _atlas_t;
		v.erase(std::remove(v.begin(), v.end(), p), v.end()); valid = true;
	}
}
size_t canvas_atlas::count()
{
	return _atlas_t.size() + _atlas_cx.size();
}
image_ptr_t* canvas_atlas::new_image2(const void* file)
{
	return stbimage_load::new_load(file, 0);
}
image_ptr_t* canvas_atlas::new_image2mem(const void* d, size_t s)
{
	return stbimage_load::new_load(d, s);
}
void canvas_atlas::free_image(image_ptr_t* p)
{
	auto t = (stbimage_load*)p;
	stbimage_load::free_img(t);
}
void canvas_atlas::convert_bgr_multiply(image_ptr_t* img)
{
	if (!img)return;
	auto t = (unsigned char*)img->data;
	if (img->type == 0 || !img->multiply) {
		bool mul = !img->multiply;
		for (size_t i = 0; i < img->height; i++)
		{
			premultiply_data(img->width, t, img->type, mul);
			t += img->stride;
		}
		img->type = 1;
	}
}

// 需要先创建纹理
void canvas_atlas::apply()
{
	if (!valid || (_atlas_t.empty() && _atlas_cx.empty()))return;
	clear();

	for (auto it : _atlas_cx)
	{
		image_rs r = {};
		if (it->_imgv.empty() || !it->img) { continue; }
		r.img = it->img;
		uint32_t color = -1;
		for (auto& kt : it->_imgv) {

			auto vt = kt.img_rc;
			r.rect = kt.tex_rc;
			r.size = { vt.z,vt.w };
			glm::vec2 npos = vt;
			r.sliced = kt.sliced;
			add_image(&r, npos, kt.color ? kt.color : color, it->clip);
		}
	}
	for (auto it : _atlas_t)
	{
		image_rs r = {};
		if (it->count < 1 || !it->img) { continue; }
		r.img = it->img;
		uint32_t color = -1;
		if (it->sliced)
		{
			for (size_t i = 0; i < it->count; i++)
			{
				auto vt = it->img_rc[i];
				r.rect = it->tex_rc[i];
				r.size = { vt.z,vt.w };
				glm::vec2 npos = vt;
				r.sliced = it->sliced[i];
				add_image(&r, npos, it->colors ? it->colors[i] : color, it->clip);
			}
		}
		else {
			r.sliced = {};
			for (size_t i = 0; i < it->count; i++)
			{
				auto vt = it->img_rc[i];
				r.rect = it->tex_rc[i];
				r.size = { vt.z,vt.w };
				glm::vec2 npos = vt;
				add_image(&r, npos, it->colors ? it->colors[i] : color, it->clip);
			}
		}
	}
	valid = false;
}


inline uint8_t is_rect_intersect(int x01, int x02, int y01, int y02,
	int x11, int x12, int y11, int y12)
{
	int zx = abs(x01 + x02 - x11 - x12);
	int x = abs(x01 - x02) + abs(x11 - x12);
	int zy = abs(y01 + y02 - y11 - y12);
	int y = abs(y01 - y02) + abs(y11 - y12);
	if (zx <= x && zy <= y)
		return 1;
	else
		return 0;
}
inline bool is_rect_intersect(glm::vec4 r1, glm::vec4 r2)
{
	//第一种情况：如果b.x > a.x + a.w，则a和b一定不相交，
		//第二种情况：如果a.y > b.y + b.h，则a和b一定不相交，
		//第三种情况：如果b.y > a.y + a.h，则a和b一定不相交，
		//第四种情况：如果a.x > b.x + b.w，则a和b一定不相交
	auto& a = r1; auto& b = r2;
	if (a.x > b.x + b.z || b.x > a.x + a.z || a.y > b.y + b.w || b.y > a.y + a.w) {
		return false;
	}
	else {
		return true;
	}
	return is_rect_intersect(r1.x, r1.y, r1.z, r1.w, r2.x, r2.y, r2.z, r2.w);
	//return !(((r1.z < r2.x) || (r1.w > r2.y)) || ((r2.z < r1.x) || (r2.w > r1.y)));
}
bool canvas_atlas::nohas_clip(glm::ivec4 a)
{
	auto clip = _clip_rect;
	if (clip.z > viewport.z || clip.z < 0)clip.z = viewport.z;
	if (clip.w > viewport.w || clip.w < 0)clip.w = viewport.w;
	if (clip.z < 0 || clip.w < 0)
	{
		return false;
	}
	return (!is_rect_intersect(clip, a));
}
void setVec(glm::vec3& d, const glm::vec2& s)
{
	//d = { s, 0.f };
	d.x = s.x; d.y = s.y; d.z = 0.f;
}
void setVec(glm::vec3& d, const glm::vec3& s)
{
	d = s;
	//d.x = s.x; d.y = s.y; d.z = s.z;
}
void setVec(glm::vec3& d, const glm::vec4& s)
{
	glm::vec4 d1 = s;
	d.x = s.x; d.y = s.y; d.z = s.z;
}
void setVec(glm::dvec3& d, const glm::vec2& s)
{
	//d = { s, 0.f };
	d.x = s.x; d.y = s.y; d.z = 0.f;
}
void setVec(glm::dvec3& d, const glm::vec3& s)
{
	d = s;
	//d.x = s.x; d.y = s.y; d.z = s.z;
}
void setVec(glm::dvec3& d, const glm::vec4& s)
{
	glm::vec4 d1 = s;
	d.x = s.x; d.y = s.y; d.z = s.z;
}
void PrimRectUV(const glm::vec2& a, const glm::vec2& c, const glm::vec2& uv_a, const glm::vec2& uv_c, const glm::vec4& col, int idx
	, vertex_v2* wvtx, int* widx)
{
	glm::vec2 b(c.x, a.y), d(a.x, c.y), uv_b(uv_c.x, uv_a.y), uv_d(uv_a.x, uv_c.y);
	widx[0] = idx; widx[1] = (idx + 1); widx[2] = (idx + 2);
	widx[3] = idx; widx[4] = (idx + 2); widx[5] = (idx + 3);
	wvtx[0].position = a; wvtx[0].tex_coord = uv_a; wvtx[0].color = col;
	wvtx[1].position = b; wvtx[1].tex_coord = uv_b; wvtx[1].color = col;
	wvtx[2].position = c; wvtx[2].tex_coord = uv_c; wvtx[2].color = col;
	wvtx[3].position = d; wvtx[3].tex_coord = uv_d; wvtx[3].color = col;
}
struct vidptr_t {
	vertex_v2* vtx = 0;
	int* idx = 0;
	size_t vsize = 0;
	size_t isize = 0;
	size_t vc = 0;
	size_t ic = 0;
};
vidptr_t PrimReserve(mesh2d_cx* ctx, int idx_count, int vtx_count)
{
	auto dc = &ctx->cmd_data[ctx->cmd_data.size() - 1];
	dc->elemCount += idx_count;
	dc->vCount += vtx_count;
	dc->idxOffset;
	dc->vtxOffset;
	auto v = ctx->vtxs.size();
	auto i = ctx->idxs.size();
	//if (v + vtx_count >= _vtx_data.size())
	{
		auto ns = v + vtx_count;
		ctx->vtxs.resize(ns);
	}
	//if (i + idx_count >= _idx_data.size())
	{
		auto ns = i + idx_count;
		ctx->idxs.resize(ns);
	}
	auto r = vidptr_t{ &ctx->vtxs[v], &ctx->idxs[i], v, i, (size_t)vtx_count, (size_t)idx_count };
	return r;
}
glm::vec4 color2f4(uint32_t c)
{
	glm::vec4 color4 = {};
	struct cu4
	{
		uint8_t r, g, b, a;
	};
	auto color = (cu4*)&c;
	color4.x = color->r / 255.0f;
	color4.y = color->g / 255.0f;
	color4.z = color->b / 255.0f;
	color4.w = color->a / 255.0f;
	return color4;
}

void canvas_atlas::add_image(image_rs* p, const glm::vec2& npos, uint32_t color32, const glm::ivec4& clip)
{
	auto& rect = p->rect;
	auto a = glm::vec4(npos, p->size);
	glm::ivec2 pos = { a.x, a.y }, size = { a.z, a.w };
	glm::vec4 v4 = { 0, 0, 1, 1 };
	glm::vec4 uv = v4;
	glm::vec2 s = size;
	auto ts = p->img;
	auto texsize = *((glm::ivec2*)ts);
	if (!(rect.x < 0))
	{
		v4 = rect;
		v4.z += v4.x; v4.w += v4.y;//加上原点坐标
		v4.z = glm::min(v4.z, (float)texsize.x);
		v4.w = glm::min(v4.w, (float)texsize.y);
		uv = { v4.x / texsize.x, v4.y / texsize.y, v4.z / texsize.x, v4.w / texsize.y };
		if (uv.x < 0) { uv.x = 0; }
		if (uv.y < 0) { uv.y = 0; }
	}
	if (a.z < 0)
		a.z *= -std::min(rect.z, texsize.x);
	if (a.w < 0)
		a.w *= -std::min(rect.w, texsize.y);
	if (nohas_clip(a))
		return;
	glm::vec4 color3 = color2f4(color32);
	if (p->sliced.x < 1)
	{
		glm::vec2 av = pos, cv = { pos.x + s.x, pos.y + s.y }, uv_a = { uv.x, uv.y }, uv_c{ uv.z, uv.w };
		auto& col = color3;
		glm::vec2 bv(cv.x, av.y), dv(av.x, cv.y), uv_b(uv_c.x, uv_a.y), uv_d(uv_a.x, uv_c.y);
		static std::vector<int> deidx = { 0,1,2,0,2,3 };
		std::vector<vertex_v2> vertex = {
			{av, uv_a, col},
			{bv, uv_b, col},
			{cv, uv_c, col},
			{dv, uv_d, col},
		};
		_mesh.add(vertex, deidx, p->img->texid, clip);// 添加矩形(两个三角形)到mesh
	}
	else
	{
		make_image_sliced(p->img->texid, a, texsize, p->sliced, rect, color32, clip);// 生成九宫格到mesh
	}

}
void canvas_atlas::clear()
{
	_mesh.cmd_data.clear();
	_mesh.vtxs.clear();
	_mesh.idxs.clear();
	_clip_rect = viewport;
	_clip_rect.x = _clip_rect.y = 0;
}
/*


九宫格渲染:
+--+---------------+--+
|0 |       1       |2 |
+--+---------------+--+
|  |               |  |
|  |               |  |
|3 |    center     |4 |
|  |               |  |
+--+---------------+--+
|5 |       6       |7 |
+--+---------------+--+

九宫格:索引
0  12                     14  2
8  4                      6   10

9  5                      7   11
1  13                     15  3
+--+-------------------------+--+
|  |                         |  |
+--+-------------------------+--+
|  |                         |  |
|  |                         |  |
+--+-------------------------+--+
|  |                         |  |
+--+-------------------------+--+
sliced.x=左宽，y上高，z右宽，w下高

SDL_Vertex
*/
void canvas_atlas::make_image_sliced(void* user_image, const glm::ivec4& a, glm::ivec2 texsize, const glm::ivec4& sliced, const glm::ivec4& rect, uint32_t col, const glm::ivec4& clip)
{
	static std::vector<int> vt_index =// { 0,8,12,4,14,6,2,10,11,6,7,4,5,8,9,1,5,13,7,15,11,3 };//E_TRIANGLE_STRIP
	{ 0, 8, 12, 8, 12, 4, 12, 4, 14, 4, 14, 6, 14, 6, 2, 6, 2, 10,
		6, 7, 10, 7, 10, 11, 4, 5, 6, 5, 6, 7, 8, 9, 4, 9, 4, 5,
		9, 1, 5, 1, 5, 13, 5, 13, 7, 13, 7, 15, 7, 15, 11, 15, 11, 3 };//E_TRIANGLE_LIST

	glm::ivec2 pos = { a.x, a.y }, size = { a.z, a.w };
	glm::vec4 uv = { 0, 0, 1, 1 };
	glm::vec4 v4 = { 0, 0, texsize.x, texsize.y };
	if (!(rect.x < 0))
	{
		v4 = rect;
		v4.z += v4.x; v4.w += v4.y;//加上原点坐标
		uv = { v4.x / texsize.x, v4.y / texsize.y, v4.z / texsize.x, v4.w / texsize.y, };
	}
	float left = sliced.x,
		top = sliced.y,
		right = sliced.z,
		bottom = sliced.w;
	float x = pos.x, y = pos.y, width = size.x, height = size.y;
	glm::vec4 suv = { (left + v4.x) / texsize.x, (top + v4.y) / texsize.y,
		(v4.z - right) / texsize.x, (v4.w - bottom) / texsize.y };

	//t_vector<vertex_v2>v;
	std::vector<vertex_v2> vertex = {
#if 1
		//0
		{{x, y}, {uv.x, uv.y}, col},
		//1
		{{x, y + height}, {uv.x, uv.w}, col},
		//2
		{{x + width, y}, {uv.z, uv.y}, col},
		//3
		{{x + width, y + height}, {uv.z, uv.w}, col},
		//4
		{{x + left, y + top}, {suv.x, suv.y}, col},
		//5
		{{x + left, y + height - bottom}, {suv.x, suv.w}, col},
		//6
		{{x + width - right, y + top}, {suv.z, suv.y}, col},
		//7
		{{x + width - right, y + height - bottom}, {suv.z, suv.w}, col},
		//8
		{{x, y + top}, {uv.x, suv.y}, col},
		//9
		{{x, y + height - bottom}, {uv.x, suv.w}, col},
		//10
		{{x + width, y + top}, {uv.z, suv.y}, col},
		//11
		{{x + width, y + height - bottom}, {uv.z, suv.w}, col},
		//12
		{{x + left, y}, {suv.x, uv.y}, col},
		//13
		{{x + left, y + height}, {suv.x, uv.w}, col},
		//14
		{{x + width - right, y}, {suv.z, uv.y}, col},
		//15
		{{x + width - right, y + height}, {suv.z, uv.w}, col},
#else
		//0
		{{x, y}, {0.0f, 0.f}, col},
		//1
		{{x, y + height}, {0.f, 1.0f}, col},
		//2
		{{x + width, y}, {1.0f, 0.f}, col},
		//3
		{{x + width, y + height}, {1.0f, 1.0f}, col},
		//4
		{{x + left, y + top}, {0.0f + suv.x, 0.f + suv.y}, col},
		//5
		{{x + left, y + height - bottom}, {0.f + suv.x, 1.0f - suv.w}, col},
		//6
		{{x + width - right, y + top}, {1.0f - suv.z, 0.f + suv.z}, col},
		//7
		{{x + width - right, y + height - bottom}, {1.0f - suv.z, 1.0f - suv.w}, col},
		//8
		{{x, y + top}, {0.0f, 0.f + suv.y}, col},
		//9
		{{x, y + height - bottom}, {0.f, 1.0f - suv.w}, col},
		//10
		{{x + width, y + top}, {1.0f, 0.f + suv.y}, col},
		//11
		{{x + width, y + height - bottom}, {1.0f, 1.0f - suv.w}, col},
		//12
		{{x + left, y}, {0.0f + suv.x, 0.f}, col},
		//13
		{{x + left, y + height}, {0.f + suv.x, 1.0f}, col},
		//14
		{{x + width - right, y}, {1.0f - suv.z, 0.f}, col},
		//15
		{{x + width - right, y + height}, {1.0f - suv.z, 1.0f}, col},
#endif
	};

	_mesh.add(vertex, vt_index, user_image, clip);

	return;
}

#endif

// B样条线
#if 1 

bspline_ct::bspline_ct() {

}
bspline_ct::~bspline_ct() {
	free_bspline(ptr); ptr = 0;
}
tinyspline::BSpline* bspline_ct::get()
{
	return (tinyspline::BSpline*)ptr;
}
std::vector<glm::vec2> bspline_ct::get_cp2()
{
	std::vector<glm::vec2> cp2;
	tinyspline::BSpline* p = (tinyspline::BSpline*)ptr;
	if (p) {
		auto cp = p->controlPoints();
		for (size_t i = 0; i < cp.size(); i++)
		{
			cp2.push_back({ cp[i],cp[i + 1] }); i++;
		}
	}
	return cp2;
}
void* bspline_ct::new_bspline(glm::vec2* cp, int n, size_t degree) {
	assert(!ptr);
	if (!cp || n < 3 || degree < 2)return 0;
	auto p = new tinyspline::BSpline(n, 2, degree);
	if (p)
	{
		std::vector<tinyspline::real> ctrlp = p->controlPoints();
		auto t = cp;
#ifdef TINYSPLINE_FLOAT_PRECISION
		auto d = (glm::vec2*)ctrlp.data();
		memcpy(d, t, sizeof(glm::vec2) * n);
#else
		auto d = (glm::dvec2*)ctrlp.data();
		for (size_t i = 0; i < n; i++)
		{
			d->x = t->x;
			d->y = t->y;
			t++; d++;
		}
#endif // TINYSPLINE_FLOAT_PRECISION
		p->setControlPoints(ctrlp);
		ptr = p;
		dim = 2;
	}
	return p;
}
void* bspline_ct::new_bspline(glm::dvec2* cp, int n, size_t degree) {
	assert(!ptr);
	if (!cp || n < 3 || degree < 2)return 0;
	auto p = new tinyspline::BSpline(n, 2, degree);
	if (p)
	{
		std::vector<tinyspline::real> ctrlp = p->controlPoints();
		auto t = cp;
#ifdef TINYSPLINE_FLOAT_PRECISION
		auto d = (glm::vec2*)ctrlp.data();
		for (size_t i = 0; i < n; i++)
		{
			d->x = t->x;
			d->y = t->y;
			t++; d++;
		}
#else
		auto d = (glm::dvec2*)ctrlp.data();
		memcpy(d, t, sizeof(glm::dvec2) * n);
#endif // TINYSPLINE_FLOAT_PRECISION
		p->setControlPoints(ctrlp);
		ptr = p;
		dim = 2;
	}
	return p;
}
void* bspline_ct::new_bspline(glm::vec3* cp, int n, size_t degree) {
	assert(!ptr);
	if (!cp || n < 3 || degree < 2)return 0;
	auto p = new tinyspline::BSpline(n, 3, degree);
	if (p)
	{
		std::vector<tinyspline::real> ctrlp = p->controlPoints();
		auto t = cp;
#ifdef TINYSPLINE_FLOAT_PRECISION
		auto d = (glm::vec3*)ctrlp.data();
		memcpy(d, t, sizeof(glm::vec3) * n);
#else
		auto d = (glm::dvec3*)ctrlp.data();
		for (size_t i = 0; i < n; i++)
		{
			d->x = t->x;
			d->y = t->y;
			d->z = t->z;
			t++; d++;
		}
#endif // TINYSPLINE_FLOAT_PRECISION
		p->setControlPoints(ctrlp);
		ptr = p;
		dim = 3;
	}
	return p;
}
void* bspline_ct::new_bspline(glm::dvec3* cp, int n, size_t degree) {
	assert(!ptr);
	if (!cp || n < 3 || degree < 2)return 0;
	auto p = new tinyspline::BSpline(n, 3, degree);
	if (p)
	{
		std::vector<tinyspline::real> ctrlp = p->controlPoints();
		auto t = cp;
#ifdef TINYSPLINE_FLOAT_PRECISION
		auto d = (glm::vec3*)ctrlp.data();
		for (size_t i = 0; i < n; i++)
		{
			d->x = t->x;
			d->y = t->y;
			d->z = t->z;
			t++; d++;
		}
#else
		auto d = (glm::dvec3*)ctrlp.data();
		memcpy(d, t, sizeof(glm::dvec3) * n);
#endif // TINYSPLINE_FLOAT_PRECISION
		p->setControlPoints(ctrlp);
		ptr = p;
		dim = 3;
	}
	return p;
}
// nurbs
void* bspline_ct::new_bspline(glm::vec4* cp, int n, size_t degree)
{
	assert(!ptr);
	if (!cp || n < 3 || degree < 2)return 0;
	auto p = new tinyspline::BSpline(n, 4, degree);
	if (p)
	{
		std::vector<tinyspline::real> ctrlp = p->controlPoints();
		auto t = cp;
#ifdef TINYSPLINE_FLOAT_PRECISION
		auto d = (glm::vec4*)ctrlp.data();
		memcpy(d, t, sizeof(glm::vec4) * n);
#else
		auto d = (glm::dvec4*)ctrlp.data();
		for (size_t i = 0; i < n; i++)
		{
			d->x = t->x;
			d->y = t->y;
			d->z = t->z;
			d->w = t->w;
			t++; d++;
		}
#endif // TINYSPLINE_FLOAT_PRECISION
		p->setControlPoints(ctrlp);
		ptr = p;
		dim = 4;
	}
	return p;
}
void* bspline_ct::new_bspline(glm::dvec4* cp, int n, size_t degree)
{
	assert(!ptr);
	if (!cp || n < 3 || degree < 2)return 0;
	auto p = new tinyspline::BSpline(n, 4, degree);
	if (p)
	{
		std::vector<tinyspline::real> ctrlp = p->controlPoints();
		auto t = cp;
#ifdef TINYSPLINE_FLOAT_PRECISION
		auto d = (glm::vec4*)ctrlp.data();
		for (size_t i = 0; i < n; i++)
		{
			d->x = t->x;
			d->y = t->y;
			d->z = t->z;
			d->w = t->w;
			t++; d++;
		}
#else
		auto d = (glm::dvec4*)ctrlp.data();
		memcpy(d, t, sizeof(glm::dvec4) * n);
#endif // TINYSPLINE_FLOAT_PRECISION
		p->setControlPoints(ctrlp);
		ptr = p;
		dim = 4;
	}
	return p;
}
std::vector<glm::vec2> bspline_ct::sample2(int m)
{
	std::vector<glm::vec2> ot;
	auto t = (tinyspline::BSpline*)ptr;
	if (t && dim == 2) {
		auto sm = t->sample(m);
		for (size_t i = 0; i < sm.size(); i++)
		{
			auto result = glm::vec2(sm[i], sm[i + 1]); i++;
			ot.push_back(result);
		}
	}
	return ot;
}
std::vector<glm::vec3> bspline_ct::sample3(int m)
{
	std::vector<glm::vec3> ot;
	auto t = (tinyspline::BSpline*)ptr;
	if (t && dim == 3) {
		auto sm = t->sample(m);
		for (size_t i = 0; i < sm.size(); i++)
		{
			auto result = glm::vec3(sm[i], sm[i + 1], sm[i + 2]); i += 2;
			ot.push_back(result);
		}
	}
	return ot;
}
std::vector<glm::vec4> bspline_ct::sample4(int m)
{
	std::vector<glm::vec4> ot;
	auto t = (tinyspline::BSpline*)ptr;
	if (t && dim == 4) {
		auto sm = t->sample(m);
		for (size_t i = 0; i < sm.size(); i++)
		{
			auto result = glm::vec4(sm[i], sm[i + 1], sm[i + 2], sm[i + 3]); i += 3;
			ot.push_back(result);
		}
	}
	return ot;
}
void bspline_ct::free_bspline(void* p)
{
	auto t = (tinyspline::BSpline*)p;
	if (t) { delete t; }
}
#endif

// 生成网格
int make_grid(const glm::vec2& vsize, const glm::vec3& step_width, glm::vec2 nps, path_v* data)
{
	if (vsize.x < 1 || vsize.y < 1 || step_width.x < 1 || step_width.y < 1 || !data)return 0;
	auto ks1 = vsize;
	auto ks = vsize;
	auto ks2 = vsize;
	ks2 /= 2.0;
	ks2 = glm::ceil(ks2);
	ks.x /= step_width.x;
	ks.y /= step_width.y;
	ks = glm::ceil(ks);
	ks += 2;

	path_v pv, pv1;
	int x = 0;
	int sj = 0;
	for (size_t i = 0; i < ks.x; i++)
	{
		glm::vec2 p1, p2;
		p1.x = p2.x = x;
		p1.y = -ks1.y;
		p2.y = ks1.y;
		if (x >= vsize.x)
		{
			p1 -= nps;
			p2 -= nps;
		}
		else
		{
			p1 += nps;
			p2 += nps;
		}

		if (sj == 0)
		{
			pv1.moveTo(p1); pv1.lineTo(p2);
			sj = step_width.z;
		}
		else {
			pv.moveTo(p1); pv.lineTo(p2);
		}
		sj--;
		x += step_width.x;
	}
	int y = 0; sj = 0;
	for (size_t i = 0; i < ks.y; i++)
	{
		glm::vec2 p1, p2;
		p1.y = p2.y = y;
		p1.x = -ks1.x;
		p2.x = ks1.x;
		if (y >= vsize.y)
		{
			p1 -= nps;
			p2 -= nps;
		}
		else
		{
			p1 += nps;
			p2 += nps;
		}
		if (sj == 0)
		{
			pv1.moveTo(p1); pv1.lineTo(p2);
			sj = step_width.z;
		}
		else {
			pv.moveTo(p1); pv.lineTo(p2);
		}
		sj--;
		y += step_width.y;
	}
	{
		if (pv.size())
			data[0].swap(pv);
		if (pv1.size())
			data[1].swap(pv1);
	}
	return 2;
}



//CircleRect判断点是否在圆/矩形内
bool in_box_cr(const glm::vec2& p, const glm::vec4* c)
{
	bool ret = false;
	do {
		if (!c)break;
		if ((int)c->w > 0)
		{
			auto r = *c;
			ret = !((p.x < r.x) || (p.y < r.y) || (p.x > r.x + r.z - 1) || (p.y > r.y + r.w - 1));
		}
		else {
			//计算点p和 当前圆圆心c 的距离
			int dis = distance(p, glm::vec2(c->x, c->y));
			auto r = c->z - 1;
			//和半径比较
			ret = (dis <= r * r);
		}
	} while (0);
	return ret;
}

glm::ivec2 check_box_cr(const glm::vec2& p, const glm::vec4* d, size_t count)
{
	bool ret = false;
	if (!d)
	{
		return {};
	}
	glm::ivec2 rs = {};
	auto c = d;
	for (size_t i = 0; i < count; i++, c++)
	{
		if ((int)c->w > 0)
		{
			auto r = *c;
			ret = !((p.x < r.x) || (p.y < r.y) || (p.x > r.z /*- 1*/) || (p.y > r.w /*- 1*/));
			if (ret)
			{
				rs.x = ret;
				rs.y = i;
				break;
			}
		}
		else {
			//计算点p和 当前圆圆心c 的距离
			int dis = distance(p, glm::vec2(c->x, c->y));
			auto r = c->z /*- 1*/; if (ret)
			{
				//和半径比较
				ret = (dis <= r * r);
				rs.x = ret;
				rs.y = i;
				break;
			}
		}
	}
	return  rs;
}
glm::ivec2 check_box_cr1(const glm::vec2& p, const glm::vec4* d, size_t count, int stride)
{
	bool ret = false;
	if (!d)
	{
		return {};
	}
	glm::ivec2 rs = {};
	auto t = (char*)d;
	for (size_t i = 0; i < count; i++, t += stride)
	{
		auto c = (glm::vec4*)t;
		if ((int)c->w > 0)
		{
			auto r = *c;
			ret = !((p.x < r.x) || (p.y < r.y) || (p.x > r.x + r.z /*- 1*/) || (p.y > r.y + r.w /*- 1*/));
			if (ret)
			{
				rs.x = ret;
				rs.y = i;
				break;
			}
		}
		else {
			//计算点p和 当前圆圆心c 的距离
			int dis = distance(p, glm::vec2(c->x, c->y));
			auto r = c->z /*- 1*/; if (ret)
			{
				//和半径比较
				ret = (dis <= r * r);
				rs.x = ret;
				rs.y = i;
				break;
			}
		}
	}
	return  rs;
}

// rc= left,top,right,bottom
bool rect_includes(const glm::vec4& rc, const glm::vec2& p)
{
	return (p.x >= rc.x) && (p.x <= rc.z) && (p.y >= rc.y) && (p.y <= rc.w);
}
// rc=left,top,right,bottom
bool rect_includes(const glm::vec4& rc, const glm::vec4& other)
{
	return
		(other.x >= rc.x) && (other.x <= rc.z) &&
		(other.y >= rc.y) && (other.y <= rc.w) &&
		(other.z >= rc.x) && (other.z <= rc.z) &&
		(other.w >= rc.y) && (other.w <= rc.w)
		;
}

// 矩形是否相交, r1={x,y,w,h}
bool check_r2_cross(const glm::vec4& r1, const glm::vec4& r2)
{
	if (glm::max(r1.x, r2.x) <= glm::min(r1.x + r1.z, r2.x + r2.z) && glm::max(r1.y, r2.y) <= glm::min(r1.y + r1.w, r2.y + r2.w))
	{
		return true;   //有交集
	}
	else {
		return false;   //无交集
	}
}

// 圆是否相交
bool check_c2(const glm::vec3& c, const glm::vec3& c1)
{
	bool r = true;
	auto d = glm::distance(glm::vec2(c), glm::vec2(c1));
	if (d > c.z * c.z || d > c1.z * c1.z)
	{
		r = false;
	}
	return r;
}

// 矩形和圆是否相交
bool check_rc(const glm::vec4& rc, const glm::vec3& c)
{
	double x1 = rc.x, y1 = rc.y, x2 = rc.z, y2 = rc.w, r = c.z;
	//条件1
	double minx, miny;
	//找出x方向与cx最接近的
	minx = std::min(abs(x1 - c.x), abs(x2 - c.x));
	//找出y方向与cy最接近的
	miny = std::min(abs(y1 - c.y), abs(y2 - c.y));
	if (minx * minx + miny * miny < r * r)return true;

	//条件2
	double x0 = (x1 + x2) / 2;
	double y0 = (y1 + y2) / 2;
	if ((abs(x0 - c.x) < abs(x2 - x1) / 2 + r) && abs(c.y - y0) < abs(y2 - y1) / 2)
		return true;
	if ((abs(y0 - c.y) < abs(y2 - y1) / 2 + r) && abs(c.x - x0) < abs(x2 - x1) / 2)
		return true;

	return 0;
}







#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // !M_PI

#ifndef THREE_SQRT
#define EPSILON 1e-8
#define EPSILON_NUMERIC 1e-4

#define THREE_SQRT sqrt(3)
#define ONE_THIRD (1.0 / 3.0)
#define INTMAX_2 INTMAX_MAX
#endif

#if 1

#if 1 
bool isAroundZero(float val) {
	return val > -EPSILON && val < EPSILON;
}
bool isNotAroundZero(float val) {
	return val > EPSILON || val < -EPSILON;
}
/**
 * 向量距离平方
 */
float v2DistSquare(float* v1, float* v2) {
	return (v1[0] - v2[0]) * (v1[0] - v2[0])
		+ (v1[1] - v2[1]) * (v1[1] - v2[1]);
}
/**
 * 计算三次贝塞尔值
 */
float cubicAt(float p0, float p1, float p2, float p3, float t) {
	auto onet = 1.0 - t;
	return onet * onet * (onet * p0 + 3.0 * t * p1) + t * t * (t * p3 + 3.0 * onet * p2);
}

/**
 * 计算三次贝塞尔导数值
 */
float cubicDerivativeAt(float p0, float p1, float p2, float p3, float t) {
	auto onet = 1.0 - t;
	return 3.0 * (
		((p1 - p0) * onet + 2.0 * (p2 - p1) * t) * onet
		+ (p3 - p2) * t * t
		);
}

/**
 * 计算三次贝塞尔方程根，使用盛金公式
 */
float cubicRootAt(float p0, float p1, float p2, float p3, float val, float* roots) {
	// Evaluate roots of cubic functions
	auto a = p3 + 3 * (p1 - p2) - p0;
	auto b = 3 * (p2 - p1 * 2 + p0);
	auto c = 3 * (p1 - p0);
	auto d = p0 - val;

	auto A = b * b - 3 * a * c;
	auto B = b * c - 9 * a * d;
	auto C = c * c - 3 * b * d;

	auto n = 0;

	if (isAroundZero(A) && isAroundZero(B)) {
		if (isAroundZero(b)) {
			roots[0] = 0;
		}
		else {
			auto t1 = -c / b;  //t1, t2, t3, b is not zero
			if (t1 >= 0 && t1 <= 1) {
				roots[n++] = t1;
			}
		}
	}
	else {
		auto disc = B * B - 4 * A * C;

		if (isAroundZero(disc)) {
			auto K = B / A;
			auto t1 = -b / a + K;  // t1, a is not zero
			auto t2 = -K / 2.0;  // t2, t3
			if (t1 >= 0 && t1 <= 1) {
				roots[n++] = t1;
			}
			if (t2 >= 0 && t2 <= 1) {
				roots[n++] = t2;
			}
		}
		else if (disc > 0) {
			auto discSqrt = sqrt(disc);
			auto Y1 = A * b + 1.5 * a * (-B + discSqrt);
			auto Y2 = A * b + 1.5 * a * (-B - discSqrt);
			if (Y1 < 0) {
				Y1 = -pow(-Y1, ONE_THIRD);
			}
			else {
				Y1 = pow(Y1, ONE_THIRD);
			}
			if (Y2 < 0) {
				Y2 = -pow(-Y2, ONE_THIRD);
			}
			else {
				Y2 = pow(Y2, ONE_THIRD);
			}
			auto t1 = (-b - (Y1 + Y2)) / (3 * a);
			if (t1 >= 0 && t1 <= 1) {
				roots[n++] = t1;
			}
		}
		else {
			auto T = (2 * A * b - 3 * a * B) / (2 * sqrt(A * A * A));
			auto theta = acos(T) / 3;
			auto ASqrt = sqrt(A);
			auto tmp = cos(theta);

			auto t1 = (-b - 2 * ASqrt * tmp) / (3 * a);
			auto t2 = (-b + ASqrt * (tmp + THREE_SQRT * sin(theta))) / (3 * a);
			auto t3 = (-b + ASqrt * (tmp - THREE_SQRT * sin(theta))) / (3 * a);
			if (t1 >= 0 && t1 <= 1) {
				roots[n++] = t1;
			}
			if (t2 >= 0 && t2 <= 1) {
				roots[n++] = t2;
			}
			if (t3 >= 0 && t3 <= 1) {
				roots[n++] = t3;
			}
		}
	}
	return n;
}

/**
 * 计算三次贝塞尔方程极限值的位置
 * @return 有效数目
 */
int cubicExtrema(float p0, float p1, float p2, float p3, float* extrema) {
	auto b = 6 * p2 - 12 * p1 + 6 * p0;
	auto a = 9 * p1 + 3 * p3 - 3 * p0 - 9 * p2;
	auto c = 3 * p1 - 3 * p0;

	auto n = 0;
	if (isAroundZero(a)) {
		if (isNotAroundZero(b)) {
			auto t1 = -c / b;
			if (t1 >= 0 && t1 <= 1) {
				extrema[n++] = t1;
			}
		}
	}
	else {
		auto disc = b * b - 4 * a * c;
		if (isAroundZero(disc)) {
			extrema[0] = -b / (2 * a);
		}
		else if (disc > 0) {
			auto discSqrt = sqrt(disc);
			auto t1 = (-b + discSqrt) / (2 * a);
			auto t2 = (-b - discSqrt) / (2 * a);
			if (t1 >= 0 && t1 <= 1) {
				extrema[n++] = t1;
			}
			if (t2 >= 0 && t2 <= 1) {
				extrema[n++] = t2;
			}
		}
	}
	return n;
}

/**
 * 细分三次贝塞尔曲线,out[8]
 */
void cubicSubdivide(float p0, float p1, float p2, float p3, float t, float* out) {
	auto p01 = (p1 - p0) * t + p0;
	auto p12 = (p2 - p1) * t + p1;
	auto p23 = (p3 - p2) * t + p2;

	auto p012 = (p12 - p01) * t + p01;
	auto p123 = (p23 - p12) * t + p12;

	auto p0123 = (p123 - p012) * t + p012;
	// Seg0
	out[0] = p0;
	out[1] = p01;
	out[2] = p012;
	out[3] = p0123;
	// Seg1
	out[4] = p0123;
	out[5] = p123;
	out[6] = p23;
	out[7] = p3;
}

/**
 * 投射点到三次贝塞尔曲线上，返回投射距离。
 * 投射点有可能会有一个或者多个，这里只返回其中距离最短的一个。
 * float out[2]
 */
double cubicProjectPoint(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float x, float y, float* out)
{
	// http://pomax.github.io/bezierinfo/#projections
	auto t = 0.0;
	auto interval = 0.005;
	double d = INTMAX_2;
	auto prev = 0.0;
	auto next = 0.0;
	auto d1 = 0.0;
	auto d2 = 0.0;
	float _v0[2] = {}, _v1[2] = {}, _v2[2] = {};
	_v0[0] = x;
	_v0[1] = y;

	// 先粗略估计一下可能的最小距离的 t 值
	// PENDING
	for (auto _t = 0.0; _t < 1; _t += 0.05) {
		_v1[0] = cubicAt(x0, x1, x2, x3, _t);
		_v1[1] = cubicAt(y0, y1, y2, y3, _t);
		d1 = v2DistSquare(_v0, _v1);
		if (d1 < d) {
			t = _t;
			d = d1;
		}
	}
	d = INTMAX_2;

	// At most 32 iteration
	for (auto i = 0; i < 32; i++) {
		if (interval < EPSILON_NUMERIC) {
			break;
		}
		prev = t - interval;
		next = t + interval;
		// t - interval
		_v1[0] = cubicAt(x0, x1, x2, x3, prev);
		_v1[1] = cubicAt(y0, y1, y2, y3, prev);

		d1 = v2DistSquare(_v1, _v0);

		if (prev >= 0 && d1 < d) {
			t = prev;
			d = d1;
		}
		else {
			// t + interval
			_v2[0] = cubicAt(x0, x1, x2, x3, next);
			_v2[1] = cubicAt(y0, y1, y2, y3, next);
			d2 = v2DistSquare(_v2, _v0);

			if (next <= 1 && d2 < d) {
				t = next;
				d = d2;
			}
			else {
				interval *= 0.5;
			}
		}
	}
	// t
	if (out) {
		out[0] = cubicAt(x0, x1, x2, x3, t);
		out[1] = cubicAt(y0, y1, y2, y3, t);
	}
	// console.log(interval, i);
	return sqrt(d);
}

/**
 * 计算三次贝塞尔曲线长度
 */
double cubicLength(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float iteration)
{
	auto px = x0;
	auto py = y0;

	auto d = 0.0;

	auto step = 1.0 / iteration;

	for (auto i = 1; i <= iteration; i++) {
		auto t = i * step;
		auto x = cubicAt(x0, x1, x2, x3, t);
		auto y = cubicAt(y0, y1, y2, y3, t);

		double dx = x - px;
		double dy = y - py;

		d += sqrt(dx * dx + dy * dy);

		px = x;
		py = y;
	}

	return d;
}

/**
 * 计算二次方贝塞尔值
 */
int quadraticAt(float p0, float p1, float p2, float t) {
	auto onet = 1.0 - t;
	return onet * (onet * p0 + 2 * t * p1) + t * t * p2;
}

/**
 * 计算二次方贝塞尔导数值
 */
int quadraticDerivativeAt(float p0, float p1, float p2, float t) {
	return 2.0 * ((1 - t) * (p1 - p0) + t * (p2 - p1));
}

/**
 * 计算二次方贝塞尔方程根
 * @return 有效根数目
 */
int quadraticRootAt(float p0, float p1, float p2, float val, float* roots) {
	auto a = p0 - 2 * p1 + p2;
	auto b = 2.0 * (p1 - p0);
	auto c = p0 - val;

	auto n = 0;
	if (isAroundZero(a)) {
		if (isNotAroundZero(b)) {
			auto t1 = -c / b;
			if (t1 >= 0 && t1 <= 1) {
				roots[n++] = t1;
			}
		}
	}
	else {
		auto disc = b * b - 4 * a * c;
		if (isAroundZero(disc)) {
			auto t1 = -b / (2 * a);
			if (t1 >= 0 && t1 <= 1) {
				roots[n++] = t1;
			}
		}
		else if (disc > 0) {
			auto discSqrt = sqrt(disc);
			auto t1 = (-b + discSqrt) / (2 * a);
			auto t2 = (-b - discSqrt) / (2 * a);
			if (t1 >= 0 && t1 <= 1) {
				roots[n++] = t1;
			}
			if (t2 >= 0 && t2 <= 1) {
				roots[n++] = t2;
			}
		}
	}
	return n;
}

/**
 * 计算二次贝塞尔方程极限值
 */
float quadraticExtremum(float p0, float p1, float p2) {
	auto divider = p0 + p2 - 2 * p1;
	if (divider == 0) {
		// p1 is center of p0 and p2
		return 0.5;
	}
	else {
		return (p0 - p1) / divider;
	}
}

/**
 * 细分二次贝塞尔曲线
 */
void quadraticSubdivide(float p0, float p1, float p2, float t, float* out) {
	auto p01 = (p1 - p0) * t + p0;
	auto p12 = (p2 - p1) * t + p1;
	auto p012 = (p12 - p01) * t + p01;

	// Seg0
	out[0] = p0;
	out[1] = p01;
	out[2] = p012;

	// Seg1
	out[3] = p012;
	out[4] = p12;
	out[5] = p2;
}
/**
 * 投射点到二次贝塞尔曲线上，返回投射距离。
 * 投射点有可能会有一个或者多个，这里只返回其中距离最短的一个。
 * @param {number} x0
 * @param {number} y0
 * @param {number} x1
 * @param {number} y1
 * @param {number} x2
 * @param {number} y2
 * @param {number} x
 * @param {number} y
 * @param {Array.<number>} out 投射点
 * @return {number}
 */
double quadraticProjectPoint(float x0, float y0, float x1, float y1, float x2, float y2, float x, float y, float* out) {
	// http://pomax.github.io/bezierinfo/#projections
	auto t = 0.0;
	auto interval = 0.005;
	auto d = INTMAX_2;

	float _v0[2] = {}, _v1[2] = {}, _v2[2] = {};
	_v0[0] = x;
	_v0[1] = y;

	// 先粗略估计一下可能的最小距离的 t 值
	// PENDING
	for (auto _t = 0.0; _t < 1; _t += 0.05) {
		_v1[0] = quadraticAt(x0, x1, x2, _t);
		_v1[1] = quadraticAt(y0, y1, y2, _t);
		auto d1 = v2DistSquare(_v0, _v1);
		if (d1 < d) {
			t = _t;
			d = d1;
		}
	}
	d = INTMAX_2;

	// At most 32 iteration
	for (auto i = 0; i < 32; i++) {
		if (interval < EPSILON_NUMERIC) {
			break;
		}
		auto prev = t - interval;
		auto next = t + interval;
		// t - interval
		_v1[0] = quadraticAt(x0, x1, x2, prev);
		_v1[1] = quadraticAt(y0, y1, y2, prev);

		auto d1 = v2DistSquare(_v1, _v0);

		if (prev >= 0 && d1 < d) {
			t = prev;
			d = d1;
		}
		else {
			// t + interval
			_v2[0] = quadraticAt(x0, x1, x2, next);
			_v2[1] = quadraticAt(y0, y1, y2, next);
			auto d2 = v2DistSquare(_v2, _v0);
			if (next <= 1 && d2 < d) {
				t = next;
				d = d2;
			}
			else {
				interval *= 0.5;
			}
		}
	}
	// t
	if (out) {
		out[0] = quadraticAt(x0, x1, x2, t);
		out[1] = quadraticAt(y0, y1, y2, t);
	}
	// console.log(interval, i);
	return sqrt(d);
}

/**
 * 计算二次贝塞尔曲线长度
 */
double quadraticLength(float x0, float y0, float x1, float y1, float x2, float y2, float iteration)
{
	auto px = x0;
	auto py = y0;

	auto d = 0.0;

	auto step = 1 / iteration;

	for (auto i = 1; i <= iteration; i++) {
		auto t = i * step;
		auto x = quadraticAt(x0, x1, x2, t);
		auto y = quadraticAt(y0, y1, y2, t);

		auto dx = x - px;
		auto dy = y - py;

		d += sqrt(dx * dx + dy * dy);

		px = x;
		py = y;
	}

	return d;
}

/**
 * 三次贝塞尔曲线描边包含判断
 */
bool containStroke(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float lineWidth, float x, float y) {
	if (lineWidth <= 0) {
		return false;
	}
	auto _l = lineWidth;
	// Quick reject
	if (
		(y > y0 + _l && y > y1 + _l && y > y2 + _l && y > y3 + _l)
		|| (y < y0 - _l && y < y1 - _l && y < y2 - _l && y < y3 - _l)
		|| (x > x0 + _l && x > x1 + _l && x > x2 + _l && x > x3 + _l)
		|| (x < x0 - _l && x < x1 - _l && x < x2 - _l && x < x3 - _l)
		) {
		return false;
	}
	auto d = cubicProjectPoint(
		x0, y0, x1, y1, x2, y2, x3, y3,
		x, y, nullptr
	);
	return d <= _l / 2.0;
}
bool containStroke(cubic_v* c, float lineWidth, glm::vec2 p)
{
	return containStroke(c->p0.x, c->p0.y, c->p1.x, c->p1.y, c->p2.x, c->p2.y, c->p3.x, c->p3.y, lineWidth, p.x, p.y);
}

/**
 * 计算二次贝塞尔曲线长度
 */
double quadraticLength(quadratic_v* c, float iteration)
{
	return quadraticLength(c->p0.x, c->p0.y, c->p1.x, c->p1.y, c->p2.x, c->p2.y, iteration);
}

/**
 * 投射点到二次贝塞尔曲线上，返回投射距离。
 * 投射点有可能会有一个或者多个，这里只返回其中距离最短的一个。
 */
double quadraticProjectPoint(quadratic_v* c, glm::vec2 p, glm::vec2* out)
{
	return quadraticProjectPoint(c->p0.x, c->p0.y, c->p1.x, c->p1.y, c->p2.x, c->p2.y, p.x, p.y, (float*)out);
}
/**
 * 计算三次贝塞尔曲线长度
 */
double cubicLength(cubic_v* c, float iteration)
{
	return cubicLength(c->p0.x, c->p0.y, c->p1.x, c->p1.y, c->p2.x, c->p2.y, c->p3.x, c->p3.y, iteration);
}
/**
 * 投射点到三次贝塞尔曲线上，返回投射距离。
 * 投射点有可能会有一个或者多个，这里只返回其中距离最短的一个。
 * float out[2]
 */
double cubicProjectPoint(cubic_v* c, glm::vec2 p, glm::vec2* out)
{
	return cubicProjectPoint(c->p0.x, c->p0.y, c->p1.x, c->p1.y, c->p2.x, c->p2.y, c->p3.x, c->p3.y, p.x, p.y, (float*)out);
}
// 返回点到线段的投射点
glm::vec2 cpol(const glm::vec2& p1, const glm::vec2& a, const glm::vec2& b)
{
	return glm::closestPointOnLine(p1, a, b);
}

glm::vec3 cpol(const glm::vec3& p1, const glm::vec3& a, const glm::vec3& b)
{
	return glm::closestPointOnLine(p1, a, b);
}



//三次贝塞尔曲线
//a1 * (1 - t) * (1 - t) * (1 - t) + 3 * a2 * t * (1 - t) * (1 - t) + 3 * a3 * t * t * (1 - t) + a4 * t * t * t;
/*
t:时间变量 [0-1]
p1:首端点
p2:末端点
c1:首端点控制点
c2:末端点控制点
返回值：t时刻贝塞尔曲线上的点
控制点、端点和返回值的数据类型为：vector2d

 * 计算三次贝塞尔值
*/
float cubicAt1(float p0, float p1, float p2, float p3, float t) {
	auto onet = 1.0 - t;
	return onet * onet * (onet * p0 + 3.0 * t * p1) + t * t * (t * p3 + 3.0 * onet * p2);
}
glm::vec2 thirdOrderBeziercurve(double t, const glm::vec2& p1, const glm::vec2& c1, const glm::vec2& c2, const glm::vec2& p2) {
	if (t < 0 || t>1)
		return {};
	return { cubicAt(p1.x, c1.x, c2.x, p2.x, t), cubicAt(p1.y, c1.y, c2.y, p2.y, t) };
}

//根据时间t获取打断后的两条贝塞尔曲线的控制点
std::vector<glm::vec2> getControlPointByT(const glm::vec2& p1, const glm::vec2& c1, const glm::vec2& c2, const glm::vec2& p2, double t)
{
	//辅助点g
	auto g = c1 * (1 - t) + (c2 * t);
	auto c11 = p1 * (1 - t) + (c1 * t);
	auto c21 = c11 * (1 - t) + (g * t);
	auto c31 = c2 * (1 - t) + (p2 * t);
	auto c22 = g * (1 - t) + (c31 * t);
	return { c11, c21, c22, c31 };
}
//判断点是否在贝塞尔曲线上，如果在（误差范围内），返回时间t和逼近点
/*
  输入:
	  p1,c1,c2,p2：是贝塞尔曲线的参数
	  p:特定点坐标
	  errorValue:误差值
 输出:
  1.如果p点在贝塞尔曲线上，返回打断后的两条贝塞尔曲线的控制点和纠正点[c11,c21,c22,c31,rightPoint]
  2.否则，返回[]
*/
std::vector<glm::vec2> getControlPointByPoint(const glm::vec2& p1, const glm::vec2& c1, const glm::vec2& c2, const glm::vec2& p2, const glm::vec2& p, double errorValue)
{
	double m = 1000000;
	double t = 0;
	glm::vec2 pt(-1, -1);
	for (double i = 0; i <= 1; i = i + 0.01)
	{
		auto pi = thirdOrderBeziercurve(i, p1, c1, c2, p2);
		auto d = glm::distance(pi, p);
		if (d < m)
		{
			m = d;
			t = i;
			pt = pi;
		}
	}
	if (m < errorValue)
	{
		return getControlPointByT(p1, c1, c2, p2, t);
	}
	return {};
}

// 分割曲线
bool tessbc(glm::vec2* c, float t, std::vector<glm::vec2>* opt)
{
	glm::vec2* P1 = (glm::vec2*)(c);
	glm::vec2* C1 = (glm::vec2*)(c + 1);
	glm::vec2* C2 = (glm::vec2*)(c + 2);
	glm::vec2* P2 = (glm::vec2*)(c + 3);

	auto cb = thirdOrderBeziercurve(t, *P1, *C1, *C2, *P2);
	auto ctrs = getControlPointByPoint(*P1, *C1, *C2, *P2, cb, 100); // 新控制点
	if (ctrs.size() == 4)
	{
		opt->push_back(*c);
		opt->push_back(ctrs[0]);
		opt->push_back(ctrs[1]);
		opt->push_back(cb);//中间点
		opt->push_back(ctrs[2]);
		opt->push_back(ctrs[3]);
		opt->push_back(*P2);
	}
	return (ctrs.size() == 4);
}


#endif

namespace bs {
	double sol2(int nn, int k)  //计算多项式的系数C(nn,k)
	{
		int i;
		double sum = 1;
		for (i = 1; i <= nn; i++)
			sum *= i;
		for (i = 1; i <= k; i++)
			sum /= i;
		for (i = 1; i <= nn - k; i++)
			sum /= i;
		return sum;
	}
	template<class T2, class Td2>
	void sol3(double t, T2* path, size_t count, size_t n, std::vector<Td2>& opt)  //计算Bezier曲线上点的坐标
	{
		glm::dvec2 point = {};
		double Ber;
		for (size_t k = 0; k < count; k++)
		{
			Ber = sol2(n - 1, k) * pow(t, k) * pow(1 - t, n - 1 - k);
			point.x += path[k].x * Ber;
			point.y += path[k].y * Ber;
		}
		opt.push_back(point);
	}
	template<class T2, class Td2>
	void sol4(T2* path, size_t n, double m, bool first, std::vector<Td2>& opt)  //根据控制点，求曲线上的m个点
	{
		for (size_t i = 0; i <= m; i++)
		{
			if (i == 0 && !first)
			{
				continue;
			}
			sol3((double)i / (double)m, path, 4, n, opt);
		}
	}

	template<class T2, class Td2>
	void get_bezier(T2* path, size_t n, double m, bool first, std::vector<Td2>& r)
	{
		sol4(path, n, m, first, r);
		return;
	}

}
//!bs


	/*
	*	贝塞尔曲线，m为细分数量，first=true就是新开的线
		3次bezier四个坐标一组，p0 p1 p2 p3
		2次bezier设置p1\p2同样数值
	*/
std::vector<glm::vec2> get_bezier(const glm::vec2* path, size_t n, double m, bool first)
{
	std::vector<glm::vec2> r = {};
	bs::sol4(path, n, m, first, r);
	return r;
}
std::vector<glm::vec2> get_bezier(const cubic_v* path, size_t n, double m)
{
	std::vector<glm::vec2> r = {};
	if (path && n > 0)
	{
		bs::sol4((glm::vec2*)path, n * 4, m, true, r);
	}
	return r;
}
std::vector<glm::dvec2> get_bezier64(const cubic_v* path, size_t n, double m)
{
	std::vector<glm::dvec2> r = {};
	if (path && n > 0)
	{
		bs::sol4((glm::vec2*)path, n * 4, m, true, r);
	}
	return r;
}
template<class T>
std::vector<T> get_bezier_t(const cubic_v* path, size_t n, double m)
{
	std::vector<T> r = {};
	if (path && n > 0)
	{
		bs::sol4((glm::vec2*)path, n * 4, m, true, r);
	}
	return r;
}


void c2to3(cubic_v& c)
{
	static double dv = 2.0 / 3.0;
	glm::vec2 c1, c2;
	auto p0 = c.p0;
	auto p1 = c.p1;
	auto p2 = c.p3;
	c1 = p1 - p0; c1 *= dv; c1 += p0;
	c2 = p1 - p2; c2 *= dv; c2 += p2;
	c.p1 = c1;
	c.p2 = c2;
}

void build_cubic(const cubic_v& shape, float percent, float m, std::vector<glm::vec2>& ctx)
{
	auto x1 = shape.p0.x;
	auto y1 = shape.p0.y;
	auto x2 = shape.p3.x;
	auto y2 = shape.p3.y;
	auto cpx1 = shape.p1.x;
	auto cpy1 = shape.p1.y;
	auto cpx2 = shape.p2.x;
	auto cpy2 = shape.p2.y;
	if (percent == 0) {
		return;
	}
	float out[8] = {};
	// ctx.push_back(shape.p0);

	if (cpx2 == 0 || cpy2 == 0) {
		if (percent < 1) {
			quadraticSubdivide(x1, cpx1, x2, percent, out);
			cpx1 = out[1];
			x2 = out[2];
			quadraticSubdivide(y1, cpy1, y2, percent, out);
			cpy1 = out[1];
			y2 = out[2];
		}
		glm::vec2 v[4] = {};
		v[0] = shape.p0;
		auto p0 = v[0];
		auto p2 = shape.p3;
		auto p1 = glm::vec2(cpx1, cpy1);
		glm::vec2 c1, c2;
		{
			static double dv = 2.0 / 3.0;
			c1 = p1 - p0; c1 *= dv; c1 += p0;
			c2 = p1 - p2; c2 *= dv; c2 += p2;
			v[1] = c1; v[2] = c2;
		}
		v[3] = shape.p3;
		v[3] = shape.p3;
		bs::get_bezier(v, 4, m, 0, ctx);
		//ctx.quadraticCurveTo(
		//    cpx1, cpy1,
		//    x2, y2
		//);
	}
	else {
		if (percent < 1) {
			cubicSubdivide(x1, cpx1, cpx2, x2, percent, out);
			cpx1 = out[1];
			cpx2 = out[2];
			x2 = out[3];
			cubicSubdivide(y1, cpy1, cpy2, y2, percent, out);
			cpy1 = out[1];
			cpy2 = out[2];
			y2 = out[3];
		}
		glm::vec2 v[4] = {};
		v[0] = shape.p0;
		v[1] = glm::vec2(cpx1, cpy1);
		v[2] = glm::vec2(cpx2, cpy2);
		v[3] = shape.p3;
		bs::get_bezier(v, 4, m, 0, ctx);
		//ctx.bezierCurveTo(
		//    cpx1, cpy1,
		//    cpx2, cpy2,
		//    x2, y2
		//);
	}
}
#ifndef path_v_h
#define path_v_h


namespace gp {

	struct ctan_t
	{
		glm::vec2 t[2];		// 切线
		glm::vec2 angle;	// 2切线点角弧度
		glm::vec3 center;	// 圆xy坐标，z半径
	};
	// 路径圆角信息
	struct path_node_t
	{
		glm::vec2* pos;		// 顶点坐标
		glm::vec2* angle;	// 2切线点角弧度
		glm::vec3* center;	// 圆心坐标xy/半径z，半径小于等于0则不圆角
		int* lengths;
		int n;
		int count;			// 顶点数量
		int cap;			// 分配的顶点数量空间
		int caplen;

	};

	struct fv_it
	{
		glm::vec2* pos = 0;		// 顶点坐标
		glm::vec2* angle = 0;	// 2切线点角弧度
		glm::vec3* center = 0;	// 圆心坐标xy/半径z 
		int* lengths = 0;
		int count = 0;
		fv_it() {}
		fv_it(path_node_t* p) {
			pos = p->pos;		// 顶点坐标
			angle = p->angle;	// 2切线点角弧度
			center = p->center;	// 圆心坐标xy/半径z 
			lengths = p->lengths;
			count = p->n;
		}
		void inc(int n)
		{
			pos += n;
			angle += n;
			center += n;
		}

	};


	// 点v的方位角
	//float atan2v(glm::vec2 v);
	// 正数向上取整，负数向下
	int floor2ceil(float a);
	// 从两个路径创建面
	int for_cvertex(fv_it& fv, fv_it& fv1, size_t num_segments, std::vector<glm::vec3>& tv, glm::vec2 z, bool pccw);
	// 判断两条内外线相交
	// todo 获取切线
	void do_tan(glm::vec2* pts, int nSize, float radius, float maxangle, path_node_t* pn, int first, bool ccw, float a);


	int floor2ceil(float a)
	{
		return a > 0 ? ceil(a) : floor(a);
	}
	glm::ivec2 fc2(const glm::vec2& a, float x)
	{
		return { floor2ceil(a.x * x), floor2ceil(a.y * x) };
	}
	glm::ivec4 fc4(const glm::vec4& a)
	{
		return { floor2ceil(a.x), floor2ceil(a.y), floor2ceil(a.z), floor2ceil(a.w) };
	}
	//返回角度{angle,dir}方向右、顺时针为负, 左、逆时针为正
	glm::vec2 angle_v3(glm::vec2 v[3])
	{
		glm::vec2& a = v[0], & b = v[1], & c = v[2];

		auto c0 = glm::normalize(a - b);
		auto c1 = glm::normalize(b - c);
		auto ak = glm::angle(c0, c1);
		auto ak1 = glm::degrees(ak);
		glm::vec3 axis = glm::normalize(glm::cross(glm::vec3(c0, 0), glm::vec3(c1, 0)));
		//float oriented_angle = glm::orientedAngle(glm::vec3(a, 0), glm::vec3(b, 0), axis);
		ak1 = floor2ceil(180.0 + ak1 * axis.z);
		return glm::vec2(ak1, floor2ceil(axis.z));
	}
	glm::vec2 angle_v3(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
	{
		auto c0 = glm::normalize(a - b);
		auto c1 = glm::normalize(b - c);
		auto ak = glm::angle(c0, c1);
		auto ak1 = glm::degrees(ak);
		glm::vec3 axis = glm::normalize(glm::cross(glm::vec3(c0, 0), glm::vec3(c1, 0)));
		//float oriented_angle = glm::orientedAngle(glm::vec3(a, 0), glm::vec3(b, 0), axis);
		ak1 = 180.0 + ak1 * axis.z;
		return glm::vec2(floor2ceil(ak1), floor2ceil(axis.z));
	}
	glm::vec2 angle_v3_r(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
	{
		auto c0 = glm::normalize(a - b);
		auto c1 = glm::normalize(b - c);
		auto ak = glm::angle(c0, c1);
		auto ak1 = glm::degrees(ak);
		glm::vec3 axis = glm::normalize(glm::cross(glm::vec3(c0, 0), glm::vec3(c1, 0)));
		//float oriented_angle = glm::orientedAngle(glm::vec3(a, 0), glm::vec3(b, 0), axis);
		ak1 = 180.0 + ak1 * axis.z;
		return glm::vec2(glm::radians((float)(ak1)), floor2ceil(axis.z));
	}

	//# 向量角度
	auto angle_v2(glm::vec2 v1, glm::vec2 v2) {
		return acos(glm::dot(v1, v2) / (length(v1) * length(v2)));
	}
	/*
	a/c = sin(A)
	a = sinA*c
	a/sinA = c
	b/c = cosA
	b/cosA = a
	ABC角，abc边长
	B |\
	  | \
	a |  \ c
	  |___\
	C   b   A
	atan2
	输入边长对角获取另外边长
	*/
	glm::vec2 get_st(float a, float A)
	{
		float c = a / sin(A * glm::pi<double>() / 180.0);
		float b = a / tan(A * glm::pi<double>() / 180.0);
		return { c,b };
	}
	glm::vec2 get_ct(float b, float A)
	{
		float c = b / cos(A * glm::pi<double>() / 180.0);
		float a = b * tan(A * glm::pi<double>() / 180.0);
		return { c,a };
	}
	glm::vec2 dotf(glm::vec2 v, double f)
	{
		v *= f;
		return v;
	}



	glm::vec2 get_circle(float a, float r)
	{
		return glm::vec2(r * cos(a), r * sin(a));
	}
	glm::vec3 get_circle(float a, const glm::vec3& c)
	{
		return glm::vec3(c.x + c.z * cos(a), c.y + c.z * sin(a), 0);
	}
	float atan2v(const glm::vec2& v)
	{
		return atan2(v.y, v.x);
	}

	// 输入三点，角点ptr2，输出圆
	glm::vec3 get_circle3(glm::vec2 pt1, glm::vec2 pt2, glm::vec2 pt3, float radius)
	{
		glm::vec3 r = {};
		auto vec2_1 = pt1 - pt2;
		auto vec2_3 = pt3 - pt2;
		if (!(radius > 0))
		{
			return r;
		}
		auto dt1 = std::min(glm::distance(pt1, pt2), glm::distance(pt3, pt2)) * 0.5;
		//计算两向量夹角的余弦值
		double dCos = glm::dot(vec2_1, vec2_3) / (glm::length(vec2_1) * glm::length(vec2_3));
		double dTheta = acos(dCos);
		//求角平分线
		auto vecBisector = glm::normalize(vec2_1) + glm::normalize(vec2_3);
		double dR = radius;
		double dDist = dR / sin(dTheta / 2.0);
		auto dv = dR / dDist;
		//圆心
		auto center = glm::vec2(pt2 + dotf(normalize(vecBisector), dDist));
		// 求出点到圆心的距离
		auto distance = glm::distance(pt2, center);
		// 判断是否符合要求 distance<=r 不符合则返回空
		if (!(distance <= dR || glm::isnan(center.x) || glm::isnan(center.y)))
		{
			r = { center, radius };
		}
		return r;
	}
	// 获取点p到圆的两条切线
	glm::vec4 gtan(const glm::vec2& P, const glm::vec3& c)
	{
		glm::vec2 C = { c.x,c.y }; float r = c.z;
		// 求出点到圆心的距离
		auto distance = glm::distance(P, C);
		// 判断是否符合要求 distance<=r 不符合则返回 否则进行运算
		if (distance <= r || r <= 0) {
			//printf("您输入的数值不在范围内!\n");
			return {};
		}
		auto pc = P - C;
		// 计算切线与点心连线的夹角
		auto angle = asin(r / distance);
		auto angle1 = atan2(pc.y, pc.x);// 点P的方位角
		auto angle2 = angle1 + angle;			// 切线点角度1
		auto angle3 = angle1 - angle;			// 切线点角度2
		glm::vec4 ret = { C + get_circle(angle2, r),C + get_circle(angle3, r) };
		return ret;
	}

	double angle_between_vectors(double ux, double uy, double vx, double vy)
	{
		double dot = ux * vx + uy * vy;
		double umag = sqrt(ux * ux + uy * uy);
		double vmag = sqrt(vx * vx + vy * vy);
		double c = dot / (umag * vmag);
		if (c > 1.0)
			c = 1.0;
		if (c < -1.0)
			c = -1.0;
		double a = acos(c);
		if (ux * vy - uy * vx < 0.0)
			a = -a;
		return a;
	}
	/*
	struct ctan_t
	{
		glm::vec2 t[2];		// 切线
		glm::vec2 angle;	// 2切线点角弧度
		glm::vec3 center;	// 圆xy坐标，z半径
	};
	*/
	// 输入三点，角点ptr2，输出圆心和切线点坐标
	ctan_t get_tan(glm::vec2 pt1, glm::vec2 pt2, glm::vec2 pt3, float radius)
	{
		ctan_t r = {};
		auto vec2_1 = pt1 - pt2;
		auto vec2_3 = pt3 - pt2;
		if (radius <= 0) { return r; }
		auto dt1 = std::min(glm::distance(pt1, pt2), glm::distance(pt3, pt2)) * 0.5;
		//计算两向量夹角的余弦值
		double dCos = glm::dot(vec2_1, vec2_3) / (glm::length(vec2_1) * glm::length(vec2_3));
		double dTheta = acos(dCos);
		//求角平分线
		auto vecBisector = glm::normalize(vec2_1) + glm::normalize(vec2_3);
		double dR = radius;
		int nc = 1;
		do {
			double dDist = dR / sin(dTheta / 2.0);
			auto dv = dR / dDist;
			//圆心
			auto center = glm::vec2(pt2 + dotf(normalize(vecBisector), dDist));
			// 求出点到圆心的距离
			auto distance = glm::distance(pt2, center);
			// 判断是否符合要求 distance<=r 不符合则返回 否则进行运算
			if (distance <= dR || glm::isnan(center.x) || glm::isnan(center.y)) { r = {}; break; }
			r.center = { center , dR };
			// 计算切线与点心连线的夹角
			auto angle = glm::radians(90.0) - asin(dR / distance);
			auto angle1 = atan2v(pt2 - center);	// 点P的方位角 
			r.angle = { angle1 + angle, angle1 - angle }; // 切线点角度1/2
			r.t[0] = center + get_circle(r.angle.x, dR); r.t[1] = center + get_circle(r.angle.y, dR);
			float dis[] = { glm::distance(r.t[0], pt2), glm::distance(r.t[1], pt2) };
			{
				auto t1 = std::min(dis[0], dis[1]);
				if (t1 > dt1)
				{
					dR = dt1 * dv;
					continue;
				}
			}
			break;
		} while (nc--);
		return r;
	}

	// 判断两条内外线相交
	// todo 获取切线
	void do_tan(glm::vec2* pts, int nSize, float radius, float maxangle, path_node_t* pn, int first, bool ccw, float a)
	{
		if (!pts || nSize < 2 || !pn || radius <= 0)
		{
			return;
		}
		if (maxangle > 180)
		{
			maxangle = 180;
		}
		if (maxangle <= 0)
		{
			maxangle = 180;
		}

		auto nn = nSize;
		int dn = 1;
		if (pts[0] == pts[nn - 1])
		{
			dn = 2;
			nn--;
		}
		int cw = ccw ? -1 : 1;
		for (int i = 0; i < nn; i++)
		{
			int x0 = i - 1, x1 = i + 1;
			if (x0 < 0)x0 = nSize - dn;
			glm::vec2 pt1 = pts[(x0)];
			glm::vec2 pt2 = pts[i];
			glm::vec2 pt3 = pts[x1];

			auto j = angle_v3(pt1, pt2, pt3);			// 计算三点角度
			auto v0 = glm::distance(pt1, pt2);
			auto v1 = glm::distance(pt2, pt3);
			auto ny = floor2ceil(j.y * cw);
			auto axx = j.x;
			bool ccw1 = ccw;
			float r = radius;
			if (ccw1)
			{
				if (a > 0 && axx < maxangle)
				{
					ny = -1; ccw1 = false; r = a;
				}
				else
				{
					axx = 360.0 - axx;
				}
			}
			ctan_t t = {};
			if (ny < 0 && axx < maxangle)
				t = get_tan(pt1, pt2, pt3, r);
			// 逆时针序的要翻转圆角位置
			if (ccw1)
				std::swap(t.angle.x, t.angle.y);
			if (glm::isnan(t.angle.x) || glm::isnan(t.angle.x))
			{
				t = {};
			}
			pn->angle[first + i] = t.angle;
			pn->center[first + i] = t.center;
		}
	}

	void getctan(glm::vec2* pts, size_t nSize, float radius, float maxangle, int ccw, glm::vec2* angle, glm::vec3* center)
	{
		auto nn = nSize;
		int dn = 1;
		if (pts[0] == pts[nn - 1])
		{
			dn = 2;
			nn--;
		}
		int cw = ccw ? -1 : 1;
		if (radius > 0)
		{
			for (int i = 0; i < nn; i++)
			{
				int x0 = i - 1, x1 = i + 1;
				if (x0 < 0)x0 = nSize - dn;
				glm::vec2 pt1 = pts[(x0)];
				glm::vec2 pt2 = pts[i];
				glm::vec2 pt3 = pts[x1];
				auto j = angle_v3(pt1, pt2, pt3);			// 计算三点角度
				auto v0 = glm::distance(pt1, pt2);
				auto v1 = glm::distance(pt2, pt3);
				auto ny = floor2ceil(j.y * cw);
				if (ny > 0 && ccw == -1) {
					ny = -1;
				}
				auto axx = j.x;
				bool ccw1 = ccw && axx > 180;
				float r = radius;
				if (ccw1)
				{
					axx = 360.0 - axx;
				}
				ctan_t t = {};
				if (ny < 0 && axx < maxangle)
					t = get_tan(pt1, pt2, pt3, r);
				// 逆时针序的要翻转圆角位置
				if (ccw1)
					std::swap(t.angle.x, t.angle.y);
				if (glm::isnan(t.angle.x) || glm::isnan(t.angle.x))
				{
					t = {};
				}
				angle[i] = t.angle;
				center[i] = t.center;
			}
		}
		else {
			ctan_t t = {};
			for (int i = 0; i < nn; i++)
			{
				angle[i] = t.angle;
				center[i] = t.center;
			}
		}
		return;
	}
	struct path_node_tafdsf
	{
		glm::vec2* pos;		// 顶点坐标
		glm::vec2* angle;	// 2切线点角弧度
		glm::vec3* center;	// 圆心坐标xy/半径z，半径小于等于0则不圆角
		int* lengths;
		int n;
		int count;			// 顶点数量
		int cap;			// 分配的顶点数量空间
		int caplen;

	};


	void free_path_node(path_node_t* t)
	{
		if (t)
		{
			auto p = (glm::vec4*)t;
			delete[] p;
		}
	}
	path_node_t* new_node0(path_v* sp, size_t sn)
	{
		path_node_t* p = 0;
		int ret = 0;
		do
		{
			if (!sp || sp->size() < 2 /*|| sp->n < 1 || !sp->s || !sp->lengths*/)break;
			int64_t ac = sizeof(path_node_t);

			ac += sn * sizeof(glm::vec2);

			std::vector<int> ls;
			int64_t x = -1;
			size_t spn = 0;
			for (auto& it : sp->_data)
			{
				if (it.type == path_v::vtype_e::e_vmove)
				{
					spn++;
					ls.push_back(1);
					x++;
				}
				else {
					ls[x]++;
				}
			}
			ac += spn * sizeof(int);

			auto acount = ac / sizeof(glm::vec4);
			acount++;
			auto mem0 = new glm::vec4[acount];
			p = (path_node_t*)mem0;
			auto cp = (char*)mem0;
			cp += sizeof(path_node_t);
			//cp += sn * sizeof(glm::vec2);
			p->lengths = (int*)cp;
			cp += spn * sizeof(int);
			p->pos = (glm::vec2*)cp;
			p->cap = sn;
			p->count = sp->size();	// 顶点数量
			for (size_t i = 0; i < spn; i++)
			{
				p->lengths[i] = ls[i];
			}
			p->caplen = spn;
			auto pt = p->pos;		// 顶点坐标
			pt += p->count;
			//p->angle = pt;			// 2切线点角弧度
			pt += p->count;
			//p->center = (glm::vec3*)pt;				// 圆心坐标xy/半径z，半径小于等于0则不圆角 
		} while (0);

		return p;
	}

	// 删除前面的
	void push_forward(std::vector<glm::vec2>& nv, const glm::vec2& p0, double ds)
	{
		if (nv.size() == 0 && (ds > 0))
		{
			size_t n = 0;
			for (auto it = nv.rbegin(); it != nv.rend(); it++)
			{
				if (glm::distance(p0, *it) > ds)
				{
					break;
				}
				n++;
			}
			if (n > 0)
			{
				nv.resize(nv.size() - n);
			}
		}
	}
	// 删除插入开头的
	void push_dv2(std::vector<glm::vec2>& nv, std::vector<glm::vec2>& points, const glm::vec2& ds)
	{
		size_t idx = 0, idx1 = 0;
		auto pts = points.size();
		auto pt = points.data();
		double lds = 0.0;
		if ((ds.x > 0 || ds.y > 0) && pts > 2) {
			for (size_t i = 1; i < points.size(); i++)
			{
				auto t0 = pt[i - 1];
				auto t = pt[i];
				lds += glm::distance(t, t0);
				if (lds > ds.x)
				{
					idx = i; lds = 0.0;
					break;
				}
			}
			lds = 0.0;
			for (size_t i = points.size() - 1; i > idx; i--)
			{
				auto t0 = pt[i - 1];
				auto t = pt[i];
				lds += glm::distance(t, t0);
				if (lds > ds.y)
				{
					idx1 = i; lds = 0.0;
					break;
				}
			}
			if (idx != 0 && (nv.empty() || pt[0] != *nv.rbegin()))
				nv.push_back(pt[0]);
			if (idx < points.size() && idx1 < points.size() && idx < idx1)
			{
				for (size_t i = idx; i < idx1; i++)
				{
					nv.push_back(pt[i]);
				}
			}
			if (idx1 < points.size() - 1)
				nv.push_back(*points.rbegin());
		}
		else {
			if (nv.size() && pt[0] == *nv.rbegin())
			{
				pt++; pts--;
			}
			for (size_t i = 0; i < pts; i++)
			{
				nv.push_back(pt[i]);
			}
		}


	}
	// todo
	path_node_t* new_path_node(path_v* sp, float expand, float radius, float less_angle, int ccw, size_t num_segments, float ml, float ds)
	{
		float maxangle = less_angle;
		if (!sp || sp->size() < 2)
		{
			return 0;
		}
		auto td = sp->data();
		float exp = expand;
		std::vector<glm::vec2> posv, anglev;
		std::vector<glm::vec3> centerv;
		std::vector<int> lengths;
		auto spn = sp->mcount();
		std::vector<std::vector<glm::vec2>> tvs;
		sp->get_expand_flatten2(0, expand, num_segments, ml, ds, 2, &tvs, 0);
		auto length = tvs.size();
		// 细分直线
		std::vector<glm::vec2> ttd, ttd1;
		if (ml > 0 && ds > 0) {
			for (auto& it : tvs) {
				auto ss = it.size();
				auto td = it.data();
				ttd.reserve(ss);
				ttd.clear();
				for (size_t i = 1; i < ss; i++)
				{
					ttd1.clear();
					auto pt0 = td[i - 1];
					auto pt1 = td[i];
					auto dc = glm::distance(pt0, pt1);
					int dc0 = dc / ml;
					if (dc0 > 0)
					{
						for (size_t x = 0; x < dc0; x++)
						{
							ttd1.push_back(glm::mix(pt0, pt1, (double)x / dc0));
						}
					}
					ttd1.push_back(pt1);
					glm::vec2 dist = { ds * 3,ds * 3 };
					push_dv2(ttd, ttd1, dist);
				}
				it.swap(ttd);

			}
		}
		for (size_t i = 0; i < length; i++)
		{
			auto& tv = tvs[i];
			auto tvs = tv.size();
			auto pc = posv.size();
			posv.resize(pc + tvs);
			anglev.resize(pc + tvs);
			centerv.resize(pc + tvs);
			memcpy(posv.data() + pc, tv.data(), sizeof(glm::vec2) * tvs);
			// 圆角
			glm::vec2* angle = anglev.data() + pc;
			glm::vec3* center = centerv.data() + pc;
			getctan(tv.data(), tv.size(), radius, maxangle, ccw, angle, center);
			lengths.push_back(tv.size());
		}

		size_t act = posv.size() * 4;
		path_node_t* pn = new_node0(sp, act);

		pn->n = length;
		pn->count = posv.size();
		memcpy(pn->pos, posv.data(), sizeof(glm::vec2) * posv.size());

		auto pa = pn->pos + posv.size();
		auto pc = pa + anglev.size();
		pn->angle = pa;
		pn->center = (glm::vec3*)pc;
		memcpy(pn->angle, anglev.data(), sizeof(glm::vec2) * anglev.size());
		memcpy(pn->center, centerv.data(), sizeof(glm::vec3) * centerv.size());
		memcpy(pn->lengths, lengths.data(), sizeof(int) * lengths.size());
		return pn;
	}

	path_node_t* new_path_node_exp(path_v* sp, float expand, float scale, float radius, float less_angle, int type, bool rccw, size_t num_segments, float ml, float ds, bool is_round)
	{
		float maxangle = less_angle;
		if (!sp || sp->size() < 2)
		{
			return 0;
		}
		auto td = sp->data();
		float exp = expand;
		std::vector<glm::vec2> posv, anglev;
		std::vector<glm::vec3> centerv;
		std::vector<int> lengths;
		auto spn = sp->mcount();
		std::vector<std::vector<glm::vec2>> _tvs;
		sp->get_expand_flatten2(expand, scale, num_segments, ml, ds, type, &_tvs, is_round);
		auto length = _tvs.size();
		for (size_t i = 0; i < length; i++)
		{
			auto& tv = _tvs[i];
			auto tvs = tv.size();
			if (tvs < 2)continue;
			auto pc = posv.size();
			posv.resize(pc + tvs);
			anglev.resize(pc + tvs);
			centerv.resize(pc + tvs);
			memcpy(posv.data() + pc, tv.data(), sizeof(glm::vec2) * tvs);
			// 圆角
			glm::vec2* angle = anglev.data() + pc;
			glm::vec3* center = centerv.data() + pc;
			getctan(tv.data(), tv.size(), radius, maxangle, rccw, angle, center);
			lengths.push_back(tv.size());
		}

		size_t act = posv.size() * 4;
		path_node_t* pn = new_node0(sp, act);

		pn->n = length;
		pn->count = posv.size();
		memcpy(pn->pos, posv.data(), sizeof(glm::vec2) * posv.size());

		auto pa = pn->pos + posv.size();
		auto pc = pa + anglev.size();
		pn->angle = pa;
		pn->center = (glm::vec3*)pc;
		memcpy(pn->angle, anglev.data(), sizeof(glm::vec2) * anglev.size());
		memcpy(pn->center, centerv.data(), sizeof(glm::vec3) * centerv.size());
		memcpy(pn->lengths, lengths.data(), sizeof(int) * lengths.size());
		return pn;
	}



	template<class T, class T1>
	auto getk(T& m, const T1& k)
	{
		auto it = m.find(k);
		return it != m.end() ? it->second : nullptr;
	}
	void mkivec(std::vector<glm::vec3>& tv1, int d);
	void mkivec(std::vector<glm::vec2>& tv1, int d);
	// 获取圆角顶点
	int for_vertex(glm::vec2* pts, glm::vec2* angle, glm::vec3* center, int nSize, size_t num_segments, std::vector<glm::vec2>& tv)
	{
		auto nn = nSize;
		int dn = 1;
		if (pts[0] == pts[nn - 1])
		{
			dn = 2;
			nn--;
		}
		//tv.reserve(nn * cn); 
		for (int i = 0; i < nn; i++)
		{
			auto ps = pts[i];
			auto a = angle[i];
			auto c = center[i];
			if (c.z > 0)
			{
				auto cn = num_segments;
				auto an = glm::normalize(a);
				auto al = glm::length(a);
				auto p1 = get_circle(a.x, c);
				auto p2 = get_circle(a.y, c);
				double k = glm::distance(a.x, a.y);
				auto kd = glm::degrees(k);
				if (cn == 0)
				{
					cn = kd;
				}
				tv.push_back(p1);
				for (int x = 1; x < cn; x++)
				{
					auto d = (double)x / cn;
					auto a1 = glm::mix(an.x, an.y, d) * al;
					auto ps1 = get_circle(a1, c);
					tv.push_back(ps1);
				}
				//auto kka = angle_v3(tv1.data());
				tv.push_back(p2);
			}
			else {
				tv.push_back(ps);
			}
		}

		//mkivec(tv, 100);
		if (tv.size() && tv[0] != tv[tv.size() - 1])
		{
			tv.push_back(tv[0]);
		}
		return nSize;
	}
	// 坐标，角度，圆，段数，高度，输出圆角点或单点
	int get_fv(glm::vec2 ps, glm::vec2 a, glm::vec3 c, size_t num_segments, float z, std::vector<glm::vec3>& tv)
	{
		if (c.z > 0)
		{
			auto cn = num_segments;
			auto an = glm::normalize(a);
			auto al = glm::length(a);
			auto p1 = get_circle(a.x, c);
			auto p2 = get_circle(a.y, c);
			if (cn == 0)
			{
				double k = glm::distance(a.x, a.y);
				cn = glm::degrees(k);//如不指定段数则用弧度转换为度
			}
			p1.z = z;
			tv.push_back(p1);
			for (int x = 1; x < cn; x++)
			{
				auto d = (double)x / cn;
				auto a1 = glm::mix(an.x, an.y, d) * al;
				auto ps1 = get_circle(a1, c);
				ps1.z = z;
				tv.push_back(ps1);
			}
			p2.z = z;
			tv.push_back(p2);
		}
		else {
			tv.push_back({ ps, z });
		}
		return 0;
	}

	// 计算法向量
	glm::vec3 normal_v3(glm::vec3* dv)
	{
		glm::vec3 a = dv[0], b = dv[1], c = dv[2];
		auto ba = b - a;
		auto ca = c - a;
		auto cr = cross(ba, ca);
		auto nv = normalize(cr);

		if (glm::isnan(nv.x) || glm::isnan(nv.y) || glm::isnan(nv.z))
		{
			printf((char*)u8"error 错误的三角形\n");
			//assert(0);
		}
		return nv;
	}
	void get3(std::vector<glm::vec3>* v, std::vector<glm::vec3>* v1, std::vector<glm::vec3>& tv)
	{
		auto& s = v[0];
		auto& s1 = v[1];
		auto& t = v1[0];
		auto& t1 = v1[1];
		glm::ivec4 n = { s.size(),s1.size(),t.size(),t1.size() };
		int kc = 0;
		// 圆角三角形
		if (n.x > 1)
		{
			if (n.z > 1)
			{
				int cn = n.x - 1;
				for (size_t i = 0; i < cn; i++)
				{
					glm::vec3 tr[3] = { s[i],t[i],t[i + 1] };
					glm::vec3 tr1[3] = { s[i + 1] ,s[i], t[i + 1] };
					auto nv = normal_v3(tr);
					auto nv1 = normal_v3(tr1);
					tv.push_back(s[i]);
					tv.push_back(t[i]);
					tv.push_back(t[i + 1]);

					tv.push_back(s[i + 1]);
					tv.push_back(s[i]);
					tv.push_back(t[i + 1]);
				}
			}
			else
			{
				auto d = t[0];
				int cn = n.x - 1;
				for (size_t i = 0; i < cn; i++)
				{
					glm::vec3 tr[3] = { s[i],d,s[i + 1] };
					auto nv = normal_v3(tr);
					tv.push_back(tr[0]);
					tv.push_back(tr[1]);
					tv.push_back(tr[2]);
				}
			}
		}
		else {
			if (n.z > 1)
			{
				auto d = s[0];
				int cn = n.z - 1;
				for (size_t i = 0; i < cn; i++)
				{
					glm::vec3 tr[3] = { t[i],d,t[i + 1] };
					glm::vec3 tr1[3] = { t[i + 1],d,t[i] };
					auto nv = normal_v3(tr);
					auto nv1 = normal_v3(tr1);
					tv.push_back(tr[2]);
					tv.push_back(tr[1]);
					tv.push_back(tr[0]);
				}
			}
		}
		// 单点三角形2个
		{
			glm::vec3 tr[3] = { s[n.x - 1] ,t[n.z - 1] ,s1[0] };
			glm::vec3 tr1[3] = { s1[0],t[n.z - 1],t1[0] };
			tv.push_back(s[n.x - 1]);
			tv.push_back(t[n.z - 1]);
			tv.push_back(s1[0]);

			tv.push_back(s1[0]);
			tv.push_back(t[n.z - 1]);
			tv.push_back(t1[0]);
		}
	}

	// 旧函数
	int get_fv(fv_it& fv, int i, size_t num_segments, float z, std::vector<glm::vec3>& tv)
	{
		auto ps = fv.pos[i];
		auto a = fv.angle[i];
		auto c = fv.center[i];
		return get_fv(ps, a, c, num_segments, z, tv);
	}
	path_v pd2pv(PathsD* p)
	{
		path_v r;
		for (auto& it : *p)
		{
			r.moveTo(it[0].x, it[0].y);
			for (size_t i = 1; i < it.size(); i++)
			{
				r.lineTo(it[i].x, it[i].y);
			}
			r.lineTo(it[0].x, it[0].y);
		}
		return r;
	}
	PathsD fv2pathsd(fv_it* p, int num_segments)
	{
		PathsD r;
		auto fv = *p;
		float z = 0;
		for (size_t x = 0; x < p->count; x++)
		{
			auto nn = *fv.lengths;
			std::vector<glm::vec3> tv1;
			PathD v0;
			for (int i = 0; i < nn; i++)
			{
				tv1.clear();
				get_fv(fv, i, num_segments, z, tv1);
				for (auto& it : tv1)
				{
					v0.push_back({ it.x,it.y });
				}
			}
			r.push_back(v0);
			fv.inc(*fv.lengths);
			fv.lengths++;
		}
		return r;
	}


	void mkivec(std::vector<glm::vec3>& tv1, int d)
	{
		std::vector<glm::vec3> tt;
		tt.reserve(tv1.size());
		tt.push_back(tv1[0]);
		glm::vec3 x = { d,d,d };
		glm::ivec3 last = tv1[0] * x;
		for (auto it : tv1)
		{
			glm::ivec3 xd = it * x;
			if (xd != last)
			{
				last = xd;
				tt.push_back(it);
			}
		}
		if (tt[0] != tt[tt.size() - 1])
		{
			tt.push_back(tt[0]);
		}
		tv1.swap(tt);

	}
	void mkivec(std::vector<glm::vec2>& tv1, int d)
	{
		std::vector<glm::vec2> tt;
		tt.reserve(tv1.size());
		tt.push_back(tv1[0]);
		glm::vec2 x = { d,d };
		glm::ivec2 last = tv1[0] * x;
		for (auto it : tv1)
		{
			glm::ivec2 xd = it * x;
			if (xd != last)
			{
				last = xd;
				tt.push_back(it);
			}
		}
		if (tt[0] != tt[tt.size() - 1])
		{
			tt.push_back(tt[0]);
		}
		tv1.swap(tt);

	}
	void mkivec_round(std::vector<glm::vec2>& tv1)
	{
		std::vector<glm::vec2> tt;
		tt.reserve(tv1.size());
		tt.push_back(tv1[0]);
		glm::ivec2 last = glm::round(tv1[0]);
		for (auto it : tv1)
		{
			glm::ivec2 xd = glm::round(it);
			if (xd != last)
			{
				last = xd;
				tt.push_back(it);
			}
		}
		if (tt[0] != tt[tt.size() - 1])
		{
			tt.push_back(tt[0]);
		}
		tv1.swap(tt);

	}
	void mkivec_round(std::vector<glm::vec3>& tv1)
	{
		std::vector<glm::vec3> tt;
		tt.reserve(tv1.size());
		tt.push_back(tv1[0]);
		glm::ivec3 last = glm::round(tv1[0]);
		for (auto it : tv1)
		{
			glm::ivec3 xd = glm::round(it);
			if (xd != last)
			{
				last = xd;
				tt.push_back(it);
			}
		}
		if (tt[0] != tt[tt.size() - 1])
		{
			tt.push_back(tt[0]);
		}
		tv1.swap(tt);

	}
	// 从两个路径创建面
	int for_cvertex_old(fv_it& fv, fv_it& fv1, size_t num_segments, std::vector<glm::vec3>& opt, glm::vec2 z)
	{
		auto nn = *fv.lengths;
		std::vector<glm::vec3> tv1[2], tv2[2], t[2];
		get_fv(fv, 0, num_segments, z.x, t[0]);
		get_fv(fv1, 0, num_segments, z.y, t[1]);
		tv1[0] = t[0];
		tv2[0] = t[1];
		nn--;
		std::vector<glm::vec3> tv;
		tv.reserve(tv.size() + fv.count * 3);

		//auto a2 = gs::area_ofRingSigned(tv2.data(), tv2.size());
		for (int i = 1; i < nn; i++)
		{
			glm::ivec2 n0, n1;
			n0[0] = tv1[0].size();
			n1[0] = tv2[0].size();
			tv1[1].clear();
			tv2[1].clear();
			get_fv(fv, i, num_segments, z.x, tv1[1]);
			get_fv(fv1, i, num_segments, z.y, tv2[1]);

			n0[1] = tv1[1].size();
			n1[1] = tv2[1].size();

			get3(tv1, tv2, tv);

			tv1[0].swap(tv1[1]);
			tv2[0].swap(tv2[1]);
		}
		tv1[1] = t[0];
		tv2[1] = t[1];
		get3(tv1, tv2, tv);

		//mkivec(tv, 100);
		opt.insert(opt.begin(), tv.begin(), tv.end());
		fv.inc(*fv.lengths);
		fv1.inc(*fv1.lengths);
		fv.lengths++;
		fv1.lengths++;
		return 0;
	}





#if 1

	namespace trianglulate_wall_detail {
		using Vec = glm::vec3;
		using Sc = float;
		class Ring {
			size_t idx = 0, nextidx = 1, startidx = 0, begin = 0, end = 0;

		public:
			explicit Ring(size_t from, size_t to) : begin(from), end(to) { init(begin); }

			size_t size() const { return end - begin; }
			std::pair<size_t, size_t> pos() const { return { idx, nextidx }; }
			bool is_lower() const { return idx < size(); }

			void inc()
			{
				if (nextidx != startidx) nextidx++;
				if (nextidx == end) nextidx = begin;
				idx++;
				if (idx == end) idx = begin;
			}

			void init(size_t pos)
			{
				startidx = begin + (pos - begin) % size();
				idx = startidx;
				nextidx = begin + (idx + 1 - begin) % size();
			}

			bool is_finished() const { return nextidx == idx; }
		};

		template<class Sc>
		static Sc sq_dst(const Vec& v1, const Vec& v2)
		{
			Vec v = v1 - v2;
			return v.x * v.x + v.y * v.y /*+ v.z() * v.z()*/;
		}

		template<class Sc>
		static Sc trscore(const Ring& onring,
			const Ring& offring,
			const std::vector<Vec>& pts)
		{
			Sc a = sq_dst<Sc>(pts[onring.pos().first], pts[offring.pos().first]);
			Sc b = sq_dst<Sc>(pts[onring.pos().second], pts[offring.pos().first]);
			return (std::abs(a) + std::abs(b)) / 2.;
		}

		template<class Sc>
		class Triangulator {
			const std::vector<Vec>* pts;
			Ring* onring, * offring;

			double calc_score() const
			{
				return trscore<Sc>(*onring, *offring, *pts);
			}

			void synchronize_rings()
			{
				Ring lring = *offring;
				auto minsc = trscore<Sc>(*onring, lring, *pts);
				size_t imin = lring.pos().first;

				lring.inc();

				while (!lring.is_finished()) {
					double score = trscore<Sc>(*onring, lring, *pts);
					if (score < minsc) { minsc = score; imin = lring.pos().first; }
					lring.inc();
				}

				offring->init(imin);
			}

			void emplace_indices(std::vector<glm::ivec3>& indices)
			{
				glm::ivec3 tr{ int(onring->pos().first), int(onring->pos().second),
						 int(offring->pos().first) };
				if (onring->is_lower()) std::swap(tr[0], tr[1]);
				indices.emplace_back(tr);
			}

		public:
			void run(std::vector<glm::ivec3>& indices)
			{
				synchronize_rings();

				double score = 0, prev_score = 0;
				while (!onring->is_finished() || !offring->is_finished()) {
					prev_score = score;
					if (onring->is_finished() || (score = calc_score()) > prev_score) {
						std::swap(onring, offring);
					}
					else {
						emplace_indices(indices);
						onring->inc();
					}
				}
			}

			explicit Triangulator(const std::vector<Vec>* points,
				Ring& lower,
				Ring& upper)
				: pts{ points }, onring{ &upper }, offring{ &lower }
			{
			}
		};

		void triangulate_wall(std::vector<Vec>& pts,
			std::vector<glm::ivec3>& ind,
			const std::vector<Vec>& lower,
			const std::vector<Vec>& upper,
			double                   lower_z_mm,
			double                   upper_z_mm)
		{
			using namespace trianglulate_wall_detail;

			if (upper.size() < 3 || lower.size() < 3) return;

			pts.reserve(lower.size() + upper.size());
			for (auto& p : lower)
				pts.emplace_back(Vec{ p.x,p.y, lower_z_mm });
			for (auto& p : upper)
				pts.emplace_back(Vec{ p.x,p.y, upper_z_mm });

			ind.reserve(2 * (lower.size() + upper.size()));

			Ring lring{ 0, lower.size() }, uring{ lower.size(), pts.size() };
			Triangulator<float> t{ &pts, lring, uring };
			t.run(ind);
		}
		void triangulate_wall(std::vector<Vec>& opts,
			const std::vector<Vec>& lower,
			const std::vector<Vec>& upper,
			double                   lower_z_mm,
			double                   upper_z_mm)
		{
			using namespace trianglulate_wall_detail;

			if (upper.size() < 3 || lower.size() < 3) return;
			std::vector<Vec> pts;
			std::vector<glm::ivec3> ind;
			pts.reserve(lower.size() + upper.size());
			for (auto& p : lower)
				pts.emplace_back(Vec{ p.x,p.y, lower_z_mm });
			for (auto& p : upper)
				pts.emplace_back(Vec{ p.x,p.y, upper_z_mm });

			ind.reserve(2 * (lower.size() + upper.size()));

			Ring lring{ 0, lower.size() }, uring{ lower.size(), pts.size() };
			Triangulator<float> t{ &pts, lring, uring };
			t.run(ind);
			opts.reserve(ind.size() * 3 + opts.size());
			for (auto& it : ind) {
				opts.push_back(pts[it.x]);
				opts.push_back(pts[it.y]);
				opts.push_back(pts[it.z]);
			}
		}

	} // namespace trianglulate_wall_detail

#endif // 1

	// 计算区域面积，负数则环是逆时针
	template<class T>
	double area_ofRingSigned(T* ring, size_t count)
	{
		std::size_t rlen = count;
		if (rlen < 3 || !ring) {
			return 0.0;
		}

		double sum = 0.0;
		/*
		 * Based on the Shoelace formula.
		 * http://en.wikipedia.org/wiki/Shoelace_formula
		 */
		double x0 = ring[0].x;
		for (std::size_t i = 1; i < rlen - 1; i++) {
			double x = ring[i].x - x0;
			double y1 = ring[i + 1].y;
			double y2 = ring[i - 1].y;
			sum += x * (y2 - y1);
		}
		return sum / 2.0;
	}
	// 判断是否为逆时针区域
	template<class T>
	bool isCCWArea(const std::vector<T>& ring)
	{
		return area_ofRingSigned(ring.data(), ring.size()) < 0;
	}
	// todo constrained_delaunay_triangulation
	int for_cvertex(fv_it& fv, fv_it& fv1, size_t num_segments, std::vector<glm::vec3>& tv, glm::vec2 z, bool pccw)
	{
		auto nn = *fv.lengths;
		tv.reserve(tv.size() + fv.count * 3);

		std::vector<glm::vec3> tv1, tv2;
		for (int i = 0; i < nn; i++)
		{
			get_fv(fv, i, num_segments, z.x, tv1);
		}
		auto nn1 = *fv1.lengths;
		for (int i = 0; i < nn1; i++)
		{
			get_fv(fv1, i, num_segments, z.y, tv2);
		}

		auto a1 = area_ofRingSigned(tv1.data(), tv1.size());
		auto a2 = area_ofRingSigned(tv2.data(), tv2.size());
		fv.inc(*fv.lengths);
		fv1.inc(*fv1.lengths);
		fv.lengths++;
		fv1.lengths++;
		return 0;
	}


	void copy_plane1(std::vector<glm::vec2>* p, float z, bool ccw, std::vector<glm::vec3>* opt)
	{
		if (opt)
		{
			auto n = p->size();
			auto ct = opt->size();
			opt->resize(ct + p->size());
			auto t = opt->data() + ct;
			auto t1 = p->data();
			for (size_t i = 0; i < n; i++)
			{
				*t = { *t1,z };
				t++; t1++;
			}
			if (ccw)
			{
				t = opt->data() + ct;
				for (size_t i = 2; i < n; i += 3)
				{
					std::swap(t[i], t[i - 1]);
				}
			}
		}
	}
	void copy_plane1(std::vector<glm::vec2>* p, bool ccw, std::vector<glm::vec2>* opt)
	{
		if (opt)
		{
			auto n = p->size();
			auto ct = opt->size();
			opt->resize(ct + p->size());
			auto t = opt->data() + ct;
			auto t1 = p->data();
			for (size_t i = 0; i < n; i++)
			{
				*t = *t1;
				t++; t1++;
			}
			if (ccw)
			{
				t = opt->data() + ct;
				for (size_t i = 2; i < n; i += 3)
				{
					std::swap(t[i], t[i - 1]);
				}
			}
		}
	}
	void copy_plane1(std::vector<glm::vec3>* p, float z, bool ccw, std::vector<glm::vec3>* opt)
	{
		if (opt)
		{
			auto n = p->size();
			auto ct = opt->size();
			opt->resize(ct + p->size());
			auto t = opt->data() + ct;
			auto t1 = p->data();
			for (size_t i = 0; i < n; i++)
			{
				*t = *t1; t->z = z;
				t++; t1++;
			}
			if (ccw)
			{
				t = opt->data() + ct;
				for (size_t i = 2; i < n; i += 3)
				{
					std::swap(t[i], t[i - 1]);
				}
			}
		}
	}
	// ccw=0,1,全部角度-1
	PathsD path_round(path_v* ptr, int ccw, float radius, int num_segments, int ml, int ds)
	{
		PathsD r;
		glm::vec3 k = { 0,radius, ccw };

		auto pt = gp::new_path_node(ptr, k.x, k.y, ptr->angle, ccw, num_segments, ml, ds);
		if (!pt)return r;
		fv_it fv = { pt };
		r = fv2pathsd(&fv, num_segments);
		gp::free_path_node(pt);
		return r;
	}
	gpv::PathsD path_round_1(path_v* ptr, int ccw, float radius, int num_segments, int ml, int ds)
	{
		PathsD r;
		glm::vec3 k = { 0,radius, ccw };
		auto pt = gp::new_path_node(ptr, k.x, k.y, ptr->angle, ccw, num_segments, ml, ds);
		if (pt);
		{
			fv_it fv = { pt };
			r = fv2pathsd(&fv, num_segments);
			gp::free_path_node(pt);
		}
		return *((gpv::PathsD*)&r);
	}


	//！gp
}
namespace gp {

	void p2tri(plane3_t* opt, std::vector<std::vector<glm::vec2>>& tr, float z, bool pccw)
	{
		using N = uint32_t;
		std::vector<N> indices = mapbox::earcut<N>(tr);
		auto length = indices.size();
		std::vector<glm::vec2> dt;
		dt.reserve(length);
		opt->reserve(opt->size() + length);
		for (auto& it : tr)
		{
			for (auto v : it)
				dt.push_back(v);
		}
		if (pccw)
		{
			for (size_t i = 0; i < length; i++)
			{
				int x = indices[i + 2];
				int x1 = indices[i + 1];
				int x2 = indices[i];
				opt->push_back({ dt[x] ,z });
				opt->push_back({ dt[x1] ,z });
				opt->push_back({ dt[x2] ,z });
				i += 2;
			}
		}
		else {
			for (size_t i = 0; i < length; i++)
			{
				int x = indices[i];
				int x1 = indices[i + 1];
				int x2 = indices[i + 2];
				opt->push_back({ dt[x] ,z });
				opt->push_back({ dt[x1] ,z });
				opt->push_back({ dt[x2] ,z });
				i += 2;
			}
		}

	}
	void p2tri_ind(mesh3_mt* opt, std::vector<std::vector<glm::vec2>>& tr, float z, bool pccw)
	{
		using N = uint32_t;
		std::vector<N> indices = mapbox::earcut<N>(tr);
		auto length = indices.size();
		std::vector<glm::vec2> dt;
		dt.reserve(length);
		auto pss = opt->vertices.size();
		opt->vertices.reserve(opt->vertices.size());
		for (auto& it : tr)
		{
			for (auto v : it)
			{
				dt.push_back(v);
				opt->vertices.push_back(glm::vec3(v, z));
			}
		}
		if (pccw)
		{
			for (size_t i = 0; i < length; i++)
			{
				glm::ivec3 x = { indices[i + 2] + pss, indices[i + 1] + pss, indices[i] + pss };
				opt->indices.push_back(x);
				i += 2;
			}
		}
		else {
			for (size_t i = 0; i < length; i++)
			{
				glm::ivec3 x = { indices[i] + pss, indices[i + 1] + pss, indices[i + 2] + pss };
				opt->indices.push_back(x);
				i += 2;
			}
		}

	}


	class polygon_ct
	{
	public:
		std::vector<PathsD*> _c;
	public:
		polygon_ct();
		~polygon_ct();
		void make(PathsD* src, bool isin = false);
		size_t triangulate(plane3_t* opt, float z, bool pccw);
		size_t triangulate(mesh3_mt* opt, float z, bool pccw);
	private:

	};

	polygon_ct::polygon_ct()
	{
	}

	polygon_ct::~polygon_ct()
	{
		for (auto it : _c)
		{
			if (it)
			{
				delete it;
			}
		}
		_c.clear();
	}

	bool in_polygon(const PathD& c, const PathD& src)
	{
		int outsideCnt = 0;
		for (auto& pt : c)
		{
			PointInPolygonResult result = PointInPolygon(pt, src);
			if (result == PointInPolygonResult::IsInside) --outsideCnt;
			else if (result == PointInPolygonResult::IsOutside) ++outsideCnt;
			if (outsideCnt > 1) { return false; }
			else if (outsideCnt < -1) break;
		}
		return true;
	}

	void polygon_ct::make(PathsD* src, bool isin)
	{
		std::map<int, std::vector<PathsD*>> mpd;
		std::vector<PathD> c1;
		for (auto& it : *src)
		{
			// 判断是逆时针
			bool ccw = IsPositive(it);
			int a = Area(it);
			if (a == 0)
			{
				continue;
			}
			if (isin)
			{
				if (a < 0)
				{
					auto p = new PathsD();
					p->push_back(std::move(it));
					mpd[a].push_back(p);
				}
				else
				{
					c1.push_back(std::move(it));
				}
			}
			else {
				if (a > 0)
				{
					auto p = new PathsD();
					p->push_back(std::move(it));
					mpd[a].push_back(p);
				}
				else
				{
					c1.push_back(std::move(it));
				}
			}
		}
		size_t xc = 0;
		for (auto& it : c1) {
			for (auto& [k, v] : mpd)
			{
				bool isb = false;
				for (auto ct : v)
				{
					if (in_polygon(it, ct->at(0)))//判断包含关系
					{
						ct->push_back(it); xc++; isb = true; break;
					}
				}
				if (isb)
					break;
			}
		}
		for (auto& [k, v] : mpd)
		{
			for (auto ct : v)
			{
				_c.push_back(ct);
			}
		}
	}

	size_t polygon_ct::triangulate(plane3_t* opt, float z, bool pccw)
	{
		auto pos = opt->size();
		{
			print_time a("earcut1");
			for (auto vt : _c)
			{
				std::vector<glm::vec2> nl;
				std::vector<std::vector<glm::vec2>> tr;
				std::vector<int> as, cs;
				for (auto& it : *vt)
				{
					nl.clear();
					// 判断是逆时针
					bool ccw = IsPositive(it);
					int a = Area(it);
					as.push_back(a);
					cs.push_back(ccw);
					for (auto& v : it)
					{
						nl.push_back({ v.x,v.y });
					}
					if (nl[0] != nl[nl.size() - 1])
						nl.push_back(nl[0]);
					tr.push_back(nl);
				}
				p2tri(opt, tr, z, pccw);
			}
		}
		return opt->size() - pos;
	}
	size_t polygon_ct::triangulate(mesh3_mt* opt, float z, bool pccw)
	{
		auto pos = opt->indices.size();
		{
			print_time a("earcut1");
			for (auto vt : _c)
			{
				std::vector<glm::vec2> nl;
				std::vector<std::vector<glm::vec2>> tr;
				std::vector<int> as, cs;
				for (auto& it : *vt)
				{
					nl.clear();
					// 判断是逆时针
					bool ccw = IsPositive(it);
					int a = Area(it);
					as.push_back(a);
					cs.push_back(ccw);
					for (auto& v : it)
					{
						nl.push_back({ v.x,v.y });
					}
					if (nl[0] != nl[nl.size() - 1])
						nl.push_back(nl[0]);
					tr.push_back(nl);
				}
				p2tri_ind(opt, tr, z, pccw);
			}
		}
		return opt->indices.size() - pos;
	}
	// 创建裁剪平面
	size_t cplane(cmd_plane_t* c, PathsD& subjects, PathsD* clips, PathsD* hole, float z, bool pccw, const glm::ivec2& rev, bool isin)
	{
		PathsD	solution = clips ? Difference(subjects, *clips, FillRule::NonZero, 6) : subjects;
		if (solution.empty())
		{
			solution = clips ? Difference(*clips, subjects, FillRule::NonZero, 6) : subjects;
		}
		if (hole && hole->size())
			solution = Difference(solution, *hole, FillRule::NonZero, 6);
		polygon_ct pct;
		pct.make(&solution, isin);
		return pct.triangulate(c->opt, z, pccw);
	}
	// 创建孔洞的竖面
	size_t mkhole_ext(PathsD& hole, cmd_plane_t* c, const glm::vec2& dz, bool pccw)
	{
		bool is_round = 0;
		size_t cn = 0;
		path_v pv;
		auto length = hole.size();
		if (length == 0)return cn;
		for (size_t i = 0; i < length; i++)
		{
			auto it = hole[i];
			pv.add_lines((glm::dvec2*)it.data(), it.size(), true);
		}
		auto pt1 = gp::new_path_node_exp(&pv, 0, 0, c->radius, c->radius_a, c->type, c->rccw.x, c->segments, c->segments_len, c->segments_len, is_round);
		if (!pt1)return cn;

		gp::fv_it fv = { pt1 };
		gp::fv_it fv1 = { pt1 };
		std::vector<glm::vec3> opt;
		for (size_t x = 0; x < fv.count; x++)
		{
			if (*fv.lengths == *fv1.lengths)
			{
				for_cvertex_old(fv, fv1, c->segments, opt, dz);
			}
		}
		cn = opt.size();
		if (pccw) {
			auto tv = opt.data();
			for (size_t i = 2; i < cn; i += 3)
			{
				std::swap(tv[i], tv[i - 1]);
			}
		}
		if (cn)
		{
			auto pos = c->opt->size();
			c->opt->resize(pos + cn);
			memcpy(c->opt->data() + pos, opt.data(), cn * sizeof(glm::vec3));
		}
		gp::free_path_node(pt1);
		return cn;
	}
	//创建竖面
	size_t mk_ext(gp::path_node_t* pt1, cmd_plane_t* c, const glm::vec2& dz, bool pccw)
	{
		size_t cn = 0;
		if (!pt1)return cn;
		gp::fv_it fv = { pt1 };
		gp::fv_it fv1 = { pt1 };
		std::vector<glm::vec3> opt;
		for (size_t x = 0; x < fv.count; x++)
		{
			if (*fv.lengths == *fv1.lengths)
			{
				for_cvertex_old(fv, fv1, c->segments, opt, dz);
			}
		}
		cn = opt.size();
		if (pccw) {
			auto tv = opt.data();
			for (size_t i = 2; i < cn; i += 3)
			{
				std::swap(tv[i], tv[i - 1]);
			}
		}
		if (cn)
		{
			auto pos = c->opt->size();
			c->opt->resize(pos + cn);
			memcpy(c->opt->data() + pos, opt.data(), cn * sizeof(glm::vec3));
		}
		return cn;
	}
	void D2s64(PathsD& s, Paths64& d) {
		if (s.size())
		{
			d.resize(s.size());
			size_t i = 0;
			for (auto& it : s) {
				auto& vt = d[i]; i++;
				vt.resize(it.size());
				auto n = it.size();
				for (size_t x = 0; x < n; x++)
				{
					vt[x] = { it[x].x,it[x].y };
				}
			}
		}
	}
	// 判断距离多边形
	bool nb_poly(std::vector<std::vector<glm::vec2>>& p, const glm::vec2& pt, float ds, size_t& nk) {
		int n = 0;
		nk = -1;
		for (auto& it : p)
		{
			auto length = it.size();
			for (size_t i = 1; i < length; i++)
			{
				glm::vec2 c = glm::mix(it[i - 1], it[i], 0.5);
				auto d = glm::distance(pt, c);
				if (d < ds)
				{
					n++;
				}
				if (nk > d)
					nk = d;
			}
		}
		return n == 0;

	}


	// 创建裁剪平面
	size_t cplane(cmd_plane_t* c, PathsD& subjects, PathsD* clips, PathsD* hole, float z, bool pccw, const glm::ivec2& rev, bool isin);
	int pt2paths(gp::path_node_t* pt, PathsD& pathd, int segments, bool isrec);

	// 单面，先扩展后比例,输出三角形
	int build_plane1(cmd_plane_t* c, float expand, float scale, float z)
	{
		bool is_round = 0;
		int r = 0;

		//return build_plane1hole(c, expand + scale, z, 0, {});
		auto pt = gp::new_path_node_exp(c->pv, expand, scale, c->radius, c->radius_a, c->type, c->rccw.x, c->segments, c->segments_len, c->dist, is_round);
		if (pt)
		{
#if 1
			PathsD subjects, subjects1;
			pt2paths(pt, subjects, c->segments, 0);
			r = cplane(c, subjects, 0, 0, z, c->pccw, {}, true);
#else
			std::vector<std::vector<glm::vec2>>  tr;
			std::vector<glm::vec2>  tv1;

			gp::fv_it fv = { pt };
			for (size_t i = 0; i < fv.count; i++)
			{
				auto n = fv.lengths[i];
				tv1.clear();
				gp::for_vertex(fv.pos, fv.angle, fv.center, n, c->segments, tv1);
				fv.inc(n);
				tr.push_back(tv1);

			}
			auto pos = c->opt->size();
			gs::constrained_delaunay_triangulation_v(&tr, *c->opt, c->rccw.x, c->pccw);
			auto ss = c->opt->size();
			auto t = c->opt->data() + pos;
			for (size_t i = pos; i < ss; i++)
			{
				t->z = z; t++;
			}
			r = c->opt->size() - pos;
#endif
			gp::free_path_node(pt);
		}
		return r;
	}
	// 双路径面，两个扩展值
	int build_plane2(cmd_plane_t* c, const glm::vec2& expand, const glm::vec2& z)
	{
		bool is_round = 0;
		int r = 0;
		auto pt = gp::new_path_node_exp(c->pv, expand.x, 0, c->radius, c->radius_a, c->type, c->rccw[0], c->segments, c->segments_len, c->dist, is_round);
		auto pt1 = pt;
		if (expand.x != expand.y || c->rccw.x != c->rccw.y)
			pt1 = gp::new_path_node_exp(c->pv, expand.y, 0, c->radius, c->radius_a, c->type, c->rccw[1], c->segments, c->segments_len, c->dist, is_round);
		if (pt)
		{
			auto pos = c->opt->size();
			std::vector<std::vector<glm::vec2>>  tr;
			std::vector<glm::vec3>  tv;
			gp::fv_it fv = { pt };
			gp::fv_it fv1 = { pt1 };
			tv.clear();
			auto n = tv.size();

			if (expand.x == expand.y)
			{
				for (size_t x = 0; x < fv.count; x++)
				{
					gp::for_cvertex_old(fv, fv1, c->segments, tv, z);
				}
			}
			else
			{
				//if (z.x == z.y)
				//{
				//	PathsD subjects, clips;
				//	pt2paths(pt, subjects, c->segments, 0);
				//	if (pt != pt1)
				//		pt2paths(pt1, clips, c->segments, 0);
				//	r = cplane(c, subjects, 0, 0, z.x, c->pccw, {}, false);
				//}
				//else
				{
					for (size_t x = 0; x < fv.count; x++)
					{
						gp::for_cvertex(fv, fv1, c->segments, tv, z, 0);
					}
				}
			}
			if (c->pccw)
			{
				auto ns1 = tv.size() - n;
				for (size_t i = 2; i < ns1; i += 3)
				{
					std::swap(tv[n + i], tv[n + i - 1]);
				}
			}
			c->opt->resize(pos + tv.size());
			memcpy(c->opt->data() + pos, tv.data(), tv.size() * sizeof(glm::vec3));
			gp::free_path_node(pt);
			if (pt != pt1)
				gp::free_path_node(pt1);
			r = c->opt->size() - pos;
		}
		return r;
	}

	int build_plane2(cmd_plane_t* c, const glm::vec2& expand, float z) {
		return build_plane2(c, expand, { z,z });
	}



	// 双路径面，先扩展路径，再比例扩展两条路径
	int build_plane3(cmd_plane_t* c, float scale, const glm::vec2& z) {
		return build_plane3(c, { scale,scale }, z);
	}
	int build_plane3(cmd_plane_t* c, const glm::vec2& scale, const glm::vec2& z)
	{
		bool is_round = 0;
		glm::vec2 expand = {};
		glm::vec2 sc = scale;
		if (c->is_expand)
		{
			std::swap(sc, expand);
		}
		int r = 0;
		auto pt = gp::new_path_node_exp(c->pv, expand.x, scale.x, c->radius, c->radius_a, c->type, c->rccw.x, c->segments, c->segments_len, c->dist, is_round);
		auto pt1 = pt;
		if (scale.x != scale.y || c->rccw.x != c->rccw.y)
			pt1 = gp::new_path_node_exp(c->pv, expand.y, scale.y, c->radius, c->radius_a, c->type, c->rccw.y, c->segments, c->segments_len, c->dist, is_round);
		if (pt)
		{
			auto pos = c->opt->size();
			std::vector<std::vector<glm::vec2>>  tr;
			std::vector<glm::vec3>  tv;
			gp::fv_it fv = { pt };
			gp::fv_it fv1 = { pt1 };
			tv.clear();
			auto n = tv.size();
			for (size_t x = 0; x < fv.count && x < fv1.count; x++)
			{
#if 1
				gp::for_cvertex_old(fv, fv1, c->segments, tv, z);
#else
				gp::for_cvertex_wall(fv, fv1, c->segments, tv, z);
#endif
			}
			if (c->pccw)
			{
				auto ns1 = tv.size() - n;
				for (size_t i = 2; i < ns1; i += 3)
				{
					std::swap(tv[n + i], tv[n + i - 1]);
				}
			}
			c->opt->resize(pos + tv.size());
			memcpy(c->opt->data() + pos, tv.data(), tv.size() * sizeof(glm::vec3));
			gp::free_path_node(pt);
			if (pt != pt1)
				gp::free_path_node(pt1);
			r = c->opt->size() - pos;
		}
		return r;
	}


	void mk_line3d(std::vector<glm::vec2>& tvs, std::vector<glm::vec2>& tvs1, std::vector<glm::vec3>* opt, const glm::vec2& z, bool rev)
	{
		std::vector<glm::vec3> tv1[2], tv2[2], t[2];
		int nn = tvs.size();
		for (size_t i = 0; i < 1; i++)
		{
			t[0].push_back({ tvs[i],z.x });
			t[1].push_back({ tvs1[i],z.y });
		}
		tv1[0] = t[0];
		tv2[0] = t[1];
		std::vector<glm::vec3> tv;
		tv.reserve(tv.size() + nn);
		//nn--;
		for (int i = 1; i < nn; i++)
		{
			glm::ivec2 n0, n1;
			n0[0] = tv1[0].size();
			n1[0] = tv2[0].size();
			tv1[1].clear();
			tv2[1].clear();

			tv1[1].push_back({ tvs[i],z.x });
			tv2[1].push_back({ tvs1[i],z.y });

			n0[1] = tv1[1].size();
			n1[1] = tv2[1].size();

			gp::get3(tv1, tv2, tv);

			tv1[0].swap(tv1[1]);
			tv2[0].swap(tv2[1]);
		}
		tv1[1] = t[0];
		tv2[1] = t[1];
		gp::get3(tv1, tv2, tv);
		if (rev) {
			auto ns1 = tv.size();
			for (size_t i = 2; i < ns1; i += 3)
			{
				std::swap(tv[i], tv[i - 1]);
			}
		}
		opt->insert(opt->begin(), tv.begin(), tv.end());
	}

	// 单线竖面
	int build_plane0(cmd_plane_t* c, const glm::vec2& scale, const glm::vec2& z)
	{
		std::vector<std::vector<glm::vec2>> tvs[2];
		c->pv->get_expand_flatten3(scale.x, 0, c->segments, c->segments_len, 1, (int)JoinType::Round, (int)EndType::Square, &tvs[0], 0);
		c->pv->get_expand_flatten3(scale.y, 0, c->segments, c->segments_len, 1, (int)JoinType::Round, (int)EndType::Square, &tvs[1], 0);
		auto cc = tvs[0].size();
		auto cc1 = tvs[1].size();
		for (size_t i = 0; i < cc; i++)
		{
			mk_line3d(tvs[0][i], tvs[1][i], c->opt, z, c->pccw); // 生成竖面
		}
		// 生成封闭面
		auto ccb = [c](std::vector<std::vector<glm::vec2>>& t, float z, bool pccw)
			{
				auto cc = t.size();
				PathsD subjects;
				for (size_t i = 0; i < cc; i++)
				{
					auto& it = t[i];
					PathD v0;
					for (size_t x = 0; x < it.size(); x++)
					{
						v0.push_back({ it[x].x,it[x].y });
					}
					subjects.push_back(v0);
				}
				cplane(c, subjects, 0, 0, z, pccw, {}, false);
			};
		ccb(tvs[0], z.x, c->pccw);	// 上封面
		ccb(tvs[1], z.y, !c->pccw);	// 下封面
		return 0;
	}


	int for_cvertex_one(gp::fv_it& fv, size_t num_segments, path_v& opt, float z)
	{
		auto nn = *fv.lengths;
		std::vector<glm::vec3> tv1[2], tv2[2], t;
		gp::get_fv(fv, 0, num_segments, z, t);
		//nn--;
		std::vector<glm::vec3> tv;
		tv.reserve(tv.size() + fv.count * 3);
		opt.moveTo(t[0]);
		for (size_t i = 1; i < t.size(); i++)
		{
			opt.lineTo(t[i]);
		}
		for (int x = 1; x < nn; x++)
		{
			t.clear();
			gp::get_fv(fv, x, num_segments, z, t);
			for (size_t i = 0; i < t.size(); i++)
			{
				opt.lineTo(t[i]);
			}
		}
		fv.inc(*fv.lengths);
		fv.lengths++;
		return 0;
	}

	pv_st build_plane3_v(cmd_plane_t* c, const glm::vec2& scale, const glm::vec2& z, const std::string& fn, void (*cb)(path_v* pv, int count, const std::string& fn, bool fy))
	{
		bool is_round = 0;
		glm::vec2 expand = {};
		glm::vec2 sc = scale;
		if (c->is_expand)
		{
			std::swap(sc, expand);
		}
		int r = 0;
		auto pt = gp::new_path_node_exp(c->pv, expand.x, scale.x, c->radius, c->radius_a, c->type, c->rccw.x, c->segments, c->segments_len, c->dist, is_round);
		auto pt1 = pt;
		if (scale.x != scale.y || c->rccw.x != c->rccw.y)
			pt1 = gp::new_path_node_exp(c->pv, expand.y, scale.y, c->radius, c->radius_a, c->type, c->rccw.y, c->segments, c->segments_len, c->dist, is_round);
		pv_st st = {};
		path_v* v = 0;
		if (pt)
		{
			st.count = pt1 != pt ? 2 : 1;
			v = new path_v[st.count];
			st.v = v;
			std::vector<std::vector<glm::vec2>>  tr;
			std::vector<glm::vec3>  tv;
			gp::fv_it fv = { pt };
			gp::fv_it fv1 = { pt1 };
			tv.clear();
			auto n = tv.size();
			std::vector<glm::vec3> t[2];
			for (size_t x = 0; x < fv.count; x++)
			{
				for_cvertex_one(fv, c->segments, *v, z.x);
			}
			v[0]._pos = c->pv->_pos;
			v[0]._baseline = c->pv->_baseline;
			if (st.count == 2)
			{
				v[1]._pos = c->pv->_pos;
				v[1]._baseline = c->pv->_baseline;
				for (size_t x = 0; x < fv1.count; x++)
				{
					for_cvertex_one(fv1, c->segments, v[1], z.y);
				}
			}
			if (cb)
			{
				cb(v, st.count, fn, c->pv->_baseline ? 1 : 0);
			}
		}
		return st;
	}
	void get_equ_xj(std::vector<glm::vec2>& cs, float k, std::vector<glm::vec2>& ot)
	{
		auto length = cs.size();
		std::vector<glm::vec3> ot1;
		std::vector<glm::vec2> ot2;
		std::list<glm::vec2> cs0, cs1;
		for (auto it : cs)
		{
			int ic = 0;
			std::vector<float> dv;
			for (size_t i = 0; i < length; i++)
			{
				auto o = cs[i];
				auto d = glm::distance(it, o);
				if (o != it)
				{
					dv.push_back(d);
					if (d < k)
					{
						ic++;
					}
				}
				else {
					dv.push_back(-999);
				}
			}
			if (ic > 0)
				ic = ic;
			ot1.push_back({ it,ic });
			if (ic == 0)
			{
				cs0.push_back(it);
				ot2.push_back(it);
			}
			else {
				cs1.push_back(it);
			}
		}

		for (auto it : cs1)
		{
			int cc = 0;
			for (auto vt : ot2)
			{
				auto d = glm::distance(it, vt);
				if (d < k)
				{
					cc++;
					break;
				}
			}
			if (cc == 0)
			{
				ot2.push_back(it);
			}
		}
		for (auto it : ot2)
		{
			ot.push_back(it);
		}
	}
	// 输入折线，获取平均点
	double get_equ_point(std::vector<glm::vec2>& lines, float width, float k, std::vector<glm::vec2>& ot, bool verify)
	{
		double r = 0.0, c = 0.0;
		auto length = lines.size();
		assert(width > 0.0);
		if (!(width > 0) || length < 2) { return r; }
		ot.reserve(ot.size() + length);
		glm::vec2 first = lines[0]; first -= width;
		auto vpos = ot.size();
		std::vector<glm::vec2> ot1;
		ot1.reserve(length);
		for (int i = 1; i < length; i++)
		{
			auto it = lines[i - 1];
			auto it1 = lines[i];
			auto d = it1 - it;
			if (d.x == 0 && d.y == 0)
			{
				continue;
			}
			float dt = glm::distance(it, it1); // 线段的长度  
			r += dt;
			for (size_t x = 0; x < dt; x += k)
			{
				float xx = x / dt;
				auto ps = glm::mix(it, it1, xx);
				float dt1 = glm::distance(ps, first); // 距离
				if (dt1 < width)
				{
					continue;
				}
				ot1.push_back(ps);
				first = ps;
			}
		}
		auto dd = glm::distance(ot1[0], ot1[ot1.size() - 1]);
		if ((dd - width) < k)
		{
			ot1.pop_back();
		}
		if (verify)
		{
			length = ot1.size();
			auto dt = ot1.data();
			std::vector<glm::vec2> ot2;
			if (vpos > 0)
			{
				for (size_t i = 0; i < length; i++)
				{
					auto o = dt[i];
					int ic = 0;
					for (size_t x = 0; x < vpos; x++)
					{
						auto it = dt[x];
						if ((glm::distance(it, o) - width) < k)
						{
							ic++;
							break;
						}
					}
					if (ic == 0)
						ot2.push_back(o);
				}
			}
			else {
				ot2.swap(ot1);
			}
			r = ot2.size();
			if (r > 0)
				ot.insert(ot.end(), ot2.begin(), ot2.end());
		}
		else {
			r = ot1.size();
			if (r > 0)
				ot.insert(ot.end(), ot1.begin(), ot1.end());
		}
		return r;
	}

	int pt2paths(gp::path_node_t* pt, PathsD& pathd, int segments, bool isrec)
	{
		std::vector<glm::vec2>  tv1;
		gp::fv_it fv = { pt };
		PathD tpd;
		int ic = 0;
		for (size_t i = 0; i < fv.count; i++)
		{
			auto n = fv.lengths[i];
			tpd.clear();
			tv1.clear();
			gp::for_vertex(fv.pos, fv.angle, fv.center, n, segments, tv1);
			fv.inc(n);
			tpd.reserve(tv1.size());
			{
				auto pd = tpd.data();
				if (isrec)
				{
					for (auto it = tv1.rbegin(); it != tv1.rend(); it++)
					{
						tpd.push_back({ it->x  ,it->y });
					}
				}
				else
				{
					for (auto& it : tv1)
					{
						tpd.push_back({ it.x,it.y });
					}
				}
			}
			pathd.push_back(tpd);
			ic += tv1.size();
		}
		return ic;
	}
	// 合并打孔
	PathsD Union2cs(PathsD& pt, std::vector<glm::vec2>* cshape, std::vector<glm::ivec2>* idx, std::vector<glm::vec3>* vpos, bool isunion = false)
	{
		PathsD subjects, sd;
		PathD shape;
		if (!idx || idx->size() < 1)return subjects;
		auto length = idx->size();
		auto d = cshape->data();
		auto dl = cshape->size();
		for (size_t i = 0; i < length; i++)
		{
			shape.clear();
			auto it = idx->at(i);
			for (size_t x = 0; x < it.y; x++)
			{
				if (x + it.x < dl)
				{
					auto dt = d[x + it.x];
					shape.push_back({ dt.x,dt.y });
				}
			}
			bool ccw1 = IsPositive(shape);
			if (!ccw1) {
				std::reverse(shape.begin(), shape.end());
			}
			sd.push_back(shape);
		}
		if (isunion) {
			for (auto& it : *vpos) {
				if (it.z < length)
					subjects.push_back(TranslatePath(sd[(int)it.z], it.x, it.y));
			}
			auto solution = Union(subjects, FillRule::NonZero, 6);
			if (solution.size())
			{
				pt.reserve(pt.size() + solution.size());
				for (auto it : solution) {
					pt.push_back(it);
				}
			}
		}
		else {
			for (auto& it : *vpos) {
				if (it.z < length)
					pt.push_back(TranslatePath(sd[(int)it.z], it.x, it.y));
			}
		}
		return subjects;
	}
	PathsD Union2Circles(PathsD& pt, std::vector<glm::vec2>& c, double r, int segments, bool isunion = false, std::vector<glm::vec2>* cshape = 0)
	{
		assert(r > 0);
		PathD shape = Clipper2Lib::Ellipse(PointD(0, 0), r, r, segments);
		bool ccw0 = IsPositive(shape);
		if (cshape && cshape->size() > 2)
		{
			shape.clear();
			for (auto it : *cshape) {
				shape.push_back({ it.x,it.y });
			}
		}
		bool ccw1 = IsPositive(shape);
		if (!ccw1) {
			std::reverse(shape.begin(), shape.end());
		}
		PathsD subjects;
		auto length = c.size();
		if (isunion) {
			for (size_t i = 0; i < length; i++)
			{
				auto it = c[i];
				subjects.push_back(TranslatePath(shape, it.x, it.y));
			}
			auto solution = Union(subjects, FillRule::NonZero, 6);
			if (solution.size())
			{
				pt.reserve(pt.size() + solution.size());
				for (auto it : solution) {
					pt.push_back(it);
				}
			}
		}
		else {
			for (size_t i = 0; i < length; i++)
			{
				auto it = c[i];
				auto dt = TranslatePath(shape, it.x, it.y);
				pt.push_back(dt);
			}
		}
		return subjects;
	}
#if 1

	// 获取中点
	void getcl(std::vector<glm::dvec2>& pv, std::vector<glm::dvec2>& v2, std::vector<glm::dvec4>& eg)
	{
		auto count = pv.size();
		assert(count > 2);
		if (count < 3)return;
		count--;
		auto polygon = pv.data();
		for (int i = 0; i < count; i++) {
			int next = (i == (count - 1) ? 0 : (i + 1));
			auto dn = polygon[next] + polygon[i];
			dn *= 0.5;
			v2.push_back(dn);
			eg.push_back({ polygon[i] ,polygon[next] });
		}
	}
	glm::vec2 get2lineIPoint(glm::vec4 lineParam1, glm::vec4 lineParam2)
	{
		//Vec4f :参数的前半部分给出的是直线的方向，而后半部分给出的是直线上的一点
		glm::vec2 result(-1, -1);

		double cos_theta = lineParam1[0];
		double sin_theta = lineParam1[1];
		double x = lineParam1[2];
		double y = lineParam1[3];
		double k = sin_theta / cos_theta;
		double b = y - k * x;

		cos_theta = lineParam2[0];
		sin_theta = lineParam2[1];
		x = lineParam2[2];
		y = lineParam2[3];
		double k1 = sin_theta / cos_theta;
		double b1 = y - k1 * x;

		result.x = (b1 - b) / (k - k1);
		result.y = k * result.x + b;

		return result;
	}
#define addt3(p,a,b,c) p->push_back(a),p->push_back(b),p->push_back(c)
	// 细分三角形
	void divide_triangle3(glm::vec2 a, glm::vec2 b, glm::vec2 c, std::vector<glm::vec2>* opt)
	{
		glm::vec2 v[3];
		{
			v[0] = a + b;
			v[1] = b + c;
			v[2] = c + a;
		}
		for (size_t i = 0; i < 3; i++)
		{
			v[i] *= 0.5;
		}

		addt3(opt, a, v[0], v[2]);
		addt3(opt, v[0], b, v[1]);
		addt3(opt, v[1], c, v[2]);
		addt3(opt, v[0], v[1], v[2]);
	}
	void divide_triangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, std::vector<glm::vec2>* opt)
	{
		glm::vec2 v[3];
		{
			v[0] = a + b;
			v[1] = b + c;
			v[2] = c + a;
		}
		for (size_t i = 0; i < 3; i++)
		{
			v[i] *= 0.5;
		}
		addt3(opt, v[0], v[1], v[2]);
	}
	struct line1_t
	{
		glm::vec2 p1, p2;
		line1_t* prev, * next;
		bool operator<(const line1_t& y)const {
			return this->p1 < y.p1 || (this->p1 == y.p1 && this->p2 < y.p2);
		}
	};

	int iterls(std::list<std::list<glm::vec2>>& ost, std::list<std::list<glm::vec2>>& ot)
	{
		int ret = 0;
		std::list<std::list<glm::vec2>> ot2;
		for (; ost.size();)
		{
			auto t1 = ost.front();
			ost.pop_front();
			for (; ost.size();)
			{
				auto it = std::move(ost.front());
				auto p1 = t1.front();
				auto p2 = t1.back();
				auto tp1 = it.front();
				auto tp2 = it.back();
				ost.pop_front();
				if (tp1 == p1)
				{
					it.pop_front();
					for (; it.size();)
					{
						auto vt = it.front();
						t1.push_front(vt);
						it.pop_front();
					}
					continue;
				}
				if (tp2 == p1)
				{
					it.pop_back();
					for (; it.size();)
					{
						auto vt = it.back();
						t1.push_front(vt);
						it.pop_back();
					}
					continue;
				}
				if (tp1 == p2)
				{
					it.pop_front();
					for (; it.size();)
					{
						auto vt = it.front();
						t1.push_back(vt);
						it.pop_front();
					}
					continue;
				}
				if (tp2 == p2)
				{
					it.pop_back();
					for (; it.size();)
					{
						auto vt = it.back();
						t1.push_back(vt);
						it.pop_back();
					}
					continue;
				}
				ot2.push_back(it);
				ret++;
			}
			ost.swap(ot2);
			ot.push_back(t1);
			ret--;
		}
		return ret;
	}
	void mkl2(std::list<line1_t*>& pls, std::list<std::list<glm::vec2>>& ot)
	{
		std::list<line1_t*> npt;
		std::list<glm::vec2> np2;
		std::list<std::list<glm::vec2>> ost, ot2;
		for (; pls.size();)
		{
			auto it = pls.front();
			pls.pop_front();
			np2.push_back(it->p1);
			np2.push_back(it->p2);
			for (; pls.size();)
			{
				auto it = pls.front();
				pls.pop_front();
				auto p1 = np2.front();
				auto p2 = np2.back();
				if (it->p1 == p1)
				{
					np2.push_front(it->p2);
					continue;
				}
				if (it->p2 == p1)
				{
					np2.push_front(it->p1);
					continue;
				}
				if (it->p1 == p2)
				{
					np2.push_back(it->p2);
					continue;
				}
				if (it->p2 == p2)
				{
					np2.push_back(it->p1);
					continue;
				}
				npt.push_back(it);
			}
			ost.push_back(np2);
			np2.clear();
			pls.swap(npt);
		}

		int xc = iterls(ost, ot);
		auto nc = ot.size();
		for (; ot.size() > 1 && xc > 0; )
		{
			ot.swap(ost);
			xc = iterls(ost, ot);
			if (nc == ot.size())
			{
				break;
			}
			nc = ot.size();
		}
	}
	std::vector<std::vector<glm::vec2>> line2one(std::vector<glm::vec2>& k)
	{
		std::vector<std::vector<glm::vec2>> r;
		std::vector<line1_t> rp;
		auto n = k.size();
		if (n == 0)
		{
			return r;
		}
		n /= 2;
		r.reserve(n);
		rp.resize(n);
		std::map<glm::vec2, int> mx;
		std::vector<float> dv;
		for (size_t i = 0; i < n; i++)
		{
			auto& it = rp[i];
			it.p1 = k[i * 2];
			it.p2 = k[i * 2 + 1];
			dv.push_back(glm::distance(it.p1, it.p2));
			mx[it.p1]++;
			mx[it.p2]++;
		}
		auto rps = rp;
		std::stable_sort(rps.begin(), rps.end());
		std::vector<std::vector<line1_t*>> nls;
		std::list<line1_t*> pls, npt;
		for (size_t i = 0; i < n - 1; i++)
		{
			auto p = &rp[i];
			pls.push_back(p);
		}
		std::list<std::list<glm::vec2>> ot;
		mkl2(pls, ot);
		std::vector<glm::vec2> r0;
		for (; ot.size();)
		{
			auto it = std::move(ot.front()); ot.pop_front();
			for (auto vt : it)
			{
				r0.push_back(vt);
			}
			if (r0.size())
			{
				r.push_back(r0);
				r0.clear();
			}
		}
		return r;
	}
#endif // 1

	// 计算延长线坐标
	glm::vec2 extend_line_segment(const glm::vec2& p1, const glm::vec2& p2, double extension_length)
	{
		// 计算p1指向p2的向量
		glm::vec2 v = { p2.x - p1.x, p2.y - p1.y };

		// 计算向量的模
		double magnitude = sqrt(v.x * v.x + v.y * v.y);

		// 单位化向量
		glm::vec2 unit_v = { static_cast<float>(v.x / magnitude), static_cast<float>(v.y / magnitude) };

		// 判断线段是否水平或垂直
		if (p1.x == p2.x) {  // 垂直线段
			glm::vec2 extended_point = { p2.x, static_cast<float>(p2.y + extension_length) };
			return extended_point;
		}
		else if (p1.y == p2.y) {  // 水平线段
			glm::vec2 extended_point = { static_cast<float>(p2.x + extension_length), p2.y };
			return extended_point;
		}
		else {  // 其他情况
			// 计算延伸后的点
			glm::vec2 extended_point = { static_cast<float>(p2.x + unit_v.x * extension_length), static_cast<float>(p2.y + unit_v.y * extension_length) };
			return extended_point;
		}
	}


#ifndef gemoc


	float AngleBetween(glm::vec2 vector1, glm::vec2 vector2)
	{
		float sin = vector1.x * vector2.y - vector2.x * vector1.y;
		float cos = vector1.x * vector2.x + vector1.y * vector2.y;

		return (float)abs(atan2(sin, cos));
	}
	float Magnitude(const glm::vec2& v) {
		return  (float)sqrt(v.x * v.x + v.x * v.x);
	}
	float Magnitude(float x, float y) {
		return  (float)sqrt(x * x + x * x);
	}
#define PI glm::pi<float>()
	/// <summary>
	/// 获得向量在直角坐标系的角度
	/// </summary>
	/// <param name="V1"></param>
	/// <returns></returns>
	float GetAngle_in_RCS(const glm::vec2& v)
	{
		float X = v.x, Y = v.y;
		float VP = 0;
		float GS = Magnitude({ X,Y });
		if (X > 0.0f && Y >= 0.0f)
		{
			float DA = 1 * X;
			float DL = Magnitude({ 0,1 });
			float An = (float)acos(DA / (DL * GS));
			VP = An / (float)PI * 180.0f;
		}
		else if (X <= 0.0f && Y > 0.0f)
		{
			float DA = 1 * Y;
			float DL = Magnitude({ 0,1 });
			float An = (float)acos(DA / (DL * GS));
			VP = 90.0f + An / (float)PI * 180.0f;
		}
		else if (X < 0.0f && Y <= 0.0f)
		{
			float DA = -1 * X;
			float DL = Magnitude(-1, 0);
			float An = (float)acos(DA / (DL * GS));
			VP = 180.0f + An / (float)PI * 180.0f;
		}
		else if (X >= 0.0f && Y < 0.0f)
		{
			float DA = -1 * Y;
			float DL = Magnitude(0, -1);
			float An = (float)acos(DA / (DL * GS));
			VP = 270.0f + An / (float)PI * 180.0f;
		}
		else if (X == 0.0f && Y == 0.0f)
		{
			VP = 0.0f;
		}
		return VP;
	}
	// An highlighted block
			/// <summary>
			/// 计算线段(M,P)与圆(C,R)的交点距离目标点(M)最近的点的距离
			/// </summary>
			/// <param name="P">线段另一顶点</param>
			/// <param name="C">圆心</param>
			/// <param name="R">圆半径</param>
			/// <param name="M">目标点</param>
			/// <returns></returns>
	float GetLineTOCirclePoint(glm::vec2 M, glm::vec2 P, glm::vec2 C, float R)
	{
		float _K;             //直线斜率
		float _B;             //直线截距
		float _A = C.x;       //圆心X坐标     (X-_A)^2+(Y-_C)^2=R^2
		float _C = C.y;       //圆心Y坐标
		float X1 = 0.0f;       //两点坐标
		float X2 = 0.0f;
		float Y1 = 0.0f;
		float Y2 = 0.0f;
		float Dis = glm::distance(glm::vec2(M.x, M.y), glm::vec2(P.x, P.y));       //获取视野距离

		if (P.y == M.y)             //情况一   斜率为0时也就是垂直Y轴
		{
			if (P.y > C.y + R || P.y < C.y - R) //如果没有交点则直接返回
				return Dis;
			else if (P.y == C.y + R || P.y == C.y - R)        //如果有一个交点
			{
				glm::vec2 m = glm::vec2(C.x, P.y);
				float d = glm::distance(M, m);
				if (AngleBetween(glm::vec2(m.x - M.x, m.y - M.y), glm::vec2(P.x - M.x, P.y - M.y)) < PI / 2)
					return d > Dis ? Dis : d;
			}
			else                            //有两个交点
			{
				Y1 = M.y;
				Y2 = M.y;
				X1 = _A + (float)sqrt(R * R - (M.y - _C) * (M.y - _C));
				X2 = _A - (float)sqrt(R * R - (M.y - _C) * (M.y - _C));
			}
		}
		else if (M.x == P.x)       //情况二   斜率不存在也就是垂直X轴
		{
			if (P.x > C.x + R || P.x < C.x - R) //如果没有交点则直接返回
				return Dis;
			else if (P.x == C.x + R || P.x == C.x - R)        //如果有一个交点
			{
				glm::vec2 m = glm::vec2(P.x, C.y);
				float d = glm::distance(M, m);
				if (AngleBetween(glm::vec2(m.x - M.x, m.y - M.y), glm::vec2(P.x - M.x, P.y - M.y)) < PI / 2)
					return d > Dis ? Dis : d;
			}
			else                            //有两个交点
			{
				X1 = M.x;
				X2 = M.x;
				Y1 = _C + (float)sqrt(R * R - (M.x - _A) * (M.x - _A));
				Y2 = _C - (float)sqrt(R * R - (M.x - _A) * (M.x - _A));
			}
		}
		else
		{
			//情况三   斜率存在不为0
			_K = (P.y - M.y) / (P.x - M.x); //计算斜率
			_B = M.y - (M.x * _K);            //由y=kx-x1k+y1计算得出 b=y1-x1*k

			float B11 = (2 * _K * (_B - _C) - 2 * _A);   //计算一元二次方程的b系数
			float A11 = (_K * _K + 1);                   //计算b
			float C11 = ((_B - _C) * (_B - _C) - R * R + _A * _A); //计算c
			if (B11 * B11 - 4 * A11 * C11 < 0)          //无解表示没有交点，所以距离为0
				return Dis;
			else if (B11 * B11 - 4 * A11 * C11 == 0)     //只有一个交点
			{
				float X = -(B11) / (2 * A11);         //韦达定理
				float Y = _K * X + _B;              //计算Y坐标
				glm::vec2 m = glm::vec2(X, Y);
				float d = glm::distance(M, m);
				if (AngleBetween(glm::vec2(m.x - M.x, m.y - M.y), glm::vec2(P.x - M.x, P.y - M.y)) < PI / 2)
					return d > Dis ? Dis : d;
			}
			else
			{
				//求根公式
				X1 = (-B11 + (float)sqrt(B11 * B11 - 4 * A11 * C11)) / (2 * A11);
				X2 = (-B11 - (float)sqrt(B11 * B11 - 4 * A11 * C11)) / (2 * A11);
				Y1 = _K * X1 + _B;
				Y2 = _K * X2 + _B;

			}
		}

		float Dis1 = glm::distance(glm::vec2(M.x, M.y), glm::vec2(X1, Y1));
		float Dis2 = glm::distance(glm::vec2(M.x, M.y), glm::vec2(X2, Y2));
		//如果两个点都在圆内
		if (glm::distance(glm::vec2(P.x, P.y), glm::vec2(C.x, C.y)) < R &&
			glm::distance(glm::vec2(M.x, M.y), glm::vec2(C.x, C.y)) < R)
		{
			return Dis;
		}
		else if (glm::distance(glm::vec2(P.x, P.y), glm::vec2(C.x, C.y)) < R &&
			glm::distance(glm::vec2(M.x, M.y), glm::vec2(C.x, C.y)) > R)
		{
			//当另一点在圆内，起始点在圆外
			return Dis1 > Dis2 ? Dis2 : Dis1;
		}
		else if (glm::distance(glm::vec2(P.x, P.y), glm::vec2(C.x, C.y)) > R &&
			glm::distance(glm::vec2(M.x, M.y), glm::vec2(C.x, C.y)) < R)
		{
			//当起始点在圆内，另一点在圆外
			glm::vec2 r1 = glm::vec2(X1 - M.x, Y1 - M.y);
			glm::vec2 r2 = glm::vec2(P.x - M.x, P.y - M.y);
			if (AngleBetween(r1, r2) < PI / 2)
			{
				//如果起始点到第一个交点的向量与起始点与另一点所构成的角度为锐角，则当前交点为最终交点
				return Dis1;
			}
			else
			{
				return Dis2;
			}

		}
		else if (glm::distance(glm::vec2(M.x, M.y), glm::vec2(C.x, C.y)) == R &&
			glm::distance(glm::vec2(P.x, P.y), glm::vec2(C.x, C.y)) != R)
		{
			//如果起始原点在圆上,另一点不再圆上
			glm::vec2 m = glm::vec2(P.x - M.x, P.y - M.y);
			float ARC = GetAngle_in_RCS(m);
			if (ARC >= 180 && ARC <= 360)
			{
				return Dis;
			}
			else
				return 0.0f;
		}
		else if (glm::distance(glm::vec2(M.x, M.y), glm::vec2(C.x, C.y)) != R &&
			glm::distance(glm::vec2(P.x, P.y), glm::vec2(C.x, C.y)) == R)
		{
			//如果另一原点在圆上,起始点不再圆上
			glm::vec2 r1 = glm::vec2(X1 - M.x, Y1 - M.y);
			glm::vec2 r2 = glm::vec2(P.x - M.x, P.y - M.y);
			if (AngleBetween(r1, r2) < PI / 2)
			{
				//如果起始点到第一个交点的向量与起始点与另一点所构成的角度为锐角，则当前交点为最终交点
				return Dis1;
			}
			else
			{
				return Dis2;
			}
		}
		else if (glm::distance(glm::vec2(M.x, M.y), glm::vec2(C.x, C.y)) == R &&
			glm::distance(glm::vec2(P.x, P.y), glm::vec2(C.x, C.y)) == R)
		{
			//如果两个点都在圆上,则距离为0
			return 0.0f;
		}
		else
		{
			//如果两点都在圆外
			//如果另一原点在圆上,起始点不再圆上
			glm::vec2 r1 = glm::vec2(X1 - M.x, Y1 - M.y);
			glm::vec2 r2 = glm::vec2(P.x - M.x, P.y - M.y);
			glm::vec2 r3 = glm::vec2(X2 - M.x, Y2 - M.y);
			float ARC1 = AngleBetween(r1, r2);
			float ARC2 = AngleBetween(r3, r2);
			if (ARC1 < PI / 2 && ARC1 > -PI / 2 && ARC2 > -PI / 2 &&
				ARC2 < PI / 2)
			{
				//如果起始点到第一个交点的向量与起始点与另一点所构成的角度为锐角，则当前交点为最终交点
				if (Dis1 > Dis2)
				{
					if (Dis2 > Dis)
					{
						return Dis;
					}
					else
					{
						return Dis2;
					}
				}
				else
				{
					if (Dis1 > Dis)
					{
						return Dis;
					}
					else
					{
						return Dis1;
					}
				}
			}
			else
			{
				return Dis;
			}
		}
	}
	/// <summary>
	/// 射线对线段检测
	/// </summary>
	/// <param name="M">起点坐标</param>
	/// <param name="P">端点坐标</param>
	/// <param name="V1">线段的一个坐标</param>
	/// <param name="V2">线段另一个坐标</param>
	template<class Vector, class VT>
	bool ABenCountCD(Vector M, Vector P, Vector V1, Vector V2, VT& dis)
	{
		VT Dis = glm::distance(M, P);
		//情况一   斜率为0
		if (P.y == M.y)
		{
			//如果另一条直线也是斜率为0，即两者相互平行
			if (V1.y == V2.y)
			{
				dis = Dis;
				return false;       //直接返回当前长度
			}
			else if (V1.x == V2.x)
			{
				VT X1 = V1.x;
				VT Y1 = M.y;
				VT Dis1 = glm::distance(M, Vector(X1, Y1));
				dis = Dis1 > Dis ? Dis : Dis1;
				return Dis1 < Dis;
			}
			else
			{
				//计算交点
				VT K = (V1.y - V2.y) / (V1.x - V2.x);        //计算V1 V2斜率
				VT X = V1.x + (1 / K) * (M.y - V1.y);       //x=x1+(1/k)(y-y1)
				VT Y = M.y;
				VT d1 = abs(X - V1.x);
				VT d2 = abs(X - V2.x);
				VT d3 = abs(V1.x - V2.x);
				//if(d1+d2==d3&&)
				if (!(((X > M.x) && (X > P.x)) || ((X < M.x) && (X < P.x))) &&
					(X != M.x) || (Y != M.y))   //如果交点在左边或在右边，则没有交点
				{
					auto _d1 = glm::distance(M, Vector(X, Y));
					dis = _d1;
					return _d1 < Dis;
				}
				else
				{
					dis = Dis;
					return false;
				}
			}

		}
		//情况二   斜率不存在
		else if (P.x == M.x)
		{
			//如果另一条直线也是斜率不存在，即两者相互平行
			if (V1.x == V2.x)
			{
				dis = Dis;
				return false;       //直接返回当前长度
			}
			else if (V1.y == V2.y)
			{
				VT X1 = P.x;
				VT Y1 = V1.y;
				VT Dis1 = glm::distance(M, Vector(X1, Y1));
				dis = Dis1;
				return Dis1 < Dis;
			}
			else
			{
				//计算交点
				VT K = (V1.y - V2.y) / (V1.x - V2.x);
				VT B = V1.y - K * V1.x;       //y=kx-kx1+y1
				VT X = M.x;
				VT Y = M.x * K + B;
				if (!(((Y > M.y) && (Y > P.y)) || ((Y < M.y) && (Y < P.y))) &&
					(X != M.x) || (Y != M.y))   //如果交点在上边或在下边，则没有交点
				{
					dis = glm::distance(M, Vector(X, Y));
					return dis < Dis;
				}
				else
				{
					dis = Dis;
					return false;
				}
			}

		}
		else {
			VT X;
			VT Y;
			VT K1 = (M.y - P.y) / (M.x - P.x);
			VT B1 = M.y - K1 * M.x;
			//如果另一条线段斜率不存在
			if (V1.x == V2.x)
			{
				X = V1.x;
				Y = K1 * X + B1;
			}
			else if (V1.y == V2.y)
			{
				X = (1 / K1) * (V1.y - B1);
				Y = V1.y;
			}
			else
			{
				VT K2 = (V1.y - V2.y) / (V1.x - V2.x);
				VT B2 = V1.y - K2 * V1.x;       //y=kx-kx1+y1
				//如果两个直线斜率相等，说明平行
				if (K1 == K2)
					return Dis;
				X = (B2 - B1) / (K1 - K2);
				Y = K1 * X + B1;
			}
			Vector G = Vector(X, Y);
			VT Dis1 = glm::distance(M, G);
			Vector v1 = Vector(P.x - M.x, P.y - M.y);
			Vector v2 = Vector(X - M.x, Y - M.y);
			VT arc = AngleBetween(v1, v2);
			if (arc > PI / 2 && G != M)     //如果交点与与射线方向相反
			{
				dis = Dis;
				return false;
			}
			else if (arc >= 0 && arc < PI / 2 && G != M)     //如果交点与与射线方向相同
			{
				///如果交点在四边形内
				if (!(((X > M.x) && (X > P.x)) || ((X < M.x) && (X < P.x))) &&
					!(((Y > M.y) && (Y > P.y)) || ((Y < M.y) && (Y < P.y))) &&
					!(((X > V1.x) && (X > V2.x)) || ((X < V1.x) && (X < V2.x))) &&
					!(((Y > V1.y) && (Y > V2.y)) || ((Y < V1.y) && (Y < V2.y))))
				{
					dis = Dis1;
					return dis < Dis;
				}
			}
		}
		dis = Dis;
		return false;
	}

	/// 计算线段(M,P)与圆(C,R)的交点距离目标点(M)最近的点的距离
	/// </summary>
	/// <param name="M">目标点</param>
	/// <param name="P">线段另一顶点</param>
	/// <param name="C">圆心</param>
	/// <param name="R">圆半径</param>
	float GetLineTOCirclePoint(glm::vec2 M, glm::vec2 P, glm::vec2 C, float R);
	/// 射线对线段检测
	/// </summary>
	/// <param name="M">起点坐标</param>
	/// <param name="P">端点坐标</param>
	/// <param name="V1">线段的一个坐标</param>
	/// <param name="V2">线段另一个坐标</param>
	float ABenCountCD(glm::vec2 M, glm::vec2 P, glm::vec2 V1, glm::vec2 V2)
	{
		float d;
		bool r = ABenCountCD(M, P, V1, V2, d);
		return d;
	}
	float ray_cast(const glm::vec4& m, const glm::vec3& c)
	{
		return GetLineTOCirclePoint({ m.x, m.y }, { m.z, m.w }, { c.x, c.y }, c.z);
	}
	float ray_cast(const glm::vec4& m, const glm::vec4& dst)
	{
		glm::vec2 m1 = { m.x,m.y }, m2 = { m.z,m.w };
		glm::vec2 v[] = { { dst.x, dst.y } ,{ dst.x + dst.z, dst.y },{ dst.x + dst.z, dst.y + dst.w },{ dst.x , dst.y + dst.w } };
		float d = ABenCountCD(m1, m2, v[0], v[1]);
		d = std::min(d, ABenCountCD(m1, m2, v[0], v[3]));
		d = std::min(d, ABenCountCD(m1, m2, v[1], v[2]));
		d = std::min(d, ABenCountCD(m1, m2, v[2], v[3]));
		return d;
	}
	bool ray_cast(const glm::vec4& m, const glm::vec2& v, const glm::vec2& v1, float& dis)
	{
		return ABenCountCD({ m.x,m.y }, { m.z,m.w }, v, v1, dis);
	}
	bool ray_cast(const glm::dvec4& m, const glm::dvec2& v, const glm::dvec2& v1, double& dis)
	{
		return ABenCountCD(glm::dvec2{ m.x,m.y }, glm::dvec2{ m.z,m.w }, v, v1, dis);
	}
	/*
	判断点是否在多边形内
	Crossing Number Method
	此算法基于一个定理：从给定点发出一条射线，如果此点在多边形内，那么这条射线和多边形的交点一定是奇数个，否则是偶数个。

	那么这里要解决的问题就是：

	如何判断射线和边相交
	判断射线和边相交很简单，我们先要做两个规定：

	顶点按照顺时针排列

	我们不要使用复杂的射线，直接用
	x=x0
	这条穿过P点的竖直向上的射线。

	首先要判断此边是否可能和直线相交：
		*/
	template<class V>
	bool Contains(V* vertices, int count, const V& p)
	{
		int crossing = 0;

		if (vertices && count > 0)
		{
			for (size_t i = 0; i < count; i++) {
				const V& v1 = vertices[i],
					v2 = vertices[((i + 1) % count)];
				double slope = (v2.y - v1.y) / (v2.x - v1.x);
				// 不计算slope，直接在above上计算
				bool above = (p.y - v1.y) * (v2.x - v1.x) < (p.x - v1.x) * (v2.y - v1.y);
				bool cond1 = v1.x <= p.x && p.x <= v2.x,
					cond2 = v2.x <= p.x && p.x <= v1.x
					/*,above = (p.y - v1.y) / (p.x - v1.x) < slope*/;

				if ((cond1 || cond2) && above) crossing++;
			}
		}
		auto n = crossing % 2;
		return n;
	}

	template <typename T>
	inline double CrossProduct(const T& pt1, const T& pt2, const T& pt3) {
		return (static_cast<double>(pt2.x - pt1.x) * static_cast<double>(pt3.y -
			pt2.y) - static_cast<double>(pt2.y - pt1.y) * static_cast<double>(pt3.x - pt2.x));
	}

	template <typename T>
	inline double CrossProduct(const T& vec1, const T& vec2)
	{
		return static_cast<double>(vec1.y * vec2.x) - static_cast<double>(vec2.y * vec1.x);
	}
#if 0
	// 边上，内部，外部
	enum class PointInPolygonResult { IsOn, IsInside, IsOutside };
	// 计算点和多边形的关系
	inline PointInPolygonResult PointInPolygon0(const glm::vec2& pt, const std::vector<glm::vec2>& polygon)
	{
		if (polygon.size() < 3)
			return PointInPolygonResult::IsOutside;

		int val = 0;
		auto cbegin = polygon.cbegin(), first = cbegin, curr = first, prev = first;
		auto cend = polygon.cend();

		while (first != cend && first->y == pt.y) ++first;
		if (first == cend) // not a proper polygon
			return PointInPolygonResult::IsOutside;

		bool is_above = first->y < pt.y, starting_above = is_above;
		curr = first + 1;
		while (true)
		{
			if (curr == cend)
			{
				if (cend == first || first == cbegin) break;
				cend = first;
				curr = cbegin;
			}

			if (is_above)
			{
				while (curr != cend && curr->y < pt.y) ++curr;
				if (curr == cend) continue;
			}
			else
			{
				while (curr != cend && curr->y > pt.y) ++curr;
				if (curr == cend) continue;
			}

			if (curr == cbegin)
				prev = polygon.cend() - 1; //nb: NOT cend (since might equal first)
			else
				prev = curr - 1;

			if (curr->y == pt.y)
			{
				if (curr->x == pt.x ||
					(curr->y == prev->y &&
						((pt.x < prev->x) != (pt.x < curr->x))))
					return PointInPolygonResult::IsOn;
				++curr;
				if (curr == first) break;
				continue;
			}

			if (pt.x < curr->x && pt.x < prev->x)
			{
				// we're only interested in edges crossing on the left
			}
			else if (pt.x > prev->x && pt.x > curr->x)
				val = 1 - val; // toggle val
			else
			{
				double d = CrossProduct(*prev, *curr, pt);
				if (d == 0) return PointInPolygonResult::IsOn;
				if ((d < 0) == is_above) val = 1 - val;
			}
			is_above = !is_above;
			++curr;
		}

		if (is_above != starting_above)
		{
			cend = polygon.cend();
			if (curr == cend) curr = cbegin;
			if (curr == cbegin) prev = cend - 1;
			else prev = curr - 1;
			double d = CrossProduct(*prev, *curr, pt);
			if (d == 0) return PointInPolygonResult::IsOn;
			if ((d < 0) == is_above) val = 1 - val;
		}

		return (val == 0) ?
			PointInPolygonResult::IsOutside :
			PointInPolygonResult::IsInside;
	}
#endif
	//获取线段交点
	glm::vec2 IntersectPoint(const glm::vec2& pt1a, const glm::vec2& pt1b, const glm::vec2& pt2a, const glm::vec2& pt2b)
	{
		if (pt1a.x == pt1b.x) //vertical
		{
			if (pt2a.x == pt2b.x) return glm::vec2(0, 0);

			double m2 = (pt2b.y - pt2a.y) / (pt2b.x - pt2a.x);
			double b2 = pt2a.y - m2 * pt2a.x;
			return glm::vec2(pt1a.x, m2 * pt1a.x + b2);
		}
		else if (pt2a.x == pt2b.x) //vertical
		{
			double m1 = (pt1b.y - pt1a.y) / (pt1b.x - pt1a.x);
			double b1 = pt1a.y - m1 * pt1a.x;
			return glm::vec2(pt2a.x, m1 * pt2a.x + b1);
		}
		else
		{
			double m1 = (pt1b.y - pt1a.y) / (pt1b.x - pt1a.x);
			double b1 = pt1a.y - m1 * pt1a.x;
			double m2 = (pt2b.y - pt2a.y) / (pt2b.x - pt2a.x);
			double b2 = pt2a.y - m2 * pt2a.x;
			if (m1 == m2) return glm::vec2(0, 0);
			double x = (b2 - b1) / (m1 - m2);
			return glm::vec2(x, m1 * x + b1);
		}
	}

	// 点到线段线的距离
	double pointToSegDistance2D(glm::vec2 p, glm::vec2 l1, glm::vec2 l2)
	{
		glm::vec2 line_vec = l2 - l1; //AB
		glm::vec2 point_vec = p - l1; //AP
		double c = glm::dot(line_vec, point_vec); //|AB*AP|
		if (c <= 0) return (glm::length(point_vec)); //|AP|
		double d = pow(glm::length(line_vec), 2); //|AB|^2
		if (c >= d) return (glm::length(p - l2)); //|BP|
		double r = c / d; //相似三角形求出c点的坐标
		double px = l1.x + (l2.x - l1.x) * r;
		double py = l1.y + (l2.y - l1.y) * r;
		glm::vec2 p_shadow = glm::vec2(px, py); //投影点c
		return (glm::length(p - p_shadow)); //|CP|
	}
	// 点到直线的距离
	double pointToLineDistance2D(glm::vec2 p, glm::vec2 l1, glm::vec2 l2)
	{
		assert(p != l1 && p != l2);
		glm::vec2 line_vec = l2 - l1; //AB
		glm::vec2 point_vec = p - l1; //AP
		double d = glm::dot(line_vec, point_vec) / glm::length(line_vec); //投影的长度 |AC|
		return (sqrt(pow(glm::length(point_vec), 2) - pow(d, 2))); //勾股定理：|CP| = sqrt(AP^2-AC^2)

	}

	// 点是否在多边形，返回距离
	bool ray_p2polygon(glm::vec2* vertices, int count, const glm::vec2& p, float& d)
	{
		bool r = 0;
		float rd = INT_MAX;
		if (vertices && count > 0)
		{
			glm::vec4 p2;
			p2.x = p.x;
			p2.y = p.y;
			p2.w = p.y;
			p2.z = -p.x * 10;
			for (size_t i = 0; i < count; i++) {
				auto v1 = vertices[i];
				auto v2 = vertices[((i + 1) % count)];
				float d = 0;
				if (ray_cast(p2, v1, v2, d))
				{
					if (d < rd)
						rd = d;
					r = true;
				}
			}
		}
		d = rd;
		return r;
	}
	bool ray_p2polygon(glm::dvec2* vertices, int count, const glm::dvec2& p, double& d)
	{
		bool r = 0;
		double rd = INT_MAX;
		if (vertices && count > 0)
		{
			glm::dvec4 p2;
			p2.x = p.x;
			p2.y = p.y;
			p2.w = p.y;
			p2.z = -p.x * 10;
			for (size_t i = 0; i < count; i++) {
				auto v1 = vertices[i];
				auto v2 = vertices[((i + 1) % count)];
				double d1 = 0;
				if (ray_cast(p2, v1, v2, d1))
				{
					if (d1 < rd)
						rd = d1;
					r = true;
				}
			}
		}
		d = rd;
		return r;
	}
#endif


	std::vector<std::vector<glm::vec2>>  get_triangulate_center_lines(path_v* p, int segments, float ml, float ds, const glm::vec2& z2
		, std::vector<std::vector<glm::vec3>>& mtv, std::vector<glm::vec2>& ms)
	{
		std::vector<glm::vec2> pv, pdt;
		p->triangulate(segments, ml, ds, 0, &pv);
		auto length = pv.size();
		pdt.reserve(length * 4 * 3);
		std::map<glm::vec2, int> mvi;
		auto t = pv.data();
		float z = z2.x + z2.y;
		float pz = z2.x;
		std::vector<glm::vec4> edg;
		std::vector<glm::vec2> bv;
		for (size_t i = 0; i < length; i += 3)
		{
			auto ps = pdt.size();
			// 获取三边中心点
			divide_triangle(t[0], t[1], t[2], &pdt);
			t += 3;
			edg.push_back({ pdt[ps] ,pdt[ps + 1] });
			edg.push_back({ pdt[ps + 1] ,pdt[ps + 2] });
			edg.push_back({ pdt[ps + 2] ,pdt[ps] });
			for (size_t x = 0; x < 3; x++)
			{
				mvi[pdt[x + ps]]++;
			}
			ms.push_back(t[0]);
			ms.push_back(t[1]);
			ms.push_back(t[2]);
		}
		length = edg.size();
		for (auto& it : edg)
		{
			glm::vec2 v = { it.x,it.y }, v1 = { it.z,it.w };
			auto vt = mvi.find(v);
			auto vt1 = mvi.find(v1);
			if (vt == mvi.end() || !(vt->second > 1) || vt1 == mvi.end() || !(vt1->second > 1))continue;
			bv.push_back(v);
			bv.push_back(v1);
		}
		auto bv1 = line2one(bv);

		return bv1;
	}
	// todo单面打孔，tr.x=半径，tr.y=间隔
	int build_plane1hole(cmd_plane_t* c, float scale, float z, const glm::vec2& trv, std::vector<glm::vec2>* cshape, std::vector<glm::ivec2>* idx, std::vector<glm::vec3>* vpos)
	{
		bool is_round = 0;
		int r = 0;
		auto pt = gp::new_path_node_exp(c->pv, 0, scale - c->thickness, c->radius, c->radius_a, c->type, c->rccw.y, c->segments, c->segments_len, c->dist, is_round);
		if (!pt)return r;
		auto pt2 = gp::new_path_node_exp(c->pv, 0, scale, c->radius, c->radius_a, c->type, c->rccw.x, c->segments, c->segments_len, c->dist, is_round);
		if (!pt2)return r;
		if (!pt)return r;

		std::vector<std::vector<glm::vec2>> ots1, tr;
		std::vector<glm::vec2> nl, nl0;
		ots1.clear();
		auto wd = -z + (z) * 0.5;

		path_v pv = *c->pv;
		//pv.add(c->pv);
		auto jg = 1;// trv.y + trv.x;
		auto jds = trv.y + trv.x;
		PathsD hole;
		int seg = 4 * c->segments;
		std::vector<glm::vec2> line0, ml, ml0, als;
		std::vector<glm::vec2> newc, newc0, nc1;
		jg = trv.y + trv.x * 2;
		auto ts = c->thickness > 0 ? c->thickness : 1;

		if (cshape && cshape->size() && idx && idx->size() && vpos && vpos->size()) {
			Union2cs(hole, cshape, idx, vpos);
		}
		else if (trv.x > 0)
		{
			if (jg > 0)
			{
				std::vector<std::vector<glm::vec3>> mtv;
				std::vector<std::vector<glm::vec2>> tvs;
				auto cs = get_triangulate_center_lines(&pv, c->segments, c->segments_len, c->dist, { 0,1 }, mtv, nl0);
				pv.get_expand_flatten2(0, -c->thickness, c->segments, c->segments_len, c->segments_len, 2, &tvs, 0);
				auto ri = abs(wd);
				// 合并连接线条
				std::queue<std::vector<glm::vec2>*> qc;
				for (size_t i = 0; i < cs.size(); i++)
				{
					auto& it = cs[i];
					if (it.size() > 1)
					{
						qc.push(&it);
					}
				}
				for (; qc.size();)
				{
					auto pt = qc.front(); qc.pop();
					for (size_t i = 0; i < cs.size(); i++)
					{
						auto it = &cs[i];
						if (it != pt && pt->size() && it->size())
						{
							auto v0 = *pt->begin();
							auto v1 = *pt->rbegin();
							auto vi0 = *it->begin();
							auto vi1 = *it->rbegin();
							if (glm::distance(v1, vi0) < ts)
							{
								it->insert(it->begin(), pt->begin(), pt->end());
								pt->clear();
							}
							if (glm::distance(v0, vi1) < ts)
							{
								it->insert(it->end(), pt->begin(), pt->end());
								pt->clear();
							}
						}
					}
				}
				for (size_t i = 0; i < cs.size(); i++)
				{
					auto& it = cs[i];
					if (it.size() < 2)continue;
					nl.clear();
					// 细分直线
					auto length = it.size();
					for (size_t x = 1; x < length; x++)
					{
						auto d = glm::distance(it[x], it[x - 1]);
						auto k1 = it[x - 1]; auto k2 = it[x];
						if (d > 1)
						{
							int dt = d;
							for (size_t o = 0; o < dt; o++)
							{
								float xx = (double)o / dt;
								auto ps = glm::mix(k1, k2, xx);
								nc1.push_back(ps);
							}
							nc1.push_back(k2);
						}
						else {
							nc1.push_back(k1);
							nc1.push_back(k2);
						}
					}
					it.swap(nc1);
					std::vector<size_t> nks;
					for (auto& kt : it)
					{
						size_t nk = 0;
						if (nb_poly(tvs, kt, jds, nk))
						{
							newc0.push_back(kt);
						}
						nks.push_back(nk);
					}
					newc.clear();
					for (auto vt : it)
					{
						als.push_back(vt);
					}

				}
				seg = seg;
			}
			nc1.clear();
			get_equ_xj(newc0, jg, nc1);
			Union2Circles(hole, nc1, trv.x, seg, false, cshape);
		}

		PathsD subjects, subjects1;
		pt2paths(pt, subjects, c->segments, 0);
		pt2paths(pt2, subjects1, c->segments, 0);
		r = cplane(c, subjects, 0, &hole, z, c->pccw, {}, false);
		auto z2 = c->pccw ? z + c->thickness : z - c->thickness;
		r += cplane(c, subjects1, 0, &hole, z2, !c->pccw, {}, false);

		r += mkhole_ext(hole, c, { z, z2 }, true);

		gp::free_path_node(pt);
		gp::free_path_node(pt2);
		return r;
	}

	PathsD get_hole(cmd_plane_t* c, float scale, float expand, float z, const glm::vec2& trv, std::vector<glm::vec2>* cshape)
	{
		PathsD subjects0;
		if (!(trv.x > 0))return subjects0;
		std::vector<std::vector<glm::vec2>> ots1;
		std::vector<glm::vec2> nl;
		ots1.clear();
		auto wd = -z + scale + (expand + z) * 0.5;
		c->pv->get_expand_flatten(-1, wd, c->segments, c->segments_len, c->dist, 1, &ots1, 0);
		//path_v cir;
		auto ri = abs(wd);
		auto jg = trv.y + trv.x * 2;
		int seg = 4 * c->segments;
		for (size_t i = 0; i < ots1.size(); i++)
		{
			auto& it = ots1[i];
			if (it.size() < 3)continue;
			if (it[0] != it[it.size() - 1])
				it.push_back(it[0]);
			nl.clear();
			auto ds = get_equ_point(it, jg, 1, nl, true);
			//cd->draw_circle(&nl, 4, st);//批量画圆 
			//cir.addCircle(nl.data(), nl.size(), trv.x);
			Union2Circles(subjects0, nl, trv.x, seg, cshape);
		}
		int decimal_prec = 100;
		for (auto& it : subjects0) {
			for (auto& v : it) {
				glm::ivec2 k = { v.x,v.y };
				v.x = k.x;
				v.y = k.y;
			}
		}
		return subjects0;
	}
	// 打孔双路径面tr.x=半径，tr.y=间隔
	int build_plane2hole(cmd_plane_t* c, float scale, float expand, float z, const glm::vec2& trv, bool rv, std::vector<glm::vec2>* cshape, std::vector<glm::ivec2>* idx, std::vector<glm::vec3>* vpos)
	{
		bool is_round = 0;
		int r = 0;
		float sc[3] = { scale - c->thickness,scale ,0 };
		auto rccw = c->rccw;
		if (expand > 0)
		{
			sc[1] = scale - c->thickness;
			rccw.x = rccw.y;
		}
		auto pt = gp::new_path_node_exp(c->pv, 0, sc[0], c->radius, c->radius_a, c->type, rccw.y, c->segments, c->segments_len, c->dist, is_round);
		auto pt2 = gp::new_path_node_exp(c->pv, 0, sc[1], c->radius, c->radius_a, c->type, rccw.x, c->segments, c->segments_len, c->dist, is_round);
		auto pt1 = gp::new_path_node_exp(c->pv, expand, 0, c->radius, c->radius_a, (int)Clipper2Lib::JoinType::Round, rccw.y, c->segments, c->segments_len, c->dist, is_round);
		if (!pt || !pt1 || !pt2)return r;

		std::vector<std::vector<glm::vec2>> tr;
		PathsD  clips, hole;
		pt2paths(pt1, clips, c->segments, 0);
		if (cshape && cshape->size() && idx && idx->size() && vpos) {
			Union2cs(hole, cshape, idx, vpos);// 自定义孔
		}
		else
		{
			hole = get_hole(c, scale, expand, z, trv, cshape);//默认孔
		}

		auto z2 = c->pccw ? z + c->thickness : z - c->thickness;

		PathsD subjects, subjects1;
		pt2paths(pt, subjects, c->segments, 0);
		pt2paths(pt2, subjects1, c->segments, 0);

		{
			r = cplane(c, subjects, &clips, &hole, z, c->pccw, { rv,0 }, false);
			if (c->thickness > 0)
				r += cplane(c, subjects1, &clips, &hole, z2, !c->pccw, { rv,0 }, false);
		}
		glm::vec2 dz = { z, z2 };
		r += mk_ext(pt1, c, dz, expand > 0);// 外圈竖面
		r += mkhole_ext(hole, c, dz, true);// 孔竖面
		gp::free_path_node(pt);
		if (pt1 != pt)
			gp::free_path_node(pt1);
		gp::free_path_node(pt2);

		return r;
	}

	int build_plane2sc(cmd_plane_t* c, float scale, float scale1, float z, bool rv)
	{
		bool is_round = 0;
		int r = 0;
		auto rccw = c->rccw;
		auto pt = gp::new_path_node_exp(c->pv, 0, scale, c->radius, c->radius_a, c->type, rccw.y, c->segments, c->segments_len, c->dist, is_round);
		auto pt2 = gp::new_path_node_exp(c->pv, 0, scale1, c->radius, c->radius_a, c->type, rccw.x, c->segments, c->segments_len, c->dist, is_round);
		if (!pt || !pt2)return r;

		std::vector<std::vector<glm::vec2>> tr;
		PathsD  clips;
		pt2paths(pt2, clips, c->segments, 0);
		PathsD subjects, subjects1;
		pt2paths(pt, subjects, c->segments, 0);
		r = cplane(c, subjects, &clips, 0, z, c->pccw, { rv,0 }, false);
		gp::free_path_node(pt);
		if (pt2 != pt)
			gp::free_path_node(pt2);

		return r;
	}
	std::vector<glm::vec2> get_cp2(tinyspline::BSpline* p)
	{
		auto cp = p->controlPoints();
		std::vector<glm::vec2> cp2;
		for (size_t i = 0; i < cp.size(); i++)
		{
			cp2.push_back({ cp[i],cp[i + 1] }); i++;
		}
		return cp2;
	}

	int BSpline0(std::vector<glm::vec2>* v, int m, std::vector<glm::vec2>& ot)
	{
		if (!v || v->empty())return -1;
		if (m < 0)
		{
			m = 0;
		}
		std::vector<glm::vec2>& Ps = *v;
		auto pos = ot.size();
		auto c = v->size();
		tinyspline::BSpline spline(c);

		// Setup control points.
		std::vector<tinyspline::real> ctrlp = spline.controlPoints();
		auto t = v->data();
		c = ctrlp.size();
		for (size_t i = 0; i < c; i++)
		{
			ctrlp[i] = t->x;
			ctrlp[i + 1] = t->y; i++; t++;
		}
		spline.setControlPoints(ctrlp);
		// Evaluate `spline` at u = 0.4 using 'eval'. 
		auto sm = spline.sample(m);
		for (size_t i = 0; i < sm.size(); i++)
		{
			auto result = glm::vec2(sm[i], sm[i + 1]); i++;
			ot.push_back(result);
			//std::cout << result[0] << " " << result[1] << std::endl;
		}
		return ot.size() - pos;
	}
	std::vector<glm::vec2> BSpline0(std::vector<glm::vec2>* v, int m)
	{
		std::vector<glm::vec2> r;
		BSpline0(v, m, r);
		return r;
	}

	void mkspline(std::vector<glm::vec2>& v, int ct, int m, std::vector<glm::vec2>& opt)
	{
		std::vector<glm::vec2> bs;
		if (ct == 0)
		{
			bs = BSpline0(&v, m);
		}
		else if (ct == 1)
		{
			bs = v;
		}
		else if (ct == 2)
		{
			auto vs = v.size();
			if (vs > 3)
			{
				vs--;
				auto length = vs / 3;
				auto t = v.data();
				for (size_t i = 0; i < length; i++)
				{
					cubic_v path = {};
					path.p0 = t[0];
					path.p1 = t[1];
					path.p2 = t[2];
					path.p3 = t[3];
					auto bs0 = get_bezier(&path, 1, m);
					if (bs0.size())
					{
						bs.insert(bs.end(), bs0.begin(), bs0.end());
					}
					t += 3;
				}
			}
		}
		opt.swap(bs);
	}

	uint64_t toUInt(const njson& v, uint64_t de)
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
			ret = std::atoll(md::trim(v[k].dump(), "\"").c_str());
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
			ret = std::atoll(md::trim(v.dump(), "\"").c_str());
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
			ret = std::atof(md::trim(v.dump(), "\"").c_str());
		}
		return ret;
	}
#ifndef toFloat
#define toFloat toDouble
#endif


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
			ret = md::trim(v[k].dump(), "\"");
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
			ret = md::trim(v.dump(), "\"");
		}
		return ret;
	}

	int64_t str2int(const char* str, int64_t de)
	{
		int64_t ret = de;
		if (str)
		{
			for (; *str && !(*str == '-' || *str == '+' || *str == '.' || (*str >= '0' && *str <= '9')); str++);
			if (*str)
				ret = atoll(str);
		}
		return ret;
	}
	njson str2ints(const std::string& s)
	{
		njson ret;
		if (s.size())
		{
			auto str = s.c_str();
			for (; *str; str++)
			{
				int d = 0;
				if (*str == '-' || *str == '+' || *str == '.')
				{
					d++;
					str++;
				}
				if (!(*str < '0' || *str > '9'))
				{
					auto t = str - d;
					char* et = 0;
					auto n = std::strtoll(t, &et, 10);//strtod
					str = et;
					ret.push_back(n);
				}
			}
		}
		return ret;
	}


	void gbx(glm::vec2 v, glm::vec2& mmin, glm::vec2& mmax)
	{
		mmin.x = std::min(v.x, mmin.x);
		mmin.y = std::min(v.y, mmin.y);
		mmax.x = std::max(v.x, mmax.x);
		mmax.y = std::max(v.y, mmax.y);
	}

	void get_v2s(njson& n, const char* k, std::vector<glm::vec2>& v) {
		if (n.find(k) == n.end() || !n[k].is_array() || n[k].size() < 2) { return; }
		float f = 0;
		int x = 0;
		auto ns = n[k];
		for (auto& it : ns) {
			x++;
			if (x == 2)
			{
				float f1 = it;
				v.push_back({ f,f1 });
				x = 0;
			}
			else { f = it; }
		}
	}
	std::vector<float> get_vs(njson& n, const char* k) {
		std::vector<float> v;
		if (n.find(k) == n.end() || n[k].empty()) { return v; }
		auto ns = n[k];
		if (ns.is_array())
		{
			for (auto& it : ns) {
				v.push_back(toFloat(it));
			}
		}
		else { v.push_back(toFloat(ns)); }
		return v;
	}
	std::vector<glm::vec2> get_v2(njson& n, const char* k)
	{
		std::vector<glm::vec2> v;
		if (n.find(k) == n.end() || !n[k].is_array() || n[k].size() < 2) { return v; }
		float f = 0;
		int x = 0;
		auto ns = n[k];
		for (auto& it : ns) {
			x++;
			if (x == 2)
			{
				int f1 = toFloat(it);
				v.push_back({ f,f1 });
				x = 0;
			}
			else { f = toFloat(it); }
		}
		return v;
	}
	std::vector<glm::ivec2> get_iv2(njson& n, const char* k)
	{
		std::vector<glm::ivec2> v;
		if (n.find(k) == n.end() || !n[k].is_array() || n[k].size() < 2) { return v; }
		int f = 0;
		int x = 0;
		auto ns = n[k];
		for (auto& it : ns) {
			x++;
			if (x == 2)
			{
				int f1 = toInt(it);
				v.push_back({ f,f1 });
				x = 0;
			}
			else { f = toInt(it); }
		}
		return v;
	}
	std::vector<glm::vec3> get_v3(njson& n, const char* k)
	{
		std::vector<glm::vec3> v;
		if (n.find(k) == n.end() || !n[k].is_array() || n[k].size() < 3) { return v; }

		auto ns = n[k];
		auto c = ns.size();
		for (size_t i = 0; i < c; i += 3)
		{
			glm::vec3 vt = { toFloat(ns[i]),toFloat(ns[i + 1]), toFloat(ns[i + 2]) };
			v.push_back(vt);
		}
		return v;
	}

	// 命令

	void dv_cmd_t::set_plane1(float expand, bool ccw, float z, float radius, int v_ccw1, int ptype)
	{
		this->expand[0] = expand;
		this->ccw[0] = ccw;
		this->plane_z[0] = z;
		this->radius[0] = radius;
		this->plane_type = ptype;
		this->plane_ccw[0] = v_ccw1;
	}
	void dv_cmd_t::set_plane2(float expand, bool ccw, float z, float radius, int v_ccw1)
	{
		this->expand[1] = expand;
		this->ccw[1] = ccw;
		this->plane_z[1] = z;
		this->radius[1] = radius;
		this->plane_ccw[1] = v_ccw1;
	}

	struct closed_ta
	{
		int type = 0;		// 0为顶，1为底
		// 封闭厚度
		float thickness = 0;
		// 台阶 宽、高、厚度、方向(0=上平，1下平)
		glm::vec4 step = {};
	};
	struct step_info_t
	{
		float pos = 0;			//坐标
		float expand = 0;		//全局偏移
		float radius = 0;		//圆角
		float thickness = 1.0;	// 墙厚
		float k = 0;			// 补高
		float expand0 = 0;		//全局内偏移
		cmd_plane_t* c = 0;		//输出
	};
	template<class T2>
	void swapv2max(T2& v)
	{
		if (v.x < v.y)
		{
			std::swap(v.x, v.y);
		}
	}

	glm::vec2 add_footstep(int dir, glm::vec3 v, glm::vec2 w, glm::vec2 r, float all_expand, cmd_plane_t* c)
	{
		glm::vec2 dz[2];
		float y = v.y;
		float z = v.z;
		if (dir == 0)
		{
			dz[0] = { z + y, z + y };
			dz[1] = { 0 + y, z + y };
		}
		else {
			dz[0] = { z + y,  y };
			dz[1] = { y,  y };
		}
		if (r.x > 0 && r.y < 0)
		{
			r.y = r.x * 2.0;
		}
		// 台阶 
		dv_cmd_t a = {};
		a.set_plane1(w.x + all_expand, true, dz[0].x, r.x, 0, 2);
		a.set_plane2(w.y + all_expand, true, dz[0].y, r.y, 0);
		if (a.plane_z.x != a.plane_z.y || a.expand.x != a.expand.y)
		{
			auto c1 = *c;
			c = &c1;
			c->radius = r.x;
			c->rccw = { 1,1 };
			c->pccw = 0;
			build_plane3(c, a.expand, a.plane_z);
		}
		a = {};
		a.set_plane1(w.x + all_expand, true, dz[1].x, r.x, 1, 2);
		a.set_plane2(w.y + all_expand, true, dz[1].y, r.y, 1);
		if (a.plane_z.x != a.plane_z.y || a.expand.x != a.expand.y)
		{
			auto c1 = *c;
			c = &c1;
			c->radius = r.x;
			c->rccw = { 1,1 };
			c->pccw = 1;
			build_plane3(c, a.expand, a.plane_z);
		}
		a = {};
		// 外墙
		dv_wall_t wt = {};
		wt.height = { dz[0].x ,dz[1].x };
		return wt.height;
	}

	void new_step(closed_t* ct, step_info_t* p)
	{
		auto c = p->c;
		float r = p->radius;
		float innerexp = -p->thickness;// 内壁收缩值
		glm::vec3 v1 = ct->step, v2 = ct->step;
		glm::vec2 pos = { p->pos,p->pos };
		glm::vec2 height = {  };
		glm::vec2 height1 = {  };
		glm::vec2 height2 = {  };
		glm::vec2 n2 = {  };
		float expand = p->expand;
		float expand0 = p->expand0 + innerexp;
		int pc = 0;
		if (p->k < 0)
			p->k = 0;
		// 墙面
		if (ct->step.x > 0)
		{
			switch (ct->type)
			{
			case 0:
				pos += v1.y + v1.z + p->k;
				v2.y = p->pos + p->k;
				height1.y = p->pos + v1.z + p->k;
				height2.x = p->pos + p->k;
				height2.y = p->pos;
				break;
			case 1:
				pos -= v1.y + v1.z + p->k;
				v2.y = p->pos - (p->k + v1.z);
				height1.y = p->pos - (v1.z + p->k);
				height2.x = p->pos - p->k;
				height2.y = p->pos;
				break;
			default:
				break;
			}
			height = { pos.x, p->pos };
			height1.x = pos.x;
			swapv2max(height);
			swapv2max(height1);
			swapv2max(height2);
			c->pccw = ct->type;
			c->rccw = { 0,1 };
			build_plane3(c, { expand,expand0 }, pos);
			n2 = add_footstep(ct->step.w, v2, { expand0 ,expand0 - ct->step.x }, { r ,r }, 0, c);

		}
		else
		{
			glm::ivec2 pccw = { 0,1 };
			switch (ct->type)
			{
			case 0:
				pos += ct->thickness + p->k;
				pos.y -= ct->thickness;
				height2.x = p->pos + p->k;
				height2.y = p->pos;
				break;
			case 1:
				pos -= ct->thickness + p->k;
				pos.y += ct->thickness;
				height2.x = p->pos - p->k;
				height2.y = p->pos;
				pccw = { 1,0 };
				pc = 1;
				break;
			default:
				break;
			}
			height = { pos.x, p->pos };
			if (ct->thickness > 0)
			{
				c->pccw = pccw.x;
				c->rccw = { 0,0 };
				build_plane1(c, 0, expand, pos.x);//外面			
				c->pccw = pccw.y;
				c->rccw = { 1,1 };
				build_plane1(c, 0, expand0, pos.y);//内面
			}
			else {
				c->pccw = ct->type;
				c->rccw = { 0,1 }; pos.y = pos.x;
				build_plane3(c, { expand,expand0 }, pos);
			}
			swapv2max(height);
		}
		//外墙
		c->pccw = 1;
		c->rccw = { 0,0 };
		if (height.x != height.y)
			build_plane3(c, expand, height);
		c->rccw = { 1,1 };
		c->pccw = pc;
		//内墙
		if (height1.x != height1.y)
			build_plane3(c, expand0, height1);
		if ((height2.x != height2.y))
			build_plane3(c, expand0, height2);

	}

	glm::vec4 get_step(const njson& v, double d = 0)
	{
		glm::vec4 rv = { d, d, d, d };
		if (v.is_number())
		{
			rv.z = v.get<double>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			if (v.size() == 4)
			{
				for (size_t i = 0; i < 4; i++)
				{
					rv[i] = v[i].get<double>();
				}
			}
			else
			{
				rv.z = v[0].get<double>();
			}
		}
		return rv;
	}
	void mkstep(base_mv_t& bmt, mkcustom_dt* n, glm::vec2 pos, glm::vec2 k, cmd_plane_t* c)
	{
		closed_t clt = {};
		auto se2 = n->step_expand;
		auto se20 = n->step_expand0;
		float tt = 0, tt1 = 0;
		if (n->step.x < 0) {
			clt.type = 0;
			auto v4 = n->step;
			if ((v4.y + v4.z + v4.w) > 1)
			{
				clt.step = v4;
				tt = clt.step.y;
			}
			else {
				clt.thickness = v4.x;
				tt = clt.thickness;

			}
			// 顶 
			step_info_t sp = {};
			sp.c = c;
			sp.radius = bmt.radius;
			sp.thickness = bmt.thickness;
			sp.k = k.x;
			sp.pos = pos.x;
			sp.expand = se2.x;
			sp.expand0 = se20.x;
			new_step(&clt, &sp);
		}
		clt = {};
		if (n->step1.y < 0) {
			clt.type = 1;
			auto v4 = n->step1;
			if ((v4.y + v4.z + v4.w) > 1)
			{
				clt.step = v4;
				tt1 = clt.step.y;
			}
			else {
				clt.thickness = v4.x;
				tt1 = clt.thickness;
			}
			// 底 
			step_info_t sp = {};
			sp.c = c;
			sp.radius = bmt.radius;
			sp.thickness = bmt.thickness;
			sp.k = k.y;
			sp.pos = pos.y;
			sp.expand = se2.y;
			sp.expand0 = se20.y;
			new_step(&clt, &sp);
		}
	}
	// 生成B样条线约束的竖三角面
	glm::vec4 mkcustom(mkcustom_dt* np, glm::vec2 k, base_mv_t& bm, cmd_plane_t* c, const glm::uvec2& bcount)
	{
		glm::vec4 v4 = {};
		do {
			std::vector<glm::vec2> bs, bs1;
			mkspline(np->v, np->ct, np->m, bs);		// v外壁
			mkspline(np->v1, np->ct, np->m, bs1);	// v1内壁
			if (bs.empty()) { break; }
			glm::vec2 sth = { bm.box_size.z,0 };
			std::vector<glm::vec2> sw;
			{
				vec_wall wv0, wv1;
				float thickness = bm.thickness;
				c->radius = bm.radius;
				c->pccw = 1;//生成外壁面
				c->rccw = { 0,0 };
				auto bsc = bs.size();
				auto pt = bs.data();
				bsc--;
				glm::vec2 tk = {};
				glm::vec2 mmin = glm::vec2(INT_MAX, INT_MAX);
				glm::vec2 mmax = glm::vec2(-INT_MAX, -INT_MAX);
				auto uc = bcount;
				for (size_t i = 0; i < bsc; i++, pt++)
				{
					auto pt1 = pt + 1;
					if (*pt == *pt1)continue;
					auto v = *pt;
					gbx(v, mmin, mmax);
					gbx(*pt1, mmin, mmax);
					tk = { pt->x,pt1->x };
					build_plane3(c, tk, { pt->y, pt1->y }); // 生成一圈三角形
				}
				sw.push_back(bs[0]);
				sw.push_back(*bs.rbegin());
				v4 = { mmin,mmax };
				glm::vec2 nh = { v4.w ,v4.y };
				sth = nh;
				c->pccw = 0;//生成内壁面
				c->rccw = { 1,1 };
				if (bs1.empty())
				{
					pt = bs.data();
				}
				else {
					pt = bs1.data();
					bsc = bs1.size(); bsc--;
				}
				auto pt0 = pt;
				float yy = 0;
				for (size_t i = 0; i < bsc; i++, pt++)
				{
					auto pt1 = pt + 1;
					if (*pt == *pt1)continue;
					tk = { pt->x - thickness, pt1->x - thickness };
					build_plane3(c, tk, { pt->y, pt1->y });
				}
				auto bt = pt0[0];
				bt.x -= thickness;
				sw.push_back(bt);
				bt = { tk.y,0 };
				sw.push_back(bt);
			}

			glm::vec2 vs = np->step2;
			int kc = 0;
			auto ctk = c->thickness;

			if (np->step.x >= 0)
			{
				auto vst = np->step;
				if (vst.y == 0 && vst[0] == 0)
				{
					vs[0] = 1;
				}
			}
			if (np->step1.x >= 0)
			{
				auto vst = np->step1;
				if (vst.y == 0 && vst[0] == 0)
				{
					vs[1] = 1;
				}
			}
			glm::vec2 tk1 = { sw[0].x,sw[2].x };
			glm::vec2 tk2 = { sw[1].x,sw[3].x };
			np->step_expand = { tk1.x,tk2.x };
			np->step_expand0 = { tk1.y + ctk,tk2.y + ctk };
			{
				if (vs[0] == 1)
				{
					c->pccw = 0;//生成端面 上
					c->rccw = { 0,1 };
					c->thickness = 0;
					build_plane3(c, { tk1.x, tk1.y }, { sw[0].y ,sw[0].y });
					np->step.x = -1;
				}
				if (vs[1] == 1)
				{
					c->pccw = 1;//生成端面 下
					c->rccw = { 0,1 };
					c->thickness = 0;
					build_plane3(c, { tk2.x, tk2.y }, { sw[1].y ,sw[1].y });
					np->step1.x = -1;
				}
				kc++;
			}
			c->thickness = ctk;
			mkstep(bm, np, sth, k, c);
		} while (0);
		return v4;
	}




	//void mesh_mt::build_halfedge()
	//{
	//	// 半边需要加载数据有索引、面边数
	//	if (face_size.empty() || face_indice.empty())return;
	//	auto fx = face_indice.data();
	//	auto fs = face_size.size();
	//	auto fds = face_size.data();
	//	halfedge.clear();
	//	halfedge.reserve(fs * 4);
	//	for (auto& ft : face_size)
	//	{
	//		switch (ft)
	//		{
	//		case 3:
	//		{
	//			auto idx = (glm::uvec3*)fx;
	//			halfedge.push_back({ idx->x,idx->y });
	//			halfedge.push_back({ idx->y,idx->z });
	//			halfedge.push_back({ idx->z,idx->x });

	//		}break;
	//		case 4:
	//		{
	//			auto idx = (glm::uvec4*)fx;
	//			halfedge.push_back({ idx->x,idx->y });
	//			halfedge.push_back({ idx->y,idx->z });
	//			halfedge.push_back({ idx->z,idx->w });
	//			halfedge.push_back({ idx->w,idx->x });
	//		}break;
	//		default:
	//			break;
	//		}
	//		fx += ft;
	//	}
	//}
	void mesh_mt::add_vertex(const glm::dvec3* v, size_t n)
	{
		if (v && n > 0)
		{
			auto ps = vertex_coord.size();
			vertex_coord.resize(vertex_coord.size() + n * 3);
			memcpy(vertex_coord.data() + ps, v, sizeof(glm::dvec3) * n);
		}
	}
	void mesh_mt::add_vertex(const glm::vec3* v, size_t n)
	{
		if (v && n > 0)
		{
			auto ps = vertex_coord.size();
			vertex_coord.reserve(vertex_coord.size() + n * 3);
			for (size_t i = 0; i < n; i++)
			{
				vertex_coord.push_back(v->x);
				vertex_coord.push_back(v->y);
				vertex_coord.push_back(v->z);
				v++;
			}
		}
	}
	/*
	绕x轴旋转180度的四元数为(1, 0, 0, 0)。‌
	绕y轴旋转180度的四元数为(0, 1, 0, 0)。‌
	绕z轴旋转180度的四元数为(0, 0, 1, 0)。‌
		struct extrude_t {
		float depth = 0;		// 深度
		float count = 5;		// 分辨率
		float thickness = 1.0;	// 厚度
		glm::ivec2 type = {};	//样式  x.0=v，1=U，2=|_|，y=-1倒过来
	};
	*/
	glm::vec3 make_poscb(float a, float r, const glm::vec2& rx, int type)
	{
		glm::vec3 ret = {};
		auto pi = (glm::pi<float>());
		auto pi2 = (glm::pi<float>() * 0.5);
		switch (type)
		{
		case 0:
		{
			// V形
			ret.x = glm::mix(r, -r, rx.x / rx.y);
			if (a > pi2)
				a = pi - a;
			ret.z = glm::mix(0.0f, r, a / pi2);
			ret.y = 0;
		}break;
		case 1:
		{
			ret.x = cos(a) * r; // 半圆
			ret.z = sin(a) * r;
			ret.y = 0;
		}break;
		case 2:
		{
			glm::ivec2 ri = rx;
			if (ri.x == 0 || ri.x == ri.y)
			{
				ret.x = glm::mix(r, -r, rx.x / rx.y);
				ret.z = 0;
			}
			else
			{
				// 方形
				if (a > pi2)
					a = pi - a;
				if (ri.x < rx.y * 0.5)
				{
					ret.x = r;
					ret.z = r;
				}
				else if (ri.x > rx.y * 0.5)
				{
					ret.x = -r;;
					ret.z = r;
				}
				else
				{
					ret.x = 0; ret.z = r;
				}
			}
			ret.y = 0;
		}break;
		default:
			break;
		}
		return ret;
	}
	// 生成3D扩展线模型 
	void build_line3d(const glm::vec3& p1, const glm::vec3& p2, const glm::ivec2& size, line_style_t* style, mesh_mt* opt)
	{
		glm::vec3 pos1 = p1, pos2 = p2;
		std::vector<glm::vec2> ots;
		auto d = pos2 - pos1;
		if (d.y < 0) {
			std::swap(pos1, pos2);
			d = pos2 - pos1;
		}
		glm::vec3 d0 = { -d.y,d.x, d.z };
		d0 = glm::normalize(d0);
		auto v1 = d0 * size.x * 0.5;
		auto v2 = d0 * -size.x * 0.5;
		std::vector<glm::vec3> v = { pos1 + v1,pos1 + v2,  pos2 + v1,pos2 + v2 };
		auto vz = pos1;
		vz.z += style->depth * style->type.y;
		std::vector<glm::vec3> vh, vh0;
		glm::quat qt = {}; qt.w = 1;
		{
			auto style_depth = size.x * 0.5;
			glm::vec3 c1 = pos1;
			c1.y = size.y - style_depth;
			c1.z = style_depth;
			int sct = (style->count);
			if (style->type.x == 2)
				sct = 1;
			int ct = sct * 2.0 + 2;
			double st = glm::pi<double>() / ct;
			glm::vec3 ce = {};
			std::vector<glm::vec3>* pv[] = { &vh,&vh0 };

			float yy = pos1.y * 0;
			float zz = pos1.z;
			auto angle1 = atan2v(v[1] - pos1);	// 点P的方位角 
			auto a1 = glm::degrees(angle1);
			qt = glm::angleAxis(angle1, glm::vec3(0, 0, 1));
			int jc = style->thickness > 0 ? 2 : 1;
			for (size_t j = 0; j < jc; j++)
			{
				double k = 0;
				auto pvt = pv[j];
				c1.z -= j * style->thickness;
				glm::vec2 rx = { 0, ct };
				for (size_t i = 0; i < ct + 1; i++)
				{
					rx.x = i;
					glm::vec3 vcs = make_poscb(k, c1.z, rx, style->type.x);
					k += st;
					vcs = qt * vcs;
					if (i == 0) {
						auto vz = vcs.z;
						vcs.z += yy;
						vcs.z += style->bottom_thickness * j;
						pvt->push_back(vcs);//右
						vcs.z = vz;
						vcs.z += c1.y;
						pvt->push_back(vcs);
					}
					else if (i == ct) {
						vcs.z += c1.y;
						pvt->push_back(vcs);
						vcs.z += yy - c1.y;
						vcs.z += style->bottom_thickness * j;
						pvt->push_back(vcs);//左
					}
					else
					{
						vcs.z += c1.y;
						pvt->push_back(vcs);
					}
				}
				auto z0 = (*pvt)[0];
				auto pvd = pvt->data();// 更新错误高度
				for (size_t i = 1; i < pvt->size() - 1; i++)
				{
					auto& it = pvd[i];
					if (it.z < z0.z) {
						it.z = glm::mix(pvd[i - 1].z, pvd[i + 1].z, 0.5f);
					}
				}
				if (style->type.y < 0)
				{
					for (auto& it : *pvt)
					{
						it.z *= -1;
						it.z += size.y;
					}
				}
			}
		}
		opt->vertex_coord.reserve((vh.size() + vh0.size()) * 3);
		opt->face_indice.reserve((vh.size() + vh0.size()) * 3);
		auto& dva = opt->vertex_coord;
		auto& fida = opt->face_indice;
		auto& fs = opt->face_size;
		fs.reserve(vh.size() + vh0.size());
		size_t oldidx = dva.size() / 3;
		size_t idx = dva.size() / 3;
		size_t rc = 0;
		if (vh.size() > 1) {
			auto length = vh.size();
			if (style->thickness > 0)
			{
				for (size_t i = 0; i < length; i++)
				{
					auto it = vh[i];
					auto v2 = it + d;
					it += pos1;
					v2 += pos1;
					dva.push_back(it.x);
					dva.push_back(it.y);
					dva.push_back(it.z);
					it = v2;
					dva.push_back(it.x);
					dva.push_back(it.y);
					dva.push_back(it.z);

				}
				for (size_t i = 1; i < length; i++)
				{
					fida.push_back(idx);
					fida.push_back(idx + 1);
					fida.push_back(idx + 3);
					fida.push_back(idx + 2);
					fs.push_back(4);
					idx += 2;
				}
			}
			else {

				std::vector<std::vector<glm::vec2>> paths;
				std::vector<std::vector<glm::vec2>> paths1;
				paths.resize(1);
				paths1.resize(1);
				for (size_t i = 0; i < length; i++)
				{
					auto it = vh[i];
					it += pos1;
					paths[0].push_back({ it.x,it.z });
					dva.push_back(it.x);
					dva.push_back(it.y);
					dva.push_back(it.z);
				}
				auto idx2 = dva.size() / 3;
				for (size_t i = 0; i < length; i++)
				{
					auto it = vh[i];
					auto v2 = it + d;
					v2 += pos1;
					paths1[0].push_back({ v2.x,v2.z });
					it = v2;
					dva.push_back(it.x);
					dva.push_back(it.y);
					dva.push_back(it.z);
				}
				// 侧面
				for (size_t i = 1; i < length; i++)
				{
					fida.push_back(idx);
					fida.push_back(idx + idx2);
					fida.push_back(idx + idx2 + 1);
					fida.push_back(idx + 1);
					fs.push_back(4);
					idx++;
				}
				// 底
				fida.push_back(oldidx);
				fida.push_back(oldidx + length - 1);
				fida.push_back(oldidx + idx2 + length - 1);
				fida.push_back(oldidx + idx2);
				fs.push_back(4);
				rc = fida.size();
				// 左右
				std::vector<glm::vec3> vd; std::vector<glm::ivec3>  idxs;
				constrained_delaunay_triangulation_v(&paths, vd, idxs, 0, 0);
				if (idxs.size())
				{
					for (auto it : idxs)
					{
						fida.push_back(oldidx + it.x);
						fida.push_back(oldidx + it.y);
						fida.push_back(oldidx + it.z);
						fs.push_back(3);
					}
				}
				vd.clear();
				idxs.clear();
				constrained_delaunay_triangulation_v(&paths1, vd, idxs, 0, 0);
				if (idxs.size())
				{
					for (auto it : idxs) {
						fida.push_back(oldidx + idx2 + it.x);
						fida.push_back(oldidx + idx2 + it.z);
						fida.push_back(oldidx + idx2 + it.y);
						fs.push_back(3);
					}
				}
			}
		}
		auto cidx = dva.size() / 3;
#if 1
		if (vh0.size() > 1) {
			idx = cidx;
			auto length = vh0.size();
			for (size_t i = 0; i < length; i++)
			{
				auto it = vh0[i];
				auto v2 = it + d;
				it += pos1;
				v2 += pos1;
				dva.push_back(it.x);
				dva.push_back(it.y);
				dva.push_back(it.z);
				it = v2;
				dva.push_back(it.x);
				dva.push_back(it.y);
				dva.push_back(it.z);

			}
			for (size_t i = 1; i < length; i++)
			{
				fida.push_back(idx);
				fida.push_back(idx + 2);
				fida.push_back(idx + 3);
				fida.push_back(idx + 1);
				fs.push_back(4);
				idx += 2;
			}
		}
#endif
#if 1
		// 封闭端面 
		auto cct = vh.size();
		auto cct0 = vh0.size();
		if (cct0 > 0 && cct > 0)
		{
			//左/前
			if (style->bottom_thickness > 0)
			{
				fida.push_back(oldidx);					// 前右
				fida.push_back(cidx);
				fida.push_back(cidx + cct0 * 2 - 2);// 前左
				fida.push_back(oldidx + cidx - 2);
				fs.push_back(4);

				// 内
				fida.push_back(cidx);
				fida.push_back(cidx + 1);
				fida.push_back(cidx + cct0 * 2 - 1);// 后
				fida.push_back(cidx + cct0 * 2 - 2);// 前左
				fs.push_back(4);
				// 外
				fida.push_back(oldidx);					// 前右
				fida.push_back(oldidx + cidx - 2); //前
				fida.push_back(oldidx + cidx - 1); //后下
				fida.push_back(oldidx + 1);//后
				fs.push_back(4);
			}
			else
			{
				fida.push_back(oldidx);
				fida.push_back(cidx);
				fida.push_back(cidx + 1);
				fida.push_back(oldidx + 1);
				fs.push_back(4);
			}

			for (size_t i = 0; i < cct - 1; i++)
			{
				fida.push_back(oldidx + i * 2);
				fida.push_back(oldidx + (i + 1) * 2);
				fida.push_back(cidx + (i + 1) * 2);
				fida.push_back(cidx + i * 2);
				fs.push_back(4);
			}
			//右/后
			if (style->bottom_thickness > 0)
			{
				fida.push_back(cidx + 1);
				fida.push_back(oldidx + 1);
				fida.push_back(oldidx + cidx - 1);//后
				fida.push_back(cidx + cct0 * 2 - 1);
				fs.push_back(4);
			}
			else
			{
				fida.push_back(oldidx + cidx - 2);//前
				fida.push_back(oldidx + cidx - 1);//后
				fida.push_back(cidx + cct0 * 2 - 1);
				fida.push_back(cidx + cct0 * 2 - 2);
				fs.push_back(4);
			}
			oldidx++;
			cidx++;
			for (size_t i = 0; i < cct - 1; i++)
			{
				fida.push_back(oldidx + i * 2);
				fida.push_back(cidx + i * 2);
				fida.push_back(cidx + (i + 1) * 2);
				fida.push_back(oldidx + (i + 1) * 2);
				fs.push_back(4);
			}
		}
		if (style->type.y < 0)
		{
			auto ns = fida.size();
			auto d = fida.data();
			auto ps = fs.data();
			if (rc > 0)
				ns = rc;
			for (size_t i = 0; i < ns; )
			{
				// 正序 0 1 2 3
				// 反序 0 3 2 1	
				if (*ps == 4)
				{
					std::swap(d[i + 1], d[i + 3]);
				}
				else if (*ps == 3) {
					// 0 1 2改成0 2 1
					std::swap(d[i + 1], d[i + 2]);
				}
				i += *ps;
				ps++;
			}
		}
#endif

		return;
	}



}
//! gp


path_v::path_v()
{
}

path_v::~path_v()
{
	//printf("%d\n", _data.size());
}
void path_v::swap(path_v& v) {
	v._data.swap(_data);
	std::swap(v._box, _box);
	std::swap(v._pos, _pos);
}
path_v::vertex_t* path_v::data() {
	return _data.empty() ? nullptr : _data.data();
}

size_t path_v::size()
{
	return _data.size();
}
size_t path_v::dsize()
{
	return _data.size() * sizeof(path_v::vertex_t);
}
size_t path_v::mcount()
{
	size_t c = 0;
	for (auto& it : _data)
	{
		if (it.type == vtype_e::e_vmove)c++;
	}
	return c;
}
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

glm::vec2 getbox2t(glm::vec4 box, int t)
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

glm::mat4 mkmat3(glm::vec2 pos, glm::vec2 scale, float rotate, glm::vec2 rotate_pos)
{
	// 平移
	auto tm = glm::translate(glm::mat4(1.0f), glm::vec3(pos, .0f));
	// 缩放
	auto sm = glm::mat4(1.0);
	assert((scale.x * scale.y) > 0);
	if ((scale.x * scale.y) > 0)
	{
		sm = glm::scale(glm::mat4(1.0f), glm::vec3(scale, 1.0));
	}
	// 旋转
	glm::vec3 rpos(rotate_pos, 0.0f);
	auto rm = glm::translate(glm::mat4(1.0f), rpos) * glm::mat4_cast(glm::quat(glm::radians(glm::vec3(0, 0, rotate))))
		* glm::translate(glm::mat4(1.0f), -rpos);
	return tm * sm * rm;
}

void path_v::moveTo(const glm::vec2& p)
{
	auto v2ss = sizeof(glm::vec2);
	vertex_t v = {};
	v.type = vtype_e::e_vmove;
	v.p = p;
	_data.push_back(v);
}
void path_v::quadraticCurveTo(const glm::vec2& cp0, const glm::vec2& p)
{
	vertex_t v = {};
	v.type = vtype_e::e_vcurve;
	v.p = p;
	v.c = cp0;
	_data.push_back(v);
	//tobox(cp0, box);
	//tobox(p, box);
}
void path_v::bezierCurveTo(const glm::vec2& cp0, const glm::vec2& cp1, const glm::vec2& p)
{
	vertex_t v = {};
	v.type = vtype_e::e_vcubic;
	v.p = p;
	v.c = cp0;
	v.c1 = cp1;
	_data.push_back(v);
	//tobox(cp0, box);
	//tobox(cp1, box);
	//tobox(p, box);
}

void path_v::curveTo(const glm::vec2& cp0, const glm::vec2& p)
{
	vertex_t v = {};
	v.type = vtype_e::e_vcurve;
	v.p = p;
	v.c = cp0;
	_data.push_back(v);
	//tobox(cp0, box);
	//tobox(p, box);
}
void path_v::cubicTo(const glm::vec2& cp0, const glm::vec2& cp1, const glm::vec2& p)
{
	vertex_t v = {};
	v.type = vtype_e::e_vcubic;
	v.p = p;
	v.c = cp0;
	v.c1 = cp1;
	_data.push_back(v);
	//tobox(cp0, box);
	//tobox(cp1, box);
	//tobox(p, box);
}

void path_v::lineTo(const glm::vec2& p)
{
	vertex_t v = {};
	v.type = vtype_e::e_vline;
	v.p = p;
	_data.push_back(v);
	//tobox(p, box);
}


void path_v::moveTo(double x, double y)
{
	moveTo({ x,y });
}
void path_v::lineTo(double x, double y)
{
	lineTo({ x,y });
}
void path_v::curveTo(double cx, double cy, double c1x, double c1y, double x, double y)
{
	cubicTo({ cx,cy }, { c1x,c1y }, { x,y });
}
void path_v::lineTo2(double x, double y)
{
	auto pts = _data.rbegin();
	double px, py, dx, dy;
	if (_data.size()) {
		auto pxy = pts;
		px = pxy->p.x;
		py = pxy->p.y;
		dx = x - px;
		dy = y - py;
		vertex_t v = {};
		v.type = vtype_e::e_vcubic;
		v.c.x = (px + dx / 3.0f); v.c.y = (py + dy / 3.0f);
		v.c1.x = (x - dx / 3.0f); v.c1.y = (y - dy / 3.0f);
		v.p.x = x;
		v.p.y = y;
		_data.push_back(v);
		//tobox(v.p, box);
	}
}
void path_v::lineTo2(const glm::vec2& p)
{
	lineTo2(p.x, p.y);
}
void path_v::cubicBezTo(double cpx1, double cpy1, double cpx2, double cpy2, double x, double y)
{
	cubicTo({ cpx1,cpy1 }, { cpx2,cpy2 }, { x,y });
}


#define c_KAPPA90 (0.5522847493f)	// Length proportional to radius of a cubic bezier handle for 90deg arcs.



void path_v::addCircle(const glm::vec2& c, double r)
{
	double cx = c.x, cy = c.y;
	if (r > 0.0f) {
		moveTo(cx + r, cy);
		cubicBezTo(cx + r, cy + r * c_KAPPA90, cx + r * c_KAPPA90, cy + r, cx, cy + r);
		cubicBezTo(cx - r * c_KAPPA90, cy + r, cx - r, cy + r * c_KAPPA90, cx - r, cy);
		cubicBezTo(cx - r, cy - r * c_KAPPA90, cx - r * c_KAPPA90, cy - r, cx, cy - r);
		cubicBezTo(cx + r * c_KAPPA90, cy - r, cx + r, cy - r * c_KAPPA90, cx + r, cy);
	}
}
void path_v::addCircle(const glm::vec2* c, size_t n, double r)
{
	if (c && r > 0.0f)
	{
		for (size_t i = 0; i < n; i++)
		{
			addCircle(*c, r); c++;
		}
	}
}
void path_v::addCircle(const glm::vec3* c, size_t n)
{
	if (c)
	{
		for (size_t i = 0; i < n; i++)
		{
			if (c->z > 0.0f)
				addCircle(*c, c->z);
			c++;
		}
	}
}

void path_v::addEllipse(const glm::vec2& c, const glm::vec2& r)
{
	double cx = c.x, cy = c.y, rx = r.x, ry = r.y;
	if (rx > 0.0f && ry > 0.0f) {
		moveTo(cx + rx, cy);
		cubicBezTo(cx + rx, cy + ry * c_KAPPA90, cx + rx * c_KAPPA90, cy + ry, cx, cy + ry);
		cubicBezTo(cx - rx * c_KAPPA90, cy + ry, cx - rx, cy + ry * c_KAPPA90, cx - rx, cy);
		cubicBezTo(cx - rx, cy - ry * c_KAPPA90, cx - rx * c_KAPPA90, cy - ry, cx, cy - ry);
		cubicBezTo(cx + rx * c_KAPPA90, cy - ry, cx + rx, cy - ry * c_KAPPA90, cx + rx, cy);
	}
}
void setrm(double& rx, double& ry, double w, double h)
{
	if (rx < 0.0f && ry > 0.0f) rx = ry;
	if (ry < 0.0f && rx > 0.0f) ry = rx;
	if (rx < 0.0f) rx = 0.0f;
	if (ry < 0.0f) ry = 0.0f;
	if (rx > w / 2.0f) rx = w / 2.0f;
	if (ry > h / 2.0f) ry = h / 2.0f;
}

void path_v::addRect(const glm::vec4& a, const glm::vec2& r)
{
	double x = a.x, y = a.y, w = a.z, h = a.w, rx = r.x, ry = r.y;
	setrm(rx, ry, w, h);
	if (w != 0.0f && h != 0.0f) {

		if (rx < 0.00001f || ry < 0.0001f) {
			moveTo(x, y);
			lineTo(x + w, y);
			lineTo(x + w, y + h);
			lineTo(x, y + h);
			lineTo(x, y); //closePath
		}
		else {
			moveTo(x + rx, y);
			lineTo(x + w - rx, y);
			cubicBezTo(x + w - rx * (1 - c_KAPPA90), y, x + w, y + ry * (1 - c_KAPPA90), x + w, y + ry);
			lineTo(x + w, y + h - ry);
			cubicBezTo(x + w, y + h - ry * (1 - c_KAPPA90), x + w - rx * (1 - c_KAPPA90), y + h, x + w - rx, y + h);
			lineTo(x + rx, y + h);
			cubicBezTo(x + rx * (1 - c_KAPPA90), y + h, x, y + h - ry * (1 - c_KAPPA90), x, y + h - ry);
			lineTo(x, y + ry);
			cubicBezTo(x, y + ry * (1 - c_KAPPA90), x + rx * (1 - c_KAPPA90), y, x + rx, y);
			lineTo(x, y);
		}
	}
}
void path_v::add_lines(const glm::vec2* d, size_t size, bool isclose)
{
	if (d && size > 0)
	{
		_data.reserve(_data.size() + size);
		moveTo(*d);
		for (size_t i = 1; i < size; i++)
		{
			lineTo(d[i]);
		}
		if (isclose && *d != d[size - 1])
		{
			lineTo(*d);
		}
	}
}
void path_v::add_lines(const glm::dvec2* d, size_t size, bool isclose)
{
	if (d && size > 0)
	{
		_data.reserve(_data.size() + size);
		moveTo(*d);
		for (size_t i = 1; i < size; i++)
		{
			lineTo(d[i]);
		}
		if (isclose && *d != d[size - 1])
		{
			lineTo(*d);
		}
	}
}
path_v::vertex_t* path_v::getline2(int x, size_t* px)
{
	auto n = _data.size();
	auto p = _data.data();
	int xx = 0;
	vertex_t* r = 0;
	for (size_t i = 0; i < n; i++, p++)
	{
		if (p->type == vtype_e::e_vmove)
		{
			if (xx == x)
			{
				if (px)
					*px = i;
				r = p;
				break;
			}
			xx++;
		}
	}
	return r;
}

struct pvt_t
{
	path_v::vertex_t* first = 0, * second = 0;
	size_t n = 0;
};
pvt_t get_idxlines(std::vector<path_v::vertex_t>& data, size_t idx, size_t count)
{
	auto n = data.size();
	auto p = data.data();
	int xx = 0;
	path_v::vertex_t* r = 0, * e = p + n;
	for (size_t i = 0; i < n; i++, p++)
	{
		if (p->type == path_v::vtype_e::e_vmove)
		{
			if (r)
			{
				count--;
				e = p;
				if (count < 1)
					break;
			}
			else if (xx == idx)
			{
				r = p;
			}
			xx++;
		}
	}
	size_t cn = e - r;
	return { r, e , cn };
}
//反转路径内部一条线
#if 1
void path_v::reverse1(int idx)
{
#if 0
	auto t = get_idxlines(_data, idx, 1);
	if (t.first && t.n)
	{
		assert(t.first->type == vtype_e::e_vmove);
		std::vector<vertex_t> tv;
		tv.resize(t.n);
		memcpy(tv.data(), t.first, sizeof(vertex_t) * t.n);
		//std::reverse(t.first, t.second);
		std::reverse(tv.begin(), tv.end());
		auto p = tv.data();
		for (size_t i = 0; i < tv.size(); i++, p++)
		{
			std::swap(p->c, p->c1);
			p->p = (p + 1)->p;
		}
		auto ted = *tv.rbegin();
		ted.type = vtype_e::e_vmove;
		tv.insert(tv.begin(), ted);
		tv.pop_back();
		memcpy(t.first, tv.data(), sizeof(vertex_t) * t.n);
	}
#endif
}
void path_v::reverse_all()
{
#if 0
	auto mc = mcount();
	for (size_t i = 0; i < mc; i++)
	{
		reverse1(i);
	}
#endif
	//std::reverse(_data.begin(), _data.end());
}
#endif




void v2m3(glm::vec2& v, const glm::mat3* m)
{
	glm::vec3 ps(v, 1);
	ps = (*m) * ps;
	v = ps;
}
glm::vec2 v2m3(const glm::vec2& v, const glm::mat3& m) {
	glm::vec3 ps(v, 1);
	return m * ps;
}
void path_v::set_mat(const glm::mat3& m)
{
	v2m3(_pos, &m);
	for (auto& it : _data) {
		v2m3(it.p, &m);
		if (it.type == vtype_e::e_vcurve)
		{
			v2m3(it.c, &m);
		}
		if (it.type == path_v::vtype_e::e_vcubic)
		{
			v2m3(it.c, &m);
			v2m3(it.c1, &m);
		}
	}
	cbox = true;
}

glm::vec2 path_v::get_size()
{
	glm::vec2 ret = { _box.z - _box.x,	_box.w - _box.y };// xy最小，zw最大
	if (!(ret.x > 0 && ret.y > 0))
	{
		mkbox();
		//ret = { _box.z - _box.x,	_box.w - _box.y };		
	}
	ret = { _box.z, _box.w };
	return ret;
}
glm::vec4 path_v::mkbox() {
	glm::vec4 bx = { INT_MAX,INT_MAX,INT_MIN,INT_MIN };
	auto ds = _data.size();
	if (ds > 3)
	{
		glm::vec2 k[] = { _data[ds - 1].p,_data[ds - 2].p };
		if (k[0] == k[1])
		{
			_data.pop_back();
		}
	}
	for (auto& it : _data) {
		tobox(it.p, bx);
		if (it.type == vtype_e::e_vcurve || it.type == vtype_e::e_vcubic)
			tobox(it.c, bx);
		if (it.type == vtype_e::e_vcubic)
			tobox(it.c1, bx);
	}
	_box = bx;
	return bx;
}
glm::vec4 path_v::mkbox_npos() {
	glm::vec4 r = {};
	glm::vec4 bx = { INT_MAX,INT_MAX,INT_MIN,INT_MIN };
	auto ds = _data.size();
	float m = 20;
	if (ds > 2)
	{
		glm::vec2 k[] = { _data[ds - 1].p,_data[ds - 2].p };
		if (k[0] == k[1])
		{
			_data.pop_back();
		}
	}
	glm::vec2 ltp = {};
	for (auto& it : _data) {
		tobox(it.p, bx);
		if (it.type == vtype_e::e_vcurve)
		{
			cubic_v cv = {};
			cv.p0 = ltp;
			cv.p1 = it.c;
			cv.p2 = it.c;
			cv.p3 = it.p;
			c2to3(cv);
			auto vb = get_bezier_t<glm::vec2>(&cv, 1, m);
			if (vb.size())
			{
				for (auto vt : vb) {
					tobox(vt, bx);
				}
			}
			else {
				tobox(it.c, bx);
			}
		}
		if (it.type == path_v::vtype_e::e_vcubic)
		{
			cubic_v cv = {};
			cv.p0 = ltp;
			cv.p1 = it.c;
			cv.p2 = it.c1;
			cv.p3 = it.p;
			auto vb = get_bezier_t<glm::vec2>(&cv, 1, m);
			if (vb.size())
			{
				for (auto vt : vb) {
					tobox(vt, bx);
				}
			}
			else {
				tobox(it.c, bx);
				tobox(it.c1, bx);
			}
		}
		ltp = it.p;
	}
	return bx;
}
void path_v::incpos(const glm::vec2& p) {
	for (auto& it : _data) {
		it.p += p;
		it.c += p;
		it.c1 += p;
	}
}
void path_v::mxfy(double fy)
{
	for (auto& d : _data) {
		d.p.y *= fy;
		d.c.y *= fy;
		d.c1.y *= fy;
	}
}
void path_v::add(const path_v::vertex_t& v)
{
	_data.push_back(v);
}
void path_v::insert(size_t idx, const vertex_t& v)
{
	if (idx > _data.size())
		_data.push_back(v);
	else
		_data.insert(_data.begin() + idx, v);
}
void path_v::erase(size_t idx)
{
	if (idx > _data.size())
	{
		_data.pop_back();
	}
	else
	{
		_data.erase(_data.begin() + idx);
	}
}
void path_v::set_data(const path_v::vertex_t* d, size_t size)
{
	int64_t n = size / sizeof(path_v::vertex_t);
	if (d && n > 0)
	{
		_data.resize(n);
		memcpy(_data.data(), d, n * sizeof(path_v::vertex_t));
	}
}
void path_v::set_data(const tinypath_t* d, int count) {
	if (!d || !d->v || d->count < 3 || count < 1)return;
	size_t ct = 0;

	for (size_t i = 0; i < count; i++)
	{
		ct += d[i].count;
		_baseline = std::max(_baseline, d[i].baseline);
	}
	{
		_data.resize(ct);
		auto length = ct;
		auto pd = _data.data();
		for (size_t j = 0; j < count; j++) {
			auto& it = d[j];
			auto pt = (vertex_v2f*)it.v;
			for (size_t i = 0; i < it.count; i++, pd++)
			{
				pd->type = (vtype_e)pt[i].type;
				pd->p = pt[i].p;
				pd->c = pt[i].c;
				pd->c1 = pt[i].c1;
			}
		}
	}
}

void path_v::add(path_v* p)
{
	if (p && p->_data.size() > 1)
	{
		auto n = _data.size();
		auto pn = p->_data.size();
		_data.resize(pn + n);
		memcpy(_data.data() + n, p->_data.data(), sizeof(vertex_t) * pn);
	}
}

void path_v::closePath() {
	if (_data.size() < 2)return;
	glm::vec2 a = _data.rbegin()->p;
	for (auto it = _data.rbegin(); it != _data.rend(); it++)
	{
		if (it->type == path_v::vtype_e::e_vmove)
		{
			auto e = glm::equal(it->p, a);
			if (!e.x || !e.y)
			{
				lineTo(it->p);
			}
			break;
		}
	}

}

void getlineptr(path_v::vertex_t* d, int n, std::vector<glm::vec2>& o, double fpn)
{
	std::vector<glm::vec2>& bez = o;
	auto p = d;
	for (size_t i = 0; i < n; i++, p++)
	{
		switch (p->type)
		{
		case path_v::vtype_e::e_vmove:
		case path_v::vtype_e::e_vline:
		{
			bez.push_back(p->p);
		}break;
		case path_v::vtype_e::e_vcurve:
		{

			bez.push_back(p->p);
		}break;
		case path_v::vtype_e::e_vcubic:
		{

			bez.push_back(p->p);
			auto vb = get_bezier(bez.data(), bez.size(), fpn, true);
		}break;
		default:
			break;
		}
	}

}



int path_v::get_expand(float width, path_v* opt)
{
	std::vector<glm::vec2> tv, ot;
	auto  length = _data.size();
	tv.reserve(length * 3);
	for (size_t i = 0; i < length; i++)
	{
		auto& it = _data[i];
		auto type = (vtype_e)it.type;
		switch (type)
		{
		case path_v::vtype_e::e_vmove:
			if (tv.size())
			{
				expand_polygon(tv.data(), tv.size(), width, ot);
				tv.clear();
			}
			tv.push_back(it.p);
			break;
		case path_v::vtype_e::e_vline:
			tv.push_back(it.p);
			break;
		case path_v::vtype_e::e_vcurve:
			tv.push_back(it.c);
			tv.push_back(it.p);
			break;
		case path_v::vtype_e::e_vcubic:
			tv.push_back(it.c);
			tv.push_back(it.c1);
			tv.push_back(it.p);
			break;
		default:
			break;
		}
	}
	if (tv.size())
	{
		expand_polygon(tv.data(), tv.size(), width, ot);
	}
	if (ot.empty())
	{
		return 0;
	}
	if (!opt)opt = new path_v();
	auto pt = ot.data();
	if (opt)
	{
		for (size_t i = 0; i < length; i++)
		{
			auto it = _data[i];
			auto type = (vtype_e)it.type;
			switch (type)
			{
			case path_v::vtype_e::e_vmove:
			case path_v::vtype_e::e_vline:
				break;
			case path_v::vtype_e::e_vcurve:
				it.c = *pt;
				pt++;
				break;
			case path_v::vtype_e::e_vcubic:
				it.c = *pt;
				pt++;
				it.c1 = *pt;
				pt++;
				break;
			default:
				break;
			}
			it.p = *pt;
			pt++;
			opt->add(it);
		}
	}
	else
	{
		for (size_t i = 0; i < length; i++)
		{
			auto& it = _data[i];
			auto type = (vtype_e)it.type;
			switch (type)
			{
			case path_v::vtype_e::e_vmove:
			case path_v::vtype_e::e_vline:
				break;
			case path_v::vtype_e::e_vcurve:
				it.c = *pt;
				pt++;
				break;
			case path_v::vtype_e::e_vcubic:
				it.c = *pt;
				pt++;
				it.c1 = *pt;
				pt++;
				break;
			default:
				break;
			}
			it.p = *pt;
			pt++;
		}
	}
	return 0;
}


void mkivec_round(std::vector<glm::vec2>& tv1)
{
	std::vector<glm::vec2> tt;
	tt.reserve(tv1.size());
	tt.push_back(tv1[0]);
	glm::ivec2 last = glm::round(tv1[0]);
	for (auto it : tv1)
	{
		glm::ivec2 xd = glm::round(it);
		if (xd != last)
		{
			last = xd;
			tt.push_back(it);
		}
	}
	if (tt[0] != tt[tt.size() - 1])
	{
		tt.push_back(tt[0]);
	}
	tv1.swap(tt);

}
void mkivec_round(std::vector<glm::vec3>& tv1)
{
	std::vector<glm::vec3> tt;
	tt.reserve(tv1.size());
	tt.push_back(tv1[0]);
	glm::ivec3 last = glm::round(tv1[0]);
	for (auto it : tv1)
	{
		glm::ivec3 xd = glm::round(it);
		if (xd != last)
		{
			last = xd;
			tt.push_back(it);
		}
	}
	if (tt[0] != tt[tt.size() - 1])
	{
		tt.push_back(tt[0]);
	}
	tv1.swap(tt);

}


template<class T>
void doflatten(T* fp)
{
	size_t length = fp->n;
	auto p = fp->first;
	float objspace_flatness_squared = 0.1;
	std::vector<glm::vec2> nv;
	std::vector<glm::vec2> points;
	std::vector<std::vector<glm::vec2>> pv, pv1;
	std::vector<glm::ivec2> pvc;

	for (size_t i = 0; i < length; i++, p++)
	{
		auto m = fp->mc;
		auto p0 = p - 1;
		points.clear();
		switch ((path_v::vtype_e)p->type)
		{
		case path_v::vtype_e::e_vmove:
		case path_v::vtype_e::e_vline:
		{
			points.push_back(p->p);
		}break;
		case path_v::vtype_e::e_vcurve:
		{
			cubic_v cv = {};
			cv.p0 = p0->p;
			cv.p1 = p->c;
			cv.p2 = p->c;
			cv.p3 = p->p;
			c2to3(cv);
			if (fp->mlen > 0) {
				quadratic_v q = { p0->p ,p->c,p->p };
				double ql = quadraticLength(&q, fp->mc);
				m = ql / fp->mlen;
				if (m < 2)
					m = 2;
			}
			points = get_bezier_t<glm::vec2>(&cv, 1, m);
			pvc.push_back({ i,i + points.size() });
		}break;
		case path_v::vtype_e::e_vcubic:
		{
			cubic_v cv = {};
			cv.p0 = p0->p;
			cv.p1 = p->c;
			cv.p2 = p->c1;
			cv.p3 = p->p;
			if (fp->mlen > 0) {
				double ql = cubicLength(&cv, fp->mc);
				m = ql / fp->mlen;
				if (m < 2)
					m = 2;
			}
			points = get_bezier_t<glm::vec2>(&cv, 1, m);
			pvc.push_back({ i,i + points.size() });
		}break;
		default:
			break;
		}
		if (points.size())
		{
			if (p0->p == points[0])
			{
				points.erase(points.begin());
			}
			pv.push_back(points);
		}
	}
	length = pv.size();
	points.clear();
	for (size_t i = 0; i < length; i++)
	{
		auto& it = pv[i];
		auto nc = it.size();
		for (auto& vt : it) {
			points.push_back(vt);
		}
	}
	if (points.size() > 2)
	{
		length = points.size();
		points.push_back(points[1]);
		std::vector<glm::ivec3> av, tav;
		// 计算角度
		for (size_t i = 1; i < length; i++)
		{
			auto& it0 = points[i - 1];
			auto& it = points[i];
			auto& it1 = points[i + 1];
			glm::ivec2 aa = gp::angle_v3(it0, it, it1);
			auto dd = glm::distance(it0, it);
			auto dd1 = glm::distance(it, it1);
			auto ang = aa.x;
			bool has = (aa.y < 0 && aa.x < fp->angle);
			if (aa.y > 0 && ang > 180 && !has)
			{
				ang = 360 - ang;
				//ang = abs(ang); 
				has = ang < fp->angle;
			}
			if (has && (dd < fp->dist || dd1 < fp->dist))
			{
				av.push_back({ aa,i });
			}
			tav.push_back({ aa,i });
		}
		auto tpv = points;
		length = points.size() - 1;
		std::vector<glm::vec3> ptvs;
		ptvs.resize(length);
		for (size_t i = 0; i < length; i++)
		{
			ptvs[i] = { points[i],0 };
		}
		for (auto& it : av) {
			length = ptvs.size();
			double dd = 0.0;
			size_t idx = it.z;
			{
				auto xf = it.z;
				if (xf == ptvs.size() - 1)
					xf = 0;
				for (size_t i = xf; i < length; i++)
				{
					auto& it0 = points[i];
					auto& it1 = points[i + 1];
					dd += glm::distance(it1, it0);
					if (dd > fp->dist)
					{
						idx = i;
						break;
					}
				}
				if (idx != xf)
				{
					for (size_t x = xf + 1; x < idx; x++)
					{
						ptvs[x].z = 1;
					}
				}
			}
			idx = it.z;
			dd = 0;
			for (size_t i = it.z; i > 1; i--)
			{
				auto& it0 = points[i];
				auto& it1 = points[i - 1];
				dd += glm::distance(it1, it0);
				if (dd > fp->dist)
				{
					idx = i;
					break;
				}
			}
			if (idx != it.z)
			{
				for (size_t x = idx + 1; x < it.z; x++)
				{
					ptvs[x].z = 1;
				}
			}
		}
		nv.reserve(ptvs.size());
		int xk = 0;
		for (auto& it : ptvs) {
			if (it.z < 1)
				nv.push_back(it);
		}
	}
	if (nv.size())
	{
		auto pos = fp->flatten->size();
		fp->flatten->resize(pos + nv.size());
		memcpy(fp->flatten->data() + pos, nv.data(), nv.size() * sizeof(glm::vec2));
	}
}

int path_v::get_flatten(size_t idx, size_t count, int m, float ml, float ds, std::vector<glm::vec2>* flatten)
{
	if (!flatten || idx > mcount() || _data.size() < 1 || m < 1)return -1;
	auto t = get_idxlines(_data, idx, count);
	flatten_t fp = {};
	fp.flatten = flatten;
	fp.mc = m;
	fp.mlen = ml;
	fp.n = t.n;
	fp.first = t.first;
	fp.dist = ds;
	fp.angle = angle;
	doflatten(&fp);
	return 0;
}
namespace gp {
	int get_flatten(tinypath_t* p, int m, float ml, float ds, std::vector<glm::vec2>* flatten)
	{
		if (!flatten || !p || p->count < 1 || m < 1)return -1;
		tinyflatten_t fp = {};
		fp.flatten = flatten;
		fp.mc = m;
		fp.mlen = ml;
		fp.n = p->count;
		fp.first = (vertex_v2f*)p->v;
		fp.dist = ds;
		fp.angle = 160;
		doflatten(&fp);
		return 0;
	}
}

int path_v::get_expand_flatten(size_t idx, float width, int segments, float ml, float ds, int type, std::vector<std::vector<glm::vec2>>* ots, std::vector<int>* ccwv)
{
	bool oldfp = true;
	auto ct = mcount();
	if (_data.size() < 1 || segments < 1)return -1;
	PathsD pd;
	if (idx < _data.size())
	{
		ct = idx + 1;
	}
	else { idx = 0; }
	std::vector<PointD> pv;
	std::vector<glm::vec2> flatten, ot;
	for (size_t x = idx; x < ct; x++)
	{
		pv.clear();
		flatten.clear();
		auto t = get_idxlines(_data, x, 1);
		flatten_t fp = {};
		fp.flatten = &flatten;
		fp.mc = segments;
		fp.mlen = ml;
		fp.n = t.n;
		fp.first = t.first;
		fp.dist = ds;
		fp.angle = angle;
		doflatten(&fp);//细分曲线
		if (abs(width) < 2 || oldexp)
		{
			ot.clear();
			expand_polygon(flatten.data(), flatten.size(), width, ot);
			ots->push_back(ot);
		}
		else
		{
			pv.resize(flatten.size());
			for (size_t i = 0; i < flatten.size(); i++)
			{
				pv[i] = { flatten[i].x, flatten[i].y };
			}
			// 判断是逆时针
			bool ccw = IsPositive(pv);
			if (ccwv)
				ccwv->push_back(ccw);
			pd.push_back(std::move(pv));
		}
	}
	if (width != 0 && pd.size())
	{
		//Square=0, Round=1, Miter=2
		if (type > 2 || type < 0)
		{
			type = 2;
		}

		// 扩展线段
		auto rv = InflatePaths(pd, width, (JoinType)type, EndType::Polygon);
		if (rv.size() && rv[0].size())
		{
			pd.swap(rv);
		}
	}

	for (const auto& path : pd) {
		flatten.clear();
		flatten.reserve(path.size());
		for (auto& it : path)
		{
			glm::vec2 pt = { it.x ,it.y };
			flatten.push_back(pt);
		}
		if (flatten.size() && flatten[0] != flatten[flatten.size() - 1])
		{
			flatten.push_back(flatten[0]);
		}
		ots->push_back(flatten);
	}

	return 0;
}
int path_v::get_expand_flatten2(float expand, float scale, int segments, float ml, float ds, int type, std::vector<std::vector<glm::vec2>>* ots, bool is_round, bool is_close)
{
	auto ct = mcount();
	if (_data.size() < 1 || segments < 1)return -1;
	PathsD pd;
	std::vector<PointD> pv;
	std::vector<glm::vec2> flatten, ot;
	for (size_t x = 0; x < ct; x++)
	{
		pv.clear();
		flatten.clear();
		auto t = get_idxlines(_data, x, 1);
		flatten_t fp = {};
		fp.flatten = &flatten;
		fp.mc = segments;
		fp.mlen = ml;
		fp.n = t.n;
		fp.first = t.first;
		fp.dist = ds;
		fp.angle = angle;
		doflatten(&fp);//细分曲线
		{
			pv.resize(flatten.size());
			for (size_t i = 0; i < flatten.size(); i++)
			{
				pv[i] = { flatten[i].x, flatten[i].y };
			}
			// 判断是逆时针
			bool ccw = IsPositive(pv);
			pd.push_back(std::move(pv));
		}
	}
	if (expand != 0 && pd.size())
	{
		//Square=0, Round=1, Miter=2
		if (type > 2 || type < 0)
		{
			type = 2;
		}
		// 扩展线段
		auto rv = InflatePaths(pd, expand, (JoinType)type, EndType::Polygon);
		if (rv.size() && rv[0].size())
		{
			pd.swap(rv);
		}
	}

	for (const auto& path : pd) {
		flatten.clear();
		flatten.reserve(path.size());
		for (auto& it : path)
		{
			glm::vec2 pt = { it.x ,it.y };
			flatten.push_back(pt);
		}
		if (is_round)
			gp::mkivec_round(flatten);
		if (abs(scale) != 0)
		{
			ot.clear();
			expand_polygon(flatten.data(), flatten.size(), scale, ot);
			flatten.swap(ot);
		}
		if (is_close && flatten.size() && flatten[0] != flatten[flatten.size() - 1])
		{
			flatten.push_back(flatten[0]);
		}
		ots->push_back(flatten);
	}
	return 0;
}

int path_v::get_expand_flatten3(float expand, float scale, int segments, float ml, float ds, int type, int etype, std::vector<std::vector<glm::vec2>>* ots, bool is_close)
{
	auto ct = mcount();
	if (_data.size() < 1 || segments < 1)return -1;
	PathsD pd;
	std::vector<PointD> pv;
	std::vector<glm::vec2> flatten, ot;
	for (size_t x = 0; x < ct; x++)
	{
		pv.clear();
		flatten.clear();
		auto t = get_idxlines(_data, x, 1);
		flatten_t fp = {};
		fp.flatten = &flatten;
		fp.mc = segments;
		fp.mlen = ml;
		fp.n = t.n;
		fp.first = t.first;
		fp.dist = ds;
		fp.angle = angle;
		doflatten(&fp);//细分曲线
		{
			pv.resize(flatten.size());
			for (size_t i = 0; i < flatten.size(); i++)
			{
				pv[i] = { flatten[i].x, flatten[i].y };
			}
			// 判断是逆时针
			bool ccw = IsPositive(pv);
			pd.push_back(std::move(pv));
		}
	}
	if (expand != 0 && pd.size())
	{
		//Square=0, Round=1, Miter=2
		if (type > 2 || type < 0)
		{
			type = 2;
		}
		// 扩展线段
		auto rv = InflatePaths(pd, expand, (JoinType)type, (EndType)etype);
		if (rv.size() && rv[0].size())
		{
			pd.swap(rv);
		}
	}

	for (const auto& path : pd) {
		flatten.clear();
		flatten.reserve(path.size());
		for (auto& it : path)
		{
			glm::vec2 pt = { it.x ,it.y };
			flatten.push_back(pt);
		}
		if (is_close && flatten.size() && flatten[0] != flatten[flatten.size() - 1])
		{
			flatten.push_back(flatten[0]);
		}
		ots->push_back(flatten);
	}
	return 0;
}


int path_v::get_expand_flatten(float width, int segments, int type, std::vector<std::vector<glm::vec2>>* ots)
{
	auto ct = ots->size();
	std::vector<PointD> pv;
	PathsD pd;
	for (size_t x = 0; x < ct; x++)
	{
		pv.clear();
		std::vector<glm::vec2>& flatten = ots->at(x);
		pv.resize(flatten.size());
		for (size_t i = 0; i < flatten.size(); i++)
		{
			pv[i] = { flatten[i].x, flatten[i].y };
		}
		// 判断是逆时针
		bool ccw = IsPositive(pv);
		pd.push_back(std::move(pv));
	}
	if (width != 0)
	{
		//Square=0, Round=1, Miter=2
		if (type > 2 || type < 0)
		{
			type = 2;
		}
		// 扩展线段
		auto rv = InflatePaths(pd, width, (JoinType)type, EndType::Polygon);
		if (rv.size() && rv[0].size())
		{
			pd.swap(rv);
		}
	}
	if (pd.size() != ct)
	{
		ots->resize(pd.size()); ct = pd.size();
	}
	for (size_t i = 0; i < ct; i++)
	{
		auto path = pd[i];
		std::vector<glm::vec2>& flatten = ots->at(i);
		flatten.clear();
		flatten.reserve(path.size());
		for (auto& it : path)
		{
			glm::vec2 pt = { it.x ,it.y };
			flatten.push_back(pt);
		}
		if (flatten.size() && flatten[0] != flatten[flatten.size() - 1])
		{
			flatten.push_back(flatten[0]);
		}
	}
	return 0;
}
// 判断点是否在多边形内
int pnpoly_aos(int nvert, const float* verts, float testx, float testy) {
	int i, j;
	bool c = 0;
	for (i = 0, j = nvert - 1; i < nvert; j = i++) {
		if (((verts[i * 2 + 1] > testy) != (verts[j * 2 + 1] > testy)) &&
			(testx < (verts[j * 2] - verts[i * 2]) * (testy - verts[i * 2 + 1]) / (verts[j * 2 + 1] - verts[i * 2 + 1]) + verts[i * 2])) {
			c = !c;
		}
	}
	return c;
}
glm::vec2 pnpoly_aos(int nvert, const glm::vec2* verts, const glm::vec2& test) {
	int i, j;
	bool c = 0;
	float d = glm::distance2(test, verts[0]);
	for (i = 0, j = nvert - 1; i < nvert; j = i++) {
		if (((verts[i].y > test.y) != (verts[j].y > test.y)) &&
			(test.x < (verts[j].x - verts[i].x) * (test.y - verts[i].y) / (verts[j].y - verts[i].y) + verts[i].x)) {
			c = !c;
		}
		auto d1 = glm::distance2(test, verts[i]);
		if (d1 < d)
			d = d1;
	}
	return { c, d };
}

int path_v::triangulate(int segments, float ml, float ds, bool pccw, std::vector<glm::vec2>* p, int type)
{
	if (!p)return 0;
	auto& ms = *p;
	auto pos = ms.size();
	auto pt = gp::new_path_node(this, 0, 0, 0, 0, segments, ml, ds);
	std::vector<std::vector<glm::vec2>>  tr, tr1;
	std::vector<glm::vec2>  tv1;
	float z = 0.0;
	gp::fv_it fv = { pt };
	std::vector<PointD> pv;
	for (size_t i = 0; i < fv.count; i++)
	{
		auto n = fv.lengths[i];
		tv1.clear();
		gp::for_vertex(fv.pos, fv.angle, fv.center, n, segments, tv1);
		fv.inc(n);

		pv.resize(tv1.size());
		for (size_t i = 0; i < tv1.size(); i++)
		{
			pv[i] = { tv1[i].x,tv1[i].y };
		}
		bool ccw = IsPositive(pv);
		if (ccw)
			tr1.push_back(tv1);
		else
		{
			tr.push_back(tv1);
		}
	}
	std::vector<glm::vec3> ms3;
	auto nc = tr.size();
	std::map<size_t, std::vector<std::vector<glm::vec2>>> shell;
	for (size_t i = 0; i < nc; i++)
	{
		shell[i].push_back(tr[i]);
	}
	for (auto& it : tr1)
	{
		float dis = -1;
		int64_t c = -1;
		std::vector<glm::vec2> disv;
		for (size_t i = 0; i < nc; i++)
		{
			auto n = tr[i].size();
			auto& st = tr[i];
			glm::vec2 ps = it[0];
			float dis1 = 0;
			auto bd = pnpoly_aos(n, st.data(), ps);// 返回是否在多边形内、最短距离
			disv.push_back(bd);
			if (bd.x > 0)
			{
				if (dis < 0 || bd.y < dis)
				{
					c = i;
					dis = bd.y;
				}
			}
		}
		if (c >= 0)
		{
			shell[c].push_back(it);//holes
		}
	}
	if (type == 1) {
		gp::cmd_plane_t c[1] = {};
		c->opt = &ms3;
		PathsD	solution;
		for (auto& [k, v] : shell)
		{
			solution.clear();
			for (auto& it : v) {
				if (it.size() > 2)
				{
					pv.resize(it.size());
					for (size_t i = 0; i < it.size(); i++)
					{
						pv[i] = { it[i].x,it[i].y };
					}
					solution.push_back(pv);
				}
			}
			if (v.size())
			{
				gp::polygon_ct pct;
				pct.make(&solution, true);
				pct.triangulate(c->opt, 0, pccw);
			}
		}
	}
	else
	{
		for (auto& [k, v] : shell)
		{
			if (v.size())
				gp::constrained_delaunay_triangulation_v(&v, ms3, pccw, z);
		}
	}

	for (auto& it : ms3)
		ms.push_back(it);
	gp::free_path_node(pt);
	return ms.size() - pos;
}


int path_v::get_triangulate_center_line(int segments, float ml, float ds, int is_reverse, const glm::vec2& z2, std::vector<glm::vec3>* ms)
{
	std::vector<glm::vec2> pv, pdt;
	triangulate(segments, ml, ds, 0, &pv);
	auto length = pv.size();
	pdt.reserve(length * 4 * 3);
	std::map<glm::vec2, int> mvi;
	auto t = pv.data();
	float z = z2.x + z2.y;
	float pz = z2.x;
	for (size_t i = 0; i < length; i += 3)
	{
		auto ps = pdt.size();
		// 获取三边中心点
		gp::divide_triangle(t[0], t[1], t[2], &pdt);
		t += 3;
		for (size_t x = 0; x < 3; x++)
		{
			mvi[pdt[x + ps]]++;
		}
	}
	length = pdt.size();
	t = pdt.data();
	auto t0 = pv.data();
	std::vector<std::vector<glm::vec3>> mtv;
	std::vector<glm::vec3> tv;
	std::vector<glm::vec2> bv;
	for (size_t i = 0; i < length; i += 3)
	{
		tv.clear();
		for (size_t x = 0; x < 3; x++, t0++)
		{
			tv.push_back({ *t0,pz });
			auto pt = t++;
			auto it = mvi.find(*pt);
			if (it != mvi.end() && it->second > 1)
			{
				tv.push_back({ *pt,z });
			}
		}

		mtv.push_back(std::move(tv));
	}
	std::vector<glm::vec3> m3;
	for (auto& it : mtv) {
		gp::cdt_pt(it.data(), it.size(), m3, !is_reverse);
	}
	if (ms)
	{
		auto ct = ms->size();
		ms->resize(ct + m3.size());
		memcpy(ms->data() + ct, m3.data(), sizeof(glm::vec3) * m3.size());
	}
	return 0;
}


//angle旋转角度(0-360)
glm::vec2 path_v::rotate_pos(const glm::vec2& pos, const glm::vec2& center, double angle)
{
	double x = pos.x, y = pos.y;				//原始点坐标
	double rx = center.x, ry = center.y;        //旋转中心点坐标
	double nx, ny;								//旋转后的点坐标
	double as, ac;
	as = sin(angle * glm::pi<double>() / 180.0);
	ac = cos(angle * glm::pi<double>() / 180.0);
	nx = rx + ((x - rx) * ac - (y - ry) * as);
	ny = ry + ((x - rx) * as + (y - ry) * ac);
	return glm::vec2(nx, ny);
}


//向量外积
double path_v::cross_v2(const glm::vec2& a, const glm::vec2& b)
{
	return a.x * b.y - a.y * b.x;
}

std::vector<glm::vec2> equidistant_zoom_contour(glm::vec2* polygon, int count, double margin, int rm)
{
	std::vector<PointD> pv; pv.resize(count);
	for (size_t i = 0; i < count; i++)
	{
		pv[i] = { polygon[i].x,polygon[i].y };
	}
	// 判断是逆时针则相反扩展
	bool ccw = IsPositive(pv);
	if (ccw)
	{
		margin = -margin;
	}
	PathsD pd;
	pd.push_back(std::move(pv));
	//Square=0, Round=1, Miter=2
	if (rm > 2 || rm < 0)
	{
		rm = 2;
	}
	auto rv = InflatePaths(pd, margin, (JoinType)rm, EndType::Polygon);
	std::vector<glm::vec2> result;
	result.reserve(count);
	for (const auto& path : rv) {
		for (auto& it : path)
		{
			glm::vec2 pt = { it.x ,it.y };
			result.push_back(pt);
		}
	}
	return result;
}


void path_v::expand_polygon_c(glm::vec2* polygon, int count, float expand, int type, std::vector<glm::vec2>& ots)
{
	std::vector<glm::vec2> ot;
#if 1  
	auto nv = equidistant_zoom_contour(polygon, count, expand, type);
	ot.swap(nv);
#endif
	if (ot.size())
	{
		ots.reserve(ots.size() + ot.size());
		ots.insert(ots.end(), ot.begin(), ot.end());
		ots.push_back(ot[0]);//ring环
	}


}
void path_v::expand_polygon(glm::vec2* polygon, int count, float expand, std::vector<glm::vec2>& ots)
{
	// already ordered by anticlockwise
	std::vector<glm::vec2> ot;
	if (expand == 0)
	{
		ots.reserve(ots.size() + count);
		ots.insert(ots.end(), polygon, polygon + count);
		return;
	}
#if 1
	if (polygon[0] == polygon[count - 1])count--;
	// 1. vertex set
	ot.reserve(ot.size() + count);
	// 2. edge set and normalize it
	std::vector<glm::vec2> dp, ndp;
	for (int i = 0; i < count; i++) {
		int next = (i == (count - 1) ? 0 : (i + 1));
		auto dn = polygon[next] - polygon[i];
		if (dn.x == 0)
		{
			dn.x = 0.0001;
		}
		if (dn.y == 0)
		{
			dn.y = 0.0001;
		}
		dp.push_back(dn);
		float unitLen = 1.0f / glm::sqrt(dot(dp.at(i), dp.at(i)));
		ndp.push_back(dp.at(i) * unitLen);
	}
	auto xd = abs(expand) * 2;
	std::vector<double> crossv;
	// 3. compute Line
	//负数为内缩， 正数为外扩。 需要注意算法本身并没有检测内缩多少后折线会自相交，那不是本代码的示范意图
	for (int i = 0; i < count; i++) {
		int startIndex = (i == 0 ? (count - 1) : (i - 1));
		int endIndex = i;
		auto sinTheta = cross_v2(ndp.at(startIndex), ndp.at(endIndex));
		crossv.push_back(sinTheta);
		glm::vec2 orientVector = ndp.at(endIndex) - ndp.at(startIndex);//i.e. PV2-V1P=PV2+PV1
		auto ex = glm::vec2(expand / sinTheta * orientVector.x, expand / sinTheta * orientVector.y);
		auto pt = polygon[i];
		ex += pt;
		if (glm::isnan(ex.x) || glm::isinf(ex.x))
		{
			ex = pt;
		}
		auto kd = glm::distance(pt, ex);
		ot.push_back(ex);
	}

#endif
	if (ot.size())
	{
		ots.reserve(ots.size() + ot.size());
		ots.insert(ots.end(), ot.begin(), ot.end());
		ots.push_back(ot[0]);//ring环
	}


}

bool path_v::is_ccw(int idx)
{
	std::vector<glm::vec2>  ms1;
	for (size_t i = 0; i < 1; i++)
	{
		ms1.clear();
		get_flatten(i, 1, 2, 1, 0, &ms1);
	}
	auto count = ms1.size();
	std::vector<PointD> pv;
	pv.resize(count);
	for (size_t i = 0; i < count; i++)
	{
		pv[i] = { ms1[i].x,ms1[i].y };
	}
	// 判断是逆时针则相反扩展
	bool ccw = IsPositive(pv);
	return ccw;
}
#endif

namespace gp {

	int triangulates(std::vector<glm::vec2>* poly, int n, bool pccw, std::vector<glm::vec2>* p, int type)
	{
		if (!p)return 0;
		auto& ms = *p;
		auto pos = ms.size();
		std::vector<std::vector<glm::vec2>>  tr, tr1;
		float z = 0.0;
		std::vector<PointD> pv;
		for (size_t i = 0; i < n; i++)
		{
			auto& tv1 = poly[i];
			pv.resize(tv1.size());
			for (size_t i = 0; i < tv1.size(); i++)
			{
				pv[i] = { tv1[i].x,tv1[i].y };
			}
			bool ccw = IsPositive(pv);
			if (ccw)
				tr1.push_back(tv1);
			else
			{
				tr.push_back(tv1);
			}
		}
		std::vector<glm::vec3> ms3;
		auto nc = tr.size();
		std::map<size_t, std::vector<std::vector<glm::vec2>>> shell;
		for (size_t i = 0; i < nc; i++)
		{
			shell[i].push_back(tr[i]);
		}
		for (auto& it : tr1)
		{
			float dis = -1;
			int64_t c = -1;
			std::vector<glm::vec2> disv;
			for (size_t i = 0; i < nc; i++)
			{
				auto n = tr[i].size();
				auto& st = tr[i];
				glm::vec2 ps = it[0];
				float dis1 = 0;
				auto bd = pnpoly_aos(n, st.data(), ps);// 返回是否在多边形内、最短距离
				disv.push_back(bd);
				if (bd.x > 0)
				{
					if (dis < 0 || bd.y < dis)
					{
						c = i;
						dis = bd.y;
					}
				}
			}
			if (c >= 0)
			{
				shell[c].push_back(it);//holes
			}
		}
		if (type == 1) {
			gp::cmd_plane_t c[1] = {};
			c->opt = &ms3;
			PathsD	solution;
			for (auto& [k, v] : shell)
			{
				solution.clear();
				for (auto& it : v) {
					if (it.size() > 2)
					{
						pv.resize(it.size());
						for (size_t i = 0; i < it.size(); i++)
						{
							pv[i] = { it[i].x,it[i].y };
						}
						solution.push_back(pv);
					}
				}
				if (v.size())
				{
					gp::polygon_ct pct;
					pct.make(&solution, true);
					pct.triangulate(c->opt, 0, pccw);
				}
			}
		}
		else
		{
			for (auto& [k, v] : shell)
			{
				if (v.size())
					gp::constrained_delaunay_triangulation_v(&v, ms3, pccw, z);
			}
		}

		for (auto& it : ms3)
			ms.push_back(it);
		return 0;
	}

	void triangle_to_mesh(const mesh3_mt& src_mesh, mesh_mt& dstMesh, const glm::mat4& src_nm = glm::identity<glm::mat4>())
	{
		auto ps = dstMesh.vertex_coord.size() / 3;
		// vertices precision convention and copy
		dstMesh.vertex_coord.reserve(src_mesh.vertices.size() * 3);
		for (int i = 0; i < src_mesh.vertices.size(); ++i) {
			const glm::vec3 v = src_nm * glm::vec4(src_mesh.vertices[i], 1.0);
			dstMesh.vertex_coord.push_back(v[0]);
			dstMesh.vertex_coord.push_back(v[1]);
			dstMesh.vertex_coord.push_back(v[2]);
		}

		// faces copy
		dstMesh.face_indice.reserve(src_mesh.indices.size() * 3);
		dstMesh.face_size.reserve(src_mesh.indices.size());
		for (int i = 0; i < src_mesh.indices.size(); ++i) {
			const int& f0 = src_mesh.indices[i][0];
			const int& f1 = src_mesh.indices[i][1];
			const int& f2 = src_mesh.indices[i][2];
			dstMesh.face_indice.push_back(f0 + ps);
			dstMesh.face_indice.push_back(f1 + ps);
			dstMesh.face_indice.push_back(f2 + ps);

			dstMesh.face_size.push_back((uint32_t)3);
		}
	}
	mesh_mt triangle_to_mesh(const mesh3_mt& M)
	{
		mesh_mt ot;
		triangle_to_mesh(M, ot);
		return ot;
	}
	int triangulates(std::vector<glm::vec2>* poly, int n, bool pccw, mesh_mt* opt, int type)
	{
		if (!opt || !poly || n < 1)return 0;
		std::vector<glm::vec3> ms;
		auto pos = ms.size();
		std::vector<std::vector<glm::vec2>>  tr, tr1;
		float z = 0.0;
		std::vector<PointD> pv;
		for (size_t i = 0; i < n; i++)
		{
			auto& tv1 = poly[i];
			pv.resize(tv1.size());
			for (size_t i = 0; i < tv1.size(); i++)
			{
				pv[i] = { tv1[i].x,tv1[i].y };
			}
			bool ccw = IsPositive(pv);
			if (ccw)
				tr1.push_back(tv1);
			else
			{
				tr.push_back(tv1);
			}
		}
		mesh3_mt m3;
		std::vector<glm::vec3> ms3;
		auto nc = tr.size();
		std::map<size_t, std::vector<std::vector<glm::vec2>>> shell;
		for (size_t i = 0; i < nc; i++)
		{
			shell[i].push_back(tr[i]);
		}
		for (auto& it : tr1)
		{
			float dis = -1;
			int64_t c = -1;
			std::vector<glm::vec2> disv;
			for (size_t i = 0; i < nc; i++)
			{
				auto n = tr[i].size();
				auto& st = tr[i];
				glm::vec2 ps = it[0];
				float dis1 = 0;
				auto bd = pnpoly_aos(n, st.data(), ps);// 返回是否在多边形内、最短距离
				disv.push_back(bd);
				if (bd.x > 0)
				{
					if (dis < 0 || bd.y < dis)
					{
						c = i;
						dis = bd.y;
					}
				}
			}
			if (c >= 0)
			{
				shell[c].push_back(it);//holes
			}
		}
		if (type == 0)
		{
			for (auto& [k, v] : shell)
			{
				if (v.size())
					gp::constrained_delaunay_triangulation_v(&v, m3.vertices, m3.indices, pccw, z);
			}
		}
		else {
			gp::cmd_plane_t c[1] = {};
			c->opt = &ms;
			PathsD	solution;
			for (auto& [k, v] : shell)
			{
				solution.clear();
				for (auto& it : v) {
					if (it.size() > 2)
					{
						pv.resize(it.size());
						for (size_t i = 0; i < it.size(); i++)
						{
							pv[i] = { it[i].x,it[i].y };
						}
						solution.push_back(pv);
					}
				}
				if (v.size())
				{
					gp::polygon_ct pct;
					pct.make(&solution, true);
					pct.triangulate(&m3, 0, pccw);
				}
			}
		}
		triangle_to_mesh(m3, *opt);
		return 0;
	}
}
//!gp

struct qv
{
	glm::vec2 p0, p1, p2;
};
struct q2
{
	glm::vec2 p1, p2;
};
struct cubic_v1
{
	glm::vec2 p0, p1, p2, p3;	// p1 p2是控制点
};
void c2to3(cubic_v1& c)
{
	static double dv = 2.0 / 3.0;
	glm::vec2 c1, c2;
	auto p0 = c.p0;
	auto p1 = c.p1;
	auto p2 = c.p3;
	c1 = p1 - p0; c1 *= dv; c1 += p0;
	c2 = p1 - p2; c2 *= dv; c2 += p2;
	c.p1 = c1;
	c.p2 = c2;
}
cubic_v1 q2c(qv* p)
{
	cubic_v1 cv = {};
	cv.p0 = p->p0;
	cv.p1 = p->p1;
	cv.p2 = p->p1;
	cv.p3 = p->p2;
	c2to3(cv);
	return cv;
}
// 画圆
void draw_circle(cairo_t* cr, const glm::vec2& pos, float r, const glm::ivec2& c)
{
	cairo_arc(cr, pos.x, pos.y, r, 0, 2 * M_PI);
	if (c.x != 0)
	{
		set_color(cr, c.x);
		if (c.y != 0)
			cairo_fill_preserve(cr);
		else
			cairo_fill(cr);
	}
	if (c.y != 0)
	{
		set_color(cr, c.y);
		cairo_stroke(cr);
	}
}
void save_png_v(path_v* pv, int count, const std::string& fn, bool fy, float sc1)
{
	//std::vector<vertex_i32> vs;
	auto rc = pv->mkbox_npos();
	glm::ivec2 ss = { rc.z ,rc.w };
	//ss += glm::abs(pv->_pos);
	ss *= sc1 + 2;
	printf("save_png_v %d\t%d\n\n", ss.x, ss.y);
	cairo_surface_t* sur = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ss.x, ss.y);
	auto cr = cairo_create(sur);
	auto pxd = (uint32_t*)cairo_image_surface_get_data(sur);
	for (size_t i = 0; i < ss.x * ss.y; i++)
	{
		pxd[i] = 0xff000000;
	}
	cairo_matrix_t flip_y = {};
	cairo_matrix_init(&flip_y, 1, 0, 0, -1, 0, 0); // 垂直翻转
	float sc = sc1 > 0 ? sc1 : 1.0;
	for (size_t x = 0; x < count; x++)
	{
		auto kt = pv; pv++;
		if (!kt)continue;
		glm::vec2 pos = kt->_pos;
		pos *= sc;
		cairo_save(cr);
		//cairo_translate(cr, 0, kt->_baseline);
		cairo_translate(cr, 0, kt->_baseline * sc);
		cairo_translate(cr, pos.x, fy ? -pos.y : pos.y);
		if (fy)
			cairo_transform(cr, &flip_y);
		auto v1 = kt->_data;
		auto v = v1.data();
		auto ks = kt->_data.size();
		for (int i = 0; i < ks; i++)
		{
			auto& it = v[i];
			auto mt = (path_v::vertex_t*)&it;
			mt->p *= sc;
			mt->c *= sc;
			mt->c1 *= sc;
		}
		for (int i = 0; i < ks; i++)
		{
			auto& it = v[i];
			auto mt = (path_v::vertex_t*)&it;
			//vs.push_back(it);
			auto type = (path_v::vtype_e)v[i].type;
			switch (type)
			{
			case path_v::vtype_e::e_vmove:
				cairo_move_to(cr, it.p.x, it.p.y);
				//printf("m%.02f,%.02f ", it.x, it.y * -1.0);
				break;
			case path_v::vtype_e::e_vline:
				cairo_line_to(cr, it.p.x, it.p.y);
				//printf("l%.02f,%.02f ", it.x, it.y * -1.0);
				break;
			case path_v::vtype_e::e_vcurve:
			{
				qv q;
				q.p0 = { v[i - 1].p.x, v[i - 1].p.y };
				q.p1 = { v[i].c.x, v[i].c.y };
				q.p2 = { v[i].p.x, v[i].p.y };
				auto c = q2c(&q);
				cairo_curve_to(cr, c.p1.x, c.p1.y, c.p2.x, c.p2.y, c.p3.x, c.p3.y);
				//printf("q%.02f,%.02f %.02f,%.02f ", it.cx, it.cy * -1.0, it.x, it.y * -1.0);
			}
			break;
			case path_v::vtype_e::e_vcubic:
			{
				cubic_v1 c = {};
				c.p1 = { v[i].c.x, v[i].c.y };
				c.p2 = { v[i].c1.x, v[i].c1.y };
				c.p3 = { v[i].p.x, v[i].p.y };
				cairo_curve_to(cr, c.p1.x, c.p1.y, c.p2.x, c.p2.y, c.p3.x, c.p3.y);
			}
			break;
			default:
				break;
			}
		}
		if (ks > 2)
		{
			cairo_close_path(cr);
			cairo_set_source_rgb(cr, 1, 0.51, 0);
			//cairo_fill_preserve(cr);
		}
		cairo_set_line_width(cr, 1.0);
		cairo_set_source_rgba(cr, 0, 0.51, 1, 0.8);
		cairo_stroke(cr);

		uint32_t cc[2] = { 0xffff8000 ,0xff0080ff };
		int r = 2;
		for (int i = 0; i < ks; i++)
		{
			auto& it = v[i];
			auto mt = (path_v::vertex_t*)&it;
			//vs.push_back(it);
			auto type = (path_v::vtype_e)v[i].type;
			draw_circle(cr, mt->p, r, { cc[x], 0 });
			switch (type)
			{
			case path_v::vtype_e::e_vcurve:
			{
				draw_circle(cr, mt->c, r, { 0xff0080ff, 0 });
			}
			break;
			case path_v::vtype_e::e_vcubic:
			{
				draw_circle(cr, mt->c, r, { 0xff0080ff, 0 });
				draw_circle(cr, mt->c1, r, { 0xff0080ff, 0 });
			}
			break;
			default:
				break;
			}
		}
		cairo_restore(cr);
	}
	cairo_destroy(cr);
	cairo_surface_write_to_png(sur, fn.c_str());
	cairo_surface_destroy(sur);
}

#endif // path_v_h


#if 1
void split_v(std::string str, const std::string& pattern, std::vector<std::string>& result)
{
	std::string::size_type pos;
	str += pattern;//扩展字符串以方便操作
	int size = str.size();
	result.clear();
	int ct = 0;
	for (int i = 0; i < size; i++)
	{
		pos = str.find(pattern, i);
		if (pos < size)
		{
			std::string s = str.substr(i, pos - i);
			result.push_back(s);
			i = pos + pattern.size() - 1;
			ct++;
		}
	}
}

layout_text_x::layout_text_x()
{
	gs = new gshadow_cx();
}

layout_text_x::~layout_text_x()
{
	if (gs)delete gs; gs = 0;
	for (auto it : msu) {
		free_image_cr(it);
	}
}

void layout_text_x::set_ctx(font_rctx* p)
{
	if (p)
	{
		ctx = p;
	}
}

size_t layout_text_x::add_familys(const char* familys, const char* style) {
	if (!ctx)return 0;
	assert(ctx);
	size_t rs = 0;
	if (ctx && familys && *familys)
	{
		std::vector<std::string> result;
		split_v(familys, ",", result);
		std::vector<font_t*> v;
		for (auto& it : result)
		{
			auto ft = ctx->get_font(it.c_str(), style);
			if (ft)
			{
				v.push_back(ft);
			}
		}
		if (v.size())
		{
			rs = familyv.size();
			familyv.push_back(v);
			cfb.push_back({});
		}
	}
	return rs;
}
void layout_text_x::cpy_familys(layout_text_x* p)
{
	if (p)
	{
		familyv = p->familyv;
		cfb = p->cfb;
	}
}
void layout_text_x::clear_family()
{
	familyv.clear();
	cfb.clear();
}
void layout_text_x::clear_text()
{
	tv.clear();
}

void layout_text_x::c_line_metrics(size_t idx, int fontsize) {
	if (idx >= familyv.size())idx = 0;
	if (fontsize == 0 || idx >= cfb.size() || familyv.empty())return;
	if (cfb[idx].z != fontsize)
	{
		glm::dvec2 r = {};
		auto& v = familyv[idx];
		for (auto it : v)
		{
			double scale = fontsize == 0 ? 1.0 : it->get_scale(fontsize);
			r.x = std::max(it->ascender * scale, r.x);
			r.y = std::max((it->ascender - it->descender + it->lineGap) * scale, r.y);
		}
		glm::ivec3 c = { r,fontsize };
		cfb[idx] = c;
	}
}
int layout_text_x::get_baseline(size_t idx, int fontsize)
{
	if (familyv.empty())return 0;
	if (idx >= familyv.size())idx = 0;
	c_line_metrics(idx, fontsize);
	return cfb[idx].x;
}

int layout_text_x::get_lineheight(size_t idx, int fontsize)
{
	if (familyv.empty())return 0;
	if (idx >= familyv.size())idx = 0;
	c_line_metrics(idx, fontsize);
	return heightline ? heightline : cfb[idx].y;
}

glm::ivec3 layout_text_x::get_text_rect(size_t idx, const void* str8, int len, int fontsize)
{
	glm::ivec3 ret = {};
	if (familyv.empty())
		return ret;
	auto str = (const char*)str8;
	if (idx >= familyv.size())idx = 0;
	auto font = familyv[idx][0];
	int x = 0;
	int y = 0;
	int n = 1;
	int lineheight = 0;// get_lineheight(idx, fontsize);

	font_t* oft0 = 0;
	do
	{
		if (!str || !(*str)) { break; }
		int ch = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
		//str = md::get_u8_last(str, &ch);
		if (ch == '\n')
		{
			ret.x = std::max(ret.x, x);
			x = 0;
			n++;
			continue;
		}
		font_t* oft = 0;
		auto rc = font->get_char_extent(ch, fontsize,/* fdpi,*/ &familyv[idx], &oft);
		if (oft != oft0 && oft) {
			oft0 = oft;
			double scale = fontsize == 0 ? 1.0 : oft->get_scale(fontsize);
			lineheight = std::max((int)((oft->ascender - oft->descender + oft->lineGap) * scale), lineheight);
		}

		x += rc.z;
		y = std::max(rc.y, y);
		ret.y = std::max(ret.y, y);
		ret.z = std::max(ret.z, rc.w);
	} while (str && *str);
	ret.x = std::max(ret.x, x);
	if (heightline > 0)
		lineheight = heightline;
	if (n > 1)
		ret.y = lineheight * n;
	else
		ret.y = get_lineheight(idx, fontsize);
	return ret;
}
glm::ivec3 layout_text_x::get_text_rect1(size_t idx, int fontsize, const void* str8)
{
	glm::ivec3 ret = {};
	if (familyv.empty())
		return ret;
	auto str = (const char*)str8;
	if (idx >= familyv.size())idx = 0;
	auto font = familyv[idx][0];
	int x = 0;
	int y = 0;
	int n = 1;
	int lineheight = 0;// get_lineheight(idx, fontsize);

	font_t* oft0 = 0;
	do
	{
		if (!str || !(*str)) { break; }
		int ch = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
		if (ch == '\n')
		{
			ret.x = std::max(ret.x, x);
			x = 0;
			n++;
			break;
		}
		font_t* oft = 0;
		auto rc = font->get_char_extent(ch, fontsize,/* fdpi,*/ &familyv[idx], &oft);
		if (oft != oft0) {
			oft0 = oft;
			double scale = fontsize == 0 ? 1.0 : oft->get_scale(fontsize);
			//lineheight = std::max((int)(oft->ascender * scale), lineheight);
			lineheight = std::max((int)((oft->ascender - oft->descender + oft->lineGap) * scale), lineheight);
			ret.z = oft->ascender * scale;
		}
		x = rc.x;
		ret.y = rc.y;
		break;
	} while (str && *str);
	ret.x = x;
	return ret;
}

int layout_text_x::get_text_pos(size_t idx, int fontsize, const void* str8, int len, int xpos)
{
	int ret = {};
	if (familyv.empty())
		return ret;
	auto str = (const char*)str8;
	auto str0 = (const char*)str8;
	if (idx >= familyv.size())idx = 0;
	auto font = familyv[idx][0];
	int x = 0;
	double ktt = 0.6;
	tem_pv.clear();
	do
	{
		if (!str || !(*str)) { break; }
		auto pstr = str;
		int ch = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
		if (ch == '\n')
		{
			break;
		}
		font_t* oft = 0;
		auto rc = font->get_char_extent(ch, fontsize,/* fdpi,*/ &familyv[idx], &oft);
		x += rc.z;
		tem_pv.push_back(x);
	} while (str && *str);
	if (tem_pv.size()) {
		auto rt = std::lower_bound(tem_pv.begin(), tem_pv.end(), xpos);
		ret = (rt != tem_pv.end()) ? *rt : *tem_pv.rbegin();
		if (xpos < tem_pv[0])
		{
			ret = tem_pv[0];
		}
		for (size_t i = 0; i < tem_pv.size(); i++)
		{
			if (ret == tem_pv[i])
			{
				ret = i; break;
			}
		}
		if (ret > tem_pv.size() || xpos > *tem_pv.rbegin())
		{
			ret = tem_pv.size();
		}
	}
	return ret;
}
int layout_text_x::get_text_ipos(size_t idx, int fontsize, const void* str8, int len, int ipos)
{
	int ret = {};
	if (familyv.empty())
		return ret;
	auto str = (const char*)str8;
	auto str0 = str + ipos;
	if (idx >= familyv.size())idx = 0;
	auto font = familyv[idx][0];
	int x = 0;
	double ktt = 0.6;
	do
	{
		if (!str || !(*str) || str >= str0) { break; }

		int ch = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
		if (ch == '\n')
		{
			x = 0;
			break;
		}
		font_t* oft = 0;
		auto rc = font->get_char_extent(ch, fontsize,/* fdpi,*/ &familyv[idx], &oft);
		x += rc.z;
	} while (str && *str);
	return x;
}
int layout_text_x::get_text_posv(size_t idx, int fontsize, const void* str8, int len, std::vector<std::vector<int>>& ow)
{
	int ret = {};
	if (familyv.empty())
		return ret;
	auto str = (const char*)str8;
	auto str0 = (const char*)str8;
	if (idx >= familyv.size())idx = 0;
	auto font = familyv[idx][0];
	int x = 0;
	double ktt = 0.6;
	ow.clear();
	std::vector<int> w0;
	w0.push_back(0);
	do
	{
		if (!str || !(*str)) { break; }

		int ch = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
		if (ch == '\n')
		{
			ow.push_back(w0);
			w0.clear();
			w0.push_back(0); x = 0;
			continue;
		}
		font_t* oft = 0;
		auto rc = font->get_char_extent(ch, fontsize,/* fdpi,*/ &familyv[idx], &oft);
		x += rc.z;
		w0.push_back(x);
	} while (str && *str);
	if (w0.size())ow.push_back(w0);
	return ret;
}

glm::ivec2 layout_text_x::add_text(size_t idx, glm::vec4& rc, const glm::vec2& text_align, const void* str8, int len, int fontsize)
{
	return build_text(idx, rc, text_align, str8, len, fontsize, tv);
}
glm::ivec2 layout_text_x::build_text(size_t idx, glm::vec4& rc, const glm::vec2& text_align, const void* str8, int len, int fontsize, std::vector<font_item_t>& rtv)
{
	assert(fontsize < 800);
	glm::ivec2 ret = { rtv.size(), 0 };
	if (!ctx) { return ret; }
	text_image_t* p = 0;
	cti.tv.clear();
	if (idx >= familyv.size())idx = 0;
	if (len > 0)
	{
		std::string str((char*)str8, len);
		p = get_glyph_item(idx, str.c_str(), fontsize, &cti);
	}
	else
	{
		p = get_glyph_item(idx, str8, fontsize, &cti);
	}
	if (p)
	{
		auto rct0 = get_text_rect(idx, str8, len, fontsize);
		glm::vec2 rct = { rct0.x,rct0.y };
		auto length = p->tv.size();
		auto baseline = rct0.z;
		baseline = get_baseline(idx, fontsize);
		int h = get_lineheight(idx, fontsize);
		if (rc.z < 1)rc.z = rct.x;
		if (rc.w < 1)rc.w = h;
		if (ctrc.y < h)ctrc.y = h;
		auto ta = text_align;
		if (ta.x < 0)ta.x = 0;
		if (ta.y < 0)ta.y = 0;
		glm::vec2 ss = { rc.z,rc.w }, bearing = { 0, baseline };
		// 区域大小 - 文本包围盒大小。居中就是text_align={0.5,0.5}
		auto ps = (ss - rct) * ta + bearing;
		ps.x += rc.x;
		ps.y += rc.y;
		glm::vec2 tps = {};
		rtv.reserve(rtv.size() + length);
		for (size_t i = 0; i < length; i++)
		{
			auto& it = p->tv[i];
			if (it.cpt == '\n')
			{
				tps.y += h;
				tps.x = 0;
			}
			if (it.cpt == '\t')
			{
				it.advance = fontsize;
			}
			it._apos = ps + tps;
			tps.x += it.advance;
			if (ctrc.x < it.advance)ctrc.x = it.advance;
			rtv.push_back(it);
		}
		ret.y = p->tv.size();
	}
	return ret;
}

atlas_t* layout_text_x::get_atlas()
{
	auto ft = ctx->bcc._data.data();
	auto n = ctx->bcc._data.size();
	tem_iptr.clear();
	tem_iptr.resize(n);
	for (size_t i = 0; i < n; i++)
	{
		auto& rt = tem_iptr[i];
		auto p = ft[i];
		rt.img = p;
	}
	for (auto& it : tv) {
		it._image;
	}
	{

		image_ptr_t* img = 0;
		glm::ivec4* img_rc = 0;	// 显示坐标、大小
		glm::ivec4* tex_rc = 0;	// 纹理区域
		glm::ivec4* sliced = 0;	// 九宫格渲染
		uint32_t* colors = 0;	// 颜色混合/透明度
		size_t count = 0;		// 数量
		glm::ivec4 clip = {};	// 裁剪区域
	};
	return nullptr;
}
bool layout_text_x::update_text()
{
	bool r = false;
	if (!ctx)return r;
	auto ft = ctx->bcc._data.data();
	auto n = ctx->bcc._data.size();
	for (size_t i = 0; i < n; i++)
	{
		auto p = ft[i];
		if (p->ptr)
		{
			if (p->valid)
			{
				update_image_cr((cairo_surface_t*)p->ptr, p);
				p->valid = 0; r = true;
			}
		}
		else {
			cairo_surface_t* su = new_image_cr(p);
			if (su)
			{
				msu.push_back(su);
				p->valid = 0; r = true;
				p->ptr = su;
			}
		}
	}
	return r;
}
atlas_cx* layout_text_x::new_shadow(const glm::ivec2& ss, const glm::ivec2& pos)
{
	if (sli.tex_rc.x < sli_radius)
	{
		rect_shadow_t rs = {};
		rs.cfrom = { 0.00,0.00,0.0,1 }, rs.cto = { 0.9,0.9,0.9,1 };
		rs.radius = sli_radius;
		rs.segment = 8;
		rs.cubic = { {0.0,0.66},{0.5,0.39},{0.4,0.1},{1.0,0.01 } };
		sli = gs->new_rect(rs);
	}
	auto rcs = sli;
	auto a = new atlas_cx();
	a->img = gs->img;
	a->img->type = 1;
	a->autofree = true;
	rcs.img_rc = { pos.x,pos.y,ss.x,ss.y };
	rcs.img_rc.x = rcs.img_rc.y = 0;
	a->add(&rcs, 1);
	//can->add_atlas(a);
	return a;
}

pvm_t layout_text_x::new_menu(int width, int height, const std::vector<std::string>& v, bool has_shadow, std::function<void(int type, int id)> cb) {
	std::vector<const char*> vs;
	vs.resize(v.size());
	for (size_t i = 0; i < v.size(); i++)
	{
		vs[i] = v[i].c_str();
	}
	return new_menu(width, height, vs.data(), v.size(), has_shadow, cb);
}
pvm_t layout_text_x::new_menu(int width, int height, const char** v, size_t n, bool has_shadow, std::function<void(int type, int id)> cb)
{
	pvm_t ret = {};
	auto ltx = this;
	auto p = new plane_cx();
	if (p && n > 0)
	{
		p->fontsize = 16;
		int lheight = height > 0 ? height : ltx->get_lineheight(0, p->fontsize) * 1.5;
		if (width < 0)
		{
			int xw = lheight;
			for (size_t i = 0; i < n; i++)
			{
				auto it = v[i];
				auto rc = ltx->get_text_rect(0, it, -1, p->fontsize);
				if (xw < rc.x) {
					xw = rc.x;
				}
			}
			width = xw + lheight;
		}
		ret.w = width;
		ret.h = lheight;
		ret.cpos = m_cpos;
		glm::ivec2 iss = { width , lheight };
		p->set_color(m_color);
		glm::ivec2 ss = { width + p->border.y * 7, n * lheight + p->border.y * 7 };

		auto radius = ltx->sli_radius;
		glm::ivec2 sas = {};
		if (has_shadow)
		{
			sas += radius;
			auto ass = ss + radius;
			auto pa = ltx->new_shadow(ass, {});
			ret.back = pa;
		}
		p->_lpos = { 0,0 }; p->_lms = { 0,0 };
		p->custom_layout = true;
		p->set_fontctx(ltx->ctx);
		p->ltx->cpy_familys(ltx);
		for (size_t i = 0; i < n; i++)
		{
			auto it = v[i];
			auto pcb = p->add_cbutton(it, iss, 2);
			pcb->pos = { 0, i * lheight };
			pcb->pos += ret.cpos;
			pcb->light = 0.051;
			pcb->effect = uTheme::light;
			pcb->pdc.hover_border_color = pcb->pdc.border_color;
			pcb->pdc.border_color = 0;
			pcb->text_align = { 0.0,0.5 };
			if (pcb && cb)
			{
				pcb->click_cb = [=](void* pr, int)
					{
						cb(1, i);
						auto pc = (color_btn*)pr;
						if (pc)
							pc->bst = (int)BTN_STATE::STATE_NOMAL;
					};
				pcb->mevent_cb = [=](void* p, int type, const glm::vec2& mps) {
					if (type == (int)event_type2::on_hover) {
						cb(0, i);
					}
					if (type == (int)event_type2::on_move) {
						cb(2, i);
					}
					};
			}
		}
		p->set_size(ss);
		p->set_pos({ radius * 0,radius * 0 });
		ret.p = p;
		ret.fsize = ss + sas;
	}
	return ret;
}

void layout_text_x::free_menu(pvm_t pt)
{
	if (pt.p)
		delete pt.p;
	if (pt.back)
		delete pt.back;
}









void layout_text_x::draw_text(cairo_t* cr, const glm::ivec2& r, uint32_t color)
{
	int mx = r.y + r.x;
	for (size_t i = r.x; i < mx; i++)
	{
		auto& it = tv[i];
		if (it._image)
		{
			auto ft = (cairo_surface_t*)it._image->ptr;
			if (ft)
			{
				draw_image(cr, ft, it._dwpos + it._apos, it._rect, it.color ? it.color : color);
			}
		}
	}
}
void layout_text_x::draw_text(cairo_t* cr, const std::vector<font_item_t>& rtv, uint32_t color)
{
	auto mx = rtv.size();
	for (size_t i = 0; i < mx; i++)
	{
		auto& it = rtv[i];
		if (it._image)
		{
			auto ft = (cairo_surface_t*)it._image->ptr;
			if (ft)
			{
				draw_image(cr, ft, it._dwpos + it._apos, it._rect, it.color ? it.color : color);
			}
		}
	}
}
void layout_text_x::draw_rect_rc(cairo_t* cr, const std::vector<font_item_t>& rtv, uint32_t color)
{
	auto mx = rtv.size();
	for (size_t i = 0; i < mx; i++)
	{
		auto& it = rtv[i];
		auto pos = it._dwpos + it._apos;
		glm::ivec4 rc = { pos.x,pos.y,it._rect.z,it._rect.w };
		draw_rect(cr, rc, color, 0, 0, 0);
	}
}
void layout_text_x::draw_text(cairo_t* cr, uint32_t color)
{
	update_text();
	for (auto& it : tv)
	{
		if (it._image)
		{
			auto ft = (cairo_surface_t*)it._image->ptr;
			if (ft)
			{
				draw_image(cr, ft, it._dwpos + it._apos, it._rect, it.color ? it.color : color);
			}
		}
	}
}

void layout_text_x::text2atlas(const glm::ivec2& r, uint32_t color, std::vector<atlas_cx>* opt)
{
	if (opt && tv.size())
	{
		int mx = r.y + r.x;
		atlas_cx ac = {};
		ac.img = tv[r.x]._image;
		for (size_t i = r.x; i < mx; i++)
		{
			auto& it = tv[i];
			if (it._image == ac.img)
			{
				ac.add({ it._dwpos,it._rect.z,it._rect.w }, it._rect, {}, it.color ? it.color : color);
			}
			else {
				if (ac._imgv.size())
				{
					opt->push_back(ac);
					ac.clear();
				}
				ac.img = it._image;
				ac.add({ it._dwpos,it._rect.z,it._rect.w }, it._rect, {}, it.color ? it.color : color);
			}
		}
		if (ac._imgv.size())
		{
			opt->push_back(ac);
			ac.clear();
		}
	}

}

text_path_t* layout_text_x::get_shape(size_t idx, const void* str8, int fontsize, text_path_t* opt)
{
	auto str = (const char*)str8;
	if (idx >= familyv.size())idx = 0;
	int adv = 0;
	do
	{
		if (!str || !(*str) || !opt) { opt = 0; break; }
		font_t* r = 0;
		int gidx = 0;
		auto ts = str;
		str = font_t::get_glyph_index_u8(str, &gidx, &r, &familyv[idx]);
		if (r && gidx >= 0)
		{
			auto k = r->get_shape(ts, fontsize, &opt->data, adv);
			adv += k.advance;
			if (k.count)
			{
				opt->tv.push_back(k);
			}
		}
	} while (str && *str);
	if (opt) {
		auto pd = opt->data.data();
		for (size_t i = 0; i < opt->tv.size(); i++)
		{
			auto& it = opt->tv[i];
			it.v = pd + it.first;
		}
	}
	return opt;
}
// todo get_glyph_item

text_image_t* layout_text_x::get_glyph_item(size_t idx, const void* str8, int fontsize, text_image_t* opt)
{
	auto str = (const char*)str8;
	if (idx >= familyv.size())idx = 0;
	do
	{
		if (!str || !(*str) || !opt) { opt = 0; break; }
		font_t* r = 0;
		int gidx = 0;
		if (*str == '\n')
			gidx = 0;
		auto ostr = str;

		int ch = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
		font_t::get_glyph_index_u8(ostr, &gidx, &r, &familyv[idx]);
		if (r && gidx >= 0)
		{
			auto k = r->get_glyph_item(gidx, ch, fontsize);
			if (k._glyph_index)
			{
				k.cpt = ch;
				opt->tv.push_back(k);
			}
		}
		else {
			font_item_t k = {};
			k.cpt = ch;
			k.advance = 0;
			opt->tv.push_back(k);
		}
	} while (str && *str);
	return opt;
}

#endif // 1




#ifndef no_cairo_



glm::ivec4 get_text_extents(cairo_t* cr, const void* str, int len, font_xi* fx);
class Ruler
{
public:
	Ruler();
	void set_range(double lower, double upper);
	void set_page(double lower, double upper);
	void set_selection(double lower, double upper);

	//void add_track_widget(Gtk::Widget& widget);

	glm::ivec2 get_drawing_size();
	bool draw_scale(cairo_t* cr);
	void draw_marker(cairo_t* cr);
	glm::ivec4 marker_rect();
	bool on_drawing_area_draw(cairo_t* cr, const glm::vec2& pos);
	void on_style_updated();
	void on_prefs_changed();

	void on_motion(void* motion, double x, double y);
	int on_click_pressed(void* click, int n_press, double x, double y);

	void set_context_menu();
	cairo_surface_t* draw_label(cairo_t* cr_in, const std::string& label_value, const glm::ivec2& ts);

	glm::ivec2 drawingsize = {};
	glm::ivec2 oldsize = {};
	//Gtk::DrawingArea* _drawing_area;
	//Inkscape::PrefObserver _watch_prefs;
	//Gtk::Popover* _popover = nullptr;
	int    _orientation = 0;
	//Inkscape::Util::Unit const* _unit;
	double _lower = 0;
	double _upper = 100;
	double _position = 0;
	double _max_size = 100;

	// Page block
	double _page_lower = 0.0;
	double _page_upper = 0.0;

	// Selection block
	double _sel_lower = 0.0;
	double _sel_upper = 0.0;
	double _sel_visible = true;
	int is_yaxisdown = 0;
	bool   has_page = false;
	bool   _backing_store_valid = false;
	bool   ruler_in = 0;//英尺
	cairo_surface_t* _backing_store = 0;
	glm::ivec4 _rect = {};

	std::unordered_map<int, cairo_surface_t*> _label_cache;

	// Cached style properties
	glm::vec4 _shadow = { 0.0,0.00,0.0,0.5 };
	glm::vec4 _foreground = { 1.0,1.0,1.0,1.0 };
	glm::vec4 _cursor_color = { 0.87,0.384,0.384,1.0 };// 1.0, 0.50, 0.0, 1.0};//
	glm::vec4 _page_fill = { 0.12, 0.12, 0.12, 1.0 };
	glm::vec4 _select_fill = {  };
	glm::vec4 _select_stroke = { 0.4,0.4,1.0,1.0 };
	int _font_size = 8;
	uint32_t _back_color = 0xff353535;
	font_xi* fx = 0;
};

class canvas_dev;
class tinyviewcanvas_x
{
public:
	glm::ivec2 vpos = {}, size = {};	// 渲染偏移，大小
	std::vector<path_v*>* _data = 0;
	std::vector<path_v*> draw_data;
	std::vector<glm::vec4> ddrect, vr;
	// 填充颜色\线框颜色
	glm::ivec2 color = { 0x20ffffff, 0xffFF8050 };
	glm::vec4 bx = {};
	glm::mat3 mx = glm::mat3(1.0);
	glm::vec2 last_mouse = {}, eoffset = {};
	glm::ivec4 hover_bx = {};		//当前鼠标对象包围框
	int ckinc = 0;
	int scaleStep = 20;
	int scale = 100;
	int oldscale = 0;
	int minScale = 2, maxScale = 25600;
	int line_width = 1.0;
	cairo_surface_t* _backing_store = 0;

	bool   _backing_store_valid = false;
	bool   _mousezoom = true;
	bool   has_move = false;
	bool   has_scale = false;
public:
	tinyviewcanvas_x();
	~tinyviewcanvas_x();
	void set_size(const glm::ivec2& ss);
	void set_view_move(bool is);	// 鼠标移动视图
	void set_view_scale(bool is);	// 滚轮缩放视图
	void draw(canvas_dev* c);
	void on_button(int idx, int down, const glm::vec2& pos, int clicks);
	void on_motion(const glm::vec2& ps);
	void on_wheel(int deltaY);
	void reset_view();
	glm::ivec4 get_hbox();
	glm::vec4 get_bbox();
	glm::mat3 get_affine();
	void hit_test(const glm::vec2& ps);
private:
	void draw_back();

};



#if 1

void draw_round_rectangle(cairo_t* cr, double x, double y, double width, double height, double r)
{
#ifndef M_PI
	auto M_PI = glm::pi<double>();
#endif
	cairo_move_to(cr, x + r, y);
	cairo_line_to(cr, x + width - r, y);
	cairo_arc(cr, x + width - r, y + r, r, 3 * M_PI / 2, 2 * M_PI);
	//cairo_move_to(cr, x + width, y + r);
	cairo_line_to(cr, x + width, y + height - r);
	cairo_arc(cr, x + width - r, y + height - r, r, 0, M_PI / 2);
	//cairo_move_to(cr, x + width - r, y + height);
	cairo_line_to(cr, x + r, y + height);
	cairo_arc(cr, x + r, y + height - r, r, M_PI / 2, M_PI);
	//cairo_move_to(cr, x, y + height - r);
	cairo_line_to(cr, x, y + r);
	cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2.0);
	cairo_close_path(cr);
}
// r，左右下左
void draw_round_rectangle(cairo_t* cr, double x, double y, double width, double height, const glm::vec4& r)
{
#ifndef M_PI
	auto M_PI = glm::pi<double>();
#endif
	cairo_move_to(cr, x + r.x, y);
	cairo_line_to(cr, x + width - r.y, y);
	if (r.y)
		cairo_arc(cr, x + width - r.y, y + r.y, r.y, 3 * M_PI / 2, 2 * M_PI);
	//cairo_move_to(cr, x + width, y + r.y);
	cairo_line_to(cr, x + width, y + height - r.z);
	if (r.z)
		cairo_arc(cr, x + width - r.z, y + height - r.z, r.z, 0, M_PI / 2);
	//cairo_move_to(cr, x + width - r.z, y + height);
	cairo_line_to(cr, x + r.w, y + height);
	if (r.w)
		cairo_arc(cr, x + r.w, y + height - r.w, r.w, M_PI / 2, M_PI);
	//cairo_move_to(cr, x, y + height - r.w);
	cairo_line_to(cr, x, y + r.x);
	if (r.x > 0)
		cairo_arc(cr, x + r.x, y + r.x, r.x, M_PI, 3 * M_PI / 2.0);
	cairo_close_path(cr);
}
void draw_round_rectangle(cairo_t* cr, const glm::vec4& rc, const glm::vec4& r)
{
	draw_round_rectangle(cr, rc.x, rc.y, rc.z, rc.w, r);
}
// 圆角矩形
void draw_rectangle(cairo_t* cr, const glm::vec4& rc, double r)
{
	if (r > 0)
	{
		draw_round_rectangle(cr, rc.x, rc.y, rc.z, rc.w, r);
	}
	else {
		cairo_rectangle(cr, rc.x, rc.y, rc.z, rc.w);
	}
}
void draw_triangle(cairo_t* cr, const glm::vec2& pos, const glm::vec2& size, const glm::vec2& dirspos)
{
	glm::vec2 tpos[3] = {};
	float df = dirspos.y;
	switch ((int)dirspos.x)
	{
	case 0:
	{
		tpos[0] = { size.x * df, 0 };
		tpos[1] = { size.x, size.y };
		tpos[2] = { 0, size.y };
	}
	break;
	case 1:
	{
		tpos[0] = { size.x, size.y * df };
		tpos[1] = { 0, size.y };
		tpos[2] = { 0, 0 };
	}
	break;
	case 2:
	{
		tpos[0] = { size.x * df, size.y };
		tpos[1] = { 0, 0 };
		tpos[2] = { size.x, 0 };
	}
	break;
	case 3:
	{
		tpos[0] = { 0, size.y * df };
		tpos[1] = { size.x, 0 };
		tpos[2] = { size.x, size.y };
	}
	break;
	default:
		break;
	}
	cairo_move_to(cr, pos.x + tpos[0].x, pos.y + tpos[0].y);
	cairo_line_to(cr, pos.x + tpos[1].x, pos.y + tpos[1].y);
	cairo_line_to(cr, pos.x + tpos[2].x, pos.y + tpos[2].y);
	cairo_close_path(cr);
}

//vg_style_t
	//float* dash = 0;		// 虚线逗号/空格分隔的数字
	//int dashOffset = 0;

void fill_stroke(cairo_t* cr, vg_style_t* st) {
	bool stroke = st && st->thickness > 0 && st->color > 0;
	if (!cr || !st)return;
	if (st->fill)
	{
		set_color(cr, st->fill);
		if (stroke)
			cairo_fill_preserve(cr);
		else
			cairo_fill(cr);
	}
	if (stroke)
	{
		cairo_set_line_width(cr, st->thickness);
		if (st->cap > 0)
			cairo_set_line_cap(cr, (cairo_line_cap_t)st->cap);
		if (st->join > 0)
			cairo_set_line_join(cr, (cairo_line_join_t)st->join);
		if (st->dash.v > 0 || st->dash_p)
		{
			double dashes[64] = {};
			uint64_t x = 1;
			auto t = dashes;
			int num_dashes = st->dash_num;
			if (num_dashes > 64)num_dashes = 64;
			if (st->dash_p)
			{
				for (size_t i = 0; i < num_dashes; i++)
				{
					*t = st->dash_p[i]; t++;
				}
			}
			else
			{
				if (num_dashes > 8)num_dashes = 8;
				for (size_t i = 0; i < num_dashes; i++)
				{
					*t = st->dash.v8[i]; t++;
				}
			}
			if (num_dashes > 0)
				cairo_set_dash(cr, dashes, num_dashes, st->dash_offset);
		}
		set_color(cr, st->color);
		cairo_stroke(cr);
	}
}
void fill_stroke(cairo_t* cr, uint32_t fill, uint32_t color, int linewidth, bool isbgr) {
	bool stroke = linewidth > 0 && color;
	if (fill)
	{
		if (isbgr)
			set_color_bgr(cr, fill);
		else
			set_color(cr, fill);
		if (stroke)
			cairo_fill_preserve(cr);
		else
			cairo_fill(cr);
	}
	if (stroke)
	{
		cairo_set_line_width(cr, linewidth);
		if (isbgr)
			set_color_bgr(cr, color);
		else
			set_color(cr, color);
		cairo_stroke(cr);
	}
}
// 画圆
void draw_circle(cairo_t* cr, const glm::vec2& pos, float r)
{
	cairo_arc(cr, pos.x, pos.y, r, 0, 2 * M_PI);
}

void draw_ellipse(cairo_t* cr, const glm::vec2& c, const glm::vec2& r)
{
	double cx = c.x, cy = c.y, rx = r.x, ry = r.y;
	if (rx > 0.0f && ry > 0.0f) {
		cairo_move_to(cr, cx + rx, cy);
		cairo_curve_to(cr, cx + rx, cy + ry * c_KAPPA90, cx + rx * c_KAPPA90, cy + ry, cx, cy + ry);
		cairo_curve_to(cr, cx - rx * c_KAPPA90, cy + ry, cx - rx, cy + ry * c_KAPPA90, cx - rx, cy);
		cairo_curve_to(cr, cx - rx, cy - ry * c_KAPPA90, cx - rx * c_KAPPA90, cy - ry, cx, cy - ry);
		cairo_curve_to(cr, cx + rx * c_KAPPA90, cy - ry, cx + rx, cy - ry * c_KAPPA90, cx + rx, cy);
		cairo_close_path(cr);
	}
}

void draw_polyline(cairo_t* cr, const glm::vec2& pos, const glm::vec2* points, int points_count, unsigned int col, bool closed, float thickness)
{
	if (!cr || !points || points_count < 2 || !col)return;
	cairo_save(cr);
	cairo_translate(cr, pos.x, pos.y);
	cairo_move_to(cr, points->x, points->y);
	for (size_t i = 1; i < points_count; i++)
	{
		cairo_line_to(cr, points[i].x, points[i].y);
	}
	if (closed) { cairo_close_path(cr); }
	cairo_set_line_width(cr, thickness);
	if (col) {
		set_color(cr, col);
		cairo_stroke(cr);
	}
	cairo_restore(cr);
}
void draw_polyline(cairo_t* cr, const glm::vec2* points, int points_count, bool closed)
{
	if (!cr || !points || points_count < 2)return;
	cairo_move_to(cr, points->x, points->y);
	for (size_t i = 1; i < points_count; i++)
	{
		cairo_line_to(cr, points[i].x, points[i].y);
	}
	if (closed) { cairo_close_path(cr); }
}

void draw_polyline(cairo_t* cr, const PathsD* p, bool closed)
{
	if (!cr || !p || p->size() < 1)return;
	auto& d = *p;
	auto length = d.size();
	for (size_t i = 0; i < length; i++)
	{
		auto points = d[i];
		cairo_move_to(cr, points[0].x, points[0].y);
		auto points_count = points.size();
		for (size_t i = 1; i < points_count; i++)
		{
			cairo_line_to(cr, points[i].x, points[i].y);
		}
		if (closed) { cairo_close_path(cr); }
	}
}

void draw_polylines(cairo_t* cr, const glm::vec2& pos, const glm::vec2* points, int points_count, int* idx, int idx_count, unsigned int col, float thickness)
{
	if (!cr || !points || points_count < 2 || idx_count < 2 || !col)return;
	cairo_save(cr);
	cairo_translate(cr, pos.x, pos.y);
	cairo_set_line_width(cr, thickness);
	int nc = 0;
	for (size_t i = 0; i < idx_count; i++)
	{
		auto x = idx[i];
		if (x < 0)
		{
			if (nc > 1) {
				set_color(cr, col);
				cairo_stroke(cr);
			}
			nc = 0;
			continue;
		}
		if (nc == 0)
			cairo_move_to(cr, points[x].x, points[x].y);
		else
			cairo_line_to(cr, points[x].x, points[x].y);
		nc++;
	}
	if (nc > 1) {
		set_color(cr, col);
		cairo_stroke(cr);
	}
	cairo_restore(cr);
}
void draw_rect(cairo_t* cr, const glm::vec4& rc, uint32_t fill, uint32_t color, double r, int linewidth)
{
	cairo_as _ss_(cr);
	draw_rectangle(cr, rc, r);
	fill_stroke(cr, fill, color, linewidth, false);
}
//glm::ivec2 layout_text_x::get_text_rect(size_t idx, const void* str8, int len, int fontsize)

void draw_text(cairo_t* cr, layout_text_x* ltx, const void* str, int len, glm::vec4 text_rc, text_style_t* st)
{
	glm::vec4 rc = text_rc;
	ltx->tem_rtv.clear();
	ltx->build_text(st->font, rc, st->text_align, str, len, st->font_size, ltx->tem_rtv);
	ltx->update_text();

	cairo_as _ss_(cr);
	if (st->clip && text_rc.z > 0 && text_rc.w > 0) {
		draw_rectangle(cr, text_rc, 0);
		cairo_clip(cr);
	}
	if (st->text_color_shadow)
	{
		cairo_as _aa_(cr);
		cairo_translate(cr, st->shadow_pos.x, st->shadow_pos.y);
		ltx->draw_text(cr, ltx->tem_rtv, st->text_color_shadow);
	}
	//ltx->draw_rect_rc(cr, ltx->tem_rtv, 0xffff8000);
	ltx->draw_text(cr, ltx->tem_rtv, st->text_color);
}
void draw_rctext(cairo_t* cr, layout_text_x* ltx, text_tx* p, int count, text_style_tx* stv, int st_count, glm::ivec4* clip)
{
	if (!stv || st_count < 1 || !cr || !ltx || !p || count < 1)return;
	cairo_as _ss_(cr);
	if (clip && clip->z > 0 && clip->w > 0) {
		glm::vec4 cliprc = *clip;
		draw_rectangle(cr, cliprc, 0);
		cairo_clip(cr);
	}
	for (size_t i = 0; i < count; i++)
	{
		auto& it = p[i];
		auto idx = it.st_index < st_count ? it.st_index : 0;
		auto& st = stv[idx];
		float pad = st.rcst.thickness > 1 ? 0.0 : -0.5;
		auto ss = it.rc;
		auto draw_sbox = ss.z > 0 && ss.w > 0;
		if (draw_sbox)
		{
			ss.x += pad; ss.y += pad;
			auto r = ss.z < st.rcst.round * 2 || ss.w < st.rcst.round * 2 ? 0 : st.rcst.round;
			draw_rectangle(cr, ss, r);
			fill_stroke(cr, &st.rcst);
		}
		draw_text(cr, ltx, it.txt, it.len, it.trc, &st.st);
	}
}
void clip_cr(cairo_t* cr, const glm::ivec4& clip)
{
	glm::vec4 cliprc = clip;
	if (clip.z > 0 && clip.w > 0) {
		draw_rectangle(cr, cliprc, 0);
		cairo_clip(cr);
	}
}
void draw_ge(cairo_t* cr, void* p, int count)
{
	auto t = (char*)p;
	if (!(t && p && cr && count > 0))return;
	for (size_t i = 0; i < count; i++)
	{
		auto type = ((rect_b*)t)->type;
		switch (type)
		{
		case eg_e::e_rect:
		{
			auto dp = (rect_b*)t;
			if (dp->r.x > 0 && dp->r.y < 0)
			{
				draw_rect(cr, dp->rc, dp->fill, dp->color, dp->r.x, dp->thickness);
			}
			else {
				draw_round_rectangle(cr, dp->rc, dp->r);
				fill_stroke(cr, dp->fill, dp->color, dp->thickness);
			}
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_text:
		{
			auto dp = (text_b*)t;
			draw_text(cr, dp->ltx, dp->str, dp->len, dp->text_rc, dp->st);
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_circle:
		{
			auto dp = (circle_b*)t;
			draw_circle(cr, dp->pos, dp->r);
			fill_stroke(cr, dp->fill, dp->color, dp->thickness);
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_ellipse:
		{
			auto dp = (ellipse_b*)t;
			draw_ellipse(cr, dp->pos, dp->r);
			fill_stroke(cr, dp->fill, dp->color, dp->thickness);
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_triangle:
		{
			auto dp = (triangle_b*)t;
			draw_triangle(cr, dp->pos, dp->size, dp->dirspos);
			fill_stroke(cr, dp->fill, dp->color, dp->thickness);
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_polyline:
		{
			auto dp = (polyline_b*)t;
			{
				cairo_as _ss_(cr);
				cairo_translate(cr, dp->pos.x, dp->pos.y);
				draw_polyline(cr, dp->points, dp->points_count, dp->closed);
				fill_stroke(cr, dp->fill, dp->color, dp->thickness);
			}
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_polylines:
		{
			auto dp = (polylines_b*)t;
			draw_polylines(cr, dp->pos, dp->points, dp->points_count, dp->idx, dp->idx_count, dp->color, dp->thickness);
			t += sizeof(dp[0]);
		}
		break;
		case eg_e::e_image:
		{
			auto dp = (image_b*)t;
			if (dp->sliced.x < 1)
			{
				draw_image(cr, (cairo_surface_t*)dp->image, dp->pos, dp->rc, dp->color, dp->dsize);
			}
			else
			{
				draw_image(cr, (cairo_surface_t*)dp->image, dp->pos, dp->rc, dp->color, dp->dsize, dp->sliced);
			}
			t += sizeof(dp[0]);
		}
		break;
		default:
			break;
		}
	}
}

cairo_surface_t* new_clip_rect(int r)
{
	glm::vec4 rc = { 0,0,r * 4,r * 4 };
	//auto d = new uint32_t[rc.z * rc.w];
	auto rrc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, rc.z, rc.w);
	//auto rrc = cairo_image_surface_create_for_data((unsigned char*)d, CAIRO_FORMAT_ARGB32, rc.z, rc.w, rc.z * 4);
	auto cr = cairo_create(rrc);
	set_color(cr, -1);
	draw_rectangle(cr, rc, r);
	cairo_fill(cr);
	cairo_destroy(cr);
	return rrc;
}
void clip_rect(cairo_t* cr, cairo_surface_t* r)
{
	if (!cr || !r)return;
	cairo_save(cr);
	auto a = cairo_get_target(cr);
	glm::vec2 aw = { cairo_image_surface_get_width(a), cairo_image_surface_get_height(a) };
	cairo_set_operator(cr, CAIRO_OPERATOR_DEST_IN);
	float w = cairo_image_surface_get_width(r);
	float h = cairo_image_surface_get_height(r);
	glm::vec2 w2 = { w * 0.5, h * 0.5 };

	cairo_save(cr);
	cairo_rectangle(cr, 0, 0, w2.x, w2.y);//左上角
	cairo_clip(cr);
	cairo_new_path(cr);
	cairo_set_source_surface(cr, r, 0, 0);
	cairo_paint(cr);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_rectangle(cr, aw.x - w2.x, 0, w2.x, w2.y);//右上角
	cairo_clip(cr);
	cairo_new_path(cr);
	cairo_set_source_surface(cr, r, aw.x - w, 0);
	cairo_paint(cr);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_rectangle(cr, 0, aw.y - w2.y, w2.x, w2.y);//左下角
	cairo_clip(cr);
	cairo_new_path(cr);
	cairo_set_source_surface(cr, r, 0, aw.y - h);
	cairo_paint(cr);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_rectangle(cr, aw.x - w2.x, aw.y - w2.y, w2.x, w2.y);//右下角
	cairo_clip(cr);
	cairo_new_path(cr);
	cairo_set_source_surface(cr, r, aw.x - w, aw.y - h);
	cairo_paint(cr);
	cairo_restore(cr);

	cairo_restore(cr);
}
struct stops_t {
	float o;
	float x, y, z, w;
};
/*
用 bezier curve（贝塞尔曲线） 来设置 color stop（颜色渐变规则），
这里使用下面的曲线形式，其中
X轴为 offset（偏移量，取值范围为 0~1，0 代表阴影绘制起点），
Y轴为 alpha（颜 色透明度，取值范围为0~1，0 代表完全透明），
*/

void draw_rectangle_gradient(cairo_t* cr, int width, int height, const rect_shadow_t& rs)
{
	std::vector<stops_t> color_stops;
	float radius = rs.radius;
	if (radius < 1 || width < 2 || height < 2)
		return;
	auto bv = get_bezier(&rs.cubic, 1, rs.segment > 3 ? rs.segment : 3);
	for (size_t i = 0; i < bv.size(); i++)
	{
		float ratio = bv[i].x;
		float a = bv[i].y;
		glm::vec4 color = mix_colors(rs.cfrom, rs.cto, ratio);
		color_stops.push_back({ ratio,color.x, color.y, color.z, color.w * a });
	}
	if (width < 2 * radius)
	{
		width = 2 * radius;
	}
	if (height < 2 * radius)
	{
		height = 2 * radius;
	}

	//# radial gradient center points for four corners, top - left, top - right, etc
	glm::vec2 corner_tl = glm::vec2(radius, radius);
	glm::vec2 corner_tr = glm::vec2(width - radius, radius);
	glm::vec2 corner_bl = glm::vec2(radius, height - radius);
	glm::vec2 corner_br = glm::vec2(width - radius, height - radius);
	std::vector<glm::vec2> corner_points = { corner_tl, corner_tr, corner_br, corner_bl };

	//# linear gradient rectangle info for four sides
	glm::vec4 side_top = glm::vec4(radius, 0, width - 2 * radius, radius);
	glm::vec4 side_bottom = glm::vec4(radius, height - radius, width - 2 * radius, radius);
	glm::vec4 side_left = glm::vec4(0, radius, radius, height - 2 * radius);
	glm::vec4 side_right = glm::vec4(width - radius, radius, radius, height - 2 * radius);

	//# draw four corners through radial gradient
	int i = 0;
	for (auto& point : corner_points)
	{
		cairo_pattern_t* rg = cairo_pattern_create_radial(point[0], point[1], 0, point[0], point[1], radius);
		for (auto& stop : color_stops)
		{
			cairo_pattern_add_color_stop_rgba(rg, stop.o, stop.x, stop.y, stop.z, stop.w);
		}
		cairo_move_to(cr, point[0], point[1]);
		double	angle1 = M_PI + 0.5 * M_PI * i, angle2 = angle1 + 0.5 * M_PI;
		cairo_arc(cr, point[0], point[1], radius, angle1, angle2);
		cairo_set_source(cr, rg);
		cairo_fill(cr);
		cairo_pattern_destroy(rg);
		i++;
	}

	//# draw four sides through linear gradient
	//# top side

	cairo_pattern_t* lg_top = cairo_pattern_create_linear(side_top[0], side_top[1] + radius, side_top[0], side_top[1]);
	for (auto& stop : color_stops)
	{
		cairo_pattern_add_color_stop_rgba(lg_top, stop.o, stop.x, stop.y, stop.z, stop.w);
	}
	draw_rectangle(cr, side_top, 0);
	cairo_set_source(cr, lg_top);
	cairo_fill(cr);

	//# bottom side
	cairo_pattern_t* lg_bottom = cairo_pattern_create_linear(side_bottom[0], side_bottom[1], side_bottom[0], side_bottom[1] + radius);
	for (auto& stop : color_stops)
	{
		cairo_pattern_add_color_stop_rgba(lg_bottom, stop.o, stop.x, stop.y, stop.z, stop.w);
	}
	draw_rectangle(cr, side_bottom, 0);
	cairo_set_source(cr, lg_bottom);
	cairo_fill(cr);

	//# left side
	cairo_pattern_t* lg_left = cairo_pattern_create_linear(side_left[0] + radius, side_left[1], side_left[0], side_left[1]);
	for (auto& stop : color_stops)
	{
		cairo_pattern_add_color_stop_rgba(lg_left, stop.o, stop.x, stop.y, stop.z, stop.w);
	}
	draw_rectangle(cr, side_left, 0);
	cairo_set_source(cr, lg_left);
	cairo_fill(cr);

	//# right side
	cairo_pattern_t* lg_right = cairo_pattern_create_linear(side_right[0], side_right[1], side_right[0] + radius, side_right[1]);
	for (auto& stop : color_stops)
	{
		cairo_pattern_add_color_stop_rgba(lg_right, stop.o, stop.x, stop.y, stop.z, stop.w);
	}
	draw_rectangle(cr, side_right, 0);
	cairo_set_source(cr, lg_right);
	cairo_fill(cr);

	cairo_pattern_destroy(lg_top);
	cairo_pattern_destroy(lg_bottom);
	cairo_pattern_destroy(lg_left);
	cairo_pattern_destroy(lg_right);
}

cairo_as::cairo_as(cairo_t* p) :cr(p)
{
	if (cr)
		cairo_save(cr);
}

cairo_as::~cairo_as()
{
	if (cr)
		cairo_restore(cr);
}

canvas_dev::canvas_dev()
{

}



canvas_dev::~canvas_dev()
{
	if (surface)
		cairo_surface_destroy(surface);
	if (cr)
		cairo_destroy(cr);
}
canvas_dev* new_cairo(cairo_surface_t* surface, canvas_dev* p = 0)
{
	if (surface)
	{
		auto cr = cairo_create(surface);
		if (cr)
		{
			if (!p)
				p = new canvas_dev();
			p->surface = surface;
			p->cr = cr;
			p->pixel = (uint32_t*)cairo_image_surface_get_data(surface);
			p->dsize.x = cairo_image_surface_get_width(surface);
			p->dsize.y = cairo_image_surface_get_height(surface);
		}
	}
	return p;
}
canvas_dev* canvas_dev::new_cdev(const glm::ivec2& size, uint32_t* data)
{
	cairo_surface_t* surface = 0;
	canvas_dev* p = 0;
	if (!data)
	{
		p = new canvas_dev();
		if (!p)return p;
		p->tdata.resize(size.x * size.y);
		data = p->tdata.data();
	}
	if (size.x > 0 && size.y > 0 && data)
	{
		surface = cairo_image_surface_create_for_data((unsigned char*)data, CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
	}
	return new_cairo(surface, p);
}

canvas_dev* canvas_dev::new_cdev_svg(const glm::ivec2& size, const char* svgfn)
{
	auto surface = cairo_svg_surface_create(svgfn, size.x, size.y);
	return new_cairo(surface);
}

canvas_dev* canvas_dev::new_cdev_dc(void* hdc)
{
#ifdef _WIN32
	auto surface = cairo_win32_surface_create((HDC)hdc);// 根据HDC创建表面
#else
	cairo_surface_t* surface = 0;
#endif 
	return new_cairo(surface);
}

void canvas_dev::free_cdev(canvas_dev* p)
{
	if (p)delete p;
}

void canvas_dev::save_png(const char* fn)
{
	if (surface && fn && *fn)
		cairo_surface_write_to_png(surface, fn);
}
void canvas_dev::save()
{
	cairo_save(cr);
}

#define SP_RGBA32_R_U(v) ((v) & 0xff)
#define SP_RGBA32_G_U(v) (((v) >> 8) & 0xff)
#define SP_RGBA32_B_U(v) (((v) >> 16) & 0xff)
#define SP_RGBA32_A_U(v) (((v) >> 24) & 0xff)
#define SP_COLOR_U_TO_F(v) ((v) / 255.0)
#define SP_COLOR_F_TO_U(v) ((uint32_t) ((v) * 255. + .5))
#define SP_RGBA32_R_F(v) SP_COLOR_U_TO_F (SP_RGBA32_R_U (v))
#define SP_RGBA32_G_F(v) SP_COLOR_U_TO_F (SP_RGBA32_G_U (v))
#define SP_RGBA32_B_F(v) SP_COLOR_U_TO_F (SP_RGBA32_B_U (v))
#define SP_RGBA32_A_F(v) SP_COLOR_U_TO_F (SP_RGBA32_A_U (v))

glm::vec4 colorv4(uint32_t rgba) {
	return { SP_RGBA32_R_F(rgba), SP_RGBA32_G_F(rgba), SP_RGBA32_B_F(rgba), SP_RGBA32_A_F(rgba) };
}
glm::vec4 colorv4_bgr(uint32_t bgra) {
	return { SP_RGBA32_B_F(bgra), SP_RGBA32_G_F(bgra),SP_RGBA32_R_F(bgra), SP_RGBA32_A_F(bgra) };
}
void set_color(cairo_t* cr, uint32_t rgba)
{
	cairo_set_source_rgba(cr, SP_RGBA32_R_F(rgba),
		SP_RGBA32_G_F(rgba),
		SP_RGBA32_B_F(rgba),
		SP_RGBA32_A_F(rgba));
}
void set_color_bgr(cairo_t* cr, uint32_t c)
{
	cairo_set_source_rgba(cr,
		SP_RGBA32_B_F(c),
		SP_RGBA32_G_F(c),
		SP_RGBA32_R_F(c),
		SP_RGBA32_A_F(c));
}
void set_color_a(cairo_t* cr, uint32_t rgba, double a)
{
	cairo_set_source_rgba(cr, SP_RGBA32_R_F(rgba),
		SP_RGBA32_G_F(rgba),
		SP_RGBA32_B_F(rgba),
		SP_RGBA32_A_F(rgba) * a);
}
void set_source_rgba(cairo_t* cr, uint32_t rgba)
{
	cairo_set_source_rgba(cr, SP_RGBA32_R_F(rgba),
		SP_RGBA32_G_F(rgba),
		SP_RGBA32_B_F(rgba),
		SP_RGBA32_A_F(rgba));
}
void set_source_rgba(cairo_t* cr, glm::vec4 rgba)
{
	cairo_set_source_rgba(cr, rgba.x, rgba.y, rgba.z, rgba.w);
}
void set_source_rgba_x(cairo_t* cr, glm::vec4 rgba)
{
	cairo_set_source_rgba(cr, rgba.x * rgba.w, rgba.y * rgba.w, rgba.z * rgba.w, 1.0);
}

glm::vec2 v2xm3(const glm::vec2& v, const glm::mat3& m) {
	glm::vec3 ps(v, 1.0);
	return m * ps;
}

glm::vec4 get_boxm3(const glm::vec4& v, const glm::mat3& m) {
	glm::vec3 ps = { v.x,v.y,1.0 };
	glm::vec3 ps1 = { v.z,v.w ,1.0 };
	ps = m * ps;
	ps1 = m * ps1;
	return glm::vec4(ps.x, ps.y, ps1.x, ps1.y);
}


glm::ivec4 canvas_dev::get_text_extents(const void* str, int len, font_xi* fx)
{
	char* t = (char*)str;
	if (!str || !(*t))
	{
		return  glm::vec4();
	}
	if (len < 1)len = strlen(t);
	cairo_text_extents_t extents = {};
	cairo_select_font_face(cr, fx->family, (cairo_font_slant_t)fx->slant, (cairo_font_weight_t)fx->weight_bold);
	cairo_set_font_size(cr, fx->font_size);
	std::string str1((char*)str, len);
	cairo_text_extents(cr, (char*)str1.c_str(), &extents);
	return glm::vec4(extents.width, extents.height, extents.x_bearing, extents.y_bearing);
}

int fc1_f(float a)
{
	return a > 0 ? ceil(a) : floor(a);
}
glm::ivec4 fcv4(const glm::vec4& a)
{
	return { fc1_f(a.x), fc1_f(a.y), fc1_f(a.z), fc1_f(a.w) };
}
glm::ivec4 get_text_extents(cairo_t* cr, const void* str, int len, font_xi* fx)
{
	char* t = (char*)str;
	if (!str || !(*t))
	{
		return  glm::vec4();
	}
	if (len < 1)len = strlen(t);
	cairo_text_extents_t extents = {};
	cairo_select_font_face(cr, fx->family, (cairo_font_slant_t)fx->slant, (cairo_font_weight_t)fx->weight_bold);
	cairo_set_font_size(cr, fx->font_size);
	std::string str1((char*)str, len);
	cairo_text_extents(cr, (char*)str1.c_str(), &extents);
	auto r = glm::vec4((extents.width), extents.height, extents.x_bearing, extents.y_bearing);
	return fcv4(r);
}


void canvas_dev::begin(uint32_t color) {
	cairo_save(cr); set_clear(color);
}
void canvas_dev::end() {
	cairo_restore(cr);
}
void canvas_dev::set_pos(const glm::vec2& pos)
{
	cairo_translate(cr, pos.x, pos.y);
}
void canvas_dev::set_dscale(const glm::vec2& sc)
{
	cairo_surface_set_device_scale(surface, sc.x, sc.y);
}
void canvas_dev::set_dscale(double sc)
{
	cairo_surface_set_device_scale(surface, sc, sc);
}
void canvas_dev::set_dscalei(int& zoom)
{
	if (zoom < 1)zoom = 1;
	if (zoom > max_zoom)zoom = max_zoom;
	double sc = zoom / 100.0;
	cairo_surface_set_device_scale(surface, sc, sc);
}
void canvas_dev::set_scale(double sc)
{
	cairo_scale(cr, sc, sc);
}
void canvas_dev::set_scalei(int& zoom)
{
	//if (zoom < 1)zoom = 1;
	//if (zoom > max_zoom)zoom = max_zoom;
	double sc = zoom / 100.0;
	cairo_scale(cr, sc, sc);
}
uint32_t* canvas_dev::data()
{
	return surface ? (uint32_t*)cairo_image_surface_get_data(surface) : tdata.data();
}
void canvas_dev::set_clear(uint32_t color)
{
	if (pixel && cr)
	{
		if (color == 0) {
			memset(tdata.data(), 0, tdata.size() * sizeof(int));
		}
		else {
			for (auto& it : tdata) {
				it = color;
			}
		}
		// clear background
		//cairo_save(cr);
		//set_color(cr, color);
		//cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		//cairo_paint(cr);
		//cairo_restore(cr);
		//cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	}
}
/*
CAIRO_LINE_CAP_BUTT,	在起点（终点）开始（停止）直线
CAIRO_LINE_CAP_ROUND,	圆形结尾，圆心为终点
CAIRO_LINE_CAP_SQUARE	正方形结尾，正方形的中心是终点
CAIRO_LINE_JOIN_MITER,	尖角
CAIRO_LINE_JOIN_ROUND,	圆角
CAIRO_LINE_JOIN_BEVEL	截断			 */
template<class T>
void draw_path0(cairo_t* cr, T* p, style_path_t* st, glm::vec2 pos, glm::vec2 scale)
{
	if (!p || !st || (!st->fill_color && !st->stroke_color || !cr))return;
	auto t = p->v;
	bool stroke = false;
	cairo_save(cr);
	pos += st->pos;
	pos.x += p->x;
	pos.y += p->y;
	if (st->flip_y)
	{
		cairo_matrix_t flip_y = {};
		cairo_matrix_init(&flip_y, 1, 0, 0, -1, 0, 0); // 垂直翻转
		cairo_set_matrix(cr, &flip_y);
		pos.y = -pos.y;
	}
	cairo_translate(cr, pos.x, pos.y);
	if (st->scale.x > 0 && st->scale.y > 0)
	{
		cairo_scale(cr, st->scale.x, st->scale.y);
	}
	else if (scale.x > 0 && scale.y > 0)
	{
		cairo_scale(cr, scale.x, scale.y);
	}
	if (st->line_width > 0 && st->stroke_color)
	{
		stroke = true;
		cairo_set_line_width(cr, st->line_width);
		cairo_set_line_cap(cr, (cairo_line_cap_t)st->cap);
		cairo_set_line_join(cr, (cairo_line_join_t)st->join);
	}
	if (st->dash.dashes && st->dash.num_dashes)
	{
		cairo_set_dash(cr, st->dash.dashes, st->dash.num_dashes, st->dash.offset);
	}
#ifdef tsave__test
	std::vector<vertex_t16> tv16;
	std::vector<std::vector<vertex_t16>> v;
	tv16.reserve(p->count);
#endif
	auto mt = *t;
	auto xt = *t;
	for (size_t i = 0; i < p->count; i++, t++)
	{
		switch ((vte_e)t->type)
		{
		case vte_e::e_vmove:
		{
			if (i > 0)
			{
				if (xt.x == mt.x && xt.y == mt.y)
					cairo_close_path(cr);
#ifdef tsave__test
				v.push_back(tv16); tv16.clear();
#endif
			}
			mt = *t;
			cairo_move_to(cr, t->x, t->y);
		}break;
		case vte_e::e_vline:
		{
			cairo_line_to(cr, t->x, t->y);
		}break;
		case vte_e::e_vcurve:
		{
			static double dv = 2.0 / 3.0;
			auto p0 = glm::vec2(xt.x, xt.y);
			auto p1 = glm::vec2(t->cx, t->cy);
			auto p2 = glm::vec2(t->x, t->y);
			glm::vec2 c1, c2;
			{
#if 1
				c1 = p1 - p0; c1 *= dv; c1 += p0;
				c2 = p1 - p2; c2 *= dv; c2 += p2;
#else
				static double dv = 2.0 / 3.0;
				auto cx1 = p0.x + 2.0f / 3.0f * (p1.x - p0.x);
				auto cy1 = p0.y + 2.0f / 3.0f * (p1.y - p0.y);
				auto cx2 = p2.x + 2.0f / 3.0f * (p1.x - p2.x);
				auto cy2 = p2.y + 2.0f / 3.0f * (p1.y - p2.y);
				c1 = { cx1,cy1 }; c2 = { cx2,cy2 };
#endif
			}
			//	C0 = Q0
			//	C1 = Q0 + (2 / 3) (Q1 - Q0)
			//	C2 = Q2 + (2 / 3) (Q1 - Q2)
			//	C3 = Q2
			cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, t->x, t->y);
		}break;
		case vte_e::e_vcubic:
		{
			cairo_curve_to(cr, t->cx, t->cy, t->cx1, t->cy1, t->x, t->y);

		}break;
		default:
			break;
		}
#ifdef tsave__test
		tv16.push_back(*t);
#endif
		xt = *t;
	}
	if (p->count > 2)
	{
		if (xt.x == mt.x && xt.y == mt.y)
			cairo_close_path(cr);
	}

	if (st->fill_color)
	{
		set_color(cr, st->fill_color);
		if (stroke)
			cairo_fill_preserve(cr);
		else
			cairo_fill(cr);
	}
	if (stroke)
	{
		set_color(cr, st->stroke_color);
		cairo_stroke(cr);
	}
	cairo_restore(cr);
}


struct path_txf
{
	float x, y;	// 坐标
	int count;		// 数量
	path_v::vertex_tf* v;	// 端点 
};
void draw_path_v(cairo_t* cr, path_v* p, style_path_t* st)
{
	if (!p || !st || p->_data.empty())return;
	path_txf tf = {};
	tf.count = p->_data.size();
	tf.v = (path_v::vertex_tf*)p->data();
	tf.x = p->_pos.x;
	tf.y = p->_pos.y + p->_baseline;
	glm::vec2 pos = {}, sc = {};
	draw_path0(cr, &tf, st, pos, sc);
}
void canvas_dev::draw_path(path_v* p, style_path_t* st)
{
	if (!p || !st || p->_data.empty())return;
	draw_path_v(cr, p, st);
}

void canvas_dev::draw_circle(std::vector<std::vector<glm::vec2>>* ov, float r, style_path_t* st)
{
	if (!ov || ov->empty())return;
	path_v v;
	for (auto& it : *ov)
	{
		for (auto& ct : it)
		{
			v.addCircle(ct, r);
		}
	}
	draw_path(&v, st);
}
void canvas_dev::draw_rect(const glm::vec2& size, style_path_t* st)
{
	path_v v;
	v.addRect({ 0,0,size }, {});
	draw_path(&v, st);
}
void canvas_dev::draw_circle(std::vector<glm::vec2>* ov, float r, style_path_t* st)
{
	if (!ov || ov->empty())return;
	path_v v;
	for (auto& ct : *ov)
	{
		v.addCircle(ct, r);
	}
	draw_path(&v, st);
}
void canvas_dev::draw_line(std::vector<glm::vec2>* ov, style_path_t* st)
{
	if (!ov || ov->empty())return;
	bool stroke = st->line_width > 0 && st->stroke_color;
	if (stroke)
	{
		auto count = ov->size() / 2;
		cairo_save(cr);
		cairo_translate(cr, st->pos.x, st->pos.y);
		set_color(cr, st->stroke_color);

		cairo_set_line_width(cr, st->line_width);
		cairo_set_line_cap(cr, (cairo_line_cap_t)st->cap);
		cairo_set_line_join(cr, (cairo_line_join_t)st->join);

		auto t = ov->data();
		auto mt = *t;
		auto xt = *t;
		for (size_t i = 0; i < count; i++, t++)
		{
			mt = *t;
			cairo_move_to(cr, t->x, t->y);
			t++;
			cairo_line_to(cr, t->x, t->y);
			xt = *t;
		}

		cairo_stroke(cr);
		cairo_restore(cr);
	}
}
void canvas_dev::draw_line(const glm::vec4& t, uint32_t stroke_color, float linewidth)
{
	bool stroke = linewidth > 0 && stroke_color;
	if (stroke)
	{
		cairo_save(cr);
		set_color(cr, stroke_color);
		cairo_set_line_width(cr, linewidth);
		cairo_set_line_cap(cr, cairo_line_cap_t::CAIRO_LINE_CAP_SQUARE);
		cairo_set_line_join(cr, cairo_line_join_t::CAIRO_LINE_JOIN_BEVEL);
		cairo_move_to(cr, t.x, t.y);
		cairo_line_to(cr, t.z, t.w);
		cairo_stroke(cr);
		cairo_restore(cr);
	}
}
void canvas_dev::draw_line(const glm::vec2& t, const glm::vec2& t1, uint32_t stroke_color, float linewidth)
{
	bool stroke = linewidth > 0 && stroke_color;
	if (stroke)
	{
		cairo_save(cr);
		set_color(cr, stroke_color);
		cairo_set_line_width(cr, linewidth);
		cairo_set_line_cap(cr, cairo_line_cap_t::CAIRO_LINE_CAP_SQUARE);
		cairo_set_line_join(cr, cairo_line_join_t::CAIRO_LINE_JOIN_BEVEL);
		cairo_move_to(cr, t.x, t.y);
		cairo_line_to(cr, t1.x, t.y);
		cairo_stroke(cr);
		cairo_restore(cr);
	}
}
void canvas_dev::draw_line1(std::vector<glm::vec2>* ov, style_path_t* st)
{
	if (!ov || ov->empty())return;
	bool stroke = st->line_width > 0 && st->stroke_color;
	if (stroke)
	{
		auto count = ov->size();
		cairo_save(cr);
		cairo_translate(cr, st->pos.x, st->pos.y);
		set_color(cr, st->stroke_color);

		cairo_set_line_width(cr, st->line_width);
		cairo_set_line_cap(cr, (cairo_line_cap_t)st->cap);
		cairo_set_line_join(cr, (cairo_line_join_t)st->join);

		auto t = ov->data();
		auto mt = *t;
		auto xt = *t;
		cairo_move_to(cr, t->x, t->y);
		t++;
		for (size_t i = 1; i < count; i++, t++)
		{
			cairo_line_to(cr, t->x, t->y);
			xt = *t;
		}
		if (count > 2)
		{
			if (xt.x == mt.x && xt.y == mt.y)
				cairo_close_path(cr);
		}
		cairo_stroke(cr);
		cairo_restore(cr);
	}
}

void canvas_dev::draw_text(const void* str, const glm::vec2& pos, font_xi* fx)
{
	cairo_save(cr);
	cairo_select_font_face(cr, fx->family, (cairo_font_slant_t)fx->slant, fx->weight_bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
	auto opt = cairo_font_options_create();
	cairo_get_font_options(cr, opt);
	auto k = cairo_get_antialias(cr);
	auto k1 = cairo_font_options_get_antialias(opt);
	//cairo_set_antialias(cr, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_antialias(opt, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_subpixel_order(opt, CAIRO_SUBPIXEL_ORDER_RGB);
	cairo_font_options_set_color_mode(opt, CAIRO_COLOR_MODE_COLOR);
	cairo_set_font_options(cr, opt);
	cairo_move_to(cr, pos.x, pos.y);
	cairo_set_font_size(cr, fx->font_size);
	if (fx->color)
		set_color(cr, fx->color);
	cairo_show_text(cr, (char*)str);
	cairo_font_options_destroy(opt);
	cairo_restore(cr);
	//cairo_paint_with_alpha(cr, 0.83);
}


class cairo_layout_text
{
public:
	glm::ivec2 _size = {};	// 布局大小
	std::string _family;
	int _fontsize = 12;
	int _linewidth = 2;
	glm::ivec2 _color = {};
	glm::vec2 align = {};
	int baseline = 12;
	int baseline_raw = 12;
public:
	cairo_layout_text();
	~cairo_layout_text();
	void set_size(const glm::ivec2& ss);
	void set_style(const char* family, int fontsize, uint32_t color);
	void set_style(const char* family, int fontsize, const glm::ivec2& color, int linewidth);
	// x=0左，0.5中，1右。y也一样
	void set_align(const glm::vec2& a);
	// 渲染文本
	void draw_text(cairo_t* cr, const void* str, const glm::vec2& pos);
	// 渲染文字路径
	void draw_text_path(cairo_t* cr, const void* str, const glm::vec2& pos);

	glm::ivec4 get_text_ext(cairo_t* cr, const void* str);
private:

};
cairo_layout_text::cairo_layout_text()
{

}

cairo_layout_text::~cairo_layout_text()
{
}
void cairo_layout_text::set_size(const glm::ivec2& ss)
{
	_size = ss;
}
void cairo_layout_text::set_style(const char* family, int fontsize, uint32_t color) {
	if (family)_family = family;
	if (color)_color.y = color;
	if (fontsize > 0) _fontsize = fontsize;
}
void cairo_layout_text::set_style(const char* family, int fontsize, const glm::ivec2& color, int linewidth) {
	if (family)_family = family;
	_color = color;
	if (fontsize > 0) _fontsize = fontsize;
	if (linewidth > 0)_linewidth = linewidth;
}
void cairo_layout_text::set_align(const glm::vec2& a)
{
	align = a;
}
glm::ivec4 cairo_layout_text::get_text_ext(cairo_t* cr, const void* str)
{
	cairo_text_extents_t extents = {};
	cairo_font_extents_t fext = {};
	cairo_select_font_face(cr, _family.c_str(), cairo_font_slant_t::CAIRO_FONT_SLANT_NORMAL, /*fx->weight_bold ? CAIRO_FONT_WEIGHT_BOLD : */CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, _fontsize);
	cairo_text_extents(cr, (char*)str, &extents);
	cairo_font_extents(cr, &fext);
	glm::vec2 bearing = { extents.x_bearing, extents.y_bearing };
	glm::vec2 ext = { extents.width,extents.height };
	baseline_raw = fext.ascent;	// 原始基线
	glm::vec2 ss = _size;
	auto ps = ss * align - (ext * align + bearing);
	baseline = ps.y;  // 调整后基线
	return { ps, extents.width, extents.height };
}
void cairo_layout_text::draw_text(cairo_t* cr, const void* str, const glm::vec2& pos)
{
	auto bx = get_text_ext(cr, str);
	cairo_save(cr);
	cairo_move_to(cr, pos.x + bx.x, pos.y + bx.y);
	if (_color.y != 0)
		set_color(cr, _color.y);
	cairo_show_text(cr, (char*)str);
	cairo_restore(cr);
}
// 渲染文字路径
void cairo_layout_text::draw_text_path(cairo_t* cr, const void* str, const glm::vec2& pos)
{
	auto bx = get_text_ext(cr, str);
	cairo_save(cr);
	cairo_move_to(cr, pos.x + bx.x, pos.y + bx.y);
	cairo_text_path(cr, (char*)str);
	if (_color.x != 0)
	{
		set_color(cr, _color.x);
		if (_color.y != 0)
			cairo_fill_preserve(cr);
		else {
			cairo_fill(cr);
		}
	}
	if (_color.y != 0)
		set_color(cr, _color.y);
	cairo_set_line_width(cr, _linewidth);
	cairo_stroke(cr);
	cairo_restore(cr);
}

glm::ivec4 get_text_align(cairo_t* cr, const char* str, const glm::vec2& pos, const glm::vec2& boxsize, const glm::vec2& text_align, const char* family, int font_size)
{
	cairo_text_extents_t extents = {};
	if (family) {
		cairo_select_font_face(cr, family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	}
	if (font_size > 0)
		cairo_set_font_size(cr, font_size);
	cairo_text_extents(cr, str, &extents);
	glm::vec2 bearing = { extents.x_bearing, extents.y_bearing };
	glm::vec2 ext = { extents.width,extents.height };
	glm::vec2 ss = boxsize;
	auto ps = ss * text_align - (ext * text_align + bearing);
	ps += pos;
	auto r = glm::vec4(ps, extents.width, extents.height);
	return fcv4(r);
}

void draw_text(cairo_t* cr, const char* str, const glm::ivec4& et, uint32_t text_color)
{
	if (text_color)
	{
		set_color(cr, text_color);
		cairo_move_to(cr, et.x, et.y);
		cairo_show_text(cr, str); // 文本渲染 
	}
}

glm::ivec4 draw_text_align(cairo_t* cr, const char* str, const glm::vec2& pos, const glm::vec2& boxsize, const glm::vec2& text_align, uint32_t text_color, const char* family, int font_size)
{
	cairo_text_extents_t extents = {};
	if (family) {
		cairo_select_font_face(cr, family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	}
	if (font_size > 0)
		cairo_set_font_size(cr, font_size);
	cairo_text_extents(cr, str, &extents);
	glm::vec2 bearing = { extents.x_bearing, extents.y_bearing };
	glm::vec2 ext = { extents.width, extents.height };
	glm::vec2 ss = boxsize;
	auto ps = pos;
	if (ss.x > 0 || ss.y > 0)
		ps += ss * text_align - (ext * text_align + bearing);
	if (text_color)
	{
		set_color(cr, text_color);
		cairo_move_to(cr, ps.x, ps.y);
		cairo_show_text(cr, str); // 文本渲染 
	}
	return { ps, extents.x_advance, extents.y_advance };
}
inline int multiply_alpha(int alpha, int color)
{
	int temp = (alpha * color) + 0x80;
	return ((temp + (temp >> 8)) >> 8);
}
// 预乘输出bgra，type=0为原数据是rgba
void premultiply_data(int w, unsigned char* data, int type, bool multiply)
{
	for (size_t i = 0; i < w; i += 4) {
		uint8_t* base = &data[i];
		uint8_t  alpha = base[3];
		uint32_t p;

		if (alpha == 0) {
			p = 0;
		}
		else {
			uint8_t  red = base[0];
			uint8_t  green = base[1];
			uint8_t  blue = base[2];

			if (alpha != 0xff && multiply) {
				red = multiply_alpha(alpha, red);
				green = multiply_alpha(alpha, green);
				blue = multiply_alpha(alpha, blue);
			}
			if (type == 0)
				p = ((uint32_t)alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
			else
				p = ((uint32_t)alpha << 24) | (blue << 16) | (green << 8) | (red << 0);
		}
		memcpy(base, &p, sizeof(uint32_t));
	}
}
glm::ivec2 get_surface_size(cairo_surface_t* p) {
	return { cairo_image_surface_get_width(p), cairo_image_surface_get_height(p) };
}
glm::vec2 draw_image(cairo_t* cr, cairo_surface_t* image, const glm::vec2& pos, const glm::vec4& rc, uint32_t color, const glm::vec2& dsize)
{
	glm::vec2 ss = { rc.z, rc.w };
	if (ss.x < 0)
	{
		ss.x = cairo_image_surface_get_width(image);
	}
	if (ss.y < 0)
	{
		ss.y = cairo_image_surface_get_height(image);
	}
	if (ss.x > 0 && ss.y > 0)
	{
		glm::vec2 sc = { 1,1 };
		if (dsize.x > 0 && dsize.y > 0) {
			sc = dsize / ss;
		}
		if (color > 0 && color != -1)
		{
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_translate(cr, pos.x, pos.y);
			if (sc.x != 1 || sc.y != 1)
				cairo_scale(cr, sc.x, sc.y);
			cairo_rectangle(cr, 0, 0, ss.x, ss.y);
			cairo_clip(cr);
			set_color(cr, color);
			cairo_mask_surface(cr, image, -rc.x, -rc.y);
			cairo_restore(cr);
		}
		else {
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_translate(cr, pos.x, pos.y);
			if (sc.x != 1 || sc.y != 1)
				cairo_scale(cr, sc.x, sc.y);
			cairo_set_source_surface(cr, image, -rc.x, -rc.y);
			cairo_rectangle(cr, 0, 0, ss.x, ss.y);
			cairo_fill(cr);
			cairo_restore(cr);
		}
	}
	return ss;
}

glm::vec2 draw_image(cairo_t* cr, cairo_surface_t* image, const glm::vec2& pos, const glm::vec4& rc, uint32_t color, const glm::vec2& size, const glm::vec4& sliced)
{
	glm::vec2 ss = { rc.z, rc.w };
	if (ss.x < 0)
	{
		ss.x = cairo_image_surface_get_width(image);
	}
	if (ss.y < 0)
	{
		ss.y = cairo_image_surface_get_height(image);
	}
#if 0
	cairo_as _a(cr);
	//cairo_scale(cr, scale.x, scale.y);
	// 上层
	{
		glm::vec2 v[] = { {sliced.x,sliced.y}, {ss.x - (sliced.x + sliced.z),sliced.y }, { sliced.z,sliced.y } };
		glm::vec2 vpos[3] = { {0,0},{sliced.x,0},{sliced.x + v[1].x,0} };
		for (size_t i = 0; i < 3; i++)
		{
			glm::vec4 rc0 = { rc.x + vpos[i].x,rc.y + vpos[i].y,v[i].x,v[i].y };
			draw_image(cr, image, vpos[i] + pos, rc0, color);
		}
	}
	// 中层
	{
		glm::vec2 v[] = { {sliced.x,ss.y - (sliced.y + sliced.w)}, { ss.x - (sliced.x + sliced.z),ss.y - (sliced.y + sliced.w) }, { sliced.z,ss.y - (sliced.y + sliced.w) } };
		glm::vec2 vpos[3] = { {0,sliced.y},{sliced.x,sliced.y},{sliced.x + v[1].x,sliced.y} };
		for (size_t i = 0; i < 3; i++)
		{
			glm::vec4 rc0 = { rc.x + vpos[i].x,rc.y + vpos[i].y,v[i].x,v[i].y };
			draw_image(cr, image, vpos[i] + pos, rc0, color);
		}
	}
	// 下层
	{
		glm::vec2 v[] = { {sliced.x,sliced.w},{ss.x - (sliced.x + sliced.z),sliced.w},{sliced.z,sliced.w} };
		glm::vec2 vpos[3] = { {0,ss.y - sliced.w},{sliced.x,ss.y - sliced.w},{sliced.x + v[1].x,ss.y - sliced.w} };
		for (size_t i = 0; i < 3; i++)
		{
			glm::vec4 rc0 = { rc.x + vpos[i].x,rc.y + vpos[i].y,v[i].x,v[i].y };
			draw_image(cr, image, vpos[i] + pos, rc0, color);
		}
	}
#endif
	return ss;
}
int64_t get_rdev() {
	static int64_t r = std::chrono::system_clock::now().time_since_epoch().count();
	return r;
}
int get_rand(int f, int s)
{
	static std::mt19937 gen(get_rdev()); //gen是一个使用rd()作种子初始化的标准梅森旋转算法的随机数发生器
	std::uniform_int_distribution<> distrib(f, s);
	return distrib(gen);
}
int64_t get_rand64(int64_t f, int64_t s)
{
	static std::mt19937_64 gen(get_rdev()); //gen是一个使用rd()作种子初始化的标准梅森旋转算法的随机数发生器
	auto d = gen();
	return f + d % (s - f + 1);
}
void destroy_image_data(void* d) {
	auto p = (uint32_t*)d;
	if (d)delete[]p;
}

cairo_surface_t* new_image_cr(image_ptr_t* img)
{
	cairo_surface_t* image = 0;
	if (img->stride < 1)img->stride = img->width * sizeof(uint32_t);
	auto px = new uint32_t[img->width * img->height];
	image = cairo_image_surface_create_for_data((unsigned char*)px, CAIRO_FORMAT_ARGB32, img->width, img->height, img->width * sizeof(int));
	if (image)
	{
		image_set_ud(image, key_def_data, px, destroy_image_data);
		memcpy(px, (unsigned char*)img->data, img->height * img->width * sizeof(int));
		if (img->multiply && img->type == 1)
		{
		}
		else {
			int stride = cairo_image_surface_get_stride(image);
			auto data = cairo_image_surface_get_data(image);
			auto t = data;
			auto ts = (unsigned char*)img->data;
			for (size_t i = 0; i < img->height; i++)
			{
				premultiply_data(img->width * 4, t, img->type, !img->multiply);
				t += stride;
				ts += img->stride;
			}
			//#define _DEBUG
			//			save_img_png(img, "update_text_img.png");
			//			cairo_surface_write_to_png(image, "update_text_surface.png");
			//#endif
		}
	}
	else {
		delete[]px;
	}
	return image;
}
cairo_surface_t* new_image_cr(const glm::ivec2& size, uint32_t* data)
{
	cairo_surface_t* image = 0;
	image_ptr_t img[1] = {};
	img->width = size.x;
	img->height = size.y;
	if (img->stride < 1)img->stride = img->width * sizeof(uint32_t);
	auto px = data ? data : new uint32_t[img->width * img->height];
	image = cairo_image_surface_create_for_data((unsigned char*)px, CAIRO_FORMAT_ARGB32, img->width, img->height, img->width * sizeof(int));
	if (image)
	{
		if (!data)
			image_set_ud(image, key_def_data, px, destroy_image_data);
		memset(px, 0, img->width * img->height * sizeof(int));
	}
	else {
		delete[]px;
	}
	return image;
}
void update_image_cr(cairo_surface_t* image, image_ptr_t* img)
{
	if (img->stride < 1)img->stride = img->width * sizeof(uint32_t);
	if (img->multiply && img->type == 1)
	{
		auto data = cairo_image_surface_get_data(image);
		memcpy(data, (unsigned char*)img->data, img->height * img->width * sizeof(int));
	}
	else {
		int stride = cairo_image_surface_get_stride(image);
		auto data = cairo_image_surface_get_data(image);
		memcpy(data, (unsigned char*)img->data, img->height * img->width * sizeof(int));
		auto t = data;
		auto ts = (unsigned char*)img->data;
		for (size_t i = 0; i < img->height; i++)
		{
			premultiply_data(img->width * 4, t, img->type, !img->multiply);
			t += stride;
			ts += img->stride;
		}
	}
}
void free_image_cr(cairo_surface_t* image)
{
	if (image)
	{
		cairo_surface_destroy(image);
	}
}

void image_set_ud(cairo_surface_t* p, uint64_t key, void* ud, void(*destroy_func)(void* data))
{
	if (p && key) {
		cairo_surface_set_user_data(p, (cairo_user_data_key_t*)key, ud, destroy_func);
	}
}

void* image_get_ud(cairo_surface_t* p, uint64_t key)
{
	return cairo_surface_get_user_data(p, (cairo_user_data_key_t*)key);
}

void image_save_png(cairo_surface_t* surface, const char* fn)
{
	if (surface && fn && *fn)
		cairo_surface_write_to_png(surface, fn);
}




#if 0

static
struct pl {
	void fo() {
		pango_layout_new();
		//Create a new PangoLayout object with attributes initialized to default values for a particular PangoContext.

		pango_layout_deserialize();
		//Loads data previously created via pango_layout_serialize().

		//Instance methods
		pango_layout_context_changed();
		//Forces recomputation of any state in the PangoLayout that might depend on the layout’s context.

		pango_layout_copy();
		//Creates a deep copy - by - value of the layout.

		pango_layout_get_alignment();
		//Gets the alignment for the layout : how partial lines are positioned within the horizontal space available.

		pango_layout_get_attributes();
		//Gets the attribute list for the layout, if any.

		pango_layout_get_auto_dir();
		//Gets whether to calculate the base direction for the layout according to its contents.

		//since: 1.4

		pango_layout_get_baseline();
		//Gets the Y position of baseline of the first line in layout.

		pango_layout_get_caret_pos();
		//Given an index within a layout, determines the positions that of the strong and weak cursors if the insertion point is at that index.

		pango_layout_get_character_count();
		//Returns the number of Unicode characters in the the text of layout.

		pango_layout_get_context();
		//Retrieves the PangoContext used for this layout.

		pango_layout_get_cursor_pos();
		//Given an index within a layout, determines the positions that of the strong and weak cursors if the insertion point is at that index.

		pango_layout_get_direction();
		//Gets the text direction at the given character position in layout.

		pango_layout_get_ellipsize();
		//Gets the type of ellipsization being performed for layout.

		pango_layout_get_extents();
		//Computes the logical and ink extents of layout.

		pango_layout_get_font_description();
		//Gets the font description for the layout, if any.

		pango_layout_get_height();
		//Gets the height of layout used for ellipsization.

		pango_layout_get_indent();
		//Gets the paragraph indent width in Pango units.

		pango_layout_get_iter();
		//Returns an iterator to iterate over the visual extents of the layout.

		pango_layout_get_justify();
		//Gets whether each complete line should be stretched to fill the entire width of the layout.

		pango_layout_get_justify_last_line();
		//Gets whether the last line should be stretched to fill the entire width of the layout.

		pango_layout_get_line();
		//Retrieves a particular line from a PangoLayout.

		pango_layout_get_line_count();
		//Retrieves the count of lines for the layout.

		pango_layout_get_line_readonly();
		//Retrieves a particular line from a PangoLayout.

		pango_layout_get_line_spacing();
		//Gets the line spacing factor of layout.

		pango_layout_get_lines();
		//Returns the lines of the layout as a list.

		pango_layout_get_lines_readonly();
		//Returns the lines of the layout as a list.

		pango_layout_get_log_attrs();
		//Retrieves an array of logical attributes for each character in the layout.

		pango_layout_get_log_attrs_readonly();
		//Retrieves an array of logical attributes for each character in the layout.

		pango_layout_get_pixel_extents();
		//Computes the logical and ink extents of layout in device units.

		pango_layout_get_pixel_size();
		//Determines the logical width and height of a PangoLayout in device units.

		pango_layout_get_serial();
		//Returns the current serial number of layout.

		pango_layout_get_single_paragraph_mode();
		//Obtains whether layout is in single paragraph mode.

		pango_layout_get_size();
		//Determines the logical width and height of a PangoLayout in Pango units.

		pango_layout_get_spacing();
		//Gets the amount of spacing between the lines of the layout.

		pango_layout_get_tabs();
		//Gets the current PangoTabArray used by this layout.

		pango_layout_get_text();
		//Gets the text in the layout.

		pango_layout_get_unknown_glyphs_count();
		//Counts the number of unknown glyphs in layout.

		pango_layout_get_width();
		//Gets the width to which the lines of the PangoLayout should wrap.

		pango_layout_get_wrap();
		//Gets the wrap mode for the layout.

		pango_layout_index_to_line_x();
		//Converts from byte index_ within the layout to line and X position.

		pango_layout_index_to_pos();
		//Converts from an index within a PangoLayout to the onscreen position corresponding to the grapheme at that index.

		pango_layout_is_ellipsized();
		//Queries whether the layout had to ellipsize any paragraphs.

		pango_layout_is_wrapped();
		//Queries whether the layout had to wrap any paragraphs.

		pango_layout_move_cursor_visually();
		//Computes a new cursor position from an old position and a direction.

		pango_layout_serialize();
		//Serializes the layout for later deserialization via pango_layout_deserialize().

		pango_layout_set_alignment();
		//Sets the alignment for the layout : how partial lines are positioned within the horizontal space available.

		pango_layout_set_attributes();
		//Sets the text attributes for a layout object.

		pango_layout_set_auto_dir();
		//Sets whether to calculate the base direction for the layout according to its contents.


		pango_layout_set_ellipsize();
		//Sets the type of ellipsization being performed for layout.


		pango_layout_set_font_description();
		//Sets the default font description for the layout.

		pango_layout_set_height();
		//Sets the height to which the PangoLayout should be ellipsized at.


		pango_layout_set_indent();
		//Sets the width in Pango units to indent each paragraph.

		pango_layout_set_justify();
		//Sets whether each complete line should be stretched to fill the entire width of the layout.

		pango_layout_set_justify_last_line();
		//Sets whether the last line should be stretched to fill the entire width of the layout.


		pango_layout_set_line_spacing();
		//Sets a factor for line spacing.


		pango_layout_set_markup();
		//Sets the layout text and attribute list from marked - up text.

		pango_layout_set_markup_with_accel();
		//Sets the layout text and attribute list from marked - up text.

		pango_layout_set_single_paragraph_mode();
		//Sets the single paragraph mode of layout.

		pango_layout_set_spacing();
		//Sets the amount of spacing in Pango units between the lines of the layout.

		pango_layout_set_tabs();
		//Sets the tabs to use for layout, overriding the default tabs.

		pango_layout_set_text();
		//Sets the text of the layout.

		pango_layout_set_width();
		//Sets the width to which the lines of the PangoLayout should wrap or ellipsized.

		//@PANGO_WRAP_WORD: wrap lines at word boundaries.
		//* @PANGO_WRAP_CHAR : wrap lines at character boundaries.
		//* @PANGO_WRAP_WORD_CHAR : wrap lines at word boundaries, but fall back to  character boundaries if there is not enough space for a full word.
		pango_layout_set_wrap();
		//Sets the wrap mode.

		pango_layout_write_to_file();
		//A convenience method to serialize a layout to a file.


		pango_layout_xy_to_index();
		//Converts from X and Y position within a layout to the byte index to the character at that logical position.
	}
};

#endif

class layout_px
{
public:
	PangoLayout* layout = 0, * fgp = 0;
	glm::vec4 extents = {};
	PangoRectangle r0 = {}, r1 = {};
	int y = 0;
	layout_px();
	~layout_px();
	void draw(cairo_t* cr) {
		pango_cairo_update_layout(cr, layout);
		//cairo_move_to(cr, 0 /*extents.x*/, r0.y - (r1.height - r0.height) * 0.5);
		cairo_move_to(cr, 0, 0);
		pango_cairo_show_layout(cr, layout);
	}
	void get_pixel_size(int& w, int& h)
	{
		w = extents.z;
		h = extents.w;
	}
private:

};

layout_px::layout_px()
{
}

layout_px::~layout_px()
{
	if (fgp)
		g_object_unref(fgp);
}

layout_px create_pango_layout(const char* str, int fs)
{

	PangoFontMap* fontMap = pango_cairo_font_map_get_default();
	//PangoFontMap* fontMap = pango_cairo_font_map_new();
	PangoContext* context = pango_font_map_create_context(fontMap);
	PangoLayout* layout = pango_layout_new(context);
	//PangoLayout* layout = pango_cairo_create_layout(cr);
	PangoFontDescription* desc;
	desc = pango_font_description_new();
	pango_font_description_set_family(desc, "Arial,NSimSun");
	//pango_font_description_set_family(desc, "NSimSun");
	pango_font_description_set_size(desc, fs * PANGO_SCALE);
	//desc = pango_font_description_from_string(FONT);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
	//pango_layout_set_markup(layout, str, strlen(str));
	//str = (char*)u8"😀工affab\na寺214668\n965";
	int sizeX = (strlen(str) + 6) * fs;
	//pango_layout_set_width(layout, sizeX * PANGO_SCALE);
	pango_layout_set_text(layout, str, -1);
	/* Draw the layout N_WORDS times in a circle */
	//pango_cairo_update_layout(cr, layout);
	layout_px r = {};
	pango_layout_get_pixel_extents(layout, &r.r0, &r.r1);
	int w = 0, h = 0;
	pango_layout_get_pixel_size(layout, &w, &h);
	int bl = (double)pango_layout_get_baseline(layout) / PANGO_SCALE;


	g_object_unref(context);

	r.y = bl;
	r.extents = { r.r0.x,r.r0.y,r.r1.width,r.r1.height };
	r.layout = layout;
	return r;
}
void draw_text(cairo_t* cr, const void* str, const glm::vec2& pos, font_xi* fx)
{
	cairo_save(cr);
#if 0
	draw_text(cr, (char*)str, pos, fx->font_size);
#else
	cairo_select_font_face(cr, fx->family, (cairo_font_slant_t)fx->slant, fx->weight_bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
	auto opt = cairo_font_options_create();
	cairo_get_font_options(cr, opt);
	auto k = cairo_get_antialias(cr);
	auto k1 = cairo_font_options_get_antialias(opt);
	//cairo_set_antialias(cr, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_antialias(opt, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_subpixel_order(opt, CAIRO_SUBPIXEL_ORDER_RGB);
	cairo_set_font_options(cr, opt);
	cairo_move_to(cr, pos.x, pos.y);
	cairo_set_font_size(cr, fx->font_size);
	if (fx->color)
		set_color(cr, fx->color);
	cairo_show_text(cr, (char*)str);
	cairo_font_options_destroy(opt);
#endif
	cairo_restore(cr);
	//cairo_paint_with_alpha(cr, 0.83);
}
void canvas_dev::draw_circle(const glm::vec2& pos, float r, float linewidth, uint32_t color, uint32_t fill)
{
	cairo_save(cr);
	bool stroke = linewidth > 0 && color;
	if (fill)
	{
		set_color(cr, fill);
		cairo_arc(cr, pos.x, pos.y, r, 0, 2 * M_PI);
		cairo_fill(cr);
	}
	if (stroke)
	{
		set_color(cr, color);
		cairo_arc(cr, pos.x, pos.y, r, 0, 2 * M_PI);
		cairo_stroke(cr);
	}
	cairo_restore(cr);
}
void canvas_dev::draw_surface(cairo_surface_t* image, const glm::vec2& pos, double alpha)
{
	cairo_set_source_surface(cr, image, pos.x, pos.y);
	cairo_paint_with_alpha(cr, alpha);
}
#if 1
int canvas_dev::get_path(path_v* pt)
{
	int r = 0;
	cairo_path_t* path;
	cairo_path_data_t* data;

	path = cairo_copy_path(cr);
	if (!path)return 0;
	r = path->num_data;
	for (int i = 0; i < path->num_data; i += path->data[i].header.length) {
		data = &path->data[i];
		switch (data->header.type) {
		case CAIRO_PATH_MOVE_TO:
			pt->moveTo(data[1].point.x, data[1].point.y);
			break;
		case CAIRO_PATH_LINE_TO:
			pt->lineTo(data[1].point.x, data[1].point.y);
			break;
		case CAIRO_PATH_CURVE_TO:
			pt->curveTo(data[1].point.x, data[1].point.y,
				data[2].point.x, data[2].point.y,
				data[3].point.x, data[3].point.y);
			break;
		case CAIRO_PATH_CLOSE_PATH:
			pt->closePath();
			break;
		}
	}
	cairo_path_destroy(path);
	return 0;
}
#endif
glm::vec4 canvas_dev::get_path_extents()
{
	double tc[4] = {};
	auto t = tc;
	if (cr)
		cairo_path_extents(cr, t, t + 1, t + 2, t + 3);
	return glm::vec4(tc[0], tc[1], tc[2], tc[3]);
}

#if 1

glm::vec4 get_merged_rect(glm::vec4* rects, int length)
{
	if (length == 0) {
		return {};
	}
	glm::vec4 r = rects[0];
	for (size_t i = 1; i < length; i++)
	{
		auto it = rects[i];
		r.x = std::min(r.x, it.x);
		r.y = std::min(r.y, it.y);
		r.z = std::max(r.z, it.z);
		r.w = std::max(r.w, it.w);
	}
	return r;
}
glm::vec4 get_bboxs(std::vector<path_v*>& v)
{
	std::vector<glm::vec4> bboxs;
	bboxs.reserve(v.size());
	for (auto it : v)
	{
		auto v4 = it->mkbox();
		bboxs.push_back(v4);
	}
	return get_merged_rect(bboxs.data(), bboxs.size());
}
#endif // 1

tinyviewcanvas_x::tinyviewcanvas_x()
{
}

tinyviewcanvas_x::~tinyviewcanvas_x()
{
	if (_backing_store)
	{
		cairo_surface_destroy(_backing_store);
	}
	_backing_store = 0;
}

void tinyviewcanvas_x::draw(canvas_dev* c) {

	if (!_backing_store_valid)
		draw_back();
	auto cr = c->cr;
	if (_backing_store)
	{
		cairo_save(cr);
		cairo_set_source_surface(cr, _backing_store, 0, 0);
		cairo_paint(cr);
		cairo_restore(cr);
	}
}
void tinyviewcanvas_x::set_size(const glm::ivec2& ss)
{
	if (ss != size)
	{
		size = ss;
		if (_backing_store)
		{
			cairo_surface_destroy(_backing_store);
			_backing_store = 0;
		}
	}
}

void tinyviewcanvas_x::set_view_move(bool is)
{
	has_move = is;
}

void tinyviewcanvas_x::set_view_scale(bool is)
{
	has_scale = is;
}



void tinyviewcanvas_x::draw_back() {
	if (!_data)return;
	print_time a("draw_back");
	if (!_backing_store)
		_backing_store = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.x, size.y);
	if (!_backing_store)return;
	// Get context
	cairo_t* cr = cairo_create(_backing_store);
	{
		cairo_save(cr);
		set_color(cr, 0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint(cr);
		cairo_restore(cr);

		//cairo_translate(cr, vpos.x, vpos.y);
		cairo_matrix_t t = {};
		auto a0 = mx[0];
		auto a1 = mx[1];
		auto a2 = mx[2];
		//auto a3 = mx[3];
		//cairo_matrix_init(&t, a0.x, 0, 0, a1.y, a3.x, a3.y); // 垂直翻转
		//cairo_set_matrix(cr, &t);
		cairo_translate(cr, a2.x + vpos.x, vpos.y + a2.y);

		style_path_t st0 = {};
		st0.line_width = line_width;
		st0.fill_color = color.x;
		st0.stroke_color = color.y;
		st0.cap = 0;
		st0.join = 1;
		st0.pos = { 0.5,0.5 };
		style_path_t* spt = &st0;
		if (oldscale != scale)
		{
			print_time a("canvas draw");
			auto m = get_affine();
			ddrect.clear();
			double sc = scale / 100.0;
			glm::mat3 sx = glm::scale(glm::mat3(1.0), glm::vec2(sc, sc));
			oldscale = scale;
			for (auto it : draw_data)
			{
				delete it;
			}
			draw_data.clear();
			for (auto it : *_data)
			{
				auto v23 = glm::vec3(it->_pos, 1.0);
				auto v2s = glm::vec3(it->get_size(), 1.0);
				glm::vec2 ips = mx * v23;// get_v2m4(it->_pos, &mx);
				glm::vec2 ss = mx * v2s;// get_v2m4(it->get_size(), &mx);
				auto nx = ips + ss;
				if (ips.x > size.x || ips.y > size.y || nx.x < -vpos.x * 2 || nx.y < -vpos.y * 2)
				{
					//continue;
				}
				auto npv = new path_v(*it);
				draw_data.push_back(npv);
				npv->_pos.y += npv->_baseline;//基线
				npv->_baseline = 0;
				npv->set_mat(sx);
				auto v4 = it->mkbox();
				ddrect.push_back(v4);
			}
			bx = get_bboxs(draw_data);
		}
		{
			print_time a("draw_path_v");
			for (auto it : draw_data) {
				draw_path_v(cr, it, spt);
			}
		}
	}
	_backing_store_valid = true;
	cairo_destroy(cr);

}

void tinyviewcanvas_x::on_button(int idx, int down, const glm::vec2& pos1, int clicks)
{
	auto pos = pos1 - (glm::vec2)vpos;
	//idx=1左，3右，2中
	if (idx == 1)
	{
		if (down == 1 && ckinc == 0)
		{
			glm::vec2 a3 = mx[2];
			last_mouse = pos - a3;
		}
		ckinc++;
		if (down == 0)
		{
			ckinc = 0;
		}

	}
	else if (idx == 3) {
		if (down == 0)
		{
			reset_view();
		}
	}
}
void tinyviewcanvas_x::on_motion(const glm::vec2& pos1)
{
	auto pos = pos1 - (glm::vec2)vpos;
	if (ckinc > 0)
	{
		if (has_move) {
			auto mp = pos;
			mp = pos - last_mouse;
			auto t = glm::translate(glm::mat3(1.0), mp);
			double sc = scale / 100.0;
			auto s = glm::scale(glm::mat3(1.0), glm::vec2(sc, sc));
			mx = t * s;
			_backing_store_valid = false;
		}
	}
	eoffset = pos;
	hit_test(pos1);
}
void tinyviewcanvas_x::on_wheel(int deltaY)
{
	if (has_scale)
	{
		auto prevZoom = scale;
		auto scale1 = scale;
		auto zoom = (deltaY * scaleStep);
		scale1 += zoom;
		if (scale1 < minScale) {
			scale = minScale;
			return;
		}
		else if (scale1 > maxScale) {
			scale = maxScale;
			return;
		}
		double sc = scale1 / 100.0;
		double sc1 = prevZoom / 100.0;
		glm::vec2 nps = mx[2];
		auto s = glm::scale(glm::mat3(1.0), glm::vec2(sc, sc));
		auto t = glm::translate(glm::mat3(1.0), glm::vec2(nps));
		mx = t * s;
		scale = scale1;
	}
	hit_test(eoffset + (glm::vec2)vpos);
	_backing_store_valid = false;
}
void tinyviewcanvas_x::reset_view()
{
	ckinc = 0;
	scale = 100;
	mx = glm::mat3(1.0);
	_backing_store_valid = false;
}

glm::ivec4 tinyviewcanvas_x::get_hbox()
{
	return hover_bx;
}
glm::vec4 tinyviewcanvas_x::get_bbox()
{
	return bx;
}

glm::mat3 tinyviewcanvas_x::get_affine()
{
	glm::mat3 r = glm::translate(glm::mat3(1.0), glm::vec2(vpos));
	return r * mx;
}

// 测试鼠标坐标的矩形
void tinyviewcanvas_x::hit_test(const glm::vec2& ps)
{
	auto ae = get_affine();
	auto m = glm::inverse(ae);//逆矩阵
	auto p2 = v2xm3(ps, m);	 //坐标和矩阵相乘
	auto k2 = check_box_cr(p2, ddrect.data(), ddrect.size());
	hover_bx = {};
	if (k2.x)
	{
		auto v4 = get_boxm3(ddrect[k2.y], ae);
		v4.z -= v4.x;
		v4.w -= v4.y;
		hover_bx = { glm::floor(v4.x),glm::floor(v4.y),glm::round(v4.z),glm::round(v4.w) };
	}
}

struct SPRulerMetric
{
	gdouble ruler_scale[16];
	gint    subdivide[5];
};

// Ruler metric for general use.
static SPRulerMetric const ruler_metric_general = {
  { 1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000 },
  { 1, 5, 10, 50, 100 }
};

// Ruler metric for inch scales.
static SPRulerMetric const ruler_metric_inches = {
  { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 },
  { 1, 2, 4, 8, 16 }
};

// Half width of pointer triangle.
static double half_width = 5.0;
// 将像素转换为厘米
double pixels_to_centimeters(int pixels, int dpi) {
	return (pixels / dpi) * 2.54;
}

// 将像素转换为毫米
double pixels_to_millimeters(int pixels, int dpi) {
	return (pixels / dpi) * 25.4;
}
// 将毫米转换为像素
double smillimeters_to_pixel(int mm, int dpi) {
	return (dpi / 25.4) * mm;
}

// 将像素转换为英寸
double pixels_to_inches(int pixels, int dpi) {
	return pixels / dpi;
}

Ruler::Ruler()
{

}

void Ruler::set_range(double lower, double upper)
{
	if (_lower != lower || _upper != upper) {

		_lower = lower;
		_upper = upper;
		_max_size = _upper - _lower;
		if (_max_size == 0) {
			_max_size = 1;
		}

		_backing_store_valid = false;
		//_drawing_area->queue_draw();
	}
}

void Ruler::set_page(double lower, double upper)
{
	if (_page_lower != lower || _page_upper != upper) {
		_page_lower = lower;
		_page_upper = upper;

		_backing_store_valid = false;
		//_drawing_area->queue_draw();
	}
}

void Ruler::set_selection(double lower, double upper)
{
	if (_sel_lower != lower || _sel_upper != upper) {
		_sel_lower = lower;
		_sel_upper = upper;

		_backing_store_valid = false;
		//_drawing_area->queue_draw();
	}
}

glm::ivec2 Ruler::get_drawing_size()
{
	return  drawingsize;
}
#define orientation_horizontal 0


#define getrgba(a,b,sp) lerp(a.sp,b.sp,ratio)

glm::dvec4 mix_colors(glm::vec4 a, glm::vec4 b, float ratio)
{
	auto lerp = [](double v0, double v1, double t) { return (1.0 - t) * v0 + t * v1; };
	glm::dvec4 result = {};
	result.x = getrgba(a, b, x);
	result.y = getrgba(a, b, y);
	result.z = getrgba(a, b, z);
	result.w = getrgba(a, b, w);
	return result;
}
cairo_pattern_t* create_cubic_gradient(
	glm::vec4 rect,
	glm::vec4 from,
	glm::vec4 to,
	glm::vec2 ctrl1,
	glm::vec2 ctrl2,
	glm::vec2 p0 = {},
	glm::vec2 p1 = { 1,1 },
	int steps = 8
);
cairo_pattern_t* create_cubic_gradient(
	glm::vec4 rect,
	glm::vec4 from,
	glm::vec4 to,
	glm::vec2 ctrl1,
	glm::vec2 ctrl2,
	glm::vec2 p0,
	glm::vec2 p1,
	int steps
) {
	// validate input points
	for (auto&& pt : { p0, ctrl1, ctrl2, p1 }) {
		if (pt.x < 0 || pt.x > 1 ||
			pt.y < 0 || pt.y > 1) {
			throw std::invalid_argument("Invalid points for cubic gradient; 0..1 coordinates expected.");
		}
	}
	if (steps < 2 || steps > 999) {
		throw std::invalid_argument("Invalid number of steps for cubic gradient; 2 to 999 steps expected.");
	}
	if (rect.x > rect.z)
	{
		std::swap(rect.x, rect.z);
	}
	if (rect.y > rect.w)
	{
		std::swap(rect.y, rect.w);
	}
	cairo_pattern_t* g = cairo_pattern_create_linear(rect.x, rect.y, rect.z, rect.w);

	//cairo_pattern_t* cairo_pattern_create_radial(double cx0, double cy0, double radius0, double cx1, double cy1, double radius1);

	--steps;
	for (int step = 0; step <= steps; ++step) {
		auto t = 1.0 * step / steps;
		auto s = 1.0 - t;
		auto p0t = p0;
		auto p1t = p1;
		auto c1 = ctrl1;
		auto c2 = ctrl2;
		p0t *= (t * t * t);
		c1 *= (3 * t * t * s);
		c2 *= (3 * t * s * s);
		p1t *= (s * s * s);
		//auto p = (t * t * t) * p0 + (3 * t * t * s) * ctrl1 + (3 * t * s * s) * ctrl2 + (s * s * s) * p1;
		auto p = p0t + c1 + c2 + p1t;
		auto offset = p.x;
		auto ratio = p.y;

		auto color = mix_colors(from, to, ratio);
		cairo_pattern_add_color_stop_rgba(g, offset, color.x, color.y, color.z, color.w);
	}

	return g;
}
void paint_shadow(cairo_t* cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, bool rev, float r)
{
	auto trans = shadow;
	trans.w = 0;
	auto gr = rev ?
		create_cubic_gradient(glm::vec4(0, 0, size_x, size_y), trans, shadow, glm::vec2(0.5, 0.0), glm::vec2(1.0, 0.5))
		: create_cubic_gradient(glm::vec4(0, 0, size_x, size_y), shadow, trans, glm::vec2(0, 0.5), glm::vec2(0.5, 1));
	//cairo_rectangle(cr, 0, 0, width, height);
	draw_rectangle(cr, { 0,0,width,height }, r);
	cairo_set_source(cr, gr);
	cairo_fill(cr);
	cairo_pattern_destroy(gr);
}
void paint_shadow(cairo_t* cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, const glm::vec4& color_to, bool rev, float r)
{
	auto gr = rev ?
		create_cubic_gradient(glm::vec4(0, 0, size_x, size_y), color_to, shadow, glm::vec2(0.5, 0.0), glm::vec2(1.0, 0.5))
		: create_cubic_gradient(glm::vec4(0, 0, size_x, size_y), shadow, color_to, glm::vec2(0, 0.5), glm::vec2(0.5, 1));
	//cairo_rectangle(cr, 0, 0, width, height);
	draw_rectangle(cr, { 0,0,width,height }, r);
	cairo_set_source(cr, gr);
	cairo_fill(cr);
	cairo_pattern_destroy(gr);
}


bool Ruler::draw_scale(cairo_t* cr_in)
{
	auto a = get_drawing_size();
	int awidth = a.x, aheight = a.y;
	// Create backing store (need surface_in to get scale factor correct).
	if (oldsize != drawingsize)
	{
		oldsize = drawingsize;
		auto surface_in = cairo_get_target(cr_in);
		if (_backing_store)
		{
			cairo_surface_destroy(_backing_store);
		}
		_backing_store = cairo_surface_create_similar_image(surface_in, CAIRO_FORMAT_ARGB32, awidth, aheight);
		//_backing_store = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, awidth, aheight);
	}
	if (!_backing_store)return false;
	// Get context
	cairo_t* cr = cairo_create(_backing_store);
	cairo_save(cr);
	set_color(cr, _back_color);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_restore(cr);

	// Color in page indication box
	if (has_page)
	{
		double psize = std::abs(_page_upper - _page_lower);
		if (psize > 0)
		{

			set_source_rgba(cr, _page_fill);
			cairo_new_path(cr);

			if (_orientation == orientation_horizontal) {
				cairo_rectangle(cr, _page_lower, 0, psize, aheight);
			}
			else {
				cairo_rectangle(cr, 0, _page_lower, awidth, is_yaxisdown > 0 ? psize : -psize);
			}
			cairo_fill(cr);
		}
	}

	cairo_set_line_width(cr, 1.0);

	// aparallel is the longer, oriented dimension of the ruler; aperpendicular shorter.
	auto const [aparallel, aperpendicular] = _orientation == orientation_horizontal
		? std::pair{ awidth , aheight }
	: std::pair{ aheight, awidth };

	// Draw bottom/right line of ruler
	set_source_rgba(cr, _foreground);
	if (_orientation == orientation_horizontal) {
		cairo_move_to(cr, 0, aheight - 0.5);
		cairo_line_to(cr, awidth, aheight - 0.5);
	}
	else {
		cairo_move_to(cr, awidth - 0.5, 0);
		cairo_line_to(cr, awidth - 0.5, aheight);
	}
	cairo_stroke(cr);

	// Draw a shadow which overlaps any previously painted object. 
	int gradient_size = 4;
	if (_orientation == orientation_horizontal) {
		paint_shadow(cr, 0, gradient_size, awidth, gradient_size, _shadow, 0);// 垂直方向
	}
	else {
		paint_shadow(cr, gradient_size, 0, gradient_size, aheight, _shadow, 0);// 水平方向
	}
	// Figure out scale. Largest ticks must be far enough apart to fit largest text in vertical ruler.
	// We actually require twice the distance.
	uint32_t scale = std::ceil(abs(_max_size)); // Largest number
	std::string scale_text = std::to_string(scale);
	uint32_t digits = scale_text.length() + 1; // Add one for negative sign.
	uint32_t minimum = digits * _font_size * 2;

	auto const pixels_per_unit = aparallel / _max_size; // pixel per distance

	SPRulerMetric ruler_metric = ruler_metric_general;
	if (ruler_in) {
		ruler_metric = ruler_metric_inches;
	}

	unsigned scale_index;
	for (scale_index = 0; scale_index < G_N_ELEMENTS(ruler_metric.ruler_scale) - 1; ++scale_index) {
		if (ruler_metric.ruler_scale[scale_index] * std::abs(pixels_per_unit) > minimum) break;
	}

	// Now we find out what is the subdivide index for the closest ticks we can draw
	unsigned divide_index;
	for (divide_index = 0; divide_index < G_N_ELEMENTS(ruler_metric.subdivide) - 1; ++divide_index) {
		if (ruler_metric.ruler_scale[scale_index] * std::abs(pixels_per_unit) < 5 * ruler_metric.subdivide[divide_index + 1]) break;
	}

	// We'll loop over all ticks.
	double pixels_per_tick = pixels_per_unit *
		ruler_metric.ruler_scale[scale_index] / ruler_metric.subdivide[divide_index];

	double units_per_tick = pixels_per_tick / pixels_per_unit;
	double ticks_per_unit = 1.0 / units_per_tick;

	// Find first and last ticks
	int start = 0;
	int end = 0;
	if (_lower < _upper) {
		start = std::floor(_lower * ticks_per_unit);
		end = std::ceil(_upper * ticks_per_unit);
	}
	else {
		start = std::floor(_upper * ticks_per_unit);
		end = std::ceil(_lower * ticks_per_unit);
	}

	// Loop over all ticks
	set_source_rgba(cr, _foreground);
	for (int i = start; i < end + 1; ++i) {

		// Position of tick (add 0.5 to center tick on pixel).
		double position = std::floor(i * pixels_per_tick - _lower * pixels_per_unit) + 0.5;

		// Height of tick
		int size = aperpendicular - 7;
		for (int j = divide_index; j > 0; --j) {
			if (i % ruler_metric.subdivide[j] == 0) break;
			size = size / 2 + 1;
		}

		// Draw text for major ticks.
		if (i % ruler_metric.subdivide[divide_index] == 0) {
			cairo_save(cr);

			int label_value = std::round(i * units_per_tick);

			auto& label = _label_cache[label_value];
			auto lv = std::to_string(label_value);// +(char*)u8"图";
			fx->font_size = _font_size;
			//auto ws = get_text_extents(cr, lv.c_str(), lv.size(), fx);
			if (!label) {
				label = draw_label(cr, lv, { /*ws.x,ws.y*/ });
			}
			// Align text to pixel
			int x = 3;// +_font_size * 0.5;
			int y = position + 2.5;
			if (_orientation == orientation_horizontal) {
				x = position + 2.5;
				y = 3;// +_font_size * 0.5;
			}

			// We don't know the surface height/width, damn you cairo.
			//cairo_rectangle(cr, x, y, 100, 100);
			//cairo_clip(cr);
			cairo_set_source_surface(cr, label, x, y);
			cairo_paint(cr);
			cairo_restore(cr);

		}

		// Draw ticks
		set_source_rgba(cr, _foreground);
		if (_orientation == orientation_horizontal) {
			cairo_move_to(cr, position, aheight - size);
			cairo_line_to(cr, position, aheight);
		}
		else {
			cairo_move_to(cr, awidth - size, position);
			cairo_line_to(cr, awidth, position);
		}
		cairo_stroke(cr);
	}

	// Draw a selection bar
	if (_sel_lower != _sel_upper && _sel_visible) {
		const auto radius = 3.0;
		const auto delta = _sel_upper - _sel_lower;
		const auto dxy = delta > 0 ? radius : -radius;
		double sy0 = _sel_lower;
		double sy1 = _sel_upper;
		double sx0 = floor(aperpendicular * 0.7);
		double sx1 = sx0;

		if (_orientation == orientation_horizontal) {
			std::swap(sy0, sx0);
			std::swap(sy1, sx1);
		}

		cairo_set_line_width(cr, 2.0);

		if (fabs(delta) > 2 * radius) {
			set_source_rgba(cr, _select_stroke);
			if (_orientation == orientation_horizontal) {
				cairo_move_to(cr, sx0 + dxy, sy0);
				cairo_line_to(cr, sx1 - dxy, sy1);
			}
			else {
				cairo_move_to(cr, sx0, sy0 + dxy);
				cairo_line_to(cr, sx1, sy1 - dxy);
			}
			cairo_stroke(cr);
		}

		// Markers
		set_source_rgba(cr, _select_fill);
		cairo_new_path(cr);
		cairo_arc(cr, sx0, sy0, radius, 0, 2 * M_PI);
		cairo_arc(cr, sx1, sy1, radius, 0, 2 * M_PI);
		cairo_fill(cr);

		set_source_rgba(cr, _select_stroke);
		cairo_new_path(cr);
		cairo_arc(cr, sx0, sy0, radius, 0, 2 * M_PI);
		cairo_stroke(cr);
		cairo_new_path(cr);
		cairo_arc(cr, sx1, sy1, radius, 0, 2 * M_PI);
		cairo_stroke(cr);
	}

	_backing_store_valid = true;

	cairo_destroy(cr);
	return true;
}
#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923   // pi/2
#endif
cairo_surface_t* Ruler::draw_label(cairo_t* cr_in, const std::string& label_value, const glm::ivec2& ts)
{
	bool rotate = _orientation != orientation_horizontal;
	//Glib::RefPtr<Pango::Layout> layout = create_pango_layout(std::to_string(label_value));
	auto surface_in = cairo_get_target(cr_in);
	auto ly = create_pango_layout(/*cr_in,*/ label_value.c_str(), _font_size);
	auto layout = &ly;
	int text_width = ts.x;
	int text_height = ts.y;
	layout->get_pixel_size(text_width, text_height);
	if (rotate) {
		std::swap(text_width, text_height);
	}
	auto surface = cairo_surface_create_similar_image(surface_in, CAIRO_FORMAT_ARGB32, text_width + 10, text_height + 10);
	cairo_t* cr = cairo_create(surface);

	cairo_save(cr);
	set_source_rgba(cr, _foreground);
	if (rotate) {
		cairo_translate(cr, text_width / 2, text_height / 2);
		cairo_rotate(cr, -M_PI_2);
		cairo_translate(cr, -text_height / 2, -text_width / 2);
	}
	ly.fgp = ly.layout;
	auto fx0 = *fx;
	fx0.color = 0;
	layout->draw(cr);
	cairo_restore(cr);

	cairo_surface_write_to_png(surface, "surface_label.png");
	cairo_destroy(cr);
	return surface;
}
void Ruler::draw_marker(cairo_t* cr)
{
	auto a = get_drawing_size();
	int awidth = a.x, aheight = a.y;
	set_source_rgba(cr, _cursor_color);
	if (_orientation == orientation_horizontal) {
		cairo_move_to(cr, _position, aheight);
		cairo_line_to(cr, _position - half_width, aheight - half_width);
		cairo_line_to(cr, _position + half_width, aheight - half_width);
		cairo_close_path(cr);
	}
	else {
		cairo_move_to(cr, awidth, _position);
		cairo_line_to(cr, awidth - half_width, _position - half_width);
		cairo_line_to(cr, awidth - half_width, _position + half_width);
		cairo_close_path(cr);
	}
	cairo_fill(cr);
}

glm::ivec4 Ruler::marker_rect()
{
	auto a = get_drawing_size();
	int awidth = a.x, aheight = a.y;
	glm::ivec4 rect;
	rect.x = 0;
	rect.y = 0;
	rect.z = 0;
	rect.w = 0;

	// Find size of rectangle to enclose triangle.
	if (_orientation == orientation_horizontal) {
		rect.x = std::floor(_position - half_width);
		rect.y = std::floor(aheight - half_width);
		rect.z = std::ceil(half_width * 2.0 + 1);
		rect.w = std::ceil(half_width);
	}
	else {
		rect.x = std::floor(awidth - half_width);
		rect.y = std::floor(_position - half_width);
		rect.z = std::ceil(half_width);
		rect.w = std::ceil(half_width * 2.0 + 1);
	}

	return rect;
}

bool Ruler::on_drawing_area_draw(cairo_t* cr, const glm::vec2& pos)
{
	if (!_backing_store_valid) {
		draw_scale(cr);

		cairo_surface_write_to_png(_backing_store, "ruler1904.png");
	}
	cairo_save(cr);

	cairo_translate(cr, pos.x, pos.y);
	auto a = get_drawing_size();
	int awidth = a.x, aheight = a.y;
	glm::vec2 c2 = { awidth, aheight };
	glm::vec2 c = {};
	if (_orientation == orientation_horizontal) {
		c.x = aheight;
	}
	else {
		c.y = awidth;
	}
	cairo_rectangle(cr, c.x, c.y, c2.x, c2.y);
	cairo_clip(cr);
	cairo_set_source_surface(cr, _backing_store, 0, 0);
	cairo_paint(cr);
	if (c.x == 0)c.x -= 1;
	if (c.y == 0)c.y -= 1;
	cairo_translate(cr, c.x, c.y);
	draw_marker(cr);
	cairo_restore(cr);
	return true;
}

void Ruler::on_style_updated()
{
	//Gtk::Box::on_style_updated();

	//Glib::RefPtr<Gtk::StyleContext> style_context = get_style_context();

	//// Cache all our colors to speed up rendering.

	//_foreground = get_foreground_color(style_context);
	//_font_size = get_font_size(*this);

	//_shadow = get_color_with_class(style_context, "shadow");
	//_page_fill = get_color_with_class(style_context, "page");

	//style_context->add_class("selection");
	//_select_fill = get_color_with_class(style_context, "background");
	//_select_stroke = get_color_with_class(style_context, "border");
	//style_context->remove_class("selection");

	_label_cache.clear();
	_backing_store_valid = false;

	//queue_resize();
	//_drawing_area->queue_draw();
}

void Ruler::on_prefs_changed()
{
	_sel_visible;
	_backing_store_valid = false;
}

void Ruler::on_motion(void* motion, double x, double y)
{
	int drawing_x = std::lround(x), drawing_y = std::lround(y);
	double const position = _orientation == orientation_horizontal ? drawing_x : drawing_y;
	if (position == _position) return;

	_position = position;
	auto new_rect = marker_rect();
	_rect = new_rect;
}

int Ruler::on_click_pressed(void* click, int n_press, double x, double y)
{
	return 0;
}

void Ruler::set_context_menu()
{

}


#endif // 1

#if 1


class svg_dp
{
public:
#ifndef NO_SVG
	RsvgHandle* handle = 0;
	RsvgDimensionData dimension = {};
#endif
public:
	svg_dp(int dpi = 0);
	~svg_dp();
	static svg_dp* new_from_file(const void* fn, size_t len);
	static svg_dp* new_from_data(const void* str, size_t len);
	glm::vec2 get_pos_id(const char* id);
	void get_bixbuf(const char* id);
	void draw(cairo_t* cr);
	void draw(cairo_t* cr, const glm::vec2& pos, const glm::vec2& scale, double angle, const char* id);
};

svg_dp::svg_dp(int dpi)
{
#ifndef NO_SVG
	rsvg_set_default_dpi(dpi < 72.0 ? 72.0 : dpi);
#endif
}

svg_dp::~svg_dp()
{
#ifndef NO_SVG
	// 释放资源  
	rsvg_handle_free(handle);
#endif
}
svg_dp* svg_dp::new_from_file(const void* fn, size_t len)
{
	svg_dp* p = 0;
#ifndef NO_SVG
	GError* er = NULL;
	auto handle = rsvg_handle_new_from_file((gchar*)fn, &er);
	if (er != NULL) {
		printf("Failed to load SVG file: %s\n", er->message);
		return 0;
	}
	p = new svg_dp();
	/* 获取SVG图像的尺寸 */
	rsvg_handle_get_dimensions(handle, &p->dimension);
	p->handle = handle;
#endif
	return p;
}
svg_dp* svg_dp::new_from_data(const void* str, size_t len)
{
	GError* er = NULL;
	svg_dp* p = 0;
#ifndef NO_SVG
	auto handle = rsvg_handle_new_from_data((guint8*)str, len, &er);
	if (er != NULL) {
		printf("Failed to load SVG file: %s\n", er->message);
		return 0;
	}
	p = new svg_dp();
	/* 获取SVG图像的尺寸 */
	rsvg_handle_get_dimensions(handle, &p->dimension);
	p->handle = handle;
#endif
	return p;
}
glm::vec2 svg_dp::get_pos_id(const char* id)
{
#ifndef NO_SVG
	RsvgPositionData pos = {};
	auto r = rsvg_handle_get_position_sub(handle, &pos, id);
	return glm::vec2(pos.x, pos.y);
#else
	return {};
#endif
}

void svg_dp::get_bixbuf(const char* id)
{
#ifndef NO_SVG
	auto pxb = id && *id ? rsvg_handle_get_pixbuf(handle) : rsvg_handle_get_pixbuf_sub(handle, id);
	uint32_t length = 0;
	auto px = gdk_pixbuf_get_pixels_with_length(pxb, &length);
	auto w = gdk_pixbuf_get_width(pxb);
	auto h = gdk_pixbuf_get_height(pxb);
	auto st = gdk_pixbuf_get_rowstride(pxb);
#endif
	//g_object_unref(pxb);
	return;
}


void svg_dp::draw(cairo_t* cr)
{
#ifndef NO_SVG
	rsvg_handle_render_cairo(handle, cr);//渲染到窗口，有刷新事件需要重新执行，这里只渲染一次 
#endif
}
void svg_dp::draw(cairo_t* cr, const glm::vec2& pos, const glm::vec2& scale, double angle, const char* id)
{
#ifndef NO_SVG
	cairo_save(cr);
	if (scale.x > 0 && scale.y > 0)
		cairo_scale(cr, scale.x, scale.y);
	if (angle > 0)
		cairo_rotate(cr, angle);
	cairo_translate(cr, pos.x, pos.y);
	if (id && *id)
	{
		RsvgDimensionData dim = {};
		RsvgPositionData poss = {};
		rsvg_handle_get_dimensions_sub(handle, &dim, id);
		if (!rsvg_handle_get_position_sub(handle, &poss, id)) {
			return;
		}
		/* Move the whole thing to 0, 0 so the object to export is at the origin */
		cairo_translate(cr, -poss.x, -poss.y);
		rsvg_handle_render_cairo_sub(handle, cr, id);
	}
	else
	{
		rsvg_handle_render_cairo(handle, cr);//渲染到窗口，有刷新事件需要重新执行，这里只渲染一次
	}
	//cairo_surface_flush(surface);
	cairo_restore(cr);
#endif
}

svg_cx* new_svg_file(const void* fn, size_t len, int dpi) {
	return (svg_cx*)svg_dp::new_from_file(fn, len);
}
svg_cx* new_svg_data(const void* str, size_t len, int dpi) {
	return (svg_cx*)svg_dp::new_from_data(str, len);
}
void free_svg(svg_cx* svg)
{
	if (svg)
	{
		auto t = (svg_dp*)svg;
		delete t;
	}
}
void render_svg(cairo_t* cr, svg_cx* svg)
{
	if (cr && svg)
	{
		auto t = (svg_dp*)svg;
		t->draw(cr);
	}
}
void render_svg(cairo_t* cr, svg_cx* svg, const glm::vec2& pos, const glm::vec2& scale, double angle, const char* id)
{
	if (cr && svg)
	{
		auto t = (svg_dp*)svg;
		t->draw(cr, pos, scale, angle, id);
	}
}
#endif // 1






view_g::view_g()
{
	fx.family = (char*)u8"Calibri";
	fx.font_size = fh;
	ruler = new Ruler[2];
	ruler[0].fx = &fx;
	ruler[1].fx = &fx;
	ruler[1]._orientation = 1;
	vcanvas = new tinyviewcanvas_x();
}

view_g::~view_g()
{
	if (_backing_store)
	{
		cairo_surface_destroy(_backing_store);
	}
	_backing_store = 0;
	if (rss)
	{
		cairo_surface_destroy(rss);
	}
	rss = 0;
	if (vcanvas) { delete vcanvas; vcanvas = 0; }
	if (ruler) { delete[]ruler; ruler = 0; }
}

view_g* view_g::new_view(const glm::vec2& size)
{
	view_g* p = 0;
	if (size.x > 1 && size.y > 1)
	{
		p = new view_g();
		p->set_view(size);
	}
	return p;
}
void view_g::free_view(view_g* p)
{
	if (p)delete p;
}
void view_g::set_view(const glm::ivec2& size)
{
	if (!(size.x > 1 && size.y > 1)) {
		assert(0);
		return;
	}
	auto _hruler = ruler;
	auto _vruler = _hruler + 1;
	auto ss = size;
	vsize = size;
	ss.y = rsize.y;
	_hruler->drawingsize = ss;
	ss = size;
	ss.x = rsize.x;
	_vruler->drawingsize = ss;
	vcanvas->set_size(size);
	if (cdv && cdv->dsize != size)
	{
		canvas_dev::free_cdev(cdv);
	}
	cdv = canvas_dev::new_cdev(size, 0);
	_backing_store_valid = false;
}

void view_g::on_motion(const glm::vec2& pos) {
	glm::ivec2 ps = pos - rsize;
	if (curpos != ps)
	{
		curpos = ps;

		auto _hruler = ruler;
		auto _vruler = _hruler + 1;
		vcanvas->on_motion(pos);
		_hruler->on_motion(0, ps.x, ps.y);
		_vruler->on_motion(0, ps.x, ps.y);
		update();
	}
}
void view_g::on_button(int idx, int down, const glm::vec2& pos, int clicks) {
	auto ps = pos - rsize;
	if (down == 0 && is_ck)
	{
		is_ck = false;
		glm::vec4 trc = { 0,0,rsize };
		auto k2 = check_box_cr(pos, &trc, 1);
		if (k2.x)
		{
			vcanvas->has_move = !vcanvas->has_move;
			vcanvas->has_scale = vcanvas->has_move;
		}
	}
	if (down == 1)
	{
		is_ck = true;
	}
	vcanvas->on_button(idx, down, pos, clicks);
	update();
	_draw_valid = true;
}
void view_g::on_wheel(double x, double y)
{
	vcanvas->on_wheel(y);
	update();
	_draw_valid = true;
}
void view_g::updateRulers()
{
	glm::vec4 viewbox = { 0,0, vsize };// _canvas->get_area_world();
	glm::vec4 startbox = viewbox;
	auto d2c_scalerot = vcanvas->get_affine();
	auto _hruler = ruler;
	auto _vruler = _hruler + 1;
	auto zoom1 = d2c_scalerot[0].x;
	glm::vec2 ps = vcanvas->vpos;
	auto xxm = glm::inverse(d2c_scalerot);		// 逆矩阵
	auto rulerbox = get_boxm3(startbox, xxm);
	_hruler->set_range(rulerbox.x, rulerbox.z);	//设置水平标尺读数范围
	_vruler->is_yaxisdown = is_yaxisdown;
	if (is_yaxisdown)
	{
		_vruler->set_range(rulerbox.y, rulerbox.w);	//设置垂直标尺范围
	}
	else {
		_vruler->set_range(-rulerbox.y, -rulerbox.w);
	}

	pagebox = { 0,0, wsize };
	pagebox = get_boxm3(pagebox, d2c_scalerot);
	_hruler->set_page(pagebox.x, pagebox.z);//水平
	_vruler->set_page(pagebox.y, pagebox.w);

	selbox = {};
	// todo
	selbox = get_boxm3(selbox, d2c_scalerot);
	if (selbox.z > 0 && selbox.w > 0) {
		_hruler->set_selection(selbox.x, selbox.z);//水平
		_vruler->set_selection(selbox.y, selbox.w);
	}
}
void view_g::mkpath()
{
	auto vss = vsize;
	glm::vec2 sw = step_width;
	sw *= step_width.z;
	sw *= step_width.z; sw *= 2;
	vss += sw;
	int rct = make_grid(vss, step_width, { 0.5,0.5 }, pvdata);
	draw_grid_back(vss);
}


void view_g::draw_grid_back(const glm::ivec2& vss)
{
	print_time a("draw_grid_back\n");
	if (!_backing_store)
		_backing_store = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, vss.x, vss.y);
	if (!_backing_store)return;
	// Get context
	cairo_t* cr = cairo_create(_backing_store);
	{
		style_path_t st0 = {};
		style_path_t st1 = {};
		st1.stroke_color = wcolor.z;
		st1.line_width = 1;
		st0.fill_color = color.x;
		//st0.box_color = color.y;// 0x80FF8050;
		st0.stroke_color = color.z;// 0x80353535;
		st0.line_width = 1.0f;
		st0.cap = 0;
		st0.join = 1;
		auto stbc = st0;

		stbc.stroke_color = color.w;// 0x80606060;

		style_path_t* spt = &st0, * spt1 = &stbc;

		if (spt && pvdata[0].size() > 0) {
			draw_path_v(cr, &pvdata[0], spt);
		}
		if (spt1 && pvdata[1].size() > 1) {
			draw_path_v(cr, &pvdata[1], spt1);
		}

	}
	_backing_store_valid = true;
	cairo_destroy(cr);
}

glm::mat3 view_g::get_mat()
{
	return vcanvas->get_affine();
}

glm::mat3x2 view_g::get_mat3x2()
{
	auto m = vcanvas->get_affine();
	return glm::mat3x2(m);
}
glm::dmat3x2 view_g::get_dmat3x2()
{
	auto m = vcanvas->get_affine();
	return glm::dmat3x2(m);
}

void view_g::on_event(uint32_t type, et_un_t* ep)
{
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	switch (t)
	{
	case devent_type_e::mouse_move_e:
	{
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y }; mps -= vgpos;
		on_motion(mps);
	}
	break;
	case devent_type_e::mouse_button_e:
	{
		auto p = e->b;
		glm::ivec2 mps = { p->x,p->y }; mps -= vgpos;
		on_button(p->button, p->down, mps, p->clicks);
	}
	break;
	case devent_type_e::mouse_wheel_e:
	{
		auto p = e->w;
		on_wheel(p->x, p->y);
	}
	break;
	case devent_type_e::keyboard_e:
	{
		//on_keyboard(ep);
	}
	break;
	}
}

void view_g::begin_draw() {
	if (!cdv || !_draw_valid)return;
	auto cr = cdv->cr;
	cdv->begin(clear_color);
	cairo_save(cr);
	//cairo_translate(cr, vpos.x, vpos.y);
	auto ae = vcanvas->get_affine();//视图矩阵
	glm::vec2 aps = ae[2];
	glm::vec2 sw = step_width;	// 单个网格宽、高
	sw *= step_width.z;			// 数量
	aps = glm::mod(aps, sw) - sw;
	cairo_set_source_surface(cr, _backing_store, aps.x, aps.y);
	cairo_paint(cr);
	cairo_restore(cr);
	cairo_save(cr);
	auto tr = glm::transpose(ae);
	auto ainv = glm::affineInverse(ae);
	auto inv = glm::inverse(ae);
	auto invtr = glm::inverseTranspose(ae);
	//set_mat3(cr, ae);
	set_color(cr, cross_color.x);
	cairo_set_line_width(cr, cross_width);
	glm::vec2 ds = { vsize };
	auto _hruler = ruler;
	auto _vruler = _hruler + 1;
	ds.x = _hruler->_page_lower;
	ds.y = _vruler->_page_lower;

	if (cross_width == 1)
		ds += 0.5;
	// 十字线
	cairo_move_to(cr, 0, ds.y);
	cairo_line_to(cr, vsize.x, ds.y);
	cairo_stroke(cr);
	set_color(cr, cross_color.y);
	cairo_move_to(cr, ds.x, 0);
	cairo_line_to(cr, ds.x, vsize.y);
	cairo_stroke(cr);
	cairo_restore(cr);

}
void view_g::end_draw() {
	if (!cdv || !_draw_valid)return;
	auto cr = cdv->cr;
	auto _hruler = ruler;
	auto _vruler = _hruler + 1;
	cairo_save(cr);
	// 左上角背景
	cairo_set_line_width(cr, 1);
	set_color(cr, ruler ? ruler->_back_color : color.z);
	cairo_rectangle(cr, 0, 0, rsize.x, rsize.y);
	cairo_fill(cr);
	//set_source_rgba(cr, ruler->_foreground);
	int gradient_size = 4;
	paint_shadow(cr, 0, gradient_size, rsize.x, gradient_size, _vruler->_shadow, 0);// 垂直方向  
	paint_shadow(cr, gradient_size, 0, gradient_size, rsize.y, _vruler->_shadow, 0);// 水平方向 
	cairo_restore(cr);

	cairo_save(cr);
	// 标尺
	_hruler->on_drawing_area_draw(cr, {});
	_vruler->on_drawing_area_draw(cr, {});
	cairo_restore(cr);
	//裁剪
	clip_rect(cr, rss);
	cdv->end();
}
void set_mat3(cairo_t* cr, const glm::mat3& m) {
	auto mm = (glm::dmat3x2)m;
	cairo_set_matrix(cr, (cairo_matrix_t*)&mm);
}
void view_g::draw()
{
	if (!cdv || !_draw_valid)return;
	auto cr = cdv->cr;
	cairo_save(cr);
	vcanvas->draw(cdv);
	cairo_restore(cr);
}
void view_g::update(float)
{
	if (!_backing_store_valid)
	{
		mkpath();
		_draw_valid = true;
	}
	vcanvas->vpos = vpos;
	updateRulers();
	auto _hruler = ruler;
	auto _vruler = _hruler + 1;
	if (!vcanvas->_backing_store_valid || !_hruler->_backing_store_valid || !_vruler->_backing_store_valid)
		_draw_valid = true;

}
#endif // !no_cairo_


// font
// todo font
#if 1



char const* sp_font_description_get_family(PangoFontDescription const* fontDescr)
{
	static auto const fontNameMap = std::map<std::string, std::string>{
		{ "Sans", "sans-serif" },
		{ "Serif", "serif" },
		{ "Monospace", "monospace" }
	};

	char const* pangoFamily = pango_font_description_get_family(fontDescr);

	if (pangoFamily) {
		if (auto it = fontNameMap.find(pangoFamily); it != fontNameMap.end()) {
			return it->second.c_str();
		}
	}

	return pangoFamily;
}




#ifndef NO_FONT_CX 
/*
* 获取字体信息名
1 family名
2 样式名
3 唯一标识符
4 全名
6 postscript名
*/
std::vector<std::string> get_name_idx(hb_face_t* face, int idx, const std::string& fn)
{
	uint32_t n = 0;
	auto ns = hb_ot_name_list_names(face, &n);
	std::string name;
	std::vector<std::string> r;
	for (size_t i = 0; i < n; i++)
	{
		auto it = ns[i];
		if (it.name_id != idx)
		{
			continue;
		}
		uint32_t ss = 0;
		auto kn8 = hb_ot_name_get_utf8(face, it.name_id, it.language, &ss, 0);
		name.resize(kn8 + 1); ss = kn8 + 1;
		auto kn = hb_ot_name_get_utf8(face, it.name_id, it.language, &ss, (char*)name.c_str());
		//printf("%d\t%s\n", it.name_id, name.c_str());
		name = name.c_str();
		if (name.size() && name != fn)
			r.push_back(name.c_str());
	}
	return r;
}
std::string get_pat_str(FcPattern* font, const char* o)
{
	FcChar8* s = nullptr;
	std::string r;
	if (o && ::FcPatternGetString(font, o, 0, &s) == FcResultMatch)
	{
		if (s)
		{
			r = (char*)s;
		}
	}
	return r;
}
int get_pat_int(FcPattern* font, const char* o)
{
	int s = 0;
	if (o && ::FcPatternGetInteger(font, o, 0, &s) == FcResultMatch)
	{
	}
	return s;
}

#ifdef _WIN32111
#define get_fmap pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT)
#else
#define get_fmap pango_cairo_font_map_get_default()
#endif // _WIN32
//pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);

std::map<std::string, fontns> get_allfont()
{
	int r = 0;
	std::map<std::string, fontns> fyv;
	PangoFontFamily** families = 0;
	int nfamilies = 0;
	PangoFontMap* fontmap = get_fmap;
	pango_font_map_list_families(fontmap, &families, &nfamilies);

	std::set<std::string> nns;
	auto context = pango_font_map_create_context(fontmap);
	for (int i = 0; i < nfamilies; i++) {
		PangoFontFamily* family = families[i];
		const char* family_name = pango_font_family_get_name(family);
		auto descr = pango_font_description_new();
		pango_font_description_set_family(descr, family_name);// (char*)u8"仿宋");
		auto font = pango_font_map_load_font(fontmap, context, descr);

		auto lang = pango_font_get_languages(font);//2052
		auto hbfont = pango_font_get_hb_font(font);
		hb_face_t* face = hb_font_get_face(hbfont);
		uint32_t ss = 8;
		wchar_t buf[8] = {};
		char buf8[32] = {};
		std::wstring namew;
		auto name = get_name_idx(face, 1, family_name);
		//auto st2 = get_name_idx(face, 2, family_name);
		auto name4 = get_name_idx(face, 4, family_name);
		nns.clear();
		for (auto it : name)
		{
			nns.insert(it);
		}
		for (auto it : name4)
		{
			nns.insert(it);
		}
		//if (name.size())
		//	printf("%s\n", name.size() > 1 ? name[1].c_str() : name[0].c_str());
		pango_font_description_free(descr);
		auto& kt = fyv[family_name];
		kt.alias.swap(nns);
		if (name4.size())
			kt.fullname = name4[0];
	}

	if (context)
	{
		g_object_unref(context); context = 0;
	}
	if (FcInit()) {
		{
			//std::string yourFontFilePath = "seguiemj.ttf";
			std::string yourFontFilePath = "C:\\Windows\\Fonts\\seguiemj.ttf";
			const FcChar8* file = (const FcChar8*)yourFontFilePath.c_str();
			FcBool fontAddStatus = FcConfigAppFontAddFile(FcConfigGetCurrent(), file);
		}
		//{
		//	std::string yourFontFilePath = "Noto-COLRv1-emojicompat.ttf";
		//	const FcChar8* file = (const FcChar8*)yourFontFilePath.c_str();
		//	FcBool fontAddStatus = FcConfigAppFontAddFile(FcConfigGetCurrent(), file);
		//}
		FcPattern* pat = ::FcPatternCreate();
		FcObjectSet* os = ::FcObjectSetBuild(FC_FILE, FC_FULLNAME, FC_FAMILY, FC_STYLE, FC_CHARSET, FC_WIDTH, (char*)0);
		FcFontSet* fs = ::FcFontList(0, pat, os);
		for (size_t i = 0; i < fs->nfont; ++i) {
			FcPattern* font = fs->fonts[i];
			FcChar8* family = nullptr;
			FcChar8* family0 = nullptr;
			FcChar8* fp = nullptr;
			FcChar8* style = nullptr;
			auto w = get_pat_int(font, FC_WIDTH);
			auto ct = get_pat_str(font, FC_CHARSET);
			auto fullname = get_pat_str(font, FC_FULLNAME);
			auto fph = get_pat_str(font, FC_FILE);
			if (fph.size())
			{
				auto ttf = fph.rfind(".ttf");
				auto ttc = fph.rfind(".ttc");
				auto otf = fph.rfind(".otf");
				if (ttf != std::string::npos || ttc != std::string::npos || otf != std::string::npos) {

					auto family = get_pat_str(font, FC_FAMILY);
					auto style = get_pat_str(font, FC_STYLE);
					if (family.size() && style.size())
					{
						auto& kt = fyv[family];
						kt.fpath.push_back(fph);
						kt.family = family;
						if (kt.fullname.empty())
							kt.fullname = fullname;
						kt.style.push_back(style);
					}
				}
			}
		}

		FcObjectSetDestroy(os);
		FcPatternDestroy(pat);
		FcFontSetDestroy(fs);

		FcFini();
	}
	// 删除空字体
	auto newfn = fyv;
	for (auto& [k, v] : fyv) {
		if (v.family.empty() || v.fpath.empty())
		{
			newfn.erase(k); r++;
		}
	}
	return newfn;
}
// 枚举字体名称/路径名
//std::map<std::string, fontns> get_all_font()
//{
//	static std::map<std::string, fontns> fyv = get_allfont();
//	return fyv;
//}
struct font_impl;
class path_v;
class info_one;
class fd_info0;


class font_imp
{
public:
	// add font string
	std::string addstr = {};
	struct mem_ft
	{
		char* data;
		std::set<std::string> vname;
	};
private:
	std::vector<font_t*> fonts;		// 字体
	std::vector<fd_info0*> fd_data;	// 字体数据
	std::vector<mem_ft> fd_data_m;	// 字体数据copy
	std::map<std::string, font_t*>	mk;	// 字体名搜索
public:
	font_imp();
	~font_imp();
	std::vector<font_t*> add_font_file(const std::string& fn, std::vector<std::string>* pname);
	std::vector<font_t*> add_font_mem(const char* data, size_t len, bool iscp, std::vector<std::string>* pname, int* rc);

	int get_font_names(std::vector<std::string>* pname);
	const char* get_font_names(const char* sp);
	font_t* get_font(size_t idx);
	size_t size();
	void free_ft(const std::string& name);
	void free_ftp(font_t* p);
private:
};


#if 1
class UTF16 {
public:
	static int toUCS4(const unsigned short* utf16, unsigned short* ucs4);
	static int toUTF8(const unsigned short* utf16, unsigned char* utf8);
	static int toUTF8(const unsigned short* utf16, int n, unsigned char* utf8);
};

using namespace std;

int UTF16::toUCS4(const unsigned short* utf16, unsigned short* ucs4)
{
	if (utf16[0] >= 0xd800 && utf16[0] <= 0xdfff)
	{
		if (utf16[0] < 0xdc00)
		{
			if (utf16[1] >= 0xdc00 && utf16[1] <= 0xdfff)
			{
				ucs4[1] = (utf16[0] & 0x3ff);
				ucs4[0] = (utf16[1] & 0x3ff);
				ucs4[0] = ((ucs4[1] << 10) | ucs4[0]);
				ucs4[1] = ((ucs4[1] >> 6) | 1);

				//printf("%04x\n", ucs4[0]);
				//printf("%04x\n", ucs4[1]);

				return 2;
			}

			return -1;
		}

		return -1;
	}
	else
	{
		ucs4[0] = utf16[0];
		ucs4[1] = 0x00;
	}

	return 1;
}

int UTF16::toUTF8(const unsigned short* utf16, unsigned char* utf8)
{
	unsigned short ucs4[2];
	uint32_t* u = (uint32_t*)ucs4;
	int w;

	if (utf16[0] >= 0xd800 && utf16[0] <= 0xdfff)
	{
		if (utf16[0] < 0xdc00)
		{
			if (utf16[1] >= 0xdc00 && utf16[1] <= 0xdfff)
			{
				ucs4[1] = (utf16[0] & 0x3ff);
				ucs4[0] = (utf16[1] & 0x3ff);
				ucs4[0] = ((ucs4[1] << 10) | ucs4[0]);
				ucs4[1] = ((ucs4[1] >> 6) | 1);
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}
	else
	{
		ucs4[0] = utf16[0];
		ucs4[1] = 0x00;
	}

	w = *u;

	if (w <= 0x0000007f)
	{
		/*U-00000000 - U-0000007F:  0xxxxxxx*/
		utf8[0] = (w & 0x7f);

		return 1;
	}
	else if (w >= 0x00000080 && w <= 0x000007ff)
	{
		/*U-00000080 - U-000007FF:  110xxxxx 10xxxxxx*/
		utf8[1] = (w & 0x3f) | 0x80;
		utf8[0] = ((w >> 6) & 0x1f) | 0xc0;

		return 2;
	}
	else if (w >= 0x00000800 && w <= 0x0000ffff)
	{
		/*U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx*/
		utf8[2] = (w & 0x3f) | 0x80;
		utf8[1] = ((w >> 6) & 0x3f) | 0x80;
		utf8[0] = ((w >> 12) & 0x0f) | 0xe0;

		return 3;
	}
	else if (w >= 0x00010000 && w <= 0x001fffff)
	{
		/*U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx*/
		utf8[3] = (w & 0x3f) | 0x80;
		utf8[2] = ((w >> 6) & 0x3f) | 0x80;
		utf8[1] = ((w >> 12) & 0x3f) | 0x80;
		utf8[0] = ((w >> 18) & 0x07) | 0xf0;

		return 4;
	}
	else if (w >= 0x00200000 && w <= 0x03ffffff)
	{
		/*U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx*/
		utf8[4] = (w & 0x3f) | 0x80;
		utf8[3] = ((w >> 6) & 0x3f) | 0x80;
		utf8[2] = ((w >> 12) & 0x3f) | 0x80;
		utf8[1] = ((w >> 18) & 0x3f) | 0x80;
		utf8[0] = ((w >> 24) & 0x03) | 0xf8;

		return 5;
	}
	else if (w >= 0x04000000 && w <= 0x7fffffff)
	{
		/*U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx*/
		utf8[5] = (w & 0x3f) | 0x80;
		utf8[4] = ((w >> 6) & 0x3f) | 0x80;
		utf8[3] = ((w >> 12) & 0x3f) | 0x80;
		utf8[2] = ((w >> 18) & 0x3f) | 0x80;
		utf8[1] = ((w >> 24) & 0x03) | 0xf8;
		utf8[0] = ((w >> 30) & 0x01) | 0xfc;

		return 6;
	}

	return 0;
}

int UTF16::toUTF8(const unsigned short* utf16, int n, unsigned char* utf8)
{
	unsigned short ucs4[2];
	uint32_t* u = (uint32_t*)ucs4;
	int w;
	int m = 0;
	int e = 0;
	int i = 0;
	int j = 0;

	for (i = 0; i < n; i += m)
	{
		if (utf16[i] >= 0xd800 && utf16[i] <= 0xdfff)
		{
			if (utf16[i] < 0xdc00)
			{
				if (utf16[i + 1] >= 0xdc00 && utf16[i + 1] <= 0xdfff)
				{
					ucs4[1] = (utf16[i + 0] & 0x3ff);
					ucs4[0] = (utf16[i + 1] & 0x3ff);
					ucs4[0] = ((ucs4[1] << 10) | ucs4[0]);
					ucs4[1] = ((ucs4[1] >> 6) | 1);

					m = 2;
				}
				else
				{
					m = -1;
				}
			}
			else
			{
				m = -1;
			}
		}
		else
		{
			ucs4[0] = utf16[i];
			ucs4[1] = 0x00;

			m = 1;
		}

		if (m == -1)
		{
			utf8[j] = 0x00;

			return j;
		}

		w = *u;

		e = 0;

		if (w <= 0x0000007f)
		{
			/*U-00000000 - U-0000007F:  0xxxxxxx*/
			utf8[j + 0] = (w & 0x7f);

			e = 1;
		}
		else if (w >= 0x00000080 && w <= 0x000007ff)
		{
			/*U-00000080 - U-000007FF:  110xxxxx 10xxxxxx*/
			utf8[j + 1] = (w & 0x3f) | 0x80;
			utf8[j + 0] = ((w >> 6) & 0x1f) | 0xc0;

			e = 2;
		}
		else if (w >= 0x00000800 && w <= 0x0000ffff)
		{
			/*U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx*/
			utf8[j + 2] = (w & 0x3f) | 0x80;
			utf8[j + 1] = ((w >> 6) & 0x3f) | 0x80;
			utf8[j + 0] = ((w >> 12) & 0x0f) | 0xe0;

			e = 3;
		}
		else if (w >= 0x00010000 && w <= 0x001fffff)
		{
			/*U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx*/
			utf8[j + 3] = (w & 0x3f) | 0x80;
			utf8[j + 2] = ((w >> 6) & 0x3f) | 0x80;
			utf8[j + 1] = ((w >> 12) & 0x3f) | 0x80;
			utf8[j + 0] = ((w >> 18) & 0x07) | 0xf0;

			e = 4;
		}
		else if (w >= 0x00200000 && w <= 0x03ffffff)
		{
			/*U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx*/
			utf8[j + 4] = (w & 0x3f) | 0x80;
			utf8[j + 3] = ((w >> 6) & 0x3f) | 0x80;
			utf8[j + 2] = ((w >> 12) & 0x3f) | 0x80;
			utf8[j + 1] = ((w >> 18) & 0x3f) | 0x80;
			utf8[j + 0] = ((w >> 24) & 0x03) | 0xf8;

			e = 5;
		}
		else if (w >= 0x04000000 && w <= 0x7fffffff)
		{
			/*U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx*/
			utf8[j + 5] = (w & 0x3f) | 0x80;
			utf8[j + 4] = ((w >> 6) & 0x3f) | 0x80;
			utf8[j + 3] = ((w >> 12) & 0x3f) | 0x80;
			utf8[j + 2] = ((w >> 18) & 0x3f) | 0x80;
			utf8[j + 1] = ((w >> 24) & 0x03) | 0xf8;
			utf8[j + 0] = ((w >> 30) & 0x01) | 0xfc;

			e = 6;
		}

		j += e;
	}

	utf8[j] = 0x00;

	return j;
}


info_one::info_one(int platform, int encoding, int language, int nameid, const char* name, int len)
{
	platform_id = platform;
	encoding_id = encoding;
	language_id = language;
	name_id = nameid;
	length_ = len;
	std::wstring w;
	size_t wlen = length_ / 2;
	char* temp = (char*)name;
	if (*temp)
	{
		name_a.assign(temp, length_);
	}
	ndata.assign(temp, length_);
	auto ss = length_ / 2;

	name_.assign((wchar_t*)name, length_ / 2);
}
char* att_ushort(char* p)
{
	std::swap(p[0], p[1]);
	return p + 2;
}

std::string info_one::get_name()
{
	std::string n, gbkstr, u8str, wstr;
	if (encoding_id)
	{
		auto sws = ndata.size() * 0.5;
		std::string nd(ndata.c_str(), ndata.size());
		char* temp = (char*)nd.data();
		for (int i = 0; i < sws; ++i)
		{
			temp = att_ushort(temp);
		}
		u8str.resize(ndata.size());
		UTF16::toUTF8((uint16_t*)nd.c_str(), (int)sws, (uint8_t*)u8str.data());

		n = u8str.c_str();
		switch (platform_id)
		{
		case 0:
			// unicode
			break;
		case 1:
			// Macintosh
			break;
		case 2:
			// (reserved; do not use)
			break;
		case 3:
			// Microsoft
			if (encoding_id == 3 && language_id == 2052)
			{
				n = "";
				char* tw = (char*)ndata.data();
				tw++;
				int len = length_ / 2;
				for (size_t i = 0; i < len; i++, tw += 2)
				{
					auto it = *tw;
					n.push_back(it);
				}
				//n = gbk_u8(n);
			}
			break;
		default:
			break;
		}
	}
	else {
		n = name_a.c_str();
	}
	return n;
}
info_one::~info_one() {}


#if 1
#define FONS_NOTUSED(v)  (void)sizeof(v)
typedef uint8_t byte;
typedef int32_t Fixed;
#define FWord int16_t
#define uFWord uint16_t
#define Card8 uint8_t
#define Card16 uint16_t
#define Card32 uint32_t

#define head_VERSION VERSION(1, 0)

#define DATE_TIME_SIZE 8
typedef Card8 longDateTime[DATE_TIME_SIZE];

/* TrueType Collection Header */
typedef struct
{
	Card32 TTCTag;
	Fixed Version;
	Card32 DirectoryCount;
	Card32* TableDirectory; /* [DirectoryCount] */
	Card32 DSIGTag;
	Card32 DSIGLength;
	Card32 DSIGOffset;
} ttcfTbl;
typedef struct
{
	Fixed	version;//	0x00010000 if (version 1.0)
	Fixed	fontRevision;//	set by font manufacturer
	uint32_t	checkSumAdjustment;//	To compute : set it to 0, calculate the checksum for the 'head' table and put it in the table directory, sum the entire font as a uint32_t, then store 0xB1B0AFBA - sum. (The checksum for the 'head' table will be wrong as a result.That is OK; do not reset it.)
	uint32_t	magicNumber;//	set to 0x5F0F3CF5
	uint16_t	flags;/*	bit 0 - y value of 0 specifies baseline
	bit 1 - x position of left most black bit is LSB
	bit 2 - scaled point size and actual point size will differ(i.e. 24 point glyph differs from 12 point glyph scaled by factor of 2)
	bit 3 - use integer scaling instead of fractional
	bit 4 - (used by the Microsoft implementation of the TrueType scaler)
	bit 5 - This bit should be set in fonts that are intended to e laid out vertically, and in which the glyphs have been drawn such that an x - coordinate of 0 corresponds to the desired vertical baseline.
	bit 6 - This bit must be set to zero.
	bit 7 - This bit should be set if the font requires layout for correct linguistic rendering(e.g.Arabic fonts).
	bit 8 - This bit should be set for an AAT font which has one or more metamorphosis effects designated as happening by default.
	bit 9 - This bit should be set if the font contains any strong right - to - left glyphs.
	bit 10 - This bit should be set if the font contains Indic - style rearrangement effects.
	bits 11 - 13 - Defined by Adobe.
	bit 14 - This bit should be set if the glyphs in the font are simply generic symbols for code point ranges, such as for a last resort font.
	*/
	uint16_t	unitsPerEm;//	range from 64 to 16384
	longDateTime	created;//	international date
	longDateTime	modified;//	international date
	FWord	xMin;//	for all glyph bounding boxes
	FWord	yMin;//	for all glyph bounding boxes
	FWord	xMax;//	for all glyph bounding boxes
	FWord	yMax;//	for all glyph bounding boxes
	uint16_t	macStyle;/*	bit 0 bold
	bit 1 italic
	bit 2 underline
	bit 3 outline
	bit 4 shadow
	bit 5 condensed(narrow)
	bit 6 extended*/
	uint16_t	lowestRecPPEM;//	smallest readable size in pixels
	int16_t	fontDirectionHint;/*	0 Mixed directional glyphs
	1 Only strongly left to right glyphs
	2 Like 1 but also contains neutrals
	- 1 Only strongly right to left glyphs
	- 2 Like - 1 but also contains neutrals*/
	int16_t	indexToLocFormat;//	0 for short offsets, 1 for long
	int16_t	glyphDataFormat;//	0 for current format

}head_table;
// Entry
typedef struct sfnt_header_
{
	uint32_t tag = 0;
	uint32_t checksum;
	uint32_t offset; //From beginning of header.
	uint32_t logicalLength;
}sfnt_header;
typedef struct
{
	Fixed version;
	Card16 numTables;
	Card16 searchRange;
	Card16 entrySelector;
	Card16 rangeShift;
	sfnt_header* directory; /* [numTables] */
} sfntTbl;
typedef struct post_header_
{
	int32_t	format;//	Format of this table
	int32_t	italicAngle;	//Italic angle in degrees
	int16_t	underlinePosition;	//Underline position
	int16_t	underlineThickness;	//Underline thickness
	uint32_t	isFixedPitch;	//Font is monospaced; set to 1 if the font is monospaced and 0 otherwise(N.B., to maintain compatibility with older versions of the TrueType spec, accept any non - zero value as meaning that the font is monospaced)
	uint32_t	minMemType42;	//Minimum memory usage when a TrueType font is downloaded as a Type 42 font
	uint32_t	maxMemType42;	//Maximum memory usage when a TrueType font is downloaded as a Type 42 font
	uint32_t	minMemType1;	//Minimum memory usage when a TrueType font is downloaded as a Type 1 font
	uint32_t	maxMemType1;
}post_header;
typedef struct
{
	Fixed version;
	FWord ascender;
	FWord descender;
	FWord lineGap;
	uFWord advanceWidthMax;
	FWord minLeftSideBearing;
	FWord minRightSideBearing;
	FWord xMaxExtent;
	int16_t caretSlopeRise;
	int16_t caretSlopeRun;
	int16_t caretOffset;
	int16_t reserved[4];
	int16_t metricDataFormat;
	uint16_t numberOfLongHorMetrics;
} hheaTbl;


typedef struct {
	uint16_t format;
	uint16_t length;
	uint16_t language;
	unsigned char glyphId[256];
} Format0;
#define FORMAT0_SIZE (uint16 * 3 + uint8 * 256)
/*
Format 2: High - byte mapping through table
*/
typedef struct {
	unsigned short firstCode;
	unsigned short entryCount;
	short idDelta;
	unsigned short idRangeOffset;
} Segment2;
#define SEGMENT2_SIZE (uint16 * 3 + int16 * 1)

typedef struct {
	unsigned short format;
	unsigned short length;
	unsigned short language;
	unsigned short segmentKeys[256];
	Segment2* segment;
	unsigned short* glyphId;
} Format2;
// ------------------------------------------------------------------------------------------------
struct Bitmap_p
{
	uint32_t    rows;
	uint32_t    width;
	int             pitch;
	float			advance;
	float			bearingX;
	float			bearingY;
	int             bit_depth;
	unsigned char* buffer;
	uint32_t	capacity = 0;
	unsigned short  num_grays;
	unsigned char   pixel_mode;
	unsigned char   lcd_mode;
	void* data = 0;
	unsigned char   palette_mode;
	void* palette;
	int x, y;
};

union ft_key_s
{
	uint64_t u = 0;
	struct
	{
		char32_t unicode_codepoint;
		unsigned short font_dpi;
		// 字号支持 1-255
		unsigned char font_size;
		// 模糊大小支持 0-255
		unsigned char blur_size;
		//unsigned char is_bitmap;
	}v;
};
union ft_char_s
{
	uint64_t u = 0;
	struct
	{
		char32_t unicode_codepoint;
		unsigned short font_dpi;
		// 字号支持 1-255
		unsigned char font_size;
	}v;
};

typedef enum  _Pixel_Mode_
{
	PX_NONE = 0,
	PX_MONO,
	PX_GRAY,
	PX_GRAY2,
	PX_GRAY4,
	PX_LCD,
	PX_LCD_V,
	PX_BGRA,

	PX_MAX      /* do not remove */

} Pixel_Mode;

typedef enum STT_
{
	TYPE_NONE = 0,
	TYPE_EBLC, /* `EBLC' (Microsoft), */
	/* `bloc' (Apple)      */
	TYPE_CBLC, /* `CBLC' (Google)     */
	TYPE_SBIX, /* `sbix' (Apple)      */
	/* do not remove */
	TYPE_MAX
} sbit_table_type;

#define DL_long(v) v=ttLONG(data);data+=4
#define DL_short(v) v=ttSHORT(data);data+=2
#define DL_ulong(v) v=ttULONG(data);data+=4
#define DL_ushort(v) v=ttUSHORT(data);data+=2

#define FONS_INVALID -1

enum FONSflags {
	FONS_ZERO_TOPLEFT = 1,
	FONS_ZERO_BOTTOMLEFT = 2,
};

enum FONSalign {
	// Horizontal align
	FONS_ALIGN_LEFT = 1 << 0,	// Default
	FONS_ALIGN_CENTER = 1 << 1,
	FONS_ALIGN_RIGHT = 1 << 2,
	// Vertical align
	FONS_ALIGN_TOP = 1 << 3,
	FONS_ALIGN_MIDDLE = 1 << 4,
	FONS_ALIGN_BOTTOM = 1 << 5,
	FONS_ALIGN_BASELINE = 1 << 6, // Default
};

enum FONSglyphBitmap {
	FONS_GLYPH_BITMAP_OPTIONAL = 1,
	FONS_GLYPH_BITMAP_REQUIRED = 2,
};

enum FONSerrorCode {
	// Font atlas is full.
	FONS_ATLAS_FULL = 1,
	// Scratch memory used to render glyphs is full, requested size reported in 'val', you may need to bump up FONS_SCRATCH_BUF_SIZE.
	FONS_SCRATCH_FULL = 2,
	// Calls to fonsPushState has created too large stack, if you need deep state stack bump up FONS_MAX_STATES.
	FONS_STATES_OVERFLOW = 3,
	// Trying to pop too many states fonsPopState().
	FONS_STATES_UNDERFLOW = 4,
};

#endif

struct meta_tag
{
	std::string tag;
	std::string v;
};


struct font_impl :public stbtt_fontinfo
{

	int colr = -1, cpal = -1; // table locations as offset from start of .ttf

	std::map<std::string, sfnt_header> _tb;
	// EBLC	Embedded bitmap location data	嵌入式位图位置数据
	int eblc = 0;
	uint32_t sbit_table_size = 0;
	int      sbit_table_type = 0;
	uint32_t sbit_num_strikes = 0;
	// EBDT	Embedded bitmap data	嵌入式位图数据 either `CBDT', `EBDT', or `bdat'
	uint32_t ebdt_start = 0;
	uint32_t ebdt_size = 0;
	// EBSC	Embedded bitmap scaling data	嵌入式位图缩放数据
	uint32_t ebsc = 0;
	int format = 0;
};
struct hps_t {
	head_table head;
	hheaTbl hhea;
	post_header post;
};
class stb_font
{
public:
	stb_font()
	{
	}

	~stb_font()
	{
	}
	static font_impl* new_fontinfo()
	{
		return new font_impl();
	}
	static void free_fontinfo(font_impl* p)
	{
		font_impl* pf = (font_impl*)p;
		delete pf;
	}
public:

	static int init(void*)
	{
		return 1;
	}

	static int done(void*)
	{
		return 1;
	}

	static int loadFont(font_impl* font, const void* data, int idx, void* ud)
	{
		int stbError;
		FONS_NOTUSED(idx);

		font->userdata = ud;
		int fso = get_offset(data, idx);
		stbError = stbtt_InitFont(font, (unsigned char*)data, fso);
		stbError = init_table(font, (unsigned char*)data, fso);
		// 字体格式
		font->format = ttUSHORT(font->data + font->index_map + 0);
		return stbError;
	}
	static int get_numbers(const void* data)
	{
		return stbtt_GetNumberOfFonts((unsigned char*)data);
	}
	static int get_offset(const void* data, int idx)
	{
		return stbtt_GetFontOffsetForIndex((unsigned char*)data, idx);
	}
	static void getFontVMetrics(font_impl* font, int* ascent, int* descent, int* lineGap)
	{
		stbtt_GetFontVMetrics(font, ascent, descent, lineGap);
	}
	static void getFontHMetrics(font_impl* font, int glyph, int* advance, int* lsb)
	{
		stbtt_GetGlyphHMetrics(font, glyph, advance, lsb);
		return;
	}

	static float getPixelHeightScale(font_impl* font, float size)
	{
		return size < 0 ? stbtt_ScaleForPixelHeight(font, -size) : stbtt_ScaleForMappingEmToPixels(font, size);
	}

	// 获取utf8字符
	static const char* get_glyph_index_last(font_impl* font, const char* str, int* index)
	{
		uint32_t codepoint = 0;
		uint32_t utf8state = 0;
		*index = -1;
		for (; str && *str && *index == -1; ++str) {
			if (md::fons_decutf8(&utf8state, &codepoint, *(const unsigned char*)str))
				continue;
			*index = stbtt_FindGlyphIndex(font, codepoint);
		}
		return str;
	}

	static uint32_t get_u8_to_u16(const char* str)
	{
		uint32_t codepoint = 0;
		uint32_t utf8state = 0;
		for (; str && *str; ++str) {
			if (md::fons_decutf8(&utf8state, &codepoint, *(const unsigned char*)str))
				continue;
			break;
		}
		return codepoint;
	}
	static const char* get_glyph_index(font_impl* font, const char* str, int* idx)
	{
		uint32_t codepoint = 0;
		uint32_t utf8state = 0;
		int index = -1;
		for (; str && *str && index == -1; ++str) {
			if (md::fons_decutf8(&utf8state, &codepoint, *(const unsigned char*)str))
				continue;
			index = stbtt_FindGlyphIndex(font, codepoint);
		}
		*idx = index;
		return str;
	}


#define BYTE_( p, i )  ( ((const unsigned char*)(p))[(i)] )
#define BYTE_U16( p, i, s1 )  ( (uint16_t)( BYTE_( p, i ) ) << (s1) )
#define BYTE_U32( p, i, s1 )  ( (uint32_t)( BYTE_( p, i ) ) << (s1) )
#define PEEK_USHORT( p )  uint16_t( BYTE_U16( p, 0, 8 ) | BYTE_U16( p, 1, 0 ) )
#define PEEK_LONG( p )  int32_t( BYTE_U32( p, 0, 24 ) | BYTE_U32( p, 1, 16 ) | BYTE_U32( p, 2,  8 ) | BYTE_U32( p, 3,  0 ) )
#define PEEK_ULONG( p )  uint32_t(BYTE_U32( p, 0, 24 ) | BYTE_U32( p, 1, 16 ) | BYTE_U32( p, 2,  8 ) | BYTE_U32( p, 3,  0 ) )

#define NEXT_CHAR( buffer ) ( (signed char)*buffer++ )

#define NEXT_BYTE( buffer ) ( (unsigned char)*buffer++ )
#define NEXT_SHORT( b ) ( (short)( b += 2, PEEK_USHORT( b - 2 ) ) )
#define NEXT_USHORT( b ) ( (unsigned short)( b += 2, PEEK_USHORT( b - 2 ) ) )
#define NEXT_LONG( buffer ) ( (long)( buffer += 4, PEEK_LONG( buffer - 4 ) ) )
#define NEXT_ULONG( buffer ) ( (unsigned long)( buffer += 4, PEEK_ULONG( buffer - 4 ) ) )



	static unsigned char* tt_cmap2_get_subheader(unsigned char* table, uint32_t char_code)
	{
		unsigned char* result = NULL;
		if (char_code < 0x10000UL)
		{
			uint32_t   char_lo = (uint32_t)(char_code & 0xFF);
			uint32_t   char_hi = (uint32_t)(char_code >> 8);
			unsigned char* p = table + 6;    /* keys table */
			unsigned char* subs = table + 518;  /* subheaders table */
			unsigned char* sub;
			if (char_hi == 0)
			{
				/* an 8-bit character code -- we use subHeader 0 in this case */
				/* to test whether the character code is in the charmap       */
				/*                                                            */
				sub = subs;
				p += char_lo * 2;
				if (PEEK_USHORT(p) != 0)sub = 0;
			}
			else
			{
				p += char_hi * 2;
				sub = subs + (uint64_t)PEEK_USHORT(p);
			}
			result = sub;
		}
		return result;
	}

	static int tt_cmap2_char_index(unsigned char* table, uint32_t char_code)
	{
		uint32_t   result = 0;
		unsigned char* subheader;
		subheader = tt_cmap2_get_subheader(table, char_code);
		if (subheader)
		{
			unsigned char* p = subheader;
			uint32_t   idx = ((uint32_t)(char_code) & 0xFF);
			uint32_t   start, count;
			int    delta;
			uint32_t   offset;
			start = NEXT_USHORT(p);
			count = NEXT_USHORT(p);
			delta = NEXT_SHORT(p);
			offset = PEEK_USHORT(p);
			idx -= start;
			if (idx < count && offset != 0)
			{
				p += offset + 2 * idx;
				idx = PEEK_USHORT(p);
				if (idx != 0)
					result = (uint32_t)((int)idx + delta) & 0xFFFFU;
			}
		}
		return result;
	}
	static int get_ext_glyph_index(const stbtt_fontinfo* info, uint32_t codepoint)
	{
		int ret = 0;
		// todo
		return ret;
	}
	// GBK字符串
	static int get_glyph_index2(font_impl* font, const char* t)
	{
		const stbtt_fontinfo* info = font;
		int ret = 0;
		uint8_t* data = info->data;
		uint32_t index_map = info->index_map;
		uint16_t format = ttUSHORT(data + index_map + 0);
		uint32_t codepoint = 0;
		if (format == 2) {
			// @TODO: high-byte mapping for japanese/chinese/korean
			codepoint = (uint32_t)t[0] & 0xff;
			if (codepoint > 127)
			{
				codepoint <<= 8;
				codepoint |= (uint32_t)t[1] & 0xff;
			}
			ret = tt_cmap2_char_index(data + index_map, codepoint);
		}
		return ret;
	}

	static int getGlyphIndex(font_impl* font, int codepoint)
	{
		return font->format == 2 ? get_ext_glyph_index(font, codepoint) : stbtt_FindGlyphIndex(font, codepoint);
	}

	static int buildGlyphBitmap(font_impl* font, int glyph, float scale,
		int* advance, int* lsb, int* x0, int* y0, int* x1, int* y1)
	{
		stbtt_GetGlyphHMetrics(font, glyph, advance, lsb);
		stbtt_GetGlyphBitmapBox(font, glyph, scale, scale, x0, y0, x1, y1);
		return 1;
	}

	static void renderGlyphBitmap(font_impl* font, unsigned char* output, int outWidth, int outHeight, int outStride,
		float scaleX, float scaleY, int glyph)
	{
		stbtt_MakeGlyphBitmap(font, output, outWidth, outHeight, outStride, scaleX, scaleY, glyph);
	}

	static int getGlyphKernAdvance(font_impl* font, int glyph1, int glyph2)
	{
		return stbtt_GetGlyphKernAdvance(font, glyph1, glyph2);
	}
	// 
	static int getKernAdvanceCH(font_impl* font, int ch1, int ch2)
	{
		return stbtt_GetCodepointKernAdvance(font, ch1, ch2);
	}
	static void get_head(font_impl* font, head_table* p)
	{
		if (font && p)
		{
			auto info = font;
			auto data = info->data + info->head;
			DL_long(p->version);
			DL_long(p->fontRevision);
			DL_long(p->checkSumAdjustment);
			DL_long(p->magicNumber);
			DL_ushort(p->flags);
			DL_ushort(p->unitsPerEm);
			int sldt = sizeof(longDateTime);
			memcpy(p->created, data, sldt); data += sldt;
			memcpy(p->modified, data, sldt); data += sldt;
			auto pxy = info->data + info->head + 36;
			DL_ushort(p->xMin);
			DL_ushort(p->yMin);
			DL_ushort(p->xMax);
			DL_ushort(p->yMax);
			DL_ushort(p->macStyle);
			DL_ushort(p->lowestRecPPEM);
			DL_short(p->fontDirectionHint);
			DL_short(p->indexToLocFormat);
			DL_short(p->glyphDataFormat);
		}
	}
	static void get_hhea(font_impl* font, hheaTbl* hhea)
	{
		if (font && hhea)
		{
			auto info = font;
			auto data = info->data + info->hhea;

			DL_long(hhea->version);
			DL_short(hhea->ascender);
			DL_short(hhea->descender);
			DL_short(hhea->lineGap);
			DL_ushort(hhea->advanceWidthMax);
			DL_short(hhea->minLeftSideBearing);
			DL_short(hhea->minRightSideBearing);
			DL_short(hhea->xMaxExtent);
			DL_short(hhea->caretSlopeRise);
			DL_short(hhea->caretSlopeRun);
			DL_short(hhea->caretOffset);
			DL_short(hhea->reserved[0]);
			DL_short(hhea->reserved[1]);
			DL_short(hhea->reserved[2]);
			DL_short(hhea->reserved[3]);
			DL_short(hhea->metricDataFormat);
			DL_ushort(hhea->numberOfLongHorMetrics);
		}
	}
	static glm::ivec4 get_bounding_box(font_impl* font)
	{
		int x0 = 0, y0 = 0, x1, y1; // =0 suppresses compiler warning
		stbtt_GetFontBoundingBox(font, &x0, &y0, &x1, &y1);
		return glm::ivec4(x0, y0, x1, y1);
	}
	static glm::ivec2 get_codepoint_hmetrics(font_impl* font, int ch)
	{
		int advance = 0, lsb = 0;
		stbtt_GetCodepointHMetrics(font, ch, &advance, &lsb);
		return glm::ivec2(advance, lsb);
	}
	static void stbtt_MakeGlyphBitmapSubpixel0(const stbtt_fontinfo* info, unsigned char* output, int out_w, int out_h, int out_stride, float scale_x, float scale_y, float shift_x, float shift_y, int glyph, int xf)
	{
		int ix0, iy0, x1, y1;
		stbtt_vertex* vertices;
		int num_verts = stbtt_GetGlyphShape(info, glyph, &vertices);
		stbtt__bitmap gbm;

		stbtt_GetGlyphBitmapBoxSubpixel(info, glyph, scale_x, scale_y, shift_x, shift_y, &ix0, &iy0, &x1, &y1);
		gbm.pixels = output;
		gbm.w = out_w;
		gbm.h = out_h;
		gbm.stride = out_stride;
		ix0 += xf;	// 修正某些抗锯齿裁剪
		if (gbm.w && gbm.h)
			stbtt_Rasterize(&gbm, 0.35f, vertices, num_verts, scale_x, scale_y, 0, 0, ix0, iy0, 1, info->userdata);

		free(vertices);
	}

	static char* get_glyph_bitmap(font_impl* font, int gidx, double scale, glm::ivec4* ot, std::vector<char>* out, glm::vec3* adlsb, glm::vec2 lcd = { 1, 1 }, int xf = 0)
	{
		int ascent, descent, linegap;
		int x0 = 0, y0 = 0, x1, y1;
		stbtt_GetFontVMetrics(font, &ascent, &descent, &linegap);
		int advancei, lsb;
		stbtt_GetGlyphHMetrics(font, gidx, &advancei, &lsb);
		double adv = advancei * scale;
		double bearing = advancei * scale;
		float shift_x = .0, shift_y = 0.0f;
		stbtt_GetGlyphBitmapBoxSubpixel(font, gidx, scale * lcd.x, scale * lcd.y, shift_x, shift_y, &x0, &y0, &x1, &y1);
		glm::ivec4 ot0 = {};
		if (!ot)ot = &ot0;
		adlsb->z = adv;
		adlsb->x = bearing;
		ot->x = x0;
		ot->y = y0;
		if (!out)
		{
			ot->z = x1 - x0;
			ot->w = y1 - y0;
		}
		size_t pcs = (int64_t)ot->z * ot->w;
		char* pxs = 0;
		if (out)
		{
			if (out->size() < pcs)
			{
				out->resize(pcs);
			}
			pxs = out->data();
			memset(pxs, 0, out->size());
			if (xf == 0)
				stbtt_MakeGlyphBitmapSubpixel(font, (unsigned char*)pxs, ot->z, ot->w, ot->z, scale * lcd.x, scale * lcd.y, shift_x, shift_y, gidx);
			else
				stbtt_MakeGlyphBitmapSubpixel0(font, (unsigned char*)pxs, ot->z, ot->w, ot->z, scale * lcd.x, scale * lcd.y, shift_x, shift_y, gidx, xf);
		}

		return pxs;
	}
public:
	static std::string get_font_name(font_impl* font, std::map<int, std::vector<info_one>>* m)
	{
		int len = 0;
		auto str = getFontNameString(font, m);
		return str ? str : "";
	}
#if 1

	static uint16_t ttUSHORT(uint8_t* p) { return p[0] * 256 + p[1]; }
	static uint16_t ttSHORT(uint8_t* p) { return p[0] * 256 + p[1]; }
	static uint32_t ttULONG(uint8_t* p) { return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3]; }
	static int ttLONG(uint8_t* p) { return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3]; }
#ifndef stbtt_tag4
#define stbtt_tag4(p,c0,c1,c2,c3) ((p)[0] == (c0) && (p)[1] == (c1) && (p)[2] == (c2) && (p)[3] == (c3))
#define stbtt_tag(p,str)           stbtt_tag4(p,str[0],str[1],str[2],str[3])
#endif // !stbtt_tag4

	// @OPTIMIZE: binary search
	static uint32_t find_table(uint8_t* data, uint32_t fontstart, const char* tag)
	{
		int num_tables = ttUSHORT(data + fontstart + 4);
		uint32_t tabledir = fontstart + 12;
		int i;
		char* t = 0;
		for (i = 0; i < num_tables; ++i) {
			uint32_t loc = tabledir + 16 * i;
			t = (char*)data + loc + 0;
			if (stbtt_tag(data + loc + 0, tag))
				return ttULONG(data + loc + 8);
		}
		return 0;
	}
#endif // !1
	static void enum_table(uint8_t* data, uint32_t fontstart, std::map<std::string, sfnt_header>& out)
	{
		char buf[128];
		memcpy(buf, data + fontstart, 128);
		int num_tables = ttUSHORT(data + fontstart + 4);
		memcpy(buf, data + fontstart + 4, 4);
		uint32_t tabledir = fontstart + 12;
		int i;
		char* t = 0;
		std::string n;
		sfnt_header sh;
		if (num_tables > 0)
		{
			for (i = 0; i < num_tables; ++i) {
				uint32_t loc = tabledir + 16 * i;
				t = (char*)data + loc + 0;
				memcpy(buf, t, 128);
				n.assign(t, 4);
				sh.tag = ttULONG(data + loc + 0);
				sh.checksum = ttULONG(data + loc + 4);
				sh.offset = ttULONG(data + loc + 8);
				sh.logicalLength = ttULONG(data + loc + 12);
				out[n] = sh; //{sh.tag, sh.checksum, sh.offset, sh.logicalLength};
			}
		}
	}

	// 获取字体信息
	static const char* getFontNameString(const stbtt_fontinfo* font, std::map<int, std::vector<info_one>>* m)
	{
		int i, count, stringOffset;
		uint8_t* fc = font->data;
		uint32_t offset = font->fontstart;
		uint32_t nm = find_table(fc, offset, "name");
		if (!nm || !m) return 0;
		std::map<int, std::vector<info_one>>& m1 = *m;
		count = ttUSHORT(fc + nm + 2);
		stringOffset = nm + ttUSHORT(fc + nm + 4);
		for (i = 0; i < count; ++i) {
			uint32_t loc = nm + 6 + 12 * i;
			int platform = ttUSHORT(fc + loc + 0);
			int encoding = ttUSHORT(fc + loc + 2);
			int language = ttUSHORT(fc + loc + 4);
			int nameid = ttUSHORT(fc + loc + 6);
			int length = ttUSHORT(fc + loc + 8);
			const char* name = (const char*)(fc + stringOffset + ttUSHORT(fc + loc + 10));
			m1[language].push_back(info_one(platform, encoding, language, nameid, name, length));
		}
		return NULL;
	}


	struct metainfo_t
	{
		uint32_t	version;//	The version of the table format, currently 1
		uint32_t	flags;//	Flags, currently unusedand set to 0
		uint32_t	dataOffset;//	Offset from the beginning of the table to the data
		uint32_t	numDataMaps;//	The number of data maps in the table
	};
	struct DataMaps_t
	{
		char tag[4];
		uint32_t	dataOffset;//	Offset from the beginning of the table to the data for this tag
		uint32_t	dataLength;//	Length of the data.The data is not required to be padded to any byte boundary.
	};
	struct metainfo_tw
	{
		uint32_t	version;//	Version number of the metadata table — set to 1.
		uint32_t	flags;//	Flags — currently unused; set to 0.
		uint32_t	reserved;//	Not used; should be set to 0.
		uint32_t	dataMapsCount;//	The number of data maps in the table.
		DataMaps_t dataMaps[1];//[dataMapsCount]	Array of data map records.
	};
	struct meta_tag_t
	{
		char tag[5];
		std::string v;
	};
	// 获取字体meta信息
	static int get_meta_string(const stbtt_fontinfo* font, std::vector<meta_tag>& mtv)
	{
		int i, count, stringOffset;
		uint8_t* fc = font->data;
		uint32_t offset = font->fontstart;
		uint32_t nm = find_table(fc, offset, "meta");
		if (!nm) return 0;
		//font_t* ttp = (font_t*)font->userdata;
		uint8_t* tp = fc + nm;
		metainfo_tw meta = {};
		uint32_t* ti = (uint32_t*)&meta;

		for (int i = 0; i < 4; i++)
			ti[i] = ttULONG(tp + i * 4);
		if (meta.dataMapsCount > 0)
		{
			auto dm = (tp + 16);
			auto dm1 = (tp);
			mtv.resize(meta.dataMapsCount);
			for (size_t i = 0; i < meta.dataMapsCount; i++)
			{
				auto& it = mtv[i];
				it.tag.assign((char*)dm, 4);
				auto offset = ttULONG(dm + 4);
				auto length = ttULONG(dm + 8);
				if (length > 0)
					it.v.assign((char*)dm1 + offset, length);
				dm += 12;
			}
		}
		return 0;
	}
	class eblc_h
	{
	public:
		uint16_t majorVersion = 0, minorVersion = 0;
		uint32_t numSizes = 0;
	public:
		eblc_h()
		{
		}

		~eblc_h()
		{
		}

	private:

	};
	//// 获取ebdt信息
	//static const char* get_ebdt(font_impl* font_i)
	//{
	//	const stbtt_fontinfo* font = &font_i->font;
	//	int i, count, stringOffset;
	//	uint8_t* fc = font->data;
	//	uint32_t offset = font->fontstart;
	//	uint32_t nm = get_tag(font_i, "EBDT");
	//	uint32_t eblc = get_tag(font_i, "EBLC");
	//	uint32_t ebsc = get_tag(font_i, "EBSC");
	//	if (!nm) return 0;
	//	font_t* ttp = (font_t*)font->userdata;
	//	int majorVersion = ttUSHORT(fc + nm + 0);
	//	int minorVersion = ttUSHORT(fc + nm + 2);
	//	//eblc
	//	uint32_t numSizes = ttULONG(fc + eblc + 4);
	//	char* b = (char*)fc + eblc;
	//	eblc_h* eblcp = (eblc_h*)b;

	//	return NULL;
	//}
public:
	// 获取字体轮廓
	static stbtt_vertex* get_char_shape(font_impl* font, const char* str, int& verCount)
	{
		stbtt_vertex* stbVertex = NULL;
		verCount = 0;
		int idx = 0;
		get_glyph_index(font, str, &idx);
		if (!(idx < 0))
			verCount = stbtt_GetGlyphShape(font, idx, &stbVertex);
		return stbVertex;
	}
	static stbtt_vertex* get_char_shape(font_impl* font, int cp, int& verCount)
	{
		stbtt_vertex* stbVertex = NULL;
		verCount = 0;
		int idx = getGlyphIndex(font, cp);
		if (!(idx < 0))
			verCount = stbtt_GetGlyphShape(font, idx, &stbVertex);
		return stbVertex;
	}
	static void free_shape(stbtt_fontinfo* font, stbtt_vertex* v)
	{
		stbtt_FreeShape(font, v);
	}
	static void free_shape(font_impl* font, stbtt_vertex* v)
	{
		stbtt_FreeShape(font, v);
	}
	static int init_table(font_impl* font, unsigned char* data, int fontstart)
	{
		enum_table(data, fontstart, font->_tb);

		return 1;
	}
private:
};//！stb_font


void test_stbfont()
{
	auto str = L"stb";
	/* 加载字体（.ttf）文件 */
	long int size = 0;
	unsigned char* fontBuffer = NULL;

	FILE* fontFile = fopen("C:\\Windows\\Fonts\\Arial.ttf", "rb");
	if (fontFile == NULL)
	{
		printf("Can not open font file!\n");
		return;
	}
	fseek(fontFile, 0, SEEK_END); /* 设置文件指针到文件尾，基于文件尾偏移0字节 */
	size = ftell(fontFile);       /* 获取文件大小（文件尾 - 文件头  单位：字节） */
	fseek(fontFile, 0, SEEK_SET); /* 重新设置文件指针到文件头 */

	fontBuffer = (unsigned char*)calloc(size, sizeof(unsigned char));
	fread(fontBuffer, size, 1, fontFile);
	fclose(fontFile);

	/* 初始化字体 */
	stbtt_fontinfo info;
	if (!stbtt_InitFont(&info, fontBuffer, 0))
	{
		printf("stb init font failed\n");
	}

	/* 创建位图 */
	int bitmap_w = 512; /* 位图的宽 */
	int bitmap_h = 128; /* 位图的高 */
	unsigned char* bitmap = (unsigned char*)calloc(bitmap_w * bitmap_h, sizeof(unsigned char));

	/* 计算字体缩放 */
	float pixels = 64.0;                                    /* 字体大小（字号） */
	float scale = stbtt_ScaleForPixelHeight(&info, pixels); /* scale = pixels / (ascent - descent) */

	/**
	 * 获取垂直方向上的度量
	 * ascent：字体从基线到顶部的高度；
	 * descent：基线到底部的高度，通常为负值；
	 * lineGap：两个字体之间的间距；
	 * 行间距为：ascent - descent + lineGap。
	*/
	int ascent = 0;
	int descent = 0;
	int lineGap = 0;
	stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

	/* 根据缩放调整字高 */
	ascent = roundf(ascent * scale);
	descent = roundf(descent * scale);

	int x = 0; /*位图的x*/
	auto nw = wcslen(str);
	/* 循环加载str中每个字符 */
	for (int i = 0; i < nw; ++i)
	{
		/**
		  * 获取水平方向上的度量
		  * advanceWidth：字宽；
		  * leftSideBearing：左侧位置；
		*/
		int advanceWidth = 0;
		int leftSideBearing = 0;
		stbtt_GetCodepointHMetrics(&info, str[i], &advanceWidth, &leftSideBearing);

		/* 获取字符的边框（边界） */
		int c_x1, c_y1, c_x2, c_y2;
		stbtt_GetCodepointBitmapBox(&info, str[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

		/* 计算位图的y (不同字符的高度不同） */
		int y = ascent + c_y1;

		/* 渲染字符 */
		int byteOffset = x + roundf(leftSideBearing * scale) + (y * bitmap_w);
		stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, bitmap_w, scale, scale, str[i]);

		/* 调整x */
		x += roundf(advanceWidth * scale);

		/* 调整字距 */
		int kern;
		kern = stbtt_GetCodepointKernAdvance(&info, str[i], str[i + 1]);
		x += roundf(kern * scale);
	}

	/* 将位图数据保存到1通道的png图像中 */
	stbi_write_png("STB.png", bitmap_w, bitmap_h, 1, bitmap, bitmap_w);

	free(fontBuffer);
	free(bitmap);
}

//k=language_id			2052 简体中文	1033 英语

std::string get_info_str(int language, int idx, std::map<int, std::vector<info_one>>& _detail)
{
	std::string ret;
	int ls[] = { language, 1033 };

	for (int i = 0; i < 2 && ret.empty(); i++)
	{
		auto it = _detail.find(ls[i]);
		if (it != _detail.end())
		{
			auto& p = it->second;
			for (size_t j = 0; j < p.size(); j++)
			{
				if (p[j].name_id == idx)
				{
					ret = (p[j].get_name());
				}
			}
		}
	}
	return ret;
}

font_t::font_t()
{
	font = new font_impl();
	hp = new hps_t();
	assert(font);
}
void free_colorinfo(gcolors_t* colorinfo);

font_t::~font_t()
{
	std::map<int, std::vector<info_one*>> detail;
	for (auto& [k, v] : detail)
	{
		for (auto it : v)
		{
			if (it)
			{
				delete it;
			}
		}
	}
	detail.clear();
	if (bitinfo)delete bitinfo; bitinfo = 0;
	if (colorinfo) {
		free_colorinfo(colorinfo);
		delete colorinfo; colorinfo = 0;
	}
	if (font)delete font; font = 0;
	if (hp)delete hp; hp = 0;
}
float font_t::get_scale(int px)
{
	return stb_font::getPixelHeightScale(font, px);
}
void font_t::init_post_table()
{
	hheaTbl hhea[1] = {}; head_table p[1] = {};
	stb_font::get_hhea(font, hhea);
	stb_font::get_head(font, p);
	xMaxExtent = hhea->xMaxExtent;
	lineGap = hhea->lineGap;
}

#ifndef TAGS_H_
#define TAGS_H_


#define MAKE_TAG(_x1, _x2, _x3, _x4) (uint32_t)(((uint32_t)_x1 << 24)|((uint32_t)_x2 << 16)|((uint32_t)_x3 << 8)|(uint32_t)_x4)


#define TAG_avar  "avar"
#define TAG_BASE  "BASE"
#define TAG_bdat  "bdat"
#define TAG_BDF   "BDF "
#define TAG_bhed  "bhed"
#define TAG_bloc  "bloc"
#define TAG_bsln  "bsln"
#define TAG_CBDT  "CBDT"
#define TAG_CBLC  "CBLC"
#define TAG_CFF   "CFF "
#define TAG_CID   "CID "
#define TAG_CPAL  "CPAL"
#define TAG_COLR  "COLR"
#define TAG_cmap  "cmap"
#define TAG_cvar  "cvar"
#define TAG_cvt   "cvt "
#define TAG_DSIG  "DSIG"
#define TAG_EBDT  "EBDT"
#define TAG_EBLC  "EBLC"
#define TAG_EBSC  "EBSC"
#define TAG_feat  "feat"
#define TAG_FOND  "FOND"
#define TAG_fpgm  "fpgm"
#define TAG_fvar  "fvar"
#define TAG_gasp  "gasp"
#define TAG_GDEF  "GDEF"
#define TAG_glyf  "glyf"
#define TAG_GPOS  "GPOS"
#define TAG_GSUB  "GSUB"
#define TAG_gvar  "gvar"
#define TAG_hdmx  "hdmx"
#define TAG_head  "head"
#define TAG_hhea  "hhea"
#define TAG_hmtx  "hmtx"
#define TAG_JSTF  "JSTF"
#define TAG_just  "just"
#define TAG_kern  "kern"
#define TAG_lcar  "lcar"
#define TAG_loca  "loca"
#define TAG_LTSH  "LTSH"
#define TAG_LWFN  "LWFN"
#define TAG_MATH  "MATH"
#define TAG_maxp  "maxp"
#define TAG_META  "META"
#define TAG_MMFX  "MMFX"
#define TAG_MMSD  "MMSD"
#define TAG_mort  "mort"
#define TAG_morx  "morx"
#define TAG_name  "name"
#define TAG_opbd  "opbd"
#define TAG_OS2   "OS/2"
#define TAG_OTTO  "OTTO"
#define TAG_PCLT  "PCLT"
#define TAG_POST  "POST"
#define TAG_post  "post"
#define TAG_prep  "prep"
#define TAG_prop  "prop"
#define TAG_sbix  "sbix"
#define TAG_sfnt  "sfnt"
#define TAG_SING  "SING"
#define TAG_trak  "trak"
#define TAG_true  "true"
#define TAG_ttc   "ttc "
#define TAG_ttcf  "ttcf"
#define TAG_TYP1  "TYP1"
#define TAG_typ1  "typ1"
#define TAG_VDMX  "VDMX"
#define TAG_vhea  "vhea"
#define TAG_vmtx  "vmtx"
#define TAG_wOFF  "wOFF"



#endif /* TAGS_H_ */


sfnt_header* get_tag(font_impl* font_i, const std::string& tag)
{
	auto it = font_i->_tb.find(tag);
	return it != font_i->_tb.end() ? &it->second : nullptr;
}


typedef struct  LayerIterator_
{
	uint32_t   num_layers;
	uint32_t   layer;
	uint8_t* p;

} LayerIterator;
typedef struct  Palette_Data_ {
	uint16_t         num_palettes;
	const uint16_t* palette_name_ids;
	const uint16_t* palette_flags;

	uint16_t         num_palette_entries;
	const uint16_t* palette_entry_name_ids;

} Palette_Data;

union Color_2
{
	uint32_t c;
	struct {
		uint8_t r, g, b, a;
	};
	struct {
		uint8_t red, green, blue, alpha;
	};
};
struct Cpal;
struct Colr;
struct gcolors_t
{
	Cpal* cpal;
	Colr* colr;
	/* glyph colors */
	Palette_Data palette_data;         /* since 2.10 */
	uint16_t palette_index;
	Color_2* palette;
	Color_2 foreground_color;
	bool have_foreground_color;
	char td[80];
};
struct GlyphSlot;

typedef union {
	uint32_t color;
	struct {
		unsigned char b, g, r, a;
	} argb;
} stbtt_color;
bool stbtt_FontHasPalette(const stbtt_fontinfo* info);
unsigned short stbtt_FontPaletteCount(const stbtt_fontinfo* info);
unsigned short stbtt_FontPaletteGetColors(const stbtt_fontinfo* info, unsigned short paletteIndex, stbtt_color** colorPalette);
//// Glyph layers (COLR) /////////////////////////////////////////////////////
typedef struct {
	unsigned short glyphid, colorid;
} stbtt_glyphlayer;
bool stbtt_FontHasLayers(const stbtt_fontinfo* info);
unsigned short stbtt_GetGlyphLayers(const stbtt_fontinfo* info, unsigned short glypId, stbtt_glyphlayer** glyphLayer);
unsigned short stbtt_GetCodepointLayers(const stbtt_fontinfo* info, unsigned short codePoint, stbtt_glyphlayer** glyphLayer);



void free_colorinfo(gcolors_t* colorinfo) {

	if (colorinfo->palette_data.palette_name_ids)delete[]colorinfo->palette_data.palette_name_ids;
	if (colorinfo->palette_data.palette_flags)delete[]colorinfo->palette_data.palette_flags;
	if (colorinfo->palette_data.palette_entry_name_ids)delete[]colorinfo->palette_data.palette_entry_name_ids;
}


int tt_face_load_colr(font_t* face, uint8_t* b, sfnt_header* sp);

void tt_face_free_colr(font_t*);

bool tt_face_get_colr_layer(font_t* face,
	uint32_t            base_glyph,
	uint32_t* aglyph_index,
	uint32_t* acolor_index,
	LayerIterator* iterator);
// 获取颜色
Color_2 get_c2(font_t* face1, uint32_t color_index);

int tt_face_colr_blend_layer(font_t* face, uint32_t       color_index, GlyphSlot* dstSlot, GlyphSlot* srcSlot);


int tt_face_load_cpal(font_t* face, uint8_t* b, sfnt_header* sp);

void tt_face_free_cpal(font_t* face);

int tt_face_palette_set(font_t* face, uint32_t  palette_index);
// 初始化颜色
int font_t::init_color()
{
	font_impl* font_i = font;
	const stbtt_fontinfo* font = font_i;
	int i, count, stringOffset;
	uint8_t* fc = font->data;
	uint32_t offset = font->fontstart, table_size = 0, sbit_num_strikes = 0;
	uint32_t ebdt_start = 0, ebdt_size = 0;
	sfnt_header* ebdt_table = 0;
	auto cpal_table = get_tag(font_i, TAG_CPAL);
	auto colr_table = get_tag(font_i, TAG_COLR);
	if (!cpal_table || !colr_table)
		return 0;
	if (!colorinfo)
		colorinfo = new gcolors_t();
	font_t* ttp = (font_t*)font->userdata;
	uint8_t* b = fc + cpal_table->offset;
	uint8_t* b1 = fc + colr_table->offset;
	tt_face_load_cpal(this, b, cpal_table);
	tt_face_load_colr(this, b1, colr_table);

	return 0;
}

int font_t::get_gcolor(uint32_t base_glyph, std::vector<uint32_t>& ag, std::vector<uint32_t>& col)
{
	uint32_t aglyph_index = base_glyph;
	uint32_t acolor_index = 0;
	LayerIterator it = {};
	for (;;)
	{
		if (!tt_face_get_colr_layer(this, aglyph_index, &aglyph_index, &acolor_index, &it))
		{
			break;
		}
		ag.push_back(aglyph_index);
		col.push_back(get_c2(this, acolor_index).c);
	}
	return it.num_layers;
}


#ifndef no_colr

uint32_t stbtt__find_table(uint8_t* data, uint32_t fontstart, const char* tag)
{
	int32_t num_tables = stb_font::ttUSHORT(data + fontstart + 4);
	uint32_t tabledir = fontstart + 12;
	int32_t i;
	for (i = 0; i < num_tables; ++i) {
		uint32_t loc = tabledir + 16 * i;
		if (stbtt_tag(data + loc + 0, tag))
			return stb_font::ttULONG(data + loc + 8);
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////////
// Glyph layered color font support using COLR/CPAL tables
//

static int stbtt__get_cpal(font_impl* info)
{
	if (info->cpal < 0 && info != NULL) //Load table if not exists
		info->cpal = stbtt__find_table(info->data, info->fontstart, "CPAL");
	return info->cpal;
}

static int stbtt__get_colr(font_impl* info)
{
	if (info->colr < 0 && info != NULL) //Load table if not exists
	{
		info->colr = stbtt__find_table(info->data, info->fontstart, "COLR");
		if (info->colr > 0) //swap bytes on table, so it can be returned
		{
			const uint32_t layerRecordsOffset = stb_font::ttULONG(info->data + info->colr + 8);
			const uint16_t numLayerRecords = stb_font::ttUSHORT(info->data + info->colr + 12);
			unsigned char* colOffset = info->data + info->colr + layerRecordsOffset;
			for (int i = 0; i < numLayerRecords; i++) //Swap bytes
			{
				unsigned short* col = (unsigned short*)colOffset + (2 * i);
				col[0] = (col[0] >> 8) | (col[0] << 8);
				col[1] = (col[1] >> 8) | (col[1] << 8);
			}
		}
	}
	return info->colr;
}

bool stbtt_FontHasLayers(const font_impl* info)
{
	const int table_colr = stbtt__get_colr((font_impl*)info);
	if (table_colr > 0 && info != NULL) //Check table and if theres glyphs
		if (stb_font::ttUSHORT(info->data + table_colr + 2 /*numBaseGlyphRecords*/) > 0)
			return 1;
	return 0;
}

bool stbtt_FontHasPalette(const font_impl* info)
{
	const int table_cpal = stbtt__get_cpal((font_impl*)info);
	if (table_cpal > 0 && info != NULL) //Check table and if theres palettes
		if (stb_font::ttUSHORT(info->data + table_cpal + 4 /*numPalettes*/) > 0)
			return 1;
	return 0;
}

unsigned short stbtt_FontPaletteCount(const font_impl* info)
{
	const int table_cpal = stbtt__get_cpal((font_impl*)info);
	if (table_cpal > 0 && info != NULL) //Check palettes and input
	{
		const uint16_t numPalettes = stb_font::ttUSHORT(info->data + table_cpal + 4);
		return numPalettes; //Success
	}
	return 0; //Failed
}

unsigned short stbtt_FontPaletteGetColors(const font_impl* info, unsigned short paletteIndex, stbtt_color** colorPalette)
{
	const int table_cpal = stbtt__get_cpal((font_impl*)info);
	if (table_cpal > 0 && info != NULL) //Check palettes
	{
		const uint16_t numPaletteEntries = stb_font::ttUSHORT(info->data + table_cpal + 2);
		if (colorPalette)
		{
			const uint16_t numPalettes = stb_font::ttUSHORT(info->data + table_cpal + 4);
			if (paletteIndex > numPalettes - 1) return 0; //Invalid palette index
			const uint32_t colorRecordsArrayOffset = stb_font::ttULONG(info->data + table_cpal + 8);
			const uint16_t colorRecordIndices = stb_font::ttUSHORT(info->data + table_cpal + 12 + (2 * paletteIndex));
			const uint8_t* colorptr = info->data + table_cpal + colorRecordsArrayOffset + (colorRecordIndices * 4);
			*colorPalette = (stbtt_color*)colorptr;
		}
		return numPaletteEntries;
	}
	return 0; //Failed
}

unsigned short stbtt_GetGlyphLayers(const font_impl* info, unsigned short glypId, stbtt_glyphlayer** glyphLayer)
{
	const int table_colr = stbtt__get_colr((font_impl*)info);
	if (table_colr > 0 && info != NULL) //Check glyph table
	{
		const uint16_t numBaseGlyphRecords = stb_font::ttUSHORT(info->data + table_colr + 2);
		const uint32_t baseGlyphRecordsOffset = stb_font::ttULONG(info->data + table_colr + 4);
		const uint32_t layerRecordsOffset = stb_font::ttULONG(info->data + table_colr + 8);
		int32_t low = 0;
		int32_t high = (int32_t)numBaseGlyphRecords;
		while (low < high) // Binary search, lookup glyph table.
		{
			int32_t mid = low + ((high - low) >> 1);
			uint16_t foundGlyphID = stb_font::ttUSHORT(info->data + table_colr + baseGlyphRecordsOffset + (6 * mid));
			if ((uint32_t)glypId < foundGlyphID) //Trim high
				high = mid;
			else if ((uint32_t)glypId > foundGlyphID) //Trim low
				low = mid + 1;
			else //Result found
			{
				const uint16_t numLayers = stb_font::ttUSHORT(info->data + table_colr + baseGlyphRecordsOffset + (6 * mid) + 4);
				if (glyphLayer)
				{
					const uint16_t firstLayerIndex = stb_font::ttUSHORT(info->data + table_colr + baseGlyphRecordsOffset + (6 * mid) + 2);
					const uint8_t* layerptr = info->data + table_colr + layerRecordsOffset + (firstLayerIndex * 4);
					*glyphLayer = (stbtt_glyphlayer*)layerptr;
				}
				return numLayers; //Sucess
			}
		}
	}
	return 0; //Not found, failed
}

unsigned short stbtt_GetCodepointLayers(const font_impl* info, unsigned short codePoint, stbtt_glyphlayer** glyphLayer)
{
	return stbtt_GetGlyphLayers(info, stbtt_FindGlyphIndex(info, codePoint), glyphLayer); //Lookup by glyph id
}
#endif // !no_colr







void init_bitmap_bitdepth(Bitmap_p* bitmap, int bit_depth);
/*
输入
int gidx			字符索引号
double height		期望高度
bool first_bitmap	是否优先查找位图字体
输出
glm::ivec4* ot		x,y,z=width,w=height
std::string* out	输出缓存区
Bitmap_p* bitmap		输出位图信息
返回1成功
*/

int font_t::get_glyph_image(int gidx, double height, glm::ivec4* ot, Bitmap_p* bitmap, std::vector<char>* out, int lcd_type, uint32_t unicode_codepoint, int xf)
{
	int ret = 0;
	if (gidx > 0)
	{
		double scale = get_scale(height);
		//double scale = get_scale_height(height);
		if (height < 0)
		{
			height *= -1;
		}
#ifndef _FONT_NO_BITMAP
		if (first_bitmap)
		{
			// 解析位图
			ret = get_glyph_bitmap(gidx, height, ot, bitmap, out);
			// 找不到位图时尝试用自定义解码
			if (!ret)
				ret = get_custom_decoder_bitmap(unicode_codepoint, height, ot, bitmap, out);
		}
#endif
		if (!ret)
		{
			// 解析轮廓并光栅化 
			glm::vec3 adlsb = { 0,0,height };
			glm::vec2 lcds[] = { {1, 1}, {3, 1}, {1, 3}, {4, 1, } };
			stb_font::get_glyph_bitmap(font, gidx, scale, ot, out, &adlsb, lcds[lcd_type], xf);
			if (bitmap)
			{
				auto hh = hp->hhea;
				auto he = hp->hhea.ascender + hp->hhea.descender + hp->hhea.lineGap;
				auto hef = hp->hhea.ascender - hp->hhea.descender + hp->hhea.lineGap;

				double hed = (scale * he);//ceil
				double hedf = (scale * hef);
				double lg = (scale * hp->hhea.lineGap);
				if (out)
					bitmap->buffer = (unsigned char*)out->data();
				bitmap->width = bitmap->pitch = ot->z;
				bitmap->rows = ot->w;
				bitmap->advance = adlsb.z;
				bitmap->bearingX = adlsb.x;
				bitmap->pixel_mode = PX_GRAY;	//255灰度图
				bitmap->lcd_mode = lcd_type;
				init_bitmap_bitdepth(bitmap, 8);
				ret = 1;
			}
		}
		if (bitmap)
		{
			bitmap->x = ot->x;
			bitmap->y = ot->y;
		}
	}
	return ret;
}


double font_t::get_base_line(double height)
{
	float scale = get_scale(height);
	double f = ascender;
	//return ceil(f * scale);
	return floor(f * scale); // 向下取整
}
int font_t::get_xmax_extent(double height, int* line_gap)
{
	float scale = get_scale(height);
	double f = xMaxExtent, lig = lineGap;
	if (line_gap)
	{
		*line_gap = ceil(lig * scale);
	}
	return ceil(f * scale);
}
// 获取字体最大box
glm::ivec4 font_t::get_bounding_box(double scale, bool is_align0)
{
	auto ret = stb_font::get_bounding_box(font);
	if (is_align0)
	{
		if (ret.x != 0)
		{
			ret.z -= ret.x; ret.x = 0;
		}
		if (ret.y != 0)
		{
			ret.w -= ret.y; ret.y = 0;
		}
	}
	glm::vec4 s = ret;
	s *= scale;
	return ceil(s);
}
tinypath_t font_t::get_shape(const void* str8, int height, std::vector<vertex_f>* opt, int adv)
{
	int vc = 0;
	tinypath_t r = {};
	if (!opt)return r;
	auto p = &r;
	stbtt_vertex* v = stb_font::get_char_shape(font, (char*)str8, vc);
	if (v)
	{
		if (p && vc > 1)
		{
			auto bs = get_shape_box(str8, height);
			r.advance = bs.x;
			r.bearing = { bs.y,0 };
			adv += bs.y;
			auto pss = opt->size();
			r.first = pss;
			opt->resize(pss + vc);
			r.v = opt->data() + pss;
			p->count = vc;
			for (size_t i = 0; i < vc; i++)
			{
				r.v[i].type = v[i].type;
				r.v[i].x = v[i].x;
				r.v[i].y = v[i].y;
				r.v[i].cx = v[i].cx;
				r.v[i].cy = v[i].cy;
				r.v[i].cx1 = v[i].cx1;
				r.v[i].cy1 = v[i].cy1;
			}
			if (height != 0)
			{
				float scale = get_scale(height);
				if (scale > 0)
				{
					r.baseline = ascender * scale;
					auto v1 = r.v;
					for (size_t i = 0; i < vc; i++)
					{
						v1->x *= scale;
						v1->y *= scale;
						v1->cx *= scale;
						v1->cy *= scale;
						v1->cx1 *= scale;
						v1->cy1 *= scale;
						v1++;
					}
				}
			}
			{
				auto v1 = r.v;
				for (size_t i = 0; i < vc; i++)
				{
					v1->x += adv;
					v1->cx += adv;
					v1->cx1 += adv;
					v1++;
				}
			}
		}
		stb_font::free_shape(font, v);
	}
	return r;
}
glm::ivec2 font_t::get_shape_box(const void* str8, int height)
{
	auto ch = stb_font::get_u8_to_u16((char*)str8);
	glm::vec2 v = stb_font::get_codepoint_hmetrics(font, ch);
	if (height != 0)
	{
		float scale = get_scale(height);
		v *= scale;
	}
	return v;
}
glm::ivec2 font_t::get_shape_box(uint32_t ch, int height)
{
	glm::vec2 v = stb_font::get_codepoint_hmetrics(font, ch);
	if (height != 0)
	{
		float scale = get_scale(height);
		v *= scale;
	}
	return v;
}
tinypath_t font_t::get_shape(int cp, int height, std::vector<vertex_f>* opt, int adv)
{
	int vc = 0;
	tinypath_t r = {};
	auto p = &r;
	stbtt_vertex* v = stb_font::get_char_shape(font, cp, vc);
	if (v)
	{
		if (p && vc > 1)
		{
			auto bs = get_shape_box(cp, height);
			r.advance = bs.x;
			r.bearing = { bs.y,0 };
			auto pss = opt->size();
			r.first = pss;
			opt->resize(pss + vc);
			r.v = opt->data() + pss;
			p->count = vc;
			for (size_t i = 0; i < vc; i++)
			{
				r.v[i].type = v[i].type;
				r.v[i].x = v[i].x;
				r.v[i].y = v[i].y;
				r.v[i].cx = v[i].cx;
				r.v[i].cy = v[i].cy;
				r.v[i].cx1 = v[i].cx1;
				r.v[i].cy1 = v[i].cy1;
			}
			if (height != 0)
			{
				float scale = get_scale(height);
				if (scale > 0)
				{
					r.baseline = ascender * scale;
					auto v1 = r.v;
					for (size_t i = 0; i < vc; i++)
					{
						v1->x *= scale;
						v1->y *= scale;
						v1->cx *= scale;
						v1->cy *= scale;
						v1->cx1 *= scale;
						v1->cy1 *= scale;
						v1++;
					}
				}
			}
			{
				auto v1 = r.v;
				for (size_t i = 0; i < vc; i++)
				{
					v1->x += adv;
					v1->cx += adv;
					v1->cx1 += adv;
					v1++;
				}
			}
		}
		stb_font::free_shape(font, v);
	}
	return r;
}

// todo 获取字符大小

glm::ivec4 font_t::get_char_extent(char32_t ch, unsigned char font_size, /*unsigned short font_dpi,*/ std::vector<font_t*>* fallbacks, font_t** oft)
{
	ft_char_s cs;
	cs.v.font_dpi = 0;// font_dpi;
	cs.v.font_size = font_size;
	cs.v.unicode_codepoint = ch;
#if 0
	{
		auto it = _char_lut.find(cs.u);
		if (it != _char_lut.end())
		{
			return it->second;
		}
	}
#endif
	glm::ivec4 ret = {};
	font_t* rfont = nullptr;
	auto g = get_glyph_index(ch, &rfont, fallbacks);
	if (g)
	{
		if (oft)*oft = rfont;
		double fns = font_size;// round((double)font_size * font_dpi / 72.0);
		double scale = rfont->get_scale(fns);
		int x0 = 0, y0 = 0, x1 = 0, y1 = 0, advance, lsb;
		stb_font::buildGlyphBitmap(rfont->font, g, scale, &advance, &lsb, &x0, &y0, &x1, &y1);
		double adv = scale * advance;
		auto bl = rfont->get_base_line(font_size);
		ret = { x1 - x0, y1 - y0, adv, bl };
		//_char_lut[cs.u] = ret;
	}
	return ret;
}

void font_t::clear_char_lut()
{
	//_char_lut.clear();
}

const char* font_t::get_glyph_index_u8(const char* u8str, int* oidx, font_t** renderFont, std::vector<font_t*>* fallbacks)
{
	int g = 0;
	const char* str = u8str;
	if (fallbacks)
	{
		for (auto it : *fallbacks)
		{
			str = stb_font::get_glyph_index(it->font, u8str, &g);
			if (g > 0) {
				*renderFont = it;
				*oidx = g;
				break;
			}
		}
	}
	return str;
}

union gcache_key
{
	uint64_t u;
	struct {
		uint32_t glyph_index;
		uint32_t height;
	}v;
};
font_item_t* font_t::push_gcache(uint32_t glyph_index, uint32_t height, image_ptr_t* img, const glm::ivec4& rect, const glm::ivec2& pos)
{
	gcache_key k = {}; k.v.glyph_index = glyph_index; k.v.height = height;
	auto& pt = _cache_glyphidx[k.u];
	if (!pt)
	{
		if (cache_count == 0)
		{
			cache_data.push_back(new font_item_t[ccount]);
			cache_count = ccount;
		}
		auto npt = cache_data.rbegin();
		pt = *npt;
		pt += ccount - cache_count;
		cache_count--;
	}
	pt->_glyph_index = glyph_index;
	pt->_image = img;
	pt->_dwpos = pos;
	pt->_rect = rect;
	return pt;
}
font_item_t* font_t::get_gcache(uint32_t glyph_index, uint32_t height)
{
	gcache_key k = {}; k.v.glyph_index = glyph_index; k.v.height = height;
	font_item_t* ret = 0;
	auto it = _cache_glyphidx.find(k.u);
	if (it != _cache_glyphidx.end())
	{
		ret = it->second;
	}
	return ret;
}
void font_t::clear_gcache()
{
	for (auto it : cache_data)
	{
		delete[]it;
	}
	cache_data.clear();
	_cache_glyphidx.clear();
}

int font_t::get_glyph_index(uint32_t codepoint, font_t** renderFont, std::vector<font_t*>* fallbacks)
{
	int g = stb_font::getGlyphIndex(font, codepoint);
	if (g == 0) {
		if (fallbacks)
		{
			for (auto it : *fallbacks)
			{
				if (font == it->font)continue;
				int fallbackIndex = stb_font::getGlyphIndex(it->font, codepoint);
				if (fallbackIndex != 0) {
					g = fallbackIndex;
					*renderFont = it;
					break;
				}
			}
		}
	}
	else
	{
		*renderFont = this;
	}
	return g;
}

font_item_t font_t::get_glyph_item(uint32_t glyph_index, uint32_t unicode_codepoint, int fontsize)
{
	uint32_t col = -1;
	int lcd_type = 0;
	int linegap = 4;
	font_t* rfont = this;
	Bitmap_p bitmap[1] = {};
	std::vector<char> bitbuf[1];
	glm::ivec4 rc;
	std::vector<font_item_t>  ps;
	glm::ivec3 rets;
	font_item_t ret = {};
	if (-1 == glyph_index)
		glyph_index = get_glyph_index(unicode_codepoint, &rfont, 0);
	if (rfont && glyph_index)
	{
		auto rp = rfont->get_gcache(glyph_index, fontsize);
		do {
			if (rp)break;

			auto bit = rfont->get_glyph_image(glyph_index, fontsize, &rc, bitmap, 0, lcd_type, unicode_codepoint);
			if (colorinfo && bit)
			{
				glm::ivec4 bs = { rc.x, rc.y, bitmap->width, bitmap->rows };
				std::vector<uint32_t> ag;
				std::vector<uint32_t> cols;
				if (get_gcolor(glyph_index, ag, cols))
				{
					glm::ivec2 pos;
					image_ptr_t* img = 0;
					img = ctx->push_cache_bitmap_old(bitmap, &pos, 0, img, 0);
					for (size_t i = 0; i < ag.size(); i++)
					{
						bitbuf->clear();
						bitbuf->resize(bitmap->width * bitmap->rows);
						// -4修正填充
						auto bit = rfont->get_glyph_image(ag[i], fontsize, &rc, bitmap, bitbuf, lcd_type, unicode_codepoint, 0);
						if (bit)
						{
							auto ps1 = pos;
							ps1.x += bitmap->x - bs.x, ps1.y += bitmap->y + abs(bs.y);
							img = ctx->push_cache_bitmap_old(bitmap, &ps1, cols[i], img, 0);
						}
					}
					glm::ivec4 rc4 = { pos.x, pos.y, bs.z,bs.w };
					if (img)
					{
						rp = rfont->push_gcache(glyph_index, fontsize, img, rc4, { bs.x,bs.y });
						if (rp)
						{
							rp->color = -1;
							rp->advance = bitmap->advance;
						}
					}
					break;
				}
			}
			bit = rfont->get_glyph_image(glyph_index, fontsize, &rc, bitmap, bitbuf, lcd_type, unicode_codepoint);
			if (bit)
			{
				glm::ivec2 pos;
				auto img = ctx->push_cache_bitmap(bitmap, &pos, linegap, col);
				glm::ivec4 rc4 = { pos.x, pos.y, bitmap->width, bitmap->rows };
				if (img)
				{
					rp = rfont->push_gcache(glyph_index, fontsize, img, rc4, { rc.x, rc.y });
					rp->color = 0;
					if (rp)rp->advance = bitmap->advance;
				}
			}
		} while (0);
		if (rp)
			ret = *rp;
	}
	return ret;
}



#ifndef _FONT_NO_BITMAP

#ifdef NO_CPU_LENDIAN
#define UINT8_BITFIELD_BENDIAN
#else
#define UINT8_BITFIELD_LENDIAN
#endif
#ifdef UINT8_BITFIELD_LENDIAN
#define UINT8_BITFIELD(f0, f1, f2, f3, f4, f5, f6, f7) \
        uint8_t f0 : 1; \
        uint8_t f1 : 1; \
        uint8_t f2 : 1; \
        uint8_t f3 : 1; \
        uint8_t f4 : 1; \
        uint8_t f5 : 1; \
        uint8_t f6 : 1; \
        uint8_t f7 : 1;
#else
#define UINT8_BITFIELD(f0, f1, f2, f3, f4, f5, f6, f7) \
        uint8_t f7 : 1; \
        uint8_t f6 : 1; \
        uint8_t f5 : 1; \
        uint8_t f4 : 1; \
        uint8_t f3 : 1; \
        uint8_t f2 : 1; \
        uint8_t f1 : 1; \
        uint8_t f0 : 1;
#endif
#define BYTE_BITFIELD UINT8_BITFIELD
// EBLC头用到的结构
struct SbitLineMetrics {
	char ascender;
	char descender;
	uint8_t widthMax;
	char caretSlopeNumerator;
	char caretSlopeDenominator;
	char caretOffset;
	char minOriginSB;
	char minAdvanceSB;
	char maxBeforeBL;
	char minAfterBL;
	char pad1;
	char pad2;
};

struct BigGlyphMetrics {
	uint8_t height;
	uint8_t width;
	char horiBearingX;
	char horiBearingY;
	uint8_t horiAdvance;
	char vertBearingX;
	char vertBearingY;
	uint8_t vertAdvance;
};

struct SmallGlyphMetrics {
	uint8_t height;
	uint8_t width;
	char bearingX;
	char bearingY;
	uint8_t advance;
};
struct BitmapSizeTable {
	uint32_t indexSubTableArrayOffset; //offset to indexSubtableArray from beginning of EBLC.
	uint32_t indexTablesSize; //number of bytes in corresponding index subtables and array
	uint32_t numberOfIndexSubTables; //an index subtable for each range or format change
	uint32_t colorRef; //not used; set to 0.
	SbitLineMetrics hori; //line metrics for text rendered horizontally
	SbitLineMetrics vert; //line metrics for text rendered vertically
	unsigned short startGlyphIndex; //lowest glyph index for this size
	unsigned short endGlyphIndex; //highest glyph index for this size
	uint8_t ppemX; //horizontal pixels per Em
	uint8_t ppemY; //vertical pixels per Em
	struct BitDepth {
		enum Value : uint8_t {
			BW = 1,
			Gray4 = 2,
			Gray16 = 4,
			Gray256 = 8,
		};
		uint8_t value;
	} bitDepth; //the Microsoft rasterizer v.1.7 or greater supports
	union Flags {
		struct Field {
			//0-7
			BYTE_BITFIELD(
				Horizontal, // Horizontal small glyph metrics
				Vertical,  // Vertical small glyph metrics
				Reserved02,
				Reserved03,
				Reserved04,
				Reserved05,
				Reserved06,
				Reserved07)
		} field;
		struct Raw {
			static const char Horizontal = 1u << 0;
			static const char Vertical = 1u << 1;
			char value;
		} raw;
	} flags;
}; //bitmapSizeTable[numSizes];

struct IndexSubTableArray {
	unsigned short firstGlyphIndex; //first glyph code of this range
	unsigned short lastGlyphIndex; //last glyph code of this range (inclusive)
	uint32_t additionalOffsetToIndexSubtable; //add to BitmapSizeTable::indexSubTableArrayOffset to get offset from beginning of 'EBLC'
}; //indexSubTableArray[BitmapSizeTable::numberOfIndexSubTables];

struct IndexSubHeader {
	unsigned short indexFormat; //format of this indexSubTable
	unsigned short imageFormat; //format of 'EBDT' image data
	uint32_t imageDataOffset; //offset to image data in 'EBDT' table
};

// Variable metrics glyphs with 4 byte offsets
struct IndexSubTable1 {
	IndexSubHeader header;
	//uint32_t offsetArray[lastGlyphIndex - firstGlyphIndex + 1 + 1]; //last element points to one past end of last glyph
	//glyphData = offsetArray[glyphIndex - firstGlyphIndex] + imageDataOffset
};

// All Glyphs have identical metrics
struct IndexSubTable2 {
	IndexSubHeader header;
	uint32_t imageSize; // all glyphs are of the same size
	BigGlyphMetrics bigMetrics; // all glyphs have the same metrics; glyph data may be compressed, byte-aligned, or bit-aligned
};

// Variable metrics glyphs with 2 byte offsets
struct IndexSubTable3 {
	IndexSubHeader header;
	//unsigned short offsetArray[lastGlyphIndex - firstGlyphIndex + 1 + 1]; //last element points to one past end of last glyph, may have extra element to force even number of elements
	//glyphData = offsetArray[glyphIndex - firstGlyphIndex] + imageDataOffset
};

// Variable metrics glyphs with sparse glyph codes
struct IndexSubTable4 {
	IndexSubHeader header;
	uint32_t numGlyphs;
	struct CodeOffsetPair {
		unsigned short glyphCode;
		unsigned short offset; //location in EBDT
	}; //glyphArray[numGlyphs+1]
};

// Constant metrics glyphs with sparse glyph codes
struct IndexSubTable5 {
	IndexSubHeader header;
	uint32_t imageSize; //all glyphs have the same data size
	BigGlyphMetrics bigMetrics; //all glyphs have the same metrics
	uint32_t numGlyphs;
	//unsigned short glyphCodeArray[numGlyphs] //must have even number of entries (set pad to 0)
};

union IndexSubTable {
	IndexSubHeader header;
	IndexSubTable1 format1;
	IndexSubTable2 format2;
	IndexSubTable3 format3;
	IndexSubTable4 format4;
	IndexSubTable5 format5;
};

union GlyphMetrics
{
	struct BigGlyphMetrics _big;
	struct SmallGlyphMetrics _small;
};
class SBitDecoder
{
public:
	Bitmap_p bitmap[1] = {};
	std::vector<char> bitdata;
	BigGlyphMetrics metrics[1] = {};
	bool          metrics_loaded;
	bool          bitmap_allocated;
	uint8_t          bit_depth;

	BitmapSizeTable* _bst;
	std::vector<IndexSubTableArray>* _ist;

	uint8_t* ebdt_base;
	uint8_t* eblc_base;
	uint8_t* eblc_limit;
	uint8_t* p, * p_limit;

	uint32_t _strike_index = 0;
	font_t* _face = 0;

	// 是否在回收状态
	int _recycle = 0;
public:
	SBitDecoder()
	{

	}

	~SBitDecoder()
	{
	}

	int init(font_t* ttp, uint32_t strike_index);
public:
	IndexSubTableArray* get_image_offset(uint32_t glyph_index)
	{
		auto& ist = *_ist;
		for (size_t i = 0; i < ist.size(); i++)
		{
			auto& it = ist[i];
			if (glyph_index >= it.firstGlyphIndex && glyph_index <= it.lastGlyphIndex)
			{
				return &ist[i];
			}
		}
		static IndexSubTableArray a;
		a.firstGlyphIndex = 0;
		a.lastGlyphIndex = 98;
		a.additionalOffsetToIndexSubtable = 0;
		return nullptr;
	}
	int resize_bitmap(uint32_t size)
	{
		if (size > bitdata.size())
		{
			bitmap->capacity = size;
			bitdata.resize(size);
			bitmap->buffer = (unsigned char*)bitdata.data();
		}
		memset(bitmap->buffer, 0, size);
		return 0;
	}
private:

};

// 获取一个字体索引的位图
int get_index(SBitDecoder* decoder, uint32_t glyph_index, int x_pos, int y_pos);
int load_metrics(uint8_t** pp, uint8_t* limit, int big, BigGlyphMetrics* metrics);

class bitmap_ttinfo
{
public:

	sbit_table_type _sttype = sbit_table_type::TYPE_NONE;
	// _sbit_table可能是CBLC、EBLC、bloc、sbix
	sfnt_header* _sbit_table = 0;
	sfnt_header* _ebdt_table = 0;
	sfnt_header* _ebsc_table = 0;
	std::vector<BitmapSizeTable> _bsts;
	std::vector<std::vector<IndexSubTableArray>> _index_sub_table;
	std::unordered_map<uint8_t, uint32_t> _msidx_table;
	std::set<SBitDecoder*> _dec_table;
	std::queue<SBitDecoder*> _free_dec;
	//LockS _sbit_lock;
	font_t* _t = 0;
	// 自定义位图
	image_ptr_t _bimg = {};
	uint32_t* _buf = 0;
	// 支持的大小
	std::vector<glm::ivec2> _chsize;
	// 范围
	std::vector<glm::ivec2> _unicode_rang;
public:
	bitmap_ttinfo() {

	}
	~bitmap_ttinfo() {
		//Image::destroy(_buf);
		if (_buf)
		{
			delete _buf; _buf = 0;
		}
		destroy_all_dec();
	}
	int get_sidx(int height)
	{
		auto it = _msidx_table.find(height);
		return it != _msidx_table.end() ? it->second : -1;
	}
	std::unordered_map<uint8_t, uint32_t>* get_msidx_table()
	{
		return &_msidx_table;
	}

	// 创建sbit解码器
	SBitDecoder* new_SBitDecoder()
	{
		SBitDecoder* p = nullptr;
		//LOCK_W(_sbit_lock);
#ifndef NO_SBIT_RECYCLE
		if (_free_dec.size())
		{
			p = _free_dec.front();
			_free_dec.pop();
			p->_recycle = 0;
		}
		else
#endif // !NO_SBIT_RECYCLE
		{
			p = new SBitDecoder();
			_dec_table.insert(p);
		}
		return p;
	}
	// 回收
	void recycle(SBitDecoder* p)
	{
		if (p)
		{
			//LOCK_W(_sbit_lock);
			// 不是自己的不回收
			auto it = _dec_table.find(p);
			if (it != _dec_table.end() && p->_recycle == 0)
			{
#ifndef NO_SBIT_RECYCLE
				p->_recycle = 1;
				_free_dec.push(p);
#else
				_dec_table.erase(p);
				delete p;
#endif
			}

		}
	}
	void destroy_all_dec()
	{
		//LOCK_W(_sbit_lock);
		for (auto it : _dec_table)
			if (it)delete it;
		_dec_table.clear();
	}

};
int SBitDecoder::init(font_t* ttp, uint32_t strike_index)
{
	int ret = 0;
	SBitDecoder* decoder = this;
	auto face = ttp->bitinfo;
	if (!face->_ebdt_table || !face->_sbit_table
		|| strike_index >= face->_bsts.size() || strike_index >= face->_index_sub_table.size())
		return 0;
	if (_face == ttp && _strike_index == strike_index)
	{
		return 1;
	}
	_face = ttp;
	_strike_index = strike_index;
	auto font_i = ttp->font;
	const stbtt_fontinfo* font = font_i;
	decoder->_bst = &face->_bsts[strike_index];
	decoder->_ist = &face->_index_sub_table[strike_index];
	uint8_t* fc = font->data;
	decoder->eblc_base = fc + face->_sbit_table->offset;
	decoder->ebdt_base = fc + face->_ebdt_table->offset;
	decoder->metrics_loaded = 0;
	decoder->bitmap_allocated = 0;
	decoder->bit_depth = decoder->_bst->bitDepth.value;
	decoder->p = decoder->eblc_base + decoder->_bst->indexSubTableArrayOffset;
	decoder->p_limit = decoder->p + decoder->_bst->indexTablesSize;
	ret = 1;
	return ret;
}

typedef int(*SBitDecoder_LoadFunc)(SBitDecoder* decoder, uint8_t* p, uint8_t* plimit, int x_pos, int y_pos);
// 复制位图到bitmap
static int load_byte_aligned(SBitDecoder* decoder, uint8_t* p, uint8_t* limit, int x_pos, int y_pos)
{
	Bitmap_p* bitmap = decoder->bitmap;
	uint8_t* line;
	int      pitch, width, height, line_bits, h;
	uint32_t     bit_height, bit_width;
	bit_width = bitmap->width;
	bit_height = bitmap->rows;
	pitch = bitmap->pitch;
	line = bitmap->buffer;

	width = bitmap->width;
	height = bitmap->rows;

	line_bits = width * bitmap->bit_depth;

	if (x_pos < 0 || (uint32_t)(x_pos + width) > bit_width ||
		y_pos < 0 || (uint32_t)(y_pos + height) > bit_height)
	{
		printf("tt_sbit_decoder_load_byte_aligned:"
			" invalid bitmap dimensions\n"); return 0;
	}
	if (p + ((line_bits + 7) >> 3) * height > limit)
	{
		printf("tt_sbit_decoder_load_byte_aligned: broken bitmap\n");
		return 0;
	}
	/* now do the blit */
	line += y_pos * pitch + (x_pos >> 3);
	x_pos &= 7;

	if (x_pos == 0)
	{
		for (h = height; h > 0; h--, line += pitch)
		{
			uint8_t* pwrite = line;
			int    w;
			for (w = line_bits; w >= 8; w -= 8)
			{
				pwrite[0] = (uint8_t)(pwrite[0] | *p++);
				pwrite += 1;
			}
			if (w > 0)
				pwrite[0] = (uint8_t)(pwrite[0] | (*p++ & (0xFF00U >> w)));
		}
	}
	else  /* x_pos > 0 */
	{
		for (h = height; h > 0; h--, line += pitch)
		{
			uint8_t* pwrite = line;
			int    w;
			uint32_t   wval = 0;
			for (w = line_bits; w >= 8; w -= 8)
			{
				wval = (uint32_t)(wval | *p++);
				pwrite[0] = (uint8_t)(pwrite[0] | (wval >> x_pos));
				pwrite += 1;
				wval <<= 8;
			}
			if (w > 0)
				wval = (uint32_t)(wval | (*p++ & (0xFF00U >> w)));
			/* 读取所有位，并有'x_pos+w'位要写入 */
			pwrite[0] = (uint8_t)(pwrite[0] | (wval >> x_pos));

			if (x_pos + w > 8)
			{
				pwrite++;
				wval <<= 8;
				pwrite[0] = (uint8_t)(pwrite[0] | (wval >> x_pos));
			}
		}
	}

	return 1;
}
// 按像素位复制
static int load_bit_aligned(SBitDecoder* decoder, uint8_t* p, uint8_t* limit, int x_pos, int y_pos)
{
	Bitmap_p* bitmap = decoder->bitmap;
	int ret = 1;
	uint8_t* line;
	int      pitch, width, height, line_bits, h, nbits;
	uint32_t     bit_height, bit_width;
	unsigned short   rval;

	bit_width = bitmap->width;
	bit_height = bitmap->rows;
	pitch = bitmap->pitch;
	line = bitmap->buffer;

	width = bit_width;
	height = bit_height;

	line_bits = width * bitmap->bit_depth;

	if (x_pos < 0 || (uint32_t)(x_pos + width) > bit_width ||
		y_pos < 0 || (uint32_t)(y_pos + height) > bit_height)
	{
		printf("tt_sbit_decoder_load_bit_aligned:"
			" invalid bitmap dimensions\n");
		return 0;
	}

	if (p + ((line_bits * height + 7) >> 3) > limit)
	{
		printf("tt_sbit_decoder_load_bit_aligned: broken bitmap\n");
		return 0;
	}

	if (!line_bits || !height)
	{
		/* nothing to do */
		return 0;
	}

	/* now do the blit */

	/* adjust `line' to point to the first byte of the bitmap */
	line += (uint64_t)y_pos * pitch + (x_pos >> 3);
	x_pos &= 7;

	/* the higher byte of `rval' is used as a buffer */
	rval = 0;
	nbits = 0;

	for (h = height; h > 0; h--, line += pitch)
	{
		uint8_t* pwrite = line;
		int    w = line_bits;
		/* handle initial byte (in target bitmap) specially if necessary */
		if (x_pos)
		{
			w = (line_bits < 8 - x_pos) ? line_bits : 8 - x_pos;
			if (h == height)
			{
				rval = *p++;
				nbits = x_pos;
			}
			else if (nbits < w)
			{
				if (p < limit)
					rval |= *p++;
				nbits += 8 - w;
			}
			else
			{
				rval >>= 8;
				nbits -= w;
			}

			*pwrite++ |= ((rval >> nbits) & 0xFF) &
				(~(0xFFU << w) << (8 - w - x_pos));
			rval <<= 8;
			w = line_bits - w;
		}

		/* handle medial bytes */
		for (; w >= 8; w -= 8)
		{
			rval |= *p++;
			*pwrite++ |= (rval >> nbits) & 0xFF;
			rval <<= 8;
		}

		/* handle final byte if necessary */
		if (w > 0)
		{
			if (nbits < w)
			{
				if (p < limit)
					rval |= *p++;
				*pwrite |= ((rval >> nbits) & 0xFF) & (0xFF00U >> w);
				nbits += 8 - w;

				rval <<= 8;
			}
			else
			{
				*pwrite |= ((rval >> nbits) & 0xFF) & (0xFF00U >> w);
				nbits -= w;
			}
		}
	}
	return ret;
}

// 解码复制
static int load_compound(SBitDecoder* decoder, uint8_t* p, uint8_t* limit, int x_pos, int y_pos)
{
	Bitmap_p* bitmap = decoder->bitmap;
	int   num_components, error = 0;
	num_components = stb_font::ttUSHORT(p); p += 2;
	for (int i = 0; i < num_components; i++)
	{
		uint32_t  gindex = stb_font::ttUSHORT(p); p += 2;
		char  dx = *p; p++;
		char  dy = *p; p++;
		/* NB: a recursive call */
		error = get_index(decoder, gindex, x_pos + dx, y_pos + dy);
		if (error)
			break;
	}
	return 0;
}
// 分配位置空间
static int alloc_bitmap(SBitDecoder* decoder)
{
	uint32_t     width, height;
	uint32_t    size;
	Bitmap_p* bitmap = decoder->bitmap;
	int bit_depth = decoder->bit_depth;
	BigGlyphMetrics* metrics = decoder->metrics;
	width = metrics->width;
	height = metrics->height;

	bitmap->width = width;
	bitmap->rows = height;
	bitmap->bit_depth = bit_depth;

	switch (bit_depth)
	{
	case 1:
		bitmap->pixel_mode = Pixel_Mode::PX_MONO;
		bitmap->pitch = (int)((bitmap->width + 7) >> 3);
		bitmap->num_grays = 2;
		break;

	case 2:
		bitmap->pixel_mode = Pixel_Mode::PX_GRAY2;
		bitmap->pitch = (int)((bitmap->width + 3) >> 2);
		bitmap->num_grays = 4;
		break;

	case 4:
		bitmap->pixel_mode = Pixel_Mode::PX_GRAY4;
		bitmap->pitch = (int)((bitmap->width + 1) >> 1);
		bitmap->num_grays = 16;
		break;

	case 8:
		bitmap->pixel_mode = Pixel_Mode::PX_GRAY;
		bitmap->pitch = (int)(bitmap->width);
		bitmap->num_grays = 256;
		break;

	case 32:
		bitmap->pixel_mode = Pixel_Mode::PX_BGRA;
		bitmap->pitch = (int)(bitmap->width * 4);
		bitmap->num_grays = 256;
		break;

	default:
		return 0;
	}

	size = bitmap->rows * (uint32_t)bitmap->pitch;

	/* check that there is no empty image */
	if (size == 0)
		return 0;     /* exit successfully! */
	decoder->resize_bitmap(size);
	return 1;
}
// 解析加载位图
static int load_bitmap(
	SBitDecoder* decoder,
	uint32_t glyph_format,
	uint32_t glyph_start,
	uint32_t glyph_size,
	int x_pos, int y_pos)
{
	uint8_t* p, * p_limit, * data;
	data = decoder->ebdt_base + glyph_start;
	p = data;
	p_limit = p + glyph_size;
	//uint8_t* ebdt_base,  Bitmap_p* bitmap,
	// 根据字形格式glyph_format读取数据int bit_depth,
	BigGlyphMetrics* metrics = decoder->metrics;
	switch (glyph_format)
	{
	case 1:
	case 2:
	case 8:
	case 17:
		load_metrics(&p, p_limit, 0, metrics);
		break;
	case 6:
	case 7:
	case 9:
	case 18:
		load_metrics(&p, p_limit, 1, metrics);
		break;
	default:
		return 0;
	}
	SBitDecoder_LoadFunc  loader = 0;
	int ret = 0;
	{
		switch (glyph_format)
		{
		case 1:
		case 6:
			loader = load_byte_aligned;
			break;

		case 2:
		case 7:
		{
			/* Don't trust `glyph_format'.  For example, Apple's main Korean */
			/* system font, `AppleMyungJo.ttf' (version 7.0d2e6), uses glyph */
			/* format 7, but the data is format 6.  We check whether we have */
			/* an excessive number of bytes in the image: If it is equal to  */
			/* the value for a byte-aligned glyph, use the other loading     */
			/* routine.                                                      */
			/*                                                               */
			/* Note that for some (width,height) combinations, where the     */
			/* width is not a multiple of 8, the sizes for bit- and          */
			/* byte-aligned data are equal, for example (7,7) or (15,6).  We */
			/* then prefer what `glyph_format' specifies.                    */

			int  width = metrics->width;
			int  height = metrics->height;

			int  bit_size = (width * height + 7) >> 3;
			int  byte_size = height * ((width + 7) >> 3);


			if (bit_size < byte_size &&
				byte_size == (int)(p_limit - p))
				loader = load_byte_aligned;
			else
				loader = load_bit_aligned;
		}
		break;

		case 5:
			loader = load_bit_aligned;
			break;

		case 8:
			if (p + 1 > p_limit)
				return 0;

			p += 1;  /* skip padding */
			/* fall-through */

		case 9:
			loader = load_compound;
			break;

		case 17: /* small metrics, PNG image data   */
		case 18: /* big metrics, PNG image data     */
		case 19: /* metrics in EBLC, PNG image data */
#ifdef FT_CONFIG_OPTION_USE_PNG
			loader = load_png;
			break;
#else
			return 0;
#endif /* FT_CONFIG_OPTION_USE_PNG */

		default:
			return 0;
		}
		ret = alloc_bitmap(decoder);
		if (!ret)
			return ret;
	}

	ret = loader(decoder, p, p_limit, x_pos, y_pos);

	return ret;
}

// 获取一个字体索引的位图
int get_index(SBitDecoder* decoder, uint32_t glyph_index, int x_pos, int y_pos)
{
	uint32_t image_start = 0, image_end = 0, image_offset = 0;
	uint32_t   start, end, index_format, image_format;
	uint8_t* p = decoder->p, * p_limit = decoder->p_limit;
	auto it = decoder->get_image_offset(glyph_index);
	if (!it)
	{
		return 0;
	}
	start = it->firstGlyphIndex;
	end = it->lastGlyphIndex;
	image_offset = it->additionalOffsetToIndexSubtable;
	if (!image_offset)
	{
		//return 0;
	}
	p += image_offset;
	//p_limit
	index_format = stb_font::ttUSHORT(p);
	image_format = stb_font::ttUSHORT(p + 2);
	image_offset = stb_font::ttULONG(p + 4);
	p += 8;
	switch (index_format)
	{
	case 1: /* 4-byte offsets relative to `image_offset' */
		p += (uint64_t)4 * (glyph_index - start);
		if (p + 8 > p_limit)
			return 0;
		image_start = stb_font::ttULONG(p);
		p += 4;
		image_end = stb_font::ttULONG(p);
		p += 4;
		if (image_start == image_end)  /* missing glyph */
			return 0;
		break;
	case 2: /* big metrics, constant image size */
	{
		uint32_t  image_size;
		if (p + 12 > p_limit)
			return 0;
		image_size = stb_font::ttULONG(p);
		p += 4;
		if (!load_metrics(&p, p_limit, 1, decoder->metrics))return 0;
		image_start = image_size * (glyph_index - start);
		image_end = image_start + image_size;
	}
	break;
	case 3: /* 2-byte offsets relative to 'image_offset' */
		p += (uint64_t)2 * (glyph_index - start);
		if (p + 4 > p_limit)
			return 0;
		image_start = stb_font::ttUSHORT(p);
		p += 2;
		image_end = stb_font::ttUSHORT(p);
		p += 2;
		if (image_start == image_end)  /* missing glyph */
			return 0;
		break;
	case 4: /* sparse glyph array with (glyph,offset) pairs */
	{
		uint32_t  mm, num_glyphs;
		if (p + 4 > p_limit)
			return 0;
		num_glyphs = stb_font::ttULONG(p);
		p += 4;
		/* overflow check for p + ( num_glyphs + 1 ) * 4 */
		if (p + 4 > p_limit ||
			num_glyphs > (uint32_t)(((p_limit - p) >> 2) - 1))
			return 0;
		for (mm = 0; mm < num_glyphs; mm++)
		{
			uint32_t  gindex = stb_font::ttUSHORT(p);
			p += 2;
			if (gindex == glyph_index)
			{
				image_start = stb_font::ttUSHORT(p);
				p += 4;
				image_end = stb_font::ttUSHORT(p);
				break;
			}
			p += 2;
		}
		if (mm >= num_glyphs)
			return 0;
	}
	break;
	case 5: /* constant metrics with sparse glyph codes */
	case 19:
	{
		uint32_t  image_size, mm, num_glyphs;
		if (p + 16 > p_limit)
			return 0;
		image_size = stb_font::ttULONG(p);
		p += 4;
		if (!load_metrics(&p, p_limit, 1, decoder->metrics))return 0;
		num_glyphs = stb_font::ttULONG(p);
		p += 4;
		/* overflow check for p + 2 * num_glyphs */
		if (num_glyphs > (uint32_t)((p_limit - p) >> 1))
			return 0;

		for (mm = 0; mm < num_glyphs; mm++)
		{
			uint32_t  gindex = stb_font::ttUSHORT(p);
			p += 2;
			if (gindex == glyph_index)
				break;
		}
		if (mm >= num_glyphs)
			return 0;
		image_start = image_size * mm;
		image_end = image_start + image_size;
	}
	break;

	default:
		return 0;
	}
	if (image_start > image_end)
	{
		return 0;
	}
	image_end -= image_start;
	image_start = image_offset + image_start;
	return load_bitmap(decoder, image_format, image_start, image_end, x_pos, y_pos);
}

// 获取子表
static void get_index_sub_table(uint8_t* d, uint32_t n, std::vector<IndexSubTableArray>& out)
{
	out.resize(n);
	for (int i = 0; i < n; i++)
	{
		out[i].firstGlyphIndex = stb_font::ttUSHORT(d);
		out[i].lastGlyphIndex = stb_font::ttUSHORT(d + 2);
		out[i].additionalOffsetToIndexSubtable = stb_font::ttULONG(d + 4);
		d += 8;
	}
	return;
}
// 获取BitmapSizeTable
njson get_bitmap_size_table(uint8_t* blc, uint32_t count,
	std::vector<BitmapSizeTable>& bsts,
	std::vector<std::vector<IndexSubTableArray>>& index_sub_table, std::unordered_map<uint8_t, uint32_t>& ms)
{
	auto b = blc + 8;
	bsts.resize(count);
	index_sub_table.resize(count);
	njson ns, n;
	for (size_t i = 0; i < count; i++)
	{
		auto& it = bsts[i];
		it.indexSubTableArrayOffset = stb_font::ttULONG(b); //offset to indexSubtableArray from beginning of EBLC.
		b += 4;
		it.indexTablesSize = stb_font::ttULONG(b); //number of bytes in corresponding index subtables and array
		b += 4;
		it.numberOfIndexSubTables = stb_font::ttULONG(b); //an index subtable for each range or format change
		b += 4;
		it.colorRef = stb_font::ttULONG(b); //not used; set to 0.
		b += 4;
		memcpy(&it.hori, b, sizeof(SbitLineMetrics));
		b += sizeof(SbitLineMetrics);
		memcpy(&it.vert, b, sizeof(SbitLineMetrics));
		b += sizeof(SbitLineMetrics);
		it.startGlyphIndex = stb_font::ttUSHORT(b); //lowest glyph index for this size
		b += 2;
		it.endGlyphIndex = stb_font::ttUSHORT(b); //highest glyph index for this size
		b += 2;
		it.ppemX = *b; //horizontal pixels per Em
		b += 1;
		it.ppemY = *b; //vertical pixels per Em
		b += 1;
		it.bitDepth.value = *b;
		b += 1;
		it.flags.raw.value = *b;
		b += 1;
		get_index_sub_table(blc + it.indexSubTableArrayOffset, it.numberOfIndexSubTables, index_sub_table[i]);
		n["ppem"] = { it.ppemX, it.ppemY };
		ms[it.ppemY] = i;
		n["id"] = i;
		ns.push_back(n);
	}
	return ns;
}
int load_metrics(uint8_t** pp, uint8_t* limit, int big, BigGlyphMetrics* metrics)
{
	uint8_t* p = *pp;
	if (p + 5 > limit)
		return 0;
	metrics->height = p[0];
	metrics->width = p[1];
	metrics->horiBearingX = (char)p[2];
	metrics->horiBearingY = (char)p[3];
	metrics->horiAdvance = p[4];
	p += 5;
	if (big)
	{
		if (p + 3 > limit)
			return 0;
		metrics->vertBearingX = (char)p[0];
		metrics->vertBearingY = (char)p[1];
		metrics->vertAdvance = p[2];
		p += 3;
	}
	else
	{
		/* avoid uninitialized data in case there is no vertical info -- */
		metrics->vertBearingX = 0;
		metrics->vertBearingY = 0;
		metrics->vertAdvance = 0;
	}
	*pp = p;
	return 1;
}


unsigned char fdata[2180] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
	0x00, 0x00, 0x02, 0xF0, 0x00, 0x00, 0x00, 0x2A, 0x01, 0x03, 0x00, 0x00, 0x00, 0xEF, 0x52, 0xFD,
	0x59, 0x00, 0x00, 0x00, 0x03, 0x73, 0x42, 0x49, 0x54, 0x08, 0x08, 0x08, 0xDB, 0xE1, 0x4F, 0xE0,
	0x00, 0x00, 0x00, 0x06, 0x50, 0x4C, 0x54, 0x45, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x55, 0xC2,
	0xD3, 0x7E, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0E, 0x9C, 0x00, 0x00,
	0x0E, 0x9C, 0x01, 0x07, 0x94, 0x53, 0xDD, 0x00, 0x00, 0x00, 0x1C, 0x74, 0x45, 0x58, 0x74, 0x53,
	0x6F, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x00, 0x41, 0x64, 0x6F, 0x62, 0x65, 0x20, 0x46, 0x69,
	0x72, 0x65, 0x77, 0x6F, 0x72, 0x6B, 0x73, 0x20, 0x43, 0x53, 0x36, 0xE8, 0xBC, 0xB2, 0x8C, 0x00,
	0x00, 0x00, 0x16, 0x74, 0x45, 0x58, 0x74, 0x43, 0x72, 0x65, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x20,
	0x54, 0x69, 0x6D, 0x65, 0x00, 0x31, 0x32, 0x2F, 0x31, 0x30, 0x2F, 0x32, 0x30, 0x4A, 0xBE, 0xBD,
	0xE5, 0x00, 0x00, 0x07, 0xCB, 0x49, 0x44, 0x41, 0x54, 0x48, 0x89, 0xAD, 0xD6, 0x5F, 0x6C, 0x14,
	0xC7, 0x19, 0x00, 0x70, 0x13, 0xA3, 0x1C, 0x8A, 0x48, 0x5C, 0x13, 0xA9, 0xBA, 0x07, 0x83, 0xA3,
	0x82, 0x1A, 0xF1, 0xD0, 0x2A, 0x4D, 0x54, 0xC5, 0xFC, 0x8B, 0x41, 0xED, 0x0B, 0x2A, 0x6D, 0xF2,
	0x80, 0xED, 0x2A, 0x04, 0xDC, 0x88, 0x4A, 0xA9, 0x82, 0x8C, 0x4D, 0x08, 0x77, 0x29, 0x9B, 0xE3,
	0x72, 0x3D, 0xA9, 0x26, 0xAA, 0xCB, 0x29, 0x42, 0x15, 0x95, 0x90, 0xB9, 0x07, 0x94, 0xA2, 0xE0,
	0x82, 0x43, 0x2D, 0xDF, 0x62, 0x6F, 0xC6, 0x1B, 0xB3, 0x84, 0x93, 0xD5, 0x86, 0x93, 0x52, 0xA1,
	0x85, 0x2E, 0x7B, 0x13, 0x74, 0x45, 0x2B, 0xD5, 0xEC, 0x7D, 0xB6, 0x4E, 0xEE, 0x60, 0x8F, 0xF7,
	0xA6, 0xDF, 0xEC, 0xDE, 0x81, 0x6D, 0x6C, 0xE7, 0x8F, 0x3A, 0x2F, 0x77, 0x3B, 0xBB, 0xFB, 0x9B,
	0x6F, 0xBF, 0xF9, 0x66, 0x76, 0x6B, 0x44, 0xBB, 0x10, 0xDC, 0xD3, 0xC5, 0xD7, 0x68, 0xB3, 0xF1,
	0x54, 0x1C, 0x16, 0xE9, 0x2F, 0x2B, 0x67, 0x5E, 0xA9, 0x59, 0xA2, 0x89, 0x76, 0x26, 0x38, 0xA7,
	0xF2, 0xB2, 0xAF, 0x1A, 0xC3, 0x83, 0xA2, 0xCE, 0x66, 0xC5, 0xB4, 0x48, 0xE7, 0xE8, 0x40, 0x76,
	0x0E, 0xCF, 0xED, 0x8F, 0x97, 0xE2, 0xF3, 0xC3, 0x05, 0x35, 0x1A, 0xF0, 0xFB, 0xF3, 0x4D, 0x3C,
	0xD4, 0xB4, 0x3F, 0x25, 0xBC, 0xF2, 0xFD, 0xC9, 0xDB, 0x4D, 0x3F, 0x59, 0xD9, 0xB0, 0xEB, 0xF1,
	0x15, 0x8F, 0x3F, 0x73, 0xDC, 0xE8, 0x69, 0xDA, 0xF1, 0xDC, 0x67, 0x3B, 0x37, 0xAC, 0x05, 0x8C,
	0xBD, 0x2C, 0x3C, 0xD1, 0x09, 0x54, 0x63, 0x73, 0x79, 0x3A, 0xB1, 0x14, 0x4F, 0x67, 0x6E, 0xB4,
	0xEA, 0x0C, 0x8E, 0xE1, 0x65, 0x7B, 0x7B, 0xCD, 0xC3, 0xC7, 0xE8, 0xDE, 0xA8, 0xE0, 0x22, 0x7A,
	0x30, 0x6F, 0x76, 0x64, 0xAC, 0x0E, 0xAA, 0x75, 0x7E, 0xDF, 0xEC, 0x88, 0x98, 0x2D, 0x99, 0x1B,
	0x07, 0x2E, 0x10, 0x56, 0xE1, 0x31, 0x45, 0x3C, 0xE0, 0xD9, 0x57, 0xF2, 0x13, 0x6D, 0xBF, 0xD0,
	0x99, 0xBB, 0x0F, 0xAF, 0xB2, 0x6D, 0x7A, 0x38, 0x56, 0xB6, 0xA3, 0x71, 0x56, 0xAB, 0xBE, 0x7A,
	0xDD, 0x74, 0x2F, 0xAA, 0x2E, 0x1D, 0x86, 0x1F, 0xB6, 0x21, 0xCF, 0x90, 0xEF, 0xFF, 0x56, 0xFC,
	0x8C, 0xD3, 0xFA, 0x32, 0x2B, 0x36, 0x35, 0xD7, 0x96, 0x6D, 0xFA, 0xC9, 0xF8, 0x93, 0x2F, 0x15,
	0xDE, 0xF0, 0x40, 0xE4, 0xBF, 0xB8, 0xF8, 0xA2, 0x7B, 0xE2, 0xC5, 0x3B, 0x74, 0x08, 0xFE, 0xD2,
	0xB6, 0x3F, 0xB2, 0x89, 0x65, 0x06, 0xA3, 0xFD, 0x31, 0xC9, 0x5F, 0xDF, 0x7A, 0xE8, 0x7A, 0xC3,
	0xFA, 0xF4, 0xE6, 0xE8, 0xEA, 0xAD, 0xC6, 0xD5, 0x3F, 0xBF, 0xBE, 0xE1, 0x4F, 0x3B, 0x9F, 0x6F,
	0xE0, 0xB0, 0x0C, 0x5F, 0xFA, 0xA8, 0x9D, 0x15, 0x5F, 0xC2, 0x99, 0xB3, 0x69, 0x7C, 0xCF, 0xDB,
	0xB6, 0x7D, 0x4A, 0x40, 0xAD, 0xFD, 0xC5, 0x45, 0x8C, 0x3E, 0xF3, 0x03, 0x8C, 0xBE, 0x55, 0xED,
	0x8C, 0x8C, 0xB0, 0xCC, 0x30, 0xA3, 0x31, 0x8E, 0x3C, 0x66, 0x4C, 0x55, 0x3A, 0xE8, 0x30, 0x6B,
	0xB5, 0x3A, 0x32, 0x0A, 0x0E, 0x7B, 0x40, 0x1D, 0x59, 0x26, 0x7A, 0x31, 0x71, 0x64, 0xAC, 0xDD,
	0x4F, 0x8E, 0x67, 0xE7, 0xC5, 0x8E, 0x29, 0xDB, 0x66, 0xF8, 0xC8, 0xE4, 0xD6, 0xEF, 0x4D, 0x77,
	0x84, 0xC9, 0xDC, 0xB7, 0x9E, 0xEA, 0xFC, 0x2E, 0x8D, 0xAA, 0x9B, 0xC7, 0xF3, 0x9E, 0xE4, 0xAF,
	0x59, 0x6E, 0xF7, 0xBB, 0x2E, 0x1D, 0xF2, 0xF9, 0x59, 0x96, 0xB9, 0xCC, 0xD4, 0x63, 0xCB, 0xF1,
	0x33, 0x37, 0x3E, 0xEA, 0xE7, 0x54, 0xA6, 0x13, 0x65, 0xE5, 0x88, 0x6D, 0x77, 0xE2, 0xD4, 0xC6,
	0x8B, 0x19, 0x9C, 0x5A, 0x8E, 0xFC, 0xC1, 0x56, 0xB3, 0xF3, 0xA0, 0x19, 0x55, 0x8F, 0x8E, 0xE7,
	0xED, 0x4A, 0xF4, 0xBA, 0xE2, 0x56, 0xA2, 0x17, 0xB2, 0x3F, 0xB7, 0x1C, 0x4F, 0x67, 0x0A, 0x99,
	0x76, 0x2C, 0x4C, 0x28, 0xF3, 0x42, 0x4F, 0x78, 0xF2, 0xC4, 0xB6, 0x42, 0x3A, 0xE6, 0x95, 0xBD,
	0xC9, 0xE7, 0x9A, 0x8C, 0x95, 0xEF, 0xFE, 0x23, 0x74, 0xA9, 0xE6, 0x95, 0x5F, 0x66, 0x7B, 0xB6,
	0x17, 0x72, 0x47, 0xC7, 0x7A, 0xD6, 0xE2, 0xA4, 0x88, 0x2B, 0x5B, 0x27, 0xAE, 0x35, 0xFC, 0x34,
	0x35, 0xB0, 0x63, 0xF7, 0xAE, 0xC2, 0xD5, 0x77, 0x0E, 0x19, 0x47, 0xC7, 0x8A, 0xEB, 0x96, 0x8B,
	0x5E, 0xCE, 0x3E, 0xAE, 0x5A, 0x2A, 0x1E, 0x54, 0xB2, 0x37, 0x67, 0x25, 0xF9, 0x4B, 0xCD, 0xA9,
	0xAC, 0xDA, 0x58, 0x2A, 0xF8, 0xC3, 0x75, 0x7F, 0x09, 0x42, 0x70, 0x07, 0x2D, 0x2B, 0xE9, 0x65,
	0xF8, 0xE5, 0xDA, 0x6C, 0x7C, 0xFE, 0xF1, 0x44, 0x73, 0xD0, 0xEB, 0xF7, 0x67, 0x0B, 0x7E, 0x5F,
	0x33, 0x26, 0xF3, 0x5B, 0xF2, 0x4B, 0x8D, 0x2A, 0x72, 0x4B, 0x9D, 0x8A, 0xF3, 0x4F, 0xE7, 0xF1,
	0xA6, 0x88, 0x0B, 0xC7, 0xD3, 0x83, 0x73, 0x5F, 0x0B, 0x4F, 0xDD, 0xE7, 0x71, 0xB6, 0xC4, 0xB9,
	0xDC, 0x6F, 0xA7, 0xB6, 0xCF, 0xE5, 0xA1, 0xFF, 0xE0, 0x16, 0x85, 0x05, 0x9B, 0x4E, 0xD9, 0x81,
	0x54, 0xE3, 0xE5, 0x4D, 0x5F, 0x1A, 0x98, 0x63, 0xCA, 0x44, 0x61, 0x09, 0x82, 0x94, 0xE5, 0xB5,
	0x71, 0x91, 0x9E, 0x15, 0x69, 0xB8, 0x37, 0x3F, 0x7F, 0x34, 0x56, 0x9E, 0x17, 0x3D, 0xB4, 0x77,
	0xB4, 0x1C, 0x41, 0x5E, 0xFA, 0x65, 0x2B, 0x15, 0x01, 0xED, 0xBF, 0x45, 0x12, 0xC5, 0xED, 0x93,
	0x4F, 0xDA, 0xC5, 0xBA, 0x55, 0x5D, 0xCF, 0x74, 0x1D, 0x4F, 0xAE, 0xDC, 0x3E, 0xF9, 0xD8, 0x0B,
	0xC9, 0xEB, 0x75, 0xAB, 0x56, 0x3C, 0xF6, 0xC4, 0xAA, 0xFA, 0xD0, 0x48, 0x39, 0x78, 0x52, 0xCA,
	0xE3, 0x14, 0x8A, 0x62, 0x01, 0x2F, 0xE6, 0xF3, 0xB3, 0x7F, 0xDC, 0x3D, 0x05, 0x0C, 0xDA, 0x30,
	0x3D, 0x65, 0x02, 0x11, 0xD8, 0x02, 0x1E, 0x51, 0xF0, 0x6E, 0xAF, 0xF8, 0x37, 0x88, 0xB8, 0x9A,
	0x7A, 0xD1, 0x86, 0xBC, 0x3B, 0xFD, 0x42, 0x87, 0xAD, 0x46, 0xC6, 0x0F, 0xA8, 0x6F, 0xDA, 0xC3,
	0xF0, 0x90, 0xC7, 0x7A, 0x2B, 0x57, 0xF9, 0xF8, 0xE2, 0xBC, 0xE8, 0x78, 0x7B, 0x4A, 0x30, 0x48,
	0x21, 0xEF, 0x11, 0x60, 0xC5, 0xCB, 0xBF, 0x41, 0x3E, 0x57, 0x9E, 0x58, 0x57, 0x3C, 0x6F, 0x1E,
	0x71, 0x2D, 0x96, 0x47, 0xDE, 0x9E, 0x1E, 0xD8, 0x67, 0x2B, 0x11, 0xA1, 0xA8, 0x6F, 0x8E, 0x9B,
	0xDF, 0x8C, 0x2F, 0xE7, 0x8F, 0xBC, 0x15, 0x67, 0xD0, 0x1F, 0xF0, 0x70, 0x46, 0x3B, 0x17, 0x33,
	0x0E, 0xE7, 0x88, 0x10, 0x31, 0xE4, 0x3D, 0x2B, 0x42, 0x91, 0x27, 0xFC, 0xC7, 0xC8, 0xD7, 0xC5,
	0x95, 0xE1, 0xFF, 0x8C, 0x9B, 0x54, 0x97, 0xBB, 0x77, 0xB6, 0xEE, 0xA9, 0xAD, 0xBF, 0x9A, 0xF8,
	0x67, 0xEA, 0x44, 0xAA, 0xEB, 0xD7, 0x93, 0xC9, 0x2B, 0xCF, 0xAE, 0x7E, 0xFA, 0x7B, 0x4F, 0xEC,
	0xAC, 0x0F, 0x3D, 0xCA, 0xD3, 0xCD, 0xE7, 0x31, 0xFA, 0xDD, 0xA1, 0x89, 0x46, 0x8E, 0xBC, 0xAB,
	0xF7, 0x2B, 0xD6, 0x1F, 0xE4, 0xD2, 0x52, 0xCE, 0x9B, 0x4F, 0x7A, 0x37, 0xBB, 0xE8, 0x28, 0xE4,
	0xB7, 0xCD, 0x0C, 0xEE, 0x33, 0x94, 0xF6, 0x34, 0xF2, 0x0C, 0x74, 0x9F, 0xD7, 0x22, 0xAE, 0xA5,
	0xB9, 0x9F, 0x43, 0x91, 0x1E, 0x1D, 0x38, 0x68, 0x6B, 0x67, 0x40, 0x3E, 0x99, 0xD6, 0x89, 0xE7,
	0x16, 0xE4, 0xBE, 0xEE, 0x35, 0xC9, 0x53, 0x8C, 0x1E, 0x79, 0x7A, 0xAF, 0xBD, 0x5F, 0x21, 0x5C,
	0x60, 0x72, 0xF8, 0xF9, 0x9C, 0x8C, 0xFE, 0xF6, 0x5E, 0x4C, 0x0E, 0xF2, 0x54, 0xE9, 0x04, 0x65,
	0xF8, 0xDF, 0xE3, 0x15, 0xBE, 0x10, 0xF1, 0x6E, 0x6E, 0x71, 0x6D, 0x7C, 0x3F, 0x1E, 0x55, 0x91,
	0xD7, 0x9B, 0xE5, 0xD0, 0x26, 0x5D, 0x18, 0x3D, 0xBE, 0x45, 0x5E, 0xFB, 0x2B, 0xF2, 0x72, 0x99,
	0x33, 0x02, 0x23, 0xA0, 0xF9, 0x3C, 0x4E, 0xAD, 0xB8, 0xA4, 0x21, 0x1F, 0xE4, 0x7E, 0x26, 0xB3,
	0x8F, 0xFE, 0x4C, 0xF2, 0x53, 0xE3, 0x10, 0xD7, 0x47, 0x90, 0xB7, 0x22, 0x9E, 0xC5, 0x4B, 0xC8,
	0x37, 0x07, 0x3C, 0x5D, 0x9C, 0x87, 0xB2, 0xFD, 0xEA, 0x05, 0x1D, 0xEB, 0xFE, 0x13, 0x51, 0x62,
	0x16, 0x56, 0x8E, 0xA6, 0x2A, 0x44, 0xCD, 0x61, 0x61, 0x4E, 0xE7, 0x35, 0x59, 0x39, 0x45, 0x59,
	0x39, 0xFF, 0xCA, 0x74, 0x50, 0x35, 0x02, 0x0A, 0x99, 0xB2, 0x21, 0xEA, 0xF3, 0x7E, 0x72, 0xC0,
	0x96, 0xC9, 0x91, 0xFC, 0x08, 0x28, 0xDB, 0x70, 0x68, 0x75, 0x91, 0xA9, 0xDD, 0xF2, 0x56, 0x9C,
	0xCB, 0x12, 0x28, 0x31, 0x27, 0x19, 0x6A, 0x6C, 0x78, 0x87, 0x1B, 0xB9, 0x9C, 0x4E, 0xD9, 0xF4,
	0x9D, 0xEF, 0xD4, 0x85, 0xBB, 0xD6, 0x4F, 0x1E, 0x4F, 0xDE, 0xDE, 0xBE, 0x26, 0x79, 0x38, 0x75,
	0xED, 0xF9, 0x75, 0x5B, 0x63, 0x53, 0x3D, 0xD7, 0x42, 0x3E, 0x3F, 0x5A, 0xB7, 0x6E, 0x75, 0x17,
	0x9E, 0xBB, 0xB7, 0x6B, 0xCD, 0xEF, 0x7E, 0x64, 0x5C, 0x79, 0xAA, 0xB1, 0xA1, 0x77, 0x6A, 0x0C,
	0x1E, 0x99, 0xDA, 0xA0, 0xA4, 0x3C, 0x1D, 0x79, 0x31, 0xE7, 0x23, 0x46, 0x9F, 0xB3, 0x54, 0xCA,
	0xFE, 0x92, 0x16, 0xA5, 0xEA, 0x71, 0xEA, 0x66, 0xF5, 0x5F, 0xB1, 0x72, 0x0F, 0x96, 0xA8, 0xDF,
	0x00, 0x37, 0x85, 0x47, 0xF8, 0xA0, 0xCD, 0xD9, 0x86, 0x17, 0xB4, 0x72, 0xF3, 0x23, 0x5D, 0x9F,
	0x06, 0x55, 0x3E, 0x29, 0x44, 0xA3, 0x3F, 0xE0, 0x64, 0xD0, 0xDD, 0x28, 0x63, 0x5D, 0x9C, 0xFF,
	0xFF, 0x37, 0x26, 0x68, 0x8D, 0x70, 0xE5, 0xBF, 0xB4, 0xA7, 0x7F, 0xE3, 0xBB, 0xEF, 0x88, 0x6E,
	0x47, 0xA4, 0x97, 0x3E, 0x3F, 0xCB, 0x9A, 0x3F, 0xAC, 0x11, 0x96, 0x7C, 0x4D, 0xD1, 0x60, 0xCB,
	0x5C, 0x26, 0x43, 0x8F, 0x36, 0xD0, 0x99, 0x25, 0x16, 0xFB, 0xE6, 0xAC, 0x34, 0xCE, 0x28, 0xAF,
	0x01, 0xCB, 0x35, 0xB4, 0x1C, 0x65, 0xC1, 0x75, 0xDC, 0x61, 0x86, 0xC1, 0x13, 0x4E, 0xC2, 0xC8,
	0xCA, 0xB1, 0x74, 0x23, 0x0B, 0xA1, 0xD1, 0x50, 0x6D, 0xAD, 0x11, 0x4E, 0x8E, 0xD6, 0x37, 0xD6,
	0x37, 0x65, 0x6B, 0xB3, 0xB5, 0xA3, 0x2B, 0xC2, 0xE1, 0x8D, 0xE1, 0xFA, 0x04, 0xD0, 0xB9, 0x9B,
	0x7E, 0xAE, 0xB4, 0x70, 0x28, 0xE4, 0x45, 0x0D, 0xA8, 0xEE, 0xE0, 0x1E, 0x15, 0x3F, 0x03, 0xFD,
	0x93, 0xDC, 0xCA, 0x12, 0x52, 0xD2, 0x2D, 0x42, 0x34, 0x3C, 0x10, 0x94, 0x68, 0xA0, 0x11, 0x0D,
	0x8F, 0x70, 0xB3, 0xD3, 0xF4, 0x81, 0x5E, 0xFF, 0xA0, 0x83, 0x90, 0x3E, 0xA2, 0x29, 0x15, 0xBE,
	0x19, 0xBF, 0x69, 0x4B, 0xA2, 0x39, 0x57, 0x6A, 0x4E, 0x2D, 0xC6, 0x0B, 0x77, 0xB0, 0x4D, 0xC5,
	0xD7, 0xB2, 0xFF, 0x62, 0xE6, 0x84, 0x49, 0x5E, 0xE7, 0x84, 0xC8, 0xA9, 0x01, 0xF2, 0xA1, 0x83,
	0x9E, 0xAA, 0x4A, 0x5E, 0xD5, 0x4F, 0xF6, 0xC6, 0x48, 0x8C, 0x10, 0x40, 0xDE, 0xD2, 0x78, 0x85,
	0xD7, 0xF1, 0x29, 0x5D, 0x59, 0xC8, 0x7A, 0x35, 0x7A, 0x98, 0xC7, 0x27, 0x54, 0x9F, 0xDF, 0x28,
	0x2F, 0x66, 0xC8, 0x7B, 0xE3, 0x3E, 0xEF, 0x08, 0x48, 0xB0, 0xDE, 0x16, 0x53, 0x8B, 0x23, 0x1F,
	0x47, 0xBE, 0x5F, 0x3F, 0x75, 0x41, 0xF2, 0x3A, 0x10, 0xAD, 0xCF, 0x32, 0xF1, 0x79, 0x1F, 0xF0,
	0xB0, 0x0C, 0x6F, 0x65, 0x5F, 0xEE, 0x47, 0x3E, 0x5B, 0xE1, 0x3D, 0x6E, 0x75, 0x9F, 0x65, 0x16,
	0x71, 0x80, 0xE2, 0x87, 0x7F, 0x8B, 0xD9, 0x1D, 0x27, 0xDD, 0xDD, 0x92, 0x4F, 0xE9, 0x27, 0x25,
	0x9F, 0xA4, 0x40, 0xAC, 0x3E, 0xA7, 0xCA, 0x67, 0xCF, 0x15, 0x46, 0x8D, 0xE4, 0xE8, 0xD9, 0x54,
	0xE1, 0x12, 0xAC, 0x58, 0x6F, 0x3C, 0x7B, 0xB7, 0x7E, 0x34, 0xB9, 0xA9, 0x7E, 0xC3, 0xFB, 0xE1,
	0x44, 0x95, 0x2F, 0xF5, 0x7D, 0x80, 0xBC, 0xF9, 0x9E, 0x48, 0x60, 0x36, 0x18, 0x77, 0x2C, 0xAD,
	0x97, 0x39, 0x43, 0x7E, 0x5C, 0xAC, 0xC5, 0x0A, 0xA2, 0x4F, 0xCA, 0xE8, 0x07, 0x06, 0x63, 0xC9,
	0x98, 0xEA, 0xF8, 0x3C, 0x40, 0xC0, 0x6B, 0xA7, 0x6D, 0x4C, 0x97, 0xD6, 0x0B, 0x5C, 0x85, 0x4C,
	0x2F, 0x39, 0x7D, 0xEB, 0x73, 0x4C, 0x9E, 0x16, 0xB1, 0x88, 0xC6, 0xB0, 0x5C, 0x24, 0x3F, 0x75,
	0x77, 0x4F, 0x35, 0x39, 0xC8, 0x33, 0xAE, 0xE1, 0x6B, 0xC5, 0xFA, 0x0C, 0xAB, 0x15, 0x03, 0x68,
	0x31, 0x88, 0xE4, 0x0F, 0xF9, 0xB9, 0xEF, 0x1B, 0x8C, 0xE9, 0x31, 0x95, 0x03, 0x31, 0x4F, 0xE2,
	0xB3, 0x01, 0x93, 0x77, 0x94, 0x88, 0x2E, 0x79, 0x02, 0xBC, 0x1F, 0x32, 0x04, 0x1B, 0x95, 0xC9,
	0x1B, 0x74, 0x2C, 0xE6, 0x47, 0x2F, 0x44, 0x62, 0xEA, 0xC6, 0x81, 0xEA, 0xD4, 0x22, 0x0F, 0xDE,
	0x7B, 0xC8, 0x13, 0x4F, 0xE6, 0x90, 0xB1, 0x16, 0x0D, 0x79, 0x5D, 0x57, 0x1E, 0xF2, 0x9A, 0x87,
	0xBC, 0x69, 0x55, 0xF9, 0xBF, 0x23, 0x9F, 0x80, 0x2C, 0xF2, 0xA9, 0x0A, 0x6F, 0xE0, 0xF9, 0x0B,
	0x60, 0x41, 0x85, 0xB7, 0x06, 0x6F, 0xBC, 0x21, 0x2A, 0x99, 0xA4, 0x84, 0x51, 0xC2, 0x49, 0xC0,
	0x63, 0xE5, 0x88, 0xB3, 0xF2, 0x0E, 0xA2, 0x07, 0x85, 0xD9, 0x37, 0x14, 0xD3, 0x7F, 0x8E, 0x7F,
	0x89, 0x83, 0x3C, 0x82, 0x7E, 0x72, 0x90, 0xD7, 0x41, 0x27, 0x7E, 0x72, 0x7C, 0x5E, 0x0E, 0xDF,
	0xE7, 0x38, 0x50, 0xCD, 0xFD, 0xE0, 0x60, 0x44, 0xBE, 0x4F, 0x70, 0x81, 0xA7, 0xA9, 0x85, 0x85,
	0xC9, 0x35, 0x1B, 0xD3, 0xEB, 0xC8, 0xBA, 0x77, 0xB0, 0xE4, 0x35, 0xAD, 0xC2, 0x93, 0x3E, 0x0D,
	0x6B, 0x5F, 0x8E, 0x0D, 0x26, 0xA9, 0xF2, 0x03, 0xA7, 0xED, 0x21, 0x02, 0x06, 0x1E, 0x69, 0x92,
	0x3F, 0x7D, 0x8B, 0x12, 0x77, 0x08, 0x2C, 0xCB, 0xB1, 0xAB, 0xBC, 0xA1, 0xBD, 0x2F, 0xE4, 0xA6,
	0x40, 0x45, 0x5A, 0x77, 0x46, 0x13, 0xC6, 0x97, 0xD9, 0x02, 0x18, 0x38, 0x3C, 0xAE, 0x5A, 0xC7,
	0x08, 0x87, 0xB2, 0xA1, 0xDA, 0xC6, 0x6C, 0x38, 0xC9, 0xEA, 0x57, 0x6C, 0x5C, 0x9B, 0x6D, 0xCC,
	0x36, 0x24, 0xB3, 0x06, 0x98, 0xE1, 0x6C, 0x6D, 0xC0, 0x8F, 0x9D, 0x2B, 0x5C, 0x35, 0x92, 0x24,
	0x9C, 0xE4, 0xF5, 0xC9, 0x35, 0x61, 0xE3, 0xDC, 0xDD, 0x50, 0xB2, 0x74, 0x15, 0xAC, 0x0F, 0x20,
	0x51, 0x49, 0x8E, 0xFF, 0x69, 0xEC, 0xE9, 0x92, 0x9F, 0xB7, 0xCD, 0x2F, 0x58, 0xE3, 0xD1, 0xE0,
	0x67, 0xFA, 0x41, 0xC7, 0x1D, 0xEF, 0xE1, 0x32, 0xC5, 0x45, 0xA8, 0xF9, 0x7F, 0xF4, 0xEA, 0xAE,
	0x05, 0x72, 0x4B, 0x7B, 0xC0, 0x07, 0x0B, 0x64, 0x5E, 0x5B, 0xC0, 0xD7, 0x89, 0x85, 0x0D, 0xA5,
	0xEA, 0x7E, 0x6E, 0x08, 0xD1, 0xED, 0xFF, 0x69, 0xBC, 0x5F, 0xE9, 0x49, 0xFB, 0xEE, 0xFF, 0x00,
	0xAA, 0xCF, 0x5C, 0xA0, 0xB7, 0x28, 0x68, 0xF7, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44,
	0xAE, 0x42, 0x60, 0x82
};

/*
新宋体ascii 94个符号位图数据
// 12号像素是6*14宽高
char r[2] = { 0x21, 0x7e };
12：6*14=564
14：7*16=658
16：8*18=752
*/
void nsimsun_ascii(bitmap_ttinfo* obt)
{
	std::string str = R"(!"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)";
	uint32_t* img = obt->_buf;
	if (!img)
	{
		stbimage_load lad;
		if (lad.load_mem((char*)fdata, 2180))
		{
			img = new uint32_t[lad.width * lad.height];
			obt->_buf = img;
			memcpy(img, lad.data, sizeof(uint32_t) * lad.width * lad.height);
			obt->_bimg.data = img;
			obt->_bimg.width = lad.width;
			obt->_bimg.height = lad.height;
			obt->_bimg.type = 0;
			obt->_bimg.valid = 1;
		}
	}

	if (!img)
	{
		obt->_chsize.clear();
		obt->_unicode_rang.clear();
		return;
	}
	// 支持的大小
	obt->_chsize = { {12, 6}, {14, 7}, {16, 8} };
	// 范围
	obt->_unicode_rang = { {0x21, 0x7e}, {0x21, 0x7e}, {0x21, 0x7e} };
}


int font_t::init_sbit()
{
	auto font_i = font;
	const stbtt_fontinfo* font = font_i;
	int i, count, stringOffset;
	uint8_t* fc = font->data;
	uint32_t offset = font->fontstart, table_size = 0, sbit_num_strikes = 0;
	uint32_t ebdt_start = 0, ebdt_size = 0;
	sfnt_header* ebdt_table = 0;
	auto sbit_table = get_tag(font_i, TAG_CBLC);
	sbit_table_type stt = sbit_table_type::TYPE_NONE;
	do
	{
		if (sbit_table)
		{
			stt = sbit_table_type::TYPE_CBLC;
			break;
		}
		sbit_table = get_tag(font_i, TAG_EBLC);
		if (sbit_table)
		{
			stt = sbit_table_type::TYPE_EBLC;
			break;
		}
		sbit_table = get_tag(font_i, TAG_bloc);
		if (sbit_table)
		{
			stt = sbit_table_type::TYPE_EBLC;
			break;
		}
		sbit_table = get_tag(font_i, TAG_sbix);
		if (sbit_table)
		{
			stt = sbit_table_type::TYPE_SBIX;
			break;
		}
		else
		{
			// error
			return 0;
		}
	} while (0);
	table_size = sbit_table->logicalLength;
	switch (stt)
	{
	case sbit_table_type::TYPE_EBLC:
	case sbit_table_type::TYPE_CBLC:
	{
		uint8_t* p = fc + sbit_table->offset;
		uint32_t   count;
		int majorVersion = stb_font::ttUSHORT(p + 0);
		int minorVersion = stb_font::ttUSHORT(p + 2);
		uint32_t num_strikes = stb_font::ttULONG(p + 4);
		if (num_strikes >= 0x10000UL)
		{
			return 0;
		}

		/*
		*  Count the number of strikes available in the table.  We are a bit
		*  paranoid there and don't trust the data.
		*/
		count = num_strikes;
		if (8 + 48UL * count > table_size)
			count = (uint32_t)((table_size - 8) / 48);
		sbit_num_strikes = count;
	}
	break;
	case sbit_table_type::TYPE_SBIX:
	{
		//TODO		解析SBIX
	}
	break;
	default:
		break;
	}
	do
	{
		ebdt_table = get_tag(font_i, TAG_CBDT);
		if (ebdt_table) break;
		ebdt_table = get_tag(font_i, TAG_EBDT);
		if (ebdt_table) break;
		ebdt_table = get_tag(font_i, TAG_bdat);
		if (!ebdt_table) return 0;
	} while (0);

	auto ebsc_table = get_tag(font_i, TAG_EBSC);
	font_t* ttp = (font_t*)font->userdata;
	uint8_t* b = fc + sbit_table->offset;
	count = stb_font::ttULONG(b + 4);
	if (!count)
	{
		return 0;
	}
	bitinfo = new bitmap_ttinfo();
	// 位图表eblc
	//std::vector<BitmapSizeTable> bsts;
	//std::vector<std::vector<IndexSubTableArray>> index_sub_table;
	njson ns = get_bitmap_size_table(b, count, bitinfo->_bsts, bitinfo->_index_sub_table, bitinfo->_msidx_table);
#if 0
	//ndef nnDEBUG
	printf("<%s>包含位图大小\n", _aname.c_str());
	for (auto& it : ns)
		printf("%s\n", it.dump().c_str());
#endif // !nnDEBUG
	// 位图数据表ebdt
	b = fc + ebdt_table->offset;
	glm::ivec2 version = { stb_font::ttUSHORT(b + 0), stb_font::ttUSHORT(b + 2) };
	bitinfo->_sbit_table = sbit_table;
	bitinfo->_ebdt_table = ebdt_table;
	bitinfo->_ebsc_table = ebsc_table;
	if (ns.size()) {
		first_bitmap = true;
	}
	return ns.size();
}


#endif // !_FONT_NO_BITMAP



#ifndef NO_COLOR_FONT

/**************************************************************************
 *
 * `COLR' table specification:
 *
 *   https://www.microsoft.com/typography/otspec/colr.htm
 *
 */



 /* NOTE: These are the table sizes calculated through the specs. */
#define BASE_GLYPH_SIZE            6
#define LAYER_SIZE                 4
#define COLR_HEADER_SIZE          14


struct BaseGlyphRecord
{
	uint16_t  gid;
	uint16_t  first_layer_index;
	uint16_t  num_layers;

};


struct Colr
{
	uint16_t  version;
	uint16_t  num_base_glyphs;
	uint16_t  num_layers;

	uint8_t* base_glyphs;
	uint8_t* layers;

	/* The memory which backs up the `COLR' table. */
	void* table;
	unsigned long  table_size;

};
typedef uint32_t Offset32;
struct colr_v0 {
	uint16_t	version;//	Table version number—set to 0.
	uint16_t	numBaseGlyphRecords;//	Number of BaseGlyph records.
	Offset32	baseGlyphRecordsOffset;//	Offset to baseGlyphRecords array, from beginning of COLR table.
	Offset32	layerRecordsOffset;//	Offset to layerRecords array, from beginning of COLR table.
	uint16_t	numLayerRecords;//	Number of Layer records.
};
struct colr_v1 {
	uint16_t	version;//	Table version number—set to 1.
	uint16_t	numBaseGlyphRecords;//	Number of BaseGlyph records; may be 0 in a version 1 table.
	Offset32	baseGlyphRecordsOffset;//	Offset to baseGlyphRecords array, from beginning of COLR table(may be NULL).
	Offset32	layerRecordsOffset;//	Offset to layerRecords array, from beginning of COLR table(may be NULL).
	uint16_t	numLayerRecords;//	Number of Layer records; may be 0 in a version 1 table.
	Offset32	baseGlyphListOffset;//	Offset to BaseGlyphList table, from beginning of COLR table.
	Offset32	layerListOffset;//	Offset to LayerList table, from beginning of COLR table(may be NULL).
	Offset32	clipListOffset;//	Offset to ClipList table, from beginning of COLR table(may be NULL).
	Offset32	varIndexMapOffset;//	Offset to DeltaSetIndexMap table, from beginning of COLR table(may be NULL).
	Offset32	itemVariationStoreOffset;//	Offset to ItemVariationStore, from beginning of COLR table(may be NULL).
};

int	tt_face_load_colr(font_t* face, uint8_t* b, sfnt_header* sp)
{
	int   error = 0;
	//FT_Memory  memory = face->root.memory;

	uint8_t* table = b;
	uint8_t* p = NULL;

	Colr* colr = NULL;

	uint32_t  base_glyph_offset, layer_offset;
	uint32_t  table_size = sp->logicalLength;
	auto cp = face->colorinfo;

	/* `COLR' always needs `CPAL' */
	//if (!face->cpal)
	//	return FT_THROW(Invalid_File_Format);

	//error = face->goto_table(face, TTAG_COLR, stream, &table_size);
	//if (error)
	//	goto NoColr;
	do {

		if (table_size < COLR_HEADER_SIZE)break;
		//	goto InvalidTable;

		//if (FT_FRAME_EXTRACT(table_size, table))
		//	goto NoColr;

		p = table;
		colr = (Colr*)cp->td;

		if (!(colr))
			break;
		auto cv0 = (colr_v0*)p;
		auto cv1 = (colr_v1*)p;
		colr->version = NEXT_USHORT(p);
		if (colr->version > 1)
		{
			error = -1; break;
		}

		colr->num_base_glyphs = NEXT_USHORT(p);
		base_glyph_offset = NEXT_ULONG(p);

		if (base_glyph_offset >= table_size)
		{
			error = -1; break;
		}
		if (colr->num_base_glyphs * BASE_GLYPH_SIZE >
			table_size - base_glyph_offset)
		{
			error = -1; break;
		}

		layer_offset = NEXT_ULONG(p);
		colr->num_layers = NEXT_USHORT(p);

		if (layer_offset >= table_size)
		{
			error = -1; break;
		}
		if (colr->num_layers * LAYER_SIZE > table_size - layer_offset)
		{
			error = -1; break;
		}

		colr->base_glyphs = (uint8_t*)(table + base_glyph_offset);
		colr->layers = (uint8_t*)(table + layer_offset);
		colr->table = table;
		colr->table_size = table_size;

		cp->colr = colr;


	} while (0);

	return error;
}


void tt_face_free_colr(font_t* p)
{
	//sfnt_header* sp = face->root.stream;
	////FT_Memory  memory = face->root.memory;

	//Colr* colr = (Colr*)face->colr;


	//if (colr)
	//{
	//	FT_FRAME_RELEASE(colr->table);
	//	FT_FREE(colr);
	//}
}


static bool find_base_glyph_record(uint8_t* base_glyph_begin,
	int            num_base_glyph,
	uint32_t           glyph_id,
	BaseGlyphRecord* record)
{
	int  min = 0;
	int  max = num_base_glyph - 1;


	while (min <= max)
	{
		int    mid = min + (max - min) / 2;
		uint8_t* p = base_glyph_begin + mid * BASE_GLYPH_SIZE;

		uint16_t  gid = NEXT_USHORT(p);


		if (gid < glyph_id)
			min = mid + 1;
		else if (gid > glyph_id)
			max = mid - 1;
		else
		{
			record->gid = gid;
			record->first_layer_index = NEXT_USHORT(p);
			record->num_layers = NEXT_USHORT(p);

			return 1;
		}
	}

	return 0;
}


bool tt_face_get_colr_layer(font_t* face,
	uint32_t            base_glyph,
	uint32_t* aglyph_index,
	uint32_t* acolor_index,
	LayerIterator* iterator)
{
	Colr* colr = (Colr*)face->colorinfo->colr;
	BaseGlyphRecord  glyph_record = {};
	auto cp = face->colorinfo;

	if (!colr)
		return 0;

	if (!iterator->p)
	{
		uint32_t  offset;


		/* first call to function */
		iterator->layer = 0;

		if (!find_base_glyph_record(colr->base_glyphs,
			colr->num_base_glyphs,
			base_glyph,
			&glyph_record))
			return 0;

		if (glyph_record.num_layers)
			iterator->num_layers = glyph_record.num_layers;
		else
			return 0;

		offset = LAYER_SIZE * glyph_record.first_layer_index;
		if (offset + LAYER_SIZE * glyph_record.num_layers > colr->table_size)
			return 0;

		iterator->p = colr->layers + offset;
	}

	if (iterator->layer >= iterator->num_layers)
		return 0;

	*aglyph_index = NEXT_USHORT(iterator->p);
	*acolor_index = NEXT_USHORT(iterator->p);

	if (*aglyph_index >= (uint32_t)(face->num_glyphs) ||
		(*acolor_index != 0xFFFF &&
			*acolor_index >= cp->palette_data.num_palette_entries))
		return 0;

	iterator->layer++;

	return 1;
}

#define PALETTE_FOR_LIGHT_BACKGROUND  0x01
#define PALETTE_FOR_DARK_BACKGROUND   0x02
Color_2 get_c2(font_t* face1, uint32_t color_index)
{
	Color_2 c = {};
	auto face = face1->colorinfo;
	if (color_index == 0xFFFF)
	{
		if (face->have_foreground_color)
		{
			c.b = face->foreground_color.blue;
			c.g = face->foreground_color.green;
			c.r = face->foreground_color.red;
			c.alpha = face->foreground_color.alpha;
		}
		else
		{
			if (face->palette_data.palette_flags &&
				(face->palette_data.palette_flags[face->palette_index] &
					PALETTE_FOR_DARK_BACKGROUND))
			{
				/* white opaque */
				c.b = 0xFF;
				c.g = 0xFF;
				c.r = 0xFF;
				c.alpha = 0xFF;
			}
			else
			{
				/* black opaque */
				c.b = 0x00;
				c.g = 0x00;
				c.r = 0x00;
				c.alpha = 0xFF;
			}
		}
	}
	else
	{
		c = face->palette[color_index];
	}
	return c;
}
int tt_face_colr_blend_layer(font_t* face1,
	uint32_t       color_index,
	GlyphSlot* dstSlot,
	GlyphSlot* srcSlot)
{
	int  error = 0;
	auto face = face1->colorinfo;
	uint32_t  x, y;
	uint8_t  b, g, r, alpha;

	uint32_t  size;
	uint8_t* src;
	uint8_t* dst;
#if 0

	if (!dstSlot->bitmap.buffer)
	{
		/* Initialize destination of color bitmap */
		/* with the size of first component.      */
		dstSlot->bitmap_left = srcSlot->bitmap_left;
		dstSlot->bitmap_top = srcSlot->bitmap_top;

		dstSlot->bitmap.width = srcSlot->bitmap.width;
		dstSlot->bitmap.rows = srcSlot->bitmap.rows;
		dstSlot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;
		dstSlot->bitmap.pitch = (int)dstSlot->bitmap.width * 4;
		dstSlot->bitmap.num_grays = 256;

		size = dstSlot->bitmap.rows * (uint32_t)dstSlot->bitmap.pitch;

		error = ft_glyphslot_alloc_bitmap(dstSlot, size);
		if (error)
			return error;

		FT_MEM_ZERO(dstSlot->bitmap.buffer, size);
	}
	else
	{
		/* Resize destination if needed such that new component fits. */
		int  x_min, x_max, y_min, y_max;


		x_min = FT_MIN(dstSlot->bitmap_left, srcSlot->bitmap_left);
		x_max = FT_MAX(dstSlot->bitmap_left + (int)dstSlot->bitmap.width,
			srcSlot->bitmap_left + (int)srcSlot->bitmap.width);

		y_min = FT_MIN(dstSlot->bitmap_top - (int)dstSlot->bitmap.rows,
			srcSlot->bitmap_top - (int)srcSlot->bitmap.rows);
		y_max = FT_MAX(dstSlot->bitmap_top, srcSlot->bitmap_top);

		if (x_min != dstSlot->bitmap_left ||
			x_max != dstSlot->bitmap_left + (int)dstSlot->bitmap.width ||
			y_min != dstSlot->bitmap_top - (int)dstSlot->bitmap.rows ||
			y_max != dstSlot->bitmap_top)
		{
			FT_Memory  memory = face->root.memory;

			uint32_t  width = (uint32_t)(x_max - x_min);
			uint32_t  rows = (uint32_t)(y_max - y_min);
			uint32_t  pitch = width * 4;

			uint8_t* buf = NULL;
			uint8_t* p;
			uint8_t* q;


			size = rows * pitch;
			if (FT_ALLOC(buf, size))
				return error;

			p = dstSlot->bitmap.buffer;
			q = buf +
				(int)pitch * (y_max - dstSlot->bitmap_top) +
				4 * (dstSlot->bitmap_left - x_min);

			for (y = 0; y < dstSlot->bitmap.rows; y++)
			{
				FT_MEM_COPY(q, p, dstSlot->bitmap.width * 4);

				p += dstSlot->bitmap.pitch;
				q += pitch;
			}

			ft_glyphslot_set_bitmap(dstSlot, buf);

			dstSlot->bitmap_top = y_max;
			dstSlot->bitmap_left = x_min;

			dstSlot->bitmap.width = width;
			dstSlot->bitmap.rows = rows;
			dstSlot->bitmap.pitch = (int)pitch;

			dstSlot->internal->flags |= FT_GLYPH_OWN_BITMAP;
			dstSlot->format = FT_GLYPH_FORMAT_BITMAP;
		}
	}

	if (color_index == 0xFFFF)
	{
		if (face->have_foreground_color)
		{
			b = face->foreground_color.blue;
			g = face->foreground_color.green;
			r = face->foreground_color.red;
			alpha = face->foreground_color.alpha;
		}
		else
		{
			if (face->palette_data.palette_flags &&
				(face->palette_data.palette_flags[face->palette_index] &
					FT_PALETTE_FOR_DARK_BACKGROUND))
			{
				/* white opaque */
				b = 0xFF;
				g = 0xFF;
				r = 0xFF;
				alpha = 0xFF;
			}
			else
			{
				/* black opaque */
				b = 0x00;
				g = 0x00;
				r = 0x00;
				alpha = 0xFF;
			}
		}
	}
	else
	{
		b = face->palette[color_index].blue;
		g = face->palette[color_index].green;
		r = face->palette[color_index].red;
		alpha = face->palette[color_index].alpha;
	}

	/* XXX Convert if srcSlot.bitmap is not grey? */
	src = srcSlot->bitmap.buffer;
	dst = dstSlot->bitmap.buffer +
		dstSlot->bitmap.pitch * (dstSlot->bitmap_top - srcSlot->bitmap_top) +
		4 * (srcSlot->bitmap_left - dstSlot->bitmap_left);

	for (y = 0; y < srcSlot->bitmap.rows; y++)
	{
		for (x = 0; x < srcSlot->bitmap.width; x++)
		{
			int  aa = src[x];
			int  fa = alpha * aa / 255;

			int  fb = b * fa / 255;
			int  fg = g * fa / 255;
			int  fr = r * fa / 255;

			int  ba2 = 255 - fa;

			int  bb = dst[4 * x + 0];
			int  bg = dst[4 * x + 1];
			int  br = dst[4 * x + 2];
			int  ba = dst[4 * x + 3];


			dst[4 * x + 0] = (uint8_t)(bb * ba2 / 255 + fb);
			dst[4 * x + 1] = (uint8_t)(bg * ba2 / 255 + fg);
			dst[4 * x + 2] = (uint8_t)(br * ba2 / 255 + fr);
			dst[4 * x + 3] = (uint8_t)(ba * ba2 / 255 + fa);
		}

		src += srcSlot->bitmap.pitch;
		dst += dstSlot->bitmap.pitch;
	}
#endif
	return error;
}



/**************************************************************************
 *
 * `CPAL' table specification:
 *
 *   https://www.microsoft.com/typography/otspec/cpal.htm
 *
 */

 /* NOTE: These are the table sizes calculated through the specs. */
#define CPAL_V0_HEADER_BASE_SIZE  12
#define COLOR_SIZE                 4


  /* all data from `CPAL' not covered in FT_Palette_Data */
struct Cpal
{
	uint16_t  version;        /* Table version number (0 or 1 supported). */
	uint16_t  num_colors;               /* Total number of color records, */
	/* combined for all palettes.     */
	uint8_t* colors;                              /* RGBA array of colors */
	uint8_t* color_indices; /* Index of each palette's first color record */
	/* in the combined color record array.        */

/* The memory which backs up the `CPAL' table. */
	void* table;
	uint32_t  table_size;

};



int tt_face_load_cpal(font_t* face1, uint8_t* b, sfnt_header* sp)
{
	int   error = 0;
	//FT_Memory  memory = face->root.memory;
	auto face = face1->colorinfo;
	uint8_t* table = b;
	uint8_t* p = NULL;

	Cpal* cpal = NULL;

	uint32_t  colors_offset = 0;
	uint32_t  table_size = sp->logicalLength;

#if 1
	//error = face->goto_table(face, TTAG_CPAL, stream, &table_size);
	//if (error)
	//	goto NoCpal;
	do {

		if (table_size < CPAL_V0_HEADER_BASE_SIZE)
			break;

		//if (FT_FRAME_EXTRACT(table_size, table))
		//	goto NoCpal;

		p = table;
		cpal = (Cpal*)&face->td[40];
		if (!cpal)
			break;

		cpal->version = NEXT_USHORT(p);
		if (cpal->version > 1)
		{
			error = -1; break;
		}

		face->palette_data.num_palette_entries = NEXT_USHORT(p);
		face->palette_data.num_palettes = NEXT_USHORT(p);

		cpal->num_colors = NEXT_USHORT(p);
		colors_offset = NEXT_ULONG(p);

		if (CPAL_V0_HEADER_BASE_SIZE +
			face->palette_data.num_palettes * 2U > table_size)
		{
			error = -1; break;
		}

		if (colors_offset >= table_size)
		{
			error = -1; break;
		}
		if (cpal->num_colors * COLOR_SIZE > table_size - colors_offset)
		{
			error = -1; break;
		}

		if (face->palette_data.num_palette_entries > cpal->num_colors)
		{
			error = -1; break;
		}

		cpal->color_indices = p;
		cpal->colors = (uint8_t*)(table + colors_offset);

		if (cpal->version == 1)
		{
			uint32_t    type_offset, label_offset, entry_label_offset;
			uint16_t* array0 = NULL;
			uint16_t* limit;
			uint16_t* q;


			if (CPAL_V0_HEADER_BASE_SIZE +
				face->palette_data.num_palettes * 2U +
				3U * 4 > table_size)
			{
				error = -1; break;
			}

			p += face->palette_data.num_palettes * 2;

			type_offset = NEXT_ULONG(p);
			label_offset = NEXT_ULONG(p);
			entry_label_offset = NEXT_ULONG(p);

			if (type_offset)
			{
				if (type_offset >= table_size)
				{
					error = -1; break;
				}
				if (face->palette_data.num_palettes * 2 >
					table_size - type_offset)
				{
					error = -1; break;
				}
				array0 = new uint16_t[face->palette_data.num_palettes];
				if (!array0)//face1->uac.new_mem(array, face->palette_data.num_palettes))
				{
					error = -2; break;
				}

				p = table + type_offset;
				q = array0;
				limit = q + face->palette_data.num_palettes;

				while (q < limit)
					*q++ = NEXT_USHORT(p);

				face->palette_data.palette_flags = array0;
			}

			if (label_offset)
			{
				if (label_offset >= table_size)
				{
					error = -1; break;
				}
				if (face->palette_data.num_palettes * 2 >
					table_size - label_offset)
				{
					error = -1; break;
				}

				//if (FT_QNEW_ARRAY(array, face->palette_data.num_palettes))

				array0 = new uint16_t[face->palette_data.num_palettes];
				if (!array0)//face1->uac.new_mem(array, face->palette_data.num_palettes))
				{
					error = -2; break;
				}

				p = table + label_offset;
				q = array0;
				limit = q + face->palette_data.num_palettes;

				while (q < limit)
					*q++ = NEXT_USHORT(p);

				face->palette_data.palette_name_ids = array0;
			}

			if (entry_label_offset)
			{
				if (entry_label_offset >= table_size)
				{
					error = -1; break;
				}
				if (face->palette_data.num_palette_entries * 2 >
					table_size - entry_label_offset)
				{
					error = -1; break;
				}

				array0 = new uint16_t[face->palette_data.num_palette_entries];
				if (!array0)//face1->uac.new_mem(array, face->palette_data.num_palette_entries))
				{
					error = -2; break;
				}

				p = table + entry_label_offset;
				q = array0;
				limit = q + face->palette_data.num_palette_entries;

				while (q < limit)
					*q++ = NEXT_USHORT(p);

				face->palette_data.palette_entry_name_ids = array0;
			}
		}

		cpal->table = table;
		cpal->table_size = table_size;

		face->cpal = cpal;

		/* set up default palette */
		face->palette = new Color_2[face->palette_data.num_palette_entries];
		//if (!face1->uac.new_mem(, face->palette_data.num_palette_entries))
		if (!face->palette) {
			error = -2; break;
		}

		if (tt_face_palette_set(face1, 0))
		{
			error = -1; break;
		}
		error = 0;
		break;

	} while (0);
	if (error < 0)
	{
		//InvalidTable:
		//	error = -1;// FT_THROW(Invalid_Table);

		//NoCpal:
			//FT_FRAME_RELEASE(table);
		//face1->uac.free_mem(cpal, 1);

		face->cpal = NULL;

	}
	/* arrays in `face->palette_data' and `face->palette' */
	/* are freed in `sfnt_done_face'                      */
#endif
	return error;
}


void tt_face_free_cpal(font_t* face)
{
	//sfnt_header* sp = face->colorinfo.;
	//FT_Memory  memory = face->root.memory;

	//Cpal* cpal = (Cpal*)face->cpal;


	//if (cpal)
	{
		//	FT_FRAME_RELEASE(cpal->table);
		//	FT_FREE(cpal);
	}
}

int tt_face_palette_set(font_t* face, uint32_t  palette_index)
{
	Cpal* cpal = (Cpal*)face->colorinfo->cpal;
	auto cp = face->colorinfo;
	uint8_t* offset;
	uint8_t* p;

	Color_2* q;
	Color_2* limit;

	uint16_t  color_index;

	if (!cpal || palette_index >= cp->palette_data.num_palettes)
		return -6;// FT_THROW(Invalid_Argument);

	offset = cpal->color_indices + 2 * palette_index;
	color_index = PEEK_USHORT(offset);

	if (color_index + cp->palette_data.num_palette_entries >
		cpal->num_colors)
		return -7;// FT_THROW(Invalid_Table);

	p = cpal->colors + COLOR_SIZE * color_index;
	q = (Color_2*)cp->palette;
	limit = q + cp->palette_data.num_palette_entries;

	while (q < limit)
	{
		q->blue = NEXT_BYTE(p);
		q->green = NEXT_BYTE(p);
		q->red = NEXT_BYTE(p);
		q->alpha = NEXT_BYTE(p);

		q++;
	}

	return 0;
}






#endif // !NO_COLOR_FONT





int font_t::get_glyph_bitmap(int gidx, int height, glm::ivec4* ot, Bitmap_p* out_bitmap, std::vector<char>* out)
{
	Bitmap_p* bitmap = 0;
	int x_pos = 0, y_pos = 0, ret = 0;
	int sidx = bitinfo->get_sidx(height);
	if (sidx < 0)
	{
		return 0;
	}
	int x = 10, y = 10;
	SBitDecoder* decoder = bitinfo->new_SBitDecoder();
	decoder->init(this, sidx);
	bitmap = decoder->bitmap;
	BigGlyphMetrics* metrics = decoder->metrics;

	if (get_index(decoder, gidx, x_pos, y_pos))
	{
		if (out) {
			out->resize((uint64_t)bitmap->rows * bitmap->pitch);
			memcpy(out->data(), bitmap->buffer, out->size());
		}
		if (ot) {
			ot->x = metrics->horiBearingX;
			ot->y = -metrics->horiBearingY;
			ot->z = bitmap->width;
			ot->w = bitmap->rows;
		}
		auto ha = metrics->horiAdvance;
		bitmap->advance = std::max(metrics->horiAdvance, metrics->vertAdvance);
		if (out_bitmap)
		{
			*out_bitmap = *bitmap;
			if (out)
				out_bitmap->buffer = (unsigned char*)out->data();
		}
		ret = 2;
	}
	else
	{
		ret = 0;
	}
	bitinfo->recycle(decoder);
	return ret;
}

void init_bitmap_bitdepth(Bitmap_p* bitmap, int bit_depth)
{
	bitmap->bit_depth = bit_depth;

	switch (bit_depth)
	{
	case 1:
		bitmap->pixel_mode = Pixel_Mode::PX_MONO;
		bitmap->pitch = (int)((bitmap->width + 7) >> 3);
		bitmap->num_grays = 2;
		break;

	case 2:
		bitmap->pixel_mode = Pixel_Mode::PX_GRAY2;
		bitmap->pitch = (int)((bitmap->width + 3) >> 2);
		bitmap->num_grays = 4;
		break;

	case 4:
		bitmap->pixel_mode = Pixel_Mode::PX_GRAY4;
		bitmap->pitch = (int)((bitmap->width + 1) >> 1);
		bitmap->num_grays = 16;
		break;

	case 8:
		bitmap->pixel_mode = Pixel_Mode::PX_GRAY;
		bitmap->pitch = (int)(bitmap->width);
		bitmap->num_grays = 256;
		break;

	case 32:
		bitmap->pixel_mode = Pixel_Mode::PX_BGRA;
		bitmap->pitch = (int)(bitmap->width * 4);
		bitmap->num_grays = 256;
		break;

	default:
		return;
	}
}
int font_t::get_custom_decoder_bitmap(uint32_t unicode_codepoint, int height, glm::ivec4* ot, Bitmap_p* out_bitmap, std::vector<char>* out)
{
	auto& bch = bitinfo->_chsize;
	auto& bur = bitinfo->_unicode_rang;
	auto img = (image_ptr_t*)&bitinfo->_bimg;
	int ret = 0;
	if (img && bch.size())
	{
		// 910=12，11=14，12/13=16f
		glm::ivec2 uc = {}, bc = {}; size_t idx = 0;
		int inc = 2;
		if (height & 1)
		{
			height--;
			inc--;
		}
		for (size_t i = 0; i < bch.size(); i++)
		{
			auto& it = bch[i];
			if (it.x == height)
			{
				uc = bur[i];
				bc = it;
				break;
			}
			idx += it.x;
		}
		if (!uc.x || !uc.y || !unicode_codepoint)
		{
			return ret;
		}
		if (unicode_codepoint >= uc.x && unicode_codepoint <= uc.y)
		{
			int px = (unicode_codepoint - uc.x) * bc.y;
			int py = idx;
			out_bitmap->rows = bc.x;
			out_bitmap->width = bc.y;
			init_bitmap_bitdepth(out_bitmap, 32);
			// RGBA
			out_bitmap->pitch = bc.y * 4;
			out_bitmap->advance = bc.y;
			if (ot) {
				ot->x = 0;
				ot->y = -bc.x + inc;
				ot->z = bc.y;
				ot->w = bc.x;
			}

			size_t size = out_bitmap->rows * (uint64_t)out_bitmap->pitch;
			if (out) {
				if (size > out->size())
				{
					out->resize(size);
				}
				out_bitmap->buffer = (unsigned char*)out->data();
				std::vector<uint32_t> tem;
				tem.reserve(bc.y * bc.x);
				size_t x1 = std::min(img->width, px + bc.y);
				size_t y1 = std::min(img->height, py + bc.x);
				for (size_t y = py; y < y1; y++)
				{
					for (size_t x = px; x < x1; x++)
					{
						auto c = img->data[y * img->width + x];
						if (c == 0xff000000)
						{
							c = 0;
						}
						tem.push_back(c);
					}
				}
				memcpy(out_bitmap->buffer, tem.data(), size);
			}
			ret = 1;
		}
	}
	return ret;
}


bitmap_cache_cx::bitmap_cache_cx()
{
	// 填充20x20白色
	auto pt = get_last_packer(false);
	if (!pt)return;
	glm::ivec2 pos = {};
	auto ret = pt->push_rect({ 20, 20 }, &pos);
	auto ptr = pt->get();
	auto px = ((uint32_t*)ptr->data) + pos.x;
	px += pos.y * width;
	for (size_t i = 0; i < 20; i++)
	{
		memset(px, -1, 20 * sizeof(int));
		px += width;
	}
}

bitmap_cache_cx::~bitmap_cache_cx()
{
}

void bitmap_cache_cx::clear()
{
	for (auto it : _packer)
	{
		if (it)delete it;
	}
	_packer.clear();
	_data.clear();
}
stb_packer* bitmap_cache_cx::get_last_packer(bool isnew)
{
	if (_packer.empty() || isnew)
	{
		auto p = new stb_packer();
		if (!p)return 0;
		_packer.push_back(p);
		p->init_target(width, width);
		_data.push_back(p->get());
	}
	return *_packer.rbegin();
}
image_ptr_t* bitmap_cache_cx::push_cache_bitmap(Bitmap_p* bitmap, glm::ivec2* pos, int linegap, uint32_t col)
{
	int width = bitmap->width + linegap, height = bitmap->rows + linegap;
	glm::ivec4 rc4 = { 0, 0, bitmap->width, bitmap->rows };

	auto pt = get_last_packer(false);
	if (!pt)return 0;
	auto ret = pt->push_rect({ width, height }, pos);
	if (!ret)
	{
		pt = get_last_packer(true);
		ret = pt->push_rect({ width, height }, pos);
	}
	if (ret)
	{
		rc4.x = pos->x;
		rc4.y = pos->y;
		image_ptr_t src = {}, dst = {};
		auto ptr = pt->get();
		dst.data = (uint32_t*)ptr->data;
		dst.width = ptr->width;
		dst.height = ptr->height;

		src.data = (uint32_t*)bitmap->buffer;
		src.stride = bitmap->pitch;
		src.width = bitmap->width;
		src.height = bitmap->rows;
		src.comp = 1;
		switch (bitmap->bit_depth)
		{
		case 1:
		{
			bit_copy2rgba(&dst, &src, { rc4.x,rc4.y }, { 0,0,rc4.z,rc4.w }, col);
		}
		break;
		case 8:
		{
			gray_copy2rgba(&dst, &src, { rc4.x,rc4.y }, { 0,0,rc4.z,rc4.w }, col, true);
		}
		break;
		case 32:
		{
			src.comp = 4;
			rgba_copy2rgba(&dst, &src, { rc4.x,rc4.y }, { 0,0,rc4.z,rc4.w }, true);
		}
		break;
		default:
			break;
		}
		ptr->valid = 1; // 更新缓存标志
	}
	return pt->get();
}
image_ptr_t* bitmap_cache_cx::push_cache_bitmap(image_ptr_t* bitmap, glm::ivec2* pos, int linegap, uint32_t col)
{
	int width = bitmap->width + linegap, height = bitmap->height + linegap;
	glm::ivec4 rc4 = { 0, 0, bitmap->width, bitmap->height };

	auto pt = get_last_packer(false);
	if (!pt)return 0;
	auto ret = pt->push_rect({ width, height }, pos);
	if (!ret)
	{
		pt = get_last_packer(true);
		ret = pt->push_rect({ width, height }, pos);
	}
	if (ret)
	{
		rc4.x = pos->x;
		rc4.y = pos->y;
		image_ptr_t src = *bitmap, dst = {};
		auto ptr = pt->get();
		dst.data = (uint32_t*)ptr->data;
		dst.width = ptr->width;
		dst.height = ptr->height;

		switch (bitmap->comp)
		{
		case 0:
		{
			src.comp = 1;
			bit_copy2rgba(&dst, &src, { rc4.x,rc4.y }, { 0,0,rc4.z,rc4.w }, col);
		}
		break;
		case 1:
		{
			src.comp = 1;
			gray_copy2rgba(&dst, &src, { rc4.x,rc4.y }, { 0,0,rc4.z,rc4.w }, col, true);
		}
		break;
		case 4:
		{
			rgba_copy2rgba(&dst, &src, { rc4.x,rc4.y }, { 0,0,rc4.z,rc4.w }, true);
		}
		break;
		default:
			break;
		}
		ptr->valid = 1; // 更新缓存标志
	}
	return pt->get();
}
image_ptr_t* bitmap_cache_cx::push_cache_bitmap_old(Bitmap_p* bitmap, glm::ivec2* pos, uint32_t col, image_ptr_t* oldimg, int linegap)
{
	int width = bitmap->width + linegap, height = bitmap->rows + linegap;
	glm::ivec4 rc4 = { 0, 0, bitmap->width, bitmap->rows };

	if (!oldimg)
	{
		return push_cache_bitmap(bitmap, pos, linegap, col);
	}
	else {
		rc4.x = pos->x;
		rc4.y = pos->y;

		image_ptr_t src = {}, dst = {};
		dst.data = (uint32_t*)oldimg->data;
		dst.width = oldimg->width;
		dst.height = oldimg->height;

		src.data = (uint32_t*)bitmap->buffer;
		src.stride = bitmap->pitch;
		src.width = bitmap->width;
		src.height = bitmap->rows;
		src.comp = 1;
		gray_copy2rgba(&dst, &src, { rc4.x - linegap,rc4.y }, { 0,0,rc4.z,rc4.w }, col, true);
		oldimg->valid = 1;
	}
	return oldimg;
}


//image_ptr_t to_imgptr(image_ptr_t* p)
//{
//	image_ptr_t r = {};
//	r.width = p->width;
//	r.height = p->height;
//	r.data = p->data;
//	r.type = p->format;
//	r.comp = 4;
//	return r;
//}




std::map<int, std::vector<info_one>> font_t::get_detail()
{
	std::map<int, std::vector<info_one>> detail;
	stb_font ft;
	ft.get_font_name(font, &detail);
	return detail;
}


class fd_info0
{
public:
	hz::mfile_t* _fp = nullptr;
	char* _data = nullptr;
	int64_t _size = 0;
	std::set<std::string> vname;
public:
	fd_info0()
	{
	}
	fd_info0(hz::mfile_t* p)
	{
		init(p);
	}

	~fd_info0()
	{
		free_mv();
	}
	void free_mv()
	{
		auto dp = _data;
		if (_fp)
		{
			if (dp == _fp->data())dp = 0;
			delete (_fp); _fp = 0;
		}
		if (dp)
		{
			delete[]dp; dp = 0;
		}
	}
	void init(hz::mfile_t* p)
	{
		_fp = p;
		if (_fp)
		{
			_data = (char*)_fp->data();
			_size = _fp->size();
		}
	}
	char* data()
	{
		return _data;
	}
	int64_t size()
	{
		return _size;
	}
private:

};
font_imp::font_imp()
{
}

font_imp::~font_imp()
{
#if 0
	if (prun)
	{
		delete prun; prun = 0;
	}
#endif
	for (auto it : fd_data)
	{
		delete it;
	}
	for (auto it : fonts)
	{
		delete it;
	}
	for (auto it : fd_data_m)
	{
		delete[] it.data;
	}
}
size_t font_imp::size()
{
	return fonts.size();
}
std::vector<font_t*> font_imp::add_font_file(const std::string& fn, std::vector<std::string>* pname)
{
	std::vector<font_t*> ret;
	auto mv = new hz::mfile_t();
	auto md = mv->open_d(fn, true);
	if (!md)
	{
		delete mv;
		return ret;
	}
	if (mv && mv->get_file_size())
	{
		int rc = 0;
		auto fdi = new fd_info0(mv);
		ret = add_font_mem(fdi->data(), fdi->size(), false, pname, &rc);
		if (rc > 0)
		{
			fd_data.emplace_back(fdi);
			for (auto it : ret)
			{
				fdi->vname.insert(it->_name);
			}
		}
		else {
			delete fdi;
		}
	}
	return ret;
}

std::vector<font_t*> font_imp::add_font_mem(const char* data, size_t len, bool iscp, std::vector<std::string>* pname, int* rc)
{
	std::vector<font_t*> fp;
	mem_ft mft = {};
	if (iscp)
	{
		char* cpd = new char[len];
		if (!cpd)return fp;
		memcpy(cpd, data, len);
		mft.data = cpd;
		data = cpd;
	}
	stb_font ft;
	int nums = ft.get_numbers(data);
	int hr = 0;
	std::vector<std::string> tpn;
	std::string sv;
	if (!pname)
		pname = &tpn;
	std::set<font_t*> fs;
	for (size_t i = 0; i < nums; i++)
	{
		font_t* font = new font_t();
		hr = ft.loadFont(font->font, data, i, 0);
		if (hr)
		{
			font->num_glyphs = font->font->numGlyphs;
			int ascent = 0, descent = 0, lineGap = 0;
			ft.getFontVMetrics(font->font, &ascent, &descent, &lineGap);
			auto fh = ascent - descent;
			std::vector<meta_tag>	_meta;
			std::map<int, std::vector<info_one>> detail;
			//auto& detail = font->detail;
			ft.get_font_name(font->font, &detail);
			//font->fh = fh;
			font->dataSize = len;
			font->_index = i;
			font->ascender = (float)ascent;
			font->descender = (float)descent;
			font->lineh = (float)(fh + lineGap);
			font->_name = get_info_str(2052, 1, detail);
			font->fullname = get_info_str(2052, 4, detail);
			font->_namew = md::u8_u16(font->_name);
			auto& mt = mk[font->_name];
			if (mt)
			{
				fs.insert(font);
				fp.push_back(mt);
				continue;
			}
			mft.vname.insert(font->_name);
			if (rc)
				*rc += 1;
			mt = font;
			fp.push_back(font);

			fonts.push_back(font);
			if (pname)
			{
				pname->push_back(font->_name);
			}
			auto cn_name = get_info_str(1033, 1, detail);
			//font->_aname = u8_gbk(font->_name);
			auto a_style = get_info_str(2052, 2, detail);
			auto a_style1 = get_info_str(1033, 2, detail);
			auto str6 = get_info_str(2052, 6, detail);
			auto str61 = get_info_str(1033, 6, detail);
			font->_style = a_style1;
			ft.get_meta_string(font->font, _meta);
			for (auto& it : _meta)
			{
				if (it.tag == "slng")
				{
					font->_slng = it.v;
				}
			}
			font->init_post_table();
			font->init_color();
#ifndef _FONT_NO_BITMAP
			font->init_sbit();
			if (cn_name == "NSimSun")
			{
				nsimsun_ascii(font->bitinfo);
			}
#endif // !_FONT_NO_BITMAP
		}
		else
		{
			fs.insert(font);
		}
	}
	for (auto it : fs)
	{
		if (it)
			delete it;
	}
	if (mft.vname.size())
	{
		fd_data_m.push_back(mft);
	}
	return fp;
}

int font_imp::get_font_names(std::vector<std::string>* pname)
{
	if (pname)
	{
		pname->clear();
		pname->reserve(fonts.size());
		for (auto it : fonts)
		{
			pname->push_back(it->_name);
		}
	}
	return fonts.size();
}
const char* font_imp::get_font_names(const char* split)
{
	if (fonts.empty())return 0;
	if (!split || !*split)
	{
		split = ",";
	}
	std::string str;
	for (auto& it : fonts)
	{
		str += it->_name + split;
	}
	addstr = str.c_str();
	return addstr.c_str();
}

font_t* font_imp::get_font(size_t idx)
{
	font_t* p = 0;
	if (fonts.empty())return p;
	if (idx < fonts.size())
	{
		p = fonts[idx];
	}
	else {
		p = *fonts.rbegin();
	}
	return p;
}
void font_imp::free_ft(const std::string& name)
{
	font_t* p = 0;
	for (auto it : fonts) {
		if (it->_name == name)
		{
			p = it; free_ftp(p);
			break;
		}
	}
}
void font_imp::free_ftp(font_t* p)
{
	for (auto pt = fd_data.begin(); pt != fd_data.end(); pt++) {
		auto mt = *pt;
		auto br = mt->vname.find(p->_name);
		if (br != mt->vname.end()) {
			mt->vname.erase(p->_name);
			if (mt->vname.empty())
			{
				auto& v = fd_data;
				delete mt;
				v.erase(pt);
			}
			break;
		}
	}

	for (auto pt = fd_data_m.begin(); pt != fd_data_m.end(); pt++) {
		auto& mt = pt;
		auto br = mt->vname.find(p->_name);
		if (br != mt->vname.end()) {
			mt->vname.erase(p->_name);
			if (mt->vname.empty())
			{
				auto& v = fd_data_m;
				delete[]mt->data;
				v.erase(pt);
			}
			break;
		}
	}
	auto& v = fonts;
	delete p;
	v.erase(std::remove(v.begin(), v.end(), p), v.end());
}
#endif


font_rctx::font_rctx()
{
	fyv = get_allfont();
	if (fyv.size())
	{
		fyvs.resize(fyv.size());
		size_t i = 0;
		for (auto& [k, v] : fyv) {
			fyvs[i] = &v; i++;
		}
	}

	imp = new font_imp();
	//PangoFontMap* fontMap = get_fmap;
	//pcontext = pango_font_map_create_context(fontMap);
	//layout = pango_layout_new(pcontext);
	//gclt.insert(layout); 
}

font_rctx::~font_rctx()
{
	if (imp)delete imp;
	imp = 0;
	//for (auto& [k, v] : fyv) {
	//	for (auto it : v.vptr)
	//	{
	//		auto p = (font_t*)it;
	//		if (p)
	//		{
	//			//delete p;

	//		}
	//	}
	//}
	//for (auto it : gclt)
	//{
	//	if (it)
	//	{
	//		g_object_unref(it); it = 0;
	//	}
	//}
	//if (pcontext)
	//{
	//	g_object_unref(pcontext); pcontext = 0;
	//}
	fyv.clear();
}

int font_rctx::get_count()
{
	return fyv.size();
}

int font_rctx::get_count_style(int idx)
{
	int r = 0;
	if (idx >= 0 && idx < fyvs.size())
	{
		r = fyvs[idx]->style.size();
	}
	return r;
	//for (auto& [k, v] : fyv) {
	//	if (idx == 0)
	//	{
	//		r = v.style.size();
	//		break;
	//	}
	//	idx--;
	//}
	//return r;
}

const char* font_rctx::get_family(int idx)
{
	std::string* r = 0;
	if (idx >= 0 && idx < fyvs.size())
	{
		return fyvs[idx]->family.c_str();
	}
	return "";
	//for (auto& [k, v] : fyv) {
	//	if (idx == 0)
	//	{
	//		r = &v.family;
	//		break;
	//	}
	//	idx--;
	//}
	//return r ? r->c_str() : nullptr;
}

const char* font_rctx::get_family_en(const char* family)
{
	const std::string* r = 0;
	std::string ret;
	if (!r)
	{
		std::set<std::string> nst;
		std::string n;
		auto t = family;
		for (size_t i = 0; *t; i++)
		{
			if (*t == ',')
			{
				nst.insert(n); n.clear();
			}
			else {
				n.push_back(*t);
			}
			t++;
		}
		if (n.size())
		{
			nst.insert(n);
		}
		for (auto& it : nst) {
			const std::string* r = 0;
			for (auto& [k, v] : fyv) {
				auto ats = v.alias.find(it);
				if (k.find(it) != std::string::npos || v.fullname == it || ats != v.alias.end())
				{
					r = &k;
					break;
				}
			}
			ret += (r) ? r->c_str() : it;
			ret.push_back(',');
		}
		if (ret.size() > 1)
		{
			ret.pop_back();
		}
		temfamily = ret;
		r = &temfamily;
	}
	return r ? r->c_str() : nullptr;
}

const char* font_rctx::get_family_cn(int idx)
{
	if (idx >= 0 && idx < fyvs.size())
	{
		auto v = fyvs[idx];
		auto r = v->family.c_str();
		if (v->alias.size())
			r = v->alias.rbegin()->c_str();
		return r;
	}
	return "";
}
const char* font_rctx::get_family_alias(int idx)
{
	static std::string r;
	r.clear();
	if (idx >= 0 && idx < fyvs.size())
	{
		auto v = fyvs[idx];
		for (int i = 0; i < v->alias.size(); i++)
		{
			r += v->alias.rbegin()->c_str(); r.push_back(';');
		}
	}
	return r.c_str();
}

const char* font_rctx::get_family_full(int idx)
{
	std::string* r = 0;
	if (idx >= 0 && idx < fyvs.size())
	{
		return fyvs[idx]->fullname.c_str();
	}
	return "";
	//for (auto& [k, v] : fyv) {
	//	if (idx == 0)
	//	{
	//		r = &v.fullname;
	//		break;
	//	}
	//	idx--;
	//}
	//return r ? r->c_str() : nullptr;
}

const char* font_rctx::get_family_style(int idx, int stidx)
{
	std::string* r = 0;
	size_t st = stidx;
	if (idx >= 0 && idx < fyvs.size())
	{
		auto& v = *fyvs[idx];
		if (v.style.size())
		{
			if (st > v.style.size())
				st = 0;
			return v.style[st].c_str();
		}
	}
	return "";
	//for (auto& [k, v] : fyv) {
	//	if (idx == 0)
	//	{
	//		if (v.style.size())
	//		{
	//			if (st > v.style.size())
	//				st = 0;
	//			r = &v.style[st];
	//		}
	//		break;
	//	}
	//	idx--;
	//}
	//return r ? r->c_str() : nullptr;
}
font_t* font_rctx::get_mk(fontns& v, size_t st)
{
	if (v.vptr.empty())
	{
		v.vptr.resize(v.style.size());
	}
	if (v.style.empty())
	{
		return 0;
	}
	auto r = (font_t*)v.vptr[st];
	if (!r)
	{
		auto vn = v.family;
		auto stn = v.style[st];
		auto pv = imp->add_font_file(v.fpath[st], 0);
		for (auto it : pv)
		{
			if ((vn == it->_name || v.fullname == it->fullname) && it->_style.find(stn) != std::string::npos)
			{
				r = it;
				r->ctx = &bcc;
				v.vptr[st] = it;
				current = it;
			}
		}
		if (!r && pv.size() && pv[0]->_style == "Regular" && stn == "Normal") {
			r = pv[0];
			r->ctx = &bcc;
			v.vptr[st] = r;
			current = r;
		}
	}
	return r;
}
font_t* font_rctx::get_font(int idx, int styleidx)
{
	font_t* r = 0;
	size_t st = styleidx;
	if (idx >= 0 && idx < fyvs.size())
	{
		auto& v = *fyvs[idx];
		if (v.style.size())
		{
			if (st > v.style.size())
				st = 0;
		}
		r = get_mk(v, st);
	}
	return r;
	//for (auto& [k, v] : fyv) {
	//	if (idx == 0)
	//	{
	//		if (v.style.size())
	//		{
	//			if (st > v.style.size())
	//				st = 0;
	//		}
	//		r = get_mk(v, st);
	//		break;
	//	}
	//	idx--;
	//}
	//return r;
}

font_t* font_rctx::get_font(const char* family, const char* style)
{
	font_t* r = 0;
	size_t st = 0;
	{
		auto ft = fyv.find(family);
		if (ft != fyv.end())
		{
			do {
				r = get_mk(ft->second, st);
				if (!style || (!*style))break;
				for (size_t i = 1; i < ft->second.style.size(); i++)
				{
					if (style == ft->second.style[i])
					{
						r = get_mk(ft->second, i);
						break;
					}
				}
			} while (0);
		}
	}
	if (!r)
	{
		for (auto& [k, v] : fyv) {
			auto ats = v.alias.find(family);
			if (k.find(family) != std::string::npos || v.fullname == family || ats != v.alias.end())
			{
				r = get_mk(v, st);
				if (!style || (!*style))break;
				for (size_t i = 1; i < v.style.size(); i++)
				{
					if (style == v.style[i])
					{
						r = get_mk(v, i);
						break;
					}
				}
				break;
			}
		}
	}
	return r;
}

font_t* font_rctx::get_font_cur()
{
	return current;
}
font_t* font_rctx::get_mfont(const std::string& name) {
	auto it = fzv.find(name);
	return it != fzv.end() ? it->second : nullptr;
}
std::vector<font_t*> font_rctx::add2file(const std::string& fn, std::vector<std::string>* pname)
{
	auto r = imp->add_font_file(fn, pname);
	for (auto it : r)
	{
		it->ctx = &bcc;
		auto& oldp = fzv[it->_name];
		if (oldp)
		{
			imp->free_ft(oldp->_name);
		}
		oldp = it;
	}
	return r;
}
std::vector<font_t*> font_rctx::add2mem(const char* data, size_t len, bool iscp, std::vector<std::string>* pname)
{
	auto r = imp->add_font_mem(data, len, true, pname, 0);
	for (auto it : r)
	{
		it->ctx = &bcc;
		auto& oldp = fzv[it->_name];
		if (oldp)
		{
			imp->free_ft(oldp->_name);
		}
		oldp = it;
	}
	return r;
}
void font_rctx::free_font(const std::string& name)
{
	for (auto& [k, v] : fyv) {
		auto ats = v.alias.find(name);
		if (k == name || v.fullname == name || ats != v.alias.end())
		{
			v.vptr;
			for (auto it : v.vptr) {
				imp->free_ftp((font_t*)it);
			}
			v.vptr.clear();
			auto& v1 = fyvs;
			for (auto it = v1.begin(); it != v1.end(); it++)
			{
				if (*it == &v)
				{
					v1.erase(it); break;
				}
			}
			break;
		}
	}
}


font_rctx* new_fonts_ctx()
{
	return new font_rctx();
}
void free_fonts_ctx(font_rctx* p)
{
	if (p)delete p;
}


//void font_rctx::set_family_size(const std::string& fam, int fs)
//{
//	auto en = get_family_en(fam.c_str());
//	if (en)
//	{
//		family = en;
//	}
//	else {
//		family = fam;
//	}
//	fontsize = fs > 5 ? fs : 12;
//	if (!layout)
//	{
//		layout = pango_layout_new(pcontext);
//		gclt.insert(layout);
//	}
//	pango_layout_set_wrap(layout, PANGO_WRAP_CHAR);
//	pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
//
//	PangoFontDescription* desc = pango_font_description_new();
//	pango_font_description_set_family(desc, family.c_str());
//	pango_font_description_set_size(desc, fontsize * PANGO_SCALE);
//	pango_layout_set_font_description(layout, desc);
//	pango_font_description_free(desc);
//
//}
//text_layout_t font_rctx::get_text_layout(const std::string& str, text_layout_t* lt)
//{
//	text_layout_t ret = {};
//	auto lay = lt && lt->layout ? lt->layout : pango_layout_copy(layout);
//	gclt.insert(lay);
//	pango_layout_set_text(lay, str.c_str(), str.size());
//	int h = 0;
//	auto line = pango_layout_get_line(lay, 0);
//	pango_layout_line_get_height(line, &h);
//	ret.lineheight = h / PANGO_SCALE;
//	pango_layout_get_pixel_size(lay, &ret.rc.x, &ret.rc.y);
//	ret.baseline = pango_layout_get_baseline(lay) / PANGO_SCALE;
//	ret.ctx = this;
//	ret.layout = lay;
//	if (lt)*lt = ret;
//	return ret;
//}
//void font_rctx::draw_text(cairo_t* cr, text_layout_t* lt)
//{
//	if (!cr || !lt || !lt->layout || !lt->text_color)return;
//	cairo_save(cr);
//	pango_cairo_update_layout(cr, lt->layout);
//	set_color(cr, lt->text_color);
//	cairo_translate(cr, lt->pos.x, lt->pos.y);
//	auto fcp = cairo_get_scaled_font(cr);
//	pango_cairo_show_layout(cr, lt->layout);
//#if 0
//	//cairo_scaled_font_t* csf = pango_cairo_font_get_scaled_font(font);
//	std::vector<PangoGlyphString*> vpgs;
//	std::vector<PangoLayoutLine*> lvs;
//	PangoLayoutIter* it = pango_layout_get_iter(lt->layout);
//	for (;;) {
//		auto lv = pango_layout_iter_get_line(it);
//		if (!lv)break;
//		for (;;) {
//			auto r = pango_layout_iter_get_run(it);
//			if (r)
//			{
//				r->item->analysis.font;
//				vpgs.push_back(r->glyphs);
//			}
//			if (!pango_layout_iter_next_run(it))break;
//		}
//		lvs.push_back(lv);
//		if (!pango_layout_iter_next_line(it))break;
//	}
//	pango_layout_iter_free(it);
//
//	//pango_cairo_show_glyph_item(cr, "", r);
//	//pango_cairo_layout_path(cr, lt->layout);
//#endif
//	cairo_restore(cr);
//	if (lt->once)
//	{
//		if (lt->layout)
//		{
//			gclt.erase(lt->layout);
//			g_object_unref(lt->layout); lt->layout = 0;
//		}
//	}
//}
//void font_rctx::free_textlayout(text_layout_t* lt)
//{
//	if (lt)
//	{
//		if (lt->layout)
//		{
//			gclt.erase(lt->layout);
//			g_object_unref(lt->layout); lt->layout = 0;
//		}
//	}
//}

#if 0
void layout_text()
{

	auto cr = cairo_create(sur);
	size_t length = dtimg.size();
	auto dt = dtimg.data();
	for (size_t i = 0; i < length; i++)
	{
		*dt = back_color; dt++;
	}


	cairo_save(cr);
#if 0
	int gradient_size = 6;

	paint_shadow(cr, 0, gradient_size, size.x, gradient_size, _shadow, 0);// 垂直方向 
	cairo_save(cr);
	cairo_translate(cr, 0, size.y - gradient_size);
	paint_shadow(cr, 0, gradient_size, size.x, gradient_size, _shadow, 1);// 垂直方向,下
	cairo_restore(cr);
	paint_shadow(cr, gradient_size, 0, gradient_size, size.y, _shadow, 0);// 水平方向 
	cairo_save(cr);
	cairo_translate(cr, size.x - gradient_size, 0);
	paint_shadow(cr, gradient_size, 0, gradient_size, size.y, _shadow, 1);// 水平方向，右
	cairo_restore(cr);
#endif

	cairo_translate(cr, -scroll_pos.x + align_pos.x, -scroll_pos.y + align_pos.y);

	auto v = get_bounds();
	if (v.x != v.y && rangerc.size()) {
		set_color(cr, select_color);
		for (auto& it : rangerc)
		{
			cairo_rectangle(cr, it.x, it.y, it.z, it.w);
			cairo_fill(cr);
		}
	}
	set_color(cr, text_color);
	pango_cairo_show_layout(cr, layout);
	cairo_restore(cr);
	cairo_destroy(cr);

}

#endif


internal_text_cx::internal_text_cx()
{
}

internal_text_cx::~internal_text_cx()
{
}



#endif
// !NO_FONT_CX

#endif
// !font

#ifndef NO_FLEX_CX 

flex_item::flex_item()
{
}

flex_item::~flex_item()
{
	if (children)
		delete children;
	children = 0;
}



void flex_item::update_should_order_children()
{
	if (order != 0 && parent != NULL) {
		parent->should_order_children = true;
	}
}


void flex_item::set_base(float* size, float* offset4, float* padding4, float* margin4, float* gsb, int order1, uint8_t* align_pdw7)
{
	if (size) {
		width = size[0];
		height = size[1];
	}
	if (offset4) {
		left = offset4[0];
		right = offset4[1];
		top = offset4[2];
		bottom = offset4[3];
	}
	if (padding4)
	{
		padding_left = padding4[0];
		padding_right = padding4[1];
		padding_top = padding4[2];
		padding_bottom = padding4[3];
	}
	if (margin4)
	{
		margin_left = margin4[0];
		margin_right = margin4[1];
		margin_top = margin4[2];
		margin_bottom = margin4[3];
	}
	auto p7 = align_pdw7;
	if (p7)
	{
		justify_content = (flex_align)p7[0];
		align_content = (flex_align)p7[1];
		align_items = (flex_align)p7[2];
		align_self = (flex_align)p7[3];

		position = (flex_position)p7[4];
		direction = (flex_direction)p7[5];
		wrap = (flex_wrap)p7[6];
		//bool should_order_children = false;
	}
	if (gsb) {
		grow = gsb[0];
		shrink = gsb[1];
		basis = gsb[2];
	}
	order = order1;
}

void flex_item::copy_a(flex_item* p)
{
	auto d0 = (char*)&left;
	auto d1 = (char*)&managed_ptr;
	auto d2 = (char*)&p->left;
	memcpy(d0, d2, d1 - d0);
}

flex_item* flex_item::init()
{
	flex_item* item = this;
	assert(item != NULL);
	*item = {};
	item->parent = NULL;
	if (item->children)
		item->children->clear();
	item->should_order_children = false;
	return item;
}


void flex_item::item_add(flex_item* child)
{
	flex_item* item = this;
	if (!item->children) {
		item->children = new std::vector<flex_item*>();
	}
	item->children->push_back(child);
	child->parent = item;
	child->update_should_order_children();
}


void flex_item::item_insert(uint32_t index, flex_item* child)
{
	flex_item* item = this;
	if (!item->children) {
		item->children = new std::vector<flex_item*>();
	}
	assert(index <= item->children->size());
	item->children->insert(item->children->begin() + index, child);
	child->parent = item;
	child->update_should_order_children();
}


flex_item* flex_item::item_delete(uint32_t index)
{
	flex_item* item = this;
	if (!children || children->empty())return nullptr;
	assert(index < item->children->size());
	assert(item->children->size() > 0);
	auto& v = *children;
	flex_item* child = v[index];
	children->erase(children->begin() + index);
	return child;
}

flex_item* flex_item::detach(flex_item* child)
{
	flex_item* item = this;
	if (!children || !child || !(item->children->size() > 0))return child;
	auto& v = *children;
	v.erase(std::remove(v.begin(), v.end(), child), v.end());
	child->parent = NULL;
	return child;
}

void flex_item::clear()
{
	if (children && children->size()) {
		children->clear();
	}
}


flex_item* flex_item_root(flex_item* item)
{
	while (item->parent != NULL) {
		item = item->parent;
	}
	return item;
}
//
//#define FRAME_GETTER(name, index) \
//     float flex_item_get_frame_##name(flex_item *item) { \
//        return item->frame[index]; \
//    }
//
//FRAME_GETTER(x, 0)
//FRAME_GETTER(y, 1)
//FRAME_GETTER(width, 2)
//FRAME_GETTER(height, 3)
//
//#undef FRAME_GETTER

struct flex_layout {
	// Set during init.
	bool wrap;
	bool reverse;               // whether main axis is reversed
	bool reverse2;              // whether cross axis is reversed (wrap only)
	bool vertical;
	float size_dim;             // main axis parent size
	float align_dim;            // cross axis parent size
	uint32_t frame_pos_i;   // main axis position
	uint32_t frame_pos2_i;  // cross axis position
	uint32_t frame_size_i;  // main axis size
	uint32_t frame_size2_i; // cross axis size
	//uint32_t* ordered_indices;
	std::vector<uint32_t> ordered_indices;
	// Set for each line layout.
	float line_dim;             // the cross axis size
	float flex_dim;             // the flexible part of the main axis size
	float extra_flex_dim;       // sizes of flexible items
	float flex_grows;
	float flex_shrinks;
	float pos2;                 // cross axis position

	// Calculated layout lines - only tracked when needed:
	//   - if the root's align_content property isn't set to FLEX_ALIGN_START
	//   - or if any child item doesn't have a cross-axis size set
	bool need_lines;
	struct flex_layout_line {
		uint32_t child_begin;
		uint32_t child_end;
		float size;
	};
	std::vector<flex_layout_line> lines;
	//uint32_t lines_count;
	float lines_sizes;
	//uint32_t lines_cap;
};

void layout_init(flex_item* item, float width, float height, struct flex_layout* layout)
{
	assert(item->padding_left >= 0);
	assert(item->padding_right >= 0);
	assert(item->padding_top >= 0);
	assert(item->padding_bottom >= 0);
	width -= item->padding_left + item->padding_right;
	height -= item->padding_top + item->padding_bottom;
	assert(width >= 0);
	assert(height >= 0);

	layout->reverse = false;
	layout->vertical = true;
	switch (item->direction) {
	case flex_item::flex_direction::ROW_REVERSE:
		layout->reverse = true;
	case flex_item::flex_direction::ROW:
		layout->vertical = false;
		layout->size_dim = width;
		layout->align_dim = height;
		layout->frame_pos_i = 0;
		layout->frame_pos2_i = 1;
		layout->frame_size_i = 2;
		layout->frame_size2_i = 3;
		break;

	case flex_item::flex_direction::COLUMN_REVERSE:
		layout->reverse = true;
	case flex_item::flex_direction::COLUMN:
		layout->size_dim = height;
		layout->align_dim = width;
		layout->frame_pos_i = 1;
		layout->frame_pos2_i = 0;
		layout->frame_size_i = 3;
		layout->frame_size2_i = 2;
		break;

	default:
		assert(false && "incorrect direction");
	}

	layout->ordered_indices.clear();
	if (item->children && item->should_order_children && item->children->size() > 0) {
		layout->ordered_indices.resize(item->children->size());
		uint32_t* indices = layout->ordered_indices.data();
		assert(indices != NULL);
		// Creating a list of item indices sorted using the children's `order'
		// attribute values. We are using a simple insertion sort as we need
		// stability (insertion order must be preserved) and cross-platform
		// support. We should eventually switch to merge sort (or something
		// else) if the number of items becomes significant enough.
		auto& icv = *item->children;
		for (uint32_t i = 0; i < item->children->size(); i++) {
			indices[i] = i;
			for (uint32_t j = i; j > 0; j--) {
				uint32_t prev = indices[j - 1];
				uint32_t curr = indices[j];
				if (icv[prev]->order <= icv[curr]->order) {
					break;
				}
				indices[j - 1] = curr;
				indices[j] = prev;
			}
		}
	}

	layout->flex_dim = 0;
	layout->flex_grows = 0;
	layout->flex_shrinks = 0;

	layout->reverse2 = false;
	layout->wrap = item->wrap != flex_item::flex_wrap::NO_WRAP;
	if (layout->wrap) {
		if (item->wrap == flex_item::flex_wrap::WRAP_REVERSE) {
			layout->reverse2 = true;
			layout->pos2 = layout->align_dim;
		}
	}
	else {
		layout->pos2 = layout->vertical
			? item->padding_left : item->padding_top;
	}

	layout->need_lines = layout->wrap && item->align_content != flex_item::flex_align::ALIGN_START;
	layout->lines.clear();
	layout->lines_sizes = 0;
}

void layout_cleanup(struct flex_layout* layout)
{
	layout->ordered_indices.clear();
	layout->lines.clear();
}

#define LAYOUT_RESET() \
    do { \
        layout->line_dim = layout->wrap ? 0 : layout->align_dim; \
        layout->flex_dim = layout->size_dim; \
        layout->extra_flex_dim = 0; \
        layout->flex_grows = 0; \
        layout->flex_shrinks = 0; \
    } \
    while (0)

#define LAYOUT_CHILD_AT(item, i) ((*item->children)[(layout->ordered_indices.size() ? layout->ordered_indices[i] : i)])
//#define LAYOUT_CHILD_AT(item, i) (item->children.ary[(layout->ordered_indices != NULL ? layout->ordered_indices[i] : i)])  

#define _LAYOUT_FRAME(child, name) child->frame[layout->frame_##name##_i]

#define CHILD_POS(child) _LAYOUT_FRAME(child, pos)
#define CHILD_POS2(child) _LAYOUT_FRAME(child, pos2)
#define CHILD_SIZE(child) _LAYOUT_FRAME(child, size)
#define CHILD_SIZE2(child) _LAYOUT_FRAME(child, size2)

#define CHILD_MARGIN(child, if_vertical, if_horizontal) \
    (layout->vertical \
     ? child->margin_##if_vertical \
     : child->margin_##if_horizontal)


bool layout_align(flex_item::flex_align align, float flex_dim, uint32_t children_count, float* pos_p, float* spacing_p, bool stretch_allowed)
{
	assert(flex_dim > 0);

	float pos = 0;
	float spacing = 0;
	switch (align) {
	case flex_item::flex_align::ALIGN_START:
		break;

	case flex_item::flex_align::ALIGN_END:
		pos = flex_dim;
		break;

	case flex_item::flex_align::ALIGN_CENTER:
		pos = flex_dim / 2;
		break;

	case flex_item::flex_align::ALIGN_SPACE_BETWEEN:
		if (children_count > 0) {
			spacing = flex_dim / (children_count - 1);
		}
		break;

	case flex_item::flex_align::ALIGN_SPACE_AROUND:
		if (children_count > 0) {
			spacing = flex_dim / children_count;
			pos = spacing / 2;
		}
		break;

	case flex_item::flex_align::ALIGN_SPACE_EVENLY:
		if (children_count > 0) {
			spacing = flex_dim / (children_count + 1);
			pos = spacing;
		}
		break;

	case flex_item::flex_align::ALIGN_STRETCH:
		if (stretch_allowed) {
			spacing = flex_dim / children_count;
			break;
		}
		// fall through
		break;
	default:
		return false;
	}

	*pos_p = pos;
	*spacing_p = spacing;
	return true;
}

flex_item::flex_align child_align(flex_item* child, flex_item* parent)
{
	auto align = child->align_self;
	if (align == flex_item::flex_align::ALIGN_AUTO) {
		align = parent->align_items;
	}
	return align;
}

void flex_item::layout_items(uint32_t child_begin, uint32_t child_end, uint32_t children_count, struct flex_layout* layout, uint32_t last_count)
{
	flex_item* item = this;
	assert(children_count <= (child_end - child_begin));
	if (children_count <= 0) {
		return;
	}
	if (last_count > 0 && last_count > children_count)
	{
		//children_count = last_count;
	}
	if (layout->flex_dim > 0 && layout->extra_flex_dim > 0) {
		// If the container has a positive flexible space, let's add to it
		// the sizes of all flexible children->
		layout->flex_dim += layout->extra_flex_dim;
	}

	// Determine the main axis initial position and optional spacing.
	float pos = 0;
	float spacing = 0;
	if (layout->flex_grows == 0 && layout->flex_dim > 0) {
		if (!layout_align(item->justify_content, layout->flex_dim,
			children_count, &pos, &spacing, false))
		{
			assert(0 && "incorrect justify_content");
		}
		if (layout->reverse) {
			pos = layout->size_dim - pos;
		}
	}

	if (layout->reverse) {
		pos -= layout->vertical ? item->padding_bottom : item->padding_right;
	}
	else {
		pos += layout->vertical ? item->padding_top : item->padding_left;
	}
	if (layout->wrap && layout->reverse2) {
		layout->pos2 -= layout->line_dim;
	}

	for (uint32_t i = child_begin; i < child_end; i++) {
		flex_item* child = LAYOUT_CHILD_AT(item, i);
		if (child->position == flex_position::POS_ABSOLUTE) {
			// Already positioned.
			continue;
		}

		// Grow or shrink the main axis item size if needed.
		float flex_size = 0;
		if (layout->flex_dim > 0) {
			if (child->grow != 0) {
				CHILD_SIZE(child) = 0; // Ignore previous size when growing.
				flex_size = (layout->flex_dim / layout->flex_grows)
					* child->grow;
			}
		}
		else if (layout->flex_dim < 0) {
			if (child->shrink != 0) {
				flex_size = (layout->flex_dim / layout->flex_shrinks)
					* child->shrink;
			}
		}
		CHILD_SIZE(child) += flex_size;

		// Set the cross axis position (and stretch the cross axis size if
		// needed).
		float align_size = CHILD_SIZE2(child);
		float align_pos = layout->pos2 + 0;
		switch (child_align(child, item)) {
		case flex_align::ALIGN_END:
			align_pos += layout->line_dim - align_size
				- CHILD_MARGIN(child, right, bottom);
			break;

		case flex_align::ALIGN_CENTER:
			align_pos += (layout->line_dim / 2) - (align_size / 2)
				+ (CHILD_MARGIN(child, left, top)
					- CHILD_MARGIN(child, right, bottom));
			break;

		case flex_align::ALIGN_STRETCH:
			if (align_size == 0) {
				CHILD_SIZE2(child) = layout->line_dim
					- (CHILD_MARGIN(child, left, top)
						+ CHILD_MARGIN(child, right, bottom));
			}
			// fall through

		case flex_align::ALIGN_START:
			align_pos += CHILD_MARGIN(child, left, top);
			break;

		default:
			assert(false && "incorrect align_self");
		}
		CHILD_POS2(child) = align_pos;

		// Set the main axis position.
		if (layout->reverse) {
			pos -= CHILD_MARGIN(child, bottom, right);
			pos -= CHILD_SIZE(child);
			CHILD_POS(child) = pos;
			pos -= spacing;
			pos -= CHILD_MARGIN(child, top, left);
		}
		else {
			pos += CHILD_MARGIN(child, top, left);
			CHILD_POS(child) = pos;
			pos += CHILD_SIZE(child);
			pos += spacing;
			pos += CHILD_MARGIN(child, bottom, right);
		}

		// Now that the item has a frame, we can layout its children.
		child->layout_item(child->frame[2], child->frame[3]);
	}

	if (layout->wrap && !layout->reverse2) {
		layout->pos2 += layout->line_dim;
	}

	if (layout->need_lines) {
		struct flex_layout::flex_layout_line line[1] = {};
		line->child_begin = child_begin;
		line->child_end = child_end;
		line->size = layout->line_dim;
		layout->lines.push_back(line[0]);
		layout->lines_sizes += line->size;
	}
}

void flex_item::layout_item(float width, float height)
{
	flex_item* item = this;
	if (!item->children || item->children->size() == 0) {
		return;
	}

	struct flex_layout layout_s = { 0 }, * layout = &layout_s;
	layout_init(item, width, height, &layout_s);

	LAYOUT_RESET();
	uint32_t last_count = 0;
	uint32_t last_layout_child = 0;
	uint32_t relative_children_count = 0;
	for (uint32_t i = 0; i < item->children->size(); i++) {
		flex_item* child = LAYOUT_CHILD_AT(item, i);

		// Items with an absolute position have their frames determined
		// directly and are skipped during layout.
		if (child->position == flex_position::POS_ABSOLUTE) {
#define ABSOLUTE_SIZE(val, pos1, pos2, dim) \
            (!isnan(val) \
             ? val \
             : (!isnan(pos1) && !isnan(pos2) \
                 ? dim - pos2 - pos1 \
                 : 0))

#define ABSOLUTE_POS(pos1, pos2, size, dim) \
            (!isnan(pos1) \
             ? pos1 \
             : (!isnan(pos2) \
                 ? dim - size - pos2 \
                 : 0))

			float child_width = ABSOLUTE_SIZE(child->width, child->left,
				child->right, width);

			float child_height = ABSOLUTE_SIZE(child->height, child->top,
				child->bottom, height);

			float child_x = ABSOLUTE_POS(child->left, child->right,
				child_width, width);

			float child_y = ABSOLUTE_POS(child->top, child->bottom,
				child_height, height);

			child->frame[0] = child_x;
			child->frame[1] = child_y;
			child->frame[2] = child_width;
			child->frame[3] = child_height;

			// Now that the item has a frame, we can layout its children.
			child->layout_item(child->frame[2], child->frame[3]);

#undef ABSOLUTE_POS
#undef ABSOLUTE_SIZE
			continue;
		}

		// Initialize frame.
		child->frame[0] = 0;
		child->frame[1] = 0;
		child->frame[2] = child->width;
		child->frame[3] = child->height;

		// Main axis size defaults to 0.
		if (isnan(CHILD_SIZE(child))) {
			CHILD_SIZE(child) = 0;
		}

		// Cross axis size defaults to the parent's size (or line size in wrap
		// mode, which is calculated later on).
		if (isnan(CHILD_SIZE2(child))) {
			if (layout->wrap) {
				layout->need_lines = true;
			}
			else {
				CHILD_SIZE2(child) = (layout->vertical ? width : height)
					- CHILD_MARGIN(child, left, top)
					- CHILD_MARGIN(child, right, bottom);
			}
		}

		// Call the self_sizing callback if provided. Only non-NAN values
		// are taken into account. If the item's cross-axis align property
		// is set to stretch, ignore the value returned by the callback.
		if (child->self_sizing != NULL) {
			float size[2] = { child->frame[2], child->frame[3] };

			child->self_sizing(child, size);

			for (uint32_t j = 0; j < 2; j++) {
				uint32_t size_off = j + 2;
				if (size_off == layout->frame_size2_i
					&& child_align(child, item) == flex_align::ALIGN_STRETCH) {
					continue;
				}
				float val = size[j];
				if (!isnan(val)) {
					child->frame[size_off] = val;
				}
			}
		}

		// Honor the `basis' property which overrides the main-axis size.
		if (!(isnan(child->basis) || child->basis < 0)) {
			assert(child->basis >= 0);
			CHILD_SIZE(child) = child->basis;
		}

		float child_size = CHILD_SIZE(child);
		if (layout->wrap) {
			if (layout->flex_dim < child_size) {
				// Not enough space for this child on this line, layout the
				// remaining items and move it to a new line.
				item->layout_items(last_layout_child, i, relative_children_count, layout, last_count);

				LAYOUT_RESET();
				last_layout_child = i;
				if (last_count < relative_children_count)
					last_count = relative_children_count;
				relative_children_count = 0;
			}

			float child_size2 = CHILD_SIZE2(child);
			if (!isnan(child_size2) && child_size2 > layout->line_dim) {
				layout->line_dim = child_size2;
			}
		}

		assert(child->grow >= 0);
		assert(child->shrink >= 0);

		layout->flex_grows += child->grow;
		layout->flex_shrinks += child->shrink;

		layout->flex_dim -= child_size
			+ (CHILD_MARGIN(child, top, left)
				+ CHILD_MARGIN(child, bottom, right));

		relative_children_count++;

		if (child_size > 0 && child->grow > 0) {
			layout->extra_flex_dim += child_size;
		}
	}

	// Layout remaining items in wrap mode, or everything otherwise.
	item->layout_items(last_layout_child, item->children ? item->children->size() : 0, relative_children_count, layout, last_count);

	// In wrap mode we may need to tweak the position of each line according to
	// the align_content property as well as the cross-axis size of items that
	// haven't been set yet.
	if (layout->need_lines && layout->lines.size() > 0) {
		float pos = 0;
		float spacing = 0;
		float flex_dim = layout->align_dim - layout->lines_sizes;
		if (flex_dim > 0) {
			if (!layout_align(item->align_content, flex_dim, layout->lines.size(), &pos, &spacing, true))
			{
				assert(0 && "incorrect align_content");
			}
		}

		float old_pos = 0;
		if (layout->reverse2) {
			pos = layout->align_dim - pos;
			old_pos = layout->align_dim;
		}

		for (uint32_t i = 0; i < layout->lines.size(); i++) {
			auto line = &layout->lines[i];

			if (layout->reverse2) {
				pos -= line->size;
				pos -= spacing;
				old_pos -= line->size;
			}

			// Re-position the children of this line, honoring any child
			// alignment previously set within the line.
			for (uint32_t j = line->child_begin; j < line->child_end;
				j++) {
				flex_item* child = LAYOUT_CHILD_AT(item, j);
				if (child->position == flex_position::POS_ABSOLUTE) {
					// Should not be re-positioned.
					continue;
				}
				if (isnan(CHILD_SIZE2(child))) {
					// If the child's cross axis size hasn't been set it, it
					// defaults to the line size.
					CHILD_SIZE2(child) = line->size
						+ (item->align_content == flex_align::ALIGN_STRETCH
							? spacing : 0);
				}
				CHILD_POS2(child) = pos + (CHILD_POS2(child) - old_pos);
			}

			if (!layout->reverse2) {
				pos += line->size;
				pos += spacing;
				old_pos += line->size;
			}
		}
	}

	layout_cleanup(layout);
}

#undef CHILD_MARGIN
#undef CHILD_POS
#undef CHILD_POS2
#undef CHILD_SIZE
#undef CHILD_SIZE2
#undef _LAYOUT_FRAME
#undef LAYOUT_CHILD_AT
#undef LAYOUT_RESET


void flex_item::layout()
{
	assert(parent == NULL);
	assert(!isnan(width));
	assert(!isnan(height));
	assert(self_sizing == NULL);
	layout_item(width, height);
}




#endif // !NO_FLEX_CX

widget_base::widget_base()
{
}
widget_base::widget_base(WIDGET_TYPE wt) :wtype(wt)
{
}
widget_base::~widget_base()
{
}
bool widget_base::on_mevent(int type, const glm::vec2& mps)
{
	return false;
}

bool widget_base::update(float delta)
{
	return false;
}

void widget_base::draw(cairo_t* cr)
{
}

glm::ivec2 widget_base::get_pos(bool has_parent)
{
	glm::ivec2 ps = pos;
	if (parent) {
		auto pss = parent->get_pos();
		auto ss = parent->get_spos();
		ss *= hscroll;
		ps += ss;
		if (has_parent) { ps += pss; }
	}
	return ps;
}

#define PANGO_EDIT0
// todo 编辑框实现
#ifndef NO_EDIT

struct PLogAttr
{
	bool is_line_break : 1;
	bool is_mandatory_break : 1;
	bool is_char_break : 1;
	bool is_white : 1;
	bool is_cursor_position : 1;
	bool is_word_start : 1;
	bool is_word_end : 1;
	bool is_sentence_boundary : 1;
	bool is_sentence_start : 1;
	bool is_sentence_end : 1;
	bool backspace_deletes_character : 1;
	bool is_expandable_space : 1;
	bool is_word_boundary : 1;
	bool break_inserts_hyphen : 1;
	bool break_removes_preceding : 1;
};
enum dir_e {
	DIRECTION_LTR,
	DIRECTION_RTL,
	DIRECTION_TTB_LTR,
	DIRECTION_TTB_RTL,
	DIRECTION_WEAK_LTR,
	DIRECTION_WEAK_RTL,
	DIRECTION_NEUTRAL
};
struct PLayoutLine {
	text_ctx_cx* layout = 0;
	int start_index = 0;
	int length = 0;
	int resolved_dir = DIRECTION_LTR;
};
struct GlyphInfoLC {
	uint32_t glyph;
	uint32_t width;
	uint32_t x_offset;
	uint32_t y_offset;
	int log_clusters;
	bool is_cluster_start;
	bool is_color;
};
struct PGlyphString {
	int num_glyphs;
	std::vector<GlyphInfoLC> glyphs;
	//int* log_clusters;
	void init(const char* text, int n_chars, int shape_width);
};

void PGlyphString::init(const char* text, int n_chars, int shape_width)
{
	const char* p = text;
	if (n_chars < 0)n_chars = strlen(text);
	glyphs.resize(n_chars);
	for (int i = 0; i < n_chars; i++, p = md::utf8_next_char(p))
	{
		glyphs[i].glyph = -1;// PANGO_GLYPH_EMPTY;
		glyphs[i].x_offset = 0;//geometry.
		glyphs[i].y_offset = 0;//geometry.
		glyphs[i].width = shape_width;//geometry.
		glyphs[i].is_cluster_start = 1;//attr.
		glyphs[i].log_clusters = p - text;
	}
}


class text_ctx_cx
{
public:
	glm::ivec2 pos = {}, size = {};
#ifdef PANGO_EDIT
	PangoContext* context = 0;
	PangoLayout* layout = 0;
	PangoLayout* layout_editing = 0;
	std::string family = {};
#else

	std::vector<PLayoutLine> lines;
	std::vector<PLogAttr> log_attrs;
	std::vector<glm::ivec2> lvs;// 行开始结束
	std::vector<std::vector<int>> widths;// 字符偏移

#endif
	image_ptr_t cacheimg = {};
	cairo_surface_t* sur = 0;
	layout_text_x* ltx = 0;
	std::string text;			// 原文本
	std::string stext;			// 显示的文本
	std::string editingstr;

	int fontid = 0;
	int fontsize = 8;
	uint32_t back_color = 0x06001020;		//背景色
	uint32_t text_color = 0xffffffff;
	std::vector<uint32_t> dtimg;
	std::vector<glm::ivec4> rangerc;
	PathsD range_path;						// 圆角选区缓存
	path_v ptr_path;
	float round_path = 0.28;				// 圆角比例
	float _posy = -1;
	glm::ivec2 cpos = {};					// 当前鼠标坐标
	glm::ivec2 scroll_pos = {};				// 滚动坐标
	glm::ivec2 _align_pos = {};				// 对齐偏移坐标
	glm::vec2 text_align = { 0, 0.5 };		// 对齐
	//std::string text;
	glm::ivec3 cursor = { 1,-1,0 };				// 闪烁光标。宽度、颜色、毫秒
	int select_color = 0x80ffb399;
	int editing_text_color = -1;
	glm::vec4 _shadow = { 0.0,0.0,0.0,0.5 };
	glm::vec4 box_color = { 0.2,0.2,0.2,0.5 };
	int64_t ccursor = 0;	//当前光标
	int64_t ccursor8 = 0;	//当前光标字符
	int64_t caret_old = {};		//保存输入光标
	glm::ivec3 cursor_pos = {};
	glm::i64vec2 cur_select = {};
	int ckselect = 0;
	int lineheight = 10;//行高
	int clineidx = 0;	// 当前行
	int c_ct = 0;
	int c_d = 0;
	int _baseline = 0;
	int ascent = 0, descent = 0;
	int curx = 0;
	char pwd = 0;
	bool valid = true;
	bool autobr = false;
	bool is_scroll = true;
	bool is_hover = false;
	bool single_line = false;
	bool show_input_cursor = true;
	bool hover_text = false;
	bool upft = true;
	bool roundselect = true;	// 圆角选区
private:
	int64_t bounds[2] = {};	//当前选择
public:
	text_ctx_cx();
	~text_ctx_cx();

	void set_autobr(bool is);
	void set_size(const glm::ivec2& ss);
	void set_family(const char* family);
	void set_font_size(int fs);

	void set_text(const std::string& str);
	void set_editing(const std::string& str);
	void set_cursor(const glm::ivec3& c);
	glm::ivec4 get_extents();
	int get_baseline();
	int get_lineheight();
	size_t get_xy_to_index(int x, int y, const char* str);
	glm::ivec4 get_line_extents(int lidx, int idx, int dir);
	glm::ivec2 get_layout_size();
	glm::ivec2 get_line_info(int y);
	glm::i64vec2 get_bounds();
	glm::i64vec2 get_bounds0();
	void set_bounds0(const glm::i64vec2& v);
	std::vector<glm::ivec4> get_bounds_px();
	void up_caret();
	bool update(float delta);
	void draw(cairo_t* cr);

	bool hit_test(const glm::ivec2& ps);
	void up_cursor(bool is);
	void set_single(bool is);
	glm::ivec4 get_cursor_posv(int idx);
#ifdef PANGO_EDIT
	glm::ivec2 get_pixel_size();
	void set_desc(const char* str);
	void set_markup(const std::string& str);
	glm::ivec2 get_layout_position(PangoLayout* layout);
	glm::ivec4 get_cursor_posv(PangoLayout* layout, int idx);
	glm::ivec2 get_line_length(int idx);
#else
	glm::ivec3 get_line_length(int idx);
	glm::ivec2 get_pixel_size(const char* str, int len);
#endif
private:
};
#ifndef PANGO_EDIT 
text_ctx_cx::text_ctx_cx()
{
#ifdef _WIN32
	auto n = GetCaretBlinkTime();
	if (cursor.z < 10)
	{
		cursor.z = n;
	}
#else
	cursor.z = 500;
#endif
}

text_ctx_cx::~text_ctx_cx()
{
	if (sur)
	{
		cairo_surface_destroy(sur); sur = 0;
	}
}

void text_ctx_cx::set_autobr(bool is)
{
}
void text_ctx_cx::set_single(bool is) {
	single_line = is;
	//if (size.y > 0 && single_line)
	//	pango_layout_set_height(layout, size.y * PANGO_SCALE);
	//else
	//	pango_layout_set_height(layout, -1);
	//pango_layout_set_single_paragraph_mode(layout, is);
}
void text_ctx_cx::set_size(const glm::ivec2& ss)
{
	if (size != ss)
	{
		size = ss;
		if (sur)
		{
			cairo_surface_destroy(sur);
		}
		dtimg.resize(size.x * size.y);
		sur = cairo_image_surface_create_for_data((unsigned char*)dtimg.data(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * sizeof(int));
		cacheimg = {};
		cacheimg.data = dtimg.data();
		cacheimg.width = size.x;
		cacheimg.height = size.y;
		cacheimg.type = 1;
		cacheimg.valid = 1;
		valid = true;
	}
}


void text_ctx_cx::set_family(const char* familys)
{
	if (familys && *familys)
	{
		//family = familys;
	}
	//PangoFontDescription* desc = pango_font_description_new();
	//pango_font_description_set_family(desc, family.c_str());
	//pango_font_description_set_size(desc, fontsize * PANGO_SCALE);
	//pango_layout_set_font_description(layout, desc);
	//pango_layout_set_font_description(layout_editing, desc);
	//pango_font_description_free(desc);
	upft = true;
	valid = true;
}

void text_ctx_cx::set_font_size(int fs)
{
	if (fs > 2)
	{
		fontsize = fs;
		//PangoFontDescription* desc = pango_font_description_new();
		//pango_font_description_set_family(desc, family.c_str());
		//pango_font_description_set_size(desc, fontsize * PANGO_SCALE);
		//pango_layout_set_font_description(layout, desc);
		//pango_layout_set_font_description(layout_editing, desc);
		//pango_font_description_free(desc);
		//pango_layout_set_line_spacing(layout, 1.2);
		//auto kf = pango_layout_get_line_spacing(layout) / PANGO_SCALE * 1.0;
		valid = true;
	}
}

void text_ctx_cx::set_text(const std::string& str)
{
	std::string pws;
	auto length = str.size();
	if (length && pwd)
	{
		pws.resize(length);
		for (size_t i = 0; i < length; i++)
		{
			pws[i] = pwd;
		}
		stext = pws;
		//pango_layout_set_text(layout, pws.c_str(), pws.size());
	}
	else
	{
		stext = str;
		//pango_layout_set_text(layout, str.c_str(), str.size());
	}
	text = str;

	{
		lvs.clear();
		auto length = stext.size();
		auto p = stext.c_str();
		size_t f = 0, s = 0;
		for (size_t i = 0; i < length; i++)
		{
			if (p[i] == '\n' || i == length - 1)
			{
				lvs.push_back({ f,i - f });
				f = i + 1;
			}
		}
		log_attrs.clear();
		log_attrs.resize(text.size());
	}
	widths.clear();
	valid = true;
	upft = true;
}

void text_ctx_cx::set_editing(const std::string& str)
{
	editingstr = str;
	//pango_layout_set_text(layout_editing, str.c_str(), str.size());
	//pango_layout_set_markup(layout_editing, str.c_str(), str.size());
	valid = true;
}


void text_ctx_cx::set_cursor(const glm::ivec3& c)
{
	cursor = c;
}

glm::ivec4 text_ctx_cx::get_extents()
{
	// todo 获取像素大小区域
	return {};
}

glm::ivec2 text_ctx_cx::get_pixel_size(const char* str, int len)
{
	int w = 0, h = 0;
	if (ltx && str && *str)
	{
		auto rc = ltx->get_text_rect(fontid, str, len, fontsize);
		w = rc.x; h = rc.y;
	}
	return glm::ivec2(w, h);
}

int text_ctx_cx::get_baseline()
{
	return ltx ? ltx->get_baseline(fontid, fontsize) : 0;
	//glm::ceil((double)pango_layout_get_baseline(layout) / PANGO_SCALE);
}
int text_ctx_cx::get_lineheight()
{
	return ltx ? ltx->get_lineheight(fontid, fontsize) : 0;
	//glm::ceil((double)pango_layout_get_baseline(layout) / PANGO_SCALE);
}
void glyph_string_x_to_index(PGlyphString* glyphs, const char* text, int length, bool r2l, int x_pos, int* index, bool* trailing)
{
	int i;
	int start_xpos = 0;
	int end_xpos = 0;
	int width = 0;

	int start_index = -1;
	int end_index = -1;

	int cluster_chars = 0;
	const char* p;

	bool found = false;

	/* Find the cluster containing the position */

	width = 0;

	if (r2l)//analysis->level % 2) /* Right to left */
	{
		for (i = glyphs->num_glyphs - 1; i >= 0; i--)
			width += glyphs->glyphs[i].width;

		for (i = glyphs->num_glyphs - 1; i >= 0; i--)
		{
			if (glyphs->glyphs[i].log_clusters != start_index)
			{
				if (found)
				{
					end_index = glyphs->glyphs[i].log_clusters;
					end_xpos = width;
					break;
				}
				else
				{
					start_index = glyphs->glyphs[i].log_clusters;
					start_xpos = width;
				}
			}

			width -= glyphs->glyphs[i].width;

			if (width <= x_pos && x_pos < width + glyphs->glyphs[i].width)
				found = true;
		}
	}
	else /* Left to right */
	{
		for (i = 0; i < glyphs->num_glyphs; i++)
		{
			if (glyphs->glyphs[i].log_clusters != start_index)
			{
				if (found)
				{
					end_index = glyphs->glyphs[i].log_clusters;
					end_xpos = width;
					break;
				}
				else
				{
					start_index = glyphs->glyphs[i].log_clusters;
					start_xpos = width;
				}
			}

			if (width <= x_pos && x_pos < width + glyphs->glyphs[i].width)
				found = true;

			width += glyphs->glyphs[i].width;
		}
	}

	if (end_index == -1)
	{
		end_index = length;
		end_xpos = (r2l) ? 0 : width;
	}

	/* Calculate number of chars within cluster */
	p = text + start_index;
	while (p < text + end_index)
	{
		p = md::utf8_next_char(p);
		cluster_chars++;
	}

	if (start_xpos == end_xpos)
	{
		if (index)
			*index = start_index;
		if (trailing)
			*trailing = false;
	}
	else
	{
		double cp = ((double)(x_pos - start_xpos) * cluster_chars) / (end_xpos - start_xpos);

		/* LTR and right-to-left have to be handled separately
		 * here because of the edge condition when we are exactly
		 * at a pixel boundary; end_xpos goes with the next
		 * character for LTR, with the previous character for RTL.
		 */
		if (start_xpos < end_xpos) /* Left-to-right */
		{
			if (index)
			{
				const char* p = text + start_index;
				int i = 0;

				while (i + 1 <= cp)
				{
					p = md::utf8_next_char(p);
					i++;
				}

				*index = (p - text);
			}

			if (trailing)
				*trailing = (cp - (int)cp >= 0.5);
		}
		else /* Right-to-left */
		{
			if (index)
			{
				const char* p = text + start_index;
				int i = 0;

				while (i + 1 < cp)
				{
					p = md::utf8_next_char(p);
					i++;
				}

				*index = (p - text);
			}

			if (trailing)
			{
				double cp_flip = cluster_chars - cp;
				*trailing = (cp_flip - (int)cp_flip >= 0.5);
			}
		}
	}
}

bool layout_line_x_to_index(PLayoutLine* line, int x_pos, int* index, int* trailing)
{
	int start_pos = 0;
	int first_index = 0; /* line->start_index */
	int first_offset;
	int last_index;      /* start of last grapheme in line */
	int last_offset;
	int end_index;       /* end iterator for line */
	int end_offset;      /* end iterator for line */
	text_ctx_cx* layout;
	int last_trailing;
	bool suppress_last_trailing;

	layout = line->layout;

	/* Find the last index in the line
	 */
	first_index = line->start_index;

	if (line->length == 0)
	{
		if (index)
			*index = first_index;
		if (trailing)
			*trailing = 0;

		return false;
	}

	assert(line->length > 0);
	auto text = layout->text.c_str();
	first_offset = md::utf8_pointer_to_offset(text, text + line->start_index);

	end_index = first_index + line->length;
	end_offset = first_offset + md::utf8_pointer_to_offset(text + first_index, text + end_index);

	last_index = end_index;
	last_offset = end_offset;
	last_trailing = 0;
	do
	{
		last_index = md::utf8_prev_char(text + last_index) - text;
		last_offset--;
		last_trailing++;
	} while (last_offset > first_offset && !layout->log_attrs[last_offset].is_cursor_position);

	/* This is a HACK. If a program only keeps track of cursor (etc)
	 * indices and not the trailing flag, then the trailing index of the
	 * last character on a wrapped line is identical to the leading
	 * index of the next line. So, we fake it and set the trailing flag
	 * to zero.
	 *
	 * That is, if the text is "now is the time", and is broken between
	 * 'now' and 'is'
	 *
	 * Then when the cursor is actually at:
	 *
	 * n|o|w| |i|s|
	 *              ^
	 * we lie and say it is at:
	 *
	 * n|o|w| |i|s|
	 *            ^
	 *
	 * So the cursor won't appear on the next line before 'the'.
	 *
	 * Actually, any program keeping cursor
	 * positions with wrapped lines should distinguish leading and
	 * trailing cursors.
	 */
	auto te = layout->lines.data() + layout->lines.size();
	auto tmp_list = layout->lines.data();
	while (tmp_list != line)
		tmp_list++;

	if (tmp_list != te && line->start_index + line->length == tmp_list->start_index)
		suppress_last_trailing = true;
	else
		suppress_last_trailing = false;

	if (x_pos < 0)
	{
		/* pick the leftmost char */
		if (index)
			*index = (line->resolved_dir == DIRECTION_LTR) ? first_index : last_index;
		/* and its leftmost edge */
		if (trailing)
			*trailing = (line->resolved_dir == DIRECTION_LTR || suppress_last_trailing) ? 0 : last_trailing;

		return false;
	}

	tmp_list = layout->lines.data();
	auto ltx = layout->ltx;
	auto strc = text + tmp_list->start_index;
	while (tmp_list)
	{
		auto cw = ltx->get_text_rect1(layout->fontid, layout->fontsize, strc);
		int logical_width = cw.x;// pango_glyph_string_get_width(run->glyphs);

		if (x_pos >= start_pos && x_pos < start_pos + logical_width)
		{
			int offset;
			bool char_trailing;
			int grapheme_start_index;
			int grapheme_start_offset;
			int grapheme_end_offset;
			int pos;
			int char_index = tmp_list->start_index;

			//glyph_string_x_to_index(run->glyphs, text + run->item->offset, run->item->length, &run->item->analysis, x_pos - start_pos, &pos, &char_trailing);

			//char_index = run->item->offset + pos;

			/* Convert from characters to graphemes */
			// 返回字符偏移
			offset = md::utf8_pointer_to_offset(text, text + char_index);

			grapheme_start_offset = offset;
			grapheme_start_index = char_index;
			while (grapheme_start_offset > first_offset && !layout->log_attrs[grapheme_start_offset].is_cursor_position)
			{
				grapheme_start_index = md::utf8_prev_char(text + grapheme_start_index) - text;
				grapheme_start_offset--;
			}

			grapheme_end_offset = offset;
			do
			{
				grapheme_end_offset++;
			} while (grapheme_end_offset < end_offset && !layout->log_attrs[grapheme_end_offset].is_cursor_position);

			if (index)
				*index = grapheme_start_index;

			if (trailing)
			{
				if ((grapheme_end_offset == end_offset && suppress_last_trailing) || offset + char_trailing <= (grapheme_start_offset + grapheme_end_offset) / 2)
					*trailing = 0;
				else
					*trailing = grapheme_end_offset - grapheme_start_offset;
			}

			return true;
		}

		start_pos += logical_width;
		tmp_list++;
	}

	/* pick the rightmost char */
	if (index)
		*index = (line->resolved_dir == DIRECTION_LTR) ? last_index : first_index;

	/* and its rightmost edge */
	if (trailing)
		*trailing = (line->resolved_dir == DIRECTION_LTR && !suppress_last_trailing) ? last_trailing : 0;

	return false;
}

bool layout_xy_to_index(text_ctx_cx* layout, int x, int y, int* index, int* trailing) {
	PLayoutLine* prev_line = 0;
	PLayoutLine* found = 0;
	int found_line_x = 0;
	int prev_last = 0;
	int prev_line_x = 0;
	bool retval = false;
	bool outside = false;
	auto iter = layout->lines.data();
	auto h = layout->get_lineheight();
	for (size_t i = 0; i < layout->lines.size(); i++, iter++)
	{
		glm::ivec4 line_logical = {};
		int first_y = i * h, last_y = h * (i + 1);
		//pango_layout_iter_get_line_extents(&iter, NULL, &line_logical);
		//pango_layout_iter_get_line_yrange(&iter, &first_y, &last_y);
		if (y < first_y)
		{
			if (prev_line && y < (prev_last + (first_y - prev_last) / 2))
			{
				found = prev_line;
				found_line_x = prev_line_x;
			}
			else
			{
				if (prev_line == 0)
					outside = true; /* off the top */
				found = iter;
				found_line_x = x - line_logical.x;
			}
		}
		else if (y >= first_y && y < last_y)
		{
			found = iter;
			found_line_x = x - line_logical.x;
		}
		prev_line = iter;
		prev_last = last_y;
		prev_line_x = x - line_logical.x;
		if (found != 0)
			break;
	}
	if (found == 0)
	{
		/* Off the bottom of the layout */
		outside = true;
		found = prev_line;
		found_line_x = prev_line_x;
	}
	retval = layout_line_x_to_index(found, found_line_x, index, trailing);
	if (outside)
		retval = false;
	return retval;
}
#if 0
size_t text_ctx_cx::get_xy_to_index(int x, int y, const char* str)
{
	if (ltx && widths.empty())
	{
		auto pstr = stext.c_str();
		ltx->get_text_posv(fontid, fontsize, pstr, stext.size(), widths);
	}
	x += scroll_pos.x - _align_pos.x;
	y += scroll_pos.y - _align_pos.y;

	if (x < 0)
	{
		x = 0;
	}
	if (y < 0)
	{
		y = 0;
	}
	int lc = lvs.size();
	glm::ivec2 lps = {};
	//pango_layout_get_pixel_size(layout, &lps.x, &lps.y);
	if (y > lps.y)
		y = lps.y - 1;
	y /= get_lineheight();
	if (y >= lvs.size())y = lvs.size() - 1;

	int index = 0, trailing = 0;
	bool k = layout_xy_to_index(this, x, y, &index, &trailing);

	clineidx = y;// 当前行号
	lineheight = get_lineheight();
	auto cursor = index + trailing;
	if (!k)
	{
		auto nk = get_line_length(cursor);
		cursor = nk.x;
	}
	auto cp = str + cursor;
	auto chp = md::get_utf8_first(cp);
	int ps = chp - cp;
	cursor += ps;
	return cursor;
}
#endif
// 获取鼠标坐标的光标位置
size_t text_ctx_cx::get_xy_to_index(int x, int y, const char* str)
{
	auto pstr = text.c_str();
	if (ltx && widths.empty())
	{
		ltx->get_text_posv(fontid, fontsize, pstr, text.size(), widths);
	}
	if (widths.size() != lvs.size())
		return -1;
	x += scroll_pos.x - _align_pos.x;
	y += scroll_pos.y - _align_pos.y;
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
	y /= get_lineheight();
	if (y >= lvs.size())y = lvs.size() - 1;
	auto ky = lvs[y];
	auto& ws = widths[y];
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
	//index += ky.x;
	curx = cw;
	//printf("gxy:%d\n", (int)index);
	return (size_t)index;
	auto cursor = index + trailing;
	auto cp = str + cursor;
	auto chp = md::get_utf8_first(cp);
	int ps = chp - cp;
	cursor += ps;
	return cursor;
}

// 输入行号，索引号，方向，输出行号选择范围
glm::ivec4 text_ctx_cx::get_line_extents(int lidx, int idx, int dir)
{
	//pango_layout_get_line_count
	return {};
}
glm::ivec3 text_ctx_cx::get_line_length(int index)
{
	int x_pos = 0;
	int lidx = 0;
	int cu = -1;
	int cx = 0;
	for (size_t i = 0; i < lvs.size(); i++)
	{
		auto c = lvs[i];
		if (index >= c.x && index < c.x + c.y + 1)//因为有换行+1
		{
			x_pos = index - c.x;
			lidx = c.x;
			cu = i;// 行号
			break;
		}
	}
	if (cu < 0 && lvs.size()) {
		cu = lvs.size() - 1;
		auto c = lvs[cu];
		x_pos = index - c.x;
		lidx = c.x;
	}
	glm::ivec3 ret = { lidx,cu, x_pos };
	return ret;
}

glm::ivec2 text_ctx_cx::get_layout_size()
{
	return size;
}
// todo line
glm::ivec2 text_ctx_cx::get_line_info(int y)
{
	return y > 0 && y < lvs.size() ? lvs[y] : glm::ivec2();
}

glm::i64vec2 text_ctx_cx::get_bounds()
{
	glm::i64vec2 v = { bounds[0] , bounds[1] };
	if (v.x > v.y) { std::swap(v.x, v.y); }
	return v;
}
glm::i64vec2 text_ctx_cx::get_bounds0()
{
	glm::i64vec2 v = { bounds[0] , bounds[1] };
	return v;
}
void text_ctx_cx::set_bounds0(const glm::i64vec2& v)
{
	bounds[0] = v.x; bounds[1] = v.y;
	//printf("bounds:%d\t%d\n", (int)v.x, (int)v.y);
	//if (v.x == v.y && v.x == 0) {
	//	printf(" \n");
	//}
}
glm::ivec2 geti2x(PangoLayout* layout, int x)
{
	int x_pos = 0;
	int lidx = 0;
	//pango_layout_index_to_line_x(layout, x, 0, &lidx, &x_pos);
	//x_pos /= PANGO_SCALE;
	return glm::ivec2(x_pos, lidx);
}

glm::ivec4 text_ctx_cx::get_cursor_posv(int idx)
{
	glm::ivec4 r = { /*w1.x,w1.y,w1.width,w1.height*/ };
	return r;
}

std::vector<glm::ivec4> get_caret_posv(PangoLayout* layout, int idx)
{
	std::vector<glm::ivec4> rv;
	//glm::ivec4 r = { w0.x,w0.y,w0.width,w0.height };
	//glm::ivec4 r1 = { w1.x,w1.y,w1.width,w1.height };
	//rv.push_back(r);
	//rv.push_back(r1); 
	return rv;
}

size_t char2pos(size_t ps, const char* str) {
	return md::get_utf8_count(str, ps);
}
// todo 获取范围的像素大小
std::vector<glm::ivec4> text_ctx_cx::get_bounds_px()
{
	float pwidth = fontsize * 0.5;// 补行尾宽度
	if (ltx && widths.empty())
	{
		auto pstr = text.c_str();
		ltx->get_text_posv(fontid, fontsize, pstr, text.size(), widths);
		pwidth = ltx->get_text_rect1(fontid, fontsize, "1").x;
	}
	std::vector<glm::ivec4> r;
	std::vector<glm::ivec4> rs, rss;
	auto pstr = text.c_str();
	auto tsize = text.size();
	auto v = get_bounds();
	if (v.x == v.y) { rangerc = rss; return rs; }
	if ((v.x >= tsize || v.y > tsize)) {
		rangerc = rss; return rs;
	}
	auto v1 = get_line_length(v.x);
	auto v2 = get_line_length(v.y);
	auto line_no = lvs.size();
	auto h = get_lineheight();
	// 计算选中范围的每行的坐标宽高
	if (v1 == v2) {}
	else {
		if (v1.y == v2.y)
		{
			auto ks = lvs[v1.y];
			auto w1 = widths[v1.y];
			auto xc = char2pos(v.x - ks.x, pstr + ks.x);
			auto yc = char2pos(v.y - ks.x, pstr + ks.x);
			int w = w1[xc];
			int ww = w1[yc] - w;
			rss.push_back({ w ,v1.y * h,ww,h });// 同一行时
		}
		else {
			auto ks = lvs[v1.y];
			auto w1 = widths[v1.y];
			auto w = w1[char2pos(v.x - ks.x, pstr + ks.x)];
			auto wd = *w1.rbegin() - w;
			rss.push_back({ w,v1.y * h,wd + pwidth,h });// 第一行
		}
		for (int i = v1.y + 1; i < line_no && i < v2.y; i++)
		{
			auto ks = lvs[i];
			auto w1 = widths[i];
			rss.push_back({ 0,i * h,*w1.rbegin() + pwidth, h });// 中间全行
		}
		if (v1.y < v2.y)
		{
			auto ks = lvs[v2.y];
			auto w1 = widths[v2.y];
			rss.push_back({ 0,v2.y * h,w1[char2pos(v.y - ks.x ,pstr + ks.x)],h });//最后一行
		}
	}
	if (roundselect)
	{
		PathsD subjects;
		PathD a;
		int py = _posy;
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
			path_v& ptr = ptr_path;
			ptr._data.clear();
			for (size_t i = 0; i < sn; i++)
			{
				auto& it = subjects[i];
				ptr.add_lines((glm::dvec2*)it.data(), it.size(), false);
			}
			range_path = gp::path_round(&ptr, -1, fontsize * round_path, 16, 0, 0);
		}
		else { range_path.clear(); }
	}
	rangerc = rss;
	return r;
}

void text_ctx_cx::up_caret()
{
	if (upft)
	{
		if (ltx) {
			get_bounds_px();
			_baseline = get_baseline(); lineheight = get_lineheight();
			upft = false;
		}
	}
	glm::ivec4 caret = {};
	auto v1 = get_line_length((int)ccursor);
	auto line_no = lvs.size();
	auto h = lineheight;
	// 计算选中范围的每行的坐标宽高 
	if (line_no > 0 && widths.size() > v1.y)
	{
		auto ks = lvs[v1.y];
		auto w1 = widths[v1.y];
		if (ltx) {
			auto pstr = text.c_str();
			caret.x = ltx->get_text_ipos(fontid, fontsize, pstr + ks.x, ks.y, ccursor - ks.x);
		}
		caret.y = cursor_pos.z * v1.y;
		//printf("cursor:\t%d\n", cursor_pos.x);
	}
	cursor_pos = caret; cursor_pos.z = h;
}
bool text_ctx_cx::update(float delta)
{
	c_ct += delta * 1000.0 * c_d;
	if (c_ct > cursor.z)
	{
		c_d = -1;
		c_ct = cursor.z;
		valid = true;
	}
	if (c_ct < 0)
	{
		c_d = 1; c_ct = 0;
		valid = true;
	}
	if (!valid)return false;
	auto cr = cairo_create(sur);
#if 1 
	size_t length = dtimg.size();
	auto dt = dtimg.data();
	for (size_t i = 0; i < length; i++)
	{
		*dt = back_color; dt++;
	}
#else
	set_color(cr, back_color);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
#endif

	cairo_as _ss_(cr);


	if (upft)
	{
		if (ltx) {
			get_bounds_px();
			_baseline = get_baseline(); lineheight = get_lineheight();
			upft = false;
		}
	}
	if (single_line) {
		glm::vec2 ext = { 0,lineheight }, ss = size;
		auto ps = ss * text_align - (ext * text_align);
		_align_pos.y = ps.y;
	}
	cairo_translate(cr, -scroll_pos.x + _align_pos.x, -scroll_pos.y + _align_pos.y);

	auto v = get_bounds();
	if (v.x != v.y && rangerc.size()) {
		set_color(cr, select_color);
		if (roundselect)
		{
			if (range_path.size() && range_path[0].size() > 3) {
				draw_polyline(cr, &range_path, true);
				cairo_fill(cr);
			}
		}
		else {
			for (auto& it : rangerc)
			{
				draw_rectangle(cr, it, std::min(it.z, it.w) * 0.18);
				cairo_fill(cr);
			}
		}
	}
	set_color(cr, text_color);
	auto lhh = get_pixel_size(stext.c_str(), stext.size());
	// 渲染文本
	glm::vec4 rc = { 0,0,  lhh };
	text_style_t st = {};
	st.font = 0;
	st.text_align = { 0.0,0.0 };
	st.font_size = fontsize;
	st.text_color = text_color;
	if (ltx)
		draw_text(cr, ltx, stext.c_str(), -1, rc, &st);

	cairo_restore(cr);
	cairo_destroy(cr);
	bool ret = valid;
	valid = false;
	return true;
}
uint32_t get_reverse_color(uint32_t color) {
	uint8_t* c = (uint8_t*)&color;
	c[0] = 255 - c[0];
	c[1] = 255 - c[1];
	c[2] = 255 - c[2];
	return color;
}
void text_ctx_cx::draw(cairo_t* cr)
{
	cairo_as _ss_(cr);
	// 裁剪区域
	cairo_rectangle(cr, pos.x, pos.y, size.x, size.y);
	cairo_clip(cr);

	auto ps = pos - scroll_pos + _align_pos;
	auto oldop = cairo_get_operator(cr);
	cairo_set_source_surface(cr, sur, pos.x, pos.y);
	cairo_paint(cr);
	double x = ps.x + cursor_pos.x, y = ps.y + cursor_pos.y;
	if (show_input_cursor && c_d == 1 && cursor.x > 0 && cursor_pos.z > 0)
	{
		set_color(cr, cursor.y);
		cairo_rectangle(cr, x, y, cursor.x, cursor_pos.z);
		cairo_fill(cr);
	}
	auto bbc = box_color;
	set_source_rgba(cr, bbc);
	cairo_rectangle(cr, pos.x - 0.5, pos.y - 0.5, size.x + 1, size.y + 1);
	cairo_set_line_width(cr, 1);
	cairo_stroke(cr);
	// 编辑中的文本
	if (editingstr.size())
	{
		cairo_translate(cr, x, y);
		// 渲染文本
		text_style_t st = {};
		st.font = 0;
		st.text_align = { 0.0,0.0 };
		st.font_size = fontsize;
		st.text_color = editing_text_color;

		glm::ivec2 lps = {};
		lps = get_pixel_size(editingstr.c_str(), editingstr.size());
		if (lps.y < lineheight)
			lps.y = lineheight;
		glm::vec4 lss = { 0,  lps.y + 0.5, lps.x, lps.y + 0.5 };
		glm::vec4 rc = { 1,1,lps.x + 2, lps.y + 2 };

		set_color(cr, get_reverse_color(editing_text_color));
		cairo_rectangle(cr, 0, 0, lps.x + 2, lps.y + 2);
		cairo_fill(cr);
		set_color(cr, editing_text_color);
		if (ltx)
			draw_text(cr, ltx, editingstr.c_str(), -1, rc, &st);
		cairo_move_to(cr, lss.x + 1, lss.y);
		cairo_line_to(cr, lss.z, lss.w);
		cairo_set_line_width(cr, 1);
		cairo_stroke(cr);
	}
}
#else

text_ctx_cx::text_ctx_cx()
{
	PangoFontMap* fontMap = get_fmap;
	context = pango_font_map_create_context(fontMap);
	//pango_context_set_round_glyph_positions(context, 0);
	layout = pango_layout_new(context);
	layout_editing = pango_layout_copy(layout);
	pango_layout_set_wrap(layout, PANGO_WRAP_CHAR);
	pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
#ifdef _WIN32
	auto n = GetCaretBlinkTime();
	if (cursor.z < 10)
	{
		cursor.z = n;
	}
#else
	cursor.z = 500;
#endif
}

text_ctx_cx::~text_ctx_cx()
{
	if (context)
	{
		g_object_unref(context); context = 0;
	}
	if (layout)
	{
		g_object_unref(layout); layout = 0;
	}
	if (layout_editing)
	{
		g_object_unref(layout_editing); layout_editing = 0;
	}
	if (sur)
	{
		cairo_surface_destroy(sur); sur = 0;
	}
}

void text_ctx_cx::set_autobr(bool is)
{
	pango_layout_set_width(layout, is ? size.x * PANGO_SCALE : -1);
}
void text_ctx_cx::set_single(bool is) {
	single_line = is;
	if (size.y > 0 && single_line)
		pango_layout_set_height(layout, size.y * PANGO_SCALE);
	else
		pango_layout_set_height(layout, -1);
	pango_layout_set_single_paragraph_mode(layout, is);
}
void text_ctx_cx::set_size(const glm::ivec2& ss)
{
	if (size != ss)
	{
		size = ss;
		if (sur)
		{
			cairo_surface_destroy(sur);
		}
		dtimg.resize(size.x * size.y);
		sur = cairo_image_surface_create_for_data((unsigned char*)dtimg.data(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * sizeof(int));
		cacheimg = {};
		cacheimg.data = dtimg.data();
		cacheimg.width = size.x;
		cacheimg.height = size.y;
		cacheimg.type = 1;
		cacheimg.valid = 1;
		valid = true;
	}
}

void text_ctx_cx::set_desc(const char* str)
{
	auto desc = pango_font_description_from_string(str);// "Sans Bold Italic Condensed 22.5px");
	if (desc)
	{
		pango_layout_set_font_description(layout, desc);
		pango_font_description_free(desc);
		valid = true;
	}
}

void text_ctx_cx::set_family(const char* familys)
{
	if (familys && *familys)
	{
		family = familys;
	}
	PangoFontDescription* desc = pango_font_description_new();
	pango_font_description_set_family(desc, family.c_str());
	pango_font_description_set_size(desc, fontsize * PANGO_SCALE);
	pango_layout_set_font_description(layout, desc);
	pango_layout_set_font_description(layout_editing, desc);
	pango_font_description_free(desc);
	upft = true;
	valid = true;
}

void text_ctx_cx::set_font_size(int fs)
{
	if (fs > 2)
	{
		fontsize = fs;
		PangoFontDescription* desc = pango_font_description_new();
		pango_font_description_set_family(desc, family.c_str());
		pango_font_description_set_size(desc, fontsize * PANGO_SCALE);
		pango_layout_set_font_description(layout, desc);
		pango_layout_set_font_description(layout_editing, desc);
		pango_font_description_free(desc);
		pango_layout_set_line_spacing(layout, 1.2);
		auto kf = pango_layout_get_line_spacing(layout) / PANGO_SCALE * 1.0;
		valid = true;
	}
}

void text_ctx_cx::set_text(const std::string& str)
{
	std::string pws;
	auto length = str.size();
	if (length && pwd)
	{
		pws.resize(length);
		for (size_t i = 0; i < length; i++)
		{
			pws[i] = pwd;
		}
		pango_layout_set_text(layout, pws.c_str(), pws.size());
	}
	else
	{
		pango_layout_set_text(layout, str.c_str(), str.size());
	}
	int h = 0;
	auto line = pango_layout_get_line(layout, 0);
	pango_layout_line_get_height(line, &h);
	lineheight = h / PANGO_SCALE;
	get_bounds_px();
	valid = true;
}

void text_ctx_cx::set_editing(const std::string& str)
{
	editingstr = str;
	pango_layout_set_text(layout_editing, str.c_str(), str.size());
	//pango_layout_set_markup(layout_editing, str.c_str(), str.size());
	valid = true;
}

void text_ctx_cx::set_markup(const std::string& str)
{
	pango_layout_set_markup(layout, str.c_str(), str.size());
	int h = 0;
	auto line = pango_layout_get_line(layout, 0);
	pango_layout_line_get_height(line, &h);
	lineheight = h / PANGO_SCALE;
	valid = true;
}

void text_ctx_cx::set_cursor(const glm::ivec3& c)
{
	cursor = c;
}

glm::ivec4 text_ctx_cx::get_extents()
{
	PangoRectangle ink_rect = {};
	PangoRectangle logical_rect = {};
	pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);
	return glm::ivec4(ink_rect.x, ink_rect.y, ink_rect.width, ink_rect.height);
}

glm::ivec2 text_ctx_cx::get_pixel_size()
{
	int w = 0, h = 0;
	pango_layout_get_pixel_size(layout, &w, &h);
	return glm::ivec2(w, h);
}

int text_ctx_cx::get_baseline()
{
	return glm::ceil((double)pango_layout_get_baseline(layout) / PANGO_SCALE);
}
size_t text_ctx_cx::get_xy_to_index(int x, int y, const char* str)
{
	x += scroll_pos.x - _align_pos.x;
	y += scroll_pos.y - _align_pos.y;

	if (x < 0)
	{
		x = 0;
	}
	if (y < 0)
	{
		y = 0;
	}
	auto cc = pango_layout_get_character_count(layout);
	int lc = pango_layout_get_line_count(layout);
	glm::ivec2 lps = {};
	pango_layout_get_pixel_size(layout, &lps.x, &lps.y);
	if (y > lps.y)
		y = lps.y - 1;
	int index = 0, trailing = 0;
	auto ls = pango_layout_get_lines_readonly(layout);
	bool k = pango_layout_xy_to_index(layout, x * PANGO_SCALE, y * PANGO_SCALE, &index, &trailing);
	int x_pos = 0;
	int lidx = 0;
	pango_layout_index_to_line_x(layout, index, 0, &lidx, &x_pos);


	x_pos /= PANGO_SCALE;
	auto cursor = index + trailing;
	if (!k)
	{
		auto nk = get_line_length(cursor);
		cursor = nk.x;
	}
	auto cp = str + cursor;
	auto chp = md::get_utf8_first(cp);
	int ps = chp - cp;
	cursor += ps;
	cursor;
	clineidx = lidx;
	//printf("%d\t%d\n", ccursor, ps);
	PangoRectangle ink_rect = {};
	PangoRectangle logical_rect = {};
	pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);	//cairo_move_to(cr, 0 /*extents.x*/, ink_rect.y - (logical_rect.height - ink_rect.height) * 0.5);
	// 获取基线位置
	int y_pos = pango_layout_get_baseline(layout);
	int h = 0;
	auto line = pango_layout_get_line(layout, lidx);
	pango_layout_line_get_height(line, &h);
	lineheight = h / PANGO_SCALE;
	return cursor;
}

glm::ivec2 text_ctx_cx::get_layout_position(PangoLayout* layout)
{
	const int text_height = size.y;
	PangoRectangle logical_rect;
	int y_pos, area_height;
	PangoLayoutLine* line;


	area_height = PANGO_SCALE * text_height;

	line = (PangoLayoutLine*)pango_layout_get_lines_readonly(layout)->data;
	pango_layout_line_get_extents(line, NULL, &logical_rect);

	/* Align primarily for locale's ascent/descent */
	if (_baseline < 0)
		y_pos = ((area_height - ascent - descent) / 2 +
			ascent + logical_rect.y);
	else
		y_pos = PANGO_SCALE * _baseline - pango_layout_get_baseline(layout);

	/* Now see if we need to adjust to fit in actual drawn string */
	if (logical_rect.height > area_height)
		y_pos = (area_height - logical_rect.height) / 2;
	else if (y_pos < 0)
		y_pos = 0;
	else if (y_pos + logical_rect.height > area_height)
		y_pos = area_height - logical_rect.height;

	y_pos = y_pos / PANGO_SCALE;
	return { -scroll_pos.x, y_pos };
}
int layout_get_char_width(PangoLayout* layout)
{
	int width;
	PangoFontMetrics* metrics;
	const PangoFontDescription* font_desc;
	PangoContext* context = pango_layout_get_context(layout);

	font_desc = pango_layout_get_font_description(layout);
	if (!font_desc)
		font_desc = pango_context_get_font_description(context);

	metrics = pango_context_get_metrics(context, font_desc, NULL);
	width = pango_font_metrics_get_approximate_char_width(metrics);
	pango_font_metrics_unref(metrics);

	return width;
}
gboolean text_util_get_block_cursor_location(PangoLayout* layout, int index, PangoRectangle* pos, gboolean* at_line_end)
{
	PangoRectangle strong_pos, weak_pos;
	PangoLayoutLine* layout_line;
	gboolean rtl;
	int line_no;
	const char* text;

	g_return_val_if_fail(layout != NULL, FALSE);
	g_return_val_if_fail(index >= 0, FALSE);
	g_return_val_if_fail(pos != NULL, FALSE);

	pango_layout_index_to_pos(layout, index, pos);

	if (pos->width != 0)
	{
		/* cursor is at some visible character, good */
		if (at_line_end)
			*at_line_end = FALSE;
		if (pos->width < 0)
		{
			pos->x += pos->width;
			pos->width = -pos->width;
		}
		return TRUE;
	}

	pango_layout_index_to_line_x(layout, index, FALSE, &line_no, NULL);
	layout_line = pango_layout_get_line_readonly(layout, line_no);
	g_return_val_if_fail(layout_line != NULL, FALSE);

	text = pango_layout_get_text(layout);

	if (index < pango_layout_line_get_start_index(layout_line) + pango_layout_line_get_length(layout_line))
	{
		/* this may be a zero-width character in the middle of the line,
		 * or it could be a character where line is wrapped, we do want
		 * block cursor in latter case */
		if (g_utf8_next_char(text + index) - text !=
			pango_layout_line_get_start_index(layout_line) + pango_layout_line_get_length(layout_line))
		{
			/* zero-width character in the middle of the line, do not
			 * bother with block cursor */
			return FALSE;
		}
	}

	/* Cursor is at the line end. It may be an empty line, or it could
	 * be on the left or on the right depending on text direction, or it
	 * even could be in the middle of visual layout in bidi text. */

	pango_layout_get_cursor_pos(layout, index, &strong_pos, &weak_pos);

	if (strong_pos.x != weak_pos.x)
	{
		/* do not show block cursor in this case, since the character typed
		 * in may or may not appear at the cursor position */
		return FALSE;
	}

	/* In case when index points to the end of line, pos->x is always most right
	 * pixel of the layout line, so we need to correct it for RTL text. */
	if (pango_layout_line_get_length(layout_line))
	{
		if (pango_layout_line_get_resolved_direction(layout_line) == PANGO_DIRECTION_RTL)
		{
			PangoLayoutIter* iter;
			PangoRectangle line_rect;
			int i;
			int left, right;
			const char* p;

			p = g_utf8_prev_char(text + index);

			pango_layout_line_index_to_x(layout_line, p - text, FALSE, &left);
			pango_layout_line_index_to_x(layout_line, p - text, TRUE, &right);
			pos->x = MIN(left, right);

			iter = pango_layout_get_iter(layout);
			for (i = 0; i < line_no; i++)
				pango_layout_iter_next_line(iter);
			pango_layout_iter_get_line_extents(iter, NULL, &line_rect);
			pango_layout_iter_free(iter);

			rtl = TRUE;
			pos->x += line_rect.x;
		}
		else
			rtl = FALSE;
	}
	else
	{
		PangoContext* context = pango_layout_get_context(layout);
		rtl = pango_context_get_base_dir(context) == PANGO_DIRECTION_RTL;
	}

	pos->width = layout_get_char_width(layout);

	if (rtl)
		pos->x -= pos->width - 1;

	if (at_line_end)
		*at_line_end = TRUE;

	return pos->width != 0;
}

glm::ivec2 get_index2pos(PangoLayout* layout, int idx) {
	PangoRectangle pos = {};
	pango_layout_index_to_pos(layout, idx, &pos);
	return glm::ivec2(pos.x / PANGO_SCALE, pos.y / PANGO_SCALE);
}
// 输入行号，索引号，方向，输出行号选择范围
glm::ivec4 text_ctx_cx::get_line_extents(int lidx, int idx, int dir)
{
	//pango_layout_get_line_count
	auto line = pango_layout_get_line(layout, lidx);
	auto lst = pango_layout_get_lines(layout);
	PangoRectangle ink_rect = {};
	PangoRectangle logical_rect = {};
	glm::ivec4 rt = {};
	gboolean at_line_end = 0;
	PangoRectangle tpos = {};
	if (line)
	{
		int h = 0;
		pango_layout_line_get_height(line, &h);
		h = h / PANGO_SCALE;
		auto xi = pango_layout_line_get_start_index(line);
		{
			gboolean rb = text_util_get_block_cursor_location(layout, idx > 0 ? idx : xi, &tpos, &at_line_end);
			pango_layout_get_cursor_pos(layout, idx > 0 ? idx : xi, &ink_rect, &logical_rect);
			glm::ivec4 r = { logical_rect.x,logical_rect.y,logical_rect.width,logical_rect.height }, r1 = { tpos.x,tpos.y,tpos.width,tpos.height };
			r /= PANGO_SCALE;
			r1 /= PANGO_SCALE;
			rt.x = r.x;
			rt.y = r.y;
			rt.w = h;
		}
		if (dir)
		{
			pango_layout_get_cursor_pos(layout, xi, &ink_rect, &logical_rect);
			rt.z = rt.x;
			rt.x = logical_rect.x / PANGO_SCALE;
		}
		else {
			pango_layout_line_get_pixel_extents(line, &ink_rect, &logical_rect);
			rt.z = logical_rect.width - rt.x;
		}
		int lh = 0;
		pango_layout_line_get_height(line, &lh);
		lh /= PANGO_SCALE;
	}
	return rt;
}
glm::ivec2 text_ctx_cx::get_line_length(int index)
{
	int x_pos = 0;
	int lidx = 0;
	pango_layout_index_to_line_x(layout, index, 0, &lidx, &x_pos);
	x_pos /= PANGO_SCALE;
	auto line = pango_layout_get_line(layout, lidx);
	int len = pango_layout_line_get_length(line);
	pango_layout_index_to_line_x(layout, len, 0, &lidx, &x_pos);
	glm::ivec2 ret = { line->start_index + len, x_pos / PANGO_SCALE };
	return ret;
}

glm::i64vec2 text_ctx_cx::get_bounds()
{
	glm::i64vec2 v = { bounds[0] , bounds[1] };
	if (v.x > v.y) { std::swap(v.x, v.y); }
	return v;
}
glm::ivec2 geti2x(PangoLayout* layout, int x)
{
	int x_pos = 0;
	int lidx = 0;
	pango_layout_index_to_line_x(layout, x, 0, &lidx, &x_pos);
	x_pos /= PANGO_SCALE;
	return glm::ivec2(x_pos, lidx);
}

glm::ivec4 text_ctx_cx::get_cursor_posv(PangoLayout* layout, int idx)
{
	std::vector<glm::ivec4> rv;
	PangoRectangle sw[2] = {};
	pango_layout_get_cursor_pos(layout, idx, &sw[0], &sw[1]);
	auto& w1 = sw[1];
	glm::ivec4 r = { w1.x,w1.y,w1.width,w1.height };
	r /= PANGO_SCALE;
	//r->y = lineheight * ly;
	return r;
}

std::vector<glm::ivec4> get_caret_posv(PangoLayout* layout, int idx)
{
	std::vector<glm::ivec4> rv;
	PangoRectangle sw[2] = {};
	pango_layout_get_caret_pos(layout, idx, &sw[0], &sw[1]);
	auto& w0 = sw[0];
	auto& w1 = sw[1];
	glm::ivec4 r = { w0.x,w0.y,w0.width,w0.height };
	glm::ivec4 r1 = { w1.x,w1.y,w1.width,w1.height };
	rv.push_back(r);
	rv.push_back(r1);
	for (auto& it : rv) {
		it /= PANGO_SCALE;
	}
	return rv;
}
int get_line_height(PangoLayout* layout, int idx) {
	auto line = pango_layout_get_line(layout, idx);
	int h = 0;
	pango_layout_line_get_height(line, &h);
	h = h / PANGO_SCALE;
	return h;
}

glm::ivec2 get_layout_size_(PangoLayout* layout)
{
	PangoRectangle ink, logical;
	pango_layout_get_extents(layout, &ink, &logical);
	return { logical.width / PANGO_SCALE,logical.height / PANGO_SCALE };
}
glm::ivec2 text_ctx_cx::get_layout_size()
{
	return get_layout_size_(layout);
}
struct it_rect
{
	glm::ivec4 line_rect = {}, char_rect = {};
	glm::ivec2 yr = {};
	int baseline = 0;
};
it_rect get_iter(PangoLayoutIter* iter) {

	it_rect ret = {};
	PangoRectangle lr = {}, cr = {};
	pango_layout_iter_get_line_extents(iter, NULL, (PangoRectangle*)&lr);
	pango_layout_iter_get_char_extents(iter, (PangoRectangle*)&cr);
	pango_layout_iter_get_line_yrange(iter, &ret.yr.x, &ret.yr.y);
	ret.baseline = pango_layout_iter_get_baseline(iter);
	ret.line_rect = glm::ivec4(lr.x, lr.y, lr.width, lr.height);
	ret.char_rect = glm::ivec4(cr.x, cr.y, cr.width, cr.height);
	ret.line_rect /= PANGO_SCALE;
	ret.char_rect /= PANGO_SCALE;
	ret.yr /= PANGO_SCALE;
	ret.baseline /= PANGO_SCALE;
	return ret;
}
std::vector<glm::ivec4> text_ctx_cx::get_bounds_px()
{
	std::vector<glm::ivec4> r;
	int x_pos = 0;
	int lidx = 0;
	auto v = get_bounds();
	auto v1 = geti2x(layout, v.x);
	auto v2 = geti2x(layout, v.y);
	if (v.x != v.y)
	{
		v = v;
	}
	auto vp1 = get_index2pos(layout, v.x);
	auto vp2 = get_index2pos(layout, v.y);

	auto nk = get_line_length(v.x);
	auto nk1 = get_line_length(v.y);
	auto ss = get_layout_size();
	auto sw0 = get_cursor_posv(layout, v.x);
	auto sw1 = get_cursor_posv(layout, v.y);
	std::vector<glm::ivec4> rs, rss;
	int line_no = pango_layout_get_line_count(layout);
	auto iter = pango_layout_get_iter(layout);
	auto pwidth = layout_get_char_width(layout) / PANGO_SCALE;
	for (int i = 0; i < line_no; i++)
	{
		auto rc = get_iter(iter);
		if (i != v1.y)
		{
			pango_layout_iter_next_line(iter); continue;
		}
		if (v1.y == v2.y)
		{
			rss.push_back({ rc.line_rect.x + sw0.x,rc.yr.x,sw1.x - sw0.x,rc.yr.y - rc.yr.x });
			break;
		}
		else
		{
			glm::ivec4 f = { rc.line_rect.x + sw0.x,rc.yr.x,(rc.line_rect.z - sw0.x) + pwidth,rc.yr.y - rc.yr.x };
			rss.push_back(f);
			pango_layout_iter_next_line(iter);
			for (size_t x = v1.y + 1; x < v2.y; x++)
			{
				rc = get_iter(iter);
				glm::ivec4 c = { rc.line_rect.x,rc.yr.x,rc.line_rect.z + pwidth,rc.yr.y - rc.yr.x };
				rss.push_back(c);
				pango_layout_iter_next_line(iter);
			}
			rc = get_iter(iter);
			f = { rc.line_rect.x ,rc.yr.x, sw1.x ,rc.yr.y - rc.yr.x };
			rss.push_back(f);
			break;
		}
	}

	pango_layout_iter_free(iter);
	if (roundselect)
	{
		PathsD subjects;
		PathD a;
		for (size_t i = 0; i < rss.size(); i++)
		{
			auto& it = rss[i];
			if (it.z < 1)
				it.z = pwidth;
			a.push_back({ it.x,it.y });
			a.push_back({ it.x + it.z,it.y });
			a.push_back({ it.x + it.z,it.y + it.w });
			a.push_back({ it.x,it.y + it.w });
			subjects.push_back(a);
			a.clear();
		}
		subjects = Union(subjects, FillRule::NonZero, 6);
		//range_path = InflatePaths(subjects, 0.5, JoinType::Round, EndType::Polygon);
		auto sn = subjects.size();
		if (sn > 0)
		{
			path_v& ptr = ptr_path;
			ptr._data.clear();
			for (size_t i = 0; i < sn; i++)
			{
				auto& it = subjects[i];
				ptr.add_lines((glm::dvec2*)it.data(), it.size(), false);
			}
			range_path = gp::path_round(&ptr, -1, fontsize * round_path, 16, 0, 0);
		}
		else { range_path.clear(); }
	}
	rangerc = rss;
	return r;
}

void text_ctx_cx::up_caret()
{
	auto kc = pango_layout_get_line_count(layout);
	auto lps = get_layout_position(layout);
	PangoRectangle sw[4] = {};
	auto v1 = geti2x(layout, ccursor);
	auto f = get_line_extents(v1.y, ccursor, 0);

	pango_layout_get_cursor_pos(layout, ccursor, &sw[0], &sw[1]);
	pango_layout_get_caret_pos(layout, ccursor, &sw[2], &sw[3]);
	glm::ivec4 caret = { sw->x,sw->y,sw[2].x,sw[2].y };
	caret /= PANGO_SCALE;
	int h = sw->height;
	h /= PANGO_SCALE;
	cursor_pos = caret; cursor_pos.z = h;
}
glm::ivec2 get_line_info_(PangoLayout* layout, int line)
{
	glm::ivec2 ret = {};
	int ct = pango_layout_get_line_count(layout);
	if (line >= ct)line = ct - 1;
	if (line < 0)line = 0;
	PangoLayoutLine* pl = pango_layout_get_line(layout, line);
	if (pl)
	{
		ret = { pl->start_index , pl->length };
	}
	return ret;
}

glm::ivec2 text_ctx_cx::get_line_info(int y)
{
	return get_line_info_(layout, y);
}

void renderer_draw_layout(cairo_t* cr, PangoLayout* layout, int x, int y, int baseline)
{
	PangoLayoutIter* iter;
	g_return_if_fail(PANGO_IS_LAYOUT(layout));
	iter = pango_layout_get_iter(layout);
	do
	{
		PangoRectangle   logical_rect;
		PangoLayoutLine* line;
		//int              baseline;

		line = pango_layout_iter_get_line_readonly(iter);
		glm::ivec2 yy = {};
		pango_layout_iter_get_line_yrange(iter, &yy.x, &yy.y);
		pango_layout_iter_get_line_extents(iter, NULL, &logical_rect);
		if (baseline == 0)
			baseline = pango_layout_iter_get_baseline(iter);
		cairo_save(cr);
		yy /= PANGO_SCALE;
		cairo_translate(cr, x + logical_rect.x, y + baseline + yy.x);
		pango_cairo_show_layout_line(cr, line);
		cairo_restore(cr);
	} while (pango_layout_iter_next_line(iter));

	pango_layout_iter_free(iter);

}
bool text_ctx_cx::update(float delta)
{
	c_ct += delta * 1000.0 * c_d;
	if (c_ct > cursor.z)
	{
		c_d = -1;
		c_ct = cursor.z;
		valid = true;
	}
	if (c_ct < 0)
	{
		c_d = 1; c_ct = 0;
		valid = true;
	}
	if (!valid)return false;
	auto cr = cairo_create(sur);
#if 1 
	size_t length = dtimg.size();
	auto dt = dtimg.data();
	for (size_t i = 0; i < length; i++)
	{
		*dt = back_color; dt++;
	}
#else
	set_color(cr, back_color);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
#endif
	cairo_save(cr);
	pango_cairo_update_layout(cr, layout);

	auto pwidth = layout_get_char_width(layout) / PANGO_SCALE;
	if (upft)
	{
		_baseline = get_baseline();
		upft = false;
	}
	if (single_line) {
		glm::vec2 ext = { 0,lineheight }, ss = size;
		auto ps = ss * text_align - (ext * text_align);
		_align_pos.y = ps.y;
	}
	cairo_translate(cr, -scroll_pos.x + _align_pos.x, -scroll_pos.y + _align_pos.y);

	auto v = get_bounds();
	if (v.x != v.y && rangerc.size()) {
		set_color(cr, select_color);
		if (roundselect)
		{
			if (range_path.size() && range_path[0].size() > 3) {
				draw_polyline(cr, &range_path, true);
				cairo_fill(cr);
			}
		}
		else {
			for (auto& it : rangerc)
			{
				draw_rectangle(cr, it, std::min(it.z, it.w) * 0.18);
				cairo_fill(cr);
			}
		}
	}
	set_color(cr, text_color);
	auto b = pango_layout_get_ellipsize(layout);
	pango_cairo_show_layout(cr, layout);
	cairo_restore(cr);
	cairo_destroy(cr);
	bool ret = valid;
	valid = false;
	return true;
}
uint32_t get_reverse_color(uint32_t color) {
	uint8_t* c = (uint8_t*)&color;
	c[0] = 255 - c[0];
	c[1] = 255 - c[1];
	c[2] = 255 - c[2];
	return color;
}
void text_ctx_cx::draw(cairo_t* cr)
{
	//printf("text_ctx_cx::draw\t%s\n",);
	cairo_save(cr);
	// 裁剪区域
	cairo_rectangle(cr, pos.x, pos.y, size.x, size.y);
	cairo_clip(cr);
	auto ps = pos - scroll_pos + _align_pos;
	auto oldop = cairo_get_operator(cr);
	cairo_set_source_surface(cr, sur, pos.x, pos.y);
	cairo_paint(cr);
	double x = ps.x + cursor_pos.x, y = ps.y + cursor_pos.y;
	if (show_input_cursor && c_d == 1 && cursor.x > 0 && cursor_pos.z > 0)
	{
		set_color(cr, cursor.y);
		cairo_rectangle(cr, x, y, cursor.x, cursor_pos.z);
		cairo_fill(cr);
	}
	cairo_restore(cr);
	auto bbc = box_color;
	set_source_rgba(cr, bbc);
	cairo_rectangle(cr, pos.x - 0.5, pos.y - 0.5, size.x + 1, size.y + 1);
	cairo_set_line_width(cr, 1);
	cairo_stroke(cr);
	// 编辑中的文本
	if (editingstr.size())
	{
		cairo_save(cr);
		cairo_translate(cr, x, y);

		pango_cairo_update_layout(cr, layout_editing);
		glm::ivec2 lps = {};
		pango_layout_get_pixel_size(layout_editing, &lps.x, &lps.y);
		if (lps.y < lineheight)
			lps.y = lineheight;
		glm::vec4 lss = { 0,  lps.y + 0.5, lps.x, lps.y + 0.5 };

		set_color(cr, get_reverse_color(editing_text_color));
		cairo_rectangle(cr, 0, 0, lps.x, lps.y + 2);
		cairo_fill(cr);
		set_color(cr, editing_text_color);
		pango_cairo_show_layout(cr, layout_editing);

		cairo_move_to(cr, lss.x, lss.y);
		cairo_line_to(cr, lss.z, lss.w);
		cairo_set_line_width(cr, 1);
		cairo_stroke(cr);
		cairo_restore(cr);
	}
}
#endif
bool text_ctx_cx::hit_test(const glm::ivec2& ps)
{
	auto p2 = ps;
	glm::vec4 rc = { 0,0,size };
	auto k2 = check_box_cr(p2, &rc, 1);
	return (k2.x);
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
			auto grab_h_pixels = clamp(scrollbar_size_v * (win_size_avail_v / win_size_v), GrabMinSize, scrollbar_size_v);
			auto grab_h_norm = grab_h_pixels / scrollbar_size_v;
			return glm::vec2{ grab_h_pixels, scrollbar_size_v };
		};
	auto x = calc_cb(vps, content_width, scw, scroll_width);
	if (!(x.x < x.y) || !isx)
		x.x = 0;
	return glm::vec3(x.x, x.y, isx);
}

void text_ctx_cx::up_cursor(bool is)
{
	if (is)
	{
		up_caret();
		glm::ivec2 cs = cursor_pos;
		auto evs = size;		// 视图大小
		auto h = cursor_pos.z;	// 行高
		if (h < 1)h = 1;
		evs.x -= _align_pos.x;
		int ey = evs.y - cursor_pos.z;
		//ey *= h;
		glm::ivec2 pos = {};
		if (is_scroll) {
			dcscroll(cs.x, evs.x, 2, scroll_pos.x);
			dcscroll(cs.y, ey, h, scroll_pos.y);
		}
		else
		{
			scroll_pos = { .0, .0 };
		}
		if (!(cs.x < 0 || cs.y < 0))
		{
			pos.x += cs.x;
			pos.y += cs.y;
		}
	}
}
edit_tl::edit_tl()
{
	widget_base::wtype = WIDGET_TYPE::WT_EDIT;
	ctx = new text_ctx_cx();
	//set_family("NSimSun", 12);
	set_family(0, 12);
	set_align_pos({ 4, 4 });
	set_color({ 0xff353535,-1,0xa0ff8000 ,0xff020202 });
}

edit_tl::~edit_tl()
{
	if (ctx)delete ctx; ctx = 0;
}
void edit_tl::set_single(bool is) {
	ctx->set_single(is);
	single_line = is;
}
void edit_tl::set_pwd(char ch)
{
	if (ctx)ctx->pwd = ch;
}
void edit_tl::set_family(int fontid, int fontsize) {
	if (fontsize > 0)
		ctx->fontsize = fontsize;
	ctx->fontid = fontid;
	//ctx->set_family(f);
}
void edit_tl::set_show_input_cursor(bool ab)
{
	ctx->show_input_cursor = ab;
}
void edit_tl::set_autobr(bool ab)
{
	ctx->set_autobr(ab);
}
void edit_tl::set_round_path(float v)
{
	ctx->round_path = std::max(0.0f, std::min(1.0f, v));
}
void edit_tl::inputchar(const char* str)
{
	int sn = strlen(str);
	if (!str || !sn || ctx->ccursor < 0 || ctx->ccursor>_text.size())
	{
		return;
	}
	ipt_text = str;
	std::string& sstr = ipt_text;
	ctx->single_line = single_line;
	if (single_line)
	{
		auto& v = sstr;
		v.erase(std::remove(v.begin(), v.end(), '\r'), v.end());
		v.erase(std::remove(v.begin(), v.end(), '\n'), v.end());
	}
	if (input_cb)
		input_cb(this, sstr);
	_text.insert(ctx->ccursor, sstr);
	sn = sstr.size();
	ctx->ccursor += sn;
	ctx->set_text(_text);
	ctx->set_bounds0({ ctx->ccursor ,ctx->ccursor });
	ctx->up_cursor(true);
	if (changed_cb)
		changed_cb(this);
}
bool edit_tl::remove_bounds()
{
	bool r = 0;
	auto v = ctx->get_bounds();
	if (v.x != v.y) {
		ctx->ccursor = v.x;
		remove_char(v.x, v.y - v.x);//删除选择的字符	 
		r = true;
		ctx->widths.clear();
		ctx->set_bounds0({ 0,0 });
	}
	return r;
}
void edit_tl::remove_char(size_t idx, int count)
{
	if (idx < _text.size() && count > 0)
	{
		_text.erase(idx, count);
		ctx->set_text(_text);
		ctx->ccursor = idx;
	}
}
void edit_tl::set_cursor(const glm::ivec3& c)
{
	if (ctx && c.z > 0)ctx->set_cursor(c);
}
uint32_t rgb2bgr(uint32_t c) {
	uint32_t r = c;
	auto c8 = (uint8_t*)&r;
	std::swap(c8[0], c8[2]);
	return r;
}
void edit_tl::set_color(const glm::ivec4& c) {
	ctx->back_color = rgb2bgr(c.x);
	ctx->text_color = c.y;
	ctx->select_color = c.z;
	ctx->editing_text_color = c.w;
}
void edit_tl::set_text(const void* str0, int len)
{
	char* str = (char*)str0;
	if (str)
	{
		if (len < 0)len = strlen(str);
		std::string nstr(str, len);
		if (nstr != _text)
		{
			_text.swap(nstr);
			ctx->set_text(_text);
		}
	}
	else {
		_text.clear();
		ctx->set_text(_text);
	}
	ctx->set_bounds0({ 0,0 });  ctx->ccursor = _text.size();
}
void edit_tl::add_text(const void* str0, int len)
{
	char* str = (char*)str0;
	if (str && *str)
	{
		if (len < 0)len = strlen(str);
		std::string nstr(str, len);
		auto ps = _text.size();
		_text += (nstr);
		ctx->set_text(_text);
		ctx->set_bounds0({ 0,0 });
		ctx->ccursor = ps + len;
	}
}
void edit_tl::set_size(const glm::ivec2& ss)
{
	size = ss;
	if (ss.x > 0 && ss.y > 0)
		ctx->set_size(ss);
}
void edit_tl::set_pos(const glm::ivec2& ps) {
	ctx->pos = pos = ps;
}
void edit_tl::set_align_pos(const glm::vec2& ps)
{
	ctx->_align_pos = ps;
}
void edit_tl::set_align(const glm::vec2& a)
{
	ctx->text_align = a;
}
glm::ivec4 edit_tl::input_pos() {
	auto p = this;
	glm::ivec2 cpos = p->ctx->cursor_pos;
	return { p->ppos + p->ctx->pos + cpos - p->ctx->scroll_pos + p->ctx->_align_pos
		,2, p->ctx->cursor_pos.z + 2 };
}
std::string edit_tl::get_select_str()
{
	auto cx = ctx->get_bounds();
	if (cx.x == cx.y)return "";
	return std::string(_text.substr(cx.x, cx.y - cx.x));
}
std::wstring edit_tl::get_select_wstr()
{
	auto cx = ctx->get_bounds();
	if (cx.x == cx.y)return L"";
	return std::wstring(hz::u8_to_u16(_text.substr(cx.x, cx.y - cx.x)));
}
// 发送事件到本edit
void edit_tl::on_event_e(uint32_t type, et_un_t* ep) {

	auto e = &ep->v;
	auto t = (devent_type_e)type;
	if (!ctx->ltx) { ctx->ltx = ltx; ctx->get_bounds_px(); }
	if (!ltx)return;
	switch (t)
	{
	case devent_type_e::mouse_move_e:
	{
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y }; mps -= ctx->pos + ppos;
		if (ctx->hit_test(mps) || mdown)
		{
			ctx->is_hover = true;
			p->cursor = (int)cursor_st::cursor_ibeam;//设置输入光标

			auto cx = ctx->get_xy_to_index(mps.x, mps.y, _text.c_str());
			auto bp = ctx->cur_select;

			if (bp.x != bp.y && (cx >= bp.x && cx < bp.y))
			{
				p->cursor = (int)cursor_st::cursor_arrow;
				ctx->hover_text = true;
			}
			else {
				ctx->hover_text = false;
			}
			if (mdown)
			{
				if (ctx->ckselect == 2 && ep->form)
				{
					ctx->ckselect = 3;
					std::wstring ws = get_select_wstr();
					ws.push_back(0);
					_istate = 0;

					printf("drag begin:%p\n", this);
					bool ok = parent && parent->dragdrop_begin ? parent->dragdrop_begin(ws.c_str(), ws.size()) : false;
					printf("drag end:%p\n", this);
					if (ok && !_read_only && !_istate) {

						auto ccr = ctx->get_bounds();
						auto d = bp.y - bp.x;

						if (ctx->ccursor < bp.x)
						{
							bp += d;
						}
						else { ccr += d; }

						ctx->set_bounds0(bp);
						remove_bounds();
						//printf("%p\t%d\t%d\n", this, ccr.x, bp.x);
						if (ctx->ckselect != 3)
						{
							ctx->set_bounds0(ccr);
							ctx->ccursor = ccr.y;
						}
						ctx->cur_select = ctx->get_bounds();
						ctx->get_bounds_px();
						ctx->up_cursor(true); mdown = false;
					}
					break;
				}
				if (ctx->ckselect == 0)
				{
					auto ob = ctx->get_bounds0();
					ctx->set_bounds0({ ob.x,cx });
					ctx->ccursor = cx;
				}
				ctx->get_bounds_px();
				if (ctx->c_d != 0)
				{
					ctx->c_d = -1; ctx->c_ct = -1;// 更新光标
				}
				ctx->up_cursor(true);
			}
		}
		else if (ctx->is_hover) {
			p->cursor = (int)cursor_st::cursor_arrow;
			ctx->is_hover = false;
		}
	}
	break;
	case devent_type_e::mouse_button_e:
	{
		auto p = e->b;
		glm::ivec2 mps = { p->x,p->y }; mps -= ctx->pos + ppos;
		bool isequal = ctx->cpos == mps;
		ctx->cpos = mps;
		if (ctx->hit_test(mps))
		{
			ep->ret = 1;
			auto cx = ctx->get_xy_to_index(mps.x, mps.y, _text.c_str());
			if (cx < 0)cx = 0;
			if (cx > _text.size())cx = _text.size();
			if (p->button == 1)
			{
				if (p->down == 0 && mdown && isequal && p->clicks == 1) //左键单击
				{
					auto bp = ctx->cur_select;
					auto ckse = ctx->ckselect;
					if (bp.x != bp.y && (cx >= bp.x && cx < bp.y))
					{
						ctx->hover_text = false;
					}
					ctx->ckselect = 0;
					ctx->ccursor = cx;
					ctx->set_bounds0({});
					ctx->up_cursor(true);
				}
				if (p->down)
				{
					auto bp = ctx->cur_select;
					if (ctx->hover_text)
					{
						ctx->ckselect = 2;
					}
					else
					{
						ctx->ckselect = 0;
						ctx->ccursor = cx;
						ctx->set_bounds0({ cx,cx });
						ctx->up_cursor(true);
					}
					if (ep->form)
					{
						if (parent && parent->form_set_input_ptr) { parent->form_set_input_ptr(ep->form, get_input_state(this, 1)); };
						ctx->c_d = -1; is_input = true;
					}
					else {
						ctx->c_d = 0; is_input = false;
					}
					mdown = true;
				}
			}
		}
		if (!p->down)
		{
			ctx->cur_select = ctx->get_bounds();
			ctx->ckselect = 1;
			mdown = false;
		}
	}
	break;
	case devent_type_e::mouse_wheel_e:
	{
		if (ctx->is_hover)
		{
			auto p = e->w;
			glm::ivec2 mps = { p->x,p->y };
			auto& sp = ctx->scroll_pos;
			sp.y -= mps.y * ctx->lineheight;
			auto lss = ctx->get_layout_size();
			lss.y -= ctx->lineheight;
			if (sp.y < 0)
			{
				sp.y = 0;
			}
			if (sp.y > lss.y)
			{
				sp.y = lss.y;
			}
		}
	}
	break;
	case devent_type_e::keyboard_e:
	{
		if (is_input)
			on_keyboard(ep);
	}
	break;
	case devent_type_e::text_editing_e:
	{
		if (!is_input)break;
		auto& p = e->e;
		bool setimepos = false;
		if (ctx->caret_old != ctx->ccursor)
		{
			ctx->caret_old = ctx->ccursor; setimepos = true;
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
		ctx->set_editing(str);
	}
	break;
	case devent_type_e::text_input_e:
	{
		auto p = e->t;
		remove_bounds();
		inputchar(p->text);
		auto ipos = input_pos();// 计算输入法坐标
		p->x = ipos.x;
		p->y = ipos.y + 3;
		p->w = ipos.z;
		p->h = ipos.w;
	}
	break;
	case devent_type_e::finger_e:
	{
		auto p = e->f;
		glm::ivec2 mps = ctx->cpos;
	}
	break;
	case devent_type_e::mgesture_e:
	{

	}
	break;
	case devent_type_e::ole_drop_e:
	{
		auto p = e->d;	// 接收ole拖放数据
		glm::ivec2 mps = { p->x,p->y }; mps -= ctx->pos + ppos;
		ctx->cpos = mps;
		if (ctx->hit_test(mps))
		{
			ep->ret = 1;
			auto cx = ctx->get_xy_to_index(mps.x, mps.y, _text.c_str());
			auto bp = ctx->cur_select;
			//printf("%d\n", ctx->c_ct);
			if (ctx->ckselect == 3 && bp.x != bp.y && (cx >= bp.x && cx < bp.y))
			{
				ctx->hover_text = true;
				//if (p->has) { *(p->has) = 0; }
				ctx->up_cursor(true);
				break;
			}
			else {
				ctx->hover_text = false;
			}
			if (p->has) { *(p->has) = 1; }
			if (ep->form && parent && parent->form_set_input_ptr) { parent->form_set_input_ptr(ep->form, get_input_state(this, 1)); }
			auto lastc = ctx->ccursor;
			ctx->ccursor = cx;
			is_input = true;
			mdown = false;
			if (ctx->c_d != 0)
			{
				ctx->c_d = -1; ctx->c_ct = -1;// 更新光标
			}
			if (p->count && p->str) {
				printf("drag input:%p\n", this);
				/*
				输入光标c、选区xy

				*/
				auto b = ctx->get_bounds();
				glm::ivec2 b0 = ctx->get_bounds0();
				int kb = b0.y - cx;
				bool r1 = false;
				if (b.x != b.y) {
					if (b0.y > cx) {
						_istate = remove_bounds();// 选区大于输入位置直接删除
						ctx->ccursor = cx;
					}
					else { r1 = true; }
				}
				int dc = (b0.x > b0.y) ? b.y - b.x : 0;//0是后

				auto c1 = ctx->ccursor;
				for (size_t i = 0; i < p->count; i++)
				{
					if (p->fmt == 1 && i) // 1是文件添加分隔符
					{
						inputchar("; ");
					}
					inputchar(p->str[i]);
				}
				auto cc = ctx->ccursor;
				c1 = cc - c1;//输入的长度 
				if (r1) {
					ctx->set_bounds0(b);
					_istate = remove_bounds();// 选区小于输入位置后删除 
				}
				//printf("dc:\t%d\txy%d %d kb %d\n", dc, b0.x, b0.y, kb);
				ctx->ccursor = cc;
				glm::ivec2 nb = { cc,cc };
				ctx->ckselect = 1;
				if (b.x != b.y) {
					if (r1)//后删除
					{
						nb.x = nb.y = cc - (b.y - b.x);
						if (dc == 0) {
							nb.x -= c1;//后光标
						}
						else {
							nb.y -= c1;//前光标
						}
					}
					else
					{
						// 前删除
						if (dc == 0) {
							nb.x -= c1;//后光标
						}
						else {
							nb.y -= c1;//前光标
						}
					}
				}
				ctx->set_bounds0(nb);
				ctx->ccursor = nb.y;
				//printf("ole %p\t%d %d \n", this, cx, ctx->ccursor); 
			}
			else
			{
				//ctx->set_bounds0({ cx,cx });
			}
		}
		else {
			ctx->c_d = 0; is_input = false;
		}
		ctx->get_bounds_px();
		ctx->cur_select = ctx->get_bounds();
		ctx->up_cursor(true);
	}
	break;
	default:
		break;
	}

}
glm::ivec2 get_cl(char* str, int cursor)
{
	auto cp = str + cursor;
	auto chp = md::get_utf8_first(cp);
	int ps = chp - cp;
	cursor += ps;
	int n = 0;
	for (size_t i = 0; i < cursor; i++)
	{
		if (str[i] == '\n') { n++; }
	}
	return { cursor, n };
}
int get_cl_count(char* str, int c0, int c1)
{
	int n = 0;
	for (size_t i = c0; i < c1; i++)
	{
		auto cp = str + i;
		auto chp = md::get_utf8_first(cp);
		int ps = chp - cp;
		i += ps;
		n++;
	}
	return n;
}
void edit_tl::on_keyboard(et_un_t* ep)
{
	auto p = ep->v.k;
	if (!p->down)
	{
		do {
			if (!p->kmod & 1)break;
			switch (p->keycode) {
			case SDLK_A:
			{
				ctx->ccursor = _text.size();
				ctx->set_bounds0({ 0,ctx->ccursor });
				ctx->get_bounds_px();
				ctx->up_cursor(true);
			}
			break;
			case SDLK_X:
			case SDLK_C:
			{
				//auto cp1 = ext->get_cpos2(0);
				//auto cp2 = ext->get_cpos2(1);
				//auto str = _storage_buf->get_range(cp1, cp2);
				auto rb = ctx->get_bounds();
				if (rb.x != rb.y)
				{
					auto str = _text.substr(rb.x, rb.y - rb.x);
					if (str.size())
					{
						set_clipboard(str.c_str());
						if (p->keycode == SDLK_X && !_read_only)
						{
							remove_bounds();
							ctx->up_cursor(true);
						}
					}
				}
			}
			break;
			case SDLK_V:
			{
				auto str = get_clipboard();
				remove_bounds();
				inputchar(str.c_str());
			}
			break;
			case SDLK_Y:
				//is_redo = true;	//_storage_buf->redo();
				break;
			case SDLK_Z:
				//is_undo = true;	//_storage_buf->undo();
				break;
			}
		} while (0);
	}
	if (!p->down || ctx->editingstr.size())
	{
		return;
	}

	bool isupcursor = false;
	switch (p->keycode)
	{
	case SDLK_TAB:
	{
		inputchar("\t");
	}
	break;
	case SDLK_BACKSPACE:
	{
		if (!remove_bounds()) {
			const char* str = _text.c_str();
			auto p = md::get_utf8_prev(str + ctx->ccursor - 1);
			size_t idx = p - str;
			size_t ct = ctx->ccursor - idx;
			remove_char(idx, ct);//删除一个字符

			ctx->up_cursor(true);
		}
	}
	break;
	case SDLK_PRINTSCREEN:
	{}
	break;
	case SDLK_SCROLLLOCK:
	{}
	break;
	case SDLK_PAUSE:
	{}
	break;
	case SDLK_INSERT:
	{

	}
	break;
	case SDLK_PAGEDOWN:
	{

	}
	break;
	case SDLK_PAGEUP:
	{

	}
	break;
	case SDLK_DELETE:
	{
		if (!remove_bounds()) {
			const char* str = _text.c_str();
			auto p = md::get_utf8_first(str + ctx->ccursor + 1);
			size_t idx = p - str;
			size_t ct = idx - ctx->ccursor;
			remove_char(ctx->ccursor, ct);
			ctx->up_cursor(true);
		}
	}
	break;
	case SDLK_HOME:
	{
		auto ts = _text.size();
		auto c = get_cl(_text.data(), ctx->ccursor);
		auto lp2 = ctx->get_line_info(c.y);
		ctx->ccursor = lp2.x;
		ctx->set_bounds0({});
		ctx->up_cursor(true);
	}
	break;
	case SDLK_END:
	{
		auto ts = _text.size();
		auto c = get_cl(_text.data(), ctx->ccursor);
		auto lp2 = ctx->get_line_info(c.y);
		ctx->ccursor = lp2.x + lp2.y;
		ctx->set_bounds0({});
		ctx->up_cursor(true);
	}
	break;
	case SDLK_RIGHT:
	{
		auto v = ctx->get_bounds();
		auto ts = _text.size();
		if (v.x != v.y) {
			ctx->ccursor = v.y;
			ctx->set_bounds0({});
		}
		else
		{
			const char* str = _text.c_str();
			auto p = md::get_utf8_first(str + ctx->ccursor + 1);
			size_t idx = p - str;
			ctx->ccursor = idx;
		}
		if (ctx->ccursor > ts)ctx->ccursor = ts;
		ctx->up_cursor(true);
	}
	break;
	case SDLK_LEFT:
	{
		auto v = ctx->get_bounds();
		if (v.x != v.y) {
			ctx->ccursor = v.x;
			ctx->set_bounds0({});
			break;
		}
		else
		{
			const char* str = _text.c_str();
			auto p = md::get_utf8_prev(str + ctx->ccursor - 1);
			size_t idx = p - str;
			ctx->ccursor = idx;
		}
		if (ctx->ccursor < 0)ctx->ccursor = 0;
		ctx->up_cursor(true);
	}
	break;
	case SDLK_DOWN:
	{
		auto ts = _text.size();
		auto c = get_cl(_text.data(), ctx->ccursor);
		auto& idx = c.y;
		auto lpo2 = ctx->get_line_info(idx);
		auto lp2 = ctx->get_line_info(++idx);
		auto x = get_cl_count(_text.data(), lpo2.x, ctx->ccursor);
		if (x >= lp2.y)
		{
			x = lp2.x + lp2.y;
		}
		else
		{
			const char* str = _text.c_str() + lp2.x;
			auto p = md::utf8_char_pos(str, x, -1);
			x = p - _text.c_str();
		}
		ctx->ccursor = x;
		ctx->set_bounds0({});
		ctx->up_cursor(true);
	}
	break;
	case SDLK_UP:
	{
		auto ts = _text.size();
		auto c = get_cl(_text.data(), ctx->ccursor);
		auto& idx = c.y;
		auto lpo2 = ctx->get_line_info(idx);
		auto lp2 = ctx->get_line_info(--idx);
		auto x = get_cl_count(_text.data(), lpo2.x, ctx->ccursor);
		if (x >= lp2.y)
		{
			x = lp2.x + lp2.y;
		}
		else
		{
			const char* str = _text.c_str() + lp2.x;
			auto p = md::utf8_char_pos(str, x, -1);
			x = p - _text.c_str();
		}
		ctx->ccursor = x;
		ctx->set_bounds0({});
		ctx->up_cursor(true);
	}
	break;
	case SDLK_RETURN:
	{
		remove_bounds();
		if (!single_line)
		{
			inputchar("\n");
		}
		ctx->up_cursor(true);
	}
	break;
	default:
		break;
	}
}

// 更新渲染啥的
bool edit_tl::update(float delta) {
	if (!ctx->ltx)
	{
		ctx->ltx = ltx;
	}
	if (!is_input)
	{
		ctx->c_d = 0;
	}
	glm::ivec2 ss = size;
	if (ctx->size != ss) {
		ctx->set_size(ss);
	}
	glm::ivec2 ps = pos;
	if (ctx->pos != ps) {
		ctx->pos = ps;
	}
	return ctx->update(delta);
}
// 获取纹理/或者渲染到cairo
image_ptr_t* edit_tl::get_render_data() {
	return &ctx->cacheimg;
}
void edit_tl::draw(cairo_t* cr)
{
	ctx->draw(cr);
}
#ifdef INPUT_STATE_TH
input_state_t* get_input_state(void* ptr, int t)
{
	static input_state_t r = {};
	if (t)
	{
		if (r.ptr)
		{
			auto p = (edit_tl*)r.ptr;
			p->ctx->editingstr.clear();
			p->is_input = false;
		}
		r.ptr = ptr;
	}
	if (ptr && r.ptr)
	{
		auto p = (edit_tl*)r.ptr;
		if (p) {
			*((glm::ivec4*)&r.x) = p->input_pos();
			r.y += 3;
		}
		if (!r.cb)
			r.cb = [](uint32_t type, et_un_t* e, void* ud) { if (ud) { ((edit_tl*)ud)->on_event_e(type, e); }	};
	}
	return &r;
}

#endif // INPUT_STATE_TH

#ifndef NO_TVIEW

class tview_x
{
public:
	glm::ivec2 vpos = {}, size = {};	// 渲染偏移，大小 
	std::vector<glm::vec4> ddrect;
	// 填充颜色 
	uint32_t clear_color = 0;
	cairo_surface_t* _backing_store = 0;
	cairo_surface_t* rss = 0;
	image_ptr_t img_rc = {};
	std::vector<uint32_t> _imgdata;
	glm::mat3 mx = glm::mat3(1.0);
	glm::vec2 last_mouse = {}, eoffset = {};
	glm::ivec4 hover_bx = {}, bx = {};		//当前鼠标对象包围框
	int ckinc = 0;
	int scaleStep = 2;
	int scale = 100;
	int oldscale = 0;
	int minScale = 2, maxScale = 25600;
	bool   _backing_store_valid = false;
	bool   has_move = false;
	bool   has_scale = false;
public:
	tview_x();
	~tview_x();
	void set_size(const glm::ivec2& ss);
	void set_view_move(bool is);	// 鼠标移动视图
	void set_view_scale(bool is);	// 滚轮缩放视图
	void set_rss(int r);
	void on_button(int idx, int down, const glm::vec2& pos, int clicks);
	void on_motion(const glm::vec2& ps);
	void on_wheel(int deltaY);
	void reset_view();
	image_ptr_t* get_ptr();
	glm::ivec4 get_hbox();
	glm::vec4 get_bbox();
	glm::mat3 get_affine();
	void hit_test(const glm::vec2& ps);
	cairo_t* begin_frame(bool redraw);
	void end_frame(cairo_t* cr);
	void set_draw_update()
	{
		_backing_store_valid = false;
	}
private:

};


tview_x::tview_x()
{
	//rss = new_clip_rect(5);
}

tview_x::~tview_x()
{
	if (rss)
	{
		cairo_surface_destroy(rss);
	}
	rss = 0;
	if (_backing_store)
	{
		cairo_surface_destroy(_backing_store);
	}
	_backing_store = 0;
}
image_ptr_t* tview_x::get_ptr()
{
	return  &img_rc;
}

cairo_t* tview_x::begin_frame(bool redraw)
{
	cairo_t* cr = 0;
	if (redraw || !_backing_store_valid && _backing_store)
	{
		cr = cairo_create(_backing_store);
		size_t length = size.x * size.y;
		auto img = (uint32_t*)cairo_image_surface_get_data(_backing_store);
		if (clear_color == 0) {
			memset(img, 0, length * sizeof(int));
		}
		else {
			for (size_t i = 0; i < length; i++)
			{
				img[i] = clear_color;
			}
		}

		//cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
		if (oldscale != scale)
		{
			oldscale = scale;
			//print_time a("canvas draw");
			auto m = get_affine();
		}

	}
	return cr;
}
void tview_x::end_frame(cairo_t* cr) {

	if (!_backing_store_valid)
	{
		_backing_store_valid = true;
		if (rss)
			clip_rect(cr, rss);
		cairo_destroy(cr);
	}
}
void tview_x::set_size(const glm::ivec2& ss)
{
	if (ss != size)
	{
		size = ss;
		if (_backing_store)
		{
			cairo_surface_destroy(_backing_store);
			_backing_store = 0;
		}
		_imgdata.resize(size.x * size.y);
		if (!_backing_store)
		{
			_backing_store = cairo_image_surface_create_for_data((unsigned char*)_imgdata.data(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
		}
		auto img = (uint32_t*)cairo_image_surface_get_data(_backing_store);
		img_rc.width = size.x;
		img_rc.height = size.y;
		img_rc.type = 1;
		img_rc.comp = 4;
		img_rc.data = img;
		img_rc.multiply = true;
	}
}

void tview_x::set_view_move(bool is)
{
	has_move = is;
}

void tview_x::set_view_scale(bool is)
{
	has_scale = is;
}

void tview_x::set_rss(int r)
{
	if (r > 0)
	{
		if (rss)
		{
			cairo_surface_destroy(rss);
		}
		rss = new_clip_rect(r);
	}
}



void tview_x::on_button(int idx, int down, const glm::vec2& pos1, int clicks)
{
	auto pos = pos1 - (glm::vec2)vpos;
	//idx=1左，3右，2中
	if (idx == 1)
	{
		if (down == 1 && ckinc == 0)
		{
			glm::vec2 a3 = mx[2];
			last_mouse = pos - a3;
		}
		ckinc++;
		if (down == 0)
		{
			ckinc = 0;
		}

	}
	else if (idx == 3) {
		if (down == 0)
		{
			//reset_view();
		}
	}
}
void tview_x::on_motion(const glm::vec2& pos1)
{
	auto pos = pos1 - (glm::vec2)vpos;
	if (ckinc > 0)
	{
		if (has_move) {
			auto mp = pos;
			mp = pos - last_mouse;
			auto t = glm::translate(glm::mat3(1.0), mp);
			double sc = scale / 100.0;
			auto s = glm::scale(glm::mat3(1.0), glm::vec2(sc, sc));
			mx = t * s;
			_backing_store_valid = false;
		}
	}
	eoffset = pos;
	hit_test(pos1);
}
void tview_x::on_wheel(int deltaY)
{
	if (has_scale)
	{
		auto prevZoom = scale;
		auto scale1 = scale;
		auto zoom = (deltaY * scaleStep);
		scale1 += zoom;
		if (scale1 < minScale) {
			scale = minScale;
			return;
		}
		else if (scale1 > maxScale) {
			scale = maxScale;
			return;
		}
		double sc = scale1 / 100.0;
		double sc1 = prevZoom / 100.0;
		glm::vec2 nps = mx[2];
		auto s = glm::scale(glm::mat3(1.0), glm::vec2(sc, sc));
		auto t = glm::translate(glm::mat3(1.0), glm::vec2(nps));
		mx = t * s;
		scale = scale1;
	}
	hit_test(eoffset + (glm::vec2)vpos);
	_backing_store_valid = false;
}
void tview_x::reset_view()
{
	ckinc = 0;
	scale = 100;
	mx = glm::mat3(1.0);
	_backing_store_valid = false;
}

glm::ivec4 tview_x::get_hbox()
{
	return hover_bx;
}
glm::vec4 tview_x::get_bbox()
{
	return bx;
}

glm::mat3 tview_x::get_affine()
{
	glm::mat3 r = glm::translate(glm::mat3(1.0), glm::vec2(vpos));
	return r * mx;
}

// 测试鼠标坐标的矩形
void tview_x::hit_test(const glm::vec2& ps)
{
	auto ae = get_affine();
	auto m = glm::inverse(ae);//逆矩阵
	auto p2 = v2xm3(ps, m);	 //坐标和矩阵相乘
	auto k2 = check_box_cr(p2, ddrect.data(), ddrect.size());
	hover_bx = {};
	if (k2.x)
	{
		auto v4 = get_boxm3(ddrect[k2.y], ae);
		v4.z -= v4.x;
		v4.w -= v4.y;
		hover_bx = { glm::floor(v4.x),glm::floor(v4.y),glm::round(v4.z),glm::round(v4.w) };
	}
}

#endif // !NO_TVIEW

#if 1
gshadow_cx::gshadow_cx()
{
}

gshadow_cx::~gshadow_cx()
{
}

image_sliced_t gshadow_cx::new_rect(const rect_shadow_t& rs)
{
	image_sliced_t r = {};
	glm::ivec2 ss = { rs.radius * 3, rs.radius * 3 };
	timg.resize(ss.x * ss.y);
	auto sur = new_image_cr(ss, timg.data());
	cairo_t* cr = cairo_create(sur);
	// 边框阴影
	draw_rectangle_gradient(cr, ss.x, ss.y, rs);

	image_ptr_t px = {};
	px.width = ss.x;
	px.height = ss.y;
	px.type = 1;
	px.stride = ss.x * sizeof(int);
	px.data = timg.data();
	px.comp = 4;
	glm::ivec2 ps = {};
	auto px0 = bcc.push_cache_bitmap(&px, &ps);
	if (px0) {
		r.img_rc = { 0,0,ss.x,ss.y };
		r.tex_rc = { ps.x,ps.y,ss.x,ss.y };
		r.sliced.x = r.sliced.y = r.sliced.z = r.sliced.w = rs.radius + 1;
		r.color = -1;
		img = px0;
	}
#ifdef _DEBUG
	image_save_png(sur, "temp/gshadow.png");
#endif
	free_image_cr(sur);
	cairo_destroy(cr);
	return r;
}


#endif

void widget_on_event(widget_base* p, uint32_t type, et_un_t* e, const glm::vec2& pos);
void send_hover(widget_base* wp, const glm::vec2& mps);

plane_cx::plane_cx()
{
	tv = new tview_x();
	ltx = new layout_text_x();
	auto st = &vgs;
	st->dash.v = 0x05050505;
	st->dash_num = 4;
	st->thickness = 0;
	st->join = 1;
	st->cap = 1;
	st->fill = 0x80FF7373;
	st->color = 0xffffffff;
	st->round = 6;
	vgtms.y = 20;

	push_dragpos({ 0,0 });
}

plane_cx::~plane_cx()
{
	remove_widget(horizontal);
	remove_widget(vertical);
	if (horizontal)
		delete horizontal;
	horizontal = 0;
	if (vertical)
		delete vertical;
	vertical = 0;
	for (auto it : widgets) {
		if (it && it->_autofree)delete it;
	}
	widgets.clear();
	if (tv)
	{
		delete tv; tv = 0;
	}
	if (_pat)
	{
		delete _pat; _pat = 0;
	}
	if (ltx)
	{
		delete ltx; ltx = 0;
	}
}
void plane_cx::set_fontctx(font_rctx* p)
{
	if (ltx && p)
		ltx->set_ctx(p);
}
glm::ivec2 plane_cx::get_pos() {
	return glm::ivec2(viewport.x, viewport.y);
}
glm::ivec2 plane_cx::get_spos()
{
	glm::vec2 sps = {};

	if (horizontal)
		sps.x = -horizontal->get_offset();
	if (vertical)
		sps.y = -vertical->get_offset();
	return sps;
}
void plane_cx::set_pos(const glm::ivec2& ps) {
	tpos = ps;
	viewport.x = ps.x; viewport.y = ps.y;
}
void plane_cx::set_size(const glm::ivec2& ss) {
	if (ss.x > 0 && ss.y > 0)
	{
		viewport.z = ss.x; viewport.w = ss.y;
		_clip_rect = { 0,0,ss.x,ss.y };
		uplayout = true;
		me.size = ss;
		if (tv)
			tv->set_size(ss);
		else
			return;
		if (!_pat)
		{
			_pat = new atlas_t();
			add_atlas(_pat);
		}
		_pat->img = tv->get_ptr();
		_pat->img->valid = true;
		//static uint32_t cc = 0x8f0080ff;
		//_pat->colors = &cc;
		_pat->img_rc = &_clip_rect;
		_pat->tex_rc = (glm::ivec4*)&tv->vpos;
		_pat->tex_rc->x = 0; _pat->tex_rc->y = 0;
		_pat->count = 1;
		_pat->clip = _clip_rect;
	}
}
glm::vec2 plane_cx::get_size()
{
	return glm::vec2(tv->size);
}
void plane_cx::set_select_box(int w, float s)
{
	vgs.thickness = w;
	vgtms.x = s;
}
size_t plane_cx::add_res(const std::string& fn)
{

	return 0;
}
size_t plane_cx::add_res(const char* data, int len)
{

	return 0;
}
//pos_width每次滚动量,垂直vnpos,水平hnpos为滚动条容器内偏移
void plane_cx::set_scroll(int width, int rcw, const glm::ivec2& pos_width, const glm::ivec2& vnpos, const glm::ivec2& hnpos)
{
	auto pss = get_size();
	{
		auto cp = add_scroll_bar({ width,pss.y - width * 2 }, pss.y, pss.y, rcw, true, vnpos);
		bind_scroll_bar(cp, true); // 绑定垂直滚动条
		cp->_pos_width = pos_width.y > 0 ? pos_width.y : width * 2;//滚轮事件每次滚动量
		cp->hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->has_hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->hscroll = {};
		cp->_absolute = true;
		cp->rounding = std::max(2, (int)(width * 0.5));
	}
	{
		auto cp = add_scroll_bar({ pss.x - width * 2,width }, pss.x, pss.x, rcw, false, hnpos);
		bind_scroll_bar(cp, false); // 绑定水平滚动条
		cp->_pos_width = pos_width.x > 0 ? pos_width.x : width;
		cp->hscroll = {};
		cp->_absolute = true;
		cp->rounding = std::max(2, (int)(width * 0.5));
	}
}
scroll2_t plane_cx::add_scroll2(const glm::ivec2& viewsize, int width, int rcw, const glm::ivec2& pos_width, const glm::ivec2& vnpos, const glm::ivec2& hnpos)
{
	scroll2_t r = {};
	auto pss = viewsize;
	{
		auto cp = add_scroll_bar({ width,pss.y - width * 2 }, pss.y, pss.y, rcw, true, vnpos);
		cp->_pos_width = pos_width.y > 0 ? pos_width.y : width * 2;//滚轮事件每次滚动量
		cp->hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->has_hover_sc = 1;	// 鼠标不在范围内也响应滚轮事件
		cp->hscroll = {};
		cp->rounding = std::max(2, (int)(width * 0.5));
		r.v = cp;
	}
	{
		auto cp = add_scroll_bar({ pss.x - width * 2,width }, pss.x, pss.x, rcw, false, hnpos);
		cp->_pos_width = pos_width.x > 0 ? pos_width.x : width;
		cp->hscroll = {};
		cp->rounding = std::max(2, (int)(width * 0.5));
		r.h = cp;
	}
	return r;
}

void plane_cx::set_scroll_hide(bool is)
{
	if (horizontal)
		horizontal->hideble = is;
	if (vertical)
		vertical->hideble = is;
}

void plane_cx::set_scroll_pos(const glm::ivec2& ps, bool v)
{
	if (v)
	{
		if (vertical)
			vertical->pos = ps;
	}
	else
	{
		if (horizontal)
			horizontal->pos = ps;
	}
}

void plane_cx::set_scroll_size(const glm::ivec2& ps, bool v)
{
	if (v)
	{
		if (vertical)
			vertical->size = ps;
	}
	else
	{
		if (horizontal)
			horizontal->size = ps;
	}
}

void plane_cx::set_view(const glm::ivec2& view_size, const glm::ivec2& content_size)
{
	if (horizontal)
		horizontal->set_viewsize(view_size.x, content_size.x, 0);
	if (vertical)
		vertical->set_viewsize(view_size.y, content_size.y, 0);
}

void plane_cx::set_scroll_visible(const glm::ivec2& hv)
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

glm::ivec2 plane_cx::get_scroll_range()
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

void plane_cx::set_scroll_pts(const glm::ivec2& pts, int t)
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
void plane_cx::set_clear_color(uint32_t c)
{
	tv->clear_color = rgb2bgr(c);
}
void plane_cx::set_color(const glm::ivec4& c) {
	border = c;
}
void plane_cx::move2end(widget_base* wp)
{
	if (wp)
	{
		auto& v = widgets;
		v.erase(std::remove_if(v.begin(), v.end(), [=](widget_base* pr) {return pr == wp; }), v.end());
		v.push_back(wp);
	}
}

size_t plane_cx::add_familys(const char* familys, const char* style)
{
	return ltx ? ltx->add_familys(familys, style) : 0;
}

scroll_bar* plane_cx::add_scroll_bar(const glm::ivec2& size, int vs, int cs, int rcw, bool v, const glm::ivec2& npos)
{
	auto p = new scroll_bar();
	if (p)
	{
		add_widget(p);
		p->_absolute = true;
		p->size = size;
		auto ss = get_size();
		glm::ivec2 dw = {};
		if (!v)
		{
			if (p->size.x < 0)
				p->size.x = ss.x - border.z * 2;
			p->pos.y = ss.y - border.y;
			p->pos.x = border.z;
			dw.y = 1;
			p->_dir = 0;
		}
		if (v)
		{
			if (p->size.y < 0)
				p->size.y = ss.y - border.z * 2;
			p->pos.x = ss.x - (border.y);
			p->pos.y = border.z;
			dw.x = 1;
			p->_dir = 1;
		}
		if (p->size.x < rcw) {
			p->size.x = rcw;
		}
		if (p->size.y < rcw) {
			p->size.y = rcw;
		}
		dw *= p->size;
		p->pos -= dw;
		p->pos -= npos;
		p->set_viewsize(vs, cs, rcw);
	}
	return p;
}

void plane_cx::bind_scroll_bar(scroll_bar* p, bool v)
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


void plane_cx::add_widget(widget_base* p)
{
	if (p)
	{
		p->ltx = ltx;
		p->parent = this;
		widgets.push_back(p); uplayout = true;
	}
}
void plane_cx::remove_widget(widget_base* p)
{
	auto& v = widgets;
	auto ps = v.size();
	v.erase(std::remove(v.begin(), v.end(), p), v.end());
	if (v.size() != ps)
	{
		valid = true; uplayout = true;
	}
}
switch_tl* plane_cx::add_switch(const glm::ivec2& size, const std::string& label, bool v, bool inlinetxt)
{
	auto p = new switch_tl();
	if (p) {
		add_widget(p);
		p->set_value(v);
		p->size = size;
		p->text = label;
		if (ltx && ltx->ctx) {
			glm::vec4 rc = { 0, 0, 0, 0 };
			glm::vec2 talign = { 0,0.5 };
			if (label.size())
			{
				//	p->txtps = ltx->add_text(0, rc, talign, label.c_str(), label.size(), fontsize);
			}
		}
		p->_autofree = true;
	}
	return p;
}
checkbox_tl* plane_cx::add_checkbox(const glm::ivec2& size, const std::string& label, bool v)
{
	auto p = new checkbox_tl();
	if (p) {
		add_widget(p);
		p->set_value(label, v);
		p->text = label;
		p->size = size;
		glm::vec4 rc = { 0, 0, 0, 0 };
		glm::vec2 talign = { 0,0.5 };
		if (label.size())
		{
		}
		p->_autofree = true;
	}
	return p;
}
radio_tl* plane_cx::add_radio(const glm::ivec2& size, const std::string& label, bool v, group_radio_t* gp)
{
	auto p = new radio_tl();
	if (p) {
		p->gr = gp;
		if (gp)
			gp->ct++;
		add_widget(p);
		p->set_value(label, v);
		p->text = label;
		p->size = size;
		glm::vec4 rc = { 0, 0, 0, 0 };
		glm::vec2 talign = { 0,0.5 };
		if (label.size())
		{
		}
		p->_autofree = true;
	}
	return p;
}

edit_tl* plane_cx::add_input(const std::string& label, const glm::ivec2& size, bool single_line) {
	edit_tl* edit1 = new edit_tl();
	if (edit1) {
		auto ss = size;
		edit1->set_size(ss);
		edit1->set_single(single_line);
		//edit1->set_family(familys.c_str(), fontsize); // 多字体混合无法对齐高度。英文行和中文行高度不同		 
		edit1->set_family(0, fontsize); // 多字体混合无法对齐高度。英文行和中文行高度不同		 
		edit1->ppos = get_pos();
		add_widget(edit1);
		edit1->_autofree = true;
	}
	return edit1;
}
gradient_btn* plane_cx::add_gbutton(const std::string& label, const glm::ivec2& size, uint32_t bcolor)
{
	gradient_btn* gb = new gradient_btn();
	if (gb) {
		gb->init({ 0,0,size }, label, bcolor);
		gb->family = familys;
		gb->font_size = fontsize;
		add_widget(gb);
		gb->_autofree = true;
	}
	return gb;
}
color_btn* plane_cx::add_cbutton(const std::string& label, const glm::ivec2& size, int idx)
{
	color_btn* gb = new color_btn();
	if (gb)
	{
		gb->str = label;
		gb->family = familys;
		gb->font_size = fontsize;
		gb->set_btn_color_bgr(idx);
		gb->size = size;
		add_widget(gb);
		gb->_autofree = true;
	}
	return gb;
}

color_btn* plane_cx::add_label(const std::string& label, const glm::ivec2& size, int idx)
{
	auto p = add_cbutton(label, size, idx);
	if (p)
	{
		p->effect = uTheme::light;
		p->light = 0;
		p->text_align.x = 0;
	}
	return p;
}

progress_tl* plane_cx::add_progress(const std::string& format, const glm::ivec2& size, double v)
{
	auto p = new progress_tl();
	if (p)
	{
		p->size = size;
		p->format = format;
		add_widget(p);
		p->set_value(v);
		p->_autofree = true;
	}
	return p;
}

slider_tl* plane_cx::add_slider(const glm::ivec2& size, int h, double v)
{
	auto p = new slider_tl();
	if (p)
	{
		p->size = size;
		p->wide = h;
		p->vertical = size.y > size.x;
		if (p->vertical)
			p->sl.x = size.x * 0.5;
		else
			p->sl.x = size.y * 0.5;
		add_widget(p);
		p->set_value(v);
		p->_autofree = true;
	}
	return p;
}

colorpick_tl* plane_cx::add_colorpick(uint32_t c, int w, int h, bool alpha)
{
	auto p = new colorpick_tl();
	if (p) {
		add_widget(p);
		p->init(c, w, h, alpha);
		p->_autofree = true;
	}
	return p;
}

void plane_cx::set_family_size(const std::string& fam, int fs, uint32_t color)
{
	if (fam.size())
		familys = fam;
	if (fs > 0)
		fontsize = fs;
	text_color = color;
}



void plane_cx::set_update()
{
	evupdate++;
}
void plane_cx::update(float delta)
{
	//print_time a("plane_cx::update");
	int ic = 0;
	if (horizontal)
	{
		horizontal->update(delta);
	}
	if (vertical) {
		vertical->update(delta);
	}
	auto sps = get_spos();	// 获取滚动量

	// 
	//printf("type\t%d\n", _hover_eq.w);
	if (_hover_eq.z > 0 && (devent_type_e)_hover_eq.w == devent_type_e::mouse_move_e)
	{
		_hover_eq.x += delta;
		// 大于设置时间触发hover事件
		if (_hover_eq.x > _hover_eq.y) {
			auto length = event_wts.size();
			for (size_t i = 0; i < length; i++)
			{
				auto pw = event_wts[i];
				if (!pw || !pw->visible || pw->_disabled_events || !(pw->bst & (int)BTN_STATE::STATE_HOVER) || pw->bst & (int)BTN_STATE::STATE_ACTIVE)continue;
				auto vpos = sps * pw->hscroll;
				send_hover(pw, _move_pos);
			}
			_hover_eq.x = _hover_eq.z = 0;//重置
		}
	}
	for (auto& it : widgets) {
		ic += it->update(delta);
	}
	if (update_cb)
		ic += update_cb(delta);
	if (uplayout) {
		mk_layout();
	}
	ic += ltx->update_text();
	if (_draw_sbox && vgs.thickness > 0 && vgtms.x > 0 && vgtms.y > 0)
	{
		auto& kt = vgtms.z;
		kt += delta;
		if (kt > vgtms.x)
		{
			vgtms.w += 1;
			if (vgtms.w > vgtms.y)vgtms.w = 0;
			kt = 0.0;
			ic++;
		}
	}
	else {
		vgtms.w = 0;
	}
	if (ic > 0 || evupdate > 0)tv->set_draw_update();
	auto kms = delta * 1000;
	dms -= kms;
	if (dms > 0 || kms <= 0)
		return;
	dms = dmsset;
	auto cr = tv->begin_frame(0);
	if (cr)
	{
		evupdate = 0;
		auto ls = get_size();
		bool has_border = (border.y > 0 && border.x != 0);
		{
			// 背景 
			if (border.w) {
				glm::vec4 rf = { 0.,0.,ls };
				if (has_border) {
					rf.x = rf.y = 0.5;
				}
				draw_rectangle(cr, rf, border.z);
				fill_stroke(cr, border.w, 0, 0, false);
			}
			if (draw_back_cb)
			{
				cairo_as _aa_(cr);
				draw_back_cb(cr, sps);
			}
			for (auto& it : widgets) {
				if (it->visible)
				{
					cairo_as __cas_(cr);
					auto scp = sps * it->hscroll;
					if (scp.x != 0 || scp.y != 0)
						cairo_translate(cr, scp.x, scp.y);// 滚动条影响
					it->draw(cr);
				}
			}
			if (draw_front_cb)
			{
				cairo_as _aa_(cr);
				draw_front_cb(cr, sps);
			}
			if (vgs.thickness > 0) {
				auto dps1 = get_dragpos(0);//获取拖动时的坐标
				auto v6 = get_dragv6(0);
				auto st = &vgs;
				st->dash_offset = -vgtms.w;
				float pad = st->thickness > 1 ? 0.0 : -0.5;
				cairo_as _ss_(cr);
				auto pos = v6->cp0;
				auto pos1 = v6->cp1;
				auto w = pos1;
				auto ss = glm::abs(pos - w);
				if (w.x < pos.x)
				{
					pos.x = w.x;
				}
				if (w.y < pos.y)
				{
					pos.y = w.y;
				}
				_draw_sbox = ss.x > 0 && ss.y > 0;
				if (_draw_sbox)
				{
					auto r = ss.x < st->round * 2 || ss.y < st->round * 2 ? 0 : st->round;
					draw_rectangle(cr, { pos.x + pad ,pos.y + pad ,ss.x,ss.y }, r);
					fill_stroke(cr, st);
				}
			}
			if (horizontal)
			{
				horizontal->draw(cr);
			}
			if (vertical) {
				vertical->draw(cr);
			}
		}
		// 边框线  
		if (has_border)
		{
			ls -= 1;
			draw_rectangle(cr, { 0.5,0.5,ls }, border.z);
			fill_stroke(cr, 0, border.x, border.y, false);
		}
		tv->end_frame(cr);
		_pat->img->valid = true;
	}
	bool savetext = false;
	if (savetext)
	{
		int i = 0;
		for (auto img : ltx->msu)
		{
			std::string fn = "cache/update_text_img_" + std::to_string(i++) + ".png";
			cairo_surface_write_to_png(img, fn.c_str());
		}
	}
}

flex_item* flexlayout(flex_item* r, std::vector<glm::vec4>& v, const glm::vec2& pos, const glm::vec2& gap)
{
	flex_item* p = 0;
	auto length = v.size();
	if (r && length)
	{
		p = new flex_item[length];
		if (p)
		{
			for (size_t i = 0; i < length; i++)
			{
				v[i].z += gap.x; v[i].w += gap.y;
				p[i].width = v[i].z;
				p[i].height = v[i].w;
				r->item_add(p + i);
			}
			r->layout();

			for (size_t i = 0; i < length; i++)
			{
				if (p[i].position != flex_item::flex_position::POS_ABSOLUTE) {
					v[i].x = p[i].frame[0] + pos.x;
					v[i].y = p[i].frame[1] + pos.y;
				}
			}
		}
	}
	return p;
}

void plane_cx::mk_layout()
{
	uplayout = false;
	if (custom_layout)return;// 自定义布局计算则退出默认而已计算
	flex_item root;
	auto ss = get_size();
	root.width = ss.x;
	root.height = ss.y;
	root.justify_content = _css.justify_content;
	root.align_content = _css.align_content;
	root.align_items = _css.align_items;
	root.wrap = _css.wrap;
	root.direction = _css.direction;

	std::vector<glm::vec4> layouts;
	std::vector<widget_base*> wbs;
	layouts.reserve(widgets.size());
	wbs.reserve(widgets.size());
	for (auto& it : widgets) {
		auto p = (widget_base*)it;
		if (p->_absolute)continue;
		layouts.push_back({ p->pos, p->size });
		wbs.push_back(p);
	}
	flex_item* c = flexlayout(&root, layouts, _lpos, _lms);
	if (c)
	{
		auto length = wbs.size();
		for (size_t i = 0; i < length; i++)
		{
			auto p = (widget_base*)wbs[i];
			auto it = layouts[i];
			glm::vec2 itss = { it.z,it.w };
			p->pos = it;
			p->pos += (itss - p->size) * _css.pos_align;
		}
		delete[] c;
	}
}
bool vht(const std::vector<widget_base*>& widgets, const glm::ivec2& p, glm::ivec2 ips, const glm::ivec2& scroll_pos) {
	bool r = false;
	for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
		auto pw = (widget_base*)*it;
		if (!pw || !pw->visible || pw->_disabled_events)continue;
		glm::vec2 mps = p; mps -= ips;
		mps -= pw->hscroll * scroll_pos;
		// 判断是否鼠标在控件上
		glm::vec4 ppos = { pw->pos,pw->size };
		auto k = check_box_cr1(mps, &ppos, 1, sizeof(glm::vec4));
		if (k.x) { r = true; }
	}
	return r;
}
bool plane_cx::hittest(const glm::ivec2& p)
{
	bool r = false;
	auto sps = get_spos();
	glm::ivec2 ips = get_pos(); auto ss = (glm::ivec2)get_size();
	glm::vec4 rc = { ips ,ips + ss };
	if (rect_includes(rc, p)) {
		if (draggable)
		{
			r = true;
		}
		else
		{
			r = vht(widgets, p, ips, sps);
			if (!r) {
				r = vht({ vertical ,horizontal }, p, ips, {});
			}
		}
	}
	//printf("%p\t%d\n", this, (int)r);
	return r;
}

size_t plane_cx::push_dragpos(const glm::ivec2& pos, const glm::ivec2& size)
{
	auto ps = drags.size();
	drag_v6 t = {};
	t.pos = pos;
	t.size = size;
	t.z = 0;
	drags.push_back(t);
	dragsp.clear();
	for (auto& it : drags) { dragsp.push_back(&it); }
	sortdg();
	return ps;
}

glm::ivec3 plane_cx::get_dragpos(size_t idx)
{
	glm::ivec3 r = (idx < drags.size()) ? glm::ivec3(drags[idx].pos, drags[idx].z) : glm::ivec3();
	r += glm::ivec3(get_spos(), 0);
	return r;
}

drag_v6* plane_cx::get_dragv6(size_t idx)
{
	return (idx < drags.size()) ? &drags[idx] : nullptr;
}

void plane_cx::sortdg()
{
	std::stable_sort(dragsp.begin(), dragsp.end(), [](const drag_v6* t1, const drag_v6* t2) { return t1->z < t2->z; });
}

gshadow_cx* plane_cx::get_gs()
{
	return ltx ? ltx->gs : nullptr;
}

void plane_cx::set_shadow(const rect_shadow_t& rs)
{
	auto gs = get_gs();
	auto rcs = gs->new_rect(rs);
	auto a = new atlas_cx();
	a->img = gs->img;
	a->img->type = 1;
	auto ss = get_size();
	ss -= rs.radius * 2;
	rcs.img_rc = { -rs.radius,-rs.radius,ss.x,ss.y };
	rcs.img_rc.x = rcs.img_rc.y = rs.radius;
	a->add(&rcs, 1);
	add_atlas(a);
}

void plane_cx::set_rss(int r)
{
	if (tv)
		tv->set_rss(r);
}

void plane_cx::on_motion(const glm::vec2& pos) {
	glm::ivec2 ps = pos;
	if (ckinc > 0)
	{
		if (draggable)
			set_pos(ps - curpos);
		//else
		//	set_scroll_pts(ps - curpos, 1);
	}

	tv->on_motion(ps - tpos);
	update(0);

}
void plane_cx::on_button(int idx, int down, const glm::vec2& pos, int clicks, int r) {
	glm::ivec2 ps = pos;
	if (idx == 1)
	{
		glm::vec4 trc = viewport;
		auto k2 = check_box_cr1(pos, &trc, 1, sizeof(glm::vec4));

		if (k2.x)
		{
			if (draggable && down == 1)
				form_move2end(form, this); // 移动窗口前面
			if (!r)
			{
				if (down == 1 && ckinc == 0)
				{
					curpos = ps - tpos;
					ckinc++;
				}
				if (on_click)
					on_click(this, down, ckinc, ps);	// 执行单击事件
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
	tv->on_button(idx, down, ps - tpos, clicks);
	update(0);
	_draw_valid = true;
}
void plane_cx::on_wheel(double x, double y)
{
	update(0);
	_draw_valid = true;
}

bool on_wpe(widget_base* pw, int type, et_un_t* ep, const glm::ivec2& ppos)
{
	bool r = false;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	auto pt = dynamic_cast<edit_tl*>(pw);
	if (!pt) {
		widget_on_event(pw, type, ep, ppos);
		if (ep->ret && t == devent_type_e::mouse_button_e)
		{
			auto p = e->b;
			if (p->down == 1)
				get_input_state(0, 1);
		}
	}
	else
	{
		pt->ppos = ppos;
		pt->on_event_e(type, ep);
	}
	return (ep->ret);
}
void plane_cx::on_event(uint32_t type, et_un_t* ep)
{
	auto e = &ep->v;
	if (!visible)return;
	auto t = (devent_type_e)type;
	glm::ivec2 vgpos = viewport;
	int r1 = 0;
	auto ppos = get_pos();
	auto sps = get_spos();
	_hover_eq.w = type;
	widget_base* hpw = 0;
	if (t == devent_type_e::mouse_move_e)
	{
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y };
		glm::vec4 trc = viewport;
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

		event_wts.clear();
		event_wts1.clear();
		if (horizontal) {
			horizontal->bst& (int)BTN_STATE::STATE_HOVER ? event_wts.push_back(horizontal) : event_wts1.push_back(horizontal);//水平滚动条
		}
		if (vertical) {
			vertical->bst& (int)BTN_STATE::STATE_HOVER ? event_wts.push_back(vertical) : event_wts1.push_back(vertical);//垂直滚动条
		}
		for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
			if ((*it)->bst & (int)BTN_STATE::STATE_HOVER)
				event_wts.push_back(*it);
			else
				event_wts1.push_back(*it);
		}
		auto length = event_wts.size();
		{
			// 生成鼠标离开消息
			for (size_t i = 1; i < length; i++) {
				auto pw = event_wts[i];
				if (!pw || !pw->visible || pw->_disabled_events)continue;
				auto vpos = sps * pw->hscroll;
				auto p = e->m;
				glm::ivec2 mps = { p->x,p->y }; mps -= ppos + vpos;
				bool isd = pw->cmpos == mps;
				pw->cmpos = mps;
				pw->bst &= ~(int)BTN_STATE::STATE_HOVER;
				// 鼠标离开
				pw->on_mevent((int)event_type2::on_leave, mps);
				if (pw->mevent_cb) {
					pw->mevent_cb(pw, (int)event_type2::on_leave, mps);
				}
			}
		}

	}
	else
	{
		int icc = 0;
		auto length = event_wts.size();
		for (size_t i = 0; i < length; i++)
		{
			auto pw = event_wts[i];
			icc++;
			if (!pw || !pw->visible || pw->_disabled_events)continue;
			auto vpos = sps * pw->hscroll;
			on_wpe(pw, type, ep, ppos + vpos);
			if (ep->ret) {
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
				auto vpos = sps * pw->hscroll;
				on_wpe(pw, type, ep, ppos + vpos);
				if (ep->ret) {
					hpw = pw; break;
				}
			}
		}
	}
	if (!ep->ret)
		ep->ret = r1;
	switch (t)
	{
	case devent_type_e::mouse_move_e:
	{
		auto length = event_wts.size();
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y };
		on_motion(mps);
		_hover_eq.z = (length > 0) ? 1 : 0;// 悬停准备
		if (ckinc > 0)
		{
			for (auto& it : drags)
			{
				if (it.ck > 0)
				{
					it.pos = mps - it.tp - ppos;	// 处理拖动坐标
					it.cp1 = mps - ppos;
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
		on_button(p->button, p->down, mps, p->clicks, ep->ret);

		if (p->button == 1) {
			if (p->down == 1) {
				mps -= ppos;
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

#endif // !NO_EDIT

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

image_btn::image_btn() :widget_base(WIDGET_TYPE::WT_IMAGE_BTN)
{

}
image_btn::~image_btn()
{
}
color_btn::color_btn() :widget_base(WIDGET_TYPE::WT_COLOR_BTN)
{
}
color_btn::~color_btn()
{
}
gradient_btn::gradient_btn() :widget_base(WIDGET_TYPE::WT_GRADIENT_BTN)
{
}
gradient_btn::~gradient_btn()
{
}
radio_tl::radio_tl() :widget_base(WIDGET_TYPE::WT_RADIO)
{
}
checkbox_tl::checkbox_tl() :widget_base(WIDGET_TYPE::WT_CHECKBOX)
{
}
checkbox_tl::~checkbox_tl()
{
}
switch_tl::switch_tl() :widget_base(WIDGET_TYPE::WT_SWITCH)
{
}
switch_tl::~switch_tl()
{
}
progress_tl::progress_tl() :widget_base(WIDGET_TYPE::WT_PROGRESS)
{
}
progress_tl::~progress_tl()
{
}
slider_tl::slider_tl() :widget_base(WIDGET_TYPE::WT_SLIDER)
{
}
slider_tl::~slider_tl()
{
}
colorpick_tl::colorpick_tl() :widget_base(WIDGET_TYPE::WT_COLORPICK)
{
}
colorpick_tl::~colorpick_tl()
{
}
scroll_bar::scroll_bar() :widget_base(WIDGET_TYPE::WT_SCROLL_BAR)
{
}
scroll_bar::~scroll_bar()
{
}


bool image_btn::on_mevent(int type, const glm::vec2& mps)
{
	return false;
}

bool image_btn::update(float) {
	return false;
}
void image_btn::draw(cairo_t* g) {

}

btn_cols_t* color_btn::set_btn_color_bgr(size_t idx)
{
	static std::vector<std::array<uint32_t, 8>> bcs = { { 0xFFffffff, 0xFF409eff, 0xFF409eff, 0xFF66b1ff, 0xFFe6e6e6, 0xFF0d84ff, 0xFF0d84ff ,0 },
		{ 0xFFffffff, 0xFF67c23a, 0xFF67c23a, 0xFF85ce61, 0xFFe6e6e6, 0xFF529b2e, 0xFF529b2e ,0},
		{ 0xFFffffff, 0xFF909399, 0xFF909399, 0xFFa6a9ad, 0xFFe6e6e6, 0xFF767980, 0xFF767980 ,0},
		{ 0xFFffffff, 0xFFe6a23c, 0xFFe6a23c, 0xFFebb563, 0xFFe6e6e6, 0xFFd48a1b, 0xFFd48a1b ,0 },
		{ 0xFFffffff, 0xFFf56c6c, 0xFFf56c6c, 0xFFf78989, 0xFFe6e6e6, 0xFFf23c3c, 0xFFf23c3c ,0 }
	};
	auto ret = (btn_cols_t*)((idx < bcs.size()) ? bcs[idx].data() : nullptr);
	if (ret)
	{
		bgr = 1;
		pdc = *ret;
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

void gradient_btn::draw(cairo_t* g)
{
	auto p = this;
	float x = p->pos.x, y = p->pos.y, w = p->size.x, h = p->size.y;
	int pushed = p->mPushed ? 0 : 1;
	uint32_t gradTop = p->gradTop;
	uint32_t gradBot = p->gradBot;
	uint32_t borderDark = p->borderDark;
	uint32_t borderLight = p->borderLight;
	double oa = p->opacity;
	auto ns = p->size;

	auto bc = effect == uTheme::light ? p->back_color : set_alpha_xf2(p->back_color, get_alpha_f(p->back_color));
	double rounding = p->rounding;
	glm::vec2 ns1 = { w * 0.5, h * 0.5 };
	auto nr = (int)std::min(ns1.x, ns1.y);
	if (rounding > nr)
	{
		rounding = nr;
	}
	cairo_save(g);
	cairo_translate(g, x, y);
	if (is_alpha(bc))
	{
		bc = set_alpha_f(bc, oa);
		draw_rectangle(g, { thickness,thickness, w - thickness, h - thickness * 2 }, rounding);
		set_color(g, bc);
		cairo_fill(g);
	}
	if (p->mPushed) {
		gradTop = set_alpha_f(gradTop, 0.8f);
		gradBot = set_alpha_f(gradBot, 0.8f);
	}
	else {
		double v = 1 - get_alpha_f(p->back_color);
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
	glm::vec2 rct = { w - thickness, h - thickness * 2 };
	glm::vec4 gtop = to_c4(gradTop);
	glm::vec4 gbot = to_c4(gradBot);

	cairo_save(g);
	if (effect == uTheme::dark)
		cairo_translate(g, thickness, thickness);
	else if (p->mPushed)
		cairo_translate(g, thickness, thickness);
	paint_shadow(g, 0, rct.y, rct.x, rct.y, gtop, gbot, 0, rounding);// 垂直方向
	cairo_restore(g);
	// 渲染标签

	glm::vec2 ps = { thickness * 2,thickness * 2 };
	if (p->mPushed) {
		ps += thickness;
	}
	ns -= thickness * 4;
	glm::vec4 rc = { ps, ns };

#if 1
	text_style_t st = {};
	st.font = 0;
	st.text_align = p->text_align;
	st.font_size = p->font_size;
	st.text_color = p->text_color;
	st.shadow_pos = { thickness, thickness };
	st.text_color_shadow = text_color_shadow;
	{
		int font = 0;
		glm::vec2 text_align = { 0.0,0.5 };
		glm::vec2 shadow_pos = { 1.0,1.0 };
		int font_size = 18;
		uint32_t text_color = 0xffffffff;
		uint32_t text_color_shadow = 0;
	};
	draw_text(g, ltx, p->str.c_str(), -1, rc, &st);

#else

	ltx->tem_rtv.clear();
	ltx->build_text(0, rc, text_align, p->str.c_str(), -1, p->font_size, ltx->tem_rtv);
	ltx->update_text();
	if (text_color_shadow)
	{
		cairo_as _aa_(g);
		cairo_translate(g, thickness, thickness);
		ltx->draw_text(g, ltx->tem_rtv, text_color_shadow);
	}
	ltx->draw_text(g, ltx->tem_rtv, p->text_color);

#endif // 1
	// 边框
	cairo_set_line_width(g, thickness);
	set_color(g, borderLight);
	draw_rectangle(g, { 0.5f,  (p->mPushed ? 0.5f : 1.5f), w, h - (p->mPushed ? 0.0f : 1.0f) }, rounding);
	cairo_stroke(g);
	set_color(g, borderDark);
	draw_rectangle(g, { 0.5f,  0.5f, w, h - 0.5 }, rounding);
	cairo_stroke(g);
	cairo_restore(g);
}


const char* gradient_btn::c_str()
{
	return str.c_str();
}

void gradient_btn::init(glm::ivec4 rect, const std::string& text, uint32_t back_color, uint32_t text_color)
{
	auto p = this;
	auto& info = *p;
	info.pos = { rect.x, rect.y };
	info.size = { rect.z, rect.w };
	info.rounding = 4;
	info.back_color = back_color;
	info.text_color = text_color;
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
	if (bst == _old_bst)return false;
	_old_bst = bst;
	auto& info = *p;

	// (sta & hz::BTN_STATE::STATE_FOCUS)
	uint32_t gradTop = 0xff4a4a4a;
	uint32_t gradBot = 0xff3a3a3a;

	info.mPushed = (bst & (int)BTN_STATE::STATE_ACTIVE);
	info.mMouseFocus = (bst & (int)BTN_STATE::STATE_HOVER);
	if (bst & (int)BTN_STATE::STATE_DISABLE)
		info.mEnabled = false;
	if (info.mPushed) {
		gradTop = 0xff292929;
		gradBot = 0xff1d1d1d;
	}
	else if (info.mMouseFocus && info.mEnabled) {
		gradTop = 0x80404040;
		gradBot = 0x80303030;
	}
	info.gradTop = gradTop;
	info.gradBot = gradBot;
	return true;
}



bool color_btn::update(float delta)
{
	dtime += delta;
	auto dt = dtime;
	if (dt > 0.150) {
		if (str != text)
			text = str;
		dtime = 0;
	}
	else
	{
		if (bst == _old_bst)return false;
	}
	_old_bst = bst;
	auto p = this;
	btn_cols_t* pdc = &p->pdc;
	p->_disabled = (bst & (int)BTN_STATE::STATE_DISABLE);
	if (p->_disabled)
	{
		p->hover = false;
		p->dfill = pdc->background_color;
		p->dcol = pdc->border_color;
		if (p->effect == uTheme::dark)
		{
			p->dcol = 0;
			p->dtext_color = (p->text_color) ? p->text_color : pdc->font_color;
		}
		if (p->effect == uTheme::light)
		{
			p->dcol = set_alpha_xf(p->dcol, p->light * 3);
			p->dfill = set_alpha_xf(p->dfill, p->light);
			p->dtext_color = (p->text_color) ? p->text_color : pdc->border_color;
		}
		if (p->effect == uTheme::plain)
		{
			p->dfill = 0;
			p->dtext_color = (p->text_color) ? p->text_color : pdc->border_color;
		}
		p->dcol = set_alpha_x(p->dcol, p->disabled_alpha);
		p->dfill = set_alpha_x(p->dfill, p->disabled_alpha);
		p->dtext_color = set_alpha_x(p->dtext_color, p->disabled_alpha);
	}
	else
	{
		bool isdown = mPushed = (bst & (int)BTN_STATE::STATE_ACTIVE);
		p->hover = (bst & (int)BTN_STATE::STATE_HOVER);
		if (isdown)
		{
			p->dfill = pdc->active_background_color;
			p->dcol = pdc->active_border_color;
			if (p->effect == uTheme::plain)
			{
				p->dfill = set_alpha_xf(p->dcol, p->light);
			}
			else {
				p->dtext_color = (p->text_color) ? p->text_color : pdc->active_font_color;
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
				p->dtext_color = (p->text_color) ? p->text_color : pdc->font_color;
			}
		}
		else {
			p->dfill = pdc->background_color;
			p->dcol = pdc->border_color;
			if (p->effect == uTheme::dark)
			{
				p->dcol = 0;
				p->dtext_color = (p->text_color) ? p->text_color : pdc->font_color;
			}
			if (p->effect == uTheme::light)
			{
				p->dcol = set_alpha_xf(p->dcol, p->light * 3);
				p->dfill = set_alpha_xf(p->dfill, p->light);
				p->dtext_color = (p->text_color) ? p->text_color : pdc->border_color;
			}
			if (p->effect == uTheme::plain)
			{
				p->dfill = 0;
				p->dtext_color = (p->text_color) ? p->text_color : pdc->border_color;
			}
		}
	}
	return true;
}

void color_btn::draw(cairo_t* g)
{
	auto p = this;
	auto ns = p->size;
	static int bid = 1234;
	if (id == bid)
	{
		id = id;
	}
	cairo_as _as_(g);
	cairo_translate(g, p->pos.x, p->pos.y);
	if (p->dfill)
	{
		if (p->_circle)
		{
			auto sp = p->pos;
			auto r = lround(p->size.y * 0.5);
			sp += r;
			draw_circle(g, sp, r);
		}
		else
		{
			draw_rectangle(g, { 0.5,0.5, p->size }, p->rounding);
		}
		fill_stroke(g, p->dfill, 0, 0, bgr);
	}
	// 渲染标签
	glm::vec2 ps = { thickness * 2, thickness * 2 };
	if (p->mPushed) {
		ps += pushedps;
	}

	ns -= thickness * 4;

	glm::vec4 rc = { ps, ns };

	text_style_t st = {};
	st.font = 0;
	st.text_align = p->text_align;
	st.font_size = p->font_size;
	st.text_color = p->text_color;

	draw_text(g, ltx, p->str.c_str(), -1, rc, &st);

	/*	ltx->tem_rtv.clear();
		ltx->build_text(0, rc, text_align, p->str.c_str(), -1, p->font_size, ltx->tem_rtv);
		ltx->update_text();
		ltx->draw_text(g, ltx->tem_rtv, p->text_color);*/
		//auto rc = draw_text_align(g, p->str.c_str(), ps, ns, text_align, p->text_color, p->family.c_str(), p->font_size);

	if (p->dcol)
	{
		if (p->_circle)
		{
			auto sp = p->pos;
			auto r = lround(p->size.y * 0.5);
			sp += r;
			draw_circle(g, sp, r);
		}
		else
		{
			draw_rectangle(g, { 0.5,0.5, p->size }, p->rounding);
		}
		fill_stroke(g, 0, p->dcol, p->thickness, bgr);
	}

	//if ((bst & (int)BTN_STATE::STATE_HOVER))
	//{
	//	draw_rectangle(g, { 0,0,size.x,size.y }, p->rounding);
	//	fill_stroke(g, 0, 0x80ff8000, 1, 0);
	//}
}









#if 1
// 通用控件鼠标事件处理 type有on_move/on_scroll/on_drag/on_down/on_up/on_click/on_dblclick/on_tripleclick
bool widget_on_move(widget_base* wp, uint32_t type, et_un_t* ep, const glm::vec2& pos) {
	bool hover = false;
	if (!wp)return hover;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	if (t == devent_type_e::mouse_move_e)
	{
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y }; mps -= pos;
		wp->mmpos = mps;
		// 判断是否鼠标进入 
		glm::vec4 trc = { wp->pos  ,wp->size };
		auto k = check_box_cr1(mps, &trc, 1, sizeof(glm::vec4));
		if (k.x) {
			bool hoverold = wp->bst & (int)BTN_STATE::STATE_HOVER;
			wp->bst |= (int)BTN_STATE::STATE_HOVER;   hover = true;
			if (!(wp->bst & (int)BTN_STATE::STATE_ACTIVE))// 不是鼠标则独占
				ep->ret = 1;
			if (!hoverold)
			{
				// 鼠标进入
				wp->on_mevent((int)event_type2::on_enter, mps);
				if (wp->mevent_cb) {
					wp->mevent_cb(wp, (int)event_type2::on_enter, mps);
				}
			}
		}
		else {
			if (wp->bst & (int)BTN_STATE::STATE_HOVER)
			{
				wp->bst &= ~(int)BTN_STATE::STATE_HOVER;
				// 鼠标离开
				wp->on_mevent((int)event_type2::on_leave, mps);
				if (wp->mevent_cb) {
					wp->mevent_cb(wp, (int)event_type2::on_leave, mps);
				}
			}
		}

		{
			if (wp->bst & (int)BTN_STATE::STATE_HOVER)
			{
				wp->on_mevent((int)event_type2::on_move, mps);
				if (wp->mevent_cb)
				{
					wp->mevent_cb(wp, (int)event_type2::on_move, mps);
				}
			}
			if (wp->bst & (int)BTN_STATE::STATE_ACTIVE) {
				auto dps = mps - wp->curpos;
				wp->on_mevent((int)event_type2::on_drag, dps);		// 拖动事件
				if (wp->mevent_cb)
				{
					wp->mevent_cb(wp, (int)event_type2::on_drag, dps);		// 拖动事件
				}
			}
		}
	}
	return hover;
}

void widget_on_event(widget_base* wp, uint32_t type, et_un_t* ep, const glm::vec2& pos) {
	if (!wp)return;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	switch (t)
	{
	case devent_type_e::mouse_move_e:
		widget_on_move(wp, type, ep, pos);
		break;
	case devent_type_e::mouse_button_e:
	{
		auto p = e->b;
		glm::ivec2 mps = { p->x,p->y }; mps -= pos;
		bool isd = wp->cmpos == mps;
		wp->cmpos = mps;
		if (wp->bst & (int)BTN_STATE::STATE_HOVER) {
			if (p->down == 1)
			{
				ep->ret = 1;
			}
			if (p->button == 1) {
				if (p->down == 1) {
					wp->bst |= (int)BTN_STATE::STATE_ACTIVE;
					wp->curpos = mps - (glm::ivec2)wp->pos;
					wp->cks = 0;
					wp->on_mevent((int)event_type2::on_down, mps);
					if (wp->mevent_cb) { wp->mevent_cb(wp, (int)event_type2::on_down, mps); }
				}
				else {
					if ((wp->bst & (int)BTN_STATE::STATE_ACTIVE) && (isd || !wp->has_drag))
					{
						wp->cks = p->clicks;
						wp->on_mevent((int)event_type2::on_up, mps);
						if (wp->mevent_cb) {
							wp->mevent_cb(wp, (int)event_type2::on_up, mps);
						}
						int tc = (int)event_type2::on_click; //左键单击
						if (p->clicks == 2) { tc = (int)event_type2::on_dblclick; }
						else if (p->clicks == 3) { tc = (int)event_type2::on_tripleclick; }
						wp->_clicks = p->clicks;
						wp->on_mevent(tc, mps);
						if (wp->mevent_cb) {
							wp->mevent_cb(wp, tc, mps);
						}
						if (wp->click_cb)
						{
							wp->click_cb(wp, p->clicks);
						}
					}
					wp->bst &= ~(int)BTN_STATE::STATE_ACTIVE;
				}
			}
		}
		if (p->down == 0) {
			wp->bst &= ~(int)BTN_STATE::STATE_ACTIVE;
			wp->bst |= (int)BTN_STATE::STATE_NOMAL;
			wp->on_mevent((int)event_type2::mouse_up, mps);
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
		if (wp->bst & (int)BTN_STATE::STATE_HOVER || wp->has_hover_sc)
		{
			ep->ret = wp->on_mevent((int)event_type2::on_scroll, mps);
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
void send_hover(widget_base* wp, const glm::vec2& mps) {

	wp->on_mevent((int)event_type2::on_hover, mps);
	if (wp->mevent_cb)
	{
		wp->mevent_cb(wp, (int)event_type2::on_hover, mps);
	}
}


#endif // 1




render_lv::render_lv()
{
}

render_lv::~render_lv()
{
}

void render_lv::set_style(const std::string& family, int fs, uint32_t tcolor)
{
	familys = family;
	if (fs > 0)
		fontsize = fs;
	text_color = tcolor;
}

void render_lv::binding(plane_cx* p)
{
	plane = p;
}


#endif // 1



template<class T>
void free_obt(T*& p) {
	if (p) {
		delete p; p = 0;
	}
}





// todo ui



void draw_radios(cairo_t* cr, radio_info_t* p, radio_style_t* ps)
{
	if (ps->radius > 0) {
		draw_circle(cr, p->pos, ps->radius);
		if (p->value || p->swidth > 0)
			fill_stroke(cr, ps->col, 0, ps->thickness, 0);
		else
			fill_stroke(cr, 0, ps->line_col, ps->thickness, 0);
	}
	if (p->swidth > 0) {
		draw_circle(cr, p->pos, p->swidth);
		fill_stroke(cr, ps->innc, 0, ps->thickness, 0);
	}
}


// check打勾
void drawCheckMark(cairo_t* cr, glm::vec2 pos, uint32_t col, float sz1, bool mixed)
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
		cairo_move_to(cr, ps.x, ps.y);
		ps.x += sz1 - thickness;
		cairo_line_to(cr, ps.x, ps.y);
	}
	else {
		cairo_move_to(cr, bx - third, by - third);
		cairo_line_to(cr, bx, by);
		cairo_line_to(cr, bx + third * 2.0f, by - third * 2.0f);
	}
	fill_stroke(cr, 0, col, thickness, 0);
}
void draw_checkbox(cairo_t* cr, check_style_t* p, checkbox_info_t* pn)
{
	if (!p)return;
	auto cc = p->check_col;
	glm::ivec2 ps = pn->pos;
	draw_rectangle(cr, { ps.x,ps.y,p->square_sz,p->square_sz }, p->rounding);
	if (pn->value || pn->new_alpha > 0)
		fill_stroke(cr, p->fill, p->col, p->thickness, 0);
	else
		fill_stroke(cr, 0, p->line_col, p->thickness, 0);
	const float pad = std::max(1.0f, floor(p->square_sz / 6.0f));
	if (pn->new_alpha > 0)
	{
		cc = set_alpha_f(cc, pn->new_alpha);
		drawCheckMark(cr, (glm::vec2)ps + glm::vec2(pad, pad), cc, p->square_sz - pad * 2.0f, pn->mixed);
	}
}



radio_tl::~radio_tl()
{
	if (gr && gr->ct > 0) {
		gr->ct--;
		if (gr->ct <= 0)
			delete gr;
	}
	gr = 0;
}

void radio_tl::bind_ptr(bool* p)
{
}

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

bool radio_tl::on_mevent(int type, const glm::vec2& mps)
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
		if (size.x <= 0) {
			size.x = style.radius * 2;
		}
		if (size.y <= 0) {
			size.y = style.radius * 2;
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

					_old_bst = bst;
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

void radio_tl::draw(cairo_t* cr)
{
	auto p = this;
	if (cr && p) {
		cairo_as _as_(cr);
		glm::ivec2 poss = p->pos;
		poss.y += size.y * 0.5 - style.radius;
		cairo_translate(cr, poss.x + 0.5, poss.y + 0.5);
		int x = 0;
		{
			auto& it = v;
			it.pos = {};
			it.pos.x = x * style.radius * 5;
			it.pos += style.radius;
			draw_radios(cr, &it, &style);
			x++;
		}
	}
}

void checkbox_tl::bind_ptr(bool* p)
{
}

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

bool checkbox_tl::on_mevent(int type, const glm::vec2& mps)
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
		if (size.x <= 0) {
			size.x = style.square_sz;
		}
		if (size.y <= 0) {
			size.y = style.square_sz;
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
					_old_bst = bst;
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

void checkbox_tl::draw(cairo_t* cr)
{
	auto p = this;
	if (cr && p) {
		cairo_as _as_(cr);
		glm::ivec2 poss = p->pos;
		poss.y += (size.y - style.square_sz) * 0.5;
		cairo_translate(cr, poss.x + 0.5, poss.y + 0.5);
		int x = 0;
		{
			auto& it = v;
			it.pos.x = x * p->style.square_sz * 2.5;
			draw_checkbox(cr, &p->style, &it);
			x++;
		}
	}
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

bool switch_tl::on_mevent(int type, const glm::vec2& mps)
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
				_old_bst = bst;
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

void switch_tl::draw(cairo_t* cr)
{
	cairo_as _as_(cr);
	glm::ivec2 poss = pos;
	auto h = height;
	auto fc = h * cv * 0.5;
	auto ss = h * wf;
	if (size.x <= 0) {
		size.x = ss;
	}
	if (size.y <= 0) {
		size.y = h;
	}
	poss.x += (size.x - ss) * 0.5;
	poss.y += (size.y - height) * 0.5;
	cairo_translate(cr, poss.x, poss.y);
	draw_rectangle(cr, { 0.5,0.5, ss, h }, h * 0.5);
	fill_stroke(cr, dcol, 0, 0, 0);
	glm::vec2 cp = {};
	{
		auto ps = h * 0.5;
		cp.x += (ss - h) * cpos + h * 0.5;
		cp.y += ps;
		draw_circle(cr, cp, fc);
		fill_stroke(cr, color.z, 0, 0, 0);
	}
}

void progress_tl::set_value(double b)
{
	if (b < 0)b = 0;
	if (b > 1)b = 1;
	value = b;
	if (format.size() && ltx)
	{
		double k = get_v();
		std::string vv;
		text = pg::to_string(k) + format;
		width = size.x;
		if (text.size() && !text_inside) {
			auto rk = ltx->get_text_rect(0, text.c_str(), -1, font_size);
			size.x = width + rk.x + rounding * 0.5;
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

bool progress_tl::on_mevent(int type, const glm::vec2& mps)
{
	auto et = (event_type2)type;
	return false;
}

bool progress_tl::update(float delta)
{
	return false;
}

void progress_tl::draw(cairo_t* cr)
{
	cairo_as _as_(cr);
	glm::ivec2 poss = pos;
	glm::ivec2 ss = size;
	ss.x = width;
	cairo_translate(cr, poss.x, poss.y);
	draw_rectangle(cr, { 0.5,0.5, ss.x, ss.y }, rounding);
	fill_stroke(cr, color.y, 0, 0, 0);
	double xx = ss.x * value;
	int kx = 0;
	int r = rounding;
	if (xx > 0)
	{
		cairo_as _as_a(cr);
		if (xx < rounding * 2)
		{
			draw_rectangle(cr, { 0,0, xx, ss.y }, r);
			cairo_clip(cr);
			xx = r * 2;
			kx = 1;
		}
		draw_rectangle(cr, { 0.5,0.5, xx, ss.y }, r);
		fill_stroke(cr, color.x, 0, 0, 0);
	}
	if (text.size()) {
		auto rk = ltx->get_text_rect(0, text.c_str(), -1, font_size);
		if (text_inside) {
			ss.x = xx;
			ss.x -= r * 0.5;
		}
		else {
			ss.x = size.x;
		}
		if (kx) {

			ss.x += rk.x;
		}
		if (right_inside) {
			ss.x = size.x - r * 0.5;
		}
		glm::vec2 ta = { 1,0.5 };
		glm::vec4 rc = { 0, 0, ss };
		text_style_t st = {};
		st.font = 0;
		st.text_align = ta;
		st.font_size = font_size;
		st.text_color = text_color;
		draw_text(cr, ltx, text.c_str(), -1, rc, &st);

	}
}



void slider_tl::bind_ptr(double* p)
{
}

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

bool slider_tl::on_mevent(int type, const glm::vec2& mps)
{
	auto et = (event_type2)type;
	glm::ivec2 poss = mps - pos;
	glm::ivec2 ss = size;
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

void slider_tl::draw(cairo_t* cr)
{
	cairo_as _as_(cr);
	glm::ivec2 poss = pos;
	glm::ivec2 ss = size;
	cairo_translate(cr, poss.x, poss.y);
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
	draw_rectangle(cr, brc, rounding);
	fill_stroke(cr, color.y, 0, 0, 0);
	if (xx >= 0)
	{
		{
			cairo_as _as_a(cr);
			draw_rectangle(cr, crc, r);
			fill_stroke(cr, color.x, 0, 0, 0);
		}
		if (sl.x > 0) {
			draw_circle(cr, spos, sl.x);
			fill_stroke(cr, sl.y, color.x, thickness, 0);
		}
	}
}







// H in [0,360)
// S, V, R, G, B in [0,1]
glm::vec4 convertHSVtoRGB(const glm::vec4& hsv)
{
	double H = hsv.x * 360.0, S = hsv.y, V = hsv.z;
	double R, G, B;
	int Hi = int(floor(H / 60.)) % 6;
	double f = H / 60. - Hi;
	double p = V * (1 - S);
	double q = V * (1 - f * S);
	double t = V * (1 - (1 - f) * S);
	switch (Hi) {
	case 0: R = V, G = t, B = p; break;
	case 1: R = q, G = V, B = p; break;
	case 2: R = p, G = V, B = t; break;
	case 3: R = p, G = q, B = V; break;
	case 4: R = t, G = p, B = V; break;
	case 5: R = V, G = p, B = q; break;
	}
	return { R,G,B,hsv.w };
}



// Convert rgb floats ([0-1],[0-1],[0-1]) to hsv floats ([0-1],[0-1],[0-1]), from Foley & van Dam p592
// Optimized http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
glm::vec4 RGBtoHSV(glm::u8vec4* c)
{
	double r = c->x / 255.0, g = c->y / 255.0, b = c->z / 255.0, a = c->w / 255.0;
	double K = 0.;
	if (g < b)
	{
		std::swap(g, b);
		K = -1.;
	}
	if (r < g)
	{
		std::swap(r, g);
		K = -2. / 6. - K;
	}
	const float chroma = r - (g < b ? g : b);
	glm::vec4 hsv = { fabs(K + (g - b) / (6.0 * chroma + 1e-20f)), chroma / (r + 1e-20f), r ,a };
	return hsv;
}
// Convert hsv floats ([0-1],[0-1],[0-1]) to rgb floats ([0-1],[0-1],[0-1]), from Foley & van Dam p593
// also http://en.wikipedia.org/wiki/HSL_and_HSV
void HSVtoRGB(const glm::vec4& hsv, glm::vec4& otc)
{
	float h = hsv.x, s = hsv.y, v = hsv.z;
	otc.w = hsv.w;
	if (s == 0.0f)
	{
		// gray
		otc.x = otc.y = otc.z = v;
		return;
	}
	h = fmod(h, 1.0f) / (60.0f / 360.0f);
	int   i = (int)h;
	float f = h - (float)i;
	float p = v * (1.0f - s);
	float q = v * (1.0f - s * f);
	float t = v * (1.0f - s * (1.0f - f));

	switch (i)
	{
	case 0: otc.x = v; otc.y = t; otc.z = p; break;
	case 1: otc.x = q; otc.y = v; otc.z = p; break;
	case 2: otc.x = p; otc.y = v; otc.z = t; break;
	case 3: otc.x = p; otc.y = q; otc.z = v; break;
	case 4: otc.x = t; otc.y = p; otc.z = v; break;
	case 5: default: otc.x = v; otc.y = p; otc.z = q; break;
	}
}

void colorpick_tl::init(uint32_t c, int w, int h, bool alpha)
{
	set_color2hsv(c);
	width = w;
	height = h;
	if (height < font_size)
		height = ltx->get_lineheight(0, font_size);
	h = height + step;
	cpx = height * 2.5;
	int minw = cpx + step * 2;
	if (width < minw) {
		width = minw + h;
	}
	size.x = width;
	int hn = 4;
	if (alpha)hn++;
	size.y = h * hn;
}

uint32_t colorpick_tl::get_color()
{
	glm::vec4 hc = {};
	HSVtoRGB(hsv, hc);
	//auto hc1 = convertHSVtoRGB(hsv);
	glm::u8vec4 c = { (int)(hc.x * 255), (int)(hc.y * 255), (int)(hc.z * 255), (int)(hc.w * 255) };
	return *((uint32_t*)&c);
}

void colorpick_tl::set_color2hsv(uint32_t c)
{
	color.y = color.x;
	color.x = c;
	hsv = RGBtoHSV((glm::u8vec4*)&c);
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
void colorpick_tl::set_posv(const glm::ivec2& poss)
{
	double htp = height + step;
	double cw0 = cw - step, x = poss.x;
	if (x < 0) { x = 0; }
	double xf = (double)poss.x / cw0;
	if (xf > 1)xf = 1;
	if (xf < 0)xf = 0;
	int x4 = alpha ? 4 : 3;
	if (dx >= 0 && dx < x4)
	{
		hsv[dx] = xf;
	}
}
bool colorpick_tl::on_mevent(int type, const glm::vec2& mps)
{
	auto et = (event_type2)type;
	glm::ivec2 poss = mps - pos;
	glm::ivec2 ss = size;
	poss.x -= cpx;
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
		hsvstr = "H:" + std::to_string(h) + (char*)u8"°";
		hsvstr += "\nS:" + std::to_string(s) + "%";
		hsvstr += "\nV:" + std::to_string(v) + "%";
		if (alpha)
		{
			hsvstr += "\nA:" + std::to_string(a) + "%";
		}
		else { hsv.w = 1; }
		glm::vec4 hc = {};
		HSVtoRGB(hsv, hc);
		char buf[256] = {};
		glm::ivec4 c = { (int)(hc.x * 255), (int)(hc.y * 255), (int)(hc.z * 255), (int)(hc.w * 255) };
		sprintf(buf, "#%02X%02X%02X%02X %d,%d,%d,%d", c.x, c.y, c.z, c.w, c.x, c.y, c.z, c.w);
		colorstr = buf;
		if (ltx)
		{
			glm::ivec2 ss = size;
			glm::vec2 ta = { 0.0, 0.0 };
			glm::vec4 rc = { step, height + step, ss };
			cw = ss.x - cpx - step * 2;
			rc.w -= rc.y;
			tem_rtv.clear();
			//auto rk = ltx->get_text_rect(1, "H:00%%"/* hsvstr.c_str()*/, -1, font_size);
			ltx->heightline = rc.y;//设置固定行高
			rc.y += step;
			ltx->build_text(1, rc, ta, hsvstr.c_str(), -1, font_size, tem_rtv);
			rc.y = 0; rc.x += cpx;
			rc.z = cw;
			rc.w = height; ta.y = 0.5;
			ltx->heightline = 0; // 使用默认行高
			ltx->build_text(1, rc, ta, colorstr.c_str(), -1, font_size, tem_rtv);
			ltx->heightline = 0;
			assert(cw > 0);
		}
		if (on_change_cb) {
			on_change_cb(this, get_color());
		}
	}
	return false;
}
void draw_grid_fill(cairo_t* cr, const glm::vec2& ss, const glm::ivec2& cols, int width)
{
	int x = fmod(ss.x, width);
	int y = fmod(ss.y, width);
	int xn = ss.x / width;
	int yn = ss.y / width;
	if (x > 0)xn++;
	if (y > 0)yn++;

	cairo_as _as_a(cr);
	draw_rectangle(cr, { 0,0,ss.x,ss.y }, 0);
	cairo_clip(cr);
	for (size_t i = 0; i < yn; i++)
	{
		for (size_t j = 0; j < xn; j++)
		{
			bool k = (j & 1);
			if (!(i & 1))
				k = !k;
			auto c = cols[k];
			draw_rectangle(cr, { j * width,i * width,width,width }, 0);
			set_color(cr, c);
			cairo_fill(cr);
		}
	}
}
void draw_linear(cairo_t* cr, const glm::vec2& ss, const glm::vec4* cols, int count)
{
	cairo_pattern_t* gr = cairo_pattern_create_linear(0, 0, ss.x, ss.y);
	double n = count - 1;
	for (size_t i = 0; i < count; i++)
	{
		auto color = cols[i];
		cairo_pattern_add_color_stop_rgba(gr, i / n, color.x, color.y, color.z, color.w);
	}
	draw_rectangle(cr, { 0,0,ss.x,ss.y }, 0);
	cairo_set_source(cr, gr);
	cairo_fill(cr);
	cairo_pattern_destroy(gr);
}
void colorpick_tl::draw(cairo_t* cr)
{
	cairo_as _as_a(cr);
	glm::ivec2 poss = pos;
	glm::ivec2 ss = size;
	cairo_translate(cr, poss.x, poss.y);
	if (ltx)
	{
		glm::vec2 ta = { 0, 0.5 };
		glm::vec4 rc = { 0, height + step, ss };
		rc.w -= rc.y;
		ltx->draw_text(cr, tem_rtv, text_color);
	}
	float style_alpha8 = 1;
	//uint32_t col_hues[] = { 0xff0000ff,0xff00ffff,0xff00ff00,0xffffff00,0xffff0000,0xffff00ff,0xff0000ff };
	const glm::vec4 col_hues[6 + 1] = { glm::vec4(1,0,0,style_alpha8), glm::vec4(1,1,0,style_alpha8), glm::vec4(0,1,0,style_alpha8)
		, glm::vec4(0,1,1,style_alpha8), glm::vec4(0,0,1,style_alpha8), glm::vec4(1,0,1,style_alpha8), glm::vec4(1,0,0,style_alpha8) };
	int yh = height + step;
	glm::vec4 hc = {};
	HSVtoRGB(hsv, hc);
	{
		glm::vec4 cc = {};
		draw_grid_fill(cr, { cpx, height }, { -1,0xffdfdfdf }, height * 0.5);// 填充格子
		draw_rectangle(cr, { 0,0,cpx, height }, 0);
		set_source_rgba(cr, hc);
		cairo_fill(cr);// 填充当前颜色
	}
	{
		cairo_translate(cr, cpx, yh);
		draw_linear(cr, { cw,height }, col_hues, 7);	// H 
		glm::ivec4 rcc = { (cw - step) * hsv.x,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		draw_rectangle(cr, rcf, 0);
		fill_stroke(cr, -1, bc_color, thickness, false);
	}
	{
		cairo_translate(cr, 0, yh);
		glm::vec4 cc[] = { {1,1,1,1}, hc };
		draw_grid_fill(cr, { cw, height }, { -1,-1 }, height * 0.5);// 背景色
		draw_linear(cr, { cw, height }, cc, 2);	// S
		glm::ivec4 rcc = { (cw - step) * hsv.y,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		draw_rectangle(cr, rcf, 0);
		fill_stroke(cr, -1, bc_color, thickness, false);
	}
	{
		cairo_translate(cr, 0, yh);
		glm::vec4 cc[] = { {0,0,0,1}, hc };
		draw_grid_fill(cr, { cw, height }, { -1,-1 }, height * 0.5);// 背景色
		draw_linear(cr, { cw, height }, cc, 2);	// V
		glm::ivec4 rcc = { (cw - step) * hsv.z,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		draw_rectangle(cr, rcf, 0);
		fill_stroke(cr, -1, bc_color, thickness, false);
	}
	{
		cairo_translate(cr, 0, yh);
		glm::vec4 cc[] = { {0,0,0,0}, {1,1,1,1} };
		draw_grid_fill(cr, { cw, height }, { -1,0xffdfdfdf }, height * 0.5);//背景色
		draw_linear(cr, { cw, height }, cc, 2);	// A
		glm::ivec4 rcc = { (cw - step) * hsv.w,-step * 0.5,step - 2, height + step };
		glm::vec4 rcf = rcc;
		rcf.x += 0.5; rcf.y += 0.5;
		draw_rectangle(cr, rcf, 0);
		fill_stroke(cr, -1, bc_color, thickness, false);
	}

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

bool scroll_bar::on_mevent(int type, const glm::vec2& mps)
{
	auto et = (event_type2)type;
	glm::ivec2 poss = mps - pos;
	glm::ivec2 ss = size;
	poss -= tps;
	int px = _dir ? 0 : 1;
	int tsm = thumb_size_m.x + tps[_dir] * 2.0;
	auto pts = poss[_dir];
	pts -= _offset;
	switch (et)
	{
	case event_type2::on_click:
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
	case event_type2::on_down:
	{
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
		if (thumb_size_m.z > 0 && ((bst & (int)BTN_STATE::STATE_HOVER) || hover_sc && (parent && parent->_hover)))
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
		set_posv(poss);
	}
	break;
	default:
		break;
	}
	return false;
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
		glm::ivec2 ss = size;
		auto pxs = (ss[px] - _rc_width) * 0.5;
		auto ss1 = ss[_dir];
		auto ss2 = pos[_dir];
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
	if (!(bst & (int)BTN_STATE::STATE_HOVER)) {
		if (!(bst & (int)BTN_STATE::STATE_ACTIVE))
		{
			_tcc = _color.y; scale_s = scale_s0.x;
		}
	}
	if ((bst & (int)BTN_STATE::STATE_ACTIVE) && _color.w) {
		_tcc = _color.w;
	}
	return r;
}

void scroll_bar::draw(cairo_t* cr)
{
	cairo_as _as_a(cr);
	glm::ivec2 poss = pos;
	glm::ivec2 ss = size;

	{
		cairo_translate(cr, poss.x, poss.y);
		// 背景
		if (!hideble || thumb_size_m.z) {
			draw_rectangle(cr, { 0,0,ss }, rounding);
			fill_stroke(cr, _color.x, 0, 0, 0);
		}
		// 滑块
		double rw = _rc_width * scale_s;
		glm::ivec4 trc = { 0,0,rw,rw };
		int px = _dir ? 0 : 1;
		auto pxs = ceil((ss[px] - rw) * 0.5);
		trc.x = pxs;
		trc.y = pxs;
		trc[_dir] = tps[_dir] + _offset;
		trc[2 + _dir] = thumb_size_m.x;
		if (thumb_size_m.z)
		{
			draw_rectangle(cr, trc, _rc_width * 0.5 * scale_s);
			fill_stroke(cr, _tcc, 0, 0, 0);
		}
	}
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
	glm::ivec2 ss = size;
	int tsm = thumb_size_m.x + tps[_dir] * 2.0;
	auto st = ss[_dir] - tsm;
	return st;
}

void scroll_bar::set_offset(int pts)
{
	glm::ivec2 ss = size;
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
	glm::ivec2 ss = size;
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


// 格子布局
grid_view::grid_view()
{
}

grid_view::~grid_view()
{
}

void grid_view::set_size(size_t x, size_t y)
{
	if (x > 0)
		_column_width.resize(x);
	if (y > 0)
		_row_height.resize(y);
	valid = true;
}

void grid_view::add_col(int width)
{
	if (width > 0)
	{
		_column_width.push_back({ width,0 });
		valid = true;
	}
}

void grid_view::add_row(int height)
{
	if (height > 0)
	{
		_row_height.push_back({ height,0 });
		valid = true;
	}
}

void grid_view::set_width(size_t idx, float v)
{
	if (idx < _column_width.size())
	{
		_column_width[idx].x = v; valid = true;
	}
}

void grid_view::set_height(size_t idx, float v)
{
	if (idx < _row_height.size())
	{
		_row_height[idx].x = v; valid = true;
	}
}

glm::vec2 grid_view::get_size()
{
	glm::vec2 r = {};
	if (valid)
	{
		for (auto& it : _column_width)
		{
			it.y = r.x;
			r.x += it.x;
		}
		for (auto& it : _row_height)
		{
			it.y = r.y;
			r.y += it.x;
		}
		_size = r; valid = false;
	}
	else { r = _size; }
	return r;
}
glm::vec4 grid_view::get(const glm::ivec2& pos)
{
	glm::vec4 r = {};
	glm::uvec2 px = pos;
	if (valid) { get_size(); }
	if (px.x < _column_width.size())
	{
		r.x = _pos.x + _column_width[px.x].y;
		r.z = _column_width[px.x].x;
	}
	if (px.y < _row_height.size())
	{
		r.y = _pos.y + _row_height[px.y].y;
		r.w = _row_height[px.y].x;
	}
	return r;
}


listview_cx::listview_cx()
{
}

listview_cx::~listview_cx()
{
	if (gv)delete gv; gv = 0;
}

void listview_cx::add_title(const column_lv& p)
{
	auto ps = _title.size();
	auto& kt = _title.emplace_back(p);
	kt.idx = ps;
	data_valid = true;
}

void listview_cx::set_view_size(const glm::ivec2& sv, const glm::ivec2& crsize)
{
	if (!gv)
		gv = new grid_view();
	rc = crsize;
	set_size(sv);
}

// 获取行数
size_t listview_cx::get_count()
{
	return _data.size();
}
// 列数
size_t listview_cx::get_column_count()
{
	return _title.size();
}

void listview_cx::on_event(uint32_t type, et_un_t* e)
{
	plane_cx::on_event(type, e);
}

void listview_cx::update(float delta)
{
	plane_cx::update(delta);
}


tree_view_cx::tree_view_cx()
{
}

tree_view_cx::~tree_view_cx()
{
	remove(&_root);
}
void tree_view_cx::binding(plane_cx* p)
{
	plane = p;
}
tree_node_t* tree_view_cx::insert(tree_node_t* parent, const std::string& str, void* data)
{
	auto p = new tree_node_t();
	p->title = str;
	p->raw = data;
	p->parent = parent;
	p->level = 0;
	p->_autofree = true;
	return insert(p, parent);
}
tree_node_t* tree_view_cx::insert(tree_node_t* c, tree_node_t* parent)
{
	auto p = parent;
	if (!p) {
		p = &_root;		// 不提供parent则插入到根节点
	}
	if (c && p)
	{
		c->parent = p;
		if (!p->child) p->child = new std::vector<tree_node_t*>();
		if (p->child) p->child->push_back(c);
	}
	return c;
}
void tree_view_cx::remove(tree_node_t* p)
{
	std::stack<tree_node_t*> q;
	if (p)
	{
		q.push(p);
		for (; q.size();)
		{
			auto t = q.top(); q.pop();
			if (t->child) {
				for (auto it : *t->child)
				{
					if (it && it->_autofree) {
						q.push(it);
					}
				}
				delete t->child;
			}
			if (t)
			{
				if (t->_autofree)
					delete t;
			}
		}
	}
}
void tree_view_cx::set_scroll_pos(const glm::ivec2& scp)
{
	scroll_pos = scp;
}


void tree_view_cx::on_mevent(int type, const glm::vec2& mps)
{
}
void tree_view_cx::update(float delta)
{
}
void tree_view_cx::draw(cairo_t* cr)
{

}
valueview_cx::valueview_cx()
{
}

valueview_cx::~valueview_cx()
{
}

void valueview_cx::on_event(uint32_t type, et_un_t* e)
{
	plane_cx::on_event(type, e);
}

void valueview_cx::update(float delta)
{
	plane_cx::update(delta);
}



std::vector<color_btn*> new_label(plane_cx* p, const std::vector<std::string>& t, int width, std::function<void(void* p, int clicks)> cb)
{
	std::vector<color_btn*> rv;
	if (p && p->ltx) {
		for (auto& it : t)
		{
			glm::vec2 bs;
			bs.x = bs.y = p->fontsize;
			bs.x = width;
			bs.y = p->ltx->get_lineheight(0, bs.y);
			auto kcb = p->add_label(it, bs, 0);
			rv.push_back(kcb);
		}
	}
	return rv;
}

std::vector<checkbox_com> new_checkbox(plane_cx* p, const std::vector<std::string>& t, int width, std::function<void(void* ptr, bool v)> cb)
{
	std::vector<checkbox_com> rv;
	if (p && p->ltx) {
		for (auto& it : t)
		{
			glm::vec2 bs;
			bs.x = bs.y = p->ltx->get_lineheight(0, p->fontsize);
			bs.x *= 1.5;
			auto c = p->add_checkbox(bs, it, false);
			c->v.on_change_cb = cb;
			bs.x = width;
			auto kcb = p->add_label(it, bs, 0);
			kcb->click_cb = [=](void* ptr, int clicks)
				{
					c->set_value();
				};
			rv.push_back({ c,kcb });
		}
	}
	return rv;
}
std::vector<radio_com> new_radio(plane_cx* p, const std::vector<std::string>& t, int width, std::function<void(void* ptr, bool v)> cb)
{
	std::vector<radio_com> rv;
	if (p && p->ltx) {
		auto gr = new group_radio_t();
		for (auto& it : t)
		{
			glm::vec2 bs;
			bs.x = bs.y = p->ltx->get_lineheight(0, p->fontsize);
			bs.x *= 1.5;
			auto c = p->add_radio(bs, it, false, gr);
			c->v.on_change_cb = cb;
			bs.x = width;
			auto kcb = p->add_label(it, bs, 0);
			kcb->click_cb = [=](void* ptr, int clicks)
				{
					c->set_value();
				};
			rv.push_back({ c,kcb });
		}
	}
	return rv;
}

#if 1

cairo_surface_t* load_imagesvg(const std::string& fn, float scale)
{
	cairo_surface_t* p = 0;
	if (fn.rfind(".svg") != std::string::npos)
	{
		svg_cx* bl = new_svg_file(fn.c_str(), 0, 96);
		int xn = bl->width * bl->height;
		float sc = scale > 1 ? scale : 1;
		auto blsur = new_image_cr({ bl->width * sc ,bl->height * sc });
		auto pxd = (uint32_t*)cairo_image_surface_get_data(blsur);
		if (pxd)
		{
			p = blsur;
			auto stride = cairo_image_surface_get_stride(blsur) / 4;
			stride *= bl->height;
			for (size_t i = 0; i < xn; i++)
			{
				pxd[i] = 0;
			}
			image_set_ud(p, key_def_data_svgptr, bl, (cairo_destroy_func_t)free_svg);
			std::thread th([=]()
				{
					print_time a("load svg");
					cairo_t* cr = cairo_create(blsur);
					render_svg(cr, bl, {}, { scale, scale }, 0, 0);
					image_set_ud(blsur, key_def_data_done, (void*)1, 0);
					cairo_destroy(cr);
				});
			th.detach();
		}
	}
	else {
		image_ptr_t* ptr = stbimage_load::new_load(fn.c_str(), 0);
		if (ptr)
		{
			p = new_image_cr(ptr);
			image_set_ud(p, key_def_data_iptr, ptr, (cairo_destroy_func_t)stbimage_load::free_img);
		}
	}
	return p;
}

rlistview_cx::rlistview_cx()
{
}

rlistview_cx::~rlistview_cx()
{
	clear_image();
}

void rlistview_cx::set_data(void* d)
{
	_data = d;
}

size_t rlistview_cx::add_image(const std::string& fn)
{
	size_t r = imagelist.size();
	auto sur = load_imagesvg(fn, 1);
	if (sur)
		imagelist.push_back(sur);
	return r;
}

void rlistview_cx::draw(cairo_t* cr, const glm::ivec2& scroll_pos)
{

}

void rlistview_cx::clear_image()
{
	for (auto it : imagelist) {
		if (it)
			cairo_surface_destroy(it);
	}
}



#endif // 1

// todo icu
#if 1

	// DLL动态加载
class Shared //:public Res
{
private:
	void* _ptr = 0;
	std::once_flag oc;
	bool isinit = false;
public:
	static std::string toLower(const std::string& s)
	{
		std::string str;
		std::transform(s.begin(), s.end(), str.begin(), ::tolower);
		return str;
	}
	static Shared* loadShared1(Shared* ptr, const std::string& fnstr, std::vector<std::string>* pdir = nullptr)
	{
#ifdef NDEBUG
		auto nd = "debug";
#else
		auto nd = "release";
#endif // NDEBUG
		bool is = ptr->loadFile(fnstr);
		if (!is && pdir)
		{
			for (auto it : *pdir)
			{
				auto str = toLower(it);
				if (str.find(nd) != std::string::npos)
				{
					continue;
				}
				if ('/' != *it.rbegin() && '\\' != *it.rbegin())
				{
					it.push_back('/');
				}
				is = ptr->loadFile(it + fnstr);
				if (is)
				{
					break;
				}
			}
		}
		if (!is)
		{
			ptr = nullptr;
		}
		return ptr;
	}
	static Shared* loadShared(const std::string& fnstr, std::vector<std::string>* pdir = nullptr)
	{
#ifdef NDEBUG
		auto nd = "debug";
#else
		auto nd = "release";
#endif // NDEBUG
		Shared* ptr = new Shared();
		bool is = ptr->loadFile(fnstr);
		if (!is && pdir)
		{
			for (auto it : *pdir)
			{
				auto nit = toLower(it);
				if (nit.find(nd) != std::string::npos)
				{
					continue;
				}
				if ('/' != *it.rbegin() && '\\' != *it.rbegin())
				{
					it.push_back('/');
				}
				is = ptr->loadFile(it + fnstr);
				if (is)
				{
					break;
				}
			}
		}
		if (!is)
		{
			delete ptr;
			ptr = nullptr;
		}
		if (ptr) {

		}
		return ptr;
	}
	static void destroy(Shared* p)
	{
		if (p)
			delete p;
	}
public:
	bool loadFile(const std::string& fnstr)
	{
#ifdef _WIN32
#define _DL_OPEN(d) LoadLibraryExA(d,0,LOAD_WITH_ALTERED_SEARCH_PATH)
		const char* sysdirstr = nullptr;
		const char* sys64 = nullptr;
#else
#define _DL_OPEN(d) dlopen(d,RTLD_NOW)
		const char* sysdirstr = "/usr/local/lib/";
		const char* sys64 = "/system/lib64/";
#endif
		//std::call_once(oc, [=]() {
		int inc = 0;
		std::string dfn = fnstr;
		std::string errstr;
		isinit = false;
		do
		{
			_ptr = _DL_OPEN(dfn.c_str());
			if (_ptr)break;
#ifndef _WIN32
			auto er = dlerror();
			if (er)
				errstr = er;
#endif // !_WIN32
#ifdef __FILE__h__
			dfn = File::getAP(dfn);
#endif
			_ptr = _DL_OPEN(dfn.c_str());
			if (_ptr)break;
			if (sysdirstr)
			{
				dfn = sysdirstr + fnstr;
				_ptr = _DL_OPEN(dfn.c_str());
			}
			if (_ptr)break;
			if (sys64)
			{
				dfn = sys64 + fnstr;
				_ptr = _DL_OPEN(dfn.c_str());
			}
			if (!_ptr) {
				errstr = getLastError();
				printf("Could not load %s dll library!\n", fnstr.c_str());
			}
		} while (0);

		if (_ptr)
		{
			isinit = true;
		}
		return isinit;
	}
	void dll_close()
	{
		if (_ptr)
		{

#ifdef _WIN32
			FreeLibrary((HMODULE)_ptr);
#else
			dlclose(_ptr);
#endif
			_ptr = 0;
		}
	}
	void* _dlsym(const char* funcname)
	{
#if defined(_WIN32)
#define __dlsym GetProcAddress
#define LIBPTR HMODULE
#else
#define __dlsym dlsym
#define LIBPTR void*
#endif
		void* func = (void*)__dlsym((LIBPTR)_ptr, funcname);
		if (!func)
			func = (void*)__dlsym((LIBPTR)_ptr, funcname);
		return func;
#undef __dlsym
#undef LIBPTR
	}

	// 批量获取
	void dllsyms(const char** funs, void** outbuf, int n)
	{
		for (int i = 0; i < n; i++)
		{
			auto fcn = funs[i];
			if (fcn)
			{
				auto it = _dlsym(fcn);
				if (it)
				{
					outbuf[i] = it;
				}
			}
		}
	}
	// 批量获取

	void dlsyms(const std::vector<std::string>& funs, void** outbuf)
	{
		auto n = funs.size();
		for (size_t i = 0; i < n; i++)
		{
			auto fcn = funs[i];
			if (fcn.size())
			{
				auto it = _dlsym(fcn.c_str());
				if (it)
				{
					outbuf[i] = it;
				}
			}
		}
	}
	void dlsyms(const std::vector<const char*>& funs, void** outbuf)
	{
		auto n = funs.size();
		for (size_t i = 0; i < n; i++)
		{
			auto fcn = funs[i];
			if (fcn && *fcn)
			{
				auto it = _dlsym(fcn);
				if (it)
				{
					outbuf[i] = it;
				}
			}
		}
	}
	//template<class T>
	//T get_cb(const char* str, T& ot)
	//{
	//	T ret = (T)_dlsym(str);
	//	ot = ret;
	//	return ret;
	//}
	template<class T>
	T get_cb(const std::string& str, T& ot)
	{
		T ret = (T)_dlsym(str.c_str());
		ot = ret;
		return ret;
	}
	template<class T>
	T get_cb(const std::string& str, T* ot)
	{
		T ret = (T)_dlsym(str.c_str());
		if (ot)
			*ot = ret;
		return ret;
	}
	template<class T>
	T get_cb(const std::string& str)
	{
		T ret = (T)_dlsym(str.c_str());
		return ret;
	}
	static void* dllsym(void* ptr, const char* fn)
	{
		Shared* ctx = (Shared*)ptr;
		void* ret = nullptr;
		if (ctx)
		{
			ret = ctx->_dlsym(fn);
		}
		return ret;
	}
	std::string getLastError()
	{
		std::string str;
#ifdef _WIN32

		char* buf = 0;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf, 0, NULL);
		if (buf)
		{
			str = buf;
			LocalFree(buf);
		}
#else
#ifdef errno
		str = strerror(errno);
#endif
#endif // _WIN32
		return str;
	}
public:
	Shared()
	{
	}

	~Shared()
	{
		dll_close();
	}

private:

};

struct icu_lib_t
{
	void (*_ubidi_setPara)(UBiDi* pBiDi, const UChar* text, int32_t length, UBiDiLevel paraLevel, UBiDiLevel* embeddingLevels, UErrorCode* pErrorCode);
	int32_t(*_ubidi_countRuns) (UBiDi* pBiDi, UErrorCode* pErrorCode);
	UBiDi* (*_ubidi_open)(void);
	void (*_ubidi_close)(UBiDi* pBiDi);
	UBiDiDirection(*_ubidi_getVisualRun)(UBiDi* pBiDi, int32_t runIndex, int32_t* pLogicalStart, int32_t* pLength);
	int32_t(*_ucnv_convert)(const char* toConverterName, const char* fromConverterName, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode);
	UScriptCode(*_uscript_getScript)(UChar32 codepoint, UErrorCode* err);
	UBool(*_uscript_hasScript)(UChar32 c, UScriptCode sc);
	const char* (*_uscript_getName)(UScriptCode scriptCode);
	const char* (*_uscript_getShortName)(UScriptCode scriptCode);
	void* _handle;
};


#ifdef _WIN32
const char* exts = ".dll";
#else
const char* exts = ".so";
#endif // _WIN32
#ifdef EAD_ICONV
static iconv_info_t ic = {};
#endif
static std::once_flag icof, icuf;
static std::set<std::string> lsic;
static icu_lib_t* icub = 0;

Shared* loadso(const std::string& dlln)
{
	auto so = Shared::loadShared(dlln + exts);
	return so;
}
icu_lib_t* get_icu(int v)
{
#define mxv 1000
	if (!icub)
	{
		try {
			std::string dlln = "libicuuc";
			std::string dlln0 = "icudt";
			int v1 = v;
#ifdef _WIN32
			dlln = "icuuc" + std::to_string(v1);
			dlln0 = "icudt" + std::to_string(v1);
			auto so0 = loadso(dlln0);
			auto so = loadso(dlln);
			if (!so0)
			{
				dlln0 = "icu";
				so0 = loadso(dlln0);
			}
			if (!so)
			{
				dlln = "icuuc";
				so = loadso(dlln);
			}
#else
			auto so = loadso(dlln);
#endif // _WIN32

			if (!so) {
				so = loadso(dlln);
				if (!so)
					throw std::runtime_error("-1");
			}
			std::string n;
			void* uc = 0;

#if 1 
			int nc = 0;
			do
			{
				for (int i = v1; i > 0; i--)
				{
					auto n1 = std::to_string(i);
					auto k = "ubidi_close_" + n1;
					auto fb = so->_dlsym(k.c_str());
					if (fb)
					{
						n = n1;
						uc = fb;
						break;
					}
				}
				if (n.empty() && nc == 0) { v1 = mxv; nc++; continue; }

			} while (0);
#endif
			std::vector<std::string> str = { "ubidi_setPara","ubidi_countRuns","ubidi_open","ubidi_close","ubidi_getVisualRun"
			,"ucnv_convert" , "uscript_getScript","uscript_hasScript","uscript_getName","uscript_getShortName" };
			if (n.size())
			{
				for (auto& it : str)
				{
					it += "_" + n;
				}
			}
			icu_lib_t tb = {};
			so->dlsyms(str, (void**)&tb);
			if (tb._ubidi_close)
			{
				icub = new icu_lib_t();
				*icub = tb;
				icub->_handle = so;
			}
			else {
				Shared::destroy(so);
			}
		}
		catch (const std::exception& e)
		{
			auto ew = e.what();
			if (ew)
			{
				printf(ew);
				printf("load icu error!\n");
			}
		}
	}
	return icub;
}

void un_icu()
{
	if (icub)
	{

		if (icub->_handle)
		{
			Shared::destroy((Shared*)icub->_handle);
		}
		delete icub;
		icub = 0;
	}
}

void init_icu()
{
	if (!icub)
		get_icu(U_ICU_VERSION_MAJOR_NUM);
}

int32_t dl_icuuc_utf82gbk(char* outbuf, int32_t buflen, const char* instring, int32_t inlen);
int32_t dl_icuuc_u162gbk(char* outbuf, int32_t buflen, const char* instring, int32_t inlen);
//utf-8,gb2312,ucs4
//utf-8:  一个英文字母或者是数字占用一个字节，汉字占3个字节
//gb2312: 一个英文字母或者是数字占用一个字节，汉字占2个字节
int32_t dl_icuuc_gbk2utf8(char* outbuf, int32_t buflen, const char* instring, int32_t inlen)
{
	if (!icub)
		get_icu(U_ICU_VERSION_MAJOR_NUM);
	int32_t iret = 0;
	if (icub && outbuf != 0 && instring != 0)
	{
		if (icub->_ucnv_convert)
		{
			UErrorCode err_code = {};
			iret = icub->_ucnv_convert("utf-8", "gbk", outbuf, buflen, instring, inlen, &err_code);
		}
	}
	return iret;
}
int32_t dl_icuuc_u162gbk(char* outbuf, int32_t buflen, const char* instring, int32_t inlen)
{
	if (!icub)
		get_icu(U_ICU_VERSION_MAJOR_NUM);
	int32_t iret = -1;
	if (icub)
	{
		UErrorCode err_code = {};
		iret = 0;
		if (outbuf != 0 && instring != 0)
		{
			if (icub->_ucnv_convert)
			{
				iret = icub->_ucnv_convert("gbk", "utf-16le"
					, outbuf, buflen
					, instring, inlen
					, &err_code);
			}
		}
	}
	return iret;
}
int32_t dl_icuuc_utf82gbk(char* outbuf, int32_t buflen, const char* instring, int32_t inlen)
{
	if (!icub)
		get_icu(U_ICU_VERSION_MAJOR_NUM);
	int32_t iret = -1;
	if (icub)
	{
		iret = 0;
		if (outbuf != 0 && instring != 0)
		{
			if (icub->_ucnv_convert)
			{
				UErrorCode err_code = {};
				iret = icub->_ucnv_convert("gbk", "utf-8"
					, outbuf, buflen
					, instring, inlen
					, &err_code);
			}
		}
	}
	return iret;
}
int32_t dl_icuuc_unicode2utf8(char* outbuf, int32_t buflen, const unsigned short* instring, int32_t inlen)
{
	if (!icub)
		get_icu(U_ICU_VERSION_MAJOR_NUM);
	int32_t iret = -1;
	if (icub)
	{
		iret = 0;
		UErrorCode err_code = {};
		if (outbuf != 0 && instring != 0)
		{
			if (icub->_ucnv_convert)
			{
				iret = icub->_ucnv_convert("utf-8", "utf-16le"
					, outbuf, buflen
					, (const char*)instring, inlen * sizeof(unsigned short) / sizeof(char)
					, &err_code);
			}
		}
	}
	return iret;
}
int32_t dl_icuuc_utf82unicode(unsigned short* outbuf, int32_t buflen, const char* instring, int32_t inlen)
{
	if (!icub)
		get_icu(U_ICU_VERSION_MAJOR_NUM);
	int32_t iret = -1;
	if (icub) {
		iret = 0;
		UErrorCode err_code = {};
		if (outbuf != 0 && instring != 0)
		{
			if (icub->_ucnv_convert)
			{
				iret = icub->_ucnv_convert("utf-16le", "utf-8"
					, (char*)outbuf, buflen * sizeof(unsigned short) / sizeof(char)
					, instring, inlen
					, &err_code);
			}
		}
	}
	return iret;
}

std::string icu_gbk_u8(const char* str, size_t size)
{
	std::string r;
	if (str)
	{
		if (size == -1)size = strlen(str);
		if (size > 0)
		{
			r.resize(size * 2);
			auto n = dl_icuuc_gbk2utf8(r.data(), r.size(), str, size);
			if (n > 0)
				r.resize(n);
		}
	}
	return r;
}
std::string icu_u8_gbk(const char* str, size_t size)
{
	std::string r;
	if (str)
	{
		if (size == -1)size = strlen(str);
		if (size > 0)
		{
			r.resize(size);
			auto n = dl_icuuc_utf82gbk(r.data(), r.size(), str, size);
			if (n > 0)
				r.resize(n);
		}
	}
	return r;
}
int utf16len(unsigned short* utf16)
{
	int utf;
	int ret = 0;
	while (utf = *utf16++)
		ret += ((utf < 0xD800) || (utf > 0xDFFF)) ? 2 : 1;
	return ret;
}
std::string icu_u16_gbk(const void* str, size_t size)
{
	std::string r;
	if (str)
	{
		if (size == -1)size = utf16len((uint16_t*)str);
		if (size > 0)
		{
			r.resize(size);
			auto n = dl_icuuc_u162gbk(r.data(), r.size(), (char*)str, size);
			if (n > 0)
				r.resize(n);
		}
	}
	return r;
}
std::string icu_u16_u8(const void* str, size_t size)
{
	std::string r;
	if (str)
	{
		if (size == -1)size = utf16len((uint16_t*)str);
		if (size > 0)
		{
			r.resize(size);
			auto n = dl_icuuc_unicode2utf8(r.data(), r.size(), (uint16_t*)str, size);
			if (n > 0)
				r.resize(n);
		}
	}
	return r;
}

std::u16string icu_u8_u16(const char* str, size_t size)
{
	std::u16string r;
	if (str)
	{
		if (size == -1)size = strlen(str);
		if (size > 0)
		{
			r.resize(size);
			auto n = dl_icuuc_utf82unicode((uint16_t*)r.data(), r.size(), str, size);
			if (n > 0)
				r.resize(n);
		}
	}
	return r;
}


struct ScriptRecord
{
	UChar32 startChar;
	UChar32 endChar;
	UScriptCode scriptCode;
};

struct ParenStackEntry
{
	int32_t pairIndex;
	UScriptCode scriptCode;
};

class ScriptRun /*: public UObject*/ {
public:
	ScriptRun();
	~ScriptRun();

	ScriptRun(const UChar chars[], int32_t length);

	ScriptRun(const UChar chars[], int32_t start, int32_t length);

	void reset();

	void reset(int32_t start, int32_t count);

	void reset(const UChar chars[], int32_t start, int32_t length);

	int32_t getScriptStart();

	int32_t getScriptEnd();

	UScriptCode getScriptCode();

	UBool next();

	/**
	 * ICU "poor man's RTTI", returns a UClassID for the actual class.
	 *
	 * @stable ICU 2.2
	 */
	virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

	/**
	 * ICU "poor man's RTTI", returns a UClassID for this class.
	 *
	 * @stable ICU 2.2
	 */
	static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

	icu_lib_t* icub = 0;
private:

	static UBool sameScript(int32_t scriptOne, int32_t scriptTwo);

	int32_t charStart;
	int32_t charLimit;
	const UChar* charArray;

	int32_t scriptStart;
	int32_t scriptEnd;
	UScriptCode scriptCode;

	ParenStackEntry parenStack[128];
	int32_t parenSP;

	static int8_t highBit(int32_t value);
	static int32_t getPairIndex(UChar32 ch);

	static UChar32 pairedChars[];
	static const int32_t pairedCharCount;
	static const int32_t pairedCharPower;
	static const int32_t pairedCharExtra;

	/**
	 * The address of this static class variable serves as this class's ID
	 * for ICU "poor man's RTTI".
	 */
	static const char fgClassID;
};

const char ScriptRun::fgClassID = 0;

UChar32 ScriptRun::pairedChars[] = {
	0x0028, 0x0029, // ascii paired punctuation
	0x003c, 0x003e,
	0x005b, 0x005d,
	0x007b, 0x007d,
	0x00ab, 0x00bb, // guillemets
	0x2018, 0x2019, // general punctuation
	0x201c, 0x201d,
	0x2039, 0x203a,
	0x3008, 0x3009, // chinese paired punctuation
	0x300a, 0x300b,
	0x300c, 0x300d,
	0x300e, 0x300f,
	0x3010, 0x3011,
	0x3014, 0x3015,
	0x3016, 0x3017,
	0x3018, 0x3019,
	0x301a, 0x301b
};
#ifndef UPRV_LENGTHOF
#define UPRV_LENGTHOF(a) (int32_t)(sizeof(a)/sizeof((a)[0]))
#endif
const int32_t ScriptRun::pairedCharCount = UPRV_LENGTHOF(pairedChars);
const int32_t ScriptRun::pairedCharPower = 1 << highBit(pairedCharCount);
const int32_t ScriptRun::pairedCharExtra = pairedCharCount - pairedCharPower;

int8_t ScriptRun::highBit(int32_t value)
{
	if (value <= 0) {
		return -32;
	}

	int8_t bit = 0;

	if (value >= 1 << 16) {
		value >>= 16;
		bit += 16;
	}

	if (value >= 1 << 8) {
		value >>= 8;
		bit += 8;
	}

	if (value >= 1 << 4) {
		value >>= 4;
		bit += 4;
	}

	if (value >= 1 << 2) {
		value >>= 2;
		bit += 2;
	}

	if (value >= 1 << 1) {
		value >>= 1;
		bit += 1;
	}

	return bit;
}

int32_t ScriptRun::getPairIndex(UChar32 ch)
{
	int32_t probe = pairedCharPower;
	int32_t index = 0;

	if (ch >= pairedChars[pairedCharExtra]) {
		index = pairedCharExtra;
	}

	while (probe > (1 << 0)) {
		probe >>= 1;

		if (ch >= pairedChars[index + probe]) {
			index += probe;
		}
	}

	if (pairedChars[index] != ch) {
		index = -1;
	}

	return index;
}

UBool ScriptRun::sameScript(int32_t scriptOne, int32_t scriptTwo)
{
	return scriptOne <= USCRIPT_INHERITED || scriptTwo <= USCRIPT_INHERITED || scriptOne == scriptTwo;
}

UBool ScriptRun::next()
{

	int32_t startSP = parenSP;  // used to find the first new open character
	UErrorCode error = U_ZERO_ERROR;

	// if we've fallen off the end of the text, we're done
	if (scriptEnd >= charLimit || !icub || !icub->_uscript_getScript || !icub->_uscript_hasScript) {
		return false;
	}

	scriptCode = USCRIPT_COMMON;

	for (scriptStart = scriptEnd; scriptEnd < charLimit; scriptEnd += 1) {
		UChar   high = charArray[scriptEnd];
		UChar32 ch = high;

		// if the character is a high surrogate and it's not the last one
		// in the text, see if it's followed by a low surrogate
		if (high >= 0xD800 && high <= 0xDBFF && scriptEnd < charLimit - 1)
		{
			UChar low = charArray[scriptEnd + 1];

			// if it is followed by a low surrogate,
			// consume it and form the full character
			if (low >= 0xDC00 && low <= 0xDFFF) {
				ch = (high - 0xD800) * 0x0400 + low - 0xDC00 + 0x10000;
				scriptEnd += 1;
			}
		}
		//uscript_getCode();
		UScriptCode sc = icub->_uscript_getScript(ch, &error);
		if (sc == USCRIPT_ARABIC)
		{
			if (icub->_uscript_hasScript(ch, USCRIPT_UGARITIC))
			{
				sc = USCRIPT_UGARITIC;
			}
		}
		int32_t pairIndex = getPairIndex(ch);

		// Paired character handling:
		//
		// if it's an open character, push it onto the stack.
		// if it's a close character, find the matching open on the
		// stack, and use that script code. Any non-matching open
		// characters above it on the stack will be poped.
		if (pairIndex >= 0) {
			if ((pairIndex & 1) == 0) {
				parenStack[++parenSP].pairIndex = pairIndex;
				parenStack[parenSP].scriptCode = scriptCode;
			}
			else if (parenSP >= 0) {
				int32_t pi = pairIndex & ~1;

				while (parenSP >= 0 && parenStack[parenSP].pairIndex != pi) {
					parenSP -= 1;
				}

				if (parenSP < startSP) {
					startSP = parenSP;
				}

				if (parenSP >= 0) {
					sc = parenStack[parenSP].scriptCode;
				}
			}
		}

		if (sameScript(scriptCode, sc)) {
			if (scriptCode <= USCRIPT_INHERITED && sc > USCRIPT_INHERITED) {
				scriptCode = sc;

				// now that we have a final script code, fix any open
				// characters we pushed before we knew the script code.
				while (startSP < parenSP) {
					parenStack[++startSP].scriptCode = scriptCode;
				}
			}

			// if this character is a close paired character,
			// pop it from the stack
			if (pairIndex >= 0 && (pairIndex & 1) != 0 && parenSP >= 0) {
				parenSP -= 1;
				startSP -= 1;
			}
		}
		else {
			// if the run broke on a surrogate pair,
			// end it before the high surrogate
			if (ch >= 0x10000) {
				scriptEnd -= 1;
			}

			break;
		}
	}

	return true;
}

ScriptRun::ScriptRun()
{
	reset(NULL, 0, 0);
}
ScriptRun::~ScriptRun()
{
}

ScriptRun::ScriptRun(const UChar chars[], int32_t length)
{
	reset(chars, 0, length);
}

ScriptRun::ScriptRun(const UChar chars[], int32_t start, int32_t length)
{
	reset(chars, start, length);
}

int32_t ScriptRun::getScriptStart()
{
	return scriptStart;
}

int32_t ScriptRun::getScriptEnd()
{
	return scriptEnd;
}

UScriptCode ScriptRun::getScriptCode()
{
	return scriptCode;
}

void ScriptRun::reset()
{
	scriptStart = charStart;
	scriptEnd = charStart;
	scriptCode = USCRIPT_INVALID_CODE;
	parenSP = -1;
}

void ScriptRun::reset(int32_t start, int32_t length)
{
	charStart = start;
	charLimit = start + length;

	reset();
}

void ScriptRun::reset(const UChar chars[], int32_t start, int32_t length)
{
	charArray = chars;

	reset(start, length);
}

//UObject::UObject(){}
//UObject::~UObject(){}

static hb_script_t get_script(const char* str)
{
#define KN(v,v1) {#v1,#v},{#v,#v}
	static std::map<std::string, std::string> ms =
	{
		KN(Adlm, Adlam),
		KN(Aghb, Caucasian_Albanian),
		KN(Ahom, Ahom),
		KN(Arab, Arabic),
		KN(Armi, Imperial_Aramaic),
		KN(Armn, Armenian),
		KN(Avst, Avestan),
		KN(Bali, Balinese),
		KN(Bamu, Bamum),
		KN(Bass, Bassa_Vah),
		KN(Batk, Batak),
		KN(Beng, Bengali),
		KN(Bhks, Bhaiksuki),
		KN(Bopo, Bopomofo),
		KN(Brah, Brahmi),
		KN(Brai, Braille),
		KN(Bugi, Buginese),
		KN(Buhd, Buhid),
		KN(Cakm, Chakma),
		KN(Cans, Canadian_Aboriginal),
		KN(Cari, Carian),
		KN(Cham, Cham),
		KN(Cher, Cherokee),
		KN(Chrs, Chorasmian),
		KN(Copt, Coptic),
		KN(Cprt, Cypriot),
		KN(Cyrl, Cyrillic),
		KN(Deva, Devanagari),
		KN(Diak, Dives_Akuru),
		KN(Dogr, Dogra),
		KN(Dsrt, Deseret),
		KN(Dupl, Duployan),
		KN(Egyp, Egyptian_Hieroglyphs),
		KN(Elba, Elbasan),
		KN(Elym, Elymaic),
		KN(Ethi, Ethiopic),
		KN(Geor, Georgian),
		KN(Glag, Glagolitic),
		KN(Gong, Gunjala_Gondi),
		KN(Gonm, Masaram_Gondi),
		KN(Goth, Gothic),
		KN(Gran, Grantha),
		KN(Grek, Greek),
		KN(Gujr, Gujarati),
		KN(Guru, Gurmukhi),
		KN(Hang, Hangul),
		KN(Hani, Han),
		KN(Hani, Hans),
		KN(Hani, Hant),
		KN(Hano, Hanunoo),
		KN(Hatr, Hatran),
		KN(Hebr, Hebrew),
		KN(Hira, Hiragana),
		KN(Hluw, Anatolian_Hieroglyphs),
		KN(Hmng, Pahawh_Hmong),
		KN(Hmnp, Nyiakeng_Puachue_Hmong),
		KN(Hrkt, Katakana_Or_Hiragana),
		KN(Hung, Old_Hungarian),
		KN(Ital, Old_Italic),
		KN(Java, Javanese),
		KN(Kali, Kayah_Li),
		KN(Kana, Katakana),
		KN(Khar, Kharoshthi),
		KN(Khmr, Khmer),
		KN(Khoj, Khojki),
		KN(Kits, Khitan_Small_Script),
		KN(Knda, Kannada),
		KN(Kthi, Kaithi),
		KN(Lana, Tai_Tham),
		KN(Laoo, Lao),
		KN(Latn, Latin),
		KN(Lepc, Lepcha),
		KN(Limb, Limbu),
		KN(Lina, Linear_A),
		KN(Linb, Linear_B),
		KN(Lisu, Lisu),
		KN(Lyci, Lycian),
		KN(Lydi, Lydian),
		KN(Mahj, Mahajani),
		KN(Maka, Makasar),
		KN(Mand, Mandaic),
		KN(Mani, Manichaean),
		KN(Marc, Marchen),
		KN(Medf, Medefaidrin),
		KN(Mend, Mende_Kikakui),
		KN(Merc, Meroitic_Cursive),
		KN(Mero, Meroitic_Hieroglyphs),
		KN(Mlym, Malayalam),
		KN(Modi, Modi),
		KN(Mong, Mongolian),
		KN(Mroo, Mro),
		KN(Mtei, Meetei_Mayek),
		KN(Mult, Multani),
		KN(Mymr, Myanmar),
		KN(Nand, Nandinagari),
		KN(Narb, Old_North_Arabian),
		KN(Nbat, Nabataean),
		KN(Newa, Newa),
		KN(Nkoo, Nko),
		KN(Nshu, Nushu),
		KN(Ogam, Ogham),
		KN(Olck, Ol_Chiki),
		KN(Orkh, Old_Turkic),
		KN(Orya, Oriya),
		KN(Osge, Osage),
		KN(Osma, Osmanya),
		KN(Palm, Palmyrene),
		KN(Pauc, Pau_Cin_Hau),
		KN(Perm, Old_Permic),
		KN(Phag, Phags_Pa),
		KN(Phli, Inscriptional_Pahlavi),
		KN(Phlp, Psalter_Pahlavi),
		KN(Phnx, Phoenician),
		KN(Plrd, Miao),
		KN(Prti, Inscriptional_Parthian),
		KN(Rjng, Rejang),
		KN(Rohg, Hanifi_Rohingya),
		KN(Runr, Runic),
		KN(Samr, Samaritan),
		KN(Sarb, Old_South_Arabian),
		KN(Saur, Saurashtra),
		KN(Sgnw, SignWriting),
		KN(Shaw, Shavian),
		KN(Shrd, Sharada),
		KN(Sidd, Siddham),
		KN(Sind, Khudawadi),
		KN(Sinh, Sinhala),
		KN(Sogd, Sogdian),
		KN(Sogo, Old_Sogdian),
		KN(Sora, Sora_Sompeng),
		KN(Soyo, Soyombo),
		KN(Sund, Sundanese),
		KN(Sylo, Syloti_Nagri),
		KN(Syrc, Syriac),
		KN(Tagb, Tagbanwa),
		KN(Takr, Takri),
		KN(Tale, Tai_Le),
		KN(Talu, New_Tai_Lue),
		KN(Taml, Tamil),
		KN(Tang, Tangut),
		KN(Tavt, Tai_Viet),
		KN(Telu, Telugu),
		KN(Tfng, Tifinagh),
		KN(Tglg, Tagalog),
		KN(Thaa, Thaana),
		KN(Thai, Thai),
		KN(Tibt, Tibetan),
		KN(Tirh, Tirhuta),
		KN(Ugar, Ugaritic),
		KN(Vaii, Vai),
		KN(Wara, Warang_Citi),
		KN(Wcho, Wancho),
		KN(Xpeo, Old_Persian),
		KN(Xsux, Cuneiform),
		KN(Yezi, Yezidi),
		KN(Yiii, Yi),
		KN(Zanb, Zanabazar_Square),
		KN(Zinh, Inherited),
		KN(Zyyy, Common),
		KN(Zzzz, Unknonn)
	};
#undef KN
	hb_script_t ret = HB_SCRIPT_UNKNOWN;
	auto it = ms.find(str);
	if (it != ms.end())
	{
		ret = hb_script_from_string(it->second.c_str(), 4);
	}
	return ret;
}

struct char_item_t
{
	unsigned int glyph_index;	// 索引
	glm::ivec2 offset;			// 偏移
	glm::ivec2 adv;				// xy_advance
	glm::ivec2 size;			// 原始大小
};
struct str_info_t
{
	union str_t
	{
		const uint8_t* p8;
		const uint16_t* p16;
		const uint32_t* p32 = 0;
	}str;
	unsigned int	type = 0;
	int             text_length = -1;
	unsigned int    item_offset = 0;
	int             item_length = -1;
	str_info_t() {}
	str_info_t(const char* s, int tlen = -1, unsigned int pos = 0, int ilen = -1) :text_length(tlen), item_offset(pos), item_length(ilen)
	{
		str.p8 = (uint8_t*)s;
	}
	str_info_t(const uint16_t* s, int tlen = -1, unsigned int pos = 0, int ilen = -1) :type(1), text_length(tlen), item_offset(pos), item_length(ilen)
	{
		str.p16 = (uint16_t*)s;
	}
	str_info_t(const uint32_t* s, int tlen = -1, unsigned int pos = 0, int ilen = -1) :type(2), text_length(tlen), item_offset(pos), item_length(ilen)
	{
		str.p32 = (uint32_t*)s;
	}
};
struct bidi_item
{
	hb_script_t sc;
	std::string s;
	uint32_t first, second;
	bool rtl = false;
};
enum class lt_dir
{
	invalid = 0,
	ltr = 4,	// 左-右
	rtl,		// 右-左
	ttb,		// 竖
	btt			// 倒竖
};
class word_key
{
private:
	// hb_script_t
	std::set<int> _sc;
	std::set<char> _split_ch;
public:
	word_key();
	~word_key();
	void push(int sc);
	// hb_script_t
	bool is_word(int sc);
	int get_split(char c);
private:

};
struct dlinfo_t
{
	font_t* font;
	double font_height;
	lt_dir dir;
	//Image* img;
	// x,y ,z=baseline
	//glm::ivec3 pos;
	bool is_word1;
	word_key* wks = 0;
	// out
	glm::ivec2 rc;
};
struct lt_item_t
{
	unsigned int _glyph_index = 0;
	// 缓存位置xy, 字符图像zw(width,height)
	glm::ivec4 _rect;
	// 渲染偏移坐标
	glm::ivec2 _dwpos;
	// 原始的advance
	int advance = 0;
	// hb偏移
	glm::ivec2 _offset;
	// hb计算的
	glm::ivec2 adv;
	// 字符
	char32_t ch[8] = {};
	int chlen = 0;
	uint32_t cluster = 0;
	// 连词
	int last_n = 0;
	// 渲染颜色
	uint32_t color = 0;
	image_ptr_t* _image = nullptr;
	bool rtl = false;
};

class hb_cx
{
public:
	enum class hb_dir
	{
		invalid = 0,
		ltr = 4,	// 左-右
		rtl,		// 右-左
		ttb,		// 竖
		btt			// 倒竖
	};
private:
	std::map<font_t*, hb_font_t*> _font;
	hb_buffer_t* _buffer = 0;
public:
	hb_cx();
	~hb_cx();
	font_t* push_font(font_t* p);
	void shape(const str_info_t& str, font_t* ttp, hb_dir dir);
	void shape(const str_info_t* str, font_t* ttp, hb_dir dir);
	//int tolayout(const std::string& str, font_t* ttf, double fontheight, bool is_word1, word_key& wks, std::vector<lt_item_t>& outlt);
	int tolayout(const std::string& str, dlinfo_t* info, std::vector<lt_item_t>& outlt);
	glm::ivec2 draw_to_image(const std::string& str, font_t* ttf, double fontheight, image_ptr_t* img, glm::ivec3 dstpos);
public:

private:
	void free_font();
};


struct BidiScriptRunRecords {
	bool isRtl;
	std::deque<ScriptRecord> records;
};
static void collectBidiScriptRuns(BidiScriptRunRecords& scriptRunRecords,
	const UChar* chars, int32_t start, int32_t end, bool isRTL) {
	scriptRunRecords.isRtl = isRTL;

	ScriptRun scriptRun(chars, start, end);
	scriptRun.icub = get_icu(U_ICU_VERSION_MAJOR_NUM);
	while (scriptRun.next()) {
		ScriptRecord scriptRecord;
		scriptRecord.startChar = scriptRun.getScriptStart();
		scriptRecord.endChar = scriptRun.getScriptEnd();
		scriptRecord.scriptCode = scriptRun.getScriptCode();

		scriptRunRecords.records.push_back(scriptRecord);
	}
}

word_key::word_key()
{
	_sc.insert(HB_SCRIPT_HAN);
	_split_ch.insert(' ');
	_split_ch.insert('\t');
}

word_key::~word_key()
{
}
void word_key::push(int sc)
{
	_sc.insert(sc);
}
bool word_key::is_word(int sc)
{
	return _sc.find(sc) != _sc.end();
}
int word_key::get_split(char c)
{
	return _split_ch.find(c) != _split_ch.end();
}

// todo hb
#if 1
static int icn = 0;
hb_cx::hb_cx()
{
	_buffer = hb_buffer_create();
	icn++;
}

hb_cx::~hb_cx()
{
	free_font();
	icn--;
	auto kuf = hb_buffer_get_unicode_funcs(_buffer);
	hb_buffer_destroy(_buffer);
	if (icn == 0)
	{
		hb_unicode_funcs_destroy(kuf);
	}
}

font_t* hb_cx::push_font(font_t* p)
{
	font_t* ret = 0;
	if (!p || _font.find(p) != _font.end())
	{
		return 0;
	}
	hb_blob_t* blob = hb_blob_create((char*)p->font->data, p->dataSize, HB_MEMORY_MODE_READONLY, 0, 0); //HB_MEMORY_MODE_WRITABLEhb_blob_create_from_file(ttfn);
	if (hb_blob_get_length(blob) > 0)
	{
		hb_face_t* face = hb_face_create(blob, p->_index);
		hb_font_t* hbf = hb_font_create(face);
		if (face && hbf)
		{
			//unsigned int upem = hb_face_get_upem(face);
			//hb_font_set_scale(hbf, upem, upem);
			//hb_font_set_ppem(hbf, ppem, ppem);
			_font[p] = hbf;
			ret = p;
		}
	}
	if (blob)
		hb_blob_destroy(blob);
	return ret;
}
void hb_cx::shape(const str_info_t& s, font_t* ttp, hb_dir dir)
{
	shape(&s, ttp, dir);
}
void hb_cx::shape(const str_info_t* str, font_t* ttp, hb_dir dir)
{
	if (_font.find(ttp) == _font.end())
	{
		return;
	}
	auto font = _font[ttp];
	if (font)
	{
		hb_buffer_t* buf = _buffer;
		hb_buffer_clear_contents(buf);
		hb_buffer_set_direction(buf, (hb_direction_t)dir);
		typedef void(*bufadd_func)(hb_buffer_t* buffer, const void* text, int text_length, unsigned int item_offset, int item_length);
		static bufadd_func cb[3] = { (bufadd_func)hb_buffer_add_utf8, (bufadd_func)hb_buffer_add_utf16, (bufadd_func)hb_buffer_add_utf32 };
		if (str->type < 3)
		{
			cb[str->type](buf, str->str.p8, str->text_length, str->item_offset, str->item_length);
			hb_buffer_guess_segment_properties(buf);
			hb_shape(font, buf, nullptr, 0);
		}

	}
}

int hb_cx::tolayout(const std::string& str, dlinfo_t* dinfo, std::vector<lt_item_t>& outlt)
//int hb_cx::tolayout(const std::string& str, font_t* ttf, double fontheight, bool is_word1, word_key& wks, std::vector<lt_item_t>& outlt)
{
	hb_buffer_t* buf = _buffer;
	unsigned int count = hb_buffer_get_length(buf);
	if (count == 0)
	{
		return 0;
	}
	font_t* ttf = dinfo->font;
	double fontheight = dinfo->font_height;
	bool is_word1 = dinfo->is_word1;
	word_key& wks = *dinfo->wks;
	hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buf, nullptr);
	hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buf, nullptr);
	auto scale_h = ttf->get_scale(fontheight);
	//auto dir = hb_buffer_get_direction(buf);
	auto pstr = str.c_str();
	//auto uc = hz::get_utf8_count(u8p, str.size());
	outlt.reserve(outlt.size() + count);
	size_t bidx = outlt.size();
	size_t n = 0;
	size_t sc = str.size();
	const char* laststr = 0;
	int64_t last_cluster = infos[0].cluster; char32_t cp32;
	bool isn = sc != count;
	std::vector<int64_t> cluster;
	int ic = 1;
	bool isback = (dinfo->dir == lt_dir::ltr) || (dinfo->dir == lt_dir::ttb);
	if (!isback)
		cluster.push_back(sc);
	// 收集cluster判断是否连写
	for (unsigned int i = 0; i < count; i++)
	{
		cluster.push_back(infos[i].cluster);
	}
	if (isback) { cluster.push_back(sc); }
	else {
		ic = 0;	//std::reverse(cluster.begin(), cluster.end());
	}

	for (unsigned int i = 0; i < count; i++)
	{
		hb_glyph_info_t* info = &infos[i];
		hb_glyph_position_t* pos = &positions[i];
		glm::vec2 adv = { ceil(pos->x_advance * scale_h), ceil(pos->y_advance * scale_h) };
		glm::vec2 offset = { ceil(pos->x_offset * scale_h), -ceil(pos->y_offset * scale_h) };
		unsigned int cp = 0;
		//laststr = md::get_u8_last(pstr + info->cluster, &cp);
		int ch = 0;
		auto nstr = pstr + info->cluster;
		auto kk = md::utf8_to_unicode(nstr, &ch);
		if (kk < 1)break;
		cp = ch;
		cp32 = cp;
		auto git = ttf->get_glyph_item(info->codepoint, cp, fontheight);

		lt_item_t lti = {};
		//memcpy(&lti, &git, sizeof(ftg_item));
		lti._image = git._image;
		lti.color = git.color;
		lti._glyph_index = git._glyph_index;
		lti._rect = git._rect;
		lti._dwpos = git._dwpos;
		lti.advance = git.advance;
		lti.rtl = !isback;
		lti.adv = adv;
		lti._offset = offset;
		lti.ch[0] = cp;
		lti.chlen = 1;
		if (isn)
		{
			int64_t clu[2] = { info->cluster, cluster[i + ic] };
			if (clu[0] > clu[1]) { std::swap(clu[0], clu[1]); }
			int64_t dif = clu[1];
			if (clu[0] != clu[1])
			{
				int c = 0;
				unsigned int cp1 = 0;
				auto uc = md::get_utf8_count(pstr + clu[0], clu[1] - clu[0]);
				if (uc > 1)
				{
					laststr = pstr + clu[0];
					for (int k = 0; k < uc; k++)
					{
						//laststr = md::get_u8_last(laststr, &cp1);
						auto kk = md::utf8_to_unicode(laststr, (int*)&cp1);
						assert(c < 8 && kk>0);
						if (kk > 0)
						{
							laststr += kk;
							lti.ch[c++] = cp1;
						}
					}
				}
				lti.chlen = uc;
			}
			if (dif == 1)
			{
				cp32 = 0;
			}
		}
		bool isctr = false;
		{

			static std::string ctn = R"(~!@#$%^&*()-+/\|/;'',.<>?`)";
			static std::u16string cn = uR"(【】、；‘：“’”，。、《》？（）——！￥)";
			if (cn.find(cp) != std::u16string::npos)
				isctr = true;
			if (cp < 255 && ctn.find(cp) != std::string::npos)
				isctr = false;
			if (cp < 255 && iscntrl(cp))
			{
				do
				{
					isctr = true;
				} while (0);
				if (cp != '\t')
					lti.adv.x = 0;
			}
		}
		lti.last_n = 0;
		lti.cluster = info->cluster;
		bool issplit = (cp < 255 && wks.get_split(cp));
		bool ispush = (is_word1 && cp > 255) || count - i == 1;
		//if (abs(info->cluster - last_cluster) > 2)
		//{
		//	issplit = true;
		//}
		last_cluster = info->cluster;
		//ispush = true;
		if (issplit || isctr)
		{
			if (n > 0)
			{
				outlt[bidx].last_n = n;
				bidx += n; n = 0;
			}
			ispush = true;
		}
		n++;
		outlt.push_back(lti);
		// 分割
		if (ispush)
		{
			outlt[bidx].last_n = n;
			bidx += n; n = 0;
		}
		//printf("%d\t", (int)adv.x);
	}

	//printf("\n");
	return count;
}
#if 0
glm::ivec2 hb_cx::draw_to_image(const std::string& str, font_t* ttf, double fontheight, hz::Image* img, glm::ivec3 dstpos)
{
	hb_buffer_t* buf = _buffer;
	unsigned int count = hb_buffer_get_length(buf);
	hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buf, nullptr);
	hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buf, nullptr);
	auto scale_h = ttf->get_scale_height(fontheight);

	auto bl = ttf->get_base_line(fontheight);
	double base_line = dstpos.z < 0 ? bl : dstpos.z;			// 基线
	//ttf ->get_VMetrics(&finfo[0], &finfo[1], &finfo[2]);
	int x = dstpos.x, y = dstpos.y;
	auto dir = hb_buffer_get_direction(buf);
	glm::ivec2 ret;
	glm::ivec2 dpos;
	auto u8p = str.c_str();
	//printf("%f\n", scale_h);
	for (unsigned int i = 0; i < count; i++)
	{
		hb_glyph_info_t* info = &infos[i];
		hb_glyph_position_t* pos = &positions[i];
		int adv = pos->x_advance * scale_h;
		int advy = pos->y_advance * scale_h;
		//printf("cluster %d	glyph 0x%x at	(%d,%d)+(%d,%d) %d\n",
		//	info->cluster,
		//	info->codepoint,
		//	pos->x_offset,
		//	pos->y_offset,
		//	pos->x_advance,
		//	pos->y_advance, adv);

		dpos.x = x + (pos->x_offset * scale_h);
		dpos.y = y - (pos->y_offset * scale_h) + base_line;
		unsigned int cp = 0;
		u8p = md::get_u8_last(u8p, &cp);
		//auto git = ttf->mk_glyph(info->codepoint, cp, fontheight);
		glm::vec2 size = {};// md::mk_fontbitmap(ttf, info->codepoint, cp, fontheight, dpos, -1, img);
		//printf("%d\t", dpos.x);
		x += adv;
		//x += size.z;
		if (dir == HB_DIRECTION_TTB && abs(advy) < size.y)
			advy = -size.y;
		y -= advy;
		ret.x += adv;
		if (size.y > ret.y)
		{
			ret.y = size.y;
		}
	}
	//printf("\n");
	return ret;
}
#endif
void hb_cx::free_font()
{
	for (auto& [k, v] : _font)
	{
		auto face = hb_font_get_face(v);
		hb_face_destroy(face);
		hb_font_destroy(v);
	}
	//hb_font_funcs_destroy(fcb);
	_font.clear();

}
#endif
void do_bidi(UChar* testChars, int len, std::vector<bidi_item>& info)
{
	//print_time ftpt("ubidi");

	static auto icub = get_icu(U_ICU_VERSION_MAJOR_NUM);
	do {
		if (!icub)
		{
			icub = get_icu(U_ICU_VERSION_MAJOR_NUM);
		}
		if (!icub)return;
		if (!icub->_ubidi_open)
		{
			return;
		}
	} while (0);


	UBiDi* bidi = icub->_ubidi_open();
	UBiDiLevel bidiReq = UBIDI_DEFAULT_LTR;
	int stringLen = len;
	if (bidi) {
		UErrorCode status = U_ZERO_ERROR;
		// Set callbacks to override bidi classes of new emoji
		//ubidi_setClassCallback(bidi, emojiBidiOverride, nullptr, nullptr, nullptr, &status);
		if (!U_SUCCESS(status)) {
			printf("error setting bidi callback function, status = %d", status);
			return;
		}
		info.reserve(len);
		icub->_ubidi_setPara(bidi, testChars, stringLen, bidiReq, NULL, &status);
		if (U_SUCCESS(status)) {
			//int paraDir = ubidi_getParaLevel(bidi);
			size_t rc = icub->_ubidi_countRuns(bidi, &status);
			for (size_t i = 0; i < rc; ++i) {
				int32_t startRun = 0;
				int32_t lengthRun = 0;
				UBiDiDirection runDir = icub->_ubidi_getVisualRun(bidi, i, &startRun, &lengthRun);
				bool isRTL = (runDir == UBIDI_RTL);
				//printf("Processing Bidi Run = %lld -- run-start = %d, run-len = %d, isRTL = %d\n", i, startRun, lengthRun, isRTL);
				BidiScriptRunRecords scriptRunRecords;
				collectBidiScriptRuns(scriptRunRecords, testChars, startRun, lengthRun, isRTL);

				//print_time ftpt("ubidi_2");
				while (!scriptRunRecords.records.empty()) {
					ScriptRecord scriptRecord;
					if (scriptRunRecords.isRtl) {
						scriptRecord = scriptRunRecords.records.back();
						scriptRunRecords.records.pop_back();
					}
					else {
						scriptRecord = scriptRunRecords.records.front();
						scriptRunRecords.records.pop_front();
					}

					uint32_t     start = scriptRecord.startChar;
					uint32_t     end = scriptRecord.endChar;
					UScriptCode code = scriptRecord.scriptCode;
					auto scrn = icub->_uscript_getName(code);
					auto sc = get_script(scrn);
					std::string u8strd = md::u16_u8((uint16_t*)(testChars + start), end - start);
					info.push_back({ sc, u8strd, start, end, isRTL });
					//printf("Script '%s' from %d to %d.\t%d\n", scrn, start, end, sc);
				}
			}
		}
	}
	return;
}


void do_text(const char* str, size_t first, size_t count)
{
	auto t = md::utf8_char_pos(str, first, count);
	auto te = md::utf8_char_pos(t, count, count);
	auto wk = md::u8_w(t, te - t);
	const uint16_t* str1 = (const uint16_t*)wk.c_str();
	size_t n = wk.size();
	std::vector<bidi_item> bidiinfo;
	{
		//print_time ftpt("bidi a");
		do_bidi((UChar*)str1, n, bidiinfo);
		std::stable_sort(bidiinfo.begin(), bidiinfo.end(), [](const bidi_item& bi, const bidi_item& bi1) { return bi.first < bi1.first; });
	}
	return;
}
#else
void do_text(const char* str, size_t first, size_t count)
{
}
#endif // 1

// 16进制编辑
#if 1
hex_editor::hex_editor()
{
}

hex_editor::~hex_editor()
{
	auto p = (hz::mfile_t*)mapfile;
	if (p)delete p;
	mapfile = 0;
	_data = 0;
	_size = 0;
}

void hex_editor::set_size(const glm::ivec2& s)
{
	if (s.x > 0 && s.y > 0)
		view_size = s;
}

bool hex_editor::set_data(const char* d, size_t len, bool is_copy)
{
	if (d && len > 0)
	{
		if (is_copy) {
			tempstr.resize(len);
			memcpy(tempstr.data(), d, len);
			_data = (unsigned char*)tempstr.data();
		}
		else {
			_data = (unsigned char*)d;
		}
		_size = len;
		return true;
	}
	return false;
}

bool hex_editor::set_file(const char* fn, bool is_rdonly)
{
	_rdonly = is_rdonly;
	auto p = (hz::mfile_t*)mapfile;
	if (fn && *fn) {
		hz::mfile_t bk;
		auto bkt = bk.open_d(fn, is_rdonly);
		if (bkt) {

			if (p) {
				p->unmap();
				p->close_m();
			}
			else {
				p = new hz::mfile_t();
			}
			*p = bk;
			bk.clear_ptr(); // 转移句柄
			_data = (unsigned char*)bkt;
			_size = p->size();
			line_offset = 0;
			mapfile = p;
			is_update = true;
			return true;
		}
	}
	return false;
}

void hex_editor::save_data(size_t pos, size_t len)
{
	auto p = (hz::mfile_t*)mapfile;
	if (!_rdonly && p) {
		p->flush(pos, len);
	}
}

size_t hex_editor::write_data(const void* d, size_t len, size_t pos, bool save)
{
	size_t r = 0;
	auto p = (hz::mfile_t*)mapfile;
	if (p && !_rdonly)
	{
		p->seek(pos);
		r = p->write_m((char*)d, len, save);
	}
	else if (_data && _size > 0) {
		r = std::min(_size - pos, len);
		memcpy(_data + pos, d, r);
	}
	return r;
}
void printBinary(uint8_t num, std::string* t)
{
	char buf[16] = {};
	auto tt = buf;
	for (int i = sizeof(num) * 8 - 1; i >= 0; i--, tt++) {
		sprintf(tt, "%d", (num >> i) & 1);
	}
	if (t)
	{
		t->append(buf); t->push_back('\n');
	}
}
float bfloat16_to_float(uint16_t val)
{
	union {
		float f;
		unsigned int u;
	} result;

	// 将bfloat16扩展为float 
	result.u = ((unsigned int)val) << 16;
	return result.f;
}
// 读取 ULEB128 编码的无符号整数 
uint64_t read_uleb128(const char* data) {
	uint64_t value = 0;
	int shift = 0;
	unsigned char byte;

	do {
		byte = (unsigned char)*data;
		value |= (uint64_t)(byte & 0x7F) << shift;
		shift += 7;
		data++;
	} while (byte & 0x80);

	return value;
}

// 读取 SLEB128 编码的有符号整数 
int64_t read_sleb128(const char* data) {
	long long value = 0;
	int shift = 0;
	unsigned char byte;

	do {
		byte = (unsigned char)*data;
		value |= (int64_t)(byte & 0x7F) << shift;
		shift += 7;
		data++;
	} while (byte & 0x80);

	// 如果符号扩展需要 
	if ((shift < 64) && (byte & 0x40)) {
		value |= ((int64_t)(-1) << shift);
	}

	return value;
}
void print_data_de(const void* p, std::string* t)
{
	std::string str;
	char buf[256] = {};

	// binary
	printBinary(*(uint8_t*)p, t);
	// 8进制octal(1) uint8(1) int8(1)
	uint8_t num8 = *(uint8_t*)p;
	int8_t num8i = *(int8_t*)p;
	uint16_t num16u = *(uint16_t*)p;
	int16_t num16 = *(int16_t*)p;
	uint32_t num32u = *(uint32_t*)p;
	int32_t num32 = *(int32_t*)p;
	uint64_t num64u = *(uint64_t*)p;
	int64_t num64 = *(int64_t*)p;
	sprintf(buf, "%o\n%u\n%d\n", num8, num8, (int8_t)num8);
	str += buf;
	sprintf(buf, "%u\n%d\n", num16u, num16);
	str += buf;

	uint32_t num24 = 0;
	memcpy(&num24, p, 3);
	sprintf(buf, "%u\n%d\n", num24, (int32_t)num24);
	str += buf;

	sprintf(buf, "%u\n%d\n", num32u, num32);
	str += buf;
	sprintf(buf, "%llu\n%lld\n", num64u, num64);
	str += buf;

	auto ul = read_uleb128((char*)p);
	auto sl = read_sleb128((char*)p);
	sprintf(buf, "%llu\n%lld\n", ul, sl);
	str += buf;

	//float16
	double f16 = glm::detail::toFloat32(num16u);
	double bf16 = bfloat16_to_float(num16u);
	sprintf(buf, "%lg\n%lg\n", f16, bf16);
	str += buf;
	double f32 = *(float*)p;
	double f64 = *(double*)p;

	sprintf(buf, "%lg\n%lg\n", f32, f64);
	str += buf;
	// ascii
	char c = num8;
	sprintf(buf, "%c\n", (c >= 32 && c <= 126) ? c : '.');
	str += buf;
	// 获取utf8
	auto s = (const char*)p;
	auto ss = md::utf8_next_char(s);
	str.append(s, ss); str.push_back('\n');
	// utf16
	{
		auto us16 = md::u16_u8((uint16_t*)p, 2);
		if (us16.size())
		{
			str += us16 + "\n";
		}
		else { str += ".\n"; }
	}
	//gb
	{
		auto g = md::gb_u8((char*)p, 2);
		if (g.size() > 2)
		{
			str += g + "\n";
		}
		else { str += ".\n"; }
	}
	//big5
	{
		auto g = hz::big5_to_u8((char*)p, 2);
		if (g.size() > 2)
		{
			str += g + "\n";
		}
		else { str += ".\n"; }
	}
	//shift_jis
	//{
	//	auto g = hz::shift_jis_to_u8((char*)p, 2);
	//	if (g.size() > 2)
	//	{
	//		str += g + "\n";
	//	}
	//	else { str += ".\n"; }
	//}


	if (t)
	{
		t->append(str);
	}
}
void hex_editor::set_pos(size_t pos)
{
	if (pos < _size) {
		auto p = _data + pos;
		data_inspector.clear();
		auto di = &data_inspector;
		print_data_de(p, di);
	}
}

void hex_editor::set_linepos(size_t pos)
{
	if (pos < _size && line_offset != pos * bytes_per_line) {
		line_offset = pos * bytes_per_line; is_update = true;
	}
}

std::string hex_editor::get_ruler_di()
{
	static std::string s = "binary\noctal\nuint8\nint8\nuint16\nint16\nuint24\nint24\nuint32\nint32\nuint64\nint64\nULEB128\nSLEB128\nfloat16\nbfloat16\nfloat32\nfloat64\nASCII\nUTF-8\nUTF-16\nGB18030\nBIG5\n";// SHIFT - JIS\n";
	return s;
}

char* hex_editor::data()
{
	return (char*)_data;
}

size_t hex_editor::size()
{
	return _size;
}

bool hex_editor::update_hex_editor()
{
	int64_t nsize = _size - line_offset;
	if (nsize > 0 && _data && _size > 0)
	{

	}
	else {
		return false;
	}
	auto file_data = this;
	if (file_data->bytes_per_line < 4)
		file_data->bytes_per_line = 4;
	if (file_data->bytes_per_line > 256)
		file_data->bytes_per_line = 256;
	int bps = file_data->bytes_per_line * 5 + 16;
	if (bps < 128)bps = 128;
	if (file_data->bpline.size() != bps)
		file_data->bpline.resize(bps);
	char* line = file_data->bpline.data();
	int nc = view_size.y / file_data->font_size;
	int anc = _size / file_data->bytes_per_line;
	anc += _size % file_data->bytes_per_line > 0 ? 1 : 0;
	acount = anc;
	int dnc = nsize / file_data->bytes_per_line;
	dnc += nsize % file_data->bytes_per_line > 0 ? 1 : 0;
	int newnc = std::min(nc + 1, dnc);
	int idx = _size > UINT32_MAX ? 1 : 0;// 大于4G则用16位数

	if (count != newnc || file_data->is_update)
	{
		count = newnc;
		is_update = true;
	}
	if (is_update)
	{
		is_update = false;
		line_number.clear();
		data_hex.clear();
		decoded_text.clear();
		if (bytes_per_line * 3 != ruler.size())
		{
			ruler.clear();
			for (size_t j = 0; j < file_data->bytes_per_line; j++) {
				snprintf(line, bps, "%02zx ", j);
				ruler += line;
			}
		}
		auto lpos = line_offset;
		const char* fmt[2] = { "%08zx: \n" ,"%016zx: \n" };
		auto data = file_data->_data + line_offset;
		int lnw = 0;
		for (size_t i = 0; i < nsize && newnc > 0; i += file_data->bytes_per_line, newnc--) {
			snprintf(line, bps, fmt[idx], i + lpos);
			line_number += line;
			if (lnw == 0)
				lnw = line_number.size();
			line[0] = 0;
			for (int j = 0; j < file_data->bytes_per_line; j++) {
				if (i + j < nsize) {
					snprintf(line + strlen(line), bps - strlen(line), "%02x ", data[i + j]);
				}
				else {
					snprintf(line + strlen(line), bps - strlen(line), "   ");
				}
			}
			snprintf(line + strlen(line), bps - strlen(line), " ");
			data_hex += line;
			data_hex.push_back('\n');
			line[0] = 0;
			for (int j = 0; j < file_data->bytes_per_line; j++) {
				if (i + j < nsize) {
					char c = data[i + j];
					snprintf(line + strlen(line), bps - strlen(line), "%c", (c >= 32 && c <= 126) ? c : '.');
				}
				else {
					snprintf(line + strlen(line), bps - strlen(line), " ");
				}
			}
			decoded_text += line;
			decoded_text.push_back('\n');
		}
		line_number_n = lnw;
	}
	return true;
}
#endif
