
#include "pch1.h"
#include "vkrenderer.h"



vkdg_cx::vkdg_cx()
{
}

vkdg_cx::~vkdg_cx()
{
}



BBoxPass::BBoxPass()
{
}

BBoxPass::~BBoxPass()
{
}

void BBoxPass::add(glm::mat4* m, BBoxPass::box_t* vcr, size_t count)
{
	if (!m || !vcr || !count)return;
	auto nps = boxs.size();
	for (size_t i = 0; i < count; i++)
	{
		boxs.push_back(vcr[i]);
	}
	box_ms.push_back({ m,nps,count });
}

void BBoxPass::clear()
{
	boxs.clear();
	box_ms.clear();
}

vkdg_cx* new_vkdg(dev_info_cx* c)
{
	auto p = new vkdg_cx();
	if (c) {

	}
	return p;
}

void free_vkdg(vkdg_cx* p)
{
	if (p)delete p;
}
