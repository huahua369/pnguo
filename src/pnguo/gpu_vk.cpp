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
	bool is_morph_target_valid(vkg::ubo_size_t* us, vkg::morph_target_t* mt)
	{
		return us && (us->ID_TARGET_DATA || us->ID_MORPHING_DATA) && mt && (mt->position_offset >= 0 || mt->normal_offset >= 0 || mt->tangent_offset >= 0 || mt->texcoord0_offset >= 0 || mt->texcoord1_offset >= 0 || mt->color0_offset >= 0);
	}
	VkDescriptorSetLayout newDescriptorSetLayout(VkDevice dev, vkg::ubo_size_t* us, vkg::morph_target_t* mt, vkg::ubo_size_t* binding)
	{
		std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
		int binc = 0;
		vkg::ubo_size_t binding0 = {};
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
		if (is_morph_target_valid(us, mt))
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
#if 1

	void new_pipeline(vkr::Device* pdev, PBRPipe_t* pbrpipe, const vkr::DefineList& defines0, std::vector<VkVertexInputAttributeDescription>* playout, VkDescriptorSetLayout layout1, VkDescriptorSetLayout layout2, pipeline_info_t* info)
	{
		if (!pbrpipe || !info || pbrpipe->m_pipeline)
		{
			return;
		}
		auto dev = pdev->GetDevice();
		auto& layout = *playout;
		if (!pbrpipe->m_pipelineLayout) {
			std::vector<VkDescriptorSetLayout> descriptorSetLayout = { layout1 };
			if (layout2 != VK_NULL_HANDLE)
			{
				descriptorSetLayout.push_back(layout2);
			}
			VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
			pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pPipelineLayoutCreateInfo.pNext = NULL;
			pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
			pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
			pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayout.size();
			pPipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout.data();
			VkResult res = vkCreatePipelineLayout(dev, &pPipelineLayoutCreateInfo, NULL, &pbrpipe->m_pipelineLayout);
			assert(res == VK_SUCCESS);
			vkr::SetResourceName(dev, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)pbrpipe->m_pipelineLayout, "GltfPbrPass PL");
		}

		// Compile and create shaders
		auto defines = defines0;
		VkPipelineShaderStageCreateInfo vertexShader = {}, fragmentShader = {};
		// Create pipeline 
		pdev->VKCompileFromFile(VK_SHADER_STAGE_VERTEX_BIT, "GLTFPbrPass-vert.glsl", "main", "", &defines, &vertexShader);
		pdev->VKCompileFromFile(VK_SHADER_STAGE_FRAGMENT_BIT, "GLTFPbrPass-frag.glsl", "main", "", &defines, &fragmentShader);
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertexShader, fragmentShader };
		// vertex input state
		std::vector<VkVertexInputBindingDescription> vi_binding(layout.size());
		for (int i = 0; i < layout.size(); i++)
		{
			vi_binding[i].binding = layout[i].binding;
			vi_binding[i].stride = vkr::SizeOfFormat(layout[i].format);
			vi_binding[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}
		//inputRate = VK_VERTEX_INPUT_RATE_INSTANCE; }
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
		ia.primitiveRestartEnable = info->primitiveRestartEnable;
		ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
		ds.stencilTestEnable = VK_TRUE;
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


void gpu_vk_test0()
{

}



