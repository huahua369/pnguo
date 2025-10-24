/*
渲染后端：cairo、vkvg
*/
#ifndef BACKEND_2D_H
#define BACKEND_2D_H

cairo_t* new_cr(cairo_surface_t* sur);
void free_cr(cairo_t* cr);


struct font_xi {
	const char* family = 0;
	float font_size = 16;
	uint32_t color = 0xffffffff;
	uint8_t slant = 0;//NORMAL=0，ITALIC=1，OBLIQUE=2
	bool weight_bold = false;
};

// auto save 
class cairo_as
{
public:
	cairo_as(cairo_t* p);
	~cairo_as();

private:
	cairo_t* cr = 0;
};

class canvas_dev
{
public:
	cairo_surface_t* surface = 0;
	cairo_t* cr = 0;
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

	void draw_surface(cairo_surface_t* image, const glm::vec2& pos, double alpha);


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
	cairo_surface_t* _backing_store = 0;
	cairo_surface_t* rss = 0;
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



#ifndef no_stb2d

#ifdef __cplusplus
extern "C" {
#endif

	/*
	 * type
	 */
	typedef signed long XCG_FT_Fixed;
	typedef signed int XCG_FT_Int;
	typedef unsigned int XCG_FT_UInt;
	typedef signed long XCG_FT_Long;
	typedef unsigned long XCG_FT_ULong;
	typedef signed short XCG_FT_Short;
	typedef unsigned char XCG_FT_Byte;
	typedef unsigned char XCG_FT_Bool;
	typedef int XCG_FT_Error;
	typedef signed long XCG_FT_Pos;

	typedef struct XCG_FT_Vector_ {
		XCG_FT_Pos x;
		XCG_FT_Pos y;
	} XCG_FT_Vector;

	/*
	 * math
	 */
	typedef XCG_FT_Fixed XCG_FT_Angle;

	XCG_FT_Long XCG_FT_MulFix(XCG_FT_Long a, XCG_FT_Long b);
	XCG_FT_Long XCG_FT_MulDiv(XCG_FT_Long a, XCG_FT_Long b, XCG_FT_Long c);
	XCG_FT_Long XCG_FT_DivFix(XCG_FT_Long a, XCG_FT_Long b);
	XCG_FT_Fixed XCG_FT_Sin(XCG_FT_Angle angle);
	XCG_FT_Fixed XCG_FT_Cos(XCG_FT_Angle angle);
	XCG_FT_Fixed XCG_FT_Tan(XCG_FT_Angle angle);
	XCG_FT_Angle XCG_FT_Atan2(XCG_FT_Fixed x, XCG_FT_Fixed y);
	XCG_FT_Angle XCG_FT_Angle_Diff(XCG_FT_Angle angle1, XCG_FT_Angle angle2);
	void XCG_FT_Vector_Unit(XCG_FT_Vector* vec, XCG_FT_Angle angle);
	void XCG_FT_Vector_Rotate(XCG_FT_Vector* vec, XCG_FT_Angle angle);
	XCG_FT_Fixed XCG_FT_Vector_Length(XCG_FT_Vector* vec);
	void XCG_FT_Vector_Polarize(XCG_FT_Vector* vec, XCG_FT_Fixed* length, XCG_FT_Angle* angle);
	void XCG_FT_Vector_From_Polar(XCG_FT_Vector* vec, XCG_FT_Fixed length, XCG_FT_Angle angle);

	/*
	 * raster
	 */
	typedef struct XCG_FT_BBox_ {
		XCG_FT_Pos xMin, yMin;
		XCG_FT_Pos xMax, yMax;
	} XCG_FT_BBox;

	typedef struct XCG_FT_Outline_ {
		int n_contours;
		int n_points;
		XCG_FT_Vector* points;
		char* tags;
		int* contours;
		char* contours_flag;
		int flags;
	} XCG_FT_Outline;

#define XCG_FT_OUTLINE_NONE				0x0
#define XCG_FT_OUTLINE_OWNER			0x1
#define XCG_FT_OUTLINE_EVEN_ODD_FILL	0x2
#define XCG_FT_OUTLINE_REVERSE_FILL		0x4
#define XCG_FT_CURVE_TAG(flag)			(flag & 3)
#define XCG_FT_CURVE_TAG_ON      		1
#define XCG_FT_CURVE_TAG_CONIC			0
#define XCG_FT_CURVE_TAG_CUBIC			2
#define XCG_FT_Curve_Tag_On				XCG_FT_CURVE_TAG_ON
#define XCG_FT_Curve_Tag_Conic			XCG_FT_CURVE_TAG_CONIC
#define XCG_FT_Curve_Tag_Cubic			XCG_FT_CURVE_TAG_CUBIC

	typedef struct XCG_FT_Span_ {
		int x;
		int len;
		int y;
		unsigned char coverage;
	} XCG_FT_Span;

	typedef void (*XCG_FT_SpanFunc)(int count, const XCG_FT_Span* spans, void* user);
#define XCG_FT_Raster_Span_Func  XCG_FT_SpanFunc

#define XCG_FT_RASTER_FLAG_DEFAULT  0x0
#define XCG_FT_RASTER_FLAG_AA       0x1
#define XCG_FT_RASTER_FLAG_DIRECT   0x2
#define XCG_FT_RASTER_FLAG_CLIP     0x4

	typedef struct XCG_FT_Raster_Params_ {
		const void* source;
		int flags;
		XCG_FT_SpanFunc gray_spans;
		void* user;
		XCG_FT_BBox clip_box;
	} XCG_FT_Raster_Params;

	XCG_FT_Error XCG_FT_Outline_Check(XCG_FT_Outline* outline);
	void XCG_FT_Outline_Get_CBox(const XCG_FT_Outline* outline, XCG_FT_BBox* acbox);
	void XCG_FT_Raster_Render(const XCG_FT_Raster_Params* params);

	/*
	 * stroker
	 */
	typedef struct XCG_FT_StrokerRec_* XCG_FT_Stroker;

	typedef enum XCG_FT_Stroker_LineJoin_ {
		XCG_FT_STROKER_LINEJOIN_ROUND = 0,
		XCG_FT_STROKER_LINEJOIN_BEVEL = 1,
		XCG_FT_STROKER_LINEJOIN_MITER_VARIABLE = 2,
		XCG_FT_STROKER_LINEJOIN_MITER = XCG_FT_STROKER_LINEJOIN_MITER_VARIABLE,
		XCG_FT_STROKER_LINEJOIN_MITER_FIXED = 3,
	} XCG_FT_Stroker_LineJoin;

	typedef enum XCG_FT_Stroker_LineCap_ {
		XCG_FT_STROKER_LINECAP_BUTT = 0,
		XCG_FT_STROKER_LINECAP_ROUND = 1,
		XCG_FT_STROKER_LINECAP_SQUARE = 2,
	} XCG_FT_Stroker_LineCap;

	typedef enum XCG_FT_StrokerBorder_ {
		XCG_FT_STROKER_BORDER_LEFT = 0,
		XCG_FT_STROKER_BORDER_RIGHT = 1,
	} XCG_FT_StrokerBorder;

	XCG_FT_Error XCG_FT_Stroker_New(XCG_FT_Stroker* astroker);
	void XCG_FT_Stroker_Set(XCG_FT_Stroker stroker, XCG_FT_Fixed radius, XCG_FT_Stroker_LineCap line_cap, XCG_FT_Stroker_LineJoin line_join, XCG_FT_Fixed miter_limit);
	XCG_FT_Error XCG_FT_Stroker_ParseOutline(XCG_FT_Stroker stroker, const XCG_FT_Outline* outline);
	XCG_FT_Error XCG_FT_Stroker_GetCounts(XCG_FT_Stroker stroker, XCG_FT_UInt* anum_points, XCG_FT_UInt* anum_contours);
	void XCG_FT_Stroker_Export(XCG_FT_Stroker stroker, XCG_FT_Outline* outline);
	void XCG_FT_Stroker_Done(XCG_FT_Stroker stroker);

	struct cg_point_t {
		double x;
		double y;
	};

	struct cg_rect_t {
		double x;
		double y;
		double w;
		double h;
	};

	struct cg_matrix_t {
		double a; double b;
		double c; double d;
		double tx; double ty;
	};

	struct cg_color_t {
		double r;
		double g;
		double b;
		double a;
	};

	struct cg_gradient_stop_t {
		double offset;
		struct cg_color_t color;
	};

	enum cg_path_element_t {
		CG_PATH_ELEMENT_MOVE_TO = 0,
		CG_PATH_ELEMENT_LINE_TO = 1,
		CG_PATH_ELEMENT_CURVE_TO = 2,
		CG_PATH_ELEMENT_CLOSE = 3,
	};

	enum cg_spread_method_t {
		CG_SPREAD_METHOD_PAD = 0,
		CG_SPREAD_METHOD_REFLECT = 1,
		CG_SPREAD_METHOD_REPEAT = 2,
	};

	enum cg_gradient_type_t {
		CG_GRADIENT_TYPE_LINEAR = 0,
		CG_GRADIENT_TYPE_RADIAL = 1,
	};

	enum cg_texture_type_t {
		CG_TEXTURE_TYPE_PLAIN = 0,
		CG_TEXTURE_TYPE_TILED = 1,
	};

	enum cg_line_cap_t {
		CG_LINE_CAP_BUTT = 0,
		CG_LINE_CAP_ROUND = 1,
		CG_LINE_CAP_SQUARE = 2,
	};

	enum cg_line_join_t {
		CG_LINE_JOIN_MITER = 0,
		CG_LINE_JOIN_ROUND = 1,
		CG_LINE_JOIN_BEVEL = 2,
	};

	enum cg_fill_rule_t {
		CG_FILL_RULE_NON_ZERO = 0,
		CG_FILL_RULE_EVEN_ODD = 1,
	};

	enum cg_paint_type_t {
		CG_PAINT_TYPE_COLOR = 0,
		CG_PAINT_TYPE_GRADIENT = 1,
		CG_PAINT_TYPE_TEXTURE = 2,
	};

	enum cg_operator_t {
		CG_OPERATOR_SRC = 0, /* r = s * ca + d * cia */
		CG_OPERATOR_SRC_OVER = 1, /* r = (s + d * sia) * ca + d * cia */
		CG_OPERATOR_DST_IN = 2, /* r = d * sa * ca + d * cia */
		CG_OPERATOR_DST_OUT = 3, /* r = d * sia * ca + d * cia */
	};

	struct cg_surface_t {
		int ref;
		int width;
		int height;
		int stride;
		int owndata;
		uint8_t* pixels;
	};

	struct cg_path_t {
		int contours;
		struct cg_point_t start;
		struct {
			enum cg_path_element_t* data;
			int size;
			int capacity;
		} elements;
		struct {
			struct cg_point_t* data;
			int size;
			int capacity;
		} points;
	};

	struct cg_gradient_t {
		enum cg_gradient_type_t type;
		enum cg_spread_method_t spread;
		struct cg_matrix_t matrix;
		double values[6];
		double opacity;
		struct {
			struct cg_gradient_stop_t* data;
			int size;
			int capacity;
		} stops;
	};

	struct cg_texture_t {
		enum cg_texture_type_t type;
		struct cg_surface_t* surface;
		struct cg_matrix_t matrix;
		double opacity;
	};

	struct cg_paint_t {
		enum cg_paint_type_t type;
		struct cg_color_t color;
		struct cg_gradient_t gradient;
		struct cg_texture_t texture;
	};

	struct cg_span_t {
		int x;
		int len;
		int y;
		unsigned char coverage;
	};

	struct cg_rle_t {
		struct {
			struct cg_span_t* data;
			int size;
			int capacity;
		} spans;
		int x;
		int y;
		int w;
		int h;
	};

	struct cg_dash_t {
		double offset;
		double* data;
		int size;
	};

	struct cg_stroke_data_t {
		double width;
		double miterlimit;
		enum cg_line_cap_t cap;
		enum cg_line_join_t join;
		struct cg_dash_t* dash;
	};

	struct cg_state_t {
		struct cg_rle_t* clippath;
		struct cg_paint_t paint;
		struct cg_matrix_t matrix;
		enum cg_fill_rule_t winding;
		struct cg_stroke_data_t stroke;
		enum cg_operator_t op;
		double opacity;
		struct cg_state_t* next;
	};

	struct cg_ctx_t {
		struct cg_surface_t* surface;
		struct cg_state_t* state;
		struct cg_path_t* path;
		struct cg_rle_t* rle;
		struct cg_rle_t* clippath;
		struct cg_rect_t clip;
		void* outline_data;
		size_t outline_size;
	};

#ifndef CG_MIN
#define CG_MIN(a, b)		({typeof(a) _amin = (a); typeof(b) _bmin = (b); (void)(&_amin == &_bmin); _amin < _bmin ? _amin : _bmin;})
#endif
#ifndef CG_MAX
#define CG_MAX(a, b)		({typeof(a) _amax = (a); typeof(b) _bmax = (b); (void)(&_amax == &_bmax); _amax > _bmax ? _amax : _bmax;})
#endif
#ifndef CG_CLAMP
#define CG_CLAMP(v, a, b)	CG_MIN(CG_MAX(a, v), b)
#endif
#ifndef CG_ALPHA
#define CG_ALPHA(c)			((c) >> 24)
#endif
#ifndef CG_DIV255
#define CG_DIV255(x)		(((x) + ((x) >> 8) + 0x80) >> 8)
#endif
#ifndef CG_BYTE_MUL
#define CG_BYTE_MUL(x, a)	((((((x) >> 8) & 0x00ff00ff) * (a)) & 0xff00ff00) + (((((x) & 0x00ff00ff) * (a)) >> 8) & 0x00ff00ff))
#endif

	void cg_memfill32(uint32_t* dst, uint32_t val, int len);
	void cg_comp_solid_source(uint32_t* dst, int len, uint32_t color, uint32_t alpha);
	void cg_comp_solid_source_over(uint32_t* dst, int len, uint32_t color, uint32_t alpha);
	void cg_comp_solid_destination_in(uint32_t* dst, int len, uint32_t color, uint32_t alpha);
	void cg_comp_solid_destination_out(uint32_t* dst, int len, uint32_t color, uint32_t alpha);
	void cg_comp_source(uint32_t* dst, int len, uint32_t* src, uint32_t alpha);
	void cg_comp_source_over(uint32_t* dst, int len, uint32_t* src, uint32_t alpha);
	void cg_comp_destination_in(uint32_t* dst, int len, uint32_t* src, uint32_t alpha);
	void cg_comp_destination_out(uint32_t* dst, int len, uint32_t* src, uint32_t alpha);

	void cg_matrix_init(struct cg_matrix_t* m, double a, double b, double c, double d, double tx, double ty);
	void cg_matrix_init_identity(struct cg_matrix_t* m);
	void cg_matrix_init_translate(struct cg_matrix_t* m, double tx, double ty);
	void cg_matrix_init_scale(struct cg_matrix_t* m, double sx, double sy);
	void cg_matrix_init_rotate(struct cg_matrix_t* m, double r);
	void cg_matrix_translate(struct cg_matrix_t* m, double tx, double ty);
	void cg_matrix_scale(struct cg_matrix_t* m, double sx, double sy);
	void cg_matrix_rotate(struct cg_matrix_t* m, double r);
	void cg_matrix_multiply(struct cg_matrix_t* m, struct cg_matrix_t* m1, struct cg_matrix_t* m2);
	void cg_matrix_invert(struct cg_matrix_t* m);
	void cg_matrix_map_point(struct cg_matrix_t* m, struct cg_point_t* p1, struct cg_point_t* p2);

	struct cg_surface_t* cg_surface_create(int width, int height);
	struct cg_surface_t* cg_surface_create_for_data(int width, int height, void* pixels);
	void cg_surface_destroy(struct cg_surface_t* surface);
	struct cg_surface_t* cg_surface_reference(struct cg_surface_t* surface);

	void cg_gradient_set_values_linear(struct cg_gradient_t* gradient, double x1, double y1, double x2, double y2);
	void cg_gradient_set_values_radial(struct cg_gradient_t* gradient, double cx, double cy, double cr, double fx, double fy, double fr);
	void cg_gradient_set_spread(struct cg_gradient_t* gradient, enum cg_spread_method_t spread);
	void cg_gradient_set_matrix(struct cg_gradient_t* gradient, struct cg_matrix_t* m);
	void cg_gradient_set_opacity(struct cg_gradient_t* gradient, double opacity);
	void cg_gradient_add_stop_rgb(struct cg_gradient_t* gradient, double offset, double r, double g, double b);
	void cg_gradient_add_stop_rgba(struct cg_gradient_t* gradient, double offset, double r, double g, double b, double a);
	void cg_gradient_add_stop_color(struct cg_gradient_t* gradient, double offset, struct cg_color_t* color);
	void cg_gradient_add_stop(struct cg_gradient_t* gradient, struct cg_gradient_stop_t* stop);
	void cg_gradient_clear_stops(struct cg_gradient_t* gradient);

	void cg_texture_set_type(struct cg_texture_t* texture, enum cg_texture_type_t type);
	void cg_texture_set_matrix(struct cg_texture_t* texture, struct cg_matrix_t* m);
	void cg_texture_set_surface(struct cg_texture_t* texture, struct cg_surface_t* surface);
	void cg_texture_set_opacity(struct cg_texture_t* texture, double opacity);

	struct cg_ctx_t* cg_create(struct cg_surface_t* surface);
	void cg_destroy(struct cg_ctx_t* ctx);
	void cg_save(struct cg_ctx_t* ctx);
	void cg_restore(struct cg_ctx_t* ctx);
	struct cg_color_t* cg_set_source_rgb(struct cg_ctx_t* ctx, double r, double g, double b);
	struct cg_color_t* cg_set_source_rgba(struct cg_ctx_t* ctx, double r, double g, double b, double a);
	struct cg_color_t* cg_set_source_color(struct cg_ctx_t* ctx, struct cg_color_t* color);
	struct cg_gradient_t* cg_set_source_linear_gradient(struct cg_ctx_t* ctx, double x1, double y1, double x2, double y2);
	struct cg_gradient_t* cg_set_source_radial_gradient(struct cg_ctx_t* ctx, double cx, double cy, double cr, double fx, double fy, double fr);
	struct cg_texture_t* cg_set_source_surface(struct cg_ctx_t* ctx, struct cg_surface_t* surface, double x, double y);
	void cg_set_operator(struct cg_ctx_t* ctx, enum cg_operator_t op);
	void cg_set_opacity(struct cg_ctx_t* ctx, double opacity);
	void cg_set_fill_rule(struct cg_ctx_t* ctx, enum cg_fill_rule_t winding);
	void cg_set_line_width(struct cg_ctx_t* ctx, double width);
	void cg_set_line_cap(struct cg_ctx_t* ctx, enum cg_line_cap_t cap);
	void cg_set_line_join(struct cg_ctx_t* ctx, enum cg_line_join_t join);
	void cg_set_miter_limit(struct cg_ctx_t* ctx, double limit);
	void cg_set_dash(struct cg_ctx_t* ctx, double* dashes, int ndash, double offset);
	void cg_translate(struct cg_ctx_t* ctx, double tx, double ty);
	void cg_scale(struct cg_ctx_t* ctx, double sx, double sy);
	void cg_rotate(struct cg_ctx_t* ctx, double r);
	void cg_transform(struct cg_ctx_t* ctx, struct cg_matrix_t* m);
	void cg_set_matrix(struct cg_ctx_t* ctx, struct cg_matrix_t* m);
	void cg_identity_matrix(struct cg_ctx_t* ctx);
	void cg_move_to(struct cg_ctx_t* ctx, double x, double y);
	void cg_line_to(struct cg_ctx_t* ctx, double x, double y);
	void cg_curve_to(struct cg_ctx_t* ctx, double x1, double y1, double x2, double y2, double x3, double y3);
	void cg_quad_to(struct cg_ctx_t* ctx, double x1, double y1, double x2, double y2);
	void cg_rel_move_to(struct cg_ctx_t* ctx, double dx, double dy);
	void cg_rel_line_to(struct cg_ctx_t* ctx, double dx, double dy);
	void cg_rel_curve_to(struct cg_ctx_t* ctx, double dx1, double dy1, double dx2, double dy2, double dx3, double dy3);
	void cg_rel_quad_to(struct cg_ctx_t* ctx, double dx1, double dy1, double dx2, double dy2);
	void cg_rectangle(struct cg_ctx_t* ctx, double x, double y, double w, double h);
	void cg_round_rectangle(struct cg_ctx_t* ctx, double x, double y, double w, double h, double rx, double ry);
	void cg_ellipse(struct cg_ctx_t* ctx, double cx, double cy, double rx, double ry);
	void cg_circle(struct cg_ctx_t* ctx, double cx, double cy, double r);
	void cg_arc(struct cg_ctx_t* ctx, double cx, double cy, double r, double a0, double a1);
	void cg_arc_negative(struct cg_ctx_t* ctx, double cx, double cy, double r, double a0, double a1);
	void cg_new_path(struct cg_ctx_t* ctx);
	void cg_close_path(struct cg_ctx_t* ctx);
	void cg_reset_clip(struct cg_ctx_t* ctx);
	void cg_clip(struct cg_ctx_t* ctx);
	void cg_clip_preserve(struct cg_ctx_t* ctx);
	void cg_fill(struct cg_ctx_t* ctx);
	void cg_fill_preserve(struct cg_ctx_t* ctx);
	void cg_stroke(struct cg_ctx_t* ctx);
	void cg_stroke_preserve(struct cg_ctx_t* ctx);
	void cg_paint(struct cg_ctx_t* ctx);

#ifdef __cplusplus
}
#endif
#endif // no_stb2d



#endif // !BACKEND_2D_H