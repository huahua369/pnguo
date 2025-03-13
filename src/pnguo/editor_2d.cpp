/*
 2d编辑器
 创建时间2025-3-8
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
void generate_atlas(const char* output_path, atlas_t* atlas)
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
atlas_t* json2atlas(njson0& n)
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

	auto ap = new atlas_t{ name, size, format, filter, repeat, pma };
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

void atlas2json(atlas_t* a, njson0& n)
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

void free_atlas(atlas_t* atlas)
{
	if (atlas)
	{
		delete atlas;
	}
}
