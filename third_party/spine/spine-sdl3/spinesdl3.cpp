/*
2D动画后端
*/
#include <spine/spine.h>

//#define MAPVIEW
#ifndef _WIN32 
#include <unistd.h>  
#include <sys/mman.h>
#include <sys/types.h>  
#include <sys/stat.h> 
#include <fcntl.h>
#endif
#include <io.h>

//#include <SDL3/SDL.h>
#include <set>
#include "spinesdl3.h"

#include <nlohmann/json.hpp>
#if defined( NLOHMANN_JSON_HPP) || defined(INCLUDE_NLOHMANN_JSON_HPP_)
using njson = nlohmann::json;			// key有序
using njson0 = nlohmann::ordered_json;	// key无序
#define NJSON_H
#endif
#if __has_include( <mapView.h>)
#include <mapView.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


	struct spSdlVertexArray;
	typedef struct spSkeletonDrawable {
		spSkeleton* skeleton;
		spAnimationState* animationState;
		spSkeletonClipping* clipper;
		spFloatArray* worldVertices;
		spSdlVertexArray* sdlVertices;
		spIntArray* sdlIndices;
		int8_t usePremultipliedAlpha;
		int8_t ownsAnimationStateData;
		int8_t ownsSkeletonData;
	} spSkeletonDrawable;

	SP_API spSkeletonDrawable* spSkeletonDrawable_create(spSkeletonData* skeletonData, spAnimationStateData* animationStateData, int ownsSkeletonData, float defaultMix);

	SP_API void spSkeletonDrawable_dispose(spSkeletonDrawable* self);

	SP_API void spSkeletonDrawable_update(spSkeletonDrawable* self, float delta, spPhysics physics);

	//SP_API void spSkeletonDrawable_draw(spSkeletonDrawable* self, struct SDL_Renderer* renderer);

	_SP_ARRAY_DECLARE_TYPE(spSdlVertexArray, struct SP_Vertex)
#ifdef __cplusplus
}
#endif


#ifndef SRC_LIBSRC

#include <spine/extension.h>


#include <stb_image.h>

_SP_ARRAY_IMPLEMENT_TYPE_NO_CONTAINS(spSdlVertexArray, SP_Vertex)

spSkeletonDrawable* spSkeletonDrawable_create(spSkeletonData* skeletonData, spAnimationStateData* animationStateData, int ownsSkeletonData, float defaultMix) {
	spBone_setYDown(-1);
	spSkeletonDrawable* self = NEW(spSkeletonDrawable);
	self->skeleton = spSkeleton_create(skeletonData);
	self->ownsAnimationStateData = animationStateData ? 0 : 1;
	if (self->ownsAnimationStateData)
	{
		animationStateData = spAnimationStateData_create(skeletonData);
	}
	if (!animationStateData)
	{
		spSkeleton_dispose(self->skeleton);
		FREE(self);
		return 0;
	}
	animationStateData->defaultMix = defaultMix;
	self->ownsSkeletonData = ownsSkeletonData;
	self->animationState = spAnimationState_create(animationStateData);
	self->usePremultipliedAlpha = 0;
	self->sdlIndices = spIntArray_create(12);
	self->sdlVertices = spSdlVertexArray_create(12);
	self->worldVertices = spFloatArray_create(12);
	self->clipper = spSkeletonClipping_create();
	return self;
}

void spSkeletonDrawable_dispose(spSkeletonDrawable* self) {
	if (self->ownsAnimationStateData)
	{
		spAnimationStateData_dispose(self->animationState->data);
	}
	if (self->ownsSkeletonData)
	{
		spSkeletonData_dispose(self->skeleton->data);
	}
	spSkeleton_dispose(self->skeleton);
	spAnimationState_dispose(self->animationState);
	spIntArray_dispose(self->sdlIndices);
	spSdlVertexArray_dispose(self->sdlVertices);
	spFloatArray_dispose(self->worldVertices);
	spSkeletonClipping_dispose(self->clipper);
	FREE(self);
}

void spSkeletonDrawable_update(spSkeletonDrawable* self, float delta, spPhysics physics) {
	spAnimationState_update(self->animationState, delta);
	spAnimationState_apply(self->animationState, self->skeleton);
	spSkeleton_update(self->skeleton, delta);
	spSkeleton_updateWorldTransform(self->skeleton, physics);
}


struct page_obj_t
{
	sp_drawable_ctx* ptr = 0;
	void* renderer = 0;
	std::map<void*, std::string> _texs;
	njson0 img;
	char* data;
	int len;
};
struct robj_t
{
	sp_drawable_ctx* ptr;
	SP_Texture* texture;
};

void _spAtlasPage_createTexture(spAtlasPage* self, const char* path)
{
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
	SP_Texture* texture = p->ptr->CreateTexture((SDL_Renderer*)p->renderer, width, height);// SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC,
	if (!texture) {
		stbi_image_free(imageData);
		return;
	}
	if (!p->ptr->UpdateTexture(texture, imageData, width * 4)) {
		stbi_image_free(imageData);
		return;
	}
	p->_texs[texture] = path;
	stbi_image_free(imageData);

	robj_t* robj = (robj_t*)malloc(sizeof(robj_t));
	if (robj)
	{
		robj->ptr = p->ptr;
		robj->texture = texture;
		self->rendererObject = robj;
	}
	else {
		p->ptr->DestroyTexture(texture);
	}
	return;
}

void _spAtlasPage_disposeTexture(spAtlasPage* self) {
	auto p = (robj_t*)self->rendererObject;
	if (p->ptr && p->ptr->DestroyTexture)
		p->ptr->DestroyTexture((SP_Texture*)p->texture);
}

char* _spUtil_readFile(const char* path, int* length) {
	return _spReadFile(path, length);
}

struct sp_obj {
	spSkeletonDrawable* drawable = 0;
	spAtlas* atlas = 0;
	bool visible = true;
};
void dispose_spobj(sp_obj* p) {
	spAtlas_dispose(p->atlas);
	spSkeletonDrawable_dispose(p->drawable);
}

sp_drawable_ctx::sp_drawable_ctx()
{
}

sp_drawable_ctx::~sp_drawable_ctx()
{
	for (auto& it : drawables) {
		dispose_spobj((sp_obj*)&it);
	}
}

void sp_drawable_ctx::set_renderer(void* p, draw_geometry_fun rendergeometryraw, newTexture_fun createtexture, UpdateTexture_fun updatetexture, DestroyTexture_fun destroytexture, SetTextureBlendMode_fun settextureblendmode)
{ 
	renderer = p;
	RenderGeometryRaw = rendergeometryraw;
	CreateTexture = createtexture;
	UpdateTexture = updatetexture;
	DestroyTexture = destroytexture;
	SetTextureBlendMode = settextureblendmode;
}
struct spe_ht
{
	char n[4] = {};
	uint64_t datalen;
};
void packages_b(const std::string& package_file, std::map<void*, std::string>& texs, int atlas_length, int ske_length, const char* atlas_data, const char* ske_data, bool isbin);

void sp_drawable_ctx::add(const std::string& atlasf, const std::string& ske, float scale, float defaultMix, const std::string& package_file)
{
	if (!renderer)return;
	sp_obj c = {};
	int atlas_length = 0;
	int ske_length = 0;
	auto atlas_data = _spUtil_readFile(atlasf.c_str(), &atlas_length);
	auto ske_data = _spUtil_readFile(ske.c_str(), &ske_length);
	if (!atlas_data || !ske_data) {
		if (atlas_data)
			_spFree(atlas_data);
		if (ske_data)
			_spFree(ske_data);
		return;
	}
	int xal = 0;
	auto ad = atlas_data;
	auto sd = ske_data;
	atlas_data = hz::tbom(atlas_data, &xal);
	atlas_length -= xal;
	ske_data = hz::tbom(ske_data, &xal);
	ske_length -= xal;
	page_obj_t pot = { this, renderer };
	c.atlas = spAtlas_createFromFile(atlasf.c_str(), &pot);
	bool isbin = false;
	std::string jd;
	spSkeletonData* skeletonData = 0;
	if (ske_data[0] == '{')
	{
		jd.assign(ske_data, ske_length); ske_length++;
		_spFree(ske_data); ske_data = sd = 0;
		spSkeletonJson* json = spSkeletonJson_create(c.atlas);
		if (json) {
			json->scale = scale;
			skeletonData = spSkeletonJson_readSkeletonData(json, jd.c_str());
			spSkeletonJson_dispose(json);
		}
	}
	else if (!skeletonData)
	{
		auto sb = spSkeletonBinary_create(c.atlas);
		if (sb) {
			sb->scale = scale;
			skeletonData = spSkeletonBinary_readSkeletonData(sb, (uint8_t*)ske_data, ske_length);
			spSkeletonBinary_dispose(sb);
			isbin = true;
		}
	}
	if (skeletonData)
	{
		c.drawable = spSkeletonDrawable_create(skeletonData, 0, 1, defaultMix);
		if (c.drawable)
		{
			c.drawable->usePremultipliedAlpha = 0;
			spSkeleton_setToSetupPose(c.drawable->skeleton);
			spSkeletonDrawable_update(c.drawable, 0, SP_PHYSICS_UPDATE);
			drawables.push_back(*((sp_obj_c*)&c));
			if (package_file.size())
			{
				// 打包
				packages_b(package_file, pot._texs, atlas_length, ske_length, atlas_data, ske_data ? ske_data : jd.c_str(), isbin);
			}
		}
	}
	if (ad)
		_spFree(ad);
	if (sd)
		_spFree(sd);
}
void sp_drawable_ctx::add_pkg_data(const char* data, size_t len, float scale, float defaultMix)
{
	if (!renderer || (sizeof(spe_ht) + 16) > len)return;
	sp_obj c = {};
	auto pkbs = (spe_ht*)data;
	std::string pkbn = "spe";
	if (pkbs->n != pkbn)return;
	njson0 pk = njson0::from_cbor(data + sizeof(spe_ht), pkbs->datalen);
	if (!(pk.is_object() && pk.size()))return;
	int atlas_offset = pk["atlas_offset"];
	int atlas_len = pk["atlas_length"];
	int ske_offset = pk["ske_offset"];
	int ske_length = pk["ske_length"];
	bool is_binary = pk["is_binary"];
	data += pkbs->datalen + sizeof(spe_ht);
	page_obj_t pot = { this, renderer };
	pot.img = pk["images"];
	pot.data = (char*)data;
	pot.len = pk["data_length"];
	c.atlas = spAtlas_create(data + atlas_offset, atlas_len, "", &pot);
	const char* ske = data + ske_offset;
	spSkeletonData* skeletonData = 0;
	if (is_binary)
	{
		auto sb = spSkeletonBinary_create(c.atlas);
		sb->scale = scale;
		skeletonData = spSkeletonBinary_readSkeletonData(sb, (uint8_t*)ske, ske_length);
		spSkeletonBinary_dispose(sb);
	}
	else
	{
		spSkeletonJson* json = spSkeletonJson_create(c.atlas);
		json->scale = scale;
		skeletonData = spSkeletonJson_readSkeletonData(json, ske);
		spSkeletonJson_dispose(json);
	}
	c.drawable = spSkeletonDrawable_create(skeletonData, 0, true, defaultMix);
	if (c.drawable)
	{
		c.drawable->usePremultipliedAlpha = 0;
		spSkeleton_setToSetupPose(c.drawable->skeleton);
		spSkeletonDrawable_update(c.drawable, 0, SP_PHYSICS_UPDATE);
		drawables.push_back(*((sp_obj_c*)&c));
	}
}

void sp_drawable_ctx::add_pkg(const std::string& pkgfn, float scale, float defaultMix)
{
#ifdef _MFILE_
	{
		hz::mfile_t m;
		auto d = m.open_d(pkgfn, true);
		if (d)
		{
			add_pkg_data(d, m.size(), scale, defaultMix);
		}
	}
#else
	int length = 0;
	auto data = _spUtil_readFile(pkgfn.c_str(), &length);
	if (data)
	{
		add_pkg(d, length, scale, defaultMix);
		_spFree(data);
	}
#endif
}

void sp_drawable_ctx::dispose_sp(size_t idx)
{
	if (idx > drawables.size() || drawables.empty())return;
	auto k = &drawables[idx];
	dispose_spobj((sp_obj*)k);
	auto& v = drawables;
	v.erase(v.begin() + idx);
}

void sp_drawable_ctx::animationstate_set_animationbyname(size_t idx, int trackIndex, const char* animationName, int loop)
{
	if (idx > drawables.size() || drawables.empty())return;
	spSkeletonDrawable* drawable = (spSkeletonDrawable*)drawables[idx].drawable;
	spAnimationState_setAnimationByName(drawable->animationState, trackIndex, animationName, loop);
}

void sp_drawable_ctx::animationstate_add_animationbyname(size_t idx, int trackIndex, const char* animationName, int loop, float delay)
{
	if (idx > drawables.size() || drawables.empty())return;
	spSkeletonDrawable* drawable = (spSkeletonDrawable*)drawables[idx].drawable;
	spAnimationState_addAnimationByName(drawable->animationState, trackIndex, animationName, loop, delay);
}

void sp_drawable_ctx::set_pos(size_t idx, int x, int y)
{
	if (idx > drawables.size() || drawables.empty())return;
	spSkeletonDrawable* drawable = (spSkeletonDrawable*)drawables[idx].drawable;
	drawable->skeleton->x = x;
	drawable->skeleton->y = y;
}

void sp_drawable_ctx::get_anim_name(size_t idx, std::vector<char*>* v)
{
	if (idx > drawables.size() || drawables.empty() || !v)return;
	spSkeletonDrawable* drawable = (spSkeletonDrawable*)drawables[idx].drawable;
	auto as = drawable->animationState;
	auto skd = as->data->skeletonData;
	for (size_t i = 0; i < skd->animationsCount; i++)
	{
		auto it = skd->animations[i];//	spAnimation*
		v->push_back(it->name);
	}
}

void sp_drawable_ctx::update_draw(double deltaTime)
{
	if (!renderer || deltaTime < 0)return;
	for (auto& it : drawables)
	{
		auto drawable = (spSkeletonDrawable*)it.drawable;
		if (it.visible && drawable) {
			spSkeletonDrawable_update(drawable, deltaTime, SP_PHYSICS_UPDATE);
			draw(drawable);
		}
	}
}
#ifndef SDL_BLENDMODE_NONE
#define SDL_BLENDMODE_NONE                  0x00000000u /**< no blending: dstRGBA = srcRGBA */
#define SDL_BLENDMODE_BLEND                 0x00000001u /**< alpha blending: dstRGB = (srcRGB * srcA) + (dstRGB * (1-srcA)), dstA = srcA + (dstA * (1-srcA)) */
#define SDL_BLENDMODE_BLEND_PREMULTIPLIED   0x00000010u /**< pre-multiplied alpha blending: dstRGBA = srcRGBA + (dstRGBA * (1-srcA)) */
#define SDL_BLENDMODE_ADD                   0x00000002u /**< additive blending: dstRGB = (srcRGB * srcA) + dstRGB, dstA = dstA */
#define SDL_BLENDMODE_ADD_PREMULTIPLIED     0x00000020u /**< pre-multiplied additive blending: dstRGB = srcRGB + dstRGB, dstA = dstA */
#define SDL_BLENDMODE_MOD                   0x00000004u /**< color modulate: dstRGB = srcRGB * dstRGB, dstA = dstA */
#define SDL_BLENDMODE_MUL                   0x00000008u /**< color multiply: dstRGB = (srcRGB * dstRGB) + (dstRGB * (1-srcA)), dstA = dstA */
#endif
void sp_drawable_ctx::draw(void* self1)
{
	auto self = (spSkeletonDrawable*)self1;
	static unsigned short quadIndices[] = { 0, 1, 2, 2, 3, 0 };
	spSkeleton* skeleton = self->skeleton;
	spSkeletonClipping* clipper = self->clipper;
	SP_Texture* texture;
	SP_Vertex sdlVertex;
	for (int i = 0; i < skeleton->slotsCount; ++i) {
		spSlot* slot = skeleton->drawOrder[i];
		spAttachment* attachment = slot->attachment;
		if (!attachment) {
			spSkeletonClipping_clipEnd(clipper, slot);
			continue;
		}

		// Early out if the slot color is 0 or the bone is not active
		if (slot->color.a == 0 || !slot->bone->active) {
			spSkeletonClipping_clipEnd(clipper, slot);
			continue;
		}

		spFloatArray* vertices = self->worldVertices;
		int verticesCount = 0;
		float* uvs = NULL;
		unsigned short* indices;
		int indicesCount = 0;
		spColor* attachmentColor = NULL;

		if (attachment->type == SP_ATTACHMENT_REGION) {
			spRegionAttachment* region = (spRegionAttachment*)attachment;
			attachmentColor = &region->color;

			// Early out if the slot color is 0
			if (attachmentColor->a == 0) {
				spSkeletonClipping_clipEnd(clipper, slot);
				continue;
			}

			spFloatArray_setSize(vertices, 8);
			spRegionAttachment_computeWorldVertices(region, slot, vertices->items, 0, 2);
			verticesCount = 4;
			uvs = region->uvs;
			indices = quadIndices;
			indicesCount = 6;
			texture = (SP_Texture*)((spAtlasRegion*)region->rendererObject)->page->rendererObject;
		}
		else if (attachment->type == SP_ATTACHMENT_MESH) {
			spMeshAttachment* mesh = (spMeshAttachment*)attachment;
			attachmentColor = &mesh->color;

			// Early out if the slot color is 0
			if (attachmentColor->a == 0) {
				spSkeletonClipping_clipEnd(clipper, slot);
				continue;
			}

			spFloatArray_setSize(vertices, mesh->super.worldVerticesLength);
			spVertexAttachment_computeWorldVertices(SUPER(mesh), slot, 0, mesh->super.worldVerticesLength, vertices->items, 0, 2);
			verticesCount = mesh->super.worldVerticesLength >> 1;
			uvs = mesh->uvs;
			indices = mesh->triangles;
			indicesCount = mesh->trianglesCount;
			texture = (SP_Texture*)((spAtlasRegion*)mesh->rendererObject)->page->rendererObject;
		}
		else if (attachment->type == SP_ATTACHMENT_CLIPPING) {
			spClippingAttachment* clip = (spClippingAttachment*)slot->attachment;
			spSkeletonClipping_clipStart(clipper, slot, clip);
			continue;
		}
		else
			continue;

		auto r = std::clamp(skeleton->color.r * slot->color.r * attachmentColor->r, 0.0f, 1.0f);
		auto g = std::clamp(skeleton->color.g * slot->color.g * attachmentColor->g, 0.0f, 1.0f);
		auto b = std::clamp(skeleton->color.b * slot->color.b * attachmentColor->b, 0.0f, 1.0f);
		auto a = std::clamp(skeleton->color.a * slot->color.a * attachmentColor->a, 0.0f, 1.0f);
		sdlVertex.color.r = r;
		sdlVertex.color.g = g;
		sdlVertex.color.b = b;
		sdlVertex.color.a = a;

		if (spSkeletonClipping_isClipping(clipper)) {
			spSkeletonClipping_clipTriangles(clipper, vertices->items, verticesCount << 1, indices, indicesCount, uvs, 2);
			vertices = clipper->clippedVertices;
			verticesCount = clipper->clippedVertices->size >> 1;
			uvs = clipper->clippedUVs->items;
			indices = clipper->clippedTriangles->items;
			indicesCount = clipper->clippedTriangles->size;
		}

		spSdlVertexArray_clear(self->sdlVertices);
		for (int ii = 0; ii < verticesCount << 1; ii += 2) {
			sdlVertex.position.x = vertices->items[ii];
			sdlVertex.position.y = vertices->items[ii + 1];
			sdlVertex.tex_coord.x = uvs[ii];
			sdlVertex.tex_coord.y = uvs[ii + 1];
			spSdlVertexArray_add(self->sdlVertices, sdlVertex);
		}
		spIntArray_clear(self->sdlIndices);
		for (int ii = 0; ii < (int)indicesCount; ii++)
			spIntArray_add(self->sdlIndices, indices[ii]);

		//static SDL_BlendMode screen = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR, SDL_BLENDOPERATION_ADD);
		bool pma0 = self->usePremultipliedAlpha == 0;
		switch (slot->data->blendMode) {
		case SP_BLEND_MODE_NORMAL:
			SetTextureBlendMode(texture, pma0 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_BLEND_PREMULTIPLIED);
			break;
		case SP_BLEND_MODE_MULTIPLY:
			SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
			break;
		case SP_BLEND_MODE_ADDITIVE:
			SetTextureBlendMode(texture, pma0 ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_ADD_PREMULTIPLIED);
			break;
		case SP_BLEND_MODE_SCREEN:
			SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);//screen);//
			break;
		}

		//SDL_RenderGeometry(renderer, texture, self->sdlVertices->items, self->sdlVertices->size, self->sdlIndices->items, indicesCount);

		auto pv = self->sdlVertices->items;
		int num_vertices = self->sdlVertices->size;
		const float* xy = &pv->position.x;
		int xy_stride = sizeof(SP_Vertex);
		const SP_color* color = &pv->color;
		int color_stride = sizeof(SP_Vertex);
		const float* uv = &pv->tex_coord.x;
		int uv_stride = sizeof(SP_Vertex);
		int size_indices = 4;
		RenderGeometryRaw(renderer, texture, xy, xy_stride, color, color_stride, uv, uv_stride, num_vertices, self->sdlIndices->items, indicesCount, size_indices);

		spSkeletonClipping_clipEnd(clipper, slot);
	}
	spSkeletonClipping_clipEnd2(clipper);
}

//void sp_drawable::add_pre(const std::string& atlas, const std::string& ske, float scale, float defaultMix)
//{
//	able_t a = { atlas, ske, scale, defaultMix };
//	if (access(atlas.c_str(), 4) + access(ske.c_str(), 4) == 0)
//	{
//		vable.push_back(a);
//	}
//}
// 

void packages_b(const std::string& package_file, std::map<void*, std::string>& texs, int atlas_length, int ske_length, const char* atlas_data, const char* ske_data, bool isbin)
{
	std::vector<std::pair<char*, int>> imgd;
	do {
		if (package_file.empty())break;
		njson0 pk;
		pk["atlas_offset"] = 0;
		pk["atlas_length"] = atlas_length;
		pk["ske_offset"] = atlas_length;
		pk["ske_length"] = ske_length;
		pk["is_binary"] = isbin;
		auto& img = pk["images"];
		int ilen = 0;
		int imgpos = atlas_length + ske_length;
		int64_t imgalen = 0;
		for (auto& [k, v] : texs) {
			njson it;
			it["offset"] = ilen + imgpos;
			auto idata = _spUtil_readFile(v.c_str(), &ilen);
			if (idata)
			{
				std::string pathstr;
				auto cc = strrchr(v.c_str(), '/');
				auto cc1 = strrchr(v.c_str(), '\\');
				if (cc > cc1)
					pathstr = cc + 1;
				else
					pathstr = cc1 + 1;
				if (pathstr.empty())
					pathstr = v.c_str();
				imgalen += ilen;
				it["name"] = pathstr;
				it["length"] = ilen;
				img.push_back(it);
				imgd.push_back({ idata ,ilen });
			}
		}
		int64_t datalen = atlas_length + ske_length + imgalen;
		pk["data_length"] = datalen;
#ifdef _MFILE_
		{
			hz::mfile_t m;
			if (!m.open_m(package_file, false))break;
			auto pkb = njson0::to_cbor(pk);
			int64_t alen = pkb.size() + sizeof(spe_ht) + datalen;
			m.ftruncate_m(alen);//重置文件大小
			auto mpd = m.map(alen, 0);//映射文件
			if (!mpd)break;
			auto pkbs = (spe_ht*)mpd;
			*pkbs = {};
			strcpy(pkbs->n, "spe");
			pkbs->datalen = pkb.size();
			mpd += sizeof(spe_ht);
			memcpy(mpd, pkb.data(), pkb.size());
			mpd += pkbs->datalen;
			memcpy(mpd, atlas_data, atlas_length);
			mpd += atlas_length;
			memcpy(mpd, ske_data, ske_length);
			mpd += ske_length;
			for (auto& [k, v] : imgd) {
				memcpy(mpd, k, v);
				mpd += v;
			}
			m.flush();//刷新数据到磁盘
		}
#endif // !_MFILE_
	} while (0);
}

drawable2d_cx::drawable2d_cx()
{
}

drawable2d_cx::~drawable2d_cx()
{
}


void testdraw(void* renderer)
{

	spAtlas* atlas = spAtlas_createFromFile("data/spineboy-pma.atlas", renderer);
	spSkeletonJson* json = spSkeletonJson_create(atlas);
	json->scale = 0.5f;
	spSkeletonData* skeletonData = spSkeletonJson_readSkeletonDataFile(json, "data/spineboy-pro.json");
	spAnimationStateData* animationStateData = spAnimationStateData_create(skeletonData);
	animationStateData->defaultMix = 0.2f;
	spSkeletonDrawable* drawable = spSkeletonDrawable_create(skeletonData, animationStateData, 1, 0.2f);
	drawable->skeleton->x = 400;
	drawable->skeleton->y = 500;
	spSkeleton_setToSetupPose(drawable->skeleton);
	spSkeletonDrawable_update(drawable, 0, SP_PHYSICS_UPDATE);
	spAnimationState_setAnimationByName(drawable->animationState, 0, "portal", 0);
	spAnimationState_addAnimationByName(drawable->animationState, 0, "run", -1, 0);
#if 0
	int quit = 0;
	uint64_t lastFrameTime = SDL_GetPerformanceCounter();
	while (!quit) {

		uint64_t now = SDL_GetPerformanceCounter();
		double deltaTime = (now - lastFrameTime) / (double)SDL_GetPerformanceFrequency();
		lastFrameTime = now;

		spSkeletonDrawable_update(drawable, deltaTime, SP_PHYSICS_UPDATE);
		spSkeletonDrawable_draw(drawable, (SDL_Renderer*)renderer);
	}
#endif
}


enum INHERIT {
	E_INHERIT_NORMAL,
	E_INHERIT_ONLYTRANSLATION,
	E_INHERIT_NOROTATIONORREFLECTION,
	E_INHERIT_NOSCALE,
	E_INHERIT_NOSCALEORREFLECTION
};

struct BoneData {
	char* name = 0;
	BoneData* parent = 0;
	const char* icon = 0;
	int index = 0;
	float length = 0;
	float x, y, rotation, scaleX, scaleY, shearX, shearY;
	uint32_t color;
	uint8_t inherit;
	bool skinRequired;
	bool visible;
};
enum sBlendMode {
	E_BLEND_MODE_NORMAL, E_BLEND_MODE_ADDITIVE, E_BLEND_MODE_MULTIPLY, E_BLEND_MODE_SCREEN
};
// 可选
struct SlotData {
	int index;
	char* name;
	BoneData* boneData;
	char* attachmentName;
	uint32_t color;
	uint32_t* darkColor;
	int8_t blendMode;
	bool visible;
};
struct IkConstraintData {
	char* name;
	int order;
	int bonesCount;
	BoneData** bones;
	BoneData* target;
	int bendDirection;
	float mix;
	float softness;
	bool skinRequired;
	bool compress;
	bool stretch;
	bool uniform;
};
struct TransformConstraintData {
	char* name;
	int order;
	int bonesCount;
	BoneData** bones;
	BoneData* target;
	float mixRotate, mixX, mixY, mixScaleX, mixScaleY, mixShearY;
	float offsetRotation, offsetX, offsetY, offsetScaleX, offsetScaleY, offsetShearY;
	bool skinRequired;
	bool relative;
	bool local;
};

//typedef enum {
//	E_POSITION_MODE_FIXED, E_POSITION_MODE_PERCENT
//} spPositionMode;
//
//typedef enum {
//	E_SPACING_MODE_LENGTH, E_SPACING_MODE_FIXED, E_SPACING_MODE_PERCENT, E_SPACING_MODE_PROPORTIONAL
//} spSpacingMode;
//
//typedef enum {
//	E_ROTATE_MODE_TANGENT, E_ROTATE_MODE_CHAIN, E_ROTATE_MODE_CHAIN_SCALE
//} spRotateMode;

struct PathConstraintData {
	char* name;
	int order;
	int bonesCount;
	BoneData** bones;
	SlotData* target;
	bool skinRequired;
	int8_t positionMode;
	int8_t spacingMode;
	int8_t rotateMode;
	float offsetRotation;
	float position, spacing;
	float mixRotate, mixX, mixY;
};

struct Skin {
	char* name;
	spBoneDataArray* bones;
	spIkConstraintDataArray* ikConstraints;
	spTransformConstraintDataArray* transformConstraints;
	spPathConstraintDataArray* pathConstraints;
	spPhysicsConstraintDataArray* physicsConstraints;
	uint32_t color;
};

enum AttachmentType {
	E_ATTACHMENT_REGION,//
	E_ATTACHMENT_BOUNDING_BOX,//
	E_ATTACHMENT_MESH,//
	E_ATTACHMENT_LINKED_MESH,
	E_ATTACHMENT_PATH,//
	E_ATTACHMENT_POINT,//
	E_ATTACHMENT_CLIPPING//
};

struct Attachment {
	char* name;
	int type;
	const void* vtable;
	int refCount;
	struct spAttachmentLoader* attachmentLoader;
};
struct RegionAttachment {
	Attachment super;
	char* path;
	float x, y, scaleX, scaleY, rotation, width, height;
	uint32_t color;

	void* rendererObject;
	spTextureRegion* region;
	spSequence* sequence;

	float offset[8];
	float uvs[8];
};
struct VertexAttachment {
	Attachment super;
	int bonesCount;
	int* bones;
	int verticesCount;
	float* vertices;
	int worldVerticesLength;
	Attachment* timelineAttachment;
	int id;
};
struct BoundingBoxAttachment {
	VertexAttachment super;
	uint32_t color;
};
struct MeshAttachment {
	VertexAttachment super;

	void* rendererObject;
	spTextureRegion* region;
	spSequence* sequence;

	char* path;

	float* regionUVs;
	float* uvs;

	int trianglesCount;
	unsigned short* triangles;

	uint32_t color;

	int hullLength;

	MeshAttachment* parentMesh;

	/* Nonessential. */
	int edgesCount;
	unsigned short* edges;
	float width, height;
};
struct PathAttachment {
	VertexAttachment super;
	int lengthsLength;
	float* lengths;
	int/*bool*/ closed, constantSpeed;
	uint32_t color;
};
struct PointAttachment {
	Attachment super;
	float x, y, rotation;
	uint32_t color;
};
struct ClippingAttachment {
	VertexAttachment super;
	SlotData* endSlot;
	uint32_t color;
};


// 皮肤数据

struct SkeletonData {
	char* version;
	char* hash;
	float x, y, width, height;
	float referenceScale;
	float fps;
	const char* imagesPath;
	const char* audioPath;

	int stringsCount;
	char** strings;

	int bonesCount;
	BoneData** bones;

	int slotsCount;
	SlotData** slots;

	int skinsCount;
	Skin** skins;
	Skin* defaultSkin;

	int eventsCount;
	spEventData** events;

	int animationsCount;
	spAnimation** animations;

	int ikConstraintsCount;
	spIkConstraintData** ikConstraints;

	int transformConstraintsCount;
	spTransformConstraintData** transformConstraints;

	int pathConstraintsCount;
	spPathConstraintData** pathConstraints;

	int physicsConstraintsCount;
	spPhysicsConstraintData** physicsConstraints;
};

#endif // !SRC_LIBSRC
