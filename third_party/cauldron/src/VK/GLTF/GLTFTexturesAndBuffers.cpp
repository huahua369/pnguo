// AMD Cauldron code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
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
#include "Misc/Misc.h"
#include "GltfHelpers.h"
#include "Base/UploadHeap.h"
#include "Misc/ThreadPool.h"
#include "GLTFTexturesAndBuffers.h"
#include "../common/GLTF/GltfPbrMaterial.h"

namespace CAULDRON_VK
{
	bool GLTFTexturesAndBuffers::OnCreate(Device* pDevice, GLTFCommon* pGLTFCommon, UploadHeap* pUploadHeap, StaticBufferPool* pStaticBufferPool, DynamicBufferRing* pDynamicBufferRing)
	{
		m_pDevice = pDevice;
		m_pGLTFCommon = pGLTFCommon;
		m_pUploadHeap = pUploadHeap;
		m_pStaticBufferPool = pStaticBufferPool;
		m_pDynamicBufferRing = pDynamicBufferRing;

		return true;
	}
	void get_texdata(IMG_INFO& header, tinygltf::Image& img, std::vector<char>& tidata)
	{
		// Get the image data from the glTF loader


	}
	void GLTFTexturesAndBuffers::LoadTextures(AsyncPool* pAsyncPool)
	{
		// load textures and create views
		//
		auto pm = m_pGLTFCommon->pm;
		if (pm && pm->images.size())
		{
			auto& images = pm->images;

			std::vector<Async*> taskQueue(images.size());

			m_textures.resize(images.size());
			m_textureViews.resize(images.size());

			for (int imageIndex = 0; imageIndex < images.size(); imageIndex++)
			{
				Texture* pTex = &m_textures[imageIndex];
				std::string filename = m_pGLTFCommon->m_path + images[imageIndex].uri;// ["uri"] .get<std::string>();

				ExecAsyncIfThereIsAPool(pAsyncPool, [imageIndex, pTex, this, filename, pm]()
					{
						bool useSRGB;
						float cutOff;
						auto& img = pm->images[imageIndex];
						GetSrgbAndCutOffOfImageGivenItsUse(imageIndex, &pm->materials, &useSRGB, &cutOff);
						bool isimg = img.image.size();
						if (isimg)
						{
							IMG_INFO header = {};
							std::vector<char> tidata;
							unsigned char* buffer = nullptr;
							VkDeviceSize   bufferSize = 0;
							bool           deleteBuffer = false;
							// We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
							if (img.component == 3)
							{
								bufferSize = img.width * img.height * 4;
								tidata.resize(bufferSize);
								buffer = (unsigned char*)tidata.data();
								unsigned char* rgba = buffer;
								unsigned char* rgb = &img.image[0];
								for (size_t i = 0; i < img.width * img.height; ++i)
								{
									memcpy(rgba, rgb, sizeof(unsigned char) * 3);
									rgba += 4;
									rgb += 3;
								}
								deleteBuffer = true;
							}
							else
							{
								buffer = &img.image[0];
								bufferSize = img.image.size();
							}
							header.width = img.width;
							header.height = img.height;
							header.depth = 1;
							header.arraySize = 1;
							header.mipMapCount = 1;
#ifdef _WIN32
							header.format = DXGI_FORMAT_R8G8B8A8_UNORM;
#else
							header.vkformat = VK_FORMAT_R8G8B8A8_UNORM;
#endif
							header.bitCount = img.bits * img.component;
							auto name = img.name.size() ? img.name : filename;
							if (img.uri.empty())
								name += std::to_string(imageIndex);
							pTex->InitFromData(m_pDevice, m_pUploadHeap, &header, buffer, bufferSize, name.c_str(), useSRGB);
						}
						else
						{

							bool result = pTex->InitFromFile(m_pDevice, m_pUploadHeap, filename.c_str(), useSRGB, 0 /*VkImageUsageFlags*/, cutOff);
							assert(result != false);
						}
						//m_pUploadHeap->FlushAndFinish();
						m_textures[imageIndex].CreateSRV(&m_textureViews[imageIndex]);
					});
			}
			 
			if (pAsyncPool)
				pAsyncPool->Flush();

			// copy textures and apply barriers, then flush the GPU
			m_pUploadHeap->FlushAndFinish();
		}
#if 0
		else if (m_pGLTFCommon->j3.find("images") != m_pGLTFCommon->j3.end())
		{
			m_pTextureNodes = &m_pGLTFCommon->j3["textures"];
			const json& images = m_pGLTFCommon->j3["images"];
			const json& materials = m_pGLTFCommon->j3["materials"];

			std::vector<Async*> taskQueue(images.size());

			m_textures.resize(images.size());
			m_textureViews.resize(images.size());
			for (int imageIndex = 0; imageIndex < images.size(); imageIndex++)
			{
				Texture* pTex = &m_textures[imageIndex];
				std::string filename = m_pGLTFCommon->m_path + images[imageIndex]["uri"].get<std::string>();

				ExecAsyncIfThereIsAPool(pAsyncPool, [imageIndex, pTex, this, filename, materials]()
					{
						bool useSRGB;
						float cutOff;
						GetSrgbAndCutOffOfImageGivenItsUse(imageIndex, materials, &useSRGB, &cutOff);

						bool result = pTex->InitFromFile(m_pDevice, m_pUploadHeap, filename.c_str(), useSRGB, 0 /*VkImageUsageFlags*/, cutOff);
						assert(result != false);

						m_textures[imageIndex].CreateSRV(&m_textureViews[imageIndex]);
					});
			}

			LoadGeometry();

			if (pAsyncPool)
				pAsyncPool->Flush();

			// copy textures and apply barriers, then flush the GPU
			m_pUploadHeap->FlushAndFinish();
		}
#endif	
	}

	template<class T>
	void push_index(std::vector<uint32_t>& indexBuffer, T* buf, size_t count, size_t first = 0)
	{
		auto ps = indexBuffer.size();
		indexBuffer.resize(ps + count);
		for (size_t i = 0; i < count; i++)
		{
			indexBuffer[ps + i] = (first + buf[i]);
		}
	}
	void GLTFTexturesAndBuffers::LoadGeometry()
	{
		auto pm = m_pGLTFCommon->pm;
		if (pm && pm->meshes.size())
		{
			for (auto& mesh : pm->meshes)
			{
				for (auto& primitive : mesh.primitives)
				{
					//
					//  Load vertex buffers
					//
					for (auto& attributeId : primitive.attributes)
					{
						tfAccessor vertexBufferAcc;
						m_pGLTFCommon->GetBufferDetails(attributeId.second, &vertexBufferAcc);

						VkDescriptorBufferInfo vbv;
						m_pStaticBufferPool->AllocBuffer(vertexBufferAcc.m_count, vertexBufferAcc.m_stride, vertexBufferAcc.m_data, &vbv);

						m_vertexBufferMap[attributeId.second] = vbv;
					}

					//
					//  Load index buffers
					//
					int indexAcc = primitive.indices;// .value("indices", -1);
					if (indexAcc >= 0)
					{
						tfAccessor indexBufferAcc = {};
						m_pGLTFCommon->GetBufferDetails(indexAcc, &indexBufferAcc);

						VkDescriptorBufferInfo ibv;

						// Some exporters use 1-byte indices, need to convert them to shorts since the GPU doesn't support 1-byte indices
						if (indexBufferAcc.m_stride < 4)
						{
							std::vector<uint32_t> v;
							//v.resize(indexBufferAcc.m_count);
							//unsigned int* pIndices = v.data();// (unsigned int*)malloc(indexBufferAcc.m_count * (2 * indexBufferAcc.m_stride));
							//for (int i = 0; i < indexBufferAcc.m_count; i++)
							//pIndices[i] = ((unsigned char*)indexBufferAcc.m_data)[i];	
							switch (indexBufferAcc.m_stride)
							{
							case 1:
								push_index(v, (uint8_t*)indexBufferAcc.m_data, indexBufferAcc.m_count, 0);
								break;
							case 2:
								push_index(v, (uint16_t*)indexBufferAcc.m_data, indexBufferAcc.m_count, 0);
								break;
							default:
								break;
							}
							unsigned int* pIndices = v.data();
							indextype = 4;
							m_pStaticBufferPool->AllocBuffer(indexBufferAcc.m_count, sizeof(uint32_t) /*2 * indexBufferAcc.m_stride*/, pIndices, &ibv);
							//free(pIndices);
						}
						else
						{
							m_pStaticBufferPool->AllocBuffer(indexBufferAcc.m_count, indexBufferAcc.m_stride, indexBufferAcc.m_data, &ibv);
						}

						m_IndexBufferMap[indexAcc] = ibv;
					}
				}
			}
		}
#if 0
		else if (m_pGLTFCommon->j3.find("meshes") != m_pGLTFCommon->j3.end())
		{
			for (const json& mesh : m_pGLTFCommon->j3["meshes"])
			{
				for (const json& primitive : mesh["primitives"])
				{
					//
					//  Load vertex buffers
					//
					for (const json& attributeId : primitive["attributes"])
					{
						tfAccessor vertexBufferAcc;
						m_pGLTFCommon->GetBufferDetails(attributeId, &vertexBufferAcc);

						VkDescriptorBufferInfo vbv;
						m_pStaticBufferPool->AllocBuffer(vertexBufferAcc.m_count, vertexBufferAcc.m_stride, vertexBufferAcc.m_data, &vbv);

						m_vertexBufferMap[attributeId] = vbv;
					}

					//
					//  Load index buffers
					//
					int indexAcc = primitive.value("indices", -1);
					if (indexAcc >= 0)
					{
						tfAccessor indexBufferAcc;
						m_pGLTFCommon->GetBufferDetails(indexAcc, &indexBufferAcc);

						VkDescriptorBufferInfo ibv;

						// Some exporters use 1-byte indices, need to convert them to shorts since the GPU doesn't support 1-byte indices
						if (indexBufferAcc.m_stride == 1)
						{
							unsigned short* pIndices = (unsigned short*)malloc(indexBufferAcc.m_count * (2 * indexBufferAcc.m_stride));
							for (int i = 0; i < indexBufferAcc.m_count; i++)
								pIndices[i] = ((unsigned char*)indexBufferAcc.m_data)[i];
							m_pStaticBufferPool->AllocBuffer(indexBufferAcc.m_count, 2 * indexBufferAcc.m_stride, pIndices, &ibv);
							free(pIndices);
						}
						else
						{
							m_pStaticBufferPool->AllocBuffer(indexBufferAcc.m_count, indexBufferAcc.m_stride, indexBufferAcc.m_data, &ibv);
						}

						m_IndexBufferMap[indexAcc] = ibv;
					}
				}
			}
		}
#endif
	}

	void GLTFTexturesAndBuffers::OnDestroy()
	{
		for (int i = 0; i < m_textures.size(); i++)
		{
			vkDestroyImageView(m_pDevice->GetDevice(), m_textureViews[i], NULL);
			m_textures[i].OnDestroy();
		}
	}

	VkImageView GLTFTexturesAndBuffers::GetTextureViewByID(int id)
	{
		auto pm = m_pGLTFCommon->pm;
		int tex = 0;
		if (pm)
		{
			tex = pm->textures[id].source;
		}
		//else
		//	tex = m_pTextureNodes->at(id)["source"];
		if (tex < m_textureViews.size() && tex >= 0)
			return m_textureViews[tex];
		else
			return 0;
	}

	// Creates a Index Buffer from the accessor
	//
	//
	void GLTFTexturesAndBuffers::CreateIndexBuffer(int indexBufferId, uint32_t* pNumIndices, VkIndexType* pIndexType, VkDescriptorBufferInfo* pIBV)
	{
		tfAccessor indexBuffer;
		m_pGLTFCommon->GetBufferDetails(indexBufferId, &indexBuffer);

		*pNumIndices = indexBuffer.m_count;
		if (indextype > 0)
			indexBuffer.m_stride = indextype;
		*pIndexType = (indexBuffer.m_stride == 4) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

		*pIBV = m_IndexBufferMap[indexBufferId];
	}

	// Creates Vertex Buffers from accessors and sets them in the Primitive struct.
	//
	//
	void GLTFTexturesAndBuffers::CreateGeometry(int indexBufferId, std::vector<int>& vertexBufferIds, Geometry* pGeometry)
	{
		CreateIndexBuffer(indexBufferId, &pGeometry->m_NumIndices, &pGeometry->m_indexType, &pGeometry->m_IBV);

		// load the rest of the buffers onto the GPU
		pGeometry->m_VBV.resize(vertexBufferIds.size());
		for (int i = 0; i < vertexBufferIds.size(); i++)
		{
			pGeometry->m_VBV[i] = m_vertexBufferMap[vertexBufferIds[i]];
		}
	}

	// Creates buffers and the input assemby at the same time. It needs a list of attributes to use.
	//
	void GLTFTexturesAndBuffers::CreateGeometry(const json& primitive, const std::vector<std::string> requiredAttributes, std::vector<VkVertexInputAttributeDescription>& layout, DefineList& defines, Geometry* pGeometry)
	{
		// Get Index buffer view
		//
		tfAccessor indexBuffer;
		int indexBufferId = primitive.value("indices", -1);
		CreateIndexBuffer(indexBufferId, &pGeometry->m_NumIndices, &pGeometry->m_indexType, &pGeometry->m_IBV);

		// Create vertex buffers and input layout
		//
		int cnt = 0;
		layout.resize(requiredAttributes.size());
		pGeometry->m_VBV.resize(requiredAttributes.size());
		const json& attributes = primitive.at("attributes");
		for (auto attrName : requiredAttributes)
		{
			// get vertex buffer view
			// 
			const int attr = attributes.find(attrName).value();
			pGeometry->m_VBV[cnt] = m_vertexBufferMap[attr];

			// let the compiler know we have this stream
			defines[std::string("ID_") + attrName] = std::to_string(cnt);

			const json& inAccessor = m_pGLTFCommon->m_pAccessors->at(attr);

			// Create Input Layout
			//
			VkVertexInputAttributeDescription l = {};
			l.location = (uint32_t)cnt;
			l.format = GetFormat(inAccessor["type"], inAccessor["componentType"]);
			l.offset = 0;
			l.binding = cnt;
			layout[cnt] = l;

			cnt++;
		}
	}
	uint32_t GetVkFormat(int type, int id)
	{
		switch (type)
		{
		case TINYGLTF_TYPE_SCALAR://if (str == "SCALAR")
		{
			switch (id)
			{
			case 5120: return VK_FORMAT_R8_SINT; //(BYTE)
			case 5121: return VK_FORMAT_R8_UINT; //(UNSIGNED_BYTE)1
			case 5122: return VK_FORMAT_R16_SINT; //(SHORT)2
			case 5123: return VK_FORMAT_R16_UINT; //(UNSIGNED_SHORT)2
			case 5124: return VK_FORMAT_R32_SINT; //(SIGNED_INT)4
			case 5125: return VK_FORMAT_R32_UINT; //(UNSIGNED_INT)4
			case 5126: return VK_FORMAT_R32_SFLOAT; //(FLOAT)
			case 5130: return VK_FORMAT_R64_SFLOAT; //(DOUBLE)
			}
		}
		case TINYGLTF_TYPE_VEC2://else if (str == "VEC2")
		{
			switch (id)
			{
			case 5120: return VK_FORMAT_R8G8_SINT; //(BYTE)
			case 5121: return VK_FORMAT_R8G8_UINT; //(UNSIGNED_BYTE)1
			case 5122: return VK_FORMAT_R16G16_SINT; //(SHORT)2
			case 5123: return VK_FORMAT_R16G16_UINT; //(UNSIGNED_SHORT)2
			case 5124: return VK_FORMAT_R32G32_SINT; //(SIGNED_INT)4
			case 5125: return VK_FORMAT_R32G32_UINT; //(UNSIGNED_INT)4
			case 5126: return VK_FORMAT_R32G32_SFLOAT; //(FLOAT)
			case 5130: return VK_FORMAT_R64G64_SFLOAT; //(DOUBLE)
			}
		}
		case TINYGLTF_TYPE_VEC3://else if (str == "VEC3")
		{
			switch (id)
			{
			case 5120: return VK_FORMAT_UNDEFINED; //(BYTE)
			case 5121: return VK_FORMAT_UNDEFINED; //(UNSIGNED_BYTE)1
			case 5122: return VK_FORMAT_UNDEFINED; //(SHORT)2
			case 5123: return VK_FORMAT_UNDEFINED; //(UNSIGNED_SHORT)2
			case 5124: return VK_FORMAT_R32G32B32_SINT; //(SIGNED_INT)4
			case 5125: return VK_FORMAT_R32G32B32_UINT; //(UNSIGNED_INT)4
			case 5126: return VK_FORMAT_R32G32B32_SFLOAT; //(FLOAT)
			case 5130: return VK_FORMAT_R64G64B64_SFLOAT; //(DOUBLE)
			}
		}
		case TINYGLTF_TYPE_VEC4://else if (str == "VEC4")
		{
			switch (id)
			{
			case 5120: return VK_FORMAT_R8G8B8A8_SINT; //(BYTE)
			case 5121: return VK_FORMAT_R8G8B8A8_UINT; //(UNSIGNED_BYTE)1
			case 5122: return VK_FORMAT_R16G16B16A16_SINT; //(SHORT)2
			case 5123: return VK_FORMAT_R16G16B16A16_UINT; //(UNSIGNED_SHORT)2
			case 5124: return VK_FORMAT_R32G32B32A32_SINT; //(SIGNED_INT)4
			case 5125: return VK_FORMAT_R32G32B32A32_UINT; //(UNSIGNED_INT)4
			case 5126: return VK_FORMAT_R32G32B32A32_SFLOAT; //(FLOAT)
			case 5130: return VK_FORMAT_R64G64B64A64_SFLOAT; //(DOUBLE)
			}
		}
		}
		return VK_FORMAT_UNDEFINED;
	}

	void GLTFTexturesAndBuffers::CreateGeometry(tinygltf::Primitive* primitive, const std::vector<std::string> requiredAttributes, std::vector<VkVertexInputAttributeDescription>& layout, DefineList& defines, Geometry* pGeometry)
	{

		// Get Index buffer view
		//
		tfAccessor indexBuffer;
		int indexBufferId = primitive->indices;// .value("indices", -1);
		CreateIndexBuffer(indexBufferId, &pGeometry->m_NumIndices, &pGeometry->m_indexType, &pGeometry->m_IBV);

		// Create vertex buffers and input layout
		//
		int cnt = 0;
		layout.resize(requiredAttributes.size());
		pGeometry->m_VBV.resize(requiredAttributes.size());
		auto& attributes = primitive->attributes;
		auto pm = m_pGLTFCommon->pm;
		for (auto attrName : requiredAttributes)
		{
			// get vertex buffer view
			// 
			const int attr = attributes[attrName];
			pGeometry->m_VBV[cnt] = m_vertexBufferMap[attr];

			// let the compiler know we have this stream
			defines[std::string("ID_") + attrName] = std::to_string(cnt);
			auto& ina = pm->accessors[attr];
			// Create Input Layout
			//
			VkVertexInputAttributeDescription l = {};
			l.location = (uint32_t)cnt;
			l.format = (VkFormat)GetVkFormat(ina.type, ina.componentType);
			l.offset = 0;
			l.binding = cnt;
			layout[cnt] = l;

			cnt++;
		}
	}
	void GLTFTexturesAndBuffers::SetPerFrameConstants()
	{
		per_frame* cbPerFrame;
		m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_frame), (void**)&cbPerFrame, &m_perFrameConstants);
		*cbPerFrame = m_pGLTFCommon->m_perFrameData;
	}

	void GLTFTexturesAndBuffers::SetSkinningMatricesForSkeletons()
	{
		for (auto& t : m_pGLTFCommon->m_worldSpaceSkeletonMats)
		{
			std::vector<Matrix2>* matrices = &t.second;

			VkDescriptorBufferInfo perSkeleton = {};
			Matrix2* cbPerSkeleton;
			uint32_t size = (uint32_t)(matrices->size() * sizeof(Matrix2));
			m_pDynamicBufferRing->AllocConstantBuffer(size, (void**)&cbPerSkeleton, &perSkeleton);
			memcpy(cbPerSkeleton, matrices->data(), size);
			m_skeletonMatricesBuffer[t.first] = perSkeleton;
		}
	}

	VkDescriptorBufferInfo* GLTFTexturesAndBuffers::GetSkinningMatricesBuffer(int skinIndex)
	{
		auto it = m_skeletonMatricesBuffer.find(skinIndex);

		if (it == m_skeletonMatricesBuffer.end())
			return NULL;

		return &it->second;
	}
}
