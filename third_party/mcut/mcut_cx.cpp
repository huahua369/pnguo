// 3D布尔运算
#include <pch1.h>
#include "mcut_cx.h"

#include "mcut/mcut.h"

mesh_mx::mesh_mx()
{
}

mesh_mx::~mesh_mx()
{
}

void mesh_mx::load_stl(const char* path)
{
}
const std::map<std::string, McFlags> booleanOps = {
	{ "A_NOT_B", MC_DISPATCH_FILTER_FRAGMENT_SEALING_INSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_ABOVE },
	{ "B_NOT_A", MC_DISPATCH_FILTER_FRAGMENT_SEALING_OUTSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_BELOW },
	{ "UNION", MC_DISPATCH_FILTER_FRAGMENT_SEALING_OUTSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_ABOVE },
	{ "INTERSECTION", MC_DISPATCH_FILTER_FRAGMENT_SEALING_INSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_BELOW }
};
void mesh_mx::begin()
{
	McContext context = MC_NULL_HANDLE;
	ctx = context;
	McResult status = mcCreateContext(&context, MC_NULL_HANDLE);
	assert(status == MC_NO_ERROR);
}
void mesh_mx::dispatch(mesh_mx* cut, flags_b f)
{
	if (!cut || cut->type != type)return;
	McContext context = (McContext)ctx;

	uint32_t flags = type ? MC_DISPATCH_VERTEX_ARRAY_DOUBLE : MC_DISPATCH_VERTEX_ARRAY_FLOAT;
	flags |= f;
	//
	//  do the cutting
	// 
	status = mcDispatch(
		context, flags,
		//MC_DISPATCH_VERTEX_ARRAY_DOUBLE | MC_DISPATCH_INCLUDE_VERTEX_MAP | MC_DISPATCH_INCLUDE_FACE_MAP, // We need vertex and face maps to propagate normals
		// source mesh
		type ? vertices.vd->data() : vertices.vf->data(),
		faces.data(),
		face_sizes.data(),
		type ? vertices.vd->size() : vertices.vf->size(),
		face_sizes.size(),
		// cut mesh
		type ? cut->vertices.vd->data() : cut->vertices.vf->data(),
		cut->faces.data(),
		cut->face_sizes.data(),
		type ? cut->vertices.vd->size() : cut->vertices.vf->size(),
		cut->face_sizes.size(), );

	assert(status == MC_NO_ERROR);



}

// destroy context
void mesh_mx::end()
{
	if (ctx)
	{
		auto status = mcReleaseContext((McContext)ctx);
		assert(status == MC_NO_ERROR);
	}
}
