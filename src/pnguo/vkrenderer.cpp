/*
--------------------------------------------------------------------------------------------------------------
vk渲染器

创建日期：2024-07-16
https://github.com/huahua369/pnguo
vk渲染流程:
	0.创建实例，枚举设备，选择物理设备，创建逻辑设备
	1.创建渲染目标：窗口交换链或纹理、fbo
	2.创建管线、状态等
	3.分配vbo/ibo/纹理/ubo等资源
	4.更新ubo、纹理、vbo等资源
	5.渲染命令
	6.提交列队
--------------------------------------------------------------------------------------------------------------
2D渲染：
	底层：三角形数据、纹理、混合模式
	前端：纹理、矩形数据、其他参数
	gui：矢量图、文本、位图
		 矢量图使用vkvg渲染到msaa纹理，再跟位图/文本纹理一起推送到SDL渲染
	控件：flex布局
3D渲染：
	场景管理：json、gltf/glb
	对象：渲染对象创建、管理
	控制：transform、参数控制

--------------------------------------------------------------------------------------------------------------
 todo 变形动画实现：
  "targets": [
	 {
	 "NORMAL": 33,
	 "POSITION": 32,
	 "TANGENT": 34
	 },
	 {
	 "NORMAL": 43,
	 "POSITION": 42,
	 "TANGENT": 44
	 }
 ],
 "weights": [0, 0.5]//默认权重

	float u_morphWeights[WEIGHT_COUNT];//插值权重数据
	for(int i = 0; i < WEIGHT_COUNT; i++)
	{
		vec4 displacement = getDisplacement(vertexID, MORPH_TARGET_POSITION_OFFSET + i * vertex_count);// 顶点目标坐标：使用顶点id获取
		pos += u_morphWeights[i] * displacement;
	}
定义常量
ID_MORPHING_DATA 绑定binding
ID_TARGET_DATA绑定ssbo的binding，统一转换成vec4数组
vertex_count顶点数量
WEIGHT_COUNT变形插值数量
MORPH_TARGET_POSITION_OFFSET	坐标变形
MORPH_TARGET_NORMAL_OFFSET		法线变形
MORPH_TARGET_TANGENT_OFFSET		切线变形
MORPH_TARGET_TEXCOORD_0_OFFSET	UV变形
MORPH_TARGET_TEXCOORD_1_OFFSET	UV变形同上
MORPH_TARGET_COLOR_0_OFFSET		颜色变形
MORPH_TARGET_COLOR_1_OFFSET		颜色变形


VK_PRESENT_MODE_IMMEDIATE_KHR: 应用程序提交的图像被立即传输到屏幕呈现，这种模式可能会造成撕裂效果。
VK_PRESENT_MODE_FIFO_KHR: 交换链被看作一个队列，当显示内容需要刷新的时候，显示设备从队列的前面获取图像，并且程序将渲染完成的图像插入队列的后面。如果队列是满的程序会等待。这种规模与视频游戏的垂直同步很类似。显示设备的刷新时刻被成为“垂直中断”。
VK_PRESENT_MODE_FIFO_RELAXED_KHR: 该模式与上一个模式略有不同的地方为，如果应用程序存在延迟，即接受最后一个垂直同步信号时队列空了，将不会等待下一个垂直同步信号，而是将图像直接传送。这样做可能导致可见的撕裂效果。
VK_PRESENT_MODE_MAILBOX_KHR: 这是第二种模式的变种。当交换链队列满的时候，选择新的替换旧的图像，从而替代阻塞应用程序的情形。这种模式通常用来实现三重缓冲区，与标准的垂直同步双缓冲相比，它可以有效避免延迟带来的撕裂效果。

gltf流程
// 如果存在任何透射式可绘制对象，则将所有不透明和透明的可绘制对象渲染到单独的帧缓冲区中。opaqueRenderTexture
if(KHR_materials_transmission透明介质透射材质){
	渲染环境
	渲染不透明物体opaqueDrawables
	渲染透明物体transparentDrawables
}

渲染环境
渲染不透明物体opaqueDrawables
渲染透明介质透射transmissionDrawables 需要opaqueRenderTexture纹理
渲染透明物体transparentDrawables

--------------------------------------------------------------------------------------------------------------
*/


#include "pch1.h"

#define USE_VMA
#include <vulkan/vulkan.h>
#ifdef _WIN32
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#include <DXGIFormat.h>
#include <D3DCompiler.h>
#include <dxcapi.h>
#include <Shlobj.h>
#include <wrl/client.h>
#elif __ANDROID__
#include <vulkan/vulkan_android.h>
#endif
#ifndef NOT_VULKAN
#include <vkh.h>
#ifdef USE_VMA

#define VMA_DEBUG_LOGa(format, ...) do { \
	   printf(format, __VA_ARGS__); \
	   printf("\n"); \
   } while(false)

//#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#endif
#ifndef DXGI_FORMAT_DEFINED
typedef uint32_t DXGI_FORMAT;
#endif 
#define TINYGLTF_IMPLEMENTATION 
#if 0
#define TINYGLTF_USE_RAPIDJSON
// 多线程初始化文档时会崩
#define TINYGLTF_NO_INCLUDE_RAPIDJSON
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#endif
#ifdef min
#undef min
#undef max
#endif // min
#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_ENABLE_DRACO
#include <tiny_gltf.h>
#include <zlib.h>
#include <queue>
#include <unordered_set>
// 本地头文件
#include <event.h>
#include <print_time.h>
#include <mapView.h>

#include "vkrenderer.h"



namespace vkr
{
	// Define a maximum number of shadows supported in a scene (note, these are only for spots and directional)
	static const uint32_t MaxLightInstances = 4;
	static const uint32_t MaxShadowInstances = 32;
	class Matrix2
	{
		glm::mat4 m_current = {};
		glm::mat4 m_previous = {};
		//glm::mat4 m_current = {};//glm::mat4::identity();
		//glm::mat4 m_previous = {};//glm::mat4::identity();
	public:
		void Set(const glm::mat4& m);
		glm::mat4 GetCurrent() const { return m_current; }
		glm::mat4 GetPrevious() const { return m_previous; }
	};

	//
	// Structures holding the per frame constant buffer data. 
	//
	struct Light
	{
		glm::mat4   mLightViewProj;
		glm::mat4   mLightView;

		float         direction[3];
		float         range;

		float         color[3];
		float         intensity;

		float         position[3];
		float         innerConeCos;

		float         outerConeCos;
		uint32_t      type;
		float         depthBias;
		int32_t       shadowMapIndex = -1;
	};


	const uint32_t LightType_Directional = 0;
	const uint32_t LightType_Point = 1;
	const uint32_t LightType_Spot = 2;

	struct PerFrame_t0
	{
		glm::mat4 mCameraCurrViewProj;
		glm::mat4 mCameraPrevViewProj;
		glm::mat4  mInverseCameraCurrViewProj;
		glm::vec4  cameraPos;
		float     iblFactor;
		float     emmisiveFactor;
		float     invScreenResolution[2];

		glm::vec4 wireframeOptions;
		float     lodBias = 0.0f;
		float	  oit_w = 0.0f;
		float	  oit_k = 0.0f;
		int		  lightCount;
		Light     lights[0];
	};
	struct PerFrame_t
	{
		glm::mat4 mCameraCurrViewProj;
		glm::mat4 mCameraPrevViewProj;
		glm::mat4 mInverseCameraCurrViewProj;
		glm::vec4 cameraPos;
		float     iblFactor;
		float     emmisiveFactor;
		float     invScreenResolution[2];

		glm::vec4 wireframeOptions;
		float     lodBias = 0.0f;
		float	  oit_w = 0.0f;
		float	  oit_k = 0.0f;
		int		  lightCount;
		Light     lights[MaxLightInstances];
	};


	struct camera_info
	{
		glm::quat qt = {};
		glm::vec3 camera_position;
		glm::vec3 camera_look_at;
		glm::vec3 camera_position_delta;//in
		glm::vec3 camera_direction;	// in
		glm::vec3 camera_up;		// in 
		float pitch;			// in
		float yaw;		// in
		float _distance = 5.0;			// old dist
		float distance;		// in
		glm::vec2 yp = {};
	};

	class Camera
	{
	public:
		Camera();
		void SetMatrix(const glm::mat4& cameraMatrix);
		void set_mat(const glm::mat4& cameraMatrix, const glm::vec3& e);
		void LookAt(const glm::vec4& eyePos, const glm::vec4& lookAt);
		void LookAt(const glm::vec3& eyePos, const glm::vec3& lookAt);
		void LookAt(float yaw, float pitch, float distance, const glm::vec4& at);
		void SetFov(float fov, uint32_t width, uint32_t height, float nearPlane, float farPlane);
		void SetFov(float fov, float aspectRatio, float nearPlane, float farPlane);
		void UpdateCameraPolar(float yaw, float pitch, float x, float y, float distance);
		void UpdateCameraWASD(float yaw, float pitch, const bool keyDown[256], double deltaTime);

		glm::mat4 GetView() const { return m_View; }
		glm::mat4 GetPrevView() const { return m_PrevView; }
		glm::vec4 GetPosition() const { return m_eyePos; }


		glm::vec4 GetDirection()    const { return glm::vec4(glm::vec3(glm::transpose(m_View) * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)), 0); }
		glm::vec4 GetUp()           const { return glm::vec4(glm::vec3(glm::transpose(m_View) * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)), 0); }
		glm::vec4 GetSide()         const { return glm::vec4(glm::vec3(glm::transpose(m_View) * glm::vec4(1.0f, 1.0f, 0.0f, 0.0f)), 0); }
		glm::mat4 GetProjection()   const { return m_Proj; }

		float GetFovH() const { return m_fovH; }
		float GetFovV() const { return m_fovV; }

		float GetAspectRatio() const { return m_aspectRatio; }

		float GetNearPlane() const { return m_near; }
		float GetFarPlane() const { return m_far; }

		float GetYaw() const { return m_yaw; }
		float GetPitch() const { return m_pitch; }
		float GetDistance() const { return m_distance; }

		void SetSpeed(float speed) { m_speed = speed; }
		void SetProjectionJitter(float jitterX, float jitterY);
		void SetProjectionJitter(uint32_t width, uint32_t height, uint32_t& seed);
		void UpdatePreviousMatrices() { m_PrevView = m_View; }

	private:
		glm::mat4       m_View = {};
		glm::mat4       m_Proj = {};
		glm::mat4       m_PrevView = {};
		glm::vec4       m_eyePos = {};
		glm::vec4       atPos = {};
		glm::quat		mqt = {};
		glm::vec3		_axis = {};
		float			_angle = 0;
		camera_info		pca = {};
		float               m_distance = 0.0f;
		float               m_fovV = 0.0f, m_fovH = 0.0f;
		float               m_near = 0.0f, m_far = 0.0f;
		float               m_aspectRatio = 0.0f;

		float               m_speed = 1.0f;
		float               m_yaw = 0.0f;
		float               m_pitch = 0.0f;
		float               m_roll = 0.0f;
	public:
		glm::vec3	ryp = {};
		bool flipY = false;
		bool is_eulerAngles = true;
	};


	class TaskQueue
	{
	public:
		std::vector<std::jthread> workers;
		std::queue<std::function<void()>> tasks;
		std::mutex mtx;
		size_t w_count = 0;
	public:
		TaskQueue(size_t count);
		~TaskQueue();
		void add(std::function<void()> task);
		void run(int num);
		void wait_stop();
	private:
	};




	//  VK_KHR_shader_float16_int8
	//	VK_KHR_16bit_storage
	//	VK_KHR_8bit_storage 

	std::string format(const char* format, ...);
	inline std::string to_string_g(double _Val) {
		const auto _Len = static_cast<size_t>(_scprintf("%g", _Val));
		std::string _Str(_Len, '\0');
		sprintf_s(&_Str[0], _Len + 1, "%g", _Val);
		return _Str;
	}

	void vkr_image_set_layout(VkCommandBuffer cmdBuff, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkImageLayout new_image_layout, VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages);

	// helpers functions to use debug markers
	void ExtDebugUtilsGetProcAddresses(VkDevice device);
	void SetResourceName(VkDevice device, VkObjectType objectType, uint64_t handle, const char* name);
	void SetPerfMarkerBegin(VkCommandBuffer cmd_buf, const char* name);
	void SetPerfMarkerEnd(VkCommandBuffer cmd_buf);

	void DestroyShadersInTheCache(VkDevice device);

	uint32_t SizeOfFormat(VkFormat format);

#define HASH_SEED 2166136261

	size_t Hash(const void* ptr, size_t size, size_t result = HASH_SEED);
	size_t Hash_p(const size_t* ptr, size_t size, size_t result = HASH_SEED);
	size_t HashString(const char* str, size_t result = HASH_SEED);
	size_t HashString(const std::string& str, size_t result = HASH_SEED);
	size_t HashInt(const int type, size_t result = HASH_SEED);
	size_t HashFloat(const float type, size_t result = HASH_SEED);
	size_t HashPtr(const void* type, size_t result = HASH_SEED);

	size_t HashShaderString(const char* pRootDir, const char* pShader, size_t result = 2166136261);


#if 1


	/* todo 相机
	  [x]第一人称相机
	  [ ]第三人称相机
	  [ ]场景漫游相机
	*/
	class CameraX
	{
	public:
		glm::mat4 view;                         //视图
		glm::dvec3 pos = { 0,2.8,0 };                         //实际坐标,计算时必须用glm::floor来偏移出正确结果,不偏移的话xyz任意一条为0时其上下两部分为0.5和-0.5,此时使用(int)强转结果都为0
		glm::vec3 front = { 1.0, 1.0, 1.0 };	//相机前向向量  
		glm::vec3 rota = {};                         //位置角度
		glm::vec3 worldUp = { 0.0, 1.0, 0.0 };    //y轴做世界坐标系法向量 
		glm::vec2 size = { 1.0f, 1.0f }; //视口大小
		glm::quat qt = {};
		glm::quat src_qt = {};
		glm::quat dst_qt = {};
		double qtime = 0;		//插值时间
		double qmaxtime = 0.1;	//最大插值时间
		// 相机参数 
		glm::vec3 cameraPos = glm::vec3(0.0f, 1.5f, 5.0f);       // 相机初始位置（玩家后方5米，上方1.5米） 
		float cameraDistance = 5.0f;                             // 相机与玩家的距离 
		float cameraHeight = 1.5f;                               // 相机垂直高度
		bool bFirstPerson = false;	// 是否第一人称视角
		float fixheight = 0;		// 0就是固定高度
		CameraX() {
			view = glm::lookAt(glm::vec3(0)/*摄像机坐标*/, glm::vec3(0)/*被观测坐标*/, worldUp);
			qt = dst_qt = e2q(rota);
			mouseMovement(0, 0, 0, 1);                //不初始化的话，开局不移动没画面
		}

	private:
		//float keySpeed = 1.0f;    //键盘移动速率,最好不要高过64
		float keySpeed = 5.0f;
		float xMouseSpeed = 0.51f;   //鼠标移动X速率
		float yMouseSpeed = 0.81f;   //鼠标移动Y速率

		// auto zz = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f))) * (moveSpeed * direction.x);
	public:
		void set_size(float w, float h) {
			size.x = w;
			size.y = h;
			xMouseSpeed = h / 360.0 * 0.15;
			yMouseSpeed = w / 360.0 * 0.25;
		}

		// 输入角度欧拉角，计算四元数向量
		glm::vec3 cfront(const glm::ivec2& r)
		{
			auto rota_rad = glm::radians(glm::vec2(r));
			glm::vec3 f = {};
			f.x = glm::cos(rota_rad.x) * glm::cos(rota_rad.y);
			f.y = glm::sin(rota_rad.x);
			f.z = glm::cos(rota_rad.x) * glm::sin(rota_rad.y);
			return f;
		}
		glm::quat e2q(const glm::ivec2& r)
		{
			auto ry = glm::radians(glm::vec2(r));
			glm::vec3 EulerAngles(ry.x, ry.y, 0.0);// x=pitch; y=yaw； roll忽略
			glm::quat q = glm::quat(EulerAngles);
			return q;
		}
		//	正弦函数y=sin(a)和余弦函数x=cos(a)
		//	正切函数tan(a) = sin(a) / cos(a)
		glm::vec3 quat_up(const glm::quat& q)
		{
			auto v = q * glm::vec3(0, 1, 0);
			return glm::vec3(v.z, v.y, v.x);
		}
		glm::vec3 quat_right(const glm::quat& q)
		{
			auto v = q * glm::vec3(1, 0, 0);
			return glm::vec3(v.z, v.y, v.x);
		}
		glm::vec3 quat_forward(const glm::quat& q)
		{
			auto v = q * glm::vec3(0, 0, 1);
			return glm::vec3(v.z, -v.y, v.x);
		}
		//键盘移动处理
		void keyMovement(glm::vec3 direction, double deltaTime) {
			float moveSpeed = deltaTime * keySpeed;
			auto z = worldUp * direction.z;
			// 第一人称相机计算
#if 1 
			auto x = -quat_right(qt);	// 左右移动
			auto y = -quat_forward(qt);	// 前后移动
			//auto z0 = quat_up(qi); 
			auto npos = moveSpeed * (x * direction.x + y * direction.y + z);
#else
			auto front1 = cfront(rota);
			auto front0 = -front1;// 获取和摄像机前向向量相反的向量
			auto right = glm::normalize(glm::cross(front0, worldUp));
			//根据摄像机坐标系下的移动方向进行移动
			auto npos1 = moveSpeed * (direction.x * right + z + direction.y * front0);
#endif
			if (abs(direction.x) > 0 || abs(direction.y) > 0)
				npos.y *= fixheight;
			pos += npos;
			updateView();
		}

		//鼠标移动处理
		void mouseMovement(float deltaX, float deltaY, double deltaTime, bool mousedown)
		{
			if (!bFirstPerson && !mousedown)
			{
				return; // 非第一人称视角且鼠标未按下时不处理鼠标移动
			}
			//计算摄像机的前向向量
			//pitch()：俯仰，将物体绕X轴旋转
			//yaw()：航向，将物体绕Y轴旋转
			//roll()：横滚，将物体绕Z轴旋转 
			// 计算前向向量 
			//front = cfront(rota);
			if (abs(deltaX) > 0 || abs(deltaY) > 0)
			{
				auto sr = rota;
				// 角度配置
				rota.x += deltaY * xMouseSpeed;
				rota.y += deltaX * yMouseSpeed;
				//角度限制
				rota.y = glm::mod(rota.y, 360.0f);
				rota.x = glm::clamp(rota.x, -89.0f, 89.0f);
				src_qt = qt; // 保存当前四元数 
				dst_qt = e2q(rota);
				auto qd = glm::distance2(sr, rota);
				if (qd > 1)
				{
					qtime = 0;
				}
			}
			qt = dst_qt;
			front = quat_forward(qt);
			updateView();
			qtime += deltaTime;
		}

		void updateView() {
			glm::vec3 tpos = pos;
			glm::vec3 camera_forward;
			glm::vec3 camera_right;
			glm::vec3 camera_up;
			float cameraSmooth = 2.0f;	// 平滑因子  
			cameraPos = tpos;
			camera_forward = glm::normalize(front);
			camera_right = glm::normalize(glm::cross(camera_forward, worldUp));
			camera_up = glm::normalize(glm::cross(camera_right, camera_forward));
			if (!bFirstPerson)
			{
				cameraPos += camera_forward * cameraDistance + camera_up * cameraHeight;
			}
			auto view0 = glm::lookAt(cameraPos, front + cameraPos, camera_up);
			view = view0;
			//cameraPos = view[3];
			//printf("pos %.2f,%.2f,%.2f\t %.2f,%.2f,%.2f\t %.2f,%.2f\n", pos.x, pos.y, pos.z, front.x, front.y, front.z, rota.x, rota.y);
		}
	};

#endif //1




#if 1

	struct inspd_t {
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
	};
	class InstanceProperties
	{
		std::vector<VkLayerProperties> m_instanceLayerProperties;
		std::vector<VkExtensionProperties> m_instanceExtensionProperties;

		std::vector<const char*> m_instance_layer_names;
		std::vector<const char*> m_instance_extension_names;
		void* m_pNext = NULL;
	public:
		VkResult Init();
		bool AddInstanceLayerName(const char* instanceLayerName);
		bool AddInstanceExtensionName(const char* instanceExtensionName);
		void* GetNext() { return m_pNext; }
		void SetNewNext(void* pNext) { m_pNext = pNext; }

		void GetExtensionNamesAndConfigs(std::vector<const char*>* pInstance_layer_names, std::vector<const char*>* pInstance_extension_names);
	private:
		bool IsLayerPresent(const char* pExtName);
		bool IsExtensionPresent(const char* pExtName);
	};

	bool f_ExtDebugUtilsCheckInstanceExtensions(InstanceProperties* pDP);


	class DeviceProperties
	{
	public:
		VkPhysicalDevice m_physicaldevice;

		std::set<std::string> m_device_extension_names;

		std::vector<VkExtensionProperties> m_deviceExtensionProperties;


		void* m_pNext = NULL;
	public:
		VkResult Init(VkPhysicalDevice physicaldevice);
		bool IsExtensionPresent(const char* pExtName);
		bool has_extension(const char* pExtName);
		bool AddDeviceExtensionName(const char* deviceExtensionName);
		int AddDeviceExtensionName(std::vector<const char*> deviceExtensionName);
		void* GetNext() { return m_pNext; }
		void SetNewNext(void* pNext) { m_pNext = pNext; }

		VkPhysicalDevice GetPhysicalDevice() { return m_physicaldevice; }
		void GetExtensionNamesAndConfigs(std::vector<const char*>* pDevice_extension_names);
	private:
	};
	bool equal_sampler_info(const VkSamplerCreateInfo& a, const VkSamplerCreateInfo& b) {
		return a.sType == b.sType &&
			a.pNext == nullptr && b.pNext == nullptr && // 忽略 pNext
			a.flags == b.flags &&
			a.magFilter == b.magFilter &&
			a.minFilter == b.minFilter &&
			a.mipmapMode == b.mipmapMode &&
			a.addressModeU == b.addressModeU &&
			a.addressModeV == b.addressModeV &&
			a.addressModeW == b.addressModeW &&
			fabsf(a.mipLodBias - b.mipLodBias) < 1e-6f &&
			a.anisotropyEnable == b.anisotropyEnable &&
			fabsf(a.maxAnisotropy - b.maxAnisotropy) < 1e-6f &&
			a.compareEnable == b.compareEnable &&
			a.compareOp == b.compareOp &&
			fabsf(a.minLod - b.minLod) < 1e-6f &&
			fabsf(a.maxLod - b.maxLod) < 1e-6f &&
			a.borderColor == b.borderColor &&
			a.unnormalizedCoordinates == b.unnormalizedCoordinates;
	}
	struct sampler_kt {
		VkSamplerCreateInfo info = {};
		bool operator==(const sampler_kt& other) const {
			return equal_sampler_info(this->info, other.info);  // 自定义比较
		}
	};
	struct SamplerKeyHash {
		size_t operator()(const sampler_kt& k) const {
			return Hash_p((const size_t*)&k.info, sizeof(VkSamplerCreateInfo));
		}
	};



	class Device
	{
	public:
		VkInstance m_instance = 0;
		VkDevice m_device = 0;
		VkPhysicalDevice m_physicaldevice = {};
		VkPhysicalDeviceMemoryProperties m_memoryProperties = {};
		VkPhysicalDeviceProperties m_deviceProperties = {};
		VkPhysicalDeviceProperties2 m_deviceProperties2 = {};
		VkPhysicalDeviceSubgroupProperties m_subgroupProperties = {};
		VkSurfaceKHR m_surface = {};

		VkQueue present_queue = 0;
		uint32_t present_queue_family_index = 0;
		VkQueue graphics_queue = 0;
		uint32_t graphics_queue_family_index = 0;
		VkQueue compute_queue = 0;
		uint32_t compute_queue_family_index = 0;
		std::vector<VkSurfaceFormatKHR> _surfaceFormats;

		std::unordered_map<sampler_kt, VkSampler, SamplerKeyHash> _samplers;
		//std::vector<sampler_t> _samplers_v;

		PFN_vkCmdDrawMeshTasksEXT _vkCmdDrawMeshTasksEXT = { };
		PFN_vkCmdBeginRenderingKHR _vkCmdBeginRenderingKHR = {};
		PFN_vkCmdEndRenderingKHR _vkCmdEndRenderingKHR = {};
		PFN_vkCmdSetFrontFace _vkCmdSetFrontFace = {};

		bool m_usingValidationLayer = false;
		bool m_usingFp16 = false;
		bool m_rt10Supported = false;
		bool m_rt11Supported = false;
		bool m_vrs1Supported = false;
		bool m_vrs2Supported = false;
#ifdef USE_VMA
		VmaAllocator m_hAllocator = NULL;
#endif
	public:
		Device();
		~Device();
		void OnCreate(dev_info_cx* d, bool cpuValidationLayerEnabled, bool gpuValidationLayerEnabled, void* pw
			, const char* spdname, std::vector<std::string>* pdnv);
		void OnCreateEx(VkInstance vulkanInstance, VkPhysicalDevice physicalDevice, VkDevice dev, void* pw, DeviceProperties* pDp);
		void OnDestroy();
		VkDevice GetDevice() { return m_device; }
		VkQueue GetGraphicsQueue() { return graphics_queue; }
		uint32_t GetGraphicsQueueFamilyIndex() { return present_queue_family_index; }
		VkQueue GetPresentQueue() { return present_queue; }
		uint32_t GetPresentQueueFamilyIndex() { return graphics_queue_family_index; }
		VkQueue GetComputeQueue() { return compute_queue; }
		uint32_t GetComputeQueueFamilyIndex() { return compute_queue_family_index; }
		VkPhysicalDevice GetPhysicalDevice() { return m_physicaldevice; }
		VkSurfaceKHR GetSurface() { return m_surface; }
		void GetDeviceInfo(std::string* deviceName, std::string* driverVersion);
#ifdef USE_VMA
		VmaAllocator GetAllocator() { return m_hAllocator; }
#endif
		VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() { return m_memoryProperties; }
		VkPhysicalDeviceProperties GetPhysicalDeviceProperries() { return m_deviceProperties; }
		VkPhysicalDeviceSubgroupProperties GetPhysicalDeviceSubgroupProperties() { return m_subgroupProperties; }

		bool IsFp16Supported() { return m_usingFp16; };
		bool IsRT10Supported() { return m_rt10Supported; }
		bool IsRT11Supported() { return m_rt11Supported; }
		bool IsVRSTier1Supported() { return m_vrs1Supported; }
		bool IsVRSTier2Supported() { return m_vrs2Supported; }

		// pipeline cache
		VkPipelineCache m_pipelineCache = {};
		void CreatePipelineCache();
		void DestroyPipelineCache();
		VkPipelineCache GetPipelineCache();

		void CreateShaderCache();
		void DestroyShaderCache();

		void GPUFlush();

		VkSampler newSampler(const VkSamplerCreateInfo* pCreateInfo);

		void SetDescriptorSet(uint32_t index, VkImageView imageView, VkImageLayout imageLayout, VkSampler pSampler, VkDescriptorSet descriptorSet);
		void SetDescriptorSet(uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkImageLayout imageLayout, VkSampler pSampler, VkDescriptorSet descriptorSet);
		void SetDescriptorSet(uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet);
		void SetDescriptorSet(uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkSampler pSampler, VkDescriptorSet descriptorSet);
		void SetDescriptorSetForDepth(uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet);
		void SetDescriptorSet(uint32_t index, VkImageView imageView, VkDescriptorSet descriptorSet);
		void SetDescriptorSet1(VkBuffer buffer, int index, uint32_t pos, uint32_t size, VkDescriptorSet descriptorSet, uint32_t dt);

		VkDescriptorSetLayout newDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding);
		VkFramebuffer newFrameBuffer(VkRenderPass renderPass, const std::vector<VkImageView>* pAttachments, uint32_t Width, uint32_t Height);
	};

	bool memory_type_from_properties(VkPhysicalDeviceMemoryProperties& memory_properties, uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex);

#endif // 1


	class GltfPbrPass;
	class GltfBBoxPass;
	class GltfDepthPass;
	class gltf_gpu_res_cx;

	// todo renderer 

	struct robj_info {
		//gltf passes
		GltfPbrPass* m_GLTFPBR = nullptr;
		GltfBBoxPass* m_GLTFBBox = nullptr;
		GltfDepthPass* m_GLTFDepth = nullptr;
		gltf_gpu_res_cx* _ptb = nullptr;
		bool shadowMap = true;		// 是否有阴影
	};
	// todo cmdlr

	class CommandListRing
	{
	private:
		struct CommandBuffersPerFrame
		{
			VkCommandPool        m_commandPool;
			VkCommandBuffer* m_pCommandBuffer;
			uint32_t m_UsedCls;
		};
		std::vector<CommandBuffersPerFrame> m_pCommandBuffers;
		CommandBuffersPerFrame* m_pCurrentFrame = 0;
		Device* m_pDevice = 0;
		uint32_t m_frameIndex = 0;
		uint32_t m_numberOfAllocators = 0;
		uint32_t m_commandListsPerBackBuffer = 0;
	public:
		void OnCreate(Device* pDevice, uint32_t numberOfBackBuffers, uint32_t commandListsPerframe, bool compute = false);
		void OnDestroy();
		void OnBeginFrame();
		VkCommandBuffer GetNewCommandList();
		//VkCommandPool GetPool() { return m_pCommandBuffers[0].m_commandPool; }
	};

	struct fbo_cxt {
		VkRenderPass renderPass = 0;
		VkFramebuffer framebuffer = 0;
		VkFence fence = {};
		VkSemaphore sem = {};
		VkImage image = 0;
	};
	// 
	class AsyncPool;
	class GltfDepthPass;
	struct PBRPrimitives;

	struct BatchList
	{
		float m_depth = 0;
		int frontFace = 0;
		PBRPrimitives* m_pPrimitive;
		VkDescriptorBufferInfo m_perFrameDesc;
		VkDescriptorBufferInfo m_perObjectDesc;
		VkDescriptorBufferInfo* m_uvtDesc = 0;
		VkDescriptorBufferInfo* m_pPerSkeleton = 0;
		VkDescriptorBufferInfo* morph = 0;
		VkDescriptorBufferInfo* instance_info = 0;
		operator float() { return -m_depth; }
	};
	struct drawables_t
	{
		std::vector<BatchList> opaque, transparent, transmission;
		std::vector<BatchList> wireframe[3];
		void clear()
		{
			opaque.clear();
			transparent.clear();
			transmission.clear();
			for (int i = 0; i < 3; i++)
				wireframe[i].clear();
		}
	};

	struct TimeStamp
	{
		std::string m_label;
		float       m_microseconds;
	};


	class Ring
	{
	public:
		void Create(uint32_t TotalSize)
		{
			m_Head = 0;
			m_AllocatedSize = 0;
			m_TotalSize = TotalSize;
		}

		uint32_t GetSize() { return m_AllocatedSize; }
		uint32_t GetHead() { return m_Head; }
		uint32_t GetTail() { return (m_Head + m_AllocatedSize) % m_TotalSize; }

		//helper to avoid allocating chunks that wouldn't fit contiguously in the ring
		uint32_t PaddingToAvoidCrossOver(uint32_t size)
		{
			int tail = GetTail();
			if ((tail + size) > m_TotalSize)
				return (m_TotalSize - tail);
			else
				return 0;
		}

		bool Alloc(uint32_t size, uint32_t* pOut)
		{
			if (m_AllocatedSize + size <= m_TotalSize)
			{
				if (pOut)
					*pOut = GetTail();
				if (size % 64)
					size = size;
				m_AllocatedSize += size;
				return true;
			}

			assert(false);
			return false;
		}

		bool Free(uint32_t size)
		{
			if (m_AllocatedSize >= size)
			{
				m_Head = (m_Head + size) % m_TotalSize;
				m_AllocatedSize -= size;
				return true;
			}
			return false;
		}
	private:
		uint32_t m_Head;
		uint32_t m_AllocatedSize;
		uint32_t m_TotalSize;
	};

	// 
	// This class can be thought as ring buffer inside a ring buffer. The outer ring is for , 
	// the frames and the internal one is for the resources that were allocated for that frame.
	// The size of the outer ring is typically the number of back buffers.
	//
	// When the outer ring is full, for the next allocation it automatically frees the entries 
	// of the oldest frame and makes those entries available for the next frame. This happens 
	// when you call 'OnBeginFrame()' 
	//
	class RingWithTabs
	{
	public:

		void OnCreate(uint32_t numberOfBackBuffers, uint32_t memTotalSize)
		{
			m_backBufferIndex = 0;
			m_numberOfBackBuffers = numberOfBackBuffers;

			//init mem per frame tracker
			m_memAllocatedInFrame = 0;
			for (int i = 0; i < 4; i++)
				m_allocatedMemPerBackBuffer[i] = 0;

			m_mem.Create(memTotalSize);
		}

		void OnDestroy()
		{
			m_mem.Free(m_mem.GetSize());
		}

		bool Alloc(uint32_t size, uint32_t* pOut)
		{
			uint32_t padding = m_mem.PaddingToAvoidCrossOver(size);
			if (padding > 0)
			{
				m_memAllocatedInFrame += padding;

				if (m_mem.Alloc(padding, NULL) == false) //alloc chunk to avoid crossover, ignore offset        
				{
					return false;  //no mem, cannot allocate apdding
				}
			}

			if (m_mem.Alloc(size, pOut) == true)
			{
				m_memAllocatedInFrame += size;
				return true;
			}
			return false;
		}

		void OnBeginFrame()
		{
			m_allocatedMemPerBackBuffer[m_backBufferIndex] = m_memAllocatedInFrame;
			m_memAllocatedInFrame = 0;

			m_backBufferIndex = (m_backBufferIndex + 1) % m_numberOfBackBuffers;

			// free all the entries for the oldest buffer in one go
			uint32_t memToFree = m_allocatedMemPerBackBuffer[m_backBufferIndex];
			m_mem.Free(memToFree);
		}
	private:
		//internal ring buffer
		Ring m_mem;

		//this is the external ring buffer (I could have reused the Ring class though)
		uint32_t m_backBufferIndex;
		uint32_t m_numberOfBackBuffers;

		uint32_t m_memAllocatedInFrame;
		uint32_t m_allocatedMemPerBackBuffer[4];
	};

	class DynamicBufferRing
	{
	public:
		VkResult OnCreate(Device* pDevice, uint32_t numberOfBackBuffers, uint32_t memTotalSize, char* name = NULL);
		void OnDestroy();
		bool AllocConstantBuffer(uint32_t size, void** pData, VkDescriptorBufferInfo* pOut);
		bool AllocConstantBuffer1(uint32_t size, void** pData, VkDescriptorBufferInfo* pOut, uint32_t algin);
		VkDescriptorBufferInfo AllocConstantBuffer(uint32_t size, void* pData);

		bool AllocBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, const void* pInitData, VkDescriptorBufferInfo* pOut);
		bool AllocVertexBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void** pData, VkDescriptorBufferInfo* pOut);
		bool AllocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void** pData, VkDescriptorBufferInfo* pOut);
		void OnBeginFrame();
		void SetDescriptorSet(int i, uint32_t size, VkDescriptorSet descriptorSet, uint32_t dt = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

	private:
		Device* m_pDevice;
		uint32_t        m_memTotalSize;
		RingWithTabs    m_mem;
		char* m_pData = nullptr;
		VkBuffer        m_buffer;

#ifdef USE_VMA
		VmaAllocation   m_bufferAlloc = VK_NULL_HANDLE;
#else
		VkDeviceMemory  m_deviceMemory = VK_NULL_HANDLE;
#endif
	};
	class StaticBufferPool
	{
	public:
		VkResult OnCreate(Device* pDevice, uint32_t totalMemSize, bool bUseVidMem, const char* name);
		void OnDestroy();

		// Allocates a IB/VB and returns a pointer to fill it + a descriptot
		//
		bool AllocBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void** pData, VkDescriptorBufferInfo* pOut);

		// Allocates a IB/VB and fill it with pInitData, returns a descriptor
		//
		bool AllocBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, const void* pInitData, VkDescriptorBufferInfo* pOut);

		// if using vidmem this kicks the upload from the upload heap to the video mem
		void UploadData(VkCommandBuffer cmd_buf);

		// if using vidmem frees the upload heap
		void FreeUploadHeap();

	private:
		Device* m_pDevice;

		std::mutex       m_mutex = {};

		bool             m_bUseVidMem = true;

		char* m_pData = nullptr;
		uint32_t         m_memOffset = 0;
		uint32_t         m_totalMemSize = 0;

		VkBuffer         m_buffer;
		VkBuffer         m_bufferVid;

#ifdef USE_VMA
		VmaAllocation    m_bufferAlloc = VK_NULL_HANDLE;
		VmaAllocation    m_bufferAllocVid = VK_NULL_HANDLE;
#else
		VkDeviceMemory   m_deviceMemory = VK_NULL_HANDLE;;
		VkDeviceMemory   m_deviceMemoryVid = VK_NULL_HANDLE;;
#endif 
	};

	// set管理
	class ResourceViewHeaps
	{
	public:
		void OnCreate(Device* pDevice, uint32_t cbvDescriptorCount, uint32_t srvDescriptorCount, uint32_t uavDescriptorCount, uint32_t samplerDescriptorCount);
		void OnDestroy();
		bool AllocDescriptor(VkDescriptorSetLayout descriptorLayout, VkDescriptorSet* pDescriptor);
		bool AllocDescriptor(int size, const VkSampler* pSamplers, VkDescriptorSetLayout* descriptorLayout, VkDescriptorSet* pDescriptor);
		bool AllocDescriptor(std::vector<uint32_t>& descriptorCounts, const VkSampler* pSamplers, VkDescriptorSetLayout* descriptorLayout, VkDescriptorSet* pDescriptor);
		bool CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding, VkDescriptorSetLayout* pDescSetLayout);
		bool CreateDescriptorSetLayoutAndAllocDescriptorSet(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding, VkDescriptorSetLayout* descriptorLayout, VkDescriptorSet* pDescriptor);
		void FreeDescriptor(VkDescriptorSet descriptorSet);
	private:
		Device* m_pDevice;
		VkDescriptorPool m_descriptorPool;
		std::mutex       m_mutex;
		int              m_allocatedDescriptorCount = 0;
	};

	class GBuffer;
	class dvk_texture;
	class Sync
	{
		int m_count = 0;
		std::mutex m_mutex;
		std::condition_variable condition;
	public:
		int Inc()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_count++;
			return m_count;
		}

		int Dec()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_count--;
			if (m_count == 0)
				condition.notify_all();
			return m_count;
		}

		int Get()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			return m_count;
		}

		void Reset()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_count = 0;
			condition.notify_all();
		}

		void Wait()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			while (m_count != 0)
				condition.wait(lock);
		}

	};

#if 0
	class Async
	{
		static int s_activeThreads;
		static int s_maxThreads;
		static std::mutex s_mutex;
		static std::condition_variable s_condition;
		static bool s_bExiting;

		std::function<void()> m_job;
		Sync* m_pSync;
		std::thread* m_pThread;

	public:
		Async(std::function<void()> job, Sync* pSync = NULL);
		~Async();
		static void Wait(Sync* pSync);
	};

	class AsyncPool
	{
		std::vector<Async*> m_pool;
	public:
		~AsyncPool();
		void Flush();
		void AddAsyncTask(std::function<void()> job, Sync* pSync = NULL);
	};

	void ExecAsyncIfThereIsAPool(AsyncPool* pAsyncPool, std::function<void()> job);
#endif



#define CACHE_ENABLE
	//#define CACHE_LOG 
	template<typename T>
	class Cache
	{
	public:
		struct CacheEntry
		{
			Sync m_Sync;
			T m_data;
		};
		typedef std::map<size_t, CacheEntry> DatabaseType;
	private:
		DatabaseType m_database;
		std::mutex m_mutex;
	public:
		void Async_Wait(Sync* pSync)
		{
			if (pSync->Get() == 0)
				return;
			pSync->Wait();
		}
		bool CacheMiss(size_t hash, T* pOut)
		{
#ifdef CACHE_ENABLE
			//DatabaseType::iterator it;
			CacheEntry* kt = {};
			// find whether the shader is in the cache, create an empty entry just so other threads know this thread will be compiling the shader
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				auto it = m_database.find(hash);

				// shader not found, we need to compile the shader!
				if (it == m_database.end())
				{
#ifdef CACHE_LOG
					Trace(format("thread 0x%04x Compi Begin: %p %i\n", GetCurrentThreadId(), hash, m_database[hash].m_Sync.Get()));
#endif
					// inc syncing object so other threads requesting this same shader can tell there is a compilation in progress and they need to wait for this thread to finish.
					m_database[hash].m_Sync.Inc();
					return true;
				}
				kt = &it->second;
			}
			// If we have seen these shaders before then:
			{
				// If there is a thread already trying to compile this shader then wait for that thread to finish
				if (kt->m_Sync.Get() == 1)
				{
#ifdef CACHE_LOG
					Trace(format("thread 0x%04x Wait: %p %i\n", GetCurrentThreadId(), hash, kt->m_Sync.Get()));
#endif
					Async_Wait(&kt->m_Sync);
				}

				// if the shader was compiled then return it
				*pOut = kt->m_data;

#ifdef CACHE_LOG
				Trace(format("thread 0x%04x Was cache: %p \n", GetCurrentThreadId(), hash));
#endif
				return false;
			}
#endif
			return true;
		}

		void UpdateCache(size_t hash, T* pValue)
		{
#ifdef CACHE_ENABLE
			// DatabaseType::iterator it;
			CacheEntry* kt = {};
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				auto it = m_database.find(hash);
				assert(it != m_database.end());
				if (it == m_database.end())return;
				kt = &it->second;
			}
#ifdef CACHE_LOG
			Trace(format("thread 0x%04x Compi End: %p %i\n", GetCurrentThreadId(), hash, kt->m_Sync.Get()));
#endif
			kt->m_data = *pValue;
			//assert(kt->m_Sync.Get() == 1);
			// The shader has been compiled, set sync to 0 to indicate it is compiled
			// This also wakes up all the threads waiting on  Async::Wait(&kt->m_Sync);
			kt->m_Sync.Dec();
#endif
			// CACHE_ENABLE 1
		}
		template<typename Func>
		void ForEach(Func func)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto it = m_database.begin(); it != m_database.end(); ++it)
			{
				func(it);
			}
		}
	};






	class UploadHeap
	{
		Sync allocating, flushing;
		struct COPY_t
		{
			VkImage m_image; VkBufferImageCopy m_bufferImageCopy;
		};
		std::vector<COPY_t> m_copies;

		std::vector<VkImageMemoryBarrier> m_toPreBarrier;
		std::vector<VkImageMemoryBarrier> m_toPostBarrier;

		std::mutex m_mutex;
	public:
		void OnCreate(Device* pDevice, SIZE_T uSize);
		void OnDestroy();

		UINT8* Suballocate(SIZE_T uSize, UINT64 uAlign);
		UINT8* BeginSuballocate(SIZE_T uSize, UINT64 uAlign);
		void EndSuballocate();
		UINT8* BasePtr() { return m_pDataBegin; }
		VkBuffer GetResource() { return m_buffer; }
		VkCommandBuffer GetCommandList() { return m_pCommandBuffer; }

		void AddCopy(VkImage image, VkBufferImageCopy bufferImageCopy);
		void AddPreBarrier(VkImageMemoryBarrier imageMemoryBarrier);
		void AddPostBarrier(VkImageMemoryBarrier imageMemoryBarrier);

		void Flush();
		void FlushAndFinish(bool bDoBarriers = false);

	private:

		Device* m_pDevice;

		VkCommandPool           m_commandPool;
		VkCommandBuffer         m_pCommandBuffer;

		VkBuffer                m_buffer;
		VkDeviceMemory          m_deviceMemory;

		VkFence m_fence;

		UINT8* m_pDataBegin = nullptr;    // starting position of upload heap
		UINT8* m_pDataCur = nullptr;      // current position of upload heap
		UINT8* m_pDataEnd = nullptr;      // ending position of upload heap 
		size_t ac = 0;
		size_t ac0 = 0;
	};

	struct IMG_INFO
	{
		UINT32           width;
		UINT32           height;
		UINT32           depth;
		UINT32           arraySize;
		UINT32           mipMapCount;
		DXGI_FORMAT      format;
		UINT32           bitCount;
		VkFormat		 vkformat;
	};

	//Loads a Image file

	class ImgLoader
	{
	public:
		virtual ~ImgLoader() {};
		virtual bool Load(const char* pFilename, float cutOff, IMG_INFO* pInfo) = 0;
		// after calling Load, calls to CopyPixels return each time a lower mip level 
		virtual void CopyPixels(void* pDest, uint32_t stride, uint32_t width, uint32_t height) = 0;
	};


	ImgLoader* CreateImageLoader(const char* pFilename);


	class Texture
	{
	public:
		Texture();
		virtual ~Texture();
		virtual void OnDestroy();

		// load file into heap
		INT32 Init(Device* pDevice, VkImageCreateInfo* pCreateInfo, const char* name = nullptr);
		INT32 InitRenderTarget(Device* pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, VkImageUsageFlags usage, bool bUAV, const char* name = nullptr, VkImageCreateFlagBits flags = (VkImageCreateFlagBits)0);
		INT32 InitDepthStencil(Device* pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, const char* name = nullptr);
		bool InitFromFile(Device* pDevice, UploadHeap* pUploadHeap, const char* szFilename, bool useSRGB = false, VkImageUsageFlags usageFlags = 0, float cutOff = 1.0f);
		//bool InitFromData(Device* pDevice, UploadHeap* uploadHeap, const IMG_INFO& header, const void* data, const char* name = nullptr, bool useSRGB = false);
		bool InitFromData(Device* pDevice, UploadHeap* uploadHeap, IMG_INFO* header, const void* data, int dsize, const char* name, bool useSRGB);

		VkImage Resource() const { return m_pResource; }
		// Render Target View (RTV) 【渲染目标纹理】
		void CreateRTV(VkImageView* pRV, int mipLevel = -1, VkFormat format = VK_FORMAT_UNDEFINED);
		// Shader resource view (SRV) 【用于提交给shader的纹理】
		void CreateSRV(VkImageView* pImageView, int mipLevel = -1);
		// Depth Stencil View (DSV) 【渲染目标深度/模板共享纹理】
		void CreateDSV(VkImageView* pView);
		void CreateCubeSRV(VkImageView* pImageView);

		uint32_t GetWidth() const { return m_header.width; }
		uint32_t GetHeight() const { return m_header.height; }
		glm::ivec2 get_size() { return glm::ivec2(m_header.width, m_header.height); }
		uint32_t GetMipCount() const { return m_header.mipMapCount; }
		uint32_t GetArraySize() const { return m_header.arraySize; }
		VkFormat GetFormat() const { return m_format; }

	private:
		Device* m_pDevice = NULL;
		std::string     m_name = "";
#ifdef USE_VMA
		VmaAllocation    m_ImageAlloc = VK_NULL_HANDLE;
#else
		VkDeviceMemory   m_deviceMemory = VK_NULL_HANDLE;
#endif
		VkFormat         m_format = {};
		VkImage          m_pResource = VK_NULL_HANDLE;

		IMG_INFO  m_header = {};

	protected:

		struct FootPrint
		{
			UINT8* pixels;
			uint32_t width, height, offset;
		};// footprints[6][12];

		VkImage CreateTextureCommitted(Device* pDevice, UploadHeap* pUploadHeap, const char* pName, bool useSRGB = false, VkImageUsageFlags usageFlags = 0);
		void LoadAndUpload(Device* pDevice, UploadHeap* pUploadHeap, ImgLoader* pDds, VkImage pTexture2D);
		void LoadAndUpload0(Device* pDevice, UploadHeap* pUploadHeap, char* data, VkImage pTexture2D);
		bool isCubemap()const;
	};

	struct SceneShadowInfo {
		Texture         ShadowMap;
		uint32_t        ShadowIndex;
		uint32_t        ShadowResolution;
		uint32_t        LightIndex;
		VkImageView     ShadowDSV;
		VkFramebuffer   ShadowFrameBuffer;
		VkImageView		ShadowSRV;
	};

	// todo GPUTimestamps
	// This class helps insert queries in the command buffer and readback the results.
	// The tricky part in fact is reading back the results without stalling the GPU. 
	// For that it splits the readback heap in <numberOfBackBuffers> pieces and it reads 
	// from the last used chuck.

	class GPUTimestamps
	{
	public:
		void OnCreate(Device* pDevice, uint32_t numberOfBackBuffers);
		void OnDestroy();

		void GetTimeStamp(VkCommandBuffer cmd_buf, const char* label);
		void GetTimeStampUser(TimeStamp ts);
		void OnBeginFrame(VkCommandBuffer cmd_buf, std::vector<TimeStamp>* pTimestamp);
		void OnEndFrame();

	private:
		Device* m_pDevice;

		const uint32_t MaxValuesPerFrame = 128;

		VkQueryPool        m_QueryPool;

		uint32_t m_frame = 0;
		uint32_t m_NumberOfBackBuffers = 0;

		std::vector<std::string> m_labels[5];
		std::vector<TimeStamp> m_cpuTimeStamps[5];
	};

	enum PresentationMode
	{
		PRESENTATIONMODE_WINDOWED,
		PRESENTATIONMODE_BORDERLESS_FULLSCREEN,
		PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN
	};

	enum DisplayMode
	{
		DISPLAYMODE_SDR,
		DISPLAYMODE_FSHDR_Gamma22,
		DISPLAYMODE_FSHDR_SCRGB,
		DISPLAYMODE_HDR10_2084,
		DISPLAYMODE_HDR10_SCRGB
	};
	//
	// DefineList, holds pairs of key & value that will be used by the compiler as defines
	//
	class DefineList : public std::map<const std::string, std::string>
	{
	public:
		bool Has(const std::string& str) const
		{
			return find(str) != end();
		}

		size_t Hash(size_t result = HASH_SEED) const
		{
			for (auto it = begin(); it != end(); it++)
			{
				result = HashString(it->first, result);
				result = HashString(it->second, result);
			}
			return result;
		}

		friend DefineList operator+(DefineList   def1,        // passing lhs by value helps optimize chained a+b+c
			const DefineList& def2)  // otherwise, both parameters may be const references
		{
			for (auto it = def2.begin(); it != def2.end(); it++)
				def1[it->first] = it->second;
			return def1;
		}
		std::string to_string() const
		{
			std::string result;
			for (auto it = begin(); it != end(); it++)
			{
				result += it->first + "=" + it->second + "; ";
			}
			return result;
		}
	};

	bool DoesMaterialUseSemantic(DefineList& defines, const std::string semanticName);


	typedef enum GBufferFlagBits
	{
		GBUFFER_NONE = 0,
		GBUFFER_DEPTH = 1,
		GBUFFER_FORWARD = 2,
		GBUFFER_MOTION_VECTORS = 4,
		GBUFFER_NORMAL_BUFFER = 8,
		GBUFFER_DIFFUSE = 16,
		GBUFFER_SPECULAR_ROUGHNESS = 32,
		GBUFFER_OIT_ACCUM = 64,
		GBUFFER_OIT_WEIGHT = 128,
	} GBufferFlagBits;

	typedef uint32_t GBufferFlags;


	class GBufferRenderPass
	{
	public:
		void OnCreate(GBuffer* pGBuffer, GBufferFlags flags, bool bClear, const std::string& name);
		void OnDestroy();
		void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height);
		void OnDestroyWindowSizeDependentResources();
		void BeginPass(VkCommandBuffer commandList, VkRect2D renderArea);
		void EndPass(VkCommandBuffer commandList);
		void GetCompilerDefines(DefineList& defines);
		VkRenderPass GetRenderPass() { return m_renderPass; }
		VkFramebuffer GetFramebuffer() { return m_frameBuffer; }
		VkSampleCountFlagBits  GetSampleCount();
	public:
		Device* m_pDevice;
		GBufferFlags                    m_flags;
		GBuffer* m_pGBuffer;
		VkRenderPass                    m_renderPass;
		VkFramebuffer                   m_frameBuffer;
		std::vector<VkClearValue>       m_clearValues;
	};

	class fbo_info_cx;
	class GLTFCommon;




	// device
#if 1

	bool InstanceProperties::IsLayerPresent(const char* pExtName)
	{
		return std::find_if(
			m_instanceLayerProperties.begin(),
			m_instanceLayerProperties.end(),
			[pExtName](const VkLayerProperties& layerProps) -> bool {
				return strcmp(layerProps.layerName, pExtName) == 0;
			}) != m_instanceLayerProperties.end();
	}

	bool InstanceProperties::IsExtensionPresent(const char* pExtName)
	{
		return std::find_if(
			m_instanceExtensionProperties.begin(),
			m_instanceExtensionProperties.end(),
			[pExtName](const VkExtensionProperties& extensionProps) -> bool {
				return strcmp(extensionProps.extensionName, pExtName) == 0;
			}) != m_instanceExtensionProperties.end();
	}

	VkResult InstanceProperties::Init()
	{
		// Query instance layers.
		//
		uint32_t instanceLayerPropertyCount = 0;
		VkResult res = vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, nullptr);
		m_instanceLayerProperties.resize(instanceLayerPropertyCount);
		assert(res == VK_SUCCESS);
		if (instanceLayerPropertyCount > 0)
		{
			res = vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, m_instanceLayerProperties.data());
			assert(res == VK_SUCCESS);
		}

		// Query instance extensions.
		//
		uint32_t instanceExtensionPropertyCount = 0;
		res = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropertyCount, nullptr);
		assert(res == VK_SUCCESS);
		m_instanceExtensionProperties.resize(instanceExtensionPropertyCount);
		if (instanceExtensionPropertyCount > 0)
		{
			res = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropertyCount, m_instanceExtensionProperties.data());
			assert(res == VK_SUCCESS);
		}

		return res;
	}

	bool InstanceProperties::AddInstanceLayerName(const char* instanceLayerName)
	{
		if (IsLayerPresent(instanceLayerName))
		{
			m_instance_layer_names.push_back(instanceLayerName);
			return true;
		}

		//Trace("The instance layer '%s' has not been found\n", instanceLayerName);

		return false;
	}

	bool InstanceProperties::AddInstanceExtensionName(const char* instanceExtensionName)
	{
		if (IsExtensionPresent(instanceExtensionName))
		{
			m_instance_extension_names.push_back(instanceExtensionName);
			return true;
		}

		//Trace("The instance extension '%s' has not been found\n", instanceExtensionName);

		return false;
	}

	void  InstanceProperties::GetExtensionNamesAndConfigs(std::vector<const char*>* pInstance_layer_names, std::vector<const char*>* pInstance_extension_names)
	{
		for (auto& name : m_instance_layer_names)
			pInstance_layer_names->push_back(name);

		for (auto& name : m_instance_extension_names)
			pInstance_extension_names->push_back(name);
	}

	bool DeviceProperties::IsExtensionPresent(const char* pExtName)
	{
		return std::find_if(
			m_deviceExtensionProperties.begin(),
			m_deviceExtensionProperties.end(),
			[pExtName](const VkExtensionProperties& extensionProps) -> bool {
				return strcmp(extensionProps.extensionName, pExtName) == 0;
			}) != m_deviceExtensionProperties.end();
	}
	bool DeviceProperties::has_extension(const char* pExtName)
	{
		auto it = m_device_extension_names.find(pExtName);
		return pExtName && *pExtName && it != m_device_extension_names.end();
	}
	// 获取设备支持的扩展
	VkResult DeviceProperties::Init(VkPhysicalDevice physicaldevice)
	{
		m_physicaldevice = physicaldevice;

		// Enumerate device extensions
		//
		uint32_t extensionCount;
		VkResult res = vkEnumerateDeviceExtensionProperties(physicaldevice, nullptr, &extensionCount, NULL);
		m_deviceExtensionProperties.resize(extensionCount);
		res = vkEnumerateDeviceExtensionProperties(physicaldevice, nullptr, &extensionCount, m_deviceExtensionProperties.data());

		return res;
	}

	bool DeviceProperties::AddDeviceExtensionName(const char* deviceExtensionName)
	{
		if (IsExtensionPresent(deviceExtensionName))
		{
			m_device_extension_names.insert(deviceExtensionName);
			return true;
		}

		//Trace("The device extension '%s' has not been found", deviceExtensionName);

		return false;
	}

	int DeviceProperties::AddDeviceExtensionName(std::vector<const char*> deviceExtensionName)
	{
		int r = 0;
		for (auto& name : deviceExtensionName)
		{
			if (IsExtensionPresent(name))
			{
				m_device_extension_names.insert(name);
				r++;
			}
		}
		return r;
	}

	void  DeviceProperties::GetExtensionNamesAndConfigs(std::vector<const char*>* pDevice_extension_names)
	{
		for (auto& name : m_device_extension_names)
			pDevice_extension_names->push_back(name.c_str());
	}

	static PFN_vkCreateDebugReportCallbackEXT g_vkCreateDebugReportCallbackEXT = NULL;
	static PFN_vkDebugReportMessageEXT g_vkDebugReportMessageEXT = NULL;
	static PFN_vkDestroyDebugReportCallbackEXT g_vkDestroyDebugReportCallbackEXT = NULL;
	static VkDebugReportCallbackEXT g_DebugReportCallback = NULL;

	static bool s_bCanUseDebugReport = false;

	static VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objectType,
		uint64_t object,
		size_t location,
		int32_t messageCode,
		const char* pLayerPrefix,
		const char* pMessage,
		void* pUserData)
	{
		OutputDebugStringA(pMessage);
		OutputDebugStringA("\n");
		return VK_FALSE;
	}

	const VkValidationFeatureEnableEXT featuresRequested[] = { VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT, VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
	VkValidationFeaturesEXT features = {};

	const char instanceExtensionName[] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
	const char instanceLayerName[] = "VK_LAYER_KHRONOS_validation";

	bool ExtDebugReportCheckInstanceExtensions(InstanceProperties* pIP, bool gpuValidation)
	{
		s_bCanUseDebugReport = pIP->AddInstanceLayerName(instanceLayerName) && pIP->AddInstanceExtensionName(instanceExtensionName);
		if (s_bCanUseDebugReport && gpuValidation)
		{
			features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
			features.pNext = pIP->GetNext();
			features.enabledValidationFeatureCount = _countof(featuresRequested);
			features.pEnabledValidationFeatures = featuresRequested;

			pIP->SetNewNext(&features);
		}

		return s_bCanUseDebugReport;
	}

	void ExtDebugReportGetProcAddresses(VkInstance instance)
	{
		if (s_bCanUseDebugReport)
		{
			g_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
			g_vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT");
			g_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
			assert(g_vkCreateDebugReportCallbackEXT);
			assert(g_vkDebugReportMessageEXT);
			assert(g_vkDestroyDebugReportCallbackEXT);
		}
	}

	void ExtDebugReportOnCreate(VkInstance instance)
	{
		if (g_vkCreateDebugReportCallbackEXT)
		{
			VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
			debugReportCallbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			debugReportCallbackInfo.pfnCallback = MyDebugReportCallback;
			VkResult res = g_vkCreateDebugReportCallbackEXT(instance, &debugReportCallbackInfo, nullptr, &g_DebugReportCallback);
			assert(res == VK_SUCCESS);
		}
	}

	void ExtDebugReportOnDestroy(VkInstance instance)
	{
		// It should happen after destroing device, before destroying instance.
		if (g_DebugReportCallback)
		{
			g_vkDestroyDebugReportCallbackEXT(instance, g_DebugReportCallback, nullptr);
			g_DebugReportCallback = nullptr;
		}
	}

	void SetEssentialInstanceExtensions(bool cpuValidationLayerEnabled, bool gpuValidationLayerEnabled, InstanceProperties* pIp)
	{
		const char* exn[] = {
#if defined(VK_USE_PLATFORM_XLIB_KHR)
			VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_XCB_KHR)
			VK_KHR_XCB_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
			VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_MIR_KHR)
			VK_KHR_MIR_SURFACE_EXTENSION_NAME
#elif defined(__ANDROID__)
			VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#elif defined(_WIN32)
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
		};
		pIp->AddInstanceExtensionName(exn[0]);
		pIp->AddInstanceExtensionName(VK_KHR_SURFACE_EXTENSION_NAME);
		pIp->AddInstanceExtensionName(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		pIp->AddInstanceExtensionName(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
		f_ExtDebugUtilsCheckInstanceExtensions(pIp);
#ifdef ExtCheckHDRInstanceExtensions
		ExtCheckHDRInstanceExtensions(pIp);
#endif
		if (cpuValidationLayerEnabled)
		{
			ExtDebugReportCheckInstanceExtensions(pIp, gpuValidationLayerEnabled);
		}
	}

	void SetEssentialDeviceExtensions(DeviceProperties* pDp)
	{
#ifdef ExtRTCheckExtensions
		m_usingFp16 = ExtFp16CheckExtensions(pDp);
		ExtRTCheckExtensions(pDp, m_rt10Supported, m_rt11Supported);
		ExtVRSCheckExtensions(pDp, m_vrs1Supported, m_vrs2Supported);
		ExtCheckHDRDeviceExtensions(pDp);
		ExtCheckFSEDeviceExtensions(pDp);
		ExtCheckFreeSyncHDRDeviceExtensions(pDp);
#endif
		pDp->AddDeviceExtensionName(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
		pDp->AddDeviceExtensionName({ VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,							// 支持yuv纹理采样
			VK_KHR_MAINTENANCE1_EXTENSION_NAME,
			VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
			VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME });
		pDp->AddDeviceExtensionName({
			// mesh shader
			 VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,VK_EXT_MESH_SHADER_EXTENSION_NAME,VK_KHR_SPIRV_1_4_EXTENSION_NAME,
			 // Required by VK_KHR_spirv_1_4
			 VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
			 // dynamic_rendering
			 // The sample uses the extension (instead of Vulkan 1.2, where dynamic rendering is core)
			 VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
			 VK_KHR_MAINTENANCE_2_EXTENSION_NAME,
			 VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
			 VK_KHR_MULTIVIEW_EXTENSION_NAME,
			 VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
			 VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
			 VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
			 VK_EXT_ROBUSTNESS_2_EXTENSION_NAME
			});
	}

	Device::Device()
	{
	}

	Device::~Device()
	{
	}

	uint32_t GetScore(void* phd)
	{
		VkPhysicalDevice physicalDevice = (VkPhysicalDevice)phd;
		uint32_t score = 0;
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		// Use the features for a more precise way to select the GPU
		//VkPhysicalDeviceFeatures deviceFeatures;
		//vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
		switch (deviceProperties.deviceType)
		{
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			score += 1000;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			score += 10000;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			score += 100;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			score += 10;
			break;
		default:
			break;
		}

		// TODO: add other constraints

		return score;
	}

	VkPhysicalDevice SelectPhysicalDevice(std::vector<device_info_t>& physicalDevices)
	{
		assert(physicalDevices.size() > 0 && "No GPU found");
		std::multimap<uint32_t, VkPhysicalDevice> ratings;
		for (auto& it : physicalDevices) {
			ratings.insert(std::make_pair(GetScore(it.phd), (VkPhysicalDevice)it.phd));
		}
		return ratings.rbegin()->second;
	}

	void Device::OnCreate(dev_info_cx* d, bool cpuValidationLayerEnabled, bool gpuValidationLayerEnabled, void* pw
		, const char* spdname, std::vector<std::string>* pdnv)
	{
		dev_info_cx nd = {};
		InstanceProperties ip;
		ip.Init();
		SetEssentialInstanceExtensions(cpuValidationLayerEnabled, gpuValidationLayerEnabled, &ip);
		auto apiVersion3 = VK_API_VERSION_1_3;
		auto apiVersion4 = VK_API_VERSION_1_4;
		auto apiVersion = apiVersion3;// VK_VERSION_1_2;// VK_API_VERSION_1_0;
		VkInstanceCreateInfo inst_info = {};
		if (d) { nd = *d; }
		d = &nd;
		if (!d->phy)
		{
			std::vector<const char*> instance_layer_names;
			std::vector<const char*> instance_extension_names;
			ip.GetExtensionNamesAndConfigs(&instance_layer_names, &instance_extension_names);
			VkInstance instance = (VkInstance)d->inst;
			if (!d->inst) {
				VkApplicationInfo app_info = {};
				app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				app_info.pNext = NULL;
				app_info.pApplicationName = "vkcmp";
				app_info.applicationVersion = 1;
				app_info.pEngineName = "pnguo";
				app_info.engineVersion = 1;
				app_info.apiVersion = apiVersion;
				inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				inst_info.pNext = 0;
				inst_info.flags = 0;
				inst_info.pApplicationInfo = &app_info;
				inst_info.enabledLayerCount = (uint32_t)instance_layer_names.size();
				inst_info.ppEnabledLayerNames = (uint32_t)instance_layer_names.size() ? instance_layer_names.data() : NULL;
				inst_info.enabledExtensionCount = (uint32_t)instance_extension_names.size();
				inst_info.ppEnabledExtensionNames = instance_extension_names.data();
				VkResult res = vkCreateInstance(&inst_info, NULL, &instance);
				assert(res == VK_SUCCESS);
			}
			if (instance)
			{
				nd.inst = instance;
				auto devs = get_devices(instance);
				if (devs.size() > 0)
				{
					if (pdnv) {
						for (auto& dt : devs) {
							pdnv->push_back(dt.name);
						}
					}
					nd.phy = SelectPhysicalDevice(devs);
					// 选择自定义显卡
					if (spdname && *spdname)
					{
						std::string pdn;
						for (size_t i = 0; i < devs.size(); i++)
						{
							pdn.assign(devs[i].name);
							if (pdn.find(spdname) != std::string::npos)
							{
								nd.phy = devs[i].phd;
								break;
							}
						}
					}
				}
			}
		}
		if (!d || !d->inst || !d->phy)
		{
			printf("Failed to create Vulkan instance or physical device.\n");
			return;
		}

		DeviceProperties dp;
		dp.Init((VkPhysicalDevice)d->phy);
		SetEssentialDeviceExtensions(&dp);

		// Create device
		OnCreateEx((VkInstance)d->inst, (VkPhysicalDevice)d->phy, (VkDevice)d->vkdev, pw, &dp);
		if (!d->vkdev)
			d->vkdev = m_device;
	}


	VkQueueFamilyProperties* get_queue_fp(VkQueueFamilyProperties* queueFamilyProperties, int c, VkQueueFlagBits queueFlags)
	{
		VkQueueFamilyProperties* ret = 0;
		do {
			if (queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				for (uint32_t i = 0; i < c; i++)
				{
					if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
					{
						ret = &queueFamilyProperties[i];
						break;
					}
				}
				if (ret)
				{
					break;
				}
			}

			// Dedicated queue for transfer
			// Try to find a queue family index that supports transfer but not graphics and compute
			if (queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				for (uint32_t i = 0; i < c; i++)
				{
					if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
					{
						ret = &queueFamilyProperties[i];
						break;
					}
				}
				if (ret)
				{
					break;
				}
			}

			// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
			for (uint32_t i = 0; i < c; i++)
			{
				if (queueFamilyProperties[i].queueFlags & queueFlags)
				{
					ret = &queueFamilyProperties[i];
					break;
				}
			}
		} while (0);
		return ret;
	}
	// 获取表面支持的格式
	void get_surface_formats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<VkSurfaceFormatKHR>& rs)
	{
		uint32_t surfaceFormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, NULL);
		if (surfaceFormatCount > 0)
		{
			rs.resize(surfaceFormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, rs.data());
		}
	}
	void* get_device_requirements(VkPhysicalDeviceFeatures* pEnabledFeatures, void* pNext, bool scalar_block_layout, bool timeline_semaphore)
	{
		pEnabledFeatures->fillModeNonSolid = VK_TRUE;
		pEnabledFeatures->sampleRateShading = VK_TRUE;
		pEnabledFeatures->logicOp = VK_TRUE;
#ifdef VK_VERSION_1_2
		static VkPhysicalDeviceVulkan12Features enabledFeatures12 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		if (scalar_block_layout)
		{
			enabledFeatures12.scalarBlockLayout = VK_TRUE;
		}
		if (timeline_semaphore)
		{
			enabledFeatures12.timelineSemaphore = VK_TRUE;
		}
		enabledFeatures12.pNext = pNext;
		pNext = &enabledFeatures12;
#else
		if (scalar_block_layout)
		{
			static VkPhysicalDeviceScalarBlockLayoutFeaturesEXT scalarBlockFeat = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT, .scalarBlockLayout = VK_TRUE };
			scalarBlockFeat.pNext = pNext;
			pNext = &scalarBlockFeat;
		}
		if (timeline_semaphore)
		{

			static VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timelineSemaFeat = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR, .timelineSemaphore = VK_TRUE };
			timelineSemaFeat.pNext = pNext;
			pNext = &timelineSemaFeat;
		}
#endif
		return pNext;
	}

	void Device::OnCreateEx(VkInstance vulkanInstance, VkPhysicalDevice physicalDevice, VkDevice dev, void* pw, DeviceProperties* pDp)
	{
		VkResult res;

		m_instance = vulkanInstance;
		m_physicaldevice = physicalDevice;

		// Get queue/memory/device properties
		//
		uint32_t queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicaldevice, &queue_family_count, NULL);
		assert(queue_family_count >= 1);

		std::vector<VkQueueFamilyProperties> queue_props;
		queue_props.resize(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicaldevice, &queue_family_count, queue_props.data());
		assert(queue_family_count >= 1);

		vkGetPhysicalDeviceMemoryProperties(m_physicaldevice, &m_memoryProperties);
		vkGetPhysicalDeviceProperties(m_physicaldevice, &m_deviceProperties);

		// Get subgroup properties to check if subgroup operations are supported
		//
		m_subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
		m_subgroupProperties.pNext = NULL;

		m_deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		m_deviceProperties2.pNext = &m_subgroupProperties;

		vkGetPhysicalDeviceProperties2(m_physicaldevice, &m_deviceProperties2);

		if (pw) {

#if defined(_WIN32)
			// Crate a Win32 Surface
			//
			VkWin32SurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.pNext = NULL;
			createInfo.hinstance = NULL;
			createInfo.hwnd = (HWND)pw;
			res = vkCreateWin32SurfaceKHR(m_instance, &createInfo, NULL, &m_surface);
#elif __ANDROID__
			ANativeWindow* window = (ANativeWindow*)pw;// platformWindow_;
			VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
			surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
			surfaceCreateInfo.window = window;
			//if (surface)
			//	vkDestroySurfaceKHR(instance, surface, 0);
			surface = 0;
			err = vkCreateAndroidSurfaceKHR(_instance, &surfaceCreateInfo, NULL, &surface);
#else
#error platform not supported
#endif
			if (m_physicaldevice && m_surface)
				get_surface_formats(m_physicaldevice, m_surface, _surfaceFormats);
		}
		auto qfp = get_queue_fp(queue_props.data(), queue_family_count, VK_QUEUE_GRAPHICS_BIT);
		// Find a graphics device and a queue that can present to the above surface
		//
		graphics_queue_family_index = 0;
		present_queue_family_index = 0;
		if (m_surface)
		{
			for (uint32_t i = 0; i < queue_family_count; ++i)
			{
				if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
				{
					if (graphics_queue_family_index == UINT32_MAX) graphics_queue_family_index = i;

					VkBool32 supportsPresent;
					vkGetPhysicalDeviceSurfaceSupportKHR(m_physicaldevice, i, m_surface, &supportsPresent);
					if (supportsPresent == VK_TRUE)
					{
						graphics_queue_family_index = i;
						present_queue_family_index = i;
						break;
					}
				}
			}

			// If didn't find a queue that supports both graphics and present, then
			// find a separate present queue.
			if (present_queue_family_index == UINT32_MAX)
			{
				for (uint32_t i = 0; i < queue_family_count; ++i)
				{
					VkBool32 supportsPresent;
					vkGetPhysicalDeviceSurfaceSupportKHR(m_physicaldevice, i, m_surface, &supportsPresent);
					if (supportsPresent == VK_TRUE)
					{
						present_queue_family_index = (uint32_t)i;
						break;
					}
				}
			}
		}
		else {

			for (uint32_t i = 0; i < queue_family_count; ++i)
			{
				if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
				{
					if (graphics_queue_family_index == UINT32_MAX) graphics_queue_family_index = i;
					present_queue_family_index = i;
					break;
				}
			}
		}

		compute_queue_family_index = UINT32_MAX;

		for (uint32_t i = 0; i < queue_family_count; ++i)
		{
			if ((queue_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
			{
				if (compute_queue_family_index == UINT32_MAX)
					compute_queue_family_index = i;
				if (i != graphics_queue_family_index) {
					compute_queue_family_index = i;
					break;
				}
			}
		}

		// prepare existing extensions names into a buffer for vkCreateDevice
		std::vector<const char*> extension_names;
		pDp->GetExtensionNamesAndConfigs(&extension_names);

		// Create device 
		//
		float queue_priorities[1] = { 0.0 };
		int qcount = 1;
		VkDeviceQueueCreateInfo queue_info[2] = {};
		queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info[0].pNext = NULL;
		queue_info[0].queueCount = 1;// qfp->queueCount;
		queue_info[0].pQueuePriorities = queue_priorities;
		queue_info[0].queueFamilyIndex = graphics_queue_family_index;
		queue_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info[1].pNext = NULL;
		queue_info[1].queueCount = 1;
		queue_info[1].pQueuePriorities = queue_priorities;
		queue_info[1].queueFamilyIndex = compute_queue_family_index;
		if (compute_queue_family_index != graphics_queue_family_index)
			qcount++;
		VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
		physicalDeviceFeatures.fillModeNonSolid = true;
		physicalDeviceFeatures.pipelineStatisticsQuery = true;
		physicalDeviceFeatures.fragmentStoresAndAtomics = true;
		physicalDeviceFeatures.vertexPipelineStoresAndAtomics = true;
		physicalDeviceFeatures.shaderImageGatherExtended = true;
		physicalDeviceFeatures.wideLines = true; //needed for drawing lines with a specific width.
		physicalDeviceFeatures.independentBlend = true; // needed for having different blend for each render target 

		// enable feature to support fp16 with subgroup operations
		//
		VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR shaderSubgroupExtendedType = {};

#if 0
		shaderSubgroupExtendedType.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES_KHR;
		shaderSubgroupExtendedType.pNext = pDp->GetNext(); //used to be pNext of VkDeviceCreateInfo
		shaderSubgroupExtendedType.shaderSubgroupExtendedTypes = VK_TRUE;
#endif
		bool enabledMS = pDp->has_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
		bool supportsKHRSamplerYCbCrConversion = pDp->has_extension(VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME);
		bool dr = pDp->has_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
		bool dr13 = pDp->has_extension(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
		bool robustness_2 = pDp->has_extension(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME);
		bool bds = pDp->has_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
		VkPhysicalDeviceRobustness2FeaturesEXT robustness2 = {};
		robustness2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
		robustness2.nullDescriptor = VK_TRUE;
		// vkvg需要
		auto pNext2 = get_device_requirements(&physicalDeviceFeatures, pDp->GetNext(), true, false);
		if (robustness_2)
		{
			robustness2.pNext = pNext2;
			pNext2 = &robustness2;
		}
		// to be able to bind NULL views
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
		physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physicalDeviceFeatures2.features = physicalDeviceFeatures;
		physicalDeviceFeatures2.pNext = pNext2;

		VkPhysicalDeviceFeatures2 features = {};
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		vkGetPhysicalDeviceFeatures2(m_physicaldevice, &features);

		VkPhysicalDeviceMeshShaderFeaturesEXT enabledMeshShaderFeatures{};
		if (enabledMS) {
			enabledMeshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
			enabledMeshShaderFeatures.meshShader = VK_TRUE;
			enabledMeshShaderFeatures.taskShader = VK_TRUE;
			enabledMeshShaderFeatures.pNext = physicalDeviceFeatures2.pNext;
			physicalDeviceFeatures2.pNext = &enabledMeshShaderFeatures;
		}
		VkPhysicalDeviceSamplerYcbcrConversionFeatures deviceSamplerYcbcrConversionFeatures = {  };
		if (supportsKHRSamplerYCbCrConversion) {
			deviceSamplerYcbcrConversionFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES;
			deviceSamplerYcbcrConversionFeatures.samplerYcbcrConversion = VK_TRUE;
			deviceSamplerYcbcrConversionFeatures.pNext = (void*)physicalDeviceFeatures2.pNext;
			physicalDeviceFeatures2.pNext = &deviceSamplerYcbcrConversionFeatures;
		}
		VkPhysicalDeviceDynamicRenderingFeaturesKHR enabledDynamicRenderingFeaturesKHR = {};
		if (dr && !dr13) {
			enabledDynamicRenderingFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
			enabledDynamicRenderingFeaturesKHR.dynamicRendering = VK_TRUE;
			enabledDynamicRenderingFeaturesKHR.pNext = (void*)physicalDeviceFeatures2.pNext;
			physicalDeviceFeatures2.pNext = &enabledDynamicRenderingFeaturesKHR;
		}

		VkPhysicalDeviceVulkan13Features device_features_1_3 = {};
		device_features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		if (dr13) {
			device_features_1_3.robustImageAccess = VK_TRUE;
			device_features_1_3.maintenance4 = VK_TRUE;
			device_features_1_3.shaderDemoteToHelperInvocation = VK_TRUE;
			device_features_1_3.dynamicRendering = VK_TRUE;
			device_features_1_3.synchronization2 = VK_TRUE;
			device_features_1_3.pNext = (void*)physicalDeviceFeatures2.pNext;
			physicalDeviceFeatures2.pNext = &device_features_1_3;
		}
		VkPhysicalDeviceExtendedDynamicStateFeaturesEXT ds = {};
		if (bds) {
			ds.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
			ds.extendedDynamicState = VK_TRUE;
			ds.pNext = (void*)physicalDeviceFeatures2.pNext;
			physicalDeviceFeatures2.pNext = &ds;
		}
		if (dev)
		{
			m_device = dev;
		}
		else
		{
			VkDeviceCreateInfo device_info = {};
			device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			device_info.pNext = &physicalDeviceFeatures2;
			device_info.queueCreateInfoCount = qcount;
			device_info.pQueueCreateInfos = queue_info;
			device_info.enabledExtensionCount = (uint32_t)extension_names.size();
			device_info.ppEnabledExtensionNames = device_info.enabledExtensionCount ? extension_names.data() : NULL;
			device_info.pEnabledFeatures = NULL;
			res = vkCreateDevice(m_physicaldevice, &device_info, NULL, &m_device);
			assert(res == VK_SUCCESS);
		}
		if (!m_device)return;
		if (dr)
		{
			_vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(m_device, "vkCmdBeginRenderingKHR");
			_vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(m_device, "vkCmdEndRenderingKHR");
			_vkCmdSetFrontFace = (PFN_vkCmdSetFrontFace)vkGetDeviceProcAddr(m_device, "vkCmdSetFrontFace");
			//if (!_vkCmdSetFrontFace)
			//	_vkCmdSetFrontFace = (PFN_vkCmdSetFrontFaceEXT)vkGetDeviceProcAddr(m_device, "vkCmdSetFrontFaceEXT");
		}
		if (enabledMS)
		{
			_vkCmdDrawMeshTasksEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetDeviceProcAddr(m_device, "vkCmdDrawMeshTasksEXT"));
		}

#ifdef USE_VMA
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = GetPhysicalDevice();
		allocatorInfo.device = GetDevice();
		allocatorInfo.instance = m_instance;
		vmaCreateAllocator(&allocatorInfo, &m_hAllocator);
#endif
		// create queues
		//
		vkGetDeviceQueue(m_device, graphics_queue_family_index, 0, &graphics_queue);
		if (graphics_queue_family_index == present_queue_family_index)
		{
			present_queue = graphics_queue;
		}
		else
		{
			vkGetDeviceQueue(m_device, present_queue_family_index, 0, &present_queue);
		}
		if (compute_queue_family_index != UINT32_MAX && compute_queue_family_index != graphics_queue_family_index)
		{
			vkGetDeviceQueue(m_device, compute_queue_family_index, 0, &compute_queue);
		}

		// Init the extensions (if they have been enabled successfuly)
		//
		ExtDebugUtilsGetProcAddresses(m_device);
#ifdef ExtGetHDRFSEFreesyncHDRProcAddresses
		ExtGetHDRFSEFreesyncHDRProcAddresses(m_instance, m_device);
#endif
	}

	void Device::GetDeviceInfo(std::string* deviceName, std::string* driverVersion)
	{
#define EXTRACT(v,offset, length) ((v>>offset) & ((1<<length)-1))
		* deviceName = m_deviceProperties.deviceName;
		*driverVersion = vkr::format("%i.%i.%i", EXTRACT(m_deviceProperties.driverVersion, 22, 10), EXTRACT(m_deviceProperties.driverVersion, 14, 8), EXTRACT(m_deviceProperties.driverVersion, 0, 16));
	}

	void Device::CreatePipelineCache()
	{
		// create pipeline cache

		VkPipelineCacheCreateInfo pipelineCache;
		pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		pipelineCache.pNext = NULL;
		pipelineCache.initialDataSize = 0;
		pipelineCache.pInitialData = NULL;
		pipelineCache.flags = 0;
		VkResult res = vkCreatePipelineCache(m_device, &pipelineCache, NULL, &m_pipelineCache);
		printf("vkCreatePipelineCache %d\n", (int)res);
		assert(res == VK_SUCCESS);
	}

	void Device::DestroyPipelineCache()
	{
		vkDestroyPipelineCache(m_device, m_pipelineCache, NULL);
	}

	VkPipelineCache Device::GetPipelineCache()
	{
		return m_pipelineCache;
	}

	void Device::CreateShaderCache()
	{
#if 0
		PWSTR path = NULL;
		SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
		std::wstring sShaderCachePathW = std::wstring(path) + L"\\AMD\\Cauldron\\ShaderCacheVK";
		CreateDirectoryW((std::wstring(path) + L"\\AMD").c_str(), 0);
		CreateDirectoryW((std::wstring(path) + L"\\AMD\\Cauldron").c_str(), 0);
		CreateDirectoryW((std::wstring(path) + L"\\AMD\\Cauldron\\ShaderCacheVK").c_str(), 0); //std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(sShaderCachePathW)
#endif
	}

	void Device::DestroyShaderCache()
	{
		DestroyShadersInTheCache(m_device);
	}

	void Device::OnDestroy()
	{
		for (auto& [k, v] : _samplers)
		{
			if (v)
			{
				vkDestroySampler(m_device, v, NULL);
			}
		}
		//for (auto& it : _samplers_v)
		//{
		//	if (it.sampler)
		//	{
		//		vkDestroySampler(m_device, it.sampler, NULL);
		//	}
		//}
		_samplers.clear();
		//_samplers_v.clear();
		if (m_surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(m_instance, m_surface, NULL);
		}

#ifdef USE_VMA
		vmaDestroyAllocator(m_hAllocator);
		m_hAllocator = NULL;
#endif

		if (m_device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(m_device, nullptr);
			m_device = VK_NULL_HANDLE;
		}
#if ExtDebugReportOnDestroy
		ExtDebugReportOnDestroy(m_instance);

		//DestroyInstance(m_instance);
#endif
		m_instance = VK_NULL_HANDLE;
	}

	void Device::GPUFlush()
	{
		vkDeviceWaitIdle(m_device);
	}


	VkSampler Device::newSampler(const VkSamplerCreateInfo* pCreateInfo) {
		sampler_kt k = { *pCreateInfo };
		auto& p = _samplers[k];
		if (!p)
		{
			vkCreateSampler(m_device, pCreateInfo, 0, &p);
		}
		return p;
	}

	bool memory_type_from_properties(VkPhysicalDeviceMemoryProperties& memory_properties, uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex) {
		// Search memtypes to find first index with those properties
		for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
			if ((typeBits & 1) == 1) {
				// Type is available, does it match user properties?
				if ((memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
					*typeIndex = i;
					return true;
				}
			}
			typeBits >>= 1;
		}
		// No memory types matched, return failure
		return false;
	}

	void Device::SetDescriptorSet(uint32_t index, VkImageView imageView, VkImageLayout imageLayout, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		VkDescriptorImageInfo desc_image;
		desc_image.sampler = pSampler;
		desc_image.imageView = imageView;
		desc_image.imageLayout = imageLayout;

		VkWriteDescriptorSet write;
		write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = NULL;
		write.dstSet = descriptorSet;
		write.descriptorCount = 1;
		write.descriptorType = (pSampler == NULL) ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo = &desc_image;
		write.dstBinding = index;
		write.dstArrayElement = 0;

		vkUpdateDescriptorSets(m_device, 1, &write, 0, NULL);
	}

	void Device::SetDescriptorSet(uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkImageLayout imageLayout, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		std::vector<VkDescriptorImageInfo> desc_images(descriptorsCount);
		uint32_t i = 0;
		for (; i < imageViews.size(); ++i)
		{
			desc_images[i].sampler = pSampler;
			desc_images[i].imageView = imageViews[i];
			desc_images[i].imageLayout = imageLayout;
		}
		// we should still assign the remaining descriptors
		// Using the VK_EXT_robustness2 extension, it is possible to assign a NULL one
		for (; i < descriptorsCount; ++i)
		{
			desc_images[i].sampler = pSampler;
			desc_images[i].imageView = VK_NULL_HANDLE;
			desc_images[i].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}

		VkWriteDescriptorSet write;
		write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = NULL;
		write.dstSet = descriptorSet;
		write.descriptorCount = descriptorsCount;
		write.descriptorType = pSampler ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.pImageInfo = desc_images.data();
		write.dstBinding = index;
		write.dstArrayElement = 0;

		vkUpdateDescriptorSets(m_device, 1, &write, 0, NULL);
	}

	void Device::SetDescriptorSet(uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		SetDescriptorSet(index, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
	}

	void Device::SetDescriptorSet(uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		SetDescriptorSet(index, descriptorsCount, imageViews, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
	}

	void Device::SetDescriptorSetForDepth(uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		SetDescriptorSet(index, imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
	}

	void Device::SetDescriptorSet(uint32_t index, VkImageView imageView, VkDescriptorSet descriptorSet)
	{
		VkDescriptorImageInfo desc_image;
		desc_image.sampler = VK_NULL_HANDLE;
		desc_image.imageView = imageView;
		desc_image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet write;
		write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = NULL;
		write.dstSet = descriptorSet;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		write.pImageInfo = &desc_image;
		write.dstBinding = index;
		write.dstArrayElement = 0;

		vkUpdateDescriptorSets(m_device, 1, &write, 0, NULL);
	}

	void Device::SetDescriptorSet1(VkBuffer buffer, int index, uint32_t pos, uint32_t size, VkDescriptorSet descriptorSet, uint32_t dt)
	{
		VkDescriptorBufferInfo out = {};
		out.buffer = buffer;
		out.offset = pos;
		out.range = size;// alignUp(size, (uint32_t)256);
		assert(buffer);
		VkWriteDescriptorSet write;
		write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = NULL;
		write.dstSet = descriptorSet;
		write.descriptorCount = 1;
		int dk = dt;
		write.descriptorType = (VkDescriptorType)(dk > 0 ? dt : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		write.pBufferInfo = &out;
		write.dstArrayElement = 0;
		write.dstBinding = index;

		vkUpdateDescriptorSets(m_device, 1, &write, 0, NULL);
	}

	VkDescriptorSetLayout Device::newDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding)
	{
		VkDescriptorSetLayout descSetLayout = {};
		VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
		descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_layout.pNext = NULL;
		descriptor_layout.bindingCount = (uint32_t)pDescriptorLayoutBinding->size();
		descriptor_layout.pBindings = pDescriptorLayoutBinding->data();

		VkResult res = vkCreateDescriptorSetLayout(m_device, &descriptor_layout, NULL, &descSetLayout);
		assert(res == VK_SUCCESS);
		return descSetLayout;
	}

	VkFramebuffer Device::newFrameBuffer(VkRenderPass renderPass, const std::vector<VkImageView>* pAttachments, uint32_t Width, uint32_t Height)
	{
		VkFramebufferCreateInfo fb_info = {};
		fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.layers = 1;
		fb_info.pNext = NULL;
		fb_info.width = Width;
		fb_info.height = Height;

		VkFramebuffer frameBuffer;

		VkResult res;
		fb_info.renderPass = renderPass;
		fb_info.pAttachments = pAttachments->data();
		fb_info.attachmentCount = (uint32_t)pAttachments->size();
		res = vkCreateFramebuffer(m_device, &fb_info, NULL, &frameBuffer);
		assert(res == VK_SUCCESS);

		SetResourceName(m_device, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)frameBuffer, "HelperCreateFrameBuffer");

		return frameBuffer;
	}



#endif // 1

	//



}
//!vkr



void DeviceShutdown(vkr::Device* dev)
{
	if (dev)
	{
		dev->GPUFlush();
		dev->DestroyPipelineCache();
		dev->OnDestroy();
	}
}
#ifdef _WIN32

static std::string GetCPUNameString()
{
	int nIDs = 0;
	int nExIDs = 0;

	char strCPUName[0x40] = { };

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

	return strlen(strCPUName) != 0 ? strCPUName : "UNAVAILABLE";
}
#else
static std::string GetCPUNameString()
{
	return "UNAVAILABLE";
}

#endif

// todo vkr
namespace vkr {


	namespace initializers
	{

		inline VkMemoryAllocateInfo memoryAllocateInfo()
		{
			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			return memAllocInfo;
		}

		inline VkMappedMemoryRange mappedMemoryRange()
		{
			VkMappedMemoryRange mappedMemoryRange{};
			mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			return mappedMemoryRange;
		}

		inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(
			VkCommandPool commandPool,
			VkCommandBufferLevel level,
			uint32_t bufferCount)
		{
			VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
			commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocateInfo.commandPool = commandPool;
			commandBufferAllocateInfo.level = level;
			commandBufferAllocateInfo.commandBufferCount = bufferCount;
			return commandBufferAllocateInfo;
		}

		inline VkCommandPoolCreateInfo commandPoolCreateInfo()
		{
			VkCommandPoolCreateInfo cmdPoolCreateInfo{};
			cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			return cmdPoolCreateInfo;
		}

		inline VkCommandBufferBeginInfo commandBufferBeginInfo()
		{
			VkCommandBufferBeginInfo cmdBufferBeginInfo{};
			cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			return cmdBufferBeginInfo;
		}

		inline VkCommandBufferInheritanceInfo commandBufferInheritanceInfo()
		{
			VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo{};
			cmdBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			return cmdBufferInheritanceInfo;
		}

		inline VkRenderPassBeginInfo renderPassBeginInfo()
		{
			VkRenderPassBeginInfo renderPassBeginInfo{};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			return renderPassBeginInfo;
		}

		inline VkRenderPassCreateInfo renderPassCreateInfo()
		{
			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			return renderPassCreateInfo;
		}

		/** @brief Initialize an image memory barrier with no image transfer ownership */
		inline VkImageMemoryBarrier imageMemoryBarrier()
		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			return imageMemoryBarrier;
		}

		/** @brief Initialize a buffer memory barrier with no image transfer ownership */
		inline VkBufferMemoryBarrier bufferMemoryBarrier()
		{
			VkBufferMemoryBarrier bufferMemoryBarrier{};
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			return bufferMemoryBarrier;
		}

		inline VkMemoryBarrier memoryBarrier()
		{
			VkMemoryBarrier memoryBarrier{};
			memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			return memoryBarrier;
		}

		inline VkImageCreateInfo imageCreateInfo()
		{
			VkImageCreateInfo imageCreateInfo{};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			return imageCreateInfo;
		}

		inline VkSamplerCreateInfo samplerCreateInfo()
		{
			VkSamplerCreateInfo samplerCreateInfo{};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.maxAnisotropy = 1.0f;
			return samplerCreateInfo;
		}

		inline VkImageViewCreateInfo imageViewCreateInfo()
		{
			VkImageViewCreateInfo imageViewCreateInfo{};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			return imageViewCreateInfo;
		}

		inline VkFramebufferCreateInfo framebufferCreateInfo()
		{
			VkFramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			return framebufferCreateInfo;
		}

		inline VkSemaphoreCreateInfo semaphoreCreateInfo()
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo{};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			return semaphoreCreateInfo;
		}

		inline VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0)
		{
			VkFenceCreateInfo fenceCreateInfo{};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = flags;
			return fenceCreateInfo;
		}

		inline VkEventCreateInfo eventCreateInfo()
		{
			VkEventCreateInfo eventCreateInfo{};
			eventCreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
			return eventCreateInfo;
		}

		inline VkSubmitInfo submitInfo()
		{
			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			return submitInfo;
		}

		inline VkViewport viewport(
			float width,
			float height,
			float minDepth,
			float maxDepth)
		{
			VkViewport viewport{};
			viewport.width = width;
			viewport.height = height;
			viewport.minDepth = minDepth;
			viewport.maxDepth = maxDepth;
			return viewport;
		}

		inline VkRect2D rect2D(
			int32_t width,
			int32_t height,
			int32_t offsetX,
			int32_t offsetY)
		{
			VkRect2D rect2D{};
			rect2D.extent.width = width;
			rect2D.extent.height = height;
			rect2D.offset.x = offsetX;
			rect2D.offset.y = offsetY;
			return rect2D;
		}

		inline VkBufferCreateInfo bufferCreateInfo()
		{
			VkBufferCreateInfo bufCreateInfo{};
			bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			return bufCreateInfo;
		}

		inline VkBufferCreateInfo bufferCreateInfo(
			VkBufferUsageFlags usage,
			VkDeviceSize size)
		{
			VkBufferCreateInfo bufCreateInfo{};
			bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufCreateInfo.usage = usage;
			bufCreateInfo.size = size;
			return bufCreateInfo;
		}

		inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
			uint32_t poolSizeCount,
			VkDescriptorPoolSize* pPoolSizes,
			uint32_t maxSets)
		{
			VkDescriptorPoolCreateInfo descriptorPoolInfo{};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.poolSizeCount = poolSizeCount;
			descriptorPoolInfo.pPoolSizes = pPoolSizes;
			descriptorPoolInfo.maxSets = maxSets;
			return descriptorPoolInfo;
		}

		inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
			const std::vector<VkDescriptorPoolSize>& poolSizes,
			uint32_t maxSets)
		{
			VkDescriptorPoolCreateInfo descriptorPoolInfo{};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			descriptorPoolInfo.pPoolSizes = poolSizes.data();
			descriptorPoolInfo.maxSets = maxSets;
			return descriptorPoolInfo;
		}

		inline VkDescriptorPoolSize descriptorPoolSize(
			VkDescriptorType type,
			uint32_t descriptorCount)
		{
			VkDescriptorPoolSize descriptorPoolSize{};
			descriptorPoolSize.type = type;
			descriptorPoolSize.descriptorCount = descriptorCount;
			return descriptorPoolSize;
		}

		inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
			VkDescriptorType type,
			VkShaderStageFlags stageFlags,
			uint32_t binding,
			uint32_t descriptorCount = 1)
		{
			VkDescriptorSetLayoutBinding setLayoutBinding{};
			setLayoutBinding.descriptorType = type;
			setLayoutBinding.stageFlags = stageFlags;
			setLayoutBinding.binding = binding;
			setLayoutBinding.descriptorCount = descriptorCount;
			return setLayoutBinding;
		}

		inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
			const VkDescriptorSetLayoutBinding* pBindings,
			uint32_t bindingCount)
		{
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCreateInfo.pBindings = pBindings;
			descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
			return descriptorSetLayoutCreateInfo;
		}

		inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
			const std::vector<VkDescriptorSetLayoutBinding>& bindings)
		{
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCreateInfo.pBindings = bindings.data();
			descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			return descriptorSetLayoutCreateInfo;
		}

		inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
			const VkDescriptorSetLayout* pSetLayouts,
			uint32_t setLayoutCount = 1)
		{
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
			pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
			return pipelineLayoutCreateInfo;
		}

		inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
			uint32_t setLayoutCount = 1)
		{
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
			return pipelineLayoutCreateInfo;
		}

		inline VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(
			VkDescriptorPool descriptorPool,
			const VkDescriptorSetLayout* pSetLayouts,
			uint32_t descriptorSetCount)
		{
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.descriptorPool = descriptorPool;
			descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
			descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
			return descriptorSetAllocateInfo;
		}

		inline VkDescriptorImageInfo descriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
		{
			VkDescriptorImageInfo descriptorImageInfo{};
			descriptorImageInfo.sampler = sampler;
			descriptorImageInfo.imageView = imageView;
			descriptorImageInfo.imageLayout = imageLayout;
			return descriptorImageInfo;
		}

		inline VkWriteDescriptorSet writeDescriptorSet(
			VkDescriptorSet dstSet,
			VkDescriptorType type,
			uint32_t binding,
			VkDescriptorBufferInfo* bufferInfo,
			uint32_t descriptorCount = 1)
		{
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.descriptorType = type;
			writeDescriptorSet.dstBinding = binding;
			writeDescriptorSet.pBufferInfo = bufferInfo;
			writeDescriptorSet.descriptorCount = descriptorCount;
			return writeDescriptorSet;
		}

		inline VkWriteDescriptorSet writeDescriptorSet(
			VkDescriptorSet dstSet,
			VkDescriptorType type,
			uint32_t binding,
			VkDescriptorImageInfo* imageInfo,
			uint32_t descriptorCount = 1)
		{
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.descriptorType = type;
			writeDescriptorSet.dstBinding = binding;
			writeDescriptorSet.pImageInfo = imageInfo;
			writeDescriptorSet.descriptorCount = descriptorCount;
			return writeDescriptorSet;
		}

		inline VkVertexInputBindingDescription vertexInputBindingDescription(
			uint32_t binding,
			uint32_t stride,
			VkVertexInputRate inputRate)
		{
			VkVertexInputBindingDescription vInputBindDescription{};
			vInputBindDescription.binding = binding;
			vInputBindDescription.stride = stride;
			vInputBindDescription.inputRate = inputRate;
			return vInputBindDescription;
		}

		inline VkVertexInputAttributeDescription vertexInputAttributeDescription(
			uint32_t binding,
			uint32_t location,
			VkFormat format,
			uint32_t offset)
		{
			VkVertexInputAttributeDescription vInputAttribDescription{};
			vInputAttribDescription.location = location;
			vInputAttribDescription.binding = binding;
			vInputAttribDescription.format = format;
			vInputAttribDescription.offset = offset;
			return vInputAttribDescription;
		}

		inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo()
		{
			VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
			pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			return pipelineVertexInputStateCreateInfo;
		}

		inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(
			const std::vector<VkVertexInputBindingDescription>& vertexBindingDescriptions,
			const std::vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions
		)
		{
			VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
			pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
			pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
			pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
			pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
			return pipelineVertexInputStateCreateInfo;
		}

		inline VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
			VkPrimitiveTopology topology,
			VkPipelineInputAssemblyStateCreateFlags flags,
			VkBool32 primitiveRestartEnable)
		{
			VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
			pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			pipelineInputAssemblyStateCreateInfo.topology = topology;
			pipelineInputAssemblyStateCreateInfo.flags = flags;
			pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
			return pipelineInputAssemblyStateCreateInfo;
		}

		inline VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
			VkPolygonMode polygonMode,
			VkCullModeFlags cullMode,
			VkFrontFace frontFace,
			VkPipelineRasterizationStateCreateFlags flags = 0)
		{
			VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
			pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
			pipelineRasterizationStateCreateInfo.cullMode = cullMode;
			pipelineRasterizationStateCreateInfo.frontFace = frontFace;
			pipelineRasterizationStateCreateInfo.flags = flags;
			pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
			pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
			return pipelineRasterizationStateCreateInfo;
		}

		inline VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
			VkColorComponentFlags colorWriteMask,
			VkBool32 blendEnable)
		{
			VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
			pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
			pipelineColorBlendAttachmentState.blendEnable = blendEnable;
			return pipelineColorBlendAttachmentState;
		}

		inline VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
			uint32_t attachmentCount,
			const VkPipelineColorBlendAttachmentState* pAttachments)
		{
			VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
			pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
			pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
			return pipelineColorBlendStateCreateInfo;
		}

		inline VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
			VkBool32 depthTestEnable,
			VkBool32 depthWriteEnable,
			VkCompareOp depthCompareOp)
		{
			VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
			pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
			pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
			pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
			pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
			return pipelineDepthStencilStateCreateInfo;
		}

		inline VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
			uint32_t viewportCount,
			uint32_t scissorCount,
			VkPipelineViewportStateCreateFlags flags = 0)
		{
			VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
			pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			pipelineViewportStateCreateInfo.viewportCount = viewportCount;
			pipelineViewportStateCreateInfo.scissorCount = scissorCount;
			pipelineViewportStateCreateInfo.flags = flags;
			return pipelineViewportStateCreateInfo;
		}

		inline VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
			VkSampleCountFlagBits rasterizationSamples,
			VkPipelineMultisampleStateCreateFlags flags = 0)
		{
			VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
			pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
			pipelineMultisampleStateCreateInfo.flags = flags;
			return pipelineMultisampleStateCreateInfo;
		}

		inline VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
			const VkDynamicState* pDynamicStates,
			uint32_t dynamicStateCount,
			VkPipelineDynamicStateCreateFlags flags = 0)
		{
			VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
			pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
			pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
			pipelineDynamicStateCreateInfo.flags = flags;
			return pipelineDynamicStateCreateInfo;
		}

		inline VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
			const std::vector<VkDynamicState>& pDynamicStates,
			VkPipelineDynamicStateCreateFlags flags = 0)
		{
			VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
			pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates.data();
			pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(pDynamicStates.size());
			pipelineDynamicStateCreateInfo.flags = flags;
			return pipelineDynamicStateCreateInfo;
		}

		inline VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo(uint32_t patchControlPoints)
		{
			VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo{};
			pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
			pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
			return pipelineTessellationStateCreateInfo;
		}

		inline VkGraphicsPipelineCreateInfo pipelineCreateInfo(
			VkPipelineLayout layout,
			VkRenderPass renderPass,
			VkPipelineCreateFlags flags = 0)
		{
			VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCreateInfo.layout = layout;
			pipelineCreateInfo.renderPass = renderPass;
			pipelineCreateInfo.flags = flags;
			pipelineCreateInfo.basePipelineIndex = -1;
			pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			return pipelineCreateInfo;
		}

		inline VkGraphicsPipelineCreateInfo pipelineCreateInfo()
		{
			VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCreateInfo.basePipelineIndex = -1;
			pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			return pipelineCreateInfo;
		}

		inline VkComputePipelineCreateInfo computePipelineCreateInfo(
			VkPipelineLayout layout,
			VkPipelineCreateFlags flags = 0)
		{
			VkComputePipelineCreateInfo computePipelineCreateInfo{};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = layout;
			computePipelineCreateInfo.flags = flags;
			return computePipelineCreateInfo;
		}

		inline VkPushConstantRange pushConstantRange(
			VkShaderStageFlags stageFlags,
			uint32_t size,
			uint32_t offset)
		{
			VkPushConstantRange pushConstantRange{};
			pushConstantRange.stageFlags = stageFlags;
			pushConstantRange.offset = offset;
			pushConstantRange.size = size;
			return pushConstantRange;
		}

		inline VkBindSparseInfo bindSparseInfo()
		{
			VkBindSparseInfo bindSparseInfo{};
			bindSparseInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
			return bindSparseInfo;
		}

		/** @brief Initialize a map entry for a shader specialization constant */
		inline VkSpecializationMapEntry specializationMapEntry(uint32_t constantID, uint32_t offset, size_t size)
		{
			VkSpecializationMapEntry specializationMapEntry{};
			specializationMapEntry.constantID = constantID;
			specializationMapEntry.offset = offset;
			specializationMapEntry.size = size;
			return specializationMapEntry;
		}

		/** @brief Initialize a specialization constant info structure to pass to a shader stage */
		inline VkSpecializationInfo specializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data)
		{
			VkSpecializationInfo specializationInfo{};
			specializationInfo.mapEntryCount = mapEntryCount;
			specializationInfo.pMapEntries = mapEntries;
			specializationInfo.dataSize = dataSize;
			specializationInfo.pData = data;
			return specializationInfo;
		}

		/** @brief Initialize a specialization constant info structure to pass to a shader stage */
		inline VkSpecializationInfo specializationInfo(const std::vector<VkSpecializationMapEntry>& mapEntries, size_t dataSize, const void* data)
		{
			VkSpecializationInfo specializationInfo{};
			specializationInfo.mapEntryCount = static_cast<uint32_t>(mapEntries.size());
			specializationInfo.pMapEntries = mapEntries.data();
			specializationInfo.dataSize = dataSize;
			specializationInfo.pData = data;
			return specializationInfo;
		}

		// Ray tracing related
		inline VkAccelerationStructureGeometryKHR accelerationStructureGeometryKHR()
		{
			VkAccelerationStructureGeometryKHR accelerationStructureGeometryKHR{};
			accelerationStructureGeometryKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			return accelerationStructureGeometryKHR;
		}

		inline VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfoKHR()
		{
			VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfoKHR{};
			accelerationStructureBuildGeometryInfoKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			return accelerationStructureBuildGeometryInfoKHR;
		}

		inline VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfoKHR()
		{
			VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfoKHR{};
			accelerationStructureBuildSizesInfoKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
			return accelerationStructureBuildSizesInfoKHR;
		}

		inline VkRayTracingShaderGroupCreateInfoKHR rayTracingShaderGroupCreateInfoKHR()
		{
			VkRayTracingShaderGroupCreateInfoKHR rayTracingShaderGroupCreateInfoKHR{};
			rayTracingShaderGroupCreateInfoKHR.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			return rayTracingShaderGroupCreateInfoKHR;
		}

		inline VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfoKHR()
		{
			VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfoKHR{};
			rayTracingPipelineCreateInfoKHR.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
			return rayTracingPipelineCreateInfoKHR;
		}

		inline VkWriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAccelerationStructureKHR()
		{
			VkWriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAccelerationStructureKHR{};
			writeDescriptorSetAccelerationStructureKHR.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
			return writeDescriptorSetAccelerationStructureKHR;
		}

	}


}

// todo PBR渲染管线

namespace vkr {
	struct Geometry
	{
		VkIndexType m_indexType = {};
		uint32_t m_NumIndices = 0;
		uint32_t instanceCount = 1;
		VkDescriptorBufferInfo m_IBV = {};
		std::vector<VkDescriptorBufferInfo> m_VBV;
	};
	using glm::vec2;
	using glm::vec4;
	using glm::mat3;
	using glm::mat3x4;
	using glm::mat4;
#ifndef GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
	using glm::vec3;
#else
	struct vec3 {
		float x = 0.0f, y = 0.0f, z = 0.0f;
		vec3() {}
		vec3(float xx, float yy, float zz) :x(xx), y(yy), z(zz) {}
		vec3(double xx, double yy, double zz) :x(xx), y(yy), z(zz) {}
		vec3(const glm::vec3& v) :x(v.x), y(v.y), z(v.z) {}
	};
#endif
	// https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_texture_transform
#define KHR_TEXTURE_TRANSFORM_EXTENSION_NAME "KHR_texture_transform"


	struct PBRMaterialParametersConstantBuffer
	{
		// pbrMetallicRoughness
		vec4 u_BaseColorFactor;
		float u_MetallicFactor;
		float u_RoughnessFactor;

		float u_AttenuationDistance;//KHR_materials_volume
		float u_ThicknessFactor;
		vec3 u_AttenuationColor;

		float u_TransmissionFactor;	//KHR_materials_transmission
		vec3 u_EmissiveFactor;
		float pad0;
		// KHR_materials_pbrSpecularGlossiness
		vec4 diffuseFactor;
		vec3 specularFactor;
		float glossinessFactor;
	};

	using glm::ivec2;

	struct old_pbrMaterial
	{
		// pbrMetallicRoughness
		vec4  baseColorFactor;
		vec3  emissiveFactor;
		float metallicFactor;

		float roughnessFactor;
		float normalTextureScale;
		int   alphaMode;
		float alphaCutoff;
		// int   pbrBaseColorTexture;   
		// int   normalTexture;           
		// int   pbrMetallicRoughnessTexture;  
		// int   emissiveTexture;      


		// KHR_materials_pbrSpecularGlossiness
		vec4 pbrDiffuseFactor;
		vec3 pbrSpecularFactor;
		float glossinessFactor;

		// KHR_materials_unlit
		int unlit;

		// KHR_materials_ior
		float ior;

		// KHR_materials_transmission
		float transmissionFactor;
		//int   transmissionTexture;   

		// KHR_materials_volume 
		float attenuationDistance;
		vec3  attenuationColor;
		float thicknessFactor;
		//int   thicknessTexture;   

		float diffuseTransmissionFactor;
		vec3 diffuseTransmissionColorFactor;

		// KHR_materials_anisotropy
		float anisotropyStrength;
		//int   anisotropyTexture; 
		float anisotropyRotation;

		// KHR_materials_clearcoat
		float clearcoatFactor;
		float clearcoatRoughness;
		//int   clearcoatTexture;            
		//int   clearcoatRoughnessTexture;  
		//int   clearcoatNormalTexture;     

		// KHR_materials_specular
		vec3  specularColorFactor;
		float specularFactor;
		//int   specularTexture;        
		//int   specularColorTexture;  

		// KHR_materials_iridescence
		float iridescenceFactor;
		//int   iridescenceTexture;         
		float iridescenceThicknessMinimum = 100;  // 100
		float iridescenceThicknessMaximum = 400;  // 400
		//int   iridescenceThicknessTexture;   
		float iridescenceIor = 1.3;               // 1.3

		// KHR_materials_sheen 
		vec3  sheenColorFactor;
		float sheenRoughnessFactor;
		//int   sheenColorTexture;     
		//int   sheenRoughnessTexture;  
		//KHR_materials_dispersion
		float dispersion = 0.0;
		// KHR_texture_transform 
		//mat3 uvTransform = mat3(1.0);

		ivec2 transmissionFramebufferSize;
	};
	enum class UVT_E
	{
		e_EnvRotation,
		e_NormalUVTransform,
		e_EmissiveUVTransform,
		e_OcclusionUVTransform,
		e_BaseColorUVTransform,
		e_MetallicRoughnessUVTransform,
		e_DiffuseUVTransform,
		e_SpecularGlossinessUVTransform,
		e_ClearcoatUVTransform,
		e_ClearcoatRoughnessUVTransform,
		e_ClearcoatNormalUVTransform,
		e_SheenColorUVTransform,
		e_SheenRoughnessUVTransform,
		e_SpecularUVTransform,
		e_SpecularColorUVTransform,
		e_TransmissionUVTransform,
		e_ThicknessUVTransform,
		e_IridescenceUVTransform,
		e_IridescenceThicknessUVTransform,
		e_DiffuseTransmissionUVTransform,
		e_DiffuseTransmissionColorUVTransform,
		e_AnisotropyUVTransform,
		e_COUNT
	};
#define UVMAT_TYPE glm::mat3x4
	// todo pbr UBO用结构体
	struct pbr_factors_t
	{
		// pbrMetallicRoughness
		vec4 baseColorFactor = vec4(1.0);
		float metallicFactor = 0;			// 金属度
		float roughnessFactor = 0;			// 粗糙度
		float normalScale = 1.0;
		// KHR_materials_emissive_strength
		float emissiveStrength = 1.0;
		vec3 emissiveFactor = glm::vec3(1.0);
		int   alphaMode;
		float alphaCutoff;
		float occlusionStrength;
		//KHR_materials_ior
		float ior = 1.5;
		int mipCount = 10;
		// KHR_materials_pbrSpecularGlossiness
		vec4 pbrDiffuseFactor = vec4(1.0);
		vec3 pbrSpecularFactor = glm::vec3(1.0);
		float glossinessFactor = 1.0;
		// Specular KHR_materials_specular
		vec3  specularColorFactor = glm::vec3(1.0);
		float specularFactor = 1.0;
		// KHR_materials_sheen 
		vec3 sheenColorFactor = glm::vec3();
		float sheenRoughnessFactor = 0;
		// KHR_materials_anisotropy
		vec3 anisotropy = glm::vec3();
		// KHR_materials_transmission
		float transmissionFactor = 0;
		ivec2 transmissionFramebufferSize;
		float thicknessFactor;
		float diffuseTransmissionFactor;
		vec3 diffuseTransmissionColorFactor;
		//KHR_materials_dispersion
		float dispersion = 0;
		vec3 attenuationColor = glm::vec3(1.0);
		float attenuationDistance = 10240;
		// KHR_materials_iridescence
		float iridescenceFactor = 0;
		float iridescenceIor = 1.3;
		float iridescenceThicknessMinimum = 100;
		float iridescenceThicknessMaximum = 400;
		// KHR_materials_clearcoat
		float clearcoatFactor = 0;
		float clearcoatRoughness = 0;
		float clearcoatNormalScale = 1;
		float envIntensity = 1;
		int unlit = 0;
		float pad[3];
	};


#define pbrMaterial pbr_factors_t
	struct PBRMaterialParameters
	{
		bool     m_doubleSided = false;
		bool     m_blending = false;
		bool     transmission = false;
		bool     useSheen = false;

		DefineList m_defines;

		pbrMaterial m_params = {};
		UVMAT_TYPE uvTransform[static_cast<int>(UVT_E::e_COUNT)] = {};
		size_t uvc = 0;
		uint32_t mid = 0;
	};
	struct morph_t;

	// Material, primitive and mesh structs specific for the PBR pass (you might want to compare these structs with the ones used for the depth pass in GltfDepthPass.h)

	struct PBRMaterial
	{
		int m_textureCount = 0;
		VkDescriptorSet m_texturesDescriptorSet = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_texturesDescriptorSetLayout = VK_NULL_HANDLE;

		PBRMaterialParameters m_pbrMaterialParameters = {};
	};
	struct material_v
	{
		uint32_t mid;
		std::vector<int> variants;
	};

	struct PBRPipe_t {
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkPipeline m_pipelineWireframe = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_uniformsDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_texturesDescriptorSetLayout = VK_NULL_HANDLE;
		std::string pipe_kv;
		PBRPipe_t* next = 0;
	};

	struct DepthPipe_t {
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_uniformsDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_texturesDescriptorSetLayout = VK_NULL_HANDLE;
	};

	struct PBRPrimitives
	{
		Geometry m_geometry;
		PBRMaterial* m_pMaterial = NULL;
		PBRPipe_t* _pipe = 0;
		VkDescriptorSet m_uniformsDescriptorSet = VK_NULL_HANDLE;
		int mid = 0;
		int variants_id = 0;
		int variants_id0 = 0;
		std::vector<material_v> material_variants;
	public:
		void DrawPrimitive(VkCommandBuffer cmd_buf, uint32_t* uniformOffsets, uint32_t uniformOffsetsCount, bool bWireframe, void* t);
	};

	struct PBRMesh
	{
		std::vector<PBRPrimitives> m_pPrimitives;
	};



	class pipeline_ptr_info;
	class queuethread_cx;

#ifndef YUV_INFO_ST
#define YUV_INFO_ST
	struct yuv_info_t {
		void* ctx = 0;
		void* data[3] = {};
		uint32_t size[3] = {};
		uint32_t ws[3] = {};
		uint32_t width = 0, height = 0;
		int8_t format = 0;		// 0=420, 1=422, 2=444
		int8_t b = 8;			// bpp=8,10,12,16
		int8_t t = 0;			// 1plane时422才有0=gbr, 1=brg
		int8_t plane = 0;		// 1 2 3
		int rotate = 0;
	};
#define t_vector std::vector
#endif
	enum class ImageLayoutBarrier
	{
		UNDEFINED,
		TRANSFER_DST,
		COLOR_ATTACHMENT,
		DEPTH_STENCIL_ATTACHMENT,
		TRANSFER_SRC,
		PRESENT_SRC,
		SHADER_READ,
		DEPTH_STENCIL_READ,
		ComputeGeneralRW,
		PixelGeneralRW,
	};// todo dvk_buffer
	class dvk_buffer
	{
	public:
		VkBuffer buffer = 0;
		VkDevice device = 0;
		VkDeviceMemory memory = 0;
		Device* _dev = 0;
		VkDescriptorBufferInfo* descriptor = 0;
		//std::vector<char> descriptors;
		//VkDescriptorType	dtype;// =
		/*
			VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
			VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
			texel storage dynamic
		*/
		uint32_t dtype = 6;
		VkDeviceSize _size = 0;
		VkDeviceSize _capacity = 0;
		uint32_t	 _count = 0, stride = 0;
		uint32_t	 type_ = 0;

		VkDeviceSize alignment = 0;
		void* mapped = nullptr;

		/** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
		//VkBufferUsageFlags usageFlags = 0;
		uint32_t usageFlags = 0;
		/** @brief Memory propertys flags to be filled by external source at buffer creation (to query at some later point) */
		//VkMemoryPropertyFlags memoryPropertyFlags = 0;
		uint32_t memoryPropertyFlags = 0;

		uint32_t e_size = 0;
		uint32_t fpos = -1;
		uint32_t fsize = 0;
		//std::vector<char>				data;
		std::vector<VkDeviceSize> pOffsets;
	public:
		dvk_buffer(Device* dev, uint32_t usage, uint32_t mempro, uint32_t size, void* data);
		virtual ~dvk_buffer();

		// 可以创建所有buffer
		static dvk_buffer* create(Device* dev, uint32_t usage, uint32_t mempro, uint32_t size, void* data = nullptr);
		// TEXEL_BUFFER
		static dvk_buffer* new_texel(Device* dev, bool storage, uint32_t size, void* data);
		static dvk_buffer* new_staging(Device* dev, uint32_t size, void* data);
		// device_local=true则CPU不能访问	compute_rw=compute shader是否可读写
		static dvk_buffer* new_vbo(Device* dev, bool device_local, bool compute_rw, uint32_t size, void* data = nullptr);
		static dvk_buffer* new_indirect(Device* dev, bool device_local, uint32_t size, void* data = nullptr);
		// 索引数量count，type 0=16，1=32
		static dvk_buffer* new_ibo(Device* dev, int type, bool device_local, uint32_t count, void* data = nullptr);
		// , uint32_t usage1可以增加vbo\ibo使用
		static dvk_buffer* new_ubo(Device* dev, uint32_t size, void* data = nullptr, uint32_t usage1 = 0);
		static dvk_buffer* new_ssbo(Device* dev, uint32_t size, void* data = nullptr);
		// todo*不能用
		static void copy_buffer(dvk_buffer* dst, dvk_buffer* src, uint64_t dst_offset = 0, uint64_t src_offset = 0, int64_t size = -1);
	public:
		//operator VkBuffer () { return buffer; }

		void setDesType(uint32_t dt);

		// 重置大小
		void resize(size_t size);
	public:
		bool make_data(void* data, size_t size, size_t offset = 0, bool isun = true);
		void* map(VkDeviceSize dsize, VkDeviceSize offset = 0);
		void* get_map(VkDeviceSize offset = 0);
		void unmap();
		uint32_t flush(VkDeviceSize dsize, VkDeviceSize offset = 0);
		uint32_t flush();
		size_t set_data(void* data, size_t size, size_t offset, bool is_flush, bool iscp = true);
		void copy_to(void* data, size_t size);
		uint32_t invalidate(VkDeviceSize size_, VkDeviceSize offset = 0);

		void destroybuf();

	};
	// todo staging_buffer
	class dvk_staging_buffer
	{
	public:
		VkBuffer buffer = 0;
		VkDeviceMemory mem = 0;
		VkDevice _dev = 0;

		VkDeviceSize bufferSize = 0, memSize = 0;

		void* mapped = nullptr;
		bool isd = true;
	public:
		dvk_staging_buffer();

		virtual	~dvk_staging_buffer();
		void freeBuffer();
		void initBuffer(Device* dev, VkDeviceSize size);
		char* map();
		void unmap();
		void copyToBuffer(void* data, size_t bsize);
		void copyGetBuffer(std::vector<char>& outbuf);
		size_t getBufSize();
		void getBuffer(char* outbuf, size_t len);
	};

	template<typename T> inline T alignUp(T& val, T alignment)
	{
		auto r = (val + alignment - (T)1) & ~(alignment - (T)1);
		val = r;
		return r;
	}

	// align val to the next multiple of alignment
	template<typename T> inline T AlignUp(T val, T alignment)
	{
		return (val + alignment - (T)1) & ~(alignment - (T)1);
	}
	// align val to the previous multiple of alignment
	template<typename T> inline T AlignDown(T val, T alignment)
	{
		return val & ~(alignment - (T)1);
	}
	template<typename T> inline T DivideRoundingUp(T a, T b)
	{
		return (a + b - (T)1) / b;
	}
	// 动态缓冲区
	class dynamic_buffer_cx
	{
	public:
		Device* dev = 0;
		dvk_buffer* _ubo = 0;
		char* mdata = 0;
		int64_t last = 0;
		uint32_t _ubo_align = 64;
	public:
		// acsize预分配大小，是否vbo\ibo混用vibo = false, 是否混用transfer
		dynamic_buffer_cx(Device* d, size_t acsize, bool vibo = false, bool transfer = false);
		~dynamic_buffer_cx();
	public:
		// 增加空间，原先分配的空间则会失效
		void append(size_t size);
		// 重新获取数据指针
		char* get_ptr(uint32_t offset);
		void flush(size_t pos, size_t size);
		// 清空分配
		void clear();
		// 删除内存显存占用
		void free_mem();
		// 分配n个对象大小的空间
		template<class T>
		uint32_t alloc_obj(T*& t, int n = 1) {
			uint32_t r = 0;
			t = (T*)alloc(sizeof(T) * n, &r);
			return r;
		}
		char* alloc(size_t size, uint32_t* offset);
		bool AllocConstantBuffer(uint32_t size, void** pData, VkDescriptorBufferInfo* pOut);
	private:

	};
	class upload_cx
	{
	public:
		struct cp2mem_t
		{
			VkImage image;
			VkBufferImageCopy icp;
			VkImageMemoryBarrier preb, postb;
			VkBuffer buffer;
		};
		struct cp2img_t
		{
			VkImage image;
			VkImageCopy icp;
			VkImageMemoryBarrier preb, postb;
			VkImage dst;
		};
		struct clearimage {
			VkImage image;
			glm::vec4 c;
		};
		struct COPY_T
		{
			VkImage _image; VkBufferImageCopy _bic;
		};
		Device* _dev = nullptr;						// 设备
		dynamic_buffer_cx* db = 0;						// 缓存自动扩容
		size_t ncap = 0;								// 初始大小
		size_t last_size = 0, last_pos = 0, ups = 0;	// 最后的大小
		t_vector<VkImageMemoryBarrier> toPreBarrier;
		t_vector<COPY_T> _copies;
		t_vector<clearimage> _clear_cols;
		t_vector<VkImageMemoryBarrier> toPostBarrier;

		t_vector<cp2mem_t> cp2m;
		t_vector<cp2img_t> cp2img;

		VkCommandPool _commandPool = {};
		VkCommandBuffer _pCommandBuffer = {};
		VkFence _fence = {};
		std::atomic_int64_t a = {};
		bool isprint = true;
	public:
		upload_cx();
		~upload_cx();
		// size初始缓存大小
		void init(Device* dev, size_t size, int idxqueue);
		void on_destroy();
		// 纹理数据复制到这里
		char* get_tbuf(size_t size, size_t uAlign);
		void AddPreBarrier(VkImageMemoryBarrier imb);
		void addCopy(VkImage image, VkBufferImageCopy bic);
		void AddPostBarrier(VkImageMemoryBarrier imb);

		void flush();
		int flushAndFinish(int wait_time = -1);
		void addClear(VkImage image, glm::vec4 color);


		// 显存不够用时，没纹理上传，释放缓存占用
		void free_buf();

		VkBuffer get_resource();
		VkCommandBuffer get_cmdbuf();

		void cmd_begin();
		void cmd_end();
	public:
		void add_pre(VkImage image, VkFormat format, uint32_t aspectMask, VkImageLayout oldLayout, VkImageLayout newLayout
			, uint32_t mipLevel = 1, uint32_t layerCount = 1);
		void add_post(VkImage image, VkFormat format, uint32_t aspectMask, VkImageLayout oldLayout, VkImageLayout newLayout
			, uint32_t mipLevel = 1, uint32_t layerCount = 1);
		// 复制纹理到内存
		void add_copy2mem(VkImage image, VkBufferImageCopy icp, VkImageSubresourceRange subresourceRange, VkImageAspectFlags aspectMask, VkImageLayout il, VkBuffer buffer);
		// 复制到另一个纹理
		void add_copy2img(VkImage image, VkImageCopy icp, VkImageSubresourceRange subresourceRange, VkImageAspectFlags aspectMask, VkImageLayout il, VkImage dst);

	private:

	};
	class dvk_texture
	{
	public:
		VkImage _image = 0;
		VkImageView _view = 0;
		VkSampler sampler = 0;
		VkDeviceMemory image_memory = 0;
		int64_t cap_device_mem_size = 0;
		int cap_inc = 0, caps = 8;			// 分配8次就重新释放显存
		VkDescriptorImageInfo* descriptor = {};
		VkImageCreateInfo* _info = 0;
		uint32_t width = 0, height = 0;
		uint32_t mipLevels = 1;
		uint32_t layerCount = 1;
		uint32_t _depth = 1;
		//VkDescriptorType dtype = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		//VkFormat _format = VK_FORMAT_R8G8B8A8_UNORM;
		uint32_t _format = 37;
		int64_t _alloc_size = 0;		// 数据字节大小
		Device* _dev = nullptr;
		std::string _name;
		ImageLayoutBarrier _image_layout = ImageLayoutBarrier::SHADER_READ;
		void* user_data = 0;
		void* uimg = 0;
		void* mapped = nullptr;
		IMG_INFO* _header = 0;
		void* ad = 0;
		void* ad1 = 0;
		pipeline_ptr_info* pipe = 0;
		yuv_info_t yuv = {};
		VkSamplerYcbcrConversion ycbcr_sampler_conversion = {};
	public:
		dvk_texture();
		dvk_texture(Device* dev);

		~dvk_texture();

		bool check_format();

		void OnDestroy();
		glm::ivec2 get_size();
	public:

		static dvk_texture* new_yuv(yuv_info_t yuv, upload_cx* up);
		//static dvk_texture* new_2d(const std::string& filename, upload_cx* up, ImageLayoutBarrier imageLayout = ImageLayoutBarrier::SHADER_READ);
		//static dvk_texture* new_image2d(Image* img, upload_cx* up, ImageLayoutBarrier imageLayout = ImageLayoutBarrier::SHADER_READ, bool storage = false);
		//static dvk_texture* new_image2d(Image* img, VkDevice dev, ImageLayoutBarrier imageLayout = ImageLayoutBarrier::SHADER_READ, bool storage = false);
		static dvk_texture* new_image2d(void* buffer, VkDeviceSize bufferSize, uint32_t format, uint32_t w, uint32_t h, upload_cx* up, uint32_t imageUsageFlags, ImageLayoutBarrier imageLayout = ImageLayoutBarrier::SHADER_READ);
		static dvk_texture* new_image2d(const char* fn, upload_cx* up, bool srgb);
		static dvk_texture* new_image2d(upload_cx* up, const void* data, int size, int width, int height, bool useSRGB, uint32_t format, uint32_t dxformat);
		//static dvk_texture* new_storage2d(Image* img, upload_cx* up, bool is_compute = false);
		static dvk_texture* new_storage2d(const std::string& filename, upload_cx* up, bool is_compute = false);
		static dvk_texture* new_storage2d(VkDevice dev, int width, int height, uint32_t format = 0, bool is_compute = false, bool sampled = true);
		static dvk_texture* new_render_target_color(VkDevice dev, int width, int height, uint32_t format = 0, uint32_t sampleCount = 1);
		static dvk_texture* new_render_target_depth(VkDevice dev, int width, int height, uint32_t format = 0, uint32_t sampleCount = 1);

		//Image* save2Image(Image* outimage, upload_cx* q, bool unpm);
		bool save2file(const char* fn, queuethread_cx* q, bool unpm);
		VkDescriptorImageInfo* get_descriptor_image_info();

	public:
		int32_t Init(VkDevice pDevice, VkImageCreateInfo* pCreateInfo, const char* name = nullptr);
		int32_t InitRenderTarget(VkDevice pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, VkImageUsageFlags usage, bool bUAV, const char* name = nullptr, VkImageCreateFlagBits flags = (VkImageCreateFlagBits)0);
		int32_t InitDepthStencil(VkDevice pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, const char* name = nullptr);
		bool InitFromFile(upload_cx* pUploadHeap, const char* szFilename, bool useSRGB = false, VkImageUsageFlags usageFlags = 0, float cutOff = 1.0f);
		bool InitFromData(upload_cx* uploadHeap, IMG_INFO& header, VkImageUsageFlags usageFlags = 0, bool useSRGB = false, const char* name = nullptr);

		VkImage Resource() const { return _image; }
		// 目标视图
		VkImageView CreateRTV(VkImageView* pRV, int mipLevel = -1, VkFormat format = VK_FORMAT_UNDEFINED);
		// shader资源视图
		VkImageView CreateSRV(VkImageView* pImageView, int mipLevel = -1);
		// 深度
		VkImageView CreateDSV(VkImageView* pView);
		VkImageView CreateCubeSRV(VkImageView* pImageView);
		// 创建image
		VkImage CreateTextureCommitted(upload_cx* pUploadHeap, const char* pName, bool useSRGB, VkImageUsageFlags usageFlags);
		void upload_data(upload_cx* up, IMG_INFO* info, uint32_t bufferOffset);

		void set_data(const void* data, int size, int width, int height, uint32_t format, upload_cx* up);

		//void set_data(Image* img, upload_cx* up);

		void up_yuv(yuv_info_t yuv, upload_cx* up, int wms = -1);
		void up_rgba(upload_cx* up, glm::ivec4 rc, uint32_t* data, int dw, int wms);
		void clear_color(upload_cx* up, glm::vec4 c);
	private:
		void free_image();
		static dvk_texture* new2d_priv(const std::string& filename
			, upload_cx* up
			, ImageLayoutBarrier imageLayout /*= ImageLayoutBarrier::SHADER_READ*/
			//VkImageUsageFlags
			, uint32_t imageUsageFlags /*= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT*/
		);
	public:
		void get_buffer(char* outbuf, upload_cx* q);// , dvk_queue* q);
		void copy2image(VkImage img, upload_cx* q);
		char* map();
		void unmap();
	};


	// 传输类
	class transfer_cx
	{
		struct cp2mem_t
		{
			VkImage image;
			VkBufferImageCopy icp;
			VkImageMemoryBarrier preb, postb;
			VkBuffer buffer;
		};
		struct cp2img_t
		{
			VkImage image;
			VkImageCopy icp;
			VkImageMemoryBarrier preb, postb;
			VkImage dst;
		};
		t_vector<cp2mem_t> cp2m;
		t_vector<cp2img_t> cp2img;
	public:
		void copy_image(Texture* image, Texture* dst, VkImageLayout src_layout, VkImageLayout dst_layout);
		void copy_image(VkImage image, VkImage dst, int width, int height, VkImageLayout src_layout, VkImageLayout dst_layout, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, int mipLevel = 0, int mipLevels = 1, int layerCount = 1);
		// 复制纹理到内存
		void add_copy2mem(VkImage image, VkBufferImageCopy icp, VkImageSubresourceRange subresourceRange, VkImageAspectFlags aspectMask, VkImageLayout src_layout, VkImageLayout dst, VkBuffer buffer);
		// 复制纹理到纹理
		void add_copy2img(VkImage image, VkImageCopy icp, VkImageSubresourceRange subresourceRange, VkImageAspectFlags aspectMask, VkImageLayout src_layout, VkImageLayout dst, VkImage buffer);
		void flush(VkCommandBuffer cmd1);
	};



	class GBuffer
	{
	public:

		void OnCreate(Device* pDevice, ResourceViewHeaps* pHeaps, const std::map<GBufferFlags, VkFormat>& m_formats, int sampleCount);
		void OnDestroy();

		void OnCreateWindowSizeDependentResources(/*SwapChain* pSwapChain,*/ uint32_t Width, uint32_t Height);
		void OnDestroyWindowSizeDependentResources();

		void GetAttachmentList(GBufferFlags flags, std::vector<VkImageView>* pAttachments, std::vector<VkClearValue>* pClearValues);
		VkRenderPass CreateRenderPass(GBufferFlags flags, bool bClear);

		//void GetCompilerDefines(DefineList& defines);
		VkSampleCountFlagBits  GetSampleCount() { return m_sampleCount; }
		Device* GetDevice() { return m_pDevice; }

		// depth buffer
		Texture                         m_DepthBuffer;
		VkImageView                     m_DepthBufferDSV;
		VkImageView                     m_DepthBufferSRV;

		// diffuse
		Texture                         m_Diffuse;
		VkImageView                     m_DiffuseSRV;

		// specular
		Texture                         m_SpecularRoughness;
		VkImageView                     m_SpecularRoughnessSRV;

		// motion vectors
		Texture                         m_MotionVectors;
		VkImageView                     m_MotionVectorsSRV;

		// normal buffer
		Texture                         m_NormalBuffer;
		VkImageView                     m_NormalBufferSRV;

		// HDR
		Texture                         m_HDR;		// color
		VkImageView                     m_HDRSRV;
		Texture                         m_HDRt;		// transmission
		VkImageView                     m_HDRSRVt;
#if 0
		Texture                         m_HDR_oit_accum;
		VkImageView                     m_HDR_oit_accumSRV;
		Texture                         m_HDR_oit_weight;
		VkImageView                     m_HDR_oit_weightSRV;
#endif
	private:
		Device* m_pDevice;

		VkSampleCountFlagBits           m_sampleCount;

		GBufferFlags                    m_GBufferFlags;
		std::vector<VkClearValue>       m_clearValues;

		std::map<GBufferFlags, VkFormat> m_formats;
	};




	class tfAccessor
	{
	public:
		const void* m_data = NULL;
		int m_count = 0;
		int m_stride;
		int m_dimension;
		int m_type;

		glm::vec4 m_min;
		glm::vec4 m_max;

		const void* Get(int i) const
		{
			if (i >= m_count)
				i = m_count - 1;

			return (const char*)m_data + m_stride * i;
		}

		int FindClosestFloatIndex(float val) const
		{
			int ini = 0;
			int fin = m_count - 1;

			while (ini <= fin)
			{
				int mid = (ini + fin) / 2;
				float v = *(const float*)Get(mid);

				if (val < v)
					fin = mid - 1;
				else if (val > v)
					ini = mid + 1;
				else
					return mid;
			}

			return fin;
		}
	};

	struct tfPrimitives
	{
		glm::vec4 m_center;
		glm::vec4 m_radius;
		/*
		x=类型
		'P':v2.x = 0;
		'N':v2.x = 1;
		'T':v2.x = 2;
		"TEXCOORD_0" = 3;
		"TEXCOORD_1" = 4;
		"COLOR_0" = 5;
		"COLOR_1" = 6;
		.y=accessors id
		*/
		std::vector<glm::ivec2> targets;//[“POSITION”，“NORMAL”，//“TANGENT”]
		uint32_t vcount = 0;
	};

	struct tfMesh
	{
		std::vector<tfPrimitives> m_pPrimitives;
		std::vector<float> weights;
	};

	struct Transform
	{
		//glm::quat
		glm::mat4   m_rotation = glm::identity<glm::mat4>();
		glm::vec4   m_translation = glm::vec4(0, 0, 0, 0);
		glm::vec4   m_scale = glm::vec4(1, 1, 1, 0);
		void LookAt(glm::vec4 source, glm::vec4 target, bool flipY);

		glm::mat4 GetWorldMat() const
		{
			auto scale = glm::vec3(m_scale);
			if (scale.x < 0 || scale.y < 0 || scale.z < 0) {
				//scale *= -1.0;
			}
			return glm::translate(glm::mat4(1), glm::vec3(m_translation)) * m_rotation * glm::scale(glm::mat4(1), glm::vec3(scale));
		}

	};

	typedef int tfNodeIdx;

	struct tfNode
	{
		std::vector<tfNodeIdx> m_children;

		int skinIndex = -1;
		int meshIndex = -1;
		int channel = -1;
		bool bIsJoint = false;
		bool negativescale = false;

		std::string m_name;

		Transform m_tranform;
	};

	struct NodeMatrixPostTransform
	{
		tfNode* pN; glm::mat4 m;
	};

	struct tfScene
	{
		std::vector<tfNodeIdx> m_nodes;
	};

	struct tfSkins
	{
		tfAccessor m_InverseBindMatrices;
		tfNode* m_pSkeleton = NULL;
		std::vector<int> m_jointsNodeIdx;
	};

	class tfChannel;
	class tfSampler;

	// 插值器，支持多维度linear/cubicSpline，四元数slerp/cubicSpline
	class tfInterpolator
	{
	public:
		size_t prevKey = 0;
		double prevT = 0.0;

	public:

		glm::vec4 step(size_t prevKey, float* output, int stride, std::vector<float>& rb)
		{
			glm::vec4 result = {};
			for (size_t i = 0; i < stride; ++i)
			{
				rb[i] = output[prevKey * stride + i];
			}
			memcpy(&result, rb.data(), std::min(4, stride) * sizeof(float));
			return result;
		}

		glm::vec4 linear(size_t prevKey, size_t nextKey, float* output, float t, int stride, std::vector<float>& rb)
		{
			glm::vec4 result = {};
			for (size_t i = 0; i < stride; ++i)
			{
				rb[i] = output[prevKey * stride + i] * (1 - t) + output[nextKey * stride + i] * t;
			}
			memcpy(&result, rb.data(), std::min(4, stride) * sizeof(float));
			return result;
		}
		// https://github.khronos.org/glTF-Tutorials/gltfTutorial/gltfTutorial_007_Animations.html
		template <typename T>
		T cubicSpline(const T& vert0, const T& tang0, const T& vert1, const T& tang1, float t) {
			float tt = t * t, ttt = tt * t;
			float s2 = -2 * ttt + 3 * tt, s3 = ttt - tt;
			float s0 = 1 - s2, s1 = s3 - tt + t;
			T p0 = vert0;
			T m0 = tang0;
			T p1 = vert1;
			T m1 = tang1;
			return s0 * p0 + s1 * m0 * t + s2 * p1 + s3 * m1 * t;
		}
		glm::vec4 cubicSpline(size_t prevKey, size_t nextKey, float* output, float keyDelta, float t, int stride, std::vector<float>& rb)
		{
			// stride: Count of components (4 in a quaternion).
			// Scale by 3, because each output entry consist of two tangents and one data-point.
			auto prevIndex = prevKey * stride * 3;
			auto nextIndex = nextKey * stride * 3;
			size_t A = 0;
			size_t V = 1 * stride;
			size_t B = 2 * stride;

			glm::vec4 result = {};
			float tSq = t * t;
			float tCub = tSq * t;
			// We assume that the components in output are laid out like this: in-tangent, point, out-tangent.
			// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-c-spline-interpolation
			for (size_t i = 0; i < stride; ++i)
			{
				auto v0 = output[prevIndex + i + V];
				auto a = keyDelta * output[nextIndex + i + A];
				auto b = keyDelta * output[prevIndex + i + B];
				auto v1 = output[nextIndex + i + V];
				rb[i] = ((2.0 * tCub - 3.0 * tSq + 1.0) * v0) + ((tCub - 2.0 * tSq + t) * b) + ((-2.0 * tCub + 3.0 * tSq) * v1) + ((tCub - tSq) * a);
			}
			memcpy(&result, rb.data(), std::min(4, stride) * sizeof(float));
			return result;
		}

		void resetKey()
		{
			prevKey = 0;
		}
		glm::vec4 get_v4(float* v, int idx, int stride, std::vector<float>& rb) {
			auto& r = rb;
			v += idx;
			glm::vec4 result = {};
			for (size_t i = 0; i < stride; i++)
			{
				r[i] = v[i];
			}
			memcpy(&result, rb.data(), std::max(4, stride) * sizeof(float));
			return result;
		}

		glm::quat getQuat(float* output, size_t index)
		{
			auto x = output[4 * index];
			auto y = output[4 * index + 1];
			auto z = output[4 * index + 2];
			auto w = output[4 * index + 3];
			return glm::quat(w, x, y, z);
		}
		glm::vec4 interpolate(tfChannel* channel, tfSampler* sampler, float t, float maxTime, std::vector<float>* rb);
	};

	class tfSampler
	{
	public:
		tfAccessor m_time;
		tfAccessor m_value;
		tfInterpolator interpolator = {};
		std::vector<float>* weights = 0;
		int mstride = 0;
		int interpolation = 0;	// linear=0，step=1，cubicspline=2
		int path = 0;			// 0"translation",1"rotation",2"scale",3"weights"; 
		void SampleLinear(float time, float* frac, float** pCurr, float** pNext) const
		{
			int curr_index = m_time.FindClosestFloatIndex(time);
			int next_index = std::min<int>(curr_index + 1, m_time.m_count - 1);

			if (curr_index < 0) curr_index++;

			if (curr_index == next_index)
			{
				*frac = 0;
				*pCurr = (float*)m_value.Get(curr_index);
				*pNext = (float*)m_value.Get(next_index);
				return;
			}

			float curr_time = *(float*)m_time.Get(curr_index);
			float next_time = *(float*)m_time.Get(next_index);

			*pCurr = (float*)m_value.Get(curr_index);
			*pNext = (float*)m_value.Get(next_index);
			*frac = (time - curr_time) / (next_time - curr_time);
			assert(*frac >= 0 && *frac <= 1.0);
		}
		glm::ivec2 get_tidx(float time, float* frac) const
		{
			int curr_index = m_time.FindClosestFloatIndex(time);
			int next_index = std::min<int>(curr_index + 1, m_time.m_count - 1);

			if (curr_index < 0) curr_index++;

			if (curr_index == next_index)
			{
				*frac = 0;
				return { curr_index,next_index };
			}

			float curr_time = *(float*)m_time.Get(curr_index);
			float next_time = *(float*)m_time.Get(next_index);
			*frac = (time - curr_time) / (next_time - curr_time);
			assert(*frac >= 0 && *frac <= 1.0);
			return { curr_index,next_index };
		}
	};
	class tfChannel
	{
	public:
		~tfChannel()
		{
			for (size_t i = 0; i < 4; i++)
			{
				if (sampler[i])
					delete sampler[i];
				sampler[i] = 0;
			}
		}
		tfSampler* sampler[4] = {};

	};

	glm::vec4 tfInterpolator::interpolate(tfChannel* channel, tfSampler* sampler, float t, float maxTime, std::vector<float>* rb)
	{
		if (!(t > 0) || !sampler)
		{
			return {};
		}
		int stride = sampler->mstride > 0 ? sampler->mstride : sampler->m_value.m_dimension;
		size_t ilength = sampler->m_time.m_count;
		size_t vlength = sampler->m_value.m_count;
		float* input = (float*)sampler->m_time.m_data;
		float* output = (float*)sampler->m_value.m_data;

		std::vector<float> rb1;
		if (!rb)rb = &rb1;
		rb->resize(stride);

		if (vlength == 1) // no interpolation for single keyFrame animations
		{
			return get_v4(output, 0, stride, *rb);
		}
		// Wrap t around, so the animation loops.
		// Make sure that t is never earlier than the first keyframe and never later then the last keyframe.
		t = fmod(t, maxTime);
		t = glm::clamp(t, input[0], input[ilength - 1]);
		if (prevT > t)
		{
			prevKey = 0;
		}
		prevT = t;
		// Find next keyframe: min{ t of input | t > prevKey }
		size_t nextKey = 0;
		for (size_t i = prevKey; i < ilength; ++i)
		{
			if (t <= input[i])
			{
				nextKey = glm::clamp(i, (size_t)1, ilength - 1);
				break;
			}
		}
		prevKey = glm::clamp(nextKey - 1, (size_t)0, nextKey);

		auto keyDelta = input[nextKey] - input[prevKey];

		// Normalize t: [t0, t1] -> [0, 1]
		float tn = 0.0;
		if (nextKey != prevKey)
			tn = (t - input[prevKey]) / keyDelta;
		else
			tn = tn;
		// 0"translation",1"rotation",2"scale",3"weights";
		if (sampler->path == 1)
		{
			glm::quat qr = {};
			//linear=0，step=1，cubicspline=2
			switch (sampler->interpolation)
			{
			case 0:
			{
				auto q0 = getQuat(output, prevKey);
				auto q1 = getQuat(output, nextKey);
				auto r = glm::normalize(glm::slerp(q0, q1, tn));
				glm::quat q(r.w, r.x, r.y, r.z);
				qr = q;
			}
			break;
			case 1:
			{
				qr = glm::normalize(getQuat(output, prevKey));
			}
			break;
			case 2:
			{
				// GLTF requires cubic spline interpolation for quaternions.
				// https://github.com/KhronosGroup/glTF/issues/1386
				auto r = cubicSpline(prevKey, nextKey, output, keyDelta, tn, 4, *rb);
				glm::quat q(r.w, r.x, r.y, r.z);
				qr = glm::normalize(q);
			}
			break;
			default:
				break;
			}
			*rb = { qr.x, qr.y, qr.z, qr.w };
			return glm::vec4(qr.x, qr.y, qr.z, qr.w);
		}
		glm::vec4 ret = {};
		switch (sampler->interpolation)
		{
		case 1:
			ret = step(prevKey, output, stride, *rb);
			break;
		case 2:
			ret = cubicSpline(prevKey, nextKey, output, keyDelta, tn, stride, *rb);
			break;
		default:
			ret = linear(prevKey, nextKey, output, tn, stride, *rb);
			break;
		}
		return ret;
	}

	struct tfAnimation
	{
		float m_duration;
		std::map<int, tfChannel> m_channels;
	};

	struct tfLight
	{
		enum LightType { LIGHT_DIRECTIONAL, LIGHT_POINTLIGHT, LIGHT_SPOTLIGHT };

		glm::vec4 m_color = glm::vec4(1.0);
		int m_type = LIGHT_DIRECTIONAL;
		float	m_range = 50;
		float	m_intensity = 0.0f;
		float	m_innerConeAngle = 0.0f;
		float	m_outerConeAngle = 0.0f;
		uint32_t m_shadowResolution = 1024;
		float	m_bias = 0.01;// 70.0f / 100000.0f;//0-2
	};

	struct LightInstance
	{
		int m_lightId = -1;
		tfNodeIdx m_nodeIndex = -1;
	};

	struct tfCamera
	{
		enum LightType { CAMERA_PERSPECTIVE };
		float yfov, zfar, znear;

		tfNodeIdx m_nodeIndex = -1;
	};


	glm::vec4 PolarToVector(float roll, float pitch);
	glm::mat4 LookAtRH(const glm::vec4& eyePos, const glm::vec4& lookAt, bool flipY);
	glm::mat4 LookAtRH(const glm::vec3& eyePos, const glm::vec3& lookAt, bool flipY);
	glm::mat4 lookatlh(const glm::vec3& eyePos, const glm::vec3& lookAt, bool flipY);
	glm::vec4 MoveWASD(const bool keyDown[256]);

	glm::vec4 get_row(const glm::mat4& m, int n);

	struct AxisAlignedBoundingBox
	{
		glm::vec4 m_min;
		glm::vec4 m_max;
		bool m_isEmpty;

		AxisAlignedBoundingBox();

		void Merge(const AxisAlignedBoundingBox& bb);
		void Grow(const glm::vec4 v);

		bool HasNoVolume() const;
	};

	// todo gltf解析后
	class GLTFCommon
	{
	public:
#ifdef TINY_GLTF_H_
		tinygltf::Model* pm = 0;
#endif
		std::string m_path;
		std::vector<tfScene> m_scenes;
		std::vector<tfMesh> m_meshes;
		std::vector<tfSkins> m_skins;
		//std::vector<tfLight> m_lights;
		std::vector<LightInstance> m_lightInstances;
		std::vector<tfCamera> m_cameras;

		std::vector<tfNode> m_nodes;
		std::vector<glm::ivec3> _meshskin;
		std::vector<tfAnimation> m_animations;
		std::vector<float> acv[4];				// 缓存动画结果	
		std::vector<char*> m_buffersData;		// 原始数据
		std::vector<float*> _sparseData;		// 解码的稀疏数据
		std::vector<hz::mfile_t*> _fileData;	// 文件数据
		//std::vector<char*> _fileData_f;		// 原始数据

		std::vector<glm::mat4> _mats;			// 动画矩阵和蒙皮矩阵
		glm::mat4* m_animatedMats = 0;			// _mats的指针。object space matrices of each node after being animated
		std::map<int, std::vector<float>> m_animated_morphWeights;// 变形插值数据
		std::vector<Matrix2> m_worldSpaceMats;     // world space matrices of each node after processing the hierarchy
		struct matp2 {
			glm::mat4* m = 0;
			size_t count = 0;
		};
		std::map<glm::ivec2, matp2> m_worldSpaceSkeletonMats; // skinning matrices, following the m_jointsNodeIdx order
		std::map<int, std::vector<glm::mat3x4>> m_uv_mats; // UVmat

		std::map<int, std::vector<glm::vec3>> targets_data;
		std::map<int, std::vector<glm::dvec3>> targets_datad;
		std::vector<glm::mat4> instance_mat;
		glm::vec3 _pos = {};
		float _scale = 1.0;
		uint32_t instanceCount = 1;
		uint32_t _animationIndex = 0;
		size_t mat_count = 0;	// 矩阵数量
		size_t ubo_size = 0;
		bool has_shadowMap = true;
		std::vector<PBRMaterial> _materialsData;
		std::vector<PBRMesh> _meshes;
		std::vector<char> _ubo_md;
		std::vector<std::string> variants;
		PerFrame_t* _perFrameData = 0;
		PerFrame_t* _perFrameData_w = 0;
	public:
		~GLTFCommon();
		bool Load(const std::string& path, const std::string& filename);
		void Unload();

		// misc functions
		int FindMeshSkinId(int meshId) const;
		int GetInverseBindMatricesBufferSizeByID(int id) const;
		void GetBufferDetails(int accessor, tfAccessor* pAccessor);
		void GetAttributesAccessors(const njson& gltfAttributes, std::vector<char*>* pStreamNames, std::vector<tfAccessor>* pAccessors);

		uint8_t* buffer_view_data(size_t bvidx, size_t pos);
		// transformation and animation functions
		void update(float time);
		void SetAnimationTime(uint32_t animationIndex, float time);
		void TransformScene(int sceneIndex, const glm::mat4& world);
		// 运行中使用
		PerFrame_t* SetPerFrameData(PerFrame_t* pfd, const Camera& cam, std::vector<light_t>& lights, std::vector<glm::mat4>& lightMats);
		PerFrame_t* SetPerFrameData_w(PerFrame_t* pfd, const Camera& cam, std::vector<light_t>& lights, std::vector<glm::mat4>& lightMats);
		bool GetCamera(uint32_t cameraIdx, Camera* pCam) const;
		tfNodeIdx AddNode(const tfNode& node);
		int AddLight(const tfNode& node, const tfLight& light);
		int get_animation_count();
		AxisAlignedBoundingBox get_BoundingBox(const glm::mat4& mLightView);

		glm::mat4 ComputeDirectionalLightOrthographicMatrix(const glm::mat4& mLightView);
	private:
		void InitTransformedData(); //this is called after loading the data from the GLTF
		void TransformNodes(const glm::mat4& world, const std::vector<tfNodeIdx>* pNodes);
		void make_mat(const glm::mat4& world, const std::vector<tfNodeIdx>* pNodes, std::vector<glm::mat4>* opt);
		void load_Buffers();
		void load_Meshes();
		void load_lights();
		void load_cameras();
		void load_nodes();
		void load_scenes();
		void load_skins();
		void load_animations();

	};
	struct morph_t
	{
		std::map<std::string, size_t> defs;
		VkDescriptorBufferInfo mdb = {};
		size_t targetCount = 0;
	};
	struct mesh_mapd_ptr {
		morph_t* m = {};
		VkDescriptorBufferInfo* uvtdata = {};
	};

	class gltf_gpu_res_cx
	{
	public:
		Device* m_pDevice = 0;
		UploadHeap* m_pUploadHeap = 0;
		std::vector<Texture> m_textures;
		std::vector<VkImageView> m_textureViews;
		// x是skin,y是mesh
		std::map<glm::ivec2, VkDescriptorBufferInfo> m_skeletonMatricesBuffer;

		ResourceViewHeaps* _pResourceViewHeaps = 0;		// 管理set分配
		//DynamicBufferRing* _pDynamicBufferRing = 0;	// 动态常量缓冲区ubo
		DynamicBufferRing* _p_static_buffer = 0;		// 静态缓冲区ubo
		StaticBufferPool* _pStaticBufferPool = 0;		// 静态顶点/索引缓冲区
	public:
		// maps GLTF ids into views
		std::map<int, VkDescriptorBufferInfo> m_vertexBufferMap;
		std::map<int, VkDescriptorBufferInfo> m_IndexBufferMap;
		std::map<int, morph_t> m_BufferMap;	// 材质id, 
		std::map<int, VkDescriptorBufferInfo> _morphWeights;	// 节点 mesh id访问
		std::map<int, VkDescriptorBufferInfo> m_uvmMap;
		VkDescriptorBufferInfo instance_info = {};		// 实例矩阵
		int _indextype = 0;
		uint32_t instanceCount = 1;
	public:
		GLTFCommon* m_pGLTFCommon;

		VkDescriptorBufferInfo m_perFrameConstants = {};
		VkDescriptorBufferInfo m_perFrameConstants_w = {}; // 线框

		~gltf_gpu_res_cx();
		bool OnCreate(Device* pDevice, GLTFCommon* pGLTFCommon, UploadHeap* pUploadHeap);
		void LoadTextures(bool async_de);
		void LoadGeometry();
		void OnDestroy();

		void CreateIndexBuffer(int indexBufferId, uint32_t* pNumIndices, VkIndexType* pIndexType, VkDescriptorBufferInfo* pIBV);
		void CreateGeometry(int indexBufferId, std::vector<int>& vertexBufferIds, Geometry* pGeometry);
		void CreateGeometry(tinygltf::Primitive* primitive, const std::vector<std::string> requiredAttributes, std::vector<VkVertexInputAttributeDescription>& layout, DefineList& defines, Geometry* pGeometry, int  instance_count);

		VkImageView GetTextureViewByID(int id);

		VkDescriptorBufferInfo* GetSkinningMatricesBuffer(const glm::ivec2& skinIndex);
		VkDescriptorBufferInfo* get_mb(int idx);
		VkDescriptorBufferInfo* get_uvm(int idx);
		VkDescriptorBufferInfo* get_instance_info();
		void SetSkinningMatricesForSkeletons(DynamicBufferRing* ubo);
		void SetPerFrameConstants(DynamicBufferRing* ubo, bool bWireframe);
	};
	// todo post

	class PostProcPS
	{
	public:
		void OnCreate(
			Device* pDevice,
			VkRenderPass renderPass,
			const std::string& shaderFilename,
			const std::string& shaderEntryPoint,
			const std::string& shaderCompilerParams,
			StaticBufferPool* pStaticBufferPool,
			DynamicBufferRing* pDynamicBufferRing,
			VkDescriptorSetLayout descriptorSetLayout,
			VkPipelineColorBlendStateCreateInfo* pBlendDesc = NULL,
			VkSampleCountFlagBits sampleDescCount = VK_SAMPLE_COUNT_1_BIT
		);
		void OnDestroy();
		void UpdatePipeline(VkRenderPass renderPass, VkPipelineColorBlendStateCreateInfo* pBlendDesc = NULL, VkSampleCountFlagBits sampleDescCount = VK_SAMPLE_COUNT_1_BIT);
		void Draw(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo* pConstantBuffer, VkDescriptorSet descriptorSet = NULL);

	private:
		Device* m_pDevice;
		std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
		std::string m_fragmentShaderName;

		// all bounding boxes of all the meshes use the same geometry, shaders and pipelines.
		uint32_t m_NumIndices;
		VkIndexType m_indexType;
		VkDescriptorBufferInfo m_IBV;

		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

	};
	class SkyDome
	{
	public:
		void OnCreate(Device* pDevice, VkRenderPass renderPass, UploadHeap* pUploadHeap, VkFormat outFormat, ResourceViewHeaps* pResourceViewHeaps, DynamicBufferRing* pDynamicBufferRing, StaticBufferPool* pStaticBufferPool, const char* pDiffuseCubemap, const char* pSpecularCubemap, VkSampleCountFlagBits sampleDescCount);
		void OnDestroy();
		void Draw(VkCommandBuffer cmd_buf, const glm::mat4& invViewProj);
		void GenerateDiffuseMapFromEnvironmentMap();

		void SetDescriptorDiff(uint32_t index, VkDescriptorSet descriptorSet);
		void SetDescriptorSpec(uint32_t index, VkDescriptorSet descriptorSet);

		VkImageView GetCubeDiffuseTextureView() const;
		VkImageView GetCubeSpecularTextureView() const;
		VkSampler GetCubeDiffuseTextureSampler() const;
		VkSampler GetCubeSpecularTextureSampler() const;

	private:
		Device* m_pDevice;

		ResourceViewHeaps* m_pResourceViewHeaps;

		Texture m_CubeDiffuseTexture;
		Texture m_CubeSpecularTexture;

		VkImageView m_CubeDiffuseTextureView;
		VkImageView m_CubeSpecularTextureView;

		VkSampler m_samplerDiffuseCube, m_samplerSpecularCube;

		VkDescriptorSet       m_descriptorSet;
		VkDescriptorSetLayout m_descriptorLayout;

		PostProcPS  m_skydome;

		DynamicBufferRing* m_pDynamicBufferRing = NULL;
		glm::vec4 default_specular = glm::vec4(0.5);	// 默认环境高光值
	};

#define BLURPS_MAX_MIP_LEVELS 12

	// Implements a simple separable gaussian blur

	class BlurPS
	{
	public:
		void OnCreate(
			Device* pDevice,
			ResourceViewHeaps* pResourceViewHeaps,
			DynamicBufferRing* pConstantBufferRing,
			StaticBufferPool* pStaticBufferPool,
			VkFormat format
		);
		void OnDestroy();

		void OnCreateWindowSizeDependentResources(Device* pDevice, uint32_t Width, uint32_t Height, Texture* pInput, int mipCount);
		void OnDestroyWindowSizeDependentResources();

		void Draw(VkCommandBuffer cmd_buf, int mipLevel);
		void Draw(VkCommandBuffer cmd_buf);

	private:
		Device* m_pDevice;

		ResourceViewHeaps* m_pResourceViewHeaps;
		DynamicBufferRing* m_pConstantBufferRing;

		VkFormat                   m_outFormat;

		uint32_t                   m_Width;
		uint32_t                   m_Height;
		int                        m_mipCount;

		Texture* m_inputTexture;
		Texture                    m_tempBlur;

		struct Pass
		{
			VkImageView     m_RTV;
			VkImageView     m_SRV;
			VkFramebuffer   m_frameBuffer;
			VkDescriptorSet m_descriptorSet;
		};

		Pass                       m_horizontalMip[BLURPS_MAX_MIP_LEVELS];
		Pass                       m_verticalMip[BLURPS_MAX_MIP_LEVELS];

		VkDescriptorSetLayout      m_descriptorSetLayout;

		PostProcPS                 m_directionalBlur;

		VkSampler                  m_sampler;

		VkRenderPass               m_in;

		struct cbBlur
		{
			float dirX, dirY;
			int mipLevel;
		};
	};

#ifndef BLOOM_MAX_MIP_LEVELS 
#define BLOOM_MAX_MIP_LEVELS 12
#endif
	class Bloom
	{
	private:
		struct cbBlend
		{
			float weight;
		};
		struct Pass
		{
			VkImageView     m_RTV;
			VkImageView     m_SRV;
			VkFramebuffer   m_frameBuffer;
			VkDescriptorSet m_descriptorSet;
			float m_weight;
		};
		Device* m_pDevice = nullptr;
		ResourceViewHeaps* m_pResourceViewHeaps;
		DynamicBufferRing* m_pConstantBufferRing;
		Pass                       m_mip[BLOOM_MAX_MIP_LEVELS] = {};
		Pass                       m_output = {};
		BlurPS                     m_blur;
		PostProcPS                 m_blendAdd;
		VkDescriptorSetLayout      m_descriptorSetLayout;
		VkSampler                  m_sampler;
		VkRenderPass               m_blendPass;
		VkFormat                   m_outFormat;
		uint32_t                   m_Width;
		uint32_t                   m_Height;
		int                        m_mipCount;
		bool                       m_doBlur;
		bool                       m_doUpscale;

	public:
		void OnCreate(Device* pDevice, ResourceViewHeaps* pHeaps, DynamicBufferRing* pConstantBufferRing, StaticBufferPool* pResourceViewHeaps, VkFormat format);
		void OnDestroy();
		void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, Texture* pInput, int mipCount, Texture* pOutput);
		void OnDestroyWindowSizeDependentResources();
		void Draw(VkCommandBuffer cmd_buf);

		void Gui();

	};

	class ColorConversionPS
	{
	public:
		void OnCreate(Device* pDevice, VkRenderPass renderPass, ResourceViewHeaps* pResourceViewHeaps, StaticBufferPool* pStaticBufferPool, DynamicBufferRing* pDynamicBufferRing);
		void OnDestroy();

		void UpdatePipelines(VkRenderPass renderPass, DisplayMode displayMode);

		void Draw(VkCommandBuffer cmd_buf, VkImageView HDRSRV);

	private:
		Device* m_pDevice;
		ResourceViewHeaps* m_pResourceViewHeaps;

		PostProcPS m_ColorConversion;
		DynamicBufferRing* m_pDynamicBufferRing = NULL;

		VkSampler m_sampler;

		uint32_t              m_descriptorIndex;
		static const uint32_t s_descriptorBuffers = 10;

		VkDescriptorSet       m_descriptorSet[s_descriptorBuffers];
		VkDescriptorSetLayout m_descriptorSetLayout;

		struct ColorConversionConsts
		{
			glm::mat4 m_contentToMonitorRecMatrix;
			DisplayMode m_displayMode;
			float m_displayMinLuminancePerNits;
			float m_displayMaxLuminancePerNits;
		};

		ColorConversionConsts m_colorConversionConsts;
	};

#define DOWNSAMPLEPS_MAX_MIP_LEVELS 12

	class DownSamplePS
	{
	public:
		void OnCreate(Device* pDevice, ResourceViewHeaps* pResourceViewHeaps, DynamicBufferRing* m_pConstantBufferRing, StaticBufferPool* pStaticBufferPool, VkFormat outFormat);
		void OnDestroy();

		void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, Texture* pInput, int mips);
		void OnDestroyWindowSizeDependentResources();

		void Draw(VkCommandBuffer cmd_buf);
		Texture* GetTexture() { return &m_result; }
		VkImageView GetTextureView(int i) { return m_mip[i].m_SRV; }
		void Gui();

		struct cbDownscale
		{
			float invWidth, invHeight;
			int mipLevel;
		};

	private:
		Device* m_pDevice;
		VkFormat                    m_outFormat;

		Texture                     m_result;

		struct Pass
		{
			VkImageView     RTV; //dest
			VkImageView     m_SRV; //src
			VkFramebuffer   frameBuffer;
			VkDescriptorSet descriptorSet;
		};

		Pass                         m_mip[DOWNSAMPLEPS_MAX_MIP_LEVELS];

		StaticBufferPool* m_pStaticBufferPool;
		ResourceViewHeaps* m_pResourceViewHeaps;
		DynamicBufferRing* m_pConstantBufferRing;

		uint32_t                     m_Width;
		uint32_t                     m_Height;
		int                          m_mipCount;

		VkDescriptorSetLayout        m_descriptorSetLayout;

		PostProcPS                   m_downscale;

		VkRenderPass                 m_in;

		VkSampler                    m_sampler;
	};

	class PostProcCS
	{
	public:
		void OnCreate(
			Device* pDevice,
			const std::string& shaderFilename,
			const std::string& shaderEntryPoint,
			const std::string& shaderCompilerParams,
			VkDescriptorSetLayout descriptorSetLayout,
			uint32_t dwWidth, uint32_t dwHeight, uint32_t dwDepth,
			DefineList* userDefines = 0
		);
		void OnDestroy();
		void Draw(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo* pConstantBuffer, VkDescriptorSet descSet, uint32_t dispatchX, uint32_t dispatchY, uint32_t dispatchZ);

	private:
		Device* m_pDevice;

		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout;
	};

	// This renders a procedural sky, see the SkyDomeProc.glsl for more references and credits
	// todo sky
	class SkyDomeProc
	{
	public:

		struct Constants
		{
			glm::mat4 invViewProj;
			glm::vec4 vSunDirection;		// 太阳方向
			float rayleigh = 2;				// 瑞利散射，视觉效果就是傍晚晚霞的红光的深度
			float turbidity = 10;			// 浑浊度
			float mieCoefficient = 0.005;	// 散射系数
			float luminance = 1.0;			// 亮度
			float mieDirectionalG = 0.8;	// 定向散射值
		};

		void OnCreate(Device* pDevice, VkRenderPass renderPass, UploadHeap* pUploadHeap, VkFormat outFormat, ResourceViewHeaps* pResourceViewHeaps, DynamicBufferRing* pDynamicBufferRing, StaticBufferPool* pStaticBufferPool, VkSampleCountFlagBits sampleDescCount);
		void OnDestroy();
		void Draw(VkCommandBuffer cmd_buf, SkyDomeProc::Constants constants);

	private:
		Device* m_pDevice;

		ResourceViewHeaps* m_pResourceViewHeaps;

		VkDescriptorSet         m_descriptorSet;
		VkDescriptorSetLayout   m_descriptorLayout;

		PostProcPS  m_skydome;

		DynamicBufferRing* m_pDynamicBufferRing = NULL;
	};
	// todo taa
	class TAA
	{
	public:
		void OnCreate(Device* pDevice, ResourceViewHeaps* pResourceViewHeaps, StaticBufferPool* pStaticBufferPool, DynamicBufferRing* pDynamicBufferRing, bool sharpening);
		void OnDestroy();

		void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, GBuffer* pGBuffer);
		void OnDestroyWindowSizeDependentResources();

		void Draw(VkCommandBuffer cmd_buf);

	private:
		Device* m_pDevice;
		ResourceViewHeaps* m_pResourceViewHeaps;

		uint32_t              m_Width, m_Height;

		GBuffer* m_pGBuffer;

		bool                  m_TexturesInUndefinedLayout;

		Texture               m_TAABuffer;
		VkImageView           m_TAABufferSRV;
		VkImageView           m_TAABufferUAV;

		Texture               m_HistoryBuffer;
		VkImageView           m_HistoryBufferSRV;
		//VkImageView           m_HistoryBufferUAV;

		VkSampler             m_samplers[4];

		VkDescriptorSet       m_TaaDescriptorSet;
		VkDescriptorSetLayout m_TaaDescriptorSetLayout;
		PostProcCS            m_TAA;
		PostProcCS            m_TAAFirst;

		//
		VkDescriptorSet       m_SharpenDescriptorSet;
		VkDescriptorSetLayout m_SharpenDescriptorSetLayout;
		PostProcCS            m_Sharpen;
		PostProcCS            m_Post;
	public:
		bool                  m_bSharpening = true;
		bool                  m_bFirst = true;
	};
	class ToneMapping
	{
	public:
		void OnCreate(Device* pDevice, VkRenderPass renderPass, ResourceViewHeaps* pResourceViewHeaps, StaticBufferPool* pStaticBufferPool, DynamicBufferRing* pDynamicBufferRing, uint32_t srvTableSize = 1, const char* shaderSource = "Tonemapping.glsl");
		void OnDestroy();

		void UpdatePipelines(VkRenderPass renderPass);

		void Draw(VkCommandBuffer cmd_buf, VkImageView HDRSRV, float exposure, int toneMapper);

	protected:
		Device* m_pDevice;
		ResourceViewHeaps* m_pResourceViewHeaps;

		PostProcPS m_toneMapping;
		DynamicBufferRing* m_pDynamicBufferRing = NULL;

		VkSampler m_sampler;

		uint32_t              m_descriptorIndex;
		static const uint32_t s_descriptorBuffers = 10;

		VkDescriptorSet       m_descriptorSet[s_descriptorBuffers];
		VkDescriptorSetLayout m_descriptorSetLayout;

		struct ToneMappingConsts { float exposure; int toneMapper; };
	};
	class ToneMappingCS
	{
	public:
		void OnCreate(Device* pDevice, ResourceViewHeaps* pResourceViewHeaps, DynamicBufferRing* pDynamicBufferRing);
		void OnDestroy();

		void Draw(VkCommandBuffer cmd_buf, VkImageView HDRSRV, float exposure, int toneMapper, int width, int height);

	private:
		Device* m_pDevice;
		ResourceViewHeaps* m_pResourceViewHeaps;

		PostProcCS m_toneMapping;
		DynamicBufferRing* m_pDynamicBufferRing = NULL;

		uint32_t              m_descriptorIndex;
		static const uint32_t s_descriptorBuffers = 10;

		VkDescriptorSet       m_descriptorSet[s_descriptorBuffers];
		VkDescriptorSetLayout m_descriptorSetLayout;

		struct ToneMappingConsts { float exposure; int toneMapper; };
	};




	// Forward declarations
	class ResourceViewHeaps;
	class DynamicBufferRing;
	//class SwapChain;

	class MagnifierPS
	{
	public:
		void OnCreate(
			Device* pDevice
			, ResourceViewHeaps* pResourceViewHeaps
			, DynamicBufferRing* pDynamicBufferRing
			, StaticBufferPool* pStaticBufferPool
			, VkFormat outFormat
			, bool bOutputsToSwapchain = false
		);

		void OnDestroy();
		void OnCreateWindowSizeDependentResources(Texture* pTexture);
		void OnDestroyWindowSizeDependentResources();
		void BeginPass(VkCommandBuffer cmd, VkRect2D renderArea);
		void EndPass(VkCommandBuffer cmd);


		// Draws a magnified region on top of the given image to OnCreateWindowSizeDependentResources()
		// if @pSwapChain is provided, expects swapchain to be sycned before this call.
		// Barriers are to be managed by the caller.
		void Draw(VkCommandBuffer cmd, PassParameters& params);

		inline Texture& GetPassOutput() { assert(!m_bOutputsToSwapchain); return m_TexPassOutput; }
		inline VkImage GetPassOutputResource() const { assert(!m_bOutputsToSwapchain); return m_TexPassOutput.Resource(); }
		inline VkImageView GetPassOutputSRV() const { assert(!m_bOutputsToSwapchain); return m_SRVOutput; }
		inline VkRenderPass GetPassRenderPass() const { assert(!m_bOutputsToSwapchain); return m_RenderPass; }
		void UpdatePipelines(VkRenderPass renderPass);
	private:

		void CompileShaders(StaticBufferPool* pStaticBufferPool, VkFormat outFormat);
		void DestroyShaders();

		void InitializeDescriptorSets();
		void update_set(VkImageView ImageViewSrc);
		void DestroyDescriptorSets();
		VkDescriptorBufferInfo SetConstantBufferData(PassParameters& params);
		static void KeepMagnifierOnScreen(PassParameters& params);

		//--------------------------------------------------------------------------------------------------------
		// DATA
		//--------------------------------------------------------------------------------------------------------
	private:
		//
		// DEVICE & MEMORY
		//
		Device* m_pDevice = nullptr;
		ResourceViewHeaps* m_pResourceViewHeaps = nullptr;
		DynamicBufferRing* m_pDynamicBufferRing = nullptr;

		//
		// DESCRIPTOR SETS & LAYOUTS
		//
		VkDescriptorSet m_DescriptorSet;
		VkDescriptorSetLayout m_DescriptorSetLayout;

		//
		// RENDER PASSES & RESOURCE VIEWS
		//
		VkSampler   m_SamplerSrc;
		VkImageView m_ImageViewSrc;

		VkRenderPass  m_RenderPass;
		VkFramebuffer m_FrameBuffer;
		VkImageView   m_RTVOutput;
		VkImageView   m_SRVOutput;

		//
		// BUFFERS
		//
		// None.

		//
		// TEXTURES
		//
		Texture m_TexPassOutput;

		// 
		// SHADERS
		//
		PostProcPS m_ShaderMagnify;

		//
		// MISC
		//
		bool m_bOutputsToSwapchain = false;
	};











	class oitblendCS
	{
	public:
		void OnCreate(Device* pDevice, ResourceViewHeaps* pResourceViewHeaps, DynamicBufferRing* pDynamicBufferRing);
		void OnDestroy();
		void Draw(VkCommandBuffer cmd_buf, VkImageView HDRSRV, VkImageView accumTex, VkImageView weightTex, int width, int height);
	private:
		Device* m_pDevice = 0;
		ResourceViewHeaps* m_pResourceViewHeaps = 0;
		PostProcCS oitblend;
		DynamicBufferRing* m_pDynamicBufferRing = NULL;
		uint32_t              m_descriptorIndex = 0;
		static const uint32_t s_descriptorBuffers = 10;
		VkDescriptorSet       m_descriptorSet[s_descriptorBuffers];
		VkDescriptorSetLayout m_descriptorSetLayout = {};
	};


	struct ts_t {
		VkImageView v;
		VkSampler s;
	};
	struct lut_tex_t
	{
		Texture texture = {};
		VkImageView view = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
	};
	struct env_res_t
	{
		SkyDome* pSkyDome;
		std::vector<VkImageView>* ShadowMapViewPool;
		VkSampler m_samplerShadow = VK_NULL_HANDLE;
		lut_tex_t* lut = 0;// lut_ggx\lut_charlie\lut_sheen_E 
		bool bUseSSAOMask = false;
		bool use_punctual = true;
		bool is_async = true;
	};

	// todo pbr
	class GltfPbrPass
	{
	public:
		struct per_object
		{
			glm::mat4 mCurrentWorld;
			glm::mat4 mPreviousWorld;
			pbrMaterial m_pbrParams;
		};
		std::map<size_t, PBRPipe_t> _pipem;
		gltf_gpu_res_cx* _ptb = 0;
		ResourceViewHeaps* m_pResourceViewHeaps = 0;
		DynamicBufferRing* m_pDynamicBufferRing = 0;
		//StaticBufferPool* m_pStaticBufferPool;

		std::vector<size_t> _nodes;
		std::vector<PBRMesh> m_meshes;
		std::vector<PBRMaterial> m_materialsData;
		std::vector<VkSampler> _samplers;
		PBRMaterial m_defaultMaterial = {};
		Device* m_pDevice = 0;
		GBufferRenderPass* m_pRenderPass = 0;
		VkSampler m_samplerPbr = VK_NULL_HANDLE;
		// PBR Brdf
		//Texture m_brdfLutTexture = {};
		//VkImageView m_brdfLutView = VK_NULL_HANDLE;
		//VkSampler m_brdfLutSampler = VK_NULL_HANDLE;
		env_res_t* _envr = 0;
	public:
		GltfPbrPass();
		~GltfPbrPass();
		void OnCreate(Device* pDevice, UploadHeap* pUploadHeap, ResourceViewHeaps* pHeaps, DynamicBufferRing* pDynamicBufferRing, gltf_gpu_res_cx* pGLTFTexturesAndBuffers
			, env_res_t* r//SkyDome* pSkyDome, bool bUseSSAOMask, std::vector<VkImageView>& ShadowMapViewPool,
			, GBufferRenderPass* pRenderPass);
		void OnDestroy();
		void BuildBatchLists(drawables_t* opt, bool bWireframe = false);
		static void DrawBatchList(Device* dev, VkCommandBuffer commandBuffer, std::vector<BatchList>* pBatchList, bool bWireframe = false);
		void OnUpdateWindowSizeDependentResources(VkImageView SSAO);
		GLTFCommon* get_cp();

		PBRPipe_t* get_pipem(DefineList* defines);
	private:
		void CreateDescriptorTableForMaterialTextures(PBRMaterial* tfmat, std::map<std::string, ts_t>& texturesBase, env_res_t* r);
		//	, SkyDome* pSkyDome, std::vector<VkImageView>& ShadowMapViewPool, bool bUseSSAOMask);
		void CreateDescriptors(int inverseMatrixBufferSize, DefineList* pAttributeDefines, PBRPrimitives* pPrimitive, mesh_mapd_ptr* m, int muvt_size, bool bUseSSAOMask);
		void CreatePipeline(std::vector<VkVertexInputAttributeDescription>& layout, const DefineList& defines, PBRPrimitives* pPrimitive);
	};

	bool bcmp(const BatchList& l, const BatchList& r)
	{
		return -l.m_depth < -r.m_depth;
	}




	struct DepthMaterial
	{
		int m_textureCount = 0;
		VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

		DefineList m_defines;
		bool m_doubleSided = false;
		bool m_blending = false;
	};

	struct DepthPrimitives
	{
		Geometry m_geometry;

		DepthMaterial* m_pMaterial = NULL;

		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

		VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;


	};

	struct DepthMesh
	{
		std::vector<DepthPrimitives> m_pPrimitives;
	};
	// todo depth
	class GltfDepthPass
	{
	public:
		struct PerFrame_t
		{
			glm::mat4 mViewProj;
		};

		struct depth_object
		{
			glm::mat4 mWorld;
		};
		bool has_shadowMap = true;
		void OnCreate(
			Device* pDevice,
			VkRenderPass renderPass,
			UploadHeap* pUploadHeap,
			ResourceViewHeaps* pHeaps,
			DynamicBufferRing* pDynamicBufferRing,
			StaticBufferPool* pStaticBufferPool,
			gltf_gpu_res_cx* pGLTFTexturesAndBuffers,
			AsyncPool* pAsyncPool = NULL);

		void load(AsyncPool* pAsyncPool);

		void OnDestroy();
		GltfDepthPass::PerFrame_t* SetPerFrameConstants();
		void Draw(VkCommandBuffer cmd_buf);
		GLTFCommon* get_cp();
	private:
		ResourceViewHeaps* m_pResourceViewHeaps;
		DynamicBufferRing* m_pDynamicBufferRing;
		//StaticBufferPool* m_pStaticBufferPool;

		std::vector<DepthMesh> m_meshes;
		std::vector<DepthMaterial> m_materialsData;

		DepthMaterial m_defaultMaterial;

		gltf_gpu_res_cx* _ptb;
		Device* m_pDevice;
		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		VkSampler m_sampler = VK_NULL_HANDLE;
		VkDescriptorBufferInfo m_perFrameDesc;
	public:
		bool m_bInvertedDepth = false;

		void CreateDescriptors(int inverseMatrixBufferSize, DefineList* pAttributeDefines, DepthPrimitives* pPrimitive, morph_t* morphing);
		void CreatePipeline(std::vector<VkVertexInputAttributeDescription>& layout, const DefineList& defines, DepthPrimitives* pPrimitive);
	};

	class Wireframe
	{
	public:
		void OnCreate(
			Device* pDevice,
			VkRenderPass renderPass,
			ResourceViewHeaps* pHeaps,
			DynamicBufferRing* pDynamicBufferRing,
			StaticBufferPool* pStaticBufferPool,
			VkSampleCountFlagBits sampleDescCount);

		void OnDestroy();
		void Draw(VkCommandBuffer cmd_buf, int numIndices, VkDescriptorBufferInfo IBV, VkDescriptorBufferInfo VBV, const glm::mat4& worldMatrix, const glm::vec4& vCenter, const glm::vec4& vRadius, const glm::vec4& vColor);

	private:
		Device* m_pDevice;

		DynamicBufferRing* m_pDynamicBufferRing;
		ResourceViewHeaps* m_pResourceViewHeaps;

		VkPipeline m_pipeline;
		VkPipelineLayout m_pipelineLayout;

		VkDescriptorSet             m_descriptorSet;
		VkDescriptorSetLayout       m_descriptorSetLayout;

		struct wireframe_object
		{
			glm::mat4 m_mWorldViewProj;
			glm::vec4 m_vCenter;
			glm::vec4 m_vRadius;
			glm::vec4 m_vColor;
		};
	};
	void GenerateSphere(int sides, std::vector<unsigned short>& outIndices, std::vector<float>& outVertices);
	void GenerateBox(std::vector<unsigned short>& outIndices, std::vector<float>& outVertices);
	class WireframeBox : public Wireframe
	{
	public:
		// all bounding boxes of all the meshes use the same geometry, shaders and pipelines.
		uint32_t m_NumIndices;
		VkDescriptorBufferInfo m_IBV;
		VkDescriptorBufferInfo m_VBV;
	public:
		void OnCreate(Device* pDevice, ResourceViewHeaps* pResourceViewHeaps, DynamicBufferRing* pDynamicBufferRing, StaticBufferPool* pStaticBufferPool)
		{
			std::vector<unsigned short> indices;
			std::vector<float> vertices;

			GenerateBox(indices, vertices);

			// set indices
			m_NumIndices = (uint32_t)indices.size();
			pStaticBufferPool->AllocBuffer(m_NumIndices, sizeof(short), indices.data(), &m_IBV);
			pStaticBufferPool->AllocBuffer((uint32_t)(vertices.size() / 3), (uint32_t)(3 * sizeof(float)), vertices.data(), &m_VBV);
		}

		void OnDestroy() {}

		void Draw(VkCommandBuffer cmd_buf, Wireframe* pWireframe, const glm::mat4& worldMatrix, const glm::vec4& vCenter, const glm::vec4& vRadius, const glm::vec4& vColor)
		{
			pWireframe->Draw(cmd_buf, m_NumIndices, m_IBV, m_VBV, worldMatrix, vCenter, vRadius, vColor);
		}
	};
	class WireframeSphere : public Wireframe
	{
	public:
		// all bounding boxes of all the meshes use the same geometry, shaders and pipelines.
		uint32_t m_NumIndices;
		VkDescriptorBufferInfo m_IBV;
		VkDescriptorBufferInfo m_VBV;
	public:
		void OnCreate(
			Device* pDevice,
			ResourceViewHeaps* pResourceViewHeaps,
			DynamicBufferRing* pDynamicBufferRing,
			StaticBufferPool* pStaticBufferPool)
		{
			std::vector<unsigned short> indices;
			std::vector<float> vertices;

			GenerateSphere(16, indices, vertices);

			// set indices
			m_NumIndices = (uint32_t)indices.size();
			pStaticBufferPool->AllocBuffer(m_NumIndices, sizeof(short), indices.data(), &m_IBV);
			pStaticBufferPool->AllocBuffer((uint32_t)(vertices.size() / 3), (uint32_t)(3 * sizeof(float)), vertices.data(), &m_VBV);
		}

		void OnDestroy() {}

		void Draw(VkCommandBuffer cmd_buf, Wireframe* pWireframe, const glm::mat4& worldMatrix, const glm::vec4& vCenter, const glm::vec4& vRadius, const glm::vec4& vColor)
		{
			pWireframe->Draw(cmd_buf, m_NumIndices, m_IBV, m_VBV, worldMatrix, vCenter, vRadius, vColor);
		}
	};
	class GltfBBoxPass
	{
	public:
		void OnCreate(
			Device* pDevice,
			VkRenderPass renderPass,
			ResourceViewHeaps* pHeaps,
			DynamicBufferRing* pDynamicBufferRing,
			StaticBufferPool* pStaticBufferPool,
			gltf_gpu_res_cx* pGLTFTexturesAndBuffers,
			Wireframe* pWireframe);

		void OnDestroy();
		void Draw(VkCommandBuffer cmd_buf, const glm::mat4& cameraViewProjMatrix, const glm::vec4& color);
		inline void Draw(VkCommandBuffer cmd_buf, const glm::mat4& cameraViewProjMatrix) { Draw(cmd_buf, cameraViewProjMatrix, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); }
	private:
		gltf_gpu_res_cx* _ptb;

		Wireframe* m_pWireframe;
		WireframeBox m_wireframeBox;
	};


	/*
		GltfPbrPass* m_GLTFPBR;
		GltfBBoxPass* m_GLTFBBox;
		GltfDepthPass* m_GLTFDepth;
	*/

	void AttachClearBeforeUse(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription* pAttachDesc);
	void AttachNoClearBeforeUse(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription* pAttachDesc);
	void AttachBlending(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription* pAttachDesc);
	VkRenderPass CreateRenderPassOptimal(VkDevice device, uint32_t colorAttachments, VkAttachmentDescription* pColorAttachments, VkAttachmentDescription* pDepthAttachment);

	// Sets the viewport and the scissor to a fixed height and width
	//
	void SetViewportAndScissor(VkCommandBuffer cmd_buf, uint32_t topX, uint32_t topY, uint32_t width, uint32_t height);

	// Creates a Render pass that will discard the contents of the render target.
	//
	VkRenderPass SimpleColorWriteRenderPass(VkDevice device, VkImageLayout initialLayout, VkImageLayout passLayout, VkImageLayout finalLayout, uint32_t format);

	// Creates a Render pass that will use the contents of the render target for blending.
	//
	VkRenderPass SimpleColorBlendRenderPass(VkDevice device, VkImageLayout initialLayout, VkImageLayout passLayout, VkImageLayout finalLayout, uint32_t format);

	// Sets the i-th Descriptor Set entry to use a given image view + sampler. The sampler can be null is a static one is being used.
	//
	//void SetDescriptorSet(VkDevice device, uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet);
	//void SetDescriptorSet(VkDevice device, uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkSampler pSampler, VkDescriptorSet descriptorSet);
	//void SetDescriptorSet(VkDevice device, uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkImageLayout imageLayout, VkSampler pSampler, VkDescriptorSet descriptorSet);

	//void SetDescriptorSetForDepth(VkDevice device, uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet);
	//void SetDescriptorSet(VkDevice device, uint32_t index, VkImageView imageView, VkDescriptorSet descriptorSet);

	//VkFramebuffer CreateFrameBuffer(VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView>* pAttachments, uint32_t Width, uint32_t Height);


	// misc

	static constexpr float AMD_PI = 3.1415926535897932384626433832795f;
	static constexpr float AMD_PI_OVER_2 = 1.5707963267948966192313216916398f;
	static constexpr float AMD_PI_OVER_4 = 0.78539816339744830961566084581988f;

	double MillisecondsNow();
	bool readfile(const char* name, char** data, size_t* size, bool isbinary);
	bool savefile(const char* name, void const* data, size_t size, bool isbinary);
	void Trace(const std::string& str);
	void Trace(const char* pFormat, ...);
	bool LaunchProcess(const char* commandLine, const char* filenameErr);

	inline void GetXYZ(float* f, glm::vec4 v)
	{
		f[0] = v.x;
		f[1] = v.y;
		f[2] = v.z;
	}

	bool CameraFrustumToBoxCollision(const glm::mat4& mCameraViewProj, const glm::vec4& boxCenter, const glm::vec4& boxExtent);



	AxisAlignedBoundingBox GetAABBInGivenSpace(const glm::mat4& mTransform, const glm::vec4& boxCenter, const glm::vec4& boxExtent);

	//// align val to the next multiple of alignment
	//template<typename T> inline T AlignUp(T val, T alignment)
	//{
	//	return (val + alignment - (T)1) & ~(alignment - (T)1);
	//}
	//// align val to the previous multiple of alignment
	//template<typename T> inline T AlignDown(T val, T alignment)
	//{
	//	return val & ~(alignment - (T)1);
	//}
	//template<typename T> inline T DivideRoundingUp(T a, T b)
	//{
	//	return (a + b - (T)1) / b;
	//}

	class Profile
	{
		double m_startTime;
		const char* m_label;
	public:
		Profile(const char* label) {
			m_startTime = MillisecondsNow(); m_label = label ? label : "";
		}
		~Profile() {
			printf("*** %s  %f ms\n", m_label, (MillisecondsNow() - m_startTime));
		}
	};

	class Log
	{
	public:
		static int InitLogSystem();
		static int TerminateLogSystem();
		static void Trace(const char* LogString);

	private:
		static Log* m_pLogInstance;

		Log();
		virtual ~Log();

		void Write(const char* LogString);

		HANDLE m_FileHandle = INVALID_HANDLE_VALUE;
#define MAX_INFLIGHT_WRITES 32

		OVERLAPPED m_OverlappedData[MAX_INFLIGHT_WRITES] = {};
		uint32_t m_CurrentIOBufferIndex = 0;

		uint32_t m_WriteOffset = 0;
		FILE* _fp = 0;
	};

	int countBits(uint32_t v);

	enum ShaderSourceType
	{
		SST_HLSL,
		SST_GLSL
	};


	// Does as the function name says and uses a cache
	VkResult VKCompileFromString(VkDevice device, ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const char* pShaderCode, const char* pShaderEntryPoint, const char* pExtraParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader);
	VkResult VKCompileFromFile(VkDevice device, const VkShaderStageFlagBits shader_type, const char* pFilename, const char* pShaderEntryPoint, const char* pExtraParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader);


	//Loads a DDS file

	class DDSLoader : public ImgLoader
	{
	public:
		~DDSLoader();
		bool Load(const char* pFilename, float cutOff, IMG_INFO* pInfo);
		// after calling Load, calls to CopyPixels return each time a lower mip level 
		void CopyPixels(void* pDest, uint32_t stride, uint32_t width, uint32_t height);
	private:
		HANDLE m_handle = INVALID_HANDLE_VALUE;
	};

	// Loads a JPEGs, PNGs, BMPs and any image the Windows Imaging Component can load.
	// It even applies some alpha scaling to prevent cutouts to fade away when lower mips are used.

	class WICLoader : public ImgLoader
	{
	public:
		~WICLoader();
		bool Load(const char* pFilename, float cutOff, IMG_INFO* pInfo);
		// after calling Load, calls to CopyPixels return each time a lower mip level 
		void CopyPixels(void* pDest, uint32_t stride, uint32_t width, uint32_t height);
	private:
		void MipImage(uint32_t width, uint32_t height);
		// scale alpha to prevent thinning when lower mips are used
		float GetAlphaCoverage(uint32_t width, uint32_t height, float scale, int cutoff) const;
		void ScaleAlpha(uint32_t width, uint32_t height, float scale);

		char* m_pData;

		float m_alphaTestCoverage;
		float m_cutOff;
	};

	void GetSrgbAndCutOffOfImageGivenItsUse(int imageIndex, const std::vector<tinygltf::Material>* p, bool* pSrgbOut, float* pCutoff);



	// todo renderer 渲染器
	class Renderer_cx
	{
	public:
		Renderer_cx(const_vk* p);
		~Renderer_cx();
		void OnCreate(Device* pDevice, VkRenderPass rp);
		void OnDestroy();

		void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height);
		void OnDestroyWindowSizeDependentResources();
		void OnUpdateDisplayDependentResources(VkRenderPass rp, DisplayMode dm, bool bUseMagnifier);
		void OnUpdateLocalDimmingChangedResources(VkRenderPass rp, DisplayMode dm);

		int load_model(GLTFCommon* pGLTFCommon, int Stage = 0);
		void UnloadScene();
		void unloadgltf(robj_info* p);

		const std::vector<TimeStamp>& GetTimingValues() { return m_TimeStamps; }

		PerFrame_t* mkPerFrameData(const Camera& cam);
		void OnRender(scene_state* pState, const Camera& Cam);
		void set_fbo(fbo_info_cx* p, int idx);
		// 释放上传堆和缓冲区
		void freeVidMBP();


		size_t AddLight(const light_t& light);
		light_t* get_light(size_t idx);
		size_t get_light_size();
		void AllocateShadowMaps();

		void new_shadow(SceneShadowInfo& ShadowInfo, uint32_t shadowResolution, int shadows, int idx);
		void init_envres();
		void un_envres();
	private:
		void draw_skydome(VkCommandBuffer cmdBuf1, const scene_state* pState, const glm::mat4& mCameraCurrViewProj);

		void draw_pbr_opaque(VkCommandBuffer cmdBuf1, const scene_state* pState, bool bWireframe);
		void draw_pbr_transparent(VkCommandBuffer cmdBuf1, const scene_state* pState, bool bWireframe);
		void copy_transmission(VkCommandBuffer cmdBuf1);
		void draw_pbr_transmission(VkCommandBuffer cmdBuf1, const scene_state* pState, bool bWireframe);
		void draw_boxes(VkCommandBuffer cmdBuf1, PerFrame_t* pf, const scene_state* pState);
	public:
		Device* m_pDevice = 0;
		const_vk ct = {};
		uint32_t                        m_Width = {};
		uint32_t                        m_Height = {};

		VkRect2D                        m_RectScissor = {};
		VkViewport                      m_Viewport = {};

		// todo Initialize helper classes资源管理
		ResourceViewHeaps               m_ResourceViewHeaps = {};	// set管理
		UploadHeap                      m_UploadHeap = {};			// 纹理上传堆
		DynamicBufferRing               m_ConstantBufferRing = {};	// 动态常量缓冲区
		StaticBufferPool                m_VidMemBufferPool = {};	// 静态顶点/索引缓冲区
		StaticBufferPool                m_SysMemBufferPool = {};	// 系统静态几何缓冲区
		CommandListRing                 m_CommandListRing = {};		// 命令管理VkCommandBuffer

		GPUTimestamps                   m_GPUTimer = {};			// 记录GPU执行时间

		// effects
		Bloom                           m_Bloom = {};
		SkyDome                         m_SkyDome = {};
		DownSamplePS                    m_DownSample = {};
		SkyDomeProc                     m_SkyDomeProc = {};
		ToneMapping                     m_ToneMappingPS = {};
		ToneMappingCS                   m_ToneMappingCS = {};
		oitblendCS						m_oitblendCS = {};
		ColorConversionPS               m_ColorConversionPS = {};
		TAA                             m_TAA = {};
		MagnifierPS                     m_MagnifierPS = {};

		// GBuffer and render passes
		GBuffer* m_GBuffer = {};							// hdr缓冲区
		GBufferRenderPass               m_RenderPassFullGBufferWithClear = {};	// 用于渲染不透明物体及清空缓冲区opaque
		GBufferRenderPass               m_RenderPassJustDepthAndHdr = {};		// 用于渲染天空盒、线框justdepth
		GBufferRenderPass               m_RenderPassFullGBuffer = {};			// 用于渲染透明物体transparent、透射材质transmission

		// shadowmaps
		VkRenderPass                    m_Render_pass_shadow = {};
		// 程序天空盒参数
		SkyDomeProc::Constants			skyDomeConstants = {};

		// pbr通用资源
		// lut_ggx\lut_charlie\lut_sheen_E
		lut_tex_t _lut[3] = {};
		env_res_t _envr = {};
		// widgets
		Wireframe                       m_Wireframe = {};
		WireframeBox                    m_WireframeBox = {};
		WireframeSphere					_WireframeSphere = {};
		//Axis							_axis= {};
		//CheckerBoardFloor				_cbf = {};
		std::vector<TimeStamp>          m_TimeStamps = {};

		//AsyncPool* m_AsyncPool = {};
		// 渲染对象
		std::vector<robj_info*>         _robject = {};
		robj_info* currobj = {};

		std::vector<SceneShadowInfo>    m_shadowMapPool = {};
		std::vector<VkImageView>        m_ShadowSRVPool = {};
		std::vector<GltfDepthPass*>     _depthpass = {};
		// 灯光管理
		std::vector<light_t> _lights;
		std::queue<light_t> _lights_q;
		std::vector<glm::mat4> lightMats;
		// 渲染对象
		drawables_t drawables;

		transfer_cx _transfer = {};


		PerFrame_t _perFrameData;

		fbo_cxt _fbo = {};
		// VkCommandBuffer
		void(*hubDraw)(void* commandBuffer) = nullptr;


		DisplayMode _dm = DISPLAYMODE_SDR;
		bool bHDR = false;
		bool m_bMagResourceReInit = false;
		// 不启用oit
		bool has_oit = false;
		// 是否启用模板测试
		bool _stencil_test = true;

	};


	class pipeinfo_cx
	{
	public:
		pipeinfo_cx();
		~pipeinfo_cx();
		void compiler_pipe(const char* vertexShader, const char* pixelShader, Device* pDevice);
		void set_pipeinfo(DynamicBufferRing* m_pDynamicBufferRing, ResourceViewHeaps* m_pResourceViewHeaps, VkRenderPass renderPass);
		VkDescriptorSet new_dset();
		void freeobj();
	private:
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		Device* pDevice = 0;
		DynamicBufferRing* m_pDynamicBufferRing = 0;
		ResourceViewHeaps* m_pResourceViewHeaps = 0;
		VkRenderPass renderPass = {};
		VkPipeline m_pipeline = {};
		VkPipelineLayout m_pipelineLayout = {};
		VkDescriptorSetLayout m_descriptorSetLayout = {};
		std::vector<VkDescriptorSet> dset;
	};

	// todo fbo

	class fbo_info_cx
	{
	public:
		class FBO
		{
		public:
			VkFramebuffer framebuffer = 0;
			//深度、模板缓冲
			dvk_texture color;
			dvk_texture depth_stencil;
			VkDescriptorImageInfo descriptor = {};
			// Semaphore used to synchronize between offscreen and final scene rendering
			//信号量用于在屏幕外和最终场景渲染之间进行同步
			VkSemaphore semaphore = VK_NULL_HANDLE;
		public:
			FBO() {}
			~FBO() {}
		};
		size_t count_ = 0;
		int _width = 0, _height = 0;
		//dvk_swapchain* _swapc = 0;
		VkRenderPass renderPass = 0, nrp = 0;
		//采样器
		VkSampler sampler = 0;
		// 渲染到纹理同步cpu
		VkFence _fence = 0;
		// 缓冲区列表
		std::vector<FBO> framebuffers;

		Device* _dev = nullptr;
		VkClearValue clearValues[2] = {};
		// VK_FORMAT_B8G8R8A8_UNORM, 浮点纹理 VK_FORMAT_R32G32B32A32_SFLOAT;//如果要模板则depthFormat用VK_FORMAT_D32_SFLOAT_S8_UINT
		VkFormat depthFormat = VK_FORMAT_D32_SFLOAT, colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
		bool isColor = false;	//渲染到纹理则为true

		int cmdcount = 0;
		// Command buffers used for rendering
		std::vector<VkCommandBuffer> drawCmdBuffers;
		//std::vector<dvk_cmd> dcmds; 
	public:
		fbo_info_cx();

		~fbo_info_cx();

		void setClearValues(uint32_t color, float depth = 1.0f, uint32_t Stencil = 0);

		void setClearValues(float* color, float depth = 1.0f, uint32_t Stencil = 0);
#ifndef VKVG_SURFACE_IMGS_REQUIREMENTS
#define FB_COLOR_FORMAT VK_FORMAT_B8G8R8A8_UNORM
#define VKVG_SURFACE_IMGS_REQUIREMENTS (VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|\
	VK_FORMAT_FEATURE_TRANSFER_DST_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_BLIT_SRC_BIT)
#define VKVG_PNG_WRITE_IMG_REQUIREMENTS (VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT|VK_FORMAT_FEATURE_BLIT_DST_BIT)
#endif
		//swapchainbuffers交换链
		void initFBO(int width, int height, int count, VkRenderPass rp);

		//窗口大小改变时需要重新创建image,如果是交换链则传swapchainbuffers
		void resetFramebuffer(int width, int height);
		void reset_fbo(int width, int height);
		void resetCommandBuffers();


		void build_cmd_empty();
	public:

		void destroyImage();
		void destroy_all();
	private:

	};

	struct SystemInfo
	{
		std::string mCPUName = "UNAVAILABLE";
		std::string mGPUName = "UNAVAILABLE";
		std::string mGfxAPI = "UNAVAILABLE";
	};

	class draw3d_ctx
	{
	public:
		draw3d_ctx();
		~draw3d_ctx();
		void OnParseCommandLine(LPSTR lpCmdLine, uint32_t* pWidth, uint32_t* pHeight);
		void OnCreate();
		void OnDestroy();
		void OnRender();
		void OnResize(bool resizeRender);
		void OnUpdateDisplay();

		void add_model(const char* fn, const glm::vec3& pos, float scale, uint32_t instanceCount, bool has_shadowMap);

		void OnUpdate();

		void HandleInput();
		void UpdateCamera(Camera& cam);
		void set_fboidx(int idx);

	public:
		Device* m_device = 0;
		int m_Width = 1280;  // application window dimensions
		int m_Height = 800;  // application window dimensions
		int shadowResolution = 1024;
		int curridx = 0;
		int setidx = 0;
		fbo_info_cx* _fbo = 0;

		GLTFCommon* _tmpgc = 0;
		std::vector<GLTFCommon*> _loaders;
		std::queue<GLTFCommon*> _lts;
		std::mutex m_ltsm;

		Renderer_cx* m_pRenderer = NULL;
		VkRenderPass _fbo_renderpass = 0;				// fbo的VkRenderPass
		DisplayMode _dm = DISPLAYMODE_SDR;
		scene_state m_UIState;
		Camera m_camera;
		glm::vec3 eyePos = {}, lookAt = {};
		CameraX tpfc = {};

		mouse_state_t io = {};
		double _time = 0; // Time accumulator in seconds, used for animation.

		SystemInfo systemi = {};
		int	m_activeCamera = 0;
		std::vector<std::string> _tlabs;
		std::string _labelstr, _cts;
		int loadingStage = 0;
		bool m_bPlay = 0;
		bool bShowProfilerWindow = true;
		bool _customize_camera = 0;
	};



}
//!vkr



template <typename T>
const T& tinygltf::Value::Get() const {
	int c = std::is_same<T, glm::vec2>::value ? 2 : 0;
	c = std::is_same<T, glm::vec3>::value ? 3 : c;
	c = std::is_same<T, glm::vec4>::value ? 4 : c;
	c = std::is_same<T, double>::value ? 1 : c;
	c = std::is_same<T, float>::value ? 1 : c;
	static T r = {};
	if (c > 0)
	{
		if (IsNumber() && c == 1)
		{
			r = (T)GetNumberAsDouble();
		}
		else
		{
			for (int i = 0; i < array_value_.size() && i < c; i++)
			{
				r[i] = array_value_[i].GetNumberAsDouble();
			}
		}
	}
	return r;
}
// todo depth pass

namespace vkr
{
	const VkColorComponentFlags allBits = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	constexpr float XM_PI = 3.141592654f;
	constexpr float XM_2PI = 6.283185307f;
	constexpr float XM_1DIVPI = 0.318309886f;
	constexpr float XM_1DIV2PI = 0.159154943f;
	constexpr float XM_PIDIV2 = 1.570796327f;
	constexpr float XM_PIDIV4 = 0.785398163f;

	void GenerateSphere(int sides, std::vector<unsigned short>& outIndices, std::vector<float>& outVertices)
	{
		int i = 0;

		outIndices.clear();
		outVertices.clear();

		for (int roll = 0; roll < sides; roll++)
		{
			for (int pitch = 0; pitch < sides; pitch++)
			{
				outIndices.push_back(i);
				outIndices.push_back(i + 1);
				outIndices.push_back(i);
				outIndices.push_back(i + 2);
				i += 3;

				glm::vec4 v1 = PolarToVector((roll) * (2.0f * XM_PI) / sides, (pitch) * (2.0f * XM_PI) / sides);
				glm::vec4 v2 = PolarToVector((roll + 1) * (2.0f * XM_PI) / sides, (pitch) * (2.0f * XM_PI) / sides);
				glm::vec4 v3 = PolarToVector((roll) * (2.0f * XM_PI) / sides, (pitch + 1) * (2.0f * XM_PI) / sides);

				outVertices.push_back(v1.x); outVertices.push_back(v1.y); outVertices.push_back(v1.z);
				outVertices.push_back(v2.x); outVertices.push_back(v2.y); outVertices.push_back(v2.z);
				outVertices.push_back(v3.x); outVertices.push_back(v3.y); outVertices.push_back(v3.z);
			}
		}
	}

	void GenerateBox(std::vector<unsigned short>& outIndices, std::vector<float>& outVertices)
	{
		outIndices.clear();
		outVertices.clear();

		std::vector<unsigned short> indices =
		{
			0,1, 1,2, 2,3, 3,0,
			4,5, 5,6, 6,7, 7,4,
			0,4,
			1,5,
			2,6,
			3,7
		};

		std::vector<float> vertices =
		{
			-1,  -1,   1,
			1,  -1,   1,
			1,   1,   1,
			-1,   1,   1,
			-1,  -1,  -1,
			1,  -1,  -1,
			1,   1,  -1,
			-1,   1,  -1,
		};

		outIndices = indices;
		outVertices = vertices;
	}

	void Wireframe::OnCreate(
		Device* pDevice,
		VkRenderPass renderPass,
		ResourceViewHeaps* pResourceViewHeaps,
		DynamicBufferRing* pDynamicBufferRing,
		StaticBufferPool* pStaticBufferPool,
		VkSampleCountFlagBits sampleDescCount)
	{
		VkResult res;

		m_pDevice = pDevice;
		m_pDynamicBufferRing = pDynamicBufferRing;
		m_pResourceViewHeaps = pResourceViewHeaps;

		// the vertex shader
		static const char* vertexShader =
			"#version 400\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			"layout (std140, binding = 0) uniform _cbPerObject\n"
			"{\n"
			"    mat4        u_mWorldViewProj;\n"
			"    vec4        u_Center;\n"
			"    vec4        u_Radius;\n"
			"    vec4        u_Color;\n"
			"} cbPerObject;\n"
			"layout(location = 0) in vec3 position; \n"
			"layout (location = 0) out vec4 outColor;\n"
			"void main() {\n"
			"   outColor = cbPerObject.u_Color;\n"
			"   gl_Position = cbPerObject.u_mWorldViewProj * (vec4(cbPerObject.u_Center.xyz + position * cbPerObject.u_Radius.xyz, 1.0f));\n"
			"}\n";

		// the pixel shader
		static const char* pixelShader =
			"#version 400\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			"layout (location = 0) in vec4 inColor;\n"
			"layout (location = 0) out vec4 outColor;\n"
			"void main() {\n"
			"   outColor = inColor;\n"
			"}";

		/////////////////////////////////////////////
		// Compile and create shaders

		DefineList attributeDefines;

		VkPipelineShaderStageCreateInfo m_vertexShader;
		res = VKCompileFromString(pDevice->m_device, SST_GLSL, VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "main", "", &attributeDefines, &m_vertexShader);
		assert(res == VK_SUCCESS);

		VkPipelineShaderStageCreateInfo m_fragmentShader;
		res = VKCompileFromString(pDevice->m_device, SST_GLSL, VK_SHADER_STAGE_FRAGMENT_BIT, pixelShader, "main", "", &attributeDefines, &m_fragmentShader);
		assert(res == VK_SUCCESS);

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { m_vertexShader, m_fragmentShader };

		/////////////////////////////////////////////
		// Create descriptor set layout

		/////////////////////////////////////////////
		// Create descriptor set 

		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(1);
		layoutBindings[0].binding = 0;
		layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		layoutBindings[0].descriptorCount = 1;
		layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBindings[0].pImmutableSamplers = NULL;

		m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindings, &m_descriptorSetLayout, &m_descriptorSet);
		m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(wireframe_object), m_descriptorSet);

		/////////////////////////////////////////////
		// Create the pipeline layout using the descriptoset

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
		pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pPipelineLayoutCreateInfo.pNext = NULL;
		pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
		pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)1;
		pPipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;

		res = vkCreatePipelineLayout(pDevice->m_device, &pPipelineLayoutCreateInfo, NULL, &m_pipelineLayout);
		assert(res == VK_SUCCESS);

		/////////////////////////////////////////////
		// Create pipeline

		// Create the input attribute description / input layout(in DX12 jargon)
		//
		VkVertexInputBindingDescription vi_binding = {};
		vi_binding.binding = 0;
		vi_binding.stride = sizeof(float) * 3;
		vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription vi_attrs[] =
		{
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
		};

		VkPipelineVertexInputStateCreateInfo vi = {};
		vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vi.pNext = NULL;
		vi.flags = 0;
		vi.vertexBindingDescriptionCount = 1;
		vi.pVertexBindingDescriptions = &vi_binding;
		vi.vertexAttributeDescriptionCount = _countof(vi_attrs);
		vi.pVertexAttributeDescriptions = vi_attrs;

		// input assembly state
		//
		VkPipelineInputAssemblyStateCreateInfo ia;
		ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		ia.pNext = NULL;
		ia.flags = 0;
		ia.primitiveRestartEnable = VK_FALSE;
		ia.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

		// rasterizer state
		//
		VkPipelineRasterizationStateCreateInfo rs;
		rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rs.pNext = NULL;
		rs.flags = 0;
		rs.polygonMode = VK_POLYGON_MODE_LINE;
		rs.cullMode = VK_CULL_MODE_NONE;
		rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rs.depthClampEnable = VK_FALSE;
		rs.rasterizerDiscardEnable = VK_FALSE;
		rs.depthBiasEnable = VK_FALSE;
		rs.depthBiasConstantFactor = 0;
		rs.depthBiasClamp = 0;
		rs.depthBiasSlopeFactor = 0;
		rs.lineWidth = 2.0f;

		VkPipelineColorBlendAttachmentState att_state[1];
		att_state[0].colorWriteMask = allBits;
		att_state[0].blendEnable = VK_TRUE;
		att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
		att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
		att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

		// Color blend state
		//
		VkPipelineColorBlendStateCreateInfo cb;
		cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cb.flags = 0;
		cb.pNext = NULL;
		cb.attachmentCount = 1;
		cb.pAttachments = att_state;
		cb.logicOpEnable = VK_FALSE;
		cb.logicOp = VK_LOGIC_OP_NO_OP;
		cb.blendConstants[0] = 1.0f;
		cb.blendConstants[1] = 1.0f;
		cb.blendConstants[2] = 1.0f;
		cb.blendConstants[3] = 1.0f;

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pNext = NULL;
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();

		// view port state
		//
		VkPipelineViewportStateCreateInfo vp = {};
		vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vp.pNext = NULL;
		vp.flags = 0;
		vp.viewportCount = 1;
		vp.scissorCount = 1;
		vp.pScissors = NULL;
		vp.pViewports = NULL;

		// depth stencil state
		//
		VkPipelineDepthStencilStateCreateInfo ds;
		ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		ds.pNext = NULL;
		ds.flags = 0;
		ds.depthTestEnable = true;
		ds.depthWriteEnable = true;
		ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		ds.depthBoundsTestEnable = VK_FALSE;
		ds.stencilTestEnable = VK_FALSE;
		ds.back.failOp = VK_STENCIL_OP_KEEP;
		ds.back.passOp = VK_STENCIL_OP_KEEP;
		ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
		ds.back.compareMask = 0;
		ds.back.reference = 0;
		ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
		ds.back.writeMask = 0;
		ds.minDepthBounds = 0;
		ds.maxDepthBounds = 0;
		ds.stencilTestEnable = VK_FALSE;
		ds.front = ds.back;

		// multi sample state
		//
		VkPipelineMultisampleStateCreateInfo ms;
		ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms.pNext = NULL;
		ms.flags = 0;
		ms.pSampleMask = NULL;
		ms.rasterizationSamples = sampleDescCount;
		ms.sampleShadingEnable = VK_FALSE;
		ms.alphaToCoverageEnable = VK_FALSE;
		ms.alphaToOneEnable = VK_FALSE;
		ms.minSampleShading = 0.0;

		// create pipeline 

		VkGraphicsPipelineCreateInfo pipeline = {};
		pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline.pNext = NULL;
		pipeline.layout = m_pipelineLayout;
		pipeline.basePipelineHandle = VK_NULL_HANDLE;
		pipeline.basePipelineIndex = 0;
		pipeline.flags = 0;
		pipeline.pVertexInputState = &vi;
		pipeline.pInputAssemblyState = &ia;
		pipeline.pRasterizationState = &rs;
		pipeline.pColorBlendState = &cb;
		pipeline.pTessellationState = NULL;
		pipeline.pMultisampleState = &ms;
		pipeline.pDynamicState = &dynamicState;
		pipeline.pViewportState = &vp;
		pipeline.pDepthStencilState = &ds;
		pipeline.pStages = shaderStages.data();
		pipeline.stageCount = (uint32_t)shaderStages.size();
		pipeline.renderPass = renderPass;
		pipeline.subpass = 0;

		res = vkCreateGraphicsPipelines(pDevice->m_device, pDevice->GetPipelineCache(), 1, &pipeline, NULL, &m_pipeline);
		assert(res == VK_SUCCESS);
		SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_pipeline, "Wireframe P");
	}

	void Wireframe::OnDestroy()
	{
		vkDestroyPipeline(m_pDevice->m_device, m_pipeline, nullptr);
		vkDestroyPipelineLayout(m_pDevice->m_device, m_pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_pDevice->m_device, m_descriptorSetLayout, NULL);
		m_pResourceViewHeaps->FreeDescriptor(m_descriptorSet);
	}

	void Wireframe::Draw(VkCommandBuffer cmd_buf, int numIndices, VkDescriptorBufferInfo IBV, VkDescriptorBufferInfo VBV, const glm::mat4& worldMatrix, const glm::vec4& vCenter, const glm::vec4& vRadius, const glm::vec4& vColor)
	{
		vkCmdBindVertexBuffers(cmd_buf, 0, 1, &VBV.buffer, &VBV.offset);
		vkCmdBindIndexBuffer(cmd_buf, IBV.buffer, IBV.offset, VK_INDEX_TYPE_UINT16);

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		VkDescriptorSet descritorSets[1] = { m_descriptorSet };

		// Set per Object constants
		//
		wireframe_object* cbPerObject = 0;
		VkDescriptorBufferInfo perObjectDesc;
		m_pDynamicBufferRing->AllocConstantBuffer(sizeof(wireframe_object), (void**)&cbPerObject, &perObjectDesc);
		cbPerObject->m_mWorldViewProj = worldMatrix;
		cbPerObject->m_vCenter = vCenter;
		cbPerObject->m_vRadius = vRadius;
		cbPerObject->m_vColor = vColor;

		uint32_t uniformOffsets[1] = { (uint32_t)perObjectDesc.offset };
		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, descritorSets, 1, uniformOffsets);
		if (IBV.buffer)
			vkCmdDrawIndexed(cmd_buf, numIndices, 1, 0, 0, 0);
		else
			vkCmdDraw(cmd_buf, numIndices, 1, 0, 0);
	}
	//--------------------------------------------------------------------------------------
	//
	// OnCreate
	//
	//--------------------------------------------------------------------------------------
	void GltfBBoxPass::OnCreate(
		Device* pDevice,
		VkRenderPass renderPass,
		ResourceViewHeaps* pResourceViewHeaps,
		DynamicBufferRing* pDynamicBufferRing,
		StaticBufferPool* pStaticBufferPool,
		gltf_gpu_res_cx* pGLTFTexturesAndBuffers,
		Wireframe* pWireframe)
	{
		m_pWireframe = pWireframe;
		_ptb = pGLTFTexturesAndBuffers;

		m_wireframeBox.OnCreate(pDevice, pResourceViewHeaps, pDynamicBufferRing, pStaticBufferPool);
	}

	//--------------------------------------------------------------------------------------
	//
	// OnDestroy
	//
	//--------------------------------------------------------------------------------------
	void GltfBBoxPass::OnDestroy()
	{
		m_wireframeBox.OnDestroy();
	}

	//--------------------------------------------------------------------------------------
	//
	// Draw
	//
	//--------------------------------------------------------------------------------------
	void GltfBBoxPass::Draw(VkCommandBuffer cmd_buf
		, const glm::mat4& cameraViewProjMatrix
		, const glm::vec4& color)
	{
		SetPerfMarkerBegin(cmd_buf, "bounding boxes");

		GLTFCommon* pC = _ptb->m_pGLTFCommon;

		for (uint32_t i = 0; i < pC->m_nodes.size(); i++)
		{
			tfNode* pNode = &pC->m_nodes[i];
			if (pNode->meshIndex < 0)
				continue;

			glm::mat4 mWorldViewProj = cameraViewProjMatrix * pC->m_worldSpaceMats[i].GetCurrent();

			tfMesh* pMesh = &pC->m_meshes[pNode->meshIndex];
			for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
			{
				m_wireframeBox.Draw(cmd_buf, m_pWireframe, mWorldViewProj, pMesh->m_pPrimitives[p].m_center, pMesh->m_pPrimitives[p].m_radius, color);
			}
		}

		SetPerfMarkerEnd(cmd_buf);
	}

	void GltfDepthPass::load(AsyncPool* pAsyncPool)
	{
		auto sampler = m_sampler;
		auto pm = _ptb->m_pGLTFCommon->pm;
		auto& materials = pm->materials;

		m_materialsData.resize(materials.size());
		for (uint32_t i = 0; i < materials.size(); i++)
		{
			auto& material = materials[i];

			DepthMaterial* tfmat = &m_materialsData[i];

			// Load material constants. This is a depth pass and we are only interested in the mask texture
			//               
			tfmat->m_doubleSided = material.doubleSided;
			std::string alphaMode = material.alphaMode;// ", "OPAQUE");
			tfmat->m_defines["DEF_alphaMode_" + alphaMode] = std::to_string(1);

			tfmat->m_blending = alphaMode == "BLEND";
			// If transparent use the baseColorTexture for alpha
			//
			if (alphaMode == "MASK")
			{
				tfmat->m_defines["DEF_alphaCutoff"] = to_string_g(material.alphaCutoff);//  0.5));

				auto pbrMetallicRoughnessIt = material.pbrMetallicRoughness;// .find("pbrMetallicRoughness");
				int id = pbrMetallicRoughnessIt.baseColorTexture.index;
				if (id >= 0)
				{
					tfmat->m_defines["MATERIAL_METALLICROUGHNESS"] = "1";

					// allocate descriptor table for the texture
					tfmat->m_textureCount = 1;
					tfmat->m_defines["ID_baseColorTexture"] = "0";
					tfmat->m_defines["ID_baseTexCoord"] = std::to_string(pbrMetallicRoughnessIt.baseColorTexture.texCoord);
					m_pResourceViewHeaps->AllocDescriptor(tfmat->m_textureCount, &sampler, &tfmat->m_descriptorSetLayout, &tfmat->m_descriptorSet);
					VkImageView textureView = _ptb->GetTextureViewByID(id);
					m_pDevice->SetDescriptorSet(0, textureView, sampler, tfmat->m_descriptorSet);

				}
			}
		}

		// Load Meshes 
		{
			auto& meshes = pm->meshes;
			TaskQueue vts(meshes.size());
			m_meshes.resize(meshes.size());
			for (uint32_t i = 0; i < meshes.size(); i++)
			{
				DepthMesh* tfmesh = &m_meshes[i];
				auto& mesh = meshes[i];
				auto& primitives = meshes[i].primitives;
				tfmesh->m_pPrimitives.resize(primitives.size());
				for (uint32_t p = 0; p < primitives.size(); p++)
				{
					auto& primitive = primitives[p];
					DepthPrimitives* pPrimitive = &tfmesh->m_pPrimitives[p];
					pPrimitive->m_geometry.instanceCount = _ptb->instanceCount;
					//ExecAsyncIfThereIsAPool(pAsyncPool, 
					std::function<void()> cb = [this, i, p, mesh, &primitive, pPrimitive]()
						{
							// Set Material
							//
							auto mid = primitive.material;// .find("material");
							if (mid < 0)
								pPrimitive->m_pMaterial = &m_defaultMaterial;
							else
								pPrimitive->m_pMaterial = &m_materialsData[mid];

							// make a list of all the attribute names our pass requires, in the case of a depth pass we only need the position and a few other things. 
							//
							std::vector<std::string > requiredAttributes;
							for (auto& it : primitive.attributes)//["attributes"].items())
							{
								const std::string semanticName = it.first;
								if (
									(semanticName == "POSITION") ||
									(semanticName.substr(0, 7) == "WEIGHTS") || // for skinning
									(semanticName.substr(0, 6) == "JOINTS") || // for skinning
									(DoesMaterialUseSemantic(pPrimitive->m_pMaterial->m_defines, semanticName) == true) // if there is transparency this will make sure we use the texture coordinates of that texture
									)
								{
									requiredAttributes.push_back(semanticName);
								}
							}

							// holds all the #defines from materials, geometry and texture IDs, the VS & PS shaders need this to get the bindings and code paths
							//
							DefineList defines = pPrimitive->m_pMaterial->m_defines;

							// create an input layout from the required attributes
							// shader's can tell the slots from the #defines
							//
							std::vector<VkVertexInputAttributeDescription> inputLayout;
							_ptb->CreateGeometry(&primitive, requiredAttributes, inputLayout, defines, &pPrimitive->m_geometry, 1);

							// Create Pipeline
							//
							{
								morph_t* morphing = 0;
								auto tsa = primitive.targets.size();// todo 变形
								if (tsa > 0) {
									auto moid = primitive.targets[0].begin()->second;
									morphing = &_ptb->m_BufferMap[moid];
									if (!morphing->mdb.buffer)
									{
										morphing = 0;
									}
								}
								int skinId = _ptb->m_pGLTFCommon->FindMeshSkinId(i);
								int inverseMatrixBufferSize = _ptb->m_pGLTFCommon->GetInverseBindMatricesBufferSizeByID(skinId);

								CreateDescriptors(inverseMatrixBufferSize, &defines, pPrimitive, morphing);
								CreatePipeline(inputLayout, defines, pPrimitive);
							}
						};
					vts.add(cb);
				}
			}
			vts.wait_stop();
		}
	}
	//--------------------------------------------------------------------------------------
	//
	// OnCreate
	//
	//--------------------------------------------------------------------------------------
	void GltfDepthPass::OnCreate(
		Device* pDevice,
		VkRenderPass renderPass,
		UploadHeap* pUploadHeap,
		ResourceViewHeaps* pHeaps,
		DynamicBufferRing* pDynamicBufferRing,
		StaticBufferPool* pStaticBufferPool,
		gltf_gpu_res_cx* pGLTFTexturesAndBuffers,
		AsyncPool* pAsyncPool)
	{
		m_pDevice = pDevice;
		m_renderPass = renderPass;

		m_pResourceViewHeaps = pHeaps;
		//m_pStaticBufferPool = pStaticBufferPool;
		m_pDynamicBufferRing = pDynamicBufferRing;
		_ptb = pGLTFTexturesAndBuffers;


		/////////////////////////////////////////////
		// Create default material

		m_defaultMaterial.m_textureCount = 0;
		m_defaultMaterial.m_descriptorSet = VK_NULL_HANDLE;
		m_defaultMaterial.m_descriptorSetLayout = VK_NULL_HANDLE;
		m_defaultMaterial.m_doubleSided = false;

		// Create static sampler in case there is transparency
		//
		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.minLod = -1000;
			info.maxLod = 1000;
			info.maxAnisotropy = 1.0f;
			m_sampler = pDevice->newSampler(&info);
		}
		if (pGLTFTexturesAndBuffers->m_pGLTFCommon->pm)
		{
			load(pAsyncPool);
			return;
		}

	}

	//--------------------------------------------------------------------------------------
	//
	// OnDestroy
	//
	//--------------------------------------------------------------------------------------
	void GltfDepthPass::OnDestroy()
	{
		for (uint32_t m = 0; m < m_meshes.size(); m++)
		{
			DepthMesh* pMesh = &m_meshes[m];
			for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
			{
				DepthPrimitives* pPrimitive = &pMesh->m_pPrimitives[p];
				vkDestroyPipeline(m_pDevice->m_device, pPrimitive->m_pipeline, nullptr);
				pPrimitive->m_pipeline = VK_NULL_HANDLE;
				vkDestroyPipelineLayout(m_pDevice->m_device, pPrimitive->m_pipelineLayout, nullptr);
				vkDestroyDescriptorSetLayout(m_pDevice->m_device, pPrimitive->m_descriptorSetLayout, NULL);
				m_pResourceViewHeaps->FreeDescriptor(pPrimitive->m_descriptorSet);
			}
		}

		for (int i = 0; i < m_materialsData.size(); i++)
		{
			if (m_materialsData[i].m_descriptorSetLayout)
				vkDestroyDescriptorSetLayout(m_pDevice->m_device, m_materialsData[i].m_descriptorSetLayout, NULL);
			m_pResourceViewHeaps->FreeDescriptor(m_materialsData[i].m_descriptorSet);
		}

		//vkDestroySampler(m_pDevice->m_device, m_sampler, nullptr);
	}

	//--------------------------------------------------------------------------------------
	//
	// CreateDescriptors for a combination of material and geometry
	//
	//--------------------------------------------------------------------------------------
	void GltfDepthPass::CreateDescriptors(int inverseMatrixBufferSize, DefineList* pAttributeDefines, DepthPrimitives* pPrimitive, morph_t* morphing)
	{
		std::vector<VkDescriptorSetLayoutBinding> layout_bindings(2);
		auto& ad = (*pAttributeDefines);
		layout_bindings[0].binding = 0;
		layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		layout_bindings[0].descriptorCount = 1;
		layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		layout_bindings[0].pImmutableSamplers = NULL;
		ad["ID_PER_FRAME"] = std::to_string(layout_bindings[0].binding);

		layout_bindings[1].binding = 1;
		layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		layout_bindings[1].descriptorCount = 1;
		layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		layout_bindings[1].pImmutableSamplers = NULL;
		ad["ID_PER_OBJECT"] = std::to_string(layout_bindings[1].binding);
		auto dt = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		auto dt1 = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		int binc = 2;
		int skb = 2;
		int muvt = 2;
		int td = 2;
		int md = 2;

		if (inverseMatrixBufferSize > 0)
		{
			VkDescriptorSetLayoutBinding b;

			// skinning matrices
			b.binding = binc++;
			b.descriptorType = dt;// VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;// ;// |
			b.descriptorCount = 1;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			b.pImmutableSamplers = NULL;
			ad["ID_SKINNING_MATRICES"] = std::to_string(b.binding);
			skb = b.binding;
			layout_bindings.push_back(b);
		}
		if (morphing)
		{
			VkDescriptorSetLayoutBinding b;
			// 变形动画
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			b.descriptorType = dt1;// 静态数据
			ad["ID_TARGET_DATA"] = std::to_string(b.binding);	td = b.binding;
			layout_bindings.push_back(b);
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			b.descriptorType = dt;// 动态数据变形插值
			ad["ID_MORPHING_DATA"] = std::to_string(b.binding);	md = b.binding;
			layout_bindings.push_back(b);
			for (auto& [k, v] : morphing->defs) {
				ad[k] = std::to_string(v);
			}
		}
		auto pc = get_cp();
		int icbinding = 0;
		uint32_t ins_size = 0;
		if (pc && pc->instanceCount > 1)
		{
			// 实例矩阵
			VkDescriptorSetLayoutBinding b;
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			b.descriptorType = dt;
			icbinding = b.binding;
			ad["ID_INSTANCING"] = std::to_string(b.binding);
			layout_bindings.push_back(b);
			ins_size = sizeof(glm::mat4) * pc->instanceCount;
		}
		m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layout_bindings, &pPrimitive->m_descriptorSetLayout, &pPrimitive->m_descriptorSet);

		// set descriptors entries

		m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(PerFrame_t), pPrimitive->m_descriptorSet);
		m_pDynamicBufferRing->SetDescriptorSet(1, sizeof(depth_object), pPrimitive->m_descriptorSet);

		if (inverseMatrixBufferSize > 0)
		{
			m_pDynamicBufferRing->SetDescriptorSet(skb, (uint32_t)inverseMatrixBufferSize, pPrimitive->m_descriptorSet, dt);
		}

		if (morphing)
		{
			m_pDevice->SetDescriptorSet1(morphing->mdb.buffer, td, morphing->mdb.offset, (uint32_t)morphing->mdb.range, pPrimitive->m_descriptorSet, dt1);
			m_pDynamicBufferRing->SetDescriptorSet(md, (uint32_t)morphing->targetCount * sizeof(float), pPrimitive->m_descriptorSet, dt);
		}

		// 实例矩阵
		if (icbinding > 0)
		{
			m_pDynamicBufferRing->SetDescriptorSet(icbinding, ins_size, pPrimitive->m_descriptorSet, dt);
		}

		std::vector<VkDescriptorSetLayout> descriptorSetLayout = { pPrimitive->m_descriptorSetLayout };
		if (pPrimitive->m_pMaterial->m_descriptorSetLayout != VK_NULL_HANDLE)
			descriptorSetLayout.push_back(pPrimitive->m_pMaterial->m_descriptorSetLayout);

		/////////////////////////////////////////////
		// Create a PSO description

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
		pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pPipelineLayoutCreateInfo.pNext = NULL;
		pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
		pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayout.size();
		pPipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout.data();

		VkResult res = vkCreatePipelineLayout(m_pDevice->m_device, &pPipelineLayoutCreateInfo, NULL, &pPrimitive->m_pipelineLayout);
		assert(res == VK_SUCCESS);
		SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)pPrimitive->m_pipelineLayout, "GltfDepthPass PL");
	}

	//--------------------------------------------------------------------------------------
	//
	// CreatePipeline
	//
	//--------------------------------------------------------------------------------------
	void GltfDepthPass::CreatePipeline(std::vector<VkVertexInputAttributeDescription>& layout, const DefineList& defines, DepthPrimitives* pPrimitive)
	{
		/////////////////////////////////////////////
		// Compile and create shaders

		VkPipelineShaderStageCreateInfo vertexShader, fragmentShader = {};
		{
			VKCompileFromFile(m_pDevice->m_device, VK_SHADER_STAGE_VERTEX_BIT, "GLTFDepthPass-vert.glsl", "main", "", &defines, &vertexShader);
			VKCompileFromFile(m_pDevice->m_device, VK_SHADER_STAGE_FRAGMENT_BIT, "GLTFDepthPass-frag.glsl", "main", "", &defines, &fragmentShader);
		}
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertexShader, fragmentShader };

		/////////////////////////////////////////////
		// Create a Pipeline 

		// vertex input state

		std::vector<VkVertexInputBindingDescription> vi_binding(layout.size());
		for (int i = 0; i < layout.size(); i++)
		{
			vi_binding[i].binding = layout[i].binding;
			vi_binding[i].stride = SizeOfFormat(layout[i].format);
			vi_binding[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}

		VkPipelineVertexInputStateCreateInfo vi = {};
		vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vi.pNext = NULL;
		vi.flags = 0;
		vi.vertexBindingDescriptionCount = (uint32_t)vi_binding.size();
		vi.pVertexBindingDescriptions = vi_binding.data();
		vi.vertexAttributeDescriptionCount = (uint32_t)layout.size();
		vi.pVertexAttributeDescriptions = layout.data();

		// input assembly state

		VkPipelineInputAssemblyStateCreateInfo ia;
		ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		ia.pNext = NULL;
		ia.flags = 0;
		ia.primitiveRestartEnable = VK_FALSE;
		ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// rasterizer state

		VkPipelineRasterizationStateCreateInfo rs;
		rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rs.pNext = NULL;
		rs.flags = 0;
		rs.polygonMode = VK_POLYGON_MODE_FILL;
		rs.cullMode = pPrimitive->m_pMaterial->m_doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
		rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rs.depthClampEnable = VK_FALSE;
		rs.rasterizerDiscardEnable = VK_FALSE;
		rs.depthBiasEnable = VK_FALSE;
		rs.depthBiasConstantFactor = 0;
		rs.depthBiasClamp = 0;
		rs.depthBiasSlopeFactor = 0;
		rs.lineWidth = 1.0f;

		VkPipelineColorBlendAttachmentState att_state[1];
		att_state[0].colorWriteMask = allBits;
		att_state[0].blendEnable = VK_TRUE;
		att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
		att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
		att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

		// Color blend state

		VkPipelineColorBlendStateCreateInfo cb;
		cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cb.flags = 0;
		cb.pNext = NULL;
		cb.attachmentCount = 0; //set to 1 when transparency
		cb.pAttachments = att_state;
		cb.logicOpEnable = VK_FALSE;
		cb.logicOp = VK_LOGIC_OP_NO_OP;
		cb.blendConstants[0] = 1.0f;
		cb.blendConstants[1] = 1.0f;
		cb.blendConstants[2] = 1.0f;
		cb.blendConstants[3] = 1.0f;

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pNext = NULL;
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();

		// view port state

		VkPipelineViewportStateCreateInfo vp = {};
		vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vp.pNext = NULL;
		vp.flags = 0;
		vp.viewportCount = 1;
		vp.scissorCount = 1;
		vp.pScissors = NULL;
		vp.pViewports = NULL;

		// depth stencil state

		VkPipelineDepthStencilStateCreateInfo ds;
		ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		ds.pNext = NULL;
		ds.flags = 0;
		ds.depthTestEnable = true;
		ds.depthWriteEnable = true;
		ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		ds.depthCompareOp = m_bInvertedDepth ? VK_COMPARE_OP_GREATER_OR_EQUAL : VK_COMPARE_OP_LESS_OR_EQUAL;

		ds.back.failOp = VK_STENCIL_OP_KEEP;
		ds.back.passOp = VK_STENCIL_OP_KEEP;
		ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
		ds.back.compareMask = 0;
		ds.back.reference = 0;
		ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
		ds.back.writeMask = 0;
		ds.depthBoundsTestEnable = VK_FALSE;
		ds.minDepthBounds = 0;
		ds.maxDepthBounds = 0;
		ds.stencilTestEnable = VK_FALSE;
		ds.front = ds.back;

		// multi sample state

		VkPipelineMultisampleStateCreateInfo ms;
		ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms.pNext = NULL;
		ms.flags = 0;
		ms.pSampleMask = NULL;
		ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		ms.sampleShadingEnable = VK_FALSE;
		ms.alphaToCoverageEnable = VK_FALSE;
		ms.alphaToOneEnable = VK_FALSE;
		ms.minSampleShading = 0.0;

		// create pipeline 

		VkGraphicsPipelineCreateInfo pipeline = {};
		pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline.pNext = NULL;
		pipeline.layout = pPrimitive->m_pipelineLayout;
		pipeline.basePipelineHandle = VK_NULL_HANDLE;
		pipeline.basePipelineIndex = 0;
		pipeline.flags = 0;
		pipeline.pVertexInputState = &vi;
		pipeline.pInputAssemblyState = &ia;
		pipeline.pRasterizationState = &rs;
		pipeline.pColorBlendState = &cb;
		pipeline.pTessellationState = NULL;
		pipeline.pMultisampleState = &ms;
		pipeline.pDynamicState = &dynamicState;
		pipeline.pViewportState = &vp;
		pipeline.pDepthStencilState = &ds;
		pipeline.pStages = shaderStages.data();
		pipeline.stageCount = (uint32_t)shaderStages.size();
		pipeline.renderPass = m_renderPass;
		pipeline.subpass = 0;

		VkResult res = vkCreateGraphicsPipelines(m_pDevice->m_device, m_pDevice->GetPipelineCache(), 1, &pipeline, NULL, &pPrimitive->m_pipeline);
		assert(res == VK_SUCCESS);
		SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pPrimitive->m_pipeline, "GltfDepthPass P");
	}

	//--------------------------------------------------------------------------------------
	//
	// SetPerFrameConstants
	//
	//--------------------------------------------------------------------------------------
	GltfDepthPass::PerFrame_t* GltfDepthPass::SetPerFrameConstants()
	{
		GltfDepthPass::PerFrame_t* cbPerFrame;
		m_pDynamicBufferRing->AllocConstantBuffer(sizeof(GltfDepthPass::PerFrame_t), (void**)&cbPerFrame, &m_perFrameDesc);

		return cbPerFrame;
	}

	//--------------------------------------------------------------------------------------
	//
	// Draw
	//
	//--------------------------------------------------------------------------------------
	void GltfDepthPass::Draw(VkCommandBuffer cmd_buf)
	{
		SetPerfMarkerBegin(cmd_buf, "DepthPass");
		// loop through nodes
		std::vector<tfNode>* pNodes = &_ptb->m_pGLTFCommon->m_nodes;
		Matrix2* pNodesMatrices = _ptb->m_pGLTFCommon->m_worldSpaceMats.data();
		auto pm = _ptb->m_pGLTFCommon->pm;
		for (uint32_t i = 0; i < pNodes->size(); i++)
		{
			tfNode* pNode = &pNodes->at(i);
			if ((pNode == NULL) || (pNode->meshIndex < 0))
				continue;
			glm::ivec2 sm = { pNode->skinIndex, pNode->meshIndex };
			// skinning matrices constant buffer
			VkDescriptorBufferInfo* pPerSkeleton = _ptb->GetSkinningMatricesBuffer(sm);
			DepthMesh* pMesh = &m_meshes[pNode->meshIndex];
			auto mesh = pm->meshes[pNode->meshIndex];
			for (int p = 0; p < pMesh->m_pPrimitives.size(); p++)
			{
				DepthPrimitives* pPrimitive = &pMesh->m_pPrimitives[p];
				if (pPrimitive->m_pipeline == VK_NULL_HANDLE || pPrimitive->m_pMaterial->m_blending)
					continue;
				auto morph = _ptb->get_mb(i);
				auto instance_info = _ptb->get_instance_info();
				// Set per Object constants
				depth_object* cbPerObject;
				VkDescriptorBufferInfo perObjectDesc;
				m_pDynamicBufferRing->AllocConstantBuffer(sizeof(depth_object), (void**)&cbPerObject, &perObjectDesc);
				cbPerObject->mWorld = pNodesMatrices[i].GetCurrent();
				// Bind indices and vertices using the right offsets into the buffer
				Geometry* pGeometry = &pPrimitive->m_geometry;
				for (uint32_t i = 0; i < pGeometry->m_VBV.size(); i++)
				{
					vkCmdBindVertexBuffers(cmd_buf, i, 1, &pGeometry->m_VBV[i].buffer, &pGeometry->m_VBV[i].offset);
				}
				if (pGeometry->m_IBV.buffer) vkCmdBindIndexBuffer(cmd_buf, pGeometry->m_IBV.buffer, pGeometry->m_IBV.offset, pGeometry->m_indexType);
				// Bind Descriptor sets
				VkDescriptorSet descriptorSets[2] = { pPrimitive->m_descriptorSet, pPrimitive->m_pMaterial->m_descriptorSet };
				uint32_t descritorSetCount = 1 + (pPrimitive->m_pMaterial->m_textureCount > 0 ? 1 : 0);
				uint32_t uniformOffsets[6] = { (uint32_t)m_perFrameDesc.offset, (uint32_t)perObjectDesc.offset, };
				uint32_t uniformOffsetsCount = 2;
				// 骨骼动画
				if (pPerSkeleton) { uniformOffsets[uniformOffsetsCount++] = pPerSkeleton->offset; }
				// 变形动画
				if (morph) { uniformOffsets[uniformOffsetsCount++] = morph->offset; }
				// 实例矩阵偏移
				if (instance_info) { uniformOffsets[uniformOffsetsCount++] = instance_info->offset; }
				vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pPrimitive->m_pipelineLayout, 0, descritorSetCount, descriptorSets, uniformOffsetsCount, uniformOffsets);
				// Bind Pipeline
				vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pPrimitive->m_pipeline);
				// Draw
				if (pGeometry->m_IBV.buffer)
					vkCmdDrawIndexed(cmd_buf, pGeometry->m_NumIndices, pGeometry->instanceCount, 0, 0, 0);
				else
					vkCmdDraw(cmd_buf, pGeometry->m_NumIndices, pGeometry->instanceCount, 0, 0);
			}
		}
		SetPerfMarkerEnd(cmd_buf);
	}
	GLTFCommon* GltfDepthPass::get_cp()
	{
		return _ptb ? _ptb->m_pGLTFCommon : nullptr;
	}
}

// todo GltfPbrPass
namespace vkr
{
	// todo GPUTimestamps

	void GPUTimestamps::OnCreate(Device* pDevice, uint32_t numberOfBackBuffers)
	{
		m_pDevice = pDevice;
		m_NumberOfBackBuffers = numberOfBackBuffers;
		m_frame = 0;

		const VkQueryPoolCreateInfo queryPoolCreateInfo =
		{
			VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,     // VkStructureType                  sType
			NULL,                                         // const void*                      pNext
			(VkQueryPoolCreateFlags)0,                    // VkQueryPoolCreateFlags           flags
			VK_QUERY_TYPE_TIMESTAMP ,                     // VkQueryType                      queryType
			MaxValuesPerFrame * numberOfBackBuffers,      // deUint32                         entryCount
			0,                                            // VkQueryPipelineStatisticFlags    pipelineStatistics
		};

		VkResult res = vkCreateQueryPool(pDevice->m_device, &queryPoolCreateInfo, NULL, &m_QueryPool);
	}

	void GPUTimestamps::OnDestroy()
	{
		vkDestroyQueryPool(m_pDevice->m_device, m_QueryPool, nullptr);

		for (uint32_t i = 0; i < m_NumberOfBackBuffers; i++)
			m_labels[i].clear();
	}

	void GPUTimestamps::GetTimeStamp(VkCommandBuffer cmd_buf, const char* label)
	{
		uint32_t measurements = (uint32_t)m_labels[m_frame].size();
		uint32_t offset = m_frame * MaxValuesPerFrame + measurements;

		vkCmdWriteTimestamp(cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_QueryPool, offset);

		m_labels[m_frame].push_back(label);
	}

	void GPUTimestamps::GetTimeStampUser(TimeStamp ts)
	{
		m_cpuTimeStamps[m_frame].push_back(ts);
	}

	void GPUTimestamps::OnBeginFrame(VkCommandBuffer cmd_buf, std::vector<TimeStamp>* pTimestamps)
	{
		std::vector<TimeStamp>& cpuTimeStamps = m_cpuTimeStamps[m_frame];
		std::vector<std::string>& gpuLabels = m_labels[m_frame];

		pTimestamps->clear();
		pTimestamps->reserve(cpuTimeStamps.size() + gpuLabels.size());

		// copy CPU timestamps
		//
		for (uint32_t i = 0; i < cpuTimeStamps.size(); i++)
		{
			pTimestamps->push_back(cpuTimeStamps[i]);
		}

		// copy GPU timestamps
		//
		uint32_t offset = m_frame * MaxValuesPerFrame;

		uint32_t measurements = (uint32_t)gpuLabels.size();
		if (measurements > 0)
		{
			// timestampPeriod is the number of nanoseconds per timestamp value increment
			double microsecondsPerTick = (1e-3f * m_pDevice->GetPhysicalDeviceProperries().limits.timestampPeriod);
			{
				UINT64 TimingsInTicks[256] = {};
				VkResult res = vkGetQueryPoolResults(m_pDevice->m_device, m_QueryPool, offset, measurements, measurements * sizeof(UINT64), &TimingsInTicks, sizeof(UINT64), VK_QUERY_RESULT_64_BIT);
				if (res == VK_SUCCESS)
				{
					for (uint32_t i = 1; i < measurements; i++)
					{
						TimeStamp ts = { m_labels[m_frame][i], float(microsecondsPerTick * (double)(TimingsInTicks[i] - TimingsInTicks[i - 1])) };
						pTimestamps->push_back(ts);
					}

					// compute total
					TimeStamp ts = { "Total GPU Time", float(microsecondsPerTick * (double)(TimingsInTicks[measurements - 1] - TimingsInTicks[0])) };
					pTimestamps->push_back(ts);
				}
				else
				{
					pTimestamps->push_back({ "GPU counters are invalid", 0.0f });
				}
			}
		}

		vkCmdResetQueryPool(cmd_buf, m_QueryPool, offset, MaxValuesPerFrame);

		// we always need to clear these ones
		cpuTimeStamps.clear();
		gpuLabels.clear();

		GetTimeStamp(cmd_buf, "Begin Frame");
	}

	void GPUTimestamps::OnEndFrame()
	{
		m_frame = (m_frame + 1) % m_NumberOfBackBuffers;
	}


	gltf_gpu_res_cx::~gltf_gpu_res_cx()
	{

	}

	// todo t2b
	bool gltf_gpu_res_cx::OnCreate(Device* pDevice, GLTFCommon* pGLTFCommon, UploadHeap* pUploadHeap)
	{
		if (!pDevice || !pGLTFCommon || !pUploadHeap)
			return false;
		// 实例数量
		instanceCount = pGLTFCommon->instanceCount;
		m_pDevice = pDevice;
		m_pGLTFCommon = pGLTFCommon;
		m_pUploadHeap = pUploadHeap;
		if (!_pStaticBufferPool)
			_pStaticBufferPool = new StaticBufferPool();
		//ResourceViewHeaps* pResourceViewHeaps,		// 管理set分配  
		return true;
	}
	struct image_pxt
	{
		IMG_INFO header = {};
		std::vector<char> tidata;
		unsigned char* buffer = nullptr;
		VkDeviceSize   bufferSize = 0;
		bool deleteBuffer = false;
	};
	void get_texdata(tinygltf::Image& img, image_pxt* opt)
	{
		bool isimg = img.image.size();
		opt->deleteBuffer = false;
		if (isimg)
		{
			IMG_INFO& header = opt->header;
			std::vector<char>& tidata = opt->tidata;
			if (img.as_is) {
				int w = 0, h = 0, comp = 4, req_comp = 0;
				auto data = (uint32_t*)stbi_load_from_memory((stbi_uc*)img.image.data(), img.image.size(), &w, &h, &req_comp, comp);
				if (data)
				{
					opt->buffer = (unsigned char*)data;
					opt->bufferSize = w * h * comp;
					opt->deleteBuffer = true;
				}
			}
			else
			{
				// We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
				if (img.component == 3)
				{
					opt->bufferSize = img.width * img.height * 4;
					tidata.resize(opt->bufferSize);
					opt->buffer = (unsigned char*)tidata.data();
					unsigned char* rgba = opt->buffer;
					unsigned char* rgb = &img.image[0];
					for (size_t i = 0; i < img.width * img.height; ++i)
					{
						memcpy(rgba, rgb, sizeof(unsigned char) * 3);
						rgba += 4;
						rgb += 3;
					}
				}
				else
				{
					opt->buffer = &img.image[0];
					opt->bufferSize = img.image.size();
				}
			}
			header.width = img.width;
			header.height = img.height;
			header.depth = 1;
			header.arraySize = 1;
			header.mipMapCount = 1;
#ifdef _WIN32
			header.format = DXGI_FORMAT_R8G8B8A8_UNORM;
#else
			header.vkformat = VK_FORMAT_R8G8B8A8_UNORM;
#endif
			header.bitCount = img.bits * img.component;
		}
	}

	TaskQueue::TaskQueue(size_t count)
	{
		if (count > 0)
		{
			size_t tn = std::thread::hardware_concurrency();
			run(std::min(count, tn));
		}
	}

	TaskQueue::~TaskQueue()
	{
		wait_stop();
	}
	void TaskQueue::add(std::function<void()> task)
	{
		if (w_count > 0)
		{
			std::lock_guard<std::mutex> lock(mtx);
			tasks.push(task);
		}
		else {
			task();
		}
	}
	void TaskQueue::run(int num)
	{
		if (num < 1)return;
		w_count += num;
		for (int i = 0; i < num; ++i) {
			workers.emplace_back([this](std::stop_token st) {
				while (!st.stop_requested() || !tasks.empty()) {
					while (tasks.size())
					{
						std::function<void()> task;
						{
							std::lock_guard<std::mutex> lock(mtx);
							//std::unique_lock<std::mutex> lock(mtx);
							if (tasks.empty()) break;
							task = std::move(tasks.front());  tasks.pop();
						}
						task();
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				});
		}
	}
	void TaskQueue::wait_stop() {
		while (tasks.size()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		for (auto& w : workers)
		{
			w.request_stop();
		}
	}
	void gltf_gpu_res_cx::LoadTextures(bool async_de)
	{
		// load textures and create views 
		auto pm = m_pGLTFCommon->pm;
		if (pm && pm->images.size())
		{
			auto& images = pm->images;
			m_textures.resize(images.size());
			m_textureViews.resize(images.size());
			TaskQueue vts(images.size());
			for (int imageIndex = 0; imageIndex < images.size(); imageIndex++)
			{
				Texture* pTex = &m_textures[imageIndex];
				std::string filename = m_pGLTFCommon->m_path + images[imageIndex].uri;
				auto tv = &m_textureViews[imageIndex];
				//ExecAsyncIfThereIsAPool(pAsyncPool, [imageIndex, pTex, tv, this, filename, pm]()
				std::function<void()> ccb = [imageIndex, pTex, tv, this, filename, pm]()
					{
						print_time Pt("load texture", 1);
						bool useSRGB = false;
						float cutOff = 1.0;
						auto& img = pm->images[imageIndex];
						GetSrgbAndCutOffOfImageGivenItsUse(imageIndex, &pm->materials, &useSRGB, &cutOff);
						image_pxt pxt = {};
						bool isimg = img.image.size();
						if (isimg)
						{
							get_texdata(img, &pxt);
							auto name = img.name.size() ? img.name : filename;
							if (img.uri.empty())
								name += std::to_string(imageIndex);
							bool result = pTex->InitFromData(m_pDevice, m_pUploadHeap, &pxt.header, pxt.buffer, pxt.bufferSize, name.c_str(), useSRGB);
							if (pxt.deleteBuffer)
								stbi_image_free(pxt.buffer);
							assert(result != false);
						}
						else
						{
							bool result = pTex->InitFromFile(m_pDevice, m_pUploadHeap, filename.c_str(), useSRGB, 0 /*VkImageUsageFlags*/, cutOff);
							assert(result != false);
						}
						pTex->CreateSRV(tv);
					};
				vts.add(ccb);
			}
			vts.wait_stop();
			// copy textures and apply barriers, then flush the GPU
			m_pUploadHeap->FlushAndFinish();
		}
	}

	template<class T>
	void push_index(std::vector<uint32_t>& indexBuffer, T* buf, size_t count, size_t first = 0)
	{
		auto ps = indexBuffer.size();
		indexBuffer.resize(ps + count);
		for (size_t i = 0; i < count; i++)
		{
			indexBuffer[ps + i] = (first + buf[i]);
		}
	}
	void gltf_gpu_res_cx::LoadGeometry()
	{
		print_time Pt("load geometry", 1);
		auto pm = m_pGLTFCommon->pm;
		if (pm && pm->meshes.size())
		{
			struct vb_t
			{
				tfAccessor vertexBufferAcc;
				int vbmidx = 0;
			};
			struct mp_t {
				morph_t* mp;
				std::vector<glm::vec4> tv;
			};
			struct ib_t {
				tfAccessor indexBufferAcc = {};
				int primitive_indices = 0;
				std::vector<uint32_t> v;
			};
			std::vector<vb_t> vbs;
			std::vector<mp_t> mbs;
			std::vector<ib_t> ibs;
			size_t all_size = 0;
			size_t subo_size = 0;
			for (auto& mesh : pm->meshes)
			{
				for (auto& primitive : mesh.primitives)
				{
					std::map<std::string, size_t> vss;
					//  Load vertex buffers
					for (auto& attributeId : primitive.attributes)
					{
						tfAccessor vertexBufferAcc = {};
						m_pGLTFCommon->GetBufferDetails(attributeId.second, &vertexBufferAcc);
						vss[attributeId.first] = vertexBufferAcc.m_count;
						vbs.push_back({ vertexBufferAcc, attributeId.second });
						all_size += AlignUp((vertexBufferAcc.m_count * vertexBufferAcc.m_stride), 64);
					}

					// 变形动画数据
					std::vector<VkDescriptorBufferInfo> tas;
					std::map<std::string, std::vector<tfAccessor>> attributes;
					auto targetCount = primitive.targets.size();
					for (auto& it : primitive.targets)
					{
						// 类型、数据id
						for (auto& [k, id] : it)
						{
							tfAccessor acc = {};
							m_pGLTFCommon->GetBufferDetails(id, &acc);
							if (acc.m_data)
								attributes[k].push_back(acc);
						}
					}
					morph_t* mp = 0;
					std::vector<glm::vec4> tv;// 临时缓存
					if (attributes.size() > 0)
					{
						auto mid = primitive.targets[0].begin()->second;
						auto& mn = m_BufferMap[mid];
						mp = &mn;
						auto ss = vss.begin()->second;// 顶点数量
						tv.reserve(attributes.size() * ss * targetCount);
						mp->defs["vertex_count"] = ss;
						mp->defs["WEIGHT_COUNT"] = targetCount;
						mp->targetCount = targetCount;
						// Pos=vec3
						// Normal=vec3
						// Tangent=vec3
						// UV=vec2
						// COLOR=vec4
						//MORPH_TARGET_${ attribute }
						size_t ct = 0;
						for (auto& [k, v] : attributes) {
							auto kn = "MORPH_TARGET_" + k + "_OFFSET";
							mp->defs[kn] = ct;
							if (k == "TEXCOORD_1" && v[0].m_stride == 8) {
								auto nt = mp->defs["MORPH_TARGET_TEXCOORD_0_OFFSET"];
								mp->defs[kn] = nt;
								auto tt = tv.data() + nt;
								for (auto& acc : v) {
									auto vd = (char*)acc.m_data;
									for (size_t i = 0; i < acc.m_count; i++)
									{
										auto vf = (glm::vec2*)vd;
										tt->z = vf->x; tt->w = vf->y; tt++; vd += acc.m_stride;
									}
								}
								continue;
							}
							for (auto& acc : v) {
								auto vd = (char*)acc.m_data;
								ct += acc.m_count;
								switch (acc.m_stride)
								{
								case 8:
								{
									for (size_t i = 0; i < acc.m_count; i++)
									{
										glm::vec4 v4 = {};
										auto vf = (glm::vec2*)vd;
										v4.x = vf->x;
										v4.y = vf->y;
										vd += acc.m_stride;
										tv.push_back(v4);
									}
								}
								break;
								case 12:
								{
									for (size_t i = 0; i < acc.m_count; i++)
									{
										auto vf = (glm::vec3*)vd;
										glm::vec4 v4(*vf, 0.0f);
										vd += acc.m_stride;
										tv.push_back(v4);
									}
								}
								break;
								case 16:
								{
									for (size_t i = 0; i < acc.m_count; i++)
									{
										auto vf = (glm::vec4*)vd;
										vd += acc.m_stride;
										tv.push_back(*vf);
									}
								}
								break;
								default:
									break;
								}
							}
						}
						if (tv.size()) {
							//VkDescriptorBufferInfo& vbv = mp->mdb;
							//tas.push_back(vbv);
							subo_size += AlignUp(tv.size() * sizeof(glm::vec4), (size_t)64u);
							mbs.push_back({ mp, std::move(tv) });
						}

					}
					//  Load index buffers
					int indexAcc = primitive.indices;
					if (indexAcc >= 0)
					{
						tfAccessor indexBufferAcc = {};
						m_pGLTFCommon->GetBufferDetails(indexAcc, &indexBufferAcc);
						std::vector<uint32_t> v;
						// Some exporters use 1-byte indices, need to convert them to shorts since the GPU doesn't support 1-byte indices
						if (indexBufferAcc.m_stride == 1)
						{
							push_index(v, (uint8_t*)indexBufferAcc.m_data, indexBufferAcc.m_count, 0);
							_indextype = indexBufferAcc.m_stride = sizeof(uint32_t);
						}
						all_size += AlignUp((size_t)indexBufferAcc.m_count * indexBufferAcc.m_stride, (size_t)64);
						ibs.push_back({ indexBufferAcc, primitive.indices, std::move(v) });
					}

				}
			}
			// Create a 'static' pool for vertices and indices  
			if (all_size > 0)
			{
				auto nas = AlignUp(all_size, (size_t)512);
				_pStaticBufferPool->OnCreate(m_pDevice, nas, true, "StaticGeom");
				if (subo_size > 0) {
					if (!_p_static_buffer)
						_p_static_buffer = new DynamicBufferRing();
					_p_static_buffer->OnCreate(m_pDevice, 1, subo_size, (char*)"StaticUBO");
					_p_static_buffer->OnBeginFrame();
				}
				for (auto& it : vbs) {
					VkDescriptorBufferInfo vbv;
					_pStaticBufferPool->AllocBuffer(it.vertexBufferAcc.m_count, it.vertexBufferAcc.m_stride, it.vertexBufferAcc.m_data, &vbv);
					m_vertexBufferMap[it.vbmidx] = vbv;
				}
				for (auto& it : mbs) {
					_p_static_buffer->AllocBuffer(it.tv.size(), sizeof(glm::vec4), it.tv.data(), &it.mp->mdb);
				}
				for (auto& it : ibs) {
					VkDescriptorBufferInfo ibv = {};
					if (it.v.empty()) {
						_pStaticBufferPool->AllocBuffer(it.indexBufferAcc.m_count, it.indexBufferAcc.m_stride, it.indexBufferAcc.m_data, &ibv);
					}
					else {
						_pStaticBufferPool->AllocBuffer(it.v.size(), sizeof(uint32_t), it.v.data(), &ibv);
					}
					m_IndexBufferMap[it.primitive_indices] = ibv;
				}
			}

			_pStaticBufferPool->UploadData(m_pUploadHeap->GetCommandList());
			m_pUploadHeap->FlushAndFinish();
			_pStaticBufferPool->FreeUploadHeap();
		}
	}

	void gltf_gpu_res_cx::OnDestroy()
	{
		for (int i = 0; i < m_textures.size(); i++)
		{
			vkDestroyImageView(m_pDevice->m_device, m_textureViews[i], NULL);
			m_textures[i].OnDestroy();
		}
		if (_pStaticBufferPool)
		{
			_pStaticBufferPool->OnDestroy();
			delete _pStaticBufferPool;
			_pStaticBufferPool = 0;
		}
		if (_p_static_buffer)
		{
			_p_static_buffer->OnDestroy();
			delete _p_static_buffer;
			_p_static_buffer = 0;
		}
	}

	VkImageView gltf_gpu_res_cx::GetTextureViewByID(int id)
	{
		auto pm = m_pGLTFCommon->pm;
		int tex = 0;
		if (pm)
		{
			tex = pm->textures[id].source;
		}
		//else
		//	tex = m_pTextureNodes->at(id)["source"];
		if (tex < m_textureViews.size() && tex >= 0)
			return m_textureViews[tex];
		else
			return 0;
	}

	// Creates a Index Buffer from the accessor
	void gltf_gpu_res_cx::CreateIndexBuffer(int indexBufferId, uint32_t* pNumIndices, VkIndexType* pIndexType, VkDescriptorBufferInfo* pIBV)
	{
		tfAccessor indexBuffer;
		m_pGLTFCommon->GetBufferDetails(indexBufferId, &indexBuffer);
		*pNumIndices = indexBuffer.m_count;
		if (_indextype > 0)
			indexBuffer.m_stride = _indextype;
		*pIndexType = (indexBuffer.m_stride == 4) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
		*pIBV = m_IndexBufferMap[indexBufferId];
	}

	// Creates Vertex Buffers from accessors and sets them in the Primitive struct.
	void gltf_gpu_res_cx::CreateGeometry(int indexBufferId, std::vector<int>& vertexBufferIds, Geometry* pGeometry)
	{
		CreateIndexBuffer(indexBufferId, &pGeometry->m_NumIndices, &pGeometry->m_indexType, &pGeometry->m_IBV);

		// load the rest of the buffers onto the GPU
		pGeometry->m_VBV.resize(vertexBufferIds.size());
		for (int i = 0; i < vertexBufferIds.size(); i++)
		{
			pGeometry->m_VBV[i] = m_vertexBufferMap[vertexBufferIds[i]];
		}
	}

	uint32_t GetVkFormat(int type, int id)
	{
		switch (type)
		{
		case TINYGLTF_TYPE_SCALAR://if (str == "SCALAR")
		{
			switch (id)
			{
			case 5120: return VK_FORMAT_R8_SINT; //(BYTE)
			case 5121: return VK_FORMAT_R8_UINT; //(UNSIGNED_BYTE)1
			case 5122: return VK_FORMAT_R16_SINT; //(SHORT)2
			case 5123: return VK_FORMAT_R16_UINT; //(UNSIGNED_SHORT)2
			case 5124: return VK_FORMAT_R32_SINT; //(SIGNED_INT)4
			case 5125: return VK_FORMAT_R32_UINT; //(UNSIGNED_INT)4
			case 5126: return VK_FORMAT_R32_SFLOAT; //(FLOAT)
			case 5130: return VK_FORMAT_R64_SFLOAT; //(DOUBLE)
			}
		}
		break;
		case TINYGLTF_TYPE_VEC2://else if (str == "VEC2")
		{
			switch (id)
			{
			case 5120: return VK_FORMAT_R8G8_SINT; //(BYTE)
			case 5121: return VK_FORMAT_R8G8_UINT; //(UNSIGNED_BYTE)1
			case 5122: return VK_FORMAT_R16G16_SINT; //(SHORT)2
			case 5123: return VK_FORMAT_R16G16_UINT; //(UNSIGNED_SHORT)2
			case 5124: return VK_FORMAT_R32G32_SINT; //(SIGNED_INT)4
			case 5125: return VK_FORMAT_R32G32_UINT; //(UNSIGNED_INT)4
			case 5126: return VK_FORMAT_R32G32_SFLOAT; //(FLOAT)
			case 5130: return VK_FORMAT_R64G64_SFLOAT; //(DOUBLE)
			}
		}
		break;
		case TINYGLTF_TYPE_VEC3://else if (str == "VEC3")
		{
			switch (id)
			{
			case 5120: return VK_FORMAT_UNDEFINED; //(BYTE)
			case 5121: return VK_FORMAT_UNDEFINED; //(UNSIGNED_BYTE)1
			case 5122: return VK_FORMAT_UNDEFINED; //(SHORT)2
			case 5123: return VK_FORMAT_UNDEFINED; //(UNSIGNED_SHORT)2
			case 5124: return VK_FORMAT_R32G32B32_SINT; //(SIGNED_INT)4
			case 5125: return VK_FORMAT_R32G32B32_UINT; //(UNSIGNED_INT)4
			case 5126: return VK_FORMAT_R32G32B32_SFLOAT; //(FLOAT)
			case 5130: return VK_FORMAT_R64G64B64_SFLOAT; //(DOUBLE)
			}
		}
		break;
		case TINYGLTF_TYPE_VEC4://else if (str == "VEC4")
		{
			switch (id)
			{
			case 5120: return VK_FORMAT_R8G8B8A8_SINT; //(BYTE)
			case 5121: return VK_FORMAT_R8G8B8A8_UINT; //(UNSIGNED_BYTE)1
			case 5122: return VK_FORMAT_R16G16B16A16_SINT; //(SHORT)2
			case 5123: return VK_FORMAT_R16G16B16A16_UINT; //(UNSIGNED_SHORT)2
			case 5124: return VK_FORMAT_R32G32B32A32_SINT; //(SIGNED_INT)4
			case 5125: return VK_FORMAT_R32G32B32A32_UINT; //(UNSIGNED_INT)4
			case 5126: return VK_FORMAT_R32G32B32A32_SFLOAT; //(FLOAT)
			case 5130: return VK_FORMAT_R64G64B64A64_SFLOAT; //(DOUBLE)
			}
		}
		break;
		}
		return VK_FORMAT_UNDEFINED;
	}

	void gltf_gpu_res_cx::CreateGeometry(tinygltf::Primitive* primitive, const std::vector<std::string> requiredAttributes, std::vector<VkVertexInputAttributeDescription>& layout, DefineList& defines, Geometry* pGeometry, int  instance_count)
	{

		// Get Index buffer view
		tfAccessor indexBuffer;
		int indexBufferId = primitive->indices;
		if (indexBufferId > -1)
			CreateIndexBuffer(indexBufferId, &pGeometry->m_NumIndices, &pGeometry->m_indexType, &pGeometry->m_IBV);

		// Create vertex buffers and input layout
		int cnt = 0;
		layout.resize(requiredAttributes.size());
		pGeometry->m_VBV.resize(requiredAttributes.size());
		auto& attributes = primitive->attributes;
		auto pm = m_pGLTFCommon->pm;
		for (auto attrName : requiredAttributes)
		{
			// get vertex buffer view
			const int attr = attributes[attrName];
			pGeometry->m_VBV[cnt] = m_vertexBufferMap[attr];

			// let the compiler know we have this stream
			defines[std::string("ID_") + attrName] = std::to_string(cnt);
			auto& ina = pm->accessors[attr];
			// Create Input Layout
			//
			VkVertexInputAttributeDescription l = {};
			l.location = (uint32_t)cnt;
			l.format = (VkFormat)GetVkFormat(ina.type, ina.componentType);
			l.offset = 0;
			l.binding = cnt;
			layout[cnt] = l;
			if (indexBufferId < 0)
				pGeometry->m_NumIndices = ina.count;
			cnt++;
		}
		if (instance_count > 1)
		{
			defines["ID_INSTANCE_MAT"] = std::to_string(cnt);
			cnt++;
		}
	}
	void gltf_gpu_res_cx::SetPerFrameConstants(DynamicBufferRing* ubo, bool bWireframe)
	{
		if (!ubo)return;
		{
			PerFrame_t* cbPerFrame;
			ubo->AllocConstantBuffer(sizeof(PerFrame_t), (void**)&cbPerFrame, &m_perFrameConstants);
			*cbPerFrame = *(m_pGLTFCommon->_perFrameData);
		}
		if (bWireframe)
		{
			PerFrame_t* cbPerFrame;
			ubo->AllocConstantBuffer(sizeof(PerFrame_t), (void**)&cbPerFrame, &m_perFrameConstants_w);
			*cbPerFrame = *(m_pGLTFCommon->_perFrameData_w);
		}
	}

	void gltf_gpu_res_cx::SetSkinningMatricesForSkeletons(DynamicBufferRing* ubo)
	{
		if (!ubo)return;
		auto pc = m_pGLTFCommon;
		for (auto& t : pc->m_worldSpaceSkeletonMats)
		{
			auto matrices = &t.second;
			VkDescriptorBufferInfo perSkeleton = {};
			glm::mat4* cbPerSkeleton = 0;
			uint32_t size = (uint32_t)(matrices->count * sizeof(glm::mat4));
			ubo->AllocConstantBuffer(size, (void**)&cbPerSkeleton, &perSkeleton);
			memcpy(cbPerSkeleton, matrices->m, size);
			m_skeletonMatricesBuffer[t.first] = perSkeleton;
		}
		// todo 设置变形插值数据到ubo
		for (auto& t : pc->m_animated_morphWeights)
		{
			auto d = &t.second;
			VkDescriptorBufferInfo per = {};
			float* cbd = 0;
			uint32_t size = (uint32_t)(d->size() * sizeof(float));
			ubo->AllocConstantBuffer(size, (void**)&cbd, &per);
			memcpy(cbd, d->data(), size);
			//auto& node = m_pGLTFCommon->m_nodes[t.first];
			//auto& mesh = m_pGLTFCommon->pm->meshes[node.meshIndex];
			_morphWeights[t.first] = per;
		}
		// todo uv矩阵
		for (auto& [k, v] : pc->m_uv_mats) {
			VkDescriptorBufferInfo db = {};
			float* cbd = 0;
			uint32_t size = (uint32_t)(v.size() * sizeof(glm::mat3x4));
			ubo->AllocConstantBuffer1(size, (void**)&cbd, &db, 64);
			memcpy(cbd, v.data(), size);
			m_uvmMap[k] = db;
		}
		// 实例矩阵
		if (instanceCount > 1)
		{
			VkDescriptorBufferInfo db = {};
			float* cbd = 0;
			uint32_t size = (uint32_t)(instanceCount * sizeof(glm::mat4));
			ubo->AllocConstantBuffer1(size, (void**)&cbd, &db, 64);
			memcpy(cbd, pc->instance_mat.data(), size);
			instance_info = db;
		}
	}

	VkDescriptorBufferInfo* gltf_gpu_res_cx::GetSkinningMatricesBuffer(const glm::ivec2& skinIndex)
	{
		auto it = m_skeletonMatricesBuffer.find(skinIndex);
		if (it == m_skeletonMatricesBuffer.end())
			return NULL;
		return &it->second;
	}

	VkDescriptorBufferInfo* gltf_gpu_res_cx::get_mb(int idx)
	{
		auto it = _morphWeights.find(idx);
		auto db = (it != _morphWeights.end()) ? &it->second : nullptr;
		return db && db->buffer ? db : nullptr;
	}
	VkDescriptorBufferInfo* gltf_gpu_res_cx::get_uvm(int idx)
	{
		auto it = m_uvmMap.find(idx);
		auto db = (it != m_uvmMap.end()) ? &it->second : nullptr;
		return db && db->buffer ? db : nullptr;
	}
	VkDescriptorBufferInfo* gltf_gpu_res_cx::get_instance_info()
	{
		return instanceCount > 1 ? &instance_info : nullptr;
	}
	//
	// Set some default parameters 
	//

	void SetDefaultMaterialParamters(PBRMaterialParameters* pPbrMaterialParameters)
	{
		pPbrMaterialParameters->m_doubleSided = false;
		pPbrMaterialParameters->m_blending = false;
		auto& pbrm = pPbrMaterialParameters->m_params;
		pbrm = pbr_factors_t();
		//pbrm.emissiveFactor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		//pbrm.baseColorFactor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		//pbrm.metallicFactor = 0.0f;
		//pbrm.roughnessFactor = 0.0f;
		//pbrm.glossinessFactor = 0.0f;
		//pbrm.pbrSpecularFactor = glm::vec3(0.0f, 0.0f, 0.0f);
	}

	bool DoesMaterialUseSemantic(DefineList& defines, const std::string semanticName)
	{
		// search if any *TexCoord mentions this channel
		//
		if (semanticName.substr(0, 9) == "TEXCOORD_")
		{
			char id = semanticName[9];

			for (auto def : defines)
			{
				uint32_t size = static_cast<uint32_t>(def.first.size());
				if (size <= 8)
					continue;

				if (def.first.substr(size - 8) == "TexCoord")
				{
					if (id == def.second.c_str()[0])
					{
						return true;
					}
				}
			}
			return false;
		}

		return false;
	}

	template<class T>
	glm::vec4 tov4(const T& v, const vec4& d)
	{
		return v.size() > 0 ? glm::vec4(v[0], v[1], v[2], v.size() > 3 ? v[3] : 0) : d;
	}
	template<class T>
	vec3 tov3(const T& v, const glm::vec3& d)
	{
		return v.size() > 0 ? vec3(v[0], v[1], v[2]) : d;
	}

	void itcb(tinygltf::Value ns, const std::string& k, const std::string& kd, DefineList& m_defines, std::map<std::string, int>& textureIds)
	{
		if (ns.Has(k))
		{
			auto t = ns.Get(k);
			int index = -1;
			int texCoord = 0;
			if (t.Has("index"))
			{
				index = t.Get("index").GetNumberAsInt();
				if (index != -1)
				{
					textureIds[k] = index;
					if (t.Has("texCoord"))
					{
						texCoord = t.Get("texCoord").GetNumberAsInt();
					}
					m_defines[kd] = std::to_string(texCoord);
				}
			}
		}
	}
	bool hasElementName(const tinygltf::ExtensionMap& extensions, const char* k) {
		auto it = extensions.find(k);
		return it != extensions.end();
	}
	const tinygltf::Value* get_ext(const tinygltf::ExtensionMap& extensions, const char* k) {
		auto it = extensions.find(k);
		return (it != extensions.end()) ? &it->second : nullptr;
	}
	glm::mat3 get_mat3(glm::vec2 offset, float rotation, glm::vec2 scale = { 1.0f, 1.0f })
	{
		glm::mat3 translation = glm::translate(glm::mat3(1.0), offset);
		auto mrotation = glm::rotate(glm::mat3(1.0), -rotation);
		glm::mat3 mscale = glm::scale(glm::mat3(1.0), scale);
		glm::mat3 matrix = translation * mrotation * mscale;
		return matrix;
	}
	template<class T>
	T get_v(const tinygltf::Value* v, const char* k, T rdef)
	{
		if (v && v->Has(k)) {
			rdef = v->Get(k).Get<T>();
		}
		return rdef;
	}
	template<class T>
	T get_v(const tinygltf::Value* v, T rdef)
	{
		if (v) {
			rdef = v->Get<T>();
		}
		return rdef;
	}
	template<class T>
	T get_v(const tinygltf::Value& v, T rdef)
	{
		if (v) {
			rdef = v.Get<T>();
		}
		return rdef;
	}
	bool get_KHR_texture_transform(tinygltf::ExtensionMap& extensions, glm::mat3* m)
	{
		auto tt = get_ext(extensions, "KHR_texture_transform");
		if (tt) {
			int tc = get_v(tt, "texCoord", -1);
			double r = get_v(tt, "rotation", 0.0);
			glm::vec2 o = get_v(tt, "offset", glm::vec2(0, 0));
			glm::vec2 s = get_v(tt, "scale", glm::vec2(1.0, 1.0));
			auto m1 = get_mat3(o, r, s);
			if (m)*m = m1;
		}
		return tt;
	}
	void ProcessMaterials(tinygltf::Material* pm, PBRMaterialParameters* tfmat, std::map<std::string, int>& textureIds, bool use_punctual)
	{
		//njson material;
		// Load material constants
		auto& material = *pm;
		glm::vec4 ones = { 1.0, 1.0, 1.0, 1.0 };
		glm::vec4 zeroes = { 0.0, 0.0, 0.0, 0.0 };
		tfmat->m_doubleSided = material.doubleSided;
		tfmat->m_blending = material.alphaMode == "BLEND";
		tfmat->m_params.emissiveFactor = tov3(material.emissiveFactor, zeroes);
		tfmat->m_defines["DEF_doubleSided"] = std::to_string(tfmat->m_doubleSided ? 1 : 0);
		if (use_punctual)
			tfmat->m_defines["USE_PUNCTUAL"] = "1";
		//tfmat->m_defines["DEF_alphaCutoff"] = to_string_g(material.alphaCutoff);
		//tfmat->m_defines["DEF_alphaMode_" + material.alphaMode] = std::to_string(1);
		// ALPHA_OPAQUE 0
		// ALPHA_MASK 1
		// ALPHA_BLEND 2 
		tfmat->m_params.alphaMode = 0;
		tfmat->m_params.alphaCutoff = material.alphaCutoff;
		if (material.alphaMode == "MASK")
		{
			tfmat->m_params.alphaMode = 1;
		}
		if (material.alphaMode == "BLEND")
		{
			tfmat->m_params.alphaMode = 2;
		}
		int uvc = 0;
		// look for textures and store their IDs in a map 
		for (size_t i = 0; i < (int)UVT_E::e_COUNT; i++)
		{
			tfmat->uvTransform[i] = UVMAT_TYPE(1.0);
		}
		auto& uvtm = tfmat->uvTransform;
		if (material.normalTexture.index != -1)
		{
			textureIds["normalTexture"] = material.normalTexture.index;

			tfmat->m_defines["ID_normalTexCoord"] = std::to_string(material.normalTexture.texCoord);
			glm::mat3 m3 = glm::mat3(1.0);
			if (get_KHR_texture_transform(material.normalTexture.extensions, &m3)) {
				uvtm[uvc] = m3;//memcpy(&uvtm[uvc], &m3, sizeof(glm::mat3x4));
				tfmat->m_defines["UVT_normalTexture"] = std::to_string(uvc++);
			}
		}
		if (material.emissiveTexture.index != -1)
		{
			textureIds["emissiveTexture"] = material.emissiveTexture.index;
			tfmat->m_defines["ID_emissiveTexCoord"] = std::to_string(material.emissiveTexture.texCoord);
			glm::mat3 m3 = glm::mat3(1.0);
			if (get_KHR_texture_transform(material.emissiveTexture.extensions, &m3)) {
				uvtm[uvc] = m3;//memcpy(&uvtm[uvc], &m3, sizeof(glm::mat3));
				tfmat->m_defines["UVT_emissiveTexture"] = std::to_string(uvc++);
			}
		}
		if (material.occlusionTexture.index != -1)
		{
			textureIds["occlusionTexture"] = material.occlusionTexture.index;
			tfmat->m_params.occlusionStrength = material.occlusionTexture.strength;
			tfmat->m_defines["ID_occlusionTexCoord"] = std::to_string(material.occlusionTexture.texCoord);
			glm::mat3 m3 = glm::mat3(1.0);
			if (get_KHR_texture_transform(material.occlusionTexture.extensions, &m3)) {
				uvtm[uvc] = m3;//memcpy(&uvtm[uvc], &m3, sizeof(glm::mat3));
				tfmat->m_defines["UVT_occlusionTexture"] = std::to_string(uvc++);
			}
		}


		// If using pbrMetallicRoughness
		//
		auto pbrMetallicRoughness = material.pbrMetallicRoughness;// .find("pbrMetallicRoughness");

		do
		{
			if (material.extensions.size())
			{
				auto extensions = material.extensions;

				auto sg = get_ext(extensions, "KHR_materials_pbrSpecularGlossiness");
				if (sg /*&& (pbrMetallicRoughness.baseColorTexture.index == -1)*/)
				{
					auto pbrSpecularGlossiness = *sg;
					tfmat->m_defines["MATERIAL_SPECULARGLOSSINESS"] = "1";
					tfmat->m_params.pbrDiffuseFactor = get_v(&pbrSpecularGlossiness, "diffuseFactor", glm::vec4(1.0));
					tfmat->m_params.pbrSpecularFactor = get_v(&pbrSpecularGlossiness, "specularFactor", glm::vec3(1.0));
					tfmat->m_params.glossinessFactor = get_v(&pbrSpecularGlossiness, "glossinessFactor", 1.0);
					itcb(pbrSpecularGlossiness, "diffuseTexture", "ID_diffuseTexCoord", tfmat->m_defines, textureIds);
					itcb(pbrSpecularGlossiness, "specularGlossinessTexture", "ID_specularGlossinessTexCoord", tfmat->m_defines, textureIds);
					pbrMetallicRoughness.baseColorFactor.clear();
				}
#if 1 
				// KHR_materials_pbrSpecularGlossiness
				// KHR_materials_unlit
				// KHR_materials_ior
				// KHR_materials_transmission
				// KHR_materials_volume 
				// KHR_materials_anisotropy
				// KHR_materials_clearcoat
				// KHR_materials_specular
				// KHR_materials_iridescence
				// KHR_materials_sheen 
				// KHR_texture_transform
				{
					auto tt = get_ext(extensions, "KHR_materials_unlit");
					if (tt)
					{
						tfmat->m_params.unlit = 1;
					}
					if (tfmat->m_params.unlit)
						tfmat->m_defines["MATERIAL_UNLIT"] = "1";
				}
				{
					auto tt = get_ext(extensions, "KHR_materials_ior");
					if (tt) {
						tfmat->m_params.ior = get_v(tt, "ior", 1.5);
					}
				}
				{
					auto tt = get_ext(extensions, "KHR_materials_emissive_strength");
					if (tt) {
						tfmat->m_params.emissiveStrength = get_v(tt, "emissiveStrength", 1.0);
						tfmat->m_defines["MATERIAL_EMISSIVE_STRENGTH"] = "1";
					}
				}
				{
					auto tt = get_ext(extensions, "KHR_materials_dispersion");
					if (tt) {
						tfmat->m_params.dispersion = get_v(tt, "dispersion", 0.0);
					}
				}
				{
					auto tt = get_ext(extensions, "KHR_materials_transmission");
					if (tt) {
						tfmat->m_defines["MATERIAL_TRANSMISSION"] = "1";
						tfmat->m_params.transmissionFactor = get_v(tt, "transmissionFactor", 0.0);
						tfmat->transmission = true;
						itcb(*tt, "transmissionTexture", "ID_transmissionTexCoord", tfmat->m_defines, textureIds);
					}
				}
				//this.shader.updateUniform("transmissionFactor", material.extensions ? .KHR_materials_transmission ? .transmissionFactor);
				//this.shader.updateUniform("transmissionUVSet", material.extensions ? .KHR_materials_transmission ? .transmissionTexture ? .texCoord);

				//this.shader.updateUniform("diffuseTransmissionFactor", material.extensions ? .KHR_materials_diffuse_transmission ? .diffuseTransmissionFactor);
				//this.shader.updateUniform("diffuseTransmissionColorFactor", jsToGl(material.extensions ? .KHR_materials_diffuse_transmission ? .diffuseTransmissionColorFactor));
				//this.shader.updateUniform("diffuseTransmissionUVSet", material.extensions ? .KHR_materials_diffuse_transmission ? .diffuseTransmissionTexture ? .texCoord);
				//this.shader.updateUniform("diffuseTransmissionColorUVSet", material.extensions ? .KHR_materials_diffuse_transmission ? .diffuseTransmissionColorTexture ? .texCoord);
				{
					auto tt = get_ext(extensions, "KHR_materials_volume");
					if (tt) {
						tfmat->m_defines["MATERIAL_VOLUME"] = "1";
						tfmat->m_params.attenuationColor = get_v(tt, "attenuationColor", glm::vec3(1.0));
						tfmat->m_params.attenuationDistance = get_v(tt, "attenuationDistance", 10240.0);
						tfmat->m_params.thicknessFactor = get_v(tt, "thicknessFactor", 0.0);
						itcb(*tt, "thicknessTexture", "ID_thicknessTexCoord", tfmat->m_defines, textureIds);
					}
				}
				{
					auto tt = get_ext(extensions, "KHR_materials_anisotropy");
					if (tt) {
						tfmat->m_defines["MATERIAL_ANISOTROPY"] = "1";
						auto anisotropyStrength = get_v(tt, "anisotropyStrength", 0.0);
						auto anisotropyRotation = get_v(tt, "anisotropyRotation", 0.0);

						tfmat->m_params.anisotropy = vec3(cos(anisotropyRotation), sin(anisotropyRotation), anisotropyStrength);

						itcb(*tt, "anisotropyTexture", "ID_anisotropyTexCoord", tfmat->m_defines, textureIds);
					}
				}
				{
					auto tt = get_ext(extensions, "KHR_materials_clearcoat");
					if (tt) {
						tfmat->m_defines["MATERIAL_CLEARCOAT"] = "1";
						tfmat->m_params.clearcoatFactor = get_v(tt, "clearcoatFactor", 0.0);
						tfmat->m_params.clearcoatRoughness = get_v(tt, "clearcoatRoughnessFactor", 0.0);
						itcb(*tt, "clearcoatTexture", "ID_clearcoatTexCoord", tfmat->m_defines, textureIds);
						itcb(*tt, "clearcoatRoughnessTexture", "ID_clearcoatRoughnessTexCoord", tfmat->m_defines, textureIds);
						itcb(*tt, "clearcoatNormalTexture", "ID_clearcoatNormalTexCoord", tfmat->m_defines, textureIds);
					}
				}
				{
					auto tt = get_ext(extensions, "KHR_materials_specular");
					if (tt) {
						tfmat->m_defines["MATERIAL_SPECULAR"] = "1";
						tfmat->m_params.specularColorFactor = get_v(tt, "specularColorFactor", glm::vec3(1.0));
						tfmat->m_params.specularFactor = get_v(tt, "specularFactor", 1.0);
						itcb(*tt, "specularTexture", "ID_specularTexCoord", tfmat->m_defines, textureIds);
						itcb(*tt, "specularColorTexture", "ID_specularColorTexCoord", tfmat->m_defines, textureIds);
					}
				}
				{
					auto tt = get_ext(extensions, "KHR_materials_iridescence");
					if (tt) {
						tfmat->m_defines["MATERIAL_IRIDESCENCE"] = "1";
						tfmat->m_params.iridescenceFactor = get_v(tt, "iridescenceFactor", 0.0);
						tfmat->m_params.iridescenceThicknessMinimum = get_v(tt, "iridescenceThicknessMinimum", 100.0);
						tfmat->m_params.iridescenceThicknessMaximum = get_v(tt, "iridescenceThicknessMaximum", 400.0);
						tfmat->m_params.iridescenceIor = get_v(tt, "iridescenceIor", 1.3);
						itcb(*tt, "iridescenceTexture", "ID_iridescenceTexCoord", tfmat->m_defines, textureIds);
						itcb(*tt, "iridescenceThicknessTexture", "ID_iridescenceThicknessTexCoord", tfmat->m_defines, textureIds);
					}
				}
				{
					auto tt = get_ext(extensions, "KHR_materials_sheen");
					if (tt) {
						tfmat->useSheen = 1;
						tfmat->m_defines["MATERIAL_SHEEN"] = "1";
						tfmat->m_params.sheenColorFactor = get_v(tt, "sheenColorFactor", glm::vec3());
						tfmat->m_params.sheenRoughnessFactor = get_v(tt, "sheenRoughnessFactor", 0.0);
						itcb(*tt, "sheenColorTexture", "ID_sheenColorTexCoord", tfmat->m_defines, textureIds);
						itcb(*tt, "sheenRoughnessTexture", "ID_sheenRoughnessTexCoord", tfmat->m_defines, textureIds);
					}
				}

#endif 
			}

			if (pbrMetallicRoughness.baseColorFactor.size())
			{
				float metallicFactor = pbrMetallicRoughness.metallicFactor;
				float roughnessFactor = pbrMetallicRoughness.roughnessFactor;
				tfmat->m_params.metallicFactor = metallicFactor;
				tfmat->m_params.roughnessFactor = roughnessFactor;
				tfmat->m_params.baseColorFactor = tov4(pbrMetallicRoughness.baseColorFactor, ones);
				tfmat->m_defines["MATERIAL_METALLICROUGHNESS"] = "1";
				if (pbrMetallicRoughness.baseColorTexture.index != -1)
				{
					textureIds["baseColorTexture"] = pbrMetallicRoughness.baseColorTexture.index;
					tfmat->m_defines["ID_baseTexCoord"] = std::to_string(pbrMetallicRoughness.baseColorTexture.texCoord);
					glm::mat3 m3 = glm::mat3(1.0);
					if (get_KHR_texture_transform(pbrMetallicRoughness.baseColorTexture.extensions, &m3)) {
						uvtm[uvc] = m3;//memcpy(&uvtm[uvc], &m3, sizeof(glm::mat3));						
						tfmat->m_defines["UVT_baseColorTexture"] = std::to_string(uvc++);
					}
				}
				if (pbrMetallicRoughness.metallicRoughnessTexture.index != -1) {
					textureIds["metallicRoughnessTexture"] = pbrMetallicRoughness.metallicRoughnessTexture.index;
					tfmat->m_defines["ID_metallicRoughnessTexCoord"] = std::to_string(pbrMetallicRoughness.metallicRoughnessTexture.texCoord);
					glm::mat3 m3 = glm::mat3(1.0);
					if (get_KHR_texture_transform(pbrMetallicRoughness.metallicRoughnessTexture.extensions, &m3)) {
						uvtm[uvc] = m3;//memcpy(&uvtm[uvc], &m3, sizeof(glm::mat3));
						tfmat->m_defines["UVT_metallicRoughnessTexture"] = std::to_string(uvc++);
					}
				}
			}

		} while (0);
		if (uvc > 0)
			uvc = uvc;
		tfmat->m_defines["UVT_count"] = std::to_string(uvc);
		tfmat->uvc = uvc;
	}

	int get_vint(tinygltf::Value& t, const char* k)
	{
		int x = 0;
		if (t.Has(k))
		{
			x = t.Get(k).GetNumberAsInt();
		}
		return x;
	}
	int get_v2int(tinygltf::Value& t, const char* k, const char* k1)
	{
		int x = 0;
		if (t.Has(k))
		{
			auto& t1 = t.Get(k);
			if (t1.Has(k1))
			{
				x = t1.Get(k1).GetNumberAsInt();
			}
		}
		return x;
	}
	//
	// Identify what material uses this texture, this helps:
	// 1) determine the color space if the texture and also the cut out level. Authoring software saves albedo and emissive images in SRGB mode, the rest are linear mode
	// 2) tell the cutOff value, to prevent thinning of alpha tested PNGs when lower mips are used. 
	// 
	void GetSrgbAndCutOffOfImageGivenItsUse(int imageIndex, const std::vector<tinygltf::Material>* p, bool* pSrgbOut, float* pCutoff)
	{
		auto& materials = *p;
		*pSrgbOut = false;
		*pCutoff = 1.0f; // no cutoff

		for (int m = 0; m < materials.size(); m++)
		{
			auto material = materials[m];
			if (material.pbrMetallicRoughness.baseColorTexture.index == imageIndex)
			{
				*pSrgbOut = true;
				*pCutoff = material.alphaCutoff;// ", 0.5);
				return;
			}
			// "extensions/KHR_materials_pbrSpecularGlossiness/specularGlossinessTexture/index"
			if (material.extensions.find("KHR_materials_pbrSpecularGlossiness") != material.extensions.end())
			{
				auto mpbrsg = material.extensions["KHR_materials_pbrSpecularGlossiness"];
				auto sgtidx = get_v2int(mpbrsg, "specularGlossinessTexture", "index");
				if (sgtidx == imageIndex)
				{
					*pSrgbOut = true;
					return;
				}
				// "extensions/KHR_materials_pbrSpecularGlossiness/diffuseTexture/index"
				auto dtidx = get_v2int(mpbrsg, "diffuseTexture", "index");
				if (dtidx == imageIndex)
				{
					*pSrgbOut = true;
					return;
				}
			}

			if (material.emissiveTexture.index == imageIndex)
			{
				*pSrgbOut = true;
				return;
			}
		}
	}


	bool get_ext_pbrSpecularGlossiness(tinygltf::ExtensionMap extm, const std::string& textureName, int* pIndex, int* pTexCoord)
	{
		int index = -1;
		int texCoord = -1;
		if (extm.size())
		{
			auto extensions = extm;
			auto KHR_materials_pbrSpecularGlossinessIt = extensions.find("KHR_materials_pbrSpecularGlossiness");
			if (KHR_materials_pbrSpecularGlossinessIt != extensions.end())
			{
				auto pbrSpecularGlossiness = KHR_materials_pbrSpecularGlossinessIt->second;
				if (pbrSpecularGlossiness.Has(textureName))
				{
					auto t = pbrSpecularGlossiness.Get(textureName);

					if (t.Has("index"))
					{
						index = t.Get("index").GetNumberAsInt();
						if (index != -1)
						{
							if (t.Has("texCoord"))
							{
								texCoord = t.Get("texCoord").GetNumberAsInt();
							}
						}
					}
				}
			}
		}
		if (pIndex)
		{
			*pIndex = index;
		}

		if (pTexCoord)
		{
			*pTexCoord = texCoord;
		}

		return (index > -1);
	}

	VkFilter getVkFilterMode(int32_t filterMode)
	{
		switch (filterMode) {
		case -1:
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
			return VK_FILTER_NEAREST;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
			return VK_FILTER_LINEAR;
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
			return VK_FILTER_NEAREST;
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
			return VK_FILTER_NEAREST;
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
			return VK_FILTER_LINEAR;
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			return VK_FILTER_LINEAR;
		}
		//VK_FILTER_CUBIC_IMG
		printf("Unknown filter mode for getVkFilterMode: %d\n", filterMode);
		return VK_FILTER_NEAREST;
	}

	//#define TINYGLTF_TEXTURE_WRAP_REPEAT (10497)
	//#define TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE (33071)
	//#define TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT (33648)
	//		enum VkSamplerAddressMode
	//		VK_SAMPLER_ADDRESS_MODE_REPEAT = 0,
	//		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1,
	//		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
	//		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3,
	//		VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4,
	VkSamplerAddressMode getVkAddressMode(int32_t addressMode) {
		VkSamplerAddressMode m = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		switch (addressMode) {
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
			m = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			break;
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
			m = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			break;
		}
		return m;
	}
	void set_sampler_info(VkSamplerCreateInfo& info, tinygltf::Sampler* samp) {
		info.magFilter = getVkFilterMode(samp->magFilter);
		info.minFilter = getVkFilterMode(samp->minFilter);
		//info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;// :VK_SAMPLER_MIPMAP_MODE_NEAREST;
		info.addressModeU = getVkAddressMode(samp->wrapS);
		info.addressModeV = getVkAddressMode(samp->wrapT);
		info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

	struct ts_t0 {
		int  v;//VkImageView
		int s;//VkSampler
	};
	void getDescriptorTableForMaterialTextures(PBRMaterial* tfmat, std::map<std::string, ts_t0>& texturesBase, int pSkyDome, int Shadow_count, bool bUseSSAOMask)
	{
		std::vector<uint32_t> descriptorCounts;
		// count the number of textures to init bindings and descriptor
		{
			tfmat->m_textureCount = (int)texturesBase.size();
			for (int i = 0; i < texturesBase.size(); ++i)
			{
				descriptorCounts.push_back(1);
			}

			if (pSkyDome)
			{
				tfmat->m_textureCount += 2;   // +3 because the skydome has a specular, diffusse and a BDRF LUT map
				descriptorCounts.push_back(1);
				descriptorCounts.push_back(1);
				if (pSkyDome > 1)
				{
					tfmat->m_textureCount++;
					descriptorCounts.push_back(1);
				}
			}

			if (bUseSSAOMask)
			{
				tfmat->m_textureCount += 1;
				descriptorCounts.push_back(1);
			}
			if (tfmat->m_pbrMaterialParameters.transmission)
			{
				tfmat->m_textureCount += 1;
				descriptorCounts.push_back(1);
			}
			if (Shadow_count > 0)
			{
				assert(Shadow_count <= MaxShadowInstances);
				tfmat->m_textureCount += Shadow_count;//1;
				// this is an array of samplers/textures
				// We should set the exact number of descriptors to avoid validation errors
				descriptorCounts.push_back(MaxShadowInstances);
			}
		}

		{
			// allocate descriptor table for the textures
			//m_pResourceViewHeaps->AllocDescriptor(descriptorCounts, NULL, &tfmat->m_texturesDescriptorSetLayout, &tfmat->m_texturesDescriptorSet);

			uint32_t cnt = 0;

			// 1) create SRV for the PBR materials
			for (auto const& it : texturesBase)
			{
				tfmat->m_pbrMaterialParameters.m_defines[std::string("ID_") + it.first] = std::to_string(cnt);

				//m_pDevice->SetDescriptorSet( cnt, it.second.v, it.second.s ? it.second.s : m_samplerPbr, tfmat->m_texturesDescriptorSet);
				cnt++;
			}

			// 2) 3 SRVs for the IBL probe
			if (pSkyDome)
			{
				tfmat->m_pbrMaterialParameters.m_defines["ID_brdfTexture"] = std::to_string(cnt);
				//tfmat->m_pbrMaterialParameters.m_defines["ID_GGXLUT"] = std::to_string(cnt);
				//m_pDevice->SetDescriptorSet( cnt, m_brdfLutView, m_brdfLutSampler, tfmat->m_texturesDescriptorSet);
				cnt++;

				tfmat->m_pbrMaterialParameters.m_defines["ID_diffuseCube"] = std::to_string(cnt);
				//pSkyDome->SetDescriptorDiff(cnt, tfmat->m_texturesDescriptorSet);
				cnt++;

				if (pSkyDome > 1) {
					tfmat->m_pbrMaterialParameters.m_defines["ID_specularCube"] = std::to_string(cnt);
					//pSkyDome->SetDescriptorSpec(cnt, tfmat->m_texturesDescriptorSet);
					cnt++;
				}

				tfmat->m_pbrMaterialParameters.m_defines["USE_IBL"] = "1";
			}

			if (tfmat->m_pbrMaterialParameters.useSheen)
			{
				tfmat->m_pbrMaterialParameters.m_defines["ID_CharlieTexture"] = std::to_string(cnt);
				cnt++;
				tfmat->m_pbrMaterialParameters.m_defines["ID_SheenETexture"] = std::to_string(cnt);
				cnt++;
			}
			// 3) SSAO mask
			//
			if (bUseSSAOMask)
			{
				tfmat->m_pbrMaterialParameters.m_defines["ID_SSAO"] = std::to_string(cnt);
				cnt++;
			}
			if (tfmat->m_pbrMaterialParameters.transmission) {
				tfmat->m_pbrMaterialParameters.m_defines["ID_transmissionFramebufferTexture"] = std::to_string(cnt);
				//m_pDevice->SetDescriptorSet( cnt, m_pRenderPass->m_pGBuffer->m_HDRSRVt, m_samplerPbr, tfmat->m_texturesDescriptorSet);
				cnt++;
			}
			// 4) Up to MaxShadowInstances SRVs for the shadowmaps
			if (Shadow_count > 0)
			{
				tfmat->m_pbrMaterialParameters.m_defines["ID_shadowMap"] = std::to_string(cnt);
				VkImageLayout ShadowMapViewLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				//m_pDevice->SetDescriptorSet( cnt, descriptorCounts[cnt], ShadowMapViewPool, ShadowMapViewLayout, m_samplerShadow, tfmat->m_texturesDescriptorSet);
				cnt++;
			}
		}
	}
	void get_model_data(tinygltf::Model* pm, std::vector<PBRMaterial>& mdv, std::vector<PBRMesh>& _meshes, bool use_punctual)
	{
		// Load PBR 2.0 Materials 
		if (pm)
		{
			auto& materials = pm->materials;
			mdv.resize(materials.size());
			for (uint32_t i = 0; i < materials.size(); i++)
			{
				auto tfmat = &mdv[i];
				//tfmat->m_pbrMaterialParameters.m_defines["MATERIAL_UNLIT"] = "1";//无光照
				// Get PBR material parameters and texture IDs
				//
				std::map<std::string, int> textureIds;
				ProcessMaterials(&pm->materials[i], &tfmat->m_pbrMaterialParameters, textureIds, use_punctual);
				tfmat->m_pbrMaterialParameters.mid = i;
				// translate texture IDs into textureViews

				std::map<std::string, ts_t0> texturesBase;
				for (auto const& value : textureIds)
				{
					auto tinfo = pm->textures[value.second];
					texturesBase[value.first].v = pm->textures[value.second].source;
					texturesBase[value.first].s = tinfo.sampler;
				}
				getDescriptorTableForMaterialTextures(tfmat, texturesBase, 1, 1, 0);
			}

			// Load Meshes
			//
			auto& meshes = pm->meshes;

			_meshes.resize(meshes.size());
			for (uint32_t i = 0; i < meshes.size(); i++)
			{
				auto& mesh = meshes[i];
				auto& primitives = meshes[i].primitives;
				auto& weights = meshes[i].weights;

				PBRMesh* tfmesh = &_meshes[i];
				tfmesh->m_pPrimitives.resize(primitives.size());

				for (uint32_t p = 0; p < primitives.size(); p++)
				{
					auto& primitive = primitives[p];
					PBRPrimitives* pPrimitive = &tfmesh->m_pPrimitives[p];

					if (primitive.extensions.size()) {
						auto extensions = primitive.extensions;
						auto tt = get_ext(extensions, "KHR_materials_variants");
						if (tt)
						{
							if (tt->Has("mappings"))
							{
								auto& vt = tt->Get("mappings");
								auto vn = vt.Size();
								if (vt.IsArray())
								{
									for (size_t k = 0; k < vn; k++)
									{
										auto& vk = vt.Get(k);
										int mid = vk.Get("material").GetNumberAsInt();
										material_v mv = {};
										mv.mid = mid;
										if (vk.Has("variants") && vk.Get("variants").IsArray())
										{
											auto& va = vk.Get("variants");
											auto van = va.Size();
											mv.variants.reserve(van);
											for (size_t m = 0; m < van; m++)
											{
												int vid = va.Get(m).GetNumberAsInt();
												mv.variants.push_back(vid);
											}
										}
										pPrimitive->material_variants.push_back(mv);
									}
								}
							}
						}
						primitive.material;
					}
				}
			}
		}
	}


	GltfPbrPass::GltfPbrPass()
	{
	}

	GltfPbrPass::~GltfPbrPass()
	{
	}

	//--------------------------------------------------------------------------------------
	//
	// OnCreate
	//
	//--------------------------------------------------------------------------------------
	void GltfPbrPass::OnCreate(Device* pDevice, UploadHeap* pUploadHeap, ResourceViewHeaps* pHeaps, DynamicBufferRing* pDynamicBufferRing
		, gltf_gpu_res_cx* pGLTFTexturesAndBuffers, env_res_t* r
		, GBufferRenderPass* pRenderPass)//AsyncPool* pAsyncPool)
	{
		print_time Pt("init pbr", 1);
		_envr = r;
		m_pDevice = pDevice;
		m_pRenderPass = pRenderPass;
		m_pResourceViewHeaps = pHeaps;
		//m_pStaticBufferPool = pStaticBufferPool;
		m_pDynamicBufferRing = pDynamicBufferRing;
		_ptb = pGLTFTexturesAndBuffers;

		auto bUseSSAOMask = r->bUseSSAOMask;

		//set bindings for the render targets
		//
		DefineList rtDefines;
		m_pRenderPass->GetCompilerDefines(rtDefines);
#if 0
		// Load BRDF look up table for the PBR shader
		if (1)
		{
			print_time Pt("load BRDFLUT", 1);
			m_brdfLutTexture.InitFromFile(pDevice, pUploadHeap, "images/BrdfLut.dds", false); // LUT images are stored as linear
		}
		//int dimm = 256;
		//{
		//	print_time Pt("1632f", 1);
		//	auto bv = generateCookTorranceBRDFLUT1632f(dimm, 0);
		//}
		//{
		//	print_time Pt("fq", 1);
		//	auto bv = generateCookTorranceBRDFLUT(dimm, 0);
		//}
		if (0) {
			print_time Pt("generate BRDFLUT", 1);
			const int cs = 256;
			static auto brdf = generateCookTorranceBRDFLUT(cs, 0);
			IMG_INFO header = {};
			unsigned char* buffer = (unsigned char*)brdf.data();
			VkDeviceSize   bufferSize = cs * cs * sizeof(int);
			header.width = cs;
			header.height = cs;
			header.depth = 1;
			header.arraySize = 1;
			header.mipMapCount = 1;
			header.vkformat = VK_FORMAT_R16G16_SFLOAT;
			header.bitCount = 32;
			m_brdfLutTexture.InitFromData(pDevice, pUploadHeap, &header, buffer, bufferSize, "brdf", false);
		}
		m_brdfLutTexture.CreateSRV(&m_brdfLutView);
#endif
		/////////////////////////////////////////////
		// Create Samplers
		tinygltf::Sampler dsa = {};

		if (pGLTFTexturesAndBuffers && _ptb->m_pGLTFCommon
			&& _ptb->m_pGLTFCommon->pm && _ptb->m_pGLTFCommon->pm->samplers.size())
		{
			dsa = _ptb->m_pGLTFCommon->pm->samplers[0];
			auto& pss = _ptb->m_pGLTFCommon->pm->samplers;
			_samplers.reserve(pss.size());
			for (auto& it : pss) {
				VkSamplerCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				info.magFilter = VK_FILTER_LINEAR;
				info.minFilter = VK_FILTER_LINEAR;
				info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				info.minLod = 0;
				info.maxLod = 10000;
				info.maxAnisotropy = 1.0f;
				set_sampler_info(info, &it);
				VkSampler r = pDevice->newSampler(&info);
				assert(r);
				_samplers.push_back(r);
			}
		}
		//for pbr materials
		for (auto& p : _samplers) {
			if (p)
			{
				m_samplerPbr = p; break;
			}
		}
		if (!m_samplerPbr)
		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;// VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.minLod = 0;
			info.maxLod = 10000;
			info.maxAnisotropy = 1.0f;
			set_sampler_info(info, &dsa);
			m_samplerPbr = pDevice->newSampler(&info);
			assert(m_samplerPbr);
		}


		// Create default material, this material will be used if none is assigned
		//
		{
			SetDefaultMaterialParamters(&m_defaultMaterial.m_pbrMaterialParameters);

			std::map<std::string, ts_t> texturesBase;
			CreateDescriptorTableForMaterialTextures(&m_defaultMaterial, texturesBase, r);
		}

		// Load PBR 2.0 Materials
		auto pm = pGLTFTexturesAndBuffers->m_pGLTFCommon->pm;
		if (pm)
		{
			auto& materials = pm->materials;
			m_materialsData.resize(materials.size());
			for (uint32_t i = 0; i < materials.size(); i++)
			{
				auto tfmat = &m_materialsData[i];
				//tfmat->m_pbrMaterialParameters.m_defines["MATERIAL_UNLIT"] = "1";//无光照
				// Get PBR material parameters and texture IDs
				//
				std::map<std::string, int> textureIds;
				ProcessMaterials(&pm->materials[i], &tfmat->m_pbrMaterialParameters, textureIds, r->use_punctual);
				tfmat->m_pbrMaterialParameters.mid = i;
				// translate texture IDs into textureViews
				//
				std::map<std::string, ts_t> texturesBase;
				for (auto const& value : textureIds)
				{
					auto tinfo = pm->textures[value.second];
					texturesBase[value.first].v = _ptb->GetTextureViewByID(value.second);
					texturesBase[value.first].s = _samplers[tinfo.sampler];
				}
				//tfmat->m_pbrMaterialParameters.m_defines["MAX_LIGHT_INSTANCES"] = "1";
				CreateDescriptorTableForMaterialTextures(tfmat, texturesBase, r);
			}

			// Load Meshes
			//
			auto& meshes = pm->meshes;

			TaskQueue vts(1);
			m_meshes.resize(meshes.size());
			for (uint32_t i = 0; i < meshes.size(); i++)
			{
				auto& mesh = meshes[i];
				auto& primitives = meshes[i].primitives;
				auto& weights = meshes[i].weights;

				// Loop through all the primitives (sets of triangles with a same material) and 
				// 1) create an input layout for the geometry
				// 2) then take its material and create a Root descriptor
				// 3) With all the above, create a pipeline
				//
				PBRMesh* tfmesh = &m_meshes[i];
				tfmesh->m_pPrimitives.resize(primitives.size());

				for (uint32_t p = 0; p < primitives.size(); p++)
				{
					auto& primitive = primitives[p];
					PBRPrimitives* pPrimitive = &tfmesh->m_pPrimitives[p];
					pPrimitive->m_geometry.instanceCount = _ptb->instanceCount;
					if (primitive.extensions.size()) {
						auto extensions = primitive.extensions;
						auto tt = get_ext(extensions, "KHR_materials_variants");
						if (tt)
						{
							if (tt->Has("mappings"))
							{
								auto& vt = tt->Get("mappings");
								auto vn = vt.Size();
								if (vt.IsArray())
								{
									for (size_t k = 0; k < vn; k++)
									{
										auto& vk = vt.Get(k);
										int mid = vk.Get("material").GetNumberAsInt();
										material_v mv = {};
										mv.mid = mid;
										if (vk.Has("variants") && vk.Get("variants").IsArray())
										{
											auto& va = vk.Get("variants");
											auto van = va.Size();
											mv.variants.reserve(van);
											for (size_t m = 0; m < van; m++)
											{
												int vid = va.Get(m).GetNumberAsInt();
												mv.variants.push_back(vid);
											}
										}
										pPrimitive->material_variants.push_back(mv);
									}
								}
							}
						}
						primitive.material;
					}

					//ExecAsyncIfThereIsAPool(0, 
					std::function<void()> cb = [this, i, p, mesh, rtDefines, &primitive, pPrimitive, bUseSSAOMask]()
						{
							// Sets primitive's material, or set a default material if none was specified in the GLTF
							//
							auto mid = primitive.material;// .find("material");
							pPrimitive->m_pMaterial = (mid != -1) ? &m_materialsData[mid] : &m_defaultMaterial;
							pPrimitive->mid = mid;
							auto ts = primitive.targets.size();// todo 变形
							if (ts > 0) {
								printf("targets %zu\n", ts);
							}
							// holds all the #defines from materials, geometry and texture IDs, the VS & PS shaders need this to get the bindings and code paths
							//
							DefineList defines = pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_defines + rtDefines;
							auto& mde = pPrimitive->m_pMaterial->m_pbrMaterialParameters.uvTransform;
							// make a list of all the attribute names our pass requires, in the case of PBR we need them all
							//
							std::vector<std::string> requiredAttributes;
							for (auto const& it : primitive.attributes)//["attributes"].items())
								requiredAttributes.push_back(it.first);

							// create an input layout from the required attributes
							// shader's can tell the slots from the #defines
							// todo 加载顶点数据到显存
							std::vector<VkVertexInputAttributeDescription> inputLayout;
							_ptb->CreateGeometry(&primitive, requiredAttributes, inputLayout, defines, &pPrimitive->m_geometry, 1);
							mesh_mapd_ptr mm = {};
							morph_t* morphing = 0;
							auto tsa = primitive.targets.size();// todo 变形判断
							if (tsa > 0) {
								auto mid = primitive.targets[0].begin()->second;
								auto bm = &_ptb->m_BufferMap[mid];
								morphing = bm;
								if (morphing->mdb.buffer)
									mm.m = morphing;
							}
							// Create descriptors and pipelines
							auto uc = defines["UVT_count"];
							int muvt_size = std::atoi(uc.c_str());
							int skinId = _ptb->m_pGLTFCommon->FindMeshSkinId(i);
							int inverseMatrixBufferSize = _ptb->m_pGLTFCommon->GetInverseBindMatricesBufferSizeByID(skinId);
							if (muvt_size)
							{
								auto& uvm = _ptb->m_pGLTFCommon->m_uv_mats[primitive.material];
								mm.uvtdata = &_ptb->m_uvmMap[primitive.material];
								uvm.resize(muvt_size);
								for (int j = 0; j < muvt_size; j++)
								{
									uvm[j] = mde[j];
								}
							}
							CreateDescriptors(inverseMatrixBufferSize, &defines, pPrimitive, &mm, sizeof(glm::mat3x4) * muvt_size, bUseSSAOMask);
							CreatePipeline(inputLayout, defines, pPrimitive);
						};
					vts.add(cb);
				}
			}
			vts.wait_stop();
		}
	}
	//--------------------------------------------------------------------------------------
	//
	// CreateDescriptorTableForMaterialTextures
	//
	//--------------------------------------------------------------------------------------
	void GltfPbrPass::CreateDescriptorTableForMaterialTextures(PBRMaterial* tfmat, std::map<std::string, ts_t>& texturesBase, env_res_t* r)
	{
		std::vector<uint32_t> descriptorCounts;
		// count the number of textures to init bindings and descriptor
		{
			tfmat->m_textureCount = (int)texturesBase.size();
			for (int i = 0; i < texturesBase.size(); ++i)
			{
				descriptorCounts.push_back(1);
			}
			if (r->pSkyDome)
			{
				descriptorCounts.push_back(1);
				descriptorCounts.push_back(1);
				if (r->pSkyDome->GetCubeSpecularTextureView())
				{
					tfmat->m_textureCount += 3;   // +3 because the skydome has a specular, diffusse and a BDRF LUT map
					descriptorCounts.push_back(1);
				}
				else {
					tfmat->m_textureCount += 2;
				}
			}
			if (tfmat->m_pbrMaterialParameters.useSheen) {
				descriptorCounts.push_back(1);
				descriptorCounts.push_back(1);
			}
			if (r->bUseSSAOMask)
			{
				tfmat->m_textureCount += 1;
				descriptorCounts.push_back(1);
			}
			if (tfmat->m_pbrMaterialParameters.transmission)
			{
				tfmat->m_textureCount += 1;
				descriptorCounts.push_back(1);
			}
			if (!r->ShadowMapViewPool->empty())
			{
				assert(r->ShadowMapViewPool->size() <= MaxShadowInstances);
				tfmat->m_textureCount += (int)r->ShadowMapViewPool->size();//1;
				// this is an array of samplers/textures
				// We should set the exact number of descriptors to avoid validation errors
				descriptorCounts.push_back(MaxShadowInstances);
			}
		}

		// Alloc a descriptor layout and init the descriptor set for the following textures 
		// 1) all the textures of the PBR material (if any)
		// 2) the 3 textures used for IBL: 
		//         - 1 BRDF LUT 
		//         - 2 cubemaps for the specular, difusse
		// 3) SSAO texture
		// 4) the shadowmaps (array of MaxShadowInstances entries -- maximum)
		// for each entry we create a #define with that texture name that hold the id of the texture. That way the PS knows in what slot is each texture.      
		{
			// allocate descriptor table for the textures
			m_pResourceViewHeaps->AllocDescriptor(descriptorCounts, NULL, &tfmat->m_texturesDescriptorSetLayout, &tfmat->m_texturesDescriptorSet);
			uint32_t cnt = 0;
			// 1) create SRV for the PBR materials
			for (auto const& it : texturesBase)
			{
				tfmat->m_pbrMaterialParameters.m_defines[std::string("ID_") + it.first] = std::to_string(cnt);
				m_pDevice->SetDescriptorSet(cnt, it.second.v, it.second.s ? it.second.s : m_samplerPbr, tfmat->m_texturesDescriptorSet);
				cnt++;
			}
			// 2) 3 SRVs for the IBL probe
			if (r->pSkyDome)
			{
				//lut_ggx\lut_charlie\lut_sheen_E
				tfmat->m_pbrMaterialParameters.m_defines["ID_GGXLUT"] = std::to_string(cnt);
				m_pDevice->SetDescriptorSet(cnt, r->lut[0].view, r->lut[0].sampler, tfmat->m_texturesDescriptorSet);
				cnt++;
				tfmat->m_pbrMaterialParameters.m_defines["ID_diffuseCube"] = std::to_string(cnt);
				r->pSkyDome->SetDescriptorDiff(cnt, tfmat->m_texturesDescriptorSet);
				cnt++;
				if (r->pSkyDome->GetCubeSpecularTextureView())
				{
					tfmat->m_pbrMaterialParameters.m_defines["ID_specularCube"] = std::to_string(cnt);
					r->pSkyDome->SetDescriptorSpec(cnt, tfmat->m_texturesDescriptorSet);
					cnt++;
				}
				tfmat->m_pbrMaterialParameters.m_defines["USE_IBL"] = "1";
			}
			if (tfmat->m_pbrMaterialParameters.useSheen)
			{
				tfmat->m_pbrMaterialParameters.m_defines["ID_CharlieTexture"] = std::to_string(cnt);
				m_pDevice->SetDescriptorSet(cnt, r->lut[1].view, r->lut[1].sampler, tfmat->m_texturesDescriptorSet);
				cnt++;
				tfmat->m_pbrMaterialParameters.m_defines["ID_SheenETexture"] = std::to_string(cnt);
				m_pDevice->SetDescriptorSet(cnt, r->lut[2].view, r->lut[2].sampler, tfmat->m_texturesDescriptorSet);
				cnt++;
			}
			// 3) SSAO mask
			if (r->bUseSSAOMask)
			{
				tfmat->m_pbrMaterialParameters.m_defines["ID_SSAO"] = std::to_string(cnt);
				cnt++;
			}
			if (tfmat->m_pbrMaterialParameters.transmission) {
				tfmat->m_pbrMaterialParameters.m_defines["ID_transmissionFramebufferTexture"] = std::to_string(cnt);
				m_pDevice->SetDescriptorSet(cnt, m_pRenderPass->m_pGBuffer->m_HDRSRVt, m_samplerPbr, tfmat->m_texturesDescriptorSet);
				cnt++;
			}
			// 4) Up to MaxShadowInstances SRVs for the shadowmaps
			if (!r->ShadowMapViewPool->empty())
			{
				tfmat->m_pbrMaterialParameters.m_defines["ID_shadowMap"] = std::to_string(cnt);
				VkImageLayout ShadowMapViewLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				m_pDevice->SetDescriptorSet(cnt, descriptorCounts[cnt], *r->ShadowMapViewPool, ShadowMapViewLayout, r->m_samplerShadow, tfmat->m_texturesDescriptorSet);
				cnt++;
			}
		}
	}

	//--------------------------------------------------------------------------------------
	// OnUpdateWindowSizeDependentResources
	//--------------------------------------------------------------------------------------
	void GltfPbrPass::OnUpdateWindowSizeDependentResources(VkImageView SSAO)
	{
		for (uint32_t i = 0; i < m_materialsData.size(); i++)
		{
			PBRMaterial* tfmat = &m_materialsData[i];

			DefineList& def = tfmat->m_pbrMaterialParameters.m_defines;

			auto id = def.find("ID_SSAO");
			if (id != def.end())
			{
				int index = std::stoi(id->second);
				m_pDevice->SetDescriptorSet(index, SSAO, m_samplerPbr, tfmat->m_texturesDescriptorSet);
			}
		}
	}

	GLTFCommon* GltfPbrPass::get_cp()
	{
		return _ptb ? _ptb->m_pGLTFCommon : nullptr;
	}
	//--------------------------------------------------------------------------------------
	// OnDestroy
	//--------------------------------------------------------------------------------------
	void GltfPbrPass::OnDestroy()
	{
		auto dev = m_pDevice->m_device;
		std::set<VkDescriptorSetLayout> sslayout;
		for (uint32_t m = 0; m < m_meshes.size(); m++)
		{
			PBRMesh* pMesh = &m_meshes[m];
			for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
			{
				PBRPrimitives* pPrimitive = &pMesh->m_pPrimitives[p];
				m_pResourceViewHeaps->FreeDescriptor(pPrimitive->m_uniformsDescriptorSet);
			}
		}
		for (auto& [k, v] : _pipem) {
			if (v.m_pipeline)
				vkDestroyPipeline(dev, v.m_pipeline, nullptr);
			if (v.m_pipelineWireframe)
				vkDestroyPipeline(dev, v.m_pipelineWireframe, nullptr);
			if (v.m_pipelineLayout)
				vkDestroyPipelineLayout(dev, v.m_pipelineLayout, nullptr);
			sslayout.insert(v.m_texturesDescriptorSetLayout);
			sslayout.insert(v.m_uniformsDescriptorSetLayout);
			auto p = v.next;
			for (; p;) {
				if (p->m_pipeline)
					vkDestroyPipeline(dev, p->m_pipeline, nullptr);
				if (p->m_pipelineWireframe)
					vkDestroyPipeline(dev, p->m_pipelineWireframe, nullptr);
				if (p->m_pipelineLayout)
					vkDestroyPipelineLayout(dev, p->m_pipelineLayout, nullptr);
				sslayout.insert(p->m_texturesDescriptorSetLayout);
				sslayout.insert(p->m_uniformsDescriptorSetLayout);

				p = p->next;
			}
		}
		for (int i = 0; i < m_materialsData.size(); i++)
		{
			sslayout.insert(m_materialsData[i].m_texturesDescriptorSetLayout);
			m_pResourceViewHeaps->FreeDescriptor(m_materialsData[i].m_texturesDescriptorSet);
		}
		//destroy default material
		sslayout.insert(m_defaultMaterial.m_texturesDescriptorSetLayout);
		for (auto& s : sslayout)
			vkDestroyDescriptorSetLayout(m_pDevice->m_device, s, NULL);
		m_pResourceViewHeaps->FreeDescriptor(m_defaultMaterial.m_texturesDescriptorSet);
		_samplers.clear();
		//vkDestroyImageView(m_pDevice->m_device, m_brdfLutView, NULL);
		//m_brdfLutTexture.OnDestroy();

	}
	PBRPipe_t* GltfPbrPass::get_pipem(DefineList* defines)
	{
		PBRPipe_t* r = 0;
		auto h = defines->Hash();
		auto& oldp = _pipem[h];
		auto nstr = defines->to_string();
		if (oldp.pipe_kv.empty())
		{
			oldp.pipe_kv = nstr;
			r = &oldp;
			return r;
		}
		if (nstr == oldp.pipe_kv)
		{
			r = &oldp;
		}
		else {
			auto p = oldp.next;
			for (; p;) {
				if (nstr == p->pipe_kv)
				{
					r = p;
					break;
				}
				p = p->next;
			}
			if (!p) {
				p = new PBRPipe_t();
				p->pipe_kv = nstr;
				r = p;
				p->next = oldp.next;
				oldp.next = p;
			}
		}
		return r;
	}
	//--------------------------------------------------------------------------------------
	// CreateDescriptors for a combination of material and geometry
	//--------------------------------------------------------------------------------------
	void GltfPbrPass::CreateDescriptors(int inverseMatrixBufferSize, DefineList* pAttributeDefines, PBRPrimitives* pPrimitive, mesh_mapd_ptr* mm, int muvt_size, bool bUseSSAOMask)
	{
		// Creates descriptor set layout binding for the constant buffers
		std::vector<VkDescriptorSetLayoutBinding> layout_bindings(2);
		auto& ad = *pAttributeDefines;
		// Constant buffer 'per frame'
		layout_bindings[0].binding = 0;
		layout_bindings[0].descriptorCount = 1;
		layout_bindings[0].pImmutableSamplers = NULL;
		layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		ad["ID_PER_FRAME"] = std::to_string(layout_bindings[0].binding);
		// Constant buffer 'per object'
		layout_bindings[1].binding = 1;
		layout_bindings[1].descriptorCount = 1;
		layout_bindings[1].pImmutableSamplers = NULL;
		layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		ad["ID_PER_OBJECT"] = std::to_string(layout_bindings[1].binding);
		auto dt = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		auto dt1 = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		int binc = 2;
		int skb = 2;
		int muvt = 2;
		int td = 2;
		int md = 2;
		// Constant buffer holding the skinning matrices
		if (inverseMatrixBufferSize > 0)
		{
			VkDescriptorSetLayoutBinding b;
			// skinning matrices
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			b.descriptorType = dt;
			skb = b.binding;
			ad["ID_SKINNING_MATRICES"] = std::to_string(b.binding);

			layout_bindings.push_back(b);
		}
		auto morphing = mm ? mm->m : 0;
		if (mm && mm->m)
		{
			VkDescriptorSetLayoutBinding b;
			// 变形动画
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			b.descriptorType = dt1;
			td = b.binding;
			ad["ID_TARGET_DATA"] = std::to_string(b.binding);
			layout_bindings.push_back(b);
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			b.descriptorType = dt;
			md = b.binding;
			ad["ID_MORPHING_DATA"] = std::to_string(b.binding);
			layout_bindings.push_back(b);
			for (auto& [k, v] : mm->m->defs) {
				ad[k] = std::to_string(v);
			}
		}
		if (muvt_size && mm && mm->uvtdata)
		{
			// UV矩阵
			VkDescriptorSetLayoutBinding b;
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			b.descriptorType = dt;
			muvt = b.binding;
			ad["ID_MATUV_DATA"] = std::to_string(b.binding);
			layout_bindings.push_back(b);
		}
		auto pc = get_cp();
		int icbinding = 0;
		uint32_t ins_size = 0;
		if (pc && pc->instanceCount > 1)
		{
			// 实例矩阵
			VkDescriptorSetLayoutBinding b;
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			b.descriptorType = dt;
			icbinding = b.binding;
			ad["ID_INSTANCING"] = std::to_string(b.binding);
			layout_bindings.push_back(b);
			ins_size = sizeof(glm::mat4) * pc->instanceCount;
		}
		// todo pbr buffer初始化
		//m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layout_bindings, &pPrimitive->m_uniformsDescriptorSetLayout, &pPrimitive->m_uniformsDescriptorSet);
		auto h = pAttributeDefines->Hash();
		auto pbrpipe = get_pipem(pAttributeDefines);
		assert(pbrpipe);
		if (!pbrpipe)
		{
			return;
		}
		if (!pbrpipe->m_pipelineLayout || !pbrpipe->m_uniformsDescriptorSetLayout) {
			m_pResourceViewHeaps->CreateDescriptorSetLayout(&layout_bindings, &pbrpipe->m_uniformsDescriptorSetLayout);
			// Create the pipeline layout
			std::vector<VkDescriptorSetLayout> descriptorSetLayout = { pbrpipe->m_uniformsDescriptorSetLayout };
			if (pPrimitive->m_pMaterial->m_texturesDescriptorSetLayout != VK_NULL_HANDLE)
			{
				descriptorSetLayout.push_back(pPrimitive->m_pMaterial->m_texturesDescriptorSetLayout);
				pbrpipe->m_texturesDescriptorSetLayout = pPrimitive->m_pMaterial->m_texturesDescriptorSetLayout;
			}
			VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
			pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pPipelineLayoutCreateInfo.pNext = NULL;
			pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
			pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
			pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayout.size();
			pPipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout.data();

			VkResult res = vkCreatePipelineLayout(m_pDevice->m_device, &pPipelineLayoutCreateInfo, NULL, &pbrpipe->m_pipelineLayout);
			assert(res == VK_SUCCESS);
			SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)pbrpipe->m_pipelineLayout, "GltfPbrPass PL");
		}
		m_pResourceViewHeaps->AllocDescriptor(pbrpipe->m_uniformsDescriptorSetLayout, &pPrimitive->m_uniformsDescriptorSet);
		// Init descriptors sets for the constant buffers
		m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(PerFrame_t), pPrimitive->m_uniformsDescriptorSet);
		m_pDynamicBufferRing->SetDescriptorSet(1, sizeof(per_object), pPrimitive->m_uniformsDescriptorSet);

		if (inverseMatrixBufferSize > 0)
		{
			m_pDynamicBufferRing->SetDescriptorSet(skb, (uint32_t)inverseMatrixBufferSize, pPrimitive->m_uniformsDescriptorSet, dt);
		}
		if (morphing)
		{
			//vec4 per_target_data[];
			m_pDevice->SetDescriptorSet1(morphing->mdb.buffer, td, morphing->mdb.offset, (uint32_t)morphing->mdb.range, pPrimitive->m_uniformsDescriptorSet, dt1);
			//float u_morphWeights[];
			m_pDynamicBufferRing->SetDescriptorSet(md, (uint32_t)morphing->targetCount * sizeof(float), pPrimitive->m_uniformsDescriptorSet, dt);
		}
		if (muvt_size > 0)
		{
			m_pDynamicBufferRing->SetDescriptorSet(muvt, (uint32_t)muvt_size, pPrimitive->m_uniformsDescriptorSet, dt);
		}
		// 实例矩阵
		if (icbinding > 0)
		{
			m_pDynamicBufferRing->SetDescriptorSet(icbinding, ins_size, pPrimitive->m_uniformsDescriptorSet, dt);
		}

	}
	void get_blend(bool blend, VkPipelineColorBlendAttachmentState& out)
	{
		VkPipelineColorBlendAttachmentState color_blend =
		{
			VK_TRUE,                                                      // VkBool32                                       blendEnable
			VK_BLEND_FACTOR_SRC_ALPHA,                                    // VkBlendFactor                                  srcColorBlendFactor
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,                          // VkBlendFactor                                  dstColorBlendFactor
			VK_BLEND_OP_ADD,                                              // VkBlendOp                                      colorBlendOp
			VK_BLEND_FACTOR_ONE,                                          // VkBlendFactor                                  srcAlphaBlendFactor
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,                          // VkBlendFactor                                  dstAlphaBlendFactor
			VK_BLEND_OP_ADD,                                              // VkBlendOp                                      alphaBlendOp
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |         // VkColorComponentFlags                          colorWriteMask
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};
		VkPipelineColorBlendAttachmentState color_no_blend = {
			VK_FALSE,                                                     // VkBool32                                       blendEnable
			VK_BLEND_FACTOR_ONE,                                          // VkBlendFactor                                  srcColorBlendFactor
			VK_BLEND_FACTOR_ZERO,                                         // VkBlendFactor                                  dstColorBlendFactor
			VK_BLEND_OP_ADD,                                              // VkBlendOp                                      colorBlendOp
			VK_BLEND_FACTOR_ONE,                                          // VkBlendFactor                                  srcAlphaBlendFactor
			VK_BLEND_FACTOR_ZERO,                                         // VkBlendFactor                                  dstAlphaBlendFactor
			VK_BLEND_OP_ADD,                                              // VkBlendOp                                      alphaBlendOp
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |         // VkColorComponentFlags                          colorWriteMask
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};
		out = blend ? color_blend : color_no_blend;
	}
	//--------------------------------------------------------------------------------------
	// CreatePipeline
	//--------------------------------------------------------------------------------------
	void GltfPbrPass::CreatePipeline(std::vector<VkVertexInputAttributeDescription>& layout, const DefineList& defines0, PBRPrimitives* pPrimitive)
	{
		// Compile and create shaders
		auto defines = defines0;
		auto h = defines.Hash();
		auto pbrpipe = get_pipem(&defines);
		if (!pbrpipe)
			return;
		auto nstr = defines.to_string();
		pPrimitive->_pipe = pbrpipe;
		if (pbrpipe->m_pipeline)
		{
			return;
		}
		VkPipelineShaderStageCreateInfo vertexShader = {}, fragmentShader = {};
		// Create pipeline 
		VKCompileFromFile(m_pDevice->m_device, VK_SHADER_STAGE_VERTEX_BIT, "GLTFPbrPass-vert.glsl", "main", "", &defines, &vertexShader);
		VKCompileFromFile(m_pDevice->m_device, VK_SHADER_STAGE_FRAGMENT_BIT, "GLTFPbrPass-frag.glsl", "main", "", &defines, &fragmentShader);
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertexShader, fragmentShader };
		// vertex input state
		std::vector<VkVertexInputBindingDescription> vi_binding(layout.size());
		for (int i = 0; i < layout.size(); i++)
		{
			vi_binding[i].binding = layout[i].binding;
			vi_binding[i].stride = SizeOfFormat(layout[i].format);
			vi_binding[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}
		if (defines.Has("ID_INSTANCE_MAT")) { vi_binding.rbegin()->inputRate = VK_VERTEX_INPUT_RATE_INSTANCE; }
		VkPipelineVertexInputStateCreateInfo vi = {};
		vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vi.pNext = NULL;
		vi.flags = 0;
		vi.vertexBindingDescriptionCount = (uint32_t)vi_binding.size();
		vi.pVertexBindingDescriptions = vi_binding.data();
		vi.vertexAttributeDescriptionCount = (uint32_t)layout.size();
		vi.pVertexAttributeDescriptions = layout.data();
		// input assembly state
		VkPipelineInputAssemblyStateCreateInfo ia;
		ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		ia.pNext = NULL;
		ia.flags = 0;
		ia.primitiveRestartEnable = VK_FALSE;
		ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		// rasterizer state
		VkPipelineRasterizationStateCreateInfo rs;
		rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rs.pNext = NULL;
		rs.flags = 0;
		rs.polygonMode = VK_POLYGON_MODE_FILL;
		rs.cullMode = pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
		rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rs.depthClampEnable = VK_FALSE;
		rs.rasterizerDiscardEnable = VK_FALSE;
		rs.depthBiasEnable = VK_FALSE;
		rs.depthBiasConstantFactor = 0;
		rs.depthBiasClamp = 0;
		rs.depthBiasSlopeFactor = 0;
		rs.lineWidth = 2.0f;
		bool depthwrite = !(pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_blending || (defines.Has("DEF_alphaMode_BLEND")));
		std::vector<VkPipelineColorBlendAttachmentState> att_states;
		if (defines.Has("HAS_FORWARD_RT"))
		{
			VkPipelineColorBlendAttachmentState att_state = {};
			get_blend(!depthwrite, att_state);
			att_states.push_back(att_state);
		}
		if (defines.Has("HAS_OIT_ACCUM_RT"))
		{
			VkPipelineColorBlendAttachmentState att_state = {};
			att_state.colorWriteMask = allBits;
			att_state.blendEnable = VK_TRUE;
			att_state.alphaBlendOp = VK_BLEND_OP_ADD;
			att_state.colorBlendOp = VK_BLEND_OP_ADD;
			att_state.srcColorBlendFactor = att_state.dstColorBlendFactor = att_state.srcAlphaBlendFactor = att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			att_states.push_back(att_state);
		}
		if (defines.Has("HAS_OIT_WEIGHT_RT"))
		{
			VkPipelineColorBlendAttachmentState att_state = {};
			att_state.colorWriteMask = allBits;
			att_state.blendEnable = VK_TRUE;
			att_state.alphaBlendOp = VK_BLEND_OP_ADD;
			att_state.colorBlendOp = VK_BLEND_OP_ADD;
			att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			att_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			att_state.srcColorBlendFactor = att_state.dstColorBlendFactor = att_state.srcAlphaBlendFactor = att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

			att_states.push_back(att_state);
		}
		if (defines.Has("HAS_SPECULAR_ROUGHNESS_RT"))
		{
			VkPipelineColorBlendAttachmentState att_state = {};
			att_state.colorWriteMask = allBits;
			att_state.blendEnable = VK_FALSE;
			att_state.alphaBlendOp = VK_BLEND_OP_ADD;
			att_state.colorBlendOp = VK_BLEND_OP_ADD;
			att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			att_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			att_states.push_back(att_state);
		}
		if (defines.Has("HAS_DIFFUSE_RT"))
		{
			VkPipelineColorBlendAttachmentState att_state = {};
			att_state.colorWriteMask = allBits;
			att_state.blendEnable = VK_FALSE;
			att_state.alphaBlendOp = VK_BLEND_OP_ADD;
			att_state.colorBlendOp = VK_BLEND_OP_ADD;
			att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			att_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			att_states.push_back(att_state);
		}
		if (defines.Has("HAS_NORMALS_RT"))
		{
			VkPipelineColorBlendAttachmentState att_state = {};
			att_state.colorWriteMask = allBits;
			att_state.blendEnable = VK_FALSE;
			att_state.alphaBlendOp = VK_BLEND_OP_ADD;
			att_state.colorBlendOp = VK_BLEND_OP_ADD;
			att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			att_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			att_states.push_back(att_state);
		}
		if (defines.Has("HAS_MOTION_VECTORS_RT"))
		{
			VkPipelineColorBlendAttachmentState att_state = {};
			att_state.colorWriteMask = allBits;
			att_state.blendEnable = VK_FALSE;
			att_state.alphaBlendOp = VK_BLEND_OP_ADD;
			att_state.colorBlendOp = VK_BLEND_OP_ADD;
			att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			att_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			att_states.push_back(att_state);
		}
		// Color blend state
		VkPipelineColorBlendStateCreateInfo cb;
		cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cb.flags = 0;
		cb.pNext = NULL;
		cb.attachmentCount = static_cast<uint32_t>(att_states.size());
		cb.pAttachments = att_states.data();
		cb.logicOpEnable = VK_FALSE;
		cb.logicOp = VK_LOGIC_OP_NO_OP;
		cb.blendConstants[0] = 0.0f;
		cb.blendConstants[1] = 0.0f;
		cb.blendConstants[2] = 0.0f;
		cb.blendConstants[3] = 0.0f;
		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			//VK_DYNAMIC_STATE_LINE_WIDTH,
			//VK_DYNAMIC_STATE_CULL_MODE,
			//VK_DYNAMIC_STATE_FRONT_FACE
		};
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pNext = NULL;
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();
		// view port state
		VkPipelineViewportStateCreateInfo vp = {};
		vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vp.pNext = NULL;
		vp.flags = 0;
		vp.viewportCount = 1;
		vp.scissorCount = 1;
		vp.pScissors = NULL;
		vp.pViewports = NULL;
		// todo depth stencil state
		VkPipelineDepthStencilStateCreateInfo ds;
		ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		ds.pNext = NULL;
		ds.flags = 0;
		ds.depthTestEnable = true;
		ds.depthWriteEnable = true;/// depthwrite;
		ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		ds.back.failOp = VK_STENCIL_OP_KEEP;
		ds.back.passOp = VK_STENCIL_OP_KEEP;
		ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
		ds.back.compareMask = 0;
		ds.back.reference = 0;
		ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
		ds.back.writeMask = 0;
		ds.depthBoundsTestEnable = VK_FALSE;
		ds.minDepthBounds = 0;
		ds.maxDepthBounds = 0;
		ds.stencilTestEnable = VK_TRUE;
		ds.front = ds.back;
		// multi sample state
		VkPipelineMultisampleStateCreateInfo ms;
		ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms.pNext = NULL;
		ms.flags = 0;
		ms.pSampleMask = NULL;
		ms.rasterizationSamples = m_pRenderPass->GetSampleCount();
		ms.sampleShadingEnable = VK_FALSE;
		ms.alphaToCoverageEnable = VK_FALSE;
		ms.alphaToOneEnable = VK_FALSE;
		ms.minSampleShading = 0.0;
		// create pipeline 
		VkGraphicsPipelineCreateInfo pipeline = {};
		pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline.pNext = NULL;
		pipeline.layout = pbrpipe->m_pipelineLayout;
		pipeline.basePipelineHandle = VK_NULL_HANDLE;
		pipeline.basePipelineIndex = 0;
		pipeline.flags = 0;
		pipeline.pVertexInputState = &vi;
		pipeline.pInputAssemblyState = &ia;
		pipeline.pRasterizationState = &rs;
		pipeline.pColorBlendState = &cb;
		pipeline.pTessellationState = NULL;
		pipeline.pMultisampleState = &ms;
		pipeline.pDynamicState = &dynamicState;
		pipeline.pViewportState = &vp;
		pipeline.pDepthStencilState = &ds;
		pipeline.pStages = shaderStages.data();
		pipeline.stageCount = (uint32_t)shaderStages.size();
		pipeline.renderPass = m_pRenderPass->GetRenderPass();
		pipeline.subpass = 0;
		uint32_t specConstants = 0;
		VkSpecializationMapEntry specializationMapEntry = VkSpecializationMapEntry{};
		specializationMapEntry.constantID = 0;
		specializationMapEntry.offset = 0;
		specializationMapEntry.size = sizeof(uint32_t);
		VkSpecializationInfo specializationInfo = VkSpecializationInfo();
		specializationInfo.mapEntryCount = 1;
		specializationInfo.pMapEntries = &specializationMapEntry;
		specializationInfo.dataSize = sizeof(uint32_t);
		specializationInfo.pData = &specConstants;
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		VkResult res = vkCreateGraphicsPipelines(m_pDevice->m_device, m_pDevice->GetPipelineCache(), 1, &pipeline, NULL, &pbrpipe->m_pipeline);
		assert(res == VK_SUCCESS);
		SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pbrpipe->m_pipeline, "GltfPbrPass P");
		// create wireframe pipeline
		rs.polygonMode = VK_POLYGON_MODE_LINE;
		rs.cullMode = VK_CULL_MODE_NONE;
		//ds.depthWriteEnable = false;
		res = vkCreateGraphicsPipelines(m_pDevice->m_device, m_pDevice->GetPipelineCache(), 1, &pipeline, NULL, &pbrpipe->m_pipelineWireframe);
		assert(res == VK_SUCCESS);
		SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pbrpipe->m_pipelineWireframe, "GltfPbrPass Wireframe P");
		pPrimitive->_pipe = pbrpipe;
	}

	//-------------------------------------------------------------------------------------- 
	// BuildLists 
	//--------------------------------------------------------------------------------------
	void GltfPbrPass::BuildBatchLists(drawables_t* opt, bool bWireframe)
	{
		std::vector<tfNode>* pNodes = &_ptb->m_pGLTFCommon->m_nodes;
		if (_nodes.empty()) {
			_nodes.reserve(pNodes->size());
			for (uint32_t i = 0; i < pNodes->size(); i++)
			{
				tfNode* pNode = &pNodes->at(i);
				if ((pNode == NULL) || (pNode->meshIndex < 0))
					continue;
				_nodes.push_back(i);
			}
		}
		Matrix2* pNodesMatrices = _ptb->m_pGLTFCommon->m_worldSpaceMats.data();
		auto pSolid = &opt->opaque;
		auto pTransparent = &opt->transparent;
		glm::ivec2 transmissionFramebufferSize = m_pRenderPass->m_pGBuffer->m_HDR.get_size();
		auto& ns = *pNodes;
		for (auto i : _nodes)
		{
			tfNode* pNode = &ns[i];
			assert(pNode && pNode->meshIndex >= 0);
			glm::ivec2 sm = { pNode->skinIndex, pNode->meshIndex };
			// skinning matrices constant buffer
			VkDescriptorBufferInfo* pPerSkeleton = _ptb->GetSkinningMatricesBuffer(sm);
			auto nodemat = pNodesMatrices[i].GetCurrent();
			glm::mat4 mModelViewProj = _ptb->m_pGLTFCommon->_perFrameData->mCameraCurrViewProj * nodemat;
			// loop through primitives
			PBRMesh* pMesh = &m_meshes[pNode->meshIndex];
			for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
			{
				PBRPrimitives* pPrimitive = &pMesh->m_pPrimitives[p];
				//if ((bWireframe && pPrimitive->m_pipelineWireframe == VK_NULL_HANDLE) || (!bWireframe && pPrimitive->m_pipeline == VK_NULL_HANDLE))
				if (!pPrimitive->_pipe->m_pipeline)
					continue;
				// 材质变体
				if (pPrimitive->material_variants.size() > 0)
				{
					if (pPrimitive->variants_id != pPrimitive->variants_id0) {
						pPrimitive->variants_id = pPrimitive->variants_id0;
						auto& it = pPrimitive->material_variants[pPrimitive->variants_id];
						if (it.mid < m_materialsData.size())
						{
							pPrimitive->mid = it.mid;
							pPrimitive->m_pMaterial = &m_materialsData[pPrimitive->mid];
						}
					}
				}
				auto morph = _ptb->get_mb(i);
				auto uvtDesc = _ptb->get_uvm(pPrimitive->mid);
				auto instance_info = _ptb->get_instance_info();
				// do frustrum culling
				/*tfPrimitives boundingBox = _ptb->m_pGLTFCommon->m_meshes[pNode->meshIndex].m_pPrimitives[p];
				//todo 相机剔除算法
				if (CameraFrustumToBoxCollision(mModelViewProj, boundingBox.m_center, boundingBox.m_radius))
					continue;*/
				PBRMaterialParameters* pPbrParams = &pPrimitive->m_pMaterial->m_pbrMaterialParameters;
				pPbrParams->m_params.transmissionFramebufferSize = transmissionFramebufferSize;
				// Set per Object constants from material
				per_object* cbPerObject;
				VkDescriptorBufferInfo perObjectDesc;
				m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_object), (void**)&cbPerObject, &perObjectDesc);
				cbPerObject->mCurrentWorld = pNodesMatrices[i].GetCurrent();
				cbPerObject->mPreviousWorld = pNodesMatrices[i].GetPrevious();
				cbPerObject->m_pbrParams = pPbrParams->m_params;
				// compute depth for sorting
				glm::vec4 v = _ptb->m_pGLTFCommon->m_meshes[pNode->meshIndex].m_pPrimitives[p].m_center;
				float depth = (mModelViewProj * v).w;
				BatchList t = {};
				t.m_depth = depth;
				if (pNode->negativescale) { t.frontFace = VK_FRONT_FACE_CLOCKWISE; }
				t.m_pPrimitive = pPrimitive;
				t.m_perFrameDesc = _ptb->m_perFrameConstants;
				t.m_perObjectDesc = perObjectDesc;
				t.m_pPerSkeleton = pPerSkeleton;
				t.morph = morph;
				t.m_uvtDesc = uvtDesc;
				t.instance_info = instance_info;
				// append primitive to list 
				int wx = 0;
				if (pPbrParams->transmission) {
					opt->transmission.push_back(t);
					wx = 2;
				}
				else if (pPbrParams->m_blending == false)
				{
					pSolid->push_back(t);
					wx = 0;
				}
				else if (pPbrParams->m_blending)
				{
					pTransparent->push_back(t);
					wx = 1;
				}
				if (bWireframe && pPrimitive->_pipe->m_pipelineWireframe) {
					t.m_perFrameDesc = _ptb->m_perFrameConstants_w;
					opt->wireframe[wx].push_back(t);
				}
			}
		}
	}

	void GltfPbrPass::DrawBatchList(Device* dev, VkCommandBuffer commandBuffer, std::vector<BatchList>* pBatchList, bool bWireframe/*=false*/)
	{
		SetPerfMarkerBegin(commandBuffer, "gltfPBR");
		uint32_t uniformOffsets[6] = {};
		for (auto& t : *pBatchList)
		{
			uint32_t c = 2;
			uniformOffsets[0] = t.m_perFrameDesc.offset;	// 相机、灯光
			uniformOffsets[1] = t.m_perObjectDesc.offset;	// 材质参数
			if (t.m_pPerSkeleton) { uniformOffsets[c++] = t.m_pPerSkeleton->offset; }	// 骨骼动画偏移
			if (t.morph) { uniformOffsets[c++] = t.morph->offset; }						// 变形动画偏移
			if (t.m_uvtDesc) { uniformOffsets[c++] = t.m_uvtDesc->offset; }				// UV矩阵偏移
			if (t.instance_info) { uniformOffsets[c++] = t.instance_info->offset; }		// 实例矩阵偏移
			// todo FrontFace
			if (dev->_vkCmdSetFrontFace) dev->_vkCmdSetFrontFace(commandBuffer, (VkFrontFace)t.frontFace);
			t.m_pPrimitive->DrawPrimitive(commandBuffer, uniformOffsets, c, bWireframe, &t);
		}
		SetPerfMarkerEnd(commandBuffer);
	}
	void PBRPrimitives::DrawPrimitive(VkCommandBuffer cmd_buf, uint32_t* uniformOffsets, uint32_t uniformOffsetsCount, bool bWireframe, void* t0)
	{
		BatchList& t = *(BatchList*)t0;
		// Bind indices and vertices using the right offsets into the buffer 
		for (uint32_t i = 0; i < m_geometry.m_VBV.size(); i++) { vkCmdBindVertexBuffers(cmd_buf, i, 1, &m_geometry.m_VBV[i].buffer, &m_geometry.m_VBV[i].offset); }
		if (m_geometry.m_IBV.buffer)
			vkCmdBindIndexBuffer(cmd_buf, m_geometry.m_IBV.buffer, m_geometry.m_IBV.offset, m_geometry.m_indexType);
		VkDescriptorSet descritorSets[2] = { m_uniformsDescriptorSet, m_pMaterial->m_texturesDescriptorSet };
		uint32_t descritorSetsCount = (m_pMaterial->m_textureCount == 0) ? 1 : 2;
		if (!uniformOffsets) { uniformOffsetsCount = 0; }
		if (!uniformOffsetsCount) { uniformOffsets = 0; }
		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipe->m_pipelineLayout, 0, descritorSetsCount, descritorSets, uniformOffsetsCount, uniformOffsets);
		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, bWireframe ? _pipe->m_pipelineWireframe : _pipe->m_pipeline);
		if (m_geometry.m_IBV.buffer)
			vkCmdDrawIndexed(cmd_buf, m_geometry.m_NumIndices, m_geometry.instanceCount, 0, 0, 0);
		else
			vkCmdDraw(cmd_buf, m_geometry.m_NumIndices, m_geometry.instanceCount, 0, 0);
	}

#if 1
	glm::vec2 hammersley(uint32_t i, uint32_t N) {
		// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
		uint32_t bits = (i << 16u) | (i >> 16u);
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		float rdi = float(bits) * static_cast<float>(2.3283064365386963e-10);
		return glm::vec2(float(i) / float(N), rdi);
	}

	float G1(float k, float NoV) {
		return NoV / (NoV * (1.0f - k) + k);
	}

	// Geometric Shadowing function
	float gSmith(float NoL, float NoV, float roughness) {
		float k = (roughness * roughness) * 0.5f;
		return G1(k, NoL) * G1(k, NoV);
	}

	// Sample a half-vector in world space
	// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
	glm::vec3 importanceSampleGGX(glm::vec2 Xi, float roughness, glm::vec3 N) {
		// Maps a 2D point to a hemisphere with spread based on roughness
		float a = roughness * roughness;
		float phi = 2.0f * glm::pi<float>() * Xi.x;
		float cosTheta = sqrt(glm::clamp((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y), 0.0f, 1.0f));
		float sinTheta = sqrt(glm::clamp(1.0f - cosTheta * cosTheta, 0.0f, 1.0f));

		glm::vec3 H = glm::vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

		glm::vec3 up = glm::abs(N.z) < 0.999 ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
		glm::vec3 tangent = glm::normalize(glm::cross(up, N));
		glm::vec3 bitangent = glm::cross(N, tangent);

		return glm::normalize(tangent * H.x + bitangent * H.y + N * H.z);
	}

	// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
	glm::vec2 integrateBRDF(float roughness, float NoV) {
		const glm::vec3 N = glm::vec3(0.0, 0.0, 1.0); // normal always pointing forward.
		const glm::vec3 V = glm::vec3(sqrt(glm::clamp(1.0 - NoV * NoV, 0.0, 1.0)), 0.0, NoV);
		float A = 0.0f;
		float B = 0.0f;

		uint32_t numSamples = 1024u;
		for (uint32_t i = 0u; i < numSamples; ++i) {
			glm::vec2 Xi = hammersley(i, numSamples);
			glm::vec3 H = importanceSampleGGX(Xi, roughness, N);
			glm::vec3 L = 2.0f * glm::dot(V, H) * H - V;

			float NoL = glm::max(glm::dot(N, L), 0.0f);
			if (NoL > 0.0f) {
				float NoH = glm::max(glm::dot(N, H), 0.001f);
				float VoH = glm::max(glm::dot(V, H), 0.001f);
				float currentNoV = glm::max(glm::dot(N, V), 0.001f);

				const float G = gSmith(NoL, currentNoV, roughness);

				const float G_Vis = (G * VoH) / (NoH * currentNoV /*avoid division by zero*/);
				const float Fc = pow(1.0f - VoH, 5.0f);

				A += (1.0f - Fc) * G_Vis;
				B += Fc * G_Vis;
			}
		}
		return glm::vec2(A, B) / float(numSamples);
	}

	// Call this to generate the BRDF Lookup table as a data array on a .h file ready to be used.
	std::vector<uint32_t> generateCookTorranceBRDFLUT(uint32_t mapDim, int type)
	{
		// byte *data = new byte[mapDim * mapDim * sizeof(glm::detail::hdata) * 2];
		std::vector<uint32_t> data;
		data.resize(mapDim * mapDim);
		auto d = data.data();
		uint32_t offset = 0;
		if (type == 0) {
			for (uint32_t j = 0; j < mapDim; ++j) {
				for (uint32_t i = 0; i < mapDim; ++i) {
					glm::vec2 v2 = integrateBRDF((static_cast<float>(j) + .5f) / static_cast<float>(mapDim), ((static_cast<float>(i) + .5f) / static_cast<float>(mapDim)));
					uint16_t halfR = glm::detail::toFloat16(v2.x);
					uint16_t halfG = glm::detail::toFloat16(v2.y);
					auto pt = d + offset;
					auto t = (glm::u16vec2*)pt;
					t->x = halfR;
					t->y = halfG;
					offset++;
				}
			}
		}
		else {
			for (uint32_t j = 0; j < mapDim; ++j) {
				for (uint32_t i = 0; i < mapDim; ++i) {
					glm::vec2 v2 = integrateBRDF((static_cast<float>(j) + .5f) / static_cast<float>(mapDim), ((static_cast<float>(i) + .5f) / static_cast<float>(mapDim)));
					uint8_t halfR = v2.x * 255.0;
					uint8_t halfG = v2.y * 255.0;
					auto pt = d + offset;
					auto t = (glm::u8vec4*)pt;
					t->x = halfR;
					t->y = halfG;
					t->z = 0;
					t->w = 0xff;
					offset++;
				}
			}
		}
		return data;
	}
#endif // 1

#if 1

	float RadicalInverse_VdC(unsigned int bits)
	{
		bits = (bits << 16u) | (bits >> 16u);
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		return float(bits) * 2.3283064365386963e-10;
	}

	glm::vec2 Hammersley(unsigned int i, unsigned int N)
	{
		return glm::vec2(float(i) / float(N), RadicalInverse_VdC(i));
	}

	glm::vec3 ImportanceSampleGGX(glm::vec2 Xi, float roughness, glm::vec3 N)
	{
		float a = roughness * roughness;

		float phi = 2.0 * glm::pi<double>() * Xi.x;
		float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
		float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

		// from spherical coordinates to cartesian coordinates
		glm::vec3 H;
		H.x = cos(phi) * sinTheta;
		H.y = sin(phi) * sinTheta;
		H.z = cosTheta;

		// from tangent-space vector to world-space sample vector
		glm::vec3 up = abs(N.z) < 0.999 ? glm::vec3(0.0, 0.0, 1.0) : glm::vec3(1.0, 0.0, 0.0);
		glm::vec3 tangent = normalize(cross(up, N));
		glm::vec3 bitangent = cross(N, tangent);

		glm::vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
		return normalize(sampleVec);
	}

	float GeometrySchlickGGX(float NdotV, float roughness)
	{
		float a = roughness;
		float k = (a * a) / 2.0;

		float nom = NdotV;
		float denom = NdotV * (1.0 - k) + k;

		return nom / denom;
	}

	float GeometrySmith(float roughness, float NoV, float NoL)
	{
		float ggx2 = GeometrySchlickGGX(NoV, roughness);
		float ggx1 = GeometrySchlickGGX(NoL, roughness);

		return ggx1 * ggx2;
	}

	glm::vec2 IntegrateBRDF(float NdotV, float roughness, unsigned int samples)
	{
		glm::vec3 V;
		V.x = sqrt(1.0 - NdotV * NdotV);
		V.y = 0.0;
		V.z = NdotV;

		float A = 0.0;
		float B = 0.0;

		glm::vec3 N = glm::vec3(0.0, 0.0, 1.0);

		for (unsigned int i = 0u; i < samples; ++i)
		{
			glm::vec2 Xi = Hammersley(i, samples);
			glm::vec3 H = ImportanceSampleGGX(Xi, roughness, N);
			glm::vec3 L = normalize(2.0f * dot(V, H) * H - V);

			float NoL = glm::max(L.z, 0.0f);
			float NoH = glm::max(H.z, 0.0f);
			float VoH = glm::max(dot(V, H), 0.0f);
			float NoV = glm::max(dot(N, V), 0.0f);

			if (NoL > 0.0)
			{
				float G = GeometrySmith(roughness, NoV, NoL);

				float G_Vis = (G * VoH) / (NoH * NoV);
				float Fc = pow(1.0 - VoH, 5.0);

				A += (1.0 - Fc) * G_Vis;
				B += Fc * G_Vis;
			}
		}

		return glm::vec2(A / float(samples), B / float(samples));
	}
	//0:VK_FORMAT_R16G16_SFLOAT, 1:VK_FORMAT_R32G32_SFLOAT
	std::vector<uint32_t> generateCookTorranceBRDFLUT1632f(uint32_t mapDim, int type)
	{
		std::vector<uint32_t> data;
		data.resize(mapDim * mapDim * (type ? 2 : 1));
		auto d = data.data();
		//Here we set up the default parameters
		int samples = 1024;
		int size = mapDim;
		if (type)
		{
			auto pt = (glm::vec2*)d;
			for (int y = 0; y < size; y++)
			{
				for (int x = 0; x < size; x++)
				{
					float NoV = (y + 0.5f) * (1.0f / size);
					float roughness = (x + 0.5f) * (1.0f / size);
					auto v = IntegrateBRDF(NoV, roughness, samples);
					pt[size - 1 - x] = v;
				}
				pt += size;
			}
		}
		else
		{
			auto pt = d;
			for (int y = 0; y < size; y++)
			{
				pt = d + y * size;
				for (int x = 0; x < size; x++)
				{
					float NoV = (y + 0.5f) * (1.0f / size);
					float roughness = (x + 0.5f) * (1.0f / size);
					auto v = IntegrateBRDF(NoV, roughness, samples);
					pt[size - 1 - x] = glm::packHalf2x16(v);
				}
			}
		}
		return data;
	}
#endif // 1


}
//!vkr

// 纹理

namespace vkr
{
	VkFormat TranslateDxgiFormatIntoVulkans(DXGI_FORMAT format);

	// Return the BPP for a particular format
	size_t BitsPerPixel(DXGI_FORMAT fmt);
	size_t GetPixelByteSize(DXGI_FORMAT fmt);

	// Convert a *_SRGB format into a non-gamma one
	DXGI_FORMAT ConvertIntoNonGammaFormat(DXGI_FORMAT format);

	DXGI_FORMAT ConvertIntoGammaFormat(DXGI_FORMAT format);

	DXGI_FORMAT SetFormatGamma(DXGI_FORMAT format, bool addGamma);

	bool IsBCFormat(DXGI_FORMAT format);
	//--------------------------------------------------------------------------------------
	// Constructor of the Texture class
	// initializes all members
	//--------------------------------------------------------------------------------------
	Texture::Texture() {}

	//--------------------------------------------------------------------------------------
	// Destructor of the Texture class
	//--------------------------------------------------------------------------------------
	Texture::~Texture() {}

	void Texture::OnDestroy()
	{
#ifdef USE_VMA
		if (m_pResource != VK_NULL_HANDLE)
		{
			vmaDestroyImage(m_pDevice->GetAllocator(), m_pResource, m_ImageAlloc);
			m_pResource = VK_NULL_HANDLE;
		}
#else
		if (m_deviceMemory != VK_NULL_HANDLE)
		{
			vkDestroyImage(m_pDevice->m_device, m_pResource, nullptr);
			vkFreeMemory(m_pDevice->m_device, m_deviceMemory, nullptr);
			m_deviceMemory = VK_NULL_HANDLE;
		}
#endif

	}

	bool Texture::isCubemap() const
	{
		return m_header.arraySize == 6;
	}

	INT32 Texture::Init(Device* pDevice, VkImageCreateInfo* pCreateInfo, const char* name)
	{
		m_pDevice = pDevice;
		m_header.mipMapCount = pCreateInfo->mipLevels;
		m_header.width = pCreateInfo->extent.width;
		m_header.height = pCreateInfo->extent.height;
		m_header.depth = pCreateInfo->extent.depth;
		m_header.arraySize = pCreateInfo->arrayLayers;
		m_format = pCreateInfo->format;
		if (name)
			m_name = name;

#ifdef USE_VMA
		VmaAllocationCreateInfo imageAllocCreateInfo = {};
		imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		imageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
		imageAllocCreateInfo.pUserData = (void*)m_name.c_str();
		VmaAllocationInfo gpuImageAllocInfo = {};
		VkResult res = vmaCreateImage(m_pDevice->GetAllocator(), pCreateInfo, &imageAllocCreateInfo, &m_pResource, &m_ImageAlloc, &gpuImageAllocInfo);
		assert(res == VK_SUCCESS);
		SetResourceName(pDevice->m_device, VK_OBJECT_TYPE_IMAGE, (uint64_t)m_pResource, m_name.c_str());
#else
		/* Create image */
		VkResult res = vkCreateImage(m_pDevice->m_device, pCreateInfo, NULL, &m_pResource);
		assert(res == VK_SUCCESS);

		VkMemoryRequirements mem_reqs;
		vkGetImageMemoryRequirements(m_pDevice->m_device, m_pResource, &mem_reqs);

		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.pNext = NULL;
		alloc_info.allocationSize = 0;
		alloc_info.allocationSize = mem_reqs.size;
		alloc_info.memoryTypeIndex = 0;

		bool pass = memory_type_from_properties(m_pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&alloc_info.memoryTypeIndex);
		assert(pass);

		/* Allocate memory */
		res = vkAllocateMemory(m_pDevice->m_device, &alloc_info, NULL, &m_deviceMemory);
		assert(res == VK_SUCCESS);

		/* bind memory */
		res = vkBindImageMemory(m_pDevice->m_device, m_pResource, m_deviceMemory, 0);
		assert(res == VK_SUCCESS);
#endif
		return 0;
	}

	INT32 Texture::InitRenderTarget(Device* pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, VkImageUsageFlags usage, bool bUAV, const char* name, VkImageCreateFlagBits flags)
	{
		VkImageCreateInfo image_info = {};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.pNext = NULL;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.format = format;
		image_info.extent.width = width;
		image_info.extent.height = height;
		image_info.extent.depth = 1;
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.samples = msaa;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.queueFamilyIndexCount = 0;
		image_info.pQueueFamilyIndices = NULL;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_info.usage = usage;
		image_info.flags = flags;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;   // VK_IMAGE_TILING_LINEAR should never be used and will never be faster

		return Init(pDevice, &image_info, name);
	}
	bool is_depth_tex(VkFormat format) {
		return !(format < VK_FORMAT_D16_UNORM || format > VK_FORMAT_D32_SFLOAT_S8_UINT);//VK_FORMAT_D32_SFLOAT
	}
	bool is_stencil_tex(VkFormat format) {
		bool d = format == VK_FORMAT_S8_UINT || format == VK_FORMAT_D16_UNORM_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
		return d;
	}
	void Texture::CreateRTV(VkImageView* pImageView, int mipLevel, VkFormat format)
	{
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = m_pResource;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		if (m_header.arraySize > 1)
		{
			info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			info.subresourceRange.layerCount = m_header.arraySize;
		}
		else
		{
			info.subresourceRange.layerCount = 1;
		}
		if (format == VK_FORMAT_UNDEFINED)
			info.format = m_format;
		else
			info.format = format;
		if (is_depth_tex(m_format))
			info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		else
			info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//if (is_stencil_tex(m_format))
		//	info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		std::string ResourceName = m_name;

		if (mipLevel == -1)
		{
			info.subresourceRange.baseMipLevel = 0;
			info.subresourceRange.levelCount = m_header.mipMapCount;
		}
		else
		{
			info.subresourceRange.baseMipLevel = mipLevel;
			info.subresourceRange.levelCount = 1;
			ResourceName += std::to_string(mipLevel);
		}

		info.subresourceRange.baseArrayLayer = 0;
		if (!m_pResource)return;
		VkResult res = vkCreateImageView(m_pDevice->m_device, &info, NULL, pImageView);
		assert(res == VK_SUCCESS);

		SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*pImageView, ResourceName.c_str());
	}

	void Texture::CreateSRV(VkImageView* pImageView, int mipLevel)
	{
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = m_pResource;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		if (m_header.arraySize > 1)
		{
			info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			info.subresourceRange.layerCount = m_header.arraySize;
		}
		else
		{
			info.subresourceRange.layerCount = 1;
		}
		info.format = m_format;
		if (is_depth_tex(m_format))
			info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		else
			info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//if (is_stencil_tex(m_format))
		//	info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		if (mipLevel == -1)
		{
			info.subresourceRange.baseMipLevel = 0;
			info.subresourceRange.levelCount = m_header.mipMapCount;
		}
		else
		{
			info.subresourceRange.baseMipLevel = mipLevel;
			info.subresourceRange.levelCount = 1;
		}

		info.subresourceRange.baseArrayLayer = 0;

		if (!m_pResource)return;
		VkResult res = vkCreateImageView(m_pDevice->m_device, &info, NULL, pImageView);
		assert(res == VK_SUCCESS);

		SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*pImageView, m_name.c_str());
	}

	void Texture::CreateCubeSRV(VkImageView* pImageView)
	{
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = m_pResource;
		info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		info.format = m_format;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = m_header.mipMapCount;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = m_header.arraySize;
		if (!m_pResource)return;
		VkResult res = vkCreateImageView(m_pDevice->m_device, &info, NULL, pImageView);
		assert(res == VK_SUCCESS);

		SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*pImageView, m_name.c_str());
	}

	void Texture::CreateDSV(VkImageView* pImageView)
	{
		VkImageViewCreateInfo view_info = {};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.pNext = NULL;
		view_info.image = m_pResource;
		view_info.format = m_format;
		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = 1;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

		if (m_format == VK_FORMAT_D16_UNORM_S8_UINT || m_format == VK_FORMAT_D24_UNORM_S8_UINT || m_format == VK_FORMAT_D32_SFLOAT_S8_UINT)
			//if (is_depth_tex(m_format))
		{
			view_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		m_header.mipMapCount = 1;

		if (!m_pResource)return;
		VkResult res = vkCreateImageView(m_pDevice->m_device, &view_info, NULL, pImageView);
		assert(res == VK_SUCCESS);

		SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*pImageView, m_name.c_str());
	}

	INT32 Texture::InitDepthStencil(Device* pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, const char* name)
	{
		VkImageCreateInfo image_info = {};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.pNext = NULL;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.format = format;
		image_info.extent.width = width;
		image_info.extent.height = height;
		image_info.extent.depth = 1;
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.samples = msaa;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.queueFamilyIndexCount = 0;
		image_info.pQueueFamilyIndices = NULL;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image_info.flags = 0;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;   // VK_IMAGE_TILING_LINEAR should never be used and will never be faster

		return Init(pDevice, &image_info, name);
	}

	//--------------------------------------------------------------------------------------
	// create a comitted resource using m_header
	//--------------------------------------------------------------------------------------
	VkImage Texture::CreateTextureCommitted(Device* pDevice, UploadHeap* pUploadHeap, const char* pName, bool useSRGB, VkImageUsageFlags usageFlags)
	{
		VkImageCreateInfo info = {};

		if (pName)
			m_name = pName;

		if (useSRGB && ((usageFlags & VK_IMAGE_USAGE_STORAGE_BIT) != 0))
		{
			// the storage bit is not supported for srgb formats
			// we can still use the srgb format on an image view if the access is read-only
			// for write access, we need to use an image view with unorm format
			// this is ok as srgb and unorm formats are compatible with each other
			VkImageFormatListCreateInfo formatListInfo = {};
			formatListInfo.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
			formatListInfo.viewFormatCount = 2;
			VkFormat list[2];
			list[0] = TranslateDxgiFormatIntoVulkans(m_header.format);
			list[1] = TranslateDxgiFormatIntoVulkans(SetFormatGamma(m_header.format, useSRGB));
			formatListInfo.pViewFormats = list;

			info.pNext = &formatListInfo;
		}
		else {
			m_header.format = SetFormatGamma(m_header.format, useSRGB);
		}

		m_format = m_header.vkformat ? m_header.vkformat : TranslateDxgiFormatIntoVulkans((DXGI_FORMAT)m_header.format);

		VkImage tex;

		// Create the Image:
		{
			info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			info.imageType = VK_IMAGE_TYPE_2D;
			info.format = m_format;
			info.extent.width = m_header.width;
			info.extent.height = m_header.height;
			info.extent.depth = 1;
			info.mipLevels = m_header.mipMapCount;
			info.arrayLayers = m_header.arraySize;
			if (m_header.arraySize == 6)
				info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			info.samples = VK_SAMPLE_COUNT_1_BIT;
			info.tiling = VK_IMAGE_TILING_OPTIMAL;
			info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | usageFlags;
			info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			// allocate memory and bind the image to it
#ifdef USE_VMA
			VmaAllocationCreateInfo imageAllocCreateInfo = {};
			imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			imageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
			imageAllocCreateInfo.pUserData = (void*)m_name.c_str();
			VmaAllocationInfo gpuImageAllocInfo = {};
			VkResult res = vmaCreateImage(pDevice->GetAllocator(), &info, &imageAllocCreateInfo, &tex, &m_ImageAlloc, &gpuImageAllocInfo);
			assert(res == VK_SUCCESS);
			SetResourceName(pDevice->m_device, VK_OBJECT_TYPE_IMAGE, (uint64_t)tex, m_name.c_str());
#else
			VkResult res = vkCreateImage(pDevice->m_device, &info, NULL, &tex);
			assert(res == VK_SUCCESS);

			VkMemoryRequirements mem_reqs;
			vkGetImageMemoryRequirements(pDevice->m_device, tex, &mem_reqs);

			VkMemoryAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize = mem_reqs.size;
			alloc_info.memoryTypeIndex = 0;

			bool pass = memory_type_from_properties(pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&alloc_info.memoryTypeIndex);
			assert(pass && "No mappable, coherent memory");

			res = vkAllocateMemory(pDevice->m_device, &alloc_info, NULL, &m_deviceMemory);
			assert(res == VK_SUCCESS);

			res = vkBindImageMemory(pDevice->m_device, tex, m_deviceMemory, 0);
			assert(res == VK_SUCCESS);
#endif
		}

		return tex;
	}

	void Texture::LoadAndUpload(Device* pDevice, UploadHeap* pUploadHeap, ImgLoader* pDds, VkImage pTexture2D)
	{
		// Upload Image
		{
			VkImageMemoryBarrier copy_barrier = {};
			copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			copy_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			copy_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier.image = pTexture2D;
			copy_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy_barrier.subresourceRange.baseMipLevel = 0;
			copy_barrier.subresourceRange.levelCount = m_header.mipMapCount;
			copy_barrier.subresourceRange.layerCount = m_header.arraySize;
			pUploadHeap->AddPreBarrier(copy_barrier);
		}

		//compute pixel size
		//
		UINT32 bytesPerPixel = (UINT32)GetPixelByteSize((DXGI_FORMAT)m_header.format); // note that bytesPerPixel in BC formats is treated as bytesPerBlock 
		UINT32 pixelsPerBlock = 1;
		if (IsBCFormat(m_header.format))
		{
			pixelsPerBlock = 4 * 4; // BC formats have 4*4 pixels per block
		}

		for (uint32_t a = 0; a < m_header.arraySize; a++)
		{
			// copy all the mip slices into the offsets specified by the footprint structure
			//
			for (uint32_t mip = 0; mip < m_header.mipMapCount; mip++)
			{
				uint32_t dwWidth = std::max<uint32_t>(m_header.width >> mip, 1);
				uint32_t dwHeight = std::max<uint32_t>(m_header.height >> mip, 1);

				UINT64 UplHeapSize = (dwWidth * dwHeight * bytesPerPixel) / pixelsPerBlock;
				UINT8* pixels = pUploadHeap->BeginSuballocate(SIZE_T(UplHeapSize), 512);

				if (pixels == NULL)
				{
					// oh! We ran out of mem in the upload heap, flush it and try allocating mem from it again
					pUploadHeap->FlushAndFinish(true);
					pixels = pUploadHeap->Suballocate(SIZE_T(UplHeapSize), 512);
					assert(pixels != NULL);
				}

				uint32_t offset = uint32_t(pixels - pUploadHeap->BasePtr());

				pDds->CopyPixels(pixels, (dwWidth * bytesPerPixel) / pixelsPerBlock, (dwWidth * bytesPerPixel) / pixelsPerBlock, dwHeight);

				pUploadHeap->EndSuballocate();

				{
					VkBufferImageCopy region = {};
					region.bufferOffset = offset;
					region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					region.imageSubresource.layerCount = 1;
					region.imageSubresource.baseArrayLayer = a;
					region.imageSubresource.mipLevel = mip;
					region.imageExtent.width = dwWidth;
					region.imageExtent.height = dwHeight;
					region.imageExtent.depth = 1;
					pUploadHeap->AddCopy(pTexture2D, region);
				}
			}
		}

		// prepare to shader read
		//
		{
			VkImageMemoryBarrier use_barrier = {};
			use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier.image = pTexture2D;
			use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			use_barrier.subresourceRange.levelCount = m_header.mipMapCount;
			use_barrier.subresourceRange.layerCount = m_header.arraySize;
			pUploadHeap->AddPostBarrier(use_barrier);
		}
	}

	char* copyPixels(void* pDest, char* data, uint32_t stride, uint32_t bytesWidth, uint32_t height)
	{
		for (uint32_t y = 0; y < height; y++)
		{
			memcpy((char*)pDest + y * stride, data, bytesWidth);
			data += stride;
		}
		return data;
	}

	void Texture::LoadAndUpload0(Device* pDevice, UploadHeap* pUploadHeap, char* data, VkImage pTexture2D)
	{
		// Upload Image
		{
			VkImageMemoryBarrier copy_barrier = {};
			copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			copy_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			copy_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier.image = pTexture2D;
			copy_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy_barrier.subresourceRange.baseMipLevel = 0;
			copy_barrier.subresourceRange.levelCount = m_header.mipMapCount;
			copy_barrier.subresourceRange.layerCount = m_header.arraySize;
			pUploadHeap->AddPreBarrier(copy_barrier);
		}
		//compute pixel size
		UINT32 bytesPerPixel = (UINT32)GetPixelByteSize((DXGI_FORMAT)m_header.format); // note that bytesPerPixel in BC formats is treated as bytesPerBlock 
		UINT32 pixelsPerBlock = 1;
		if (IsBCFormat(m_header.format))
		{
			pixelsPerBlock = 4 * 4; // BC formats have 4*4 pixels per block
		}
		auto t = data;
		for (uint32_t a = 0; a < m_header.arraySize; a++)
		{
			// copy all the mip slices into the offsets specified by the footprint structure 
			for (uint32_t mip = 0; mip < m_header.mipMapCount; mip++)
			{
				uint32_t dwWidth = std::max<uint32_t>(m_header.width >> mip, 1);
				uint32_t dwHeight = std::max<uint32_t>(m_header.height >> mip, 1);
				UINT64 UplHeapSize = (dwWidth * dwHeight * bytesPerPixel) / pixelsPerBlock;
				UINT8* pixels = pUploadHeap->BeginSuballocate(SIZE_T(UplHeapSize), 512);
				if (pixels == NULL)
				{
					// oh! We ran out of mem in the upload heap, flush it and try allocating mem from it again
					pUploadHeap->FlushAndFinish(true);
					pixels = pUploadHeap->Suballocate(SIZE_T(UplHeapSize), 512);
					assert(pixels != NULL);
				}
				uint32_t offset = uint32_t(pixels - pUploadHeap->BasePtr());
				t = copyPixels(pixels, t, (dwWidth * bytesPerPixel) / pixelsPerBlock, (dwWidth * bytesPerPixel) / pixelsPerBlock, dwHeight);
				pUploadHeap->EndSuballocate();
				{
					VkBufferImageCopy region = {};
					region.bufferOffset = offset;
					region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					region.imageSubresource.layerCount = 1;
					region.imageSubresource.baseArrayLayer = a;
					region.imageSubresource.mipLevel = mip;
					region.imageExtent.width = dwWidth;
					region.imageExtent.height = dwHeight;
					region.imageExtent.depth = 1;
					pUploadHeap->AddCopy(pTexture2D, region);
				}
			}
		}
		// prepare to shader read
		{
			VkImageMemoryBarrier use_barrier = {};
			use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier.image = pTexture2D;
			use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			use_barrier.subresourceRange.levelCount = m_header.mipMapCount;
			use_barrier.subresourceRange.layerCount = m_header.arraySize;
			pUploadHeap->AddPostBarrier(use_barrier);
		}
	}

	void upload_data(Device* pDevice, UploadHeap* up, IMG_INFO* info, uint32_t bufferOffset, uint32_t usage, VkImage image)
	{
		uint32_t imageUsageFlags = usage | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; //uint32_t imageLayout;
		{

			//mipLevels = info->mipMapCount;
			//mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

			// Copy to Image:
			{
				VkImageMemoryBarrier copy_barrier = {};
				copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				copy_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				copy_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				copy_barrier.image = image;
				copy_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copy_barrier.subresourceRange.levelCount = info->mipMapCount;
				copy_barrier.subresourceRange.layerCount = info->arraySize;
				//vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copy_barrier);
				up->AddPreBarrier(copy_barrier);
				VkBufferImageCopy region = {};
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.layerCount = info->arraySize;
				region.imageExtent.width = info->width;
				region.imageExtent.height = info->height;
				region.imageExtent.depth = info->depth;
				region.bufferOffset = bufferOffset;
				//vkCmdCopyBufferToImage(copyCmd, staging->buffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
				up->AddCopy(image, region);
				VkImageMemoryBarrier use_barrier = {};
				use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				use_barrier.image = image;
				use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				use_barrier.subresourceRange.levelCount = info->mipMapCount;
				use_barrier.subresourceRange.layerCount = info->arraySize;
				//vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, use_barrier);
				up->AddPostBarrier(use_barrier);
			}

		}
	}
	//--------------------------------------------------------------------------------------
	// entry function to initialize an image from a .DDS texture
	//--------------------------------------------------------------------------------------
	bool Texture::InitFromFile(Device* pDevice, UploadHeap* pUploadHeap, const char* pFilename, bool useSRGB, VkImageUsageFlags usageFlags, float cutOff)
	{
		assert(m_pResource == NULL);
		if (!pFilename || !(*pFilename))return false;
		m_pDevice = pDevice;
		ImgLoader* img = CreateImageLoader(pFilename);
		bool result = img->Load(pFilename, cutOff, &m_header);
		if (result)
		{
			m_pResource = CreateTextureCommitted(pDevice, pUploadHeap, pFilename, useSRGB, usageFlags);
			LoadAndUpload(pDevice, pUploadHeap, img, m_pResource);
		}
		else
		{
			Trace("Error loading texture from file: %s", pFilename);
			assert(result && "Could not load requested file. Please make sure it exists on disk.");
		}

		delete(img);

		return result;
	}

	bool Texture::InitFromData(Device* pDevice, UploadHeap* uploadHeap, IMG_INFO* header, const void* data, int dsize, const char* name, bool useSRGB)
	{
		assert(!m_pResource && (m_pDevice || m_pDevice != pDevice));
		m_pDevice = pDevice;
		m_header = *header;
		m_pResource = CreateTextureCommitted(m_pDevice, uploadHeap, name, useSRGB);
		if (header->arraySize == 1 && header->mipMapCount == 1) {
			UINT8* pixels = 0;
			pixels = uploadHeap->Suballocate(dsize, 512);
			assert(pixels);
			memcpy(pixels, data, dsize);
			uint32_t offset = uint32_t(pixels - uploadHeap->BasePtr());
			upload_data(pDevice, uploadHeap, header, offset, 0, m_pResource);
		}
		else {
			LoadAndUpload0(pDevice, uploadHeap, (char*)data, m_pResource);
		}
		return true;
	}

	VkFormat TranslateDxgiFormatIntoVulkans(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
		case DXGI_FORMAT_BC1_UNORM:         return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		case DXGI_FORMAT_BC1_UNORM_SRGB:    return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		case DXGI_FORMAT_BC2_UNORM:         return VK_FORMAT_BC2_UNORM_BLOCK;
		case DXGI_FORMAT_BC2_UNORM_SRGB:    return VK_FORMAT_BC2_SRGB_BLOCK;
		case DXGI_FORMAT_BC3_UNORM:         return VK_FORMAT_BC3_UNORM_BLOCK;
		case DXGI_FORMAT_BC3_UNORM_SRGB:    return VK_FORMAT_BC3_SRGB_BLOCK;
		case DXGI_FORMAT_BC4_UNORM:         return VK_FORMAT_BC4_UNORM_BLOCK;
		case DXGI_FORMAT_BC4_SNORM:         return VK_FORMAT_BC4_UNORM_BLOCK;
		case DXGI_FORMAT_BC5_UNORM:         return VK_FORMAT_BC5_UNORM_BLOCK;
		case DXGI_FORMAT_BC5_SNORM:         return VK_FORMAT_BC5_UNORM_BLOCK;
		case DXGI_FORMAT_B5G6R5_UNORM:      return VK_FORMAT_B5G6R5_UNORM_PACK16;
		case DXGI_FORMAT_B5G5R5A1_UNORM:    return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
		case DXGI_FORMAT_BC6H_UF16:         return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case DXGI_FORMAT_BC6H_SF16:         return VK_FORMAT_BC6H_SFLOAT_BLOCK;
		case DXGI_FORMAT_BC7_UNORM:         return VK_FORMAT_BC7_UNORM_BLOCK;
		case DXGI_FORMAT_BC7_UNORM_SRGB:    return VK_FORMAT_BC7_SRGB_BLOCK;
		case DXGI_FORMAT_R10G10B10A2_UNORM: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case DXGI_FORMAT_R16G16B16A16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case DXGI_FORMAT_R32G32B32A32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case DXGI_FORMAT_A8_UNORM:          return VK_FORMAT_R8_UNORM;
		default: assert(false);  return VK_FORMAT_UNDEFINED;
		}
	}

	//--------------------------------------------------------------------------------------
	// Return the BPP for a particular format
	//--------------------------------------------------------------------------------------
	size_t BitsPerPixel(DXGI_FORMAT fmt)
	{
		switch (fmt)
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 128;

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 96;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_Y416:
		case DXGI_FORMAT_Y210:
		case DXGI_FORMAT_Y216:
			return 64;

		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_AYUV:
		case DXGI_FORMAT_Y410:
		case DXGI_FORMAT_YUY2:
			return 32;

		case DXGI_FORMAT_P010:
		case DXGI_FORMAT_P016:
			return 24;

		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_A8P8:
		case DXGI_FORMAT_B4G4R4A4_UNORM:
			return 16;

		case DXGI_FORMAT_NV12:
		case DXGI_FORMAT_420_OPAQUE:
		case DXGI_FORMAT_NV11:
			return 12;

		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_AI44:
		case DXGI_FORMAT_IA44:
		case DXGI_FORMAT_P8:
			return 8;

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 8;

		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return 4;

		case DXGI_FORMAT_R1_UNORM:
			return 1;

		default:
			return 0;
		}
	}

	//--------------------------------------------------------------------------------------
	// return the byte size of a pixel (or block if block compressed)
	//--------------------------------------------------------------------------------------
	size_t GetPixelByteSize(DXGI_FORMAT fmt)
	{
		switch (fmt)
		{
		case(DXGI_FORMAT_A8_UNORM):
			return 1;

		case(DXGI_FORMAT_R10G10B10A2_TYPELESS):
		case(DXGI_FORMAT_R10G10B10A2_UNORM):
		case(DXGI_FORMAT_R10G10B10A2_UINT):
		case(DXGI_FORMAT_R11G11B10_FLOAT):
		case(DXGI_FORMAT_R8G8B8A8_TYPELESS):
		case(DXGI_FORMAT_R8G8B8A8_UNORM):
		case(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB):
		case(DXGI_FORMAT_R8G8B8A8_UINT):
		case(DXGI_FORMAT_R8G8B8A8_SNORM):
		case(DXGI_FORMAT_R8G8B8A8_SINT):
		case(DXGI_FORMAT_B8G8R8A8_UNORM):
		case(DXGI_FORMAT_B8G8R8X8_UNORM):
		case(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM):
		case(DXGI_FORMAT_B8G8R8A8_TYPELESS):
		case(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB):
		case(DXGI_FORMAT_B8G8R8X8_TYPELESS):
		case(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB):
		case(DXGI_FORMAT_R16G16_TYPELESS):
		case(DXGI_FORMAT_R16G16_FLOAT):
		case(DXGI_FORMAT_R16G16_UNORM):
		case(DXGI_FORMAT_R16G16_UINT):
		case(DXGI_FORMAT_R16G16_SNORM):
		case(DXGI_FORMAT_R16G16_SINT):
		case(DXGI_FORMAT_R32_TYPELESS):
		case(DXGI_FORMAT_D32_FLOAT):
		case(DXGI_FORMAT_R32_FLOAT):
		case(DXGI_FORMAT_R32_UINT):
		case(DXGI_FORMAT_R32_SINT):
			return 4;

		case(DXGI_FORMAT_BC1_TYPELESS):
		case(DXGI_FORMAT_BC1_UNORM):
		case(DXGI_FORMAT_BC1_UNORM_SRGB):
		case(DXGI_FORMAT_BC4_TYPELESS):
		case(DXGI_FORMAT_BC4_UNORM):
		case(DXGI_FORMAT_BC4_SNORM):
		case(DXGI_FORMAT_R16G16B16A16_FLOAT):
		case(DXGI_FORMAT_R16G16B16A16_TYPELESS):
			return 8;

		case(DXGI_FORMAT_BC2_TYPELESS):
		case(DXGI_FORMAT_BC2_UNORM):
		case(DXGI_FORMAT_BC2_UNORM_SRGB):
		case(DXGI_FORMAT_BC3_TYPELESS):
		case(DXGI_FORMAT_BC3_UNORM):
		case(DXGI_FORMAT_BC3_UNORM_SRGB):
		case(DXGI_FORMAT_BC5_TYPELESS):
		case(DXGI_FORMAT_BC5_UNORM):
		case(DXGI_FORMAT_BC5_SNORM):
		case(DXGI_FORMAT_BC6H_TYPELESS):
		case(DXGI_FORMAT_BC6H_UF16):
		case(DXGI_FORMAT_BC6H_SF16):
		case(DXGI_FORMAT_BC7_TYPELESS):
		case(DXGI_FORMAT_BC7_UNORM):
		case(DXGI_FORMAT_BC7_UNORM_SRGB):
		case(DXGI_FORMAT_R32G32B32A32_FLOAT):
		case(DXGI_FORMAT_R32G32B32A32_TYPELESS):
			return 16;

		default:
			assert(0);
			break;
		}
		return 0;
	}



	//--------------------------------------------------------------------------------------
	// Convert a *_SRGB format into a non-gamma one
	//--------------------------------------------------------------------------------------
	DXGI_FORMAT ConvertIntoNonGammaFormat(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM;
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8X8_UNORM;
		case DXGI_FORMAT_BC1_UNORM_SRGB: return DXGI_FORMAT_BC1_UNORM;
		case DXGI_FORMAT_BC2_UNORM_SRGB: return DXGI_FORMAT_BC2_UNORM;
		case DXGI_FORMAT_BC3_UNORM_SRGB: return DXGI_FORMAT_BC3_UNORM;
		case DXGI_FORMAT_BC7_UNORM_SRGB: return DXGI_FORMAT_BC7_UNORM;
		}

		return format;
	}

	DXGI_FORMAT ConvertIntoGammaFormat(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case DXGI_FORMAT_B8G8R8X8_UNORM: return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
		case DXGI_FORMAT_BC1_UNORM: return DXGI_FORMAT_BC1_UNORM_SRGB;
		case DXGI_FORMAT_BC2_UNORM: return DXGI_FORMAT_BC2_UNORM_SRGB;
		case DXGI_FORMAT_BC3_UNORM: return DXGI_FORMAT_BC3_UNORM_SRGB;
		case DXGI_FORMAT_BC7_UNORM: return DXGI_FORMAT_BC7_UNORM_SRGB;
		}

		return format;
	}

	DXGI_FORMAT SetFormatGamma(DXGI_FORMAT format, bool addGamma)
	{
		if (addGamma)
		{
			format = ConvertIntoGammaFormat(format);
		}
		else
		{
			format = ConvertIntoNonGammaFormat(format);
		}

		return format;
	}

	bool IsBCFormat(DXGI_FORMAT format)
	{
		return (format >= DXGI_FORMAT_BC1_TYPELESS && format <= DXGI_FORMAT_BC5_SNORM) || (format >= DXGI_FORMAT_BC6H_TYPELESS && format <= DXGI_FORMAT_BC7_UNORM_SRGB);
	}

	//--------------------------------------------------------------------------------------
	//
	// OnCreate
	//
	//--------------------------------------------------------------------------------------
	void UploadHeap::OnCreate(Device* pDevice, SIZE_T uSize)
	{
		m_pDevice = pDevice;

		VkResult res;

		// Create command list and allocators 
		{
			VkCommandPoolCreateInfo cmd_pool_info = {};
			cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmd_pool_info.queueFamilyIndex = m_pDevice->GetGraphicsQueueFamilyIndex();
			cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			res = vkCreateCommandPool(m_pDevice->m_device, &cmd_pool_info, NULL, &m_commandPool);
			assert(res == VK_SUCCESS);

			VkCommandBufferAllocateInfo cmd = {};
			cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmd.pNext = NULL;
			cmd.commandPool = m_commandPool;
			cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmd.commandBufferCount = 1;
			res = vkAllocateCommandBuffers(m_pDevice->m_device, &cmd, &m_pCommandBuffer);
			assert(res == VK_SUCCESS);
		}

		// Create buffer to suballocate
		{
			VkBufferCreateInfo buffer_info = {};
			buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffer_info.size = uSize;
			buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			res = vkCreateBuffer(m_pDevice->m_device, &buffer_info, NULL, &m_buffer);
			assert(res == VK_SUCCESS);

			VkMemoryRequirements mem_reqs;
			vkGetBufferMemoryRequirements(m_pDevice->m_device, m_buffer, &mem_reqs);

			VkMemoryAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize = mem_reqs.size;
			alloc_info.memoryTypeIndex = 0;
			auto dmp = m_pDevice->GetPhysicalDeviceMemoryProperties();
			bool pass = memory_type_from_properties(dmp, mem_reqs.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				&alloc_info.memoryTypeIndex);
			assert(pass && "No mappable, coherent memory");

			res = vkAllocateMemory(m_pDevice->m_device, &alloc_info, NULL, &m_deviceMemory);
			assert(res == VK_SUCCESS);

			res = vkBindBufferMemory(m_pDevice->m_device, m_buffer, m_deviceMemory, 0);
			assert(res == VK_SUCCESS);

			res = vkMapMemory(m_pDevice->m_device, m_deviceMemory, 0, mem_reqs.size, 0, (void**)&m_pDataBegin);
			assert(res == VK_SUCCESS);

			m_pDataCur = m_pDataBegin;
			m_pDataEnd = m_pDataBegin + mem_reqs.size;
		}

		// Create fence
		{
			VkFenceCreateInfo fence_ci = {};
			fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

			res = vkCreateFence(m_pDevice->m_device, &fence_ci, NULL, &m_fence);
			assert(res == VK_SUCCESS);
		}

		// Begin Command Buffer
		{
			VkCommandBufferBeginInfo cmd_buf_info = {};
			cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			res = vkBeginCommandBuffer(m_pCommandBuffer, &cmd_buf_info);
			assert(res == VK_SUCCESS);
		}
	}

	//--------------------------------------------------------------------------------------
	//
	// OnDestroy
	//
	//--------------------------------------------------------------------------------------
	void UploadHeap::OnDestroy()
	{
		if (m_buffer)
		{
			vkDestroyBuffer(m_pDevice->m_device, m_buffer, NULL);
			vkUnmapMemory(m_pDevice->m_device, m_deviceMemory);
			vkFreeMemory(m_pDevice->m_device, m_deviceMemory, NULL);

			vkFreeCommandBuffers(m_pDevice->m_device, m_commandPool, 1, &m_pCommandBuffer);
			vkDestroyCommandPool(m_pDevice->m_device, m_commandPool, NULL);

			vkDestroyFence(m_pDevice->m_device, m_fence, NULL);

			m_buffer = 0;
		}
	}

	//--------------------------------------------------------------------------------------
	//
	// SuballocateFromUploadHeap
	//
	//--------------------------------------------------------------------------------------
	UINT8* UploadHeap::Suballocate(SIZE_T uSize, UINT64 uAlign)
	{
		// wait until we are done flusing the heap
		flushing.Wait();

		UINT8* pRet = NULL;

		{
			std::unique_lock<std::mutex> lock(m_mutex);

			// make sure resource (and its mips) would fit the upload heap, if not please make the upload heap bigger
			auto bca = (m_pDataEnd - m_pDataBegin);
			assert(uSize < (size_t)bca);

			m_pDataCur = reinterpret_cast<UINT8*>(AlignUp(reinterpret_cast<SIZE_T>(m_pDataCur), uAlign));
			uSize = AlignUp(uSize, uAlign);

			ac = m_pDataCur - m_pDataBegin;
			ac0 = m_pDataEnd - m_pDataBegin;
			// return NULL if we ran out of space in the heap
			if ((m_pDataCur >= m_pDataEnd) || (m_pDataCur + uSize >= m_pDataEnd))
			{
				return NULL;
			}

			pRet = m_pDataCur;
			m_pDataCur += uSize;
		}

		return pRet;
	}

	UINT8* UploadHeap::BeginSuballocate(SIZE_T uSize, UINT64 uAlign)
	{
		UINT8* pRes = NULL;

		for (;;) {
			pRes = Suballocate(uSize, uAlign);
			if (pRes != NULL)
			{
				break;
			}

			FlushAndFinish();
		}

		allocating.Inc();

		return pRes;
	}

	void UploadHeap::EndSuballocate()
	{
		allocating.Dec();
	}


	void UploadHeap::AddCopy(VkImage image, VkBufferImageCopy bufferImageCopy)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_copies.push_back({ image, bufferImageCopy });
	}

	void UploadHeap::AddPreBarrier(VkImageMemoryBarrier imageMemoryBarrier)
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		m_toPreBarrier.push_back(imageMemoryBarrier);
	}


	void UploadHeap::AddPostBarrier(VkImageMemoryBarrier imageMemoryBarrier)
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		m_toPostBarrier.push_back(imageMemoryBarrier);
	}

	void UploadHeap::Flush()
	{
		VkResult res;

		VkMappedMemoryRange range[1] = {};
		range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[0].memory = m_deviceMemory;
		range[0].size = m_pDataCur - m_pDataBegin;
		res = vkFlushMappedMemoryRanges(m_pDevice->m_device, 1, range);
		assert(res == VK_SUCCESS);
	}

	//--------------------------------------------------------------------------------------
	//
	// FlushAndFinish
	//
	//--------------------------------------------------------------------------------------
	void UploadHeap::FlushAndFinish(bool bDoBarriers)
	{
		// make sure another thread is not already flushing
		flushing.Wait();

		// begins a critical section, and make sure no allocations happen while a thread is inside it
		flushing.Inc();

		// wait for pending allocations to finish
		allocating.Wait();

		std::unique_lock<std::mutex> lock(m_mutex);
		Flush();
		if (m_copies.size()) {
			Trace("flushing %i", m_copies.size());

			//apply pre barriers in one go
			if (m_toPreBarrier.size() > 0)
			{
				vkCmdPipelineBarrier(GetCommandList(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, (uint32_t)m_toPreBarrier.size(), m_toPreBarrier.data());
				m_toPreBarrier.clear();
			}

			for (auto c : m_copies)
			{
				vkCmdCopyBufferToImage(GetCommandList(), GetResource(), c.m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &c.m_bufferImageCopy);
			}
			m_copies.clear();

			//apply post barriers in one go
			if (m_toPostBarrier.size() > 0)
			{
				vkCmdPipelineBarrier(GetCommandList(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, (uint32_t)m_toPostBarrier.size(), m_toPostBarrier.data());
				m_toPostBarrier.clear();
			}
		}

		// Close 
		VkResult res = vkEndCommandBuffer(m_pCommandBuffer);
		assert(res == VK_SUCCESS);

		// Submit
		const VkCommandBuffer cmd_bufs[] = { m_pCommandBuffer };
		VkSubmitInfo submit_info;
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pNext = NULL;
		submit_info.waitSemaphoreCount = 0;
		submit_info.pWaitSemaphores = NULL;
		submit_info.pWaitDstStageMask = NULL;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = cmd_bufs;
		submit_info.signalSemaphoreCount = 0;
		submit_info.pSignalSemaphores = NULL;
		auto st = vkGetFenceStatus(m_pDevice->m_device, m_fence);
		printf("UploadHeap fence: %d\n", st);
		res = vkQueueSubmit(m_pDevice->graphics_queue, 1, &submit_info, m_fence);
		assert(res == VK_SUCCESS);

		// Make sure it's been processed by the GPU

		res = vkWaitForFences(m_pDevice->m_device, 1, &m_fence, VK_TRUE, UINT64_MAX);
		assert(res == VK_SUCCESS);

		vkResetFences(m_pDevice->m_device, 1, &m_fence);

		// Reset so it can be reused
		VkCommandBufferBeginInfo cmd_buf_info = {};
		cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		res = vkBeginCommandBuffer(m_pCommandBuffer, &cmd_buf_info);
		assert(res == VK_SUCCESS);

		m_pDataCur = m_pDataBegin;

		flushing.Dec();
	}


	void ResourceViewHeaps::OnCreate(Device* pDevice, uint32_t cbvDescriptorCount, uint32_t srvDescriptorCount, uint32_t uavDescriptorCount, uint32_t samplerDescriptorCount)
	{
		m_pDevice = pDevice;
		m_allocatedDescriptorCount = 0;

		const VkDescriptorPoolSize type_count[] =
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, cbvDescriptorCount },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, cbvDescriptorCount },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, srvDescriptorCount },
			{ VK_DESCRIPTOR_TYPE_SAMPLER, samplerDescriptorCount },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, uavDescriptorCount },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, uavDescriptorCount },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, uavDescriptorCount },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, samplerDescriptorCount }
		};

		VkDescriptorPoolCreateInfo descriptor_pool = {};
		descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptor_pool.pNext = NULL;
		descriptor_pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		descriptor_pool.maxSets = 50000;
		descriptor_pool.poolSizeCount = _countof(type_count);
		descriptor_pool.pPoolSizes = type_count;

		VkResult res = vkCreateDescriptorPool(pDevice->m_device, &descriptor_pool, NULL, &m_descriptorPool);
		assert(res == VK_SUCCESS);
	}

	void ResourceViewHeaps::OnDestroy()
	{
		vkDestroyDescriptorPool(m_pDevice->m_device, m_descriptorPool, NULL);
	}

	bool ResourceViewHeaps::CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding, VkDescriptorSetLayout* pDescSetLayout)
	{
		// Next take layout bindings and use them to create a descriptor set layout

		VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
		descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_layout.pNext = NULL;
		descriptor_layout.bindingCount = (uint32_t)pDescriptorLayoutBinding->size();
		descriptor_layout.pBindings = pDescriptorLayoutBinding->data();

		VkResult res = vkCreateDescriptorSetLayout(m_pDevice->m_device, &descriptor_layout, NULL, pDescSetLayout);
		assert(res == VK_SUCCESS);
		return (res == VK_SUCCESS);
	}
	bool ResourceViewHeaps::CreateDescriptorSetLayoutAndAllocDescriptorSet(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding, VkDescriptorSetLayout* pDescSetLayout, VkDescriptorSet* pDescriptorSet)
	{
		// Next take layout bindings and use them to create a descriptor set layout

		VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
		descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_layout.pNext = NULL;
		descriptor_layout.bindingCount = (uint32_t)pDescriptorLayoutBinding->size();
		descriptor_layout.pBindings = pDescriptorLayoutBinding->data();

		VkResult res = vkCreateDescriptorSetLayout(m_pDevice->m_device, &descriptor_layout, NULL, pDescSetLayout);
		assert(res == VK_SUCCESS);

		return AllocDescriptor(*pDescSetLayout, pDescriptorSet);
	}

	bool ResourceViewHeaps::AllocDescriptor(VkDescriptorSetLayout descLayout, VkDescriptorSet* pDescriptorSet)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		VkDescriptorSetAllocateInfo alloc_info;
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.pNext = NULL;
		alloc_info.descriptorPool = m_descriptorPool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = &descLayout;
		VkResult res = vkAllocateDescriptorSets(m_pDevice->m_device, &alloc_info, pDescriptorSet);
		assert(res == VK_SUCCESS);
		m_allocatedDescriptorCount++;
		return res == VK_SUCCESS;
	}

	void ResourceViewHeaps::FreeDescriptor(VkDescriptorSet descriptorSet)
	{
		m_allocatedDescriptorCount--;
		vkFreeDescriptorSets(m_pDevice->m_device, m_descriptorPool, 1, &descriptorSet);
	}

	bool ResourceViewHeaps::AllocDescriptor(int size, const VkSampler* pSamplers, VkDescriptorSetLayout* pDescSetLayout, VkDescriptorSet* pDescriptorSet)
	{
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(size);
		for (int i = 0; i < size; i++)
		{
			layoutBindings[i].binding = i;
			layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBindings[i].descriptorCount = 1;
			layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBindings[i].pImmutableSamplers = (pSamplers != NULL) ? &pSamplers[i] : NULL;
		}

		return CreateDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindings, pDescSetLayout, pDescriptorSet);
	}

	bool ResourceViewHeaps::AllocDescriptor(std::vector<uint32_t>& descriptorCounts, const VkSampler* pSamplers, VkDescriptorSetLayout* pDescSetLayout, VkDescriptorSet* pDescriptorSet)
	{
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(descriptorCounts.size());
		for (int i = 0; i < descriptorCounts.size(); i++)
		{
			layoutBindings[i].binding = i;
			layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBindings[i].descriptorCount = descriptorCounts[i];
			layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBindings[i].pImmutableSamplers = (pSamplers != NULL) ? &pSamplers[i] : NULL;
		}

		return CreateDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindings, pDescSetLayout, pDescriptorSet);
	}

	VkResult StaticBufferPool::OnCreate(Device* pDevice, uint32_t totalMemSize, bool bUseVidMem, const char* name)
	{
		VkResult res;
		m_pDevice = pDevice;

		m_totalMemSize = totalMemSize;
		m_memOffset = 0;
		m_pData = NULL;
		m_bUseVidMem = bUseVidMem;

#ifdef USE_VMA
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = m_totalMemSize;
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		if (bUseVidMem)
			bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
		allocInfo.pUserData = (void*)name;
		res = vmaCreateBuffer(pDevice->GetAllocator(), &bufferInfo, &allocInfo, &m_buffer, &m_bufferAlloc, nullptr);
		assert(res == VK_SUCCESS);
		SetResourceName(pDevice->m_device, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, "StaticBufferPool (sys mem)");

		res = vmaMapMemory(pDevice->GetAllocator(), m_bufferAlloc, (void**)&m_pData);
		assert(res == VK_SUCCESS);
#else
		// create the buffer, allocate it in SYSTEM memory, bind it and map it

		VkBufferCreateInfo buf_info = {};
		buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buf_info.pNext = NULL;
		buf_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		if (bUseVidMem)
			buf_info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		buf_info.size = m_totalMemSize;
		buf_info.queueFamilyIndexCount = 0;
		buf_info.pQueueFamilyIndices = NULL;
		buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		buf_info.flags = 0;
		res = vkCreateBuffer(m_pDevice->m_device, &buf_info, NULL, &m_buffer);
		assert(res == VK_SUCCESS);

		// allocate the buffer in system memory

		VkMemoryRequirements mem_reqs;
		vkGetBufferMemoryRequirements(m_pDevice->m_device, m_buffer, &mem_reqs);

		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.pNext = NULL;
		alloc_info.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		alloc_info.allocationSize = mem_reqs.size;

		bool pass = memory_type_from_properties(m_pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&alloc_info.memoryTypeIndex);
		assert(pass && "No mappable, coherent memory");

		res = vkAllocateMemory(m_pDevice->m_device, &alloc_info, NULL, &m_deviceMemory);
		assert(res == VK_SUCCESS);

		// bind buffer

		res = vkBindBufferMemory(m_pDevice->m_device, m_buffer, m_deviceMemory, 0);
		assert(res == VK_SUCCESS);

		// Map it and leave it mapped. This is fine for Win10 and Win7.

		res = vkMapMemory(m_pDevice->m_device, m_deviceMemory, 0, mem_reqs.size, 0, (void**)&m_pData);
		assert(res == VK_SUCCESS);
#endif

		if (m_bUseVidMem)
		{
#ifdef USE_VMA
			VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			bufferInfo.size = m_totalMemSize;
			bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
			allocInfo.pUserData = (void*)name;

			res = vmaCreateBuffer(pDevice->GetAllocator(), &bufferInfo, &allocInfo, &m_bufferVid, &m_bufferAllocVid, nullptr);
			assert(res == VK_SUCCESS);
			SetResourceName(pDevice->m_device, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, "StaticBufferPool (vid mem)");
#else

			// create the buffer, allocate it in VIDEO memory and bind it 

			VkBufferCreateInfo buf_info = {};
			buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buf_info.pNext = NULL;
			buf_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			buf_info.size = m_totalMemSize;
			buf_info.queueFamilyIndexCount = 0;
			buf_info.pQueueFamilyIndices = NULL;
			buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			buf_info.flags = 0;
			res = vkCreateBuffer(m_pDevice->m_device, &buf_info, NULL, &m_bufferVid);
			assert(res == VK_SUCCESS);

			// allocate the buffer in VIDEO memory

			VkMemoryRequirements mem_reqs;
			vkGetBufferMemoryRequirements(m_pDevice->m_device, m_bufferVid, &mem_reqs);

			VkMemoryAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.pNext = NULL;
			alloc_info.memoryTypeIndex = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			alloc_info.allocationSize = mem_reqs.size;

			bool pass = memory_type_from_properties(m_pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &alloc_info.memoryTypeIndex);
			assert(pass && "No mappable, coherent memory");

			res = vkAllocateMemory(m_pDevice->m_device, &alloc_info, NULL, &m_deviceMemoryVid);
			assert(res == VK_SUCCESS);

			// bind buffer

			res = vkBindBufferMemory(m_pDevice->m_device, m_bufferVid, m_deviceMemoryVid, 0);
			assert(res == VK_SUCCESS);
#endif
		}

		return res;
	}

	void StaticBufferPool::OnDestroy()
	{
		if (m_bUseVidMem)
		{
#ifdef USE_VMA
			vmaDestroyBuffer(m_pDevice->GetAllocator(), m_bufferVid, m_bufferAllocVid);
#else
			vkFreeMemory(m_pDevice->m_device, m_deviceMemoryVid, NULL);
			vkDestroyBuffer(m_pDevice->m_device, m_bufferVid, NULL);
#endif
			m_bufferVid = 0; m_bufferAllocVid = 0;
		}

		if (m_buffer != VK_NULL_HANDLE)
		{
#ifdef USE_VMA
			vmaUnmapMemory(m_pDevice->GetAllocator(), m_bufferAlloc);
			vmaDestroyBuffer(m_pDevice->GetAllocator(), m_buffer, m_bufferAlloc);
#else
			vkUnmapMemory(m_pDevice->m_device, m_deviceMemory);
			vkFreeMemory(m_pDevice->m_device, m_deviceMemory, NULL);
			vkDestroyBuffer(m_pDevice->GetDevice(), m_buffer, NULL);
#endif
			m_buffer = VK_NULL_HANDLE;
		}
	}


	bool StaticBufferPool::AllocBuffer(uint32_t numbeOfElements, uint32_t strideInBytes, void** pData, VkDescriptorBufferInfo* pOut)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		uint32_t size = AlignUp(numbeOfElements * strideInBytes, 64u);
		auto d = (int64_t)m_totalMemSize - (m_memOffset + size);
		assert(m_memOffset + size <= m_totalMemSize);

		*pData = (void*)(m_pData + m_memOffset);

		pOut->buffer = m_bUseVidMem ? m_bufferVid : m_buffer;
		pOut->offset = m_memOffset;
		pOut->range = size;

		m_memOffset += size;

		return true;
	}

	bool StaticBufferPool::AllocBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, const void* pInitData, VkDescriptorBufferInfo* pOut)
	{
		void* pData;
		if (AllocBuffer(numbeOfVertices, strideInBytes, &pData, pOut))
		{
			memcpy(pData, pInitData, numbeOfVertices * strideInBytes);
			return true;
		}
		return false;
	}

	void StaticBufferPool::UploadData(VkCommandBuffer cmd_buf)
	{
		VkBufferCopy region;
		region.srcOffset = 0;
		region.dstOffset = 0;
		region.size = m_totalMemSize;

		vkCmdCopyBuffer(cmd_buf, m_buffer, m_bufferVid, 1, &region);
	}

	void StaticBufferPool::FreeUploadHeap()
	{
		if (m_bUseVidMem)
		{
			assert(m_buffer != VK_NULL_HANDLE);
#ifdef USE_VMA
			vmaUnmapMemory(m_pDevice->GetAllocator(), m_bufferAlloc);
			vmaDestroyBuffer(m_pDevice->GetAllocator(), m_buffer, m_bufferAlloc);
#else
			//release upload heap
			vkUnmapMemory(m_pDevice->GetDevice(), m_deviceMemory);
			vkFreeMemory(m_pDevice->GetDevice(), m_deviceMemory, NULL);
			m_deviceMemory = VK_NULL_HANDLE;
			vkDestroyBuffer(m_pDevice->GetDevice(), m_buffer, NULL);

#endif
			m_buffer = VK_NULL_HANDLE;
		}
	}

	void GBufferRenderPass::OnCreate(GBuffer* pGBuffer, GBufferFlags flags, bool bClear, const std::string& name)
	{
		m_flags = flags;
		m_pGBuffer = pGBuffer;
		m_pDevice = pGBuffer->GetDevice();

		m_renderPass = pGBuffer->CreateRenderPass(flags, bClear);
		SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)m_renderPass, name.c_str());
	}

	void GBufferRenderPass::OnDestroy()
	{
		vkDestroyRenderPass(m_pGBuffer->GetDevice()->GetDevice(), m_renderPass, nullptr);
	}

	void GBufferRenderPass::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height)
	{
		std::vector<VkImageView> attachments;
		m_pGBuffer->GetAttachmentList(m_flags, &attachments, &m_clearValues);
		m_frameBuffer = m_pGBuffer->GetDevice()->newFrameBuffer(m_renderPass, &attachments, Width, Height);
	}

	void GBufferRenderPass::OnDestroyWindowSizeDependentResources()
	{
		vkDestroyFramebuffer(m_pGBuffer->GetDevice()->GetDevice(), m_frameBuffer, nullptr);
	}

	void GBufferRenderPass::BeginPass(VkCommandBuffer commandList, VkRect2D renderArea)
	{
		VkRenderPassBeginInfo rp_begin;
		rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rp_begin.pNext = NULL;
		rp_begin.renderPass = m_renderPass;
		rp_begin.framebuffer = m_frameBuffer;
		rp_begin.renderArea = renderArea;
		rp_begin.pClearValues = m_clearValues.data();
		rp_begin.clearValueCount = (uint32_t)m_clearValues.size();
		vkCmdBeginRenderPass(commandList, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

		SetViewportAndScissor(commandList, renderArea.offset.x, renderArea.offset.y, renderArea.extent.width, renderArea.extent.height);
	}

	void GBufferRenderPass::EndPass(VkCommandBuffer commandList)
	{
		vkCmdEndRenderPass(commandList);
	}

	void GBufferRenderPass::GetCompilerDefines(DefineList& defines)
	{
		int rtIndex = 0;

		// GDR (Forward pass)
		//
		if (m_flags & GBUFFER_FORWARD)
		{
			defines["HAS_FORWARD_RT"] = std::to_string(rtIndex++);
		}
		if (m_flags & GBUFFER_OIT_ACCUM)
		{
			defines["HAS_OIT_ACCUM_RT"] = std::to_string(rtIndex++);
		}
		if (m_flags & GBUFFER_OIT_WEIGHT)
		{
			defines["HAS_OIT_WEIGHT_RT"] = std::to_string(rtIndex++);
		}

		// Motion Vectors
		//
		if (m_flags & GBUFFER_MOTION_VECTORS)
		{
			defines["HAS_MOTION_VECTORS"] = std::to_string(1);
			defines["HAS_MOTION_VECTORS_RT"] = std::to_string(rtIndex++);
		}

		// Normal Buffer
		//
		if (m_flags & GBUFFER_NORMAL_BUFFER)
		{
			defines["HAS_NORMALS_RT"] = std::to_string(rtIndex++);
		}

		// Diffuse
		//
		if (m_flags & GBUFFER_DIFFUSE)
		{
			defines["HAS_DIFFUSE_RT"] = std::to_string(rtIndex++);
		}

		// Specular roughness
		//
		if (m_flags & GBUFFER_SPECULAR_ROUGHNESS)
		{
			defines["HAS_SPECULAR_ROUGHNESS_RT"] = std::to_string(rtIndex++);
		}
	}

	VkSampleCountFlagBits GBufferRenderPass::GetSampleCount()
	{
		return m_pGBuffer->GetSampleCount();
	}

	//-------------------------------------------------------------------------------------

	void GBuffer::OnCreate(Device* pDevice, ResourceViewHeaps* pHeaps, const std::map<GBufferFlags, VkFormat>& formats, int sampleCount)
	{
		m_GBufferFlags = GBUFFER_NONE;
		for (auto a : formats)
			m_GBufferFlags = m_GBufferFlags | a.first;

		m_pDevice = pDevice;
		m_sampleCount = (VkSampleCountFlagBits)sampleCount;
		m_formats = formats;
	}

	void GBuffer::OnDestroy()
	{
	}

	//
	// create render pass based on usage flags
	//
	VkRenderPass GBuffer::CreateRenderPass(GBufferFlags flags, bool bClear)
	{
		VkAttachmentDescription depthAttachment;
		VkAttachmentDescription colorAttachments[10];
		uint32_t colorAttanchmentCount = 0;

		auto addAttachment = bClear ? AttachClearBeforeUse : AttachBlending;
		VkImageLayout previousColor = bClear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkImageLayout previousDepth = bClear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		if (flags & GBUFFER_FORWARD)
		{
			addAttachment(m_formats[GBUFFER_FORWARD], m_sampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttanchmentCount++]);
			assert(m_GBufferFlags & GBUFFER_FORWARD); // asserts if there if the RT is not present in the GBuffer
		}
		// GBUFFER_OIT_ACCUM GBUFFER_OIT_WEIGHT
		if (flags & GBUFFER_OIT_ACCUM)
		{
			addAttachment(m_formats[GBUFFER_OIT_ACCUM], m_sampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttanchmentCount++]);
			assert(m_GBufferFlags & GBUFFER_OIT_ACCUM); // asserts if there if the RT is not present in the GBuffer
		}

		if (flags & GBUFFER_OIT_WEIGHT)
		{
			addAttachment(m_formats[GBUFFER_OIT_WEIGHT], m_sampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttanchmentCount++]);
			assert(m_GBufferFlags & GBUFFER_OIT_WEIGHT); // asserts if there if the RT is not present in the GBuffer
		}

		if (flags & GBUFFER_MOTION_VECTORS)
		{
			addAttachment(m_formats[GBUFFER_MOTION_VECTORS], m_sampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttanchmentCount++]);
			assert(m_GBufferFlags & GBUFFER_MOTION_VECTORS); // asserts if there if the RT is not present in the GBuffer
		}

		if (flags & GBUFFER_NORMAL_BUFFER)
		{
			addAttachment(m_formats[GBUFFER_NORMAL_BUFFER], m_sampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttanchmentCount++]);
			assert(m_GBufferFlags & GBUFFER_NORMAL_BUFFER); // asserts if there if the RT is not present in the GBuffer
		}

		if (flags & GBUFFER_DIFFUSE)
		{
			addAttachment(m_formats[GBUFFER_DIFFUSE], m_sampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttanchmentCount++]);
			assert(m_GBufferFlags & GBUFFER_DIFFUSE); // asserts if there if the RT is not present in the GBuffer
		}

		if (flags & GBUFFER_SPECULAR_ROUGHNESS)
		{
			addAttachment(m_formats[GBUFFER_SPECULAR_ROUGHNESS], m_sampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttanchmentCount++]);
			assert(m_GBufferFlags & GBUFFER_SPECULAR_ROUGHNESS); // asserts if there if the RT is not present in the GBuffer
		}

		if (flags & GBUFFER_DEPTH)
		{
			addAttachment(m_formats[GBUFFER_DEPTH], m_sampleCount, previousDepth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, &depthAttachment);
			assert(m_GBufferFlags & GBUFFER_DEPTH); // asserts if there if the RT is not present in the GBuffer
		}

		return CreateRenderPassOptimal(m_pDevice->m_device, colorAttanchmentCount, colorAttachments, &depthAttachment);
	}

	void GBuffer::GetAttachmentList(GBufferFlags flags, std::vector<VkImageView>* pAttachments, std::vector<VkClearValue>* pClearValues)
	{
		pAttachments->clear();
		pClearValues->clear();

		// Create Texture + RTV, to hold the resolved scene
		//
		if (flags & GBUFFER_FORWARD)
		{
			pAttachments->push_back(m_HDRSRV);

			if (pClearValues)
			{
				VkClearValue cv;
				cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
				pClearValues->push_back(cv);
			}
		}
		// GBUFFER_OIT_ACCUM GBUFFER_OIT_WEIGHT
#if 0
		if (flags & GBUFFER_OIT_ACCUM)
		{
			pAttachments->push_back(m_HDR_oit_accumSRV);

			if (pClearValues)
			{
				VkClearValue cv;
				cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
				pClearValues->push_back(cv);
			}
		}
		if (flags & GBUFFER_OIT_WEIGHT)
		{
			pAttachments->push_back(m_HDR_oit_weightSRV);

			if (pClearValues)
			{
				VkClearValue cv;
				cv.color = { 1.0f };
				pClearValues->push_back(cv);
			}
		}
#endif
		// Motion Vectors
		//
		if (flags & GBUFFER_MOTION_VECTORS)
		{
			pAttachments->push_back(m_MotionVectorsSRV);

			if (pClearValues)
			{
				VkClearValue cv;
				cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
				pClearValues->push_back(cv);
			}
		}

		// Normal Buffer
		//
		if (flags & GBUFFER_NORMAL_BUFFER)
		{
			pAttachments->push_back(m_NormalBufferSRV);

			if (pClearValues)
			{
				VkClearValue cv;
				cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
				pClearValues->push_back(cv);
			}
		}

		// Diffuse
		//
		if (flags & GBUFFER_DIFFUSE)
		{
			pAttachments->push_back(m_DiffuseSRV);

			if (pClearValues)
			{
				VkClearValue cv;
				cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
				pClearValues->push_back(cv);
			}
		}

		// Specular Roughness
		//
		if (flags & GBUFFER_SPECULAR_ROUGHNESS)
		{
			pAttachments->push_back(m_SpecularRoughnessSRV);

			if (pClearValues)
			{
				VkClearValue cv;
				cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
				pClearValues->push_back(cv);
			}
		}

		// Create depth buffer
		//
		if (flags & GBUFFER_DEPTH)
		{
			pAttachments->push_back(m_DepthBufferDSV);

			if (pClearValues)
			{
				VkClearValue cv;
				cv.depthStencil = { 1.0f, 0 };
				pClearValues->push_back(cv);
			}
		}
	}

	void GBuffer::OnCreateWindowSizeDependentResources(/*SwapChain* pSwapChain,*/ uint32_t Width, uint32_t Height)
	{
		// Create Texture + RTV, to hold the resolved scene
		//
		if (m_GBufferFlags & GBUFFER_FORWARD)
		{
			m_HDR.InitRenderTarget(m_pDevice, Width, Height, m_formats[GBUFFER_FORWARD], m_sampleCount, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT), false, "m_HDR");
			m_HDR.CreateSRV(&m_HDRSRV);
			m_HDRt.InitRenderTarget(m_pDevice, Width, Height, m_formats[GBUFFER_FORWARD], m_sampleCount, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT), false, "m_HDRt");
			m_HDRt.CreateSRV(&m_HDRSRVt);
		}
#if 0
		if (m_GBufferFlags & GBUFFER_OIT_ACCUM)
		{
			m_HDR_oit_accum.InitRenderTarget(m_pDevice, Width, Height, m_formats[GBUFFER_OIT_ACCUM], m_sampleCount, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_HDR_oit_accum");
			m_HDR_oit_accum.CreateSRV(&m_HDR_oit_accumSRV);
		}
		if (m_GBufferFlags & GBUFFER_OIT_WEIGHT)
		{
			m_HDR_oit_weight.InitRenderTarget(m_pDevice, Width, Height, m_formats[GBUFFER_OIT_WEIGHT], m_sampleCount, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_HDR_oit_weight");
			m_HDR_oit_weight.CreateSRV(&m_HDR_oit_weightSRV);
		}
#endif
		// Motion Vectors
		//
		if (m_GBufferFlags & GBUFFER_MOTION_VECTORS)
		{
			m_MotionVectors.InitRenderTarget(m_pDevice, Width, Height, m_formats[GBUFFER_MOTION_VECTORS], m_sampleCount, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_MotionVector");
			m_MotionVectors.CreateSRV(&m_MotionVectorsSRV);
		}

		// Normal Buffer
		//
		if (m_GBufferFlags & GBUFFER_NORMAL_BUFFER)
		{
			m_NormalBuffer.InitRenderTarget(m_pDevice, Width, Height, m_formats[GBUFFER_NORMAL_BUFFER], m_sampleCount, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_NormalBuffer");
			m_NormalBuffer.CreateSRV(&m_NormalBufferSRV);
		}

		// Diffuse
		//
		if (m_GBufferFlags & GBUFFER_DIFFUSE)
		{
			m_Diffuse.InitRenderTarget(m_pDevice, Width, Height, m_formats[GBUFFER_DIFFUSE], m_sampleCount, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_Diffuse");
			m_Diffuse.CreateSRV(&m_DiffuseSRV);
		}

		// Specular Roughness
		//
		if (m_GBufferFlags & GBUFFER_SPECULAR_ROUGHNESS)
		{
			m_SpecularRoughness.InitRenderTarget(m_pDevice, Width, Height, m_formats[GBUFFER_SPECULAR_ROUGHNESS], m_sampleCount, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_SpecularRoughness");
			m_SpecularRoughness.CreateSRV(&m_SpecularRoughnessSRV);
		}

		// Create depth buffer
		//
		if (m_GBufferFlags & GBUFFER_DEPTH)
		{
			m_DepthBuffer.InitDepthStencil(m_pDevice, Width, Height, m_formats[GBUFFER_DEPTH], m_sampleCount, "DepthBuffer");
			m_DepthBuffer.CreateDSV(&m_DepthBufferDSV);
			m_DepthBuffer.CreateRTV(&m_DepthBufferSRV);
		}
	}

	void GBuffer::OnDestroyWindowSizeDependentResources()
	{
		if (m_GBufferFlags & GBUFFER_SPECULAR_ROUGHNESS)
		{
			vkDestroyImageView(m_pDevice->m_device, m_SpecularRoughnessSRV, nullptr);
			m_SpecularRoughness.OnDestroy();
		}

		if (m_GBufferFlags & GBUFFER_DIFFUSE)
		{
			vkDestroyImageView(m_pDevice->m_device, m_DiffuseSRV, nullptr);
			m_Diffuse.OnDestroy();
		}

		if (m_GBufferFlags & GBUFFER_NORMAL_BUFFER)
		{
			vkDestroyImageView(m_pDevice->m_device, m_NormalBufferSRV, nullptr);
			m_NormalBuffer.OnDestroy();
		}

		if (m_GBufferFlags & GBUFFER_MOTION_VECTORS)
		{
			vkDestroyImageView(m_pDevice->m_device, m_MotionVectorsSRV, nullptr);
			m_MotionVectors.OnDestroy();
		}

		if (m_GBufferFlags & GBUFFER_FORWARD)
		{
			vkDestroyImageView(m_pDevice->m_device, m_HDRSRV, nullptr);
			m_HDR.OnDestroy();
			vkDestroyImageView(m_pDevice->m_device, m_HDRSRVt, nullptr);
			m_HDRt.OnDestroy();
		}
#if 0
		if (m_GBufferFlags & GBUFFER_OIT_ACCUM)
		{
			vkDestroyImageView(m_pDevice->m_device, m_HDR_oit_accumSRV, nullptr);
			m_HDR_oit_accum.OnDestroy();
		}

		if (m_GBufferFlags & GBUFFER_OIT_WEIGHT)
		{
			vkDestroyImageView(m_pDevice->m_device, m_HDR_oit_weightSRV, nullptr);
			m_HDR_oit_weight.OnDestroy();
		}
#endif
		if (m_GBufferFlags & GBUFFER_DEPTH)
		{
			vkDestroyImageView(m_pDevice->m_device, m_DepthBufferDSV, nullptr);
			vkDestroyImageView(m_pDevice->m_device, m_DepthBufferSRV, nullptr);
			m_DepthBuffer.OnDestroy();
		}
	}
	//--------------------------------------------------------------------------------------
	//
	// OnCreate
	//
	//--------------------------------------------------------------------------------------
	VkResult DynamicBufferRing::OnCreate(Device* pDevice, uint32_t numberOfBackBuffers, uint32_t memTotalSize, char* name)
	{
		VkResult res;
		m_pDevice = pDevice;

		m_memTotalSize = AlignUp(memTotalSize, 64u);

		m_mem.OnCreate(numberOfBackBuffers, m_memTotalSize);

#ifdef USE_VMA
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = m_memTotalSize;
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
		allocInfo.pUserData = name;

		res = vmaCreateBuffer(pDevice->GetAllocator(), &bufferInfo, &allocInfo, &m_buffer, &m_bufferAlloc, nullptr);
		assert(res == VK_SUCCESS);
		SetResourceName(pDevice->m_device, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, "DynamicBufferRing");

		res = vmaMapMemory(pDevice->GetAllocator(), m_bufferAlloc, (void**)&m_pData);
		assert(res == VK_SUCCESS);
#else
		// create a buffer that can host uniforms, indices and vertexbuffers
		VkBufferCreateInfo buf_info = {};
		buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buf_info.pNext = NULL;
		buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		buf_info.size = m_memTotalSize;
		buf_info.queueFamilyIndexCount = 0;
		buf_info.pQueueFamilyIndices = NULL;
		buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		buf_info.flags = 0;
		res = vkCreateBuffer(m_pDevice->m_device, &buf_info, NULL, &m_buffer);
		assert(res == VK_SUCCESS);

		VkMemoryRequirements mem_reqs;
		vkGetBufferMemoryRequirements(m_pDevice->m_device, m_buffer, &mem_reqs);

		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.pNext = NULL;
		alloc_info.memoryTypeIndex = 0;
		alloc_info.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		alloc_info.allocationSize = mem_reqs.size;
		alloc_info.memoryTypeIndex = 0;

		bool pass = memory_type_from_properties(m_pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&alloc_info.memoryTypeIndex);
		assert(pass && "No mappable, coherent memory");

		res = vkAllocateMemory(m_pDevice->m_device, &alloc_info, NULL, &m_deviceMemory);
		assert(res == VK_SUCCESS);

		res = vkMapMemory(m_pDevice->m_device, m_deviceMemory, 0, mem_reqs.size, 0, (void**)&m_pData);
		assert(res == VK_SUCCESS);

		res = vkBindBufferMemory(m_pDevice->m_device, m_buffer, m_deviceMemory, 0);
		assert(res == VK_SUCCESS);
#endif
		return res;
	}

	//--------------------------------------------------------------------------------------
	//
	// OnDestroy
	//
	//--------------------------------------------------------------------------------------
	void DynamicBufferRing::OnDestroy()
	{
#ifdef USE_VMA
		vmaUnmapMemory(m_pDevice->GetAllocator(), m_bufferAlloc);
		vmaDestroyBuffer(m_pDevice->GetAllocator(), m_buffer, m_bufferAlloc);
		m_bufferAlloc = 0;
#else
		vkUnmapMemory(m_pDevice->m_device, m_deviceMemory);
		vkFreeMemory(m_pDevice->m_device, m_deviceMemory, NULL);
		vkDestroyBuffer(m_pDevice->m_device, m_buffer, NULL);
		m_deviceMemory = 0;
#endif
		m_buffer = 0;
		m_mem.OnDestroy();
	}

	//--------------------------------------------------------------------------------------
	//
	// AllocConstantBuffer
	//
	//--------------------------------------------------------------------------------------
	bool DynamicBufferRing::AllocConstantBuffer1(uint32_t size, void** pData, VkDescriptorBufferInfo* pOut, uint32_t algin)
	{
		size = AlignUp(size, algin);

		uint32_t memOffset;
		if (m_mem.Alloc(size, &memOffset) == false)
		{
			assert("Ran out of mem for 'dynamic' buffers, please increase the allocated size");
			return false;
		}

		*pData = (void*)(m_pData + memOffset);

		pOut->buffer = m_buffer;
		pOut->offset = memOffset;
		pOut->range = size;

		return true;
	}
	bool DynamicBufferRing::AllocConstantBuffer(uint32_t size, void** pData, VkDescriptorBufferInfo* pOut)
	{
		return AllocConstantBuffer1(size, pData, pOut, 64u);
	}

	bool DynamicBufferRing::AllocBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, const void* pInitData, VkDescriptorBufferInfo* pOut)
	{
		auto db = AllocConstantBuffer(numbeOfVertices * strideInBytes, (void*)pInitData);
		if (pOut)*pOut = db;
		return db.buffer != 0;
	}
	//--------------------------------------------------------------------------------------
	//
	// AllocConstantBuffer
	//
	//--------------------------------------------------------------------------------------
	VkDescriptorBufferInfo DynamicBufferRing::AllocConstantBuffer(uint32_t size, void* pData)
	{
		void* pBuffer;
		VkDescriptorBufferInfo out;
		if (AllocConstantBuffer(size, &pBuffer, &out))
		{
			memcpy(pBuffer, pData, size);
		}

		return out;
	}

	//--------------------------------------------------------------------------------------
	//
	// AllocVertexBuffer
	//
	//--------------------------------------------------------------------------------------
	bool DynamicBufferRing::AllocVertexBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void** pData, VkDescriptorBufferInfo* pOut)
	{
		return AllocConstantBuffer(numbeOfVertices * strideInBytes, pData, pOut);
	}

	bool DynamicBufferRing::AllocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void** pData, VkDescriptorBufferInfo* pOut)
	{
		return AllocConstantBuffer(numbeOfIndices * strideInBytes, pData, pOut);
	}

	//--------------------------------------------------------------------------------------
	//
	// OnBeginFrame
	//
	//--------------------------------------------------------------------------------------
	void DynamicBufferRing::OnBeginFrame()
	{
		m_mem.OnBeginFrame();
	}

	void DynamicBufferRing::SetDescriptorSet(int index, uint32_t size, VkDescriptorSet descriptorSet, uint32_t dt)
	{
		VkDescriptorBufferInfo out = {};
		assert(m_buffer);
		out.buffer = m_buffer;
		out.offset = 0;
		out.range = size;// alignUp(size, (uint32_t)256);

		VkWriteDescriptorSet write;
		write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = NULL;
		write.dstSet = descriptorSet;
		write.descriptorCount = 1;
		int dk = dt;
		write.descriptorType = (VkDescriptorType)(dk > 0 ? dt : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		write.pBufferInfo = &out;
		write.dstArrayElement = 0;
		write.dstBinding = index;

		vkUpdateDescriptorSets(m_pDevice->m_device, 1, &write, 0, NULL);
	}



}

// todo post

namespace vkr {
	// todo post proc src

	// postproc
	//--------------------------------------------------------------------------------------
	//
	// OnCreate
	//
	//--------------------------------------------------------------------------------------    
	void PostProcPS::OnCreate(
		Device* pDevice,
		VkRenderPass renderPass,
		const std::string& shaderFilename,
		const std::string& shaderEntryPoint,
		const std::string& shaderCompilerParams,
		StaticBufferPool* pStaticBufferPool,
		DynamicBufferRing* pDynamicBufferRing,
		VkDescriptorSetLayout descriptorSetLayout,
		VkPipelineColorBlendStateCreateInfo* pBlendDesc,
		VkSampleCountFlagBits sampleDescCount
	)
	{
		m_pDevice = pDevice;

		// Create the vertex shader
		static const char* vertexShader =
			"static const float4 FullScreenVertsPos[3] = { float4(-1, 1, 1, 1), float4(3, 1, 1, 1), float4(-1, -3, 1, 1) };\
            static const float2 FullScreenVertsUVs[3] = { float2(0, 0), float2(2, 0), float2(0, 2) };\
            struct VERTEX_OUT\
            {\
                float2 vTexture : TEXCOORD;\
                float4 vPosition : SV_POSITION;\
            };\
            VERTEX_OUT mainVS(uint vertexId : SV_VertexID)\
            {\
                VERTEX_OUT Output;\
                Output.vPosition = FullScreenVertsPos[vertexId];\
                Output.vTexture = FullScreenVertsUVs[vertexId];\
                return Output;\
            }";

		VkResult res;

		// Compile shaders
		//
		DefineList attributeDefines;

		VkPipelineShaderStageCreateInfo m_vertexShader;
#ifdef _DEBUG
		std::string CompileFlagsVS("-T vs_6_0 -Zi -Od");
#else
		std::string CompileFlagsVS("-T vs_6_0");
#endif // _DEBUG
		//res = VKCompileFromString(m_pDevice->m_device, SST_HLSL, VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "mainVS", CompileFlagsVS.c_str(), &attributeDefines, &m_vertexShader);
		res = VKCompileFromFile(pDevice->m_device, VK_SHADER_STAGE_VERTEX_BIT, "sky.v.glsl", "main", "", &attributeDefines, &m_vertexShader);
		assert(res == VK_SUCCESS);

		m_fragmentShaderName = shaderEntryPoint;
		VkPipelineShaderStageCreateInfo m_fragmentShader;
		res = VKCompileFromFile(m_pDevice->m_device, VK_SHADER_STAGE_FRAGMENT_BIT, shaderFilename.c_str(), m_fragmentShaderName.c_str(), shaderCompilerParams.c_str(), &attributeDefines, &m_fragmentShader);
		assert(res == VK_SUCCESS);

		m_shaderStages.clear();
		m_shaderStages.push_back(m_vertexShader);
		m_shaderStages.push_back(m_fragmentShader);

		// Create pipeline layout
		//
		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
		pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pPipelineLayoutCreateInfo.pNext = NULL;
		pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
		pPipelineLayoutCreateInfo.setLayoutCount = 1;
		pPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

		res = vkCreatePipelineLayout(pDevice->m_device, &pPipelineLayoutCreateInfo, NULL, &m_pipelineLayout);
		assert(res == VK_SUCCESS);

		UpdatePipeline(renderPass, pBlendDesc, sampleDescCount);
	}

	//--------------------------------------------------------------------------------------
	//
	// UpdatePipeline
	//
	//--------------------------------------------------------------------------------------
	void PostProcPS::UpdatePipeline(VkRenderPass renderPass, VkPipelineColorBlendStateCreateInfo* pBlendDesc, VkSampleCountFlagBits sampleDescCount)
	{
		if (renderPass == VK_NULL_HANDLE)
			return;

		if (m_pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_pDevice->m_device, m_pipeline, nullptr);
			m_pipeline = VK_NULL_HANDLE;
		}

		VkResult res;

		// input assembly state and layout

		VkPipelineVertexInputStateCreateInfo vi = {};
		vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vi.pNext = NULL;
		vi.flags = 0;
		vi.vertexBindingDescriptionCount = 0;
		vi.pVertexBindingDescriptions = nullptr;
		vi.vertexAttributeDescriptionCount = 0;
		vi.pVertexAttributeDescriptions = nullptr;

		VkPipelineInputAssemblyStateCreateInfo ia;
		ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		ia.pNext = NULL;
		ia.flags = 0;
		ia.primitiveRestartEnable = VK_FALSE;
		ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// rasterizer state

		VkPipelineRasterizationStateCreateInfo rs;
		rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rs.pNext = NULL;
		rs.flags = 0;
		rs.polygonMode = VK_POLYGON_MODE_FILL;
		rs.cullMode = VK_CULL_MODE_NONE;
		rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rs.depthClampEnable = VK_FALSE;
		rs.rasterizerDiscardEnable = VK_FALSE;
		rs.depthBiasEnable = VK_FALSE;
		rs.depthBiasConstantFactor = 0;
		rs.depthBiasClamp = 0;
		rs.depthBiasSlopeFactor = 0;
		rs.lineWidth = 1.0f;

		VkPipelineColorBlendAttachmentState att_state[1];
		att_state[0].colorWriteMask = allBits;
		att_state[0].blendEnable = VK_FALSE;
		att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
		att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
		att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

		// Color blend state

		VkPipelineColorBlendStateCreateInfo cb;
		cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cb.flags = 0;
		cb.pNext = NULL;
		cb.attachmentCount = 1;
		cb.pAttachments = att_state;
		cb.logicOpEnable = VK_FALSE;
		cb.logicOp = VK_LOGIC_OP_NO_OP;
		cb.blendConstants[0] = 1.0f;
		cb.blendConstants[1] = 1.0f;
		cb.blendConstants[2] = 1.0f;
		cb.blendConstants[3] = 1.0f;

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_BLEND_CONSTANTS
		};
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pNext = NULL;
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();

		// view port state

		VkPipelineViewportStateCreateInfo vp = {};
		vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vp.pNext = NULL;
		vp.flags = 0;
		vp.viewportCount = 1;
		vp.scissorCount = 1;
		vp.pScissors = NULL;
		vp.pViewports = NULL;

		// depth stencil state

		VkPipelineDepthStencilStateCreateInfo ds;
		ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		ds.pNext = NULL;
		ds.flags = 0;
		ds.depthTestEnable = VK_TRUE;
		ds.depthWriteEnable = VK_FALSE;
		ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		ds.depthBoundsTestEnable = VK_FALSE;
		ds.stencilTestEnable = VK_FALSE;
		ds.back.failOp = VK_STENCIL_OP_KEEP;
		ds.back.passOp = VK_STENCIL_OP_KEEP;
		ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
		ds.back.compareMask = 0;
		ds.back.reference = 0;
		ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
		ds.back.writeMask = 0;
		ds.minDepthBounds = 0;
		ds.maxDepthBounds = 0;
		ds.stencilTestEnable = VK_FALSE;
		ds.front = ds.back;

		// multi sample state

		VkPipelineMultisampleStateCreateInfo ms;
		ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms.pNext = NULL;
		ms.flags = 0;
		ms.pSampleMask = NULL;
		ms.rasterizationSamples = sampleDescCount;
		ms.sampleShadingEnable = VK_FALSE;
		ms.alphaToCoverageEnable = VK_FALSE;
		ms.alphaToOneEnable = VK_FALSE;
		ms.minSampleShading = 0.0;

		// create pipeline 

		VkGraphicsPipelineCreateInfo pipeline = {};
		pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline.pNext = NULL;
		pipeline.layout = m_pipelineLayout;
		pipeline.basePipelineHandle = VK_NULL_HANDLE;
		pipeline.basePipelineIndex = 0;
		pipeline.flags = 0;
		pipeline.pVertexInputState = &vi;
		pipeline.pInputAssemblyState = &ia;
		pipeline.pRasterizationState = &rs;
		pipeline.pColorBlendState = (pBlendDesc == NULL) ? &cb : pBlendDesc;
		pipeline.pTessellationState = NULL;
		pipeline.pMultisampleState = &ms;
		pipeline.pDynamicState = &dynamicState;
		pipeline.pViewportState = &vp;
		pipeline.pDepthStencilState = &ds;
		pipeline.pStages = m_shaderStages.data();
		pipeline.stageCount = (uint32_t)m_shaderStages.size();
		pipeline.renderPass = renderPass;
		pipeline.subpass = 0;

		res = vkCreateGraphicsPipelines(m_pDevice->m_device, m_pDevice->GetPipelineCache(), 1, &pipeline, NULL, &m_pipeline);
		assert(res == VK_SUCCESS);
		SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_pipeline, "cacaca P");
	}

	//--------------------------------------------------------------------------------------
	//
	// OnDestroy
	//
	//--------------------------------------------------------------------------------------    
	void PostProcPS::OnDestroy()
	{
		if (m_pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_pDevice->m_device, m_pipeline, nullptr);
			m_pipeline = VK_NULL_HANDLE;
		}

		if (m_pipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(m_pDevice->m_device, m_pipelineLayout, nullptr);
			m_pipelineLayout = VK_NULL_HANDLE;
		}
	}

	//--------------------------------------------------------------------------------------
	//
	// OnDraw
	//
	//--------------------------------------------------------------------------------------    
	void PostProcPS::Draw(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo* pConstantBuffer, VkDescriptorSet descriptorSet)
	{
		if (m_pipeline == VK_NULL_HANDLE)
			return;

		// Bind Descriptor sets
		//                
		int numUniformOffsets = 0;
		uint32_t uniformOffset = 0;
		if (pConstantBuffer != NULL && pConstantBuffer->buffer != NULL)
		{
			numUniformOffsets = 1;
			uniformOffset = (uint32_t)pConstantBuffer->offset;
		}

		VkDescriptorSet descritorSets[1] = { descriptorSet };
		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, descritorSets, numUniformOffsets, &uniformOffset);

		// Bind Pipeline
		//
		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		// Draw
		//
		vkCmdDraw(cmd_buf, 3, 1, 0, 0);
	}

	void PostProcCS::OnCreate(
		Device* pDevice,
		const std::string& shaderFilename,
		const std::string& shaderEntryPoint,
		const std::string& shaderCompilerParams,
		VkDescriptorSetLayout descriptorSetLayout,
		uint32_t dwWidth, uint32_t dwHeight, uint32_t dwDepth,
		DefineList* pUserDefines
	)
	{
		m_pDevice = pDevice;
		VkResult res = {};
		VkPipelineShaderStageCreateInfo computeShader = {};
		DefineList defines = {};
		if (pUserDefines)
			defines = *pUserDefines;
		defines["WIDTH"] = std::to_string(dwWidth);
		defines["HEIGHT"] = std::to_string(dwHeight);
		defines["DEPTH"] = std::to_string(dwDepth);
		res = VKCompileFromFile(m_pDevice->m_device, VK_SHADER_STAGE_COMPUTE_BIT, shaderFilename.c_str(), shaderEntryPoint.c_str(), shaderCompilerParams.c_str(), &defines, &computeShader);
		assert(res == VK_SUCCESS);
		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
		pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pPipelineLayoutCreateInfo.pNext = NULL;
		pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
		pPipelineLayoutCreateInfo.setLayoutCount = 1;
		pPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
		res = vkCreatePipelineLayout(pDevice->m_device, &pPipelineLayoutCreateInfo, NULL, &m_pipelineLayout);
		assert(res == VK_SUCCESS);
		VkComputePipelineCreateInfo pipeline = {};
		pipeline.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeline.pNext = NULL;
		pipeline.flags = 0;
		pipeline.layout = m_pipelineLayout;
		pipeline.stage = computeShader;
		pipeline.basePipelineHandle = VK_NULL_HANDLE;
		pipeline.basePipelineIndex = 0;
		res = vkCreateComputePipelines(pDevice->m_device, pDevice->GetPipelineCache(), 1, &pipeline, NULL, &m_pipeline);
		assert(res == VK_SUCCESS);
	}
	void PostProcCS::OnDestroy()
	{
		vkDestroyPipeline(m_pDevice->m_device, m_pipeline, nullptr);
		vkDestroyPipelineLayout(m_pDevice->m_device, m_pipelineLayout, nullptr);
	}
	void PostProcCS::Draw(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo* pConstantBuffer, VkDescriptorSet descSet, uint32_t dispatchX, uint32_t dispatchY, uint32_t dispatchZ)
	{
		if (m_pipeline == VK_NULL_HANDLE)
			return;
		int numUniformOffsets = 0;
		uint32_t uniformOffset = 0;
		if (pConstantBuffer != NULL && pConstantBuffer->buffer != NULL)
		{
			numUniformOffsets = 1;
			uniformOffset = (uint32_t)pConstantBuffer->offset;
		}
		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &descSet, numUniformOffsets, &uniformOffset);
		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
		vkCmdDispatch(cmd_buf, dispatchX, dispatchY, dispatchZ);
	}

	// todo camera
	Camera::Camera()
	{
		m_View = {};// glm::mat4::identity();
		m_eyePos = glm::vec4(0, 0, 0, 0);
		atPos = glm::vec4(0, 0, 0, 0);
		m_distance = -1;
		glm::quat qx = glm::angleAxis(0.0f, glm::vec3(1, 0, 0));
		glm::quat qy = glm::angleAxis(0.0f, glm::vec3(0, 1, 0));
		mqt = qx * qy;
		pca.qt = mqt;
		pca.camera_direction = glm::vec3(0.0, 0.0, 1.0);
		pca.camera_look_at = glm::vec3(0.0, 0.0, 0.0);
	}

	//--------------------------------------------------------------------------------------
	//
	// OnCreate 
	//
	//--------------------------------------------------------------------------------------
	void Camera::SetFov(float fovV, uint32_t width, uint32_t height, float nearPlane, float farPlane)
	{
		SetFov(fovV, width * 1.f / height, nearPlane, farPlane);
	}

	void Camera::SetFov(float fovV, float aspectRatio, float nearPlane, float farPlane)
	{
		m_aspectRatio = aspectRatio;

		m_near = nearPlane;
		m_far = farPlane;

		m_fovV = fovV;
		m_fovH = std::min<float>(m_fovV * aspectRatio, XM_PI / 2.0f);
		m_fovV = m_fovH / aspectRatio;

		m_Proj = glm::perspective(fovV, m_aspectRatio, nearPlane, farPlane);
	}

	void Camera::SetMatrix(const glm::mat4& cameraMatrix)
	{
		//m_eyePos = cameraMatrix.getCol3();
		m_eyePos = cameraMatrix[3];
		LookAt(m_eyePos, m_eyePos + cameraMatrix * glm::vec4(0, 0, 1, 0));
	}

	void Camera::set_mat(const glm::mat4& cameraMatrix, const glm::vec3& e)
	{
		m_eyePos = glm::vec4(e, 1.0);
		m_View = cameraMatrix;
	}

	//--------------------------------------------------------------------------------------
	//
	// LookAt, use this functions before calling update functions
	//
	//--------------------------------------------------------------------------------------
	//void Camera::LookAt(const glm::vec4& eyePos, const glm::vec4& lookAt)
	//{
	//    m_eyePos = eyePos;
	//    m_View = LookAtRH(eyePos, lookAt);
	//    m_distance = math::SSE::length(lookAt - eyePos);
	//
	//	glm::vec4 zBasis = m_View.getRow(2);
	//	m_yaw = atan2f(zBasis.getX(), zBasis.getZ());
	//	float fLen = sqrtf(zBasis.getZ() * zBasis.getZ() + zBasis.getX() * zBasis.getX());
	//	m_pitch = atan2f(zBasis.getY(), fLen);
	//}
	glm::vec4 get_row(const glm::mat4& m, int n)
	{
		assert(!(n < 0 || n>3));
		glm::vec4 r = { m[0][n], m[1][n], m[2][n], m[3][n] };
		return r;
	}
	void Camera::LookAt(const glm::vec4& eyePos, const glm::vec4& lookAt)
	{
		atPos = lookAt;
		m_eyePos = eyePos;
		m_View = LookAtRH(eyePos, lookAt, flipY);
		m_distance = glm::length(lookAt - eyePos);

		glm::vec4 zBasis = get_row(m_View, 2);
		m_yaw = atan2f(zBasis.x, zBasis.z);
		float fLen = sqrtf(zBasis.z * zBasis.z + zBasis.x * zBasis.x);
		m_pitch = atan2f(zBasis.y, fLen);
	}
	void Camera::LookAt(const glm::vec3& eyePos, const glm::vec3& lookAt)
	{
		LookAt(glm::vec4(eyePos, 1.0), glm::vec4(lookAt, 1.0));
	}
	void Camera::LookAt(float yaw, float pitch, float distance, const glm::vec4& at)
	{
		LookAt(at + PolarToVector(yaw, pitch) * distance, at);
	}

	//--------------------------------------------------------------------------------------
	//
	// UpdateCamera
	//
	//--------------------------------------------------------------------------------------
	void Camera::UpdateCameraWASD(float yaw, float pitch, const bool keyDown[256], double deltaTime)
	{
		// m_eyePos += math::transpose(m_View) * (MoveWASD(keyDown) * m_speed * (float)deltaTime);
		m_eyePos += glm::transpose(m_View) * (MoveWASD(keyDown) * m_speed * (float)deltaTime);
		glm::vec4 dir = PolarToVector(yaw, pitch) * m_distance;
		LookAt(GetPosition(), GetPosition() - dir);
	}
	glm::vec4 p2v(float yaw, float pitch)
	{
		yaw = glm::radians(yaw);
		pitch = glm::radians(pitch);
		return glm::vec4(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch), 0);
	}
	void rotateCameraAroundPoint(glm::vec3& cameraPos, glm::vec3 focusPoint, glm::vec3 upVector, float angle) {
		// 计算相机到焦点的向量
		glm::vec3 toFocus = focusPoint - cameraPos;

		// 计算旋转轴，它是相机上方向和到焦点的垂直向量的叉乘结果
		glm::vec3 axis = glm::normalize(glm::cross(upVector, toFocus));

		// 创建一个四元数，表示绕给定轴的旋转
		glm::quat rotationQuat = glm::angleAxis(angle, axis);

		// 旋转相机位置
		cameraPos = glm::vec3(glm::rotate(rotationQuat, glm::vec3(toFocus)) + focusPoint);

		// 旋转相机的上方向（如果需要）
		// upVector = glm::vec3(glm::rotate(rotationQuat, glm::vec3(upVector)));
	}

	float angle_normalized_v3v3(const glm::vec3& v1, const glm::vec3& v2)
	{
		/* double check they are normalized */
		/* this is the same as acos(dot_v3v3(v1, v2)), but more accurate */
		if (glm::dot(v1, v2) >= 0.0f) {
			return 2.0f * glm::asin(glm::length(v2 - v1) / 2.0f);
		}
		glm::vec3 v2_n = -v2;
		return (float)XM_PI - 2.0f * glm::asin(glm::length(v2_n - v1) / 2.0f);
	}
	glm::mat4 update_yp(glm::ivec3 r, float radius, glm::vec3& position, glm::vec3& target, bool flipY)
	{
		float yaw = r.x;
		float pitch = r.y;

		glm::vec3 direction;
		direction.x = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
		direction.y = sin(glm::radians(pitch));
		direction.z = cos(glm::radians(pitch)) * cos(glm::radians(yaw));

		auto pos = target + direction * radius;
		position = pos;
		auto up = glm::vec3(0, flipY ? -1 : 1, 0);
		return glm::lookAt(position, target, up);
	}

	float modv(float x, float y) {
		if (x < -y) {
			x = fmod(x, -y);
		}
		if (x > y) {
			x = fmod(x, y);
		}
		return x;
	}
	void Camera::UpdateCameraPolar(float yaw, float pitch, float x, float y, float distance)
	{
		if (is_eulerAngles)
		{
			ryp.x += -yaw;
			ryp.y += pitch;
			ryp.x = modv(ryp.x, 360);
			//ryp.y = modv(ryp.y, 90);
			ryp.y = glm::clamp(ryp.y, -90.0f + 1e-3f, 90.0f - 1e-3f);
			yaw = glm::radians(ryp.x);
			pitch = glm::radians(ryp.y);
			m_eyePos += GetSide() * x * distance / 1000.0f;
			m_eyePos += GetUp() * y * distance / 1000.0f;
			glm::vec4 dir = GetDirection();
			glm::vec4 pol = PolarToVector(yaw, pitch);		// 欧拉角计算向量
			glm::vec4 at = m_eyePos - (dir * m_distance);	// 计算目标点
			auto eye = at + (pol * distance);	// 计算相机坐标
			LookAt(eye, at);
			m_eyePos = eye;
			m_distance = glm::length(at - m_eyePos);
		}
		else
		{
			yaw = glm::radians(yaw);
			pitch = glm::radians(pitch);
			glm::quat qx = glm::angleAxis(pitch, glm::vec3(1, 0, 0)) * mqt;
			glm::quat qy = glm::angleAxis(yaw, glm::vec3(0, 1, 0)) * qx;
			mqt = glm::normalize(qy);
			glm::mat4 rotate = glm::mat4_cast(mqt);
			atPos.x += x * distance / 1000.0f;
			atPos.y += y * distance / 1000.0f;
			glm::vec4 dir = GetDirection();
			glm::vec4 at = atPos;	// 观察目标坐标
			auto eye = at + glm::vec4(0, 0, distance, 1.0);
			m_eyePos = glm::inverse(glm::mat4_cast(mqt)) * (eye); // *相机坐标

			glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(-eye));
			m_View = translate * rotate;
			m_distance = distance;	// 新距离 
		}
	}
#if 0
	void UpdateView(float yaw, float pitch, float roll)
	{
		//FPS camera:  RotationX(pitch) * RotationY(yaw)
		glm::quat qPitch = glm::angleAxis(pitch, glm::vec3(1, 0, 0));
		glm::quat qYaw = glm::angleAxis(yaw, glm::vec3(0, 1, 0));
		glm::quat qRoll = glm::angleAxis(roll, glm::vec3(0, 0, 1));

		//For a FPS camera we can omit roll
		glm::quat orientation = qPitch * qYaw;
		orientation = glm::normalize(orientation);
		glm::mat4 rotate = glm::mat4_cast(orientation);

		glm::mat4 translate = glm::mat4(1.0f);
		translate = glm::translate(translate, -eye);

		viewMatrix = rotate * translate;
	}
	void RotatePitch(float rads) // rotate around cams local X axis
	{
		glm::quat qPitch = glm::angleAxis(rads, glm::vec3(1, 0, 0));

		m_orientation = glm::normalize(qPitch) * m_orientation;
		glm::mat4 rotate = glm::mat4_cast(m_orientation);

		glm::mat4 translate = glm::mat4(1.0f);
		translate = glm::translate(translate, -eye);

		m_viewMatrix = rotate * translate;
	}
	void Update(float deltaTimeSeconds)
	{
		//FPS camera:  RotationX(pitch) * RotationY(yaw)
		glm::quat qPitch = glm::angleAxis(m_d_pitch, glm::vec3(1, 0, 0));
		glm::quat qYaw = glm::angleAxis(m_d_yaw, glm::vec3(0, 1, 0));
		glm::quat qRoll = glm::angleAxis(m_d_roll, glm::vec3(0, 0, 1));

		//For a FPS camera we can omit roll
		glm::quat m_d_orientation = qPitch * qYaw;
		glm::quat delta = glm::mix(glm::quat(0, 0, 0, 0), m_d_orientation, deltaTimeSeconds);
		m_orientation = glm::normalize(delta) * m_orientation;
		glm::mat4 rotate = glm::mat4_cast(orientation);

		glm::mat4 translate = glm::mat4(1.0f);
		translate = glm::translate(translate, -eye);

		viewMatrix = rotate * translate;
	}
#endif
	//--------------------------------------------------------------------------------------
	//
	// SetProjectionJitter
	//
	//--------------------------------------------------------------------------------------
	void Camera::SetProjectionJitter(float jitterX, float jitterY)
	{
		//glm::vec4 proj = m_Proj.getCol2();
		//proj.setX(jitterX);
		//proj.setY(jitterY);
		//m_Proj.setCol2(proj);
		glm::vec4 proj = m_Proj[2];
		proj.x = (jitterX);
		proj.y = (jitterY);
		m_Proj[2] = (proj);
	}

	void Camera::SetProjectionJitter(uint32_t width, uint32_t height, uint32_t& sampleIndex)
	{
		static const auto CalculateHaltonNumber = [](uint32_t index, uint32_t base)
			{
				float f = 1.0f, result = 0.0f;

				for (uint32_t i = index; i > 0;)
				{
					f /= static_cast<float>(base);
					result = result + f * static_cast<float>(i % base);
					i = static_cast<uint32_t>(floorf(static_cast<float>(i) / static_cast<float>(base)));
				}

				return result;
			};

		sampleIndex = (sampleIndex + 1) % 16;   // 16x TAA

		float jitterX = 2.0f * CalculateHaltonNumber(sampleIndex + 1, 2) - 1.0f;
		float jitterY = 2.0f * CalculateHaltonNumber(sampleIndex + 1, 3) - 1.0f;

		jitterX /= static_cast<float>(width);
		jitterY /= static_cast<float>(height);

		SetProjectionJitter(jitterX, jitterY);
	}

	//--------------------------------------------------------------------------------------
	//
	// Get a vector pointing in the direction of yaw and pitch
	//
	//--------------------------------------------------------------------------------------
	glm::vec4 PolarToVector(float yaw, float pitch)
	{
		return glm::vec4(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch), 0);
	}
	glm::mat4 LookAtRH(const glm::vec4& eyePos, const glm::vec4& lookAt, bool flipY)
	{
		return glm::lookAt(glm::vec3(eyePos), glm::vec3(lookAt), glm::vec3(0, flipY ? -1 : 1, 0));
	}
	glm::mat4 LookAtRH(const glm::vec3& eyePos, const glm::vec3& lookAt, bool flipY)
	{
		return glm::lookAt(glm::vec3(eyePos), glm::vec3(lookAt), glm::vec3(0, flipY ? -1 : 1, 0));
	}
	glm::mat4 lookatlh(const glm::vec3& eyePos, const glm::vec3& lookAt, bool flipY)
	{
		return glm::lookAtLH((eyePos), (lookAt), glm::vec3(0, flipY ? -1 : 1, 0));
	}
	//glm::mat4 LookAtRH(const glm::vec4& eyePos, const glm::vec4& lookAt)
	//{
	//    return MAT40::lookAt(math::toPoint3(eyePos), math::toPoint3(lookAt), math::Vector3(0, 1, 0));
	//}

	glm::vec4 MoveWASD(const bool keyDown[256])
	{
		float scale = keyDown[VK_SHIFT] ? 5.0f : 1.0f;
		float x = 0, y = 0, z = 0;

		if (keyDown['W'])
		{
			z = -scale;
		}
		if (keyDown['S'])
		{
			z = scale;
		}
		if (keyDown['A'])
		{
			x = -scale;
		}
		if (keyDown['D'])
		{
			x = scale;
		}
		if (keyDown['E'])
		{
			y = scale;
		}
		if (keyDown['Q'])
		{
			y = -scale;
		}

		return glm::vec4(x, y, z, 0.0f);
	}
	// todo sky

	void SkyDomeProc::OnCreate(Device* pDevice, VkRenderPass renderPass, UploadHeap* pUploadHeap, VkFormat outFormat, ResourceViewHeaps* pResourceViewHeaps, DynamicBufferRing* pDynamicBufferRing, StaticBufferPool* pStaticBufferPool, VkSampleCountFlagBits sampleDescCount)
	{
		m_pDevice = pDevice;
		m_pDynamicBufferRing = pDynamicBufferRing;
		m_pResourceViewHeaps = pResourceViewHeaps;

		// Create Descriptor Set Layout, all we need is a uniform dynamic buffer to pass some parameters to the shader. All is procedural and we need no textures.
		//
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(1);
		layoutBindings[0].binding = 0;
		layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		layoutBindings[0].descriptorCount = 1;
		layoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBindings[0].pImmutableSamplers = NULL;

		m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindings, &m_descriptorLayout, &m_descriptorSet);
		pDynamicBufferRing->SetDescriptorSet(0, sizeof(SkyDomeProc::Constants), m_descriptorSet);

		m_skydome.OnCreate(pDevice, renderPass, "SkyDomeProc.hlsl", "main", "-T ps_6_0", pStaticBufferPool, pDynamicBufferRing, m_descriptorLayout, NULL, sampleDescCount);
	}

	void SkyDomeProc::OnDestroy()
	{
		m_skydome.OnDestroy();

		m_pResourceViewHeaps->FreeDescriptor(m_descriptorSet);

		vkDestroyDescriptorSetLayout(m_pDevice->m_device, m_descriptorLayout, NULL);
	}

	void SkyDomeProc::Draw(VkCommandBuffer cmd_buf, SkyDomeProc::Constants constants)
	{
		SetPerfMarkerBegin(cmd_buf, "Skydome Proc");

		SkyDomeProc::Constants* cbPerDraw;
		VkDescriptorBufferInfo constantBuffer;
		m_pDynamicBufferRing->AllocConstantBuffer(sizeof(SkyDomeProc::Constants), (void**)&cbPerDraw, &constantBuffer);
		*cbPerDraw = constants;

		m_skydome.Draw(cmd_buf, &constantBuffer, m_descriptorSet);

		SetPerfMarkerEnd(cmd_buf);
	}

	void SkyDome::OnCreate(Device* pDevice, VkRenderPass renderPass, UploadHeap* pUploadHeap, VkFormat outFormat, ResourceViewHeaps* pResourceViewHeaps, DynamicBufferRing* pDynamicBufferRing, StaticBufferPool* pStaticBufferPool, const char* pDiffuseCubemap, const char* pSpecularCubemap, VkSampleCountFlagBits sampleDescCount)
	{
		m_pDevice = pDevice;
		m_pDynamicBufferRing = pDynamicBufferRing;
		m_pResourceViewHeaps = pResourceViewHeaps;

		m_CubeDiffuseTexture.InitFromFile(pDevice, pUploadHeap, pDiffuseCubemap, true); // SRGB
		m_CubeSpecularTexture.InitFromFile(pDevice, pUploadHeap, pSpecularCubemap, true);
		if (!m_CubeSpecularTexture.Resource())
		{
			int cs = 16;
			IMG_INFO header = {};
			std::vector<glm::vec4> px;
			px.resize(cs * cs * 6);
			unsigned char* buffer = (unsigned char*)px.data();
			VkDeviceSize   bufferSize = px.size() * sizeof(px[0]);
			header.width = cs;
			header.height = cs;
			header.depth = 1;
			header.arraySize = 6;
			header.mipMapCount = 1;
			header.vkformat = VK_FORMAT_R32G32B32A32_SFLOAT;
			header.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			header.bitCount = 32 * 4;
			for (auto& c : px) { c = default_specular; }
			m_CubeSpecularTexture.InitFromData(pDevice, pUploadHeap, &header, buffer, bufferSize, "cubeSpecular", false);
		}
		pUploadHeap->FlushAndFinish();

		m_CubeDiffuseTexture.CreateCubeSRV(&m_CubeDiffuseTextureView);
		m_CubeSpecularTexture.CreateCubeSRV(&m_CubeSpecularTextureView);

		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_NEAREST;
			info.minFilter = VK_FILTER_NEAREST;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.minLod = -1000;
			info.maxLod = 1000;
			info.maxAnisotropy = 1.0f;
			m_samplerDiffuseCube = pDevice->newSampler(&info);
			/*		VkResult res = vkCreateSampler(pDevice->m_device, &info, NULL, &m_samplerDiffuseCube);
					assert(res == VK_SUCCESS);*/
		}

		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.minLod = -1000;
			info.maxLod = 1000;
			info.maxAnisotropy = 1.0f;
			m_samplerSpecularCube = pDevice->newSampler(&info);
			//VkResult res = vkCreateSampler(pDevice->m_device, &info, NULL, &m_samplerSpecularCube);
			//assert(res == VK_SUCCESS);
		}

		//create descriptor
		//
		std::vector<VkDescriptorSetLayoutBinding> layout_bindings(2);
		layout_bindings[0].binding = 0;
		layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		layout_bindings[0].descriptorCount = 1;
		layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layout_bindings[0].pImmutableSamplers = NULL;

		layout_bindings[1].binding = 1;
		layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layout_bindings[1].descriptorCount = 1;
		layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layout_bindings[1].pImmutableSamplers = NULL;

		m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layout_bindings, &m_descriptorLayout, &m_descriptorSet);
		pDynamicBufferRing->SetDescriptorSet(0, sizeof(glm::mat4), m_descriptorSet);
		SetDescriptorSpec(1, m_descriptorSet);

		m_skydome.OnCreate(pDevice, renderPass, "SkyDome.glsl", "main", "", pStaticBufferPool, pDynamicBufferRing, m_descriptorLayout, NULL, sampleDescCount);
	}

	void SkyDome::OnDestroy()
	{
		m_skydome.OnDestroy();
		vkDestroyDescriptorSetLayout(m_pDevice->m_device, m_descriptorLayout, NULL);

		//vkDestroySampler(m_pDevice->m_device, m_samplerDiffuseCube, nullptr);
		//vkDestroySampler(m_pDevice->m_device, m_samplerSpecularCube, nullptr);

		vkDestroyImageView(m_pDevice->m_device, m_CubeDiffuseTextureView, NULL);
		vkDestroyImageView(m_pDevice->m_device, m_CubeSpecularTextureView, NULL);

		m_pResourceViewHeaps->FreeDescriptor(m_descriptorSet);

		m_CubeDiffuseTexture.OnDestroy();
		m_CubeSpecularTexture.OnDestroy();
	}

	void SkyDome::SetDescriptorDiff(uint32_t index, VkDescriptorSet descriptorSet)
	{
		m_pDevice->SetDescriptorSet(index, m_CubeDiffuseTextureView, m_samplerDiffuseCube, descriptorSet);
	}

	void SkyDome::SetDescriptorSpec(uint32_t index, VkDescriptorSet descriptorSet)
	{
		m_pDevice->SetDescriptorSet(index, m_CubeSpecularTextureView, m_samplerSpecularCube, descriptorSet);
	}

	void SkyDome::Draw(VkCommandBuffer cmd_buf, const glm::mat4& invViewProj)
	{
		SetPerfMarkerBegin(cmd_buf, "Skydome cube");

		glm::mat4* cbPerDraw;
		VkDescriptorBufferInfo constantBuffer;
		m_pDynamicBufferRing->AllocConstantBuffer(sizeof(glm::mat4), (void**)&cbPerDraw, &constantBuffer);
		*cbPerDraw = invViewProj;

		m_skydome.Draw(cmd_buf, &constantBuffer, m_descriptorSet);

		SetPerfMarkerEnd(cmd_buf);
	}


	void SkyDome::GenerateDiffuseMapFromEnvironmentMap()
	{

	}

	// Sampler and TextureView getters
	//

	VkImageView SkyDome::GetCubeDiffuseTextureView() const
	{
		return m_CubeDiffuseTextureView;
	}

	VkImageView SkyDome::GetCubeSpecularTextureView() const
	{
		return m_CubeSpecularTextureView;
	}

	VkSampler SkyDome::GetCubeDiffuseTextureSampler() const
	{
		return m_samplerDiffuseCube;
	}

	VkSampler SkyDome::GetCubeSpecularTextureSampler() const
	{
		return m_samplerSpecularCube;
	}

	// todo taa src
	void TAA::OnCreate(Device* pDevice, ResourceViewHeaps* pResourceViewHeaps, StaticBufferPool* pStaticBufferPool, DynamicBufferRing* pDynamicBufferRing, bool sharpening)
	{
		m_bSharpening = sharpening;
		m_pDevice = pDevice;
		m_pResourceViewHeaps = pResourceViewHeaps;
		VkResult res;

		// TAA
		//
		{
			// Create samplers
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_NEAREST;
			info.minFilter = VK_FILTER_NEAREST;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.minLod = -1000;
			info.maxLod = 1000;
			info.maxAnisotropy = 1.0f;

			m_samplers[0] = m_samplers[1] = m_samplers[3] = pDevice->newSampler(&info);
			//res = vkCreateSampler(pDevice->m_device, &info, NULL, &m_samplers[0]);
			//assert(res == VK_SUCCESS);

			//res = vkCreateSampler(pDevice->m_device, &info, NULL, &m_samplers[1]);
			//assert(res == VK_SUCCESS);

			//res = vkCreateSampler(pDevice->m_device, &info, NULL, &m_samplers[3]);
			//(res == VK_SUCCESS);

			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			m_samplers[2] = pDevice->newSampler(&info);
			/*		res = vkCreateSampler(pDevice->m_device, &info, NULL, &m_samplers[2]);
					assert(res == VK_SUCCESS);*/

					// Create VkDescriptor Set Layout Bindings
					//

			std::vector<VkDescriptorSetLayoutBinding> m_TAAInputs(5 + 4);
			for (int i = 0; i < 4; i++)
			{
				m_TAAInputs[i].binding = i;
				m_TAAInputs[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;//VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;// 
				m_TAAInputs[i].descriptorCount = 1;
				m_TAAInputs[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
				m_TAAInputs[i].pImmutableSamplers = NULL;
			}

			m_TAAInputs[4].binding = 4;
			m_TAAInputs[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			m_TAAInputs[4].descriptorCount = 1;
			m_TAAInputs[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			m_TAAInputs[4].pImmutableSamplers = NULL;

			for (int i = 5; i < 4 + 5; i++)
			{
				m_TAAInputs[i].binding = i;
				m_TAAInputs[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				m_TAAInputs[i].descriptorCount = 1;
				m_TAAInputs[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
				m_TAAInputs[i].pImmutableSamplers = NULL;// &m_samplers[i];
			}

			m_pResourceViewHeaps->CreateDescriptorSetLayout(&m_TAAInputs, &m_TaaDescriptorSetLayout);
			m_pResourceViewHeaps->AllocDescriptor(m_TaaDescriptorSetLayout, &m_TaaDescriptorSet);
#if 1
			m_TAA.OnCreate(m_pDevice, "TAA.hlsl", "main", "-T cs_6_0", m_TaaDescriptorSetLayout, 16, 16, 1, NULL);
			m_TAAFirst.OnCreate(m_pDevice, "TAA.hlsl", "first", "-T cs_6_0", m_TaaDescriptorSetLayout, 16, 16, 1, NULL);
#else
			DefineList defines;
			defines["FIRST_PASS"] = "1";
			m_TAA.OnCreate(m_pDevice, "TAA.glsl.h", "main", "", m_TaaDescriptorSetLayout, 16, 16, 1, NULL);
			m_TAAFirst.OnCreate(m_pDevice, "TAA.glsl.h", "main", "", m_TaaDescriptorSetLayout, 16, 16, 1, &defines);
#endif
		}

		// Sharpener
		//
		{
			std::vector<VkDescriptorSetLayoutBinding> m_SharpenInputs(3);
			m_SharpenInputs[0].binding = 0;
			m_SharpenInputs[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			m_SharpenInputs[0].descriptorCount = 1;
			m_SharpenInputs[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			m_SharpenInputs[0].pImmutableSamplers = NULL;

			m_SharpenInputs[1].binding = 1;
			m_SharpenInputs[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			m_SharpenInputs[1].descriptorCount = 1;
			m_SharpenInputs[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			m_SharpenInputs[1].pImmutableSamplers = NULL;

			m_SharpenInputs[2].binding = 2;
			m_SharpenInputs[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			m_SharpenInputs[2].descriptorCount = 1;
			m_SharpenInputs[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			m_SharpenInputs[2].pImmutableSamplers = NULL;

			m_pResourceViewHeaps->CreateDescriptorSetLayout(&m_SharpenInputs, &m_SharpenDescriptorSetLayout);
			m_pResourceViewHeaps->AllocDescriptor(m_SharpenDescriptorSetLayout, &m_SharpenDescriptorSet);

			m_Sharpen.OnCreate(m_pDevice, "TAASharpenerCS.hlsl", "mainCS", "-T cs_6_0", m_SharpenDescriptorSetLayout, 8, 8, 1, NULL);
			m_Post.OnCreate(m_pDevice, "TAASharpenerCS.hlsl", "postCS", "-T cs_6_0", m_SharpenDescriptorSetLayout, 8, 8, 1, NULL);
		}
	}

	void TAA::OnDestroy()
	{
		m_TAA.OnDestroy();
		m_TAAFirst.OnDestroy();
		vkDestroyDescriptorSetLayout(m_pDevice->m_device, m_TaaDescriptorSetLayout, NULL);

		m_Sharpen.OnDestroy();
		m_Post.OnDestroy();
		vkDestroyDescriptorSetLayout(m_pDevice->m_device, m_SharpenDescriptorSetLayout, NULL);

		//for (int i = 0; i < 4; i++)
		//	vkDestroySampler(m_pDevice->m_device, m_samplers[i], nullptr);

		m_pResourceViewHeaps->FreeDescriptor(m_SharpenDescriptorSet);
		m_pResourceViewHeaps->FreeDescriptor(m_TaaDescriptorSet);
	}

	//--------------------------------------------------------------------------------------
	//
	// OnCreateWindowSizeDependentResources
	//
	//--------------------------------------------------------------------------------------
	void TAA::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, GBuffer* pGBuffer)
	{
		m_pGBuffer = pGBuffer;

		m_Width = Width;
		m_Height = Height;

		// TAA buffers
		//
		m_TAABuffer.InitRenderTarget(m_pDevice, Width, Height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_TAABuffer");
		m_TAABuffer.CreateSRV(&m_TAABufferSRV);
		m_TAABuffer.CreateSRV(&m_TAABufferUAV);

		m_HistoryBuffer.InitRenderTarget(m_pDevice, Width, Height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_HistoryBuffer");
		m_HistoryBuffer.CreateSRV(&m_HistoryBufferSRV);
		//m_HistoryBuffer.CreateSRV(&m_HistoryBufferUAV);

		m_TexturesInUndefinedLayout = true;

		// update the TAA descriptor
		//
		m_pDevice->SetDescriptorSet(0, m_pGBuffer->m_HDRSRV, NULL, m_TaaDescriptorSet);
		m_pDevice->SetDescriptorSet(1, m_pGBuffer->m_DepthBufferSRV, NULL, m_TaaDescriptorSet);
		m_pDevice->SetDescriptorSet(2, m_HistoryBufferSRV, NULL, m_TaaDescriptorSet);
		m_pDevice->SetDescriptorSet(3, m_pGBuffer->m_MotionVectorsSRV, NULL, m_TaaDescriptorSet);
		m_pDevice->SetDescriptorSet(4, m_TAABufferUAV, m_TaaDescriptorSet);

		for (int i = 0; i < 4; i++)
		{
			VkDescriptorImageInfo samplerInfo = {};
			samplerInfo.sampler = m_samplers[i];

			VkWriteDescriptorSet write;
			write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.pNext = NULL;
			write.dstSet = m_TaaDescriptorSet;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			write.pImageInfo = &samplerInfo;
			write.dstBinding = i + 5;
			write.dstArrayElement = 0;

			vkUpdateDescriptorSets(m_pDevice->m_device, 1, &write, 0, NULL);
		}

		// update the Sharpen descriptor
		//
		m_pDevice->SetDescriptorSet(0, m_TAABufferSRV, NULL, m_SharpenDescriptorSet);
		m_pDevice->SetDescriptorSet(1, m_pGBuffer->m_HDRSRV, m_SharpenDescriptorSet);
		m_pDevice->SetDescriptorSet(2, m_HistoryBufferSRV, m_SharpenDescriptorSet);

		m_bFirst = true;
	}

	//--------------------------------------------------------------------------------------
	//
	// OnDestroyWindowSizeDependentResources
	//
	//--------------------------------------------------------------------------------------
	void TAA::OnDestroyWindowSizeDependentResources()
	{
		m_HistoryBuffer.OnDestroy();
		m_TAABuffer.OnDestroy();

		vkDestroyImageView(m_pDevice->m_device, m_TAABufferSRV, nullptr);
		vkDestroyImageView(m_pDevice->m_device, m_TAABufferUAV, nullptr);
		vkDestroyImageView(m_pDevice->m_device, m_HistoryBufferSRV, nullptr);
		//vkDestroyImageView(m_pDevice->m_device, m_HistoryBufferUAV, nullptr);
	}

	void TAA::Draw(VkCommandBuffer cmd_buf)
	{
		{
			SetPerfMarkerBegin(cmd_buf, "TAA");

			{
				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.pNext = NULL;
				barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkImageMemoryBarrier barriers[2];
				barriers[0] = barrier;
				barriers[0].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				barriers[0].oldLayout = m_TexturesInUndefinedLayout ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barriers[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
				barriers[0].image = m_TAABuffer.Resource();

				barriers[1] = barrier;
				barriers[1].oldLayout = m_TexturesInUndefinedLayout ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barriers[1].image = m_HistoryBuffer.Resource();

				m_TexturesInUndefinedLayout = false;

				vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 2, barriers);
			}
			if (m_bFirst)
			{
				m_bFirst = false;
				m_TAAFirst.Draw(cmd_buf, NULL, m_TaaDescriptorSet, (m_Width + 15) / 16, (m_Height + 15) / 16, 1);
			}
			else
				m_TAA.Draw(cmd_buf, NULL, m_TaaDescriptorSet, (m_Width + 15) / 16, (m_Height + 15) / 16, 1);
		}

		{
			SetPerfMarkerBegin(cmd_buf, "TAASharpener");

			{
				// default is color texture from SRV to UAV
				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.pNext = NULL;
				barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkImageMemoryBarrier barriers[3];
				barriers[0] = barrier;
				barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				barriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				barriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barriers[0].image = m_TAABuffer.Resource();

				barriers[1] = barrier;
				barriers[1].image = m_HistoryBuffer.Resource();

				barriers[2] = barrier;
				barriers[2].image = m_pGBuffer->m_HDR.Resource();

				vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0
					, NULL, 0, NULL, 3, barriers);
			}
			if (m_bSharpening)
				m_Sharpen.Draw(cmd_buf, NULL, m_SharpenDescriptorSet, (m_Width + 7) / 8, (m_Height + 7) / 8, 1);
			else
				m_Post.Draw(cmd_buf, NULL, m_SharpenDescriptorSet, (m_Width + 7) / 8, (m_Height + 7) / 8, 1);
			{
				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.pNext = NULL;
				barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkImageMemoryBarrier barriers[2];
				barriers[0] = barrier;
				barriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				barriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barriers[0].image = m_HistoryBuffer.Resource();

				barriers[1] = barrier;
				barriers[1].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				barriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barriers[1].image = m_pGBuffer->m_HDR.Resource();

				vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0
					, NULL, 0, NULL, 2, barriers);
			}
			SetPerfMarkerEnd(cmd_buf);
		}

		SetPerfMarkerEnd(cmd_buf);
	}

	// todo tonemapping

	void ToneMapping::OnCreate(Device* pDevice, VkRenderPass renderPass, ResourceViewHeaps* pResourceViewHeaps, StaticBufferPool* pStaticBufferPool, DynamicBufferRing* pDynamicBufferRing, uint32_t srvTableSize, const char* shaderSource)
	{
		m_pDevice = pDevice;
		m_pDynamicBufferRing = pDynamicBufferRing;
		m_pResourceViewHeaps = pResourceViewHeaps;

		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.minLod = -1000;
			info.maxLod = 1000;
			info.maxAnisotropy = 1.0f;
			m_sampler = pDevice->newSampler(&info);
			/*		VkResult res = vkCreateSampler(m_pDevice->m_device, &info, NULL, &m_sampler);
					assert(res == VK_SUCCESS);*/
		}

		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(2);
		layoutBindings[0].binding = 0;
		layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		layoutBindings[0].descriptorCount = 1;
		layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBindings[0].pImmutableSamplers = NULL;

		layoutBindings[1].binding = 1;
		layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindings[1].descriptorCount = srvTableSize;
		layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBindings[1].pImmutableSamplers = NULL;

		m_pResourceViewHeaps->CreateDescriptorSetLayout(&layoutBindings, &m_descriptorSetLayout);

		m_toneMapping.OnCreate(m_pDevice, renderPass, shaderSource, "main", "", pStaticBufferPool, pDynamicBufferRing, m_descriptorSetLayout, NULL, VK_SAMPLE_COUNT_1_BIT);

		m_descriptorIndex = 0;
		for (int i = 0; i < s_descriptorBuffers; i++)
			m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_descriptorSet[i]);
	}

	void ToneMapping::OnDestroy()
	{
		m_toneMapping.OnDestroy();

		for (int i = 0; i < s_descriptorBuffers; i++)
			m_pResourceViewHeaps->FreeDescriptor(m_descriptorSet[i]);

		//vkDestroySampler(m_pDevice->m_device, m_sampler, nullptr);

		vkDestroyDescriptorSetLayout(m_pDevice->m_device, m_descriptorSetLayout, NULL);
	}

	void ToneMapping::UpdatePipelines(VkRenderPass renderPass)
	{
		m_toneMapping.UpdatePipeline(renderPass, NULL, VK_SAMPLE_COUNT_1_BIT);
	}

	void ToneMapping::Draw(VkCommandBuffer cmd_buf, VkImageView HDRSRV, float exposure, int toneMapper)
	{
		SetPerfMarkerBegin(cmd_buf, "tonemapping");

		VkDescriptorBufferInfo cbTonemappingHandle;
		ToneMappingConsts* pToneMapping;
		m_pDynamicBufferRing->AllocConstantBuffer(sizeof(ToneMappingConsts), (void**)&pToneMapping, &cbTonemappingHandle);
		pToneMapping->exposure = exposure;
		pToneMapping->toneMapper = toneMapper;

		// We'll be modifying the descriptor set(DS), to prevent writing on a DS that is in use we 
		// need to do some basic buffering. Just to keep it safe and simple we'll have 10 buffers.
		VkDescriptorSet descriptorSet = m_descriptorSet[m_descriptorIndex];
		m_descriptorIndex = (m_descriptorIndex + 1) % s_descriptorBuffers;

		// modify Descriptor set
		m_pDevice->SetDescriptorSet(1, HDRSRV, m_sampler, descriptorSet);
		m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(ToneMappingConsts), descriptorSet);

		// Draw!
		m_toneMapping.Draw(cmd_buf, &cbTonemappingHandle, descriptorSet);

		SetPerfMarkerEnd(cmd_buf);
	}

	void ToneMappingCS::OnCreate(Device* pDevice, ResourceViewHeaps* pResourceViewHeaps, DynamicBufferRing* pDynamicBufferRing)
	{
		m_pDevice = pDevice;
		m_pDynamicBufferRing = pDynamicBufferRing;
		m_pResourceViewHeaps = pResourceViewHeaps;

		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(2);
		layoutBindings[0].binding = 0;
		layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		layoutBindings[0].descriptorCount = 1;
		layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		layoutBindings[0].pImmutableSamplers = NULL;

		layoutBindings[1].binding = 1;
		layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		layoutBindings[1].descriptorCount = 1;
		layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		layoutBindings[1].pImmutableSamplers = NULL;

		m_pResourceViewHeaps->CreateDescriptorSetLayout(&layoutBindings, &m_descriptorSetLayout);

		m_toneMapping.OnCreate(m_pDevice, "ToneMappingCS.glsl", "main", "", m_descriptorSetLayout, 8, 8, 1, NULL);

		m_descriptorIndex = 0;
		for (int i = 0; i < s_descriptorBuffers; i++)
			m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_descriptorSet[i]);
	}

	void ToneMappingCS::OnDestroy()
	{
		m_toneMapping.OnDestroy();

		for (int i = 0; i < s_descriptorBuffers; i++)
			m_pResourceViewHeaps->FreeDescriptor(m_descriptorSet[i]);

		vkDestroyDescriptorSetLayout(m_pDevice->m_device, m_descriptorSetLayout, NULL);
	}

	void ToneMappingCS::Draw(VkCommandBuffer cmd_buf, VkImageView HDRSRV, float exposure, int toneMapper, int width, int height)
	{
		SetPerfMarkerBegin(cmd_buf, "ToneMappingCS");

		VkDescriptorBufferInfo cbTonemappingHandle;
		ToneMappingConsts* pToneMapping;
		m_pDynamicBufferRing->AllocConstantBuffer(sizeof(ToneMappingConsts), (void**)&pToneMapping, &cbTonemappingHandle);
		pToneMapping->exposure = exposure;
		pToneMapping->toneMapper = toneMapper;

		// We'll be modifying the descriptor set(DS), to prevent writing on a DS that is in use we 
		// need to do some basic buffering. Just to keep it safe and simple we'll have 10 buffers.
		VkDescriptorSet descriptorSet = m_descriptorSet[m_descriptorIndex];
		m_descriptorIndex = (m_descriptorIndex + 1) % s_descriptorBuffers;

		// modify Descriptor set
		m_pDevice->SetDescriptorSet(1, HDRSRV, descriptorSet);
		m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(ToneMappingConsts), descriptorSet);

		// Draw!
		m_toneMapping.Draw(cmd_buf, &cbTonemappingHandle, descriptorSet, (width + 7) / 8, (height + 7) / 8, 1);

		SetPerfMarkerEnd(cmd_buf);
	}
	// oit混合cs
	void oitblendCS::OnCreate(Device* pDevice, ResourceViewHeaps* pResourceViewHeaps, DynamicBufferRing* pDynamicBufferRing)
	{
		m_pDevice = pDevice;
		m_pDynamicBufferRing = pDynamicBufferRing;
		m_pResourceViewHeaps = pResourceViewHeaps;
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(3);
		for (int i = 0; i < 3; i++) {
			layoutBindings[i].binding = i;
			layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			layoutBindings[i].descriptorCount = 1;
			layoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			layoutBindings[i].pImmutableSamplers = NULL;
		}
		m_pResourceViewHeaps->CreateDescriptorSetLayout(&layoutBindings, &m_descriptorSetLayout);
		oitblend.OnCreate(m_pDevice, "oit_blendCS.glsl", "main", "", m_descriptorSetLayout, 8, 8, 1, NULL);
		m_descriptorIndex = 0;
		for (int i = 0; i < s_descriptorBuffers; i++)
			m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_descriptorSet[i]);
	}
	void oitblendCS::OnDestroy()
	{
		oitblend.OnDestroy();
		for (int i = 0; i < s_descriptorBuffers; i++)
			m_pResourceViewHeaps->FreeDescriptor(m_descriptorSet[i]);
		vkDestroyDescriptorSetLayout(m_pDevice->m_device, m_descriptorSetLayout, NULL);
	}
	void oitblendCS::Draw(VkCommandBuffer cmd_buf, VkImageView HDRSRV, VkImageView accumTex, VkImageView weightTex, int width, int height)
	{
		SetPerfMarkerBegin(cmd_buf, "oitblendCS");
		VkDescriptorSet descriptorSet = m_descriptorSet[m_descriptorIndex];
		m_descriptorIndex = (m_descriptorIndex + 1) % s_descriptorBuffers;
		// modify Descriptor set
		m_pDevice->SetDescriptorSet(0, HDRSRV, descriptorSet);
		m_pDevice->SetDescriptorSet(1, accumTex, descriptorSet);
		m_pDevice->SetDescriptorSet(2, weightTex, descriptorSet);
		// Draw!
		oitblend.Draw(cmd_buf, 0, descriptorSet, (width + 7) / 8, (height + 7) / 8, 1);
		SetPerfMarkerEnd(cmd_buf);
	}




	// todo bloom

	void Bloom::OnCreate(
		Device* pDevice,
		ResourceViewHeaps* pResourceViewHeaps,
		DynamicBufferRing* pConstantBufferRing,
		StaticBufferPool* pStaticBufferPool,
		VkFormat outFormat
	)
	{
		m_pDevice = pDevice;
		m_pResourceViewHeaps = pResourceViewHeaps;
		m_pConstantBufferRing = pConstantBufferRing;
		m_outFormat = outFormat;

		m_blur.OnCreate(pDevice, m_pResourceViewHeaps, m_pConstantBufferRing, pStaticBufferPool, m_outFormat);

		// Create Descriptor Set Layout (for each mip level we will create later on the individual Descriptor Sets)
		//
		{
			std::vector<VkDescriptorSetLayoutBinding> layoutBindings(2);
			layoutBindings[0].binding = 0;
			layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			layoutBindings[0].descriptorCount = 1;
			layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBindings[0].pImmutableSamplers = NULL;

			layoutBindings[1].binding = 1;
			layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBindings[1].descriptorCount = 1;
			layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBindings[1].pImmutableSamplers = NULL;

			VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
			descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptor_layout.pNext = NULL;
			descriptor_layout.bindingCount = (uint32_t)layoutBindings.size();
			descriptor_layout.pBindings = layoutBindings.data();

			VkResult res = vkCreateDescriptorSetLayout(pDevice->m_device, &descriptor_layout, NULL, &m_descriptorSetLayout);
			assert(res == VK_SUCCESS);
		}

		// Create a Render pass that accounts for blending
		//
		m_blendPass = SimpleColorBlendRenderPass(pDevice->m_device, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, outFormat);

		//blending add
		{
			VkPipelineColorBlendAttachmentState att_state[1];
			att_state[0].colorWriteMask = allBits;
			att_state[0].blendEnable = VK_TRUE;
			att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
			att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
			att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
			att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

			VkPipelineColorBlendStateCreateInfo cb;
			cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			cb.flags = 0;
			cb.pNext = NULL;
			cb.attachmentCount = 1;
			cb.pAttachments = att_state;
			cb.logicOpEnable = VK_FALSE;
			cb.logicOp = VK_LOGIC_OP_NO_OP;
			cb.blendConstants[0] = 1.0f;
			cb.blendConstants[1] = 1.0f;
			cb.blendConstants[2] = 1.0f;
			cb.blendConstants[3] = 1.0f;

			m_blendAdd.OnCreate(pDevice, m_blendPass, "blend.glsl", "main", "", pStaticBufferPool, pConstantBufferRing, m_descriptorSetLayout, &cb);
		}

		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.minLod = -1000;
			info.maxLod = 1000;
			info.maxAnisotropy = 1.0f;
			m_sampler = pDevice->newSampler(&info);
			/*		VkResult res = vkCreateSampler(pDevice->m_device, &info, NULL, &m_sampler);
					assert(res == VK_SUCCESS);*/
		}

		// Allocate descriptors for the mip chain
		//
		for (int i = 0; i < BLOOM_MAX_MIP_LEVELS; i++)
		{
			m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_mip[i].m_descriptorSet);
		}

		// Allocate descriptors for the output pass
		//
		m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_output.m_descriptorSet);

		m_doBlur = true;
		m_doUpscale = true;
	}

	void Bloom::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, Texture* pInput, int mipCount, Texture* pOutput)
	{
		m_Width = Width;
		m_Height = Height;
		m_mipCount = mipCount;

		m_blur.OnCreateWindowSizeDependentResources(m_pDevice, Width, Height, pInput, mipCount);

		for (int i = 0; i < m_mipCount; i++)
		{
			pInput->CreateSRV(&m_mip[i].m_SRV, i);     // source (pInput)
			pInput->CreateRTV(&m_mip[i].m_RTV, i);     // target (pInput)

			// Create framebuffer for target
			//
			{
				VkImageView attachments[1] = { m_mip[i].m_RTV };

				VkFramebufferCreateInfo fb_info = {};
				fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				fb_info.pNext = NULL;
				fb_info.renderPass = m_blendPass;
				fb_info.attachmentCount = 1;
				fb_info.pAttachments = attachments;
				fb_info.width = Width >> i;
				fb_info.height = Height >> i;
				fb_info.layers = 1;
				VkResult res = vkCreateFramebuffer(m_pDevice->m_device, &fb_info, NULL, &m_mip[i].m_frameBuffer);
				assert(res == VK_SUCCESS);

				SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_mip[i].m_frameBuffer, "BloomBlended");
			}

			// Set descriptors        
			m_pConstantBufferRing->SetDescriptorSet(0, sizeof(Bloom::cbBlend), m_mip[i].m_descriptorSet);
			m_pDevice->SetDescriptorSet(1, m_mip[i].m_SRV, m_sampler, m_mip[i].m_descriptorSet);
		}

		{
			// output pass
			pOutput->CreateRTV(&m_output.m_RTV, 0);

			// Create framebuffer for target
			//
			{
				VkImageView attachments[1] = { m_output.m_RTV };

				VkFramebufferCreateInfo fb_info = {};
				fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				fb_info.pNext = NULL;
				fb_info.renderPass = m_blendPass;
				fb_info.attachmentCount = 1;
				fb_info.pAttachments = attachments;
				fb_info.width = Width * 2;
				fb_info.height = Height * 2;
				fb_info.layers = 1;
				VkResult res = vkCreateFramebuffer(m_pDevice->m_device, &fb_info, NULL, &m_output.m_frameBuffer);
				assert(res == VK_SUCCESS);

				SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_output.m_frameBuffer, "BloomOutput");
			}

			// Set descriptors        
			m_pConstantBufferRing->SetDescriptorSet(0, sizeof(Bloom::cbBlend), m_output.m_descriptorSet);
			m_pDevice->SetDescriptorSet(1, m_mip[1].m_SRV, m_sampler, m_output.m_descriptorSet);
		}

		// set weights of each mip level
		m_mip[0].m_weight = 1.0f - 0.08f;
		m_mip[1].m_weight = 0.25f;
		m_mip[2].m_weight = 0.75f;
		m_mip[3].m_weight = 1.5f;
		m_mip[4].m_weight = 2.5f;
		m_mip[5].m_weight = 3.0f;

		// normalize weights
		float n = 0;
		for (uint32_t i = 1; i < 6; i++)
			n += m_mip[i].m_weight;

		for (uint32_t i = 1; i < 6; i++)
			m_mip[i].m_weight /= n;
	}

	void Bloom::OnDestroyWindowSizeDependentResources()
	{
		m_blur.OnDestroyWindowSizeDependentResources();

		for (int i = 0; i < m_mipCount; i++)
		{
			vkDestroyImageView(m_pDevice->m_device, m_mip[i].m_SRV, NULL);
			vkDestroyImageView(m_pDevice->m_device, m_mip[i].m_RTV, NULL);
			vkDestroyFramebuffer(m_pDevice->m_device, m_mip[i].m_frameBuffer, NULL);
		}

		vkDestroyImageView(m_pDevice->m_device, m_output.m_RTV, NULL);
		vkDestroyFramebuffer(m_pDevice->m_device, m_output.m_frameBuffer, NULL);
	}

	void Bloom::OnDestroy()
	{
		for (int i = 0; i < BLOOM_MAX_MIP_LEVELS; i++)
		{
			m_pResourceViewHeaps->FreeDescriptor(m_mip[i].m_descriptorSet);
		}

		m_pResourceViewHeaps->FreeDescriptor(m_output.m_descriptorSet);

		m_blur.OnDestroy();

		vkDestroyDescriptorSetLayout(m_pDevice->m_device, m_descriptorSetLayout, NULL);
		//vkDestroySampler(m_pDevice->m_device, m_sampler, nullptr);

		m_blendAdd.OnDestroy();

		vkDestroyRenderPass(m_pDevice->m_device, m_blendPass, NULL);
	}

	void Bloom::Draw(VkCommandBuffer cmd_buf)
	{
		SetPerfMarkerBegin(cmd_buf, "Bloom");

		//float weights[6] = { 0.25, 0.75, 1.5, 2, 2.5, 3.0 };

		// given a RT, and its mip chain m0, m1, m2, m3, m4 
		// 
		// m4 = blur(m5)
		// m4 = blur(m4) + w5 *m5
		// m3 = blur(m3) + w4 *m4
		// m2 = blur(m2) + w3 *m3
		// m1 = blur(m1) + w2 *m2
		// m0 = blur(m0) + w1 *m1
		// RT = 0.92 * RT + 0.08 * m0

		// blend and upscale
		for (int i = m_mipCount - 1; i >= 0; i--)
		{
			// blur mip level
			//
			if (m_doBlur)
			{
				m_blur.Draw(cmd_buf, i);
				// force wait for the draw to completely finish
				// TODO: need to find a better way to do it
				vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 0, NULL);
			}

			// blend with mip above   
			SetPerfMarkerBegin(cmd_buf, "blend above");

			Bloom::cbBlend* data;
			VkDescriptorBufferInfo constantBuffer;
			m_pConstantBufferRing->AllocConstantBuffer(sizeof(Bloom::cbBlend), (void**)&data, &constantBuffer);

			if (i != 0)
			{
				VkRenderPassBeginInfo rp_begin = {};
				rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				rp_begin.pNext = NULL;
				rp_begin.renderPass = m_blendPass;
				rp_begin.framebuffer = m_mip[i - 1].m_frameBuffer;
				rp_begin.renderArea.offset.x = 0;
				rp_begin.renderArea.offset.y = 0;
				rp_begin.renderArea.extent.width = m_Width >> (i - 1);
				rp_begin.renderArea.extent.height = m_Height >> (i - 1);
				rp_begin.clearValueCount = 0;
				rp_begin.pClearValues = NULL;
				vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

				SetViewportAndScissor(cmd_buf, 0, 0, m_Width >> (i - 1), m_Height >> (i - 1));

				float blendConstants[4] = { m_mip[i].m_weight, m_mip[i].m_weight, m_mip[i].m_weight, m_mip[i].m_weight };
				vkCmdSetBlendConstants(cmd_buf, blendConstants);

				data->weight = 1.0f;
			}
			else
			{
				//composite
				VkRenderPassBeginInfo rp_begin = {};
				rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				rp_begin.pNext = NULL;
				rp_begin.renderPass = m_blendPass;
				rp_begin.framebuffer = m_output.m_frameBuffer;
				rp_begin.renderArea.offset.x = 0;
				rp_begin.renderArea.offset.y = 0;
				rp_begin.renderArea.extent.width = m_Width * 2;
				rp_begin.renderArea.extent.height = m_Height * 2;
				rp_begin.clearValueCount = 0;
				rp_begin.pClearValues = NULL;
				vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

				SetViewportAndScissor(cmd_buf, 0, 0, m_Width * 2, m_Height * 2);

				float blendConstants[4] = { m_mip[i].m_weight, m_mip[i].m_weight, m_mip[i].m_weight, m_mip[i].m_weight };
				vkCmdSetBlendConstants(cmd_buf, blendConstants);

				data->weight = 1.0f - m_mip[i].m_weight;
			}

			if (m_doUpscale)
				m_blendAdd.Draw(cmd_buf, &constantBuffer, m_mip[i].m_descriptorSet);

			vkCmdEndRenderPass(cmd_buf);
			SetPerfMarkerEnd(cmd_buf);
		}

		SetPerfMarkerEnd(cmd_buf);
	}

	void Bloom::Gui()
	{
		//bool opened = true;
		//if (ImGui::Begin("Bloom Controls", &opened))
		//{
		//	ImGui::Checkbox("Blur Bloom Stages", &m_doBlur);
		//	ImGui::Checkbox("Upscaling", &m_doUpscale);

		//	ImGui::SliderFloat("weight 0", &m_mip[0].m_weight, 0.0f, 1.0f);

		//	for (int i = 1; i < m_mipCount; i++)
		//	{
		//		char buf[32];
		//		sprintf_s<32>(buf, "weight %i", i);
		//		ImGui::SliderFloat(buf, &m_mip[i].m_weight, 0.0f, 4.0f);
		//	}
		//}
		//ImGui::End();
	}
	// todo blur

	void BlurPS::OnCreate(
		Device* pDevice,
		ResourceViewHeaps* pResourceViewHeaps,
		DynamicBufferRing* pConstantBufferRing,
		StaticBufferPool* pStaticBufferPool,
		VkFormat format
	)
	{
		m_pDevice = pDevice;
		m_pResourceViewHeaps = pResourceViewHeaps;
		m_pConstantBufferRing = pConstantBufferRing;

		m_outFormat = format;

		// Create Descriptor Set Layout, the shader needs a uniform dynamic buffer and a texture + sampler
		// The Descriptor Sets will be created and initialized once we know the input to the shader, that happens in OnCreateWindowSizeDependentResources()
		{
			std::vector<VkDescriptorSetLayoutBinding> layoutBindings(2);
			layoutBindings[0].binding = 0;
			layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			layoutBindings[0].descriptorCount = 1;
			layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBindings[0].pImmutableSamplers = NULL;

			layoutBindings[1].binding = 1;
			layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBindings[1].descriptorCount = 1;
			layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBindings[1].pImmutableSamplers = NULL;

			VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
			descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptor_layout.pNext = NULL;
			descriptor_layout.bindingCount = (uint32_t)layoutBindings.size();
			descriptor_layout.pBindings = layoutBindings.data();

			VkResult res = vkCreateDescriptorSetLayout(pDevice->m_device, &descriptor_layout, NULL, &m_descriptorSetLayout);
			assert(res == VK_SUCCESS);
		}

		// Create a Render pass that will discard the contents of the render target.
		//
		m_in = SimpleColorWriteRenderPass(pDevice->m_device, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, format);

		// The sampler we want to use for downsampling, all linear
		//
		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.minLod = -1000;
			info.maxLod = 1000;
			info.maxAnisotropy = 1.0f;
			m_sampler = pDevice->newSampler(&info);
			/*		VkResult res = vkCreateSampler(pDevice->m_device, &info, NULL, &m_sampler);
					assert(res == VK_SUCCESS);*/
		}

		// Use helper class_to create the fullscreen pass 
		//
		m_directionalBlur.OnCreate(pDevice, m_in, "blur.glsl", "main", "", pStaticBufferPool, pConstantBufferRing, m_descriptorSetLayout);

		// Allocate descriptors for the mip chain
		//
		for (int i = 0; i < BLURPS_MAX_MIP_LEVELS; i++)
		{
			// Horizontal pass
			//
			m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_horizontalMip[i].m_descriptorSet);

			// Vertical pass
			//
			m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_verticalMip[i].m_descriptorSet);
		}

	}

	void BlurPS::OnCreateWindowSizeDependentResources(Device* pDevice, uint32_t Width, uint32_t Height, Texture* pInput, int mipCount)
	{
		m_Width = Width;
		m_Height = Height;
		m_mipCount = mipCount;

		m_inputTexture = pInput;

		// Create a temporary texture to hold the horizontal pass (only now we know the size of the render target we want to downsample, hence we create the temporary render target here)
		//
		VkImageCreateInfo image_info = {};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.pNext = NULL;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.format = m_outFormat;
		image_info.extent.width = m_Width;
		image_info.extent.height = m_Height;
		image_info.extent.depth = 1;
		image_info.mipLevels = mipCount;
		image_info.arrayLayers = 1;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.queueFamilyIndexCount = 0;
		image_info.pQueueFamilyIndices = NULL;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_info.usage = (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		image_info.flags = 0;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;   // VK_IMAGE_TILING_LINEAR should never be used and will never be faster
		m_tempBlur.Init(m_pDevice, &image_info, "BlurHorizontal");

		// Create framebuffers and views for the mip chain
		//
		for (int i = 0; i < m_mipCount; i++)
		{
			// Horizontal pass, from pInput to m_tempBlur
			//
			{
				pInput->CreateSRV(&m_horizontalMip[i].m_SRV, i);     // source (pInput)
				m_tempBlur.CreateRTV(&m_horizontalMip[i].m_RTV, i);  // target (m_tempBlur)

				// Create framebuffer for target
				//
				{
					VkImageView attachments[1] = { m_horizontalMip[i].m_RTV };

					VkFramebufferCreateInfo fb_info = {};
					fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
					fb_info.pNext = NULL;
					fb_info.renderPass = m_in;
					fb_info.attachmentCount = 1;
					fb_info.pAttachments = attachments;
					fb_info.width = Width >> i;
					fb_info.height = Height >> i;
					fb_info.layers = 1;
					VkResult res = vkCreateFramebuffer(m_pDevice->m_device, &fb_info, NULL, &m_horizontalMip[i].m_frameBuffer);
					assert(res == VK_SUCCESS);

					SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_horizontalMip[i].m_frameBuffer, "BlurPSHorizontal");
				}

				// Create Descriptor sets (all of them use the same Descriptor Layout)            
				m_pConstantBufferRing->SetDescriptorSet(0, sizeof(BlurPS::cbBlur), m_horizontalMip[i].m_descriptorSet);
				m_pDevice->SetDescriptorSet(1, m_horizontalMip[i].m_SRV, m_sampler, m_horizontalMip[i].m_descriptorSet);
			}

			// Vertical pass, from m_tempBlur back to pInput
			//
			{
				m_tempBlur.CreateSRV(&m_verticalMip[i].m_SRV, i);   // source (pInput)(m_tempBlur)
				pInput->CreateRTV(&m_verticalMip[i].m_RTV, i);      // target (pInput)

				{
					VkImageView attachments[1] = { m_verticalMip[i].m_RTV };

					VkFramebufferCreateInfo fb_info = {};
					fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
					fb_info.pNext = NULL;
					fb_info.renderPass = m_in;
					fb_info.attachmentCount = 1;
					fb_info.pAttachments = attachments;
					fb_info.width = Width >> i;
					fb_info.height = Height >> i;
					fb_info.layers = 1;
					VkResult res = vkCreateFramebuffer(m_pDevice->m_device, &fb_info, NULL, &m_verticalMip[i].m_frameBuffer);
					assert(res == VK_SUCCESS);

					SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_verticalMip[i].m_frameBuffer, "BlurPSVertical");
				}

				// create and update descriptor
				m_pConstantBufferRing->SetDescriptorSet(0, sizeof(BlurPS::cbBlur), m_verticalMip[i].m_descriptorSet);
				m_pDevice->SetDescriptorSet(1, m_verticalMip[i].m_SRV, m_sampler, m_verticalMip[i].m_descriptorSet);
			}
		}
	}

	void BlurPS::OnDestroyWindowSizeDependentResources()
	{
		// destroy views and framebuffers of the vertical and horizontal passes
		//
		for (int i = 0; i < m_mipCount; i++)
		{
			vkDestroyImageView(m_pDevice->m_device, m_horizontalMip[i].m_SRV, NULL);
			vkDestroyImageView(m_pDevice->m_device, m_horizontalMip[i].m_RTV, NULL);
			vkDestroyFramebuffer(m_pDevice->m_device, m_horizontalMip[i].m_frameBuffer, NULL);

			vkDestroyImageView(m_pDevice->m_device, m_verticalMip[i].m_SRV, NULL);
			vkDestroyImageView(m_pDevice->m_device, m_verticalMip[i].m_RTV, NULL);
			vkDestroyFramebuffer(m_pDevice->m_device, m_verticalMip[i].m_frameBuffer, NULL);
		}

		// Destroy temporary render target used to hold the horizontal pass
		//
		m_tempBlur.OnDestroy();
	}

	void BlurPS::OnDestroy()
	{
		// destroy views
		for (int i = 0; i < BLURPS_MAX_MIP_LEVELS; i++)
		{
			m_pResourceViewHeaps->FreeDescriptor(m_horizontalMip[i].m_descriptorSet);
			m_pResourceViewHeaps->FreeDescriptor(m_verticalMip[i].m_descriptorSet);
		}

		m_directionalBlur.OnDestroy();
		vkDestroyDescriptorSetLayout(m_pDevice->m_device, m_descriptorSetLayout, NULL);
		//vkDestroySampler(m_pDevice->m_device, m_sampler, nullptr);
		vkDestroyRenderPass(m_pDevice->m_device, m_in, NULL);
	}

	void BlurPS::Draw(VkCommandBuffer cmd_buf, int mipLevel)
	{
		SetPerfMarkerBegin(cmd_buf, "blur");

		SetViewportAndScissor(cmd_buf, 0, 0, m_Width >> mipLevel, m_Height >> mipLevel);

		// horizontal pass
		//
		{
			VkRenderPassBeginInfo rp_begin = {};
			rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rp_begin.pNext = NULL;
			rp_begin.renderPass = m_in;
			rp_begin.framebuffer = m_horizontalMip[mipLevel].m_frameBuffer;
			rp_begin.renderArea.offset.x = 0;
			rp_begin.renderArea.offset.y = 0;
			rp_begin.renderArea.extent.width = m_Width >> mipLevel;
			rp_begin.renderArea.extent.height = m_Height >> mipLevel;
			rp_begin.clearValueCount = 0;
			rp_begin.pClearValues = NULL;
			vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

			BlurPS::cbBlur* data;
			VkDescriptorBufferInfo constantBuffer;
			m_pConstantBufferRing->AllocConstantBuffer(sizeof(BlurPS::cbBlur), (void**)&data, &constantBuffer);
			data->dirX = 1.0f / (float)(m_Width >> mipLevel);
			data->dirY = 0.0f / (float)(m_Height >> mipLevel);
			data->mipLevel = mipLevel;
			m_directionalBlur.Draw(cmd_buf, &constantBuffer, m_horizontalMip[mipLevel].m_descriptorSet);

			vkCmdEndRenderPass(cmd_buf);
		}

		// Memory barrier to transition input texture layout from shader read to render target
		// Note the miplevel
		//
		{
			VkImageMemoryBarrier barrier[1] = {};

			// transition input to render target
			barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier[0].pNext = nullptr;
			barrier[0].srcAccessMask = 0;
			barrier[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier[0].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier[0].image = m_inputTexture->Resource();
			barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier[0].subresourceRange.baseMipLevel = mipLevel;
			barrier[0].subresourceRange.levelCount = 1;
			barrier[0].subresourceRange.baseArrayLayer = 0;
			barrier[0].subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, barrier);
		}

		// vertical pass
		//
		{
			VkRenderPassBeginInfo rp_begin = {};
			rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rp_begin.pNext = NULL;
			rp_begin.renderPass = m_in;
			rp_begin.framebuffer = m_verticalMip[mipLevel].m_frameBuffer;
			rp_begin.renderArea.offset.x = 0;
			rp_begin.renderArea.offset.y = 0;
			rp_begin.renderArea.extent.width = m_Width >> mipLevel;
			rp_begin.renderArea.extent.height = m_Height >> mipLevel;
			rp_begin.clearValueCount = 0;
			rp_begin.pClearValues = NULL;
			vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

			BlurPS::cbBlur* data;
			VkDescriptorBufferInfo constantBuffer;
			m_pConstantBufferRing->AllocConstantBuffer(sizeof(BlurPS::cbBlur), (void**)&data, &constantBuffer);
			data->dirX = 0.0f / (float)(m_Width >> mipLevel);
			data->dirY = 1.0f / (float)(m_Height >> mipLevel);
			data->mipLevel = mipLevel;
			m_directionalBlur.Draw(cmd_buf, &constantBuffer, m_verticalMip[mipLevel].m_descriptorSet);

			vkCmdEndRenderPass(cmd_buf);
		}

		SetPerfMarkerEnd(cmd_buf);
	}

	void BlurPS::Draw(VkCommandBuffer cmd_buf)
	{
		for (int i = 0; i < m_mipCount; i++)
		{
			Draw(cmd_buf, i);
		}
	}

	// todo cc

	void ColorConversionPS::OnCreate(Device* pDevice, VkRenderPass renderPass, ResourceViewHeaps* pResourceViewHeaps, StaticBufferPool* pStaticBufferPool, DynamicBufferRing* pDynamicBufferRing)
	{
		m_pDevice = pDevice;
		m_pDynamicBufferRing = pDynamicBufferRing;
		m_pResourceViewHeaps = pResourceViewHeaps;

		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.minLod = -1000;
			info.maxLod = 1000;
			info.maxAnisotropy = 1.0f;
			m_sampler = pDevice->newSampler(&info);
			//VkResult res = vkCreateSampler(m_pDevice->m_device, &info, NULL, &m_sampler);
			//assert(res == VK_SUCCESS);
		}

		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(2);
		layoutBindings[0].binding = 0;
		layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		layoutBindings[0].descriptorCount = 1;
		layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBindings[0].pImmutableSamplers = NULL;

		layoutBindings[1].binding = 1;
		layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindings[1].descriptorCount = 1;
		layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBindings[1].pImmutableSamplers = NULL;

		m_pResourceViewHeaps->CreateDescriptorSetLayout(&layoutBindings, &m_descriptorSetLayout);

		m_ColorConversion.OnCreate(m_pDevice, renderPass, "ColorConversionPS.glsl", "main", "", pStaticBufferPool, pDynamicBufferRing, m_descriptorSetLayout, NULL, VK_SAMPLE_COUNT_1_BIT);

		m_descriptorIndex = 0;
		for (int i = 0; i < s_descriptorBuffers; i++)
			m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_descriptorSet[i]);
	}

	void ColorConversionPS::OnDestroy()
	{
		m_ColorConversion.OnDestroy();

		for (int i = 0; i < s_descriptorBuffers; i++)
			m_pResourceViewHeaps->FreeDescriptor(m_descriptorSet[i]);

		//vkDestroySampler(m_pDevice->m_device, m_sampler, nullptr);

		vkDestroyDescriptorSetLayout(m_pDevice->m_device, m_descriptorSetLayout, NULL);
	}

	void ColorConversionPS::UpdatePipelines(VkRenderPass renderPass, DisplayMode displayMode)
	{
		m_ColorConversion.UpdatePipeline(renderPass, NULL, VK_SAMPLE_COUNT_1_BIT);

		m_colorConversionConsts.m_displayMode = displayMode;

		if (displayMode != DISPLAYMODE_SDR)
		{
#ifdef fsHdrGetDisplayInfo
			const VkHdrMetadataEXT* pHDRMetatData = fsHdrGetDisplayInfo();

			m_colorConversionConsts.m_displayMinLuminancePerNits = (float)pHDRMetatData->minLuminance / 80.0f; // RGB(1, 1, 1) maps to 80 nits in scRGB;
			m_colorConversionConsts.m_displayMaxLuminancePerNits = (float)pHDRMetatData->maxLuminance / 80.0f; // This means peak white equals RGB(m_maxLuminanace/80.0f, m_maxLuminanace/80.0f, m_maxLuminanace/80.0f) in scRGB;

			FillDisplaySpecificPrimaries(
				pHDRMetatData->whitePoint.x, pHDRMetatData->whitePoint.y,
				pHDRMetatData->displayPrimaryRed.x, pHDRMetatData->displayPrimaryRed.y,
				pHDRMetatData->displayPrimaryGreen.x, pHDRMetatData->displayPrimaryGreen.y,
				pHDRMetatData->displayPrimaryBlue.x, pHDRMetatData->displayPrimaryBlue.y
			);

			SetupGamutMapperMatrices(
				ColorSpace_REC709,
				ColorSpace_Display,
				&m_colorConversionConsts.m_contentToMonitorRecMatrix);
#endif
		}
	}

	void ColorConversionPS::Draw(VkCommandBuffer cmd_buf, VkImageView HDRSRV)
	{
		SetPerfMarkerBegin(cmd_buf, "ColorConversion");

		VkDescriptorBufferInfo cbTonemappingHandle;
		ColorConversionConsts* pColorConversionConsts;
		m_pDynamicBufferRing->AllocConstantBuffer(sizeof(ColorConversionConsts), (void**)&pColorConversionConsts, &cbTonemappingHandle);
		*pColorConversionConsts = m_colorConversionConsts;

		// We'll be modifying the descriptor set(DS), to prevent writing on a DS that is in use we 
		// need to do some basic buffering. Just to keep it safe and simple we'll have 10 buffers.
		VkDescriptorSet descriptorSet = m_descriptorSet[m_descriptorIndex];
		m_descriptorIndex = (m_descriptorIndex + 1) % s_descriptorBuffers;

		// modify Descriptor set
		m_pDevice->SetDescriptorSet(1, HDRSRV, m_sampler, descriptorSet);
		m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(ColorConversionConsts), descriptorSet);

		// Draw!
		m_ColorConversion.Draw(cmd_buf, &cbTonemappingHandle, descriptorSet);

		SetPerfMarkerEnd(cmd_buf);
	}
	// todo downs

	void DownSamplePS::OnCreate(
		Device* pDevice,
		ResourceViewHeaps* pResourceViewHeaps,
		DynamicBufferRing* pConstantBufferRing,
		StaticBufferPool* pStaticBufferPool,
		VkFormat outFormat
	)
	{
		m_pDevice = pDevice;
		m_pStaticBufferPool = pStaticBufferPool;
		m_pResourceViewHeaps = pResourceViewHeaps;
		m_pConstantBufferRing = pConstantBufferRing;
		m_outFormat = outFormat;

		// Create Descriptor Set Layout, the shader needs a uniform dynamic buffer and a texture + sampler
		// The Descriptor Sets will be created and initialized once we know the input to the shader, that happens in OnCreateWindowSizeDependentResources()
		{
			std::vector<VkDescriptorSetLayoutBinding> layoutBindings(2);
			layoutBindings[0].binding = 0;
			layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			layoutBindings[0].descriptorCount = 1;
			layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBindings[0].pImmutableSamplers = NULL;

			layoutBindings[1].binding = 1;
			layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBindings[1].descriptorCount = 1;
			layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBindings[1].pImmutableSamplers = NULL;

			VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
			descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptor_layout.pNext = NULL;
			descriptor_layout.bindingCount = (uint32_t)layoutBindings.size();
			descriptor_layout.pBindings = layoutBindings.data();

			VkResult res = vkCreateDescriptorSetLayout(pDevice->m_device, &descriptor_layout, NULL, &m_descriptorSetLayout);
			assert(res == VK_SUCCESS);
		}

		// In Render pass
		//
		m_in = SimpleColorWriteRenderPass(pDevice->m_device, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, outFormat);

		// The sampler we want to use for downsampling, all linear
		//
		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.minLod = -1000;
			info.maxLod = 1000;
			info.maxAnisotropy = 1.0f;
			m_sampler = pDevice->newSampler(&info);
			//VkResult res = vkCreateSampler(pDevice->m_device, &info, NULL, &m_sampler);
			//assert(res == VK_SUCCESS);
		}

		// Use helper class_to create the fullscreen pass
		//
		m_downscale.OnCreate(pDevice, m_in, "DownSamplePS.glsl", "main", "", pStaticBufferPool, pConstantBufferRing, m_descriptorSetLayout);

		// Allocate descriptors for the mip chain
		//
		for (int i = 0; i < DOWNSAMPLEPS_MAX_MIP_LEVELS; i++)
		{
			m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_mip[i].descriptorSet);
		}
	}

	void DownSamplePS::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, Texture* pInput, int mipCount)
	{
		m_Width = Width;
		m_Height = Height;
		m_mipCount = mipCount;
		VkImageCreateInfo image_info = {};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.pNext = NULL;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.format = m_outFormat;
		image_info.extent.width = m_Width >> 1;
		image_info.extent.height = m_Height >> 1;
		image_info.extent.depth = 1;
		image_info.mipLevels = mipCount;
		image_info.arrayLayers = 1;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.queueFamilyIndexCount = 0;
		image_info.pQueueFamilyIndices = NULL;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_info.usage = (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		image_info.flags = 0;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		m_result.Init(m_pDevice, &image_info, "DownsampleMip");
		// Create views for the mip chain
		for (int i = 0; i < m_mipCount; i++)
		{
			// source -----------
			if (i == 0)
			{
				pInput->CreateSRV(&m_mip[i].m_SRV, 0);
			}
			else
			{
				m_result.CreateSRV(&m_mip[i].m_SRV, i - 1);
			}
			// Create and initialize the Descriptor Sets (all of them use the same Descriptor Layout)        
			m_pConstantBufferRing->SetDescriptorSet(0, sizeof(DownSamplePS::cbDownscale), m_mip[i].descriptorSet);
			m_pDevice->SetDescriptorSet(1, m_mip[i].m_SRV, m_sampler, m_mip[i].descriptorSet);
			// destination -----------
			m_result.CreateRTV(&m_mip[i].RTV, i);
			// Create framebuffer 
			{
				VkImageView attachments[1] = { m_mip[i].RTV };
				VkFramebufferCreateInfo fb_info = {};
				fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				fb_info.pNext = NULL;
				fb_info.renderPass = m_in;
				fb_info.attachmentCount = 1;
				fb_info.pAttachments = attachments;
				fb_info.width = m_Width >> (i + 1);
				fb_info.height = m_Height >> (i + 1);
				fb_info.layers = 1;
				VkResult res = vkCreateFramebuffer(m_pDevice->m_device, &fb_info, NULL, &m_mip[i].frameBuffer);
				assert(res == VK_SUCCESS);

				std::string ResourceName = "DownsamplePS";
				ResourceName += std::to_string(i);

				SetResourceName(m_pDevice->m_device, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_mip[i].frameBuffer, ResourceName.c_str());
			}
		}
	}

	void DownSamplePS::OnDestroyWindowSizeDependentResources()
	{
		for (int i = 0; i < m_mipCount; i++)
		{
			vkDestroyImageView(m_pDevice->m_device, m_mip[i].m_SRV, NULL);
			vkDestroyImageView(m_pDevice->m_device, m_mip[i].RTV, NULL);
			vkDestroyFramebuffer(m_pDevice->m_device, m_mip[i].frameBuffer, NULL);
		}

		m_result.OnDestroy();
	}

	void DownSamplePS::OnDestroy()
	{
		for (int i = 0; i < DOWNSAMPLEPS_MAX_MIP_LEVELS; i++)
		{
			m_pResourceViewHeaps->FreeDescriptor(m_mip[i].descriptorSet);
		}

		m_downscale.OnDestroy();
		vkDestroyDescriptorSetLayout(m_pDevice->m_device, m_descriptorSetLayout, NULL);
		//vkDestroySampler(m_pDevice->m_device, m_sampler, nullptr);

		vkDestroyRenderPass(m_pDevice->m_device, m_in, NULL);
	}

	void DownSamplePS::Draw(VkCommandBuffer cmd_buf)
	{
		SetPerfMarkerBegin(cmd_buf, "Downsample");

		// downsample
		//
		for (int i = 0; i < m_mipCount; i++)
		{
			VkRenderPassBeginInfo rp_begin = {};
			rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rp_begin.pNext = NULL;
			rp_begin.renderPass = m_in;
			rp_begin.framebuffer = m_mip[i].frameBuffer;
			rp_begin.renderArea.offset.x = 0;
			rp_begin.renderArea.offset.y = 0;
			rp_begin.renderArea.extent.width = m_Width >> (i + 1);
			rp_begin.renderArea.extent.height = m_Height >> (i + 1);
			rp_begin.clearValueCount = 0;
			rp_begin.pClearValues = NULL;
			vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
			SetViewportAndScissor(cmd_buf, 0, 0, m_Width >> (i + 1), m_Height >> (i + 1));

			cbDownscale* data;
			VkDescriptorBufferInfo constantBuffer;
			m_pConstantBufferRing->AllocConstantBuffer(sizeof(cbDownscale), (void**)&data, &constantBuffer);
			data->invWidth = 1.0f / (float)(m_Width >> i);
			data->invHeight = 1.0f / (float)(m_Height >> i);
			data->mipLevel = i;

			m_downscale.Draw(cmd_buf, &constantBuffer, m_mip[i].descriptorSet);

			vkCmdEndRenderPass(cmd_buf);
		}

		SetPerfMarkerEnd(cmd_buf);
	}

	void DownSamplePS::Gui()
	{
		//bool opened = true;
		//ImGui::Begin("DownSamplePS", &opened);

		//for (int i = 0; i < m_mipCount; i++)
		//{
		//	ImGui::Image((ImTextureID)m_mip[i].m_SRV, ImVec2(320 / 2, 180 / 2));
		//}

		//ImGui::End();
	}

	// todo

	static const char* SHADER_FILE_NAME_MAGNIFIER = "MagnifierPS.glsl";
	static const char* SHADER_ENTRY_POINT = "main";
	using cbHandle_t = VkDescriptorBufferInfo;



	void MagnifierPS::OnCreate(Device* pDevice, ResourceViewHeaps* pResourceViewHeaps, DynamicBufferRing* pDynamicBufferRing, StaticBufferPool* pStaticBufferPool, VkFormat outFormat, bool bOutputsToSwapchain /*= false*/)
	{
		m_pDevice = pDevice;
		m_pResourceViewHeaps = pResourceViewHeaps;
		m_pDynamicBufferRing = pDynamicBufferRing;
		m_bOutputsToSwapchain = bOutputsToSwapchain;
		InitializeDescriptorSets();
		//VK_FORMAT_R16G16B16A16_SFLOAT
		m_RenderPass = SimpleColorBlendRenderPass(m_pDevice->GetDevice(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, outFormat);
		CompileShaders(pStaticBufferPool, outFormat); // expects a non-null Render Pass!
	}


	void MagnifierPS::OnDestroy()
	{
		vkDestroyRenderPass(m_pDevice->GetDevice(), m_RenderPass, NULL);
		m_RenderPass = 0;
		DestroyShaders();
		DestroyDescriptorSets();
	}


	void MagnifierPS::OnCreateWindowSizeDependentResources(Texture* pTexture)
	{
		pTexture->CreateSRV(&m_ImageViewSrc);
		update_set(m_ImageViewSrc);

		if (!m_bOutputsToSwapchain)
		{
			VkDevice device = m_pDevice->GetDevice();
			const uint32_t ImgWidth = pTexture->GetWidth();
			const uint32_t ImgHeight = pTexture->GetHeight();

			// create pass output image and its views
			VkImageCreateInfo image_info = {};
			image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			image_info.pNext = NULL;
			image_info.imageType = VK_IMAGE_TYPE_2D;
			image_info.format = pTexture->GetFormat();
			image_info.extent.width = ImgWidth;
			image_info.extent.height = ImgHeight;
			image_info.extent.depth = 1;
			image_info.mipLevels = 1;
			image_info.arrayLayers = 1;
			image_info.samples = VK_SAMPLE_COUNT_1_BIT;
			image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			image_info.queueFamilyIndexCount = 0;
			image_info.pQueueFamilyIndices = NULL;
			image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			image_info.usage = (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
			image_info.flags = 0;
			image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
			m_TexPassOutput.Init(m_pDevice, &image_info, "TexMagnifierOutput");
			m_TexPassOutput.CreateSRV(&m_SRVOutput);
			m_TexPassOutput.CreateRTV(&m_RTVOutput);

			// create framebuffer
			VkImageView attachments[] = { m_RTVOutput };
			VkFramebufferCreateInfo fb_info = {};
			fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fb_info.pNext = NULL;
			fb_info.renderPass = m_RenderPass;
			fb_info.attachmentCount = 1;
			fb_info.pAttachments = attachments;
			fb_info.width = ImgWidth;
			fb_info.height = ImgHeight;
			fb_info.layers = 1;
			VkResult res = vkCreateFramebuffer(device, &fb_info, NULL, &m_FrameBuffer);
			assert(res == VK_SUCCESS);
			SetResourceName(device, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_FrameBuffer, "MagnifierPS");
		}
	}


	void MagnifierPS::OnDestroyWindowSizeDependentResources()
	{
		VkDevice device = m_pDevice->GetDevice();
		vkDestroyImageView(m_pDevice->GetDevice(), m_ImageViewSrc, NULL);
		vkDestroyImageView(device, m_SRVOutput, NULL);
		vkDestroySampler(m_pDevice->GetDevice(), m_SamplerSrc, nullptr);
		if (!m_bOutputsToSwapchain)
		{
			vkDestroyImageView(device, m_RTVOutput, NULL);
			m_TexPassOutput.OnDestroy();
			vkDestroyFramebuffer(device, m_FrameBuffer, NULL);
		}
	}

	void MagnifierPS::CompileShaders(StaticBufferPool* pStaticBufferPool, VkFormat outFormat)
	{
		// Compile Magnifier Pixel Shader
		m_ShaderMagnify.OnCreate(m_pDevice, m_RenderPass, SHADER_FILE_NAME_MAGNIFIER, SHADER_ENTRY_POINT, "", pStaticBufferPool,
			m_pDynamicBufferRing, m_DescriptorSetLayout, NULL, VK_SAMPLE_COUNT_1_BIT);
		return;
	}

	void MagnifierPS::DestroyShaders()
	{
		m_ShaderMagnify.OnDestroy();
	}


	void MagnifierPS::InitializeDescriptorSets()
	{
		std::vector<VkDescriptorSetLayoutBinding> layoutBindingsMagnifier(2);
		layoutBindingsMagnifier[0] = {};
		layoutBindingsMagnifier[0].binding = 0;
		layoutBindingsMagnifier[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBindingsMagnifier[0].descriptorCount = 1;
		layoutBindingsMagnifier[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindingsMagnifier[0].pImmutableSamplers = NULL;

		layoutBindingsMagnifier[1] = {};
		layoutBindingsMagnifier[1].binding = 1;
		layoutBindingsMagnifier[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBindingsMagnifier[1].descriptorCount = 1;
		layoutBindingsMagnifier[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		layoutBindingsMagnifier[1].pImmutableSamplers = NULL;

		m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindingsMagnifier, &m_DescriptorSetLayout, &m_DescriptorSet);
		m_pDynamicBufferRing->SetDescriptorSet(1, sizeof(PassParameters), m_DescriptorSet);
	}
	void MagnifierPS::update_set(VkImageView ImageViewSrc)
	{
		// nearest sampler
		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_NEAREST;
			info.minFilter = VK_FILTER_NEAREST;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
			info.minLod = -1000;
			info.maxLod = 1000;
			info.maxAnisotropy = 1.0f;
			VkResult res = vkCreateSampler(m_pDevice->GetDevice(), &info, NULL, &m_SamplerSrc);
			assert(res == VK_SUCCESS);
		}

		constexpr size_t NUM_WRITES = 1;
		constexpr size_t NUM_IMAGES = 1;

		VkDescriptorImageInfo ImgInfo[NUM_IMAGES] = {};
		VkWriteDescriptorSet  SetWrites[NUM_WRITES] = {};

		for (size_t i = 0; i < NUM_IMAGES; ++i) { ImgInfo[i].sampler = m_SamplerSrc; }
		for (size_t i = 0; i < NUM_WRITES; ++i)
		{
			SetWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			SetWrites[i].dstSet = m_DescriptorSet;
			SetWrites[i].descriptorCount = 1;
			SetWrites[i].pImageInfo = ImgInfo + i;
		}

		SetWrites[0].dstBinding = 0;
		SetWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		ImgInfo[0].imageView = ImageViewSrc;
		ImgInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		vkUpdateDescriptorSets(m_pDevice->GetDevice(), _countof(SetWrites), SetWrites, 0, NULL);
	}

	void MagnifierPS::DestroyDescriptorSets()
	{
		m_pResourceViewHeaps->FreeDescriptor(m_DescriptorSet);
		vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_DescriptorSetLayout, NULL);
	}

	void MagnifierPS::UpdatePipelines(VkRenderPass renderPass)
	{
		m_ShaderMagnify.UpdatePipeline(renderPass, NULL, VK_SAMPLE_COUNT_1_BIT);
	}

	void MagnifierPS::BeginPass(VkCommandBuffer cmd, VkRect2D renderArea)
	{
		VkRenderPassBeginInfo rp_begin = {};
		rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rp_begin.pNext = NULL;
		rp_begin.renderPass = /*pSwapChain ? pSwapChain->GetRenderPass() :*/ m_RenderPass;
		rp_begin.framebuffer = /*pSwapChain ? pSwapChain->GetCurrentFramebuffer() :*/ m_FrameBuffer;
		rp_begin.renderArea = renderArea;
		rp_begin.clearValueCount = 0;
		rp_begin.pClearValues = NULL;
		vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
	}

	void MagnifierPS::EndPass(VkCommandBuffer cmd)
	{
		vkCmdEndRenderPass(cmd);
	}


	void MagnifierPS::Draw(VkCommandBuffer cmd, PassParameters& params /*, SwapChain* pSwapChain  = nullptr*/)
	{
		SetPerfMarkerBegin(cmd, "Magnifier");

		if (m_bOutputsToSwapchain)
		{
			Trace("Warning: MagnifierPS::Draw() is called with NULL swapchain RTV handle, bOutputsToSwapchain was true during MagnifierPS::OnCreate(). No Draw calls will be issued.");
			return;
		}

		cbHandle_t cbHandle = SetConstantBufferData(params);

		VkRect2D renderArea;
		renderArea.offset.x = 0;
		renderArea.offset.y = 0;
		renderArea.extent.width = params.uImageWidth;
		renderArea.extent.height = params.uImageHeight;
		BeginPass(cmd, renderArea);

		m_ShaderMagnify.Draw(cmd, &cbHandle, m_DescriptorSet);
		EndPass(cmd);
		SetPerfMarkerEnd(cmd);
	}


	VkDescriptorBufferInfo MagnifierPS::SetConstantBufferData(PassParameters& params)
	{
		KeepMagnifierOnScreen(params);

		// memcpy to cbuffer
		cbHandle_t cbHandle = {};
		uint32_t* pConstMem = nullptr;
		const uint32_t szConstBuff = sizeof(PassParameters);
		void const* pConstBufSrc = static_cast<void const*>(&params);
		m_pDynamicBufferRing->AllocConstantBuffer(szConstBuff, reinterpret_cast<void**>(&pConstMem), &cbHandle);
		memcpy(pConstMem, pConstBufSrc, szConstBuff);

		return cbHandle;
	}

	void MagnifierPS::KeepMagnifierOnScreen(PassParameters& params)
	{
		const int IMAGE_SIZE[2] = { static_cast<int>(params.uImageWidth), static_cast<int>(params.uImageHeight) };
		const int& W = IMAGE_SIZE[0];
		const int& H = IMAGE_SIZE[1];

		const int radiusInPixelsMagifier = static_cast<int>(params.fMagnifierScreenRadius * H);
		const int radiusInPixelsMagifiedArea = static_cast<int>(params.fMagnifierScreenRadius * H / params.fMagnificationAmount);

		const bool bCirclesAreOverlapping = radiusInPixelsMagifiedArea + radiusInPixelsMagifier > std::sqrt(params.iMagnifierOffset[0] * params.iMagnifierOffset[0] + params.iMagnifierOffset[1] * params.iMagnifierOffset[1]);


		if (bCirclesAreOverlapping) // don't let the two circles overlap
		{
			params.iMagnifierOffset[0] = radiusInPixelsMagifiedArea + radiusInPixelsMagifier + 1;
			params.iMagnifierOffset[1] = radiusInPixelsMagifiedArea + radiusInPixelsMagifier + 1;
		}

		for (int i = 0; i < 2; ++i) // try to move the magnified area to be fully on screen, if possible
		{
			const bool bMagnifierOutOfScreenRegion = params.iMousePos[i] + params.iMagnifierOffset[i] + radiusInPixelsMagifier > IMAGE_SIZE[i]
				|| params.iMousePos[i] + params.iMagnifierOffset[i] - radiusInPixelsMagifier < 0;
			if (bMagnifierOutOfScreenRegion)
			{
				if (!(params.iMousePos[i] - params.iMagnifierOffset[i] + radiusInPixelsMagifier > IMAGE_SIZE[i]
					|| params.iMousePos[i] - params.iMagnifierOffset[i] - radiusInPixelsMagifier < 0))
				{
					// flip offset if possible
					params.iMagnifierOffset[i] = -params.iMagnifierOffset[i];
				}
				else
				{
					// otherwise clamp
					if (params.iMousePos[i] + params.iMagnifierOffset[i] + radiusInPixelsMagifier > IMAGE_SIZE[i])
						params.iMagnifierOffset[i] = IMAGE_SIZE[i] - params.iMousePos[i] - radiusInPixelsMagifier;
					if (params.iMousePos[i] + params.iMagnifierOffset[i] - radiusInPixelsMagifier < 0)
						params.iMagnifierOffset[i] = -params.iMousePos[i] + radiusInPixelsMagifier;
				}
			}
		}
	}


	// todo



}
//!vkr

// todo 通用函数 
namespace vkr {

#ifdef USE_WIC
#include "wincodec.h"
	static IWICImagingFactory* m_pWICFactory = NULL;
#endif

	WICLoader::~WICLoader()
	{
		free(m_pData);
	}

	bool WICLoader::Load(const char* pFilename, float cutOff, IMG_INFO* pInfo)
	{
#ifdef USE_WIC
		HRESULT hr = S_OK;

		if (m_pWICFactory == NULL)
		{
			hr = CoInitialize(NULL);
			assert(SUCCEEDED(hr));
			hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pWICFactory));
			assert(SUCCEEDED(hr));
		}

		IWICStream* pWicStream;
		hr = m_pWICFactory->CreateStream(&pWicStream);
		assert(SUCCEEDED(hr));

		wchar_t  uniName[1024];
		swprintf(uniName, 1024, L"%S", pFilename);
		hr = pWicStream->InitializeFromFilename(uniName, GENERIC_READ);
		//assert(hr == S_OK);
		if (FAILED(hr))
			return false;

		IWICBitmapDecoder* pBitmapDecoder;
		hr = m_pWICFactory->CreateDecoderFromStream(pWicStream, NULL, WICDecodeMetadataCacheOnDemand, &pBitmapDecoder);
		assert(SUCCEEDED(hr));

		IWICBitmapFrameDecode* pFrameDecode;
		hr = pBitmapDecoder->GetFrame(0, &pFrameDecode);
		assert(SUCCEEDED(hr));

		IWICFormatConverter* pIFormatConverter;
		hr = m_pWICFactory->CreateFormatConverter(&pIFormatConverter);
		assert(SUCCEEDED(hr));

		hr = pIFormatConverter->Initialize(pFrameDecode, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, NULL, 100.0, WICBitmapPaletteTypeCustom);
		assert(SUCCEEDED(hr));

		uint32_t width, height;
		pFrameDecode->GetSize(&width, &height);
		pFrameDecode->Release();

		int bufferSize = width * height * 4;
		m_pData = (char*)malloc(bufferSize);
		hr = pIFormatConverter->CopyPixels(NULL, width * 4, bufferSize, (BYTE*)m_pData);
		assert(SUCCEEDED(hr));
#else
		int32_t width, height, channels;
		m_pData = (char*)stbi_load(pFilename, &width, &height, &channels, STBI_rgb_alpha);
#endif

		// compute number of mips
		//
		uint32_t mipWidth = width;
		uint32_t mipHeight = height;
		uint32_t mipCount = 0;
		for (;;)
		{
			mipCount++;
			if (mipWidth > 1) mipWidth >>= 1;
			if (mipHeight > 1) mipHeight >>= 1;
			if (mipWidth == 1 && mipHeight == 1)
				break;
		}

		// fill img struct
		//
		pInfo->arraySize = 1;
		pInfo->width = width;
		pInfo->height = height;
		pInfo->depth = 1;
		pInfo->mipMapCount = mipCount;
		pInfo->bitCount = 32;
		pInfo->format = DXGI_FORMAT_R8G8B8A8_UNORM;

		// if there is a cut off, compute the alpha test coverage of the top mip
		// mip generation will try to match this value so objects dont get thinner
		// as they use lower mips
		m_cutOff = cutOff;
		if (m_cutOff < 1.0f)
			m_alphaTestCoverage = GetAlphaCoverage(width, height, 1.0f, (int)(255 * m_cutOff));
		else
			m_alphaTestCoverage = 1.0f;

#ifdef USE_WIC
		pIFormatConverter->Release();
		pBitmapDecoder->Release();
		pWicStream->Release();
#endif

		return true;
	}

	void WICLoader::CopyPixels(void* pDest, uint32_t stride, uint32_t bytesWidth, uint32_t height)
	{
		for (uint32_t y = 0; y < height; y++)
		{
			memcpy((char*)pDest + y * stride, m_pData + y * bytesWidth, bytesWidth);
		}

		MipImage(bytesWidth / 4, height);
	}

	float WICLoader::GetAlphaCoverage(uint32_t width, uint32_t height, float scale, int cutoff) const
	{
		double val = 0;

		uint32_t* pImg = (uint32_t*)m_pData;

		for (uint32_t y = 0; y < height; y++)
		{
			for (uint32_t x = 0; x < width; x++)
			{
				uint8_t* pPixel = (uint8_t*)pImg++;

				int alpha = (int)(scale * (float)pPixel[3]);
				if (alpha > 255) alpha = 255;
				if (alpha <= cutoff)
					continue;

				val += alpha;
			}
		}

		return (float)(val / (height * width * 255));
	}

	void WICLoader::ScaleAlpha(uint32_t width, uint32_t height, float scale)
	{
		uint32_t* pImg = (uint32_t*)m_pData;

		for (uint32_t y = 0; y < height; y++)
		{
			for (uint32_t x = 0; x < width; x++)
			{
				uint8_t* pPixel = (uint8_t*)pImg++;

				int alpha = (int)(scale * (float)pPixel[3]);
				if (alpha > 255) alpha = 255;

				pPixel[3] = alpha;
			}
		}
	}

	void WICLoader::MipImage(uint32_t width, uint32_t height)
	{
		//compute mip so next call gets the lower mip
		int offsetsX[] = { 0,1,0,1 };
		int offsetsY[] = { 0,0,1,1 };

		uint32_t* pImg = (uint32_t*)m_pData;

#define GetByte(color, component) (((color) >> (8 * (component))) & 0xff)
#define GetColor(ptr, x,y) (ptr[(x)+(y)*width])
#define SetColor(ptr, x,y, col) ptr[(x)+(y)*width/2]=col;

		for (uint32_t y = 0; y < height; y += 2)
		{
			for (uint32_t x = 0; x < width; x += 2)
			{
				uint32_t ccc = 0;
				for (uint32_t c = 0; c < 4; c++)
				{
					uint32_t cc = 0;
					for (uint32_t i = 0; i < 4; i++)
						cc += GetByte(GetColor(pImg, x + offsetsX[i], y + offsetsY[i]), 3 - c);

					ccc = (ccc << 8) | (cc / 4);
				}
				SetColor(pImg, x / 2, y / 2, ccc);
			}
		}


		// For cutouts we need to scale the alpha channel to match the coverage of the top MIP map
		// otherwise cutouts seem to get thinner when smaller mips are used
		// Credits: http://www.ludicon.com/castano/blog/articles/computing-alpha-mipmaps/    
		if (m_alphaTestCoverage < 1.0)
		{
			float ini = 0;
			float fin = 10;
			float mid;
			float alphaPercentage;
			int iter = 0;
			for (; iter < 50; iter++)
			{
				mid = (ini + fin) / 2;
				alphaPercentage = GetAlphaCoverage(width / 2, height / 2, mid, (int)(m_cutOff * 255));

				if (fabs(alphaPercentage - m_alphaTestCoverage) < .001)
					break;

				if (alphaPercentage > m_alphaTestCoverage)
					fin = mid;
				if (alphaPercentage < m_alphaTestCoverage)
					ini = mid;
			}
			ScaleAlpha(width / 2, height / 2, mid);
			//Trace(format("(%4i x %4i), %f, %f, %i\n", width, height, alphaPercentage, 1.0f, 0));       
		}

	}

	struct DDS_PIXELFORMAT
	{
		UINT32 size;
		UINT32 flags;
		UINT32 fourCC;
		UINT32 bitCount;
		UINT32 bitMaskR;
		UINT32 bitMaskG;
		UINT32 bitMaskB;
		UINT32 bitMaskA;
	};

	struct DDS_HEADER
	{

		UINT32       dwSize;
		UINT32       dwHeaderFlags;
		UINT32       dwHeight;
		UINT32       dwWidth;
		UINT32       dwPitchOrLinearSize;
		UINT32       dwDepth;
		UINT32       dwMipMapCount;
		UINT32       dwReserved1[11];
		DDS_PIXELFORMAT ddspf;
		UINT32       dwSurfaceFlags;
		UINT32       dwCubemapFlags;
		UINT32       dwCaps3;
		UINT32       dwCaps4;
		UINT32       dwReserved2;
	};

	//--------------------------------------------------------------------------------------
	// retrieve the GetDxgiFormat from a DDS_PIXELFORMAT
	//--------------------------------------------------------------------------------------
	static DXGI_FORMAT GetDxgiFormat(DDS_PIXELFORMAT pixelFmt)
	{
		if (pixelFmt.flags & 0x00000004)   //DDPF_FOURCC
		{
			// Check for D3DFORMAT enums being set here
			switch (pixelFmt.fourCC)
			{
			case '1TXD':         return DXGI_FORMAT_BC1_UNORM;
			case '3TXD':         return DXGI_FORMAT_BC2_UNORM;
			case '5TXD':         return DXGI_FORMAT_BC3_UNORM;
			case 'U4CB':         return DXGI_FORMAT_BC4_UNORM;
			case 'A4CB':         return DXGI_FORMAT_BC4_SNORM;
			case '2ITA':         return DXGI_FORMAT_BC5_UNORM;
			case 'S5CB':         return DXGI_FORMAT_BC5_SNORM;
			case 'GBGR':         return DXGI_FORMAT_R8G8_B8G8_UNORM;
			case 'BGRG':         return DXGI_FORMAT_G8R8_G8B8_UNORM;
			case 36:             return DXGI_FORMAT_R16G16B16A16_UNORM;
			case 110:            return DXGI_FORMAT_R16G16B16A16_SNORM;
			case 111:            return DXGI_FORMAT_R16_FLOAT;
			case 112:            return DXGI_FORMAT_R16G16_FLOAT;
			case 113:            return DXGI_FORMAT_R16G16B16A16_FLOAT;
			case 114:            return DXGI_FORMAT_R32_FLOAT;
			case 115:            return DXGI_FORMAT_R32G32_FLOAT;
			case 116:            return DXGI_FORMAT_R32G32B32A32_FLOAT;
			default:             return DXGI_FORMAT_UNKNOWN;
			}
		}
		else
		{
			switch (pixelFmt.bitMaskR)
			{
			case 0xff:        return DXGI_FORMAT_R8G8B8A8_UNORM;
			case 0x00ff0000:  return DXGI_FORMAT_B8G8R8A8_UNORM;
			case 0xffff:      return DXGI_FORMAT_R16G16_UNORM;
			case 0x3ff:       return DXGI_FORMAT_R10G10B10A2_UNORM;
			case 0x7c00:      return DXGI_FORMAT_B5G5R5A1_UNORM;
			case 0xf800:      return DXGI_FORMAT_B5G6R5_UNORM;
			case 0:           return DXGI_FORMAT_A8_UNORM;
			default:          return DXGI_FORMAT_UNKNOWN;
			};
		}
	}

	DDSLoader::~DDSLoader()
	{
		if (m_handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_handle);
		}
	}

	bool DDSLoader::Load(const char* pFilename, float cutOff, IMG_INFO* pInfo)
	{
		typedef enum RESOURCE_DIMENSION
		{
			RESOURCE_DIMENSION_UNKNOWN = 0,
			RESOURCE_DIMENSION_BUFFER = 1,
			RESOURCE_DIMENSION_TEXTURE1D = 2,
			RESOURCE_DIMENSION_TEXTURE2D = 3,
			RESOURCE_DIMENSION_TEXTURE3D = 4
		} RESOURCE_DIMENSION;

		typedef struct
		{
			DXGI_FORMAT      dxgiFormat;
			RESOURCE_DIMENSION  resourceDimension;
			UINT32           miscFlag;
			UINT32           arraySize;
			UINT32           reserved;
		} DDS_HEADER_DXT10;

		if (GetFileAttributesA(pFilename) == 0xFFFFFFFF)
			return false;

		m_handle = CreateFileA(pFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (m_handle == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		LARGE_INTEGER largeFileSize;
		GetFileSizeEx(m_handle, &largeFileSize);
		assert(0 == largeFileSize.HighPart);
		UINT32 fileSize = largeFileSize.LowPart;
		UINT32 rawTextureSize = fileSize;

		// read the header
		char headerData[4 + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10)];
		DWORD dwBytesRead = 0;
		if (::ReadFile(m_handle, headerData, 4 + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10), &dwBytesRead, NULL))
		{
			char* pByteData = headerData;
			UINT32 dwMagic = *reinterpret_cast<UINT32*>(pByteData);
			if (dwMagic != ' SDD')   // "DDS "
			{
				return false;
			}

			pByteData += 4;
			rawTextureSize -= 4;

			DDS_HEADER* header = reinterpret_cast<DDS_HEADER*>(pByteData);
			pByteData += sizeof(DDS_HEADER);
			rawTextureSize -= sizeof(DDS_HEADER);

			pInfo->width = header->dwWidth;
			pInfo->height = header->dwHeight;
			pInfo->depth = header->dwDepth ? header->dwDepth : 1;
			pInfo->mipMapCount = header->dwMipMapCount ? header->dwMipMapCount : 1;

			if (header->ddspf.fourCC == '01XD')
			{
				DDS_HEADER_DXT10* header10 = reinterpret_cast<DDS_HEADER_DXT10*>((char*)header + sizeof(DDS_HEADER));
				rawTextureSize -= sizeof(DDS_HEADER_DXT10);

				pInfo->arraySize = header10->arraySize;
				pInfo->format = header10->dxgiFormat;
				pInfo->bitCount = header->ddspf.bitCount;
			}
			else
			{
				pInfo->arraySize = (header->dwCubemapFlags == 0xfe00) ? 6 : 1;
				pInfo->format = GetDxgiFormat(header->ddspf);
				pInfo->bitCount = (UINT32)BitsPerPixel(pInfo->format);
			}
		}

		SetFilePointer(m_handle, fileSize - rawTextureSize, 0, FILE_BEGIN);
		return true;
	}

	void DDSLoader::CopyPixels(void* pDest, uint32_t stride, uint32_t bytesWidth, uint32_t height)
	{
		assert(m_handle != INVALID_HANDLE_VALUE);
		for (uint32_t y = 0; y < height; y++)
		{
			::ReadFile(m_handle, (char*)pDest + y * stride, bytesWidth, NULL, NULL);
		}
	}

	ImgLoader* CreateImageLoader(const char* pFilename)
	{
		// get the last 4 char (assuming this is the file extension)
		size_t len = strlen(pFilename);
		const char* ext = pFilename + (len - 4);
		if (_stricmp(ext, ".dds") == 0)
		{
			return new DDSLoader();
		}
		else
		{
			return new WICLoader();
		}
	}
#if 0
	Async::Async(std::function<void()> job, Sync* pSync) :
		m_job{ job },
		m_pSync{ pSync }
	{
		if (m_pSync)
			m_pSync->Inc();

		{
			std::unique_lock<std::mutex> lock(s_mutex);

			while (s_activeThreads >= s_maxThreads)
			{
				s_condition.wait(lock);
			}

			s_activeThreads++;
		}

		m_pThread = new std::thread([this]()
			{
				m_job();

				{
					std::lock_guard<std::mutex> lock(s_mutex);
					s_activeThreads--;
				}

				s_condition.notify_one();

				if (m_pSync)
					m_pSync->Dec();
			});
	}

	Async::~Async()
	{
		m_pThread->join();
		delete m_pThread;
	}

	void Async::Wait(Sync* pSync)
	{
		if (pSync->Get() == 0)
			return;

		{
			std::lock_guard <std::mutex> lock(s_mutex);
			s_activeThreads--;
		}

		s_condition.notify_one();

		pSync->Wait();

		{
			std::unique_lock<std::mutex> lock(s_mutex);

			s_condition.wait(lock, []
				{
					return s_bExiting || (s_activeThreads < s_maxThreads);
				});

			s_activeThreads++;
		}
	}

	//
	// Basic async pool
	//

	AsyncPool::~AsyncPool()
	{
		Flush();
	}

	void AsyncPool::Flush()
	{
		for (int i = 0; i < m_pool.size(); i++)
			delete m_pool[i];
		m_pool.clear();
	}

	void AsyncPool::AddAsyncTask(std::function<void()> job, Sync* pSync)
	{
		m_pool.push_back(new Async(job, pSync));
	}

	//
	// ExecAsyncIfThereIsAPool, will use async if there is a pool, otherwise will run the task synchronously
	void ExecAsyncIfThereIsAPool(AsyncPool* pAsyncPool, std::function<void()> job)
	{
		// use MT if there is a pool
		if (pAsyncPool != NULL)
		{
			pAsyncPool->AddAsyncTask(job);
		}
		else
		{
			job();
		}
	}

	//
	// Some static functions
	//
	int Async::s_activeThreads = 0;
	std::mutex Async::s_mutex;
	std::condition_variable Async::s_condition;
	bool Async::s_bExiting = false;
	int Async::s_maxThreads = std::thread::hardware_concurrency();
#endif
	//
	// Compute a hash of an array
	//
	size_t Hash(const void* ptr, size_t size, size_t result)
	{
		for (size_t i = 0; i < size; ++i)
		{
			result = (result * 16777619) ^ ((char*)ptr)[i];
		}

		return result;
	}

	size_t Hash_p(const size_t* ptr, size_t size, size_t result)
	{
		auto n = size / sizeof(size_t);
		auto m = size % sizeof(size_t);
		for (size_t i = 0; i < n; ++i)
		{
			result = (result * 16777619) ^ ptr[i];
		}
		auto p = ptr + n;
		for (size_t i = 0; i < m; ++i)
		{
			result = (result * 16777619) ^ ((char*)p)[i];
		}
		return result;
	}

	size_t HashString(const char* str, size_t result)
	{
		return Hash(str, strlen(str), result);
	}

	size_t HashString(const std::string& str, size_t result)
	{
		return HashString(str.c_str(), result);
	}

	size_t HashInt(const int type, size_t result) { return Hash(&type, sizeof(int), result); }
	size_t HashFloat(const float type, size_t result) { return Hash(&type, sizeof(float), result); }
	size_t HashPtr(const void* type, size_t result) { return Hash(&type, sizeof(void*), result); }


	// this is when you need to clear the attachment, for example when you are not rendering the full screen.
	//
	void AttachClearBeforeUse(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription* pAttachDesc)
	{
		pAttachDesc->format = format;
		pAttachDesc->samples = sampleCount;
		pAttachDesc->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		pAttachDesc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		pAttachDesc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		pAttachDesc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		pAttachDesc->initialLayout = initialLayout;
		pAttachDesc->finalLayout = finalLayout;
		pAttachDesc->flags = 0;
	}

	// No clear, attachment will keep data that was not written (if this is the first pass make sure you are filling the whole screen)
	void AttachNoClearBeforeUse(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription* pAttachDesc)
	{
		pAttachDesc->format = format;
		pAttachDesc->samples = sampleCount;
		pAttachDesc->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		pAttachDesc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		pAttachDesc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		pAttachDesc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		pAttachDesc->initialLayout = initialLayout;
		pAttachDesc->finalLayout = finalLayout;
		pAttachDesc->flags = 0;
	}

	// Attanchment where we will be using alpha blending, this means we care about the previous contents
	void AttachBlending(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription* pAttachDesc)
	{
		pAttachDesc->format = format;
		pAttachDesc->samples = sampleCount;
		pAttachDesc->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		pAttachDesc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		pAttachDesc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		pAttachDesc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		pAttachDesc->initialLayout = initialLayout;
		pAttachDesc->finalLayout = finalLayout;
		pAttachDesc->flags = 0;
	}


	VkRenderPass CreateRenderPassOptimal(VkDevice device, uint32_t colorAttachments, VkAttachmentDescription* pColorAttachments, VkAttachmentDescription* pDepthAttachment)
	{
		// we need to put all the color and the depth attachments in the same buffer
		//
		VkAttachmentDescription attachments[16];
		assert(colorAttachments < 16); // make sure we don't overflow the scratch buffer above

		memcpy(attachments, pColorAttachments, sizeof(VkAttachmentDescription) * colorAttachments);
		if (pDepthAttachment != NULL)
			memcpy(&attachments[colorAttachments], pDepthAttachment, sizeof(VkAttachmentDescription));

		//create references for the attachments
		//
		VkAttachmentReference color_reference[10];
		for (uint32_t i = 0; i < colorAttachments; i++)
			color_reference[i] = { i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		VkAttachmentReference depth_reference = { colorAttachments, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		// Create subpass
		//
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.flags = 0;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = NULL;
		subpass.colorAttachmentCount = colorAttachments;
		subpass.pColorAttachments = color_reference;
		subpass.pResolveAttachments = NULL;
		subpass.pDepthStencilAttachment = (pDepthAttachment) ? &depth_reference : NULL;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = NULL;

		VkSubpassDependency dep = {};
		dep.dependencyFlags = 0;
		dep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT |
			((colorAttachments) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
			((pDepthAttachment) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);
		dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
			((colorAttachments) ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : 0) |
			((pDepthAttachment) ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : 0);
		dep.dstSubpass = VK_SUBPASS_EXTERNAL;
		dep.srcAccessMask = ((colorAttachments) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
			((pDepthAttachment) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);
		dep.srcStageMask = ((colorAttachments) ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : 0) |
			((pDepthAttachment) ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : 0);
		dep.srcSubpass = 0;

		// Create render pass
		//
		VkRenderPassCreateInfo rp_info = {};
		rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rp_info.pNext = NULL;
		rp_info.attachmentCount = colorAttachments;
		if (pDepthAttachment != NULL)
			rp_info.attachmentCount++;
		rp_info.pAttachments = attachments;
		rp_info.subpassCount = 1;
		rp_info.pSubpasses = &subpass;
		rp_info.dependencyCount = 1;
		rp_info.pDependencies = &dep;

		VkRenderPass render_pass;
		VkResult res = vkCreateRenderPass(device, &rp_info, NULL, &render_pass);
		assert(res == VK_SUCCESS);

		SetResourceName(device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)render_pass, "CreateRenderPassOptimal");

		return render_pass;
	}

	VkRenderPass SimpleColorWriteRenderPass(VkDevice device, VkImageLayout initialLayout, VkImageLayout passLayout, VkImageLayout finalLayout, uint32_t format)
	{
		// color RT
		VkAttachmentDescription attachments[1];
		attachments[0].format = (VkFormat)format;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // we don't care about the previous contents, this is for a full screen pass with no blending
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = initialLayout;
		attachments[0].finalLayout = finalLayout;
		attachments[0].flags = 0;

		VkAttachmentReference color_reference = { 0, passLayout };

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.flags = 0;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = NULL;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_reference;
		subpass.pResolveAttachments = NULL;
		subpass.pDepthStencilAttachment = NULL;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = NULL;

		VkSubpassDependency dep = {};
		dep.dependencyFlags = 0;
		dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dep.dstSubpass = VK_SUBPASS_EXTERNAL;
		dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dep.srcSubpass = 0;

		VkRenderPassCreateInfo rp_info = {};
		rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rp_info.pNext = NULL;
		rp_info.attachmentCount = 1;
		rp_info.pAttachments = attachments;
		rp_info.subpassCount = 1;
		rp_info.pSubpasses = &subpass;
		rp_info.dependencyCount = 1;
		rp_info.pDependencies = &dep;

		VkRenderPass renderPass;
		VkResult res = vkCreateRenderPass(device, &rp_info, NULL, &renderPass);
		assert(res == VK_SUCCESS);

		SetResourceName(device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)renderPass, "SimpleColorWriteRenderPass");

		return renderPass;
	}

	VkRenderPass SimpleColorBlendRenderPass(VkDevice device, VkImageLayout initialLayout, VkImageLayout passLayout, VkImageLayout finalLayout, uint32_t format)
	{
		// color RT
		VkAttachmentDescription attachments[1];
		attachments[0].format = (VkFormat)format;// 
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = initialLayout;
		attachments[0].finalLayout = finalLayout;
		attachments[0].flags = 0;

		VkAttachmentReference color_reference = { 0, passLayout };

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.flags = 0;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = NULL;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_reference;
		subpass.pResolveAttachments = NULL;
		subpass.pDepthStencilAttachment = NULL;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = NULL;

		VkSubpassDependency dep = {};
		dep.dependencyFlags = 0;
		dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dep.dstSubpass = VK_SUBPASS_EXTERNAL;
		dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dep.srcSubpass = 0;

		VkRenderPassCreateInfo rp_info = {};
		rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rp_info.pNext = NULL;
		rp_info.attachmentCount = 1;
		rp_info.pAttachments = attachments;
		rp_info.subpassCount = 1;
		rp_info.pSubpasses = &subpass;
		rp_info.dependencyCount = 1;
		rp_info.pDependencies = &dep;

		VkRenderPass renderPass;
		VkResult res = vkCreateRenderPass(device, &rp_info, NULL, &renderPass);
		assert(res == VK_SUCCESS);

		SetResourceName(device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)renderPass, "SimpleColorBlendRenderPass");

		return renderPass;
	}


	void SetViewportAndScissor(VkCommandBuffer cmd_buf, uint32_t topX, uint32_t topY, uint32_t width, uint32_t height)
	{
		VkViewport viewport;
		viewport.x = static_cast<float>(topX);
		viewport.y = static_cast<float>(topY) + static_cast<float>(height);
		viewport.width = static_cast<float>(width);
		viewport.height = -static_cast<float>(height);
		viewport.minDepth = (float)0.0f;
		viewport.maxDepth = (float)1.0f;
		vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

		VkRect2D   scissor;
		scissor.extent.width = (uint32_t)(width);
		scissor.extent.height = (uint32_t)(height);
		scissor.offset.x = topX;
		scissor.offset.y = topY;
		vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
	}


	std::vector<VkDynamicState> GetDynamicStatesForPipeline()
	{
		std::vector<VkDynamicState> dynamicStates =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE,
			VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
			VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
			VK_DYNAMIC_STATE_STENCIL_REFERENCE,
			VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
			VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
		};
		return dynamicStates;
	}

	//VK_STENCIL_FACE_FRONT_BIT = 0x00000001,
	//VK_STENCIL_FACE_BACK_BIT = 0x00000002,
	//VK_STENCIL_FACE_FRONT_AND_BACK = 0x00000003,
	struct ds_t
	{
		VkStencilFaceFlags faceMask;
		uint32_t compareMask, writeMask, reference;
		VkBool32 depthTestEnable;
		VkBool32 depthWriteEnable;
		VkBool32 stencilTestEnable;
	};
	void set_ds(VkCommandBuffer c, ds_t* t)
	{
		vkCmdSetDepthTestEnable(c, t->depthTestEnable);
		vkCmdSetDepthWriteEnable(c, t->depthWriteEnable);
		vkCmdSetStencilTestEnable(c, t->stencilTestEnable);
		vkCmdSetStencilCompareMask(c, t->faceMask, t->compareMask);
		vkCmdSetStencilWriteMask(c, t->faceMask, t->writeMask);
		vkCmdSetStencilReference(c, t->faceMask, t->reference);
	}

	void BeginRenderPass(VkCommandBuffer commandList, VkRenderPass renderPass, VkFramebuffer frameBuffer, const std::vector<VkClearValue>* pClearValues, uint32_t Width, uint32_t Height)
	{
		VkRenderPassBeginInfo rp_begin;
		rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rp_begin.pNext = NULL;
		rp_begin.renderPass = renderPass;
		rp_begin.framebuffer = frameBuffer;
		rp_begin.renderArea.offset.x = 0;
		rp_begin.renderArea.offset.y = 0;
		rp_begin.renderArea.extent.width = Width;
		rp_begin.renderArea.extent.height = Height;
		rp_begin.pClearValues = pClearValues->data();
		rp_begin.clearValueCount = (uint32_t)pClearValues->size();
		vkCmdBeginRenderPass(commandList, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
	}
#if 1
	static PFN_vkSetDebugUtilsObjectNameEXT     s_vkSetDebugUtilsObjectName = nullptr;
	static PFN_vkCmdBeginDebugUtilsLabelEXT     s_vkCmdBeginDebugUtilsLabel = nullptr;
	static PFN_vkCmdEndDebugUtilsLabelEXT       s_vkCmdEndDebugUtilsLabel = nullptr;
	static bool s_bCanUseDebugUtils = false;
	static std::mutex s_mutex;

	bool f_ExtDebugUtilsCheckInstanceExtensions(InstanceProperties* pDP)
	{
		s_bCanUseDebugUtils = pDP->AddInstanceExtensionName("VK_EXT_debug_utils");
		return s_bCanUseDebugUtils;
	}

	//
	//
	void ExtDebugUtilsGetProcAddresses(VkDevice device)
	{
		if (s_bCanUseDebugUtils)
		{
			s_vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
			s_vkCmdBeginDebugUtilsLabel = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
			s_vkCmdEndDebugUtilsLabel = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
		}
	}

	void SetResourceName(VkDevice device, VkObjectType objectType, uint64_t handle, const char* name)
	{
		if (s_vkSetDebugUtilsObjectName && handle && name)
		{
			std::unique_lock<std::mutex> lock(s_mutex);

			VkDebugUtilsObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = objectType;
			nameInfo.objectHandle = handle;
			nameInfo.pObjectName = name;
			s_vkSetDebugUtilsObjectName(device, &nameInfo);
		}
	}

	void SetPerfMarkerBegin(VkCommandBuffer cmd_buf, const char* name)
	{
		if (s_vkCmdBeginDebugUtilsLabel)
		{
			VkDebugUtilsLabelEXT label = {};
			label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			label.pLabelName = name;
			const float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
			memcpy(label.color, color, sizeof(color));
			s_vkCmdBeginDebugUtilsLabel(cmd_buf, &label);
		}
	}

	void SetPerfMarkerEnd(VkCommandBuffer cmd_buf)
	{
		if (s_vkCmdEndDebugUtilsLabel)
		{
			s_vkCmdEndDebugUtilsLabel(cmd_buf);
		}
	}
#endif
	// gltf信息
#if 1

	VkFormat GetFormat(const std::string& str, int id)
	{
		if (str == "SCALAR")
		{
			switch (id)
			{
			case 5120: return VK_FORMAT_R8_SINT; //(BYTE)
			case 5121: return VK_FORMAT_R8_UINT; //(UNSIGNED_BYTE)1
			case 5122: return VK_FORMAT_R16_SINT; //(SHORT)2
			case 5123: return VK_FORMAT_R16_UINT; //(UNSIGNED_SHORT)2
			case 5124: return VK_FORMAT_R32_SINT; //(SIGNED_INT)4
			case 5125: return VK_FORMAT_R32_UINT; //(UNSIGNED_INT)4
			case 5126: return VK_FORMAT_R32_SFLOAT; //(FLOAT)
			}
		}
		else if (str == "VEC2")
		{
			switch (id)
			{
			case 5120: return VK_FORMAT_R8G8_SINT; //(BYTE)
			case 5121: return VK_FORMAT_R8G8_UINT; //(UNSIGNED_BYTE)1
			case 5122: return VK_FORMAT_R16G16_SINT; //(SHORT)2
			case 5123: return VK_FORMAT_R16G16_UINT; //(UNSIGNED_SHORT)2
			case 5124: return VK_FORMAT_R32G32_SINT; //(SIGNED_INT)4
			case 5125: return VK_FORMAT_R32G32_UINT; //(UNSIGNED_INT)4
			case 5126: return VK_FORMAT_R32G32_SFLOAT; //(FLOAT)
			}
		}
		else if (str == "VEC3")
		{
			switch (id)
			{
			case 5120: return VK_FORMAT_UNDEFINED; //(BYTE)
			case 5121: return VK_FORMAT_UNDEFINED; //(UNSIGNED_BYTE)1
			case 5122: return VK_FORMAT_UNDEFINED; //(SHORT)2
			case 5123: return VK_FORMAT_UNDEFINED; //(UNSIGNED_SHORT)2
			case 5124: return VK_FORMAT_R32G32B32_SINT; //(SIGNED_INT)4
			case 5125: return VK_FORMAT_R32G32B32_UINT; //(UNSIGNED_INT)4
			case 5126: return VK_FORMAT_R32G32B32_SFLOAT; //(FLOAT)
			}
		}
		else if (str == "VEC4")
		{
			switch (id)
			{
			case 5120: return VK_FORMAT_R8G8B8A8_SINT; //(BYTE)
			case 5121: return VK_FORMAT_R8G8B8A8_UINT; //(UNSIGNED_BYTE)1
			case 5122: return VK_FORMAT_R16G16B16A16_SINT; //(SHORT)2
			case 5123: return VK_FORMAT_R16G16B16A16_UINT; //(UNSIGNED_SHORT)2
			case 5124: return VK_FORMAT_R32G32B32A32_SINT; //(SIGNED_INT)4
			case 5125: return VK_FORMAT_R32G32B32A32_UINT; //(UNSIGNED_INT)4
			case 5126: return VK_FORMAT_R32G32B32A32_SFLOAT; //(FLOAT)
			}
		}

		return VK_FORMAT_UNDEFINED;
	}

	uint32_t SizeOfFormat(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R8_SINT: return 1;//(BYTE)
		case VK_FORMAT_R8_UINT: return 1;//(UNSIGNED_BYTE)1
		case VK_FORMAT_R16_SINT: return 2;//(SHORT)2
		case VK_FORMAT_R16_UINT: return 2;//(UNSIGNED_SHORT)2
		case VK_FORMAT_R32_SINT: return 4;//(SIGNED_INT)4
		case VK_FORMAT_R32_UINT: return 4;//(UNSIGNED_INT)4
		case VK_FORMAT_R32_SFLOAT: return 4;//(FLOAT)

		case VK_FORMAT_R8G8_SINT: return 2 * 1;//(BYTE)
		case VK_FORMAT_R8G8_UINT: return 2 * 1;//(UNSIGNED_BYTE)1
		case VK_FORMAT_R16G16_SINT: return 2 * 2;//(SHORT)2
		case VK_FORMAT_R16G16_UINT: return 2 * 2; // (UNSIGNED_SHORT)2
		case VK_FORMAT_R32G32_SINT: return 2 * 4;//(SIGNED_INT)4
		case VK_FORMAT_R32G32_UINT: return 2 * 4;//(UNSIGNED_INT)4
		case VK_FORMAT_R32G32_SFLOAT: return 2 * 4;//(FLOAT)

		case VK_FORMAT_UNDEFINED: return 0;//(BYTE) (UNSIGNED_BYTE) (SHORT) (UNSIGNED_SHORT)
		case VK_FORMAT_R32G32B32_SINT: return 3 * 4;//(SIGNED_INT)4
		case VK_FORMAT_R32G32B32_UINT: return 3 * 4;//(UNSIGNED_INT)4
		case VK_FORMAT_R32G32B32_SFLOAT: return 3 * 4;//(FLOAT)

		case VK_FORMAT_R8G8B8A8_SINT: return 4 * 1;//(BYTE)
		case VK_FORMAT_R8G8B8A8_UINT: return 4 * 1;//(UNSIGNED_BYTE)1
		case VK_FORMAT_R16G16B16A16_SINT: return 4 * 2;//(SHORT)2
		case VK_FORMAT_R16G16B16A16_UINT: return 4 * 2;//(UNSIGNED_SHORT)2
		case VK_FORMAT_R32G32B32A32_SINT: return 4 * 4;//(SIGNED_INT)4
		case VK_FORMAT_R32G32B32A32_UINT: return 4 * 4;//(UNSIGNED_INT)4
		case VK_FORMAT_R32G32B32A32_SFLOAT: return 4 * 4;//(FLOAT)
		}

		return 0;
	}
#endif // 1

	// 编译shader
#if 1


	std::string s_shaderLibDir;
	std::string s_shaderCacheDir;

	bool InitShaderCompilerCache(const std::string shaderLibDir, std::string shaderCacheDir)
	{
		s_shaderLibDir = hz::genfn(shaderLibDir);
		s_shaderCacheDir = hz::genfn(shaderCacheDir);

		return true;
	}

	std::string GetShaderCompilerLibDir()
	{
		return s_shaderLibDir;
	}

	std::string GetShaderCompilerCacheDir()
	{
		return s_shaderCacheDir;
	}

#ifdef _WIN32

	void ShowErrorMessageBox(LPCWSTR lpErrorString)
	{
		int msgboxID = MessageBoxW(NULL, lpErrorString, L"Error", MB_OK);
	}

	void ShowCustomErrorMessageBox(_In_opt_ LPCWSTR lpErrorString)
	{
		int msgboxID = MessageBoxW(NULL, lpErrorString, L"Error", MB_OK | MB_TOPMOST);
	}
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			wchar_t err[256];
			memset(err, 0, 256);
			FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL);
			char errA[256];
			size_t returnSize;
			wcstombs_s(&returnSize, errA, 255, err, 255);
			Trace(errA);
#ifdef _DEBUG
			ShowErrorMessageBox(err);
#endif
			throw 1;
		}
	}
#define USE_DXC_SPIRV_FROM_DISK

	void CompileMacros(const DefineList* pMacros, std::vector<D3D_SHADER_MACRO>* pOut)
	{
		if (pMacros != NULL)
		{
			for (auto it = pMacros->begin(); it != pMacros->end(); it++)
			{
				D3D_SHADER_MACRO macro;
				macro.Name = it->first.c_str();
				macro.Definition = it->second.c_str();
				pOut->push_back(macro);
			}
		}
	}

	DxcCreateInstanceProc s_dxc_create_func;

	bool InitDirectXCompiler()
	{
		std::string fullshaderCompilerPath = "dxcompiler.dll";
		std::string fullshaderDXILPath = "dxil.dll";

		//HMODULE dxil_module = ::LoadLibrary(fullshaderDXILPath.c_str());

		//HMODULE dxc_module = ::LoadLibrary(fullshaderCompilerPath.c_str());
		//if (dxc_module)
		//	s_dxc_create_func = (DxcCreateInstanceProc)::GetProcAddress(dxc_module, "DxcCreateInstance");
		//else
		s_dxc_create_func = DxcCreateInstance;

		return s_dxc_create_func != NULL;
	}

	interface Includer : public ID3DInclude
	{
	public:
		virtual ~Includer() {}

		HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID * ppData, UINT * pBytes)
		{
			std::string fullpath = hz::genfn(GetShaderCompilerLibDir() + "/" + pFileName);
			return readfile(fullpath.c_str(), (char**)ppData, (size_t*)pBytes, false) ? S_OK : E_FAIL;
		}
		HRESULT Close(LPCVOID pData)
		{
			free((void*)pData);
			return S_OK;
		}
	};

	interface IncluderDxc : public IDxcIncludeHandler
	{
		IDxcLibrary * m_pLibrary;
	public:
		IncluderDxc(IDxcLibrary* pLibrary) : m_pLibrary(pLibrary) {}
		HRESULT QueryInterface(const IID&, void**) { return S_OK; }
		ULONG AddRef() { return 0; }
		ULONG Release() { return 0; }
		HRESULT LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource)
		{
			std::string kfp = hz::genfn(GetShaderCompilerLibDir() + "/" + hz::u16_to_gbk(pFilename));
			LPCVOID pData;
			size_t bytes;
			HRESULT hr = readfile(kfp.c_str(), (char**)&pData, (size_t*)&bytes, false) ? S_OK : E_FAIL;

			if (hr == E_FAIL)
			{
				// return the failure here instead of crashing on CreateBlobWithEncodingFromPinned 
				// to be able to report the error to the output console
				return hr;
			}

			IDxcBlobEncoding* pSource;
			ThrowIfFailed(m_pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)pData, (UINT32)bytes, CP_UTF8, &pSource));

			*ppIncludeSource = pSource;

			return S_OK;
		}
	};

	bool DXCompileToDXO(size_t hash, const char* pSrcCode, const DefineList* pDefines, const char* pEntryPoint, const char* pParams, char** outSpvData, size_t* outSpvSize)
	{
		//detect output bytecode type (DXBC/SPIR-V) and use proper extension
		std::string filenameOut;
		{
			auto found = std::string(pParams).find("-spirv ");
			if (found == std::string::npos)
				filenameOut = hz::genfn(GetShaderCompilerCacheDir() + format("\\%p.dxo", hash));
			else
				filenameOut = hz::genfn(GetShaderCompilerCacheDir() + format("\\%p.spv", hash));
		}

#ifdef USE_DXC_SPIRV_FROM_DISK
		if (readfile(filenameOut.c_str(), outSpvData, outSpvSize, true) && *outSpvSize > 0)
		{
			//Trace(format("thread 0x%04x compile: %p disk\n", GetCurrentThreadId(), hash));
			return true;
		}
#endif

		// create hlsl file for shader compiler to compile
		//
		std::string filenameHlsl = hz::genfn(GetShaderCompilerCacheDir() + format("\\%p.hlsl", hash));
		std::ofstream ofs(filenameHlsl, std::ofstream::out);
		ofs << pSrcCode;
		ofs.close();

		std::string filenamePdb = hz::genfn(GetShaderCompilerCacheDir() + format("\\%p.lld", hash));

		std::wstring twstr;
		// get defines
		// 
		std::vector<DxcDefine> defines;
		defines.reserve(50);
		int defineCount = 0;
		if (pDefines != NULL)
		{
			for (auto it = pDefines->begin(); it != pDefines->end(); it++)
			{
				auto w = md::u8_w(it->first);
				auto w2 = md::u8_w(it->second);
				DxcDefine d = {};
				auto pos = twstr.size();
				w.push_back(0);
				twstr.append(w);
				auto pos2 = twstr.size();
				w.push_back(0);
				twstr.append(w2);
				d.Name = (wchar_t*)pos;
				d.Value = (wchar_t*)pos2;
				defineCount++;
				defines.push_back(d);
			}
		}
		// check whether DXCompiler is initialized
		if (s_dxc_create_func == nullptr)
		{
			Trace("Error: s_dxc_create_func() is null, have you called InitDirectXCompiler() ?");
			return false;
		}

		IDxcLibrary* pLibrary;
		ThrowIfFailed(s_dxc_create_func(CLSID_DxcLibrary, IID_PPV_ARGS(&pLibrary)));

		IDxcBlobEncoding* pSource = 0;
		ThrowIfFailed(pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)pSrcCode, (UINT32)strlen(pSrcCode), CP_UTF8, &pSource));
		if (!pSource)
			return false;
		IDxcCompiler2* pCompiler;
		ThrowIfFailed(s_dxc_create_func(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));

		IncluderDxc Includer(pLibrary);

		std::vector<LPCWSTR> ppArgs;
		std::string params;
		// splits params string into an array of strings
		{

			if (pParams && *pParams)
				params = pParams;
			auto pv = md::split(params, " ");
			for (auto& it : pv) {
				auto w = md::u8_w(it.c_str());
				auto pos = twstr.size();
				w.push_back(0);
				twstr.append(w);
				ppArgs.push_back((wchar_t*)pos);
			}
		}
		auto ws = twstr.data();
		for (auto& it : ppArgs) {
			it = ws + (size_t)it;
		}
		for (auto& it : defines) {
			it.Name = ws + (size_t)it.Name;
			it.Value = ws + (size_t)it.Value;
		}

		auto pEntryPointW = md::u8_w(pEntryPoint, -1);
		IDxcOperationResult* pResultPre;
		HRESULT res1 = pCompiler->Preprocess(pSource, L"", NULL, 0, defines.data(), defineCount, &Includer, &pResultPre);
		if (res1 == S_OK)
		{
			Microsoft::WRL::ComPtr<IDxcBlob> pCode1;
			pResultPre->GetResult(pCode1.GetAddressOf());
			std::string preprocessedCode = "";

			preprocessedCode = "// dxc -E" + std::string(pEntryPoint) + " " + params + " " + filenameHlsl + "\n\n";
			if (pDefines)
			{
				for (auto it = pDefines->begin(); it != pDefines->end(); it++)
					preprocessedCode += "#define " + it->first + " " + it->second + "\n";
			}
			preprocessedCode += std::string((char*)pCode1->GetBufferPointer());
			preprocessedCode += "\n";
			savefile(filenameHlsl.c_str(), preprocessedCode.c_str(), preprocessedCode.size(), false);

			IDxcOperationResult* pOpRes;
			HRESULT res;
#if 0
			if (false)
			{
				Microsoft::WRL::ComPtr<IDxcBlob> pPDB;
				LPWSTR pDebugBlobName[1024];
				res = pCompiler->CompileWithDebug(pSource, NULL, pEntryPointW.c_str(), L"", ppArgs.data(), (UINT32)ppArgs.size(), defines.data(), defineCount, &Includer, &pOpRes, pDebugBlobName, pPDB.GetAddressOf());

				// Setup the correct name for the PDB
				if (pPDB)
				{
					char pPDBName[1024];
					sprintf_s(pPDBName, "%s\\%ls", GetShaderCompilerCacheDir().c_str(), *pDebugBlobName);
					savefile(pPDBName, pPDB->GetBufferPointer(), pPDB->GetBufferSize(), true);
				}
			}
			else
#endif
			{
				res = pCompiler->Compile(pSource, NULL, pEntryPointW.c_str(), L"", ppArgs.data(), (UINT32)ppArgs.size(), defines.data(), defineCount, &Includer, &pOpRes);
			}

			pSource->Release();
			pLibrary->Release();
			pCompiler->Release();

			IDxcBlob* pResult = NULL;
			IDxcBlobEncoding* pError = NULL;
			if (pOpRes != NULL)
			{
				pOpRes->GetResult(&pResult);
				pOpRes->GetErrorBuffer(&pError);
				pOpRes->Release();
			}

			if (pResult != NULL && pResult->GetBufferSize() > 0)
			{
				*outSpvSize = pResult->GetBufferSize();
				*outSpvData = (char*)malloc(*outSpvSize);

				memcpy(*outSpvData, pResult->GetBufferPointer(), *outSpvSize);

				pResult->Release();

				// Make sure pError doesn't leak if it was allocated
				if (pError)
					pError->Release();

#ifdef USE_DXC_SPIRV_FROM_DISK
				savefile(filenameOut.c_str(), *outSpvData, *outSpvSize, true);
#endif
				return true;
			}
			else
			{
				IDxcBlobEncoding* pErrorUtf8 = 0;
				if (pError)
					pLibrary->GetBlobAsUtf8(pError, &pErrorUtf8);

				Trace("*** Error compiling %p.hlsl ***\n", hash);

				std::string filenameErr = hz::genfn(GetShaderCompilerCacheDir() + format("\\%p.err", hash));
				savefile(filenameErr.c_str(), pErrorUtf8->GetBufferPointer(), pErrorUtf8->GetBufferSize(), false);

				std::string errMsg = std::string((char*)pErrorUtf8->GetBufferPointer(), pErrorUtf8->GetBufferSize());
				Trace(errMsg);

				// Make sure pResult doesn't leak if it was allocated
				if (pResult)
					pResult->Release();

				pErrorUtf8->Release();
			}
		}

		return false;
	}


#endif // _WIN32

	//
	// Compiles a shader into SpirV
	//
	bool VKCompileToSpirv(size_t hash, ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const std::string& shaderCode, const char* pShaderEntryPoint, const char* shaderCompilerParams, const DefineList* pDefines, char** outSpvData, size_t* outSpvSize)
	{
		// create glsl file for shader compiler to compile
		//
		std::string filenameSpv;
		std::string filenameGlsl;
		if (sourceType == SST_GLSL)
		{
			filenameSpv = hz::genfn(format("%s\\%p.spv", GetShaderCompilerCacheDir().c_str(), hash));
			filenameGlsl = hz::genfn(format("%s\\%p.glsl", GetShaderCompilerCacheDir().c_str(), hash));
		}
		else if (sourceType == SST_HLSL)
		{
			filenameSpv = hz::genfn(format("%s\\%p.dxo", GetShaderCompilerCacheDir().c_str(), hash));
			filenameGlsl = hz::genfn(format("%s\\%p.hlsl", GetShaderCompilerCacheDir().c_str(), hash));
		}
		else
			assert(!"unknown shader extension");

		std::ofstream ofs(filenameGlsl, std::ofstream::out);
		ofs << shaderCode;
		ofs.close();

		// compute command line to invoke the shader compiler
		//
		const char* stage = NULL;
		switch (shader_type)
		{
		case VK_SHADER_STAGE_VERTEX_BIT:  stage = "vertex"; break;
		case VK_SHADER_STAGE_FRAGMENT_BIT:  stage = "fragment"; break;
		case VK_SHADER_STAGE_COMPUTE_BIT:  stage = "compute"; break;
		case VK_SHADER_STAGE_TASK_BIT_EXT:  stage = "task"; break;
		case VK_SHADER_STAGE_MESH_BIT_EXT:  stage = "mesh"; break;
		}

		// add the #defines
		//
		std::string defines;
		if (pDefines)
		{
			for (auto it = pDefines->begin(); it != pDefines->end(); it++)
				defines += "-D" + it->first + "=" + it->second + " ";
		}
		std::string commandLine;
		if (sourceType == SST_GLSL)
		{
			commandLine = format("glslc --target-env=vulkan1.1 -fshader-stage=%s -fentry-point=%s %s \"%s\" -o \"%s\" -I %s %s", stage, pShaderEntryPoint, shaderCompilerParams, filenameGlsl.c_str(), filenameSpv.c_str(), GetShaderCompilerLibDir().c_str(), defines.c_str());

			std::string filenameErr = hz::genfn(format("%s\\%p.err", GetShaderCompilerCacheDir().c_str(), hash));

			if (LaunchProcess(commandLine.c_str(), filenameErr.c_str()) == true)
			{
				readfile(filenameSpv.c_str(), outSpvData, outSpvSize, true);
				assert(*outSpvSize != 0);
				return true;
			}
		}
		else
		{
			std::string scp = format("-spirv -fspv-target-env=vulkan1.1 -I %s %s %s", GetShaderCompilerLibDir().c_str(), defines.c_str(), shaderCompilerParams);
			DXCompileToDXO(hash, shaderCode.c_str(), pDefines, pShaderEntryPoint, scp.c_str(), outSpvData, outSpvSize);
			assert(*outSpvSize != 0);

			return true;
		}

		return false;
	}

	//
	// Generate sources from the input data
	//
	std::string GenerateSource(ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const char* pshader, const char* shaderCompilerParams, const DefineList* pDefines)
	{
		std::string shaderCode(pshader);
		std::string code;

		if (sourceType == SST_GLSL)
		{
			// the first line in a GLSL shader must be the #version, insert the #defines right after this line
			size_t index = shaderCode.find_first_of('\n');
			code = shaderCode.substr(index, shaderCode.size() - index);

			shaderCode = shaderCode.substr(0, index) + "\n";
		}
		else if (sourceType == SST_HLSL)
		{
			code = shaderCode;
			shaderCode = "";
		}

		// add the #defines to the code to help debugging
		if (pDefines)
		{
			for (auto it = pDefines->begin(); it != pDefines->end(); it++)
				shaderCode += "#define " + it->first + " " + it->second + "\n";
		}
		// concat the actual shader code
		shaderCode += code;

		return shaderCode;
	}

	Cache<VkShaderModule> s_shaderCache;

	void DestroyShadersInTheCache(VkDevice device)
	{
		s_shaderCache.ForEach([device](const Cache<VkShaderModule>::DatabaseType::iterator& it)
			{
				vkDestroyShaderModule(device, it->second.m_data, NULL);
			});
	}

	VkResult CreateModule(VkDevice device, char* SpvData, size_t SpvSize, VkShaderModule* pShaderModule)
	{
		VkShaderModuleCreateInfo moduleCreateInfo = {};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pCode = (uint32_t*)SpvData;
		moduleCreateInfo.codeSize = SpvSize;
		return vkCreateShaderModule(device, &moduleCreateInfo, NULL, pShaderModule);
	}

	//
	// Compile a GLSL or a HLSL, will cache binaries to disk
	//
	VkResult VKCompile(VkDevice device, ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const char* pshader, const char* pShaderEntryPoint, const char* shaderCompilerParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader)
	{
		VkResult res = VK_SUCCESS;

		//compute hash
		// 
		size_t hash;
		size_t pslen = strlen(pshader);
		auto sf = hz::genfn(GetShaderCompilerLibDir() + "\\");
		hash = HashShaderString(sf.c_str(), pshader);
		hash = Hash(pShaderEntryPoint, strlen(pShaderEntryPoint), hash);
		hash = Hash(shaderCompilerParams, strlen(shaderCompilerParams), hash);
		hash = Hash((char*)&shader_type, sizeof(shader_type), hash);
		if (pDefines != NULL)
		{
			hash = pDefines->Hash(hash);
		}

#define USE_MULTITHREADED_CACHE 
#define USE_SPIRV_FROM_DISK   

#ifdef USE_MULTITHREADED_CACHE
		// Compile if not in cache
		//
		if (s_shaderCache.CacheMiss(hash, &pShader->module))
#endif
		{
			auto strk = format("%p", hash);
			print_time Pt("Compile Pipeline" + strk, 1);
			char* SpvData = NULL;
			size_t SpvSize = 0;
#ifdef USE_SPIRV_FROM_DISK
			std::string filenameSpv = hz::genfn(format("%s\\%p.spv", GetShaderCompilerCacheDir().c_str(), hash));
			if (readfile(filenameSpv.c_str(), &SpvData, &SpvSize, true) == false)
#endif
			{
				std::string shader = GenerateSource(sourceType, shader_type, pshader, shaderCompilerParams, pDefines);
				VKCompileToSpirv(hash, sourceType, shader_type, shader.c_str(), pShaderEntryPoint, shaderCompilerParams, 0, &SpvData, &SpvSize);
				if (SpvSize == 0) {
					printf("\n%s\n", shader.c_str());
				}
				assert(SpvSize != 0);
			}
			else {
				if (SpvSize == 0) {
					printf("\n%s\n", strk.c_str());
				}
			}
			//auto c = crc32(-1, (Bytef*)pshader, strlen(pshader));  
			assert(SpvSize != 0);
			CreateModule(device, SpvData, SpvSize, &pShader->module);

#ifdef USE_MULTITHREADED_CACHE
			s_shaderCache.UpdateCache(hash, &pShader->module);
#endif
		}
		else {
			//print_time Pt("no Compile Pipeline", 1);
		}

		pShader->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pShader->pNext = NULL;
		pShader->pSpecializationInfo = NULL;
		pShader->flags = 0;
		pShader->stage = shader_type;
		pShader->pName = pShaderEntryPoint;

		return res;
	}


	//
	// VKCompileFromString
	//
	VkResult VKCompileFromString(VkDevice device, ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const char* pShaderCode, const char* pShaderEntryPoint, const char* shaderCompilerParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader)
	{
		assert(strlen(pShaderCode) > 0);

		VkResult res = VKCompile(device, sourceType, shader_type, pShaderCode, pShaderEntryPoint, shaderCompilerParams, pDefines, pShader);
		assert(res == VK_SUCCESS);

		return res;
	}

	//
	// VKCompileFromFile
	//
	VkResult VKCompileFromFile(VkDevice device, const VkShaderStageFlagBits shader_type, const char* pFilename, const char* pShaderEntryPoint, const char* shaderCompilerParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader)
	{
		char* pShaderCode;
		size_t size;

		ShaderSourceType sourceType;
		std::string pfn = pFilename;
		if (pfn.find(".glsl") != std::string::npos)
		{
			sourceType = SST_GLSL;
		}
		else if (pfn.find(".hlsl") != std::string::npos)
		{
			sourceType = SST_HLSL;
		}
		//const char* pExtension = pFilename + std::max<size_t>(strlen(pFilename) - 4, 0);
		//if (strcmp(pExtension, "glsl") == 0)
		//	sourceType = SST_GLSL;
		//else if (strcmp(pExtension, "hlsl") == 0)
		//	sourceType = SST_HLSL;
		else
			assert(!"Can't tell shader type from its extension");

		//append path
		auto fullpath = hz::genfn(GetShaderCompilerLibDir() + std::string("/") + pFilename);

		if (readfile(fullpath.c_str(), &pShaderCode, &size, false))
		{
			VkResult res = VKCompileFromString(device, sourceType, shader_type, pShaderCode, pShaderEntryPoint, shaderCompilerParams, pDefines, pShader);
			SetResourceName(device, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)pShader->module, pFilename);
			return res;
		}

		return VK_NOT_READY;
	}

#endif 
	// 12
	// misc
#if 1

//
// Get current time in milliseconds
//
	double MillisecondsNow()
	{
		double milliseconds = 0;
#ifdef _WIN32
		static LARGE_INTEGER s_frequency = {};
		static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
		if (s_use_qpc)
		{
			LARGE_INTEGER now = {};
			QueryPerformanceCounter(&now);
			milliseconds = double(1000.0 * now.QuadPart) / s_frequency.QuadPart;
		}
		else
		{
			milliseconds = (double)GetTickCount64();
		}
#endif
		return milliseconds;
	}

	// Formats a string
	std::string format(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
#ifndef _MSC_VER
		size_t size = std::snprintf(nullptr, 0, format, args) + 1; // Extra space for '\0'
		std::string buf; buf.resize(size);
		std::vsnprintf(buf.Data(), size, format, args);
		va_end(args);
		return std::string(buf.Data(), buf.Data() + size - 1); // We don't want the '\0' inside
#else
		const size_t size = (size_t)_vscprintf(format, args) + 1;
		std::string buf; buf.resize(size);
		vsnprintf_s(buf.data(), size, _TRUNCATE, format, args);
		va_end(args);
		return buf.c_str();
#endif
	}

	void Trace(const std::string& str)
	{
#ifdef _WIN32
		static std::mutex mutex;
		std::unique_lock<std::mutex> lock(mutex);
		// Output to attached debugger
		OutputDebugStringA(str.c_str());
		// Also log to file
		Log::Trace(str.c_str());
#endif
	}

	void Trace(const char* pFormat, ...)
	{
#ifdef _WIN32
		static std::mutex mutex;
		std::unique_lock<std::mutex> lock(mutex);
		va_list args;
		// generate formatted string
		va_start(args, pFormat);
		const size_t bufLen = (size_t)_vscprintf(pFormat, args) + 2;
		std::string buf; buf.resize(bufLen);
		vsnprintf_s(buf.data(), bufLen, bufLen, pFormat, args);
		va_end(args);
		buf.push_back('\n');
		// Output to attached debugger
		OutputDebugStringA(buf.data());
		// Also log to file
		Log::Trace(buf.data());
#endif
	}

	//  Reads a file into a buffer
	bool readfile(const char* name, char** data, size_t* size, bool isbinary)
	{
		FILE* file;
		//Open file
		if (fopen_s(&file, name, isbinary ? "rb" : "r") != 0)
		{
			return false;
		}
		//Get file length
		fseek(file, 0, SEEK_END);
		size_t fileLen = ftell(file);
		fseek(file, 0, SEEK_SET);
		// if ascii add one more char to accomodate for the \0
		if (!isbinary)
			fileLen++;
		//Allocate memory
		char* buffer = (char*)malloc(std::max<size_t>(fileLen, 1));
		if (!buffer)
		{
			fclose(file);
			return false;
		}
		//Read file contents into buffer
		size_t bytesRead = 0;
		if (fileLen > 0)
		{
			bytesRead = fread(buffer, 1, fileLen, file);
		}
		fclose(file);
		if (!isbinary)
		{
			buffer[bytesRead] = 0;
			fileLen = bytesRead;
		}
		*data = buffer;
		if (size != NULL)
			*size = fileLen;
		return true;
	}
	bool readfile(const char* name, std::vector<char>& data, bool isbinary)
	{
		FILE* file;
		//Open file
		if (fopen_s(&file, name, isbinary ? "rb" : "r") != 0)
		{
			return false;
		}
		//Get file length
		fseek(file, 0, SEEK_END);
		size_t fileLen = ftell(file);
		fseek(file, 0, SEEK_SET);
		// if ascii add one more char to accomodate for the \0
		if (!isbinary)
			fileLen++;
		//Allocate memory
		data.resize(std::max<size_t>(fileLen, 1));
		char* buffer = data.data();// (char*)malloc(std::max<size_t>(fileLen, 1));
		if (!buffer)
		{
			fclose(file);
			return false;
		}
		//Read file contents into buffer
		size_t bytesRead = 0;
		if (fileLen > 0)
		{
			bytesRead = fread(buffer, 1, fileLen, file);
		}
		fclose(file);
		if (!isbinary)
		{
			buffer[bytesRead] = 0;
			fileLen = bytesRead;
		}
		data.resize(fileLen);
		return true;
	}

	bool savefile(const char* name, void const* data, size_t size, bool isbinary)
	{
		FILE* file;
		if (fopen_s(&file, name, isbinary ? "wb" : "w") == 0)
		{
			fwrite(data, size, 1, file);
			fclose(file);
			return true;
		}
		return false;
	}

	// Launch a process, captures stderr into a file
	bool LaunchProcess(const char* commandLine, const char* filenameErr)
	{
#ifdef _WIN32
		char cmdLine[1024];
		strcpy_s<1024>(cmdLine, commandLine);
		// create a pipe to get possible errors from the process
		HANDLE g_hChildStd_OUT_Rd = NULL;
		HANDLE g_hChildStd_OUT_Wr = NULL;
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;
		if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
			return false;
		// launch process
		PROCESS_INFORMATION pi = {};
		STARTUPINFOA si = {};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdError = g_hChildStd_OUT_Wr;
		si.hStdOutput = g_hChildStd_OUT_Wr;
		si.wShowWindow = SW_HIDE;
		if (CreateProcessA(NULL, cmdLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(g_hChildStd_OUT_Wr);
			ULONG rc;
			if (GetExitCodeProcess(pi.hProcess, &rc))
			{
				if (rc == 0)
				{
					DeleteFileA(filenameErr);
					return true;
				}
				else
				{
					Trace(format("*** Process %s returned an error, see %s ***\n\n", commandLine, filenameErr));
					// save errors to disk
					std::ofstream ofs(filenameErr, std::ofstream::out);
					for (;;)
					{
						DWORD dwRead;
						char chBuf[2049];
						BOOL bSuccess = ::ReadFile(g_hChildStd_OUT_Rd, chBuf, 2048, &dwRead, NULL);
						chBuf[dwRead] = 0;
						if (!bSuccess || dwRead == 0) break;
						Trace(chBuf);
						ofs << chBuf;
					}
					ofs.close();
				}
			}
			CloseHandle(g_hChildStd_OUT_Rd);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else
		{
			Trace(format("*** Can't launch: %s \n", commandLine));
		}
#endif
		return false;
	}

	// Frustum culls an AABB. The culling is done in clip space. 
	bool CameraFrustumToBoxCollision(const glm::mat4& mCameraViewProj, const glm::vec4& boxCenter, const glm::vec4& boxExtent)
	{
		float ex = boxExtent.x;
		float ey = boxExtent.y;
		float ez = boxExtent.z;
		glm::vec4 p[8];
		p[0] = mCameraViewProj * (boxCenter + glm::vec4(ex, ey, ez, 0));
		p[1] = mCameraViewProj * (boxCenter + glm::vec4(ex, ey, -ez, 0));
		p[2] = mCameraViewProj * (boxCenter + glm::vec4(ex, -ey, ez, 0));
		p[3] = mCameraViewProj * (boxCenter + glm::vec4(ex, -ey, -ez, 0));
		p[4] = mCameraViewProj * (boxCenter + glm::vec4(-ex, ey, ez, 0));
		p[5] = mCameraViewProj * (boxCenter + glm::vec4(-ex, ey, -ez, 0));
		p[6] = mCameraViewProj * (boxCenter + glm::vec4(-ex, -ey, ez, 0));
		p[7] = mCameraViewProj * (boxCenter + glm::vec4(-ex, -ey, -ez, 0));
		uint32_t left = 0;
		uint32_t right = 0;
		uint32_t top = 0;
		uint32_t bottom = 0;
		uint32_t back = 0;
		for (int i = 0; i < 8; i++)
		{
			float x = p[i].x;
			float y = p[i].y;
			float z = p[i].z;
			float w = p[i].w;
			if (x < -w) left++;
			if (x > w) right++;
			if (y < -w) bottom++;
			if (y > w) top++;
			if (z < 0) back++;
		}
		return left == 8 || right == 8 || top == 8 || bottom == 8 || back == 8;
	}
	AxisAlignedBoundingBox::AxisAlignedBoundingBox() : m_min(), m_max(), m_isEmpty{ true }
	{
	}

	void AxisAlignedBoundingBox::Merge(const AxisAlignedBoundingBox& bb)
	{
		if (bb.m_isEmpty)
			return;
		if (m_isEmpty)
		{
			m_max = bb.m_max;
			m_min = bb.m_min;
			m_isEmpty = false;
		}
		else
		{
			m_min = glm::min(m_min, bb.m_min);// Vectormath::SSE::minPerElem(m_min, bb.m_min);
			m_max = glm::max(m_max, bb.m_max);//Vectormath::SSE::maxPerElem(m_max, bb.m_max);
		}
	}

	void AxisAlignedBoundingBox::Grow(const glm::vec4 v)
	{
		if (m_isEmpty)
		{
			m_max = v;
			m_min = v;
			m_isEmpty = false;
		}
		else
		{
			m_min = glm::min(m_min, v);
			m_max = glm::max(m_max, v);
		}
	}

	bool AxisAlignedBoundingBox::HasNoVolume() const
	{
		return m_isEmpty || (m_max.x == m_min.x && m_max.y == m_min.y && m_max.z == m_min.z);
	}

	AxisAlignedBoundingBox GetAABBInGivenSpace(const glm::mat4& mTransform, const glm::vec4& boxCenter, const glm::vec4& boxExtent)
	{
		float ex = boxExtent.x;
		float ey = boxExtent.y;
		float ez = boxExtent.z;

		AxisAlignedBoundingBox aabb;

		// get the position of each corner of the bounding box in the camera space
		glm::vec4 p[8];
		p[0] = mTransform * (boxCenter + glm::vec4(ex, ey, ez, 0));
		p[1] = mTransform * (boxCenter + glm::vec4(ex, ey, -ez, 0));
		p[2] = mTransform * (boxCenter + glm::vec4(ex, -ey, ez, 0));
		p[3] = mTransform * (boxCenter + glm::vec4(ex, -ey, -ez, 0));
		p[4] = mTransform * (boxCenter + glm::vec4(-ex, ey, ez, 0));
		p[5] = mTransform * (boxCenter + glm::vec4(-ex, ey, -ez, 0));
		p[6] = mTransform * (boxCenter + glm::vec4(-ex, -ey, ez, 0));
		p[7] = mTransform * (boxCenter + glm::vec4(-ex, -ey, -ez, 0));

		for (int i = 0; i < 8; ++i)
			aabb.Grow(p[i]);

		return aabb;
	}


	int countBits(uint32_t v)
	{
		v = v - ((v >> 1) & 0x55555555);                // put count of each 2 bits into those 2 bits
		v = (v & 0x33333333) + ((v >> 2) & 0x33333333); // put count of each 4 bits into those 4 bits  
		return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
	}

	Log* Log::m_pLogInstance = nullptr;

	int Log::InitLogSystem()
	{
		// Create an instance of the log system if non already exists
		if (!m_pLogInstance)
		{
			m_pLogInstance = new Log();
			assert(m_pLogInstance);
			if (m_pLogInstance)
				return 0;
		}

		// Something went wrong
		return -1;
	}

	int Log::TerminateLogSystem()
	{
		if (m_pLogInstance)
		{
			delete m_pLogInstance;
			m_pLogInstance = nullptr;
			return 0;
		}

		// Something went wrong
		return -1;
	}

	void Log::Trace(const char* LogString)
	{
		assert(m_pLogInstance); // Can't do anything without a valid instance
		if (m_pLogInstance)
		{
			m_pLogInstance->Write(LogString);
		}
	}

	Log::Log()
	{
		std::wstring pn = L"temp/cvkr.log";
		std::string pna = "temp/cvkr.log";
		hz::genfn(pna);
		_fp = fopen(pna.c_str(), "a+");
		if (!_fp)
		{
#ifdef _WIN32 
			m_FileHandle = CreateFileW(pn.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, nullptr);
			assert(m_FileHandle != INVALID_HANDLE_VALUE);

			// Initialize the overlapped structure for asynchronous write
			for (uint32_t i = 0; i < MAX_INFLIGHT_WRITES; i++)
				m_OverlappedData[i] = { 0 };
#endif

		}
	}

	Log::~Log()
	{
		if (_fp)
		{
			fclose(_fp); _fp = 0;
		}
#ifdef _WIN32 
		if (m_FileHandle)
			CloseHandle(m_FileHandle);
		m_FileHandle = INVALID_HANDLE_VALUE;
#endif
	}

	void OverlappedCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		// We never go into an alert state, so this is just to compile
	}

	void Log::Write(const char* LogString)
	{
		if (_fp)
		{
			auto s = strlen(LogString);
			fwrite(LogString, s, 1, _fp);
			return;
		}
#ifdef _WIN32 
		OVERLAPPED* pOverlapped = &m_OverlappedData[m_CurrentIOBufferIndex];

		// Make sure any previous write with this overlapped structure has completed
		DWORD numTransferedBytes;
		GetOverlappedResult(m_FileHandle, pOverlapped, &numTransferedBytes, TRUE);  // This will wait on the current thread

		// Apply increments accordingly
		pOverlapped->Offset = m_WriteOffset;
		m_WriteOffset += static_cast<uint32_t>(strlen(LogString));

		m_CurrentIOBufferIndex = (++m_CurrentIOBufferIndex % MAX_INFLIGHT_WRITES);  // Wrap when we get to the end

		bool result = WriteFileEx(m_FileHandle, LogString, static_cast<DWORD>(strlen(LogString)), pOverlapped, OverlappedCompletionRoutine);
		assert(result);
#endif
	}
#endif // 1

	bool vkmReadWholeFile(std::vector<unsigned char>* out, std::string* err, const std::string& filepath, void*);


	//
	// Hash a string of source code and recurse over its #include files
	//
	size_t HashShaderString(const char* pRootDir, const char* pShader, size_t hash)
	{
		hash = Hash(pShader, strlen(pShader), hash);
		std::vector<unsigned char> tfd;
		std::vector<char> td;
		const char* pch = pShader;
		while (*pch != 0)
		{
			if (*pch == '/') // parse comments
			{
				pch++;
				if (*pch != 0 && *pch == '/')
				{
					pch++;
					while (*pch != 0 && *pch != '\n')
						pch++;
				}
				else if (*pch != 0 && *pch == '*')
				{
					pch++;
					while ((*pch != 0 && *pch != '*') && (*(pch + 1) != 0 && *(pch + 1) != '/'))
						pch++;
				}
			}
			else if (*pch == '#') // parse #include
			{
				pch++;
				const char include[] = "include";
				int i = 0;
				while ((*pch != 0) && *pch == include[i])
				{
					pch++;
					i++;
				}

				if (i == strlen(include))
				{
					while (*pch != 0 && *pch == ' ')
						pch++;

					if (*pch != 0 && *pch == '\"')
					{
						pch++;
						const char* pName = pch;

						while (*pch != 0 && *pch != '\"')
							pch++;

						char includeName[1024];
						strcpy_s<1024>(includeName, pRootDir);
						strncat_s<1024>(includeName, pName, pch - pName);

						pch++;

						tfd.clear();
						vkmReadWholeFile(&tfd, 0, includeName, 0);
						if (tfd.size())
						{
							tfd.push_back(0);
							hash = HashShaderString(pRootDir, (char*)tfd.data(), hash);
						}
					}
				}
			}
			else
			{
				pch++;
			}
		}

		return hash;
	}

#ifdef _WIN32
#define fseeki64 _fseeki64
#define ftelli64 _ftelli64
#else			
#define fseeki64 fseeko64
#define ftelli64 ftello64
#endif // _WIN32

	bool vkmReadWholeFile(std::vector<unsigned char>* out, std::string* err, const std::string& filepath, void* ptr)
	{
#ifdef TINYGLTF_ANDROID_LOAD_FROM_ASSETS
		if (asset_manager) {
			AAsset* asset = AAssetManager_open(asset_manager, filepath.c_str(),
				AASSET_MODE_STREAMING);
			if (!asset) {
				if (err) {
					(*err) += "File open error : " + filepath + "\n";
				}
				return false;
			}
			size_t size = AAsset_getLength(asset);
			if (size == 0) {
				if (err) {
					(*err) += "Invalid file size : " + filepath +
						" (does the path point to a directory?)";
				}
				return false;
			}
			out->resize(size);
			AAsset_read(asset, reinterpret_cast<char*>(&out->at(0)), size);
			AAsset_close(asset);
			return true;
		}
		else {
			if (err) {
				(*err) += "No asset manager specified : " + filepath + "\n";
			}
			return false;
		}
#else
		int64_t size = 0;
		const char* fn = filepath.c_str();
#if 0
		FILE* fp = fopen(filepath.c_str(), "rb");
		if (fp)
		{
			fseeki64(fp, 0L, SEEK_END);
			size = ftelli64(fp);
			fseeki64(fp, 0L, SEEK_SET);
			out->resize(size);
			auto buff = out->data();
			auto retval = fread(buff, size, 1, fp);
			assert(retval == 1);
			fclose(fp);
		}
#else
		hz::mfile_t mt;
		auto fp = mt.open_d(fn, true);
		if (fp)
		{
			size = mt.size();
			out->resize(size);
			auto buff = out->data();
			memcpy(buff, fp, size);
		}
#endif
		return fp;// hz::File::read_binary_file(filepath.c_str(), *out);
#endif
	}
	void Transform::LookAt(glm::vec4 source, glm::vec4 target, bool flipY)
	{
		auto mat = m_rotation = glm::inverse(glm::lookAt(glm::vec3(source), glm::vec3(target), glm::vec3(0, flipY ? -1 : 1, 0)));
		m_translation = mat[3];  m_rotation[3] = glm::vec4(0, 0, 0, 1);

	}
	void Matrix2::Set(const glm::mat4& m) {
		m_previous = m_current;
		m_current = m;
	}

#if 1
	GLTFCommon::~GLTFCommon()
	{
		Unload();
	}

	bool GLTFCommon::Load(const std::string& path, const std::string& filename)
	{
		//Profile p("GLTFCommon::Load");
		m_path = path;
		auto pm1 = new tinygltf::Model();
		if (!pm1)
		{
			return false;
		}
		pm = pm1;

		_ubo_md.resize(sizeof(PerFrame_t) * 2);
		_perFrameData = (PerFrame_t*)_ubo_md.data();
		_perFrameData_w = _perFrameData + 1;
		tinygltf::TinyGLTF loader = {};
		std::string err;
		std::string warn;
		tinygltf::FsCallbacks cb = { &tinygltf::FileExists, &tinygltf::ExpandFilePath, &vkmReadWholeFile, &tinygltf::WriteWholeFile, };
		loader.SetFsCallbacks(cb);
		loader.SetImagesAsIs(true); // true: load images as is, false: decode images
		char* bytes = 0;// rawdata.data();
		auto size = 0;// rawdata.size();
		bool ret = false;
		std::string basedir = path;// GetBaseDir(fn); 
		if (filename.empty() && basedir.find(".glb") != std::string::npos || filename.find(".glb") != std::string::npos)
		{
			//print_time Pt("LoadBinaryFromFile", 1);
			ret = loader.LoadBinaryFromFile(pm, &err, &warn, path + filename);
		}
		else
		{
			//print_time Pt("LoadASCIIFromFile", 1);
			ret = loader.LoadASCIIFromFile(pm, &err, &warn, path + filename);
		}
		// for binary glTF(.glb)
		//auto ret = loader.LoadBinaryFromMemory(pm, &err, &warn, (unsigned char*)bytes, size, basedir);
		//err.clear();
		//if (!ret) {
		//	ret = loader.LoadASCIIFromString(pm, &err, &warn, (char*)bytes, size, basedir);
		//}
		do {
			if (err.size())
			{
				printf("error:\t%s\n", err.c_str());
				delete pm;
				pm = 0;
				break;
			}
			if (warn.size())
				printf("warn:\t%s\n", warn.c_str());

			load_Buffers();
			load_Meshes();
			load_lights();
			load_cameras();
			load_nodes();
			load_scenes();
			load_skins();
			load_animations();
			InitTransformedData();
			return true;
		} while (0);

		return false;
	}

	void GLTFCommon::Unload()
	{
		for (auto p : _sparseData) {
			if (p)
				free(p);
		}
		//for (int i = 0; i < _fileData_f.size(); i++)
		//{
		//	delete[] _fileData_f[i];
		//}
		for (auto& it : _fileData) {
			delete it;
		}
		_sparseData.clear();
		m_buffersData.clear();

		m_animations.clear();
		m_nodes.clear();
		m_scenes.clear();
		//m_lights.clear();
		m_lightInstances.clear();
		if (pm)delete pm;
		pm = 0;
	}


	void GLTFCommon::load_Buffers()
	{
		{
			auto& buffers = pm->buffers;
			m_buffersData.resize(buffers.size());
			for (int i = 0; i < buffers.size(); i++)
			{
				auto& it = buffers[i];
				if (it.data.size())
				{
					m_buffersData[i] = (char*)it.data.data();
					continue;
				}
				const std::string& name = it.uri;
#ifdef _MFILE_
				hz::mfile_t mt;
				auto fp = mt.open_d(m_path + name, true);
				if (fp && mt.size() > 0)
				{
					auto ptr = new hz::mfile_t();
					if (ptr) {
						ptr->swap(mt);
						m_buffersData[i] = ptr->data();
						_fileData.push_back(ptr);
					}
				}
#else
				std::ifstream ff(m_path + name, std::ios::in | std::ios::binary);
				ff.seekg(0, ff.end);
				std::streamoff length = ff.tellg();
				if (length > 0)
				{
					ff.seekg(0, ff.beg);
					char* p = new char[length];
					if (p) {
						ff.read(p, length);
						m_buffersData[i] = p;
						_fileData_f.push_back(p);
					}
				}
#endif
			}
		}
	}

	template<class T, class T1>
	T1 getv4(const T& v, T1 d)
	{
		T1 r = d;// { 0, 0, 0, 0 };
		for (int i = 0; i < v.size() && i < 4; i++)
		{
			r[i] = v[i];
		}
		return r;
	}

	template<class T>
	glm::mat4 get_mat(const T& accessor)
	{
		return glm::mat4(
			glm::vec4(accessor[0], accessor[1], accessor[2], accessor[3]),
			glm::vec4(accessor[4], accessor[5], accessor[6], accessor[7]),
			glm::vec4(accessor[8], accessor[9], accessor[10], accessor[11]),
			glm::vec4(accessor[12], accessor[13], accessor[14], accessor[15]));
	}
	template<class T>
	glm::mat4 get_mat1(const T& accessor)
	{
		return glm::mat4(
			glm::vec4(accessor[0], accessor[1], accessor[2], accessor[3]),
			glm::vec4(accessor[4], accessor[5], accessor[6], accessor[7]),
			glm::vec4(accessor[8], accessor[9], accessor[10], accessor[11]),
			glm::vec4(accessor[12], accessor[13], accessor[14], accessor[15]));
	}

	void* get_bvd(tinygltf::Model* pm, int bufferViewIdx, int byteOffset, std::vector<char*>& m_buffersData)
	{
		auto& bufferView = pm->bufferViews[bufferViewIdx];
		int32_t bufferIdx = bufferView.buffer;
		assert(bufferIdx >= 0);
		char* buffer = m_buffersData[bufferIdx];
		int32_t offset = bufferView.byteOffset;
		int byteLength = bufferView.byteLength;
		offset += byteOffset;
		byteLength -= byteOffset;
		return &buffer[offset];
	}
	void GLTFCommon::load_Meshes()
	{
		auto& meshes = pm->meshes;
		m_meshes.resize(meshes.size());
		for (int i = 0; i < meshes.size(); i++)
		{
			tfMesh* tfmesh = &m_meshes[i];
			auto& primitives = meshes[i].primitives;
			tfmesh->m_pPrimitives.resize(primitives.size());
			for (int p = 0; p < primitives.size(); p++)
			{
				tfPrimitives* pPrimitive = &tfmesh->m_pPrimitives[p];

				int positionId = primitives[p].attributes["POSITION"];
				auto& accessor = pm->accessors[positionId];

				glm::vec4 max1 = getv4(accessor.maxValues, glm::vec4(0, 0, 0, 0));
				glm::vec4 min1 = getv4(accessor.minValues, glm::vec4(0, 0, 0, 0));

				pPrimitive->m_center = (min1 + max1) * 0.5f;
				pPrimitive->m_radius = max1 - pPrimitive->m_center;

				pPrimitive->m_center = glm::vec4(glm::vec3(pPrimitive->m_center), 1.0f); //set the W to 1 since this is a position not a direction
				pPrimitive->vcount = accessor.count;
				auto& tar = primitives[p].targets;
				auto tn = tar.size();
				for (size_t n = 0; n < tn; n++)
				{
					auto it = tar[n];
					for (auto& [k, v] : it)
					{
						glm::ivec2 v2 = { 0, v };
						auto& tacc = pm->accessors[v];
						auto& kn = k;// POSITION,NORMAL,TANGENT==vec3
						switch (kn[0])
						{
						case 'P':v2.x = 0; break;
						case 'N':v2.x = 1;
							break;
						case 'T':v2.x = 2; break;
						default:
							break;
						}
						if (kn == "TEXCOORD_0")//vec2
						{
							v2.x = 3;
						}
						if (kn == "TEXCOORD_1")
						{
							v2.x = 4;
						}
						if (kn == "COLOR_0")//vec3/4
						{
							v2.x = 5;
						}
						if (kn == "COLOR_1")
						{
							v2.x = 6;
						}
						pPrimitive->targets.push_back(v2);
					}
				}
			}
			auto& weights = meshes[i].weights;// 默认变形插值数据
			auto& pw = tfmesh->weights;
			auto length = weights.size();
			pw.resize(length);
			for (size_t w = 0; w < length; w++)
			{
				pw[w] = weights[w];
			}
		}
		return;
	}
	void GLTFCommon::load_lights()
	{
		m_lightInstances.push_back({ 0, 0 });// 默认灯光
#if 0
		if (pm)
		{
			m_lights.resize(pm->lights.size());
			for (int i = 0; i < pm->lights.size(); i++)
			{
				auto& light = pm->lights[i];
				m_lights[i].m_color = getv4(light.color, glm::vec4(1, 1, 1, 0));
				m_lights[i].m_range = light.range;
				m_lights[i].m_intensity = light.intensity;
				m_lights[i].m_innerConeAngle = light.spot.innerConeAngle;
				m_lights[i].m_outerConeAngle = light.spot.outerConeAngle;

				std::string lightName = light.name;

				std::string lightType = light.type;
				if (lightType == "spot")
					m_lights[i].m_type = tfLight::LIGHT_SPOTLIGHT;
				else if (lightType == "point")
					m_lights[i].m_type = tfLight::LIGHT_POINTLIGHT;
				else if (lightType == "directional")
					m_lights[i].m_type = tfLight::LIGHT_DIRECTIONAL;

				// Deal with shadow settings
				if (m_lights[i].m_type == tfLight::LIGHT_DIRECTIONAL || m_lights[i].m_type == tfLight::LIGHT_SPOTLIGHT)
				{
					// Unless "NoShadow" is present in the name, the light will have a shadow
					if (std::string::npos != lightName.find("NoShadow", 0, 8))
						m_lights[i].m_shadowResolution = 0; // 0 shadow resolution means no shadow
					else
					{
						// See if we have specified a resolution
						size_t offset = lightName.find("Resolution_", 0, 11);
						if (std::string::npos != offset)
						{
							// Update offset to start from after "_"
							offset += 11;
							// Look for the end separator
							size_t endOffset = lightName.find("_", offset, 1);
							if (endOffset != std::string::npos)
							{
								// Try to grab the value
								std::string ResString = lightName.substr(offset, endOffset - offset);
								int32_t Resolution = -1;
								try {
									Resolution = std::stoi(ResString);
								}
								catch (std::invalid_argument)
								{
									// Wasn't a valid argument to convert to int, use default
								}
								catch (std::out_of_range)
								{
									// Value larger than an int can hold (also invalid), use default
								}

								// Check if resolution is a power of 2
								if (Resolution == 1 || (Resolution & (Resolution - 1)) == 0)
									m_lights[i].m_shadowResolution = (uint32_t)Resolution;
							}
						}

						// See if we have specified a bias 
						offset = lightName.find("Bias_", 0, 5);
						if (std::string::npos != offset)
						{
							// Update offset to start from after "_"
							offset += 5;

							// Look for the end separator
							size_t endOffset = lightName.find("_", offset, 1);
							if (endOffset != std::string::npos)
							{
								// Try to grab the value
								std::string BiasString = lightName.substr(offset, endOffset - offset);
								float Bias = (m_lights[i].m_type == tfLight::LIGHT_SPOTLIGHT) ? (70.0f / 100000.0f) : (1000.0f / 100000.0f);

								try {
									Bias = std::stof(BiasString);
								}
								catch (std::invalid_argument)
								{
									// Wasn't a valid argument to convert to float, use default
								}
								catch (std::out_of_range)
								{
									// Value larger than a float can hold (also invalid), use default
								}

								// Set what we have 
								m_lights[i].m_bias = Bias;
							}
						}
					}
				}
			}
		}
#endif
	}
	void GLTFCommon::load_cameras()
	{
		auto& cameras = pm->cameras;
		m_cameras.resize(cameras.size());
		for (int i = 0; i < cameras.size(); i++)
		{
			auto& camera = cameras[i];
			tfCamera* tfcamera = &m_cameras[i];
			tfcamera->yfov = camera.perspective.yfov;// GetElementFloat(camera, "perspective/yfov", 0.1f);
			tfcamera->znear = camera.perspective.znear;// GetElementFloat(camera, "perspective/znear", 0.1f);
			tfcamera->zfar = camera.perspective.zfar;// GetElementFloat(camera, "perspective/zfar", 100.0f);
			tfcamera->m_nodeIndex = -1;
		}
	}
	void GLTFCommon::load_nodes()
	{
		auto& nodes = pm->nodes;
		m_nodes.resize(nodes.size());
		mat_count = nodes.size();
		for (int i = 0; i < nodes.size(); i++)
		{
			tfNode* tfnode = &m_nodes[i];

			// Read node data
			//
			auto& node = nodes[i];

			for (int c = 0; c < node.children.size(); c++)
			{
				int nodeID = node.children[c];
				tfnode->m_children.push_back(nodeID);
			}

			tfnode->meshIndex = node.mesh;// GetElementInt(node, "mesh", -1);
			tfnode->skinIndex = node.skin;// GetElementInt(node, "skin", -1);

			int cameraIdx = node.camera;// GetElementInt(node, "camera", -1);
			if (cameraIdx >= 0)
				m_cameras[cameraIdx].m_nodeIndex = i;

			node.extras;
			if (node.extensions.find("KHR_lights_punctual") != node.extensions.end())
			{
				auto lp = node.extensions["KHR_lights_punctual"];
				// GetElementInt(node, "extensions/KHR_lights_punctual/light", -1);
				if (lp.Has("light"))
				{
					int lightIdx = lp.Get("light").Get<int>();
					if (lightIdx > -1)
						m_lightInstances.push_back({ lightIdx, i });
				}

			}
			if (node.translation.size() > 2)
				tfnode->m_tranform.m_translation = glm::vec4(glm::make_vec3(node.translation.data()), 0.0f);// GetElementVector(node, "translation", glm::vec4(0, 0, 0, 0));

			if (node.scale.size() > 2)
			{
				tfnode->m_tranform.m_scale = glm::vec4(glm::make_vec3(node.scale.data()), 0.0f);// getv4(node.scale, glm::vec4(1, 1, 1, 0));//GetElementVector(node, "scale", glm::vec4(1, 1, 1, 0));
				tfnode->negativescale = tfnode->m_tranform.m_scale.x < 0 || tfnode->m_tranform.m_scale.y < 0 || tfnode->m_tranform.m_scale.z < 0;
			}

			tfnode->m_name = node.name.c_str() ? node.name : "unnamed";// GetElementString(node, "name", "unnamed");
			tfnode->m_tranform.m_rotation = glm::identity<glm::mat4>();
			if (node.rotation.size())
			{
				glm::quat q = glm::make_quat(node.rotation.data());
				tfnode->m_tranform.m_rotation = glm::mat4(q);
			}
			else if (node.matrix.size())
			{
				tfnode->m_tranform.m_rotation = (glm::make_mat4x4(node.matrix.data()));
			}
			//float yaw, pitch, roll;
			//glm::extractEulerAngleYXZ(tfnode->m_tranform.m_rotation, yaw, pitch, roll);
			//// 转换为角度并输出
			//printf((char*)u8"i %d\tYaw: %d°\tPitch: %d°\tRoll: %d°\t\n", i, (int)glm::degrees(yaw), (int)glm::degrees(pitch), (int)glm::degrees(roll));

			if (node.weights.size()) {
				// todo node weights
				printf("\n");
			}
		}

		return;
	}
	void GLTFCommon::load_scenes()
	{
		auto& scenes = pm->scenes;
		m_scenes.resize(scenes.size());
		for (int i = 0; i < scenes.size(); i++)
		{
			auto& scene = scenes[i];
			for (int n = 0; n < scene.nodes.size(); n++)
			{
				int nodeId = scene.nodes[n];
				m_scenes[i].m_nodes.push_back(nodeId);
			}
		}
	}
	void GLTFCommon::load_skins()
	{
		auto& skins = pm->skins;
		m_skins.resize(skins.size());
		for (uint32_t i = 0; i < skins.size(); i++)
		{
			GetBufferDetails(skins[i].inverseBindMatrices, &m_skins[i].m_InverseBindMatrices);
			auto idx = skins[i].skeleton;
			m_skins[i].m_pSkeleton = &m_nodes[idx < 0 ? 0 : idx];

			auto& joints = skins[i].joints;
			for (uint32_t n = 0; n < joints.size(); n++)
			{
				m_skins[i].m_jointsNodeIdx.push_back(joints[n]);
			}
		}
	}
	void GLTFCommon::load_animations()
	{
		auto& animations = pm->animations;
		m_animations.resize(animations.size());
		for (int i = 0; i < animations.size(); i++)
		{
			auto& channels = animations[i].channels;
			auto& samplers = animations[i].samplers;
			tfAnimation* tfanim = &m_animations[i];
			for (int c = 0; c < channels.size(); c++)
			{
				auto& channel = channels[c];
				int sampler = channel.sampler;
				int node = channel.target_node;// GetElementInt(channel, "target/node", -1);
				std::string path = channel.target_path;// GetElementString(channel, "target/path", std::string());
				tfChannel* tfchannel;
				auto ch = tfanim->m_channels.find(node);
				if (ch == tfanim->m_channels.end())
				{
					tfchannel = &tfanim->m_channels[node];
				}
				else
				{
					tfchannel = &ch->second;
				}
				tfSampler* tfsmp = new tfSampler();
				// Get time line
				GetBufferDetails(samplers[sampler].input, &tfsmp->m_time);
				assert(tfsmp->m_time.m_stride == 4);
				tfanim->m_duration = std::max<float>(tfanim->m_duration, *(float*)tfsmp->m_time.Get(tfsmp->m_time.m_count - 1));
				// Get value line
				GetBufferDetails(samplers[sampler].output, &tfsmp->m_value);
				tfsmp->mstride = tfsmp->m_value.m_dimension;
				// Index appropriately
				if (path == "translation")
				{
					tfsmp->path = 0;
					assert(tfsmp->m_value.m_stride == 3 * 4);
					assert(tfsmp->m_value.m_dimension == 3);
				}
				else if (path == "rotation")
				{
					tfsmp->path = 1;
					assert(tfsmp->m_value.m_stride == 4 * 4);
					assert(tfsmp->m_value.m_dimension == 4);
				}
				else if (path == "scale")
				{
					tfsmp->path = 2;
					assert(tfsmp->m_value.m_stride == 3 * 4);
					assert(tfsmp->m_value.m_dimension == 3);
				}
				else if (path == "weights")
				{
					if (node >= 0)
					{
						auto& np = m_nodes[node];
						if (np.meshIndex >= 0) {
							auto& mp = m_meshes[np.meshIndex];
							tfsmp->mstride = mp.weights.size();
							tfsmp->weights = &mp.weights;
						}
					}
					tfsmp->path = 3;
					assert(tfsmp->m_value.m_stride == 4);
					assert(tfsmp->m_value.m_dimension == 1);
				}
				tfchannel->sampler[tfsmp->path] = tfsmp;
			}
		}
	}
	int GLTFCommon::get_animation_count()
	{
		return m_animations.size();
	}

	// Animates the matrices (they are still in object space)
	//loop animation
	void GLTFCommon::update(float time) {
		SetAnimationTime(_animationIndex, time);
	}
	void GLTFCommon::SetAnimationTime(uint32_t animationIndex, float time)
	{
		if (animationIndex < m_animations.size())
		{
			tfAnimation* anim = &m_animations[animationIndex];
			time = fmod(time, anim->m_duration);
			for (auto it = anim->m_channels.begin(); it != anim->m_channels.end(); it++)
			{
				auto np = &m_nodes[it->first];
				Transform* pSourceTrans = &m_nodes[it->first].m_tranform;
				Transform animated = {};
				float frac, * pCurr, * pNext;
				auto c = &it->second;
				animated.m_translation = pSourceTrans->m_translation;
				animated.m_rotation = pSourceTrans->m_rotation;
				animated.m_scale = pSourceTrans->m_scale;
				for (size_t k = 0; k < 4; k++) {
					auto& rb = acv[k];
					auto st = it->second.sampler[k];
					if (st)
					{
						// 插值类型支持：线性、步长、三次样条插值
						auto v4 = st->interpolator.interpolate(c, st, time, anim->m_duration, &rb);
						if (k == 0)
							animated.m_translation = v4;
						if (k == 1)
							animated.m_rotation = glm::mat4(glm::quat(v4.w, v4.x, v4.y, v4.z));
						if (k == 2)
							animated.m_scale = v4;
					}
				}
				if (it->second.sampler[3])
				{
					auto& va = m_animated_morphWeights[it->first];
					va.clear();
					va.swap(acv[3]);
				}
				// it->first是节点idx
				m_animatedMats[it->first] = animated.GetWorldMat();
			}
		}
	}

	uint8_t* GLTFCommon::buffer_view_data(size_t bvidx, size_t pos)
	{
		int32_t bufferViewIdx = bvidx;
		if (bufferViewIdx < 0)
			return 0;
		assert(bufferViewIdx >= 0);
		auto& bufferView = pm->bufferViews[bufferViewIdx];
		int32_t bufferIdx = bufferView.buffer;
		if (bufferIdx < 0)
			return 0;
		assert(bufferIdx >= 0);
		uint8_t* buffer = (uint8_t*)m_buffersData[bufferIdx];
		int32_t offset = bufferView.byteOffset;
		int byteLength = bufferView.byteLength;
		int32_t byteOffset = pos;
		offset += byteOffset;
		byteLength -= byteOffset;
		return buffer + offset;
	}
#if 1
	int GetFormatSize(int id)
	{
		switch (id)
		{
		case 5120: return 1; //(BYTE)
		case 5121: return 1; //(UNSIGNED_BYTE)1
		case 5122: return 2; //(SHORT)2
		case 5123: return 2; //(UNSIGNED_SHORT)2
		case 5124: return 4; //(SIGNED_INT)4
		case 5125: return 4; //(UNSIGNED_INT)4
		case 5126: return 4; //(FLOAT)
		}
		return -1;
	}

	int GetDimensions(const std::string& str)
	{
		if (str == "SCALAR")    return  1;
		else if (str == "VEC2") return  2;
		else if (str == "VEC3") return  3;
		else if (str == "VEC4") return  4;
		else if (str == "MAT4") return  4 * 4;
		else return  -1;
	}

	void SplitGltfAttribute(std::string attribute, std::string* semanticName, uint32_t* semanticIndex)
	{
		*semanticIndex = 0;

		if (isdigit(attribute.back()))
		{
			*semanticIndex = attribute.back() - '0';

			attribute.pop_back();
			if (attribute.back() == '_')
				attribute.pop_back();
		}

		*semanticName = attribute;
	}

	glm::vec4 GetVector(const njson::array_t& accessor)
	{
		return glm::vec4(accessor[0], accessor[1], accessor[2], (accessor.size() == 4) ? accessor[3].get<double>() : 0.0);
	}

	glm::mat4 GetMatrix(const njson::array_t& accessor)
	{
		return glm::mat4(
			glm::vec4(accessor[0], accessor[1], accessor[2], accessor[3]),
			glm::vec4(accessor[4], accessor[5], accessor[6], accessor[7]),
			glm::vec4(accessor[8], accessor[9], accessor[10], accessor[11]),
			glm::vec4(accessor[12], accessor[13], accessor[14], accessor[15]));
	}

	template <class type>
	type GetElement(const njson::object_t* pRoot, const char* path, type pDefault)
	{
		const char* p = path;
		char token[128];
		while (true)
		{
			for (; *p != '/' && *p != 0 && *p != '['; p++);
			memcpy(token, path, p - path);
			token[p - path] = 0;

			auto it = pRoot->find(token);
			if (it == pRoot->end())
				return pDefault;

			if (*p == '[')
			{
				p++;
				int i = atoi(p);
				for (; *p != 0 && *p != ']'; p++);
				pRoot = it->second.at(i).get_ptr<const njson::object_t*>();
				p++;
			}
			else
			{
				if (it->second.is_object())
					pRoot = it->second.get_ptr<const njson::object_t*>();
				else
				{
					return it->second.get<type>();
				}
			}
			p++;
			path = p;
		}

		return pDefault;
	}

	std::string GetElementString(const njson::object_t& root, const char* path, std::string pDefault)
	{
		return GetElement<std::string>(&root, path, pDefault);
	}

	bool GetElementBoolean(const njson::object_t& root, const char* path, bool def)
	{
		return GetElement<bool>(&root, path, def);
	}

	float GetElementFloat(const njson::object_t& root, const char* path, float def)
	{
		return GetElement<float>(&root, path, def);
	}

	int GetElementInt(const njson::object_t& root, const char* path, int def)
	{
		return GetElement<int>(&root, path, def);
	}

	njson::array_t GetElementJsonArray(const njson::object_t& root, const char* path, njson::array_t def)
	{
		return GetElement<njson::array_t>(&root, path, def);
	}

	glm::vec4 GetElementVector(njson::object_t& root, const char* path, glm::vec4 def)
	{
		if (root.find(path) != root.end() && !root[path].is_null())
		{
			return GetVector(root[path].get<njson::array_t>());
		}
		else
			return def;
	}

	static size_t component_read_index(const void* in, uint32_t component_type)
	{
		switch (component_type)
		{
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			return *((const uint16_t*)in);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			return *((const uint32_t*)in);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			return *((const uint8_t*)in);
		default:
			return 0;
		}
	}

	static size_t component_read_integer(const void* in, uint32_t component_type)
	{
		switch (component_type)
		{
		case TINYGLTF_COMPONENT_TYPE_SHORT:
			return *((const int16_t*)in);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			return *((const uint16_t*)in);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			return *((const uint32_t*)in);
		case TINYGLTF_COMPONENT_TYPE_BYTE:
			return *((const int8_t*)in);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			return *((const uint8_t*)in);
		default:
			return 0;
		}
	}
	static float component_read_float(const void* in, uint32_t component_type, bool normalized)
	{
		if (component_type == TINYGLTF_COMPONENT_TYPE_FLOAT)// component_type_r_32f)
		{
			return *((const float*)in);
		}

		if (normalized)
		{
			switch (component_type)
			{
				// note: glTF spec doesn't currently define normalized conversions for 32-bit integers
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				return *((const int16_t*)in) / (float)32767;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return *((const uint16_t*)in) / (float)65535;
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				return *((const int8_t*)in) / (float)127;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return *((const uint8_t*)in) / (float)255;
			default:
				return 0;
			}
		}

		return (float)component_read_integer(in, component_type);
	}

	//#define TINYGLTF_TYPE_VEC2 (2)
	//#define TINYGLTF_TYPE_VEC3 (3)
	//#define TINYGLTF_TYPE_VEC4 (4)
	//#define TINYGLTF_TYPE_MAT2 (32 + 2)
	//#define TINYGLTF_TYPE_MAT3 (32 + 3)
	//#define TINYGLTF_TYPE_MAT4 (32 + 4)
	//#define TINYGLTF_TYPE_SCALAR (64 + 1)
	//#define TINYGLTF_TYPE_VECTOR (64 + 4)
	//#define TINYGLTF_TYPE_MATRIX (64 + 16)
	static bool element_read_float(const uint8_t* element, uint32_t type, uint32_t component_type, bool normalized, float* out, size_t element_size)
	{
		size_t num_components = tinygltf::GetNumComponentsInType(type);

		if (element_size < num_components) {
			return 0;
		}

		// There are three special cases for component extraction, see #data-alignment in the 2.0 spec.

		size_t component_size = GetFormatSize(component_type);

		if (type == TINYGLTF_TYPE_MAT2 && component_size == 1)
		{
			out[0] = component_read_float(element, component_type, normalized);
			out[1] = component_read_float(element + 1, component_type, normalized);
			out[2] = component_read_float(element + 4, component_type, normalized);
			out[3] = component_read_float(element + 5, component_type, normalized);
			return 1;
		}

		if (type == TINYGLTF_TYPE_MAT4 && component_size == 1)
		{
			out[0] = component_read_float(element, component_type, normalized);
			out[1] = component_read_float(element + 1, component_type, normalized);
			out[2] = component_read_float(element + 2, component_type, normalized);
			out[3] = component_read_float(element + 4, component_type, normalized);
			out[4] = component_read_float(element + 5, component_type, normalized);
			out[5] = component_read_float(element + 6, component_type, normalized);
			out[6] = component_read_float(element + 8, component_type, normalized);
			out[7] = component_read_float(element + 9, component_type, normalized);
			out[8] = component_read_float(element + 10, component_type, normalized);
			return 1;
		}

		if (type == TINYGLTF_TYPE_MAT4 && component_size == 2)
		{
			out[0] = component_read_float(element, component_type, normalized);
			out[1] = component_read_float(element + 2, component_type, normalized);
			out[2] = component_read_float(element + 4, component_type, normalized);
			out[3] = component_read_float(element + 8, component_type, normalized);
			out[4] = component_read_float(element + 10, component_type, normalized);
			out[5] = component_read_float(element + 12, component_type, normalized);
			out[6] = component_read_float(element + 16, component_type, normalized);
			out[7] = component_read_float(element + 18, component_type, normalized);
			out[8] = component_read_float(element + 20, component_type, normalized);
			return 1;
		}

		for (size_t i = 0; i < num_components; ++i)
		{
			out[i] = component_read_float(element + component_size * i, component_type, normalized);
		}
		return 1;
	}
#endif
	void GLTFCommon::GetBufferDetails(int accessor_idx, tfAccessor* pAccessor)
	{
		if (pm)
		{
			auto& inAccessor = pm->accessors[accessor_idx];
			auto accessor = &inAccessor;
			do {
				// Second pass: write out each element in the sparse accessor.
				if (inAccessor.sparse.isSparse)
				{
					auto& sparse = inAccessor.sparse;
					uint8_t* index_data = buffer_view_data(sparse.indices.bufferView, sparse.indices.byteOffset);
					uint8_t* reader_head = buffer_view_data(sparse.values.bufferView, sparse.values.byteOffset);

					if (index_data == NULL || reader_head == NULL)
					{
						break;
					}
					auto dimension = tinygltf::GetNumComponentsInType(accessor->type);
					size_t floats_per_element = dimension;
					auto type = GetFormatSize(accessor->componentType);
					auto stride = dimension * type;
					size_t index_stride = GetFormatSize(sparse.indices.componentType);
					float* out = (float*)malloc(accessor->count * stride);
					if (!out)
						break;
					for (size_t i = 0; i < accessor->count * dimension; i++)
					{
						out[i] = 0.0f;
					}
					_sparseData.push_back(out);
					float* vd = (float*)reader_head;
					float* bd = (float*)out;
					auto values_size = sparse.count * dimension;
					auto base_data_size = accessor->count * dimension;
					size_t comp_count = tinygltf::GetNumComponentsInType(accessor->type);
					for (size_t i = 0; i < sparse.count; ++i) {
						uint32_t base_idx = component_read_integer(index_data + i * index_stride, sparse.indices.componentType);
						size_t value_start = i * comp_count;
						size_t base_start = base_idx * comp_count;
						if (value_start + comp_count > values_size || base_start + comp_count > base_data_size) {
							//throw std::runtime_error("稀疏数据越界！");
							continue;
						}
						float* writer_head = bd + base_start;
						if (!element_read_float(reader_head + i * stride, accessor->type, accessor->componentType, accessor->normalized, writer_head, floats_per_element))
						{
							continue;
						}
					}
					pAccessor->m_data = out;
					pAccessor->m_dimension = tinygltf::GetNumComponentsInType(inAccessor.type);
					pAccessor->m_type = GetFormatSize(inAccessor.componentType);
					pAccessor->m_stride = pAccessor->m_dimension * pAccessor->m_type;
					pAccessor->m_count = inAccessor.count;
					return;	// sparse数据
				}
			} while (0);
			pAccessor->m_data = buffer_view_data(inAccessor.bufferView, inAccessor.byteOffset);
			pAccessor->m_dimension = tinygltf::GetNumComponentsInType(inAccessor.type);
			pAccessor->m_type = GetFormatSize(inAccessor.componentType);
			pAccessor->m_stride = pAccessor->m_dimension * pAccessor->m_type;
			pAccessor->m_count = inAccessor.count;
			return;	// 正常数据
		}

	}

	void GLTFCommon::GetAttributesAccessors(const njson& gltfAttributes, std::vector<char*>* pStreamNames, std::vector<tfAccessor>* pAccessors)
	{
		int streamIndex = 0;
		for (int s = 0; s < pStreamNames->size(); s++)
		{
			auto attr = gltfAttributes.find(pStreamNames->at(s));
			if (attr != gltfAttributes.end())
			{
				tfAccessor accessor;
				GetBufferDetails(attr.value(), &accessor);
				pAccessors->push_back(accessor);
			}
		}
	}

	// Given a mesh find the skin it belongs to
	int GLTFCommon::FindMeshSkinId(int meshId) const
	{
		for (int i = 0; i < m_nodes.size(); i++)
		{
			if (m_nodes[i].meshIndex == meshId)
			{
				return m_nodes[i].skinIndex;
			}
		}

		return -1;
	}

	// given a skinId return the size of the skeleton matrices (vulkan needs this to compute the offsets into the uniform buffers)
	int GLTFCommon::GetInverseBindMatricesBufferSizeByID(int id) const
	{
		if (id == -1 || (id >= m_skins.size()))
			return -1;
		//return m_skins[id].m_InverseBindMatrices.m_count * (4 * 4 * sizeof(float));
		return m_skins[id].m_InverseBindMatrices.m_count * (sizeof(glm::mat4));// Matrix2));
	}

	// Transforms a node hierarchy recursively 
#if 0
	void GLTFCommon::TransformNodes(const glm::mat4& world, const std::vector<tfNodeIdx>* pNodes)
	{
		auto p = pNodes->data();
		for (uint32_t n = 0; n < pNodes->size(); n++)
		{
			uint32_t nodeIdx = p[n];
			glm::mat4 m = world * m_animatedMats[nodeIdx];
			m_worldSpaceMats[nodeIdx].Set(m);
			TransformNodes(m, &m_nodes[nodeIdx].m_children);
		}
	}
#else
	struct node_mat
	{
		uint32_t idx = 0;
		uint32_t pidx = (uint32_t)-1;
		glm::mat4 m = glm::mat4(1.0);
	};
	void GLTFCommon::TransformNodes(const glm::mat4& world, const std::vector<tfNodeIdx>* pNodes)
	{
		std::stack<node_mat> q;
		auto p = pNodes->data();
		for (uint32_t n = 0; n < pNodes->size(); n++)
		{
			uint32_t nodeIdx = p[n];
			if (nodeIdx >= m_nodes.size())
				continue;
			node_mat nm = {};
			nm.idx = nodeIdx;
			nm.m = world * m_animatedMats[nodeIdx];
			m_worldSpaceMats[nodeIdx].Set(nm.m);
			q.push(nm);
		}
		while (q.size()) {
			auto k = q.top(); q.pop();
			if (m_nodes[k.idx].m_children.size())
			{
				for (auto idx : m_nodes[k.idx].m_children)
				{
					node_mat nm = {};
					nm.idx = idx;
					nm.m = k.m * m_animatedMats[idx];
					m_worldSpaceMats[idx].Set(nm.m);
					q.push(nm);
				}
			}
		}
		return;
	}
	void GLTFCommon::make_mat(const glm::mat4& world, const std::vector<tfNodeIdx>* pNodes, std::vector<glm::mat4>* opt)
	{
		std::stack<node_mat> q;
		auto p = pNodes->data();
		if (opt->size() != m_nodes.size())
			opt->resize(m_nodes.size());
		auto pt = opt->data();
		for (uint32_t n = 0; n < pNodes->size(); n++)
		{
			uint32_t nodeIdx = p[n];
			if (nodeIdx >= m_nodes.size())
				continue;
			node_mat nm = {};
			nm.idx = nodeIdx;
			nm.m = world * m_animatedMats[nodeIdx];
			pt[nodeIdx] = nm.m;
			q.push(nm);
		}
		while (q.size()) {
			auto k = q.top(); q.pop();
			if (m_nodes[k.idx].m_children.size())
			{
				for (auto idx : m_nodes[k.idx].m_children)
				{
					node_mat nm = {};
					nm.idx = idx;
					nm.m = k.m * m_animatedMats[idx];
					pt[idx] = nm.m;
					q.push(nm);
				}
			}
		}
		return;
	}
#endif

	void mat2json(const glm::mat4& m, njson& a) {
		std::string str;
		for (int i = 0; i < 4; i++) {
			auto it = m[i];
			str += to_string_g(it.x) + ",";
			str += to_string_g(it.y) + ",";
			str += to_string_g(it.z) + ",";
			str += to_string_g(it.w) + "; ";
		}
		a = str;
	}

	void load_materials_variant(tinygltf::ExtensionMap& extensions, std::vector<std::string>& vns)
	{
		auto tt = get_ext(extensions, "KHR_materials_variants");
		if (tt)
		{
			if (tt->Has("variants"))
			{
				auto& vt = tt->Get("variants");
				auto vn = vt.Size();
				if (vt.IsArray())
				{
					for (size_t k = 0; k < vn; k++)
					{
						auto& vk = vt.Get(k);
						std::string name = vk.Get("name").Get<std::string>();
						vns.push_back(name);
					}
				}
			}
		}
	}

	// Initializes the GLTFCommonTransformed structure 
	void GLTFCommon::InitTransformedData()
	{
		if (m_nodes.empty() || mat_count == 0 || !pm)
			return;
		// initializes matrix buffers to have the same dimension as the nodes
		m_worldSpaceMats.resize(m_nodes.size());
		load_materials_variant(pm->extensions, variants);
		get_model_data(pm, _materialsData, _meshes, false);

		size_t dysize = sizeof(PerFrame_t) * 2;
		for (auto& it : _materialsData)
		{
			auto c = it.m_pbrMaterialParameters.uvc;
			dysize += (c * sizeof(glm::mat3x4));
		}

		auto& meshes = pm->meshes;
		_meshskin.clear();
		for (uint32_t i = 0; i < m_nodes.size(); i++)
		{
			auto& it = m_nodes[i];
			if (it.skinIndex >= 0 && it.skinIndex < m_skins.size()) {
				mat_count += m_skins[it.skinIndex].m_InverseBindMatrices.m_count;
			}
		}
		_mats.resize(mat_count);
		auto ptr = _mats.data();
		m_animatedMats = ptr;
		ptr += m_nodes.size();

		for (uint32_t i = 0; i < m_nodes.size(); i++)
		{
			auto& it = m_nodes[i];
			m_animatedMats[i] = it.m_tranform.GetWorldMat();
			if (it.meshIndex >= 0) {
				_meshskin.push_back({ it.skinIndex,it.meshIndex,i });
			}
		}
		for (auto& mt : _meshskin)
		{
			auto& mesh = meshes[mt.y];
			auto& weights = mesh.weights;
			auto a = mesh.primitives[0].targets.size();
			a = std::max(a, weights.size());
			if (a > 0) {
				auto& mw = m_animated_morphWeights[mt.z];
				dysize += a * sizeof(float);
				mw.resize(a);
			}
		}
		if (m_skins.size())
		{
			for (auto& mt : _meshskin)
			{
				glm::ivec2 k = mt;
				auto& it = m_worldSpaceSkeletonMats[k];
				it.m = ptr;
				if (mt.x >= 0 && mt.x < m_skins.size()) {
					it.count = m_skins[mt.x].m_InverseBindMatrices.m_count;
					ptr += it.count;
					dysize += it.count * sizeof(glm::mat4);
				}
			}
		}
		ubo_size = dysize;
		return;
	}

	// Takes the animated matrices and processes the hierarchy, also computes the skinning matrix buffers. 
	void GLTFCommon::TransformScene(int sceneIndex, const glm::mat4& world)
	{
		if (m_worldSpaceMats.size() < m_nodes.size())
		{
			m_worldSpaceMats.resize(m_nodes.size());
		}
		// transform all the nodes of the scene (and make 
		std::vector<int> sceneNodes = { m_scenes[sceneIndex].m_nodes };
		TransformNodes(world, &sceneNodes);
		//process skeletons, takes the skinning matrices from the scene and puts them into a buffer that the vertex shader will consume
#if 1
		auto num = m_skins.size();
		if (num > 0) {
			for (auto& mt : _meshskin)
			{
				if (mt.x < 0 || mt.x >= num)continue;
				glm::ivec2 k = mt;
				auto& it = m_worldSpaceSkeletonMats[k];
				auto m = m_worldSpaceMats[mt.z].GetCurrent();
				glm::mat4 inverseTransform = glm::inverse(m);
				glm::mat4* jointMatrix = it.m;
				tfSkins& skin = m_skins[mt.x];
				auto jointNode = skin.m_pSkeleton;
				glm::mat4* pM = (glm::mat4*)skin.m_InverseBindMatrices.m_data;
				for (int j = 0; j < skin.m_InverseBindMatrices.m_count; j++)
				{
					auto nidx = skin.m_jointsNodeIdx[j];
					auto& ibm = pM[j];
					auto jointMat = m_worldSpaceMats[nidx].GetCurrent() * ibm;
					jointMatrix[j] = inverseTransform * jointMat;
				}
			}
		}
#else
		for (uint32_t i = 0; i < m_skins.size(); i++)
		{
			tfSkins& skin = m_skins[i];
			//pick the matrices that affect the skin and multiply by the inverse of the bind      
			glm::mat4* pM = (glm::mat4*)skin.m_InverseBindMatrices.m_data;
			auto& skinningMats = m_worldSpaceSkeletonMats[i];
			for (int j = 0; j < skin.m_InverseBindMatrices.m_count; j++)
			{
				auto nidx = skin.m_jointsNodeIdx[j];
				skinningMats.m[j] = m_worldSpaceMats[nidx].GetCurrent() * pM[j];// todo Set
			}
		}
#endif
		return;
	}

	bool GLTFCommon::GetCamera(uint32_t cameraIdx, Camera* pCam) const
	{
		if (cameraIdx < 0 || cameraIdx >= m_cameras.size())
		{
			return false;
		}
		glm::mat4 m1 = m_worldSpaceMats[m_cameras[cameraIdx].m_nodeIndex].GetCurrent();
		pCam->SetMatrix(m1);
		pCam->SetFov(m_cameras[cameraIdx].yfov, pCam->GetAspectRatio(), m_cameras[cameraIdx].znear, m_cameras[cameraIdx].zfar);
		return true;
	}
	// Sets the per frame data from the GLTF, returns a pointer to it in case the user wants to override some values
	// The scene needs to be animated and transformed before we can set the PerFrame_t data. We need those final matrices for the lights and the camera.
	PerFrame_t* GLTFCommon::SetPerFrameData(PerFrame_t* pfd, const Camera& cam, std::vector<light_t>& lights, std::vector<glm::mat4>& lightMats)
	{
		if (pfd)
		{
			*_perFrameData = *pfd;
			_perFrameData->lightCount = (int32_t)m_lightInstances.size();
			int32_t ShadowMapIndex = 0;
			for (int i = 0; i < m_lightInstances.size(); i++)
			{
				Light* pSL = &_perFrameData->lights[i];
				*pSL = pfd->lights[m_lightInstances[i].m_lightId];
			}
		}
		return _perFrameData;
	}
	PerFrame_t* GLTFCommon::SetPerFrameData_w(PerFrame_t* pfd, const Camera& cam, std::vector<light_t>& lights, std::vector<glm::mat4>& lightMats)
	{
		if (pfd)
		{
			*_perFrameData_w = *pfd;
			_perFrameData_w->lightCount = (int32_t)m_lightInstances.size();
			int32_t ShadowMapIndex = 0;
			for (int i = 0; i < m_lightInstances.size(); i++)
			{
				Light* pSL = &_perFrameData_w->lights[i];
				*pSL = pfd->lights[m_lightInstances[i].m_lightId];
			}
		}
		return _perFrameData_w;
	}


	tfNodeIdx GLTFCommon::AddNode(const tfNode& node)
	{
		tfNodeIdx idx = 0;
		//m_nodes.push_back(node);
		//tfNodeIdx idx = (tfNodeIdx)(m_nodes.size() - 1);
		//m_scenes[0].m_nodes.push_back(idx);
		//auto tf = node.m_tranform;
		//m_animatedMats.push_back(tf.GetWorldMat());
		return idx;
	}

	int GLTFCommon::AddLight(const tfNode& node, const tfLight& light)
	{
#if 1
		return 0;
#else
		int nodeID = AddNode(node);
		m_lights.push_back(light);

		int lightInstanceID = (int)(m_lights.size() - 1);
		m_lightInstances.push_back({ lightInstanceID, (tfNodeIdx)nodeID });

		return lightInstanceID;
#endif
	}
	AxisAlignedBoundingBox GLTFCommon::get_BoundingBox(const glm::mat4& mLightView)
	{
		AxisAlignedBoundingBox projectedBoundingBox = {};
		// NOTE: we consider all objects here, but the scene might not display all of them.
		for (uint32_t i = 0; i < m_nodes.size(); ++i)
		{
			tfNode* pNode = &m_nodes.at(i);
			if ((pNode == NULL) || (pNode->meshIndex < 0))
				continue;

			std::vector<tfPrimitives>& primitives = m_meshes[pNode->meshIndex].m_pPrimitives;
			// loop through primitives
			//
			for (size_t p = 0; p < primitives.size(); ++p)
			{
				tfPrimitives boundingBox = m_meshes[pNode->meshIndex].m_pPrimitives[p];
				projectedBoundingBox.Merge(GetAABBInGivenSpace(mLightView * m_worldSpaceMats[i].GetCurrent(), boundingBox.m_center, boundingBox.m_radius));
			}
		}
		return projectedBoundingBox;
	}

	glm::mat4 ComputeDirectionalLight_mat(const AxisAlignedBoundingBox& projectedBoundingBox)
	{
		if (projectedBoundingBox.HasNoVolume())
		{
			// default ortho matrix that won't work in most cases
			return glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);
		}

		// we now have the final bounding box
		tfPrimitives finalBoundingBox;
		finalBoundingBox.m_center = 0.5f * (projectedBoundingBox.m_max + projectedBoundingBox.m_min);
		finalBoundingBox.m_radius = 0.5f * (projectedBoundingBox.m_max - projectedBoundingBox.m_min);

		finalBoundingBox.m_center.w = (1.0f);
		finalBoundingBox.m_radius.w = (0.0f);

		// we want a square aspect ratio
		float spanX = finalBoundingBox.m_radius.x;
		float spanY = finalBoundingBox.m_radius.y;
		float maxSpan = spanX > spanY ? spanX : spanY;

		//return glm::ortho(projectedBoundingBox.m_min.x, projectedBoundingBox.m_max.x, projectedBoundingBox.m_min.y, projectedBoundingBox.m_max.y, 0.1f, 100.0f);

		// manually create the orthographic matrix
		glm::mat4 projectionMatrix = {};//glm::mat4::identity();
		projectionMatrix[0] = (glm::vec4(
			1.0f / maxSpan, 0.0f, 0.0f, 0.0f
		));
		projectionMatrix[1] = (glm::vec4(
			0.0f, 1.0f / maxSpan, 0.0f, 0.0f
		));
		projectionMatrix[2] = (glm::vec4(
			0.0f, 0.0f, -0.5f / finalBoundingBox.m_radius.z, 0.0f
		));
		projectionMatrix[3] = (glm::vec4(
			-finalBoundingBox.m_center.x / maxSpan,
			-finalBoundingBox.m_center.y / maxSpan,
			0.5f * (finalBoundingBox.m_center.z + finalBoundingBox.m_radius.z) / finalBoundingBox.m_radius.z,
			1.0f
		));
		return projectionMatrix;
	}

	glm::mat4 GLTFCommon::ComputeDirectionalLightOrthographicMatrix(const glm::mat4& mLightView)
	{
		AxisAlignedBoundingBox projectedBoundingBox = get_BoundingBox(mLightView);
		return ComputeDirectionalLight_mat(projectedBoundingBox);
	}
#endif // 1




}
//! vkr 


namespace vkr {

	uint32_t getMemoryType(Device* dev, uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound)
	{

		auto memoryProperties = dev->GetPhysicalDeviceMemoryProperties();
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)
			{
				if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					if (memTypeFound)
					{
						*memTypeFound = true;
					}
					return i;
				}
			}
			typeBits >>= 1;
		}

#if defined(__ANDROID__)
		//todo : Exceptions are disabled by default on Android (need to add LOCAL_CPP_FEATURES += exceptions to Android.mk), so for now just return zero
		if (memTypeFound)
		{
			*memTypeFound = false;
		}
		return 0;
#else
		if (memTypeFound)
		{
			*memTypeFound = false;
			return 0;
		}
		else
		{
			throw std::runtime_error("Could not find a matching memory type");
		}
#endif
	}

	VkResult create_buffer(Device* dev, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags
		, size_t size, VkBuffer* buffer, VkDeviceMemory* memory, void* data, size_t* cap_size, void* _this)
	{

		auto device = dev->m_device;
		// Create the buffer handle
		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.usage = usageFlags;
		bufferCreateInfo.size = size;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (!(*buffer))
			(vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer));

		// Create the memory backing up the buffer handle
		VkMemoryRequirements memReqs;
		VkMemoryAllocateInfo memAlloc = {};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vkGetBufferMemoryRequirements(device, *buffer, &memReqs);
		//if (memReqs.size < 4096)
		//{
		//	memAlloc.allocationSize = 4096;
		//}
		//else
		{
			memAlloc.allocationSize = memReqs.size;
		}
		assert(memAlloc.allocationSize >= size);
		// Find a memory type index that fits the properties of the buffer
		VkBool32 memTypeFound = 0;
		memAlloc.memoryTypeIndex = getMemoryType(dev, memReqs.memoryTypeBits, memoryPropertyFlags, &memTypeFound);
#ifdef _WIN32
		printf("create_buffer\t[%p]\tallocationSize:%d\tsize:%d\n", _this, (int)memAlloc.allocationSize, (int)size);
#endif
		if (*memory)
			assert(0);
		auto hr = (vkAllocateMemory(device, &memAlloc, nullptr, memory));

		if (cap_size)
		{
			*cap_size = size;// memAlloc.allocationSize;
		}
		// If a pointer to the buffer data has been passed, map the buffer and copy over the data
		// 如果已传递指向缓冲区数据的指针，请映射缓冲区并在数据上进行复制
		if (data != nullptr && !(memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		{
			void* mapped = 0;
			(vkMapMemory(device, *memory, 0, size, 0, &mapped));
			memcpy(mapped, data, size);
			// 如果没有请求主机一致性，请手动刷新以使写入可见
			// If host coherency hasn't been requested, do a manual flush to make writes visible
			if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				VkMappedMemoryRange mappedRange = {};
				mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				mappedRange.memory = *memory;
				mappedRange.offset = 0;
				mappedRange.size = size;
				vkFlushMappedMemoryRanges(device, 1, &mappedRange);
			}
			vkUnmapMemory(device, *memory);
		}
		// Attach the memory to the buffer object
		hr = (vkBindBufferMemory(device, *buffer, *memory, 0));

		return hr;
	}
#ifndef dvkbuffer

	dvk_buffer::dvk_buffer(Device* dev, uint32_t usage, uint32_t mempro, uint32_t size, void* data) :_dev(dev), _size(size)
		, usageFlags(usage), memoryPropertyFlags(mempro)
	{
		assert(dev);
		if (dev)
		{
			descriptor = new VkDescriptorBufferInfo();
			device = dev->m_device;
			create_buffer(dev, usage, (VkMemoryPropertyFlags)mempro, size, &buffer, &memory, data, &_capacity, this);
		}
	}

	dvk_buffer::~dvk_buffer()
	{
		if (descriptor)delete descriptor; descriptor = 0;
		destroybuf();
	}
	void dvk_buffer::destroybuf()
	{
		unmap();
		if (buffer)
		{
			vkDestroyBuffer(device, buffer, nullptr);
			buffer = 0;
		}
		if (memory)
		{
			vkFreeMemory(device, memory, nullptr);
			memory = 0;
		}
		//buffer.clear();
		//descriptors.clear();
	}

	void dvk_buffer::resize(size_t size)
	{
		if (size > _capacity)
		{
			destroybuf();
			create_buffer(_dev, usageFlags, (VkMemoryPropertyFlags)memoryPropertyFlags, size, &buffer, &memory, nullptr, &_capacity, this);
		}
		_size = size;
		mapped = 0;
	}
#if 0
	{
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT = 0x00000001,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT = 0x00000002,
			VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT = 0x00000004,
			VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT = 0x00000008,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT = 0x00000010,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT = 0x00000020,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT = 0x00000040,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT = 0x00000080,
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT = 0x00000100,
			VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT = 0x00000800,
			VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT = 0x00001000,
			VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT = 0x00000200,
			VK_BUFFER_USAGE_RAY_TRACING_BIT_NV = 0x00000400,
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT = 0x00020000,
	}
#endif


	VkMemoryPropertyFlags get_mpflags(bool device_local, uint32_t* usage)
	{
		VkMemoryPropertyFlags mempro = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		if (device_local)
		{
			mempro = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			if (usage)
				*usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}
		return mempro;
	}
	dvk_buffer* dvk_buffer::create(Device* dev, uint32_t usage, uint32_t mempro, uint32_t size, void* data)
	{
		return new dvk_buffer(dev, usage, mempro, size, data);
	}
	dvk_buffer* dvk_buffer::new_texel(Device* dev, bool storage, uint32_t size, void* data)
	{
		uint32_t usage = (storage ? (uint32_t)VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT : (uint32_t)VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
		VkMemoryPropertyFlags mempro = get_mpflags(false, &usage);
		auto p = new dvk_buffer(dev, usage, mempro, size, data);

		return p;
	}
	dvk_buffer* dvk_buffer::new_staging(Device* dev, uint32_t size, void* data)
	{
		uint32_t usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VkMemoryPropertyFlags mempro = get_mpflags(false, &usage);
		auto p = new dvk_buffer(dev, usage, mempro, size, data);

		return p;
	}
	dvk_buffer* dvk_buffer::new_vbo(Device* dev, bool device_local, bool compute_rw, uint32_t size, void* data)
	{
		uint32_t usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		VkMemoryPropertyFlags mempro = get_mpflags(device_local, &usage);
		if (compute_rw)
		{
			usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		auto p = new dvk_buffer(dev, usage, mempro, size, data);

		p->descriptor->buffer = p->buffer;
		p->descriptor->offset = 0;
		p->descriptor->range = -1;
		return p;
	}
	dvk_buffer* dvk_buffer::new_indirect(Device* dev, bool device_local, uint32_t size, void* data)
	{
		uint32_t usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		VkMemoryPropertyFlags mempro = get_mpflags(device_local, &usage);
		auto p = new dvk_buffer(dev, usage, mempro, size, data);

		p->descriptor->buffer = p->buffer;
		p->descriptor->offset = 0;
		p->descriptor->range = -1;
		return p;
	}
	dvk_buffer* dvk_buffer::new_ibo(Device* dev, int type, bool device_local, uint32_t count, void* data)
	{
		uint32_t usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		uint32_t indexBufferSize = count * (type ? sizeof(uint32_t) : sizeof(uint16_t));
		VkMemoryPropertyFlags mempro = get_mpflags(device_local, &usage);

		auto p = new dvk_buffer(dev, usage, mempro, indexBufferSize, data);
		p->descriptor->buffer = p->buffer;
		p->descriptor->offset = 0;
		p->descriptor->range = -1;

		return p;
	}
	// todo new ubo
	dvk_buffer* dvk_buffer::new_ubo(Device* dev, uint32_t size, void* data, uint32_t usage1)
	{
		uint32_t usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | usage1;
		VkMemoryPropertyFlags mempro = get_mpflags(false, &usage);
		auto p = new dvk_buffer(dev, usage, mempro, size, data);
		p->descriptor->buffer = p->buffer;
		p->descriptor->offset = 0;
		p->descriptor->range = -1;

		return p;
	}

	dvk_buffer* dvk_buffer::new_ssbo(Device* dev, uint32_t size, void* data)
	{
		uint32_t usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		VkMemoryPropertyFlags mempro = get_mpflags(false, &usage);
		auto p = new dvk_buffer(dev, usage, mempro, size, data);
		p->descriptor->buffer = p->buffer;
		p->descriptor->offset = 0;
		p->descriptor->range = -1;
		return p;
	}

	void dvk_buffer::copy_buffer(dvk_buffer* dst, dvk_buffer* src, uint64_t dst_offset, uint64_t src_offset, int64_t size)
	{
#if 0
		assert(dst->_size <= src->_size);
		auto dev = (Device*)dst->_dev;
		auto devs = (Device*)src->_dev;
		if (dev != devs)
		{
			assert(_dev == devs);
			return;
		}
		auto device = dev->m_device;
		auto qctx = _dev->get_graphics_queue(1);
		auto cp = qctx->new_cmd_pool();
		auto copyQueue = qctx->get_vkptr();
		VkCommandBuffer copyCmd = vkc::createCommandBuffer1(device, cp->command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkBufferCopy bufferCopy = {};

		bufferCopy.size = (size > 0) ? size : src->_size;
		if (dst->_size < bufferCopy.size)
		{
			bufferCopy.size = dst->_size;
			assert(dst->_size >= bufferCopy.size);
		}
		bufferCopy.dstOffset = dst_offset;
		bufferCopy.srcOffset = src_offset;

		vkCmdCopyBuffer(copyCmd, src->buffer, dst->buffer, 1, &bufferCopy);
		vkc::flushCommandBuffer(device, copyCmd, cp->command_pool, copyQueue, true);
		qctx->free_cmd_pool(cp);
#endif
	}
	void dvk_buffer::setDesType(uint32_t dt)
	{
		dtype = (VkDescriptorType)dt;
	}

	bool dvk_buffer::make_data(void* data, size_t size, size_t offset, bool isun)
	{
		map(size, offset);
		if (mapped)
		{
			memcpy(mapped, data, size);
			if (isun)unmap();
			return true;
		}
		return false;
	}
	size_t dvk_buffer::set_data(void* data, size_t size, size_t offset, bool is_flush, bool iscp)
	{
		if (_size < size)
			return 0;
		if (!mapped)map(_size, 0);
		assert(mapped);
		if (iscp)
			memcpy((char*)mapped + offset, data, size);
		if (is_flush)
		{
			flush(size, offset);
		}
		if (fpos > offset)
		{
			fpos = offset;
		}
		if (fsize < size + offset)
		{
			fsize = size + offset;
		}
		return size + offset;
	}

	void dvk_buffer::copy_to(void* data, size_t size)
	{
		set_data(data, size, 0, false);
	}

	//uint32_t dvk_buffer::bind(VkDeviceSize offset)
	//{
	//	return vkBindBufferMemory(device, buffer, memory, offset);
	//}


	void* dvk_buffer::map(VkDeviceSize m_size, VkDeviceSize offset)
	{
		if (mapped)return mapped;
		if (m_size > _size || m_size == 0)
		{
			m_size = _size;
		}
		VkResult r = vkMapMemory(device, memory, offset, m_size, 0, &mapped);
		assert(r == VK_SUCCESS);
		return mapped;
	}
	uint32_t dvk_buffer::flush(VkDeviceSize size_, VkDeviceSize offset)
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size_ > 0 ? size_ : -1;
		return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
	}
	uint32_t dvk_buffer::flush()
	{
		auto ret = flush(fsize, fpos);
		fpos = -1;
		fsize = 0;
		return ret;
	}
	void dvk_buffer::unmap()
	{
		if (mapped)
		{
			vkUnmapMemory(device, memory);
			mapped = nullptr;
		}
	}


	void* dvk_buffer::get_map(VkDeviceSize offset)
	{
		char* p = (char*)mapped;
		assert(p);
		return p + offset;
	}

	uint32_t dvk_buffer::invalidate(VkDeviceSize size_, VkDeviceSize offset)
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size_;
		return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
	}

#endif // !dvkbuffer

	// todo dynamic_buffer

	dynamic_buffer_cx::dynamic_buffer_cx(Device* d, size_t acsize, bool vibo, bool transfer) :dev(d)
	{
		assert(dev);
		if (dev)
		{
			_ubo_align = 512;// dev->get_ubo_align();
			if (!acsize)
			{
				acsize = 64 * 1024;// dev->get_ubo_range();
			}
			uint32_t usage = 0;
			if (vibo)
			{
				usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			}
			if (transfer)
			{
				usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			}
			size_t a = 512;
			acsize = AlignUp(acsize, a);
			_ubo = dvk_buffer::new_ubo(dev, acsize, 0, usage);
			clear();
		}
	}

	dynamic_buffer_cx::~dynamic_buffer_cx()
	{
		if (_ubo)delete _ubo; _ubo = 0;
	}

	void dynamic_buffer_cx::append(size_t size)
	{
		size_t a = 512;
		size = AlignUp(size, a);
		_ubo->resize(_ubo->_size + size);
		mdata = (char*)_ubo->map(-1);
		assert(mdata);
	}

	char* dynamic_buffer_cx::get_ptr(uint32_t offset)
	{
		assert(mdata);
		return mdata + offset;
	}

	void dynamic_buffer_cx::flush(size_t pos, size_t size)
	{
		VkResult res = {};
		VkMappedMemoryRange range[1] = {};
		range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[0].memory = _ubo->memory;
		range[0].size = size;
		res = vkFlushMappedMemoryRanges(dev->m_device, 1, range);
		assert(res == VK_SUCCESS);
	}

	void dynamic_buffer_cx::clear()
	{
		last = 0;
		if (!mdata)
			mdata = (char*)_ubo->map(-1);
	}

	void dynamic_buffer_cx::free_mem()
	{
		_ubo->destroybuf();
		mdata = 0;
	}

	char* dynamic_buffer_cx::alloc(size_t size, uint32_t* offset)
	{
		assert(mdata);
		uint32_t r = last;
		size_t a = _ubo_align;
		alignUp(size, a);
		auto nac = r + size;
		if (nac < 0 || nac > _ubo->_size)
		{
			assert("'dynamic' buffers空间不足!");
			return 0;
		}
		auto ret = mdata + r;
		if (offset) {
			*offset = r;
		}
		last = nac;
		return ret;
	}

	bool dynamic_buffer_cx::AllocConstantBuffer(uint32_t size, void** pData, VkDescriptorBufferInfo* pOut)
	{
		//size = AlignUp(size, 64u);

		uint32_t memOffset = 0;
		*pData = alloc(size, &memOffset);
		if (!*pData)
		{
			assert("Ran out of mem for 'dynamic' buffers, please increase the allocated size");
			return false;
		}

		pOut->buffer = _ubo->buffer;
		pOut->offset = memOffset;
		pOut->range = size;

		return true;
	}


	// todo dvk_staging_buffer

	dvk_staging_buffer::dvk_staging_buffer()
	{
	}

	dvk_staging_buffer::~dvk_staging_buffer()
	{
		freeBuffer();
	}

	void dvk_staging_buffer::freeBuffer()
	{
		if (isd)
		{
			// Clean up staging resources
			if (mem)
				vkFreeMemory(_dev, mem, nullptr);
			if (buffer)
				vkDestroyBuffer(_dev, buffer, nullptr);
		}
		mem = 0;
		buffer = 0;
	}

	void dvk_staging_buffer::initBuffer(Device* dev, VkDeviceSize size)
	{
		_dev = dev->m_device;
		if (bufferSize != size)
		{
			bufferSize = size;
		}
		if (size > memSize)
		{
			freeBuffer();
		}
		// Create a host-visible staging buffer that contains the raw image data
		if (!buffer)
		{
			VkMemoryAllocateInfo memAllocInfo = {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			VkMemoryRequirements memReqs;

			VkBufferCreateInfo bufferCreateInfo = {};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = bufferSize;
			// This buffer is used as a transfer source for the buffer copy
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			auto hr = (vkCreateBuffer(_dev, &bufferCreateInfo, nullptr, &buffer));

			// Get memory requirements for the staging buffer (alignment, memory type bits)
			vkGetBufferMemoryRequirements(_dev, buffer, &memReqs);
			memSize = memReqs.size;
			memAllocInfo.allocationSize = memReqs.size;
			// Get memory type index for a host visible buffer
			VkBool32 memTypeFound = 0;
			memAllocInfo.memoryTypeIndex = getMemoryType(dev, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memTypeFound);

			hr = (vkAllocateMemory(_dev, &memAllocInfo, nullptr, &mem));
			hr = (vkBindBufferMemory(_dev, buffer, mem, 0));

		}
	}

	char* dvk_staging_buffer::map()
	{
		if (!mapped)
			(vkMapMemory(_dev, mem, 0, memSize, 0, (void**)&mapped));
		return (char*)mapped;
	}
	void  dvk_staging_buffer::unmap()
	{
		assert(mem);
		if (!mem)return;
		// 如果没有请求主机一致性，请手动刷新以使写入可见
		// If host coherency hasn't been requested, do a manual flush to make writes visible
		//if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
		{
			VkMappedMemoryRange mappedRange = {};
			mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedRange.memory = mem;
			mappedRange.offset = 0;
			mappedRange.size = memSize;
			vkFlushMappedMemoryRanges(_dev, 1, &mappedRange);
		}
		vkUnmapMemory(_dev, mem);
	}

	void dvk_staging_buffer::copyToBuffer(void* data, size_t bsize)
	{
		// Copy texture data into staging buffer
		if (data)
		{
			auto hr = (vkMapMemory(_dev, mem, 0, memSize, 0, (void**)&mapped));
			memcpy(mapped, data, bsize);
			// 如果没有请求主机一致性，请手动刷新以使写入可见
			// If host coherency hasn't been requested, do a manual flush to make writes visible
			//if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				VkMappedMemoryRange mappedRange = {};
				mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				mappedRange.memory = mem;
				mappedRange.offset = 0;
				mappedRange.size = bsize;
				vkFlushMappedMemoryRanges(_dev, 1, &mappedRange);
			}
			vkUnmapMemory(_dev, mem);
		}
	}

	void dvk_staging_buffer::copyGetBuffer(std::vector<char>& outbuf)
	{
		outbuf.resize(bufferSize);
		// Copy texture data into staging buffer
		uint8_t* data;
		auto hr = (vkMapMemory(_dev, mem, 0, memSize, 0, (void**)&data));
		memcpy(outbuf.data(), data, bufferSize);
		vkUnmapMemory(_dev, mem);
	}

	size_t dvk_staging_buffer::getBufSize()
	{
		return memSize;
	}

	void dvk_staging_buffer::getBuffer(char* outbuf, size_t len)
	{
		len = std::min((size_t)bufferSize, len);
		uint8_t* data;
		auto hr = (vkMapMemory(_dev, mem, 0, memSize, 0, (void**)&data));
		memcpy(outbuf, data, len);
		vkUnmapMemory(_dev, mem);
	}
	// todo upload_cx
	upload_cx::upload_cx()
	{
	}

	upload_cx::~upload_cx()
	{
		on_destroy();
	}
	void upload_cx::init(Device* dev, size_t size, int idxqueue)
	{
		if (_dev)return;
		_dev = dev;
		assert(_dev);
		ncap = size;
		VkResult res;
		auto device = dev->m_device;
		db = new dynamic_buffer_cx(dev, size, false, true);
		// 获取列队

		// Create command list and allocators 
		{
			VkCommandPoolCreateInfo cmd_pool_info = {};
			cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmd_pool_info.queueFamilyIndex = _dev->GetGraphicsQueueFamilyIndex();// get_familyIndex(0);// GetGraphicsQueueFamilyIndex();
			cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			res = vkCreateCommandPool(device, &cmd_pool_info, NULL, &_commandPool);
			assert(res == VK_SUCCESS);

			VkCommandBufferAllocateInfo cmd = {};
			cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmd.pNext = NULL;
			cmd.commandPool = _commandPool;
			cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmd.commandBufferCount = 1;
			res = vkAllocateCommandBuffers(device, &cmd, &_pCommandBuffer);
			assert(res == VK_SUCCESS);
		}
		// Create fence
		{
			VkFenceCreateInfo fence_ci = {};
			fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			res = vkCreateFence(device, &fence_ci, NULL, &_fence);
			assert(res == VK_SUCCESS);
		}

		{
			//VkCommandBufferBeginInfo cmd_buf_info = {};
			//cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			//res = vkBeginCommandBuffer(_pCommandBuffer, &cmd_buf_info);
			//assert(res == VK_SUCCESS);
		}
	}
	void upload_cx::on_destroy()
	{
		//delop(db);
		if (db)delete db; db = 0;
		//vkDestroyBuffer(_dev->device, _buffer, NULL);
		//vkUnmapMemory(_dev->device, _deviceMemory);
		//vkFreeMemory(_dev->device, _deviceMemory, NULL);
		if (_commandPool && _pCommandBuffer)
		{
			vkFreeCommandBuffers(_dev->m_device, _commandPool, 1, &_pCommandBuffer);
			vkDestroyCommandPool(_dev->m_device, _commandPool, NULL);
			_commandPool = {}; _pCommandBuffer = {};
		}
		if (_fence)
		{
			vkDestroyFence(_dev->m_device, _fence, NULL);
			_fence = {};
		}
	}
	char* upload_cx::get_tbuf(size_t size, size_t uAlign)
	{
		auto ns = ncap - (last_pos + last_size);
		flush();
		if (!uAlign)uAlign = 512;
		size = AlignUp(size, uAlign);
		if (ns < size)
		{
			flushAndFinish();
		}
		if (ncap < size)
		{
			db->append(size - ncap);
		}
		uint32_t ps = 0;
		auto d = db->alloc(size, &ps);
		last_pos = ps;
		last_size = size;
		ups = size;
		return d;
	}
	void upload_cx::AddPreBarrier(VkImageMemoryBarrier imb)
	{
		toPreBarrier.push_back(imb);
	}
	void upload_cx::addCopy(VkImage image, VkBufferImageCopy bic)
	{
		_copies.push_back({ image,bic });
	}
	void upload_cx::AddPostBarrier(VkImageMemoryBarrier imb)
	{
		toPostBarrier.push_back(imb);
	}
	void upload_cx::flush()
	{
		if (ups > 0)
		{
			db->flush(last_pos, last_size); ups = 0;
		}
	}
	int upload_cx::flushAndFinish(int wait_time)
	{
		if (cp2m.empty() && cp2img.empty() && toPreBarrier.empty())return 0;
		// Reset so it can be reused
		flush();	// 刷新数据缓存
		auto cbf = get_cmdbuf();
		auto bf = get_resource();
		VkResult res;

		//vkResetFences(_dev->device, 1, &_fence);

		cmd_begin();

		if (toPreBarrier.size() > 0)
		{
			vkCmdPipelineBarrier(cbf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
				, 0, 0, NULL, 0, NULL, (uint32_t)toPreBarrier.size(), toPreBarrier.data());
		}

		for (auto c : _copies)
		{
			vkCmdCopyBufferToImage(cbf, bf, c._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &c._bic);
		}
		for (auto it : _clear_cols)
		{
			VkImageSubresourceRange srRange = {};
			srRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			srRange.baseMipLevel = 0;
			srRange.levelCount = VK_REMAINING_MIP_LEVELS;
			srRange.baseArrayLayer = 0;
			srRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
			VkClearColorValue ccv;
			ccv.float32[0] = it.c[0];
			ccv.float32[1] = it.c[1];
			ccv.float32[2] = it.c[2];
			ccv.float32[3] = it.c[3];

			vkCmdClearColorImage(cbf, it.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ccv, 1, &srRange);

		}
		//apply post barriers in one go
		if (toPostBarrier.size() > 0)
		{
			vkCmdPipelineBarrier(cbf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				, 0, 0, NULL, 0, NULL, (uint32_t)toPostBarrier.size(), toPostBarrier.data());
		}

		// 复制纹理到内存
		VkPipelineStageFlags mask2 = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		if (cp2m.size())
		{
			for (auto& it : cp2m)
			{
				vkCmdPipelineBarrier(cbf, mask2, mask2, 0, 0, nullptr, 0, nullptr, 1, &it.preb);
			}
			for (auto& it : cp2m)
			{
				vkCmdCopyImageToBuffer(cbf, it.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, it.buffer, 1, &it.icp);
			}
			for (auto& it : cp2m)
			{
				vkCmdPipelineBarrier(cbf, mask2, mask2, 0, 0, nullptr, 0, nullptr, 1, &it.postb);
			}
		}
		// 复制纹理到纹理
		if (cp2img.size())
		{
			for (auto& it : cp2img)
			{
				vkCmdPipelineBarrier(cbf, mask2, mask2, 0, 0, nullptr, 0, nullptr, 1, &it.preb);
			}
			for (auto& it : cp2img)
			{
				vkCmdCopyImage(cbf, it.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, it.dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &it.icp);
			}
			for (auto& it : cp2img)
			{
				vkCmdPipelineBarrier(cbf, mask2, mask2, 0, 0, nullptr, 0, nullptr, 1, &it.postb);
			}
		}


		// Close 
		cmd_end();
#if 0
		if (_queue)
		{
			a = 0;
			_queue->push(_pCommandBuffer, _fence, &a);
			int r = 0;
			do {
				// 等待GPU返回
				print_time Pt("vkQueueSubmit upload", isprint);
				res = _queue->wait_status(_fence, &a, wait_time);
				assert(res == VK_SUCCESS);
				if (res != VK_SUCCESS)
				{
					r = -7;
					break;
				}

			} while (0);
		}
#else
		{
			// 等待GPU返回
			print_time Pt("vkQueueSubmit upload", isprint);
			VkSubmitInfo submit_info;
			submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.pNext = NULL;
			submit_info.waitSemaphoreCount = 0;
			submit_info.pWaitSemaphores = NULL;
			submit_info.pWaitDstStageMask = NULL;
			submit_info.commandBufferCount = 1;
			submit_info.pCommandBuffers = &_pCommandBuffer;
			submit_info.signalSemaphoreCount = 0;
			submit_info.pSignalSemaphores = NULL;
			auto st = vkGetFenceStatus(_dev->m_device, _fence);
			printf("upload fence: %d\n", st);
			res = vkQueueSubmit(_dev->graphics_queue, 1, &submit_info, _fence);
			assert(res == VK_SUCCESS);
			vkWaitForFences(_dev->m_device, 1, &_fence, VK_TRUE, UINT64_MAX);
			vkResetFences(_dev->m_device, 1, &_fence);
		}
#endif

		toPreBarrier.clear();
		_copies.clear();
		_clear_cols.clear();
		toPostBarrier.clear();
		cp2m.clear();
		db->clear();
		last_size = last_pos = ups = 0;
		return res;
	}

	void upload_cx::addClear(VkImage image, glm::vec4 color)
	{
		_clear_cols.push_back({ image,color });
	}

	void upload_cx::free_buf()
	{
		ncap = 0;
		db->free_mem();
	}

	VkBuffer upload_cx::get_resource() { return db->_ubo->buffer; }
	VkCommandBuffer upload_cx::get_cmdbuf() { return _pCommandBuffer; }

	void upload_cx::cmd_begin()
	{
		VkCommandBufferBeginInfo cmd_buf_info = {};
		cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		auto res = vkBeginCommandBuffer(_pCommandBuffer, &cmd_buf_info);
		assert(res == VK_SUCCESS);
	}

	void upload_cx::cmd_end()
	{
		auto res = vkEndCommandBuffer(_pCommandBuffer);
		assert(res == VK_SUCCESS);
	}

	void upload_cx::add_pre(VkImage image, VkFormat format, uint32_t aspectMask, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevel, uint32_t layerCount)
	{
		VkImageMemoryBarrier copy_barrier = {};
		copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		copy_barrier.oldLayout = oldLayout;
		copy_barrier.newLayout = newLayout;
		copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copy_barrier.image = image;
		copy_barrier.subresourceRange.aspectMask = aspectMask;
		copy_barrier.subresourceRange.levelCount = mipLevel;
		copy_barrier.subresourceRange.layerCount = layerCount;

		toPreBarrier.push_back(copy_barrier);
	}

	void upload_cx::add_post(VkImage image, VkFormat format, uint32_t aspectMask, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevel, uint32_t layerCount)
	{
		VkImageMemoryBarrier use_barrier = {};
		use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		use_barrier.oldLayout = oldLayout;
		use_barrier.newLayout = newLayout;
		use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		use_barrier.image = image;
		use_barrier.subresourceRange.aspectMask = aspectMask;
		use_barrier.subresourceRange.levelCount = mipLevel;
		use_barrier.subresourceRange.layerCount = layerCount;
		toPostBarrier.push_back(use_barrier);
	}

	VkImageMemoryBarrier get_layout(
		VkImage image,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange);

	void upload_cx::add_copy2mem(VkImage image, VkBufferImageCopy icp, VkImageSubresourceRange subresourceRange, VkImageAspectFlags aspectMask, VkImageLayout il, VkBuffer buffer)
	{
		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy
		auto preb = get_layout(image, aspectMask, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);
		// Change texture image layout to shader read after all mip levels have been copied
		auto postb = get_layout(image, aspectMask, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, il, subresourceRange);
		cp2m.push_back({ image,  icp,  preb,  postb,  buffer });
	}
	void upload_cx::add_copy2img(VkImage image, VkImageCopy icp, VkImageSubresourceRange subresourceRange, VkImageAspectFlags aspectMask, VkImageLayout il, VkImage buffer)
	{
		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy
		auto preb = get_layout(image, aspectMask, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);
		// Change texture image layout to shader read after all mip levels have been copied
		auto postb = get_layout(image, aspectMask, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, il, subresourceRange);
		cp2img.push_back({ image,  icp,  preb,  postb,  buffer });
	}

	VkImageMemoryBarrier get_layout(
		VkImage image,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange)
	{
		// Create an image barrier object
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		// Source layouts (old)
		// Source access mask controls actions that have to be finished on the old layout
		// before it will be transitioned to the new layout
		switch (oldImageLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			imageMemoryBarrier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch (newImageLayout)
		{
		case VK_IMAGE_LAYOUT_GENERAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from and writes to the image have been finished
			imageMemoryBarrier.srcAccessMask = imageMemoryBarrier.srcAccessMask | VK_ACCESS_TRANSFER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (imageMemoryBarrier.srcAccessMask == 0)
			{
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		}

		return imageMemoryBarrier;
	}


	void transfer_cx::copy_image(Texture* image, Texture* dst, VkImageLayout src_layout, VkImageLayout dst_layout)
	{
		if (!image || !dst)
		{
			assert("copy_image: image or dst is null");
			return;
		}
		copy_image(image->Resource(), dst->Resource(), image->GetWidth(), image->GetHeight()
			, src_layout, dst_layout, (VkImageAspectFlags)(is_depth_tex((VkFormat)image->GetFormat()) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT)
			, 0, image->GetMipCount(), image->GetArraySize());
	}

	void transfer_cx::copy_image(VkImage image, VkImage dst, int width, int height, VkImageLayout src_layout, VkImageLayout dst_layout, VkImageAspectFlags aspectMask, int mipLevel, int mipLevels, int layerCount)
	{
		if (!aspectMask)aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		VkImageCopy cr = {};
		cr.dstOffset = {};
		cr.dstSubresource.aspectMask = aspectMask;
		cr.dstSubresource.mipLevel = mipLevel;
		cr.dstSubresource.baseArrayLayer = 0;
		cr.dstSubresource.layerCount = layerCount;

		cr.extent.width = width;
		cr.extent.height = height;
		cr.extent.depth = 1;

		cr.srcOffset = {};
		cr.srcSubresource = cr.dstSubresource;

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = aspectMask;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipLevels;
		subresourceRange.layerCount = layerCount;
		add_copy2img(image, cr, subresourceRange, aspectMask, src_layout, dst_layout, dst);
	}

	void transfer_cx::add_copy2mem(VkImage image, VkBufferImageCopy icp, VkImageSubresourceRange subresourceRange, VkImageAspectFlags aspectMask, VkImageLayout src_layout, VkImageLayout dst_layout, VkBuffer buffer)
	{
		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy
		auto preb = get_layout(image, aspectMask, src_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);
		// Change texture image layout to shader read after all mip levels have been copied
		auto postb = get_layout(image, aspectMask, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_layout, subresourceRange);
		cp2m.push_back({ image,  icp,  preb,  postb,  buffer });
	}
	void transfer_cx::add_copy2img(VkImage image, VkImageCopy icp, VkImageSubresourceRange subresourceRange, VkImageAspectFlags aspectMask, VkImageLayout src_layout, VkImageLayout dst_layout, VkImage buffer)
	{
		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy
		auto preb = get_layout(image, aspectMask, src_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);
		// Change texture image layout to shader read after all mip levels have been copied
		auto postb = get_layout(image, aspectMask, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_layout, subresourceRange);
		cp2img.push_back({ image,  icp,  preb,  postb,  buffer });
	}
	void transfer_cx::flush(VkCommandBuffer cmd1)
	{
		if (cp2m.empty() && cp2img.empty())return;
		VkResult res;
		// 复制纹理到内存
		VkPipelineStageFlags mask2 = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		if (cp2m.size())
		{
			for (auto& it : cp2m)
			{
				vkCmdPipelineBarrier(cmd1, mask2, mask2, 0, 0, nullptr, 0, nullptr, 1, &it.preb);
			}
			for (auto& it : cp2m)
			{
				vkCmdCopyImageToBuffer(cmd1, it.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, it.buffer, 1, &it.icp);
			}
			for (auto& it : cp2m)
			{
				vkCmdPipelineBarrier(cmd1, mask2, mask2, 0, 0, nullptr, 0, nullptr, 1, &it.postb);
			}
			cp2m.clear();
		}
		// 复制纹理到纹理
		if (cp2img.size())
		{
			for (auto& it : cp2img)
			{
				vkCmdPipelineBarrier(cmd1, mask2, mask2, 0, 0, nullptr, 0, nullptr, 1, &it.preb);
			}
			for (auto& it : cp2img)
			{
				vkCmdCopyImage(cmd1, it.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, it.dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &it.icp);
			}
			for (auto& it : cp2img)
			{
				vkCmdPipelineBarrier(cmd1, mask2, mask2, 0, 0, nullptr, 0, nullptr, 1, &it.postb);
			}
			cp2img.clear();
		}
	}






	// todo texture

	dvk_texture::dvk_texture()
	{
		descriptor = new VkDescriptorImageInfo();
		_info = new VkImageCreateInfo();
		//_header = new IMG_INFO();
	}

	dvk_texture::dvk_texture(Device* dev) :_dev(dev)
	{
		descriptor = new VkDescriptorImageInfo();
		_info = new VkImageCreateInfo();
	}

	dvk_texture::~dvk_texture()
	{
		if (ycbcr_sampler_conversion)
		{
			//_dev->destroy_samplerYcbcrConversion(_dev->device, ycbcr_sampler_conversion, 0);
			//vkDestroySamplerYcbcrConversion(_dev->device, ycbcr_sampler_conversion, 0);
		}
		//free_image();
		//delop(_header);
		//delop(_info);
		//delop(descriptor);
	}

	uint32_t GetImageLayout(ImageLayoutBarrier target)
	{
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

		switch (target)
		{
		case ImageLayoutBarrier::UNDEFINED:
		{
			layout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
		break;

		case ImageLayoutBarrier::TRANSFER_DST:
		{
			layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		}
		break;

		case ImageLayoutBarrier::COLOR_ATTACHMENT:
		{
			layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		break;

		case ImageLayoutBarrier::DEPTH_STENCIL_ATTACHMENT:
		{
			layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		break;

		case ImageLayoutBarrier::TRANSFER_SRC:
		{
			layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		}
		break;

		case ImageLayoutBarrier::PRESENT_SRC:
		{
			layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}
		break;

		case ImageLayoutBarrier::SHADER_READ:
		{
			layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		break;

		case ImageLayoutBarrier::DEPTH_STENCIL_READ:
		{
			layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		}
		break;

		case ImageLayoutBarrier::PixelGeneralRW:
		case ImageLayoutBarrier::ComputeGeneralRW:
		{
			layout = VK_IMAGE_LAYOUT_GENERAL;
		}
		break;
		default:
		{
			//MLOGE("Unknown ImageLayoutBarrier %d", (int32)target);
		}
		break;
		}

		return layout;
	}

	// TODO Barrier

	// This method is based on https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.h#L88
	void vkr_image_set_layout_subres(VkCommandBuffer cmdBuff, VkImage image, VkImageSubresourceRange subresourceRange,
		VkImageLayout old_image_layout, VkImageLayout new_image_layout,
		VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages) {
		VkImageMemoryBarrier image_memory_barrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
													  .oldLayout = old_image_layout,
													  .newLayout = new_image_layout,
													  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
													  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
													  .image = image,
													  .subresourceRange = subresourceRange };

		switch (old_image_layout) {
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;
		default:
			break;
		}

		switch (new_image_layout) {
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		default:
			break;
		}

		vkCmdPipelineBarrier(cmdBuff, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
		//image->layout = new_image_layout;
	}

	void vkr_image_set_layout(VkCommandBuffer cmdBuff, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkImageLayout new_image_layout, VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages)
	{
		VkImageSubresourceRange subres = { aspectMask,0,1,0,1 };
		vkr_image_set_layout_subres(cmdBuff, image, subres, old_image_layout, new_image_layout, src_stages, dest_stages);
	}


	void dvk_texture::get_buffer(char* outbuf, upload_cx* q)//, dvk_queue* q)
	{
		dvk_staging_buffer staging;
		size_t as = width * height * 4, align = 512;
		as = alignUp(as, align);
		staging.initBuffer(_dev, as ? as : _alloc_size);
		VkImageAspectFlags    aspectMask = (VkImageAspectFlags)(is_depth_tex((VkFormat)_format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);
		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = aspectMask;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = width;
		bufferCopyRegion.imageExtent.height = height;
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = 0;
		bufferCopyRegion.bufferImageHeight = height;
		bufferCopyRegion.bufferRowLength = width;
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = aspectMask;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipLevels;
		subresourceRange.layerCount = 1;
		VkImageLayout il = !descriptor ? (VkImageLayout)GetImageLayout(_image_layout) : descriptor->imageLayout;
		q->add_copy2mem(_image, bufferCopyRegion, subresourceRange, aspectMask, il, staging.buffer);
		q->flushAndFinish();
		staging.getBuffer(outbuf, -1);
	}
	void dvk_texture::copy2image(VkImage img, upload_cx* q)
	{
		size_t as = width * height * 4, align = 512;
		as = alignUp(as, align);
		VkImageAspectFlags    aspectMask = (VkImageAspectFlags)(is_depth_tex((VkFormat)_format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);
		VkImageCopy cr = {};
		cr.dstOffset = {};
		cr.dstSubresource.aspectMask = aspectMask;
		cr.dstSubresource.mipLevel = 0;
		cr.dstSubresource.baseArrayLayer = 0;
		cr.dstSubresource.layerCount = 1;

		cr.extent.width = width;
		cr.extent.height = height;
		cr.extent.depth = 1;

		cr.srcOffset = {};
		cr.srcSubresource = cr.dstSubresource;

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = aspectMask;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipLevels;
		subresourceRange.layerCount = 1;
		VkImageLayout il = !descriptor ? (VkImageLayout)GetImageLayout(_image_layout) : descriptor->imageLayout;
		q->add_copy2img(_image, cr, subresourceRange, aspectMask, il, img);
		q->flushAndFinish();
	}
}


namespace vkr {
	static const int backBufferCount = 3;
	// todo clring

	//--------------------------------------------------------------------------------------
	//
	// OnCreate
	//
	//--------------------------------------------------------------------------------------
	void CommandListRing::OnCreate(Device* pDevice, uint32_t numberOfBackBuffers, uint32_t commandListsPerBackBuffer, bool compute /* = false */)
	{
		m_pDevice = pDevice;
		m_numberOfAllocators = numberOfBackBuffers;
		m_commandListsPerBackBuffer = commandListsPerBackBuffer;

		m_pCommandBuffers.resize(m_numberOfAllocators);

		// Create command allocators, for each frame in flight we wannt to have a single Command Allocator, and <commandListsPerBackBuffer> command buffers
		//
		for (uint32_t a = 0; a < m_numberOfAllocators; a++)
		{
			CommandBuffersPerFrame* pCBPF = &m_pCommandBuffers[a];

			// Create allocator
			//
			VkCommandPoolCreateInfo cmd_pool_info = {};
			cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmd_pool_info.pNext = NULL;
			if (compute == false)
			{
				cmd_pool_info.queueFamilyIndex = pDevice->GetGraphicsQueueFamilyIndex();
			}
			else
			{
				cmd_pool_info.queueFamilyIndex = pDevice->GetComputeQueueFamilyIndex();
			}
			cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			VkResult res = vkCreateCommandPool(pDevice->m_device, &cmd_pool_info, NULL, &pCBPF->m_commandPool);
			assert(res == VK_SUCCESS);

			// Create command buffers
			//
			pCBPF->m_pCommandBuffer = new VkCommandBuffer[m_commandListsPerBackBuffer];
			VkCommandBufferAllocateInfo cmd = {};
			cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmd.pNext = NULL;
			cmd.commandPool = pCBPF->m_commandPool;
			cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmd.commandBufferCount = commandListsPerBackBuffer;
			res = vkAllocateCommandBuffers(pDevice->m_device, &cmd, pCBPF->m_pCommandBuffer);
			assert(res == VK_SUCCESS);

			pCBPF->m_UsedCls = 0;
		}

		m_frameIndex = 0;
		m_pCurrentFrame = &m_pCommandBuffers[m_frameIndex % m_numberOfAllocators];
		m_frameIndex++;
		m_pCurrentFrame->m_UsedCls = 0;
	}


	//--------------------------------------------------------------------------------------
	//
	// OnDestroy
	//
	//--------------------------------------------------------------------------------------
	void CommandListRing::OnDestroy()
	{
		//release and delete command allocators
		for (uint32_t a = 0; a < m_numberOfAllocators; a++)
		{
			vkFreeCommandBuffers(m_pDevice->m_device, m_pCommandBuffers[a].m_commandPool, m_commandListsPerBackBuffer, m_pCommandBuffers[a].m_pCommandBuffer);
			vkDestroyCommandPool(m_pDevice->m_device, m_pCommandBuffers[a].m_commandPool, NULL);
		}
		m_pCommandBuffers.clear();
	}

	//--------------------------------------------------------------------------------------
	//
	// GetNewCommandList
	//
	//--------------------------------------------------------------------------------------
	VkCommandBuffer CommandListRing::GetNewCommandList()
	{
		VkCommandBuffer commandBuffer = m_pCurrentFrame->m_pCommandBuffer[m_pCurrentFrame->m_UsedCls++];
		assert(m_pCurrentFrame->m_UsedCls < m_commandListsPerBackBuffer); //if hit increase commandListsPerBackBuffer
		return commandBuffer;
	}

	//--------------------------------------------------------------------------------------
	//
	// OnBeginFrame
	//
	//--------------------------------------------------------------------------------------
	void CommandListRing::OnBeginFrame()
	{
		m_pCurrentFrame = &m_pCommandBuffers[m_frameIndex % m_numberOfAllocators];
		m_pCurrentFrame->m_UsedCls = 0;
		m_frameIndex++;
	}

#if 1


	//创建信号
	void createSemaphore(VkDevice dev, VkSemaphore* semaphore, VkSemaphoreCreateInfo* semaphoreCreateInfo)
	{
		VkSemaphoreCreateInfo semaphore_create_info = {
			VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,      // VkStructureType          sType
			nullptr,                                      // const void*              pNext
			0                                             // VkSemaphoreCreateFlags   flags
		};
		if (!semaphoreCreateInfo)
			semaphoreCreateInfo = &semaphore_create_info;
		auto hr = vkCreateSemaphore(dev, semaphoreCreateInfo, nullptr, semaphore);
		return;
	}
	void destroyTexture(Device* dev, dvk_texture* texture) {

		auto device = dev->m_device;
		if (texture->_image)
		{
			vkDestroyImage(device, texture->_image, 0); texture->_image = 0;
		}
		if (texture->image_memory)
		{
			vkFreeMemory(device, texture->image_memory, 0); texture->image_memory = 0;
		}
		if (texture->_view)
		{
			vkDestroyImageView(device, texture->_view, 0); texture->_view = 0;
		}
	}
	//创建图像
	int64_t createImage(Device* dev, VkImageCreateInfo* imageinfo, VkImageViewCreateInfo* viewinfo, dvk_texture* texture)//, VkSampler* sampler = nullptr, VkSamplerCreateInfo* info = nullptr)
	{
		VkImageView* imageview;
		VkMemoryAllocateInfo memAlloc = {};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs;
		auto device = dev->m_device;
#if 1
		VkImage* image = &texture->_image;
		VkDeviceMemory mem = texture->image_memory;
		if (texture->_image)
		{
			vkDestroyImage(device, texture->_image, 0); texture->_image = 0;
		}
		auto hr = vkCreateImage(device, imageinfo, nullptr, image);
		vkGetImageMemoryRequirements(device, texture->_image, &memReqs);
		// 设备内存分配空间小于需求的重新申请内存
		if (texture->cap_device_mem_size < memReqs.size || !mem)
		{
			texture->cap_inc = 0;
			if (texture->image_memory)
			{
				vkFreeMemory(device, texture->image_memory, 0);
				texture->image_memory = 0;
			}
			memAlloc.allocationSize = memReqs.size;
			texture->cap_device_mem_size = memReqs.size;
			VkBool32 memTypeFound = 0;
			uint32_t dh = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			if ((imageinfo->usage & VK_IMAGE_USAGE_STORAGE_BIT))
			{
				dh = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			}
			do {
				memAlloc.memoryTypeIndex = getMemoryType(dev, memReqs.memoryTypeBits, dh, &memTypeFound);
				if (!memTypeFound && dh != VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
				{
					dh = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
					continue;
				}
				break;
			} while (1);

			auto hr = vkAllocateMemory(device, &memAlloc, nullptr, &mem);
			assert(hr == VK_SUCCESS);
			texture->image_memory = mem;
		}
		texture->cap_inc++;
		// 绑定显存
		hr = vkBindImageMemory(device, texture->_image, mem, 0);

		viewinfo->image = texture->_image;
		if (texture->_view)
		{
			vkDestroyImageView(device, texture->_view, 0);
		}
		hr = vkCreateImageView(device, viewinfo, nullptr, &texture->_view);
#endif
		return memAlloc.allocationSize;
	}
	VkFence createFence(VkDevice dev, VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT)
	{
		VkFenceCreateInfo fence_create_info = {
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,              // VkStructureType                sType
			nullptr,                                          // const void                    *pNext
			flags                      // VkFenceCreateFlags             flags
		};

		VkFence fence = 0;
		(vkCreateFence(dev, &fence_create_info, nullptr, &fence));
		return fence;
	}





	fbo_info_cx::fbo_info_cx()
	{
	}

	fbo_info_cx::~fbo_info_cx()
	{
		destroy_all();
	}

	void fbo_info_cx::setClearValues(uint32_t color, float depth, uint32_t Stencil)
	{
		//float r[] = { vk_R(color) / 255.0f,  vk_G(color) / 255.0f,  vk_B(color) / 255.0f,  vk_A(color) / 255.0f };
		//memcpy(&clearValues[0].color, r, sizeof(float) * 4);
		unsigned char* uc = (unsigned char*)&color;
		clearValues[0].color = { uc[0] / 255.0f, uc[1] / 255.0f, uc[2] / 255.0f, uc[3] / 255.0f };
		clearValues[1].depthStencil = { depth, Stencil };
	}

	void fbo_info_cx::setClearValues(float* color, float depth, uint32_t Stencil)
	{
		clearValues[0].color = { { color[0], color[1], color[2], color[3] } };
		clearValues[1].depthStencil = { depth, Stencil };
	}

	//swapchainbuffers交换链
	void fbo_info_cx::initFBO(int width, int height, int count, VkRenderPass rp)
	{
		_width = width; _height = height;
		count_ = count;
		//是否创建颜色缓冲纹理
		isColor = true;// !swapchainbuffers || swapchainbuffers->empty();
		// Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering
		resetCommandBuffers();

		if (!renderPass)
		{
			if (!rp)
			{
				//nrp = rp = _dev->new_render_pass(colorFormat, depthFormat);
			}
			renderPass = rp;
		}
		// Create sampler to sample from the color attachments
		VkSamplerCreateInfo samplerinfo = {};
		samplerinfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerinfo.magFilter = VK_FILTER_LINEAR;
		samplerinfo.minFilter = VK_FILTER_LINEAR;
		samplerinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerinfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerinfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerinfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerinfo.mipLodBias = 0.0f;
		samplerinfo.maxAnisotropy = 1;
		samplerinfo.minLod = 0.0f;
		samplerinfo.maxLod = 1.0f;
		samplerinfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		if (!sampler)
		{
			sampler = _dev->newSampler(&samplerinfo);
		}
		//VkFormatProperties phyStencilProps = { 0 }, phyImgProps[2] = {0};
		////check png blit format
		//VkFormat pngBlitFormats[] = { VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_UNORM };
		//for (int i = 0; i < 2; i++)
		//{
		//	vkGetPhysicalDeviceFormatProperties(_dev->GetPhysicalDevice(), pngBlitFormats[i], &phyImgProps[i]);	
		//	if ((phyImgProps[i].linearTilingFeatures & VKVG_PNG_WRITE_IMG_REQUIREMENTS) == VKVG_PNG_WRITE_IMG_REQUIREMENTS) {
		//		//dev->pngStagFormat = pngBlitFormats[i];
		//		//dev->pngStagTiling = VK_IMAGE_TILING_LINEAR;
		//		//break;
		//		printf("");
		//	}
		//	else if ((phyImgProps[i].optimalTilingFeatures & VKVG_PNG_WRITE_IMG_REQUIREMENTS) == VKVG_PNG_WRITE_IMG_REQUIREMENTS) {
		//		//dev->pngStagFormat = pngBlitFormats[i];
		//		//dev->pngStagTiling = VK_IMAGE_TILING_OPTIMAL;
		//		//break;
		//		printf("");
		//	}
		//}
		// Create num frame buffers
		resetFramebuffer(width, height);
	}

	//窗口大小改变时需要重新创建image,如果是交换链则传swapchainbuffers
	void fbo_info_cx::resetFramebuffer(int width, int height)
	{
		_width = width;
		_height = height;
#ifdef _WIN32
		destroyImage();
#endif
		// Color attachment
		VkImageCreateInfo image = {};
		image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = colorFormat;
		image.extent.width = width;
		image.extent.height = height;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		// We will sample directly from the color attachment
		image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageViewCreateInfo colorImageView = {};
		colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorImageView.format = colorFormat;
		colorImageView.flags = 0;
		colorImageView.subresourceRange = {};
		colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorImageView.subresourceRange.baseMipLevel = 0;
		colorImageView.subresourceRange.levelCount = 1;
		colorImageView.subresourceRange.baseArrayLayer = 0;
		colorImageView.subresourceRange.layerCount = 1;
		if (isColor)
		{
			for (auto& it : framebuffers)
			{
				createImage(_dev, &image, &colorImageView, &it.color);
				it.color.width = width;
				it.color.height = height;
				it.color._format = colorFormat;
			}
			if (!_fence)
			{
				_fence = createFence(_dev->m_device);
			}
		}
		else
		{
			//int i = 0;
			//for (auto it : *swapchainbuffers)
			//{
			//	if (framebuffers.size() > i)
			//	{
			//		framebuffers[i].color._dev = _dev;
			//		framebuffers[i].color._image = it.image;
			//		framebuffers[i].color._view = it._view;
			//		framebuffers[i].color.width = width;
			//		framebuffers[i].color.height = height;
			//		framebuffers[i].color._format = (VkFormat)it.colorFormat;
			//		i++;
			//	}
			//}
		}
		// Depth stencil attachment
		image.format = depthFormat;
		image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VkImageViewCreateInfo depthStencilView = {};
		depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = depthFormat;
		depthStencilView.flags = 0;
		depthStencilView.subresourceRange = {};
		depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		depthStencilView.subresourceRange.baseMipLevel = 0;
		depthStencilView.subresourceRange.levelCount = 1;
		depthStencilView.subresourceRange.baseArrayLayer = 0;
		depthStencilView.subresourceRange.layerCount = 1;
		if (is_stencil_tex(depthFormat)) {
			depthStencilView.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		for (auto& it : framebuffers)
		{
			createImage(_dev, &image, &depthStencilView, &it.depth_stencil);
			it.depth_stencil.width = width; it.depth_stencil.height = height; it.depth_stencil._format = depthFormat;
		}

		VkImageView attachments[2];
		int inc = 0;
		for (auto& it : framebuffers)
		{
			inc++;
			attachments[0] = it.color._view;
			attachments[1] = it.depth_stencil._view;
			VkFramebufferCreateInfo fbufCreateInfo = {};
			fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbufCreateInfo.renderPass = renderPass;
			fbufCreateInfo.attachmentCount = 2;
			fbufCreateInfo.pAttachments = attachments;
			fbufCreateInfo.width = width;
			fbufCreateInfo.height = height;
			fbufCreateInfo.layers = 1;
			auto hr = vkCreateFramebuffer(_dev->m_device, &fbufCreateInfo, nullptr, &it.framebuffer);
			// Fill a descriptor for later use in a descriptor set
			it.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			it.descriptor.imageView = it.color._view;
			it.descriptor.sampler = sampler;
		}
	}

	void fbo_info_cx::reset_fbo(int width, int height)
	{
		resetFramebuffer(width, height);
	}

	void fbo_info_cx::resetCommandBuffers()
	{
		if (count_ < 1)
			return;
		destroy_all();
		framebuffers.resize(count_);
		drawCmdBuffers.resize(count_);
		for (auto& it : framebuffers)
		{
			it.depth_stencil._dev = _dev;
			if (it.semaphore == VK_NULL_HANDLE)
			{
				createSemaphore(_dev->m_device, &it.semaphore, 0);
			}
		}
		if (isColor)
		{
			for (auto& it : framebuffers)
				it.color._dev = _dev;
		}
	}

	void fbo_info_cx::build_cmd_empty()
	{
		VkCommandBufferBeginInfo cmdBufInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 0, 0, 0 };
		VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, 0, 0, 0, 0, 0, 0 };
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = _width;
		renderPassBeginInfo.renderArea.extent.height = _height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		VkResult hr = {};
		for (size_t i = 0; i < framebuffers.size(); ++i)
		{
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = framebuffers[i].framebuffer;
			hr = vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo);
			// Draw the particle system using the update vertex buffer
			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdEndRenderPass(drawCmdBuffers[i]);
			hr = vkEndCommandBuffer(drawCmdBuffers[i]);
		}
	}

	void fbo_info_cx::destroyImage()
	{
		for (auto& it : framebuffers)
		{
			if (isColor)
			{
				destroyTexture(_dev, &it.color);
				destroyTexture(_dev, &it.depth_stencil);
			}
			if (it.framebuffer)
				vkDestroyFramebuffer(_dev->m_device, it.framebuffer, 0);
			it.framebuffer = 0;
		}
	}

	void fbo_info_cx::destroy_all()
	{
		destroyImage();
		for (auto& it : framebuffers)
		{
			if (it.semaphore)
				vkDestroySemaphore(_dev->m_device, it.semaphore, 0);
			it.semaphore = 0;
			if (it.framebuffer)
				vkDestroyFramebuffer(_dev->m_device, it.framebuffer, 0);
			it.framebuffer = 0;
		}
		if (_fence)
			vkDestroyFence(_dev->m_device, _fence, 0);
		_fence = 0;
		renderPass = 0;
	}






	// todo vkrenderer
	Renderer_cx::Renderer_cx(const_vk* p)
	{
		//m_AsyncPool = new AsyncPool();
		if (p)
			ct = *p;
	}


	Renderer_cx::~Renderer_cx()
	{
		//if (m_AsyncPool) {
		//	delete m_AsyncPool;
		//	m_AsyncPool = nullptr;
		//}
		if (m_GBuffer)
			delete m_GBuffer;
		m_GBuffer = 0;
	}

	//--------------------------------------------------------------------------------------
	//
	// OnCreate
	//
	//--------------------------------------------------------------------------------------
	void Renderer_cx::OnCreate(Device* pDevice, VkRenderPass rp)
	{
		m_pDevice = pDevice;

		// Initialize helpers

		// Create all the heaps for the resources views
		/*const uint32_t cbvDescriptorCount = 2000;
		const uint32_t srvDescriptorCount = 8000;
		const uint32_t uavDescriptorCount = 10;
		const uint32_t samplerDescriptorCount = 20;*/
		m_ResourceViewHeaps.OnCreate(pDevice, ct.cbvDescriptorCount, ct.srvDescriptorCount, ct.uavDescriptorCount, ct.samplerDescriptorCount);

		// Create a commandlist ring for the Direct queue
		//uint32_t commandListsPerBackBuffer = 8;
		m_CommandListRing.OnCreate(pDevice, backBufferCount, ct.commandListsPerBackBuffer);

		// Create a 'dynamic' constant buffer
		const uint32_t constantBuffersMemSize = 64 * 1024;
		m_ConstantBufferRing.OnCreate(pDevice, backBufferCount, ct.constantBuffersMemSize, (char*)"Uniforms");

		// Create a 'static' pool for vertices and indices 
		const uint32_t staticGeometryMemSize = 128 * 1024 * 1024;
		m_VidMemBufferPool.OnCreate(pDevice, staticGeometryMemSize, true, "StaticGeom");

		// Create a 'static' pool for vertices and indices in system memory
		const uint32_t systemGeometryMemSize = 32 * 1024;
		m_SysMemBufferPool.OnCreate(pDevice, systemGeometryMemSize, true, "PostProcGeom");

		// initialize the GPU time stamps module
		m_GPUTimer.OnCreate(pDevice, backBufferCount);

		// Quick helper to upload resources, it has it's own commandList and uses suballocation.
		//const uint32_t uploadHeapMemSize = 1000 * 1024 * 1024;
		m_UploadHeap.OnCreate(pDevice, ct.uploadHeapMemSize);    // initialize an upload heap (uses suballocation for faster results)
		// Create GBuffer and render passes
		assert(!m_GBuffer);
		m_GBuffer = new GBuffer();
		// 向前颜色格式
		VkFormat mformat = VK_FORMAT_R16G16B16A16_SFLOAT;
		// 深度缓冲格式
		VkFormat dformat = _stencil_test ? VK_FORMAT_D32_SFLOAT_S8_UINT : VK_FORMAT_D32_SFLOAT;
		{
			std::map<GBufferFlags, VkFormat> formats =
			{
				{ GBUFFER_DEPTH, dformat},
				{ GBUFFER_FORWARD, mformat},
				{ GBUFFER_MOTION_VECTORS, VK_FORMAT_R16G16_SFLOAT}, //运动矢量
				//{ GBUFFER_NORMAL_BUFFER, GBFORMAT_E},
			};
			//if (has_oit) {
			//	formats[GBUFFER_OIT_ACCUM] = mformat;
			//	formats[GBUFFER_OIT_WEIGHT] = VK_FORMAT_R16_SFLOAT;
			//}
			m_GBuffer->OnCreate(pDevice, &m_ResourceViewHeaps, formats, 1);
			GBufferFlags fullGBuffer = GBUFFER_DEPTH | GBUFFER_FORWARD | GBUFFER_MOTION_VECTORS;// | GBUFFER_NORMAL_BUFFER;
			//if (has_oit) fullGBuffer |= GBUFFER_OIT_ACCUM | GBUFFER_OIT_WEIGHT;
			bool bClear = true;
			// 用于渲染不透明物体及清空缓冲区opaque
			m_RenderPassFullGBufferWithClear.OnCreate(m_GBuffer, fullGBuffer, bClear, "m_RenderPassFullGBufferWithClear");
			// 用于渲染透明物体transparent、透射材质transmission
			m_RenderPassFullGBuffer.OnCreate(m_GBuffer, fullGBuffer, !bClear, "m_RenderPassFullGBuffer");
			// 用于渲染天空盒、线框justdepth
			m_RenderPassJustDepthAndHdr.OnCreate(m_GBuffer, GBUFFER_DEPTH | GBUFFER_FORWARD, !bClear, "m_RenderPassJustDepthAndHdr");
		}

		// Create render pass shadow, will clear contents
		{
			VkAttachmentDescription depthAttachments;
			AttachClearBeforeUse(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &depthAttachments);
			m_Render_pass_shadow = CreateRenderPassOptimal(m_pDevice->m_device, 0, NULL, &depthAttachments);
		}

		{
			skyDomeConstants.vSunDirection = glm::vec4(1.0f, 0.05f, 0.0f, glm::radians(2.0f));
			skyDomeConstants.turbidity = 10.0f;
			skyDomeConstants.rayleigh = 2.0f;
			skyDomeConstants.mieCoefficient = 0.005f;
			skyDomeConstants.mieDirectionalG = 0.8f;
			skyDomeConstants.luminance = 1.0f;
		}
		auto specular = "images\\specular.dds"; specular = 0;
		m_SkyDome.OnCreate(pDevice, m_RenderPassJustDepthAndHdr.GetRenderPass(), &m_UploadHeap, mformat, &m_ResourceViewHeaps
			, &m_ConstantBufferRing, &m_SysMemBufferPool, "images\\diffuse.dds", specular, VK_SAMPLE_COUNT_1_BIT);
		m_SkyDomeProc.OnCreate(pDevice, m_RenderPassJustDepthAndHdr.GetRenderPass(), &m_UploadHeap, mformat, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_SysMemBufferPool, VK_SAMPLE_COUNT_1_BIT);
		m_Wireframe.OnCreate(pDevice, m_RenderPassJustDepthAndHdr.GetRenderPass(), &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_SysMemBufferPool, VK_SAMPLE_COUNT_1_BIT);
		m_WireframeBox.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_SysMemBufferPool);
		_WireframeSphere.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_SysMemBufferPool);
		//_cbf.OnCreate(pDevice, m_RenderPassJustDepthAndHdr.GetRenderPass(), &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_SysMemBufferPool, VK_SAMPLE_COUNT_1_BIT);

		//_axis.OnCreate(pDevice, m_RenderPassJustDepthAndHdr.GetRenderPass(), &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_SysMemBufferPool, VK_SAMPLE_COUNT_1_BIT);


		m_DownSample.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_SysMemBufferPool, mformat);
		m_Bloom.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_SysMemBufferPool, mformat);
		m_TAA.OnCreate(pDevice, &m_ResourceViewHeaps, &m_SysMemBufferPool, &m_ConstantBufferRing, false);
		m_MagnifierPS.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_SysMemBufferPool, mformat);

		// Create tonemapping pass
		m_ToneMappingCS.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing);
		m_oitblendCS.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing);
		m_ToneMappingPS.OnCreate(m_pDevice, rp, &m_ResourceViewHeaps, &m_SysMemBufferPool, &m_ConstantBufferRing);
		m_ColorConversionPS.OnCreate(pDevice, rp, &m_ResourceViewHeaps, &m_SysMemBufferPool, &m_ConstantBufferRing);

		// Initialize UI rendering resources
		//m_ImGUI.OnCreate(m_pDevice, pSwapChain->GetRenderPass(), &m_UploadHeap, &m_ConstantBufferRing, FontSize);

		init_envres();

		// Make sure upload heap has finished uploading before continuing
		m_SysMemBufferPool.UploadData(m_UploadHeap.GetCommandList());
		m_UploadHeap.FlushAndFinish();
		m_SysMemBufferPool.FreeUploadHeap();


	}

	//--------------------------------------------------------------------------------------
	//
	// OnDestroy 
	//
	//--------------------------------------------------------------------------------------
	void Renderer_cx::OnDestroy()
	{
		//m_AsyncPool->Flush();
		un_envres();
		//m_ImGUI.OnDestroy();
		m_ColorConversionPS.OnDestroy();
		m_ToneMappingPS.OnDestroy();
		m_ToneMappingCS.OnDestroy();
		m_oitblendCS.OnDestroy();
		m_TAA.OnDestroy();
		m_Bloom.OnDestroy();
		m_DownSample.OnDestroy();
		m_MagnifierPS.OnDestroy();
		m_WireframeBox.OnDestroy();
		m_Wireframe.OnDestroy();
		_WireframeSphere.OnDestroy();
		//_cbf.OnDestroy();
		//_axis.OnDestroy();
		m_SkyDomeProc.OnDestroy();
		m_SkyDome.OnDestroy();

		m_RenderPassFullGBufferWithClear.OnDestroy();
		m_RenderPassJustDepthAndHdr.OnDestroy();
		m_RenderPassFullGBuffer.OnDestroy();
		m_GBuffer->OnDestroy();

		vkDestroyRenderPass(m_pDevice->m_device, m_Render_pass_shadow, nullptr);
		m_UploadHeap.OnDestroy();
		m_GPUTimer.OnDestroy();
		m_VidMemBufferPool.OnDestroy();
		m_SysMemBufferPool.OnDestroy();
		m_ConstantBufferRing.OnDestroy();
		m_ResourceViewHeaps.OnDestroy();
		m_CommandListRing.OnDestroy();
	}

	//--------------------------------------------------------------------------------------
	//
	// OnCreateWindowSizeDependentResources
	//
	//--------------------------------------------------------------------------------------
	void Renderer_cx::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height)
	{
		m_Width = Width;
		m_Height = Height;

		// Set the viewport
		//
		m_Viewport.x = 0;
		m_Viewport.y = (float)Height;
		m_Viewport.width = (float)Width;
		m_Viewport.height = -(float)(Height);
		m_Viewport.minDepth = (float)0.0f;
		m_Viewport.maxDepth = (float)1.0f;

		// Create scissor rectangle
		//
		m_RectScissor.extent.width = Width;
		m_RectScissor.extent.height = Height;
		m_RectScissor.offset.x = 0;
		m_RectScissor.offset.y = 0;

		// Create GBuffer
		//
		m_GBuffer->OnCreateWindowSizeDependentResources(Width, Height);

		// Create frame buffers for the GBuffer render passes
		//
		m_RenderPassFullGBufferWithClear.OnCreateWindowSizeDependentResources(Width, Height);
		m_RenderPassJustDepthAndHdr.OnCreateWindowSizeDependentResources(Width, Height);
		m_RenderPassFullGBuffer.OnCreateWindowSizeDependentResources(Width, Height);

		// Update PostProcessing passes
		//
		m_DownSample.OnCreateWindowSizeDependentResources(Width, Height, &m_GBuffer->m_HDR, 6); //downsample the HDR texture 6 times
		// todo bloom
		m_Bloom.OnCreateWindowSizeDependentResources(Width / 2, Height / 2, m_DownSample.GetTexture(), 6, &m_GBuffer->m_HDR);
		m_TAA.OnCreateWindowSizeDependentResources(Width, Height, m_GBuffer);
		m_MagnifierPS.OnCreateWindowSizeDependentResources(&m_GBuffer->m_HDR);



		m_bMagResourceReInit = true;
	}

	//--------------------------------------------------------------------------------------
	//
	// OnDestroyWindowSizeDependentResources
	//
	//--------------------------------------------------------------------------------------
	void Renderer_cx::OnDestroyWindowSizeDependentResources()
	{
		m_Bloom.OnDestroyWindowSizeDependentResources();
		m_DownSample.OnDestroyWindowSizeDependentResources();
		m_TAA.OnDestroyWindowSizeDependentResources();
		m_MagnifierPS.OnDestroyWindowSizeDependentResources();

		m_RenderPassFullGBufferWithClear.OnDestroyWindowSizeDependentResources();
		m_RenderPassJustDepthAndHdr.OnDestroyWindowSizeDependentResources();
		m_RenderPassFullGBuffer.OnDestroyWindowSizeDependentResources();
		m_GBuffer->OnDestroyWindowSizeDependentResources();
	}

	void Renderer_cx::OnUpdateDisplayDependentResources(VkRenderPass rp, DisplayMode dm, bool bUseMagnifier)
	{
		// Update the pipelines if the swapchain render pass has changed (for example when the format of the swapchain changes)
		//
		m_ColorConversionPS.UpdatePipelines(rp, dm);
		m_ToneMappingPS.UpdatePipelines(rp);

		//m_ImGUI.UpdatePipeline((pSwapChain->GetDisplayMode() == DISPLAYMODE_SDR) ? pSwapChain->GetRenderPass() : bUseMagnifier ? m_MagnifierPS.GetPassRenderPass() : m_RenderPassJustDepthAndHdr.GetRenderPass());
	}

	//--------------------------------------------------------------------------------------
	//
	// OnUpdateLocalDimmingChangedResources
	//
	//--------------------------------------------------------------------------------------
	void Renderer_cx::OnUpdateLocalDimmingChangedResources(VkRenderPass rp, DisplayMode dm)
	{
		m_ColorConversionPS.UpdatePipelines(rp, dm);
	}

	//--------------------------------------------------------------------------------------
	//
	// load_model
	//
	//--------------------------------------------------------------------------------------
	int Renderer_cx::load_model(GLTFCommon* pGLTFCommon, int Stage)
	{
		// show loading progress
		//
		//ImGui::OpenPopup("Loading");
		//if (ImGui::BeginPopupModal("Loading", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		//{
		//	float progress = (float)Stage / 12.0f;
		//	ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), NULL);
		//	ImGui::EndPopup();
		//}

		// use multi threading
		//AsyncPool* pAsyncPool = m_AsyncPool;

		// Loading stages
		//
		if (Stage == 0)
		{
			if (!currobj)
				currobj = new robj_info();
		}
		else if (Stage == 5)
		{
			Profile p("m_pGltfLoader->Load");

			currobj->_ptb = new gltf_gpu_res_cx();
			currobj->_ptb->OnCreate(m_pDevice, pGLTFCommon, &m_UploadHeap);
			//currobj->_ptb->OnCreate(m_pDevice, pGLTFCommon, &, &m_VidMemBufferPool, );
		}
		else if (Stage == 6)
		{
			Profile p("Load(Textures Geometry)");
			// 纹理、数据传输到GPU       
			currobj->_ptb->LoadTextures(true);
			currobj->_ptb->LoadGeometry();
		}
		else if (Stage == 7)
		{
			Profile p("m_GLTFDepth->OnCreate");
			//create the glTF's textures, VBs, IBs, shaders and descriptors for this particular pass    
			currobj->m_GLTFDepth = new GltfDepthPass();
			currobj->m_GLTFDepth->has_shadowMap = pGLTFCommon->has_shadowMap;
			currobj->m_GLTFDepth->OnCreate(m_pDevice, m_Render_pass_shadow, &m_UploadHeap, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, currobj->_ptb);
			m_VidMemBufferPool.UploadData(m_UploadHeap.GetCommandList());
			m_UploadHeap.FlushAndFinish();
		}
		else if (Stage == 8)
		{
			Profile p("m_GLTFPBR->OnCreate");

			// same thing as above but for the PBR pass
			currobj->m_GLTFPBR = new GltfPbrPass();
			currobj->m_GLTFPBR->OnCreate(m_pDevice, &m_UploadHeap, &m_ResourceViewHeaps, &m_ConstantBufferRing
				, currobj->_ptb, &_envr, &m_RenderPassFullGBufferWithClear);

			m_VidMemBufferPool.UploadData(m_UploadHeap.GetCommandList());
			m_UploadHeap.FlushAndFinish();
		}
		else if (Stage == 9)
		{
			Profile p("m_GLTFBBox->OnCreate");

			// just a bounding box pass that will draw boundingboxes instead of the geometry itself
			currobj->m_GLTFBBox = new GltfBBoxPass();
			currobj->m_GLTFBBox->OnCreate(m_pDevice, m_RenderPassJustDepthAndHdr.GetRenderPass(), &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool
				, currobj->_ptb, &m_Wireframe);

			// we are borrowing the upload heap command list for uploading to the GPU the IBs and VBs
			m_VidMemBufferPool.UploadData(m_UploadHeap.GetCommandList());

		}
		else if (Stage == 10)
		{
			Profile p("Flush");
			m_UploadHeap.FlushAndFinish();
			//once everything is uploaded we dont need the upload heaps anymore
			//m_VidMemBufferPool.FreeUploadHeap(); 
			_robject.push_back(currobj);
			_depthpass.push_back(currobj->m_GLTFDepth);
			currobj = 0;
			// tell caller that we are done loading the map
			return 0;
		}

		Stage++;
		return Stage;
	}

	//--------------------------------------------------------------------------------------
	//
	// UnloadScene
	//
	//--------------------------------------------------------------------------------------
	void Renderer_cx::unloadgltf(robj_info* p)
	{
		if (!p)return;

		if (p->m_GLTFPBR)
		{
			p->m_GLTFPBR->OnDestroy();
			delete p->m_GLTFPBR;
			p->m_GLTFPBR = NULL;
		}

		if (p->m_GLTFDepth)
		{
			p->m_GLTFDepth->OnDestroy();
			delete p->m_GLTFDepth;
			p->m_GLTFDepth = NULL;
		}

		if (p->m_GLTFBBox)
		{
			p->m_GLTFBBox->OnDestroy();
			delete p->m_GLTFBBox;
			p->m_GLTFBBox = NULL;
		}

		if (p->_ptb)
		{
			p->_ptb->OnDestroy();
			delete p->_ptb;
			p->_ptb = NULL;
		}

		delete p;
	}
	void Renderer_cx::UnloadScene()
	{
		// wait for all the async loading operations to finish
		//m_AsyncPool->Flush();

		m_pDevice->GPUFlush();
		for (auto it : _robject)
			unloadgltf(it);
		_robject.clear();

		// todo 阴影
		assert(m_shadowMapPool.size() == m_ShadowSRVPool.size());
		while (!m_shadowMapPool.empty())
		{
			m_shadowMapPool.back().ShadowMap.OnDestroy();
			vkDestroyFramebuffer(m_pDevice->m_device, m_shadowMapPool.back().ShadowFrameBuffer, nullptr);
			vkDestroyImageView(m_pDevice->m_device, m_ShadowSRVPool.back(), nullptr);
			vkDestroyImageView(m_pDevice->m_device, m_shadowMapPool.back().ShadowDSV, nullptr);
			m_ShadowSRVPool.pop_back();
			m_shadowMapPool.pop_back();
		}
	}
	size_t Renderer_cx::AddLight(const light_t& light)
	{
		_lights_q.push(light);
		return _lights_q.size();
	}
	light_t* Renderer_cx::get_light(size_t idx)
	{
		if (idx < _lights.size())
			return &_lights[idx];
		return nullptr;
	}
	size_t Renderer_cx::get_light_size()
	{
		return _lights.size();
	}
	void Renderer_cx::new_shadow(SceneShadowInfo& ShadowInfo, uint32_t shadowResolution, int shadows, int idx)
	{
		ShadowInfo.ShadowResolution = shadowResolution;
		ShadowInfo.ShadowIndex = shadows;
		ShadowInfo.LightIndex = idx;

		{
			auto CurrentShadow = &ShadowInfo;
			// 初始化阴影深度图
			CurrentShadow->ShadowMap.InitDepthStencil(m_pDevice, CurrentShadow->ShadowResolution, CurrentShadow->ShadowResolution, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, "ShadowMap");
			CurrentShadow->ShadowMap.CreateDSV(&CurrentShadow->ShadowDSV);
			// Create render pass shadow, will clear contents
			{
				VkAttachmentDescription depthAttachments;
				AttachClearBeforeUse(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &depthAttachments);

				// Create frame buffer
				VkImageView attachmentViews[1] = { CurrentShadow->ShadowDSV };
				VkFramebufferCreateInfo fb_info = {};
				fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				fb_info.pNext = NULL;
				fb_info.renderPass = m_Render_pass_shadow;
				fb_info.attachmentCount = 1;
				fb_info.pAttachments = attachmentViews;
				fb_info.width = CurrentShadow->ShadowResolution;
				fb_info.height = CurrentShadow->ShadowResolution;
				fb_info.layers = 1;
				VkResult res = vkCreateFramebuffer(m_pDevice->m_device, &fb_info, NULL, &CurrentShadow->ShadowFrameBuffer);
				assert(res == VK_SUCCESS);
			}
			VkImageView ShadowSRV;
			CurrentShadow->ShadowMap.CreateSRV(&CurrentShadow->ShadowSRV);
		}
	}
	void Renderer_cx::un_envres() {
		for (size_t i = 0; i < 3; i++)
		{
			auto& it = _lut[i];
			vkDestroyImageView(m_pDevice->m_device, it.view, NULL);
			it.texture.OnDestroy();
		}
	}
	void Renderer_cx::init_envres()
	{
		// lut_ggx\lut_charlie\lut_sheen_E
		const char* lut_files[3] = { "images/lut_ggx.png", "images/lut_charlie.png","images/lut_sheen_E.png" };
		{
			print_time Pt("load LUT", 1);
			// specular BRDF lut sampler 
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.minLod = -1000;
			info.maxLod = 1000;
			info.maxAnisotropy = 1.0f;
			auto luts = m_pDevice->newSampler(&info);
			assert(luts);
			for (size_t i = 0; i < 3; i++)
			{
				auto& it = _lut[i];
				it.texture.InitFromFile(m_pDevice, &m_UploadHeap, lut_files[i], false);
				it.texture.CreateSRV(&it.view);
				it.sampler = luts;
			}
		}

		// shadowmap sampler
		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.compareEnable = VK_TRUE;
			info.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			info.minLod = -1000;
			info.maxLod = 1000;
			info.maxAnisotropy = 1.0f;
			_envr.m_samplerShadow = m_pDevice->newSampler(&info);
			assert(_envr.m_samplerShadow);
		}
		_envr.use_punctual = true;
		_envr.pSkyDome = &m_SkyDome; _envr.bUseSSAOMask = false;
		_envr.ShadowMapViewPool = &m_ShadowSRVPool;
		_envr.lut = _lut;

	}
#if 1
	void Renderer_cx::AllocateShadowMaps()
	{
		auto NumShadows = m_shadowMapPool.size();
		if (_lights_q.size())
		{
			auto i = _lights.size();
			for (; _lights_q.size();) {
				auto q = _lights_q.front(); _lights_q.pop();
				if (q._shadowResolution && q._type != light_t::LIGHT_POINTLIGHT)
				{
					SceneShadowInfo ShadowInfo;
					new_shadow(ShadowInfo, q._shadowResolution, NumShadows++, i); // 创建场景阴影信息
					m_ShadowSRVPool.push_back(ShadowInfo.ShadowSRV);
					m_shadowMapPool.push_back(ShadowInfo);
				}
				_lights.push_back(q); i++;
			}
		}
		if (NumShadows > MaxShadowInstances)
		{
			Trace("Number of shadows has exceeded maximum supported. Please grow value in gltfCommon.h/perFrameStruct.h");
			throw;
		}

	}
#endif

	PerFrame_t* Renderer_cx::mkPerFrameData(const Camera& cam)
	{
		//Sets the camera
		_perFrameData.mCameraCurrViewProj = cam.GetProjection() * cam.GetView();
		_perFrameData.mCameraPrevViewProj = cam.GetProjection() * cam.GetPrevView();
		// more accurate calculation
		_perFrameData.mInverseCameraCurrViewProj = cam.GetView();// glm::affineInverse(cam.GetView())* glm::inverse(cam.GetProjection());
		_perFrameData.cameraPos = cam.GetPosition();

		_perFrameData.wireframeOptions = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

		// Process lights
		_perFrameData.lightCount = (int32_t)_lights.size();
		int32_t ShadowMapIndex = 0;
#if 1
		for (int i = 0; i < _perFrameData.lightCount; i++)
		{
			Light* pSL = &_perFrameData.lights[i];

			// get light data and node trans
			auto& lightData = _lights[i];
			auto q = glm::normalize(lightData._rotation);
			glm::mat4 lightMat = glm::translate(glm::mat4(1.0), lightData._position) * glm::mat4(q);

			glm::mat4 lightView = glm::affineInverse(lightMat);
			auto a = glm::radians(glm::clamp(lightData._cone_angle, 1.0f, 180.0f) * 0.5f);
			glm::mat4 per = glm::perspective(a * 2.0f, 1.0f, .1f, 100.0f);
			pSL->mLightView = lightView;
			if (lightData._type == LightType_Spot)
				pSL->mLightViewProj = per * lightView;
			else if (lightData._type == LightType_Directional)
			{
				AxisAlignedBoundingBox projectedBoundingBox = {};
				for (auto it : _robject)
				{
					if (it->_ptb)
					{
						projectedBoundingBox.Merge(it->_ptb->m_pGLTFCommon->get_BoundingBox(lightView));
					}
				}
				auto dlm = ComputeDirectionalLight_mat(projectedBoundingBox);
				pSL->mLightViewProj = dlm * lightView;
			}
			//transpose
			GetXYZ(pSL->direction, glm::transpose(lightView) * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
			GetXYZ(pSL->position, lightMat[3]);
			GetXYZ(pSL->color, lightData._color);
			pSL->range = lightData._range;
			pSL->intensity = lightData._intensity;

			pSL->outerConeCos = cosf(a);
			pSL->innerConeCos = cosf(a * (1.0 - glm::clamp(lightData._cone_mix, 0.0f, 1.0f)));
			pSL->type = lightData._type;

			// Setup shadow information for light (if it has any)
			if (lightData._shadowResolution && lightData._type != LightType_Point)
			{
				pSL->shadowMapIndex = ShadowMapIndex++;
				pSL->depthBias = lightData._bias;
			}
			else
				pSL->shadowMapIndex = -1;
		}
#endif
		return &_perFrameData;
	}
	void insertImageMemoryBarrier(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		VkImageSubresourceRange subresourceRange)
	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.srcAccessMask = srcAccessMask;
		imageMemoryBarrier.dstAccessMask = dstAccessMask;
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		vkCmdPipelineBarrier(
			cmdbuffer,
			srcStageMask,
			dstStageMask,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);
	}
	struct dynamicrendering_t
	{
		uint32_t width, height;	// 可选
		VkImageView sc_view;	// 	
		VkImageView ds_view;
		VkImage sc_image;
		VkImage depthStencil_image;
		VkCommandBuffer _cmd1;
		PFN_vkCmdBeginRenderingKHR _vkCmdBeginRenderingKHR = VK_NULL_HANDLE;
		PFN_vkCmdEndRenderingKHR _vkCmdEndRenderingKHR = VK_NULL_HANDLE;
	public:
		void dr_begin(VkCommandBuffer cmd1);
		void dr_end();
	};
	void dynamicrendering_t::dr_begin(VkCommandBuffer cmd1)
	{
		if (!_vkCmdBeginRenderingKHR)return;
		_cmd1 = cmd1;
		// With dynamic rendering there are no subpass dependencies, so we need to take care of proper layout transitions by using barriers
		// This set of barriers prepares the color and depth images for output
		insertImageMemoryBarrier(
			cmd1,
			sc_image,
			0,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		insertImageMemoryBarrier(
			cmd1,
			depthStencil_image,
			0,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 });

		// New structures are used to define the attachments used in dynamic rendering
		VkRenderingAttachmentInfoKHR colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = sc_view;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue.color = { 0.0f,0.0f,0.0f,0.0f };

		// A single depth stencil attachment info can be used, but they can also be specified separately.
		// When both are specified separately, the only requirement is that the image view is identical.			
		VkRenderingAttachmentInfoKHR depthStencilAttachment{};
		depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		depthStencilAttachment.imageView = ds_view;// depthStencil.view;
		depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthStencilAttachment.clearValue.depthStencil = { 1.0f,  0 };

		VkRenderingInfoKHR renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, width,height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = &depthStencilAttachment;
		renderingInfo.pStencilAttachment = &depthStencilAttachment;

		// Begin dynamic rendering
		_vkCmdBeginRenderingKHR(cmd1, &renderingInfo);

		if (width > 0 && height > 0) {
			VkViewport viewport = {}; viewport.width = (float)width; viewport.height = (float)height; viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;
			vkCmdSetViewport(cmd1, 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			//initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(cmd1, 0, 1, &scissor);
		}

		//vkCmdBindDescriptorSets(cmd1, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		//vkCmdBindPipeline(cmd1, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		//model.draw(cmd1, vkglTF::RenderFlags::BindImages, pipelineLayout);

		//drawUI(cmd1);

	}
	void dynamicrendering_t::dr_end() {
		if (!_vkCmdEndRenderingKHR)return;
		// End dynamic rendering
		_vkCmdEndRenderingKHR(_cmd1);

		// This set of barriers prepares the color image for presentation, we don't need to care for the depth image
		insertImageMemoryBarrier(
			_cmd1,
			sc_image,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			0,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
	}

#if 1

	void cp_barrier(VkCommandBuffer cmdBuf1, GBuffer* gbp)
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = NULL;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkImageMemoryBarrier barriers[3];
		barriers[0] = barrier;
		barriers[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barriers[0].oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (is_stencil_tex(gbp->m_DepthBuffer.GetFormat()))
		{
			barriers[0].subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		barriers[0].image = gbp->m_DepthBuffer.Resource();

		barriers[1] = barrier;
		barriers[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barriers[1].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barriers[1].image = gbp->m_MotionVectors.Resource();

		// no layout transition but we still need to wait
		barriers[2] = barrier;
		barriers[2].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barriers[2].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barriers[2].image = gbp->m_HDR.Resource();

		vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
			, 0, 0, NULL, 0, NULL, 3, barriers);
	}
#endif // 1

	//--------------------------------------------------------------------------------------
	//
	// todo OnRender
	//
	//--------------------------------------------------------------------------------------
	void Renderer_cx::draw_skydome(VkCommandBuffer cmdBuf1, const scene_state* pState, const glm::mat4& mCameraCurrViewProj)
	{
		VkRect2D renderArea = { 0, 0, m_Width, m_Height };
		// Render skydome天空盒
		{
			m_RenderPassJustDepthAndHdr.BeginPass(cmdBuf1, renderArea);
			glm::mat4 clipToView = glm::inverse(mCameraCurrViewProj);
			if (pState->SelectedSkydomeTypeIndex == 1)
			{
				m_SkyDome.Draw(cmdBuf1, clipToView);
				m_GPUTimer.GetTimeStamp(cmdBuf1, "Skydome cube");
			}
			else if (pState->SelectedSkydomeTypeIndex == 0)
			{
				skyDomeConstants.invViewProj = clipToView;
				m_SkyDomeProc.Draw(cmdBuf1, skyDomeConstants);
				m_GPUTimer.GetTimeStamp(cmdBuf1, "Skydome Proc");
			}
			m_RenderPassJustDepthAndHdr.EndPass(cmdBuf1);
		}
	}

	void draw_pbrpass(vkr::Device* dev, VkCommandBuffer cmdBuf1, const scene_state* pState, std::vector<BatchList>* ds, std::vector<BatchList>* dsw, bool bWireframe)
	{
		if (pState->WireframeMode & (int)scene_state::WireframeMode::WIREFRAME_MODE_SOLID_COLOR)
		{
			if (pState->WireframeMode & (int)scene_state::WireframeMode::WIREFRAME_MODE_FACE)
				GltfPbrPass::DrawBatchList(dev, cmdBuf1, ds, false);
			GltfPbrPass::DrawBatchList(dev, cmdBuf1, dsw, bWireframe);
		}
		else
		{
			GltfPbrPass::DrawBatchList(dev, cmdBuf1, ds, bWireframe);
		}
	}
	void Renderer_cx::draw_pbr_opaque(VkCommandBuffer cmdBuf1, const scene_state* pState, bool bWireframe)
	{
		VkRect2D renderArea = { 0, 0, m_Width, m_Height };
		// Render opaque 渲染不透明物体
		{
			m_RenderPassFullGBufferWithClear.BeginPass(cmdBuf1, renderArea);
			draw_pbrpass(m_pDevice, cmdBuf1, pState, &drawables.opaque, &drawables.wireframe[0], bWireframe);
			m_GPUTimer.GetTimeStamp(cmdBuf1, "PBR Opaque");
			m_RenderPassFullGBufferWithClear.EndPass(cmdBuf1);
		}
	}
	void Renderer_cx::draw_pbr_transparent(VkCommandBuffer cmdBuf1, const scene_state* pState, bool bWireframe)
	{
		VkRect2D renderArea = { 0, 0, m_Width, m_Height };
		// draw transparent geometry 渲染透明物体
		if (!drawables.transparent.empty()) {
			m_RenderPassFullGBuffer.BeginPass(cmdBuf1, renderArea);
			std::stable_sort(drawables.transparent.begin(), drawables.transparent.end(), bcmp);
			draw_pbrpass(m_pDevice, cmdBuf1, pState, &drawables.transparent, &drawables.wireframe[1], bWireframe);
			m_GPUTimer.GetTimeStamp(cmdBuf1, "PBR Transparent");
			m_RenderPassFullGBuffer.EndPass(cmdBuf1);
		}
	}
	void Renderer_cx::copy_transmission(VkCommandBuffer cmdBuf1)
	{
		if (!drawables.transmission.empty()) {
			// 复制 HDR 图像到 HDRt
			//_transfer.copy_image(&m_GBuffer->m_HDR, &m_GBuffer->m_HDRt, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			//_transfer.flush(cmdBuf1);
			auto image = &m_GBuffer->m_HDR;
			VkImageCopy cr = {};
			cr.dstOffset = {};
			cr.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			cr.dstSubresource.mipLevel = 0;
			cr.dstSubresource.baseArrayLayer = 0;
			cr.dstSubresource.layerCount = image->GetArraySize();
			cr.extent.width = image->GetWidth();
			cr.extent.height = image->GetHeight();
			cr.extent.depth = 1;
			cr.srcOffset = {};
			cr.srcSubresource = cr.dstSubresource;
			vkr_image_set_layout(cmdBuf1, m_GBuffer->m_HDR.Resource(), VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			vkr_image_set_layout(cmdBuf1, m_GBuffer->m_HDRt.Resource(), VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			// copy HDR to HDRt
			vkCmdCopyImage(cmdBuf1, m_GBuffer->m_HDR.Resource(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_GBuffer->m_HDRt.Resource(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cr);

			vkr_image_set_layout(cmdBuf1, m_GBuffer->m_HDR.Resource(), VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			vkr_image_set_layout(cmdBuf1, m_GBuffer->m_HDRt.Resource(), VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}
	}
	void Renderer_cx::draw_pbr_transmission(VkCommandBuffer cmdBuf1, const scene_state* pState, bool bWireframe)
	{
		VkRect2D renderArea = { 0, 0, m_Width, m_Height };
		// todo 渲染 透射材质(KHR_materials_transmission、KHR_materials_volume)
		if (!drawables.transmission.empty()) {
			m_RenderPassFullGBuffer.BeginPass(cmdBuf1, renderArea);
			std::stable_sort(drawables.transmission.begin(), drawables.transmission.end(), bcmp);
			draw_pbrpass(m_pDevice, cmdBuf1, pState, &drawables.transmission, &drawables.wireframe[2], bWireframe);
			m_GPUTimer.GetTimeStamp(cmdBuf1, "PBR transmission");
			m_RenderPassFullGBuffer.EndPass(cmdBuf1);
		}
	}

	void Renderer_cx::draw_boxes(VkCommandBuffer cmdBuf1, PerFrame_t* pfd, const scene_state* pState)
	{
		glm::mat4 mCameraCurrViewProj = pfd->mCameraCurrViewProj;
		VkRect2D renderArea = { 0, 0, m_Width, m_Height };
		bool has_box = (pState->bDrawLightFrustum && pfd->lightCount > 0) || (pState->bDrawBoundingBoxes && _robject.size());
		if (has_box)
		{
			m_RenderPassJustDepthAndHdr.BeginPass(cmdBuf1, renderArea);
			// todo 渲染bounding boxes
			if (pState->bDrawBoundingBoxes && _robject.size())
			{
				for (auto it : _robject)
				{
					if (it->m_GLTFBBox)
					{
						it->m_GLTFBBox->Draw(cmdBuf1, mCameraCurrViewProj);
					}
				}
				m_GPUTimer.GetTimeStamp(cmdBuf1, "Bounding Box");
			}

			// draw light's frustums
			if (pState->bDrawLightFrustum)
			{
				SetPerfMarkerBegin(cmdBuf1, "light frustums");
				glm::vec4 vCenter = glm::vec4(0.0f, 0.0f, 0.5f, 0.0f);
				glm::vec4 vRadius = glm::vec4(1.0f, 1.0f, 0.5f, 0.0f);
				glm::vec4 vColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				for (uint32_t i = 0; i < pfd->lightCount; i++)
				{
					glm::mat4 spotlightMatrix = glm::inverse(pfd->lights[i].mLightViewProj);
					glm::mat4 worldMatrix = mCameraCurrViewProj * spotlightMatrix;
					m_WireframeBox.Draw(cmdBuf1, &m_Wireframe, worldMatrix, vCenter, vRadius, vColor);
					//_WireframeSphere.Draw(cmdBuf1, &m_Wireframe, worldMatrix, vCenter, vRadius, vColor);
				}
				m_GPUTimer.GetTimeStamp(cmdBuf1, "Light's frustum");
				SetPerfMarkerEnd(cmdBuf1);
			}
			m_RenderPassJustDepthAndHdr.EndPass(cmdBuf1);
		}
	}

	void Renderer_cx::OnRender(scene_state* pState, const Camera& Cam)
	{
		//printf("OnRender \t\thdr\t%p\n", m_GBuffer->m_HDR.Resource());
		// Let our resource managers do some house keeping 
		m_ConstantBufferRing.OnBeginFrame();

		// command buffer calls
		VkCommandBuffer cmdBuf1 = m_CommandListRing.GetNewCommandList();

		{
			VkCommandBufferBeginInfo cmd_buf_info;
			cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmd_buf_info.pNext = NULL;
			cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			cmd_buf_info.pInheritanceInfo = NULL;
			VkResult res = vkBeginCommandBuffer(cmdBuf1, &cmd_buf_info);
			assert(res == VK_SUCCESS);
		}

		m_GPUTimer.OnBeginFrame(cmdBuf1, &m_TimeStamps);

		// Sets the perFrame data 
		auto pfd = mkPerFrameData(Cam);
		glm::mat4 mCameraCurrViewProj = pfd->mCameraCurrViewProj;
		const bool bWireframe = pState->WireframeMode != (int)scene_state::WireframeMode::WIREFRAME_MODE_OFF;
		Light* lights = pfd->lights;
		auto ubo = &m_ConstantBufferRing;
		int lightCount = pfd->lightCount;
		{
			PerFrame_t* pPerFrame = NULL;
			for (auto& it : _robject)
			{
				if (it->_ptb)
				{
					// fill as much as possible using the GLTF (camera, lights, ...)
					pPerFrame = it->_ptb->m_pGLTFCommon->SetPerFrameData(pfd, Cam, _lights, lightMats);
					// Set some lighting factors
					pPerFrame->iblFactor = pState->IBLFactor;
					pPerFrame->emmisiveFactor = pState->EmissiveFactor;
					pPerFrame->invScreenResolution[0] = 1.0f / ((float)m_Width);
					pPerFrame->invScreenResolution[1] = 1.0f / ((float)m_Height);

					pPerFrame->wireframeOptions.x = (pState->WireframeColor[0]);
					pPerFrame->wireframeOptions.y = (pState->WireframeColor[1]);
					pPerFrame->wireframeOptions.z = (pState->WireframeColor[2]);
					pPerFrame->wireframeOptions.w = 0;
					pPerFrame->lodBias = 0.0f;
					//if (!it->shadowMap)
					//{
					for (size_t i = 0; i < pPerFrame->lightCount; i++)
					{
						//pPerFrame->lights[i].shadowMapIndex = -1;
						//auto& ml = pPerFrame->lights[i];
						//auto lvp = ml.mLightViewProj;
						//if (ml.type == LightType_Directional)
						//{
						//	auto cp = it->_ptb->m_pGLTFCommon;
						//	lvp = cp->ComputeDirectionalLightOrthographicMatrix(ml.mLightView) * ml.mLightView;
						//}
						////ml.mLightViewProj = lvp;
					}
					//}
					if (bWireframe)
					{
						pPerFrame = it->_ptb->m_pGLTFCommon->SetPerFrameData_w(pfd, Cam, _lights, lightMats);
						pPerFrame->iblFactor = pState->IBLFactor;
						pPerFrame->emmisiveFactor = pState->EmissiveFactor;
						pPerFrame->invScreenResolution[0] = 1.0f / ((float)m_Width);
						pPerFrame->invScreenResolution[1] = 1.0f / ((float)m_Height);

						pPerFrame->wireframeOptions.x = (pState->WireframeColor[0]);
						pPerFrame->wireframeOptions.y = (pState->WireframeColor[1]);
						pPerFrame->wireframeOptions.z = (pState->WireframeColor[2]);
						pPerFrame->wireframeOptions.w = (pState->WireframeMode & (int)scene_state::WireframeMode::WIREFRAME_MODE_SOLID_COLOR ? 1.0f : 0.0f);
						pPerFrame->lodBias = 0.0f;
					}
					it->_ptb->SetPerFrameConstants(ubo, bWireframe);
					// 更新骨骼矩阵到ubo
					it->_ptb->SetSkinningMatricesForSkeletons(ubo);
				}
			}
		}

		// Render all shadow maps

		if (_depthpass.size())
		{
			SetPerfMarkerBegin(cmdBuf1, "ShadowPass");

			VkClearValue depth_clear_values[1];
			depth_clear_values[0].depthStencil.depth = 1.0f;
			depth_clear_values[0].depthStencil.stencil = 0;

			VkRenderPassBeginInfo rp_begin;
			rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rp_begin.pNext = NULL;
			rp_begin.renderPass = m_Render_pass_shadow;
			rp_begin.renderArea.offset.x = 0;
			rp_begin.renderArea.offset.y = 0;
			rp_begin.clearValueCount = 1;
			rp_begin.pClearValues = depth_clear_values;
			int idx = 0;
			std::vector<SceneShadowInfo>::iterator ShadowMap = m_shadowMapPool.begin();
			while (ShadowMap < m_shadowMapPool.end())
			{
				// Clear shadow map
				rp_begin.framebuffer = ShadowMap->ShadowFrameBuffer;
				rp_begin.renderArea.extent.width = ShadowMap->ShadowResolution;
				rp_begin.renderArea.extent.height = ShadowMap->ShadowResolution;
				vkCmdBeginRenderPass(cmdBuf1, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
				// Render to shadow map
				SetViewportAndScissor(cmdBuf1, 0, 0, ShadowMap->ShadowResolution, ShadowMap->ShadowResolution);
				for (auto& it : _depthpass)
				{
					if (!it->has_shadowMap)continue;
					// Set per frame constant buffer values
					GltfDepthPass::PerFrame_t* cbPerFrame = it->SetPerFrameConstants();
					auto& ml = lights[ShadowMap->LightIndex];
					auto lvp = ml.mLightViewProj;
					//if (ml.type == LightType_Directional)
					//{
					//	auto cp = it->get_cp();
					//	lvp = cp->ComputeDirectionalLightOrthographicMatrix(ml.mLightView) * ml.mLightView;
					//}
					cbPerFrame->mViewProj = lvp;
					it->Draw(cmdBuf1);
				}
				m_GPUTimer.GetTimeStamp(cmdBuf1, "Shadow Map Render");
				vkCmdEndRenderPass(cmdBuf1);
				++ShadowMap;
			}
			SetPerfMarkerEnd(cmdBuf1);
		}

		// Render Scene to the GBuffer ------------------------------------------------
		SetPerfMarkerBegin(cmdBuf1, "Color pass");

		VkRect2D renderArea = { 0, 0, m_Width, m_Height };
		if (_robject.size() > 0)
		{
			drawables.clear();	// 清空获取可渲染物体
			for (auto it : _robject) {
				it->m_GLTFPBR->BuildBatchLists(&drawables, bWireframe);
			}
			if (!drawables.transmission.empty())
			{
				draw_pbr_opaque(cmdBuf1, pState, bWireframe);		// 渲染不透明物体
				draw_skydome(cmdBuf1, pState, mCameraCurrViewProj);	// 渲染天空盒
				draw_pbr_transparent(cmdBuf1, pState, bWireframe);	// 渲染透明物体
				copy_transmission(cmdBuf1);
				if (!drawables.transparent.empty())
				{
					draw_pbr_opaque(cmdBuf1, pState, bWireframe);	// 有透明物体时，需要重新渲染
					draw_skydome(cmdBuf1, pState, mCameraCurrViewProj);
				}
				draw_pbr_transmission(cmdBuf1, pState, bWireframe);	// 渲染透射材质(KHR_materials_transmission、KHR_materials_volume)
				if (!drawables.transparent.empty())
				{
					draw_pbr_transparent(cmdBuf1, pState, bWireframe);
				}
			}
			else {
				draw_pbr_opaque(cmdBuf1, pState, bWireframe);		// 无透射材质时的渲染
				draw_skydome(cmdBuf1, pState, mCameraCurrViewProj);
				draw_pbr_transparent(cmdBuf1, pState, bWireframe);
			}
			draw_boxes(cmdBuf1, pfd, pState);						// 渲染物体的包围框
		}
		else
		{
			m_RenderPassFullGBufferWithClear.BeginPass(cmdBuf1, renderArea);
			m_RenderPassFullGBufferWithClear.EndPass(cmdBuf1);
			draw_skydome(cmdBuf1, pState, mCameraCurrViewProj);		// 无物体时只渲染天空盒
		}

		VkImageMemoryBarrier barrier[1] = {};
		barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier[0].pNext = NULL;
		barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier[0].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier[0].subresourceRange.baseMipLevel = 0;
		barrier[0].subresourceRange.levelCount = 1;
		barrier[0].subresourceRange.baseArrayLayer = 0;
		barrier[0].subresourceRange.layerCount = 1;
		barrier[0].image = m_GBuffer->m_HDR.Resource();
		vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, barrier);

		SetPerfMarkerEnd(cmdBuf1);
#if 0
		if (has_oit)
		{
			SetPerfMarkerBegin(cmdBuf1, "oit");
			vkr_image_set_layout(cmdBuf1, m_GBuffer->m_HDR.Resource(), VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			vkr_image_set_layout(cmdBuf1, m_GBuffer->m_HDR_oit_accum.Resource(), VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			vkr_image_set_layout(cmdBuf1, m_GBuffer->m_HDR_oit_weight.Resource(), VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			m_oitblendCS.Draw(cmdBuf1, m_GBuffer->m_HDRSRV, m_GBuffer->m_HDR_oit_accumSRV, m_GBuffer->m_HDR_oit_weightSRV, m_Width, m_Height);

			vkr_image_set_layout(cmdBuf1, m_GBuffer->m_HDR.Resource(), VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			vkr_image_set_layout(cmdBuf1, m_GBuffer->m_HDR_oit_accum.Resource(), VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			vkr_image_set_layout(cmdBuf1, m_GBuffer->m_HDR_oit_weight.Resource(), VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);


			SetPerfMarkerEnd(cmdBuf1);

		}
#endif

		// Post proc---------------------------------------------------------------------------

		// Bloom, takes HDR as input and applies bloom to it.
		if (pState->bBloom)
		{
			SetPerfMarkerBegin(cmdBuf1, "PostProcess");
			// Downsample pass
			m_DownSample.Draw(cmdBuf1);
			m_GPUTimer.GetTimeStamp(cmdBuf1, "Downsample");

			// Bloom pass (needs the downsampled data)
			m_Bloom.Draw(cmdBuf1);
			m_GPUTimer.GetTimeStamp(cmdBuf1, "Bloom");
			SetPerfMarkerEnd(cmdBuf1);
		}

		// Apply TAA & Sharpen to m_HDR
		if (pState->bUseTAA)
		{
			m_TAA.m_bSharpening = pState->bTAAsharpening;
			cp_barrier(cmdBuf1, m_GBuffer);

			m_TAA.Draw(cmdBuf1);
			m_GPUTimer.GetTimeStamp(cmdBuf1, "TAA");
		}


#if 1
		// Magnifier Pass: m_HDR as input, pass' own output
		if (pState->bUseMagnifier)
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.pNext = NULL;
			barrier.srcAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
			barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.image = m_MagnifierPS.GetPassOutputResource();
			if (m_bMagResourceReInit)
			{
				barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
				m_bMagResourceReInit = false;
			}
			else
			{
				barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
			}
			// Note: assumes the input texture (specified in OnCreateWindowSizeDependentResources()) is in read state
			m_MagnifierPS.Draw(cmdBuf1, pState->MagnifierParams);
			m_GPUTimer.GetTimeStamp(cmdBuf1, "Magnifier");

		}
#endif 
		// Start tracking input/output resources at this point to handle HDR and SDR render paths 
		VkImage      ImgCurrentInput = pState->bUseMagnifier ? m_MagnifierPS.GetPassOutputResource() : m_GBuffer->m_HDR.Resource();
		VkImageView  SRVCurrentInput = pState->bUseMagnifier ? m_MagnifierPS.GetPassOutputSRV() : m_GBuffer->m_HDRSRV;

		//VkRect2D renderArea = { 0, 0, m_Width, m_Height };
		// If using FreeSync HDR, we need to do these in order: Tonemapping -> GUI -> Color Conversion
		bHDR = _dm != DISPLAYMODE_SDR;
		if (bHDR)
		{
			// In place Tonemapping ------------------------------------------------------------------------
			{
				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.pNext = NULL;
				barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;
				barrier.image = ImgCurrentInput;
				vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

				m_ToneMappingCS.Draw(cmdBuf1, SRVCurrentInput, pState->Exposure, pState->SelectedTonemapperIndex, m_Width, m_Height);

				barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				barrier.image = ImgCurrentInput;
				vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

			}

			// Render HUD  ------------------------------------------------------------------------
			{
#if 1
				if (pState->bUseMagnifier)
				{
					m_MagnifierPS.BeginPass(cmdBuf1, renderArea);
				}
				else
				{
					m_RenderPassJustDepthAndHdr.BeginPass(cmdBuf1, renderArea);
				}

				vkCmdSetScissor(cmdBuf1, 0, 1, &m_RectScissor);
				vkCmdSetViewport(cmdBuf1, 0, 1, &m_Viewport);

				//m_ImGUI.Draw(cmdBuf1);

				if (pState->bUseMagnifier)
				{
					m_MagnifierPS.EndPass(cmdBuf1);
				}
				else
				{
					m_RenderPassJustDepthAndHdr.EndPass(cmdBuf1);
				}

				if (bHDR && !pState->bUseMagnifier)
				{
					VkImageMemoryBarrier barrier = {};
					barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier.pNext = NULL;
					barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					barrier.subresourceRange.baseMipLevel = 0;
					barrier.subresourceRange.levelCount = 1;
					barrier.subresourceRange.baseArrayLayer = 0;
					barrier.subresourceRange.layerCount = 1;
					barrier.image = ImgCurrentInput;
					vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
				}

				m_GPUTimer.GetTimeStamp(cmdBuf1, "GUI Rendering");
#endif
			}
		}
		// submit command buffer
		{
			//print_time a("qsubmit 0");
			VkResult res = vkEndCommandBuffer(cmdBuf1);
			assert(res == VK_SUCCESS);

			VkSubmitInfo submit_info;
			submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.pNext = NULL;
			submit_info.waitSemaphoreCount = 0;
			submit_info.pWaitSemaphores = NULL;
			submit_info.pWaitDstStageMask = NULL;
			submit_info.commandBufferCount = 1;
			submit_info.pCommandBuffers = &cmdBuf1;
			submit_info.signalSemaphoreCount = 1;
			submit_info.pSignalSemaphores = &_fbo.sem;	// 完成发信号
			res = vkQueueSubmit(m_pDevice->graphics_queue, 1, &submit_info, 0);
			assert(res == VK_SUCCESS);

		}
		// Keep tracking input/output resource views 
		ImgCurrentInput = pState->bUseMagnifier ? m_MagnifierPS.GetPassOutputResource() : m_GBuffer->m_HDR.Resource(); // these haven't changed, re-assign as sanity check
		SRVCurrentInput = pState->bUseMagnifier ? m_MagnifierPS.GetPassOutputSRV() : m_GBuffer->m_HDRSRV;         // these haven't changed, re-assign as sanity check

		m_CommandListRing.OnBeginFrame();
		VkCommandBuffer cmdBuf2 = m_CommandListRing.GetNewCommandList();
		{
			VkCommandBufferBeginInfo cmd_buf_info;
			cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmd_buf_info.pNext = NULL;
			cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			cmd_buf_info.pInheritanceInfo = NULL;
			VkResult res = vkBeginCommandBuffer(cmdBuf2, &cmd_buf_info);

			assert(res == VK_SUCCESS);
			// prepare render pass		
			SetPerfMarkerBegin(cmdBuf2, "Swapchain RenderPass");
			VkRenderPassBeginInfo rp_begin = {};
			rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rp_begin.pNext = NULL;
			rp_begin.renderPass = _fbo.renderPass;
			rp_begin.framebuffer = _fbo.framebuffer;
			rp_begin.renderArea.offset.x = 0;
			rp_begin.renderArea.offset.y = 0;
			rp_begin.renderArea.extent.width = m_Width;
			rp_begin.renderArea.extent.height = m_Height;
			VkClearValue clearValues[2] = {};
			clearValues->color = { 0.0f, 0.0f, 0.0f, 0.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };
			rp_begin.clearValueCount = 2;
			rp_begin.pClearValues = clearValues;
			vkCmdBeginRenderPass(cmdBuf2, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetScissor(cmdBuf2, 0, 1, &m_RectScissor);
			vkCmdSetViewport(cmdBuf2, 0, 1, &m_Viewport);
			if (bHDR)
			{
				m_ColorConversionPS.Draw(cmdBuf2, SRVCurrentInput);
				m_GPUTimer.GetTimeStamp(cmdBuf2, "Color Conversion");
			}
			else
			{
				// For SDR pipeline, we apply the tonemapping and then draw the GUI and skip the color conversion
				// 对于 SDR 流水线，我们先应用色调映射，然后绘制 GUI 并跳过颜色转换
				// Tonemapping ------------------------------------------------------------------------ 
				m_ToneMappingPS.Draw(cmdBuf2, SRVCurrentInput, pState->Exposure, pState->SelectedTonemapperIndex);
				m_GPUTimer.GetTimeStamp(cmdBuf2, "Tonemapping");
			}
			// Render HUD  -------------------------------------------------------------------------
			if (hubDraw)
			{
				hubDraw(cmdBuf2);
				m_GPUTimer.GetTimeStamp(cmdBuf2, "HUB Rendering");
			}
			SetPerfMarkerEnd(cmdBuf2);
			m_GPUTimer.OnEndFrame();
			vkCmdEndRenderPass(cmdBuf2);
			res = vkEndCommandBuffer(cmdBuf2);

			assert(res == VK_SUCCESS);
			VkSemaphore ImageAvailableSemaphore;
			VkSemaphore RenderFinishedSemaphores;
			VkPipelineStageFlags submitWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;	//等待所有gpu操作
			VkSubmitInfo submit_info2;
			submit_info2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info2.pNext = NULL;
			submit_info2.waitSemaphoreCount = 1;
			submit_info2.pWaitSemaphores = &_fbo.sem;			// 等待3D场景渲染完成信号再执行 
			submit_info2.pWaitDstStageMask = &submitWaitStage;
			submit_info2.commandBufferCount = 1;
			submit_info2.pCommandBuffers = &cmdBuf2;
			submit_info2.signalSemaphoreCount = 0;
			submit_info2.pSignalSemaphores = 0;					// todo 渲染完成信号，SDL3默认渲染器不支持接入外部信号，所有这里直接WaitForFence

			auto st = vkGetFenceStatus(m_pDevice->m_device, _fbo.fence);
			if (!st)
			{
				vkResetFences(m_pDevice->m_device, 1, &_fbo.fence);
			}
			//printf("Renderer_cx fence %p: %d\n", _fbo.fence, st);
			res = vkQueueSubmit(m_pDevice->graphics_queue, 1, &submit_info2, _fbo.fence);
			assert(res == VK_SUCCESS);
			vkWaitForFences(m_pDevice->m_device, 1, &_fbo.fence, VK_TRUE, UINT64_MAX);
			//vkResetFences(m_pDevice->m_device, 1, &_fbo.fence);
		}
	}
	void Renderer_cx::set_fbo(fbo_info_cx* p, int idx)
	{
		_fbo.fence = p->_fence;
		_fbo.framebuffer = p->framebuffers[idx].framebuffer;
		_fbo.sem = p->framebuffers[idx].semaphore;
		_fbo.renderPass = p->renderPass;
		_fbo.image = p->framebuffers[idx].color._image;
	}
	void Renderer_cx::freeVidMBP()
	{
		m_VidMemBufferPool.FreeUploadHeap();
		m_UploadHeap.OnDestroy();
	}
#endif






#if 1
	draw3d_ctx::draw3d_ctx()
	{
		_time = 0;
		m_bPlay = true;

	}
	draw3d_ctx::~draw3d_ctx() {
		OnDestroy();
		if (_fbo)delete _fbo; _fbo = 0;
	}
	//--------------------------------------------------------------------------------------
	//
	// OnParseCommandLine
	//
	//--------------------------------------------------------------------------------------
	void draw3d_ctx::OnParseCommandLine(LPSTR lpCmdLine, uint32_t* pWidth, uint32_t* pHeight)
	{
		// set some default values
		if (pWidth)
			*pWidth = 1920;
		if (pHeight)
			*pHeight = 1080;
		m_activeCamera = 0;

	}

	VkRenderPass newRenderPass(Device* pDevice, VkFormat format, VkFormat depth_format)
	{
		VkAttachmentDescription depthAttachment;
		VkAttachmentDescription colorAttachments[10];
		uint32_t colorAttanchmentCount = 0;
		bool bClear = true; // 是否清除
		auto addAttachment = bClear ? AttachClearBeforeUse : AttachBlending;
		// fbo颜色渲染结束转换布局SHADER_READ_ONLY
		addAttachment(format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &colorAttachments[colorAttanchmentCount++]);
		if (is_depth_tex(depth_format))
			addAttachment(depth_format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, &depthAttachment);
		return CreateRenderPassOptimal(pDevice->m_device, colorAttanchmentCount, colorAttachments, &depthAttachment);
	}

	void DestroyRenderPass(Device* pDevice, VkRenderPass rp)
	{
		if (rp != VK_NULL_HANDLE)
		{
			vkDestroyRenderPass(pDevice->m_device, rp, nullptr);
		}
	}


	//--------------------------------------------------------------------------------------
	//
	// OnCreate
	//
	//--------------------------------------------------------------------------------------
	void draw3d_ctx::OnCreate()
	{
		// Init the shader compiler
		InitDirectXCompiler();
		m_device->CreateShaderCache();

		// Create a instance of the renderer and initialize it, we need to do that for each GPU
		m_pRenderer = new Renderer_cx(nullptr);
		auto format = VK_FORMAT_R8G8B8A8_UNORM;//VK_FORMAT_R8G8B8A8_SRGB;//  VK_FORMAT_B8G8R8A8_SRGB
		_fbo_renderpass = newRenderPass(m_device, format, VK_FORMAT_D32_SFLOAT);

		auto f = new fbo_info_cx();
		f->_dev = m_device;
		f->colorFormat = format;
		f->initFBO(m_Width, m_Height, 2, _fbo_renderpass);
		m_pRenderer->set_fbo(f, 0);
		m_pRenderer->OnCreate(m_device, _fbo_renderpass);
		_fbo = f;
		OnResize(true);
		OnUpdateDisplay();

		// Init Camera, looking at the origin
		m_camera.LookAt(glm::vec4(0, 10, 15, 0), glm::vec4(0, 0, 0, 0));
		// todo set camera
		m_camera.is_eulerAngles = false;
		float yaw = m_camera.GetYaw();
		float pitch = m_camera.GetPitch();
		int distance = m_camera.GetDistance();
		m_camera.UpdatePreviousMatrices(); // set previous view matrix
		m_camera.UpdateCameraPolar(yaw, pitch, 0.0f, 0.0f, distance);

		// todo 添加默认灯光Add a default light in case there are none
		{
			glm::vec4 src = PolarToVector(AMD_PI_OVER_2, 0.58f) * 3.5f;
			src = { 50,150,50,0 };
			Transform transform = {};
			transform.m_translation = src;
			//transform.LookAt(src, glm::vec4(0, 0, 0, 0), false);
			{
				// todo 平行光
				light_t l = {};
				l._type = light_t::LIGHT_DIRECTIONAL;
				l._intensity = 50.0;
				l._color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				l._range = 500.0;
				//l._outerConeAngle = 0.0f;
				//l._innerConeAngle = 0.0f;
				l._shadowResolution = 1024;
				//m_pRenderer->AddLight(transform, l);
			}

			{
				transform = {};
				transform.LookAt(PolarToVector(AMD_PI_OVER_2, 0.58f) * 3.5f, glm::vec4(0, 0, 0, 0), false);
				light_t l = {};
				l._type = light_t::LIGHT_SPOTLIGHT;
				l._intensity = 50.0;
				l._color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				l._range = 28.0;
				//l._outerConeAngle = AMD_PI_OVER_4;// glm::radians(20.0);
				//l._innerConeAngle = AMD_PI_OVER_4 * 0.9f;
				l._cone_angle = 75;
				l._cone_mix = 0.2;
				l._shadowResolution = 1024;
				l._bias = 0.0007;
				l._rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
				l._position = glm::vec3(0, 6, 0);
				m_pRenderer->AddLight(l);
			}
		}
		// Allocate shadow information (if any)
		m_pRenderer->AllocateShadowMaps();
	}

	//--------------------------------------------------------------------------------------
	//
	// OnDestroy
	//
	//--------------------------------------------------------------------------------------
	void draw3d_ctx::OnDestroy()
	{
		//ImGUI_Shutdown();

		m_device->GPUFlush();

		m_pRenderer->UnloadScene();
		m_pRenderer->OnDestroyWindowSizeDependentResources();
		m_pRenderer->OnDestroy();
		DestroyRenderPass(m_device, _fbo_renderpass); _fbo_renderpass = 0;
		delete m_pRenderer;
		m_pRenderer = 0;
		// shut down the shader compiler 
		m_device->DestroyShaderCache();

		for (auto it : _loaders)
		{
			delete it;
		}
		_loaders.clear();
	}


	//--------------------------------------------------------------------------------------
	//
	// OnResize
	//
	//--------------------------------------------------------------------------------------
	void draw3d_ctx::OnResize(bool resizeRender)
	{
		// destroy resources (if we are not minimized)
		if (resizeRender && m_Width && m_Height && m_pRenderer)
		{
			_fbo->reset_fbo(m_Width, m_Height);
			m_pRenderer->set_fbo(_fbo, 0);
			setidx = curridx = 0;
			m_pRenderer->OnDestroyWindowSizeDependentResources();
			m_pRenderer->OnCreateWindowSizeDependentResources(m_Width, m_Height);
		}
		if (m_Width && m_Height)
		{
			m_camera.SetFov(AMD_PI_OVER_4, m_Width, m_Height, 0.1f, 1000.0f);
			tpfc.set_size((float)m_Width, (float)m_Height);
		}
	}

	//--------------------------------------------------------------------------------------
	//
	// UpdateDisplay
	//
	//--------------------------------------------------------------------------------------
	void draw3d_ctx::OnUpdateDisplay()
	{
		// Destroy resources (if we are not minimized)
		if (m_pRenderer)
		{
			m_pRenderer->OnUpdateDisplayDependentResources(_fbo_renderpass, _dm, false);// m_UIState.bUseMagnifier);
		}
	}

	//--------------------------------------------------------------------------------------
	//
	// add_model
	// TODO 加载模型文件
	//--------------------------------------------------------------------------------------
	void draw3d_ctx::add_model(const char* fn, const glm::vec3& pos, float scale, uint32_t instanceCount, bool has_shadowMap)
	{
		//print_time Pt("push gltf", 1);
		std::string fn1 = fn && *fn ? fn : "";
		if (instanceCount < 1 || instanceCount == -1)instanceCount = 1;
		std::thread a([=]()
			{
				GLTFCommon* pgc = 0;
				{
					print_time Pt("load gltf", 1);
					pgc = new GLTFCommon();
					pgc->has_shadowMap = has_shadowMap;
					pgc->_pos = pos;
					pgc->_scale = scale;
					pgc->instanceCount = instanceCount;
					pgc->instance_mat.resize(instanceCount);
#ifdef DEBUG
					float x = 2;
					for (size_t i = 0; i < instanceCount; i++)
					{
						pgc->instance_mat[i] = glm::translate(glm::vec3(x * i, 0.0f, 0.0f)) * glm::rotate(glm::radians(i * 45.0f), glm::vec3(0.0, 1.0, 0.0));
					}
#endif
					if (pgc->Load(fn1.c_str(), "") == false)
					{
						printf("The selected model couldn't be found, please check the documentation!\n");
						delete pgc;
						return;
					}
				}
				if (pgc)
				{
					std::unique_lock<std::mutex> lock(m_ltsm);
					_lts.push(pgc);
				}
			}
		);
		//a.join();
		a.detach();
	}


	//--------------------------------------------------------------------------------------
	//
	// OnUpdate
	//
	//--------------------------------------------------------------------------------------
	void draw3d_ctx::OnUpdate()
	{
		if (bShowProfilerWindow)
		{
			constexpr size_t NUM_FRAMES = 128;
			static float FRAME_TIME_ARRAY[NUM_FRAMES] = { 0 };

			// track highest frame rate and determine the max value of the graph based on the measured highest value
			static float RECENT_HIGHEST_FRAME_TIME = 0.0f;
			constexpr int FRAME_TIME_GRAPH_MAX_FPS[] = { 800, 240, 120, 90, 60, 45, 30, 15, 10, 5, 4, 3, 2, 1 };
			static float  FRAME_TIME_GRAPH_MAX_VALUES[_countof(FRAME_TIME_GRAPH_MAX_FPS)] = { 0 }; // us
			for (int i = 0; i < _countof(FRAME_TIME_GRAPH_MAX_FPS); ++i) { FRAME_TIME_GRAPH_MAX_VALUES[i] = 1000000.f / FRAME_TIME_GRAPH_MAX_FPS[i]; }

			//scrolling data and average FPS computing
			const std::vector<TimeStamp>& timeStamps = m_pRenderer->GetTimingValues();
			const bool bTimeStampsAvailable = timeStamps.size() > 0;
			if (bTimeStampsAvailable)
			{
				RECENT_HIGHEST_FRAME_TIME = 0;
				FRAME_TIME_ARRAY[NUM_FRAMES - 1] = timeStamps.back().m_microseconds;
				for (uint32_t i = 0; i < NUM_FRAMES - 1; i++)
				{
					FRAME_TIME_ARRAY[i] = FRAME_TIME_ARRAY[i + 1];
				}
				RECENT_HIGHEST_FRAME_TIME = std::max(RECENT_HIGHEST_FRAME_TIME, FRAME_TIME_ARRAY[NUM_FRAMES - 1]);
			}
			const float& frameTime_us = FRAME_TIME_ARRAY[NUM_FRAMES - 1];
			const float  frameTime_ms = frameTime_us * 0.001f;
			const int fps = bTimeStampsAvailable ? static_cast<int>(1000000.0f / frameTime_us) : 0;

			const char* pStrUnit = m_UIState.bShowMilliseconds ? "ms" : "us";
			// UI  
			//ImGui::Begin("PROFILER (F2)", &m_UIState.bShowProfilerWindow);
			_tlabs.clear();
			std::string txtrs = format("Resolution : %ix%i", m_Width, m_Height);
			std::string txtAPI = format("API        : %s", systemi.mGfxAPI.c_str());
			std::string txtGPU = format("GPU        : %s", systemi.mGPUName.c_str());
			std::string txtCPU = format("CPU        : %s", systemi.mCPUName.c_str());
			std::string txtFPS = format("FPS        : %d (%.2f %s)", fps, m_UIState.bShowMilliseconds ? frameTime_ms : frameTime_us, pStrUnit);
			_tlabs.push_back(txtrs);
			_tlabs.push_back(txtAPI);
			_tlabs.push_back(txtGPU);
			_tlabs.push_back(txtCPU);
			_tlabs.push_back(txtFPS);
			{
				// find the index of the FrameTimeGraphMaxValue as the next higher-than-recent-highest-frame-time in the pre-determined value list
				size_t iFrameTimeGraphMaxValue = 0;
				for (uint64_t i = 0; i < _countof(FRAME_TIME_GRAPH_MAX_VALUES); ++i)
				{
					if (RECENT_HIGHEST_FRAME_TIME < FRAME_TIME_GRAPH_MAX_VALUES[i]) // FRAME_TIME_GRAPH_MAX_VALUES are in increasing order
					{
						iFrameTimeGraphMaxValue = std::min(_countof(FRAME_TIME_GRAPH_MAX_VALUES) - 1, i + 1);
						break;
					}
				}
				//ImGui::PlotLines("", FRAME_TIME_ARRAY, NUM_FRAMES, 0, "GPU frame time (us)", 0.0f, FRAME_TIME_GRAPH_MAX_VALUES[iFrameTimeGraphMaxValue], ImVec2(0, 80));
				_tlabs.push_back("GPU Timings");
				for (uint32_t i = 0; i < timeStamps.size(); i++)
				{
					float value = m_UIState.bShowMilliseconds ? timeStamps[i].m_microseconds / 1000.0f : timeStamps[i].m_microseconds;
					std::string txt = format("%-18s: %7.2f %s", timeStamps[i].m_label.c_str(), value, pStrUnit);
					_tlabs.push_back(txt);
				}
				_cts.clear();
				for (auto& it : _tlabs) { _cts += it + "\n"; }
				_labelstr.swap(_cts);
			}
		}
		if (io.WantCaptureMouse)
		{
			io.MouseDelta.x = 0;
			io.MouseDelta.y = 0;
			io.wheel = {};
		}

		// Update Camera
		UpdateCamera(m_camera);
		if (m_UIState.bUseTAA)
		{
			static uint32_t Seed = 0;
			m_camera.SetProjectionJitter(m_Width, m_Height, Seed);
		}
		else
			m_camera.SetProjectionJitter(0.f, 0.f);

		// Keyboard & Mouse
		HandleInput();

		// Animation Update
		if (m_bPlay)
			_time += io.DeltaTime;// (float)m_deltaTime / 1000.0f; // animation time in seconds

		int i = 0;
		static float speed = 1.0;
		for (auto it : _loaders)
		{
			auto n = it->get_animation_count();
			it->update(_time * speed);
			auto m = glm::translate(glm::mat4(1.0f), it->_pos) * glm::scale(glm::mat4(1.0f), glm::vec3(it->_scale));
			it->TransformScene(0, m);
		}
	}

	void draw3d_ctx::HandleInput()
	{
#if 0
		auto fnIsKeyTriggered = [&io](char key) { return io.KeysDown[key] && io.KeysDownDuration[key] == 0.0f; };

		// Handle Keyboard/Mouse input here

		/* MAGNIFIER CONTROLS */
		if (fnIsKeyTriggered('L'))                       m_UIState.ToggleMagnifierLock();
		if (fnIsKeyTriggered('M') || io.MouseClicked[2]) // middle mouse / M key toggles magnifier
		{
			m_UIState.bUseMagnifier ^= 1;
			// We need to update IMGUI's renderpass to draw to magnfier's renderpass when in hdr
			// Hence, flush GPU and update it through OnUpdateDisplay
			// Which needs to do the same thing when display mode is changed.
			m_device.GPUFlush();
			OnUpdateDisplay();
		}

		if (io.MouseClicked[1] && m_UIState.bUseMagnifier) // right mouse click
			m_UIState.ToggleMagnifierLock();
#endif
	}

	glm::vec4 onMoveWASD(const bool keyDown[256])
	{
		float scale = keyDown[VK_SHIFT] ? 5.0f : 1.0f;
		float x = 0, y = 0, z = 0;
		int t = 0;
		// x=ad y=ws z=zx
		if (keyDown['A'])
		{
			t = 6;
			x = scale;
		}
		else if (keyDown['D'])
		{
			t = 4;
			x = -scale;
		}
		if (keyDown['W'])
		{
			t = 8;
			y = -scale;
		}
		else if (keyDown['S'])
		{
			t = 2;
			y = scale;
		}
		if (keyDown['Z'])
		{
			t = 1;
			z = scale;
		}
		else if (keyDown['X'])
		{
			t = 3;
			z = -scale;
		}

		return glm::vec4(x, y, z, t);
	}
	void draw3d_ctx::UpdateCamera(Camera& cam)
	{
		float yaw = cam.GetYaw();
		float pitch = cam.GetPitch();
		int distance = cam.GetDistance();

		cam.UpdatePreviousMatrices(); // set previous view matrix
#if 1
		//yaw = 0;
		//pitch = 0.0;
		// Sets Camera based on UI selection (WASD, Orbit or any of the GLTF cameras)
		if ((io.KeyCtrl == false) && (io.MouseDown[0] == true))
		{
			yaw += io.MouseDelta.x;
			pitch += io.MouseDelta.y;
		}
		if (_customize_camera)
		{
			// todo 相机
			int wy = io.wheel.y;
			if (!wy && (!io.MouseDown[0] || (!io.MouseDelta.x && !io.MouseDelta.y)))
			{
				auto vw = onMoveWASD(io.KeysDown);
				if (vw.w > 0)
				{
					tpfc.keyMovement(vw, io.DeltaTime);
				}
			}
			//鼠标移动位置  
			tpfc.mouseMovement(io.MouseDelta.x, -io.MouseDelta.y, io.DeltaTime, io.MouseDown[0]);
			cam.set_mat(tpfc.view, tpfc.cameraPos);
		}
		else
		{
			// Choose camera movement depending on setting
			if (m_activeCamera == 0)
			{
				// If nothing has changed, don't calculate an update (we are getting micro changes in view causing bugs)
				int wy = io.wheel.y;
				if (!wy && (!io.MouseDown[0] || (!io.MouseDelta.x && !io.MouseDelta.y)))
				{
					//pitch = io.DeltaTime * 60;
					//yaw = io.DeltaTime * 60;
					return;
				}

				//  Orbiting
				distance -= (float)io.wheel.y;// / 3.0f;
				distance = std::max<float>(distance, 0.1f);

				bool panning = (io.KeyCtrl == true) && (io.MouseDown[0] == true);
				if (panning) { yaw = 0; pitch = 0; }
				//if (yaw < 0)yaw = 0;//偏航角
				//if (pitch < 0)pitch = 0;//俯仰角
				cam.UpdateCameraPolar(yaw, pitch,
					panning ? -io.MouseDelta.x : 0.0f,
					panning ? io.MouseDelta.y : 0.0f,
					distance);

			}
			else if (m_activeCamera == 1)
			{
				//  WASD
				cam.UpdateCameraWASD(yaw, pitch, io.KeysDown, io.DeltaTime);
			}
		}

#endif
		if (m_activeCamera > 1)
		{
			// Use a camera from the GLTF
			//m_pGltfLoader->GetCamera(m_activeCamera - 2, &cam);
		}
	}

	void draw3d_ctx::set_fboidx(int idx)
	{
		setidx = idx;
	}

	//--------------------------------------------------------------------------------------
	//
	// OnRender, updates the state from the UI, animates, transforms and renders the scene
	//
	//--------------------------------------------------------------------------------------
	void draw3d_ctx::OnRender()
	{
		if (_lts.size() || _tmpgc)
		{
			// the scene loads in chuncks, that way we can show a progress bar 
			if (!_tmpgc) {
				std::unique_lock<std::mutex> lock(m_ltsm);
				_tmpgc = _lts.front(); _lts.pop();
			}
			loadingStage = m_pRenderer->load_model(_tmpgc, loadingStage);
			if (loadingStage == 0)
			{
				_loaders.push_back(_tmpgc);
				_tmpgc = 0;
			}
		}
		// Do Render frame using AFR
		m_pRenderer->OnRender(&m_UIState, m_camera);
		if (setidx != curridx) {
			curridx = setidx;
			m_pRenderer->set_fbo(_fbo, curridx);
		}
	}
#endif

	bool has_fh(const char* v, char c) {
		bool r = false;
		for (; v && *v; v++)
		{
			if (*v == c) { r = true; break; }
		}
		return r;
	}


	pipeinfo_cx::pipeinfo_cx()
	{
	}

	pipeinfo_cx::~pipeinfo_cx()
	{
	}
	void pipeinfo_cx::freeobj()
	{
		vkDestroyPipeline(pDevice->m_device, m_pipeline, nullptr);
		vkDestroyPipelineLayout(pDevice->m_device, m_pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(pDevice->m_device, m_descriptorSetLayout, NULL);
		for (auto it : dset)
		{
			if (it)
				m_pResourceViewHeaps->FreeDescriptor(it);
		}
	}
	void pipeinfo_cx::compiler_pipe(const char* vertexShader, const char* pixelShader, Device* pdev)
	{
		VkResult res;
		pDevice = pdev;
		// Compile and create shaders

		DefineList attributeDefines;

		VkPipelineShaderStageCreateInfo m_vertexShader, m_fragmentShader;
		if (has_fh(vertexShader, '='))
		{
			res = VKCompileFromString(pDevice->m_device, SST_GLSL, VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "main", "", &attributeDefines, &m_vertexShader);
			assert(res == VK_SUCCESS);
		}
		else
		{
			res = VKCompileFromFile(pDevice->m_device, VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "main", "", &attributeDefines, &m_vertexShader);
			assert(res == VK_SUCCESS);
		}
		if (has_fh(pixelShader, '='))
		{
			res = VKCompileFromString(pDevice->m_device, SST_GLSL, VK_SHADER_STAGE_FRAGMENT_BIT, pixelShader, "main", "", &attributeDefines, &m_fragmentShader);
			assert(res == VK_SUCCESS);
		}
		else
		{
			res = VKCompileFromFile(pDevice->m_device, VK_SHADER_STAGE_FRAGMENT_BIT, pixelShader, "main", "", &attributeDefines, &m_fragmentShader);
			assert(res == VK_SUCCESS);
		}

		shaderStages = { m_vertexShader, m_fragmentShader };
		return;
	}
	void pipeinfo_cx::set_pipeinfo(DynamicBufferRing* dbr, ResourceViewHeaps* rvh, VkRenderPass rp)
	{
		m_pDynamicBufferRing = dbr;
		m_pResourceViewHeaps = rvh;
		renderPass = rp;
		struct per_object_t
		{
			glm::mat4 m_mWorldViewProj;
		};
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(2);
		layoutBindings[0].binding = 0;
		layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		layoutBindings[0].descriptorCount = 1;
		layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBindings[0].pImmutableSamplers = NULL;
		layoutBindings[1].binding = 1;
		layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindings[1].descriptorCount = 1;
		layoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBindings[1].pImmutableSamplers = NULL;

		m_pResourceViewHeaps->CreateDescriptorSetLayout(&layoutBindings, &m_descriptorSetLayout);

		/////////////////////////////////////////////
		// Create the pipeline layout using the descriptoset

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
		pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pPipelineLayoutCreateInfo.pNext = NULL;
		pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
		pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)1;
		pPipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;

		auto res = vkCreatePipelineLayout(pDevice->m_device, &pPipelineLayoutCreateInfo, NULL, &m_pipelineLayout);
		assert(res == VK_SUCCESS);

		/////////////////////////////////////////////
		// Create pipeline

		// Create the input attribute description / input layout(in DX12 jargon)
		//
		VkVertexInputBindingDescription vi_binding = {};
		vi_binding.binding = 0;
		vi_binding.stride = sizeof(float) * 5 + sizeof(int) * 2;
		vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		/*
		layout(location = 0) in vec3 pos;
		layout(location = 1) in vec2 uv;
		layout(location = 2) in vec4 col;
		layout(location = 3) in vec4 col1;
		*/
		VkVertexInputAttributeDescription vi_attrs[] =
		{
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
			{ 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 },
			{ 0, 0, VK_FORMAT_R8G8B8A8_UNORM, 0 },
			{ 0, 0, VK_FORMAT_R8G8B8A8_UNORM, 0 },
		};

		VkPipelineVertexInputStateCreateInfo vi = {};
		vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vi.pNext = NULL;
		vi.flags = 0;
		vi.vertexBindingDescriptionCount = 1;
		vi.pVertexBindingDescriptions = &vi_binding;
		vi.vertexAttributeDescriptionCount = _countof(vi_attrs);
		vi.pVertexAttributeDescriptions = vi_attrs;

		// input assembly state
		//
		VkPipelineInputAssemblyStateCreateInfo ia;
		ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		ia.pNext = NULL;
		ia.flags = 0;
		ia.primitiveRestartEnable = VK_FALSE;
		ia.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

		// rasterizer state
		//
		VkPipelineRasterizationStateCreateInfo rs;
		rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rs.pNext = NULL;
		rs.flags = 0;
		rs.polygonMode = VK_POLYGON_MODE_FILL;
		rs.cullMode = VK_CULL_MODE_NONE;
		rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rs.depthClampEnable = VK_FALSE;
		rs.rasterizerDiscardEnable = VK_FALSE;
		rs.depthBiasEnable = VK_FALSE;
		rs.depthBiasConstantFactor = 0;
		rs.depthBiasClamp = 0;
		rs.depthBiasSlopeFactor = 0;
		rs.lineWidth = 2.0f;

		VkPipelineColorBlendAttachmentState att_state[1];
		att_state[0].colorWriteMask = allBits;
		att_state[0].blendEnable = VK_TRUE;
		att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
		att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
		att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

		// Color blend state
		//
		VkPipelineColorBlendStateCreateInfo cb;
		cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cb.flags = 0;
		cb.pNext = NULL;
		cb.attachmentCount = 1;
		cb.pAttachments = att_state;
		cb.logicOpEnable = VK_FALSE;
		cb.logicOp = VK_LOGIC_OP_NO_OP;
		cb.blendConstants[0] = 1.0f;
		cb.blendConstants[1] = 1.0f;
		cb.blendConstants[2] = 1.0f;
		cb.blendConstants[3] = 1.0f;

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			//VK_DYNAMIC_STATE_CULL_MODE,
			//VK_DYNAMIC_STATE_FRONT_FACE
		};
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pNext = NULL;
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();

		// view port state
		//
		VkPipelineViewportStateCreateInfo vp = {};
		vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vp.pNext = NULL;
		vp.flags = 0;
		vp.viewportCount = 1;
		vp.scissorCount = 1;
		vp.pScissors = NULL;
		vp.pViewports = NULL;

		// depth stencil state
		//
		VkPipelineDepthStencilStateCreateInfo ds;
		ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		ds.pNext = NULL;
		ds.flags = 0;
		ds.depthTestEnable = true;
		ds.depthWriteEnable = true;
		ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		ds.depthBoundsTestEnable = VK_FALSE;
		ds.stencilTestEnable = VK_FALSE;
		ds.back.failOp = VK_STENCIL_OP_KEEP;
		ds.back.passOp = VK_STENCIL_OP_KEEP;
		ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
		ds.back.compareMask = 0;
		ds.back.reference = 0;
		ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
		ds.back.writeMask = 0;
		ds.minDepthBounds = 0;
		ds.maxDepthBounds = 0;
		ds.stencilTestEnable = VK_FALSE;
		ds.front = ds.back;

		// multi sample state
		//
		VkPipelineMultisampleStateCreateInfo ms;
		ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms.pNext = NULL;
		ms.flags = 0;
		ms.pSampleMask = NULL;
		ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;// sampleDescCount;
		ms.sampleShadingEnable = VK_FALSE;
		ms.alphaToCoverageEnable = VK_FALSE;
		ms.alphaToOneEnable = VK_FALSE;
		ms.minSampleShading = 0.0;

		// create pipeline 

		VkGraphicsPipelineCreateInfo pipeline = {};
		pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline.pNext = NULL;
		pipeline.layout = m_pipelineLayout;
		pipeline.basePipelineHandle = VK_NULL_HANDLE;
		pipeline.basePipelineIndex = 0;
		pipeline.flags = 0;
		pipeline.pVertexInputState = &vi;
		pipeline.pInputAssemblyState = &ia;
		pipeline.pRasterizationState = &rs;
		pipeline.pColorBlendState = &cb;
		pipeline.pTessellationState = NULL;
		pipeline.pMultisampleState = &ms;
		pipeline.pDynamicState = &dynamicState;
		pipeline.pViewportState = &vp;
		pipeline.pDepthStencilState = &ds;
		pipeline.pStages = shaderStages.data();
		pipeline.stageCount = (uint32_t)shaderStages.size();
		pipeline.renderPass = renderPass;
		pipeline.subpass = 0;

		res = vkCreateGraphicsPipelines(pDevice->m_device, pDevice->GetPipelineCache(), 1, &pipeline, NULL, &m_pipeline);
		assert(res == VK_SUCCESS);
		SetResourceName(pDevice->m_device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_pipeline, "Wireframe P");

	}

	VkDescriptorSet pipeinfo_cx::new_dset()
	{
		VkDescriptorSet r = {};
		return r;
		//m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(per_object_t), m_descriptorSet);
	}


}
//!vkr


vkdg_cx::vkdg_cx()
{
}

vkdg_cx::~vkdg_cx()
{
	auto q = (vkr::upload_cx*)qupload;
	if (q)
		delete q;
	qupload = 0;
	if (ctx)
		delete ctx;
	ctx = 0;
	DeviceShutdown((vkr::Device*)dev);
}


void vkdg_cx::update(mouse_state_t* io)
{
	if (ctx) {
		auto tx = (vkr::draw3d_ctx*)ctx;
		tx->io = *io;
		{
			io->MouseDelta.x = 0;
			io->MouseDelta.y = 0;
			io->wheel.y = 0;
		}
		if (_state.bUseMagnifier) {
			auto& params = _state.MagnifierParams;
			params.uImageHeight = height;
			params.uImageWidth = width;
			params.iMousePos[0] = _state.bLockMagnifierPosition ? _state.LockedMagnifiedScreenPositionX : static_cast<int>(io->MousePos.x);
			params.iMousePos[1] = _state.bLockMagnifierPosition ? _state.LockedMagnifiedScreenPositionY : static_cast<int>(io->MousePos.y);

			//ImGui::Checkbox("Show Magnifier (M)", &_state.bUseMagnifier);

			//DisableUIStateBegin(_state.bUseMagnifier);
			//{
			//	// Use a local bool state here to track locked state through the UI widget,
			//	// and then call ToggleMagnifierLockedState() to update the persistent state (_state).
			//	// The keyboard input for toggling lock directly operates on the persistent state.
			//	const bool bIsMagnifierCurrentlyLocked = _state.bLockMagnifierPosition;
			//	bool bMagnifierToggle = bIsMagnifierCurrentlyLocked;
			//	ImGui::Checkbox("Lock Position (L)", &bMagnifierToggle);

			//	if (bMagnifierToggle != bIsMagnifierCurrentlyLocked)
			//		_state.ToggleMagnifierLock();

			//	ImGui::SliderFloat("Screen Size", &params.fMagnifierScreenRadius, MAGNIFIER_RADIUS_MIN, MAGNIFIER_RADIUS_MAX);
			//	ImGui::SliderFloat("Magnification", &params.fMagnificationAmount, MAGNIFICATION_AMOUNT_MIN, MAGNIFICATION_AMOUNT_MAX);
			//	ImGui::SliderInt("OffsetX", &params.iMagnifierOffset[0], -m_Width, m_Width);
			//	ImGui::SliderInt("OffsetY", &params.iMagnifierOffset[1], -m_Height, m_Height);
			//}
			//DisableUIStateEnd(_state.bUseMagnifier);
		}


		tx->m_UIState = _state;
		tx->OnUpdate();
	}
}

const char* vkdg_cx::get_label()
{
	auto tx = (vkr::draw3d_ctx*)ctx;
	return tx ? tx->_labelstr.c_str() : "";
}

void vkdg_cx::on_render()
{
	if (ctx) {
		auto tx = (vkr::draw3d_ctx*)ctx;
		tx->OnRender();

	}
}

image_vkr vkdg_cx::get_vkimage(int idx)
{
	image_vkr r = {};
	if (ctx) {
		auto tx = (vkr::draw3d_ctx*)ctx;
		if (tx->_fbo && tx->_fbo->framebuffers.size() > idx) {
			r.vkimage = tx->_fbo->framebuffers[idx].color._image;
			r.size = { tx->_fbo->framebuffers[idx].color.width,tx->_fbo->framebuffers[idx].color.height };
			width = r.size.x;
			height = r.size.y;
		}
	}
	return r;
}
//glm::vec3 vkdg_cx::get_value(int idx)
//{
//	glm::vec3 r = {};
//	if (!ctx) return r;
//	auto tx = (vkr::sample_cx*)ctx;
//	switch (idx)
//	{
//	case 0:
//		r = tx->m_camera.GetPosition();
//		break;
//	case 1:
//		r = { tx->m_camera.GetYaw(),tx->m_camera.GetPitch(),0 };
//		break;
//	default:
//		break;
//	}
//	return r;
//}
void vkdg_cx::resize(int w, int h) {
	if (ctx) {
		auto tx = (vkr::draw3d_ctx*)ctx;
		if (w > 0 && h > 0 && (w != tx->m_Width || h != tx->m_Height)) {
			tx->m_Width = w;
			tx->m_Height = h;
			tx->OnResize(true);
		}
	}
}
void vkdg_cx::resize(const glm::vec2& size)
{
	resize(size.x, size.y);
}
void vkdg_cx::save_fbo(int idx)
{
	if (ctx) {
		auto q = (vkr::upload_cx*)qupload;
		if (!q)
		{
			q = new vkr::upload_cx();
			q->init((vkr::Device*)dev, width * height * 6, 0);
			qupload = q;
		}
		auto tx = (vkr::draw3d_ctx*)ctx;
		if (tx->_fbo && tx->_fbo->framebuffers.size() > idx) {
			auto ptex = &tx->_fbo->framebuffers[idx].color;
			dt.resize(width * height);
			ptex->get_buffer((char*)dt.data(), q);
		}
	}
}

void get_tex_buffer(vkr::Texture* tex, char* outbuf, vkr::upload_cx* q)//, dvk_queue* q)
{
	vkr::dvk_staging_buffer staging;
	int width = tex->GetWidth(), height = tex->GetHeight();
	size_t as = width * height * 4, align = 512;
	auto image = tex->Resource();
	as = vkr::alignUp(as, align);
	staging.initBuffer(q->_dev, as);
	VkImageAspectFlags aspectMask = (VkImageAspectFlags)(vkr::is_depth_tex((VkFormat)tex->GetFormat()) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);
	VkBufferImageCopy bufferCopyRegion = {};
	bufferCopyRegion.imageSubresource.aspectMask = aspectMask;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = width;
	bufferCopyRegion.imageExtent.height = height;
	bufferCopyRegion.imageExtent.depth = 1;
	bufferCopyRegion.bufferOffset = 0;
	bufferCopyRegion.bufferImageHeight = height;
	bufferCopyRegion.bufferRowLength = width;
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = aspectMask;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = tex->GetMipCount();// mipLevels;
	subresourceRange.layerCount = 1;
	VkImageLayout il = (VkImageLayout)GetImageLayout(vkr::ImageLayoutBarrier::SHADER_READ);
	q->add_copy2mem(image, bufferCopyRegion, subresourceRange, aspectMask, il, staging.buffer);
	q->flushAndFinish();
	staging.getBuffer(outbuf, -1);
}
image_dr vkdg_cx::save_shadow(int idx)
{
	if (!ctx) return {};
	auto tx = (vkr::draw3d_ctx*)ctx;
	auto& smp = tx->m_pRenderer->m_shadowMapPool;
	auto& m = smp[idx];

	auto q = (vkr::upload_cx*)qupload;
	image_dr r = {};
	if (!q)
	{
		q = new vkr::upload_cx();
		q->init((vkr::Device*)dev, width * height * 6, 0);
		qupload = q;
	}
	if (q) {
		dt.resize(m.ShadowMap.GetWidth() * m.ShadowMap.GetHeight());
		get_tex_buffer(&m.ShadowMap, (char*)dt.data(), q);
		r.data = dt.data();
		r.size = { m.ShadowMap.GetWidth(),m.ShadowMap.GetHeight() };
	}
	return r;
}

void vkdg_cx::copy2(int idx, void* vkptr)
{
	if (ctx && vkptr) {
		auto q = (vkr::upload_cx*)qupload;
		if (!q)
		{
			q = new vkr::upload_cx();
			q->init((vkr::Device*)dev, width * height * 6, 0);
			qupload = q;
		}
		auto tx = (vkr::draw3d_ctx*)ctx;
		if (tx->_fbo && tx->_fbo->framebuffers.size() > idx) {
			auto ptex = &tx->_fbo->framebuffers[idx].color;
			ptex->copy2image((VkImage)vkptr, q);
		}
	}

}
//void* vkdg_cx::new_pipe(const char* vertexShader, const char* pixelShader)
//{
//	void* p = 0;
//	if (ctx) {
//		auto tx = (vkr::sample_cx*)ctx;
//		//vkr::new_pipe(vertexShader, pixelShader, tx->m_device);
//	}
//	return p;
//}
void vkdg_cx::add_gltf(const char* fn, const glm::vec3& pos, float scale, uint32_t instanceCount, bool has_shadowMap)
{
	if (!ctx || !fn)return;
	auto tx = (vkr::draw3d_ctx*)ctx;
	tx->add_model(fn, pos, scale, instanceCount, has_shadowMap);
}

vkr::light_t* vkdg_cx::get_light(size_t idx)
{
	vkr::light_t* p = 0;
	if (ctx) {
		auto tx = (vkr::draw3d_ctx*)ctx;
		if (tx->m_pRenderer)
			p = tx->m_pRenderer->get_light(idx);
	}
	return p;
}
size_t vkdg_cx::get_light_size()
{
	size_t c = 0;
	if (ctx) {
		auto tx = (vkr::draw3d_ctx*)ctx;
		if (tx->m_pRenderer)
			c = tx->m_pRenderer->get_light_size();
	}
	return c;
}

void* new_instance()
{
	vkr::InstanceProperties ip;
	ip.Init();
	vkr::SetEssentialInstanceExtensions(1, 1, &ip);
	auto apiVersion3 = VK_API_VERSION_1_3;
	auto apiVersion4 = VK_API_VERSION_1_4;
	VkInstanceCreateInfo inst_info = {};
	std::vector<const char*> instance_layer_names;
	std::vector<const char*> instance_extension_names;
	ip.GetExtensionNamesAndConfigs(&instance_layer_names, &instance_extension_names);
	VkInstance instance = {};
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = "vkcmp";
	app_info.applicationVersion = 1;
	app_info.pEngineName = "pnguo";
	app_info.engineVersion = 1;
	app_info.apiVersion = VK_API_VERSION_1_3;
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pNext = 0;
	inst_info.flags = 0;
	inst_info.pApplicationInfo = &app_info;
	inst_info.enabledLayerCount = (uint32_t)instance_layer_names.size();
	inst_info.ppEnabledLayerNames = (uint32_t)instance_layer_names.size() ? instance_layer_names.data() : NULL;
	inst_info.enabledExtensionCount = (uint32_t)instance_extension_names.size();
	inst_info.ppEnabledExtensionNames = instance_extension_names.data();
	VkResult res = vkCreateInstance(&inst_info, NULL, &instance);
	assert(res == VK_SUCCESS);
	return instance;
}

void free_instance(void* inst)
{
	if (inst)
		vkDestroyInstance((VkInstance)inst, 0);
}

vkdg_cx* new_vkdg(void* inst, void* phy, void* dev, const char* shaderLibDir, const char* shaderCacheDir, const char* devname)
{
	dev_info_cx c[1] = {};
	c->inst = inst;
	c->phy = phy;
	c->vkdev = dev;
	auto p = new vkdg_cx();
	vkr::Log::InitLogSystem();
	if (c) {
		vkr::SystemInfo m_systemInfo;
		bool cpuvalid = false;
		bool gpuvalid = false;
#ifdef _DEBUG
		cpuvalid = 1;
		gpuvalid = 1;
#endif // _DEBUG

		auto dev = new vkr::Device();

		dev->OnCreate(c, cpuvalid, gpuvalid, 0, devname, &p->dev_name);
		dev->CreatePipelineCache();
		// Get system info
		std::string dummyStr;
		dev->GetDeviceInfo(&m_systemInfo.mGPUName, &dummyStr); // 2nd parameter is unused
		m_systemInfo.mCPUName = GetCPUNameString();
		m_systemInfo.mGfxAPI = "Vulkan";
		p->_dev_info = c[0];
		p->dev = dev;
		auto tx = new vkr::draw3d_ctx();
		tx->m_device = dev;
		tx->systemi = m_systemInfo;
		tx->OnParseCommandLine(0, 0, 0);
		if (!shaderLibDir || !(*shaderLibDir))
		{
			shaderLibDir = "ShaderLibVK";
		}
		if (!shaderCacheDir || !(*shaderCacheDir)) {
			shaderCacheDir = "cache/shadervk/";
		}
		vkr::InitShaderCompilerCache(shaderLibDir, shaderCacheDir);
		tx->OnCreate();
		p->_dev_info = { dev->m_instance,dev->m_physicaldevice,dev->m_device };
		p->ctx = tx;
		p->renderpass_opaque = tx->m_pRenderer->m_RenderPassFullGBufferWithClear.m_renderPass;
		p->renderpass_justdepth = tx->m_pRenderer->m_RenderPassJustDepthAndHdr.m_renderPass;
		p->renderpass_transparent = tx->m_pRenderer->m_RenderPassFullGBuffer.m_renderPass;
		p->renderpass_fbo = tx->_fbo_renderpass;
	}
	return p;
}

void free_vkdg(vkdg_cx* p)
{
	if (p)
	{
		delete p;
	}
}
//
//void add_gltf(vkdg_cx* p, const char* fn, const float* pos, float scale)
//{
//	if (!p || !fn)return;
//	glm::vec3 ps = {};
//	if (pos)
//		ps = { pos[0],pos[1],pos[2] };
//	p->add_gltf(fn, ps, scale);
//}

#endif 
namespace vkr {

	BBoxPass::BBoxPass()
	{
	}

	BBoxPass::~BBoxPass()
	{
	}

	void BBoxPass::add(glm::mat4* m, BBoxPass::box_t* vcr, size_t count)
	{
		if (!m || !vcr || !count)return;
		auto nps = boxs.size();
		for (size_t i = 0; i < count; i++)
		{
			boxs.push_back(vcr[i]);
		}
		box_ms.push_back({ m,nps,count });
	}

	void BBoxPass::clear()
	{
		boxs.clear();
		box_ms.clear();
	}
}
// !vkr



std::vector<device_info_t> get_devices(void* inst)
{
	VkPhysicalDeviceProperties dp = {};
	std::vector<device_info_t> r;
	std::vector<VkExtensionProperties> dep;
	std::vector<void*> phyDevices;
	uint32_t count = 0;
	if (inst) {
		auto hr = (vkEnumeratePhysicalDevices((VkInstance)inst, &count, NULL));
		if (count > 0)
		{
			phyDevices.resize(count);
			hr = (vkEnumeratePhysicalDevices((VkInstance)inst, &count, (VkPhysicalDevice*)phyDevices.data()));
		}
		if (count)
		{
			r.reserve(count);
			for (auto p : phyDevices)
			{
				uint32_t extensionCount;
				VkResult res = vkEnumerateDeviceExtensionProperties((VkPhysicalDevice)p, nullptr, &extensionCount, NULL);
				dep.resize(extensionCount);
				res = vkEnumerateDeviceExtensionProperties((VkPhysicalDevice)p, nullptr, &extensionCount, dep.data());
				vkGetPhysicalDeviceProperties((VkPhysicalDevice)p, &dp);
				device_info_t d = {};
				strcpy(d.name, dp.deviceName);
				d.phd = p;
				r.push_back(d);
			}
		}
	}
	return r;
}

void get_queue_info(void* physicaldevice)
{
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties((VkPhysicalDevice)physicaldevice, &queue_family_count, NULL);
	assert(queue_family_count >= 1);
	std::vector<VkQueueFamilyProperties> queue_props;
	queue_props.resize(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties((VkPhysicalDevice)physicaldevice, &queue_family_count, queue_props.data());
	assert(queue_family_count >= 1);
}



struct wireframe_object0
{
	glm::mat4 m_mWorldViewProj;
	glm::vec4 m_vCenter;
	glm::vec4 m_vRadius;
	glm::vec4 m_vColor;
};

void Wireframe_pipe(vkr::Device* pDevice, VkRenderPass renderPass,
	vkr::ResourceViewHeaps* pResourceViewHeaps, vkr::DynamicBufferRing* pDynamicBufferRing, vkr::StaticBufferPool* pStaticBufferPool, VkSampleCountFlagBits sampleDescCount)
{
	VkResult res;

	VkDescriptorSetLayout      m_descriptorSetLayout;
	VkDescriptorSet           m_descriptorSet;
	VkPipelineLayout          m_pipelineLayout;
	VkPipeline pipe = {};

	// the vertex shader
	static const char* vertexShader =
		"#version 400\n"
		"#extension GL_ARB_separate_shader_objects : enable\n"
		"#extension GL_ARB_shading_language_420pack : enable\n"
		"layout (std140, binding = 0) uniform _cbPerObject\n"
		"{\n"
		"    mat4        u_mWorldViewProj;\n"
		"    vec4        u_Center;\n"
		"    vec4        u_Radius;\n"
		"    vec4        u_Color;\n"
		"} cbPerObject;\n"
		"layout(location = 0) in vec3 position; \n"
		"layout (location = 0) out vec4 outColor;\n"
		"void main() {\n"
		"   outColor = cbPerObject.u_Color;\n"
		"   gl_Position = cbPerObject.u_mWorldViewProj * (vec4(cbPerObject.u_Center.xyz + position * cbPerObject.u_Radius.xyz, 1.0f));\n"
		"}\n";

	// the pixel shader
	static const char* pixelShader =
		"#version 400\n"
		"#extension GL_ARB_separate_shader_objects : enable\n"
		"#extension GL_ARB_shading_language_420pack : enable\n"
		"layout (location = 0) in vec4 inColor;\n"
		"layout (location = 0) out vec4 outColor;\n"
		"void main() {\n"
		"   outColor = inColor;\n"
		"}";

	/////////////////////////////////////////////
	// Compile and create shaders

	vkr::DefineList attributeDefines;

	VkPipelineShaderStageCreateInfo m_vertexShader;
	res = VKCompileFromString(pDevice->m_device, vkr::SST_GLSL, VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "main", "", &attributeDefines, &m_vertexShader);
	assert(res == VK_SUCCESS);

	VkPipelineShaderStageCreateInfo m_fragmentShader;
	res = VKCompileFromString(pDevice->m_device, vkr::SST_GLSL, VK_SHADER_STAGE_FRAGMENT_BIT, pixelShader, "main", "", &attributeDefines, &m_fragmentShader);
	assert(res == VK_SUCCESS);

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { m_vertexShader, m_fragmentShader };

	/////////////////////////////////////////////
	// Create descriptor set layout

	/////////////////////////////////////////////
	// Create descriptor set 

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings(1);
	layoutBindings[0].binding = 0;
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	layoutBindings[0].descriptorCount = 1;
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	layoutBindings[0].pImmutableSamplers = NULL;

	pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindings, &m_descriptorSetLayout, &m_descriptorSet);
	pDynamicBufferRing->SetDescriptorSet(0, sizeof(wireframe_object0), m_descriptorSet);

	/////////////////////////////////////////////
	// Create the pipeline layout using the descriptoset

	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext = NULL;
	pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
	pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)1;
	pPipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;

	res = vkCreatePipelineLayout(pDevice->m_device, &pPipelineLayoutCreateInfo, NULL, &m_pipelineLayout);
	assert(res == VK_SUCCESS);

	/////////////////////////////////////////////
	// Create pipeline

	// Create the input attribute description / input layout(in DX12 jargon)
	//
	VkVertexInputBindingDescription vi_binding = {};
	vi_binding.binding = 0;
	vi_binding.stride = sizeof(float) * 3;
	vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vi_attrs[] =
	{
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
	};

	VkPipelineVertexInputStateCreateInfo vi = {};
	vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vi.pNext = NULL;
	vi.flags = 0;
	vi.vertexBindingDescriptionCount = 1;
	vi.pVertexBindingDescriptions = &vi_binding;
	vi.vertexAttributeDescriptionCount = _countof(vi_attrs);
	vi.pVertexAttributeDescriptions = vi_attrs;

	// input assembly state
	//
	VkPipelineInputAssemblyStateCreateInfo ia;
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.pNext = NULL;
	ia.flags = 0;
	ia.primitiveRestartEnable = VK_FALSE;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

	// rasterizer state
	//
	VkPipelineRasterizationStateCreateInfo rs;
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.pNext = NULL;
	rs.flags = 0;
	rs.polygonMode = VK_POLYGON_MODE_LINE;
	rs.cullMode = VK_CULL_MODE_NONE;
	rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rs.depthClampEnable = VK_FALSE;
	rs.rasterizerDiscardEnable = VK_FALSE;
	rs.depthBiasEnable = VK_FALSE;
	rs.depthBiasConstantFactor = 0;
	rs.depthBiasClamp = 0;
	rs.depthBiasSlopeFactor = 0;
	rs.lineWidth = 2.0f;

	VkPipelineColorBlendAttachmentState att_state[1];
	att_state[0].colorWriteMask = vkr::allBits;
	att_state[0].blendEnable = VK_TRUE;
	att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
	att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
	att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

	// Color blend state
	//
	VkPipelineColorBlendStateCreateInfo cb;
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	cb.flags = 0;
	cb.pNext = NULL;
	cb.attachmentCount = 1;
	cb.pAttachments = att_state;
	cb.logicOpEnable = VK_FALSE;
	cb.logicOp = VK_LOGIC_OP_NO_OP;
	cb.blendConstants[0] = 1.0f;
	cb.blendConstants[1] = 1.0f;
	cb.blendConstants[2] = 1.0f;
	cb.blendConstants[3] = 1.0f;

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = NULL;
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();

	// view port state
	//
	VkPipelineViewportStateCreateInfo vp = {};
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.pNext = NULL;
	vp.flags = 0;
	vp.viewportCount = 1;
	vp.scissorCount = 1;
	vp.pScissors = NULL;
	vp.pViewports = NULL;

	// depth stencil state
	//
	VkPipelineDepthStencilStateCreateInfo ds;
	ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	ds.pNext = NULL;
	ds.flags = 0;
	ds.depthTestEnable = true;
	ds.depthWriteEnable = true;
	ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	ds.depthBoundsTestEnable = VK_FALSE;
	ds.stencilTestEnable = VK_FALSE;
	ds.back.failOp = VK_STENCIL_OP_KEEP;
	ds.back.passOp = VK_STENCIL_OP_KEEP;
	ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
	ds.back.compareMask = 0;
	ds.back.reference = 0;
	ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
	ds.back.writeMask = 0;
	ds.minDepthBounds = 0;
	ds.maxDepthBounds = 0;
	ds.stencilTestEnable = VK_FALSE;
	ds.front = ds.back;

	// multi sample state
	//
	VkPipelineMultisampleStateCreateInfo ms;
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.pNext = NULL;
	ms.flags = 0;
	ms.pSampleMask = NULL;
	ms.rasterizationSamples = sampleDescCount;
	ms.sampleShadingEnable = VK_FALSE;
	ms.alphaToCoverageEnable = VK_FALSE;
	ms.alphaToOneEnable = VK_FALSE;
	ms.minSampleShading = 0.0;

	// create pipeline 

	VkGraphicsPipelineCreateInfo pipeline = {};
	pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline.pNext = NULL;
	pipeline.layout = m_pipelineLayout;
	pipeline.basePipelineHandle = VK_NULL_HANDLE;
	pipeline.basePipelineIndex = 0;
	pipeline.flags = 0;
	pipeline.pVertexInputState = &vi;
	pipeline.pInputAssemblyState = &ia;
	pipeline.pRasterizationState = &rs;
	pipeline.pColorBlendState = &cb;
	pipeline.pTessellationState = NULL;
	pipeline.pMultisampleState = &ms;
	pipeline.pDynamicState = &dynamicState;
	pipeline.pViewportState = &vp;
	pipeline.pDepthStencilState = &ds;
	pipeline.pStages = shaderStages.data();
	pipeline.stageCount = (uint32_t)shaderStages.size();
	pipeline.renderPass = renderPass;
	pipeline.subpass = 0;

	res = vkCreateGraphicsPipelines(pDevice->m_device, pDevice->GetPipelineCache(), 1, &pipeline, NULL, &pipe);
	assert(res == VK_SUCCESS);
	vkr::SetResourceName(pDevice->m_device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pipe, "Wireframe P");
}
uint32_t gray_float_to_rgba(float gray)
{
	// 1. 边界检查与截断
	gray = (gray < 0.0f) ? 0.0f : (gray > 1.0f) ? 1.0f : gray;

	// 2. 转换为8位整数（四舍五入）
	uint8_t gray_int = (uint8_t)(gray * 255.0f + 0.5f);

	// 3. 组合为RGBA整数
	return ((uint32_t)gray_int << 24) |
		((uint32_t)gray_int << 16) |
		((uint32_t)gray_int << 8) |
		gray_int;
}
namespace vkr
{
	/*
	场景编辑管理
	*/
	scene_edit::scene_edit()
	{
	}
	scene_edit::~scene_edit()
	{
		// 清理光源
		for (auto& light : lights)
		{
			delete[] light;
		}
		lights.clear(); last_idx = 0;
	}
	void scene_edit::default_light()
	{
		// 创建默认光源
		light_t* light = add_directional_light(50.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 0);
		light->_rotation = glm::angleAxis(glm::radians(-45.0f), glm::vec3(1, 0, 0));
	}
	light_t* scene_edit::add_directional_light(float intensity, const glm::vec4& color, int shadow_width)
	{
		light_t l = {};
		l._type = light_t::LIGHT_DIRECTIONAL;
		l._intensity = glm::max(intensity, 0.0f);
		l._color = color;
		l._shadowResolution = shadow_width;
		l._bias = 0.0007;
		l._rotation = glm::quat(1, 0, 0, 0);
		l._position = glm::vec3(0, 0, 0);
		auto p = add_light(); *p = l;
		return p;
	}
	light_t* scene_edit::add_spot_light(float intensity, const glm::vec4& color, float range, float cone_angle, float cone_mix, int shadow_width)
	{
		light_t l = {};
		l._type = light_t::LIGHT_SPOTLIGHT;
		l._intensity = glm::max(intensity, 0.0f);
		l._color = color;
		l._range = range;
		l._cone_angle = cone_angle;
		l._cone_mix = cone_mix;
		l._shadowResolution = shadow_width;
		l._bias = 0.0007;
		l._rotation = glm::quat(1, 0, 0, 0);
		l._position = glm::vec3(0, 0, 0);
		auto p = add_light(); *p = l;
		return p;
	}
	light_t* scene_edit::add_point_light(float intensity, const glm::vec4& color, float range)
	{
		light_t l = {};
		l._type = light_t::LIGHT_POINTLIGHT;
		l._intensity = glm::max(intensity, 0.0f);
		l._color = color;
		l._range = range;
		l._shadowResolution = 0; // 点光源不需要阴影
		l._position = glm::vec3(0, 0, 0);
		auto p = add_light(); *p = l;
		return p;
	}
	light_t* scene_edit::add_light()
	{
		bool need_new = lights.empty();
		if (lights.size() > 0)
		{
			auto p = *lights.rbegin();
			if (p->_ac < 1)p->_ac = max_count;
			if (last_idx == p->_ac)
			{
				need_new = true;
			}
		}
		if (need_new)
		{
			auto p1 = new light_t[max_count];
			p1->_ac = max_count;
			lights.push_back(p1);
			last_idx = 0;
		}
		auto p = *lights.rbegin();
		p += last_idx;
		last_idx++;
		*p = {};
		return p;
	}
	// mesh shader测试
	const char* ms_frag = R"(#version 450 
layout (location = 0) in VertexInput {
  vec4 color;
} vertexInput;
layout(location = 0) out vec4 outFragColor;
void main()
{
	outFragColor = vertexInput.color;
})";

	const char* ms_mesh = R"(
#version 450
#extension GL_EXT_mesh_shader : require
layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
} ubo;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = 3, max_primitives = 1) out;
layout(location = 0) out VertexOutput
{
	vec4 color;
} vertexOutput[];
const vec4[3] positions = {
	vec4( 0.0, -1.0, 0.0, 1.0),
	vec4(-1.0,  1.0, 0.0, 1.0),
	vec4( 1.0,  1.0, 0.0, 1.0)};
const vec4[3] colors = {
	vec4(0.0, 1.0, 0.0, 1.0),
	vec4(0.0, 0.0, 1.0, 1.0),
	vec4(1.0, 0.0, 0.0, 1.0)
};
void main()
{
	uint iid = gl_LocalInvocationID.x;
	vec4 offset = vec4(0.0, 0.0, gl_GlobalInvocationID.x, 0.0);
	SetMeshOutputsEXT(3, 1);
	mat4 mvp = ubo.projection * ubo.view * ubo.model;
	gl_MeshVerticesEXT[0].gl_Position = mvp * (positions[0] + offset);
	gl_MeshVerticesEXT[1].gl_Position = mvp * (positions[1] + offset);
	gl_MeshVerticesEXT[2].gl_Position = mvp * (positions[2] + offset);
	vertexOutput[0].color = colors[0];
	vertexOutput[1].color = colors[1];
	vertexOutput[2].color = colors[2];
	gl_PrimitiveTriangleIndicesEXT[gl_LocalInvocationIndex] =  uvec3(0, 1, 2);
})";

	const char* ms_task = R"(
#version 450
#extension GL_EXT_mesh_shader : require
void main()
{
	EmitMeshTasksEXT(3, 1, 1);
})";



	void* new_ms_pipe(void* dev, void* rpass)
	{
		VkDevice device = (VkDevice)dev;  VkRenderPass renderPass = (VkRenderPass)rpass;
		struct UniformData {
			glm::mat4 projection;
			glm::mat4 model;
			glm::mat4 view;
		} uniformData;
		VkPipelineLayout pipelineLayout = { VK_NULL_HANDLE };
		VkDescriptorSet descriptorSet = { VK_NULL_HANDLE };
		VkDescriptorSetLayout descriptorSetLayout = { VK_NULL_HANDLE };
		VkDescriptorPool descriptorPool = { VK_NULL_HANDLE };
		VkDescriptorBufferInfo descriptor = {};
		// Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = initializers::descriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), 1);
		(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT, 0),
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorSetLayout));

		// Set
		VkDescriptorSetAllocateInfo allocInfo = initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
		std::vector<VkWriteDescriptorSet> modelWriteDescriptorSets = {
			initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &descriptor),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(modelWriteDescriptorSets.size()), modelWriteDescriptorSets.data(), 0, nullptr);

		DefineList attributeDefines = {};
		VkPipelineShaderStageCreateInfo msmShader;
		auto res = VKCompileFromString(device, SST_GLSL, VK_SHADER_STAGE_MESH_BIT_EXT, ms_mesh, "main", "", &attributeDefines, &msmShader);
		assert(res == VK_SUCCESS);

		VkPipelineShaderStageCreateInfo mfragmentShader;
		res = VKCompileFromString(device, SST_GLSL, VK_SHADER_STAGE_FRAGMENT_BIT, ms_frag, "main", "", &attributeDefines, &mfragmentShader);
		assert(res == VK_SUCCESS);

		VkPipelineShaderStageCreateInfo taskShader;
		res = VKCompileFromString(device, SST_GLSL, VK_SHADER_STAGE_TASK_BIT_EXT, ms_task, "main", "", &attributeDefines, &taskShader);
		assert(res == VK_SUCCESS);

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { msmShader,taskShader, mfragmentShader };

		// Layout
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		res = (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

		// Pipeline
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = {};
		get_blend(false, blendAttachmentState);
		VkPipelineColorBlendStateCreateInfo colorBlendState = initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

		VkGraphicsPipelineCreateInfo pipelineCI = initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
		VkPipeline pipeline = nullptr;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.renderPass = 0;
		// Not using a vertex shader, mesh shading doesn't require vertex input state
		pipelineCI.pInputAssemblyState = nullptr;
		pipelineCI.pVertexInputState = nullptr;
		res = vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCI, nullptr, &pipeline);
		assert(res == VK_SUCCESS);
		return pipeline;
	}


}
// !vkr
	// 半边
#if 1

namespace std {
	template<typename A, typename B>
	struct hash< std::pair<A, B>> {
		size_t operator()(std::pair<A, B> const& key) const {
			static const std::hash<A> ha;
			static const std::hash<B> hb;
			size_t hf = ha(key.first);
			size_t hs = hb(key.second);
			//NOTE: if this were C++20 we could use std::rotr or std::rotl
			return hf ^ (hs << (sizeof(size_t) * 4)) ^ (hs >> (sizeof(size_t) * 4));
		}
	};
	template <>
	struct hash<glm::uvec2> {
		size_t operator()(const glm::uvec2& key) const noexcept {
			size_t seed = 0;
			// 组合 x 和 y 的哈希值
			seed ^= hash<uint32_t>()(key.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= hash<uint32_t>()(key.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}
namespace vkr {
	MeshHalfEdge* from_indexed_faces(std::vector< glm::vec3 > const& vertices_,
		std::vector< std::vector< Index > > const& faces_,
		std::vector< std::vector< Index > > const& corner_normal_idxs,
		std::vector< std::vector< Index > > const& corner_uv_idxs,
		std::vector<glm::vec3> const& corner_normals_,
		std::vector<glm::vec2> const& corner_uvs_)
	{
		auto rp = new MeshHalfEdge();
		MeshHalfEdge& mesh = *rp;
		//for quick lookup of vertices by index
		mesh.vertices.reserve(vertices_.size());
		for (auto const& v : vertices_) {
			Vertex vx = {};
			vx.pos = v;
			mesh.vertices.emplace_back(vx);
		}

		// std::pair< Index, Index >,HalfEdge*
		std::unordered_map<glm::uvec2, size_t > halfedges; //for quick lookup of halfedges by from/to vertex index

		uint32_t num_faces = static_cast<uint32_t>(faces_.size());
		const bool add_corner_normals = corner_normal_idxs.size() >= num_faces;
		const bool add_corner_uvs = corner_uv_idxs.size() >= num_faces;
		//helper to add a face (and, later, boundary):
		auto add_loop = [&](std::vector< Index > const& loop, bool boundary,
			std::vector< Index> const& n_loop = std::vector<Index>{},
			std::vector< Index> const& uv_loop = std::vector<Index>{}) {
				assert(loop.size() >= 3);

				for (uint32_t j = 0; j < loop.size(); ++j) {
					//omit adding a face with vertices of the same index, otherwise crashes on cylinder/cone
					if (loop[j] == loop[(j + 1) % loop.size()]) { return; }
				}

				Face* face = mesh.emplace_face(); face->boundary = boundary;
				HalfEdge* prev = &(*mesh.halfedges.end()); //keep track of previous edge around face to set next pointer
				for (uint32_t i = 0; i < loop.size(); ++i) {
					Index a = loop[i];
					Index b = loop[(i + 1) % loop.size()];
					assert(a != b);

					HalfEdge* halfedge = mesh.emplace_halfedge();
					if (i == 0) face->halfedge = halfedge->id; //store first edge as face's halfedge pointer
					halfedge->origin = a;// mesh.vertices[a];

					//if first to mention vertex, set vertex's halfedge pointer:
					if (mesh.vertices[a].leavingEdge > mesh.halfedges.size()) {
						assert(!boundary); //boundary faces should never be mentioning novel vertices, since they are created second
						mesh.vertices[a].leavingEdge = halfedge->id;
					}
					halfedge->face = face->id;
					size_t hid = halfedge->id;
					auto k = glm::uvec2(a, b);
					auto inserted = halfedges.emplace(k, hid);
					assert(inserted.second); //if edge mentioned more than once in the same direction, not an oriented, manifold mesh

					auto twin = halfedges.find(glm::uvec2(b, a));
					if (twin == halfedges.end()) {
						assert(!boundary); //boundary faces exist only to complete edges so should *always* match
						//not twinned yet -- create an edge just for this halfedge:
						Edge* edge = mesh.emplace_edge();
						halfedge->edge = edge->id;
						edge->halfedge = halfedge->id;
					}
					else {
						//found a twin -- connect twin pointers and reference its edge:
						auto& ts = mesh.halfedges[twin->second];
						assert(ts.twin > mesh.halfedges.size());
						halfedge->twin = twin->second;
						halfedge->edge = ts.edge;
						ts.twin = halfedge->id;
					}

					if (i != 0) prev->next = halfedge->id; //set previous halfedge's next pointer
					prev = halfedge;

					if (add_corner_normals && i < n_loop.size()) halfedge->corner_normal = corner_normals_[n_loop[i]];
					if (add_corner_uvs && i < uv_loop.size()) halfedge->corner_uv = corner_uvs_[uv_loop[i]];
				}

				prev->next = face->halfedge; //set next pointer for last halfedge to first edge
			};

		//add all faces:
		for (uint32_t i = 0; i < num_faces; i++) {
			if (add_corner_normals && add_corner_uvs) add_loop(faces_[i], false, corner_normal_idxs[i], corner_uv_idxs[i]);
			else if (add_corner_normals) add_loop(faces_[i], false, corner_normal_idxs[i]);
			else add_loop(faces_[i], false);
		}

		// All halfedges created so far have valid next pointers, but some may be missing twins because they are at a boundary.

		std::map< Index, Index > next_on_boundary;

		//first, look for all un-twinned halfedges to figure out the shape of the boundary:
		for (auto const& [from_to, halfedge] : halfedges) {
			auto& it = mesh.halfedges[halfedge];
			if (it.twin > mesh.halfedges.size()) {
				auto ret = next_on_boundary.emplace(from_to.y, from_to.x); //twin needed on the boundary
				assert(ret.second); //every boundary vertex should have a unique successor because the boundary is "half-disc-like"
			}
		}

		//now pull out boundary loops until all edges are exhausted:
		while (!next_on_boundary.empty()) {
			std::vector< Index > loop;
			loop.emplace_back(next_on_boundary.begin()->first);

			do {
				//look up next hop on the boundary:
				auto next = next_on_boundary.find(loop.back());
				//should never be dead ends on boundary:
				assert(next != next_on_boundary.end());

				//add next hop to loop:
				loop.emplace_back(next->second);
				//...and remove from nexts structure:
				next_on_boundary.erase(next);
			} while (loop[0] != loop.back());

			loop.pop_back(); //remove duplicated first/last element

			assert(loop.size() >= 3); //all faces must be non-degenerate

			//add boundary loop:
			add_loop(loop, true);
		}

		//with boundary faces created, mesh should be ready to go with all edges nicely twinned.

		//PARANOIA: this should never happen:
		//auto error = mesh.validate();
		//if (error) {
		//	std::cerr << "MeshHalfEdge from_indexed_faces failed validation: " << error->second << std::endl;
		//	assert(0);
		//}

		return rp;
	}
	void MeshHalfEdge::set_corner_normals(float threshold) {
#if 0
		//first, figure out which edges to consider sharp for this operation:
		std::unordered_set< Edge const* > sharp_edges;
		sharp_edges.reserve(edges.size());

		//all edges between boundary and non-boundary get marked sharp regardless of mode:
		for (auto const& edge : edges) {
			auto& ef = halfedges[edge.halfedge];
			auto& tf = halfedges[ef.twin];
			if (faces[ef.face].boundary != faces[tf.face].boundary) {
				sharp_edges.emplace(&edge);
			}
		}

		if (threshold >= 180.0f) {
			//"smooth mode" -- all other edges are considered smooth
		}
		else {
			//"flat mode" / "auto mode" -- any edges which are marked sharp or have face angle <= threshold get marked sharp:
			float cos_threshold = std::cos(glm::radians(std::clamp(threshold, 0.0f, 180.0f)));
			if (threshold <= 0.0f) cos_threshold = 2.0f; //make sure everything is sharp
			for (auto const& edge : edges) {
				//get adjacent halfedges:
				HalfEdge* h1 = &halfedges[edge.halfedge];
				HalfEdge* h2 = &halfedges[h1->twin];
				if (faces[h1->face].boundary || faces[h2->face].boundary) {
					//don't care about edges boundary-boundary, and inside-boundary already marked.
					//thus: nothing to do here
				}
				else if (edge.sharp) {
					//flagged as sharp, so mark it sharp:
					sharp_edges.emplace(&edge);
				}
				else {
					//inside-inside edge, non-marked, check angle:
					glm::vec3 n1 = face_normal(h1->face);
					glm::vec3 n2 = face_normal(h2->face);
					float cos = dot(n1, n2);
					if (cos <= cos_threshold) {
						//treat as sharp:
						sharp_edges.emplace(&edge);
					}
				}
			}
		}

		//clear current corner normals:
		for (auto h = halfedges.begin(); h != halfedges.end(); ++h) {
			h->corner_normal = glm::vec3{ 0.0f, 0.0f, 0.0f };
		}

		//now circulate all vertices to set normals:
		for (auto const& v : vertices) {
			//get halfedge leaving this vertex:
			HalfEdge* begin = &halfedges[v.leavingEdge];
			assert(begin->origin == v.leavingEdge);

			//circulate begin until it is at a sharp edge (thus, the next corner starts a smoothing group):
			do {
				if (sharp_edges.count(&edges[begin->edge])) break;
				begin = &halfedges[halfedges[begin->twin].next];
			} while (begin->id != v.leavingEdge); //could be all one big happy smoothing group

			//store all corners around the vertex:
			struct Corner {
				HalfEdge* in; //halfedge pointing to v
				HalfEdge* out; //halfedge pointing away from v
				glm::vec3 weighted_normal; //face normal at corner, weighted... somehow (see below)
			};

			std::vector< std::vector< Corner > > groups;
			HalfEdge* h = begin;
			do {
				//start a new smoothing group on sharp edges (or at the very first edge):
				if (h == begin || sharp_edges.count(&*h->edge)) {
					groups.emplace_back();
				}
				//add corner after h to current smoothing group:
				Corner corner;
				corner.in = h->twin;
				corner.out = h->twin->next;
				assert(corner.in->face == corner.out->face); //PARANOIA
				{ //compute some sort of weighted normal:
					assert(&*corner.in->vertex != &v);
					assert(&*corner.in->twin->vertex == &v);
					glm::vec3 from = corner.in->vertex->pos - v.pos;

					assert(&*corner.out->vertex == &v);
					assert(&*corner.out->twin->vertex != &v);
					glm::vec3 to = corner.out->twin->vertex->pos - v.pos;
					/*
					//basic area weighting (weird with non-flat faces and reflex vertices):
					corner.weighted_normal = cross(to - v.pos, from - v.pos);
					*/
					/*//sort sort of angle weighting thing -- this never works as well as one would hope:
					//...still needs work for reflex angles also
					float angle = std::atan2(cross(from,to).norm(), dot(from, to));
					corner.weighted_normal = angle * corner.in->face->normal();
					*/
					//some other sort of slightly fancy area weighting:
					corner.weighted_normal = cross(to - v.pos, from - v.pos).norm() * corner.in->face->normal();
				}
				groups.back().emplace_back(corner);

				//advance h:
				h = h->twin->next;
			} while (h != begin);

			//compute weighted normals per-corner:
			for (auto const& group : groups) {
				assert(!group.empty());
				if (group[0].in->face->boundary) {
					//boundary group.
					//PARANOIA:
					for (auto const& corner : group) {
						assert(corner.in->face->boundary);
					}
					//no need for normals on boundary corners
					continue;
				}
				//compute weighted normal:
				glm::vec3 sum = glm::vec3{ 0.0f, 0.0f, 0.0f };
				for (auto const& corner : group) {
					sum += corner.weighted_normal;
				}
				//normalize:
				sum = sum.unit();
				//assign to all corners in group:
				for (auto const& corner : group) {
					assert(&*corner.out->vertex == &v);
					corner.out->corner_normal = sum;
				}
			}
		}
#endif
		//normals computed!
	}

	/// Take minimum of each component
	inline glm::vec2 hmin(glm::vec2 l, glm::vec2 r) {
		return glm::vec2(std::min(l.x, r.x), std::min(l.y, r.y));
	}
	/// Take maximum of each component
	inline glm::vec2 hmax(glm::vec2 l, glm::vec2 r) {
		return glm::vec2(std::max(l.x, r.x), std::max(l.y, r.y));
	}
	/*
	 * set_corner_uvs_per_face: set uv coordinates to map texture per-face
	 */
	void MeshHalfEdge::set_corner_uvs_per_face() {
#if 0
		//clear existing UVs:
		for (auto& halfedge : halfedges) {
			halfedge.corner_uv = glm::vec2(0.0f, 0.0f);
		}

		//set UVs per-face:
		for (auto const& face : faces) {
			if (face.boundary) continue;

			//come up with a plane perpendicular-ish to the face:
			glm::vec3 n = face.normal();
			glm::vec3 p1;
			if (std::abs(n.x) < std::abs(n.y) && std::abs(n.x) < std::abs(n.z)) {
				p1 = glm::vec3(1.0f, 0.0f, 0.0f);
			}
			else if (std::abs(n.y) < std::abs(n.z)) {
				p1 = glm::vec3(0.0f, 1.0f, 0.0f);
			}
			else {
				p1 = glm::vec3(0.0f, 0.0f, 1.0f);
			}
			p1 = (p1 - dot(p1, n) * n).unit();
			glm::vec3 p2 = cross(n, p1);

			//find bounds of face on plane:
			glm::vec2 min0 = glm::vec2(std::numeric_limits< float >::infinity(), std::numeric_limits< float >::infinity());
			glm::vec2 max0 = glm::vec2(-std::numeric_limits< float >::infinity(), -std::numeric_limits< float >::infinity());
			HalfEdge* v = face.halfedge;
			do {
				glm::vec2 pt = glm::vec2(dot(p1, v->vertex->pos), dot(p2, v->vertex->pos));
				min0 = hmin(min0, pt);
				max0 = hmax(max0, pt);
				v = v->next;
			} while (v != face.halfedge);

			//set corner uvs based on position within bounds:
			do {
				glm::vec2 pt = glm::vec2(dot(p1, v->vertex->pos), dot(p2, v->vertex->pos));
				v->corner_uv = glm::vec2(
					(pt.x - min0.x) / (max0.x - min0.x),
					(pt.y - min0.y) / (max0.y - min0.y)
				);
				v = v->next;
			} while (v != face.halfedge);
		}
#endif
	}

	/*
	 * set_corner_uvs_project: set uv coordinates to map texture by projection to a plane
	 */
	void MeshHalfEdge::set_corner_uvs_project(glm::vec3 origin, glm::vec3 u_axis, glm::vec3 v_axis) {

#if 0
		u_axis /= u_axis.norm_squared();
		v_axis /= v_axis.norm_squared();

		for (auto& halfedge : halfedges) {
			if (halfedge.face->boundary) {
				halfedge.corner_uv = glm::vec2(0.0f, 0.0f);
			}
			else {
				halfedge.corner_uv = glm::vec2(
					dot(halfedge.vertex->pos - origin, u_axis),
					dot(halfedge.vertex->pos - origin, v_axis)
				);
			}
		}
#endif
	}

	HalfEdge* MeshHalfEdge::emplace_halfedge()
	{
		HalfEdge he = {};
		return (HalfEdge*)&halfedges.emplace_back(he);
	}

	Edge* MeshHalfEdge::emplace_edge()
	{
		Edge e = {};
		return (Edge*)&edges.emplace_back(e);
	}

	Face* MeshHalfEdge::emplace_face()
	{
		Face f = {};
		return (Face*)&faces.emplace_back(f);
	}


	MeshHalfEdge* new_cube(float r) {
		MeshHalfEdge* mesh = from_indexed_faces(
			std::vector< glm::vec3 >{
			glm::vec3{ -r, -r, -r }, glm::vec3{ r, -r, -r },
				glm::vec3{ -r,  r, -r }, glm::vec3{ r,  r, -r },
				glm::vec3{ -r, -r,  r }, glm::vec3{ r, -r,  r },
				glm::vec3{ -r,  r,  r }, glm::vec3{ r,  r,  r }
		},
			std::vector< std::vector< Index > >{
				{0, 2, 3, 1}, { 4,5,7,6 },
				{ 0,1,5,4 }, { 1,3,7,5 },
				{ 3,2,6,7 }, { 2,0,4,6 },
		}
		);

		for (auto& edge : mesh->edges) {
			edge.sharp = true;
		}
		mesh->set_corner_normals(0.0f);
		mesh->set_corner_uvs_per_face();

		return mesh;
	}
#endif // 1半边

}
//!vkr


uint64_t vkr_get_ticks() {
	auto now = std::chrono::high_resolution_clock::now();
	// 转换为毫秒级时间戳
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	return ms;
}

void vkrender_test0()
{
	auto inst = new_instance();
	//auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
	std::vector<device_info_t> devs = get_devices(inst);  // 获取设备名称列表
	if (devs.empty())return;
	//vkdg_cx* vkd = new_vkdg(sdldev.inst, sdldev.phy, sdldev.vkdev);	// 从SDL3获取设备创建vk渲染器
	vkdg_cx* vkd = new_vkdg(inst, devs[0].phd, 0, "ShaderLibVK", "cache/shadervk");			// 创建vk渲染器
	vkd->add_gltf(R"(E:\model\sharp2.glb)", { 0,0,0 }, 1.0);// 添加一个地板
	vkd->resize(1024, 800);				// 设置fbo缓冲区大小
	auto vki = vkd->get_vkimage(0);	// 获取第一个fbo纹理弄到窗口显示
	//auto texok = form0->add_vkimage(vki.size, vki.vkimage, { 20,36 }, 0);// 创建SDL的rgba纹理
	auto dstate = vkd->_state;	// 渲染状态属性
	{
		mouse_state_t mio = {};
		int64_t prev_time = 0;
		// mio = *form0->io; // 有form就不需要计算DeltaTime和prev_time
		// 独立线程或主线程执行渲染
		while (1) {
			uint64_t curr_time = vkr_get_ticks();
			if (prev_time > 0)
			{
				double delta = (float)(curr_time - prev_time) / 1000.0f;
				mio.DeltaTime = delta;
			}
			vkd->update(&mio);		// 更新事件
			vkd->on_render();		// 执行渲染
			// todo 提交到窗口渲染
			Sleep(1);
			prev_time = curr_time;
		}
	}
	free_vkdg(vkd);				// 释放渲染器
	free_instance(inst);		// 释放实例
}



