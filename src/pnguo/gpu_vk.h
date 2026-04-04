#pragma once
/*
	vk渲染器
	Copyright (c) 华仔
	188665600@qq.com

	创建日期：2026-02-07

--------------------------------------------------------------------------------------------------------------
todo:星号是已实现完成的
	设备：*
		采样器：	结构体sampler_info_t*，newSampler函数创建VkSampler
		图像：	2D纹理、3D纹理、立方体贴图、纹理数组。*
		缓冲区：	static_buffer_pool_cx*(vbo、ibo)、buffer_ring_cx*(ubo、ssbo)
		管线：	pbr着色器*、自定义着色器*
		渲染器：	向前渲染缓冲区、bloom、后处理等*
	对象：
		帧图：相机、世界、渲染器、rgba纹理、深度纹理等
		世界：群组、实例
		实例：群组、实例数量、实例矩阵等数据
		群组：表面、灯光
		表面：几何、材质
		几何：三角形->顶点、索引(可选)
		材质：pbr参数、纹理、自定义管线。
		变换：transform_t
		动画：
		灯光：
		相机：



	几何：6种默认可选顶点属性
		vec3 a_Position;
		vec3 a_Color0;		vec3 a_Color1;
		vec2 a_UV0;			vec2 a_UV1;
		vec3 a_Normal;
		vec4 a_Tangent;
		vec4 a_Weights0;	vec4 a_Weights1;	uvec4 a_Joints0;	uvec4 a_Joints1;
	变形动画
		变形插值ubo数据：float u_morphWeights[];
		静态目标ssbo数据：每个顶点有插值数据、数量、偏移
			int vertex_count;			int weight_count;
			int position_offset;		int normal_offset;
			int tangent_offset;			int texcoord0_offset;
			int color0_offset;			int texcoord1_offset;
			vec4 per_target_data[];
	骨骼动画
		计算后的矩阵ubo数据：mat4 u_ModelMatrix[];
--------------------------------------------------------------------------------------------------------------
	对象：Camera、World、Renderer、Frame、Instance、Group、Surface、Geometry、Material、Light
	关系图
		Frame		: Camera(1:1)、World(1:1)、Renderer(1:1)
		World		: Instance(1:N)、Group(1:N)
		Instance	: Group(1:1) 一个Instance对象只能绑定一个Group。参数需要设置实例数量\矩阵等数据
		Group		: Surface(1:N)、Light(1:N)
		Surface		: Geometry(1:1)、Material(1:1)
		Geometry	: 网格数据、transform、动画等属性
		FrameCS		: 计算帧,绑定pipelineCS。用于计算任务，可绑定输出到数组或纹理
		pipelineCS	: 计算管线, 绑定数组、纹理做输入
		Material	：材质由shader和参数、纹理、自定义pipeline等资源组成。
		NO_WAIT 0
		WAIT 1
--------------------------------------------------------------------------------------------------------------

渲染需要：VkBuffer、VkDescriptorSet、VkPipeline
	VkBuffer：vbo、ibo、ubo、ssbo
	VkDescriptorSet：纹理、ubo\ssbo绑定点7个ubo, 23个纹理、3个环境纹理、MAX_SHADOW_INSTANCES个阴影纹理
	VkPipeline：
vs:
	顶点属性：使用分散绑定
	Frame：set 0,binding 0
		mat4 u_mCameraCurrViewProj;
		mat4 u_mCameraPrevViewProj;
	Object：绑定set 0,binding 1		mat4[2]关节动画用
	变形动画(可选)：float u_morphWeights[]，vec4 per_target_data[]
	骨骼动画(可选)：mat4 u_ModelMatrix[]
	实例化(可选)：mat4 instance_model_matrix[]
ps:
	场景结构：set 0,binding 0 struct PerFrame,相机、灯光等参数
	材质结构：set 0,binding 1 Object和struct pbrMaterial
	UV矩(可选)：mat3 u_matuv[]
	纹理：set 1
		普通uniform sampler2D[23]
		环境samplerCube[3]
		阴影sampler2DShadow[MAX_SHADOW_INSTANCES]


*/
namespace vkg {
#pragma pack(push, 16)
	// 224字节
	struct pbr_factors_t
	{
		glm::vec4 baseColorFactor = glm::vec4(1.0);
		float metallicFactor = 0;			// pbrMetallicRoughness金属度
		float roughnessFactor = 0;			// 粗糙度
		float normalScale = 1.0;
		// KHR_materials_emissive_strength自发光强度
		float emissiveStrength = 1.0;
		glm::vec3 emissiveFactor = glm::vec3(0.0);// 自发光颜色
		int   alphaMode;	float alphaCutoff;	float occlusionStrength;
		//KHR_materials_ior折射率
		float ior = 1.5;		int mipCount = 10;
		// KHR_materials_pbrSpecularGlossiness镜面反射-光泽度
		glm::vec4 pbrDiffuseFactor = glm::vec4(1.0);	glm::vec3 pbrSpecularFactor = glm::vec3(1.0);
		float glossinessFactor = 1.0;
		// Specular KHR_materials_specular镜面反射
		glm::vec3  specularColorFactor = glm::vec3(1.0);
		float specularFactor = 1.0;
		// KHR_materials_sheen 光泽、织物丝光
		glm::vec3 sheenColorFactor = glm::vec3();
		float sheenRoughnessFactor = 0;
		// KHR_materials_anisotropy各向异性
		glm::vec3 anisotropy = glm::vec3();
		// KHR_materials_transmission透射材质
		float transmissionFactor = 0;	glm::ivec2 transmissionFramebufferSize;
		float thicknessFactor;
		float diffuseTransmissionFactor;
		glm::vec3 diffuseTransmissionColorFactor;
		//KHR_materials_dispersion色散/弥散
		float dispersion = 0;
		glm::vec3 attenuationColor = glm::vec3(1.0);
		float attenuationDistance = 10240;
		// KHR_materials_iridescence彩虹色
		float iridescenceFactor = 0;	float iridescenceIor = 1.3;		float iridescenceThicknessMinimum = 100;	float iridescenceThicknessMaximum = 400;
		// KHR_materials_clearcoat清漆
		float clearcoatFactor = 0;		float clearcoatRoughness = 0;	float clearcoatNormalScale = 1;
		float envIntensity = 1;		// 环境强度
		//int unlit = 0;			// 宏定义实现非pbr材质，unlit材质不受光照影响，直接输出baseColorFactor颜色		 
	};
#pragma pack(pop)

	struct sampler_info_t {
		uint32_t magFilter;
		uint32_t minFilter;
		uint32_t addressModeU;
		uint32_t addressModeV;
		uint32_t addressModeW;
		int mipLodBias;
		int maxAnisotropy;
		uint32_t compareOp;
		int minLod;
		int maxLod;
		uint32_t borderColor;
		bool mipmapMode;		//		VK_SAMPLER_MIPMAP_MODE_NEAREST = 0,			VK_SAMPLER_MIPMAP_MODE_LINEAR = 1,
		bool anisotropyEnable;
		bool compareEnable;
		bool unnormalizedCoordinates;
	};
	enum GBufferFlagBits
	{
		GBUFFER_NONE = 0,
		GBUFFER_DEPTH = 1,
		GBUFFER_FORWARD = 2,
		GBUFFER_MOTION_VECTORS = 4,
		GBUFFER_NORMAL_BUFFER = 8,
		GBUFFER_DIFFUSE = 16,
		GBUFFER_SPECULAR_ROUGHNESS = 32,
	};
	struct transform_t
	{
		glm::quat rotation;		//  四元数，用于存储变换在世界空间中的旋转。
		glm::vec3 position;		//	世界空间中的变换位置。
		glm::vec3 forward;		//	返回一个标准化矢量，它表示世界空间中变换的蓝轴。
		glm::vec3 right;		//	世界空间中变换的红轴。 
		glm::vec3 up;			//	世界空间中变换的绿轴。 
		glm::vec3 scale;		//	缩放。 
		bool hasChanged;		//	自上次将标志设置为false以来，变换是否发生更改  
	};
	struct node_t
	{
		int idx;
		int root;				//	transform_t*层级视图中最顶层的变换。
		int parent;				//	变换的父级。
		int* child;				//	子项。 
		int child_count;		//	子项数量。
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
	};

	// 变形动画偏移 MORPH_TARGET_*_OFFSET	 
	struct morph_target_t {
		int vertex_count = 0;
		int weight_count = 0;
		int position_offset = -1;
		int normal_offset = -1;
		int tangent_offset = -1;
		int texcoord0_offset = -1;
		int color0_offset = -1;
		int texcoord1_offset = -1;	//int color1_offset;
	};
	// 全局、对象、骨骼动画、实例化、材质UV、目标数据、变形数据，的大小或绑定点索引
	struct ubotex_size_t {
		// 普通纹理、环境纹理、阴影纹理数量
		int texCounts[3] = { 25, 3, 4 };
		int ID_PER_FRAME = 1;
		int ID_PER_OBJECT = 1;
		int ID_SKINNING_MATRICES = 0;
		int ID_INSTANCING = 0;
		int ID_MATUV_DATA = 0;
		int ID_TARGET_DATA = 0;
		int ID_MORPHING_DATA = 0;
	};
	struct vertex_attribute_b
	{
		uint16_t name;		// 0=POSITION、1=COLOR_0、2=TEXCOORD_0、3=NORMAL、4=TANGENT、5=WEIGHTS_0、6=JOINTS_0、7=TEXCOORD_1、8=WEIGHTS_1、9=JOINTS_1
		uint16_t binding;	// 同样绑定号则同一块
		uint32_t format;	// VkFormat
		uint32_t offset;	// 如果bindings/bindingCount为空则自动计算
	};
	struct vertex_input_binding_description {
		uint32_t binding;	// 绑定号
		uint32_t stride;	// 结构大小
		uint32_t inputRate;	// 0顶点，1实例
	};
	enum ShaderSourceType
	{
		SST_HLSL,
		SST_GLSL
	};
	// 管线状态信息
	struct pipeline_state_t {
		int lineWidth = 1;
		int polygonMode = 0;//    VK_POLYGON_MODE_FILL = 0,		VK_POLYGON_MODE_LINE = 1,			VK_POLYGON_MODE_POINT = 2,
		int frontFace = 0;
		int topology = 3;
		int rasterizationSamples = 0x00000001;
		uint64_t renderPass = 0;
		bool blending = false;
		bool primitiveRestartEnable = false;
		bool doubleSided = false;	// cullMode
		bool depthTestEnable = true;
		bool stencilTestEnable = false;
	};
	struct PBRPipe_t;
	// 输入管线信息
	struct pipeline_info_2 {
		pipeline_state_t info = {};		// 状态信息
		ubotex_size_t set_binding = {}; // 绑定信息 
		morph_target_t morph = {};		// 变形结构信息
		vertex_attribute_b attributes[16] = {};	// 最多16个顶点属性
		vertex_input_binding_description bindings[16] = {};// 最多16个绑定，为空时默认自动计算
		const char** names = 0;					// 自定义属性名称，10开始，0-9为预定义属性
		int attributeCount = 0;
		int bindingCount = 0;
		int namesCount = 0;
		int define_count = 0;			// kv数量
		GBufferFlagBits gbufferflag = GBufferFlagBits::GBUFFER_NONE;
		const char** defines0 = 0;		// "key","value"
		const char* vertexShaderFile = 0;
		const char* fragmentShaderFile = 0;
	};

	struct PassParameters
	{
		uint32_t    uImageWidth = 1;
		uint32_t    uImageHeight = 1;
		int         iMousePos[2] = {};            // in pixels, driven by MousePos.xy
		float       fBorderColorRGB[4] = { 1,1,1,1 };      // Linear RGBA
		float       fMagnificationAmount = 6.0f;    // [1-...]
		float       fMagnifierScreenRadius = 0.35f;  // [0-1]
		mutable int iMagnifierOffset[2] = { 500,-500 };     // in pixels
	};



	class cxObject;		// 通用对象
	class cxDevice;		// 设备
	class cxCamera;		// 相机
	class cxArray;		// 数组
	class cxArray1D;	// 一维数组
	class cxArray2D;	// 二维数组
	class cxArray3D;	// 三维数组
	class cxFrame;		// 帧：渲染帧、计算帧
	class cxGeometry;	// 几何体
	class cxGroup;		// 组
	class cxInstance;	// 实例
	class cxLight;		// 光源
	class cxMaterial;	// 材质
	class cxPipeline;	// 渲染管线
	class cxPipelineCS;	// 计算管线
	class cxSampler;	// 纹理采样
	class cxSurface;	// 表面
	class cxRenderer;	// 渲染器
	class cxWorld;		// 世界

	void new_pbr_pipeline(cxDevice* pdev, PBRPipe_t* pbrpipe, pipeline_info_2* info2);

	void new_shader(cxDevice* pdev, const char* f, const char* v, int type);
}
//!vkg

// 数据类型
#if 1

#ifdef __cplusplus
struct ADataType
{
	ADataType() = default;
	constexpr ADataType(int v) noexcept : value(v) {}
	constexpr operator int() const noexcept { return value; }
	constexpr bool operator==(int v) const noexcept { return v == value; }
private:
	int value;
};
constexpr bool operator<(ADataType v1, ADataType v2)
{
	return static_cast<int>(v1) < static_cast<int>(v2);
}
#define A_DATA_TYPE_DEFINE(v) ADataType(v)
#else
typedef int ADataType;
#define A_DATA_TYPE_DEFINE(v) v
#endif 

#endif // 1


#ifdef __cplusplus
extern "C" {
#endif
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
	typedef void* aObject;		// 通用对象
	typedef void* aDevice;		// 设备
	typedef void* aCamera;		// 相机
	typedef void* aArray;		// 数组： 一维数组、二维数组、三维数组
	typedef void* aFrame;		// 帧：渲染帧、计算帧
	typedef void* aGeometry;	// 几何体
	typedef void* aGroup;		// 组
	typedef void* aInstance;	// 实例
	typedef void* aLight;		// 光源
	typedef void* aMaterial;	// 材质
	typedef void* aPipeline;	// 渲染管线
	typedef void* aPipelineCS;	// 计算管线
	typedef void* aSampler;		// 纹理采样
	typedef void* aSurface;		// 表面
	typedef void* aRenderer;	// 渲染器
	typedef void* aWorld;		// 世界
	enum obj_type_e
	{
		OBJ_DEVICE,
		OBJ_CAMERA,
		OBJ_ARRAY,
		OBJ_FRAME,
		OBJ_GEOMETRY,
		OBJ_GROUP,
		OBJ_INSTANCE,
		OBJ_LIGHT,
		OBJ_MATERIAL,
		OBJ_PIPELINE, OBJ_PIPELINECS,
		OBJ_SAMPLER,
		OBJ_SURFACE,
		OBJ_RENDERER,
		OBJ_WORLD
	};

	/*
	*	void* inst, void* phy, void* dev可选
	* 	set_param(); 着色器编译缓存目录const char* shaderLibDir = 0, const char* shaderCacheDir = 0,
	*	devname显卡名
	*/
	struct adevice3_t
	{
		void* ctx = 0;
		aDevice(*new_device)(void* ctx, void* phy, void* dev, const char* devname, void* hwnd);
		// obj_type_e
		aObject(*new_object)(aDevice* dev, int obj_type, const char* type);
		// 删除对象
		void (*release)(aObject obj);
		// 增加引用计数
		void (*retain)(aObject obj);
		// 设置参数
		void (*set_param)(aObject obj, const char* name, int data_type, void* data);
		// 取消参数
		void (*unset_param)(aObject object, const char* name);
		void (*unset_allparams)(aObject object);
		// 提交参数
		void (*commit_params)(aObject object);
		// 获取对象支持的参数名
		size_t(*get_param_count)(aObject object);
		const char** (*get_param_names)(aObject object);
		// 获取参数值, data_type返回值类型
		void* (*get_param)(aObject object, const char* name, int* data_type);
		// 获取属性
		bool (*get_property)(aObject object, const char* name, int data_type, void* mem, uint64_t size, int wait_mask);
		// 提交帧渲染
		void (*render_frame)(aFrame frame);
		// 帧渲染完成检查，0完成，1未完成，-1渲染出错
		int (*frame_ready)(aFrame frame, int wait_mask);
	};
	// 创建管理器
	adevice3_t* new_gdev(const char* pApplicationName, const char* pEngineName);
	// 删除管理器
	void free_gdev(adevice3_t* p);

	void gpu_vk_test0();

#ifdef __cplusplus
}
#endif


