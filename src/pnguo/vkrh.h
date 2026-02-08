#pragma once
/*

vk渲染器
Copyright (c) 华仔
188665600@qq.com

创建日期：2026-02-08


*/

namespace vkr {
#ifndef DEV_INFO_CX
#define DEV_INFO_CX
	struct dev_info_cx
	{
		void* inst = 0;
		void* phy = 0;
		void* vkdev = 0;
		uint32_t qFamIdx = 0;		// familyIndex
		uint32_t qIndex = 0;
	};
#endif
#ifndef DEVICE_INFO_T
#define DEVICE_INFO_T
	struct device_info_t
	{
		char name[256];
		void* phd;
	};
#endif
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
#ifndef HASH_SEED
#define HASH_SEED 2166136261
#endif
	class DefineList : public std::map<const std::string, std::string>
	{
	public:
		bool Has(const std::string& str) const;
		size_t Hash(size_t result = HASH_SEED) const;
		friend DefineList operator+(DefineList def1, const DefineList& def2);
		std::string to_string() const;
	};
}
//!vkc
