#ifndef EDITOR_2D
#define EDITOR_2D
// 2d编辑器
// 创建时间2025-3-8

#ifndef TEX_CB
#define TEX_CB
struct texture_cb
{
	// 创建纹理
	void* (*new_texture)(void* renderer, int width, int height, int type, void* data, int stride, int bm, bool static_tex, bool multiply);
	// 更新纹理数据
	void (*update_texture)(void* texture, const glm::ivec4* rect, const void* pixels, int pitch);
	// 设置纹理混合模式
	void (*set_texture_blend)(void* texture, uint32_t b, bool multiply);
	// 删除纹理
	void (*free_texture)(void* texture);
	// 创建或更新纹理
	void* (*make_tex)(void* renderer, image_ptr_t* img);
	// 从图片文件创建纹理
	void* (*new_texture_file)(void* renderer, const char* fn);
	// 纹理渲染
	// 批量区域渲染
	int (*render_texture)(void* renderer, void* texture, texture_dt* p, int count);
	// 单个区域支持旋转
	bool (*render_texture_rotated)(void* renderer, void* texture, texture_angle_dt* p, int count);
	// 平铺渲染
	bool (*render_texture_tiled)(void* renderer, void* texture, texture_tiled_dt* p, int count);
	// 九宫格渲染
	bool (*render_texture_9grid)(void* renderer, void* texture, texture_9grid_dt* p, int count);
	// 渲染2d三角网,支持顶点色、纹理
	bool (*render_geometryraw)(void* renderer, void* texture, geometryraw_dt* p, int count);
};
#else
typedef struct texture_cb texture_cb;
#endif

struct atlas_xt;
struct AtlasPage {
	atlas_xt* atlas;
	std::string name;
	std::string path;
	int format;
	int minFilter, magFilter;
	int uWrap, vWrap;

	void* rendererObject;
	int width, height;
	int /*boolean*/ pma;

	AtlasPage* next;
};

struct subimage_t
{
	std::string name;
	int index = -1;
	int degrees = 0;
	glm::ivec4 bounds = {};// 519, 223, 17, 38，图片切片大小
	glm::ivec4 offsets = {};//2, 2, 21, 42，  图片偏移量和原始大小
	//glm::ivec4 split = {};//废弃
	//glm::ivec4 pad = {};//废弃
	glm::vec2 uv, uv2;
	njson keyValues;
};
struct page_obj_t
{
	void* renderer = 0;
	std::map<void*, std::string> _texs;
	njson0 img;
	char* data;
	int len;
	texture_cb* cb = 0;
};
// 图集信息
struct atlas_xt
{
	std::string name;//: 首行为该页中的图像名称.图片位置由atlas加载器来查找, 通常是再atlas文件的同目录下.
	glm::ivec2 size = {};//: 页中图像的宽度和高度.在加载图像之前告知atlas加载器是非常有必要的, 例如, 可以提前为其分配缓冲区.若省略则默认为0, 0.
	int format = 7;//: atlas加载器在内存中存储图像时应使用的格式.Atlas加载器可忽略该属性, 其可用值为 : Alpha、Intensity、LuminanceAlpha、RGB565、RGBA4444、RGB888或RGBA8888.若省略则默认为RGBA8888.
	glm::ivec2 filter = { 2,2 };//: Texture过滤器的缩略和放大方式.Atlas加载器可忽略该属性.其可用值为 : Nearest, Linear, MipMap, MipMapNearestNearest, MipMapLinearNearest, MipMapNearestLinear, 或MipMapLinearLinear.若省略则默认为Nearest.
	glm::ivec2 repeat = {}; //: Texture包裹设置.Atlas加载器可忽略该属性.其可用值为 : x, y, xy, 或 none.若省略则默认为none.
	bool pma = false;//: 若值为true则表示图像使用了premultiplied alpha.若省略则默认为false.
	std::vector<subimage_t> region;
	page_obj_t* rendererObject = 0;
	AtlasPage* pages;
};
struct atlas_strinfo
{
	const char** formatNames;
	const char** textureFilterNames;
};


// 获取图集格式信息
atlas_strinfo get_atlas_strinfo();
// 图集导出spine格式
void generate_atlas(const char* output_path, atlas_xt* atlas);
// 从json数据创建图集
atlas_xt* json2atlas(njson0& n);
// 从图集创建json数据
void atlas2json(atlas_xt* a, njson0& n);
// 释放图集
void free_atlas(atlas_xt* atlas);

subimage_t* atlas_find(atlas_xt* atlas, const std::string& k);
size_t atlas_findi(atlas_xt* atlas, const std::string& k);

atlas_xt* new_atlas(const char* path, page_obj_t* rendererObject);

// 2d编辑器类
class editor2d_cx
{
private:
	struct texture_t
	{
		uint32_t format;
		int w;
		int h;

		int refcount;
	};
	std::string configdir;
	std::string config_name;

	void* renderer = 0;			// 渲染器绑定
	// 设置纹理创建
	texture_cb tex_cb = {};
public:
	editor2d_cx();
	~editor2d_cx();
	void init(void* renderer, const texture_cb& cb);
	// 设置自动保存目录等配置
	void set_config(const char* fn);
	// 导入图片，支持stb能导入的格式
	bool import_image(const char* fn);
	// 导出图集
	bool export_atlas(const char* dir, const char* name);
	// 保存
	bool save(const char* fn);
	// 更新动画之类 
	void update(double deltaTime);
	// 渲染
	void draw();
private:

};


#endif // !EDITOR_2D
