/*
	2d渲染器
	{
		"atlas":[数据索引],
		"ske":[数据索引],
		"drawable":[{"atlas","ske",scale,mix}],
		"data_view":[{"offset","length","type可选0二进制1字符串"}]
	}
*/
#ifndef SPSDL3_H
#define SPSDL3_H

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#ifdef __cplusplus
extern "C" {
#endif
	typedef struct SDL_Renderer SDL_Renderer;
	typedef struct SP_Texture SP_Texture;

	struct SP_vec2
	{
		float x;
		float y;
	};
	struct SP_color
	{
		float r;
		float g;
		float b;
		float a;
	};
	struct SP_Vertex
	{
		SP_vec2 position;        /**< Vertex position, in SDL_Renderer coordinates  */
		SP_color color;           /**< Vertex color */
		SP_vec2 tex_coord;       /**< Normalized texture coordinates, if needed */
	};
	// 渲染函数指针
	typedef bool (*draw_geometry_fun)(void* renderer, SP_Texture* texture, const float* xy, int xy_stride
		, const SP_color* color, int color_stride, const float* uv, int uv_stride
		, int num_vertices, const void* indices, int num_indices, int size_indices);
	// 设置纹理混合模式函数指针
	typedef bool (*SetTextureBlendMode_fun)(SP_Texture* texture, uint32_t blendMode);
	// 创建纹理函数指针, rgba8格式
	typedef SP_Texture* (*newTexture_fun)(void* renderer, int w, int h);
	typedef bool (*UpdateTexture_fun)(SP_Texture* texture, const void* pixels, int pitch);
	typedef void (*DestroyTexture_fun)(SP_Texture* texture);
#ifdef __cplusplus
}
#endif

class sp_drawable_ctx
{
public:
	void* renderer = nullptr;
	draw_geometry_fun RenderGeometryRaw = nullptr;
	SetTextureBlendMode_fun SetTextureBlendMode = nullptr;
	newTexture_fun CreateTexture = nullptr;
	UpdateTexture_fun UpdateTexture = nullptr;
	DestroyTexture_fun DestroyTexture = nullptr;
	struct sp_obj_c {
		void* drawable = 0;
		void* atlas = 0;
		bool visible = true;
	};
	std::vector<sp_obj_c> drawables;
	struct able_t
	{
		std::string atlas;
		std::string ske;
		float scale = 0.5f;
		float defaultMix = 0.2f;
	};
	std::vector<able_t> vable;
public:
	sp_drawable_ctx();
	~sp_drawable_ctx();
	void set_renderer(void* p, draw_geometry_fun rendergeometryraw, newTexture_fun createtexture, UpdateTexture_fun updatetexture, DestroyTexture_fun destroytexture, SetTextureBlendMode_fun settextureblendmode);
	// package_file可用于输出到打包
	void add(const std::string& atlas, const std::string& ske, float scale = 0.5f, float defaultMix = 0.2f, const std::string& package_file = "");
	// 从打包文件加载
	void add_pkg_data(const char* data, size_t len, float scale = 0.5f, float defaultMix = 0.2f);
	void add_pkg(const std::string& pkgfn, float scale = 0.5f, float defaultMix = 0.2f);
	void dispose_sp(size_t idx);
	void animationstate_set_animationbyname(size_t idx, int trackIndex, const char* animationName, int loop);
	void animationstate_add_animationbyname(size_t idx, int trackIndex, const char* animationName, int loop, float delay);
	void set_pos(size_t idx, int x, int y);
	void get_anim_name(size_t idx, std::vector<char*>* v);
	void update_draw(double deltaTime);

private:
	void draw(void* self);

};

class drawable2d_cx
{
public:
	struct obj_c {
		void* drawable = 0;
		void* skeletonData = 0;
		void* animationStateData = 0;
		bool visible = true;
	};
	void* atlas = 0;
	void* renderer = 0;
	std::vector<obj_c> vable;
public:
	drawable2d_cx();
	~drawable2d_cx();

private:

};



#endif // !SPSDL3_H