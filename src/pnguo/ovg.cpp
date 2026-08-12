/*
2026/8/8
*/

#ifndef GLM_FORCE_XYZW_ONLY 
#define GLM_ENABLE_EXPERIMENTAL
//#define GLM_FORCE_ALIGNED
//#define GLM_FORCE_INTRINSICS
// 定义glm启用simd
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_XYZW_ONLY
#include <glm/glm.hpp>  

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/closest_point.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp> 
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/euler_angles.hpp>
#endif



#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <vulkan/vulkan.h>

#include "ovg.h"

#ifndef USE_VMA_OFF
#define VMA_IMPLEMENTATION
#endif
#include <vk_mem_alloc.h>

#include <array>
#include <map>
#include <memory_resource>

void init_ovg_cb(ovg_canvas_cb* cb);

#ifndef MEMAC_PMR
template<class _Ty>
using pmalloc_t = std::pmr::polymorphic_allocator<_Ty>;		// 指定类型内存分配
using uspool_t = std::pmr::unsynchronized_pool_resource;	// 线程不安全
using mbpool_t = std::pmr::monotonic_buffer_resource;		// 线程不安全，多次分配，统一释放
using spool_t = std::pmr::synchronized_pool_resource;		// 线程安全的

class usp_ac_cx
{
public:
	uspool_t _alloc = {};				// pmr内存分配
	size_t _Align = 16;
public:
	usp_ac_cx() {}
	~usp_ac_cx() {}
public:
	void* allocate(const size_t _Bytes, const size_t align = 0) {
		return  _alloc.allocate(_Bytes, align > 0 ? align : _Align);
	}
	void* new_mem(size_t n)
	{
		n = std::max((size_t)1, n);
		auto p = _alloc.allocate(n, _Align);
		memset(p, 0, n);
		return p;
	}
	void* new_mem0(size_t n)
	{
		n = std::max((size_t)1, n);
		auto p = _alloc.allocate(n, _Align);
		return p;
	}
	template<class T>
	T* new_mem(size_t n)
	{
		n = std::max((size_t)1, n);
		auto p = (T*)_alloc.allocate(sizeof(T) * n, _Align);
		auto ptr = p;
		for (int i = 0; i < n; i++)
		{
			p[i] = {};
		}
		return p;
	}
	template<class T >
	T* new_mem(size_t n, T*& p)
	{
		n = std::max((size_t)1, n);
		p = (T*)_alloc.allocate(sizeof(T) * n, _Align);
		auto ptr = p;
		for (int i = 0; i < n; i++)
		{
			p[i] = {};
		}
		return p;
	}
	template<class T >
	T* new_mem_o(size_t n)
	{
		n = std::max((size_t)1, n);
		auto p = (T*)_alloc.allocate(sizeof(T) * n, _Align);
		return p;
	}
	template<class T>
	T* new_mem(T*& p, size_t n)
	{
		return new_mem(n, p);
	}
	template<class T>
	void free_mem(T* t, size_t n)
	{
		auto ptr = t;
		if (t && n > 0)
		{
			_alloc.deallocate(t, sizeof(T) * n, _Align);
		}
	}
	void free_mem0(void* t, size_t n)
	{
		auto ptr = t;
		if (t && n > 0)
		{
			_alloc.deallocate(t, n, _Align);
		}
	}
	template<class T, class... Ts>
	T* new_obj(Ts &&... args)
	{
		auto p = (T*)new_mem(sizeof(T));
		if (p)
		{
#ifdef _WIN32
			std::uninitialized_construct_using_allocator(p, _alloc, std::forward<Ts>(args)...);
#else
			std::__uninitialized_construct_using_allocator(p, _alloc, std::forward<Ts>(args)...);
#endif
		}
		return p;
	}
	template<class T>
	void free_obj(T* t)
	{
		auto ptr = t;
		if (t)
		{
			std::destroy_at(ptr);
			_alloc.deallocate(t, sizeof(T), _Align);
		}
	}
};

#endif // !MEMAC_PMR

// todo 引用spv
#if 1
#include "shaders/spv_c/a_vg.vert.h"
#include "shaders/spv_c/a_vg.frag.h"

#include "shaders/spv_c/a_base3d.vert.h"
#include "shaders/spv_c/a_base3d.frag.h"

#include "shaders/spv_c/a_base3d_mask.vert.h"
#include "shaders/spv_c/a_base3d_mask.frag.h"

#include "shaders/spv_c/a_base3d_dsc.vert.h"
#include "shaders/spv_c/a_base3d_dsc.frag.h"

#include "shaders/spv_c/a_base3d_inst.vert.h"
#include "shaders/spv_c/a_base3d_inst.frag.h"

#include "shaders/spv_c/a_base3d_dsc_inst.vert.h"
#include "shaders/spv_c/a_base3d_dsc_inst.frag.h"
#endif // 1
struct shadermodule_vf {
	VkShaderModule vert;
	VkShaderModule frag;
};
struct spv_u2 {
	const uint32_t* vert;
	const uint32_t* frag;
};

struct spv_len2 {
	size_t vert;
	size_t frag;
};
spv_u2 code[] = { vg_vert, vg_frag,					// 矢量图管线						2d
	a_base3d_vert,	a_base3d_frag,					// 普通三角形(纹理)					2d/3d
	a_base3d_mask_vert,	a_base3d_mask_frag,			// 普通三角形+遮罩纹理				2d/3d
	a_base3d_dsc_vert,	a_base3d_dsc_frag,			// 双面三角形(两种颜色/纹理)			
	a_base3d_inst_vert,	a_base3d_inst_frag,			// 三角形(纹理)实例化					
	a_base3d_dsc_inst_vert,	a_base3d_dsc_inst_frag,	// 双面三角形(两种颜色/纹理)实例化		
};
spv_len2 code_len[] = { sizeof(vg_vert) , sizeof(vg_frag),
	sizeof(a_base3d_vert), sizeof(a_base3d_frag),
	sizeof(a_base3d_mask_vert), sizeof(a_base3d_mask_frag),
	sizeof(a_base3d_dsc_vert), sizeof(a_base3d_dsc_frag),
	sizeof(a_base3d_inst_vert), sizeof(a_base3d_inst_frag),
	sizeof(a_base3d_dsc_inst_vert), sizeof(a_base3d_dsc_inst_frag), };

ovg_canvas_cb* new_canvas_cb()
{
	auto p = new ovg_canvas_cb();
	auto ac = new usp_ac_cx();
	p->ac = (mem_resource_t*)ac;
	init_ovg_cb(p);
	return p;
}
void free_canvas_cb(ovg_canvas_cb* p) {
	if (p)delete p;
}
// vg
#if 1
struct ovg_path_t {
	usp_ac_cx* ac = 0;
	std::pmr::vector<glm::vec2> points;	// 点数组
	std::pmr::vector<uint32_t> pathes;	// 每段大小
	std::pmr::vector<uint32_t> colors;	// 颜色数组，和pathes大小一样
	uint32_t color = 0xffffffff;		// 默认颜色
	uint32_t segmentPtr;   // current segment count in current path having curves
	uint32_t subpathCount; // store count of subpath, not straight forward to retrieve from segmented path array

	uint32_t  pathPtr = 0;		// 路径数组中的指针pointer in the path array  
	vg_state_save_t* t = 0;
	uint32_t curVertOffset = 0;
	bool     simpleConvex; // true if path is single rect or concave closed curve.
};

#define PATH_CLOSED_BIT 0x80000000 /* most significant bit of path elmts is closed/open path state */
#define PATH_HAS_CURVES_BIT                                                                                            \
    0x40000000                        /* 2rd most significant bit of path elmts is curved status                       \
                                       * for main path, this indicate that curve datas are present.                    \
                                       * For segments, this indicate that the segment is curved or not */
#define PATH_IS_CONVEX_BIT 0x20000000 /* simple rectangle or circle. */
#define PATH_ELT_MASK      0x1FFFFFFF /* Bit mask for fetching path element value */

#define ROUNDF(f, c)       (((float)((int)((f) * (c))) / (c)))
#define ROUND_DOWN(v, p)   (floorf(v * p) / p)
#define EQUF(a, b)         (fabsf(a - (b)) <= FLT_EPSILON)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_2_PI 0.63661977236758134308 // 2/pi
#endif
void _matrix_get_scale(const glm::mat3x2* matrix, float* sx, float* sy) {
	auto c0 = (*matrix)[0];
	auto c1 = (*matrix)[1];
	auto c2 = (*matrix)[2];
	*sx = sqrt(c0.x * c0.x + c1.x * c1.x);
	/*if (matrix->xx < 0)
	 *sx = -*sx;*/
	*sy = sqrt(c0.y * c0.y + c1.y * c1.y);
	/*if (matrix->yy < 0)
	 *sy = -*sy;*/
}

void o_finish_path(ovg_path_t* ctx) {
	if (!ctx)return;
	do {
		if (ctx->pathes.empty())
			ctx->pathes.push_back(0);
		if (ctx->pathes[ctx->pathPtr] == 0) // empty
			break;
		if ((ctx->pathes[ctx->pathPtr] & PATH_ELT_MASK) < 2) {
			// only current pos is in path
			auto pointCount = ctx->points.size();
			pointCount -= ctx->pathes[ctx->pathPtr]; // what about the bounds?
			ctx->points.resize(pointCount);
			ctx->pathes[ctx->pathPtr] = 0;
			ctx->segmentPtr = 0;
			break;
		}

		if (ctx->pathPtr == 0 && ctx->simpleConvex)
			ctx->pathes[0] |= PATH_IS_CONVEX_BIT;

		if (ctx->segmentPtr > 0) { // pathes having curves are segmented
			ctx->pathes[ctx->pathPtr] |= PATH_HAS_CURVES_BIT;
			// curved segment increment segmentPtr on curve end,
			// so if last segment is not a curve and point count > 0
			if ((ctx->pathes[ctx->pathPtr + ctx->segmentPtr] & PATH_HAS_CURVES_BIT) == 0 &&
				(ctx->pathes[ctx->pathPtr + ctx->segmentPtr] & PATH_ELT_MASK) > 0)
				ctx->segmentPtr++; // current segment has to be included
			ctx->pathPtr += ctx->segmentPtr;
		}
		else
			ctx->pathPtr++;

		if (ctx->pathes.size() <= ctx->pathPtr)
			ctx->pathes.resize(ctx->pathPtr + 1);

		ctx->pathes[ctx->pathPtr] = 0;
		ctx->segmentPtr = 0;
		ctx->subpathCount++;
		ctx->simpleConvex = false;
	} while (0);

}
void o_remove_last_point(ovg_path_t* ctx) {
	ctx->points.pop_back();
	ctx->pathes[ctx->pathPtr]--;
	if (ctx->segmentPtr > 0) {                            // if path is segmented
		if (!ctx->pathes[ctx->pathPtr + ctx->segmentPtr]) // if current segment is empty
			ctx->segmentPtr--;
		ctx->pathes[ctx->pathPtr + ctx->segmentPtr]--;                          // decrement last segment point count
		if ((ctx->pathes[ctx->pathPtr + ctx->segmentPtr] & PATH_ELT_MASK) == 0) // if no point left (was only one)
			ctx->pathes[ctx->pathPtr + ctx->segmentPtr] = 0;                    // reset current segment
		else if (ctx->pathes[ctx->pathPtr + ctx->segmentPtr] & PATH_HAS_CURVES_BIT) // if segment is a curve
			ctx->segmentPtr++; // then segPtr has to be forwarded to new segment
	}
	if (ctx->pathes.size() < ctx->segmentPtr + ctx->pathPtr)
		ctx->pathes.resize(ctx->segmentPtr + ctx->pathPtr + 1);
}
// test equality of two single precision vectors
inline bool vec2_equ(const glm::vec2& a, const glm::vec2& b) { return (EQUF(a.x, b.x) & EQUF(a.y, b.y)); }
inline glm::vec2 vec2_line_norm(const glm::vec2& a, const glm::vec2& b) {
	glm::vec2  d = { b.x - a.x, b.y - a.y };
	float md = sqrtf(d.x * d.x + d.y * d.y);
	d.x /= md;
	d.y /= md;
	return d;
}
// compute sum of two single precision vectors
inline glm::vec2 vec2_add(const glm::vec2& a, const glm::vec2& b) { return glm::vec2{ a.x + b.x, a.y + b.y }; }
// compute subbstraction of two single precision vectors
inline glm::vec2 vec2_sub(const glm::vec2& a, const glm::vec2& b) { return glm::vec2{ a.x - b.x, a.y - b.y }; }
// multiply 2d vector by scalar
inline glm::vec2 vec2_mult_s(const glm::vec2& a, float m) { return glm::vec2{ a.x * m, a.y * m }; }
// devide 2d vector by scalar
inline glm::vec2 vec2_div_s(const glm::vec2& a, float m) { return glm::vec2{ a.x / m, a.y / m }; }
// normalize float vector
inline glm::vec2 vec2_norm(const glm::vec2& a) {
	float m = sqrtf(a.x * a.x + a.y * a.y);
	return glm::vec2{ a.x / m, a.y / m };
}
inline glm::vec2 vec2_perp(const glm::vec2& a) { return glm::vec2{ a.y, -a.x }; }

void matrix_transform_distance(const glm::mat3x2* matrix, float* dx, float* dy) {
	float new_x, new_y;
	auto m = *matrix;
	new_x = (m[0].x * *dx + m[1].x * *dy);
	new_y = (m[0].y * *dx + m[1].y * *dy);

	*dx = new_x;
	*dy = new_y;
}

void matrix_transform_point(const glm::mat3x2* matrix, float* x, float* y) {
	glm::mat3x2 m = *matrix;
	glm::vec3 v = { *x,*y,1.0f };
	auto vv = m * v;
	*x = v.x; *y = v.y;
}

inline float vec2_zcross(const glm::vec2& v1, const glm::vec2& v2) { return v1.x * v2.y - v1.y * v2.x; }

#ifndef VG_COL32_A_MASK
#define VG_COL32_A_MASK     0xFF000000
#endif // !VG_COL32_A_MASK
#ifndef FIXNORMAL2F_MAX_INVLEN2
void normalize2f_over_zero(float& VX, float& VY)
{
	float d2 = VX * VX + VY * VY;
	if (d2 > 0.0f) {
		float inv_len = 1.0f / sqrtf(d2);
		VX *= inv_len; VY *= inv_len;
	}
}
#define FIXNORMAL2F_MAX_INVLEN2          100.0f // 500.0f (see #4053, #3366)
void fixnormal2f(float& VX, float& VY)
{
	float d2 = VX * VX + VY * VY;
	if (d2 > 0.000001f) {
		float inv_len2 = 1.0f / d2;
		if (inv_len2 > FIXNORMAL2F_MAX_INVLEN2)
			inv_len2 = FIXNORMAL2F_MAX_INVLEN2;
		VX *= inv_len2; VY *= inv_len2;
	}
}
#endif
bool o_path_has_curves(uint32_t* pathes, uint32_t ptrPath) { return   pathes[ptrPath] & PATH_HAS_CURVES_BIT; }

void _ovg_path_extents(ovg_path_t* ctx, bool transformed, float* x1, float* y1, float* x2, float* y2) {
	uint32_t ptrPath = 0;
	uint32_t firstPtIdx = 0;

	float xMin = FLT_MAX, yMin = FLT_MAX;
	float xMax = FLT_MIN, yMax = FLT_MIN;

	while (ptrPath < ctx->pathPtr) {
		uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;

		for (uint32_t i = firstPtIdx; i < firstPtIdx + pathPointCount; i++) {
			glm::vec2 p = ctx->points[i];
			if (transformed)
				matrix_transform_point(&ctx->t->pushConsts.mat, &p.x, &p.y);
			if (p.x < xMin)
				xMin = p.x;
			if (p.x > xMax)
				xMax = p.x;
			if (p.y < yMin)
				yMin = p.y;
			if (p.y > yMax)
				yMax = p.y;
		}

		firstPtIdx += pathPointCount;
		if (o_path_has_curves(ctx->pathes.data(), ptrPath)) {
			// skip segments lengths used in stroke
			ptrPath++;
			uint32_t totPts = 0;
			while (totPts < pathPointCount)
				totPts += (ctx->pathes[ptrPath++] & PATH_ELT_MASK);
		}
		else
			ptrPath++;
	}
	*x1 = xMin;
	*x2 = xMax;
	*y1 = yMin;
	*y2 = yMax;
}
bool _current_path_is_empty(ovg_path_t* ctx) {
	return ctx && (ctx->pathes.empty() || ctx->pathes[ctx->pathPtr] == 0);
}
// this function expect that current point exists
glm::vec2 _get_current_position(ovg_path_t* ctx) {
	return ctx->points.empty() ? glm::vec2() : ctx->points.back();
}

glm::vec2 _get_current_point(ovg_path_t* ctx) {
	glm::vec2 cp = {};
	if (_current_path_is_empty(ctx)) {
	}
	else
	{
		cp = _get_current_position(ctx);
	}
	return cp;
}

void _set_curve_start(ovg_path_t* ctx) {
	if (ctx->segmentPtr > 0) {
		// check if current segment has points (straight)
		if ((ctx->pathes[ctx->pathPtr + ctx->segmentPtr] & PATH_ELT_MASK) > 0)
			ctx->segmentPtr++;
	}
	else {
		// not yet segmented path, first segment length is copied
		if (ctx->pathes[ctx->pathPtr] > 0) { // create first straight segment first
			ctx->pathes.push_back(ctx->pathes[ctx->pathPtr]);
			ctx->segmentPtr = 2;
		}
		else
			ctx->segmentPtr = 1;
	}
	//_check_pathes_array(ctx);
	if (ctx->pathes.size() <= ctx->pathPtr + ctx->segmentPtr)
		ctx->pathes.resize(ctx->pathPtr + ctx->segmentPtr + 1);
	ctx->pathes[ctx->pathPtr + ctx->segmentPtr] = 0;
}
void _set_curve_end(ovg_path_t* ctx) {
	ctx->pathes[ctx->pathPtr + ctx->segmentPtr] |= PATH_HAS_CURVES_BIT;
	ctx->segmentPtr++;
	ctx->pathes.push_back(0);
}
bool _path_is_closed(ovg_path_t* ctx, uint32_t ptrPath) { return ctx->pathes[ptrPath] & PATH_CLOSED_BIT; }
void _add_point(ovg_path_t* ctx, float x, float y) {
	if (isnan(x) || isnan(y)) {
		return;
	}
	glm::vec2 v = { x, y };
	ctx->points.push_back(v);
	if (ctx->pathes.size() <= ctx->pathPtr + ctx->segmentPtr)
		ctx->pathes.resize(ctx->pathPtr + ctx->segmentPtr + 1);
	ctx->pathes[ctx->pathPtr]++; // total point count in path
	if (ctx->segmentPtr > 0)
		ctx->pathes[ctx->pathPtr + ctx->segmentPtr]++; // total point count in path's segment
}

void _line_to(ovg_path_t* ctx, float x, float y) {
	glm::vec2 p = { x, y };
	if (!_current_path_is_empty(ctx)) {
		// prevent adding the same point
		if (vec2_equ(_get_current_position(ctx), p))
			return;
	}
	_add_point(ctx, x, y);
	ctx->simpleConvex = false;
}

float _get_arc_step(ovg_path_t* ctx, float radius) {
	float sx = 1.0, sy = 1.0;
	if (ctx->t)
		_matrix_get_scale(&ctx->t->pushConsts.mat, &sx, &sy);
	float r = radius * fabsf(fmaxf(sx, sy));
	if (r < 30.0f)
		return fminf(M_PI / 3.f, M_PI / r);
	return fminf(M_PI / 3.f, M_PI / (r * 0.4f));
}

void ovg_move_to(ovg_path_t* path, float x, float y);
void ovg_line_to(ovg_path_t* path, float x, float y);
void ovg_quadratic_to(ovg_path_t* path, float x1, float y1, float x2, float y2);
void ovg_curve_to(ovg_path_t* path, float x1, float y1, float x2, float y2, float x3, float y3);
void ovg_elliptic_arc_to(ovg_path_t* path, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);

#define M_APPROXIMATION_SCALE         1.0
#define M_ANGLE_TOLERANCE             0.01
#define M_CUSP_LIMIT                  0.01
#define CURVE_RECURSION_LIMIT         100
#define CURVE_COLLINEARITY_EPSILON    1.7
#define CURVE_ANGLE_TOLERANCE_EPSILON 0.001
// no floating point arithmetic operation allowed in macro.
#pragma warning(disable : 4127)
void _recursive_bezier(ovg_path_t* ctx, float distanceTolerance, float x1, float y1, float x2, float y2, float x3,
	float y3, float x4, float y4, unsigned level) {
	if (level > CURVE_RECURSION_LIMIT) {
		return;
	}

	// Calculate all the mid-points of the line segments
	//----------------------
	float x12 = (x1 + x2) / 2;
	float y12 = (y1 + y2) / 2;
	float x23 = (x2 + x3) / 2;
	float y23 = (y2 + y3) / 2;
	float x34 = (x3 + x4) / 2;
	float y34 = (y3 + y4) / 2;
	float x123 = (x12 + x23) / 2;
	float y123 = (y12 + y23) / 2;
	float x234 = (x23 + x34) / 2;
	float y234 = (y23 + y34) / 2;
	float x1234 = (x123 + x234) / 2;
	float y1234 = (y123 + y234) / 2;

	if (level > 0) // Enforce subdivision first time
	{
		// Try to approximate the full cubic curve by a single straight line
		//------------------
		float dx = x4 - x1;
		float dy = y4 - y1;

		float d2 = fabsf(((x2 - x4) * dy - (y2 - y4) * dx));
		float d3 = fabsf(((x3 - x4) * dy - (y3 - y4) * dx));

		float da1, da2;

		if (d2 > CURVE_COLLINEARITY_EPSILON && d3 > CURVE_COLLINEARITY_EPSILON) {
			// Regular care
			//-----------------
			if ((d2 + d3) * (d2 + d3) <= (dx * dx + dy * dy) * distanceTolerance) {
				// If the curvature doesn't exceed the distance_tolerance value
				// we tend to finish subdivisions.
				//----------------------
				if (M_ANGLE_TOLERANCE < CURVE_ANGLE_TOLERANCE_EPSILON) {
					_add_point(ctx, x1234, y1234);
					return;
				}

				// Angle & Cusp Condition
				//----------------------
				float a23 = atan2f(y3 - y2, x3 - x2);
				da1 = fabsf(a23 - atan2f(y2 - y1, x2 - x1));
				da2 = fabsf(atan2f(y4 - y3, x4 - x3) - a23);
				if (da1 >= M_PI)
					da1 = M_2_PI - da1;
				if (da2 >= M_PI)
					da2 = M_2_PI - da2;

				if (da1 + da2 < (float)M_ANGLE_TOLERANCE) {
					// Finally we can stop the recursion
					//----------------------
					_add_point(ctx, x1234, y1234);
					return;
				}

				if (M_CUSP_LIMIT != 0.0) {
					if (da1 > M_CUSP_LIMIT) {
						_add_point(ctx, x2, y2);
						return;
					}

					if (da2 > M_CUSP_LIMIT) {
						_add_point(ctx, x3, y3);
						return;
					}
				}
			}
		}
		else {
			if (d2 > CURVE_COLLINEARITY_EPSILON) {
				// p1,p3,p4 are collinear, p2 is considerable
				//----------------------
				if (d2 * d2 <= distanceTolerance * (dx * dx + dy * dy)) {
					if (M_ANGLE_TOLERANCE < CURVE_ANGLE_TOLERANCE_EPSILON) {
						_add_point(ctx, x1234, y1234);
						return;
					}

					// Angle Condition
					//----------------------
					da1 = fabsf(atan2f(y3 - y2, x3 - x2) - atan2f(y2 - y1, x2 - x1));
					if (da1 >= M_PI)
						da1 = M_2_PI - da1;

					if (da1 < M_ANGLE_TOLERANCE) {
						_add_point(ctx, x2, y2);
						_add_point(ctx, x3, y3);
						return;
					}

					if (M_CUSP_LIMIT != 0.0) {
						if (da1 > M_CUSP_LIMIT) {
							_add_point(ctx, x2, y2);
							return;
						}
					}
				}
			}
			else if (d3 > CURVE_COLLINEARITY_EPSILON) {
				// p1,p2,p4 are collinear, p3 is considerable
				//----------------------
				if (d3 * d3 <= distanceTolerance * (dx * dx + dy * dy)) {
					if (M_ANGLE_TOLERANCE < CURVE_ANGLE_TOLERANCE_EPSILON) {
						_add_point(ctx, x1234, y1234);
						return;
					}

					// Angle Condition
					//----------------------
					da1 = fabsf(atan2f(y4 - y3, x4 - x3) - atan2f(y3 - y2, x3 - x2));
					if (da1 >= M_PI)
						da1 = M_2_PI - da1;

					if (da1 < M_ANGLE_TOLERANCE) {
						_add_point(ctx, x2, y2);
						_add_point(ctx, x3, y3);
						return;
					}

					if (M_CUSP_LIMIT != 0.0) {
						if (da1 > M_CUSP_LIMIT) {
							_add_point(ctx, x3, y3);
							return;
						}
					}
				}
			}
			else {
				// Collinear case
				//-----------------
				dx = x1234 - (x1 + x4) / 2;
				dy = y1234 - (y1 + y4) / 2;
				if (dx * dx + dy * dy <= distanceTolerance) {
					_add_point(ctx, x1234, y1234);
					return;
				}
			}
		}
	}

	// Continue subdivision
	//----------------------
	_recursive_bezier(ctx, distanceTolerance, x1, y1, x12, y12, x123, y123, x1234, y1234, level + 1);
	_recursive_bezier(ctx, distanceTolerance, x1234, y1234, x234, y234, x34, y34, x4, y4, level + 1);
}

static const glm::vec2 _v2_unit_x = { 1.f, 0 };
static const glm::vec2 _v2_unit_y = { 0, 1.f };
void _elliptic_arc(ovg_path_t* ctx, float x1, float y1, float x2, float y2, bool largeArc, bool counterClockWise, float _rx, float _ry, float phi) {
	if (!ctx)
		return;

	if (_rx == 0 || _ry == 0) {
		if (_current_path_is_empty(ctx))
			ovg_move_to(ctx, x1, y1);
		ovg_line_to(ctx, x2, y2);
		return;
	}
	float rx = fabsf(_rx);
	float ry = fabsf(_ry);

	glm::mat2 m = { {cosf(phi), sinf(phi)}, {-sinf(phi), cosf(phi)} };
	glm::vec2 p = { (x1 - x2) / 2, (y1 - y2) / 2 };
	glm::vec2 p1 = m * p;

	// radii corrections
	double lambda = powf(p1.x, 2) / powf(rx, 2) + powf(p1.y, 2) / powf(ry, 2);
	if (lambda > 1) {
		lambda = sqrtf(lambda);
		rx *= lambda;
		ry *= lambda;
	}

	p = glm::vec2{ rx * p1.y / ry, -ry * p1.x / rx };

	glm::vec2 cp = p * sqrtf(fabsf((powf(rx, 2) * powf(ry, 2) - powf(rx, 2) * powf(p1.y, 2) - powf(ry, 2) * powf(p1.x, 2)) /
		(powf(rx, 2) * powf(p1.y, 2) + powf(ry, 2) * powf(p1.x, 2))));

	if (largeArc == counterClockWise)
		cp = -cp;

	m = glm::mat2({ cosf(phi), -sinf(phi) }, { sinf(phi), cosf(phi) });
	p = glm::vec2((x1 + x2) / 2, (y1 + y2) / 2);
	glm::vec2 c = (m * cp) + p;

	glm::vec2   u = _v2_unit_x;
	glm::vec2   v = { (p1.x - cp.x) / rx, (p1.y - cp.y) / ry };
	double sa = acosf(glm::dot(u, v) / (fabsf(glm::length(v)) * fabsf(glm::length(u))));
	if (isnan((float)sa))
		sa = M_PI;
	if (u.x * v.y - u.y * v.x < 0)
		sa = -sa;

	u = v;
	v = glm::vec2{ (-p1.x - cp.x) / rx, (-p1.y - cp.y) / ry };
	double delta_theta = acosf(glm::dot(u, v) / (fabsf(glm::length(v)) * fabsf(glm::length(u))));
	if (isnan((float)delta_theta))
		delta_theta = M_PI;
	if (u.x * v.y - u.y * v.x < 0)
		delta_theta = -delta_theta;

	if (counterClockWise) {
		if (delta_theta < 0)
			delta_theta += M_PI * 2.0;
	}
	else if (delta_theta > 0)
		delta_theta -= M_PI * 2.0;

	m = glm::mat2{ {cosf(phi), -sinf(phi)}, {sinf(phi), cosf(phi)} };

	double theta = sa;
	double ea = sa + delta_theta;

	float step = fmaxf(0.001f, fminf(M_PI, _get_arc_step(ctx, fminf(rx, ry)) * 0.1f));

	p = glm::vec2{ rx * cosf(theta), ry * sinf(theta) };
	glm::vec2 xy = ((m * p) + c);
	if (_current_path_is_empty(ctx)) {
		_set_curve_start(ctx);
		_add_point(ctx, xy.x, xy.y);
		if (!ctx->pathPtr)
			ctx->simpleConvex = true;
		else
			ctx->simpleConvex = false;
	}
	else {
		ovg_line_to(ctx, xy.x, xy.y);
		_set_curve_start(ctx);
		ctx->simpleConvex = false;
	}

	_set_curve_start(ctx);

	if (sa < ea) {
		theta += step;
		while (theta < ea) {
			p = glm::vec2{ rx * cosf(theta), ry * sinf(theta) };
			xy = ((m * p) + c);
			_add_point(ctx, xy.x, xy.y);
			theta += step;
		}
	}
	else {
		theta -= step;
		while (theta > ea) {
			p = glm::vec2{ rx * cosf(theta), ry * sinf(theta) };
			xy = ((m * p) + c);
			_add_point(ctx, xy.x, xy.y);
			theta -= step;
		}
	}
	p = glm::vec2{ rx * cosf(ea), ry * sinf(ea) };
	xy = ((m * p) + c);
	_add_point(ctx, xy.x, xy.y);
	_set_curve_end(ctx);
}

// todo 接口实现开始

void ovg_clear_path(ovg_path_t* path) {
	if (!path)return;
	path->points.clear();
	path->pathes.clear();
	path->pathes.push_back(0);
	path->pathPtr = 0;
	path->segmentPtr = 0;
	path->subpathCount = 0;
	path->curVertOffset = 0;
	path->simpleConvex = 0;
}

void ovg_close_path(ovg_path_t* path)
{
	auto ctx = path;
	if (!ctx)
		return;
	if (ctx->pathes[ctx->pathPtr] & PATH_CLOSED_BIT) // already closed
		return;
	// check if at least 3 points are present
	if (ctx->pathes[ctx->pathPtr] < 3)
		return;
	auto pointCount = ctx->points.size();
	// prevent closing on the same point
	if (vec2_equ(ctx->points[pointCount - 1], ctx->points[pointCount - ctx->pathes[ctx->pathPtr]])) {
		if (ctx->pathes[ctx->pathPtr] < 4) // ensure enough points left for closing
			return;
		o_remove_last_point(ctx);
	}

	ctx->pathes[ctx->pathPtr] |= PATH_CLOSED_BIT;

	o_finish_path(ctx);
}
void ovg_new_sub_path(ovg_path_t* path)
{
	o_finish_path(path);
}
void ovg_path_extents(ovg_path_t* path, float* x1, float* y1, float* x2, float* y2)
{
	if (!path)return;
	o_finish_path(path);
	if (!path->pathPtr) { // no path
		*x1 = *x2 = *y1 = *y2 = 0;
		return;
	}
	_ovg_path_extents(path, false, x1, y1, x2, y2);
}
void ovg_get_current_point(ovg_path_t* path, float* x, float* y)
{
	auto cp = _get_current_point(path);
	if (x)*x = cp.x;
	if (y)*y = cp.y;
}
size_t ovg_get_segment_count(ovg_path_t* path) {
	return path ? path->pathes.size() : 0;
}
void ovg_set_segment_color(ovg_path_t* path, size_t idx, uint32_t color) {
	if (path)
	{
		if (path->colors.size() < path->pathes.size())
			path->colors.resize(path->pathes.size());
		if (idx < path->colors.size())
			path->colors[idx] = color;
	}
}
// 添加数据到当前路径，参考path_type_e
void ovg_add_path(ovg_path_t* path, float* data, size_t count)
{
	if (!path || !data || !count)return;
	auto d = data;
	for (; d - data < count;) {
		auto t = (path_type_et)*d;
		float x = d[1], y = d[2];
		d += 3;
		switch (t) {
		case path_type_et::e_vmove:
			ovg_move_to(path, x, y);
			break;
		case path_type_et::e_vline:
			ovg_line_to(path, x, y);
			break;
		case path_type_et::e_vcurve:
		{
			ovg_quadratic_to(path, x, y, d[0], d[1]);
			d += 2;
		}
		break;
		case path_type_et::e_vcubic:
		{
			ovg_curve_to(path, x, y, d[0], d[1], d[2], d[3]);
			d += 4;
		}
		break;
		}
	}
}
// todo path copy
void ovg_add_path0(ovg_path_t* path, ovg_path_t* src)
{
	if (!path || !src)return;
	if (_current_path_is_empty(src))return;

}
void ovg_move_to(ovg_path_t* path, float x, float y)
{
	if (!path)
		return;
	o_finish_path(path);
	_add_point(path, x, y);

}
void ovg_rel_move_to(ovg_path_t* path, float x, float y)
{
	if (!path)return;
	if (_current_path_is_empty(path))
		_add_point(path, 0, 0);
	auto cp = _get_current_position(path);
	o_finish_path(path);
	_add_point(path, cp.x + x, cp.y + y);
}
void ovg_line_to(ovg_path_t* path, float x, float y)
{
	if (!path)
		return;
	_line_to(path, x, y);
}
void ovg_rel_line_to(ovg_path_t* path, float dx, float dy)
{
	if (!path)
		return;
	auto cp = _get_current_position(path);
	_line_to(path, cp.x + dx, cp.y + dy);
}
void ovg_arc(ovg_path_t* path, float xc, float yc, float radius, float a1, float a2)
{
	if (!path)
		return;
	while (a2 < a1) // positive arc must have a1<a2
		a2 += 2.f * M_PI;
	if (a2 - a1 > 2.f * M_PI) // limit arc to 2PI
		a2 = a1 + 2.f * M_PI;
	glm::vec2 v = { cosf(a1) * radius + xc, sinf(a1) * radius + yc };
	float step = _get_arc_step(path, radius);
	float a = a1;
	if (_current_path_is_empty(path)) {
		_set_curve_start(path);
		_add_point(path, v.x, v.y);
		if (!path->pathPtr)
			path->simpleConvex = true;
		else
			path->simpleConvex = false;
	}
	else {
		_line_to(path, v.x, v.y);
		_set_curve_start(path);
		path->simpleConvex = false;
	}
	a += step;
	if (EQUF(a2, a1))
		return;
	while (a < a2) {
		v.x = cosf(a) * radius + xc;
		v.y = sinf(a) * radius + yc;
		_add_point(path, v.x, v.y);
		a += step;
	}
	if (EQUF(a2 - a1, M_PI * 2.f)) { // if arc is complete circle, last point is the same as the first one
		_set_curve_end(path);
		ovg_close_path(path);
		return;
	}
	a = a2;
	// vec2 lastP = v;
	v.x = cosf(a) * radius + xc;
	v.y = sinf(a) * radius + yc;
	// if (!vec2_equ (v,lastP))//this test should not be required
	_add_point(path, v.x, v.y);
	_set_curve_end(path);
}
void ovg_arc_negative(ovg_path_t* path, float xc, float yc, float radius, float a1, float a2)
{
	if (!path)
		return;
	auto ctx = path;

	while (a2 > a1)
		a2 -= 2.f * M_PI;
	if (a1 - a2 > a1 + 2.f * M_PI) // limit arc to 2PI
		a2 = a1 - 2.f * M_PI;

	glm::vec2 v = { cosf(a1) * radius + xc, sinf(a1) * radius + yc };

	float step = _get_arc_step(ctx, radius);
	float a = a1;

	if (_current_path_is_empty(ctx)) {
		_set_curve_start(ctx);
		_add_point(ctx, v.x, v.y);
		if (!ctx->pathPtr)
			ctx->simpleConvex = true;
		else
			ctx->simpleConvex = false;
	}
	else {
		_line_to(ctx, v.x, v.y);
		_set_curve_start(ctx);
		ctx->simpleConvex = false;
	}

	a -= step;

	if (EQUF(a2, a1))
		return;

	while (a > a2) {
		v.x = cosf(a) * radius + xc;
		v.y = sinf(a) * radius + yc;
		_add_point(ctx, v.x, v.y);
		a -= step;
	}

	if (EQUF(a1 - a2, M_PI * 2.f)) { // if arc is complete circle, last point is the same as the first one
		_set_curve_end(ctx);
		ovg_close_path(ctx);
		return;
	}

	a = a2;
	// vec2 lastP = v;
	v.x = cosf(a) * radius + xc;
	v.y = sinf(a) * radius + yc;
	// if (!vec2_equ (v,lastP))
	_add_point(ctx, v.x, v.y);
	_set_curve_end(ctx);
}
void ovg_curve_to(ovg_path_t* path, float x1, float y1, float x2, float y2, float x3, float y3)
{
	if (EQUF(x1, x2) && EQUF(x2, x3) && EQUF(y1, y2) && EQUF(y2, y3)) {
		auto cp = _get_current_position(path);
		if (_current_path_is_empty(path) || (EQUF(cp.x, x1) && EQUF(cp.y, y1)))
			return;
	}

	path->simpleConvex = false;
	_set_curve_start(path);
	if (_current_path_is_empty(path))
		_add_point(path, x1, y1);

	glm::vec2 cp = _get_current_position(path);
	float sx = 1, sy = 1;
	//vkvg_matrix_get_scale(&ctx->pushConsts.mat, &sx, &sy);
	float distanceTolerance = fabs(0.25f / fmaxf(sx, sy));
	_recursive_bezier(path, distanceTolerance, cp.x, cp.y, x1, y1, x2, y2, x3, y3, 0);
	_add_point(path, x3, y3);
	_set_curve_end(path);
}
void ovg_rel_curve_to(ovg_path_t* path, float x1, float y1, float x2, float y2, float x3, float y3)
{
	glm::vec2 cp = _get_current_position(path);
	ovg_curve_to(path, cp.x + x1, cp.y + y1, cp.x + x2, cp.y + y2, cp.x + x3, cp.y + y3);
}
const double quadraticFact = 2.0 / 3.0;
void ovg_quadratic_to(ovg_path_t* path, float x1, float y1, float x2, float y2)
{
	float x0, y0;
	if (_current_path_is_empty(path)) {
		x0 = x1;
		y0 = y1;
	}
	else
	{
		glm::vec2 cp = _get_current_position(path);
		x0 = cp.x; y0 = cp.y;
	}
	ovg_curve_to(path, x0 + (x1 - x0) * quadraticFact, y0 + (y1 - y0) * quadraticFact, x2 + (x1 - x2) * quadraticFact,
		y2 + (y1 - y2) * quadraticFact, x2, y2);
}
void ovg_rel_quadratic_to(ovg_path_t* path, float x1, float y1, float x2, float y2)
{
	glm::vec2 cp = _get_current_position(path);
	ovg_quadratic_to(path, cp.x + x1, cp.y + y1, cp.x + x2, cp.y + y2);
}
void ovg_rectangle(ovg_path_t* path, float x, float y, float w, float h)
{
	if (!path)
		return;
	o_finish_path(path);
	if (w <= 0 || h <= 0)
		return;
	_add_point(path, x, y);
	_add_point(path, x + w, y);
	_add_point(path, x + w, y + h);
	_add_point(path, x, y + h);
	assert(path->pathPtr < path->pathes.size());
	path->pathes[path->pathPtr] |= (PATH_CLOSED_BIT | PATH_IS_CONVEX_BIT);
	o_finish_path(path);
}
void ovg_rounded_rectangle(ovg_path_t* path, float x, float y, float w, float h, float radius)
{
	if (!path)
		return;
	if (w <= 0 || h <= 0)
		return;
	o_finish_path(path);
	if ((radius > w / 2.0f) || (radius > h / 2.0f))
		radius = fmin(w / 2.0f, h / 2.0f);
	ovg_move_to(path, x, y + radius);
	ovg_arc(path, x + radius, y + radius, radius, M_PI, -M_PI_2);
	ovg_line_to(path, x + w - radius, y);
	ovg_arc(path, x + w - radius, y + radius, radius, -M_PI_2, 0);
	ovg_line_to(path, x + w, y + h - radius);
	ovg_arc(path, x + w - radius, y + h - radius, radius, 0, M_PI_2);
	ovg_line_to(path, x + radius, y + h);
	ovg_arc(path, x + radius, y + h - radius, radius, M_PI_2, M_PI);
	ovg_line_to(path, x, y + radius);
	ovg_close_path(path);
	o_finish_path(path);
}
void ovg_rounded_rectangle2(ovg_path_t* path, float x, float y, float w, float h, float rx, float ry)
{
	if (!path)
		return;
	ovg_move_to(path, x + rx, y);
	ovg_line_to(path, x + w - rx, y);
	ovg_elliptic_arc_to(path, x + w, y + ry, false, true, rx, ry, 0);

	ovg_line_to(path, x + w, y + h - ry);
	ovg_elliptic_arc_to(path, x + w - rx, y + h, false, true, rx, ry, 0);

	ovg_line_to(path, x + rx, y + h);
	ovg_elliptic_arc_to(path, x, y + h - ry, false, true, rx, ry, 0);

	ovg_line_to(path, x, y + ry);
	ovg_elliptic_arc_to(path, x + rx, y, false, true, rx, ry, 0);

	ovg_close_path(path);
}
void ovg_ellipse(ovg_path_t* path, float radiusX, float radiusY, float x, float y, float rotationAngle)
{
	if (!path)
		return;
	float width_two_thirds = radiusX * 4 / 3;

	float dx1 = sinf(rotationAngle) * radiusY;
	float dy1 = cosf(rotationAngle) * radiusY;
	float dx2 = cosf(rotationAngle) * width_two_thirds;
	float dy2 = sinf(rotationAngle) * width_two_thirds;

	float topCenterX = x - dx1;
	float topCenterY = y + dy1;
	float topRightX = topCenterX + dx2;
	float topRightY = topCenterY + dy2;
	float topLeftX = topCenterX - dx2;
	float topLeftY = topCenterY - dy2;

	float bottomCenterX = x + dx1;
	float bottomCenterY = y - dy1;
	float bottomRightX = bottomCenterX + dx2;
	float bottomRightY = bottomCenterY + dy2;
	float bottomLeftX = bottomCenterX - dx2;
	float bottomLeftY = bottomCenterY - dy2;

	o_finish_path(path);
	_add_point(path, bottomCenterX, bottomCenterY);

	ovg_curve_to(path, bottomRightX, bottomRightY, topRightX, topRightY, topCenterX, topCenterY);
	ovg_curve_to(path, topLeftX, topLeftY, bottomLeftX, bottomLeftY, bottomCenterX, bottomCenterY);

	path->pathes[path->pathPtr] |= PATH_CLOSED_BIT;
	o_finish_path(path);
}
void ovg_elliptic_arc_to(ovg_path_t* path, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi)
{
	if (!path)
		return;
	float x1, y1;
	auto cp = _get_current_point(path);
	_elliptic_arc(path, x1, y1, x, y, large_arc_flag, sweep_flag, rx, ry, phi);
}
void ovg_rel_elliptic_arc_to(ovg_path_t* path, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi)
{
	if (!path)
		return;
	auto cp = _get_current_point(path);
	_elliptic_arc(path, cp.x, cp.y, x + cp.x, y + cp.y, large_arc_flag, sweep_flag, rx, ry, phi);
}
void ovg_circle(ovg_path_t* path, float x, float y, float radius) {
	ovg_arc(path, x, y, radius, 0, 2.0 * glm::pi<float>());
}

#ifdef CreateRgbaf
#undef CreateRgbaf
#endif
#define CreateRgbaf(r, g, b, a)                                                                                        \
    (((int)(a * 255.0f) << 24) | ((int)(b * 255.0f) << 16) | ((int)(g * 255.0f) << 8) | (int)(r * 255.0f))



struct ss_act :public  vg_state_save_t {
	usp_ac_cx* ac = 0;
};

struct pat_act :public  vg_pattern_t {
	vg_gradient_t g = {};
	usp_ac_cx* ac = 0;
};

vg_pattern_t* ovg_pattern_create_for_surface(usp_ac_cx* ac, void* surf) {
	if (!surf || !ac) {
		return 0;
	}
	pat_act* pat = (pat_act*)ac->new_obj<pat_act>();
	if (!pat) {
		return 0;
	}
	pat->ac = ac;
	pat->type = vg_pattern_type_t::VG_PATTERN_TYPE_SURFACE;
	pat->extend = vg_extend_t::VG_EXTEND_NONE;
	pat->data = surf;
	pat->references = 1;

	return pat;
}
// todo vg_state_save_t
void ovg_set_opacity(vg_state_save_t* ctx, float opacity) {
	if (ctx)ctx->pushConsts.opacity = opacity;
}
void ovg_set_source_color(vg_state_save_t* ctx, uint32_t c) {
	if (ctx)ctx->color = c;
}
void ovg_set_source_rgba(vg_state_save_t* ctx, float r, float g, float b, float a) {
	if (ctx)ctx->color = CreateRgbaf(r, g, b, a); ctx->pattern = 0;
}
void ovg_set_source_rgb(vg_state_save_t* ctx, float r, float g, float b) {
	ovg_set_source_rgba(ctx, r, g, b, 1.0f);
}
void ovg_set_line_width(vg_state_save_t* ctx, float width) {
	if (ctx)ctx->lineWidth = width;
}
void ovg_set_miter_limit(vg_state_save_t* ctx, float limit) {
	if (ctx)ctx->miterLimit = limit;
}
void ovg_set_line_cap(vg_state_save_t* ctx, int cap) {
	if (ctx)ctx->lineCap = cap;
}
void ovg_set_line_join(vg_state_save_t* ctx, int join) {
	if (ctx)ctx->lineJoin = join;
}
void ovg_set_source_surface(vg_state_save_t* ctx, vg_surface_t* surf, float x, float y) {
	auto p = (ss_act*)ctx;
	p->pushConsts.source.x = x;
	p->pushConsts.source.y = y;
	auto pat = ovg_pattern_create_for_surface(p->ac, surf);
	p->pattern = pat;
}
void ovg_set_source(vg_state_save_t* ctx, vg_pattern_t* pat) {
	if (ctx)ctx->pattern = pat;
}
void ovg_set_operator(vg_state_save_t* ctx, int op) {
	if (ctx)ctx->curOperator = op;
}
void ovg_set_fill_rule(vg_state_save_t* ctx, int fr) {
	if (ctx)ctx->curFillRule = fr;
}
void ovg_set_dash(vg_state_save_t* ctx, const float* dashes, uint32_t num_dashes, float offset) {
	if (!ctx)return;
	auto t = (ss_act*)ctx;
	if (!dashes || !num_dashes) {
		t->dashCount = 0;
	}
	if (t->dashes && t->dashCount != num_dashes)
	{
		t->ac->free_mem(t->dashes, t->dashCount);
		t->dashes = (float*)t->ac->allocate(sizeof(float) * num_dashes);
	}
	t->dashOffset = offset;
	if (t->dashes)
		memcpy(t->dashes, dashes, sizeof(float) * t->dashCount);
	else
		t->dashCount = 0;
}
void ovg_set_dash8(vg_state_save_t* ctx, uint64_t dashes0, uint32_t num_dashes, float offset) {

	float dashes[64] = {};
	uint64_t x = 1;
	auto t = dashes;
	auto v8 = (uint8_t*)&dashes0;
	if (num_dashes > 64)num_dashes = 64;
	{
		if (num_dashes > 8)num_dashes = 8;
		for (size_t i = 0; i < num_dashes; i++)
		{
			*t = v8[i]; t++;
		}
		if (num_dashes > 0)
			ovg_set_dash(ctx, dashes, num_dashes, offset);
	}
}
void ovg_translate(vg_state_save_t* ctx, float dx, float dy) {
	if (!ctx)return;
	auto m = glm::translate(glm::mat3x3(1.0), glm::vec2(dx, dy));
	glm::mat3x3 inv = ctx->pushConsts.mat;
	ctx->pushConsts.mat = inv * m; inv = ctx->pushConsts.mat;
	ctx->pushConsts.matInv = glm::inverse(inv);
}
void ovg_scale(vg_state_save_t* ctx, float sx, float sy) {
	if (!ctx)return;
	auto m = glm::scale(glm::mat3x3(1.0), glm::vec2(sx, sy));
	glm::mat3x3 inv = ctx->pushConsts.mat;
	ctx->pushConsts.mat = inv * m; inv = ctx->pushConsts.mat;
	ctx->pushConsts.matInv = glm::inverse(inv);
}
void ovg_rotate(vg_state_save_t* ctx, float radians) {
	if (!ctx)return;
	auto m = glm::rotate(glm::mat3x3(1.0), radians);
	glm::mat3x3 inv = ctx->pushConsts.mat;
	ctx->pushConsts.mat = inv * m; inv = ctx->pushConsts.mat;
	ctx->pushConsts.matInv = glm::inverse(inv);
}
void ovg_transform(vg_state_save_t* ctx, const void* matrix) {
	auto m = (glm::mat3x2*)matrix;
	if (!ctx || !m)return;
	glm::mat3x3 inv = ctx->pushConsts.mat;
	glm::mat3x3 m0 = *m;
	ctx->pushConsts.mat = inv * m0; inv = ctx->pushConsts.mat;
	ctx->pushConsts.matInv = glm::inverse(inv);
}
void ovg_set_matrix(vg_state_save_t* ctx, const void* matrix) {
	auto m = (glm::mat3x2*)matrix;
	if (!ctx || !m)return;
	ctx->pushConsts.mat = *m;
	glm::mat3x3 inv = *m;
	ctx->pushConsts.matInv = glm::inverse(inv);
}
void ovg_get_matrix(vg_state_save_t* ctx, void* matrix) {
	auto m = (glm::mat3x2*)matrix;
	if (!ctx || !m)return;
	*m = ctx->pushConsts.mat;
}
void ovg_identity_matrix(vg_state_save_t* ctx) {
	if (!ctx)return;
	ctx->pushConsts.mat = glm::mat3x2(1.0);
	glm::mat3x3 inv = ctx->pushConsts.mat;
	ctx->pushConsts.matInv = glm::inverse(inv);
}

int  ovg_pattern_add_color_stop(vg_pattern_t* pat, float o, float r, float g, float b, float a) {
	if (pat->type == vg_pattern_type_t::VG_PATTERN_TYPE_SURFACE || pat->type == vg_pattern_type_t::VG_PATTERN_TYPE_SOLID)
		return -1;
	vg_gradient_t* grad = (vg_gradient_t*)pat->data;
	if (grad->count < MAX_STOPS)
	{
		glm::vec4 c = { r, g, b, a };
		grad->colors[grad->count] = c;
#ifndef NOT_VG_ENABLE_VK_SCALAR_BLOCK_LAYOUT
		grad->stops[grad->count] = o;
#else
		grad->stops[grad->count].r = o;
#endif
		grad->count++;
	}
}
int  ovg_pattern_set_color_stop(vg_pattern_t* pat, int idx, float o, float r, float g, float b, float a) {
	if (!pat)return -1;
	if (pat->type == vg_pattern_type_t::VG_PATTERN_TYPE_SURFACE || pat->type == vg_pattern_type_t::VG_PATTERN_TYPE_SOLID)
		return -2;
	vg_gradient_t* grad = (vg_gradient_t*)pat->data;
	if (idx < 0 || idx >= MAX_STOPS)return -3;
	if (idx >= grad->count)
		grad->count = idx + 1;
	glm::vec4 c = { r, g, b, a };
	grad->colors[idx] = c;
	grad->stops[idx] = o;
	return 0;
}
void ovg_pattern_set_matrix(vg_pattern_t* pat, const void* matrix) {
	if (!pat || !matrix)return;
	pat->matrix = *((glm::mat3x2*)matrix);
	pat->hasMatrix = true;
}
void ovg_pattern_get_matrix(vg_pattern_t* pat, void* matrix) {
	if (!pat || !matrix)
		return;
	*((glm::mat3x2*)matrix) = (pat->hasMatrix) ? pat->matrix : glm::mat3x2(1.0);
}
void ovg_pattern_set_extend(vg_pattern_t* pat, int extend) {
	if (pat)pat->extend = (vg_extend_t)extend;
}
void ovg_pattern_set_filter(vg_pattern_t* pat, int filter) {
	if (pat)pat->filter = (vg_filter_t)filter;
}
void ovg_pattern_destroy(vg_pattern_t* pat) {
	if (pat) {
		auto p = (pat_act*)pat;
		if (p->ac) {
			p->ac->free_obj(p);
		}
	}
}

int _vg_pattern_edit_linear(vg_pattern_t* pat, float x0, float y0, float x1, float y1) {
	if (!pat)
		return -2;
	if (pat->type != vg_pattern_type_t::VG_PATTERN_TYPE_LINEAR)
		return -1;
	vg_gradient_t* grad = (vg_gradient_t*)pat->data;
	*grad = {};
	grad->cp[0] = glm::vec4{ {x0}, {y0}, {x1}, {y1} };
	grad->m = glm::ivec4(1024, 0, 0, 1024);
	grad->extend = pat->extend;
	grad->scale = glm::vec2{ 1.0,1.0 };
	return 0;
}
// 自定义分配
vg_pattern_t* ovg_new_pattern_linear(mem_resource_t* ac0, float x0, float y0, float x1, float y1) {
	auto ac = (usp_ac_cx*)ac0;
	if (!ac) {
		return 0;
	}
	pat_act* pat = (pat_act*)ac->new_obj<pat_act>();
	if (!pat) {
		return 0;
	}
	pat->ac = ac;
	pat->type = vg_pattern_type_t::VG_PATTERN_TYPE_LINEAR;
	pat->extend = vg_extend_t::VG_EXTEND_NONE;
	pat->data = &pat->g;
	_vg_pattern_edit_linear(pat, x0, y0, x1, y1);
	pat->references = 1;
}
int vg_pattern_edit_radial(pat_act* pat, float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse) {
	if (!(pat))
		return -2;
	if (pat->type != vg_pattern_type_t::VG_PATTERN_TYPE_RADIAL)
		return -1;
	vg_gradient_t* grad = (vg_gradient_t*)pat->data;
	*grad = {};
	glm::vec2 c0 = { cx0, cy0 };
	glm::vec2 c1 = { cx1, cy1 };
	if (radius0 > radius1 - 1.0f)
		radius0 = radius1 - 1.0f;
	glm::vec2  u = (c0 - c1);
	float l = glm::length(u);
	if (l + radius0 + 1.0f >= radius1) {
		glm::vec2 v = (u / l);
		c0 = (c1 + (v * (radius1 - radius0 - 1.0f)));
	}
	grad->cp[0] = glm::vec4{ {c0.x}, {c0.y}, {radius0}, {0} };
	grad->cp[1] = glm::vec4{ {c1.x}, {c1.y}, {radius1}, {0} };
	grad->m = glm::ivec4(1024, 0, 0, 1024);
	grad->extend = pat->extend;
	grad->scale = glm::vec2{ 1.0,1.0 };
	if (is_ellipse)grad->scale.x *= 2;
	return 0;
}
int vg_pattern_edit_sweep(pat_act* pat, float cx, float cy, float start_angle, float end_angle) {
	if (!(pat))
		return -1;
	if (pat->type != vg_pattern_type_t::VG_PATTERN_TYPE_SWEEP)
		return -2;
	vg_gradient_t* grad = (vg_gradient_t*)pat->data;
	*grad = {};
	grad->cp[0] = glm::vec4{ cx, cy, start_angle, end_angle };
	grad->m = glm::ivec4(1024, 0, 0, 1024);
	grad->extend = pat->extend;
	grad->scale = glm::vec2{ 1.0,1.0 };
	return 0;
}
vg_pattern_t* ovg_new_pattern_radial(mem_resource_t* ac0, float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse) {
	auto ac = (usp_ac_cx*)ac0;
	if (!ac) {
		return 0;
	}
	pat_act* pat = (pat_act*)ac->new_obj<pat_act>();
	if (!pat) {
		return 0;
	}
	pat->ac = ac;
	pat->type = vg_pattern_type_t::VG_PATTERN_TYPE_RADIAL;
	pat->extend = vg_extend_t::VG_EXTEND_NONE;
	pat->data = &pat->g;
	vg_pattern_edit_radial(pat, cx0, cy0, radius0, cx1, cy1, radius1, is_ellipse);
	pat->references = 1;
}
vg_pattern_t* ovg_new_pattern_sweep(mem_resource_t* ac0, float cx, float cy, float start_angle, float end_angle) {
	auto ac = (usp_ac_cx*)ac0;
	if (!ac) {
		return 0;
	}
	pat_act* pat = (pat_act*)ac->new_obj<pat_act>();
	if (!pat) {
		return 0;
	}
	pat->ac = ac;
	pat->type = vg_pattern_type_t::VG_PATTERN_TYPE_MESH;
	pat->extend = vg_extend_t::VG_EXTEND_NONE;
	pat->data = &pat->g;
	vg_pattern_edit_sweep(pat, cx, cy, start_angle, end_angle);
	pat->references = 1;
}
ovg_path_t* ovg_new_path(mem_resource_t* ac0) {
	auto ac = (usp_ac_cx*)ac0;
	ovg_path_t* p = 0;
	if (ac) {
		p = ac->new_obj<ovg_path_t>();
		ovg_clear_path(p);
		p->ac = ac;
	}
	return p;
}
void ovg_path_destroy(ovg_path_t* path) {
	if (path && path->ac)
		path->ac->free_obj(path);
}

vg_state_save_t* ovg_new_state(mem_resource_t* ac0) {
	auto ac = (usp_ac_cx*)ac0;
	vg_state_save_t* p = 0;
	if (ac) {
		auto pp = (ss_act*)ac->new_mem(sizeof(ss_act));
		pp->ac = ac;
		p = pp;
	}
	return p;
}
void ovg_state_destroy(vg_state_save_t* p) {
	auto p0 = (ss_act*)p;
	if (!p0 || !p0->ac)return;
	if (p0->dashes) {
		p0->ac->free_mem(p0->dashes, p0->dashCount);
	}
	p0->ac->free_mem(p, sizeof(ss_act));
}
// 渲染对象
#if 1


// 普通三角形命令
struct geom_cmd_t {
	int stype = 1;
	gem_info_t state = {};
	void* texture = nullptr;
	glm::mat4 mat = glm::mat4(1.0f);	// 矩阵
	float mask_time = 1.0;				// 遮罩时间
	uint32_t count = 0;
	uint32_t firstIndex = 0;
	int32_t  vertexOffset = 0;
	size_t offset = 0, ioffset = 0;
};
struct geom2d_cmd_c
{
	glm::ivec4 clip_rect = {};
	void* texid = 0;
	uint32_t vtxOffset = 0;
	uint32_t idxOffset = 0;
	uint32_t elemCount = 0;
	uint32_t vCount = 0;
	uint16_t blend_mode = 0;		// 混合模式	 
};
struct scmd {
	uint32_t vertexCount;
	uint32_t firstVertex;
};
// 矢量命令
struct vgcmd_t {
	int stype = 0;
	scmd* v = 0;
	int vc = 0;
	int full_screen_quad = 0;
	glm::ivec2 vertex = {};			// 顶点开始、数量
	glm::ivec2 index = {};			// 索引开始、数量
	vg_state_save_t* state = {};	// 渲染参数
	glm::vec4 bounds = {};			// 全屏填充,odd/clip专用
	int8_t type = 0;			// 类型：填充0、描边1、裁剪2、全屏3、清屏4
};
union gcmd_t {
	vgcmd_t vg;
	geom_cmd_t g;
};

struct dash_context_t {
	bool     dashOn;
	uint32_t curDash;       // current dash index
	float    curDashOffset; // cur dash offset between defined path point and last dash segment(on/off) start
	float    totDashLength; // total length of dashes
	glm::vec2 normal;
};

struct stroke_context_t {
	uint32_t iL;
	uint32_t iR;
	uint32_t cp; // current point

	uint32_t firstIdx; // save first point idx for closed path
	float               hw;       // stroke half width, computed once.
	float               lhMax;    // miter limit * line width
	float arcStep; // cached arcStep, prevent compute multiple times for same stroke, 0 if not yet computed
};

class mesh2d_x
{
public:
	struct vertex_t
	{
		glm::vec2 position = {};		// 坐标	
		glm::vec2 tex_coord = {};		// 纹理uv
		uint32_t color = 0xffffffff;	// 顶点颜色 
	};

	std::vector<geom2d_cmd_c> cmd_data;	// 渲染命令
	std::vector<vertex_t> vtxs;		// 顶点数据
	std::vector<int> idxs;				// 索引
	glm::ivec4 viewport = { 0,0,0,0 };
	glm::ivec4 _clip_rect = { };// 当前裁剪 
public:
	mesh2d_x();
	virtual ~mesh2d_x();
	void set_viewport(const glm::ivec4& vp);
	void set_clip(const glm::ivec4& rc);
	// 清除数据,保留viewport
	void clear_m2d();
	bool nohas_clip(glm::ivec4 a);
	// 添加相同纹理/裁剪区域则自动合批
	void add(void* user_image, std::vector<vertex_t>& vertex, std::vector<int>& vt_index, const glm::ivec4& clip);
	void add(void* user_image, vertex_t* vertex, size_t vcount, int* vt_index, size_t icount, const glm::ivec4& clip);
	// 添加图片渲染，自动生成顶点数据
	void add_image0(void* img, const glm::ivec2& texsize, const glm::ivec4& clip, const glm::ivec4& dst, const glm::ivec4& src, const glm::ivec4& sliced, uint32_t color = 0xffffffff);
	// 添加九宫格图片渲染
	void add_image_sliced(void* user_image, const glm::ivec2& texsize, const glm::ivec4& a, const glm::ivec4& sliced, const glm::ivec4& rect, uint32_t col, const glm::ivec4& clip);
	// 添加旋转图片渲染，angle为旋转角度，center为旋转中心坐标（相对于dst）
	void add_image_angle(void* img, const glm::ivec2& texsize, const glm::ivec4& src, const glm::ivec4& dst, float angle, const glm::vec2* center, uint32_t col, const glm::ivec4& clip, int flip);
private:

};

class geom_primitive :public mesh2d_x {
public:
	struct Vertex1 {
		glm::vec3 pos;
		glm::vec2 uv;
		uint32_t color;
	};
	struct Vertex2 {
		glm::vec3 pos;
		glm::vec2 uv;
		uint32_t color;
		uint32_t color1;
	};
public:
	std::pmr::vector<Vertex1> vd1;	// 单面顶点
	std::pmr::vector<Vertex2> vd2;	// 双面顶点
	std::pmr::vector<uint32_t> ids;	// 索引 
	glm::mat4 mat = glm::mat4(1.0f);// 当前矩阵
	gem_info_t curState = {};		// 当前状态	 
	std::pmr::vector<gcmd_t>* gt = 0;
	rvg_t* dc = 0;
public:
	geom_primitive();
	~geom_primitive();
public:
	// 清空数据
	void clear();
	void set_state(gem_info_t* info, const glm::mat4* matrix);
	// 添加几何数据到缓冲区，xy顶点坐标，color顶点颜色，uv顶点纹理坐标，indices索引数据，color_type=0表示float4，1表示uint32_t
	bool add_geometry(void* texture, const float* xy, int xy_stride, const void* color, int color_stride
		, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type);
	// 添加3D几何数据到缓冲区，xyz顶点坐标，color顶点颜色（双面则要双倍），uv顶点纹理坐标，indices索引数据
	bool add_geometry3d(void* texture, const float* xyz, int xyz_stride, const void* color, int color_stride
		, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type);
	void add_text(text_st_t* p, text_style_t* ts, text_box_rt* box);
	void add_image(ovg_image_r* r);
};

struct rvg_t {
	struct Vertex {
		glm::vec2	pos;
		glm::vec2	uv;
		uint32_t	color;
	};
	struct ear_clip_point {
		glm::vec2 pos;
		uint32_t idx;
		struct ear_clip_point* next;
	};
	usp_ac_cx* ac = 0;
	mbpool_t mac;
	std::pmr::vector<gcmd_t> cmdlist;		// 命令列表
	std::pmr::vector<Vertex> _vertex;		// 矢量顶点
	std::pmr::vector<uint32_t> _indices;	// 矢量索引
	// 临时缓冲用
	std::pmr::vector<ear_clip_point> ecpsd;
	std::pmr::vector<glm::vec2> _normals;
	// 23d
	geom_primitive gps = {};
#ifndef NOT_FILL_NZ_GLUTESS
	void (*vertex_cb)(uint32_t, rvg_t*) = 0; // tesselator vertex callback
	uint32_t tesselator_fan_start = 0;
	uint32_t tesselator_idx_counter = 0;
#endif
	vg_state_save_t* cur_st = 0;
	ovg_path_t* cur_path = 0;
	size_t gCount = 0;	// ubo数量
	size_t _curVertOffset = 0;
	uint32_t curColor = 0;
	glm::ivec4 curClip = {};
public:
	rvg_t();
	~rvg_t();
	void clear_all();
	void set_path(ovg_path_t* path, vg_state_save_t* st);
	void stroke_preserve();
	void fill_preserve();
	void clip_preserve();
	void fill();
	void paint();
	void clip();
	void clip0();
	void clip(const glm::ivec4* rc);
public:
	void poly_fill(ovg_path_t* ctx, glm::vec4* bounds, vgcmd_t& c);
	void glutess_fill_non_zero(ovg_path_t* p);
	void fill_non_zero(ovg_path_t* p);

	bool _build_vb_step(ovg_path_t* ctx, stroke_context_t* str, bool isCurve);
	void _draw_stoke_cap(ovg_path_t* ctx, stroke_context_t* str, glm::vec2 p0, glm::vec2 n, bool isStart);
	float _draw_dashed_segment(ovg_path_t* ctx, stroke_context_t* str, dash_context_t* dc, bool isCurve);
	void _draw_segment(ovg_path_t* ctx, stroke_context_t* str, dash_context_t* dc, bool isCurve);

	void _add_triangle_indices(ovg_path_t* ctx, uint32_t i0, uint32_t i1, uint32_t i2);
	void _add_tri_indices_for_rect(uint32_t i);
	void _add_vertexf(ovg_path_t* ctx, float x, float y);
	// 复制状态，自动释放
	void cp_cmdt(vgcmd_t* c, vg_state_save_t* t);
};

rvg_t::rvg_t()
{
	gps.gt = &cmdlist;
	gps.dc = this;
}

rvg_t::~rvg_t()
{}
void rvg_t::clear_all()
{
	cur_path = 0;
	_curVertOffset = 0;
	gCount = 0;
	mac.release();
	_vertex.clear();
	_indices.clear();
	cmdlist.clear();
}
void rvg_t::set_path(ovg_path_t* path, vg_state_save_t* st)
{
	cur_path = path;
	cur_st = st;
}
void rvg_t::stroke_preserve()
{
	o_finish_path(cur_path);
	if (!cur_path || !cur_path->pathPtr || !cur_st)
		return;
	auto p = cur_path;
	p->t = cur_st;
	if (p->t->pattern)
		gCount++;
	auto ctx = p;
	vgcmd_t c = {};
	c.vertex.x = _vertex.size();
	c.index.x = _indices.size();
	c.type = 1;
	cp_cmdt(&c, cur_st);
	ctx->curVertOffset = c.vertex.x;
	stroke_context_t str = { 0 };
	str.hw = p->t->lineWidth * 0.5f;
	str.lhMax = p->t->miterLimit * p->t->lineWidth;
	uint32_t ptrPath = 0;
	curColor = p->t->color;
	while (ptrPath < ctx->pathPtr) {
		uint32_t ptrSegment = 0, lastSegmentPointIdx = 0;
		uint32_t firstPathPointIdx = str.cp;
		uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;
		uint32_t lastPathPointIdx = str.cp + pathPointCount - 1;

		dash_context_t dc = { 0 };

		if (o_path_has_curves(ctx->pathes.data(), ptrPath)) {
			ptrSegment = 1;
			lastSegmentPointIdx = str.cp + (ctx->pathes[ptrPath + ptrSegment] & PATH_ELT_MASK) - 1;
		}

		str.firstIdx = (uint32_t)_vertex.size() - ctx->curVertOffset;

		// LOG(VKVG_LOG_INFO_PATH, "\tPATH: points count=%10d end point idx=%10d", ctx->pathes[ptrPath]&PATH_ELT_MASK,
		// lastPathPointIdx);

		if (p->t->dashCount > 0) {
			// init dash stroke
			dc.dashOn = true;
			dc.curDash = 0; // current dash index
			dc.totDashLength = 0; // limit offset to total length of dashes
			for (uint32_t i = 0; i < p->t->dashCount; i++)
				dc.totDashLength += p->t->dashes[i];
			if (dc.totDashLength == 0) {
				//ctx->status = VKVG_STATUS_INVALID_DASH;
				break;
				//return;
			}
			dc.curDashOffset = fmodf(
				p->t->dashOffset,
				dc.totDashLength); // cur dash offset between defined path point and last dash segment(on/off) start
			str.iL = lastPathPointIdx;
		}
		else if (_path_is_closed(ctx, ptrPath)) {
			str.iL = lastPathPointIdx;
		}
		else {
			_draw_stoke_cap(ctx, &str, ctx->points[str.cp],
				vec2_line_norm(ctx->points[str.cp], ctx->points[str.cp + 1]), true);
			str.iL = str.cp++;
		}

		if (o_path_has_curves(ctx->pathes.data(), ptrPath)) {
			while (str.cp < lastPathPointIdx) {

				bool curved = ctx->pathes[ptrPath + ptrSegment] & PATH_HAS_CURVES_BIT;
				if (lastSegmentPointIdx == lastPathPointIdx) // last segment of path, dont draw end point here
					lastSegmentPointIdx--;
				while (str.cp <= lastSegmentPointIdx)
					_draw_segment(ctx, &str, &dc, curved);

				ptrSegment++;
				uint32_t cptSegPts = ctx->pathes[ptrPath + ptrSegment] & PATH_ELT_MASK;
				lastSegmentPointIdx = str.cp + cptSegPts - 1;
				if (lastSegmentPointIdx == lastPathPointIdx && cptSegPts == 1) {
					// single point last segment
					ptrSegment++;
					break;
				}
			}
		}
		else
			while (str.cp < lastPathPointIdx)
				_draw_segment(ctx, &str, &dc, false);

		if (p->t->dashCount > 0) {
			if (_path_is_closed(ctx, ptrPath)) {
				str.iR = firstPathPointIdx;

				_draw_dashed_segment(ctx, &str, &dc, false);

				str.iL++;
				str.cp++;
			}
			if (!dc.dashOn) {
				// finishing last dash that is already started, draw end caps but not too close to start
				// the default gap is the next void
				int32_t prevDash = (int32_t)dc.curDash - 1;
				if (prevDash < 0)
					dc.curDash = p->t->dashCount - 1;
				float m = fminf(p->t->dashes[prevDash] - dc.curDashOffset, p->t->dashes[dc.curDash]);
				glm::vec2  p2 = vec2_sub(ctx->points[str.iR], vec2_mult_s(dc.normal, m));
				_draw_stoke_cap(ctx, &str, p2, dc.normal, false);
			}
		}
		else if (_path_is_closed(ctx, ptrPath)) {
			str.iR = firstPathPointIdx;
			bool inverse = _build_vb_step(ctx, &str, false);
			uint32_t* inds = &_indices[_indices.size() - 6];
			uint32_t  ii = str.firstIdx;
			if (inverse) {
				inds[1] = ii + 1;
				inds[4] = ii + 1;
				inds[5] = ii;
			}
			else {
				inds[1] = ii;
				inds[4] = ii;
				inds[5] = ii + 1;
			}
			str.cp++;
		}
		else
			_draw_stoke_cap(ctx, &str, ctx->points[str.cp],
				vec2_line_norm(ctx->points[str.cp - 1], ctx->points[str.cp]), false);

		str.cp = firstPathPointIdx + pathPointCount;

		if (ptrSegment > 0)
			ptrPath += ptrSegment;
		else
			ptrPath++;

	}
	c.vertex.y = _vertex.size() - c.vertex.x;
	c.index.y = _indices.size() - c.index.x;
	cmdlist.push_back({ .vg = c });

}

void rvg_t::fill_preserve()
{
	o_finish_path(cur_path);
	if (!cur_path || !cur_path->pathPtr || !cur_st)
		return;
	auto p = cur_path;
	p->t = cur_st;
	if (p->t->pattern)
		gCount++;
	vgcmd_t c = {};
	if (p->t->curFillRule == VG_FILL_RULE_EVEN_ODD) {

		glm::vec4 bounds = { FLT_MAX, FLT_MAX, FLT_MIN, FLT_MIN };
		c.type = 0;
		poly_fill(p, &bounds, c);
		c.full_screen_quad = _vertex.size();
		Vertex v = {};
		v.pos = { -1,-1 };
		v.color = p->t->color;

		_vertex.push_back(v);
		v.pos = { 3,-1 };
		_vertex.push_back(v);
		v.pos = { -1,3 };
		_vertex.push_back(v);
	}
	else
	{
		c.vertex.x = _vertex.size();
		c.index.x = _indices.size();
		c.type = 0;
		cp_cmdt(&c, cur_st);
		p->curVertOffset = c.vertex.x;
		fill_non_zero(p);
		c.vertex.y = _vertex.size() - c.vertex.x;
		c.index.y = _indices.size() - c.index.x;

	}
	cmdlist.push_back({ .vg = c });
}

void rvg_t::clip_preserve()
{
	o_finish_path(cur_path);
	if (!cur_path || !cur_path->pathPtr || !cur_st)
		return;
	cur_path->t = cur_st;
	auto p = cur_path;
	auto t = cur_st;
	vgcmd_t c = {};
	c.type = 2;
	if (t->curFillRule == VG_FILL_RULE_EVEN_ODD) {
		poly_fill(p, NULL, c);
	}
	else {
		c.vertex.x = _vertex.size();
		c.index.x = _indices.size();
		cp_cmdt(&c, t);
		p->curVertOffset = c.vertex.x;
		fill_non_zero(p);
		c.vertex.y = _vertex.size() - c.vertex.x;
		c.index.y = _indices.size() - c.index.x;

	}
	c.full_screen_quad = _vertex.size();
	//gt->push(&c);
	cmdlist.push_back({ .vg = c });
	Vertex v = {};
	v.pos = { -1,-1 };
	v.color = t->color;
	_vertex.push_back(v);
	v.pos = { 3,-1 };
	_vertex.push_back(v);
	v.pos = { -1,3 };
	_vertex.push_back(v);
}
void rvg_t::clip0()
{
	vgcmd_t c = {};
	c.type = 2;
	cmdlist.push_back({ .vg = c });
}

void rvg_t::clip()
{
	clip_preserve();
	ovg_clear_path(cur_path);
}
void rvg_t::clip(const glm::ivec4* rc)
{
	if (rc)
	{
		curClip = *rc;
		vgcmd_t c = {};
		c.type = 2;
		c.bounds = *rc;// vec4{ (float)rc->x, (float)rc->y, (float)rc->z, (float)rc->w };
		cmdlist.push_back({ .vg = c });
	}
}
void rvg_t::fill()
{
	fill_preserve();
	ovg_clear_path(cur_path);
}

void rvg_t::paint()
{
	auto ph = cur_path;
	o_finish_path(ph);
	if (!cur_path || !cur_path->pathPtr || !cur_st)return;
	if (ph->pathPtr) {
		fill();
		return;
	}
	vgcmd_t c = {};
	c.type = 3;
	c.full_screen_quad = _vertex.size();
	Vertex v = {};
	v.pos = { -1,-1 };
	v.color = cur_st->color;
	_vertex.push_back(v);
	v.pos = { 3,-1 };
	_vertex.push_back(v);
	v.pos = { -1,3 };
	_vertex.push_back(v);
	cmdlist.push_back({ .vg = c });
}


void rvg_t::poly_fill(ovg_path_t* ctx, glm::vec4* bounds, vgcmd_t& c)
{
	Vertex v = {}; v.color = ctx->color; v.uv = { };

	uint32_t ptrPath = 0;
	uint32_t firstPtIdx = 0;
	size_t nc = 0;
	while (ptrPath < ctx->pathPtr) {
		uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;
		if (pathPointCount > 2) {
			nc++;
		}
		if (o_path_has_curves(ctx->pathes.data(), ptrPath)) {
			ptrPath++;
			uint32_t totPts = 0;
			while (totPts < pathPointCount)
				totPts += (ctx->pathes[ptrPath++] & PATH_ELT_MASK);
		}
		else
			ptrPath++;
	}
	if (!nc)return;
	ptrPath = 0;

	c.v = (scmd*)mac.allocate(sizeof(scmd) * nc);
	if (!c.v)return;
	cp_cmdt(&c, ctx->t);
	c.vc = nc;
	ctx->curVertOffset = _vertex.size();
	auto cv = c.v;
	while (ptrPath < ctx->pathPtr) {
		uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;
		if (pathPointCount > 2) {
			uint32_t firstVertIdx = (uint32_t)_vertex.size();
			c.vertex.x = _vertex.size();
			for (uint32_t i = 0; i < pathPointCount; i++) {
				v.pos = ctx->points[i + firstPtIdx];
				_vertex.push_back(v);
				if (!bounds)
					continue;
				matrix_transform_point(&c.state->pushConsts.mat, &v.pos.x, &v.pos.y);
				if (v.pos.x < bounds->x)
					bounds->x = v.pos.x;
				if (v.pos.x > bounds->z)
					bounds->z = v.pos.x;
				if (v.pos.y < bounds->y)
					bounds->y = v.pos.y;
				if (v.pos.y > bounds->w)
					bounds->w = v.pos.y;
			}
			cv->firstVertex = firstVertIdx;
			cv->vertexCount = pathPointCount;
			cv++;
		}
		firstPtIdx += pathPointCount;

		if (o_path_has_curves(ctx->pathes.data(), ptrPath)) {
			ptrPath++;
			uint32_t totPts = 0;
			while (totPts < pathPointCount)
				totPts += (ctx->pathes[ptrPath++] & PATH_ELT_MASK);
		}
		else
			ptrPath++;
	}
	if (bounds)
		c.bounds = *bounds;
}

#ifndef NOT_FILL_NZ_GLUTESS

#include <glutess.h>
namespace glutess_p {
	void a_set_vertex(rvg_t* ctx, uint32_t idx, rvg_t::Vertex v) { ctx->_vertex[idx] = v; }

	void _add_indicea(rvg_t* ctx, uint32_t i) {
		ctx->_indices.push_back(i);
	}
	void _add_indice_for_fana(rvg_t* ctx, uint32_t i) {
		uint32_t inds[3] = { ctx->tesselator_fan_start, ctx->_indices.back(),i };
		ctx->_indices.insert(ctx->_indices.end(), inds, inds + 3);
	}
	void _add_indice_for_stripa(rvg_t* ctx, uint32_t i, bool odd) {
		uint32_t inds[3] = {};
		auto indCount = ctx->_indices.size();
		assert(indCount > 2);
		if (odd) {
			inds[0] = ctx->_indices[indCount - 2];
			inds[1] = i;
			inds[2] = ctx->_indices[indCount - 1];
		}
		else {
			inds[0] = ctx->_indices[indCount - 1];
			inds[1] = ctx->_indices[indCount - 2];
			inds[2] = i;
		}
		ctx->_indices.insert(ctx->_indices.end(), inds, inds + 3);
	}
	void fan_vertex2a(uint32_t v, rvg_t* ctx) {
		uint32_t i = (uint32_t)v;
		switch (ctx->tesselator_idx_counter) {
		case 0:
			_add_indicea(ctx, i);
			ctx->tesselator_fan_start = i;
			ctx->tesselator_idx_counter++;
			break;
		case 1:
		case 2:
			_add_indicea(ctx, i);
			ctx->tesselator_idx_counter++;
			break;
		default:
			_add_indice_for_fana(ctx, i);
			break;
		}
	}
	void strip_vertex2a(uint32_t v, rvg_t* ctx) {
		uint32_t i = (uint32_t)v;
		if (ctx->tesselator_idx_counter < 3) {
			_add_indicea(ctx, i);
		}
		else
			_add_indice_for_stripa(ctx, i, ctx->tesselator_idx_counter % 2);
		ctx->tesselator_idx_counter++;
	}
	void triangle_vertex2a(uint32_t v, rvg_t* ctx) {
		uint32_t i = (uint32_t)v;
		_add_indicea(ctx, i);
	}
	void skip_vertex2a(uint32_t v, rvg_t* ctx) {}
	void begin2a(GLenum which, void* poly_data) {
		rvg_t* ctx = (rvg_t*)poly_data;
		switch (which) {
		case GL_TRIANGLES:
			ctx->vertex_cb = &triangle_vertex2a;
			break;
		case GL_TRIANGLE_STRIP:
			ctx->tesselator_idx_counter = 0;
			ctx->vertex_cb = &strip_vertex2a;
			break;
		case GL_TRIANGLE_FAN:
			ctx->tesselator_idx_counter = ctx->tesselator_fan_start = 0;
			ctx->vertex_cb = &fan_vertex2a;
			break;
		default:
			fprintf(stderr, "ERROR, can't handle %d\n", (int)which);
			ctx->vertex_cb = &skip_vertex2a;
		}
	}

	void combine2a(const GLdouble newVertex[3], const void* neighborVertex_s[4], const GLfloat neighborWeight[4],
		void** outData, void* poly_data) {
		rvg_t* ctx = (rvg_t*)poly_data;
		rvg_t::Vertex      v = { {newVertex[0], newVertex[1]}, {}, ctx->curColor };
		*outData = (void*)(ctx->_vertex.size() - ctx->_curVertOffset);
		ctx->_vertex.push_back(v);
	}
	void vertex2a(void* vertex_data, void* poly_data) {
		uint32_t i = (uint32_t)vertex_data;
		rvg_t* ctx = (rvg_t*)poly_data;
		ctx->vertex_cb(i, ctx);
	}
	void g_fill_non_zero(rvg_t* r, ovg_path_t* ctx)
	{
		rvg_t::Vertex v = { {0,0}, {},ctx->color };
		r->curColor = ctx->color;
		uint32_t ptrPath = 0;
		uint32_t firstPtIdx = 0;
		r->_curVertOffset = ctx->curVertOffset;
		if (ctx->pathPtr == 1 && ctx->pathes[0] & PATH_IS_CONVEX_BIT) {
			uint32_t firstVertIdx = (uint32_t)(r->_vertex.size() - ctx->curVertOffset);
			uint32_t            pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;
			uint32_t i = 0;
			while (i < 2) {
				v.pos = ctx->points[i++];
				r->_vertex.push_back(v);
			}
			while (i < pathPointCount) {
				v.pos = ctx->points[i];
				r->_vertex.push_back(v);
				uint32_t ind[3] = { firstVertIdx, firstVertIdx + i - 1, firstVertIdx + i };
				r->_indices.insert(r->_indices.end(), ind + 0, ind + 3);
				i++;
			}
			return;
		}
		GLUtesselator* tess = gluNewTess();
		gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
		gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (GLvoid(*)()) & vertex2a);
		gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (GLvoid(*)()) & begin2a);
		gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (GLvoid(*)()) & combine2a);
		gluTessBeginPolygon(tess, r);
		while (ptrPath < ctx->pathPtr) {
			uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;

			if (pathPointCount > 2) {
				uint32_t firstVertIdx = (uint32_t)(r->_vertex.size() - ctx->curVertOffset);
				gluTessBeginContour(tess);
				uint32_t i = 0;

				while (i < pathPointCount) {
					v.pos = ctx->points[i + firstPtIdx];
					double dp[] = { v.pos.x, v.pos.y, 0 };
					r->_vertex.push_back(v);
					gluTessVertex(tess, dp, (void*)((unsigned long)firstVertIdx + i));
					i++;
				}
				gluTessEndContour(tess);
			}
			firstPtIdx += pathPointCount;
			if (o_path_has_curves(ctx->pathes.data(), ptrPath)) {
				ptrPath++;
				uint32_t totPts = 0;
				while (totPts < pathPointCount)
					totPts += (ctx->pathes[ptrPath++] & PATH_ELT_MASK);
			}
			else
				ptrPath++;
		}
		gluTessEndPolygon(tess);
		gluDeleteTess(tess);
	}
}
#endif
void rvg_t::glutess_fill_non_zero(ovg_path_t* p)
{
#ifndef NOT_FILL_NZ_GLUTESS
	glutess_p::g_fill_non_zero(this, p);
#endif
}

inline float ecp_zcross(rvg_t::ear_clip_point* p0, rvg_t::ear_clip_point* p1, rvg_t::ear_clip_point* p2) {
	return vec2_zcross(vec2_sub(p1->pos, p0->pos), vec2_sub(p2->pos, p0->pos));
}

bool ptInTriangle(const glm::vec2& p, const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2) {
	float dX = p.x - p2.x;
	float dY = p.y - p2.y;
	float dX21 = p2.x - p1.x;
	float dY12 = p1.y - p2.y;
	float D = dY12 * (p0.x - p2.x) + dX21 * (p0.y - p2.y);
	float s = dY12 * dX + dX21 * dY;
	float t = (p2.y - p0.y) * dX + (p0.x - p2.x) * dY;
	if (D < 0)
		return (s <= 0) && (t <= 0) && (s + t >= D);
	return (s >= 0) && (t >= 0) && (s + t <= D);
}
void rvg_t::fill_non_zero(ovg_path_t* p)
{

	auto t = p->t;
	uint32_t color = t->color;
	p->color = color;
	if (t->glutessEnable)
	{
#ifndef NOT_FILL_NZ_GLUTESS
		glutess_fill_non_zero(p);
		return;
#endif
	}
	uint32_t ptrPath = 0;
	uint32_t firstPtIdx = 0;
	const glm::vec3 uv = { 0,0,-1 };
	bool aa = false;// t->aa; 
	Vertex v = {}; v.color = color; v.uv = { 0, 0 };
	uint32_t cur_idx = _vertex.size() - p->curVertOffset;
	auto pcolor = p->colors.data();
	auto pcn = p->colors.size();
	if (p->colors.empty())pcolor = 0;
	while (ptrPath < p->pathPtr) {
		uint32_t pathPointCount = p->pathes[ptrPath] & PATH_ELT_MASK;
		auto col = pcolor && ptrPath < pcn ? pcolor[ptrPath] : color;
		v.color = col;
		if (pathPointCount > 2) {
			uint32_t firstVertIdx = (uint32_t)cur_idx;
			ecpsd.resize(pathPointCount);
			auto ecps = ecpsd.data();
			if (!ecps)break;
			uint32_t            ecps_count = pathPointCount;
			uint32_t i = 0;
			auto points = p->points.data() + firstPtIdx;
			while (i < pathPointCount - 1) {
				v.pos = points[i];
				ear_clip_point ecp = { v.pos, firstVertIdx + i, &ecps[i + 1] };
				ecps[i] = ecp;
				if (!aa)
					_vertex.push_back(v);
				i++;
			}
			v.pos = points[i];
			ear_clip_point ecp = { v.pos, firstVertIdx + i, ecps };
			ecps[i] = ecp;
			if (!aa)
				_vertex.push_back(v);

			ear_clip_point* ecp_current = ecps;
			uint32_t        tries = 0;

			while (ecps_count > 3) {
				if (tries > ecps_count) {
					break;
				}
				ear_clip_point* v0 = ecp_current->next, * v1 = ecp_current, * v2 = ecp_current->next->next;
				if (ecp_zcross(v0, v2, v1) < 0) {
					ecp_current = ecp_current->next;
					tries++;
					continue;
				}
				ear_clip_point* vP = v2->next;
				bool            isEar = true;
				while (vP != v1) {
					if (ptInTriangle(vP->pos, v0->pos, v2->pos, v1->pos)) {
						isEar = false;
						break;
					}
					vP = vP->next;
				}
				if (isEar) {
					uint32_t t3[3] = { v0->idx, v1->idx, v2->idx };
					if (aa) {
						t3[0] = v0->idx << 1;
						t3[1] = v1->idx << 1;
						t3[1] = v2->idx << 1;
					}
					_indices.insert(_indices.end(), t3, t3 + 3);
					v1->next = v2;
					ecps_count--;
					tries = 0;
				}
				else {
					ecp_current = ecp_current->next;
					tries++;
				}
			}
			if (ecps_count == 3)
			{
				uint32_t t3[3] = { ecp_current->next->idx, ecp_current->idx, ecp_current->next->next->idx };
				if (aa) {
					t3[0] = t3[0] << 1;
					t3[1] = t3[1] << 1;
					t3[1] = t3[1] << 1;
				}
				_indices.insert(_indices.end(), t3, t3 + 3);
			}
			// todo 抗锯齿填充有bug。Anti-aliased Fill
			if (aa)
			{
				auto points_count = pathPointCount;
				const float AA_SIZE = 1.0;
				const uint32_t col_trans = col & ~VG_COL32_A_MASK;
				const int idx_count = (points_count - 2) * 3 + points_count * 6;
				const int vtx_count = (points_count * 2);
				//PrimReserve(idx_count, vtx_count);
				auto ips = _indices.size();
				_indices.resize(idx_count);
				auto idxw = _indices.data() + ips;
				// Add indexes for fill
				unsigned int vtx_inner_idx = firstVertIdx;
				unsigned int vtx_outer_idx = firstVertIdx + 1;

				// Compute normals
				_normals.resize(points_count);
				auto temp_normals = _normals.data();
				for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
				{
					const glm::vec2& p0 = points[i0];
					const glm::vec2& p1 = points[i1];
					float dx = p1.x - p0.x;
					float dy = p1.y - p0.y;
					normalize2f_over_zero(dx, dy);
					temp_normals[i0].x = dy;
					temp_normals[i0].y = -dx;
				}

				for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
				{
					// Average normals
					const glm::vec2& n0 = temp_normals[i0];
					const glm::vec2& n1 = temp_normals[i1];
					float dm_x = (n0.x + n1.x) * 0.5f;
					float dm_y = (n0.y + n1.y) * 0.5f;
					fixnormal2f(dm_x, dm_y);
					dm_x *= AA_SIZE * 0.5f;
					dm_y *= AA_SIZE * 0.5f;
					// Add vertices
					v.pos = { (points[i1].x - dm_x),(points[i1].y - dm_y) };
					v.color = col;      // Inner
					_vertex.push_back(v);
					v.pos = { (points[i1].x + dm_x),(points[i1].y + dm_y) };
					v.color = col_trans;  // Outer					 
					_vertex.push_back(v);

					// Add indexes for fringes
					idxw[0] = (vtx_inner_idx + (i1 << 1));
					idxw[1] = (vtx_inner_idx + (i0 << 1));
					idxw[2] = (vtx_outer_idx + (i0 << 1));
					idxw[3] = (vtx_outer_idx + (i0 << 1));
					idxw[4] = (vtx_outer_idx + (i1 << 1));
					idxw[5] = (vtx_inner_idx + (i1 << 1));
					idxw += 6;
				}
				cur_idx += vtx_count;
			}
			else {
				cur_idx += pathPointCount;
			}
		}

		firstPtIdx += pathPointCount;
		if (o_path_has_curves(p->pathes.data(), ptrPath)) {
			// skip segments lengths used in stroke
			ptrPath++;
			uint32_t totPts = 0;
			while (totPts < pathPointCount)
				totPts += (p->pathes[ptrPath++] & PATH_ELT_MASK);
		}
		else
			ptrPath++;
	}
}



void rvg_t::_add_triangle_indices(ovg_path_t* ctx, uint32_t i0, uint32_t i1, uint32_t i2) {
	_indices.push_back(i0);
	_indices.push_back(i1);
	_indices.push_back(i2);
}
void rvg_t::_add_tri_indices_for_rect(uint32_t i) {
	_indices.resize(_indices.size() + 6);
	uint32_t* inds = _indices.data() + _indices.size() - 6;
	inds[0] = i;
	inds[1] = i + 2;
	inds[2] = i + 1;
	inds[3] = i + 1;
	inds[4] = i + 2;
	inds[5] = i + 3;
}
void rvg_t::_add_vertexf(ovg_path_t* ctx, float x, float y) {
	Vertex v = {};
	v.pos = { x,y };
	v.color = ctx->color;
	_vertex.push_back(v);
}
void rvg_t::cp_cmdt(vgcmd_t* c, vg_state_save_t* t)
{
	c->state = (vg_state_save_t*)mac.allocate(sizeof(vg_state_save_t) * 1);
	if (!c->state)return;
	*c->state = *t;
	if (t->dashes && t->dashCount > 0) {
		c->state->dashes = (float*)mac.allocate(sizeof(float) * t->dashCount);
		if (c->state->dashes)
			memcpy(c->state->dashes, t->dashes, sizeof(float) * t->dashCount);
		else
			c->state->dashCount = 0;
	}

}
bool rvg_t::_build_vb_step(ovg_path_t* ctx, stroke_context_t* str, bool isCurve) {
	Vertex v = {};
	v.color = ctx->color; v.uv = { };
	glm::vec2   p0 = ctx->points[str->cp];
	glm::vec2   v0 = p0 - ctx->points[str->iL];
	glm::vec2   v1 = ctx->points[str->iR] - p0;
	float  length_v0 = glm::length(v0);
	float  length_v1 = glm::length(v1);
	if (length_v0 < FLT_EPSILON || length_v1 < FLT_EPSILON) {
		return false;
	}
	glm::vec2  v0n = (v0 / length_v0);
	glm::vec2  v1n = (v1 / length_v1);
	float dot = glm::dot(v0n, v1n);
	float det = v0n.x * v1n.y - v0n.y * v1n.x;
	if (EQUF(dot, 1.0f)) { // colinear

		return false;
	}
	if (EQUF(dot, -1.0f)) { // cusp (could draw line butt?)
		glm::vec2 vPerp = (vec2_perp(v0n) * str->hw);
		uint32_t idx = (uint32_t)(_vertex.size() - ctx->curVertOffset);
		v.pos = (p0 + vPerp);
		_vertex.push_back(v);
		v.pos = (p0 - vPerp);
		_vertex.push_back(v);
		_add_triangle_indices(ctx, idx, idx + 1, idx + 2);
		_add_triangle_indices(ctx, idx, idx + 2, idx + 3);
		return true;
	}
	glm::vec2  bisec_n = glm::normalize(v0n + v1n); // bisec/bisec_perp are inverted names
	float alpha = acosf(dot);

	if (det < 0)
		alpha = -alpha;
	float halfAlpha = alpha / 2.f;
	float cosHalfAlpha = cosf(halfAlpha);
	float lh = str->hw / cosHalfAlpha;
	glm::vec2  bisec_n_perp = vec2_perp(bisec_n);
	// limit bisectrice length
	float rlh = lh; // rlh is for inside pos tweeks
	if (dot < 0.f)
		rlh = fminf(rlh, fminf(length_v0, length_v1));
	//---

	glm::vec2 bisec = (bisec_n_perp * rlh);

	uint32_t idx = (uint32_t)(_vertex.size() - ctx->curVertOffset);

	glm::vec2 rlh_inside_pos, rlh_outside_pos;
	if (rlh < lh) {
		glm::vec2 vnPerp;
		if (length_v0 < length_v1)
			vnPerp = vec2_perp(v1n);
		else
			vnPerp = vec2_perp(v0n);
		glm::vec2 vHwPerp = (vnPerp * str->hw);

		double lbc = cosHalfAlpha * rlh;
		if (det < 0.f) {
			rlh_inside_pos = ((vnPerp * glm::vec2(-lbc) + (p0 + bisec)) + vHwPerp);
			rlh_outside_pos = (p0 - (bisec_n_perp * lh));
		}
		else {
			rlh_inside_pos = vec2_sub(vec2_add(vec2_mult_s(vnPerp, lbc), vec2_sub(p0, bisec)), vHwPerp);
			rlh_outside_pos = vec2_add(p0, vec2_mult_s(bisec_n_perp, lh));
		}
	}
	else {
		if (det < 0.0) {
			rlh_inside_pos = vec2_add(p0, bisec);
			rlh_outside_pos = vec2_sub(p0, bisec);
		}
		else {
			rlh_inside_pos = vec2_sub(p0, bisec);
			rlh_outside_pos = vec2_add(p0, bisec);
		}
	}

	auto join = (vg_line_join_t)ctx->t->lineJoin;

	if (isCurve) {
		if (dot < 0.8f)
			join = VG_LINE_JOIN_ROUND;
		else
			join = VG_LINE_JOIN_MITER;
	}

	if (join == VG_LINE_JOIN_MITER) {
		if (lh > str->lhMax) { // miter limit
			double x = (lh - str->lhMax) * cosHalfAlpha;
			glm::vec2   bisecPerp = vec2_mult_s(bisec_n, x);
			bisec = vec2_mult_s(bisec_n_perp, str->lhMax);
			if (det < 0) {
				v.pos = rlh_inside_pos;
				_vertex.push_back(v);

				glm::vec2 p = vec2_sub(p0, bisec);

				v.pos = vec2_sub(p, bisecPerp);
				_vertex.push_back(v);
				v.pos = vec2_add(p, bisecPerp);
				_vertex.push_back(v);

				_add_triangle_indices(ctx, idx, idx + 2, idx + 1);
				_add_triangle_indices(ctx, idx + 2, idx + 4, idx);
				_add_triangle_indices(ctx, idx, idx + 3, idx + 4);
				return true;
			}
			else {
				glm::vec2 p = vec2_add(p0, bisec);
				v.pos = vec2_sub(p, bisecPerp);
				_vertex.push_back(v);

				v.pos = rlh_inside_pos;
				_vertex.push_back(v);

				v.pos = vec2_add(p, bisecPerp);
				_vertex.push_back(v);

				_add_triangle_indices(ctx, idx, idx + 2, idx + 1);
				_add_triangle_indices(ctx, idx + 2, idx + 3, idx + 1);
				_add_triangle_indices(ctx, idx + 1, idx + 3, idx + 4);
				return false;
			}

		}
		else { // normal miter
			if (det < 0) {
				v.pos = rlh_inside_pos;
				_vertex.push_back(v);
				v.pos = rlh_outside_pos;
				_vertex.push_back(v);
			}
			else {
				v.pos = rlh_outside_pos;
				_vertex.push_back(v);
				v.pos = rlh_inside_pos;
				_vertex.push_back(v);
			}

			_add_tri_indices_for_rect(idx);
			return false;
		}
	}
	else {
		glm::vec2 vp = vec2_perp(v0n);

		if (det < 0) {
			if (dot < 0 && rlh < lh)
				v.pos = rlh_inside_pos;
			else
				v.pos = vec2_add(p0, bisec);
			_vertex.push_back(v);
			v.pos = vec2_sub(p0, vec2_mult_s(vp, str->hw));
		}
		else {
			v.pos = vec2_add(p0, vec2_mult_s(vp, str->hw));
			_vertex.push_back(v);
			if (dot < 0 && rlh < lh)
				v.pos = rlh_inside_pos;
			else
				v.pos = vec2_sub(p0, bisec);
		}
		_vertex.push_back(v);

		if (join == VG_LINE_JOIN_BEVEL) {
			if (det < 0) {
				_add_triangle_indices(ctx, idx, idx + 2, idx + 1);
				_add_triangle_indices(ctx, idx + 2, idx + 4, idx + 0);
				_add_triangle_indices(ctx, idx, idx + 3, idx + 4);
			}
			else {
				_add_triangle_indices(ctx, idx, idx + 2, idx + 1);
				_add_triangle_indices(ctx, idx + 2, idx + 3, idx + 1);
				_add_triangle_indices(ctx, idx + 1, idx + 3, idx + 4);
			}
		}
		else if (join == VG_LINE_JOIN_ROUND) {
			if (!str->arcStep)
				str->arcStep = _get_arc_step(ctx, str->hw);
			float a = acosf(vp.x);
			if (vp.y < 0)
				a = -a;

			if (det < 0) {
				a += M_PI;
				float a1 = a + alpha;
				a -= str->arcStep;
				while (a > a1) {
					_add_vertexf(ctx, cosf(a) * str->hw + p0.x, sinf(a) * str->hw + p0.y);
					a -= str->arcStep;
				}
			}
			else {
				float a1 = a + alpha;
				a += str->arcStep;
				while (a < a1) {
					_add_vertexf(ctx, cosf(a) * str->hw + p0.x, sinf(a) * str->hw + p0.y);
					a += str->arcStep;
				}
			}
			uint32_t p0Idx = (uint32_t)(_vertex.size() - ctx->curVertOffset);
			_add_triangle_indices(ctx, idx, idx + 2, idx + 1);
			if (det < 0) {
				for (uint32_t p = idx + 2; p < p0Idx; p++)
					_add_triangle_indices(ctx, p, p + 1, idx);
				_add_triangle_indices(ctx, p0Idx, p0Idx + 2, idx);
				_add_triangle_indices(ctx, idx, p0Idx + 1, p0Idx + 2);
			}
			else {
				for (uint32_t p = idx + 2; p < p0Idx; p++)
					_add_triangle_indices(ctx, p, p + 1, idx + 1);
				_add_triangle_indices(ctx, p0Idx, p0Idx + 1, idx + 1);
				_add_triangle_indices(ctx, idx + 1, p0Idx + 1, p0Idx + 2);
			}
		}

		vp = vec2_mult_s(vec2_perp(v1n), str->hw);
		if (det < 0)
			v.pos = vec2_sub(p0, vp);
		else
			v.pos = vec2_add(p0, vp);
		_vertex.push_back(v);
	}

	return (det < 0);
}

void rvg_t::_draw_stoke_cap(ovg_path_t* ctx, stroke_context_t* str, glm::vec2 p0, glm::vec2 n, bool isStart) {
	Vertex v = {}; v.color = ctx->color; v.uv = { };

	uint32_t firstIdx = (uint32_t)(_vertex.size() - ctx->curVertOffset);

	if (isStart) {
		glm::vec2 vhw = vec2_mult_s(n, str->hw);

		if (ctx->t->lineCap == VG_LINE_CAP_SQUARE)
			p0 = vec2_sub(p0, vhw);

		vhw = vec2_perp(vhw);

		if (ctx->t->lineCap == VG_LINE_CAP_ROUND) {
			if (!str->arcStep)
				str->arcStep = _get_arc_step(ctx, str->hw);

			float a = acosf(n.x) + M_PI_2;
			if (n.y < 0)
				a = M_PI - a;
			float a1 = a + M_PI;

			a += str->arcStep;
			while (a < a1) {
				_add_vertexf(ctx, cosf(a) * str->hw + p0.x, sinf(a) * str->hw + p0.y);
				a += str->arcStep;
			}
			uint32_t p0Idx = (uint32_t)(_vertex.size() - ctx->curVertOffset);
			for (uint32_t p = firstIdx; p < p0Idx; p++)
				_add_triangle_indices(ctx, p0Idx + 1, p, p + 1);
			firstIdx = p0Idx;
		}

		v.pos = vec2_add(p0, vhw);
		_vertex.push_back(v);
		v.pos = vec2_sub(p0, vhw);
		_vertex.push_back(v);

		_add_tri_indices_for_rect(firstIdx);
	}
	else {
		glm::vec2 vhw = vec2_mult_s(n, str->hw);

		if (ctx->t->lineCap == VG_LINE_CAP_SQUARE)
			p0 = vec2_add(p0, vhw);

		vhw = vec2_perp(vhw);

		v.pos = vec2_add(p0, vhw);
		_vertex.push_back(v);
		v.pos = vec2_sub(p0, vhw);
		_vertex.push_back(v);

		firstIdx = (uint32_t)(_vertex.size() - ctx->curVertOffset);

		if (ctx->t->lineCap == VG_LINE_CAP_ROUND) {
			if (!str->arcStep)
				str->arcStep = _get_arc_step(ctx, str->hw);

			float a = acosf(n.x) + M_PI_2;
			if (n.y < 0)
				a = M_PI - a;
			float a1 = a - M_PI;

			a -= str->arcStep;
			while (a > a1) {
				_add_vertexf(ctx, cosf(a) * str->hw + p0.x, sinf(a) * str->hw + p0.y);
				a -= str->arcStep;
			}

			uint32_t p0Idx = (uint32_t)(_vertex.size() - ctx->curVertOffset);
			for (uint32_t p = firstIdx - 1; p < p0Idx; p++)
				_add_triangle_indices(ctx, p + 1, p, firstIdx - 2);
		}
	}
}
float rvg_t::_draw_dashed_segment(ovg_path_t* ctx, stroke_context_t* str, dash_context_t* dc, bool isCurve) {
	// vec2 pL = ctx->points[str->iL];
	glm::vec2 p = ctx->points[str->cp];
	glm::vec2 pR = ctx->points[str->iR];

	if (!dc->dashOn) // we test in fact the next dash start, if dashOn = true => next segment is a void.
		_build_vb_step(ctx, str, isCurve);

	glm::vec2 d = vec2_sub(pR, p);
	dc->normal = vec2_norm(d);
	float segmentLength = glm::length(d);

	while (dc->curDashOffset < segmentLength) {
		glm::vec2 p0 = vec2_add(p, vec2_mult_s(dc->normal, dc->curDashOffset));

		_draw_stoke_cap(ctx, str, p0, dc->normal, dc->dashOn);
		dc->dashOn ^= true;
		dc->curDashOffset += ctx->t->dashes[dc->curDash];
		if (++dc->curDash == ctx->t->dashCount)
			dc->curDash = 0;
	}
	dc->curDashOffset -= segmentLength;
	dc->curDashOffset = fmodf(dc->curDashOffset, dc->totDashLength);
	return segmentLength;
}
void rvg_t::_draw_segment(ovg_path_t* ctx, stroke_context_t* str, dash_context_t* dc, bool isCurve) {
	str->iR = str->cp + 1;
	if (ctx->t->dashCount > 0)
		_draw_dashed_segment(ctx, str, dc, isCurve);
	else
		_build_vb_step(ctx, str, isCurve);
	str->iL = str->cp++;
}

// todo 渲染操作，rvg_t可以多次执行fill或stroke/clip
rvg_t* ovg_new_rvg(mem_resource_t* ac0)
{
	auto ac = (usp_ac_cx*)ac0;
	if (!ac) {
		return 0;
	}
	auto p = ac->new_obj<rvg_t>();
	return p;
}
void ovg_destroy_rvg(rvg_t* p) {
	if (p && p->ac) {
		p->ac->free_obj(p);
	}
}
void ovg_clear(rvg_t* v)
{
	if (v)v->clear_all();
}
void ovg_set_path(rvg_t* v, ovg_path_t* path, vg_state_save_t* st)
{
	if (!v)return;
	v->set_path(path, st);
}
void ovg_reset_clip(rvg_t* v)
{
	if (v)v->clip0();
}
void ovg_clip(rvg_t* v)
{
	if (v)v->clip();
}
void ovg_clip_preserve(rvg_t* v)
{
	if (v)v->clip_preserve();
}
void ovg_clip_rect(rvg_t* v, int x, int y, int width, int height)
{
	glm::ivec4 c[1] = { {x,y,width,height} };
	if (v)v->clip(c);
}
void ovg_set_clip_rect(rvg_t* v, void* rc) {
	if (v && rc) {
		if (v)v->clip((glm::ivec4*)rc);
	}
}
void ovg_get_clip_rect(rvg_t* v, void* rc) {
	if (v && rc) {
		*((glm::ivec4*)rc) = v->curClip;
	}
}
void ovg_stroke(rvg_t* v)
{
	if (!v)return;
	v->stroke_preserve();
	ovg_clear_path(v->cur_path);
}
void ovg_stroke_preserve(rvg_t* v) {
	if (!v)return;
	v->stroke_preserve();
}
void ovg_fill(rvg_t* v)
{
	if (!v)return;
	v->fill_preserve();
	ovg_clear_path(v->cur_path);
}
void ovg_fill_preserve(rvg_t* v)
{
	if (!v)return;
	v->fill_preserve();
}
void ovg_paint(rvg_t* v)
{
	if (v)v->paint();
}

// 添加文本，风格，渲染区可选
void  ovg_add_text(rvg_t* dc, text_st_t* p, text_style_t* ts, text_box_rt* box)
{
	if (dc)dc->gps.add_text(p, ts, box);
}
// 普通图片，支持九宫格、混合颜色
void  ovg_add_image(rvg_t* dc, ovg_image_r* r)
{
	if (dc)dc->gps.add_image(r);
}
// 原始三角形，输入0则不修改
void  ovg_set_geom_state(rvg_t* dc, gem_info_t* info, const glm::mat4* matrix)
{
	if (dc)dc->gps.set_state(info, matrix);
}
// 添加几何数据到缓冲区，xy顶点坐标，color顶点颜色，uv顶点纹理坐标，indices索引数据，color_type=0表示float4，1表示uint32_t
void  ovg_add_geometry(rvg_t* dc, void* texture, const float* xy, int xy_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type)
{
	if (dc)dc->gps.add_geometry(texture, xy, xy_stride, color, color_stride, uv, uv_stride, num_vertices, indices, num_indices, size_indices, color_type);
}
// 添加3D几何数据到缓冲区，xyz顶点坐标，color顶点颜色（双面则要双倍），uv顶点纹理坐标，indices索引数据
void  ovg_add_geometry3d(rvg_t* dc, void* texture, const float* xyz, int xyz_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type)
{
	if (dc)dc->gps.add_geometry3d(texture, xyz, xyz_stride, color, color_stride, uv, uv_stride, num_vertices, indices, num_indices, size_indices, color_type);
}

#endif // 1

// todo init cb
void init_ovg_cb(ovg_canvas_cb* cb) {
	if (!cb)return;
	cb->new_path = ovg_new_path;		// 可自定义分配
	cb->path_destroy = ovg_path_destroy;
	cb->clear_path = ovg_clear_path;
	cb->close_path = ovg_close_path;
	cb->new_sub_path = ovg_new_sub_path;
	cb->path_extents = ovg_path_extents;
	cb->get_current_point = ovg_get_current_point;
	cb->get_segment_count = ovg_get_segment_count;
	cb->set_segment_color = ovg_set_segment_color;
	cb->add_path = ovg_add_path;
	//cb->add_path0 = ovg_add_path0;
	cb->move_to = ovg_move_to;
	cb->rel_move_to = ovg_rel_move_to;
	cb->line_to = ovg_line_to;
	cb->rel_line_to = ovg_rel_line_to;
	cb->arc = ovg_arc;
	cb->arc_negative = ovg_arc_negative;
	cb->curve_to = ovg_curve_to;
	cb->rel_curve_to = ovg_rel_curve_to;
	cb->quadratic_to = ovg_quadratic_to;
	cb->rel_quadratic_to = ovg_rel_quadratic_to;
	cb->rectangle = ovg_rectangle;
	cb->rounded_rectangle = ovg_rounded_rectangle;
	cb->rounded_rectangle2 = ovg_rounded_rectangle2;
	cb->ellipse = ovg_ellipse;
	cb->elliptic_arc_to = ovg_elliptic_arc_to;
	cb->rel_elliptic_arc_to = ovg_rel_elliptic_arc_to;
	cb->circle = ovg_circle;

	cb->new_state = ovg_new_state;
	cb->state_destroy = ovg_state_destroy;
	cb->set_opacity = ovg_set_opacity;
	cb->set_source_color = ovg_set_source_color;
	cb->set_source_rgba = ovg_set_source_rgba;
	cb->set_source_rgb = ovg_set_source_rgb;
	cb->set_line_width = ovg_set_line_width;
	cb->set_miter_limit = ovg_set_miter_limit;
	cb->set_line_cap = ovg_set_line_cap;
	cb->set_line_join = ovg_set_line_join;
	cb->set_source_surface = ovg_set_source_surface;
	cb->set_source = ovg_set_source;
	cb->set_operator = ovg_set_operator;
	cb->set_fill_rule = ovg_set_fill_rule;
	cb->set_dash = ovg_set_dash;
	cb->set_dash8 = ovg_set_dash8;
	cb->translate = ovg_translate;
	cb->scale = ovg_scale;
	cb->rotate = ovg_rotate;
	cb->transform = ovg_transform;
	cb->set_matrix = ovg_set_matrix;
	cb->get_matrix = ovg_get_matrix;
	cb->identity_matrix = ovg_identity_matrix;

	cb->new_pattern_linear = ovg_new_pattern_linear;
	cb->new_pattern_radial = ovg_new_pattern_radial;
	cb->new_pattern_sweep = ovg_new_pattern_sweep;
	cb->pattern_add_color_stop = ovg_pattern_add_color_stop;
	cb->pattern_set_color_stop = ovg_pattern_set_color_stop;
	cb->pattern_set_matrix = ovg_pattern_set_matrix;
	cb->pattern_set_extend = ovg_pattern_set_extend;
	cb->pattern_set_filter = ovg_pattern_set_filter;
	cb->pattern_destroy = ovg_pattern_destroy;

	// 渲染操作，rvg_t可以多次执行fill或stroke/clip
	cb->new_rvg = ovg_new_rvg;
	cb->destroy_rvg = ovg_destroy_rvg;
	cb->set_path = ovg_set_path;
	cb->stroke = ovg_stroke;
	cb->stroke_preserve = ovg_stroke_preserve;
	cb->fill = ovg_fill;
	cb->fill_preserve = ovg_fill_preserve;
	cb->paint = ovg_paint;
	cb->clear = ovg_clear;
	cb->reset_clip = ovg_reset_clip;
	cb->clip = ovg_clip;
	cb->clip_preserve = ovg_clip_preserve;
	cb->clip_rect = ovg_clip_rect;
	cb->set_clip_rect = ovg_set_clip_rect;
	cb->get_clip_rect = ovg_get_clip_rect;
	cb->add_text = ovg_add_text;
	cb->add_image = ovg_add_image;
	cb->set_geom_state = (void (*)(rvg_t*, gem_info_t*, const void*)) ovg_set_geom_state;
	cb->add_geometry = ovg_add_geometry;
	cb->add_geometry3d = ovg_add_geometry3d;

}

#endif // 1


geom_primitive::geom_primitive()
{}

geom_primitive::~geom_primitive()
{}

void geom_primitive::clear()
{
	vd1.clear();
	vd2.clear();
	ids.clear();
	mat = glm::mat4(1.0f);
	curState = {};
}

void geom_primitive::set_state(gem_info_t* info, const glm::mat4* matrix)
{
	if (info) { curState = *info; }
	if (matrix) { mat = *matrix; }
}

bool geom_primitive::add_geometry(void* texture, const float* xy, int xy_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type)
{
	if (!xy || num_vertices < 1)return false;
	geom_cmd_t c = {};
	c.state = curState;
	c.texture = texture;
	c.mat = mat;
	c.ioffset = 0;
	float scale_x = 1.0, scale_y = 1.0;
	float u_scale = 1.0, v_scale = 1.0;
	size_indices = indices ? size_indices : 0;
	ids.reserve(ids.size() + num_indices);
	c.firstIndex = ids.size();
	c.count = num_indices;
	if (num_indices < 1 || size_indices < 1)
	{
		c.count = num_vertices;
	}
	if (curState.shader == ST_DOUBLESIDED) {
		c.vertexOffset = vd2.size();
		c.offset = 1;
		vd2.resize(vd2.size() + num_vertices);
		auto mem = vd2.data() + c.vertexOffset;	// 双面顶点
		auto verts = mem;
		for (size_t i = 0; i < num_indices; i++) {
			int j;
			float* xy_;
			if (size_indices == 4) {
				j = ((const uint32_t*)indices)[i];
			}
			else if (size_indices == 2) {
				j = ((const uint16_t*)indices)[i];
			}
			else if (size_indices == 1) {
				j = ((const uint8_t*)indices)[i];
			}
			else {
				j = i;
			}
			ids.push_back(j);
		}
		for (size_t i = 0; i < num_vertices; i++) {
			float* xy_;
			xy_ = (float*)((char*)xy + i * xy_stride);
			verts->pos.x = xy_[0] * scale_x;
			verts->pos.y = xy_[1] * scale_y;
			if (color_type == 1) {
				auto c8 = (uint32_t*)((char*)color + i * color_stride);
				verts->color = *c8; c8++;
				verts->color1 = *c8;
			}
			else
			{
				auto c4 = (glm::vec4*)((char*)color + i * color_stride);
				verts->color = CreateRgbaf(c4->x, c4->y, c4->z, c4->w); c4++;
				verts->color1 = CreateRgbaf(c4->x, c4->y, c4->z, c4->w);
			}
			if (texture && uv) {
				float* uv_ = (float*)((char*)uv + i * uv_stride);
				verts->uv.x = uv_[0] * u_scale;
				verts->uv.y = uv_[1] * v_scale;
			}
			else {
				verts->uv = { 0.0f, 0.0f };
			}
			verts += 1;
		}
	}
	else
	{
		c.vertexOffset = vd1.size();
		vd1.resize(vd1.size() + num_vertices);
		auto mem = vd1.data() + c.vertexOffset;	// 单面顶点
		auto verts = mem;
		for (size_t i = 0; i < num_indices; i++) {
			int j;
			float* xy_;
			if (size_indices == 4) {
				j = ((const uint32_t*)indices)[i];
			}
			else if (size_indices == 2) {
				j = ((const uint16_t*)indices)[i];
			}
			else if (size_indices == 1) {
				j = ((const uint8_t*)indices)[i];
			}
			else {
				j = i;
			}
			ids.push_back(j);
		}
		for (size_t i = 0; i < num_vertices; i++) {
			float* xy_;
			xy_ = (float*)((char*)xy + i * xy_stride);
			verts->pos.x = xy_[0] * scale_x;
			verts->pos.y = xy_[1] * scale_y;
			if (color_type == 1) {
				auto c8 = (uint32_t*)((char*)color + i * color_stride);
				verts->color = *c8; c8++;
			}
			else
			{
				auto c4 = (glm::vec4*)((char*)color + i * color_stride);
				verts->color = CreateRgbaf(c4->x, c4->y, c4->z, c4->w);
			}
			if (texture && uv) {
				float* uv_ = (float*)((char*)uv + i * uv_stride);
				verts->uv.x = uv_[0] * u_scale;
				verts->uv.y = uv_[1] * v_scale;
			}
			else {
				verts->uv = { 0.0f, 0.0f };
			}
			verts += 1;
		}
	}

	gt->push_back({ .g = c });
	return true;
}

bool geom_primitive::add_geometry3d(void* texture, const float* xyz, int xyz_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type)
{
	if (!xyz || num_vertices < 1)return false;
	geom_cmd_t c = {};
	c.state = curState;
	c.texture = texture;
	c.mat = mat;
	c.ioffset = 0;
	float scale_x = 1.0, scale_y = 1.0, scale_z = 1.0;
	float u_scale = 1.0, v_scale = 1.0;
	size_indices = indices ? size_indices : 0;
	ids.reserve(ids.size() + num_indices);
	c.firstIndex = ids.size();
	c.count = num_indices;
	if (num_indices < 1 || size_indices < 1)
	{
		c.count = num_vertices;
	}
	if (curState.shader == ST_DOUBLESIDED) {
		c.vertexOffset = vd2.size();
		c.offset = 1;
		vd2.resize(vd2.size() + num_vertices);
		auto mem = vd2.data() + c.vertexOffset;	// 双面顶点
		auto verts = mem;
		for (size_t i = 0; i < num_indices; i++) {
			int j;
			if (size_indices == 4) {
				j = ((const uint32_t*)indices)[i];
			}
			else if (size_indices == 2) {
				j = ((const uint16_t*)indices)[i];
			}
			else if (size_indices == 1) {
				j = ((const uint8_t*)indices)[i];
			}
			else {
				j = i;
			}
			ids.push_back(j);
		}
		for (size_t i = 0; i < num_vertices; i++) {
			float* xyz_;
			xyz_ = (float*)((char*)xyz + i * xyz_stride);
			verts->pos.x = xyz_[0] * scale_x;
			verts->pos.y = xyz_[1] * scale_y;
			verts->pos.z = xyz_[2] * scale_z;
			if (color_type == 1) {
				auto c8 = (uint32_t*)((char*)color + i * color_stride);
				verts->color = *c8; c8++;
				verts->color1 = *c8;
			}
			else
			{
				auto c4 = (glm::vec4*)((char*)color + i * color_stride);
				verts->color = CreateRgbaf(c4->x, c4->y, c4->z, c4->w); c4++;
				verts->color1 = CreateRgbaf(c4->x, c4->y, c4->z, c4->w);
			}
			if (texture && uv) {
				float* uv_ = (float*)((char*)uv + i * uv_stride);
				verts->uv.x = uv_[0] * u_scale;
				verts->uv.y = uv_[1] * v_scale;
			}
			else {
				verts->uv = { 0.0f, 0.0f };
			}
			verts += 1;
		}
	}
	else
	{
		c.vertexOffset = vd1.size();
		vd1.resize(vd1.size() + num_vertices);
		auto mem = vd1.data() + c.vertexOffset;	// 单面顶点
		auto verts = mem;
		for (size_t i = 0; i < num_indices; i++) {
			int j;
			if (size_indices == 4) {
				j = ((const uint32_t*)indices)[i];
			}
			else if (size_indices == 2) {
				j = ((const uint16_t*)indices)[i];
			}
			else if (size_indices == 1) {
				j = ((const uint8_t*)indices)[i];
			}
			else {
				j = i;
			}
			ids.push_back(j);
		}
		for (size_t i = 0; i < num_vertices; i++) {
			float* xyz_;
			xyz_ = (float*)((char*)xyz + i * xyz_stride);
			verts->pos.x = xyz_[0] * scale_x;
			verts->pos.y = xyz_[1] * scale_y;
			verts->pos.z = xyz_[2] * scale_z;
			if (color_type == 1) {
				auto c8 = (uint32_t*)((char*)color + i * color_stride);
				verts->color = *c8; c8++;
			}
			else
			{
				auto c4 = (glm::vec4*)((char*)color + i * color_stride);
				verts->color = CreateRgbaf(c4->x, c4->y, c4->z, c4->w);
			}
			if (texture && uv) {
				float* uv_ = (float*)((char*)uv + i * uv_stride);
				verts->uv.x = uv_[0] * u_scale;
				verts->uv.y = uv_[1] * v_scale;
			}
			else {
				verts->uv = { 0.0f, 0.0f };
			}
			verts += 1;
		}
	}
	gt->push_back({ .g = c });
	return true;
}
void geom_primitive::add_text(text_st_t* p, text_style_t* ts, text_box_rt* box)
{
	if (!p || !p->text || !*p->text || !ts || !ts->family || ts->fontsize < 1)return;

}

glm::mat4 ovg_ortho(float width, float height, float znear, float zfar, bool is_top)
{
	return is_top ? glm::ortho(0.0f, width, height, 0.0f, znear, zfar) : glm::ortho(0.0f, width, 0.0f, height, znear, zfar);
}
void draw_mesh2d_x(rvg_t* ctx, geom_primitive* gp, const glm::vec2& render_scale)
{
	mesh2d_x* dc = gp;
	glm::vec2 clip_off = {};
	glm::vec2 clip_scale = render_scale;
	glm::ivec4 vp = { 0,0,-1,-1 };
	if (dc->viewport.z > 0 && dc->viewport.w > 0)
	{
		vp.x = dc->viewport.x;
		vp.y = dc->viewport.y;
		vp.z = dc->viewport.z;
		vp.w = dc->viewport.w;
	}
	auto av = dc;
	auto vd = av->vtxs.data();
	auto vdt = av->vtxs.data();
	auto idv = av->idxs.data();
	auto vbs = av->vtxs.size();
	auto ibs = av->idxs.size();
	std::vector<int> idxs;
	struct { void* texture; uint32_t blendMode; } states = {};
	glm::ivec4 oldclip = {};
	ovg_get_clip_rect(ctx, (int*)&oldclip);
	size_t cclip = 0;
	gem_info_t info = {};
	info.blendMode = (uint8_t)blendMode_e::normal;
	info.topology = 3;
	//info.doubleSided = false;
	//info.depthTestEnable = false;
	//info.depthWriteEnable = false;
	//info.stencilTestEnable = true;
	info.flags = D_STENCILTESTENABLE;
	info.frontFace = 0;
	info.cullMode = 0;
	glm::mat4 mat = ovg_ortho(dc->viewport.z, dc->viewport.w, -1.0f, 1.0f, 0);
	ovg_set_clip_rect(ctx, &vp);
	gp->set_state(&info, &mat);
	for (auto& pcmd : av->cmd_data)
	{
		glm::vec2 clip_min((pcmd.clip_rect.x - clip_off.x) * clip_scale.x, (pcmd.clip_rect.y - clip_off.y) * clip_scale.y);
		glm::vec2 clip_max((pcmd.clip_rect.z - clip_off.x) * clip_scale.x, (pcmd.clip_rect.w - clip_off.y) * clip_scale.y);
		if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
		{
			ovg_set_clip_rect(ctx, &vp); cclip++;
		}
		else
		{
			glm::ivec4 r = { (int)(clip_min.x), (int)(clip_min.y), (int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y) };
			ovg_set_clip_rect(ctx, &r); cclip++;
		}
		auto texture = pcmd.texid;
		auto vertices = vdt + pcmd.vtxOffset;
		const float* xy = &vertices->position.x;
		int stride = sizeof(mesh2d_x::vertex_t);
		auto color = &vertices->color;
		const float* uv = &vertices->tex_coord.x;
		int size_indices = 4;
		auto indices = ibs ? idv + pcmd.idxOffset : nullptr;
		auto num_indices = pcmd.elemCount;
		uint32_t blend = pcmd.blend_mode;
		if (states.blendMode != blend) {
			states.blendMode = blend;
			info.blendMode = states.blendMode;
			gp->set_state(&info, &mat);
		}
		gp->add_geometry(texture, xy, stride, color, stride, uv, stride, pcmd.vCount, indices, num_indices, size_indices, 1);
	}
	dc->clear_m2d();
	if (cclip > 0) {
		ovg_set_clip_rect(ctx, &oldclip);
	}
}

void geom_primitive::add_image(ovg_image_r* r)
{
	if (!r || !r->img || (r->dst.z * r->dst.w <= 0) || (r->rc.z < 1 || r->rc.w < 1))return;
	add_image0(r->img, r->texsize, {}, r->dst, r->rc, r->sliced, r->color);
	draw_mesh2d_x(dc, this, { 1.0,1.0 });
}
#if 1

mesh2d_x::mesh2d_x()
{}

mesh2d_x::~mesh2d_x()
{}

void mesh2d_x::set_viewport(const glm::ivec4& vp)
{
	viewport = vp;
}

void mesh2d_x::set_clip(const glm::ivec4& rc)
{
	_clip_rect = rc;
}

void mesh2d_x::clear_m2d()
{
	vtxs.clear();
	idxs.clear();
	cmd_data.clear();
	_clip_rect = viewport;
	_clip_rect.x = _clip_rect.y = 0;
}

inline uint8_t is_rect_intersect0(int x01, int x02, int y01, int y02,
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
	return is_rect_intersect0(r1.x, r1.y, r1.z, r1.w, r2.x, r2.y, r2.z, r2.w);
}
bool mesh2d_x::nohas_clip(glm::ivec4 a)
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
void mesh2d_x::add(void* user_image, std::vector<vertex_t>& vertex, std::vector<int>& vt_index, const glm::ivec4& clip)
{
	add(user_image, vertex.data(), vertex.size(), vt_index.data(), vt_index.size(), clip);
}

void mesh2d_x::add(void* user_image, vertex_t* vertex, size_t vcount, int* vt_index, size_t icount, const glm::ivec4& clip)
{
	auto ps0 = vcount;
	auto ps = vtxs.size();
	auto ix = idxs.size();
	auto ic = icount;
	vtxs.resize(ps + vcount);
	idxs.resize(ix + icount);
	auto& cd = cmd_data;
	if (cd.empty())
	{
		cd.push_back({});
	}
	auto dt = &cd.back();
	auto pidx = idxs.data() + ix;
	if (dt->texid != user_image || dt->clip_rect != clip)
	{
		if (dt->elemCount > 0)
			cd.push_back({});
		dt = &cd.back();
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
		auto idt = vt_index;
		for (size_t i = 0; i < ic; i++)
		{
			idt[i] += ix;
		}
	}
	memcpy(vtxs.data() + ps, vertex, vcount * sizeof(vertex[0]));
	memcpy(pidx, vt_index, icount * sizeof(vt_index[0]));
}


void mesh2d_x::add_image0(void* img, const glm::ivec2& texsize, const glm::ivec4& clip, const glm::ivec4& dst, const glm::ivec4& src, const glm::ivec4& sliced, uint32_t color)
{
	auto a = glm::vec4(dst);
	glm::ivec2 pos = { a.x, a.y }, size = { a.z, a.w };
	glm::vec4 v4 = { 0, 0, 1, 1 };
	glm::vec4 uv = v4;
	glm::vec2 s = size;
	if (a.z < 0)
		a.z *= -std::min(src.z, texsize.x);
	if (a.w < 0)
		a.w *= -std::min(src.w, texsize.y);
	if (nohas_clip(a))
		return;

	if (sliced.x > 0)
	{
		add_image_sliced(img, texsize, a, sliced, src, color, clip);// 生成九宫格到mesh
	}
	else
	{
		if (!(src.x < 0))
		{
			v4 = src;
			v4.z += v4.x; v4.w += v4.y;//加上原点坐标
			v4.z = glm::min(v4.z, (float)texsize.x);
			v4.w = glm::min(v4.w, (float)texsize.y);
			uv = { v4.x / texsize.x, v4.y / texsize.y, v4.z / texsize.x, v4.w / texsize.y };
			if (uv.x < 0) { uv.x = 0; }
			if (uv.y < 0) { uv.y = 0; }
		}
		glm::vec2 av = pos, cv = { pos.x + s.x, pos.y + s.y }, uv_a = { uv.x, uv.y }, uv_c{ uv.z, uv.w };
		auto& col = color;
		glm::vec2 bv(cv.x, av.y), dv(av.x, cv.y), uv_b(uv_c.x, uv_a.y), uv_d(uv_a.x, uv_c.y);

		vertex_t vertex[] = {
		   {av, uv_a, col},
		   {bv, uv_b, col},
		   {cv, uv_c, col},
		   {dv, uv_d, col},
		};
		int rect_index_order[] = { 0, 1, 2, 0, 2, 3 };
		add(img, vertex, 4, rect_index_order, 6, clip);// 添加矩形(两个三角形)到mesh
	}
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

*/
void mesh2d_x::add_image_sliced(void* user_image, const glm::ivec2& texsize, const glm::ivec4& a, const glm::ivec4& sliced, const glm::ivec4& rect, uint32_t col, const glm::ivec4& clip)
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

	vertex_t vertex[] = {
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
		{{x + width - right, y + height}, {suv.z, uv.w}, col}
	};

	add(user_image, vertex, 16, vt_index.data(), vt_index.size(), clip);

	return;
}
void mesh2d_x::add_image_angle(void* img, const glm::ivec2& texsize, const glm::ivec4& srcrect, const glm::ivec4& dstrect, float angle, const glm::vec2* center, uint32_t col, const glm::ivec4& clip, int flip)
{
	int rect_index_order[] = { 0, 1, 2, 0, 2, 3 };
	glm::ivec4 real_srcrect = {};
	glm::vec2 real_center = {};
	if (flip == FLIP_NONE && (int)(angle / 360) == angle / 360) { // fast path when we don't need rotation or flipping
		add_image0(img, texsize, clip, srcrect, dstrect, {}, col);
		return;
	}
	real_srcrect.x = 0.0f;
	real_srcrect.y = 0.0f;
	real_srcrect.z = (float)texsize.x;
	real_srcrect.w = (float)texsize.y;
	if (center) {
		real_center = *center;
	}
	else {
		real_center.x = dstrect.z / 2.0f;
		real_center.y = dstrect.w / 2.0f;
	}
	vertex_t v[4];
	//float xy[8];
	const int xy_stride = 2 * sizeof(float);
	//float uv[8];
	const int uv_stride = 2 * sizeof(float);
	const int num_vertices = 4;
	const int* indices = rect_index_order;
	const int num_indices = 6;
	const int size_indices = 4;
	glm::vec2 minuv, maxuv;
	glm::vec2 minxy, maxxy;
	float centerx, centery;

	float s_minx, s_miny, s_maxx, s_maxy;
	float c_minx, c_miny, c_maxx, c_maxy;

	const float radian_angle = glm::radians(angle);
	const float s = glm::sin(radian_angle);
	const float c = glm::cos(radian_angle);

	minuv.x = real_srcrect.x / texsize.x;
	minuv.y = real_srcrect.y / texsize.y;
	maxuv.x = (real_srcrect.x + real_srcrect.z) / texsize.x;
	maxuv.y = (real_srcrect.y + real_srcrect.w) / texsize.y;

	centerx = real_center.x + dstrect.x;
	centery = real_center.y + dstrect.y;

	if (flip & FLIP_HORIZONTAL) {
		minxy.x = dstrect.x + dstrect.z;
		maxxy.x = dstrect.x;
	}
	else {
		minxy.x = dstrect.x;
		maxxy.x = dstrect.x + dstrect.z;
	}

	if (flip & FLIP_VERTICAL) {
		minxy.y = dstrect.y + dstrect.w;
		maxxy.y = dstrect.y;
	}
	else {
		minxy.y = dstrect.y;
		maxxy.y = dstrect.y + dstrect.w;
	}

	v[0].tex_coord = minuv;
	v[1].tex_coord = maxuv;
	v[2].tex_coord = maxuv;
	v[3].tex_coord = minuv;

	/* apply rotation with 2x2 matrix ( c -s )
	 *                                ( s  c ) */
	s_minx = s * (minxy.x - centerx);
	s_miny = s * (minxy.y - centery);
	s_maxx = s * (maxxy.x - centerx);
	s_maxy = s * (maxxy.y - centery);
	c_minx = c * (minxy.x - centerx);
	c_miny = c * (minxy.y - centery);
	c_maxx = c * (maxxy.x - centerx);
	c_maxy = c * (maxxy.y - centery);

	// (minx, miny)
	v[0].position = glm::vec2((c_minx - s_miny) + centerx, (s_minx + c_miny) + centery);
	// (maxx, miny)
	v[1].position = glm::vec2((c_maxx - s_miny) + centerx, (s_maxx + c_miny) + centery);
	// (maxx, maxy)
	v[2].position = glm::vec2((c_maxx - s_maxy) + centerx, (s_maxx + c_maxy) + centery);
	// (minx, maxy)
	v[3].position = glm::vec2((c_minx - s_maxy) + centerx, (s_minx + c_maxy) + centery);
	auto c4 = (col);
	v[0].color = c4;
	v[1].color = c4;
	v[2].color = c4;
	v[3].color = c4;
	add(img, v, 4, rect_index_order, 6, clip);
}


#endif // 1

// todo 后端
#if 1

#include "vkh_device.h"
#include "vkh_app.h"
#include "vkh_queue.h"
#include "vkh_image.h"
#include "vkh_buffer.h"

struct pipelinestate_p
{
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VkDescriptorSetLayout descriptorSetLayout;
	gem_info_t state = {};
};
struct ovgVertex2 {
	glm::vec2     pos;
	glm::vec2     uv;
	uint32_t color;
};
#ifndef STENCIL_FILL_BIT
#define STENCIL_FILL_BIT              0x1
#define STENCIL_CLIP_BIT              0x2
#define STENCIL_ALL_BIT               0x3
#define FULLSCREEN_BIT         0x10000000
#define SRCTYPE_MASK           0x000000FF
#endif
struct vg_surface_t {
	VkhImage img;
	int width, height;
};
// 兼容VkhDevice
struct ovg_device_t {
	VkDevice dev;
	VkPhysicalDeviceMemoryProperties phyMemProps;
	VkPhysicalDevice phy;
	VkInstance instance;
	VmaAllocator allocator;
	void* application;
	usp_ac_cx* ac;	// 内存分配器 
	VkPipelineCache pipelineCache;

	VkhQueue gQueue;
	VkCommandPool   cmdPool;
	VkCommandBuffer cmd;
	VkFence fence;

};
struct vg_surface
{
	int   status; /**< Current status of surface, affected by last operation */
	uint32_t        references;
	ovg_device_t* dev;
	uint32_t        width;
	uint32_t        height;
	VkFormat        format;
	VkFramebuffer   fb;
	VkhImage        img;
	VkhImage        imgMS;
	VkhImage        stencil;
	VkCommandPool   cmdPool; // local pools ensure thread safety
	VkCommandBuffer cmd;     // surface local command buffer.
	bool            newSurf;
	mtx_t           mutex;
	VkFence flushFence; // unsignaled idle. 
	VkSemaphore sem;
	VkSemaphore sem0;
};
// vg使用的ubo/vbo/ibo
struct res_vt
{
	ovg_device_t* dev = 0;

	vkh_buffer_t uboGrad = {};
	vkh_buffer_t vertices = {};
	vkh_buffer_t indices = {};
	uint32_t     sizeUBO = 0;
	uint32_t     sizeVBO = 0;
	uint32_t     sizeIBO = 0;
	int	 vstride = 0;
	int	 ustride = 0;
public:
	res_vt();
	~res_vt();
	void create_vertices_buff(int stride);
	void create_uniform_buff(int stride, int count);
	void resize_ubo(uint32_t new_count, int stride);
	void resize_vbo(uint32_t new_size, int stride);
	void resize_ibo(size_t new_size);
	void upload_ubo(void* data, uint32_t offset, uint32_t size, bool flush);
	void upload_vbo(void* data, uint32_t offset, uint32_t size, bool flush);
	void upload_ibo(void* data, uint32_t offset, uint32_t size, bool flush);
};

struct ovg_ctx_t {
	ovg_device_t* dev = 0;
	VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM; VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
	VkSampleCountFlags samples = VK_SAMPLE_COUNT_1_BIT;

	VkDescriptorSetLayout dslPushDset = 0;
	VkPipelineLayout pipelineLayout = 0;
	VkPipeline pipe_OVER = 0; /**< default operator */
	VkPipeline pipe_SUB = 0;
	VkPipeline pipe_CLEAR = 0; /**< clear operator */
	VkPipeline pipelinePolyFill = 0; /**< even-odd polygon filling first step */
	VkPipeline pipelineClipping = 0; /**< draw on stencil to update clipping regions */

	ovg_canvas_cb ccb = {};
	res_vt res_vg = {};
	res_vt res_geom = {};
	PFN_vkCmdBeginRenderingKHR _vkCmdBeginRenderingKHR = VK_NULL_HANDLE;
	PFN_vkCmdEndRenderingKHR _vkCmdEndRenderingKHR = VK_NULL_HANDLE;
	PFN_vkCmdPushDescriptorSet _vkCmdPushDescriptorSet = {};
	uint32_t maxPushDescriptors = 0;
	VkhImage emptyImg = 0;
	size_t gxCount = 0;
	VkRect2D bounds = {};
	VkClearRect clearRect = {};
	vg_pattern_t* cu_pat = 0;
	glm::ivec2 fbo_size = {};
	VkhImage d_img = 0;
	uint64_t d_offset = 0;
	// geom用
	shadermodule_vf shaderModule[5] = {};	// 基础shader
	std::map<uint64_t, pipelinestate_p> pipelines;
	pipelinestate_p* pipeline = nullptr;	// 当前管线
	uint64_t curState = 0;	// 当前状态	
	uint64_t v2offset = 0;
	int status = 0;
	int curClipState = 0;
	bool isdset = false;
	bool cmdStarted = false;
public:
	ovg_ctx_t();
	~ovg_ctx_t();
	void init(ovg_device_t* dev, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlags samples);
	void init_geom();
	void destroy_geom();

	void draw_dynamic(rvg_t* rvg, VkCommandBuffer cmd, vg_fbo_t* fbo, bool clear_all);
	void draw_geom(VkCommandBuffer cmd, geom_cmd_t* c);
	void draw_vg(VkCommandBuffer cmd, vgcmd_t* c, VkRect2D& cuclip);

	void update_va(rvg_t* rvg);
	void dy_start_cmd(VkCommandBuffer cmd);

	pipelinestate_p* get_state(gem_info_t* info);
	pipelinestate_p new_spv_base(gem_info_t* info);
	void bind_geom(VkCommandBuffer cmd, size_t offset, size_t ioffset);
	void push_update_descriptor_set(VkCommandBuffer cmd, pipelinestate_p* cp, VkhImage img);
	VkRect2D set_scissor(VkCommandBuffer cmd, glm::vec4* scissor);
	void bind_draw_pipeline(VkCommandBuffer cmd, vg_state_save_t* t);
	void cmd_draw_full_screen_quad(VkCommandBuffer cmd, vgcmd_t* c, glm::vec4* scissor, VkRect2D* clip);
	void update_push_constants(VkCommandBuffer cmd, vg_state_save_t* t);
	void update_pattern(VkCommandBuffer cmd, vg_pattern_t* pat, vg_state_save_t* st);
	void push_update_descriptor_set_a(VkCommandBuffer cmd, VkhImage img, uint64_t offset);
};
template <typename T>
inline T ovgAlignUp(T val, T align)
{
	return (val + align - 1) / align * align;
}


ovg_ctx_t::ovg_ctx_t()
{}

ovg_ctx_t::~ovg_ctx_t()
{
	destroy_geom();
	vkDestroyPipelineLayout(dev->dev, pipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(dev->dev, dslPushDset, NULL);
#ifndef __APPLE__
	vkDestroyPipeline(dev->dev, pipelinePolyFill, NULL);
#endif
	vkDestroyPipeline(dev->dev, pipelineClipping, NULL);

	vkDestroyPipeline(dev->dev, pipe_OVER, NULL);
	vkDestroyPipeline(dev->dev, pipe_SUB, NULL);
	vkDestroyPipeline(dev->dev, pipe_CLEAR, NULL);
	dslPushDset = 0;
	pipelineLayout = 0;
	pipe_OVER = 0;
	pipe_SUB = 0;
	pipe_CLEAR = 0;
	pipelinePolyFill = 0;
	pipelineClipping = 0;
}


void free_vkdevctx(ovg_device_t* dev) {
	if (dev && dev->ac) {
		auto ac = dev->ac;
		if (dev->pipelineCache) {
			vkDestroyPipelineCache(dev->dev, dev->pipelineCache, 0);
		}
		ac->free_obj(dev);
		delete ac;
	}
}

ovg_device_t* new_vkdevctx(VkDevice vkdev, VkPhysicalDevice phy, VkInstance instance, uint32_t qFamIdx)
{
	auto ac = new usp_ac_cx();
	if (!ac || !vkdev || !phy || !instance) { if (ac) { delete ac; }return 0; }
	ovg_device_t* dev = ac->new_obj<ovg_device_t>();
	dev->dev = vkdev;
	dev->phy = phy;
	dev->instance = instance;
	dev->ac = ac;
	vkGetPhysicalDeviceMemoryProperties(phy, &dev->phyMemProps);
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = phy; allocatorInfo.device = dev->dev;
	vmaCreateAllocator(&allocatorInfo, (VmaAllocator*)&dev->allocator);
	VkhDevice vkhd = (VkhDevice)&dev->dev;
	dev->gQueue = vkh_queue_create(vkhd, qFamIdx, 0);
	dev->cmdPool = vkh_cmd_pool_create(vkhd, dev->gQueue->familyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	dev->cmd = vkh_cmd_buff_create(vkhd, dev->cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	dev->fence = vkh_fence_create_signaled(vkhd);
	return dev;
}
ovg_ctx_t* new_ovgctx(ovg_device_t* dev, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlags samples) {
	if (!dev || !dev->ac)return 0;
	auto p = new ovg_ctx_t();
	p->init(dev, colorFormat, depthFormat, samples);
	return p;
}

VkhImage dc_device_create_empty_texture(ovg_device_t* dev, VkFormat format, int width, int height, const glm::vec4& color);

void ovg_ctx_t::init(ovg_device_t* dev, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlags samples)
{
	if (!dev || !dev->dev)return;
	ovg_ctx_t* ctx = this;
	ctx->dev = dev;
	ctx->colorFormat = colorFormat;
	ctx->depthFormat = depthFormat;
	ctx->samples = samples;
	res_vg.dev = dev;
	res_vg.create_uniform_buff(sizeof(vg_gradient_t), 8);
	res_vg.create_vertices_buff(sizeof(rvg_t::Vertex));
	res_geom.dev = dev;
	res_geom.create_vertices_buff(sizeof(geom_primitive::Vertex2));
	ctx->ccb.ac = (mem_resource_t*)dev->ac;
	init_ovg_cb(&ctx->ccb);
	init_geom();
	emptyImg = dc_device_create_empty_texture(dev, colorFormat, 16, 16, glm::vec4(1.0));

	_vkCmdPushDescriptorSet = (PFN_vkCmdPushDescriptorSet)vkGetDeviceProcAddr(dev->dev, "vkCmdPushDescriptorSet");
	if (!_vkCmdPushDescriptorSet) _vkCmdPushDescriptorSet = (PFN_vkCmdPushDescriptorSet)vkGetInstanceProcAddr(dev->instance, "vkCmdPushDescriptorSet");
	_vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(dev->dev, "vkCmdBeginRenderingKHR"));
	_vkCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(dev->dev, "vkCmdEndRenderingKHR"));


	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	auto hr1 = vkCreatePipelineCache(dev->dev, &pipelineCacheCreateInfo, NULL, &dev->pipelineCache);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN };
	VkPipelineRasterizationStateCreateInfo rasterizationState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f };

	VkPipelineColorBlendAttachmentState blendAttachmentState = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	VkPipelineColorBlendStateCreateInfo colorBlendState = { .sType =
															   VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
														   .attachmentCount = 1,
														   .pAttachments = &blendAttachmentState };

	/*failOp,passOp,depthFailOp,compareOp, compareMask, writeMask, reference;*/
	VkStencilOpState polyFillOpState = { VK_STENCIL_OP_KEEP,
										VK_STENCIL_OP_INVERT,
										VK_STENCIL_OP_KEEP,
										VK_COMPARE_OP_EQUAL,
										STENCIL_CLIP_BIT,
										STENCIL_FILL_BIT,
										0 };
	VkStencilOpState clipingOpState = { VK_STENCIL_OP_ZERO,
										VK_STENCIL_OP_REPLACE,
										VK_STENCIL_OP_KEEP,
										VK_COMPARE_OP_EQUAL,
										STENCIL_FILL_BIT,
										STENCIL_ALL_BIT,
										0x2 };
	VkStencilOpState stencilOpState = { VK_STENCIL_OP_KEEP,
										VK_STENCIL_OP_ZERO,
										VK_STENCIL_OP_KEEP,
										VK_COMPARE_OP_EQUAL,
										STENCIL_FILL_BIT,
										STENCIL_FILL_BIT,
										0x1 };

	VkPipelineDepthStencilStateCreateInfo dsStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_ALWAYS,
		.stencilTestEnable = VK_TRUE,
		.front = polyFillOpState,
		.back = polyFillOpState };

	VkDynamicState dynamicStateEnables[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
		VK_DYNAMIC_STATE_STENCIL_REFERENCE,
		VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
	};
	VkPipelineDynamicStateCreateInfo dynamicState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2, .pDynamicStates = dynamicStateEnables };
	VkPipelineViewportStateCreateInfo viewportState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1, .scissorCount = 1 };
	VkPipelineMultisampleStateCreateInfo multisampleState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = (VkSampleCountFlagBits)ctx->samples };
	/*if (ctx->samples != VK_SAMPLE_COUNT_1_BIT){
		multisampleState.sampleShadingEnable = VK_TRUE;
		multisampleState.minSampleShading = 0.5f;
	}*/
	VkVertexInputBindingDescription vertexInputBinding = { .binding = 0, .stride = sizeof(ovgVertex2), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };
	VkVertexInputAttributeDescription vertexInputAttributs[3] = { {0, 0, VK_FORMAT_R32G32_SFLOAT, 0},
																 {1, 0, VK_FORMAT_R32G32_SFLOAT, 8},
																 {2, 0, VK_FORMAT_R8G8B8A8_UNORM, 16}
	};
	VkPipelineVertexInputStateCreateInfo vertexInputState = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	.vertexBindingDescriptionCount = 1,
	.pVertexBindingDescriptions = &vertexInputBinding,
	.vertexAttributeDescriptionCount = 3,
	.pVertexAttributeDescriptions = vertexInputAttributs };

	VkShaderModule modVert = {}, modFrag = {};
	VkShaderModuleCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = code_len[0].vert, .pCode = (uint32_t*)code[0].vert };
	auto hr = vkCreateShaderModule(dev->dev, &createInfo, NULL, &modVert);
	createInfo.pCode = (uint32_t*)code[0].frag;
	createInfo.codeSize = code_len[0].frag;
	hr = vkCreateShaderModule(dev->dev, &createInfo, NULL, &modFrag);

	VkDescriptorSetLayoutBinding    dsLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL };
	VkDescriptorSetLayoutCreateInfo dsLayoutCreateInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &dsLayoutBinding };
	std::array<VkDescriptorSetLayoutBinding, 2> setLayoutBindings = { };
	dsLayoutBinding.binding = 1;
	setLayoutBindings[1] = dsLayoutBinding;
	dsLayoutBinding.binding = 0;
	dsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	setLayoutBindings[0] = dsLayoutBinding;
	dsLayoutCreateInfo.bindingCount = 2;
	dsLayoutCreateInfo.pBindings = setLayoutBindings.data();
	dsLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
	hr = vkCreateDescriptorSetLayout(dev->dev, &dsLayoutCreateInfo, NULL, &ctx->dslPushDset);
	VkPushConstantRange pushConstantRange[] = {
		{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants_t)},
		//{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(push_constants_t)}
	};
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &ctx->dslPushDset,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = (VkPushConstantRange*)&pushConstantRange };
	hr = vkCreatePipelineLayout(dev->dev, &pipelineLayoutCreateInfo, NULL, &ctx->pipelineLayout);
	VkPipelineShaderStageCreateInfo vertStage = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = modVert,
		.pName = "main",
	};
	VkPipelineShaderStageCreateInfo fragStage = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = modFrag,
		.pName = "main",
	};
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };
	pipelineCreateInfo.stageCount = 1;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pDepthStencilState = &dsStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.layout = ctx->pipelineLayout;
	pipelineCreateInfo.renderPass = nullptr;
	VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &ctx->colorFormat,
		.depthAttachmentFormat = ctx->depthFormat,
		.stencilAttachmentFormat = ctx->depthFormat
	};
	pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;

#ifndef __APPLE__
	hr = vkCreateGraphicsPipelines(dev->dev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &ctx->pipelinePolyFill);
#endif
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	dsStateCreateInfo.back = dsStateCreateInfo.front = clipingOpState;
	dynamicState.dynamicStateCount = 5;
	hr = vkCreateGraphicsPipelines(dev->dev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &ctx->pipelineClipping);

	dsStateCreateInfo.back = dsStateCreateInfo.front = stencilOpState;
	blendAttachmentState.colorWriteMask = 0xf;
	dynamicState.dynamicStateCount = 3;
	pipelineCreateInfo.stageCount = 2;
	hr = vkCreateGraphicsPipelines(dev->dev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &ctx->pipe_OVER);
	blendAttachmentState.alphaBlendOp = blendAttachmentState.colorBlendOp = VK_BLEND_OP_SUBTRACT;
	hr = vkCreateGraphicsPipelines(dev->dev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &ctx->pipe_SUB);
	colorBlendState.logicOpEnable = VK_TRUE;
	blendAttachmentState.blendEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_CLEAR;
	hr = vkCreateGraphicsPipelines(dev->dev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &ctx->pipe_CLEAR);
	vkDestroyShaderModule(dev->dev, modVert, NULL);
	vkDestroyShaderModule(dev->dev, modFrag, NULL);
	return;
}




VkhImage dc_device_create_empty_texture(ovg_device_t* dev, VkFormat format, int width, int height, const glm::vec4& color)
{
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	// create empty image to bind to context source descriptor when not in use
	VkhImage emptyImg = vkh_image_create((VkhDevice)&dev->dev, format, width > 0 ? width : 16, height > 0 ? height : 16, tiling, VKH_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	vkh_image_create_descriptor(emptyImg, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_NEAREST,
		VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	vkDeviceWaitIdle(dev->dev);
	auto cmd = dev->cmd;
	vkh_cmd_begin(dev->cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkClearColorValue       cclr = { {color.x, color.y, color.z, color.w} };
	VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VkhImage img = emptyImg;
	//vkh_image_set_layout(dev->cmd, img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
	//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	//	VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	vkh_image_set_layout(cmd, img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	vkCmdClearColorImage(cmd, vkh_image_get_vkimage(img), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &cclr, 1, &range);
	vkh_image_set_layout(dev->cmd, emptyImg, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	vkh_cmd_end(dev->cmd);
	vkResetFences(dev->dev, 1, &dev->fence);
	vkh_cmd_submit(dev->gQueue, &cmd, dev->fence);
	vkWaitForFences(dev->dev, 1, &dev->fence, VK_TRUE, UINT64_MAX);
	return emptyImg;
}
struct vg_fbo_t0
{
	uint32_t width, height;
	VkhImage img;
	VkhImage imgMS;
	VkhImage depthStencil;
};
void ovg_new_fbo(ovg_ctx_t* ctx, vg_fbo_t0* surf) {
	if (!ctx)return;
	auto dev = ctx->dev;
	if (!dev || !dev->dev || !surf)return;
	VkhDevice vkhd = (VkhDevice)&dev->dev;
	surf->img = vkh_image_create(vkhd, ctx->colorFormat, surf->width, surf->height, VK_IMAGE_TILING_OPTIMAL,
		VKH_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	vkh_image_create_descriptor(surf->img, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_NEAREST,
		VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	if (ctx->samples > VK_SAMPLE_COUNT_1_BIT) {
		surf->imgMS = vkh_image_ms_create(
			vkhd, ctx->colorFormat, (VkSampleCountFlagBits)ctx->samples, surf->width, surf->height, VKH_MEMORY_USAGE_GPU_ONLY,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		vkh_image_create_descriptor(surf->imgMS, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_NEAREST,
			VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
#if defined(DEBUG)  
		vkh_image_set_name(surf->imgMS, "SURF MS color IMG");
		vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)vkh_image_get_view(surf->imgMS),
			"SURF MS color VIEW");
		vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_SAMPLER, (uint64_t)vkh_image_get_sampler(surf->imgMS),
			"SURF MS color SAMPLER");
#endif
	}
	surf->depthStencil = vkh_image_ms_create(vkhd, ctx->depthFormat, (VkSampleCountFlagBits)ctx->samples, surf->width, surf->height,
		VKH_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

	vkh_image_create_descriptor(surf->depthStencil, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT, VK_FILTER_NEAREST,
		VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
#if defined(DEBUG)  
	vkh_image_set_name(surf->depthStencil, "SURF depthStencil");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)vkh_image_get_view(surf->depthStencil),
		"SURF stencil VIEW");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_SAMPLER, (uint64_t)vkh_image_get_sampler(surf->depthStencil),
		"SURF stencil SAMPLER");
#endif
}
void ovg_free_fbo(vg_fbo_t0* surf) {
	if (!surf->img->imported)
		vkh_image_destroy(surf->img);
	vkh_image_destroy(surf->imgMS);
	vkh_image_destroy(surf->depthStencil);
}

void ovg_explicit_ms_resolve(VkCommandBuffer cmd, vg_fbo_t* fbo) {
	auto surf = (vg_fbo_t0*)fbo;
	vkh_image_set_layout(cmd, surf->imgMS, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	vkh_image_set_layout(cmd, surf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	VkImageResolve re = {
					 .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
					 .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
					 .extent = {surf->width, surf->height, 1} };
	vkCmdResolveImage(cmd, vkh_image_get_vkimage(surf->imgMS), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkh_image_get_vkimage(surf->img), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &re);
	vkh_image_set_layout(cmd, surf->imgMS, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	vkh_image_set_layout(cmd, surf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

}

void ovg_ctx_t::draw_dynamic(rvg_t* rvg, VkCommandBuffer cmd, vg_fbo_t* fbo, bool clear_all)
{
	if (!cmd || !_vkCmdBeginRenderingKHR || !fbo)return;
	VkhImage image = (VkhImage)(fbo->imgMS ? fbo->imgMS : fbo->img);
	VkhImage depthStencil = (VkhImage)fbo->depthStencil;
	if (!image || !depthStencil)return;
	update_va(rvg);
	vkh_cmd_begin(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkImageSubresourceRange crange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }; VkImageSubresourceRange dsrange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };
	vkh_image_set_layout_subres(cmd, image, crange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	vkh_image_set_layout_subres(cmd, depthStencil, dsrange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
	VkRenderingAttachmentInfoKHR colorAttachment{
	.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
	.imageView = image->view,
	.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	.loadOp = clear_all ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
	.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	.clearValue = {.color = {0.0f,0.0f,0.0f,0.0f} },
	};
	VkRenderingAttachmentInfoKHR depthStencilAttachment{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
		.imageView = depthStencil->view,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.loadOp = clear_all ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = {.depthStencil = {1.0f,  0} }
	};
	VkRenderingInfoKHR renderingInfo{
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
		.renderArea = { 0, 0, image->infos.extent.width, image->infos.extent.height },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachment,
		.pDepthAttachment = &depthStencilAttachment,
		.pStencilAttachment = &depthStencilAttachment
	};
	_vkCmdBeginRenderingKHR(cmd, &renderingInfo);
	dy_start_cmd(cmd);
	VkViewport viewport = { 0.0f,0.0f,(float)image->infos.extent.width, (float)image->infos.extent.height, 0.0f, 1.0f };
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = { 0, 0, image->infos.extent.width, image->infos.extent.height };
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	pipelinestate_p* cp = 0;
	VkRect2D cuclip = {};
	for (auto& it : rvg->cmdlist) {
		switch (it.g.stype) {
		case 0:
			draw_geom(cmd, &it.g);
			break;
		case 1:
			draw_vg(cmd, &it.vg, cuclip);
			break;
		}
	}
	_vkCmdEndRenderingKHR(cmd);
	// 更新渐变ubo
	vkh_buffer_flush(&res_vg.uboGrad);
	if (fbo->imgMS && fbo->img)
		ovg_explicit_ms_resolve(cmd, fbo);
	vkh_cmd_end(cmd);
}

void free_ovgctx(ovg_ctx_t* p) {
	if (!p)return;
	delete p;
}

ovg_canvas_cb* get_canvas_cb(ovg_ctx_t* ctx)
{
	return &ctx->ccb;
}

void** get_ctx_pipe(ovg_ctx_t* ctx)
{
	return (void**)&ctx->pipelineLayout;
}


#if defined(DEBUG)
const float DBG_LAB_COLOR_RP[4] = { 0, 0, 1, 1 };
const float DBG_LAB_COLOR_FSQ[4] = { 1, 0, 0, 1 };
const float DBG_LAB_COLOR_SAV[4] = { 1, 0, 1, 1 };
const float DBG_LAB_COLOR_CLIP[4] = { 0, 1, 1, 1 };
#endif
#define VG_PTS_SIZE        1024
#define VG_VBO_SIZE        (VG_PTS_SIZE * 4)
#define VG_IBO_SIZE        (VG_VBO_SIZE * 6)
void res_vt::create_uniform_buff(int size, int count) {
	int us = size * count;
	ustride = size;
	vkh_buffer_init((VkhDevice)&dev->dev, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VKH_MEMORY_USAGE_CPU_TO_GPU,
		ovgAlignUp(us, 64), &uboGrad, true);
	sizeUBO = us;
}
res_vt::res_vt()
{
	sizeVBO = VG_VBO_SIZE; sizeIBO = VG_IBO_SIZE;
}
res_vt::~res_vt()
{
	vkh_buffer_reset(&uboGrad);
	vkh_buffer_reset(&vertices);
	vkh_buffer_reset(&indices);
	uboGrad = {};
	vertices = {};
	indices = {};
}
void res_vt::create_vertices_buff(int stride) {
	if (stride < 1)stride = 1;
	vstride = stride;
	sizeVBO *= stride;
	vkh_buffer_init((VkhDevice)&dev->dev, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VKH_MEMORY_USAGE_CPU_TO_GPU,
		sizeVBO, &vertices, true);
	vkh_buffer_init((VkhDevice)&dev->dev, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VKH_MEMORY_USAGE_CPU_TO_GPU,
		sizeIBO * sizeof(uint32_t), &indices, true);
}
void res_vt::resize_ubo(uint32_t new_count, int stride) {

	stride = std::max(1, stride);
	new_count *= stride;
	if (sizeUBO < new_count)
	{
		sizeUBO = new_count;
		int us = sizeUBO;
		vkh_buffer_resize(&uboGrad, ovgAlignUp(us, 64), true);
	}
}
void res_vt::resize_vbo(uint32_t new_size, int stride) {
	uint32_t mod = new_size % VG_VBO_SIZE;
	stride = std::max(1, stride);
	if (mod > 0)
		new_size += VG_VBO_SIZE - mod;
	new_size *= stride;
	if (sizeVBO < new_size)
	{
		sizeVBO = new_size;
		vkh_buffer_resize(&vertices, sizeVBO, true);
	}
}
void res_vt::resize_ibo(size_t new_size) {
	uint32_t mod = new_size % VG_IBO_SIZE;
	if (mod > 0)
		new_size += VG_IBO_SIZE - mod;
	if (sizeVBO < new_size)
	{
		sizeIBO = new_size;
		vkh_buffer_resize(&indices, sizeIBO * sizeof(uint32_t), true);
	}
}

void res_vt::upload_ubo(void* data, uint32_t offset, uint32_t size, bool flush)
{
	memcpy((char*)vkh_buffer_get_mapped_pointer(&uboGrad) + offset, data, size);
	if (flush)
		vkh_buffer_flush(&uboGrad);
}

void res_vt::upload_vbo(void* data, uint32_t offset, uint32_t size, bool flush)
{
	memcpy((char*)vkh_buffer_get_mapped_pointer(&vertices) + offset, data, size);
	if (flush)
		vkh_buffer_flush(&vertices);
}

void res_vt::upload_ibo(void* data, uint32_t offset, uint32_t size, bool flush)
{
	memcpy((char*)vkh_buffer_get_mapped_pointer(&indices) + offset, data, size);
	if (flush)
		vkh_buffer_flush(&indices);
}

void ovg_ctx_t::update_va(rvg_t* rvg)
{
	auto vgsize = rvg->_vertex.size();
	auto isize = rvg->_indices.size();
	if (vgsize > 0) {
		res_vg.resize_vbo(vgsize, sizeof(rvg_t::Vertex));
		res_vg.resize_ibo(isize);
		res_vg.resize_ubo(rvg->gCount, sizeof(vg_gradient_t));
		res_vg.upload_vbo(rvg->_vertex.data(), 0, vgsize * sizeof(rvg_t::Vertex), true);
		res_vg.upload_ibo(rvg->_indices.data(), 0, isize, true);
	}
	// geom数据
	v2offset = rvg->gps.vd1.size() * sizeof(geom_primitive::Vertex1);
	auto v2 = rvg->gps.vd2.size() * sizeof(geom_primitive::Vertex2);
	auto gs = v2offset + v2;
	res_geom.resize_vbo(gs, 1);
	res_geom.resize_ibo(rvg->gps.ids.size());
	if (rvg->gps.vd1.size())
		res_geom.upload_vbo(rvg->gps.vd1.data(), 0, v2offset, false);
	if (rvg->gps.vd2.size())
		res_geom.upload_vbo(rvg->gps.vd2.data(), v2offset, v2, true);
	if (rvg->gps.ids.size())
		res_geom.upload_ibo(rvg->gps.ids.data(), 0, rvg->gps.ids.size() * sizeof(int), true);
}

void ovg_ctx_t::dy_start_cmd(VkCommandBuffer cmd) {

#if defined(DEBUG) 
	vkh_cmd_label_start(cmd, "dc render pass", DBG_LAB_COLOR_RP);
#endif

	VkViewport viewport = { 0, 0, (float)fbo_size.x, (float)fbo_size.y, 0, 1.f };
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	vkCmdSetScissor(cmd, 0, 1, &bounds);

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, &res_vg.vertices.buffer, offsets);
	vkCmdBindIndexBuffer(cmd, res_vg.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
	push_update_descriptor_set_a(cmd, d_img, d_offset);
	cmdStarted = true;
}

void ovg_ctx_t::bind_draw_pipeline(VkCommandBuffer cmd, vg_state_save_t* t) {
	auto ctx = this;
	switch (t->curOperator) {
	case VG_OPERATOR_OVER:
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipe_OVER);
		break;
	case VG_OPERATOR_CLEAR:
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipe_CLEAR);
		break;
	case VG_OPERATOR_DIFFERENCE:
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipe_SUB);
		break;
	default:
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipe_OVER);
		break;
	}
	vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
}

void ovg_ctx_t::cmd_draw_full_screen_quad(VkCommandBuffer cmd, vgcmd_t* c, glm::vec4* scissor, VkRect2D* clip)
{
#if defined(DEBUG)
	vkh_cmd_label_start(cmd, "_draw_full_screen_quad", DBG_LAB_COLOR_FSQ);
#endif
	if (scissor) {
		VkRect2D r = { {(int32_t)glm::max((int)scissor->x, 0), (int32_t)glm::max((int)scissor->y, 0)},
					  {(int32_t)glm::max((int)scissor->z - (int32_t)scissor->x + 1, 1),
					   (int32_t)glm::max((int)scissor->w - (int32_t)scissor->y + 1, 1)} };
		vkCmdSetScissor(cmd, 0, 1, &r);
	}

	uint32_t firstVertIdx = c->full_screen_quad;
	uint32_t fsq_patternType = 0;
	if (c->state)
		fsq_patternType = c->state->pushConsts.fsq_patternType;
	fsq_patternType |= FULLSCREEN_BIT;
	vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 24, 4, &fsq_patternType);
	vkCmdDraw(cmd, 3, 1, firstVertIdx, 0);
	fsq_patternType &= ~FULLSCREEN_BIT;
	vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 24, 4, &fsq_patternType);
	if (scissor)
		vkCmdSetScissor(cmd, 0, 1, clip && clip->extent.width > 0 && clip->extent.height > 0 ? clip : &bounds);

#if defined(DEBUG) 
	vkh_cmd_label_end(cmd);
#endif
}
VkRect2D ovg_ctx_t::set_scissor(VkCommandBuffer cmd, glm::vec4* scissor)
{
	VkRect2D r = { 0,0,-1,-1 };
	if (cmd)
	{
		r = bounds;
		if (scissor)
		{
			r.offset = { (int32_t)scissor->x , (int32_t)scissor->y };
			r.extent = { (uint32_t)glm::max(scissor->z, 1.0f), (uint32_t)std::max(scissor->w, 1.0f) };
		}
		vkCmdSetScissor(cmd, 0, 1, &r);
	}
	return r;
}
void o_sort_gradient_stops(glm::vec4* colors, float* stops, uint32_t count) {
	for (uint32_t i = 1; i < count; i++) {
		float   key_stop = stops[i];
		auto key_color = colors[i];
		int j = (int)i - 1;
		while (j >= 0 && stops[j] > key_stop) {
			stops[j + 1] = stops[j];
			colors[j + 1] = colors[j];
			j--;
		}
		stops[j + 1] = key_stop;
		colors[j + 1] = key_color;
	}
}
void ovg_ctx_t::update_push_constants(VkCommandBuffer cmd, vg_state_save_t* t) {
	t->pushConsts.size = { (float)fbo_size.x, (float)fbo_size.y };
	vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants_t), &t->pushConsts);
}
void ovg_ctx_t::update_pattern(VkCommandBuffer cmd, vg_pattern_t* pat, vg_state_save_t* st) {
	if (!cmd)return;
	vg_pattern_t* lastPat = cu_pat;
	cu_pat = pat;
	uint32_t newPatternType = VG_PATTERN_TYPE_SOLID;
	if (pat == NULL) {       // solid color
		if (lastPat == NULL) // solid
			return;          // solid to solid transition, no extra action requested
	}
	else
		newPatternType = pat->type;
	switch (newPatternType) {
	case VG_PATTERN_TYPE_SOLID:
		push_update_descriptor_set_a(cmd, 0, 0);
		break;
	case VG_PATTERN_TYPE_SURFACE:
	{
		auto surf = (vg_surface_t*)pat->data;
		vkh_cmd_begin(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		// transition source surface for sampling
		vkh_image_set_layout(cmd, surf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		vkh_cmd_end(cmd);
		VkSamplerAddressMode addrMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkFilter             filter = VK_FILTER_NEAREST;
		switch (pat->extend) {
		case VG_EXTEND_NONE:
			addrMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			break;
		case VG_EXTEND_PAD:
			addrMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			break;
		case VG_EXTEND_REPEAT:
			addrMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			break;
		case VG_EXTEND_REFLECT:
			addrMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			break;
		}
		switch (pat->filter) {
		case VG_FILTER_BILINEAR:
		case VG_FILTER_BEST:
			filter = VK_FILTER_LINEAR;
			break;
		default:
			filter = VK_FILTER_NEAREST;
			break;
		}
		vkh_image_create_sampler(surf->img, filter, filter, VK_SAMPLER_MIPMAP_MODE_NEAREST, addrMode);
		push_update_descriptor_set_a(cmd, surf->img, 0);
		st->pushConsts.source.z = (float)surf->width;
		st->pushConsts.source.w = (float)surf->height;
		glm::mat3x3 mat;
		if (pat->hasMatrix) {
			mat = pat->matrix;
			st->pushConsts.matInv = st->pushConsts.matInv * mat;
		}
	}
	break;
	case VG_PATTERN_TYPE_LINEAR:
	case VG_PATTERN_TYPE_RADIAL:
	case VG_PATTERN_TYPE_SWEEP:
	{
		float fm = std::max((float)fbo_size.x, (float)fbo_size.y);
		glm::vec4 bounds = { fm,fm,0.0f,0.0f }; // store img bounds in unused source field
		st->pushConsts.source = bounds;
		// transform control point with current ctx matrix 
		vg_gradient_t grad = *(vg_gradient_t*)pat->data;
		if (grad.count < 2) {
			status = -1;// 错误
			return;
		}
		grad.extend = pat->extend;
		glm::mat3x2 mat;
		if (pat->hasMatrix) {
			glm::mat3x3 m = pat->matrix;
			mat = glm::inverse(m);
			matrix_transform_point(&mat, &grad.cp[0].x, &grad.cp[0].y);
		}
		matrix_transform_point(&st->pushConsts.mat, &grad.cp[0].x, &grad.cp[0].y);
		if (pat->type == VG_PATTERN_TYPE_LINEAR) {
			if (pat->hasMatrix)
				matrix_transform_point(&mat, &grad.cp[0].z, &grad.cp[0].w);
			matrix_transform_point(&st->pushConsts.mat, &grad.cp[0].z, &grad.cp[0].w);
		}
		else {
			if (pat->hasMatrix)
				matrix_transform_point(&mat, &grad.cp[1].x, &grad.cp[1].y);
			matrix_transform_point(&st->pushConsts.mat, &grad.cp[1].x, &grad.cp[1].y);
			// radii
			if (pat->hasMatrix) {
				matrix_transform_distance(&mat, &grad.cp[0].z, &grad.cp[0].w);
				matrix_transform_distance(&mat, &grad.cp[1].z, &grad.cp[0].w);
			}
			matrix_transform_distance(&st->pushConsts.mat, &grad.cp[0].z, &grad.cp[0].w);
			matrix_transform_distance(&st->pushConsts.mat, &grad.cp[1].z, &grad.cp[0].w);
		}
		o_sort_gradient_stops(grad.colors, grad.stops, grad.count);
		memcpy(((char*)vkh_buffer_get_mapped_pointer(&res_vg.uboGrad)) + gxCount, &grad, sizeof(vg_gradient_t));
		//vkh_buffer_flush(&ctx->uboGrad); 
		push_update_descriptor_set_a(cmd, 0, gxCount);
		gxCount += sizeof(vg_gradient_t);
	}
	break;
	}
	st->pushConsts.fsq_patternType = (st->pushConsts.fsq_patternType & FULLSCREEN_BIT) + newPatternType;

}

void ovg_ctx_t::push_update_descriptor_set_a(VkCommandBuffer cmd, VkhImage img, uint64_t offset) {
	if (!maxPushDescriptors || !_vkCmdPushDescriptorSet)return;
	d_img = img;//VkhImage
	d_offset = offset;
	if (!cmdStarted)
	{
		isdset = true;
		return;
	}
	VkDescriptorImageInfo dst1 = vkh_image_get_descriptor(img ? img : emptyImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	VkWriteDescriptorSet  wimg = {
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.dstSet = 0,
	.dstBinding = 1,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	.pImageInfo = 0 };
	VkDescriptorBufferInfo dbi = { res_vg.uboGrad.buffer, offset, sizeof(vg_gradient_t) };
	VkWriteDescriptorSet   wu = {
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.dstSet = 0,
	.dstBinding = 0,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	.pBufferInfo = &dbi };
	VkWriteDescriptorSet  wds[3] = {};
	wimg.dstBinding = 1;
	wimg.pImageInfo = &dst1;
	wds[1] = wimg;
	wds[0] = wu;
	_vkCmdPushDescriptorSet(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 2, wds);
}
pipelinestate_p* ovg_ctx_t::get_state(gem_info_t* info)
{
	uint64_t v = *(uint64_t*)info;
	if (!pipeline || curState != v)
	{
		curState = v;
		auto& pl = pipelines[v];
		if (!pl.pipeline) {
			pipelinestate_p p0 = new_spv_base(info);
			pl = p0;
		}
		if (pl.pipeline)
			pipeline = &pl;
	}
	return pipeline;
}

// 普通图片三角形渲染
#if 1

namespace ovg {
	VkShaderModule newModule(VkDevice device, const uint32_t* SpvData, size_t SpvSize, VkResult* r)
	{
		VkShaderModule pShaderModule = {};
		VkShaderModuleCreateInfo moduleCreateInfo = {};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pCode = (uint32_t*)SpvData;
		moduleCreateInfo.codeSize = SpvSize;
		auto hr = vkCreateShaderModule(device, &moduleCreateInfo, NULL, &pShaderModule);
		if (r)*r = hr;
		return pShaderModule;
	}
	void freeModule(VkDevice device, VkShaderModule module0)
	{
		if (module0)
			vkDestroyShaderModule(device, module0, NULL);
	}
#define new_module(device,spvdata,r) ovg::newModule(device,spvdata,sizeof(spvdata),r)

#define BLENDMODE_NONE                  0x00000000u /**< no blending: dstRGBA = srcRGBA */
#define BLENDMODE_BLEND                 0x00000001u /**< alpha blending: dstRGB = (srcRGB * srcA) + (dstRGB * (1-srcA)), dstA = srcA + (dstA * (1-srcA)) */
#define BLENDMODE_BLEND_PREMULTIPLIED   0x00000010u /**< pre-multiplied alpha blending: dstRGBA = srcRGBA + (dstRGBA * (1-srcA)) */
#define BLENDMODE_ADD                   0x00000002u /**< additive blending: dstRGB = (srcRGB * srcA) + dstRGB, dstA = dstA */
#define BLENDMODE_ADD_PREMULTIPLIED     0x00000020u /**< pre-multiplied additive blending: dstRGB = srcRGB + dstRGB, dstA = dstA */
#define BLENDMODE_MOD                   0x00000004u /**< color modulate: dstRGB = srcRGB * dstRGB, dstA = dstA */
#define BLENDMODE_MUL                   0x00000008u /**< color multiply: dstRGB = (srcRGB * dstRGB) + (dstRGB * (1-srcA)), dstA = dstA */

	void set_cblend(VkPipelineColorBlendAttachmentState& opt, const std::array<uint32_t, 6>& b) {
		opt.srcColorBlendFactor = (VkBlendFactor)b[0];
		opt.dstColorBlendFactor = (VkBlendFactor)b[1];
		opt.colorBlendOp = (VkBlendOp)b[2];
		opt.srcAlphaBlendFactor = (VkBlendFactor)b[3];
		opt.dstAlphaBlendFactor = (VkBlendFactor)b[4];
		opt.alphaBlendOp = (VkBlendOp)b[5];
	}
	/*
		VkBlendFactor            srcColorBlendFactor;
		VkBlendFactor            dstColorBlendFactor;
		VkBlendOp                colorBlendOp;
		VkBlendFactor            srcAlphaBlendFactor;
		VkBlendFactor            dstAlphaBlendFactor;
		VkBlendOp                alphaBlendOp;
		VkColorComponentFlags    colorWriteMask;
	*/

#if 1
#define BLENDMODE_NONE_FULL set_cblend(cba,{VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD})
#define BLENDMODE_BLEND_FULL set_cblend(cba,{VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD})
#define BLENDMODE_BLEND_PREMULTIPLIED_FULL set_cblend(cba,{VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD})
#define BLENDMODE_ADD_FULL set_cblend(cba,{VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD})
#define BLENDMODE_ADD_PREMULTIPLIED_FULL set_cblend(cba,{VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD})
#define BLENDMODE_MOD_FULL set_cblend(cba,{VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD})
#define BLENDMODE_MUL_FULL set_cblend(cba,{VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD})
#define BLENDMODE_SCREEN_FULL set_cblend(cba,{VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, VK_BLEND_OP_ADD})
#endif

	// Color blend
	void set_blend(VkPipelineColorBlendAttachmentState& colorBlendAttachment, uint32_t blendMode)
	{
		colorBlendAttachment = {};
		colorBlendAttachment.blendEnable = VK_TRUE;
		auto bm = (blendMode_e)blendMode;
		auto& cba = colorBlendAttachment;
		switch (bm)
		{
		case blendMode_e::none:
			BLENDMODE_NONE_FULL;
			colorBlendAttachment.blendEnable = VK_FALSE;
			break;
		case blendMode_e::normal:
			BLENDMODE_BLEND_FULL;
			break;
		case blendMode_e::additive:
			BLENDMODE_ADD_FULL;
			break;
		case blendMode_e::normal_prem:
			BLENDMODE_BLEND_PREMULTIPLIED_FULL;
			break;
		case blendMode_e::additive_prem:
			BLENDMODE_ADD_PREMULTIPLIED_FULL;
			break;
		case blendMode_e::multiply:
			BLENDMODE_MUL_FULL;
			break;
		case blendMode_e::modulate:
			BLENDMODE_MOD_FULL;
			break;
		case blendMode_e::screen:
			BLENDMODE_SCREEN_FULL;
			break;
		default:
			break;
		}
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}
	struct pipe_data {
		VkDevice dev = {};
		shadermodule_vf* spv = 0;
		VkRenderPass renderPass = {};
		uint32_t colorCount = 0;
		VkFormat colorFormat[8];
		VkFormat depthFormat;
		VkSampleCountFlags samples;
	};

	pipelinestate_p newPipelineState(pipe_data* pd, gem_info_t* info)
	{
		pipelinestate_p pipelineStates = {};
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkResult result = VK_SUCCESS;
		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
		VkVertexInputAttributeDescription attributeDescriptions[3];
		VkVertexInputBindingDescription bindingDescriptions[1];
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2];
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.flags = 0;
		pipelineCreateInfo.pStages = shaderStageCreateInfo;
		pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
		pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
		pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
		pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
		pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
		int shader = info->shader;
		// Shaders
		const char* name = "main";
		for (uint32_t i = 0; i < 2; i++) {
			shaderStageCreateInfo[i] = {};
			shaderStageCreateInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCreateInfo[i].module = (i == 0) ? pd->spv[shader].vert : pd->spv[shader].frag;
			shaderStageCreateInfo[i].stage = (i == 0) ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderStageCreateInfo[i].pName = name;
		}
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = &shaderStageCreateInfo[0];


		VkPipelineLayout pipelineLayout = {};
		VkDescriptorSetLayout descriptorSetLayout = {};
		VkPushConstantRange pushConstantRange[] = {
			{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4) + sizeof(float)},
			//{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(push_constants)}
		};
		std::array<VkDescriptorSetLayoutBinding, 3> laybs;
		VkDescriptorSetLayoutBinding layb_tex, layb_tex_mask, layb_ubo_ins;
		int binc = 1;
		layb_tex.binding = 0;	// 纹理
		layb_tex.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layb_tex.descriptorCount = 1;
		layb_tex.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layb_tex.pImmutableSamplers = NULL;
		if (shader == ST_MASK) {
			layb_tex_mask.binding = 1;	// 遮罩纹理
			layb_tex_mask.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layb_tex_mask.descriptorCount = 1;
			layb_tex_mask.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layb_tex_mask.pImmutableSamplers = NULL;
			binc++;
		}
		if (shader == ST_INSTANCE || shader == ST_INSTANCE_DOUBLESIDED) {
			layb_tex.binding = 1;
			layb_ubo_ins.binding = 0;	// 实例
			layb_ubo_ins.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			layb_ubo_ins.descriptorCount = 1;
			layb_ubo_ins.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			layb_ubo_ins.pImmutableSamplers = NULL;
			binc++;
		}
		VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
		descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_layout.pNext = NULL;
		descriptor_layout.bindingCount = (uint32_t)binc;
		descriptor_layout.pBindings = laybs.data();
		descriptor_layout.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
		result = vkCreateDescriptorSetLayout(pd->dev, &descriptor_layout, NULL, &descriptorSetLayout);
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = (VkPushConstantRange*)&pushConstantRange;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
		result = vkCreatePipelineLayout(pd->dev, &pipelineLayoutCreateInfo, NULL, &pipelineLayout);
		/*
		layout(location = 0) in vec3 pos;
		layout(location = 1) in vec2 uv;
		layout(location = 2) in vec4 col;
		layout(location = 3) in vec4 col1;
		*/
		VkVertexInputAttributeDescription vi_attrs[] =
		{
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
			{ 1, 0, VK_FORMAT_R32G32_SFLOAT, 12 },
			{ 2, 0, VK_FORMAT_R8G8B8A8_UNORM, 20 },
			{ 3, 0, VK_FORMAT_R8G8B8A8_UNORM, 24 },
		};
		//uint32_t    location;
		//uint32_t    binding;
		//VkFormat    format;
		//uint32_t    offset;
		//_countof(vi_attrs); 
		// Vertex input
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = 3 + shader;
		vertexInputCreateInfo.pVertexAttributeDescriptions = vi_attrs;
		vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescriptions[0];

		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		bindingDescriptions[0].stride = sizeof(float) * 5 + sizeof(int) * (shader + 1);

		// Input assembly
		inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCreateInfo.topology = (VkPrimitiveTopology)(info->topology > VK_PRIMITIVE_TOPOLOGY_PATCH_LIST ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : info->topology);
		inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.scissorCount = 1;
		viewportStateCreateInfo.viewportCount = 1;

		// Dynamic states
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		VkDynamicState dynamicStates[2] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		dynamicStateCreateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
		dynamicStateCreateInfo.pDynamicStates = dynamicStates;

		// Rasterization state
		rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
		rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
		rasterizationStateCreateInfo.polygonMode = (VkPolygonMode)(info->polygon > VK_POLYGON_MODE_POINT ? 0 : info->polygon);
		rasterizationStateCreateInfo.frontFace = (VkFrontFace)(info->frontFace > VK_FRONT_FACE_COUNTER_CLOCKWISE ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE);
		rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
		rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
		rasterizationStateCreateInfo.lineWidth = info->lineWidth;

		// MSAA state
		multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		VkSampleMask multiSampleMask = 0xFFFFFFFF;
		multisampleStateCreateInfo.pSampleMask = &multiSampleMask;
		multisampleStateCreateInfo.rasterizationSamples = (VkSampleCountFlagBits)pd->samples;

		// Depth Stencil
		auto& ds = depthStencilStateCreateInfo;
		ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		ds.pNext = NULL;
		ds.flags = 0;
		ds.depthTestEnable = info->flags & D_DEPTHTESTENABLE;
		ds.depthWriteEnable = info->flags & D_DEPTHWRITEENABLE;
		ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		ds.back.failOp = VK_STENCIL_OP_KEEP;
		ds.back.passOp = VK_STENCIL_OP_KEEP;
		ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
		ds.back.compareMask = 0;
		ds.back.reference = 0;
		ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
		ds.back.writeMask = 0;
		ds.depthBoundsTestEnable = VK_FALSE;
		ds.minDepthBounds = 0;
		ds.maxDepthBounds = 0;
		ds.stencilTestEnable = info->flags & D_STENCILTESTENABLE;
		ds.front = ds.back;
		VkStencilOpState clipingOpState = { VK_STENCIL_OP_ZERO,
										VK_STENCIL_OP_REPLACE,
										VK_STENCIL_OP_KEEP,
										VK_COMPARE_OP_EQUAL,
										STENCIL_FILL_BIT,
										STENCIL_ALL_BIT,
										0x2 };
		//ds.back = ds.front = clipingOpState;
		// Color blend
		VkPipelineColorBlendAttachmentState colorBlendAttachment = { 0 };
		colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCreateInfo.attachmentCount = 1;
		colorBlendStateCreateInfo.pAttachments = &colorBlendAttachment;
		set_blend(colorBlendAttachment, info->blendMode);
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		// Renderpass / layout

		pipelineCreateInfo.subpass = 0;
		pipelineCreateInfo.layout = pipelineLayout;
		VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
		.colorAttachmentCount = pd->colorCount,
		.pColorAttachmentFormats = pd->colorFormat,
		.depthAttachmentFormat = pd->depthFormat,
		.stencilAttachmentFormat = pd->depthFormat
		};
		//if (info->dynamicrenderingEnable)
		pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
		//else
		//	pipelineCreateInfo.renderPass = pd->dev->renderPass;
		result = vkCreateGraphicsPipelines(pd->dev, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &pipeline);
		if (result != VK_SUCCESS) {
			return {};
		}

		pipelineStates.state = *info;
		pipelineStates.pipeline = pipeline;
		pipelineStates.descriptorSetLayout = descriptorSetLayout;
		pipelineStates.pipelineLayout = pipelineCreateInfo.layout;
		return pipelineStates;
	}
	void freePipelineState(VkDevice device, pipelinestate_p* pipelineStates)
	{
		if (pipelineStates->pipeline) {
			vkDestroyPipeline(device, pipelineStates->pipeline, NULL);
			pipelineStates->pipeline = VK_NULL_HANDLE;
		}
		if (pipelineStates->pipelineLayout) {
			vkDestroyPipelineLayout(device, pipelineStates->pipelineLayout, NULL);
			pipelineStates->pipelineLayout = VK_NULL_HANDLE;
		}
		if (pipelineStates->descriptorSetLayout) {
			vkDestroyDescriptorSetLayout(device, pipelineStates->descriptorSetLayout, NULL);
			pipelineStates->descriptorSetLayout = VK_NULL_HANDLE;
		}
	}
}
//!ovg

void ovg_ctx_t::destroy_geom() {

	for (size_t i = 0; i < 5; i++)
	{
		ovg::freeModule(dev->dev, shaderModule[i].vert);
		ovg::freeModule(dev->dev, shaderModule[i].frag);
		shaderModule[i] = {};
	}
	for (auto& [k, p] : pipelines) {
		ovg::freePipelineState((VkDevice)dev->dev, &p);
	}
}
void ovg_ctx_t::init_geom()
{
	for (size_t i = 0; i < 5; i++)
	{
		shaderModule[i] = { ovg::newModule(dev->dev, code[1 + i].vert, code_len[1 + i].vert, 0),
			ovg::newModule(dev->dev, code[1 + i].frag, code_len[1 + i].frag, 0) };
	}

}
pipelinestate_p ovg_ctx_t::new_spv_base(gem_info_t* info)
{
	ovg::pipe_data pd[1] = {};
	pd->dev = dev->dev;
	pd->spv = shaderModule;
	//pd->vert[0] = [0];
	//pd->frag[0] = shaderModule[1];
	//pd->vert[1] = shaderModule[2];
	//pd->frag[1] = shaderModule[3];
	pd->colorCount = 1;		// 一个输出随件
	pd->colorFormat[0] = colorFormat;
	pd->depthFormat = depthFormat;
	pd->samples = samples;
	auto p = ovg::newPipelineState(pd, info);

	return p;
}
#endif // 1
void ovg_ctx_t::bind_geom(VkCommandBuffer cmd, size_t offset, size_t ioffset)
{
	VkDeviceSize offsets[1] = { offset };
	vkCmdBindVertexBuffers(cmd, 0, 1, &res_geom.vertices.buffer, offsets);
	vkCmdBindIndexBuffer(cmd, res_geom.indices.buffer, ioffset, VK_INDEX_TYPE_UINT32);
}
void ovg_ctx_t::push_update_descriptor_set(VkCommandBuffer cmd, pipelinestate_p* cp, VkhImage img)
{
	if (!_vkCmdPushDescriptorSet || !img)return;
	VkDescriptorImageInfo descSrcTex = vkh_image_get_descriptor(img ? img : emptyImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	VkWriteDescriptorSet  writeDescriptorSet = {
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.dstSet = 0,
	.dstBinding = 1,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	.pImageInfo = &descSrcTex };
	_vkCmdPushDescriptorSet(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cp->pipelineLayout, 0, 1, &writeDescriptorSet);
}
void ovg_ctx_t::draw_geom(VkCommandBuffer cmd, geom_cmd_t* c)
{
	if (!c)return;
	auto cp = get_state(&c->state);
	if (cp)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cp->pipeline);
		vkCmdPushConstants(cmd, cp->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0
			, sizeof(glm::mat4) + sizeof(float), &c->mat);
		push_update_descriptor_set(cmd, cp, (VkhImage)c->texture);
	}
	bind_geom(cmd, c->offset * v2offset, c->ioffset);
	if (c->firstIndex == -1)
		vkCmdDraw(cmd, c->count, 1, c->vertexOffset, 0);
	else
		vkCmdDrawIndexed(cmd, c->count, 1, c->firstIndex, c->vertexOffset, 0);
}
const VkClearAttachment clearStencil = { VK_IMAGE_ASPECT_STENCIL_BIT, 1, {{{1,0}}} };
const VkClearAttachment clearColorAttach = { VK_IMAGE_ASPECT_COLOR_BIT, 0, {{{0}}} };

void ovg_ctx_t::draw_vg(VkCommandBuffer cmd, vgcmd_t* c, VkRect2D& cuclip)
{
	auto t = c->state;
	if (t)
	{
		bind_draw_pipeline(cmd, t);
		update_pattern(cmd, t->pattern, t);
		update_push_constants(cmd, t);
		if (!cmdStarted)
			cmdStarted = true;
		push_update_descriptor_set_a(cmd, d_img, d_offset);
	}
	switch (c->type)
	{
	case 0:// 填充
	{
		if (t && t->curFillRule == VG_FILL_RULE_EVEN_ODD) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinePolyFill);// 奇偶填充
			for (size_t i = 0; i < c->vc; i++) { vkCmdDraw(cmd, c->v[i].vertexCount, 1, c->v[i].firstVertex, 0); }
			bind_draw_pipeline(cmd, t);
			vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
			cmd_draw_full_screen_quad(cmd, c, &c->bounds, &cuclip);
			vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
		}
		else { vkCmdDrawIndexed(cmd, c->index.y, 1, c->index.x, (int32_t)c->vertex.x, 0); }// 非零填充
	}
	break;
	case 1: // 描边
	{
		vkCmdDrawIndexed(cmd, c->index.y, 1, c->index.x, (int32_t)c->vertex.x, 0);
	}
	break;
	case 2:// 裁剪
	{
		int bw = c->bounds.z; int bh = c->bounds.w;
		if (bw != 0 && bh != 0) { cuclip = set_scissor(cmd, bw < 0 || bh < 0 ? nullptr : &c->bounds); break; }
		if (c->vc > 0 || c->index.y > 0) {
#if defined(DEBUG) 
			vkh_cmd_label_start(cmd, "clip", DBG_LAB_COLOR_CLIP);
#endif
			if (t && t->curFillRule == VG_FILL_RULE_EVEN_ODD) {
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinePolyFill);
				for (size_t i = 0; i < c->vc; i++) { vkCmdDraw(cmd, c->v[i].vertexCount, 1, c->v[i].firstVertex, 0); }
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineClipping);
			}
			else {
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineClipping);
				vkCmdSetStencilReference(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
				vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
				vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
				vkCmdDrawIndexed(cmd, c->index.y, 1, c->index.x, (int32_t)c->vertex.x, 0);
			}
			vkCmdSetStencilReference(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
			vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
			vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_ALL_BIT);
			cmd_draw_full_screen_quad(cmd, c, NULL, 0);
			curClipState = vg_clip_state_clip;
#if defined(DEBUG)  
			vkh_cmd_label_end(cmd);
#endif
		}
		else {
			auto cs = clearStencil;
			cs.clearValue.depthStencil.depth = 1;
			cs.clearValue.depthStencil.stencil = 0;
			vkCmdClearAttachments(cmd, 1, &cs, 1, &clearRect);
		}
	}
	break;
	case 3:
	{
		cmd_draw_full_screen_quad(cmd, c, NULL, 0);
	}
	break;
	case 4:
	{
		VkClearAttachment ca[2] = { clearColorAttach, clearStencil };
		ca[1].clearValue.depthStencil.depth = 1;
		vkCmdClearAttachments(cmd, 2, ca, 1, &clearRect);
	}
	break;
	}
}

// todo gpu

static SDL_GPUShader* compileShader(SDL_GPUDevice* device, SDL_GPUShaderStage stage)
{
	SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
	// SDL_GPU_SHADERSTAGE_VERTEX,
	// SDL_GPU_SHADERSTAGE_FRAGMENT
	SDL_GPUShaderCreateInfo sci = { };
	//sci.code = code[stage];
	//sci.code_size = code_len[stage];
	sci.format = SDL_GPU_SHADERFORMAT_SPIRV;
	// FIXME not sure if this is correctstage ? "fragMain" :
	sci.entrypoint = "main";
	sci.num_samplers = stage;
	sci.num_uniform_buffers = stage;
	sci.stage = stage;

	return SDL_CreateGPUShader(device, &sci);
}
void* new_gpu()
{
	SDL_PropertiesID create_props = SDL_CreateProperties();
	bool debug = SDL_GetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, false);
	bool lowpower = SDL_GetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_PREFERLOWPOWER_BOOLEAN, false);

	debug = SDL_GetHintBoolean(SDL_HINT_RENDER_GPU_DEBUG, debug);
	lowpower = SDL_GetHintBoolean(SDL_HINT_RENDER_GPU_LOW_POWER, lowpower);

	SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, debug);
	SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_PREFERLOWPOWER_BOOLEAN, lowpower);

	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING))
	{
		SDL_SetStringProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "vulkan");
	}
	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_D3D12_ALLOW_FEWER_RESOURCE_SLOTS_BOOLEAN)) {
		SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_D3D12_ALLOW_FEWER_RESOURCE_SLOTS_BOOLEAN, true);
	}
	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN)) {
		SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN, false);
	}
	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_DEPTH_CLAMPING_BOOLEAN)) {
		SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_DEPTH_CLAMPING_BOOLEAN, false);
	}
	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN)) {
		SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN, false);
	}
	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_ANISOTROPY_BOOLEAN)) {
		SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_ANISOTROPY_BOOLEAN, false);
	}
	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_METAL_ALLOW_MACFAMILY1_BOOLEAN)) {
		SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_METAL_ALLOW_MACFAMILY1_BOOLEAN, false);
	}
	SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, 1);
	auto device = SDL_CreateGPUDeviceWithProperties(create_props);
	if (!device) return 0;
	SDL_GPUShader* v = compileShader(device, SDL_GPUShaderStage::SDL_GPU_SHADERSTAGE_VERTEX);
	SDL_GPUShader* f = compileShader(device, SDL_GPUShaderStage::SDL_GPU_SHADERSTAGE_FRAGMENT);
	SDL_GPUGraphicsPipelineCreateInfo pipelinedesc = {};
	SDL_GPUColorTargetDescription color_target_desc = {};
	pipelinedesc.target_info.num_color_targets = 1;
	pipelinedesc.target_info.color_target_descriptions = &color_target_desc;
	pipelinedesc.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
	pipelinedesc.target_info.has_depth_stencil_target = true;

	pipelinedesc.depth_stencil_state.enable_depth_test = true;
	pipelinedesc.depth_stencil_state.enable_depth_write = true;
	pipelinedesc.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

	pipelinedesc.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;

	pipelinedesc.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

	pipelinedesc.vertex_shader = v;
	pipelinedesc.fragment_shader = f;
	SDL_GPUVertexBufferDescription vertex_buffer_desc = {};
	vertex_buffer_desc.slot = 0;
	vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	vertex_buffer_desc.instance_step_rate = 0;
	vertex_buffer_desc.pitch = sizeof(float) * 9;
	SDL_GPUVertexAttribute vertex_attributes[3] = {};
	vertex_attributes[0].buffer_slot = 0;
	vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
	vertex_attributes[0].location = 0;
	vertex_attributes[0].offset = 0;

	vertex_attributes[1].buffer_slot = 0;
	vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertex_attributes[1].location = 1;
	vertex_attributes[1].offset = sizeof(float) * 3;

	vertex_attributes[1].buffer_slot = 0;
	vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertex_attributes[1].location = 2;
	vertex_attributes[1].offset = sizeof(float) * 5;

	pipelinedesc.vertex_input_state.num_vertex_buffers = 1;
	pipelinedesc.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
	pipelinedesc.vertex_input_state.num_vertex_attributes = 2;
	pipelinedesc.vertex_input_state.vertex_attributes = (SDL_GPUVertexAttribute*)&vertex_attributes;

	pipelinedesc.props = 0;

	auto pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelinedesc);
	return device;
}
#endif // 1
// todo vk 
#ifndef NOT_VKH_CPP

#ifndef VKH_USE_VMA
void _set_size_and_bind(VkhDevice pDev, VkBufferUsageFlags usage, VkhMemoryUsage memoryUsage, VkDeviceSize size, VkhBuffer buff) {
	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(pDev->dev, buff->buffer, &memReq);
	VkMemoryAllocateInfo memAllocInfo = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
										  .allocationSize = memReq.size };
	assert(vkh_memory_type_from_properties(&pDev->phyMemProps, memReq.memoryTypeBits, memoryUsage, &memAllocInfo.memoryTypeIndex) == true);
	VK_CHECK_RESULT(vkAllocateMemory(pDev->dev, &memAllocInfo, NULL, &buff->memory));

	buff->alignment = memReq.alignment;
	buff->size = memAllocInfo.allocationSize;
	buff->usageFlags = usage;
	buff->memprops = memoryUsage;

	VK_CHECK_RESULT(vkBindBufferMemory(buff->pDev->dev, buff->buffer, buff->memory, 0));
}
#endif

void vkh_buffer_init(VkhDevice pDev, VkBufferUsageFlags usage, VkhMemoryUsage memprops, VkDeviceSize size, VkhBuffer buff, bool mapped) {
	buff->pDev = pDev;
	VkBufferCreateInfo* pInfo = &buff->infos;
	pInfo->sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	pInfo->usage = usage;
	pInfo->size = size;
	pInfo->sharingMode = VK_SHARING_MODE_EXCLUSIVE;
#ifdef VKH_USE_VMA	
	buff->allocCreateInfo.usage = (VmaMemoryUsage)memprops;
	if (mapped)
		buff->allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	VK_CHECK_RESULT(vmaCreateBuffer(pDev->allocator, pInfo, &buff->allocCreateInfo, &buff->buffer, &buff->alloc, &buff->allocInfo));
#else
	VK_CHECK_RESULT(vkCreateBuffer(pDev->dev, pInfo, NULL, &buff->buffer));
	_set_size_and_bind(pDev, usage, memprops, size, buff);
	buff->memprops = memprops;
	if (mapped)
		VK_CHECK_RESULT(vkMapMemory(buff->pDev->dev, buff->memory, 0, VK_WHOLE_SIZE, 0, &buff->mapped));
#endif
}

VkhBuffer vkh_buffer_create(VkhDevice pDev, VkBufferUsageFlags usage, VkhMemoryUsage memprops, VkDeviceSize size) {
	VkhBuffer buff = (VkhBuffer)calloc(1, sizeof(vkh_buffer_t));
	vkh_buffer_init(pDev, usage, memprops, size, buff, false);
	return buff;
}

void vkh_buffer_reset(VkhBuffer buff) {
	if (buff->buffer)
#ifdef VKH_USE_VMA
		vmaDestroyBuffer(buff->pDev->allocator, buff->buffer, buff->alloc);
#else
		vkDestroyBuffer(buff->pDev->dev, buff->buffer, NULL);
	if (buff->memory)
		vkFreeMemory(buff->pDev->dev, buff->memory, NULL);
#endif
}
void vkh_buffer_destroy(VkhBuffer buff) {
	if (buff->buffer)
#ifdef VKH_USE_VMA
		vmaDestroyBuffer(buff->pDev->allocator, buff->buffer, buff->alloc);
#else
		vkDestroyBuffer(buff->pDev->dev, buff->buffer, NULL);
	if (buff->memory)
		vkFreeMemory(buff->pDev->dev, buff->memory, NULL);
#endif
	free(buff);
	buff = NULL;
}
void vkh_buffer_resize(VkhBuffer buff, VkDeviceSize newSize, bool mapped) {
	vkh_buffer_reset(buff);
	buff->infos.size = newSize;
#ifdef VKH_USE_VMA
	VK_CHECK_RESULT(vmaCreateBuffer(buff->pDev->allocator, &buff->infos, &buff->allocCreateInfo, &buff->buffer, &buff->alloc, &buff->allocInfo));
#else
	VK_CHECK_RESULT(vkCreateBuffer(buff->pDev->dev, &buff->infos, NULL, &buff->buffer));
	_set_size_and_bind(buff->pDev, buff->usageFlags, buff->memprops, buff->infos.size, buff);
	if (mapped)
		VK_CHECK_RESULT(vkMapMemory(buff->pDev->dev, buff->memory, 0, VK_WHOLE_SIZE, 0, &buff->mapped));
#endif
}

VkDescriptorBufferInfo vkh_buffer_get_descriptor(VkhBuffer buff) {
	VkDescriptorBufferInfo desc = {
		.buffer = buff->buffer,
		.offset = 0,
		.range = VK_WHOLE_SIZE };
	return desc;
}


VkResult vkh_buffer_map(VkhBuffer buff) {
#ifdef VKH_USE_VMA
	return vmaMapMemory(buff->pDev->allocator, buff->alloc, &buff->mapped);
#else
	return vkMapMemory(buff->pDev->dev, buff->memory, 0, VK_WHOLE_SIZE, 0, &buff->mapped);
#endif
}
void vkh_buffer_unmap(VkhBuffer buff) {
#ifdef VKH_USE_VMA
	vmaUnmapMemory(buff->pDev->allocator, buff->alloc);
#else
	if (!buff->mapped)
		return;
	vkUnmapMemory(buff->pDev->dev, buff->memory);
	buff->mapped = NULL;
#endif
}
VkBuffer vkh_buffer_get_vkbuffer(VkhBuffer buff) {
	return buff->buffer;
}
void* vkh_buffer_get_mapped_pointer(VkhBuffer buff) {
#ifdef VKH_USE_VMA
	//vmaFlushAllocation (buff->pDev->allocator, buff->alloc, buff->allocInfo.offset, buff->allocInfo.size);
	return buff->allocInfo.pMappedData;
#else
	return buff->mapped;
#endif
}
void vkh_buffer_flush(VkhBuffer buff) {
#ifdef VKH_USE_VMA
	vmaFlushAllocation(buff->pDev->allocator, buff->alloc, buff->allocInfo.offset, buff->allocInfo.size);
#else
#endif
}
#include "vkh_phyinfo.h"

VkhQueue _init_queue(VkhDevice dev) {
	VkhQueue q = (vkh_queue_t*)calloc(1, sizeof(vkh_queue_t));
	q->dev = dev;
	return q;
}


VkhQueue vkh_queue_create(VkhDevice dev, uint32_t familyIndex, uint32_t qIndex) {
	VkhQueue q = _init_queue(dev);
	q->familyIndex = familyIndex;
	vkGetDeviceQueue(dev->dev, familyIndex, qIndex, &q->queue);
	return q;
}
void vkh_queue_destroy(VkhQueue queue) {
	free(queue);
}

VkhPhyInfo vkh_phyinfo_create(VkPhysicalDevice phy, VkSurfaceKHR surface) {
	VkhPhyInfo pi = (vkh_phy_t*)calloc(1, sizeof(vkh_phy_t));
	pi->phy = phy;

	vkGetPhysicalDeviceProperties(phy, &pi->properties);
	vkGetPhysicalDeviceMemoryProperties(phy, &pi->memProps);

	vkGetPhysicalDeviceQueueFamilyProperties(phy, &pi->queueCount, NULL);
	pi->queues = (VkQueueFamilyProperties*)malloc(pi->queueCount * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(phy, &pi->queueCount, pi->queues);

	//identify dedicated queues

	pi->cQueue = -1;
	pi->gQueue = -1;
	pi->tQueue = -1;
	pi->pQueue = -1;

	//try to find dedicated queues first
	for (uint32_t j = 0; j < pi->queueCount; j++) {
		VkBool32 present = VK_FALSE;
		switch (pi->queues[j].queueFlags) {
		case VK_QUEUE_GRAPHICS_BIT:
			if (surface)
				vkGetPhysicalDeviceSurfaceSupportKHR(phy, j, surface, &present);
			if (present) {
				if (pi->pQueue < 0)
					pi->pQueue = j;
			}
			else if (pi->gQueue < 0)
				pi->gQueue = j;
			break;
		case VK_QUEUE_COMPUTE_BIT:
			if (pi->cQueue < 0)
				pi->cQueue = j;
			break;
		case VK_QUEUE_TRANSFER_BIT:
			if (pi->tQueue < 0)
				pi->tQueue = j;
			break;
		}
	}
	//try to find suitable queue if no dedicated one found
	for (uint32_t j = 0; j < pi->queueCount; j++) {
		if (pi->queues[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			VkBool32 present = 0;
			if (surface)
				vkGetPhysicalDeviceSurfaceSupportKHR(phy, j, surface, &present);
			//printf ("surf=%d, q=%d, present=%d\n",surface,j,present);
			if (present) {
				if (pi->pQueue < 0)
					pi->pQueue = j;
			}
			else if (pi->gQueue < 0)
				pi->gQueue = j;
		}
		if ((pi->queues[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (pi->gQueue < 0))
			pi->gQueue = j;
		if ((pi->queues[j].queueFlags & VK_QUEUE_COMPUTE_BIT) && (pi->cQueue < 0))
			pi->cQueue = j;
		if ((pi->queues[j].queueFlags & VK_QUEUE_TRANSFER_BIT) && (pi->tQueue < 0))
			pi->tQueue = j;
	}

	return pi;
}

void vkh_phyinfo_destroy(VkhPhyInfo phy) {
	if (phy->pExtensionProperties != NULL)
		free(phy->pExtensionProperties);
	free(phy->queues);
	free(phy);
}

VkPhysicalDeviceProperties vkh_phyinfo_get_properties(VkhPhyInfo phy) {
	return phy->properties;
}
VkPhysicalDeviceMemoryProperties vkh_phyinfo_get_memory_properties(VkhPhyInfo phy) {
	return phy->memProps;
}

void vkh_phyinfo_get_queue_fam_indices(VkhPhyInfo phy, int* pQueue, int* gQueue, int* tQueue, int* cQueue) {
	if (pQueue)	*pQueue = phy->pQueue;
	if (gQueue)	*gQueue = phy->gQueue;
	if (tQueue)	*tQueue = phy->tQueue;
	if (cQueue)	*cQueue = phy->cQueue;
}
VkQueueFamilyProperties* vkh_phyinfo_get_queues_props(VkhPhyInfo phy, uint32_t* qCount) {
	*qCount = phy->queueCount;
	return phy->queues;
}
bool vkh_phyinfo_create_queues(VkhPhyInfo phy, int qFam, uint32_t queueCount, const float* queue_priorities, VkDeviceQueueCreateInfo* const qInfo) {
	qInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	if (phy->queues[qFam].queueCount < queueCount)
		fprintf(stderr, "Request %d queues of family %d, but only %d available\n", queueCount, qFam, phy->queues[qFam].queueCount);
	else {
		qInfo->queueCount = queueCount,
			qInfo->queueFamilyIndex = qFam,
			qInfo->pQueuePriorities = queue_priorities;
		phy->queues[qFam].queueCount -= queueCount;
		return true;
	}
	return false;
}
bool vkh_phyinfo_create_presentable_queues(VkhPhyInfo phy, uint32_t queueCount, const float* queue_priorities, VkDeviceQueueCreateInfo* const qInfo) {
	qInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	if (phy->pQueue < 0)
		perror("No queue with presentable support found");
	else if (phy->queues[phy->pQueue].queueCount < queueCount)
		fprintf(stderr, "Request %d queues of family %d, but only %d available\n", queueCount, phy->pQueue, phy->queues[phy->pQueue].queueCount);
	else {
		qInfo->queueCount = queueCount,
			qInfo->queueFamilyIndex = phy->pQueue,
			qInfo->pQueuePriorities = queue_priorities;
		phy->queues[phy->pQueue].queueCount -= queueCount;
		return true;
	}
	return false;
}
bool vkh_phyinfo_create_transfer_queues(VkhPhyInfo phy, uint32_t queueCount, const float* queue_priorities, VkDeviceQueueCreateInfo* const qInfo) {
	qInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	if (phy->tQueue < 0)
		perror("No transfer queue found");
	else if (phy->queues[phy->tQueue].queueCount < queueCount)
		fprintf(stderr, "Request %d transfer queues of family %d, but only %d available\n", queueCount, phy->tQueue, phy->queues[phy->tQueue].queueCount);
	else {
		qInfo->queueCount = queueCount;
		qInfo->queueFamilyIndex = phy->tQueue;
		qInfo->pQueuePriorities = queue_priorities;
		phy->queues[phy->tQueue].queueCount -= queueCount;
		return true;
	}
	return false;
}
bool vkh_phyinfo_create_compute_queues(VkhPhyInfo phy, uint32_t queueCount, const float* queue_priorities, VkDeviceQueueCreateInfo* const qInfo) {
	qInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	if (phy->cQueue < 0)
		perror("No compute queue found");
	else if (phy->queues[phy->cQueue].queueCount < queueCount)
		fprintf(stderr, "Request %d compute queues of family %d, but only %d available\n", queueCount, phy->cQueue, phy->queues[phy->cQueue].queueCount);
	else {
		qInfo->queueCount = queueCount,
			qInfo->queueFamilyIndex = phy->cQueue,
			qInfo->pQueuePriorities = queue_priorities;
		phy->queues[phy->cQueue].queueCount -= queueCount;
		return true;
	}
	return false;
}
bool vkh_phy_info_create_graphic_queues(VkhPhyInfo phy, uint32_t queueCount, const float* queue_priorities, VkDeviceQueueCreateInfo* const qInfo) {
	qInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	if (phy->gQueue < 0)
		perror("No graphic queue found");
	else if (phy->queues[phy->gQueue].queueCount < queueCount)
		fprintf(stderr, "Request %d graphic queues of family %d, but only %d available\n", queueCount, phy->gQueue, phy->queues[phy->gQueue].queueCount);
	else {
		qInfo->queueCount = queueCount,
			qInfo->queueFamilyIndex = phy->gQueue,
			qInfo->pQueuePriorities = queue_priorities;
		phy->queues[phy->gQueue].queueCount -= queueCount;
		return true;
	}
	return false;
}
bool vkh_phyinfo_try_get_extension_properties(VkhPhyInfo phy, const char* name, const VkExtensionProperties* properties) {
	if (phy->pExtensionProperties == NULL) {
		VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(phy->phy, NULL, &phy->extensionCount, NULL));
		phy->pExtensionProperties = (VkExtensionProperties*)malloc(phy->extensionCount * sizeof(VkExtensionProperties));
		VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(phy->phy, NULL, &phy->extensionCount, phy->pExtensionProperties));
	}
	for (uint32_t i = 0; i < phy->extensionCount; i++) {
		if (strcmp(name, phy->pExtensionProperties[i].extensionName) == 0) {
			if (properties)
				properties = &phy->pExtensionProperties[i];
			return true;
		}
	}
	properties = NULL;
	return false;
}

VkhImage _vkh_image_create(VkhDevice pDev, VkImageType imageType,
	VkFormat format, uint32_t width, uint32_t height,
	VkhMemoryUsage memprops, VkImageUsageFlags usage,
	VkSampleCountFlagBits samples, VkImageTiling tiling,
	uint32_t mipLevels, uint32_t arrayLayers) {

	VkhImage img = (VkhImage)calloc(1, sizeof(vkh_image_t));

	img->pDev = pDev;

	VkImageCreateInfo* pInfo = &img->infos;
	pInfo->sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	pInfo->imageType = imageType;
	pInfo->tiling = tiling;
	pInfo->initialLayout = (tiling == VK_IMAGE_TILING_OPTIMAL) ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PREINITIALIZED;
	pInfo->sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	pInfo->usage = usage;
	pInfo->format = format;
	pInfo->extent.width = width;
	pInfo->extent.height = height;
	pInfo->extent.depth = 1;
	pInfo->mipLevels = mipLevels;
	pInfo->arrayLayers = arrayLayers;
	pInfo->samples = samples;

	/*
	img->imported = false;
	img->alloc	= VK_NULL_HANDLE;
	img->image	= VK_NULL_HANDLE;
	img->sampler= VK_NULL_HANDLE;
	img->view	= VK_NULL_HANDLE;*/
#ifdef VKH_USE_VMA
	VmaAllocationCreateInfo allocInfo = { .usage = (VmaMemoryUsage)memprops };
	VK_CHECK_RESULT(vmaCreateImage(pDev->allocator, pInfo, &allocInfo, &img->image, &img->alloc, &img->allocInfo));
#else
	VK_CHECK_RESULT(vkCreateImage(pDev->dev, pInfo, NULL, &img->image));
	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(pDev->dev, img->image, &memReq);
	VkMemoryAllocateInfo memAllocInfo = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
										  .allocationSize = memReq.size };
	vkh_memory_type_from_properties(&pDev->phyMemProps, memReq.memoryTypeBits, memprops, &memAllocInfo.memoryTypeIndex);
	VK_CHECK_RESULT(vkAllocateMemory(pDev->dev, &memAllocInfo, NULL, &img->memory));
	VK_CHECK_RESULT(vkBindImageMemory(pDev->dev, img->image, img->memory, 0));
#endif

	mtx_init(&img->mutex, mtx_plain);
	img->references = 1;

	return img;
}
void vkh_image_destroy(VkhImage img)
{
	if (img == NULL)
		return;

	mtx_lock(&img->mutex);
	img->references--;
	if (img->references > 0) {
		mtx_unlock(&img->mutex);
		return;
	}

	mtx_unlock(&img->mutex);
	mtx_destroy(&img->mutex);

	if (img->view != VK_NULL_HANDLE)
		vkDestroyImageView(img->pDev->dev, img->view, NULL);
	if (img->sampler != VK_NULL_HANDLE)
		vkDestroySampler(img->pDev->dev, img->sampler, NULL);

	if (!img->imported) {
#ifdef VKH_USE_VMA
		vmaDestroyImage(img->pDev->allocator, img->image, img->alloc);
#else
		vkDestroyImage(img->pDev->dev, img->image, NULL);
		vkFreeMemory(img->pDev->dev, img->memory, NULL);

#endif
	}


	free(img);
	img = NULL;
}
void vkh_image_reference(VkhImage img) {
	mtx_lock(&img->mutex);
	img->references++;
	mtx_unlock(&img->mutex);
}
VkhImage vkh_tex2d_array_create(VkhDevice pDev,
	VkFormat format, uint32_t width, uint32_t height, uint32_t layers,
	VkhMemoryUsage memprops, VkImageUsageFlags usage) {
	return _vkh_image_create(pDev, VK_IMAGE_TYPE_2D, format, width, height, memprops, usage,
		VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, 1, layers);
}
VkhImage vkh_image_create(VkhDevice pDev,
	VkFormat format, uint32_t width, uint32_t height, VkImageTiling tiling,
	VkhMemoryUsage memprops,
	VkImageUsageFlags usage)
{
	return _vkh_image_create(pDev, VK_IMAGE_TYPE_2D, format, width, height, memprops, usage,
		VK_SAMPLE_COUNT_1_BIT, tiling, 1, 1);
}
//create vkhImage from existing VkImage
VkhImage vkh_image_import(VkhDevice pDev, VkImage vkImg, VkFormat format, uint32_t width, uint32_t height) {
	VkhImage img = (VkhImage)calloc(1, sizeof(vkh_image_t));
	img->pDev = pDev;
	img->image = vkImg;
	img->imported = true;

	VkImageCreateInfo* pInfo = &img->infos;
	pInfo->imageType = VK_IMAGE_TYPE_2D;
	pInfo->format = format;
	pInfo->extent.width = width;
	pInfo->extent.height = height;
	pInfo->extent.depth = 1;
	pInfo->mipLevels = 1;
	pInfo->arrayLayers = 1;
	//pInfo->samples		= samples;
	img->references = 1;

	mtx_init(&img->mutex, mtx_plain);

	return img;
}
VkhImage vkh_image_ms_create(VkhDevice pDev,
	VkFormat format, VkSampleCountFlagBits num_samples, uint32_t width, uint32_t height,
	VkhMemoryUsage memprops,
	VkImageUsageFlags usage) {
	return  _vkh_image_create(pDev, VK_IMAGE_TYPE_2D, format, width, height, memprops, usage,
		num_samples, VK_IMAGE_TILING_OPTIMAL, 1, 1);
}
void vkh_image_create_view(VkhImage img, VkImageViewType viewType, VkImageAspectFlags aspectFlags) {
	if (img->view != VK_NULL_HANDLE)
		vkDestroyImageView(img->pDev->dev, img->view, NULL);

	VkImageViewCreateInfo viewInfo = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
										 .image = img->image,
										 .viewType = viewType,
										 .format = img->infos.format,
										 .components = {VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_G,VK_COMPONENT_SWIZZLE_B,VK_COMPONENT_SWIZZLE_A},
										 .subresourceRange = {aspectFlags,0,1,0,img->infos.arrayLayers} };
	VK_CHECK_RESULT(vkCreateImageView(img->pDev->dev, &viewInfo, NULL, &img->view));
}
void vkh_image_create_sampler(VkhImage img, VkFilter magFilter, VkFilter minFilter,
	VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode) {
	if (img->sampler != VK_NULL_HANDLE)
		vkDestroySampler(img->pDev->dev, img->sampler, NULL);
	VkSamplerCreateInfo samplerCreateInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = magFilter,
		.minFilter = minFilter,
		.mipmapMode = mipmapMode,
		.addressModeU = addressMode,
		.addressModeV = addressMode,
		.addressModeW = addressMode,
		.maxAnisotropy = 1.0,
	};
	VK_CHECK_RESULT(vkCreateSampler(img->pDev->dev, &samplerCreateInfo, NULL, &img->sampler));
}
void vkh_image_set_sampler(VkhImage img, VkSampler sampler) {
	img->sampler = sampler;
}
void vkh_image_create_descriptor(VkhImage img, VkImageViewType viewType, VkImageAspectFlags aspectFlags, VkFilter magFilter,
	VkFilter minFilter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode)
{
	vkh_image_create_view(img, viewType, aspectFlags);
	vkh_image_create_sampler(img, magFilter, minFilter, mipmapMode, addressMode);
}
VkImage vkh_image_get_vkimage(VkhImage img) {
	return img->image;
}
VkSampler vkh_image_get_sampler(VkhImage img) {
	if (img == NULL)
		return NULL;
	return img->sampler;
}
VkImageView vkh_image_get_view(VkhImage img) {
	if (img == NULL)
		return NULL;
	return img->view;
}
VkImageLayout vkh_image_get_layout(VkhImage img) {
	if (img == NULL)
		return VK_IMAGE_LAYOUT_UNDEFINED;
	return img->layout;
}
VkDescriptorImageInfo vkh_image_get_descriptor(VkhImage img, VkImageLayout imageLayout) {
	VkDescriptorImageInfo desc = { .sampler = img->sampler,.imageView = img->view,
								   .imageLayout = imageLayout

	};
	return desc;
}

void vkh_image_set_layout(VkCommandBuffer cmdBuff, VkhImage image, VkImageAspectFlags aspectMask,
	VkImageLayout old_image_layout, VkImageLayout new_image_layout,
	VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages) {
	VkImageSubresourceRange subres = { aspectMask,0,1,0,1 };
	vkh_image_set_layout_subres(cmdBuff, image, subres, old_image_layout, new_image_layout, src_stages, dest_stages);
}
// This method is based on https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.h#L88
void vkh_image_set_layout_subres(VkCommandBuffer cmdBuff, VkhImage image, VkImageSubresourceRange subresourceRange,
	VkImageLayout old_image_layout, VkImageLayout new_image_layout,
	VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages) {
	VkImageMemoryBarrier image_memory_barrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
												  .oldLayout = image->layout,
												  .newLayout = new_image_layout,
												  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												  .image = image->image,
												  .subresourceRange = subresourceRange };

	switch (old_image_layout) {
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;
	default:
		break;
	}

	switch (new_image_layout) {
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	default:
		break;
	}

	vkCmdPipelineBarrier(cmdBuff, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
	image->layout = new_image_layout;
}
void vkh_image_destroy_sampler(VkhImage img) {
	if (img == NULL)
		return;
	if (img->sampler != VK_NULL_HANDLE)
		vkDestroySampler(img->pDev->dev, img->sampler, NULL);
	img->sampler = VK_NULL_HANDLE;
}

void* vkh_image_map(VkhImage img) {
	void* data;
#ifdef VKH_USE_VMA
	vmaMapMemory(img->pDev->allocator, img->alloc, &data);
#else
#endif
	return data;
}
void vkh_image_unmap(VkhImage img) {
#ifdef VKH_USE_VMA
	vmaUnmapMemory(img->pDev->allocator, img->alloc);
#else
#endif
}
void vkh_image_set_name(VkhImage img, const char* name) {
	if (img == NULL)
		return;
	vkh_device_set_object_name(img->pDev, VK_OBJECT_TYPE_IMAGE, (uint64_t)img->image, name);
}
uint64_t vkh_image_get_stride(VkhImage img) {
	VkImageSubresource subres = { VK_IMAGE_ASPECT_COLOR_BIT,0,0 };
	VkSubresourceLayout layout = { 0 };
	vkGetImageSubresourceLayout(img->pDev->dev, img->image, &subres, &layout);
	return (uint64_t)layout.rowPitch;
}

static PFN_vkSetDebugUtilsObjectNameEXT		SetDebugUtilsObjectNameEXT;
static PFN_vkQueueBeginDebugUtilsLabelEXT	QueueBeginDebugUtilsLabelEXT;
static PFN_vkQueueEndDebugUtilsLabelEXT		QueueEndDebugUtilsLabelEXT;
static PFN_vkCmdBeginDebugUtilsLabelEXT		CmdBeginDebugUtilsLabelEXT;
static PFN_vkCmdEndDebugUtilsLabelEXT		CmdEndDebugUtilsLabelEXT;
static PFN_vkCmdInsertDebugUtilsLabelEXT	CmdInsertDebugUtilsLabelEXT;

VkhDevice vkh_device_create(VkhApp app, VkhPhyInfo phyInfo, VkDeviceCreateInfo* pDevice_info) {
	VkDevice dev;
	VK_CHECK_RESULT(vkCreateDevice(phyInfo->phy, pDevice_info, NULL, &dev));
	VkhDevice vkhd = vkh_device_import(app->inst, phyInfo->phy, dev);
	vkhd->vkhApplication = app;
	return vkhd;
}
VkhDevice vkh_device_import(VkInstance inst, VkPhysicalDevice phy, VkDevice vkDev) {
	VkhDevice dev = (vkh_device_t*)calloc(1, sizeof(vkh_device_t));
	dev->dev = vkDev;
	dev->phy = phy;
	dev->instance = inst;

	vkGetPhysicalDeviceMemoryProperties(phy, &dev->phyMemProps);
#ifdef VKH_USE_VMA
	VmaAllocatorCreateInfo allocatorInfo = {
		.physicalDevice = phy,
		.device = vkDev
	};
	vmaCreateAllocator(&allocatorInfo, &dev->allocator);
#else
#endif

	return dev;
}
VkDevice vkh_device_get_vkdev(VkhDevice dev) {
	return dev->dev;
}
VkPhysicalDevice vkh_device_get_phy(VkhDevice dev) {
	return dev->phy;
}
VkhApp vkh_device_get_app(VkhDevice dev) {
	return dev->vkhApplication;
}
/**
 * @brief get instance proc addresses for debug utils (name, label,...)
 * @param vkh device
 */
void vkh_device_init_debug_utils(VkhDevice dev) {
	SetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(dev->instance, "vkSetDebugUtilsObjectNameEXT");
	QueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(dev->instance, "vkQueueBeginDebugUtilsLabelEXT");
	QueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(dev->instance, "vkQueueEndDebugUtilsLabelEXT");
	CmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(dev->instance, "vkCmdBeginDebugUtilsLabelEXT");
	CmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(dev->instance, "vkCmdEndDebugUtilsLabelEXT");
	CmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(dev->instance, "vkCmdInsertDebugUtilsLabelEXT");
}
VkSampler vkh_device_create_sampler(VkhDevice dev, VkFilter magFilter, VkFilter minFilter,
	VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode) {
	VkSampler sampler = VK_NULL_HANDLE;
	VkSamplerCreateInfo samplerCreateInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
											  .magFilter = magFilter,
											  .minFilter = minFilter,
											  .mipmapMode = mipmapMode ,
											  .addressModeU = addressMode,
											  .addressModeV = addressMode,
											  .addressModeW = addressMode,
											  .maxAnisotropy = 1.0 };
	VK_CHECK_RESULT(vkCreateSampler(dev->dev, &samplerCreateInfo, NULL, &sampler));
	return sampler;
}
void vkh_device_destroy_sampler(VkhDevice dev, VkSampler sampler) {
	vkDestroySampler(dev->dev, sampler, NULL);
}
void vkh_device_destroy(VkhDevice dev) {
#ifdef VKH_USE_VMA
	vmaDestroyAllocator(dev->allocator);
#else
#endif
	vkDestroyDevice(dev->dev, NULL);
	free(dev);
}

void vkh_device_set_object_name(VkhDevice dev, VkObjectType objectType, uint64_t handle, const char* name) {
	const VkDebugUtilsObjectNameInfoEXT info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.pNext = 0,
		.objectType = objectType,
		.objectHandle = handle,
		.pObjectName = name
	};
	SetDebugUtilsObjectNameEXT(dev->dev, &info);
}
void vkh_cmd_label_start(VkCommandBuffer cmd, const char* name, const float color[4]) {
	const VkDebugUtilsLabelEXT info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pNext = 0,
		.pLabelName = name
	};
	memcpy((void*)info.color, (void*)color, 4 * sizeof(float));
	CmdBeginDebugUtilsLabelEXT(cmd, &info);
}
void vkh_cmd_label_insert(VkCommandBuffer cmd, const char* name, const float color[4]) {
	const VkDebugUtilsLabelEXT info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pNext = 0,
		.pLabelName = name
	};
	memcpy((void*)info.color, (void*)color, 4 * sizeof(float));
	CmdInsertDebugUtilsLabelEXT(cmd, &info);
}
void vkh_cmd_label_end(VkCommandBuffer cmd) {
	CmdEndDebugUtilsLabelEXT(cmd);
}

#define CHECK_BIT(var,pos) (((var)>>(pos)) & 1)

VkFence vkh_fence_create(VkhDevice dev) {
	VkFence fence;
	VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
									.pNext = NULL,
									.flags = 0 };
	VK_CHECK_RESULT(vkCreateFence(dev->dev, &fenceInfo, NULL, &fence));
	return fence;
}
VkFence vkh_fence_create_signaled(VkhDevice dev) {
	VkFence fence;
	VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
									.pNext = NULL,
									.flags = VK_FENCE_CREATE_SIGNALED_BIT };
	VK_CHECK_RESULT(vkCreateFence(dev->dev, &fenceInfo, NULL, &fence));
	return fence;
}
VkSemaphore vkh_semaphore_create(VkhDevice dev) {
	VkSemaphore semaphore;
	VkSemaphoreCreateInfo info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
								   .pNext = NULL,
								   .flags = 0 };
	VK_CHECK_RESULT(vkCreateSemaphore(dev->dev, &info, NULL, &semaphore));
	return semaphore;
}
VkSemaphore vkh_timeline_create(VkhDevice dev, uint64_t initialValue) {
	VkSemaphore semaphore;
	VkSemaphoreTypeCreateInfo timelineInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, .pNext = NULL,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue = initialValue };
	VkSemaphoreCreateInfo info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
								   .pNext = &timelineInfo,
								   .flags = 0 };
	VK_CHECK_RESULT(vkCreateSemaphore(dev->dev, &info, NULL, &semaphore));
	return semaphore;
}

VkResult vkh_timeline_wait(VkhDevice dev, VkSemaphore timeline, const uint64_t wait) {
	VkSemaphoreWaitInfo waitInfo;
	waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	waitInfo.pNext = NULL;
	waitInfo.flags = 0;
	waitInfo.semaphoreCount = 1;
	waitInfo.pSemaphores = &timeline;
	waitInfo.pValues = &wait;

	return vkWaitSemaphores(dev->dev, &waitInfo, UINT64_MAX);
}
void vkh_cmd_submit_timelined(VkhQueue queue, VkCommandBuffer* pCmdBuff, VkSemaphore timeline, const uint64_t wait, const uint64_t signal) {
	static VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkTimelineSemaphoreSubmitInfo timelineInfo;
	timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	timelineInfo.pNext = NULL;
	timelineInfo.waitSemaphoreValueCount = 1;
	timelineInfo.pWaitSemaphoreValues = &wait;
	timelineInfo.signalSemaphoreValueCount = 1;
	timelineInfo.pSignalSemaphoreValues = &signal;

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = &timelineInfo;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &timeline;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &timeline;
	submitInfo.pWaitDstStageMask = &stageFlags,
		submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = pCmdBuff;

	VK_CHECK_RESULT(vkQueueSubmit(queue->queue, 1, &submitInfo, VK_NULL_HANDLE));
}
void vkh_cmd_submit_timelined2(VkhQueue queue, VkCommandBuffer* pCmdBuff, VkSemaphore timelines[2], const uint64_t waits[2], const uint64_t signals[2]) {
	static VkPipelineStageFlags stageFlags[2] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	VkTimelineSemaphoreSubmitInfo timelineInfo;
	timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	timelineInfo.pNext = NULL;
	timelineInfo.waitSemaphoreValueCount = 2;
	timelineInfo.pWaitSemaphoreValues = waits;
	timelineInfo.signalSemaphoreValueCount = 2;
	timelineInfo.pSignalSemaphoreValues = signals;

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = &timelineInfo;
	submitInfo.waitSemaphoreCount = 2;
	submitInfo.pWaitSemaphores = timelines;
	submitInfo.signalSemaphoreCount = 2;
	submitInfo.pSignalSemaphores = timelines;
	submitInfo.pWaitDstStageMask = stageFlags,
		submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = pCmdBuff;

	VK_CHECK_RESULT(vkQueueSubmit(queue->queue, 1, &submitInfo, VK_NULL_HANDLE));
}
VkEvent vkh_event_create(VkhDevice dev) {
	VkEvent evt;
	VkEventCreateInfo evtInfo = { .sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO };
	VK_CHECK_RESULT(vkCreateEvent(dev->dev, &evtInfo, NULL, &evt));
	return evt;
}
VkCommandPool vkh_cmd_pool_create(VkhDevice dev, uint32_t qFamIndex, VkCommandPoolCreateFlags flags) {
	VkCommandPool cmdPool;
	VkCommandPoolCreateInfo cmd_pool_info = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
											  .pNext = NULL,
											  .flags = flags,
											  .queueFamilyIndex = qFamIndex };
	VK_CHECK_RESULT(vkCreateCommandPool(dev->dev, &cmd_pool_info, NULL, &cmdPool));
	return cmdPool;
}
VkCommandBuffer vkh_cmd_buff_create(VkhDevice dev, VkCommandPool cmdPool, VkCommandBufferLevel level) {
	VkCommandBuffer cmdBuff;
	VkCommandBufferAllocateInfo cmd = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
										.pNext = NULL,
										.commandPool = cmdPool,
										.level = level,
										.commandBufferCount = 1 };
	VK_CHECK_RESULT(vkAllocateCommandBuffers(dev->dev, &cmd, &cmdBuff));
	return cmdBuff;
}
void vkh_cmd_buffs_create(VkhDevice dev, VkCommandPool cmdPool, VkCommandBufferLevel level, uint32_t count, VkCommandBuffer* cmdBuffs) {
	VkCommandBufferAllocateInfo cmd = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
										.pNext = NULL,
										.commandPool = cmdPool,
										.level = level,
										.commandBufferCount = count };
	VK_CHECK_RESULT(vkAllocateCommandBuffers(dev->dev, &cmd, cmdBuffs));
}
void vkh_cmd_begin(VkCommandBuffer cmdBuff, VkCommandBufferUsageFlags flags) {
	VkCommandBufferBeginInfo cmd_buf_info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
											  .pNext = NULL,
											  .flags = flags,
											  .pInheritanceInfo = NULL };

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuff, &cmd_buf_info));
}
void vkh_cmd_end(VkCommandBuffer cmdBuff) {
	VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuff));
}
void vkh_cmd_submit(VkhQueue queue, VkCommandBuffer* pCmdBuff, VkFence fence) {
	VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkSubmitInfo submit_info = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
								 .pWaitDstStageMask = &stageFlags,
								 .commandBufferCount = 1,
								 .pCommandBuffers = pCmdBuff };
	VK_CHECK_RESULT(vkQueueSubmit(queue->queue, 1, &submit_info, fence));
}
void vkh_cmd_submit_with_semaphores(VkhQueue queue, VkCommandBuffer* pCmdBuff, VkSemaphore waitSemaphore,
	VkSemaphore signalSemaphore, VkFence fence) {

	VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkSubmitInfo submit_info = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
								 .pWaitDstStageMask = &stageFlags,
								 .commandBufferCount = 1,
								 .pCommandBuffers = pCmdBuff };

	if (waitSemaphore != VK_NULL_HANDLE) {
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &waitSemaphore;
	}
	if (signalSemaphore != VK_NULL_HANDLE) {
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &signalSemaphore;
	}

	VK_CHECK_RESULT(vkQueueSubmit(queue->queue, 1, &submit_info, fence));
}


void set_image_layout(VkCommandBuffer cmdBuff, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout,
	VkImageLayout new_image_layout, VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages) {
	VkImageSubresourceRange subres = { aspectMask,0,1,0,1 };
	set_image_layout_subres(cmdBuff, image, subres, old_image_layout, new_image_layout, src_stages, dest_stages);
}

void set_image_layout_subres(VkCommandBuffer cmdBuff, VkImage image, VkImageSubresourceRange subresourceRange,
	VkImageLayout old_image_layout, VkImageLayout new_image_layout,
	VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages) {
	VkImageMemoryBarrier image_memory_barrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
												  .oldLayout = old_image_layout,
												  .newLayout = new_image_layout,
												  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												  .image = image,
												  .subresourceRange = subresourceRange };

	switch (old_image_layout) {
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;

	default:
		break;
	}

	switch (new_image_layout) {
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	default:
		break;
	}

	vkCmdPipelineBarrier(cmdBuff, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
}

bool vkh_memory_type_from_properties(VkPhysicalDeviceMemoryProperties* memory_properties, uint32_t typeBits, VkhMemoryUsage memUsage, uint32_t* typeIndex) {
	VkMemoryPropertyFlags memFlags = {};
	switch (memUsage) {
	case VKH_MEMORY_USAGE_GPU_ONLY:
		memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		break;
	case VKH_MEMORY_USAGE_CPU_ONLY:
		memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		break;
	case VKH_MEMORY_USAGE_CPU_TO_GPU:
		memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		break;
	case VKH_MEMORY_USAGE_GPU_TO_CPU:
		memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		break;
	case VKH_MEMORY_USAGE_CPU_COPY:
		memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		break;
	case VKH_MEMORY_USAGE_GPU_LAZILY_ALLOCATED:
		memFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
		break;
	}

	// Search memtypes to find first index with those properties
	for (uint32_t i = 0; i < memory_properties->memoryTypeCount; i++) {
		if (CHECK_BIT(typeBits, i)) {
			// Type is available, does it match user properties?
			if ((memory_properties->memoryTypes[i].propertyFlags & memFlags) == memFlags) {
				*typeIndex = i;
				return true;
			}
		}
	}
	// No memory types matched, return failure
	return false;
}

VkShaderModule vkh_load_module(VkDevice dev, const char* path) {
	VkShaderModule module;
	size_t filelength;
	uint32_t* pCode = (uint32_t*)read_spv(path, &filelength);
	VkShaderModuleCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
											.codeSize = filelength,
											.pCode = pCode };
	VK_CHECK_RESULT(vkCreateShaderModule(dev, &createInfo, NULL, &module));
	free(pCode);
	//assert(module != VK_NULL_HANDLE);
	return module;
}

char* read_spv(const char* filename, size_t* psize) {
	size_t size;
	size_t retval;
	void* shader_code;

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
	filename = [[[NSBundle mainBundle]resourcePath] stringByAppendingPathComponent:@(filename)] .UTF8String;
#endif

	FILE* fp = fopen(filename, "rb");
	if (!fp)
		return NULL;

	fseek(fp, 0L, SEEK_END);
	size = (size_t)ftell(fp);

	fseek(fp, 0L, SEEK_SET);

	shader_code = malloc(size);
	retval = fread(shader_code, size, 1, fp);
	assert(retval == 1);

	*psize = size;

	fclose(fp);
	return (char*)shader_code;
}

// Read file into array of bytes, and cast to uint32_t*, then return.
// The data has been padded, so that it fits into an array uint32_t.
uint32_t* readFile(uint32_t* length, const char* filename) {

	FILE* fp = fopen(filename, "rb");
	if (fp == 0) {
		printf("Could not find or open file: %s\n", filename);
	}

	// get file size.
	fseek(fp, 0, SEEK_END);
	long filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	long filesizepadded = (long)(ceil(filesize / 4.0)) * 4;

	// read file contents.
	char* str = (char*)malloc(filesizepadded * sizeof(char));
	fread(str, filesize, sizeof(char), fp);
	fclose(fp);

	// data padding.
	for (int i = filesize; i < filesizepadded; i++)
		str[i] = 0;

	*length = filesizepadded;
	return (uint32_t*)str;
}

void dumpLayerExts() {
	printf("Layers:\n");
	uint32_t instance_layer_count;
	VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL));
	if (instance_layer_count == 0)
		return;
	VkLayerProperties* vk_props = (VkLayerProperties*)malloc(instance_layer_count * sizeof(VkLayerProperties));
	VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_props));

	for (uint32_t i = 0; i < instance_layer_count; i++) {
		printf("\t%s, %s\n", vk_props[i].layerName, vk_props[i].description);
		/*		  res = init_global_extension_properties(layer_props);
		if (res) return res;
		info.instance_layer_properties.push_back(layer_props);*/
	}
	free(vk_props);
}

static VkExtensionProperties* instExtProps;
static uint32_t instExtCount;
bool vkh_instance_extension_supported(const char* instanceName) {
	for (uint32_t i = 0; i < instExtCount; i++) {
		if (!strcmp(instExtProps[i].extensionName, instanceName))
			return true;
	}
	return false;
}
void vkh_instance_extensions_check_init() {
	VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(NULL, &instExtCount, NULL));
	instExtProps = (VkExtensionProperties*)malloc(instExtCount * sizeof(VkExtensionProperties));
	VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(NULL, &instExtCount, instExtProps));
}
void vkh_instance_extensions_check_release() {
	free(instExtProps);
	instExtCount = 0;
}

static VkLayerProperties* instLayerProps;
static uint32_t instance_layer_count;
bool vkh_layer_is_present(const char* layerName) {
	for (uint32_t i = 0; i < instance_layer_count; i++) {
		if (!strcmp(instLayerProps[i].layerName, layerName))
			return true;
	}
	return false;
}
void vkh_layers_check_init() {
	VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL));
	instLayerProps = (VkLayerProperties*)malloc(instance_layer_count * sizeof(VkLayerProperties));
	VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instance_layer_count, instLayerProps));
}
void vkh_layers_check_release() {
	free(instLayerProps);
	instance_layer_count = 0;
}

#define ENGINE_NAME		"vkhelpers"
#define ENGINE_VERSION	1

VkBool32 debugUtilsMessengerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT			 messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT					 messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	switch (messageSeverity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		printf(KYEL);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		printf(KRED);
		break;
	default:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		printf(KGRN);
		break;
	}
	switch (messageTypes) {
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
		printf("GEN: ");
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
		printf("VAL: ");
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
		printf("PRF: ");
		break;
	}

	printf(KNRM);
	printf("%s\n", pCallbackData->pMessage);


	fflush(stdout);
	return VK_FALSE;
}

VkhApp vkh_app_create(uint32_t version_major, uint32_t version_minor, const char* app_name, uint32_t enabledLayersCount, const char** enabledLayers, uint32_t ext_count, const char* extentions[]) {
	VkhApp app = (VkhApp)malloc(sizeof(vkh_app_t));

	VkApplicationInfo infos = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
								.pApplicationName = app_name,
								.applicationVersion = 1,
								.pEngineName = ENGINE_NAME,
								.engineVersion = ENGINE_VERSION,
#ifdef VK_MAKE_API_VERSION
								.apiVersion = VK_MAKE_API_VERSION(0, version_major, version_minor, 0) };
#else
		.apiVersion = VK_MAKE_VERSION(version_major, version_minor, 0)
};
#endif

	VkInstanceCreateInfo inst_info = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
									   .pApplicationInfo = &infos,
									   .enabledLayerCount = enabledLayersCount,
									   .ppEnabledLayerNames = enabledLayers,
									   .enabledExtensionCount = ext_count,
									   .ppEnabledExtensionNames = extentions };

	VK_CHECK_RESULT(vkCreateInstance(&inst_info, NULL, &app->inst));
	app->infos = infos;
	app->debugMessenger = VK_NULL_HANDLE;
	return app;
}

void vkh_app_destroy(VkhApp app) {
	if (app->debugMessenger != VK_NULL_HANDLE) {
		PFN_vkDestroyDebugUtilsMessengerEXT	 DestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(app->inst, "vkDestroyDebugUtilsMessengerEXT");

		DestroyDebugUtilsMessenger(app->inst, app->debugMessenger, VK_NULL_HANDLE);
	}
	vkDestroyInstance(app->inst, NULL);
	free(app);
}

VkInstance vkh_app_get_inst(VkhApp app) {
	return app->inst;
}

VkhPhyInfo* vkh_app_get_phyinfos(VkhApp app, uint32_t* count, VkSurfaceKHR surface) {
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(app->inst, count, NULL));
	VkPhysicalDevice* phyDevices = (VkPhysicalDevice*)malloc((*count) * sizeof(VkPhysicalDevice));
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(app->inst, count, phyDevices));
	VkhPhyInfo* infos = (VkhPhyInfo*)malloc((*count) * sizeof(VkhPhyInfo));

	for (uint32_t i = 0; i < (*count); i++)
		infos[i] = vkh_phyinfo_create(phyDevices[i], surface);

	free(phyDevices);
	return infos;
}

void vkh_app_free_phyinfos(uint32_t count, VkhPhyInfo* infos) {
	for (uint32_t i = 0; i < count; i++)
		vkh_phyinfo_destroy(infos[i]);
	free(infos);
}
/**
 * @brief Add a Debug utils messenger to this  VkhApp. It will be destroyed on VkhApp end.
 * @param VKH application pointer containing vkInstance.
 * @param Message type flags
 * @param Message severity flags.
 * @param optional message callback, if null a default one which print to stdout is configured.
 */
void vkh_app_enable_debug_messenger(VkhApp app,
	VkDebugUtilsMessageTypeFlagsEXT typeFlags,
	VkDebugUtilsMessageSeverityFlagsEXT severityFlags,
	PFN_vkDebugUtilsMessengerCallbackEXT callback) {

	VkDebugUtilsMessengerCreateInfoEXT info = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
												.pNext = VK_NULL_HANDLE,
												.flags = 0,
												.messageSeverity = severityFlags,
												.messageType = typeFlags,
												.pUserData = NULL };
	if (callback == NULL)
		info.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)debugUtilsMessengerCallback;
	else
		info.pfnUserCallback = callback;

	PFN_vkCreateDebugUtilsMessengerEXT	CreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(app->inst, "vkCreateDebugUtilsMessengerEXT");

	CreateDebugUtilsMessenger(app->inst, &info, VK_NULL_HANDLE, &app->debugMessenger);
}
vg_fbo_t new_vgfbo(ovg_ctx_t* p, int width, int height) {
	vg_fbo_t fbo = {};
	fbo.width = width; fbo.height = height;
	if (p) {
		ovg_new_fbo(p, (vg_fbo_t0*)&fbo);
	}
	return fbo;
}
void free_vgfbo(vg_fbo_t* p) {
	if (p) {
		ovg_free_fbo((vg_fbo_t0*)p);
	}
}
#endif // 
