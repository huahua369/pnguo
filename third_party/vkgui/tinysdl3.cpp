
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
	HRESULT EnableBlurBehindWindowMY(HWND window, bool enable = true, HRGN region = 0, bool transitionOnMaximized = false)
	{
		BOOL pfe = 0;
		auto hr = DwmIsCompositionEnabled(&pfe);
		if (!pfe)
		{
			DwmEnableComposition(enable ? DWM_EC_ENABLECOMPOSITION : DWM_EC_DISABLECOMPOSITION);
		}
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
			region = CreateRectRgn(0, 0, -1, -1);
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
	{}

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
	int kr = SDL_Init(0);
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
}

app_cx::~app_cx()
{
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
	if (derender) {
		std::string rn;
		auto rdc = SDL_GetHint(SDL_HINT_RENDER_DRIVER);
		//if (rdv.find("gpu") != rdv.end())
		//{
		//	rn = "gpu";
		//}
		if (rdv.find("vulkan") != rdv.end())
		{
			rn = "vulkan";
		}
		else if (rdv.find("direct3d12") != rdv.end())
		{
			rn = "direct3d12";
		}
		else {
			rn = *rdv.begin();
		}
		renderer = SDL_CreateRenderer(window, rn.empty() ? 0 : rn.c_str());
	}
	int vsync = 0;
	if (renderer) {
		SDL_GetRenderVSync(renderer, &vsync);
		SDL_SetRenderVSync(renderer, 0);
	}
	auto pw = new form_x();
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
	if (fw) {
		rw = on_call_we(e, fw);
	}
}
void app_cx::sleep_ms(int ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
void app_cx::set_fps(int n) {
	if (n > 0)
	{
		_fps = n; fms = 1000.0 / n;
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
		break;
	}
#endif
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

int app_cx::run_loop(int t)
{
	int t1 = t;
	do {
		fct->restart();
		uint32_t curr_time = SDL_GetTicks();
		if (prev_time > 0)
		{
			float delta = (float)(curr_time - prev_time) / 1000.0f;
			auto length = forms.size();
			for (size_t i = 0; i < length; i++)
			{
				auto it = forms[i];
				it->update(delta);
			}
			for (auto it : forms)
			{
				it->present();
			}
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
	for (size_t i = 0; i < skeleton->slots_size; ++i) {
		slot_t* slot = skeleton->solt_data + i;
		auto attachment = slot->attachment;
		// 如果附件插槽颜色为0或骨骼未处于活动状态，则提前退出
		if (slot->color.w == 0 || !slot->bone->count || !attachment || attachment->color.w == 0) { continue; }
		texture = (SDL_Texture*)attachment->tex;
		auto c = skeleton->color * slot->color * attachment->color;
		*(glm::vec4*)(&vertex.color) = glm::clamp(c, 0.0f, 1.0f);
		auto blend = get_blend((BlendMode_e)slot->blend_mode, skeleton->usePremultipliedAlpha);
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
				vertex.color = *((SDL_FColor*)&attachment->colors[i]);
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
		if (renderer && !ptx)
		{
			auto ty = img->type;
			if (ty == 0) ty = SDL_PIXELFORMAT_ABGR8888;
			if (ty == 1) ty = SDL_PIXELFORMAT_ARGB8888;
			ptx = SDL_CreateTexture(renderer, (SDL_PixelFormat)ty, img->static_tex ? SDL_TEXTUREACCESS_STATIC : SDL_TEXTUREACCESS_STREAMING, img->width, img->height);
			img->texid = ptx;
			img->valid = true;
			pr = ptx;
			// 设置texture 混合模式
			//SDL_SetTextureBlendMode(p, SDL_BLENDMODE_BLEND);
			SDL_SetTextureBlendMode(ptx, get_blend((BlendMode_e)img->blendmode, img->multiply));
		}
		if (img->data && img->valid)
		{
			//print_time a("SDL_UpdateTexture");
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
			}
		}
		p->valid = true;
		p->_renderer = renderer;
		p->destroy_texture_cb = (void(*)(void* tex))SDL_DestroyTexture;
	}
	for (auto it : p->_atlas_t)
	{
		auto ptx = newuptex(renderer, it->img);
		if (ptx)
			p->_texs_t.push_back(ptx);
	}
	for (auto it : p->_atlas_cx)
	{
		auto ptx = newuptex(renderer, it->img);
		if (ptx)
			p->_texs_t.push_back(ptx);
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
	if (!ud)return;
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
	do
	{
		et.form = this;
		et.v.b = (mouse_button_et*)e;
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
					if (et.ret)
					{
						break;
					}
				}
			}
			if (et.ret)
			{
				break;
			}
		}
		for (int i = 1; i >= 0; i--) {
			auto pt = _planes[i];
			for (auto it = pt.rbegin(); it != pt.rend(); it++)
			{
				(*it)->on_event((uint32_t)type, &et);
				if (et.ret)
				{
					break;
				}
			}
			if (et.ret)
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
	ekm->state = e->key.down;        /**< ::SDL_PRESSED or ::SDL_RELEASED */
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
		t.state = e->button.down; //SDL_PRESSED; SDL_RELEASED;
		t.clicks = e->button.clicks;
		t.x = e->button.x;
		t.y = e->button.y;
		if (t.state)
		{
			pw->hide_child();
		}
		//pw->hittest({ t.x,t.y });
		if (pw->io && !pw->_HitTest) {
			pw->io->MouseDown[t.button - 1] = t.state;
		}
		else {
			pw->io->MouseDown[t.button - 1] = 0;
		}
		pw->trigger((uint32_t)devent_type_e::mouse_button_e, &t);

		if (pw && pw->capture_type)
		{
			if (t.state)
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
		if (t.kmod & KM_SHIFT)
		{
			//printf("shift\n");
		}
		pw->trigger((uint32_t)devent_type_e::keyboard_e, &t);
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
		case SDL_EVENT_WINDOW_DESTROYED: {
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
			pw->on_size(pw->save_size);
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
void* form_x::get_nptr()
{
	void* p = 0;
#ifdef _WIN32
	p = (HWND)pce::get_windowptr(_ptr);
#endif
	return p;
}
void form_x::update_w()
{
	int w, h;
	SDL_GetWindowSize(_ptr, &w, &h);
	if (SDL_GetWindowFlags(_ptr) & SDL_WINDOW_MINIMIZED)
		w = h = 0;
	_size.x = w;
	_size.y = h;
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
	//SDL_Delay(dwt);
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
	auto idv = av->idxs.data();
	auto vbs = av->vtxs.size();
	auto ibs = av->idxs.size();
	std::vector<int> idxs;
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
		auto tex = (SDL_Texture*)pcmd.texid;
		SDL_RenderGeometry(renderer, tex, vd + pcmd.vtxOffset, pcmd.vCount, ibs ? idv + pcmd.idxOffset : nullptr, pcmd.elemCount);
	}
	//SDL_SetRenderViewport(renderer, (SDL_Rect*)0);
}
void form_x::present()
{
	if (!visible || !renderer || !app || !app->r2d || display_size.x < 1 || display_size.y < 1)return;
	float rsx = 1.0f;
	float rsy = 1.0f;
	SDL_GetRenderScale(renderer, &rsx, &rsy);
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

	{
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
	}
	{

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
		auto ktd = textures[1].data();
		auto length = textures[1].size();
		for (size_t i = 0; i < length; i++)
			//for (auto& it : textures[1])
		{
			auto& it = ktd[i];
			auto src = it.src.w > 0 && it.src.z > 0 ? &it.src : nullptr;
			auto dst = it.dst.w > 0 && it.dst.z > 0 ? &it.dst : nullptr;
			SDL_RenderTexture(renderer, it.tex, (SDL_FRect*)src, (SDL_FRect*)dst);
		}
	}
	SDL_RenderPresent(renderer);
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
	pce::EnableBlurBehindWindowMY(hWnd, is);
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

SDL_Texture* form_x::new_texture(int width, int height, int type, void* data, int stride, int bm, bool static_tex, bool multiply)
{
	SDL_Texture* p = 0;
	if (renderer && width > 0 && height > 0)
	{
		if (type == 0)type = SDL_PIXELFORMAT_ABGR8888;//rgba
		if (type == 1)type = SDL_PIXELFORMAT_ARGB8888;//bgra
		p = SDL_CreateTexture(renderer, (SDL_PixelFormat)type, static_tex ? SDL_TEXTUREACCESS_STATIC : SDL_TEXTUREACCESS_STREAMING, width, height);
		if (p && data)
		{
			if (stride < 1)stride = width * 4;
			SDL_UpdateTexture(p, 0, data, stride);
		}
		// 设置texture 混合模式 
		SDL_SetTextureBlendMode(p, get_blend((BlendMode_e)bm, multiply));
	}
	return p;
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
		if (stride < 1)stride = rc.z * 4;
		SDL_UpdateTexture(p, rect.h > 1 && rect.w > 1 ? &rect : nullptr, data, stride);
	}
}
void form_x::set_texture_blend(SDL_Texture* p, uint32_t b, bool multiply)
{
	if (p)
		SDL_SetTextureBlendMode(p, get_blend((BlendMode_e)b, multiply));
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

int form_x::add_vkimage(const glm::ivec2& size, void* vkimageptr, const glm::vec2& pos, int type)
{
	int ret = -1;
	if (vkimageptr)
	{
		auto tex = new_texture(size.x, size.y, vkimageptr, type);// 创建SDL的bgra纹理
		if (tex)
		{
			// 添加纹理到SDL窗口渲染 
			set_texture_blend(tex, (int)BlendMode_e::normal, 0);
			push_texture(tex, { 0,0,size.x,size.y }, { pos,size }, 0);
			ret = 0;
		}
	}
	return ret;
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
		c.qFamIdx = SDL_GetNumberProperty(pid, SDL_PROP_RENDERER_VULKAN_GRAPHICS_QUEUE_FAMILY_INDEX_NUMBER, 0);
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
		ptf.flags = ef_vulkan | ef_resizable | ef_borderless | ef_popup;
		ptf.parent = parent;
		ptf.pos = { 0,0 };
		form1 = (form_x*)call_data((int)cdtype_e::new_form, &ptf);
		if (form1) {
			form1->set_alpha(true);
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
		ptf.flags = ef_vulkan | ef_transparent | ef_borderless | ef_tooltip;//  ef_utility;
		ptf.parent = parent;
		ptf.pos = { 0,0 };
		form1 = (form_x*)call_data((int)cdtype_e::new_form, &ptf);
		if (form1) {
			form1->set_alpha(true);
			form1->mmove_type = 0;
		}
	}
	return form1;
}
