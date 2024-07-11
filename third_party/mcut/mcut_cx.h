/* 3D布尔运算
	创建时候2024-07-11
*/

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
		std::vector<glm::vec3>* vf;		// 32位
		std::vector<glm::dvec3>* vd;	// 64位
	}vertices;
	int type = 0;	// 0=32,1=64
	std::vector<uint32_t> faces, face_sizes;
	// 顶点数量vertices，面数量 face_sizes

	void* ctx = 0;
public:
	mesh_mx();
	~mesh_mx();
	// 加载STL模型
	void load_stl(const char* path);
	
	void begin();
	void dispatch(mesh_mx* cut, flags_b f);
	void end();
private:

};

