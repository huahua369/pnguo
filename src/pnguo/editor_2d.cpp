/*
 2d编辑器
 创建时间2025-3-8

 todo 图集编辑器
 一、核心功能模块
1.画布与坐标系统
	实现多层级画布结构，支持无限放大/缩小（比例建议限制在10%-1000%）
	采用双坐标系：屏幕坐标系（像素定位）与逻辑坐标系（保持元素相对位置）
	集成标尺和辅助线系统，支持智能吸附对齐
2.资源管理模块
	支持PNG/JPG/SVG等格式导入，自动解析元数据（如透明通道、尺寸信息）
	智能排列算法（如矩形装箱算法、Guillotine算法）优化图集空间利用率
	实时生成图集预览与配套描述文件（JSON/XML格式）
3.图层与元素操作
	分层管理图集元素，支持锁定/隐藏/排序操作1
	提供九宫格编辑模式，特殊处理UI元素的拉伸需求
	元素属性面板（坐标定位、旋转角度、锚点设置）
二、关键技术实现
1.渲染引擎选择
	优先采用Canvas技术栈（Fabric.js/Konva.js ）处理复杂图形操作
	集成WebGL加速模块处理超大尺寸图集（4096x4096+）
2.历史记录系统
	命令模式实现操作堆栈，支持50+步撤销/重做
	增量存储机制优化内存占用，关键操作自动创建还原点
3.自动化处理流程
	智能空白填充算法（基于边缘检测）
	批量重命名与路径映射功能
	导出预设
三、交互设计要点
1.可视化操作
	拖拽式资源导入（支持文件夹批量拖入）
	快捷键体系设计（空格键平移、Ctrl+滚轮缩放）
	实时间距显示与碰撞预警
2.性能优化策略
	虚拟化渲染技术处理超多元素场景
	后台计算任务（如自动排列算法）
	分级LOD机制（缩放时动态切换渲染精度）
四、扩展能力设计
1.插件系统架构
	开放格式转换插件接口（支持Photoshop脚本导入）
	算法扩展点（可替换不同装箱算法）
2.协作功能
	操作历史云端同步
	多用户光标追踪与批注系统
五、测试验证重点
1.边界用例验证
	透明元素边缘融合测试
	极端尺寸混合排列测试（如1024x1024与16x16元素共存）

*/
#include "pch1.h"
#include "editor_2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mapView.h>
#include <stb_image.h>
#include "tria.h"
// https://zh.esotericsoftware.com/spine-atlas-format
atlas_strinfo get_atlas_strinfo()
{
	static const char* formatNames[] = { "", "Alpha", "Intensity", "LuminanceAlpha", "RGB565", "RGBA4444", "RGB888", "RGBA8888" };
	static const char* textureFilterNames[] = { "", "Nearest", "Linear", "MipMap", "MipMapNearestNearest", "MipMapLinearNearest", "MipMapNearestLinear", "MipMapLinearLinear" };
	return { formatNames, textureFilterNames };
}
void generate_atlas(const char* output_path, atlas_xt* atlas)
{
	if (!atlas || !output_path)
	{
		return;
	}
	auto [formatNames, textureFilterNames] = get_atlas_strinfo();
	FILE* fp = fopen(output_path, "w");
	if (!fp) {
		perror("Failed to create atlas file");
		exit(EXIT_FAILURE);
	}

	// 写入文件头
	fprintf(fp, "%s\n", atlas->name.c_str());
	fprintf(fp, "\tsize: %d, %d\n", atlas->size.x, atlas->size.y);
	fprintf(fp, "\tformat: %s\n", formatNames[atlas->format]);
	fprintf(fp, "\tfilter: %s,%s\n", textureFilterNames[atlas->filter.x], textureFilterNames[atlas->filter.y]);
	fprintf(fp, "\trepeat: none\n");// todo trepeat
	fprintf(fp, "\tpma: %s\n\n", atlas->pma ? "true" : "false");

	// 写入每个子图信息 
	for (int i = 0; i < atlas->region.size(); i++) {
		auto& it = atlas->region[i];
		fprintf(fp, "%s\n", it.name.c_str());
		if (it.degrees > 0)
			fprintf(fp, "\trotate: %d\n", it.degrees);
		fprintf(fp, "\tbounds: %d, %d, %d, %d\n", it.bounds.x, it.bounds.y, it.bounds.z, it.bounds.w);
		fprintf(fp, "\toffsets: %d, %d, %d, %d\n", it.offsets.x, it.offsets.y, it.offsets.z, it.offsets.w);
		fprintf(fp, "\tindex: %d\n\n", it.index);
	}

	fclose(fp);
}
atlas_xt* json2atlas(njson0& n)
{
	std::string name = n["name"].get<std::string>();
	auto ss = gp::get_iv2(n, "size");
	auto filter2 = gp::get_iv2(n, "filter");
	auto repeat2 = gp::get_iv2(n, "repeat");
	glm::ivec2 size = ss.size() ? ss[0] : glm::ivec2();
	int format = gp::toInt(n["format"]);
	glm::ivec2 filter = filter2.size() ? filter2[0] : glm::ivec2();
	glm::ivec2 repeat = repeat2.size() ? repeat2[0] : glm::ivec2();
	bool pma = gp::toInt(n["pma"]) == 0 ? false : true;

	auto ap = new atlas_xt{ name, size, format, filter, repeat, pma };
	auto region2 = n["region"];
	int region_count = region2.size(); ap->region.resize(region_count);
	subimage_t* region = ap->region.data();
	if (region2.is_array() && region_count > 0)
	{
		auto pr = region;
		for (auto& it : region2.items()) {
			auto& nt = it.value();
			auto name = nt["name"].get<std::string>();
			int index = gp::toInt(nt["index"]);
			int rotate = gp::toInt(nt["rotate"]);
			auto bounds = gp::get_iv4(nt, "bounds");
			auto offsets = gp::get_iv4(nt, "offsets");
			*pr = { name, index, rotate, bounds[0], offsets[0] };
			pr++;
		}
	}
	return ap;
}

void atlas2json(atlas_xt* a, njson0& n)
{
	if (!a)
	{
		return;
	}
	n["name"] = a->name;
	n["size"] = { a->size.x, a->size.y };
	n["format"] = a->format;
	n["filter"] = { a->filter.x ,  a->filter.y };
	n["repeat"] = "none";
	n["pma"] = a->pma;
	auto region2 = njson0::array();
	for (int i = 0; i < a->region.size(); i++) {
		auto& it = a->region[i];
		njson0 nt;
		nt["name"] = it.name;
		nt["index"] = it.index;
		nt["rotate"] = it.degrees;
		nt["bounds"] = { it.bounds.x, it.bounds.y, it.bounds.z, it.bounds.w };
		nt["offsets"] = { it.offsets.x, it.offsets.y, it.offsets.z, it.offsets.w };
		region2.push_back(nt);
	}
	n["region"] = region2;
}
namespace e2d {

	typedef struct SimpleString {
		char* start;
		char* end;
		int length;
	} SimpleString;

	static SimpleString* ss_trim(SimpleString* self) {
		while (isspace((unsigned char)*self->start) && self->start < self->end)
			self->start++;
		if (self->start == self->end) {
			self->length = self->end - self->start;
			return self;
		}
		self->end--;
		while (((unsigned char)*self->end == '\r') && self->end >= self->start)
			self->end--;
		self->end++;
		self->length = self->end - self->start;
		return self;
	}

	static int ss_indexOf(SimpleString* self, char needle) {
		char* c = self->start;
		while (c < self->end) {
			if (*c == needle) return c - self->start;
			c++;
		}
		return -1;
	}

	static int ss_indexOf2(SimpleString* self, char needle, int at) {
		char* c = self->start + at;
		while (c < self->end) {
			if (*c == needle) return c - self->start;
			c++;
		}
		return -1;
	}

	static SimpleString ss_substr(SimpleString* self, int s, int e) {
		SimpleString result;
		e = s + e;
		result.start = self->start + s;
		result.end = self->start + e;
		result.length = e - s;
		return result;
	}

	static SimpleString ss_substr2(SimpleString* self, int s) {
		SimpleString result;
		result.start = self->start + s;
		result.end = self->end;
		result.length = result.end - result.start;
		return result;
	}

	static int /*boolean*/ ss_equals(SimpleString* self, const char* str) {
		int i;
		int otherLen = strlen(str);
		if (self->length != otherLen) return 0;
		for (i = 0; i < self->length; i++) {
			if (self->start[i] != str[i]) return 0;
		}
		return -1;
	}

	static std::string ss_copy(SimpleString* self) {
		std::string str;
		str.assign(self->start, self->length);
		return str;
	}

	static int ss_toInt(SimpleString* self) {
		return (int)strtol(self->start, &self->end, 10);
	}

	typedef struct AtlasInput {
		const char* start;
		const char* end;
		char* index;
		int length;
		SimpleString line;
	} AtlasInput;

	static SimpleString* ai_readLine(AtlasInput* self) {
		if (self->index >= self->end) return 0;
		self->line.start = self->index;
		while (self->index < self->end && *self->index != '\n')
			self->index++;
		self->line.end = self->index;
		if (self->index != self->end) self->index++;
		self->line = *ss_trim(&self->line);
		self->line.length = self->line.end - self->line.start;
		return &self->line;
	}

	static int ai_readEntry(SimpleString entry[5], SimpleString* line) {
		int colon, i, lastMatch;
		SimpleString substr;
		if (line == NULL) return 0;
		ss_trim(line);
		if (line->length == 0) return 0;

		colon = ss_indexOf(line, ':');
		if (colon == -1) return 0;
		substr = ss_substr(line, 0, colon);
		entry[0] = *ss_trim(&substr);
		for (i = 1, lastMatch = colon + 1;; i++) {
			int comma = ss_indexOf2(line, ',', lastMatch);
			if (comma == -1) {
				substr = ss_substr2(line, lastMatch);
				entry[i] = *ss_trim(&substr);
				return i;
			}
			substr = ss_substr(line, lastMatch, comma - lastMatch);
			entry[i] = *ss_trim(&substr);
			lastMatch = comma + 1;
			if (i == 4) return 4;
		}
	}

	static const char* formatNames[] = { "", "Alpha", "Intensity", "LuminanceAlpha", "RGB565", "RGBA4444", "RGB888",
										"RGBA8888" };
	static const char* textureFilterNames[] = { "", "Nearest", "Linear", "MipMap", "MipMapNearestNearest",
											   "MipMapLinearNearest",
											   "MipMapNearestLinear", "MipMapLinearLinear" };

	int indexOf(const char** array, int count, SimpleString* str) {
		int i;
		for (i = 0; i < count; i++)
			if (ss_equals(str, array[i])) return i;
		return 0;
	}
	typedef enum {
		SP_ATLAS_UNKNOWN_FORMAT,
		SP_ATLAS_ALPHA,
		SP_ATLAS_INTENSITY,
		SP_ATLAS_LUMINANCE_ALPHA,
		SP_ATLAS_RGB565,
		SP_ATLAS_RGBA4444,
		SP_ATLAS_RGB888,
		SP_ATLAS_RGBA8888
	} AtlasFormat;

	typedef enum {
		SP_ATLAS_UNKNOWN_FILTER,
		SP_ATLAS_NEAREST,
		SP_ATLAS_LINEAR,
		SP_ATLAS_MIPMAP,
		SP_ATLAS_MIPMAP_NEAREST_NEAREST,
		SP_ATLAS_MIPMAP_LINEAR_NEAREST,
		SP_ATLAS_MIPMAP_NEAREST_LINEAR,
		SP_ATLAS_MIPMAP_LINEAR_LINEAR
	} AtlasFilter;

	typedef enum {
		SP_ATLAS_MIRROREDREPEAT,
		SP_ATLAS_CLAMPTOEDGE,
		SP_ATLAS_REPEAT
	} spAtlasWrap;
	AtlasPage* AtlasPage_create(atlas_xt* atlas, const char* name) {
		AtlasPage* self = new AtlasPage();
		self->atlas = atlas;
		self->name = name;
		self->minFilter = SP_ATLAS_NEAREST;
		self->magFilter = SP_ATLAS_NEAREST;
		self->format = SP_ATLAS_RGBA8888;
		self->uWrap = SP_ATLAS_CLAMPTOEDGE;
		self->vWrap = SP_ATLAS_CLAMPTOEDGE;
		return self;
	}
	struct page_obj_t
	{
		void* renderer = 0;
		std::map<void*, std::string> _texs;
		njson0 img;
		char* data;
		int len;
	};

	void AtlasPage_createTexture(AtlasPage* self, const char* path)
	{
#if 0
		int width, height, components;
		auto p = (page_obj_t*)self->atlas->rendererObject;
		int len = 0;
		uint8_t* data = 0;
		if (p->img.size() && p->data)
		{
			for (auto& it : p->img)
			{
				std::string name = it["name"];
				if (path == name)
				{
					len = it["length"];
					int ps = it["offset"];
					data = (uint8_t*)p->data + ps;
					break;
				}
			}
		}

		stbi_uc* imageData = data && len > 0 ? stbi_load_from_memory(data, len, &width, &height, &components, 4)
			: stbi_load(path, &width, &height, &components, 4);
		if (!imageData) return;

		SDL_Texture* texture = SDL_CreateTexture((SDL_Renderer*)p->renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, width, height);
		if (!texture) {
			stbi_image_free(imageData);
			return;
		}
		if (!SDL_UpdateTexture(texture, NULL, imageData, width * 4)) {
			stbi_image_free(imageData);
			return;
		}

		p->_texs[texture] = path;
		stbi_image_free(imageData);
		self->rendererObject = texture;
#endif
		return;
	}
	atlas_xt* atlas_create(const char* begin, int length, const char* dir, void* rendererObject) {
		atlas_xt* self;
		AtlasInput reader;
		SimpleString* line;
		SimpleString entry[5];

		AtlasPage* page = NULL;
		AtlasPage* lastPage = NULL;
		subimage_t* lastRegion = NULL;

		int count;
		int dirLength = (int)strlen(dir);
		int needsSlash = dirLength > 0 && dir[dirLength - 1] != '/' && dir[dirLength - 1] != '\\';

		self = new atlas_xt();
		self->rendererObject = rendererObject;

		reader.start = begin;
		reader.end = begin + length;
		reader.index = (char*)begin;
		reader.length = length;

		line = ai_readLine(&reader);
		while (line != NULL && line->length == 0)
			line = ai_readLine(&reader);

		while (-1) {
			if (line == NULL || line->length == 0) break;
			if (ai_readEntry(entry, line) == 0) break;
			line = ai_readLine(&reader);
		}

		while (-1) {
			if (line == NULL) break;
			if (ss_trim(line)->length == 0) {
				page = NULL;
				line = ai_readLine(&reader);
			}
			else if (page == NULL) {
				std::string name = ss_copy(line);
				std::string path = dir;
				if (needsSlash) path[dirLength] = '/';
				path += name;
				page = AtlasPage_create(self, name.c_str());

				if (lastPage)
					lastPage->next = page;
				else
					self->pages = page;
				lastPage = page;

				while (-1) {
					line = ai_readLine(&reader);
					if (ai_readEntry(entry, line) == 0) break;
					if (ss_equals(&entry[0], "size")) {
						page->width = ss_toInt(&entry[1]);
						page->height = ss_toInt(&entry[2]);
					}
					else if (ss_equals(&entry[0], "format")) {
						page->format = (AtlasFormat)indexOf(formatNames, 8, &entry[1]);
					}
					else if (ss_equals(&entry[0], "filter")) {
						page->minFilter = (AtlasFilter)indexOf(textureFilterNames, 8, &entry[1]);
						page->magFilter = (AtlasFilter)indexOf(textureFilterNames, 8, &entry[2]);
					}
					else if (ss_equals(&entry[0], "repeat")) {
						page->uWrap = SP_ATLAS_CLAMPTOEDGE;
						page->vWrap = SP_ATLAS_CLAMPTOEDGE;
						if (ss_indexOf(&entry[1], 'x') != -1) page->uWrap = SP_ATLAS_REPEAT;
						if (ss_indexOf(&entry[1], 'y') != -1) page->vWrap = SP_ATLAS_REPEAT;
					}
					else if (ss_equals(&entry[0], "pma")) {
						page->pma = ss_equals(&entry[1], "true");
					}
				}
				page->path = path;
				//AtlasPage_createTexture(page, path.c_str());
			}
			else {
				subimage_t region1 = {};
				subimage_t* region = &region1;
				lastRegion = region;
				region->name = ss_copy(line);
				while (-1) {
					line = ai_readLine(&reader);
					count = ai_readEntry(entry, line);
					if (count == 0) break;
					if (ss_equals(&entry[0], "xy")) {
						region->bounds.x = ss_toInt(&entry[1]);
						region->bounds.y = ss_toInt(&entry[2]);
					}
					else if (ss_equals(&entry[0], "size")) {
						region->bounds.z = ss_toInt(&entry[1]);
						region->bounds.w = ss_toInt(&entry[2]);
					}
					else if (ss_equals(&entry[0], "bounds")) {
						region->bounds.x = ss_toInt(&entry[1]);
						region->bounds.y = ss_toInt(&entry[2]);
						region->bounds.z = ss_toInt(&entry[3]);
						region->bounds.w = ss_toInt(&entry[4]);
					}
					else if (ss_equals(&entry[0], "offset")) {
						region->offsets.x = ss_toInt(&entry[1]);
						region->offsets.y = ss_toInt(&entry[2]);
					}
					else if (ss_equals(&entry[0], "orig")) {
						region->offsets.z = ss_toInt(&entry[1]);
						region->offsets.w = ss_toInt(&entry[2]);
					}
					else if (ss_equals(&entry[0], "offsets")) {
						region->offsets.x = ss_toInt(&entry[1]);
						region->offsets.y = ss_toInt(&entry[2]);
						region->offsets.z = ss_toInt(&entry[3]);
						region->offsets.w = ss_toInt(&entry[4]);
					}
					else if (ss_equals(&entry[0], "rotate")) {
						if (ss_equals(&entry[1], "true")) {
							region->degrees = 90;
						}
						else if (!ss_equals(&entry[1], "false")) {
							region->degrees = ss_toInt(&entry[1]);
						}
					}
					else if (ss_equals(&entry[0], "index")) {
						region->index = ss_toInt(&entry[1]);
					}
					else {
						int i = 0;
						njson keyValue;
						auto& v = keyValue[ss_copy(&entry[0])];
						for (i = 0; i < count; i++) {
							v.push_back(ss_toInt(&entry[i + 1]));
						}
						region->keyValues.push_back(keyValue);
					}
				}
				if (region->offsets.z == 0 && region->offsets.w == 0) {
					region->offsets.z = region->bounds.z;
					region->offsets.w = region->bounds.w;
				}

				region->uv.x = (float)region->bounds.x / page->width;
				region->uv.y = (float)region->bounds.y / page->height;
				if (region->degrees == 90) {
					region->uv2.x = (float)(region->bounds.x + region->bounds.w) / page->width;
					region->uv2.y = (float)(region->bounds.y + region->bounds.z) / page->height;
				}
				else {
					region->uv2.x = (float)(region->bounds.x + region->bounds.z) / page->width;
					region->uv2.y = (float)(region->bounds.y + region->bounds.w) / page->height;
				}
			}
		}

		return self;
	}

}
atlas_xt* spAtlas_createFromFile(const char* path, void* rendererObject) {
	int dirLength;
	char* dir;
	int length;
	const char* data;

	atlas_xt* atlas = 0;

	/* Get directory from atlas path. */
	const char* lastForwardSlash = strrchr(path, '/');
	const char* lastBackwardSlash = strrchr(path, '\\');
	const char* lastSlash = lastForwardSlash > lastBackwardSlash ? lastForwardSlash : lastBackwardSlash;
	if (lastSlash == path) lastSlash++; /* Never drop starting slash. */
	dirLength = (int)(lastSlash ? lastSlash - path : 0);
	std::string dir2;
	dir2.assign(path, dirLength);
	dir = (char*)dir2.c_str();
	hz::mfile_t mf;
	data = mf.open_d(path, true);
	if (data)
	{
		length = mf.size();
		atlas = e2d::atlas_create(data, length, dir, rendererObject);
	}
	return atlas;
}

void free_atlas(atlas_xt* atlas)
{
	if (atlas)
	{
		delete atlas;
	}
}

editor2d_cx::editor2d_cx()
{
}

editor2d_cx::~editor2d_cx()
{
}

void editor2d_cx::init(void* p, const texture_cb& cb)
{
	renderer = p;
	tex_cb = cb;
}

void editor2d_cx::set_config(const char* fn)
{
}

bool editor2d_cx::import_image(const char* fn)
{
	return false;
}

bool editor2d_cx::export_atlas(const char* dir, const char* name)
{
	return false;
}

bool editor2d_cx::save(const char* fn)
{
	return false;
}
#define IM_COL32_A_MASK     0xFF000000
enum ImDrawFlags
{
	ImDrawFlags_None = 0,
	ImDrawFlags_Closed = 1 << 0, // PathStroke(), AddPolyline(): specify that shape should be closed (Important: this is always == 1 for legacy reason)
	ImDrawFlags_RoundCornersTopLeft = 1 << 4, // AddRect(), AddRectFilled(), PathRect(): enable rounding top-left corner only (when rounding > 0.0f, we default to all corners). Was 0x01.
	ImDrawFlags_RoundCornersTopRight = 1 << 5, // AddRect(), AddRectFilled(), PathRect(): enable rounding top-right corner only (when rounding > 0.0f, we default to all corners). Was 0x02.
	ImDrawFlags_RoundCornersBottomLeft = 1 << 6, // AddRect(), AddRectFilled(), PathRect(): enable rounding bottom-left corner only (when rounding > 0.0f, we default to all corners). Was 0x04.
	ImDrawFlags_RoundCornersBottomRight = 1 << 7, // AddRect(), AddRectFilled(), PathRect(): enable rounding bottom-right corner only (when rounding > 0.0f, we default to all corners). Wax 0x08.
	ImDrawFlags_RoundCornersNone = 1 << 8, // AddRect(), AddRectFilled(), PathRect(): disable rounding on all corners (when rounding > 0.0f). This is NOT zero, NOT an implicit flag!
	ImDrawFlags_RoundCornersTop = ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersTopRight,
	ImDrawFlags_RoundCornersBottom = ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersBottomRight,
	ImDrawFlags_RoundCornersLeft = ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersTopLeft,
	ImDrawFlags_RoundCornersRight = ImDrawFlags_RoundCornersBottomRight | ImDrawFlags_RoundCornersTopRight,
	ImDrawFlags_RoundCornersAll = ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersTopRight | ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersBottomRight,
	ImDrawFlags_RoundCornersDefault_ = ImDrawFlags_RoundCornersAll, // Default to ALL corners if none of the _RoundCornersXX flags are specified.
	ImDrawFlags_RoundCornersMask_ = ImDrawFlags_RoundCornersAll | ImDrawFlags_RoundCornersNone,
};
struct Vertex_d
{
	glm::vec2 pos;
	glm::vec2 uv;// tex_coord;
	uint32_t col;
};
struct DrawCmd_t
{
	glm::ivec4 ClipRect;
	void* TextureId;
	uint32_t VtxOffset;
	uint32_t IdxOffset;
	uint32_t ElemCount;
};
struct draw_path_info_t
{
	uint32_t col;
	float thickness;
	ImDrawFlags flags;
	bool tex_lines = false;
	bool anti_aliased;
	bool closed;
};

class draw_list_cx
{
public:
	draw_list_cx();
	~draw_list_cx();
	// 折线三角化
	void strokePolyline(const glm::vec2* points, const int points_count, draw_path_info_t* dp);
	// 只支持凸多边形
	void ConvexPolyFilled(const glm::vec2* points, const int points_count, draw_path_info_t* dp);
	// 支持凹凸多边形、带孔洞
	void ConcavePolyFilled(const glm::vec2* points, const int* points_count, int count, draw_path_info_t* dp);
	void PrimReserve(int idx_count, int vtx_count);
	void add_cmd(void* tex, const glm::ivec4& clip_rect, bool clipintersect_with_current_clip_rect);
private:
	float _FringeScale = 1.0;
	glm::vec2 TexUvWhitePixel = {};
	std::vector<glm::vec2> TempBuffer;
	std::vector<DrawCmd_t> CmdBuffer;
	std::vector<uint32_t> IdxBuffer;
	std::vector<Vertex_d> VtxBuffer;
	mesh3_mt _tem_mesh;
	std::vector<std::vector<glm::vec2>> _tem_poly;
	uint32_t _VtxCurrentIdx = 0;     // [Internal] generally == VtxBuffer.Size unless we are past 64K vertices, in which case this gets reset to 0.
	Vertex_d* _VtxWritePtr = 0;       // [Internal] point within VtxBuffer.data() after each add command (to avoid using the ImVector<> operators too much)
	uint32_t* _IdxWritePtr = 0;       // [Internal] point within IdxBuffer.data() after each add command (to avoid using the ImVector<> operators too much)
	const int tex_lines_width_max = 32;
	glm::vec4 TexUvLines[32 + 1] = {};  // UVs for baked anti-aliased lines 
	struct DrawCmdHeader
	{
		glm::ivec4 ClipRect;
		void* TextureId;
		size_t VtxOffset;
	};
	DrawCmdHeader _CmdHeader = {};

};

#if 1

draw_list_cx::draw_list_cx()
{
}

draw_list_cx::~draw_list_cx()
{
}
#define IM_NORMALIZE2F_OVER_ZERO(VX, VY) { float d2 = VX * VX + VY * VY; if (d2 > 0.0f) { float inv_len = 1.0f / sqrtf(d2); VX *= inv_len; VY *= inv_len; } } (void)0
#define IM_FIXNORMAL2F_MAX_INVLEN2          100.0f // 500.0f (see #4053, #3366)
#define IM_FIXNORMAL2F(VX,VY)               { float d2 = VX*VX + VY*VY; if (d2 > 0.000001f) { float inv_len2 = 1.0f / d2; if (inv_len2 > IM_FIXNORMAL2F_MAX_INVLEN2) inv_len2 = IM_FIXNORMAL2F_MAX_INVLEN2; VX *= inv_len2; VY *= inv_len2; } } (void)0

void draw_list_cx::strokePolyline(const glm::vec2* points, const int points_count, draw_path_info_t* dp) {
	if (points_count < 2 || (dp->col & IM_COL32_A_MASK) == 0)
		return;

	const bool closed = dp->closed;
	const glm::vec2 opaque_uv = TexUvWhitePixel;
	const int count = closed ? points_count : points_count - 1; // 线段数
	const bool thick_line = (dp->thickness > _FringeScale);
	auto thickness = dp->thickness;
	auto col = dp->col;
	if (dp->anti_aliased)
	{
		// Anti-aliased stroke
		const float AA_SIZE = _FringeScale;
		const uint32_t col_trans = dp->col & ~IM_COL32_A_MASK;

		// Thicknesses <1.0 should behave like thickness 1.0
		thickness = glm::max(dp->thickness, 1.0f);
		const int integer_thickness = (int)thickness;
		const float fractional_thickness = thickness - integer_thickness;

		// Do we want to draw this line using a texture?
		// - For now, only draw integer-width lines using textures to avoid issues with the way scaling occurs, could be improved.
		// - If AA_SIZE is not 1.0f we cannot use the texture path.
		const bool use_texture = dp->tex_lines && (integer_thickness < tex_lines_width_max) && (fractional_thickness <= 0.00001f) && (AA_SIZE == 1.0f);

		// We should never hit this, because NewFrame() doesn't set ImDrawListFlags_AntiAliasedLinesUseTex unless ImFontAtlasFlags_NoBakedLines is off
		//assert(!use_texture || !(_Data->Font->ContainerAtlas->Flags & ImFontAtlasFlags_NoBakedLines));

		const int idx_count = use_texture ? (count * 6) : (thick_line ? count * 18 : count * 12);
		const int vtx_count = use_texture ? (points_count * 2) : (thick_line ? points_count * 4 : points_count * 3);
		PrimReserve(idx_count, vtx_count);
		// Temporary buffer
		// The first <points_count> items are normals at each line point, then after that there are either 2 or 4 temp points for each line point
		TempBuffer.resize(points_count * ((use_texture || !thick_line) ? 3 : 5));
		glm::vec2* temp_normals = TempBuffer.data();
		glm::vec2* temp_points = temp_normals + points_count;

		// Calculate normals (tangents) for each line segment
		for (int i1 = 0; i1 < count; i1++)
		{
			const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
			float dx = points[i2].x - points[i1].x;
			float dy = points[i2].y - points[i1].y;
			IM_NORMALIZE2F_OVER_ZERO(dx, dy);
			temp_normals[i1].x = dy;
			temp_normals[i1].y = -dx;
		}
		if (!closed)
			temp_normals[points_count - 1] = temp_normals[points_count - 2];

		// If we are drawing a one-pixel-wide line without a texture, or a textured line of any width, we only need 2 or 3 vertices per point
		if (use_texture || !thick_line)
		{
			const float half_draw_size = use_texture ? ((thickness * 0.5f) + 1) : AA_SIZE;
			// If line is not closed, the first and last points need to be generated differently as there are no normals to blend
			if (!closed)
			{
				temp_points[0] = points[0] + temp_normals[0] * half_draw_size;
				temp_points[1] = points[0] - temp_normals[0] * half_draw_size;
				temp_points[(points_count - 1) * 2 + 0] = points[points_count - 1] + temp_normals[points_count - 1] * half_draw_size;
				temp_points[(points_count - 1) * 2 + 1] = points[points_count - 1] - temp_normals[points_count - 1] * half_draw_size;
			}

			// Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
			// This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
			// FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
			uint32_t idx1 = _VtxCurrentIdx; // Vertex index for start of line segment
			for (int i1 = 0; i1 < count; i1++) // i1 is the first point of the line segment
			{
				const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1; // i2 is the second point of the line segment
				const uint32_t idx2 = ((i1 + 1) == points_count) ? _VtxCurrentIdx : (idx1 + (use_texture ? 2 : 3)); // Vertex index for end of segment

				// Average normals
				float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
				float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
				IM_FIXNORMAL2F(dm_x, dm_y);
				dm_x *= half_draw_size; // dm_x, dm_y are offset to the outer edge of the AA area
				dm_y *= half_draw_size;

				// Add temporary vertices for the outer edges
				glm::vec2* out_vtx = &temp_points[i2 * 2];
				out_vtx[0].x = points[i2].x + dm_x;
				out_vtx[0].y = points[i2].y + dm_y;
				out_vtx[1].x = points[i2].x - dm_x;
				out_vtx[1].y = points[i2].y - dm_y;
				if (use_texture)
				{
					// Add indices for two triangles
					_IdxWritePtr[0] = (uint32_t)(idx2 + 0); _IdxWritePtr[1] = (uint32_t)(idx1 + 0); _IdxWritePtr[2] = (uint32_t)(idx1 + 1); // Right tri
					_IdxWritePtr[3] = (uint32_t)(idx2 + 1); _IdxWritePtr[4] = (uint32_t)(idx1 + 1); _IdxWritePtr[5] = (uint32_t)(idx2 + 0); // Left tri
					_IdxWritePtr += 6;
				}
				else
				{
					// Add indexes for four triangles
					_IdxWritePtr[0] = (uint32_t)(idx2 + 0); _IdxWritePtr[1] = (uint32_t)(idx1 + 0); _IdxWritePtr[2] = (uint32_t)(idx1 + 2); // Right tri 1
					_IdxWritePtr[3] = (uint32_t)(idx1 + 2); _IdxWritePtr[4] = (uint32_t)(idx2 + 2); _IdxWritePtr[5] = (uint32_t)(idx2 + 0); // Right tri 2
					_IdxWritePtr[6] = (uint32_t)(idx2 + 1); _IdxWritePtr[7] = (uint32_t)(idx1 + 1); _IdxWritePtr[8] = (uint32_t)(idx1 + 0); // Left tri 1
					_IdxWritePtr[9] = (uint32_t)(idx1 + 0); _IdxWritePtr[10] = (uint32_t)(idx2 + 0); _IdxWritePtr[11] = (uint32_t)(idx2 + 1); // Left tri 2
					_IdxWritePtr += 12;
				}

				idx1 = idx2;
			}

			// Add vertices for each point on the line
			if (use_texture)
			{
				// If we're using textures we only need to emit the left/right edge vertices
				glm::vec4 tex_uvs = TexUvLines[integer_thickness];
				glm::vec2 tex_uv0(tex_uvs.x, tex_uvs.y);
				glm::vec2 tex_uv1(tex_uvs.z, tex_uvs.w);
				for (int i = 0; i < points_count; i++)
				{
					_VtxWritePtr[0].pos = temp_points[i * 2 + 0]; _VtxWritePtr[0].uv = tex_uv0; _VtxWritePtr[0].col = col; // Left-side outer edge
					_VtxWritePtr[1].pos = temp_points[i * 2 + 1]; _VtxWritePtr[1].uv = tex_uv1; _VtxWritePtr[1].col = col; // Right-side outer edge
					_VtxWritePtr += 2;
				}
			}
			else
			{
				// If we're not using a texture, we need the center vertex as well
				for (int i = 0; i < points_count; i++)
				{
					_VtxWritePtr[0].pos = points[i];              _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col;       // Center of line
					_VtxWritePtr[1].pos = temp_points[i * 2 + 0]; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col_trans; // Left-side outer edge
					_VtxWritePtr[2].pos = temp_points[i * 2 + 1]; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col_trans; // Right-side outer edge
					_VtxWritePtr += 3;
				}
			}
		}
		else
		{
			// [PATH 2] Non texture-based lines (thick): we need to draw the solid line core and thus require four vertices per point
			const float half_inner_thickness = (thickness - AA_SIZE) * 0.5f;

			// If line is not closed, the first and last points need to be generated differently as there are no normals to blend
			if (!closed)
			{
				const int points_last = points_count - 1;
				temp_points[0] = points[0] + temp_normals[0] * (half_inner_thickness + AA_SIZE);
				temp_points[1] = points[0] + temp_normals[0] * (half_inner_thickness);
				temp_points[2] = points[0] - temp_normals[0] * (half_inner_thickness);
				temp_points[3] = points[0] - temp_normals[0] * (half_inner_thickness + AA_SIZE);
				temp_points[points_last * 4 + 0] = points[points_last] + temp_normals[points_last] * (half_inner_thickness + AA_SIZE);
				temp_points[points_last * 4 + 1] = points[points_last] + temp_normals[points_last] * (half_inner_thickness);
				temp_points[points_last * 4 + 2] = points[points_last] - temp_normals[points_last] * (half_inner_thickness);
				temp_points[points_last * 4 + 3] = points[points_last] - temp_normals[points_last] * (half_inner_thickness + AA_SIZE);
			}

			// Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
			// This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
			// FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
			uint32_t idx1 = _VtxCurrentIdx; // Vertex index for start of line segment
			for (int i1 = 0; i1 < count; i1++) // i1 is the first point of the line segment
			{
				const int i2 = (i1 + 1) == points_count ? 0 : (i1 + 1); // i2 is the second point of the line segment
				const uint32_t idx2 = (i1 + 1) == points_count ? _VtxCurrentIdx : (idx1 + 4); // Vertex index for end of segment

				// Average normals
				float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
				float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
				IM_FIXNORMAL2F(dm_x, dm_y);
				float dm_out_x = dm_x * (half_inner_thickness + AA_SIZE);
				float dm_out_y = dm_y * (half_inner_thickness + AA_SIZE);
				float dm_in_x = dm_x * half_inner_thickness;
				float dm_in_y = dm_y * half_inner_thickness;

				// Add temporary vertices
				glm::vec2* out_vtx = &temp_points[i2 * 4];
				out_vtx[0].x = points[i2].x + dm_out_x;
				out_vtx[0].y = points[i2].y + dm_out_y;
				out_vtx[1].x = points[i2].x + dm_in_x;
				out_vtx[1].y = points[i2].y + dm_in_y;
				out_vtx[2].x = points[i2].x - dm_in_x;
				out_vtx[2].y = points[i2].y - dm_in_y;
				out_vtx[3].x = points[i2].x - dm_out_x;
				out_vtx[3].y = points[i2].y - dm_out_y;

				// Add indexes
				_IdxWritePtr[0] = (uint32_t)(idx2 + 1); _IdxWritePtr[1] = (uint32_t)(idx1 + 1); _IdxWritePtr[2] = (uint32_t)(idx1 + 2);
				_IdxWritePtr[3] = (uint32_t)(idx1 + 2); _IdxWritePtr[4] = (uint32_t)(idx2 + 2); _IdxWritePtr[5] = (uint32_t)(idx2 + 1);
				_IdxWritePtr[6] = (uint32_t)(idx2 + 1); _IdxWritePtr[7] = (uint32_t)(idx1 + 1); _IdxWritePtr[8] = (uint32_t)(idx1 + 0);
				_IdxWritePtr[9] = (uint32_t)(idx1 + 0); _IdxWritePtr[10] = (uint32_t)(idx2 + 0); _IdxWritePtr[11] = (uint32_t)(idx2 + 1);
				_IdxWritePtr[12] = (uint32_t)(idx2 + 2); _IdxWritePtr[13] = (uint32_t)(idx1 + 2); _IdxWritePtr[14] = (uint32_t)(idx1 + 3);
				_IdxWritePtr[15] = (uint32_t)(idx1 + 3); _IdxWritePtr[16] = (uint32_t)(idx2 + 3); _IdxWritePtr[17] = (uint32_t)(idx2 + 2);
				_IdxWritePtr += 18;

				idx1 = idx2;
			}

			// Add vertices
			for (int i = 0; i < points_count; i++)
			{
				_VtxWritePtr[0].pos = temp_points[i * 4 + 0]; _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col_trans;
				_VtxWritePtr[1].pos = temp_points[i * 4 + 1]; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col;
				_VtxWritePtr[2].pos = temp_points[i * 4 + 2]; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col;
				_VtxWritePtr[3].pos = temp_points[i * 4 + 3]; _VtxWritePtr[3].uv = opaque_uv; _VtxWritePtr[3].col = col_trans;
				_VtxWritePtr += 4;
			}
		}
		_VtxCurrentIdx += (uint32_t)vtx_count;
	}
	else
	{
		// [PATH 4] Non texture-based, Non anti-aliased lines
		const int idx_count = count * 6;
		const int vtx_count = count * 4;    // FIXME-OPT: Not sharing edges
		PrimReserve(idx_count, vtx_count);

		for (int i1 = 0; i1 < count; i1++)
		{
			const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
			const glm::vec2& p1 = points[i1];
			const glm::vec2& p2 = points[i2];

			float dx = p2.x - p1.x;
			float dy = p2.y - p1.y;
			IM_NORMALIZE2F_OVER_ZERO(dx, dy);
			dx *= (thickness * 0.5f);
			dy *= (thickness * 0.5f);

			_VtxWritePtr[0].pos.x = p1.x + dy; _VtxWritePtr[0].pos.y = p1.y - dx; _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col;
			_VtxWritePtr[1].pos.x = p2.x + dy; _VtxWritePtr[1].pos.y = p2.y - dx; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col;
			_VtxWritePtr[2].pos.x = p2.x - dy; _VtxWritePtr[2].pos.y = p2.y + dx; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col;
			_VtxWritePtr[3].pos.x = p1.x - dy; _VtxWritePtr[3].pos.y = p1.y + dx; _VtxWritePtr[3].uv = opaque_uv; _VtxWritePtr[3].col = col;
			_VtxWritePtr += 4;

			_IdxWritePtr[0] = (uint32_t)(_VtxCurrentIdx); _IdxWritePtr[1] = (uint32_t)(_VtxCurrentIdx + 1); _IdxWritePtr[2] = (uint32_t)(_VtxCurrentIdx + 2);
			_IdxWritePtr[3] = (uint32_t)(_VtxCurrentIdx); _IdxWritePtr[4] = (uint32_t)(_VtxCurrentIdx + 2); _IdxWritePtr[5] = (uint32_t)(_VtxCurrentIdx + 3);
			_IdxWritePtr += 6;
			_VtxCurrentIdx += 4;
		}
	}
}

// - We intentionally avoid using glm::vec2 and its math operators here to reduce cost to a minimum for debug/non-inlined builds.
// - Filled shapes must always use clockwise winding order. The anti-aliasing fringe depends on it. Counter-clockwise shapes will have "inward" anti-aliasing.
void draw_list_cx::ConvexPolyFilled(const glm::vec2* points, const int points_count, draw_path_info_t* dp)
{
	auto col = dp->col;
	if (points_count < 3 || (col & IM_COL32_A_MASK) == 0)
		return;

	const glm::vec2 uv = TexUvWhitePixel;

	if (dp->anti_aliased)
	{
		// Anti-aliased Fill
		const float AA_SIZE = _FringeScale;
		const uint32_t col_trans = col & ~IM_COL32_A_MASK;
		const int idx_count = (points_count - 2) * 3 + points_count * 6;
		const int vtx_count = (points_count * 2);
		PrimReserve(idx_count, vtx_count);

		// Add indexes for fill
		uint32_t vtx_inner_idx = _VtxCurrentIdx;
		uint32_t vtx_outer_idx = _VtxCurrentIdx + 1;
		for (int i = 2; i < points_count; i++)
		{
			_IdxWritePtr[0] = (uint32_t)(vtx_inner_idx); _IdxWritePtr[1] = (uint32_t)(vtx_inner_idx + ((i - 1) << 1)); _IdxWritePtr[2] = (uint32_t)(vtx_inner_idx + (i << 1));
			_IdxWritePtr += 3;
		}

		// Compute normals
		TempBuffer.resize(points_count);
		glm::vec2* temp_normals = TempBuffer.data();
		for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
		{
			const glm::vec2& p0 = points[i0];
			const glm::vec2& p1 = points[i1];
			float dx = p1.x - p0.x;
			float dy = p1.y - p0.y;
			IM_NORMALIZE2F_OVER_ZERO(dx, dy);
			temp_normals[i0].x = dy;
			temp_normals[i0].y = -dx;
		}

		for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
		{
			// Average normals
			const glm::vec2& n0 = temp_normals[i0];
			const glm::vec2& n1 = temp_normals[i1];
			float dm_x = (n0.x + n1.x) * 0.5f;
			float dm_y = (n0.y + n1.y) * 0.5f;
			IM_FIXNORMAL2F(dm_x, dm_y);
			dm_x *= AA_SIZE * 0.5f;
			dm_y *= AA_SIZE * 0.5f;

			// Add vertices
			_VtxWritePtr[0].pos.x = (points[i1].x - dm_x); _VtxWritePtr[0].pos.y = (points[i1].y - dm_y); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
			_VtxWritePtr[1].pos.x = (points[i1].x + dm_x); _VtxWritePtr[1].pos.y = (points[i1].y + dm_y); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
			_VtxWritePtr += 2;

			// Add indexes for fringes
			_IdxWritePtr[0] = (uint32_t)(vtx_inner_idx + (i1 << 1)); _IdxWritePtr[1] = (uint32_t)(vtx_inner_idx + (i0 << 1)); _IdxWritePtr[2] = (uint32_t)(vtx_outer_idx + (i0 << 1));
			_IdxWritePtr[3] = (uint32_t)(vtx_outer_idx + (i0 << 1)); _IdxWritePtr[4] = (uint32_t)(vtx_outer_idx + (i1 << 1)); _IdxWritePtr[5] = (uint32_t)(vtx_inner_idx + (i1 << 1));
			_IdxWritePtr += 6;
		}
		_VtxCurrentIdx += (uint32_t)vtx_count;
	}
	else
	{
		// Non Anti-aliased Fill
		const int idx_count = (points_count - 2) * 3;
		const int vtx_count = points_count;
		PrimReserve(idx_count, vtx_count);
		for (int i = 0; i < vtx_count; i++)
		{
			_VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
			_VtxWritePtr++;
		}
		for (int i = 2; i < points_count; i++)
		{
			_IdxWritePtr[0] = (uint32_t)(_VtxCurrentIdx); _IdxWritePtr[1] = (uint32_t)(_VtxCurrentIdx + i - 1); _IdxWritePtr[2] = (uint32_t)(_VtxCurrentIdx + i);
			_IdxWritePtr += 3;
		}
		_VtxCurrentIdx += (uint32_t)vtx_count;
	}
}

// Use ear-clipping algorithm to triangulate a simple polygon (no self-interaction, no holes).
// (Reminder: we don't perform any coarse clipping/culling in ImDrawList layer!
// It is up to caller to ensure not making costly calls that will be outside of visible area.
// As concave fill is noticeably more expensive than other primitives, be mindful of this...
// Caller can build AABB of points, and avoid filling if 'draw_list->_CmdHeader.ClipRect.Overlays(points_bb) == false')
void draw_list_cx::ConcavePolyFilled(const glm::vec2* points, const int* points_count0, int count, draw_path_info_t* dp)
{
	auto col = dp->col;
	if (!points_count0 || count < 1 || (col & IM_COL32_A_MASK) == 0)
		return;

	const glm::vec2 uv = TexUvWhitePixel;
	size_t points_count = 0;
	_tem_poly.resize(count);
	auto v2 = (glm::vec2*)points;
	for (size_t i = 0; i < count; i++)
	{
		auto& it = _tem_poly[i];
		it.reserve(points_count0[i]);
		points_count += points_count0[i];
		for (size_t x = 0; x < points_count0[i]; x++)
		{
			it.push_back(*v2);
			v2++;
		}
	}

	triangulates(_tem_poly.data(), count, 0, &_tem_mesh, 0);
	points_count = _tem_mesh.vertices.size();
	auto pt = _tem_mesh.vertices.data();
	auto ic = _tem_mesh.indices.size();

	if (dp->anti_aliased)
	{
		// Anti-aliased Fill
		const float AA_SIZE = _FringeScale;
		const uint32_t col_trans = col & ~IM_COL32_A_MASK;
		const int idx_count = ic + points_count * 6;
		const int vtx_count = points_count * 2;
		PrimReserve(idx_count, vtx_count);
		// Add indexes for fill
		uint32_t vtx_inner_idx = _VtxCurrentIdx;
		uint32_t vtx_outer_idx = _VtxCurrentIdx + 1;

		for (auto& it : _tem_mesh.indices)
		{
			_IdxWritePtr[0] = (uint32_t)(vtx_inner_idx + (it[0] << 1)); _IdxWritePtr[1] = (uint32_t)(vtx_inner_idx + (it[1] << 1)); _IdxWritePtr[2] = (uint32_t)(vtx_inner_idx + (it[2] << 1));
			_IdxWritePtr += 3;
		}
		// Compute normals
		TempBuffer.resize(points_count);
		glm::vec2* temp_normals = TempBuffer.data();
		for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
		{
			const glm::vec2& p0 = pt[i0];
			const glm::vec2& p1 = pt[i1];
			float dx = p1.x - p0.x;
			float dy = p1.y - p0.y;
			IM_NORMALIZE2F_OVER_ZERO(dx, dy);
			temp_normals[i0].x = dy;
			temp_normals[i0].y = -dx;
		}

		for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
		{
			// Average normals
			const glm::vec2& n0 = temp_normals[i0];
			const glm::vec2& n1 = temp_normals[i1];
			float dm_x = (n0.x + n1.x) * 0.5f;
			float dm_y = (n0.y + n1.y) * 0.5f;
			IM_FIXNORMAL2F(dm_x, dm_y);
			dm_x *= AA_SIZE * 0.5f;
			dm_y *= AA_SIZE * 0.5f;

			// Add vertices
			_VtxWritePtr[0].pos.x = (pt[i1].x - dm_x); _VtxWritePtr[0].pos.y = (pt[i1].y - dm_y); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
			_VtxWritePtr[1].pos.x = (pt[i1].x + dm_x); _VtxWritePtr[1].pos.y = (pt[i1].y + dm_y); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
			_VtxWritePtr += 2;

			// Add indexes for fringes
			_IdxWritePtr[0] = (uint32_t)(vtx_inner_idx + (i1 << 1)); _IdxWritePtr[1] = (uint32_t)(vtx_inner_idx + (i0 << 1)); _IdxWritePtr[2] = (uint32_t)(vtx_outer_idx + (i0 << 1));
			_IdxWritePtr[3] = (uint32_t)(vtx_outer_idx + (i0 << 1)); _IdxWritePtr[4] = (uint32_t)(vtx_outer_idx + (i1 << 1)); _IdxWritePtr[5] = (uint32_t)(vtx_inner_idx + (i1 << 1));
			_IdxWritePtr += 6;
		}
		_VtxCurrentIdx += (uint32_t)vtx_count;
	}
	else
	{
		// Non Anti-aliased Fill
		const int idx_count = _tem_mesh.indices.size();
		const int vtx_count = _tem_mesh.vertices.size();
		PrimReserve(idx_count, vtx_count);
		for (int i = 0; i < vtx_count; i++)
		{
			_VtxWritePtr[0].pos = pt[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
			_VtxWritePtr++;
		}
		for (auto& it : _tem_mesh.indices)
		{
			_IdxWritePtr[0] = (uint32_t)(_VtxCurrentIdx + it[0]); _IdxWritePtr[1] = (uint32_t)(_VtxCurrentIdx + it[1]); _IdxWritePtr[2] = (uint32_t)(_VtxCurrentIdx + it[2]);
			_IdxWritePtr += 3;
		}
		_VtxCurrentIdx += (uint32_t)vtx_count;
	}
}
void draw_list_cx::PrimReserve(int idx_count, int vtx_count)
{
	size_t ns = IdxBuffer.size() + idx_count;
	size_t vs = VtxBuffer.size() + vtx_count;
	//ns = align_up(ns, 12);
	//vs = align_up(vs, 12);
	assert(CmdBuffer.size());
	auto draw_cmd = CmdBuffer.rbegin();
	draw_cmd->ElemCount += idx_count;

	IdxBuffer.resize(IdxBuffer.size() + idx_count);
	VtxBuffer.resize(VtxBuffer.size() + vtx_count);
	_VtxWritePtr = VtxBuffer.data();
	_IdxWritePtr = IdxBuffer.data();
}

glm::ivec4 clipintersect_clip_rect(const glm::ivec4& clip_rect, const glm::ivec4& current)
{
	glm::ivec4 cr = clip_rect;
	if (cr.x < current.x) cr.x = current.x;
	if (cr.y < current.y) cr.y = current.y;
	if (cr.z > current.z) cr.z = current.z;
	if (cr.w > current.w) cr.w = current.w;
	cr.z = glm::max(cr.x, cr.z);
	cr.w = glm::max(cr.y, cr.w);
	return cr;
}
void draw_list_cx::add_cmd(void* tex, const glm::ivec4& clip_rect, bool clipintersect_with_current_clip_rect)
{
	DrawCmd_t draw_cmd = {};
	draw_cmd.ClipRect = clipintersect_with_current_clip_rect ? clipintersect_clip_rect(clip_rect, _CmdHeader.ClipRect) : clip_rect;
	draw_cmd.TextureId = tex;
	draw_cmd.VtxOffset = VtxBuffer.size();
	draw_cmd.IdxOffset = IdxBuffer.size();
	assert(draw_cmd.ClipRect.x <= draw_cmd.ClipRect.z && draw_cmd.ClipRect.y <= draw_cmd.ClipRect.w);
	CmdBuffer.push_back(draw_cmd);
}
#endif

/**
 * 与门
 */
bool and_gate(bool in_1, bool in_2)
{
	return in_1 && in_2;
}

/**
 * 或门
 */
bool or_gate(bool in_1, bool in_2)
{
	return in_1 || in_2;
}

/**
 * 非门
 */
bool not_gate(bool in_1)
{
	return !in_1;
}

/**
 * 或非门
 */
bool not_or_gate(bool in_1, bool in_2)
{
	return !(in_1 || in_2);
}

/**
 * 与非门
 */
bool not_and_gate(bool in_1, bool in_2)
{
	return !(in_1 && in_2);
}

/**
 * 异或门
 */
bool xor_gate(bool in_1, bool in_2)
{
	return in_1 ^ in_2;
	//bool or_val = or_gate(in_1, in_2);
	//bool not_and_val = not_and_gate(in_1, in_2);
	//return or_val && not_and_val;
}
