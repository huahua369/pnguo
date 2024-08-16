// 3D布尔运算
#include <pch1.h>
#include "mcut_cx.h"

#include "mcut/mcut.h"
#include "stlrw.h"
#include "obj.h"
#include <iostream>
#if 1

bool empty(const mmesh_t& mesh) { return mesh.vertexCoordsArray.empty() || mesh.faceIndicesArray.empty(); }
void triangle_mesh_to_mcut(const mesh_triangle_cx& src_mesh, mmesh_t& srcMesh, const glm::mat4& src_nm = glm::identity<glm::mat4>())
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
mmesh_t triangle_mesh_to_mcut(const mesh_triangle_cx& M)
{
	mmesh_t ot;
	triangle_mesh_to_mcut(M, ot);
	return ot;
}
#if 1

void its_reverse_all_facets(mesh_triangle_cx& its)
{
	for (auto& face : its.indices)
		std::swap(face[0], face[1]);
}

void its_merge(mesh_triangle_cx& A, const mesh_triangle_cx& B)
{
	auto N = int(A.vertices.size());
	auto N_f = A.indices.size();

	A.vertices.insert(A.vertices.end(), B.vertices.begin(), B.vertices.end());
	A.indices.insert(A.indices.end(), B.indices.begin(), B.indices.end());

	for (size_t n = N_f; n < A.indices.size(); n++)
		A.indices[n] += glm::ivec3{ N, N, N };
}

void its_merge(mesh_triangle_cx& A, const std::vector<glm::vec3>& triangles)
{
	const size_t offs = A.vertices.size();
	A.vertices.insert(A.vertices.end(), triangles.begin(), triangles.end());
	A.indices.reserve(A.indices.size() + A.vertices.size() / 3);

	for (int i = int(offs); i < int(A.vertices.size()); i += 3)
		A.indices.emplace_back(i, i + 1, i + 2);
}

template<class T, class O = T>
using IntegerOnly = std::enable_if_t<std::is_integral<T>::value, O>;
template<class T, class I, class... Args> // Arbitrary allocator can be used
IntegerOnly<I, std::vector<T, Args...>> reserve_vector(I capacity)
{
	std::vector<T, Args...> ret;
	if (capacity > I(0)) ret.reserve(size_t(capacity));

	return ret;
}
//void its_merge(mesh_triangle_cx& A, const std::vector<glm::vec3>& triangles)
//{
//	auto trianglesf = reserve_vector<glm::vec3>(triangles.size());
//	for (auto& t : triangles)
//		trianglesf.emplace_back(t );
//
//	its_merge(A, trianglesf);
//}

// Index of a vertex inside triangle_indices.
inline int its_triangle_vertex_index(const glm::ivec3& triangle_indices, int vertex_idx)
{
	return vertex_idx == triangle_indices[0] ? 0 :
		vertex_idx == triangle_indices[1] ? 1 :
		vertex_idx == triangle_indices[2] ? 2 : -1;
}

inline glm::ivec2 its_triangle_edge(const glm::ivec3& triangle_indices, int edge_idx)
{
	int next_edge_idx = (edge_idx == 2) ? 0 : edge_idx + 1;
	return { triangle_indices[edge_idx], triangle_indices[next_edge_idx] };
}

// Index of an edge inside triangle.
inline int its_triangle_edge_index(const glm::ivec3& triangle_indices, const glm::ivec2& triangle_edge)
{
	return triangle_edge.x == triangle_indices[0] && triangle_edge.y == triangle_indices[1] ? 0 :
		triangle_edge.x == triangle_indices[1] && triangle_edge.y == triangle_indices[2] ? 1 :
		triangle_edge.x == triangle_indices[2] && triangle_edge.y == triangle_indices[0] ? 2 : -1;
}

// juedge whether two triangles has the same vertices
inline bool its_triangle_vertex_the_same(const glm::ivec3& triangle_indices_1, const glm::ivec3& triangle_indices_2)
{
	bool ret = false;
	if (triangle_indices_1[0] == triangle_indices_2[0])
	{
		if ((triangle_indices_1[1] == triangle_indices_2[1])
			&& (triangle_indices_1[2] == triangle_indices_2[2]))
			ret = true;
		else if ((triangle_indices_1[1] == triangle_indices_2[2])
			&& (triangle_indices_1[2] == triangle_indices_2[1]))
			ret = true;
	}
	else if (triangle_indices_1[0] == triangle_indices_2[1])
	{
		if ((triangle_indices_1[1] == triangle_indices_2[0])
			&& (triangle_indices_1[2] == triangle_indices_2[2]))
			ret = true;
		else if ((triangle_indices_1[1] == triangle_indices_2[2])
			&& (triangle_indices_1[2] == triangle_indices_2[0]))
			ret = true;
	}
	else if (triangle_indices_1[0] == triangle_indices_2[2])
	{
		if ((triangle_indices_1[1] == triangle_indices_2[0])
			&& (triangle_indices_1[2] == triangle_indices_2[1]))
			ret = true;
		else if ((triangle_indices_1[1] == triangle_indices_2[1])
			&& (triangle_indices_1[2] == triangle_indices_2[0]))
			ret = true;
	}

	return ret;
}

#endif // 1

void stl_generate_shared_vertices(stl3d_cx* stl, mesh_triangle_cx& its)
{
	// 3 indices to vertex per face
	auto number_of_facets = stl->faces.size();
	its.indices.assign(number_of_facets, glm::ivec3(-1, -1, -1));
	its.properties.resize(its.indices.size());
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


mesh_triangle_cx mcut_to_triangle_mesh(const mmesh_t& mcutmesh)
{
	uint32_t ccVertexCount = mcutmesh.vertexCoordsArray.size() / 3;
	auto& ccVertices = mcutmesh.vertexCoordsArray;
	auto& ccFaceIndices = mcutmesh.faceIndicesArray;
	auto& faceSizes = mcutmesh.faceSizesArray;
	uint32_t ccFaceCount = faceSizes.size();
	std::vector<glm::vec3> vertices(ccVertexCount);
	for (uint32_t i = 0; i < ccVertexCount; i++) {
		vertices[i][0] = (float)ccVertices[(uint64_t)i * 3 + 0];
		vertices[i][1] = (float)ccVertices[(uint64_t)i * 3 + 1];
		vertices[i][2] = (float)ccVertices[(uint64_t)i * 3 + 2];
	}

	int64_t faceVertexOffsetBase = 0;
	std::vector<int> poly;

	// 循环每个面
	std::vector<glm::ivec3> faces;
	faces.reserve(ccFaceCount);
	for (uint32_t f = 0; f < ccFaceCount; ++f) {
		int faceSize = faceSizes.at(f);
		// 转成三角面
		if (faceSize == 3 || faceSize == 4) {
			glm::ivec4 ind = {};
			auto vx = std::min(faceSize, 4);
			for (int v = 0; v < faceSize; v++) {
				ind[v] = ccFaceIndices[(uint64_t)faceVertexOffsetBase + v];
			}
			faces.push_back(ind);
			if (faceSize == 4)
			{
				ind.y = ind.z;// 0 1 2 3 多一个三角形0 2 3
				ind.z = ind.w;
				faces.push_back(ind);
			}
		}
		else if (faceSize > 4) {
			poly.resize(faceSize);
			for (int v = 0; v < faceSize; v++) {
				poly[v] = ccFaceIndices[(uint64_t)faceVertexOffsetBase + v];
			}
			int n = faceSize - 2;
			for (size_t i = 0; i < n; i++)
			{
				glm::ivec3 ind = { 0,i + 1,i + 2 };
				faces.push_back(ind);
			}
		}
		faceVertexOffsetBase += faceSize;
	}

	mesh_triangle_cx out(vertices, faces);
	return out;
}
void merge_mcut_meshes(mmesh_t& src, const mmesh_t& cut) {
	auto cs = cut.faceIndicesArray.size();
	auto vs = add_v(src.vertexCoordsArray, cut.vertexCoordsArray) / 3;
	auto ids = add_v(src.faceIndicesArray, cut.faceIndicesArray);
	add_v(src.faceSizesArray, cut.faceSizesArray);
	for (size_t i = 0; i < cs; i++)
	{
		src.faceIndicesArray[ids + i] += vs;
	}
	return;
	mesh_triangle_cx all_its;
	auto tri_src = mcut_to_triangle_mesh(src);
	auto tri_cut = mcut_to_triangle_mesh(cut);
	its_merge(all_its, tri_src);
	its_merge(all_its, tri_cut);
	src = triangle_mesh_to_mcut(all_its);
}
#if 1 
template<class It> class Range
{
	It from, to;
public:

	// The class is ready for range based for loops.
	It begin() const { return from; }
	It end() const { return to; }

	// The iterator type can be obtained this way.
	using iterator = It;
	using value_type = typename std::iterator_traits<It>::value_type;

	Range() = default;
	Range(It b, It e) : from(std::move(b)), to(std::move(e)) {}

	// Some useful container-like methods...
	inline size_t size() const { return std::distance(from, to); }
	inline bool   empty() const { return from == to; }
};

// Index of face indices incident with a vertex index.
struct VertexFaceIndex
{
public:
	using iterator = std::vector<size_t>::const_iterator;

	VertexFaceIndex(const mesh_triangle_cx& its) { this->create(its); }
	VertexFaceIndex() {}

	void create(const mesh_triangle_cx& its);
	void clear() { m_vertex_to_face_start.clear(); m_vertex_faces_all.clear(); }

	// Iterators of face indices incident with the input vertex_id.
	iterator begin(size_t vertex_id) const throw() { return m_vertex_faces_all.begin() + m_vertex_to_face_start[vertex_id]; }
	iterator end(size_t vertex_id) const throw() { return m_vertex_faces_all.begin() + m_vertex_to_face_start[vertex_id + 1]; }
	// Vertex incidence.
	size_t   count(size_t vertex_id) const throw() { return m_vertex_to_face_start[vertex_id + 1] - m_vertex_to_face_start[vertex_id]; }

	const Range<iterator> operator[](size_t vertex_id) const { return { begin(vertex_id), end(vertex_id) }; }

private:
	std::vector<size_t>     m_vertex_to_face_start;
	std::vector<size_t>     m_vertex_faces_all;
};


void VertexFaceIndex::create(const mesh_triangle_cx& its)
{
	m_vertex_to_face_start.assign(its.vertices.size() + 1, 0);
	// 1) Calculate vertex incidence by scatter.
	for (auto& face : its.indices) {
		++m_vertex_to_face_start[face.x + 1];
		++m_vertex_to_face_start[face.y + 1];
		++m_vertex_to_face_start[face.z + 1];
	}
	// 2) Prefix sum to calculate offsets to m_vertex_faces_all.
	for (size_t i = 2; i < m_vertex_to_face_start.size(); ++i)
		m_vertex_to_face_start[i] += m_vertex_to_face_start[i - 1];
	// 3) Scatter indices of faces incident to a vertex into m_vertex_faces_all.
	m_vertex_faces_all.assign(m_vertex_to_face_start.back(), 0);
	for (size_t face_idx = 0; face_idx < its.indices.size(); ++face_idx) {
		auto& face = its.indices[face_idx];
		for (int i = 0; i < 3; ++i)
			m_vertex_faces_all[m_vertex_to_face_start[face[i]]++] = face_idx;
	}
	// 4) The previous loop modified m_vertex_to_face_start. Revert the change.
	for (auto i = int(m_vertex_to_face_start.size()) - 1; i > 0; --i)
		m_vertex_to_face_start[i] = m_vertex_to_face_start[i - 1];
	m_vertex_to_face_start.front() = 0;
}

template<class ExPolicy>
std::vector<glm::ivec3> create_face_neighbors_index(ExPolicy&& ex, const mesh_triangle_cx& its)
{
	const std::vector<glm::ivec3>& indices = its.indices;

	if (indices.empty()) return {};

	assert(!its.vertices.empty());

	auto vertex_triangles = VertexFaceIndex{ its };
	static constexpr int no_value = -1;
	std::vector<glm::ivec3> neighbors(indices.size(),
		glm::ivec3(no_value, no_value, no_value));

	for (int face_idx = 0; face_idx < indices.size(); face_idx++) {
		//execution::for_each(ex, size_t(0), indices.size(),
		//	[&neighbors, &indices, &vertex_triangles](size_t face_idx)		{
		glm::ivec3& neighbor = neighbors[face_idx];
		const glm::ivec3& triangle_indices = indices[face_idx];
		for (int edge_index = 0; edge_index < 3; ++edge_index) {
			// check if done
			int& neighbor_edge = neighbor[edge_index];
			if (neighbor_edge != no_value)
				// This edge already has a neighbor assigned.
				continue;
			glm::ivec2 edge_indices = its_triangle_edge(triangle_indices, edge_index);
			// IMPROVE: use same vector for 2 sides of triangle
			for (const size_t other_face : vertex_triangles[edge_indices[0]]) {
				if (other_face <= face_idx) continue;
				const glm::ivec3& face_indices = indices[other_face];
				int vertex_index = its_triangle_vertex_index(face_indices, edge_indices[1]);
				// NOT Contain second vertex?
				if (vertex_index < 0) continue;
				// Has NOT oposite direction?
				if (edge_indices[0] != face_indices[(vertex_index + 1) % 3]) continue;
				//BBS: if this neighbor has already marked before, skip it
				if (neighbors[other_face][vertex_index] != no_value)
					continue;
				//BBS: the same triangle with opposite direction, also treat it as open edges
				//if (its_triangle_vertex_the_same(face_indices, triangle_indices))
				//    continue;
				neighbor_edge = other_face;
				neighbors[other_face][vertex_index] = face_idx;
				break;
			}
		}
		//}, execution::max_concurrency(ex));
	}
	return neighbors;
}


// Discover connected patches of facets one by one.
template<class NeighborIndex>
struct NeighborVisitor {
	NeighborVisitor(const mesh_triangle_cx& its, const NeighborIndex& neighbor_index) :
		its(its), neighbor_index(neighbor_index) {
		m_visited.assign(its.indices.size(), false);
		m_facestack.reserve(its.indices.size());
	}
	NeighborVisitor(const mesh_triangle_cx& its, NeighborIndex&& aneighbor_index) :
		its(its), neighbor_index(m_neighbor_index_data), m_neighbor_index_data(std::move(aneighbor_index)) {
		m_visited.assign(its.indices.size(), false);
		m_facestack.reserve(its.indices.size());
	}

	template<typename Visitor>
	void visit(Visitor visitor)
	{
		// find the next unvisited facet and push the index
		auto facet = std::find(m_visited.begin() + m_seed, m_visited.end(), false);
		m_seed = facet - m_visited.begin();

		if (facet != m_visited.end()) {
			// Skip this element in the next round.
			auto idx = m_seed++;
			if (!visitor(idx))
				return;
			this->push(idx);
			m_visited[idx] = true;
			while (!m_facestack.empty()) {
				size_t facet_idx = this->pop();
				for (size_t i = 0; i < 3; i++)
				{
					auto& neighbor_idx = neighbor_index[facet_idx][i];
					assert(neighbor_idx < int(m_visited.size()));
					if (neighbor_idx >= 0 && !m_visited[neighbor_idx]) {
						if (!visitor(size_t(neighbor_idx)))
							return;
						m_visited[neighbor_idx] = true;
						this->push(stack_el(neighbor_idx));
					}
				}
			}
		}
	}

	const mesh_triangle_cx& its;
	const NeighborIndex& neighbor_index;

private:
	// If initialized with &&neighbor_index, take the ownership of the data.
	const NeighborIndex          m_neighbor_index_data;

	std::vector<char>            m_visited;

	using                        stack_el = size_t;
	std::vector<stack_el>        m_facestack;
	void                         push(const stack_el& s) { m_facestack.emplace_back(s); }
	stack_el                     pop() { stack_el ret = m_facestack.back(); m_facestack.pop_back(); return ret; }

	// Last face visited.
	size_t                       m_seed{ 0 };
};

template<class Fn>
struct SplitOutputFn {

	Fn fn;

	SplitOutputFn(Fn f) : fn{ std::move(f) } {}

	SplitOutputFn& operator *() { return *this; }
	void           operator=(mesh_triangle_cx&& its) { fn(std::move(its)); }
	void           operator=(mesh_triangle_cx& its) { fn(its); }
	SplitOutputFn& operator++() { return *this; };
};
std::vector<glm::ivec3> get_index(const mesh_triangle_cx& its)
{
	void* ex_tbb = 0;
	return create_face_neighbors_index(ex_tbb, its);
}
// Splits a mesh into multiple meshes when possible.
template<class Its, class OutputIt>
void its_split(const Its& m, OutputIt out_it)
{
	const mesh_triangle_cx& its = m;

	struct VertexConv {
		size_t part_id = std::numeric_limits<size_t>::max();
		size_t vertex_image;
	};
	std::vector<VertexConv> vidx_conv(its.vertices.size());

	NeighborVisitor visitor(its, get_index(m));

	std::vector<size_t> facets;
	for (size_t part_id = 0;; ++part_id) {
		// Collect all faces of the next patch.
		facets.clear();
		visitor.visit([&facets](size_t idx) { facets.emplace_back(idx); return true; });
		if (facets.empty())
			break;

		// Create a new mesh for the part that was just split off.
		mesh_triangle_cx mesh;
		mesh.indices.reserve(facets.size());
		mesh.vertices.reserve(std::min(facets.size() * 3, its.vertices.size()));

		// Assign the facets to the new mesh.
		for (size_t face_id : facets) {
			const auto& face = its.indices[face_id];
			glm::ivec3       new_face;
			for (size_t v = 0; v < 3; ++v) {
				auto vi = face[v];

				if (vidx_conv[vi].part_id != part_id) {
					vidx_conv[vi] = { part_id, mesh.vertices.size() };
					mesh.vertices.emplace_back(its.vertices[size_t(vi)]);
				}

				new_face[v] = vidx_conv[vi].vertex_image;
			}

			mesh.indices.emplace_back(new_face);
		}

		*out_it = std::move(mesh);
		++out_it;
	}
}

template<class Its>
std::vector<mesh_triangle_cx> its_split(const Its& its)
{
	auto ret = reserve_vector<mesh_triangle_cx>(3);
	its_split(its, std::back_inserter(ret));

	return ret;
}

#endif // 1

MCAPI_ATTR void MCAPI_CALL mcDebugOutput(McDebugSource source,
	McDebugType type,
	unsigned int id,
	McDebugSeverity severity,
	size_t length,
	const char* message,
	const void* userParam)
{
	std::cout << std::format("mcut mcDebugOutput message ( %d ): %s ", id, message);

	switch (source) {
	case MC_DEBUG_SOURCE_API:
		std::cout << ("Source: API");
		break;
	case MC_DEBUG_SOURCE_KERNEL:
		std::cout << ("Source: Kernel");
		break;
	}

	switch (type) {
	case MC_DEBUG_TYPE_ERROR:
		std::cout << ("Type: Error");
		break;
	case MC_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		std::cout << ("Type: Deprecated Behaviour");
		break;
	case MC_DEBUG_TYPE_OTHER:
		std::cout << ("Type: Other");
		break;
	}

	switch (severity) {
	case MC_DEBUG_SEVERITY_HIGH:
		std::cout << ("Severity: high");
		break;
	case MC_DEBUG_SEVERITY_MEDIUM:
		std::cout << ("Severity: medium");
		break;
	case MC_DEBUG_SEVERITY_LOW:
		std::cout << ("Severity: low");
		break;
	case MC_DEBUG_SEVERITY_NOTIFICATION:
		std::cout << ("Severity: notification");
		break;
	}
}

uint32_t get_opflags(flags_b f) {
	uint32_t flags = 0;
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
	return flags;
}



bool do_boolean_single(mmesh_t& srcMesh, const mmesh_t& cutMesh, flags_b boolean_opts)
{
	// create context
	McContext context = MC_NULL_HANDLE;
	McResult  err = mcCreateContext(&context, 0);
	// add debug callback according to https://cutdigital.github.io/mcut.site/tutorials/debugging/
	mcDebugMessageCallback(context, mcDebugOutput, nullptr);
	mcDebugMessageControl(
		context,
		MC_DEBUG_SOURCE_ALL,
		MC_DEBUG_TYPE_ERROR,
		MC_DEBUG_SEVERITY_MEDIUM,
		true);
	// We can either let MCUT compute all possible meshes (including patches etc.), or we can
	// constrain the library to compute exactly the boolean op mesh we want. This 'constrained' case
	// is done with the following flags.
	// NOTE#1: you can extend these flags by bitwise ORing with additional flags (see `McDispatchFlags' in mcut.h)
	// NOTE#2: below order of columns MATTERS


	McFlags                                        boolOpFlags = get_opflags(boolean_opts);;

	if (srcMesh.vertexCoordsArray.empty() && (boolean_opts == flags_b::UNION || boolean_opts == flags_b::B_NOT_A)) {
		srcMesh = cutMesh;
		mcReleaseContext(context);
		return true;
	}

	err = mcDispatch(context,
		MC_DISPATCH_VERTEX_ARRAY_DOUBLE |          // vertices are in array of doubles
		MC_DISPATCH_ENFORCE_GENERAL_POSITION | // perturb if necessary
		boolOpFlags,                           // filter flags which specify the type of output we want
		// source mesh
		(srcMesh.vertexCoordsArray.data()), (uint32_t*)(srcMesh.faceIndicesArray.data()),
		srcMesh.faceSizesArray.data(), (uint32_t)(srcMesh.vertexCoordsArray.size() / 3), (uint32_t)(srcMesh.faceSizesArray.size()),
		// cut mesh
		(cutMesh.vertexCoordsArray.data()), cutMesh.faceIndicesArray.data(), cutMesh.faceSizesArray.data(),
		(uint32_t)(cutMesh.vertexCoordsArray.size() / 3), (uint32_t)(cutMesh.faceSizesArray.size()));
	if (err != MC_NO_ERROR) {
		std::cout << "MCUT mcDispatch fails! err=" << err;
		mcReleaseContext(context);
		if (boolean_opts == flags_b::UNION) {
			merge_mcut_meshes(srcMesh, cutMesh);
			return true;
		}
		return false;
	}

	// query the number of available connected component
	uint32_t numConnComps;
	err = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_FRAGMENT, 0, NULL, &numConnComps);
	if (err != MC_NO_ERROR || numConnComps == 0) {
		std::cout << "MCUT mcGetConnectedComponents fails! err=" << err << ", numConnComps" << numConnComps;
		mcReleaseContext(context);
		if (numConnComps == 0 && boolean_opts == flags_b::UNION) {
			merge_mcut_meshes(srcMesh, cutMesh);
			return true;
		}
		return false;
	}

	std::vector<McConnectedComponent> connectedComponents(numConnComps, MC_NULL_HANDLE);
	err = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_FRAGMENT, (uint32_t)connectedComponents.size(), connectedComponents.data(), NULL);

	mmesh_t outMesh;
	int N_vertices = 0;
	// traversal of all connected components
	for (int n = 0; n < numConnComps; ++n) {
		// query the data of each connected component from MCUT
		McConnectedComponent connComp = connectedComponents[n];

		// query the vertices
		McSize numBytes = 0;
		err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE, 0, NULL, &numBytes);
		uint32_t            ccVertexCount = (uint32_t)(numBytes / (sizeof(double) * 3));
		std::vector<double> ccVertices((uint64_t)ccVertexCount * 3u, 0);
		err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE, numBytes, (void*)ccVertices.data(), NULL);

		// query the faces
		numBytes = 0;
		err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION, 0, NULL, &numBytes);
		std::vector<uint32_t> ccFaceIndices(numBytes / sizeof(uint32_t), 0);
		err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION, numBytes, ccFaceIndices.data(), NULL);
		std::vector<uint32_t> faceSizes(ccFaceIndices.size() / 3, 3);

		const uint32_t ccFaceCount = (uint32_t)(faceSizes.size());

		// Here we show, how to know when connected components, pertain particular boolean operations.
		McPatchLocation patchLocation = (McPatchLocation)0;
		err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_PATCH_LOCATION, sizeof(McPatchLocation), &patchLocation, NULL);

		McFragmentLocation fragmentLocation = (McFragmentLocation)0;
		err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FRAGMENT_LOCATION, sizeof(McFragmentLocation), &fragmentLocation, NULL);

		outMesh.vertexCoordsArray.insert(outMesh.vertexCoordsArray.end(), ccVertices.begin(), ccVertices.end());

		// add offset to face index
		for (size_t i = 0; i < ccFaceIndices.size(); i++) {
			ccFaceIndices[i] += N_vertices;
		}

		int faceVertexOffsetBase = 0;

		// for each face in CC
		std::vector<glm::ivec3> faces(ccFaceCount);
		for (uint32_t f = 0; f < ccFaceCount; ++f) {
			bool reverseWindingOrder = (fragmentLocation == MC_FRAGMENT_LOCATION_BELOW) && (patchLocation == MC_PATCH_LOCATION_OUTSIDE);
			int  faceSize = faceSizes.at(f);
			if (reverseWindingOrder) {
				std::vector<uint32_t> faceIndex(faceSize);
				// for each vertex in face
				for (int v = faceSize - 1; v >= 0; v--) { faceIndex[v] = ccFaceIndices[(uint64_t)faceVertexOffsetBase + v]; }
				std::copy(faceIndex.begin(), faceIndex.end(), ccFaceIndices.begin() + faceVertexOffsetBase);
			}
			faceVertexOffsetBase += faceSize;
		}

		outMesh.faceIndicesArray.insert(outMesh.faceIndicesArray.end(), ccFaceIndices.begin(), ccFaceIndices.end());
		outMesh.faceSizesArray.insert(outMesh.faceSizesArray.end(), faceSizes.begin(), faceSizes.end());

		N_vertices += ccVertexCount;
	}

	// free connected component data
	err = mcReleaseConnectedComponents(context, 0, NULL);
	// destroy context
	err = mcReleaseContext(context);

	srcMesh = outMesh;

	return true;
}

void do_boolean(mmesh_t& srcMesh, const mmesh_t& cutMesh, flags_b boolean_opts)
{
	auto tri_src = mcut_to_triangle_mesh(srcMesh);
	std::vector<mesh_triangle_cx> src_parts = its_split(tri_src);

	auto tri_cut = mcut_to_triangle_mesh(cutMesh);
	std::vector<mesh_triangle_cx> cut_parts = its_split(tri_cut);

	if (src_parts.empty() && boolean_opts == flags_b::UNION) {
		srcMesh = cutMesh;
		return;
	}
	if (cut_parts.empty()) return;

	// when src mesh has multiple connected components, mcut refuses to work.
	// But we can force it to work by spliting the src mesh into disconnected components,
	// and do booleans seperately, then merge all the results.
	mesh_triangle_cx all_its;
	if (boolean_opts == flags_b::UNION || boolean_opts == flags_b::A_NOT_B || boolean_opts == flags_b::B_NOT_A) {
		for (size_t i = 0; i < src_parts.size(); i++) {
			auto src_part = triangle_mesh_to_mcut(src_parts[i]);
			for (size_t j = 0; j < cut_parts.size(); j++) {
				auto cut_part = triangle_mesh_to_mcut(cut_parts[j]);
				do_boolean_single(src_part, cut_part, boolean_opts);
			}
			auto tri_part = mcut_to_triangle_mesh(src_part);
			its_merge(all_its, tri_part);
		}
	}
	else if (boolean_opts == flags_b::INTERSECTION) {
		for (size_t i = 0; i < src_parts.size(); i++) {
			for (size_t j = 0; j < cut_parts.size(); j++) {
				auto src_part = triangle_mesh_to_mcut(src_parts[i]);
				auto cut_part = triangle_mesh_to_mcut(cut_parts[j]);
				bool success = do_boolean_single(src_part, cut_part, boolean_opts);
				if (success) {
					auto tri_part = mcut_to_triangle_mesh(src_part);
					its_merge(all_its, tri_part);
				}
			}
		}
	}
	srcMesh = triangle_mesh_to_mcut(all_its);
}

mesh_triangle_cx::mesh_triangle_cx() {	}
void mesh_triangle_cx::set_data(const glm::vec3* v, size_t n, uint32_t* idx, size_t idxnum)
{
	if (v && n && idx && idxnum) {
		size_t xn = idxnum / 3;
		vertices.resize(n);
		indices.resize(xn);
		properties.clear();
		properties.resize(xn);
		xn *= 3;
		memcpy(vertices.data(), v, sizeof(glm::vec3) * n);
		memcpy(indices.data(), idx, sizeof(int) * xn);
	}
}
void mesh_triangle_cx::set_data(const glm::dvec3* v, size_t n, uint32_t* idx, size_t idxnum)
{
	if (v && n && idx && idxnum) {
		size_t xn = idxnum / 3;
		vertices.resize(n);
		indices.resize(xn);
		properties.clear();
		properties.resize(xn);
		xn *= 3;
		auto t = vertices.data();
		for (size_t i = 0; i < n; i++, t++)
		{
			*t = v[i];
		}
		memcpy(indices.data(), idx, sizeof(int) * xn);
	}
}
mesh_triangle_cx::mesh_triangle_cx(std::vector<glm::ivec3> indices_, std::vector<glm::vec3> vertices_) :indices(indices_), vertices(vertices_) {	}

mesh_triangle_cx::mesh_triangle_cx(std::vector<glm::vec3> vertices_, std::vector<glm::ivec3> indices_) : indices(indices_), vertices(vertices_) {	}

void mesh_triangle_cx::clear() { indices.clear(); vertices.clear(); properties.clear(); }

size_t mesh_triangle_cx::memsize() const {
	return sizeof(*this) + (sizeof(glm::ivec3) + sizeof(FaceProperty)) * indices.size() + sizeof(glm::vec3) * vertices.size();
}

bool mesh_triangle_cx::empty() const { return indices.empty() || vertices.empty(); }

glm::vec3 mesh_triangle_cx::get_vertex(int facet_idx, int vertex_idx) const {
	return vertices[indices[facet_idx][vertex_idx]];
}

float mesh_triangle_cx::facet_area(int facet_idx) const {
	auto f0 = get_vertex(facet_idx, 0);
	auto f1 = get_vertex(facet_idx, 1);
	auto f2 = get_vertex(facet_idx, 2);
	float x = glm::abs(glm::normalize(glm::cross(f0 - f1, f0 - f2)).z);
	return x / 2;
}

mesh_triangle_cx* new_mesh(const char* path)
{
	auto p = new mesh_triangle_cx();
	if (path && *path)
	{
		std::string fn = path;
		if (fn.rfind(".stl") != std::string::npos)
		{
			stl3d_cx st;
			st.load(path);
			auto length = st.faces.size();
			if (length > 0)
			{
#if 0
				stl_generate_shared_vertices(&st, *p);
#else
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
				}
				p->set_data(vf->data(), vf->size(), idxs.data(), idxs.size());
#endif // 0 

			}
		}
		else if (fn.rfind(".obj") != std::string::npos) {

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
			p->set_data((glm::dvec3*)srcMesh.pVertices, srcMesh.numVertices, srcMesh.pFaceVertexIndices, srcMesh.numFaces * 3);
			mioFreeMesh(&srcMesh);
		}
	}
	return p;
}

mesh_triangle_cx* new_mesh_stl(const char* data, size_t size)
{
	mesh_triangle_cx* p = 0;
	if (data && size)
	{
		p = new mesh_triangle_cx();
		{
			stl3d_cx st;
			st.load_binary((char*)data, size);
			auto length = st.faces.size();
			if (length > 0)
			{
#if 0
				stl_generate_shared_vertices(&st, *p);
#else
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
				}
				p->set_data(vf->data(), vf->size(), idxs.data(), idxs.size());
#endif // 0 

			}
		}

	}
	return p;
}

mesh_triangle_cx* new_mesh(const glm::vec3* v, size_t n, uint32_t* idx, size_t idxnum)
{
	auto p = new mesh_triangle_cx();
	p->set_data(v, n, idx, idxnum);
	return p;
}

mesh_triangle_cx* new_mesh(const glm::dvec3* v, size_t n, uint32_t* idx, size_t idxnum)
{
	auto p = new mesh_triangle_cx();
	p->set_data(v, n, idx, idxnum);
	return p;
}

void free_mesh(mesh_triangle_cx* p)
{
	if (p)delete p;
}

void mesh_save_stl(mesh_triangle_cx* p, const char* fn, int type)
{
	if (!p || !fn || *fn == 0 || p->empty())return;
	stl3d_cx sc;
	auto length = p->indices.size();
	auto d = p->vertices.data();
	auto t = p->indices.data();
	sc.add(0, 0, length * 3);
	for (size_t i = 0; i < length; i++)
	{
		auto it = t[i];
		glm::vec3 v[3] = { d[it.x],d[it.y],d[it.z] };
		sc.add(v, 3, 0);
	}
	sc.save(fn, type);
}
void mesh_save_stl(const void* src_mesh, const char* fn, int type)
{
	auto p = (mmesh_t*)src_mesh;
	if (!p || !fn || *fn == 0 || p->faceSizesArray.empty() || p->vertexCoordsArray.empty() || p->faceIndicesArray.empty())return;
	auto tm = mcut_to_triangle_mesh(*p);
	mesh_save_stl(&tm, fn, type);
}

void make_boolean(const mesh_triangle_cx* src_mesh, const mesh_triangle_cx* cut_mesh, std::vector<mesh_triangle_cx>& dst_mesh, flags_b boolean_opts)
{
	if (!src_mesh || !cut_mesh)return;
	mmesh_t srcMesh, cutMesh;
	triangle_mesh_to_mcut(*src_mesh, srcMesh);
	triangle_mesh_to_mcut(*cut_mesh, cutMesh);
	do_boolean(srcMesh, cutMesh, boolean_opts);
	mesh_triangle_cx tri_src = mcut_to_triangle_mesh(srcMesh);
	if (!tri_src.empty())
		dst_mesh.push_back(std::move(tri_src));
}

void make_boolean(const mesh_triangle_cx* src_mesh, const void* cut_mesh, std::vector<mesh_triangle_cx>& dst_mesh, flags_b boolean_opts)
{
	if (!src_mesh || !cut_mesh)return;
	mmesh_t srcMesh, cutMesh = *(mmesh_t*)cut_mesh;
	triangle_mesh_to_mcut(*src_mesh, srcMesh);
	do_boolean(srcMesh, cutMesh, boolean_opts);
	mesh_triangle_cx tri_src = mcut_to_triangle_mesh(srcMesh);
	if (!tri_src.empty())
		dst_mesh.push_back(std::move(tri_src));
}

void make_boolean(const void* src_mesh, const void* cut_mesh, std::vector<mesh_triangle_cx>& dst_mesh, flags_b boolean_opts)
{
	if (!src_mesh || !cut_mesh)return;
	mmesh_t srcMesh = *(mmesh_t*)src_mesh, cutMesh = *(mmesh_t*)cut_mesh;
	if (do_boolean_single(srcMesh, cutMesh, boolean_opts))
	{
		mesh_triangle_cx tri_src = mcut_to_triangle_mesh(srcMesh);
		if (!tri_src.empty())
			dst_mesh.push_back(std::move(tri_src));
	}
}
