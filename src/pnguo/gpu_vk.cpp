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



// 新渲染器声明  
namespace vkg {
	static const uint32_t MaxLightInstances = 4;
	static const uint32_t MaxShadowInstances = 4;
	const VkColorComponentFlags allBits = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

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
		VkBuffer        m_buffer;

#ifdef USE_VMA
		VmaAllocation   m_bufferAlloc = VK_NULL_HANDLE;
#else
		VkDeviceMemory  m_deviceMemory = VK_NULL_HANDLE;
#endif
	};
	class static_buffer_pool_cx
	{
	public:
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

}
// !vkg


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
		if (defines.Has("HAS_FORWARD_RT"))
		{
			VkPipelineColorBlendAttachmentState att_state = {};
			get_blend(info->blending, att_state);
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

		VkPipelineColorBlendAttachmentState cblend = {};
		cblend.colorWriteMask = allBits;
		cblend.blendEnable = VK_FALSE;
		cblend.alphaBlendOp = VK_BLEND_OP_ADD;
		cblend.colorBlendOp = VK_BLEND_OP_ADD;
		cblend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		cblend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		cblend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		cblend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		if (defines.Has("HAS_SPECULAR_ROUGHNESS_RT"))
		{
			att_states.push_back(cblend);
		}
		if (defines.Has("HAS_DIFFUSE_RT"))
		{
			att_states.push_back(cblend);
		}
		if (defines.Has("HAS_NORMALS_RT"))
		{
			att_states.push_back(cblend);
		}
		if (defines.Has("HAS_MOTION_VECTORS_RT"))
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
			vkUnmapMemory(m_pDevice->GetDevice(), m_deviceMemory);
			vkFreeMemory(m_pDevice->GetDevice(), m_deviceMemory, NULL);
			m_deviceMemory = VK_NULL_HANDLE;
			vkDestroyBuffer(m_pDevice->GetDevice(), m_buffer, NULL);

#endif
			m_buffer = VK_NULL_HANDLE;
		}
	}

	// 纹理
#if 1
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
	void UploadHeap::OnCreate(cxDevice* pDevice, SIZE_T uSize)
	{
		m_pDevice = pDevice;

		VkResult res;

		// Create command list and allocators 
		{
			VkCommandPoolCreateInfo cmd_pool_info = {};
			cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmd_pool_info.queueFamilyIndex = m_pDevice->GetGraphicsQueueFamilyIndex();
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



