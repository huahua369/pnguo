#pragma once
/*
	vk渲染器
	Copyright (c) 华仔
	188665600@qq.com

	创建日期：2024-07-15
*/


// 渲染器配置
// 资源管理：加载、卸载
// 渲染指令

struct dev_info_cx;
struct image_vkr
{
	glm::ivec2 size = {};
	void* vkimageptr = 0;
};

struct mouse_state_t;
class vkdg_cx
{
public:
	void* ctx = 0;
	void* dev = 0;
	void* qupload = 0;
	std::vector<uint32_t> dt;
	int width = 0, height = 0;
public:
	vkdg_cx();
	~vkdg_cx();
	void update(mouse_state_t* io);
	void on_render();
	image_vkr get_vkimage(int idx);
	// 重置大小
	void resize(int w, int h);
	void save_fbo(int idx);
	void copy2(int idx, void* vkptr);
	void* new_pipe(const char* vertexShader, const char* pixelShader);
private:

};

// 节点包围盒渲染
class BBoxPass
{
public:
	struct mat_idx
	{
		glm::mat4* m = 0;
		size_t pos = 0, count = 0;
	};
	struct box_t
	{
		glm::vec4 vCenter, vRadius;
	};
	std::vector<mat_idx> box_ms;	// 节点矩阵，box开始\数量
	std::vector<box_t> boxs;		// box坐标半径
public:
	BBoxPass();
	~BBoxPass();
	// 收集所有节点包围盒
	void add(glm::mat4* m, box_t* vcr, size_t count);
	void clear();
private:

};


// 创建渲染器
vkdg_cx* new_vkdg(dev_info_cx* c = 0);
void free_vkdg(vkdg_cx* p);
void load_gltf(vkdg_cx* p, const char* fn);
