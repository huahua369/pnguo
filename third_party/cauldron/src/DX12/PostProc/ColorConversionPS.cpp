// AMD Cauldron code
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
#include "Base/DynamicBufferRing.h"
#include "Base/StaticBufferPool.h"
#include "Base/UploadHeap.h"
#include "Base/FreesyncHDR.h"
#include "Misc/ColorConversion.h"
#include "ColorConversionPS.h"

namespace CAULDRON_DX12
{
    void ColorConversionPS::OnCreate(Device* pDevice, ResourceViewHeaps *pResourceViewHeaps, DynamicBufferRing *pDynamicBufferRing, StaticBufferPool  *pStaticBufferPool, DXGI_FORMAT outFormat)
    {
        m_pDynamicBufferRing = pDynamicBufferRing;

        D3D12_STATIC_SAMPLER_DESC SamplerDesc = {};
        SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        SamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        SamplerDesc.MinLOD = 0.0f;
        SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        SamplerDesc.MipLODBias = 0;
        SamplerDesc.MaxAnisotropy = 1;
        SamplerDesc.ShaderRegister = 0;
        SamplerDesc.RegisterSpace = 0;
        SamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        m_ColorConversion.OnCreate(pDevice, "ColorConversionPS.hlsl", pResourceViewHeaps, pStaticBufferPool, 1, 1, &SamplerDesc, outFormat);
    }

    void ColorConversionPS::OnDestroy()
    {
        m_ColorConversion.OnDestroy();
    }

    void ColorConversionPS::UpdatePipelines(DXGI_FORMAT outFormat, DisplayMode displayMode)
    {
        m_ColorConversion.UpdatePipeline(outFormat);

        m_colorConversionConsts.m_displayMode = displayMode;

        if (displayMode != DISPLAYMODE_SDR)
        {
            const DXGI_OUTPUT_DESC1 *displayInfo = GetDisplayInfo();

            m_colorConversionConsts.m_displayMinLuminancePerNits = displayInfo->MinLuminance / 80.0f; // RGB(1, 1, 1) maps to 80 nits in scRGB;
            m_colorConversionConsts.m_displayMaxLuminancePerNits = displayInfo->MaxLuminance / 80.0f; // This means peak white equals RGB(m_maxLuminanace/80.0f, m_maxLuminanace/80.0f, m_maxLuminanace/80.0f) in scRGB;

            FillDisplaySpecificPrimaries(
                displayInfo->WhitePoint[0], displayInfo->WhitePoint[1],
                displayInfo->RedPrimary[0], displayInfo->RedPrimary[1],
                displayInfo->GreenPrimary[0], displayInfo->GreenPrimary[1],
                displayInfo->BluePrimary[0], displayInfo->BluePrimary[1]
            );

            SetupGamutMapperMatrices(
                ColorSpace_REC709,
                ColorSpace_Display,
                &m_colorConversionConsts.m_contentToMonitorRecMatrix);
        }
    }

    void ColorConversionPS::Draw(ID3D12GraphicsCommandList* pCommandList, CBV_SRV_UAV *pHDRSRV)
    {
        UserMarker marker(pCommandList, "ColorConversionPS");

        D3D12_GPU_VIRTUAL_ADDRESS cbTonemappingHandle;
        ColorConversionConsts *pColorConversionConsts;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(ColorConversionConsts), (void **)&pColorConversionConsts, &cbTonemappingHandle);
        *pColorConversionConsts = m_colorConversionConsts;

        m_ColorConversion.Draw(pCommandList, 1, pHDRSRV, cbTonemappingHandle);
    }
}
