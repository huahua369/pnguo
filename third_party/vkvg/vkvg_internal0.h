#pragma once
#ifndef VKVG_DEVICE_INTERNAL_H
#define VKVG_DEVICE_INTERNAL_H

// disable warning on iostream functions on windows
#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h> // needed before stdarg.h on Windows
#include <stdarg.h>
#include <string.h>

// should be supported by c11
// #include <threads.h>
// #include <stdatomic.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifndef M_PIF
#define M_PIF   3.14159265358979323846f /* float pi */
#define M_PIF_2 1.57079632679489661923f
#define M_2_PIF 0.63661977236758134308f // 2/pi
#endif

/*#ifndef M_2_PI
	#define M_2_PI		0.63661977236758134308	// 2/pi
#endif*/

#ifdef DEBUG
#define LOG(level, ...)                                                                                                \
    {                                                                                                                  \
        if ((vkvg_log_level) & (level))                                                                                \
            fprintf(stdout, __VA_ARGS__);                                                                              \
    }
#else
#define LOG
#endif

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

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

//#include "deps/tinycthread.h"
#ifndef _TINYCTHREAD_H_
#define _TINYCTHREAD_H_
#if !defined(_TTHREAD_PLATFORM_DEFINED_)
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#define _TTHREAD_WIN32_
#else
#define _TTHREAD_POSIX_
#endif
#define _TTHREAD_PLATFORM_DEFINED_
#endif

/* Activate some POSIX functionality (e.g. clock_gettime and recursive mutexes) */
#if defined(_TTHREAD_POSIX_)
#undef _FEATURES_H
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#if !defined(_POSIX_C_SOURCE) || ((_POSIX_C_SOURCE - 0) < 199309L)
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#if !defined(_XOPEN_SOURCE) || ((_XOPEN_SOURCE - 0) < 500)
#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#endif

/* Generic includes */
#include <time.h>

/* Platform specific includes */
#if defined(_TTHREAD_POSIX_)
#include <sys/time.h>
#include <pthread.h>
#elif defined(_TTHREAD_WIN32_)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define __UNDEF_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifdef __UNDEF_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#undef __UNDEF_LEAN_AND_MEAN
#endif
#endif

/* Workaround for missing TIME_UTC: If time.h doesn't provide TIME_UTC,
   it's quite likely that libc does not support it either. Hence, fall back to
   the only other supported time specifier: CLOCK_REALTIME (and if that fails,
   we're probably emulating clock_gettime anyway, so anything goes). */
#ifndef TIME_UTC
#ifdef CLOCK_REALTIME
#define TIME_UTC CLOCK_REALTIME
#else
#define TIME_UTC 0
#endif
#endif

   /* Workaround for missing clock_gettime (most Windows compilers, afaik) */
#if defined(_TTHREAD_WIN32_) || defined(__APPLE_CC__)
#define _TTHREAD_EMULATE_CLOCK_GETTIME_
/* Emulate struct timespec */
#if defined(_TTHREAD_WIN32_)
struct _ttherad_timespec {
	time_t tv_sec;
	long   tv_nsec;
};
#define timespec _ttherad_timespec
#endif

/* Emulate clockid_t */
typedef int _tthread_clockid_t;
#define clockid_t _tthread_clockid_t

/* Emulate clock_gettime */
int _tthread_clock_gettime(clockid_t clk_id, struct timespec* ts);
#define clock_gettime _tthread_clock_gettime
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
#endif

/** TinyCThread version (major number). */
#define TINYCTHREAD_VERSION_MAJOR 1
/** TinyCThread version (minor number). */
#define TINYCTHREAD_VERSION_MINOR 1
/** TinyCThread version (full version). */
#define TINYCTHREAD_VERSION (TINYCTHREAD_VERSION_MAJOR * 100 + TINYCTHREAD_VERSION_MINOR)

/**
 * @def _Thread_local
 * Thread local storage keyword.
 * A variable that is declared with the @c _Thread_local keyword makes the
 * value of the variable local to each thread (known as thread-local storage,
 * or TLS). Example usage:
 * @code
 * // This variable is local to each thread.
 * _Thread_local int variable;
 * @endcode
 * @note The @c _Thread_local keyword is a macro that maps to the corresponding
 * compiler directive (e.g. @c __declspec(thread)).
 * @note This directive is currently not supported on Mac OS X (it will give
 * a compiler error), since compile-time TLS is not supported in the Mac OS X
 * executable format. Also, some older versions of MinGW (before GCC 4.x) do
 * not support this directive.
 * @hideinitializer
 */

 /* FIXME: Check for a PROPER value of __STDC_VERSION__ to know if we have C11 */
#if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L)) && !defined(_Thread_local)
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
#define _Thread_local __thread
#else
#define _Thread_local __declspec(thread)
#endif
#endif

/* Macros */
#define TSS_DTOR_ITERATIONS 0

/* Function return values */
#define thrd_error   0 /**< The requested operation failed */
#define thrd_success 1 /**< The requested operation succeeded */
#define thrd_timeout 2 /**< The time specified in the call was reached without acquiring the requested resource */
#define thrd_busy                                                                                                      \
    3 /**< The requested operation failed because a tesource requested by a test and return function is already in use \
       */
#define thrd_nomem 4 /**< The requested operation failed because it was unable to allocate memory */

/* Mutex types */
#define mtx_plain     1
#define mtx_timed     2
#define mtx_try       4
#define mtx_recursive 8

/* Mutex */
#if defined(_TTHREAD_WIN32_)
typedef struct {
	CRITICAL_SECTION mHandle;        /* Critical section handle */
	int              mAlreadyLocked; /* TRUE if the mutex is already locked */
	int              mRecursive;     /* TRUE if the mutex is recursive */
} mtx_t;
#else
typedef pthread_mutex_t mtx_t;
#endif

/** Create a mutex object.
 * @param mtx A mutex object.
 * @param type Bit-mask that must have one of the following six values:
 *   @li @c mtx_plain for a simple non-recursive mutex
 *   @li @c mtx_timed for a non-recursive mutex that supports timeout
 *   @li @c mtx_try for a non-recursive mutex that supports test and return
 *   @li @c mtx_plain | @c mtx_recursive (same as @c mtx_plain, but recursive)
 *   @li @c mtx_timed | @c mtx_recursive (same as @c mtx_timed, but recursive)
 *   @li @c mtx_try | @c mtx_recursive (same as @c mtx_try, but recursive)
 * @return @ref thrd_success on success, or @ref thrd_error if the request could
 * not be honored.
 */
int mtx_init(mtx_t* mtx, int type);

/** Release any resources used by the given mutex.
 * @param mtx A mutex object.
 */
void mtx_destroy(mtx_t* mtx);

/** Lock the given mutex.
 * Blocks until the given mutex can be locked. If the mutex is non-recursive, and
 * the calling thread already has a lock on the mutex, this call will block
 * forever.
 * @param mtx A mutex object.
 * @return @ref thrd_success on success, or @ref thrd_error if the request could
 * not be honored.
 */
int mtx_lock(mtx_t* mtx);

/** NOT YET IMPLEMENTED.
 */
int mtx_timedlock(mtx_t* mtx, const struct timespec* ts);

/** Try to lock the given mutex.
 * The specified mutex shall support either test and return or timeout. If the
 * mutex is already locked, the function returns without blocking.
 * @param mtx A mutex object.
 * @return @ref thrd_success on success, or @ref thrd_busy if the resource
 * requested is already in use, or @ref thrd_error if the request could not be
 * honored.
 */
int mtx_trylock(mtx_t* mtx);

/** Unlock the given mutex.
 * @param mtx A mutex object.
 * @return @ref thrd_success on success, or @ref thrd_error if the request could
 * not be honored.
 */
int mtx_unlock(mtx_t* mtx);

/* Condition variable */
#if defined(_TTHREAD_WIN32_)
typedef struct {
	HANDLE           mEvents[2];        /* Signal and broadcast event HANDLEs. */
	unsigned int     mWaitersCount;     /* Count of the number of waiters. */
	CRITICAL_SECTION mWaitersCountLock; /* Serialize access to mWaitersCount. */
} cnd_t;
#else
typedef pthread_cond_t  cnd_t;
#endif

/** Create a condition variable object.
 * @param cond A condition variable object.
 * @return @ref thrd_success on success, or @ref thrd_error if the request could
 * not be honored.
 */
int cnd_init(cnd_t* cond);

/** Release any resources used by the given condition variable.
 * @param cond A condition variable object.
 */
void cnd_destroy(cnd_t* cond);

/** Signal a condition variable.
 * Unblocks one of the threads that are blocked on the given condition variable
 * at the time of the call. If no threads are blocked on the condition variable
 * at the time of the call, the function does nothing and return success.
 * @param cond A condition variable object.
 * @return @ref thrd_success on success, or @ref thrd_error if the request could
 * not be honored.
 */
int cnd_signal(cnd_t* cond);

/** Broadcast a condition variable.
 * Unblocks all of the threads that are blocked on the given condition variable
 * at the time of the call. If no threads are blocked on the condition variable
 * at the time of the call, the function does nothing and return success.
 * @param cond A condition variable object.
 * @return @ref thrd_success on success, or @ref thrd_error if the request could
 * not be honored.
 */
int cnd_broadcast(cnd_t* cond);

/** Wait for a condition variable to become signaled.
 * The function atomically unlocks the given mutex and endeavors to block until
 * the given condition variable is signaled by a call to cnd_signal or to
 * cnd_broadcast. When the calling thread becomes unblocked it locks the mutex
 * before it returns.
 * @param cond A condition variable object.
 * @param mtx A mutex object.
 * @return @ref thrd_success on success, or @ref thrd_error if the request could
 * not be honored.
 */
int cnd_wait(cnd_t* cond, mtx_t* mtx);

/** Wait for a condition variable to become signaled.
 * The function atomically unlocks the given mutex and endeavors to block until
 * the given condition variable is signaled by a call to cnd_signal or to
 * cnd_broadcast, or until after the specified time. When the calling thread
 * becomes unblocked it locks the mutex before it returns.
 * @param cond A condition variable object.
 * @param mtx A mutex object.
 * @param xt A point in time at which the request will time out (absolute time).
 * @return @ref thrd_success upon success, or @ref thrd_timeout if the time
 * specified in the call was reached without acquiring the requested resource, or
 * @ref thrd_error if the request could not be honored.
 */
int cnd_timedwait(cnd_t* cond, mtx_t* mtx, const struct timespec* ts);

/* Thread */
#if defined(_TTHREAD_WIN32_)
typedef HANDLE thrd_t;
#else
typedef pthread_t       thrd_t;
#endif

/** Thread start function.
 * Any thread that is started with the @ref thrd_create() function must be
 * started through a function of this type.
 * @param arg The thread argument (the @c arg argument of the corresponding
 *        @ref thrd_create() call).
 * @return The thread return value, which can be obtained by another thread
 * by using the @ref thrd_join() function.
 */
typedef int (*thrd_start_t)(void* arg);

/** Create a new thread.
 * @param thr Identifier of the newly created thread.
 * @param func A function pointer to the function that will be executed in
 *        the new thread.
 * @param arg An argument to the thread function.
 * @return @ref thrd_success on success, or @ref thrd_nomem if no memory could
 * be allocated for the thread requested, or @ref thrd_error if the request
 * could not be honored.
 * @note A thread’s identifier may be reused for a different thread once the
 * original thread has exited and either been detached or joined to another
 * thread.
 */
int thrd_create(thrd_t* thr, thrd_start_t func, void* arg);

/** Identify the calling thread.
 * @return The identifier of the calling thread.
 */
thrd_t thrd_current(void);

/** NOT YET IMPLEMENTED.
 */
int thrd_detach(thrd_t thr);

/** Compare two thread identifiers.
 * The function determines if two thread identifiers refer to the same thread.
 * @return Zero if the two thread identifiers refer to different threads.
 * Otherwise a nonzero value is returned.
 */
int thrd_equal(thrd_t thr0, thrd_t thr1);

/** Terminate execution of the calling thread.
 * @param res Result code of the calling thread.
 */
void thrd_exit(int res);

/** Wait for a thread to terminate.
 * The function joins the given thread with the current thread by blocking
 * until the other thread has terminated.
 * @param thr The thread to join with.
 * @param res If this pointer is not NULL, the function will store the result
 *        code of the given thread in the integer pointed to by @c res.
 * @return @ref thrd_success on success, or @ref thrd_error if the request could
 * not be honored.
 */
int thrd_join(thrd_t thr, int* res);

/** Put the calling thread to sleep.
 * Suspend execution of the calling thread.
 * @param time_point A point in time at which the thread will resume (absolute time).
 * @param remaining If non-NULL, this parameter will hold the remaining time until
 *                  time_point upon return. This will typically be zero, but if
 *                  the thread was woken up by a signal that is not ignored before
 *                  time_point was reached @c remaining will hold a positive
 *                  time.
 * @return 0 (zero) on successful sleep, or -1 if an interrupt occurred.
 */
int thrd_sleep(const struct timespec* time_point, struct timespec* remaining);

/** Yield execution to another thread.
 * Permit other threads to run, even if the current thread would ordinarily
 * continue to run.
 */
void thrd_yield(void);

/* Thread local storage */
#if defined(_TTHREAD_WIN32_)
typedef DWORD tss_t;
#else
typedef pthread_key_t   tss_t;
#endif

/** Destructor function for a thread-specific storage.
 * @param val The value of the destructed thread-specific storage.
 */
typedef void (*tss_dtor_t)(void* val);

/** Create a thread-specific storage.
 * @param key The unique key identifier that will be set if the function is
 *        successful.
 * @param dtor Destructor function. This can be NULL.
 * @return @ref thrd_success on success, or @ref thrd_error if the request could
 * not be honored.
 * @note The destructor function is not supported under Windows. If @c dtor is
 * not NULL when calling this function under Windows, the function will fail
 * and return @ref thrd_error.
 */
int tss_create(tss_t* key, tss_dtor_t dtor);

/** Delete a thread-specific storage.
 * The function releases any resources used by the given thread-specific
 * storage.
 * @param key The key that shall be deleted.
 */
void tss_delete(tss_t key);

/** Get the value for a thread-specific storage.
 * @param key The thread-specific storage identifier.
 * @return The value for the current thread held in the given thread-specific
 * storage.
 */
void* tss_get(tss_t key);

/** Set the value for a thread-specific storage.
 * @param key The thread-specific storage identifier.
 * @param val The value of the thread-specific storage to set for the current
 *        thread.
 * @return @ref thrd_success on success, or @ref thrd_error if the request could
 * not be honored.
 */
int tss_set(tss_t key, void* val);
#endif
//#include "cross_os.h"

// cross platform os helpers
#if defined(_WIN32) || defined(_WIN64)
// disable warning on iostream functions on windows
#define _CRT_SECURE_NO_WARNINGS
#include "windows.h"
#if defined(_WIN64)
#ifndef isnan
#define isnan _isnanf
#endif
#endif
#define vkvg_inline     __forceinline
#define disable_warning (warn)
#define reset_warning   (warn)
#elif __APPLE__
#include <math.h>
#define vkvg_inline     static
#define disable_warning (warn)
#define reset_warning   (warn)
#elif __unix__
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#define vkvg_inline     static inline __attribute((always_inline))
#define disable_warning (warn) #pragma GCC diagnostic ignored "-W" #warn
#define reset_warning   (warn) #pragma GCC diagnostic warning "-W" #warn
#if __linux__
void _linux_register_error_handler();
#endif
#endif

const char* getUserDir();
// width of the stencil buffer will determine the number of context saving/restore layers
// the two first bits of the stencil are the FILL and the CLIP bits, all other bits are
// used to store clipping bit on context saving. 8 bit stencil will allow 6 save/restore layer
#define FB_COLOR_FORMAT VK_FORMAT_B8G8R8A8_UNORM
#define VKVG_SURFACE_IMGS_REQUIREMENTS                                                                                 \
    (VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |                                    \
     VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT)
#define VKVG_PNG_WRITE_IMG_REQUIREMENTS                                                                                \
    (VK_FORMAT_FEATURE_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT)
// 30 seconds fence timeout
#define VKVG_FENCE_TIMEOUT 30000000000
// #define VKVG_FENCE_TIMEOUT 10000

#include "vkvg.h"
#include "vkh.h"

struct vec2 {
	float x;
	float y;
};

static const vec2 vec2_unit_x = { 1.f, 0 };
static const vec2 vec2_unit_y = { 0, 1.f };

typedef struct {
	double x;
	double y;
} vec2d;

/*const vec2d vec2d_unit_x = {1.0,0};
const vec2d vec2d_unit_y = {0,1.0};*/

typedef struct {
	float x;
	float y;
	float z;
} vec3;

typedef struct {
	union {
		float x;
		float r;
		float xMin;
	};
	union {
		float y;
		float g;
		float yMin;
	};
	union {
		float z;
		float width;
		float b;
		float xMax;
	};
	union {
		float w;
		float height;
		float a;
		float yMax;
	};

} vec4;

struct ivec2 { int x, y; };
struct ivec4 { int x, y, z, w; };

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t z;
	uint16_t w;
} vec4i16;

typedef struct {
	int16_t x;
	int16_t y;
} vec2i16;

typedef struct {
	vec2 row0;
	vec2 row1;
} mat2;

// compute length of float vector 2d
vkvg_inline float vec2_length(vec2 v);
// compute normal direction vector from line defined by 2 points in double precision
vkvg_inline vec2d vec2d_line_norm(vec2d a, vec2d b);
// compute normal direction vector from line defined by 2 points
vkvg_inline vec2 vec2_line_norm(vec2 a, vec2 b);
// compute sum of two double precision vectors
vkvg_inline vec2d vec2d_add(vec2d a, vec2d b);
// compute subbstraction of two double precision vectors
vkvg_inline vec2d vec2d_sub(vec2d a, vec2d b);
// multiply 2d vector by scalar
vkvg_inline vec2d vec2d_mult_s(vec2d a, double m);
vkvg_inline vec2d vec2d_div_s(vec2d a, double m);
// compute length of double vector 2d
vkvg_inline double vec2d_length(vec2d v);
// normalize double vector
vkvg_inline vec2d vec2d_norm(vec2d a);
// compute perpendicular vector
vkvg_inline vec2d vec2d_perp(vec2d a);
vkvg_inline bool  vec2d_isnan(vec2d v);

// test equality of two single precision vectors
vkvg_inline bool vec2_equ(vec2 a, vec2 b);
// compute sum of two single precision vectors
vkvg_inline vec2 vec2_add(vec2 a, vec2 b);
// compute subbstraction of two single precision vectors
vkvg_inline vec2 vec2_sub(vec2 a, vec2 b);
// multiply 2d vector by scalar
vkvg_inline vec2 vec2_mult_s(vec2 a, float m);
// devide 2d vector by scalar
vkvg_inline vec2 vec2_div_s(vec2 a, float m);
// normalize float vector
vkvg_inline vec2 vec2_norm(vec2 a);
// compute perpendicular vector
vkvg_inline vec2 vec2_perp(vec2 a);
// compute opposite of single precision vector
vkvg_inline void vec2_inv(vec2* v);
// test if one component of float vector is nan
vkvg_inline bool vec2_isnan(vec2 v);
// test if one component of double vector is nan
vkvg_inline float vec2_dot(vec2 a, vec2 b);
vkvg_inline float vec2_det(vec2 a, vec2 b);
vkvg_inline float vec2_slope(vec2 a, vec2 b);

// convert double precision vector to single precision
vkvg_inline vec2 vec2d_to_vec2(vec2d vd);
vkvg_inline bool vec4_equ(vec4 a, vec4 b);
vkvg_inline vec2 mat2_mult_vec2(mat2 m, vec2 v);
vkvg_inline float mat2_det(mat2* m);

/*typedef struct {
	vkvg_status_t status;
} _vkvg_no_mem_struct;*/

// static error value whose address is returned instead of vkvg object pointers to avoid null pointers.
static vkvg_status_t _vkvg_status_no_memory = VKVG_STATUS_NO_MEMORY;
static vkvg_status_t _vkvg_status_null_pointer = VKVG_STATUS_NULL_POINTER;
static vkvg_status_t _vkvg_status_invalid_dev_ci = VKVG_STATUS_INVALID_DEVICE_CREATE_INFO;
static vkvg_status_t _vkvg_status_device_error = VKVG_STATUS_DEVICE_ERROR;
static vkvg_status_t _vkvg_status_invalid_surface = VKVG_STATUS_INVALID_SURFACE;


#define STENCIL_FILL_BIT              0x1
#define STENCIL_CLIP_BIT              0x2
#define STENCIL_ALL_BIT               0x3

#define VKVG_MAX_CACHED_CONTEXT_COUNT 2

//extern PFN_vkCmdBindPipeline       CmdBindPipeline;
//extern PFN_vkCmdBindDescriptorSets CmdBindDescriptorSets;
//extern PFN_vkCmdBindIndexBuffer    CmdBindIndexBuffer;
//extern PFN_vkCmdBindVertexBuffers  CmdBindVertexBuffers;
//
//extern PFN_vkCmdDrawIndexed CmdDrawIndexed;
//extern PFN_vkCmdDraw        CmdDraw;
//
//extern PFN_vkCmdSetStencilCompareMask CmdSetStencilCompareMask;
//extern PFN_vkCmdSetStencilReference   CmdSetStencilReference;
//extern PFN_vkCmdSetStencilWriteMask   CmdSetStencilWriteMask;
//extern PFN_vkCmdBeginRenderPass       CmdBeginRenderPass;
//extern PFN_vkCmdEndRenderPass         CmdEndRenderPass;
//extern PFN_vkCmdSetViewport           CmdSetViewport;
//extern PFN_vkCmdSetScissor            CmdSetScissor;
//
//extern PFN_vkCmdPushConstants   CmdPushConstants;
//extern PFN_vkWaitForFences      WaitForFences;
//extern PFN_vkResetFences        ResetFences;
//extern PFN_vkResetCommandBuffer ResetCommandBuffer;

typedef struct _font_cache_t font_cache_t;


typedef struct _cached_ctx {
	thrd_t              thread;
	VkvgContext         ctx;
	struct _cached_ctx* pNext;
} _cached_ctx;

typedef struct _vkvg_device_t {
	vkvg_status_t                    status;      /**< Current status of device, affected by last operation */
	VkDevice                         vkDev;       /**< Vulkan Logical Device */
	VkPhysicalDeviceMemoryProperties phyMemProps; /**< Vulkan Physical device memory properties */
	VkPhysicalDevice                 phy;         /**< Vulkan Physical device */
	VkInstance                       instance;    /**< Vulkan instance */
#ifdef VKH_USE_VMA
	void* allocator; /**< Vulkan Memory allocator */
#endif

	VkImageTiling      supportedTiling;   /**< Supported image tiling for surface, 0xFF=no support */
	VkFormat           stencilFormat;     /**< Supported vulkan image format for stencil */
	VkImageAspectFlags stencilAspectFlag; /**< stencil only or depth stencil, could be solved by
											 VK_KHR_separate_depth_stencil_layouts*/
	VkFormat      pngStagFormat;          /**< Supported vulkan image format png write staging img */
	VkImageTiling pngStagTiling;          /**< tiling for the blit operation */

	mtx_t    mutex;       /**< protect device access (queue, cahes, ...)from ctxs in separate threads */
	bool     threadAware; /**< if true, mutex is created and guard device queue and caches access */
	VkhQueue gQueue;      /**< Vulkan Queue with Graphic flag */

	VkRenderPass renderPass; /**< Vulkan render pass, common for all surfaces */
	VkRenderPass
		renderPass_ClearStencil;      /**< Vulkan render pass for first draw with context, stencil has to be cleared */
	VkRenderPass renderPass_ClearAll; /**< Vulkan render pass for new surface, clear all attacments*/

	uint32_t        references; /**< Reference count, prevent destroying device if still in use */
	VkCommandPool   cmdPool;    /**< Global command pool for processing on surfaces without context */
	VkCommandBuffer cmd;        /**< Global command buffer */
	VkFence fence; /**< this fence is kept signaled when idle, wait and reset are called before each recording. */

	VkPipeline pipe_OVER; /**< default operator */
	VkPipeline pipe_SUB;
	VkPipeline pipe_CLEAR; /**< clear operator */

	VkPipeline pipelinePolyFill; /**< even-odd polygon filling first step */
	VkPipeline pipelineClipping; /**< draw on stencil to update clipping regions */

	VkPipelineCache       pipelineCache;  /**< speed up startup by caching configured pipelines on disk */
	VkPipelineLayout      pipelineLayout; /**< layout common to all pipelines */
	VkPipelineLayout      pipelineLayout0; /**< layout common to all pipelines */
	VkDescriptorSetLayout dslFont;        /**< font cache descriptors layout */
	VkDescriptorSetLayout dslSrc;         /**< context source surface descriptors layout */
	VkDescriptorSetLayout dslGrad;        /**< context gradient descriptors layout */
	VkDescriptorSetLayout dslPushDset;        // push dset

	int hdpi, /**< only used for FreeType fonts and svg loading */
		vdpi;

	VkhDevice vkhDev; /**< old VkhDev created during vulkan context creation by @ref vkvg_device_create. */

	VkhImage           emptyImg;        /**< prevent unbound descriptor to trigger Validation error 61 */
	VkSampleCountFlags samples;         /**< samples count common to all surfaces */
	bool               deferredResolve; /**< if true, resolve only on context destruction and set as source */

	font_cache_t* fontCache; /**< Store everything relative to common font caching system */

	VkvgContext lastCtx; /**< last element of double linked list of context, used to trigger font caching system update
							on all contexts*/

	int32_t      cachedContextMaxCount; /**< Maximum context cache element count.*/
	int32_t      cachedContextCount;    /**< Current context cache element count.*/
	_cached_ctx* cachedContextLast;     /**< Last element of single linked list of saved context for fast reuse.*/

#ifdef VKVG_WIRED_DEBUG
	VkPipeline pipelineWired;
	VkPipeline pipelineLineList;
#endif
#if VKVG_DBG_STATS
	vkvg_debug_stats_t debug_stats; /**< debug statistics on memory usage and vulkan ressources */
#endif
} vkvg_device;

#define LOCK_DEVICE                                                                                                    \
    if (dev->threadAware)                                                                                              \
        mtx_lock(&dev->mutex);
#define UNLOCK_DEVICE                                                                                                  \
    if (dev->threadAware)                                                                                              \
        mtx_unlock(&dev->mutex);

bool _device_try_get_phyinfo(VkhPhyInfo* phys, uint32_t phyCount, VkPhysicalDeviceType gpuType, VkhPhyInfo* phy);
bool _device_init_function_pointers(VkvgDevice dev);
void _device_create_empty_texture(VkvgDevice dev, VkFormat format, VkImageTiling tiling);
void _device_get_best_image_tiling(VkvgDevice dev, VkFormat format, VkImageTiling* pTiling);
void _device_check_best_image_tiling(VkvgDevice dev, VkFormat format);
void _device_create_pipeline_cache(VkvgDevice dev);
VkRenderPass _device_createRenderPassMS(VkvgDevice dev, VkAttachmentLoadOp loadOp, VkAttachmentLoadOp stencilLoadOp);
VkRenderPass _device_createRenderPassNoResolve(VkvgDevice dev, VkAttachmentLoadOp loadOp,
	VkAttachmentLoadOp stencilLoadOp);
void         _device_setupPipelines(VkvgDevice dev);
void         _device_createDescriptorSetLayout(VkvgDevice dev);
void         _device_wait_idle(VkvgDevice dev);
void         _device_wait_and_reset_device_fence(VkvgDevice dev);
void         _device_submit_cmd(VkvgDevice dev, VkCommandBuffer* cmd, VkFence fence);

bool _device_try_get_cached_context(VkvgDevice dev, VkvgContext* pCtx);
void _device_store_context(VkvgContext ctx);
#endif


#define FONT_NAME_MAX_SIZE      128

#if VKVG_RECORDING
//#include "recording/vkvg_record_internal.h"

#define VKVG_CMD_SAVE               0x0001
#define VKVG_CMD_RESTORE            0x0002

#define VKVG_CMD_PATH_COMMANDS      0x0100
#define VKVG_CMD_DRAW_COMMANDS      0x0200
#define VKVG_CMD_RELATIVE_COMMANDS  (0x0400 | VKVG_CMD_PATH_COMMANDS)
#define VKVG_CMD_PATHPROPS_COMMANDS (0x1000 | VKVG_CMD_PATH_COMMANDS)
#define VKVG_CMD_PRESERVE_COMMANDS  (0x0400 | VKVG_CMD_DRAW_COMMANDS)
#define VKVG_CMD_PATTERN_COMMANDS   0x0800
#define VKVG_CMD_TRANSFORM_COMMANDS 0x2000
#define VKVG_CMD_TEXT_COMMANDS      0x4000

#define VKVG_CMD_NEW_PATH           (0x0001 | VKVG_CMD_PATH_COMMANDS)
#define VKVG_CMD_NEW_SUB_PATH       (0x0002 | VKVG_CMD_PATH_COMMANDS)
#define VKVG_CMD_CLOSE_PATH         (0x0003 | VKVG_CMD_PATH_COMMANDS)
#define VKVG_CMD_MOVE_TO            (0x0004 | VKVG_CMD_PATH_COMMANDS)
#define VKVG_CMD_LINE_TO            (0x0005 | VKVG_CMD_PATH_COMMANDS)
#define VKVG_CMD_RECTANGLE          (0x0006 | VKVG_CMD_PATH_COMMANDS)
#define VKVG_CMD_ARC                (0x0007 | VKVG_CMD_PATH_COMMANDS)
#define VKVG_CMD_ARC_NEG            (0x0008 | VKVG_CMD_PATH_COMMANDS)
// #define VKVG_CMD_ELLIPSE			(0x0009|VKVG_CMD_PATH_COMMANDS)
#define VKVG_CMD_CURVE_TO              (0x000A | VKVG_CMD_PATH_COMMANDS)
#define VKVG_CMD_QUADRATIC_TO          (0x000B | VKVG_CMD_PATH_COMMANDS)
#define VKVG_CMD_ELLIPTICAL_ARC_TO     (0x000C | VKVG_CMD_PATH_COMMANDS)

#define VKVG_CMD_SET_LINE_WIDTH        (0x0001 | VKVG_CMD_PATHPROPS_COMMANDS)
#define VKVG_CMD_SET_MITER_LIMIT       (0x0002 | VKVG_CMD_PATHPROPS_COMMANDS)
#define VKVG_CMD_SET_LINE_JOIN         (0x0003 | VKVG_CMD_PATHPROPS_COMMANDS)
#define VKVG_CMD_SET_LINE_CAP          (0x0004 | VKVG_CMD_PATHPROPS_COMMANDS)
#define VKVG_CMD_SET_OPERATOR          (0x0005 | VKVG_CMD_PATHPROPS_COMMANDS)
#define VKVG_CMD_SET_FILL_RULE         (0x0006 | VKVG_CMD_PATHPROPS_COMMANDS)
#define VKVG_CMD_SET_DASH              (0x0007 | VKVG_CMD_PATHPROPS_COMMANDS)

#define VKVG_CMD_TRANSLATE             (0x0001 | VKVG_CMD_TRANSFORM_COMMANDS)
#define VKVG_CMD_ROTATE                (0x0002 | VKVG_CMD_TRANSFORM_COMMANDS)
#define VKVG_CMD_SCALE                 (0x0003 | VKVG_CMD_TRANSFORM_COMMANDS)
#define VKVG_CMD_TRANSFORM             (0x0004 | VKVG_CMD_TRANSFORM_COMMANDS)
#define VKVG_CMD_IDENTITY_MATRIX       (0x0005 | VKVG_CMD_TRANSFORM_COMMANDS)

#define VKVG_CMD_SET_MATRIX            (0x0006 | VKVG_CMD_TRANSFORM_COMMANDS)

#define VKVG_CMD_SET_FONT_SIZE         (0x0001 | VKVG_CMD_TEXT_COMMANDS)
#define VKVG_CMD_SET_FONT_FACE         (0x0002 | VKVG_CMD_TEXT_COMMANDS)
#define VKVG_CMD_SET_FONT_PATH         (0x0003 | VKVG_CMD_TEXT_COMMANDS)
#define VKVG_CMD_SHOW_TEXT             (0x0004 | VKVG_CMD_TEXT_COMMANDS)

#define VKVG_CMD_REL_MOVE_TO           (VKVG_CMD_MOVE_TO | VKVG_CMD_RELATIVE_COMMANDS)
#define VKVG_CMD_REL_LINE_TO           (VKVG_CMD_LINE_TO | VKVG_CMD_RELATIVE_COMMANDS)
#define VKVG_CMD_REL_CURVE_TO          (VKVG_CMD_CURVE_TO | VKVG_CMD_RELATIVE_COMMANDS)
#define VKVG_CMD_REL_QUADRATIC_TO      (VKVG_CMD_QUADRATIC_TO | VKVG_CMD_RELATIVE_COMMANDS)
#define VKVG_CMD_REL_ELLIPTICAL_ARC_TO (VKVG_CMD_ELLIPTICAL_ARC_TO | VKVG_CMD_RELATIVE_COMMANDS)

#define VKVG_CMD_PAINT                 (0x0001 | VKVG_CMD_DRAW_COMMANDS)
#define VKVG_CMD_FILL                  (0x0002 | VKVG_CMD_DRAW_COMMANDS)
#define VKVG_CMD_STROKE                (0x0003 | VKVG_CMD_DRAW_COMMANDS)
#define VKVG_CMD_CLIP                  (0x0004 | VKVG_CMD_DRAW_COMMANDS)
#define VKVG_CMD_RESET_CLIP            (0x0005 | VKVG_CMD_DRAW_COMMANDS)
#define VKVG_CMD_CLEAR                 (0x0006 | VKVG_CMD_DRAW_COMMANDS)

#define VKVG_CMD_FILL_PRESERVE         (VKVG_CMD_FILL | VKVG_CMD_PRESERVE_COMMANDS)
#define VKVG_CMD_STROKE_PRESERVE       (VKVG_CMD_STROKE | VKVG_CMD_PRESERVE_COMMANDS)
#define VKVG_CMD_CLIP_PRESERVE         (VKVG_CMD_CLIP | VKVG_CMD_PRESERVE_COMMANDS)

#define VKVG_CMD_SET_SOURCE_RGB        (0x0001 | VKVG_CMD_PATTERN_COMMANDS)
#define VKVG_CMD_SET_SOURCE_RGBA       (0x0002 | VKVG_CMD_PATTERN_COMMANDS)
#define VKVG_CMD_SET_SOURCE_COLOR      (0x0003 | VKVG_CMD_PATTERN_COMMANDS)
#define VKVG_CMD_SET_SOURCE            (0x0004 | VKVG_CMD_PATTERN_COMMANDS)
#define VKVG_CMD_SET_SOURCE_SURFACE    (0x0005 | VKVG_CMD_PATTERN_COMMANDS)

typedef struct _vkvg_record_t {
	uint16_t cmd;
	size_t   dataOffset;
} vkvg_record_t;

typedef struct _vkvg_recording_t {
	vkvg_record_t* commands;
	uint32_t       commandsCount;
	uint32_t       commandsReservedCount;
	size_t         bufferSize;
	size_t         bufferReservedSize;
	char* buffer;
} vkvg_recording_t;

void              _start_recording(VkvgContext ctx);
vkvg_recording_t* _stop_recording(VkvgContext ctx);
void              _destroy_recording(vkvg_recording_t* rec);
void              _replay_command(VkvgContext ctx, VkvgRecording rec, uint32_t index);
void              _record(vkvg_recording_t* rec, ...);

#define RECORD(ctx, ...)                                                                                               \
    {                                                                                                                  \
        if (ctx->recording) {                                                                                          \
            _record(ctx->recording, __VA_ARGS__);                                                                      \
            return;                                                                                                    \
        }                                                                                                              \
    }
#define RECORD2(ctx, ...)                                                                                              \
    {                                                                                                                  \
        if (ctx->recording) {                                                                                          \
            _record(ctx->recording, __VA_ARGS__);                                                                      \
            return 0;                                                                                                  \
        }                                                                                                              \
    }

#else
#define RECORD(ctx, cmd, ...)
#define RECORD2(ctx, cmd, ...)
#endif


// disable warning on iostream functions on windows
#define _CRT_SECURE_NO_WARNINGS

#ifdef VKVG_USE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#if defined(VKVG_LCD_FONT_FILTER) && defined(FT_CONFIG_OPTION_SUBPIXEL_RENDERING)
#include <freetype/ftlcdfil.h>
#endif
#define FT_CHECK_RESULT(f)                                                                                             \
    {                                                                                                                  \
        FT_Error res = (f);                                                                                            \
        if (res != 0) {                                                                                                \
            fprintf(stderr, "Fatal : FreeType error is %d in %s at line %d\n", res, __FILE__, __LINE__);               \
            assert(res == 0);                                                                                          \
        }                                                                                                              \
    }
#else
#include "stb_truetype.h"
#endif

#ifdef VKVG_USE_HARFBUZZ
#include <hb.h>
//#include <hb-ft.h>
#else
#endif

#ifdef VKVG_USE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

#define FONT_PAGE_SIZE          1024
#define FONT_CACHE_INIT_LAYERS  1
#define FONT_FILE_NAME_MAX_SIZE 1024

//#include "vkvg_internal.h"
#include "vkh_buffer.h"

// texture coordinates of one character in font cache array texture.
typedef struct {
	vec4    bounds;  /* normalized float bounds of character bitmap in font cache texture. */
	vec2i16 bmpDiff; /* Difference in pixel between char bitmap top left corner and char glyph*/
	uint8_t pageIdx; /* Page index in font cache texture array */
#ifdef VKVG_USE_FREETYPE
	FT_Vector advance; /* horizontal or vertical advance */
#else
	vec2               advance;
#endif
} _char_ref;

// Current location in font cache texture array for new character addition. Each font holds such structure to locate
// where to upload new chars.
typedef struct {
	uint8_t pageIdx; /* Current page number in font cache */
	int     penX;    /* Current X in cache for next char addition */
	int     penY;    /* Current Y in cache for next char addition */
	int     height;  /* Height of current line pointed by this structure */
} _tex_ref_t;

// Loaded font structure, one per size, holds informations for glyphes upload in cache and the lookup table of
// characters.
typedef struct {
#ifdef VKVG_USE_FREETYPE
	FT_F26Dot6 charSize; /* Font size*/
	FT_Face    face;     /* FreeType face*/
#else
	uint32_t           charSize; /* Font size in pixel */
	float              scale;    /* scale factor for the given size */
	int                ascent;   /* unscalled stb font metrics */
	int                descent;
	int                lineGap;
#endif

#ifdef VKVG_USE_HARFBUZZ
	hb_font_t* hb_font; /* HarfBuzz font instance*/
#endif
	_char_ref** charLookup; /* Lookup table of characteres in cache, if not found, upload is queued*/

	_tex_ref_t curLine; /* tex coord where to add new char bmp's */
} _vkvg_font_t;

/* Font identification structure */
typedef struct {
	char** names; /* Resolved Input names to this font by fontConfig or custom name set by @ref vkvg_load_from_path*/
	uint32_t namesCount;        /* Count of resolved names by fontConfig */
	unsigned char* fontBuffer;  /* stb_truetype in memory buffer */
	long           fontBufSize; /* */
	char* fontFile;    /* Font file full path*/
#ifndef VKVG_USE_FREETYPE
	stbtt_fontinfo stbInfo; /* stb_truetype structure */
	int            ascent;  /* unscalled stb font metrics */
	int            descent;
	int            lineGap;
#endif
	uint32_t      sizeCount; /* available font size loaded */
	_vkvg_font_t* sizes;     /* loaded font size array */
} _vkvg_font_identity_t;

// Font cache global structure, entry point for all font related operations.
struct _font_cache_t {
#ifdef VKVG_USE_FREETYPE
	FT_Library library; /* FreeType library*/
#else
#endif
#ifdef VKVG_USE_FONTCONFIG
	FcConfig* config; /* Font config, used to find font files by font names*/
#endif

	int      stagingX; /* x pen in host buffer */
	uint8_t* hostBuff; /* host memory where bitmaps are first loaded */

	VkCommandBuffer cmd;          /* vulkan command buffer for font textures upload */
	vkh_buffer_t    buff;         /* stagin buffer */
	VkhImage        texture;      /* 2d array texture used by contexts to draw characteres */
	VkFormat        texFormat;    /* Format of the fonts texture array */
	uint8_t         texPixelSize; /* Size in byte of a single pixel in a font texture */
	uint8_t texLength;   /* layer count of 2d array texture, starts with FONT_CACHE_INIT_LAYERS count and increased when
							needed */
	int* pensY;       /* array of current y pen positions for each texture in cache 2d array */
	VkFence uploadFence; /* Signaled when upload is finished */
	mtx_t   mutex;       /* font cache global mutex, used only if device is in thread aware mode (see:
							vkvg_device_set_thread_aware) */

	_vkvg_font_identity_t* fonts;      /* Loaded fonts structure array */
	int32_t                fontsCount; /* Loaded fonts array count*/
};

#define LOCK_FONTCACHE(dev)                                                                                            \
    if (dev->threadAware)                                                                                              \
        mtx_lock(&dev->fontCache->mutex);
#define UNLOCK_FONTCACHE(dev)                                                                                          \
    if (dev->threadAware)                                                                                              \
        mtx_unlock(&dev->fontCache->mutex);

// Precompute everything necessary to measure and draw one line of text, usefull to draw the same text multiple times.
typedef struct _vkvg_text_run_t {
	_vkvg_font_identity_t* fontId;      /* vkvg font structure pointer */
	_vkvg_font_t* font;        /* vkvg font structure pointer */
	VkvgDevice             dev;         /* vkvg device associated with this text run */
	vkvg_text_extents_t    extents;     /* store computed text extends */
	const char* text;        /* utf8 char array of text*/
	unsigned int           glyph_count; /* Total glyph count */
#ifdef VKVG_USE_HARFBUZZ
	hb_buffer_t* hbBuf;  /* HarfBuzz buffer of text */
	hb_glyph_position_t* glyphs; /* HarfBuzz computed glyph positions array */
#else
	vkvg_glyph_info_t* glyphs; /* computed glyph positions array */
#endif
} vkvg_text_run_t;

// Create font cache.
void _fonts_cache_create(VkvgDevice dev);
// Release all ressources of font cache.
void                   _font_cache_destroy(VkvgDevice dev);
_vkvg_font_identity_t* _font_cache_add_font_identity(VkvgContext ctx, const char* fontFile, const char* name);
bool                   _font_cache_load_font_file_in_memory(_vkvg_font_identity_t* fontId);
// Draw text
void _font_cache_show_text(VkvgContext ctx, const char* text);
// Get text dimmensions
void _font_cache_text_extents(VkvgContext ctx, const char* text, int length, vkvg_text_extents_t* extents);
// Get font global dimmensions
void _font_cache_font_extents(VkvgContext ctx, vkvg_font_extents_t* extents);
// Create text object that could be drawn multiple times minimizing harfbuzz and compute processing.
void _font_cache_create_text_run(VkvgContext ctx, const char* text, int length, VkvgText textRun);
// Release ressources held by a text run.
void _font_cache_destroy_text_run(VkvgText textRun);
// Draw text run
void _font_cache_show_text_run(VkvgContext ctx, VkvgText tr);
// update context font cache descriptor set
void _font_cache_update_context_descset(VkvgContext ctx);


#define VKVG_PTS_SIZE        1024
#define VKVG_VBO_SIZE        (VKVG_PTS_SIZE * 4)
#define VKVG_IBO_SIZE        (VKVG_VBO_SIZE * 6)
#define VKVG_PATHES_SIZE     16
#define VKVG_ARRAY_THRESHOLD 8

#define VKVG_IBO_16          0
#define VKVG_IBO_32          1

#define VKVG_CUR_IBO_TYPE    VKVG_IBO_32 // change this only

#if VKVG_CUR_IBO_TYPE == VKVG_IBO_16
#define VKVG_IBO_MAX        UINT16_MAX
#define VKVG_IBO_INDEX_TYPE uint16_t
#define VKVG_VK_INDEX_TYPE  VK_INDEX_TYPE_UINT16
#else
#define VKVG_IBO_MAX        UINT32_MAX
#define VKVG_IBO_INDEX_TYPE uint32_t
#define VKVG_VK_INDEX_TYPE  VK_INDEX_TYPE_UINT32
#endif

#define FULLSCREEN_BIT         0x10000000
#define SRCTYPE_MASK           0x000000FF

#define CreateRgba(r, g, b, a) (((a&0xFF) << 24) | ((r&0xFF) << 16) | ((g&0xFF) << 8) | b)
#ifdef VKVG_PREMULT_ALPHA
#define CreateRgbaf(r, g, b, a)                                                                                        \
    ((((uint32_t)(a * 255.0f)&0xFF) << 24) | (((uint32_t)(b * a * 255.0f)&0xFF) << 16) | (((uint32_t)(g * a * 255.0f)&0xFF) << 8) | ((uint32_t)(r * a * 255.0f)&0xFF))
#else
#define CreateRgbaf(r, g, b, a)                                                                                        \
    (((int)(a * 255.0f) << 24) | ((int)(b * 255.0f) << 16) | ((int)(g * 255.0f) << 8) | (int)(r * 255.0f))
#endif

typedef struct Vertex_c {
	vec2     pos;
	uint32_t color;
	vec3     uv;
} Vertex;

typedef struct push_constants {
	vec4          source;
	vec2          size;
	uint32_t      fsq_patternType;
	float         opacity;
	vkvg_matrix_t mat;
	vkvg_matrix_t matInv;
} push_constants;

/* context.curClipState may be one of the following, it's set
 * with check of the previous saved state:
 * - none: no clipping operation since the previous state
 * - clear: current has cleared the clip, or previous state is also clear.
 * - clip: current have been clipped with a new region since the last save.
 *
 * the saved context may have following savedState:
 * - clear: no clip
 * - clip: context is clipped, but not at this save/restore level, no stencil is saved at that level
 * - clip_saved: context is clipped and the clip region is saved at that level.
 */
typedef enum {
	vkvg_clip_state_none = 0x00,
	vkvg_clip_state_clear = 0x01,
	vkvg_clip_state_clip = 0x02,
	vkvg_clip_state_clip_saved = 0x06,
} vkvg_clip_state_t;

typedef struct _vkvg_context_save_t {
	struct _vkvg_context_save_t* pNext;

	float    lineWidth;
	float    miterLimit;
	uint32_t dashCount;  // value count in dash array, 0 if dash not set.
	float    dashOffset; // an offset for dash
	float* dashes;     // an array of alternate lengths of on and off stroke.

	vkvg_operator_t  curOperator;
	vkvg_line_cap_t  lineCap;
	vkvg_line_join_t lineJoin;
	vkvg_fill_rule_t curFillRule;

	long                   selectedCharSize; /* Font size*/
	char                   selectedFontName[FONT_NAME_MAX_SIZE];
	_vkvg_font_identity_t  selectedFont; // hold current face and size before cache addition
	_vkvg_font_identity_t* currentFont;  // font ready for lookup
	vkvg_direction_t       textDirection;
	push_constants         pushConsts;
	uint32_t               curColor;
	VkvgPattern            pattern;
	vkvg_clip_state_t      clippingState;

} vkvg_context_save_t;

typedef struct _ear_clip_point {
	vec2                    pos;
	VKVG_IBO_INDEX_TYPE     idx;
	struct _ear_clip_point* next;
} ear_clip_point;

typedef struct _vkvg_context_t {
	vkvg_status_t status;
	uint32_t      references; // reference count

	VkvgDevice  dev;
	VkvgSurface pSurf; // surface bound to context, set on creation of ctx
#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	uint64_t timelineStep; // context cmd last submission timeline id.
#else
	VkFence flushFence; // context fence
#endif
	// VkDescriptorImageInfo sourceDescriptor;	//Store view/sampler in context

	VkCommandPool    cmdPool;        // local pools ensure thread safety
	VkCommandBuffer  cmdBuffers[2];  // double cmd buff for context operations
	VkCommandBuffer  cmd;            // current recording buffer
	VkDescriptorPool descriptorPool; // one pool per thread
	VkDescriptorSet  dsFont;         // fonts glyphs texture atlas descriptor (local for thread safety)
	VkDescriptorSet  dsSrc;          // source ds
	VkDescriptorSet  dsGrad;         // gradient uniform buffer

	VkhImage fontCacheImg; // current font cache, may not be the last one, updated only if new glyphs are
	// uploaded by the current context

	VkRect2D bounds;

	uint32_t curColor;

#if VKVG_FILL_NZ_GLUTESS
	void (*vertex_cb)(VKVG_IBO_INDEX_TYPE, VkvgContext); // tesselator vertex callback
	VKVG_IBO_INDEX_TYPE tesselator_fan_start;
	uint32_t            tesselator_idx_counter;
#endif

#if VKVG_RECORDING
	vkvg_recording_t* recording;
#endif

	vkh_buffer_t uboGrad; // uniform buff obj holdings gradient infos

	// vk buffers, holds data until flush
	vkh_buffer_t indices;     // index buffer with persistent map memory
	uint32_t     sizeIBO;     // size of vk ibo
	uint32_t     sizeIndices; // reserved size
	uint32_t     indCount;    // current indice count

	uint32_t            curIndStart;   // last index recorded in cmd buff
	VKVG_IBO_INDEX_TYPE curVertOffset; // vertex offset in draw indexed command

	vkh_buffer_t vertices;     // vertex buffer with persistent mapped memory
	uint32_t     sizeVBO;      // size of vk vbo size
	uint32_t     sizeVertices; // reserved size
	uint32_t     vertCount;    // effective vertices count
	uint32_t     gCount;    // 渐变ubo数量
	uint32_t     gxCount;    // ubo计数

	VkhImage d_img;
	uint64_t d_offset;
	bool isdset;

	Vertex* vertexCache;
	VKVG_IBO_INDEX_TYPE* indexCache;

	// pathes, exists until stroke of fill
	vec2* points;     // points array
	uint32_t sizePoints; // reserved size
	uint32_t pointCount; // effective points count

	// pathes array is a list of point count per segment
	uint32_t  pathPtr; // pointer in the path array
	uint32_t* pathes;
	uint32_t  sizePathes;

	uint32_t segmentPtr;   // current segment count in current path having curves
	uint32_t subpathCount; // store count of subpath, not straight forward to retrieve from segmented path array
	bool     simpleConvex; // true if path is single rect or concave closed curve.

	bool     cmdStarted;     // prevent flushing empty renderpass
	bool     pushCstDirty;   // prevent pushing to gpu if not requested

	float    lineWidth;
	float    miterLimit;
	uint32_t dashCount;  // value count in dash array, 0 if dash not set.
	float    dashOffset; // an offset for dash
	float* dashes;     // an array of alternate lengths of on and off stroke.

	vkvg_operator_t  curOperator;
	vkvg_line_cap_t  lineCap;
	vkvg_line_join_t lineJoin;
	vkvg_fill_rule_t curFillRule;

	long selectedCharSize; /* Font size*/
	char selectedFontName[FONT_NAME_MAX_SIZE];
	//_vkvg_font_t		  selectedFont;		//hold current face and size before cache addition
	_vkvg_font_identity_t* currentFont;     // font pointing to cached fonts identity
	_vkvg_font_t* currentFontSize; // font structure by size ready for lookup
	vkvg_direction_t       textDirection;

	push_constants pushConsts;
	VkvgPattern    pattern;

	vkvg_context_save_t* pSavedCtxs;   // last ctx saved ptr
	uint8_t              curSavBit;    // current stencil bit used to save context, 6 bits used by stencil for save/restore
	VkhImage* savedStencils;// additional image for saving contexes once more than 6 save/restore are reached
	vkvg_clip_state_t    curClipState; // current clipping status relative to the previous saved one or clear state if
	// none.

	VkClearRect           clearRect;
	VkRenderPassBeginInfo renderPassBeginInfo;

	PFN_vkCmdPushDescriptorSet _vkCmdPushDescriptorSet = {};
	uint32_t maxPushDescriptors = 0;

	uint32_t capPathPointCount = 0;
	ear_clip_point* ecps = 0;
	//vgdev_ctx* pathCtx = 0;
	bool fill_rule_winding = false;
} vkvg_context;


typedef struct {
	bool     dashOn;
	uint32_t curDash;       // current dash index
	float    curDashOffset; // cur dash offset between defined path point and last dash segment(on/off) start
	float    totDashLength; // total length of dashes
	vec2     normal;
} dash_context_t;

typedef struct {
	uint32_t iL;
	uint32_t iR;
	uint32_t cp; // current point

	VKVG_IBO_INDEX_TYPE firstIdx; // save first point idx for closed path
	float               hw;       // stroke half width, computed once.
	float               lhMax;    // miter limit * line width
	float arcStep; // cached arcStep, prevent compute multiple times for same stroke, 0 if not yet computed
} stroke_context_t;

void _check_vertex_cache_size(VkvgContext ctx);
void _ensure_vertex_cache_size(VkvgContext ctx, uint32_t addedVerticesCount);
void _resize_vertex_cache(VkvgContext ctx, uint32_t newSize);

void _check_index_cache_size(VkvgContext ctx);
void _ensure_index_cache_size(VkvgContext ctx, uint32_t addedIndicesCount);
void _resize_index_cache(VkvgContext ctx, uint32_t newSize);

bool _check_pathes_array(VkvgContext ctx);

bool _current_path_is_empty(VkvgContext ctx);
void _finish_path(VkvgContext ctx);
void _clear_path(VkvgContext ctx);
void _remove_last_point(VkvgContext ctx);
bool _path_is_closed(VkvgContext ctx, uint32_t ptrPath);
void _set_curve_start(VkvgContext ctx);
void _set_curve_end(VkvgContext ctx);
bool _path_has_curves(VkvgContext ctx, uint32_t ptrPath);

float _normalizeAngle(float a);
float _get_arc_step(VkvgContext ctx, float radius);

vec2 _get_current_position(VkvgContext ctx);
void _add_point(VkvgContext ctx, float x, float y);

void  _resetMinMax(VkvgContext ctx);
void  _vkvg_path_extents(VkvgContext ctx, bool transformed, float* x1, float* y1, float* x2, float* y2);
void  _draw_stoke_cap(VkvgContext ctx, stroke_context_t* str, vec2 p0, vec2 n, bool isStart);
void  _draw_segment(VkvgContext ctx, stroke_context_t* str, dash_context_t* dc, bool isCurve);
float _draw_dashed_segment(VkvgContext ctx, stroke_context_t* str, dash_context_t* dc, bool isCurve);
bool  _build_vb_step(VkvgContext ctx, stroke_context_t* str, bool isCurve);

void _poly_fill(VkvgContext ctx, vec4* bounds);
void _fill_non_zero(VkvgContext ctx);
void _draw_full_screen_quad(VkvgContext ctx, vec4* scissor);

void _create_gradient_buff(VkvgContext ctx);
void _create_vertices_buff(VkvgContext ctx);
void _add_vertex(VkvgContext ctx, Vertex v);
void _add_vertexf(VkvgContext ctx, float x, float y);
void _set_vertex(VkvgContext ctx, uint32_t idx, Vertex v);
void _add_triangle_indices(VkvgContext ctx, VKVG_IBO_INDEX_TYPE i0, VKVG_IBO_INDEX_TYPE i1, VKVG_IBO_INDEX_TYPE i2);
void _add_tri_indices_for_rect(VkvgContext ctx, VKVG_IBO_INDEX_TYPE i);

void _vao_add_rectangle(VkvgContext ctx, float x, float y, float width, float height);

void _bind_draw_pipeline(VkvgContext ctx);
void _create_cmd_buff(VkvgContext ctx);
void _check_vao_size(VkvgContext ctx);
void _flush_cmd_buff(VkvgContext ctx);
void _ensure_renderpass_is_started(VkvgContext ctx);
void _emit_draw_cmd_undrawn_vertices(VkvgContext ctx);
void _flush_cmd_until_vx_base(VkvgContext ctx);
bool _wait_ctx_flush_end(VkvgContext ctx);
bool _wait_and_submit_cmd(VkvgContext ctx);
void _update_push_constants(VkvgContext ctx);
void _update_cur_pattern(VkvgContext ctx, VkvgPattern pat);
void _set_mat_inv_and_vkCmdPush(VkvgContext ctx);
void _start_cmd_for_render_pass(VkvgContext ctx);

void _createDescriptorPool(VkvgContext ctx);
void _init_descriptor_sets(VkvgContext ctx);
void _update_descriptor_set(VkvgContext ctx, VkhImage img, VkDescriptorSet ds);
void _update_gradient_desc_set(VkvgContext ctx);
void _free_ctx_save(vkvg_context_save_t* sav);
void _release_context_ressources(VkvgContext ctx);

static inline float vec2_zcross(vec2 v1, vec2 v2) { return v1.x * v2.y - v1.y * v2.x; }
static inline float ecp_zcross(ear_clip_point* p0, ear_clip_point* p1, ear_clip_point* p2) {
	return vec2_zcross(vec2_sub(p1->pos, p0->pos), vec2_sub(p2->pos, p0->pos));
}
void _recursive_bezier(VkvgContext ctx, float distanceTolerance, float x1, float y1, float x2, float y2, float x3,
	float y3, float x4, float y4, unsigned level);
void _bezier(VkvgContext ctx, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
void _line_to(VkvgContext ctx, float x, float y);
void _elliptic_arc(VkvgContext ctx, float x1, float y1, float x2, float y2, bool largeArc, bool counterClockWise,
	float _rx, float _ry, float phi);

void _select_font_face(VkvgContext ctx, const char* name);


typedef struct _vkvg_surface_t {
	vkvg_status_t   status; /**< Current status of surface, affected by last operation */
	uint32_t        references;
	VkvgDevice      dev;
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
#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	VkSemaphore timeline; /**< Timeline semaphore */
	uint64_t    timelineStep;
#else
	VkFence flushFence; // unsignaled idle.
#endif
	VkSemaphore sem;
	VkSemaphore sem0;
} vkvg_surface;

#define LOCK_SURFACE(surf)                                                                                             \
    if (surf->dev->threadAware) {                                                                                      \
        mtx_lock(&surf->mutex);                                                                                        \
    }
#define UNLOCK_SURFACE(surf)                                                                                           \
    if (surf->dev->threadAware) {                                                                                      \
        mtx_unlock(&surf->mutex);                                                                                      \
    }

void        _explicit_ms_resolve(VkvgSurface surf);
void        _transition_surf_images(VkvgSurface surf);
void        _clear_surface(VkvgSurface surf, VkImageAspectFlags aspect);
void        _create_surface_main_image(VkvgSurface surf);
void        _create_surface_secondary_images(VkvgSurface surf);
void        _create_framebuffer(VkvgSurface surf);
void        _create_surface_images(VkvgSurface surf);
VkvgSurface _create_surface(VkvgDevice dev, VkFormat format);

void _surface_submit_cmd(VkvgSurface surf);

typedef struct _vkvg_pattern_t {
	vkvg_status_t       status;
	uint32_t            references;
	vkvg_pattern_type_t type;
	vkvg_extend_t       extend;
	vkvg_filter_t       filter;
	vkvg_matrix_t       matrix;
	bool                hasMatrix;
	void* data;
} vkvg_pattern_t;
#if 0
typedef struct _vkvg_gradient_t {
	vkvg_color_t colors[16];
#ifdef VKVG_ENABLE_VK_SCALAR_BLOCK_LAYOUT
	float stops[16];
#else
	vec4 stops[16];
#endif
	vec4     cp[2];
	uint32_t count;
} vkvg_gradient_t;
#endif
#define MAX_STOPS 32
typedef struct vkvg_gradient_t {
	vkvg_color_t colors[MAX_STOPS];
	float stops[MAX_STOPS];
	vec4 cp[2];
	ivec4 m;
	vec2 scale;	// 缩放目标
	uint32_t count;
	int extend;
};


struct state_save_t {
	float		lineWidth;
	float		miterLimit;
	uint32_t	dashCount;  // value count in dash array, 0 if dash not set.
	float		dashOffset; // an offset for dash
	float* dashes;     // an array of alternate lengths of on and off stroke.
	vkvg_operator_t		curOperator;
	vkvg_line_cap_t		lineCap;
	vkvg_line_join_t	lineJoin;
	vkvg_fill_rule_t	curFillRule;
	push_constants		pushConsts;
	uint32_t			curColor;
	VkvgPattern			pattern;
	vkvg_clip_state_t	clippingState;
	int					clip_idx = -1;		// 裁剪的命令索引,小于0无
	uint32_t			references = 1;
	bool aa = true;
	bool afree = false;
};

