// 3D布尔运算
#include <pch1.h>
#include "mcut_cx.h"

#include "mcut/mcut.h"
#include "stlrw.h"
mesh_mx::mesh_mx()
{
}

mesh_mx::~mesh_mx()
{
	if (type == 0)
	{
		if (vertices.vf)delete vertices.vf; vertices.vf = 0;
	}
	else
	{
		if (vertices.vd)delete vertices.vd; vertices.vd = 0;
	}
}
// 加载stl模型
void mesh_mx::load_stl(const char* path)
{
	if (path && *path)
	{
		stl3d_cx st;
		st.load(path);
		auto length = st.faces.size();
		if (length)
		{
			auto vf = new std::vector<glm::vec3>();		// 32位 
			if (!vf)
			{
				return;
			}
			type = 0;
			vf->resize(length * 3);
			auto dt = vf->data();
			auto ft = st.faces.data();
			face_sizes.resize(length);
			faces.resize(length * 3);
			for (size_t i = 0; i < length; i++)
			{
				memcpy(&dt[i * 3], &ft[i].v, sizeof(glm::vec3) * 3);
				face_sizes[i] = 3;
			}
			length *= 3;
			for (size_t i = 0; i < length; i++)
			{
				faces[i] = i;
			}
		}
	}
}

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
	switch (f)
	{
	case flags_b::A_NOT_B:
		flags |= MC_DISPATCH_FILTER_FRAGMENT_SEALING_INSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_ABOVE;
		break;
	case flags_b::B_NOT_A:
		flags |= MC_DISPATCH_FILTER_FRAGMENT_SEALING_OUTSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_BELOW;
		break;
	case flags_b::UNION:
		flags |= MC_DISPATCH_FILTER_FRAGMENT_SEALING_OUTSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_ABOVE;
		break;
	case flags_b::INTERSECTION:
		flags |= MC_DISPATCH_FILTER_FRAGMENT_SEALING_INSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_BELOW;
		break;
	default:
		break;
	}
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
