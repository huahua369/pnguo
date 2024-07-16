/*
vk渲染器


*/
#include "pch1.h"
#include "vkrenderer.h"

#define USE_VMA

#include <vulkan/vulkan.h>
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#elif __ANDROID__
#include <vulkan/vulkan_android.h>
#endif
#ifdef USE_VMA
//#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#endif

#include <tinysdl3.h>

namespace cvk
{
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
	class DeviceProperties
	{
	public:
		VkPhysicalDevice m_physicaldevice;

		std::vector<const char*> m_device_extension_names;

		std::vector<VkExtensionProperties> m_deviceExtensionProperties;


		void* m_pNext = NULL;
	public:
		VkResult Init(VkPhysicalDevice physicaldevice);
		bool IsExtensionPresent(const char* pExtName);
		bool AddDeviceExtensionName(const char* deviceExtensionName);
		void* GetNext() { return m_pNext; }
		void SetNewNext(void* pNext) { m_pNext = pNext; }

		VkPhysicalDevice GetPhysicalDevice() { return m_physicaldevice; }
		void GetExtensionNamesAndConfigs(std::vector<const char*>* pDevice_extension_names);
	private:
	};
	class Device
	{
	public:
		Device();
		~Device();
		void OnCreate(dev_info_cx* d, bool cpuValidationLayerEnabled, bool gpuValidationLayerEnabled, void* pw
			, const char* spdname, std::vector<std::string>* pdnv);
		void SetEssentialInstanceExtensions(bool cpuValidationLayerEnabled, bool gpuValidationLayerEnabled, InstanceProperties* pIp);
		void SetEssentialDeviceExtensions(DeviceProperties* pDp);
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

		void CreateShaderCache() {};
		void DestroyShaderCache() {};

		void GPUFlush();

	private:
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

		bool m_usingValidationLayer = false;
		bool m_usingFp16 = false;
		bool m_rt10Supported = false;
		bool m_rt11Supported = false;
		bool m_vrs1Supported = false;
		bool m_vrs2Supported = false;
#ifdef USE_VMA
		VmaAllocator m_hAllocator = NULL;
#endif
	};

	bool memory_type_from_properties(VkPhysicalDeviceMemoryProperties& memory_properties, uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex);

#endif // 1
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
			m_device_extension_names.push_back(deviceExtensionName);
			return true;
		}

		//Trace("The device extension '%s' has not been found", deviceExtensionName);

		return false;
	}

	void  DeviceProperties::GetExtensionNamesAndConfigs(std::vector<const char*>* pDevice_extension_names)
	{
		for (auto& name : m_device_extension_names)
			pDevice_extension_names->push_back(name);
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
		InstanceProperties ip;
		ip.Init();
		SetEssentialInstanceExtensions(cpuValidationLayerEnabled, gpuValidationLayerEnabled, &ip);

		DeviceProperties dp;
		dp.Init((VkPhysicalDevice)d->phy);
		SetEssentialDeviceExtensions(&dp);

		// Create device
		OnCreateEx((VkInstance)d->inst, (VkPhysicalDevice)d->phy, (VkDevice)d->vkdev, pw, &dp);
	}

	void Device::SetEssentialInstanceExtensions(bool cpuValidationLayerEnabled, bool gpuValidationLayerEnabled, InstanceProperties* pIp)
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
#ifdef ExtCheckHDRInstanceExtensions
		ExtCheckHDRInstanceExtensions(pIp);
		ExtDebugUtilsCheckInstanceExtensions(pIp);
		if (cpuValidationLayerEnabled)
		{
			ExtDebugReportCheckInstanceExtensions(pIp, gpuValidationLayerEnabled);
		}
#endif
	}

	void Device::SetEssentialDeviceExtensions(DeviceProperties* pDp)
	{
#ifdef ExtRTCheckExtensions
		m_usingFp16 = ExtFp16CheckExtensions(pDp);
		ExtRTCheckExtensions(pDp, m_rt10Supported, m_rt11Supported);
		ExtVRSCheckExtensions(pDp, m_vrs1Supported, m_vrs2Supported);
		ExtCheckHDRDeviceExtensions(pDp);
		ExtCheckFSEDeviceExtensions(pDp);
		ExtCheckFreeSyncHDRDeviceExtensions(pDp);
#endif
		pDp->AddDeviceExtensionName(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		pDp->AddDeviceExtensionName(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
		pDp->AddDeviceExtensionName(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
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
	
		{

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
		}

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
		queue_info[0].queueCount = 1;
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
		shaderSubgroupExtendedType.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES_KHR;
		shaderSubgroupExtendedType.pNext = pDp->GetNext(); //used to be pNext of VkDeviceCreateInfo
		shaderSubgroupExtendedType.shaderSubgroupExtendedTypes = VK_TRUE;

		VkPhysicalDeviceRobustness2FeaturesEXT robustness2 = {};
		robustness2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
		robustness2.pNext = &shaderSubgroupExtendedType;
		robustness2.nullDescriptor = VK_TRUE;

		// to be able to bind NULL views
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
		physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physicalDeviceFeatures2.features = physicalDeviceFeatures;
		physicalDeviceFeatures2.pNext = &robustness2;
		if (dev)
		{
			m_device = dev;
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
			res = vkCreateDevice(m_physicaldevice, &device_info, NULL, &m_device);
			assert(res == VK_SUCCESS);
		}
		if (!m_device)return;

		auto cbr = vkGetInstanceProcAddr(m_instance, "vkCmdBeginRenderingKHR");
		auto cer = vkGetInstanceProcAddr(m_instance, "vkCmdEndRenderingKHR");
		auto cer1 = vkGetDeviceProcAddr(m_device, "vkCmdEndRenderingKHR");

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
		if (compute_queue_family_index != UINT32_MAX)
		{
			vkGetDeviceQueue(m_device, compute_queue_family_index, 0, &compute_queue);
		}

		// Init the extensions (if they have been enabled successfuly)
		//
#ifdef ExtDebugUtilsGetProcAddresses
		ExtDebugUtilsGetProcAddresses(m_device);
		ExtGetHDRFSEFreesyncHDRProcAddresses(m_instance, m_device);
#endif
	}

	void Device::GetDeviceInfo(std::string* deviceName, std::string* driverVersion)
	{
#define EXTRACT(v,offset, length) ((v>>offset) & ((1<<length)-1))
		* deviceName = m_deviceProperties.deviceName;
		*driverVersion = std::format("%i.%i.%i", EXTRACT(m_deviceProperties.driverVersion, 22, 10), EXTRACT(m_deviceProperties.driverVersion, 14, 8), EXTRACT(m_deviceProperties.driverVersion, 0, 16));
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

	void Device::OnDestroy()
	{
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

	//


}


vkdg_cx::vkdg_cx()
{
}

vkdg_cx::~vkdg_cx()
{
}



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

void DeviceShutdown(cvk::Device* dev)
{
	if (dev)
	{
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
vkdg_cx* new_vkdg(dev_info_cx* c)
{
	auto p = new vkdg_cx();
	if (c) {


#if 1		// System info
		struct SystemInfo
		{
			std::string mCPUName = "UNAVAILABLE";
			std::string mGPUName = "UNAVAILABLE";
			std::string mGfxAPI = "UNAVAILABLE";
		};
		SystemInfo  m_systemInfo;
		bool cpuvalid = false;
		bool gpuvalid = false;
		auto dev = new cvk::Device();
		dev->OnCreate(c, cpuvalid, gpuvalid, 0, 0, 0);
		dev->CreatePipelineCache();
		// Get system info
		std::string dummyStr;
		dev->GetDeviceInfo(&m_systemInfo.mGPUName, &dummyStr); // 2nd parameter is unused
		m_systemInfo.mCPUName = GetCPUNameString();
		m_systemInfo.mGfxAPI = "Vulkan";


#endif // 0


	}
	return p;
}

void free_vkdg(vkdg_cx* p)
{
	if (p)
	{
		DeviceShutdown((cvk::Device*)p->dev);
		delete p;
	}
}
