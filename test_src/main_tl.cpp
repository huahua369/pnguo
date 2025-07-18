﻿
#include <pch1.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <pnguo/win_core.h>
#include "win32msg.h"
#include <pnguo/event.h>
#define GUI_STATIC_LIB
#include <pnguo/pnguo.h>
#include <pnguo/tinysdl3.h>
#include <pnguo/vkrenderer.h>
#include <pnguo/page.h>
#include <pnguo/mapView.h>
#include <pnguo/print_time.h>
#include <pnguo/mnet.h> 
#include <pnguo/ecc_sv.h> 
#include <cairo/cairo.h>
#include "mshell.h"
#ifdef GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#endif 
#include <pnguo/editor_2d.h>
#include <spine/spine-sdl3/spinesdl3.h>
#include <stb_image_write.h>
#include <mcut/mcut_cx.h>
#include <mcut/stlrw.h>

#include "logic_gates.h"

auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman";// , Malgun Gothic";
auto fontn1 = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Malgun Gothic";
auto fontn2 = (char*)u8"Consolas,新宋体,Segoe UI Emoji,Times New Roman,Malgun Gothic";
text_style_t* get_defst(int idx)
{
	static text_style_t st[5] = {};
	st[0].font = 0;
	st[0].text_align = { 0.0,0.0 };
	st[0].font_size = 16;
	st[0].text_color = -1;

	st[1].font = 1;
	st[1].text_align = { 0.0,0.0 };
	st[1].font_size = 16;
	st[1].text_color = -1;
	st[2].font = 2;
	st[2].text_align = { 0.0,0.0 };
	st[2].font_size = 16;
	st[2].text_color = -1;
	return &st[idx];
}
extern "C" {
}
/*
	CLAY_RENDER_COMMAND_TYPE_NONE,
	CLAY_RENDER_COMMAND_TYPE_RECTANGLE,
	CLAY_RENDER_COMMAND_TYPE_BORDER,
	CLAY_RENDER_COMMAND_TYPE_TEXT,
	CLAY_RENDER_COMMAND_TYPE_IMAGE,
	CLAY_RENDER_COMMAND_TYPE_SCISSOR_START,
	CLAY_RENDER_COMMAND_TYPE_SCISSOR_END,
	CLAY_RENDER_COMMAND_TYPE_CUSTOM,
*/


/*
容器控件：子项渲染、事件处理、数据结构
*/
#include <memory>
#include <filesystem>


#if 1
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <sstream>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <comutil.h>
#include <wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

#elif defined(__linux__)
#include <fstream>

#elif defined(__APPLE__) || defined(__MACH__)
#include <sys/sysctl.h>

#endif

#include <libcpuid.h>  
#include <winioctl.h>
#include <winerror.h>

struct msr_driver_t2 {
	char driver_path[MAX_PATH + 1];
	SC_HANDLE scManager;
	volatile SC_HANDLE scDriver;
	HANDLE hhDriver;
	OVERLAPPED ovl;
	int errorcode;
};

static BOOL wait_for_service_state2(SC_HANDLE hService, DWORD dwDesiredState, SERVICE_STATUS* lpsrvStatus) {
	BOOL fOK = FALSE;
	DWORD dwWaitHint;

	if (hService != NULL) {
		while (TRUE) {
			fOK = QueryServiceStatus(hService, lpsrvStatus);
			if (!fOK)
				break;
			if (lpsrvStatus->dwCurrentState == dwDesiredState)
				break;

			dwWaitHint = lpsrvStatus->dwWaitHint / 10;    // Poll 1/10 of the wait hint
			if (dwWaitHint < 1000)
				dwWaitHint = 1000;  // At most once per second
			if (dwWaitHint > 10000)
				dwWaitHint = 10000; // At least every 10 seconds
			Sleep(dwWaitHint);
		}
	}

	return fOK;
}
static int load_driver(struct msr_driver_t2* drv)
{
	LPTSTR		lpszInfo = (char*)("RDMSR Executor Driver");
	USHORT		uLen = 0;
	SERVICE_STATUS srvStatus = { 0 };
	BOOL		fRunning = FALSE;
	DWORD		dwLastError;
	LPTSTR		lpszDriverServiceName = (char*)("WinRing0_1_2_0");
	TCHAR		lpszDriverName[] = __TEXT("\\\\.\\Global\\TmpRdr");

	if ((LPVOID)(drv->scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) != NULL) {
		drv->scDriver = CreateService(drv->scManager, lpszDriverServiceName, lpszInfo, SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
			drv->driver_path, NULL, NULL, NULL, NULL, NULL);
		if (drv->scDriver == NULL) {
			switch (dwLastError = GetLastError()) {
			case ERROR_SERVICE_EXISTS:
			case ERROR_SERVICE_MARKED_FOR_DELETE: {
				LPQUERY_SERVICE_CONFIG lpqsc;
				DWORD dwBytesNeeded;

				drv->scDriver = OpenService(drv->scManager, lpszDriverServiceName, SERVICE_ALL_ACCESS);
				if (drv->scDriver == NULL) {
					//debugf(1, "Error opening service: %d\n", GetLastError());
					break;
				}

				QueryServiceConfig(drv->scDriver, NULL, 0, &dwBytesNeeded);
				if ((dwLastError = GetLastError()) == ERROR_INSUFFICIENT_BUFFER) {
					lpqsc = (LPQUERY_SERVICE_CONFIG)calloc(1, dwBytesNeeded);
					if (!QueryServiceConfig(drv->scDriver, lpqsc, dwBytesNeeded, &dwBytesNeeded)) {
						free(lpqsc);
						//debugf(1, "Error query service config(adjusted buffer): %d\n", GetLastError());
						goto clean_up;
					}
					else {
						free(lpqsc);
					}
				}
				else {
					//debugf(1, "Error query service config: %d\n", dwLastError);
					goto clean_up;
				}

				break;
			}
			case ERROR_ACCESS_DENIED:
				drv->errorcode = ERR_NO_PERMS;
				break;
			default:
				//debugf(1, "Create driver service failed: %d\n", dwLastError);
				break;
			}
		}
		if (drv->scDriver != NULL) {
			if (StartService(drv->scDriver, 0, NULL)) {
				if (!wait_for_service_state2(drv->scDriver, SERVICE_RUNNING, &srvStatus)) {
					printf("Driver load failed.\n");
					DeleteService(drv->scDriver);
					CloseServiceHandle(drv->scManager);
					drv->scDriver = NULL;
					goto clean_up;
				}
				else {
					fRunning = TRUE;
				}
			}
			else {
				if ((dwLastError = GetLastError()) == ERROR_SERVICE_ALREADY_RUNNING)
					fRunning = TRUE;
				else {
					//debugf(1, "Driver start failed.\n");
					DeleteService(drv->scDriver);
					CloseServiceHandle(drv->scManager);
					drv->scDriver = NULL;
					goto clean_up;
				}

			}
			if (fRunning)
				printf("Driver already running.\n");
			else
				printf("Driver loaded.\n");
			CloseServiceHandle(drv->scManager);
			drv->hhDriver = CreateFile(lpszDriverName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
			drv->ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			return 1;
		}
	}
	else {
		printf("Open SCM failed: %d\n", GetLastError());
	}

clean_up:
	if (drv->scManager != NULL) {
		CloseServiceHandle(drv->scManager);
		drv->scManager = 0; // pointless
	}
	if (drv->scDriver != NULL) {
		if (!DeleteService(drv->scDriver))
			printf("Delete driver service failed: %d\n", GetLastError());
		CloseServiceHandle(drv->scDriver);
		drv->scDriver = 0;
	}

	return 0;
}
#ifndef _T
#define _T(x) x
#endif // !_T

#include "E:\code\cs\WinRing0\dll\OlsDef.h"
#include "E:\code\cs\WinRing0\dll\OlsApiInit.h"
void get_cpu_temperature(int* opt, cpuinfo_t& cpuinfo) {
	DWORD eax = 0, edx = 0;
	DWORD TjMax = 0;
	DWORD IAcore = 0;
	DWORD PKGsts = 0;
	int Cputemp = 0;
	Rdmsr(0x1A2, &eax, &edx);
	TjMax = eax;
	TjMax &= 0xFF0000;
	TjMax = TjMax >> 16;

	//Rdmsr(0x19C, &eax, &edx);
	//IAcore = eax;
	//IAcore &= 0xFF0000;
	//IAcore = IAcore >> 16;

	Cputemp = (int)(TjMax - IAcore);
	PROCESSOR_NUMBER ProcNumber = {};
	for (size_t i = 0; i < cpuinfo.processorCoreCount; i++)
	{
		auto result = SetThreadAffinityMask(GetCurrentThread(), cpuinfo.core_mask[i]);
		auto n0 = GetCurrentProcessorNumber();
		GetCurrentProcessorNumberEx(&ProcNumber);
		Rdmsr(0x19c, &eax, &edx); //read Temperature 
		*opt = TjMax - ((eax & 0x007f0000) >> 16);
		opt++;
	}

	auto result = SetThreadAffinityMask(GetCurrentThread(), cpuinfo.core_mask[0]);
	auto n0 = GetCurrentProcessorNumber();
	GetCurrentProcessorNumberEx(&ProcNumber);
	Rdmsr(0x1B1, &eax, &edx); //read package Temperature
	*opt = TjMax - ((eax & 0x007f0000) >> 16);
	opt++;
}
bool get_cpu_temp() {
	if (hz::check_useradmin())
	{
		printf("有管理员权限\n");
	}
	else {
		printf("请以管理员身份运行\n");
		return false;
	}
	//msr_driver_t2 md = {};
	//strcpy(md.driver_path, R"(E:\code\cs\openhardwaremonitor\Hardware\WinRing0x64.sys)");
	//load_driver(&md);
	HMODULE _hOpenLibSys = 0;
	if (InitOpenLibSys(&_hOpenLibSys) != TRUE)
	{
		std::cout << "DLL Load Error!\n";
		return false;
	}
	int cpu_temp[32] = { 0 };
	cpuinfo_t cpuinfo = get_cpuinfo();
	while (1) {
		get_cpu_temperature(cpu_temp, cpuinfo);
		for (size_t i = 0; i < 7; i++)
		{
			std::cout << cpu_temp[i] << "\t";
		}
		std::cout << "\n";
		Sleep(500);
	}
	if (_hOpenLibSys)
		DeinitOpenLibSys(&_hOpenLibSys);
	return true;
}
// 主函数
int dmain()
{
	auto kct = get_cpu_temp();

	return 0;
}
#endif // 1

// sdl gpu
#if 1
#include <SDL3/SDL_test_common.h>
#include "../third_lib/SDL/test/testgpu/testgpu_spirv.h"
#include "../third_lib/SDL/test/testgpu/testgpu_dxil.h"
#include "../third_lib/SDL/test/testgpu/testgpu_metallib.h"

#define TESTGPU_SUPPORTED_FORMATS (SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXBC | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_METALLIB)

#define CHECK_CREATE(var, thing) { if (!(var)) { SDL_Log("Failed to create %s: %s", thing, SDL_GetError()); quit(2); } }






static Uint32 frames = 0;

typedef struct RenderState
{
	SDL_GPUBuffer* buf_vertex;
	SDL_GPUGraphicsPipeline* pipeline;
	SDL_GPUSampleCount sample_count;
} RenderState;

typedef struct WindowState
{
	int angle_x, angle_y, angle_z;
	SDL_GPUTexture* tex_depth, * tex_msaa, * tex_resolve;
	Uint32 prev_drawablew, prev_drawableh;
} WindowState;

static SDL_GPUDevice* gpu_device = NULL;
static RenderState render_state;
static SDLTest_CommonState* state = NULL;
static WindowState* window_states = NULL;

static void shutdownGPU(void)
{
	if (window_states) {
		int i;
		for (i = 0; i < state->num_windows; i++) {
			WindowState* winstate = &window_states[i];
			SDL_ReleaseGPUTexture(gpu_device, winstate->tex_depth);
			SDL_ReleaseGPUTexture(gpu_device, winstate->tex_msaa);
			SDL_ReleaseGPUTexture(gpu_device, winstate->tex_resolve);
			SDL_ReleaseWindowFromGPUDevice(gpu_device, state->windows[i]);
		}
		SDL_free(window_states);
		window_states = NULL;
	}

	SDL_ReleaseGPUBuffer(gpu_device, render_state.buf_vertex);
	SDL_ReleaseGPUGraphicsPipeline(gpu_device, render_state.pipeline);
	SDL_DestroyGPUDevice(gpu_device);

	SDL_zero(render_state);
	gpu_device = NULL;
}


/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
	shutdownGPU();
	SDLTest_CommonQuit(state);
	exit(rc);
}

/*
 * Simulates desktop's glRotatef. The matrix is returned in column-major
 * order.
 */
static void
rotate_matrix(float angle, float x, float y, float z, float* r)
{
	float radians, c, s, c1, u[3], length;
	int i, j;

	radians = angle * SDL_PI_F / 180.0f;

	c = SDL_cosf(radians);
	s = SDL_sinf(radians);

	c1 = 1.0f - SDL_cosf(radians);

	length = (float)SDL_sqrt(x * x + y * y + z * z);

	u[0] = x / length;
	u[1] = y / length;
	u[2] = z / length;

	for (i = 0; i < 16; i++) {
		r[i] = 0.0;
	}

	r[15] = 1.0;

	for (i = 0; i < 3; i++) {
		r[i * 4 + (i + 1) % 3] = u[(i + 2) % 3] * s;
		r[i * 4 + (i + 2) % 3] = -u[(i + 1) % 3] * s;
	}

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			r[i * 4 + j] += c1 * u[i] * u[j] + (i == j ? c : 0.0f);
		}
	}
}

/*
 * Simulates gluPerspectiveMatrix
 */
static void
perspective_matrix(float fovy, float aspect, float znear, float zfar, float* r)
{
	int i;
	float f;

	f = 1.0f / SDL_tanf(fovy * 0.5f);

	for (i = 0; i < 16; i++) {
		r[i] = 0.0;
	}

	r[0] = f / aspect;
	r[5] = f;
	r[10] = (znear + zfar) / (znear - zfar);
	r[11] = -1.0f;
	r[14] = (2.0f * znear * zfar) / (znear - zfar);
	r[15] = 0.0f;
}

/*
 * Multiplies lhs by rhs and writes out to r. All matrices are 4x4 and column
 * major. In-place multiplication is supported.
 */
static void
multiply_matrix(const float* lhs, const float* rhs, float* r)
{
	int i, j, k;
	float tmp[16];

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			tmp[j * 4 + i] = 0.0;

			for (k = 0; k < 4; k++) {
				tmp[j * 4 + i] += lhs[k * 4 + i] * rhs[j * 4 + k];
			}
		}
	}

	for (i = 0; i < 16; i++) {
		r[i] = tmp[i];
	}
}

typedef struct VertexData
{
	float x, y, z; /* 3D data. Vertex range -0.5..0.5 in all axes. Z -0.5 is near, 0.5 is far. */
	float red, green, blue;  /* intensity 0 to 1 (alpha is always 1). */
} VertexData;

static const VertexData vertex_data[] = {
	/* Front face. */
	/* Bottom left */
	{ -0.5,  0.5, -0.5, 1.0, 0.0, 0.0 }, /* red */
	{  0.5, -0.5, -0.5, 0.0, 0.0, 1.0 }, /* blue */
	{ -0.5, -0.5, -0.5, 0.0, 1.0, 0.0 }, /* green */

	/* Top right */
	{ -0.5, 0.5, -0.5, 1.0, 0.0, 0.0 }, /* red */
	{ 0.5,  0.5, -0.5, 1.0, 1.0, 0.0 }, /* yellow */
	{ 0.5, -0.5, -0.5, 0.0, 0.0, 1.0 }, /* blue */

	/* Left face */
	/* Bottom left */
	{ -0.5,  0.5,  0.5, 1.0, 1.0, 1.0 }, /* white */
	{ -0.5, -0.5, -0.5, 0.0, 1.0, 0.0 }, /* green */
	{ -0.5, -0.5,  0.5, 0.0, 1.0, 1.0 }, /* cyan */

	/* Top right */
	{ -0.5,  0.5,  0.5, 1.0, 1.0, 1.0 }, /* white */
	{ -0.5,  0.5, -0.5, 1.0, 0.0, 0.0 }, /* red */
	{ -0.5, -0.5, -0.5, 0.0, 1.0, 0.0 }, /* green */

	/* Top face */
	/* Bottom left */
	{ -0.5, 0.5,  0.5, 1.0, 1.0, 1.0 }, /* white */
	{  0.5, 0.5, -0.5, 1.0, 1.0, 0.0 }, /* yellow */
	{ -0.5, 0.5, -0.5, 1.0, 0.0, 0.0 }, /* red */

	/* Top right */
	{ -0.5, 0.5,  0.5, 1.0, 1.0, 1.0 }, /* white */
	{  0.5, 0.5,  0.5, 0.0, 0.0, 0.0 }, /* black */
	{  0.5, 0.5, -0.5, 1.0, 1.0, 0.0 }, /* yellow */

	/* Right face */
	/* Bottom left */
	{ 0.5,  0.5, -0.5, 1.0, 1.0, 0.0 }, /* yellow */
	{ 0.5, -0.5,  0.5, 1.0, 0.0, 1.0 }, /* magenta */
	{ 0.5, -0.5, -0.5, 0.0, 0.0, 1.0 }, /* blue */

	/* Top right */
	{ 0.5,  0.5, -0.5, 1.0, 1.0, 0.0 }, /* yellow */
	{ 0.5,  0.5,  0.5, 0.0, 0.0, 0.0 }, /* black */
	{ 0.5, -0.5,  0.5, 1.0, 0.0, 1.0 }, /* magenta */

	/* Back face */
	/* Bottom left */
	{  0.5,  0.5, 0.5, 0.0, 0.0, 0.0 }, /* black */
	{ -0.5, -0.5, 0.5, 0.0, 1.0, 1.0 }, /* cyan */
	{  0.5, -0.5, 0.5, 1.0, 0.0, 1.0 }, /* magenta */

	/* Top right */
	{  0.5,  0.5,  0.5, 0.0, 0.0, 0.0 }, /* black */
	{ -0.5,  0.5,  0.5, 1.0, 1.0, 1.0 }, /* white */
	{ -0.5, -0.5,  0.5, 0.0, 1.0, 1.0 }, /* cyan */

	/* Bottom face */
	/* Bottom left */
	{ -0.5, -0.5, -0.5, 0.0, 1.0, 0.0 }, /* green */
	{  0.5, -0.5,  0.5, 1.0, 0.0, 1.0 }, /* magenta */
	{ -0.5, -0.5,  0.5, 0.0, 1.0, 1.0 }, /* cyan */

	/* Top right */
	{ -0.5, -0.5, -0.5, 0.0, 1.0, 0.0 }, /* green */
	{  0.5, -0.5, -0.5, 0.0, 0.0, 1.0 }, /* blue */
	{  0.5, -0.5,  0.5, 1.0, 0.0, 1.0 } /* magenta */
};

static SDL_GPUTexture*
CreateDepthTexture(Uint32 drawablew, Uint32 drawableh)
{
	SDL_GPUTextureCreateInfo createinfo;
	SDL_GPUTexture* result;

	createinfo.type = SDL_GPU_TEXTURETYPE_2D;
	createinfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
	createinfo.width = drawablew;
	createinfo.height = drawableh;
	createinfo.layer_count_or_depth = 1;
	createinfo.num_levels = 1;
	createinfo.sample_count = render_state.sample_count;
	createinfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
	createinfo.props = 0;

	result = SDL_CreateGPUTexture(gpu_device, &createinfo);
	CHECK_CREATE(result, "Depth Texture");

	return result;
}

static SDL_GPUTexture*
CreateMSAATexture(Uint32 drawablew, Uint32 drawableh)
{
	SDL_GPUTextureCreateInfo createinfo;
	SDL_GPUTexture* result;

	if (render_state.sample_count == SDL_GPU_SAMPLECOUNT_1) {
		return NULL;
	}

	createinfo.type = SDL_GPU_TEXTURETYPE_2D;
	createinfo.format = SDL_GetGPUSwapchainTextureFormat(gpu_device, state->windows[0]);
	createinfo.width = drawablew;
	createinfo.height = drawableh;
	createinfo.layer_count_or_depth = 1;
	createinfo.num_levels = 1;
	createinfo.sample_count = render_state.sample_count;
	createinfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
	createinfo.props = 0;

	result = SDL_CreateGPUTexture(gpu_device, &createinfo);
	CHECK_CREATE(result, "MSAA Texture");

	return result;
}

static SDL_GPUTexture*
CreateResolveTexture(Uint32 drawablew, Uint32 drawableh)
{
	SDL_GPUTextureCreateInfo createinfo;
	SDL_GPUTexture* result;

	if (render_state.sample_count == SDL_GPU_SAMPLECOUNT_1) {
		return NULL;
	}

	createinfo.type = SDL_GPU_TEXTURETYPE_2D;
	createinfo.format = SDL_GetGPUSwapchainTextureFormat(gpu_device, state->windows[0]);
	createinfo.width = drawablew;
	createinfo.height = drawableh;
	createinfo.layer_count_or_depth = 1;
	createinfo.num_levels = 1;
	createinfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
	createinfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
	createinfo.props = 0;

	result = SDL_CreateGPUTexture(gpu_device, &createinfo);
	CHECK_CREATE(result, "Resolve Texture");

	return result;
}

static void
Render(SDL_Window* window, const int windownum)
{
	WindowState* winstate = &window_states[windownum];
	SDL_GPUTexture* swapchainTexture;
	SDL_GPUColorTargetInfo color_target;
	SDL_GPUDepthStencilTargetInfo depth_target;
	float matrix_rotate[16], matrix_modelview[16], matrix_perspective[16], matrix_final[16];
	SDL_GPUCommandBuffer* cmd;
	SDL_GPURenderPass* pass;
	SDL_GPUBufferBinding vertex_binding;
	SDL_GPUBlitInfo blit_info;
	Uint32 drawablew, drawableh;

	/* Acquire the swapchain texture */

	cmd = SDL_AcquireGPUCommandBuffer(gpu_device);
	if (!cmd) {
		SDL_Log("Failed to acquire command buffer :%s", SDL_GetError());
		quit(2);
	}
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, state->windows[windownum], &swapchainTexture, &drawablew, &drawableh)) {
		SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
		quit(2);
	}

	if (swapchainTexture == NULL) {
		/* Swapchain is unavailable, cancel work */
		SDL_CancelGPUCommandBuffer(cmd);
		return;
	}

	/*
	* Do some rotation with Euler angles. It is not a fixed axis as
	* quaternions would be, but the effect is cool.
	*/
	rotate_matrix((float)winstate->angle_x, 1.0f, 0.0f, 0.0f, matrix_modelview);
	rotate_matrix((float)winstate->angle_y, 0.0f, 1.0f, 0.0f, matrix_rotate);

	multiply_matrix(matrix_rotate, matrix_modelview, matrix_modelview);

	rotate_matrix((float)winstate->angle_z, 0.0f, 1.0f, 0.0f, matrix_rotate);

	multiply_matrix(matrix_rotate, matrix_modelview, matrix_modelview);

	/* Pull the camera back from the cube */
	matrix_modelview[14] -= 2.5f;

	perspective_matrix(45.0f, (float)drawablew / drawableh, 0.01f, 100.0f, matrix_perspective);
	multiply_matrix(matrix_perspective, matrix_modelview, (float*)&matrix_final);

	winstate->angle_x += 3;
	winstate->angle_y += 2;
	winstate->angle_z += 1;

	if (winstate->angle_x >= 360) winstate->angle_x -= 360;
	if (winstate->angle_x < 0) winstate->angle_x += 360;
	if (winstate->angle_y >= 360) winstate->angle_y -= 360;
	if (winstate->angle_y < 0) winstate->angle_y += 360;
	if (winstate->angle_z >= 360) winstate->angle_z -= 360;
	if (winstate->angle_z < 0) winstate->angle_z += 360;

	/* Resize the depth buffer if the window size changed */

	if (winstate->prev_drawablew != drawablew || winstate->prev_drawableh != drawableh) {
		SDL_ReleaseGPUTexture(gpu_device, winstate->tex_depth);
		SDL_ReleaseGPUTexture(gpu_device, winstate->tex_msaa);
		SDL_ReleaseGPUTexture(gpu_device, winstate->tex_resolve);
		winstate->tex_depth = CreateDepthTexture(drawablew, drawableh);
		winstate->tex_msaa = CreateMSAATexture(drawablew, drawableh);
		winstate->tex_resolve = CreateResolveTexture(drawablew, drawableh);
	}
	winstate->prev_drawablew = drawablew;
	winstate->prev_drawableh = drawableh;

	/* Set up the pass */

	SDL_zero(color_target);
	color_target.clear_color.a = 1.0f;
	if (winstate->tex_msaa) {
		color_target.load_op = SDL_GPU_LOADOP_CLEAR;
		color_target.store_op = SDL_GPU_STOREOP_RESOLVE;
		color_target.texture = winstate->tex_msaa;
		color_target.resolve_texture = winstate->tex_resolve;
		color_target.cycle = true;
		color_target.cycle_resolve_texture = true;
	}
	else {
		color_target.load_op = SDL_GPU_LOADOP_CLEAR;
		color_target.store_op = SDL_GPU_STOREOP_STORE;
		color_target.texture = swapchainTexture;
	}

	SDL_zero(depth_target);
	depth_target.clear_depth = 1.0f;
	depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
	depth_target.store_op = SDL_GPU_STOREOP_DONT_CARE;
	depth_target.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
	depth_target.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
	depth_target.texture = winstate->tex_depth;
	depth_target.cycle = true;

	/* Set up the bindings */

	vertex_binding.buffer = render_state.buf_vertex;
	vertex_binding.offset = 0;

	/* Draw the cube! */

	SDL_PushGPUVertexUniformData(cmd, 0, matrix_final, sizeof(matrix_final));

	pass = SDL_BeginGPURenderPass(cmd, &color_target, 1, &depth_target);
	SDL_BindGPUGraphicsPipeline(pass, render_state.pipeline);
	SDL_BindGPUVertexBuffers(pass, 0, &vertex_binding, 1);
	SDL_DrawGPUPrimitives(pass, 36, 1, 0, 0);
	SDL_EndGPURenderPass(pass);

	/* Blit MSAA resolve target to swapchain, if needed */
	if (render_state.sample_count > SDL_GPU_SAMPLECOUNT_1) {
		SDL_zero(blit_info);
		blit_info.source.texture = winstate->tex_resolve;
		blit_info.source.w = drawablew;
		blit_info.source.h = drawableh;

		blit_info.destination.texture = swapchainTexture;
		blit_info.destination.w = drawablew;
		blit_info.destination.h = drawableh;

		blit_info.load_op = SDL_GPU_LOADOP_DONT_CARE;
		blit_info.filter = SDL_GPU_FILTER_LINEAR;

		SDL_BlitGPUTexture(cmd, &blit_info);
	}

	/* Submit the command buffer! */
	SDL_SubmitGPUCommandBuffer(cmd);

	++frames;
}

static SDL_GPUShader*
load_shader(bool is_vertex)
{
	SDL_GPUShaderCreateInfo createinfo;
	createinfo.num_samplers = 0;
	createinfo.num_storage_buffers = 0;
	createinfo.num_storage_textures = 0;
	createinfo.num_uniform_buffers = is_vertex ? 1 : 0;
	createinfo.props = 0;

	SDL_GPUShaderFormat format = SDL_GetGPUShaderFormats(gpu_device);
	if (format & SDL_GPU_SHADERFORMAT_DXIL) {
		createinfo.format = SDL_GPU_SHADERFORMAT_DXIL;
		createinfo.code = is_vertex ? D3D12_CubeVert : D3D12_CubeFrag;
		createinfo.code_size = is_vertex ? SDL_arraysize(D3D12_CubeVert) : SDL_arraysize(D3D12_CubeFrag);
		createinfo.entrypoint = is_vertex ? "VSMain" : "PSMain";
	}
	else if (format & SDL_GPU_SHADERFORMAT_METALLIB) {
		createinfo.format = SDL_GPU_SHADERFORMAT_METALLIB;
		createinfo.code = is_vertex ? cube_vert_metallib : cube_frag_metallib;
		createinfo.code_size = is_vertex ? cube_vert_metallib_len : cube_frag_metallib_len;
		createinfo.entrypoint = is_vertex ? "vs_main" : "fs_main";
	}
	else {
		createinfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
		createinfo.code = is_vertex ? cube_vert_spv : cube_frag_spv;
		createinfo.code_size = is_vertex ? cube_vert_spv_len : cube_frag_spv_len;
		createinfo.entrypoint = "main";
	}

	createinfo.stage = is_vertex ? SDL_GPU_SHADERSTAGE_VERTEX : SDL_GPU_SHADERSTAGE_FRAGMENT;
	return SDL_CreateGPUShader(gpu_device, &createinfo);
}
std::string generateUUID() {
	std::stringstream ss;
	auto t = std::chrono::system_clock::now().time_since_epoch().count();
	ss << std::hex << t;
	return ss.str();
}
static void init_render_state(int msaa)
{
	SDL_GPUCommandBuffer* cmd;
	SDL_GPUTransferBuffer* buf_transfer;
	void* map;
	SDL_GPUTransferBufferLocation buf_location;
	SDL_GPUBufferRegion dst_region;
	SDL_GPUCopyPass* copy_pass;
	SDL_GPUBufferCreateInfo buffer_desc;
	SDL_GPUTransferBufferCreateInfo transfer_buffer_desc;
	SDL_GPUGraphicsPipelineCreateInfo pipelinedesc;
	SDL_GPUColorTargetDescription color_target_desc;
	Uint32 drawablew, drawableh;
	SDL_GPUVertexAttribute vertex_attributes[2];
	SDL_GPUVertexBufferDescription vertex_buffer_desc;
	SDL_GPUShader* vertex_shader;
	SDL_GPUShader* fragment_shader;
	gpu_device = SDL_CreateGPUDevice(
		TESTGPU_SUPPORTED_FORMATS,
		true,
		state->gpudriver
	);
	CHECK_CREATE(gpu_device, "GPU device");
	int ng = SDL_GetNumGPUDrivers();
	std::vector<std::string> gpunames;
	for (size_t i = 0; i < ng; i++)
	{
		const char* a = SDL_GetGPUDriver(i);
		if (a)
			gpunames.push_back(a);
	}
	const char* aa = SDL_GetGPUDeviceDriver(gpu_device);
	/* Claim the windows */

	for (int i = 0; i < state->num_windows; i++) {
		SDL_ClaimWindowForGPUDevice(
			gpu_device,
			state->windows[i]
		);
	}

	/* Create shaders */

	vertex_shader = load_shader(true);
	fragment_shader = load_shader(false);
	{
		CHECK_CREATE(vertex_shader, "Vertex Shader");
		CHECK_CREATE(fragment_shader, "Fragment Shader");
	}

	/* Create buffers */

	buffer_desc.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	buffer_desc.size = sizeof(vertex_data);
	buffer_desc.props = SDL_CreateProperties();
	SDL_SetStringProperty(buffer_desc.props, SDL_PROP_GPU_BUFFER_CREATE_NAME_STRING, "космонавт");
	render_state.buf_vertex = SDL_CreateGPUBuffer(
		gpu_device,
		&buffer_desc
	);
	CHECK_CREATE(render_state.buf_vertex, "Static vertex buffer");
	SDL_DestroyProperties(buffer_desc.props);

	transfer_buffer_desc.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	transfer_buffer_desc.size = sizeof(vertex_data);
	transfer_buffer_desc.props = SDL_CreateProperties();
	SDL_SetStringProperty(transfer_buffer_desc.props, SDL_PROP_GPU_TRANSFERBUFFER_CREATE_NAME_STRING, "Transfer Buffer");
	buf_transfer = SDL_CreateGPUTransferBuffer(
		gpu_device,
		&transfer_buffer_desc
	);
	CHECK_CREATE(buf_transfer, "Vertex transfer buffer");
	SDL_DestroyProperties(transfer_buffer_desc.props);

	/* We just need to upload the static data once. */
	map = SDL_MapGPUTransferBuffer(gpu_device, buf_transfer, false);
	SDL_memcpy(map, vertex_data, sizeof(vertex_data));
	SDL_UnmapGPUTransferBuffer(gpu_device, buf_transfer);

	cmd = SDL_AcquireGPUCommandBuffer(gpu_device);
	copy_pass = SDL_BeginGPUCopyPass(cmd);
	buf_location.transfer_buffer = buf_transfer;
	buf_location.offset = 0;
	dst_region.buffer = render_state.buf_vertex;
	dst_region.offset = 0;
	dst_region.size = sizeof(vertex_data);
	SDL_UploadToGPUBuffer(copy_pass, &buf_location, &dst_region, false);
	SDL_EndGPUCopyPass(copy_pass);
	SDL_SubmitGPUCommandBuffer(cmd);

	SDL_ReleaseGPUTransferBuffer(gpu_device, buf_transfer);

	/* Determine which sample count to use */
	render_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
	auto tf = SDL_GetGPUSwapchainTextureFormat(gpu_device, state->windows[0]);
	if (msaa)
	{
		SDL_GPUSampleCount k[] = { SDL_GPU_SAMPLECOUNT_2,  /**< MSAA 2x */
			SDL_GPU_SAMPLECOUNT_4,  /**< MSAA 4x */
			SDL_GPU_SAMPLECOUNT_8 };   /**< MSAA 8x */
		bool r[3] = {};

		for (size_t i = 0; i < 3; i++)
		{
			r[i] = SDL_GPUTextureSupportsSampleCount(gpu_device, tf, k[i]);
		}
		if (r[2])
		{
			render_state.sample_count = k[2];
		}
		else if (r[1])
		{
			render_state.sample_count = k[1];
		}
		else if (r[0])
		{
			render_state.sample_count = k[0];
		}
	}

	/* Set up the graphics pipeline */

	SDL_zero(pipelinedesc);
	SDL_zero(color_target_desc);

	color_target_desc.format = SDL_GetGPUSwapchainTextureFormat(gpu_device, state->windows[0]);

	pipelinedesc.target_info.num_color_targets = 1;
	pipelinedesc.target_info.color_target_descriptions = &color_target_desc;
	pipelinedesc.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
	pipelinedesc.target_info.has_depth_stencil_target = true;

	pipelinedesc.depth_stencil_state.enable_depth_test = true;
	pipelinedesc.depth_stencil_state.enable_depth_write = true;
	pipelinedesc.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

	pipelinedesc.multisample_state.sample_count = render_state.sample_count;

	pipelinedesc.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

	pipelinedesc.vertex_shader = vertex_shader;
	pipelinedesc.fragment_shader = fragment_shader;

	vertex_buffer_desc.slot = 0;
	vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	vertex_buffer_desc.instance_step_rate = 0;
	vertex_buffer_desc.pitch = sizeof(VertexData);

	vertex_attributes[0].buffer_slot = 0;
	vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertex_attributes[0].location = 0;
	vertex_attributes[0].offset = 0;

	vertex_attributes[1].buffer_slot = 0;
	vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertex_attributes[1].location = 1;
	vertex_attributes[1].offset = sizeof(float) * 3;

	pipelinedesc.vertex_input_state.num_vertex_buffers = 1;
	pipelinedesc.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
	pipelinedesc.vertex_input_state.num_vertex_attributes = 2;
	pipelinedesc.vertex_input_state.vertex_attributes = (SDL_GPUVertexAttribute*)&vertex_attributes;

	pipelinedesc.props = 0;

	render_state.pipeline = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipelinedesc);
	CHECK_CREATE(render_state.pipeline, "Render Pipeline");

	/* These are reference-counted; once the pipeline is created, you don't need to keep these. */
	SDL_ReleaseGPUShader(gpu_device, vertex_shader);
	SDL_ReleaseGPUShader(gpu_device, fragment_shader);

	/* Set up per-window state */

	window_states = (WindowState*)SDL_calloc(state->num_windows, sizeof(WindowState));
	if (!window_states) {
		SDL_Log("Out of memory!");
		quit(2);
	}

	for (int i = 0; i < state->num_windows; i++) {
		WindowState* winstate = &window_states[i];

		/* create a depth texture for the window */
		SDL_GetWindowSizeInPixels(state->windows[i], (int*)&drawablew, (int*)&drawableh);
		winstate->tex_depth = CreateDepthTexture(drawablew, drawableh);
		winstate->tex_msaa = CreateMSAATexture(drawablew, drawableh);
		winstate->tex_resolve = CreateResolveTexture(drawablew, drawableh);

		/* make each window different */
		winstate->angle_x = (i * 10) % 360;
		winstate->angle_y = (i * 20) % 360;
		winstate->angle_z = (i * 30) % 360;
	}
}

static int done = 0;

void loop(void)
{
	SDL_Event event;
	int i;

	/* Check for events */
	while (SDL_PollEvent(&event) && !done) {
		SDLTest_CommonEvent(state, &event, &done);
	}
	if (!done) {
		for (i = 0; i < state->num_windows; ++i) {
			Render(state->windows[i], i);
		}
	}
#ifdef __EMSCRIPTEN__
	else {
		emscripten_cancel_main_loop();
	}
#endif
}

int sdlmain(int argc, char* argv[])
{
	int msaa;
	int i;
	const SDL_DisplayMode* mode;
	Uint64 then, now;

	/* Initialize params */
	msaa = 1;

	/* Initialize test framework */
	state = SDLTest_CommonCreateState(argv, SDL_INIT_VIDEO);
	if (!state) {
		return 1;
	}
	for (i = 1; i < argc;) {
		int consumed;

		consumed = SDLTest_CommonArg(state, i);
		if (consumed == 0) {
			if (SDL_strcasecmp(argv[i], "--msaa") == 0) {
				++msaa;
				consumed = 1;
			}
			else {
				consumed = -1;
			}
		}
		if (consumed < 0) {
			static const char* options[] = { "[--msaa]", NULL };
			SDLTest_CommonLogUsage(state, argv[0], options);
			quit(1);
		}
		i += consumed;
	}

	state->skip_renderer = 1;
	state->window_flags |= SDL_WINDOW_RESIZABLE;

	if (!SDLTest_CommonInit(state)) {
		quit(2);
		return 0;
	}

	mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(state->windows[0]));
	SDL_Log("Screen bpp: %d", SDL_BITSPERPIXEL(mode->format));

	init_render_state(msaa);

	/* Main render loop */
	frames = 0;
	then = SDL_GetTicks();
	done = 0;

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(loop, 0, 1);
#else
	while (!done) {
		loop();
	}
#endif

	/* Print out some timing information */
	now = SDL_GetTicks();
	if (now > then) {
		SDL_Log("%2.2f frames per second",
			((double)frames * 1000) / (now - then));
	}
#if !defined(__ANDROID__)
	quit(0);
#endif
	return 0;
}
SDL_GPUTexture*
CreateDepthTexture(SDL_GPUDevice* gpu_device, Uint32 drawablew, Uint32 drawableh)
{
	SDL_GPUTextureCreateInfo createinfo;
	SDL_GPUTexture* result;

	createinfo.type = SDL_GPU_TEXTURETYPE_2D;
	createinfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
	createinfo.width = drawablew;
	createinfo.height = drawableh;
	createinfo.layer_count_or_depth = 1;
	createinfo.num_levels = 1;
	createinfo.sample_count = SDL_GPU_SAMPLECOUNT_1;// render_state.sample_count;
	createinfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
	createinfo.props = 0;

	result = SDL_CreateGPUTexture(gpu_device, &createinfo);


	return result;
}
void new_gpu(int argc, char* argv[]) {
	sdlmain(argc, argv);
	exit(0);
	auto dev = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_METALLIB, true, 0);
	SDL_GPUTexture* depth_texture = nullptr;
	if (dev)
	{
		depth_texture = CreateDepthTexture(dev, 1024, 1024);
	}
	auto dt = (SDL_GPUTextureCreateInfo*)depth_texture;
	printf("");
}
#endif // 1








class vcpkg_cx
{
public:
	std::queue<std::string> cmds;
	std::mutex _lock;
	std::string rootdir; // vcpkg根目录
	njson vlist;
	njson vsearch;

public:
	vcpkg_cx();
	~vcpkg_cx();
	void do_clone(const std::string& dir, int depth);
	void do_bootstrap();
	void do_pull();
	void do_update(const std::string& t);
	void do_search();
	void do_list();
	void do_integrate_i();
	void do_integrate_r();
	void do_get_triplet();
	void do_install(const std::string& t, const std::string& triplet);
	void do_remove(const std::string& t, const std::string& triplet);
	void push_cmd(const std::string& c);
	void do_cmd();

private:
	bool is_valid_rootdir() const;
	void add_cmd(const std::string& cmd);
};

vcpkg_cx::vcpkg_cx() {}
vcpkg_cx::~vcpkg_cx() {}

bool vcpkg_cx::is_valid_rootdir() const
{
	return rootdir.size() >= 2;
}

void vcpkg_cx::add_cmd(const std::string& cmd)
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push(cmd);
}

void vcpkg_cx::do_clone(const std::string& dir, int depth)
{
	if (dir.size() < 2) return;
	std::lock_guard<std::mutex> lock(_lock);
	rootdir = dir;
	std::string c = "git clone https://github.com/microsoft/vcpkg.git";
	if (depth > 0)
	{
		c += " --depth=" + std::to_string(depth);
	}
	cmds.push(c);
}

void vcpkg_cx::do_bootstrap()
{
	if (!is_valid_rootdir()) return;
	add_cmd(
#ifdef _WIN32
		"bootstrap-vcpkg.bat"
#else
		"bootstrap-vcpkg.sh"
#endif
	);
}

void vcpkg_cx::do_pull()
{
	if (!is_valid_rootdir()) return;
	add_cmd("git pull");
}

void vcpkg_cx::do_update(const std::string& t)
{
	add_cmd("vcpkg update");
}

void vcpkg_cx::do_search()
{
	add_cmd("vcpkg search --x-full-desc --x-json");
}

void vcpkg_cx::do_list()
{
	add_cmd("vcpkg list --x-full-desc --x-json");
}

void vcpkg_cx::do_integrate_i()
{
	add_cmd("vcpkg integrate install");
}

void vcpkg_cx::do_integrate_r()
{
	add_cmd("vcpkg integrate remove");
}

void vcpkg_cx::do_get_triplet()
{
	add_cmd("vcpkg help triplet");
}

void vcpkg_cx::do_install(const std::string& t, const std::string& triplet)
{
	std::string c = "vcpkg install " + t;
	if (!triplet.empty())
	{
		c += ":" + triplet;
	}
	add_cmd(c);
}

void vcpkg_cx::do_remove(const std::string& t, const std::string& triplet)
{
	std::string c = "vcpkg remove " + t;
	if (!triplet.empty())
	{
		c += ":" + triplet;
	}
	add_cmd(c);
}

void vcpkg_cx::push_cmd(const std::string& c)
{
	add_cmd(c);
}

void vcpkg_cx::do_cmd()
{
	std::string c;
	while (!cmds.empty())
	{
		{
			std::lock_guard<std::mutex> lock(_lock);
			c.swap(cmds.front());
			cmds.pop();
		}
		if (c.empty()) continue;
		auto jstr = hz::cmdexe(c, rootdir.empty() ? nullptr : rootdir.c_str());
		try
		{
			njson k = njson::parse(jstr);
			if (!k.empty())
				vlist = k;
		}
		catch (const std::exception& e)
		{
			printf("json error:\t%s\n", e.what());
		}
		printf("%s\n", jstr.c_str());
	}
}

#if 1
// hexedit

#define HEX_EDITOR_STATIC_LIB
#include <hex_editor.h>

struct pack_hex_t
{
	hex_editor* hex = 0;
	scroll2_t hex_scroll = {};
	int fl = 0;
	glm::vec2 hex_size = { 900,600 };
	int width = 16;
	//int rcw = 14;
	plane_cx* p = 0;
	size_t dpx = 0;
};
void loadfile(pack_hex_t* ph, const std::string& str) {

	if (ph->hex->set_file(str.c_str(), true))
	{
		ph->hex->update_hex_editor();
		ph->hex->set_pos(0);
		ph->hex_scroll.v->set_viewsize(ph->hex_size.y, (ph->hex->acount + 5) * ph->fl, 0);
	}
}
void downmuose(pack_hex_t* ph, plane_ev* e) {
	auto dps = ph->p->get_dragpos(ph->dpx);//获取拖动时的坐标 
	auto dgp = ph->p->get_dragv6(ph->dpx);
	dgp->size.x = -1;
	glm::ivec2 scpos = e->mpos - ph->p->tpos;// 减去本面板坐标
	scpos -= (glm::ivec2)dps;
	//printf("old click:%d\t%d\n", scpos.x, scpos.y);
	auto hps = ph->hex_scroll.h->get_offset_ns();
	auto vps = ph->hex_scroll.v->get_offset_ns();
	scpos -= (glm::ivec2)ph->hex->text_rc[2];
	ph->hex->on_mouse(e->clicks, scpos, hps, vps);
	scpos.x += hps;
	scpos.y += vps;
}
void draw_hex(pack_hex_t* ph, cairo_t* cr)
{
	auto dps = ph->p->get_dragpos(ph->dpx);//获取拖动时的坐标 
	{
		cairo_as _ss_(cr);
		cairo_translate(cr, dps.x, dps.y);

		glm::ivec2 hps = dps;
		glm::vec4 chs = { 0,0,ph->hex_size };
		hps.y += ph->hex_size.y - ph->width;
		ph->hex_scroll.h->pos = hps;
		dps.x += ph->hex_size.x - ph->width;
		ph->hex_scroll.v->pos = dps;
		glm::vec4 bgrc = { -2.5,  -2.5,ph->hex_size.x + 3,ph->hex_size.y + 3 };
		auto phex = ph->hex;
		glm::vec4 rc = { 0,0,300,1000 };
		if (phex->acount == 0)
		{
			draw_rect(cr, bgrc, 0xf0121212, 0x80ffffff, 2, 1);
			return;
		}
		auto st = get_defst(1);
		auto fl = ph->p->ltx->get_lineheight(st->font, st->font_size);
		auto pxx = ph->hex_scroll.h->get_offset_ns();
		auto pyy = ph->hex_scroll.v->get_offset_ns();
		auto vps = pyy / fl;
		pyy = vps * fl;
		hex_style_t hst = {};
		hst.st = st;
		hst.ltx = ph->p->ltx;
		hst.pxx = pxx;
		hst.pyy = pyy;
		hst.fl = fl;
		hst.hex_size = ph->hex_size;
		hst.bgrc = bgrc;
		hst.view_size = { ph->hex_scroll.h->_view_size,ph->hex_scroll.v->_view_size };
		hst.cr = cr;
		phex->update_draw(&hst);
		auto dt = phex->get_drawt();
		if (dt->box_rc.z != ph->hex_scroll.h->_content_size)
			ph->hex_scroll.h->set_viewsize(ph->hex_size.x, dt->box_rc.z + fl, 0);
	}
}
pack_hex_t* testhex(plane_cx* p) {
	auto hex = new hex_editor();
	glm::vec2 hex_size = { 900,600 };
	int width = 16;
	int rcw = 14;
	auto dpx = p->push_dragpos({ 20,60 }, hex_size);// 增加一个拖动坐标
	auto rint = get_rand64(0, (uint32_t)-1);

	auto ft = p->ltx->ctx;
	int fc = ft->get_count();						// 获取注册的字体数量
	const char* get_family(int idx);		// 获取字体名称
	const char* get_family_en(const char* family);		// 获取字体名称
	const char* get_family_full(int idx);	// 获取字体全名
	struct ft_info
	{
		std::string name, full, style;
	};
	static std::vector<ft_info> fvs;
	fvs.resize(fc);
	static std::vector<std::string> ftns;
	static std::string kc = (char*)"ab 串口fad1\r\n231ffwfadfsfgdfgdfhjhg";
	auto g = md::gb_u8((char*)kc.c_str(), -1);
	auto nhs = hex_size;
	nhs.y -= width;
	auto st = get_defst(1);
	auto fl = p->ltx->get_lineheight(st->font, st->font_size);
	hex->set_linechar(fl, p->ltx->get_text_rect1(st->font, st->font_size, "x").x);
	hex->set_view_size(nhs);
	auto hex_scroll = p->add_scroll2(hex_size, width, rcw, { fl, fl }, { 2 * 0,0 }, { 0,2 * 0 });
	hex_scroll.v->set_viewsize(hex_size.y, (hex->acount + 5) * fl, 0);
	hex_scroll.h->set_viewsize(hex_size.x, hex_size.x, 0);
	hex_scroll.v->has_hover_sc = true;
	hex_scroll.v->hover_sc = true;
	auto ph = new pack_hex_t();
	ph->fl = fl;
	ph->p = p;
	ph->hex = hex;
	ph->hex_scroll = hex_scroll;
	ph->hex_size = hex_size;
	ph->dpx = dpx;
	ph->width = width;

	return ph;
}
#endif // 0



std::function<void(void* p, int type, int id)> mmcb;
void mcb(void* p, int type, int id) {
	if (mmcb)
		mmcb(p, type, id);
}

menu_cx* menu_m(form_x* form0)
{
	menumain_info m = {};
	m.form0 = form0;
	m.fontn = fontn;
	// 主菜单
	std::vector<std::string> mvs = { (char*)u8"文件",(char*)u8"编辑",(char*)u8"视图",(char*)u8"工具",(char*)u8"帮助" };
	m.mvs = &mvs;
	m.cb = [=](mitem_t* p, int type, int id) {
		//mcb(p, type, id);
		};
	m.mcb = [=](void* p, int clicks, int id) {
		mcb(p, clicks, id);
		};
	auto mc = mg::new_mm(&m);
	return mc;
}
void tohexstr(std::string& ot, const char* d, int len, int xlen, uint64_t line, bool n16) {
	auto hs = pg::to_string_hex(line, n16 ? 16 : 8, "x");
	hs += ": ";
	auto aa = std::string(d, len);
	bool isu8 = hz::is_utf8(d, len);
	if (!isu8)
	{
		auto u = hz::gbk_to_u8(aa);
		if (u.size())aa = u;
	}
	for (auto& it : aa) {
		if (it == 0)
		{
			it = '.';
		}
	}

	{
		unsigned int cp1 = 0;
		auto uc = md::get_utf8_count(aa.c_str(), aa.size());
		if (uc > 1)
		{
			auto laststr = aa.c_str();
			std::string c, c0;
			for (int k = 0; k < uc; k++)
			{
				auto t = laststr;
				laststr = md::utf8_next_char(laststr);
				c.assign(t, laststr);
				if (c.size() == 1)
				{
					if ((*t > 0 && !isprint(*t)) || *t < 0)
						c = ".";
				}
				c0 += c;
			}
			aa.swap(c0);
		}
	}
}
plane_cx* show_ui(form_x* form0, menu_cx* gm)
{
	if (!form0)return 0;
	glm::ivec2 size = { 1024,800 };
	glm::ivec2 cview = { 1580,1580 };
	auto p = new plane_cx();
	uint32_t pbc = 0xff2c2c2c;
	p->set_color({ 0x80ff802C,1,5,pbc });
	form0->bind(p);	// 绑定到窗口  
	//build_spr_ui(form0);
	p->set_rss(5);
	p->_lms = { 8,8 };
	p->add_familys(fontn, 0);
	p->add_familys(fontn1, 0);
	p->add_familys(fontn2, 0);
	p->draggable = false; //可拖动
	p->set_size(size);
	p->set_pos({ 1,30 });
	p->set_select_box(2 * 0, 0.012);	// 设置是否显示选择框

	p->fontsize = 16;
	int width = 16;
	int rcw = 14;
	{
		// 设置带滚动条
		p->set_scroll(width, rcw, { 4, 4 }, { 2 * 0,0 }, { 0,2 * 0 });
		//p->set_scroll_hide(0);
		p->set_view(size, cview);
		p->vertical->hover_sc = false;
		p->vertical->has_hover_sc = false;
	}
	size.y -= 50;
	size.x -= 50 * 10;
	size /= 2;
	vcpkg_cx* vx = new vcpkg_cx();
	std::vector<const char*> btnname = {
	"install",//安装库(const std::string & t, const std::string & triplet); 
	"remove",//删除库(const std::string & t, const std::string & triplet); 
	"clone",//(const std::string & dir, int depth); 
	"bootstrap",// 
	"update",//比较更新了哪些库(const std::string & t);	 
	"ecmd",//执行自定义命令(const std::string & c); 
	};
	std::vector<const char*> mname = {
	"list",//列出安装库 
	"search",//搜索库 
	"pull",//拉取  	 
	"get triplet",//查看支持的架构  
	"integrate install",//集成到全局 
	"integrate remove",//移除全局 
	};
	auto color = 0x2c2c2c;// hz::get_themecolor();
	((uint8_t*)(&color))[3] = 0x80;

	//form_x* form1 = (form_x*)new_form(form0->app, "", 600,500, -1, -1, ef_vulkan);
	//auto cp = p->add_gbutton((char*)u8"功能", { 160,30 }, color);
	//cp->click_cb = [=](void* ptr, int clicks)
	//	{
	//		form1->flash_window(2);
	//	};
#if 0
	std::vector<menu_info> vm;
	vm.push_back({ mname.data(),mname.size() });
	auto mms = gm->new_menu_g(vm.data(), vm.size(), { 200,30 }, [=](mitem_t* pm, int type, int idx)
		{
			if (type == 1)
			{
				printf("click:%d\t%d\n", type, idx);
			}
		});
	//for (auto& nt : btnname)
	{
		auto cp = p->add_gbutton((char*)u8"功能", { 160,30 }, color);
		cp->click_cb = [=](void* ptr, int clicks)
			{
				auto cps = cp->get_pos();
				cps.y += cp->size.y + cp->thickness;
				gm->show_mg(mms, 0, cps);
			};
	}
#else
	//for (auto& nt : mname)
	//{
	//	auto cp = p->add_cbutton(nt, { 180,30 }, 0);
	//	cp->effect = uTheme::light;
	//	cp->click_cb = [=](void* ptr, int clicks)
	//		{
	//			auto pb = (color_btn*)ptr;
	//			printf("%d\n", clicks);
	//			print_time aa("list");
	//			vx->do_list();
	//			vx->do_cmd();
	//		};
	//}
	//auto ce = p->add_input("", { 1000,30 }, 1);

	//for (auto& nt : btnname)
	//{
	//	auto cp = p->add_gbutton(nt, { 160,30 }, color);
	//	cp->click_cb = [=](void* ptr, int clicks)
	//		{
	//		};
	//}

#endif
	auto ce = p->add_input("", { 500,30 }, 1);
	int cs[] = { sizeof(glm::vec1),sizeof(glm::vec2),sizeof(glm::vec3),sizeof(glm::ivec3) };
	auto a4 = glm::translate(glm::vec3(1.2, 0.2, 1.3));
	auto a40 = glm::translate(glm::vec3(0.2, 0.2, 1.3));
	auto aa = a4 * a40;
	glm::vec4 a = glm::vec4(1.0, 2.2, 3.0, 1.0);
	auto b = glm::vec4(0.1, .2, .3, 1.0);
	auto c = a * b;
	size /= 2;
	glm::vec2 hex_size = { 900,600 };
	auto dpx = p->push_dragpos({ 20,60 }, hex_size);// 增加一个拖动坐标
	auto rint = get_rand64(0, (uint32_t)-1);

	auto ft = p->ltx->ctx;
	int fc = ft->get_count();						// 获取注册的字体数量
	const char* get_family(int idx);		// 获取字体名称
	const char* get_family_en(const char* family);		// 获取字体名称
	const char* get_family_full(int idx);	// 获取字体全名
	struct ft_info
	{
		std::string name, full, style;
	};
	static std::vector<ft_info> fvs;
	fvs.resize(fc);
	static std::vector<std::string> ftns;
	static std::string kc = (char*)"ab 串口fad1\r\n231ffwfadfsfgdfgdfhjhg";
	auto g = md::gb_u8((char*)kc.c_str(), -1);
	auto nhs = hex_size;
	nhs.y -= width;
	auto st = get_defst(1);
	auto fl = p->ltx->get_lineheight(st->font, st->font_size);

	//{
	//	print_time aak("load file");
	//	if (hex.set_file(R"(E:\迅雷下载\g\tenoke-romance.of.the.three.kingdoms.8.remake.iso)", true))
	//	{
	//		hex.update_hex_editor();
	//		hex.set_pos(0);
	//		//kc.assign((char*)hex.data(), hex.size());
	//	}
	//}
	std::string k;
	for (size_t i = 0; i < fc; i++)
	{
		int cs = ft->get_count_style(i);			// 获取idx字体风格数量
		auto& it = fvs[i];
		it.name = ft->get_family_cn(i);
		it.full = ft->get_family_full(i);
		for (size_t c = 0; c < cs; c++)
		{
			auto sty = ft->get_family_style(i, c);
			it.style += sty; it.style.push_back(';');
		}
		{
			if (i > 0 && (i % 30 == 0))
			{
				ftns.push_back(k); k.clear();
			}
			k += it.name + "\n";
		}
	}
	auto ftff = ft->get_font("Source Han Sans SC", 0);
	if (k.size())
		ftns.push_back(k);
	//p->add_colorpick(0, 280, 30, true);
#if 0
	hz::mcurl_cx mc;
	{
		auto kv = hz::read_json("temp/aiv.json");
		mc.set_httpheader("Authorization", "Bearer " + kv["v"][0].get<std::string>());
		njson aa;
		aa["model"] = "deepseek-chat";
		aa["stream"] = false;
		aa["temperature"] = 0.7;
		std::string ct = (char*)u8"你是谁";
		aa["messages"] = { {{"role", "user"}, {"content",ct.c_str()}} };
		auto aad = aa.dump();
		mc.post("https://api.deepseek.com/chat/completions", aad.data(), aad.size());
		//mc.get("https://api.deepseek.com/user/balance");//查余额
		if (mc._data.size())
		{
			auto dd = (char*)mc._data.data();
			auto n = njson::parse(dd, dd + mc._data.size());
			//hz::save_json("temp/ai_get.json", n, 2);
		}
		printf("%s\n", mc._data.data());
	}
	if (0) {
		cmde_cx cmdx([](const char* str, int len)
			{
				auto s = md::gb_u8(str, len);
				printf("%s", s.c_str());
			});
		while (1) {
			static bool k = false;
			static bool c = false;
			if (k)
			{
				cmdx.write_str("cd E:\\aicpp\n"); k = false;
			}
			if (c)
			{
				//cmdx.write_str("exit\n");//退出cmd
				c = false;
				break;
			}
			Sleep(100);
		}
	}
#endif
	auto hex_edit = p->add_input("", { fl * 3,fl }, 1);
	hex_edit->_absolute = true;
	hex_edit->visible = false;
	pack_hex_t* ph = testhex(p);
	static int stt = 0;
	mmcb = [=](void* p, int type, int id) {
		if (id > 0)return;
		auto fn = hz::browse_openfile("选择文件", "", "所有文件\t*.*\tPNG格式(*.png)\t*.png\tJPEG(*.jpg)\t*.jpg\t", form0->get_nptr(), 0);
		if (fn.size()) {
			std::string str = fn[0];

			{
				print_time aak("load file");
				loadfile(ph, str);
			}
		}
		};
	p->add_mouse_cb([=](plane_ev* e)
		{
			if (e->down)
			{
				downmuose(ph, e);
			}
		});
	p->update_cb = [=](float delta)
		{
			return 0;
		};
	p->draw_front_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
		};
	p->draw_back_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
			auto f = ft->get_font("a", 0);
			uint32_t color = 0x80FF7373;// hz::get_themecolor(); 
			draw_hex(ph, cr);

		};

	return p;
}

void show_ui2(form_x* form0, menu_cx* gm)
{
	if (!form0)return;
}
void test_img() {

	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr.png", 0);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr60.jpg", 60);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr80.jpg", 80);
}
void clearpdb()
{
#ifdef _DEBUG 
	system("rd /s /q E:\\temcpp\\SymbolCache\\vkcmp.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\cedit.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\p86.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\mtl.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\mw.pdb");
#endif 
}

// 计算网格点
SDL_FPoint* generateGridPoints(int gridSize, int canvasSize, std::vector<SDL_FPoint>* opt) {
	int lines = static_cast<int>(std::ceil(static_cast<float>(canvasSize) / gridSize));
	int totalPoints = lines * 2 * 2 + 4; // 水平+垂直线，每线2个点
	if (opt->size() != totalPoints)
	{
		opt->resize(totalPoints);
	}
	SDL_FPoint* points = opt->data();
	int index = 0;
	float a = lines * gridSize;
	// 水平线 
	for (int y = 0; y <= canvasSize; y += gridSize) {
		points[index++] = { 0.0f, static_cast<float>(y) };
		points[index++] = { a, static_cast<float>(y) };
	}

	// 垂直线 
	for (int x = 0; x <= canvasSize; x += gridSize) {
		points[index++] = { static_cast<float>(x), 0.0f };
		points[index++] = { static_cast<float>(x), a };
	}

	return points;
}

// 绘制网格 
void genGrid(int gridSize, int canvasSize, std::vector<SDL_FPoint>* opt) {
	generateGridPoints(gridSize, canvasSize, opt);
}
void drawGrid(SDL_Renderer* renderer, const glm::vec2& pos, int gridSize, int canvasSize, std::vector<SDL_FPoint>* opt) {
	SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 1.0); // 设置线条颜色为黑色 
	SDL_FPoint* points = opt->data();
	auto length = opt->size() / 2;
	for (size_t i = 0; i < length; i++)
	{
		auto it = points[0];
		auto it1 = points[1];
		SDL_RenderLine(renderer, it.x + pos.x, it.y + pos.y, it1.x + pos.x, it1.y + pos.y);
		points += 2;
	}
}
void spriteUnravel(const std::string& cv, std::vector<int>& window_paletteref, int window_digitsize, std::vector<uint32_t>& output)
{
	std::vector<int> paletteref;
	std::string tt;
	auto t = cv.c_str();
	tt.assign(t, 2);
	auto width = std::atoi(tt.c_str());
	t += 2;
	output.push_back(width);
	int digitsize = window_digitsize;
	int current = 0;
	paletteref = window_paletteref;
	for (; *t; t++)
	{
		auto c = *t;
		switch (c)
		{
		case 'x':
		{
			// A loop, ordered as 'x char times ,'
			// Get the location of the ending comma 
			t++;
			tt.assign(t, digitsize);
			auto cc = std::atoi(tt.c_str());
			t += digitsize;
			auto t0 = t;
			for (; *t != ','; t++);
			tt.assign(t0, t - t0);
			auto n = std::atoi(tt.c_str());
			current = paletteref[cc];
			for (auto k = 0; k < n; k++)
				output.push_back(current);
		}
		break;
		case 'p':
		{
			// A palette changer, in the form 'p[X,Y,Z...]' (or 'p' for default)
			t++;
			c = *t;
			// If the next character is a '[', customize. 
			if (c == '[')
			{
				t++;
				auto t0 = t;
				for (; *t != ']'; t++);
				tt.assign(t0, t - t0);
				auto locc = md::split(tt, ",");
				paletteref.clear();
				paletteref.reserve(locc.size());
				for (auto& it : locc) {
					paletteref.push_back(std::atoi(it.c_str()));
				}
				//nixloc = colors.indexOf(']');
				// Isolate and split the new palette's numbers
				//paletteref = getPaletteReference(colors.slice(loc + 1, nixloc).split(","));
				//loc = nixloc + 1;
				digitsize = 1;
			}
			// Otherwise go back to default
			else {
				paletteref = window_paletteref;
				digitsize = window_digitsize;
			}
		}
		break;
		default:
		{
			// A typical number 
			tt.assign(t, digitsize);
			t += digitsize - 1;
			auto cc = std::atoi(tt.c_str());
			output.push_back(paletteref[cc]);
		}
		break;
		}
	}
}
void copy2image(image_ptr_t& ipt, const glm::ivec2& pos, int width, std::vector<glm::i8vec4>& palette, uint32_t* colors, int csize)
{
	if (4 != ipt.comp || !ipt.width || !ipt.height || !ipt.data || width >= ipt.width)return;
	auto dd = colors;
	auto pal = palette.data();
	int stride = ipt.stride ? ipt.stride / sizeof(uint32_t) : ipt.width;
	if (dd) {
		int ch = csize / width;
		auto h = std::min(ch + pos.y, ipt.height);
		auto w = std::min(width + pos.x, ipt.width);
		auto img = ipt.data + pos.y * stride;
		for (size_t y = pos.y; y < h; y++)
		{
			for (size_t x = pos.x; x < w; x++)
			{
				uint32_t* col = (uint32_t*)&pal[*dd];
				img[x] = *col;
				dd++;
			}
			img += stride;
		}
	}

}
int get_width(const std::string& k, njson& d, int& iw)
{
	int r = 0;
	for (auto& [n, v] : d.items()) {
		if (k.find(n) == 0)
		{
			r = v[0].get<int>() * 2;
			iw = r;
			break;
		}
	}
	return r;
}
void makemjson(const char* fn) {

	std::vector<glm::i8vec4> palette = {
		{0, 0, 0, 0},
		// Grayscales (1-4)
		{255, 255, 255, 255},
		{0, 0, 0, 255},
		{188, 188, 188, 255},
		{116, 116, 116, 255},
		// Reds & Browns (5-11)
		{252, 216, 168, 255},
		{252, 152, 56, 255},
		{252, 116, 180, 255},
		{216, 40, 0, 255},
		{200, 76, 12, 255},
		{136, 112, 0, 255},
		{124, 7, 0, 255},
		// Greens (12-14, and 21)
		{168, 250, 188, 255},
		{128, 208, 16, 255},
		{0, 168, 0, 255},
		// Blues (15-20)
		{24, 60, 92, 255},
		{0, 128, 136, 255},
		{32, 56, 236, 255},
		{156, 252, 240, 255},
		{60, 188, 252, 255},
		{92, 148, 252, 255},
		//Greens (21) for Luigi
		{0, 130, 0, 255}
	};
	struct sd_t
	{
		njson* p = 0;
		std::string k;
	};
	auto md = hz::read_json(fn);
	auto rawsprites = md["rawsprites"];
	std::stack<sd_t> sv;
	std::stack<int> st;
	for (auto& [k, v] : rawsprites.items()) {
		if (v.size() && v.is_object())
		{
			sv.push({ &v, k });
		}
	}
	int window_digitsize = 2;
	std::vector<int> window_paletteref;
	std::vector<int> paletteref;
	std::vector<uint32_t> output;
	window_paletteref.resize(palette.size());
	for (size_t i = 0; i < palette.size(); i++)
	{
		window_paletteref[i] = i;
	}
	auto Scenery = md["Scenery"]["sprites"];
	std::map<std::string, std::vector<uint32_t>> mcd;
	static int sckk = 300;
	static int minck = 280;
	int ckk = 0;
	while (sv.size()) {
		auto pt = sv.top();
		sv.pop();
		auto p = pt.p;
		for (auto& [k, v] : p->items()) {
			auto nk = k;
			if (pt.k.size())
			{
				nk = pt.k + "_" + k;
			}
			if (v.is_object())
			{
				if (v.size())
					sv.push({ &v, nk });
			}
			else if (v.is_string())
			{
				auto vs = v.get<std::string>();
				spriteUnravel(vs, window_paletteref, window_digitsize, output);
				if (output.size())
				{
					mcd[nk].swap(output);
					if (mcd.find(nk) != mcd.end())output.clear();
				}
			}
		}
	}
	image_ptr_t ipt = {};
	ipt.comp = 4;
	ipt.width = 1024;
	ipt.height = 1024;
	std::vector<uint32_t> dimg;
	dimg.resize(1024 * 1024);
	ipt.data = dimg.data();
	int aw = 0;
	int ah = 0;
	std::map<size_t, int> ss;
	packer_base* pack = new_packer(1024, 1024);
	std::vector<std::string> vname;

	std::vector<glm::ivec4> rcv;
	struct vnk_t
	{
		std::vector<uint32_t>* v;
		std::string k;
	};
	std::vector<vnk_t> rcvd;
	njson nw;
	for (auto& [k, v] : mcd) {
		//if (ckk > sckk)break;
		//ckk++;
		//if (ckk < minck)
		//{
		//	continue;
		//}
		vname.push_back(k);
		auto& data = v;
		auto ds = data.size() - 1;
		int iw = data[0];
		ss[ds]++;
		glm::ivec2 npos = {};
		auto ih = (ds / iw) + 4;
		rcv.push_back({ 0,0, iw + 4, ih });
		rcvd.push_back({ &v ,k });
	}
	pack->push_rect(rcv.data(), rcv.size());
	auto length = rcv.size();
	for (size_t i = 0; i < length; i++)
	{
		auto rc = rcv[i];
		auto& v = rcvd[i];
		auto ds = v.v->size() - 1;
		copy2image(ipt, rc, rc.z - 4, palette, v.v->data() + 1, ds);
		nw[v.k] = { rc.x,rc.y,rc.z - 4,rc.w - 4 };
	}
	hz::save_json("temp/mario2.json", nw, 1);
	save_img_png(&ipt, "temp/mario2.png");
	printf("pack size:%d\t%d\n", pack->width, pack->height);

}

// 计算平面距离 
double distance2d(const glm::dvec3& a, const glm::dvec3& b)
{
	return glm::distance((glm::dvec2)a, (glm::dvec2)b);
}
// 3D距离
double distance3d(const glm::dvec3& a, const glm::dvec3& b)
{
	return glm::distance(a, b);
}
// 计算平面欧几里得距离 
double distance2d2(const glm::dvec3& a, const glm::dvec3& b)
{
	return glm::distance2((glm::dvec2)a, (glm::dvec2)b);
}
// 3D欧几里得距离
double distance3d2(const glm::dvec3& a, const glm::dvec3& b)
{
	return glm::distance2(a, b);
}


// 计算区域面积，负数则环是逆时针，首尾坐标必需一样组成多边形环
template<class T>
double area_ring(T* xx, T* yy, size_t count)
{
	std::size_t rlen = count;
	if (rlen < 3 || !xx || !yy) { return 0.0; }
	double sum = 0.0;
	double x0 = xx[0];
	for (std::size_t i = 1; i < rlen - 1; i++) {
		double x = xx[i] - x0;
		double y1 = yy[i + 1];
		double y2 = yy[i - 1];
		sum += x * (y2 - y1);
	}
	return sum / 2.0;
}
// 输入泄露点坐标，范围，节点坐标。遍历检测受影响要素
std::vector<glm::dvec3> findAffectedPoints(const glm::dvec3& leak, const std::vector<glm::dvec3>& allPoints, double radius)
{
	std::vector<glm::dvec3> results;
	auto r2 = radius;
	for (const auto& pt : allPoints) {
		// 基于欧氏距离计算受影响范围
		if (glm::distance2(leak, pt) <= r2)
			results.push_back(pt);
	}
	return results;
}
struct PipeSegment { glm::dvec3 start, end; }; // 管段数据结构 
//1判断管线是否连通，连通性修复策略 ：延长孤立管段至最近邻管段（三维空间最近邻搜索）


#if 0
class KDNode {
public:
	glm::dvec3 point;
	std::shared_ptr<KDNode> left, right;
	int axis; // 分割轴 (0=x,1=y,2=z)

	KDNode(glm::dvec3 p, int a) : point(p), axis(a) {}
};

std::shared_ptr<KDNode> buildKDTree(std::vector<glm::dvec3>& points, int depth = 0) {
	if (points.empty())  return nullptr;

	int axis = depth % 3;
	auto mid = points.begin() + points.size() / 2;
	std::nth_element(points.begin(), mid, points.end(),
		[axis](const glm::dvec3& a, const glm::dvec3& b) {
			return (axis == 0) ? (a.x < b.x) :
				(axis == 1) ? (a.y < b.y) : (a.z < b.z);
		});

	auto node = std::make_shared<KDNode>(*mid, axis);
	node->left = buildKDTree({ points.begin(),  mid }, depth + 1);
	node->right = buildKDTree({ mid + 1, points.end() }, depth + 1);
	return node;
}

void nearestNeighborSearch(const std::shared_ptr<KDNode>& root,
	const glm::dvec3& target,
	glm::dvec3& best,
	double& best_dist) {
	if (!root) return;

	double dx = target.x - root->point.x;
	double dy = target.y - root->point.y;
	double dz = target.z - root->point.z;
	double dist = dx * dx + dy * dy + dz * dz;

	if (dist < best_dist) {
		best_dist = dist;
		best = root->point;
	}

	int axis = root->axis;
	double target_val = (axis == 0) ? target.x :
		(axis == 1) ? target.y : target.z;
	double node_val = (axis == 0) ? root->point.x :
		(axis == 1) ? root->point.y : root->point.z;

	auto& first = (target_val <= node_val) ? root->left : root->right;
	auto& second = (target_val <= node_val) ? root->right : root->left;

	nearestNeighborSearch(first, target, best, best_dist);

	if ((target_val - node_val) * (target_val - node_val) < best_dist) {
		nearestNeighborSearch(second, target, best, best_dist);
	}
}

glm::dvec3 findNearestPointOptimized(const glm::dvec3& target,
	const std::vector<glm::dvec3>& points) {
	if (points.empty())  throw std::invalid_argument("Empty collection");

	auto mutable_points = points; // 拷贝以允许修改 
	auto kd_root = buildKDTree(mutable_points);

	glm::dvec3 best = points[0];
	double best_dist = std::numeric_limits<double>::max();
	nearestNeighborSearch(kd_root, target, best, best_dist);
	return best;
}
#endif

//拓扑关系构建
class PipeNetwork {
private:
	std::unordered_map<glm::dvec3*, std::vector<PipeSegment*>> adjacencyList; // 邻接表 
	std::vector<PipeSegment> _segments;
public:
	void buildTopology(const std::vector<PipeSegment>& segments) {
		_segments = segments;
		for (auto& seg : _segments) {
			adjacencyList[&seg.start].push_back(&seg);
			adjacencyList[&seg.end].push_back(&seg);
		}
	}
	//孤立管段检测 
	//统计节点连接度：
	std::vector<PipeSegment> findIsolatedSegments() {
		std::vector<PipeSegment> isolated;
		for (auto& node : adjacencyList) {
			if (node.second.empty())  continue; // 过滤虚拟交点 
			if (node.second.size() == 1) { // 仅一端连接 
				isolated.push_back(*(node.second.front()));
			}
		}
		return isolated;
	}
	//连通性修复策略 
	//策略1：延长孤立管段至最近邻管段（三维空间最近邻搜索）
	//策略2：插入虚拟连接点并生成新管段
	void repairIsolatedSegment(PipeSegment& seg, double searchRadius) {
		glm::dvec3 nearest = findNearestPoint(seg.end, searchRadius); // 可用KD或R树加速 
		seg.end = nearest; // 修改端点坐标 
	}
	glm::dvec3 findNearestPoint(const glm::dvec3& target, double searchRadius)
	{
		double min_sq_dist = searchRadius;
		glm::dvec3 nearest_point = {};
		for (auto& pt : _segments) {
			auto sq_dist = glm::distance(pt.start, target);
			if (sq_dist < min_sq_dist)
			{
				min_sq_dist = sq_dist;
				nearest_point = pt.start;
			}
		}
		return nearest_point;
	}
};

/*
graph LR
A[泄露点检测] --> B[500m缓冲区分析]
B --> C{受影响要素}
C --> D[关闭阀门]
D --> E[拓扑修复]
E --> F[生成应急方案]
*/



struct he_vert {
	glm::vec3 v = {};
	//float x = 0.0f, y = 0.0f, z = 0.0f;		// 顶点坐标
	int edge = -1;			// 指向以该顶点为起点的任意半边
};
struct he_edge {
	int vert = -1;			// 指向半边的终点顶点 
	int next = -1;			// 同一面内下一条半边
	int pair = -1;			// 对应的反向半边（孪生）, 孪生半边索引（-1表示无，即边界边twin
	int face = -1;			// 所属面
};
struct he_face {
	int edge = 0;			// 指向该面任意一条半边
	int count = 0;			// 边数
	int material = 0;		// 材质id
	// todo 法向等
};
struct he_mesh {
	std::vector<he_vert> _v;
	std::vector<he_edge> _e;
	std::vector<he_face> _f;
	std::vector<int> t_indices;
public:
	int add_vert(const glm::vec3& v);
	int add_vert(const glm::vec3* v, int n);
	// 添加面（顶点索引按逆时针顺序排列，返回面索引）
	int add_face(int* vertex_indices, int num_vertices);
	//遍历面的所有半边
	void traverse_face_edges(int face);
	//围绕顶点的半边遍历
	void traverse_vertex_edges(int v);
};

int he_mesh::add_vert(const glm::vec3& v)
{
	auto r = _v.size();
	he_vert hv = { v,-1 };
	_v.push_back(hv);
	return r;
}

int he_mesh::add_vert(const glm::vec3* vs, int n)
{
	if (vs && n > 0)
	{
		_v.reserve(_v.size() + n);
		auto r = _v.size();
		for (size_t i = 0; i < n; i++)
		{
			he_vert hv = { vs[i],-1 };
			_v.push_back(hv);
		}
		return r;
	}
	return 0;
}

// 添加面（顶点索引按逆时针顺序排列，返回面索引）
int he_mesh::add_face(int* vertex_indices, int num_vertices) {
	if (_v.empty() || !vertex_indices || num_vertices < 3) return -1;

	// 1. 创建新面（存入面数组）
	int face_idx = _f.size();
	auto face = &_f.emplace_back(he_face());
	face->edge = -1; // 初始化为-1，后续设置 

	// 2. 为面的每条边创建半边（临时存储半边索引）
	t_indices.resize(num_vertices);
	int* he_indices = t_indices.data();
	if (!he_indices) return -1;
	auto vertex_count = _v.size();
	auto half_edge_count = _e.size();
	_e.resize(half_edge_count + num_vertices * 1);
	for (int i = 0; i < num_vertices; i++) {
		int curr_v = vertex_indices[i];       // 当前顶点 
		int next_v = vertex_indices[(i + 1) % num_vertices]; // 下一个顶点（循环）

		// 检查顶点索引是否有效 
		if (curr_v < 0 || curr_v >= vertex_count) {
			return -1;
		}
		if (next_v < 0 || next_v >= vertex_count) {
			return -1;
		}

		// 创建新半边（存入半边数组） 
		he_edge* he = &_e[half_edge_count];
		he->vert = curr_v;       // 原点顶点是当前顶点 
		he->face = face_idx;       // 所属面是新面 
		he->next = -1;             // 下一个半边后续设置 
		he->pair = -1;             // 孪生半边后续查找 
		int he_idx = half_edge_count++;
		he_indices[i] = he_idx;

		// 设置顶点的离开半边（任意一个离开该顶点的半边均可）
		_v[curr_v].edge = he_idx;
	}

	// 3. 设置每个半边的next指针（形成面的边界循环）
	for (int i = 0; i < num_vertices; i++) {
		int curr_he = he_indices[i];
		int next_he = he_indices[(i + 1) % num_vertices];
		_e[curr_he].next = next_he;
	}
	// 4. 查找每个半边的孪生半边（方向相反的同一条边） 
	// 注：此处为简化，采用遍历方式查找，实际可优化为哈希表（如用(u, v)作为键） 
	for (int i = 0; i < num_vertices; i++) {
		int curr_he = he_indices[i];
		int curr_v = vertex_indices[i];
		int next_v = vertex_indices[(i + 1) % num_vertices];

		// 孪生半边的条件：origin是next_v，且其next的origin是curr_v（即方向为next_v→curr_v）
		for (int j = 0; j < half_edge_count; j++) {
			if (j == curr_he) continue; // 跳过自身 
			he_edge* twin_he = &_e[j];
			if (twin_he->vert != next_v) continue; // 原点必须是next_v 
			if (twin_he->next == -1) continue;       // 必须有下一个半边 
			he_edge* twin_next_he = &_e[twin_he->next];
			if (twin_next_he->vert != curr_v) continue; // 下一个半边的原点必须是curr_v 

			// 找到孪生半边，设置两者的twin指针 
			_e[curr_he].pair = j;
			_e[j].pair = curr_he;
			break; // 找到后退出循环 
		}
	}

	// 5. 设置面的起始半边（取第一个半边）
	face->edge = he_indices[0];
	return face_idx;
}
void he_mesh::traverse_face_edges(int f) {
	// 打印面的信息（遍历面的边界半边） 
	int face_idx = f;
	if (_f.empty() || face_idx < 0 || face_idx >= _f.size()) return;
	auto face = &_f[face_idx];
	if (face->edge == -1) {
		printf((char*)u8"Face %d: 无效（无起始半边）\n", face_idx);
		return;
	}
	printf((char*)u8"Face %d: 起始半边%d\n", face_idx, face->edge);
	int he_idx = face->edge;
	do {
		auto he = &_e[he_idx];
		auto v = &_v[he->vert].v;
		printf((char*)u8"  半边%d: 原点(%f, %f, %f)，所属面%d，下一个半边%d，孪生半边%d\n", he_idx, v->x, v->y, v->z, he->face, he->next, he->pair);
		he_idx = he->next;
	} while (he_idx != face->edge); // 循环直到回到起始半边  
}
void he_mesh::traverse_vertex_edges(int v) {
	he_vert* vert = &_v[v];
	int start = vert->edge;
	int current = vert->edge;
	do {
		auto he = &_e[current];
		auto& v = _v[he->vert].v;
		// 输出当前半边的终点坐标
		printf("Adjacent Vertex: (%f, %f, %f)\n", v.x, v.y, v.z);
		if (he->pair < 0)
		{
			break;
		}
		if (current < 0)
		{
			break;
		}
		current = _e[he->pair].next;   // 切换到相邻面的下一条半边
	} while (current != start);
}
int64_t testexp3d()
{
	int64_t ret = 0;
	do {
		std::string savedt;
		std::string savefn = "temp/rs.stl";
		{
			// 加载源stl
			mesh_triangle_cx* tc = new_mesh("E:\\tx\\0_straight_0_0.stl");
			if (!tc) { ret = -4; break; }
			std::vector<glm::vec3> poss = { { 40,20,0 },{40,-20,0} };
			if (poss.size() < 2)
			{
				ret = -5; break;
			}
			printf("bool_lines begin\n");
			std::vector<mesh_triangle_cx> mv, mv1, mvt;
			gp::line_style_t et = { 3,6,1,0,{1,1} };
			gp::mesh_mt ltf = {};			// 四边形网络
			gp::mesh_mt ltf0 = {};			// 四边形网络
			et.depth = 3;		// 深度
			et.count = 6;		// 分辨率
			et.thickness = 1;	// 厚度
			et.bottom_thickness = 0;//封底厚度
			et.type = { 0,1 };// hz::toVec2(n["btype"]);//样式 x.0=v，1=U，2=|_|，y=-1倒过来，
			glm::vec2 ss = { 6,5 };
			auto ctt = poss.size();
			auto tk = 1;
			for (int i = 0; i < 2; i++)
			{
				printf("bool_lines %d\n", i);
				auto et1 = et;
				et1.thickness = tk;
				auto v0 = poss[i];
				auto v1 = poss[i + 1];
				v0.z += 0.1;
				v1.z += 0.1;
				gp::build_line3d(v0, v1, ss, &et1, &ltf);
				et.thickness = 0;	// 厚度0为实心。
				gp::build_line3d(poss[i], poss[i + 1], ss, &et, &ltf0);
				i++;
			}

			std::vector<uint32_t> face_size;		// 面的边数3\4、多边形
			std::vector<uint32_t> face_indice;		// 索引
			std::vector<double> vertex_coord;		// 顶点坐标
			if (ltf.vertex_coord.empty() || ltf0.vertex_coord.empty())
			{
				ret = -6; break;
			}
			auto src_tc = mesh_split(tc);
			auto cut_ltf = mesh_split(&ltf);	// 切割块
			auto cut_ltf0 = mesh_split(&ltf0);	// 实心块

			mesh_save_stl(&cut_ltf[0], "temp/cltf.stl");
			mesh_save_stl(&cut_ltf0[0], "temp/cltf0.stl");
			std::string fns[] = {
				"a_not_b",		// 差集a-=b
				"b_not_a",		// 差集b-=a
				"union",		// 并集
				"intersection"	// 交集
			};
			{
				print_time aadk("bool mesh A_NOT_B");
				make_boolean(tc, &ltf0, mv, flags_b::A_NOT_B);
				make_boolean(&ltf, tc, mv1, flags_b::A_NOT_B);
				std::vector<mesh_triangle_cx> vs;
				printf("bool_lines 5181\n");
				if (mv1.size())
				{
					for (auto& mt : mv1) {
						mesh_split(&mt, &vs);
					}
					if (vs.size())
					{
						mv1.clear();
						for (auto& mt : vs)
						{
							make_boolean(&mt, tc, mv1, flags_b::A_NOT_B);
						}
						vs.clear();
						for (auto& mt : mv1) {
							mesh_split(&mt, &vs);
						}
						vs.swap(mv1);
					}
					vs.clear();
					glm::vec2 p2[2] = { poss[0], poss[1] };
					struct vs2 { float d; size_t i; };
					std::vector<vs2> ks;
					auto pd = glm::distance(p2[0], p2[1]);
					for (size_t i = 0; i < mv1.size(); i++)
					{
						auto& it = mv1[i];
						auto& dv = it.vertices;
						int icc = 0;
						float d = pd;
						for (auto& vt : dv)
						{
							glm::vec2 vt0 = vt;
							d = std::min(d, std::min(glm::distance(p2[0], vt0), glm::distance(p2[1], vt0)));//判断是否是线头尾
						}
						ks.push_back({ d,i });
					}
					std::sort(ks.begin(), ks.end(), [](const vs2& v1, const vs2& v2) {return v1.d > v2.d; });

					for (auto& it : ks) {
						if (it.d > 0)
						{
							vs.push_back(std::move(mv1[it.i]));
						}
					}
					if (vs.size())
					{
						mv1.swap(vs);
					}
					for (auto& kt : mv1) {
						for (auto& xt : kt.vertices) {
							xt.z -= 0.1;
						}
					}
				}
			}
			if (mv.size() && mv1.size())
			{
				print_time aadk("bool mesh UNION");
				printf("bool_lines 5213\n");
				for (size_t i = 1; i < mv.size(); i++)
				{
					its_merge(mv[0], mv[i]);
				}
				for (size_t i = 1; i < mv1.size(); i++)
				{
					its_merge(mv1[0], mv1[i]);
				}

				make_boolean(&mv[0], &mv1[0], mvt, flags_b::UNION);
				printf("bool_lines 5219\n");
			}
			else if (mv.size()) {

				print_time aadk("bool mesh UNION");
				make_boolean(&mv[0], &ltf, mvt, flags_b::UNION);
				printf("bool_lines 5225\n");
			}
			if (mvt.size()) {
				// 保存结果
				auto& it = mvt[0];
				auto fn = savefn;
				printf("bool_lines save\n");
				mesh_save_stl(&it, fn.c_str());
				ret = mvt.size();
				if (savedt.size())
				{
					std::vector<char> btd;
					auto p = &mvt[0];
					stl3d_cx sc;
					auto length = p->indices.size();
					auto d = p->vertices.data();
					auto t = p->indices.data();
					sc.add(0, 0, length * 3);
					printf("bool_lines save 0\n");
					for (size_t i = 0; i < length; i++)
					{
						auto it = t[i];
						glm::vec3 v[3] = { d[it.x],d[it.y],d[it.z] };
						sc.add(v, 3, 0);
					}
					printf("bool_lines save 1\n");
					sc.save_binary(btd);
					if (btd.size())
					{
						//set_data(savedt.c_str(), btd.data(), btd.size());

					}
					printf("bool_lines save 2\n");
				}
			}
			free_mesh(tc);
			printf("bool_lines end\n");
		}

	} while (0);
	return ret;
}


void he_test() {
	he_mesh m = {};
	int v0 = m.add_vert(glm::vec3(0.0f, 0.0f, 0.0f)); // (0,0,0) 
	int v1 = m.add_vert(glm::vec3(1.0f, 0.0f, 0.0f)); // (1,0,0) 
	int v2 = m.add_vert(glm::vec3(1.0f, 1.0f, 0.0f)); // (1,1,0) 
	int v3 = m.add_vert(glm::vec3(0.0f, 1.0f, 0.0f)); // (0,1,0) 
	if (v0 == -1 || v1 == -1 || v2 == -1 || v3 == -1) {
		printf((char*)u8"添加顶点失败！\n");
		return;
	}
	// 3. 添加面（底面，顶点按逆时针顺序：v0→v1→v2→v3）
	int face_vertices[] = { v0, v1, v2, v3 };
	int face_idx = m.add_face(face_vertices, 4);
	m.traverse_face_edges(0);
	m.traverse_vertex_edges(1);
	testexp3d();
	testexp3d();
	printf("");
}

int main(int argc, char* argv[])
{

	{
		auto rd = hz::read_json(R"(E:\3DG\TF\RQGXPT\merge\1\1.json)");
		std::vector<glm::dvec3> pts;
		auto& ID = rd["ID"];
		auto& X = rd["X"];
		auto& Y = rd["Y"];
		auto& Z = rd["H"];
		auto cc = X.size();
		pts.reserve(cc);
		std::set<std::string> idn;
		for (size_t i = 0; i < cc; i++)
		{
			auto id = ID[i].get<std::string>();
			if (idn.insert(id).second)
			{
				auto x = X[i].get<std::string>();
				auto y = Y[i].get<std::string>();
				auto z = Z[i].get<std::string>();
				pts.push_back({ std::atof(x.c_str()),std::atof(y.c_str()),std::atof(z.c_str()), });
			}
		}
		double x[] = { 630261.334, 630269.64, 630270.465, 630261.334, };
		double y[] = { 3352941.215, 3353010.228, 3353056.656, 3352941.215 };
		PipeNetwork network;
		network.buildTopology({
			{ {629998.349, 3353938.047, 0}, {629989.256, 3353955.858, 0} },
			{ {629989.256, 3353955.858, 0}, {629984.836,3353962.421,0} },
			});
		auto lss = network.findIsolatedSegments();
		auto a = area_ring(x, y, 4);
		glm::dvec3 target = { 5,3,2.2 };
		glm::dvec3 pt = { 1,2,2 };
		glm::dvec3 pt1 = { 10,21,3 };
		auto d0 = glm::distance2(pt, target);
		auto d1 = glm::distance2(pt1, target);
		auto d0x = glm::distance(pt, target);
		auto d1x = glm::distance(pt1, target);
		printf("area:%f\n", a);
	}
	clearpdb();
	//for (;;)
	//makemjson("temp/mariodata.json");
#if 0
	{
		hz::mfile_t vco;
		//auto vcod = vco.open_d(R"(E:\code\cpp\vk\b\blender\release\datafiles\icons\ops.transform.translate.dat)", true);
		auto vcod = vco.open_d(R"(E:\code\cpp\vk\b\blender\release\datafiles\icons\ops.transform.rotate.dat)", true);
		struct vco_dt
		{
			char t[3];
			char v;
			glm::u8vec2 size;
			glm::u8vec2 pos;
		};
		auto pc = (vco_dt*)vcod;
		auto n = vco.size() - 8;
		// 6字节三角形坐标，12字节颜色
		n /= 6 + 12;
		auto t = vcod + 8;
		auto tc = (uint32_t*)(t + n * 6);
		std::vector<glm::vec2> v3;
		std::vector<uint32_t> cv;
		for (size_t i = 0; i < n; i++)
		{
			auto t3 = (glm::u8vec2*)t;
			v3.push_back(t3[0]);
			v3.push_back(t3[1]);
			v3.push_back(t3[2]);
			t += 6;
			cv.push_back(tc[0]);
			cv.push_back(tc[1]);
			cv.push_back(tc[2]);
			tc += 3;
		}
		printf("%d\t%d\n", (int)pc->size.x, (int)pc->size.y);
	}
	if (!hz::check_useradmin())
	{
		hz::shell_exe(argv[0], true);
		return 0;
	}
#endif
	//dmain();
	//new_gpu(argc, argv);

	const char* wtitle = (char*)u8"多功能管理工具";
	auto tstr = hz::u8_to_gbk(wtitle);
	auto app = new_app();


	he_test();

	cpuinfo_t cpuinfo = get_cpuinfo();
	glm::ivec2 ws = { 1280,860 };
	// ef_vulkan ef_gpu|ef_resizable
	form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, (ef_gpu | ef_resizable));
	auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
	auto kd = sdldev.vkdev;
	sdldev.vkdev = 0;					// 清空使用独立创建逻辑设备
	std::vector<device_info_t> devs = get_devices(sdldev.inst); // 获取设备名称列表

	//vkdg_cx* vkd = new_vkdg(sdldev.inst, sdldev.phy);	// 创建vk渲染器 
	bs::test_timeline();
	auto gm = menu_m(form0);
	printf("%p\n", form0);
	auto pl = show_ui(form0, gm);
	show_ui2(form0, gm);
	//show_cpuinfo(form0);
	glm::mat3 m3x = glm::shearX(glm::mat3(1), 0.5f);
	glm::mat3 m3y = glm::shearY(glm::mat3(1), 0.5f);
	glm::vec3 v3 = { 1,2,3 };
	auto v3x = m3x * v3;
	auto v3x0 = v3 * m3x;
	float x = 0.2;
	auto v3y = m3y * x;
	auto v3y0 = x * m3y;
	auto d2 = new sp_drawable();
	d2->set_renderer(form0->renderer);
	/*d2->add(R"(E:\vsz\g3d\s2d\spine-runtimes\spine-sdl\data\spineboy-pma.atlas)"
		, R"(E:\vsz\g3d\s2d\spine-runtimes\spine-sdl\data\spineboy-pro.json)", 0.25, 0.2, "temp/spineboy-j.spt");
	d2->add(R"(E:\vsz\g3d\s2d\spine-runtimes\spine-glfw\data\spineboy-pma.atlas)"
		, R"(E:\vsz\g3d\s2d\spine-runtimes\spine-glfw\data\spineboy-pro.skel)", 0.25, 0.2, "temp/spineboy-skel.spt");
	*/
	d2->add_pkg("temp/spineboy-j.spt", 0.25, 0.2);
	d2->add_pkg("temp/spineboy-skel.spt", 0.25, 0.2);
	d2->set_pos(0, 600, 650);
	d2->set_pos(1, 300, 650);

	{
		njson j = hz::read_json(R"(E:\vsz\g3d\s2d\spine-runtimes\spine-sdl\data\spineboy-pro.json)");
		const char* kname[] = { "skeleton","bones","slots","ik","skins","transform","animations","events" };
		auto& skeleton = j["skeleton"];
		auto& bones = j["bones"];
		auto& slots = j["slots"];
		auto& ik = j["ik"];
		auto& skins = j["skins"];
		auto& transform = j["transform"];
		auto& animations = j["animations"];
		auto& events = j["events"];
		glm::vec4 size = { hz::toDouble(skeleton["x"]),hz::toDouble(skeleton["y"]),hz::toDouble(skeleton["width"]),hz::toDouble(skeleton["height"]), };
		std::string hash = hz::toStr(skeleton["hash"]);
		std::set<std::string> names;
		std::map<std::string, std::set<std::string>> mm;
		for (size_t i = 0; i < sizeof(kname) / sizeof(char*); i++)
		{
			auto& kk = j[kname[i]];
			if (kk.is_object())
			{
				for (auto& [k, v] : kk.items())
				{
					names.insert(k);
				}
			}
			else if (kk.is_array())
			{
				for (auto& it : kk)
				{
					for (auto& [k, v] : it.items())
					{
						names.insert(k);
					}
				}
			}
			mm[kname[i]] = (std::move(names));
			names.clear();
		}
		if (mm.size())
		{
			printf("\n");
		}
	}
	atlas_strinfo ass = get_atlas_strinfo();
	static std::vector<char*> nv;
	d2->get_anim_name(0, &nv);
	d2->animationstate_set_animationbyname(0, 0, "portal", 0);
	d2->animationstate_add_animationbyname(0, 0, "run", -1, 0);
	d2->animationstate_set_animationbyname(1, 0, "portal", 0);
	d2->animationstate_add_animationbyname(1, 0, "shoot", -1, 0);
	auto ft = app->font_ctx->get_font("Corbel", 0);
	{
		std::string k8 = (char*)u8"👩‍👩‍👧‍👧👩";
		uint32_t kw = md::get_u8_idx(k8.c_str(), 0);
		{
			/*
				c:\windows\fonts\seguiemj.ttf
				c:\windows\fonts\seguiemj.ttf
			*/
			cairo_surface_t* sur = new_image_cr({ 1024,1024 }, 0);
			auto sue = app->font_ctx->get_font("Segoe UI Emoji", 0);
			do_text(k8.c_str(), 0, k8.size());
			font_t::GlyphPositions gp = {};
			auto nn0 = sue->CollectGlyphsFromFont(k8.data(), k8.size(), 8, 0, 0, &gp);
			double scale_h = sue->get_scale(100);
			uint32_t color = -1;
			auto cr = cairo_create(sur);
			set_color(cr, 0xff000000);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_paint(cr);
			int xx = 0;
			int yy = sue->get_line_height(100);
			std::vector<font_item_t> tm;
			for (size_t i = 0; i < gp.len; i++)
			{
				auto pos = &gp.pos[i];
				auto git = sue->get_glyph_item(pos->index, 0, 100);// 光栅化glyph index并缓存
				glm::vec2 offset = { ceil(pos->x_offset * scale_h), -ceil(pos->y_offset * scale_h) };
				git._apos = offset;
				tm.push_back(git);
			}
			for (size_t i = 0; i < gp.len; i++)
			{
				auto pos = &gp.pos[i];
				glm::vec2 adv = { ceil(pos->x_advance * scale_h), ceil(pos->y_advance * scale_h) };
				glm::vec2 offset = { ceil(pos->x_offset * scale_h), -ceil(pos->y_offset * scale_h) };
				auto git = tm[i];
				if (git._image) {
					auto ft = (cairo_surface_t*)git._image->ptr;
					if (!ft) {
						ft = new_image_cr(git._image);
						git._image->ptr = ft;
					}
					if (ft)
					{
						auto ps = git._dwpos;// +git._apos;
						ps.x += xx;
						ps += offset;
						ps.y += yy;
						draw_image(cr, ft, ps, git._rect, git.color ? git.color : color);
					}
				}
				xx += adv.x;
			}
			image_save_png(sur, "temp/v1.png");
			auto sue1 = sue;
			auto gis = sue->get_glyph_index(kw, 0, 0);
			std::vector<vertex_f> vdp;
			auto vnn = sue->GetGlyphShapeTT(gis, &vdp);
			auto baseline = sue->ascender;
			int blur = 2;
			auto src = new path_v();
			src->set_data((path_v::vertex_t*)vdp.data(), vdp.size());
			auto k = src->get_size();
			//src->incpos({ 0,-src->_box.y * 2 });
			image_gray bmp = {};
			bmp.width = k.x * 1.5;
			bmp.height = k.y * 1.5;
			glm::vec2 ps = { 0,-200 };
			ps.x -= blur * 2;
			ps.y -= blur * 2;
			ps.y -= baseline;
			auto bmp1 = bmp;
			get_path_bitmap((vertex_32f*)src->data(), src->size(), &bmp, { 1,1 }, ps, 1);
			save_img_png(&bmp, "temp/v4.png");
		}

		font_t::GlyphPositions gp = {};
		font_t::GlyphPositions gp1 = {};
		auto nn0 = ft->CollectGlyphsFromFont("fi", -1, 8, 0, 0, &gp);
		auto nn1 = ft->CollectGlyphsFromFont("fttt", -1, 8, 0, 0, &gp1);
		auto nn2 = ft->CollectGlyphsFromFont("ft", -1, 8, 0, 0, &gp1);
		auto nn3 = ft->CollectGlyphsFromFont("tt", -1, 8, 0, 0, &gp1);
		texture_cb tex_cb = get_texture_cb();
		auto mari_tex = (SDL_Texture*)tex_cb.new_texture_file(form0->renderer, "data/mari.png");
		page_obj_t ro = {};
		ro.renderer = form0->renderer;
		auto ptex = new texture_cb();
		*ptex = tex_cb;
		ro.cb = ptex;
#if 0
		auto xha = new_atlas("data/xh1.atlas", &ro);
		logic_cx* logic = new_logic(xha);
		logic->_pos = { 20,90 };
		logic->add_gate(dType::AND_GATE, "and", { 60,60 }, 0);
		logic->add_gate(dType::AND_GATE, "and", { 60 ,150 }, 0);
		logic->add_gate(dType::AND_GATE, "and", { 150,60 }, 0);
		logic->add_gate(dType::AND_GATE, "and", { 150,150 }, 0);
		logic->add_gate(dType::AND_GATE, "and", { 230,60 }, 0);
		logic->gates[0].input = 0x00;
		logic->gates[1].input = 0x03;
		logic->gates[2].input = 0x01;
		logic->gates[3].input = 0x02;
		logic->gates[4].build = 1;
		glm::vec2 nnpos = { 280,0 };
		logic->add_gate(dType::XOR_GATE, "xor0", { nnpos.x + 60,nnpos.y + 60 }, 0);
		logic->add_gate(dType::XOR_GATE, "xor3", { nnpos.x + 60 ,nnpos.y + 150 }, 0);
		logic->add_gate(dType::XOR_GATE, "xor1", { nnpos.x + 150,nnpos.y + 60 }, 0);
		logic->add_gate(dType::XOR_GATE, "xor2", { nnpos.x + 150,nnpos.y + 150 }, 0);
		logic->add_gate(dType::XOR_GATE, "xor", { nnpos.x + 230,nnpos.y + 60 }, 0);
		int xx = 5;
		logic->gates[xx + 0].input = 0x00;
		logic->gates[xx + 1].input = 0x03;
		logic->gates[xx + 2].input = 0x01;
		logic->gates[xx + 3].input = 0x02;
		logic->gates[xx + 4].build = 1;
		nnpos += glm::vec2({ 0,180 });
		logic->add_gate(dType::OR_GATE, "or0", { nnpos.x + 60,nnpos.y + 60 }, 0);
		logic->add_gate(dType::OR_GATE, "or3", { nnpos.x + 60 ,nnpos.y + 150 }, 0);
		logic->add_gate(dType::OR_GATE, "or1", { nnpos.x + 150,nnpos.y + 60 }, 0);
		logic->add_gate(dType::OR_GATE, "or2", { nnpos.x + 150,nnpos.y + 150 }, 0);
		logic->add_gate(dType::OR_GATE, "or", { nnpos.x + 230,nnpos.y + 60 }, 0);
		xx += 5;
		logic->gates[xx + 0].input = 0x00;
		logic->gates[xx + 1].input = 0x03;
		logic->gates[xx + 2].input = 0x01;
		logic->gates[xx + 3].input = 0x02;
		logic->gates[xx + 4].build = 1;
		nnpos += glm::vec2({ -280,0 });
		logic->add_gate(dType::NOT_GATE, "not1", { nnpos.x + 150,nnpos.y + 60 }, 0);
		logic->add_gate(dType::NOT_GATE, "not2", { nnpos.x + 150,nnpos.y + 150 }, 0);
		logic->add_gate(dType::NOT_GATE, "not", { nnpos.x + 230,nnpos.y + 60 }, 0);
		xx += 5;
		logic->gates[xx + 0].input = 0x00;
		logic->gates[xx + 1].input = 0x01;
		logic->gates[xx + 2].build = 1;
		logic->_scale = 1.0;
#endif
		form0->add_event(0, [=](uint32_t type, et_un_t* e, void* ud)
			{
				auto btn = e->v.b;
				static int idx = -1;
				if (type != (uint32_t)devent_type_e::mouse_button_e || btn->down)return;
				idx++;
				if (idx < nv.size())
				{
					d2->animationstate_add_animationbyname(1, 0, nv[idx], -1, 0);
				}

			});

		form0->render_cb = [=](SDL_Renderer* renderer, double delta)
			{
				static double deltas = 0;
				deltas += delta;
				d2->update_draw(delta);
				//logic->draw_update(delta);
				//if (mari_tex)
				//{
				//	texture_dt nadt = {};
				//	glm::ivec2 ps = { };
				//	glm::ivec2 npos = { 106,306 };
				//	nadt.src_rect = { 0,0,16,8 };
				//	nadt.dst_rect = { npos.x + ps.x * gh,yps + npos.y + ps.y * gh,nadt.src_rect.z, nadt.src_rect.w };
				//	tex_cb.render_texture(renderer, mari_tex, &nadt, 1);
				//}
			};
	}


	// 运行消息循环
	run_app(app, 0);
	delete d2;
	free_app(app);
#ifdef _WIN32
	ExitProcess(0);
#endif
	return 0;
}

void test_m(layout_text_x* ltx)
{
	auto lt = ltx;
	text_path_t opt = {};
	text_image_t opti = {};
	auto tp = lt->get_shape(0, 50, u8"富强", &opt);
	auto tpi = lt->get_glyph_item(0, 50, u8"富强", &opti);
	text_image_pt* tip = text_blur(&opt, 4, 2, 0xff111111, 0xff0000ff);
	uint32_t color = -1;
	auto image = opti.tv[0]._image;
	for (auto& it : opti.tv) {
		//ft, it._dwpos + it._apos, it._rect, it.color ? it.color : color

	}
	save_img_png(image, "temp/font_fw.png");
	// 保存到png\jpg
	textimage_file(tip, "temp/text_inflate.png");
	free_textimage(tip);

	maze_cx maze;
	astar_search as;
	int ww = 100;
	maze.set_seed(15687951215562);
	maze.init(ww, ww);
	as.init(ww, ww, (uint8_t*)maze.data(), false);
	// 大于0则是不通
	as.set_wallheight(0);
	glm::ivec2 pStart = maze.start, pEnd = maze.dest;//随机开始、结束坐标
	bool hok = as.FindPath(&pStart, &pEnd, 0);
	std::vector<uint32_t> mg;
	mg.resize(ww * ww, 0);
	auto nma = maze.data();
	for (size_t i = 0; i < mg.size(); i++)
	{
		mg[i] = nma[i] == 0 ? 0 : 0xff000000;
	}
	stbi_write_png("temp/maze.png", ww, ww, 4, mg.data(), 0);
	for (auto& it : as.path)
	{
		mg[it.x + it.y * ww] = 0xff0020ff;
	}
	mg[pStart.x + pStart.y * ww] = 0xff00ff00;
	mg[pEnd.x + pEnd.y * ww] = 0xffff0000;
	stbi_write_png("temp/maze2.png", ww, ww, 4, mg.data(), 0);
}
void tiled_data(const char* data)
{
	// Bits on the far end of the 32-bit global tile ID are used for tile flags
	const uint32_t FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
	const uint32_t FLIPPED_VERTICALLY_FLAG = 0x40000000;
	const uint32_t FLIPPED_DIAGONALLY_FLAG = 0x20000000;

	uint32_t tile_index = 0;

	// Here you should check that the data has the right size
	// (map_width * map_height * 4)

	//for (int y = 0; y < map_height; ++y) {
	//	for (int x = 0; x < map_width; ++x) {
	//		uint32_t global_tile_id = data[tile_index] |
	//			data[tile_index + 1] << 8 |
	//			data[tile_index + 2] << 16 |
	//			data[tile_index + 3] << 24;
	//		tile_index += 4;

	//		// Read out the flags
	//		bool flipped_horizontally = (global_tile_id & FLIPPED_HORIZONTALLY_FLAG);
	//		bool flipped_vertically = (global_tile_id & FLIPPED_VERTICALLY_FLAG);
	//		bool flipped_diagonally = (global_tile_id & FLIPPED_DIAGONALLY_FLAG);

	//		// Clear the flags
	//		global_tile_id &= ~(FLIPPED_HORIZONTALLY_FLAG |
	//			FLIPPED_VERTICALLY_FLAG |
	//			FLIPPED_DIAGONALLY_FLAG);

	//		// Resolve the tile
	//		for (int i = tileset_count - 1; i >= 0; --i) {
	//			Tileset* tileset = tilesets[i];

	//			if (tileset->first_gid() <= global_tile_id) {
	//				tiles[y][x] = tileset->tileAt(global_tile_id - tileset->first_gid());
	//				break;
	//			}
	//		}
	//	}
	//}
}
