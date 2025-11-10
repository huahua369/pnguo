
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
#include <stb_image.h>
#include <stb_image_write.h>
#include <stb_rect_pack.h>
// 样条线算法
#include <tinyspline/tinysplinecxx.h>
// 三角化算法
//#include <earcut.hpp>
// 多边形算法
//#include <clipper2/clipper.h> 
//using namespace Clipper2Lib;

#include <pnguo.h>

#include <SDL3/SDL_keycode.h>
#include <print_time.h>
#include <mapView.h>
#include <event.h>
#include <thread>
#ifndef NO_FONT_CX
#include <hb.h>
#include <hb-ot.h>
#include <fontconfig/fontconfig.h> 
#endif




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
		uint64_t p = 0; p = _Val;
		const auto _Len = static_cast<size_t>(_scprintf("%p", (void*)p));
		std::string _Str(_Len, '\0');
		sprintf_s(&_Str[0], _Len + 1, "%p", (void*)p);
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
#if 0

"translate":
{
	"time": 1.9,
		"x" : 0.83,
		"y" : -30.29,
		// 没有curve时则线性插值
		"curve" : [2.078, 1.43, 2.274, 2.88, 2.071, -30.29, 2.289, 4.48]
		"curve" : "stepped"//直接跳到
},

class tfAccessor
{
	const void* _data = NULL;
	int _count = 0;
	int _stride = 0;
	int _dimension = 0;
	int _type = 0;
	glm::vec4 _min = {};
	glm::vec4 _max = {};
};
class tfSampler
{
	tfAccessor _time;	// input
	tfAccessor _value;	// output 
};
class tfChannel
{
	tfSampler* sampler = 0;
	enum { TRANSLATION, ROTATION, SCALE, SHEAR/*, WEIGHTS*/ } transformType = {};
	enum { LINEAR, STEP, CUBIC/*, BEZIER, BSPLINE*/ } interpolation = LINEAR;
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
#endif
void SetAnimationTime(uint32_t animationIndex, float dtime)
{
	std::vector<tfSampler> samplers;		// 动画采样列表
	std::vector<tfAnimation> animations;	// 动画列表
	std::vector<tfNode> nodes;				// 节点列表
	std::vector<glm::mat3> animatedMats;	// 缓存矩阵

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


skeleton_data::skeleton_data()
{
}

skeleton_data::~skeleton_data()
{
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



// 预乘输出bgra，type=0为原数据是rgba
void premultiply_data(int w, unsigned char* data, int type, bool multiply);




vertex_v2::vertex_v2() {}
vertex_v2::vertex_v2(glm::vec3 p, glm::vec2 u, uint32_t  c) :position(p.x, p.y), tex_coord(u) {
	color = colorv4(c);
}
vertex_v2::vertex_v2(glm::vec2 p, glm::vec2 u, uint32_t  c) :position(p), tex_coord(u) {
	color = colorv4(c);
}
vertex_v2::vertex_v2(glm::vec2 p, glm::vec2 u, glm::vec4 c) :position(p), tex_coord(u), color(c) {}


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
size_t bspline_ct::get_num_knots()
{
	return ptr ? ts_bspline_num_knots((tsBSpline*)ptr) : 0;
}
void bspline_ct::set_knot_vector(double* kv, size_t n)
{
	if (!kv || n < 1 || !ptr)return;
	auto t = (tinyspline::BSpline*)ptr;
	size_t expected = ts_bspline_num_knots((tsBSpline*)t);
	size_t actual = n;
	if (expected != actual) {
		std::ostringstream oss;
		oss << "Expected size: " << expected
			<< ", Actual size: " << actual;
		throw std::runtime_error(oss.str());
	}
	else
	{
		tsStatus status;
		if (ts_bspline_set_knots((tsBSpline*)t, kv, &status))
			throw std::runtime_error(status.message);
	}
}
void bspline_ct::set_knot_vector(float* kv, size_t n)
{
	if (!kv || n < 1 || !ptr)return;
	auto t = (tinyspline::BSpline*)ptr;
	std::vector<tsReal> tkv;
	size_t expected = ts_bspline_num_knots((tsBSpline*)t);
	size_t actual = n;
	if (expected != actual) {
		std::ostringstream oss;
		oss << "Expected size: " << expected
			<< ", Actual size: " << actual;
		throw std::runtime_error(oss.str());
	}
	else
	{
		tkv.resize(n);
		for (size_t i = 0; i < n; i++)
		{
			tkv[i] = kv[i];
		}
		tsStatus status;
		if (ts_bspline_set_knots((tsBSpline*)t, tkv.data(), &status))
			throw std::runtime_error(status.message);
	}
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
#define EPSILON8 1e-8
#define EPSILON_NUMERIC 1e-4

#define THREE_SQRT sqrt(3)
#define ONE_THIRD (1.0 / 3.0)
#define INTMAX_2 INTMAX_MAX
#endif

#if 1

#if 1 
bool isAroundZero(float val) {
	return val > -EPSILON8 && val < EPSILON8;
}
bool isNotAroundZero(float val) {
	return val > EPSILON8 || val < -EPSILON8;
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


#define EPSILON 1e-6  // 积分精度阈值 
#define MAX_ITER 100  // 二分法最大迭代次数 

	// 贝塞尔曲线控制点结构体
	typedef struct {
		double x0, y0;    // P0
		double x1, y1;    // P1
		double x2, y2;    // P2
		double x3, y3;    // P3
	} BezierControlPoints;

	// 计算贝塞尔曲线点坐标 
	void bezier_point(double t, BezierControlPoints* ctrl, double* x, double* y) {
		double u = 1 - t;
		*x = pow(u, 3) * ctrl->x0 + 3 * t * pow(u, 2) * ctrl->x1 + 3 * pow(t, 2) * u * ctrl->x2 + pow(t, 3) * ctrl->x3;
		*y = pow(u, 3) * ctrl->y0 + 3 * t * pow(u, 2) * ctrl->y1 + 3 * pow(t, 2) * u * ctrl->y2 + pow(t, 3) * ctrl->y3;
	}

	// 计算贝塞尔曲线导数 
	void bezier_derivative(double t, BezierControlPoints* ctrl, double* dx, double* dy) {
		double u = 1 - t;
		*dx = 3 * pow(u, 2) * (ctrl->x1 - ctrl->x0) + 6 * t * u * (ctrl->x2 - ctrl->x1) + 3 * pow(t, 2) * (ctrl->x3 - ctrl->x2);
		*dy = 3 * pow(u, 2) * (ctrl->y1 - ctrl->y0) + 6 * t * u * (ctrl->y2 - ctrl->y1) + 3 * pow(t, 2) * (ctrl->y3 - ctrl->y2);
	}

	// 自适应辛普森积分计算弧长 
	double adaptive_simpson(double a, double b, BezierControlPoints* ctrl) {
		double c = (a + b) / 2;
		double h = b - a;
		double fa, fb, fc, x1, y1, x2, y2;

		bezier_derivative(a, ctrl, &fa, &x1);
		bezier_derivative(b, ctrl, &fb, &x2);
		bezier_derivative(c, ctrl, &fc, &y1);

		double s = (h / 6) * (sqrt(fa * fa + x1 * x1) + 4 * sqrt(fc * fc + y1 * y1) + sqrt(fb * fb + x2 * x2));
		bezier_derivative((a + c) / 2, ctrl, &fa, &x1);
		bezier_derivative((c + b) / 2, ctrl, &fb, &x2);

		double left = (c - a) / 6 * (sqrt(fa * fa + x1 * x1) + 4 * sqrt(fc * fc + y1 * y1) + sqrt(fb * fb + x2 * x2));
		double right = (b - c) / 6 * (sqrt(fc * fc + y1 * y1) + 4 * sqrt(fb * fb + x2 * x2) + sqrt(fb * fb + x2 * x2));

		if (fabs(s - (left + right)) <= 15 * EPSILON) {
			return left + right + (left + right - s) / 15;
		}
		return adaptive_simpson(a, c, ctrl) + adaptive_simpson(c, b, ctrl);
	}

	// 二分法查找满足弧长条件的t值
	double find_t(double target_length, double a, double b, BezierControlPoints* ctrl) {
		double current_length, mid;
		for (int i = 0; i < MAX_ITER; i++) {
			mid = (a + b) / 2;
			current_length = adaptive_simpson(0, mid, ctrl);
			if (current_length < target_length - EPSILON) {
				a = mid;
			}
			else if (current_length > target_length + EPSILON) {
				b = mid;
			}
			else {
				return mid;
			}
		}
		return (a + b) / 2;
	}

	// 等距采样主函数 
	void equidistant_sample(BezierControlPoints* ctrl, int num_points, std::vector<double>* points) {
		// 计算总长度 
		double total_length = adaptive_simpson(0, 1, ctrl);
		double step = total_length / (num_points - 1);

		points->resize(num_points * 2);

		for (int i = 0; i < num_points; i++) {
			double t = (i == 0) ? 0 : find_t(i * step, 0, 1, ctrl);
			double x, y;
			bezier_point(t, ctrl, &x, &y);
			(*points)[2 * i] = x;
			(*points)[2 * i + 1] = y;
		}
	}


#define CURVE_LINEAR 0
#define CURVE_STEPPED 1
#define CURVE_BEZIER 2
#define BEZIER_SIZE 18
	struct Timeline_t
	{
		std::vector<float> curves;
		std::vector<float> curves1;
		int frameCount = 0;
		int frameEntries = 0;
		int type = 0;				//TimelineType
	};
	void CurveTimeline_setBezier(Timeline_t* timeline, int bezier, int frame, float value, float time1, float value1,
		float cx1, float cy1, float cx2, float cy2, float time2, float value2)
	{
		auto self = timeline;
		float tmpx, tmpy, dddx, dddy, ddx, ddy, dx, dy, x, y;
		int i = self->frameCount + bezier * BEZIER_SIZE, n;
		float* curves = self->curves.data();
		float* curves1 = self->curves1.data();
		if (value == 0) curves[frame] = CURVE_BEZIER + i;
		tmpx = (time1 - cx1 * 2 + cx2) * 0.03;
		tmpy = (value1 - cy1 * 2 + cy2) * 0.03;
		dddx = ((cx1 - cx2) * 3 - time1 + time2) * 0.006;
		dddy = ((cy1 - cy2) * 3 - value1 + value2) * 0.006;
		ddx = tmpx * 2 + dddx;
		ddy = tmpy * 2 + dddy;
		dx = (cx1 - time1) * 0.3 + tmpx + dddx * 0.16666667;
		dy = (cy1 - value1) * 0.3 + tmpy + dddy * 0.16666667;
		x = time1 + dx, y = value1 + dy;
		int ct = BEZIER_SIZE / 2;
		for (n = i + ct; i < n; i++) {
			curves[i] = x;
			curves1[i] = y;//+ 1
			dx += ddx;
			dy += ddy;
			ddx += dddx;
			ddy += dddy;
			x += dx;
			y += dy;
		}
		//for (n = i + BEZIER_SIZE; i < n; i += 2) {
		//	curves[i] = x;
		//	curves[i + 1] = y;
		//	dx += ddx;
		//	dy += ddy;
		//	ddx += dddx;
		//	ddy += dddy;
		//	x += dx;
		//	y += dy;
		//}
	}
	struct rtline_t {
		float time;
		float value;
		glm::vec2 c1;
		glm::vec2 c2;
	};
	void test_timeline()
	{
		rtline_t t[2] = {};
		t[0].time = 0.0;
		t[0].value = -44.7;
		t[0].c1 = { 0.033, -44.7 };
		t[0].c2 = { 0.12, 54.89 };
		t[1].time = 0.1333;
		t[1].value = 64.62;
		t[1].c1 = { 0.154, 79.18 };
		t[1].c2 = { 0.214, 79.42 };
		Timeline_t tt = {};
		tt.curves.resize(BEZIER_SIZE * 2);
		tt.curves1.resize(BEZIER_SIZE * 2);
		std::vector<glm::vec2> r = { {t[0].time, t[0].value}, t[0].c1, t[0].c2, {t[1].time, t[1].value } };
		std::vector<glm::vec2> r2 = { };
		get_bezier(r.data(), r.size(), BEZIER_SIZE, 1, r2);
		std::vector<float> v2;
		for (auto& it : r2) {
			v2.push_back(it.x);
		}
		auto kk = cubicAt1(t[0].value, t[0].c1.y, t[0].c2.y, t[1].value, 0.5f);

		BezierControlPoints ctrl = { 0, 0, 1, 3, 2, -1, 3, 0 };
		int num_points = BEZIER_SIZE;
		std::vector<double> points;

		equidistant_sample(&ctrl, num_points, &points);
		v2.clear();
		for (int i = 0; i < num_points; i++) {
			v2.push_back(points[2 * i + 1]);
			printf("Point %d: (%f, %f)\n", i, points[2 * i], points[2 * i + 1]);
		}


		CurveTimeline_setBezier(&tt, 0, 0, t[0].value, t[0].time, t[0].value, t[0].c1.x, t[0].c1.y, t[0].c2.x, t[0].c2.y, t[1].time, t[1].value);
		printf("");
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

	// 输入三点、目标圆半径。角点为pt2，输出圆心和半径
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
			free(t);
			//auto p = (glm::vec4*)t;
			//delete[] p;
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
			ac += sn;
			auto mem0 = malloc(ac);
			if (!mem0) { p = 0; break; }
			p = (path_node_t*)mem0;
			*p = {};
			auto cp = (char*)mem0;
			cp += sizeof(path_node_t);
			//cp += sn * sizeof(glm::vec2);
			p->lengths = (int*)cp;
			cp += spn * sizeof(int);
			p->pos = (glm::vec2*)cp;
			p->cap = sn;
			for (size_t i = 0; i < spn; i++)
			{
				p->lengths[i] = ls[i];
			}
			p->caplen = spn;
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
		act = sizeof(glm::vec2) * posv.size() + sizeof(glm::vec2) * anglev.size() * sizeof(glm::vec3) * centerv.size() + sizeof(int) * lengths.size();
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
	int get_fv(const glm::vec2& ps, const glm::vec2& a, const glm::vec3& c, size_t num_segments, float z, std::vector<glm::vec3>& tv)
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
	int get_fv_old(fv_it* fv, int i, size_t num_segments, float z, std::vector<glm::vec3>& tv)
	{
		auto ps = fv->pos[i];
		auto a = fv->angle[i];
		auto c = fv->center[i];
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
		PathsD r = {};
		auto fv = *p;
		float z = 0;
		for (size_t x = 0; x < p->count; x++)
		{
			auto nn = *fv.lengths;
			std::vector<glm::vec3> tv1;
			PathD v0 = {};
			for (int i = 0; i < nn; i++)
			{
				tv1.clear();
				get_fv_old(&fv, i, num_segments, z, tv1);
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
	int for_cvertex_old(fv_it* fv, fv_it* fv1, size_t num_segments, std::vector<glm::vec3>& opt, glm::vec2 z)
	{
		auto nn = *(fv->lengths);
		std::vector<glm::vec3> tv1[2], tv2[2], t[2];
		get_fv_old(fv, 0, num_segments, z.x, t[0]);
		get_fv_old(fv1, 0, num_segments, z.y, t[1]);
		tv1[0] = t[0];
		tv2[0] = t[1];
		nn--;
		std::vector<glm::vec3> tv;
		tv.reserve(tv.size() + fv->count * 3);

		//auto a2 = gs::area_ofRingSigned(tv2.data(), tv2.size());
		for (int i = 1; i < nn; i++)
		{
			glm::ivec2 n0, n1;
			n0[0] = tv1[0].size();
			n1[0] = tv2[0].size();
			tv1[1].clear();
			tv2[1].clear();
			get_fv_old(fv, i, num_segments, z.x, tv1[1]);
			get_fv_old(fv1, i, num_segments, z.y, tv2[1]);

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
		fv->inc(*fv->lengths);
		fv1->inc(*fv1->lengths);
		fv->lengths++;
		fv1->lengths++;
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
			get_fv_old(&fv, i, num_segments, z.x, tv1);
		}
		auto nn1 = *fv1.lengths;
		for (int i = 0; i < nn1; i++)
		{
			get_fv_old(&fv1, i, num_segments, z.y, tv2);
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
		PathsD r = {};
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
		if (pt)
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

		gp::fv_it fv(pt1);
		gp::fv_it fv1(pt1);
		std::vector<glm::vec3> opt;
		for (size_t x = 0; x < fv.count; x++)
		{
			if (*fv.lengths == *fv1.lengths)
			{
				for_cvertex_old(&fv, &fv1, c->segments, opt, dz);
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
		gp::fv_it fv(pt1);
		gp::fv_it fv1(pt1);
		std::vector<glm::vec3> opt;
		for (size_t x = 0; x < fv.count; x++)
		{
			if (*fv.lengths == *fv1.lengths)
			{
				for_cvertex_old(&fv, &fv1, c->segments, opt, dz);
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
			gp::fv_it fv(pt);
			gp::fv_it fv1(pt1);
			tv.clear();
			auto n = tv.size();

			if (expand.x == expand.y)
			{
				for (size_t x = 0; x < fv.count; x++)
				{
					gp::for_cvertex_old(&fv, &fv1, c->segments, tv, z);
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
			gp::fv_it fv(pt);
			gp::fv_it fv1(pt1);
			tv.clear();
			auto n = tv.size();
			for (size_t x = 0; x < fv.count && x < fv1.count; x++)
			{
#if 1
				gp::for_cvertex_old(&fv, &fv1, c->segments, tv, z);
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
		gp::get_fv_old(&fv, 0, num_segments, z, t);
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
			gp::get_fv_old(&fv, x, num_segments, z, t);
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
			gp::fv_it fv1(pt1);
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
	std::vector<float> get_vs(njson& n, const char* k)
	{
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
		auto ns = n[k];
		auto c = ns.size();
		for (size_t i = 0; i < c; i += 2)
		{
			glm::ivec2 vt = { toFloat(ns[i]),toFloat(ns[i + 1]) };
			v.push_back(vt);
		}
		return v;
	}
	std::vector<glm::ivec2> get_iv2(njson& n, const char* k)
	{
		std::vector<glm::ivec2> v;
		if (n.find(k) == n.end() || !n[k].is_array() || n[k].size() < 2) { return v; }
		auto ns = n[k];
		auto c = ns.size();
		for (size_t i = 0; i < c; i += 2)
		{
			glm::ivec2 vt = { toInt(ns[i]),toInt(ns[i + 1]) };
			v.push_back(vt);
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
	std::vector<glm::vec4> get_v4(njson& n, const char* k)
	{
		std::vector<glm::vec4> v;
		if (n.find(k) == n.end() || !n[k].is_array() || n[k].size() < 3) { return v; }

		auto ns = n[k];
		auto c = ns.size();
		for (size_t i = 0; i < c; i += 3)
		{
			glm::vec4 vt = { toFloat(ns[i]),toFloat(ns[i + 1]), toFloat(ns[i + 2]), toFloat(ns[i + 3]) };
			v.push_back(vt);
		}
		return v;
	}
	std::vector<glm::ivec4> get_iv4(njson& n, const char* k)
	{
		std::vector<glm::ivec4> v;
		if (n.find(k) == n.end() || !n[k].is_array() || n[k].size() < 4) { return v; }
		auto ns = n[k];
		auto c = ns.size();
		for (size_t i = 0; i < c; i += 4)
		{
			glm::ivec4 vt = { toInt(ns[i]),toInt(ns[i + 1]), toInt(ns[i + 2]), toInt(ns[i + 3]) };
			v.push_back(vt);
		}
		return v;
	}
	std::vector<glm::vec2> get_v2(njson0& n, const char* k)
	{
		std::vector<glm::vec2> v;
		if (n.find(k) == n.end() || !n[k].is_array() || n[k].size() < 2) { return v; }
		auto ns = n[k];
		auto c = ns.size();
		for (size_t i = 0; i < c; i += 2)
		{
			glm::ivec2 vt = { toFloat(ns[i]),toFloat(ns[i + 1]) };
			v.push_back(vt);
		}
		return v;
	}
	std::vector<glm::ivec2> get_iv2(njson0& n, const char* k)
	{
		std::vector<glm::ivec2> v;
		if (n.find(k) == n.end() || !n[k].is_array() || n[k].size() < 2) { return v; }
		auto ns = n[k];
		auto c = ns.size();
		for (size_t i = 0; i < c; i += 2)
		{
			glm::ivec2 vt = { toInt(ns[i]),toInt(ns[i + 1]) };
			v.push_back(vt);
		}
		return v;
	}
	std::vector<glm::vec3> get_v3(njson0& n, const char* k)
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
	std::vector<glm::vec4> get_v4(njson0& n, const char* k)
	{
		std::vector<glm::vec4> v;
		if (n.find(k) == n.end() || !n[k].is_array() || n[k].size() < 3) { return v; }

		auto ns = n[k];
		auto c = ns.size();
		for (size_t i = 0; i < c; i += 3)
		{
			glm::vec4 vt = { toFloat(ns[i]),toFloat(ns[i + 1]), toFloat(ns[i + 2]), toFloat(ns[i + 3]) };
			v.push_back(vt);
		}
		return v;
	}
	std::vector<glm::ivec4> get_iv4(njson0& n, const char* k)
	{
		std::vector<glm::ivec4> v;
		if (n.find(k) == n.end() || !n[k].is_array() || n[k].size() < 4) { return v; }
		auto ns = n[k];
		auto c = ns.size();
		for (size_t i = 0; i < c; i += 4)
		{
			glm::ivec4 vt = { toInt(ns[i]),toInt(ns[i + 1]), toInt(ns[i + 2]), toInt(ns[i + 3]) };
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
		float bh = 0;			// 补高
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
		if (p->bh < 0)
			p->bh = 0;
		// 墙面
		if (ct->step.x > 0)
		{
			switch (ct->type)
			{
			case 0:
				pos += v1.y + v1.z + p->bh;
				v2.y = p->pos + p->bh;
				height1.y = p->pos + v1.z + p->bh;
				height2.x = p->pos + p->bh;
				height2.y = p->pos;
				break;
			case 1:
				pos -= v1.y + v1.z + p->bh;
				v2.y = p->pos - (p->bh + v1.z);
				height1.y = p->pos - (v1.z + p->bh);
				height2.x = p->pos - p->bh;
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
				pos += ct->thickness + p->bh;
				pos.y -= ct->thickness;
				height2.x = p->pos + p->bh;
				height2.y = p->pos;
				break;
			case 1:
				pos -= ct->thickness + p->bh;
				pos.y += ct->thickness;
				height2.x = p->pos - p->bh;
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
	void mkstep(base_mv_t& bmt, mkcustom_dt* n, const glm::vec2& pos, cmd_plane_t* c)
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
			sp.bh = n->step_bh.x;
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
			sp.bh = n->step_bh.y;
			sp.pos = pos.y;
			sp.expand = se2.y;
			sp.expand0 = se20.y;
			new_step(&clt, &sp);
		}
	}
	// 生成B样条线约束的竖三角面
	glm::vec4 mkcustom(mkcustom_dt* np, base_mv_t& bm, cmd_plane_t* c, const glm::uvec2& bcount)
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
			mkstep(bm, np, sth, c);
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

#ifndef c_KAPPA90
#define c_KAPPA90 (0.5522847493f)	// Length proportional to radius of a cubic bezier handle for 90deg arcs.
#endif


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

size_t path_v::get_line_count()
{
	auto n = _data.size();
	auto p = _data.data();
	size_t xx = 0;
	for (size_t i = 0; i < n; i++, p++)
	{
		if (p->type == vtype_e::e_vmove)
		{
			xx++;
		}
	}
	return xx;
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
	int64_t n = size;
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
int pnpoly_aos0(int nvert, const float* verts, float testx, float testy) {
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
glm::vec2 pnpoly_aos0(int nvert, const glm::vec2* verts, const glm::vec2& test) {
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
			auto bd = pnpoly_aos0(n, st.data(), ps);// 返回是否在多边形内、最短距离
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


int inflate2flatten(path_v* p, path_v* dst, inflate_t* t)
{
	if (!p || !dst || !t || p->size() == 0 || !(t->width > 0) || t->segments < 1)
		return -1;
	auto ct = p->mcount();
	if (ct < 1)return -1;
	PathsD pd;
	std::vector<PointD> pv;
	std::vector<glm::vec2> flatten;
	for (size_t x = 0; x < ct; x++)
	{
		pv.clear();
		flatten.clear();
		auto vt = get_idxlines(p->_data, x, 1);
		path_v::flatten_t fp = {};
		fp.flatten = &flatten;
		fp.mc = t->segments;
		fp.mlen = t->mlen;
		fp.n = vt.n;
		fp.first = vt.first;
		fp.dist = t->ds;
		fp.angle = t->angle;
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
	if (t->width != 0 && pd.size())
	{
		auto type = t->type;
		//Square=0, Round=1, Miter=2
		if (type > (int)JoinType::Miter || type < 0)
		{
			type = (int)JoinType::Round;
		}
		auto etype = std::clamp(t->etype, (int)EndType::Polygon, (int)EndType::Round);
		// 扩展线段
		auto rv = InflatePaths(pd, t->width, (JoinType)type, (EndType)etype);
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
		if (t->is_reverse)
		{
			std::reverse(flatten.begin(), flatten.end());
		}
		dst->add_lines(flatten.data(), flatten.size(), t->is_close);
	}
	return 0;
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
				auto bd = pnpoly_aos0(n, st.data(), ps);// 返回是否在多边形内、最短距离
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
				auto bd = pnpoly_aos0(n, st.data(), ps);// 返回是否在多边形内、最短距离
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


#endif // 1



void set_grid_repeat(grid_div* p, size_t rows, size_t columns, float w, float h)
{
	if (!p)return;
	p->rows = rows; p->columns = columns;
	p->rows_per = w; p->columns_per = h;
	p->cellw = 0; p->cellh = 0;
}

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

//// align val to the next multiple of alignment
//uint64_t align_up(uint64_t val, uint64_t alignment)
//{
//	return (val + alignment - (uint64_t)1) & ~(alignment - (uint64_t)1);
//}
//// align val to the previous multiple of alignment
//uint64_t align_down(uint64_t val, uint64_t alignment)
//{
//	return val & ~(alignment - (uint64_t)1);
//}
//uint64_t divideroundingup(uint64_t a, uint64_t b)
//{
//	return (a + b - (uint64_t)1) / b;
//}


astar_search::astar_search()
{
}

astar_search::~astar_search()
{
	if (_node)
	{
		delete[]_node; _node = 0;
	}
	if (m)free(m); m = 0;
}
void astar_search::init(uint32_t w, uint32_t h, uint8_t* d, bool copydata)
{
	if (w < 2 || h < 2 || !d)
	{
		return;
	}
	width = w;
	height = h;
	data = d;
	size = w * h;
	if (cap_size < size)
	{
		cap_size = align_up(size, 64);
		if (_node)
		{
			delete[]_node; _node = 0;
		}
	}
	_node = new grid_node[size];
	memset(_node, 0, size * sizeof(grid_node));
	if (copydata)
	{
		if (m)free(m); m = 0;
		m = malloc(size);
		memccpy(m, d, size, 1);
		data = (uint8_t*)m;
	}
}
void astar_search::set_wallheight(int h) {
	wall = h;
}
void astar_search::set_speed(float* d, int n)
{
	if (d && n > 0)
	{
		speeds.resize(n, 1.0);
		memcpy(speeds.data(), d, n * sizeof(float));
	}
}
// 计算距离
size_t CalcDistance(glm::ivec2 p1, glm::ivec2 p2, double speed)
{
	auto d = glm::abs(p1 - p2);
	return (d.x + d.y) * speed;
}

grid_node* astar_search::pop_n()
{
	grid_node* node = 0;
	node = _open.top();
	_open.pop();
	return node;
}
uint8_t* astar_search::Get(uint8_t* d, size_t i)
{
	if (i >= size)
		i = size - 1;

	return (d + stride * i);
}

bool astar_search::FindPath(glm::ivec2* pStart, glm::ivec2* pEnd, bool mode)
{
	/* 终点开始查找 */
	glm::ivec2 start = *pEnd;
	glm::ivec2 end = *pStart;

	bool isOK = false;
	static int x[] = { -1,-1,0,1,1,1,0,-1 };
	static int y[] = { 0,-1,-1,-1,0,1,1,1 };

	grid_node* node = _node;
	memset(node, 0, size * sizeof(grid_node));

	size_t index = width * start.y + start.x;
	if (index >= size)
		return false;
	while (_open.size())
		_open.pop();
	node[index].start = G_CHECK;
	node[index].end = CalcDistance(start, end, 1.0);
	node[index].total = node[index].end;
	node[index].parent;
	_open.push(&node[index]); //将起点放入开放列表
	auto sp = speeds.data();
	uint8_t spn = speeds.size();
	if (speeds.empty())
	{
		sp = nullptr;
	}
	while (_open.size() != 0 && isOK == false)
	{
		/* 在待检链表中取出 F(总距离) 最小的节点, 并将其选为当前点 */
		auto np = pop_n();
		size_t now_index = np - node;
		np->state = G_CLOSE;
		glm::ivec2 cur_pos =
		{
			(int)(now_index % width),
			(int)(now_index / width)
		};

		/* 遍历当前坐标的八个相邻坐标 */
		for (int i = 0; i < 8; i++)
		{
			if (mode && i % 2)
				continue;
			glm::ivec2 beside_pos =
			{
				cur_pos.x + x[i],
				cur_pos.y + y[i]
			};

			/* 检查坐标有效性 */
			if (beside_pos.x < 0 || (size_t)beside_pos.x >= width
				|| beside_pos.y < 0 || (size_t)beside_pos.y >= height)
				continue;

			size_t beside_index = width * beside_pos.y + beside_pos.x;
			auto kd = Get(data, beside_index);
			if (beside_index < 1 || beside_index < width || *kd > wall || beside_index + 1 >= size || beside_index + width >= size)
				continue;
			switch (i)
			{
			case 1:
				if (*Get(data, beside_index + 1) > wall || *Get(data, beside_index + width) > wall)
					continue;
			case 3:
				if (*Get(data, beside_index - 1) > wall || *Get(data, beside_index + width) > wall)
					continue;
			case 5:
				if (*Get(data, beside_index - 1) > wall || *Get(data, beside_index - width) > wall)
					continue;
			case 7:
				if (*Get(data, beside_index + 1) > wall || *Get(data, beside_index - width) > wall)
					continue;
			}

			/* 检查是否已到达终点 */
			if (beside_pos.x == end.x && beside_pos.y == end.y)
			{
				isOK = true;
				node[beside_index].parent = cur_pos;
				break;
			}

			size_t g = ((i % 2) ? gdist.y : gdist.x);// +abs(data[now_index] - data[beside_index]);
			double speed = 1.0;
			auto idx = std::min(*kd, spn);
			if (sp)speed = sp[idx];
			if (node[beside_index].state == G_UNKNOWN)
			{
				/* 放入待检链表中 */
				node[beside_index].state = G_CHECK;
				node[beside_index].start = node[now_index].start + g;
				node[beside_index].end = CalcDistance(beside_pos, end, speed);
				node[beside_index].total = node[beside_index].start + node[beside_index].end;
				node[beside_index].parent = cur_pos;

				size_t total = node[beside_index].total;
				_open.push(&node[beside_index]); //将当前点放入开放列表 
			}
			else if (node[beside_index].state == G_CHECK)
			{
				/* 如果将当前点设为父 G(距起点) 值是否更小 */
				if (node[beside_index].start > node[now_index].start + g)
				{
					node[beside_index].parent = cur_pos;
					node[beside_index].start = node[now_index].start + g;
					node[beside_index].total = node[beside_index].start + node[beside_index].end;
				}
			}
		}
	}
	if (isOK) {
		glm::ivec2 current = end;
		path.clear();
		while (current != start) {
			path.push_back(current);
			size_t idx = width * current.y + current.x;
			current = node[idx].parent;
		}
		path.push_back(start);
		std::reverse(path.begin(), path.end());
		// 此时path存储了从起点到终点的坐标序列 
	}
	return isOK;
}

bool astar_search::NextPath(glm::ivec2* pos)
{
	size_t index = pos->y * width + pos->x;
	if (index >= 0 && index < size)
	{
		*pos = _node[index].parent;
		return true;
	}
	return false;
}


maze_cx::maze_cx()
{
}

maze_cx::~maze_cx()
{
}
void maze_cx::init(int w, int h)
{
	width = w;
	height = h;
	gen.seed(std::chrono::system_clock::now().time_since_epoch().count());
	//_map_way.resize(w * h, WALL);
	_map_way.resize(w * h, WAY);
	generateMazeD(_map_way.data(), width, height);
}

uint8_t* maze_cx::data()
{
	return _map_way.data();
}

void maze_cx::set_seed(uint64_t c)
{
	seed = c;
	gen_map.seed(c);
}

int64_t maze_cx::get_rand(int64_t f, int64_t s)
{
	auto d = gen();
	return f + d % (s - f + 1);
}
int64_t maze_cx::get_rand_m(int64_t f, int64_t s)
{
	auto d = gen_map();
	return f + d % (s - f + 1);
}

#if 0
// 打乱方向数组 
void shuffle(int* array, int n) {
	for (int i = n - 1; i > 0; i--) {
		int j = rand() % (i + 1);
		int temp = array[i];
		array[i] = array[j];
		array[j] = temp;
	}
}
// 生成迷宫的递归函数 
void maze_cx::generateMaze(int x, int y) {
	int directions[] = { 0, 1, 2, 3 };
	shuffle(directions, 4);
	auto maze = (uint8_t*)_map_way.data();
	for (int i = 0; i < 4; i++) {
		int dx = 0, dy = 0;
		switch (directions[i]) {
		case 0: dx = 1; break;
		case 1: dx = -1; break;
		case 2: dy = 1; break;
		case 3: dy = -1; break;
		}
		int nx = x + 2 * dx, ny = y + 2 * dy;
		if (nx >= 0 && nx < width && ny >= 0 && ny < height && maze[nx + ny * width] == WALL) {
			maze[nx + ny * width] = WAY;
			maze[(x + dx) + (y + dy) * width] = WAY;
			generateMaze(nx, ny);
		}
	}
}
#endif
void maze_cx::generateMazeD(uint8_t* maze, int rows, int cols)
{
	// 初始化迷宫地图为全部为 '#'
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			maze[i + j * rows] = WALL;
		}
	}

	// 随机选择起点和终点（位置不能在边界上）
	//srand((unsigned int)time(NULL));
	int start_x, start_y, dest_x, dest_y;
	do {
		start_x = get_rand_m(1, rows - 1);// rand() % (rows - 2) + 1;
		start_y = get_rand_m(1, cols - 1);// rand() % (cols - 2) + 1;
		dest_x = get_rand_m(1, rows - 1);// rand() % (rows - 2) + 1;
		dest_y = get_rand_m(1, cols - 1); //rand() % (cols - 2) + 1;
	} while (start_x == dest_x && start_y == dest_y);

	// 生成迷宫
	maze[start_x + start_y * rows] = WAY;         // 起点设为可走
	maze[dest_x + dest_y * rows] = WAY;           // 终点设为可走
	std::vector<int> visiteds;
	visiteds.resize(rows * cols, 0);//全部初始为墙
	int* visited = visiteds.data();// [rows] [cols] ;
	// 向四个方向移动 
	glm::ivec2 d[4] = { {-1,0},{0,1},{1,0},{0,-1} };
	std::stack<glm::ivec2> stack_x; // 模拟栈，用于回溯
	start = { start_x , start_y };
	dest = { dest_x , dest_y };
	int top = 0;
	stack_x.push({ start_x,start_y });
	visited[start_x + start_y * rows] = 1;
	while (stack_x.size()) {
		auto ps = stack_x.top();
		int x = ps.x;
		int y = ps.y;
		int flag = 0;
		for (int i = 0; i < 4; i++) {
			int new_x = x + d[i].x;
			int new_y = y + d[i].y;
			auto md = maze[new_x + new_y * rows];
			if (new_x >= 1 && new_x < rows - 1 && new_y >= 1 && new_y < cols - 1 && md == WALL && visited[new_x + new_y * rows] == 0)
			{
				flag = 1;
				break;
			}
		}
		if (flag) {
			int r = get_rand_m(0, 3);// rand() % 4;
			int new_x = x + d[r].x;
			int new_y = y + d[r].y;
			if (new_x >= 1 && new_x < rows - 1 && new_y >= 1 && new_y < cols - 1 && maze[new_x + new_y * rows] == WALL && visited[new_x + new_y * rows] == 0)
			{
				maze[(x + new_x) / 2 + ((y + new_y) / 2 * rows)] = WAY;  // 打通两个相邻格子之间的墙
				visited[new_x + new_y * rows] = 1;
				stack_x.push({ new_x,new_y });
				top++;
			}
		}
		else {
			top--;
			stack_x.pop();
		}
	}
	return;
}
