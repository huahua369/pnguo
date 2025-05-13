/*
* pnguo
	本文件的实现：2D骨骼动画、图集、(基础控件、面板)、字体管理、字体软光栅渲染、flex布局算法、简易路径path_v、样条线、stb加载图片、svg加载、cairo画布输出svg等
	多边形三角化（CDT、割耳法）、模型布尔运算、stl模型读写

	font_rctx、font_t
   字体纹理缓存管理
  layout_text_x

	关系：窗口->面板->控件/图形
	面板持有控件，窗口分发事件给面板。

	组件支持内容：文本、图片、矢量图、基础控件
todo 基础组件
- [ ] 表格视图：
- [ ] 树形视图：
- [ ] 属性视图：
- [ ] 对话框：
- [ ] 大文本编辑：
- [ ] 16进制文件编辑：
*/

#pragma once

#ifndef PNGUO_H
#define PNGUO_H

#include <clipper2/clipper.h> 
using namespace Clipper2Lib;

//typedef struct _PangoContext PangoContext;
struct input_state_t;
struct et_un_t;

#ifndef BIT_INC
#define BIT_INC(x) (1<<x)
#endif

struct font_config_t {
	int id = 0, size = 16;
	uint32_t color = 0xffffffff;
	uint32_t back_color = 0xff000000;
};


namespace md {
	int64_t get_utf8_count(const char* buffer, int64_t len);
	const char* utf8_char_pos(const char* buffer, int64_t pos, uint64_t len);
	const char* utf8_prev_char(const char* str);
	const char* utf8_next_char(const char* p);
	int utf8_pointer_to_offset(const char* str, const char* pos);
	char* utf8_offset_to_pointer(const char* str, int offset);
	uint32_t get_u8_idx(const char* str, int64_t idx);
	const char* get_u8_last(const char* str, uint32_t* codepoint);
	std::string u16to_u8(uint16_t* str, size_t len);
	std::wstring u8to_w(const char* str, size_t len);
}
namespace pg
{
	std::string to_string(double _Val);
	std::string to_string(double _Val, const char* fmt);
	std::string to_string_p(uint32_t _Val);
	std::string to_string_p(uint64_t _Val);
	std::string to_string_hex(uint32_t _Val);
	std::string to_string_hex(uint64_t _Val, int n, const char* x);
	std::string to_string_hex2(uint32_t _Val);
}


/*
图集画布
class canvas_atlas
2D骨骼动画
	数据
		skeleton
		bones
		slots
		ik
		transform
		skins:attachments
		animations
		events
*/
class skeleton_data
{
public:
	struct skeleton_t
	{
		char* hash; char* spine;
		float x, y, width, height;
	};
	typedef enum {
		SP_INHERIT_NORMAL,
		SP_INHERIT_ONLYTRANSLATION,
		SP_INHERIT_NOROTATIONORREFLECTION,
		SP_INHERIT_NOSCALE,
		SP_INHERIT_NOSCALEORREFLECTION
	} spInherit;
	struct BoneData
	{
		int index;
		char* name;
		BoneData* parent;
		float length;
		float x = 0, y = 0, rotation = 0, scaleX = 1, scaleY = 1, shearX = 0, shearY = 0;
		spInherit inherit;//transform该属性决定子骨骼以何种方式继承父骨骼的变换
		int/*bool*/ skinRequired = 0;//skin若为true, 则只有当活动皮肤包含该骨骼时该骨骼才算活动. 若省略则默认为false.
		uint32_t color = 0xFF989898;
		const char* icon;
		int/*bool*/ visible = 1;
	};
public:
	skeleton_data();
	~skeleton_data();

private:

};

// 骨骼数据
struct bone_data_t
{
	int index;
	char* name;
	bone_data_t* parent;
	float length;		// 骨骼长度. 运行时通常不使用骨骼长度属性
	float x, y, rotation, scaleX, scaleY, shearX, shearY;
	uint32_t color;
	int/*bool*/ visible;
};
// 骨骼动画矩阵
struct bone_t {
	bone_data_t* data = 0;
	glm::mat3* m = 0;			// 计算好的动画矩阵animated_mats
	int count = 0;
};
//todo data: Channel/Node/Animation
struct attachment_t
{
	void* tex = 0;			// 纹理，无纹理则显示纯色
	glm::vec2* pos = 0;		// 坐标
	glm::vec2* uvs = 0;		// UV
	glm::vec4* colors = 0;	// 顶点颜色(可选)
	size_t vs = 0;			// 顶点数量
	int* idxs = 0;			// 索引
	size_t is = 0;			// 索引数量
	glm::vec4 color = {};	// 无单独顶点颜色时，skeleton->color * slot->color * attachment->color;
	bool isActive = 0;		// 是否激活
};
struct slot_t
{
	glm::vec4 color = {};			// 动画计算结果颜色
	int blend_mode = 0;				// 混合模式
	attachment_t* attachment = 0;	// 激活的附件 
	bone_t* bone = 0;				// 骨头
	glm::vec4 weights = {};			// 权重
	glm::uvec4 joints = {};			// 骨骼id
};
class component2d_t;
struct skeleton_t
{
	glm::vec4 color = {};
	slot_t* solt_data = 0;
	size_t slots_count = 0;
	attachment_t* att_data = 0;
	size_t att_count = 0;
	component2d_t* anim = 0;		// 动画组件，私有数据结构
	bool usePremultipliedAlpha = false;//纹理预乘
	bool visible = 0;				// 是否可见
	void update(float delta);
	uint32_t active_idx = 0;		// 当前执行的动画
	uint32_t active_aidx = 0;		// 当前执行的动画
	// 切换动画id,或附件号
	void set_active(uint32_t idx, uint32_t aidx);
};
// todo 未实现
skeleton_t* load_skeleton_file(const void* filepath);
skeleton_t* load_skeleton_mem(const void* d, size_t len);
// !end 2d skeleton 


// 线，二阶曲线，三阶曲线
enum class vte_e :uint32_t
{
	e_vmove = 1,
	e_vline,
	e_vcurve,
	e_vcubic
};

// 混合模式
enum class BlendMode_e :int {
	none = -1,
	normal = 0,	// 普通混合
	additive,
	multiply,
	modulate,
	screen
};


// 白色
#ifndef col_white
#define col_white 0xffffffff
//uint32_t color = col_white;			// 混合颜色，默认白色不混合
#endif

struct image_ptr_t;
struct image_rc_t
{
	glm::ivec4 img_rc = {};	// 显示坐标、大小
	glm::ivec4 tex_rc = {};	// 纹理区域  
	uint32_t color = 0;	// 颜色混合/透明度
};
struct image_sliced_t
{
	glm::ivec4 img_rc = {};	// 显示坐标、大小
	glm::ivec4 tex_rc = {};	// 纹理区域
	glm::ivec4 sliced = {};	// 九宫格渲染0则不启用 
	uint32_t color = 0;	// 颜色混合/透明度
};
struct atlas_t;

struct quadratic_v
{
	glm::vec2 p0, p1, p2;
};
struct cubic_v
{
	glm::vec2 p0, p1, p2, p3;	// p1 p2是控制点
};

struct vertex_v2
{
	glm::vec2 position = {};		// 坐标	
	glm::vec2 tex_coord = {};		// 纹理uv
	glm::vec4 color = { 1.0,1.0,1.0,1.0 };	// 顶点颜色
public:
	vertex_v2();
	vertex_v2(glm::vec3 p, glm::vec2 u, uint32_t  c);
	vertex_v2(glm::vec2 p, glm::vec2 u, uint32_t  c);
	vertex_v2(glm::vec2 p, glm::vec2 u, glm::vec4  c);
};
struct tex9Grid {
	glm::vec4 srcrect, dstrect;
	float left_width, right_width, top_height, bottom_height, scale;
};




struct image_rect {
	void* tex = 0;
	glm::vec4 src = {}, dst = {};
	int blend_mode = 0;					// 混合模式	
};
struct image_tiled {
	void* tex = 0;
	glm::vec4 src = {}, dst = {};
	int blend_mode = 0;					// 混合模式	
	float scale = 0.0f;					// Tiled 
};
struct image_9grid {
	void* tex = 0;
	glm::vec4 src = {}, dst = {};
	int blend_mode = 0;					// 混合模式	
	float scale = 0.0f;					// 9Grid
	float left_width = 0.0f, right_width = 0.0f, top_height = 0.0f, bottom_height = 0.0f;
};
class mesh2d_cx
{
public:
	struct tex_rs {
		void* tex = 0;
		glm::vec4 src = {}, dst = {};
		float scale = 0.0f;					// Tiled、9Grid
		float left_width = 0.0f, right_width = 0.0f, top_height = 0.0f, bottom_height = 0.0f;
		int blend_mode = 0;					// 混合模式	
	};
	struct draw_cmd_c
	{
		glm::ivec4 clip_rect = {};
		void* texid = 0;
		uint32_t vtxOffset = 0;
		uint32_t idxOffset = 0;
		uint32_t elemCount = 0;
		uint32_t vCount = 0;
		int blend_mode = 0;				// 混合模式	
	};
	std::vector<draw_cmd_c> cmd_data;	// 渲染命令
	std::vector<vertex_v2> vtxs;		// 顶点数据
	std::vector<int> idxs;				// 索引
public:
	mesh2d_cx();
	virtual ~mesh2d_cx();
	// 添加相同纹理/裁剪区域则自动合批
	void add(std::vector<vertex_v2>& vertex, std::vector<int>& vt_index, void* user_image, const glm::ivec4& clip);
private:

};
class gshadow_cx;
class atlas_cx;
// 图集画布
class canvas_atlas
{
public:
	struct image_rs
	{
		image_ptr_t* img = 0;
		glm::ivec2 size = { -1,-1 };		// *大小	
		glm::ivec4 rect = { 0,0,-1,-1 };	// *区域/坐标大小
		glm::ivec4 sliced = {};				// 九宫格渲染 left top right bottom 
	};
	mesh2d_cx _mesh;

	std::vector<gshadow_cx*> _gs;		// 阴影管理
	std::vector<atlas_cx*> _atlas_cx;	// 显示的图集
	std::vector<atlas_t*> _atlas_t;		// 显示的图集
	std::vector<void* > _texs_t;		// 图集的纹理
	glm::ivec4 viewport = { 0,0,0,0 };  // maxint
	glm::ivec4 _clip_rect = { 0,0,200000,200000 };// 当前裁剪 
	void* _renderer = 0;	// 当前渲染器
	void(*destroy_texture_cb)(void* tex) = 0;//删除纹理回调 
	bool valid = false;
	bool visible = true;
	bool autofree = 0;
public:
	canvas_atlas();
	virtual ~canvas_atlas();
	gshadow_cx* new_gs();
	void add_atlas(atlas_cx* p);	// 添加显示的图集
	void remove_atlas(atlas_cx* p);	// 删除显示的图集
	void add_atlas(atlas_t* p);		// 添加显示的图集
	void remove_atlas(atlas_t* p);	// 删除显示的图集
	// 加载图片
	image_ptr_t* new_image2(const void* file);
	image_ptr_t* new_image2mem(const void* d, size_t s);
	void free_image(image_ptr_t* p);
	// 转换成bgra预乘
	void convert_bgr_multiply(image_ptr_t* p);
	size_t count();
public:
	// 窗口调用 
	void apply();	// 创建顶点啥的
private:
	void add_image(image_rs* r, const glm::vec2& pos, uint32_t color, const glm::ivec4& clip);
	void clear();	//清除渲染数据
	bool nohas_clip(glm::ivec4 a);
	// 创建九宫格顶点
	void make_image_sliced(void* img, const glm::ivec4& a, glm::ivec2 texsize, const glm::ivec4& sliced, const glm::ivec4& rect, uint32_t col, const glm::ivec4& clip);
};

/*
bspline
阶数degree\曲线类型
1直线
2二次曲线 例如圆，抛物线
3三次曲线 最常用的nurbs曲线
5连续度更高的曲线
7连续度极高的曲线
*/
namespace tinyspline {
	class BSpline;
}
class bspline_ct
{
public:
	void* ptr = 0;	// tinyspline::BSpline*
	int dim = 0;	// dimension vec2/3/4
public:
	bspline_ct();
	~bspline_ct();
	tinyspline::BSpline* get();	// 获取原始指针
	// 创建B样条线，返回ptr
	void* new_bspline(glm::vec2* cp, int n, size_t degree = 3);
	void* new_bspline(glm::dvec2* cp, int n, size_t degree = 3);
	void* new_bspline(glm::vec3* cp, int n, size_t degree = 3);
	void* new_bspline(glm::dvec3* cp, int n, size_t degree = 3);
	// nurbs
	void* new_bspline(glm::vec4* cp, int n, size_t degree = 2);
	void* new_bspline(glm::dvec4* cp, int n, size_t degree = 2);
	std::vector<glm::vec2> get_cp2();		// 获取控制点
	std::vector<glm::vec2> sample2(int m);	// 获取细分坐标
	std::vector<glm::vec3> sample3(int m);
	std::vector<glm::vec4> sample4(int m);
private:
	void free_bspline(void* p);

};

// 分割3次贝塞尔曲线
bool tessbc(glm::vec2* c, float t, std::vector<glm::vec2>* opt);


struct dash_t
{
	const double* dashes;
	size_t	      num_dashes;
	double	      offset;
};
struct dashf_t
{
	const float* dashes;
	int			 num_dashes;
	float	     offset;
};
/*
CAIRO_LINE_CAP_BUTT,	0在起点（终点）开始（停止）直线
CAIRO_LINE_CAP_ROUND,	1圆形结尾，圆心为终点
CAIRO_LINE_CAP_SQUARE	2正方形结尾，正方形的中心是终点
CAIRO_LINE_JOIN_MITER,	0尖角
CAIRO_LINE_JOIN_ROUND,	1圆角
CAIRO_LINE_JOIN_BEVEL	2截断
*/
struct style_path_t
{
	glm::vec2 pos = {};
	glm::vec2 scale = { 1.0f,1.0f };
	uint32_t fill_color = 0;
	uint32_t stroke_color = 0xffffffff;
	float line_width = 1.0f;
	dash_t dash = {};
	int8_t cap = 0;
	int8_t join = 0;
	int8_t flip_y = 0;
};




struct vertex_v2f;
struct tinyflatten_t
{
	vertex_v2f* first = 0;
	size_t n = 0;
	int mc; float mlen;
	std::vector<glm::vec2>* flatten;
	float dist = 0;			// 距离剔除
	int angle = 100;		// 优化小于100度的
	bool oldfp = false;
};

struct font_impl;

class svg_cx
{
public:
	void* ptr;
	int width;
	int height;
	double em;
	double ex;
};
// 字体缓存
struct font_item_t;

union ft_key_s;
struct gcolors_t;
struct bitmap_ttinfo;
struct Bitmap_p;
class stb_packer;

//image_ptr_t to_imgptr(tinyimage_cx* p);


typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
typedef struct _PangoLayout      PangoLayout;

class font_t;
class font_rctx;
class font_imp;


struct text_tx
{
	glm::ivec4 rc = {}, trc = {};// 背景矩形，文本矩形
	const char* txt = 0;
	int len = 0;
	int st_index = 0;
};


// 获取粘贴板文本
std::string get_clipboard();
// 设置粘贴板文本
void set_clipboard(const char* str);

#ifndef path_v_dh
#define path_v_dh
#endif // !path_v_dh

struct tinypath_t;
struct cubic_v;
class path_v;

namespace gpv {
	using Path64 = std::vector<glm::i64vec2>;
	using PathD = std::vector<glm::dvec2>;
	using Path = std::vector<glm::vec2>;
	using Paths64 = std::vector<Path64>;
	using Paths = std::vector<Path>;
	using PathsD = std::vector<PathD>;
	// 输入路径，输出细分圆角路径线，ccw=0,1,全部角度-1
	Paths path_round_0(path_v* ptr, int ccw, float radius, int num_segments, int ml, int ds);
	PathsD path_round_1(path_v* ptr, int ccw, float radius, int num_segments, int ml, int ds);
}

std::vector<glm::vec2> get_bezier(const cubic_v* path, size_t n, double m);
int make_grid(const glm::vec2& vsize, const glm::vec3& step_width, glm::vec2 nps, path_v* data);

namespace gp {
	using plane3_t = std::vector<glm::vec3>;

	// 获取圆角弧度a的坐标，圆心x=0,y=0
	glm::vec2 get_circle(float a, float r);
	glm::vec3 get_circle(float a, const glm::vec3& c);

	// 输入三点，角点ptr2，输出圆
	glm::vec3 get_circle3(glm::vec2 pt1, glm::vec2 pt2, glm::vec2 pt3, float radius);
	// 获取点p到圆的两条切线
	glm::vec4 gtan(const glm::vec2& P, const glm::vec3& c);

	struct cmd_plane_t
	{
		path_v* pv = 0;					// 路径，优先级0
		gpv::Paths* fv = 0;				// 自定义折线路径，1
		gpv::PathsD* dv = 0;			// 自定义折线路径，2
		plane3_t* opt = 0;				// *输出三角形
		int type = 2;					// 扩展大小可选类型Square=0, Round=1, Miter=2
		int radius = 0;					// 圆角半径
		glm::ivec2 rccw = {}; 			// 圆角顺序
		int pccw = 0;					// 面的顺序
		int segments = 8;				// 段数
		float segments_len = 0.0;		// 细分长度
		float dist = 0.0;				// 细分裁剪距离
		float thickness = 1.0;			// 厚度
		int radius_a = 150;				// 小于角度执行圆角
		bool is_expand = 0;				// 1扩展（顶点数不一致）、0比例缩放（有自相交问题）
	};

	int get_flatten(tinypath_t* p, int m, float ml, float ds, std::vector<glm::vec2>* flatten);
	// 一壳多孔多边形，输出到ms保存三角形每个点坐标
	void constrained_delaunay_triangulation_v(std::vector<std::vector<glm::vec2>>* paths, std::vector<glm::vec3>& ms, bool pccw, double z);
	// 一壳多孔多边形，输出到vd、idxs索引三角形
	void constrained_delaunay_triangulation_v(std::vector<std::vector<glm::vec2>>* paths, std::vector<glm::vec3>& vd, std::vector<glm::ivec3>& idxs, bool pccw, double z);
	// 单线多边形
	void cdt_pt(glm::vec3* pt, int n, std::vector<glm::vec3>& ms, bool pccw); /*double tolerance,*/
	void cdt_pt(glm::vec3* pt, int n, std::vector<glm::vec3>* ms, bool pccw);
	void cdt_pt(glm::vec2* pt, int n, std::vector<glm::vec3>* msp, bool pccw);

	// 单线面，先扩展后比例
	int build_plane1(cmd_plane_t* c, float expand, float scale, float z);
	// 单面打孔，tr.x=半径，tr.y=间隔
	int build_plane1hole(cmd_plane_t* c, float scale, float z, const glm::vec2& tr, std::vector<glm::vec2>* cshape, std::vector<glm::ivec2>* idx, std::vector<glm::vec3>* vpos);
	// 打孔双路径面
	int build_plane2hole(cmd_plane_t* c, float scale, float expand, float z, const glm::vec2& tr, bool rv, std::vector<glm::vec2>* cshape, std::vector<glm::ivec2>* idx, std::vector<glm::vec3>* vpos);
	// 缩放2裁剪面
	int build_plane2sc(cmd_plane_t* c, float scale, float scale1, float z, bool rv);
	// 双路径面， 比例扩展两条路径成竖面
	int build_plane3(cmd_plane_t* c, const glm::vec2& scale, const glm::vec2& z);
	int build_plane3(cmd_plane_t* c, float scale, const glm::vec2& z);
	// 生成竖面，单线
	int build_plane0(cmd_plane_t* c, const glm::vec2& scale, const glm::vec2& z);

	// 获取细分后的曲线
	struct pv_st {
		path_v* v = 0;
		int count = 0;
	};
	pv_st build_plane3_v(cmd_plane_t* c, const glm::vec2& scale, const glm::vec2& z, const std::string& fn, void (*cb)(path_v* pv, int count, const std::string& fn, bool fy));

	// 命令
	struct dv_cmd_t
	{
		glm::vec2 expand;		// 扩展大小
		glm::ivec2 ccw;			// 圆角顺序
		glm::vec2 plane_z;		// 面高度
		glm::vec2 radius;		// 圆角半径大小 
		glm::ivec2 plane_ccw;	// 面的顺序
		int plane_type;			// 面类型，1是单路径面，2是双路径面
		void set_plane1(float expand, bool ccw, float z, float radius, int v_ccw1, int ptype);
		void set_plane2(float expand, bool ccw, float z, float radius, int v_ccw1);
	};
	// 墙
	struct dv_wall_t
	{
		glm::vec2 expand;
		glm::vec2 height;
		glm::vec2 r;			// 圆角
		glm::ivec2 ccw;			// 圆角顺序
		int plane_ccw = 0;		//0=外墙outer,1=内墙inner
	};
	typedef std::vector<dv_cmd_t> vec_cmd;
	typedef std::vector<dv_wall_t> vec_wall;
	// 上下封闭输入结构
	struct closed_t
	{
		int type = 0;		// 0为顶，1为底
		// 封闭厚度
		float thickness = 0;
		// 台阶 宽、高、厚度、方向(0=上平，1下平)
		glm::vec4 step = {};
	};
	struct mkcustom_dt
	{
		// 壁曲线参数
		int m = 20;
		int ct = 0;
		std::vector<glm::vec2> v, v1;

		// 台阶参数
		glm::vec2 step_expand = {};
		glm::vec2 step_expand0 = {};
		glm::vec4 step = {};				// 上台阶, 端面厚度x，-1则无封面。4个参数则台阶
		glm::vec4 step1 = {};				// 下台阶
		glm::vec2 step2 = {};
	};
	struct base_mv_t
	{
		// 大小，xy宽深度为0则不改，z高度
		glm::vec3 box_size = { 0, 0, 20 };
		// 尺寸缩放比例，默认1不缩放
		glm::vec3 box_scale = { 1.0,1.0,1.0 };
		// 分段数量
		int segments = 12;
		float ds = 1.0;
		// 轮廓厚度默认1
		float thickness = 1.0;
		// 0不倒角, 圆角半径
		int radius = 0;
	};
	// 生成B样条线约束的竖三角面
	glm::vec4 mkcustom(mkcustom_dt* np, glm::vec2 k, base_mv_t& bm, cmd_plane_t* c, const glm::uvec2& bcount = { -1,-1 });

	// 多边形
	struct mesh_mt
	{
		std::vector<uint32_t> face_size;		// 面的边数3\4、多边形
		std::vector<uint32_t> face_indice;		// 索引
		std::vector<double> vertex_coord;		// 顶点坐标
	public:
		// 查找循环边，输入一个边序号，返回边
		std::vector<glm::uvec2> find_edge_loop(uint32_t idx);
		// 查找循环面，输入一个面序号，返回面
		std::vector<glm::uvec4> find_face_loop(uint32_t idx);

		void add_vertex(const glm::dvec3* v, size_t n);
		void add_vertex(const glm::vec3* v, size_t n);
	};
	// 面编辑结构
	struct edit_mesh_t
	{
		struct halfedge_t
		{
			glm::uvec2 indice = {};					//顶点索引，x=开头，y下一点索引
			size_t front_half, back_half, rev_half;	// 前后反半边
			size_t face_idx;						// 面集合序号
		};
		struct face_t {
			glm::uvec2 half[4]; // 半边集合索引，最多支持四边形

		};
		std::map<glm::uvec2, size_t> _m;		// 半边集合，key{点索引，下一点索引}，数据索引
		std::vector<halfedge_t> _data;			// 半边集合数据
		std::vector<face_t> _face;				// 面集合
		mesh_mt* mesh = 0;						// 网格数据
		// 生成半边
		void build_halfedge();
	};
	// 三角形
	struct mesh3_mt
	{
		std::vector<glm::ivec3>	indices;	// 三角形索引
		std::vector<glm::vec3>	vertices;	// 顶点坐标
	};


	// 挤出、倒角
	struct extrude_bevel_t {
		float bevel_width = 0;	// 挤出宽度
		float depth = 0;		// 深度
		float count = 5;		// 分辨率
		float thickness = 1.0;	// 厚度
		glm::ivec2 type = { 0,1 };	//样式  x.0=v，1=U，2=|_|，y=-1倒过来
	};
	struct line_style_t {
		float depth = 0;		// 深度
		float count = 5;		// 分辨率
		float thickness = 1.0;	// 厚度
		float bottom_thickness = 0.0;//封底厚度
		glm::ivec2 type = { 0,1 };	//样式 x.0=v，1=U，2=|_|，y=-1倒过来，
	};
	// 生成3D扩展线模型
	void build_line3d(const glm::vec3& pos1, const glm::vec3& pos2, const glm::ivec2& size, line_style_t* style, mesh_mt* opt);

	// 多边形三角化，输出到3角面网络opt，自动判断洞，，pccw是否置反三角面，type 0=CDT,1=割耳法
	int triangulates(std::vector<glm::vec2>* poly, int n, bool pccw, mesh_mt* opt, int type);

	PathsD path_round(path_v* ptr, int ccw, float radius, int num_segments, int ml, int ds);

}
// todo path_v
class path_v
{
public:
	// 线，二阶曲线，三阶曲线
	enum class vtype_e :uint32_t
	{
		e_vmove = 1,
		e_vline,
		e_vcurve,
		e_vcubic
	};
	struct vertex_t
	{
		// 24字节
		glm::vec2 p, c, c1;
		// 4字节
		vtype_e type;
	};
	struct vertex_tf
	{
		// 24字节
		float x, y, cx, cy, cx1, cy1;
		// 4字节
		vtype_e type;
	};
	struct flatten_t
	{
		path_v::vertex_t* first = 0;
		size_t n = 0;
		int mc; float mlen;
		std::vector<glm::vec2>* flatten;
		float dist = 0;			// 距离剔除
		int angle = 100;		// 优化小于100度的
		bool oldfp = false;
	};
	std::vector<vertex_t> _data;
	// xy最小，zw最大
	glm::vec4 _box = { INT_MAX,INT_MAX,INT_MIN,INT_MIN };
	// 坐标
	glm::vec2 _pos = {};
	uint64_t ud = 0;
	int _baseline = 0;
	wchar_t text = 0;
	int angle = 100;
	bool oldexp = false;
	bool flip_y = false;	//生成模型时翻转y
	bool cbox = false;
public:
	path_v();
	~path_v();
	void swap(path_v& v);
	vertex_t* data();
	// 端点数量
	size_t size();
	// 数据长度
	size_t dsize();
	// 曲线条数
	size_t mcount();
	// 开始一条路径
	void moveTo(const glm::vec2& p);
	// 二次
	void quadraticCurveTo(const glm::vec2& cp0, const glm::vec2& p);
	// 三次
	void bezierCurveTo(const glm::vec2& cp0, const glm::vec2& cp1, const glm::vec2& p);
	void curveTo(const glm::vec2& cp0, const glm::vec2& p);
	void cubicTo(const glm::vec2& cp0, const glm::vec2& cp1, const glm::vec2& p);
	void lineTo(const glm::vec2& p);
	// 闭合一条曲线
	void closePath();

	//#define c_KAPPA90 (0.5522847493f)	// Length proportional to radius of a cubic bezier handle for 90deg arcs.

	void addCircle(const glm::vec2& c, double r);
	void addCircle(const glm::vec3* c, size_t n);
	void addCircle(const glm::vec2* c, size_t n, double r);
	void addEllipse(const glm::vec2& c, const glm::vec2& r);
	void addRect(const glm::vec4& a, const glm::vec2& r);
	void add(const vertex_t& v);
	void set_data(const vertex_t* d, size_t size);
	void set_data(const tinypath_t* d, int count);
	void add_lines(const glm::vec2* d, size_t size, bool isclose = false);
	void add_lines(const glm::dvec2* d, size_t size, bool isclose = false);

	void insert(size_t idx, const vertex_t& v);
	void erase(size_t idx);

	void moveTo(double x, double y);
	void lineTo(double x, double y);
	void curveTo(double x, double y, double cx, double cy, double c1x, double c1y);
	// 转成曲线
	void lineTo2(double x, double y);
	void lineTo2(const glm::vec2& p);
	void cubicBezTo(double cpx1, double cpy1, double cpx2, double cpy2, double x, double y);
	// 获取指定idx的一条线首指针
	vertex_t* getline2(int x, size_t* px);
	size_t get_line_count();
	// 反转指定一条线
	void reverse1(int idx);
	void reverse_all();
	// 设置矩阵
	void set_mat(const glm::mat3& m);
public:
	static glm::vec2 rotate_pos(const glm::vec2& pos, const glm::vec2& center, double angle);
	//向量外积
	static double cross_v2(const glm::vec2& a, const glm::vec2& b);
	// 扩展多边形,会有自相交bug，但顶点数相同
	static void expand_polygon(glm::vec2* polygon, int count, float expand, std::vector<glm::vec2>& ots);

	//type取{Square=0, Round=1, Miter=2}顶数会不同
	static void expand_polygon_c(glm::vec2* polygon, int count, float expand, int type, std::vector<glm::vec2>& ots);
public:
	// 获取整体大小
	glm::vec2 get_size();
	glm::vec4 mkbox();
	glm::vec4 mkbox_npos();
	void add(path_v* p);
	void incpos(const glm::vec2& p);
	void mxfy(double fy);
	// 曲线序号，段数，输出std::vector<glm::vec2/glm::dvec2>，type=0，1是double
	//int get_flatten(size_t idx, size_t count, int m, void* flatten, int type);
	int get_flatten(size_t idx, size_t count, int m, float ml, float ds, std::vector<glm::vec2>* flatten);

	// 获取扩展路径,输出到opt，空则修改自身。顶点数相同，注意自相交
	int get_expand(float width, path_v* opt);

	// type取{Square=0, Round=1, Miter=2} ，不同width会使顶点数不同
	int get_expand_flatten(size_t idx, float width, int segments, float ml, float ds, int type, std::vector<std::vector<glm::vec2>>* ots, std::vector<int>* ccwv);
	int get_expand_flatten(float width, int segments, int type, std::vector<std::vector<glm::vec2>>* iod);

	int get_expand_flatten2(float expand, float scale, int segments, float ml, float ds, int type, std::vector<std::vector<glm::vec2>>* ots, bool is_round, bool is_close = 1);

	int get_expand_flatten3(float expand, float scale, int segments, float ml, float ds, int type, int etype, std::vector<std::vector<glm::vec2>>* ots, bool is_close);

	// 三角化，曲线细分段，是否转置成反面0/1，输出到ms数组。type=0cdt，1=割耳
	int triangulate(int segments, float ml, float ds, bool pccw, std::vector<glm::vec2>* ms, int type = 0);
	// 获取每条边的中心点拉高z
	int get_triangulate_center_line(int segments, float ml, float ds, int is_reverse, const glm::vec2& z, std::vector<glm::vec3>* ms);

	bool is_ccw(int idx);
private:

};
struct inflate_t
{
	int width = 0;	// 
	int segments = 8;
	float mlen = 0;
	float ds = 0;
	int angle = 100;
	int type = 2; //(JoinType)  Square=0, Bevel, Round, Miter
	int etype = 0; //(EndType) Polygon=0, Joined, Butt, Square, Round
	bool is_reverse = false;	// 是否反转
	bool is_close = false;		// 自动闭合
};
int inflate2flatten(path_v* p, path_v* dst, inflate_t* t);

void save_png_v(path_v* pv, int count, const std::string& fn, bool fy, float sc);

class gshadow_cx;
class plane_cx;
class layout_text_x;
struct text_style_t;
struct text_style_tx;
struct vg_style_t;

#ifndef NO_FLEX_CX
/*
	根元素要求
	assert(parent == NULL);
	assert(!isnan(width));
	assert(!isnan(height));
	assert(self_sizing == NULL);

	FLEX_ALIGN_SPACE_BETWEEN,	//两端对齐，两端间隔0，中间间隔1
	FLEX_ALIGN_SPACE_AROUND,	//分散居中,两端间隔0.5，中间间隔1
	FLEX_ALIGN_SPACE_EVENLY,	//分散居中,每个间隔1
*/
class flex_item
{
public:
	enum class flex_align :uint8_t {
		ALIGN_AUTO = 0,
		ALIGN_STRETCH,
		ALIGN_CENTER,
		ALIGN_START,
		ALIGN_END,
		ALIGN_SPACE_BETWEEN,
		ALIGN_SPACE_AROUND,
		ALIGN_SPACE_EVENLY
	};

	enum class flex_position :uint8_t {
		POS_RELATIVE = 0,
		POS_ABSOLUTE
	};

	enum flex_direction :uint8_t {
		ROW = 0,
		ROW_REVERSE,
		COLUMN,
		COLUMN_REVERSE
	};

	enum class flex_wrap :uint8_t {
		NO_WRAP = 0,
		WRAP,
		WRAP_REVERSE
	};

	// size[0] == width, size[1] == height
	typedef void (*flex_self_sizing)(flex_item* item, float* size);
	float width = NAN;
	float height = NAN;

	float left = NAN;			// 左偏移坐标
	float right = NAN;
	float top = NAN;
	float bottom = NAN;

	float padding_left = 0;		// 本元素内边距
	float padding_right = 0;
	float padding_top = 0;
	float padding_bottom = 0;

	float margin_left = 0;		// 本元素外边距
	float margin_right = 0;
	float margin_top = 0;
	float margin_bottom = 0;

	float grow = 0;		// 子元素:自身放大比例，默认为0不放大
	float shrink = 0;	// 子元素:空间不足时自身缩小比例，默认为1自动缩小，0不缩小
	int	  order = 0;	// 子元素:自身排列顺序。数值越小，越靠前
	float basis = NAN;	// 子元素:定义最小空间

	flex_align justify_content = flex_align::ALIGN_START;	// 父元素:主轴上的元素的排列方式 start\end\center\space-between\space-around
	flex_align align_content = flex_align::ALIGN_STRETCH;	// 父元素:适用多行的flex容器 start\end\center\space-between\space-around\stretch 
	flex_align align_items = flex_align::ALIGN_STRETCH;		// 父元素:副轴上的元素的排列方式 start\end\center\stretch 
	flex_align align_self = flex_align::ALIGN_AUTO;			// 子元素:覆盖父容器align-items的设置

	flex_position position = flex_position::POS_RELATIVE;	// 子元素:
	flex_direction direction = flex_direction::ROW;			// 父元素:
	flex_wrap wrap = flex_wrap::WRAP;						// 父元素:是否换行，超出宽度自动换行
	bool should_order_children = false;

	void* managed_ptr = NULL;	// 用户数据指针
	flex_self_sizing self_sizing = NULL; // 运行时计算大小

	float frame[4] = {};	// 输出坐标、大小
	flex_item* parent = 0;	// 父级
	std::vector<flex_item*>* children = 0;	// 子级
public:
	flex_item();
	~flex_item();
	// 指针为空就默认值，80字节
	void set_base(float* size, float* offset4, float* padding4, float* margin4, float* gsb, int order, uint8_t* align_pdw7);
	// 复制除宽高外的属性
	void copy_a(flex_item* p);
	flex_item* init();
	void update_should_order_children();	// 子元素属性改变时执行

	void item_add(flex_item* child);
	void item_insert(uint32_t index, flex_item* child);
	flex_item* item_delete(uint32_t index);
	flex_item* detach(flex_item* child);
	// 清空子元素
	void clear();
	// 执行布局计算
	void layout();
private:
	void layout_items(uint32_t child_begin, uint32_t child_end, uint32_t children_count, struct flex_layout* layout, uint32_t last_count);
	void layout_item(float width, float height);
};


#endif // NO_FLEX_CX

class grid_view
{
public:
	// 行列的宽高
	std::vector<glm::vec2> _column_width;	// 列宽，x宽，y为本列坐标缓存
	std::vector<glm::vec2> _row_height;		// 行高，x高，y为本行坐标缓存
	glm::vec2 _pos = {};
	glm::vec2 _size = {};
	bool valid = true;
public:
	grid_view();
	~grid_view();
	// 设置列行数量
	void set_size(size_t x, size_t y);
	void add_col(int width);
	void add_row(int height);
	// 设置列宽，idx为列索引，v宽度
	void set_width(size_t idx, float v);
	// 设置行高，idx为列索引，v行高
	void set_height(size_t idx, float v);
	// 获取总大小
	glm::vec2 get_size();
	// 获取指定坐标单元格的坐标/大小。
	glm::vec4 get(const glm::ivec2& pos);
private:

};

struct column_lv
{
	std::string title;	// 文本 
	glm::vec2 align = { 0.5,0.5 };	// 0左，0.5中，1右
	int type = 0;		// 类型
	int width = 0;		// 宽
	int idx = 0;		// 初始序号
	bool visible = true;//是否显示列
};
/*
switch_tl* add_switch(const glm::ivec2& size, const std::string& label, bool v, bool inlinetxt = false);
checkbox_tl* add_checkbox(const glm::ivec2& size, const std::string& label, bool v);
radio_tl* add_radio(const glm::ivec2& size, const std::string& label, bool v, group_radio_t* gp);
edit_tl* add_input(const std::string& label, const glm::ivec2& size, bool single_line);
gradient_btn* add_gbutton(const std::string& label, const glm::ivec2& size, uint32_t bcolor);
color_btn* add_cbutton(const std::string& label, const glm::ivec2& size, int idx);
color_btn* add_label(const std::string& label, const glm::ivec2& size, int idx);
progress_tl* add_progress(const std::string& format, const glm::ivec2& size, double v);
colorpick_tl* add_colorpick(uint32_t c, int width, int h, bool alpha);
*/


// 列表视图控件。显示文本、image_ptr_t、svg_cx 





flex_item* flexlayout(flex_item* r, std::vector<glm::vec4>& v, const glm::vec2& pos, const glm::vec2& gap);


double get_alpha_f(uint32_t c);
bool is_alpha(uint32_t c);
uint32_t set_alpha(uint32_t c, uint32_t a);

bool rect_includes(const glm::vec4& rc, const glm::vec2& p);
// rc=left,top,right,bottom
bool rect_includes(const glm::vec4& rc, const glm::vec4& other);
glm::ivec2 check_box_cr(const glm::vec2& p, const glm::vec4* d, size_t count);
glm::ivec2 check_box_cr1(const glm::vec2& p, const glm::vec4* d, size_t count, int stride);
glm::vec2 v2xm3(const glm::vec2& v, const glm::mat3& m);
glm::vec4 get_boxm3(const glm::vec4& v, const glm::mat3& m);

#if 1
svg_cx* new_svg_file(const void* fn, size_t len, int dpi);
svg_cx* new_svg_data(const void* str, size_t len, int dpi);
void free_svg(svg_cx* svg);
void render_svg(cairo_t* cr, svg_cx* svg);
void render_svg(cairo_t* cr, svg_cx* svg, const glm::vec2& pos, const glm::vec2& scale, double angle, const char* id = 0);

// 加载svg或图片
cairo_surface_t* load_imagesvg(const std::string& fn, float scale);

glm::vec4 colorv4(uint32_t rgba);
glm::vec4 colorv4_bgr(uint32_t bgra);
void set_color(cairo_t* cr, uint32_t rgba);
void set_color_bgr(cairo_t* cr, uint32_t c);
void set_color_a(cairo_t* cr, uint32_t rgba, double a);
void set_source_rgba(cairo_t* cr, glm::vec4 rgba);
void draw_rectangle(cairo_t* cr, const glm::vec4& rc, double r);
void draw_round_rectangle(cairo_t* cr, double x, double y, double width, double height, double r);
void draw_round_rectangle(cairo_t* cr, double x, double y, double width, double height, const glm::vec4& r);
void draw_round_rectangle(cairo_t* cr, const glm::vec4& rc, const glm::vec4& r);
void draw_circle(cairo_t* cr, const glm::vec2& pos, float r);
void draw_ellipse(cairo_t* cr, const glm::vec2& pos, const glm::vec2& r);
// 三角形基于矩形内 
//	 dir = 0;		// 尖角方向，0上，1右，2下，3左
//	 spos = 50;		// 尖角点位置0-1，中间就是0.5
void draw_triangle(cairo_t* cr, const glm::vec2& pos, const glm::vec2& size, const glm::vec2& dirspos);
void fill_stroke(cairo_t* cr, vg_style_t* st);
void fill_stroke(cairo_t* cr, uint32_t fill, uint32_t color, int linewidth = 1, bool isbgr = 0);
void draw_polyline(cairo_t* cr, const glm::vec2& pos, const glm::vec2* points, int points_count, uint32_t col, bool closed, float thickness);
void draw_polyline(cairo_t* cr, const PathsD* p, bool closed);
// 渲染索引多段线，索引-1则跳过
void draw_polylines(cairo_t* cr, const glm::vec2& pos, const glm::vec2* points, int points_count, int* idx, int idx_count, uint32_t col, float thickness);

void draw_rect(cairo_t* cr, const glm::vec4& rc, uint32_t fill, uint32_t color, double r, int linewidth);
//glm::ivec2 layout_text_x::get_text_rect(size_t idx, const void* str8, int len, int fontsize)
void draw_text(cairo_t* cr, layout_text_x* ltx, const void* str, int len, glm::vec4 text_rc, text_style_t* st, glm::ivec2* orc = 0);


enum class eg_e :uint32_t {
	enull,
	e_rect, e_text, e_circle, e_ellipse, e_triangle, e_polyline, e_polylines, e_image
};
struct rect_b {
	eg_e type = eg_e::e_rect;
	uint32_t fill, color; int thickness;
	glm::vec4 rc;	// 坐标，大小
	glm::ivec4 r;	// 圆角
};
struct text_b
{
	eg_e type = eg_e::e_text;
	const void* str;
	int len;
	glm::vec4 text_rc;
	text_style_t* st;
	layout_text_x* ltx;
};
struct circle_b {
	eg_e type = eg_e::e_circle;
	uint32_t fill, color; int thickness; float r;
	glm::vec2 pos;
};
struct ellipse_b {
	eg_e type = eg_e::e_ellipse;
	uint32_t fill, color; int thickness;
	glm::vec2 pos, r;
};
struct triangle_b {
	eg_e type = eg_e::e_triangle;
	uint32_t fill, color; int thickness;
	glm::vec2 pos, size, dirspos;
};
struct polyline_b
{
	eg_e type = eg_e::e_polyline;
	glm::vec2& pos; const glm::vec2* points; int points_count; uint32_t color, fill; bool closed; float thickness;
};
struct polylines_b
{
	eg_e type = eg_e::e_polylines;
	glm::vec2 pos;
	const glm::vec2* points; int points_count; int* idx; int idx_count; uint32_t color, fill; float thickness;
};
struct image_b {
	eg_e type = eg_e::e_image;
	void* image; glm::vec2 pos; glm::vec4 rc; uint32_t color = -1; glm::vec2 dsize = { -1,-1 };
	// 九宫格图片
	glm::vec4 sliced = {};
};

struct text_draw_t
{
	cairo_t* cr; layout_text_x* ltx;
	std::string* text = 0;
	glm::vec4* text_rc;
	uint32_t* color;
	int count = 0;
	text_style_t* st;
	glm::vec4 box_rc;
};

class dtext_cache
{
public:
	cairo_t* cr = 0;
	cairo_surface_t* surface = 0;
	cairo_t* cr_in = 0;
	glm::ivec2 size = {};
public:
	dtext_cache();
	~dtext_cache();
	void reset(cairo_t* cr_in, const glm::ivec2& vsize);
	void free_surface();
	void save2png(const char* name);
	void clear_color(uint32_t color);
private:

};

text_draw_t* new_text_drawable(layout_text_x* ltx, const glm::vec4& box_rc, text_style_t* st);
void draw_draw_texts(text_draw_t* p);

// 图形通用软渲染接口
void draw_ge(cairo_t* cr, void* p, int count);
// 批量渲染文本+矩形背景
void draw_rctext(cairo_t* cr, layout_text_x* ltx, text_tx* p, int count, text_style_tx* stv, int st_count, glm::ivec4* clip);

void clip_cr(cairo_t* cr, const glm::ivec4& clip);


cairo_surface_t* new_clip_rect(int r);
void clip_rect(cairo_t* cr, cairo_surface_t* r);
void paint_shadow(cairo_t* cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, bool rev, float r = 0);//color_to=shadow;shadow.w=0;
void paint_shadow(cairo_t* cr, double size_x, double size_y, double width, double height, const glm::vec4& shadow, const glm::vec4& color_to, bool rev, float r = 0);

// 采样颜色
glm::dvec4 mix_colors(glm::vec4 a, glm::vec4 b, float ratio);
/*
用 bezier curve（贝塞尔曲线） 来设置 color stop（颜色渐变规则），
这里使用下面的曲线形式，其中
X轴为 offset（偏移量，取值范围为 0~1，0 代表阴影绘制起点），
Y轴为 alpha（颜 色透明度，取值范围为0~1，0 代表完全透明），
*/
struct rect_shadow_t
{
	float radius = 4;	// 半径
	int segment = 6;	// 细分段
	glm::vec4 cfrom = { 0,0,0,0.8 }, cto = { 0.5,0.5,0.5,0.5 };// 颜色从cf到ct
	/*	cubic
		X轴为 offset（偏移量，取值范围为 0~1，0 代表阴影绘制起点），
		Y轴为 alpha（颜 色透明度，取值范围为0~1，0 代表完全透明），
	*/
	cubic_v cubic = { {0.0,0.6},{0.5,0.39},{0.4,0.1},{1.0,0.0 } };
};
void draw_rectangle_gradient(cairo_t* cr, int width, int height, const rect_shadow_t* rs);

// 获取对齐坐标
glm::ivec4 get_text_align(cairo_t* cr, const char* str, const glm::vec2& pos, const glm::vec2& boxsize, const glm::vec2& text_align, const char* family, int fontsize);
void draw_text(cairo_t* cr, const char* str, const glm::ivec4& et, uint32_t text_color);
// box渲染对齐文本
glm::ivec4 draw_text_align(cairo_t* cr, const char* str, const glm::vec2& pos, const glm::vec2& boxsize, const glm::vec2& text_align, uint32_t text_color, const char* family, int fontsize);
#ifndef key_def_data
#define key_def_data 1024
#define key_def_data_iptr 1025
#define key_def_data_svgptr 1026
#define key_def_data_done 1000
#endif
cairo_surface_t* new_image_cr(image_ptr_t* img);
cairo_surface_t* new_image_cr(const glm::ivec2& size, uint32_t* data = 0);
void update_image_cr(cairo_surface_t* image, image_ptr_t* img);
void free_image_cr(cairo_surface_t* image);
void image_set_ud(cairo_surface_t* p, uint64_t key, void* ud, void (*destroy_func)(void* data));
void* image_get_ud(cairo_surface_t* p, uint64_t key);
void image_save_png(cairo_surface_t* cr, const char* fn);
glm::ivec2 get_surface_size(cairo_surface_t* p);
// 渲染图片到cr
glm::vec2 draw_image(cairo_t* cr, cairo_surface_t* image, const glm::vec2& pos, const glm::vec4& rc, uint32_t color = -1, const glm::vec2& dsize = { -1,-1 });
// 渲染九宫格图片
glm::vec2 draw_image(cairo_t* cr, cairo_surface_t* image, const glm::vec2& pos, const glm::vec4& rc, uint32_t color, const glm::vec2& size, const glm::vec4& sliced);
#endif

int get_rand(int f, int s);
int64_t get_rand64(int64_t f, int64_t s);


/*
<表格、树形>
数据结构设计:
	网格线：		虚线/实线、颜色
	背景：		纯色、图片
	内容store：	数值、文本、简单公式、图片/SVG

*/
//vg_style_t
using cell_line_style = vg_style_t;
// 文本、填充颜色
struct cell_color_style {
	uint32_t text_color = 0xff282828;
	uint32_t color = 0xffffffff;
};
// 单元格内容
struct cell_store
{
	int type = 0;		// 0文本，1整数，2无符号整数，3浮点数，4公式
	std::string text;	// 显示文本
	union {
		int64_t iv;
		uint64_t uv;
		double fv;
		const char* expression;
	}v = {};
};
// 格式
struct format_style {
	const char* format = 0;		// 数字格式化
	const char* familys = 0;	// 字体名，逗号分隔
	int fontsize = 12;			// 字体大小
};
// 对齐格式
struct align_style {
	glm::vec3 a;	// x水平，y垂直
	//bool autobr = false;// z自动换行
};
//图片/svg
struct cell_image_store
{
	std::string uri;			// uri、base64（图片数据或svg字符串）
	glm::ivec2 size;			// 指定图片显示大小，0则填充单元格大小，-1则原始大小，其它则缩放数值
	glm::i64vec2 pos = {};		// 单元格坐标
};


// 线样式
struct cell_line_pos
{
	int style = 0;//线风格
	glm::ivec2 first = {};	// 开始格坐标
	glm::ivec2 second = {};	// 结束格坐标
};
struct cell_line_pos64
{
	int style = 0;
	glm::i64vec2 first = {};
	glm::i64vec2 second = {};
};
// 单元格 文本、填充颜色
struct cell_fill_style {
	int style = 0;
	glm::ivec2 first = {};	// 开始格坐标
	glm::ivec2 second = {};	// 结束格坐标，-1全部
};
struct cell_fill_style64 {
	int style = 0;
	glm::i64vec2 first = {};
	glm::i64vec2 second = {};
};
// 单元格内容坐标
struct cell_store_pos
{
	cell_store* value = 0;
	format_style* format = 0;
	align_style* align = 0;
	glm::ivec2 pos = {};			// 单元格坐标 
};
struct cell_store_pos64
{
	cell_store* value = 0;
	glm::i64vec2 pos = {};			// 64位坐标
};






// 表格填充支持的控件类型
enum class widget_type :uint32_t {
	wt_null,
	wt_text,
	wt_image,
	wt_edit,
	wt_color_btn,
	wt_gradient_btn,
	wt_radio_btn,
	wt_checkbox_btn,
	wt_switch_btn,
	wt_progress,
	wt_slider
};



// 灰度图转rgba
void gray_copy2rgba(image_ptr_t* dst, image_ptr_t* src, const glm::ivec2& dst_pos, const glm::ivec4& rc, uint32_t col, bool isblend);
//单色位图1位
void bit_copy2rgba(image_ptr_t* dst, image_ptr_t* src, const glm::ivec2& dst_pos, const glm::ivec4& rc, uint32_t color);

void rgba_copy2rgba(image_ptr_t* dst, image_ptr_t* src, const glm::ivec2& dst_pos, const glm::ivec4& rc, bool isblend);

void split_v(std::string str, const std::string& pattern, std::vector<std::string>& result);


std::string icu_u16_gbk(const void* str, size_t size);
std::string icu_gbk_u8(const char* str, size_t size);
std::string icu_u8_gbk(const char* str, size_t size);
std::string icu_u16_u8(const void* str, size_t size);
std::u16string icu_u8_u16(const char* str, size_t size);

void do_text(const char* str, size_t first, size_t count);

// align val to the next multiple of alignment
uint64_t align_up(uint64_t val, uint64_t alignment);
// align val to the previous multiple of alignment
uint64_t align_down(uint64_t val, uint64_t alignment);
uint64_t divideroundingup(uint64_t a, uint64_t b);

enum GRID_ATTR
{
	OK = 0,
	G_UNKNOWN = 0,
	G_CHECK,
	G_CLOSE
};

struct grid_node
{
	uint8_t	state;		//状态
	size_t	total;		//总距离
	size_t	start;		//距起点
	size_t	end;		//距终点
	glm::ivec2		parent;		//父坐标
};

class astar_search
{
public:
	struct grid_node_cmp
	{
		bool operator()(grid_node* n1, grid_node* n2)
		{
			return n1->total > n2->total;
		}
	};
	uint8_t* data = 0;	// 地图数据
	int stride = 1;		// 默认步长
	int wall = 0;		// 高度大于wall为墙
	std::vector<float> speeds;	// 加速表，data为索引
	glm::ivec2 gdist = { 10,14 };	//直线、斜线距离1.414
	grid_node* _node = 0;		//节点
	uint32_t width = 0;		//宽度
	uint32_t height = 0;	//高度
	uint32_t size = 0;		//大小
	uint32_t cap_size = 0;	//分配大小
	void* m = 0;
	std::priority_queue<grid_node*, std::vector<grid_node*>, grid_node_cmp> _open;//最小堆(开放列表)
	std::vector<glm::ivec2> path;
public:
	astar_search();
	~astar_search();
	void init(uint32_t w, uint32_t h, uint8_t* d, bool copydata);
	void set_wallheight(int h);
	void set_speed(float* d, int n);
	// 设置起点、终点、0为支持斜角，1不支持斜角
	bool FindPath(glm::ivec2* pStart, glm::ivec2* pEnd, bool mode);
	bool NextPath(glm::ivec2* pos);

	uint8_t* Get(uint8_t* d, size_t i);
	grid_node* pop_n();
private:
};

#define WAY 0
#define WALL 1
#define TRAP 2
#define M_EXIT 3

class maze_cx
{
public:
	glm::ivec2 start = {}, dest = {};
private:
	std::vector<uint8_t> _map_way;
	int width = 0, height = 0;
	int Rank = 0;
	uint64_t seed = 0;
	std::stack<glm::ivec2> _stack;
	std::mt19937_64 gen_map, gen;	// 地图随机种子、普通随机种子
public:
	maze_cx();
	~maze_cx();
	void init(int w, int h);
	uint8_t* data();
	void set_seed(uint64_t c);
	int64_t get_rand(int64_t f, int64_t s);
	int64_t get_rand_m(int64_t f, int64_t s);
	void generateMazeD(uint8_t* maze, int rows, int cols);
private:

};
/*
a	b	y
与
0	0	0
0	1	0
1	0	0
1	1	1
或
0	0	0
0	1	1
1	0	1
1	1	1
异或
0	0	0
0	1	1
1	0	1
1	1	0
非
0		1
1		0
与非
0	0	1
0	1	1
1	0	1
1	1	0
或非
0	0	1
0	1	0
1	0	0
1	1	0
异或非
0	0	1
0	1	0
1	0	0
1	1	1

*/


#include "font_core.h"
#include "backend_2d.h"
#include "gui.h"
#include "audio.h"





namespace bs {

	void test_timeline();
}

#endif // !PNGUO_H
