
#include <spine/spine.h>

#include <SDL3/SDL.h>
#include "spinesdl3.h"
#ifdef __cplusplus
extern "C" {
#endif

	_SP_ARRAY_DECLARE_TYPE(spSdlVertexArray, struct SDL_Vertex)

		typedef struct spSkeletonDrawable {
		spSkeleton* skeleton;
		spAnimationState* animationState;
		int usePremultipliedAlpha;

		spSkeletonClipping* clipper;
		spFloatArray* worldVertices;
		spSdlVertexArray* sdlVertices;
		spIntArray* sdlIndices;
	} spSkeletonDrawable;

	SP_API spSkeletonDrawable* spSkeletonDrawable_create(spSkeletonData* skeletonData, spAnimationStateData* animationStateData);

	SP_API void spSkeletonDrawable_dispose(spSkeletonDrawable* self);

	SP_API void spSkeletonDrawable_update(spSkeletonDrawable* self, float delta, spPhysics physics);

	SP_API void spSkeletonDrawable_draw(spSkeletonDrawable* self, struct SDL_Renderer* renderer);

#ifdef __cplusplus
}
#endif


#ifndef SRC_LIBSRC

#include <spine/extension.h>


#include <stb_image.h>

_SP_ARRAY_IMPLEMENT_TYPE_NO_CONTAINS(spSdlVertexArray, SDL_Vertex)

spSkeletonDrawable* spSkeletonDrawable_create(spSkeletonData* skeletonData, spAnimationStateData* animationStateData) {
	spBone_setYDown(-1);
	spSkeletonDrawable* self = NEW(spSkeletonDrawable);
	self->skeleton = spSkeleton_create(skeletonData);
	self->animationState = spAnimationState_create(animationStateData);
	self->usePremultipliedAlpha = 0;
	self->sdlIndices = spIntArray_create(12);
	self->sdlVertices = spSdlVertexArray_create(12);
	self->worldVertices = spFloatArray_create(12);
	self->clipper = spSkeletonClipping_create();
	return self;
}

void spSkeletonDrawable_dispose(spSkeletonDrawable* self) {
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

void spSkeletonDrawable_draw(spSkeletonDrawable* self, struct SDL_Renderer* renderer) {
	static unsigned short quadIndices[] = { 0, 1, 2, 2, 3, 0 };
	spSkeleton* skeleton = self->skeleton;
	spSkeletonClipping* clipper = self->clipper;
	SDL_Texture* texture;
	SDL_Vertex sdlVertex;
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
			texture = (SDL_Texture*)((spAtlasRegion*)region->rendererObject)->page->rendererObject;
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
			texture = (SDL_Texture*)((spAtlasRegion*)mesh->rendererObject)->page->rendererObject;
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

		if (!self->usePremultipliedAlpha) {
			switch (slot->data->blendMode) {
			case SP_BLEND_MODE_NORMAL:
				SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
				break;
			case SP_BLEND_MODE_MULTIPLY:
				SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
				break;
			case SP_BLEND_MODE_ADDITIVE:
				SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
				break;
			case SP_BLEND_MODE_SCREEN:
				SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
				break;
			}
		}
		else {
			SDL_BlendMode target;
			switch (slot->data->blendMode) {
			case SP_BLEND_MODE_NORMAL:
				target = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
				SDL_SetTextureBlendMode(texture, target);
				break;
			case SP_BLEND_MODE_MULTIPLY:
				SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
				break;
			case SP_BLEND_MODE_ADDITIVE:
				target = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
				SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
				break;
			case SP_BLEND_MODE_SCREEN:
				target = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
				SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
				break;
			}
		}

		SDL_RenderGeometry(renderer, texture, self->sdlVertices->items, self->sdlVertices->size, self->sdlIndices->items,
			indicesCount);
		spSkeletonClipping_clipEnd(clipper, slot);
	}
	spSkeletonClipping_clipEnd2(clipper);
}

void _spAtlasPage_createTexture(spAtlasPage* self, const char* path) {
	int width, height, components;
	stbi_uc* imageData = stbi_load(path, &width, &height, &components, 4);
	if (!imageData) return;
	SDL_Texture* texture = SDL_CreateTexture((SDL_Renderer*)self->atlas->rendererObject, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, width,
		height);
	if (!texture) {
		stbi_image_free(imageData);
		return;
	}
	if (!SDL_UpdateTexture(texture, NULL, imageData, width * 4)) {
		stbi_image_free(imageData);
		return;
	}
	stbi_image_free(imageData);
	self->rendererObject = texture;
	return;
}

void _spAtlasPage_disposeTexture(spAtlasPage* self) {
	SDL_DestroyTexture((SDL_Texture*)self->rendererObject);
}

char* _spUtil_readFile(const char* path, int* length) {
	return _spReadFile(path, length);
}


struct sp_obj {
	spSkeletonDrawable* drawable = 0;
	spAtlas* atlas = 0;
	spSkeletonData* skeletonData = 0;
	spAnimationStateData* animationStateData = 0;
};
void dispose_spobj(sp_obj* p) {
	spAtlas_dispose(p->atlas);
	spAnimationStateData_dispose(p->animationStateData);
	spSkeletonData_dispose(p->skeletonData);
	spSkeletonDrawable_dispose(p->drawable);
}

sp_drawable::sp_drawable()
{
}

sp_drawable::~sp_drawable()
{
	for (auto& it : drawables) {
		dispose_spobj((sp_obj*)&it);
	}
}

void sp_drawable::set_renderer(void* p)
{
	renderer = p;
}
void sp_drawable::add(const std::string& atlasf, const std::string& ske, float scale, float defaultMix)
{
	if (!renderer)return;
	sp_obj c = {};
	c.atlas = spAtlas_createFromFile(atlasf.c_str(), renderer);
	if (ske.find(".json") != std::string::npos)
	{
		spSkeletonJson* json = spSkeletonJson_create(c.atlas);
		json->scale = scale;
		c.skeletonData = spSkeletonJson_readSkeletonDataFile(json, ske.c_str());
		spSkeletonJson_dispose(json);
	}
	else
	{
		auto sb = spSkeletonBinary_create(c.atlas);
		sb->scale = scale;
		//spSkeletonData* spSkeletonBinary_readSkeletonData(spSkeletonBinary * self, const unsigned char* binary, const int length);
		c.skeletonData = spSkeletonBinary_readSkeletonDataFile(sb, ske.c_str());
		spSkeletonBinary_dispose(sb);
	}
	spAnimationStateData* animationStateData = spAnimationStateData_create(c.skeletonData);
	if (animationStateData) {
		c.animationStateData = animationStateData;
		animationStateData->defaultMix = defaultMix;
		c.drawable = spSkeletonDrawable_create(c.skeletonData, animationStateData);
		if (c.drawable)
		{
			c.drawable->usePremultipliedAlpha = -1;
			spSkeleton_setToSetupPose(c.drawable->skeleton);
			spSkeletonDrawable_update(c.drawable, 0, SP_PHYSICS_UPDATE);
			drawables.push_back(*((sp_obj_c*)&c));
		}
	}
}

void sp_drawable::dispose_sp(size_t idx)
{
	if (idx > drawables.size() || drawables.empty())return;
	auto k = &drawables[idx];
	dispose_spobj((sp_obj*)k);
	auto& v = drawables;
	v.erase(v.begin() + idx);
}

void sp_drawable::animationstate_set_animationbyname(size_t idx, int trackIndex, const char* animationName, int loop)
{
	if (idx > drawables.size() || drawables.empty())return;
	spSkeletonDrawable* drawable = (spSkeletonDrawable*)drawables[idx].drawable;
	spAnimationState_setAnimationByName(drawable->animationState, trackIndex, animationName, loop);
}

void sp_drawable::animationstate_add_animationbyname(size_t idx, int trackIndex, const char* animationName, int loop, float delay)
{
	if (idx > drawables.size() || drawables.empty())return;
	spSkeletonDrawable* drawable = (spSkeletonDrawable*)drawables[idx].drawable;
	spAnimationState_addAnimationByName(drawable->animationState, trackIndex, animationName, loop, delay);
}

void sp_drawable::set_pos(size_t idx, int x, int y)
{
	if (idx > drawables.size() || drawables.empty())return;
	spSkeletonDrawable* drawable = (spSkeletonDrawable*)drawables[idx].drawable;
	drawable->skeleton->x = x;
	drawable->skeleton->y = y;
}

void sp_drawable::get_anim_name(size_t idx, std::vector<char*>* v)
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

void sp_drawable::update_draw(double deltaTime)
{
	if (!renderer || deltaTime < 0)return;
	auto length = drawables.size();
	for (size_t i = 0; i < length; i++)
	{
		auto drawable = (spSkeletonDrawable*)drawables[i].drawable;
		spSkeletonDrawable_update(drawable, deltaTime, SP_PHYSICS_UPDATE);
		spSkeletonDrawable_draw(drawable, (SDL_Renderer*)renderer);
	}
}

void testdraw(void* renderer)
{

	spAtlas* atlas = spAtlas_createFromFile("data/spineboy-pma.atlas", renderer);
	spSkeletonJson* json = spSkeletonJson_create(atlas);
	json->scale = 0.5f;
	spSkeletonData* skeletonData = spSkeletonJson_readSkeletonDataFile(json, "data/spineboy-pro.json");
	spAnimationStateData* animationStateData = spAnimationStateData_create(skeletonData);
	animationStateData->defaultMix = 0.2f;
	spSkeletonDrawable* drawable = spSkeletonDrawable_create(skeletonData, animationStateData);
	drawable->skeleton->x = 400;
	drawable->skeleton->y = 500;
	spSkeleton_setToSetupPose(drawable->skeleton);
	spSkeletonDrawable_update(drawable, 0, SP_PHYSICS_UPDATE);
	spAnimationState_setAnimationByName(drawable->animationState, 0, "portal", 0);
	spAnimationState_addAnimationByName(drawable->animationState, 0, "run", -1, 0);

	int quit = 0;
	uint64_t lastFrameTime = SDL_GetPerformanceCounter();
	while (!quit) {

		uint64_t now = SDL_GetPerformanceCounter();
		double deltaTime = (now - lastFrameTime) / (double)SDL_GetPerformanceFrequency();
		lastFrameTime = now;

		spSkeletonDrawable_update(drawable, deltaTime, SP_PHYSICS_UPDATE);
		spSkeletonDrawable_draw(drawable, (SDL_Renderer*)renderer);


	}
}

#endif // !SRC_LIBSRC
