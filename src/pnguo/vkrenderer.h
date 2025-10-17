﻿#pragma once
/*
	vk渲染器
	Copyright (c) 华仔
	188665600@qq.com

	创建日期：2024-07-15

	示例：

void vkrender_test()
{
	auto inst = new_instance();
	//auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
	std::vector<device_info_t> devs = get_devices(inst);  // 获取设备名称列表
	if (devs.empty())return;
	//vkdg_cx* vkd = new_vkdg(sdldev.inst, sdldev.phy, sdldev.vkdev);	// 从SDL3获取设备创建vk渲染器
	vkdg_cx* vkd = new_vkdg(inst, devs[0].phd, 0);			// 创建vk渲染器
	vkd->add_gltf(R"(E:\model\sharp2.glb)", { 0,0,0 }, 1.0);// 添加一个地板
	vkd->resize(1024, 800);				// 设置fbo缓冲区大小
	auto vki = vkd->get_vkimage(0);	// 获取第一个fbo纹理弄到窗口显示
	//auto texok = form0->add_vkimage(vki.size, vki.vkimage, { 20,36 }, 0);// 创建SDL的rgba纹理
	auto dstate = vkd->_state;	// 渲染状态属性
	{
		mouse_state_t mio = {};
		//mio = *form0->io;
		// 独立线程或主线程执行渲染
		while (1) {
			vkd->update(&mio);		// 更新事件
			vkd->on_render();		// 执行渲染
			// todo 提交到窗口渲染
			Sleep(1);
		}
	}
	free_vkdg(vkd);				// 释放渲染器
	free_instance(inst);		// 释放实例
}


 渲染器配置
 资源管理：加载、卸载

todo gltf格式
gltf加载：解析gltf文件，提取纹理、材质、网格信息
提交网格、纹理、材质数据，实现渲染

 ResourceViewHeaps* pResourceViewHeaps,		// 管理set分配
 DynamicBufferRing* pConstantBufferRing,	// 动态常量缓冲区ubo
 StaticBufferPool* pStaticBufferPool,		// 静态顶点/索引缓冲区
*/
namespace vkr {
	struct transform_t
	{
		glm::quat rotation;		//  四元数，用于存储变换在世界空间中的旋转。
		glm::vec3 position;		//	世界空间中的变换位置。
		glm::vec3 forward;		//	返回一个标准化矢量，它表示世界空间中变换的蓝轴。
		glm::vec3 right;		//	世界空间中变换的红轴。 
		glm::vec3 up;			//	世界空间中变换的绿轴。 
		glm::vec3 scale;		//	缩放。 
		bool hasChanged;		//	自上次将标志设置为“false”以来，变换是否发生更改  
	};
	struct node_t
	{
		int idx;
		int root;				//	transform_t*层级视图中最顶层的变换。
		int parent;				//	变换的父级。
		int* child;				//	子项。 
		int child_count;		//	子项数量。
	};

	struct const_vk {
		// Create all the heaps for the resources views
		uint32_t cbvDescriptorCount = 2000;
		uint32_t srvDescriptorCount = 8000;
		uint32_t uavDescriptorCount = 10;
		uint32_t samplerDescriptorCount = 20;
		// Create a commandlist ring for the Direct queue
		uint32_t commandListsPerBackBuffer = 8;
		// Create a 'dynamic' constant buffer动态常量缓冲区大小
		uint32_t constantBuffersMemSize = 10 * 1024 * 1024;
		// Create a 'static' pool for vertices and indices 静态顶点/索引缓冲区大小
		uint32_t staticGeometryMemSize = (10 * 128) * 1024 * 1024;
		// Create a 'static' pool for vertices and indices in system memory静态几何缓冲区大小
		uint32_t systemGeometryMemSize = 32 * 1024;

		// Quick helper to upload resources, it has it's own commandList and uses suballocation.
		uint32_t uploadHeapMemSize = (uint32_t)128 * 2048 * 2048;
	};
	struct gtime_t
	{
	public:
		// 只读
		uint64_t frameCount = 0.0;			//*The total number of frames that have passed(Read Only).
		double time = 0.0;					//*The time at the beginning of this frame(Read Only).This is the time in seconds since the start of the game.
		double fixedTime = 0.0;				//The time the latest FixedUpdate has started(Read Only).This is the time in seconds since the start of the game.
		double realtimeSinceStartup = 0.0;	//*The real time in seconds since the game started(Read Only).
		double unscaledTime = 0.0;			//*The timeScale - independant time for this frame(Read Only).This is the time in seconds since the start of the game.

		float deltaTime = 0.0;				//*The completion time in seconds since the last frame. (Read Only).
		float fixedUnscaledDeltaTime = 0.0; //The timeScale - independent interval in seconds from the last fixed frame to the current one(Read Only).
		float fixedUnscaledTime = 0.0;		//The TimeScale - independant time the latest FixedUpdate has started(Read Only).This is the time in seconds since the start of the game.

		float smoothDeltaTime = 0.0;		//*A smoothed out Time.deltaTime(Read Only).
		float timeSinceLevelLoad = 0.0;		//*The time this frame has started(Read Only).This is the time in seconds since the last level has been loaded.
		float unscaledDeltaTime = 0.0;		//*The timeScale - independent interval in seconds from the last frame to the current one(Read Only).
		float fps = 0.0f;
		// 可写
		float captureFramerate = 0.0;		//Slows game playback time to allow screenshots to be saved between frames.
		float timeScale = 1.0;				//The scale at which the time is passing.This can be used for slow motion effects.
		float fixedDeltaTime = 0.02;		//The interval in seconds at which physicsand other fixed frame rate updates(like MonoBehaviour's FixedUpdate) are performed.

		float maximumDeltaTime = 1.0 / 3.0;		//The maximum time a frame can take.Physics and other fixed frame rate updates(like MonoBehaviour's FixedUpdate) will be performed only for this duration of time per frame.
		float maximumParticleDeltaTime = 1.0 / 3.0;//The maximum time a frame can spend on particle updates.If the frame takes longer than this, then updates are split into multiple smaller updates.
		bool inFixedTimeStep = true;		//Returns true if called inside a fixed time step callback(like MonoBehaviour's FixedUpdate), otherwise returns false.
	};
	// https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
	struct light_t
	{
		/*
		平行光：只有角度的光源，无衰减
		点光源：位置、范围、无阴影、没有旋转缩放
		聚光灯：有位置、方向、角度
		*/
		enum LightType { LIGHT_DIRECTIONAL, LIGHT_POINTLIGHT, LIGHT_SPOTLIGHT };
		glm::vec4	_color = glm::vec4(1.0);		// 颜色
		float		_range = 50;					// 范围, 点光、聚光灯有效
		float       _intensity = 1.0f;				// 强度
		float       _cone_angle = 45.0f;			// 锥角，聚光灯有效,1-180
		float       _cone_mix = 0.20f;				// 混合，聚光灯有效
		uint32_t    _shadowResolution = 1024;		// 阴影分辨率
		float       _bias = 0.0007;					// 偏差0-2 
		glm::quat	_rotation = glm::quat(1, 0, 0, 0);		// 旋转\方向
		glm::vec3	_position = glm::vec3(0, 0, 0);			// 位置
		int16_t	_type = 0;
		int16_t	_ac = 0;
	};


	std::string format(const char* format, ...);

	/*
	场景编辑管理
	*/
	class scene_edit
	{
	public:
		std::vector<light_t*> lights;
		int last_idx = 0;				// 当前分配的索引位置
		int max_count = 80;				// 每次分配最大数量
	public:
		scene_edit();
		~scene_edit();
		// 默认光源
		void default_light();
		light_t* add_directional_light(float intensity, const glm::vec4& color, int shadow_width);
		light_t* add_spot_light(float intensity, const glm::vec4& color, float range, float cone_angle, float cone_mix, int shadow_width);
		light_t* add_point_light(float intensity, const glm::vec4& color, float range);
		// 添加空类型光源，后面填充数据
		light_t* add_light();
	};
}
//!vkr

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

struct scene_state {
	// POST PROCESS CONTROLS 
	int   SelectedTonemapperIndex = 0;// 0-5
	float Exposure = 1.0;
	// APP/SCENE CONTROLS 
	float IBLFactor = 1.0;
	float EmissiveFactor = 50.0;
	int   SelectedSkydomeTypeIndex = 0; // 0-1 

	enum class WireframeMode : int
	{
		WIREFRAME_MODE_OFF = 0,
		WIREFRAME_MODE_SHADED = 1,
		WIREFRAME_MODE_SOLID_COLOR = 2,
	};
	int WireframeMode = 0;
	float WireframeColor[3] = { 1.0,1.0,1.0 };
	bool  bUseTAA = true;
	bool  bTAAsharpening = true;
	bool  bDrawLightFrustum = false;
	bool  bDrawBoundingBoxes = false;
	bool  bShowMilliseconds = true;
	bool  bBloom = true;
};

struct image_d
{
	glm::ivec2 size = {};
	uint32_t* data = 0;
};
struct mouse_state_t;
struct image_vkr
{
	glm::ivec2 size = {};
	void* vkimage = 0;
};
class vkdg_cx
{
public:
	dev_info_cx _dev_info = {};
	void* ctx = 0;
	void* dev = 0;
	void* qupload = 0;
	void* renderpass_opaque = 0;		// 用于渲染不透明物体及清空缓冲区opaque
	void* renderpass_justdepth = 0;		// 用于渲染天空盒、线框justdepth
	void* renderpass_transparent = 0;	// 用于渲染透明物体transparent
	void* renderpass_fbo = 0;			// 结果缓冲区fbo 
	std::vector<uint32_t> dt;  // save_fbo缓存像素时用
	int width = 0, height = 0;
	scene_state _state = {};
public:
	vkdg_cx();
	~vkdg_cx();
	void set_label_cb(std::function<void(int count, int idx, const char* str)> cb); // 设置标签回调
	void update(mouse_state_t* io); // 更新事件
	void on_render();				// 执行渲染
	// 获取framebuffer image
	image_vkr get_vkimage(int idx);
	// 重置大小
	void resize(int w, int h);
	// 保存fbo idx的像素到dt
	void save_fbo(int idx);
	image_d save_shadow(int idx);
	// 复制fbo idx到 VkImage vkptr
	void copy2(int idx, void* vkptr);
	// 添加gltf模型到渲染器
	void add_gltf(const char* fn, const glm::vec3& pos, float scale, bool has_shadowMap = false);
	// 获取灯光信息
	vkr::light_t* get_light(size_t idx);
	size_t get_light_size();
};

// 创建vk实例,参数可选
void* new_instance();
void free_instance(void* inst);

// todo 创建渲染器，shader目录默认"ShaderLibVK", "cache/shadervk"
vkdg_cx* new_vkdg(void* inst, void* phy, void* dev, const char* shaderLibDir = 0, const char* shaderCacheDir = 0);
// f是否需要删除vk资源
void free_vkdg(vkdg_cx* p, bool f = false);

struct device_info_t
{
	char name[256];
	void* phd;
};
// 获取设备名称列表
std::vector<device_info_t> get_devices(void* inst);

void get_queue_info(void* physicaldevice);


namespace vkr {
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

	// 顶点数据、纹理、矩阵、灯光、渲染命令
	// 多边形
	struct mesh_mt
	{
		std::vector<uint32_t> face_size;		// 面的边数3\4、多边形
		std::vector<uint32_t> face_indice;		// 索引
		std::vector<double> vertex_coord;		// 顶点坐标
		std::vector<glm::uvec2> halfedge;		// 半边
	public:
		void add_vertex(const glm::dvec3* v, size_t n);
		void add_vertex(const glm::vec3* v, size_t n);
	};
	// 三角形
	struct mesh3_mt
	{
		std::vector<glm::ivec3>	indices;	// 三角形索引
		std::vector<glm::vec3>	vertices;	// 顶点坐标
	};
	// 简易材质
	struct mesh_material_mt
	{
		std::vector<uint32_t> color;	// 颜色uv数量对应顶点坐标
		std::vector<glm::vec2> uv;
	};
	// 点、线模式
	struct point_draw_t
	{
		uint32_t color = 0;
		uint32_t width = 0;
	};
	struct line_draw_t
	{
		uint32_t color = 0;
		uint32_t width = 0;
	};
	// 渲染数据
	struct rdata_t {
		mesh3_mt* d = 0;
		glm::mat4 m;
		bool visible = true;
	};
	// 添加渲染数据对象
	void vkdg_add(vkdg_cx* p, rdata_t* d);
	// 删除对象
	void vkdg_remove(vkdg_cx* p, rdata_t* d);

	// 创建brdflut纹理数据，大小，类型0=float16，1=rgba
	std::vector<uint32_t> generateCookTorranceBRDFLUT(uint32_t mapDim, int type);
	// todo速度比上面函数慢，类型0 = float16，1 = float32
	std::vector<uint32_t> generateCookTorranceBRDFLUT1632f(uint32_t mapDim, int type);

	void* new_ms_pipe(void* device, void* renderPass);
}
//!vkr

uint32_t gray_float_to_rgba(float gray);

