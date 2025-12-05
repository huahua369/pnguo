/*

vk渲染器
Copyright (c) 华仔
188665600@qq.com

创建日期：205-12-05

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
#include <tiny_gltf.h>
#include <zlib.h>
#include <queue> 

#include "vkrenderer_core.h"

#ifdef min
#undef min
#undef max
#endif // min

// set创建管理  
namespace vkc
{

#if 1
	// 创建描述符堆
	bool new_set_layout(VkDevice pDevice, VkDescriptorSetLayoutBinding* layout_binding, uint32_t count, uint32_t maxcount, layout_set_pool* opt)
	{
		if (!pDevice || !layout_binding || count == 0 || maxcount == 0 || !opt)
			return false;
		VkDescriptorPoolSize tc[10] = {};
		VkDescriptorPoolSize tc1[10] = {};
		for (uint32_t i = 0; i < count; i++)
		{
			auto& it = layout_binding[i];
			tc[it.descriptorType].type = it.descriptorType;
			tc[it.descriptorType].descriptorCount += it.descriptorCount;
		}
		uint32_t n = 0;
		for (uint32_t i = 0; i < 10; i++)
		{
			if (tc[i].descriptorCount > 0)
			{
				tc1[n++] = tc[i];
			}
		}
		VkDescriptorPoolCreateInfo descriptor_pool = {};
		descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptor_pool.pNext = NULL;
		descriptor_pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		descriptor_pool.maxSets = maxcount;
		descriptor_pool.poolSizeCount = n;
		descriptor_pool.pPoolSizes = tc1;
		VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
		descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_layout.pNext = NULL;
		descriptor_layout.bindingCount = (uint32_t)count;
		descriptor_layout.pBindings = layout_binding;
		do
		{
			VkResult res = vkCreateDescriptorPool(pDevice, &descriptor_pool, NULL, &opt->descriptorPool);
			assert(res == VK_SUCCESS);
			if (res != VK_SUCCESS)
				break;
			res = vkCreateDescriptorSetLayout(pDevice, &descriptor_layout, NULL, &opt->descSetLayout);
			assert(res == VK_SUCCESS);
			if (res != VK_SUCCESS)
			{
				vkDestroyDescriptorPool(pDevice, opt->descriptorPool, NULL);
				break;
			}
			opt->alloc_count = 0;
			opt->max_count = maxcount;
			opt->dev = pDevice;
		} while (0);
		return true;
	}

	void free_layout_set_pool(layout_set_pool* p)
	{
		vkDestroyDescriptorSetLayout(p->dev, p->descSetLayout, NULL);
		vkDestroyDescriptorPool(p->dev, p->descriptorPool, NULL);
	}


	VkDescriptorSet alloc_descriptor(layout_set_pool* p)
	{
#ifdef _HAS_VK_MULTITHREADING
		std::lock_guard<std::mutex> lock(p->_mutex);
#endif
		VkDescriptorSetAllocateInfo alloc_info;
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.pNext = NULL;
		alloc_info.descriptorPool = p->descriptorPool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = &p->descSetLayout;
		VkDescriptorSet descriptorSet = {};
		VkResult res = vkAllocateDescriptorSets(p->dev, &alloc_info, &descriptorSet);
		assert(res == VK_SUCCESS);
		if (res == VK_SUCCESS)
		{
			p->alloc_count++;
		}
		else {
			descriptorSet = nullptr;
		}
		return descriptorSet;
	}

	void free_descriptor(layout_set_pool* p, VkDescriptorSet descriptorSet)
	{
		p->alloc_count--;
		vkFreeDescriptorSets(p->dev, p->descriptorPool, 1, &descriptorSet);
	}
	// pSamplers如果不为空时应该和size对应
	VkDescriptorSet alloc_descriptor_image(VkDevice pDevice, int size, const VkSampler* pSamplers, layout_set_pool* opt, uint32_t maxcount)
	{
		VkDescriptorSet dset = {};
		if (!pDevice || size < 1 || !opt)
			return dset;
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(size);
		for (int i = 0; i < size; i++)
		{
			layoutBindings[i].binding = i;
			layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBindings[i].descriptorCount = 1;
			layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBindings[i].pImmutableSamplers = (pSamplers != NULL) ? &pSamplers[i] : NULL;
		}
		if (maxcount < 1) { maxcount = 1; }
		if (new_set_layout(pDevice, layoutBindings.data(), layoutBindings.size(), maxcount, opt)) {
			dset = alloc_descriptor(opt);
		}
		return dset;
	}

	// pSamplers如果不为空时应该和descriptorCounts.size()对应
	VkDescriptorSet alloc_descriptor_images(VkDevice pDevice, std::vector<uint32_t>& descriptorCounts, const VkSampler* pSamplers, layout_set_pool* opt, uint32_t maxcount)
	{
		VkDescriptorSet dset = {};
		if (!pDevice || descriptorCounts.empty() || !opt)
			return dset;
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(descriptorCounts.size());
		for (int i = 0; i < descriptorCounts.size(); i++)
		{
			layoutBindings[i].binding = i;
			layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBindings[i].descriptorCount = descriptorCounts[i];
			layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBindings[i].pImmutableSamplers = (pSamplers != NULL) ? &pSamplers[i] : NULL;
		}
		if (maxcount < 1) { maxcount = 1; }
		if (new_set_layout(pDevice, layoutBindings.data(), layoutBindings.size(), maxcount, opt)) {
			dset = alloc_descriptor(opt);
		}
		return dset;
	}

	void updatedsets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies)
	{
		if (pDescriptorWrites && pDescriptorWrites->pBufferInfo) {
			if (!pDescriptorWrites->pBufferInfo->buffer)
				assert(pDescriptorWrites->pBufferInfo->offset == 0 && pDescriptorWrites->pBufferInfo->range == 0);
		}
		vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
	}
	void set_descriptorset(VkDevice device, VkBuffer buffer, int index, uint32_t size, VkDescriptorSet descriptorSet, VkDescriptorType dt)
	{
		VkDescriptorBufferInfo out = {};
		assert(buffer);
		out.buffer = buffer;
		out.offset = 0;
		out.range = size;
		VkWriteDescriptorSet write;
		write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = NULL;
		write.dstSet = descriptorSet;
		write.descriptorCount = 1;
		write.descriptorType = dt; ;
		write.pBufferInfo = &out;
		write.dstArrayElement = 0;
		write.dstBinding = index;
		updatedsets(device, 1, &write, 0, NULL);
	}
	void set_descriptorset1(VkDevice device, VkBuffer buffer, int index, uint32_t pos, uint32_t size, VkDescriptorSet descriptorSet, VkDescriptorType dt)
	{
		VkDescriptorBufferInfo out = {};
		out.buffer = buffer;
		out.offset = pos;
		out.range = size;
		assert(buffer);
		VkWriteDescriptorSet write;
		write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = NULL;
		write.dstSet = descriptorSet;
		write.descriptorCount = 1;
		write.descriptorType = dt;
		write.pBufferInfo = &out;
		write.dstArrayElement = 0;
		write.dstBinding = index;
		updatedsets(device, 1, &write, 0, NULL);
	}

	void set_descriptorset(VkDevice device, uint32_t index, VkImageView imageView, VkImageLayout imageLayout, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		VkDescriptorImageInfo desc_image;
		desc_image.sampler = pSampler;
		desc_image.imageView = imageView;
		desc_image.imageLayout = imageLayout;
		VkWriteDescriptorSet write;
		write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = NULL;
		write.dstSet = descriptorSet;
		write.descriptorCount = 1;
		write.descriptorType = (pSampler == NULL) ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo = &desc_image;
		write.dstBinding = index;
		write.dstArrayElement = 0;
		updatedsets(device, 1, &write, 0, NULL);
	}
	void set_descriptorset(VkDevice device, uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkImageLayout imageLayout, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		std::vector<VkDescriptorImageInfo> desc_images(descriptorsCount);
		uint32_t i = 0;
		for (; i < imageViews.size(); ++i)
		{
			desc_images[i].sampler = pSampler;
			desc_images[i].imageView = imageViews[i];
			desc_images[i].imageLayout = imageLayout;
		}
		// we should still assign the remaining descriptors
		// Using the VK_EXT_robustness2 extension, it is possible to assign a NULL one
		for (; i < descriptorsCount; ++i)
		{
			desc_images[i].sampler = pSampler;
			desc_images[i].imageView = VK_NULL_HANDLE;
			desc_images[i].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}

		VkWriteDescriptorSet write;
		write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = NULL;
		write.dstSet = descriptorSet;
		write.descriptorCount = descriptorsCount;
		write.descriptorType = pSampler ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.pImageInfo = desc_images.data();
		write.dstBinding = index;
		write.dstArrayElement = 0;

		updatedsets(device, 1, &write, 0, NULL);
	}

	void set_descriptorset(VkDevice device, uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		set_descriptorset(device, index, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
	}

	void set_descriptorset(VkDevice device, uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		set_descriptorset(device, index, descriptorsCount, imageViews, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
	}

	void set_descriptorset_depth(VkDevice device, uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet)
	{
		set_descriptorset(device, index, imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
	}
#endif
	// end set

}
//!vkc