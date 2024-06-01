// AMD SampleDX12 sample code
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

struct UIState;

// We are queuing (backBufferCount + 0.5) frames, so we need to triple buffer the resources that get modified each frame
static const int backBufferCount = 3;

#define USE_SHADOWMASK false

using namespace CAULDRON_DX12;
struct const_dx12 {

	// Create all the heaps for the resources views
	uint32_t cbvDescriptorCount = 4000;
	uint32_t srvDescriptorCount = 8000;
	uint32_t uavDescriptorCount = 10;
	uint32_t dsvDescriptorCount = 10;
	uint32_t rtvDescriptorCount = 60;
	uint32_t samplerDescriptorCount = 20;
	// Create a commandlist ring for the Direct queue
	uint32_t commandListsPerBackBuffer = 8;

	// Create a 'dynamic' constant buffer
	uint32_t constantBuffersMemSize = 200 * 1024 * 1024;

	// Create a 'static' pool for vertices, indices and constant buffers
	uint32_t staticGeometryMemSize = (5 * 128) * 1024 * 1024;

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
	Renderer(const_dx12* p);
	~Renderer();
	void OnCreate(Device* pDevice, SwapChain* pSwapChain, float FontSize);
	void OnDestroy();

	void OnCreateWindowSizeDependentResources(SwapChain* pSwapChain, uint32_t Width, uint32_t Height);
	void OnDestroyWindowSizeDependentResources();

	void OnUpdateDisplayDependentResources(SwapChain* pSwapChain);

	int LoadScene(GLTFCommon* pGLTFCommon, int Stage = 0);
	void UnloadScene();

	void AllocateShadowMaps(GLTFCommon* pGLTFCommon);

	const std::vector<TimeStamp>& GetTimingValues() const { return m_TimeStamps; }
	std::string& GetScreenshotFileName() { return m_pScreenShotName; }

	void OnRender(const UIState* pState, const Camera& Cam, SwapChain* pSwapChain);
	void unloadgltf(robj_info* p);
private:
	Device* m_pDevice={};
	const_dx12 ct = {};
	uint32_t                        m_Width= {};
	uint32_t                        m_Height= {};
	D3D12_VIEWPORT                  m_Viewport= {};
	D3D12_RECT                      m_RectScissor= {};
	bool                            m_HasTAA = false;

	// Initialize helper classes
	ResourceViewHeaps               m_ResourceViewHeaps= {};
	UploadHeap                      m_UploadHeap= {};
	DynamicBufferRing               m_ConstantBufferRing= {};
	StaticBufferPool                m_VidMemBufferPool= {};     // todo
	CommandListRing                 m_CommandListRing= {};
	GPUTimestamps                   m_GPUTimer= {};

	// effects
	Bloom                           m_Bloom= {};
	SkyDome                         m_SkyDome= {};
	DownSamplePS                    m_DownSample= {};
	SkyDomeProc                     m_SkyDomeProc= {};
	ToneMapping                     m_ToneMappingPS= {};
	ToneMappingCS                   m_ToneMappingCS= {};
	ColorConversionPS               m_ColorConversionPS= {};
	TAA                             m_TAA= {};
	MagnifierPS                     m_MagnifierPS= {};

	// GUI
	ImGUI                           m_ImGUI= {};

	// Temporary render targets
	GBuffer                         m_GBuffer= {};
	GBufferRenderPass               m_RenderPassFullGBuffer= {};
	GBufferRenderPass               m_RenderPassJustDepthAndHdr= {};

	Texture                         m_MotionVectorsDepthMap= {};
	DSV                             m_MotionVectorsDepthMapDSV= {};
	CBV_SRV_UAV                     m_MotionVectorsDepthMapSRV= {};

#if USE_SHADOWMASK
	// shadow mask
	Texture                         m_ShadowMask= {};
	CBV_SRV_UAV                     m_ShadowMaskUAV= {};
	CBV_SRV_UAV                     m_ShadowMaskSRV= {};
	ShadowResolvePass               m_shadowResolve= {};
#endif

	// shadowmaps
	typedef struct {
		Texture     ShadowMap;
		uint32_t    ShadowIndex;
		uint32_t    ShadowResolution;
		uint32_t    LightIndex;
	} SceneShadowInfo;

	std::vector<SceneShadowInfo>    m_shadowMapPool= {};
	DSV                             m_ShadowMapPoolDSV= {};
	CBV_SRV_UAV                     m_ShadowMapPoolSRV= {};

	// widgets
	Wireframe                       m_Wireframe= {};
	WireframeBox                    m_WireframeBox= {};

	std::vector<TimeStamp>          m_TimeStamps= {};

	// screen shot
	std::string                     m_pScreenShotName = "";
	SaveTexture                     m_SaveTexture= {};
	AsyncPool                       m_AsyncPool= {};

	// ��Ⱦ����
	std::vector<robj_info*>         _robject= {};
	robj_info* currobj = 0;
	std::vector<GltfDepthPass*>     _depthpass= {};
};
