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
	void md_vis(pipeline_info_2* info2, vkr::DefineList& defines, std::vector<VkVertexInputAttributeDescription>& playout, std::vector<VkVertexInputBindingDescription>& vi_binding)
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
	void new_pipeline(vkr::Device* pdev, PBRPipe_t* pbrpipe, pipeline_info_2* info2)
	{
		auto info = &info2->info;
		if (!pbrpipe || !info2 || !info || pbrpipe->m_pipeline || !(info2->attributeCount > 0))
		{
			return;
		}
		auto dev = pdev->GetDevice();

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
			vkr::SetResourceName(dev, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)pbrpipe->m_pipelineLayout, "GltfPbrPass PL");
		}

		// Compile and create shaders
		vkr::DefineList defines;
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
		vkr::SetResourceName(dev, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pbrpipe->m_pipeline, "GltfPbrPass P");
		// create wireframe pipeline
		rs.polygonMode = VK_POLYGON_MODE_LINE;
		rs.cullMode = VK_CULL_MODE_NONE;
		//ds.depthWriteEnable = false;
		res = vkCreateGraphicsPipelines(dev, pdev->GetPipelineCache(), 1, &pipeline, NULL, &pbrpipe->m_pipelineWireframe);
		assert(res == VK_SUCCESS);
		vkr::SetResourceName(dev, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pbrpipe->m_pipelineWireframe, "GltfPbrPass Wireframe P");
	}
#endif


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
	struct SamplerCreateInfo {
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
	struct sampler_kt {
		SamplerCreateInfo info = {};
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
		//std::unordered_map<sampler_kt, VkSampler, SamplerKeyHash> _samplers;
		std::unordered_map<sampler_kt, VkSampler, SAKHash> _samplers;
	public:
		cxDevice();
		~cxDevice();
		// 设置vk设备和队列，必须调用此函数设置设备后才能使用其他对象
		void set_device(void* dev, void* q);
		// 创建采样器，内部会缓存相同参数的采样器对象
		VkSampler newSampler(const SamplerCreateInfo* pCreateInfo);
	};
	class cxCamera :public cxObject
	{
	public:
	public:
		cxCamera();
		~cxCamera();

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


#endif // 1

	// 实现
#if 1
#ifndef HASH_SEED
#define HASH_SEED 2166136261
#endif
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
	bool sampler_kt::operator==(const sampler_kt& other) const
	{
		return memcmp(&info, &other.info, sizeof(SamplerCreateInfo)) == 0;
	}
	size_t SAKHash::operator()(const sampler_kt& k) const
	{
		return Hash_p((const size_t*)&k.info, sizeof(SamplerCreateInfo), HASH_SEED);
	}

	cxObject::cxObject() {}
	cxObject::cxObject(int t) :obj_type(t)
	{
	}
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
	}
	cxDevice::~cxDevice()
	{
	}

	void cxDevice::set_device(void* dev, void* q)
	{
		_dev = (VkDevice)dev;
		_queue = (VkQueue)q;
		assert(_dev && _queue);
	}

	VkSampler cxDevice::newSampler(const SamplerCreateInfo* pCreateInfo) {
		sampler_kt k = { *pCreateInfo };
		auto kt = *pCreateInfo;
		auto& p = _samplers[k];
		if (!p)
		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.flags = 0;
			info.magFilter = (VkFilter)kt.magFilter;
			info.minFilter =(VkFilter) kt.minFilter;
			info.addressModeU = (VkSamplerAddressMode)kt.addressModeU;
			info.addressModeV = (VkSamplerAddressMode)kt.addressModeV;
			info.addressModeW =(VkSamplerAddressMode) kt.addressModeW;
			info.mipLodBias = kt.mipLodBias;
			info.maxAnisotropy = kt.maxAnisotropy;
			info.compareOp =(VkCompareOp) kt.compareOp;
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
	{
	}

	cxCamera::~cxCamera()
	{
	}
	cxFrame::cxFrame() :cxObject(OBJ_FRAME)
	{
	}

	cxFrame::~cxFrame()
	{
	}
	cxArray::cxArray() :cxObject(OBJ_ARRAY)
	{
	}
	cxArray::~cxArray()
	{
	}
	cxGeometry::cxGeometry() :cxObject(OBJ_GEOMETRY)
	{
	}

	cxGeometry::~cxGeometry()
	{
	}

	cxGroup::cxGroup() :cxObject(OBJ_GROUP)
	{
	}

	cxGroup::~cxGroup()
	{
	}
	cxInstance::cxInstance() :cxObject(OBJ_INSTANCE)
	{
	}
	cxInstance::~cxInstance()
	{
	}

	cxLight::cxLight() :cxObject(OBJ_LIGHT)
	{
	}
	cxLight::~cxLight()
	{
	}
	cxMaterial::cxMaterial() :cxObject(OBJ_MATERIAL)
	{
	}
	cxMaterial::~cxMaterial()
	{
	}
	cxPipeline::cxPipeline() :cxObject(OBJ_PIPELINE)
	{
	}
	cxPipeline::~cxPipeline()
	{
	}
	cxPipelineCS::cxPipelineCS() :cxObject(OBJ_PIPELINECS)
	{
	}
	cxPipelineCS::~cxPipelineCS()
	{
	}
	cxSampler::cxSampler() :cxObject(OBJ_SAMPLER)
	{
	}
	cxSampler::~cxSampler()
	{
	}
	cxSurface::cxSurface() :cxObject(OBJ_SURFACE)
	{
	}
	cxSurface::~cxSurface()
	{
	}
	cxRenderer::cxRenderer() :cxObject(OBJ_RENDERER)
	{
	}
	cxRenderer::~cxRenderer()
	{
	}
	cxWorld::cxWorld() :cxObject(OBJ_WORLD)
	{
	}
	cxWorld::~cxWorld()
	{
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


#endif // !1 实现


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
		auto c = new cxCamera();
		p = c;
		return p;
	}
	aArray new_aArray(void* ctx, const char* type) {
		aArray p = nullptr;
		auto c = new cxArray();
		p = c;
		return p;
	}

	aFrame new_aFrame(void* ctx, const char* type) {
		aFrame p = nullptr;
		auto c = new cxFrame();
		p = c;
		return p;
	}
	aGeometry new_aGeometry(void* ctx, const char* type) {
		aGeometry p = nullptr;
		auto c = new cxGeometry();
		p = c;
		return p;
	}
	aGroup new_aGroup(void* ctx, const char* type) {
		aGroup p = nullptr;
		auto c = new cxGroup();
		p = c;
		return p;
	}
	aInstance new_aInstance(void* ctx, const char* type) {
		aInstance p = nullptr;
		auto c = new cxInstance();
		p = c;
		return p;
	}
	aLight new_aLight(void* ctx, const char* type) {
		aLight p = nullptr;
		auto c = new cxLight();
		p = c;
		return p;
	}
	aMaterial new_aMaterial(void* ctx, const char* type) {
		aMaterial p = nullptr;
		auto c = new cxMaterial();
		p = c;
		return p;
	}
	aPipeline new_aPipeline(void* ctx, const char* type) {
		aPipeline p = nullptr;
		auto c = new cxPipeline();
		p = c;
		return p;
	}
	aPipelineCS new_aPipelineCS(void* ctx, const char* type) {
		aPipelineCS p = nullptr;
		auto c = new cxPipelineCS();
		p = c;
		return p;
	}
	aSampler new_aSampler(void* ctx, const char* type) {
		aSampler p = nullptr;
		auto c = new cxSampler();
		p = c;
		return p;
	}
	aSurface new_aSurface(void* ctx, const char* type) {
		aSurface p = nullptr;
		auto c = new cxSurface();
		p = c;
		return p;
	}
	aRenderer new_aRenderer(void* ctx, const char* type) {
		aRenderer p = nullptr;
		auto c = new cxRenderer();
		p = c;
		return p;
	}
	aWorld new_aWorld(void* ctx, const char* type) {
		aWorld p = nullptr;
		auto c = new cxWorld();
		p = c;
		return p;
	}


	aObject new_object(aDevice* ctx, int obj_type, const char* type) {
		auto t = (obj_type_e)obj_type;
		aObject p = nullptr;
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


void gpu_vk_test0()
{

}



