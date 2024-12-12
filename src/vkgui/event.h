#pragma once
#include <functional>
/*
struct mouse_move_et;
struct mouse_button_et;
struct mouse_wheel_et;
struct keyboard_et;
struct text_editing_et;
struct text_input_et;
struct finger_et;
struct ole_drop_et;
*/

#ifndef KM_CTRL

enum class event_type2 :uint32_t {
	none = 0,
	mouse_move,		// 鼠标移动，
	mouse_down,		// 鼠标按下
	mouse_up,		// 鼠标弹起
	mouse_wheel,	// 滚轮消息
	// on需要拾取目标（*矩形、*圆形、自定义判断）才能触发，单击、双击、三击（指定毫秒内）
	on_keypress,	// 按键，需要输入焦点
	on_input,		// 输入法字符，需要输入焦点
	on_editing,		// 输入法: 显示编辑的字符

	on_click,		// 单击双击三击时鼠标在目标范围才能触发
	on_dblclick,
	on_tripleclick,
	on_enter,		// 鼠标进入
	on_leave,		// 鼠标离开
	on_move,		// 鼠标在元素内移动
	on_down,		// 鼠标在元素内按下
	on_up,			// 鼠标弹起，触发条件：先down，鼠标位置不限
	on_hover,		// 鼠标停留在目标区域n毫秒触发
	on_scroll,		// 滚动条，目标范围内触发

	on_drag,		// 元素被拖动时鼠标移动触发
	on_dragstart,	// 当拖拽元素开始被拖拽的时候触发的事件，此事件作用在被拖曳元素上
	on_dragend,		// 当拖拽完成后触发的事件，此事件作用在被拖曳元素上

	on_dragenter,	// 当拖曳元素进入目标元素的时候触发的事件，此事件作用在目标元素上
	on_dragleave,	// 当被鼠标拖动的对象离开其容器范围内时触发此事件，此事件作用在目标元素上
	on_dragover,	// 拖拽元素在目标元素上移动的时候触发的事件，此事件作用在目标元素上
	on_drop,		// 被拖拽的元素在目标元素上同时鼠标放开触发的事件，此事件作用在目标元素上

	on_ole_dragover,// 接收OLE拖放在目标元素上移动的时
	on_ole_drop,	// 接收OLE拖放

	on_touch,		// 触控事件

	et_max_num
};
struct mouse_move_et
{
	int x, y;			// 鼠标移动坐标
	int xrel, yrel;		// The relative motion in the XY direction 
	uint8_t which;		// 鼠标实例 
	int cursor;			// 切换鼠标
};
struct mouse_button_et		// 鼠标弹起
{
	uint8_t which;
	uint8_t button;       // The mouse button index 1左，3右，2中
	uint8_t down;        // ::SDL_PRESSED or ::SDL_RELEASED 
	uint8_t clicks;       // 1 for single-click, 2 for double-click, etc. 
	int x, y;
};
struct mouse_wheel_et	// 滚轮消息
{
	int x, y;
	uint8_t which;
};
#define KM_CTRL 1
#define KM_SHIFT 2
#define KM_ALT 4
#define KM_GUI 8

struct keyboard_et
{
	uint16_t scancode;  // SDL physical key code - see ::SDL_Scancode for details 
	uint16_t mod;       // current key modifiers 
	int sym;            // SDL virtual key code - see ::SDL_Keycode for details 
	int keycode;
	int16_t kmod;		// Control=1 Shift=2 Alt=4 8Cmd/Super/Windows
	int8_t down;       // ::SDL_PRESSED or ::SDL_RELEASED 
	int8_t repeat;      // Non-zero if this is a key repeat 

};
struct text_editing_et
{
	char* text;// [32 * 8] ;  //The editing text
	int start;		// The start cursor of selected editing text 
	int length;		// The length of selected editing text 
	int x, y, w, h;
};
struct text_input_et
{
	char* text;  // The input text 
	int x, y, w, h;
};
// touch事件结构体
struct finger_et {
	int tid, touchId;
	float x, y, pressure; // 坐标、压力
	int t;	// FINGERDOWN=1 UP=2 MOTION=3 

};
// 多手势
struct mgesture_et {
	int touchId;
	float dTheta;
	float dDist;
	float x;
	float y;
	uint16_t numFingers;
};

// 接收ole拖动，结束数据count大于0
struct ole_drop_et
{
	const char** str;
	int count;			// 大于1一般是文件列表
	int fmt = -1;		// 0文本，1文件
	float x, y;			// 鼠标坐标

	int* has;			// 是否接收, 0不接收，1接收

};
class form_x;
struct et_un_t
{
	union
	{
		struct mouse_move_et* m;
		struct mouse_button_et* b;
		struct mouse_wheel_et* w;
		struct keyboard_et* k;
		struct text_editing_et* e;
		struct text_input_et* t;
		struct finger_et* f;
		struct ole_drop_et* d;
	}v;
	form_x* form = 0;
	uint8_t ret = 0;
};
enum class devent_type_e :uint32_t {
	none = 0,
	mouse_move_e,
	mouse_button_e,
	mouse_wheel_e,
	keyboard_e,
	text_editing_e,
	text_input_e,
	finger_e,
	mgesture_e,
	ole_drop_e,
	max_det
};
#else
struct et_un_t;
#endif


#ifndef INPUT_STATE_TH
#define INPUT_STATE_TH
struct input_state_t
{
	void* ptr = 0;
	std::function<void(uint32_t type, et_un_t* e, void* ud)> cb = nullptr;
	int x, y, w, h;
};
#endif

enum class cursor_st :uint32_t
{
	cursor_null,
	cursor_arrow,
	cursor_ibeam,
	cursor_wait,
	cursor_no,
	cursor_hand,
};

struct mouse_state_t
{
	float       DeltaTime;
	glm::vec2   MouseDelta;
	glm::vec2   MousePos;			// Mouse position, in pixels. Set to ImVec2(-FLT_MAX, -FLT_MAX) if mouse is unavailable (on another screen, etc.)
	bool        MouseDown[5];		// Mouse buttons: 0=left, 1=right, 2=middle + extras (ImGuiMouseButton_COUNT == 5). Dear ImGui mostly uses left and right buttons. Others buttons allows us to track if the mouse is being used by your application + available to user as a convenience via IsMouse** API.
	glm::vec2   wheel;				// x=Horizontal，y=Vertical: 1 unit scrolls about 5 lines text. 
	bool        KeysDown[512];
	bool        KeyCtrl;
	bool        KeyShift;
	bool        KeyAlt;
	bool        KeySuper;
	bool		WantCaptureMouse;
};
