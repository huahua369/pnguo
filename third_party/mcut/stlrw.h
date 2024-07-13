
#include <string>
#include <vector>
/*
	STL_ASCII 1
	STL_BINARY 2
*/

// 文件固定50字节，结构体52字节
struct stl_face_t
{
	glm::vec3 normal;	// 法向量，可选
	glm::vec3 v[3];		// 3个顶点
	uint16_t at;		// 属性，可选
	void set_pos(const glm::vec3& ps);
};
glm::vec3 normal_v3(glm::vec3* dv);
float normal_v2(glm::vec2* dv);

struct stl_neighbors {
	stl_neighbors() { reset(); }
	void reset() {
		neighbor[0] = -1;
		neighbor[1] = -1;
		neighbor[2] = -1;
		which_vertex_not[0] = -1;
		which_vertex_not[1] = -1;
		which_vertex_not[2] = -1;
	}
	int num_neighbors() const { return 3 - ((this->neighbor[0] == -1) + (this->neighbor[1] == -1) + (this->neighbor[2] == -1)); }

	// Index of a neighbor facet.
	int   neighbor[3];
	// Index of an opposite vertex at the neighbor face.
	char  which_vertex_not[3];
};

class stl3d_cx
{
public:
	std::vector<stl_face_t> faces;
	std::vector<stl_neighbors> neighbors_start;
	// 模型宽高厚
	glm::vec3 size = {};
	glm::vec3 bounding[2] = {};
	glm::vec3 pos = {};
public:
	stl3d_cx();
	stl3d_cx(const std::string& fn);

	~stl3d_cx();
public:
	int add(glm::vec3* p, int n, int resn);
	// 增加三角形，n顶点数量,大于3
	int add(glm::vec3* p, int n, glm::vec3 pos = {});
	int add(glm::vec2* p, int n, glm::vec3 pos = {});
	void add(stl3d_cx* p, glm::vec3 pos = {});
	// 计算法向量
	glm::vec3 normal2(stl_face_t& d);
	void get_bounding();
	void box_scale(const glm::vec3& s);
	void box_scale_i(const glm::ivec3& s);
public:
	// 加载stl文件
	void load(const std::string& fn);
	// 判断是二进制/字符串
	int get_stlFormat(const char* data, int64_t size);
	// 保存 二进制2/字符串1
	int save(const std::string& fn, int fmt = 2);
	int save_ascii(const std::string& fn);
	int save_binary(const std::string& fn);
	int save_binary(std::vector<char>& opt);
	void load_ascii(char* data, int64_t cfs);
	void load_binary(char* d, int64_t cfs);
};
