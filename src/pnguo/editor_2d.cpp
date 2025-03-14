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
		if (it.rotate > 0)
			fprintf(fp, "\trotate: %d\n", it.rotate);
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
		nt["rotate"] = it.rotate;
		nt["bounds"] = { it.bounds.x, it.bounds.y, it.bounds.z, it.bounds.w };
		nt["offsets"] = { it.offsets.x, it.offsets.y, it.offsets.z, it.offsets.w };
		region2.push_back(nt);
	}
	n["region"] = region2;
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
