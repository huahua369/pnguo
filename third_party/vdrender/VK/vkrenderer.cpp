
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
