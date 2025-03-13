#ifndef EDITOR_2D
#define EDITOR_2D
// 2d编辑器
// 创建时间2025-3-8

struct subimage_t
{
	std::string name;
	int index = -1;
	int rotate = false;			// true是90度
	glm::ivec4 bounds = {};// 519, 223, 17, 38
	glm::ivec4 offsets = {};//2, 2, 21, 42
	//glm::ivec4 split = {};//废弃
	//glm::ivec4 pad = {};//废弃
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

#endif // !EDITOR_2D
