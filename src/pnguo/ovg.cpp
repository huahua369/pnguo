/*

2026/8/13 版本1.0
2026/8/8 创建文件

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

#if (__has_include(<ovg.h>))
#include <ovg.h>
#else
#include "../ovg.h"
#endif

#include <array>
#include <map>
#include <vector>
#include <stack>
#include <memory_resource>

void init_ovg_cb(ovg_canvas_cb* cb);
void init_ovg_ctx_cb(ovg_ctx_cb* cb);

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
	void* ptr = 0;
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

ovg_canvas_cb* new_canvas_cb()
{
	auto p = new ovg_canvas_cb();
	auto ac = new usp_ac_cx();
	p->ac = (mem_resource_t*)ac;
	init_ovg_cb(p);
	return p;
}
void free_canvas_cb(ovg_canvas_cb* p) {
	if (p) {
		if (p->ac)delete p->ac;
		delete p;
	}
}
class ovg_ctx_cx :public ovg_ctx_cb
{
public:

public:
	ovg_ctx_cx();
	~ovg_ctx_cx();

private:

};

ovg_ctx_cx::ovg_ctx_cx()
{
	auto ac0 = new usp_ac_cx();
	ac = (mem_resource_t*)ac0;
	init_ovg_ctx_cb(this);
	ac0->ptr = this;
}

ovg_ctx_cx::~ovg_ctx_cx()
{
	auto ac1 = (usp_ac_cx*)ac;
	if (ac1)
		delete ac1;
	ac = 0;
}
ovg_ctx_cb* new_ctx_cb()
{
	auto p = new ovg_ctx_cx();
	return p;
}
void free_ctx_cb(ovg_ctx_cb* p) {
	auto p1 = (ovg_ctx_cx*)p;
	if (p) {
		delete p1;
	}
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
	if (path->t)
		_matrix_get_scale(&path->t->pushConsts.mat, &sx, &sy);
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
	pat->matrix = glm::mat3x2(1.0);
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
	pat->matrix = glm::mat3x2(1.0);
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
	pat->type = vg_pattern_type_t::VG_PATTERN_TYPE_SWEEP;
	pat->extend = vg_extend_t::VG_EXTEND_NONE;
	pat->data = &pat->g;
	pat->matrix = glm::mat3x2(1.0);
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
		*p = {};
		push_constants_t pc = {};
		pc.source.w = 1;
		pc.size = { (float)100, (float)100 };
		pc.fsq_patternType = VG_PATTERN_TYPE_SOLID;
		pc.opacity = 1.0f;
		pc.mat = pc.matInv = glm::mat3x2(1.0);
		p->lineWidth = 1.f;
		p->miterLimit = 10.f;
		p->curOperator = VG_OPERATOR_OVER;
		p->curFillRule = VG_FILL_RULE_NON_ZERO;
		p->pushConsts = pc;
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
	std::stack<vg_state_save_t*> _cst;	// 保存栈 
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

	void save();
	void restore();
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

	vg_state_save_t* new_ss(vg_state_save_t* src);
	vg_state_save_t* new_state();
	void free_state(vg_state_save_t* p);
	void swap_state(vg_state_save_t* p, vg_state_save_t* p1);
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
	ovg_clear_path(cur_path);
	_curVertOffset = 0;
	gCount = 0;
	mac.release();
	_vertex.clear();
	_indices.clear();
	cmdlist.clear();
	while (_cst.size())
	{
		auto c = _cst.top();
		free_state(c);
		_cst.pop();
	}
	free_state(cur_st);
	cur_st = new_state();
}
void rvg_t::set_path(ovg_path_t* path, vg_state_save_t* st)
{
	cur_path = path;
	cur_path->t = st;
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

		if (p->t->dashCount > 0) {
			// init dash stroke
			dc.dashOn = true;
			dc.curDash = 0; // current dash index
			dc.totDashLength = 0; // limit offset to total length of dashes
			for (uint32_t i = 0; i < p->t->dashCount; i++)
				dc.totDashLength += p->t->dashes[i];
			if (dc.totDashLength == 0) {
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
	auto t = p->t;
	uint32_t color = t->color;
	p->color = color;
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


vg_state_save_t* rvg_t::new_ss(vg_state_save_t* src)
{
	vg_state_save_t* dst = new_state();
	if (!dst)return dst;
	if (src)
	{
		*dst = *src;
		if (src->dashes && src->dashCount > 0) {
			ac->free_mem(dst->dashes, src->dashCount);
			dst->dashes = (float*)ac->allocate(sizeof(float) * src->dashCount);
			if (dst->dashes)
				memcpy(dst->dashes, src->dashes, sizeof(float) * src->dashCount);
			else
				dst->dashCount = 0;
		}
	}
	else
		*dst = {};
	return dst;
}
vg_state_save_t* rvg_t::new_state()
{
	auto t = (vg_state_save_t*)ac->allocate(sizeof(vg_state_save_t));
	*t = {};
	t->color = -1;
	//VkRect2D b = {};
	//b.extent = { ctx->pSurf->width, ctx->pSurf->height };
	push_constants_t pc = {};
	pc.source.w = 1;
	pc.size = { (float)100, (float)100 };
	pc.fsq_patternType = VG_PATTERN_TYPE_SOLID;
	pc.opacity = 1.0f;
	pc.mat = pc.matInv = glm::mat3x2(1.0);

	t->lineWidth = 1.f;
	t->miterLimit = 10.f;
	t->curOperator = VG_OPERATOR_OVER;
	t->curFillRule = VG_FILL_RULE_NON_ZERO;
	t->aa = false;
	//t->bounds = b;
	t->pushConsts = pc;
	return t;
}
void rvg_t::free_state(vg_state_save_t* p)
{
	if (p) {
		if (p->dashes && p->dashCount > 0)
			ac->free_mem(p->dashes, p->dashCount);
		ac->free_mem(p, 1);
	}
}
void rvg_t::swap_state(vg_state_save_t* p, vg_state_save_t* p1)
{
	std::swap(*p, *p1);
}
void rvg_t::save()
{
	auto ss = new_ss(cur_st);
	_cst.push(ss);
}
void rvg_t::restore()
{
	auto c = _cst.top();
	swap_state(cur_st, c);
	free_state(c);
	_cst.pop();
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

	cp_cmdt(&c, ctx->t);
	ctx->curVertOffset = _vertex.size();
	c.vertex.x = _vertex.size();
#if 1
	while (ptrPath < ctx->pathPtr) {
		uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;
		if (pathPointCount > 2) {
			uint32_t firstVertIdx = (uint32_t)_vertex.size();


			// ---- 1. 先收集局部坐标 + 算 bounds（避免展开时重复 transform）----
			std::vector<glm::vec2> polyPoints(pathPointCount);
			for (uint32_t i = 0; i < pathPointCount; i++) {
				glm::vec2 localPos = ctx->points[i + firstPtIdx];
				polyPoints[i] = localPos;

				if (bounds) {
					glm::vec2 transformedPos = localPos;
					matrix_transform_point(&c.state->pushConsts.mat,
						&transformedPos.x, &transformedPos.y);
					if (transformedPos.x < bounds->x) bounds->x = transformedPos.x;
					if (transformedPos.x > bounds->z) bounds->z = transformedPos.x;
					if (transformedPos.y < bounds->y) bounds->y = transformedPos.y;
					if (transformedPos.y > bounds->w) bounds->w = transformedPos.y;
				}
			}

			// ---- 2. 展开为 TRIANGLE_LIST ----
			// 原 FAN 语义: v0 是共享顶点，三角形为 (v0, v_i, v_{i+1})
			for (uint32_t i = 1; i < pathPointCount - 1; i++) {
				// 三角形 (v0, v_i, v_{i+1})
				v.pos = polyPoints[0];
				_vertex.push_back(v);

				v.pos = polyPoints[i];
				_vertex.push_back(v);

				v.pos = polyPoints[i + 1];
				_vertex.push_back(v);
			}
			c.vertex.y += (pathPointCount - 2) * 3;  // ← 关键改动

		}
		firstPtIdx += pathPointCount;

		// 跳过曲线数据（和原来一样）
		if (o_path_has_curves(ctx->pathes.data(), ptrPath)) {
			ptrPath++;
			uint32_t totPts = 0;
			while (totPts < pathPointCount)
				totPts += (ctx->pathes[ptrPath++] & PATH_ELT_MASK);
		}
		else {
			ptrPath++;
		}
	}
#else
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
#endif
}

#if (__has_include(<glutess.h>))
#ifdef NOT_FILL_NZ_GLUTESS
#undef NOT_FILL_NZ_GLUTESS
#endif
#else
#define NOT_FILL_NZ_GLUTESS
#endif

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
	p->ac = ac;
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
void  ovg_add_geometry(rvg_t* dc, vg_surface_t* texture, const float* xy, int xy_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type)
{
	if (dc)dc->gps.add_geometry(texture, xy, xy_stride, color, color_stride, uv, uv_stride, num_vertices, indices, num_indices, size_indices, color_type);
}
// 添加3D几何数据到缓冲区，xyz顶点坐标，color顶点颜色（双面则要双倍），uv顶点纹理坐标，indices索引数据
void  ovg_add_geometry3d(rvg_t* dc, vg_surface_t* texture, const float* xyz, int xyz_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type)
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
	float scale_x = 1.0, scale_y = 1.0;
	float u_scale = 1.0, v_scale = 1.0;
	size_indices = indices ? size_indices : 0;
	ids.reserve(ids.size() + num_indices);
	c.firstIndex = ids.size();
	c.elemCount = num_indices;
	if (num_indices < 1 || size_indices < 1)
	{
		c.elemCount = num_vertices;
	}
	if (curState.shader == ST_DOUBLESIDED) {
		c.vertexOffset = vd2.size();
		c.v_offset = 1;
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
	float scale_x = 1.0, scale_y = 1.0, scale_z = 1.0;
	float u_scale = 1.0, v_scale = 1.0;
	size_indices = indices ? size_indices : 0;
	ids.reserve(ids.size() + num_indices);
	c.firstIndex = ids.size();
	c.elemCount = num_indices;
	if (num_indices < 1 || size_indices < 1)
	{
		c.elemCount = num_vertices;
	}
	if (curState.shader == ST_DOUBLESIDED) {
		c.vertexOffset = vd2.size();
		c.v_offset = 1;
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
	info.flags = (uint8_t)depth_stencil_State::d_stenciltest_enable;
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

ovg_draw_data get_draw_list(rvg_t* p)
{
	ovg_draw_data r = {};
	if (p)
	{
		r.d = p->cmdlist.data(); r.count = p->cmdlist.size();
		r.vg_vertex = (ovgVertex*)p->_vertex.data();
		r.v_count = p->_vertex.size();
		r.vg_indices = p->_indices.data();
		r.i_count = p->_indices.size();
		r.uboCount = p->gCount;
		r.vertex1 = (geomVertex1*)p->gps.vd1.data();
		r.v1_count = p->gps.vd1.size();
		r.vertex2 = (geomVertex2*)p->gps.vd2.data();
		r.v2_count = p->gps.vd2.size();
		r.geom_indices = p->gps.ids.data();
		r.g_count = p->gps.ids.size();
	}
	return r;
}

// cmd ctx
#if 1

rvg_t* vctx_new_rvg(mem_resource_t* ac);
void  vctx_destroy_rvg(rvg_t* p);
void vctx_clear(rvg_t* v);			// 清空画布 
// 路径操作
ovg_path_t* vctx_get_path(rvg_t* ctx);
void  vctx_new_path(rvg_t* ctx);
void vctx_clear_path(rvg_t* ctx);
void vctx_close_path(rvg_t* ctx);
void vctx_new_sub_path(rvg_t* ctx);
void vctx_path_extents(rvg_t* ctx, float* x1, float* y1, float* x2, float* y2);
void vctx_get_current_point(rvg_t* ctx, float* x, float* y);
size_t vctx_get_segment_count(rvg_t* ctx);
void vctx_set_segment_color(rvg_t* ctx, size_t idx, uint32_t color);
// 添加数据到当前路径，参考path_type_e
void vctx_add_path(rvg_t* ctx, float* data, size_t count);
void vctx_move_to(rvg_t* ctx, float x, float y);
void vctx_rel_move_to(rvg_t* ctx, float x, float y);
void vctx_line_to(rvg_t* ctx, float x, float y);
void vctx_rel_line_to(rvg_t* ctx, float dx, float dy);
void vctx_arc(rvg_t* ctx, float xc, float yc, float radius, float a1, float a2);
void vctx_arc_negative(rvg_t* ctx, float xc, float yc, float radius, float a1, float a2);
// 有缩放时，先执行set_path一次再执行curve_to
void vctx_curve_to(rvg_t* ctx, float x1, float y1, float x2, float y2, float x3, float y3);
void vctx_rel_curve_to(rvg_t* ctx, float x1, float y1, float x2, float y2, float x3, float y3);
void vctx_quadratic_to(rvg_t* ctx, float x1, float y1, float x2, float y2);
void vctx_rel_quadratic_to(rvg_t* ctx, float x1, float y1, float x2, float y2);
void vctx_rectangle(rvg_t* ctx, float x, float y, float w, float h);
void vctx_rounded_rectangle(rvg_t* ctx, float x, float y, float w, float h, float radius);
void vctx_rounded_rectangle2(rvg_t* ctx, float x, float y, float w, float h, float rx, float ry);
void vctx_ellipse(rvg_t* ctx, float radiusX, float radiusY, float x, float y, float rotationAngle);
void vctx_elliptic_arc_to(rvg_t* ctx, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);
void vctx_rel_elliptic_arc_to(rvg_t* ctx, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);
void vctx_circle(rvg_t* ctx, float x, float y, float radius);
// 配置
void vctx_set_opacity(rvg_t* ctx, float opacity);
void vctx_set_source_color(rvg_t* ctx, uint32_t c);
void vctx_set_source_rgba(rvg_t* ctx, float r, float g, float b, float a);
void vctx_set_source_rgb(rvg_t* ctx, float r, float g, float b);
void vctx_set_line_width(rvg_t* ctx, float width);
void vctx_set_miter_limit(rvg_t* ctx, float limit);
void vctx_set_line_cap(rvg_t* ctx, int cap);
void vctx_set_line_join(rvg_t* ctx, int join);
void vctx_set_source_surface(rvg_t* ctx, vg_surface_t* surf, float x, float y);
void vctx_set_source(rvg_t* ctx, vg_pattern_t* pat);
void vctx_set_operator(rvg_t* ctx, int op);
void vctx_set_fill_rule(rvg_t* ctx, int fr);
void vctx_set_dash(rvg_t* ctx, const float* dashes, uint32_t num_dashes, float offset);		// 虚线
void vctx_set_dash8(rvg_t* ctx, uint64_t dashes, uint32_t num_dashes, float offset);								// 虚线,用uint8_t v8[8]表示
void vctx_translate(rvg_t* ctx, float dx, float dy);
void vctx_scale(rvg_t* ctx, float sx, float sy);
void vctx_rotate(rvg_t* ctx, float radians);
void vctx_transform(rvg_t* ctx, const void* matrix);
void vctx_set_matrix(rvg_t* ctx, const void* matrix);
void vctx_get_matrix(rvg_t* ctx, void* matrix);
void vctx_identity_matrix(rvg_t* ctx);

// 图案：渐变/图片 
vg_pattern_t* vctx_new_pattern_linear(rvg_t* ctx, float x0, float y0, float x1, float y1);
vg_pattern_t* vctx_new_pattern_radial(rvg_t* ctx, float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse);
vg_pattern_t* vctx_new_pattern_sweep(rvg_t* ctx, float cx, float cy, float start_angle, float end_angle);
int  vctx_pattern_add_color_stop(vg_pattern_t* pat, float o, float r, float g, float b, float a);
int  vctx_pattern_set_color_stop(vg_pattern_t* pat, int idx, float o, float r, float g, float b, float a);
void vctx_pattern_set_matrix(vg_pattern_t* pat, const void* matrix);	// mat3x2
void vctx_pattern_set_extend(vg_pattern_t* pat, int extend);
void vctx_pattern_set_filter(vg_pattern_t* pat, int filter);

void vctx_save(rvg_t* v);
void vctx_restore(rvg_t* v);
void vctx_stroke(rvg_t* v);
void vctx_stroke_preserve(rvg_t* v);
void vctx_fill(rvg_t* v);
void vctx_fill_preserve(rvg_t* v);
void vctx_paint(rvg_t* v);			// 全屏渲染
void vctx_reset_clip(rvg_t* v);	// 重置裁剪
void vctx_clip(rvg_t* v);			// 路径裁剪，清空当前路径
void vctx_clip_preserve(rvg_t* v);	// 路径裁剪
void vctx_clip_rect(rvg_t* v, int x, int y, int width, int height);	// 矩形裁剪
void vctx_set_clip_rect(rvg_t* v, void* rc);	// 矩形裁剪,int[4]
void vctx_get_clip_rect(rvg_t* v, void* rc);	// 获取矩形裁剪

// 添加文本，风格，渲染区可选
void  vctx_add_text(rvg_t* dc, text_st_t* p, text_style_t* ts, text_box_rt* box);
// 普通图片，支持九宫格、混合颜色
void  vctx_add_image(rvg_t* dc, ovg_image_r* r);
// 原始三角形，输入0则不修改
void  vctx_set_geom_state(rvg_t* dc, gem_info_t* info, const void* matrix4x4);
// 添加几何数据到缓冲区，xy顶点坐标，color顶点颜色，uv顶点纹理坐标，indices索引数据，color_type=0表示float4，1表示uint32_t
void  vctx_add_geometry(rvg_t* dc, vg_surface_t* texture, const float* xy, int xy_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type);
// 添加3D几何数据到缓冲区，xyz顶点坐标，color顶点颜色（双面则要双倍），uv顶点纹理坐标，indices索引数据
void  vctx_add_geometry3d(rvg_t* dc, vg_surface_t* texture, const float* xyz, int xyz_stride, const void* color, int color_stride, const float* uv, int uv_stride, int num_vertices, const void* indices, int num_indices, int size_indices, int color_type);

void vctx_start_recording(rvg_t* ctx);
ovg_recording_t* vctx_stop_recording(rvg_t* ctx);
void vctx_replay(rvg_t* ctx, ovg_recording_t* rec);
void vctx_replay_command(rvg_t* ctx, ovg_recording_t* rec, uint32_t cmdIndex);
uint32_t vctx_recording_get_count(ovg_recording_t* rec);
void* vctx_recording_get_data(ovg_recording_t* rec);
void  vctx_recording_destroy(ovg_recording_t* rec);

// TODO 命令模式实现
#define PRI2CTX(ac) 
rvg_t* vctx_new_rvg(mem_resource_t* ac) {
	auto r = ovg_new_rvg(ac);
	if (r)
	{
		r->cur_path = ovg_new_path(ac);
		r->cur_st = r->new_state();
		assert(r->cur_path && r->cur_st);
	}
	return r;
}
void  vctx_destroy_rvg(rvg_t* p) {
	ovg_destroy_rvg(p);
}
void vctx_clear(rvg_t* v) {
	ovg_clear(v);
}
ovg_path_t* vctx_get_path(rvg_t* ctx) {
	return ctx->cur_path;
}
void  vctx_new_path(rvg_t* ctx) {
	if (ctx)
		ovg_clear_path(ctx->cur_path);
}
void vctx_clear_path(rvg_t* ctx) {
	vctx_new_path(ctx);
}
void vctx_close_path(rvg_t* ctx) {
	if (ctx)ovg_close_path(ctx->cur_path);
}
void vctx_new_sub_path(rvg_t* ctx) {
	if (ctx)ovg_new_sub_path(ctx->cur_path);
}
void vctx_path_extents(rvg_t* ctx, float* x1, float* y1, float* x2, float* y2) {
	if (ctx)ovg_path_extents(ctx->cur_path, x1, y1, x2, y2);
}
void vctx_get_current_point(rvg_t* ctx, float* x, float* y) {
	if (ctx)ovg_get_current_point(ctx->cur_path, x, y);
}
size_t vctx_get_segment_count(rvg_t* ctx) {
	return ctx ? ovg_get_segment_count(ctx->cur_path) : 0;
}
void vctx_set_segment_color(rvg_t* ctx, size_t idx, uint32_t color) {
	if (ctx)ovg_set_segment_color(ctx->cur_path, idx, color);
}
// 添加数据到当前路径，参考path_type_e
void vctx_add_path(rvg_t* ctx, float* data, size_t count) {
	if (ctx)ovg_add_path(ctx->cur_path, data, count);
}
void vctx_move_to(rvg_t* ctx, float x, float y) {
	if (ctx)ovg_move_to(ctx->cur_path, x, y);
}
void vctx_rel_move_to(rvg_t* ctx, float x, float y) {
	if (!ctx) return;
	float cx, cy;
	ovg_get_current_point(ctx->cur_path, &cx, &cy);
	ovg_move_to(ctx->cur_path, cx + x, cy + y);
}

void vctx_line_to(rvg_t* ctx, float x, float y) {
	if (ctx) ovg_line_to(ctx->cur_path, x, y);
}

void vctx_rel_line_to(rvg_t* ctx, float dx, float dy) {
	if (!ctx) return;
	float cx, cy;
	ovg_get_current_point(ctx->cur_path, &cx, &cy);
	ovg_line_to(ctx->cur_path, cx + dx, cy + dy);
}

void vctx_arc(rvg_t* ctx, float xc, float yc, float radius, float a1, float a2) {
	if (ctx) ovg_arc(ctx->cur_path, xc, yc, radius, a1, a2);
}

void vctx_arc_negative(rvg_t* ctx, float xc, float yc, float radius, float a1, float a2) {
	if (ctx) ovg_arc_negative(ctx->cur_path, xc, yc, radius, a1, a2);
}

void vctx_curve_to(rvg_t* ctx, float x1, float y1,
	float x2, float y2,
	float x3, float y3) {
	if (ctx) ovg_curve_to(ctx->cur_path, x1, y1, x2, y2, x3, y3);
}

void vctx_rel_curve_to(rvg_t* ctx, float x1, float y1,
	float x2, float y2,
	float x3, float y3) {
	if (!ctx) return;
	float cx, cy;
	ovg_get_current_point(ctx->cur_path, &cx, &cy);
	ovg_curve_to(ctx->cur_path,
		cx + x1, cy + y1,
		cx + x2, cy + y2,
		cx + x3, cy + y3);
}

void vctx_quadratic_to(rvg_t* ctx, float x1, float y1, float x2, float y2) {
	if (ctx) ovg_quadratic_to(ctx->cur_path, x1, y1, x2, y2);
}

void vctx_rel_quadratic_to(rvg_t* ctx, float x1, float y1, float x2, float y2) {
	if (!ctx) return;
	float cx, cy;
	ovg_get_current_point(ctx->cur_path, &cx, &cy);
	ovg_quadratic_to(ctx->cur_path,
		cx + x1, cy + y1,
		cx + x2, cy + y2);
}

void vctx_rectangle(rvg_t* ctx, float x, float y, float w, float h) {
	if (ctx) ovg_rectangle(ctx->cur_path, x, y, w, h);
}

void vctx_rounded_rectangle(rvg_t* ctx, float x, float y,
	float w, float h, float radius) {
	if (ctx) ovg_rounded_rectangle(ctx->cur_path, x, y, w, h, radius);
}

void vctx_rounded_rectangle2(rvg_t* ctx, float x, float y,
	float w, float h, float rx, float ry) {
	if (ctx) ovg_rounded_rectangle2(ctx->cur_path, x, y, w, h, rx, ry);
}

void vctx_ellipse(rvg_t* ctx, float radiusX, float radiusY,
	float x, float y, float rotationAngle) {
	if (ctx) ovg_ellipse(ctx->cur_path, radiusX, radiusY, x, y, rotationAngle);
}

void vctx_elliptic_arc_to(rvg_t* ctx, float x, float y,
	bool large_arc_flag, bool sweep_flag,
	float rx, float ry, float phi) {
	if (ctx) ovg_elliptic_arc_to(ctx->cur_path, x, y,
		large_arc_flag, sweep_flag,
		rx, ry, phi);
}

void vctx_rel_elliptic_arc_to(rvg_t* ctx, float x, float y,
	bool large_arc_flag, bool sweep_flag,
	float rx, float ry, float phi) {
	if (!ctx) return;
	float cx, cy;
	ovg_get_current_point(ctx->cur_path, &cx, &cy);
	ovg_elliptic_arc_to(ctx->cur_path,
		cx + x, cy + y,
		large_arc_flag, sweep_flag,
		rx, ry, phi);
}

void vctx_circle(rvg_t* ctx, float x, float y, float radius) {
	if (ctx) ovg_circle(ctx->cur_path, x, y, radius);
}
// 配置 
void vctx_set_opacity(rvg_t* ctx, float opacity) {
	if (ctx) ovg_set_opacity(ctx->cur_st, opacity);
}

void vctx_set_source_color(rvg_t* ctx, uint32_t c) {
	if (ctx) ovg_set_source_color(ctx->cur_st, c);
}

void vctx_set_source_rgba(rvg_t* ctx, float r, float g, float b, float a) {
	if (ctx) ovg_set_source_rgba(ctx->cur_st, r, g, b, a);
}

void vctx_set_source_rgb(rvg_t* ctx, float r, float g, float b) {
	if (ctx) ovg_set_source_rgba(ctx->cur_st, r, g, b, 1.0f);
}

void vctx_set_line_width(rvg_t* ctx, float width) {
	if (ctx) ovg_set_line_width(ctx->cur_st, width);
}

void vctx_set_miter_limit(rvg_t* ctx, float limit) {
	if (ctx) ovg_set_miter_limit(ctx->cur_st, limit);
}

void vctx_set_line_cap(rvg_t* ctx, int cap) {
	if (ctx) ovg_set_line_cap(ctx->cur_st, cap);
}

void vctx_set_line_join(rvg_t* ctx, int join) {
	if (ctx) ovg_set_line_join(ctx->cur_st, join);
}

void vctx_set_source_surface(rvg_t* ctx, vg_surface_t* surf, float x, float y) {
	if (ctx) ovg_set_source_surface(ctx->cur_st, surf, x, y);
}

void vctx_set_source(rvg_t* ctx, vg_pattern_t* pat) {
	if (ctx) ovg_set_source(ctx->cur_st, pat);
}

void vctx_set_operator(rvg_t* ctx, int op) {
	if (ctx) ovg_set_operator(ctx->cur_st, op);
}

void vctx_set_fill_rule(rvg_t* ctx, int fr) {
	if (ctx) ovg_set_fill_rule(ctx->cur_st, fr);
}

void vctx_set_dash(rvg_t* ctx, const float* dashes, uint32_t num_dashes, float offset) {
	if (ctx) ovg_set_dash(ctx->cur_st, dashes, num_dashes, offset);
}

void vctx_set_dash8(rvg_t* ctx, uint64_t dashes, uint32_t num_dashes, float offset) {
	if (ctx) ovg_set_dash8(ctx->cur_st, dashes, num_dashes, offset);
}

void vctx_translate(rvg_t* ctx, float dx, float dy) {
	if (ctx) ovg_translate(ctx->cur_st, dx, dy);
}

void vctx_scale(rvg_t* ctx, float sx, float sy) {
	if (ctx) ovg_scale(ctx->cur_st, sx, sy);
}

void vctx_rotate(rvg_t* ctx, float radians) {
	if (ctx) ovg_rotate(ctx->cur_st, radians);
}

void vctx_transform(rvg_t* ctx, const void* matrix) {
	if (ctx) ovg_transform(ctx->cur_st, matrix);
}

void vctx_set_matrix(rvg_t* ctx, const void* matrix) {
	if (ctx) ovg_set_matrix(ctx->cur_st, matrix);
}

void vctx_get_matrix(rvg_t* ctx, void* matrix) {
	if (ctx) ovg_get_matrix(ctx->cur_st, matrix);
}

void vctx_identity_matrix(rvg_t* ctx) {
	if (ctx) ovg_identity_matrix(ctx->cur_st);
}

typedef glm::mat3x2 ovg_matrix_t;
struct pat_act0 :public  vg_pattern_t {
	vg_gradient_t g = {};
};
// 图案：渐变/图片 
vg_pattern_t* vctx_new_pattern_linear(rvg_t* ctx, float x0, float y0, float x1, float y1) {
	if (!ctx)return 0;
	auto pat = (pat_act*)ctx->mac.allocate(sizeof(pat_act0));
	if (!pat) {
		return 0;
	}
	pat->type = vg_pattern_type_t::VG_PATTERN_TYPE_LINEAR;
	pat->extend = vg_extend_t::VG_EXTEND_NONE;
	pat->data = &pat->g;
	_vg_pattern_edit_linear(pat, x0, y0, x1, y1);
	pat->matrix = glm::mat3x2(1.0);
	pat->references = 1;
	return pat;
}
vg_pattern_t* vctx_new_pattern_radial(rvg_t* ctx, float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse) {
	if (!ctx)return 0;
	auto pat = (pat_act*)ctx->mac.allocate(sizeof(pat_act0));
	if (!pat) {
		return 0;
	}
	pat->type = vg_pattern_type_t::VG_PATTERN_TYPE_RADIAL;
	pat->extend = vg_extend_t::VG_EXTEND_NONE;
	pat->data = &pat->g;
	pat->matrix = glm::mat3x2(1.0);
	vg_pattern_edit_radial(pat, cx0, cy0, radius0, cx1, cy1, radius1, is_ellipse);
	pat->references = 1;
	return pat;
}
vg_pattern_t* vctx_new_pattern_sweep(rvg_t* ctx, float cx, float cy, float start_angle, float end_angle) {
	if (!ctx)return 0;
	auto pat = (pat_act*)ctx->mac.allocate(sizeof(pat_act0));
	if (!pat) {
		return 0;
	}
	pat->type = vg_pattern_type_t::VG_PATTERN_TYPE_SWEEP;
	pat->extend = vg_extend_t::VG_EXTEND_NONE;
	pat->data = &pat->g;
	pat->matrix = glm::mat3x2(1.0);
	vg_pattern_edit_sweep(pat, cx, cy, start_angle, end_angle);
	pat->references = 1;
	return pat;
}
int  vctx_pattern_add_color_stop(vg_pattern_t* pat, float o, float r, float g, float b, float a) {
	if (!pat) return -1;
	return ovg_pattern_add_color_stop(pat, o, r, g, b, a);
}
int  vctx_pattern_set_color_stop(vg_pattern_t* pat, int idx, float o, float r, float g, float b, float a) {
	if (!pat || idx < 0) return -1;
	return ovg_pattern_set_color_stop(pat, idx, o, r, g, b, a);
}
void vctx_pattern_set_matrix(vg_pattern_t* pat, const void* matrix) {
	if (pat) ovg_pattern_set_matrix(pat, (const ovg_matrix_t*)matrix);
}
void vctx_pattern_set_extend(vg_pattern_t* pat, int extend) {
	if (pat) ovg_pattern_set_extend(pat, extend);
}
void vctx_pattern_set_filter(vg_pattern_t* pat, int filter) {
	if (pat) ovg_pattern_set_filter(pat, filter);
}

void vctx_save(rvg_t* v) {
	if (v)v->save();
}
void vctx_restore(rvg_t* v) {
	if (v)v->restore();
}
void vctx_stroke(rvg_t* v) {
	if (v) ovg_stroke(v);
}
void vctx_stroke_preserve(rvg_t* v) {
	if (v) ovg_stroke_preserve(v);
}

void vctx_fill(rvg_t* v) {
	if (v) ovg_fill(v);
}

void vctx_fill_preserve(rvg_t* v) {
	if (v) ovg_fill_preserve(v);
}

void vctx_paint(rvg_t* v) {
	if (v) ovg_paint(v);
}

void vctx_reset_clip(rvg_t* v) {
	if (v) ovg_reset_clip(v);
}

void vctx_clip(rvg_t* v) {
	if (v) ovg_clip(v);
}

void vctx_clip_preserve(rvg_t* v) {
	if (v) ovg_clip_preserve(v);
}

void vctx_clip_rect(rvg_t* v, int x, int y, int width, int height) {
	if (v) ovg_clip_rect(v, x, y, width, height);
}

void vctx_set_clip_rect(rvg_t* v, void* rc) {
	if (v && rc) ovg_set_clip_rect(v, rc);
}

void vctx_get_clip_rect(rvg_t* v, void* rc) {
	if (v && rc) ovg_get_clip_rect(v, rc);
}

/* ================= 高层绘制命令 ================= */

// 添加文本，风格，渲染区可选
void vctx_add_text(rvg_t* dc, text_st_t* p, text_style_t* ts, text_box_rt* box) {
	if (dc) ovg_add_text(dc, p, ts, box);
}

// 普通图片，支持九宫格、混合颜色
void vctx_add_image(rvg_t* dc, ovg_image_r* r) {
	if (dc) ovg_add_image(dc, r);
}

// 原始三角形，输入0则不修改
void vctx_set_geom_state(rvg_t* dc, gem_info_t* info, const void* matrix4x4) {
	if (dc) ovg_set_geom_state(dc, info, (glm::mat4*)matrix4x4);
}

// 添加几何数据到缓冲区
void vctx_add_geometry(rvg_t* dc,
	vg_surface_t* texture,
	const float* xy, int xy_stride,
	const void* color, int color_stride,
	const float* uv, int uv_stride,
	int num_vertices,
	const void* indices, int num_indices,
	int size_indices, int color_type) {
	if (dc) {
		ovg_add_geometry(dc,
			texture,
			xy, xy_stride,
			color, color_stride,
			uv, uv_stride,
			num_vertices,
			indices, num_indices,
			size_indices, color_type);
	}
}

// 添加3D几何数据到缓冲区
void vctx_add_geometry3d(rvg_t* dc,
	vg_surface_t* texture,
	const float* xyz, int xyz_stride,
	const void* color, int color_stride,
	const float* uv, int uv_stride,
	int num_vertices,
	const void* indices, int num_indices,
	int size_indices, int color_type) {
	if (dc) {
		ovg_add_geometry3d(dc,
			texture,
			xyz, xyz_stride,
			color, color_stride,
			uv, uv_stride,
			num_vertices,
			indices, num_indices,
			size_indices, color_type);
	}
}


void vctx_start_recording(rvg_t* ctx) {

}
ovg_recording_t* vctx_stop_recording(rvg_t* ctx) {
	return 0;
}
void vctx_replay(rvg_t* ctx, ovg_recording_t* rec) {

}
void vctx_replay_command(rvg_t* ctx, ovg_recording_t* rec, uint32_t cmdIndex) {

}
uint32_t vctx_recording_get_count(ovg_recording_t* rec) {
	return 0;
}
void* vctx_recording_get_data(ovg_recording_t* rec) {
	return 0;
}
void  vctx_recording_destroy(ovg_recording_t* rec) {

}
void init_ovg_ctx_cb(ovg_ctx_cb* cb)
{
	if (!cb)return;
	cb->new_rvg = vctx_new_rvg;
	cb->destroy_rvg = vctx_destroy_rvg;
	cb->clear = vctx_clear;
	cb->get_path = vctx_get_path;
	cb->new_path = vctx_new_path;
	cb->clear_path = vctx_clear_path;
	cb->close_path = vctx_close_path;
	cb->new_sub_path = vctx_new_sub_path;
	cb->path_extents = vctx_path_extents;
	cb->get_current_point = vctx_get_current_point;
	cb->get_segment_count = vctx_get_segment_count;
	cb->set_segment_color = vctx_set_segment_color;
	cb->add_path = vctx_add_path;
	cb->move_to = vctx_move_to;
	cb->rel_move_to = vctx_rel_move_to;
	cb->line_to = vctx_line_to;
	cb->rel_line_to = vctx_rel_line_to;
	cb->arc = vctx_arc;
	cb->arc_negative = vctx_arc_negative;
	cb->curve_to = vctx_curve_to;
	cb->rel_curve_to = vctx_rel_curve_to;
	cb->quadratic_to = vctx_quadratic_to;
	cb->rel_quadratic_to = vctx_rel_quadratic_to;
	cb->rectangle = vctx_rectangle;
	cb->rounded_rectangle = vctx_rounded_rectangle;
	cb->rounded_rectangle2 = vctx_rounded_rectangle2;
	cb->ellipse = vctx_ellipse;
	cb->elliptic_arc_to = vctx_elliptic_arc_to;
	cb->rel_elliptic_arc_to = vctx_rel_elliptic_arc_to;
	cb->circle = vctx_circle;
	cb->set_opacity = vctx_set_opacity;
	cb->set_source_color = vctx_set_source_color;
	cb->set_source_rgba = vctx_set_source_rgba;
	cb->set_source_rgb = vctx_set_source_rgb;
	cb->set_line_width = vctx_set_line_width;
	cb->set_miter_limit = vctx_set_miter_limit;
	cb->set_line_cap = vctx_set_line_cap;
	cb->set_line_join = vctx_set_line_join;
	cb->set_source_surface = vctx_set_source_surface;
	cb->set_source = vctx_set_source;
	cb->set_operator = vctx_set_operator;
	cb->set_fill_rule = vctx_set_fill_rule;
	cb->set_dash = vctx_set_dash;
	cb->set_dash8 = vctx_set_dash8;
	cb->translate = vctx_translate;
	cb->scale = vctx_scale;
	cb->rotate = vctx_rotate;
	cb->transform = vctx_transform;
	cb->set_matrix = vctx_set_matrix;
	cb->get_matrix = vctx_get_matrix;
	cb->identity_matrix = vctx_identity_matrix;
	cb->new_pattern_linear = vctx_new_pattern_linear;
	cb->new_pattern_radial = vctx_new_pattern_radial;
	cb->new_pattern_sweep = vctx_new_pattern_sweep;
	cb->pattern_add_color_stop = vctx_pattern_add_color_stop;
	cb->pattern_set_color_stop = vctx_pattern_set_color_stop;
	cb->pattern_set_matrix = vctx_pattern_set_matrix;
	cb->pattern_set_extend = vctx_pattern_set_extend;
	cb->pattern_set_filter = vctx_pattern_set_filter;
	cb->save = vctx_save;
	cb->restore = vctx_restore;
	cb->stroke = vctx_stroke;
	cb->stroke_preserve = vctx_stroke_preserve;
	cb->fill = vctx_fill;
	cb->fill_preserve = vctx_fill_preserve;
	cb->paint = vctx_paint;
	cb->reset_clip = vctx_reset_clip;
	cb->clip = vctx_clip;
	cb->clip_preserve = vctx_clip_preserve;
	cb->clip_rect = vctx_clip_rect;
	cb->set_clip_rect = vctx_set_clip_rect;
	cb->get_clip_rect = vctx_get_clip_rect;
	cb->add_text = vctx_add_text;
	cb->add_image = vctx_add_image;
	cb->set_geom_state = vctx_set_geom_state;
	cb->add_geometry = vctx_add_geometry;
	cb->add_geometry3d = vctx_add_geometry3d;
	cb->start_recording = vctx_start_recording;
	cb->stop_recording = vctx_stop_recording;
	cb->replay = vctx_replay;
	cb->replay_command = vctx_replay_command;
	cb->recording_get_count = vctx_recording_get_count;
	cb->recording_get_data = vctx_recording_get_data;
	cb->recording_destroy = vctx_recording_destroy;

}
#endif // 1
