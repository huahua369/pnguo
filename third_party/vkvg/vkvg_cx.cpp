
//#define VKVG_WIRED_DEBUG
#define VKVG_USE_HARFBUZZ

#include <pch1.h>
#include <ntype.h>
class vgdev_ctx;
#ifdef __cplusplus
extern "C" {
#endif
	//#define VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
#include "vkvg_internal0.h" 

#ifdef __cplusplus
}
#endif 
#include "vkh_queue.h"
#include "vkh_image.h"

#ifdef VKVG_FILL_NZ_GLUTESS
#include <glutess.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include "vkh_phyinfo.h"
#include "vk_mem_alloc.h"
#include "shaders.h"
#include "vkvg_matrix.h"
#include "vkh.h"
//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <locale.h>
#include <string.h>
#include <wchar.h>

#ifndef VKVG_USE_FREETYPE
 //#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

#include <array> 
#include <pnguo/print_time.h> 
#include <vg.h> 
#include <clipper2/clipper.h> 

#include <font_core.h>
#include "vkvg_cx.h"
/*
命令行
glslangValidator vkvg_main0.frag.h -DVKVG_PREMULT_ALPHA -S frag -V --vn vkvg_main_frag1_spv -o vkvg_main.frag.h


*/

#define PWAIT_MS



#include <stdlib.h>

/* Platform specific includes */
#if defined(_TTHREAD_POSIX_)
#include <signal.h>
#include <sched.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#elif defined(_TTHREAD_WIN32_)
#include <process.h>
#include <sys/timeb.h>
#endif

/* Standard, good-to-have defines */
#ifndef NULL
#define NULL (void *)0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


void _device_submit_cmd_sem(VkvgDevice dev, VkCommandBuffer* cmd, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence fence);


class vgdev_ctx
{
public:
	USP_CX ac;						// 
	hz::mbpool_t mac;				// 帧内存池
	// 输出
	vg_vector<Vertex> _vertex;
	vg_vector<uint32_t> _indices;
	struct scmd {
		uint32_t vertexCount;
		uint32_t firstVertex;
	};
	struct cmd_t {
		scmd* v = 0;
		int vc = 0;
		int full_screen_quad = 0;
		ivec2 vertex = {};			// 顶点开始、数量
		ivec2 index = {};			// 索引开始、数量
		state_save_t* state = {};	// 渲染参数
		vec4 bounds = {};			// 全屏填充,odd/clip专用
		int8_t type = 0;			// 类型：填充0、描边1、裁剪2
	};
	t_vector<cmd_t> cmdlist;		// 命令
	// 临时缓冲用
	vg_vector<ear_clip_point> ecpsd;
	vg_vector<vec2> _normals;
#if VKVG_FILL_NZ_GLUTESS
	void (*vertex_cb)(uint32_t, vgdev_ctx*) = 0; // tesselator vertex callback
	uint32_t tesselator_fan_start = 0;
	uint32_t tesselator_idx_counter = 0;
#endif
	uint32_t curColor = 0xFFffffff;
	uint32_t curVertOffset = 0;
	uint32_t gCount = 0;		// ubo数量
	state_save_t* t = 0;
	paths_t* _dpath = 0;		// 默认路径缓存
	bool is_glutess = false;
	bool is_fence = true;
public:
	vgdev_ctx();
	~vgdev_ctx();

	void clip0();
	void clip_preserve(paths_t* ctx);
	void fill_preserve(paths_t* p);
	void stroke_preserve(paths_t* p);
	void draw(VkvgContext ctx, void* waitSemaphore);
	void begin_frame();
	void end_frame();
	void set_glutess(bool b);
	paths_t* get_path();
private:
	void poly_fill(paths_t* ctx, vec4* bounds, cmd_t& c);
	void _fill_non_zero(paths_t*);
	void fill_non_zero(paths_t* p);
	bool _build_vb_step(paths_t* ctx, stroke_context_t* str, bool isCurve);
	void _draw_stoke_cap(paths_t* ctx, stroke_context_t* str, vec2 p0, vec2 n, bool isStart);
	float _draw_dashed_segment(paths_t* ctx, stroke_context_t* str, dash_context_t* dc, bool isCurve);
	void _draw_segment(paths_t* ctx, stroke_context_t* str, dash_context_t* dc, bool isCurve);

	void a_add_triangle_indices(paths_t* ctx, uint32_t i0, uint32_t i1, uint32_t i2);
	void a_add_tri_indices_for_rect(uint32_t i);
	void _add_vertexf(paths_t* ctx, float x, float y);
	void cp_cmdt(cmd_t* c, state_save_t* t);
};



state_save_t* dc_new_state(vgdev_ctx* ctx);



#if 1
int mtx_init(mtx_t* mtx, int type) {
#if defined(_TTHREAD_WIN32_)
	mtx->mAlreadyLocked = FALSE;
	mtx->mRecursive = type & mtx_recursive;
	InitializeCriticalSection(&mtx->mHandle);
	return thrd_success;
#else
	int                 ret;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	if (type & mtx_recursive) {
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	}
	ret = pthread_mutex_init(mtx, &attr);
	pthread_mutexattr_destroy(&attr);
	return ret == 0 ? thrd_success : thrd_error;
#endif
}

void mtx_destroy(mtx_t* mtx) {
#if defined(_TTHREAD_WIN32_)
	DeleteCriticalSection(&mtx->mHandle);
#else
	pthread_mutex_destroy(mtx);
#endif
}

int mtx_lock(mtx_t* mtx) {
#if defined(_TTHREAD_WIN32_)
	EnterCriticalSection(&mtx->mHandle);
	if (!mtx->mRecursive) {
		while (mtx->mAlreadyLocked)
			Sleep(1000); /* Simulate deadlock... */
		mtx->mAlreadyLocked = TRUE;
	}
	return thrd_success;
#else
	return pthread_mutex_lock(mtx) == 0 ? thrd_success : thrd_error;
#endif
}

int mtx_timedlock(mtx_t* mtx, const struct timespec* ts) {
	/* FIXME! */
	(void)mtx;
	(void)ts;
	return thrd_error;
}

int mtx_trylock(mtx_t* mtx) {
#if defined(_TTHREAD_WIN32_)
	int ret = TryEnterCriticalSection(&mtx->mHandle) ? thrd_success : thrd_busy;
	if ((!mtx->mRecursive) && (ret == thrd_success) && mtx->mAlreadyLocked) {
		LeaveCriticalSection(&mtx->mHandle);
		ret = thrd_busy;
	}
	return ret;
#else
	return (pthread_mutex_trylock(mtx) == 0) ? thrd_success : thrd_busy;
#endif
}

int mtx_unlock(mtx_t* mtx) {
#if defined(_TTHREAD_WIN32_)
	mtx->mAlreadyLocked = FALSE;
	LeaveCriticalSection(&mtx->mHandle);
	return thrd_success;
#else
	return pthread_mutex_unlock(mtx) == 0 ? thrd_success : thrd_error;
	;
#endif
}

#if defined(_TTHREAD_WIN32_)
#define _CONDITION_EVENT_ONE 0
#define _CONDITION_EVENT_ALL 1
#endif

int cnd_init(cnd_t* cond) {
#if defined(_TTHREAD_WIN32_)
	cond->mWaitersCount = 0;

	/* Init critical section */
	InitializeCriticalSection(&cond->mWaitersCountLock);

	/* Init events */
	cond->mEvents[_CONDITION_EVENT_ONE] = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (cond->mEvents[_CONDITION_EVENT_ONE] == NULL) {
		cond->mEvents[_CONDITION_EVENT_ALL] = NULL;
		return thrd_error;
	}
	cond->mEvents[_CONDITION_EVENT_ALL] = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (cond->mEvents[_CONDITION_EVENT_ALL] == NULL) {
		CloseHandle(cond->mEvents[_CONDITION_EVENT_ONE]);
		cond->mEvents[_CONDITION_EVENT_ONE] = NULL;
		return thrd_error;
	}

	return thrd_success;
#else
	return pthread_cond_init(cond, NULL) == 0 ? thrd_success : thrd_error;
#endif
}

void cnd_destroy(cnd_t* cond) {
#if defined(_TTHREAD_WIN32_)
	if (cond->mEvents[_CONDITION_EVENT_ONE] != NULL) {
		CloseHandle(cond->mEvents[_CONDITION_EVENT_ONE]);
	}
	if (cond->mEvents[_CONDITION_EVENT_ALL] != NULL) {
		CloseHandle(cond->mEvents[_CONDITION_EVENT_ALL]);
	}
	DeleteCriticalSection(&cond->mWaitersCountLock);
#else
	pthread_cond_destroy(cond);
#endif
}

int cnd_signal(cnd_t* cond) {
#if defined(_TTHREAD_WIN32_)
	int haveWaiters;

	/* Are there any waiters? */
	EnterCriticalSection(&cond->mWaitersCountLock);
	haveWaiters = (cond->mWaitersCount > 0);
	LeaveCriticalSection(&cond->mWaitersCountLock);

	/* If we have any waiting threads, send them a signal */
	if (haveWaiters) {
		if (SetEvent(cond->mEvents[_CONDITION_EVENT_ONE]) == 0) {
			return thrd_error;
		}
	}

	return thrd_success;
#else
	return pthread_cond_signal(cond) == 0 ? thrd_success : thrd_error;
#endif
}

int cnd_broadcast(cnd_t* cond) {
#if defined(_TTHREAD_WIN32_)
	int haveWaiters;

	/* Are there any waiters? */
	EnterCriticalSection(&cond->mWaitersCountLock);
	haveWaiters = (cond->mWaitersCount > 0);
	LeaveCriticalSection(&cond->mWaitersCountLock);

	/* If we have any waiting threads, send them a signal */
	if (haveWaiters) {
		if (SetEvent(cond->mEvents[_CONDITION_EVENT_ALL]) == 0) {
			return thrd_error;
		}
	}

	return thrd_success;
#else
	return pthread_cond_signal(cond) == 0 ? thrd_success : thrd_error;
#endif
}

#if defined(_TTHREAD_WIN32_)
static int _cnd_timedwait_win32(cnd_t* cond, mtx_t* mtx, DWORD timeout) {
	int result, lastWaiter;

	/* Increment number of waiters */
	EnterCriticalSection(&cond->mWaitersCountLock);
	++cond->mWaitersCount;
	LeaveCriticalSection(&cond->mWaitersCountLock);

	/* Release the mutex while waiting for the condition (will decrease
	   the number of waiters when done)... */
	mtx_unlock(mtx);

	/* Wait for either event to become signaled due to cnd_signal() or
	   cnd_broadcast() being called */
	result = WaitForMultipleObjects(2, cond->mEvents, FALSE, timeout);
	if (result == WAIT_TIMEOUT) {
		return thrd_timeout;
	}
	else if (result == (int)WAIT_FAILED) {
		return thrd_error;
	}

	/* Check if we are the last waiter */
	EnterCriticalSection(&cond->mWaitersCountLock);
	--cond->mWaitersCount;
	lastWaiter = (result == (WAIT_OBJECT_0 + _CONDITION_EVENT_ALL)) && (cond->mWaitersCount == 0);
	LeaveCriticalSection(&cond->mWaitersCountLock);

	/* If we are the last waiter to be notified to stop waiting, reset the event */
	if (lastWaiter) {
		if (ResetEvent(cond->mEvents[_CONDITION_EVENT_ALL]) == 0) {
			return thrd_error;
		}
	}

	/* Re-acquire the mutex */
	mtx_lock(mtx);

	return thrd_success;
}
#endif

int cnd_wait(cnd_t* cond, mtx_t* mtx) {
#if defined(_TTHREAD_WIN32_)
	return _cnd_timedwait_win32(cond, mtx, INFINITE);
#else
	return pthread_cond_wait(cond, mtx) == 0 ? thrd_success : thrd_error;
#endif
}

int cnd_timedwait(cnd_t* cond, mtx_t* mtx, const struct timespec* ts) {
#if defined(_TTHREAD_WIN32_)
	struct timespec now;
	if (clock_gettime(CLOCK_REALTIME, &now) == 0) {
		DWORD delta = (DWORD)((ts->tv_sec - now.tv_sec) * 1000 + (ts->tv_nsec - now.tv_nsec + 500000) / 1000000);
		return _cnd_timedwait_win32(cond, mtx, delta);
	}
	else
		return thrd_error;
#else
	int ret;
	ret = pthread_cond_timedwait(cond, mtx, ts);
	if (ret == ETIMEDOUT) {
		return thrd_timeout;
	}
	return ret == 0 ? thrd_success : thrd_error;
#endif
}

/** Information to pass to the new thread (what to run). */
typedef struct {
	thrd_start_t mFunction; /**< Pointer to the function to be executed. */
	void* mArg;      /**< Function argument for the thread function. */
} _thread_start_info;

/* Thread wrapper function. */
#if defined(_TTHREAD_WIN32_)
static unsigned WINAPI _thrd_wrapper_function(void* aArg)
#elif defined(_TTHREAD_POSIX_)
static void* _thrd_wrapper_function(void* aArg)
#endif
{
	thrd_start_t fun;
	void* arg;
	int          res;
#if defined(_TTHREAD_POSIX_)
	void* pres;
#endif

	/* Get thread startup information */
	_thread_start_info* ti = (_thread_start_info*)aArg;
	fun = ti->mFunction;
	arg = ti->mArg;

	/* The thread is responsible for freeing the startup information */
	free((void*)ti);

	/* Call the actual client thread function */
	res = fun(arg);

#if defined(_TTHREAD_WIN32_)
	return res;
#else
	pres = malloc(sizeof(int));
	if (pres != NULL) {
		*(int*)pres = res;
	}
	return pres;
#endif
}

int thrd_create(thrd_t* thr, thrd_start_t func, void* arg) {
	/* Fill out the thread startup information (passed to the thread wrapper,
	   which will eventually free it) */
	_thread_start_info* ti = (_thread_start_info*)malloc(sizeof(_thread_start_info));
	if (ti == NULL) {
		return thrd_nomem;
	}
	ti->mFunction = func;
	ti->mArg = arg;

	/* Create the thread */
#if defined(_TTHREAD_WIN32_)
	*thr = (HANDLE)_beginthreadex(NULL, 0, _thrd_wrapper_function, (void*)ti, 0, NULL);
#elif defined(_TTHREAD_POSIX_)
	if (pthread_create(thr, NULL, _thrd_wrapper_function, (void*)ti) != 0) {
		*thr = 0;
	}
#endif

	/* Did we fail to create the thread? */
	if (!*thr) {
		free(ti);
		return thrd_error;
	}

	return thrd_success;
}

thrd_t thrd_current(void) {
#if defined(_TTHREAD_WIN32_)
	return GetCurrentThread();
#else
	return pthread_self();
#endif
}

int thrd_detach(thrd_t thr) {
	/* FIXME! */
	(void)thr;
	return thrd_error;
}

int thrd_equal(thrd_t thr0, thrd_t thr1) {
#if defined(_TTHREAD_WIN32_)
	return thr0 == thr1;
#else
	return pthread_equal(thr0, thr1);
#endif
}

void thrd_exit(int res) {
#if defined(_TTHREAD_WIN32_)
	ExitThread(res);
#else
	void* pres = malloc(sizeof(int));
	if (pres != NULL) {
		*(int*)pres = res;
	}
	pthread_exit(pres);
#endif
}

int thrd_join(thrd_t thr, int* res) {
#if defined(_TTHREAD_WIN32_)
	if (WaitForSingleObject(thr, INFINITE) == WAIT_FAILED) {
		return thrd_error;
	}
	if (res != NULL) {
		DWORD dwRes;
		GetExitCodeThread(thr, &dwRes);
		*res = dwRes;
	}
#elif defined(_TTHREAD_POSIX_)
	void* pres;
	int   ires = 0;
	if (pthread_join(thr, &pres) != 0) {
		return thrd_error;
	}
	if (pres != NULL) {
		ires = *(int*)pres;
		free(pres);
	}
	if (res != NULL) {
		*res = ires;
	}
#endif
	return thrd_success;
}

int thrd_sleep(const struct timespec* time_point, struct timespec* remaining) {
	struct timespec now;
#if defined(_TTHREAD_WIN32_)
	DWORD delta;
#else
	long delta;
#endif

	/* Get the current time */
	if (clock_gettime(CLOCK_REALTIME, &now) != 0)
		return -2; // FIXME: Some specific error code?

#if defined(_TTHREAD_WIN32_)
	/* Delta in milliseconds */
	delta = (DWORD)((time_point->tv_sec - now.tv_sec) * 1000 + (time_point->tv_nsec - now.tv_nsec + 500000) / 1000000);
	if (delta > 0) {
		Sleep(delta);
	}
#else
	/* Delta in microseconds */
	delta = (time_point->tv_sec - now.tv_sec) * 1000000L + (time_point->tv_nsec - now.tv_nsec + 500L) / 1000L;

	/* On some systems, the usleep argument must be < 1000000 */
	while (delta > 999999L) {
		usleep(999999);
		delta -= 999999L;
	}
	if (delta > 0L) {
		usleep((useconds_t)delta);
	}
#endif

	/* We don't support waking up prematurely (yet) */
	if (remaining) {
		remaining->tv_sec = 0;
		remaining->tv_nsec = 0;
	}
	return 0;
}

void thrd_yield(void) {
#if defined(_TTHREAD_WIN32_)
	Sleep(0);
#else
	sched_yield();
#endif
}

int tss_create(tss_t* key, tss_dtor_t dtor) {
#if defined(_TTHREAD_WIN32_)
	/* FIXME: The destructor function is not supported yet... */
	if (dtor != NULL) {
		return thrd_error;
	}
	*key = TlsAlloc();
	if (*key == TLS_OUT_OF_INDEXES) {
		return thrd_error;
	}
#else
	if (pthread_key_create(key, dtor) != 0) {
		return thrd_error;
	}
#endif
	return thrd_success;
}

void tss_delete(tss_t key) {
#if defined(_TTHREAD_WIN32_)
	TlsFree(key);
#else
	pthread_key_delete(key);
#endif
}

void* tss_get(tss_t key) {
#if defined(_TTHREAD_WIN32_)
	return TlsGetValue(key);
#else
	return pthread_getspecific(key);
#endif
}

int tss_set(tss_t key, void* val) {
#if defined(_TTHREAD_WIN32_)
	if (TlsSetValue(key, val) == 0) {
		return thrd_error;
	}
#else
	if (pthread_setspecific(key, val) != 0) {
		return thrd_error;
	}
#endif
	return thrd_success;
}

#if defined(_TTHREAD_EMULATE_CLOCK_GETTIME_)
int _tthread_clock_gettime(clockid_t clk_id, struct timespec* ts) {
#if defined(_TTHREAD_WIN32_)
	struct _timeb tb;
	_ftime(&tb);
	ts->tv_sec = (time_t)tb.time;
	ts->tv_nsec = 1000000L * (long)tb.millitm;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ts->tv_sec = (time_t)tv.tv_sec;
	ts->tv_nsec = 1000L * (long)tv.tv_usec;
#endif
	return 0;
}
#endif // _TTHREAD_EMULATE_CLOCK_GETTIME_

#endif // 1




#define _CRT_SECURE_NO_WARNINGS
int directoryExists(const char* path) {
#if defined(_WIN32) || defined(_WIN64) || __APPLE__ || __unix__
	struct stat st = { 0 };
	return stat(path, &st) + 1;
#else
	return -1;
#endif
}
const char* getUserDir() {
#if defined(_WIN32) || defined(_WIN64)
	return getenv("HOME");
#elif __APPLE__
#elif __unix__
	struct passwd* pw = getpwuid(getuid());
	return pw->pw_dir;
#endif
}
#if defined(__linux__) && defined(__GLIBC__)
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void handler(int sig) {
	void* array[100];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 100);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}

void _linux_register_error_handler() {
	signal(SIGSEGV, handler); // install our handler
	signal(SIGABRT, handler); // install our handler
}
#endif


// vk
#if 1
PFN_vkCmdBindPipeline       CmdBindPipeline;
PFN_vkCmdBindDescriptorSets CmdBindDescriptorSets;
PFN_vkCmdBindIndexBuffer    CmdBindIndexBuffer;
PFN_vkCmdBindVertexBuffers  CmdBindVertexBuffers;

PFN_vkCmdDrawIndexed CmdDrawIndexed;
PFN_vkCmdDraw        CmdDraw;

PFN_vkCmdSetStencilCompareMask CmdSetStencilCompareMask;
PFN_vkCmdSetStencilReference   CmdSetStencilReference;
PFN_vkCmdSetStencilWriteMask   CmdSetStencilWriteMask;
PFN_vkCmdBeginRenderPass       CmdBeginRenderPass;
PFN_vkCmdEndRenderPass         CmdEndRenderPass;
PFN_vkCmdSetViewport           CmdSetViewport;
PFN_vkCmdSetScissor            CmdSetScissor;

PFN_vkCmdPushConstants		CmdPushConstants;
PFN_vkWaitForFences			WaitForFences;
PFN_vkResetFences			ResetFences;
PFN_vkResetCommandBuffer	ResetCommandBuffer;
#endif
#include "shaders/vkvg_main.frag.h"


// ctx
#if 1
// Copyright (c) 2018-2024 Jean-Philippe Bruyère <jp_bruyere@hotmail.com>
//
// This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)

#ifdef DEBUG
static vec2     debugLinePoints[1000];
static uint32_t dlpCount = 0;
#if defined(VKVG_DBG_UTILS)
const float DBG_LAB_COLOR_SAV[4] = { 1, 0, 1, 1 };
const float DBG_LAB_COLOR_CLIP[4] = { 0, 1, 1, 1 };
#endif
#endif

#ifdef VKVG_IDENTITY_MATRIX
#undef VKVG_IDENTITY_MATRIX
#define VKVG_IDENTITY_MATRIX vkvg_matrix_t( 1, 0, 0, 1, 0, 0 )
#endif
#ifdef CreateRgbaf
#undef CreateRgbaf
#define CreateRgbaf(r, g, b, a)                                                                                        \
    (((int)(a * 255.0f) << 24) | ((int)(b * 255.0f) << 16) | ((int)(g * 255.0f) << 8) | (int)(r * 255.0f))
#endif



void push_update_descriptor_set_a(VkvgContext ctx, VkhImage img, uint64_t offset);

void _init_ctx(VkvgContext ctx) {
	static VkClearValue clearValues[3] = {};
	clearValues[1].depthStencil = { 1.0f, 0 };
	VkRect2D b = {};
	b.extent = { ctx->pSurf->width, ctx->pSurf->height };
	push_constants pc = {};
	pc.source.a = 1;
	pc.size = { (float)ctx->pSurf->width, (float)ctx->pSurf->height };
	pc.fsq_patternType = VKVG_PATTERN_TYPE_SOLID;
	pc.opacity = 1.0f;
	pc.mat = VKVG_IDENTITY_MATRIX;
	pc.matInv = VKVG_IDENTITY_MATRIX;

	ctx->lineWidth = 1.f;
	ctx->miterLimit = 10.f;
	ctx->curOperator = VKVG_OPERATOR_OVER;
	ctx->curFillRule = VKVG_FILL_RULE_NON_ZERO;
	ctx->bounds = b;
	ctx->pushConsts = pc;
	ctx->clearRect = { {{0}, {ctx->pSurf->width, ctx->pSurf->height}}, 0, 1 };
	ctx->renderPassBeginInfo.framebuffer = ctx->pSurf->fb;
	ctx->renderPassBeginInfo.renderArea.extent.width = ctx->pSurf->width;
	ctx->renderPassBeginInfo.renderArea.extent.height = ctx->pSurf->height;
	ctx->renderPassBeginInfo.pClearValues = clearValues;

	LOCK_SURFACE(ctx->pSurf)

		if (ctx->pSurf->newSurf) {
			ctx->renderPassBeginInfo.renderPass = ctx->dev->renderPass_ClearAll;
			ctx->pSurf->newSurf = false;
		}
		else {
			ctx->renderPassBeginInfo.renderPass = ctx->dev->renderPass_ClearStencil;
		}

	UNLOCK_SURFACE(ctx->pSurf);

	vkvg_surface_reference(ctx->pSurf);

	ctx->renderPassBeginInfo.clearValueCount = ctx->dev->samples == VK_SAMPLE_COUNT_1_BIT ? 2 : 3;

	ctx->selectedCharSize = 10 << 6;
	ctx->currentFont = NULL;
	ctx->selectedFontName[0] = 0;
	ctx->pattern = NULL;
	ctx->curColor = 0xff000000; // opaque black
	ctx->cmdStarted = false;
	ctx->curClipState = vkvg_clip_state_none;


	ctx->vertCount = ctx->indCount = 0;
#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	ctx->timelineStep = 0;
#endif
	auto cx = (vkvg_context*)ctx;
	cx->_vkCmdPushDescriptorSet = (PFN_vkCmdPushDescriptorSet)vkGetDeviceProcAddr(cx->dev->vkDev, "vkCmdPushDescriptorSet");
	// Get device push descriptor properties (to display them)
	PFN_vkGetPhysicalDeviceProperties2 _vkGetPhysicalDeviceProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(vkGetInstanceProcAddr(cx->dev->instance, "vkGetPhysicalDeviceProperties2"));
	if (cx->_vkCmdPushDescriptorSet && _vkGetPhysicalDeviceProperties2) {
		VkPhysicalDevicePushDescriptorProperties pushDescriptorProps = {};
		VkPhysicalDeviceProperties2KHR deviceProps2{};
		pushDescriptorProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;
		deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
		deviceProps2.pNext = &pushDescriptorProps;
		_vkGetPhysicalDeviceProperties2(cx->dev->phy, &deviceProps2);
		cx->maxPushDescriptors = pushDescriptorProps.maxPushDescriptors;
	}
}
VkvgContext vkvg_create(VkvgSurface surf) {
	LOG(VKVG_LOG_INFO, "CREATE Context\n");
	if (vkvg_surface_status(surf)) {
		LOG(VKVG_LOG_ERR, "CREATE Context failed, invalid surface\n");
		return (VkvgContext)&_vkvg_status_invalid_surface;
	}
	VkvgDevice  dev = surf->dev;
	if (vkvg_device_status(dev)) {
		LOG(VKVG_LOG_ERR, "CREATE Context failed, invalid device\n");
		return (VkvgContext)&_vkvg_status_device_error;
	}
	VkvgContext ctx = NULL;

	if (_device_try_get_cached_context(dev, &ctx)) {
		ctx->pSurf = surf;
		ctx->status = VKVG_STATUS_SUCCESS;
		_init_ctx(ctx);
		//_update_descriptor_set(ctx, surf->dev->emptyImg, ctx->dsSrc);
		push_update_descriptor_set_a(ctx, 0, 0);
		_clear_path(ctx);
		ctx->cmd = ctx->cmdBuffers[0]; // current recording buffer
		return ctx;
	}
	ctx = (vkvg_context*)calloc(1, sizeof(vkvg_context));

	if (!ctx) {
		LOG(VKVG_LOG_ERR, "CREATE context failed, no memory\n");
		return (VkvgContext)&_vkvg_status_no_memory;
	}

	LOG(VKVG_LOG_INFO, "CREATE Context: ctx = %p; surf = %p\n", ctx, surf);
	ctx->pSurf = surf;

	ctx->sizePoints = VKVG_PTS_SIZE;
	ctx->sizeVertices = ctx->sizeVBO = VKVG_VBO_SIZE;
	ctx->sizeIndices = ctx->sizeIBO = VKVG_IBO_SIZE;
	ctx->sizePathes = VKVG_PATHES_SIZE;
	ctx->renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	ctx->dev = surf->dev;

	ctx->points = (vec2*)malloc(VKVG_VBO_SIZE * sizeof(vec2));
	ctx->pathes = (uint32_t*)malloc(VKVG_PATHES_SIZE * sizeof(uint32_t));
	ctx->vertexCache = (Vertex*)malloc(ctx->sizeVertices * sizeof(Vertex));
	ctx->indexCache = (VKVG_IBO_INDEX_TYPE*)malloc(ctx->sizeIndices * sizeof(VKVG_IBO_INDEX_TYPE));

	if (!ctx->points || !ctx->pathes || !ctx->vertexCache || !ctx->indexCache) {
		if (ctx->points)
			free(ctx->points);
		if (ctx->pathes)
			free(ctx->pathes);
		if (ctx->vertexCache)
			free(ctx->vertexCache);
		if (ctx->indexCache)
			free(ctx->indexCache);
		free(ctx);
		LOG(VKVG_LOG_ERR, "CREATE context failed, no memory\n");
		return (VkvgContext)&_vkvg_status_no_memory;
	}

	//ctx->pathCtx = new_vgctx();
	_init_ctx(ctx);

	VkhDevice vkhd = (VkhDevice)&dev->vkDev;
	// for context to be thread safe, command pool and descriptor pool have to be created in the thread of the context.
	ctx->cmdPool = vkh_cmd_pool_create(vkhd, dev->gQueue->familyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

#ifndef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	ctx->flushFence = vkh_fence_create_signaled((VkhDevice)&ctx->dev->vkDev);
#endif
	ctx->d_img = 0;
	ctx->d_offset = 0;
	ctx->isdset = false;
	_create_vertices_buff(ctx);
	_create_gradient_buff(ctx);
	_create_cmd_buff(ctx);
	_createDescriptorPool(ctx);
	_init_descriptor_sets(ctx);
	ctx->cmd = ctx->cmdBuffers[0]; // current recording buffer
	_font_cache_update_context_descset(ctx);
	//_update_descriptor_set(ctx, surf->dev->emptyImg, ctx->dsSrc); 
	//_update_gradient_desc_set(ctx);
	push_update_descriptor_set_a(ctx, 0, 0);
	_clear_path(ctx);


	ctx->references = 1;
	ctx->status = VKVG_STATUS_SUCCESS;

	LOG(VKVG_LOG_DBG_ARRAYS, "INIT\tctx = %p; pathes:%ju pts:%ju vch:%d vbo:%d ich:%d ibo:%d\n", ctx,
		(uint64_t)ctx->sizePathes, (uint64_t)ctx->sizePoints, ctx->sizeVertices, ctx->sizeVBO, ctx->sizeIndices,
		ctx->sizeIBO);

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)ctx->cmdPool, "CTX Cmd Pool");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)ctx->cmdBuffers[0], "CTX Cmd Buff A");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)ctx->cmdBuffers[1], "CTX Cmd Buff B");
#ifndef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_FENCE, (uint64_t)ctx->flushFence, "CTX Flush Fence");
#endif
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t)ctx->descriptorPool,
		"CTX Descriptor Pool");
	//vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)ctx->dsSrc, "CTX DescSet SOURCE");
	//vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)ctx->dsFont, "CTX DescSet FONT");
	//vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)ctx->dsGrad, "CTX DescSet GRADIENT");

	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_BUFFER, (uint64_t)ctx->indices.buffer, "CTX Index Buff");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_BUFFER, (uint64_t)ctx->vertices.buffer, "CTX Vertex Buff");
#endif

	return ctx;
}
void _flush_cmd_buff0(VkvgContext ctx);
void vkvg_flush(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;
	//c_runtime_cx rtc;
	//rtc.begin();
	//ctx->pathCtx->draw(ctx);
	//ctx->pathCtx->end_frame();
	//int ms = rtc.end();
	//if (ms > 0)
	//	printf("vkvg_flush wait ms: %d\n", ms);
	//return;
	_flush_cmd_buff0(ctx);
	_wait_ctx_flush_end(ctx);
	/*
	#ifdef DEBUG

		vec4 red = {0,0,1,1};
		vec4 green = {0,1,0,1};
		vec4 white = {1,1,1,1};

		int j = 0;
		while (j < dlpCount) {
			add_line(ctx, debugLinePoints[j], debugLinePoints[j+1],green);
			j+=2;
			add_line(ctx, debugLinePoints[j], debugLinePoints[j+1],red);
			j+=2;
			add_line(ctx, debugLinePoints[j], debugLinePoints[j+1],white);
			j+=2;
		}
		dlpCount = 0;
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pSurf->dev->pipelineLineList);
		CmdDrawIndexed(ctx->cmd, ctx->indCount-ctx->curIndStart, 1, ctx->curIndStart, 0, 1);
		_flush_cmd_buff(ctx);
	#endif
	*/
}

void _clear_context(VkvgContext ctx) {
	// free saved context stack elmt
	vkvg_context_save_t* next = ctx->pSavedCtxs;
	ctx->pSavedCtxs = NULL;
	while (next != NULL) {
		vkvg_context_save_t* cur = next;
		next = cur->pNext;
		_free_ctx_save(cur);
	}
	// free additional stencil use in save/restore process
	if (ctx->savedStencils) {
		uint8_t curSaveStencil = ctx->curSavBit / 6;
		for (int i = curSaveStencil; i > 0; i--)
			vkh_image_destroy(ctx->savedStencils[i - 1]);
		free(ctx->savedStencils);
		ctx->savedStencils = NULL;
		ctx->curSavBit = 0;
	}

	// remove context from double linked list of context in device
	/*if (ctx->dev->lastCtx == ctx){
		ctx->dev->lastCtx = ctx->pPrev;
		if (ctx->pPrev != NULL)
			ctx->pPrev->pNext = NULL;
	}else if (ctx->pPrev == NULL){
		//first elmt, and it's not last one so pnext is not null
		ctx->pNext->pPrev = NULL;
	}else{
		ctx->pPrev->pNext = ctx->pNext;
		ctx->pNext->pPrev = ctx->pPrev;
	}*/
	if (ctx->dashCount > 0) {
		free(ctx->dashes);
		ctx->dashCount = 0;
	}
}

void vkvg_destroy(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;

	ctx->references--;
	if (ctx->references > 0)
		return;

	LOG(VKVG_LOG_INFO, "DESTROY Context: ctx = %p (status:%d); surf = %p\n", ctx, ctx->status, ctx->pSurf);

	vkvg_flush(ctx);

	LOG(VKVG_LOG_DBG_ARRAYS, "END\tctx = %p; pathes:%d pts:%d vch:%d vbo:%d ich:%d ibo:%d\n", ctx, ctx->sizePathes,
		ctx->sizePoints, ctx->sizeVertices, ctx->sizeVBO, ctx->sizeIndices, ctx->sizeIBO);

#if VKVG_RECORDING
	if (ctx->recording)
		_destroy_recording(ctx->recording);
#endif

	if (ctx->pattern)
		vkvg_pattern_destroy(ctx->pattern);

	_clear_context(ctx);

#if VKVG_DBG_STATS
	if (ctx->dev->threadAware)
		mtx_lock(&ctx->dev->mutex);

	vkvg_debug_stats_t* dbgstats = &ctx->dev->debug_stats;
	if (dbgstats->sizePoints < ctx->sizePoints)
		dbgstats->sizePoints = ctx->sizePoints;
	if (dbgstats->sizePathes < ctx->sizePathes)
		dbgstats->sizePathes = ctx->sizePathes;
	if (dbgstats->sizeVertices < ctx->sizeVertices)
		dbgstats->sizeVertices = ctx->sizeVertices;
	if (dbgstats->sizeIndices < ctx->sizeIndices)
		dbgstats->sizeIndices = ctx->sizeIndices;
	if (dbgstats->sizeVBO < ctx->sizeVBO)
		dbgstats->sizeVBO = ctx->sizeVBO;
	if (dbgstats->sizeIBO < ctx->sizeIBO)
		dbgstats->sizeIBO = ctx->sizeIBO;

	if (ctx->dev->threadAware)
		mtx_unlock(&ctx->dev->mutex);
#endif

	vkvg_surface_destroy(ctx->pSurf);

	if (!ctx->status && ctx->dev->cachedContextCount < VKVG_MAX_CACHED_CONTEXT_COUNT) {
		_device_store_context(ctx);
		ctx->status = VKVG_STATUS_IN_CACHE;
		return;
	}

	_release_context_ressources(ctx);

	ctx = NULL;
}
void vkvg_set_opacity(VkvgContext ctx, float opacity) {
	if (vkvg_status(ctx))
		return;

	if (EQUF(ctx->pushConsts.opacity, opacity))
		return;

	_emit_draw_cmd_undrawn_vertices(ctx);
	ctx->pushConsts.opacity = opacity;
	ctx->pushCstDirty = true;
}
float vkvg_get_opacity(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return 0;
	return ctx->pushConsts.opacity;
}
vkvg_status_t vkvg_status(VkvgContext ctx) { return !ctx ? VKVG_STATUS_NULL_POINTER : ctx->status; }
VkvgContext   vkvg_reference(VkvgContext ctx) {
	if (!ctx->status)
		ctx->references++;
	return ctx;
}
uint32_t vkvg_get_reference_count(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return 0;
	return ctx->references;
}
void vkvg_new_sub_path(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;

	RECORD(ctx, VKVG_CMD_NEW_SUB_PATH);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: new_sub_path:\n");

	_finish_path(ctx);
}
void vkvg_new_path(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;

	RECORD(ctx, VKVG_CMD_NEW_PATH);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: new_path:\n");

	_clear_path(ctx);
}
void vkvg_close_path(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;

	RECORD(ctx, VKVG_CMD_CLOSE_PATH);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: close_path:\n");

	if (ctx->pathes[ctx->pathPtr] & PATH_CLOSED_BIT) // already closed
		return;
	// check if at least 3 points are present
	if (ctx->pathes[ctx->pathPtr] < 3)
		return;

	// prevent closing on the same point
	if (vec2_equ(ctx->points[ctx->pointCount - 1], ctx->points[ctx->pointCount - ctx->pathes[ctx->pathPtr]])) {
		if (ctx->pathes[ctx->pathPtr] < 4) // ensure enough points left for closing
			return;
		_remove_last_point(ctx);
	}

	ctx->pathes[ctx->pathPtr] |= PATH_CLOSED_BIT;

	_finish_path(ctx);
}
void vkvg_rel_line_to(VkvgContext ctx, float dx, float dy) {
	if (vkvg_status(ctx))
		return;

	RECORD(ctx, VKVG_CMD_REL_LINE_TO, dx, dy);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: rel_line_to: %f, %f\n", dx, dy);

	if (_current_path_is_empty(ctx))
		_add_point(ctx, 0, 0);
	vec2 cp = _get_current_position(ctx);
	_line_to(ctx, cp.x + dx, cp.y + dy);
}
void vkvg_line_to(VkvgContext ctx, float x, float y) {
	if (vkvg_status(ctx))
		return;

	RECORD(ctx, VKVG_CMD_LINE_TO, x, y);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: line_to: %f, %f\n", x, y);
	_line_to(ctx, x, y);
}
void vkvg_arc(VkvgContext ctx, float xc, float yc, float radius, float a1, float a2) {
	if (vkvg_status(ctx))
		return;

	RECORD(ctx, VKVG_CMD_ARC, xc, yc, radius, a1, a2);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: arc: %f,%f %f %f %f\n", xc, yc, radius, a1, a2);

	while (a2 < a1) // positive arc must have a1<a2
		a2 += 2.f * M_PIF;

	if (a2 - a1 > 2.f * M_PIF) // limit arc to 2PI
		a2 = a1 + 2.f * M_PIF;

	vec2 v = { cosf(a1) * radius + xc, sinf(a1) * radius + yc };

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

	a += step;

	if (EQUF(a2, a1))
		return;

	while (a < a2) {
		v.x = cosf(a) * radius + xc;
		v.y = sinf(a) * radius + yc;
		_add_point(ctx, v.x, v.y);
		a += step;
	}

	if (EQUF(a2 - a1, M_PIF * 2.f)) { // if arc is complete circle, last point is the same as the first one
		_set_curve_end(ctx);
		vkvg_close_path(ctx);
		return;
	}
	a = a2;
	// vec2 lastP = v;
	v.x = cosf(a) * radius + xc;
	v.y = sinf(a) * radius + yc;
	// if (!vec2_equ (v,lastP))//this test should not be required
	_add_point(ctx, v.x, v.y);
	_set_curve_end(ctx);
}
void vkvg_arc_negative(VkvgContext ctx, float xc, float yc, float radius, float a1, float a2) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_ARC_NEG, xc, yc, radius, a1, a2);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: %f,%f %f %f %f\n", xc, yc, radius, a1, a2);
	while (a2 > a1)
		a2 -= 2.f * M_PIF;
	if (a1 - a2 > a1 + 2.f * M_PIF) // limit arc to 2PI
		a2 = a1 - 2.f * M_PIF;

	vec2 v = { cosf(a1) * radius + xc, sinf(a1) * radius + yc };

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

	if (EQUF(a1 - a2, M_PIF * 2.f)) { // if arc is complete circle, last point is the same as the first one
		_set_curve_end(ctx);
		vkvg_close_path(ctx);
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
void vkvg_rel_move_to(VkvgContext ctx, float x, float y) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_REL_MOVE_TO, x, y);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: rel_mote_to: %f, %f\n", x, y);
	if (_current_path_is_empty(ctx))
		_add_point(ctx, 0, 0);
	vec2 cp = _get_current_position(ctx);
	_finish_path(ctx);
	_add_point(ctx, cp.x + x, cp.y + y);
}
void vkvg_move_to(VkvgContext ctx, float x, float y) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_MOVE_TO, x, y);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: move_to: %f,%f\n", x, y);
	_finish_path(ctx);
	_add_point(ctx, x, y);
}
bool vkvg_has_current_point(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return false;
	return !_current_path_is_empty(ctx);
}
void vkvg_get_current_point(VkvgContext ctx, float* x, float* y) {
	if (vkvg_status(ctx))
		return;
	assert(x);
	assert(y);
	if (_current_path_is_empty(ctx)) {
		*x = *y = 0;
		return;
	}
	vec2 cp = _get_current_position(ctx);
	*x = cp.x;
	*y = cp.y;
}
void _curve_to(VkvgContext ctx, float x1, float y1, float x2, float y2, float x3, float y3) {
	// prevent running _recursive_bezier when all 4 curve points are equal
	if (EQUF(x1, x2) && EQUF(x2, x3) && EQUF(y1, y2) && EQUF(y2, y3)) {
		if (_current_path_is_empty(ctx) ||
			(EQUF(_get_current_position(ctx).x, x1) && EQUF(_get_current_position(ctx).y, y1)))
			return;
	}
	ctx->simpleConvex = false;
	_set_curve_start(ctx);
	if (_current_path_is_empty(ctx))
		_add_point(ctx, x1, y1);

	vec2 cp = _get_current_position(ctx);

	// compute dyn distanceTolerance depending on current scale
	float sx = 1, sy = 1;
	vkvg_matrix_get_scale(&ctx->pushConsts.mat, &sx, &sy);
	float distanceTolerance = fabs(0.25f / fmaxf(sx, sy));

	_recursive_bezier(ctx, distanceTolerance, cp.x, cp.y, x1, y1, x2, y2, x3, y3, 0);
	/*cp.x = x3;
	cp.y = y3;
	if (!vec2_equ(ctx->points[ctx->pointCount-1],cp))*/
	_add_point(ctx, x3, y3);
	_set_curve_end(ctx);
}
const double quadraticFact = 2.0 / 3.0;
void _quadratic_to(VkvgContext ctx, float x1, float y1, float x2, float y2) {
	float x0, y0;
	if (_current_path_is_empty(ctx)) {
		x0 = x1;
		y0 = y1;
	}
	else
		vkvg_get_current_point(ctx, &x0, &y0);
	_curve_to(ctx, x0 + (x1 - x0) * quadraticFact, y0 + (y1 - y0) * quadraticFact, x2 + (x1 - x2) * quadraticFact,
		y2 + (y1 - y2) * quadraticFact, x2, y2);
}
void vkvg_quadratic_to(VkvgContext ctx, float x1, float y1, float x2, float y2) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_QUADRATIC_TO, x1, y1, x2, y2);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: quadratic_to: %f, %f, %f, %f\n", x1, y1, x2, y2);
	_quadratic_to(ctx, x1, y1, x2, y2);
}
void vkvg_rel_quadratic_to(VkvgContext ctx, float x1, float y1, float x2, float y2) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_REL_QUADRATIC_TO, x1, y1, x2, y2);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: rel_quadratic_to: %f, %f, %f, %f\n", x1, y1, x2, y2);
	vec2 cp = _get_current_position(ctx);
	_quadratic_to(ctx, cp.x + x1, cp.y + y1, cp.x + x2, cp.y + y2);
}
void vkvg_curve_to(VkvgContext ctx, float x1, float y1, float x2, float y2, float x3, float y3) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_CURVE_TO, x1, y1, x2, y2, x3, y3);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: curve_to %f,%f %f,%f %f,%f:\n", x1, y1, x2, y2, x3, y3);
	_curve_to(ctx, x1, y1, x2, y2, x3, y3);
}
void vkvg_rel_curve_to(VkvgContext ctx, float x1, float y1, float x2, float y2, float x3, float y3) {
	if (vkvg_status(ctx))
		return;
	if (_current_path_is_empty(ctx)) {
		ctx->status = VKVG_STATUS_NO_CURRENT_POINT;
		return;
	}
	RECORD(ctx, (uint32_t)VKVG_CMD_REL_CURVE_TO, x1, y1, x2, y2, x3, y3);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: rel curve_to %f,%f %f,%f %f,%f:\n", x1, y1, x2, y2, x3, y3);
	vec2 cp = _get_current_position(ctx);
	_curve_to(ctx, cp.x + x1, cp.y + y1, cp.x + x2, cp.y + y2, cp.x + x3, cp.y + y3);
}
void vkvg_fill_rectangle(VkvgContext ctx, float x, float y, float w, float h) {
	if (vkvg_status(ctx))
		return;
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: fill_rectangle:\n");
	_vao_add_rectangle(ctx, x, y, w, h);
	//_record_draw_cmd(ctx);
}
#ifdef RECORD2
#undef RECORD2
#define RECORD2(ctx, ...)                                                                                              \
    {                                                                                                                  \
        if (ctx->recording) {                                                                                          \
            _record(ctx->recording, __VA_ARGS__);                                                                      \
            return VKVG_STATUS_SUCCESS;                                                                                                  \
        }                                                                                                              \
    } 
#endif

vkvg_status_t vkvg_rectangle(VkvgContext ctx, float x, float y, float w, float h) {
	if (vkvg_status(ctx))
		return ctx->status;
	RECORD2(ctx, VKVG_CMD_RECTANGLE, x, y, w, h);
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: rectangle: %f,%f,%f,%f\n", x, y, w, h);
	_finish_path(ctx);

	if (w <= 0 || h <= 0)
		return VKVG_STATUS_INVALID_RECT;

	_add_point(ctx, x, y);
	_add_point(ctx, x + w, y);
	_add_point(ctx, x + w, y + h);
	_add_point(ctx, x, y + h);

	ctx->pathes[ctx->pathPtr] |= (PATH_CLOSED_BIT | PATH_IS_CONVEX_BIT);

	_finish_path(ctx);
	return VKVG_STATUS_SUCCESS;
}
vkvg_status_t vkvg_rounded_rectangle(VkvgContext ctx, float x, float y, float w, float h, float radius) {
	if (vkvg_status(ctx))
		return ctx->status;
	LOG(VKVG_LOG_INFO_CMD, "CMD: rounded_rectangle:\n");
	_finish_path(ctx);

	if (w <= 0 || h <= 0)
		return VKVG_STATUS_INVALID_RECT;

	if ((radius > w / 2.0f) || (radius > h / 2.0f))
		radius = fmin(w / 2.0f, h / 2.0f);

	vkvg_move_to(ctx, x, y + radius);
	vkvg_arc(ctx, x + radius, y + radius, radius, M_PIF, -M_PIF_2);
	vkvg_line_to(ctx, x + w - radius, y);
	vkvg_arc(ctx, x + w - radius, y + radius, radius, -M_PIF_2, 0);
	vkvg_line_to(ctx, x + w, y + h - radius);
	vkvg_arc(ctx, x + w - radius, y + h - radius, radius, 0, M_PIF_2);
	vkvg_line_to(ctx, x + radius, y + h);
	vkvg_arc(ctx, x + radius, y + h - radius, radius, M_PIF_2, M_PIF);
	vkvg_line_to(ctx, x, y + radius);
	vkvg_close_path(ctx);

	return VKVG_STATUS_SUCCESS;
}
void vkvg_rounded_rectangle2(VkvgContext ctx, float x, float y, float w, float h, float rx, float ry) {
	if (vkvg_status(ctx))
		return;
	LOG(VKVG_LOG_INFO_CMD, "CMD: rounded_rectangle2:\n");
	vkvg_move_to(ctx, x + rx, y);
	vkvg_line_to(ctx, x + w - rx, y);
	vkvg_elliptic_arc_to(ctx, x + w, y + ry, false, true, rx, ry, 0);

	vkvg_line_to(ctx, x + w, y + h - ry);
	vkvg_elliptic_arc_to(ctx, x + w - rx, y + h, false, true, rx, ry, 0);

	vkvg_line_to(ctx, x + rx, y + h);
	vkvg_elliptic_arc_to(ctx, x, y + h - ry, false, true, rx, ry, 0);

	vkvg_line_to(ctx, x, y + ry);
	vkvg_elliptic_arc_to(ctx, x + rx, y, false, true, rx, ry, 0);

	vkvg_close_path(ctx);
}
void vkvg_path_extents(VkvgContext ctx, float* const x1, float* const y1, float* const x2, float* const y2) {
	if (vkvg_status(ctx))
		return;

	_finish_path(ctx);

	if (!ctx->pathPtr) { // no path
		*x1 = *x2 = *y1 = *y2 = 0;
		return;
	}

	_vkvg_path_extents(ctx, false, x1, y1, x2, y2);
}

vkvg_clip_state_t _get_previous_clip_state(VkvgContext ctx) {
	if (!ctx->pSavedCtxs) // no clip saved => clear
		return vkvg_clip_state_clear;
	return ctx->pSavedCtxs->clippingState;
}
static const VkClearAttachment clearStencil = { VK_IMAGE_ASPECT_STENCIL_BIT, 1, {{{0}}} };
static const VkClearAttachment clearColorAttach = { VK_IMAGE_ASPECT_COLOR_BIT, 0, {{{0}}} };

void _reset_clip(VkvgContext ctx) {
	_emit_draw_cmd_undrawn_vertices(ctx);
	if (!ctx->cmdStarted) {
		// if command buffer is not already started and in a renderpass, we use the renderpass
		// with the loadop clear for stencil
		ctx->renderPassBeginInfo.renderPass = ctx->dev->renderPass_ClearStencil;
		// force run of one renderpass (even empty) to perform clear load op
		_start_cmd_for_render_pass(ctx);
		return;
	}
	vkCmdClearAttachments(ctx->cmd, 1, &clearStencil, 1, &ctx->clearRect);
}

void vkvg_reset_clip(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;

	RECORD(ctx, VKVG_CMD_RESET_CLIP);

	if (ctx->curClipState == vkvg_clip_state_clear)
		return;
	if (_get_previous_clip_state(ctx) == vkvg_clip_state_clear)
		ctx->curClipState = vkvg_clip_state_none;
	else
		ctx->curClipState = vkvg_clip_state_clear;

	_reset_clip(ctx);
}
void vkvg_clear(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;

	RECORD(ctx, VKVG_CMD_CLEAR);

	if (_get_previous_clip_state(ctx) == vkvg_clip_state_clear)
		ctx->curClipState = vkvg_clip_state_none;
	else
		ctx->curClipState = vkvg_clip_state_clear;
	//ctx->pathCtx->begin_frame();
	//return;
	_emit_draw_cmd_undrawn_vertices(ctx);
	if (!ctx->cmdStarted) {
		ctx->renderPassBeginInfo.renderPass = ctx->dev->renderPass_ClearAll;
		_start_cmd_for_render_pass(ctx);
		return;
	}
	VkClearAttachment ca[2] = { clearColorAttach, clearStencil };
	vkCmdClearAttachments(ctx->cmd, 2, ca, 1, &ctx->clearRect);
}
void ear_fill_non_zero(VkvgContext ctx);
#if 1
void _clip_preserve(VkvgContext ctx) {
	_finish_path(ctx);

	if (!ctx->pathPtr) // nothing to clip
		return;

	_emit_draw_cmd_undrawn_vertices(ctx);

	LOG(VKVG_LOG_INFO, "CLIP: ctx = %p; path cpt = %d;\n", ctx, ctx->pathPtr / 2);

	_ensure_renderpass_is_started(ctx);

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_cmd_label_start(ctx->cmd, "clip", DBG_LAB_COLOR_CLIP);
#endif

	if (ctx->curFillRule == VKVG_FILL_RULE_EVEN_ODD) {
		_poly_fill(ctx, NULL);
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelineClipping);
	}
	else {
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelineClipping);
		CmdSetStencilReference(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
		CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
		CmdSetStencilWriteMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
		ear_fill_non_zero(ctx);
		_emit_draw_cmd_undrawn_vertices(ctx);
	}
	CmdSetStencilReference(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
	CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
	CmdSetStencilWriteMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_ALL_BIT);

	_draw_full_screen_quad(ctx, NULL);

	_bind_draw_pipeline(ctx);
	CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_cmd_label_end(ctx->cmd);
#endif

	ctx->curClipState = vkvg_clip_state_clip;
}
void _fill_preserve(VkvgContext ctx) {
	_finish_path(ctx);

	if (!ctx->pathPtr) // nothing to fill
		return;

	LOG(VKVG_LOG_INFO, "FILL: ctx = %p; path cpt = %d;\n", ctx, ctx->subpathCount);

	if (ctx->curFillRule == VKVG_FILL_RULE_EVEN_ODD) {
		_emit_draw_cmd_undrawn_vertices(ctx);
		vec4 bounds = { FLT_MAX, FLT_MAX, FLT_MIN, FLT_MIN };
		_poly_fill(ctx, &bounds);
		_bind_draw_pipeline(ctx);
		CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
		_draw_full_screen_quad(ctx, &bounds);
		CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
		return;
	}

	if (ctx->vertCount - ctx->curVertOffset + ctx->pointCount > VKVG_IBO_MAX)
		_emit_draw_cmd_undrawn_vertices(ctx); // limit draw call to addressable vx with choosen index type

	if (ctx->pattern) // if not solid color, source img or gradient has to be bound
		_ensure_renderpass_is_started(ctx);
	if (ctx->fill_rule_winding)
		_fill_non_zero(ctx);
	else
		ear_fill_non_zero(ctx);
}
void _stroke_preserve(VkvgContext ctx) {
	_finish_path(ctx);

	if (!ctx->pathPtr) // nothing to stroke
		return;

	LOG(VKVG_LOG_INFO, "STROKE: ctx = %p; path ptr = %d;\n", ctx, ctx->pathPtr);

	stroke_context_t str = { 0 };
	str.hw = ctx->lineWidth * 0.5f;
	str.lhMax = ctx->miterLimit * ctx->lineWidth;
	uint32_t ptrPath = 0;

	while (ptrPath < ctx->pathPtr) {
		uint32_t ptrSegment = 0, lastSegmentPointIdx = 0;
		uint32_t firstPathPointIdx = str.cp;
		uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;
		uint32_t lastPathPointIdx = str.cp + pathPointCount - 1;

		dash_context_t dc = { 0 };

		if (_path_has_curves(ctx, ptrPath)) {
			ptrSegment = 1;
			lastSegmentPointIdx = str.cp + (ctx->pathes[ptrPath + ptrSegment] & PATH_ELT_MASK) - 1;
		}

		str.firstIdx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);

		// LOG(VKVG_LOG_INFO_PATH, "\tPATH: points count=%10d end point idx=%10d", ctx->pathes[ptrPath]&PATH_ELT_MASK,
		// lastPathPointIdx);

		if (ctx->dashCount > 0) {
			// init dash stroke
			dc.dashOn = true;
			dc.curDash = 0; // current dash index
			dc.totDashLength = 0; // limit offset to total length of dashes
			for (uint32_t i = 0; i < ctx->dashCount; i++)
				dc.totDashLength += ctx->dashes[i];
			if (dc.totDashLength == 0) {
				ctx->status = VKVG_STATUS_INVALID_DASH;
				return;
			}
			dc.curDashOffset = fmodf(
				ctx->dashOffset,
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

		if (_path_has_curves(ctx, ptrPath)) {
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

		if (ctx->dashCount > 0) {
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
					dc.curDash = ctx->dashCount - 1;
				float m = fminf(ctx->dashes[prevDash] - dc.curDashOffset, ctx->dashes[dc.curDash]);
				vec2  p = vec2_sub(ctx->points[str.iR], vec2_mult_s(dc.normal, m));
				_draw_stoke_cap(ctx, &str, p, dc.normal, false);
			}
		}
		else if (_path_is_closed(ctx, ptrPath)) {
			str.iR = firstPathPointIdx;
			bool inverse = _build_vb_step(ctx, &str, false);

			VKVG_IBO_INDEX_TYPE* inds = &ctx->indexCache[ctx->indCount - 6];
			VKVG_IBO_INDEX_TYPE  ii = str.firstIdx;
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

		// limit batch size here to 1/3 of the ibo index type ability
		if (ctx->vertCount - ctx->curVertOffset > VKVG_IBO_MAX / 3)
			_emit_draw_cmd_undrawn_vertices(ctx);
	}
}
#endif
void vkvg_clip(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_CLIP);
	_clip_preserve(ctx);
	_clear_path(ctx);
}
void vkvg_stroke(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_STROKE);
	_stroke_preserve(ctx);
	_clear_path(ctx);
}
void vkvg_fill(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_FILL);
	_fill_preserve(ctx);
	_clear_path(ctx);
}
void vkvg_clip_preserve(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_CLIP_PRESERVE);
	_clip_preserve(ctx);
}
void vkvg_fill_preserve(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_FILL_PRESERVE);
	_fill_preserve(ctx);
}
void vkvg_stroke_preserve(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_STROKE_PRESERVE);
	_stroke_preserve(ctx);
}

void vkvg_paint(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_PAINT);
	_finish_path(ctx);

	if (ctx->pathPtr) {
		vkvg_fill(ctx);
		return;
	}

	_ensure_renderpass_is_started(ctx);
	_draw_full_screen_quad(ctx, NULL);
}
// 将32位RGBA颜色转换为预乘Alpha格式 
uint32_t rgba_to_premultiplied(uint32_t color) {
	uint8_t a = (color >> 24) & 0xFF;
	uint8_t r = color & 0xFF;
	uint8_t g = (color >> 8) & 0xFF;
	uint8_t b = (color >> 16) & 0xFF;
	// 若Alpha为0或255，可直接优化 
	if (a == 0) return 0;          // 全透明
	if (a == 255) return color;    // 不透明，无需预乘 
	r = (uint8_t)((r * a + 128) / 255);
	g = (uint8_t)((g * a + 128) / 255);
	b = (uint8_t)((b * a + 128) / 255);
	return (a << 24) | (b << 16) | (g << 8) | r;
}
void vkvg_set_source_color(VkvgContext ctx, uint32_t c) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SET_SOURCE_COLOR, c);
	ctx->curColor = c;
	// shader处理预乘了
#ifdef VKVG_PREMULT_ALPHA
	//ctx->curColor = rgba_to_premultiplied(ctx->curColor);
#endif
	_update_cur_pattern(ctx, NULL);
}
void vkvg_set_source_rgb(VkvgContext ctx, float r, float g, float b) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SET_SOURCE_RGB, r, g, b);
	ctx->curColor = CreateRgbaf(r, g, b, 1);
	_update_cur_pattern(ctx, NULL);
}
void vkvg_set_source_rgba(VkvgContext ctx, float r, float g, float b, float a) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SET_SOURCE_RGBA, r, g, b, a);
	ctx->curColor = CreateRgbaf(r, g, b, a);
#ifdef VKVG_PREMULT_ALPHA
	//ctx->curColor = rgba_to_premultiplied(ctx->curColor);
#endif
	_update_cur_pattern(ctx, NULL);
}
void vkvg_set_source_surface(VkvgContext ctx, VkvgSurface surf, float x, float y) {
	if (vkvg_status(ctx) || vkvg_surface_status(surf))
		return;
	RECORD(ctx, VKVG_CMD_SET_SOURCE_SURFACE, x, y, surf);
	ctx->pushConsts.source.x = x;
	ctx->pushConsts.source.y = y;
	_update_cur_pattern(ctx, vkvg_pattern_create_for_surface(surf));
	ctx->pushCstDirty = true;
}
void vkvg_set_source(VkvgContext ctx, VkvgPattern pat) {
	if (vkvg_status(ctx) || vkvg_pattern_status(pat))
		return;
	RECORD(ctx, VKVG_CMD_SET_SOURCE, pat);
	_update_cur_pattern(ctx, pat);
	vkvg_pattern_reference(pat);
}
void vkvg_set_line_width(VkvgContext ctx, float width) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SET_LINE_WIDTH, width);
	ctx->lineWidth = width;
}
void vkvg_set_miter_limit(VkvgContext ctx, float limit) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SET_LINE_WIDTH, limit);
	ctx->miterLimit = limit;
}
void vkvg_set_line_cap(VkvgContext ctx, vkvg_line_cap_t cap) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SET_LINE_CAP, cap);
	ctx->lineCap = cap;
}
void vkvg_set_line_join(VkvgContext ctx, vkvg_line_join_t join) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SET_LINE_JOIN, join);
	ctx->lineJoin = join;
}
void vkvg_set_operator(VkvgContext ctx, vkvg_operator_t op) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SET_OPERATOR, op);
	if (op == ctx->curOperator)
		return;

	_emit_draw_cmd_undrawn_vertices(
		ctx); // draw call with different ops cant be combined, so emit draw cmd for previous vertices.

	ctx->curOperator = op;

	if (ctx->cmdStarted)
		_bind_draw_pipeline(ctx);
}
void vkvg_set_fill_rule(VkvgContext ctx, vkvg_fill_rule_t fr) {
	if (vkvg_status(ctx))
		return;
#ifndef __APPLE__
	RECORD(ctx, VKVG_CMD_SET_FILL_RULE, fr);
	ctx->curFillRule = fr;
#endif
}
vkvg_fill_rule_t vkvg_get_fill_rule(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return VKVG_FILL_RULE_NON_ZERO;
	return ctx->curFillRule;
}
float vkvg_get_line_width(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return 0;
	return ctx->lineWidth;
}
float vkvg_get_miter_limit(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return 0;
	return ctx->miterLimit;
}
void vkvg_set_dash(VkvgContext ctx, const float* dashes, uint32_t num_dashes, float offset) {
	if (vkvg_status(ctx))
		return;
	if (ctx->dashCount > 0)
		free(ctx->dashes);
	RECORD(ctx, VKVG_CMD_SET_DASH, num_dashes, offset, dashes);
	ctx->dashCount = num_dashes;
	ctx->dashOffset = offset;
	if (ctx->dashCount == 0)
		return;
	ctx->dashes = (float*)malloc(sizeof(float) * ctx->dashCount);
	memcpy(ctx->dashes, dashes, sizeof(float) * ctx->dashCount);
}
void vkvg_get_dash(VkvgContext ctx, const float* dashes, uint32_t* num_dashes, float* offset) {
	if (vkvg_status(ctx))
		return;
	*num_dashes = ctx->dashCount;
	*offset = ctx->dashOffset;
	if (ctx->dashCount == 0 || dashes == NULL)
		return;
	memcpy((float*)dashes, ctx->dashes, sizeof(float) * ctx->dashCount);
}

vkvg_line_cap_t vkvg_get_line_cap(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return (vkvg_line_cap_t)0;
	return ctx->lineCap;
}
vkvg_line_join_t vkvg_get_line_join(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return (vkvg_line_join_t)0;
	return ctx->lineJoin;
}
vkvg_operator_t vkvg_get_operator(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return VKVG_OPERATOR_OVER;
	return ctx->curOperator;
}
VkvgPattern vkvg_get_source(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return NULL;
	vkvg_pattern_reference(ctx->pattern);
	return ctx->pattern;
}

void vkvg_select_font_face(VkvgContext ctx, const char* name) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SET_FONT_FACE, name);
	_select_font_face(ctx, name);
}
void vkvg_load_font_from_path(VkvgContext ctx, const char* path, const char* name) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SET_FONT_PATH, name);
	_vkvg_font_identity_t* fid = _font_cache_add_font_identity(ctx, path, name);
	if (!_font_cache_load_font_file_in_memory(fid)) {
		ctx->status = VKVG_STATUS_FILE_NOT_FOUND;
		return;
	}
	_select_font_face(ctx, name);
}
void vkvg_load_font_from_memory(VkvgContext ctx, unsigned char* fontBuffer, long fontBufferByteSize, const char* name) {
	if (vkvg_status(ctx))
		return;
	// RECORD(ctx, VKVG_CMD_SET_FONT_PATH, name);
	_vkvg_font_identity_t* fid = _font_cache_add_font_identity(ctx, NULL, name);
	fid->fontBuffer = fontBuffer;
	fid->fontBufSize = fontBufferByteSize;

	_select_font_face(ctx, name);
}
void vkvg_set_font_size(VkvgContext ctx, uint32_t size) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SET_FONT_SIZE, size);
#ifdef VKVG_USE_FREETYPE
	long newSize = size << 6;
#else
	long newSize = size;
#endif
	if (ctx->selectedCharSize == newSize)
		return;
	ctx->selectedCharSize = newSize;
	ctx->currentFont = NULL;
	ctx->currentFontSize = NULL;
}

void vkvg_set_text_direction(vkvg_context* ctx, vkvg_direction_t direction) {}

void vkvg_show_text(VkvgContext ctx, const char* text) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SHOW_TEXT, text);
	LOG(VKVG_LOG_INFO_CMD, "CMD: show_text:\n");
	//_ensure_renderpass_is_started(ctx);
	_font_cache_show_text(ctx, text);
	//_flush_undrawn_vertices (ctx);
}

VkvgText vkvg_text_run_create(VkvgContext ctx, const char* text) {
	if (vkvg_status(ctx))
		return NULL;
	VkvgText tr = (vkvg_text_run_t*)calloc(1, sizeof(vkvg_text_run_t));
	_font_cache_create_text_run(ctx, text, -1, tr);
	return tr;
}
VkvgText vkvg_text_run_create_with_length(VkvgContext ctx, const char* text, uint32_t length) {
	if (vkvg_status(ctx))
		return NULL;
	VkvgText tr = (vkvg_text_run_t*)calloc(1, sizeof(vkvg_text_run_t));
	_font_cache_create_text_run(ctx, text, length, tr);
	return tr;
}
uint32_t vkvg_text_run_get_glyph_count(VkvgText textRun) { return textRun->glyph_count; }
void     vkvg_text_run_get_glyph_position(VkvgText textRun, uint32_t index, vkvg_glyph_info_t* pGlyphInfo) {
	if (index >= textRun->glyph_count) {
		*pGlyphInfo = {};// (vkvg_glyph_info_t) { 0 };
		return;
	}
#if VKVG_USE_HARFBUZZ_CX
	memcpy(pGlyphInfo, &textRun->glyphs[index], sizeof(vkvg_glyph_info_t));
#else
	* pGlyphInfo = textRun->glyphs[index];
#endif
}
void vkvg_text_run_destroy(VkvgText textRun) {
	_font_cache_destroy_text_run(textRun);
	free(textRun);
}
void vkvg_show_text_run(VkvgContext ctx, VkvgText textRun) {
	if (vkvg_status(ctx))
		return;
	_font_cache_show_text_run(ctx, textRun);
}
void vkvg_text_run_get_extents(VkvgText textRun, vkvg_text_extents_t* extents) { *extents = textRun->extents; }

void vkvg_text_extents(VkvgContext ctx, const char* text, vkvg_text_extents_t* extents) {
	if (vkvg_status(ctx))
		return;
	_font_cache_text_extents(ctx, text, -1, extents);
}
void vkvg_font_extents(VkvgContext ctx, vkvg_font_extents_t* extents) {
	if (vkvg_status(ctx))
		return;
	_font_cache_font_extents(ctx, extents);
}

void vkvg_save(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SAVE);
	LOG(VKVG_LOG_INFO, "SAVE CONTEXT: ctx = %p\n", ctx);

	VkvgDevice           dev = ctx->dev;
	vkvg_context_save_t* sav = (vkvg_context_save_t*)calloc(1, sizeof(vkvg_context_save_t));

	_flush_cmd_buff(ctx);

	//if (!_wait_ctx_flush_end(ctx)) {
	//	free(sav);
	//	return;
	//}

	if (ctx->curClipState == vkvg_clip_state_clip) {
		sav->clippingState = vkvg_clip_state_clip_saved;

		uint8_t curSaveStencil = ctx->curSavBit / 6;

		if (ctx->curSavBit > 0 && ctx->curSavBit % 6 == 0) { // new save/restore stencil image have to be created
			VkhImage* savedStencilsPtr = NULL;
			if (savedStencilsPtr)
				savedStencilsPtr = (VkhImage*)realloc(ctx->savedStencils, curSaveStencil * sizeof(VkhImage));
			else
				savedStencilsPtr = (VkhImage*)malloc(curSaveStencil * sizeof(VkhImage));
			if (savedStencilsPtr == NULL) {
				free(sav);
				ctx->status = VKVG_STATUS_NO_MEMORY;
				return;
			}
			ctx->savedStencils = savedStencilsPtr;
			VkhImage savStencil = vkh_image_ms_create(
				(VkhDevice)&ctx->dev->vkDev, dev->stencilFormat, (VkSampleCountFlagBits)dev->samples, ctx->pSurf->width, ctx->pSurf->height,
				VKH_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
			ctx->savedStencils[curSaveStencil - 1] = savStencil;

			vkh_cmd_begin(ctx->cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			ctx->cmdStarted = true;

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
			vkh_cmd_label_start(ctx->cmd, "new save/restore stencil", DBG_LAB_COLOR_SAV);
#endif

			vkh_image_set_layout(ctx->cmd, ctx->pSurf->stencil, dev->stencilAspectFlag,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			vkh_image_set_layout(ctx->cmd, savStencil, dev->stencilAspectFlag, VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT);

			VkImageCopy cregion = { .srcSubresource = {VK_IMAGE_ASPECT_STENCIL_BIT, 0, 0, 1},
								   .dstSubresource = {VK_IMAGE_ASPECT_STENCIL_BIT, 0, 0, 1},
								   .extent = {ctx->pSurf->width, ctx->pSurf->height, 1} };
			vkCmdCopyImage(ctx->cmd, vkh_image_get_vkimage(ctx->pSurf->stencil), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				vkh_image_get_vkimage(savStencil), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cregion);

			vkh_image_set_layout(ctx->cmd, ctx->pSurf->stencil, dev->stencilAspectFlag,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
			vkh_cmd_label_end(ctx->cmd);
#endif

			VK_CHECK_RESULT(vkEndCommandBuffer(ctx->cmd));
			_wait_and_submit_cmd(ctx);
		}

		uint8_t curSaveBit = 1 << (ctx->curSavBit % 6 + 2);

		_start_cmd_for_render_pass(ctx);

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
		vkh_cmd_label_start(ctx->cmd, "save rp", DBG_LAB_COLOR_SAV);
#endif

		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelineClipping);

		CmdSetStencilReference(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT | curSaveBit);
		CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
		CmdSetStencilWriteMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, curSaveBit);

		_draw_full_screen_quad(ctx, NULL);

		_bind_draw_pipeline(ctx);
		CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
		vkh_cmd_label_end(ctx->cmd);
#endif
		ctx->curSavBit++;
	}
	else if (ctx->curClipState == vkvg_clip_state_none)
		sav->clippingState = (vkvg_clip_state_t)(_get_previous_clip_state(ctx) & 0x03);
	else
		sav->clippingState = vkvg_clip_state_clear;

	sav->dashOffset = ctx->dashOffset;
	sav->dashCount = ctx->dashCount;
	if (ctx->dashCount > 0) {
		sav->dashes = (float*)malloc(sizeof(float) * ctx->dashCount);
		memcpy(sav->dashes, ctx->dashes, sizeof(float) * ctx->dashCount);
	}
	sav->lineWidth = ctx->lineWidth;
	sav->miterLimit = ctx->miterLimit;
	sav->curOperator = ctx->curOperator;
	sav->lineCap = ctx->lineCap;
	sav->lineWidth = ctx->lineWidth;
	sav->curFillRule = ctx->curFillRule;

	sav->selectedCharSize = ctx->selectedCharSize;
	strcpy(sav->selectedFontName, ctx->selectedFontName);

	sav->currentFont = ctx->currentFont;
	sav->textDirection = ctx->textDirection;
	sav->pushConsts = ctx->pushConsts;
	if (ctx->pattern) {
		sav->pattern = ctx->pattern; // TODO:pattern sav must be imutable (copy?)
		vkvg_pattern_reference(ctx->pattern);
	}
	else
		sav->curColor = ctx->curColor;

	sav->pNext = ctx->pSavedCtxs;
	ctx->pSavedCtxs = sav;
}
void vkvg_restore(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;

	RECORD(ctx, VKVG_CMD_RESTORE);

	if (ctx->pSavedCtxs == NULL) {
		ctx->status = VKVG_STATUS_INVALID_RESTORE;
		return;
	}

	LOG(VKVG_LOG_INFO, "RESTORE CONTEXT: ctx = %p\n", ctx);

	vkvg_context_save_t* sav = ctx->pSavedCtxs;
	ctx->pSavedCtxs = sav->pNext;

	_flush_cmd_buff(ctx);

	//if (!_wait_ctx_flush_end(ctx))
	//	return;

	ctx->pushConsts = sav->pushConsts;
	ctx->pushCstDirty = true;

	if (ctx->curClipState) { //!=none
		if (ctx->curClipState == vkvg_clip_state_clip && sav->clippingState == vkvg_clip_state_clear) {
			_reset_clip(ctx);
		}
		else {

			uint8_t curSaveBit = 1 << ((ctx->curSavBit - 1) % 6 + 2);

			_start_cmd_for_render_pass(ctx);

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
			vkh_cmd_label_start(ctx->cmd, "restore rp", DBG_LAB_COLOR_SAV);
#endif

			CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelineClipping);

			CmdSetStencilReference(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT | curSaveBit);
			CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, curSaveBit);
			CmdSetStencilWriteMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);

			_draw_full_screen_quad(ctx, NULL);

			_bind_draw_pipeline(ctx);
			CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
			vkh_cmd_label_end(ctx->cmd);
#endif

			_flush_cmd_buff(ctx);
			if (!_wait_ctx_flush_end(ctx))
				return;
		}
	}
	if (sav->clippingState == vkvg_clip_state_clip_saved) {
		ctx->curSavBit--;

		uint8_t curSaveStencil = ctx->curSavBit / 6;
		if (ctx->curSavBit > 0 &&
			ctx->curSavBit % 6 ==
			0) { // addtional save/restore stencil image have to be copied back to surf stencil first
			VkhImage savStencil = ctx->savedStencils[curSaveStencil - 1];

			vkh_cmd_begin(ctx->cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			ctx->cmdStarted = true;

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
			vkh_cmd_label_start(ctx->cmd, "additional stencil copy while restoring", DBG_LAB_COLOR_SAV);
#endif

			vkh_image_set_layout(ctx->cmd, ctx->pSurf->stencil, ctx->dev->stencilAspectFlag,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			vkh_image_set_layout(ctx->cmd, savStencil, ctx->dev->stencilAspectFlag,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

			VkImageCopy cregion = { .srcSubresource = {VK_IMAGE_ASPECT_STENCIL_BIT, 0, 0, 1},
								   .dstSubresource = {VK_IMAGE_ASPECT_STENCIL_BIT, 0, 0, 1},
								   .extent = {ctx->pSurf->width, ctx->pSurf->height, 1} };
			vkCmdCopyImage(ctx->cmd, vkh_image_get_vkimage(savStencil), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				vkh_image_get_vkimage(ctx->pSurf->stencil), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
				&cregion);
			vkh_image_set_layout(ctx->cmd, ctx->pSurf->stencil, ctx->dev->stencilAspectFlag,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
			vkh_cmd_label_end(ctx->cmd);
#endif

			VK_CHECK_RESULT(vkEndCommandBuffer(ctx->cmd));
			_wait_and_submit_cmd(ctx);
			if (!_wait_ctx_flush_end(ctx))
				return;
			vkh_image_destroy(savStencil);
		}
	}

	ctx->curClipState = vkvg_clip_state_none;

	ctx->dashOffset = sav->dashOffset;
	if (ctx->dashCount > 0)
		free(ctx->dashes);
	ctx->dashCount = sav->dashCount;
	if (ctx->dashCount > 0) {
		ctx->dashes = (float*)malloc(sizeof(float) * ctx->dashCount);
		memcpy(ctx->dashes, sav->dashes, sizeof(float) * ctx->dashCount);
	}

	ctx->lineWidth = sav->lineWidth;
	ctx->miterLimit = sav->miterLimit;
	ctx->curOperator = sav->curOperator;
	ctx->lineCap = sav->lineCap;
	ctx->lineJoin = sav->lineJoin;
	ctx->curFillRule = sav->curFillRule;

	ctx->selectedCharSize = sav->selectedCharSize;
	strcpy(ctx->selectedFontName, sav->selectedFontName);

	ctx->currentFont = sav->currentFont;
	ctx->textDirection = sav->textDirection;

	if (sav->pattern) {
		if (sav->pattern != ctx->pattern)
			_update_cur_pattern(ctx, sav->pattern);
		else
			vkvg_pattern_destroy(sav->pattern);
	}
	else {
		ctx->curColor = sav->curColor;
		_update_cur_pattern(ctx, NULL);
	}

	_free_ctx_save(sav);
}

void vkvg_translate(VkvgContext ctx, float dx, float dy) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_TRANSLATE, dx, dy);
	LOG(VKVG_LOG_INFO_CMD, "CMD: translate: %f, %f\n", dx, dy);
	_emit_draw_cmd_undrawn_vertices(ctx);
	vkvg_matrix_translate(&ctx->pushConsts.mat, dx, dy);
	_set_mat_inv_and_vkCmdPush(ctx);
}
void vkvg_scale(VkvgContext ctx, float sx, float sy) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SCALE, sx, sy);
	LOG(VKVG_LOG_INFO_CMD, "CMD: scale: %f, %f\n", sx, sy);
	_emit_draw_cmd_undrawn_vertices(ctx);
	vkvg_matrix_scale(&ctx->pushConsts.mat, sx, sy);
	_set_mat_inv_and_vkCmdPush(ctx);
}
void vkvg_rotate(VkvgContext ctx, float radians) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_ROTATE, radians);
	LOG(VKVG_LOG_INFO_CMD, "CMD: rotate: %f\n", radians);
	_emit_draw_cmd_undrawn_vertices(ctx);
	vkvg_matrix_rotate(&ctx->pushConsts.mat, radians);
	_set_mat_inv_and_vkCmdPush(ctx);
}
void vkvg_transform(VkvgContext ctx, const vkvg_matrix_t* matrix) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_TRANSFORM, matrix);
	LOG(VKVG_LOG_INFO_CMD, "CMD: transform: %f, %f, %f, %f, %f, %f\n", matrix->xx, matrix->yx, matrix->xy, matrix->yy,
		matrix->x0, matrix->y0);
	_emit_draw_cmd_undrawn_vertices(ctx);
	vkvg_matrix_t res;
	vkvg_matrix_multiply(&res, &ctx->pushConsts.mat, matrix);
	ctx->pushConsts.mat = res;
	_set_mat_inv_and_vkCmdPush(ctx);
}
void vkvg_identity_matrix(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_IDENTITY_MATRIX);
	LOG(VKVG_LOG_INFO_CMD, "CMD: identity_matrix:\n");
	_emit_draw_cmd_undrawn_vertices(ctx);
	vkvg_matrix_t im = VKVG_IDENTITY_MATRIX;
	ctx->pushConsts.mat = im;
	_set_mat_inv_and_vkCmdPush(ctx);
}
void vkvg_set_matrix(VkvgContext ctx, const vkvg_matrix_t* matrix) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_SET_MATRIX, matrix);
	LOG(VKVG_LOG_INFO_CMD, "CMD: set_matrix: %f, %f, %f, %f, %f, %f\n", matrix->xx, matrix->yx, matrix->xy, matrix->yy,
		matrix->x0, matrix->y0);
	_emit_draw_cmd_undrawn_vertices(ctx);
	ctx->pushConsts.mat = (*matrix);
	_set_mat_inv_and_vkCmdPush(ctx);
}
void vkvg_get_matrix(VkvgContext ctx, vkvg_matrix_t* const matrix) {
	if (vkvg_status(ctx) || !matrix)
		return;
	*matrix = ctx->pushConsts.mat;
}

void vkvg_elliptic_arc_to(VkvgContext ctx, float x2, float y2, bool largeArc, bool sweepFlag, float rx, float ry,
	float phi) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_ELLIPTICAL_ARC_TO, x2, y2, rx, ry, phi, largeArc, sweepFlag);
	LOG(VKVG_LOG_INFO_CMD,
		"\tCMD: elliptic_arc_to: x2:%10.5f y2:%10.5f large:%d sweep:%d rx:%10.5f ry:%10.5f phi:%10.5f \n", x2, y2,
		largeArc, sweepFlag, rx, ry, phi);
	float x1, y1;
	vkvg_get_current_point(ctx, &x1, &y1);
	_elliptic_arc(ctx, x1, y1, x2, y2, largeArc, sweepFlag, rx, ry, phi);
}
void vkvg_rel_elliptic_arc_to(VkvgContext ctx, float x2, float y2, bool largeArc, bool sweepFlag, float rx, float ry,
	float phi) {
	if (vkvg_status(ctx))
		return;
	RECORD(ctx, VKVG_CMD_REL_ELLIPTICAL_ARC_TO, x2, y2, rx, ry, phi, largeArc, sweepFlag);
	LOG(VKVG_LOG_INFO_CMD,
		"\tCMD: rel_elliptic_arc_to: x2:%10.5f y2:%10.5f large:%d sweep:%d rx:%10.5f ry:%10.5f phi:%10.5f \n", x2, y2,
		largeArc, sweepFlag, rx, ry, phi);

	float x1, y1;
	vkvg_get_current_point(ctx, &x1, &y1);
	_elliptic_arc(ctx, x1, y1, x2 + x1, y2 + y1, largeArc, sweepFlag, rx, ry, phi);
}

void vkvg_ellipse(VkvgContext ctx, float radiusX, float radiusY, float x, float y, float rotationAngle) {
	if (vkvg_status(ctx))
		return;
	LOG(VKVG_LOG_INFO_CMD, "CMD: ellipse:\n");

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

	_finish_path(ctx);
	_add_point(ctx, bottomCenterX, bottomCenterY);

	_curve_to(ctx, bottomRightX, bottomRightY, topRightX, topRightY, topCenterX, topCenterY);
	_curve_to(ctx, topLeftX, topLeftY, bottomLeftX, bottomLeftY, bottomCenterX, bottomCenterY);

	ctx->pathes[ctx->pathPtr] |= PATH_CLOSED_BIT;
	_finish_path(ctx);
}

VkvgSurface vkvg_get_target(VkvgContext ctx) {
	if (vkvg_status(ctx))
		return NULL;
	return ctx->pSurf;
}

const char* vkvg_status_to_string(vkvg_status_t status) {
	switch (status) {
	case VKVG_STATUS_SUCCESS:
		return "no error has occurred";
		/*case VKVG_STATUS_NO_MEMORY:
			return "out of memory";*/
	case VKVG_STATUS_INVALID_RESTORE:
		return "vkvg_restore() without matching vkvg_save()";
	case VKVG_STATUS_NO_CURRENT_POINT:
		return "no current point defined";
	case VKVG_STATUS_INVALID_MATRIX:
		return "invalid matrix (not invertible)";
	case VKVG_STATUS_INVALID_STATUS:
		return "invalid value for an input vkvg_status_t";
	case VKVG_STATUS_INVALID_INDEX:
		return "invalid index passed to getter";
	case VKVG_STATUS_NULL_POINTER:
		return "NULL pointer";
	case VKVG_STATUS_WRITE_ERROR:
		return "error while writing to output stream";
	case VKVG_STATUS_PATTERN_TYPE_MISMATCH:
		return "the pattern type is not appropriate for the operation";
	case VKVG_STATUS_PATTERN_INVALID_GRADIENT:
		return "the stops count is zero";
	case VKVG_STATUS_INVALID_FORMAT:
		return "invalid value for an input vkvg_format_t";
	case VKVG_STATUS_FILE_NOT_FOUND:
		return "file not found";
	case VKVG_STATUS_INVALID_DASH:
		return "invalid value for a dash setting";
	case VKVG_STATUS_INVALID_RECT:
		return "a rectangle has the height or width equal to 0";
	case VKVG_STATUS_TIMEOUT:
		return "waiting for a Vulkan operation to finish resulted in a fence timeout (5 seconds)";
	case VKVG_STATUS_DEVICE_ERROR:
		return "the initialization of the device resulted in an error";
	case VKVG_STATUS_INVALID_IMAGE:
		return "invalid image";
	case VKVG_STATUS_INVALID_SURFACE:
		return "invalid surface";
	case VKVG_STATUS_INVALID_FONT:
		return "unresolved font name";
	default:
		return "<unknown error status>";
	}
}


void _resize_vertex_cache(VkvgContext ctx, uint32_t newSize) {
	Vertex* tmp = (Vertex*)realloc(ctx->vertexCache, (size_t)newSize * sizeof(Vertex));
	LOG(VKVG_LOG_DBG_ARRAYS,
		"resize vertex cache (vx count=%u): old size: %u -> new size: %u size(byte): %zu Ptr: %p -> %p\n",
		ctx->vertCount, ctx->sizeVertices, newSize, (size_t)newSize * sizeof(Vertex), ctx->vertexCache, tmp);
	if (tmp == NULL) {
		ctx->status = VKVG_STATUS_NO_MEMORY;
		LOG(VKVG_LOG_ERR, "resize vertex cache failed: vert count: %u byte size: %zu\n", newSize,
			newSize * sizeof(Vertex));
		return;
	}
	ctx->vertexCache = tmp;
	ctx->sizeVertices = newSize;
}
void _resize_index_cache(VkvgContext ctx, uint32_t newSize) {
	VKVG_IBO_INDEX_TYPE* tmp =
		(VKVG_IBO_INDEX_TYPE*)realloc(ctx->indexCache, (size_t)newSize * sizeof(VKVG_IBO_INDEX_TYPE));
	LOG(VKVG_LOG_DBG_ARRAYS, "resize IBO: new size: %lu Ptr: %p -> %p\n", (size_t)newSize * sizeof(VKVG_IBO_INDEX_TYPE),
		ctx->indexCache, tmp);
	if (tmp == NULL) {
		ctx->status = VKVG_STATUS_NO_MEMORY;
		LOG(VKVG_LOG_ERR, "resize IBO failed: idx count: %u size(byte): %zu\n", newSize,
			(size_t)newSize * sizeof(VKVG_IBO_INDEX_TYPE));
		return;
	}
	ctx->indexCache = tmp;
	ctx->sizeIndices = newSize;
}
void _ensure_vertex_cache_size(VkvgContext ctx, uint32_t addedVerticesCount) {
	if (ctx->sizeVertices - ctx->vertCount > VKVG_ARRAY_THRESHOLD + addedVerticesCount)
		return;
	uint32_t newSize = ctx->sizeVertices + addedVerticesCount;
	uint32_t modulo = addedVerticesCount % VKVG_VBO_SIZE;
	if (modulo > 0)
		newSize += VKVG_VBO_SIZE - modulo;
	_resize_vertex_cache(ctx, newSize);
}
void _check_vertex_cache_size(VkvgContext ctx) {
	assert(ctx->sizeVertices > ctx->vertCount);
	if (ctx->sizeVertices - VKVG_ARRAY_THRESHOLD > ctx->vertCount)
		return;
	_resize_vertex_cache(ctx, ctx->sizeVertices + VKVG_VBO_SIZE);
}
void _ensure_index_cache_size(VkvgContext ctx, uint32_t addedIndicesCount) {
	assert(ctx->sizeIndices > ctx->indCount);
	if (ctx->sizeIndices - VKVG_ARRAY_THRESHOLD > ctx->indCount + addedIndicesCount)
		return;
	uint32_t newSize = ctx->sizeIndices + addedIndicesCount;
	uint32_t modulo = addedIndicesCount % VKVG_IBO_SIZE;
	if (modulo > 0)
		newSize += VKVG_IBO_SIZE - modulo;
	_resize_index_cache(ctx, newSize);
}
void _check_index_cache_size(VkvgContext ctx) {
	if (ctx->sizeIndices - VKVG_ARRAY_THRESHOLD > ctx->indCount)
		return;
	_resize_index_cache(ctx, ctx->sizeIndices + VKVG_IBO_SIZE);
}
// check host path array size, return true if error. pathPtr is already incremented
bool _check_pathes_array(VkvgContext ctx) {
	if (ctx->sizePathes - ctx->pathPtr - ctx->segmentPtr > VKVG_ARRAY_THRESHOLD)
		return false;
	ctx->sizePathes += VKVG_PATHES_SIZE;
	uint32_t* tmp = (uint32_t*)realloc(ctx->pathes, (size_t)ctx->sizePathes * sizeof(uint32_t));
	LOG(VKVG_LOG_DBG_ARRAYS, "resize PATH: new size: %u Ptr: %p -> %p\n", ctx->sizePathes, ctx->pathes, tmp);
	if (tmp == NULL) {
		ctx->status = VKVG_STATUS_NO_MEMORY;
		LOG(VKVG_LOG_ERR, "resize PATH failed: new size(byte): %zu\n", ctx->sizePathes * sizeof(uint32_t));
		_clear_path(ctx);
		return true;
	}
	ctx->pathes = tmp;
	return false;
}
// check host point array size, return true if error
bool _check_point_array(VkvgContext ctx) {
	if (ctx->sizePoints - VKVG_ARRAY_THRESHOLD > ctx->pointCount)
		return false;
	ctx->sizePoints += VKVG_PTS_SIZE;
	vec2* tmp = (vec2*)realloc(ctx->points, (size_t)ctx->sizePoints * sizeof(vec2));
	LOG(VKVG_LOG_DBG_ARRAYS, "resize Points: new size(point): %u Ptr: %p -> %p\n", ctx->sizePoints, ctx->points, tmp);
	if (tmp == NULL) {
		ctx->status = VKVG_STATUS_NO_MEMORY;
		LOG(VKVG_LOG_ERR, "resize PATH failed: new size(byte): %zu\n", ctx->sizePoints * sizeof(vec2));
		_clear_path(ctx);
		return true;
	}
	ctx->points = tmp;
	return false;
}
bool _current_path_is_empty(VkvgContext ctx) { return ctx->pathes[ctx->pathPtr] == 0; }
// this function expect that current point exists
vec2 _get_current_position(VkvgContext ctx) { return ctx->points[ctx->pointCount - 1]; }
// set curve start point and set path has curve bit
void _set_curve_start(VkvgContext ctx) {
	if (ctx->segmentPtr > 0) {
		// check if current segment has points (straight)
		if ((ctx->pathes[ctx->pathPtr + ctx->segmentPtr] & PATH_ELT_MASK) > 0)
			ctx->segmentPtr++;
	}
	else {
		// not yet segmented path, first segment length is copied
		if (ctx->pathes[ctx->pathPtr] > 0) { // create first straight segment first
			ctx->pathes[ctx->pathPtr + 1] = ctx->pathes[ctx->pathPtr];
			ctx->segmentPtr = 2;
		}
		else
			ctx->segmentPtr = 1;
	}
	_check_pathes_array(ctx);
	ctx->pathes[ctx->pathPtr + ctx->segmentPtr] = 0;
}
// compute segment length and set is curved bit
void _set_curve_end(VkvgContext ctx) {
	// ctx->pathes [ctx->pathPtr + ctx->segmentPtr] = ctx->pathes [ctx->pathPtr] - ctx->pathes [ctx->pathPtr +
	// ctx->segmentPtr];
	ctx->pathes[ctx->pathPtr + ctx->segmentPtr] |= PATH_HAS_CURVES_BIT;
	ctx->segmentPtr++;
	_check_pathes_array(ctx);
	ctx->pathes[ctx->pathPtr + ctx->segmentPtr] = 0;
}
// path start pointed at ptrPath has curve bit
bool _path_has_curves(VkvgContext ctx, uint32_t ptrPath) { return ctx->pathes[ptrPath] & PATH_HAS_CURVES_BIT; }
void _finish_path(VkvgContext ctx) {
	if (ctx->pathes[ctx->pathPtr] == 0) // empty
		return;
	if ((ctx->pathes[ctx->pathPtr] & PATH_ELT_MASK) < 2) {
		// only current pos is in path
		ctx->pointCount -= ctx->pathes[ctx->pathPtr]; // what about the bounds?
		ctx->pathes[ctx->pathPtr] = 0;
		ctx->segmentPtr = 0;
		return;
	}

	LOG(VKVG_LOG_INFO_PATH, "PATH: points count=%10d\n", ctx->pathes[ctx->pathPtr] & PATH_ELT_MASK);

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

	if (_check_pathes_array(ctx))
		return;

	ctx->pathes[ctx->pathPtr] = 0;
	ctx->segmentPtr = 0;
	ctx->subpathCount++;
	ctx->simpleConvex = false;
}
// clear path datas in context
void _clear_path(VkvgContext ctx) {
	ctx->pathPtr = 0;
	ctx->pathes[ctx->pathPtr] = 0;
	ctx->pointCount = 0;
	ctx->segmentPtr = 0;
	ctx->subpathCount = 0;
	ctx->simpleConvex = false;
}
void _remove_last_point(VkvgContext ctx) {
	ctx->pathes[ctx->pathPtr]--;
	ctx->pointCount--;
	if (ctx->segmentPtr > 0) {                            // if path is segmented
		if (!ctx->pathes[ctx->pathPtr + ctx->segmentPtr]) // if current segment is empty
			ctx->segmentPtr--;
		ctx->pathes[ctx->pathPtr + ctx->segmentPtr]--;                          // decrement last segment point count
		if ((ctx->pathes[ctx->pathPtr + ctx->segmentPtr] & PATH_ELT_MASK) == 0) // if no point left (was only one)
			ctx->pathes[ctx->pathPtr + ctx->segmentPtr] = 0;                    // reset current segment
		else if (ctx->pathes[ctx->pathPtr + ctx->segmentPtr] & PATH_HAS_CURVES_BIT) // if segment is a curve
			ctx->segmentPtr++; // then segPtr has to be forwarded to new segment
	}
}
bool _path_is_closed(VkvgContext ctx, uint32_t ptrPath) { return ctx->pathes[ptrPath] & PATH_CLOSED_BIT; }
void _add_point(VkvgContext ctx, float x, float y) {
	if (_check_point_array(ctx))
		return;
	if (isnan(x) || isnan(y)) {
		LOG(VKVG_LOG_DEBUG, "_add_point: (%f, %f)\n", x, y);
		return;
	}
	vec2 v = { x, y };
	/*if (!_current_path_is_empty(ctx) && vec2_length(vec2_sub(ctx->points[ctx->pointCount-1], v))<1.f)
		return;*/
	LOG(VKVG_LOG_INFO_PTS, "_add_point: (%f, %f)\n", x, y);

	ctx->points[ctx->pointCount] = v;
	ctx->pointCount++;           // total point count of pathes, (for array bounds check)
	ctx->pathes[ctx->pathPtr]++; // total point count in path
	if (ctx->segmentPtr > 0)
		ctx->pathes[ctx->pathPtr + ctx->segmentPtr]++; // total point count in path's segment
}
float _normalizeAngle(float a) {
	float res = ROUND_DOWN(fmodf(a, 2.0f * M_PIF), 100);
	if (res < 0.0f)
		res += 2.0f * M_PIF;
	return res;
}
float _get_arc_step(VkvgContext ctx, float radius) {
	float sx, sy;
	vkvg_matrix_get_scale(&ctx->pushConsts.mat, &sx, &sy);
	float r = radius * fabsf(fmaxf(sx, sy));
	if (r < 30.0f)
		return fminf(M_PIF / 3.f, M_PIF / r);
	return fminf(M_PIF / 3.f, M_PIF / (r * 0.4f));
}
template <typename T>
static inline T vgAlignUp(T val, T align)
{
	return (val + align - 1) / align * align;
}
void _create_gradient_buff(VkvgContext ctx) {
	int us = sizeof(vkvg_gradient_t) * 8;
	vkh_buffer_init((VkhDevice)&ctx->dev->vkDev, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VKH_MEMORY_USAGE_CPU_TO_GPU,
		vgAlignUp(us, 64), &ctx->uboGrad, true);
	ctx->gCount = 8;
}
void _create_vertices_buff(VkvgContext ctx) {
	vkh_buffer_init((VkhDevice)&ctx->dev->vkDev, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VKH_MEMORY_USAGE_CPU_TO_GPU,
		ctx->sizeVBO * sizeof(Vertex), &ctx->vertices, true);
	vkh_buffer_init((VkhDevice)&ctx->dev->vkDev, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VKH_MEMORY_USAGE_CPU_TO_GPU,
		ctx->sizeIBO * sizeof(VKVG_IBO_INDEX_TYPE), &ctx->indices, true);
}
void _resize_ubo(VkvgContext ctx, uint32_t new_count) {
	if (!_wait_ctx_flush_end(ctx)) // wait previous cmd if not completed
		return;
	if (ctx->gCount < new_count)
	{
		ctx->gCount = new_count;
		int us = ctx->gCount * sizeof(vkvg_gradient_t);
		vkh_buffer_resize(&ctx->uboGrad, vgAlignUp(us, 64), true);
	}
}
void _resize_vbo(VkvgContext ctx, uint32_t new_size) {
	if (!_wait_ctx_flush_end(ctx)) // wait previous cmd if not completed
		return;
	LOG(VKVG_LOG_DBG_ARRAYS, "resize VBO: %d -> ", ctx->sizeVBO);
	ctx->sizeVBO = new_size;
	uint32_t mod = ctx->sizeVBO % VKVG_VBO_SIZE;
	if (mod > 0)
		ctx->sizeVBO += VKVG_VBO_SIZE - mod;
	LOG(VKVG_LOG_DBG_ARRAYS, "%d\n", ctx->sizeVBO);
	vkh_buffer_resize(&ctx->vertices, ctx->sizeVBO * sizeof(Vertex), true);
}
void _resize_ibo(VkvgContext ctx, size_t new_size) {
	if (!_wait_ctx_flush_end(ctx)) // wait previous cmd if not completed
		return;
	ctx->sizeIBO = new_size;
	uint32_t mod = ctx->sizeIBO % VKVG_IBO_SIZE;
	if (mod > 0)
		ctx->sizeIBO += VKVG_IBO_SIZE - mod;
	LOG(VKVG_LOG_DBG_ARRAYS, "resize IBO: new size: %d\n", ctx->sizeIBO);
	vkh_buffer_resize(&ctx->indices, ctx->sizeIBO * sizeof(VKVG_IBO_INDEX_TYPE), true);
}
void _add_vertexf(VkvgContext ctx, float x, float y) {
	Vertex* pVert = &ctx->vertexCache[ctx->vertCount];
	pVert->pos.x = x;
	pVert->pos.y = y;
	pVert->color = ctx->curColor;
	pVert->uv.z = -1;
	LOG(VKVG_LOG_INFO_VBO, "Add Vertexf %10d: pos:(%10.4f, %10.4f) uv:(%10.4f,%10.4f,%10.4f) color:0x%.8x \n",
		ctx->vertCount, pVert->pos.x, pVert->pos.y, pVert->uv.x, pVert->uv.y, pVert->uv.z, pVert->color);
	ctx->vertCount++;
	_check_vertex_cache_size(ctx);
}
void _add_vertexf_unchecked(VkvgContext ctx, float x, float y) {
	Vertex* pVert = &ctx->vertexCache[ctx->vertCount];
	pVert->pos.x = x;
	pVert->pos.y = y;
	pVert->color = ctx->curColor;
	pVert->uv.z = -1;
	LOG(VKVG_LOG_INFO_VBO, "Add Vertexf %10d: pos:(%10.4f, %10.4f) uv:(%10.4f,%10.4f,%10.4f) color:0x%.8x \n",
		ctx->vertCount, pVert->pos.x, pVert->pos.y, pVert->uv.x, pVert->uv.y, pVert->uv.z, pVert->color);
	ctx->vertCount++;
}
void _add_vertex(VkvgContext ctx, Vertex v) {
	ctx->vertexCache[ctx->vertCount] = v;
	LOG(VKVG_LOG_INFO_VBO, "Add Vertex  %10d: pos:(%10.4f, %10.4f) uv:(%10.4f,%10.4f,%10.4f) color:0x%.8x \n",
		ctx->vertCount, v.pos.x, v.pos.y, v.uv.x, v.uv.y, v.uv.z, v.color);
	ctx->vertCount++;
	_check_vertex_cache_size(ctx);
}
void _set_vertex(VkvgContext ctx, uint32_t idx, Vertex v) { ctx->vertexCache[idx] = v; }
#ifdef VKVG_FILL_NZ_GLUTESS
void _add_indice(VkvgContext ctx, VKVG_IBO_INDEX_TYPE i) {
	ctx->indexCache[ctx->indCount++] = i;
	_check_index_cache_size(ctx);
}
void _add_indice_for_fan(VkvgContext ctx, VKVG_IBO_INDEX_TYPE i) {
	VKVG_IBO_INDEX_TYPE* inds = &ctx->indexCache[ctx->indCount];
	inds[0] = ctx->tesselator_fan_start;
	inds[1] = ctx->indexCache[ctx->indCount - 1];
	inds[2] = i;
	ctx->indCount += 3;
	_check_index_cache_size(ctx);
}
void _add_indice_for_strip(VkvgContext ctx, VKVG_IBO_INDEX_TYPE i, bool odd) {
	VKVG_IBO_INDEX_TYPE* inds = &ctx->indexCache[ctx->indCount];
	if (odd) {
		inds[0] = ctx->indexCache[ctx->indCount - 2];
		inds[1] = i;
		inds[2] = ctx->indexCache[ctx->indCount - 1];
	}
	else {
		inds[0] = ctx->indexCache[ctx->indCount - 1];
		inds[1] = ctx->indexCache[ctx->indCount - 2];
		inds[2] = i;
	}
	ctx->indCount += 3;
	_check_index_cache_size(ctx);
}
#endif
void _add_tri_indices_for_rect(VkvgContext ctx, VKVG_IBO_INDEX_TYPE i) {
	VKVG_IBO_INDEX_TYPE* inds = &ctx->indexCache[ctx->indCount];
	inds[0] = i;
	inds[1] = i + 2;
	inds[2] = i + 1;
	inds[3] = i + 1;
	inds[4] = i + 2;
	inds[5] = i + 3;
	ctx->indCount += 6;

	_check_index_cache_size(ctx);
	LOG(VKVG_LOG_INFO_IBO, "Rectangle IDX: %d %d %d | %d %d %d (count=%d)\n", inds[0], inds[1], inds[2], inds[3],
		inds[4], inds[5], ctx->indCount);
}
void _add_triangle_indices(VkvgContext ctx, VKVG_IBO_INDEX_TYPE i0, VKVG_IBO_INDEX_TYPE i1, VKVG_IBO_INDEX_TYPE i2) {
	VKVG_IBO_INDEX_TYPE* inds = &ctx->indexCache[ctx->indCount];
	inds[0] = i0;
	inds[1] = i1;
	inds[2] = i2;
	ctx->indCount += 3;

	_check_index_cache_size(ctx);
	LOG(VKVG_LOG_INFO_IBO, "Triangle IDX: %d %d %d (indCount=%d)\n", i0, i1, i2, ctx->indCount);
}
void _add_triangle_indices_unchecked(VkvgContext ctx, VKVG_IBO_INDEX_TYPE i0, VKVG_IBO_INDEX_TYPE i1,
	VKVG_IBO_INDEX_TYPE i2) {
	VKVG_IBO_INDEX_TYPE* inds = &ctx->indexCache[ctx->indCount];
	inds[0] = i0;
	inds[1] = i1;
	inds[2] = i2;
	ctx->indCount += 3;

	LOG(VKVG_LOG_INFO_IBO, "Triangle IDX: %d %d %d (indCount=%d)\n", i0, i1, i2, ctx->indCount);
}
void _vao_add_rectangle(VkvgContext ctx, float x, float y, float width, float height) {
	Vertex              v[4] = { {{x, y}, ctx->curColor, {0, 0, -1}},
									{{x, y + height}, ctx->curColor, {0, 0, -1}},
									{{x + width, y}, ctx->curColor, {0, 0, -1}},
									{{x + width, y + height}, ctx->curColor, {0, 0, -1}} };
	VKVG_IBO_INDEX_TYPE firstIdx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);
	Vertex* pVert = &ctx->vertexCache[ctx->vertCount];
	memcpy(pVert, v, 4 * sizeof(Vertex));
	ctx->vertCount += 4;

	_check_vertex_cache_size(ctx);

	_add_tri_indices_for_rect(ctx, firstIdx);
}
// start render pass if not yet started or update push const if requested
// 如果还没开始渲染通道，就开始渲染通道；或者如果有请求，就更新推送常量
void _ensure_renderpass_is_started(VkvgContext ctx) {
	LOG(VKVG_LOG_INFO, "_ensure_renderpass_is_started\n");
	if (!ctx->cmdStarted)
		_start_cmd_for_render_pass(ctx);
	else if (ctx->pushCstDirty)
		_update_push_constants(ctx);
}
void _create_cmd_buff(VkvgContext ctx) {
	vkh_cmd_buffs_create((VkhDevice)&ctx->dev->vkDev, ctx->cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 2,
		ctx->cmdBuffers);
#if defined(DEBUG) && defined(ENABLE_VALIDATION)
	vkh_device_set_object_name((VkhDevice)&ctx->pSurf->dev->vkDev, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
		(uint64_t)ctx->cmd, "vkvgCtxCmd");
#endif
}
void _clear_attachment(VkvgContext ctx) {}

bool _wait_ctx_flush_end(VkvgContext ctx) {
	LOG(VKVG_LOG_INFO, "CTX: _wait_flush_fence\n");
	bool ret = false;
	c_runtime_cx rtc;
	rtc.begin();
	VkResult st = {};
#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	ret = (vkh_timeline_wait((VkhDevice)&ctx->dev->vkDev, ctx->pSurf->timeline, ctx->timelineStep) == VK_SUCCESS);
#else
	do
	{
		st = vkGetFenceStatus(ctx->dev->vkDev, ctx->flushFence);
	} while (0);
	ret = !st;
	if (st)
		ret = (WaitForFences(ctx->dev->vkDev, 1, &ctx->flushFence, VK_TRUE, VKVG_FENCE_TIMEOUT) == VK_SUCCESS);
#endif
	int ms = rtc.end();
#ifdef PWAIT_MS
	if (ms > 0)
		printf("wait ms: %d\n", ms);
#endif
	if (!ret) {
		LOG(VKVG_LOG_DEBUG, "CTX: _wait_flush_fence timeout\n");
		ctx->status = VKVG_STATUS_TIMEOUT;
	}
	return ret;
}

bool _wait_and_submit_cmd(VkvgContext ctx) {
	if (!ctx->cmdStarted) // current cmd buff is empty, be aware that wait is also canceled!!
		return true;

	LOG(VKVG_LOG_INFO, "CTX: _wait_and_submit_cmd\n");

#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	VkvgSurface surf = ctx->pSurf;
	VkvgDevice  dev = surf->dev;
	// vkh_timeline_wait ((VkhDevice)&dev->vkDev, surf->timeline, ct->timelineStep);
	if (ctx->pattern && ctx->pattern->type == VKVG_PATTERN_TYPE_SURFACE) {
		// add source surface timeline sync.
		VkvgSurface source = (VkvgSurface)ctx->pattern->data;
		VkSemaphore sems[2] = { surf->timeline, source->timeline };
		uint64_t waits[2] = { surf->timelineStep, source->timelineStep };
		uint64_t sigs[2] = { surf->timelineStep + 1, source->timelineStep + 1 };
		LOCK_SURFACE(surf)
			LOCK_SURFACE(source)
			LOCK_DEVICE
			vkh_cmd_submit_timelined2(dev->gQueue, &ctx->cmd, sems, waits, sigs);
		surf->timelineStep++;
		source->timelineStep++;
		ctx->timelineStep = surf->timelineStep;
		UNLOCK_DEVICE
			UNLOCK_SURFACE(source)
			UNLOCK_SURFACE(surf)
	}
	else {
		LOCK_SURFACE(surf)
			LOCK_DEVICE
			vkh_cmd_submit_timelined(dev->gQueue, &ctx->cmd, surf->timeline, surf->timelineStep, surf->timelineStep + 1);
		surf->timelineStep++;
		ctx->timelineStep = surf->timelineStep;
		UNLOCK_DEVICE
			UNLOCK_SURFACE(surf)
	}
#else

	if (!_wait_ctx_flush_end(ctx))
		return false;
	ResetFences(ctx->dev->vkDev, 1, &ctx->flushFence);
	_device_submit_cmd(ctx->dev, &ctx->cmd, ctx->flushFence);
#endif

	if (ctx->cmd == ctx->cmdBuffers[0])
		ctx->cmd = ctx->cmdBuffers[1];
	else
		ctx->cmd = ctx->cmdBuffers[0];

	ResetCommandBuffer(ctx->cmd, 0);
	ctx->cmdStarted = false;
	return true;
}

bool _submit_cmd(VkvgContext ctx) {
	if (!ctx->cmdStarted) // current cmd buff is empty, be aware that wait is also canceled!!
		return true;

	LOG(VKVG_LOG_INFO, "CTX: _wait_and_submit_cmd\n");

#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	VkvgSurface surf = ctx->pSurf;
	VkvgDevice  dev = surf->dev;
	// vkh_timeline_wait ((VkhDevice)&dev->vkDev, surf->timeline, ct->timelineStep);
	if (ctx->pattern && ctx->pattern->type == VKVG_PATTERN_TYPE_SURFACE) {
		// add source surface timeline sync.
		VkvgSurface source = (VkvgSurface)ctx->pattern->data;
		VkSemaphore sems[2] = { surf->timeline, source->timeline };
		uint64_t waits[2] = { surf->timelineStep, source->timelineStep };
		uint64_t sigs[2] = { surf->timelineStep + 1, source->timelineStep + 1 };
		LOCK_SURFACE(surf)
			LOCK_SURFACE(source)
			LOCK_DEVICE
			vkh_cmd_submit_timelined2(dev->gQueue, &ctx->cmd, sems, waits, sigs);
		surf->timelineStep++;
		source->timelineStep++;
		ctx->timelineStep = surf->timelineStep;
		UNLOCK_DEVICE
			UNLOCK_SURFACE(source)
			UNLOCK_SURFACE(surf)
	}
	else {
		LOCK_SURFACE(surf)
			LOCK_DEVICE
			vkh_cmd_submit_timelined(dev->gQueue, &ctx->cmd, surf->timeline, surf->timelineStep, surf->timelineStep + 1);
		surf->timelineStep++;
		ctx->timelineStep = surf->timelineStep;
		UNLOCK_DEVICE
			UNLOCK_SURFACE(surf)
	}
#else	 
	_device_submit_cmd(ctx->dev, &ctx->cmd, 0);
#endif

	if (ctx->cmd == ctx->cmdBuffers[0])
		ctx->cmd = ctx->cmdBuffers[1];
	else
		ctx->cmd = ctx->cmdBuffers[0];

	ResetCommandBuffer(ctx->cmd, 0);
	ctx->cmdStarted = false;
	return true;
}
/*void _explicit_ms_resolve (VkvgContext ctx){//should init cmd before calling this (unused, using automatic resolve by
renderpass) vkh_image_set_layout (ctx->cmd, ctx->pSurf->imgMS, VK_IMAGE_ASPECT_COLOR_BIT,
						  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	vkh_image_set_layout (ctx->cmd, ctx->pSurf->img, VK_IMAGE_ASPECT_COLOR_BIT,
						  VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

	VkImageResolve re = {
		.extent = {ctx->pSurf->width, ctx->pSurf->height,1},
		.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},
		.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT,0,0,1}
	};

	vkCmdResolveImage(ctx->cmd,
					  vkh_image_get_vkimage (ctx->pSurf->imgMS), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					  vkh_image_get_vkimage (ctx->pSurf->img) ,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					  1,&re);
	vkh_image_set_layout (ctx->cmd, ctx->pSurf->imgMS, VK_IMAGE_ASPECT_COLOR_BIT,
						  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ,
						  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}*/

// pre flush vertices because of vbo or ibo too small, all vertices except last draw call are flushed
// this function expects a vertex offset > 0
void _flush_vertices_caches_until_vertex_base(VkvgContext ctx) {
	_wait_ctx_flush_end(ctx);

	memcpy(vkh_buffer_get_mapped_pointer(&ctx->vertices), ctx->vertexCache, ctx->curVertOffset * sizeof(Vertex));
	memcpy(vkh_buffer_get_mapped_pointer(&ctx->indices), ctx->indexCache,
		ctx->curIndStart * sizeof(VKVG_IBO_INDEX_TYPE));

	// copy remaining vertices and indices to caches starts
	// this could be optimized at the cost of additional offsets.
	ctx->vertCount -= ctx->curVertOffset;
	ctx->indCount -= ctx->curIndStart;
	memcpy(ctx->vertexCache, &ctx->vertexCache[ctx->curVertOffset], ctx->vertCount * sizeof(Vertex));
	memcpy(ctx->indexCache, &ctx->indexCache[ctx->curIndStart], ctx->indCount * sizeof(VKVG_IBO_INDEX_TYPE));

	ctx->curVertOffset = 0;
	ctx->curIndStart = 0;
}
// copy vertex and index caches to the vbo and ibo vkbuffers used by gpu for drawing
// current running cmd has to be completed to free usage of those
void _flush_vertices_caches(VkvgContext ctx) {
	if (!_wait_ctx_flush_end(ctx))
	{
		return;
	}
	if (ctx->vertCount)
		memcpy(vkh_buffer_get_mapped_pointer(&ctx->vertices), ctx->vertexCache, ctx->vertCount * sizeof(Vertex));
	if (ctx->indCount)
		memcpy(vkh_buffer_get_mapped_pointer(&ctx->indices), ctx->indexCache, ctx->indCount * sizeof(VKVG_IBO_INDEX_TYPE));
	ctx->vertCount = ctx->indCount = ctx->curIndStart = ctx->curVertOffset = 0;
}
// this func expect cmdStarted to be true
void _end_render_pass(VkvgContext ctx) {
	LOG(VKVG_LOG_INFO, "END RENDER PASS: ctx = %p;\n", ctx);
	CmdEndRenderPass(ctx->cmd);
#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_cmd_label_end(ctx->cmd);
#endif
	ctx->renderPassBeginInfo.renderPass = ctx->dev->renderPass;
}

void _check_vao_size(VkvgContext ctx) {
	if (ctx->vertCount > ctx->sizeVBO || ctx->indCount > ctx->sizeIBO) {
		// vbo or ibo buffers too small
		if (ctx->cmdStarted)
			// if cmd is started buffers, are already bound, so no resize is possible
			// instead we flush, and clear vbo and ibo caches
			_flush_cmd_until_vx_base(ctx);
		if (ctx->vertCount > ctx->sizeVBO)
			_resize_vbo(ctx, ctx->sizeVertices);
		if (ctx->indCount > ctx->sizeIBO)
			_resize_ibo(ctx, ctx->sizeIndices);
	}
}

// stroke and non-zero draw call for solid color flush
void _emit_draw_cmd_undrawn_vertices(VkvgContext ctx) {
	if (ctx->indCount == ctx->curIndStart)
		return;

	_check_vao_size(ctx);

	_ensure_renderpass_is_started(ctx);

#ifdef VKVG_WIRED_DEBUG
	if (vkvg_wired_debug & vkvg_wired_debug_mode_normal)
		CmdDrawIndexed(ctx->cmd, ctx->indCount - ctx->curIndStart, 1, ctx->curIndStart, (int32_t)ctx->curVertOffset, 0);
	if (vkvg_wired_debug & vkvg_wired_debug_mode_lines) {
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pSurf->dev->pipelineLineList);
		CmdDrawIndexed(ctx->cmd, ctx->indCount - ctx->curIndStart, 1, ctx->curIndStart, (int32_t)ctx->curVertOffset, 0);
	}
	if (vkvg_wired_debug & vkvg_wired_debug_mode_points) {
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pSurf->dev->pipelineWired);
		CmdDrawIndexed(ctx->cmd, ctx->indCount - ctx->curIndStart, 1, ctx->curIndStart, (int32_t)ctx->curVertOffset, 0);
	}
	if (vkvg_wired_debug & vkvg_wired_debug_mode_both)
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pSurf->dev->pipe_OVER);
#else
	CmdDrawIndexed(ctx->cmd, ctx->indCount - ctx->curIndStart, 1, ctx->curIndStart, (int32_t)ctx->curVertOffset, 0);
#endif
	LOG(VKVG_LOG_INFO,
		"RECORD DRAW CMD: ctx = %p; vertices = %d; indices = %d (vxOff = %d idxStart = %d idxTot = %d )\n", ctx,
		ctx->vertCount - ctx->curVertOffset, ctx->indCount - ctx->curIndStart, ctx->curVertOffset, ctx->curIndStart,
		ctx->indCount);

	ctx->curIndStart = ctx->indCount;
	ctx->curVertOffset = ctx->vertCount;
}
// preflush vertices with drawcommand already emited
void _flush_cmd_until_vx_base(VkvgContext ctx) {
	_end_render_pass(ctx);
	if (ctx->curVertOffset > 0) {
		LOG(VKVG_LOG_INFO, "FLUSH UNTIL VX BASE CTX: ctx = %p; vertices = %d; indices = %d\n", ctx, ctx->vertCount,
			ctx->indCount);
		_flush_vertices_caches_until_vertex_base(ctx);
	}
	vkh_cmd_end(ctx->cmd);
	_wait_and_submit_cmd(ctx);
}
void _flush_cmd_buff(VkvgContext ctx) {
	_emit_draw_cmd_undrawn_vertices(ctx);
	if (!ctx->cmdStarted)
		return;
	_end_render_pass(ctx);
	LOG(VKVG_LOG_INFO, "FLUSH CTX: ctx = %p; vertices = %d; indices = %d\n", ctx, ctx->vertCount, ctx->indCount);
	_flush_vertices_caches(ctx);
	vkh_cmd_end(ctx->cmd);

	_wait_and_submit_cmd(ctx);
}
void _flush_cmd_buff0(VkvgContext ctx) {
	_emit_draw_cmd_undrawn_vertices(ctx);
	if (!ctx->cmdStarted)
		return;
	_end_render_pass(ctx);
	LOG(VKVG_LOG_INFO, "FLUSH CTX: ctx = %p; vertices = %d; indices = %d\n", ctx, ctx->vertCount, ctx->indCount);
	_flush_vertices_caches(ctx);
	vkh_cmd_end(ctx->cmd);

	_submit_cmd(ctx);
}

// bind correct draw pipeline depending on current OPERATOR
void _bind_draw_pipeline(VkvgContext ctx) {
	switch (ctx->curOperator) {
	case VKVG_OPERATOR_OVER:
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipe_OVER);
		break;
	case VKVG_OPERATOR_CLEAR:
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipe_CLEAR);
		break;
	case VKVG_OPERATOR_DIFFERENCE:
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipe_SUB);
		break;
	default:
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipe_OVER);
		break;
	}
}
#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
const float DBG_LAB_COLOR_RP[4] = { 0, 0, 1, 1 };
const float DBG_LAB_COLOR_FSQ[4] = { 1, 0, 0, 1 };
#endif

void _start_cmd_for_render_pass(VkvgContext ctx) {
	LOG(VKVG_LOG_INFO, "START RENDER PASS: ctx = %p\n", ctx);
	vkh_cmd_begin(ctx->cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	if (ctx->pSurf->img->layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL || ctx->dev->threadAware) {
		VkhImage imgMs = ctx->pSurf->imgMS;
		if (imgMs != NULL)
			vkh_image_set_layout(ctx->cmd, imgMs, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		vkh_image_set_layout(ctx->cmd, ctx->pSurf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		vkh_image_set_layout(ctx->cmd, ctx->pSurf->stencil, ctx->dev->stencilAspectFlag,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
	}

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_cmd_label_start(ctx->cmd, "ctx render pass", DBG_LAB_COLOR_RP);
#endif

	CmdBeginRenderPass(ctx->cmd, &ctx->renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	VkViewport viewport = { 0, 0, (float)ctx->pSurf->width, (float)ctx->pSurf->height, 0, 1.f };
	CmdSetViewport(ctx->cmd, 0, 1, &viewport);

	CmdSetScissor(ctx->cmd, 0, 1, &ctx->bounds);

	VkDescriptorSet dss[] = { ctx->dsFont, ctx->dsSrc, ctx->dsGrad };
	//CmdBindDescriptorSets(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelineLayout, 0, 3, dss, 0, NULL);

	VkDeviceSize offsets[1] = { 0 };
	CmdBindVertexBuffers(ctx->cmd, 0, 1, &ctx->vertices.buffer, offsets);
	CmdBindIndexBuffer(ctx->cmd, ctx->indices.buffer, 0, VKVG_VK_INDEX_TYPE);

	_update_push_constants(ctx);

	_bind_draw_pipeline(ctx);
	CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
	ctx->cmdStarted = true;
	push_update_descriptor_set_a(ctx, ctx->d_img, ctx->d_offset);
}
// compute inverse mat used in shader when context matrix has changed
// then trigger push constants command
void _set_mat_inv_and_vkCmdPush(VkvgContext ctx) {
	ctx->pushConsts.matInv = ctx->pushConsts.mat;
	vkvg_matrix_invert(&ctx->pushConsts.matInv);
	ctx->pushCstDirty = true;
}
void _update_push_constants(VkvgContext ctx) {
	CmdPushConstants(ctx->cmd, ctx->dev->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants),
		&ctx->pushConsts);
	ctx->pushCstDirty = false;
}
void _sort_gradient_stops(vkvg_color_t* colors, float* stops, uint32_t count) {
	for (uint32_t i = 1; i < count; i++) {
		float   key_stop = stops[i];
		vkvg_color_t key_color = colors[i];
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
void _update_cur_pattern(VkvgContext ctx, VkvgPattern pat) {
	VkvgPattern lastPat = ctx->pattern;
	ctx->pattern = pat;

	uint32_t newPatternType = VKVG_PATTERN_TYPE_SOLID;

	LOG(VKVG_LOG_INFO, "CTX: _update_cur_pattern: %p -> %p\n", lastPat, pat);

	if (pat == NULL) {       // solid color
		if (lastPat == NULL) // solid
			return;          // solid to solid transition, no extra action requested
	}
	else
		newPatternType = pat->type;

	switch (newPatternType) {
	case VKVG_PATTERN_TYPE_SOLID:
		_flush_cmd_buff(ctx);
		if (!_wait_ctx_flush_end(ctx))
			return;
		if (lastPat->type == VKVG_PATTERN_TYPE_SURFACE) // unbind current source surface by replacing it with empty texture
		{
			//_update_descriptor_set(ctx, ctx->dev->emptyImg, ctx->dsSrc);
			push_update_descriptor_set_a(ctx, 0, 0);
		}
		break;
	case VKVG_PATTERN_TYPE_SURFACE: {
		_emit_draw_cmd_undrawn_vertices(ctx);

		VkvgSurface surf = (VkvgSurface)pat->data;

		// flush ctx in two steps to add the src transitioning in the cmd buff
		if (ctx->cmdStarted) { // transition of img without appropriate dependencies in subpass must be done outside
			// renderpass.
			_end_render_pass(ctx);
			_flush_vertices_caches(ctx);
		}
		else {
			vkh_cmd_begin(ctx->cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			ctx->cmdStarted = true;
		}

		// transition source surface for sampling
		vkh_image_set_layout(ctx->cmd, surf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		vkh_cmd_end(ctx->cmd);
		_wait_and_submit_cmd(ctx);
		if (!_wait_ctx_flush_end(ctx))
			return;

		VkSamplerAddressMode addrMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkFilter             filter = VK_FILTER_NEAREST;
		switch (pat->extend) {
		case VKVG_EXTEND_NONE:
			addrMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			break;
		case VKVG_EXTEND_PAD:
			addrMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			break;
		case VKVG_EXTEND_REPEAT:
			addrMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			break;
		case VKVG_EXTEND_REFLECT:
			addrMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			break;
		}
		switch (pat->filter) {
		case VKVG_FILTER_BILINEAR:
		case VKVG_FILTER_BEST:
			filter = VK_FILTER_LINEAR;
			break;
		default:
			filter = VK_FILTER_NEAREST;
			break;
		}
		vkh_image_create_sampler(surf->img, filter, filter, VK_SAMPLER_MIPMAP_MODE_NEAREST, addrMode);
		push_update_descriptor_set_a(ctx, surf->img, 0);
		//_update_descriptor_set(ctx, surf->img, ctx->dsSrc);

		ctx->pushConsts.source.width = (float)surf->width;
		ctx->pushConsts.source.height = (float)surf->height;

		vkvg_matrix_t mat;
		if (pat->hasMatrix) {
			vkvg_pattern_get_matrix(pat, &mat);
			// if (vkvg_matrix_invert(&mat) != VKVG_STATUS_SUCCESS)
			//     mat = VKVG_IDENTITY_MATRIX;
			// vkvg_matrix_transform_point(&mat, &ctx->pushConsts.source.x, &ctx->pushConsts.source.y);
			// vkvg_matrix_transform_distance(&mat, &ctx->pushConsts.source.width, &ctx->pushConsts.source.height);
			vkvg_matrix_multiply(&ctx->pushConsts.matInv, &ctx->pushConsts.matInv, &mat);
		}
		break;
	}
	case VKVG_PATTERN_TYPE_LINEAR:
	case VKVG_PATTERN_TYPE_RADIAL:
	case VKVG_PATTERN_TYPE_SWEEP:
		_flush_cmd_buff(ctx);
		if (!_wait_ctx_flush_end(ctx))
			return;
		if (lastPat && lastPat->type == VKVG_PATTERN_TYPE_SURFACE)
		{
			//_update_descriptor_set(ctx, ctx->dev->emptyImg, ctx->dsSrc); 
		}
		float fm = std::max((float)ctx->pSurf->width, (float)ctx->pSurf->height);
		vec4 bounds = { fm,fm,0.0f,0.0f }; // store img bounds in unused source field
		ctx->pushConsts.source = bounds;
		// transform control point with current ctx matrix 
		vkvg_gradient_t grad = *(vkvg_gradient_t*)pat->data;
		if (grad.count < 2) {
			ctx->status = VKVG_STATUS_PATTERN_INVALID_GRADIENT;
			return;
		}
		grad.extend = pat->extend;
		vkvg_matrix_t mat;
		if (pat->hasMatrix) {
			vkvg_pattern_get_matrix(pat, &mat);
			if (vkvg_matrix_invert(&mat) != VKVG_STATUS_SUCCESS)
				mat = VKVG_IDENTITY_MATRIX;
			vkvg_matrix_transform_point(&mat, &grad.cp[0].x, &grad.cp[0].y);
		}
		vkvg_matrix_transform_point(&ctx->pushConsts.mat, &grad.cp[0].x, &grad.cp[0].y);
		if (pat->type == VKVG_PATTERN_TYPE_LINEAR) {
			if (pat->hasMatrix)
				vkvg_matrix_transform_point(&mat, &grad.cp[0].z, &grad.cp[0].w);
			vkvg_matrix_transform_point(&ctx->pushConsts.mat, &grad.cp[0].z, &grad.cp[0].w);
		}
		else {
			if (pat->hasMatrix)
				vkvg_matrix_transform_point(&mat, &grad.cp[1].x, &grad.cp[1].y);
			vkvg_matrix_transform_point(&ctx->pushConsts.mat, &grad.cp[1].x, &grad.cp[1].y);
			// radii
			if (pat->hasMatrix) {
				vkvg_matrix_transform_distance(&mat, &grad.cp[0].z, &grad.cp[0].w);
				vkvg_matrix_transform_distance(&mat, &grad.cp[1].z, &grad.cp[0].w);
			}
			vkvg_matrix_transform_distance(&ctx->pushConsts.mat, &grad.cp[0].z, &grad.cp[0].w);
			vkvg_matrix_transform_distance(&ctx->pushConsts.mat, &grad.cp[1].z, &grad.cp[0].w);
		}
		_sort_gradient_stops(grad.colors, grad.stops, grad.count);
		memcpy(vkh_buffer_get_mapped_pointer(&ctx->uboGrad), &grad, sizeof(vkvg_gradient_t));
		vkh_buffer_flush(&ctx->uboGrad);
		push_update_descriptor_set_a(ctx, 0, 0);
		break;
	}
	ctx->pushConsts.fsq_patternType = (ctx->pushConsts.fsq_patternType & FULLSCREEN_BIT) + newPatternType;
	ctx->pushCstDirty = true;
	if (lastPat)
		vkvg_pattern_destroy(lastPat);
}
void _update_descriptor_set(VkvgContext ctx, VkhImage img, VkDescriptorSet ds) {

	VkDescriptorImageInfo descSrcTex = vkh_image_get_descriptor(img, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	VkWriteDescriptorSet  writeDescriptorSet = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
												.dstSet = ds,
												.dstBinding = 0,
												.descriptorCount = 1,
												.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
												.pImageInfo = &descSrcTex };
	vkUpdateDescriptorSets(ctx->dev->vkDev, 1, &writeDescriptorSet, 0, NULL);
}

void _update_gradient_desc_set(VkvgContext ctx) {
	VkDescriptorBufferInfo dbi = { ctx->uboGrad.buffer, 0, VK_WHOLE_SIZE };
	VkWriteDescriptorSet   writeDescriptorSet = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
												 .dstSet = ctx->dsGrad,
												 .dstBinding = 0,
												 .descriptorCount = 1,
												 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
												 .pBufferInfo = &dbi };
	vkUpdateDescriptorSets(ctx->dev->vkDev, 1, &writeDescriptorSet, 0, NULL);
}
/*
 * Reset currently bound descriptor which image could be destroyed
 */
 /*void _reset_src_descriptor_set (VkvgContext ctx){
	 VkvgDevice dev = ctx->pSurf->dev;
	 //VkDescriptorSet dss[] = {ctx->dsSrc};
	 vkFreeDescriptorSets	(dev->vkDev, ctx->descriptorPool, 1, &ctx->dsSrc);

	 VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
															   .descriptorPool = ctx->descriptorPool,
															   .descriptorSetCount = 1,
															   .pSetLayouts = &dev->dslSrc };
	 VK_CHECK_RESULT(vkAllocateDescriptorSets(dev->vkDev, &descriptorSetAllocateInfo, &ctx->dsSrc));
 }*/

void _createDescriptorPool(VkvgContext ctx) {
	VkvgDevice                 dev = ctx->dev;
	const VkDescriptorPoolSize descriptorPoolSize[] = { {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2},
														   {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1} };
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.maxSets = 3;
	descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptorPoolCreateInfo.poolSizeCount = 2;
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize;
	VK_CHECK_RESULT(vkCreateDescriptorPool(dev->vkDev, &descriptorPoolCreateInfo, NULL, &ctx->descriptorPool));
}
void _init_descriptor_sets(VkvgContext ctx) {
	VkvgDevice                  dev = ctx->dev;
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
															 .descriptorPool = ctx->descriptorPool,
															 .descriptorSetCount = 1,
															 .pSetLayouts = &dev->dslFont };
	//VK_CHECK_RESULT(vkAllocateDescriptorSets(dev->vkDev, &descriptorSetAllocateInfo, &ctx->dsFont));
	descriptorSetAllocateInfo.pSetLayouts = &dev->dslSrc;
	//VK_CHECK_RESULT(vkAllocateDescriptorSets(dev->vkDev, &descriptorSetAllocateInfo, &ctx->dsSrc));
	descriptorSetAllocateInfo.pSetLayouts = &dev->dslGrad;
	//VK_CHECK_RESULT(vkAllocateDescriptorSets(dev->vkDev, &descriptorSetAllocateInfo, &ctx->dsGrad));
}
void _release_context_ressources(VkvgContext ctx) {
	VkDevice dev = ctx->dev->vkDev;

#ifndef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	vkDestroyFence(dev, ctx->flushFence, NULL);
#endif
	vkFreeCommandBuffers(dev, ctx->cmdPool, 2, ctx->cmdBuffers);
	vkDestroyCommandPool(dev, ctx->cmdPool, NULL);
	VkDescriptorSet dss[] = { ctx->dsFont, ctx->dsSrc, ctx->dsGrad };
	vkFreeDescriptorSets(dev, ctx->descriptorPool, 3, dss);
	vkDestroyDescriptorPool(dev, ctx->descriptorPool, NULL);
	vkh_buffer_reset(&ctx->uboGrad);
	vkh_buffer_reset(&ctx->indices);
	vkh_buffer_reset(&ctx->vertices);
	free(ctx->vertexCache);
	free(ctx->indexCache);
	vkh_image_destroy(ctx->fontCacheImg);
	// TODO:check this for source counter
	// vkh_image_destroy (ctx->source);
	auto pcx = (vkvg_context*)ctx;
	if (pcx->ecps)free(pcx->ecps);
	pcx->ecps = 0;
	pcx->capPathPointCount = 0;
	free(ctx->pathes);
	free(ctx->points);
	//if (ctx->pathCtx) {
	//	free_vgctx(ctx->pathCtx);
	//	ctx->pathCtx = 0;
	//}
	free(ctx);
}
// populate vertice buff for stroke
bool _build_vb_step(VkvgContext ctx, stroke_context_t* str, bool isCurve) {
	Vertex v = { {0}, ctx->curColor, {0, 0, -1} };
	vec2   p0 = ctx->points[str->cp];
	vec2   v0 = vec2_sub(p0, ctx->points[str->iL]);
	vec2   v1 = vec2_sub(ctx->points[str->iR], p0);
	float  length_v0 = vec2_length(v0);
	float  length_v1 = vec2_length(v1);
	if (length_v0 < FLT_EPSILON || length_v1 < FLT_EPSILON) {
		LOG(VKVG_LOG_STROKE, "vb_step discard, length<epsilon: l0:%f l1:%f\n", length_v0, length_v1);
		return false;
	}
	vec2  v0n = vec2_div_s(v0, length_v0);
	vec2  v1n = vec2_div_s(v1, length_v1);
	float dot = vec2_dot(v0n, v1n);
	float det = v0n.x * v1n.y - v0n.y * v1n.x;
	if (EQUF(dot, 1.0f)) { // colinear
		LOG(VKVG_LOG_STROKE, "vb_step discard, dot==1\n");
		return false;
	}
	if (EQUF(dot, -1.0f)) { // cusp (could draw line butt?)
		vec2 vPerp = vec2_mult_s(vec2_perp(v0n), str->hw);
		VKVG_IBO_INDEX_TYPE idx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);
		v.pos = vec2_add(p0, vPerp);
		_add_vertex(ctx, v);
		v.pos = vec2_sub(p0, vPerp);
		_add_vertex(ctx, v);
		_add_triangle_indices(ctx, idx, idx + 1, idx + 2);
		_add_triangle_indices(ctx, idx, idx + 2, idx + 3);
		LOG(VKVG_LOG_STROKE, "vb_step cusp, dot==-1\n");
		return true;
	}
	vec2  bisec_n = vec2_norm(vec2_add(v0n, v1n)); // bisec/bisec_perp are inverted names
	float alpha = acosf(dot);

	if (det < 0)
		alpha = -alpha;
	float halfAlpha = alpha / 2.f;
	float cosHalfAlpha = cosf(halfAlpha);
	float lh = str->hw / cosHalfAlpha;
	vec2  bisec_n_perp = vec2_perp(bisec_n);
	// limit bisectrice length
	float rlh = lh; // rlh is for inside pos tweeks
	if (dot < 0.f)
		rlh = fminf(rlh, fminf(length_v0, length_v1));
	//---

	vec2 bisec = vec2_mult_s(bisec_n_perp, rlh);

	VKVG_IBO_INDEX_TYPE idx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);

	vec2 rlh_inside_pos, rlh_outside_pos;
	if (rlh < lh) {
		vec2 vnPerp;
		if (length_v0 < length_v1)
			vnPerp = vec2_perp(v1n);
		else
			vnPerp = vec2_perp(v0n);
		vec2 vHwPerp = vec2_mult_s(vnPerp, str->hw);

		double lbc = cosHalfAlpha * rlh;
		if (det < 0.f) {
			rlh_inside_pos = vec2_add(vec2_add(vec2_mult_s(vnPerp, -lbc), vec2_add(p0, bisec)), vHwPerp);
			rlh_outside_pos = vec2_sub(p0, vec2_mult_s(bisec_n_perp, lh));
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

	vkvg_line_join_t join = ctx->lineJoin;

	if (isCurve) {
		if (dot < 0.8f)
			join = VKVG_LINE_JOIN_ROUND;
		else
			join = VKVG_LINE_JOIN_MITER;
	}

	if (join == VKVG_LINE_JOIN_MITER) {
		if (lh > str->lhMax) { // miter limit
			double x = (lh - str->lhMax) * cosHalfAlpha;
			vec2   bisecPerp = vec2_mult_s(bisec_n, x);
			bisec = vec2_mult_s(bisec_n_perp, str->lhMax);
			if (det < 0) {
				v.pos = rlh_inside_pos;
				_add_vertex(ctx, v);

				vec2 p = vec2_sub(p0, bisec);

				v.pos = vec2_sub(p, bisecPerp);
				_add_vertex(ctx, v);
				v.pos = vec2_add(p, bisecPerp);
				_add_vertex(ctx, v);

				_add_triangle_indices(ctx, idx, idx + 2, idx + 1);
				_add_triangle_indices(ctx, idx + 2, idx + 4, idx);
				_add_triangle_indices(ctx, idx, idx + 3, idx + 4);
				return true;
			}
			else {
				vec2 p = vec2_add(p0, bisec);
				v.pos = vec2_sub(p, bisecPerp);
				_add_vertex(ctx, v);

				v.pos = rlh_inside_pos;
				_add_vertex(ctx, v);

				v.pos = vec2_add(p, bisecPerp);
				_add_vertex(ctx, v);

				_add_triangle_indices(ctx, idx, idx + 2, idx + 1);
				_add_triangle_indices(ctx, idx + 2, idx + 3, idx + 1);
				_add_triangle_indices(ctx, idx + 1, idx + 3, idx + 4);
				return false;
			}

		}
		else { // normal miter
			if (det < 0) {
				v.pos = rlh_inside_pos;
				_add_vertex(ctx, v);
				v.pos = rlh_outside_pos;
				_add_vertex(ctx, v);
			}
			else {
				v.pos = rlh_outside_pos;
				_add_vertex(ctx, v);
				v.pos = rlh_inside_pos;
				_add_vertex(ctx, v);
			}

			_add_tri_indices_for_rect(ctx, idx);
			return false;
		}
	}
	else {
		vec2 vp = vec2_perp(v0n);

		if (det < 0) {
			if (dot < 0 && rlh < lh)
				v.pos = rlh_inside_pos;
			else
				v.pos = vec2_add(p0, bisec);
			_add_vertex(ctx, v);
			v.pos = vec2_sub(p0, vec2_mult_s(vp, str->hw));
		}
		else {
			v.pos = vec2_add(p0, vec2_mult_s(vp, str->hw));
			_add_vertex(ctx, v);
			if (dot < 0 && rlh < lh)
				v.pos = rlh_inside_pos;
			else
				v.pos = vec2_sub(p0, bisec);
		}
		_add_vertex(ctx, v);

		if (join == VKVG_LINE_JOIN_BEVEL) {
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
		else if (join == VKVG_LINE_JOIN_ROUND) {
			if (!str->arcStep)
				str->arcStep = _get_arc_step(ctx, str->hw);
			float a = acosf(vp.x);
			if (vp.y < 0)
				a = -a;

			if (det < 0) {
				a += M_PIF;
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
			VKVG_IBO_INDEX_TYPE p0Idx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);
			_add_triangle_indices(ctx, idx, idx + 2, idx + 1);
			if (det < 0) {
				for (VKVG_IBO_INDEX_TYPE p = idx + 2; p < p0Idx; p++)
					_add_triangle_indices(ctx, p, p + 1, idx);
				_add_triangle_indices(ctx, p0Idx, p0Idx + 2, idx);
				_add_triangle_indices(ctx, idx, p0Idx + 1, p0Idx + 2);
			}
			else {
				for (VKVG_IBO_INDEX_TYPE p = idx + 2; p < p0Idx; p++)
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
		_add_vertex(ctx, v);
	}

	/*
	#ifdef DEBUG

		debugLinePoints[dlpCount] = p0;
		debugLinePoints[dlpCount+1] = _v2add(p0, _vec2dToVec2(_v2Multd(v0n,10)));
		dlpCount+=2;
		debugLinePoints[dlpCount] = p0;
		debugLinePoints[dlpCount+1] = _v2add(p0, _vec2dToVec2(_v2Multd(v1n,10)));
		dlpCount+=2;
		debugLinePoints[dlpCount] = p0;
		debugLinePoints[dlpCount+1] = pR;
		dlpCount+=2;
	#endif*/
	/*if (reducedLH)
		return -det;
	else*/
	return (det < 0);
}

void _draw_stoke_cap(VkvgContext ctx, stroke_context_t* str, vec2 p0, vec2 n, bool isStart) {
	Vertex v = { {0}, ctx->curColor, {0, 0, -1} };

	VKVG_IBO_INDEX_TYPE firstIdx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);

	if (isStart) {
		vec2 vhw = vec2_mult_s(n, str->hw);

		if (ctx->lineCap == VKVG_LINE_CAP_SQUARE)
			p0 = vec2_sub(p0, vhw);

		vhw = vec2_perp(vhw);

		if (ctx->lineCap == VKVG_LINE_CAP_ROUND) {
			if (!str->arcStep)
				str->arcStep = _get_arc_step(ctx, str->hw);

			float a = acosf(n.x) + M_PIF_2;
			if (n.y < 0)
				a = M_PIF - a;
			float a1 = a + M_PIF;

			a += str->arcStep;
			while (a < a1) {
				_add_vertexf(ctx, cosf(a) * str->hw + p0.x, sinf(a) * str->hw + p0.y);
				a += str->arcStep;
			}
			VKVG_IBO_INDEX_TYPE p0Idx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);
			for (VKVG_IBO_INDEX_TYPE p = firstIdx; p < p0Idx; p++)
				_add_triangle_indices(ctx, p0Idx + 1, p, p + 1);
			firstIdx = p0Idx;
		}

		v.pos = vec2_add(p0, vhw);
		_add_vertex(ctx, v);
		v.pos = vec2_sub(p0, vhw);
		_add_vertex(ctx, v);

		_add_tri_indices_for_rect(ctx, firstIdx);
	}
	else {
		vec2 vhw = vec2_mult_s(n, str->hw);

		if (ctx->lineCap == VKVG_LINE_CAP_SQUARE)
			p0 = vec2_add(p0, vhw);

		vhw = vec2_perp(vhw);

		v.pos = vec2_add(p0, vhw);
		_add_vertex(ctx, v);
		v.pos = vec2_sub(p0, vhw);
		_add_vertex(ctx, v);

		firstIdx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);

		if (ctx->lineCap == VKVG_LINE_CAP_ROUND) {
			if (!str->arcStep)
				str->arcStep = _get_arc_step(ctx, str->hw);

			float a = acosf(n.x) + M_PIF_2;
			if (n.y < 0)
				a = M_PIF - a;
			float a1 = a - M_PIF;

			a -= str->arcStep;
			while (a > a1) {
				_add_vertexf(ctx, cosf(a) * str->hw + p0.x, sinf(a) * str->hw + p0.y);
				a -= str->arcStep;
			}

			VKVG_IBO_INDEX_TYPE p0Idx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset - 1);
			for (VKVG_IBO_INDEX_TYPE p = firstIdx - 1; p < p0Idx; p++)
				_add_triangle_indices(ctx, p + 1, p, firstIdx - 2);
		}
	}
}
float _draw_dashed_segment(VkvgContext ctx, stroke_context_t* str, dash_context_t* dc, bool isCurve) {
	// vec2 pL = ctx->points[str->iL];
	vec2 p = ctx->points[str->cp];
	vec2 pR = ctx->points[str->iR];

	if (!dc->dashOn) // we test in fact the next dash start, if dashOn = true => next segment is a void.
		_build_vb_step(ctx, str, isCurve);

	vec2 d = vec2_sub(pR, p);
	dc->normal = vec2_norm(d);
	float segmentLength = vec2_length(d);

	while (dc->curDashOffset < segmentLength) {
		vec2 p0 = vec2_add(p, vec2_mult_s(dc->normal, dc->curDashOffset));

		_draw_stoke_cap(ctx, str, p0, dc->normal, dc->dashOn);
		dc->dashOn ^= true;
		dc->curDashOffset += ctx->dashes[dc->curDash];
		if (++dc->curDash == ctx->dashCount)
			dc->curDash = 0;
	}
	dc->curDashOffset -= segmentLength;
	dc->curDashOffset = fmodf(dc->curDashOffset, dc->totDashLength);
	return segmentLength;
}
void _draw_segment(VkvgContext ctx, stroke_context_t* str, dash_context_t* dc, bool isCurve) {
	str->iR = str->cp + 1;
	if (ctx->dashCount > 0)
		_draw_dashed_segment(ctx, str, dc, isCurve);
	else
		_build_vb_step(ctx, str, isCurve);
	str->iL = str->cp++;
	if (ctx->vertCount - ctx->curVertOffset > VKVG_IBO_MAX / 3) {
		Vertex v0 = ctx->vertexCache[ctx->curVertOffset + str->firstIdx];
		Vertex v1 = ctx->vertexCache[ctx->curVertOffset + str->firstIdx + 1];
		_emit_draw_cmd_undrawn_vertices(ctx);
		// repeat first 2 vertices for closed pathes
		str->firstIdx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);
		_add_vertex(ctx, v0);
		_add_vertex(ctx, v1);
		ctx->curVertOffset = ctx->vertCount; // prevent redrawing them at the start of the batch
	}
}

bool ptInTriangle(vec2 p, vec2 p0, vec2 p1, vec2 p2) {
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

void _free_ctx_save(vkvg_context_save_t* sav) {
	if (sav->dashCount > 0)
		free(sav->dashes);
	if (sav->pattern)
		vkvg_pattern_destroy(sav->pattern);
	free(sav);
}

#define M_APPROXIMATION_SCALE         1.0
#define M_ANGLE_TOLERANCE             0.01
#define M_CUSP_LIMIT                  0.01
#define CURVE_RECURSION_LIMIT         100
#define CURVE_COLLINEARITY_EPSILON    1.7
#define CURVE_ANGLE_TOLERANCE_EPSILON 0.001
// no floating point arithmetic operation allowed in macro.
#pragma warning(disable : 4127)
void _recursive_bezier(VkvgContext ctx, float distanceTolerance, float x1, float y1, float x2, float y2, float x3,
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
				if (da1 >= M_PIF)
					da1 = M_2_PIF - da1;
				if (da2 >= M_PIF)
					da2 = M_2_PIF - da2;

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
					if (da1 >= M_PIF)
						da1 = M_2_PIF - da1;

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
					if (da1 >= M_PIF)
						da1 = M_2_PIF - da1;

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
#pragma warning(default : 4127)
void _line_to(VkvgContext ctx, float x, float y) {
	vec2 p = { x, y };
	if (!_current_path_is_empty(ctx)) {
		// prevent adding the same point
		if (vec2_equ(_get_current_position(ctx), p))
			return;
	}
	_add_point(ctx, x, y);
	ctx->simpleConvex = false;
}
void _elliptic_arc(VkvgContext ctx, float x1, float y1, float x2, float y2, bool largeArc, bool counterClockWise,
	float _rx, float _ry, float phi) {
	if (ctx->status)
		return;

	if (_rx == 0 || _ry == 0) {
		if (_current_path_is_empty(ctx))
			vkvg_move_to(ctx, x1, y1);
		vkvg_line_to(ctx, x2, y2);
		return;
	}
	float rx = fabsf(_rx);
	float ry = fabsf(_ry);

	mat2 m = { {cosf(phi), sinf(phi)}, {-sinf(phi), cosf(phi)} };
	vec2 p = { (x1 - x2) / 2, (y1 - y2) / 2 };
	vec2 p1 = mat2_mult_vec2(m, p);

	// radii corrections
	double lambda = powf(p1.x, 2) / powf(rx, 2) + powf(p1.y, 2) / powf(ry, 2);
	if (lambda > 1) {
		lambda = sqrtf(lambda);
		rx *= lambda;
		ry *= lambda;
	}

	p = vec2{ rx * p1.y / ry, -ry * p1.x / rx };

	vec2 cp = vec2_mult_s(
		p, sqrtf(fabsf((powf(rx, 2) * powf(ry, 2) - powf(rx, 2) * powf(p1.y, 2) - powf(ry, 2) * powf(p1.x, 2)) /
			(powf(rx, 2) * powf(p1.y, 2) + powf(ry, 2) * powf(p1.x, 2)))));

	if (largeArc == counterClockWise)
		vec2_inv(&cp);

	m = mat2({ cosf(phi), -sinf(phi) }, { sinf(phi), cosf(phi) });
	p = vec2((x1 + x2) / 2, (y1 + y2) / 2);
	vec2 c = vec2_add(mat2_mult_vec2(m, cp), p);

	vec2   u = vec2_unit_x;
	vec2   v = { (p1.x - cp.x) / rx, (p1.y - cp.y) / ry };
	double sa = acosf(vec2_dot(u, v) / (fabsf(vec2_length(v)) * fabsf(vec2_length(u))));
	if (isnan((float)sa))
		sa = M_PIF;
	if (u.x * v.y - u.y * v.x < 0)
		sa = -sa;

	u = v;
	v = vec2{ (-p1.x - cp.x) / rx, (-p1.y - cp.y) / ry };
	double delta_theta = acosf(vec2_dot(u, v) / (fabsf(vec2_length(v)) * fabsf(vec2_length(u))));
	if (isnan((float)delta_theta))
		delta_theta = M_PIF;
	if (u.x * v.y - u.y * v.x < 0)
		delta_theta = -delta_theta;

	if (counterClockWise) {
		if (delta_theta < 0)
			delta_theta += M_PIF * 2.0;
	}
	else if (delta_theta > 0)
		delta_theta -= M_PIF * 2.0;

	m = mat2{ {cosf(phi), -sinf(phi)}, {sinf(phi), cosf(phi)} };

	double theta = sa;
	double ea = sa + delta_theta;

	float step = fmaxf(0.001f, fminf(M_PIF, _get_arc_step(ctx, fminf(rx, ry)) * 0.1f));

	p = vec2{ rx * cosf(theta), ry * sinf(theta) };
	vec2 xy = vec2_add(mat2_mult_vec2(m, p), c);

	if (_current_path_is_empty(ctx)) {
		_set_curve_start(ctx);
		_add_point(ctx, xy.x, xy.y);
		if (!ctx->pathPtr)
			ctx->simpleConvex = true;
		else
			ctx->simpleConvex = false;
	}
	else {
		_line_to(ctx, xy.x, xy.y);
		_set_curve_start(ctx);
		ctx->simpleConvex = false;
	}

	_set_curve_start(ctx);

	if (sa < ea) {
		theta += step;
		while (theta < ea) {
			p = vec2{ rx * cosf(theta), ry * sinf(theta) };
			xy = vec2_add(mat2_mult_vec2(m, p), c);
			_add_point(ctx, xy.x, xy.y);
			theta += step;
		}
	}
	else {
		theta -= step;
		while (theta > ea) {
			p = vec2{ rx * cosf(theta), ry * sinf(theta) };
			xy = vec2_add(mat2_mult_vec2(m, p), c);
			_add_point(ctx, xy.x, xy.y);
			theta -= step;
		}
	}
	p = vec2{ rx * cosf(ea), ry * sinf(ea) };
	xy = vec2_add(mat2_mult_vec2(m, p), c);
	_add_point(ctx, xy.x, xy.y);
	_set_curve_end(ctx);
}

// Even-Odd inside test with stencil buffer implementation.
void _poly_fill(VkvgContext ctx, vec4* bounds) {
	// we anticipate the check for vbo buffer size, ibo is not used in poly_fill
	// the polyfill emit a single vertex for each point in the path.
	if (ctx->sizeVBO - VKVG_ARRAY_THRESHOLD < ctx->vertCount + ctx->pointCount) {
		if (ctx->cmdStarted) {
			_end_render_pass(ctx);
			if (ctx->vertCount > 0)
				_flush_vertices_caches(ctx);
			vkh_cmd_end(ctx->cmd);
			_wait_and_submit_cmd(ctx);
			_wait_ctx_flush_end(ctx);
			if (ctx->sizeVBO - VKVG_ARRAY_THRESHOLD < ctx->pointCount) {
				_resize_vbo(ctx, ctx->pointCount + VKVG_ARRAY_THRESHOLD);
				_resize_vertex_cache(ctx, ctx->sizeVBO);
			}
		}
		else {
			_resize_vbo(ctx, ctx->vertCount + ctx->pointCount + VKVG_ARRAY_THRESHOLD);
			_resize_vertex_cache(ctx, ctx->sizeVBO);
		}

		_start_cmd_for_render_pass(ctx);
	}
	else {
		_ensure_vertex_cache_size(ctx, ctx->pointCount);
		_ensure_renderpass_is_started(ctx);
	}

	CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelinePolyFill);

	Vertex   v = { {0}, ctx->curColor, {0, 0, -1} };
	uint32_t ptrPath = 0;
	uint32_t firstPtIdx = 0;

	while (ptrPath < ctx->pathPtr) {
		uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;
		if (pathPointCount > 2) {
			VKVG_IBO_INDEX_TYPE firstVertIdx = (VKVG_IBO_INDEX_TYPE)ctx->vertCount;

			for (uint32_t i = 0; i < pathPointCount; i++) {
				v.pos = ctx->points[i + firstPtIdx];
				ctx->vertexCache[ctx->vertCount++] = v;
				if (!bounds)
					continue;
				// bounds are computed here to scissor the painting operation
				// that speed up fill drastically.
				vkvg_matrix_transform_point(&ctx->pushConsts.mat, &v.pos.x, &v.pos.y);

				if (v.pos.x < bounds->xMin)
					bounds->xMin = v.pos.x;
				if (v.pos.x > bounds->xMax)
					bounds->xMax = v.pos.x;
				if (v.pos.y < bounds->yMin)
					bounds->yMin = v.pos.y;
				if (v.pos.y > bounds->yMax)
					bounds->yMax = v.pos.y;
			}

			LOG(VKVG_LOG_INFO_PATH, "\tpoly fill: point count = %d; 1st vert = %d; vert count = %d\n", pathPointCount,
				firstVertIdx, ctx->vertCount - firstVertIdx);
			CmdDraw(ctx->cmd, pathPointCount, 1, firstVertIdx, 0);
		}
		firstPtIdx += pathPointCount;

		if (_path_has_curves(ctx, ptrPath)) {
			// skip segments lengths used in stroke
			ptrPath++;
			uint32_t totPts = 0;
			while (totPts < pathPointCount)
				totPts += (ctx->pathes[ptrPath++] & PATH_ELT_MASK);
		}
		else
			ptrPath++;
	}
	ctx->curVertOffset = ctx->vertCount;
}
#ifdef VKVG_FILL_NZ_GLUTESS
void fan_vertex2(VKVG_IBO_INDEX_TYPE v, VkvgContext ctx) {
	VKVG_IBO_INDEX_TYPE i = (VKVG_IBO_INDEX_TYPE)v;
	switch (ctx->tesselator_idx_counter) {
	case 0:
		_add_indice(ctx, i);
		ctx->tesselator_fan_start = i;
		ctx->tesselator_idx_counter++;
		break;
	case 1:
	case 2:
		_add_indice(ctx, i);
		ctx->tesselator_idx_counter++;
		break;
	default:
		_add_indice_for_fan(ctx, i);
		break;
	}
}
void strip_vertex2(VKVG_IBO_INDEX_TYPE v, VkvgContext ctx) {
	VKVG_IBO_INDEX_TYPE i = (VKVG_IBO_INDEX_TYPE)v;
	if (ctx->tesselator_idx_counter < 3) {
		_add_indice(ctx, i);
	}
	else
		_add_indice_for_strip(ctx, i, ctx->tesselator_idx_counter % 2);
	ctx->tesselator_idx_counter++;
}
void triangle_vertex2(VKVG_IBO_INDEX_TYPE v, VkvgContext ctx) {
	VKVG_IBO_INDEX_TYPE i = (VKVG_IBO_INDEX_TYPE)v;
	_add_indice(ctx, i);
}
void skip_vertex2(VKVG_IBO_INDEX_TYPE v, VkvgContext ctx) {}
void begin2(GLenum which, void* poly_data) {
	VkvgContext ctx = (VkvgContext)poly_data;
	switch (which) {
	case GL_TRIANGLES:
		ctx->vertex_cb = &triangle_vertex2;
		break;
	case GL_TRIANGLE_STRIP:
		ctx->tesselator_idx_counter = 0;
		ctx->vertex_cb = &strip_vertex2;
		break;
	case GL_TRIANGLE_FAN:
		ctx->tesselator_idx_counter = ctx->tesselator_fan_start = 0;
		ctx->vertex_cb = &fan_vertex2;
		break;
	default:
		fprintf(stderr, "ERROR, can't handle %d\n", (int)which);
		ctx->vertex_cb = &skip_vertex2;
	}
}

void combine2(const GLdouble newVertex[3], const void* neighborVertex_s[4], const GLfloat neighborWeight[4],
	void** outData, void* poly_data) {
	VkvgContext ctx = (VkvgContext)poly_data;
	Vertex      v = { {newVertex[0], newVertex[1]}, ctx->curColor, {0, 0, -1} };
	*outData = (void*)((unsigned long)(ctx->vertCount - ctx->curVertOffset));
	_add_vertex(ctx, v);
}
void vertex2(void* vertex_data, void* poly_data) {
	VKVG_IBO_INDEX_TYPE i = (VKVG_IBO_INDEX_TYPE)vertex_data;
	VkvgContext         ctx = (VkvgContext)poly_data;
	ctx->vertex_cb(i, ctx);
}
void _fill_non_zero(VkvgContext ctx) {
	Vertex v = { {0}, ctx->curColor, {0, 0, -1} };

	uint32_t ptrPath = 0;
	uint32_t firstPtIdx = 0;

	if (ctx->pathPtr == 1 && ctx->pathes[0] & PATH_IS_CONVEX_BIT) {
		// simple concave rectangle or circle
		VKVG_IBO_INDEX_TYPE firstVertIdx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);
		uint32_t            pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;

		_ensure_vertex_cache_size(ctx, pathPointCount);
		_ensure_index_cache_size(ctx, (pathPointCount - 2) * 3);

		VKVG_IBO_INDEX_TYPE i = 0;
		while (i < 2) {
			v.pos = ctx->points[i++];
			_set_vertex(ctx, ctx->vertCount++, v);
		}
		while (i < pathPointCount) {
			v.pos = ctx->points[i];
			_set_vertex(ctx, ctx->vertCount++, v);
			_add_triangle_indices_unchecked(ctx, firstVertIdx, firstVertIdx + i - 1, firstVertIdx + i);
			i++;
		}
		return;
	}

	GLUtesselator* tess = gluNewTess();
	gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
	gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (GLvoid(*)()) & vertex2);
	gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (GLvoid(*)()) & begin2);
	gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (GLvoid(*)()) & combine2);

	gluTessBeginPolygon(tess, ctx);

	while (ptrPath < ctx->pathPtr) {
		uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;

		if (pathPointCount > 2) {
			VKVG_IBO_INDEX_TYPE firstVertIdx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);
			gluTessBeginContour(tess);

			VKVG_IBO_INDEX_TYPE i = 0;

			while (i < pathPointCount) {
				v.pos = ctx->points[i + firstPtIdx];
				double dp[] = { v.pos.x, v.pos.y, 0 };
				_add_vertex(ctx, v);
				gluTessVertex(tess, dp, (void*)((unsigned long)firstVertIdx + i));
				i++;
			}
			gluTessEndContour(tess);

			// limit batch size here to 1/3 of the ibo index type ability
			// if (ctx->vertCount - ctx->curVertOffset > VKVG_IBO_MAX / 3)
			//	_emit_draw_cmd_undrawn_vertices(ctx);
		}

		firstPtIdx += pathPointCount;
		if (_path_has_curves(ctx, ptrPath)) {
			// skip segments lengths used in stroke
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
#endif
#if 1
// create fill from current path with ear clipping technic
void ear_fill_non_zero(VkvgContext ctx) {
	Vertex v = { {0}, ctx->curColor, {0, 0, -1} };

	uint32_t& capPathPointCount = ctx->capPathPointCount;
	auto& ecps = ctx->ecps;
	uint32_t ptrPath = 0;
	uint32_t firstPtIdx = 0;
	while (ptrPath < ctx->pathPtr) {
		uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;

		if (pathPointCount > 2) {
			VKVG_IBO_INDEX_TYPE firstVertIdx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);
			// ear_clip_point* ecps = (ear_clip_point*)malloc(pathPointCount*sizeof(ear_clip_point));
			if (!ecps || pathPointCount > capPathPointCount)
			{
				auto newCount = pathPointCount + 64;
				ear_clip_point* kp = (ear_clip_point*)realloc(ecps, newCount * sizeof(ear_clip_point));
				if (kp)
				{
					ecps = kp;
					capPathPointCount = newCount;
				}
			}
			if (!ecps)break;
			//ear_clip_point      ecps[pathPointCount];
			uint32_t            ecps_count = pathPointCount;
			VKVG_IBO_INDEX_TYPE i = 0;

			// init points link list
			while (i < pathPointCount - 1) {
				v.pos = ctx->points[i + firstPtIdx];
				ear_clip_point ecp = { v.pos, firstVertIdx + i, &ecps[i + 1] };
				ecps[i] = ecp;
				_add_vertex(ctx, v);
				i++;
			}

			v.pos = ctx->points[i + firstPtIdx];
			ear_clip_point ecp = { v.pos, firstVertIdx + i, ecps };
			ecps[i] = ecp;
			_add_vertex(ctx, v);

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
					_add_triangle_indices(ctx, v0->idx, v1->idx, v2->idx);
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
				_add_triangle_indices(ctx, ecp_current->next->idx, ecp_current->idx, ecp_current->next->next->idx);

			// limit batch size here to 1/3 of the ibo index type ability
			if (ctx->vertCount - ctx->curVertOffset > VKVG_IBO_MAX / 3)
				_emit_draw_cmd_undrawn_vertices(ctx);
		}

		firstPtIdx += pathPointCount;
		if (_path_has_curves(ctx, ptrPath)) {
			// skip segments lengths used in stroke
			ptrPath++;
			uint32_t totPts = 0;
			while (totPts < pathPointCount)
				totPts += (ctx->pathes[ptrPath++] & PATH_ELT_MASK);
		}
		else
			ptrPath++;
	}
	//if (ecps)free(ecps);
}
#endif

void _vkvg_path_extents(VkvgContext ctx, bool transformed, float* x1, float* y1, float* x2, float* y2) {
	uint32_t ptrPath = 0;
	uint32_t firstPtIdx = 0;

	float xMin = FLT_MAX, yMin = FLT_MAX;
	float xMax = FLT_MIN, yMax = FLT_MIN;

	while (ptrPath < ctx->pathPtr) {
		uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;

		for (uint32_t i = firstPtIdx; i < firstPtIdx + pathPointCount; i++) {
			vec2 p = ctx->points[i];
			if (transformed)
				vkvg_matrix_transform_point(&ctx->pushConsts.mat, &p.x, &p.y);
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
		if (_path_has_curves(ctx, ptrPath)) {
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

void _draw_full_screen_quad(VkvgContext ctx, vec4* scissor) {
#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_cmd_label_start(ctx->cmd, "_draw_full_screen_quad", DBG_LAB_COLOR_FSQ);
#endif
	if (scissor) {
		VkRect2D r = { {(int32_t)MAX(scissor->xMin, 0), (int32_t)MAX(scissor->yMin, 0)},
					  {(int32_t)MAX(scissor->xMax - (int32_t)scissor->xMin + 1, 1),
					   (int32_t)MAX(scissor->yMax - (int32_t)scissor->yMin + 1, 1)} };
		CmdSetScissor(ctx->cmd, 0, 1, &r);
	}

	uint32_t firstVertIdx = ctx->vertCount;
	_ensure_vertex_cache_size(ctx, 3);

	_add_vertexf_unchecked(ctx, -1, -1);
	_add_vertexf_unchecked(ctx, 3, -1);
	_add_vertexf_unchecked(ctx, -1, 3);

	ctx->curVertOffset = ctx->vertCount;

	ctx->pushConsts.fsq_patternType |= FULLSCREEN_BIT;
	CmdPushConstants(ctx->cmd, ctx->dev->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 24, 4,
		&ctx->pushConsts.fsq_patternType);
	CmdDraw(ctx->cmd, 3, 1, firstVertIdx, 0);
	ctx->pushConsts.fsq_patternType &= ~FULLSCREEN_BIT;
	CmdPushConstants(ctx->cmd, ctx->dev->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 24, 4,
		&ctx->pushConsts.fsq_patternType);
	if (scissor)
		CmdSetScissor(ctx->cmd, 0, 1, &ctx->bounds);

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_cmd_label_end(ctx->cmd);
#endif
}

void _select_font_face(VkvgContext ctx, const char* name) {
	if (strcmp(ctx->selectedFontName, name) == 0)
		return;
	strcpy(ctx->selectedFontName, name);
	ctx->currentFont = NULL;
	ctx->currentFontSize = NULL;
}

#endif // 1

// dev
#if 1 

#define TRY_LOAD_DEVICE_EXT(ext)                                                                                       \
    {                                                                                                                  \
        if (vkh_phyinfo_try_get_extension_properties(pi, #ext, NULL))                                                  \
            enabledExts[enabledExtsCount++] = #ext;                                                                    \
    }

#define _CHECK_INST_EXT(ext)                                                                                           \
if (vkh_instance_extension_supported(#ext)) {                                                                      \
        if (pExtensions)                                                                                               \
        pExtensions[*pExtCount] = #ext;                                                                            \
        (*pExtCount)++;                                                                                                \
}

#define _CHECK_DEV_EXT(ext)                                                                                            \
{                                                                                                                  \
        if (_get_dev_extension_is_supported(pExtensionProperties, extensionCount, #ext)) {                             \
            if (pExtensions)                                                                                           \
            pExtensions[*pExtCount] = #ext;                                                                        \
            (*pExtCount)++;                                                                                            \
    }                                                                                                              \
}

void vkvg_device_set_context_cache_size(VkvgDevice dev, uint32_t maxCount) {
	if (maxCount == dev->cachedContextMaxCount)
		return;

	dev->cachedContextMaxCount = maxCount;

	_cached_ctx* cur = dev->cachedContextLast;
	while (cur && dev->cachedContextCount > dev->cachedContextMaxCount) {
		_release_context_ressources(cur->ctx);
		_cached_ctx* prev = cur;
		cur = cur->pNext;
		free(prev);
		dev->cachedContextCount--;
	}
	dev->cachedContextLast = cur;
}
void _device_init(VkvgDevice dev, const vkvg_device_create_info_t* info) {
	dev->vkDev = info->vkdev;
	dev->phy = info->phy;
	dev->instance = info->inst;
	dev->hdpi = 72;
	dev->vdpi = 72;
	dev->samples = info->samples;
	if (dev->samples == VK_SAMPLE_COUNT_1_BIT)
		dev->deferredResolve = false;
	else
		dev->deferredResolve = info->deferredResolve;

	dev->cachedContextMaxCount = VKVG_MAX_CACHED_CONTEXT_COUNT;

#if VKVG_DBG_STATS
	dev->debug_stats = (vkvg_debug_stats_t){ 0 };
#endif

	VkFormat format = FB_COLOR_FORMAT;

	_device_check_best_image_tiling(dev, format);
	if (dev->status != VKVG_STATUS_SUCCESS)
		return;

	VkhDevice vkhd = (VkhDevice)&dev->vkDev;

	if (!_device_init_function_pointers(dev)) {
		dev->status = VKVG_STATUS_NULL_POINTER;
		return;
	}

	VkhPhyInfo phyInfos = vkh_phyinfo_create(dev->phy, NULL);

	dev->phyMemProps = phyInfos->memProps;
	dev->gQueue = vkh_queue_create(vkhd, info->qFamIdx, info->qIndex);
	// mtx_init (&dev->gQMutex, mtx_plain);

	vkh_phyinfo_destroy(phyInfos);

#ifdef VKH_USE_VMA
	VmaAllocatorCreateInfo allocatorInfo = { .physicalDevice = info->phy, .device = info->vkdev };
	vmaCreateAllocator(&allocatorInfo, (VmaAllocator*)&dev->allocator);
#endif

	dev->cmdPool = vkh_cmd_pool_create(vkhd, dev->gQueue->familyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	dev->cmd = vkh_cmd_buff_create(vkhd, dev->cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	dev->fence = vkh_fence_create_signaled(vkhd);

	_device_create_pipeline_cache(dev);
	_fonts_cache_create(dev);
	if (dev->deferredResolve || dev->samples == VK_SAMPLE_COUNT_1_BIT) {
		dev->renderPass =
			_device_createRenderPassNoResolve(dev, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_LOAD);
		dev->renderPass_ClearStencil =
			_device_createRenderPassNoResolve(dev, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_CLEAR);
		dev->renderPass_ClearAll =
			_device_createRenderPassNoResolve(dev, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_CLEAR);
	}
	else {
		dev->renderPass = _device_createRenderPassMS(dev, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_LOAD);
		dev->renderPass_ClearStencil =
			_device_createRenderPassMS(dev, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_CLEAR);
		dev->renderPass_ClearAll =
			_device_createRenderPassMS(dev, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_CLEAR);
	}
	_device_createDescriptorSetLayout(dev);
	_device_setupPipelines(dev);

	_device_create_empty_texture(dev, format, dev->supportedTiling);

#ifdef DEBUG
#if defined(__linux__) && defined(__GLIBC__)
	_linux_register_error_handler();
#endif
#ifdef VKVG_DBG_UTILS
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)dev->cmdPool, "Device Cmd Pool");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)dev->cmd, "Device Cmd Buff");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_FENCE, (uint64_t)dev->fence, "Device Fence");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)dev->renderPass, "RP load img/stencil");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)dev->renderPass_ClearStencil,
		"RP clear stencil");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)dev->renderPass_ClearAll, "RP clear all");

	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)dev->dslSrc, "DSLayout SOURCE");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)dev->dslFont, "DSLayout FONT");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)dev->dslGrad, "DSLayout GRADIENT");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)dev->pipelineLayout, "PLLayout dev");

#ifndef __APPLE__
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_PIPELINE, (uint64_t)dev->pipelinePolyFill, "PL Poly fill");
#endif
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_PIPELINE, (uint64_t)dev->pipelineClipping, "PL Clipping");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_PIPELINE, (uint64_t)dev->pipe_OVER, "PL draw Over");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_PIPELINE, (uint64_t)dev->pipe_SUB, "PL draw Substract");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_PIPELINE, (uint64_t)dev->pipe_CLEAR, "PL draw Clear");

	vkh_image_set_name(dev->emptyImg, "empty IMG");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)vkh_image_get_view(dev->emptyImg),
		"empty IMG VIEW");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_SAMPLER, (uint64_t)vkh_image_get_sampler(dev->emptyImg),
		"empty IMG SAMPLER");
#endif
#endif
	dev->status = VKVG_STATUS_SUCCESS;
}


void vkvg_get_required_instance_extensions(const char** pExtensions, uint32_t* pExtCount) {
	*pExtCount = 0;

	vkh_instance_extensions_check_init();

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	_CHECK_INST_EXT(VK_EXT_debug_utils)
#endif
		_CHECK_INST_EXT(VK_KHR_get_physical_device_properties2)

		vkh_instance_extensions_check_release();
}

bool _get_dev_extension_is_supported(VkExtensionProperties* pExtensionProperties, uint32_t extensionCount,
	const char* name) {
	for (uint32_t i = 0; i < extensionCount; i++) {
		if (strcmp(name, pExtensionProperties[i].extensionName) == 0)
			return true;
	}
	return false;
}


vkvg_status_t vkvg_get_required_device_extensions(VkPhysicalDevice phy, const char** pExtensions, uint32_t* pExtCount) {
	VkExtensionProperties* pExtensionProperties;
	uint32_t               extensionCount;

	*pExtCount = 0;
	VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(phy, NULL, &extensionCount, NULL));
	pExtensionProperties = (VkExtensionProperties*)malloc(extensionCount * sizeof(VkExtensionProperties));
	VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(phy, NULL, &extensionCount, pExtensionProperties));

	// https://vulkan.lunarg.com/doc/view/1.2.162.0/mac/1.2-extensions/vkspec.html#VK_KHR_portability_subset
	_CHECK_DEV_EXT(VK_KHR_portability_subset);
	VkPhysicalDeviceFeatures2 phyFeat2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

#ifdef VKVG_ENABLE_VK_SCALAR_BLOCK_LAYOUT
	// ensure feature is implemented by driver.
	VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutSupport = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES };
	phyFeat2.pNext = &scalarBlockLayoutSupport;
#endif

#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreSupport = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES };
	timelineSemaphoreSupport.pNext = phyFeat2.pNext;
	phyFeat2.pNext = &timelineSemaphoreSupport;
#endif

	vkGetPhysicalDeviceFeatures2(phy, &phyFeat2);

#ifdef VKVG_ENABLE_VK_SCALAR_BLOCK_LAYOUT
	if (!scalarBlockLayoutSupport.scalarBlockLayout) {
		LOG(VKVG_LOG_ERR, "CREATE Device failed, vkvg compiled with VKVG_ENABLE_VK_SCALAR_BLOCK_LAYOUT and feature is "
			"not implemented for physical device.\n");
		return VKVG_STATUS_DEVICE_ERROR;
	}
	_CHECK_DEV_EXT(VK_EXT_scalar_block_layout)
#endif
#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
		if (!timelineSemaphoreSupport.timelineSemaphore) {
			LOG(VKVG_LOG_ERR, "CREATE Device failed, VK_SEMAPHORE_TYPE_TIMELINE not supported.\n");
			return VKVG_STATUS_DEVICE_ERROR;
		}
	_CHECK_DEV_EXT(VK_KHR_timeline_semaphore)
#endif

		return VKVG_STATUS_SUCCESS;
}

// enabledFeature12 is guarantied to be the first in pNext chain
const void* vkvg_get_device_requirements(VkPhysicalDeviceFeatures* pEnabledFeatures) {

	pEnabledFeatures->fillModeNonSolid = VK_TRUE;
	pEnabledFeatures->sampleRateShading = VK_TRUE;
	pEnabledFeatures->logicOp = VK_TRUE;

	void* pNext = NULL;

#ifdef VK_VERSION_1_2
	static VkPhysicalDeviceVulkan12Features enabledFeatures12 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
#ifdef VKVG_ENABLE_VK_SCALAR_BLOCK_LAYOUT
		,
		.scalarBlockLayout = VK_TRUE
#endif
#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
		,
		.timelineSemaphore = VK_TRUE
#endif
	};
	enabledFeatures12.pNext = pNext;
	pNext = &enabledFeatures12;
#else
#ifdef VKVG_ENABLE_VK_SCALAR_BLOCK_LAYOUT
	static VkPhysicalDeviceScalarBlockLayoutFeaturesEXT scalarBlockFeat = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT, .scalarBlockLayout = VK_TRUE };
	scalarBlockFeat.pNext = pNext;
	pNext = &scalarBlockFeat;
#endif
#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	static VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timelineSemaFeat = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR, .timelineSemaphore = VK_TRUE };
	timelineSemaFeat.pNext = pNext;
	pNext = &timelineSemaFeat;
#endif
#endif

	return pNext;
}

VkvgDevice vkvg_device_create(vkvg_device_create_info_t* info) {
	LOG(VKVG_LOG_INFO, "CREATE Device\n");
	if (!info) {
		LOG(VKVG_LOG_ERR, "CREATE Device failed, provided vkvg_device_create_info_t is null\n");
		return (VkvgDevice)&_vkvg_status_invalid_dev_ci;
	}
	VkvgDevice dev = (vkvg_device*)calloc(1, sizeof(vkvg_device));
	if (!dev) {
		LOG(VKVG_LOG_ERR, "CREATE Device failed, no memory\n");
		return (VkvgDevice)&_vkvg_status_no_memory;
	}

	dev->references = 1;

	dev->threadAware = info->threadAware;

	if (!info->vkdev) {
		const char* enabledExts[10];
		const char* enabledLayers[10];
		uint32_t    enabledExtsCount = 0, enabledLayersCount = 0, phyCount = 0;

		vkh_layers_check_init();

#ifdef VKVG_USE_VALIDATION
		if (vkh_layer_is_present("VK_LAYER_KHRONOS_validation"))
			enabledLayers[enabledLayersCount++] = "VK_LAYER_KHRONOS_validation";
#endif

#ifdef VKVG_USE_RENDERDOC
		if (vkh_layer_is_present("VK_LAYER_RENDERDOC_Capture"))
			enabledLayers[enabledLayersCount++] = "VK_LAYER_RENDERDOC_Capture";
#endif
		vkh_layers_check_release();

		vkvg_get_required_instance_extensions(enabledExts, &enabledExtsCount);

#ifdef VK_VERSION_1_2
		VkhApp app = vkh_app_create(1, 2, "vkvg", enabledLayersCount, enabledLayers, enabledExtsCount, enabledExts);
#else
		VkhApp app = vkh_app_create(1, 1, "vkvg", enabledLayersCount, enabledLayers, enabledExtsCount, enabledExts);
#endif

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
		vkh_app_enable_debug_messenger(app, VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, NULL);
#endif

		VkhPhyInfo* phys = vkh_app_get_phyinfos(app, &phyCount, VK_NULL_HANDLE);
		if (phyCount == 0) {
			dev->status = VKVG_STATUS_DEVICE_ERROR;
			vkh_app_destroy(app);
			return dev;
		}

		VkhPhyInfo pi = 0;
		if (!_device_try_get_phyinfo(phys, phyCount, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, &pi))
			if (!_device_try_get_phyinfo(phys, phyCount, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, &pi))
				pi = phys[0];

		if (!info->samples) // if not set, default to 1 sample
			info->samples = VK_SAMPLE_COUNT_1_BIT;

		if (!(pi->properties.limits.framebufferColorSampleCounts & info->samples)) {
			LOG(VKVG_LOG_ERR, "CREATE Device failed: sample count not supported: %d\n", info->samples);
			dev->status = VKVG_STATUS_DEVICE_ERROR;
			vkh_app_free_phyinfos(phyCount, phys);
			vkh_app_destroy(app);
			return dev;
		}

		uint32_t                qCount = 0;
		float                   qPriorities[] = { 0.0 };
		VkDeviceQueueCreateInfo pQueueInfos[3] = {};// { { 0 }, { 0 }, { 0 } };

		if (vkh_phyinfo_create_queues(pi, pi->gQueue, 1, qPriorities, &pQueueInfos[qCount]))
			qCount++;

		enabledExtsCount = 0;

		if (vkvg_get_required_device_extensions(pi->phy, enabledExts, &enabledExtsCount) != VKVG_STATUS_SUCCESS) {
			dev->status = VKVG_STATUS_DEVICE_ERROR;
			vkh_app_free_phyinfos(phyCount, phys);
			vkh_app_destroy(app);
			return dev;
		}

		VkPhysicalDeviceFeatures enabledFeatures = { 0 };
		const void* pNext = vkvg_get_device_requirements(&enabledFeatures);

		VkDeviceCreateInfo device_info = {}; device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.queueCreateInfoCount = qCount;
		device_info.pQueueCreateInfos = (VkDeviceQueueCreateInfo*)&pQueueInfos;
		device_info.enabledExtensionCount = enabledExtsCount;
		device_info.ppEnabledExtensionNames = enabledExts;
		device_info.pEnabledFeatures = &enabledFeatures;
		device_info.pNext = pNext;
		dev->vkhDev = vkh_device_create(app, pi, &device_info);

		info->inst = vkh_app_get_inst(app);
		info->phy = vkh_device_get_phy(dev->vkhDev);
		info->vkdev = vkh_device_get_vkdev(dev->vkhDev);
		info->qFamIdx = pi->gQueue;
		info->qIndex = 0;

		vkh_app_free_phyinfos(phyCount, phys);
	}

	_device_init(dev, info);
	if (dev->threadAware) {
		mtx_init(&dev->mutex, mtx_plain);
		mtx_init(&dev->fontCache->mutex, mtx_plain);
		dev->threadAware = true;
	}
	return dev;
}

void vkvg_device_destroy(VkvgDevice dev) {
	if (vkvg_device_status(dev)) {
		LOG(VKVG_LOG_ERR, "DESTROY Device failed, see Status for info");
		return;
	}
	LOCK_DEVICE
		dev->references--;
	if (dev->references > 0) {
		UNLOCK_DEVICE
			return;
	}
	UNLOCK_DEVICE

		LOG(VKVG_LOG_INFO, "DESTROY Device\n");

	if (dev->cachedContextCount > 0) {
		_cached_ctx* cur = dev->cachedContextLast;
		while (cur) {
			assert(cur->ctx->status == VKVG_STATUS_IN_CACHE);
			cur->ctx->status = VKVG_STATUS_SUCCESS;
			_release_context_ressources(cur->ctx);
			_cached_ctx* prev = cur;
			cur = cur->pNext;
			free(prev);
		}
	}

	vkDeviceWaitIdle(dev->vkDev);

	vkh_image_destroy(dev->emptyImg);

	vkDestroyDescriptorSetLayout(dev->vkDev, dev->dslGrad, NULL);
	vkDestroyDescriptorSetLayout(dev->vkDev, dev->dslPushDset, NULL);
	vkDestroyDescriptorSetLayout(dev->vkDev, dev->dslFont, NULL);
	vkDestroyDescriptorSetLayout(dev->vkDev, dev->dslSrc, NULL);
#ifndef __APPLE__
	vkDestroyPipeline(dev->vkDev, dev->pipelinePolyFill, NULL);
#endif
	vkDestroyPipeline(dev->vkDev, dev->pipelineClipping, NULL);

	vkDestroyPipeline(dev->vkDev, dev->pipe_OVER, NULL);
	vkDestroyPipeline(dev->vkDev, dev->pipe_SUB, NULL);
	vkDestroyPipeline(dev->vkDev, dev->pipe_CLEAR, NULL);

#ifdef VKVG_WIRED_DEBUG
	vkDestroyPipeline(dev->vkDev, dev->pipelineWired, NULL);
	vkDestroyPipeline(dev->vkDev, dev->pipelineLineList, NULL);
#endif

	vkDestroyPipelineLayout(dev->vkDev, dev->pipelineLayout, NULL);
	vkDestroyPipelineCache(dev->vkDev, dev->pipelineCache, NULL);
	vkDestroyRenderPass(dev->vkDev, dev->renderPass, NULL);
	vkDestroyRenderPass(dev->vkDev, dev->renderPass_ClearStencil, NULL);
	vkDestroyRenderPass(dev->vkDev, dev->renderPass_ClearAll, NULL);

	vkWaitForFences(dev->vkDev, 1, &dev->fence, VK_TRUE, UINT64_MAX);
	vkDestroyFence(dev->vkDev, dev->fence, NULL);

	vkFreeCommandBuffers(dev->vkDev, dev->cmdPool, 1, &dev->cmd);
	vkDestroyCommandPool(dev->vkDev, dev->cmdPool, NULL);

	vkh_queue_destroy(dev->gQueue);

	_font_cache_destroy(dev);

#ifdef VKH_USE_VMA
	vmaDestroyAllocator((VmaAllocator)dev->allocator);
#endif

	if (dev->threadAware) {
		mtx_destroy(&dev->mutex);
		mtx_destroy(&dev->fontCache->mutex);
	}

	if (dev->vkhDev) {
		VkhApp app = vkh_device_get_app(dev->vkhDev);
		vkh_device_destroy(dev->vkhDev);
		vkh_app_destroy(app);
	}

	free(dev);
	dev = NULL;
}

vkvg_status_t vkvg_device_status(VkvgDevice dev) { return !dev ? VKVG_STATUS_NULL_POINTER : dev->status; }
VkvgDevice    vkvg_device_reference(VkvgDevice dev) {
	if (!vkvg_device_status(dev)) {
		LOCK_DEVICE
			dev->references++;
		UNLOCK_DEVICE
	}
	return dev;
}
uint32_t vkvg_device_get_reference_count(VkvgDevice dev) { return vkvg_device_status(dev) ? 0 : dev->references; }
// TODO dpy reorganisation
void vkvg_device_set_dpy(VkvgDevice dev, int hdpy, int vdpy) {
	if (vkvg_device_status(dev))
		return;
	dev->hdpi = hdpy;
	dev->vdpi = vdpy;

	// TODO: reset font cache
}
void vkvg_device_get_dpy(VkvgDevice dev, int* hdpy, int* vdpy) {
	if (vkvg_device_status(dev))
		return;
	*hdpy = dev->hdpi;
	*vdpy = dev->vdpi;
}
#if VKVG_DBG_STATS
vkvg_debug_stats_t vkvg_device_get_stats(VkvgDevice dev) {
	return vkvg_device_status(dev) ? (vkvg_debug_stats_t) { 0 } : dev->debug_stats;
}
void vkvg_device_reset_stats(VkvgDevice dev) {
	if (vkvg_device_status(dev))
		return;
	dev->debug_stats = (vkvg_debug_stats_t){ 0 };
}
#endif

#define GetInstProcAddress(inst, func) (PFN_##func) vkGetInstanceProcAddr(inst, #func);

#define GetVkProcAddress(dev, inst, func)                                                                              \
    (vkGetDeviceProcAddr(dev, #func) == NULL) ? (PFN_##func)vkGetInstanceProcAddr(inst, #func)                         \
                                              : (PFN_##func)vkGetDeviceProcAddr(dev, #func)

uint32_t vkvg_log_level = VKVG_LOG_DEBUG;
#ifdef VKVG_WIRED_DEBUG
vkvg_wired_debug_mode vkvg_wired_debug = vkvg_wired_debug_mode_normal;
#endif

bool _device_try_get_phyinfo(VkhPhyInfo* phys, uint32_t phyCount, VkPhysicalDeviceType gpuType, VkhPhyInfo* phy) {
	for (uint32_t i = 0; i < phyCount; i++) {
		if (vkh_phyinfo_get_properties(phys[i]).deviceType == gpuType) {
			*phy = phys[i];
			return true;
		}
	}
	return false;
}
// TODO:save/reload cache in user temp directory
void _device_create_pipeline_cache(VkvgDevice dev) {

	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	VK_CHECK_RESULT(vkCreatePipelineCache(dev->vkDev, &pipelineCacheCreateInfo, NULL, &dev->pipelineCache));
}

VkRenderPass _device_createRenderPassNoResolve(VkvgDevice dev, VkAttachmentLoadOp loadOp,
	VkAttachmentLoadOp stencilLoadOp) {
	VkAttachmentDescription attColor = { .format = FB_COLOR_FORMAT,
										.samples = (VkSampleCountFlagBits)dev->samples,
										.loadOp = loadOp,
										.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
										.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
										.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
										.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
										.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentDescription attDS = { .format = dev->stencilFormat,
										.samples = (VkSampleCountFlagBits)dev->samples,
										.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
										.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
										.stencilLoadOp = stencilLoadOp,
										.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
										.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
										.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkAttachmentDescription attachments[] = { attColor, attDS };
	VkAttachmentReference   colorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference   dsRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = { .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
											   .colorAttachmentCount = 1,
											   .pColorAttachments = &colorRef,
											   .pDepthStencilAttachment = &dsRef };

	VkSubpassDependency dependencies[] = {
		{VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		 VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		 VK_DEPENDENCY_BY_REGION_BIT},
		{0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		 VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
		 VK_DEPENDENCY_BY_REGION_BIT},
	};

	VkRenderPassCreateInfo renderPassInfo = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
											 .attachmentCount = 2,
											 .pAttachments = attachments,
											 .subpassCount = 1,
											 .pSubpasses = &subpassDescription,
											 .dependencyCount = 2,
											 .pDependencies = dependencies };
	VkRenderPass           rp;
	VK_CHECK_RESULT(vkCreateRenderPass(dev->vkDev, &renderPassInfo, NULL, &rp));
	return rp;
}
VkRenderPass _device_createRenderPassMS(VkvgDevice dev, VkAttachmentLoadOp loadOp, VkAttachmentLoadOp stencilLoadOp) {
	VkAttachmentDescription attColor = { .format = FB_COLOR_FORMAT,
											   .samples = (VkSampleCountFlagBits)dev->samples,
											   .loadOp = loadOp,
											   .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
											   .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
											   .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
											   .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
											   .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentDescription attColorResolve = { .format = FB_COLOR_FORMAT,
											   .samples = VK_SAMPLE_COUNT_1_BIT,
											   .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
											   .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
											   .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
											   .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
											   .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
											   .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentDescription attDS = { .format = dev->stencilFormat,
											   .samples = (VkSampleCountFlagBits)dev->samples,
											   .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
											   .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
											   .stencilLoadOp = stencilLoadOp,
											   .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
											   .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
											   .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkAttachmentDescription attachments[] = { attColorResolve, attDS, attColor };
	VkAttachmentReference   resolveRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference   dsRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	VkAttachmentReference   colorRef = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = { .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
											   .colorAttachmentCount = 1,
											   .pColorAttachments = &colorRef,
											   .pResolveAttachments = &resolveRef,
											   .pDepthStencilAttachment = &dsRef };

	VkSubpassDependency dependencies[] = {
		{VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		 VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		 VK_DEPENDENCY_BY_REGION_BIT},
		{0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		 VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
		 VK_DEPENDENCY_BY_REGION_BIT},
	};

	VkRenderPassCreateInfo renderPassInfo = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
											 .attachmentCount = 3,
											 .pAttachments = attachments,
											 .subpassCount = 1,
											 .pSubpasses = &subpassDescription,
											 .dependencyCount = 2,
											 .pDependencies = dependencies };
	VkRenderPass           rp;
	VK_CHECK_RESULT(vkCreateRenderPass(dev->vkDev, &renderPassInfo, NULL, &rp));
	return rp;
}

void _device_setupPipelines(VkvgDevice dev) {
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
													   .renderPass = dev->renderPass };

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
#ifdef VKVG_PREMULT_ALPHA
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.alphaBlendOp = VK_BLEND_OP_ADD,
#else
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
#endif
		.colorWriteMask = 0x0,
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
													 .dynamicStateCount = 2,
													 .pDynamicStates = dynamicStateEnables };

	VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .scissorCount = 1 };

	VkPipelineMultisampleStateCreateInfo multisampleState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = (VkSampleCountFlagBits)dev->samples };
	/*if (dev->samples != VK_SAMPLE_COUNT_1_BIT){
		multisampleState.sampleShadingEnable = VK_TRUE;
		multisampleState.minSampleShading = 0.5f;
	}*/
	VkVertexInputBindingDescription vertexInputBinding = {
		.binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };

	VkVertexInputAttributeDescription vertexInputAttributs[3] = { {0, 0, VK_FORMAT_R32G32_SFLOAT, 0},
																 {1, 0, VK_FORMAT_R8G8B8A8_UNORM, 8},
																 {2, 0, VK_FORMAT_R32G32B32_SFLOAT, 12} };

	VkPipelineVertexInputStateCreateInfo vertexInputState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertexInputBinding,
		.vertexAttributeDescriptionCount = 3,
		.pVertexAttributeDescriptions = vertexInputAttributs };
#ifdef VKVG_WIRED_DEBUG
	VkShaderModule modVert, modFrag, modFragWired;
#else
	VkShaderModule modVert, modFrag;
#endif
	VkShaderModuleCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
										   .codeSize = vkvg_main_vert_spv_len,
										   .pCode = (uint32_t*)vkvg_main_vert_spv };
	VK_CHECK_RESULT(vkCreateShaderModule(dev->vkDev, &createInfo, NULL, &modVert));
#if defined(VKVG_LCD_FONT_FILTER) && defined(FT_CONFIG_OPTION_SUBPIXEL_RENDERING)
	createInfo.pCode = (uint32_t*)vkvg_main_lcd_frag_spv;
	createInfo.codeSize = vkvg_main_lcd_frag_spv_len;
#else
	createInfo.pCode = (uint32_t*)vkvg_main_frag_spv;
	createInfo.codeSize = vkvg_main_frag_spv_len;
	createInfo.pCode = (uint32_t*)vkvg_main_frag1_spv;
	createInfo.codeSize = sizeof(vkvg_main_frag1_spv);
#endif
	VK_CHECK_RESULT(vkCreateShaderModule(dev->vkDev, &createInfo, NULL, &modFrag));

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

	// Use specialization constants to pass number of samples to the shader (used for MSAA resolve)
	/*VkSpecializationMapEntry specializationEntry = {
		.constantID = 0,
		.offset = 0,
		.size = sizeof(uint32_t)};
	uint32_t specializationData = VKVG_SAMPLES;
	VkSpecializationInfo specializationInfo = {
		.mapEntryCount = 1,
		.pMapEntries = &specializationEntry,
		.dataSize = sizeof(specializationData),
		.pData = &specializationData};*/

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
	pipelineCreateInfo.layout = dev->pipelineLayout;

#ifndef __APPLE__
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(dev->vkDev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL,
		&dev->pipelinePolyFill));
#endif

	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	dsStateCreateInfo.back = dsStateCreateInfo.front = clipingOpState;
	dynamicState.dynamicStateCount = 5;
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(dev->vkDev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL,
		&dev->pipelineClipping));

	dsStateCreateInfo.back = dsStateCreateInfo.front = stencilOpState;
	blendAttachmentState.colorWriteMask = 0xf;
	dynamicState.dynamicStateCount = 3;
	pipelineCreateInfo.stageCount = 2;
	VK_CHECK_RESULT(
		vkCreateGraphicsPipelines(dev->vkDev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &dev->pipe_OVER));

	blendAttachmentState.alphaBlendOp = blendAttachmentState.colorBlendOp = VK_BLEND_OP_SUBTRACT;
	VK_CHECK_RESULT(
		vkCreateGraphicsPipelines(dev->vkDev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &dev->pipe_SUB));

	colorBlendState.logicOpEnable = VK_TRUE;
	blendAttachmentState.blendEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_CLEAR;
	VK_CHECK_RESULT(
		vkCreateGraphicsPipelines(dev->vkDev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &dev->pipe_CLEAR));

#ifdef VKVG_WIRED_DEBUG
	colorBlendState.logicOpEnable = VK_FALSE;
	blendAttachmentState.blendEnable = VK_TRUE;
	colorBlendState.logicOp = VK_LOGIC_OP_CLEAR;

	createInfo.pCode = (uint32_t*)wired_frag_spv;

	createInfo.codeSize = wired_frag_spv_len;
	VK_CHECK_RESULT(vkCreateShaderModule(dev->vkDev, &createInfo, NULL, &modFragWired));

	shaderStages[1].module = modFragWired;

	rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(dev->vkDev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL,
		&dev->pipelineLineList));

	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	VK_CHECK_RESULT(
		vkCreateGraphicsPipelines(dev->vkDev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &dev->pipelineWired));

	vkDestroyShaderModule(dev->vkDev, modFragWired, NULL);
#endif

	vkDestroyShaderModule(dev->vkDev, modVert, NULL);
	vkDestroyShaderModule(dev->vkDev, modFrag, NULL);
}

void _device_createDescriptorSetLayout(VkvgDevice dev) {

	VkDescriptorSetLayoutBinding    dsLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
														  VK_SHADER_STAGE_FRAGMENT_BIT, NULL };
	VkDescriptorSetLayoutCreateInfo dsLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &dsLayoutBinding };
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(dev->vkDev, &dsLayoutCreateInfo, NULL, &dev->dslFont));
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(dev->vkDev, &dsLayoutCreateInfo, NULL, &dev->dslSrc));
	std::array<VkDescriptorSetLayoutBinding, 3> setLayoutBindings = { };
	setLayoutBindings[0] = dsLayoutBinding;
	dsLayoutBinding.binding = 1;
	setLayoutBindings[1] = dsLayoutBinding;
	dsLayoutBinding.binding = 2;
	dsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	setLayoutBindings[2] = dsLayoutBinding;
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(dev->vkDev, &dsLayoutCreateInfo, NULL, &dev->dslGrad));

	dsLayoutCreateInfo.bindingCount = 3;
	dsLayoutCreateInfo.pBindings = setLayoutBindings.data();
	dsLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(dev->vkDev, &dsLayoutCreateInfo, NULL, &dev->dslPushDset));

	VkPushConstantRange pushConstantRange[] = {
		{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants)},
		//{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(push_constants)}
	};
	VkDescriptorSetLayout dsls[] = { dev->dslFont, dev->dslSrc, dev->dslGrad };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
														   .setLayoutCount = 1,
		//.pSetLayouts = dsls,
		.pSetLayouts = &dev->dslPushDset,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = (VkPushConstantRange*)&pushConstantRange };
	VK_CHECK_RESULT(vkCreatePipelineLayout(dev->vkDev, &pipelineLayoutCreateInfo, NULL, &dev->pipelineLayout));
}

void _device_wait_idle(VkvgDevice dev) { vkDeviceWaitIdle(dev->vkDev); }
void _device_wait_and_reset_device_fence(VkvgDevice dev) {
	vkWaitForFences(dev->vkDev, 1, &dev->fence, VK_TRUE, UINT64_MAX);
	ResetFences(dev->vkDev, 1, &dev->fence);
}

bool _device_try_get_cached_context(VkvgDevice dev, VkvgContext* pCtx) {
	LOCK_DEVICE

		if (dev->cachedContextCount) {
			thrd_t       curThread = thrd_current();
			_cached_ctx* prev = NULL;
			_cached_ctx* cur = dev->cachedContextLast;
			while (cur) {
				if (thrd_equal(cur->thread, curThread)) {
					if (prev)
						prev->pNext = cur->pNext;
					else
						dev->cachedContextLast = cur->pNext;

					dev->cachedContextCount--;

					LOG(VKVG_LOG_THREAD, "get cached context: %p, thd:%lu cached ctx: %d\n", cur->ctx, cur->thread,
						dev->cachedContextCount);

					*pCtx = cur->ctx;
					free(cur);
					UNLOCK_DEVICE
						return true;
				}
				prev = cur;
				cur = cur->pNext;
			}
		}
	*pCtx = NULL;
	UNLOCK_DEVICE
		return false;
}
void _device_store_context(VkvgContext ctx) {
	VkvgDevice dev = ctx->dev;

	LOCK_DEVICE

		_cached_ctx* cur = (_cached_ctx*)calloc(1, sizeof(_cached_ctx));
	cur->ctx = ctx;
	cur->thread = thrd_current();
	cur->pNext = dev->cachedContextLast;

	dev->cachedContextLast = cur;
	dev->cachedContextCount++;

	LOG(VKVG_LOG_THREAD, "store context: %p, thd:%lu cached ctx: %d\n", cur->ctx, cur->thread, dev->cachedContextCount);

	ctx->references++;

	UNLOCK_DEVICE
}
void _device_submit_cmd(VkvgDevice dev, VkCommandBuffer* cmd, VkFence fence) {
	LOCK_DEVICE
		vkh_cmd_submit(dev->gQueue, cmd, fence);
	UNLOCK_DEVICE
}

bool _device_init_function_pointers(VkvgDevice dev) {
#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	if (vkGetInstanceProcAddr(dev->instance, "vkSetDebugUtilsObjectNameEXT") == VK_NULL_HANDLE) {
		LOG(VKVG_LOG_ERR, "vkvg create device failed: 'VK_EXT_debug_utils' has to be loaded for Debug build\n");
		return false;
	}
	vkh_device_init_debug_utils((VkhDevice)&dev->vkDev);
#endif
	CmdBindPipeline = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdBindPipeline);
	CmdBindDescriptorSets = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdBindDescriptorSets);
	CmdBindIndexBuffer = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdBindIndexBuffer);
	CmdBindVertexBuffers = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdBindVertexBuffers);
	CmdDrawIndexed = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdDrawIndexed);
	CmdDraw = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdDraw);
	CmdSetStencilCompareMask = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdSetStencilCompareMask);
	CmdSetStencilReference = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdSetStencilReference);
	CmdSetStencilWriteMask = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdSetStencilWriteMask);
	CmdBeginRenderPass = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdBeginRenderPass);
	CmdEndRenderPass = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdEndRenderPass);
	CmdSetViewport = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdSetViewport);
	CmdSetScissor = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdSetScissor);
	CmdPushConstants = GetVkProcAddress(dev->vkDev, dev->instance, vkCmdPushConstants);
	WaitForFences = GetVkProcAddress(dev->vkDev, dev->instance, vkWaitForFences);
	ResetFences = GetVkProcAddress(dev->vkDev, dev->instance, vkResetFences);
	ResetCommandBuffer = GetVkProcAddress(dev->vkDev, dev->instance, vkResetCommandBuffer);
	return true;
}

void _device_create_empty_texture(VkvgDevice dev, VkFormat format, VkImageTiling tiling) {
	// create empty image to bind to context source descriptor when not in use
	dev->emptyImg = vkh_image_create((VkhDevice)&dev->vkDev, format, 16, 16, tiling, VKH_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	vkh_image_create_descriptor(dev->emptyImg, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_NEAREST,
		VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	_device_wait_and_reset_device_fence(dev);

	vkh_cmd_begin(dev->cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkh_image_set_layout(dev->cmd, dev->emptyImg, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	vkh_cmd_end(dev->cmd);
	_device_submit_cmd(dev, &dev->cmd, dev->fence);
}
void _device_check_best_image_tiling(VkvgDevice dev, VkFormat format) {
	VkFlags            stencilFormats[] = { VK_FORMAT_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT,
										   VK_FORMAT_D32_SFLOAT_S8_UINT };
	VkFormatProperties phyStencilProps = { 0 }, phyImgProps = { 0 };

	// check png blit format
	VkFlags pngBlitFormats[] = { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SRGB };
	dev->pngStagFormat = VK_FORMAT_UNDEFINED;
	for (int i = 0; i < 2; i++) {
		vkGetPhysicalDeviceFormatProperties(dev->phy, (VkFormat)pngBlitFormats[i], &phyImgProps);
		if ((phyImgProps.linearTilingFeatures & VKVG_PNG_WRITE_IMG_REQUIREMENTS) == VKVG_PNG_WRITE_IMG_REQUIREMENTS) {
			dev->pngStagFormat = (VkFormat)pngBlitFormats[i];
			dev->pngStagTiling = VK_IMAGE_TILING_LINEAR;
			break;
		}
		else if ((phyImgProps.optimalTilingFeatures & VKVG_PNG_WRITE_IMG_REQUIREMENTS) ==
			VKVG_PNG_WRITE_IMG_REQUIREMENTS) {
			dev->pngStagFormat = (VkFormat)pngBlitFormats[i];
			dev->pngStagTiling = VK_IMAGE_TILING_OPTIMAL;
			break;
		}
	}

	if (dev->pngStagFormat == VK_FORMAT_UNDEFINED)
		LOG(VKVG_LOG_DEBUG, "vkvg create device failed: no suitable image format for png write\n");

	dev->stencilFormat = VK_FORMAT_UNDEFINED;
	dev->stencilAspectFlag = VK_IMAGE_ASPECT_STENCIL_BIT;
	dev->supportedTiling = (VkImageTiling)0xff;

	vkGetPhysicalDeviceFormatProperties(dev->phy, format, &phyImgProps);

	if ((phyImgProps.optimalTilingFeatures & VKVG_SURFACE_IMGS_REQUIREMENTS) == VKVG_SURFACE_IMGS_REQUIREMENTS) {
		for (int i = 0; i < 4; i++) {
			vkGetPhysicalDeviceFormatProperties(dev->phy, (VkFormat)stencilFormats[i], &phyStencilProps);
			if (phyStencilProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				dev->stencilFormat = (VkFormat)stencilFormats[i];
				if (i > 0)
					dev->stencilAspectFlag |= VK_IMAGE_ASPECT_DEPTH_BIT;
				dev->supportedTiling = VK_IMAGE_TILING_OPTIMAL;
				return;
			}
		}
	}
	if ((phyImgProps.linearTilingFeatures & VKVG_SURFACE_IMGS_REQUIREMENTS) == VKVG_SURFACE_IMGS_REQUIREMENTS) {
		for (int i = 0; i < 4; i++) {
			vkGetPhysicalDeviceFormatProperties(dev->phy, (VkFormat)stencilFormats[i], &phyStencilProps);
			if (phyStencilProps.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				dev->stencilFormat = (VkFormat)stencilFormats[i];
				if (i > 0)
					dev->stencilAspectFlag |= VK_IMAGE_ASPECT_DEPTH_BIT;
				dev->supportedTiling = VK_IMAGE_TILING_LINEAR;
				return;
			}
		}
	}
	dev->status = VKVG_STATUS_INVALID_FORMAT;
	LOG(VKVG_LOG_ERR, "vkvg create device failed: image format not supported: %d\n", format);
}

void _dump_image_format_properties(VkvgDevice dev, VkFormat format) {
	/*VkImageFormatProperties imgProps;
	VK_CHECK_RESULT(vkGetPhysicalDeviceImageFormatProperties(dev->phy,
															 format, VK_IMAGE_TYPE_2D, VKVG_TILING,
															 VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
															 0, &imgProps));
	printf ("tiling			  = %d\n", VKVG_TILING);
	printf ("max extend		  = (%d, %d, %d)\n", imgProps.maxExtent.width, imgProps.maxExtent.height,
	imgProps.maxExtent.depth); printf ("max mip levels	  = %d\n", imgProps.maxMipLevels); printf ("max array layers =
	%d\n", imgProps.maxArrayLayers); printf ("sample counts	  = "); if (imgProps.sampleCounts & VK_SAMPLE_COUNT_1_BIT)
		printf ("1,");
	if (imgProps.sampleCounts & VK_SAMPLE_COUNT_2_BIT)
		printf ("2,");
	if (imgProps.sampleCounts & VK_SAMPLE_COUNT_4_BIT)
		printf ("4,");
	if (imgProps.sampleCounts & VK_SAMPLE_COUNT_8_BIT)
		printf ("8,");
	if (imgProps.sampleCounts & VK_SAMPLE_COUNT_16_BIT)
		printf ("16,");
	if (imgProps.sampleCounts & VK_SAMPLE_COUNT_32_BIT)
		printf ("32,");
	printf ("\n");
	printf ("max resource size= %lu\n", imgProps.maxResourceSize);
*/
}

#endif // 1

#define ISFINITE(x) ((x) * (x) >= 0.) /* check for NaNs */

// matrix computations mainly taken from http://cairographics.org
static void _vkvg_matrix_scalar_multiply(vkvg_matrix_t* matrix, float scalar) {
	matrix->xx *= scalar;
	matrix->yx *= scalar;

	matrix->xy *= scalar;
	matrix->yy *= scalar;

	matrix->x0 *= scalar;
	matrix->y0 *= scalar;
}
void _vkvg_matrix_get_affine(const vkvg_matrix_t* matrix, float* xx, float* yx, float* xy, float* yy, float* x0,
	float* y0) {
	*xx = matrix->xx;
	*yx = matrix->yx;

	*xy = matrix->xy;
	*yy = matrix->yy;

	if (x0)
		*x0 = matrix->x0;
	if (y0)
		*y0 = matrix->y0;
}
static void _vkvg_matrix_compute_adjoint(vkvg_matrix_t* matrix) {
	/* adj (A) = transpose (C:cofactor (A,i,j)) */
	float a, b, c, d, tx, ty;

	_vkvg_matrix_get_affine(matrix, &a, &b, &c, &d, &tx, &ty);

	vkvg_matrix_init(matrix, d, -b, -c, a, c * ty - d * tx, b * tx - a * ty);
}
float _vkvg_matrix_compute_determinant(const vkvg_matrix_t* matrix) {
	float a, b, c, d;

	a = matrix->xx;
	b = matrix->yx;
	c = matrix->xy;
	d = matrix->yy;

	return a * d - b * c;
}
vkvg_status_t vkvg_matrix_invert(vkvg_matrix_t* matrix) {
	float det;

	/* Simple scaling|translation matrices are quite common... */
	if (matrix->xy == 0. && matrix->yx == 0.) {
		matrix->x0 = -matrix->x0;
		matrix->y0 = -matrix->y0;

		if (matrix->xx != 1.f) {
			if (matrix->xx == 0.)
				return VKVG_STATUS_INVALID_MATRIX;

			matrix->xx = 1.f / matrix->xx;
			matrix->x0 *= matrix->xx;
		}

		if (matrix->yy != 1.f) {
			if (matrix->yy == 0.)
				return VKVG_STATUS_INVALID_MATRIX;

			matrix->yy = 1.f / matrix->yy;
			matrix->y0 *= matrix->yy;
		}

		return VKVG_STATUS_SUCCESS;
	}

	/* inv (A) = 1/det (A) * adj (A) */
	det = _vkvg_matrix_compute_determinant(matrix);

	if (!ISFINITE(det))
		return VKVG_STATUS_INVALID_MATRIX;

	if (det == 0)
		return VKVG_STATUS_INVALID_MATRIX;

	_vkvg_matrix_compute_adjoint(matrix);
	_vkvg_matrix_scalar_multiply(matrix, 1 / det);

	return VKVG_STATUS_SUCCESS;
}
void vkvg_matrix_init_identity(vkvg_matrix_t* matrix) { vkvg_matrix_init(matrix, 1, 0, 0, 1, 0, 0); }

void vkvg_matrix_init(vkvg_matrix_t* matrix, float xx, float yx, float xy, float yy, float x0, float y0) {
	matrix->xx = xx;
	matrix->yx = yx;
	matrix->xy = xy;
	matrix->yy = yy;
	matrix->x0 = x0;
	matrix->y0 = y0;
}

void vkvg_matrix_init_translate(vkvg_matrix_t* matrix, float tx, float ty) {
	vkvg_matrix_init(matrix, 1, 0, 0, 1, tx, ty);
}
void vkvg_matrix_init_scale(vkvg_matrix_t* matrix, float sx, float sy) { vkvg_matrix_init(matrix, sx, 0, 0, sy, 0, 0); }
void vkvg_matrix_init_rotate(vkvg_matrix_t* matrix, float radians) {
	float s;
	float c;

	s = sinf(radians);
	c = cosf(radians);

	vkvg_matrix_init(matrix, c, s, -s, c, 0, 0);
}
void vkvg_matrix_translate(vkvg_matrix_t* matrix, float tx, float ty) {
	vkvg_matrix_t tmp;

	vkvg_matrix_init_translate(&tmp, tx, ty);

	vkvg_matrix_multiply(matrix, &tmp, matrix);
}
void vkvg_matrix_scale(vkvg_matrix_t* matrix, float sx, float sy) {
	vkvg_matrix_t tmp;

	vkvg_matrix_init_scale(&tmp, sx, sy);

	vkvg_matrix_multiply(matrix, &tmp, matrix);
}
void vkvg_matrix_rotate(vkvg_matrix_t* matrix, float radians) {
	vkvg_matrix_t tmp;

	vkvg_matrix_init_rotate(&tmp, radians);

	vkvg_matrix_multiply(matrix, &tmp, matrix);
}
void vkvg_matrix_multiply(vkvg_matrix_t* result, const vkvg_matrix_t* a, const vkvg_matrix_t* b) {
	vkvg_matrix_t r;

	r.xx = a->xx * b->xx + a->yx * b->xy;
	r.yx = a->xx * b->yx + a->yx * b->yy;

	r.xy = a->xy * b->xx + a->yy * b->xy;
	r.yy = a->xy * b->yx + a->yy * b->yy;

	r.x0 = a->x0 * b->xx + a->y0 * b->xy + b->x0;
	r.y0 = a->x0 * b->yx + a->y0 * b->yy + b->y0;

	*result = r;
}
void vkvg_matrix_transform_distance(const vkvg_matrix_t* matrix, float* dx, float* dy) {
	float new_x, new_y;

	new_x = (matrix->xx * *dx + matrix->xy * *dy);
	new_y = (matrix->yx * *dx + matrix->yy * *dy);

	*dx = new_x;
	*dy = new_y;
}
void vkvg_matrix_transform_point(const vkvg_matrix_t* matrix, float* x, float* y) {
	vkvg_matrix_transform_distance(matrix, x, y);

	*x += matrix->x0;
	*y += matrix->y0;
}
void vkvg_matrix_get_scale(const vkvg_matrix_t* matrix, float* sx, float* sy) {
	*sx = sqrt(matrix->xx * matrix->xx + matrix->xy * matrix->xy);
	/*if (matrix->xx < 0)
	 *sx = -*sx;*/
	*sy = sqrt(matrix->yx * matrix->yx + matrix->yy * matrix->yy);
	/*if (matrix->yy < 0)
	 *sy = -*sy;*/
}
// pattern
#if 1


VkvgPattern vkvg_pattern_create_for_surface(VkvgSurface surf) {
	if (!surf) {
		LOG(VKVG_LOG_ERR, "CREATE Pattern failed, invalid surface\n");
		return (VkvgPattern)&_vkvg_status_null_pointer;
	}
	VkvgPattern pat = (vkvg_pattern_t*)calloc(1, sizeof(vkvg_pattern_t));
	if (!pat) {
		LOG(VKVG_LOG_ERR, "CREATE Pattern failed, no memory\n");
		return (VkvgPattern)&_vkvg_status_null_pointer;
	}

	pat->type = VKVG_PATTERN_TYPE_SURFACE;
	pat->extend = VKVG_EXTEND_NONE;
	pat->data = surf;
	pat->references = 1;

	vkvg_surface_reference(surf);
	if (vkvg_surface_status(surf))
		pat->status = VKVG_STATUS_INVALID_SURFACE;

	return pat;
}
vkvg_status_t vkvg_pattern_get_linear_points(VkvgPattern pat, float* x0, float* y0, float* x1, float* y1) {
	if (vkvg_pattern_status(pat))
		return vkvg_pattern_status(pat);
	if (pat->type != VKVG_PATTERN_TYPE_LINEAR)
		return VKVG_STATUS_PATTERN_TYPE_MISMATCH;

	vkvg_gradient_t* grad = (vkvg_gradient_t*)pat->data;

	*x0 = grad->cp[0].x;
	*y0 = grad->cp[0].y;
	*x1 = grad->cp[0].z;
	*y1 = grad->cp[0].w;
	return VKVG_STATUS_SUCCESS;
}
vkvg_status_t vkvg_pattern_edit_linear(VkvgPattern pat, float x0, float y0, float x1, float y1) {
	if (vkvg_pattern_status(pat))
		return vkvg_pattern_status(pat);
	if (pat->type != VKVG_PATTERN_TYPE_LINEAR)
		return VKVG_STATUS_PATTERN_TYPE_MISMATCH;
	vkvg_gradient_t* grad = (vkvg_gradient_t*)pat->data;
	*grad = {};
	grad->cp[0] = vec4{ {x0}, {y0}, {x1}, {y1} };
	grad->m = ivec4(1024, 0, 0, 1024);
	grad->extend = pat->extend;
	grad->scale = vec2{ 1.0,1.0 };
	return VKVG_STATUS_SUCCESS;
}
VkvgPattern vkvg_pattern_create_linear(float x0, float y0, float x1, float y1) {
	VkvgPattern pat = (vkvg_pattern_t*)calloc(1, sizeof(vkvg_pattern_t));
	if (!pat) {
		LOG(VKVG_LOG_ERR, "CREATE Pattern failed, no memory\n");
		return (VkvgPattern)&_vkvg_status_null_pointer;
	}
	pat->type = VKVG_PATTERN_TYPE_LINEAR;
	pat->extend = VKVG_EXTEND_NONE;
	pat->data = (void*)calloc(1, sizeof(vkvg_gradient_t));
	if (pat->data) {
		vkvg_pattern_edit_linear(pat, x0, y0, x1, y1);
		pat->references = 1;
	}
	else {
		pat->status = VKVG_STATUS_PATTERN_INVALID_GRADIENT;
	}
	return pat;
}
vkvg_status_t vkvg_pattern_edit_radial(VkvgPattern pat, float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse) {
	if (vkvg_pattern_status(pat))
		return vkvg_pattern_status(pat);
	if (pat->type != VKVG_PATTERN_TYPE_RADIAL)
		return VKVG_STATUS_PATTERN_TYPE_MISMATCH;
	vkvg_gradient_t* grad = (vkvg_gradient_t*)pat->data;
	*grad = {};
	vec2 c0 = { cx0, cy0 };
	vec2 c1 = { cx1, cy1 };
	if (radius0 > radius1 - 1.0f)
		radius0 = radius1 - 1.0f;
	vec2  u = vec2_sub(c0, c1);
	float l = vec2_length(u);
	if (l + radius0 + 1.0f >= radius1) {
		vec2 v = vec2_div_s(u, l);
		c0 = vec2_add(c1, vec2_mult_s(v, radius1 - radius0 - 1.0f));
	}
	grad->cp[0] = vec4{ {c0.x}, {c0.y}, {radius0}, {0} };
	grad->cp[1] = vec4{ {c1.x}, {c1.y}, {radius1}, {0} };
	grad->m = ivec4(1024, 0, 0, 1024);
	grad->extend = pat->extend;
	grad->scale = vec2{ 1.0,1.0 };
	if (is_ellipse)grad->scale.x *= 2;
	return VKVG_STATUS_SUCCESS;
}

// circle 或 ellipse
VkvgPattern vkvg_pattern_create_radial(float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse) {
	VkvgPattern pat = (vkvg_pattern_t*)calloc(1, sizeof(vkvg_pattern_t));
	if (!pat) {
		LOG(VKVG_LOG_ERR, "CREATE Pattern failed, no memory\n");
		return (VkvgPattern)&_vkvg_status_null_pointer;
	}
	pat->type = VKVG_PATTERN_TYPE_RADIAL;
	pat->extend = VKVG_EXTEND_NONE;
	pat->data = (void*)calloc(1, sizeof(vkvg_gradient_t));
	if (pat->data) {
		vkvg_pattern_edit_radial(pat, cx0, cy0, radius0, cx1, cy1, radius1, is_ellipse);
		pat->references = 1;
	}
	else
		pat->status = VKVG_STATUS_NO_MEMORY;
	return pat;
}

VkvgPattern vkvg_pattern_create_radial(float cx0, float cy0, float radius0, float cx1, float cy1, float radius1)
{
	return vkvg_pattern_create_radial(cx0, cy0, radius0, cx1, cy1, radius1, false);
}

vkvg_status_t vkvg_pattern_edit_sweep(VkvgPattern pat, float cx, float cy, float start_angle, float end_angle) {
	if (vkvg_pattern_status(pat))
		return vkvg_pattern_status(pat);
	if (pat->type != VKVG_PATTERN_TYPE_SWEEP)
		return VKVG_STATUS_PATTERN_TYPE_MISMATCH;
	vkvg_gradient_t* grad = (vkvg_gradient_t*)pat->data;
	*grad = {};
	grad->cp[0] = vec4{ cx, cy, start_angle, end_angle };
	grad->m = ivec4(1024, 0, 0, 1024);
	grad->extend = pat->extend;
	grad->scale = vec2{ 1.0,1.0 };
	return VKVG_STATUS_SUCCESS;
}
VkvgPattern vkvg_pattern_create_sweep(float cx, float cy, float start_angle, float end_angle) {
	VkvgPattern pat = (vkvg_pattern_t*)calloc(1, sizeof(vkvg_pattern_t));
	if (!pat) {
		LOG(VKVG_LOG_ERR, "CREATE Pattern failed, no memory\n");
		return (VkvgPattern)&_vkvg_status_null_pointer;
	}
	pat->type = VKVG_PATTERN_TYPE_SWEEP;
	pat->extend = VKVG_EXTEND_NONE;
	pat->data = (void*)calloc(1, sizeof(vkvg_gradient_t));
	if (pat->data) {
		vkvg_pattern_edit_sweep(pat, cx, cy, start_angle, end_angle);
		pat->references = 1;
	}
	else
		pat->status = VKVG_STATUS_NO_MEMORY;
	return pat;
}
VkvgPattern vkvg_pattern_reference(VkvgPattern pat) {
	if (!vkvg_pattern_status(pat))
		pat->references++;
	return pat;
}
uint32_t vkvg_pattern_get_reference_count(VkvgPattern pat) {
	if (vkvg_pattern_status(pat))
		return 0;
	return pat->references;
}

vkvg_status_t vkvg_pattern_set_scale(VkvgPattern pat, float scale_x, float scale_y) {
	if (vkvg_pattern_status(pat))
		return vkvg_pattern_status(pat);
	if (pat->type == VKVG_PATTERN_TYPE_SURFACE || pat->type == VKVG_PATTERN_TYPE_SOLID)
		return VKVG_STATUS_PATTERN_TYPE_MISMATCH;
	vkvg_gradient_t* grad = (vkvg_gradient_t*)pat->data;
	grad->scale = vec2{ scale_x ,scale_y };
	return VKVG_STATUS_SUCCESS;
}

vkvg_status_t vkvg_pattern_set_color_stop(VkvgPattern pat, int idx, float r, float g, float b, float a) {
	if (vkvg_pattern_status(pat))
		return vkvg_pattern_status(pat);
	if (pat->type == VKVG_PATTERN_TYPE_SURFACE || pat->type == VKVG_PATTERN_TYPE_SOLID)
		return VKVG_STATUS_PATTERN_TYPE_MISMATCH;
	vkvg_gradient_t* grad = (vkvg_gradient_t*)pat->data;
	if (idx < 0 || idx >= grad->count)return VKVG_STATUS_PATTERN_INVALID_GRADIENT;
	//#ifdef VKVG_PREMULT_ALPHA
	//	vkvg_color_t c = { a * r, a * g, a * b, a };
	//#else
	//#endif
	vkvg_color_t c = { r, g, b, a };
	grad->colors[idx] = c;
	return VKVG_STATUS_SUCCESS;
}

ivec4 rota2x2(int r) {
	// 角度转弧度 
	float rad = r * 0.01745329251994329576923690768489;
	// 计算余弦和正弦 
	float c = cos(rad);
	float s = sin(rad);
	// 构造2x2旋转矩阵（列主序），各元素乘1024并取整 
	int a = static_cast<int>(round(c * 1024.0f));
	int b = static_cast<int>(round(-s * 1024.0f));
	int c1 = static_cast<int>(round(s * 1024.0f));
	int d = static_cast<int>(round(c * 1024.0f));
	// 列主序布局：mat = [ a, c, b, d ]
	// 转置后：逆矩阵 = [ a, b, c, d ] → 交换第 1 和第 2 个分量的位置 
	return ivec4(a, b, c1, d);
}
vkvg_status_t vkvg_pattern_set_rotate(VkvgPattern pat, int angle) {
	if (vkvg_pattern_status(pat))
		return vkvg_pattern_status(pat);
	if (pat->type == VKVG_PATTERN_TYPE_SURFACE || pat->type == VKVG_PATTERN_TYPE_SOLID)
		return VKVG_STATUS_PATTERN_TYPE_MISMATCH;
	vkvg_gradient_t* grad = (vkvg_gradient_t*)pat->data;
	if (!grad)return VKVG_STATUS_PATTERN_INVALID_GRADIENT;
	grad->m = rota2x2(angle);
	return VKVG_STATUS_SUCCESS;
}
vkvg_status_t vkvg_set_glutess(VkvgContext ctx, bool enable)
{
	if (ctx)
		ctx->fill_rule_winding = enable;
	return ctx ? VKVG_STATUS_SUCCESS : VKVG_STATUS_NULL_POINTER;
}
vkvg_status_t vkvg_pattern_set_color_stop_offset(VkvgPattern pat, int idx, float offset) {
	if (vkvg_pattern_status(pat))
		return vkvg_pattern_status(pat);
	if (pat->type == VKVG_PATTERN_TYPE_SURFACE || pat->type == VKVG_PATTERN_TYPE_SOLID)
		return VKVG_STATUS_PATTERN_TYPE_MISMATCH;
	vkvg_gradient_t* grad = (vkvg_gradient_t*)pat->data;
	if (idx < 0 || idx >= grad->count)return VKVG_STATUS_PATTERN_INVALID_GRADIENT;
	grad->stops[idx] = offset;
	return VKVG_STATUS_SUCCESS;
}
vkvg_status_t vkvg_pattern_add_color_stop(VkvgPattern pat, float offset, float r, float g, float b, float a) {
	if (vkvg_pattern_status(pat))
		return vkvg_pattern_status(pat);
	if (pat->type == VKVG_PATTERN_TYPE_SURFACE || pat->type == VKVG_PATTERN_TYPE_SOLID)
		return VKVG_STATUS_PATTERN_TYPE_MISMATCH;

	vkvg_gradient_t* grad = (vkvg_gradient_t*)pat->data;
	if (grad->count < MAX_STOPS)
	{
		vkvg_color_t c = { r, g, b, a };
		grad->colors[grad->count] = c;
#ifdef VKVG_ENABLE_VK_SCALAR_BLOCK_LAYOUT
		grad->stops[grad->count] = offset;
#else
		grad->stops[grad->count].r = offset;
#endif
		grad->count++;
	}
	return VKVG_STATUS_SUCCESS;
}
void vkvg_pattern_set_extend(VkvgPattern pat, vkvg_extend_t extend) {
	if (vkvg_pattern_status(pat))
		return;
	pat->extend = extend;
}
void vkvg_pattern_set_filter(VkvgPattern pat, vkvg_filter_t filter) {
	if (vkvg_pattern_status(pat))
		return;
	pat->filter = filter;
}
vkvg_extend_t vkvg_pattern_get_extend(VkvgPattern pat) {
	if (vkvg_pattern_status(pat))
		return (vkvg_extend_t)0;
	return pat->extend;
}
vkvg_filter_t vkvg_pattern_get_filter(VkvgPattern pat) {
	if (vkvg_pattern_status(pat))
		return (vkvg_filter_t)0;
	return pat->filter;
}
vkvg_pattern_type_t vkvg_pattern_get_type(VkvgPattern pat) {
	if (vkvg_pattern_status(pat))
		return (vkvg_pattern_type_t)0;
	return pat->type;
}
vkvg_status_t vkvg_pattern_get_color_stop_count(VkvgPattern pat, uint32_t* count) {
	if (vkvg_pattern_status(pat))
		return vkvg_pattern_status(pat);
	if (pat->type == VKVG_PATTERN_TYPE_SURFACE || pat->type == VKVG_PATTERN_TYPE_SOLID)
		return VKVG_STATUS_PATTERN_TYPE_MISMATCH;
	vkvg_gradient_t* grad = (vkvg_gradient_t*)pat->data;
	*count = grad->count;
	return VKVG_STATUS_SUCCESS;
}
vkvg_status_t vkvg_pattern_get_color_stop_rgba(VkvgPattern pat, uint32_t index, float* offset, float* r, float* g,
	float* b, float* a) {
	if (vkvg_pattern_status(pat))
		return vkvg_pattern_status(pat);
	if (pat->type == VKVG_PATTERN_TYPE_SURFACE || pat->type == VKVG_PATTERN_TYPE_SOLID)
		return VKVG_STATUS_PATTERN_TYPE_MISMATCH;
	vkvg_gradient_t* grad = (vkvg_gradient_t*)pat->data;
	if (index >= grad->count)
		return VKVG_STATUS_INVALID_INDEX;
#ifdef VKVG_ENABLE_VK_SCALAR_BLOCK_LAYOUT
	* offset = grad->stops[index];
#else
	* offset = grad->stops[index].r;
#endif
	vkvg_color_t c = grad->colors[index];
	*r = c.r;
	*g = c.g;
	*b = c.b;
	*a = c.a;
	return VKVG_STATUS_SUCCESS;
}
void vkvg_pattern_set_matrix(VkvgPattern pat, const vkvg_matrix_t* matrix) {
	if (vkvg_pattern_status(pat))
		return;
	pat->matrix = *matrix;
	pat->hasMatrix = true;
}
void vkvg_pattern_get_matrix(VkvgPattern pat, vkvg_matrix_t* matrix) {
	if (vkvg_pattern_status(pat))
		return;
	if (pat->hasMatrix)
		*matrix = pat->matrix;
	else
		*matrix = VKVG_IDENTITY_MATRIX;
}
vkvg_status_t vkvg_pattern_status(VkvgPattern pat) { return !pat ? VKVG_STATUS_NULL_POINTER : pat->status; }

void vkvg_pattern_destroy(VkvgPattern pat) {
	if (vkvg_pattern_status(pat))
		return;
	pat->references--;
	if (pat->references > 0)
		return;

	if (pat->type == VKVG_PATTERN_TYPE_SURFACE) {
		VkvgSurface surf = (VkvgSurface)pat->data;
		vkvg_surface_destroy(surf);
	}
	else
		free(pat->data);

	free(pat);
	pat = NULL;
}

#endif // 1
// surface
#if 1


void vkvg_surface_clear(VkvgSurface surf) {
	if (vkvg_surface_status(surf))
		return;
	_clear_surface(surf, VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_COLOR_BIT);
}

VkvgSurface vkvg_surface_create(VkvgDevice dev, uint32_t width, uint32_t height) {
	VkvgSurface surf = _create_surface(dev, FB_COLOR_FORMAT);
	if (surf->status)
		return surf;

	surf->width = MAX(1, width);
	surf->height = MAX(1, height);
	surf->newSurf = true; // used to clear all attacments on first render pass

	_create_surface_images(surf);
	_transition_surf_images(surf);

	surf->status = VKVG_STATUS_SUCCESS;
	return surf;
}
VkvgSurface vkvg_surface_create_for_VkhImage(VkvgDevice dev, void* vkhImg) {
	VkvgSurface surf = _create_surface(dev, FB_COLOR_FORMAT);
	if (surf->status)
		return surf;

	if (!vkhImg) {
		surf->status = VKVG_STATUS_INVALID_IMAGE;
		return surf;
	}

	VkhImage img = (VkhImage)vkhImg;
	surf->width = img->infos.extent.width;
	surf->height = img->infos.extent.height;

	surf->img = img;

	vkh_image_create_sampler(img, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	_create_surface_secondary_images(surf);
	_create_framebuffer(surf);

	_transition_surf_images(surf);
	//_clear_surface						(surf, VK_IMAGE_ASPECT_STENCIL_BIT);

	surf->status = VKVG_STATUS_SUCCESS;
	return surf;
}
// TODO: it would be better to blit in original size and create ms final image with dest surf dims
VkvgSurface vkvg_surface_create_from_bitmap(VkvgDevice dev, unsigned char* img, uint32_t width, uint32_t height) {
	VkvgSurface surf = _create_surface(dev, FB_COLOR_FORMAT);
	if (surf->status)
		return surf;
	if (!img || width <= 0 || height <= 0) {
		surf->status = VKVG_STATUS_INVALID_IMAGE;
		return surf;
	}

	surf->width = MAX(1, width);
	surf->height = MAX(1, height);

	_create_surface_images(surf);

	uint32_t                 imgSize = width * height * 4;
	VkImageSubresourceLayers imgSubResLayers = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	// original format image
	VkhImage stagImg = vkh_image_create((VkhDevice)&surf->dev->vkDev, VK_FORMAT_R8G8B8A8_UNORM, surf->width,
		surf->height, VK_IMAGE_TILING_LINEAR, VKH_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	// bgra bliting target
	VkhImage tmpImg =
		vkh_image_create((VkhDevice)&surf->dev->vkDev, surf->format, surf->width, surf->height, VK_IMAGE_TILING_LINEAR,
			VKH_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	vkh_image_create_descriptor(tmpImg, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_NEAREST,
		VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	// staging buffer
	vkh_buffer_t buff = { 0 };
	vkh_buffer_init((VkhDevice)&dev->vkDev, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VKH_MEMORY_USAGE_CPU_TO_GPU, imgSize,
		&buff, true);

	memcpy(vkh_buffer_get_mapped_pointer(&buff), img, imgSize);
	vkh_buffer_flush(&buff);
	VkCommandBuffer cmd = surf->cmd;

	vkh_cmd_begin(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkh_image_set_layout(cmd, stagImg, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);

	VkBufferImageCopy bufferCopyRegion = { .imageSubresource = imgSubResLayers,
										  .imageExtent = {surf->width, surf->height, 1} };

	vkCmdCopyBufferToImage(cmd, buff.buffer, vkh_image_get_vkimage(stagImg), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
		&bufferCopyRegion);

	vkh_image_set_layout(cmd, stagImg, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);
	vkh_image_set_layout(cmd, tmpImg, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);

	VkImageBlit blit = {
		.srcSubresource = imgSubResLayers,
		.dstSubresource = imgSubResLayers,
	};
	blit.srcOffsets[1] = { (int32_t)surf->width, (int32_t)surf->height, 1 };
	blit.dstOffsets[1] = { (int32_t)surf->width, (int32_t)surf->height, 1 };

	vkCmdBlitImage(cmd, vkh_image_get_vkimage(stagImg), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		vkh_image_get_vkimage(tmpImg), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

	vkh_image_set_layout(cmd, tmpImg, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	vkh_cmd_end(cmd);

	_surface_submit_cmd(surf); // lock surface?

	vkh_buffer_reset(&buff);
	vkh_image_destroy(stagImg);

	surf->newSurf = true;

	// create tmp context with rendering pipeline to create the multisample img
	VkvgContext ctx = vkvg_create(surf);

	/*	  VkClearAttachment ca = {VK_IMAGE_ASPECT_COLOR_BIT,0, { 0.0f, 0.0f, 0.0f, 0.0f }};
		VkClearRect cr = {{{0,0},{surf->width,surf->height}},0,1};
		vkCmdClearAttachments(ctx->cmd, 1, &ca, 1, &cr);*/

	vec4 srcRect = { .x = 0, .y = 0, .width = (float)surf->width, .height = (float)surf->height };
	ctx->pushConsts.source = srcRect;
	ctx->pushConsts.fsq_patternType = (ctx->pushConsts.fsq_patternType & FULLSCREEN_BIT) + VKVG_PATTERN_TYPE_SURFACE;

	//_update_push_constants (ctx);
	//_update_descriptor_set(ctx, tmpImg, ctx->dsSrc); 
	_ensure_renderpass_is_started(ctx);
	push_update_descriptor_set_a(ctx, tmpImg, 0);

	vkvg_paint(ctx);
	vkvg_destroy(ctx);
	vkvg_surface_resolve(surf);

	vkh_image_destroy(tmpImg);

	surf->status = VKVG_STATUS_SUCCESS;
	return surf;
}
VkvgSurface vkvg_surface_create_from_image(VkvgDevice dev, const char* filePath) {
	int            w = 0, h = 0, channels = 0;
	unsigned char* img = stbi_load(filePath, &w, &h, &channels, 4); // force 4 components per pixel
	if (!img) {
		LOG(VKVG_LOG_ERR, "Could not load texture from %s, %s\n", filePath, stbi_failure_reason());
		return (VkvgSurface)&_vkvg_status_null_pointer;
	}

	VkvgSurface surf = vkvg_surface_create_from_bitmap(dev, img, (uint32_t)w, (uint32_t)h);

	stbi_image_free(img);

	return surf;
}

void vkvg_surface_destroy(VkvgSurface surf) {
	if (vkvg_surface_status(surf)) {
		LOG(VKVG_LOG_ERR, "DESTROY surface failed, invalid surface\n");
		return;
	}
	if (vkvg_device_status(surf->dev)) {
		LOG(VKVG_LOG_ERR, "DESTROY surface failed, device error\n");
		return;
	}

	LOCK_SURFACE(surf)
		surf->references--;
	if (surf->references > 0) {
		UNLOCK_SURFACE(surf)
			return;
	}
	UNLOCK_SURFACE(surf)

		LOG(VKVG_LOG_INFO, "DESTROY Surface\n");

	vkDestroyCommandPool(surf->dev->vkDev, surf->cmdPool, NULL);
	vkDestroyFramebuffer(surf->dev->vkDev, surf->fb, NULL);

	if (!surf->img->imported)
		vkh_image_destroy(surf->img);

	vkh_image_destroy(surf->imgMS);
	vkh_image_destroy(surf->stencil);

	if (surf->dev->threadAware)
		mtx_destroy(&surf->mutex);

#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	vkDestroySemaphore(surf->dev->vkDev, surf->timeline, NULL);
#else
	vkDestroyFence(surf->dev->vkDev, surf->flushFence, NULL);
#endif
	if (surf->sem)
		vkDestroySemaphore(surf->dev->vkDev, surf->sem, NULL);
	vkvg_device_destroy(surf->dev);
	free(surf);
	surf = NULL;
}

vkvg_status_t vkvg_surface_status(VkvgSurface surf) { return !surf ? VKVG_STATUS_NULL_POINTER : surf->status; }

VkvgSurface vkvg_surface_reference(VkvgSurface surf) {
	if (!vkvg_surface_status(surf)) {
		LOCK_SURFACE(surf)
			surf->references++;
		UNLOCK_SURFACE(surf)
	}
	return surf;
}
uint32_t vkvg_surface_get_reference_count(VkvgSurface surf) {
	if (vkvg_surface_status(surf))
		return 0;
	return surf->references;
}

VkImage vkvg_surface_get_vk_image(VkvgSurface surf) {
	if (vkvg_surface_status(surf))
		return NULL;
	if (surf->dev->deferredResolve)
		_explicit_ms_resolve(surf);
	return vkh_image_get_vkimage(surf->img);
}
void vkvg_surface_resolve(VkvgSurface surf) {
	if (vkvg_surface_status(surf) || !surf->dev->deferredResolve)
		return;
	_explicit_ms_resolve(surf);
}
VkFormat vkvg_surface_get_vk_format(VkvgSurface surf) {
	if (vkvg_surface_status(surf))
		return VK_FORMAT_UNDEFINED;
	return surf->format;
}
uint32_t vkvg_surface_get_width(VkvgSurface surf) {
	if (vkvg_surface_status(surf))
		return 0;
	return surf->width;
}
uint32_t vkvg_surface_get_height(VkvgSurface surf) {
	if (vkvg_surface_status(surf))
		return 0;
	return surf->height;
}

vkvg_status_t vkvg_surface_write_to_png(VkvgSurface surf, const char* path) {
	if (vkvg_surface_status(surf)) {
		LOG(VKVG_LOG_ERR, "vkvg_surface_write_to_png failed, invalid status: %d\n", vkvg_surface_status(surf));
		return VKVG_STATUS_INVALID_STATUS;
	}
	if (vkvg_device_status(surf->dev)) {
		LOG(VKVG_LOG_ERR, "vkvg_surface_write_to_png failed, invalid device status: %d\n", vkvg_device_status(surf->dev));
		return VKVG_STATUS_INVALID_STATUS;
	}
	if (surf->dev->pngStagFormat == VK_FORMAT_UNDEFINED) {
		LOG(VKVG_LOG_ERR, "no suitable image format for png write\n");
		return VKVG_STATUS_INVALID_FORMAT;
	}
	if (!path) {
		LOG(VKVG_LOG_ERR, "vkvg_surface_write_to_png failed, null path\n");
		return VKVG_STATUS_WRITE_ERROR;
	}
	LOCK_SURFACE(surf)
		VkImageSubresourceLayers imgSubResLayers = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	VkvgDevice               dev = surf->dev;

	// RGBA to blit to, surf img is bgra
	VkhImage stagImg;

	if (dev->pngStagTiling == VK_IMAGE_TILING_LINEAR)
		stagImg = vkh_image_create((VkhDevice)&surf->dev->vkDev, dev->pngStagFormat, surf->width, surf->height,
			dev->pngStagTiling, VKH_MEMORY_USAGE_GPU_TO_CPU,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	else
		stagImg = vkh_image_create((VkhDevice)&surf->dev->vkDev, dev->pngStagFormat, surf->width, surf->height,
			dev->pngStagTiling, VKH_MEMORY_USAGE_GPU_ONLY,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	VkCommandBuffer cmd = surf->cmd;
	vkh_cmd_begin(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkh_image_set_layout(cmd, stagImg, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	vkh_image_set_layout(cmd, surf->img, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

	VkImageBlit blit = {
		.srcSubresource = imgSubResLayers,
		.dstSubresource = imgSubResLayers,
	};
	blit.srcOffsets[1] = { (int32_t)surf->width, (int32_t)surf->height, 1 };
	blit.dstOffsets[1] = { (int32_t)surf->width, (int32_t)surf->height, 1 };
	vkCmdBlitImage(cmd, vkh_image_get_vkimage(surf->img), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		vkh_image_get_vkimage(stagImg), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);

	vkh_cmd_end(cmd);
	_surface_submit_cmd(surf);

	VkhImage stagImgLinear = stagImg;

	if (dev->pngStagTiling == VK_IMAGE_TILING_OPTIMAL) {
		stagImgLinear = vkh_image_create((VkhDevice)&surf->dev->vkDev, dev->pngStagFormat, surf->width, surf->height,
			VK_IMAGE_TILING_LINEAR, VKH_MEMORY_USAGE_GPU_TO_CPU,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		VkImageCopy cpy = { .srcSubresource = imgSubResLayers,
						   .srcOffset = {0},
						   .dstSubresource = imgSubResLayers,
						   .dstOffset = {0},
						   .extent = {(uint32_t)surf->width, (uint32_t)surf->height, 1} };

		vkh_cmd_begin(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		vkh_image_set_layout(cmd, stagImgLinear, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		vkh_image_set_layout(cmd, stagImg, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		vkCmdCopyImage(cmd, vkh_image_get_vkimage(stagImg), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			vkh_image_get_vkimage(stagImgLinear), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cpy);

		vkh_cmd_end(cmd);

		_surface_submit_cmd(surf);

		vkh_image_destroy(stagImg);
	}

	unsigned char* img = (unsigned char*)vkh_image_map(stagImgLinear);

	uint64_t stride = vkh_image_get_stride(stagImgLinear);

#ifdef VKVG_PREMULT_ALPHA
	//unpremult alpha for saving on disk.
	for (int y = 0; y < surf->height; y++) {
		for (int x = 0; x < surf->width; x++) {
			unsigned char* p = img + y * stride + x * 4;
			double alpha = (double)p[3] / 255.f;
			p[0] = (unsigned char)((double)p[0] / alpha);
			p[1] = (unsigned char)((double)p[1] / alpha);
			p[2] = (unsigned char)((double)p[2] / alpha);
		}
	}
#endif

	stbi_write_png(path, (int32_t)surf->width, (int32_t)surf->height, 4, img, (int32_t)stride);

	vkh_image_unmap(stagImgLinear);
	vkh_image_destroy(stagImgLinear);

	UNLOCK_SURFACE(surf)
		return VKVG_STATUS_SUCCESS;
}

vkvg_status_t vkvg_surface_write_to_memory(VkvgSurface surf, unsigned char* const bitmap) {
	if (vkvg_surface_status(surf)) {
		LOG(VKVG_LOG_ERR, "vkvg_surface_write_to_memory failed, invalid status: %d\n", vkvg_surface_status(surf));
		return VKVG_STATUS_INVALID_STATUS;
	}
	if (!bitmap) {
		LOG(VKVG_LOG_ERR, "vkvg_surface_write_to_memory failed, null path\n");
		return VKVG_STATUS_WRITE_ERROR;
	}

	LOCK_SURFACE(surf)

		VkImageSubresourceLayers imgSubResLayers = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	VkvgDevice               dev = surf->dev;

	// RGBA to blit to, surf img is bgra
	VkhImage stagImg = vkh_image_create((VkhDevice)&surf->dev->vkDev, VK_FORMAT_R8G8B8A8_UNORM, surf->width,
		surf->height, VK_IMAGE_TILING_LINEAR, VKH_MEMORY_USAGE_GPU_TO_CPU,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	VkCommandBuffer cmd = surf->cmd;

	vkh_cmd_begin(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkh_image_set_layout(cmd, stagImg, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);
	vkh_image_set_layout(cmd, surf->img, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

	VkImageBlit blit = {
		.srcSubresource = imgSubResLayers,
		.dstSubresource = imgSubResLayers,
	};
	blit.srcOffsets[1] = { (int32_t)surf->width, (int32_t)surf->height, 1 };
	blit.dstOffsets[1] = { (int32_t)surf->width, (int32_t)surf->height, 1 };
	vkCmdBlitImage(cmd, vkh_image_get_vkimage(surf->img), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		vkh_image_get_vkimage(stagImg), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);

	vkh_cmd_end(cmd);

	_surface_submit_cmd(surf);

	uint64_t stride = vkh_image_get_stride(stagImg);
	uint32_t dest_stride = surf->width * 4;

	unsigned char* img = (unsigned char*)vkh_image_map(stagImg);

#ifdef VKVG_PREMULT_ALPHA
	//unpremult alpha for saving on disk.
	for (int y = 0; y < surf->height; y++) {
		for (int x = 0; x < surf->width; x++) {
			unsigned char* p = img + y * stride + x * 4;
			double alpha = (double)p[3] / 255.f;
			p[0] = (unsigned char)((double)p[0] / alpha);
			p[1] = (unsigned char)((double)p[1] / alpha);
			p[2] = (unsigned char)((double)p[2] / alpha);
		}
	}
#endif

	unsigned char* row = (unsigned char*)bitmap;
	for (uint32_t y = 0; y < surf->height; y++) {
		memcpy(row, img, dest_stride);
		row += dest_stride;
		img += stride;
	}

	vkh_image_unmap(stagImg);
	vkh_image_destroy(stagImg);

	UNLOCK_SURFACE(surf)

		return VKVG_STATUS_SUCCESS;
}

void _explicit_ms_resolve(VkvgSurface surf) {
	LOCK_SURFACE(surf)

		VkCommandBuffer cmd = surf->cmd;

	vkh_cmd_begin(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkh_image_set_layout(cmd, surf->imgMS, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);
	vkh_image_set_layout(cmd, surf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);

	VkImageResolve re = {
						 .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
						 .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
						 .extent = {surf->width, surf->height, 1} };

	vkCmdResolveImage(cmd, vkh_image_get_vkimage(surf->imgMS), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		vkh_image_get_vkimage(surf->img), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &re);
	vkh_image_set_layout(cmd, surf->imgMS, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	vkh_image_set_layout(cmd, surf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	vkh_cmd_end(cmd);

	_surface_submit_cmd(surf);

	UNLOCK_SURFACE(surf)
}
void _transition_surf_images(VkvgSurface surf) {
	LOCK_SURFACE(surf)
		VkvgDevice dev = surf->dev;

	//_surface_wait_cmd (surf);

	vkh_cmd_begin(surf->cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VkhImage imgMs = surf->imgMS;
	if (imgMs != NULL)
		vkh_image_set_layout(surf->cmd, imgMs, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	vkh_image_set_layout(surf->cmd, surf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	vkh_image_set_layout(surf->cmd, surf->stencil, dev->stencilAspectFlag, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
	vkh_cmd_end(surf->cmd);

	_surface_submit_cmd(surf);

	UNLOCK_SURFACE(surf)
}
void _clear_surface(VkvgSurface surf, VkImageAspectFlags aspect) {
	LOCK_SURFACE(surf)

		VkCommandBuffer cmd = surf->cmd;

	vkh_cmd_begin(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	if (aspect & VK_IMAGE_ASPECT_COLOR_BIT) {
		VkClearColorValue       cclr = { {0, 0, 0, 0} };
		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		VkhImage img = surf->imgMS;
		if (surf->dev->samples == VK_SAMPLE_COUNT_1_BIT)
			img = surf->img;

		vkh_image_set_layout(cmd, img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);

		vkCmdClearColorImage(cmd, vkh_image_get_vkimage(img), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &cclr, 1, &range);

		vkh_image_set_layout(cmd, img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}
	if (aspect & VK_IMAGE_ASPECT_STENCIL_BIT) {
		VkClearDepthStencilValue clr = { 0, 0 };
		VkImageSubresourceRange  range = { VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };

		vkh_image_set_layout(cmd, surf->stencil, VK_IMAGE_ASPECT_STENCIL_BIT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		vkCmdClearDepthStencilImage(cmd, vkh_image_get_vkimage(surf->stencil), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			&clr, 1, &range);

		vkh_image_set_layout(cmd, surf->stencil, VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
	}
	vkh_cmd_end(cmd);

	_surface_submit_cmd(surf);

	UNLOCK_SURFACE(surf)
}

void _create_surface_main_image(VkvgSurface surf) {
	VkhDevice vkhd = (VkhDevice)&surf->dev->vkDev;
	surf->img = vkh_image_create(vkhd, surf->format, surf->width, surf->height, surf->dev->supportedTiling,
		VKH_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	vkh_image_create_descriptor(surf->img, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_NEAREST,
		VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_image_set_name(surf->img, "SURF main color");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)vkh_image_get_view(surf->img),
		"SURF main color VIEW");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_SAMPLER, (uint64_t)vkh_image_get_sampler(surf->img),
		"SURF main color SAMPLER");
#endif
}
// create multisample color img if sample count > 1 and the stencil buffer multisampled or not
void _create_surface_secondary_images(VkvgSurface surf) {
	VkhDevice vkhd = (VkhDevice)&surf->dev->vkDev;
	if (surf->dev->samples > VK_SAMPLE_COUNT_1_BIT) {
		surf->imgMS = vkh_image_ms_create(
			vkhd, surf->format, (VkSampleCountFlagBits)surf->dev->samples, surf->width, surf->height, VKH_MEMORY_USAGE_GPU_ONLY,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		vkh_image_create_descriptor(surf->imgMS, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_NEAREST,
			VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
		vkh_image_set_name(surf->imgMS, "SURF MS color IMG");
		vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)vkh_image_get_view(surf->imgMS),
			"SURF MS color VIEW");
		vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_SAMPLER, (uint64_t)vkh_image_get_sampler(surf->imgMS),
			"SURF MS color SAMPLER");
#endif
	}
	surf->stencil = vkh_image_ms_create(vkhd, surf->dev->stencilFormat, (VkSampleCountFlagBits)surf->dev->samples, surf->width, surf->height,
		VKH_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	vkh_image_create_descriptor(surf->stencil, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_STENCIL_BIT, VK_FILTER_NEAREST,
		VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_image_set_name(surf->stencil, "SURF stencil");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)vkh_image_get_view(surf->stencil),
		"SURF stencil VIEW");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_SAMPLER, (uint64_t)vkh_image_get_sampler(surf->stencil),
		"SURF stencil SAMPLER");
#endif
}
void _create_framebuffer(VkvgSurface surf) {
	VkImageView attachments[] = {
		vkh_image_get_view(surf->img),
		vkh_image_get_view(surf->stencil),
		vkh_image_get_view(surf->imgMS),
	};
	VkFramebufferCreateInfo frameBufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
													 .renderPass = surf->dev->renderPass,
													 .attachmentCount = 3,
													 .pAttachments = attachments,
													 .width = surf->width,
													 .height = surf->height,
													 .layers = 1 };
	if (surf->dev->samples == VK_SAMPLE_COUNT_1_BIT)
		frameBufferCreateInfo.attachmentCount = 2;
	else if (surf->dev->deferredResolve) {
		attachments[0] = attachments[2];
		frameBufferCreateInfo.attachmentCount = 2;
	}
	VK_CHECK_RESULT(vkCreateFramebuffer(surf->dev->vkDev, &frameBufferCreateInfo, NULL, &surf->fb));
#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_device_set_object_name((VkhDevice)&surf->dev->vkDev, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)surf->fb, "SURF FB");
#endif
}
void _create_surface_images(VkvgSurface surf) {

	_create_surface_main_image(surf);
	_create_surface_secondary_images(surf);
	_create_framebuffer(surf);

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_image_set_name(surf->img, "surfImg");
	vkh_image_set_name(surf->imgMS, "surfImgMS");
	vkh_image_set_name(surf->stencil, "surfStencil");
#endif
}
VkvgSurface _create_surface(VkvgDevice dev, VkFormat format) {
	LOG(VKVG_LOG_INFO, "CREATE Surface\n");
	if (vkvg_device_status(dev)) {
		LOG(VKVG_LOG_ERR, "CREATE Surface failed, invalid Device\n");
		return (VkvgSurface)&_vkvg_status_device_error;
	}

	VkvgSurface surf = (vkvg_surface*)calloc(1, sizeof(vkvg_surface));
	if (!surf) {
		LOG(VKVG_LOG_ERR, "CREATE Surface failed, no memory\n");
		return (VkvgSurface)&_vkvg_status_no_memory;
	}

	surf->references = 1;
	surf->dev = dev;
	surf->format = format;

	if (dev->threadAware)
		mtx_init(&surf->mutex, mtx_plain);

	VkhDevice vkhd = (VkhDevice)&surf->dev->vkDev;

	surf->cmdPool =
		vkh_cmd_pool_create(vkhd, dev->gQueue->familyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	vkh_cmd_buffs_create(vkhd, surf->cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, &surf->cmd);

#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	surf->timeline = vkh_timeline_create(vkhd, 0);
#else
	surf->flushFence = vkh_fence_create(vkhd);
#endif
	surf->sem = vkh_semaphore_create(vkhd);
#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)surf->cmd, "vkvgSurfCmd");
#endif
	vkvg_device_reference(surf->dev);
	return surf;
}
// if fence sync, surf mutex must be locked.
/*bool _surface_wait_cmd (VkvgSurface surf) {
	LOG(VKVG_LOG_INFO, "SURF: _surface__wait_flush_fence\n");
#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	if (vkh_timeline_wait ((VkhDevice)surf->dev, surf->timeline, surf->timelineStep) == VK_SUCCESS)
		return true;
#else
	if (WaitForFences (surf->dev->vkDev, 1, &surf->flushFence, VK_TRUE, VKVG_FENCE_TIMEOUT) == VK_SUCCESS) {
		ResetFences (surf->dev->vkDev, 1, &surf->flushFence);
		return true;
	}
#endif
	LOG(VKVG_LOG_DEBUG, "CTX: _wait_flush_fence timeout\n");
	surf->status = VKVG_STATUS_TIMEOUT;
	return false;
}*/
// surface mutex must be locked to call this method, locking to guard also the surf->cmd local buffer usage.
void _surface_submit_cmd(VkvgSurface surf) {
	VkvgDevice dev = surf->dev;
	c_runtime_cx rtc;
	rtc.begin();
#ifdef VKVG_ENABLE_VK_TIMELINE_SEMAPHORE
	LOCK_DEVICE
		vkh_cmd_submit_timelined(dev->gQueue, &surf->cmd, surf->timeline, surf->timelineStep, surf->timelineStep + 1);
	surf->timelineStep++;
	UNLOCK_DEVICE
		vkh_timeline_wait((VkhDevice)&dev->vkDev, surf->timeline, surf->timelineStep);
#else
	LOCK_DEVICE
		vkh_cmd_submit(surf->dev->gQueue, &surf->cmd, surf->flushFence);
	UNLOCK_DEVICE
		WaitForFences(surf->dev->vkDev, 1, &surf->flushFence, VK_TRUE, VKVG_FENCE_TIMEOUT);
	ResetFences(surf->dev->vkDev, 1, &surf->flushFence);
#endif	 
	int ms = rtc.end();
#ifdef PWAIT_MS
	if (ms > 0)
		printf("surface wait ms: %d\n", ms);
#endif
}

#endif // 1

// recording
#if 1

#ifdef VKVG_RECORDING
void vkvg_start_recording(VkvgContext ctx) {
	if (ctx->status)
		return;
	_start_recording(ctx);
}
VkvgRecording vkvg_stop_recording(VkvgContext ctx) {
	if (ctx->status)
		return NULL;
	return _stop_recording(ctx);
}
uint32_t vkvg_recording_get_count(VkvgRecording rec) {
	if (!rec)
		return 0;
	return rec->commandsCount;
}
void* vkvg_recording_get_data(VkvgRecording rec) {
	if (!rec)
		return 0;
	return rec->buffer;
}
void vkvg_recording_get_command(VkvgRecording rec, uint32_t cmdIndex, uint32_t* cmd, void** dataOffset) {
	if (!rec)
		return;
	if (cmdIndex < rec->commandsCount) {
		*cmd = rec->commands[cmdIndex].cmd;
		*dataOffset = (void*)rec->commands[cmdIndex].dataOffset;
	}
	else {
		*cmd = 0;
		*dataOffset = NULL;
	}
}
void vkvg_replay(VkvgContext ctx, VkvgRecording rec) {
	if (!rec)
		return;
	for (uint32_t i = 0; i < rec->commandsCount; i++)
		_replay_command(ctx, rec, i);
}
void vkvg_replay_command(VkvgContext ctx, VkvgRecording rec, uint32_t cmdIndex) {
	if (!rec)
		return;
	if (cmdIndex < rec->commandsCount)
		_replay_command(ctx, rec, cmdIndex);
}
void vkvg_recording_destroy(VkvgRecording rec) {
	if (!rec)
		return;
	_destroy_recording(rec);
}
#endif

#define VKVG_RECORDING_INIT_BUFFER_SIZE_TRESHOLD 64
#define VKVG_RECORDING_INIT_BUFFER_SIZE          1024
#define VKVG_RECORDING_INIT_COMMANDS_COUNT       64

vkvg_recording_t* _new_recording() {

	vkvg_recording_t* rec = (vkvg_recording_t*)calloc(1, sizeof(vkvg_recording_t));

	rec->commandsReservedCount = VKVG_RECORDING_INIT_COMMANDS_COUNT;
	rec->bufferReservedSize = VKVG_RECORDING_INIT_BUFFER_SIZE;
	rec->commands = (vkvg_record_t*)malloc(rec->commandsReservedCount * sizeof(vkvg_record_t));
	rec->buffer = (char*)malloc(rec->bufferReservedSize);

	return rec;
}
void _destroy_recording(vkvg_recording_t* rec) {
	if (!rec)
		return;
	for (uint32_t i = 0; i < rec->commandsCount; i++) {
		if (rec->commands[i].cmd == VKVG_CMD_SET_SOURCE)
			vkvg_pattern_destroy((VkvgPattern)(rec->buffer + rec->commands[i].dataOffset));
		else if (rec->commands[i].cmd == VKVG_CMD_SET_SOURCE_SURFACE)
			vkvg_surface_destroy((VkvgSurface)(rec->buffer + rec->commands[i].dataOffset + 2 * sizeof(float)));
	}
	free(rec->commands);
	free(rec->buffer);
	free(rec);
}
void _start_recording(VkvgContext ctx) {
	if (ctx->recording)
		_destroy_recording(ctx->recording);
	ctx->recording = _new_recording();
}
vkvg_recording_t* _stop_recording(VkvgContext ctx) {
	vkvg_recording_t* rec = ctx->recording;
	if (!rec)
		return NULL;
	if (!rec->commandsCount) {
		_destroy_recording(rec);
		ctx->recording = NULL;
		return NULL;
	}
	/*rec->buffer = realloc(rec->buffer, rec->bufferSize);
	rec->commands = (vkvg_record_t*)realloc(rec->commands, rec->commandsCount * sizeof (vkvg_record_t));*/
	ctx->recording = NULL;
	return rec;
}
void* _ensure_recording_buffer(vkvg_recording_t* rec, size_t size) {
	if (rec->bufferReservedSize >= rec->bufferSize - VKVG_RECORDING_INIT_BUFFER_SIZE_TRESHOLD - size) {
		rec->bufferReservedSize += VKVG_RECORDING_INIT_BUFFER_SIZE;
		rec->buffer = (char*)realloc(rec->buffer, rec->bufferReservedSize);
	}
	return rec->buffer + rec->bufferSize;
}
void* _advance_recording_buffer_unchecked(vkvg_recording_t* rec, size_t size) {
	rec->bufferSize += size;
	return rec->buffer + rec->bufferSize;
}

#define STORE_FLOATS(floatcount)                                                                                       \
    for (i = 0; i < floatcount; i++) {                                                                                 \
        buff           = (char*)_ensure_recording_buffer(rec, sizeof(float));                                                 \
        *(float *)buff = (float)va_arg(args, double);                                                                  \
        buff           = (char*)_advance_recording_buffer_unchecked(rec, sizeof(float));                                      \
    }
#define STORE_BOOLS(count)                                                                                             \
    for (i = 0; i < count; i++) {                                                                                      \
        buff          = (char*)_ensure_recording_buffer(rec, sizeof(bool));                                                   \
        *(bool *)buff = (bool)va_arg(args, int);                                                                       \
        _advance_recording_buffer_unchecked(rec, sizeof(bool));                                                        \
    }
#define STORE_UINT32(count)                                                                                            \
    for (i = 0; i < count; i++) {                                                                                      \
        buff              = (char*)_ensure_recording_buffer(rec, sizeof(uint32_t));                                           \
        *(uint32_t *)buff = (uint32_t)va_arg(args, uint32_t);                                                          \
        buff              = (char*)_advance_recording_buffer_unchecked(rec, sizeof(uint32_t));                                \
    }

void _record(vkvg_recording_t* rec, ...) {
	va_list args;
	va_start(args, rec);

	uint32_t cmd = va_arg(args, uint32_t);

	if (rec->commandsCount == rec->commandsReservedCount) {
		rec->commandsReservedCount += VKVG_RECORDING_INIT_COMMANDS_COUNT;
		rec->commands = (vkvg_record_t*)realloc(rec->commands, rec->commandsReservedCount * sizeof(vkvg_record_t));
	}
	vkvg_record_t* r = &rec->commands[rec->commandsCount++];
	r->cmd = cmd;
	r->dataOffset = rec->bufferSize;

	char* buff;
	int   i = 0;

	if (cmd & VKVG_CMD_PATH_COMMANDS) {
		if ((cmd & VKVG_CMD_PATHPROPS_COMMANDS) == VKVG_CMD_PATHPROPS_COMMANDS) {
			switch (r->cmd) {
			case VKVG_CMD_SET_LINE_WIDTH:
			case VKVG_CMD_SET_MITER_LIMIT:
				STORE_FLOATS(1);
				break;
			case VKVG_CMD_SET_LINE_JOIN:
				STORE_UINT32(1);
				break;
			case VKVG_CMD_SET_LINE_CAP:
				STORE_UINT32(1);
				break;
			case VKVG_CMD_SET_OPERATOR:
				STORE_UINT32(1);
				break;
			case VKVG_CMD_SET_FILL_RULE:
				STORE_UINT32(1);
				break;
			case VKVG_CMD_SET_DASH:
				break;
			}
		}
		else {
			switch (cmd) {
			case VKVG_CMD_MOVE_TO:
			case VKVG_CMD_LINE_TO:
			case VKVG_CMD_REL_MOVE_TO:
			case VKVG_CMD_REL_LINE_TO:
				STORE_FLOATS(2);
				break;
			case VKVG_CMD_RECTANGLE:
			case VKVG_CMD_QUADRATIC_TO:
			case VKVG_CMD_REL_QUADRATIC_TO:
				STORE_FLOATS(4);
				break;
			case VKVG_CMD_ARC:
			case VKVG_CMD_ARC_NEG:
				STORE_FLOATS(5);
				break;
			case VKVG_CMD_CURVE_TO:
			case VKVG_CMD_REL_CURVE_TO:
				STORE_FLOATS(6);
				break;
			case VKVG_CMD_ELLIPTICAL_ARC_TO:
			case VKVG_CMD_REL_ELLIPTICAL_ARC_TO:
				STORE_FLOATS(5);
				STORE_BOOLS(2);
				break;
			case VKVG_CMD_NEW_PATH:
			case VKVG_CMD_NEW_SUB_PATH:
			case VKVG_CMD_CLOSE_PATH:
				break;
			}
		}
	}
	else if (!(r->cmd & VKVG_CMD_DRAW_COMMANDS)) {
		if (r->cmd & VKVG_CMD_TRANSFORM_COMMANDS) {
			switch (r->cmd) {
			case VKVG_CMD_TRANSLATE:
			case VKVG_CMD_SCALE:
				STORE_FLOATS(2);
				break;
			case VKVG_CMD_ROTATE:
				STORE_FLOATS(1);
				break;
			case VKVG_CMD_IDENTITY_MATRIX:
				break;
			case VKVG_CMD_SET_MATRIX:
			case VKVG_CMD_TRANSFORM: {
				buff = (char*)_ensure_recording_buffer(rec, sizeof(vkvg_matrix_t));
				vkvg_matrix_t* mat = (vkvg_matrix_t*)va_arg(args, vkvg_matrix_t*);
				memcpy(buff, mat, sizeof(vkvg_matrix_t));
				buff = (char*)_advance_recording_buffer_unchecked(rec, sizeof(vkvg_matrix_t));

			} break;
			}
		}
		else if (r->cmd & VKVG_CMD_PATTERN_COMMANDS) {
			switch (r->cmd) {
			case VKVG_CMD_SET_SOURCE_RGBA:
				STORE_FLOATS(4);
				break;
			case VKVG_CMD_SET_SOURCE_RGB:
				STORE_FLOATS(3);
				break;
			case VKVG_CMD_SET_SOURCE_COLOR:
				STORE_UINT32(1);
				break;
			case VKVG_CMD_SET_SOURCE: {
				buff = (char*)_ensure_recording_buffer(rec, sizeof(VkvgPattern));
				VkvgPattern pat = (VkvgPattern)va_arg(args, VkvgPattern);
				vkvg_pattern_reference(pat);
				VkvgPattern* pPat = (VkvgPattern*)buff;
				*pPat = pat;
				_advance_recording_buffer_unchecked(rec, sizeof(VkvgPattern));
			} break;
			case VKVG_CMD_SET_SOURCE_SURFACE:
				STORE_FLOATS(2);
				{
					buff = (char*)_ensure_recording_buffer(rec, sizeof(VkvgSurface));
					VkvgSurface surf = (VkvgSurface)va_arg(args, VkvgSurface);
					vkvg_surface_reference(surf);
					*(VkvgSurface*)buff = surf;
					_advance_recording_buffer_unchecked(rec, sizeof(VkvgSurface));
				}
				break;
			}
		}
		else if (r->cmd & VKVG_CMD_TEXT_COMMANDS) {
			char* txt;
			int   txtLen;
			switch (r->cmd) {
			case VKVG_CMD_SET_FONT_SIZE:
				STORE_UINT32(1);
				break;
			case VKVG_CMD_SHOW_TEXT:
			case VKVG_CMD_SET_FONT_FACE:
				txt = (char*)va_arg(args, char*);
				txtLen = strlen(txt);
				buff = (char*)_ensure_recording_buffer(rec, txtLen * sizeof(char));
				strcpy(buff, txt);
				_advance_recording_buffer_unchecked(rec, txtLen * sizeof(char));
				break;
			case VKVG_CMD_SET_FONT_PATH:
				break;
			}
		}
	}
	va_end(args);
}
void _replay_command(VkvgContext ctx, VkvgRecording rec, uint32_t index) {
	vkvg_record_t* r = &rec->commands[index];
	float* floats = (float*)(rec->buffer + r->dataOffset);
	uint32_t* uints = (uint32_t*)floats;
	if (r->cmd & VKVG_CMD_PATH_COMMANDS) {
		if ((r->cmd & VKVG_CMD_RELATIVE_COMMANDS) == VKVG_CMD_RELATIVE_COMMANDS) {
			switch (r->cmd) {
			case VKVG_CMD_REL_MOVE_TO:
				vkvg_rel_move_to(ctx, floats[0], floats[1]);
				return;
			case VKVG_CMD_REL_LINE_TO:
				vkvg_rel_line_to(ctx, floats[0], floats[1]);
				return;
			case VKVG_CMD_REL_CURVE_TO:
				vkvg_rel_curve_to(ctx, floats[0], floats[1], floats[2], floats[3], floats[4], floats[5]);
				return;
			case VKVG_CMD_REL_QUADRATIC_TO:
				vkvg_rel_quadratic_to(ctx, floats[0], floats[1], floats[2], floats[3]);
				return;
			case VKVG_CMD_REL_ELLIPTICAL_ARC_TO: {
				bool* flags = (bool*)&floats[5];
				vkvg_rel_elliptic_arc_to(ctx, floats[0], floats[1], flags[0], flags[1], floats[2], floats[3],
					floats[4]);
			}
											   return;
			}
		}
		else if ((r->cmd & VKVG_CMD_PATHPROPS_COMMANDS) == VKVG_CMD_PATHPROPS_COMMANDS) {
			switch (r->cmd) {
			case VKVG_CMD_SET_LINE_WIDTH:
				vkvg_set_line_width(ctx, floats[0]);
				return;
			case VKVG_CMD_SET_MITER_LIMIT:
				vkvg_set_miter_limit(ctx, floats[0]);
				return;
			case VKVG_CMD_SET_LINE_JOIN:
				vkvg_set_line_join(ctx, (vkvg_line_join_t)uints[0]);
				return;
			case VKVG_CMD_SET_LINE_CAP:
				vkvg_set_line_cap(ctx, (vkvg_line_cap_t)uints[0]);
				return;
			case VKVG_CMD_SET_OPERATOR:
				vkvg_set_operator(ctx, (vkvg_operator_t)uints[0]);
				return;
			case VKVG_CMD_SET_FILL_RULE:
				vkvg_set_fill_rule(ctx, (vkvg_fill_rule_t)uints[0]);
				return;
			case VKVG_CMD_SET_DASH:
				vkvg_set_dash(ctx, &floats[2], uints[0], floats[1]);
				return;
			}
		}
		else {
			switch (r->cmd) {
			case VKVG_CMD_NEW_PATH:
				vkvg_new_path(ctx);
				return;
			case VKVG_CMD_NEW_SUB_PATH:
				vkvg_new_sub_path(ctx);
				return;
			case VKVG_CMD_CLOSE_PATH:
				vkvg_close_path(ctx);
				return;
			case VKVG_CMD_RECTANGLE:
				vkvg_rectangle(ctx, floats[0], floats[1], floats[2], floats[3]);
				return;
			case VKVG_CMD_ARC:
				vkvg_arc(ctx, floats[0], floats[1], floats[2], floats[3], floats[4]);
				return;
			case VKVG_CMD_ARC_NEG:
				vkvg_arc(ctx, floats[0], floats[1], floats[2], floats[3], floats[4]);
				return;
				/*case VKVG_CMD_ELLIPSE:
					vkvg_ellipse (ctx, floats[0], floats[1], floats[2], floats[3], floats[4]);
					break;*/
			case VKVG_CMD_MOVE_TO:
				vkvg_move_to(ctx, floats[0], floats[1]);
				return;
			case VKVG_CMD_LINE_TO:
				vkvg_line_to(ctx, floats[0], floats[1]);
				return;
			case VKVG_CMD_CURVE_TO:
				vkvg_curve_to(ctx, floats[0], floats[1], floats[2], floats[3], floats[4], floats[5]);
				return;
			case VKVG_CMD_ELLIPTICAL_ARC_TO: {
				bool* flags = (bool*)&floats[5];
				vkvg_elliptic_arc_to(ctx, floats[0], floats[1], flags[0], flags[1], floats[2], floats[3], floats[4]);
			}
										   return;
			case VKVG_CMD_QUADRATIC_TO:
				vkvg_quadratic_to(ctx, floats[0], floats[1], floats[2], floats[3]);
				return;
			}
		}
	}
	else if (r->cmd & VKVG_CMD_DRAW_COMMANDS) {
		switch (r->cmd) {
		case VKVG_CMD_PAINT:
			vkvg_paint(ctx);
			return;
		case VKVG_CMD_FILL:
			vkvg_fill(ctx);
			return;
		case VKVG_CMD_STROKE:
			vkvg_stroke(ctx);
			return;
		case VKVG_CMD_CLIP:
			vkvg_clip(ctx);
			return;
		case VKVG_CMD_CLEAR:
			vkvg_clear(ctx);
			return;
		case VKVG_CMD_FILL_PRESERVE:
			vkvg_fill_preserve(ctx);
			return;
		case VKVG_CMD_STROKE_PRESERVE:
			vkvg_stroke_preserve(ctx);
			return;
		case VKVG_CMD_CLIP_PRESERVE:
			vkvg_clip_preserve(ctx);
			return;
		}
	}
	else if (r->cmd & VKVG_CMD_TRANSFORM_COMMANDS) {
		switch (r->cmd) {
		case VKVG_CMD_TRANSLATE:
			vkvg_translate(ctx, floats[0], floats[1]);
			return;
		case VKVG_CMD_SCALE:
			vkvg_scale(ctx, floats[0], floats[1]);
			return;
		case VKVG_CMD_ROTATE:
			vkvg_rotate(ctx, floats[0]);
			return;
		case VKVG_CMD_IDENTITY_MATRIX:
			vkvg_identity_matrix(ctx);
			return;
		case VKVG_CMD_TRANSFORM: {
			vkvg_matrix_t* mat = (vkvg_matrix_t*)&floats[0];
			vkvg_transform(ctx, mat);
		}
							   return;
		case VKVG_CMD_SET_MATRIX: {
			vkvg_matrix_t* mat = (vkvg_matrix_t*)&floats[0];
			vkvg_set_matrix(ctx, mat);
		}
								return;
		}
	}
	else if (r->cmd & VKVG_CMD_PATTERN_COMMANDS) {
		switch (r->cmd) {
		case VKVG_CMD_SET_SOURCE_RGB:
			vkvg_set_source_rgb(ctx, floats[0], floats[1], floats[2]);
			return;
		case VKVG_CMD_SET_SOURCE_RGBA:
			vkvg_set_source_rgba(ctx, floats[0], floats[1], floats[2], floats[3]);
			return;
		case VKVG_CMD_SET_SOURCE_COLOR:
			vkvg_set_source_color(ctx, uints[0]);
			return;
		case VKVG_CMD_SET_SOURCE: {
			VkvgPattern pat = *((VkvgPattern*)(rec->buffer + r->dataOffset));
			vkvg_set_source(ctx, pat);
		}
								return;
		case VKVG_CMD_SET_SOURCE_SURFACE: {
			VkvgSurface surf = *((VkvgSurface*)&floats[2]);
			vkvg_set_source_surface(ctx, surf, floats[0], floats[1]);
		}
										return;
		}
	}
	else if (r->cmd & VKVG_CMD_TEXT_COMMANDS) {
		char* txt = (char*)floats;
		switch (r->cmd) {
		case VKVG_CMD_SET_FONT_SIZE:
			vkvg_set_font_size(ctx, uints[0]);
			return;
		case VKVG_CMD_SET_FONT_FACE:
			vkvg_select_font_face(ctx, txt);
			return;
			/*case VKVG_CMD_SET_FONT_PATH:
				vkvg_load_font_from_path (ctx, txt);
				return;	*/
		case VKVG_CMD_SHOW_TEXT:
			vkvg_show_text(ctx, txt);
			return;
		}
	}
	else {
		switch (r->cmd) {
		case VKVG_CMD_SAVE:
			vkvg_save(ctx);
			return;
		case VKVG_CMD_RESTORE:
			vkvg_restore(ctx);
			return;
		}
	}
	LOG(VKVG_LOG_ERR, "[REPLAY] unimplemented command: %.4x\n", r->cmd);
}
#endif // 1
#if 1

// compute length of float vector 2d
vkvg_inline float vec2_length(vec2 v) { return sqrtf(v.x * v.x + v.y * v.y); }
// compute normal direction vector from line defined by 2 points in double precision
vkvg_inline vec2d vec2d_line_norm(vec2d a, vec2d b) {
	vec2d  d = { b.x - a.x, b.y - a.y };
	double md = sqrt(d.x * d.x + d.y * d.y);
	d.x /= md;
	d.y /= md;
	return d;
}
// compute normal direction vector from line defined by 2 points
vkvg_inline vec2 vec2_line_norm(vec2 a, vec2 b) {
	vec2  d = { b.x - a.x, b.y - a.y };
	float md = sqrtf(d.x * d.x + d.y * d.y);
	d.x /= md;
	d.y /= md;
	return d;
}
// compute sum of two double precision vectors
vkvg_inline vec2d vec2d_add(vec2d a, vec2d b) { return vec2d{ a.x + b.x, a.y + b.y }; }
// compute subbstraction of two double precision vectors
vkvg_inline vec2d vec2d_sub(vec2d a, vec2d b) { return vec2d{ a.x - b.x, a.y - b.y }; }
// multiply 2d vector by scalar
vkvg_inline vec2d vec2d_mult_s(vec2d a, double m) { return vec2d{ a.x * m, a.y * m }; }
vkvg_inline vec2d vec2d_div_s(vec2d a, double m) { return vec2d{ a.x / m, a.y / m }; }
// compute length of double vector 2d
vkvg_inline double vec2d_length(vec2d v) { return sqrt(v.x * v.x + v.y * v.y); }
// normalize double vector
vkvg_inline vec2d vec2d_norm(vec2d a) {
	double m = sqrt(a.x * a.x + a.y * a.y);
	return vec2d{ a.x / m, a.y / m };
}
// compute perpendicular vector
vkvg_inline vec2d vec2d_perp(vec2d a) { return vec2d{ a.y, -a.x }; }
vkvg_inline bool  vec2d_isnan(vec2d v) { return (bool)(isnan((float)v.x) || isnan((float)v.y)); }

// test equality of two single precision vectors
vkvg_inline bool vec2_equ(vec2 a, vec2 b) { return (EQUF(a.x, b.x) & EQUF(a.y, b.y)); }
// compute sum of two single precision vectors
vkvg_inline vec2 vec2_add(vec2 a, vec2 b) { return vec2{ a.x + b.x, a.y + b.y }; }
// compute subbstraction of two single precision vectors
vkvg_inline vec2 vec2_sub(vec2 a, vec2 b) { return vec2{ a.x - b.x, a.y - b.y }; }
// multiply 2d vector by scalar
vkvg_inline vec2 vec2_mult_s(vec2 a, float m) { return vec2{ a.x * m, a.y * m }; }
// devide 2d vector by scalar
vkvg_inline vec2 vec2_div_s(vec2 a, float m) { return vec2{ a.x / m, a.y / m }; }
// normalize float vector
vkvg_inline vec2 vec2_norm(vec2 a) {
	float m = sqrtf(a.x * a.x + a.y * a.y);
	return vec2{ a.x / m, a.y / m };
}
// compute perpendicular vector
vkvg_inline vec2 vec2_perp(vec2 a) { return vec2{ a.y, -a.x }; }
// compute opposite of single precision vector
vkvg_inline void vec2_inv(vec2* v) {
	v->x = -v->x;
	v->y = -v->y;
}
// test if one component of float vector is nan
vkvg_inline bool vec2_isnan(vec2 v) { return (bool)(isnan(v.x) || isnan(v.y)); }
// test if one component of double vector is nan
vkvg_inline float vec2_dot(vec2 a, vec2 b) { return (a.x * b.x) + (a.y * b.y); }
vkvg_inline float vec2_det(vec2 a, vec2 b) { return a.x * b.y - a.y * b.x; }
vkvg_inline float vec2_slope(vec2 a, vec2 b) { return (b.y - a.y) / (b.x - a.x); }

// convert double precision vector to single precision
vkvg_inline vec2 vec2d_to_vec2(vec2d vd) { return vec2{ (float)vd.x, (float)vd.y }; }
vkvg_inline bool vec4_equ(vec4 a, vec4 b) {
	return (EQUF(a.x, b.x) & EQUF(a.y, b.y) & EQUF(a.z, b.z) & EQUF(a.w, b.w));
}
vkvg_inline vec2 mat2_mult_vec2(mat2 m, vec2 v) {
	return vec2{ (m.row0.x * v.x) + (m.row0.y * v.y), (m.row1.x * v.x) + (m.row1.y * v.y) };
}
vkvg_inline float mat2_det(mat2* m) { return (m->row0.x * m->row1.y) - (m->row0.y * m->row1.y); }

#endif // 1

// fonts
#if 1

static int defaultFontCharSize = 12 << 6;

void _fonts_cache_create(VkvgDevice dev) {
	_font_cache_t* cache = (_font_cache_t*)calloc(1, sizeof(_font_cache_t));

	if (dev->threadAware)
		mtx_init(&cache->mutex, mtx_plain);

#ifdef VKVG_USE_FONTCONFIG
	cache->config = FcInitLoadConfigAndFonts();
	if (!cache->config) {
		LOG(VKVG_LOG_DEBUG,
			"Font config initialisation failed, consider using 'FONTCONFIG_PATH' and 'FONTCONFIG_FILE' environmane\
					   variables to point to 'fonts.conf' needed for FontConfig startup");
		assert(cache->config);
	}
#endif

#ifdef VKVG_USE_FREETYPE
	FT_CHECK_RESULT(FT_Init_FreeType(&cache->library));
#endif

#if defined(VKVG_LCD_FONT_FILTER) && defined(FT_CONFIG_OPTION_SUBPIXEL_RENDERING)
	FT_CHECK_RESULT(FT_Library_SetLcdFilter(cache->library, FT_LCD_FILTER_LIGHT));
	cache->texFormat = FB_COLOR_FORMAT;
	cache->texPixelSize = 4;
#else
	cache->texFormat = VK_FORMAT_R8_UNORM;
	cache->texPixelSize = 1;
#endif

	VkhDevice vkhd = (VkhDevice)&dev->vkDev;

	cache->texLength = FONT_CACHE_INIT_LAYERS;
	cache->texture = vkh_tex2d_array_create(
		vkhd, cache->texFormat, FONT_PAGE_SIZE, FONT_PAGE_SIZE, cache->texLength, VKH_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	vkh_image_create_descriptor(cache->texture, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

	cache->uploadFence = vkh_fence_create(vkhd);

	const uint32_t buffLength = FONT_PAGE_SIZE * FONT_PAGE_SIZE * cache->texPixelSize;

	vkh_buffer_init(vkhd, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VKH_MEMORY_USAGE_CPU_TO_GPU, buffLength, &cache->buff,
		true);

	cache->cmd = vkh_cmd_buff_create(vkhd, dev->cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	// Set texture cache initial layout to shaderReadOnly to prevent error msg if cache is not fill
	const VkImageSubresourceRange subres = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, cache->texLength };
	vkh_cmd_begin(cache->cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkh_image_set_layout_subres(cache->cmd, cache->texture, subres, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	VK_CHECK_RESULT(vkEndCommandBuffer(cache->cmd));
	_device_submit_cmd(dev, &cache->cmd, cache->uploadFence);

	cache->hostBuff = (uint8_t*)malloc(buffLength);
	cache->pensY = (int*)calloc(cache->texLength, sizeof(int));

	dev->fontCache = cache;
}
/// increase layer count of 2d texture array used as font cache.
void _increase_font_tex_array(VkvgDevice dev) {
	LOG(VKVG_LOG_INFO, "_increase_font_tex_array\n");

	_font_cache_t* cache = dev->fontCache;

	WaitForFences(dev->vkDev, 1, &cache->uploadFence, VK_TRUE, UINT64_MAX);
	ResetFences(dev->vkDev, 1, &cache->uploadFence);

	vkResetCommandBuffer(cache->cmd, 0);

	uint8_t  newSize = cache->texLength + FONT_CACHE_INIT_LAYERS;
	VkhImage newImg = vkh_tex2d_array_create(
		(VkhDevice)&dev->vkDev, cache->texFormat, FONT_PAGE_SIZE, FONT_PAGE_SIZE, newSize, VKH_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	vkh_image_create_descriptor(newImg, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_NEAREST,
		VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

	VkImageSubresourceRange subresNew = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, newSize };
	VkImageSubresourceRange subres = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, cache->texLength };

	vkh_cmd_begin(cache->cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vkh_image_set_layout_subres(cache->cmd, newImg, subresNew, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);
	vkh_image_set_layout_subres(cache->cmd, cache->texture, subres, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);

	VkImageCopy cregion = { .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, cache->texLength},
						   .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, cache->texLength},
						   .extent = {FONT_PAGE_SIZE, FONT_PAGE_SIZE, 1} };

	vkCmdCopyImage(cache->cmd, vkh_image_get_vkimage(cache->texture), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		vkh_image_get_vkimage(newImg), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cregion);

	vkh_image_set_layout_subres(cache->cmd, newImg, subresNew, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	vkh_image_set_layout_subres(cache->cmd, cache->texture, subres, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	VK_CHECK_RESULT(vkEndCommandBuffer(cache->cmd));

	_device_submit_cmd(dev, &cache->cmd, cache->uploadFence);
	WaitForFences(dev->vkDev, 1, &cache->uploadFence, VK_TRUE, UINT64_MAX);

	cache->pensY = (int*)realloc(cache->pensY, newSize * sizeof(int));
	void* tmp = memset(&cache->pensY[cache->texLength], 0, FONT_CACHE_INIT_LAYERS * sizeof(int));

	vkh_image_destroy(cache->texture);

	cache->texLength = newSize;
	cache->texture = newImg;

	_device_wait_idle(dev);
}
// flush font stagging buffer to cache texture array
// Trigger stagging buffer to be uploaded in font cache. Groupping upload improve performances.
void _flush_chars_to_tex(VkvgDevice dev, _vkvg_font_t* f) {

	_font_cache_t* cache = dev->fontCache;
	if (cache->stagingX == 0) // no char in stagging buff to flush
		return;

	LOG(VKVG_LOG_INFO, "_flush_chars_to_tex pen(%d, %d)\n", f->curLine.penX, f->curLine.penY);
	WaitForFences(dev->vkDev, 1, &cache->uploadFence, VK_TRUE, UINT64_MAX);
	ResetFences(dev->vkDev, 1, &cache->uploadFence);

	vkResetCommandBuffer(cache->cmd, 0);

	memcpy(vkh_buffer_get_mapped_pointer(&cache->buff), cache->hostBuff,
		(uint64_t)f->curLine.height * FONT_PAGE_SIZE * cache->texPixelSize);

	vkh_cmd_begin(cache->cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkImageSubresourceRange subres = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, f->curLine.pageIdx, 1 };
	vkh_image_set_layout_subres(cache->cmd, cache->texture, subres, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);

	VkBufferImageCopy bufferCopyRegion = {};
	bufferCopyRegion.bufferRowLength = FONT_PAGE_SIZE;
	bufferCopyRegion.bufferImageHeight = f->curLine.height;
	bufferCopyRegion.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, f->curLine.pageIdx, 1 };
	bufferCopyRegion.imageOffset = { f->curLine.penX, f->curLine.penY, 0 };
	bufferCopyRegion.imageExtent = { (uint32_t)(FONT_PAGE_SIZE - f->curLine.penX),(uint32_t)f->curLine.height, 1 };

	vkCmdCopyBufferToImage(cache->cmd, cache->buff.buffer, vkh_image_get_vkimage(cache->texture),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

	vkh_image_set_layout_subres(cache->cmd, cache->texture, subres, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	VK_CHECK_RESULT(vkEndCommandBuffer(cache->cmd));

	_device_submit_cmd(dev, &cache->cmd, cache->uploadFence);

	f->curLine.penX += cache->stagingX;
	cache->stagingX = 0;
	memset(cache->hostBuff, 0, (uint64_t)FONT_PAGE_SIZE * FONT_PAGE_SIZE * cache->texPixelSize);
}
/// Start a new line in font cache, increase texture layer count if needed.
void _init_next_line_in_tex_cache(VkvgDevice dev, _vkvg_font_t* f) {
	_font_cache_t* cache = dev->fontCache;
	int            i;
	for (i = 0; i < cache->texLength; ++i) {
		if (cache->pensY[i] + f->curLine.height >= FONT_PAGE_SIZE)
			continue;
		f->curLine.pageIdx = (unsigned char)i;
		f->curLine.penX = 0;
		f->curLine.penY = cache->pensY[i];
		cache->pensY[i] += f->curLine.height;
		return;
	}
	_flush_chars_to_tex(dev, f);
	_increase_font_tex_array(dev);
	_init_next_line_in_tex_cache(dev, f);
}
void _font_cache_destroy(VkvgDevice dev) {
	_font_cache_t* cache = (_font_cache_t*)dev->fontCache;

	free(cache->hostBuff);

	for (int i = 0; i < cache->fontsCount; ++i) {
		_vkvg_font_identity_t* f = &cache->fonts[i];
		for (uint32_t j = 0; j < f->sizeCount; j++) {
			_vkvg_font_t* s = &f->sizes[j];
#ifdef VKVG_USE_FREETYPE
			for (int g = 0; g < s->face->num_glyphs; ++g) {
				if (s->charLookup[g] != NULL)
					free(s->charLookup[g]);
			}
			FT_Done_Face(s->face);
#else
			for (int g = 0; g < f->stbInfo.numGlyphs; ++g) {
				if (s->charLookup[g] != NULL)
					free(s->charLookup[g]);
			}
#endif

#ifdef VKVG_USE_HARFBUZZ_CX
			hb_font_destroy(s->hb_font);
#endif

			free(s->charLookup);
		}
		free(f->sizes);
		free(f->fontFile);
		for (uint32_t j = 0; j < f->namesCount; j++)
			free(f->names[j]);
		if (f->namesCount > 0)
			free(f->names);
		free(f->fontBuffer);
	}

	free(cache->fonts);
	free(cache->pensY);

	vkh_buffer_reset(&cache->buff);
	vkh_image_destroy(cache->texture);
	// vkFreeCommandBuffers(dev->vkDev,dev->cmdPool, 1, &cache->cmd);
	vkDestroyFence(dev->vkDev, cache->uploadFence, NULL);
#ifdef VKVG_USE_FREETYPE
	FT_Done_FreeType(cache->library);
#endif
#ifdef VKVG_USE_FONTCONFIG
	FcConfigDestroy(cache->config);
	FcFini();
#endif

	if (dev->threadAware)
		mtx_destroy(&cache->mutex);

	free(dev->fontCache);
}

void _font_cache_update_context_descset(VkvgContext ctx) {
	if (ctx->fontCacheImg)
		vkh_image_destroy(ctx->fontCacheImg);

	LOCK_FONTCACHE(ctx->dev)

		ctx->fontCacheImg = ctx->dev->fontCache->texture;
	vkh_image_reference(ctx->fontCacheImg);

	//_update_descriptor_set(ctx, ctx->fontCacheImg, ctx->dsFont); 
	push_update_descriptor_set_a(ctx, ctx->fontCacheImg, 0);

	UNLOCK_FONTCACHE(ctx->dev)
}
// create a new char entry and put glyph in stagging buffer, ready for upload.
_char_ref* _prepare_char(VkvgDevice dev, VkvgText tr, uint32_t gindex) {
	_vkvg_font_t* f = tr->font;
#ifdef VKVG_USE_FREETYPE
#if defined(VKVG_LCD_FONT_FILTER) && defined(FT_CONFIG_OPTION_SUBPIXEL_RENDERING)
	FT_CHECK_RESULT(FT_Load_Glyph(f->face, gindex, FT_LOAD_TARGET_NORMAL));
	FT_CHECK_RESULT(FT_Render_Glyph(f->face->glyph, FT_RENDER_MODE_LCD));
#else
	FT_CHECK_RESULT(FT_Load_Glyph(f->face, gindex, FT_LOAD_RENDER));
#endif

	FT_GlyphSlot   slot = f->face->glyph;
	FT_Bitmap      bmp = slot->bitmap;
	uint32_t       bmpByteWidth = bmp.width;
	uint32_t       bmpPixelWidth = bmp.width;
	uint32_t       bmpRows = bmp.rows;
	unsigned char* buffer = bmp.buffer;

#if defined(VKVG_LCD_FONT_FILTER) && defined(FT_CONFIG_OPTION_SUBPIXEL_RENDERING)
	bmpPixelWidth /= 3;
#endif
#else
	stbtt_fontinfo* pStbInfo = &tr->fontId->stbInfo;
	int             c_x1, c_y1, c_x2, c_y2;
	stbtt_GetGlyphBitmapBox(pStbInfo, gindex, f->scale, f->scale, &c_x1, &c_y1, &c_x2, &c_y2);
	uint32_t bmpByteWidth = c_x2 - c_x1;
	uint32_t bmpPixelWidth = bmpByteWidth;
	uint32_t bmpRows = c_y2 - c_y1;
#endif
	uint8_t* data = dev->fontCache->hostBuff;

	if (dev->fontCache->stagingX + f->curLine.penX + bmpPixelWidth > FONT_PAGE_SIZE) {
		_flush_chars_to_tex(dev, f);
		_init_next_line_in_tex_cache(dev, f);
	}

	_char_ref* cr = (_char_ref*)malloc(sizeof(_char_ref));
	int        penX = dev->fontCache->stagingX;

#ifdef VKVG_USE_FREETYPE
	for (uint32_t y = 0; y < bmpRows; y++) {
		for (uint32_t x = 0; x < bmpPixelWidth; x++) {
#if defined(VKVG_LCD_FONT_FILTER) && defined(FT_CONFIG_OPTION_SUBPIXEL_RENDERING)
			unsigned char r = buffer[y * bmp.pitch + x * 3];
			unsigned char g = buffer[y * bmp.pitch + x * 3 + 1];
			unsigned char b = buffer[y * bmp.pitch + x * 3 + 2];

			data[(penX + x + y * FONT_PAGE_SIZE) * 4] = b;
			data[(penX + x + y * FONT_PAGE_SIZE) * 4 + 1] = g;
			data[(penX + x + y * FONT_PAGE_SIZE) * 4 + 2] = r;
			data[(penX + x + y * FONT_PAGE_SIZE) * 4 + 3] = (r + g + b) / 3;
#else
			data[penX + x + y * FONT_PAGE_SIZE] = buffer[x + y * bmpPixelWidth];
#endif
		}
	}
	cr->bmpDiff.x = (int16_t)slot->bitmap_left;
	cr->bmpDiff.y = (int16_t)slot->bitmap_top;
	cr->advance = slot->advance;
#else
	int      advance;
	int      lsb;
	stbtt_GetGlyphHMetrics(pStbInfo, gindex, &advance, &lsb);
	stbtt_MakeGlyphBitmap(pStbInfo, data + penX, bmpPixelWidth, bmpRows, FONT_PAGE_SIZE, f->scale, f->scale, gindex);
	cr->bmpDiff.x = (int16_t)c_x1;
	cr->bmpDiff.y = (int16_t)-c_y1;
	cr->advance.x = (uint32_t)roundf(f->scale * advance) << 6;
	cr->advance.y = 0.0f;
#endif
	vec4 uvBounds = { {(float)(penX + f->curLine.penX) / (float)FONT_PAGE_SIZE},
					 {(float)f->curLine.penY / (float)FONT_PAGE_SIZE},
					 {(float)bmpPixelWidth},
					 {(float)bmpRows} };
	cr->bounds = uvBounds;
	cr->pageIdx = f->curLine.pageIdx;

	f->charLookup[gindex] = cr;
	dev->fontCache->stagingX += bmpPixelWidth;
	return cr;
}
void _font_add_name(_vkvg_font_identity_t* font, const char* name) {
	if (++font->namesCount == 1)
		font->names = (char**)malloc(sizeof(char*));
	else
		font->names = (char**)realloc(font->names, font->namesCount * sizeof(char*));
	font->names[font->namesCount - 1] = (char*)calloc(strlen(name) + 1, sizeof(char));
	strcpy(font->names[font->namesCount - 1], name);
}
bool _font_cache_load_font_file_in_memory(_vkvg_font_identity_t* fontId) {
	FILE* fontFile = fopen(fontId->fontFile, "rb");
	if (!fontFile)
		return false;
	fseek(fontFile, 0, SEEK_END);
	fontId->fontBufSize = ftell(fontFile); /* how long is the file ? */
	fseek(fontFile, 0, SEEK_SET);          /* reset */
	fontId->fontBuffer = (unsigned char*)malloc(fontId->fontBufSize);
	fread(fontId->fontBuffer, fontId->fontBufSize, 1, fontFile);
	fclose(fontFile);
	return true;
}
_vkvg_font_identity_t* _font_cache_add_font_identity(VkvgContext ctx, const char* fontFilePath, const char* name) {
	_font_cache_t* cache = (_font_cache_t*)ctx->dev->fontCache;
	if (++cache->fontsCount == 1)
		cache->fonts = (_vkvg_font_identity_t*)malloc(cache->fontsCount * sizeof(_vkvg_font_identity_t));
	else
		cache->fonts =
		(_vkvg_font_identity_t*)realloc(cache->fonts, cache->fontsCount * sizeof(_vkvg_font_identity_t));
	_vkvg_font_identity_t nf = { 0 };

	if (fontFilePath) {
		int fflength = strlen(fontFilePath) + 1;
		nf.fontFile = (char*)malloc(fflength * sizeof(char));
		strcpy(nf.fontFile, fontFilePath);
	}

	_font_add_name(&nf, name);

	cache->fonts[cache->fontsCount - 1] = nf;
	return &cache->fonts[cache->fontsCount - 1];
}
// select current font for context
_vkvg_font_t* _find_or_create_font_size(VkvgContext ctx) {
	_vkvg_font_identity_t* font = ctx->currentFont;

	for (uint32_t i = 0; i < font->sizeCount; ++i) {
		if (font->sizes[i].charSize == ctx->selectedCharSize)
			return &font->sizes[i];
	}
	// if not found, create a new font size structure
	if (++font->sizeCount == 1)
		font->sizes = (_vkvg_font_t*)malloc(sizeof(_vkvg_font_t));
	else
		font->sizes = (_vkvg_font_t*)realloc(font->sizes, font->sizeCount * sizeof(_vkvg_font_t));
	_vkvg_font_t newSize = { }; newSize.charSize = ctx->selectedCharSize;

	VkvgDevice dev = ctx->dev;
#ifdef VKVG_USE_FREETYPE
	_font_cache_t* cache = (_font_cache_t*)ctx->dev->fontCache;
	FT_CHECK_RESULT(FT_New_Memory_Face(cache->library, font->fontBuffer, font->fontBufSize, 0, &newSize.face));
	FT_CHECK_RESULT(FT_Set_Char_Size(newSize.face, 0, newSize.charSize, dev->hdpi, dev->vdpi));

	newSize.charLookup = (_char_ref**)calloc(newSize.face->num_glyphs, sizeof(_char_ref*));

	if (FT_IS_SCALABLE(newSize.face))
		newSize.curLine.height = newSize.face->size->metrics.height >> 6;
	else
		newSize.curLine.height = newSize.face->height >> 6;
#else
	int result = stbtt_InitFont(&font->stbInfo, font->fontBuffer, stbtt_GetFontOffsetForIndex(font->fontBuffer, 0));
	assert(result && "stbtt_initFont failed");
	if (!result) {
		ctx->status = VKVG_STATUS_INVALID_FONT;
		return NULL;
	}
	stbtt_GetFontVMetrics(&font->stbInfo, &font->ascent, &font->descent, &font->lineGap);
	newSize.charLookup = (_char_ref**)calloc(font->stbInfo.numGlyphs, sizeof(_char_ref*));
	// newSize.scale		= stbtt_ScaleForPixelHeight(&font->stbInfo, newSize.charSize);
	newSize.scale = stbtt_ScaleForMappingEmToPixels(&font->stbInfo, newSize.charSize);
	newSize.curLine.height = roundf(newSize.scale * (font->ascent - font->descent + font->lineGap));
	newSize.ascent = roundf(newSize.scale * font->ascent);
	newSize.descent = roundf(newSize.scale * font->descent);
	newSize.lineGap = roundf(newSize.scale * font->lineGap);
#endif

#ifdef VKVG_USE_HARFBUZZ_CX
#ifdef VKVG_USE_FREETYPE
	newSize.hb_font = hb_ft_font_create(newSize.face, NULL);
#else
	int _index = 0;
	hb_blob_t* blob = hb_blob_create((char*)font->fontBuffer, font->fontBufSize, HB_MEMORY_MODE_READONLY, 0, 0);
	if (hb_blob_get_length(blob) > 0)
	{
		hb_face_t* face = hb_face_create(blob, _index);
		newSize.hb_font = hb_font_create(face);
		if (newSize.hb_font == NULL) {
			if (face)hb_face_destroy(face);
		}
	}
	if (blob)hb_blob_destroy(blob);
#endif
#endif

	_init_next_line_in_tex_cache(dev, &newSize);

	font->sizes[font->sizeCount - 1] = newSize;
	return &font->sizes[font->sizeCount - 1];
}

// try find font already resolved with fontconfig by font name
bool _tryFindFontByName(VkvgContext ctx, _vkvg_font_identity_t** font) {
	_font_cache_t* cache = ctx->dev->fontCache;
	for (int i = 0; i < cache->fontsCount; ++i) {
		for (uint32_t j = 0; j < cache->fonts[i].namesCount; j++) {
			if (strcmp(cache->fonts[i].names[j], ctx->selectedFontName) == 0) {
				*font = &cache->fonts[i];
				return true;
			}
		}
	}
	return false;
}

#ifdef VKVG_USE_FONTCONFIG
bool _tryResolveFontNameWithFontConfig(VkvgContext ctx, _vkvg_font_identity_t** resolvedFont) {
	_font_cache_t* cache = (_font_cache_t*)ctx->dev->fontCache;
	char* fontFile = NULL;

	FcPattern* pat = FcNameParse((const FcChar8*)ctx->selectedFontName);
	FcConfigSubstitute(cache->config, pat, FcMatchPattern);
	FcDefaultSubstitute(pat);
	FcResult   result;
	FcPattern* font = FcFontMatch(cache->config, pat, &result);
	if (font)
		FcPatternGetString(font, FC_FILE, 0, (FcChar8**)&fontFile);
	*resolvedFont = NULL;
	if (fontFile) {
		// try find font in cache by path
		for (int i = 0; i < cache->fontsCount; ++i) {
			if (cache->fonts[i].fontFile && strcmp(cache->fonts[i].fontFile, fontFile) == 0) {
				_font_add_name(&cache->fonts[i], ctx->selectedFontName);
				*resolvedFont = &cache->fonts[i];
				break;
			}
		}
		if (!*resolvedFont) {
			// if not found, create a new vkvg_font
			_vkvg_font_identity_t* fid = _font_cache_add_font_identity(ctx, fontFile, ctx->selectedFontName);
			_font_cache_load_font_file_in_memory(fid);
			*resolvedFont = &cache->fonts[cache->fontsCount - 1];
		}
	}

	FcPatternDestroy(pat);
	FcPatternDestroy(font);

	return (fontFile != NULL);
}
#endif

// try to find corresponding font in cache (defined by context selectedFont) and create a new font entry if not found.
void _update_current_font(VkvgContext ctx) {
	if (ctx->currentFont == NULL) {
		LOCK_FONTCACHE(ctx->dev)
			if (ctx->selectedFontName[0] == 0)
				_select_font_face(ctx, "sans");

		if (!_tryFindFontByName(ctx, &ctx->currentFont)) {
#ifdef VKVG_USE_FONTCONFIG
			_tryResolveFontNameWithFontConfig(ctx, &ctx->currentFont);
#else
			LOG(VKVG_LOG_ERR, "Unresolved font: %s\n", ctx->selectedFontName);
			UNLOCK_FONTCACHE(ctx->dev)
				ctx->status = VKVG_STATUS_INVALID_FONT;
			return;
#endif
		}

		ctx->currentFontSize = _find_or_create_font_size(ctx);
		UNLOCK_FONTCACHE(ctx->dev)
	}
}

#ifdef VKVG_USE_HARFBUZZ_CX
// Get harfBuzz buffer for provided text.
hb_buffer_t* _get_hb_buffer(_vkvg_font_t* font, const char* text, int length) {
	hb_buffer_t* buf = hb_buffer_create();

	hb_script_t         script = HB_SCRIPT_LATIN;
	hb_unicode_funcs_t* ucfunc = hb_unicode_funcs_get_default();
	wchar_t             firstChar = 0;
	if (mbstowcs(&firstChar, text, 1))
		script = hb_unicode_script(ucfunc, firstChar);

	hb_direction_t dir = hb_script_get_horizontal_direction(script);
	hb_buffer_set_direction(buf, dir);
	hb_buffer_set_script(buf, script);
	// hb_buffer_set_language	(buf, hb_language_from_string (lng, (int)strlen(lng)));
	hb_buffer_add_utf8(buf, text, length, 0, length);

	hb_shape(font->hb_font, buf, NULL, 0);

	return buf;
}
#endif

// retrieve global font extends of context's current font as defined by FreeType
void _font_cache_font_extents(VkvgContext ctx, vkvg_font_extents_t* extents) {
	_update_current_font(ctx);

	if (ctx->status)
		return;

	// TODO: ensure correct metrics are returned (scalled/unscalled, etc..)
	_vkvg_font_t* font = ctx->currentFontSize;
#ifdef VKVG_USE_FREETYPE
	FT_BBox* bbox = &font->face->bbox;
	FT_Size_Metrics* metrics = &font->face->size->metrics;

	extents->ascent = (float)(FT_MulFix(font->face->ascender, metrics->y_scale) >> 6);   // metrics->ascender >> 6;
	extents->descent = -(float)(FT_MulFix(font->face->descender, metrics->y_scale) >> 6); // metrics->descender >> 6;
	extents->height = (float)(FT_MulFix(font->face->height, metrics->y_scale) >> 6);     // metrics->height >> 6;
	extents->max_x_advance = (float)(bbox->xMax >> 6);
	extents->max_y_advance = (float)(bbox->yMax >> 6);
#else
	extents->ascent = roundf(font->scale * ctx->currentFont->ascent);
	extents->descent = -roundf(font->scale * ctx->currentFont->descent);
	extents->height =
		roundf(font->scale * (ctx->currentFont->ascent - ctx->currentFont->descent + ctx->currentFont->lineGap));
	extents->max_x_advance = 0; // TODO
	extents->max_y_advance = 0;
#endif
}
// compute text extends for provided string.
void _font_cache_text_extents(VkvgContext ctx, const char* text, int length, vkvg_text_extents_t* extents) {
	if (text == NULL) {
		memset(extents, 0, sizeof(vkvg_text_extents_t));
		return;
	}

	vkvg_text_run_t tr = { 0 };
	_font_cache_create_text_run(ctx, text, length, &tr);

	if (ctx->status)
		return;

	*extents = tr.extents;

	_font_cache_destroy_text_run(&tr);
}
// text is expected as utf8 encoded
// if length is < 0, text must be null terminated, else it contains glyph count
void _font_cache_create_text_run(VkvgContext ctx, const char* text, int length, VkvgText textRun) {

	_update_current_font(ctx);

	if (ctx->status)
		return;

	textRun->fontId = ctx->currentFont;
	textRun->font = ctx->currentFontSize;
	textRun->dev = ctx->dev;

	LOCK_FONTCACHE(ctx->dev)

#ifdef VKVG_USE_HARFBUZZ_CX
		textRun->hbBuf = _get_hb_buffer(ctx->currentFontSize, text, length);
	textRun->glyphs = hb_buffer_get_glyph_positions(textRun->hbBuf, &textRun->glyph_count);
#else

		size_t wsize;
	if (length < 0)
		wsize = mbstowcs(NULL, text, 0);
	else
		wsize = (size_t)length;
	wchar_t* tmp = (wchar_t*)malloc((wsize + 1) * sizeof(wchar_t));
	textRun->glyph_count = mbstowcs(tmp, text, wsize);
	textRun->glyphs = (vkvg_glyph_info_t*)malloc(textRun->glyph_count * sizeof(vkvg_glyph_info_t));
	for (unsigned int i = 0; i < textRun->glyph_count; i++) {
#ifdef VKVG_USE_FREETYPE
		uint32_t   gindex = FT_Get_Char_Index(textRun->font->face, tmp[i]);
#else
		uint32_t gindex = stbtt_FindGlyphIndex(&textRun->fontId->stbInfo, tmp[i]);
#endif
		_char_ref* cr = textRun->font->charLookup[gindex];
		if (cr == NULL)
			cr = _prepare_char(textRun->dev, textRun, gindex);
		textRun->glyphs[i].codepoint = gindex;
		textRun->glyphs[i].x_advance = cr->advance.x;
		textRun->glyphs[i].y_advance = cr->advance.y;
		textRun->glyphs[i].x_offset = 0;
		textRun->glyphs[i].y_offset = 0;
		/*textRun->glyphs[i].x_offset	 = cr->bmpDiff.x;
		textRun->glyphs[i].y_offset	 = cr->bmpDiff.y;*/
	}
	free(tmp);
#endif

	UNLOCK_FONTCACHE(ctx->dev)

		unsigned int string_width_in_pixels = 0;
	for (uint32_t i = 0; i < textRun->glyph_count; ++i)
		string_width_in_pixels += textRun->glyphs[i].x_advance >> 6;
#ifdef VKVG_USE_FREETYPE
	FT_Size_Metrics* metrics = &ctx->currentFontSize->face->size->metrics;
	textRun->extents.height = (float)(FT_MulFix(ctx->currentFontSize->face->height, metrics->y_scale) >>
		6); // (metrics->ascender + metrics->descender) >> 6;
#else
	textRun->extents.height = textRun->font->ascent - textRun->font->descent + textRun->font->lineGap;
#endif
	textRun->extents.x_advance = (float)string_width_in_pixels;
	if (textRun->glyph_count > 0) {
		textRun->extents.y_advance = (float)(textRun->glyphs[textRun->glyph_count - 1].y_advance >> 6);
		textRun->extents.x_bearing = -(float)(textRun->glyphs[0].x_offset >> 6);
		textRun->extents.y_bearing = -(float)(textRun->glyphs[0].y_offset >> 6);
	}

	textRun->extents.width = textRun->extents.x_advance;
}
void _font_cache_destroy_text_run(VkvgText textRun) {
#ifdef VKVG_USE_HARFBUZZ_CX
	hb_buffer_destroy(textRun->hbBuf);
#else
	if (textRun->glyph_count > 0)
		free(textRun->glyphs);
#endif
}
#ifdef DEBUG
void _show_texture(vkvg_context* ctx) {
	Vertex vs[] = { {{0, 0}, 0, {0, 0, 0}},
				   {{0, FONT_PAGE_SIZE}, 0, {0, 1, 0}},
				   {{FONT_PAGE_SIZE, 0}, 0, {1, 0, 0}},
				   {{FONT_PAGE_SIZE, FONT_PAGE_SIZE}, 0, {1, 1, 0}} };

	VKVG_IBO_INDEX_TYPE firstIdx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);
	Vertex* pVert = &ctx->vertexCache[ctx->vertCount];
	memcpy(pVert, vs, 4 * sizeof(Vertex));
	ctx->vertCount += 4;

	_check_vertex_cache_size(ctx);

	_add_tri_indices_for_rect(ctx, firstIdx);
}
#endif
void _font_cache_show_text_run(VkvgContext ctx, VkvgText tr) {
	unsigned int glyph_count;
#ifdef VKVG_USE_HARFBUZZ_CX
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(tr->hbBuf, &glyph_count);
#else
	vkvg_glyph_info_t* glyph_info = tr->glyphs;
	glyph_count = tr->glyph_count;
#endif

	Vertex v = { {0}, ctx->curColor, {0, 0, -1} };
	vec2   pen = { 0, 0 };

	if (!_current_path_is_empty(ctx))
		pen = _get_current_position(ctx);

	LOCK_FONTCACHE(ctx->dev)

		for (uint32_t i = 0; i < glyph_count; ++i) {
			_char_ref* cr = tr->font->charLookup[glyph_info[i].codepoint];

#ifdef VKVG_USE_HARFBUZZ_CX
			if (cr == NULL)
				cr = _prepare_char(tr->dev, tr, glyph_info[i].codepoint);
#endif

			float uvWidth = cr->bounds.width / (float)FONT_PAGE_SIZE;
			float uvHeight = cr->bounds.height / (float)FONT_PAGE_SIZE;
			vec2  p0 = { pen.x + cr->bmpDiff.x + (tr->glyphs[i].x_offset >> 6),
							  pen.y - cr->bmpDiff.y + (tr->glyphs[i].y_offset >> 6) };
			v.pos = p0;

			VKVG_IBO_INDEX_TYPE firstIdx = (VKVG_IBO_INDEX_TYPE)(ctx->vertCount - ctx->curVertOffset);

			v.uv.x = cr->bounds.x;
			v.uv.y = cr->bounds.y;
			v.uv.z = cr->pageIdx;
			_add_vertex(ctx, v);

			v.pos.y += cr->bounds.height;
			v.uv.y += uvHeight;
			_add_vertex(ctx, v);

			v.pos.x += cr->bounds.width;
			v.pos.y = p0.y;
			v.uv.x += uvWidth;
			v.uv.y = cr->bounds.y;
			_add_vertex(ctx, v);

			v.pos.y += cr->bounds.height;
			v.uv.y += uvHeight;
			_add_vertex(ctx, v);

			_add_tri_indices_for_rect(ctx, firstIdx);

			pen.x += (tr->glyphs[i].x_advance >> 6);
			pen.y -= (tr->glyphs[i].y_advance >> 6);
		}

	// equivalent to a moveto
	_finish_path(ctx);
	_add_point(ctx, pen.x, pen.y);
	_flush_chars_to_tex(tr->dev, tr->font);
	UNLOCK_FONTCACHE(ctx->dev);
	push_update_descriptor_set_a(ctx, ctx->fontCacheImg, 0);
	if (ctx->fontCacheImg != ctx->dev->fontCache->texture) {
		vkvg_flush(ctx);
		_font_cache_update_context_descset(ctx);
	}
}

void _font_cache_show_text(VkvgContext ctx, const char* text) {

	vkvg_text_run_t tr = { 0 };
	_font_cache_create_text_run(ctx, text, -1, &tr);

	if (ctx->status)
		return;

	_font_cache_show_text_run(ctx, &tr);

	_font_cache_destroy_text_run(&tr);

	//_show_texture(ctx); return;
}


#endif // 1




//layout(set = 0, binding = 0) uniform sampler2DArray fontMap;
//layout(set = 1, binding = 0) uniform sampler2D		source;
//layout(scalar, set = 2, binding = 0) uniform _uboGrad  

void push_update_descriptor_set_a(VkvgContext ctx, VkhImage img, uint64_t offset) {
	auto cx = (vkvg_context*)ctx;
	if (!cx || !cx->maxPushDescriptors || !cx->_vkCmdPushDescriptorSet)return;
	ctx->d_img = img;
	ctx->d_offset = offset;
	if (!ctx->cmdStarted)
	{
		ctx->isdset = true;
		return;
	}
	VkDescriptorImageInfo dst1 = vkh_image_get_descriptor(img ? img : ctx->dev->emptyImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	VkDescriptorImageInfo dst0 = vkh_image_get_descriptor(ctx->fontCacheImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	VkWriteDescriptorSet  wimg = {
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.dstSet = 0,
	.dstBinding = 0,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	.pImageInfo = 0 };
	VkDescriptorBufferInfo dbi = { ctx->uboGrad.buffer, offset, sizeof(vkvg_gradient_t) };
	VkWriteDescriptorSet   wu = {
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.dstSet = 0,
	.dstBinding = 2,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	.pBufferInfo = &dbi };
	VkWriteDescriptorSet  wds[3] = {};
	wimg.pImageInfo = &dst0;
	wds[0] = wimg;
	wimg.dstBinding = 1;
	wimg.pImageInfo = &dst1;
	wds[1] = wimg;
	wds[2] = wu;
	cx->_vkCmdPushDescriptorSet(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelineLayout, 0, 3, wds);
}
#if 0
void push_update_descriptor_set(VkvgContext ctx, VkhImage img, uint32_t idx) {
	auto cx = (vkvg_context*)ctx;
	if (!cx || !cx->maxPushDescriptors || !cx->_vkCmdPushDescriptorSet)return;
	VkDescriptorImageInfo descSrcTex = vkh_image_get_descriptor(img, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	VkWriteDescriptorSet  writeDescriptorSet = {
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.dstSet = 0,
	.dstBinding = idx,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	.pImageInfo = &descSrcTex };
	VkWriteDescriptorSet  wds[3] = {};

	cx->_vkCmdPushDescriptorSet(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelineLayout, 0, 3, wds);
}
//VK_WHOLE_SIZE
void push_update_gradient_desc_set(VkvgContext ctx, uint64_t offset, uint64_t size) {
	auto cx = (vkvg_context*)ctx;
	if (!cx || !cx->maxPushDescriptors || !cx->_vkCmdPushDescriptorSet)return;
	VkDescriptorBufferInfo dbi = { ctx->uboGrad.buffer, offset, size };
	VkWriteDescriptorSet   writeDescriptorSet = {
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.dstSet = 0,
	.dstBinding = 2,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	.pBufferInfo = &dbi };
	cx->_vkCmdPushDescriptorSet(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelineLayout, 0, 1, &writeDescriptorSet);
}
#endif
void update_pattern(VkvgContext ctx, VkvgPattern pat, state_save_t* st) {
	if (!ctx)return;
	VkvgPattern lastPat = ctx->pattern;
	ctx->pattern = pat;
	uint32_t newPatternType = VKVG_PATTERN_TYPE_SOLID;
	if (pat == NULL) {       // solid color
		if (lastPat == NULL) // solid
			return;          // solid to solid transition, no extra action requested
	}
	else
		newPatternType = pat->type;
	switch (newPatternType) {
	case VKVG_PATTERN_TYPE_SOLID:
		push_update_descriptor_set_a(ctx, 0, 0);
		break;
	case VKVG_PATTERN_TYPE_SURFACE:
	{
		VkvgSurface surf = (VkvgSurface)pat->data;
		vkh_cmd_begin(ctx->cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		// transition source surface for sampling
		vkh_image_set_layout(ctx->cmd, surf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		vkh_cmd_end(ctx->cmd);
		VkSamplerAddressMode addrMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkFilter             filter = VK_FILTER_NEAREST;
		switch (pat->extend) {
		case VKVG_EXTEND_NONE:
			addrMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			break;
		case VKVG_EXTEND_PAD:
			addrMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			break;
		case VKVG_EXTEND_REPEAT:
			addrMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			break;
		case VKVG_EXTEND_REFLECT:
			addrMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			break;
		}
		switch (pat->filter) {
		case VKVG_FILTER_BILINEAR:
		case VKVG_FILTER_BEST:
			filter = VK_FILTER_LINEAR;
			break;
		default:
			filter = VK_FILTER_NEAREST;
			break;
		}
		vkh_image_create_sampler(surf->img, filter, filter, VK_SAMPLER_MIPMAP_MODE_NEAREST, addrMode);
		push_update_descriptor_set_a(ctx, surf->img, 0);
		st->pushConsts.source.width = (float)surf->width;
		st->pushConsts.source.height = (float)surf->height;
		vkvg_matrix_t mat;
		if (pat->hasMatrix) {
			vkvg_pattern_get_matrix(pat, &mat);
			vkvg_matrix_multiply(&st->pushConsts.matInv, &st->pushConsts.matInv, &mat);
		}
	}
	break;
	case VKVG_PATTERN_TYPE_LINEAR:
	case VKVG_PATTERN_TYPE_RADIAL:
	case VKVG_PATTERN_TYPE_SWEEP:
	{
		float fm = std::max((float)ctx->pSurf->width, (float)ctx->pSurf->height);
		vec4 bounds = { fm,fm,0.0f,0.0f }; // store img bounds in unused source field
		st->pushConsts.source = bounds;
		// transform control point with current ctx matrix 
		vkvg_gradient_t grad = *(vkvg_gradient_t*)pat->data;
		if (grad.count < 2) {
			ctx->status = VKVG_STATUS_PATTERN_INVALID_GRADIENT;
			return;
		}
		grad.extend = pat->extend;
		vkvg_matrix_t mat;
		if (pat->hasMatrix) {
			vkvg_pattern_get_matrix(pat, &mat);
			if (vkvg_matrix_invert(&mat) != VKVG_STATUS_SUCCESS)
				mat = VKVG_IDENTITY_MATRIX;
			vkvg_matrix_transform_point(&mat, &grad.cp[0].x, &grad.cp[0].y);
		}
		vkvg_matrix_transform_point(&st->pushConsts.mat, &grad.cp[0].x, &grad.cp[0].y);
		if (pat->type == VKVG_PATTERN_TYPE_LINEAR) {
			if (pat->hasMatrix)
				vkvg_matrix_transform_point(&mat, &grad.cp[0].z, &grad.cp[0].w);
			vkvg_matrix_transform_point(&st->pushConsts.mat, &grad.cp[0].z, &grad.cp[0].w);
		}
		else {
			if (pat->hasMatrix)
				vkvg_matrix_transform_point(&mat, &grad.cp[1].x, &grad.cp[1].y);
			vkvg_matrix_transform_point(&st->pushConsts.mat, &grad.cp[1].x, &grad.cp[1].y);
			// radii
			if (pat->hasMatrix) {
				vkvg_matrix_transform_distance(&mat, &grad.cp[0].z, &grad.cp[0].w);
				vkvg_matrix_transform_distance(&mat, &grad.cp[1].z, &grad.cp[0].w);
			}
			vkvg_matrix_transform_distance(&st->pushConsts.mat, &grad.cp[0].z, &grad.cp[0].w);
			vkvg_matrix_transform_distance(&st->pushConsts.mat, &grad.cp[1].z, &grad.cp[0].w);
		}
		_sort_gradient_stops(grad.colors, grad.stops, grad.count);
		memcpy(((char*)vkh_buffer_get_mapped_pointer(&ctx->uboGrad)) + ctx->gxCount, &grad, sizeof(vkvg_gradient_t));
		//vkh_buffer_flush(&ctx->uboGrad); 
		push_update_descriptor_set_a(ctx, 0, ctx->gxCount);
		ctx->gxCount += sizeof(vkvg_gradient_t);
	}
	break;
	}
	st->pushConsts.fsq_patternType = (st->pushConsts.fsq_patternType & FULLSCREEN_BIT) + newPatternType;
	ctx->pushCstDirty = true;
	//if (lastPat)
	//	vkvg_pattern_destroy(lastPat);
}
#if 1

bool a_path_has_curves(uint32_t* pathes, uint32_t ptrPath) { return   pathes[ptrPath] & PATH_HAS_CURVES_BIT; }

#define VG_COL32_A_MASK     0xFF000000

void NORMALIZE2F_OVER_ZERO(float& VX, float& VY)
{
	float d2 = VX * VX + VY * VY;
	if (d2 > 0.0f) {
		float inv_len = 1.0f / sqrtf(d2);
		VX *= inv_len; VY *= inv_len;
	}
}
#define FIXNORMAL2F_MAX_INVLEN2          100.0f // 500.0f (see #4053, #3366)
void FIXNORMAL2F(float& VX, float& VY)
{
	float d2 = VX * VX + VY * VY;
	if (d2 > 0.000001f) {
		float inv_len2 = 1.0f / d2;
		if (inv_len2 > FIXNORMAL2F_MAX_INVLEN2)
			inv_len2 = FIXNORMAL2F_MAX_INVLEN2;
		VX *= inv_len2; VY *= inv_len2;
	}
}


paths_t* dc_new_paths(vgdev_ctx* ctx);
void dc_free_paths(paths_t* p);

vgdev_ctx::vgdev_ctx()
{
	_vertex.ac = _indices.ac = ecpsd.ac = _normals.ac = &ac;
	_vertex.reserve(128);
	_indices.reserve(1024);
	_normals.reserve(128);
	ecpsd.reserve(128);
	cmdlist.reserve(128);
	_dpath = dc_new_paths(this);
}

vgdev_ctx::~vgdev_ctx()
{
	dc_free_paths(_dpath); _dpath = 0;
}

void add_vertexf_unchecked(Vertex* pVert, float x, float y, uint32_t c)
{
	*pVert = {};
	pVert->pos.x = x;
	pVert->pos.y = y;
	pVert->color = c;
	pVert->uv.z = -1;
}
void vgdev_ctx::clip_preserve(paths_t* ctx)
{
	if (!ctx->pathPtr) // nothing to clip
		return;
	ctx->t = t;

	cmd_t c = {};
	c.type = 2;
	if (t->curFillRule == VKVG_FILL_RULE_EVEN_ODD) {
		poly_fill(ctx, NULL, c);
	}
	else {
		c.vertex.x = _vertex.size();
		c.index.x = _indices.size();
		cp_cmdt(&c, t);
		fill_non_zero(ctx);
		c.vertex.y = _vertex.size() - c.vertex.x;
		c.index.y = _indices.size() - c.index.x;
		cmdlist.push_back(c);
	}
	cmdlist.back().full_screen_quad = _vertex.size();
	Vertex v = {};
	v.pos = { -1,-1 };
	v.color = t->curColor;
	v.uv.z = -1;
	_vertex.push_back(v);
	v.pos = { 3,-1 };
	_vertex.push_back(v);
	v.pos = { -1,3 };
	_vertex.push_back(v);
}
void vgdev_ctx::clip0()
{
	cmd_t c = {};
	c.type = 2;
	c.full_screen_quad = _vertex.size();
	Vertex v = {};
	v.pos = { -1,-1 };
	v.color = -1;
	v.uv.z = -1;
	_vertex.push_back(v);
	v.pos = { 3,-1 };
	_vertex.push_back(v);
	v.pos = { -1,3 };
	_vertex.push_back(v);
	cmdlist.push_back(c);
}


#ifdef VKVG_FILL_NZ_GLUTESS

void a_set_vertex(vgdev_ctx* ctx, uint32_t idx, Vertex v) { ctx->_vertex[idx] = v; }

void _add_indicea(vgdev_ctx* ctx, VKVG_IBO_INDEX_TYPE i) {
	ctx->_indices.push_back(i);
}
void _add_indice_for_fana(vgdev_ctx* ctx, VKVG_IBO_INDEX_TYPE i) {
	uint32_t inds[3] = { ctx->tesselator_fan_start, ctx->_indices.back(),i };
	ctx->_indices.insert(-1, inds, 3);
}
void _add_indice_for_stripa(vgdev_ctx* ctx, VKVG_IBO_INDEX_TYPE i, bool odd) {
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
	ctx->_indices.insert(-1, inds, 3);
}

void fan_vertex2a(uint32_t v, vgdev_ctx* ctx) {
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
void strip_vertex2a(uint32_t v, vgdev_ctx* ctx) {
	uint32_t i = (uint32_t)v;
	if (ctx->tesselator_idx_counter < 3) {
		_add_indicea(ctx, i);
	}
	else
		_add_indice_for_stripa(ctx, i, ctx->tesselator_idx_counter % 2);
	ctx->tesselator_idx_counter++;
}
void triangle_vertex2a(uint32_t v, vgdev_ctx* ctx) {
	uint32_t i = (uint32_t)v;
	_add_indicea(ctx, i);
}
void skip_vertex2a(uint32_t v, vgdev_ctx* ctx) {}
void begin2a(GLenum which, void* poly_data) {
	vgdev_ctx* ctx = (vgdev_ctx*)poly_data;
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
	vgdev_ctx* ctx = (vgdev_ctx*)poly_data;
	Vertex      v = { {newVertex[0], newVertex[1]}, ctx->curColor, {0, 0, -1} };
	*outData = (void*)(ctx->_vertex.size() - ctx->curVertOffset);
	ctx->_vertex.push_back(v);
}
void vertex2a(void* vertex_data, void* poly_data) {
	uint32_t i = (uint32_t)vertex_data;
	vgdev_ctx* ctx = (vgdev_ctx*)poly_data;
	ctx->vertex_cb(i, ctx);
}
void vgdev_ctx::_fill_non_zero(paths_t* ctx)
{
	Vertex v = { {0}, ctx->curColor, {0, 0, -1} };
	curColor = ctx->curColor;
	uint32_t ptrPath = 0;
	uint32_t firstPtIdx = 0;
	curVertOffset = _vertex.size();
	if (ctx->pathPtr == 1 && ctx->pathes[0] & PATH_IS_CONVEX_BIT) {
		// simple concave rectangle or circle
		uint32_t firstVertIdx = (uint32_t)(_vertex.size() - ctx->curVertOffset);
		uint32_t            pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;

		//_ensure_vertex_cache_size(ctx, pathPointCount);
		//_ensure_index_cache_size(ctx, (pathPointCount - 2) * 3);

		uint32_t i = 0;
		while (i < 2) {
			v.pos = ctx->points[i++];
			_vertex.push_back(v);
		}
		while (i < pathPointCount) {
			v.pos = ctx->points[i];
			_vertex.push_back(v);
			uint32_t ind[3] = { firstVertIdx, firstVertIdx + i - 1, firstVertIdx + i };
			_indices.insert(-1, ind, 3);
			i++;
		}
		return;
	}

	GLUtesselator* tess = gluNewTess();
	gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
	gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (GLvoid(*)()) & vertex2a);
	gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (GLvoid(*)()) & begin2a);
	gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (GLvoid(*)()) & combine2a);

	gluTessBeginPolygon(tess, this);
	while (ptrPath < ctx->pathPtr) {
		uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;

		if (pathPointCount > 2) {
			uint32_t firstVertIdx = (uint32_t)(_vertex.size() - ctx->curVertOffset);
			gluTessBeginContour(tess);
			uint32_t i = 0;

			while (i < pathPointCount) {
				v.pos = ctx->points[i + firstPtIdx];
				double dp[] = { v.pos.x, v.pos.y, 0 };
				_vertex.push_back(v);
				gluTessVertex(tess, dp, (void*)((unsigned long)firstVertIdx + i));
				i++;
			}
			gluTessEndContour(tess);
		}

		firstPtIdx += pathPointCount;
		if (a_path_has_curves(ctx->pathes, ptrPath)) {
			// skip segments lengths used in stroke
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
#endif

void vgdev_ctx::fill_non_zero(paths_t* p)
{
	auto t = p->t;
	uint32_t color = t->curColor;
	p->curColor = color;
	if (is_glutess)
	{
		_fill_non_zero(p);
		return;
	}
	uint32_t ptrPath = 0;
	uint32_t firstPtIdx = 0;
	const vec3 uv = { 0,0,-1 };
	bool aa = t->aa;
	Vertex v = { {0}, color, {0, 0, -1} };
	uint32_t cur_idx = _vertex.size() - p->curVertOffset;
	while (ptrPath < p->pathPtr) {
		uint32_t pathPointCount = p->pathes[ptrPath] & PATH_ELT_MASK;
		auto col = p->color ? p->color[ptrPath] : color;
		v.color = col;
		if (pathPointCount > 2) {
			uint32_t firstVertIdx = (uint32_t)cur_idx;
			ecpsd.resize(pathPointCount);
			auto ecps = ecpsd.data();
			if (!ecps)break;
			uint32_t            ecps_count = pathPointCount;
			uint32_t i = 0;
			auto points = p->points + firstPtIdx;
			// init points link list
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
					_indices.insert(-1, t3, t3 + 3);
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
				_indices.insert(-1, t3, t3 + 3);
			}
			// Anti-aliased Fill
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
				vec2* temp_normals = _normals.data();
				for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
				{
					const vec2& p0 = points[i0];
					const vec2& p1 = points[i1];
					float dx = p1.x - p0.x;
					float dy = p1.y - p0.y;
					NORMALIZE2F_OVER_ZERO(dx, dy);
					temp_normals[i0].x = dy;
					temp_normals[i0].y = -dx;
				}

				for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
				{
					// Average normals
					const vec2& n0 = temp_normals[i0];
					const vec2& n1 = temp_normals[i1];
					float dm_x = (n0.x + n1.x) * 0.5f;
					float dm_y = (n0.y + n1.y) * 0.5f;
					FIXNORMAL2F(dm_x, dm_y);
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
		if (a_path_has_curves(p->pathes, ptrPath)) {
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
// Even-Odd inside test with stencil buffer implementation.
void vgdev_ctx::poly_fill(paths_t* ctx, vec4* bounds, cmd_t& c) {
	Vertex   v = { {0}, ctx->t->curColor, {0, 0, -1} };
	uint32_t ptrPath = 0;
	uint32_t firstPtIdx = 0;
	size_t nc = 0;
	while (ptrPath < ctx->pathPtr) {
		uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;
		if (pathPointCount > 2) {
			nc++;
		}
		if (a_path_has_curves(ctx->pathes, ptrPath)) {
			// skip segments lengths used in stroke
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
				// bounds are computed here to scissor the painting operation
				// that speed up fill drastically.
				vkvg_matrix_transform_point(&ctx->t->pushConsts.mat, &v.pos.x, &v.pos.y);
				if (v.pos.x < bounds->xMin)
					bounds->xMin = v.pos.x;
				if (v.pos.x > bounds->xMax)
					bounds->xMax = v.pos.x;
				if (v.pos.y < bounds->yMin)
					bounds->yMin = v.pos.y;
				if (v.pos.y > bounds->yMax)
					bounds->yMax = v.pos.y;
			}
			cv->firstVertex = firstVertIdx;
			cv->vertexCount = pathPointCount;
			cv++;
		}
		firstPtIdx += pathPointCount;

		if (a_path_has_curves(ctx->pathes, ptrPath)) {
			// skip segments lengths used in stroke
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
	cmdlist.push_back(c);
}
void vgdev_ctx::cp_cmdt(cmd_t* c, state_save_t* t)
{
	c->state = (state_save_t*)mac.allocate(sizeof(state_save_t) * 1);
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
void vgdev_ctx::fill_preserve(paths_t* ctx)
{
	if (!ctx || !ctx->pathPtr || !t) // nothing to fill
		return;
	ctx->t = t;
	if (t->pattern)
		gCount++;
	if (ctx->t->curFillRule == VKVG_FILL_RULE_EVEN_ODD) {

		vec4 bounds = { FLT_MAX, FLT_MAX, FLT_MIN, FLT_MIN };
		cmd_t c = {};
		c.type = 0;
		poly_fill(ctx, &bounds, c);
		cmdlist.back().full_screen_quad = _vertex.size();
		Vertex v = {};
		v.pos = { -1,-1 };
		v.color = t->curColor;
		v.uv.z = -1;
		_vertex.push_back(v);
		v.pos = { 3,-1 };
		_vertex.push_back(v);
		v.pos = { -1,3 };
		_vertex.push_back(v);
	}
	else
	{
		cmd_t c = {};
		c.vertex.x = _vertex.size();
		c.index.x = _indices.size();
		c.type = 0;
		cp_cmdt(&c, t);
		ctx->curVertOffset = c.vertex.x;
		fill_non_zero(ctx);
		c.vertex.y = _vertex.size() - c.vertex.x;
		c.index.y = _indices.size() - c.index.x;
		cmdlist.push_back(c);
	}
}


bool a_path_is_closed(paths_t* ctx, uint32_t ptrPath) { return ctx->pathes[ptrPath] & PATH_CLOSED_BIT; }
void vgdev_ctx::a_add_triangle_indices(paths_t* ctx, uint32_t i0, uint32_t i1, uint32_t i2) {
	_indices.push_back(i0);
	_indices.push_back(i1);
	_indices.push_back(i2);
}
void vgdev_ctx::a_add_tri_indices_for_rect(uint32_t i) {
	_indices.resize(_indices.size() + 6);
	uint32_t* inds = _indices.data() + _indices.size() - 6;
	inds[0] = i;
	inds[1] = i + 2;
	inds[2] = i + 1;
	inds[3] = i + 1;
	inds[4] = i + 2;
	inds[5] = i + 3;
}
void vgdev_ctx::_add_vertexf(paths_t* ctx, float x, float y) {
	Vertex v = {};
	v.pos = { x,y };
	v.color = ctx->curColor;
	v.uv.z = -1;
	_vertex.push_back(v);
}
float a_get_arc_step(paths_t* ctx, float radius) {
	float sx = 1.0f, sy = 1.0f;
	vkvg_matrix_get_scale(&ctx->t->pushConsts.mat, &sx, &sy);
	float r = radius * fabsf(fmaxf(sx, sy));
	if (r < 30.0f)
		return fminf(M_PIF / 3.f, M_PIF / r);
	return fminf(M_PIF / 3.f, M_PIF / (r * 0.4f));
}
bool vgdev_ctx::_build_vb_step(paths_t* ctx, stroke_context_t* str, bool isCurve) {
	Vertex v = { {0}, ctx->curColor, {0, 0, -1} };
	vec2   p0 = ctx->points[str->cp];
	vec2   v0 = vec2_sub(p0, ctx->points[str->iL]);
	vec2   v1 = vec2_sub(ctx->points[str->iR], p0);
	float  length_v0 = vec2_length(v0);
	float  length_v1 = vec2_length(v1);
	if (length_v0 < FLT_EPSILON || length_v1 < FLT_EPSILON) {
		LOG(VKVG_LOG_STROKE, "vb_step discard, length<epsilon: l0:%f l1:%f\n", length_v0, length_v1);
		return false;
	}
	vec2  v0n = vec2_div_s(v0, length_v0);
	vec2  v1n = vec2_div_s(v1, length_v1);
	float dot = vec2_dot(v0n, v1n);
	float det = v0n.x * v1n.y - v0n.y * v1n.x;
	if (EQUF(dot, 1.0f)) { // colinear
		LOG(VKVG_LOG_STROKE, "vb_step discard, dot==1\n");
		return false;
	}
	if (EQUF(dot, -1.0f)) { // cusp (could draw line butt?)
		vec2 vPerp = vec2_mult_s(vec2_perp(v0n), str->hw);
		uint32_t idx = (uint32_t)(_vertex.size() - ctx->curVertOffset);
		v.pos = vec2_add(p0, vPerp);
		_vertex.push_back(v);
		v.pos = vec2_sub(p0, vPerp);
		_vertex.push_back(v);
		a_add_triangle_indices(ctx, idx, idx + 1, idx + 2);
		a_add_triangle_indices(ctx, idx, idx + 2, idx + 3);
		LOG(VKVG_LOG_STROKE, "vb_step cusp, dot==-1\n");
		return true;
	}
	vec2  bisec_n = vec2_norm(vec2_add(v0n, v1n)); // bisec/bisec_perp are inverted names
	float alpha = acosf(dot);

	if (det < 0)
		alpha = -alpha;
	float halfAlpha = alpha / 2.f;
	float cosHalfAlpha = cosf(halfAlpha);
	float lh = str->hw / cosHalfAlpha;
	vec2  bisec_n_perp = vec2_perp(bisec_n);
	// limit bisectrice length
	float rlh = lh; // rlh is for inside pos tweeks
	if (dot < 0.f)
		rlh = fminf(rlh, fminf(length_v0, length_v1));
	//---

	vec2 bisec = vec2_mult_s(bisec_n_perp, rlh);

	uint32_t idx = (uint32_t)(_vertex.size() - ctx->curVertOffset);

	vec2 rlh_inside_pos, rlh_outside_pos;
	if (rlh < lh) {
		vec2 vnPerp;
		if (length_v0 < length_v1)
			vnPerp = vec2_perp(v1n);
		else
			vnPerp = vec2_perp(v0n);
		vec2 vHwPerp = vec2_mult_s(vnPerp, str->hw);

		double lbc = cosHalfAlpha * rlh;
		if (det < 0.f) {
			rlh_inside_pos = vec2_add(vec2_add(vec2_mult_s(vnPerp, -lbc), vec2_add(p0, bisec)), vHwPerp);
			rlh_outside_pos = vec2_sub(p0, vec2_mult_s(bisec_n_perp, lh));
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

	vkvg_line_join_t join = ctx->t->lineJoin;

	if (isCurve) {
		if (dot < 0.8f)
			join = VKVG_LINE_JOIN_ROUND;
		else
			join = VKVG_LINE_JOIN_MITER;
	}

	if (join == VKVG_LINE_JOIN_MITER) {
		if (lh > str->lhMax) { // miter limit
			double x = (lh - str->lhMax) * cosHalfAlpha;
			vec2   bisecPerp = vec2_mult_s(bisec_n, x);
			bisec = vec2_mult_s(bisec_n_perp, str->lhMax);
			if (det < 0) {
				v.pos = rlh_inside_pos;
				_vertex.push_back(v);

				vec2 p = vec2_sub(p0, bisec);

				v.pos = vec2_sub(p, bisecPerp);
				_vertex.push_back(v);
				v.pos = vec2_add(p, bisecPerp);
				_vertex.push_back(v);

				a_add_triangle_indices(ctx, idx, idx + 2, idx + 1);
				a_add_triangle_indices(ctx, idx + 2, idx + 4, idx);
				a_add_triangle_indices(ctx, idx, idx + 3, idx + 4);
				return true;
			}
			else {
				vec2 p = vec2_add(p0, bisec);
				v.pos = vec2_sub(p, bisecPerp);
				_vertex.push_back(v);

				v.pos = rlh_inside_pos;
				_vertex.push_back(v);

				v.pos = vec2_add(p, bisecPerp);
				_vertex.push_back(v);

				a_add_triangle_indices(ctx, idx, idx + 2, idx + 1);
				a_add_triangle_indices(ctx, idx + 2, idx + 3, idx + 1);
				a_add_triangle_indices(ctx, idx + 1, idx + 3, idx + 4);
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

			a_add_tri_indices_for_rect(idx);
			return false;
		}
	}
	else {
		vec2 vp = vec2_perp(v0n);

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

		if (join == VKVG_LINE_JOIN_BEVEL) {
			if (det < 0) {
				a_add_triangle_indices(ctx, idx, idx + 2, idx + 1);
				a_add_triangle_indices(ctx, idx + 2, idx + 4, idx + 0);
				a_add_triangle_indices(ctx, idx, idx + 3, idx + 4);
			}
			else {
				a_add_triangle_indices(ctx, idx, idx + 2, idx + 1);
				a_add_triangle_indices(ctx, idx + 2, idx + 3, idx + 1);
				a_add_triangle_indices(ctx, idx + 1, idx + 3, idx + 4);
			}
		}
		else if (join == VKVG_LINE_JOIN_ROUND) {
			if (!str->arcStep)
				str->arcStep = a_get_arc_step(ctx, str->hw);
			float a = acosf(vp.x);
			if (vp.y < 0)
				a = -a;

			if (det < 0) {
				a += M_PIF;
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
			a_add_triangle_indices(ctx, idx, idx + 2, idx + 1);
			if (det < 0) {
				for (uint32_t p = idx + 2; p < p0Idx; p++)
					a_add_triangle_indices(ctx, p, p + 1, idx);
				a_add_triangle_indices(ctx, p0Idx, p0Idx + 2, idx);
				a_add_triangle_indices(ctx, idx, p0Idx + 1, p0Idx + 2);
			}
			else {
				for (uint32_t p = idx + 2; p < p0Idx; p++)
					a_add_triangle_indices(ctx, p, p + 1, idx + 1);
				a_add_triangle_indices(ctx, p0Idx, p0Idx + 1, idx + 1);
				a_add_triangle_indices(ctx, idx + 1, p0Idx + 1, p0Idx + 2);
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

void vgdev_ctx::_draw_stoke_cap(paths_t* ctx, stroke_context_t* str, vec2 p0, vec2 n, bool isStart) {
	Vertex v = { {0}, ctx->curColor, {0, 0, -1} };

	uint32_t firstIdx = (uint32_t)(_vertex.size() - ctx->curVertOffset);

	if (isStart) {
		vec2 vhw = vec2_mult_s(n, str->hw);

		if (ctx->t->lineCap == VKVG_LINE_CAP_SQUARE)
			p0 = vec2_sub(p0, vhw);

		vhw = vec2_perp(vhw);

		if (ctx->t->lineCap == VKVG_LINE_CAP_ROUND) {
			if (!str->arcStep)
				str->arcStep = a_get_arc_step(ctx, str->hw);

			float a = acosf(n.x) + M_PIF_2;
			if (n.y < 0)
				a = M_PIF - a;
			float a1 = a + M_PIF;

			a += str->arcStep;
			while (a < a1) {
				_add_vertexf(ctx, cosf(a) * str->hw + p0.x, sinf(a) * str->hw + p0.y);
				a += str->arcStep;
			}
			uint32_t p0Idx = (uint32_t)(_vertex.size() - ctx->curVertOffset);
			for (uint32_t p = firstIdx; p < p0Idx; p++)
				a_add_triangle_indices(ctx, p0Idx + 1, p, p + 1);
			firstIdx = p0Idx;
		}

		v.pos = vec2_add(p0, vhw);
		_vertex.push_back(v);
		v.pos = vec2_sub(p0, vhw);
		_vertex.push_back(v);

		a_add_tri_indices_for_rect(firstIdx);
	}
	else {
		vec2 vhw = vec2_mult_s(n, str->hw);

		if (ctx->t->lineCap == VKVG_LINE_CAP_SQUARE)
			p0 = vec2_add(p0, vhw);

		vhw = vec2_perp(vhw);

		v.pos = vec2_add(p0, vhw);
		_vertex.push_back(v);
		v.pos = vec2_sub(p0, vhw);
		_vertex.push_back(v);

		firstIdx = (uint32_t)(_vertex.size() - ctx->curVertOffset);

		if (ctx->t->lineCap == VKVG_LINE_CAP_ROUND) {
			if (!str->arcStep)
				str->arcStep = a_get_arc_step(ctx, str->hw);

			float a = acosf(n.x) + M_PIF_2;
			if (n.y < 0)
				a = M_PIF - a;
			float a1 = a - M_PIF;

			a -= str->arcStep;
			while (a > a1) {
				_add_vertexf(ctx, cosf(a) * str->hw + p0.x, sinf(a) * str->hw + p0.y);
				a -= str->arcStep;
			}

			uint32_t p0Idx = (uint32_t)(_vertex.size() - ctx->curVertOffset);
			for (uint32_t p = firstIdx - 1; p < p0Idx; p++)
				a_add_triangle_indices(ctx, p + 1, p, firstIdx - 2);
		}
	}
}
float vgdev_ctx::_draw_dashed_segment(paths_t* ctx, stroke_context_t* str, dash_context_t* dc, bool isCurve) {
	// vec2 pL = ctx->points[str->iL];
	vec2 p = ctx->points[str->cp];
	vec2 pR = ctx->points[str->iR];

	if (!dc->dashOn) // we test in fact the next dash start, if dashOn = true => next segment is a void.
		_build_vb_step(ctx, str, isCurve);

	vec2 d = vec2_sub(pR, p);
	dc->normal = vec2_norm(d);
	float segmentLength = vec2_length(d);

	while (dc->curDashOffset < segmentLength) {
		vec2 p0 = vec2_add(p, vec2_mult_s(dc->normal, dc->curDashOffset));

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
void vgdev_ctx::_draw_segment(paths_t* ctx, stroke_context_t* str, dash_context_t* dc, bool isCurve) {
	str->iR = str->cp + 1;
	if (ctx->t->dashCount > 0)
		_draw_dashed_segment(ctx, str, dc, isCurve);
	else
		_build_vb_step(ctx, str, isCurve);
	str->iL = str->cp++;
}

void vgdev_ctx::stroke_preserve(paths_t* ctx) {
	if (!ctx || !ctx->pathPtr || !t) // nothing to stroke
		return;
	ctx->t = t;

	if (t->pattern)
		gCount++;

	cmd_t c = {};
	c.vertex.x = _vertex.size();
	c.index.x = _indices.size();
	c.type = 1;
	cp_cmdt(&c, t);
	ctx->curVertOffset = c.vertex.x;
	stroke_context_t str = { 0 };
	str.hw = t->lineWidth * 0.5f;
	str.lhMax = t->miterLimit * t->lineWidth;
	uint32_t ptrPath = 0;
	ctx->curColor = t->curColor;
	while (ptrPath < ctx->pathPtr) {
		uint32_t ptrSegment = 0, lastSegmentPointIdx = 0;
		uint32_t firstPathPointIdx = str.cp;
		uint32_t pathPointCount = ctx->pathes[ptrPath] & PATH_ELT_MASK;
		uint32_t lastPathPointIdx = str.cp + pathPointCount - 1;

		dash_context_t dc = { 0 };

		if (a_path_has_curves(ctx->pathes, ptrPath)) {
			ptrSegment = 1;
			lastSegmentPointIdx = str.cp + (ctx->pathes[ptrPath + ptrSegment] & PATH_ELT_MASK) - 1;
		}

		str.firstIdx = (uint32_t)_vertex.size() - ctx->curVertOffset;

		// LOG(VKVG_LOG_INFO_PATH, "\tPATH: points count=%10d end point idx=%10d", ctx->pathes[ptrPath]&PATH_ELT_MASK,
		// lastPathPointIdx);

		if (t->dashCount > 0) {
			// init dash stroke
			dc.dashOn = true;
			dc.curDash = 0; // current dash index
			dc.totDashLength = 0; // limit offset to total length of dashes
			for (uint32_t i = 0; i < t->dashCount; i++)
				dc.totDashLength += t->dashes[i];
			if (dc.totDashLength == 0) {
				//ctx->status = VKVG_STATUS_INVALID_DASH;
				break;
				//return;
			}
			dc.curDashOffset = fmodf(
				t->dashOffset,
				dc.totDashLength); // cur dash offset between defined path point and last dash segment(on/off) start
			str.iL = lastPathPointIdx;
		}
		else if (a_path_is_closed(ctx, ptrPath)) {
			str.iL = lastPathPointIdx;
		}
		else {
			_draw_stoke_cap(ctx, &str, ctx->points[str.cp],
				vec2_line_norm(ctx->points[str.cp], ctx->points[str.cp + 1]), true);
			str.iL = str.cp++;
		}

		if (a_path_has_curves(ctx->pathes, ptrPath)) {
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

		if (t->dashCount > 0) {
			if (a_path_is_closed(ctx, ptrPath)) {
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
					dc.curDash = t->dashCount - 1;
				float m = fminf(t->dashes[prevDash] - dc.curDashOffset, t->dashes[dc.curDash]);
				vec2  p2 = vec2_sub(ctx->points[str.iR], vec2_mult_s(dc.normal, m));
				_draw_stoke_cap(ctx, &str, p2, dc.normal, false);
			}
		}
		else if (a_path_is_closed(ctx, ptrPath)) {
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
	cmdlist.push_back(c);
}

void bind_draw_pipeline(VkvgContext ctx, state_save_t* t) {
	switch (t->curOperator) {
	case VKVG_OPERATOR_OVER:
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipe_OVER);
		break;
	case VKVG_OPERATOR_CLEAR:
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipe_CLEAR);
		break;
	case VKVG_OPERATOR_DIFFERENCE:
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipe_SUB);
		break;
	default:
		CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipe_OVER);
		break;
	}
}

void cmd_draw_full_screen_quad(VkvgContext ctx, vgdev_ctx::cmd_t* c, vec4* scissor)
{
#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_cmd_label_start(ctx->cmd, "_draw_full_screen_quad", DBG_LAB_COLOR_FSQ);
#endif
	if (scissor) {
		VkRect2D r = { {(int32_t)MAX(scissor->xMin, 0), (int32_t)MAX(scissor->yMin, 0)},
					  {(int32_t)MAX(scissor->xMax - (int32_t)scissor->xMin + 1, 1),
					   (int32_t)MAX(scissor->yMax - (int32_t)scissor->yMin + 1, 1)} };
		CmdSetScissor(ctx->cmd, 0, 1, &r);
	}

	uint32_t firstVertIdx = c->full_screen_quad;
	uint32_t fsq_patternType = ctx->pushConsts.fsq_patternType;
	if (c->state)
		fsq_patternType = c->state->pushConsts.fsq_patternType;
	fsq_patternType |= FULLSCREEN_BIT;
	CmdPushConstants(ctx->cmd, ctx->dev->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 24, 4, &fsq_patternType);
	CmdDraw(ctx->cmd, 3, 1, firstVertIdx, 0);
	fsq_patternType &= ~FULLSCREEN_BIT;
	CmdPushConstants(ctx->cmd, ctx->dev->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 24, 4, &fsq_patternType);
	if (scissor)
		CmdSetScissor(ctx->cmd, 0, 1, &ctx->bounds);

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_cmd_label_end(ctx->cmd);
#endif
}
void resize_vbo(VkvgContext ctx, uint32_t new_size) {

	LOG(VKVG_LOG_DBG_ARRAYS, "resize VBO: %d -> ", ctx->sizeVBO);
	ctx->sizeVBO = new_size;
	uint32_t mod = ctx->sizeVBO % VKVG_VBO_SIZE;
	if (mod > 0)
		ctx->sizeVBO += VKVG_VBO_SIZE - mod;
	LOG(VKVG_LOG_DBG_ARRAYS, "%d\n", ctx->sizeVBO);
	vkh_buffer_resize(&ctx->vertices, ctx->sizeVBO * sizeof(Vertex), true);
}
void resize_ibo(VkvgContext ctx, size_t new_size) {

	ctx->sizeIBO = new_size;
	uint32_t mod = ctx->sizeIBO % VKVG_IBO_SIZE;
	if (mod > 0)
		ctx->sizeIBO += VKVG_IBO_SIZE - mod;
	LOG(VKVG_LOG_DBG_ARRAYS, "resize IBO: new size: %d\n", ctx->sizeIBO);
	vkh_buffer_resize(&ctx->indices, ctx->sizeIBO * sizeof(VKVG_IBO_INDEX_TYPE), true);
}
void check_vao_size(VkvgContext ctx, size_t vertCount, size_t indCount) {
	if (vertCount > ctx->sizeVBO || indCount > ctx->sizeIBO) {
		if (vertCount > ctx->sizeVBO)
			resize_vbo(ctx, vertCount);
		if (indCount > ctx->sizeIBO)
			resize_ibo(ctx, indCount);
	}
}

void dc_explicit_ms_resolve(VkCommandBuffer cmd, VkvgSurface surf) {
	LOCK_SURFACE(surf);
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

	UNLOCK_SURFACE(surf)
}

void dc_explicit_shader_read(VkCommandBuffer cmd, VkvgSurface surf) {
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = surf->img->image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(
		cmd,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void dc_start_cmd_for_render_pass(VkvgContext ctx) {
	LOG(VKVG_LOG_INFO, "START RENDER PASS: ctx = %p\n", ctx);
	vkh_cmd_begin(ctx->cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	if (ctx->pSurf->img->layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL || ctx->dev->threadAware) {
		VkhImage imgMs = ctx->pSurf->imgMS;
		if (imgMs != NULL)
			vkh_image_set_layout(ctx->cmd, imgMs, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		vkh_image_set_layout(ctx->cmd, ctx->pSurf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		vkh_image_set_layout(ctx->cmd, ctx->pSurf->stencil, ctx->dev->stencilAspectFlag,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
	}

#if defined(DEBUG) && defined(VKVG_DBG_UTILS)
	vkh_cmd_label_start(ctx->cmd, "dc render pass", DBG_LAB_COLOR_RP);
#endif

	CmdBeginRenderPass(ctx->cmd, &ctx->renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	VkViewport viewport = { 0, 0, (float)ctx->pSurf->width, (float)ctx->pSurf->height, 0, 1.f };
	CmdSetViewport(ctx->cmd, 0, 1, &viewport);

	CmdSetScissor(ctx->cmd, 0, 1, &ctx->bounds);

	VkDescriptorSet dss[] = { ctx->dsFont, ctx->dsSrc, ctx->dsGrad };
	//CmdBindDescriptorSets(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelineLayout, 0, 3, dss, 0, NULL);

	VkDeviceSize offsets[1] = { 0 };
	CmdBindVertexBuffers(ctx->cmd, 0, 1, &ctx->vertices.buffer, offsets);
	CmdBindIndexBuffer(ctx->cmd, ctx->indices.buffer, 0, VKVG_VK_INDEX_TYPE);

	_update_push_constants(ctx);
	_bind_draw_pipeline(ctx);
	CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
	ctx->cmdStarted = true;
	push_update_descriptor_set_a(ctx, ctx->d_img, ctx->d_offset);
}

void dc_start_cmd_for_render_pass0(VkvgContext ctx) {
	if (ctx->pSurf->img->layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL || ctx->dev->threadAware) {
		VkhImage imgMs = ctx->pSurf->imgMS;
		if (imgMs != NULL)
			vkh_image_set_layout(ctx->cmd, imgMs, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		vkh_image_set_layout(ctx->cmd, ctx->pSurf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		vkh_image_set_layout(ctx->cmd, ctx->pSurf->stencil, ctx->dev->stencilAspectFlag,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
	}

	CmdBeginRenderPass(ctx->cmd, &ctx->renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	VkViewport viewport = { 0, 0, (float)ctx->pSurf->width, (float)ctx->pSurf->height, 0, 1.f };
	CmdSetViewport(ctx->cmd, 0, 1, &viewport);

	CmdSetScissor(ctx->cmd, 0, 1, &ctx->bounds);

	VkDescriptorSet dss[] = { ctx->dsFont, ctx->dsSrc, ctx->dsGrad };
	CmdBindDescriptorSets(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelineLayout, 0, 3, dss, 0, NULL);

	VkDeviceSize offsets[1] = { 0 };
	CmdBindVertexBuffers(ctx->cmd, 0, 1, &ctx->vertices.buffer, offsets);
	CmdBindIndexBuffer(ctx->cmd, ctx->indices.buffer, 0, VKVG_VK_INDEX_TYPE);

	_update_push_constants(ctx);

	_bind_draw_pipeline(ctx);
	CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
	ctx->cmdStarted = true;
}
void _end_render_pass0(VkvgContext ctx) {
	LOG(VKVG_LOG_INFO, "END RENDER PASS: ctx = %p;\n", ctx);
	CmdEndRenderPass(ctx->cmd);
	ctx->renderPassBeginInfo.renderPass = ctx->dev->renderPass;
}
void dc_update_push_constants(VkvgContext ctx, state_save_t* t) {
	t->pushConsts.size = { (float)ctx->pSurf->width, (float)ctx->pSurf->height };
	CmdPushConstants(ctx->cmd, ctx->dev->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants)
		, &t->pushConsts);
}

void vgdev_ctx::draw(VkvgContext ctx, void* waitSemaphore)
{
	if (!ctx || cmdlist.empty() || _vertex.empty())return;// 填充0、描边1、裁剪2
	check_vao_size(ctx, _vertex.size(), _indices.size());
	_resize_ubo(ctx, gCount);
	ctx->gxCount = 0;
	memcpy(vkh_buffer_get_mapped_pointer(&ctx->vertices), _vertex.data(), _vertex.size() * sizeof(Vertex));
	memcpy(vkh_buffer_get_mapped_pointer(&ctx->indices), _indices.data(), _indices.size() * sizeof(uint32_t));
	if (ctx->cmdStarted)
	{
		if (ctx->cmd == ctx->cmdBuffers[0])
			ctx->cmd = ctx->cmdBuffers[1];
		else
			ctx->cmd = ctx->cmdBuffers[0];
		ResetCommandBuffer(ctx->cmd, 0);
	}
	ctx->cmdStarted = false;
	dc_start_cmd_for_render_pass(ctx);
	for (auto& it : cmdlist)
	{
		auto t = it.state;
		if (t)
		{
			update_pattern(ctx, t->pattern, t);
			dc_update_push_constants(ctx, t);
		}
		switch (it.type)
		{
		case 0:
		{// 填充
			if (t && t->curFillRule == VKVG_FILL_RULE_EVEN_ODD) {
				CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelinePolyFill);
				for (size_t i = 0; i < it.vc; i++)
				{
					vkCmdDraw(ctx->cmd, it.v[i].vertexCount, 1, it.v[i].firstVertex, 0);
				}
				bind_draw_pipeline(ctx, it.state);
				CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
				cmd_draw_full_screen_quad(ctx, &it, &it.bounds);
				CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
			}
			else {
				bind_draw_pipeline(ctx, it.state);
				vkCmdDrawIndexed(ctx->cmd, it.index.y, 1, it.index.x, (int32_t)it.vertex.x, 0);
			}
		}
		break;
		case 1:
		{// 描边
			bind_draw_pipeline(ctx, it.state);
			vkCmdDrawIndexed(ctx->cmd, it.index.y, 1, it.index.x, (int32_t)it.vertex.x, 0);
		}
		break;
		case 2:
		{// 裁剪
			if (it.vc > 0 || it.index.y > 0) {
				if (t && t->curFillRule == VKVG_FILL_RULE_EVEN_ODD) {
					CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelinePolyFill);
					for (size_t i = 0; i < it.vc; i++)
					{
						vkCmdDraw(ctx->cmd, it.v[i].vertexCount, 1, it.v[i].firstVertex, 0);
					}
					CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelineClipping);
				}
				else {
					CmdBindPipeline(ctx->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->dev->pipelineClipping);
					CmdSetStencilReference(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
					CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
					CmdSetStencilWriteMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
					vkCmdDrawIndexed(ctx->cmd, it.index.y, 1, it.index.x, (int32_t)it.vertex.x, 0);
				}
				CmdSetStencilReference(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
				CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
				CmdSetStencilWriteMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_ALL_BIT);
				cmd_draw_full_screen_quad(ctx, &it, NULL);
				CmdSetStencilCompareMask(ctx->cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
			}
			else {
				vkCmdClearAttachments(ctx->cmd, 1, &clearStencil, 1, &ctx->clearRect);
			}
		}
		break;
		}
	}
	_end_render_pass(ctx);
	// 更新渐变ubo
	vkh_buffer_flush(&ctx->uboGrad);
	if (ctx->dev->deferredResolve)
		dc_explicit_ms_resolve(ctx->cmd, ctx->pSurf);
	vkh_cmd_end(ctx->cmd);
	if (!ctx->cmdStarted) // current cmd buff is empty, be aware that wait is also canceled!!
		return;
	if (is_fence)
	{
		vkResetFences(ctx->dev->vkDev, 1, &ctx->flushFence);
		_device_submit_cmd(ctx->dev, &ctx->cmd, ctx->flushFence);
		if (!_wait_ctx_flush_end(ctx))
			return;
	}
	else
	{
		_device_submit_cmd_sem(ctx->dev, &ctx->cmd, (VkSemaphore)waitSemaphore, ctx->pSurf->sem, VK_NULL_HANDLE);
	}
	ctx->cmdStarted = false;
}

void _device_submit_cmd_sem(VkvgDevice dev, VkCommandBuffer* cmd, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence fence)
{
	LOCK_DEVICE
		vkh_cmd_submit_with_semaphores(dev->gQueue, cmd, waitSemaphore, signalSemaphore, fence);
	UNLOCK_DEVICE
}

void vgdev_ctx::begin_frame()
{
	gCount = 0;
	mac.release();
	_vertex.clear();
	_indices.clear();
	cmdlist.clear();
	t = dc_new_state(this);
}

void vgdev_ctx::end_frame()
{
	_vertex.clear();
	_indices.clear();
	cmdlist.clear();
}

void vgdev_ctx::set_glutess(bool b) { is_glutess = b; }

paths_t* vgdev_ctx::get_path()
{
	return _dpath;
}

#endif



void vkvg_clear_rect(VkvgContext ctx, int x, int y, int width, int height) {
	if (vkvg_status(ctx))
		return;
	if (!ctx->cmdStarted) {
		ctx->renderPassBeginInfo.renderPass = ctx->dev->renderPass;
		// force run of one renderpass (even empty) to perform clear load op
		_start_cmd_for_render_pass(ctx);
	}
	VkClearRect cr = ctx->clearRect;
	if (width > 0 && height > 0 && x >= 0 && y >= 0) {
		cr.rect.offset = { x,y };
		cr.rect.extent = { std::min((uint32_t)width,cr.rect.extent.width - x)
			,std::min((uint32_t)height,cr.rect.extent.height - y) };
	}
	VkClearAttachment ca[2] = { clearColorAttach, clearStencil };
	ca[1].clearValue.depthStencil.depth = 1;
	vkCmdClearAttachments(ctx->cmd, 2, ca, 1, &cr);
}
vgdev_ctx* new_vgctx() {
	auto p = new vgdev_ctx();
	return p;
}
void free_vgctx(vgdev_ctx* p) {
	if (p)delete p;
}
void dc_set_glutess(vgdev_ctx* ctx, bool enable)
{
	if (ctx)ctx->set_glutess(enable);
}
void dc_clip_preserve(vgdev_ctx* ctx, paths_t* p) {
	if (ctx && p)
		ctx->clip_preserve(p);
}
void dc_fill_preserve(vgdev_ctx* ctx, paths_t* p) {
	if (ctx && p)
		ctx->fill_preserve(p);
}
void dc_stroke_preserve(vgdev_ctx* ctx, paths_t* p) {
	if (ctx && p)
		ctx->stroke_preserve(p);
}

void dc_draw(vgdev_ctx* ctx, VkvgContext ctxvg, void* waitSemaphore) {
	if (ctx && ctxvg)
		ctx->draw(ctxvg, waitSemaphore);
}
void dc_begin_frame(vgdev_ctx* ctx) {
	if (ctx)ctx->begin_frame();
}
void dc_end_frame(vgdev_ctx* ctx) {
	if (ctx)ctx->end_frame();
}
struct path_pri :public paths_t
{
	uint32_t segmentPtr = 0;   // current segment count in current path having curves
	uint32_t subpathCount = 0; // store count of subpath, not straight forward to retrieve from segmented path array
	t_vector<vec2> points;
	t_vector<uint32_t> pathes;
	vgdev_ctx* ctx = 0;
	bool simpleConvex = false; // true if path is single rect or concave closed curve.
};
void dc_finish_path(paths_t* ctx) {
	auto pri = (path_pri*)ctx;
	do {
		if (pri->pathes.empty())
			pri->pathes.push_back(0);
		if (pri->pathes[ctx->pathPtr] == 0) // empty
			break;
		if ((pri->pathes[ctx->pathPtr] & PATH_ELT_MASK) < 2) {
			// only current pos is in path
			ctx->pointCount -= ctx->pathes[ctx->pathPtr]; // what about the bounds?
			pri->pathes[ctx->pathPtr] = 0;
			pri->segmentPtr = 0;
			break;
		}

		if (ctx->pathPtr == 0 && pri->simpleConvex)
			pri->pathes[0] |= PATH_IS_CONVEX_BIT;

		if (pri->segmentPtr > 0) { // pathes having curves are segmented
			pri->pathes[ctx->pathPtr] |= PATH_HAS_CURVES_BIT;
			// curved segment increment segmentPtr on curve end,
			// so if last segment is not a curve and point count > 0
			if ((pri->pathes[ctx->pathPtr + pri->segmentPtr] & PATH_HAS_CURVES_BIT) == 0 &&
				(pri->pathes[ctx->pathPtr + pri->segmentPtr] & PATH_ELT_MASK) > 0)
				pri->segmentPtr++; // current segment has to be included
			ctx->pathPtr += pri->segmentPtr;
		}
		else
			ctx->pathPtr++;

		if (pri->pathes.size() <= ctx->pathPtr)
			pri->pathes.resize(ctx->pathPtr + 1);

		pri->pathes[ctx->pathPtr] = 0;
		pri->segmentPtr = 0;
		pri->subpathCount++;
		pri->simpleConvex = false;
	} while (0);

	ctx->pathes = pri->pathes.data();
	ctx->points = pri->points.data();
}
// clear path datas in context
void dc_clear_path(paths_t* ctx) {
	ctx->pathPtr = 0;
	auto pri = (path_pri*)ctx;
	pri->pathes.resize(1);
	pri->pathes[ctx->pathPtr] = 0;
	pri->points.clear();
	ctx->pointCount = 0;
	pri->segmentPtr = 0;
	pri->subpathCount = 0;
	pri->simpleConvex = false;
}

void dc_clip(vgdev_ctx* ctx, paths_t* p) {
	if (ctx && p)
	{
		ctx->clip_preserve(p);
		dc_clear_path(p);
	}
}
void dc_fill(vgdev_ctx* ctx, paths_t* p) {
	if (ctx && p)
	{
		ctx->fill_preserve(p);
		dc_clear_path(p);
	}
}
void dc_stroke(vgdev_ctx* ctx, paths_t* p) {
	if (ctx && p)
	{
		ctx->stroke_preserve(p);
		dc_clear_path(p);
	}
}

void dc_remove_last_point(paths_t* ctx) {
	ctx->pointCount--;
	auto pri = (path_pri*)ctx;
	pri->pathes[ctx->pathPtr]--;
	pri->points.pop_back();
	if (pri->segmentPtr > 0) {                            // if path is segmented
		if (!pri->pathes[ctx->pathPtr + pri->segmentPtr]) // if current segment is empty
			pri->segmentPtr--;
		pri->pathes[ctx->pathPtr + pri->segmentPtr]--;                          // decrement last segment point count
		if ((pri->pathes[ctx->pathPtr + pri->segmentPtr] & PATH_ELT_MASK) == 0) // if no point left (was only one)
			pri->pathes[ctx->pathPtr + pri->segmentPtr] = 0;                    // reset current segment
		else if (pri->pathes[ctx->pathPtr + pri->segmentPtr] & PATH_HAS_CURVES_BIT) // if segment is a curve
			pri->segmentPtr++; // then segPtr has to be forwarded to new segment
	}
	if (pri->pathes.size() < pri->segmentPtr + ctx->pathPtr)
		pri->pathes.resize(pri->segmentPtr + ctx->pathPtr + 1);
}
void dc_set_curve_start(path_pri* ctx) {
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
void dc_set_curve_end(path_pri* ctx) {
	ctx->pathes[ctx->pathPtr + ctx->segmentPtr] |= PATH_HAS_CURVES_BIT;
	ctx->segmentPtr++;
	ctx->pathes.push_back(0);
}
bool dc_path_is_closed(paths_t* ctx, uint32_t ptrPath) { return ctx->pathes[ptrPath] & PATH_CLOSED_BIT; }
void dc_add_point(paths_t* ctx, float x, float y) {
	if (isnan(x) || isnan(y)) {
		return;
	}
	auto pri = (path_pri*)ctx;
	vec2 v = { x, y };
	pri->points.push_back(v);
	ctx->pointCount++;           // total point count of pathes, (for array bounds check)
	if (pri->pathes.size() <= ctx->pathPtr + pri->segmentPtr)
		pri->pathes.resize(ctx->pathPtr + pri->segmentPtr + 1);
	pri->pathes[ctx->pathPtr]++; // total point count in path
	if (pri->segmentPtr > 0)
		pri->pathes[ctx->pathPtr + pri->segmentPtr]++; // total point count in path's segment
}

void dc_move_to(paths_t* ctx, float x, float y) {
	if (!ctx)
		return;
	dc_finish_path(ctx);
	dc_add_point(ctx, x, y);
}
bool dc_current_path_is_empty(path_pri* ctx) {
	return ctx->pathes.empty() || ctx->pathes[ctx->pathPtr] == 0;
}
// this function expect that current point exists
vec2 dc_get_current_position(path_pri* ctx) {
	return ctx->points.back();
}
void dc0_line_to(path_pri* ctx, float x, float y) {
	vec2 p = { x, y };
	if (!dc_current_path_is_empty(ctx)) {
		// prevent adding the same point
		if (vec2_equ(dc_get_current_position(ctx), p))
			return;
	}
	dc_add_point(ctx, x, y);
	ctx->simpleConvex = false;
}
void dc_line_to(paths_t* ctx, float x, float y) {
	if (!ctx)
		return;
	dc0_line_to((path_pri*)ctx, x, y);
}

void dc_close_path(path_pri* ctx) {
	if (!ctx)
		return;
	if (ctx->pathes[ctx->pathPtr] & PATH_CLOSED_BIT) // already closed
		return;
	// check if at least 3 points are present
	if (ctx->pathes[ctx->pathPtr] < 3)
		return;
	ctx->pointCount = ctx->points.size();
	// prevent closing on the same point
	if (vec2_equ(ctx->points[ctx->pointCount - 1], ctx->points[ctx->pointCount - ctx->pathes[ctx->pathPtr]])) {
		if (ctx->pathes[ctx->pathPtr] < 4) // ensure enough points left for closing
			return;
		dc_remove_last_point(ctx);
	}

	ctx->pathes[ctx->pathPtr] |= PATH_CLOSED_BIT;

	dc_finish_path(ctx);
}

float dc_get_arc_step(path_pri* ctx, float radius) {
	float sx = 1.0, sy = 1.0;
	//vkvg_matrix_get_scale(&ctx->pushConsts.mat, &sx, &sy);
	float r = radius * fabsf(fmaxf(sx, sy));
	if (r < 30.0f)
		return fminf(M_PIF / 3.f, M_PIF / r);
	return fminf(M_PIF / 3.f, M_PIF / (r * 0.4f));
}
void dc_arc(paths_t* ctx0, float xc, float yc, float radius, float a1, float a2) {
	if (!ctx0)
		return;
	while (a2 < a1) // positive arc must have a1<a2
		a2 += 2.f * M_PIF;

	if (a2 - a1 > 2.f * M_PIF) // limit arc to 2PI
		a2 = a1 + 2.f * M_PIF;

	auto ctx = (path_pri*)ctx0;
	vec2 v = { cosf(a1) * radius + xc, sinf(a1) * radius + yc };

	float step = dc_get_arc_step(ctx, radius);
	float a = a1;

	if (dc_current_path_is_empty(ctx)) {
		dc_set_curve_start(ctx);
		dc_add_point(ctx, v.x, v.y);
		if (!ctx->pathPtr)
			ctx->simpleConvex = true;
		else
			ctx->simpleConvex = false;
	}
	else {
		dc0_line_to(ctx, v.x, v.y);
		dc_set_curve_start(ctx);
		ctx->simpleConvex = false;
	}

	a += step;

	if (EQUF(a2, a1))
		return;

	while (a < a2) {
		v.x = cosf(a) * radius + xc;
		v.y = sinf(a) * radius + yc;
		dc_add_point(ctx, v.x, v.y);
		a += step;
	}

	if (EQUF(a2 - a1, M_PIF * 2.f)) { // if arc is complete circle, last point is the same as the first one
		dc_set_curve_end(ctx);
		dc_close_path(ctx);
		return;
	}
	a = a2;
	// vec2 lastP = v;
	v.x = cosf(a) * radius + xc;
	v.y = sinf(a) * radius + yc;
	// if (!vec2_equ (v,lastP))//this test should not be required
	dc_add_point(ctx, v.x, v.y);
	dc_set_curve_end(ctx);
}

void dc_arc_negative(paths_t* ctx0, float xc, float yc, float radius, float a1, float a2) {
	if (!ctx0)
		return;
	auto ctx = (path_pri*)ctx0;
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: %f,%f %f %f %f\n", xc, yc, radius, a1, a2);
	while (a2 > a1)
		a2 -= 2.f * M_PIF;
	if (a1 - a2 > a1 + 2.f * M_PIF) // limit arc to 2PI
		a2 = a1 - 2.f * M_PIF;

	vec2 v = { cosf(a1) * radius + xc, sinf(a1) * radius + yc };

	float step = dc_get_arc_step(ctx, radius);
	float a = a1;

	if (dc_current_path_is_empty(ctx)) {
		dc_set_curve_start(ctx);
		dc_add_point(ctx, v.x, v.y);
		if (!ctx->pathPtr)
			ctx->simpleConvex = true;
		else
			ctx->simpleConvex = false;
	}
	else {
		dc0_line_to(ctx, v.x, v.y);
		dc_set_curve_start(ctx);
		ctx->simpleConvex = false;
	}

	a -= step;

	if (EQUF(a2, a1))
		return;

	while (a > a2) {
		v.x = cosf(a) * radius + xc;
		v.y = sinf(a) * radius + yc;
		dc_add_point(ctx, v.x, v.y);
		a -= step;
	}

	if (EQUF(a1 - a2, M_PIF * 2.f)) { // if arc is complete circle, last point is the same as the first one
		dc_set_curve_end(ctx);
		dc_close_path(ctx);
		return;
	}

	a = a2;
	// vec2 lastP = v;
	v.x = cosf(a) * radius + xc;
	v.y = sinf(a) * radius + yc;
	// if (!vec2_equ (v,lastP))
	dc_add_point(ctx, v.x, v.y);
	dc_set_curve_end(ctx);
}
void dc_rel_move_to(paths_t* ctx0, float x, float y) {
	if (!ctx0)
		return;
	auto ctx = (path_pri*)ctx0;
	LOG(VKVG_LOG_INFO_CMD, "\tCMD: rel_mote_to: %f, %f\n", x, y);
	if (dc_current_path_is_empty(ctx))
		dc_add_point(ctx, 0, 0);
	vec2 cp = dc_get_current_position(ctx);
	dc_finish_path(ctx);
	dc_add_point(ctx, cp.x + x, cp.y + y);
}


int dc_rounded_rectangle(paths_t* ctx, float x, float y, float w, float h, float radius) {
	if (!ctx)
		return -1;
	dc_finish_path(ctx);

	if (w <= 0 || h <= 0)
		return VKVG_STATUS_INVALID_RECT;

	if ((radius > w / 2.0f) || (radius > h / 2.0f))
		radius = fmin(w / 2.0f, h / 2.0f);

	auto pri = (path_pri*)ctx;
	dc_move_to(ctx, x, y + radius);
	dc_arc(ctx, x + radius, y + radius, radius, M_PIF, -M_PIF_2);
	dc_line_to(ctx, x + w - radius, y);
	dc_arc(ctx, x + w - radius, y + radius, radius, -M_PIF_2, 0);
	dc_line_to(ctx, x + w, y + h - radius);
	dc_arc(ctx, x + w - radius, y + h - radius, radius, 0, M_PIF_2);
	dc_line_to(ctx, x + radius, y + h);
	dc_arc(ctx, x + radius, y + h - radius, radius, M_PIF_2, M_PIF);
	dc_line_to(ctx, x, y + radius);
	dc_close_path((path_pri*)ctx);
	ctx->pathes = pri->pathes.data();
	ctx->points = pri->points.data();
	return VKVG_STATUS_SUCCESS;
}
int dc_rectangle(paths_t* ctx, float x, float y, float w, float h, float r) {
	if (!ctx)
		return -1;
	if (r > 0)
		return dc_rounded_rectangle(ctx, x, y, w, h, r);
	dc_finish_path(ctx);

	if (w <= 0 || h <= 0)
		return VKVG_STATUS_INVALID_RECT;

	auto pri = (path_pri*)ctx;
	dc_add_point(ctx, x, y);
	dc_add_point(ctx, x + w, y);
	dc_add_point(ctx, x + w, y + h);
	dc_add_point(ctx, x, y + h);

	pri->pathes[ctx->pathPtr] |= (PATH_CLOSED_BIT | PATH_IS_CONVEX_BIT);

	dc_finish_path(ctx);
	return VKVG_STATUS_SUCCESS;
}

state_save_t* dc_new_state(vgdev_ctx* ctx) {
	auto t = (state_save_t*)ctx->mac.allocate(sizeof(state_save_t));
	*t = {};
	t->curColor = -1;
	//VkRect2D b = {};
	//b.extent = { ctx->pSurf->width, ctx->pSurf->height };
	push_constants pc = {};
	pc.source.a = 1;
	pc.size = { (float)100, (float)100 };
	pc.fsq_patternType = VKVG_PATTERN_TYPE_SOLID;
	pc.opacity = 1.0f;
	pc.mat = VKVG_IDENTITY_MATRIX;
	pc.matInv = VKVG_IDENTITY_MATRIX;
	t->lineWidth = 1.f;
	t->miterLimit = 10.f;
	t->curOperator = VKVG_OPERATOR_OVER;
	t->curFillRule = VKVG_FILL_RULE_NON_ZERO;
	t->aa = false;
	//t->bounds = b;
	t->pushConsts = pc;
	return t;
}

VkvgPattern dc_pattern_create_linear(vgdev_ctx* ctx, float x0, float y0, float x1, float y1) {

	auto pat = (vkvg_pattern_t*)ctx->mac.allocate(sizeof(vkvg_pattern_t) + sizeof(vkvg_gradient_t));
	if (!pat) {
		LOG(VKVG_LOG_ERR, "CREATE Pattern failed, no memory\n");
		return (VkvgPattern)&_vkvg_status_null_pointer;
	}
	*pat = {};
	pat->type = VKVG_PATTERN_TYPE_LINEAR;
	pat->extend = VKVG_EXTEND_NONE;
	pat->data = pat + 1;
	if (pat->data) {
		vkvg_pattern_edit_linear(pat, x0, y0, x1, y1);
		pat->references = 1;
	}
	else {
		pat->status = VKVG_STATUS_PATTERN_INVALID_GRADIENT;
	}
	return pat;
}
// circle 或 ellipse
VkvgPattern dc_pattern_create_radial(vgdev_ctx* ctx, float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse) {
	auto pat = (vkvg_pattern_t*)ctx->mac.allocate(sizeof(vkvg_pattern_t) + sizeof(vkvg_gradient_t));
	if (!pat) {
		LOG(VKVG_LOG_ERR, "CREATE Pattern failed, no memory\n");
		return (VkvgPattern)&_vkvg_status_null_pointer;
	}
	*pat = {};
	pat->type = VKVG_PATTERN_TYPE_RADIAL;
	pat->extend = VKVG_EXTEND_NONE;
	pat->data = pat + 1;
	if (pat->data) {
		vkvg_pattern_edit_radial(pat, cx0, cy0, radius0, cx1, cy1, radius1, is_ellipse);
		pat->references = 1;
	}
	else
		pat->status = VKVG_STATUS_NO_MEMORY;
	return pat;
}

VkvgPattern dc_pattern_create_sweep(vgdev_ctx* ctx, float cx, float cy, float start_angle, float end_angle) {
	auto pat = (vkvg_pattern_t*)ctx->mac.allocate(sizeof(vkvg_pattern_t) + sizeof(vkvg_gradient_t));
	if (!pat) {
		LOG(VKVG_LOG_ERR, "CREATE Pattern failed, no memory\n");
		return (VkvgPattern)&_vkvg_status_null_pointer;
	}
	*pat = {};
	pat->type = VKVG_PATTERN_TYPE_SWEEP;
	pat->extend = VKVG_EXTEND_NONE;
	pat->data = pat + 1;
	if (pat->data) {
		vkvg_pattern_edit_sweep(pat, cx, cy, start_angle, end_angle);
		pat->references = 1;
	}
	else
		pat->status = VKVG_STATUS_NO_MEMORY;
	return pat;
}

vkvg_status_t dc_pattern_set_color_stop(VkvgPattern pat, int idx, float r, float g, float b, float a) {
	if (vkvg_pattern_status(pat))
		return vkvg_pattern_status(pat);
	if (pat->type == VKVG_PATTERN_TYPE_SURFACE || pat->type == VKVG_PATTERN_TYPE_SOLID)
		return VKVG_STATUS_PATTERN_TYPE_MISMATCH;
	vkvg_gradient_t* grad = (vkvg_gradient_t*)pat->data;
	if (idx < 0 || idx >= grad->count)return VKVG_STATUS_PATTERN_INVALID_GRADIENT;
	vkvg_color_t c = { r, g, b, a };
	grad->colors[idx] = c;
	return VKVG_STATUS_SUCCESS;
}

void dc_set_line_cap(vgdev_ctx* ctx, vkvg_line_cap_t lineCap) {
	auto t = ctx ? ctx->t : nullptr;
	if (t) t->lineCap = lineCap;
}
void dc_set_line_join(vgdev_ctx* ctx, vkvg_line_join_t lineJoin) {
	auto t = ctx ? ctx->t : nullptr;
	if (t) t->lineJoin = lineJoin;
}
void dc_set_fill_rule(vgdev_ctx* ctx, vkvg_fill_rule_t fillRule) {
	auto t = ctx ? ctx->t : nullptr;
	if (t) t->curFillRule = fillRule;
}
void dc_set_color(vgdev_ctx* ctx, uint32_t color) {
	auto t = ctx ? ctx->t : nullptr;
	if (t) { t->curColor = color; t->pattern = 0; }
}
void dc_set_mat_inv_and_vkCmdPush(state_save_t* t) {
	if (t) {
		t->pushConsts.matInv = t->pushConsts.mat;
		vkvg_matrix_invert(&t->pushConsts.matInv);
	}
}
void dc_translate(vgdev_ctx* ctx, float dx, float dy)
{
	auto t = ctx ? ctx->t : nullptr;
	if (t) {
		vkvg_matrix_translate(&t->pushConsts.mat, dx, dy);
		dc_set_mat_inv_and_vkCmdPush(t);
	}
}
void dc_set_source_rgba(vgdev_ctx* ctx, float r, float g, float b, float a) {
	auto t = ctx ? ctx->t : nullptr;
	if (!t)
		return;
	t->curColor = CreateRgbaf(r, g, b, a); t->pattern = 0;
}
void dc_set_source(vgdev_ctx* ctx, VkvgPattern pat) {
	auto t = ctx ? ctx->t : nullptr;
	if (!t)
		return;
	t->pattern = pat;
}
void dc_set_operator(vgdev_ctx* ctx, vkvg_operator_t op) {
	auto t = ctx ? ctx->t : nullptr;
	if (t) t->curOperator = op;
}


paths_t* dc_get_paths(vgdev_ctx* ctx) {
	return ctx ? ctx->_dpath : nullptr;
}

paths_t* dc_new_paths(vgdev_ctx* ctx) {
	auto t = (path_pri*)ctx->ac.new_obj<path_pri>();
	if (t)t->ctx = ctx;
	return (paths_t*)t;
}
void dc_free_paths(paths_t* p) {
	auto pri = (path_pri*)p;
	if (p && pri->ctx)
		pri->ctx->ac.free_obj(pri);
}
void dc_set_line_width(vgdev_ctx* ctx, float width) {
	auto t = ctx ? ctx->t : nullptr;
	if (t && width > 0)t->lineWidth = width;
}
void dc_clip0(vgdev_ctx* ctx) {
	if (!ctx) // nothing to clip
		return;
	ctx->clip0();
}

void dc_grid_fill(vgdev_ctx* cr, glm::vec2 size, glm::ivec2 cols, int width)
{
	int x = fmod(size.x, width);
	int y = fmod(size.y, width);
	int xn = size.x / width;
	int yn = size.y / width;
	if (x > 0)xn++;
	if (y > 0)yn++;
	auto path = cr->get_path();
	//dc_rectangle(path, 0, 0, size.x, size.y, 0);
	//dc_clip(cr, path);
	for (size_t i = 0; i < yn; i++)
	{
		auto iw = i * width;
		for (size_t j = 0; j < xn; j++)
		{
			bool k0 = (j & 1);
			bool k1 = !(j & 1);
			auto k = !(i & 1) ? k0 : k1;
			if (k)
				dc_rectangle(path, j * width, iw, width, width, 0);
		}
	}
	auto c = cols[0];
	dc_set_color(cr, c);
	dc_fill(cr, path);
	for (size_t i = 0; i < yn; i++)
	{
		auto iw = i * width;
		for (size_t j = 0; j < xn; j++)
		{
			bool k0 = (j & 1);
			bool k1 = !(j & 1);
			auto k = (i & 1) ? k0 : k1;
			if (k)
				dc_rectangle(path, j * width, iw, width, width, 0);
		}
	}
	c = cols[1];
	dc_set_color(cr, c);
	dc_fill(cr, path);
}

drawctx_t get_drawctx(vgdev_ctx* p)
{
	drawctx_t r = {};
	if (p) {
		r.ptr = p;
		r.set_glutess = dc_set_glutess;
		r.clip_preserve = dc_clip_preserve;
		r.fill_preserve = dc_fill_preserve;
		r.stroke_preserve = dc_stroke_preserve;
		r.clip = dc_clip;
		r.clip0 = dc_clip0;
		r.fill = dc_fill;
		r.stroke = dc_stroke;
		r.clear_path = dc_clear_path;
		r.draw = dc_draw;
		r.begin_frame = dc_begin_frame;
		r.end_frame = dc_end_frame;
		r.get_paths = dc_get_paths;
		r.new_paths = dc_new_paths;
		r.new_sub_path = dc_finish_path;
		r.arc = dc_arc;
		r.arc_negative = dc_arc_negative;
		r.set_line_cap = dc_set_line_cap;
		r.set_line_join = dc_set_line_join;
		r.set_fill_rule = dc_set_fill_rule;
		r.set_color = dc_set_color;
		r.set_source_rgba = dc_set_source_rgba;
		r.set_operator = dc_set_operator;
		r.set_source = dc_set_source;
		r.new_pattern_linear = dc_pattern_create_linear;
		r.new_pattern_radial = dc_pattern_create_radial;
		r.new_pattern_sweep = dc_pattern_create_sweep;
		r.pattern_set_color_stop = dc_pattern_set_color_stop;
		r.grid_fill = dc_grid_fill;
		r.rectangle = dc_rectangle;
		r.translate = dc_translate;
		r.set_line_width = dc_set_line_width;
	}
	return r;
}

// rvg_x
#if 1


rvg_x::rvg_x() {
	ctx = new_vgctx();

}
rvg_x::~rvg_x()
{
	free_vgctx(ctx); ctx = 0;
}
void rvg_x::set_pos(const glm::ivec2& ps) { pos = ps; }
void rvg_x::clear()
{
	pos = {};
}
void rvg_x::submit(fill_style_d* st) {}
void rvg_x::submit(uint32_t fill, uint32_t color, int linewidth) {}
// 填充网格
void rvg_x::grid_fill(const glm::vec2& size, const glm::ivec2& cols, int width) {}
// 线性渐变填充
void rvg_x::linear_fill(const glm::vec2& size, const glm::vec4* cols, int count) {}
// 画2d箭头type=0线终点在三角形中间
void rvg_x::add_arrow(const glm::vec2& p0, const glm::vec2& p1, float arrow_hwidth, float arrow_size, bool type) {}
// 批量渲染同样式的块(圆或矩形)
void rvg_x::draw_block(dblock_d* p, fill_style_d* st) {}
// 描边或填充路径
void rvg_x::draw_path(path_d* path, fill_style_d* style) {}
// 批量画单条线
void rvg_x::add_line(glm::vec4* p, size_t count) {}
void rvg_x::add_line(const glm::vec2& ps0, const glm::vec2& ps1) {}

void rvg_x::add_rect(const glm::vec4& rc, double r) {}
void rvg_x::add_rect(const glm::vec4& rc, const glm::vec4& r) {}
void rvg_x::add_circle(const glm::vec2& pos, float r) {}
void rvg_x::add_ellipse(const glm::vec2& pos, const glm::vec2& r, float rotationAngle) {}
// 三角形基于矩形内 
//	 dir = 0{}		// 尖角方向，0上，1右，2下，3左
//	 spos = 50{}		// 尖角点位置0-1，中间就是0.5
void rvg_x::add_triangle(const glm::vec2& pos, const glm::vec2& size, const glm::vec2& dirspos) {}
// 折线,polygon=closed
void rvg_x::draw_polyline(const glm::vec2& pos, const glm::vec2* points, int points_count, uint32_t col, bool closed, float thickness) {}
void rvg_x::add_polyline(const Clipper2Lib::PathsD* p, bool closed) {}
void rvg_x::add_polyline(const glm::vec2* p, size_t count) {}
// 渲染索引多段线，索引-1则跳过
void rvg_x::draw_polylines(const glm::vec2& pos, const glm::vec2* points, int points_count, int* idx, int idx_count, uint32_t col, float thickness) {}
// 文本渲染
void rvg_x::set_text_style(text_style* ts) {}
void rvg_x::add_text(text_st* p, text_style* ts) {}
void rvg_x::add_image(image_ptr_t* img, const glm::ivec4& rc, const glm::ivec4& sliced, uint32_t color, const glm::ivec2& dsize, const glm::ivec2& pos) {}
void rvg_x::add_image(image_r* r) {}
// 添加gem指针的内容。顶点坐标不受状态影响
void rvg_x::add_geometry(geometry_d* geo) {}
void rvg_x::paint_shadow(double size_x, double size_y, double width, double height, const glm::vec4& shadow, const glm::vec4& color_to, bool rev, float r) {}

void rvg_x::clip() {}
void rvg_x::clip(const glm::ivec4& c) {}
glm::ivec4 rvg_x::get_clip() { return _clip; }
void rvg_x::save() {}
void rvg_x::restore() {}
void rvg_x::save0() {}
void rvg_x::restore0() {}
void rvg_x::fill() {}
void rvg_x::stroke() {}
void rvg_x::fill_preserve() {}
void rvg_x::stroke_preserve() {}
void rvg_x::fill_stroke(uint32_t fill, uint32_t color) {}
void rvg_x::set_line_width(float w) {}
void rvg_x::set_color(uint32_t color) {}
void rvg_x::set_color(const glm::vec4& rgba) {}
void rvg_x::translate(const glm::vec2& offset) {}
void rvg_x::scale(float sx, float sy) {}
void rvg_x::scale(const glm::vec2& sc) {}
void rvg_x::rotate(float radians) {}
void rvg_x::transform(const glm::mat3x2* matrix) {}
void rvg_x::set_matrix(const glm::mat3x2* matrix) {}
void rvg_x::get_matrix(glm::mat3x2* matrix) {}
glm::vec2 rvg_x::get_translate() {
	return _translate;
}

void rvg_x::push_ct(uint8_t op) {}
void rvg_x::push_view(const glm::ivec4& c, void* ptr) {}
void rvg_x::pop_view() {}
void rvg_x::push_null(int v) {}
uint32_t get_crc() {
	return 0;
}
uint32_t get_crc2index(size_t i) {
	return 0;
}

#endif 
// !1 rvg_x
