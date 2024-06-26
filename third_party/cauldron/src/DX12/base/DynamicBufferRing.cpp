﻿// AMD Cauldron code
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

#include "stdafx.h"
#include "DynamicBufferRing.h"
#include "Misc/Misc.h"

namespace CAULDRON_DX12
{
	//--------------------------------------------------------------------------------------
	//
	// OnCreate
	//
	//--------------------------------------------------------------------------------------
	void DynamicBufferRing::OnCreate(Device* pDevice, uint32_t numberOfBackBuffers, uint32_t memTotalSize, ResourceViewHeaps* pHeaps)
	{
		m_memTotalSize = AlignUp(memTotalSize, 256u);

		m_mem.OnCreate(numberOfBackBuffers, memTotalSize);
		auto p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto p1 = CD3DX12_RESOURCE_DESC::Buffer(memTotalSize);
		ThrowIfFailed(pDevice->GetDevice()->CreateCommittedResource(
			&p,
			D3D12_HEAP_FLAG_NONE,
			&p1,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pBuffer)));
		SetName(m_pBuffer, "DynamicBufferRing::m_pBuffer");

		m_pBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pData));
	}

	//--------------------------------------------------------------------------------------
	//
	// OnDestroy
	//
	//--------------------------------------------------------------------------------------
	void DynamicBufferRing::OnDestroy()
	{
		m_pBuffer->Release();
		m_mem.OnDestroy();
	}

	//--------------------------------------------------------------------------------------
	//
	// AllocConstantBuffer
	//
	//--------------------------------------------------------------------------------------
	bool DynamicBufferRing::AllocConstantBuffer(uint32_t size, void** pData, D3D12_GPU_VIRTUAL_ADDRESS* pBufferViewDesc)
	{
		size = AlignUp(size, 256u);

		uint32_t memOffset;
		if (m_mem.Alloc(size, &memOffset) == false)
		{
			Trace("Ran out of mem for 'dynamic' buffers, please increase the allocated size\n");
			return false;
		}

		*pData = (void*)(m_pData + memOffset);

		*pBufferViewDesc = m_pBuffer->GetGPUVirtualAddress() + memOffset;

		return true;
	}

	D3D12_GPU_VIRTUAL_ADDRESS DynamicBufferRing::AllocConstantBuffer(uint32_t size, const void* pInitData)
	{
		void* pBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS bufferViewDesc;
		if (AllocConstantBuffer(size, &pBuffer, &bufferViewDesc))
		{
			memcpy(pBuffer, pInitData, size);
		}

		return bufferViewDesc;
	}

	//--------------------------------------------------------------------------------------
	//
	// AllocVertexBuffer
	//
	//--------------------------------------------------------------------------------------
	bool DynamicBufferRing::AllocVertexBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void** pData, D3D12_VERTEX_BUFFER_VIEW* pView)
	{
		uint32_t size = AlignUp(numbeOfVertices * strideInBytes, 256u);

		uint32_t memOffset;
		if (m_mem.Alloc(size, &memOffset) == false)
			return false;

		*pData = (void*)(m_pData + memOffset);

		pView->BufferLocation = m_pBuffer->GetGPUVirtualAddress() + memOffset;
		pView->StrideInBytes = strideInBytes;
		pView->SizeInBytes = size;

		return true;
	}

	bool DynamicBufferRing::AllocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void** pData, D3D12_INDEX_BUFFER_VIEW* pView)
	{
		assert(strideInBytes == 2 || strideInBytes == 4);

		uint32_t size = AlignUp(numbeOfIndices * strideInBytes, 256u);

		uint32_t memOffset;
		if (m_mem.Alloc(size, &memOffset) == false)
			return false;

		*pData = (void*)(m_pData + memOffset);

		pView->BufferLocation = m_pBuffer->GetGPUVirtualAddress() + memOffset;
		pView->Format = (strideInBytes == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
		pView->SizeInBytes = size;

		return true;
	}

	//--------------------------------------------------------------------------------------
	//
	// OnBeginFrame
	//
	//--------------------------------------------------------------------------------------
	void DynamicBufferRing::OnBeginFrame()
	{
		m_mem.OnBeginFrame();
	}
}