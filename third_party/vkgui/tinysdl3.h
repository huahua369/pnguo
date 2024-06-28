#pragma once
#include <string>
#include <set>
#include <mutex>
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
#ifdef __cplusplus
}
#endif
#define ADBIT(x) (1<<x)
struct image_raw_x
{
	int w = 0, h = 0;
	int pitch = 0;		// 1行的字节宽度
	int format = 0;		// 0=rgba,1=bgra
	uint32_t* data = 0;
};

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

struct dev_info_cx
{
	void* inst;
	void* phy;
	void* vkdev;
	uint32_t qFamIdx;		// familyIndex
	uint32_t qIndex = 0;
};
struct et_un_t;
class font_rctx;
class render_2d;
class plane_cx;
class form_x;
class Timer;
// 管理窗口等资源
class app_cx
{
public:
	std::set<std::string> rdv;
	std::vector<form_x*> forms;		// 窗口列表
	render_2d* r2d = 0;				// 2d动画渲染
	font_rctx* font_ctx = 0;
	SDL_Cursor** system_cursor = 0;	// 系统光标
	Timer* fct = {};

	double crtms = 0.0;
	uint32_t prev_time = 0;
	int _fps = 60;

public:
	app_cx();
	~app_cx();

	//form_x* new_form_renderer(const std::string& title, const glm::ivec2& ws, int flags, bool derender);
	form_x* new_form_renderer(const std::string& title, const glm::ivec2& pos, const glm::ivec2& ws1, int fgs, bool derender, form_x* parent);
public:
	int run_loop(int t);
	void call_cb(SDL_Event* e);
	static void sleep_ms(int ms);
	void set_fps(int n);
	void set_syscursor(int type);
	void set_defcursor(uint32_t t);
	void remove(form_x* f);
private:
	void get_event();

};

class skeleton_t;
class canvas_atlas;
struct input_state_t;
namespace hz {
	class drop_regs;
	class drop_info_cx;
}

struct event_fw {
	void* ptr = 0;
	std::function<void(uint32_t type, et_un_t* e, void* ud)> cb = nullptr;
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
	std::function<int()> on_close_cb;		// 关闭事件

	std::vector<skeleton_t*> skeletons;		// 2D动画渲染列表
	std::vector<canvas_atlas*> atlas;		// 图集渲染列表		简单贴图或ui用
	std::vector<plane_cx*> _planes;
	struct tex_rs {
		SDL_Texture* tex = 0;
		glm::vec4 src = {}, dst = {};
	};
	std::vector<tex_rs> textures[2];		// 纹理渲染列表,0是背景，1是前景
	glm::ivec4 skelet_viewport = {};
	glm::ivec4 skelet_clip = {};

	std::string title;
	char* clipstr = 0;
	input_state_t* input_ptr = 0;	// 接收输入法的对象
	void* activate_ptr = 0;	// 激活的对象
	std::vector<event_fw>* events = 0;
	std::vector<event_fw>* events_a = 0;
	std::vector<event_fw> first_cs;	// 优先
	std::vector<form_x*> childfs;
	// 接收拖动OLE管理
	hz::drop_regs* dragdrop = 0;
	// 默认接收ole
	hz::drop_info_cx* _oledrop = 0;
	int _dx = -1, _dy = -1;

	std::mutex lkecb;
	// 标题栏高度
	int titlebarheight = 22;
	// 锁定鼠标
	bool capture_type = true;
	bool close_type = true;		// 关闭按钮风格：true关闭退出，false则隐藏窗口
	bool _HitTest = true;
public:
	form_x();
	~form_x();
	void init_dragdrop();
	void set_curr_drop(hz::drop_info_cx* p);
	void add_event(uint32_t type, void* ud, std::function<void(uint32_t type, et_un_t* e, void* ud)> cb);
	void add_event(void* ud, std::function<void(uint32_t type, et_un_t* e, void* ud)> cb);
	size_t remove_event(uint32_t type, void* ud);
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

	// 设置窗口图标
	void set_icon(const char* fn);
	void set_icon(const uint32_t* d, int w, int h);
	// 设置透明窗口
	void set_alpha(bool is);
	// type:0==RGBA。 static_tex? SDL_TEXTUREACCESS_STATIC : SDL_TEXTUREACCESS_STREAMING
	SDL_Texture* new_texture(int width, int height, int type, void* data, int stride, int bm = 0, bool static_tex = false, bool multiply = false);
	//  int format:0=RGBA,1=BGRA
	SDL_Texture* new_texture(int width, int height, void* vkptr, int format);
	SDL_Texture* new_texture(image_raw_x* p, bool multiply = false);

	void update_texture(SDL_Texture* p, void* data, glm::ivec4 rc, int stride);
	void set_texture_blend(SDL_Texture* p, uint32_t b, bool multiply = false);
	void free_texture(SDL_Texture* p);
	// 获取纹理vk image
	void* get_texture_vk(SDL_Texture* p);
	// 添加纹理渲染。target = 0背景层，1上层
	void push_texture(SDL_Texture* p, const glm::vec4& src, const glm::vec4& dst, int target);
	// 弹出纹理渲染
	void pop_texture(SDL_Texture* p);

	// 添加动画、图集渲染
	void add_skeleton(skeleton_t* p);
	void add_canvas_atlas(canvas_atlas* p);
	void remove(skeleton_t* p);
	void remove(canvas_atlas* p);
	// 绑定面板组件
	void bind(plane_cx* p);
	void unbind(plane_cx* p);
	void move2end(plane_cx* p);
	dev_info_cx get_dev();
	glm::ivec2 get_size();
	// 是否支持Bindless
	bool has_variable();
public:

	void update(float delta);
	void present();

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
private:
};


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
	ef_null = ADBIT(0),			// ef_default
	ef_vulkan = ADBIT(1),		// vk渲染
	ef_resizable = ADBIT(2),	// 可以拉伸大小
	ef_transparent = ADBIT(3),	// 透明
	ef_borderless = ADBIT(4),	// 无系统边框
	ef_popup = ADBIT(5),		// 弹出式窗口，需要有父窗口
	ef_tooltip = ADBIT(6),		// 工具提示窗口，需要有父窗口
	ef_utility = ADBIT(7),		// 不出现在任务栏
	ef_fullscreen = ADBIT(8),	// 全屏
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

// 导出接口
uint64_t call_data(int type, void* data);
// 运行消息循环count==0则死循环, 执行循环次数
int run_app(void* app, int count);
app_cx* new_app();
void* new_app0();
void free_app(void* app);
