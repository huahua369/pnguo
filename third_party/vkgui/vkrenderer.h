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

namespace vkr {

	struct const_vk {
		// Create all the heaps for the resources views
		uint32_t cbvDescriptorCount = 2000;
		uint32_t srvDescriptorCount = 8000;
		uint32_t uavDescriptorCount = 10;
		uint32_t samplerDescriptorCount = 20;
		// Create a commandlist ring for the Direct queue
		uint32_t commandListsPerBackBuffer = 8;
		// Create a 'dynamic' constant buffer动态常量缓冲区大小
		uint32_t constantBuffersMemSize = 5 * 1024 * 1024;
		// Create a 'static' pool for vertices and indices 静态顶点/索引缓冲区大小
		uint32_t staticGeometryMemSize = (1 * 128) * 1024 * 1024;
		// Create a 'static' pool for vertices and indices in system memory静态几何缓冲区大小
		uint32_t systemGeometryMemSize = 32 * 1024;

		// Quick helper to upload resources, it has it's own commandList and uses suballocation.
		uint32_t uploadHeapMemSize = 500 * 1024 * 1024;
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

	struct light_t
	{
		// 平行光、点光源、聚光灯
		enum LightType { LIGHT_DIRECTIONAL, LIGHT_POINTLIGHT, LIGHT_SPOTLIGHT };
		glm::vec4	_color = glm::vec4(1.0);					// 颜色
		LightType	_type = LIGHT_DIRECTIONAL;
		float		_range = 50;					// 范围
		float       _intensity = 0.0f;			// 强度
		float       _innerConeAngle = 0.0f;	// 内锥角
		float       _outerConeAngle = 0.0f;	// 外锥角
		uint32_t    _shadowResolution = 1024;	// 阴影分辨率
		float       _bias = 70.0f / 100000.0f;	// 偏差0-2 
		size_t		_nodeid = 0;
	};


}

struct dev_info_cx;
#ifndef image_vkr
struct image_vkr
{
	glm::ivec2 size = {};
	void* vkimageptr = 0;
};
#endif

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
	bool  bDrawLightFrustum = false;
	bool  bDrawBoundingBoxes = false;
	bool  bShowMilliseconds = true;
};

struct mouse_state_t;
class vkdg_cx
{
public:
	void* ctx = 0;
	void* dev = 0;
	void* qupload = 0;
	std::vector<uint32_t> dt;
	int width = 0, height = 0;
	scene_state state = {};
public:
	vkdg_cx();
	~vkdg_cx();
	void set_label_cb(std::function<void(int count, int idx, const char* str)> cb);
	void update(mouse_state_t* io);
	void on_render();
	image_vkr get_vkimage(int idx);
	// 获取内部数据，0相机坐标
	glm::vec3 get_value(int idx);
	// 重置大小
	void resize(int w, int h);
	void save_fbo(int idx);
	void copy2(int idx, void* vkptr);
	void* new_pipe(const char* vertexShader, const char* pixelShader);


	void load_gltf(const char* fn, const glm::vec3& pos, float scale);
	vkr::light_t* get_light(size_t idx);
	size_t get_light_size();
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


// todo 创建渲染器
vkdg_cx* new_vkdg(dev_info_cx* c = 0);
void free_vkdg(vkdg_cx* p);
void load_gltf(vkdg_cx* p, const char* fn, const float* pos, float scale);




namespace vkr {
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



}
//!vkr