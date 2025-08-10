
#include <pch1.h>

#include <print_time.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>  
#include <SDL3/SDL_system.h>   
#include <vulkan/vulkan.h>
#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#include <imm.h>
#pragma comment(lib,"Imm32.lib")
#pragma comment(lib, "dwmapi")
#include <commctrl.h>
#include <ole2.h>
#include <win_core.h>
#endif  
#include <pnguo.h>
#include <tinysdl3.h>
#include <event.h>

#ifdef max
#undef max
#undef min
#endif // max 

std::string get_clipboard()
{
	std::string ret = {};
	if (SDL_HasClipboardText())
	{
		auto p = SDL_GetClipboardText();
		if (p)
		{
			ret = p;
			SDL_free((void*)p);
		}
	}
	return ret;
}

void set_clipboard(const char* str)
{
	if (str)
		SDL_SetClipboardText(str);
}

void set_col_u8()
{
#ifdef _WIN32
	//AllocConsole();
	//FILE* tempFile = nullptr;
	//freopen_s(&tempFile, "conin$", "r+t", stdin); //reopen the stdin, we can use std::cout.
	//freopen_s(&tempFile, "conout$", "w+t", stdout);
	system("color 00");
	system("CHCP 65001");
#endif

}
namespace pce {

	void set_property(SDL_Window* w, const char* str, void* p);
	void* get_property(SDL_Window* w, const char* str);


	void set_property(SDL_Window* w, const char* str, void* p)
	{
		SDL_SetPointerProperty(SDL_GetWindowProperties(w), str, p);
	}
	void* get_property(SDL_Window* w, const char* str)
	{
		return SDL_GetPointerProperty(SDL_GetWindowProperties(w), str, 0);
	}

	void* get_windowptr(SDL_Window* w)
	{
		//static SDL_SysWMinfo systemInfo;
		//SDL_VERSION(&systemInfo.version);
		//SDL_GetWindowWMInfo(_ptr, &systemInfo);
#ifdef _WIN32
		return get_property(w, SDL_PROP_WINDOW_WIN32_HWND_POINTER);// "SDL.window.win32.hwnd");
		//return systemInfo.info.win.window;
#elif __ANDROID__
	//return systemInfo.info.android.window;
		return get_property(w, "SDL.window.android.window", );
#endif
		//return &systemInfo.info;
		return 0;
	}

#ifdef _WIN32
	bool windowColorKey(SDL_Window* window, COLORREF colorKey) {
		HWND hWnd = (HWND)get_windowptr(window);

		// Change window type to layered
		SetWindowLongW(hWnd, GWL_EXSTYLE, GetWindowLongW(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);

		// Set transparency color
		return SetLayeredWindowAttributes(hWnd, colorKey, 0, LWA_COLORKEY);
	}
	HRESULT DisableNCRendering(HWND hWnd)
	{
		HRESULT hr = S_OK;

		DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;

		// Disable non-client area rendering on the window.
		hr = ::DwmSetWindowAttribute(hWnd,
			DWMWA_NCRENDERING_POLICY,
			&ncrp,
			sizeof(ncrp));

		if (SUCCEEDED(hr))
		{
			// ...
		}
		return hr;
	}
	HRESULT EnableBlurBehindWindowMY(HWND window, bool enable = true, HRGN region = 0, bool transitionOnMaximized = false, const glm::ivec2& size = { -1,-1 })
	{
		BOOL pfe = 0;
		auto hr = DwmIsCompositionEnabled(&pfe);
		//if (!pfe)
		{
			DwmEnableComposition(enable ? DWM_EC_ENABLECOMPOSITION : DWM_EC_DISABLECOMPOSITION);
		}
		//DWM_BLURBEHIND bb = { 0 };
		//HRGN hRgn = CreateRectRgn(0, 0, -1, -1); //应用毛玻璃的矩形范围，
		////参数(0,0,-1,-1)可以让整个窗口客户区变成透明的，而鼠标是可以捕获到透明的区域
		//bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
		//bb.hRgnBlur = hRgn;
		//bb.fEnable = TRUE;
		//hr = DwmEnableBlurBehindWindow(window, &bb);
		//if (hRgn)
		//	DeleteObject(hRgn);
		//return hr;
		BOOL isNCRenderingEnabled{ FALSE };
		hr = ::DwmGetWindowAttribute(window,
			DWMWA_NCRENDERING_ENABLED,
			&isNCRenderingEnabled,
			sizeof(isNCRenderingEnabled));
		RECT extendedFrameBounds{ 0,0,0,0 };
		hr = ::DwmGetWindowAttribute(window,
			DWMWA_EXTENDED_FRAME_BOUNDS,
			&extendedFrameBounds,
			sizeof(extendedFrameBounds));
		//hr = DisableNCRendering(window);
		BOOL isNCRenderingEnabled1{ FALSE };
		hr = ::DwmGetWindowAttribute(window,
			DWMWA_NCRENDERING_ENABLED,
			&isNCRenderingEnabled1,
			sizeof(isNCRenderingEnabled1));

		DWM_BLURBEHIND blurBehind = { 0 };
		blurBehind.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION | DWM_BB_TRANSITIONONMAXIMIZED;
		blurBehind.fEnable = enable;
		blurBehind.fTransitionOnMaximized = transitionOnMaximized;
		bool isnew = false;
		if (!region)
		{
			region = CreateRectRgn(0, 0, size.x, size.y);
			isnew = true;
		}
		if (enable && 0 != region)
		{
			blurBehind.dwFlags |= DWM_BB_BLURREGION;
			blurBehind.hRgnBlur = region;
		}
		MARGINS margins = { -1 };
		//DwmExtendFrameIntoClientArea(window, &margins);
		hr = ::DwmEnableBlurBehindWindow(window, &blurBehind);
		if (isnew && region)
			DeleteObject(region);
		//PostMessageW(window, WM_NCPAINT, 0, 0);
		return hr;
	}
#endif

}//!pce

SDL_HitTestResult HitTestCallback2(SDL_Window* win, const SDL_Point* area, void* data);



class render_2d
{
public:
	std::vector<SDL_Vertex> vertexs;
	std::vector<int> indexs;
public:
	render_2d();
	~render_2d();
	// 渲染一个对象
	void draw_data(SDL_Renderer* renderer, skeleton_t* skeleton);
private:

};

class Timer
{
public:
	float target_fps = 0.0;
	float screen_ticks_per_frame = 0.0f;
	Uint64 started_ticks = 0;
	float extra_time = 0.0;
	Timer()
		:started_ticks{ SDL_GetPerformanceCounter() }
	{
	}

	void restart() {
		started_ticks = SDL_GetPerformanceCounter();
	}

	float get_time() {
		return (static_cast<float>(SDL_GetPerformanceCounter() - started_ticks) / static_cast<float>(SDL_GetPerformanceFrequency()) * 1000.0f);
	}
	void set_fps(float f) {

		target_fps = f;
		screen_ticks_per_frame = 1000.0f / static_cast<float>(target_fps);
	}
	void fps_sleep()
	{
	}
};



sem_s::sem_s()
{
	sem = SDL_CreateSemaphore(0);
}

sem_s::~sem_s()
{
	if (sem)
		SDL_DestroySemaphore(sem);
	sem = 0;
}

void sem_s::post()
{
	SDL_SignalSemaphore(sem);
}

int sem_s::wait(int ms)
{
	if ((ms > 0))
		return SDL_WaitSemaphoreTimeout(sem, ms) ? 0 : -1;
	SDL_WaitSemaphore(sem);
	return 0;
}
int sem_s::wait_try()
{
	return SDL_TryWaitSemaphore(sem);
}
// 窗口应用管理
#if 1

#ifdef _WIN32

bool wMessageHook(void* userdata, MSG* msg) {
	auto app = (app_cx*)userdata;
	if (app && msg)
	{
		switch (msg->message)
		{
		case WM_NCLBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		case WM_NCMBUTTONDOWN:
			app->nc_down = true;
			app->kncdown();
			break;
			//case WM_NCLBUTTONUP:
			//case WM_NCRBUTTONUP:
			//case WM_NCMBUTTONUP:
			//	app->nc_down = false;
			//	break;
		default:
			break;
		}
	}
	return 1;
}
#endif


app_cx::app_cx()
{
	set_col_u8();
	font_ctx = new_fonts_ctx();

	fct = new Timer();
	r2d = new render_2d();
	uint32_t f = -1;
	//f &= ~SDL_INIT_CAMERA; // 相机退出有bug
	int kr = SDL_Init(f);
	SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, false);
	SDL_SetEventEnabled(SDL_EVENT_DROP_TEXT, false);
	auto ct = SDL_GetNumRenderDrivers();
	for (size_t i = 0; i < ct; i++)
	{
		auto n = SDL_GetRenderDriver(i);
		if (n)
		{
			rdv.insert(n);
		}
	}
	// Enable native IME.
	SDL_SetHintWithPriority(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_IME_IMPLEMENTED_UI, "composition", SDL_HINT_OVERRIDE);
	//SDL_SetHint(SDL_HINT_RENDER_VULKAN_DEBUG, "true");
	//SDL_SetHintWithPriority(SDL_HINT_IME_INTERNAL_EDITING, "1", SDL_HINT_OVERRIDE);
	//SDL_SetHintWithPriority(SDL_HINT_IME_SHOW_UI, "1", SDL_HINT_OVERRIDE);
#ifdef __ANDROID__
	SDL_SetHintWithPriority(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "1", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_TOUCH_MOUSE_EVENTS, "1", SDL_HINT_OVERRIDE);
#endif


#ifdef _WIN32
	{
		auto ctxe = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
		//auto tr = SetThreadDpiAwarenessContext(ctxe);
		// 设置当前进程的DPI感知等级。
		auto pd = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		auto td = SetThreadDpiHostingBehavior(DPI_HOSTING_BEHAVIOR_MIXED);
		auto k = GetThreadDpiAwarenessContext();
		if (k)
		{
			printf("%p\n", k);
		}
	}
#endif // _WIN32
	system_cursor = (SDL_Cursor**) new size_t[SDL_SystemCursor::SDL_SYSTEM_CURSOR_COUNT];
	memset(system_cursor, 0, sizeof(void*) * SDL_SystemCursor::SDL_SYSTEM_CURSOR_COUNT);
	set_fps(_fps);
#if _WIN32
	auto hr = OleInitialize(NULL);
	SDL_SetWindowsMessageHook(wMessageHook, this);
#endif
#ifdef EVWATCH
	SDL_AddEventWatch([](void* userdata, SDL_Event* e) {
		auto ctx = (app_cx*)userdata;
		if (ctx && e) {
			ctx->call_cb(e);
		}
		return 0;
		}, this);
#endif 
	audio_device = open_audio(0, 0, 0);
}

app_cx::~app_cx()
{
	close_audio(audio_device);
	if (system_cursor)
	{
		for (size_t i = 0; i < SDL_SystemCursor::SDL_SYSTEM_CURSOR_COUNT; i++)
		{
			if (system_cursor[i]) {
				SDL_DestroyCursor(system_cursor[i]);
			}
		}
		delete[]system_cursor; system_cursor = 0;
	}
	if (r2d)
	{
		delete r2d; r2d = 0;
	}
	if (fct)
	{
		delete fct; fct = 0;
	}
	free_fonts_ctx(font_ctx);
	SDL_Quit();
#if _WIN32
	OleUninitialize();
#endif
}
uint32_t get_flags(int fgs)
{
	uint32_t flags = 0;
	if (fgs == 0)fgs = ef_default;
	if (fgs & ef_borderless)
	{
		flags |= SDL_WINDOW_BORDERLESS;
		SDL_SetHintWithPriority("SDL_BORDERLESS_RESIZABLE_STYLE", "1", SDL_HINT_OVERRIDE);
		SDL_SetHintWithPriority("SDL_BORDERLESS_WINDOWED_STYLE", "1", SDL_HINT_OVERRIDE);

	}
	if (fgs & ef_fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN;
	if (fgs & ef_resizable)
		flags |= SDL_WINDOW_RESIZABLE;
	//flags |= SDL_WINDOW_MOUSE_GRABBED;//锁定鼠标在窗口内
	flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_MOUSE_FOCUS | SDL_WINDOW_INPUT_FOCUS;

	if (fgs & ef_transparent)
		flags |= SDL_WINDOW_TRANSPARENT;
	if (fgs & ef_vulkan)
		flags |= SDL_WINDOW_VULKAN;
	if (fgs & ef_metal)
		flags |= SDL_WINDOW_METAL;
	if (fgs & ef_tooltip)
		flags |= SDL_WINDOW_TOOLTIP;
	if (fgs & ef_popup)
		flags |= SDL_WINDOW_POPUP_MENU;
	else if (fgs & ef_utility)
		flags |= SDL_WINDOW_UTILITY;
	return flags;
}
int on_call_we(const SDL_Event* e, form_x* pw);
form_x* app_cx::new_form_renderer(const std::string& title, const glm::ivec2& pos, const glm::ivec2& ws1, int fgs, bool derender, form_x* parent)
{
	auto ws = ws1;
#ifdef __ANDROID__
	ws.x = Android_ScreenWidth;
	ws.y = Android_ScreenHeight;
#endif
	SDL_Window* window = 0;
	SDL_Renderer* renderer = 0;
	auto flags = get_flags(fgs);
	bool setpos = false;
	auto st = SDL_GetSystemTheme();
	if (fgs & ef_tooltip || fgs & ef_popup)
	{
		window = SDL_CreatePopupWindow(parent->_ptr, pos.x, pos.y, ws.x, ws.y, flags);
	}
	else
	{
		setpos = (pos.x > 0 && pos.y > 0);
		window = SDL_CreateWindow(title.c_str(), ws.x, ws.y, flags);
	}
	//SDL_CreateWindowAndRenderer(ws.x, ws.y, get_flags(fgs), &window, &renderer);
	if (!window)
	{
		return 0;
	}
	auto pw = new form_x();
	pw->app = this;
	pw->_ptr = window;
	pw->_size = ws;
	//if (fgs & ef_transparent)
	//	pw->set_alpha(true);
	if (derender) {
		std::string rn;
		auto rdc = SDL_GetHint(SDL_HINT_RENDER_DRIVER);
		if (fgs & ef_cpu && (rdv.find("software") != rdv.end()))
		{
			rn = "software";
		}
		else if (fgs & ef_gpu && (rdv.find("gpu") != rdv.end()))
		{
			rn = "gpu";
		}
		else if (fgs & ef_vulkan && rdv.find("vulkan") != rdv.end())
		{
			rn = "vulkan";
		}
		else if (fgs & ef_dx11 && rdv.find("direct3d11") != rdv.end())
		{
			rn = "direct3d11";
		}
		else if (fgs & ef_dx12 && rdv.find("direct3d12") != rdv.end())
		{
			rn = "direct3d12";
		}
		else if (rdv.find("gpu") != rdv.end())
		{
			rn = "gpu";
		}
		else {
			rn = *rdv.begin();
		}
		renderer = SDL_CreateRenderer(window, rn.c_str());
	}
	int vsync = 0;
	if (renderer) {
		printf("renderer:\t%s\n", SDL_GetRendererName(renderer));
		SDL_GetRenderVSync(renderer, &vsync);
		if (fgs & ef_vsync)
		{
			SDL_SetRenderVSync(renderer, 1);
			set_fps(0);
		}
	}
	if (!pw)
	{
		SDL_DestroyWindow(window);
		if (renderer)
			SDL_DestroyRenderer(renderer);
		return 0;
	}
	if (parent)
	{
		pw->parent = parent;
		parent->childfs.push_back(pw);
	}
	if (setpos) {
		pw->set_pos(pos);
	}
	pw->app = this;
	pw->_ptr = window;
	pw->renderer = renderer;
	pw->_size = ws;
	pce::set_property(window, "form_x", pw);
	SDL_SetWindowTitle(window, title.c_str());
#ifdef _WIN32
	HWND hWnd = (HWND)pce::get_windowptr(window);
	if (fgs & ef_borderless)
	{
		SDL_SetWindowHitTest(window, HitTestCallback2, pw);
	}

#endif
	pw->init_dragdrop();
	forms.push_back(pw);

	return pw;
}
void app_cx::call_cb(SDL_Event* e)
{
	auto fwp = SDL_GetWindowFromID(e->window.windowID);
	auto fw = (form_x*)pce::get_property(fwp, "form_x");
	int rw = 0;

	if (e->type == SDL_EVENT_SYSTEM_THEME_CHANGED) {
		auto st = SDL_GetSystemTheme();
		printf("");
	}
	if (fw) {
		rw = on_call_we(e, fw);
	}
}
void app_cx::sleep_ms(int ms)
{
	SDL_Delay(ms);
	//std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
void app_cx::sleep_ns(int ns)
{
	SDL_DelayNS(ns);
}
uint64_t app_cx::get_ticks() {
	return SDL_GetTicks();
}
void app_cx::set_fps(int n) {
	_fps = n;
	if (n > 0)
	{
		fms = 1000.0 / n;
		fct->set_fps(n);
	}
}

void app_cx::set_syscursor(int type)
{
	if (type < 0)type = 0;
	if (type < SDL_SYSTEM_CURSOR_COUNT)
	{
		auto& psc = system_cursor[type];
		if (!psc)
			psc = SDL_CreateSystemCursor((SDL_SystemCursor)type);
		if (psc)
		{
			SDL_SetCursor(psc);
		}
	}
}

void app_cx::set_defcursor(uint32_t t)
{
	switch ((cursor_st)t)
	{
	case cursor_st::cursor_arrow:
		set_syscursor(SDL_SYSTEM_CURSOR_DEFAULT);
		break;
	case cursor_st::cursor_ibeam:
		set_syscursor(SDL_SYSTEM_CURSOR_TEXT);
		break;
	case cursor_st::cursor_wait:
		set_syscursor(SDL_SYSTEM_CURSOR_WAIT);
		break;
	case cursor_st::cursor_no:
		set_syscursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED);
		break;
	case cursor_st::cursor_hand:
		set_syscursor(SDL_SYSTEM_CURSOR_POINTER);
		break;
	default:
		break;
	}
}

void app_cx::remove(form_x* fw)
{
	if (fw)
	{
		auto& v = forms;
		v.erase(std::remove(v.begin(), v.end(), fw), v.end());
		if (!fw->_ref)
		{
			fw->_ref = 1;
			reforms.push(fw);
		}
	}
}

void app_cx::clearf()
{
	for (; reforms.size();) {
		auto it = reforms.front();
		if (it)
			delete it;
		reforms.pop();
	}
}
const char* power_str(SDL_PowerState t) {
	const char* r = 0;
	switch (t)
	{
	case SDL_POWERSTATE_ERROR:
		r = (char*)u8"确定电源状态时出错";
		break;
	case SDL_POWERSTATE_UNKNOWN:r = (char*)u8"无法确定电源状态";
		break;
	case SDL_POWERSTATE_ON_BATTERY:r = (char*)u8"未插入电源，使用电池运行";
		break;
	case SDL_POWERSTATE_NO_BATTERY:r = (char*)u8"无电池";
		break;
	case SDL_POWERSTATE_CHARGING:r = (char*)u8"充电中";
		break;
	case SDL_POWERSTATE_CHARGED:r = (char*)u8"接通电源，电池已充满";
		break;
	default:
		break;
	}
	return r;
}
glm::vec3 app_cx::get_power_info()
{
	glm::vec3 v = {};
	int s = 0, p = 0;
	v.z = SDL_GetPowerInfo(&s, &p);
	v.x = s < 0 ? 0 : s;
	v.y = p < 0 ? 0.0 : 0.01 * p;
	return v;
}

const char* app_cx::get_power_str()
{
	auto p = get_power_info();
	return power_str((SDL_PowerState)p.z);
}
void app_cx::kncdown()
{
	if (nc_down)
	{
		for (auto it : forms) {
			it->hide_child();
		}
		//printf("event:1\t \n");
		nc_down = false;
	}
}

SDL_AudioSpec get_spec(int format_idx, int channels, int freq) {

	SDL_AudioSpec spec = {};
	SDL_AudioFormat format[] = { SDL_AUDIO_S16, SDL_AUDIO_S32, SDL_AUDIO_F32 };
	format_idx = std::clamp(format_idx, 0, 2);
	spec.format = format[format_idx];
	spec.channels = channels;
	spec.freq = freq;
	return spec;
}
// 音频
uint32_t app_cx::open_audio(int format, int channels, int freq)
{
	auto spec = get_spec(format, channels, freq);
	auto audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, freq && channels ? &spec : NULL);
	if (audio_device == 0) {
		SDL_Log("error\tCouldn't open audio device: %s", SDL_GetError());
	}
	return audio_device;
}
void app_cx::close_audio(uint32_t dev)
{
	if (dev)
		SDL_CloseAudioDevice(dev);
}
uint32_t app_cx::get_audio_device() {
	return audio_device;
}

// 类型、通道数、采样数
void* app_cx::new_audio_stream0(int format_idx, int channels, int freq)
{
	return new_audio_stream(audio_device, format_idx, channels, freq);
}
void* app_cx::new_audio_stream(uint32_t dev, int format_idx, int channels, int freq)
{
	auto spec = get_spec(format_idx, channels, freq);
	auto stream = SDL_CreateAudioStream(&spec, 0);
	if (!SDL_BindAudioStream(dev, stream)) {  /* once bound, it'll start playing when there is data available! */
		SDL_Log("Failed to bind stream to device: %s", SDL_GetError());
	}
	return stream;
}
void app_cx::bindaudio(uint32_t dev, void* st) {
	SDL_AudioStream* as = (SDL_AudioStream*)st;
	auto d = SDL_GetAudioStreamDevice(as);
	if (d == 0 && !SDL_BindAudioStream(dev, as)) {  /* once bound, it'll start playing when there is data available! */
		SDL_Log("Failed to bind stream to device: %s", SDL_GetError());
	}
}
void app_cx::unbindaudio(void* st) {
	SDL_AudioStream* as = (SDL_AudioStream*)st;
	if (st)
		SDL_UnbindAudioStream(as);
}
void app_cx::unbindaudios(void** st, int count) {
	SDL_AudioStream** as = (SDL_AudioStream**)st;
	if (st && *as && count > 0)
		SDL_UnbindAudioStreams(as, count);
}
void app_cx::free_audio_stream(void* st) {
	if (st)
	{
		unbindaudio(st);
		SDL_DestroyAudioStream((SDL_AudioStream*)st);
	}
}
int app_cx::get_audio_dst_framesize(void* st)
{
	auto stream = (SDL_AudioStream*)st;
	SDL_AudioSpec dst_spec = {};
	SDL_GetAudioStreamFormat(stream, 0, &dst_spec);
	return SDL_AUDIO_FRAMESIZE(dst_spec);
}
int app_cx::get_audio_stream_queued(void* st)
{
	return st ? SDL_GetAudioStreamQueued((SDL_AudioStream*)st) : 0;
}
int app_cx::get_audio_stream_available(void* st)
{
	return st ? SDL_GetAudioStreamAvailable((SDL_AudioStream*)st) : 0;
}
void app_cx::put_audio(void* stream, void* data, int len)
{
	auto st = (SDL_AudioStream*)stream;
	if (stream && data && len > 0) {
		SDL_PutAudioStreamData((SDL_AudioStream*)st, data, (int)len);
	}
}
void app_cx::pause_audio(void* st_, int v)
{
	auto st = (SDL_AudioStream*)st_;
	if (st)
	{
		if (v)
			SDL_PauseAudioStreamDevice(st);
		else
			SDL_ResumeAudioStreamDevice(st);
	}
}
bool app_cx::mix_audio(uint8_t* dst, uint8_t* src, int format, size_t len, float volume)
{
	SDL_AudioFormat fmt[] = { SDL_AUDIO_S16 , SDL_AUDIO_S32 , SDL_AUDIO_F32 };
	format = glm::clamp(format, 0, 2);
	return SDL_MixAudio(dst, src, fmt[format], len, volume);
}
void app_cx::clear_audio(void* st_)
{
	auto st = (SDL_AudioStream*)st_;
	if (st)
	{
		SDL_ClearAudioStream(st);
	}
}

int app_cx::get_event()
{
	int64_t ts = 0;
#if 1
	SDL_Event e = {};
	while (SDL_PollEvent(&e) != 0)
	{
		if (e.type == SDL_EVENT_KEY_DOWN/* && e.key.repeat*/)
		{
			SDL_SetEventEnabled(SDL_EVENT_KEY_DOWN, 0);
			ts = 2;
		}
		if (e.type == SDL_EVENT_KEY_UP) {
			SDL_SetEventEnabled(SDL_EVENT_KEY_DOWN, 1);
		}
#ifndef EVWATCH
		call_cb(&e);
#endif
		//break;
	}
#endif
	if (c_fps != _fps)
	{
		set_fps(c_fps);
	}
	clearf();
	if (forms.empty())
	{
		prev_time = 0;
	}
	if (e.type)
	{
		for (auto it : forms)
		{
			it->update_w();
		}
	}
	return ts;
}

void app_cx::render(double delta)
{
	for (auto it : forms)
	{
		it->present(delta);
	}
}

int app_cx::run_loop(int t)
{
	int t1 = t;
	do {
		fct->restart();
		uint32_t curr_time = SDL_GetTicks();
		if (prev_time > 0)
		{
			double delta = (float)(curr_time - prev_time) / 1000.0f;
			auto length = forms.size();
			for (size_t i = 0; i < length; i++)
			{
				auto it = forms[i];
				it->update(delta);
			}
			render(delta);
			auto power = get_power_info();
		}
		prev_time = curr_time;
		int gev = get_event();
		if (_fps > 0 && gev < 1)
		{
			auto kt = (fct->get_time() + fct->extra_time);
			do {
				get_event();
				sleep_ms(2);
			} while ((fct->get_time() + fct->extra_time) < fct->screen_ticks_per_frame);
			if (fct->get_time() < (fct->screen_ticks_per_frame)) {
				fct->extra_time -= fct->screen_ticks_per_frame - fct->get_time();
			}
			else {
				fct->extra_time += fct->get_time() - fct->screen_ticks_per_frame;
			}
		}
		if (t1 > 0)
		{
			if (t == 1)
			{
				break;
			}
			t--;
		}
	} while (prev_time > 0);
	return t;
}
#endif

// 2D骨骼渲染
#if 1
glm::vec2 getv2(slot_t* s, const glm::vec2& ps)
{
	glm::vec3 v2 = { ps, 1 };
	if (s->joints.x >= s->bone->count || s->joints.y >= s->bone->count || s->joints.z >= s->bone->count || s->joints.w >= s->bone->count)
	{
		return v2;
	}
	glm::mat3 skinningMatrix =
		s->weights.x * s->bone->m[s->joints.x] +
		s->weights.y * s->bone->m[s->joints.y] +
		s->weights.z * s->bone->m[s->joints.z] +
		s->weights.w * s->bone->m[s->joints.w];
	v2 = skinningMatrix * v2;
	return v2;
}
SDL_FPoint get_v2(slot_t* s, const glm::vec2& ps)
{
	glm::vec3 v2 = { ps, 1 };
	if (s->joints.x >= s->bone->count || s->joints.y >= s->bone->count || s->joints.z >= s->bone->count || s->joints.w >= s->bone->count)
	{
		return { v2.x,v2.y };
	}
	glm::mat3 skinningMatrix =
		s->weights.x * s->bone->m[s->joints.x] +
		s->weights.y * s->bone->m[s->joints.y] +
		s->weights.z * s->bone->m[s->joints.z] +
		s->weights.w * s->bone->m[s->joints.w];

	v2 = skinningMatrix * v2;
	return { v2.x,v2.y };
}
// todo 骨骼动画渲染
render_2d::render_2d()
{
}

render_2d::~render_2d()
{
}

SDL_BlendMode get_blend_x(BlendMode_e blendMode, bool pma);
SDL_BlendMode get_blend(BlendMode_e blend_mode, bool usePremultipliedAlpha)
{
	static struct bmt {
		SDL_BlendMode normal = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
			SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);

		SDL_BlendMode additive = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
			SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);

		SDL_BlendMode multiply = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
			SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);

		SDL_BlendMode screen = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR, SDL_BLENDOPERATION_ADD,
			SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR, SDL_BLENDOPERATION_ADD);

		SDL_BlendMode normalPma = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
			SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);

		SDL_BlendMode additivePma = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
			SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);

		SDL_BlendMode multiplyPma = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
			SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);

		SDL_BlendMode screenPma = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR, SDL_BLENDOPERATION_ADD,
			SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR, SDL_BLENDOPERATION_ADD);
	}blend_n;

	SDL_BlendMode blend = SDL_BLENDMODE_NONE;
	switch ((BlendMode_e)blend_mode) {
	case BlendMode_e::normal:
		blend = usePremultipliedAlpha ? blend_n.normalPma : blend_n.normal;
		break;
	case BlendMode_e::additive:
		blend = usePremultipliedAlpha ? blend_n.additivePma : blend_n.additive;
		break;
	case BlendMode_e::multiply:
		blend = usePremultipliedAlpha ? blend_n.multiplyPma : blend_n.multiply;
		break;
	case BlendMode_e::screen:
		blend = usePremultipliedAlpha ? blend_n.screenPma : blend_n.screen;
		break;
	}
	return blend;
}

void render_2d::draw_data(SDL_Renderer* renderer, skeleton_t* skeleton)
{
	// 如果骨骼不可见，则提前退出
	if (!skeleton || skeleton->color.w == 0 || !skeleton->visible) return;
	unsigned short quadIndices[] = { 0,1,2,2,3,0 };
	struct { SDL_Texture* texture; SDL_BlendMode blendMode; } states = {};
	vertexs.clear();
	states.texture = NULL;
	int offsetidx = 0;
	SDL_Vertex vertex = {};
	SDL_Texture* texture = NULL;
	for (size_t i = 0; i < skeleton->slots_count; ++i) {
		slot_t* slot = skeleton->solt_data + i;
		auto attachment = slot->attachment;
		// 如果附件插槽颜色为0或骨骼未处于活动状态，则提前退出
		if (slot->color.w == 0 || !slot->bone->count || !attachment || !attachment->isActive || attachment->color.w == 0) { continue; }
		texture = (SDL_Texture*)attachment->tex;
		auto ssc = skeleton->color * slot->color;
		auto c = ssc * attachment->color;
		*(glm::vec4*)(&vertex.color) = glm::clamp(c, 0.0f, 1.0f);
		auto blend = get_blend_x((BlendMode_e)slot->blend_mode, skeleton->usePremultipliedAlpha);
		//if (states.texture == 0) states.texture = texture;
		if (states.blendMode != blend || states.texture != texture) {
			if (vertexs.size() > 0) {
				auto count = indexs.size() ? indexs.size() : vertexs.size();
				assert(count % 3 == 0);	// 渲染合批数据
				SDL_SetTextureBlendMode(states.texture, states.blendMode);
				SDL_RenderGeometry(renderer, states.texture, vertexs.data(), vertexs.size(), indexs.size() ? indexs.data() : nullptr, indexs.size());
				vertexs.clear(); indexs.clear();
			}
			states.blendMode = blend;
			states.texture = texture;
		}
		// 顶点数据添加到合批
		offsetidx = vertexs.size();
		if (attachment->colors) {
			for (size_t i = 0; i < attachment->vs; i++)
			{
				vertex.position = get_v2(slot, attachment->pos[i]);
				vertex.tex_coord = *((SDL_FPoint*)&attachment->uvs[i]);
				auto c0 = ssc * attachment->colors[i];
				vertex.color = *((SDL_FColor*)&c0);
				vertexs.push_back(vertex);
			}
		}
		else {
			for (size_t i = 0; i < attachment->vs; i++)
			{
				vertex.position = get_v2(slot, attachment->pos[i]);
				vertex.tex_coord = *((SDL_FPoint*)&attachment->uvs[i]);
				vertexs.push_back(vertex);
			}
		}
		if (attachment->is > 0) {
			auto pss = indexs.size();
			indexs.resize(pss + attachment->is);
			auto dst = indexs.data() + pss;
			memcpy(dst, attachment->idxs, attachment->is * sizeof(int));
			auto t = dst;
			if (offsetidx > 0)
			{
				for (size_t i = 0; i < attachment->is; i++, t++)
				{
					*t += offsetidx;
				}
			}
		}
	}
	if (vertexs.size() > 0) {
		auto count = indexs.size() ? indexs.size() : vertexs.size();
		assert(count % 3 == 0); // 渲染合批数据
		SDL_SetTextureBlendMode(states.texture, states.blendMode);
		SDL_RenderGeometry(renderer, states.texture, vertexs.data(), vertexs.size(), indexs.size() ? indexs.data() : nullptr, indexs.size());
		vertexs.clear();
		indexs.clear();
	}
}
SDL_BlendMode get_blend_x(BlendMode_e blendMode, bool pma)
{
	SDL_BlendMode b = SDL_BLENDMODE_BLEND;
	static SDL_BlendMode screen = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR, SDL_BLENDOPERATION_ADD);
	switch (blendMode)
	{
	case BlendMode_e::none:
		break;
	case BlendMode_e::normal:
		b = !pma ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_BLEND_PREMULTIPLIED;
		break;
	case BlendMode_e::additive:
		b = !pma ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_ADD_PREMULTIPLIED;
		break;
	case BlendMode_e::multiply:
		b = SDL_BLENDMODE_MUL;
		break;
	case BlendMode_e::modulate:
		b = SDL_BLENDMODE_MOD;
		break;
	case BlendMode_e::screen:
		b = screen;
		break;
	default:
		break;
	}

	return b;
}
#if 0
void render_2d::draw_data(SDL_Renderer* renderer, skeleton_t* skeleton)
{
	// 如果骨骼不可见，则提前退出
	if (!skeleton || skeleton->color.w == 0 || !skeleton->visible) return;
	unsigned short quadIndices[] = { 0,1,2,2,3,0 };
	struct { SDL_Texture* texture; SDL_BlendMode blendMode; } states = {};
	vertexs.clear();
	states.texture = NULL;
	int offsetidx = 0;
	SDL_Vertex vertex = {};
	SDL_Texture* texture = NULL;
	for (size_t i = 0; i < skeleton->slots_count; ++i) {
		slot_t* slot = skeleton->solt_data + i;
		auto attachment = slot->attachment;
		// 如果附件插槽颜色为0或骨骼未处于活动状态，则提前退出
		if (slot->color.w == 0 || !slot->bone->count || !attachment || !attachment->isActive || attachment->color.w == 0) { continue; }
		texture = (SDL_Texture*)attachment->tex;
		auto ssc = skeleton->color * slot->color;
		auto c = ssc * attachment->color;
		*(glm::vec4*)(&vertex.color) = glm::clamp(c, 0.0f, 1.0f);
		auto blend = get_blend_x((BlendMode_e)slot->blend_mode, skeleton->usePremultipliedAlpha);
		//if (states.texture == 0) states.texture = texture;
		if (states.blendMode != blend || states.texture != texture) {
			if (vertexs.size() > 0) {
				auto count = indexs.size() ? indexs.size() : vertexs.size();
				assert(count % 3 == 0);	// 渲染合批数据
				SDL_SetTextureBlendMode(states.texture, states.blendMode);
				SDL_RenderGeometry(renderer, states.texture, vertexs.data(), vertexs.size(), indexs.size() ? indexs.data() : nullptr, indexs.size());
				vertexs.clear(); indexs.clear();
			}
			states.blendMode = blend;
			states.texture = texture;
		}
		// 顶点数据添加到合批
		offsetidx = vertexs.size();
		if (attachment->colors) {
			for (size_t i = 0; i < attachment->vs; i++)
			{
				vertex.position = get_v2(slot, attachment->pos[i]);
				vertex.tex_coord = *((SDL_FPoint*)&attachment->uvs[i]);
				auto c0 = ssc * attachment->colors[i];
				vertex.color = *((SDL_FColor*)&c0);
				vertexs.push_back(vertex);
			}
		}
		else {
			for (size_t i = 0; i < attachment->vs; i++)
			{
				vertex.position = get_v2(slot, attachment->pos[i]);
				vertex.tex_coord = *((SDL_FPoint*)&attachment->uvs[i]);
				vertexs.push_back(vertex);
			}
		}
		if (attachment->is > 0) {
			auto pss = indexs.size();
			indexs.resize(pss + attachment->is);
			auto dst = indexs.data() + pss;
			memcpy(dst, attachment->idxs, attachment->is * sizeof(int));
			auto t = dst;
			if (offsetidx > 0)
			{
				for (size_t i = 0; i < attachment->is; i++, t++)
				{
					*t += offsetidx;
				}
			}
		}
	}
	if (vertexs.size() > 0) {
		auto count = indexs.size() ? indexs.size() : vertexs.size();
		assert(count % 3 == 0); // 渲染合批数据
		SDL_SetTextureBlendMode(states.texture, states.blendMode);
		SDL_RenderGeometry(renderer, states.texture, vertexs.data(), vertexs.size(), indexs.size() ? indexs.data() : nullptr, indexs.size());
		vertexs.clear();
		indexs.clear();
	}
}
#endif

SDL_PixelFormat get_rgbafe() {
	uint32_t rmask, gmask, bmask, amask;
	rmask = 0x000000FF; gmask = 0x0000FF00; bmask = 0x00FF0000; amask = 0xFF000000;	// RGBA8888模式 
	int depth = 32;
	return SDL_GetPixelFormatForMasks(depth, rmask, gmask, bmask, amask);
}
SDL_Texture* newuptex(SDL_Renderer* renderer, image_ptr_t* img) {
	SDL_Texture* pr = 0;
	if (img && img->width > 0 && img->height > 0)
	{
		SDL_Texture* ptx = (SDL_Texture*)img->texid;
		if (renderer)
		{
			if (ptx)
			{
				glm::vec2 oldss = {};
				if (SDL_GetTextureSize(ptx, &oldss.x, &oldss.y)) {
					glm::ivec2 iss = oldss;
					if (img->width != iss.x || img->height != iss.y) {
						SDL_DestroyTexture(ptx);
						ptx = nullptr;
					}
				}
			}
			if (!ptx)
			{
				auto ty = img->type;
				if (ty == 0) ty = SDL_PIXELFORMAT_ABGR8888;
				if (ty == 1) ty = SDL_PIXELFORMAT_ARGB8888;
				ptx = SDL_CreateTexture(renderer, (SDL_PixelFormat)ty, img->static_tex ? SDL_TEXTUREACCESS_STATIC : SDL_TEXTUREACCESS_STREAMING, img->width, img->height);
				img->texid = ptx; img->stride = 0;
				img->valid = true;
				pr = ptx;
				// 设置texture 混合模式
				//SDL_SetTextureBlendMode(p, SDL_BLENDMODE_BLEND);
				SDL_SetTextureBlendMode(ptx, get_blend_x((BlendMode_e)img->blendmode, img->multiply));
			}
		}
		if (img->data && img->valid)
		{
			img->valid = false;
			if (img->stride < 1)img->stride = img->width * 4;
			SDL_UpdateTexture(ptx, 0, img->data, img->stride);
		}
	}
	return pr;
}
void canvas_atlas_update(canvas_atlas* p, SDL_Renderer* renderer, float delta)
{
	if (!p)return;
	if (p->_renderer != renderer)
	{
		for (auto it : p->_atlas_t)
		{
			if (it->img && it->img->texid)
			{
				auto ptx = (SDL_Texture*)it->img->texid;
				SDL_DestroyTexture(ptx);
				auto& v = p->_texs_t;
				v.erase(std::remove(v.begin(), v.end(), ptx), v.end());
				it->img->texid = 0;
				it->img->stride = 0;
			}
		}
		for (auto it : p->_atlas_cx)
		{
			if (it->img && it->img->texid)
			{
				auto ptx = (SDL_Texture*)it->img->texid;
				SDL_DestroyTexture(ptx);
				auto& v = p->_texs_t;
				v.erase(std::remove(v.begin(), v.end(), ptx), v.end());
				it->img->texid = 0;
				it->img->stride = 0;
			}
		}
		p->valid = true;
		p->_renderer = renderer;
		p->destroy_texture_cb = (void(*)(void* tex))SDL_DestroyTexture;
	}
	for (auto it : p->_atlas_t)
	{
		auto optx = (SDL_Texture*)it->img->texid;
		auto ptx = newuptex(renderer, it->img);
		if (ptx)
		{
			if (optx && optx != ptx) {
				auto& v = p->_texs_t;
				SDL_DestroyTexture(optx);
				v.erase(std::remove(v.begin(), v.end(), optx), v.end());
			}
			p->_texs_t.push_back(ptx);
			p->valid = true;
		}
	}
	for (auto it : p->_atlas_cx)
	{
		auto optx = (SDL_Texture*)it->img->texid;
		auto ptx = newuptex(renderer, it->img);
		if (ptx)
		{
			if (optx && optx != ptx) {
				auto& v = p->_texs_t;
				SDL_DestroyTexture(optx);
				v.erase(std::remove(v.begin(), v.end(), optx), v.end());
			}
			p->_texs_t.push_back(ptx);
			p->valid = true;
		}
	}

	p->apply();
}


#endif


// 窗口
#if 1

template<class T>
class lock_auto_x
{
public:
	T* p = 0;
public:
	lock_auto_x(T* k) {
		if (k)
		{
			k->lock();
			p = k;
		}
	}
	~lock_auto_x() {
		if (p)
			p->unlock();
	}

private:

};
form_x::form_x()
{
	//events = new std::vector<event_fw>[(uint32_t)devent_type_e::max_det];
	events_a = new std::vector<event_fw>();
	io = new mouse_state_t();
}

form_x::~form_x()
{
	//lock_auto_x lx(&lkecb);
	clear_wt();
	for (auto it : childfs) {
		it->_ptr = 0;
		app->remove(it);
	}
	if (io)
	{
		delete io;
	}
	io = 0;
	if (events_a)
	{
		delete events_a;
	}
	events_a = 0;
	childfs.clear();
	_ref = 1;
	app->remove(this);
	destroy();
	sdlfree(clipstr); clipstr = 0;
	_ptr = 0;
}

void form_x::init_dragdrop()
{
	// OLE拖放
	if (!dragdrop)
	{
		dragdrop = new hz::drop_regs();
		dragdrop->init((HWND)pce::get_windowptr(_ptr));
		_oledrop = new hz::drop_info_cx();
		_oledrop->on_drop_cb = [=](int type, int idx)
			{
				ole_drop_et t = {};
				t.x = _dx; t.y = _dy;
				t.fmt = _oledrop->_type;
				//printf("drag ole :%d\t%d\n", t.x, t.y);
				if (type == 0)
				{
					_oledrop->_tmp.resize(1);
					_oledrop->_tmp[0] = _oledrop->_str.c_str();
				}
				else
				{
					auto ct = _oledrop->_files.size();
					t.count = ct;
					if (ct)
					{
						_oledrop->_tmp.resize(ct);
						int i = 0;
						for (auto& it : _oledrop->_files) {

							_oledrop->_tmp[i++] = it.c_str();
						}
					}
				}
				t.str = _oledrop->_tmp.data();
				t.count = _oledrop->_tmp.size();
				trigger((uint32_t)devent_type_e::ole_drop_e, &t);
			};
	}
	dragdrop->set_over([=](int x, int y, int fmt)
		{
			POINT pt = { x, y };
			ScreenToClient((HWND)pce::get_windowptr(_ptr), &pt);
			//printf("on drag ole :%d\t%d\tpos:%d\t%d\n", pt.x, pt.y, x, y);
			if (_dx != pt.x || _dy != pt.y)
			{
				_dx = pt.x;
				_dy = pt.y;
				hz::drop_info_cx* rt = 0;
				//if (_hot_em)
				//{
				//	//trigger((uint32_t)devent_type_e::ole_drop_e, &t);
				//	//rt = _hot_em->call_mouse_move_oledrop(pt.x, pt.y);
				//}
				if (!rt)
				{
					ole_drop_et t = {};
					t.x = pt.x; t.y = pt.y;
					{
						t.count = 0;
						t.fmt = fmt;
						t.has = &_oledrop->has;
						(*t.has) = 0;
						trigger((uint32_t)devent_type_e::ole_drop_e, &t);
						if (_oledrop->has == 1)
							rt = _oledrop;
					}
				}
				//if (rt)
				dragdrop->set_target(rt);
			}

		});

}
void form_x::set_curr_drop(hz::drop_info_cx* p)
{
#ifdef _WIN32
	if (dragdrop)
		dragdrop->set_target(p);
#endif
}

void form_x::add_event(void* ud, std::function<void(uint32_t type, et_un_t* e, void* ud)> cb)
{
	lock_auto_x lx(&lkecb);
	events_a->push_back({ ud, cb });
}


size_t form_x::remove_event(void* ud)
{
	lock_auto_x lx(&lkecb);
	auto& v = events_a[0];
	auto ns = v.size();
	v.erase(std::remove_if(v.begin(), v.end(), [ud](event_fw& r) {return r.ptr == ud; }), v.end());
	return ns - v.size();
}
void form_x::move2end_e(void* ud)
{
	lock_auto_x lx(&lkecb);
	auto& v = events_a[0];
	for (auto it = v.begin(); it != v.end(); it++)
	{
		if (it->ptr == ud)
		{
			auto kt = *it;
			v.erase(it);
			v.push_back(kt);
			break;
		}
	}
}
void form_x::set_input_ptr(void* ud)
{
	lock_auto_x lx(&lkecb);
	input_ptr = (input_state_t*)ud;
}

void form_x::trigger(uint32_t etype, void* e)
{
	devent_type_e type = (devent_type_e)etype;
	lkecb.lock();
	auto cbs0 = *events_a;
	auto iptr = input_ptr;
	lkecb.unlock();
	et_un_t et = {};
	et.form = this;
	et.v.b = (mouse_button_et*)e;
	bool btn = !(type == devent_type_e::mouse_button_e && et.v.b->down == 0);
	do
	{
		if (type == devent_type_e::text_input_e || devent_type_e::text_editing_e == type)
		{
			if (iptr)
			{
				iptr->cb((uint32_t)type, &et, iptr->ptr);
			}
			return;
		}
		if (et.ret)
		{
			break;
		}
		if (cbs0.size())
		{
			for (auto it = cbs0.rbegin(); it != cbs0.rend(); it++)
			{
				if (it->cb) {
					it->cb((uint32_t)type, &et, it->ptr);
					if (et.ret && btn)
					{
						break;
					}
				}
			}
			if (et.ret && btn)
			{
				break;
			}
		}
		for (int i = 1; i >= 0; i--) {
			auto pt = _planes[i];
			for (auto it = pt.rbegin(); it != pt.rend(); it++)
			{
				(*it)->on_event((uint32_t)type, &et);
				if (et.ret && btn)
				{
					break;
				}
			}
			if (et.ret && btn)
			{
				break;
			}
		}
	} while (0);
}
void form_x::set_capture()
{
	SDL_CaptureMouse(1);
}

void form_x::release_capture()
{
	SDL_CaptureMouse(0);
}
// 关闭窗口
void form_x::close() {
	SDL_Event e = {};
	e.type = e.window.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
	e.window.windowID = SDL_GetWindowID(_ptr);
	SDL_PushEvent(&e);
	//SDL_SendWindowEvent(_ptr, SDL_EVENT_WINDOW_CLOSE_REQUESTED, 0, 0);
}
// 显示/隐藏窗口
void form_x::show() {

	lock_auto_x lx(&lkqcv);
	qcmd_value.push({ (int)fcv_type::e_show,0,0,0 });
}
void form_x::hide() {

	lock_auto_x lx(&lkqcv);
	qcmd_value.push({ (int)fcv_type::e_hide,0,0,0 });
}
void form_x::show_reverse() {

	lock_auto_x lx(&lkqcv);
	qcmd_value.push({ (int)fcv_type::e_visible_rev,0,0,0 });
}
void form_x::raise()
{
	SDL_RaiseWindow(_ptr);
}
bool form_x::get_visible()
{
	auto f = SDL_GetWindowFlags(_ptr);
	bool v = !(f & SDL_WINDOW_HIDDEN);
	return v;
}


void et2key(const SDL_Event* e, keyboard_et* ekm)
{
	if (!e || !(e->type == SDL_EVENT_KEY_DOWN || e->type == SDL_EVENT_KEY_UP))return;
	int ks = 0;
	auto pk = SDL_GetKeyboardState(&ks);
	int key = (int)e->key.scancode;
	ekm->sym = (e->key.key);
	ekm->keycode = SDL_GetKeyFromScancode(e->key.scancode, e->key.mod, 1);
	ekm->scancode = key;      /**< SDL physical key code - see ::SDL_Scancode for details */
	ekm->mod = e->key.mod;                 /**< current key modifiers */
	ekm->down = e->key.down;        /**< ::SDL_PRESSED or ::SDL_RELEASED */
	ekm->repeat = e->key.repeat;       /**< Non-zero if this is a key repeat */
	static int64_t ts = 0, ts1 = 0;

	if (ekm->repeat > 0) {
		ts += (e->button.timestamp - ts1) * 0.000001;
		ekm->repeat = ekm->repeat;
		//printf("ms: %d\n", ts); ts = 0; ts1 = e->button.timestamp;
	}
	else {
		ts = 0; ts1 = e->button.timestamp;
	}
	int f1 = SDLK_F1;
	int ms = SDL_GetModState();
	static int kcs[] = { SDLK_END, SDLK_DOWN, SDLK_PAGEDOWN, SDLK_LEFT, 0, SDLK_RIGHT, SDLK_HOME, SDLK_UP, SDLK_PAGEUP, SDLK_INSERT, SDLK_DELETE };
	if ((!(key<SDL_SCANCODE_KP_1 || key> SDL_SCANCODE_KP_PERIOD)) && !(ms & SDL_KMOD_NUM))
	{
		ekm->keycode = kcs[key - SDL_SCANCODE_KP_1];
	}
	//ekm->kn = SDL_GetKeyName(ekm->keycode);
	if (ms & SDL_KMOD_LCTRL || ms & SDL_KMOD_RCTRL)
	{
		ekm->kmod |= KM_CTRL;
	}
	if (ms & SDL_KMOD_LSHIFT || ms & SDL_KMOD_RSHIFT)
	{
		ekm->kmod |= KM_SHIFT;
	}
	if (ms & SDL_KMOD_LALT || ms & SDL_KMOD_RALT)
	{
		ekm->kmod |= KM_ALT;
	}
	if (ms & SDL_KMOD_LGUI || ms & SDL_KMOD_RGUI)
	{
		ekm->kmod |= KM_GUI;
	}

}

/*struct mouse_move_et;
struct mouse_button_et;
struct mouse_wheel_et;
struct keyboard_et;
struct text_editing_et;
struct text_input_et;
struct finger_et;
ole_drop_et*/


bool on_call_emit(const SDL_Event* e, form_x* pw)
{
	if (!pw)return false;
	switch (e->type)
	{
	case SDL_EVENT_MOUSE_MOTION:
	{
		//auto afp = ctx->get_activate_form();
		mouse_move_et mt = {};
		mt.x = e->motion.x;
		mt.y = e->motion.y;			// 鼠标移动坐标
		mt.xrel = e->motion.xrel;
		mt.yrel = e->motion.yrel;		// The relative motion in the XY direction 
		mt.which = e->motion.which;		// 鼠标实例 

		pw->hittest({ mt.x, mt.y });
		if (pw->io) {
			pw->io->MousePos = { mt.x,mt.y };
			pw->io->MouseDelta = { mt.xrel,mt.yrel };
			int ms = SDL_GetModState();
			pw->io->KeyCtrl = (ms & SDL_KMOD_LCTRL || ms & SDL_KMOD_RCTRL);
			pw->io->KeyShift = (ms & SDL_KMOD_LSHIFT || ms & SDL_KMOD_RSHIFT);
			pw->io->KeyAlt = (ms & SDL_KMOD_LALT || ms & SDL_KMOD_RALT);
			pw->io->KeySuper = (ms & SDL_KMOD_LGUI || ms & SDL_KMOD_RGUI);
		}
		pw->trigger((uint32_t)devent_type_e::mouse_move_e, &mt);
		pw->_last_pos = { mt.x, mt.y };
		if (mt.cursor > 0) {
			pw->app->set_defcursor(mt.cursor);
		}
	}
	break;
	case SDL_EVENT_MOUSE_WHEEL:
	{
		mouse_wheel_et wt = {};
		wt.which = e->wheel.which;
		int dir = e->wheel.direction;
		wt.x = e->wheel.x;
		wt.y = e->wheel.y;
		float preciseX = e->wheel.mouse_x;// preciseX;
		float preciseY = e->wheel.mouse_y;
		if (pw->io && !pw->_HitTest) {
			pw->io->wheel = { wt.x,wt.y };
		}
		pw->trigger((uint32_t)devent_type_e::mouse_wheel_e, &wt);
	}
	break;
#if 0
	case SDL_MULTIGESTURE:
	{
		auto& mg = e->mgesture;
		mgesture_et t = {};
		t.touchId = mg.touchId;
		t.dDist = mg.dDist;
		t.dTheta = mg.dTheta;
		t.numFingers = mg.numFingers;
		t.x = mg.x;
		t.y = mg.y;
		pw->trigger(t);
	}
	break;
#endif
	case SDL_EVENT_FINGER_DOWN:
	case SDL_EVENT_FINGER_UP:
	case SDL_EVENT_FINGER_MOTION:
	{
		finger_et ft = {};
		ft.t = e->type - SDL_EVENT_FINGER_DOWN + 1;
		ft.tid = e->tfinger.fingerID;
		ft.touchId = e->tfinger.fingerID;
		ft.x = e->tfinger.x; ft.y = e->tfinger.y;
		ft.pressure = e->tfinger.pressure;
		if (e->type == SDL_EVENT_FINGER_DOWN)
		{
			pw->hide_child();
		}
		pw->trigger((uint32_t)devent_type_e::finger_e, &ft);

	}break;
	//case SDL_TOUCH_MOUSEID: 
	case SDL_EVENT_MOUSE_BUTTON_DOWN:	//1
	case SDL_EVENT_MOUSE_BUTTON_UP:		//0
	{
		mouse_button_et t = {};
		t.which = e->button.which;
		t.button = e->button.button;
		t.down = e->button.down; //SDL_PRESSED; SDL_RELEASED;
		t.clicks = e->button.clicks;
		t.x = e->button.x;
		t.y = e->button.y;
		if (t.down)
		{
			pw->hide_child();
		}
		//pw->hittest({ t.x,t.y });
		if (pw->io && !pw->_HitTest) {
			pw->io->MouseDown[t.button - 1] = t.down;
		}
		else {
			pw->io->MouseDown[t.button - 1] = 0;
		}
		pw->trigger((uint32_t)devent_type_e::mouse_button_e, &t);

		if (pw && pw->capture_type)
		{
			if (t.down)
			{
				pw->set_capture();		// 锁定鼠标	
			}
			else {
				pw->release_capture();	// 释放鼠标			
				// 开始输入法
				if (pw->input_ptr)
				{
					pw->start_text_input();
					auto irc = (glm::ivec4*)&pw->input_ptr->x;
					pw->set_ime_pos(*irc);
				}
			}
		}
	}
	break;
	case SDL_EVENT_TEXT_INPUT:
	{
		text_input_et t = {};
		t.text = (char*)e->text.text;
		pw->trigger((uint32_t)devent_type_e::text_input_e, &t);
		auto irc = (glm::ivec4*)&t.x;
		pw->set_ime_pos(*irc);
	}
	break;
	case SDL_EVENT_TEXT_EDITING:
	{
		text_editing_et t = {};
		t.text = (char*)e->edit.text;
		t.start = e->edit.start;
		t.length = e->edit.length;
		pw->trigger((uint32_t)devent_type_e::text_editing_e, &t);
		auto irc = (glm::ivec4*)&t.x;
		pw->set_ime_pos(*irc);
	}
	break;
	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP:
	{
		keyboard_et t = {};
		et2key(e, &t);
		auto kn = SDL_GetKeyName(t.keycode);
		pw->io->KeysDown[*kn] = t.down;
		pw->io->KeysDown[VK_SHIFT] = (t.kmod & KM_SHIFT);
		pw->io->KeyShift = (t.kmod & KM_SHIFT);
		pw->io->KeyAlt = (t.kmod & KM_ALT);
		pw->io->KeyCtrl = (t.kmod & KM_CTRL);
		pw->io->KeySuper = (t.kmod & KM_GUI);
		pw->trigger((uint32_t)devent_type_e::keyboard_e, &t);
	}
	break;
	case SDL_EVENT_DROP_BEGIN:
		pw->drop_text.clear();
		break;
	case SDL_EVENT_DROP_POSITION:
	{
		ole_drop_et t = {};
		t.x = e->drop.x;
		t.y = e->drop.y;
		pw->trigger((uint32_t)devent_type_e::ole_drop_e, &t);
		printf("pos\n");
	}break;
	case SDL_EVENT_DROP_COMPLETE:
	{
		ole_drop_et t = {};
		t.x = e->drop.x;
		t.y = e->drop.y;//结束
		if (pw->drop_text.size()) {
			if ('\n' == *pw->drop_text.rbegin())
				pw->drop_text.pop_back();
			auto str = pw->drop_text.data();
			t.str = (const char**)&str;
			t.count = 1;
			pw->trigger((uint32_t)devent_type_e::ole_drop_e, &t);
		}
	}break;
	case SDL_EVENT_DROP_TEXT:
	{
		if (e->drop.data)
		{
			pw->drop_text += (char*)e->drop.data;	pw->drop_text.push_back('\n');
		}
	}
	break;
	case SDL_EVENT_DROP_FILE:
	{
		if (e->drop.data)
		{
			pw->drop_text += (char*)e->drop.data;	pw->drop_text.push_back('\n');
		}
	}
	break;
	}
	return false;
}
int on_call_we(const SDL_Event* e, form_x* pw)
{
	if (!pw)return 0;
	int r = 0;
	//if (e->type == SDL_WINDOWEVENT)
	{
		//switch (e->window.event)
		switch (e->type)
		{
		case SDL_EVENT_WINDOW_DESTROYED:
		{
		}break;
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
		{
			int cbr = 0;
			if (pw->on_close_cb)
			{
				cbr = pw->on_close_cb();
			}
			if (pw->close_type || cbr == 1)
			{
				if (pw->parent)
				{
					pw->parent->remove_f(pw);
				}
				else {
					pw->destroy();
					pw->app->remove(pw);
				}
			}
			else {
				pw->hide();
				r = 1;
			}
		}
		break;
		case SDL_EVENT_WINDOW_MOUSE_ENTER:
		{
		}break;
		case SDL_EVENT_WINDOW_MOUSE_LEAVE:
		{
		}break;
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
		{
		}break;
		case SDL_EVENT_WINDOW_FOCUS_LOST:
		{
			pw->focus_lost();
		}break;
		case SDL_EVENT_WINDOW_MINIMIZED:
		{
			pw->save_size = pw->_size;
			pw->on_size({});
		}break;
		case SDL_EVENT_WINDOW_RESTORED:
		{
			pw->on_size(pw->save_size); pw->present_e();
		}break;
		case SDL_EVENT_WINDOW_RESIZED:
		{
			pw->save_size = pw->_size;
			pw->on_size({ e->window.data1,e->window.data2 });
		}break;
		default:
			on_call_emit(e, pw);
			break;
		}
	}
	return 0;
}
void form_x::on_size(const glm::ivec2& ss)
{
	if (ss != _size)
		_size = ss;
}


void form_x::destroy()
{
	if (_ptr)
	{
		SDL_DestroyWindow(_ptr); _ptr = 0;
	}
	if (renderer)
	{
		SDL_DestroyRenderer(renderer); renderer = 0;
	}
}
void show_window(SDL_Window* ptr, bool visible) {

	if (visible)
	{
		auto flags = SDL_GetWindowFlags(ptr);
		//SDL_SetHint(SDL_HINT_WINDOW_NO_ACTIVATION_WHEN_SHOWN, flags & SDL_WINDOW_TOOLTIP || flags & SDL_WINDOW_POPUP_MENU ? "1" : "0");
		SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN, (flags & SDL_WINDOW_TOOLTIP || flags & SDL_WINDOW_POPUP_MENU) ? "0" : "1");
		SDL_ShowWindow(ptr);
	}
	else
	{
		SDL_HideWindow(ptr);
	}
}
//void form_x::show_menu(mnode_t* m)
//{
//	if (m && m->child.size())
//	{
//		if (m->indep) {
//			form_x* f = new_form_popup(this, m->fsize.x, m->fsize.y);
//			f->bind(m->ui);
//			f->set_size(m->fsize);
//			f->set_pos(m->fpos);
//		}
//		else {
//			m->ui->set_pos(m->fpos);
//			bind(m->ui);
//		}
//	}
//}
void form_x::remove_f(form_x* p)
{
	if (p)
	{
		auto& v = childfs;
		v.erase(std::remove_if(v.begin(), v.end(), [p](form_x* r) {return r == p; }), v.end());
		app->remove(p);
	}
}
void form_x::clear_wt()
{
	for (int i = 0; i < 2; i++)
	{
		auto pks = _planes[i];
		for (auto it : pks)
		{
			unbind(it);
			if (it->autofree)
				delete it;
		}
	}
	_planes[0].clear();
	_planes[1].clear();
	for (size_t i = 0; i < 2; i++)
	{
		auto& tt = atlas[i];
		for (auto it : tt) {
			if (it && it->autofree)
				delete it;
		}
		tt.clear();
	}
}
size_t form_x::count_wt()
{
	return _planes[0].size() + _planes[1].size();
}
void* form_x::get_nptr()
{
	void* p = 0;
#ifdef _WIN32
	p = (HWND)pce::get_windowptr(_ptr);
#endif
	return p;
}
void form_x::push_menu(menu_cx* p)
{
	if (p)
		menus.push_back(p);
}
void form_x::update_w()
{
	int w, h;
	SDL_GetWindowSize(_ptr, &w, &h);
	auto flags = SDL_GetWindowFlags(_ptr);
	if (flags & SDL_WINDOW_MINIMIZED)
		w = h = 0;
	_size.x = w;
	_size.y = h;
	if (flags & SDL_WINDOW_MOUSE_FOCUS)
	{
		// 有鼠标焦点
	}
	int qc = qcmd_value.size();
	if (qcmd_value.size() && _ptr) {
		//printf("mm%d\n", qc);
		lock_auto_x lx(&lkqcv);
		glm::ivec2 oldsize = { w,h }, old_pos = get_pos(), ss = oldsize, ps = old_pos;
		for (; qcmd_value.size();)
		{
			auto v = qcmd_value.front(); qcmd_value.pop();
			auto t = (fcv_type)v.x;
			switch (t)
			{
			case fcv_type::e_null:
				break;
			case fcv_type::e_show:
				visible = true; visible_old = false;
				break;
			case fcv_type::e_hide:
				visible = false; visible_old = true;
				break;
			case fcv_type::e_visible_rev:
				visible = !visible_old;
				break;
			case fcv_type::e_size:
				ss = { v.y, v.z };
				break;
			case fcv_type::e_pos:
				ps = { v.y, v.z };
				break;
			default:
				break;
			}
		}
		if (oldsize != ss)
		{
			SDL_SetWindowSize(_ptr, ss.x, ss.y);
		}
		if (visible_old != visible)
		{
			visible_old = visible;
			if (visible && old_pos != ps)
				SDL_SetWindowPosition(_ptr, ps.x, ps.y);// 显示前移动
			show_window(_ptr, visible);
			if (!visible && old_pos != ps)
				SDL_SetWindowPosition(_ptr, ps.x, ps.y);// 先隐藏再移动
		}
		else {
			if (old_pos != ps)
				SDL_SetWindowPosition(_ptr, ps.x, ps.y);
		}
	}
}
void form_x::update(float delta)
{
	if (!visible)return;
	// Setup display size (every frame to accommodate for window resizing)

	int display_w, display_h;

	SDL_GetWindowSizeInPixels(_ptr, &display_w, &display_h);
	display_size = _size;
	if (_size.x > 0 && _size.y > 0)
	{
		display_framebuffer_scale = glm::vec2((float)display_w / _size.x, (float)display_h / _size.y);
	}
	else
	{
		return;
	}
	int dwt = 0;
	if (io) {
		io->DeltaTime = delta;
	}
	if (up_cb)
	{
		up_cb(delta, &dwt);
	}

	for (int i = 0; i < 2; i++) {
		for (auto it = _planes[i].begin(); it != _planes[i].end(); it++)
		{
			(*it)->update(delta);
		}
	}

	for (size_t i = 0; i < 2; i++)
	{
		auto& it = atlas[i];
		for (auto kt : it)
		{
			if (_size.x > 0 && _size.y > 0)
			{
				if (kt->viewport.z <= 0)
				{
					kt->viewport.z = display_size.x;
				}
				if (kt->viewport.w <= 0)
				{
					kt->viewport.w = display_size.y;
				}
			}
			if (kt->visible)
				canvas_atlas_update(kt, renderer, delta);
		}
	}
	for (auto kt : skeletons) {
		kt->update(delta);
	}
	app_cx::sleep_ms(dwt);
}
void form_x::draw_rects(const glm::vec4* rects, int n, const glm::vec4& color)
{
	assert(renderer);
	if (renderer && rects && n > 0)
	{
		SDL_SetRenderDrawColorFloat(renderer, color.x, color.y, color.z, color.w);  /* white, full alpha */
		SDL_RenderFillRects(renderer, (SDL_FRect*)rects, n);
	}
}
void form_x::warp_mouse_in_window(float x, float y)
{
	if (x < 0 || y < 0)
	{
		x = _size.x / 2;
		y = _size.y / 2;
	}
	SDL_WarpMouseInWindow(_ptr, x, y);
}
void form_x::set_mouse_mode(bool grab_enable, bool rmode)
{
	_rmode = rmode;
	SDL_SetWindowRelativeMouseMode(_ptr, rmode);	//设置窗口的相对鼠标模式。
	SDL_SetWindowMouseGrab(_ptr, grab_enable);		// 设置鼠标范围在窗口内
	if (rmode)warp_mouse_in_window(-1, -1);
}
// todo 图集渲染
void draw_data(SDL_Renderer* renderer, canvas_atlas* dc, int fb_width, int fb_height, const glm::vec2& render_scale, const glm::ivec2& display_size)
{
	glm::vec2 clip_off = {};
	glm::vec2 clip_scale = render_scale;
	SDL_Rect vp = { 0,0,-1,-1 };
	if (dc->viewport.z > 0 && dc->viewport.w > 0)
	{
		vp.x = dc->viewport.x;
		vp.y = dc->viewport.y;
		vp.w = dc->viewport.z;
		vp.h = dc->viewport.w;
		SDL_SetRenderViewport(renderer, &vp);
	}

	auto av = &dc->_mesh;
	auto vd = (SDL_Vertex*)av->vtxs.data();
	auto vdt = av->vtxs.data();
	auto idv = av->idxs.data();
	auto vbs = av->vtxs.size();
	auto ibs = av->idxs.size();
	std::vector<int> idxs;
	struct { SDL_Texture* texture; SDL_BlendMode blendMode; } states = {};
	for (auto& pcmd : av->cmd_data)
	{
		glm::vec2 clip_min((pcmd.clip_rect.x - clip_off.x) * clip_scale.x, (pcmd.clip_rect.y - clip_off.y) * clip_scale.y);
		glm::vec2 clip_max((pcmd.clip_rect.z - clip_off.x) * clip_scale.x, (pcmd.clip_rect.w - clip_off.y) * clip_scale.y);
		if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
		{
			SDL_SetRenderClipRect(renderer, 0);
		}
		else
		{
			SDL_Rect r = { (int)(clip_min.x), (int)(clip_min.y), (int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y) };
			SDL_SetRenderClipRect(renderer, &r);
		}
		//SDL_SetTextureBlendMode(states.texture, states.blendMode);
		auto texture = (SDL_Texture*)pcmd.texid;
#if 0
		SDL_RenderGeometry(renderer, tex, vd + pcmd.vtxOffset, pcmd.vCount, ibs ? idv + pcmd.idxOffset : nullptr, pcmd.elemCount);
#else
		auto vertices = vdt + pcmd.vtxOffset;
		const float* xy = &vertices->position.x;
		int stride = sizeof(vertex_v2);
		const SDL_FColor* color = (SDL_FColor*)&vertices->color;
		const float* uv = &vertices->tex_coord.x;
		int size_indices = 4;
		auto indices = ibs ? idv + pcmd.idxOffset : nullptr;
		auto num_indices = pcmd.elemCount;
		if ((BlendMode_e)pcmd.blend_mode != BlendMode_e::none) {
			auto blend = get_blend_x((BlendMode_e)pcmd.blend_mode, false);
			if (states.blendMode != blend || states.texture != texture) {
				states.texture = texture; states.blendMode = blend;
				SDL_SetTextureBlendMode(states.texture, states.blendMode);
			}
		}
		SDL_RenderGeometryRaw(renderer, texture, xy, stride, color, stride, uv, stride, pcmd.vCount, indices, num_indices, size_indices);

#endif
	}
	//SDL_SetRenderViewport(renderer, (SDL_Rect*)0);
}
void form_x::present(double delta)
{
	if (!visible || !renderer || !app || !app->r2d || display_size.x < 1 || display_size.y < 1)return;
	float rsx = 1.0f;
	float rsy = 1.0f;
	SDL_GetRenderScale(renderer, &rsx, &rsy);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	glm::vec2 render_scale;
	render_scale.x = (rsx == 1.0f) ? display_framebuffer_scale.x : 1.0f;
	render_scale.y = (rsy == 1.0f) ? display_framebuffer_scale.y : 1.0f;

	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	int fb_width = (int)(display_size.x * render_scale.x);
	int fb_height = (int)(display_size.y * render_scale.y);
	if (fb_width == 0 || fb_height == 0)
		return;

	SDL_Rect    viewport = { 0,0,display_size.x,display_size.y };
	SDL_SetRenderViewport(renderer, &viewport);
	SDL_SetRenderClipRect(renderer, &viewport);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
#if 1
	{
#if 1
		auto ktd = textures[0].data();
		auto length = textures[0].size();
		for (size_t i = 0; i < length; i++)
			//for (auto& it : textures[1])
		{
			auto& it = ktd[i];
			auto src = it.src.w > 0 && it.src.z > 0 ? &it.src : nullptr;
			auto dst = it.dst.w > 0 && it.dst.z > 0 ? &it.dst : nullptr;
			SDL_RenderTexture(renderer, it.tex, (SDL_FRect*)src, (SDL_FRect*)dst);
		}
#endif
	}
	if (!skeletons.empty()) {

		if (skelet_viewport.z > 0 && skelet_viewport.w > 0)
			SDL_SetRenderViewport(renderer, (SDL_Rect*)&skelet_viewport);
		if (skelet_clip.z > 0 && skelet_clip.w > 0)
			SDL_SetRenderClipRect(renderer, (SDL_Rect*)&skelet_clip);
		// 渲染骨骼动画
		for (auto kt : skeletons) {
			app->r2d->draw_data(renderer, kt);
		}
	}
	SDL_SetRenderViewport(renderer, &viewport); //恢复默认视图
	SDL_SetRenderClipRect(renderer, &viewport);
	// 渲染图集/UI
	for (int i = 0; i < 2; i++) {
		auto& it = atlas[i];
		for (auto a : it)
		{
			if (a->visible)
				draw_data(renderer, a, fb_width, fb_height, render_scale, display_size);
		}
	}

	SDL_SetRenderViewport(renderer, &viewport); //恢复默认视图
	SDL_SetRenderClipRect(renderer, &viewport);
	{
		if (render_cb)
		{
			render_cb(renderer, delta);
		}
#if 1
		auto ktd = textures[1].data();
		auto length = textures[1].size();
		for (size_t i = 0; i < length; i++)
			//for (auto& it : textures[1])
		{
			auto& it = ktd[i];
			auto src = it.src.w > 0 && it.src.z > 0 ? &it.src : nullptr;
			auto dst = it.dst.w > 0 && it.dst.z > 0 ? &it.dst : nullptr;
			if (it.scale > 0)
			{
				if (it.left_width > 0 && it.right_width > 0 && it.top_height > 0 && it.bottom_height)
				{
					SDL_RenderTexture9Grid(renderer, it.tex, (SDL_FRect*)src, it.left_width, it.right_width, it.top_height, it.bottom_height, it.scale, (SDL_FRect*)dst);
				}
				else {
					SDL_RenderTextureTiled(renderer, it.tex, (SDL_FRect*)src, it.scale, (SDL_FRect*)dst);
				}
			}
			else {
				SDL_RenderTexture(renderer, it.tex, (SDL_FRect*)src, (SDL_FRect*)dst);
			}
		}
#endif
	}
#endif
	SDL_RenderPresent(renderer);

}

void form_x::present_e()
{
	if (renderer)
	{
		printf("reset form\n");
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
	}
}


char* form_x::get_clipboard0()
{
	char* ret = 0;
	if (SDL_HasClipboardText())
	{
		ret = (char*)SDL_GetClipboardText();
		if (clipstr)
			SDL_free(clipstr);
		clipstr = ret;
	}
	return ret;
}
void form_x::sdlfree(void* p)
{
	if (p)
	{
		SDL_free(p);
	}
}
void form_x::sdlfree(void** p)
{
	if (p && *p)
	{
		SDL_free(*p); *p = 0;
	}
}
void form_x::set_clipboard(const char* str)
{
	if (str)
		SDL_SetClipboardText(str);
}

bool dragdrop_begin(const wchar_t* str, size_t size)
{
	if (size == -1)size = wcslen(str);
	return str && size ? hz::do_dragdrop_begin(str, size) : false;
}
bool form_x::do_dragdrop_begin(const wchar_t* str, size_t size)
{
	return dragdrop_begin(str, size);
}
void form_x::new_tool_tip(const glm::ivec2& pos, const void* str)
{


}

bool form_x::hittest(const glm::ivec2& pos)
{
	_HitTest = false;
	if (_ptr)
	{
		for (int i = 1; i >= 0; i--) {
			for (auto it = _planes[i].rbegin(); it != _planes[i].rend(); it++)
			{
				if ((*it)->visible && (*it)->hittest(pos))
				{
					_HitTest = true; break;
				}
			}
		}
	}
	return _HitTest;
}

void form_x::focus_lost()
{
	if (_focus_lost_hide)
	{
		hide();
	}
	else
	{
		hide_child();
	}
}

void form_x::hide_child()
{
	for (auto it : childfs) {
		if (it->_focus_lost_hide)
			it->hide();
	}
}




#if 1
SDL_Surface* SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
	return SDL_CreateSurface(width, height,
		SDL_GetPixelFormatForMasks(depth, Rmask, Gmask, Bmask, Amask));
}

SDL_Surface* SDL_CreateRGBSurfaceWithFormat(Uint32 flags, int width, int height, int depth, Uint32 format)
{
	return SDL_CreateSurface(width, height, (SDL_PixelFormat)format);
}

SDL_Surface* SDL_CreateRGBSurfaceFrom(void* pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
	return SDL_CreateSurfaceFrom(width, height, SDL_GetPixelFormatForMasks(depth, Rmask, Gmask, Bmask, Amask), pixels, pitch);
}

SDL_Surface* SDL_CreateRGBSurfaceWithFormatFrom(void* pixels, int width, int height, int depth, int pitch, Uint32 format)
{
	return SDL_CreateSurfaceFrom(width, height, (SDL_PixelFormat)format, pixels, pitch);
}
#else
SDL_Surface* SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);

SDL_Surface* SDL_CreateRGBSurfaceWithFormat(Uint32 flags, int width, int height, int depth, Uint32 format);

SDL_Surface* SDL_CreateRGBSurfaceFrom(void* pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);

SDL_Surface* SDL_CreateRGBSurfaceWithFormatFrom(void* pixels, int width, int height, int depth, int pitch, Uint32 format);

#endif

void form_x::set_icon(const char* fn)
{
	auto window = _ptr;
	SDL_Surface* icon = nullptr;
	uint32_t rmask, gmask, bmask, amask;
	//rmask = 0xFF000000; gmask = 0x00FF0000; bmask = 0x0000FF00; amask = 0x000000FF;	// RGBA8888模式
	rmask = 0x000000FF; gmask = 0x0000FF00; bmask = 0x00FF0000; amask = 0xFF000000;	// RGBA8888模式
	//rmask = 0xFF000000; gmask = 0x00FF0000; bmask = 0x0000FF00; amask = 0x00000000;	// RGB8888模式
	//加载图片文件创建一个RGB Surface
	stbimage_load img(fn);
	int depth = 32, pitch = img.width * 4;
	if (img.data)
	{
		icon = SDL_CreateRGBSurfaceFrom(img.data, img.width, img.height, depth, pitch, rmask, gmask, bmask, amask);
		if (NULL == icon) return;
		SDL_SetWindowIcon(window, icon);
		SDL_DestroySurface(icon);
	}
}

void form_x::set_icon(const uint32_t* d, int w, int h)
{
	if (!d || w < 6 || h < 6)return;
	auto window = _ptr;
	SDL_Surface* icon = nullptr;
	uint32_t rmask, gmask, bmask, amask;
	rmask = 0x000000FF; gmask = 0x0000FF00; bmask = 0x00FF0000; amask = 0xFF000000;	// RGBA8888模式
	//加载图片文件创建一个RGB Surface
	int depth = 32, pitch = w * 4;
	icon = SDL_CreateRGBSurfaceFrom((void*)d, w, h, depth, pitch, rmask, gmask, bmask, amask);
	if (NULL == icon) return;
	SDL_SetWindowIcon(window, icon);
	SDL_DestroySurface(icon);
}

void form_x::set_alpha(bool is)
{
#ifdef _WIN32
	auto hWnd = (HWND)pce::get_windowptr(_ptr);
	pce::EnableBlurBehindWindowMY(hWnd, is, 0, false, _size);
#endif
}



SDL_HitTestResult HitTestCallback2(SDL_Window* win, const SDL_Point* area, void* data)
{
	int winWidth = 0, winHeight = 0;
	SDL_GetWindowSize(win, &winWidth, &winHeight);

	const int RESIZE_AREA = 8;
	const int RESIZE_AREAC = RESIZE_AREA * 2;
	SDL_HitTestResult ret = SDL_HITTEST_NORMAL;
	auto fw = (form_x*)data;
	do
	{
		if (fw && fw->mmove_type == false)break;
		// Resize top
		if (area->x < RESIZE_AREAC && area->y < RESIZE_AREAC)
		{
			ret = SDL_HITTEST_RESIZE_TOPLEFT; break;
		}
		if (area->x > winWidth - RESIZE_AREAC && area->y < RESIZE_AREAC)
		{
			ret = SDL_HITTEST_RESIZE_TOPRIGHT; break;
		}
		if (area->x < RESIZE_AREA)
		{
			ret = SDL_HITTEST_RESIZE_LEFT; break;
		}
		if (area->y < RESIZE_AREA)
		{
			ret = SDL_HITTEST_RESIZE_TOP; break;
		}

		int tbh = 0;
		if (fw)
		{
			tbh = fw->titlebarheight;
			if (tbh > 0) {
				if (area->y < tbh && area->x < winWidth - 128)
				{
					// Title bar
					ret = SDL_HITTEST_DRAGGABLE; break;
				}
			}
			else if (!fw->hittest({ area->x,area->y }))
			{
				ret = SDL_HITTEST_DRAGGABLE; break;
			}
		}

		if (area->x < RESIZE_AREAC && area->y > winHeight - RESIZE_AREAC)
		{
			ret = SDL_HITTEST_RESIZE_BOTTOMLEFT; break;
		}
		if (area->x > winWidth - RESIZE_AREAC && area->y > winHeight - RESIZE_AREAC)
		{
			ret = SDL_HITTEST_RESIZE_BOTTOMRIGHT; break;
		}
		if (area->x > winWidth - RESIZE_AREA)
		{
			ret = SDL_HITTEST_RESIZE_RIGHT; break;
		}
		if (area->y > winHeight - RESIZE_AREA)
		{
			ret = SDL_HITTEST_RESIZE_BOTTOM; break;
		}

	} while (0);

	//printf("%d\n", ret);
	return ret;
}

void* new_texture_r(void* renderer, int width, int height, int type, void* data, int stride, int bm, bool static_tex, bool multiply)
{
	SDL_Texture* p = 0;
	if (renderer && width > 0 && height > 0)
	{
		if (type == 0)type = SDL_PIXELFORMAT_ABGR8888;//rgba
		if (type == 1)type = SDL_PIXELFORMAT_ARGB8888;//bgra
		p = SDL_CreateTexture((SDL_Renderer*)renderer, (SDL_PixelFormat)type, static_tex ? SDL_TEXTUREACCESS_STATIC : SDL_TEXTUREACCESS_STREAMING, width, height);
		if (p && data)
		{
			if (stride < 1)stride = width * 4;
			SDL_UpdateTexture(p, 0, data, stride);
		}
		// 设置texture 混合模式 
		SDL_SetTextureBlendMode(p, get_blend_x((BlendMode_e)bm, multiply));
	}
	return p;
}
void update_texture_r(void* texture, const glm::ivec4* rect, const void* pixels, int pitch)
{
	if (texture && pixels)
	{
		if (pitch < 1)pitch = ((SDL_Texture*)texture)->w * 4;
		SDL_UpdateTexture(((SDL_Texture*)texture), (SDL_Rect*)rect, pixels, pitch);
	}
}
void set_texture_blend_r(void* texture, uint32_t b, bool multiply)
{
	if (texture)
		SDL_SetTextureBlendMode(((SDL_Texture*)texture), get_blend_x((BlendMode_e)b, multiply));
}
void free_texture_r(void* texture)
{
	if (texture)
		SDL_DestroyTexture(((SDL_Texture*)texture));
}
void* new_tex2file(void* renderer, const char* fn) {
	SDL_Texture* p = nullptr;
	stbimage_load img(fn);
	int depth = 32, pitch = img.width * 4;
	if (img.data)
	{
		p = (SDL_Texture*)new_texture_r(renderer, img.width, img.height, 0, img.data, pitch, 0, true, false);
	}
	return p;
}



// 纹理渲染
int render_texture(void* renderer, void* texture, texture_dt* p, int count)
{
	if (!renderer || !p || !texture || !count)return 0;
	int ern = 0;
	for (size_t i = 0; i < count; i++, p++)
	{
		auto& srcrect = p->src_rect;
		auto& dstrect = p->dst_rect;
		bool r = SDL_RenderTexture((SDL_Renderer*)renderer, (SDL_Texture*)texture, (const SDL_FRect*)&srcrect, (const SDL_FRect*)&dstrect);
		if (r)
		{
			ern++;
		}
	}
	return ern;
}
bool render_texture_rotated(void* renderer, void* texture, texture_angle_dt* p, int count)
{
	if (!renderer || !p || !texture || count < 1)return false;
	// 源区域、目标区域、旋转角度、旋转中心、翻转模式，0=SDL_FLIP_NONE，1=SDL_FLIP_HORIZONTAL，2=SDL_FLIP_VERTICAL
	int r = 0;
	for (size_t i = 0; i < count; i++, p++)
	{
		r += SDL_RenderTextureRotated((SDL_Renderer*)renderer, (SDL_Texture*)texture,
			(const SDL_FRect*)&p->src_rect, (const SDL_FRect*)&p->dst_rect, p->angle, (const SDL_FPoint*)&p->center, (SDL_FlipMode)p->flip);
	}
	return r;
}
bool render_texture_tiled(void* renderer, void* texture, texture_tiled_dt* p, int count)
{
	if (!renderer || !p || !texture || count < 1)return false;
	int r = 0;
	// 渲染重复平铺
	for (size_t i = 0; i < count; i++, p++)
	{
		r += SDL_RenderTextureTiled((SDL_Renderer*)renderer, (SDL_Texture*)texture, (const SDL_FRect*)&p->src_rect, p->scale, (const SDL_FRect*)&p->dst_rect);
	}
	return r;
}
bool render_texture_9grid(void* renderer, void* texture, texture_9grid_dt* p, int count)
{
	if (!renderer || !p || !texture || count < 1)return false;
	// 九宫格渲染
	int r = 0;
	for (size_t i = 0; i < count; i++, p++)
	{
		r += p->tileScale > 0.0 ? SDL_RenderTexture9GridTiled((SDL_Renderer*)renderer, (SDL_Texture*)texture,
			(const SDL_FRect*)&p->src_rect, p->left_width, p->right_width, p->top_height, p->bottom_height, p->scale, (const SDL_FRect*)&p->dst_rect, p->tileScale)
			: SDL_RenderTexture9Grid((SDL_Renderer*)renderer, (SDL_Texture*)texture,
				(const SDL_FRect*)&p->src_rect, p->left_width, p->right_width, p->top_height, p->bottom_height, p->scale, (const SDL_FRect*)&p->dst_rect);
	}
	return r;

}
bool render_geometryraw(void* renderer, void* texture, geometryraw_dt* p, int count)
{
	if (!renderer || !p || !texture || !p->xy || count < 1)return false;
	// 渲染三角网，indices支持uint8_t uint16_t uint(实际最大值int_max)

	int r = 0;
	for (size_t i = 0; i < count; i++, p++)
	{
		r += SDL_RenderGeometryRaw((SDL_Renderer*)renderer,
			(SDL_Texture*)texture,
			p->xy, p->xy_stride,
			(SDL_FColor*)p->color, p->color_stride,
			p->uv, p->uv_stride,
			p->num_vertices,
			p->indices, p->num_indices, p->size_indices);
	}
	return r;
}



texture_cb get_texture_cb() {
	texture_cb cb = { new_texture_r, update_texture_r, set_texture_blend_r, free_texture_r };
	cb.make_tex = (void* (*)(void*, image_ptr_t*))newuptex;
	cb.new_texture_file = new_tex2file;
	cb.render_texture = render_texture;
	cb.render_texture_rotated = render_texture_rotated;
	cb.render_texture_tiled = render_texture_tiled;
	cb.render_texture_9grid = render_texture_9grid;
	cb.render_geometryraw = render_geometryraw;
	return cb;
}

SDL_Texture* form_x::new_texture(int width, int height, int type, void* data, int stride, int bm, bool static_tex, bool multiply)
{
	return (SDL_Texture*)new_texture_r(renderer, width, height, type, data, stride, bm, static_tex, multiply);
}
glm::ivec2 form_x::get_size()
{
	glm::ivec2 r = {};
	SDL_GetWindowSize(_ptr, &r.x, &r.y);
	return r;
}
glm::ivec2 form_x::get_pos()
{
	glm::ivec2 r = {};
	SDL_GetWindowPosition(_ptr, &r.x, &r.y);
	return r;
}
void form_x::set_size(const glm::vec2& v)
{
	lock_auto_x lx(&lkqcv);
	qcmd_value.push({ (int)fcv_type::e_size,v,0 });
}
void form_x::set_pos(const glm::vec2& v)
{
	lock_auto_x lx(&lkqcv);
	qcmd_value.push({ (int)fcv_type::e_pos,v,0 });

}

SDL_Texture* form_x::new_texture(int width, int height, void* vkptr, int format)
{
	SDL_Texture* texture = 0;
	if (renderer && width > 0 && height > 0 && vkptr)
	{
		int access = SDL_TEXTUREACCESS_STATIC;
		if (format == 0)
		{
			format = SDL_PIXELFORMAT_ABGR8888; // VK_RGBA
		}
		if (format == 1)
		{
			format = SDL_PIXELFORMAT_ARGB8888; // VK_BGRA
		}
		SDL_PropertiesID props = SDL_CreateProperties();
		SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, format);
		SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_VULKAN_TEXTURE_NUMBER, (int64_t)vkptr);
		SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, access);
		SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, width);
		SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, height);
		texture = SDL_CreateTextureWithProperties(renderer, props);
		SDL_DestroyProperties(props);
	}
	return texture;
}
SDL_Texture* form_x::new_texture(const char* fn)
{
	SDL_Texture* p = nullptr;
	stbimage_load img(fn);
	int depth = 32, pitch = img.width * 4;
	if (img.data)
	{
		p = (SDL_Texture*)new_texture_r(renderer, img.width, img.height, 0, img.data, pitch, 0, true, false);
	}
	return p;
}
void form_x::update_texture(SDL_Texture* p, void* data, glm::ivec4 rc, int stride)
{
	if (p && data)
	{
		SDL_Rect rect = { rc.x,rc.y,rc.z,rc.w };
		if (rect.w < 1)
		{
			glm::vec2 ss = {};
			SDL_GetTextureSize(p, &ss.x, &ss.y);
			rect.w = ss.x;
			rect.h = ss.y;
		}
		if (stride < 1)stride = rect.w * 4;
		SDL_UpdateTexture(p, rect.h > 1 && rect.w > 1 ? &rect : nullptr, data, stride);
	}
}
void form_x::set_texture_blend(SDL_Texture* p, uint32_t b, bool multiply)
{
	if (p)
		SDL_SetTextureBlendMode(p, get_blend_x((BlendMode_e)b, multiply));
}
void* form_x::get_texture_data(SDL_Texture* p, int* ss)
{
	SDL_Rect r = { 0,0,1280,800 };
	void* ptr = 0;
	int pitch = 0;
	auto hr = SDL_LockTexture(p, 0, &ptr, &pitch);
	std::vector<uint32_t> pd;
	pd.resize(r.w * r.h);
	if (ptr) {
		memcpy(pd.data(), ptr, pd.size() * sizeof(int));
	}
	SDL_UnlockTexture(p);
	return ptr;
}
void form_x::free_texture(SDL_Texture* p)
{
	if (p)
		SDL_DestroyTexture(p);
}
void* form_x::get_texture_vk(SDL_Texture* p)
{
	if (!p)return nullptr;
	SDL_PropertiesID props = SDL_GetTextureProperties(p);
	auto ra = (void*)SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_VULKAN_TEXTURE_NUMBER, 0);
	auto r = (void*)SDL_GetPointerProperty(props, SDL_PROP_TEXTURE_CREATE_VULKAN_TEXTURE_NUMBER, 0);
	if (r)ra = r;
	auto r1 = (void*)SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_VULKAN_TEXTURE_NUMBER, 0);
	if (r1)ra = r1;
	return ra;
}


void form_x::start_text_input()
{
	if (!SDL_TextInputActive(_ptr))
		SDL_StartTextInput(_ptr);
}
void form_x::stop_text_input()
{
	if (SDL_TextInputActive(_ptr))
		SDL_StopTextInput(_ptr);
}
bool form_x::text_input_active()
{
	return SDL_TextInputActive(_ptr);
}
void form_x::set_ime_pos(const glm::ivec4& r)
{
	do
	{
		if (!(r.w >= 0 && r.z > 0))break;
		ime_pos = r;
#ifdef _WIN320
		auto hWnd = (HWND)pce::get_windowptr(_ptr);
		if (!hWnd)break;
		HIMC hIMC = ::ImmGetContext(hWnd);
		if (hIMC)
		{
			COMPOSITIONFORM cf;
			cf.dwStyle = CFS_POINT;
			RECT rc = { 0 };
			if (r.x > 0 || r.y > 0)
			{
				rc.left = r.x;
				rc.top = r.y;
			}
			cf.rcArea.top = cf.rcArea.left = cf.rcArea.right = cf.rcArea.bottom = 0;
			cf.ptCurrentPos.x = rc.left;//输入法坐标
			cf.ptCurrentPos.y = rc.top;
			::ImmSetCompositionWindow(hIMC, &cf);
			::ImmReleaseContext(hWnd, hIMC);
		}
#else 
		SDL_Rect rect = { r.x,r.y, r.z, r.w }; //ime_pos;
		//printf("ime pos: %d,%d\n", r.x, r.y);
		SDL_SetTextInputArea(_ptr, &rect, 0);
#endif
	} while (0);

}
void form_x::enable_window(bool bEnable)
{
#ifdef _WIN32
	auto hWnd = (HWND)pce::get_windowptr(_ptr);
	EnableWindow(hWnd, bEnable);
#endif
}
void form_x::set_mouse_pos(const glm::ivec2& pos)
{
	SDL_WarpMouseInWindow(_ptr, pos.x, pos.y);
}
void form_x::set_mouse_pos_global(const glm::ivec2& pos)
{
	SDL_WarpMouseGlobal(pos.x, pos.y);
}
void form_x::show_cursor()
{
	SDL_ShowCursor();
}
void form_x::hide_cursor()
{
	SDL_HideCursor();
}
void form_x::flash_window(int opera)
{
	auto o = std::clamp((SDL_FlashOperation)opera, SDL_FlashOperation::SDL_FLASH_CANCEL, SDL_FlashOperation::SDL_FLASH_UNTIL_FOCUSED);
	if (_ptr)
	{
		SDL_FlashWindow(_ptr, o);
	}
}
#if 1
bool form_x::add_vkimage(const glm::ivec2& size, void* vkimage, const glm::vec2& pos, int type)
{
	bool ret = false;
	if (vkimage)
	{
		auto tex = new_texture(size.x, size.y, vkimage, type);// 创建SDL的bgra纹理
		if (tex)
		{
			// 添加纹理到SDL窗口渲染 
			set_texture_blend(tex, (int)BlendMode_e::normal, 0);
			push_texture(tex, { 0,0,size.x,size.y }, { pos,size }, 0);
			ret = true;
		}
	}
	return ret;
}
void form_x::push_texture(SDL_Texture* p, const glm::vec4& src, const glm::vec4& dst, int target)
{
	if (!p)
	{
		return;
	}
	target = target > 0 ? 1 : 0;
	textures[target].push_back({ p, src, dst });
}
void form_x::pop_texture(SDL_Texture* p)
{
	if (p)
	{
		auto& v = textures[0];
		v.erase(std::remove_if(v.begin(), v.end(), [p](tex_rs& r) {return r.tex == p; }), v.end());
	}
	if (p)
	{
		auto& v = textures[1];
		v.erase(std::remove_if(v.begin(), v.end(), [p](tex_rs& r) {return r.tex == p; }), v.end());
	}
}
#endif
void form_x::add_skeleton(skeleton_t* p)
{
	if (p)
		skeletons.push_back(p);
}
void form_x::add_canvas_atlas(canvas_atlas* p, int level)
{
	level = glm::clamp(level, 0, 1);
	if (p)
		atlas[level].push_back(p);
}
void form_x::remove(skeleton_t* p)
{
	auto& v = skeletons;
	v.erase(std::remove(v.begin(), v.end(), p), v.end());
}
void form_x::remove(canvas_atlas* p)
{
	for (int i = 0; i < 2; i++) {
		auto& v = atlas[i];
		v.erase(std::remove(v.begin(), v.end(), p), v.end());
	}
}

void form_x::bind(plane_cx* p, int level)
{
	if (p)
	{
		level = glm::clamp(level, 0, 1);
		if (p->form)
		{
			unbind(p);
		}
		p->form = this;
		p->set_fontctx(app->font_ctx);
		p->form_move2end = form_move2end;
		p->form_set_input_ptr = form_set_input_ptr;
		p->dragdrop_begin = dragdrop_begin;
		_planes[level].push_back(p);
		add_canvas_atlas(p, level);
	}
}
void form_x::unbind(plane_cx* p) {
	if (p)
	{
		if (p->form)
		{
			p->form->remove_event(p);
			p->form->remove(p);
			for (int i = 0; i < 2; i++) {
				auto& v = p->form->_planes[i];
				v.erase(std::remove(v.begin(), v.end(), p), v.end());
			}
			p->form = 0;
		}
	}
}

void form_x::move2end(plane_cx* p)
{
	bind(p);
}

dev_info_cx form_x::get_dev()
{
	dev_info_cx c = {};
	if (renderer)
	{
		SDL_PropertiesID pid = SDL_GetRendererProperties(renderer);
		c.inst = (void*)SDL_GetPointerProperty(pid, SDL_PROP_RENDERER_VULKAN_INSTANCE_POINTER, 0);
		c.phy = (void*)SDL_GetPointerProperty(pid, SDL_PROP_RENDERER_VULKAN_PHYSICAL_DEVICE_POINTER, 0);
		c.vkdev = (void*)SDL_GetPointerProperty(pid, SDL_PROP_RENDERER_VULKAN_DEVICE_POINTER, 0);
		c.qFamIdx = (uint32_t)SDL_GetNumberProperty(pid, SDL_PROP_RENDERER_VULKAN_GRAPHICS_QUEUE_FAMILY_INDEX_NUMBER, 0);
		//c.qFamIdx = SDL_GetNumberProperty(pid, SDL_PROP_RENDERER_VULKAN_PRESENT_QUEUE_FAMILY_INDEX_NUMBER, 0);
	}
	return c;
}

bool form_x::has_variable()
{
	dev_info_cx c = get_dev();
	if (!c.phy)return false;
	VkPhysicalDeviceFeatures fea = {};
	vkGetPhysicalDeviceFeatures((VkPhysicalDevice)c.phy, &fea);
	VkPhysicalDeviceProperties dp = {};
	vkGetPhysicalDeviceProperties((VkPhysicalDevice)c.phy, &dp);
	// 各向异性过滤
	bool anis = fea.samplerAnisotropy;
	auto maxanis = dp.limits.maxSamplerAnisotropy;
	VkPhysicalDeviceDescriptorIndexingFeatures phyIndexingFeatures = {};
	phyIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	VkPhysicalDeviceFeatures2 phyFeatures = {};
	phyFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	phyFeatures.pNext = &phyIndexingFeatures;
	vkGetPhysicalDeviceFeatures2((VkPhysicalDevice)c.phy, &phyFeatures);
	// 是否支持Bindless
	return phyIndexingFeatures.runtimeDescriptorArray && phyIndexingFeatures.descriptorBindingVariableDescriptorCount;
}

#endif

// cpu信息
#include <hwloc.h>

#ifdef _WIN32
typedef BOOL(WINAPI* LPFN_GLPI)(
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
	PDWORD);


// Helper function to count set bits in the processor mask.
DWORD CountSetBits(ULONG_PTR bitMask)
{
	DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
	DWORD bitSetCount = 0;
	ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
	DWORD i;

	for (i = 0; i <= LSHIFT; ++i)
	{
		bitSetCount += ((bitMask & bitTest) ? 1 : 0);
		bitTest /= 2;
	}

	return bitSetCount;
}
#endif


int get_cpus(cpuinfo_t* lct) {

#ifdef _WIN32
	LPFN_GLPI glpi;
	BOOL done = FALSE;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
	DWORD returnLength = 0;
	DWORD logicalProcessorCount = 0;
	DWORD numaNodeCount = 0;
	DWORD processorCoreCount = 0;
	DWORD processorPackageCount = 0;
	DWORD byteOffset = 0;
	PCACHE_DESCRIPTOR Cache;
	std::vector<char> dt;
	glpi = (LPFN_GLPI)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")),
		"GetLogicalProcessorInformation");
	if (NULL == glpi)
	{
		printf(TEXT("\nGetLogicalProcessorInformation is not supported.\n"));
		return -1;
	}
	while (!done)
	{
		DWORD rc = glpi(buffer, &returnLength);
		if (FALSE == rc)
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				dt.resize(returnLength);
				buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)dt.data();
				if (NULL == buffer)
				{
					printf(TEXT("\nError: Allocation failure\n"));
					return (2);
				}
			}
			else
			{
				printf(TEXT("\nError %d\n"), GetLastError());
				return (3);
			}
		}
		else
		{
			done = TRUE;
		}
	}

	ptr = buffer;
	glm::ivec3 lc[6] = {};
	while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength && ptr)
	{
		switch (ptr->Relationship)
		{
		case RelationProcessorCore:
		{
			lct->core_mask[processorCoreCount] = ptr->ProcessorMask;
			processorCoreCount++;
			// A hyperthreaded core supplies more than one logical processor.
			logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
		}
		break;
		case RelationNumaNode:
			// Non-NUMA systems report a single record of this type.
			numaNodeCount++;
			break;
		case RelationCache:
			// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
			Cache = &ptr->Cache;
			lc[Cache->Level].x += Cache->Size;
			lc[Cache->Level].y++;
			lc[Cache->Level].z = Cache->Size;
			break;
		case RelationProcessorPackage:
			// Logical processors share a physical package.
			processorPackageCount++;
			break;
		default:
			printf(TEXT("\nError: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n"));
			break;
		}
		byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	}

	int r = 0;
	if (lct)
	{
		//PROCESSOR_ARCHITECTURE_INTEL;
		//SYSTEM_INFO sinfo = {};
		//GetSystemInfo(&sinfo);
		//sinfo.dwProcessorType;
		lct->numaNodeCount = numaNodeCount;
		lct->processorPackageCount = processorPackageCount;
		lct->processorCoreCount = processorCoreCount;
		lct->NumLogicalCPUCores = logicalProcessorCount;
		auto pt = lct->cache;
		for (size_t i = 0; i < 6; i++)
		{
			if (lc[i].z > 0) {
				*pt = lc[i]; r++; pt++;
			}
		}
		lct->count = r;
	}
#endif
	return 0;

}

#ifdef _WIN32

static int GetCPUNameString(cpuinfo_t& pt)
{
	int nIDs = 0;
	int nExIDs = 0;

	char strCPUName[128] = { };

	std::array<int, 4> cpuInfo;
	std::vector<std::array<int, 4>> extData;

	__cpuid(cpuInfo.data(), 0);

	// Calling __cpuid with 0x80000000 as the function_id argument
	// gets the number of the highest valid extended ID.
	__cpuid(cpuInfo.data(), 0x80000000);

	nExIDs = cpuInfo[0];
	for (int i = 0x80000000; i <= nExIDs; ++i)
	{
		__cpuidex(cpuInfo.data(), i, 0);
		extData.push_back(cpuInfo);
	}

	// Interpret CPU strCPUName string if reported
	if (nExIDs >= 0x80000004)
	{
		memcpy(strCPUName, extData[2].data(), sizeof(cpuInfo));
		memcpy(strCPUName + 16, extData[3].data(), sizeof(cpuInfo));
		memcpy(strCPUName + 32, extData[4].data(), sizeof(cpuInfo));
	}

	strcpy(pt.name, strlen(strCPUName) != 0 ? strCPUName : "UNAVAILABLE");
	return strlen(pt.name);
}
#else
static int GetCPUNameString(cpuinfo_t& pt)
{
	strcpy(pt.name, "UNAVAILABLE");
	return strlen(pt.name);
}

#endif
std::string formatBytes(uint64_t bytes)
{
	const std::vector<std::string> units = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB","YB", "BB" };
	double size = static_cast<double>(bytes);
	int unitIndex = 0;
	while (size >= 1024 && unitIndex < units.size() - 1) {
		size /= 1024;
		unitIndex++;
	}
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
	return oss.str();
}

cpuinfo_t get_cpuinfo()
{
	cpuinfo_t r = {};
	hwloc_topology_t topology = {};
	hwloc_topology_init(&topology);      // 初始化拓扑结构 
	hwloc_topology_load(topology);       // 加载硬件信息 

	// 遍历所有缓存对象 
	hwloc_obj_t cache, obj;
	int levels = 0;
	std::vector<glm::i64vec2> lc;
	for (obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, 0); obj; obj = obj->parent)
	{
		int size = 0;
		if (hwloc_obj_type_is_cache(obj->type)) {
			levels++;
			lc.push_back({ obj->attr->cache.size, obj->attr->cache.linesize });
			//printf("*** L%d caches %lluKB\n", levels, obj->attr->cache.size / 1024);
		}
	}
	hwloc_topology_destroy(topology);    // 释放资源 
	levels = 0;
	auto nn = get_cpus(&r);
	for (size_t i = 0; i < 6; i++)
	{
		auto kt = r.cache[i];
		if (kt.y > 0)
		{
			levels++;
			auto a = formatBytes(kt.x);
			auto z = formatBytes(kt.z);
			printf("*** L%d caches %s\tcount %02d\tsingle %s\n", levels, a.c_str(), kt.y, z.c_str());
		}
	}
	GetCPUNameString(r);
	r.NumLogicalCPUCores = SDL_GetNumLogicalCPUCores();
	r.CPUCacheLineSize = SDL_GetCPUCacheLineSize();
	r.SystemRAM = SDL_GetSystemRAM();
	r.SIMDAlignment = SDL_GetSIMDAlignment();
	r.AltiVec = SDL_HasAltiVec();
	r.MMX = SDL_HasMMX();
	r.SSE = SDL_HasSSE();
	r.SSE2 = SDL_HasSSE2();
	r.SSE3 = SDL_HasSSE3();
	r.SSE41 = SDL_HasSSE41();
	r.SSE42 = SDL_HasSSE42();
	r.AVX = SDL_HasAVX();
	r.AVX2 = SDL_HasAVX2();
	r.AVX512F = SDL_HasAVX512F();
	r.ARMSIMD = SDL_HasARMSIMD();
	r.NEON = SDL_HasNEON();
	r.LSX = SDL_HasLSX();
	r.LASX = SDL_HasLASX();
	return r;
}

// 导出接口
uint64_t call_data(int type, void* data)
{
	auto e = (cdtype_e)type;
	uint64_t ret = 0;
	switch (e)
	{
	case cdtype_e::cpu_count:
		ret = SDL_GetNumLogicalCPUCores();// SDL_GetCPUCount();
		break;
	case cdtype_e::new_app:
	{
		auto app = new app_cx();
		ret = (uint64_t)app;
	}
	break;
	case cdtype_e::new_form:
	{
		auto p = (form_newinfo_t*)data;
		if (p && p->app)
		{
			auto app = (app_cx*)p->app;
			ret = (uint64_t)app->new_form_renderer(p->title, p->pos, p->size, p->flags, p->has_renderer, p->parent);
		}
	}
	break;
	default:
		break;
	}
	return ret;
}
int run_app(void* p, int c)
{
	auto app = (app_cx*)p;
	int r = 0;
	if (app)
	{
		r = app->run_loop(c);
	}
	return r;
}
app_cx* new_app()
{
	return new app_cx();
}
void* new_app0()
{
	return new_app();
}
void free_app(void* p)
{
	auto app = (app_cx*)p;
	if (app)delete app;
}

void form_move2end(form_x* f, plane_cx* ud) {
	if (f && ud)
	{
		f->move2end(ud);
	}
}
void form_set_input_ptr(form_x* f, void* ud) {
	if (f) { f->set_input_ptr(ud); }
}
form_x* new_form(void* app, const char* title, int width, int height, int x, int y, uint32_t flags) {

	form_x* form1 = 0;
	if (width > 0 && height > 0)
	{
		form_newinfo_t ptf = {};
		ptf.app = app; ptf.title = title;
		ptf.size = { width,height };
		ptf.has_renderer = true;
		ptf.flags = flags ? flags : ef_vulkan | ef_resizable;
		ptf.parent = 0;
		ptf.pos = { x,y };
		form1 = (form_x*)call_data((int)cdtype_e::new_form, &ptf);
	}
	return form1;
}
form_x* new_form_popup(form_x* parent, int width, int height)
{
	form_x* form1 = 0;
	if (parent && width > 0 && height > 0)
	{
		form_newinfo_t ptf = {};
		ptf.app = parent->app; ptf.title = (char*)u8"menu";
		ptf.size = { width,height };
		ptf.has_renderer = true;
		ptf.flags = ef_resizable | ef_borderless | ef_popup;
		//ptf.flags |= ef_cpu | ef_transparent;		// 透明窗口支持有问题
		//ptf.flags |= ef_gpu | ef_transparent;
		//ptf.flags |= ef_vulkan| ef_transparent;	
		ptf.flags |= ef_dx11 | ef_transparent;
		//ptf.flags |= ef_dx12;						// 不支持透明窗口
		ptf.parent = parent;
		ptf.pos = { 0,0 };
		form1 = (form_x*)call_data((int)cdtype_e::new_form, &ptf);
		if (form1) {
			/*form1->set_alpha(true);*/
			form1->mmove_type = 0;
			form1->_focus_lost_hide = true;
		}
	}
	return form1;
}
form_x* new_form_tooltip(form_x* parent, int width, int height)
{
	form_x* form1 = 0;
	if (parent && width > 0 && height > 0)
	{
		form_newinfo_t ptf = {};
		ptf.app = parent->app; ptf.title = (char*)u8"tooltip";
		ptf.size = { width,height };
		ptf.has_renderer = true;
		ptf.flags = ef_dx11 | ef_transparent | ef_borderless | ef_tooltip;//  ef_utility;
		ptf.parent = parent;
		ptf.pos = { 0,0 };
		form1 = (form_x*)call_data((int)cdtype_e::new_form, &ptf);
		if (form1) {
			/*form1->set_alpha(true);*/
			form1->mmove_type = 0;
		}
	}
	return form1;
}

// 生成单个矩形的顶点数据（6个顶点）
std::vector<SDL_Vertex> GenerateRectangleVertices(
	float x, float y,     // 左上角坐标 
	float width,          // 矩形宽度 
	float height,         // 矩形高度 
	SDL_FColor colorTL,    // 左上颜色 
	SDL_FColor colorTR,    // 右上颜色 
	SDL_FColor colorBL,    // 左下颜色 
	SDL_FColor colorBR)    // 右下颜色 
{
	// 定义顶点数组（6个顶点，两个三角形）
	std::vector<SDL_Vertex> vertices(6);

	// 顶点坐标计算 
	const float x1 = x;
	const float y1 = y;
	const float x2 = x + width;
	const float y2 = y + height;

	// 第一个三角形（左上、右上、左下）
	vertices[0] = { {x1, y1}, colorTL, {0} };
	vertices[1] = { {x2, y1}, colorTR, {0} };
	vertices[2] = { {x1, y2}, colorBL, {0} };

	// 第二个三角形（右上、左下、右下）
	vertices[3] = { {x2, y1}, colorTR, {0} };
	vertices[4] = { {x1, y2}, colorBL, {0} };
	vertices[5] = { {x2, y2}, colorBR, {0} };

	return vertices;
}

// 示例：生成多个矩形数据 
std::vector<SDL_Vertex> GenerateBatchRectangles() {
	std::vector<SDL_Vertex> batch;

	// 生成红色渐变矩形（位置：100,100 尺寸：200x150）
	SDL_FColor red = { 1, 0, 0, 1 };
	SDL_FColor red1 = { 0.1, 0, 0, 0.2 };
	auto rect1 = GenerateRectangleVertices(100, 100, 200, 150, red, red, red1, red1);
	batch.insert(batch.end(), rect1.begin(), rect1.end());

	// 生成蓝绿渐变矩形（位置：350,200 尺寸：150x200）
	SDL_FColor blue = { 0, 0, 1, 0.9 };
	SDL_FColor green = { 0, 1, 0, 0.5 };
	auto rect2 = GenerateRectangleVertices(350, 200, 150, 200, blue, green, blue, red);
	batch.insert(batch.end(), rect2.begin(), rect2.end());

	return batch;
}
void gen_rects(std::vector<glm::vec4>& _rect, std::vector<SDL_Vertex>& opt, const glm::vec4& color, const glm::vec4& color1)
{
	opt.clear();
	for (auto& it : _rect) {
		SDL_FColor* j = (SDL_FColor*)&color;
		SDL_FColor* b = (SDL_FColor*)&color1;
		auto rect2 = GenerateRectangleVertices(it.x, it.y, it.z, it.w, *j, *j, *b, *b);
		opt.insert(opt.end(), rect2.begin(), rect2.end());
	}
}

#if 0

// 源区域、目标区域
bool SDL_RenderTexture(SDL_Renderer* renderer, SDL_Texture* texture,
	const SDL_FRect* srcrect, const SDL_FRect* dstrect);
// 源区域、目标区域、旋转角度、旋转中心、翻转模式，0=SDL_FLIP_NONE，1=SDL_FLIP_HORIZONTAL，2=SDL_FLIP_VERTICAL
bool SDL_RenderTextureRotated(SDL_Renderer* renderer, SDL_Texture* texture,
	const SDL_FRect* srcrect, const SDL_FRect* dstrect, double angle, const SDL_FPoint* center, SDL_FlipMode flip);
// 渲染重复平铺
bool SDL_RenderTextureTiled(SDL_Renderer* renderer, SDL_Texture* texture,
	const SDL_FRect* srcrect, float scale, const SDL_FRect* dstrect);
// 九宫格渲染
bool SDL_RenderTexture9Grid(SDL_Renderer* renderer, SDL_Texture* texture,
	const SDL_FRect* srcrect, float left_width, float right_width, float top_height, float bottom_height, float scale, const SDL_FRect* dstrect);
// 九宫格中间平铺
bool SDL_RenderTexture9GridTiled(SDL_Renderer* renderer, SDL_Texture* texture,
	const SDL_FRect* srcrect, float left_width, float right_width, float top_height, float bottom_height, float scale, const SDL_FRect* dstrect, float tileScale);
// 渲染三角网，indices支持uint8_t uint16_t uint(实际最大值int_max)
bool SDL_RenderGeometryRaw(SDL_Renderer* renderer,
	SDL_Texture* texture,
	const float* xy, int xy_stride,
	const SDL_FColor* color, int color_stride,
	const float* uv, int uv_stride,
	int num_vertices,
	const void* indices, int num_indices, int size_indices);

bool result, use_rendergeometry = true;
struct text_vertex_t
{
	glm::vec2 pos, uv;
	glm::vec4 color;
};
std::vector<int> _indices;
std::vector<text_vertex_t> _vertexs;
SDL_Texture* texture = 0;
for (auto it : tbt->tv)
{
	if (!it._image || !it._image->texid)continue;
	auto epos = _vertexs.size();
	_vertexs.resize(epos + 4);
	auto dd = _vertexs.data() + epos;
	float minu, minv, maxu, maxv;
	float minx, miny, maxx, maxy;
	SDL_FRect srcrect = { it._rect.x, it._rect.y, it._rect.z, it._rect.w };
	SDL_FRect dstrect = { pos.x + it._apos.x + it._dwpos.x,pos.y + it._apos.y + it._dwpos.y, it._rect.z, it._rect.w };
	minu = srcrect.x / text_tex->w;
	minv = srcrect.y / text_tex->h;
	maxu = (srcrect.x + srcrect.w) / text_tex->w;
	maxv = (srcrect.y + srcrect.h) / text_tex->h;
	minx = dstrect.x;
	miny = dstrect.y;
	maxx = dstrect.x + dstrect.w;
	maxy = dstrect.y + dstrect.h;
	dd[0].pos = { minx,miny };
	dd[1].pos = { maxx ,maxy };
	dd[2].pos = { maxx ,maxy };
	dd[3].pos = { minx ,maxy };
	dd[0].uv = { minu  ,minv };
	dd[1].uv = { maxu  ,minv };
	dd[2].uv = { maxu ,maxv };
	dd[3].uv = { minu , maxv };
}
auto v = _vertexs.data();
const float* posxy = &v->pos.x;
const SDL_FColor* color = (SDL_FColor*)&v->color;
const float* uv = &v->uv.x;
int size_indices = 4;
int stride = sizeof(text_vertex_t);
auto num_vertices = _vertexs.size();
SDL_RenderGeometryRaw(renderer, text_tex, posxy, stride, color, stride, uv, stride, num_vertices, _indices.data(), _indices.size(), size_indices);


#endif // 0




#if 1

extern SDL_DECLSPEC bool SDLCALL SDL_GPUSupportsShaderFormats(SDL_GPUShaderFormat format_flags, const char* name);
extern SDL_DECLSPEC bool SDLCALL SDL_GPUSupportsProperties(SDL_PropertiesID props);
extern SDL_DECLSPEC SDL_GPUDevice* SDLCALL SDL_CreateGPUDevice(SDL_GPUShaderFormat format_flags, bool debug_mode, const char* name);
extern SDL_DECLSPEC SDL_GPUDevice* SDLCALL SDL_CreateGPUDeviceWithProperties(SDL_PropertiesID props);
extern SDL_DECLSPEC void SDLCALL SDL_DestroyGPUDevice(SDL_GPUDevice * device);
extern SDL_DECLSPEC int SDLCALL SDL_GetNumGPUDrivers(void);
extern SDL_DECLSPEC const char* SDLCALL SDL_GetGPUDriver(int index);
extern SDL_DECLSPEC const char* SDLCALL SDL_GetGPUDeviceDriver(SDL_GPUDevice * device);
extern SDL_DECLSPEC SDL_GPUShaderFormat SDLCALL SDL_GetGPUShaderFormats(SDL_GPUDevice * device);
extern SDL_DECLSPEC SDL_PropertiesID SDLCALL SDL_GetGPUDeviceProperties(SDL_GPUDevice * device);

extern SDL_DECLSPEC SDL_GPUComputePipeline* SDLCALL SDL_CreateGPUComputePipeline(SDL_GPUDevice * device, const SDL_GPUComputePipelineCreateInfo * createinfo);
extern SDL_DECLSPEC SDL_GPUGraphicsPipeline* SDLCALL SDL_CreateGPUGraphicsPipeline(SDL_GPUDevice * device, const SDL_GPUGraphicsPipelineCreateInfo * createinfo);
extern SDL_DECLSPEC SDL_GPUSampler* SDLCALL SDL_CreateGPUSampler(SDL_GPUDevice * device, const SDL_GPUSamplerCreateInfo * createinfo);
extern SDL_DECLSPEC SDL_GPUShader* SDLCALL SDL_CreateGPUShader(SDL_GPUDevice * device, const SDL_GPUShaderCreateInfo * createinfo);
extern SDL_DECLSPEC SDL_GPUTexture* SDLCALL SDL_CreateGPUTexture(SDL_GPUDevice * device, const SDL_GPUTextureCreateInfo * createinfo);
extern SDL_DECLSPEC SDL_GPUBuffer* SDLCALL SDL_CreateGPUBuffer(SDL_GPUDevice * device, const SDL_GPUBufferCreateInfo * createinfo);
extern SDL_DECLSPEC SDL_GPUTransferBuffer* SDLCALL SDL_CreateGPUTransferBuffer(SDL_GPUDevice * device, const SDL_GPUTransferBufferCreateInfo * createinfo);
extern SDL_DECLSPEC void SDLCALL SDL_SetGPUBufferName(SDL_GPUDevice * device, SDL_GPUBuffer * buffer, const char* text);
extern SDL_DECLSPEC void SDLCALL SDL_SetGPUTextureName(SDL_GPUDevice * device, SDL_GPUTexture * texture, const char* text);
extern SDL_DECLSPEC void SDLCALL SDL_InsertGPUDebugLabel(SDL_GPUCommandBuffer * command_buffer, const char* text);
extern SDL_DECLSPEC void SDLCALL SDL_PushGPUDebugGroup(SDL_GPUCommandBuffer * command_buffer, const char* name);
extern SDL_DECLSPEC void SDLCALL SDL_PopGPUDebugGroup(SDL_GPUCommandBuffer * command_buffer);
extern SDL_DECLSPEC void SDLCALL SDL_ReleaseGPUTexture(SDL_GPUDevice * device, SDL_GPUTexture * texture);
extern SDL_DECLSPEC void SDLCALL SDL_ReleaseGPUSampler(SDL_GPUDevice * device, SDL_GPUSampler * sampler);
extern SDL_DECLSPEC void SDLCALL SDL_ReleaseGPUBuffer(SDL_GPUDevice * device, SDL_GPUBuffer * buffer);
extern SDL_DECLSPEC void SDLCALL SDL_ReleaseGPUTransferBuffer(SDL_GPUDevice * device, SDL_GPUTransferBuffer * transfer_buffer);
extern SDL_DECLSPEC void SDLCALL SDL_ReleaseGPUComputePipeline(SDL_GPUDevice * device, SDL_GPUComputePipeline * compute_pipeline);
extern SDL_DECLSPEC void SDLCALL SDL_ReleaseGPUShader(SDL_GPUDevice * device, SDL_GPUShader * shader);
extern SDL_DECLSPEC void SDLCALL SDL_ReleaseGPUGraphicsPipeline(SDL_GPUDevice * device, SDL_GPUGraphicsPipeline * graphics_pipeline);
extern SDL_DECLSPEC SDL_GPUCommandBuffer* SDLCALL SDL_AcquireGPUCommandBuffer(SDL_GPUDevice * device);
extern SDL_DECLSPEC void SDLCALL SDL_PushGPUVertexUniformData(SDL_GPUCommandBuffer * command_buffer, Uint32 slot_index, const void* data, Uint32 length);
extern SDL_DECLSPEC void SDLCALL SDL_PushGPUFragmentUniformData(SDL_GPUCommandBuffer * command_buffer, Uint32 slot_index, const void* data, Uint32 length);
extern SDL_DECLSPEC void SDLCALL SDL_PushGPUComputeUniformData(SDL_GPUCommandBuffer * command_buffer, Uint32 slot_index, const void* data, Uint32 length);
extern SDL_DECLSPEC SDL_GPURenderPass* SDLCALL SDL_BeginGPURenderPass(SDL_GPUCommandBuffer * command_buffer, const SDL_GPUColorTargetInfo * color_target_infos, Uint32 num_color_targets, const SDL_GPUDepthStencilTargetInfo * depth_stencil_target_info);
extern SDL_DECLSPEC void SDLCALL SDL_BindGPUGraphicsPipeline(SDL_GPURenderPass * render_pass, SDL_GPUGraphicsPipeline * graphics_pipeline);
extern SDL_DECLSPEC void SDLCALL SDL_SetGPUViewport(SDL_GPURenderPass * render_pass, const SDL_GPUViewport * viewport);
extern SDL_DECLSPEC void SDLCALL SDL_SetGPUScissor(SDL_GPURenderPass * render_pass, const SDL_Rect * scissor);
extern SDL_DECLSPEC void SDLCALL SDL_SetGPUBlendConstants(SDL_GPURenderPass * render_pass, SDL_FColor blend_constants);
extern SDL_DECLSPEC void SDLCALL SDL_SetGPUStencilReference(SDL_GPURenderPass * render_pass, Uint8 reference);
extern SDL_DECLSPEC void SDLCALL SDL_BindGPUVertexBuffers(SDL_GPURenderPass * render_pass, Uint32 first_slot, const SDL_GPUBufferBinding * bindings, Uint32 num_bindings);
extern SDL_DECLSPEC void SDLCALL SDL_BindGPUIndexBuffer(SDL_GPURenderPass * render_pass, const SDL_GPUBufferBinding * binding, SDL_GPUIndexElementSize index_element_size);
extern SDL_DECLSPEC void SDLCALL SDL_BindGPUVertexSamplers(SDL_GPURenderPass * render_pass, Uint32 first_slot, const SDL_GPUTextureSamplerBinding * texture_sampler_bindings, Uint32 num_bindings);
extern SDL_DECLSPEC void SDLCALL SDL_BindGPUVertexStorageTextures(SDL_GPURenderPass * render_pass, Uint32 first_slot, SDL_GPUTexture* const* storage_textures, Uint32 num_bindings);
extern SDL_DECLSPEC void SDLCALL SDL_BindGPUVertexStorageBuffers(SDL_GPURenderPass * render_pass, Uint32 first_slot, SDL_GPUBuffer* const* storage_buffers, Uint32 num_bindings);
extern SDL_DECLSPEC void SDLCALL SDL_BindGPUFragmentSamplers(SDL_GPURenderPass * render_pass, Uint32 first_slot, const SDL_GPUTextureSamplerBinding * texture_sampler_bindings, Uint32 num_bindings);
extern SDL_DECLSPEC void SDLCALL SDL_BindGPUFragmentStorageTextures(SDL_GPURenderPass * render_pass, Uint32 first_slot, SDL_GPUTexture* const* storage_textures, Uint32 num_bindings);
extern SDL_DECLSPEC void SDLCALL SDL_BindGPUFragmentStorageBuffers(SDL_GPURenderPass * render_pass, Uint32 first_slot, SDL_GPUBuffer* const* storage_buffers, Uint32 num_bindings);
extern SDL_DECLSPEC void SDLCALL SDL_DrawGPUIndexedPrimitives(SDL_GPURenderPass * render_pass, Uint32 num_indices, Uint32 num_instances, Uint32 first_index, Sint32 vertex_offset, Uint32 first_instance);
extern SDL_DECLSPEC void SDLCALL SDL_DrawGPUPrimitives(SDL_GPURenderPass * render_pass, Uint32 num_vertices, Uint32 num_instances, Uint32 first_vertex, Uint32 first_instance);
extern SDL_DECLSPEC void SDLCALL SDL_DrawGPUPrimitivesIndirect(SDL_GPURenderPass * render_pass, SDL_GPUBuffer * buffer, Uint32 offset, Uint32 draw_count);
extern SDL_DECLSPEC void SDLCALL SDL_DrawGPUIndexedPrimitivesIndirect(SDL_GPURenderPass * render_pass, SDL_GPUBuffer * buffer, Uint32 offset, Uint32 draw_count);
extern SDL_DECLSPEC void SDLCALL SDL_EndGPURenderPass(SDL_GPURenderPass * render_pass);
extern SDL_DECLSPEC SDL_GPUComputePass* SDLCALL SDL_BeginGPUComputePass(SDL_GPUCommandBuffer * command_buffer, const SDL_GPUStorageTextureReadWriteBinding * storage_texture_bindings, Uint32 num_storage_texture_bindings, const SDL_GPUStorageBufferReadWriteBinding * storage_buffer_bindings, Uint32 num_storage_buffer_bindings);
extern SDL_DECLSPEC void SDLCALL SDL_BindGPUComputePipeline(SDL_GPUComputePass * compute_pass, SDL_GPUComputePipeline * compute_pipeline);
extern SDL_DECLSPEC void SDLCALL SDL_BindGPUComputeSamplers(SDL_GPUComputePass * compute_pass, Uint32 first_slot, const SDL_GPUTextureSamplerBinding * texture_sampler_bindings, Uint32 num_bindings);
extern SDL_DECLSPEC void SDLCALL SDL_BindGPUComputeStorageTextures(SDL_GPUComputePass * compute_pass, Uint32 first_slot, SDL_GPUTexture* const* storage_textures, Uint32 num_bindings);
extern SDL_DECLSPEC void SDLCALL SDL_BindGPUComputeStorageBuffers(SDL_GPUComputePass * compute_pass, Uint32 first_slot, SDL_GPUBuffer* const* storage_buffers, Uint32 num_bindings);
extern SDL_DECLSPEC void SDLCALL SDL_DispatchGPUCompute(SDL_GPUComputePass * compute_pass, Uint32 groupcount_x, Uint32 groupcount_y, Uint32 groupcount_z);
extern SDL_DECLSPEC void SDLCALL SDL_DispatchGPUComputeIndirect(SDL_GPUComputePass * compute_pass, SDL_GPUBuffer * buffer, Uint32 offset);
extern SDL_DECLSPEC void SDLCALL SDL_EndGPUComputePass(SDL_GPUComputePass * compute_pass);
extern SDL_DECLSPEC void* SDLCALL SDL_MapGPUTransferBuffer(SDL_GPUDevice * device, SDL_GPUTransferBuffer * transfer_buffer, bool cycle);
extern SDL_DECLSPEC void SDLCALL SDL_UnmapGPUTransferBuffer(SDL_GPUDevice * device, SDL_GPUTransferBuffer * transfer_buffer);
extern SDL_DECLSPEC SDL_GPUCopyPass* SDLCALL SDL_BeginGPUCopyPass(SDL_GPUCommandBuffer * command_buffer);
extern SDL_DECLSPEC void SDLCALL SDL_UploadToGPUTexture(SDL_GPUCopyPass * copy_pass, const SDL_GPUTextureTransferInfo * source, const SDL_GPUTextureRegion * destination, bool cycle);
extern SDL_DECLSPEC void SDLCALL SDL_UploadToGPUBuffer(SDL_GPUCopyPass * copy_pass, const SDL_GPUTransferBufferLocation * source, const SDL_GPUBufferRegion * destination, bool cycle);
extern SDL_DECLSPEC void SDLCALL SDL_CopyGPUTextureToTexture(SDL_GPUCopyPass * copy_pass, const SDL_GPUTextureLocation * source, const SDL_GPUTextureLocation * destination, Uint32 w, Uint32 h, Uint32 d, bool cycle);
extern SDL_DECLSPEC void SDLCALL SDL_CopyGPUBufferToBuffer(SDL_GPUCopyPass * copy_pass, const SDL_GPUBufferLocation * source, const SDL_GPUBufferLocation * destination, Uint32 size, bool cycle);
extern SDL_DECLSPEC void SDLCALL SDL_DownloadFromGPUTexture(SDL_GPUCopyPass * copy_pass, const SDL_GPUTextureRegion * source, const SDL_GPUTextureTransferInfo * destination);
extern SDL_DECLSPEC void SDLCALL SDL_DownloadFromGPUBuffer(SDL_GPUCopyPass * copy_pass, const SDL_GPUBufferRegion * source, const SDL_GPUTransferBufferLocation * destination);
extern SDL_DECLSPEC void SDLCALL SDL_EndGPUCopyPass(SDL_GPUCopyPass * copy_pass);
extern SDL_DECLSPEC void SDLCALL SDL_GenerateMipmapsForGPUTexture(SDL_GPUCommandBuffer * command_buffer, SDL_GPUTexture * texture);
extern SDL_DECLSPEC void SDLCALL SDL_BlitGPUTexture(SDL_GPUCommandBuffer * command_buffer, const SDL_GPUBlitInfo * info);

extern SDL_DECLSPEC bool SDLCALL SDL_WindowSupportsGPUSwapchainComposition(SDL_GPUDevice * device, SDL_Window * window, SDL_GPUSwapchainComposition swapchain_composition);
extern SDL_DECLSPEC bool SDLCALL SDL_WindowSupportsGPUPresentMode(SDL_GPUDevice * device, SDL_Window * window, SDL_GPUPresentMode present_mode);
extern SDL_DECLSPEC bool SDLCALL SDL_ClaimWindowForGPUDevice(SDL_GPUDevice * device, SDL_Window * window);
extern SDL_DECLSPEC void SDLCALL SDL_ReleaseWindowFromGPUDevice(SDL_GPUDevice * device, SDL_Window * window);
extern SDL_DECLSPEC bool SDLCALL SDL_SetGPUSwapchainParameters(SDL_GPUDevice * device, SDL_Window * window, SDL_GPUSwapchainComposition swapchain_composition, SDL_GPUPresentMode present_mode);
extern SDL_DECLSPEC bool SDLCALL SDL_SetGPUAllowedFramesInFlight(SDL_GPUDevice * device, Uint32 allowed_frames_in_flight);
extern SDL_DECLSPEC SDL_GPUTextureFormat SDLCALL SDL_GetGPUSwapchainTextureFormat(SDL_GPUDevice * device, SDL_Window * window);
extern SDL_DECLSPEC bool SDLCALL SDL_AcquireGPUSwapchainTexture(SDL_GPUCommandBuffer * command_buffer, SDL_Window * window, SDL_GPUTexture * *swapchain_texture, Uint32 * swapchain_texture_width, Uint32 * swapchain_texture_height);
extern SDL_DECLSPEC bool SDLCALL SDL_WaitForGPUSwapchain(SDL_GPUDevice * device, SDL_Window * window);
extern SDL_DECLSPEC bool SDLCALL SDL_WaitAndAcquireGPUSwapchainTexture(SDL_GPUCommandBuffer * command_buffer, SDL_Window * window, SDL_GPUTexture * *swapchain_texture, Uint32 * swapchain_texture_width, Uint32 * swapchain_texture_height);

extern SDL_DECLSPEC bool SDLCALL SDL_SubmitGPUCommandBuffer(SDL_GPUCommandBuffer * command_buffer);
extern SDL_DECLSPEC SDL_GPUFence* SDLCALL SDL_SubmitGPUCommandBufferAndAcquireFence(SDL_GPUCommandBuffer * command_buffer);
extern SDL_DECLSPEC bool SDLCALL SDL_CancelGPUCommandBuffer(SDL_GPUCommandBuffer * command_buffer);
extern SDL_DECLSPEC bool SDLCALL SDL_WaitForGPUIdle(SDL_GPUDevice * device);
extern SDL_DECLSPEC bool SDLCALL SDL_WaitForGPUFences(SDL_GPUDevice * device, bool wait_all, SDL_GPUFence* const* fences, Uint32 num_fences);
extern SDL_DECLSPEC bool SDLCALL SDL_QueryGPUFence(SDL_GPUDevice * device, SDL_GPUFence * fence);
extern SDL_DECLSPEC void SDLCALL SDL_ReleaseGPUFence(SDL_GPUDevice * device, SDL_GPUFence * fence);
extern SDL_DECLSPEC Uint32 SDLCALL SDL_GPUTextureFormatTexelBlockSize(SDL_GPUTextureFormat format);
extern SDL_DECLSPEC bool SDLCALL SDL_GPUTextureSupportsFormat(SDL_GPUDevice * device, SDL_GPUTextureFormat format, SDL_GPUTextureType type, SDL_GPUTextureUsageFlags usage);
extern SDL_DECLSPEC bool SDLCALL SDL_GPUTextureSupportsSampleCount(SDL_GPUDevice * device, SDL_GPUTextureFormat format, SDL_GPUSampleCount sample_count);
extern SDL_DECLSPEC Uint32 SDLCALL SDL_CalculateGPUTextureFormatSize(SDL_GPUTextureFormat format, Uint32 width, Uint32 height, Uint32 depth_or_layer_count);

#ifdef SDL_PLATFORM_GDK 
extern SDL_DECLSPEC void SDLCALL SDL_GDKSuspendGPU(SDL_GPUDevice * device);
extern SDL_DECLSPEC void SDLCALL SDL_GDKResumeGPU(SDL_GPUDevice * device);

#endif /* SDL_PLATFORM_GDK */
#endif // 1

struct gpu_propertie_t
{
	SDL_GPUShaderFormat format_flags = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXBC | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_METALLIB;
	const char* name = 0;
	const char* dx12_semantic = 0;		// 用于所有顶点语义的前缀，默认为 “TEXCOORD”。
	bool debug_mode = true;			// 启用调试模式，默认为 true
	bool verbose = true;			// 启用此选项可在设备创建时自动记录有用的调试信息，默认为 true
	bool preferlowpower = false;	// 首选低功率，默认为 false
	bool vk_shaderclipdistance = true;	// 启用设备功能 shaderClipDistance。如果禁用，则着色器代码中不支持裁剪距离：GLSL 的 gl_ClipDistance[] 内置、HLSL 的 SV_ClipDistance0/1 语义和 Metal 的 [[clip_distance]] 属性。默认为 true。
	bool vk_depthclamp = true;			// 启用设备功能 depthClamp。如果禁用，则不支持深度限制，并且 SDL_GPURasterizerState 中的 enable_depth_clip 必须始终设置为 true。默认为 true。
	bool vk_drawindirectfirstinstance = true;//启用设备功能 drawIndirectFirstInstance。如果禁用，则必须将 SDL_GPUIndirectDrawCommand 的参数 first_instance 设置为零。默认为 true。
	bool vk_sampleranisotropy = true;	// 启用设备功能 samplerAnisotropy。如果禁用，则必须将 enable_anisotropy of SDL_GPUSamplerCreateInfo 设置为 false。默认为 true。
};
// 创建GPU设备
SDL_GPUDevice* new_gpu_device(gpu_propertie_t * pt)
{
#ifndef SDL_GPU_DISABLED
	SDL_PropertiesID props = SDL_CreateProperties();
	if (pt->format_flags & SDL_GPU_SHADERFORMAT_PRIVATE) {
		SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_PRIVATE_BOOLEAN, true);
	}
	if (pt->format_flags & SDL_GPU_SHADERFORMAT_SPIRV) {
		SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
		if (!pt->vk_shaderclipdistance)
			SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_SHADERCLIPDISTANCE_BOOLEAN, false);
		if (!pt->vk_depthclamp)
			SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_DEPTHCLAMP_BOOLEAN, false);
		if (!pt->vk_drawindirectfirstinstance)
			SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_DRAWINDIRECTFIRST_BOOLEAN, false);
		if (!pt->vk_sampleranisotropy)
			SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_SAMPLERANISOTROPY_BOOLEAN, false);
	}
	if (pt->format_flags & SDL_GPU_SHADERFORMAT_DXBC) {
		SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXBC_BOOLEAN, true);
	}
	if (pt->format_flags & SDL_GPU_SHADERFORMAT_DXIL) {
		SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);
	}
	if (pt->dx12_semantic && (pt->format_flags & SDL_GPU_SHADERFORMAT_DXBC || pt->format_flags & SDL_GPU_SHADERFORMAT_DXIL))
		SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_D3D12_SEMANTIC_NAME_STRING, pt->dx12_semantic);
	if (pt->format_flags & SDL_GPU_SHADERFORMAT_MSL) {
		SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true);
	}
	if (pt->format_flags & SDL_GPU_SHADERFORMAT_METALLIB) {
		SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_METALLIB_BOOLEAN, true);
	}
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, pt->debug_mode);
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VERBOSE_BOOLEAN, pt->verbose);
	if (pt->preferlowpower)
		SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_PREFERLOWPOWER_BOOLEAN, true);
	SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, pt->name);
	SDL_GPUDevice* p = 0;
	if (SDL_GPUSupportsProperties(props))
	{
		p = SDL_CreateGPUDeviceWithProperties(props);
	}
	SDL_DestroyProperties(props);
	return p;
#else
	SDL_SetError("SDL not built with GPU support");
	return NULL;
#endif // SDL_GPU_DISABLED
}
// 获取支持的GPU驱动名称列表
std::vector<const char*> get_gpudrivers()
{
	int ng = SDL_GetNumGPUDrivers();
	std::vector<const char*> gpunames;
	for (size_t i = 0; i < ng; i++)
	{
		const char* a = SDL_GetGPUDriver(i);
		if (a)
			gpunames.push_back(a);
	}
	return gpunames;
}
class gpu_device_cx
{
public:
	SDL_GPUDevice* dev = 0;
	std::string _dx12_semantic;
public:
	gpu_device_cx();
	~gpu_device_cx();
	bool init_gpu(bool preferlowpower = false, bool is_dx12 = false, const char* dx12_semantic = 0);
	// 获取设备名称，要求在init_gpu之后调用
	const char* get_devname();
	// 获取支持的着色器格式
	uint32_t get_shaderformat();
private:

};

gpu_device_cx::gpu_device_cx()
{
}

gpu_device_cx::~gpu_device_cx()
{
	if (dev)
		SDL_DestroyGPUDevice(dev);
	dev = 0;
}

bool gpu_device_cx::init_gpu(bool preferlowpower, bool is_dx12, const char* dx12_semantic)
{
	bool bs[] = { SDL_GPUSupportsShaderFormats(SDL_GPU_SHADERFORMAT_SPIRV, "vulkan"),
	SDL_GPUSupportsShaderFormats(SDL_GPU_SHADERFORMAT_DXBC | SDL_GPU_SHADERFORMAT_DXIL, "direct3d12"),
	SDL_GPUSupportsShaderFormats(SDL_GPU_SHADERFORMAT_METALLIB | SDL_GPU_SHADERFORMAT_MSL, "metal")
	};
	gpu_propertie_t pt = {};
	pt.preferlowpower = preferlowpower;
#ifndef _DEBUG
	pt.debug_mode = false;
#endif // !_DEBUG
	pt.format_flags = 0;
	if (is_dx12 && bs[1])
	{
		if (dx12_semantic && *dx12_semantic)
		{
			_dx12_semantic = dx12_semantic;
			pt.dx12_semantic = _dx12_semantic.c_str();
		}
		pt.format_flags = SDL_GPU_SHADERFORMAT_DXBC | SDL_GPU_SHADERFORMAT_DXIL;
	}
	else
	{
		if (bs[0])
		{
			pt.format_flags |= SDL_GPU_SHADERFORMAT_SPIRV;
		}
		if (bs[2])
		{
			pt.format_flags |= SDL_GPU_SHADERFORMAT_METALLIB | SDL_GPU_SHADERFORMAT_MSL;
		}
	}
	dev = new_gpu_device(&pt);
	return dev ? true : false;
}

const char* gpu_device_cx::get_devname()
{
	return dev ? SDL_GetGPUDeviceDriver(dev) : nullptr;
}

uint32_t gpu_device_cx::get_shaderformat()
{
	return dev ? SDL_GetGPUShaderFormats(dev) : 0;
}

// todo 测试文本
#if 1

enum TTF_ImageType
{
	TTF_IMAGE_INVALID,
	TTF_IMAGE_ALPHA,    /**< The color channels are white */
	TTF_IMAGE_COLOR,    /**< The color channels have image data */
	TTF_IMAGE_SDF,      /**< The alpha channel has signed distance field information */
};
struct AtlasDrawSequence
{
	SDL_Texture* texture;
	TTF_ImageType image_type;
	int num_rects;
	glm::vec4* rects;
	glm::vec2* texcoords;
	glm::vec2* positions;
	int* indices;
};
enum TTF_Direction
{
	TTF_DIRECTION_INVALID = 0,
	TTF_DIRECTION_LTR = 4,        /**< Left to Right */
	TTF_DIRECTION_RTL,            /**< Right to Left */
	TTF_DIRECTION_TTB,            /**< Top to Bottom */
	TTF_DIRECTION_BTT             /**< Bottom to Top */
};
struct TTF_TextLayout
{
	int direction;
	Uint32 script;
	int font_height;
	int wrap_length;
	bool wrap_whitespace_visible;
	int* lines;
};
struct TTF_TextData
{
	font_t* font;             /**< The font used by this text, read-only. */
	glm::vec4 color;           /**< The color of the text, read-only. */

	bool needs_layout_update;   /**< True if the layout needs to be updated */
	TTF_TextLayout* layout;     /**< Cached layout information, read-only. */
	int x;                      /**< The x offset of the upper left corner of this text, in pixels, read-only. */
	int y;                      /**< The y offset of the upper left corner of this text, in pixels, read-only. */
	int w;                      /**< The width of this text, in pixels, read-only. */
	int h;                      /**< The height of this text, in pixels, read-only. */
	int num_ops;                /**< The number of drawing operations to render this text, read-only. */
	//TTF_DrawOperation* ops;     /**< The drawing operations used to render this text, read-only. */
	//int num_clusters;           /**< The number of substrings representing clusters of glyphs in the string, read-only */
	//TTF_SubString* clusters;    /**< Substrings representing clusters of glyphs in the string, read-only */

	//SDL_PropertiesID props;     /**< Custom properties associated with this text, read-only. This field is created as-needed using TTF_GetTextProperties() and the properties may be then set and read normally */

	bool needs_engine_update;   /**< True if the engine text needs to be updated */
	SDL_Renderer* renderer;
	AtlasDrawSequence* draw_sequence;
	int num_sequence;
};
struct ttf_text
{
	char* text;             /**< A copy of the UTF-8 string that this text object represents, useful for layout, debugging and retrieving substring text. This is updated when the text object is modified and will be freed automatically when the object is destroyed. */
	int num_lines;          /**< The number of lines in the text, 0 if it's empty */

	int refcount;           /**< Application reference count, used when freeing surface */

	TTF_TextData* internal; /**< Private */

};
void new_sequence(AtlasDrawSequence * sequence, int count)
{
	static const uint8_t rect_index_order[] = { 0, 1, 2, 0, 2, 3 };
	int vertex_index = 0;
	int* indices = sequence->indices;
	for (int i = 0; i < count; ++i) {
		*indices++ = vertex_index + rect_index_order[0];
		*indices++ = vertex_index + rect_index_order[1];
		*indices++ = vertex_index + rect_index_order[2];
		*indices++ = vertex_index + rect_index_order[3];
		*indices++ = vertex_index + rect_index_order[4];
		*indices++ = vertex_index + rect_index_order[5];
		vertex_index += 4;
	}
}

bool ttf_updatetext(ttf_text * text)
{
	if (!text || !text->internal) {
		return false;
	}
	// Update the internal text data
	TTF_TextData* internal = text->internal;
	// Check if the text needs to be updated
	if (internal->needs_engine_update) {
		// Perform the update logic here, such as updating the layout, drawing operations, etc.
		internal->needs_engine_update = false; // Reset the flag after updating
	}
	return true;
}
bool draw_renderer_text(ttf_text * text, float x, float y)
{
	if (!text || !text->internal/* || text->internal->engine->CreateText != CreateText*/) {
		return false;
	}

	// Make sure the text is up to date
	if (!ttf_updatetext(text)) {
		return false;
	}


	SDL_Renderer* renderer = text->internal->renderer;
	AtlasDrawSequence* sequence = text->internal->draw_sequence;
	for (int m = 0; m < text->internal->num_sequence; m++, sequence++)
	{
		glm::vec2* position = sequence->positions;
		for (int i = 0; i < sequence->num_rects; ++i) {
			auto dst = &sequence->rects[i];
			float minx = x + dst->x;
			float maxx = x + dst->x + dst->z;// 宽
			float miny = y + dst->y;
			float maxy = y + dst->y + dst->w;// 高

			*position++ = glm::vec2(minx, miny);
			*position++ = glm::vec2(maxx, miny);
			*position++ = glm::vec2(maxx, maxy);
			*position++ = glm::vec2(minx, maxy);
		}

		SDL_FColor color = {};
		if (sequence->image_type == TTF_IMAGE_ALPHA) {
			SDL_copyp(&color, &text->internal->color);
		}
		else {
			// Don't alter the color data in the image
			color.r = 1.0f;
			color.g = 1.0f;
			color.b = 1.0f;
			color.a = text->internal->color.w;
		}

		SDL_RenderGeometryRaw(renderer,
			sequence->texture,
			(float*)sequence->positions, 2 * sizeof(float),
			&color, 0,
			(float*)sequence->texcoords, 2 * sizeof(float),
			sequence->num_rects * 4,
			sequence->indices, sequence->num_rects * 6, sizeof(*sequence->indices));
	}
	return true;
}
#endif
// !1

