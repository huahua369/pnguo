
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
#include <cairo/cairo.h>
#include "mshell.h"
#ifdef GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#endif 
#include <pnguo/editor_2d.h>
#include <spine/spine-sdl3/spinesdl3.h>
#include <stb_image_write.h>

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
	auto n = cpuid_get_total_cpus() + 2;
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
int get_cpu_temp() {
	auto n = cpuid_get_total_cpus();
	cpu_raw_data_t raw = {};
	uint32_t msr_temp = 0x1A2; // Intel Thermal Status Register 

	if (cpuid_get_raw_data(&raw) < 0) {
		fprintf(stderr, "CPUID初始化失败\n");
		return 1;
	}
	uint32_t r[32] = {};
	cpu_exec_cpuid(msr_temp, r);
	msr_driver_t2 md = {};
	strcpy(md.driver_path, R"(E:\code\cs\openhardwaremonitor\Hardware\WinRing0x64.sys)");
	load_driver(&md);

	HMODULE m_hOpenLibSys;
	DWORD eax, edx;
	DWORD TjMax;
	DWORD IAcore;
	DWORD PKGsts;
	int Cputemp = 0;

	m_hOpenLibSys = NULL;
	if (InitOpenLibSys(&m_hOpenLibSys) != TRUE)
	{
		std::cout << "DLL Load Error!\n";
		return FALSE;
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
		break;
		Sleep(500);
	}
	return 0;
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
int main(int argc, char* argv[])
{
	std::stack<int> st;
	clearpdb();
	dmain();
	new_gpu(argc, argv);

	const char* wtitle = (char*)u8"多功能管理工具";
	auto tstr = hz::u8_to_gbk(wtitle);
	auto app = new_app();
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

	texture_cb tex_cb = get_texture_cb();
	{
		auto xh_tex = (SDL_Texture*)tex_cb.new_texture_file(form0->renderer, "data/xh1.png");
		auto mari_tex = (SDL_Texture*)tex_cb.new_texture_file(form0->renderer, "data/mari.png");
		page_obj_t ro = {};
		ro.renderer = form0->renderer;
		auto ptex = new texture_cb();
		*ptex = tex_cb;
		ro.cb = ptex;
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

		// 随机数种子
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dis(0, 5);
		std::vector<int> lines;
		int lw = 26;
		int gh = 30;
		int yps = 0;
		lines.resize(26 * 16);
		for (size_t i = 0; i < lines.size(); i++)
		{
			int tt = dis(gen);
			lines[i] = tt;
		}
		std::vector<SDL_FPoint>* opt2 = new std::vector<SDL_FPoint>();
		genGrid(15, 70 * 15, opt2);
		form0->render_cb = [=](SDL_Renderer* renderer, double delta)
			{
				static double deltas = 0;
				deltas += delta;
				if (xh_tex) {
					glm::ivec4 notm = get_lgates_rc(2);
					glm::ivec4 andm = get_lgates_rc(3);
					glm::ivec4 orm = get_lgates_rc(4);
					glm::ivec4 xorm = get_lgates_rc(5);
					glm::ivec4 wline[6] = {};
					for (size_t i = 0; i < 6; i++)
					{
						wline[i] = get_lgates_rc(6 + i);
					}

					drawGrid(renderer, { 20,90 }, 15, 70 * 15, opt2);

					texture_dt adt = {   };
					adt.src_rect = { 0,0,512,512 };
					adt.dst_rect = { 106,106,adt.src_rect.z,adt.src_rect.w };
					//tex_cb.render_texture(renderer, xh_tex, &adt, 1); 



					auto tl = lines.data();
					glm::ivec2 npos = { 106,306 };
					texture_dt nadt = {};
					for (size_t i = 0; i < lines.size(); i++)
					{
						glm::ivec2 ps = { i % lw,i / lw };
						auto x = *tl;
						nadt.src_rect = wline[x];
						nadt.dst_rect = { npos.x + ps.x * gh,yps + npos.y + ps.y * gh,nadt.src_rect.z, nadt.src_rect.w };
						//tex_cb.render_texture(renderer, xh_tex, &nadt, 1);
						tl++;
					}
				}
				d2->update_draw(delta);
				logic->draw_update(delta);
				if (mari_tex)
				{
					texture_dt nadt = {};
					glm::ivec2 ps = { };
					glm::ivec2 npos = { 106,306 };
					nadt.src_rect = { 0,0,16,8 };
					nadt.dst_rect = { npos.x + ps.x * gh,yps + npos.y + ps.y * gh,nadt.src_rect.z, nadt.src_rect.w };
					tex_cb.render_texture(renderer, mari_tex, &nadt, 1);
				}
			};
	}


	// 运行消息循环
	run_app(app, 0);
	delete d2;
	free_app(app);
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