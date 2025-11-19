/*
渲染后端：cairo、vkvg
*/
#ifndef BACKEND_2D_H
#define BACKEND_2D_H
typedef struct d2ctx_t d2ctx_t;
typedef struct d2_surface_t d2_surface_t;
d2ctx_t* new_cr(d2_surface_t* sur);
void free_cr(d2ctx_t* cr);

// 预乘输出bgra，type=0为原数据是rgba
void premultiply_data(int w, unsigned char* data, int type, bool multiply);


/*
用 bezier curve（贝塞尔曲线） 来设置 color stop（颜色渐变规则），
这里使用下面的曲线形式，其中
X轴为 offset（偏移量，取值范围为 0~1，0 代表阴影绘制起点），
Y轴为 alpha（颜 色透明度，取值范围为0~1，0 代表完全透明），
*/
struct rect_shadow_t0
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

glm::vec4 colorv4(uint32_t rgba);
glm::vec4 colorv4_bgr(uint32_t bgra);

struct font_xi {
	const char* family = 0;
	float font_size = 16;
	uint32_t color = 0xffffffff;
	uint8_t slant = 0;//NORMAL=0，ITALIC=1，OBLIQUE=2
	bool weight_bold = false;
};

// auto save 
class d2_as
{
public:
	d2_as(d2ctx_t* p);
	~d2_as();

private:
	d2ctx_t* cr = 0;
};

class canvas_dev
{
public:
	d2_surface_t* surface = 0;
	d2ctx_t* cr = 0;
	uint32_t* pixel = 0;
	glm::ivec2 dsize = {};
	// 全局偏移、缩放
	glm::vec2 translate = { 0.0f,0.0f };
	glm::vec2 scale = { 1.0f,1.0f };
	double angle = 0;
	int max_zoom = 25600;//最大百分比
	//std::vector<vertex_t32>* tv = 0;
	std::vector<uint32_t> tdata;
public:
	canvas_dev();
	~canvas_dev();

	static canvas_dev* new_cdev(const glm::ivec2& size, uint32_t* data);
	static canvas_dev* new_cdev_svg(const glm::ivec2& size, const char* svgfn);
	static canvas_dev* new_cdev_dc(void* hdc);
	static void free_cdev(canvas_dev* p);
	// 保存图像到文件 
	void save_png(const char* fn);
	// 使用颜色清屏
	void set_clear(uint32_t color);
	// 渲染一条路径 


	void draw_path(path_v* p, style_path_t* st);

	void draw_circle(std::vector<std::vector<glm::vec2>>* ov, float r, style_path_t* st);
	void draw_rect(const glm::vec2&, style_path_t* st);
	// 画多个圆
	void draw_circle(std::vector<glm::vec2>* ov, float r, style_path_t* st);
	// 两个坐标一条线的多条线
	void draw_line(std::vector<glm::vec2>* ov, style_path_t* st);
	void draw_line(const glm::vec4& ov, uint32_t color, float linewidth);
	void draw_line(const glm::vec2& t, const glm::vec2& t1, uint32_t stroke_color, float linewidth);
	// 一条线
	void draw_line1(std::vector<glm::vec2>* ov, style_path_t* st);
	void draw_text(const void* str, const glm::vec2& pos, font_xi* fx);
	void draw_circle(const glm::vec2& pos, float r, float linewidth, uint32_t color, uint32_t fill);

	void draw_surface(d2_surface_t* image, const glm::vec2& pos, double alpha);


	int get_path(path_v* pt);

	glm::vec4 get_path_extents();
	glm::ivec4 get_text_extents(const void* str, int len, font_xi* fx);
	void begin(uint32_t color);
	void end();
	void save();
	void set_pos(const glm::vec2& pos);
	void set_scale(double sc);
	void set_scalei(int& zoom);
	void set_dscale(const glm::vec2& sc);
	void set_dscale(double sc);
	void set_dscalei(int& zoom);
	uint32_t* data();
private:

};
struct et_un_t;
class tinyviewcanvas_x;
class Ruler;
class view_g
{
public:
	glm::ivec2 vsize = { 1000,800 };	//视图大小
	glm::ivec2 vgpos = { 0,0 };			//视图全局坐标
	tinyviewcanvas_x* vcanvas = 0;		//画板
	canvas_dev* cdv = 0;
	font_xi fx = {};
	Ruler* ruler = {};					// 标尺
	glm::vec4 pagebox = {}, selbox = {};	// 页面框、选中框
	// 方框大小、坐标
	glm::vec2 wsize = { 800,600 };
	glm::vec2 vpos = { 500,400 };
	// 标尺大小
	glm::vec2 rsize = { 20, 20 };
	int fh = 12;	//字高
	// 背景格宽
	int bwidth = 8;
	// 1单位宽度
	glm::ivec3 step_width = { 20,20,5 };
	// 背景色x、线框颜色y、背景格颜色z
	glm::ivec4 color = { 0xff505050, 0x80FF8050,0x80353535,0x80606060 };
	glm::ivec4 wcolor = { -1, 0xff000000,-1 ,0 };//填充、线、方框线
	uint32_t clear_color = 0xff1b1b1b;
	uint32_t hover_color = 0x800000ff;
	glm::ivec2 cross_color = { 0xff503d85, 0xff327660 };
	int cross_width = 2;
	// 当前鼠标坐标
	glm::ivec2 curpos = {};
	//btype_e btype = {};		// 按钮状态
	path_v pvdata[2] = {};
	d2_surface_t* _backing_store = 0;
	d2_surface_t* rss = 0;
	bool _backing_store_valid = false;
	bool _draw_valid = true;
	bool is_yaxisdown = 1;		// 1=y向下
	bool is_ck = false;
public:
	view_g();
	~view_g();
	static view_g* new_view(const glm::vec2& size);
	static void free_view(view_g* p);
	void set_view(const glm::ivec2& size);

	void update(float a = 0.0);
	void begin_draw();
	void draw();
	void end_draw();
	glm::mat3 get_mat();
	glm::mat3x2 get_mat3x2();
	glm::dmat3x2 get_dmat3x2();
	// 发送事件到
	void on_event(uint32_t type, et_un_t* ep);
private:
	void on_motion(const glm::vec2& pos);
	void on_button(int idx, int down, const glm::vec2& pos, int clicks);
	void on_wheel(double x, double y);
	void updateRulers();
	void mkpath();
	void draw_grid_back(const glm::ivec2& vss);
};

 


#endif // !BACKEND_2D_H