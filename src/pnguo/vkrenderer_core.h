#pragma once
/*

vk渲染器
Copyright (c) 华仔
188665600@qq.com

创建日期：205-12-05


vk渲染流程:
	0.创建实例，枚举设备，选择物理设备，创建逻辑设备
	1.创建渲染目标：窗口交换链或纹理、fbo
	2.创建管线、状态等
	3.分配vbo/ibo/纹理/ubo等资源
	4.更新ubo、纹理、vbo等资源
	5.渲染命令
	6.提交列队
*/

namespace vkc {
	struct dev_info_cx
	{
		void* inst;
		void* phy;
		void* vkdev;
		uint32_t qFamIdx;		// familyIndex
		uint32_t qIndex = 0;
	};
	struct device_info_t
	{
		char name[256];
		void* phd;
	};
	struct layout_set_pool
	{
		VkDescriptorPool descriptorPool = {};
		VkDescriptorSetLayout descSetLayout = {};
		uint32_t max_count = 0;
		uint32_t alloc_count = 0;
		VkDevice dev = {};
#ifdef _HAS_VK_MULTITHREADING
		std::mutex _mutex;
#endif // _HAS_VK_MULTITHREADING

	};
	// 实例、设备
	class Device;

	// 纹理

	// 渲染目标


	// 着色器管线创建

	// 创建描述符堆
	bool new_set_layout(VkDevice pDevice, VkDescriptorSetLayoutBinding* layout_binding, uint32_t count, uint32_t maxcount, layout_set_pool* opt);
	void free_layout_set_pool(layout_set_pool* p);
	// 分配描述符
	VkDescriptorSet alloc_descriptor(layout_set_pool* p);
	void free_descriptor(layout_set_pool* p, VkDescriptorSet descriptorSet);
	// 创建描述符堆并分配1个描述符
	VkDescriptorSet alloc_descriptor_image(VkDevice pDevice, int size, const VkSampler* pSamplers, layout_set_pool* opt, uint32_t maxcount);
	VkDescriptorSet alloc_descriptor_images(VkDevice pDevice, std::vector<uint32_t>& descriptorCounts, const VkSampler* pSamplers, layout_set_pool* opt, uint32_t maxcount);
	// 更新描述符
	void set_descriptorset(VkDevice device, VkBuffer buffer, int index, uint32_t size, VkDescriptorSet descriptorSet, VkDescriptorType dt);
	void set_descriptorset1(VkDevice device, VkBuffer buffer, int index, uint32_t pos, uint32_t size, VkDescriptorSet descriptorSet, VkDescriptorType dt);
	void set_descriptorset(VkDevice device, uint32_t index, VkImageView imageView, VkImageLayout imageLayout, VkSampler pSampler, VkDescriptorSet descriptorSet);
	void set_descriptorset(VkDevice device, uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkImageLayout imageLayout, VkSampler pSampler, VkDescriptorSet descriptorSet);
	void set_descriptorset(VkDevice device, uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet);
	void set_descriptorset(VkDevice device, uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkSampler pSampler, VkDescriptorSet descriptorSet);
	void set_descriptorset_depth(VkDevice device, uint32_t index, VkImageView imageView, VkSampler pSampler, VkDescriptorSet descriptorSet);

	// 缓冲区

	// 列队

}
//!vkc