// 3D布尔运算
#include <pch1.h>
#include "mcut_cx.h"

#include "mcut/mcut.h"
#include "stlrw.h"
#include "obj.h"

#if 1
struct McutMesh
{
	// variables for mesh data in a format suited for mcut
	std::vector<uint32_t> faceSizesArray;
	std::vector<uint32_t> faceIndicesArray;
	std::vector<double>   vertexCoordsArray;
};

struct indexed_triangle_set
{
	indexed_triangle_set(std::vector<glm::ivec3>    indices_,
		std::vector<glm::vec3>                     vertices_) :indices(indices_), vertices(vertices_) {
		//properties.resize(indices_.size());
	}
	indexed_triangle_set() {}

	void clear() { indices.clear(); vertices.clear(); /*properties.clear();*/ }

	size_t memsize() const {
		return sizeof(*this) + (sizeof(glm::ivec3) /*+ sizeof(FaceProperty)*/) * indices.size() + sizeof(glm::vec3) * vertices.size();
	}

	std::vector<glm::ivec3>	indices;
	std::vector<glm::vec3>	vertices;
	//std::vector<FaceProperty>	properties;

	bool empty() const { return indices.empty() || vertices.empty(); }
	glm::vec3 get_vertex(int facet_idx, int vertex_idx) const {
		return vertices[indices[facet_idx][vertex_idx]];
	}
	float facet_area(int facet_idx) const {
		auto f0 = get_vertex(facet_idx, 0);
		auto f1 = get_vertex(facet_idx, 1);
		auto f2 = get_vertex(facet_idx, 2);
		float x = glm::abs(glm::normalize(glm::cross(f0 - f1, f0 - f2)).z);
		return x / 2;
	}
	//FaceProperty& get_property(int face_idx) {
	//	if (properties.size() != indices.size()) {
	//		properties.clear();
	//		properties.resize(indices.size());
	//	}
	//	return properties[face_idx];
	//}
};

bool empty(const McutMesh& mesh) { return mesh.vertexCoordsArray.empty() || mesh.faceIndicesArray.empty(); }
void triangle_mesh_to_mcut(const indexed_triangle_set& src_mesh, McutMesh& srcMesh, const glm::mat4& src_nm = glm::identity<glm::mat4>())
{
	// vertices precision convention and copy
	srcMesh.vertexCoordsArray.reserve(src_mesh.vertices.size() * 3);
	for (int i = 0; i < src_mesh.vertices.size(); ++i) {
		const glm::vec3 v = src_nm * glm::vec4(src_mesh.vertices[i], 1.0);
		srcMesh.vertexCoordsArray.push_back(v[0]);
		srcMesh.vertexCoordsArray.push_back(v[1]);
		srcMesh.vertexCoordsArray.push_back(v[2]);
	}

	// faces copy
	srcMesh.faceIndicesArray.reserve(src_mesh.indices.size() * 3);
	srcMesh.faceSizesArray.reserve(src_mesh.indices.size());
	for (int i = 0; i < src_mesh.indices.size(); ++i) {
		const int& f0 = src_mesh.indices[i][0];
		const int& f1 = src_mesh.indices[i][1];
		const int& f2 = src_mesh.indices[i][2];
		srcMesh.faceIndicesArray.push_back(f0);
		srcMesh.faceIndicesArray.push_back(f1);
		srcMesh.faceIndicesArray.push_back(f2);

		srcMesh.faceSizesArray.push_back((uint32_t)3);
	}
}

void stl_generate_shared_vertices(stl3d_cx* stl, indexed_triangle_set& its)
{
	// 3 indices to vertex per face
	auto number_of_facets = stl->faces.size();
	its.indices.assign(number_of_facets, glm::ivec3(-1, -1, -1));
	// Shared vertices (3D coordinates)
	its.vertices.clear();
	its.vertices.reserve(number_of_facets / 2);

	// A degenerate mesh may contain loops: Traversing a fan will end up in an endless loop
	// while never reaching the starting face. To avoid these endless loops, traversed faces at each fan traversal
	// are marked with a unique fan_traversal_stamp.
	unsigned int			  fan_traversal_stamp = 0;
	std::vector<unsigned int> fan_traversal_facet_visited(number_of_facets, 0);

	for (uint32_t facet_idx = 0; facet_idx < number_of_facets; ++facet_idx) {
		for (int j = 0; j < 3; ++j) {
			if (its.indices[facet_idx][j] != -1)
				// Shared vertex was already assigned.
				continue;
			// Create a new shared vertex.
			its.vertices.emplace_back(stl->faces[facet_idx].v[j]);
			// Traverse the fan around the j-th vertex of the i-th face, assign the newly created shared vertex index to all the neighboring triangles in the triangle fan.
			int  facet_in_fan_idx = facet_idx;
			bool edge_direction = false;
			bool traversal_reversed = false;
			int  vnot = (j + 2) % 3;
			// Increase the 
			++fan_traversal_stamp;
			for (;;) {
				// Next edge on facet_in_fan_idx to be traversed. The edge is indexed by its starting vertex index.
				int next_edge = 0;
				// Vertex index in facet_in_fan_idx, which is being pivoted around, and which is being assigned a new shared vertex.
				int pivot_vertex = 0;
				if (vnot > 2) {
					// The edge of facet_in_fan_idx opposite to vnot is equally oriented, therefore
					// the neighboring facet is flipped.
					if (!edge_direction) {
						pivot_vertex = (vnot + 2) % 3;
						next_edge = pivot_vertex;
					}
					else {
						pivot_vertex = (vnot + 1) % 3;
						next_edge = vnot % 3;
					}
					edge_direction = !edge_direction;
				}
				else {
					// The neighboring facet is correctly oriented.
					if (!edge_direction) {
						pivot_vertex = (vnot + 1) % 3;
						next_edge = vnot;
					}
					else {
						pivot_vertex = (vnot + 2) % 3;
						next_edge = pivot_vertex;
					}
				}
				its.indices[facet_in_fan_idx][pivot_vertex] = its.vertices.size() - 1;
				fan_traversal_facet_visited[facet_in_fan_idx] = fan_traversal_stamp;

				// next_edge is an index of the starting vertex of the edge, not an index of the opposite vertex to the edge!
				int next_facet = stl->neighbors_start[facet_in_fan_idx].neighbor[next_edge];
				if (next_facet == -1) {
					// No neighbor going in the current direction.
					if (traversal_reversed) {
						// Went to one limit, then turned back and reached the other limit. Quit the fan traversal.
						break;
					}
					else {
						// Reached the first limit. Now try to reverse and traverse up to the other limit.
						edge_direction = true;
						vnot = (j + 1) % 3;
						traversal_reversed = true;
						facet_in_fan_idx = facet_idx;
					}
				}
				else if (next_facet == facet_idx) {
					// Traversed a closed fan all around.
//					assert(! traversal_reversed);
					break;
				}
				else if (next_facet >= (int)number_of_facets) {
					// The mesh is not valid!
					// assert(false);
					break;
				}
				else if (fan_traversal_facet_visited[next_facet] == fan_traversal_stamp) {
					// Traversed a closed fan all around, but did not reach the starting face.
					// This indicates an invalid geometry (non-manifold).
					//assert(false);
					break;
				}
				else {
					// Continue traversal.
					// next_edge is an index of the starting vertex of the edge, not an index of the opposite vertex to the edge!
					vnot = stl->neighbors_start[facet_in_fan_idx].which_vertex_not[next_edge];
					facet_in_fan_idx = next_facet;
				}
			}
		}
	}
}

template <int DIM, typename Real = double>
class KDTree {
	//  define Point type for convenience
	using Point = glm::vec<DIM, Real>;

	//  define Node type for private operations on the tree. No one should use
	//  this outside KDTree
	class Node {
		Node(int id, int8_t axis = 0) : id_(id), axis_(axis) {}
		~Node() { delete left_; delete right_; }
		Node* left_ = nullptr;
		Node* right_ = nullptr;
		uint32_t id_;
		int8_t axis_;
		friend class KDTree<DIM, Real>;
	};

	using NodePtr = Node*;

	//  pointer to the root node
	Node* root_ = nullptr;

	//  vector of all points; this can dynamically grow or shrink
	//  Note that this is the sinle data structure for storing points data. The
	//  tree has access only to elements of this vector, but there is no point
	//  data in the tree. This simplifies the algorithms and makes the API
	//  easier to use.
	std::vector<Point> data_;

public: // methos
	//  default constructor
	KDTree() = default;

	//  default destructor
	~KDTree() { delete root_; }

	//  delete copy constructor, assignment operator, move constructor, and
	//  move assignment operator. we don't want someone accidentally copies a
	//  search tree
	KDTree(const KDTree&) = delete;
	KDTree(KDTree&&) = delete;
	KDTree& operator=(const KDTree&) = delete;
	KDTree& operator=(KDTree&&) = delete;

	//  insert a new point into the tree
	void insert(const Point& point);
	//  get the current size
	size_t size() { return data_.size(); }

	//  This function is N^2 so it's very inefficient. It's included only for
	//  testing purpose to confirm the correctness of algorithm. Otherwise,
	//  it shouldn't be used in actual code.
	int findNearestBruteForce(const Point& pt);

	//  This function is NlogN, so it should be used in actual code.
	int findNearest(const Point& pt);

	//  return the point from its id
	Point getPoint(int index) {
		return data_[index];
	}

private: // methods
	int findNearest(Node* node, const Point& point, Real& minDist);
	Node* getParentNode(const Point& point) const;
};

template <int DIM, typename Real>
void KDTree<DIM, Real>::insert(const Point& point) {
	uint32_t id = data_.size();
	data_.push_back(point);
	if (!root_) {
		root_ = new Node(0, 0);
	}
	else {
		Node* parent = getParentNode(point);
		if (data_[id][parent->axis_] <= data_[parent->id_][parent->axis_]) {
			parent->left_ = new Node(id, (parent->axis_ + 1) % DIM);
		}
		else {
			parent->right_ = new Node(id, (parent->axis_ + 1) % DIM);
		}
	}
}

float get_magnit_sqr(const glm::vec3& v_)
{
	float r = 0.0;
	for (unsigned i = 0; i < 3; ++i) r += v_[i] * v_[i];
	return r;
}
float get_dist_sqr(const glm::vec3& v_, const glm::vec3& v2)
{
	return get_magnit_sqr(v_ - v2);
}
//  Find the nearest point in the data set to "point".
//  This is the public version of this function. It does two things:
//  1) It finds the potential tree node were "point" to be inserted into tree.
//  2) It traverses the tree once more to find all points that might be closer
//     to point.
//  return value: index of the nearest node
template <int DIM, typename Real>
int KDTree<DIM, Real>::findNearest(const Point& point)
{
	Node* parent = getParentNode(point);
	if (!parent) return -1;
	Real minDist = get_dist_sqr(point, data_[parent->id_]);
	int better = findNearest(root_, point, minDist);
	return (better >= 0) ? better : parent->id_;
}

//  Find the nearest point in the data set to "point"
//  This is the private version of this function. Although it works for
//  arbitrary large values of "minDist", it's only efficient for small values
//  of minDist. For large values of minDist, it behaves like a brute-force
//  search.
template <int DIM, typename Real>
int
KDTree<DIM, Real>::findNearest(Node* node, const Point& point, Real& minDist)
{
	if (!node) return -1;
	Real d = get_dist_sqr(point, data_[node->id_]);

	int result = -1;
	if (d < minDist) {
		result = node->id_;
		minDist = d;
	}

	Real dp = data_[node->id_][node->axis_] - point[node->axis_];
	if (dp * dp < minDist) {
		int pt = findNearest(node->left_, point, minDist);
		if (pt >= 0) result = pt;
		pt = findNearest(node->right_, point, minDist);
		if (pt >= 0) result = pt;
	}
	else if (point[node->axis_] <= data_[node->id_][node->axis_]) {
		int pt = findNearest(node->left_, point, minDist);
		if (pt >= 0) result = pt;
	}
	else {
		int pt = findNearest(node->right_, point, minDist);
		if (pt >= 0) result = pt;
	}
	return result;
}

//  Give a point "point" and a node "node", return the parent node if we were to
//  insert the point into the tree. This is useful because it gives us the
//  initial guess about the nearest point in the tree.

template <int DIM, class Real>
typename KDTree<DIM, Real>::Node*
KDTree<DIM, Real>::getParentNode(const Point& point) const
{
	Node* node = root_;
	Node* parent = nullptr;
	while (node) {
		parent = node;
		node = (point[node->axis_] <= data_[node->id_][node->axis_])
			? node->left_ : node->right_;
	}
	return parent;
}


// This is just a brute force O(n) search. Use only for testing.
template <int DIM, typename Real>
int KDTree<DIM, Real>::findNearestBruteForce(const Point& pt)
{
	int index = -1;
	Real minD2 = std::numeric_limits<Real>::max();
	for (int i = 0; i < data_.size(); i++) {
		Real d2 = Point::get_dist_sqr(pt, data_[i]);
		if (d2 < minD2) {
			minD2 = d2;
			index = i;
		}
	}
	return index;
}
#endif
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
		indexed_triangle_set its = {};
		if (length)
		{
			stl_generate_shared_vertices(&st, its);
			std::vector<glm::vec3> vf[1];
			std::vector<uint32_t> idxs;
			idxs.reserve(length * 3);
			vf->reserve(length * 3);
			auto dt = vf->data();
			auto ft = st.faces.data();
			KDTree<3, float> tree;
			for (size_t i = 0; i < length; i++)
			{
				for (unsigned j = 0; j < 3; j++) {
					unsigned index;
					auto vec = ft[i].v[j];
					int ind = tree.findNearest(vec);
					if ((ind < 0) || (glm::distance(vec, tree.getPoint(ind)) > 1.0e-8)) {
						index = tree.size();
						tree.insert(vec);
						vf->push_back(vec);
					}
					else {
						index = ind;
					}
					idxs.push_back(index);
				}
				//memcpy(dt, &ft[i].v, sizeof(glm::vec3) * 3);
				//dt += 3;
				//face_sizes[i] = 3;
			}
			for (size_t i = 0; i < length * 3; i++)
			{
				//idxs[i] = i;
			}
			//set_data(vf->data(), vf->size(), idxs.data(), idxs.size(), 3, 0, 0);
			set_data(its.vertices.data(), its.vertices.size(), (uint32_t*)its.indices.data(), its.indices.size() * 3, 3, 0, 0);
		}
	}
}

void mesh_mx::load_obj(const char* path)
{
	MioMesh srcMesh = {
	 nullptr, // pVertices
	 nullptr, // pNormals
	 nullptr, // pTexCoords
	 nullptr, // pFaceSizes
	 nullptr, // pFaceVertexIndices
	 nullptr, // pFaceVertexTexCoordIndices
	 nullptr, // pFaceVertexNormalIndices
	 0, // numVertices
	 0, // numNormals
	 0, // numTexCoords
	 0, // numFaces
	};

	mioReadOBJ(path,
		&srcMesh.pVertices,
		&srcMesh.pNormals,
		&srcMesh.pTexCoords,
		&srcMesh.pFaceSizes,
		&srcMesh.pFaceVertexIndices,
		&srcMesh.pFaceVertexTexCoordIndices,
		&srcMesh.pFaceVertexNormalIndices,
		&srcMesh.numVertices,
		&srcMesh.numNormals,
		&srcMesh.numTexCoords,
		&srcMesh.numFaces);
	set_data((glm::dvec3*)srcMesh.pVertices, srcMesh.numVertices, srcMesh.pFaceVertexIndices, srcMesh.numFaces * 3, 3, 0, 0);
	mioFreeMesh(&srcMesh);
}

void mesh_mx::set_data(const glm::vec3* v, size_t n, uint32_t* idx, size_t idxnum, int fsize, uint32_t* pfaces, size_t face_sizes_num)
{
	if (!vertices.vf)
	{
		auto vf = new std::vector<glm::vec3>();		// 32位 
		if (!vf)
		{
			return;
		}
		vertices.vf = vf;
		type = 0;
	}
	if (v && n > 0) {
		auto vf = vertices.vf;
		vf->resize(n);
		auto dt = vf->data();
		memcpy(dt, v, sizeof(glm::vec3) * n);
		num_faces = n / fsize;
	}

	if (idx && idxnum > 0) {
		faces.resize(idxnum);
		memcpy(faces.data(), idx, sizeof(int) * idxnum);
		num_faces = idxnum / fsize;
	}
	else { faces.clear(); }
	if (pfaces && face_sizes_num)
	{
		face_sizes.resize(face_sizes_num);
		memcpy(face_sizes.data(), pfaces, sizeof(int) * face_sizes_num);
	}
	else { face_sizes.clear(); }
}
void mesh_mx::set_data(const glm::dvec3* v, size_t n, uint32_t* idx, size_t idxnum, int fsize, uint32_t* pfaces, size_t face_sizes_num)
{
	if (!vertices.vd)
	{
		auto vf = new std::vector<glm::dvec3>();		// 32位 
		if (!vf)
		{
			return;
		}
		vertices.vd = vf;
		type = 1;
	}
	if (v && n > 0) {
		auto vf = vertices.vd;
		vf->resize(n);
		auto dt = vf->data();
		memcpy(dt, v, sizeof(glm::dvec3) * n);
		num_faces = n / fsize;
	}

	if (idx && idxnum > 0) {
		faces.resize(idxnum);
		memcpy(faces.data(), idx, sizeof(int) * idxnum);
		num_faces = idxnum / fsize;
	}
	else { faces.clear(); }
	if (pfaces && face_sizes_num)
	{
		face_sizes.resize(face_sizes_num);
		memcpy(face_sizes.data(), pfaces, sizeof(int) * face_sizes_num);
	}
	else { face_sizes.clear(); }
}


void test12() {

	McFloat srcMeshVertices[] = {
		-5, -5, 5,  // vertex 0
		5, -5, 5,   // vertex 1
		5, 5, 5,    // vertex 2
		-5, 5, 5,   // vertex 3
		-5, -5, -5, // vertex 4
		5, -5, -5,  // vertex 5
		5, 5, -5,   // vertex 6
		-5, 5, -5   // vertex 7
	};

	McUint32 srcMeshFaces[] = {
		0, 1, 2, 3, // face 0
		7, 6, 5, 4, // face 1
		1, 5, 6, 2, // face 2
		0, 3, 7, 4, // face 3
		3, 2, 6, 7, // face 4
		4, 5, 1, 0  // face 5
	};

	McUint32 srcMeshFaceSizes[] = { 4, 4, 4, 4, 4, 4 };

	McUint32 srcMeshVertexCount = 8;
	McUint32 srcMeshFaceCount = 6;

	// the cut mesh (a quad formed of two triangles)

	McFloat cutMeshVertices[] = {
		-20, -4, 0, // vertex 0
		0, 20, 20,  // vertex 1
		20, -4, 0,  // vertex 2
		0, 20, -20  // vertex 3
	};

	McUint32 cutMeshFaces[] = {
		0, 1, 2, // face 0
		0, 2, 3  // face 1
	};

	// McUint32 cutMeshFaceSizes[] = { 3, 3};

	McUint32 cutMeshVertexCount = 4;
	McUint32 cutMeshFaceCount = 2;

	//
	// create a context
	// 

	McContext context = MC_NULL_HANDLE;
	McResult status = mcCreateContext(&context, MC_NULL_HANDLE);

	assert(status == MC_NO_ERROR);

	//
	// do the cutting
	// 

	status = mcDispatch(
		context,
		MC_DISPATCH_VERTEX_ARRAY_FLOAT,
		srcMeshVertices,
		srcMeshFaces,
		srcMeshFaceSizes,
		srcMeshVertexCount,
		srcMeshFaceCount,
		cutMeshVertices,
		cutMeshFaces,
		nullptr, // cutMeshFaceSizes, // no need to give 'cutMeshFaceSizes' parameter since the cut-mesh is a triangle mesh
		cutMeshVertexCount,
		cutMeshFaceCount);

	assert(status == MC_NO_ERROR);
}
void mesh_mx::begin()
{
	McContext context = MC_NULL_HANDLE;
	McResult status = mcCreateContext(&context, MC_NULL_HANDLE);
	ctx = context;
	assert(status == MC_NO_ERROR);
}
void mesh_mx::dispatch(mesh_mx* cut, flags_b f)
{
	if (!cut || cut->type != type || !vertices.vd || !cut->vertices.vd)return;
	McContext context = (McContext)ctx;

	uint32_t flags = MC_DISPATCH_ENFORCE_GENERAL_POSITION;// type ? MC_DISPATCH_VERTEX_ARRAY_DOUBLE : MC_DISPATCH_VERTEX_ARRAY_FLOAT;
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
	auto srcMeshVertices = vertices.vf->data();
	auto srcMeshFaces = faces.size() ? faces.data() : nullptr;
	auto srcMeshFaceSizes = face_sizes.size() ? face_sizes.data() : nullptr;
	auto srcMeshVertexCount = vertices.vf->size();
	auto srcMeshFaceCount = num_faces;
	auto cutMeshVertices = cut->vertices.vf->data();
	auto cutMeshFaces = cut->faces.size() ? cut->faces.data() : nullptr;
	auto cutMeshFaceSizes = cut->face_sizes.size() ? cut->face_sizes.data() : nullptr;
	auto cutMeshVertexCount = cut->vertices.vf->size();
	auto cutMeshFaceCount = cut->num_faces;
	if (type) {
		status = mcDispatch(
			context, flags | MC_DISPATCH_VERTEX_ARRAY_DOUBLE,
			// source mesh
			vertices.vd->data(),
			faces.data(),
			face_sizes.size() ? face_sizes.data() : nullptr,
			vertices.vd->size(),
			num_faces,
			// cut mesh
			cut->vertices.vd->data(),
			cut->faces.data(),
			cut->face_sizes.size() ? cut->face_sizes.data() : nullptr,
			cut->vertices.vd->size(),
			cut->num_faces);
	}
	else
	{
		status = mcDispatch(
			context, flags | MC_DISPATCH_VERTEX_ARRAY_DOUBLE,
			// source mesh
			vertices.vf->data(),
			faces.data(),
			face_sizes.size() ? face_sizes.data() : nullptr,
			vertices.vf->size(),
			num_faces,
			// cut mesh
			cut->vertices.vf->data(),
			cut->faces.data(),
			cut->face_sizes.size() ? cut->face_sizes.data() : nullptr,
			cut->vertices.vf->size(),
			cut->num_faces);
		//status = mcDispatch(
		//	context,
		//	flags | MC_DISPATCH_VERTEX_ARRAY_FLOAT,
		//	srcMeshVertices,
		//	srcMeshFaces,
		//	srcMeshFaceSizes,
		//	srcMeshVertexCount,
		//	srcMeshFaceCount,
		//	cutMeshVertices,
		//	cutMeshFaces,
		//	cutMeshFaceSizes,	//triangle mesh可以nullptr
		//	cutMeshVertexCount,
		//	cutMeshFaceCount);
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
		std::vector<glm::vec3> ccVertices(ccVertexCount);

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
