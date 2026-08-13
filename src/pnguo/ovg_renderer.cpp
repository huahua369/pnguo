/*
2026/8/8
sdl3 gpu规则
set0 v 纹理
set1 v ubo
set2 f 纹理
set3 f ubo

*/

#ifndef GLM_FORCE_XYZW_ONLY 
#define GLM_ENABLE_EXPERIMENTAL
//#define GLM_FORCE_ALIGNED
//#define GLM_FORCE_INTRINSICS
// 定义glm启用simd
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_XYZW_ONLY
#include <glm/glm.hpp>  

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/closest_point.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp> 
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/euler_angles.hpp>
#endif



#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <vulkan/vulkan.h>

#include "ovg_renderer.h"
#include "ovg.h"

#ifndef USE_VMA_OFF
#define VMA_IMPLEMENTATION
#endif
#include <vk_mem_alloc.h>

#include <array>
#include <map>
#include <memory_resource>


// todo 引用spv
#if 1
#include "shaders/spv_c/a_vg.vert.h"
#include "shaders/spv_c/a_vg.frag.h"

#include "shaders/spv_c/a_base3d.vert.h"
#include "shaders/spv_c/a_base3d.frag.h"

#include "shaders/spv_c/a_base3d_mask.vert.h"
#include "shaders/spv_c/a_base3d_mask.frag.h"

#include "shaders/spv_c/a_base3d_dsc.vert.h"
#include "shaders/spv_c/a_base3d_dsc.frag.h"

#include "shaders/spv_c/a_base3d_inst.vert.h"
#include "shaders/spv_c/a_base3d_inst.frag.h"

#include "shaders/spv_c/a_base3d_dsc_inst.vert.h"
#include "shaders/spv_c/a_base3d_dsc_inst.frag.h"
#endif // 1
struct shadermodule_vf {
	VkShaderModule vert;
	VkShaderModule frag;
};
struct spv_u2 {
	const uint32_t* vert;
	const uint32_t* frag;
};

struct spv_len2 {
	size_t vert;
	size_t frag;
};
spv_u2 code[] = { vg_vert, vg_frag,					// 矢量图管线						2d
	a_base3d_vert,	a_base3d_frag,					// 普通三角形(纹理)					2d/3d
	a_base3d_mask_vert,	a_base3d_mask_frag,			// 普通三角形+遮罩纹理				2d/3d
	a_base3d_dsc_vert,	a_base3d_dsc_frag,			// 双面三角形(两种颜色/纹理)			
	a_base3d_inst_vert,	a_base3d_inst_frag,			// 三角形(纹理)实例化					
	a_base3d_dsc_inst_vert,	a_base3d_dsc_inst_frag,	// 双面三角形(两种颜色/纹理)实例化		
};
spv_len2 code_len[] = { sizeof(vg_vert) , sizeof(vg_frag),
	sizeof(a_base3d_vert), sizeof(a_base3d_frag),
	sizeof(a_base3d_mask_vert), sizeof(a_base3d_mask_frag),
	sizeof(a_base3d_dsc_vert), sizeof(a_base3d_dsc_frag),
	sizeof(a_base3d_inst_vert), sizeof(a_base3d_inst_frag),
	sizeof(a_base3d_dsc_inst_vert), sizeof(a_base3d_dsc_inst_frag), };



void matrix_transform_distance(const glm::mat3x2* matrix, float* dx, float* dy);
void matrix_transform_point(const glm::mat3x2* matrix, float* x, float* y);

glm::mat4 ovg_ortho(float width, float height, float znear, float zfar, bool is_top);

// todo 后端
#if 1

#include "vkh_device.h"
#include "vkh_app.h"
#include "vkh_queue.h"
#include "vkh_image.h"
#include "vkh_buffer.h"

struct pipelinestate_p
{
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VkDescriptorSetLayout descriptorSetLayout;
	gem_info_t state = {};
};
struct ovgVertex2 {
	glm::vec2     pos;
	glm::vec2     uv;
	uint32_t color;
};
#ifndef STENCIL_FILL_BIT
#define STENCIL_FILL_BIT              0x1
#define STENCIL_CLIP_BIT              0x2
#define STENCIL_ALL_BIT               0x3
#define FULLSCREEN_BIT         0x10000000
#define SRCTYPE_MASK           0x000000FF
#endif
struct vg_surface_t {
	VkhImage img;
	int width, height;
};
// 兼容VkhDevice
struct ovg_device_t {
	VkDevice dev;
	VkPhysicalDeviceMemoryProperties phyMemProps;
	VkPhysicalDevice phy;
	VkInstance instance;
	VmaAllocator allocator;
	void* application;
	VkPipelineCache pipelineCache;

	VkhQueue gQueue;
	VkCommandPool   cmdPool;
	VkCommandBuffer cmd;
	VkFence fence;

};
struct vg_surface
{
	int   status; /**< Current status of surface, affected by last operation */
	uint32_t        references;
	ovg_device_t* dev;
	uint32_t        width;
	uint32_t        height;
	VkFormat        format;
	VkFramebuffer   fb;
	VkhImage        img;
	VkhImage        imgMS;
	VkhImage        stencil;
	VkCommandPool   cmdPool; // local pools ensure thread safety
	VkCommandBuffer cmd;     // surface local command buffer.
	bool            newSurf;
	mtx_t           mutex;
	VkFence flushFence; // unsignaled idle. 
	VkSemaphore sem;
	VkSemaphore sem0;
};
// vg使用的ubo/vbo/ibo
struct res_vt
{
	ovg_device_t* dev = 0;

	vkh_buffer_t uboGrad = {};
	vkh_buffer_t vertices = {};
	vkh_buffer_t indices = {};
	uint32_t     sizeUBO = 0;
	uint32_t     sizeVBO = 0;
	uint32_t     sizeIBO = 0;
	int	 vstride = 0;
	int	 ustride = 0;
public:
	res_vt();
	~res_vt();
	void create_vertices_buff(int stride);
	void create_uniform_buff(int stride, int count);
	void resize_ubo(uint32_t new_count, int stride);
	void resize_vbo(uint32_t new_size, int stride);
	void resize_ibo(size_t new_size);
	void upload_ubo(void* data, uint32_t offset, uint32_t size, bool flush);
	void upload_vbo(void* data, uint32_t offset, uint32_t size, bool flush);
	void upload_ibo(void* data, uint32_t offset, uint32_t size, bool flush);
};

struct ovg_ctx_t {
	ovg_device_t* dev = 0;
	VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM; VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
	VkSampleCountFlags samples = VK_SAMPLE_COUNT_1_BIT;

	VkDescriptorSetLayout dslPushDset = 0;
	VkPipelineLayout pipelineLayout = 0;
	VkPipeline pipe_OVER = 0; /**< default operator */
	VkPipeline pipe_SUB = 0;
	VkPipeline pipe_CLEAR = 0; /**< clear operator */
	VkPipeline pipelinePolyFill = 0; /**< even-odd polygon filling first step */
	VkPipeline pipelineClipping = 0; /**< draw on stencil to update clipping regions */

	ovg_canvas_cb ccb = {};
	res_vt res_vg = {};
	res_vt res_geom = {};
	PFN_vkCmdBeginRenderingKHR _vkCmdBeginRenderingKHR = VK_NULL_HANDLE;
	PFN_vkCmdEndRenderingKHR _vkCmdEndRenderingKHR = VK_NULL_HANDLE;
	PFN_vkCmdPushDescriptorSet _vkCmdPushDescriptorSet = {};
	uint32_t maxPushDescriptors = 0;
	VkhImage emptyImg = 0;
	size_t gxCount = 0;
	VkRect2D bounds = {};
	VkClearRect clearRect = {};
	vg_pattern_t* cu_pat = 0;
	glm::ivec2 fbo_size = {};
	VkhImage d_img = 0;
	uint64_t d_offset = 0;
	// geom用
	shadermodule_vf shaderModule[5] = {};	// 基础shader
	std::map<uint64_t, pipelinestate_p> pipelines;
	pipelinestate_p* pipeline = nullptr;	// 当前管线
	uint64_t curState = 0;	// 当前状态	
	uint64_t v2offset = 0;
	int status = 0;
	int curClipState = 0;
	bool isdset = false;
	bool cmdStarted = false;
public:
	ovg_ctx_t();
	~ovg_ctx_t();
	void init(ovg_device_t* dev, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlags samples);
	void init_geom();
	void destroy_geom();

	void draw_dynamic(ovg_draw_data* rvg, VkCommandBuffer cmd, vg_fbo_t* fbo, bool clear_all);
	void draw_geom(VkCommandBuffer cmd, geom_cmd_t* c);
	void draw_vg(VkCommandBuffer cmd, vgcmd_t* c, VkRect2D& cuclip);

	void update_va(ovg_draw_data* rvg);
	void dy_start_cmd(VkCommandBuffer cmd);

	pipelinestate_p* get_state(gem_info_t* info);
	pipelinestate_p new_spv_base(gem_info_t* info);
	void bind_geom(VkCommandBuffer cmd, size_t offset, size_t ioffset);
	void push_update_descriptor_set(VkCommandBuffer cmd, pipelinestate_p* cp, VkhImage img);
	VkRect2D set_scissor(VkCommandBuffer cmd, glm::vec4* scissor);
	void bind_draw_pipeline(VkCommandBuffer cmd, vg_state_save_t* t);
	void cmd_draw_full_screen_quad(VkCommandBuffer cmd, vgcmd_t* c, glm::vec4* scissor, VkRect2D* clip);
	void update_push_constants(VkCommandBuffer cmd, vg_state_save_t* t);
	void update_pattern(VkCommandBuffer cmd, vg_pattern_t* pat, vg_state_save_t* st);
	void push_update_descriptor_set_a(VkCommandBuffer cmd, VkhImage img, uint64_t offset);
};
template <typename T>
inline T ovgAlignUp(T val, T align)
{
	return (val + align - 1) / align * align;
}


ovg_ctx_t::ovg_ctx_t()
{}

ovg_ctx_t::~ovg_ctx_t()
{
	destroy_geom();
	vkDestroyPipelineLayout(dev->dev, pipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(dev->dev, dslPushDset, NULL);
#ifndef __APPLE__
	vkDestroyPipeline(dev->dev, pipelinePolyFill, NULL);
#endif
	vkDestroyPipeline(dev->dev, pipelineClipping, NULL);

	vkDestroyPipeline(dev->dev, pipe_OVER, NULL);
	vkDestroyPipeline(dev->dev, pipe_SUB, NULL);
	vkDestroyPipeline(dev->dev, pipe_CLEAR, NULL);

	dslPushDset = 0;
	pipelineLayout = 0;
	pipe_OVER = 0;
	pipe_SUB = 0;
	pipe_CLEAR = 0;
	pipelinePolyFill = 0;
	pipelineClipping = 0;
}


void free_vkdevctx(ovg_device_t* dev) {
	if (dev) {
		if (dev->pipelineCache) {
			vkDestroyPipelineCache(dev->dev, dev->pipelineCache, 0);
		}
		delete dev;
	}
}

ovg_device_t* new_vkdevctx(VkDevice vkdev, VkPhysicalDevice phy, VkInstance instance, uint32_t qFamIdx)
{
	if (!vkdev || !phy || !instance) { return 0; }
	ovg_device_t* dev = new ovg_device_t();
	dev->dev = vkdev;
	dev->phy = phy;
	dev->instance = instance;
	vkGetPhysicalDeviceMemoryProperties(phy, &dev->phyMemProps);
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = phy; allocatorInfo.device = dev->dev;
	vmaCreateAllocator(&allocatorInfo, (VmaAllocator*)&dev->allocator);
	VkhDevice vkhd = (VkhDevice)&dev->dev;
	dev->gQueue = vkh_queue_create(vkhd, qFamIdx, 0);
	dev->cmdPool = vkh_cmd_pool_create(vkhd, dev->gQueue->familyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	dev->cmd = vkh_cmd_buff_create(vkhd, dev->cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	dev->fence = vkh_fence_create_signaled(vkhd);
	return dev;
}
ovg_ctx_t* new_ovgctx(ovg_device_t* dev, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlags samples) {
	if (!dev)return 0;
	auto p = new ovg_ctx_t();
	p->init(dev, colorFormat, depthFormat, samples);
	return p;
}

VkhImage dc_device_create_empty_texture(ovg_device_t* dev, VkFormat format, int width, int height, const glm::vec4& color);

void ovg_ctx_t::init(ovg_device_t* dev, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlags samples)
{
	if (!dev || !dev->dev)return;
	ovg_ctx_t* ctx = this;
	ctx->dev = dev;
	ctx->colorFormat = colorFormat;
	ctx->depthFormat = depthFormat;
	ctx->samples = samples;
	res_vg.dev = dev;
	res_vg.create_uniform_buff(sizeof(vg_gradient_t), 8);
	res_vg.create_vertices_buff(sizeof(ovgVertex));
	res_geom.dev = dev;
	res_geom.create_vertices_buff(sizeof(geomVertex2));

	init_geom();
	emptyImg = dc_device_create_empty_texture(dev, colorFormat, 16, 16, glm::vec4(1.0));

	_vkCmdPushDescriptorSet = (PFN_vkCmdPushDescriptorSet)vkGetDeviceProcAddr(dev->dev, "vkCmdPushDescriptorSet");
	if (!_vkCmdPushDescriptorSet) _vkCmdPushDescriptorSet = (PFN_vkCmdPushDescriptorSet)vkGetInstanceProcAddr(dev->instance, "vkCmdPushDescriptorSet");
	_vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(dev->dev, "vkCmdBeginRenderingKHR"));
	_vkCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(dev->dev, "vkCmdEndRenderingKHR"));


	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	auto hr1 = vkCreatePipelineCache(dev->dev, &pipelineCacheCreateInfo, NULL, &dev->pipelineCache);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN };
	VkPipelineRasterizationStateCreateInfo rasterizationState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f };

	VkPipelineColorBlendAttachmentState blendAttachmentState = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	VkPipelineColorBlendStateCreateInfo colorBlendState = { .sType =
															   VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
														   .attachmentCount = 1,
														   .pAttachments = &blendAttachmentState };

	/*failOp,passOp,depthFailOp,compareOp, compareMask, writeMask, reference;*/
	VkStencilOpState polyFillOpState = { VK_STENCIL_OP_KEEP,
										VK_STENCIL_OP_INVERT,
										VK_STENCIL_OP_KEEP,
										VK_COMPARE_OP_EQUAL,
										STENCIL_CLIP_BIT,
										STENCIL_FILL_BIT,
										0 };
	VkStencilOpState clipingOpState = { VK_STENCIL_OP_ZERO,
										VK_STENCIL_OP_REPLACE,
										VK_STENCIL_OP_KEEP,
										VK_COMPARE_OP_EQUAL,
										STENCIL_FILL_BIT,
										STENCIL_ALL_BIT,
										0x2 };
	VkStencilOpState stencilOpState = { VK_STENCIL_OP_KEEP,
										VK_STENCIL_OP_ZERO,
										VK_STENCIL_OP_KEEP,
										VK_COMPARE_OP_EQUAL,
										STENCIL_FILL_BIT,
										STENCIL_FILL_BIT,
										0x1 };

	VkPipelineDepthStencilStateCreateInfo dsStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_ALWAYS,
		.stencilTestEnable = VK_TRUE,
		.front = polyFillOpState,
		.back = polyFillOpState };

	VkDynamicState dynamicStateEnables[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
		VK_DYNAMIC_STATE_STENCIL_REFERENCE,
		VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
	};
	VkPipelineDynamicStateCreateInfo dynamicState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2, .pDynamicStates = dynamicStateEnables };
	VkPipelineViewportStateCreateInfo viewportState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1, .scissorCount = 1 };
	VkPipelineMultisampleStateCreateInfo multisampleState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = (VkSampleCountFlagBits)ctx->samples };
	/*if (ctx->samples != VK_SAMPLE_COUNT_1_BIT){
		multisampleState.sampleShadingEnable = VK_TRUE;
		multisampleState.minSampleShading = 0.5f;
	}*/
	VkVertexInputBindingDescription vertexInputBinding = { .binding = 0, .stride = sizeof(ovgVertex2), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };
	VkVertexInputAttributeDescription vertexInputAttributs[3] = { {0, 0, VK_FORMAT_R32G32_SFLOAT, 0},
																 {1, 0, VK_FORMAT_R32G32_SFLOAT, 8},
																 {2, 0, VK_FORMAT_R8G8B8A8_UNORM, 16}
	};
	VkPipelineVertexInputStateCreateInfo vertexInputState = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	.vertexBindingDescriptionCount = 1,
	.pVertexBindingDescriptions = &vertexInputBinding,
	.vertexAttributeDescriptionCount = 3,
	.pVertexAttributeDescriptions = vertexInputAttributs };

	VkShaderModule modVert = {}, modFrag = {};
	VkShaderModuleCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = code_len[0].vert, .pCode = (uint32_t*)code[0].vert };
	auto hr = vkCreateShaderModule(dev->dev, &createInfo, NULL, &modVert);
	createInfo.pCode = (uint32_t*)code[0].frag;
	createInfo.codeSize = code_len[0].frag;
	hr = vkCreateShaderModule(dev->dev, &createInfo, NULL, &modFrag);

	VkDescriptorSetLayoutBinding    dsLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL };
	VkDescriptorSetLayoutCreateInfo dsLayoutCreateInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &dsLayoutBinding };
	std::array<VkDescriptorSetLayoutBinding, 2> setLayoutBindings = { };
	dsLayoutBinding.binding = 1;
	setLayoutBindings[1] = dsLayoutBinding;
	dsLayoutBinding.binding = 0;
	dsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	setLayoutBindings[0] = dsLayoutBinding;
	dsLayoutCreateInfo.bindingCount = 2;
	dsLayoutCreateInfo.pBindings = setLayoutBindings.data();
	dsLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
	hr = vkCreateDescriptorSetLayout(dev->dev, &dsLayoutCreateInfo, NULL, &ctx->dslPushDset);
	VkPushConstantRange pushConstantRange[] = {
		{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants_t)},
		//{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(push_constants_t)}
	};
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &ctx->dslPushDset,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = (VkPushConstantRange*)&pushConstantRange };
	hr = vkCreatePipelineLayout(dev->dev, &pipelineLayoutCreateInfo, NULL, &ctx->pipelineLayout);
	VkPipelineShaderStageCreateInfo vertStage = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = modVert,
		.pName = "main",
	};
	VkPipelineShaderStageCreateInfo fragStage = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = modFrag,
		.pName = "main",
	};
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };
	pipelineCreateInfo.stageCount = 1;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pDepthStencilState = &dsStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.layout = ctx->pipelineLayout;
	pipelineCreateInfo.renderPass = nullptr;
	VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &ctx->colorFormat,
		.depthAttachmentFormat = ctx->depthFormat,
		.stencilAttachmentFormat = ctx->depthFormat
	};
	pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;

#ifndef __APPLE__
	hr = vkCreateGraphicsPipelines(dev->dev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &ctx->pipelinePolyFill);
#endif
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	dsStateCreateInfo.back = dsStateCreateInfo.front = clipingOpState;
	dynamicState.dynamicStateCount = 5;
	hr = vkCreateGraphicsPipelines(dev->dev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &ctx->pipelineClipping);

	dsStateCreateInfo.back = dsStateCreateInfo.front = stencilOpState;
	blendAttachmentState.colorWriteMask = 0xf;
	dynamicState.dynamicStateCount = 3;
	pipelineCreateInfo.stageCount = 2;
	hr = vkCreateGraphicsPipelines(dev->dev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &ctx->pipe_OVER);
	blendAttachmentState.alphaBlendOp = blendAttachmentState.colorBlendOp = VK_BLEND_OP_SUBTRACT;
	hr = vkCreateGraphicsPipelines(dev->dev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &ctx->pipe_SUB);
	colorBlendState.logicOpEnable = VK_TRUE;
	blendAttachmentState.blendEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_CLEAR;
	hr = vkCreateGraphicsPipelines(dev->dev, dev->pipelineCache, 1, &pipelineCreateInfo, NULL, &ctx->pipe_CLEAR);
	vkDestroyShaderModule(dev->dev, modVert, NULL);
	vkDestroyShaderModule(dev->dev, modFrag, NULL);
	return;
}




VkhImage dc_device_create_empty_texture(ovg_device_t* dev, VkFormat format, int width, int height, const glm::vec4& color)
{
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	// create empty image to bind to context source descriptor when not in use
	VkhImage emptyImg = vkh_image_create((VkhDevice)&dev->dev, format, width > 0 ? width : 16, height > 0 ? height : 16, tiling, VKH_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	vkh_image_create_descriptor(emptyImg, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_NEAREST,
		VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	vkDeviceWaitIdle(dev->dev);
	auto cmd = dev->cmd;
	vkh_cmd_begin(dev->cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkClearColorValue       cclr = { {color.x, color.y, color.z, color.w} };
	VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VkhImage img = emptyImg;
	//vkh_image_set_layout(dev->cmd, img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
	//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	//	VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	vkh_image_set_layout(cmd, img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	vkCmdClearColorImage(cmd, vkh_image_get_vkimage(img), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &cclr, 1, &range);
	vkh_image_set_layout(dev->cmd, emptyImg, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	vkh_cmd_end(dev->cmd);
	vkResetFences(dev->dev, 1, &dev->fence);
	vkh_cmd_submit(dev->gQueue, &cmd, dev->fence);
	vkWaitForFences(dev->dev, 1, &dev->fence, VK_TRUE, UINT64_MAX);
	return emptyImg;
}
struct vg_fbo_t0
{
	uint32_t width, height;
	VkhImage img;
	VkhImage imgMS;
	VkhImage depthStencil;
};
void ovg_new_fbo(ovg_ctx_t* ctx, vg_fbo_t0* surf) {
	if (!ctx)return;
	auto dev = ctx->dev;
	if (!dev || !dev->dev || !surf)return;
	VkhDevice vkhd = (VkhDevice)&dev->dev;
	surf->img = vkh_image_create(vkhd, ctx->colorFormat, surf->width, surf->height, VK_IMAGE_TILING_OPTIMAL,
		VKH_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	vkh_image_create_descriptor(surf->img, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_NEAREST,
		VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	if (ctx->samples > VK_SAMPLE_COUNT_1_BIT) {
		surf->imgMS = vkh_image_ms_create(
			vkhd, ctx->colorFormat, (VkSampleCountFlagBits)ctx->samples, surf->width, surf->height, VKH_MEMORY_USAGE_GPU_ONLY,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		vkh_image_create_descriptor(surf->imgMS, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_NEAREST,
			VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
#if defined(DEBUG)  
		vkh_image_set_name(surf->imgMS, "SURF MS color IMG");
		vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)vkh_image_get_view(surf->imgMS),
			"SURF MS color VIEW");
		vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_SAMPLER, (uint64_t)vkh_image_get_sampler(surf->imgMS),
			"SURF MS color SAMPLER");
#endif
	}
	surf->depthStencil = vkh_image_ms_create(vkhd, ctx->depthFormat, (VkSampleCountFlagBits)ctx->samples, surf->width, surf->height,
		VKH_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

	vkh_image_create_descriptor(surf->depthStencil, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT, VK_FILTER_NEAREST,
		VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
#if defined(DEBUG)  
	vkh_image_set_name(surf->depthStencil, "SURF depthStencil");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)vkh_image_get_view(surf->depthStencil),
		"SURF stencil VIEW");
	vkh_device_set_object_name(vkhd, VK_OBJECT_TYPE_SAMPLER, (uint64_t)vkh_image_get_sampler(surf->depthStencil),
		"SURF stencil SAMPLER");
#endif
}
void ovg_free_fbo(vg_fbo_t0* surf) {
	if (!surf->img->imported)
		vkh_image_destroy(surf->img);
	vkh_image_destroy(surf->imgMS);
	vkh_image_destroy(surf->depthStencil);
}

void ovg_explicit_ms_resolve(VkCommandBuffer cmd, vg_fbo_t* fbo) {
	auto surf = (vg_fbo_t0*)fbo;
	vkh_image_set_layout(cmd, surf->imgMS, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	vkh_image_set_layout(cmd, surf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	VkImageResolve re = {
					 .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
					 .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
					 .extent = {surf->width, surf->height, 1} };
	vkCmdResolveImage(cmd, vkh_image_get_vkimage(surf->imgMS), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkh_image_get_vkimage(surf->img), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &re);
	vkh_image_set_layout(cmd, surf->imgMS, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	vkh_image_set_layout(cmd, surf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

}

void ovg_ctx_t::draw_dynamic(ovg_draw_data* rvg, VkCommandBuffer cmd, vg_fbo_t* fbo, bool clear_all)
{
	if (!cmd || !_vkCmdBeginRenderingKHR || !fbo)return;
	VkhImage image = (VkhImage)(fbo->imgMS ? fbo->imgMS : fbo->img);
	VkhImage depthStencil = (VkhImage)fbo->depthStencil;
	if (!image || !depthStencil)return;
	clearRect = { {{0}, {fbo->width,fbo->height}}, 0, 1 };
	update_va(rvg);
	vkh_cmd_begin(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkImageSubresourceRange crange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }; VkImageSubresourceRange dsrange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };
	vkh_image_set_layout_subres(cmd, image, crange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	vkh_image_set_layout_subres(cmd, depthStencil, dsrange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
	VkRenderingAttachmentInfoKHR colorAttachment{
	.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
	.imageView = image->view,
	.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	.loadOp = clear_all ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
	.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	.clearValue = {.color = {0.0f,0.0f,0.0f,0.0f} },
	};
	VkRenderingAttachmentInfoKHR depthStencilAttachment{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
		.imageView = depthStencil->view,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.loadOp = clear_all ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = {.depthStencil = {1.0f,  0} }
	};
	VkRenderingInfoKHR renderingInfo{
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
		.renderArea = { 0, 0, image->infos.extent.width, image->infos.extent.height },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachment,
		.pDepthAttachment = &depthStencilAttachment,
		.pStencilAttachment = &depthStencilAttachment
	};
	_vkCmdBeginRenderingKHR(cmd, &renderingInfo);
	dy_start_cmd(cmd);
	VkViewport viewport = { 0.0f,0.0f,(float)image->infos.extent.width, (float)image->infos.extent.height, 0.0f, 1.0f };
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = { 0, 0, image->infos.extent.width, image->infos.extent.height };
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	pipelinestate_p* cp = 0;
	VkRect2D cuclip = {};
	for (size_t i = 0; i < rvg->count; i++)
	{
		auto& it = rvg->d[i];
		switch (it.g.stype) {
		case 0:
			draw_geom(cmd, &it.g);
			break;
		case 1:
			draw_vg(cmd, &it.vg, cuclip);
			break;
		}
	}
	_vkCmdEndRenderingKHR(cmd);
	// 更新渐变ubo
	vkh_buffer_flush(&res_vg.uboGrad);
	if (fbo->imgMS && fbo->img)
		ovg_explicit_ms_resolve(cmd, fbo);
	vkh_cmd_end(cmd);
}

void free_ovgctx(ovg_ctx_t* p) {
	if (!p)return;
	delete p;
}

ovg_canvas_cb* get_canvas_cb(ovg_ctx_t* ctx)
{
	return &ctx->ccb;
}

void** get_ctx_pipe(ovg_ctx_t* ctx)
{
	return (void**)&ctx->pipelineLayout;
}


#if defined(DEBUG)
const float DBG_LAB_COLOR_RP[4] = { 0, 0, 1, 1 };
const float DBG_LAB_COLOR_FSQ[4] = { 1, 0, 0, 1 };
const float DBG_LAB_COLOR_SAV[4] = { 1, 0, 1, 1 };
const float DBG_LAB_COLOR_CLIP[4] = { 0, 1, 1, 1 };
#endif
#define VG_PTS_SIZE        1024
#define VG_VBO_SIZE        (VG_PTS_SIZE * 4)
#define VG_IBO_SIZE        (VG_VBO_SIZE * 6)
void res_vt::create_uniform_buff(int size, int count) {
	int us = size * count;
	ustride = size;
	vkh_buffer_init((VkhDevice)&dev->dev, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VKH_MEMORY_USAGE_CPU_TO_GPU,
		ovgAlignUp(us, 64), &uboGrad, true);
	sizeUBO = us;
}
res_vt::res_vt()
{
	sizeVBO = VG_VBO_SIZE; sizeIBO = VG_IBO_SIZE;
}
res_vt::~res_vt()
{
	vkh_buffer_reset(&uboGrad);
	vkh_buffer_reset(&vertices);
	vkh_buffer_reset(&indices);
	uboGrad = {};
	vertices = {};
	indices = {};
}
void res_vt::create_vertices_buff(int stride) {
	if (stride < 1)stride = 1;
	vstride = stride;
	sizeVBO *= stride;
	vkh_buffer_init((VkhDevice)&dev->dev, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VKH_MEMORY_USAGE_CPU_TO_GPU,
		sizeVBO, &vertices, true);
	vkh_buffer_init((VkhDevice)&dev->dev, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VKH_MEMORY_USAGE_CPU_TO_GPU,
		sizeIBO * sizeof(uint32_t), &indices, true);
}
void res_vt::resize_ubo(uint32_t new_count, int stride) {

	stride = std::max(1, stride);
	new_count *= stride;
	if (sizeUBO < new_count)
	{
		sizeUBO = new_count;
		int us = sizeUBO;
		vkh_buffer_resize(&uboGrad, ovgAlignUp(us, 64), true);
	}
}
void res_vt::resize_vbo(uint32_t new_size, int stride) {
	uint32_t mod = new_size % VG_VBO_SIZE;
	stride = std::max(1, stride);
	if (mod > 0)
		new_size += VG_VBO_SIZE - mod;
	new_size *= stride;
	if (sizeVBO < new_size)
	{
		sizeVBO = new_size;
		vkh_buffer_resize(&vertices, sizeVBO, true);
	}
}
void res_vt::resize_ibo(size_t new_size) {
	uint32_t mod = new_size % VG_IBO_SIZE;
	if (mod > 0)
		new_size += VG_IBO_SIZE - mod;
	if (sizeVBO < new_size)
	{
		sizeIBO = new_size;
		vkh_buffer_resize(&indices, sizeIBO * sizeof(uint32_t), true);
	}
}

void res_vt::upload_ubo(void* data, uint32_t offset, uint32_t size, bool flush)
{
	memcpy((char*)vkh_buffer_get_mapped_pointer(&uboGrad) + offset, data, size);
	if (flush)
		vkh_buffer_flush(&uboGrad);
}

void res_vt::upload_vbo(void* data, uint32_t offset, uint32_t size, bool flush)
{
	memcpy((char*)vkh_buffer_get_mapped_pointer(&vertices) + offset, data, size);
	if (flush)
		vkh_buffer_flush(&vertices);
}

void res_vt::upload_ibo(void* data, uint32_t offset, uint32_t size, bool flush)
{
	memcpy((char*)vkh_buffer_get_mapped_pointer(&indices) + offset, data, size);
	if (flush)
		vkh_buffer_flush(&indices);
}

void ovg_ctx_t::update_va(ovg_draw_data* rvg)
{
	auto vgsize = rvg->v_count;
	auto isize = rvg->i_count;
	if (vgsize > 0) {
		res_vg.resize_vbo(vgsize, sizeof(ovgVertex));
		res_vg.resize_ibo(isize);
		res_vg.resize_ubo(rvg->uboCount, sizeof(vg_gradient_t));
		res_vg.upload_vbo(rvg->vg_vertex, 0, vgsize * sizeof(ovgVertex), true);
		res_vg.upload_ibo(rvg->vg_indices, 0, isize, true);
	}
	// geom数据
	v2offset = rvg->v1_count * sizeof(geomVertex1);
	auto v2 = rvg->v2_count * sizeof(geomVertex2);
	auto gs = v2offset + v2;
	res_geom.resize_vbo(gs, 1);
	res_geom.resize_ibo(rvg->g_count);
	if (rvg->v1_count)
		res_geom.upload_vbo(rvg->vertex1, 0, v2offset, false);
	if (rvg->v2_count)
		res_geom.upload_vbo(rvg->vertex2, v2offset, v2, true);
	if (rvg->g_count)
		res_geom.upload_ibo(rvg->geom_indices, 0, rvg->g_count * sizeof(int), true);
}

void ovg_ctx_t::dy_start_cmd(VkCommandBuffer cmd) {

#if defined(DEBUG) 
	vkh_cmd_label_start(cmd, "dc render pass", DBG_LAB_COLOR_RP);
#endif

	VkViewport viewport = { 0, 0, (float)fbo_size.x, (float)fbo_size.y, 0, 1.f };
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	vkCmdSetScissor(cmd, 0, 1, &bounds);

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, &res_vg.vertices.buffer, offsets);
	vkCmdBindIndexBuffer(cmd, res_vg.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
	push_update_descriptor_set_a(cmd, d_img, d_offset);
	cmdStarted = true;
}

void ovg_ctx_t::bind_draw_pipeline(VkCommandBuffer cmd, vg_state_save_t* t) {
	auto ctx = this;
	switch (t->curOperator) {
	case VG_OPERATOR_OVER:
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipe_OVER);
		break;
	case VG_OPERATOR_CLEAR:
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipe_CLEAR);
		break;
	case VG_OPERATOR_DIFFERENCE:
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipe_SUB);
		break;
	default:
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipe_OVER);
		break;
	}
	vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
}

void ovg_ctx_t::cmd_draw_full_screen_quad(VkCommandBuffer cmd, vgcmd_t* c, glm::vec4* scissor, VkRect2D* clip)
{
#if defined(DEBUG)
	vkh_cmd_label_start(cmd, "_draw_full_screen_quad", DBG_LAB_COLOR_FSQ);
#endif
	if (scissor) {
		VkRect2D r = { {(int32_t)glm::max((int)scissor->x, 0), (int32_t)glm::max((int)scissor->y, 0)},
					  {(int32_t)glm::max((int)scissor->z - (int32_t)scissor->x + 1, 1),
					   (int32_t)glm::max((int)scissor->w - (int32_t)scissor->y + 1, 1)} };
		vkCmdSetScissor(cmd, 0, 1, &r);
	}

	uint32_t firstVertIdx = c->full_screen_quad;
	uint32_t fsq_patternType = 0;
	if (c->state)
		fsq_patternType = c->state->pushConsts.fsq_patternType;
	fsq_patternType |= FULLSCREEN_BIT;
	vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 24, 4, &fsq_patternType);
	vkCmdDraw(cmd, 3, 1, firstVertIdx, 0);
	fsq_patternType &= ~FULLSCREEN_BIT;
	vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 24, 4, &fsq_patternType);
	if (scissor)
		vkCmdSetScissor(cmd, 0, 1, clip && clip->extent.width > 0 && clip->extent.height > 0 ? clip : &bounds);

#if defined(DEBUG) 
	vkh_cmd_label_end(cmd);
#endif
}
VkRect2D ovg_ctx_t::set_scissor(VkCommandBuffer cmd, glm::vec4* scissor)
{
	VkRect2D r = { 0,0,-1,-1 };
	if (cmd)
	{
		r = bounds;
		if (scissor)
		{
			r.offset = { (int32_t)scissor->x , (int32_t)scissor->y };
			r.extent = { (uint32_t)glm::max(scissor->z, 1.0f), (uint32_t)std::max(scissor->w, 1.0f) };
		}
		vkCmdSetScissor(cmd, 0, 1, &r);
	}
	return r;
}
void o_sort_gradient_stops(glm::vec4* colors, float* stops, uint32_t count) {
	for (uint32_t i = 1; i < count; i++) {
		float   key_stop = stops[i];
		auto key_color = colors[i];
		int j = (int)i - 1;
		while (j >= 0 && stops[j] > key_stop) {
			stops[j + 1] = stops[j];
			colors[j + 1] = colors[j];
			j--;
		}
		stops[j + 1] = key_stop;
		colors[j + 1] = key_color;
	}
}
void ovg_ctx_t::update_push_constants(VkCommandBuffer cmd, vg_state_save_t* t) {
	t->pushConsts.size = { (float)fbo_size.x, (float)fbo_size.y };
	vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants_t), &t->pushConsts);
}
void ovg_ctx_t::update_pattern(VkCommandBuffer cmd, vg_pattern_t* pat, vg_state_save_t* st) {
	if (!cmd)return;
	vg_pattern_t* lastPat = cu_pat;
	cu_pat = pat;
	uint32_t newPatternType = VG_PATTERN_TYPE_SOLID;
	if (pat == NULL) {       // solid color
		if (lastPat == NULL) // solid
			return;          // solid to solid transition, no extra action requested
	}
	else
		newPatternType = pat->type;
	switch (newPatternType) {
	case VG_PATTERN_TYPE_SOLID:
		push_update_descriptor_set_a(cmd, 0, 0);
		break;
	case VG_PATTERN_TYPE_SURFACE:
	{
		auto surf = (vg_surface_t*)pat->data;
		vkh_cmd_begin(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		// transition source surface for sampling
		vkh_image_set_layout(cmd, surf->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		vkh_cmd_end(cmd);
		VkSamplerAddressMode addrMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkFilter             filter = VK_FILTER_NEAREST;
		switch (pat->extend) {
		case VG_EXTEND_NONE:
			addrMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			break;
		case VG_EXTEND_PAD:
			addrMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			break;
		case VG_EXTEND_REPEAT:
			addrMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			break;
		case VG_EXTEND_REFLECT:
			addrMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			break;
		}
		switch (pat->filter) {
		case VG_FILTER_BILINEAR:
		case VG_FILTER_BEST:
			filter = VK_FILTER_LINEAR;
			break;
		default:
			filter = VK_FILTER_NEAREST;
			break;
		}
		vkh_image_create_sampler(surf->img, filter, filter, VK_SAMPLER_MIPMAP_MODE_NEAREST, addrMode);
		push_update_descriptor_set_a(cmd, surf->img, 0);
		st->pushConsts.source.z = (float)surf->width;
		st->pushConsts.source.w = (float)surf->height;
		glm::mat3x3 mat;
		if (pat->hasMatrix) {
			mat = pat->matrix;
			st->pushConsts.matInv = st->pushConsts.matInv * mat;
		}
	}
	break;
	case VG_PATTERN_TYPE_LINEAR:
	case VG_PATTERN_TYPE_RADIAL:
	case VG_PATTERN_TYPE_SWEEP:
	{
		float fm = std::max((float)fbo_size.x, (float)fbo_size.y);
		glm::vec4 bounds = { fm,fm,0.0f,0.0f }; // store img bounds in unused source field
		st->pushConsts.source = bounds;
		// transform control point with current ctx matrix 
		vg_gradient_t grad = *(vg_gradient_t*)pat->data;
		if (grad.count < 2) {
			status = -1;// 错误
			return;
		}
		grad.extend = pat->extend;
		glm::mat3x2 mat;
		if (pat->hasMatrix) {
			glm::mat3x3 m = pat->matrix;
			mat = glm::inverse(m);
			matrix_transform_point(&mat, &grad.cp[0].x, &grad.cp[0].y);
		}
		matrix_transform_point(&st->pushConsts.mat, &grad.cp[0].x, &grad.cp[0].y);
		if (pat->type == VG_PATTERN_TYPE_LINEAR) {
			if (pat->hasMatrix)
				matrix_transform_point(&mat, &grad.cp[0].z, &grad.cp[0].w);
			matrix_transform_point(&st->pushConsts.mat, &grad.cp[0].z, &grad.cp[0].w);
		}
		else {
			if (pat->hasMatrix)
				matrix_transform_point(&mat, &grad.cp[1].x, &grad.cp[1].y);
			matrix_transform_point(&st->pushConsts.mat, &grad.cp[1].x, &grad.cp[1].y);
			// radii
			if (pat->hasMatrix) {
				matrix_transform_distance(&mat, &grad.cp[0].z, &grad.cp[0].w);
				matrix_transform_distance(&mat, &grad.cp[1].z, &grad.cp[0].w);
			}
			matrix_transform_distance(&st->pushConsts.mat, &grad.cp[0].z, &grad.cp[0].w);
			matrix_transform_distance(&st->pushConsts.mat, &grad.cp[1].z, &grad.cp[0].w);
		}
		o_sort_gradient_stops(grad.colors, grad.stops, grad.count);
		memcpy(((char*)vkh_buffer_get_mapped_pointer(&res_vg.uboGrad)) + gxCount, &grad, sizeof(vg_gradient_t));
		//vkh_buffer_flush(&ctx->uboGrad); 
		push_update_descriptor_set_a(cmd, 0, gxCount);
		gxCount += sizeof(vg_gradient_t);
	}
	break;
	}
	st->pushConsts.fsq_patternType = (st->pushConsts.fsq_patternType & FULLSCREEN_BIT) + newPatternType;

}

void ovg_ctx_t::push_update_descriptor_set_a(VkCommandBuffer cmd, VkhImage img, uint64_t offset) {
	if (!maxPushDescriptors || !_vkCmdPushDescriptorSet)return;
	d_img = img;//VkhImage
	d_offset = offset;
	if (!cmdStarted)
	{
		isdset = true;
		return;
	}
	VkDescriptorImageInfo dst1 = vkh_image_get_descriptor(img ? img : emptyImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	VkWriteDescriptorSet  wimg = {
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.dstSet = 0,
	.dstBinding = 1,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	.pImageInfo = 0 };
	VkDescriptorBufferInfo dbi = { res_vg.uboGrad.buffer, offset, sizeof(vg_gradient_t) };
	VkWriteDescriptorSet   wu = {
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.dstSet = 0,
	.dstBinding = 0,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	.pBufferInfo = &dbi };
	VkWriteDescriptorSet  wds[3] = {};
	wimg.dstBinding = 1;
	wimg.pImageInfo = &dst1;
	wds[1] = wimg;
	wds[0] = wu;
	_vkCmdPushDescriptorSet(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 2, wds);
}
pipelinestate_p* ovg_ctx_t::get_state(gem_info_t* info)
{
	uint64_t v = *(uint64_t*)info;
	if (!pipeline || curState != v)
	{
		curState = v;
		auto& pl = pipelines[v];
		if (!pl.pipeline) {
			pipelinestate_p p0 = new_spv_base(info);
			pl = p0;
		}
		if (pl.pipeline)
			pipeline = &pl;
	}
	return pipeline;
}

// 普通图片三角形渲染
#if 1

namespace ovg {
	VkShaderModule newModule(VkDevice device, const uint32_t* SpvData, size_t SpvSize, VkResult* r)
	{
		VkShaderModule pShaderModule = {};
		VkShaderModuleCreateInfo moduleCreateInfo = {};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pCode = (uint32_t*)SpvData;
		moduleCreateInfo.codeSize = SpvSize;
		auto hr = vkCreateShaderModule(device, &moduleCreateInfo, NULL, &pShaderModule);
		if (r)*r = hr;
		return pShaderModule;
	}
	void freeModule(VkDevice device, VkShaderModule module0)
	{
		if (module0)
			vkDestroyShaderModule(device, module0, NULL);
	}
#define new_module(device,spvdata,r) ovg::newModule(device,spvdata,sizeof(spvdata),r)

#define BLENDMODE_NONE                  0x00000000u /**< no blending: dstRGBA = srcRGBA */
#define BLENDMODE_BLEND                 0x00000001u /**< alpha blending: dstRGB = (srcRGB * srcA) + (dstRGB * (1-srcA)), dstA = srcA + (dstA * (1-srcA)) */
#define BLENDMODE_BLEND_PREMULTIPLIED   0x00000010u /**< pre-multiplied alpha blending: dstRGBA = srcRGBA + (dstRGBA * (1-srcA)) */
#define BLENDMODE_ADD                   0x00000002u /**< additive blending: dstRGB = (srcRGB * srcA) + dstRGB, dstA = dstA */
#define BLENDMODE_ADD_PREMULTIPLIED     0x00000020u /**< pre-multiplied additive blending: dstRGB = srcRGB + dstRGB, dstA = dstA */
#define BLENDMODE_MOD                   0x00000004u /**< color modulate: dstRGB = srcRGB * dstRGB, dstA = dstA */
#define BLENDMODE_MUL                   0x00000008u /**< color multiply: dstRGB = (srcRGB * dstRGB) + (dstRGB * (1-srcA)), dstA = dstA */

	void set_cblend(VkPipelineColorBlendAttachmentState& opt, const std::array<uint32_t, 6>& b) {
		opt.srcColorBlendFactor = (VkBlendFactor)b[0];
		opt.dstColorBlendFactor = (VkBlendFactor)b[1];
		opt.colorBlendOp = (VkBlendOp)b[2];
		opt.srcAlphaBlendFactor = (VkBlendFactor)b[3];
		opt.dstAlphaBlendFactor = (VkBlendFactor)b[4];
		opt.alphaBlendOp = (VkBlendOp)b[5];
	}
	/*
		VkBlendFactor            srcColorBlendFactor;
		VkBlendFactor            dstColorBlendFactor;
		VkBlendOp                colorBlendOp;
		VkBlendFactor            srcAlphaBlendFactor;
		VkBlendFactor            dstAlphaBlendFactor;
		VkBlendOp                alphaBlendOp;
		VkColorComponentFlags    colorWriteMask;
	*/

#if 1
#define BLENDMODE_NONE_FULL set_cblend(cba,{VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD})
#define BLENDMODE_BLEND_FULL set_cblend(cba,{VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD})
#define BLENDMODE_BLEND_PREMULTIPLIED_FULL set_cblend(cba,{VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD})
#define BLENDMODE_ADD_FULL set_cblend(cba,{VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD})
#define BLENDMODE_ADD_PREMULTIPLIED_FULL set_cblend(cba,{VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD})
#define BLENDMODE_MOD_FULL set_cblend(cba,{VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD})
#define BLENDMODE_MUL_FULL set_cblend(cba,{VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD})
#define BLENDMODE_SCREEN_FULL set_cblend(cba,{VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, VK_BLEND_OP_ADD})
#endif

	// Color blend
	void set_blend(VkPipelineColorBlendAttachmentState& colorBlendAttachment, uint32_t blendMode)
	{
		colorBlendAttachment = {};
		colorBlendAttachment.blendEnable = VK_TRUE;
		auto bm = (blendMode_e)blendMode;
		auto& cba = colorBlendAttachment;
		switch (bm)
		{
		case blendMode_e::none:
			BLENDMODE_NONE_FULL;
			colorBlendAttachment.blendEnable = VK_FALSE;
			break;
		case blendMode_e::normal:
			BLENDMODE_BLEND_FULL;
			break;
		case blendMode_e::additive:
			BLENDMODE_ADD_FULL;
			break;
		case blendMode_e::normal_prem:
			BLENDMODE_BLEND_PREMULTIPLIED_FULL;
			break;
		case blendMode_e::additive_prem:
			BLENDMODE_ADD_PREMULTIPLIED_FULL;
			break;
		case blendMode_e::multiply:
			BLENDMODE_MUL_FULL;
			break;
		case blendMode_e::modulate:
			BLENDMODE_MOD_FULL;
			break;
		case blendMode_e::screen:
			BLENDMODE_SCREEN_FULL;
			break;
		default:
			break;
		}
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}
	struct pipe_data {
		VkDevice dev = {};
		shadermodule_vf* spv = 0;
		VkRenderPass renderPass = {};
		uint32_t colorCount = 0;
		VkFormat colorFormat[8];
		VkFormat depthFormat;
		VkSampleCountFlags samples;
	};

	pipelinestate_p newPipelineState(pipe_data* pd, gem_info_t* info)
	{
		pipelinestate_p pipelineStates = {};
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkResult result = VK_SUCCESS;
		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
		VkVertexInputAttributeDescription attributeDescriptions[3];
		VkVertexInputBindingDescription bindingDescriptions[1];
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2];
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.flags = 0;
		pipelineCreateInfo.pStages = shaderStageCreateInfo;
		pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
		pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
		pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
		pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
		pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
		int shader = info->shader;
		// Shaders
		const char* name = "main";
		for (uint32_t i = 0; i < 2; i++) {
			shaderStageCreateInfo[i] = {};
			shaderStageCreateInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCreateInfo[i].module = (i == 0) ? pd->spv[shader].vert : pd->spv[shader].frag;
			shaderStageCreateInfo[i].stage = (i == 0) ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderStageCreateInfo[i].pName = name;
		}
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = &shaderStageCreateInfo[0];


		VkPipelineLayout pipelineLayout = {};
		VkDescriptorSetLayout descriptorSetLayout = {};
		VkPushConstantRange pushConstantRange[] = {
			{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4) + sizeof(float)},
			//{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(push_constants)}
		};
		std::array<VkDescriptorSetLayoutBinding, 3> laybs;
		VkDescriptorSetLayoutBinding layb_tex, layb_tex_mask, layb_ubo_ins;
		int binc = 1;
		layb_tex.binding = 0;	// 纹理
		layb_tex.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layb_tex.descriptorCount = 1;
		layb_tex.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layb_tex.pImmutableSamplers = NULL;
		if (shader == ST_MASK) {
			layb_tex_mask.binding = 1;	// 遮罩纹理
			layb_tex_mask.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layb_tex_mask.descriptorCount = 1;
			layb_tex_mask.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layb_tex_mask.pImmutableSamplers = NULL;
			binc++;
		}
		if (shader == ST_INSTANCE || shader == ST_INSTANCE_DOUBLESIDED) {
			layb_tex.binding = 1;
			layb_ubo_ins.binding = 0;	// 实例
			layb_ubo_ins.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			layb_ubo_ins.descriptorCount = 1;
			layb_ubo_ins.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			layb_ubo_ins.pImmutableSamplers = NULL;
			binc++;
		}
		VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
		descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_layout.pNext = NULL;
		descriptor_layout.bindingCount = (uint32_t)binc;
		descriptor_layout.pBindings = laybs.data();
		descriptor_layout.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
		result = vkCreateDescriptorSetLayout(pd->dev, &descriptor_layout, NULL, &descriptorSetLayout);
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = (VkPushConstantRange*)&pushConstantRange;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
		result = vkCreatePipelineLayout(pd->dev, &pipelineLayoutCreateInfo, NULL, &pipelineLayout);
		/*
		layout(location = 0) in vec3 pos;
		layout(location = 1) in vec2 uv;
		layout(location = 2) in vec4 col;
		layout(location = 3) in vec4 col1;
		*/
		VkVertexInputAttributeDescription vi_attrs[] =
		{
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
			{ 1, 0, VK_FORMAT_R32G32_SFLOAT, 12 },
			{ 2, 0, VK_FORMAT_R8G8B8A8_UNORM, 20 },
			{ 3, 0, VK_FORMAT_R8G8B8A8_UNORM, 24 },
		};
		//uint32_t    location;
		//uint32_t    binding;
		//VkFormat    format;
		//uint32_t    offset;
		//_countof(vi_attrs); 
		// Vertex input
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = 3 + shader;
		vertexInputCreateInfo.pVertexAttributeDescriptions = vi_attrs;
		vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescriptions[0];

		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		bindingDescriptions[0].stride = sizeof(float) * 5 + sizeof(int) * (shader + 1);

		// Input assembly
		inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCreateInfo.topology = (VkPrimitiveTopology)(info->topology > VK_PRIMITIVE_TOPOLOGY_PATCH_LIST ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : info->topology);
		inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.scissorCount = 1;
		viewportStateCreateInfo.viewportCount = 1;

		// Dynamic states
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		VkDynamicState dynamicStates[2] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		dynamicStateCreateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
		dynamicStateCreateInfo.pDynamicStates = dynamicStates;

		// Rasterization state
		rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
		rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
		rasterizationStateCreateInfo.polygonMode = (VkPolygonMode)(info->polygon > VK_POLYGON_MODE_POINT ? 0 : info->polygon);
		rasterizationStateCreateInfo.frontFace = (VkFrontFace)(info->frontFace > VK_FRONT_FACE_COUNTER_CLOCKWISE ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE);
		rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
		rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
		rasterizationStateCreateInfo.lineWidth = info->lineWidth;

		// MSAA state
		multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		VkSampleMask multiSampleMask = 0xFFFFFFFF;
		multisampleStateCreateInfo.pSampleMask = &multiSampleMask;
		multisampleStateCreateInfo.rasterizationSamples = (VkSampleCountFlagBits)pd->samples;

		// Depth Stencil
		auto& ds = depthStencilStateCreateInfo;
		ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		ds.pNext = NULL;
		ds.flags = 0;
		ds.depthTestEnable = info->flags & (uint8_t)depth_stencil_State::d_depthtest_enable;
		ds.depthWriteEnable = info->flags & (uint8_t)depth_stencil_State::d_depthwrite_enable;
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
		ds.stencilTestEnable = info->flags & (uint8_t)depth_stencil_State::d_stenciltest_enable;
		ds.front = ds.back;
		VkStencilOpState clipingOpState = { VK_STENCIL_OP_ZERO,
										VK_STENCIL_OP_REPLACE,
										VK_STENCIL_OP_KEEP,
										VK_COMPARE_OP_EQUAL,
										STENCIL_FILL_BIT,
										STENCIL_ALL_BIT,
										0x2 };
		//ds.back = ds.front = clipingOpState;
		// Color blend
		VkPipelineColorBlendAttachmentState colorBlendAttachment = { 0 };
		colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCreateInfo.attachmentCount = 1;
		colorBlendStateCreateInfo.pAttachments = &colorBlendAttachment;
		set_blend(colorBlendAttachment, info->blendMode);
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		// Renderpass / layout

		pipelineCreateInfo.subpass = 0;
		pipelineCreateInfo.layout = pipelineLayout;
		VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
		.colorAttachmentCount = pd->colorCount,
		.pColorAttachmentFormats = pd->colorFormat,
		.depthAttachmentFormat = pd->depthFormat,
		.stencilAttachmentFormat = pd->depthFormat
		};
		//if (info->dynamicrenderingEnable)
		pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
		//else
		//	pipelineCreateInfo.renderPass = pd->dev->renderPass;
		result = vkCreateGraphicsPipelines(pd->dev, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &pipeline);
		if (result != VK_SUCCESS) {
			return {};
		}

		pipelineStates.state = *info;
		pipelineStates.pipeline = pipeline;
		pipelineStates.descriptorSetLayout = descriptorSetLayout;
		pipelineStates.pipelineLayout = pipelineCreateInfo.layout;
		return pipelineStates;
	}
	void freePipelineState(VkDevice device, pipelinestate_p* pipelineStates)
	{
		if (pipelineStates->pipeline) {
			vkDestroyPipeline(device, pipelineStates->pipeline, NULL);
			pipelineStates->pipeline = VK_NULL_HANDLE;
		}
		if (pipelineStates->pipelineLayout) {
			vkDestroyPipelineLayout(device, pipelineStates->pipelineLayout, NULL);
			pipelineStates->pipelineLayout = VK_NULL_HANDLE;
		}
		if (pipelineStates->descriptorSetLayout) {
			vkDestroyDescriptorSetLayout(device, pipelineStates->descriptorSetLayout, NULL);
			pipelineStates->descriptorSetLayout = VK_NULL_HANDLE;
		}
	}
}
//!ovg

void ovg_ctx_t::destroy_geom() {

	for (size_t i = 0; i < 5; i++)
	{
		ovg::freeModule(dev->dev, shaderModule[i].vert);
		ovg::freeModule(dev->dev, shaderModule[i].frag);
		shaderModule[i] = {};
	}
	for (auto& [k, p] : pipelines) {
		ovg::freePipelineState((VkDevice)dev->dev, &p);
	}
}
void ovg_ctx_t::init_geom()
{
	for (size_t i = 0; i < 5; i++)
	{
		shaderModule[i] = { ovg::newModule(dev->dev, code[1 + i].vert, code_len[1 + i].vert, 0),
			ovg::newModule(dev->dev, code[1 + i].frag, code_len[1 + i].frag, 0) };
	}

}
pipelinestate_p ovg_ctx_t::new_spv_base(gem_info_t* info)
{
	ovg::pipe_data pd[1] = {};
	pd->dev = dev->dev;
	pd->spv = shaderModule;
	//pd->vert[0] = [0];
	//pd->frag[0] = shaderModule[1];
	//pd->vert[1] = shaderModule[2];
	//pd->frag[1] = shaderModule[3];
	pd->colorCount = 1;		// 一个输出随件
	pd->colorFormat[0] = colorFormat;
	pd->depthFormat = depthFormat;
	pd->samples = samples;
	auto p = ovg::newPipelineState(pd, info);

	return p;
}
#endif // 1
void ovg_ctx_t::bind_geom(VkCommandBuffer cmd, size_t offset, size_t ioffset)
{
	VkDeviceSize offsets[1] = { offset };
	vkCmdBindVertexBuffers(cmd, 0, 1, &res_geom.vertices.buffer, offsets);
	vkCmdBindIndexBuffer(cmd, res_geom.indices.buffer, ioffset, VK_INDEX_TYPE_UINT32);
}
void ovg_ctx_t::push_update_descriptor_set(VkCommandBuffer cmd, pipelinestate_p* cp, VkhImage img)
{
	if (!_vkCmdPushDescriptorSet || !img)return;
	VkDescriptorImageInfo descSrcTex = vkh_image_get_descriptor(img ? img : emptyImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	VkWriteDescriptorSet  writeDescriptorSet = {
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.dstSet = 0,
	.dstBinding = 1,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	.pImageInfo = &descSrcTex };
	_vkCmdPushDescriptorSet(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cp->pipelineLayout, 0, 1, &writeDescriptorSet);
}
void ovg_ctx_t::draw_geom(VkCommandBuffer cmd, geom_cmd_t* c)
{
	if (!c)return;
	auto cp = get_state(&c->state);
	if (cp)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cp->pipeline);
		vkCmdPushConstants(cmd, cp->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0
			, sizeof(glm::mat4) + sizeof(float), &c->mat);
		push_update_descriptor_set(cmd, cp, (VkhImage)c->texture);
	}
	bind_geom(cmd, c->v_offset * v2offset, 0);
	if (c->firstIndex == -1)
		vkCmdDraw(cmd, c->elemCount, 1, c->vertexOffset, 0);
	else
		vkCmdDrawIndexed(cmd, c->elemCount, 1, c->firstIndex, c->vertexOffset, 0);
}
const VkClearAttachment clearStencil = { VK_IMAGE_ASPECT_STENCIL_BIT, 1, {{{1,0}}} };
const VkClearAttachment clearColorAttach = { VK_IMAGE_ASPECT_COLOR_BIT, 0, {{{0}}} };

void ovg_ctx_t::draw_vg(VkCommandBuffer cmd, vgcmd_t* c, VkRect2D& cuclip)
{
	auto t = c->state;
	if (t)
	{
		bind_draw_pipeline(cmd, t);
		update_pattern(cmd, t->pattern, t);
		update_push_constants(cmd, t);
		if (!cmdStarted)
			cmdStarted = true;
		push_update_descriptor_set_a(cmd, d_img, d_offset);
	}
	switch (c->type)
	{
	case 0:// 填充
	{
		if (t && t->curFillRule == VG_FILL_RULE_EVEN_ODD) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinePolyFill);// 奇偶填充
			for (size_t i = 0; i < c->vc; i++) { vkCmdDraw(cmd, c->v[i].vertexCount, 1, c->v[i].firstVertex, 0); }
			bind_draw_pipeline(cmd, t);
			vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
			cmd_draw_full_screen_quad(cmd, c, &c->bounds, &cuclip);
			vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
		}
		else { vkCmdDrawIndexed(cmd, c->index.y, 1, c->index.x, (int32_t)c->vertex.x, 0); }// 非零填充
	}
	break;
	case 1: // 描边
	{
		vkCmdDrawIndexed(cmd, c->index.y, 1, c->index.x, (int32_t)c->vertex.x, 0);
	}
	break;
	case 2:// 裁剪
	{
		int bw = c->bounds.z; int bh = c->bounds.w;
		if (bw != 0 && bh != 0) { cuclip = set_scissor(cmd, bw < 0 || bh < 0 ? nullptr : &c->bounds); break; }
		if (c->vc > 0 || c->index.y > 0) {
#if defined(DEBUG) 
			vkh_cmd_label_start(cmd, "clip", DBG_LAB_COLOR_CLIP);
#endif
			if (t && t->curFillRule == VG_FILL_RULE_EVEN_ODD) {
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinePolyFill);
				for (size_t i = 0; i < c->vc; i++) { vkCmdDraw(cmd, c->v[i].vertexCount, 1, c->v[i].firstVertex, 0); }
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineClipping);
			}
			else {
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineClipping);
				vkCmdSetStencilReference(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
				vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
				vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
				vkCmdDrawIndexed(cmd, c->index.y, 1, c->index.x, (int32_t)c->vertex.x, 0);
			}
			vkCmdSetStencilReference(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_CLIP_BIT);
			vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_FILL_BIT);
			vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FRONT_AND_BACK, STENCIL_ALL_BIT);
			cmd_draw_full_screen_quad(cmd, c, NULL, 0);
			curClipState = vg_clip_state_clip;
#if defined(DEBUG)  
			vkh_cmd_label_end(cmd);
#endif
		}
		else {
			auto cs = clearStencil;
			cs.clearValue.depthStencil.depth = 1;
			cs.clearValue.depthStencil.stencil = 0;
			vkCmdClearAttachments(cmd, 1, &cs, 1, &clearRect);
		}
	}
	break;
	case 3:
	{
		cmd_draw_full_screen_quad(cmd, c, NULL, 0);
	}
	break;
	case 4:
	{
		VkClearAttachment ca[2] = { clearColorAttach, clearStencil };
		ca[1].clearValue.depthStencil.depth = 1;
		glm::ivec4 ccrt = c->bounds;
		VkClearRect cr = clearRect;
		if (ccrt.z > 0 && ccrt.w > 0) {
			if (ccrt.x < 0)
			{
				ccrt.z += ccrt.x; ccrt.x = 0;
			}
			if (ccrt.y < 0)
			{
				ccrt.w += ccrt.y; ccrt.y = 0;
			}
			cr.rect.offset = { ccrt.x,ccrt.y };
			cr.rect.extent = { std::min((uint32_t)ccrt.z,cr.rect.extent.width - ccrt.x), std::min((uint32_t)ccrt.w,cr.rect.extent.height - ccrt.y) };
		}
		vkCmdClearAttachments(cmd, 2, ca, 1, &cr);
	}
	break;
	}
}

// todo gpu

static SDL_GPUShader* compileShader(SDL_GPUDevice* device, SDL_GPUShaderStage stage)
{
	SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
	// SDL_GPU_SHADERSTAGE_VERTEX,
	// SDL_GPU_SHADERSTAGE_FRAGMENT
	SDL_GPUShaderCreateInfo sci = { };
	//sci.code = code[stage];
	//sci.code_size = code_len[stage];
	sci.format = SDL_GPU_SHADERFORMAT_SPIRV;
	// FIXME not sure if this is correctstage ? "fragMain" :
	sci.entrypoint = "main";
	sci.num_samplers = stage;
	sci.num_uniform_buffers = stage;
	sci.stage = stage;

	return SDL_CreateGPUShader(device, &sci);
}
void* new_gpu()
{
	SDL_PropertiesID create_props = SDL_CreateProperties();
	bool debug = SDL_GetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, false);
	bool lowpower = SDL_GetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_PREFERLOWPOWER_BOOLEAN, false);

	debug = SDL_GetHintBoolean(SDL_HINT_RENDER_GPU_DEBUG, debug);
	lowpower = SDL_GetHintBoolean(SDL_HINT_RENDER_GPU_LOW_POWER, lowpower);

	SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, debug);
	SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_PREFERLOWPOWER_BOOLEAN, lowpower);

	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING))
	{
		SDL_SetStringProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "vulkan");
	}
	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_D3D12_ALLOW_FEWER_RESOURCE_SLOTS_BOOLEAN)) {
		SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_D3D12_ALLOW_FEWER_RESOURCE_SLOTS_BOOLEAN, true);
	}
	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN)) {
		SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN, false);
	}
	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_DEPTH_CLAMPING_BOOLEAN)) {
		SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_DEPTH_CLAMPING_BOOLEAN, false);
	}
	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN)) {
		SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN, false);
	}
	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_ANISOTROPY_BOOLEAN)) {
		SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_ANISOTROPY_BOOLEAN, false);
	}
	if (!SDL_HasProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_METAL_ALLOW_MACFAMILY1_BOOLEAN)) {
		SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_METAL_ALLOW_MACFAMILY1_BOOLEAN, false);
	}
	SDL_SetBooleanProperty(create_props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, 1);
	auto device = SDL_CreateGPUDeviceWithProperties(create_props);
	if (!device) return 0;
	SDL_GPUShader* v = compileShader(device, SDL_GPUShaderStage::SDL_GPU_SHADERSTAGE_VERTEX);
	SDL_GPUShader* f = compileShader(device, SDL_GPUShaderStage::SDL_GPU_SHADERSTAGE_FRAGMENT);
	SDL_GPUGraphicsPipelineCreateInfo pipelinedesc = {};
	SDL_GPUColorTargetDescription color_target_desc = {};
	pipelinedesc.target_info.num_color_targets = 1;
	pipelinedesc.target_info.color_target_descriptions = &color_target_desc;
	pipelinedesc.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
	pipelinedesc.target_info.has_depth_stencil_target = true;

	pipelinedesc.depth_stencil_state.enable_depth_test = true;
	pipelinedesc.depth_stencil_state.enable_depth_write = true;
	pipelinedesc.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

	pipelinedesc.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;

	pipelinedesc.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

	pipelinedesc.vertex_shader = v;
	pipelinedesc.fragment_shader = f;
	SDL_GPUVertexBufferDescription vertex_buffer_desc = {};
	vertex_buffer_desc.slot = 0;
	vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	vertex_buffer_desc.instance_step_rate = 0;
	vertex_buffer_desc.pitch = sizeof(float) * 9;
	SDL_GPUVertexAttribute vertex_attributes[3] = {};
	vertex_attributes[0].buffer_slot = 0;
	vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
	vertex_attributes[0].location = 0;
	vertex_attributes[0].offset = 0;

	vertex_attributes[1].buffer_slot = 0;
	vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertex_attributes[1].location = 1;
	vertex_attributes[1].offset = sizeof(float) * 3;

	vertex_attributes[1].buffer_slot = 0;
	vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertex_attributes[1].location = 2;
	vertex_attributes[1].offset = sizeof(float) * 5;

	pipelinedesc.vertex_input_state.num_vertex_buffers = 1;
	pipelinedesc.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
	pipelinedesc.vertex_input_state.num_vertex_attributes = 2;
	pipelinedesc.vertex_input_state.vertex_attributes = (SDL_GPUVertexAttribute*)&vertex_attributes;

	pipelinedesc.props = 0;

	auto pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelinedesc);
	return device;
}
#endif // 1
// todo vk 
#ifndef NOT_VKH_CPP

#ifndef VKH_USE_VMA
void _set_size_and_bind(VkhDevice pDev, VkBufferUsageFlags usage, VkhMemoryUsage memoryUsage, VkDeviceSize size, VkhBuffer buff) {
	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(pDev->dev, buff->buffer, &memReq);
	VkMemoryAllocateInfo memAllocInfo = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
										  .allocationSize = memReq.size };
	assert(vkh_memory_type_from_properties(&pDev->phyMemProps, memReq.memoryTypeBits, memoryUsage, &memAllocInfo.memoryTypeIndex) == true);
	VK_CHECK_RESULT(vkAllocateMemory(pDev->dev, &memAllocInfo, NULL, &buff->memory));

	buff->alignment = memReq.alignment;
	buff->size = memAllocInfo.allocationSize;
	buff->usageFlags = usage;
	buff->memprops = memoryUsage;

	VK_CHECK_RESULT(vkBindBufferMemory(buff->pDev->dev, buff->buffer, buff->memory, 0));
}
#endif

void vkh_buffer_init(VkhDevice pDev, VkBufferUsageFlags usage, VkhMemoryUsage memprops, VkDeviceSize size, VkhBuffer buff, bool mapped) {
	buff->pDev = pDev;
	VkBufferCreateInfo* pInfo = &buff->infos;
	pInfo->sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	pInfo->usage = usage;
	pInfo->size = size;
	pInfo->sharingMode = VK_SHARING_MODE_EXCLUSIVE;
#ifdef VKH_USE_VMA	
	buff->allocCreateInfo.usage = (VmaMemoryUsage)memprops;
	if (mapped)
		buff->allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	VK_CHECK_RESULT(vmaCreateBuffer(pDev->allocator, pInfo, &buff->allocCreateInfo, &buff->buffer, &buff->alloc, &buff->allocInfo));
#else
	VK_CHECK_RESULT(vkCreateBuffer(pDev->dev, pInfo, NULL, &buff->buffer));
	_set_size_and_bind(pDev, usage, memprops, size, buff);
	buff->memprops = memprops;
	if (mapped)
		VK_CHECK_RESULT(vkMapMemory(buff->pDev->dev, buff->memory, 0, VK_WHOLE_SIZE, 0, &buff->mapped));
#endif
}

VkhBuffer vkh_buffer_create(VkhDevice pDev, VkBufferUsageFlags usage, VkhMemoryUsage memprops, VkDeviceSize size) {
	VkhBuffer buff = (VkhBuffer)calloc(1, sizeof(vkh_buffer_t));
	vkh_buffer_init(pDev, usage, memprops, size, buff, false);
	return buff;
}

void vkh_buffer_reset(VkhBuffer buff) {
	if (buff->buffer)
#ifdef VKH_USE_VMA
		vmaDestroyBuffer(buff->pDev->allocator, buff->buffer, buff->alloc);
#else
		vkDestroyBuffer(buff->pDev->dev, buff->buffer, NULL);
	if (buff->memory)
		vkFreeMemory(buff->pDev->dev, buff->memory, NULL);
#endif
}
void vkh_buffer_destroy(VkhBuffer buff) {
	if (buff->buffer)
#ifdef VKH_USE_VMA
		vmaDestroyBuffer(buff->pDev->allocator, buff->buffer, buff->alloc);
#else
		vkDestroyBuffer(buff->pDev->dev, buff->buffer, NULL);
	if (buff->memory)
		vkFreeMemory(buff->pDev->dev, buff->memory, NULL);
#endif
	free(buff);
	buff = NULL;
}
void vkh_buffer_resize(VkhBuffer buff, VkDeviceSize newSize, bool mapped) {
	vkh_buffer_reset(buff);
	buff->infos.size = newSize;
#ifdef VKH_USE_VMA
	VK_CHECK_RESULT(vmaCreateBuffer(buff->pDev->allocator, &buff->infos, &buff->allocCreateInfo, &buff->buffer, &buff->alloc, &buff->allocInfo));
#else
	VK_CHECK_RESULT(vkCreateBuffer(buff->pDev->dev, &buff->infos, NULL, &buff->buffer));
	_set_size_and_bind(buff->pDev, buff->usageFlags, buff->memprops, buff->infos.size, buff);
	if (mapped)
		VK_CHECK_RESULT(vkMapMemory(buff->pDev->dev, buff->memory, 0, VK_WHOLE_SIZE, 0, &buff->mapped));
#endif
}

VkDescriptorBufferInfo vkh_buffer_get_descriptor(VkhBuffer buff) {
	VkDescriptorBufferInfo desc = {
		.buffer = buff->buffer,
		.offset = 0,
		.range = VK_WHOLE_SIZE };
	return desc;
}


VkResult vkh_buffer_map(VkhBuffer buff) {
#ifdef VKH_USE_VMA
	return vmaMapMemory(buff->pDev->allocator, buff->alloc, &buff->mapped);
#else
	return vkMapMemory(buff->pDev->dev, buff->memory, 0, VK_WHOLE_SIZE, 0, &buff->mapped);
#endif
}
void vkh_buffer_unmap(VkhBuffer buff) {
#ifdef VKH_USE_VMA
	vmaUnmapMemory(buff->pDev->allocator, buff->alloc);
#else
	if (!buff->mapped)
		return;
	vkUnmapMemory(buff->pDev->dev, buff->memory);
	buff->mapped = NULL;
#endif
}
VkBuffer vkh_buffer_get_vkbuffer(VkhBuffer buff) {
	return buff->buffer;
}
void* vkh_buffer_get_mapped_pointer(VkhBuffer buff) {
#ifdef VKH_USE_VMA
	//vmaFlushAllocation (buff->pDev->allocator, buff->alloc, buff->allocInfo.offset, buff->allocInfo.size);
	return buff->allocInfo.pMappedData;
#else
	return buff->mapped;
#endif
}
void vkh_buffer_flush(VkhBuffer buff) {
#ifdef VKH_USE_VMA
	vmaFlushAllocation(buff->pDev->allocator, buff->alloc, buff->allocInfo.offset, buff->allocInfo.size);
#else
#endif
}
#include "vkh_phyinfo.h"

VkhQueue _init_queue(VkhDevice dev) {
	VkhQueue q = (vkh_queue_t*)calloc(1, sizeof(vkh_queue_t));
	q->dev = dev;
	return q;
}


VkhQueue vkh_queue_create(VkhDevice dev, uint32_t familyIndex, uint32_t qIndex) {
	VkhQueue q = _init_queue(dev);
	q->familyIndex = familyIndex;
	vkGetDeviceQueue(dev->dev, familyIndex, qIndex, &q->queue);
	return q;
}
void vkh_queue_destroy(VkhQueue queue) {
	free(queue);
}

VkhPhyInfo vkh_phyinfo_create(VkPhysicalDevice phy, VkSurfaceKHR surface) {
	VkhPhyInfo pi = (vkh_phy_t*)calloc(1, sizeof(vkh_phy_t));
	pi->phy = phy;

	vkGetPhysicalDeviceProperties(phy, &pi->properties);
	vkGetPhysicalDeviceMemoryProperties(phy, &pi->memProps);

	vkGetPhysicalDeviceQueueFamilyProperties(phy, &pi->queueCount, NULL);
	pi->queues = (VkQueueFamilyProperties*)malloc(pi->queueCount * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(phy, &pi->queueCount, pi->queues);

	//identify dedicated queues

	pi->cQueue = -1;
	pi->gQueue = -1;
	pi->tQueue = -1;
	pi->pQueue = -1;

	//try to find dedicated queues first
	for (uint32_t j = 0; j < pi->queueCount; j++) {
		VkBool32 present = VK_FALSE;
		switch (pi->queues[j].queueFlags) {
		case VK_QUEUE_GRAPHICS_BIT:
			if (surface)
				vkGetPhysicalDeviceSurfaceSupportKHR(phy, j, surface, &present);
			if (present) {
				if (pi->pQueue < 0)
					pi->pQueue = j;
			}
			else if (pi->gQueue < 0)
				pi->gQueue = j;
			break;
		case VK_QUEUE_COMPUTE_BIT:
			if (pi->cQueue < 0)
				pi->cQueue = j;
			break;
		case VK_QUEUE_TRANSFER_BIT:
			if (pi->tQueue < 0)
				pi->tQueue = j;
			break;
		}
	}
	//try to find suitable queue if no dedicated one found
	for (uint32_t j = 0; j < pi->queueCount; j++) {
		if (pi->queues[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			VkBool32 present = 0;
			if (surface)
				vkGetPhysicalDeviceSurfaceSupportKHR(phy, j, surface, &present);
			//printf ("surf=%d, q=%d, present=%d\n",surface,j,present);
			if (present) {
				if (pi->pQueue < 0)
					pi->pQueue = j;
			}
			else if (pi->gQueue < 0)
				pi->gQueue = j;
		}
		if ((pi->queues[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (pi->gQueue < 0))
			pi->gQueue = j;
		if ((pi->queues[j].queueFlags & VK_QUEUE_COMPUTE_BIT) && (pi->cQueue < 0))
			pi->cQueue = j;
		if ((pi->queues[j].queueFlags & VK_QUEUE_TRANSFER_BIT) && (pi->tQueue < 0))
			pi->tQueue = j;
	}

	return pi;
}

void vkh_phyinfo_destroy(VkhPhyInfo phy) {
	if (phy->pExtensionProperties != NULL)
		free(phy->pExtensionProperties);
	free(phy->queues);
	free(phy);
}

VkPhysicalDeviceProperties vkh_phyinfo_get_properties(VkhPhyInfo phy) {
	return phy->properties;
}
VkPhysicalDeviceMemoryProperties vkh_phyinfo_get_memory_properties(VkhPhyInfo phy) {
	return phy->memProps;
}

void vkh_phyinfo_get_queue_fam_indices(VkhPhyInfo phy, int* pQueue, int* gQueue, int* tQueue, int* cQueue) {
	if (pQueue)	*pQueue = phy->pQueue;
	if (gQueue)	*gQueue = phy->gQueue;
	if (tQueue)	*tQueue = phy->tQueue;
	if (cQueue)	*cQueue = phy->cQueue;
}
VkQueueFamilyProperties* vkh_phyinfo_get_queues_props(VkhPhyInfo phy, uint32_t* qCount) {
	*qCount = phy->queueCount;
	return phy->queues;
}
bool vkh_phyinfo_create_queues(VkhPhyInfo phy, int qFam, uint32_t queueCount, const float* queue_priorities, VkDeviceQueueCreateInfo* const qInfo) {
	qInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	if (phy->queues[qFam].queueCount < queueCount)
		fprintf(stderr, "Request %d queues of family %d, but only %d available\n", queueCount, qFam, phy->queues[qFam].queueCount);
	else {
		qInfo->queueCount = queueCount,
			qInfo->queueFamilyIndex = qFam,
			qInfo->pQueuePriorities = queue_priorities;
		phy->queues[qFam].queueCount -= queueCount;
		return true;
	}
	return false;
}
bool vkh_phyinfo_create_presentable_queues(VkhPhyInfo phy, uint32_t queueCount, const float* queue_priorities, VkDeviceQueueCreateInfo* const qInfo) {
	qInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	if (phy->pQueue < 0)
		perror("No queue with presentable support found");
	else if (phy->queues[phy->pQueue].queueCount < queueCount)
		fprintf(stderr, "Request %d queues of family %d, but only %d available\n", queueCount, phy->pQueue, phy->queues[phy->pQueue].queueCount);
	else {
		qInfo->queueCount = queueCount,
			qInfo->queueFamilyIndex = phy->pQueue,
			qInfo->pQueuePriorities = queue_priorities;
		phy->queues[phy->pQueue].queueCount -= queueCount;
		return true;
	}
	return false;
}
bool vkh_phyinfo_create_transfer_queues(VkhPhyInfo phy, uint32_t queueCount, const float* queue_priorities, VkDeviceQueueCreateInfo* const qInfo) {
	qInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	if (phy->tQueue < 0)
		perror("No transfer queue found");
	else if (phy->queues[phy->tQueue].queueCount < queueCount)
		fprintf(stderr, "Request %d transfer queues of family %d, but only %d available\n", queueCount, phy->tQueue, phy->queues[phy->tQueue].queueCount);
	else {
		qInfo->queueCount = queueCount;
		qInfo->queueFamilyIndex = phy->tQueue;
		qInfo->pQueuePriorities = queue_priorities;
		phy->queues[phy->tQueue].queueCount -= queueCount;
		return true;
	}
	return false;
}
bool vkh_phyinfo_create_compute_queues(VkhPhyInfo phy, uint32_t queueCount, const float* queue_priorities, VkDeviceQueueCreateInfo* const qInfo) {
	qInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	if (phy->cQueue < 0)
		perror("No compute queue found");
	else if (phy->queues[phy->cQueue].queueCount < queueCount)
		fprintf(stderr, "Request %d compute queues of family %d, but only %d available\n", queueCount, phy->cQueue, phy->queues[phy->cQueue].queueCount);
	else {
		qInfo->queueCount = queueCount,
			qInfo->queueFamilyIndex = phy->cQueue,
			qInfo->pQueuePriorities = queue_priorities;
		phy->queues[phy->cQueue].queueCount -= queueCount;
		return true;
	}
	return false;
}
bool vkh_phy_info_create_graphic_queues(VkhPhyInfo phy, uint32_t queueCount, const float* queue_priorities, VkDeviceQueueCreateInfo* const qInfo) {
	qInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	if (phy->gQueue < 0)
		perror("No graphic queue found");
	else if (phy->queues[phy->gQueue].queueCount < queueCount)
		fprintf(stderr, "Request %d graphic queues of family %d, but only %d available\n", queueCount, phy->gQueue, phy->queues[phy->gQueue].queueCount);
	else {
		qInfo->queueCount = queueCount,
			qInfo->queueFamilyIndex = phy->gQueue,
			qInfo->pQueuePriorities = queue_priorities;
		phy->queues[phy->gQueue].queueCount -= queueCount;
		return true;
	}
	return false;
}
bool vkh_phyinfo_try_get_extension_properties(VkhPhyInfo phy, const char* name, const VkExtensionProperties* properties) {
	if (phy->pExtensionProperties == NULL) {
		VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(phy->phy, NULL, &phy->extensionCount, NULL));
		phy->pExtensionProperties = (VkExtensionProperties*)malloc(phy->extensionCount * sizeof(VkExtensionProperties));
		VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(phy->phy, NULL, &phy->extensionCount, phy->pExtensionProperties));
	}
	for (uint32_t i = 0; i < phy->extensionCount; i++) {
		if (strcmp(name, phy->pExtensionProperties[i].extensionName) == 0) {
			if (properties)
				properties = &phy->pExtensionProperties[i];
			return true;
		}
	}
	properties = NULL;
	return false;
}

VkhImage _vkh_image_create(VkhDevice pDev, VkImageType imageType,
	VkFormat format, uint32_t width, uint32_t height,
	VkhMemoryUsage memprops, VkImageUsageFlags usage,
	VkSampleCountFlagBits samples, VkImageTiling tiling,
	uint32_t mipLevels, uint32_t arrayLayers) {

	VkhImage img = (VkhImage)calloc(1, sizeof(vkh_image_t));

	img->pDev = pDev;

	VkImageCreateInfo* pInfo = &img->infos;
	pInfo->sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	pInfo->imageType = imageType;
	pInfo->tiling = tiling;
	pInfo->initialLayout = (tiling == VK_IMAGE_TILING_OPTIMAL) ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PREINITIALIZED;
	pInfo->sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	pInfo->usage = usage;
	pInfo->format = format;
	pInfo->extent.width = width;
	pInfo->extent.height = height;
	pInfo->extent.depth = 1;
	pInfo->mipLevels = mipLevels;
	pInfo->arrayLayers = arrayLayers;
	pInfo->samples = samples;

	/*
	img->imported = false;
	img->alloc	= VK_NULL_HANDLE;
	img->image	= VK_NULL_HANDLE;
	img->sampler= VK_NULL_HANDLE;
	img->view	= VK_NULL_HANDLE;*/
#ifdef VKH_USE_VMA
	VmaAllocationCreateInfo allocInfo = { .usage = (VmaMemoryUsage)memprops };
	VK_CHECK_RESULT(vmaCreateImage(pDev->allocator, pInfo, &allocInfo, &img->image, &img->alloc, &img->allocInfo));
#else
	VK_CHECK_RESULT(vkCreateImage(pDev->dev, pInfo, NULL, &img->image));
	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(pDev->dev, img->image, &memReq);
	VkMemoryAllocateInfo memAllocInfo = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
										  .allocationSize = memReq.size };
	vkh_memory_type_from_properties(&pDev->phyMemProps, memReq.memoryTypeBits, memprops, &memAllocInfo.memoryTypeIndex);
	VK_CHECK_RESULT(vkAllocateMemory(pDev->dev, &memAllocInfo, NULL, &img->memory));
	VK_CHECK_RESULT(vkBindImageMemory(pDev->dev, img->image, img->memory, 0));
#endif

	mtx_init(&img->mutex, mtx_plain);
	img->references = 1;

	return img;
}
void vkh_image_destroy(VkhImage img)
{
	if (img == NULL)
		return;

	mtx_lock(&img->mutex);
	img->references--;
	if (img->references > 0) {
		mtx_unlock(&img->mutex);
		return;
	}

	mtx_unlock(&img->mutex);
	mtx_destroy(&img->mutex);

	if (img->view != VK_NULL_HANDLE)
		vkDestroyImageView(img->pDev->dev, img->view, NULL);
	if (img->sampler != VK_NULL_HANDLE)
		vkDestroySampler(img->pDev->dev, img->sampler, NULL);

	if (!img->imported) {
#ifdef VKH_USE_VMA
		vmaDestroyImage(img->pDev->allocator, img->image, img->alloc);
#else
		vkDestroyImage(img->pDev->dev, img->image, NULL);
		vkFreeMemory(img->pDev->dev, img->memory, NULL);

#endif
	}


	free(img);
	img = NULL;
}
void vkh_image_reference(VkhImage img) {
	mtx_lock(&img->mutex);
	img->references++;
	mtx_unlock(&img->mutex);
}
VkhImage vkh_tex2d_array_create(VkhDevice pDev,
	VkFormat format, uint32_t width, uint32_t height, uint32_t layers,
	VkhMemoryUsage memprops, VkImageUsageFlags usage) {
	return _vkh_image_create(pDev, VK_IMAGE_TYPE_2D, format, width, height, memprops, usage,
		VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, 1, layers);
}
VkhImage vkh_image_create(VkhDevice pDev,
	VkFormat format, uint32_t width, uint32_t height, VkImageTiling tiling,
	VkhMemoryUsage memprops,
	VkImageUsageFlags usage)
{
	return _vkh_image_create(pDev, VK_IMAGE_TYPE_2D, format, width, height, memprops, usage,
		VK_SAMPLE_COUNT_1_BIT, tiling, 1, 1);
}
//create vkhImage from existing VkImage
VkhImage vkh_image_import(VkhDevice pDev, VkImage vkImg, VkFormat format, uint32_t width, uint32_t height) {
	VkhImage img = (VkhImage)calloc(1, sizeof(vkh_image_t));
	img->pDev = pDev;
	img->image = vkImg;
	img->imported = true;

	VkImageCreateInfo* pInfo = &img->infos;
	pInfo->imageType = VK_IMAGE_TYPE_2D;
	pInfo->format = format;
	pInfo->extent.width = width;
	pInfo->extent.height = height;
	pInfo->extent.depth = 1;
	pInfo->mipLevels = 1;
	pInfo->arrayLayers = 1;
	//pInfo->samples		= samples;
	img->references = 1;

	mtx_init(&img->mutex, mtx_plain);

	return img;
}
VkhImage vkh_image_ms_create(VkhDevice pDev,
	VkFormat format, VkSampleCountFlagBits num_samples, uint32_t width, uint32_t height,
	VkhMemoryUsage memprops,
	VkImageUsageFlags usage) {
	return  _vkh_image_create(pDev, VK_IMAGE_TYPE_2D, format, width, height, memprops, usage,
		num_samples, VK_IMAGE_TILING_OPTIMAL, 1, 1);
}
void vkh_image_create_view(VkhImage img, VkImageViewType viewType, VkImageAspectFlags aspectFlags) {
	if (img->view != VK_NULL_HANDLE)
		vkDestroyImageView(img->pDev->dev, img->view, NULL);

	VkImageViewCreateInfo viewInfo = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
										 .image = img->image,
										 .viewType = viewType,
										 .format = img->infos.format,
										 .components = {VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_G,VK_COMPONENT_SWIZZLE_B,VK_COMPONENT_SWIZZLE_A},
										 .subresourceRange = {aspectFlags,0,1,0,img->infos.arrayLayers} };
	VK_CHECK_RESULT(vkCreateImageView(img->pDev->dev, &viewInfo, NULL, &img->view));
}
void vkh_image_create_sampler(VkhImage img, VkFilter magFilter, VkFilter minFilter,
	VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode) {
	if (img->sampler != VK_NULL_HANDLE)
		vkDestroySampler(img->pDev->dev, img->sampler, NULL);
	VkSamplerCreateInfo samplerCreateInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = magFilter,
		.minFilter = minFilter,
		.mipmapMode = mipmapMode,
		.addressModeU = addressMode,
		.addressModeV = addressMode,
		.addressModeW = addressMode,
		.maxAnisotropy = 1.0,
	};
	VK_CHECK_RESULT(vkCreateSampler(img->pDev->dev, &samplerCreateInfo, NULL, &img->sampler));
}
void vkh_image_set_sampler(VkhImage img, VkSampler sampler) {
	img->sampler = sampler;
}
void vkh_image_create_descriptor(VkhImage img, VkImageViewType viewType, VkImageAspectFlags aspectFlags, VkFilter magFilter,
	VkFilter minFilter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode)
{
	vkh_image_create_view(img, viewType, aspectFlags);
	vkh_image_create_sampler(img, magFilter, minFilter, mipmapMode, addressMode);
}
VkImage vkh_image_get_vkimage(VkhImage img) {
	return img->image;
}
VkSampler vkh_image_get_sampler(VkhImage img) {
	if (img == NULL)
		return NULL;
	return img->sampler;
}
VkImageView vkh_image_get_view(VkhImage img) {
	if (img == NULL)
		return NULL;
	return img->view;
}
VkImageLayout vkh_image_get_layout(VkhImage img) {
	if (img == NULL)
		return VK_IMAGE_LAYOUT_UNDEFINED;
	return img->layout;
}
VkDescriptorImageInfo vkh_image_get_descriptor(VkhImage img, VkImageLayout imageLayout) {
	VkDescriptorImageInfo desc = { .sampler = img->sampler,.imageView = img->view,
								   .imageLayout = imageLayout

	};
	return desc;
}

void vkh_image_set_layout(VkCommandBuffer cmdBuff, VkhImage image, VkImageAspectFlags aspectMask,
	VkImageLayout old_image_layout, VkImageLayout new_image_layout,
	VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages) {
	VkImageSubresourceRange subres = { aspectMask,0,1,0,1 };
	vkh_image_set_layout_subres(cmdBuff, image, subres, old_image_layout, new_image_layout, src_stages, dest_stages);
}
// This method is based on https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.h#L88
void vkh_image_set_layout_subres(VkCommandBuffer cmdBuff, VkhImage image, VkImageSubresourceRange subresourceRange,
	VkImageLayout old_image_layout, VkImageLayout new_image_layout,
	VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages) {
	VkImageMemoryBarrier image_memory_barrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
												  .oldLayout = image->layout,
												  .newLayout = new_image_layout,
												  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												  .image = image->image,
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
	image->layout = new_image_layout;
}
void vkh_image_destroy_sampler(VkhImage img) {
	if (img == NULL)
		return;
	if (img->sampler != VK_NULL_HANDLE)
		vkDestroySampler(img->pDev->dev, img->sampler, NULL);
	img->sampler = VK_NULL_HANDLE;
}

void* vkh_image_map(VkhImage img) {
	void* data;
#ifdef VKH_USE_VMA
	vmaMapMemory(img->pDev->allocator, img->alloc, &data);
#else
#endif
	return data;
}
void vkh_image_unmap(VkhImage img) {
#ifdef VKH_USE_VMA
	vmaUnmapMemory(img->pDev->allocator, img->alloc);
#else
#endif
}
void vkh_image_set_name(VkhImage img, const char* name) {
	if (img == NULL)
		return;
	vkh_device_set_object_name(img->pDev, VK_OBJECT_TYPE_IMAGE, (uint64_t)img->image, name);
}
uint64_t vkh_image_get_stride(VkhImage img) {
	VkImageSubresource subres = { VK_IMAGE_ASPECT_COLOR_BIT,0,0 };
	VkSubresourceLayout layout = { 0 };
	vkGetImageSubresourceLayout(img->pDev->dev, img->image, &subres, &layout);
	return (uint64_t)layout.rowPitch;
}

static PFN_vkSetDebugUtilsObjectNameEXT		SetDebugUtilsObjectNameEXT;
static PFN_vkQueueBeginDebugUtilsLabelEXT	QueueBeginDebugUtilsLabelEXT;
static PFN_vkQueueEndDebugUtilsLabelEXT		QueueEndDebugUtilsLabelEXT;
static PFN_vkCmdBeginDebugUtilsLabelEXT		CmdBeginDebugUtilsLabelEXT;
static PFN_vkCmdEndDebugUtilsLabelEXT		CmdEndDebugUtilsLabelEXT;
static PFN_vkCmdInsertDebugUtilsLabelEXT	CmdInsertDebugUtilsLabelEXT;

VkhDevice vkh_device_create(VkhApp app, VkhPhyInfo phyInfo, VkDeviceCreateInfo* pDevice_info) {
	VkDevice dev;
	VK_CHECK_RESULT(vkCreateDevice(phyInfo->phy, pDevice_info, NULL, &dev));
	VkhDevice vkhd = vkh_device_import(app->inst, phyInfo->phy, dev);
	vkhd->vkhApplication = app;
	return vkhd;
}
VkhDevice vkh_device_import(VkInstance inst, VkPhysicalDevice phy, VkDevice vkDev) {
	VkhDevice dev = (vkh_device_t*)calloc(1, sizeof(vkh_device_t));
	dev->dev = vkDev;
	dev->phy = phy;
	dev->instance = inst;

	vkGetPhysicalDeviceMemoryProperties(phy, &dev->phyMemProps);
#ifdef VKH_USE_VMA
	VmaAllocatorCreateInfo allocatorInfo = {
		.physicalDevice = phy,
		.device = vkDev
	};
	vmaCreateAllocator(&allocatorInfo, &dev->allocator);
#else
#endif

	return dev;
}
VkDevice vkh_device_get_vkdev(VkhDevice dev) {
	return dev->dev;
}
VkPhysicalDevice vkh_device_get_phy(VkhDevice dev) {
	return dev->phy;
}
VkhApp vkh_device_get_app(VkhDevice dev) {
	return dev->vkhApplication;
}
/**
 * @brief get instance proc addresses for debug utils (name, label,...)
 * @param vkh device
 */
void vkh_device_init_debug_utils(VkhDevice dev) {
	SetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(dev->instance, "vkSetDebugUtilsObjectNameEXT");
	QueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(dev->instance, "vkQueueBeginDebugUtilsLabelEXT");
	QueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(dev->instance, "vkQueueEndDebugUtilsLabelEXT");
	CmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(dev->instance, "vkCmdBeginDebugUtilsLabelEXT");
	CmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(dev->instance, "vkCmdEndDebugUtilsLabelEXT");
	CmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(dev->instance, "vkCmdInsertDebugUtilsLabelEXT");
}
VkSampler vkh_device_create_sampler(VkhDevice dev, VkFilter magFilter, VkFilter minFilter,
	VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode) {
	VkSampler sampler = VK_NULL_HANDLE;
	VkSamplerCreateInfo samplerCreateInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
											  .magFilter = magFilter,
											  .minFilter = minFilter,
											  .mipmapMode = mipmapMode ,
											  .addressModeU = addressMode,
											  .addressModeV = addressMode,
											  .addressModeW = addressMode,
											  .maxAnisotropy = 1.0 };
	VK_CHECK_RESULT(vkCreateSampler(dev->dev, &samplerCreateInfo, NULL, &sampler));
	return sampler;
}
void vkh_device_destroy_sampler(VkhDevice dev, VkSampler sampler) {
	vkDestroySampler(dev->dev, sampler, NULL);
}
void vkh_device_destroy(VkhDevice dev) {
#ifdef VKH_USE_VMA
	vmaDestroyAllocator(dev->allocator);
#else
#endif
	vkDestroyDevice(dev->dev, NULL);
	free(dev);
}

void vkh_device_set_object_name(VkhDevice dev, VkObjectType objectType, uint64_t handle, const char* name) {
	const VkDebugUtilsObjectNameInfoEXT info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.pNext = 0,
		.objectType = objectType,
		.objectHandle = handle,
		.pObjectName = name
	};
	SetDebugUtilsObjectNameEXT(dev->dev, &info);
}
void vkh_cmd_label_start(VkCommandBuffer cmd, const char* name, const float color[4]) {
	const VkDebugUtilsLabelEXT info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pNext = 0,
		.pLabelName = name
	};
	memcpy((void*)info.color, (void*)color, 4 * sizeof(float));
	CmdBeginDebugUtilsLabelEXT(cmd, &info);
}
void vkh_cmd_label_insert(VkCommandBuffer cmd, const char* name, const float color[4]) {
	const VkDebugUtilsLabelEXT info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pNext = 0,
		.pLabelName = name
	};
	memcpy((void*)info.color, (void*)color, 4 * sizeof(float));
	CmdInsertDebugUtilsLabelEXT(cmd, &info);
}
void vkh_cmd_label_end(VkCommandBuffer cmd) {
	CmdEndDebugUtilsLabelEXT(cmd);
}

#define CHECK_BIT(var,pos) (((var)>>(pos)) & 1)

VkFence vkh_fence_create(VkhDevice dev) {
	VkFence fence;
	VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
									.pNext = NULL,
									.flags = 0 };
	VK_CHECK_RESULT(vkCreateFence(dev->dev, &fenceInfo, NULL, &fence));
	return fence;
}
VkFence vkh_fence_create_signaled(VkhDevice dev) {
	VkFence fence;
	VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
									.pNext = NULL,
									.flags = VK_FENCE_CREATE_SIGNALED_BIT };
	VK_CHECK_RESULT(vkCreateFence(dev->dev, &fenceInfo, NULL, &fence));
	return fence;
}
VkSemaphore vkh_semaphore_create(VkhDevice dev) {
	VkSemaphore semaphore;
	VkSemaphoreCreateInfo info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
								   .pNext = NULL,
								   .flags = 0 };
	VK_CHECK_RESULT(vkCreateSemaphore(dev->dev, &info, NULL, &semaphore));
	return semaphore;
}
VkSemaphore vkh_timeline_create(VkhDevice dev, uint64_t initialValue) {
	VkSemaphore semaphore;
	VkSemaphoreTypeCreateInfo timelineInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, .pNext = NULL,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue = initialValue };
	VkSemaphoreCreateInfo info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
								   .pNext = &timelineInfo,
								   .flags = 0 };
	VK_CHECK_RESULT(vkCreateSemaphore(dev->dev, &info, NULL, &semaphore));
	return semaphore;
}

VkResult vkh_timeline_wait(VkhDevice dev, VkSemaphore timeline, const uint64_t wait) {
	VkSemaphoreWaitInfo waitInfo;
	waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	waitInfo.pNext = NULL;
	waitInfo.flags = 0;
	waitInfo.semaphoreCount = 1;
	waitInfo.pSemaphores = &timeline;
	waitInfo.pValues = &wait;

	return vkWaitSemaphores(dev->dev, &waitInfo, UINT64_MAX);
}
void vkh_cmd_submit_timelined(VkhQueue queue, VkCommandBuffer* pCmdBuff, VkSemaphore timeline, const uint64_t wait, const uint64_t signal) {
	static VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkTimelineSemaphoreSubmitInfo timelineInfo;
	timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	timelineInfo.pNext = NULL;
	timelineInfo.waitSemaphoreValueCount = 1;
	timelineInfo.pWaitSemaphoreValues = &wait;
	timelineInfo.signalSemaphoreValueCount = 1;
	timelineInfo.pSignalSemaphoreValues = &signal;

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = &timelineInfo;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &timeline;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &timeline;
	submitInfo.pWaitDstStageMask = &stageFlags,
		submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = pCmdBuff;

	VK_CHECK_RESULT(vkQueueSubmit(queue->queue, 1, &submitInfo, VK_NULL_HANDLE));
}
void vkh_cmd_submit_timelined2(VkhQueue queue, VkCommandBuffer* pCmdBuff, VkSemaphore timelines[2], const uint64_t waits[2], const uint64_t signals[2]) {
	static VkPipelineStageFlags stageFlags[2] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	VkTimelineSemaphoreSubmitInfo timelineInfo;
	timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	timelineInfo.pNext = NULL;
	timelineInfo.waitSemaphoreValueCount = 2;
	timelineInfo.pWaitSemaphoreValues = waits;
	timelineInfo.signalSemaphoreValueCount = 2;
	timelineInfo.pSignalSemaphoreValues = signals;

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = &timelineInfo;
	submitInfo.waitSemaphoreCount = 2;
	submitInfo.pWaitSemaphores = timelines;
	submitInfo.signalSemaphoreCount = 2;
	submitInfo.pSignalSemaphores = timelines;
	submitInfo.pWaitDstStageMask = stageFlags,
		submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = pCmdBuff;

	VK_CHECK_RESULT(vkQueueSubmit(queue->queue, 1, &submitInfo, VK_NULL_HANDLE));
}
VkEvent vkh_event_create(VkhDevice dev) {
	VkEvent evt;
	VkEventCreateInfo evtInfo = { .sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO };
	VK_CHECK_RESULT(vkCreateEvent(dev->dev, &evtInfo, NULL, &evt));
	return evt;
}
VkCommandPool vkh_cmd_pool_create(VkhDevice dev, uint32_t qFamIndex, VkCommandPoolCreateFlags flags) {
	VkCommandPool cmdPool;
	VkCommandPoolCreateInfo cmd_pool_info = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
											  .pNext = NULL,
											  .flags = flags,
											  .queueFamilyIndex = qFamIndex };
	VK_CHECK_RESULT(vkCreateCommandPool(dev->dev, &cmd_pool_info, NULL, &cmdPool));
	return cmdPool;
}
VkCommandBuffer vkh_cmd_buff_create(VkhDevice dev, VkCommandPool cmdPool, VkCommandBufferLevel level) {
	VkCommandBuffer cmdBuff;
	VkCommandBufferAllocateInfo cmd = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
										.pNext = NULL,
										.commandPool = cmdPool,
										.level = level,
										.commandBufferCount = 1 };
	VK_CHECK_RESULT(vkAllocateCommandBuffers(dev->dev, &cmd, &cmdBuff));
	return cmdBuff;
}
void vkh_cmd_buffs_create(VkhDevice dev, VkCommandPool cmdPool, VkCommandBufferLevel level, uint32_t count, VkCommandBuffer* cmdBuffs) {
	VkCommandBufferAllocateInfo cmd = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
										.pNext = NULL,
										.commandPool = cmdPool,
										.level = level,
										.commandBufferCount = count };
	VK_CHECK_RESULT(vkAllocateCommandBuffers(dev->dev, &cmd, cmdBuffs));
}
void vkh_cmd_begin(VkCommandBuffer cmdBuff, VkCommandBufferUsageFlags flags) {
	VkCommandBufferBeginInfo cmd_buf_info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
											  .pNext = NULL,
											  .flags = flags,
											  .pInheritanceInfo = NULL };

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuff, &cmd_buf_info));
}
void vkh_cmd_end(VkCommandBuffer cmdBuff) {
	VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuff));
}
void vkh_cmd_submit(VkhQueue queue, VkCommandBuffer* pCmdBuff, VkFence fence) {
	VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkSubmitInfo submit_info = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
								 .pWaitDstStageMask = &stageFlags,
								 .commandBufferCount = 1,
								 .pCommandBuffers = pCmdBuff };
	VK_CHECK_RESULT(vkQueueSubmit(queue->queue, 1, &submit_info, fence));
}
void vkh_cmd_submit_with_semaphores(VkhQueue queue, VkCommandBuffer* pCmdBuff, VkSemaphore waitSemaphore,
	VkSemaphore signalSemaphore, VkFence fence) {

	VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkSubmitInfo submit_info = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
								 .pWaitDstStageMask = &stageFlags,
								 .commandBufferCount = 1,
								 .pCommandBuffers = pCmdBuff };

	if (waitSemaphore != VK_NULL_HANDLE) {
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &waitSemaphore;
	}
	if (signalSemaphore != VK_NULL_HANDLE) {
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &signalSemaphore;
	}

	VK_CHECK_RESULT(vkQueueSubmit(queue->queue, 1, &submit_info, fence));
}


void set_image_layout(VkCommandBuffer cmdBuff, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout,
	VkImageLayout new_image_layout, VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages) {
	VkImageSubresourceRange subres = { aspectMask,0,1,0,1 };
	set_image_layout_subres(cmdBuff, image, subres, old_image_layout, new_image_layout, src_stages, dest_stages);
}

void set_image_layout_subres(VkCommandBuffer cmdBuff, VkImage image, VkImageSubresourceRange subresourceRange,
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
}

bool vkh_memory_type_from_properties(VkPhysicalDeviceMemoryProperties* memory_properties, uint32_t typeBits, VkhMemoryUsage memUsage, uint32_t* typeIndex) {
	VkMemoryPropertyFlags memFlags = {};
	switch (memUsage) {
	case VKH_MEMORY_USAGE_GPU_ONLY:
		memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		break;
	case VKH_MEMORY_USAGE_CPU_ONLY:
		memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		break;
	case VKH_MEMORY_USAGE_CPU_TO_GPU:
		memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		break;
	case VKH_MEMORY_USAGE_GPU_TO_CPU:
		memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		break;
	case VKH_MEMORY_USAGE_CPU_COPY:
		memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		break;
	case VKH_MEMORY_USAGE_GPU_LAZILY_ALLOCATED:
		memFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
		break;
	}

	// Search memtypes to find first index with those properties
	for (uint32_t i = 0; i < memory_properties->memoryTypeCount; i++) {
		if (CHECK_BIT(typeBits, i)) {
			// Type is available, does it match user properties?
			if ((memory_properties->memoryTypes[i].propertyFlags & memFlags) == memFlags) {
				*typeIndex = i;
				return true;
			}
		}
	}
	// No memory types matched, return failure
	return false;
}

VkShaderModule vkh_load_module(VkDevice dev, const char* path) {
	VkShaderModule module;
	size_t filelength;
	uint32_t* pCode = (uint32_t*)read_spv(path, &filelength);
	VkShaderModuleCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
											.codeSize = filelength,
											.pCode = pCode };
	VK_CHECK_RESULT(vkCreateShaderModule(dev, &createInfo, NULL, &module));
	free(pCode);
	//assert(module != VK_NULL_HANDLE);
	return module;
}

char* read_spv(const char* filename, size_t* psize) {
	size_t size;
	size_t retval;
	void* shader_code;

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
	filename = [[[NSBundle mainBundle]resourcePath] stringByAppendingPathComponent:@(filename)] .UTF8String;
#endif

	FILE* fp = fopen(filename, "rb");
	if (!fp)
		return NULL;

	fseek(fp, 0L, SEEK_END);
	size = (size_t)ftell(fp);

	fseek(fp, 0L, SEEK_SET);

	shader_code = malloc(size);
	retval = fread(shader_code, size, 1, fp);
	assert(retval == 1);

	*psize = size;

	fclose(fp);
	return (char*)shader_code;
}

// Read file into array of bytes, and cast to uint32_t*, then return.
// The data has been padded, so that it fits into an array uint32_t.
uint32_t* readFile(uint32_t* length, const char* filename) {

	FILE* fp = fopen(filename, "rb");
	if (fp == 0) {
		printf("Could not find or open file: %s\n", filename);
	}

	// get file size.
	fseek(fp, 0, SEEK_END);
	long filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	long filesizepadded = (long)(ceil(filesize / 4.0)) * 4;

	// read file contents.
	char* str = (char*)malloc(filesizepadded * sizeof(char));
	fread(str, filesize, sizeof(char), fp);
	fclose(fp);

	// data padding.
	for (int i = filesize; i < filesizepadded; i++)
		str[i] = 0;

	*length = filesizepadded;
	return (uint32_t*)str;
}

void dumpLayerExts() {
	printf("Layers:\n");
	uint32_t instance_layer_count;
	VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL));
	if (instance_layer_count == 0)
		return;
	VkLayerProperties* vk_props = (VkLayerProperties*)malloc(instance_layer_count * sizeof(VkLayerProperties));
	VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_props));

	for (uint32_t i = 0; i < instance_layer_count; i++) {
		printf("\t%s, %s\n", vk_props[i].layerName, vk_props[i].description);
		/*		  res = init_global_extension_properties(layer_props);
		if (res) return res;
		info.instance_layer_properties.push_back(layer_props);*/
	}
	free(vk_props);
}

static VkExtensionProperties* instExtProps;
static uint32_t instExtCount;
bool vkh_instance_extension_supported(const char* instanceName) {
	for (uint32_t i = 0; i < instExtCount; i++) {
		if (!strcmp(instExtProps[i].extensionName, instanceName))
			return true;
	}
	return false;
}
void vkh_instance_extensions_check_init() {
	VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(NULL, &instExtCount, NULL));
	instExtProps = (VkExtensionProperties*)malloc(instExtCount * sizeof(VkExtensionProperties));
	VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(NULL, &instExtCount, instExtProps));
}
void vkh_instance_extensions_check_release() {
	free(instExtProps);
	instExtCount = 0;
}

static VkLayerProperties* instLayerProps;
static uint32_t instance_layer_count;
bool vkh_layer_is_present(const char* layerName) {
	for (uint32_t i = 0; i < instance_layer_count; i++) {
		if (!strcmp(instLayerProps[i].layerName, layerName))
			return true;
	}
	return false;
}
void vkh_layers_check_init() {
	VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL));
	instLayerProps = (VkLayerProperties*)malloc(instance_layer_count * sizeof(VkLayerProperties));
	VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instance_layer_count, instLayerProps));
}
void vkh_layers_check_release() {
	free(instLayerProps);
	instance_layer_count = 0;
}

#define ENGINE_NAME		"vkhelpers"
#define ENGINE_VERSION	1

VkBool32 debugUtilsMessengerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT			 messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT					 messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	switch (messageSeverity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		printf(KYEL);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		printf(KRED);
		break;
	default:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		printf(KGRN);
		break;
	}
	switch (messageTypes) {
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
		printf("GEN: ");
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
		printf("VAL: ");
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
		printf("PRF: ");
		break;
	}

	printf(KNRM);
	printf("%s\n", pCallbackData->pMessage);


	fflush(stdout);
	return VK_FALSE;
}

VkhApp vkh_app_create(uint32_t version_major, uint32_t version_minor, const char* app_name, uint32_t enabledLayersCount, const char** enabledLayers, uint32_t ext_count, const char* extentions[]) {
	VkhApp app = (VkhApp)malloc(sizeof(vkh_app_t));

	VkApplicationInfo infos = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
								.pApplicationName = app_name,
								.applicationVersion = 1,
								.pEngineName = ENGINE_NAME,
								.engineVersion = ENGINE_VERSION,
#ifdef VK_MAKE_API_VERSION
								.apiVersion = VK_MAKE_API_VERSION(0, version_major, version_minor, 0) };
#else
		.apiVersion = VK_MAKE_VERSION(version_major, version_minor, 0)
};
#endif

	VkInstanceCreateInfo inst_info = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
									   .pApplicationInfo = &infos,
									   .enabledLayerCount = enabledLayersCount,
									   .ppEnabledLayerNames = enabledLayers,
									   .enabledExtensionCount = ext_count,
									   .ppEnabledExtensionNames = extentions };

	VK_CHECK_RESULT(vkCreateInstance(&inst_info, NULL, &app->inst));
	app->infos = infos;
	app->debugMessenger = VK_NULL_HANDLE;
	return app;
}

void vkh_app_destroy(VkhApp app) {
	if (app->debugMessenger != VK_NULL_HANDLE) {
		PFN_vkDestroyDebugUtilsMessengerEXT	 DestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(app->inst, "vkDestroyDebugUtilsMessengerEXT");

		DestroyDebugUtilsMessenger(app->inst, app->debugMessenger, VK_NULL_HANDLE);
	}
	vkDestroyInstance(app->inst, NULL);
	free(app);
}

VkInstance vkh_app_get_inst(VkhApp app) {
	return app->inst;
}

VkhPhyInfo* vkh_app_get_phyinfos(VkhApp app, uint32_t* count, VkSurfaceKHR surface) {
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(app->inst, count, NULL));
	VkPhysicalDevice* phyDevices = (VkPhysicalDevice*)malloc((*count) * sizeof(VkPhysicalDevice));
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(app->inst, count, phyDevices));
	VkhPhyInfo* infos = (VkhPhyInfo*)malloc((*count) * sizeof(VkhPhyInfo));

	for (uint32_t i = 0; i < (*count); i++)
		infos[i] = vkh_phyinfo_create(phyDevices[i], surface);

	free(phyDevices);
	return infos;
}

void vkh_app_free_phyinfos(uint32_t count, VkhPhyInfo* infos) {
	for (uint32_t i = 0; i < count; i++)
		vkh_phyinfo_destroy(infos[i]);
	free(infos);
}
/**
 * @brief Add a Debug utils messenger to this  VkhApp. It will be destroyed on VkhApp end.
 * @param VKH application pointer containing vkInstance.
 * @param Message type flags
 * @param Message severity flags.
 * @param optional message callback, if null a default one which print to stdout is configured.
 */
void vkh_app_enable_debug_messenger(VkhApp app,
	VkDebugUtilsMessageTypeFlagsEXT typeFlags,
	VkDebugUtilsMessageSeverityFlagsEXT severityFlags,
	PFN_vkDebugUtilsMessengerCallbackEXT callback) {

	VkDebugUtilsMessengerCreateInfoEXT info = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
												.pNext = VK_NULL_HANDLE,
												.flags = 0,
												.messageSeverity = severityFlags,
												.messageType = typeFlags,
												.pUserData = NULL };
	if (callback == NULL)
		info.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)debugUtilsMessengerCallback;
	else
		info.pfnUserCallback = callback;

	PFN_vkCreateDebugUtilsMessengerEXT	CreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(app->inst, "vkCreateDebugUtilsMessengerEXT");

	CreateDebugUtilsMessenger(app->inst, &info, VK_NULL_HANDLE, &app->debugMessenger);
}
vg_fbo_t new_vgfbo(ovg_ctx_t* p, int width, int height) {
	vg_fbo_t fbo = {};
	fbo.width = width; fbo.height = height;
	if (p) {
		ovg_new_fbo(p, (vg_fbo_t0*)&fbo);
	}
	return fbo;
}
void free_vgfbo(vg_fbo_t* p) {
	if (p) {
		ovg_free_fbo((vg_fbo_t0*)p);
	}
}
#endif // 
