/*
 * vg_renderer.cpp
 *
 * SDL3 GPU vector-graphics renderer built around vg.slang.h 
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
#include <vulkan/vulkan.h>
#include "ovg.h"
#include "vg_renderer.h" 

//#include "Private/a_vg.vert.h"
//#include "Private/a_vg.frag.h"

#include "shaders/spv_c/a_vg.vert.h"
#include "shaders/spv_c/a_vg.frag.h"

#define FULLSCREEN_BIT         0x10000000
#define SRCTYPE_MASK           0x000000FF

 /* ── 4.  Load a pre-compiled SPIR-V file ─────────────────────── */
void* LoadSPIRV(const char* path, size_t* outSize) {
	SDL_IOStream* f = SDL_IOFromFile(path, "rb");
	if (!f) { SDL_Log("Cannot open %s: %s", path, SDL_GetError()); return NULL; }
	Sint64 sz = SDL_GetIOSize(f);
	if (sz <= 0) { SDL_CloseIO(f); return NULL; }
	void* buf = SDL_malloc((size_t)sz);
	if (SDL_ReadIO(f, buf, (size_t)sz) != (size_t)sz) {
		SDL_free(buf); SDL_CloseIO(f); return NULL;
	}
	SDL_CloseIO(f);
	*outSize = (size_t)sz;
	return buf;
}

/* ── 5.  Create an SDL_GPUShader from a .spv file ────────────── */
SDL_GPUShader* CreateShader(VGState* g, const char* spvPath,
	SDL_GPUShaderStage stage,
	Uint32 numSamplers,
	Uint32 numStorageTex,
	Uint32 numStorageBuf,
	Uint32 numUniformBuf) {
	size_t codeSize = 0;
	//void* code = LoadSPIRV(spvPath, &codeSize);
	//if (!code) return NULL;
	const void* code = spvPath[3] == 'v' ? vg_vert : vg_frag;
	codeSize = spvPath[3] == 'v' ? sizeof(vg_vert) : sizeof(vg_frag);
	SDL_GPUShaderCreateInfo ci = {
		.code_size = codeSize,
		.code = (const Uint8*)code,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = stage,
		.num_samplers = numSamplers,
		.num_storage_textures = numStorageTex,
		.num_storage_buffers = numStorageBuf,
		.num_uniform_buffers = numUniformBuf,
		.props = 0,
	};
	SDL_GPUShader* sh = SDL_CreateGPUShader(g->device, &ci);
	//SDL_free(code);
	if (!sh) SDL_Log("Shader create failed (%s): %s", spvPath, SDL_GetError());
	return sh;
}

/* ── 6.  Init ─────────────────────────────────────────────────── */
bool VG_Init(VGState* g, int width, int height) {
	SDL_Init(SDL_INIT_VIDEO);

	g->window = SDL_CreateWindow("SDL3 GPU Vector Graphics",
		width, height,
		SDL_WINDOW_RESIZABLE |
		SDL_WINDOW_HIGH_PIXEL_DENSITY);
	if (!g->window) return false;
	SDL_GPUVulkanOptions vo = {};
	VkPhysicalDeviceScalarBlockLayoutFeatures scalarFeatures = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES,
		.pNext = NULL,
		.scalarBlockLayout = VK_TRUE,
	};
	VkPhysicalDeviceVulkan12Features enabledFeatures12 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };

	enabledFeatures12.scalarBlockLayout = VK_TRUE;
	// 1b. Vulkan 1.1 复合特性（包含 shaderDrawParameters） 
	VkPhysicalDeviceVulkan11Features vk11Features = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,	   .pNext = &enabledFeatures12, };
	// 其他 1.1 特性默认由驱动填充，我们只关心 shaderDrawParameters
	vk11Features.shaderDrawParameters = VK_TRUE;
	// 以下字段留 0，让 SDL/Vulkan 使用默认值
	vk11Features.storageBuffer16BitAccess = VK_FALSE;
	vk11Features.uniformAndStorageBuffer16BitAccess = VK_FALSE;
	vk11Features.storagePushConstant16 = VK_FALSE;
	vk11Features.storageInputOutput16 = VK_FALSE;
	vk11Features.multiview = VK_FALSE;
	vk11Features.multiviewGeometryShader = VK_FALSE;
	vk11Features.multiviewTessellationShader = VK_FALSE;
	vk11Features.variablePointersStorageBuffer = VK_FALSE;
	vk11Features.variablePointers = VK_FALSE;
	vk11Features.protectedMemory = VK_FALSE;
	vk11Features.samplerYcbcrConversion = VK_TRUE;
	const char* devext[] = { VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
			VK_KHR_MAINTENANCE_5_EXTENSION_NAME,VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME };
	const char* insext[] = { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };
	// 2. 填充 SDL_GPUVulkanOptions
	SDL_GPUVulkanOptions vkOpts = {
		.vulkan_api_version = VK_API_VERSION_1_2,  // 必须 >= 1.2 才能启用 scalarBlockLayout
		.feature_list = &vk11Features,             // pNext 链头
		.vulkan_10_physical_device_features = NULL, // 不需要额外 1.0 特性
		.device_extension_count = 3,
		.device_extension_names = devext,
		.instance_extension_count = 1,
		.instance_extension_names = insext,
	};

	// 3. 通过属性创建 GPU 设备
	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetPointerProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_OPTIONS_POINTER, &vkOpts);
	SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "vulkan");
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);

	SDL_GPUDevice* device = g->device = SDL_CreateGPUDeviceWithProperties(props);
	SDL_DestroyProperties(props);
	if (!g->device) {
		SDL_Log("GPU device create failed: %s", SDL_GetError());
		return false;
	}
	SDL_ClaimWindowForGPUDevice(g->device, g->window);
	 
	SDL_GPUShader* vs = CreateShader(g, "vg.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, 0, 1); 
	SDL_GPUShader* fs = CreateShader(g, "vg.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 1);
	if (!vs || !fs) return false;

	/* ── 6b. Vertex input layout ─────────────────────────── */
	SDL_GPUVertexBufferDescription vbDesc = {
		.slot = 0,
		.pitch = sizeof(ovgVertex),
		.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
		.instance_step_rate = 0,
	};
	SDL_GPUVertexAttribute attrs[3] = {
		{.location = 0,
		  .buffer_slot = 0,
		  .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
		  .offset = offsetof(ovgVertex, pos) },
		{.location = 1,
		  .buffer_slot = 0,
		  .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
		  .offset = offsetof(ovgVertex, uv) },
		{.location = 2,
		  .buffer_slot = 0,
		  .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
		  .offset = offsetof(ovgVertex, color) },
	};

	/* ── 6c. Color target with alpha blending ────────────── */
	SDL_GPUColorTargetDescription colorTarget = {
		.format = SDL_GetGPUSwapchainTextureFormat(g->device, g->window),
		.blend_state = {
			.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.color_blend_op = SDL_GPU_BLENDOP_ADD,
			.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			.color_write_mask = SDL_GPU_COLORCOMPONENT_R |
								   SDL_GPU_COLORCOMPONENT_G |
								   SDL_GPU_COLORCOMPONENT_B |
								   SDL_GPU_COLORCOMPONENT_A,
			.enable_blend = true,
		},
	};

	/* ── 6d. Create the graphics pipeline ────────────────── */
	SDL_GPUDepthStencilState dss = {};
	SDL_GPUStencilOpState polyFillOpState = {
	.fail_op = SDL_GPU_STENCILOP_KEEP,
	.pass_op = SDL_GPU_STENCILOP_INVERT,
	.depth_fail_op = SDL_GPU_STENCILOP_KEEP,
	.compare_op = SDL_GPU_COMPAREOP_EQUAL,
	};
	SDL_GPUStencilOpState clipingOpState = {
	.fail_op = SDL_GPU_STENCILOP_ZERO,
	.pass_op = SDL_GPU_STENCILOP_REPLACE,
	.depth_fail_op = SDL_GPU_STENCILOP_KEEP,
	.compare_op = SDL_GPU_COMPAREOP_EQUAL,
	};
	SDL_GPUStencilOpState stencilOpState = {
	.fail_op = SDL_GPU_STENCILOP_KEEP,
	.pass_op = SDL_GPU_STENCILOP_ZERO,
	.depth_fail_op = SDL_GPU_STENCILOP_KEEP,
	.compare_op = SDL_GPU_COMPAREOP_EQUAL,
	};
	dss.compare_op = SDL_GPUCompareOp::SDL_GPU_COMPAREOP_ALWAYS;
	dss.back_stencil_state = polyFillOpState;
	dss.front_stencil_state = polyFillOpState;
	dss.enable_depth_test = false;
	dss.enable_depth_write = false;
	dss.enable_stencil_test = true;
	auto poly_dss = dss;
	poly_dss.compare_mask = STENCIL_CLIP_BIT;
	poly_dss.write_mask = STENCIL_FILL_BIT;
	auto cliping_dss = dss;
	cliping_dss.compare_mask = STENCIL_FILL_BIT;
	cliping_dss.write_mask = STENCIL_ALL_BIT;
	auto stencil_dss = dss;
	stencil_dss.back_stencil_state = stencil_dss.front_stencil_state = stencilOpState;
	stencil_dss.compare_mask = STENCIL_FILL_BIT;
	stencil_dss.write_mask = STENCIL_FILL_BIT;
	SDL_GPUDepthStencilState dsstate[3] = { poly_dss ,cliping_dss,stencil_dss };
	SDL_GPUGraphicsPipelineCreateInfo pipeCI = {
		.vertex_shader = vs,
		.fragment_shader = fs,
		.vertex_input_state = {
			.vertex_buffer_descriptions = &vbDesc,
			.num_vertex_buffers = 1,
			.vertex_attributes = attrs,
			.num_vertex_attributes = 3,
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.rasterizer_state = {
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_NONE,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
		},
		.depth_stencil_state = stencil_dss,
		.target_info = {
			.color_target_descriptions = &colorTarget,
			.num_color_targets = 1,
			.has_depth_stencil_target = false,
		},
	};
	g->pipeline = SDL_CreateGPUGraphicsPipeline(g->device, &pipeCI);
	SDL_ReleaseGPUShader(g->device, vs);
	SDL_ReleaseGPUShader(g->device, fs);
	if (!g->pipeline) {
		SDL_Log("Pipeline create failed: %s", SDL_GetError());
		return false;
	}

	g->cap_v = 1024;
	SDL_GPUBufferCreateInfo bc = { .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = sizeof(ovgVertex) * g->cap_v , };
	g->vertexBuffer = SDL_CreateGPUBuffer(g->device, &bc);
	if (!g->vertexBuffer) return false;

	/* ── 6f. Linear sampler for SURFACE mode ─────────────── */
	SDL_GPUSamplerCreateInfo sc = {
		.min_filter = SDL_GPU_FILTER_LINEAR,
			.mag_filter = SDL_GPU_FILTER_LINEAR,
			.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	};
	g->linearSampler = SDL_CreateGPUSampler(g->device, &sc);
	if (!g->linearSampler) return false;
	return true;
}

bool resize_res(VGState* g, size_t vcount) {
	if (!g)return false;
	if (vcount > g->cap_v)
	{
		g->cap_v += vcount;
		SDL_GPUBufferCreateInfo bc = { .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = sizeof(ovgVertex) * g->cap_v, };
		if (g->vertexBuffer)
			SDL_ReleaseGPUBuffer(g->device, g->vertexBuffer);
		g->vertexBuffer = SDL_CreateGPUBuffer(g->device, &bc);
		if (!g->vertexBuffer) return false;
	}
	return true;
}
SDL_GPUTexture* CreateWhiteTexture16x16(SDL_GPUDevice* device) {
	const int W = 16, H = 16;
	const Uint32 pixelCount = W * H;
	Uint32* pixels = (Uint32*)SDL_malloc(pixelCount * sizeof(Uint32));
	if (!pixels) {
		SDL_Log("Failed to alloc pixels: %s", SDL_GetError());
		return NULL;
	}

	// 填充纯白 RGBA (0xFFFFFFFF)
	for (Uint32 i = 0; i < pixelCount; i++) {
		pixels[i] = 0xFFFFFFFF;
	}

	// 1. 创建 GPU 纹理
	SDL_GPUTextureCreateInfo tci;
	SDL_zero(tci);
	tci.type = SDL_GPU_TEXTURETYPE_2D;
	tci.width = W;
	tci.height = H;
	tci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	tci.layer_count_or_depth = 1;
	tci.num_levels = 1;
	tci.sample_count = SDL_GPU_SAMPLECOUNT_1;
	tci.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
	SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &tci);
	if (!texture) {
		SDL_Log("Failed to create texture: %s", SDL_GetError());
		SDL_free(pixels);
		return NULL;
	}

	// 2. 获取命令缓冲
	SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
	if (!cmd) {
		SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
		SDL_ReleaseGPUTexture(device, texture);
		SDL_free(pixels);
		return NULL;
	}

	// 3. ★★★ 必须创建复制通道（Copy Pass）才能上传纹理 ★★★
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
	if (!copyPass) {
		SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
		SDL_SubmitGPUCommandBuffer(cmd);  // 释放命令缓冲
		SDL_ReleaseGPUTexture(device, texture);
		SDL_free(pixels);
		return NULL;
	}
	auto data_size = pixelCount * sizeof(uint32_t);
	auto row_size = W * sizeof(uint32_t);
	auto pitch = W * sizeof(uint32_t);
	// 4. 定义传输信息（具名变量，避免临时量取地址）
	SDL_GPUTransferBufferCreateInfo tbci;
	SDL_zero(tbci);
	tbci.size = (Uint32)data_size;
	tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;

	SDL_GPUTransferBuffer* tbuf = SDL_CreateGPUTransferBuffer(device, &tbci);
	if (tbuf == NULL) {
		return 0;
	}

	Uint8* output = (Uint8*)SDL_MapGPUTransferBuffer(device, tbuf, false);
	if (!output) {
		return 0;
	}
	if ((size_t)pitch == row_size) {
		SDL_memcpy(output, pixels, data_size);
	}
	else {
		const Uint8* input = (Uint8*)pixels;
		for (int i = 0; i < H; ++i) {
			SDL_memcpy(output, input, row_size);
			output += row_size;
			input += pitch;
		}
	}
	SDL_UnmapGPUTransferBuffer(device, tbuf);
	SDL_GPUTextureTransferInfo tex_src = {};
	tex_src.transfer_buffer = tbuf;
	tex_src.rows_per_layer = H;
	tex_src.pixels_per_row = W;
	// 5. 定义目标纹理区域

	SDL_GPUTextureRegion tex_dst;
	SDL_zero(tex_dst);
	tex_dst.texture = texture;
	tex_dst.x = 0;
	tex_dst.y = 0;
	tex_dst.w = W;
	tex_dst.h = H;
	tex_dst.d = 1;

	// 6. 执行上传（现在传的是变量的地址，是左值）
	SDL_UploadToGPUTexture(
		copyPass,          // 第一个参数是 Copy Pass，不是 command buffer！
		&tex_src,     // 左值地址，MSVC 不再报错
		&tex_dst,
		false              // 不生成 mipmap
	);

	// 7. 结束复制通道
	SDL_EndGPUCopyPass(copyPass);

	// 8. 提交并等待上传完成（纹理创建必须同步，否则像素内存会被提前释放）
	//SDL_SubmitGPUCommandBufferAndWait(cmd);
	SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
	SDL_WaitForGPUFences(device, true, &fence, 1);
	SDL_ReleaseGPUFence(device, fence);
	// 9. 清理 CPU 侧像素内存
	SDL_free(pixels);
	SDL_ReleaseGPUTransferBuffer(device, tbuf);
	return texture;
}
SDL_GPUSampler* CreateLinearSampler(SDL_GPUDevice* device) {
	SDL_GPUSamplerCreateInfo s = {};
	s.min_filter = SDL_GPU_FILTER_LINEAR;
	s.mag_filter = SDL_GPU_FILTER_LINEAR;
	s.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	s.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	s.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	s.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	s.compare_op = SDL_GPU_COMPAREOP_ALWAYS;

	return SDL_CreateGPUSampler(device, &s);
}
/* ── 8.  Draw a filled rectangle (2 triangles, 6 vertices) ──────── */
#define PAT_SOLID  0
#define PAT_SURFACE 1
#define PAT_LINEAR 2
#define PAT_RADIAL 3
#define PAT_MESH   4
#define PAT_RASTER 5
#define PAT_SWEEP  6	

void mul_pat(vg_gradient_t* grad, int type, const glm::mat3& mat, const glm::mat3& pat_mat);

void begin_frame(VGState* g) {}
int submit_draw(VGState* g) {

	uint32_t sw = 0, sh = 0;
	SDL_GPUTexture* swapchain = NULL;
	uint32_t vcount = 0;
	if (g->data)vcount += g->data->v_count;
	resize_res(g, vcount);
	SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(g->device);
	if (!cmd) return 0;
	if (!SDL_AcquireGPUSwapchainTexture(cmd, g->window, &swapchain, &sw, &sh)) {
		SDL_CancelGPUCommandBuffer(cmd);
		return 0;
	}
	g->width = sw; g->height = sh;
	if (swapchain == NULL) {
		/* Window minimized or not ready — cancel and skip this frame */
		SDL_CancelGPUCommandBuffer(cmd);
		return 0;
	}
	/* ── Begin render pass ─────────────────────────────────── */
	SDL_GPUColorTargetInfo cti = {
		.texture = swapchain,
		.clear_color = { 0.0f },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
	};
	g->cmd = cmd;
	auto copyPass = SDL_BeginGPUCopyPass(cmd);
	/* ── Upload vertices via a transfer buffer ────────────── */
	SDL_GPUTransferBufferCreateInfo tbc = {};
	tbc.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbc.size = 0;
	if (g->data)
		tbc.size += g->data->v_count * sizeof(ovgVertex);
	SDL_GPUTransferBuffer* staging = SDL_CreateGPUTransferBuffer(g->device, &tbc);
	g->rq.push(staging);
	void* map = SDL_MapGPUTransferBuffer(g->device, staging, false);

	if (g->data)
		memcpy(map, g->data->vg_vertex, tbc.size);
	SDL_UnmapGPUTransferBuffer(g->device, staging);

	/* Use a copy pass for the upload, then a render pass for drawing */
	SDL_GPUTransferBufferLocation tbl = { .transfer_buffer = staging, .offset = 0 };
	SDL_GPUBufferRegion br = { .buffer = g->vertexBuffer, .offset = 0, .size = tbc.size };
	SDL_UploadToGPUBuffer(copyPass, &tbl, &br, true   /* cycle */);
	SDL_EndGPUCopyPass(copyPass);
	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &cti, 1, NULL);
	g->pass = pass;
	SDL_BindGPUGraphicsPipeline(g->pass, g->pipeline);

	SDL_GPUBufferBinding vbBind = { .buffer = g->vertexBuffer, .offset = 0 };
	SDL_BindGPUVertexBuffers(g->pass, 0, &vbBind, 1);
	SDL_GPUTextureSamplerBinding binding = { .texture = g->emptyImg,	.sampler = g->sampler, };
	SDL_BindGPUFragmentSamplers(g->pass, 0, &binding, 1);
	SDL_SetGPUStencilReference(g->pass, 0x1);
	SDL_GPUViewport view = { 0,0,0,0,0,1 };
	int smax = std::max(g->width, g->height);
	view.x = 0;
	view.y = g->height;
	view.w = g->width;
	view.h = -g->height;
	SDL_Rect clip = { 0,0,g->width,g->height };
	SDL_SetGPUViewport(g->pass, &view);
	SDL_SetGPUScissor(g->pass, &clip);

	for (size_t i = 0; g->data && i < g->data->count; i++)
	{
		auto& it = g->data->d[i];
		auto pc = it.vg.state->pushConsts;
		pc.size = { g->width,g->height };
		switch (it.g.stype) {
		case 0:
		{
			push_constants_t pc0 = {};
			if (it.vg.state->pattern && it.vg.state->pattern->type != PAT_SOLID) {
				pc.source = { smax,smax,0,0 };
			}
			memcpy(&pc0.source, &pc.source, sizeof(float) * 4);
			pc0.size[0] = pc.size.x;
			pc0.size[1] = pc.size.y;
			if (it.vg.state->pattern)
				pc.fsq_patternType = (pc.fsq_patternType & FULLSCREEN_BIT) + it.vg.state->pattern->type;
			pc0.fsq_patternType = pc.fsq_patternType;
			pc0.opacity = pc.opacity;
			pc0.mat = pc.mat; //glm::transpose(pc.mat);
			pc0.matInv = pc.matInv;// glm::transpose(pc.matInv);
			if (it.vg.state->pattern) {
				auto gr = *(vg_gradient_t*)it.vg.state->pattern->data;
				glm::mat3 patmat = it.vg.state->pattern->matrix;
				mul_pat(&gr, it.vg.state->pattern->type, pc.mat, patmat);
				SDL_PushGPUFragmentUniformData(cmd, 0, &gr, sizeof(vg_gradient_t));
			}
			SDL_PushGPUVertexUniformData(cmd, 0, &pc0, sizeof(pc0));

			if (it.vg.index.y < 1)
			{
				SDL_DrawGPUPrimitives(g->pass, it.vg.vertex.y, 1, it.vg.vertex.x, 0);
			}
		}
		break;
		case 1:
		{

		}
		break;
		}
	}

	SDL_EndGPURenderPass(g->pass);

	SDL_SubmitGPUCommandBuffer(g->cmd);
	SDL_WaitForGPUIdle(g->device);
	for (; g->rq.size();)
	{
		auto staging = g->rq.front(); g->rq.pop();
		if (staging)
			SDL_ReleaseGPUTransferBuffer(g->device, staging);
	}
	return 0;
}
void pattern_add_color_stop(vg_gradient_t* grad, float o, float r, float g, float b, float a) {

	if (grad->count < MAX_STOPS)
	{
		glm::vec4 c = { r, g, b, a };
		grad->colors[grad->count] = c;
		grad->stops[grad->count] = o;
		grad->count++;
	}
}
int pattern_edit_radial(vg_gradient_t* grad, float cx0, float cy0, float radius0, float cx1, float cy1, float radius1, bool is_ellipse) {
	if (!grad)
		return -2;
	*grad = {};
	glm::vec2 c0 = { cx0, cy0 };
	glm::vec2 c1 = { cx1, cy1 };
	if (radius0 > radius1 - 1.0f)
		radius0 = radius1 - 1.0f;
	glm::vec2  u = (c0 - c1);
	float l = glm::length(u);
	if (l + radius0 + 1.0f >= radius1) {
		glm::vec2 v = (u / l);
		c0 = (c1 + (v * (radius1 - radius0 - 1.0f)));
	}
	grad->cp[0] = glm::vec4{ {c0.x}, {c0.y}, {radius0}, {0} };
	grad->cp[1] = glm::vec4{ {c1.x}, {c1.y}, {radius1}, {0} };
	grad->m = glm::ivec4(1024, 0, 0, 1024);
	grad->extend = 0;
	grad->scale = glm::vec2{ 1.0,1.0 };
	if (is_ellipse)grad->scale.x *= 2;
	return 0;
}
int pattern_edit_sweep(vg_gradient_t* grad, float cx, float cy, float start_angle, float end_angle) {
	if (!grad)
		return -1;
	*grad = {};
	grad->cp[0] = glm::vec4{ cx, cy, start_angle, end_angle };
	grad->m = glm::ivec4(1024, 0, 0, 1024);
	grad->extend = 0;
	grad->scale = glm::vec2{ 1.0,1.0 };
	return 0;
}
#ifdef CreateRgbaf
#undef CreateRgbaf
#endif
#define CreateRgbaf(r, g, b, a)     (((int)(a * 255.0f) << 24) | ((int)(b * 255.0f) << 16) | ((int)(g * 255.0f) << 8) | (int)(r * 255.0f))

struct vkvg_matrix_t {
	float xx;
	float yx;
	float xy;
	float yy;
	float x0;
	float y0;
};
void vkvg_matrix_init(vkvg_matrix_t* matrix, float xx, float yx, float xy, float yy, float x0, float y0) {
	matrix->xx = xx;
	matrix->yx = yx;
	matrix->xy = xy;
	matrix->yy = yy;
	matrix->x0 = x0;
	matrix->y0 = y0;
}
void vg_sort_gradient_stops(vg_gradient_t* grad, float* stops, uint32_t count) {
	auto colors = grad->colors;
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
void mul_pat(vg_gradient_t* grad, int type, const glm::mat3& mat, const glm::mat3& pat_mat) {
	glm::vec3 cp0[2] = { glm::vec3(grad->cp[0].x, grad->cp[0].y,1.0f) ,glm::vec3(grad->cp[0].z, grad->cp[0].w,1.0f) };
	glm::vec3 cp1[2] = { glm::vec3(grad->cp[1].x, grad->cp[1].y,1.0f) ,glm::vec3(grad->cp[1].z, grad->cp[1].w,1.0f) };
	auto m = mat * pat_mat;
	cp0[0] = m * cp0[0];
	if (type == PAT_LINEAR) {
		cp0[1] = m * cp0[1];
	}
	else {
		cp1[0] = m * cp1[0];
		// radii
		cp0[1].z = 0.0;
		cp0[1] = m * cp0[1];
		cp1[1].z = 0.0;
		cp1[1] = m * cp1[1];
		//if (pat->hasMatrix) {
		//	vkvg_matrix_transform_distance(&mat, &grad.cp[0].z, &grad.cp[0].w);
		//	vkvg_matrix_transform_distance(&mat, &grad.cp[1].z, &grad.cp[0].w);
		//}
		//vkvg_matrix_transform_distance(&st->pushConsts.mat, &grad.cp[0].z, &grad.cp[0].w);
		//vkvg_matrix_transform_distance(&st->pushConsts.mat, &grad.cp[1].z, &grad.cp[0].w);
	}
	grad->cp[0] = glm::vec4(glm::vec2(cp0[0]), glm::vec2(cp0[1]));
	grad->cp[1] = glm::vec4(glm::vec2(cp1[0]), glm::vec2(cp1[1]));
	vg_sort_gradient_stops(grad, grad->stops, grad->count);
}

/* ── 9.  Per-frame rendering ───────────────────────────────────── */
void VG_RenderFrame(VGState* g, ovg_draw_data* data) {
	int w, h;
	SDL_GetWindowSizeInPixels(g->window, &w, &h);
	float fw = (float)w, fh = (float)h;
	if (!g->emptyImg)
	{
		g->emptyImg = CreateWhiteTexture16x16(g->device);
		g->sampler = CreateLinearSampler(g->device);
	}
	begin_frame(g);
	g->data = data;
	submit_draw(g);
}

