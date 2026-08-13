#pragma once
#include <cstdint>
 
struct vg_fbo_t
{
	uint32_t width, height;
	void* img;
	void* imgMS;
	void* depthStencil;
};

struct ovg_device_t;
struct ovg_ctx_t;
ovg_device_t* new_vkdevctx(VkDevice vkdev, VkPhysicalDevice phy, VkInstance instance, uint32_t qFamIdx);
void free_vkdevctx(ovg_device_t* dev);
ovg_ctx_t* new_ovgctx(ovg_device_t* dev, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlags samples);
void free_ovgctx(ovg_ctx_t* p);
vg_fbo_t new_vgfbo(ovg_ctx_t* p, int width, int height);
void free_vgfbo(vg_fbo_t*);
