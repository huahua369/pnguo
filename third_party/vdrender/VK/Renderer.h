// AMD SampleVK sample code
// 
// Copyright(c) 2020 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#pragma once

#include "stdafx.h"

#include "base/GBuffer.h"
#include "PostProc/MagnifierPS.h"

// We are queuing (backBufferCount + 0.5) frames, so we need to triple buffer the resources that get modified each frame
static const int backBufferCount = 3;

using namespace CAULDRON_VK;

struct UIState;

typedef struct {
	Texture         ShadowMap;
	uint32_t        ShadowIndex;
	uint32_t        ShadowResolution;
	uint32_t        LightIndex;
	VkImageView     ShadowDSV;
	VkFramebuffer   ShadowFrameBuffer;
} SceneShadowInfo;

struct const_vk {
	// Create all the heaps for the resources views
	uint32_t cbvDescriptorCount = 2000;
	uint32_t srvDescriptorCount = 8000;
	uint32_t uavDescriptorCount = 10;
	uint32_t samplerDescriptorCount = 20;
	// Create a commandlist ring for the Direct queue
	uint32_t commandListsPerBackBuffer = 8;
	// Create a 'dynamic' constant buffer
	uint32_t constantBuffersMemSize = 200 * 1024 * 1024;
	// Create a 'static' pool for vertices and indices 
	uint32_t staticGeometryMemSize = (1 * 128) * 1024 * 1024;
	// Create a 'static' pool for vertices and indices in system memory
	uint32_t systemGeometryMemSize = 32 * 1024;

	// Quick helper to upload resources, it has it's own commandList and uses suballocation.
	uint32_t uploadHeapMemSize = 1000 * 1024 * 1024;
};
struct robj_info {
	//gltf passes
	GltfPbrPass* m_GLTFPBR;
	GltfBBoxPass* m_GLTFBBox;
	GltfDepthPass* m_GLTFDepth;
	GLTFTexturesAndBuffers* m_pGLTFTexturesAndBuffers;


};
//
// Renderer class is responsible for rendering resources management and recording command buffers.
class Renderer
{
public:
	Renderer(const_vk* p);
	~Renderer();
	void OnCreate(Device* pDevice, SwapChain* pSwapChain, float FontSize);
	void OnDestroy();

	void OnCreateWindowSizeDependentResources(SwapChain* pSwapChain, uint32_t Width, uint32_t Height);
	void OnDestroyWindowSizeDependentResources();

	void OnUpdateDisplayDependentResources(SwapChain* pSwapChain, bool bUseMagnifier);
	void OnUpdateLocalDimmingChangedResources(SwapChain* pSwapChain);

	int LoadScene(GLTFCommon* pGLTFCommon, int Stage = 0);
	void UnloadScene();
	void unloadgltf(robj_info* p);

	void AllocateShadowMaps(GLTFCommon* pGLTFCommon);

	const std::vector<TimeStamp>& GetTimingValues() { return m_TimeStamps; }

	void OnRender(const UIState* pState, const Camera& Cam, SwapChain* pSwapChain);

private:
	Device* m_pDevice = 0;
	const_vk ct = {};
	uint32_t                        m_Width = {};
	uint32_t                        m_Height = {};

	VkRect2D                        m_RectScissor = {};
	VkViewport                      m_Viewport = {};

	// Initialize helper classes
	ResourceViewHeaps               m_ResourceViewHeaps = {};
	UploadHeap                      m_UploadHeap = {};
	DynamicBufferRing               m_ConstantBufferRing = {};
	StaticBufferPool                m_VidMemBufferPool = {};
	StaticBufferPool                m_SysMemBufferPool = {};
	CommandListRing                 m_CommandListRing = {};
	GPUTimestamps                   m_GPUTimer = {};

	// effects
	Bloom                           m_Bloom = {};
	SkyDome                         m_SkyDome = {};
	DownSamplePS                    m_DownSample = {};
	SkyDomeProc                     m_SkyDomeProc = {};
	ToneMapping                     m_ToneMappingPS = {};
	ToneMappingCS                   m_ToneMappingCS = {};
	ColorConversionPS               m_ColorConversionPS = {};
	TAA                             m_TAA = {};
	MagnifierPS                     m_MagnifierPS = {};
	bool                            m_bMagResourceReInit = false;

	// GBuffer and render passes
	GBuffer                         m_GBuffer = {};
	GBufferRenderPass               m_RenderPassFullGBufferWithClear = {};
	GBufferRenderPass               m_RenderPassJustDepthAndHdr = {};
	GBufferRenderPass               m_RenderPassFullGBuffer = {};

	// shadowmaps
	VkRenderPass                    m_Render_pass_shadow = {};


	// widgets
	Wireframe                       m_Wireframe = {};
	WireframeBox                    m_WireframeBox = {};
	//Axis							_axis= {};
	CheckerBoardFloor				_cbf = {};
	std::vector<TimeStamp>          m_TimeStamps = {};

	AsyncPool                       m_AsyncPool = {};
	// 渲染对象
	std::vector<robj_info*>         _robject = {};
	robj_info* currobj = {};

	std::vector<SceneShadowInfo>    m_shadowMapPool = {};
	std::vector<VkImageView>        m_ShadowSRVPool = {};
	std::vector<GltfDepthPass*>     _depthpass = {};
	// GUI
	ImGUI                           m_ImGUI = {};
	bool bHDR = false;
};

