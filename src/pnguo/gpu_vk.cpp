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

	class Device;

	struct SystemInfo
	{
		std::string mCPUName = "UNAVAILABLE";
		std::string mGPUName = "UNAVAILABLE";
		std::string mGfxAPI = "UNAVAILABLE";
	};

	class gdev_cx
	{
	public:
		adevice3_t ad_cb = {};
		void* inst = 0;
		Device* _dev = nullptr;
		std::vector<Device*> devicelist;
		std::vector<std::string> dev_name;
		SystemInfo _systemInfo;
	public:
		gdev_cx();
		~gdev_cx();

		void* new_device(void* phy, void* dev, const char* devname);
		dev_info_cx get_devinfo();
	private:

	};
}
// !vkg

// 新渲染器实现
namespace vkg {
	gdev_cx::gdev_cx()
	{
	}

	gdev_cx::~gdev_cx()
	{
	}
	void* gdev_cx::new_device(void* phy, void* dev0, const char* devname)
	{
		Device* dev = 0;
		dev_info_cx c[1] = {};
		c->inst = inst;
		c->phy = phy;
		c->vkdev = dev0;
		if (!c) return 0;
		SystemInfo systemInfo;
		bool cpuvalid = false;
		bool gpuvalid = false;
#ifdef _DEBUG
		cpuvalid = 1;
		gpuvalid = 1;
#endif // _DEBUG 
		return dev;
	}
	dev_info_cx gdev_cx::get_devinfo()
	{
		dev_info_cx r = {};
		if (_dev) {
			//r.inst = _dev->_instance;
			//r.phy = _dev->_physicaldevice;
			//r.vkdev = _dev->_device;
		}
		return r;
	}
	aDevice new_device(void* vctx, void* phy, void* dev, const char* devname) {
		aDevice p = nullptr;
		if (!vctx)return p;
		auto ctx = (gdev_cx*)vctx;
		auto d = ctx->new_device(phy, dev, devname);
		p = d;
		return p;
	}
	aCamera new_aCamera(void* ctx, const char* type) {
		aCamera p = nullptr;
		return p;
	}
	aArray new_aArray(void* ctx, const char* type) {
		aArray p = nullptr;
		return p;
	}
	aArray1D new_aArray1D(void* ctx, const char* type) {
		aArray1D p = nullptr;
		return p;
	}
	aArray2D new_aArray2D(void* ctx, const char* type) {
		aArray2D p = nullptr;
		return p;
	}
	aArray3D new_aArray3D(void* ctx, const char* type) {
		aArray3D p = nullptr;
		return p;
	}
	aFrame new_aFrame(void* ctx, const char* type) {
		aFrame p = nullptr;
		return p;
	}
	aGeometry new_aGeometry(void* ctx, const char* type) {
		aGeometry p = nullptr;
		return p;
	}
	aGroup new_aGroup(void* ctx, const char* type) {
		aGroup p = nullptr;
		return p;
	}
	aInstance new_aInstance(void* ctx, const char* type) {
		aInstance p = nullptr;
		return p;
	}
	aLight new_aLight(void* ctx, const char* type) {
		aLight p = nullptr;
		return p;
	}
	aMaterial new_aMaterial(void* ctx, const char* type) {
		aMaterial p = nullptr;
		return p;
	}
	aPipeline new_aPipeline(void* ctx, const char* type) {
		aPipeline p = nullptr;
		return p;
	}
	aPipelineCS new_aPipelineCS(void* ctx, const char* type) {
		aPipelineCS p = nullptr;
		return p;
	}
	aSampler new_aSampler(void* ctx, const char* type) {
		aSampler p = nullptr;
		return p;
	}
	aSurface new_aSurface(void* ctx, const char* type) {
		aSurface p = nullptr;
		return p;
	}
	aRenderer new_aRenderer(void* ctx, const char* type) {
		aRenderer p = nullptr;
		return p;
	}
	aWorld new_aWorld(void* ctx, const char* type) {
		aWorld p = nullptr;
		return p;
	}


	aObject new_object(aDevice* ctx, int obj_type, const char* type) {
		auto t = (obj_type_e)obj_type;
		aObject p = nullptr;
		switch (t) {
		case OBJ_CAMERA:	p = new_aCamera(ctx, type); break;
		case OBJ_ARRAY:		p = new_aArray(ctx, type); break;
		case OBJ_ARRAY1D:	p = new_aArray1D(ctx, type); break;
		case OBJ_ARRAY2D:	p = new_aArray2D(ctx, type); break;
		case OBJ_ARRAY3D:	p = new_aArray3D(ctx, type); break;
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
	}
	// 增加引用计数
	void retain(aObject obj) {
	}
	// 设置参数
	void set_param(void* obj, const char* name, int data_type, void* data) {
	}
	// 取消参数
	void unset_param(aObject object, const char* name) {
	}
	void unset_allparams(aObject object) {}
	// 提交参数
	void commit_params(aObject object) {
	}
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
	void render_frame(aFrame frame) {
	}
	// 帧渲染完成检查
	int frame_ready(aFrame frame, int wait_mask) {
		return 0;
	}
}
//!vkg

adevice3_t* new_gdev(void* inst) {
	auto ctx = new vkg::gdev_cx();
	ctx->inst = inst;
	auto p = &ctx->ad_cb;
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
void makeTextureDescriptors(int first, std::vector<VkDescriptorSetLayoutBinding>& layout_bindings)
{
	uint32_t descriptorCounts[] = { 23, 3, MaxShadowInstances };
	for (int i = 0; i < 3; i++)
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = first + i;
		b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		b.descriptorCount = descriptorCounts[i];
		b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		b.pImmutableSamplers = NULL;
		layout_bindings.push_back(b);
	}
}
VkDescriptorSetLayout newDescriptorSetLayout(VkDevice dev, std::vector<uint32_t>& descriptorCounts, const VkSampler* pSamplers, VkDescriptorSetLayout* pDescSetLayout, VkDescriptorSet* pDescriptorSet)
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
	return newDescriptorSetLayout(dev, &layoutBindings);
}

void gpu_vk_test0()
{

}



