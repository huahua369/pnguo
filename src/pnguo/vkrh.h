#pragma once
/*

vk渲染器
Copyright (c) 华仔
188665600@qq.com

创建日期：2026-02-08


*/

namespace vkr {
#ifndef DEV_INFO_CX
#define DEV_INFO_CX
	struct dev_info_cx
	{
		void* inst = 0;
		void* phy = 0;
		void* vkdev = 0;
		uint32_t qFamIdx = 0;		// familyIndex
		uint32_t qIndex = 0;
	};
#endif
#ifndef DEVICE_INFO_T
#define DEVICE_INFO_T
	struct device_info_t
	{
		char name[256];
		void* phd;
	};
#endif
	struct layout_set_pool
	{
		VkDescriptorPool descriptorPool = {};
		VkDescriptorSetLayout descSetLayout = {};
		uint32_t max_count = 0;
		uint32_t alloc_count = 0;
		VkDevice dev = {};
#ifdef _HAS_VK_MULTITHREADING
		std::mutex _mutex;
#endif // _HAS_VK_MULTITHREADING

	};
	// 实例、设备
	class Device;
#ifndef HASH_SEED
#define HASH_SEED 2166136261
#endif
	class DefineList : public std::map<const std::string, std::string>
	{
	public:
		bool Has(const std::string& str) const;
		size_t Hash(size_t result = HASH_SEED) const;
		friend DefineList operator+(DefineList def1, const DefineList& def2);
		std::string to_string() const;
	};

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

	enum ShaderSourceType
	{
		SST_HLSL,
		SST_GLSL
	};
	struct sampler_kt {
		VkSamplerCreateInfo info = {};
		bool operator==(const sampler_kt& other) const;
	};
	struct SamplerKeyHash {
		size_t operator()(const sampler_kt& k) const;
	};

	class DeviceProperties;

	template<typename T>
	class Cache;

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

		Cache<VkShaderModule>* s_shaderCache = 0;
		VkPipelineCache m_pipelineCache = {};

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
		void OnCreate(dev_info_cx* d, bool cpuValidationLayerEnabled, bool gpuValidationLayerEnabled, void* pw, const char* spdname, std::vector<std::string>* pdnv);
		void OnCreateEx(VkInstance vulkanInstance, VkPhysicalDevice physicalDevice, VkDevice dev, void* pw, DeviceProperties* pDp);
		void OnDestroy();
		VkDevice GetDevice();
		VkQueue GetGraphicsQueue();
		uint32_t GetGraphicsQueueFamilyIndex();
		VkQueue GetPresentQueue();
		uint32_t GetPresentQueueFamilyIndex();
		VkQueue GetComputeQueue();
		uint32_t GetComputeQueueFamilyIndex();
		VkPhysicalDevice GetPhysicalDevice();
		VkSurfaceKHR GetSurface();
		void GetDeviceInfo(std::string* deviceName, std::string* driverVersion);
#ifdef USE_VMA
		VmaAllocator GetAllocator();
#endif
		VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties();
		VkPhysicalDeviceProperties GetPhysicalDeviceProperries();
		VkPhysicalDeviceSubgroupProperties GetPhysicalDeviceSubgroupProperties();

		bool IsFp16Supported();
		bool IsRT10Supported();
		bool IsRT11Supported();
		bool IsVRSTier1Supported();
		bool IsVRSTier2Supported();

		// pipeline cache
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



		// Does as the function name says and uses a cache
		VkResult VKCompileFromString(ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const char* pShaderCode, const char* pShaderEntryPoint, const char* pExtraParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader);
		VkResult VKCompileFromFile(const VkShaderStageFlagBits shader_type, const char* pFilename, const char* pShaderEntryPoint, const char* pExtraParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader);
	private:
		VkResult VKCompile(ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const char* pshader, const char* pShaderEntryPoint, const char* shaderCompilerParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader);

	};

	class TaskQueue
	{
	public:
		std::vector<std::jthread> workers;
		std::queue<std::function<void()>> tasks;
		std::mutex mtx;
		size_t w_count = 0;
		std::atomic_int rc = 0;
	public:
		TaskQueue(size_t count);
		~TaskQueue();
		void add(std::function<void()> task);
		void run(int num);
		void wait_stop();
	private:
	};

	uint32_t SizeOfFormat(VkFormat format);
	std::string format(const char* format, ...);
	void ExtDebugUtilsGetProcAddresses(VkDevice device);
	void SetResourceName(VkDevice device, VkObjectType objectType, uint64_t handle, const char* name);
	void SetPerfMarkerBegin(VkCommandBuffer cmd_buf, const char* name);
	void SetPerfMarkerEnd(VkCommandBuffer cmd_buf);


}
//!vkc
