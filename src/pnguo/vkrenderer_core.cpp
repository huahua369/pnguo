/*

vk渲染器
Copyright (c) 华仔
188665600@qq.com

创建日期：205-12-05

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
#include <tiny_gltf.h>
#include <zlib.h>
#include <queue> 

#include "vkrenderer_core.h"

#ifdef min
#undef min
#undef max
#endif // min

// set创建管理  
namespace vkc
{
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


	// 实例、设备
#if 1

	struct inspd_t {
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
	};
	class InstanceProperties
	{
		std::vector<VkLayerProperties> _instanceLayerProperties;
		std::vector<VkExtensionProperties> _instanceExtensionProperties;

		std::vector<const char*> _instance_layer_names;
		std::vector<const char*> _instance_extension_names;
		void* _pNext = NULL;
	public:
		VkResult Init();
		bool AddInstanceLayerName(const char* instanceLayerName);
		bool AddInstanceExtensionName(const char* instanceExtensionName);
		void* GetNext() { return _pNext; }
		void SetNewNext(void* pNext) { _pNext = pNext; }

		void GetExtensionNamesAndConfigs(std::vector<const char*>* pInstance_layer_names, std::vector<const char*>* pInstance_extension_names);
	private:
		bool IsLayerPresent(const char* pExtName);
		bool IsExtensionPresent(const char* pExtName);
	};

	bool f_ExtDebugUtilsCheckInstanceExtensions(InstanceProperties* pDP);



	class DeviceProperties
	{
	public:
		VkPhysicalDevice _physicaldevice;

		std::set<std::string> _device_extension_names;

		std::vector<VkExtensionProperties> _deviceExtensionProperties;


		void* _pNext = NULL;
	public:
		VkResult Init(VkPhysicalDevice physicaldevice);
		bool IsExtensionPresent(const char* pExtName);
		bool has_extension(const char* pExtName);
		bool AddDeviceExtensionName(const char* deviceExtensionName);
		int AddDeviceExtensionName(std::vector<const char*> deviceExtensionName);
		void* GetNext() { return _pNext; }
		void SetNewNext(void* pNext) { _pNext = pNext; }

		VkPhysicalDevice GetPhysicalDevice() { return _physicaldevice; }
		void GetExtensionNamesAndConfigs(std::vector<const char*>* pDevice_extension_names);
	private:
	};
	class Device
	{
	public:
		VkInstance _instance = 0;
		VkDevice _device = 0;
		VkPhysicalDevice _physicaldevice = {};
		VkPhysicalDeviceMemoryProperties _memoryProperties = {};
		VkPhysicalDeviceProperties _deviceProperties = {};
		VkPhysicalDeviceProperties2 _deviceProperties2 = {};
		VkPhysicalDeviceSubgroupProperties _subgroupProperties = {};
		VkSurfaceKHR _surface = {};

		VkQueue present_queue = 0;
		uint32_t present_queue_family_index = 0;
		VkQueue graphics_queue = 0;
		uint32_t graphics_queue_family_index = 0;
		VkQueue compute_queue = 0;
		uint32_t compute_queue_family_index = 0;
		std::vector<VkSurfaceFormatKHR> _surfaceFormats;
		std::map<size_t, VkSampler> _samplers;

		PFN_vkCmdDrawMeshTasksEXT _vkCmdDrawMeshTasksEXT = { };
		PFN_vkCmdBeginRenderingKHR _vkCmdBeginRenderingKHR = {};
		PFN_vkCmdEndRenderingKHR _vkCmdEndRenderingKHR = {};
		PFN_vkCmdSetFrontFace _vkCmdSetFrontFace = {};

		bool _usingValidationLayer = false;
		bool _usingFp16 = false;
		bool _rt10Supported = false;
		bool _rt11Supported = false;
		bool _vrs1Supported = false;
		bool _vrs2Supported = false;
#ifdef USE_VMA
		VmaAllocator _hAllocator = NULL;
#endif
	public:
		Device();
		~Device();
		void OnCreate(dev_info_cx* d, bool cpuValidationLayerEnabled, bool gpuValidationLayerEnabled, void* pw
			, const char* spdname, std::vector<std::string>* pdnv);
		void OnCreateEx(VkInstance vulkanInstance, VkPhysicalDevice physicalDevice, VkDevice dev, void* pw, DeviceProperties* pDp);
		void OnDestroy();
		VkDevice GetDevice() { return _device; }
		VkQueue GetGraphicsQueue() { return graphics_queue; }
		uint32_t GetGraphicsQueueFamilyIndex() { return present_queue_family_index; }
		VkQueue GetPresentQueue() { return present_queue; }
		uint32_t GetPresentQueueFamilyIndex() { return graphics_queue_family_index; }
		VkQueue GetComputeQueue() { return compute_queue; }
		uint32_t GetComputeQueueFamilyIndex() { return compute_queue_family_index; }
		VkPhysicalDevice GetPhysicalDevice() { return _physicaldevice; }
		VkSurfaceKHR GetSurface() { return _surface; }
		void GetDeviceInfo(std::string* deviceName, std::string* driverVersion);
#ifdef USE_VMA
		VmaAllocator GetAllocator() { return _hAllocator; }
#endif
		VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() { return _memoryProperties; }
		VkPhysicalDeviceProperties GetPhysicalDeviceProperries() { return _deviceProperties; }
		VkPhysicalDeviceSubgroupProperties GetPhysicalDeviceSubgroupProperties() { return _subgroupProperties; }

		bool IsFp16Supported() { return _usingFp16; };
		bool IsRT10Supported() { return _rt10Supported; }
		bool IsRT11Supported() { return _rt11Supported; }
		bool IsVRSTier1Supported() { return _vrs1Supported; }
		bool IsVRSTier2Supported() { return _vrs2Supported; }

		// pipeline cache
		VkPipelineCache _pipelineCache = {};
		void CreatePipelineCache();
		void DestroyPipelineCache();
		VkPipelineCache GetPipelineCache();

		void CreateShaderCache() {};
		void DestroyShaderCache() {};

		void GPUFlush();
		VkSampler newSampler(const VkSamplerCreateInfo* pCreateInfo);

	};

	bool memory_type_from_properties(VkPhysicalDeviceMemoryProperties& memory_properties, uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex);

#endif // 1





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

	// device
#if 1

	bool InstanceProperties::IsLayerPresent(const char* pExtName)
	{
		return std::find_if(
			_instanceLayerProperties.begin(),
			_instanceLayerProperties.end(),
			[pExtName](const VkLayerProperties& layerProps) -> bool {
				return strcmp(layerProps.layerName, pExtName) == 0;
			}) != _instanceLayerProperties.end();
	}

	bool InstanceProperties::IsExtensionPresent(const char* pExtName)
	{
		return std::find_if(
			_instanceExtensionProperties.begin(),
			_instanceExtensionProperties.end(),
			[pExtName](const VkExtensionProperties& extensionProps) -> bool {
				return strcmp(extensionProps.extensionName, pExtName) == 0;
			}) != _instanceExtensionProperties.end();
	}

	VkResult InstanceProperties::Init()
	{
		// Query instance layers.
		//
		uint32_t instanceLayerPropertyCount = 0;
		VkResult res = vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, nullptr);
		_instanceLayerProperties.resize(instanceLayerPropertyCount);
		assert(res == VK_SUCCESS);
		if (instanceLayerPropertyCount > 0)
		{
			res = vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, _instanceLayerProperties.data());
			assert(res == VK_SUCCESS);
		}

		// Query instance extensions.
		//
		uint32_t instanceExtensionPropertyCount = 0;
		res = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropertyCount, nullptr);
		assert(res == VK_SUCCESS);
		_instanceExtensionProperties.resize(instanceExtensionPropertyCount);
		if (instanceExtensionPropertyCount > 0)
		{
			res = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropertyCount, _instanceExtensionProperties.data());
			assert(res == VK_SUCCESS);
		}

		return res;
	}

	bool InstanceProperties::AddInstanceLayerName(const char* instanceLayerName)
	{
		if (IsLayerPresent(instanceLayerName))
		{
			_instance_layer_names.push_back(instanceLayerName);
			return true;
		}

		//Trace("The instance layer '%s' has not been found\n", instanceLayerName);

		return false;
	}

	bool InstanceProperties::AddInstanceExtensionName(const char* instanceExtensionName)
	{
		if (IsExtensionPresent(instanceExtensionName))
		{
			_instance_extension_names.push_back(instanceExtensionName);
			return true;
		}

		//Trace("The instance extension '%s' has not been found\n", instanceExtensionName);

		return false;
	}

	void  InstanceProperties::GetExtensionNamesAndConfigs(std::vector<const char*>* pInstance_layer_names, std::vector<const char*>* pInstance_extension_names)
	{
		for (auto& name : _instance_layer_names)
			pInstance_layer_names->push_back(name);

		for (auto& name : _instance_extension_names)
			pInstance_extension_names->push_back(name);
	}

	bool DeviceProperties::IsExtensionPresent(const char* pExtName)
	{
		return std::find_if(
			_deviceExtensionProperties.begin(),
			_deviceExtensionProperties.end(),
			[pExtName](const VkExtensionProperties& extensionProps) -> bool {
				return strcmp(extensionProps.extensionName, pExtName) == 0;
			}) != _deviceExtensionProperties.end();
	}
	bool DeviceProperties::has_extension(const char* pExtName)
	{
		auto it = _device_extension_names.find(pExtName);
		return pExtName && *pExtName && it != _device_extension_names.end();
	}
	// 获取设备支持的扩展
	VkResult DeviceProperties::Init(VkPhysicalDevice physicaldevice)
	{
		_physicaldevice = physicaldevice;

		// Enumerate device extensions
		//
		uint32_t extensionCount;
		VkResult res = vkEnumerateDeviceExtensionProperties(physicaldevice, nullptr, &extensionCount, NULL);
		_deviceExtensionProperties.resize(extensionCount);
		res = vkEnumerateDeviceExtensionProperties(physicaldevice, nullptr, &extensionCount, _deviceExtensionProperties.data());

		return res;
	}

	bool DeviceProperties::AddDeviceExtensionName(const char* deviceExtensionName)
	{
		if (IsExtensionPresent(deviceExtensionName))
		{
			_device_extension_names.insert(deviceExtensionName);
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
				_device_extension_names.insert(name);
				r++;
			}
		}
		return r;
	}

	void  DeviceProperties::GetExtensionNamesAndConfigs(std::vector<const char*>* pDevice_extension_names)
	{
		for (auto& name : _device_extension_names)
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
		_usingFp16 = ExtFp16CheckExtensions(pDp);
		ExtRTCheckExtensions(pDp, _rt10Supported, _rt11Supported);
		ExtVRSCheckExtensions(pDp, _vrs1Supported, _vrs2Supported);
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


	Device::Device()
	{
	}

	Device::~Device()
	{
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
		if (!d || !d->inst || !d->phy)
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
					nd.phy = devs[0].phd;
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
				d = &nd;
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
			d->vkdev = _device;
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

		_instance = vulkanInstance;
		_physicaldevice = physicalDevice;

		// Get queue/memory/device properties
		//
		uint32_t queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(_physicaldevice, &queue_family_count, NULL);
		assert(queue_family_count >= 1);

		std::vector<VkQueueFamilyProperties> queue_props;
		queue_props.resize(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(_physicaldevice, &queue_family_count, queue_props.data());
		assert(queue_family_count >= 1);

		vkGetPhysicalDeviceMemoryProperties(_physicaldevice, &_memoryProperties);
		vkGetPhysicalDeviceProperties(_physicaldevice, &_deviceProperties);

		// Get subgroup properties to check if subgroup operations are supported
		//
		_subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
		_subgroupProperties.pNext = NULL;

		_deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		_deviceProperties2.pNext = &_subgroupProperties;

		vkGetPhysicalDeviceProperties2(_physicaldevice, &_deviceProperties2);

		{

#if defined(_WIN32)
			// Crate a Win32 Surface
			//
			VkWin32SurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.pNext = NULL;
			createInfo.hinstance = NULL;
			createInfo.hwnd = (HWND)pw;
			res = vkCreateWin32SurfaceKHR(_instance, &createInfo, NULL, &_surface);
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
		}
		if (_physicaldevice && _surface)
			get_surface_formats(_physicaldevice, _surface, _surfaceFormats);
		auto qfp = get_queue_fp(queue_props.data(), queue_family_count, VK_QUEUE_GRAPHICS_BIT);
		// Find a graphics device and a queue that can present to the above surface
		//
		graphics_queue_family_index = UINT32_MAX;
		present_queue_family_index = UINT32_MAX;
		for (uint32_t i = 0; i < queue_family_count; ++i)
		{
			if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				if (graphics_queue_family_index == UINT32_MAX) graphics_queue_family_index = i;

				VkBool32 supportsPresent;
				vkGetPhysicalDeviceSurfaceSupportKHR(_physicaldevice, i, _surface, &supportsPresent);
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
				vkGetPhysicalDeviceSurfaceSupportKHR(_physicaldevice, i, _surface, &supportsPresent);
				if (supportsPresent == VK_TRUE)
				{
					present_queue_family_index = (uint32_t)i;
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
		vkGetPhysicalDeviceFeatures2(_physicaldevice, &features);

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
			_device = dev;
		}
		else
		{
			VkDeviceCreateInfo device_info = {};
			device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			device_info.pNext = &physicalDeviceFeatures2;
			device_info.queueCreateInfoCount = 2;
			device_info.pQueueCreateInfos = queue_info;
			device_info.enabledExtensionCount = (uint32_t)extension_names.size();
			device_info.ppEnabledExtensionNames = device_info.enabledExtensionCount ? extension_names.data() : NULL;
			device_info.pEnabledFeatures = NULL;
			res = vkCreateDevice(_physicaldevice, &device_info, NULL, &_device);
			assert(res == VK_SUCCESS);
		}
		if (!_device)return;
		if (dr)
		{
			_vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(_device, "vkCmdBeginRenderingKHR");
			_vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(_device, "vkCmdEndRenderingKHR");
			_vkCmdSetFrontFace = (PFN_vkCmdSetFrontFace)vkGetDeviceProcAddr(_device, "vkCmdSetFrontFace");
			//if (!_vkCmdSetFrontFace)
			//	_vkCmdSetFrontFace = (PFN_vkCmdSetFrontFaceEXT)vkGetDeviceProcAddr(_device, "vkCmdSetFrontFaceEXT");
		}
		if (enabledMS)
		{
			_vkCmdDrawMeshTasksEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetDeviceProcAddr(_device, "vkCmdDrawMeshTasksEXT"));
		}

#ifdef USE_VMA
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = GetPhysicalDevice();
		allocatorInfo.device = GetDevice();
		allocatorInfo.instance = _instance;
		vmaCreateAllocator(&allocatorInfo, &_hAllocator);
#endif
		// create queues
		//
		vkGetDeviceQueue(_device, graphics_queue_family_index, 0, &graphics_queue);
		if (graphics_queue_family_index == present_queue_family_index)
		{
			present_queue = graphics_queue;
		}
		else
		{
			vkGetDeviceQueue(_device, present_queue_family_index, 0, &present_queue);
		}
		//if (compute_queue_family_index != UINT32_MAX)
		//{
		//	vkGetDeviceQueue(_device, compute_queue_family_index, 0, &compute_queue);
		//}

		// Init the extensions (if they have been enabled successfuly)
		//
		ExtDebugUtilsGetProcAddresses(_device);
#ifdef ExtGetHDRFSEFreesyncHDRProcAddresses
		ExtGetHDRFSEFreesyncHDRProcAddresses(_instance, _device);
#endif
	}

	void Device::GetDeviceInfo(std::string* deviceName, std::string* driverVersion)
	{
#define EXTRACT(v,offset, length) ((v>>offset) & ((1<<length)-1))
		* deviceName = _deviceProperties.deviceName;
		*driverVersion = vkc::format("%i.%i.%i", EXTRACT(_deviceProperties.driverVersion, 22, 10), EXTRACT(_deviceProperties.driverVersion, 14, 8), EXTRACT(_deviceProperties.driverVersion, 0, 16));
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
		VkResult res = vkCreatePipelineCache(_device, &pipelineCache, NULL, &_pipelineCache);
		printf("vkCreatePipelineCache %d\n", (int)res);
		assert(res == VK_SUCCESS);
	}

	void Device::DestroyPipelineCache()
	{
		vkDestroyPipelineCache(_device, _pipelineCache, NULL);
	}

	VkPipelineCache Device::GetPipelineCache()
	{
		return _pipelineCache;
	}

	void Device::OnDestroy()
	{
		for (auto& it : _samplers)
		{
			if (it.second)
			{
				vkDestroySampler(_device, it.second, NULL);
			}
		}
		_samplers.clear();
		if (_surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(_instance, _surface, NULL);
		}

#ifdef USE_VMA
		vmaDestroyAllocator(_hAllocator);
		_hAllocator = NULL;
#endif

		if (_device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(_device, nullptr);
			_device = VK_NULL_HANDLE;
		}
#if ExtDebugReportOnDestroy
		ExtDebugReportOnDestroy(_instance);

		//DestroyInstance(_instance);
#endif
		_instance = VK_NULL_HANDLE;
	}

	void Device::GPUFlush()
	{
		vkDeviceWaitIdle(_device);
	}

	VkSampler Device::newSampler(const VkSamplerCreateInfo* pCreateInfo) {
		auto h = Hash_p((const size_t*)pCreateInfo, sizeof(VkSamplerCreateInfo));
		auto& sampler = _samplers[h];
		if (!sampler)
		{
			vkCreateSampler(_device, pCreateInfo, 0, &sampler);
		}
		return sampler;
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

#endif // 1
	// !device
#if 1
	// 创建描述符堆
	bool new_set_layout(VkDevice pDevice, VkDescriptorSetLayoutBinding* layout_binding, uint32_t count, uint32_t maxcount, layout_set_pool* opt)
	{
		if (!pDevice || !layout_binding || count == 0 || maxcount == 0 || !opt)
			return false;
		VkDescriptorPoolSize tc[10] = {};
		VkDescriptorPoolSize tc1[10] = {};
		for (uint32_t i = 0; i < count; i++)
		{
			auto& it = layout_binding[i];
			tc[it.descriptorType].type = it.descriptorType;
			tc[it.descriptorType].descriptorCount += it.descriptorCount;
		}
		uint32_t n = 0;
		for (uint32_t i = 0; i < 10; i++)
		{
			if (tc[i].descriptorCount > 0)
			{
				tc1[n++] = tc[i];
			}
		}
		VkDescriptorPoolCreateInfo descriptor_pool = {};
		descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptor_pool.pNext = NULL;
		descriptor_pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		descriptor_pool.maxSets = maxcount;
		descriptor_pool.poolSizeCount = n;
		descriptor_pool.pPoolSizes = tc1;
		VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
		descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_layout.pNext = NULL;
		descriptor_layout.bindingCount = (uint32_t)count;
		descriptor_layout.pBindings = layout_binding;
		do
		{
			VkResult res = vkCreateDescriptorPool(pDevice, &descriptor_pool, NULL, &opt->descriptorPool);
			assert(res == VK_SUCCESS);
			if (res != VK_SUCCESS)
				break;
			res = vkCreateDescriptorSetLayout(pDevice, &descriptor_layout, NULL, &opt->descSetLayout);
			assert(res == VK_SUCCESS);
			if (res != VK_SUCCESS)
			{
				vkDestroyDescriptorPool(pDevice, opt->descriptorPool, NULL);
				break;
			}
			opt->alloc_count = 0;
			opt->max_count = maxcount;
			opt->dev = pDevice;
		} while (0);
		return true;
	}

	void free_layout_set_pool(layout_set_pool* p)
	{
		vkDestroyDescriptorSetLayout(p->dev, p->descSetLayout, NULL);
		vkDestroyDescriptorPool(p->dev, p->descriptorPool, NULL);
	}


	VkDescriptorSet alloc_descriptor(layout_set_pool* p)
	{
#ifdef _HAS_VK_MULTITHREADING
		std::lock_guard<std::mutex> lock(p->_mutex);
#endif
		VkDescriptorSetAllocateInfo alloc_info;
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.pNext = NULL;
		alloc_info.descriptorPool = p->descriptorPool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = &p->descSetLayout;
		VkDescriptorSet descriptorSet = {};
		VkResult res = vkAllocateDescriptorSets(p->dev, &alloc_info, &descriptorSet);
		assert(res == VK_SUCCESS);
		if (res == VK_SUCCESS)
		{
			p->alloc_count++;
		}
		else {
			descriptorSet = nullptr;
		}
		return descriptorSet;
	}

	void free_descriptor(layout_set_pool* p, VkDescriptorSet descriptorSet)
	{
		p->alloc_count--;
		vkFreeDescriptorSets(p->dev, p->descriptorPool, 1, &descriptorSet);
	}
	// pSamplers如果不为空时应该和size对应
	VkDescriptorSet alloc_descriptor_image(VkDevice pDevice, int size, const VkSampler* pSamplers, layout_set_pool* opt, uint32_t maxcount)
	{
		VkDescriptorSet dset = {};
		if (!pDevice || size < 1 || !opt)
			return dset;
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(size);
		for (int i = 0; i < size; i++)
		{
			layoutBindings[i].binding = i;
			layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBindings[i].descriptorCount = 1;
			layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBindings[i].pImmutableSamplers = (pSamplers != NULL) ? &pSamplers[i] : NULL;
		}
		if (maxcount < 1) { maxcount = 1; }
		if (new_set_layout(pDevice, layoutBindings.data(), layoutBindings.size(), maxcount, opt)) {
			dset = alloc_descriptor(opt);
		}
		return dset;
	}

	// pSamplers如果不为空时应该和descriptorCounts.size()对应
	VkDescriptorSet alloc_descriptor_images(VkDevice pDevice, std::vector<uint32_t>& descriptorCounts, const VkSampler* pSamplers, layout_set_pool* opt, uint32_t maxcount)
	{
		VkDescriptorSet dset = {};
		if (!pDevice || descriptorCounts.empty() || !opt)
			return dset;
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(descriptorCounts.size());
		for (int i = 0; i < descriptorCounts.size(); i++)
		{
			layoutBindings[i].binding = i;
			layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBindings[i].descriptorCount = descriptorCounts[i];
			layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBindings[i].pImmutableSamplers = (pSamplers != NULL) ? &pSamplers[i] : NULL;
		}
		if (maxcount < 1) { maxcount = 1; }
		if (new_set_layout(pDevice, layoutBindings.data(), layoutBindings.size(), maxcount, opt)) {
			dset = alloc_descriptor(opt);
		}
		return dset;
	}

	void updatedsets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies)
	{
		if (pDescriptorWrites && pDescriptorWrites->pBufferInfo) {
			if (!pDescriptorWrites->pBufferInfo->buffer)
				assert(pDescriptorWrites->pBufferInfo->offset == 0 && pDescriptorWrites->pBufferInfo->range == 0);
		}
		vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
	}
	void set_descriptorset(VkDevice device, VkBuffer buffer, int index, uint32_t size, VkDescriptorSet descriptorSet, VkDescriptorType dt)
	{
		VkDescriptorBufferInfo out = {};
		assert(buffer);
		out.buffer = buffer;
		out.offset = 0;
		out.range = size;
		VkWriteDescriptorSet write;
		write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = NULL;
		write.dstSet = descriptorSet;
		write.descriptorCount = 1;
		write.descriptorType = dt; ;
		write.pBufferInfo = &out;
		write.dstArrayElement = 0;
		write.dstBinding = index;
		updatedsets(device, 1, &write, 0, NULL);
	}
	void set_descriptorset1(VkDevice device, VkBuffer buffer, int index, uint32_t pos, uint32_t size, VkDescriptorSet descriptorSet, VkDescriptorType dt)
	{
		VkDescriptorBufferInfo out = {};
		out.buffer = buffer;
		out.offset = pos;
		out.range = size;
		assert(buffer);
		VkWriteDescriptorSet write;
		write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = NULL;
		write.dstSet = descriptorSet;
		write.descriptorCount = 1;
		write.descriptorType = dt;
		write.pBufferInfo = &out;
		write.dstArrayElement = 0;
		write.dstBinding = index;
		updatedsets(device, 1, &write, 0, NULL);
	}

	void set_descriptorset(VkDevice device, uint32_t index, VkImageView imageView, VkImageLayout imageLayout, VkSampler pSampler, VkDescriptorSet descriptorSet)
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
		updatedsets(device, 1, &write, 0, NULL);
	}
	void set_descriptorset(VkDevice device, uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkImageLayout imageLayout, VkSampler pSampler, VkDescriptorSet descriptorSet)
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

		updatedsets(device, 1, &write, 0, NULL);
	}

	void set_descriptorset(VkDevice device, uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		set_descriptorset(device, index, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
	}

	void set_descriptorset(VkDevice device, uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		set_descriptorset(device, index, descriptorsCount, imageViews, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
	}

	void set_descriptorset_depth(VkDevice device, uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		set_descriptorset(device, index, imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
	}
#endif
	// end set

}
//!vkc