/* 3D布尔运算
	创建时候2024-07-11
*/


// booleanOps
enum class flags_b {
	null,
	A_NOT_B,
	B_NOT_A,
	UNION,
	INTERSECTION
};

class mesh_mx
{
public:
	union vts
	{
		std::vector<glm::vec3>* vf = nullptr;		// 32位
		std::vector<glm::dvec3>* vd;	// 64位
	}vertices;
	int type = 0;	// 0=32,1=64
	std::vector<uint32_t> faces, face_sizes;
	// 顶点数量vertices，面数量 face_sizes
	uint32_t num_faces = 0;
	void* ctx = 0;
public:
	mesh_mx();
	~mesh_mx();
	// 加载STL模型
	void load_stl(const char* path);
	void load_obj(const char* path);
	void set_data(const glm::vec3* v, size_t n, uint32_t* idx, size_t idxnum, int fsize, uint32_t* face_sizes, size_t face_sizes_num);
	void set_data(const glm::dvec3* v, size_t n, uint32_t* idx, size_t idxnum, int fsize, uint32_t* face_sizes, size_t face_sizes_num);
	void begin();
	void dispatch(mesh_mx* cut, flags_b f);
	void end();
private:

};

