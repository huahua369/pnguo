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


struct spine_ctx {
	void* renderer = nullptr;								// 渲染器指针
	draw_geometry_fun RenderGeometryRaw = nullptr;			// 渲染函数指针
	SetTextureBlendMode_fun SetTextureBlendMode = nullptr;	// 设置纹理混合模式函数指针，可选
	newTexture_fun CreateTexture = nullptr;					// 创建纹理函数指针	
	UpdateTexture_fun UpdateTexture = nullptr;				// 更新纹理函数指针
	DestroyTexture_fun DestroyTexture = nullptr;			// 销毁纹理函数指针
	size_t rc_count = 0;									// 创建资源计数
};

typedef struct spine_atlas_t spine_atlas_t;
typedef struct spSkeletonDrawable spine_drawable_t;
// 创建上下文
spine_ctx* sp_ctx_create(void* renderer, draw_geometry_fun rendergeometryraw, newTexture_fun createtexture, UpdateTexture_fun updatetexture, DestroyTexture_fun destroytexture, SetTextureBlendMode_fun settextureblendmode);
// 销毁上下文
void sp_ctx_dispose(spine_ctx* ctx);
// 创建图集,支持打包格式
spine_atlas_t* sp_new_atlas(spine_ctx* ctx, const char* atlasf, size_t fdsize = 0);
// 销毁图集
void sp_atlas_dispose(spine_atlas_t* atlas);
// 打包图集和纹理到内存数据
void sp_atlas_packages(spine_atlas_t* atlas, std::vector<char>* opt);
// 创建可绘制对象,json\skel
spine_drawable_t* sp_new_drawable(spine_ctx* ctx, spine_atlas_t* atlas, const char* skef, size_t fdsize = 0, float scale = 1.0f, float defaultMix = 0.2f);
void sp_drawable_dispose(spine_drawable_t* drawable);
void sp_drawable_set_pos(spine_drawable_t* drawable, int x, int y);
void sp_drawable_get_anim_names(spine_drawable_t* drawable, std::vector<char*>* v);
void sp_drawable_set_animationbyname(spine_drawable_t* drawable, int trackIndex, const char* animationName, int loop);
void sp_drawable_add_animationbyname(spine_drawable_t* drawable, int trackIndex, const char* animationName, int loop, float delay);
// 更新动画数据
void sp_drawable_update(spine_drawable_t* drawable, double deltaTime);
// 绘制
void sp_drawable_draw(spine_drawable_t* drawable);

#endif // !SPSDL3_H