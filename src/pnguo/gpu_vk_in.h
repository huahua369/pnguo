#pragma once


// 新渲染器声明  
namespace vkg {
	static const uint32_t MaxLightInstances = 4;
	static const uint32_t MaxShadowInstances = 4;
	const VkColorComponentFlags allBits = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	class dvk_texture;

	// 输出管线信息
	struct PBRPipe_t {
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkPipeline m_pipelineWireframe = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		ubotex_size_t binding = {}; // 输出的绑定号
		std::string pipe_kv;
		PBRPipe_t* next = 0;
		std::atomic_int st;
	};

	struct SystemInfo
	{
		std::string mCPUName = "UNAVAILABLE";
		std::string mGPUName = "UNAVAILABLE";
		std::string mGfxAPI = "UNAVAILABLE";
	};

	struct device_info_t
	{
		char name[256];
		void* phd;
	};
	struct devinfo_x {
		VkInstance _instance = 0;
		VkDevice _device = 0;
		VkPhysicalDevice _physicaldevice = {};
		VkSurfaceKHR _surface = {};
		VkPhysicalDeviceMemoryProperties _memoryProperties = {};
		VkPhysicalDeviceProperties _deviceProperties = {};
		VkPhysicalDeviceProperties2 _deviceProperties2 = {};
		VkPhysicalDeviceSubgroupProperties _subgroupProperties = {};
		std::string name;
		uint32_t graphics_queueFlags = 0;
		uint32_t _queue_family_index = 0;
		std::vector<VkQueue> _graphics_queues;
		std::vector<VkSurfaceFormatKHR> _surfaceFormats;
#ifdef USE_VMA
		VmaAllocator _hAllocator = NULL;
#endif
		bool is_newdevice = false;
	};
	void free_devinfo(devinfo_x* d);


	class gdev_cx
	{
	public:
		adevice3_t ad_cb = {};
		void* inst = 0;
		std::vector<device_info_t> devs;
		std::vector<devinfo_x*> devicelist;
		std::vector<std::string> dev_name;
		SystemInfo _systemInfo;
	public:
		gdev_cx();
		~gdev_cx();
		void init(const char* pApplicationName, const char* pEngineName);
		void* new_device(void* phy, void* dev, const char* devname, void* hwnd);
		dev_info_cx get_devinfo(uint32_t idx);
	private:

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

		void GetExtensionNamesAndConfigs(std::vector<const char*>* pInstance_layer_names, std::vector<const char*>* pInstance_extension_names, bool all);
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

	template<typename T>
	class Cache;

	class Sync
	{
		int m_count = 0;
		std::mutex m_mutex;
		std::condition_variable condition;
	public:
		Sync();
		~Sync();
		int Inc();

		int Dec();

		int Get();

		void Reset();

		void Wait();

	};
	// 对象实现头
#if 1
	/*
		Frame		: Camera(1:1)、World(1:1)、Renderer(1:1)
		World		: Instance(1:N)、Group(1:N)
		Instance	: Group(1:1) 一个Instance对象只能绑定一个Group。参数需要设置实例数量\矩阵等数据
		Group		: Surface(1:N)、Light(1:N)
		Surface		: Geometry(1:1)、Material(1:1)、或自定义pipeline
		Geometry	: 网格数据、transform、动画等属性
		FrameCS		: 计算帧,绑定pipelineCS。用于计算任务，可绑定输出到数组或纹理
		pipelineCS	: 计算管线, 绑定数组、纹理做输入
	*/

	class cxObject
	{
	public:
		int refcount = 1;
		int obj_type = 0;
	public:
		cxObject();
		cxObject(int t);
		virtual ~cxObject();
		int get_release();
		void retain();
	};
	struct sampler_kt {
		sampler_info_t info = {};
		bool operator==(const sampler_kt& other) const;
	};
	struct SAKHash {
		size_t operator()(const sampler_kt& k) const;
	};
	class cxDevice :public cxObject
	{
	public:
		// 逻辑设备
		VkDevice _dev = nullptr;
		VkQueue _queue = nullptr;
		uint32_t _queue_family_index = 0;
		devinfo_x* d = {};
		std::unordered_map<sampler_kt, VkSampler, SAKHash> _samplers;
		Cache<VkShaderModule>* s_shaderCache = 0;
		VkPipelineCache _pipelineCache = {};

	public:
		cxDevice();
		~cxDevice();
		// 设置vk设备和队列，必须调用此函数设置设备后才能使用其他对象
		void set_device(void* dev, uint32_t idx);
		// 创建采样器，内部会缓存相同参数的采样器对象
		VkSampler newSampler(const sampler_info_t* pCreateInfo);
		VkSampler newSampler(const VkSamplerCreateInfo* pCreateInfo);

		//创建信号
		void newSemaphore(VkSemaphore* semaphore, VkSemaphoreCreateInfo* semaphoreCreateInfo);
		//创建图像
		int64_t newImage(VkImageCreateInfo* imageinfo, VkImageViewCreateInfo* viewinfo, dvk_texture* texture);
		void destroyTexture(dvk_texture* texture);
		VkFence newFence(VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT);

		uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound);

		VkResult VKCompile(ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const char* pshader, const char* pShaderEntryPoint, const char* shaderCompilerParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader);
		VkResult VKCompileFromString(ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const char* pShaderCode, const char* pShaderEntryPoint, const char* shaderCompilerParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader);
		VkResult VKCompileFromFile(const VkShaderStageFlagBits shader_type, const char* pFilename, const char* pShaderEntryPoint, const char* shaderCompilerParams, const DefineList* pDefines, VkPipelineShaderStageCreateInfo* pShader);

		void CreatePipelineCache();
		void DestroyPipelineCache();
		VkPipelineCache GetPipelineCache();

#ifdef USE_VMA
		VmaAllocator GetAllocator();
#endif
		VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties();
		VkPhysicalDeviceLimits* get_limits();

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

	/* 第一人称：鼠标旋转、键盘移动、跳
	 上帝视角：
		移动：键盘wsad、鼠标边缘
		旋转：键盘qe、鼠标右键拖动
	*/
	struct firstPerson_t
	{
		glm::vec3 pos = { 0.0, 0.0, 0.0 };		//物体位置
		glm::vec3 rota = {};					//位置角度
		glm::vec3 worldUp = { 0.0, 1.0, 0.0 };	//y轴做世界坐标系法向量 
		glm::vec3 front = { 1.0, 1.0, 1.0 };	//相机前向向量

		// 相机参数 
		glm::vec3 cameraPos = glm::vec3(0.0f, 1.5f, 5.0f);       // 相机初始位置（玩家后方5米，上方1.5米） 
		float cameraDistance = 5.0f;                             // 相机与玩家的距离 
		float cameraHeight = 1.5f;                               // 相机垂直高度

		glm::quat qt = {};
		glm::quat src_qt = {};
		glm::quat dst_qt = {};

		float fixheight = 0;		// 0就是固定高度
	};

	class cxCamera :public cxObject
	{
	public:
		glm::mat4 proj = glm::mat4(1.0f), view = glm::mat4(1.0f);
		glm::vec3 _eye = {}, _center = {}, _up = {};
		firstPerson_t fp = {};
		float keySpeed = 5.0f;
		float xMouseSpeed = 0.51f;  //鼠标移动X速率
		float yMouseSpeed = 0.81f;  //鼠标移动Y速率
		int type = 1;				//1第一人称
	public:
		cxCamera();
		~cxCamera();
		void set_fov(float fovy, float aspect, float zNear, float zFar);
		void set_lookat(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);

		// 第一人称移动，direction为移动方向向量，deltaTime为帧间时间
		void keyMovement(glm::vec3 direction, double deltaTime);
		void mouseMovement(float deltaX, float deltaY, double deltaTime, bool mousedown);

	private:

	};
	class cxArray :public cxObject
	{
	public:
	public:
		cxArray();
		~cxArray();
	};
	class cxFrame :public cxObject
	{
	public:
	public:
		cxFrame();
		~cxFrame();

	private:

	};
	class cxGeometry :public cxObject
	{
	public:
	public:
		cxGeometry();
		~cxGeometry();

	private:

	};
	class cxGroup :public cxObject
	{
	public:
	public:
		cxGroup();
		~cxGroup();

	private:

	};
	class cxInstance :public cxObject
	{
	public:
	public:
		cxInstance();
		~cxInstance();
	};
	class cxLight :public cxObject
	{
	public:
	public:
		cxLight();
		~cxLight();
	};
	class cxMaterial :public cxObject
	{
	public:
	public:
		cxMaterial();
		~cxMaterial();
	};
	class cxPipeline :public cxObject
	{
	public:
	public:
		cxPipeline();
		~cxPipeline();
	};
	class cxPipelineCS :public cxObject
	{
	public:
	public:
		cxPipelineCS();
		~cxPipelineCS();
	};
	class cxSampler :public cxObject
	{
	public:
	public:
		cxSampler();
		~cxSampler();
	};
	class cxSurface :public cxObject
	{
	public:
	public:
		cxSurface();
		~cxSurface();
	};
	class cxRenderer :public cxObject
	{
	public:
	public:
		cxRenderer();
		~cxRenderer();
	};
	class cxWorld :public cxObject
	{
	public:
	public:
		cxWorld();
		~cxWorld();
	};


	class Ring
	{
	public:
		Ring() {}
		~Ring() {}
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
	class RingWithTabs
	{
	public:
		RingWithTabs() {}
		~RingWithTabs() {}
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
	// Dynamic/Static UBO
	class buffer_ring_cx
	{
	public:
		buffer_ring_cx() {}
		~buffer_ring_cx() {}
		VkResult OnCreate(cxDevice* pDevice, uint32_t numberOfBackBuffers, uint32_t memTotalSize, char* name = NULL);
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
		cxDevice* m_pDevice = nullptr;
		uint32_t        m_memTotalSize = 0;
		RingWithTabs    m_mem;
		char* m_pData = nullptr;
		VkBuffer        m_buffer = {};

#ifdef USE_VMA
		VmaAllocation   m_bufferAlloc = VK_NULL_HANDLE;
#else
		VkDeviceMemory  m_deviceMemory = VK_NULL_HANDLE;
#endif
	};
	class static_buffer_pool_cx
	{
	public:
		static_buffer_pool_cx() {}
		~static_buffer_pool_cx() {}
		VkResult OnCreate(cxDevice* pDevice, uint32_t totalMemSize, bool bUseVidMem, const char* name);
		void OnDestroy();
		// 分配IB/VB, 返回descriptor、pData
		bool AllocBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void** pData, VkDescriptorBufferInfo* pOut);
		// 分配IB/VB用pInitData填充, 返回descriptor
		bool AllocBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, const void* pInitData, VkDescriptorBufferInfo* pOut);
		// 更新到video mem
		void UploadData(VkCommandBuffer cmd_buf);
		// m_bUseVidMem是否释放上传堆
		void FreeUploadHeap();

	private:
		cxDevice* m_pDevice = nullptr;
		std::mutex       m_mutex = {};
		char* m_pData = nullptr;
		uint32_t         m_memOffset = 0;
		uint32_t         m_totalMemSize = 0;
		VkBuffer         m_buffer = {};
		VkBuffer         m_bufferVid = {};
#ifdef USE_VMA
		VmaAllocation    m_bufferAlloc = VK_NULL_HANDLE;
		VmaAllocation    m_bufferAllocVid = VK_NULL_HANDLE;
#else
		VkDeviceMemory   m_deviceMemory = VK_NULL_HANDLE;;
		VkDeviceMemory   m_deviceMemoryVid = VK_NULL_HANDLE;;
#endif 
		bool             m_bUseVidMem = true;
	};


	// set管理
	class ResourceViewHeaps
	{
	public:
		void OnCreate(cxDevice* pDevice, uint32_t cbvDescriptorCount, uint32_t srvDescriptorCount, uint32_t uavDescriptorCount, uint32_t samplerDescriptorCount);
		void OnDestroy();
		bool AllocDescriptor(VkDescriptorSetLayout descriptorLayout, VkDescriptorSet* pDescriptor);
		bool AllocDescriptor(int size, const VkSampler* pSamplers, VkDescriptorSetLayout* descriptorLayout, VkDescriptorSet* pDescriptor);
		bool AllocDescriptor(std::vector<uint32_t>& descriptorCounts, const VkSampler* pSamplers, VkDescriptorSetLayout* descriptorLayout, VkDescriptorSet* pDescriptor);
		bool CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding, VkDescriptorSetLayout* pDescSetLayout);
		bool CreateDescriptorSetLayoutAndAllocDescriptorSet(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding, VkDescriptorSetLayout* descriptorLayout, VkDescriptorSet* pDescriptor);
		void FreeDescriptor(VkDescriptorSet descriptorSet);
		cxDevice* get_dev();
	private:
		cxDevice* m_pDevice = 0;
		VkDescriptorPool m_descriptorPool;
		//std::mutex       _mutex;
		int              m_allocatedDescriptorCount = 0;
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
		UploadHeap() {}
		~UploadHeap() {}
		void OnCreate(cxDevice* pDevice, SIZE_T uSize);
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

		cxDevice* m_pDevice;

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


	class DDSLoader : public ImgLoader
	{
	public:
		DDSLoader();
		~DDSLoader();
		bool Load(const char* pFilename, float cutOff, IMG_INFO* pInfo);
		// after calling Load, calls to CopyPixels return each time a lower mip level 
		void CopyPixels(void* pDest, uint32_t stride, uint32_t width, uint32_t height);
	private:
#ifdef _WIN32
		HANDLE m_handle = INVALID_HANDLE_VALUE;
#endif
	};

	// Loads a JPEGs, PNGs, BMPs and any image the Windows Imaging Component can load.
	// It even applies some alpha scaling to prevent cutouts to fade away when lower mips are used.

	class WICLoader : public ImgLoader
	{
	public:
		WICLoader();
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


	ImgLoader* CreateImageLoader(const char* pFilename);


	class Texture
	{
	public:
		Texture();
		virtual ~Texture();
		virtual void OnDestroy();

		// load file into heap
		INT32 Init(cxDevice* pDevice, VkImageCreateInfo* pCreateInfo, const char* name = nullptr);
		INT32 InitRenderTarget(cxDevice* pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, VkImageUsageFlags usage, bool bUAV, const char* name = nullptr, VkImageCreateFlagBits flags = (VkImageCreateFlagBits)0);
		INT32 InitDepthStencil(cxDevice* pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, const char* name = nullptr);
		bool InitFromFile(cxDevice* pDevice, UploadHeap* pUploadHeap, const char* szFilename, bool useSRGB = false, VkImageUsageFlags usageFlags = 0, float cutOff = 1.0f);
		//bool InitFromData(cxDevice* pDevice, UploadHeap* uploadHeap, const IMG_INFO& header, const void* data, const char* name = nullptr, bool useSRGB = false);
		bool InitFromData(cxDevice* pDevice, UploadHeap* uploadHeap, IMG_INFO* header, const void* data, int dsize, const char* name, bool useSRGB);

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
		cxDevice* m_pDevice = NULL;
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

		VkImage CreateTextureCommitted(cxDevice* pDevice, UploadHeap* pUploadHeap, const char* pName, bool useSRGB = false, VkImageUsageFlags usageFlags = 0);
		void LoadAndUpload(cxDevice* pDevice, UploadHeap* pUploadHeap, ImgLoader* pDds, VkImage pTexture2D);
		void LoadAndUpload0(cxDevice* pDevice, UploadHeap* pUploadHeap, char* data, VkImage pTexture2D);
		bool isCubemap()const;
	};




#endif // 1


	// 对象实现头
#if 1

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

	struct SceneShadowInfo {
		Texture         ShadowMap;
		uint32_t        ShadowIndex;
		uint32_t        ShadowResolution;
		uint32_t        LightIndex;
		VkImageView     ShadowDSV;
		VkFramebuffer   ShadowFrameBuffer;
		VkImageView		ShadowSRV;
	};

	class GPUTimestamps
	{
	public:
		void OnCreate(cxDevice* pDevice, uint32_t numberOfBackBuffers);
		void OnDestroy();

		void GetTimeStamp(VkCommandBuffer cmd_buf, const char* label);
		void GetTimeStampUser(TimeStamp ts);
		void OnBeginFrame(VkCommandBuffer cmd_buf, std::vector<TimeStamp>* pTimestamp);
		void OnEndFrame();

	private:
		cxDevice* m_pDevice;
		double timestampPeriod = 0.0;
		const uint32_t MaxValuesPerFrame = 128;

		VkQueryPool        m_QueryPool;

		uint32_t m_frame = 0;
		uint32_t m_NumberOfBackBuffers = 0;

		std::vector<std::string> m_labels[5];
		std::vector<TimeStamp> m_cpuTimeStamps[5];
	};

	typedef uint32_t GBufferFlags;
	class GBuffer;

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
		cxDevice* m_pDevice;
		GBufferFlags                    m_flags;
		GBuffer* m_pGBuffer;
		VkRenderPass                    m_renderPass;
		VkFramebuffer                   m_frameBuffer;
		std::vector<VkClearValue>       m_clearValues;
	};

	class GBuffer
	{
	public:

		void OnCreate(cxDevice* pDevice, ResourceViewHeaps* pHeaps, const std::map<GBufferFlags, VkFormat>& m_formats, int sampleCount);
		void OnDestroy();

		void OnCreateWindowSizeDependentResources(/*SwapChain* pSwapChain,*/ uint32_t Width, uint32_t Height);
		void OnDestroyWindowSizeDependentResources();

		void GetAttachmentList(GBufferFlags flags, std::vector<VkImageView>* pAttachments, std::vector<VkClearValue>* pClearValues);
		VkRenderPass CreateRenderPass(GBufferFlags flags, bool bClear);

		//void GetCompilerDefines(DefineList& defines);
		VkSampleCountFlagBits  GetSampleCount() { return m_sampleCount; }
		cxDevice* GetDevice() { return m_pDevice; }

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
		cxDevice* m_pDevice;
		VkSampleCountFlagBits           m_sampleCount;
		GBufferFlags                    m_GBufferFlags;
		std::vector<VkClearValue>       m_clearValues;
		std::map<GBufferFlags, VkFormat> m_formats;
	};




	class PostProcPS
	{
	public:
		void OnCreate(
			cxDevice* pDevice,
			VkRenderPass renderPass,
			const std::string& shaderFilename,
			const std::string& shaderEntryPoint,
			const std::string& shaderCompilerParams,
			static_buffer_pool_cx* pStaticBufferPool,
			buffer_ring_cx* pDynamicBufferRing,
			VkDescriptorSetLayout descriptorSetLayout,
			VkPipelineColorBlendStateCreateInfo* pBlendDesc = NULL,
			VkSampleCountFlagBits sampleDescCount = VK_SAMPLE_COUNT_1_BIT
		);
		void OnDestroy();
		void UpdatePipeline(VkRenderPass renderPass, VkPipelineColorBlendStateCreateInfo* pBlendDesc = NULL, VkSampleCountFlagBits sampleDescCount = VK_SAMPLE_COUNT_1_BIT);
		void Draw(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo* pConstantBuffer, VkDescriptorSet descriptorSet = NULL);

	private:
		cxDevice* m_pDevice;
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
		void OnCreate(cxDevice* pDevice, VkRenderPass renderPass, UploadHeap* pUploadHeap, VkFormat outFormat, ResourceViewHeaps* pResourceViewHeaps, buffer_ring_cx* pDynamicBufferRing, static_buffer_pool_cx* pStaticBufferPool, const char* pDiffuseCubemap, const char* pSpecularCubemap, VkSampleCountFlagBits sampleDescCount);
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
		cxDevice* m_pDevice;

		ResourceViewHeaps* m_pResourceViewHeaps;

		Texture m_CubeDiffuseTexture;
		Texture m_CubeSpecularTexture;

		VkImageView m_CubeDiffuseTextureView;
		VkImageView m_CubeSpecularTextureView;

		VkSampler m_samplerDiffuseCube, m_samplerSpecularCube;

		VkDescriptorSet       m_descriptorSet;
		VkDescriptorSetLayout m_descriptorLayout;

		PostProcPS  m_skydome;

		buffer_ring_cx* m_pDynamicBufferRing = NULL;
		glm::vec4 default_specular = glm::vec4(0.5);	// 默认环境高光值
	};

#define BLURPS_MAX_MIP_LEVELS 12

	// Implements a simple separable gaussian blur

	class BlurPS
	{
	public:
		void OnCreate(
			cxDevice* pDevice,
			ResourceViewHeaps* pResourceViewHeaps,
			buffer_ring_cx* pConstantBufferRing,
			static_buffer_pool_cx* pStaticBufferPool,
			VkFormat format
		);
		void OnDestroy();

		void OnCreateWindowSizeDependentResources(cxDevice* pDevice, uint32_t Width, uint32_t Height, Texture* pInput, int mipCount);
		void OnDestroyWindowSizeDependentResources();

		void Draw(VkCommandBuffer cmd_buf, int mipLevel);
		void Draw(VkCommandBuffer cmd_buf);

	private:
		cxDevice* m_pDevice;

		ResourceViewHeaps* m_pResourceViewHeaps;
		buffer_ring_cx* m_pConstantBufferRing;

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
		cxDevice* m_pDevice = nullptr;
		ResourceViewHeaps* m_pResourceViewHeaps;
		buffer_ring_cx* m_pConstantBufferRing;
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
		void OnCreate(cxDevice* pDevice, ResourceViewHeaps* pHeaps, buffer_ring_cx* pConstantBufferRing, static_buffer_pool_cx* pResourceViewHeaps, VkFormat format);
		void OnDestroy();
		void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, Texture* pInput, int mipCount, Texture* pOutput);
		void OnDestroyWindowSizeDependentResources();
		void Draw(VkCommandBuffer cmd_buf);

		void Gui();

	};

	class ColorConversionPS
	{
	public:
		void OnCreate(cxDevice* pDevice, VkRenderPass renderPass, ResourceViewHeaps* pResourceViewHeaps, static_buffer_pool_cx* pStaticBufferPool, buffer_ring_cx* pDynamicBufferRing);
		void OnDestroy();

		void UpdatePipelines(VkRenderPass renderPass, DisplayMode displayMode);

		void Draw(VkCommandBuffer cmd_buf, VkImageView HDRSRV);

	private:
		cxDevice* m_pDevice;
		ResourceViewHeaps* m_pResourceViewHeaps;

		PostProcPS m_ColorConversion;
		buffer_ring_cx* m_pDynamicBufferRing = NULL;

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
		void OnCreate(cxDevice* pDevice, ResourceViewHeaps* pResourceViewHeaps, buffer_ring_cx* m_pConstantBufferRing, static_buffer_pool_cx* pStaticBufferPool, VkFormat outFormat);
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
		cxDevice* m_pDevice;
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

		static_buffer_pool_cx* m_pStaticBufferPool;
		ResourceViewHeaps* m_pResourceViewHeaps;
		buffer_ring_cx* m_pConstantBufferRing;

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
		void OnCreate(cxDevice* pDevice, const std::string& shaderFilename, const std::string& shaderEntryPoint, const std::string& shaderCompilerParams
			, VkDescriptorSetLayout descriptorSetLayout, uint32_t dwWidth, uint32_t dwHeight, uint32_t dwDepth, DefineList* userDefines = 0);
		void OnDestroy();
		void Draw(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo* pConstantBuffer, VkDescriptorSet descSet, uint32_t dispatchX, uint32_t dispatchY, uint32_t dispatchZ);

	private:
		cxDevice* m_pDevice;

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

		void OnCreate(cxDevice* pDevice, VkRenderPass renderPass, UploadHeap* pUploadHeap, VkFormat outFormat, ResourceViewHeaps* pResourceViewHeaps, buffer_ring_cx* pDynamicBufferRing, static_buffer_pool_cx* pStaticBufferPool, VkSampleCountFlagBits sampleDescCount);
		void OnDestroy();
		void Draw(VkCommandBuffer cmd_buf, SkyDomeProc::Constants constants);

	private:
		cxDevice* m_pDevice;

		ResourceViewHeaps* m_pResourceViewHeaps;

		VkDescriptorSet         m_descriptorSet;
		VkDescriptorSetLayout   m_descriptorLayout;

		PostProcPS  m_skydome;

		buffer_ring_cx* m_pDynamicBufferRing = NULL;
	};
	// todo taa
	class TAA
	{
	public:
		void OnCreate(cxDevice* pDevice, ResourceViewHeaps* pResourceViewHeaps, static_buffer_pool_cx* pStaticBufferPool, buffer_ring_cx* pDynamicBufferRing, bool sharpening);
		void OnDestroy();

		void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, GBuffer* pGBuffer);
		void OnDestroyWindowSizeDependentResources();

		void Draw(VkCommandBuffer cmd_buf);

	private:
		cxDevice* m_pDevice;
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
		void OnCreate(cxDevice* pDevice, VkRenderPass renderPass, ResourceViewHeaps* pResourceViewHeaps, static_buffer_pool_cx* pStaticBufferPool, buffer_ring_cx* pDynamicBufferRing, uint32_t srvTableSize = 1, const char* shaderSource = "Tonemapping.glsl");
		void OnDestroy();

		void UpdatePipelines(VkRenderPass renderPass);

		void Draw(VkCommandBuffer cmd_buf, VkImageView HDRSRV, float exposure, int toneMapper);

	protected:
		cxDevice* m_pDevice;
		ResourceViewHeaps* m_pResourceViewHeaps;

		PostProcPS m_toneMapping;
		buffer_ring_cx* m_pDynamicBufferRing = NULL;

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
		void OnCreate(cxDevice* pDevice, ResourceViewHeaps* pResourceViewHeaps, buffer_ring_cx* pDynamicBufferRing);
		void OnDestroy();

		void Draw(VkCommandBuffer cmd_buf, VkImageView HDRSRV, float exposure, int toneMapper, int width, int height);

	private:
		cxDevice* m_pDevice;
		ResourceViewHeaps* m_pResourceViewHeaps;

		PostProcCS m_toneMapping;
		buffer_ring_cx* m_pDynamicBufferRing = NULL;

		uint32_t              m_descriptorIndex;
		static const uint32_t s_descriptorBuffers = 10;

		VkDescriptorSet       m_descriptorSet[s_descriptorBuffers];
		VkDescriptorSetLayout m_descriptorSetLayout;

		struct ToneMappingConsts { float exposure; int toneMapper; };
	};

	class MagnifierPS
	{
	public:
		void OnCreate(
			cxDevice* pDevice
			, ResourceViewHeaps* pResourceViewHeaps
			, buffer_ring_cx* pDynamicBufferRing
			, static_buffer_pool_cx* pStaticBufferPool
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

		void CompileShaders(static_buffer_pool_cx* pStaticBufferPool, VkFormat outFormat);
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
		// DEVICE & MEMORY
		cxDevice* m_pDevice = nullptr;
		ResourceViewHeaps* m_pResourceViewHeaps = nullptr;
		buffer_ring_cx* m_pDynamicBufferRing = nullptr;

		// DESCRIPTOR SETS & LAYOUTS
		VkDescriptorSet m_DescriptorSet;
		VkDescriptorSetLayout m_DescriptorSetLayout;
		// RENDER PASSES & RESOURCE VIEWS
		VkSampler   m_SamplerSrc;
		VkImageView m_ImageViewSrc;

		VkRenderPass  m_RenderPass;
		VkFramebuffer m_FrameBuffer;
		VkImageView   m_RTVOutput;
		VkImageView   m_SRVOutput;

		Texture m_TexPassOutput;
		PostProcPS m_ShaderMagnify;
		bool m_bOutputsToSwapchain = false;
	};

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
	};

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
#endif
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
		cxDevice* _dev = nullptr;
		std::string _name;
		ImageLayoutBarrier _image_layout = ImageLayoutBarrier::SHADER_READ;
		void* user_data = 0;
		void* uimg = 0;
		void* mapped = nullptr;
		IMG_INFO* _header = 0;
		void* ad = 0;
		void* ad1 = 0;
		//pipeline_ptr_info* pipe = 0;
		yuv_info_t yuv = {};
		VkSamplerYcbcrConversion ycbcr_sampler_conversion = {};
	public:
		dvk_texture();
		dvk_texture(cxDevice* dev);

		~dvk_texture();
	};
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
			VkSemaphore semaphore1 = VK_NULL_HANDLE;
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

		cxDevice* _dev = nullptr;
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
	struct const_vk {
		// Create all the heaps for the resources views
		uint32_t cbvDescriptorCount = 2000;
		uint32_t srvDescriptorCount = 8000;
		uint32_t uavDescriptorCount = 10;
		uint32_t samplerDescriptorCount = 20;
		// Create a commandlist ring for the Direct queue
		uint32_t commandListsPerBackBuffer = 8;
		// Create a 'dynamic' constant buffer动态常量缓冲区大小
		uint32_t constantBuffersMemSize = 15 * 1024 * 1024;
		// Create a 'static' pool for vertices and indices 静态顶点/索引缓冲区大小
		uint32_t staticGeometryMemSize = (10 * 128) * 1024 * 1024;
		// Create a 'static' pool for vertices and indices in system memory静态几何缓冲区大小
		uint32_t systemGeometryMemSize = 32 * 1024;

		// Quick helper to upload resources, it has it's own commandList and uses suballocation.
		uint32_t uploadHeapMemSize = (uint32_t)1000 * 1024 * 1024;
	};



	struct scene_state {
		// POST PROCESS CONTROLS 
		int   SelectedTonemapperIndex = 0;// 0-5
		float Exposure = 1.0;
		// APP/SCENE CONTROLS 
		float IBLFactor = 1.0;
		float EmissiveFactor = 1.0;
		int   SelectedSkydomeTypeIndex = 0; // 0-1 

		PassParameters MagnifierParams = {};
		int   LockedMagnifiedScreenPositionX;
		int   LockedMagnifiedScreenPositionY;

		enum class WireframeMode : int
		{
			WIREFRAME_MODE_OFF = 0,
			WIREFRAME_MODE_FACE = 1,
			WIREFRAME_MODE_SHADED = 2,
			WIREFRAME_MODE_SOLID_COLOR = 4,
			WIREFRAME_MODE_SOLID_COLOR_FACE = 5,
		};
		int WireframeMode = 0;
		float WireframeColor[3] = { 1.0,1.0,1.0 };
		bool  bUseTAA = true;
		bool  bTAAsharpening = true;
		bool  bDrawLightFrustum = false;
		bool  bDrawBoundingBoxes = false;
		bool  bShowMilliseconds = true;
		bool  bBloom = true;
		bool  bUseMagnifier = false;
		bool  bLockMagnifierPosition;
		bool  bLockMagnifierPositionHistory;
	};

	struct robj_info {
		//gltf passes
		//GltfPbrPass* m_GLTFPBR = nullptr;
		//GltfBBoxPass* m_GLTFBBox = nullptr;
		//GltfDepthPass* m_GLTFDepth = nullptr;
		//gltf_gpu_res_cx* _ptb = nullptr;
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
		cxDevice* m_pDevice = 0;
		uint32_t m_frameIndex = 0;
		uint32_t m_numberOfAllocators = 0;
		uint32_t m_commandListsPerBackBuffer = 0;
	public:
		void OnCreate(cxDevice* pDevice, uint32_t numberOfBackBuffers, uint32_t commandListsPerframe, bool compute = false);
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
		VkSemaphore sem_out = {};
		VkImage image = 0;
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
	struct Geometry
	{
		VkIndexType m_indexType = {};
		uint32_t m_NumIndices = 0;
		uint32_t instanceCount = 1;
		VkDescriptorBufferInfo m_IBV = {};
		std::vector<VkDescriptorBufferInfo> m_VBV;
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
	using UVMAT_TYPE = glm::mat3x4;
	using pbrMaterial = pbr_factors_t;

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


	// todo renderer 渲染器
	class Renderer_cx
	{
	public:
		Renderer_cx(const_vk* p);
		~Renderer_cx();
		void OnCreate(cxDevice* pDevice, VkRenderPass rp);
		void OnDestroy();

		void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height);
		void OnDestroyWindowSizeDependentResources();
		void OnUpdateDisplayDependentResources(VkRenderPass rp, DisplayMode dm, bool bUseMagnifier);
		void OnUpdateLocalDimmingChangedResources(VkRenderPass rp, DisplayMode dm);

		//int load_model(GLTFCommon* pGLTFCommon, int Stage = 0);
		//void UnloadScene();
		//void unloadgltf(robj_info* p);

		const std::vector<TimeStamp>& GetTimingValues() { return m_TimeStamps; }

		//PerFrame_t* mkPerFrameData(const Camera& cam);
		PerFrame_t* mkPerFrameData();
		//void OnRender(scene_state* pState, const Camera& Cam);
		void OnRender(scene_state* pState);
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
		cxDevice* m_pDevice = 0;
		const_vk ct = {};
		uint32_t                        m_Width = {};
		uint32_t                        m_Height = {};

		VkRect2D                        m_RectScissor = {};
		VkViewport                      m_Viewport = {};

		// todo Initialize helper classes资源管理
		ResourceViewHeaps               m_ResourceViewHeaps = {};	// set管理
		UploadHeap                      m_UploadHeap = {};			// 纹理上传堆
		buffer_ring_cx					m_ConstantBufferRing = {};	// 动态常量缓冲区
		static_buffer_pool_cx			m_VidMemBufferPool = {};	// 静态顶点/索引缓冲区
		static_buffer_pool_cx			m_SysMemBufferPool = {};	// 系统静态几何缓冲区
		CommandListRing                 m_CommandListRing = {};		// 命令管理VkCommandBuffer

		GPUTimestamps                   m_GPUTimer = {};			// 记录GPU执行时间

		// effects
		Bloom                           m_Bloom = {};
		SkyDome                         m_SkyDome = {};
		DownSamplePS                    m_DownSample = {};
		SkyDomeProc                     m_SkyDomeProc = {};
		ToneMapping                     m_ToneMappingPS = {};
		ToneMappingCS                   m_ToneMappingCS = {};
		//oitblendCS						m_oitblendCS = {};
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
		//Wireframe                       m_Wireframe = {};
		//WireframeBox                    m_WireframeBox = {};
		//WireframeSphere					_WireframeSphere = {};
		////Axis							_axis= {};
		////CheckerBoardFloor				_cbf = {};
		std::vector<TimeStamp>          m_TimeStamps = {};

		//AsyncPool* m_AsyncPool = {};
		// 渲染对象
		std::vector<robj_info*>         _robject = {};
		robj_info* currobj = {};

		std::vector<SceneShadowInfo>    m_shadowMapPool = {};
		std::vector<VkImageView>        m_ShadowSRVPool = {};
		//std::vector<GltfDepthPass*>     _depthpass = {};
		// 灯光管理
		std::vector<light_t> _lights;
		std::queue<light_t> _lights_q;
		std::vector<glm::mat4> lightMats;
		// 渲染对象
		drawables_t drawables;

		//transfer_cx _transfer = {};


		PerFrame_t _perFrameData;

		fbo_cxt _fbo = {};
		// VkCommandBuffer
		void(*hubDraw)(void* commandBuffer) = nullptr;
		int backBufferCount = 3;

		DisplayMode _dm = DISPLAYMODE_SDR;
		bool bHDR = false;
		bool m_bMagResourceReInit = false;
		// 不启用oit
		bool has_oit = false;
		// 是否启用模板测试
		bool _stencil_test = false;
		bool has_skydome = true;

		bool flipY = false;
	};
	class dynamicrendering_t
	{
	public:
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


#endif
	// 1对象头

}
// !vkg