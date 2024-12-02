/* 3D布尔运算、三角化
	本文件创建时间2024-07-11
*/

namespace gp {
	void constrained_delaunay_triangulation_v(std::vector<std::vector<glm::vec2>>* paths, std::vector<glm::vec3>& ms, bool pccw, double z);
	void constrained_delaunay_triangulation_v(std::vector<std::vector<glm::vec2>>* paths, std::vector<glm::vec3>& vd, std::vector<glm::ivec3>& idxs, bool pccw, double z);

	void cdt_pt(glm::vec3* pt, int n, std::vector<glm::vec3>& ms, bool pccw);
	void cdt_pt(glm::vec3* pt, int n, std::vector<glm::vec3>* ms, bool pccw);
	void cdt_pt(glm::vec2* pt, int n, std::vector<glm::vec3>* msp, bool pccw);

}
//!gp

// booleanOps
enum class flags_b :uint8_t {
	ALL = 0,		// 所有
	A_NOT_B,		// 差集a-=b
	B_NOT_A,		// 差集b-=a
	UNION,			// 并集
	INTERSECTION	// 交集
};

struct mesh_triangle_cx
{
public:
	enum class eft :uint8_t {
		eNormal,  // normal face
		eSmallOverhang,  // small overhang
		eSmallHole,      // face with small hole
		eExteriorAppearance,  // exterior appearance
		eMaxNumFaceTypes
	};
	struct FaceProperty {
		double	area = 0.0;
		eft		type = eft::eNormal;
	};
	std::vector<glm::ivec3>	indices;
	std::vector<glm::vec3>	vertices;
	std::vector<FaceProperty> properties;
public:
	mesh_triangle_cx(std::vector<glm::ivec3> indices_, std::vector<glm::vec3> vertices_);
	mesh_triangle_cx(std::vector<glm::vec3> vertices_, std::vector<glm::ivec3> indices_);
	mesh_triangle_cx();
	void set_data(const glm::vec3* v, size_t n, uint32_t* idx, size_t idxnum);
	void set_data(const glm::dvec3* v, size_t n, uint32_t* idx, size_t idxnum);
	void clear();

	size_t memsize() const;
	bool empty() const;
	glm::vec3 get_vertex(int facet_idx, int vertex_idx) const;
	float facet_area(int facet_idx) const;
};
struct mmesh_t
{
	// variables for mesh data in a format suited for mcut
	std::vector<uint32_t> faceSizesArray;
	std::vector<uint32_t> faceIndicesArray;
	std::vector<double>   vertexCoordsArray;
};
// 加载stl、obj
mesh_triangle_cx* new_mesh(const char* path);
mesh_triangle_cx* new_mesh_stl(const char* data, size_t size);
mesh_triangle_cx* new_mesh(const glm::vec3* v, size_t n, uint32_t* idx, size_t idxnum);
mesh_triangle_cx* new_mesh(const glm::dvec3* v, size_t n, uint32_t* idx, size_t idxnum);
void free_mesh(mesh_triangle_cx* p);
// type=0二进制，1文本
void mesh_save_stl(mesh_triangle_cx* p, const char* fn, int type = 0);
// mmesh_t*
void mesh_save_stl(const void* src_mesh, const char* fn, int type = 0);
// 合并不相交的模型
void its_merge(mesh_triangle_cx& A, const mesh_triangle_cx& B);

// 布尔运算
void make_boolean(const mesh_triangle_cx* src_mesh, const mesh_triangle_cx* cut_mesh, std::vector<mesh_triangle_cx>& dst_mesh, flags_b boolean_opts);
void make_boolean(const mesh_triangle_cx* src_mesh, const void* cut_mesh, std::vector<mesh_triangle_cx>& dst_mesh, flags_b boolean_opts);
void make_boolean(const void* src_mesh, const mesh_triangle_cx* cut_mesh, std::vector<mesh_triangle_cx>& dst_mesh, flags_b boolean_opts);
void make_boolean(const void* src_mesh, const void* cut_mesh, std::vector<mesh_triangle_cx>& dst_mesh, flags_b boolean_opts);
// 单个
void make_boolean_s(const void* src_mesh, const void* cut_mesh, std::vector<mesh_triangle_cx>& dst_mesh, flags_b boolean_opts);
// 分割成独立模型
std::vector<mesh_triangle_cx> mesh_split(mesh_triangle_cx* srcMesh);
std::vector<mesh_triangle_cx> mesh_split(void* srcMesh);
size_t mesh_split(mesh_triangle_cx* srcMesh, std::vector<mesh_triangle_cx>* opt);
size_t mesh_split(void* srcMesh, std::vector<mesh_triangle_cx>* opt);

template<class T>
size_t add_v(T& src, const T& v) {
	auto ps = src.size();
	src.resize(src.size() + v.size());
	memcpy(src.data() + ps, v.data(), sizeof(v[0]) * v.size());
	return ps;
}
