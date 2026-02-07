#pragma once
/*
	vk渲染器
	Copyright (c) 华仔
	188665600@qq.com

	创建日期：2026-02-07

--------------------------------------------------------------------------------------------------------------
	对象：Camera、World、Renderer、Frame、Instance、Group、Surface、Geometry、Material、Light	 
	关系图
		Frame		: Camera(1:1)、World(1:1)、Renderer(1:1)
		World		: Instance(1:N)、Group(1:N)
		Instance	: Group(1:1) 一个Instance对象只能绑定一个Group。参数需要设置实例数量\矩阵等数据
		Group		: Surface(1:N)、Light(1:N)
		Surface		: Geometry(1:1)、Material(1:1)、或自定义pipeline
		Geometry	: 网格数据、transform、动画等属性
		FrameCS		: 计算帧,绑定pipelineCS。用于计算任务，可绑定输出到数组或纹理
		pipelineCS	: 计算管线, 绑定数组、纹理做输入

		NO_WAIT 0
		WAIT 1 
--------------------------------------------------------------------------------------------------------------

渲染需要：VkBuffer、VkDescriptorSet、VkPipeline
	VkBuffer：vbo、ibo、ubo、ssbo
	VkDescriptorSet：纹理、ubo\ssbo绑定
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
		普通uniform sampler2D[24]
		环境samplerCube[3]
		阴影sampler2DShadow[MAX_SHADOW_INSTANCES]


*/
namespace vkg {
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
#define A_UNKNOWN A_DATA_TYPE_DEFINE(0)
#define A_DATA_TYPE A_DATA_TYPE_DEFINE(100)
#define A_STRING A_DATA_TYPE_DEFINE(101)
#define A_VOID_POINTER A_DATA_TYPE_DEFINE(102)
#define A_BOOL A_DATA_TYPE_DEFINE(103)
#define A_STRING_LIST A_DATA_TYPE_DEFINE(150)
#define A_DATA_TYPE_LIST A_DATA_TYPE_DEFINE(151)
#define A_PARAMETER_LIST A_DATA_TYPE_DEFINE(152)
#define A_FUNCTION_POINTER A_DATA_TYPE_DEFINE(200)
#define A_MEMORY_DELETER A_DATA_TYPE_DEFINE(201)
#define A_STATUS_CALLBACK A_DATA_TYPE_DEFINE(202)
#define A_LIBRARY A_DATA_TYPE_DEFINE(500)
#define A_DEVICE A_DATA_TYPE_DEFINE(501)
#define A_OBJECT A_DATA_TYPE_DEFINE(502)
#define A_ARRAY A_DATA_TYPE_DEFINE(503)
#define A_ARRAY1D A_DATA_TYPE_DEFINE(504)
#define A_ARRAY2D A_DATA_TYPE_DEFINE(505)
#define A_ARRAY3D A_DATA_TYPE_DEFINE(506)
#define A_CAMERA A_DATA_TYPE_DEFINE(507)
#define A_FRAME A_DATA_TYPE_DEFINE(508)
#define A_GEOMETRY A_DATA_TYPE_DEFINE(509)
#define A_GROUP A_DATA_TYPE_DEFINE(510)
#define A_INSTANCE A_DATA_TYPE_DEFINE(511)
#define A_LIGHT A_DATA_TYPE_DEFINE(512)
#define A_MATERIAL A_DATA_TYPE_DEFINE(513)
#define A_RENDERER A_DATA_TYPE_DEFINE(514)
#define A_SURFACE A_DATA_TYPE_DEFINE(515)
#define A_SAMPLER A_DATA_TYPE_DEFINE(516)
#define A_SPATIAL_FIELD A_DATA_TYPE_DEFINE(517)
#define A_VOLUME A_DATA_TYPE_DEFINE(518)
#define A_WORLD A_DATA_TYPE_DEFINE(519)
#define A_INT8 A_DATA_TYPE_DEFINE(1000)
#define A_INT8_VEC2 A_DATA_TYPE_DEFINE(1001)
#define A_INT8_VEC3 A_DATA_TYPE_DEFINE(1002)
#define A_INT8_VEC4 A_DATA_TYPE_DEFINE(1003)
#define A_UINT8 A_DATA_TYPE_DEFINE(1004)
#define A_UINT8_VEC2 A_DATA_TYPE_DEFINE(1005)
#define A_UINT8_VEC3 A_DATA_TYPE_DEFINE(1006)
#define A_UINT8_VEC4 A_DATA_TYPE_DEFINE(1007)
#define A_INT16 A_DATA_TYPE_DEFINE(1008)
#define A_INT16_VEC2 A_DATA_TYPE_DEFINE(1009)
#define A_INT16_VEC3 A_DATA_TYPE_DEFINE(1010)
#define A_INT16_VEC4 A_DATA_TYPE_DEFINE(1011)
#define A_UINT16 A_DATA_TYPE_DEFINE(1012)
#define A_UINT16_VEC2 A_DATA_TYPE_DEFINE(1013)
#define A_UINT16_VEC3 A_DATA_TYPE_DEFINE(1014)
#define A_UINT16_VEC4 A_DATA_TYPE_DEFINE(1015)
#define A_INT32 A_DATA_TYPE_DEFINE(1016)
#define A_INT32_VEC2 A_DATA_TYPE_DEFINE(1017)
#define A_INT32_VEC3 A_DATA_TYPE_DEFINE(1018)
#define A_INT32_VEC4 A_DATA_TYPE_DEFINE(1019)
#define A_UINT32 A_DATA_TYPE_DEFINE(1020)
#define A_UINT32_VEC2 A_DATA_TYPE_DEFINE(1021)
#define A_UINT32_VEC3 A_DATA_TYPE_DEFINE(1022)
#define A_UINT32_VEC4 A_DATA_TYPE_DEFINE(1023)
#define A_INT64 A_DATA_TYPE_DEFINE(1024)
#define A_INT64_VEC2 A_DATA_TYPE_DEFINE(1025)
#define A_INT64_VEC3 A_DATA_TYPE_DEFINE(1026)
#define A_INT64_VEC4 A_DATA_TYPE_DEFINE(1027)
#define A_UINT64 A_DATA_TYPE_DEFINE(1028)
#define A_UINT64_VEC2 A_DATA_TYPE_DEFINE(1029)
#define A_UINT64_VEC3 A_DATA_TYPE_DEFINE(1030)
#define A_UINT64_VEC4 A_DATA_TYPE_DEFINE(1031)
#define A_FIXED8 A_DATA_TYPE_DEFINE(1032)
#define A_FIXED8_VEC2 A_DATA_TYPE_DEFINE(1033)
#define A_FIXED8_VEC3 A_DATA_TYPE_DEFINE(1034)
#define A_FIXED8_VEC4 A_DATA_TYPE_DEFINE(1035)
#define A_UFIXED8 A_DATA_TYPE_DEFINE(1036)
#define A_UFIXED8_VEC2 A_DATA_TYPE_DEFINE(1037)
#define A_UFIXED8_VEC3 A_DATA_TYPE_DEFINE(1038)
#define A_UFIXED8_VEC4 A_DATA_TYPE_DEFINE(1039)
#define A_FIXED16 A_DATA_TYPE_DEFINE(1040)
#define A_FIXED16_VEC2 A_DATA_TYPE_DEFINE(1041)
#define A_FIXED16_VEC3 A_DATA_TYPE_DEFINE(1042)
#define A_FIXED16_VEC4 A_DATA_TYPE_DEFINE(1043)
#define A_UFIXED16 A_DATA_TYPE_DEFINE(1044)
#define A_UFIXED16_VEC2 A_DATA_TYPE_DEFINE(1045)
#define A_UFIXED16_VEC3 A_DATA_TYPE_DEFINE(1046)
#define A_UFIXED16_VEC4 A_DATA_TYPE_DEFINE(1047)
#define A_FIXED32 A_DATA_TYPE_DEFINE(1048)
#define A_FIXED32_VEC2 A_DATA_TYPE_DEFINE(1049)
#define A_FIXED32_VEC3 A_DATA_TYPE_DEFINE(1050)
#define A_FIXED32_VEC4 A_DATA_TYPE_DEFINE(1051)
#define A_UFIXED32 A_DATA_TYPE_DEFINE(1052)
#define A_UFIXED32_VEC2 A_DATA_TYPE_DEFINE(1053)
#define A_UFIXED32_VEC3 A_DATA_TYPE_DEFINE(1054)
#define A_UFIXED32_VEC4 A_DATA_TYPE_DEFINE(1055)
#define A_FIXED64 A_DATA_TYPE_DEFINE(1056)
#define A_FIXED64_VEC2 A_DATA_TYPE_DEFINE(1057)
#define A_FIXED64_VEC3 A_DATA_TYPE_DEFINE(1058)
#define A_FIXED64_VEC4 A_DATA_TYPE_DEFINE(1059)
#define A_UFIXED64 A_DATA_TYPE_DEFINE(1060)
#define A_UFIXED64_VEC2 A_DATA_TYPE_DEFINE(1061)
#define A_UFIXED64_VEC3 A_DATA_TYPE_DEFINE(1062)
#define A_UFIXED64_VEC4 A_DATA_TYPE_DEFINE(1063)
#define A_FLOAT16 A_DATA_TYPE_DEFINE(1064)
#define A_FLOAT16_VEC2 A_DATA_TYPE_DEFINE(1065)
#define A_FLOAT16_VEC3 A_DATA_TYPE_DEFINE(1066)
#define A_FLOAT16_VEC4 A_DATA_TYPE_DEFINE(1067)
#define A_FLOAT32 A_DATA_TYPE_DEFINE(1068)
#define A_FLOAT32_VEC2 A_DATA_TYPE_DEFINE(1069)
#define A_FLOAT32_VEC3 A_DATA_TYPE_DEFINE(1070)
#define A_FLOAT32_VEC4 A_DATA_TYPE_DEFINE(1071)
#define A_FLOAT64 A_DATA_TYPE_DEFINE(1072)
#define A_FLOAT64_VEC2 A_DATA_TYPE_DEFINE(1073)
#define A_FLOAT64_VEC3 A_DATA_TYPE_DEFINE(1074)
#define A_FLOAT64_VEC4 A_DATA_TYPE_DEFINE(1075)
#define A_UFIXED8_RGBA_SRGB A_DATA_TYPE_DEFINE(2003)
#define A_UFIXED8_RGB_SRGB A_DATA_TYPE_DEFINE(2002)
#define A_UFIXED8_RA_SRGB A_DATA_TYPE_DEFINE(2001)
#define A_UFIXED8_R_SRGB A_DATA_TYPE_DEFINE(2000)
#define A_INT32_BOX1 A_DATA_TYPE_DEFINE(2004)
#define A_INT32_BOX2 A_DATA_TYPE_DEFINE(2005)
#define A_INT32_BOX3 A_DATA_TYPE_DEFINE(2006)
#define A_INT32_BOX4 A_DATA_TYPE_DEFINE(2007)
#define A_FLOAT32_BOX1 A_DATA_TYPE_DEFINE(2008)
#define A_FLOAT32_BOX2 A_DATA_TYPE_DEFINE(2009)
#define A_FLOAT32_BOX3 A_DATA_TYPE_DEFINE(2010)
#define A_FLOAT32_BOX4 A_DATA_TYPE_DEFINE(2011)
#define A_FLOAT64_BOX1 A_DATA_TYPE_DEFINE(2208)
#define A_FLOAT64_BOX2 A_DATA_TYPE_DEFINE(2209)
#define A_FLOAT64_BOX3 A_DATA_TYPE_DEFINE(2210)
#define A_FLOAT64_BOX4 A_DATA_TYPE_DEFINE(2211)
#define A_UINT64_REGION1 A_DATA_TYPE_DEFINE(2104)
#define A_UINT64_REGION2 A_DATA_TYPE_DEFINE(2105)
#define A_UINT64_REGION3 A_DATA_TYPE_DEFINE(2106)
#define A_UINT64_REGION4 A_DATA_TYPE_DEFINE(2107)
#define A_FLOAT32_MAT2 A_DATA_TYPE_DEFINE(2012)
#define A_FLOAT32_MAT3 A_DATA_TYPE_DEFINE(2013)
#define A_FLOAT32_MAT4 A_DATA_TYPE_DEFINE(2014)
#define A_FLOAT32_MAT2x3 A_DATA_TYPE_DEFINE(2015)
#define A_FLOAT32_MAT3x4 A_DATA_TYPE_DEFINE(2016)
#define A_FLOAT32_QUAT_IJKW A_DATA_TYPE_DEFINE(2017)
#define A_FRAME_COMPLETION_CALLBACK A_DATA_TYPE_DEFINE(203)
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
	typedef void* aArray;		// 数组
	typedef void* aArray1D;		// 一维数组
	typedef void* aArray2D;		// 二维数组
	typedef void* aArray3D;		// 三维数组
	typedef void* aFrame;		// 帧：渲染帧、计算帧
	typedef void* aGeometry;	// 几何体
	typedef void* aGroup;		// 组
	typedef void* aInstance;	// 实例
	typedef void* aLight;		// 光源
	typedef void* aMaterial;	// 材质
	typedef void* aPipeline;	// 渲染管线
	typedef void* aPipelineCS;	// 计算管线
	typedef void* aSampler;		// 纹理采样器
	typedef void* aSurface;		// 表面
	typedef void* aRenderer;	// 渲染器
	typedef void* aWorld;		// 世界
	enum obj_type_e
	{
		OBJ_DEVICE,
		OBJ_CAMERA,
		OBJ_ARRAY, OBJ_ARRAY1D, OBJ_ARRAY2D, OBJ_ARRAY3D,
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

	struct adevice3_t
	{
		void* ctx = 0;
		aDevice(*new_device)(void* ctx, void* phy, void* dev, const char* devname);
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
		int (*get_property)(aObject object, const char* name, int data_type, void* mem, uint64_t size, int wait_mask);
		// 提交帧渲染
		void (*render_frame)(aFrame frame);
		// 帧渲染完成检查
		int (*frame_ready)(aFrame frame, int wait_mask);
	};
	/*
	*	void* inst, void* phy, void* dev可选
	* 	set_param(); 着色器编译缓存目录const char* shaderLibDir = 0, const char* shaderCacheDir = 0,
	*	devname可以显卡名
	*/
	adevice3_t* new_gdev(void* inst);
	// 删除渲染器
	void free_gdev(adevice3_t* p);

	void gpu_vk_test0();

#ifdef __cplusplus
}
#endif


