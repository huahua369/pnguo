
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
	// 保存 二进制0/字符串1
	int save(const std::string& fn, int fmt = 0);
	int save_ascii(const std::string& fn);
	int save_binary(const std::string& fn);
	int save_binary(std::vector<char>& opt);
	void load_ascii(char* data, int64_t cfs);
	void load_binary(char* d, int64_t cfs);
};

class step_cx
{
public:
	void* mfile = nullptr;
	const char* mdata = nullptr;
	std::vector<std::string_view> linev;

	enum class STEP_TYPE
	{
		T_NULL,
		//1. 几何基础
		//这些命令用于定义几何的基本元素，如点、方向、坐标系和单位等。
		CARTESIAN_POINT, // 表示三维空间中的一个点。
		DIRECTION, // 表示一个方向向量。
		AXIS2_PLACEMENT_3D, // 表示三维空间中的一个坐标系。
		VECTOR, // 表示一个向量，包括方向和长度。
		DIMENSIONAL_EXPONENTS, // 表示物理量的量纲指数。
		PLANE_ANGLE_MEASURE_WITH_UNIT, // 表示带有单位的平面角度测量值。
		UNCERTAINTY_MEASURE_WITH_UNIT, // 表示带有单位的不确定性测量值。
		// 几何结构
		//这些命令用于定义几何的拓扑结构，如边、环、壳等。
		VERTEX_POINT, // 表示一个顶点，由点定义。
		LINE, // 表示一条直线，由两个点定义。
		ORIENTED_EDGE, // 表示一个有方向的边。
		EDGE_LOOP, // 表示由一组有向边组成的闭合环。
		FACE_OUTER_BOUND, // 表示面的外边界。
		FACE_BOUND, // 表示面的边界。
		OPEN_SHELL, // 表示一个开放的壳。
		CLOSED_SHELL, // 表示一个闭合的壳。
		EDGE_CURVE, // 表示一条边曲线。
		TRIMMED_CURVE, // 表示一条修剪曲线。
		COMPOSITE_CURVE_SEGMENT, // 表示复合曲线的段。
		COMPOSITE_CURVE, // 表示复合曲线。
		//3. 几何面
		//这些命令用于定义几何面，如平面、圆柱面、球面等。
		PLANE, // 表示一个无限延伸的平面。
		CYLINDRICAL_SURFACE, // 表示一个圆柱面。
		CONICAL_SURFACE, // 表示一个圆锥面。
		TOROIDAL_SURFACE, // 表示一个圆环面。
		SPHERICAL_SURFACE, // 表示一个球面。
		ADVANCED_FACE, // 表示一个复杂的面。
		FACE_SURFACE, // 表示一个面表面。
		SURFACE_OF_LINEAR_EXTRUSION, // 表示线性拉伸生成的表面。
		SURFACE_OF_REVOLUTION, // 表示旋转生成的表面。
		//4. 高级几何表示
		//这些命令用于定义复杂的几何表示，如 B - Rep、B 样条曲线和表面模型等。
		B_SPLINE_CURVE_WITH_KNOTS, // 表示带节点的 B 样条曲线。
		B_SPLINE_SURFACE_WITH_KNOTS, // 表示带节点的 B 样条曲面。
		MANIFOLD_SOLID_BREP, // 表示流形实体边界表示（B - Rep）。
		BREP_WITH_VOIDS, // 表示带空洞的边界表示（B - Rep）。
		MANIFOLD_SURFACE_SHAPE_REPRESENTATION, // 表示流形表面的形状表示。
		GEOMETRICALLY_BOUNDED_SURFACE_SHAPE_REPRESENTATION, // 表示几何有界表面的形状表示。
		ADVANCED_BREP_SHAPE_REPRESENTATION, // 表示高级边界表示（B - Rep）的形状表示。
		SHELL_BASED_SURFACE_MODEL, // 表示基于壳的表面模型。
		//5. 样式和显示
		//这些命令用于定义几何的样式、颜色和显示属性。
		DRAUGHTING_PRE_DEFINED_CURVE_FONT, // 表示预定义的曲线字体。
		CURVE_STYLE, // 表示曲线的样式。
		PRESENTATION_STYLE_ASSIGNMENT, // 表示将样式分配给几何实体。
		STYLED_ITEM, // 表示带有样式的几何实体。
		PRESENTATION_LAYER_ASSIGNMENT, // 表示将几何实体分配到显示层。
		FILL_AREA_STYLE_COLOUR, // 表示填充区域的颜色。
		FILL_AREA_STYLE, // 表示填充区域的样式。
		SURFACE_STYLE_FILL_AREA, // 表示表面填充区域的样式。
		SURFACE_SIDE_STYLE, // 表示表面的侧面样式。
		SURFACE_STYLE_USAGE, // 表示表面样式的使用。
		DRAUGHTING_PRE_DEFINED_COLOUR, // 表示预定义的颜色。
		COLOUR_RGB, // 表示 RGB 颜色。
		//6. 产品定义和上下文
		//这些命令用于定义产品、上下文和审批信息。
		APPLICATION_CONTEXT, // 表示应用程序的上下文。
		APPLICATION_PROTOCOL_DEFINITION, // 表示应用程序协议的定义。
		DESIGN_CONTEXT, // 表示设计上下文。
		MECHANICAL_CONTEXT, // 表示机械上下文。
		PRODUCT, // 表示一个产品。
		PRODUCT_DEFINITION_FORMATION_WITH_SPECIFIED_SOURCE, // 表示产品的定义形成。
		ITEM_DEFINED_TRANSFORMATION, // 表示项目的定义变换。
		CONTEXT_DEPENDENT_SHAPE_REPRESENTATION, // 表示上下文相关的形状表示。
		GEOMETRIC_SET, // 表示几何集合。
		SHAPE_REPRESENTATION_RELATIONSHIP, // 表示形状表示的关系。
		PRODUCT_DEFINITION, // 表示产品的定义。
		PRODUCT_DEFINITION_SHAPE, // 表示产品形状的定义。
		SHAPE_DEFINITION_REPRESENTATION, // 表示形状定义的表示。
		SHAPE_REPRESENTATION, // 表示形状的几何表示。
		NEXT_ASSEMBLY_USAGE_OCCURRENCE, // 表示下一个装配的使用实例。
		//7. 安全性和审批
		//这些命令用于定义安全性和审批信息。
		SECURITY_CLASSIFICATION_LEVEL, // 表示安全分类级别。
		SECURITY_CLASSIFICATION, // 表示安全分类。
		CC_DESIGN_SECURITY_CLASSIFICATION, // 表示设计的安全分类。
		APPROVAL_STATUS, // 表示审批状态。
		APPROVAL, // 表示审批。
		CC_DESIGN_APPROVAL, // 表示设计的审批。
		//8. 时间和人员组织
		//这些命令用于定义时间、日期和人员组织信息。
		CALENDAR_DATE, // 表示日历日期。
		COORDINATED_UNIVERSAL_TIME_OFFSET, // 表示协调世界时的偏移量。
		LOCAL_TIME, // 表示本地时间。
		DATE_AND_TIME, // 表示日期和时间。
		APPROVAL_DATE_TIME, // 表示审批的日期和时间。
		DATE_TIME_ROLE, // 表示日期和时间的角色。
		CC_DESIGN_DATE_AND_TIME_ASSIGNMENT, // 表示设计的日期和时间分配。
		PERSON, // 表示人员。
		ORGANIZATION, // 表示组织。
		PERSON_AND_ORGANIZATION, // 表示人员和组织的关联。
		APPROVAL_ROLE, // 表示审批的角色。
		APPROVAL_PERSON_ORGANIZATION, // 表示审批的人员和组织。
		PERSON_AND_ORGANIZATION_ROLE, // 表示人员和组织的角色。
		CC_DESIGN_PERSON_AND_ORGANIZATION_ASSIGNMENT, // 表示设计的人员和组织分配。
	};

	struct bdata
	{
		STEP_TYPE t;
		std::vector<std::string_view> d;
	};
	std::vector<bdata> g_basic;
	std::vector<bdata> g_struct;
	std::vector<bdata> g_facE;
	std::vector<bdata> g_adv;

public:
	step_cx();
	~step_cx();

	size_t load(const std::string& fn);
private:

};
