#pragma once
/*
	vk渲染器
	Copyright (c) 华仔
	188665600@qq.com

	创建日期：2024-07-15
*/


// 渲染器配置
// 资源管理：加载、卸载
// 渲染指令

struct dev_info_cx;
struct image_vkr
{
	glm::ivec2 size = {};
	void* vkimageptr = 0;
};
struct mouse_state_t
{
	float       DeltaTime;
	glm::vec2   MouseDelta;               
	glm::vec2   MousePos;                       // Mouse position, in pixels. Set to ImVec2(-FLT_MAX, -FLT_MAX) if mouse is unavailable (on another screen, etc.)
	bool        MouseDown[5];                   // Mouse buttons: 0=left, 1=right, 2=middle + extras (ImGuiMouseButton_COUNT == 5). Dear ImGui mostly uses left and right buttons. Others buttons allows us to track if the mouse is being used by your application + available to user as a convenience via IsMouse** API.
	float       MouseWheel;                     // Mouse wheel Vertical: 1 unit scrolls about 5 lines text.
	float       MouseWheelH;                    // Mouse wheel Horizontal. Most users don't have a mouse with an horizontal wheel, may not be filled by all backends.
	bool        KeysDown[512];

	bool        KeyCtrl;		
	bool        KeyShift;		
	bool        KeyAlt;		
	bool        KeySuper;
	bool		WantCaptureMouse;
};
class vkdg_cx
{
public:
	void* ctx = 0;
	void* dev = 0;
	void* qupload = 0;
	std::vector<uint32_t> dt;
	int width = 0, height = 0;
	mouse_state_t io = {};
public:
	vkdg_cx();
	~vkdg_cx();
	void update();
	void on_render();
	image_vkr get_vkimage(int idx);
	void save_fbo(int idx);
	void copy2(int idx, void* vkptr);
private:

};

// 节点包围盒渲染
class BBoxPass
{
public:
	struct mat_idx
	{
		glm::mat4* m = 0;
		size_t pos = 0, count = 0;
	};
	struct box_t
	{
		glm::vec4 vCenter, vRadius;
	};
	std::vector<mat_idx> box_ms;	// 节点矩阵，box开始\数量
	std::vector<box_t> boxs;		// box坐标半径
public:
	BBoxPass();
	~BBoxPass();
	// 收集所有节点包围盒
	void add(glm::mat4* m, box_t* vcr, size_t count);
	void clear();
private:

};


// 创建渲染器
vkdg_cx* new_vkdg(dev_info_cx* c = 0);
void free_vkdg(vkdg_cx* p);
void load_gltf(vkdg_cx* p, const char* fn);
