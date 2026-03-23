/*
--------------------------------------------------------------------------------------------------------------
vk渲染器

创建日期：2026-02-07
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
#ifdef min
#undef min
#undef max
#endif // min 
#include <zlib.h>
#include <queue>
#include <unordered_set>
// 本地头文件
#include <event.h>
#include <print_time.h>
#include <mapView.h>
#include <stb_image.h>


#include "vkrh.h"
#include "gpu_vk.h"
#define CACHE_ENABLE
//#define CACHE_LOG 

/*
pbrMetallicRoughness基础颜色、金属度、粗糙度
KHR_materials_emissive_strength
vec3 emissiveFactor
KHR_materials_ior
KHR_materials_pbrSpecularGlossiness
KHR_materials_specular
KHR_materials_sheen
KHR_materials_anisotropy
KHR_materials_transmission
KHR_materials_dispersion
KHR_materials_iridescence
KHR_materials_clearcoat
*/

#include "gpu_vk_in.h"


// 新渲染器实现
namespace vkg {

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

	Sync::Sync() {}
	Sync::~Sync() {}
	int Sync::Inc()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_count++;
		return m_count;
	}

	int Sync::Dec()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_count--;
		if (m_count == 0)
			condition.notify_all();
		return m_count;
	}

	int Sync::Get()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_count;
	}

	void Sync::Reset()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_count = 0;
		condition.notify_all();
	}

	void Sync::Wait()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		while (m_count != 0)
			condition.wait(lock);
	}

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

	void  InstanceProperties::GetExtensionNamesAndConfigs(std::vector<const char*>* pInstance_layer_names, std::vector<const char*>* pInstance_extension_names, bool all)
	{
		if (all)
		{
			for (auto& it : m_instanceLayerProperties)
				pInstance_layer_names->push_back(it.layerName);
			for (auto& it : m_instanceExtensionProperties) {
				pInstance_extension_names->push_back(it.extensionName);
			}
		}
		else {
			for (auto& name : m_instance_layer_names)
				pInstance_layer_names->push_back(name);
			for (auto& name : m_instance_extension_names)
				pInstance_extension_names->push_back(name);
		}
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

	void* new_instance(const char* pApplicationName, const char* pEngineName)
	{
		vkg::InstanceProperties ip;
		ip.Init();
		bool cpuvalid = false;
		bool gpuvalid = false;
#ifdef _DEBUG
		cpuvalid = 1;
		gpuvalid = 1;
#endif // _DEBUG
		vkg::SetEssentialInstanceExtensions(cpuvalid, gpuvalid, &ip);
		auto apiVersion3 = VK_API_VERSION_1_3;
		auto apiVersion4 = VK_API_VERSION_1_4;
		VkInstanceCreateInfo inst_info = {};
		std::vector<const char*> instance_layer_names;
		std::vector<const char*> instance_extension_names;
		ip.GetExtensionNamesAndConfigs(&instance_layer_names, &instance_extension_names, 0);
		VkInstance instance = {};
		VkApplicationInfo app_info = {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pNext = NULL;
		app_info.pApplicationName = pApplicationName ? pApplicationName : "vkapp";
		app_info.applicationVersion = 1;
		app_info.pEngineName = pEngineName ? pEngineName : "pnguo";
		app_info.engineVersion = 1;
		app_info.apiVersion = apiVersion4;
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

	// 获取设备分数
	uint32_t GetScore(void* phd)
	{
		VkPhysicalDevice physicalDevice = (VkPhysicalDevice)phd;
		uint32_t score = 0;
		VkPhysicalDeviceProperties deviceProperties = {};
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		//score += deviceProperties.limits.maxImageDimension1D;
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
		return score;
	}
	// 选择物理设备
	VkPhysicalDevice SelectPhysicalDevice(std::vector<device_info_t>& physicalDevices)
	{
		assert(physicalDevices.size() > 0 && "No GPU found");
		std::multimap<uint32_t, VkPhysicalDevice> ratings;
		for (auto& it : physicalDevices) {
			ratings.insert(std::make_pair(GetScore(it.phd), (VkPhysicalDevice)it.phd));
		}
		return ratings.rbegin()->second;
	}

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
	void get_qfp(VkPhysicalDevice physicalDevice, std::vector<VkQueueFamilyProperties>& queue_props)
	{
		uint32_t queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_count, NULL);
		assert(queue_family_count >= 1);
		queue_props.resize(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_count, queue_props.data());
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

	void free_devinfo(devinfo_x* d)
	{
		if (!d)return;
		if (d->_surface)
			vkDestroySurfaceKHR(d->_instance, d->_surface, 0);
		d->_surface = 0;
#ifdef USE_VMA
		if (d->_hAllocator)
		{
			vmaDestroyAllocator(d->_hAllocator);
			d->_hAllocator = NULL;
		}
#endif

		if (d->is_newdevice && d->_device)
			vkDestroyDevice(d->_device, 0);
	}

	bool f_ExtDebugUtilsCheckInstanceExtensions(InstanceProperties* pDP)
	{
		s_bCanUseDebugUtils = pDP->AddInstanceExtensionName("VK_EXT_debug_utils");
		return s_bCanUseDebugUtils;
	}

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

	void Device_CreateEx(VkInstance vulkanInstance, VkPhysicalDevice physicalDevice, VkDevice dev, void* pw, DeviceProperties* pDp, devinfo_x* px)
	{
		VkResult res;
		// Get queue/memory/device properties 
		std::vector<VkQueueFamilyProperties> queue_props;
		get_qfp(physicalDevice, queue_props);
		uint32_t queue_family_count = queue_props.size();
		assert(queue_family_count >= 1);
		px->_physicaldevice = physicalDevice;
		px->_device = dev;
		px->_instance = vulkanInstance;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &px->_memoryProperties);
		vkGetPhysicalDeviceProperties(physicalDevice, &px->_deviceProperties);

		// Get subgroup properties to check if subgroup operations are supported 
		px->_subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
		px->_subgroupProperties.pNext = NULL;

		px->_deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		px->_deviceProperties2.pNext = &px->_subgroupProperties;

		vkGetPhysicalDeviceProperties2(physicalDevice, &px->_deviceProperties2);

		if (pw) {

#if defined(_WIN32)
			// Crate a Win32 Surface
			//
			VkWin32SurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.pNext = NULL;
			createInfo.hinstance = NULL;
			createInfo.hwnd = (HWND)pw;
			res = vkCreateWin32SurfaceKHR(px->_instance, &createInfo, NULL, &px->_surface);
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
			if (physicalDevice && px->_surface)
				get_surface_formats(physicalDevice, px->_surface, px->_surfaceFormats);
		}
		uint32_t graphics_queue_family_index = UINT32_MAX;
		uint32_t present_queue_family_index = UINT32_MAX;
		if (px->_surface)
		{
			for (uint32_t i = 0; i < queue_family_count; ++i)
			{
				if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
				{
					if (graphics_queue_family_index == UINT32_MAX) graphics_queue_family_index = i;

					VkBool32 supportsPresent;
					vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, px->_surface, &supportsPresent);
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
					vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, px->_surface, &supportsPresent);
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

		// prepare existing extensions names into a buffer for vkCreateDevice
		std::vector<const char*> extension_names;
		pDp->GetExtensionNamesAndConfigs(&extension_names);

		// Create device 
		std::vector<VkDeviceQueueCreateInfo> device_queue_create_infos;
		std::vector<float> queue_priorities;
		int qcount = 1;
#if 1
		size_t qc = 0;
		for (size_t i = 0; i < queue_family_count; i++)
		{
			auto& it = queue_props[i];
			qc += it.queueCount;
		}
		device_queue_create_infos.resize(queue_family_count);
		queue_priorities.resize(qc, 0.0f);
		auto qp = queue_priorities.data();
		for (size_t i = 0; i < queue_family_count; i++)
		{
			auto& it = queue_props[i];
			auto& qf = device_queue_create_infos[i];
			qf.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			qf.pNext = NULL;
			qf.queueCount = it.queueCount;
			qf.pQueuePriorities = qp;
			qp += it.queueCount;
			qf.queueFamilyIndex = i;
		}
		VkDeviceQueueCreateInfo* queue_info = device_queue_create_infos.data();
		qcount = queue_family_count;
#else
		float queue_priorities[1] = { 0.0 };
		VkDeviceQueueCreateInfo queue_info[2] = {};
		queue_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info[1].pNext = NULL;
		queue_info[1].queueCount = 1;
		queue_info[1].pQueuePriorities = queue_priorities;
		queue_info[1].queueFamilyIndex = compute_queue_family_index;
		if (compute_queue_family_index != graphics_queue_family_index)
			qcount++;
#endif
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
		bool descriptor_heap = pDp->IsExtensionPresent(VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME);
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
		vkGetPhysicalDeviceFeatures2(physicalDevice, &features);

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
			px->_device = dev;
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
			res = vkCreateDevice(physicalDevice, &device_info, NULL, &px->_device);
			assert(res == VK_SUCCESS);
			if (px->_device)
				px->is_newdevice = true;
		}
		if (!px->_device)return;
		//if (dr)
		//{
		//	_vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(px->_device, "vkCmdBeginRenderingKHR");
		//	_vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(px->_device, "vkCmdEndRenderingKHR");
		//	_vkCmdSetFrontFace = (PFN_vkCmdSetFrontFace)vkGetDeviceProcAddr(px->_device, "vkCmdSetFrontFace");
		//	//if (!_vkCmdSetFrontFace)
		//	//	_vkCmdSetFrontFace = (PFN_vkCmdSetFrontFaceEXT)vkGetDeviceProcAddr(m_device, "vkCmdSetFrontFaceEXT");
		//}
		//if (enabledMS)
		//{
		//	_vkCmdDrawMeshTasksEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetDeviceProcAddr(px->_device, "vkCmdDrawMeshTasksEXT"));
		//}

#ifdef USE_VMA
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = px->_device;
		allocatorInfo.instance = px->_instance;
		vmaCreateAllocator(&allocatorInfo, &px->_hAllocator);
#endif
		// create queues
		uint32_t graphics_fidx = 0;
		std::vector<size_t> gqf, cqf, tqf;
		for (size_t i = 0; i < queue_family_count; i++)
		{
			auto& it = queue_props[i];
			if (it.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				gqf.push_back(i);
			}
			if (it.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				cqf.push_back(i);
			}
			if (it.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				tqf.push_back(i);
			}
			if (it.queueFlags & VK_QUEUE_GRAPHICS_BIT && it.queueFlags & VK_QUEUE_COMPUTE_BIT) {
				graphics_fidx = i;
				px->graphics_queueFlags = it.queueFlags;
				px->_queue_family_index = graphics_fidx;
				px->_graphics_queues.resize(it.queueCount);
			}
		}
		for (int i = 0; i < px->_graphics_queues.size(); i++) {
			VkQueue gq = 0;
			vkGetDeviceQueue(px->_device, graphics_fidx, i, &gq);
			px->_graphics_queues[i] = gq;
		}

		// 初始化扩展函数地址
		ExtDebugUtilsGetProcAddresses(px->_device);
#ifdef ExtGetHDRFSEFreesyncHDRProcAddresses
		ExtGetHDRFSEFreesyncHDRProcAddresses(px->_instance, m_device);
#endif
	}
	void Device_init(devinfo_x* px, dev_info_cx* d, void* pw)
	{
		dev_info_cx nd = {};
		if (d) { nd = *d; }
		d = &nd;
		if (!d || !d->inst || !d->phy)
		{
			printf("Failed to create Vulkan instance or physical device.\n");
			return;
		}
		DeviceProperties dp;
		dp.Init((VkPhysicalDevice)d->phy);
		SetEssentialDeviceExtensions(&dp);

		Device_CreateEx((VkInstance)d->inst, (VkPhysicalDevice)d->phy, (VkDevice)d->vkdev, pw, &dp, px);
		if (!d->vkdev)
			d->vkdev = px->_device;
	}



	// 哈希
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

	bool DefineList::Has(const std::string& str) const
	{
		return find(str) != end();
	}

	size_t DefineList::Hash(size_t result) const
	{
		for (auto it = begin(); it != end(); it++)
		{
			result = HashString(it->first, result);
			result = HashString(it->second, result);
		}
		return result;
	}
	// passing lhs by value helps optimize chained a+b+c
	// otherwise, both parameters may be const references
	DefineList operator+(DefineList def1, const DefineList& def2)
	{
		for (auto it = def2.begin(); it != def2.end(); it++)
			def1[it->first] = it->second;
		return def1;
	}
	std::string DefineList::to_string() const
	{
		std::string result;
		for (auto it = begin(); it != end(); it++)
		{
			result += it->first + "=" + it->second + "; ";
		}
		return result;
	}


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
#ifndef fseeki64
#ifdef _WIN32
#define fseeki64 _fseeki64
#define ftelli64 _ftelli64
#else			
#define fseeki64 fseeko64
#define ftelli64 ftello64
#endif // _WIN32
#endif
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
#endif
	}

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

	bool LaunchProcess(const char* commandLine, const char* filenameErr)
	{
#ifdef _WIN32
		std::string cmdlinestr;
		cmdlinestr = commandLine && *commandLine ? commandLine : "";
		auto cmdLine = (char*)cmdlinestr.c_str();
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
			commandLine = format("glslc --target-env=vulkan1.3 -fshader-stage=%s -fentry-point=%s %s \"%s\" -o \"%s\" -I %s %s", stage, pShaderEntryPoint, shaderCompilerParams, filenameGlsl.c_str(), filenameSpv.c_str(), GetShaderCompilerLibDir().c_str(), defines.c_str());

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


	VkResult CreateModule(VkDevice device, char* SpvData, size_t SpvSize, VkShaderModule* pShaderModule)
	{
		VkShaderModuleCreateInfo moduleCreateInfo = {};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pCode = (uint32_t*)SpvData;
		moduleCreateInfo.codeSize = SpvSize;
		return vkCreateShaderModule(device, &moduleCreateInfo, NULL, pShaderModule);
	}

#endif 
	// 1 shader


	VkDescriptorSetLayout newDescriptorSetLayout(VkDevice dev, std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding)
	{
		VkDescriptorSetLayout descSetLayout = {};
		VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
		descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_layout.pNext = NULL;
		descriptor_layout.bindingCount = (uint32_t)pDescriptorLayoutBinding->size();
		descriptor_layout.pBindings = pDescriptorLayoutBinding->data();
		VkResult res = vkCreateDescriptorSetLayout(dev, &descriptor_layout, NULL, &descSetLayout);
		assert(res == VK_SUCCESS);
		return descSetLayout;
	}
	bool is_morph_target_valid(vkg::ubotex_size_t* us, vkg::morph_target_t* mt)
	{
		return us && (us->ID_TARGET_DATA || us->ID_MORPHING_DATA) && mt && (mt->position_offset >= 0 || mt->normal_offset >= 0 || mt->tangent_offset >= 0 || mt->texcoord0_offset >= 0 || mt->texcoord1_offset >= 0 || mt->color0_offset >= 0);
	}
	bool is_morph_target_valid(vkg::ubotex_size_t* us)
	{
		return us && (us->ID_TARGET_DATA || us->ID_MORPHING_DATA);
	}
	VkDescriptorSetLayout newDescriptorSetLayout(VkDevice dev, vkg::ubotex_size_t* us, vkg::ubotex_size_t* binding)
	{
		std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
		int binc = 0;
		vkg::ubotex_size_t binding0 = {};
		if (!binding)
			binding = &binding0;
		layout_bindings.reserve(10);
		for (int i = 0; i < 3; i++)
		{
			VkDescriptorSetLayoutBinding b = {};
			auto ct = us->texCounts[i];
			if (ct > 0) {
				b.binding = binc++;
				b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				b.descriptorCount = ct;
				b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				b.pImmutableSamplers = NULL;
				layout_bindings.push_back(b);
			}
		}
		{
			VkDescriptorSetLayoutBinding b;
			// Constant buffer 'per frame'
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			binding->ID_PER_FRAME = b.binding;
			layout_bindings.push_back(b);
		}
		// Constant buffer 'per object'
		{
			VkDescriptorSetLayoutBinding b;
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			binding->ID_PER_OBJECT = b.binding;
			layout_bindings.push_back(b);
		}
		auto dt = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		auto dt1 = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		// Constant buffer holding the skinning matrices
		if (us->ID_SKINNING_MATRICES > 0)
		{
			VkDescriptorSetLayoutBinding b;
			// skinning matrices
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			b.descriptorType = dt;
			binding->ID_SKINNING_MATRICES = b.binding;
			layout_bindings.push_back(b);
		}
		if (is_morph_target_valid(us))
		{
			VkDescriptorSetLayoutBinding b;
			// 变形动画
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			b.descriptorType = dt1;
			binding->ID_TARGET_DATA = b.binding;
			layout_bindings.push_back(b);
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			b.descriptorType = dt;
			binding->ID_MORPHING_DATA = b.binding;
			layout_bindings.push_back(b);
		}
		int icbinding = 0;
		uint32_t ins_size = 0;
		if (us->ID_INSTANCING > 1)
		{
			// 实例矩阵
			VkDescriptorSetLayoutBinding b;
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			b.descriptorType = dt;
			icbinding = b.binding;
			binding->ID_INSTANCING = b.binding;
			layout_bindings.push_back(b);
		}
		if (us->ID_MATUV_DATA)
		{
			// UV矩阵
			VkDescriptorSetLayoutBinding b;
			b.binding = binc++;
			b.descriptorCount = 1;
			b.pImmutableSamplers = NULL;
			b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			b.descriptorType = dt;
			binding->ID_MATUV_DATA = b.binding;
			layout_bindings.push_back(b);
		}
		VkDescriptorSetLayout descSetLayout = {};
		VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
		descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_layout.pNext = NULL;
		descriptor_layout.bindingCount = (uint32_t)layout_bindings.size();
		descriptor_layout.pBindings = layout_bindings.data();
		VkResult res = vkCreateDescriptorSetLayout(dev, &descriptor_layout, NULL, &descSetLayout);
		assert(res == VK_SUCCESS);
		return descSetLayout;
	}
	void get_blend(bool blend, VkPipelineColorBlendAttachmentState& out)
	{
		VkPipelineColorBlendAttachmentState color_blend =
		{
			blend ? VK_TRUE : VK_FALSE,                                                      // VkBool32                                       blendEnable
			VK_BLEND_FACTOR_SRC_ALPHA,                                    // VkBlendFactor                                  srcColorBlendFactor
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,                          // VkBlendFactor                                  dstColorBlendFactor
			VK_BLEND_OP_ADD,                                              // VkBlendOp                                      colorBlendOp
			VK_BLEND_FACTOR_ONE,                                          // VkBlendFactor                                  srcAlphaBlendFactor
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,                          // VkBlendFactor                                  dstAlphaBlendFactor
			VK_BLEND_OP_ADD,                                              // VkBlendOp                                      alphaBlendOp
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |         // VkColorComponentFlags                          colorWriteMask
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};
		out = color_blend;
	}
	void get_one_blend(bool blend, VkPipelineColorBlendAttachmentState& out)
	{
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
		out = color_no_blend;
	}
#if 1
	// 计算顶点输入布局和绑定
	void md_vis(pipeline_info_2* info2, DefineList& defines, std::vector<VkVertexInputAttributeDescription>& playout, std::vector<VkVertexInputBindingDescription>& vi_binding)
	{
		static const char* kdefstr[] = { "ID_POSITION","ID_COLOR_0","ID_TEXCOORD_0","ID_NORMAL","ID_TANGENT","ID_WEIGHTS_0","ID_JOINTS_0","ID_TEXCOORD_1","ID_WEIGHTS_1","ID_JOINTS_1" };
		playout.reserve(info2->attributeCount);
		char tmp[64] = {};
		for (size_t i = 0; i < info2->attributeCount; i++)
		{
			auto& it = info2->attributes[i];
			VkVertexInputAttributeDescription a = {};
			a.location = i;
			a.binding = it.binding;
			a.format = (VkFormat)it.format;
			a.offset = it.offset;
			if (it.name < 10)
			{
				defines[kdefstr[it.name]] = i;
			}
			else {
				auto idx = it.name - 10;
				if (info2->names && info2->namesCount > idx && info2->names[idx])
				{
					defines[info2->names[idx]] = i;
				}
				else {
					sprintf(tmp, "ID_ATTR_%d", it.name);
					defines[tmp] = i;
				}
			}
			playout.push_back(a);
		}
		std::stable_sort(playout.begin(), playout.end(), [](const VkVertexInputAttributeDescription& a, const VkVertexInputAttributeDescription& b) { return a.location < b.location; });
		if (info2->bindingCount > 0)
		{
			vi_binding.resize(info2->bindingCount);
			for (int i = 0; i < info2->bindingCount; i++)
			{
				vi_binding[i].binding = info2->bindings[i].binding;
				vi_binding[i].stride = info2->bindings[i].stride;
				vi_binding[i].inputRate = info2->bindings[i].inputRate ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
			}
		}
		else {
			auto cc = playout[playout.size() - 1].binding;
			vi_binding.resize(cc);
			int offset = 0;
			int last_binding = 0;
			for (auto& it : playout)
			{
				auto b = it.binding;
				auto f = it.format;
				auto s = vkr::SizeOfFormat(f);
				assert(b < vi_binding.size());
				if (last_binding != b)
				{
					offset = 0; last_binding = b;
				}
				it.offset = offset;
				offset += s;
				vi_binding[b].binding = b;
				vi_binding[b].stride += s;
				vi_binding[b].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			}
		}
		return;
	}
	void new_pbr_pipeline(cxDevice* pdev, PBRPipe_t* pbrpipe, pipeline_info_2* info2)
	{
		auto info = &info2->info;
		if (!pbrpipe || !info2 || !info || pbrpipe->m_pipeline || !(info2->attributeCount > 0))
		{
			return;
		}
		auto dev = pdev->_dev;

		// 创建描述符布局
		VkDescriptorSetLayout layout1 = newDescriptorSetLayout(dev, &info2->set_binding, &pbrpipe->binding);

		if (!pbrpipe->m_pipelineLayout) {
			VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
			pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pPipelineLayoutCreateInfo.pNext = NULL;
			pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
			pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
			pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)1;
			pPipelineLayoutCreateInfo.pSetLayouts = &layout1;
			VkResult res = vkCreatePipelineLayout(dev, &pPipelineLayoutCreateInfo, NULL, &pbrpipe->m_pipelineLayout);
			assert(res == VK_SUCCESS);
			vkr::SetResourceName(dev, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)pbrpipe->m_pipelineLayout, "GltfPbrPass PipelineLayout");
		}

		// Compile and create shaders
		DefineList defines;
		auto tt = info2->defines0;
		for (size_t i = 0; i < info2->define_count; i++, tt += 2)
		{
			auto k = tt[0];
			auto v = tt[1];
			if (k && v)
				defines[k] = v;
		}
		VkPipelineShaderStageCreateInfo vertexShader = {}, fragmentShader = {};
		// vertex input state
		std::vector<VkVertexInputAttributeDescription> playout;
		std::vector<VkVertexInputBindingDescription> vi_binding;
		VkPipelineVertexInputStateCreateInfo vi = {};
		vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vi.pNext = NULL;
		vi.flags = 0;
		md_vis(info2, defines, playout, vi_binding);// 解析顶点输入布局，生成绑定和属性描述
		vi.vertexBindingDescriptionCount = (uint32_t)vi_binding.size();
		vi.pVertexBindingDescriptions = vi_binding.data();
		vi.vertexAttributeDescriptionCount = (uint32_t)playout.size();
		vi.pVertexAttributeDescriptions = playout.data();
		// Create pipeline 
		pdev->VKCompileFromFile(VK_SHADER_STAGE_VERTEX_BIT, info2->vertexShaderFile ? info2->vertexShaderFile : "GLTFPbrPass-vert.glsl", "main", "", &defines, &vertexShader);
		pdev->VKCompileFromFile(VK_SHADER_STAGE_FRAGMENT_BIT, info2->fragmentShaderFile ? info2->fragmentShaderFile : "GLTFPbrPass-frag.glsl", "main", "", &defines, &fragmentShader);
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertexShader, fragmentShader };
		// input assembly state
		VkPipelineInputAssemblyStateCreateInfo ia;
		ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		ia.pNext = NULL;
		ia.flags = 0;
		ia.primitiveRestartEnable = info->primitiveRestartEnable;
		ia.topology = (VkPrimitiveTopology)info->topology;// VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		// rasterizer state
		VkPipelineRasterizationStateCreateInfo rs;
		rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rs.pNext = NULL;
		rs.flags = 0;
		rs.polygonMode = (VkPolygonMode)info->polygonMode; //VK_POLYGON_MODE_FILL;
		rs.cullMode = info->doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
		rs.frontFace = (VkFrontFace)info->frontFace;// VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rs.depthClampEnable = VK_FALSE;
		rs.rasterizerDiscardEnable = VK_FALSE;
		rs.depthBiasEnable = VK_FALSE;
		rs.depthBiasConstantFactor = 0;
		rs.depthBiasClamp = 0;
		rs.depthBiasSlopeFactor = 0;
		rs.lineWidth = info->lineWidth;
		bool depthwrite = !info->blending;// || (defines.Has("DEF_alphaMode_BLEND")));
		std::vector<VkPipelineColorBlendAttachmentState> att_states;
		if ((uint32_t)info2->gbufferflag & (uint32_t)GBufferFlagBits::GBUFFER_FORWARD)
		{
			VkPipelineColorBlendAttachmentState att_state = {};
			get_blend(info->blending, att_state);
			att_states.push_back(att_state);
		}
		VkPipelineColorBlendAttachmentState cblend = {};
		cblend.colorWriteMask = allBits;
		cblend.blendEnable = VK_FALSE;
		cblend.alphaBlendOp = VK_BLEND_OP_ADD;
		cblend.colorBlendOp = VK_BLEND_OP_ADD;
		cblend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		cblend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		cblend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		cblend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		if ((uint32_t)info2->gbufferflag & (uint32_t)GBufferFlagBits::GBUFFER_SPECULAR_ROUGHNESS)
		{
			att_states.push_back(cblend);
		}
		if ((uint32_t)info2->gbufferflag & (uint32_t)GBufferFlagBits::GBUFFER_DIFFUSE)
		{
			att_states.push_back(cblend);
		}
		if ((uint32_t)info2->gbufferflag & (uint32_t)GBufferFlagBits::GBUFFER_NORMAL_BUFFER)
		{
			att_states.push_back(cblend);
		}
		if ((uint32_t)info2->gbufferflag & (uint32_t)GBufferFlagBits::GBUFFER_MOTION_VECTORS)
		{
			att_states.push_back(cblend);
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
		ds.depthTestEnable = info->depthTestEnable;
		ds.depthWriteEnable = depthwrite;
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
		ds.stencilTestEnable = info->stencilTestEnable;
		ds.front = ds.back;
		// multi sample state
		VkPipelineMultisampleStateCreateInfo ms;
		ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms.pNext = NULL;
		ms.flags = 0;
		ms.pSampleMask = NULL;
		ms.rasterizationSamples = (VkSampleCountFlagBits)info->rasterizationSamples;
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
		pipeline.renderPass = (VkRenderPass)info->renderPass;
		pipeline.subpass = 0;
#if 0
		struct sc {
			bool ID_diffuseCube = true;	// 漫反环境
			bool ID_specularCube = false;	// 高光镜面环境u_GGXEnvSampler
			bool ID_CharlieCube = false;	// 光泽环境
			bool USE_TEX_LOD = true;		// 环境lod

			int ID_baseColorTexture = -1;
			int ID_normalTexture = -1;
			int ID_emissiveTexture = -1;
			int ID_metallicRoughnessTexture = -1;
			int ID_occlusionTexture = -1;
			int ID_diffuseTexture = -1;
			int ID_specularGlossinessTexture = -1;
			int ID_GGXLUT = -1;
			int ID_CharlieTexture = -1;
			int ID_SheenETexture = -1;
			int ID_sheenColorTexture = -1;
			int ID_sheenRoughnessTexture = -1;
			int ID_specularTexture = -1;
			int ID_specularColorTexture = -1;
			int ID_transmissionTexture = -1;
			int ID_thicknessTexture = -1;
			int ID_clearcoatRoughnessTexture = -1;
			int ID_clearcoatNormalTexture = -1;
			int ID_iridescenceTexture = -1;
			int ID_iridescenceThicknessTexture = -1;
			int ID_anisotropyTexture = -1;
			int ID_SSAO = -1;
			int ID_transmissionFramebufferTexture = -1;

			int UVT_baseColorTexture = 0;
			int UVT_normalTexture = 0;
			int UVT_emissiveTexture = 0;
			int UVT_metallicRoughnessTexture = 0;
			int UVT_occlusionTexture = 0;
			int UVT_diffuseTexture = 0;
			int UVT_specularGlossinessTexture = 0;
			int UVT_GGXLUT = 0;
			int UVT_CharlieTexture = 0;
			int UVT_SheenETexture = 0;
			int UVT_sheenColorTexture = 0;
			int UVT_sheenRoughnessTexture = 0;
			int UVT_specularTexture = 0;
			int UVT_specularColorTexture = 0;
			int UVT_transmissionTexture = 0;
			int UVT_thicknessTexture = 0;
			int UVT_clearcoatRoughnessTexture = 0;
			int UVT_clearcoatNormalTexture = 0;
			int UVT_iridescenceTexture = 0;
			int UVT_iridescenceThicknessTexture = 0;
			int UVT_anisotropyTexture = 0;
			int UVT_SSAO = 0;
			int UVT_transmissionFramebufferTexture = 0;
		};
		sc specConstants = {};
		std::vector<VkSpecializationMapEntry> specializationMapEntry = {};
		specializationMapEntry.resize(50);
		auto smt = specializationMapEntry.data();
		int offset = 0;
		for (size_t i = 0; i < 4; i++)
		{
			smt->constantID = i;
			smt->offset = offset;
			smt->size = sizeof(bool);
			offset += smt->size;
			smt++;
		}
		for (size_t i = 4; i < 50; i++)
		{
			smt->constantID = i;
			smt->offset = offset;
			smt->size = sizeof(int);
			offset += smt->size;
			smt++;
		}
		VkSpecializationInfo specializationInfo = VkSpecializationInfo();
		specializationInfo.mapEntryCount = specializationMapEntry.size();
		specializationInfo.pMapEntries = specializationMapEntry.data();
		specializationInfo.dataSize = sizeof(sc);
		specializationInfo.pData = &specConstants;
		shaderStages[1].pSpecializationInfo = &specializationInfo;
#endif
		VkResult res = vkCreateGraphicsPipelines(dev, pdev->GetPipelineCache(), 1, &pipeline, NULL, &pbrpipe->m_pipeline);
		assert(res == VK_SUCCESS);
		vkr::SetResourceName(dev, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pbrpipe->m_pipeline, "GltfPbrPass Pipeline");
		// create wireframe pipeline
		rs.polygonMode = VK_POLYGON_MODE_LINE;
		rs.cullMode = VK_CULL_MODE_NONE;
		//ds.depthWriteEnable = false;
		res = vkCreateGraphicsPipelines(dev, pdev->GetPipelineCache(), 1, &pipeline, NULL, &pbrpipe->m_pipelineWireframe);
		assert(res == VK_SUCCESS);
		vkr::SetResourceName(dev, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pbrpipe->m_pipelineWireframe, "GltfPbrPass Wireframe Pipeline");
	}
#endif


	VkResult buffer_ring_cx::OnCreate(cxDevice* pDevice, uint32_t numberOfBackBuffers, uint32_t memTotalSize, char* name)
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
		SetResourceName(pDevice->_dev, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, "buffer_ring_cx");

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
		res = vkCreateBuffer(m_pDevice->_dev, &buf_info, NULL, &m_buffer);
		assert(res == VK_SUCCESS);

		VkMemoryRequirements mem_reqs;
		vkGetBufferMemoryRequirements(m_pDevice->_dev, m_buffer, &mem_reqs);

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

		res = vkAllocateMemory(m_pDevice->_dev, &alloc_info, NULL, &m_deviceMemory);
		assert(res == VK_SUCCESS);

		res = vkMapMemory(m_pDevice->_dev, m_deviceMemory, 0, mem_reqs.size, 0, (void**)&m_pData);
		assert(res == VK_SUCCESS);

		res = vkBindBufferMemory(m_pDevice->_dev, m_buffer, m_deviceMemory, 0);
		assert(res == VK_SUCCESS);
#endif
		return res;
	}

	//-------------------------------------------------------------------------------------- 
	// OnDestroy 
	void buffer_ring_cx::OnDestroy()
	{
#ifdef USE_VMA
		vmaUnmapMemory(m_pDevice->GetAllocator(), m_bufferAlloc);
		vmaDestroyBuffer(m_pDevice->GetAllocator(), m_buffer, m_bufferAlloc);
		m_bufferAlloc = 0;
#else
		vkUnmapMemory(m_pDevice->_dev, m_deviceMemory);
		vkFreeMemory(m_pDevice->_dev, m_deviceMemory, NULL);
		vkDestroyBuffer(m_pDevice->_dev, m_buffer, NULL);
		m_deviceMemory = 0;
#endif
		m_buffer = 0;
		m_mem.OnDestroy();
	}

	//-------------------------------------------------------------------------------------- 
	// AllocConstantBuffer 
	bool buffer_ring_cx::AllocConstantBuffer1(uint32_t size, void** pData, VkDescriptorBufferInfo* pOut, uint32_t algin)
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
	bool buffer_ring_cx::AllocConstantBuffer(uint32_t size, void** pData, VkDescriptorBufferInfo* pOut)
	{
		return AllocConstantBuffer1(size, pData, pOut, 64u);
	}

	bool buffer_ring_cx::AllocBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, const void* pInitData, VkDescriptorBufferInfo* pOut)
	{
		auto db = AllocConstantBuffer(numbeOfVertices * strideInBytes, (void*)pInitData);
		if (pOut)*pOut = db;
		return db.buffer != 0;
	}
	//-------------------------------------------------------------------------------------- 
	// AllocConstantBuffer 
	VkDescriptorBufferInfo buffer_ring_cx::AllocConstantBuffer(uint32_t size, void* pData)
	{
		void* pBuffer;
		VkDescriptorBufferInfo out;
		if (AllocConstantBuffer(size, &pBuffer, &out) && pData)
		{
			memcpy(pBuffer, pData, size);
		}

		return out;
	}

	//-------------------------------------------------------------------------------------- 
	// AllocVertexBuffer 
	bool buffer_ring_cx::AllocVertexBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void** pData, VkDescriptorBufferInfo* pOut)
	{
		return AllocConstantBuffer(numbeOfVertices * strideInBytes, pData, pOut);
	}

	bool buffer_ring_cx::AllocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void** pData, VkDescriptorBufferInfo* pOut)
	{
		return AllocConstantBuffer(numbeOfIndices * strideInBytes, pData, pOut);
	}

	//-------------------------------------------------------------------------------------- 
	void buffer_ring_cx::OnBeginFrame()
	{
		m_mem.OnBeginFrame();
	}

	void buffer_ring_cx::SetDescriptorSet(int index, uint32_t size, VkDescriptorSet descriptorSet, uint32_t dt)
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

		vkUpdateDescriptorSets(m_pDevice->_dev, 1, &write, 0, NULL);
	}
	VkResult static_buffer_pool_cx::OnCreate(cxDevice* pDevice, uint32_t totalMemSize, bool bUseVidMem, const char* name)
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
		SetResourceName(pDevice->_dev, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, "static buffer pool (sys mem)");

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
		res = vkCreateBuffer(m_pDevice->_dev, &buf_info, NULL, &m_buffer);
		assert(res == VK_SUCCESS);

		// allocate the buffer in system memory

		VkMemoryRequirements mem_reqs;
		vkGetBufferMemoryRequirements(m_pDevice->_dev, m_buffer, &mem_reqs);

		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.pNext = NULL;
		alloc_info.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		alloc_info.allocationSize = mem_reqs.size;

		bool pass = memory_type_from_properties(m_pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&alloc_info.memoryTypeIndex);
		assert(pass && "No mappable, coherent memory");

		res = vkAllocateMemory(m_pDevice->_dev, &alloc_info, NULL, &m_deviceMemory);
		assert(res == VK_SUCCESS);

		// bind buffer

		res = vkBindBufferMemory(m_pDevice->_dev, m_buffer, m_deviceMemory, 0);
		assert(res == VK_SUCCESS);

		// Map it and leave it mapped. This is fine for Win10 and Win7.

		res = vkMapMemory(m_pDevice->_dev, m_deviceMemory, 0, mem_reqs.size, 0, (void**)&m_pData);
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
			SetResourceName(pDevice->_dev, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, "static buffer pool (vid mem)");
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
			res = vkCreateBuffer(m_pDevice->_dev, &buf_info, NULL, &m_bufferVid);
			assert(res == VK_SUCCESS);

			// allocate the buffer in VIDEO memory

			VkMemoryRequirements mem_reqs;
			vkGetBufferMemoryRequirements(m_pDevice->_dev, m_bufferVid, &mem_reqs);

			VkMemoryAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.pNext = NULL;
			alloc_info.memoryTypeIndex = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			alloc_info.allocationSize = mem_reqs.size;

			bool pass = memory_type_from_properties(m_pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &alloc_info.memoryTypeIndex);
			assert(pass && "No mappable, coherent memory");

			res = vkAllocateMemory(m_pDevice->_dev, &alloc_info, NULL, &m_deviceMemoryVid);
			assert(res == VK_SUCCESS);

			// bind buffer

			res = vkBindBufferMemory(m_pDevice->_dev, m_bufferVid, m_deviceMemoryVid, 0);
			assert(res == VK_SUCCESS);
#endif
		}

		return res;
	}

	void static_buffer_pool_cx::OnDestroy()
	{
		if (m_bUseVidMem)
		{
#ifdef USE_VMA
			vmaDestroyBuffer(m_pDevice->GetAllocator(), m_bufferVid, m_bufferAllocVid);
#else
			vkFreeMemory(m_pDevice->_dev, m_deviceMemoryVid, NULL);
			vkDestroyBuffer(m_pDevice->_dev, m_bufferVid, NULL);
#endif
			m_bufferVid = 0; m_bufferAllocVid = 0;
		}

		if (m_buffer != VK_NULL_HANDLE)
		{
#ifdef USE_VMA
			vmaUnmapMemory(m_pDevice->GetAllocator(), m_bufferAlloc);
			vmaDestroyBuffer(m_pDevice->GetAllocator(), m_buffer, m_bufferAlloc);
#else
			vkUnmapMemory(m_pDevice->_dev, m_deviceMemory);
			vkFreeMemory(m_pDevice->_dev, m_deviceMemory, NULL);
			vkDestroyBuffer(m_pDevice->_dev, m_buffer, NULL);
#endif
			m_buffer = VK_NULL_HANDLE;
		}
	}


	bool static_buffer_pool_cx::AllocBuffer(uint32_t numbeOfElements, uint32_t strideInBytes, void** pData, VkDescriptorBufferInfo* pOut)
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

	bool static_buffer_pool_cx::AllocBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, const void* pInitData, VkDescriptorBufferInfo* pOut)
	{
		void* pData;
		if (AllocBuffer(numbeOfVertices, strideInBytes, &pData, pOut))
		{
			memcpy(pData, pInitData, numbeOfVertices * strideInBytes);
			return true;
		}
		return false;
	}

	void static_buffer_pool_cx::UploadData(VkCommandBuffer cmd_buf)
	{
		VkBufferCopy region;
		region.srcOffset = 0;
		region.dstOffset = 0;
		region.size = m_totalMemSize;

		vkCmdCopyBuffer(cmd_buf, m_buffer, m_bufferVid, 1, &region);
	}

	void static_buffer_pool_cx::FreeUploadHeap()
	{
		if (m_bUseVidMem)
		{
			assert(m_buffer != VK_NULL_HANDLE);
#ifdef USE_VMA
			vmaUnmapMemory(m_pDevice->GetAllocator(), m_bufferAlloc);
			vmaDestroyBuffer(m_pDevice->GetAllocator(), m_buffer, m_bufferAlloc);
#else
			//release upload heap
			vkUnmapMemory(m_pDevice->_dev, m_deviceMemory);
			vkFreeMemory(m_pDevice->_dev, m_deviceMemory, NULL);
			m_deviceMemory = VK_NULL_HANDLE;
			vkDestroyBuffer(m_pDevice->_dev, m_buffer, NULL);

#endif
			m_buffer = VK_NULL_HANDLE;
		}
	}

	// todo set

	void ResourceViewHeaps::OnCreate(cxDevice* pDevice, uint32_t cbvDescriptorCount, uint32_t srvDescriptorCount, uint32_t uavDescriptorCount, uint32_t samplerDescriptorCount)
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

		VkResult res = vkCreateDescriptorPool(pDevice->_dev, &descriptor_pool, NULL, &m_descriptorPool);
		assert(res == VK_SUCCESS);
	}

	void ResourceViewHeaps::OnDestroy()
	{
		vkDestroyDescriptorPool(m_pDevice->_dev, m_descriptorPool, NULL);
	}

	bool ResourceViewHeaps::CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding, VkDescriptorSetLayout* pDescSetLayout)
	{
		// Next take layout bindings and use them to create a descriptor set layout

		VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
		descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_layout.pNext = NULL;
		descriptor_layout.bindingCount = (uint32_t)pDescriptorLayoutBinding->size();
		descriptor_layout.pBindings = pDescriptorLayoutBinding->data();

		VkResult res = vkCreateDescriptorSetLayout(m_pDevice->_dev, &descriptor_layout, NULL, pDescSetLayout);
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

		VkResult res = vkCreateDescriptorSetLayout(m_pDevice->_dev, &descriptor_layout, NULL, pDescSetLayout);
		assert(res == VK_SUCCESS);

		return AllocDescriptor(*pDescSetLayout, pDescriptorSet);
	}

	bool ResourceViewHeaps::AllocDescriptor(VkDescriptorSetLayout descLayout, VkDescriptorSet* pDescriptorSet)
	{
		//std::lock_guard<std::mutex> lock(m_mutex);

		VkDescriptorSetAllocateInfo alloc_info;
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.pNext = NULL;
		alloc_info.descriptorPool = m_descriptorPool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = &descLayout;
		VkResult res = vkAllocateDescriptorSets(m_pDevice->_dev, &alloc_info, pDescriptorSet);
		assert(res == VK_SUCCESS);
		m_allocatedDescriptorCount++;
		return res == VK_SUCCESS;
	}

	void ResourceViewHeaps::FreeDescriptor(VkDescriptorSet descriptorSet)
	{
		m_allocatedDescriptorCount--;
		vkFreeDescriptorSets(m_pDevice->_dev, m_descriptorPool, 1, &descriptorSet);
	}

	cxDevice* ResourceViewHeaps::get_dev()
	{
		return m_pDevice;
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



	// todo 纹理相关
#if 1


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


#ifdef USE_WIC
#include "wincodec.h"
	static IWICImagingFactory* m_pWICFactory = NULL;
#endif

	WICLoader::WICLoader()
	{}
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

	DDSLoader::DDSLoader()
	{}
	DDSLoader::~DDSLoader()
	{
#ifdef _WIN32
		if (m_handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_handle);
		}
#endif
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

#ifdef _WIN32
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
#endif
		return true;
	}

	void DDSLoader::CopyPixels(void* pDest, uint32_t stride, uint32_t bytesWidth, uint32_t height)
	{
#ifdef _WIN32
		assert(m_handle != INVALID_HANDLE_VALUE);
		for (uint32_t y = 0; y < height; y++)
		{
			::ReadFile(m_handle, (char*)pDest + y * stride, bytesWidth, NULL, NULL);
		}
#endif
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
			vkDestroyImage(m_pDevice->_dev, m_pResource, nullptr);
			vkFreeMemory(m_pDevice->_dev, m_deviceMemory, nullptr);
			m_deviceMemory = VK_NULL_HANDLE;
		}
#endif

	}

	bool Texture::isCubemap() const
	{
		return m_header.arraySize == 6;
	}

	INT32 Texture::Init(cxDevice* pDevice, VkImageCreateInfo* pCreateInfo, const char* name)
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
		SetResourceName(pDevice->_dev, VK_OBJECT_TYPE_IMAGE, (uint64_t)m_pResource, m_name.c_str());
#else
		/* Create image */
		VkResult res = vkCreateImage(m_pDevice->_dev, pCreateInfo, NULL, &m_pResource);
		assert(res == VK_SUCCESS);

		VkMemoryRequirements mem_reqs;
		vkGetImageMemoryRequirements(m_pDevice->_dev, m_pResource, &mem_reqs);

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
		res = vkAllocateMemory(m_pDevice->_dev, &alloc_info, NULL, &m_deviceMemory);
		assert(res == VK_SUCCESS);

		/* bind memory */
		res = vkBindImageMemory(m_pDevice->_dev, m_pResource, m_deviceMemory, 0);
		assert(res == VK_SUCCESS);
#endif
		return 0;
	}

	INT32 Texture::InitRenderTarget(cxDevice* pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, VkImageUsageFlags usage, bool bUAV, const char* name, VkImageCreateFlagBits flags)
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
		VkResult res = vkCreateImageView(m_pDevice->_dev, &info, NULL, pImageView);
		assert(res == VK_SUCCESS);

		SetResourceName(m_pDevice->_dev, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*pImageView, ResourceName.c_str());
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
		VkResult res = vkCreateImageView(m_pDevice->_dev, &info, NULL, pImageView);
		assert(res == VK_SUCCESS);

		SetResourceName(m_pDevice->_dev, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*pImageView, m_name.c_str());
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
		VkResult res = vkCreateImageView(m_pDevice->_dev, &info, NULL, pImageView);
		assert(res == VK_SUCCESS);

		SetResourceName(m_pDevice->_dev, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*pImageView, m_name.c_str());
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
		VkResult res = vkCreateImageView(m_pDevice->_dev, &view_info, NULL, pImageView);
		assert(res == VK_SUCCESS);

		SetResourceName(m_pDevice->_dev, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*pImageView, m_name.c_str());
	}

	INT32 Texture::InitDepthStencil(cxDevice* pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, const char* name)
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
	VkImage Texture::CreateTextureCommitted(cxDevice* pDevice, UploadHeap* pUploadHeap, const char* pName, bool useSRGB, VkImageUsageFlags usageFlags)
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
			SetResourceName(pDevice->_dev, VK_OBJECT_TYPE_IMAGE, (uint64_t)tex, m_name.c_str());
#else
			VkResult res = vkCreateImage(pDevice->_dev, &info, NULL, &tex);
			assert(res == VK_SUCCESS);

			VkMemoryRequirements mem_reqs;
			vkGetImageMemoryRequirements(pDevice->_dev, tex, &mem_reqs);

			VkMemoryAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize = mem_reqs.size;
			alloc_info.memoryTypeIndex = 0;

			bool pass = memory_type_from_properties(pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&alloc_info.memoryTypeIndex);
			assert(pass && "No mappable, coherent memory");

			res = vkAllocateMemory(pDevice->_dev, &alloc_info, NULL, &m_deviceMemory);
			assert(res == VK_SUCCESS);

			res = vkBindImageMemory(pDevice->_dev, tex, m_deviceMemory, 0);
			assert(res == VK_SUCCESS);
#endif
		}

		return tex;
	}

	void Texture::LoadAndUpload(cxDevice* pDevice, UploadHeap* pUploadHeap, ImgLoader* pDds, VkImage pTexture2D)
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

	void Texture::LoadAndUpload0(cxDevice* pDevice, UploadHeap* pUploadHeap, char* data, VkImage pTexture2D)
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

	void upload_data(cxDevice* pDevice, UploadHeap* up, IMG_INFO* info, uint32_t bufferOffset, uint32_t usage, VkImage image)
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
	bool Texture::InitFromFile(cxDevice* pDevice, UploadHeap* pUploadHeap, const char* pFilename, bool useSRGB, VkImageUsageFlags usageFlags, float cutOff)
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

	bool Texture::InitFromData(cxDevice* pDevice, UploadHeap* uploadHeap, IMG_INFO* header, const void* data, int dsize, const char* name, bool useSRGB)
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


	//--------------------------------------------------------------------------------------
	//
	// OnCreate
	//
	//--------------------------------------------------------------------------------------
	void UploadHeap::OnCreate(cxDevice* pDevice, SIZE_T uSize)
	{
		m_pDevice = pDevice;

		VkResult res;

		// Create command list and allocators 
		{
			VkCommandPoolCreateInfo cmd_pool_info = {};
			cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmd_pool_info.queueFamilyIndex = m_pDevice->_queue_family_index;
			cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			res = vkCreateCommandPool(m_pDevice->_dev, &cmd_pool_info, NULL, &m_commandPool);
			assert(res == VK_SUCCESS);

			VkCommandBufferAllocateInfo cmd = {};
			cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmd.pNext = NULL;
			cmd.commandPool = m_commandPool;
			cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmd.commandBufferCount = 1;
			res = vkAllocateCommandBuffers(m_pDevice->_dev, &cmd, &m_pCommandBuffer);
			assert(res == VK_SUCCESS);
		}

		// Create buffer to suballocate
		{
			VkBufferCreateInfo buffer_info = {};
			buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffer_info.size = uSize;
			buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			res = vkCreateBuffer(m_pDevice->_dev, &buffer_info, NULL, &m_buffer);
			assert(res == VK_SUCCESS);

			VkMemoryRequirements mem_reqs;
			vkGetBufferMemoryRequirements(m_pDevice->_dev, m_buffer, &mem_reqs);

			VkMemoryAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize = mem_reqs.size;
			alloc_info.memoryTypeIndex = 0;
			auto dmp = m_pDevice->GetPhysicalDeviceMemoryProperties();
			bool pass = memory_type_from_properties(dmp, mem_reqs.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				&alloc_info.memoryTypeIndex);
			assert(pass && "No mappable, coherent memory");

			res = vkAllocateMemory(m_pDevice->_dev, &alloc_info, NULL, &m_deviceMemory);
			assert(res == VK_SUCCESS);

			res = vkBindBufferMemory(m_pDevice->_dev, m_buffer, m_deviceMemory, 0);
			assert(res == VK_SUCCESS);

			res = vkMapMemory(m_pDevice->_dev, m_deviceMemory, 0, mem_reqs.size, 0, (void**)&m_pDataBegin);
			assert(res == VK_SUCCESS);

			m_pDataCur = m_pDataBegin;
			m_pDataEnd = m_pDataBegin + mem_reqs.size;
		}

		// Create fence
		{
			VkFenceCreateInfo fence_ci = {};
			fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

			res = vkCreateFence(m_pDevice->_dev, &fence_ci, NULL, &m_fence);
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
			vkDestroyBuffer(m_pDevice->_dev, m_buffer, NULL);
			vkUnmapMemory(m_pDevice->_dev, m_deviceMemory);
			vkFreeMemory(m_pDevice->_dev, m_deviceMemory, NULL);

			vkFreeCommandBuffers(m_pDevice->_dev, m_commandPool, 1, &m_pCommandBuffer);
			vkDestroyCommandPool(m_pDevice->_dev, m_commandPool, NULL);

			vkDestroyFence(m_pDevice->_dev, m_fence, NULL);

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
		res = vkFlushMappedMemoryRanges(m_pDevice->_dev, 1, range);
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
			//Trace("flushing %i", m_copies.size());

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
		auto st = vkGetFenceStatus(m_pDevice->_dev, m_fence);
		printf("UploadHeap fence: %d\n", st);
		res = vkQueueSubmit(m_pDevice->_queue, 1, &submit_info, m_fence);
		assert(res == VK_SUCCESS);

		// Make sure it's been processed by the GPU

		res = vkWaitForFences(m_pDevice->_dev, 1, &m_fence, VK_TRUE, UINT64_MAX);
		assert(res == VK_SUCCESS);

		vkResetFences(m_pDevice->_dev, 1, &m_fence);

		// Reset so it can be reused
		VkCommandBufferBeginInfo cmd_buf_info = {};
		cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		res = vkBeginCommandBuffer(m_pCommandBuffer, &cmd_buf_info);
		assert(res == VK_SUCCESS);

		m_pDataCur = m_pDataBegin;

		flushing.Dec();
	}

#endif // 1
	// 纹理


	void GPUTimestamps::OnCreate(cxDevice* pDevice, uint32_t numberOfBackBuffers, double timestamp_period)
	{
		m_pDevice = pDevice;
		m_NumberOfBackBuffers = numberOfBackBuffers;
		m_frame = 0;
		timestampPeriod = timestamp_period;
		const VkQueryPoolCreateInfo queryPoolCreateInfo =
		{
			VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,     // VkStructureType                  sType
			NULL,                                         // const void*                      pNext
			(VkQueryPoolCreateFlags)0,                    // VkQueryPoolCreateFlags           flags
			VK_QUERY_TYPE_TIMESTAMP ,                     // VkQueryType                      queryType
			MaxValuesPerFrame * numberOfBackBuffers,      // deUint32                         entryCount
			0,                                            // VkQueryPipelineStatisticFlags    pipelineStatistics
		};
		VkResult res = vkCreateQueryPool(pDevice->_dev, &queryPoolCreateInfo, NULL, &m_QueryPool);
	}

	void GPUTimestamps::OnDestroy()
	{
		vkDestroyQueryPool(m_pDevice->_dev, m_QueryPool, nullptr);
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
		for (uint32_t i = 0; i < cpuTimeStamps.size(); i++)
		{
			pTimestamps->push_back(cpuTimeStamps[i]);
		}
		// copy GPU timestamps
		uint32_t offset = m_frame * MaxValuesPerFrame;
		uint32_t measurements = (uint32_t)gpuLabels.size();
		if (measurements > 0)
		{
			// timestampPeriod is the number of nanoseconds per timestamp value increment
			double microsecondsPerTick = (1e-3f * timestampPeriod);
			{
				UINT64 TimingsInTicks[256] = {};
				VkResult res = vkGetQueryPoolResults(m_pDevice->_dev, m_QueryPool, offset, measurements, measurements * sizeof(UINT64), &TimingsInTicks, sizeof(UINT64), VK_QUERY_RESULT_64_BIT);
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
		attachments[0].format = (VkFormat)format;
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
	void SetViewportAndScissor(VkCommandBuffer cmd_buf, uint32_t topX, uint32_t topY, uint32_t width, uint32_t height, bool flipY)
	{
		VkViewport viewport;
		viewport.x = static_cast<float>(topX);
		viewport.width = static_cast<float>(width);
		if (flipY) {
			viewport.y = static_cast<float>(topY);
			viewport.height = static_cast<float>(height);
		}
		else {
			viewport.y = static_cast<float>(topY) + static_cast<float>(height);
			viewport.height = -static_cast<float>(height);
		}
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

	//-------------------------------------------------------------------------------------
	void GBufferRenderPass::OnCreate(GBuffer* pGBuffer, GBufferFlags flags, bool bClear, const std::string& name)
	{
		m_flags = flags;
		m_pGBuffer = pGBuffer;
		m_pDevice = pGBuffer->GetDevice();

		m_renderPass = pGBuffer->CreateRenderPass(flags, bClear);
		SetResourceName(m_pDevice->_dev, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)m_renderPass, name.c_str());
	}

	void GBufferRenderPass::OnDestroy()
	{
		vkDestroyRenderPass(m_pGBuffer->GetDevice()->_dev, m_renderPass, nullptr);
	}

	void GBufferRenderPass::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height)
	{
		std::vector<VkImageView> attachments;
		m_pGBuffer->GetAttachmentList(m_flags, &attachments, &m_clearValues);
		m_frameBuffer = m_pGBuffer->GetDevice()->newFrameBuffer(m_renderPass, &attachments, Width, Height);
	}

	void GBufferRenderPass::OnDestroyWindowSizeDependentResources()
	{
		vkDestroyFramebuffer(m_pGBuffer->GetDevice()->_dev, m_frameBuffer, nullptr);
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

		SetViewportAndScissor(commandList, renderArea.offset.x, renderArea.offset.y, renderArea.extent.width, renderArea.extent.height, false);
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

	void GBuffer::OnCreate(cxDevice* pDevice, ResourceViewHeaps* pHeaps, const std::map<GBufferFlags, VkFormat>& formats, int sampleCount)
	{
		m_GBufferFlags = GBUFFER_NONE;
		for (auto a : formats)
			m_GBufferFlags = m_GBufferFlags | a.first;

		m_pDevice = pDevice;
		m_sampleCount = (VkSampleCountFlagBits)sampleCount;
		m_formats = formats;
	}

	void GBuffer::OnDestroy()
	{}

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

		return CreateRenderPassOptimal(m_pDevice->_dev, colorAttanchmentCount, colorAttachments, &depthAttachment);
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
			vkDestroyImageView(m_pDevice->_dev, m_SpecularRoughnessSRV, nullptr);
			m_SpecularRoughness.OnDestroy();
		}

		if (m_GBufferFlags & GBUFFER_DIFFUSE)
		{
			vkDestroyImageView(m_pDevice->_dev, m_DiffuseSRV, nullptr);
			m_Diffuse.OnDestroy();
		}

		if (m_GBufferFlags & GBUFFER_NORMAL_BUFFER)
		{
			vkDestroyImageView(m_pDevice->_dev, m_NormalBufferSRV, nullptr);
			m_NormalBuffer.OnDestroy();
		}

		if (m_GBufferFlags & GBUFFER_MOTION_VECTORS)
		{
			vkDestroyImageView(m_pDevice->_dev, m_MotionVectorsSRV, nullptr);
			m_MotionVectors.OnDestroy();
		}

		if (m_GBufferFlags & GBUFFER_FORWARD)
		{
			vkDestroyImageView(m_pDevice->_dev, m_HDRSRV, nullptr);
			m_HDR.OnDestroy();
			vkDestroyImageView(m_pDevice->_dev, m_HDRSRVt, nullptr);
			m_HDRt.OnDestroy();
		}
#if 0
		if (m_GBufferFlags & GBUFFER_OIT_ACCUM)
		{
			vkDestroyImageView(m_pDevice->_dev, m_HDR_oit_accumSRV, nullptr);
			m_HDR_oit_accum.OnDestroy();
		}

		if (m_GBufferFlags & GBUFFER_OIT_WEIGHT)
		{
			vkDestroyImageView(m_pDevice->_dev, m_HDR_oit_weightSRV, nullptr);
			m_HDR_oit_weight.OnDestroy();
		}
#endif
		if (m_GBufferFlags & GBUFFER_DEPTH)
		{
			vkDestroyImageView(m_pDevice->_dev, m_DepthBufferDSV, nullptr);
			vkDestroyImageView(m_pDevice->_dev, m_DepthBufferSRV, nullptr);
			m_DepthBuffer.OnDestroy();
		}
	}

	//--------------------------------------------------------------------------------------
	// todo PostProcPS
	void PostProcPS::OnCreate(
		cxDevice* pDevice,
		VkRenderPass renderPass,
		const std::string& shaderFilename,
		const std::string& shaderEntryPoint,
		const std::string& shaderCompilerParams,
		static_buffer_pool_cx* pStaticBufferPool,
		buffer_ring_cx* pDynamicBufferRing,
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
		//res = VKCompileFromString(m_pDevice->_dev, SST_HLSL, VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "mainVS", CompileFlagsVS.c_str(), &attributeDefines, &m_vertexShader);
		res = pDevice->VKCompileFromFile(VK_SHADER_STAGE_VERTEX_BIT, "sky.v.glsl", "main", "", &attributeDefines, &m_vertexShader);
		assert(res == VK_SUCCESS);

		m_fragmentShaderName = shaderEntryPoint;
		VkPipelineShaderStageCreateInfo m_fragmentShader;
		res = m_pDevice->VKCompileFromFile(VK_SHADER_STAGE_FRAGMENT_BIT, shaderFilename.c_str(), m_fragmentShaderName.c_str(), shaderCompilerParams.c_str(), &attributeDefines, &m_fragmentShader);
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

		res = vkCreatePipelineLayout(pDevice->_dev, &pPipelineLayoutCreateInfo, NULL, &m_pipelineLayout);
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
			vkDestroyPipeline(m_pDevice->_dev, m_pipeline, nullptr);
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

		res = vkCreateGraphicsPipelines(m_pDevice->_dev, m_pDevice->GetPipelineCache(), 1, &pipeline, NULL, &m_pipeline);
		assert(res == VK_SUCCESS);
		SetResourceName(m_pDevice->_dev, VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_pipeline, "cacaca P");
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
			vkDestroyPipeline(m_pDevice->_dev, m_pipeline, nullptr);
			m_pipeline = VK_NULL_HANDLE;
		}

		if (m_pipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(m_pDevice->_dev, m_pipelineLayout, nullptr);
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
		cxDevice* pDevice,
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
		res = m_pDevice->VKCompileFromFile(VK_SHADER_STAGE_COMPUTE_BIT, shaderFilename.c_str(), shaderEntryPoint.c_str(), shaderCompilerParams.c_str(), &defines, &computeShader);
		assert(res == VK_SUCCESS);
		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
		pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pPipelineLayoutCreateInfo.pNext = NULL;
		pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
		pPipelineLayoutCreateInfo.setLayoutCount = 1;
		pPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
		res = vkCreatePipelineLayout(pDevice->_dev, &pPipelineLayoutCreateInfo, NULL, &m_pipelineLayout);
		assert(res == VK_SUCCESS);
		VkComputePipelineCreateInfo pipeline = {};
		pipeline.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeline.pNext = NULL;
		pipeline.flags = 0;
		pipeline.layout = m_pipelineLayout;
		pipeline.stage = computeShader;
		pipeline.basePipelineHandle = VK_NULL_HANDLE;
		pipeline.basePipelineIndex = 0;
		res = vkCreateComputePipelines(pDevice->_dev, pDevice->GetPipelineCache(), 1, &pipeline, NULL, &m_pipeline);
		assert(res == VK_SUCCESS);
	}
	void PostProcCS::OnDestroy()
	{
		vkDestroyPipeline(m_pDevice->_dev, m_pipeline, nullptr);
		vkDestroyPipelineLayout(m_pDevice->_dev, m_pipelineLayout, nullptr);
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

	void SkyDomeProc::OnCreate(cxDevice* pDevice, VkRenderPass renderPass, UploadHeap* pUploadHeap, VkFormat outFormat, ResourceViewHeaps* pResourceViewHeaps, buffer_ring_cx* pDynamicBufferRing, static_buffer_pool_cx* pStaticBufferPool, VkSampleCountFlagBits sampleDescCount)
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

		vkDestroyDescriptorSetLayout(m_pDevice->_dev, m_descriptorLayout, NULL);
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

	void SkyDome::OnCreate(cxDevice* pDevice, VkRenderPass renderPass, UploadHeap* pUploadHeap, VkFormat outFormat, ResourceViewHeaps* pResourceViewHeaps, buffer_ring_cx* pDynamicBufferRing, static_buffer_pool_cx* pStaticBufferPool, const char* pDiffuseCubemap, const char* pSpecularCubemap, VkSampleCountFlagBits sampleDescCount)
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
			/*		VkResult res = vkCreateSampler(pDevice->_dev, &info, NULL, &m_samplerDiffuseCube);
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
			//VkResult res = vkCreateSampler(pDevice->_dev, &info, NULL, &m_samplerSpecularCube);
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
		vkDestroyDescriptorSetLayout(m_pDevice->_dev, m_descriptorLayout, NULL);

		//vkDestroySampler(m_pDevice->_dev, m_samplerDiffuseCube, nullptr);
		//vkDestroySampler(m_pDevice->_dev, m_samplerSpecularCube, nullptr);

		vkDestroyImageView(m_pDevice->_dev, m_CubeDiffuseTextureView, NULL);
		vkDestroyImageView(m_pDevice->_dev, m_CubeSpecularTextureView, NULL);

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
	void TAA::OnCreate(cxDevice* pDevice, ResourceViewHeaps* pResourceViewHeaps, static_buffer_pool_cx* pStaticBufferPool, buffer_ring_cx* pDynamicBufferRing, bool sharpening)
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
			//res = vkCreateSampler(pDevice->_dev, &info, NULL, &m_samplers[0]);
			//assert(res == VK_SUCCESS);

			//res = vkCreateSampler(pDevice->_dev, &info, NULL, &m_samplers[1]);
			//assert(res == VK_SUCCESS);

			//res = vkCreateSampler(pDevice->_dev, &info, NULL, &m_samplers[3]);
			//(res == VK_SUCCESS);

			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			m_samplers[2] = pDevice->newSampler(&info);
			/*		res = vkCreateSampler(pDevice->_dev, &info, NULL, &m_samplers[2]);
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
		vkDestroyDescriptorSetLayout(m_pDevice->_dev, m_TaaDescriptorSetLayout, NULL);

		m_Sharpen.OnDestroy();
		m_Post.OnDestroy();
		vkDestroyDescriptorSetLayout(m_pDevice->_dev, m_SharpenDescriptorSetLayout, NULL);

		//for (int i = 0; i < 4; i++)
		//	vkDestroySampler(m_pDevice->_dev, m_samplers[i], nullptr);

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

			vkUpdateDescriptorSets(m_pDevice->_dev, 1, &write, 0, NULL);
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

		vkDestroyImageView(m_pDevice->_dev, m_TAABufferSRV, nullptr);
		vkDestroyImageView(m_pDevice->_dev, m_TAABufferUAV, nullptr);
		vkDestroyImageView(m_pDevice->_dev, m_HistoryBufferSRV, nullptr);
		//vkDestroyImageView(m_pDevice->_dev, m_HistoryBufferUAV, nullptr);
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

	void ToneMapping::OnCreate(cxDevice* pDevice, VkRenderPass renderPass, ResourceViewHeaps* pResourceViewHeaps, static_buffer_pool_cx* pStaticBufferPool, buffer_ring_cx* pDynamicBufferRing, uint32_t srvTableSize, const char* shaderSource)
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
			/*		VkResult res = vkCreateSampler(m_pDevice->_dev, &info, NULL, &m_sampler);
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

		//vkDestroySampler(m_pDevice->_dev, m_sampler, nullptr);

		vkDestroyDescriptorSetLayout(m_pDevice->_dev, m_descriptorSetLayout, NULL);
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

	void ToneMappingCS::OnCreate(cxDevice* pDevice, ResourceViewHeaps* pResourceViewHeaps, buffer_ring_cx* pDynamicBufferRing)
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

		vkDestroyDescriptorSetLayout(m_pDevice->_dev, m_descriptorSetLayout, NULL);
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


	// todo bloom

	void Bloom::OnCreate(
		cxDevice* pDevice,
		ResourceViewHeaps* pResourceViewHeaps,
		buffer_ring_cx* pConstantBufferRing,
		static_buffer_pool_cx* pStaticBufferPool,
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

			VkResult res = vkCreateDescriptorSetLayout(pDevice->_dev, &descriptor_layout, NULL, &m_descriptorSetLayout);
			assert(res == VK_SUCCESS);
		}

		// Create a Render pass that accounts for blending
		m_blendPass = SimpleColorBlendRenderPass(pDevice->_dev, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
			/*		VkResult res = vkCreateSampler(pDevice->_dev, &info, NULL, &m_sampler);
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
				VkResult res = vkCreateFramebuffer(m_pDevice->_dev, &fb_info, NULL, &m_mip[i].m_frameBuffer);
				assert(res == VK_SUCCESS);

				SetResourceName(m_pDevice->_dev, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_mip[i].m_frameBuffer, "BloomBlended");
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
				VkResult res = vkCreateFramebuffer(m_pDevice->_dev, &fb_info, NULL, &m_output.m_frameBuffer);
				assert(res == VK_SUCCESS);

				SetResourceName(m_pDevice->_dev, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_output.m_frameBuffer, "BloomOutput");
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
			vkDestroyImageView(m_pDevice->_dev, m_mip[i].m_SRV, NULL);
			vkDestroyImageView(m_pDevice->_dev, m_mip[i].m_RTV, NULL);
			vkDestroyFramebuffer(m_pDevice->_dev, m_mip[i].m_frameBuffer, NULL);
		}

		vkDestroyImageView(m_pDevice->_dev, m_output.m_RTV, NULL);
		vkDestroyFramebuffer(m_pDevice->_dev, m_output.m_frameBuffer, NULL);
	}

	void Bloom::OnDestroy()
	{
		for (int i = 0; i < BLOOM_MAX_MIP_LEVELS; i++)
		{
			m_pResourceViewHeaps->FreeDescriptor(m_mip[i].m_descriptorSet);
		}

		m_pResourceViewHeaps->FreeDescriptor(m_output.m_descriptorSet);

		m_blur.OnDestroy();

		vkDestroyDescriptorSetLayout(m_pDevice->_dev, m_descriptorSetLayout, NULL);
		//vkDestroySampler(m_pDevice->_dev, m_sampler, nullptr);

		m_blendAdd.OnDestroy();

		vkDestroyRenderPass(m_pDevice->_dev, m_blendPass, NULL);
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

				SetViewportAndScissor(cmd_buf, 0, 0, m_Width >> (i - 1), m_Height >> (i - 1), false);

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

				SetViewportAndScissor(cmd_buf, 0, 0, m_Width * 2, m_Height * 2, false);

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
		cxDevice* pDevice,
		ResourceViewHeaps* pResourceViewHeaps,
		buffer_ring_cx* pConstantBufferRing,
		static_buffer_pool_cx* pStaticBufferPool,
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

			VkResult res = vkCreateDescriptorSetLayout(pDevice->_dev, &descriptor_layout, NULL, &m_descriptorSetLayout);
			assert(res == VK_SUCCESS);
		}

		// Create a Render pass that will discard the contents of the render target.
		//
		m_in = SimpleColorWriteRenderPass(pDevice->_dev, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
			/*		VkResult res = vkCreateSampler(pDevice->_dev, &info, NULL, &m_sampler);
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

	void BlurPS::OnCreateWindowSizeDependentResources(cxDevice* pDevice, uint32_t Width, uint32_t Height, Texture* pInput, int mipCount)
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
					VkResult res = vkCreateFramebuffer(m_pDevice->_dev, &fb_info, NULL, &m_horizontalMip[i].m_frameBuffer);
					assert(res == VK_SUCCESS);

					SetResourceName(m_pDevice->_dev, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_horizontalMip[i].m_frameBuffer, "BlurPSHorizontal");
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
					VkResult res = vkCreateFramebuffer(m_pDevice->_dev, &fb_info, NULL, &m_verticalMip[i].m_frameBuffer);
					assert(res == VK_SUCCESS);

					SetResourceName(m_pDevice->_dev, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_verticalMip[i].m_frameBuffer, "BlurPSVertical");
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
			vkDestroyImageView(m_pDevice->_dev, m_horizontalMip[i].m_SRV, NULL);
			vkDestroyImageView(m_pDevice->_dev, m_horizontalMip[i].m_RTV, NULL);
			vkDestroyFramebuffer(m_pDevice->_dev, m_horizontalMip[i].m_frameBuffer, NULL);

			vkDestroyImageView(m_pDevice->_dev, m_verticalMip[i].m_SRV, NULL);
			vkDestroyImageView(m_pDevice->_dev, m_verticalMip[i].m_RTV, NULL);
			vkDestroyFramebuffer(m_pDevice->_dev, m_verticalMip[i].m_frameBuffer, NULL);
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
		vkDestroyDescriptorSetLayout(m_pDevice->_dev, m_descriptorSetLayout, NULL);
		//vkDestroySampler(m_pDevice->_dev, m_sampler, nullptr);
		vkDestroyRenderPass(m_pDevice->_dev, m_in, NULL);
	}

	void BlurPS::Draw(VkCommandBuffer cmd_buf, int mipLevel)
	{
		SetPerfMarkerBegin(cmd_buf, "blur");

		SetViewportAndScissor(cmd_buf, 0, 0, m_Width >> mipLevel, m_Height >> mipLevel, false);

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

	void ColorConversionPS::OnCreate(cxDevice* pDevice, VkRenderPass renderPass, ResourceViewHeaps* pResourceViewHeaps, static_buffer_pool_cx* pStaticBufferPool, buffer_ring_cx* pDynamicBufferRing)
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
			//VkResult res = vkCreateSampler(m_pDevice->_dev, &info, NULL, &m_sampler);
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

		//vkDestroySampler(m_pDevice->_dev, m_sampler, nullptr);

		vkDestroyDescriptorSetLayout(m_pDevice->_dev, m_descriptorSetLayout, NULL);
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
		cxDevice* pDevice,
		ResourceViewHeaps* pResourceViewHeaps,
		buffer_ring_cx* pConstantBufferRing,
		static_buffer_pool_cx* pStaticBufferPool,
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

			VkResult res = vkCreateDescriptorSetLayout(pDevice->_dev, &descriptor_layout, NULL, &m_descriptorSetLayout);
			assert(res == VK_SUCCESS);
		}

		// In Render pass
		//
		m_in = SimpleColorWriteRenderPass(pDevice->_dev, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
			//VkResult res = vkCreateSampler(pDevice->_dev, &info, NULL, &m_sampler);
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
				VkResult res = vkCreateFramebuffer(m_pDevice->_dev, &fb_info, NULL, &m_mip[i].frameBuffer);
				assert(res == VK_SUCCESS);

				std::string ResourceName = "DownsamplePS";
				ResourceName += std::to_string(i);

				SetResourceName(m_pDevice->_dev, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_mip[i].frameBuffer, ResourceName.c_str());
			}
		}
	}

	void DownSamplePS::OnDestroyWindowSizeDependentResources()
	{
		for (int i = 0; i < m_mipCount; i++)
		{
			vkDestroyImageView(m_pDevice->_dev, m_mip[i].m_SRV, NULL);
			vkDestroyImageView(m_pDevice->_dev, m_mip[i].RTV, NULL);
			vkDestroyFramebuffer(m_pDevice->_dev, m_mip[i].frameBuffer, NULL);
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
		vkDestroyDescriptorSetLayout(m_pDevice->_dev, m_descriptorSetLayout, NULL);
		//vkDestroySampler(m_pDevice->_dev, m_sampler, nullptr);

		vkDestroyRenderPass(m_pDevice->_dev, m_in, NULL);
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
			SetViewportAndScissor(cmd_buf, 0, 0, m_Width >> (i + 1), m_Height >> (i + 1), false);

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



	void MagnifierPS::OnCreate(cxDevice* pDevice, ResourceViewHeaps* pResourceViewHeaps, buffer_ring_cx* pDynamicBufferRing, static_buffer_pool_cx* pStaticBufferPool, VkFormat outFormat, bool bOutputsToSwapchain /*= false*/)
	{
		m_pDevice = pDevice;
		m_pResourceViewHeaps = pResourceViewHeaps;
		m_pDynamicBufferRing = pDynamicBufferRing;
		m_bOutputsToSwapchain = bOutputsToSwapchain;
		InitializeDescriptorSets();
		//VK_FORMAT_R16G16B16A16_SFLOAT
		m_RenderPass = SimpleColorBlendRenderPass(m_pDevice->_dev, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, outFormat);
		CompileShaders(pStaticBufferPool, outFormat); // expects a non-null Render Pass!
	}


	void MagnifierPS::OnDestroy()
	{
		vkDestroyRenderPass(m_pDevice->_dev, m_RenderPass, NULL);
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
			VkDevice device = m_pDevice->_dev;
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
		VkDevice device = m_pDevice->_dev;
		vkDestroyImageView(device, m_ImageViewSrc, NULL);
		vkDestroyImageView(device, m_SRVOutput, NULL);
		vkDestroySampler(device, m_SamplerSrc, nullptr);
		if (!m_bOutputsToSwapchain)
		{
			vkDestroyImageView(device, m_RTVOutput, NULL);
			m_TexPassOutput.OnDestroy();
			vkDestroyFramebuffer(device, m_FrameBuffer, NULL);
		}
	}

	void MagnifierPS::CompileShaders(static_buffer_pool_cx* pStaticBufferPool, VkFormat outFormat)
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
			VkResult res = vkCreateSampler(m_pDevice->_dev, &info, NULL, &m_SamplerSrc);
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

		vkUpdateDescriptorSets(m_pDevice->_dev, _countof(SetWrites), SetWrites, 0, NULL);
	}

	void MagnifierPS::DestroyDescriptorSets()
	{
		m_pResourceViewHeaps->FreeDescriptor(m_DescriptorSet);
		vkDestroyDescriptorSetLayout(m_pDevice->_dev, m_DescriptorSetLayout, NULL);
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


	//--------------------------------------------------------------------------------------

	gdev_cx::gdev_cx()
	{}

	gdev_cx::~gdev_cx()
	{
		free_instance(inst);
	}
	void gdev_cx::init(const char* pApplicationName, const char* pEngineName)
	{
		inst = vkg::new_instance(pApplicationName, pEngineName);

		VkInstance instance = (VkInstance)inst;
		if (instance)
		{
			devs = get_devices(instance);
			if (devs.size() > 0)
			{
				devicelist.resize(devs.size());
				dev_name.reserve(devs.size());
				for (size_t i = 0; i < devs.size(); i++)
				{

				}
				for (auto& dt : devs) {
					dev_name.push_back(dt.name);

				}
			}
		}
	}
	void* gdev_cx::new_device(void* phy, void* dev0, const char* devname, void* hwnd)
	{
		dev_info_cx c[1] = {};
		c->inst = inst;
		c->phy = phy;
		c->vkdev = dev0;
		devinfo_x* px = 0;
		if (!dev0)
		{
			// 选择自定义显卡
			if (devname && *devname)
			{
				std::string pdn;
				for (size_t i = 0; i < devs.size(); i++)
				{
					pdn.assign(devs[i].name);
					if (pdn.find(devname) != std::string::npos)
					{
						c->phy = (VkPhysicalDevice)devs[i].phd;
						break;
					}
				}
			}
			if (!c->phy)
				c->phy = SelectPhysicalDevice(devs);

			for (size_t i = 0; i < devs.size(); i++)
			{
				if (devs[i].phd == c->phy)
				{
					auto& px0 = devicelist[i];
					if (!px0)
					{
						px0 = new devinfo_x();
					}
					px = px0;
					break;
				}
			}
		}
		if (px)
			Device_init(px, c, hwnd);
		return px;
	}
	dev_info_cx gdev_cx::get_devinfo(uint32_t idx)
	{
		dev_info_cx r = {};
		if (idx < devicelist.size())
		{
			auto dev = devicelist[idx];
			if (dev) {
				r.inst = dev->_instance;
				r.phy = dev->_physicaldevice;
				r.vkdev = dev->_device;
			}
		}
		return r;
	}

	// 实现
#if 1
	bool sampler_kt::operator==(const sampler_kt& other) const
	{
		return memcmp(&info, &other.info, sizeof(sampler_info_t)) == 0;
	}
	size_t SAKHash::operator()(const sampler_kt& k) const
	{
		return Hash_p((const size_t*)&k.info, sizeof(sampler_info_t), HASH_SEED);
	}

	cxObject::cxObject() {}
	cxObject::cxObject(int t) :obj_type(t)
	{}
	cxObject::~cxObject() {}
	int cxObject::get_release()
	{
		refcount--;
		return refcount;
	}
	void cxObject::retain()
	{
		refcount++;
	}
	cxDevice::cxDevice() :cxObject(OBJ_DEVICE)
	{
		s_shaderCache = new Cache<VkShaderModule>();
	}
	cxDevice::~cxDevice()
	{
		if (s_shaderCache)delete s_shaderCache; s_shaderCache = 0;
		free_devinfo(d);
	}

	void cxDevice::set_device(void* dev, uint32_t idx)
	{
		_dev = (VkDevice)dev;
		_queue_family_index = idx;
		//assert(_dev && _queue);
		CreatePipelineCache();
	}

	// Compile a GLSL or a HLSL, will cache binaries to disk
	//
	VkResult cxDevice::VKCompile(ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const char* pshader, const char* pShaderEntryPoint, const char* shaderCompilerParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader)
	{
		VkResult res = VK_SUCCESS;
		//compute hash
		size_t hash;
		size_t pslen = strlen(pshader);
		auto sf = hz::genfn(GetShaderCompilerLibDir() + "\\");
		hash = HashShaderString(sf.c_str(), pshader, HASH_SEED);
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
		if (s_shaderCache->CacheMiss(hash, &pShader->module))
#endif
		{
			auto strk = format("%p", hash);
			print_time Pt("new shaderModule " + strk, 1);
			char* SpvData = NULL;
			size_t SpvSize = 0;
#ifdef USE_SPIRV_FROM_DISK
			std::string filenameSpv = hz::genfn(format("%s\\%p.spv", GetShaderCompilerCacheDir().c_str(), hash));
			if (readfile(filenameSpv.c_str(), &SpvData, &SpvSize, true) == false)
#endif
			{
				print_time Pt("Compile Pipeline " + strk, 1);
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
			CreateModule(_dev, SpvData, SpvSize, &pShader->module);
			if (!pShader->module) {
				assert(!pShader->module);
			}
#ifdef USE_MULTITHREADED_CACHE
			s_shaderCache->UpdateCache(hash, &pShader->module);
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

	VkResult cxDevice::VKCompileFromString(ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const char* pShaderCode, const char* pShaderEntryPoint, const char* shaderCompilerParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader)
	{
		assert(strlen(pShaderCode) > 0);
		VkResult res = VKCompile(sourceType, shader_type, pShaderCode, pShaderEntryPoint, shaderCompilerParams, pDefines, pShader);
		assert(res == VK_SUCCESS);
		return res;
	}
	// pExtraParams
	VkResult cxDevice::VKCompileFromFile(const VkShaderStageFlagBits shader_type, const char* pFilename, const char* pShaderEntryPoint, const char* shaderCompilerParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader)
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
		{
			assert(!"Can't tell shader type from its extension");
			return VK_NOT_READY;
		}

		//append path
		auto fullpath = hz::genfn(GetShaderCompilerLibDir() + std::string("/") + pFilename);

		if (readfile(fullpath.c_str(), &pShaderCode, &size, false))
		{
			VkResult res = VKCompileFromString(sourceType, shader_type, pShaderCode, pShaderEntryPoint, shaderCompilerParams, pDefines, pShader);
			SetResourceName(_dev, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)pShader->module, pFilename);
			return res;
		}

		return VK_NOT_READY;
	}

	void cxDevice::CreatePipelineCache()
	{
		if (!_dev || _pipelineCache)
			return;
		VkPipelineCacheCreateInfo pipelineCache;
		pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		pipelineCache.pNext = NULL;
		pipelineCache.initialDataSize = 0;
		pipelineCache.pInitialData = NULL;
		pipelineCache.flags = 0;
		VkResult res = vkCreatePipelineCache(_dev, &pipelineCache, NULL, &_pipelineCache);
		printf("vkCreatePipelineCache %d\n", (int)res);
		assert(res == VK_SUCCESS);
	}

	void cxDevice::DestroyPipelineCache()
	{
		if (_dev && _pipelineCache)
			vkDestroyPipelineCache(_dev, _pipelineCache, NULL);
	}
	VkPipelineCache cxDevice::GetPipelineCache()
	{
		return _pipelineCache;
	}

	VmaAllocator cxDevice::GetAllocator()
	{
		return d ? d->_hAllocator : nullptr;
	}

	VkPhysicalDeviceMemoryProperties cxDevice::GetPhysicalDeviceMemoryProperties()
	{
		return d->_memoryProperties;
	}
	VkPhysicalDeviceLimits* cxDevice::get_limits()
	{
		return &d->_deviceProperties.limits;
	}

	VkSampler cxDevice::newSampler(const sampler_info_t* pCreateInfo) {
		sampler_kt k = { *pCreateInfo };
		auto kt = *pCreateInfo;
		auto& p = _samplers[k];
		if (!p && _dev)
		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.flags = 0;
			info.magFilter = (VkFilter)kt.magFilter;
			info.minFilter = (VkFilter)kt.minFilter;
			info.addressModeU = (VkSamplerAddressMode)kt.addressModeU;
			info.addressModeV = (VkSamplerAddressMode)kt.addressModeV;
			info.addressModeW = (VkSamplerAddressMode)kt.addressModeW;
			info.mipLodBias = kt.mipLodBias;
			info.maxAnisotropy = kt.maxAnisotropy;
			info.compareOp = (VkCompareOp)kt.compareOp;
			info.minLod = kt.minLod;
			info.maxLod = kt.maxLod;
			info.borderColor = (VkBorderColor)kt.borderColor;
			info.mipmapMode = kt.mipmapMode ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
			info.anisotropyEnable = kt.anisotropyEnable ? VK_TRUE : VK_FALSE;
			info.compareEnable = kt.compareEnable ? VK_TRUE : VK_FALSE;
			info.unnormalizedCoordinates = kt.unnormalizedCoordinates ? VK_TRUE : VK_FALSE;
			vkCreateSampler(_dev, &info, 0, &p);
		}
		return p;
	}

	VkSampler cxDevice::newSampler(const VkSamplerCreateInfo* pCreateInfo)
	{
		VkSampler p = {};
		sampler_info_t ki = {};
		if (pCreateInfo)
		{
			ki.magFilter = pCreateInfo->magFilter;

		}
		return p;
	}



	void cxDevice::SetDescriptorSet(uint32_t index, VkImageView imageView, VkImageLayout imageLayout, VkSampler pSampler, VkDescriptorSet descriptorSet)
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

		vkUpdateDescriptorSets(_dev, 1, &write, 0, NULL);
	}

	void cxDevice::SetDescriptorSet(uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkImageLayout imageLayout, VkSampler pSampler, VkDescriptorSet descriptorSet)
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

		vkUpdateDescriptorSets(_dev, 1, &write, 0, NULL);
	}

	void cxDevice::SetDescriptorSet(uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		SetDescriptorSet(index, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
	}

	void cxDevice::SetDescriptorSet(uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		SetDescriptorSet(index, descriptorsCount, imageViews, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
	}

	void cxDevice::SetDescriptorSetForDepth(uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		SetDescriptorSet(index, imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
	}

	void cxDevice::SetDescriptorSet(uint32_t index, VkImageView imageView, VkDescriptorSet descriptorSet)
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

		vkUpdateDescriptorSets(_dev, 1, &write, 0, NULL);
	}

	void cxDevice::SetDescriptorSet1(VkBuffer buffer, int index, uint32_t pos, uint32_t size, VkDescriptorSet descriptorSet, uint32_t dt)
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

		vkUpdateDescriptorSets(_dev, 1, &write, 0, NULL);
	}

	VkDescriptorSetLayout cxDevice::newDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding)
	{
		VkDescriptorSetLayout descSetLayout = {};
		VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
		descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_layout.pNext = NULL;
		descriptor_layout.bindingCount = (uint32_t)pDescriptorLayoutBinding->size();
		descriptor_layout.pBindings = pDescriptorLayoutBinding->data();

		VkResult res = vkCreateDescriptorSetLayout(_dev, &descriptor_layout, NULL, &descSetLayout);
		assert(res == VK_SUCCESS);
		return descSetLayout;
	}

	VkFramebuffer cxDevice::newFrameBuffer(VkRenderPass renderPass, const std::vector<VkImageView>* pAttachments, uint32_t Width, uint32_t Height)
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
		res = vkCreateFramebuffer(_dev, &fb_info, NULL, &frameBuffer);
		assert(res == VK_SUCCESS);

		SetResourceName(_dev, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)frameBuffer, "HelperCreateFrameBuffer");

		return frameBuffer;
	}








	cxCamera::cxCamera() :cxObject(OBJ_CAMERA)
	{}

	cxCamera::~cxCamera()
	{}
	void cxCamera::set_fov(float fovy, float aspect, float zNear, float zFar)
	{
		proj = glm::perspective(fovy, aspect, zNear, zFar);
	}
	void cxCamera::set_lookat(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up)
	{
		_eye = eye; _center = center; _up = up;
		view = glm::lookAt(eye, center, up);
	}

	glm::quat e2q(const glm::ivec2& r)
	{
		auto ry = glm::radians(glm::vec2(r));
		glm::vec3 EulerAngles(ry.x, ry.y, 0.0);// x=pitch; y=yaw； roll忽略
		glm::quat q = glm::quat(EulerAngles);
		return q;
	}
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
	void key_move(firstPerson_t* p, glm::vec3 direction, double deltaTime, float keySpeed) {
		float moveSpeed = deltaTime * keySpeed;
		auto z = p->worldUp * direction.z;
		// 第一人称相机计算 
		auto x = -quat_right(p->qt);	// 左右移动
		auto y = -quat_forward(p->qt);	// 前后移动
		//auto z0 = quat_up(qi); 
		auto npos = moveSpeed * (x * direction.x + y * direction.y + z);
		if (abs(direction.x) > 0 || abs(direction.y) > 0)
			npos.y *= p->fixheight;
		p->pos += npos;
	}
	void mouse_move(firstPerson_t* p, float deltaX, float deltaY, double deltaTime, bool mousedown, float xMouseSpeed, float yMouseSpeed) {
		if (abs(deltaX) > 0 || abs(deltaY) > 0)
		{
			auto sr = p->rota;
			// 角度配置
			p->rota.x += deltaY * xMouseSpeed;
			p->rota.y += deltaX * yMouseSpeed;
			//角度限制
			p->rota.y = glm::mod(p->rota.y, 360.0f);
			p->rota.x = glm::clamp(p->rota.x, -89.0f, 89.0f);
			p->src_qt = p->qt; // 保存当前四元数 
			p->dst_qt = e2q(p->rota);
			auto qd = glm::distance2(sr, p->rota);
		}
		p->qt = p->dst_qt;
		p->front = quat_forward(p->qt);
	}
	glm::mat4 get_view(firstPerson_t* p)
	{
		glm::vec3 tpos = p->pos;
		glm::vec3 camera_forward;
		glm::vec3 camera_right;
		glm::vec3 camera_up;
		//float cameraSmooth = 2.0f;	// 平滑因子  
		p->cameraPos = tpos;
		camera_forward = glm::normalize(p->front);
		camera_right = glm::normalize(glm::cross(camera_forward, p->worldUp));
		camera_up = glm::normalize(glm::cross(camera_right, camera_forward));
		//if (type == 0)
		//{
		//	p->cameraPos += camera_forward * p->cameraDistance + camera_up * p->cameraHeight;
		//}
		auto view0 = glm::lookAt(p->cameraPos, p->front + p->cameraPos, camera_up);
		return view0;
	}

	//键盘移动处理
	void cxCamera::keyMovement(glm::vec3 direction, double deltaTime) {
		if (type == 1)
		{
			key_move(&fp, direction, deltaTime, keySpeed);
			view = get_view(&fp);
		}
	}

	//鼠标移动处理
	void cxCamera::mouseMovement(float deltaX, float deltaY, double deltaTime, bool mousedown)
	{
		if (type == 1 && !mousedown)
		{
			return; // 非第一人称视角且鼠标未按下时不处理鼠标移动
		}
		mouse_move(&fp, deltaX, deltaY, deltaTime, mousedown, xMouseSpeed, yMouseSpeed);
		view = get_view(&fp);
	}


	cxFrame::cxFrame() :cxObject(OBJ_FRAME)
	{}

	cxFrame::~cxFrame()
	{}
	cxArray::cxArray() :cxObject(OBJ_ARRAY)
	{}
	cxArray::~cxArray()
	{}
	cxGeometry::cxGeometry() :cxObject(OBJ_GEOMETRY)
	{}

	cxGeometry::~cxGeometry()
	{}

	cxGroup::cxGroup() :cxObject(OBJ_GROUP)
	{}

	cxGroup::~cxGroup()
	{}
	cxInstance::cxInstance() :cxObject(OBJ_INSTANCE)
	{}
	cxInstance::~cxInstance()
	{}

	cxLight::cxLight() :cxObject(OBJ_LIGHT)
	{}
	cxLight::~cxLight()
	{}
	cxMaterial::cxMaterial() :cxObject(OBJ_MATERIAL)
	{}
	cxMaterial::~cxMaterial()
	{}
	cxPipeline::cxPipeline() :cxObject(OBJ_PIPELINE)
	{}
	cxPipeline::~cxPipeline()
	{}
	cxPipelineCS::cxPipelineCS() :cxObject(OBJ_PIPELINECS)
	{}
	cxPipelineCS::~cxPipelineCS()
	{}
	cxSampler::cxSampler() :cxObject(OBJ_SAMPLER)
	{}
	cxSampler::~cxSampler()
	{}
	cxSurface::cxSurface() :cxObject(OBJ_SURFACE)
	{}
	cxSurface::~cxSurface()
	{}
	cxRenderer::cxRenderer() :cxObject(OBJ_RENDERER)
	{}
	cxRenderer::~cxRenderer()
	{}
	cxWorld::cxWorld() :cxObject(OBJ_WORLD)
	{}
	cxWorld::~cxWorld()
	{}



#endif // !1 实现


	aDevice new_device(void* vctx, void* phy, void* dev, const char* devname, void* hwnd) {
		aDevice p = nullptr;
		if (!vctx)return p;
		auto ctx = (gdev_cx*)vctx;
		auto c = new cxDevice();
		if (c)
		{
			auto d = (devinfo_x*)ctx->new_device(phy, dev, devname, hwnd);
			if (d && d->_device)
			{
				c->d = d;
				c->set_device(d->_device, d->_queue_family_index);
			}
			else {
				delete c;
				c = nullptr;
			}
			p = c;
		}
		return p;
	}
	aCamera new_aCamera(cxDevice* ctx, const char* type) {
		aCamera p = nullptr;
		auto c = new cxCamera();
		p = c;
		return p;
	}
	aArray new_aArray(cxDevice* ctx, const char* type) {
		aArray p = nullptr;
		auto c = new cxArray();
		p = c;
		return p;
	}

	aFrame new_aFrame(cxDevice* ctx, const char* type) {
		aFrame p = nullptr;
		auto c = new cxFrame();
		p = c;
		return p;
	}
	aGeometry new_aGeometry(cxDevice* ctx, const char* type) {
		aGeometry p = nullptr;
		auto c = new cxGeometry();
		p = c;
		return p;
	}
	aGroup new_aGroup(cxDevice* ctx, const char* type) {
		aGroup p = nullptr;
		auto c = new cxGroup();
		p = c;
		return p;
	}
	aInstance new_aInstance(cxDevice* ctx, const char* type) {
		aInstance p = nullptr;
		auto c = new cxInstance();
		p = c;
		return p;
	}
	aLight new_aLight(cxDevice* ctx, const char* type) {
		aLight p = nullptr;
		auto c = new cxLight();
		p = c;
		return p;
	}
	aMaterial new_aMaterial(cxDevice* ctx, const char* type) {
		aMaterial p = nullptr;
		auto c = new cxMaterial();
		p = c;
		return p;
	}
	aPipeline new_aPipeline(cxDevice* ctx, const char* type) {
		aPipeline p = nullptr;
		auto c = new cxPipeline();
		p = c;
		return p;
	}
	aPipelineCS new_aPipelineCS(cxDevice* ctx, const char* type) {
		aPipelineCS p = nullptr;
		auto c = new cxPipelineCS();
		p = c;
		return p;
	}
	aSampler new_aSampler(cxDevice* ctx, const char* type) {
		aSampler p = nullptr;
		auto c = new cxSampler();
		sampler_info_t info = {};
		ctx->newSampler(&info);
		p = c;
		return p;
	}
	aSurface new_aSurface(cxDevice* ctx, const char* type) {
		aSurface p = nullptr;
		auto c = new cxSurface();
		p = c;
		return p;
	}
	aRenderer new_aRenderer(cxDevice* ctx, const char* type) {
		aRenderer p = nullptr;
		auto c = new cxRenderer();
		p = c;
		return p;
	}
	aWorld new_aWorld(cxDevice* ctx, const char* type) {
		aWorld p = nullptr;
		auto c = new cxWorld();
		p = c;
		return p;
	}


	aObject new_object(aDevice* ctx0, int obj_type, const char* type) {
		auto t = (obj_type_e)obj_type;
		aObject p = nullptr;
		auto ctx = (cxDevice*)ctx0;
		switch (t) {
		case OBJ_CAMERA:	p = new_aCamera(ctx, type); break;
		case OBJ_ARRAY:		p = new_aArray(ctx, type); break;
		case OBJ_FRAME:		p = new_aFrame(ctx, type); break;
		case OBJ_GEOMETRY:	p = new_aGeometry(ctx, type); break;
		case OBJ_GROUP:		p = new_aGroup(ctx, type); break;
		case OBJ_INSTANCE:	p = new_aInstance(ctx, type); break;
		case OBJ_LIGHT:		p = new_aLight(ctx, type); break;
		case OBJ_MATERIAL:	p = new_aMaterial(ctx, type); break;
		case OBJ_PIPELINE:	p = new_aPipeline(ctx, type); break;
		case OBJ_PIPELINECS:p = new_aPipelineCS(ctx, type); break;
		case OBJ_SAMPLER:	p = new_aSampler(ctx, type); break;
		case OBJ_SURFACE:	p = new_aSurface(ctx, type); break;
		case OBJ_RENDERER:	p = new_aRenderer(ctx, type); break;
		case OBJ_WORLD:		p = new_aWorld(ctx, type); break;
		default:
			break;
		}
		return p;
	}
	void release(aObject obj) {
		auto c = (cxObject*)obj;
		if (c && c->get_release() == 0)
			delete c;
	}
	// 增加引用计数
	void retain(aObject obj) {
		auto c = (cxObject*)obj;
		if (c)
			c->retain();
	}
	// 设置参数
	void set_param(void* obj, const char* name, int data_type, void* data) {}
	// 取消参数
	void unset_param(aObject object, const char* name) {}
	void unset_allparams(aObject object) {}
	// 提交参数
	void commit_params(aObject object) {}
	// 获取对象支持的参数名
	size_t get_param_count(aObject object) {
		return 0;
	}
	const char** get_param_names(aObject object) {
		return 0;
	}
	// 获取参数值, data_type返回值类型
	void* get_param(aObject object, const char* name, int* data_type) {
		return nullptr;
	}
	// 获取属性
	int get_property(aObject object, const char* name, int data_type, void* mem, uint64_t size, int mask) {
		return 0;
	}
	// 提交帧渲染
	void render_frame(aFrame frame) {}
	// 帧渲染完成检查
	int frame_ready(aFrame frame, int wait_mask) {
		return 0;
	}

}
//!vkg

adevice3_t* new_gdev(const char* pApplicationName, const char* pEngineName) {
	auto ctx = new vkg::gdev_cx();
	adevice3_t* p = nullptr;
	if (ctx)
	{
		p = &ctx->ad_cb;
		ctx->init(pApplicationName, pEngineName);
		if (!ctx->inst)
		{
			delete ctx;
			ctx = 0;
			p = 0;
		}
	}
	if (p)
	{
		p->ctx = ctx;
		p->new_device = vkg::new_device;
		p->new_object = vkg::new_object;
		p->release = vkg::release;
		p->retain = vkg::retain;
		p->set_param = vkg::set_param;
		p->unset_param = vkg::unset_param;
		p->unset_allparams = vkg::unset_allparams;
		p->commit_params = vkg::commit_params;
		p->get_param_count = vkg::get_param_count;
		p->get_param_names = vkg::get_param_names;
		p->get_param = vkg::get_param;
		p->get_property = vkg::get_property;
		p->render_frame = vkg::render_frame;
		p->frame_ready = vkg::frame_ready;
	}
	return p;
}
// 删除渲染器
void free_gdev(adevice3_t* p) {
	if (p && p->ctx) {
		delete ((vkg::gdev_cx*)p->ctx);
	}
}

// !新渲染器


uint64_t vkg_get_ticks() {
	auto now = std::chrono::high_resolution_clock::now();
	// 转换为毫秒级时间戳
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	return ms;
}



// 生成四边形（XY平面，中心在原点，边长2） 
void generate_quad(std::vector<glm::vec3>& vertices, std::vector<unsigned int>& indices, std::vector<glm::vec3>& normals) {
	// 顶点：左下、右下、右上、左上 
	vertices = {
		glm::vec3(-1.0f, -1.0f, 0.0f),
		glm::vec3(1.0f, -1.0f, 0.0f),
		glm::vec3(1.0f, 1.0f, 0.0f),
		glm::vec3(-1.0f, 1.0f, 0.0f)
	};
	// 索引：两个三角形（0,1,2）、（0,2,3）
	indices = { 0, 1, 2, 0, 2, 3 };
	// 法线：正面朝Z轴正方向（0,0,1）
	normals = {
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)
	};
}
/*
圆柱（Cylinder）
描述：由底面（圆形）、顶面（圆形）、**侧面（矩形绕Y轴旋转）**组成。
参数：radius（底面半径）、height（高度）、segments（圆周分段数，越大越光滑）。
	*/
void generate_cylinder(float radius, float height, int segments,
	std::vector<glm::vec3>& vertices, std::vector<unsigned int>& indices, std::vector<glm::vec3>& normals) {
	vertices.clear();
	indices.clear();
	normals.clear();

	// 1. 底面（Y=0）
	vertices.push_back(glm::vec3(0.0f, 0.0f, 0.0f)); // 底面中心 
	for (int i = 0; i < segments; ++i) {
		float theta = 2.0f * glm::pi<float>() * static_cast<float>(i) / segments;
		float x = radius * glm::cos(theta);
		float z = radius * glm::sin(theta);
		vertices.push_back(glm::vec3(x, 0.0f, z)); // 底面圆周点
	}

	// 2. 顶面（Y=height）
	vertices.push_back(glm::vec3(0.0f, height, 0.0f)); // 顶面中心 
	for (int i = 0; i < segments; ++i) {
		float theta = 2.0f * glm::pi<float>() * static_cast<float>(i) / segments;
		float x = radius * glm::cos(theta);
		float z = radius * glm::sin(theta);
		vertices.push_back(glm::vec3(x, height, z)); // 顶面圆周点
	}

	// 3. 索引数组（三角扇+三角带） 
	// 底面三角扇（中心+圆周点） 
	for (int i = 1; i < segments; ++i) {
		indices.push_back(0);  // 底面中心 
		indices.push_back(i);
		indices.push_back(i + 1);
	}
	indices.push_back(0);
	indices.push_back(segments);
	indices.push_back(1);  // 闭合底面 

	// 顶面三角扇（中心+圆周点） 
	int top_center = segments + 1;
	for (int i = 0; i < segments; ++i) {
		indices.push_back(top_center);  // 顶面中心 
		indices.push_back(top_center + 1 + i);
		indices.push_back(top_center + 1 + (i + 1) % segments);
	}

	// 侧面三角带（底面圆周点+顶面圆周点） 
	for (int i = 0; i < segments; ++i) {
		int bottom = 1 + i;
		int top = top_center + 1 + i;
		indices.push_back(bottom);
		indices.push_back(top);
	}
	indices.push_back(1);
	indices.push_back(top_center + 1); // 闭合侧面 

	// 4. 法线数组 
	// 底面中心法线（垂直向下）
	normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
	// 底面圆周点法线（径向，归一化） 
	for (int i = 0; i < segments; ++i) {
		float theta = 2.0f * glm::pi<float>() * static_cast<float>(i) / segments;
		normals.push_back(glm::vec3(glm::cos(theta), 0.0f, glm::sin(theta)));
	}
	// 顶面中心法线（垂直向上）
	normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	// 顶面圆周点法线（径向，归一化） 
	for (int i = 0; i < segments; ++i) {
		float theta = 2.0f * glm::pi<float>() * static_cast<float>(i) / segments;
		normals.push_back(glm::vec3(glm::cos(theta), 0.0f, glm::sin(theta)));
	}
}

/*
圆锥（Cone）
描述：由底面（圆形）、**侧面（三角形带，连接底面圆周点与顶点）**组成。
参数：radius（底面半径）、height（高度）、segments（圆周分段数）。
	*/
void generate_cone(float radius, float height, int segments,
	std::vector<glm::vec3>& vertices, std::vector<unsigned int>& indices, std::vector<glm::vec3>& normals) {
	vertices.clear();
	indices.clear();
	normals.clear();

	// 1. 顶点（底面中心+底面圆周点+顶点）
	vertices.push_back(glm::vec3(0.0f, 0.0f, 0.0f)); // 底面中心 
	for (int i = 0; i < segments; ++i) {
		float theta = 2.0f * glm::pi<float>() * static_cast<float>(i) / segments;
		float x = radius * glm::cos(theta);
		float z = radius * glm::sin(theta);
		vertices.push_back(glm::vec3(x, 0.0f, z)); // 底面圆周点
	}
	vertices.push_back(glm::vec3(0.0f, height, 0.0f)); // 顶点（圆锥顶） 

	// 2. 索引数组（底面三角扇+侧面三角列表）
	// 底面三角扇
	for (int i = 1; i < segments; ++i) {
		indices.push_back(0);
		indices.push_back(i);
		indices.push_back(i + 1);
	}
	indices.push_back(0);
	indices.push_back(segments);
	indices.push_back(1);  // 闭合底面 

	// 侧面三角列表（每个三角形：顶点+底面点i+底面点i+1）
	int apex = segments + 1;
	for (int i = 0; i < segments; ++i) {
		int b = 1 + i;
		int c = 1 + (i + 1) % segments;
		indices.push_back(apex);
		indices.push_back(b);
		indices.push_back(c);
	}

	// 3. 法线数组（侧面法线=三角形叉乘，顶点法线=侧面法线平均）
	std::vector<glm::vec3> face_normals;
	for (int i = 0; i < segments; ++i) {
		glm::vec3 V = vertices[apex];
		glm::vec3 B_i = vertices[1 + i];
		glm::vec3 B_ip1 = vertices[1 + (i + 1) % segments];
		glm::vec3 normal = glm::normalize(glm::cross(B_i - V, B_ip1 - V));
		face_normals.push_back(normal);
	}

	// 底面中心法线（垂直向下）
	normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
	// 底面圆周点法线（相邻侧面法线平均）
	for (int i = 0; i < segments; ++i) {
		glm::vec3 normal = (face_normals[i] + face_normals[(i - 1 + segments) % segments]) / 2.0f;
		normals.push_back(glm::normalize(normal));
	}
	// 顶点法线（所有侧面法线平均）
	glm::vec3 apex_normal(0.0f);
	for (const auto& n : face_normals) apex_normal += n;
	normals.push_back(glm::normalize(apex_normal));
}
/*
曲线（三次贝塞尔曲线的管状体）
描述：将贝塞尔曲线扩展为管状体（中心轴为曲线，垂直于曲线的平面上生成圆形，连接相邻圆形点形成三角带）。
参数：control_points（三次贝塞尔控制点，4个）、radius（管状体半径）、curve_segments（曲线分段数）、tube_segments（管状体圆周分段数）。

	*/
struct bcp_pt {
	double x0, y0;    // P0
	double x1, y1;    // P1
	double x2, y2;    // P2
	double x3, y3;    // P3
};
// 计算贝塞尔曲线点坐标 
glm::vec3 bezier_point(const glm::vec3* ctrl, double t) {
	double u = 1.0 - t;
	glm::vec3 r = {};
	r.x = pow(u, 3) * ctrl[0].x + 3 * t * pow(u, 2) * ctrl[1].x + 3 * pow(t, 2) * u * ctrl[2].x + pow(t, 3) * ctrl[3].x;
	r.y = pow(u, 3) * ctrl[0].y + 3 * t * pow(u, 2) * ctrl[1].y + 3 * pow(t, 2) * u * ctrl[2].y + pow(t, 3) * ctrl[3].y;
	return r;
}

// 计算贝塞尔曲线导数 
glm::vec3 bezier_derivative(const glm::vec3* ctrl, double t) {
	double u = 1.0 - t;
	glm::vec3 r = {};
	r.x = 3 * pow(u, 2) * (ctrl[1].x - ctrl[0].x) + 6 * t * u * (ctrl[2].x - ctrl[1].x) + 3 * pow(t, 2) * (ctrl[3].x - ctrl[2].x);
	r.y = 3 * pow(u, 2) * (ctrl[1].y - ctrl[0].y) + 6 * t * u * (ctrl[2].y - ctrl[1].y) + 3 * pow(t, 2) * (ctrl[3].y - ctrl[2].y);
	return r;
}

void generate_curve_tube(const std::vector<glm::vec3>& control_points, float radius, int curve_segments, int tube_segments,
	std::vector<glm::vec3>& vertices, std::vector<unsigned int>& indices, std::vector<glm::vec3>& normals) {
	vertices.clear();
	indices.clear();
	normals.clear();

	// 1. 计算贝塞尔曲线上的点和切线 
	std::vector<glm::vec3> curve_points;
	std::vector<glm::vec3> tangents;
	for (int i = 0; i <= curve_segments; ++i) {
		float t = static_cast<float>(i) / curve_segments;
		curve_points.push_back(bezier_point(control_points.data(), t));
		tangents.push_back(glm::normalize(bezier_derivative(control_points.data(), t)));
	}

	// 2. 生成管状体顶点（每个曲线点处的圆形）
	for (int i = 0; i < curve_segments; ++i) {
		glm::vec3 point = curve_points[i];
		glm::vec3 tangent = tangents[i];
		// 计算垂直于切线的两个单位向量（u: 切线×上方向，v: 切线×u）
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		if (glm::dot(tangent, up) > 0.99f) up = glm::vec3(1.0f, 0.0f, 0.0f); // 避免切线与up平行 
		glm::vec3 u = glm::normalize(glm::cross(tangent, up));
		glm::vec3 v = glm::normalize(glm::cross(tangent, u));

		// 生成圆形上的点
		for (int j = 0; j < tube_segments; ++j) {
			float theta = 2.0f * glm::pi<float>() * static_cast<float>(j) / tube_segments;
			glm::vec3 offset = radius * (glm::cos(theta) * u + glm::sin(theta) * v);
			vertices.push_back(point + offset);
		}
	}

	// 3. 生成索引（三角带，连接相邻圆形点） 
	for (int i = 0; i < curve_segments - 1; ++i) {
		for (int j = 0; j < tube_segments; ++j) {
			int current = i * tube_segments + j;
			int next = (i + 1) * tube_segments + j;
			int next_j = (j + 1) % tube_segments;
			indices.push_back(current);
			indices.push_back(next);
			indices.push_back(next + next_j); // 三角形1: current→next→next_j 
			indices.push_back(current);
			indices.push_back(next + next_j);
			indices.push_back(current + next_j); // 三角形2: current→next_j→current_next_j 
		}
	}

	// 4. 法线数组（径向向量，顶点-曲线点归一化） 
	for (int i = 0; i < curve_segments; ++i) {
		glm::vec3 point = curve_points[i];
		for (int j = 0; j < tube_segments; ++j) {
			normals.push_back(glm::normalize(vertices[i * tube_segments + j] - point));
		}
	}
}
/*
棱角球（正二十面体细分）
描述：通过细分正二十面体生成球面（每个三角形面分成四个小三角形，边中点归一化到球面上）。
参数：subdivisions（细分次数，越大越光滑）
	*/
	// 正二十面体初始顶点（黄金比例φ=(1+√5)/2≈1.618） 
std::vector<glm::vec3> get_icosahedron_vertices() {
	float phi = (1.0f + glm::sqrt(5.0f)) / 2.0f;
	return {
		glm::vec3(0.0f, 1.0f, phi),
		glm::vec3(0.0f, -1.0f, phi),
		glm::vec3(0.0f, 1.0f, -phi),
		glm::vec3(0.0f, -1.0f, -phi),
		glm::vec3(1.0f, phi, 0.0f),
		glm::vec3(-1.0f, phi, 0.0f),
		glm::vec3(1.0f, -phi, 0.0f),
		glm::vec3(-1.0f, -phi, 0.0f),
		glm::vec3(phi, 0.0f, 1.0f),
		glm::vec3(-phi, 0.0f, 1.0f),
		glm::vec3(phi, 0.0f, -1.0f),
		glm::vec3(-phi, 0.0f, -1.0f)
	};
}

// 正二十面体初始面索引（每个面3个顶点）
std::vector<unsigned int> get_icosahedron_indices() {
	return {
		0, 11, 5,  0, 5, 1,  0, 1, 7,  0, 7, 10,  0, 10, 11,
		11, 10, 2, 10, 7, 6,  7, 1, 4,  1, 5, 3,   5, 11, 8,
		2, 10, 6,  6, 7, 4,  4, 1, 3,  3, 5, 8,   8, 11, 2
	};
}

// 细分函数：将每个三角形面分成四个小三角形 
void subdivide_icosahedron(std::vector<glm::vec3>& vertices, std::vector<unsigned int>& indices) {
	std::vector<glm::vec3> new_vertices = vertices;
	std::vector<unsigned int> new_indices;
	std::map<std::pair<unsigned int, unsigned int>, unsigned int> edge_midpoints;

	// 处理每个原三角形面
	for (size_t i = 0; i < indices.size(); i += 3) {
		unsigned int a = indices[i];
		unsigned int b = indices[i + 1];
		unsigned int c = indices[i + 2];

		// 获取边中点（避免重复计算）
		auto get_midpoint = [&](unsigned int u, unsigned int v) -> unsigned int {
			if (u > v) std::swap(u, v);
			auto key = std::make_pair(u, v);
			if (edge_midpoints.count(key))  return edge_midpoints[key];
			glm::vec3 mid = glm::normalize((vertices[u] + vertices[v]) / 2.0f);
			unsigned int idx = new_vertices.size();
			new_vertices.push_back(mid);
			edge_midpoints[key] = idx;
			return idx;
			};

		unsigned int ab = get_midpoint(a, b);
		unsigned int bc = get_midpoint(b, c);
		unsigned int ca = get_midpoint(c, a);

		// 新增四个小三角形 
		new_indices.insert(new_indices.end(), { a, ab, ca });
		new_indices.insert(new_indices.end(), { b, ab, bc });
		new_indices.insert(new_indices.end(), { c, bc, ca });
		new_indices.insert(new_indices.end(), { ab, bc, ca });
	}

	vertices = new_vertices;
	indices = new_indices;
}

// 生成棱角球（正二十面体细分）
void generate_geodesic_sphere(int subdivisions,
	std::vector<glm::vec3>& vertices, std::vector<unsigned int>& indices, std::vector<glm::vec3>& normals) {
	vertices = get_icosahedron_vertices();
	indices = get_icosahedron_indices();

	// 细分指定次数 
	for (int i = 0; i < subdivisions; ++i) {
		subdivide_icosahedron(vertices, indices);
	}

	// 法线：顶点坐标归一化（已在球面上）
	normals.reserve(vertices.size());
	for (const auto& v : vertices) {
		normals.push_back(glm::normalize(v));
	}
}
/*
经纬球（UV Sphere）
描述：通过**纬度（θ，-π/2到π/2）和经度（φ，0到2π）**划分球面，生成三角网格（每个四边形分成两个三角形）。
参数：radius（半径）、lat_segments（纬度分段数，如10段=11个纬度圈）、lon_segments（经度分段数，如20段=21个经度线）。
	*/

void generate_uv_sphere(float radius, int lat_segments, int lon_segments,
	std::vector<glm::vec3>& vertices, std::vector<unsigned int>& indices, std::vector<glm::vec3>& normals) {
	vertices.clear();
	indices.clear();
	normals.clear();

	// 1. 计算顶点坐标 
	for (int lat = 0; lat <= lat_segments; ++lat) {
		float theta = glm::pi<float>() * static_cast<float>(lat) / lat_segments - glm::pi<float>() / 2.0f; // 纬度（-π/2到π/2） 
		float sin_theta = glm::sin(theta);
		float cos_theta = glm::cos(theta);
		for (int lon = 0; lon <= lon_segments; ++lon) {
			float phi = 2.0f * glm::pi<float>() * static_cast<float>(lon) / lon_segments; // 经度（0到2π） 
			float sin_phi = glm::sin(phi);
			float cos_phi = glm::cos(phi);
			// 球面坐标转笛卡尔坐标 
			float x = radius * cos_theta * cos_phi;
			float y = radius * sin_theta;
			float z = radius * cos_theta * sin_phi;
			vertices.push_back(glm::vec3(x, y, z));
		}
	}

	// 2. 生成索引（每个四边形分成两个三角形）
	for (int lat = 0; lat < lat_segments; ++lat) {
		for (int lon = 0; lon < lon_segments; ++lon) {
			unsigned int a = lat * (lon_segments + 1) + lon;
			unsigned int b = (lat + 1) * (lon_segments + 1) + lon;
			unsigned int c = (lat + 1) * (lon_segments + 1) + (lon + 1);
			unsigned int d = lat * (lon_segments + 1) + (lon + 1);
			indices.insert(indices.end(), { a, b, c, a, c, d }); // 两个三角形 
		}
	}

	// 3. 法线数组（顶点坐标归一化，已在球面上）
	for (const auto& v : vertices) {
		normals.push_back(glm::normalize(v));
	}
}

void gpu_vk_test0()
{

}



