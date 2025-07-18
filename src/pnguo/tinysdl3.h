﻿/*
	Copyright (c) 华仔
	188665600@qq.com

	本文件的实现SDL封装，管理实例、窗口、菜单组件、信号量
*/
#pragma once
#include <string>
#include <set>
#include <mutex>
#include <queue>
#ifndef SDL_render_h_
#ifdef __cplusplus
extern "C" {
#endif
	typedef struct SDL_Window SDL_Window;
	typedef struct SDL_Renderer SDL_Renderer;
	typedef struct SDL_Texture SDL_Texture;
	typedef struct SDL_Semaphore SDL_Semaphore;
	typedef struct SDL_Cursor SDL_Cursor;
#ifndef SDL_events_h_
	typedef union SDL_Event SDL_Event;
#endif // !SDL_events_h_  
	typedef struct SDL_Vertex
	{
		glm::vec2 position;        /**< Vertex position, in SDL_Renderer coordinates  */
		glm::vec4 color;           /**< Vertex color */
		glm::vec2 tex_coord;       /**< Normalized texture coordinates, if needed */
	} SDL_Vertex;
#ifdef __cplusplus
}
#endif
#endif

#ifndef BIT_INC
#define BIT_INC(x) (1<<x)
#endif


// todo 创建atlas_t、canvas_atlas、skeleton_t

class sem_s
{
public:
	SDL_Semaphore* sem = 0;
public:
	sem_s();
	~sem_s();
	void post();
	int wait(int ms);
	// 返回0则有信号，1无信号
	int wait_try();
private:

};
#ifndef DEV_INFO_CXH
#define DEV_INFO_CXH
struct dev_info_cx
{
	void* inst;
	void* phy;
	void* vkdev;
	uint32_t qFamIdx;		// familyIndex
	uint32_t qIndex = 0;
};
#endif // !DEV_INFO_CXH
struct et_un_t;
struct image_ptr_t;
class font_rctx;
class render_2d;
class plane_cx;
class menu_cx;
struct mnode_t;
class form_x;
class Timer;
// 管理实例：管理窗口等资源
class app_cx
{
public:
	std::set<std::string> rdv;
	std::vector<form_x*> forms;		// 窗口列表
	render_2d* r2d = 0;				// 2d动画渲染
	font_rctx* font_ctx = 0;
	SDL_Cursor** system_cursor = 0;	// 系统光标
	Timer* fct = {};
	std::queue<form_x*> reforms;
	double crtms = 0.0;
	uint32_t prev_time = 0;
	int _fps = 60;
	int c_fps = 60;
	int fms = 0;

	uint32_t audio_device = 0;

	bool nc_down = 0;
public:
	app_cx();
	~app_cx();

	form_x* new_form_renderer(const std::string& title, const glm::ivec2& pos, const glm::ivec2& ws1, int fgs, bool derender, form_x* parent);
public:
	int run_loop(int t);
	void call_cb(SDL_Event* e);
	// 睡眠毫秒
	static void sleep_ms(int ms);
	// 睡眠纳秒
	static void sleep_ns(int ns);
	static uint64_t get_ticks();
	void set_fps(int n);
	void set_syscursor(int type);
	void set_defcursor(uint32_t t);
	void remove(form_x* f);
	void clearf();

	glm::vec3 get_power_info();
	const char* get_power_str();
	// 隐藏子窗口
	void kncdown();
	// 音频播放
	uint32_t open_audio(int format, int channels, int freq);
	void close_audio(uint32_t dev);
	uint32_t get_audio_device();
	// 音频	format	0=S16, 1=S32, 2=F32
	void* new_audio_stream0(int format, int channels, int freq);
	static void* new_audio_stream(uint32_t dev, int format, int channels, int freq);
	static void free_audio_stream(void* st);
	static void bindaudio(uint32_t dev, void* st);
	static void unbindaudio(void* st);
	static void unbindaudios(void** st, int count);
	static int get_audio_stream_queued(void* st);
	static int get_audio_dst_framesize(void* st);
	static int get_audio_stream_available(void* st);
	static void put_audio(void* stream, void* data, int len);
	// v=0播放、1暂停
	static void pause_audio(void* st_, int v);
	// format：0=S16 , 1=S32 ,2=F32，volume取0-1
	static bool mix_audio(uint8_t* dst, uint8_t* src, int format, size_t len, float volume);
	static void clear_audio(void* st);
private:
	int get_event();
	void render(double delta);
};

class skeleton_t;
class canvas_atlas;
struct input_state_t;
struct mouse_state_t;
namespace hz {
	class drop_regs;
	class drop_info_cx;
}

struct event_fw {
	void* ptr = 0;
	std::function<void(uint32_t type, et_un_t* e, void* ud)> cb = nullptr;
};
enum class fcv_type {
	e_null,
	e_show,
	e_hide,
	e_visible_rev,
	e_size,
	e_pos
};

class form_x
{
public:
	SDL_Window* _ptr = 0;
	app_cx* app = 0;				// 应用ctx
	glm::ivec2 _size = {};			// 窗口大小
	glm::ivec2 display_size = {};	// 窗口显示大小
	glm::ivec2 save_size = {};	// 保存窗口大小
	glm::vec2 display_framebuffer_scale = {};
	SDL_Renderer* renderer = 0;
	std::function<void(float delta, int* ret)> up_cb;	// 更新动画等
	std::function<void(SDL_Renderer* renderer, double delta)> render_cb;	// 更新和渲染
	std::function<int()> on_close_cb;		// 关闭事件

	std::vector<skeleton_t*> skeletons;		// 2D动画渲染列表
	std::vector<canvas_atlas*> atlas[2];		// 图集渲染列表		简单贴图或ui用
	std::vector<plane_cx*> _planes[2];		// 0是背景，1是顶层
#if 0
	struct tex_rs {
		SDL_Texture* tex = 0;
		glm::vec4 src = {}, dst = {};
		float scale = 0.0f;					// Tiled、9Grid
		float left_width = 0.0f, right_width = 0.0f, top_height = 0.0f, bottom_height = 0.0f;
	};
	std::vector<tex_rs> textures[2];		// 纹理渲染列表,0是背景，1是前景
#endif
	glm::ivec4 skelet_viewport = {};
	glm::ivec4 skelet_clip = {};

	std::string title;
	char* clipstr = 0;
	input_state_t* input_ptr = 0;	// 接收输入法的对象
	glm::ivec4 ime_pos = {};
	void* activate_ptr = 0;	// 激活的对象
	mouse_state_t* io = 0;
	std::vector<event_fw>* events_a = 0;
	std::vector<event_fw> first_cs;	// 优先
	std::vector<form_x*> childfs;
	std::vector<menu_cx*> menus;
	form_x* parent = 0;
	form_x* tooltip = 0;	// 提示窗口
	// 接收拖动OLE管理
	hz::drop_regs* dragdrop = 0;
	// 默认接收ole
	hz::drop_info_cx* _oledrop = 0;
	std::string drop_text;
	int _dx = -1, _dy = -1;
	void* uptr = 0;						// 用户设置指针
	std::queue<glm::ivec4> qcmd_value;	// 操作列表
	std::mutex lkecb, lkqcv;
	// 标题栏高度
	int titlebarheight = 0;
	// 锁定鼠标
	bool capture_type = true;
	bool close_type = true;		// 关闭按钮风格：true关闭退出，false则隐藏窗口
	bool mmove_type = true;		// 鼠标拖动
	bool _HitTest = true;
	bool _ref = false;
	bool _focus_lost_hide = false;	// 失去焦点隐藏
private:
	bool visible = true;
	bool visible_old = true;
public:
	form_x();
	~form_x();
	void init_dragdrop();
	void set_curr_drop(hz::drop_info_cx* p);
	void add_event(void* ud, std::function<void(uint32_t type, et_un_t* e, void* ud)> cb);
	size_t remove_event(void* ud);
	void move2end_e(void* ud);
	void trigger(uint32_t etype, void* e);
	void set_input_ptr(void* ud);				// 设置接收输入法的对象指针 
	void set_capture();
	void release_capture();
	void on_size(const glm::ivec2& ss);
	//销毁窗口
	void destroy();
	// 关闭窗口
	void close();
	// 显示/隐藏窗口
	void show();
	void hide();
	void show_reverse();
	// 置顶窗口
	void raise();
	bool get_visible();
	// 开始输入法
	void start_text_input();
	void stop_text_input();
	bool text_input_active();
	// 设置输入法坐标
	void set_ime_pos(const glm::ivec4& r);
	// 禁用窗口鼠标键盘操作。模态窗口用
	void enable_window(bool bEnable);
	// 移动鼠标到窗口指定位置
	void set_mouse_pos(const glm::ivec2& pos);
	void set_mouse_pos_global(const glm::ivec2& pos);
	// 显示/隐藏鼠标
	void show_cursor();
	void hide_cursor();
	void flash_window(int opera);
	// 设置窗口图标
	void set_icon(const char* fn);
	void set_icon(const uint32_t* d, int w, int h);
	void set_alpha(bool is);
	// type:0==RGBA。 static_tex? SDL_TEXTUREACCESS_STATIC : SDL_TEXTUREACCESS_STREAMING
	SDL_Texture* new_texture(int width, int height, int type, void* data, int stride, int bm = 0, bool static_tex = false, bool multiply = false);
	//  int format:0=RGBA,1=BGRA
	SDL_Texture* new_texture(int width, int height, void* vkptr, int format);
	SDL_Texture* new_texture(const char* fn);

	void update_texture(SDL_Texture* p, void* data, glm::ivec4 rc, int stride);
	void set_texture_blend(SDL_Texture* p, uint32_t b, bool multiply = false);
	void* get_texture_data(SDL_Texture* p, int* ss);
	void free_texture(SDL_Texture* p);
	// 获取纹理vk image
	void* get_texture_vk(SDL_Texture* p);

#if 0
	// 添加纹理渲染。target = 0背景层，1上层
	void push_texture(SDL_Texture* p, const glm::vec4& src, const glm::vec4& dst, int target);
	// 弹出纹理渲染
	void pop_texture(SDL_Texture* p);
	bool add_vkimage(const glm::ivec2& size, void* vkimageptr, const glm::vec2& pos, int type);
#endif
	// 添加动画、图集渲染
	void add_skeleton(skeleton_t* p);
	void add_canvas_atlas(canvas_atlas* p, int level = 0);
	void remove(skeleton_t* p);
	void remove(canvas_atlas* p);
	// 绑定面板组件
	void bind(plane_cx* p, int level = 0);
	void unbind(plane_cx* p);
	void move2end(plane_cx* p);
	dev_info_cx get_dev();
	glm::ivec2 get_size();
	glm::ivec2 get_pos();
	void set_size(const glm::vec2& v);
	void set_pos(const glm::vec2& pos);
	// 是否支持Bindless
	bool has_variable();

	void remove_f(form_x* c);

	size_t count_wt();
	// 清空控件
	void clear_wt();
	// 获取hwnd
	void* get_nptr();
	// 增加菜单管理器
	void push_menu(menu_cx* p);
	void draw_rects(const glm::vec4* rects, int n, const glm::vec4& color);
public:
	void update_w();
	void update(float delta);
	void present(double delta);
	void present_e();

	// 获取粘贴板文本
	char* get_clipboard0();
	// 释放SDL申请的内存
	void sdlfree(void* p);
	void sdlfree(void** p);
	// 设置粘贴板文本
	void set_clipboard(const char* str);
	bool do_dragdrop_begin(const wchar_t* str, size_t size);
	void new_tool_tip(const glm::ivec2& pos, const void* str);
	// 返回是否命中ui
	bool hittest(const glm::ivec2& pos);
	void focus_lost();
	void hide_child();
private:
};

struct texture_dt
{
	// 源区域、目标区域 
	glm::vec4 src_rect = {};
	glm::vec4 dst_rect = {};
};
struct texture_angle_dt
{
	// 源区域、目标区域 
	glm::vec4 src_rect = {};
	glm::vec4 dst_rect = {};
	glm::vec2 center = {};	//旋转中心
	double angle = 0;		//旋转角度
	int flip = 0;			//翻转模式，0=SDL_FLIP_NONE，1=SDL_FLIP_HORIZONTAL，2=SDL_FLIP_VERTICAL
};
// 渲染重复平铺
struct texture_tiled_dt
{
	glm::vec4 src_rect = {};
	glm::vec4 dst_rect = {};
	float scale = 1.0;
};
// 九宫格渲染
struct texture_9grid_dt
{
	glm::vec4 src_rect = {};
	glm::vec4 dst_rect = {};
	float scale = 1.0;
	float left_width = 1.0, right_width = 1.0, top_height = 1.0, bottom_height = 1.0;
	float tileScale = -1; // 大于0则是9gridtiled中间平铺
};

// 渲染三角网，indices支持uint8_t uint16_t uint(实际最大值int_max)
struct geometryraw_dt
{
	const float* xy; int xy_stride;
	const glm::vec4* color; int color_stride;// color_stride等于0时，用一个颜色
	const float* uv; int uv_stride;
	int num_vertices;
	const void* indices; int num_indices; int size_indices;
};
#ifndef TEX_CB
#define TEX_CB
struct texture_cb
{
	// 创建纹理
	void* (*new_texture)(void* renderer, int width, int height, int type, void* data, int stride, int bm, bool static_tex, bool multiply);
	// 更新纹理数据
	void (*update_texture)(void* texture, const glm::ivec4* rect, const void* pixels, int pitch);
	// 设置纹理混合模式
	void (*set_texture_blend)(void* texture, uint32_t b, bool multiply);
	// 删除纹理
	void (*free_texture)(void* texture);
	// 创建或更新纹理
	void* (*make_tex)(void* renderer, image_ptr_t* img);
	// 从图片文件创建纹理
	void* (*new_texture_file)(void* renderer, const char* fn);
	// 纹理渲染
	// 批量区域渲染
	int (*render_texture)(void* renderer, void* texture, texture_dt* p, int count);
	// 单个区域支持旋转
	bool (*render_texture_rotated)(void* renderer, void* texture, texture_angle_dt* p, int count);
	// 平铺渲染
	bool (*render_texture_tiled)(void* renderer, void* texture, texture_tiled_dt* p, int count);
	// 九宫格渲染
	bool (*render_texture_9grid)(void* renderer, void* texture, texture_9grid_dt* p, int count);
	// 渲染2d三角网,支持顶点色、纹理
	bool (*render_geometryraw)(void* renderer, void* texture, geometryraw_dt* p, int count);
};
#else
typedef struct texture_cb texture_cb;
#endif

texture_cb get_texture_cb();

void form_move2end(form_x* f, plane_cx* ud);
// 设置接收输入的控件
void form_set_input_ptr(form_x* f, void* ud);
// OLO拖放文本
bool dragdrop_begin(const wchar_t* str, size_t size);

// 获取粘贴板文本
std::string get_clipboard();
// 设置粘贴板文本
void set_clipboard(const char* str);
// 控制台使用utf8
void set_col_u8();

// 窗口属性
enum form_flags_e
{
	ef_null = 0,					// ef_default
	ef_fullscreen = BIT_INC(0),		// 全屏
	ef_utility = BIT_INC(1),		// 不出现在任务栏
	ef_resizable = BIT_INC(2),		// 可以拉伸大小
	ef_transparent = BIT_INC(3),	// 透明窗口
	ef_borderless = BIT_INC(4),		// 无系统边框
	ef_popup = BIT_INC(5),			// 弹出式窗口，需要有父窗口
	ef_tooltip = BIT_INC(6),		// 工具提示窗口，需要有父窗口
	ef_cpu = BIT_INC(7),			// cpu software渲染
	ef_vulkan = BIT_INC(8),			// vk渲染
	ef_gpu = BIT_INC(9),			// gpu渲染
	ef_metal = BIT_INC(10),			// mac metal渲染
	ef_dx11 = BIT_INC(11),
	ef_dx12 = BIT_INC(12),
	ef_vsync = BIT_INC(13),
	ef_default = ef_resizable | ef_vulkan
};
// 创建窗口的信息
struct form_newinfo_t {
	void* app = 0;
	form_x* parent = 0;
	const char* title = 0;
	glm::ivec2 pos;
	glm::ivec2 size;
	int flags = 0;
	bool has_renderer = 0;
};


// 命令列表
enum class cdtype_e :uint32_t
{
	cpu_count = 1,	//获取CPU数量
	new_app,		//创建应用实例
	new_form,		//创建窗口
};


struct cpuinfo_t
{
	char name[128];
	uint64_t core_mask[64];
	glm::ivec3 cache[6];		// x缓存总字节、y缓存数量、z单个缓存字节
	int count;					// L缓存数
	int numaNodeCount;			// NUMA节点数
	int processorPackageCount;	// 物理处理器封装数量
	int processorCoreCount;		// 内核
	int NumLogicalCPUCores;		// 逻辑处理器
	int CPUCacheLineSize;
	int SystemRAM;
	size_t SIMDAlignment;
	bool AltiVec;
	bool MMX;
	bool SSE;
	bool SSE2;
	bool SSE3;
	bool SSE41;
	bool SSE42;
	bool AVX;
	bool AVX2;
	bool AVX512F;
	bool ARMSIMD;
	bool NEON;
	bool LSX;
	bool LASX;
};

// 获取CPU信息
cpuinfo_t get_cpuinfo();
std::string formatBytes(uint64_t bytes);

app_cx* new_app();

// 导出接口 
// 运行消息循环count==0则死循环, 执行循环次数
MC_EXPORT int run_app(void* app, int count);
MC_EXPORT void* new_app0();
MC_EXPORT void free_app(void* app);
// 创建主窗口，-1默认坐标
form_x* new_form(void* app, const char* title, int width, int height, int x, int y, uint32_t flags);
// 菜单窗口
form_x* new_form_popup(form_x* parent, int width, int height);
// 提示窗口
form_x* new_form_tooltip(form_x* parent, int width, int height);

void gen_rects(std::vector<glm::vec4>& _rect, std::vector<SDL_Vertex>& opt, const glm::vec4& color, const glm::vec4& color1);

