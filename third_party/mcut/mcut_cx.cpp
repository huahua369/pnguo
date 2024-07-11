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
			vertices.vf = vf;
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
	if (!cut || cut->type != type || !vertices.vd || !cut->vertices.vd)return;
	McContext context = (McContext)ctx;

	uint32_t flags = 0;// type ? MC_DISPATCH_VERTEX_ARRAY_DOUBLE : MC_DISPATCH_VERTEX_ARRAY_FLOAT;
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
	McResult status = {};
	if (type) {
		status = mcDispatch(
			context, flags | MC_DISPATCH_VERTEX_ARRAY_DOUBLE,
			//MC_DISPATCH_VERTEX_ARRAY_DOUBLE | MC_DISPATCH_INCLUDE_VERTEX_MAP | MC_DISPATCH_INCLUDE_FACE_MAP, // We need vertex and face maps to propagate normals
			// source mesh
			vertices.vd->data(),
			faces.data(),
			face_sizes.data(),
			vertices.vd->size(),
			face_sizes.size(),
			// cut mesh
			cut->vertices.vd->data(),
			cut->faces.data(),
			cut->face_sizes.data(),
			cut->vertices.vd->size(),
			cut->face_sizes.size());
	}
	else
	{
		status = mcDispatch(
			context, flags | MC_DISPATCH_VERTEX_ARRAY_FLOAT,
			//MC_DISPATCH_VERTEX_ARRAY_DOUBLE | MC_DISPATCH_INCLUDE_VERTEX_MAP | MC_DISPATCH_INCLUDE_FACE_MAP, // We need vertex and face maps to propagate normals
			// source mesh
			vertices.vf->data(),
			faces.data(),
			face_sizes.data(),
			vertices.vf->size(),
			face_sizes.size(),
			// cut mesh
			cut->vertices.vf->data(),
			cut->faces.data(),
			cut->face_sizes.data(),
			cut->vertices.vf->size(),
			cut->face_sizes.size());
	}

	assert(status == MC_NO_ERROR);
	//
  // query the number of available connected components after the cut
  // 

	McUint32 connectedComponentCount;
	std::vector<McConnectedComponent> connectedComponents;

	status = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_ALL, 0, NULL, &connectedComponentCount);

	assert(status == MC_NO_ERROR);

	if (connectedComponentCount == 0) {
		fprintf(stdout, "no connected components found\n");
		exit(EXIT_FAILURE);
	}

	connectedComponents.resize(connectedComponentCount); // allocate for the amount we want to get

	status = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_ALL, (McUint32)connectedComponents.size(), connectedComponents.data(), NULL);

	assert(status == MC_NO_ERROR);

	//
	//  query the data of each connected component 
	// 

	for (McInt32 i = 0; i < (McInt32)connectedComponents.size(); ++i) {

		McConnectedComponent cc = connectedComponents[i]; // connected compoenent id
		McSize numBytes = 0;

		//
		// vertices
		// 

		status = mcGetConnectedComponentData(context, cc, MC_CONNECTED_COMPONENT_DATA_VERTEX_FLOAT, 0, NULL, &numBytes);

		assert(status == MC_NO_ERROR);

		McUint32 ccVertexCount = (McUint32)(numBytes / (sizeof(McFloat) * 3ull));
		std::vector<McFloat> ccVertices(ccVertexCount * 3u);

		status = mcGetConnectedComponentData(context, cc, MC_CONNECTED_COMPONENT_DATA_VERTEX_FLOAT, numBytes, (McVoid*)ccVertices.data(), NULL);

		assert(status == MC_NO_ERROR);

		//
		// faces
		// 

		numBytes = 0;

		status = mcGetConnectedComponentData(context, cc, MC_CONNECTED_COMPONENT_DATA_FACE, 0, NULL, &numBytes);

		assert(status == MC_NO_ERROR);

		std::vector<McUint32> ccFaceIndices;
		ccFaceIndices.resize(numBytes / sizeof(McUint32));

		status = mcGetConnectedComponentData(context, cc, MC_CONNECTED_COMPONENT_DATA_FACE, numBytes, (McVoid*)ccFaceIndices.data(), NULL);

		assert(status == MC_NO_ERROR);

		//
		// face sizes (vertices per face)
		// 

		numBytes = 0;

		status = mcGetConnectedComponentData(context, cc, MC_CONNECTED_COMPONENT_DATA_FACE_SIZE, 0, NULL, &numBytes);

		assert(status == MC_NO_ERROR);

		std::vector<McUint32> ccFaceSizes;
		ccFaceSizes.resize(numBytes / sizeof(McUint32));

		status = mcGetConnectedComponentData(context, cc, MC_CONNECTED_COMPONENT_DATA_FACE_SIZE, numBytes, (McVoid*)ccFaceSizes.data(), NULL);

		assert(status == MC_NO_ERROR);

		//
		// save connected component (mesh) to an .stl file
		// 


	}
	status = mcReleaseConnectedComponents(context, 0, NULL);
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
