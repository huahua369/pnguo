#pragma once
/*
*
*	vkvg渲染矢量图到VkvgSurface表面
*
*/
#include "vkvg.h"
#ifndef vkvg_pattern_add_color_stop_rgba
#define vkvg_pattern_add_color_stop_rgba vkvg_pattern_add_color_stop
#endif 

#ifndef VK_DEFINE_HANDLE
#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_HANDLE(VkPhysicalDevice)
VK_DEFINE_HANDLE(VkDevice)
VK_DEFINE_HANDLE(VkQueue)
#endif // !VK_DEFINE_HANDLE 

#ifdef __cplusplus
extern "C" {
#endif
#ifndef VKVG_H
	typedef struct _vkvg_device_t* VkvgDevice;
	typedef struct _vkvg_surface_t* VkvgSurface;
	typedef struct _vkvg_context_t* VkvgContext;
	typedef struct _vkvg_pattern_t* VkvgPattern;
	struct vkvg_matrix_t;
	enum vkvg_status_t;
	enum vkvg_line_cap_t;
	enum vkvg_line_join_t;
	enum vkvg_operator_t;
	enum vkvg_fill_rule_t;
#endif
	struct vkvg_func_t
	{
		VkvgContext(*vkvg_create)(VkvgSurface surf);
		void(*vkvg_destroy)(VkvgContext ctx);
		vkvg_status_t(*vkvg_status)(VkvgContext ctx);
		const char* (*vkvg_status_to_string)(vkvg_status_t status);
		VkvgContext(*vkvg_reference)(VkvgContext ctx);
		uint32_t(*vkvg_get_reference_count)(VkvgContext ctx);
		void(*vkvg_flush)(VkvgContext ctx);
		void(*vkvg_new_path)(VkvgContext ctx);
		void(*vkvg_close_path)(VkvgContext ctx);
		void(*vkvg_new_sub_path)(VkvgContext ctx);
		void(*vkvg_path_extents)(VkvgContext ctx, float* x1, float* y1, float* x2, float* y2);
		void(*vkvg_get_current_point)(VkvgContext ctx, float* x, float* y);
		void(*vkvg_line_to)(VkvgContext ctx, float x, float y);
		void(*vkvg_rel_line_to)(VkvgContext ctx, float dx, float dy);
		void(*vkvg_move_to)(VkvgContext ctx, float x, float y);
		void(*vkvg_rel_move_to)(VkvgContext ctx, float x, float y);
		void(*vkvg_arc)(VkvgContext ctx, float xc, float yc, float radius, float a1, float a2);
		void(*vkvg_arc_negative)(VkvgContext ctx, float xc, float yc, float radius, float a1, float a2);
		void(*vkvg_curve_to)(VkvgContext ctx, float x1, float y1, float x2, float y2, float x3, float y3);
		void(*vkvg_rel_curve_to)(VkvgContext ctx, float x1, float y1, float x2, float y2, float x3, float y3);
		void(*vkvg_quadratic_to)(VkvgContext ctx, float x1, float y1, float x2, float y2);
		void(*vkvg_rel_quadratic_to)(VkvgContext ctx, float x1, float y1, float x2, float y2);
		vkvg_status_t(*vkvg_rectangle)(VkvgContext ctx, float x, float y, float w, float h);
		vkvg_status_t(*vkvg_rounded_rectangle)(VkvgContext ctx, float x, float y, float w, float h, float radius);
		void(*vkvg_rounded_rectangle2)(VkvgContext ctx, float x, float y, float w, float h, float rx, float ry);
		void(*vkvg_ellipse)(VkvgContext ctx, float radiusX, float radiusY, float x, float y, float rotationAngle);
		void(*vkvg_elliptic_arc_to)(VkvgContext ctx, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);
		void(*vkvg_rel_elliptic_arc_to)(VkvgContext ctx, float x, float y, bool large_arc_flag, bool sweep_flag, float rx, float ry, float phi);
		void(*vkvg_stroke)(VkvgContext ctx);
		void(*vkvg_stroke_preserve)(VkvgContext ctx);
		void(*vkvg_fill)(VkvgContext ctx);
		void(*vkvg_fill_preserve)(VkvgContext ctx);
		void(*vkvg_paint)(VkvgContext ctx);
		void(*vkvg_clear)(VkvgContext ctx);
		void(*vkvg_reset_clip)(VkvgContext ctx);
		void(*vkvg_clip)(VkvgContext ctx);
		void(*vkvg_clip_preserve)(VkvgContext ctx);
		void(*vkvg_set_opacity)(VkvgContext ctx, float opacity);
		float(*vkvg_get_opacity)(VkvgContext ctx);
		void(*vkvg_set_source_color)(VkvgContext ctx, uint32_t c);
		void(*vkvg_set_source_rgba)(VkvgContext ctx, float r, float g, float b, float a);
		void(*vkvg_set_source_rgb)(VkvgContext ctx, float r, float g, float b);
		void(*vkvg_set_line_width)(VkvgContext ctx, float width);
		void(*vkvg_set_miter_limit)(VkvgContext ctx, float limit);
		//float(*vkvg_get_miter_limit)(VkvgContext ctx);
		void(*vkvg_set_line_cap)(VkvgContext ctx, vkvg_line_cap_t cap);
		void(*vkvg_set_line_join)(VkvgContext ctx, vkvg_line_join_t join);
		void(*vkvg_set_source_surface)(VkvgContext ctx, VkvgSurface surf, float x, float y);
		void(*vkvg_set_source)(VkvgContext ctx, VkvgPattern pat);
		void(*vkvg_set_operator)(VkvgContext ctx, vkvg_operator_t op);
		void(*vkvg_set_fill_rule)(VkvgContext ctx, vkvg_fill_rule_t fr);
		void(*vkvg_set_dash)(VkvgContext ctx, const float* dashes, uint32_t num_dashes, float offset);		// 虚线
		void(*vkvg_get_dash)(VkvgContext ctx, const float* dashes, uint32_t* num_dashes, float* offset);
		float(*vkvg_get_line_width)(VkvgContext ctx);
		vkvg_line_cap_t(*vkvg_get_line_cap)(VkvgContext ctx);
		vkvg_line_join_t(*vkvg_get_line_join)(VkvgContext ctx);
		vkvg_operator_t(*vkvg_get_operator)(VkvgContext ctx);
		vkvg_fill_rule_t(*vkvg_get_fill_rule)(VkvgContext ctx);
		VkvgPattern(*vkvg_get_source)(VkvgContext ctx);
		VkvgSurface(*vkvg_get_target)(VkvgContext ctx);
		bool(*vkvg_has_current_point)(VkvgContext ctx);
		void(*vkvg_save)(VkvgContext ctx);
		void(*vkvg_restore)(VkvgContext ctx);
		void(*vkvg_translate)(VkvgContext ctx, float dx, float dy);
		void(*vkvg_scale)(VkvgContext ctx, float sx, float sy);
		void(*vkvg_rotate)(VkvgContext ctx, float radians);
		void(*vkvg_transform)(VkvgContext ctx, const vkvg_matrix_t* matrix);
		void(*vkvg_set_matrix)(VkvgContext ctx, const vkvg_matrix_t* matrix);
		void(*vkvg_get_matrix)(VkvgContext ctx, vkvg_matrix_t* const matrix);
		void(*vkvg_identity_matrix)(VkvgContext ctx);
	};

#ifdef __cplusplus
}
#endif



// 混合模式
enum class blendmode_e :int {
	none = -1,
	normal = 0,	// 普通混合
	additive,
	multiply,
	modulate,
	screen,
	normal_prem,	// 预乘alpha
	additive_prem,
};


class vkvg_ctx
{
public:
	VkvgContext ctx = 0;
	VkvgSurface _surf = 0;
	vkvg_func_t* fun = {};

public:
	vkvg_ctx(VkvgSurface surf);
	~vkvg_ctx();

	void transform(const glm::mat3* matrix);
	void transform(const glm::mat3x2* matrix);
	void set_matrix(const  glm::mat3* matrix);
	void set_matrix(const  glm::mat3x2* matrix);
	glm::mat3x2 get_matrix();
private:

};


class vkvg_dev
{
public:
	VkvgDevice dev = 0;
	VkDevice vkdev = 0;
	vkvg_func_t* fun = {};
	VkSampleCountFlags samplecount = {};
	void* memoryProperties = 0;
public:
	vkvg_dev();
	~vkvg_dev();
	vkvg_func_t* get_fun();

	VkvgSurface new_surface(int width, int height);
	VkvgSurface new_surface(uint32_t* data, int width, int height);
	VkvgSurface new_surface(void* vkimg);
	VkvgSurface new_surface(const char* fn);
	void free_surface(VkvgSurface p);
	vkvg_ctx* new_ctx(VkvgSurface p);
	vkvg_ctx* new_context(VkvgSurface p);
	void free_ctx(vkvg_ctx* p);

	void update_image(VkvgSurface image, uint32_t* data, int width, int height);

	// 直接获取VkImage指针
	void* get_vkimage(void* surface);
	void image_save(void* surface, const char* fn);
	glm::ivec2 get_image_data(void* surface, std::vector<uint32_t>* opt);
private:

};

struct dev_info_c
{
	VkInstance inst;
	VkPhysicalDevice phy;
	VkDevice vkdev;
	uint32_t qFamIdx;		// familyIndex
	uint32_t qIndex = 0;
};
// 64,32,16,8,4,2,1
vkvg_dev* new_vkvgdev(dev_info_c* c = 0, int sample = 8);
void free_vkvgdev(vkvg_dev* p);

void submit_style(VkvgContext cr, fill_style_d* st);
void draw_rounded_rectangle(VkvgContext cr, double x, double y, double width, double height, const glm::vec4& r);
void draw_triangle(VkvgContext cr, const glm::vec2& pos, const glm::vec2& size, const glm::vec2& dirspos);
void draw_arrow2(VkvgContext ctx, float x, float y);
// type=0线终点在三角形中间
void draw_arrow(VkvgContext ctx, const glm::vec2& p0, const glm::vec2& p1, float arrow_hwidth, float arrow_size, bool type = 0);

// 画网格填充
void draw_grid_fill(VkvgContext cr, const glm::vec2& ss, const glm::ivec2& cols, int width);
// 画线性渐变填充
void draw_linear(VkvgContext cr, const glm::vec2& ss, const glm::vec4* cols, int count);


/*
渲染命令函数，
ctx：渲染上下文指针VkvgContext
cmds：命令数组
count：命令数量
data：命令数据指针
size：数据大小
order：渲染顺序，0=默认，数值越小，越靠前
*/
void vgc_draw_cmds(void* ctx, uint8_t* cmds, size_t count, void* data, size_t size, vg_style_data* style);
// 绘制路径函数
void vgc_draw_path(void* ctx, path_d* path, fill_style_d* style);
/*
批量渲染块(圆或矩形)
坐标数组point，count数量
rc的xy宽高。如果y为0，则表示绘制圆形，x为直径
*/
struct dblock_d {
	glm::vec2* points = 0; int count = 0; glm::vec2 rc = {};
	glm::vec2 pos = {};			// 块偏移，受scale_pos影响
	glm::vec2 view_pos = {};	// 视图偏移，不受scale_pos影响
	float scale_pos = 0;		// 视图缩放，不缩放线宽
};
void vgc_draw_block(void* ctx, dblock_d* p, fill_style_d* style);

// SDL渲染器专用
typedef struct texture_cb texture_cb;
struct text_vx
{
	glm::vec2 pos;
	glm::vec2 uv;
	glm::vec4 color;
};
struct sdl3_textdata
{
	std::map<image_ptr_t*, void*> vt;
	std::vector<text_vx> opt; std::vector<uint32_t> idx;
	texture_cb* rcb = 0;
	void* tex = 0;
	void* rptr = 0;
	uint32_t color = 0;
};
// 渲染一组图文列表
void r_render_data(layout_tx* p, const glm::vec2& pos, sdl3_textdata* pt);
// 渲染一段文本
void r_update_data_text(text_render_o* p, sdl3_textdata* pt, float delta);
void r_render_data_text(text_render_o* p, const glm::vec2& pos, sdl3_textdata* pt);
// 释放渲染器的纹理
void r_render_free_tex(sdl3_textdata* p);


#ifdef __cplusplus
extern "C" {
#endif

	typedef void* rLibrary;
	typedef void* rObject;
	typedef void* rDevice;
	typedef void* rCamera;
	typedef void* rArray;
	typedef void* rArray1D;
	typedef void* rArray2D;
	typedef void* rArray3D;
	typedef void* rFrame;
	typedef void* rFuture;
	typedef void* rGeometry;
	typedef void* rGroup;
	typedef void* rInstance;
	typedef void* rLight;
	typedef void* rMaterial;
	typedef void* rSampler;
	typedef void* rSurface;
	typedef void* rRenderer;
	typedef void* rSpatialField;
	typedef void* rVolume;
	typedef void* rWorld;

	typedef int rDataType;
	typedef int rStatusSeverity;
	typedef int rStatusCode;
	typedef uint32_t rWaitMask;

	typedef struct {
		const char* name;
		rDataType type;
	} rParameter;

	typedef void (*rMemoryDeleter)(const void* userPtr, const void* appMemory);
	typedef void (*rStatusCallback)(const void* userPtr, rDevice device, rObject source, rDataType sourceType, rStatusSeverity severity, rStatusCode code, const char* message);
	typedef void (*rFrameCompletionCallback)(const void* userPtr, rDevice device, rFrame frame);
	/*
		关系图
		Frame		: Camera(1:1)、World(1:1)、Renderer(1:1)
		World		: Instance(1:N)、Surface(1:N)、Light(1:N)
		Instance	: Group(1:1)	属性名transform、group。一个Instance只能绑定一个Group
		Group		: Surface(1:N)、Light(1:N)
		Surface		: Geometry(1:1)、Material(1:1)
		Geometry	: transform、动画等属性

		rLibrary(*LoadLibrary)(const char* name, rStatusCallback statusCallback, const void* statusCallbackUserData);
		void (*UnloadLibrary)(rLibrary module);
		void (*LoadModule)(rLibrary library, const char* name);
		void (*UnloadModule)(rLibrary library, const char* name);
	*/

	struct rdev_impl
	{
		rLibrary library = 0;
		rDevice(*NewDevice)(rLibrary library, const char* type/* = "default"*/);
		rArray1D(*NewArray1D)(rDevice device, const void* appMemory, rMemoryDeleter deleter, const void* userData, rDataType dataType, uint64_t numElements1);
		rArray2D(*NewArray2D)(rDevice device, const void* appMemory, rMemoryDeleter deleter, const void* userData, rDataType dataType, uint64_t numElements1, uint64_t numElements2);
		rArray3D(*NewArray3D)(rDevice device, const void* appMemory, rMemoryDeleter deleter, const void* userData, rDataType dataType, uint64_t numElements1, uint64_t numElements2, uint64_t numElements3);
		void* (*MapArray)(rDevice device, rArray array);
		void (*UnmapArray)(rDevice device, rArray array);
		rLight(*NewLight)(rDevice device, const char* type);
		rCamera(*NewCamera)(rDevice device, const char* type);
		rGeometry(*NewGeometry)(rDevice device, const char* type);
		rSpatialField(*NewSpatialField)(rDevice device, const char* type);
		rVolume(*NewVolume)(rDevice device, const char* type);
		rSurface(*NewSurface)(rDevice device);
		rMaterial(*NewMaterial)(rDevice device, const char* type);
		rSampler(*NewSampler)(rDevice device, const char* type);
		rGroup(*NewGroup)(rDevice device);
		rInstance(*NewInstance)(rDevice device, const char* type);
		rWorld(*NewWorld)(rDevice device);
		rObject(*NewObject)(rDevice device, const char* objectType, const char* type);
		void (*SetParameter)(rDevice device, rObject object, const char* name, rDataType dataType, const void* mem);
		void (*UnsetParameter)(rDevice device, rObject object, const char* name);
		void (*UnsetAllParameters)(rDevice device, rObject object);
		void* (*MapParameterArray1D)(rDevice device, rObject object, const char* name, rDataType dataType, uint64_t numElements1, uint64_t* elementStride);
		void* (*MapParameterArray2D)(rDevice device, rObject object, const char* name, rDataType dataType, uint64_t numElements1, uint64_t numElements2, uint64_t* elementStride);
		void* (*MapParameterArray3D)(rDevice device, rObject object, const char* name, rDataType dataType, uint64_t numElements1, uint64_t numElements2, uint64_t numElements3, uint64_t* elementStride);
		void (*UnmapParameterArray)(rDevice device, rObject object, const char* name);
		void (*CommitParameters)(rDevice device, rObject object);
		void (*Release)(rDevice device, rObject object);
		void (*Retain)(rDevice device, rObject object);
		const char** (*GetDeviceSubtypes)(rLibrary library);
		const char** (*GetDeviceExtensions)(rLibrary library, const char* deviceSubtype);
		const char** (*GetObjectSubtypes)(rDevice device, rDataType objectType);
		const void* (*GetObjectInfo)(rDevice device, rDataType objectType, const char* objectSubtype, const char* infoName, rDataType infoType);
		const void* (*GetParameterInfo)(rDevice device, rDataType objectType, const char* objectSubtype, const char* parameterName, rDataType parameterType, const char* infoName, rDataType infoType);
		int (*GetProperty)(rDevice device, rObject object, const char* name, rDataType type, void* mem, uint64_t size, rWaitMask mask);
		rFrame(*NewFrame)(rDevice device);
		const void* (*MapFrame)(rDevice device, rFrame frame, const char* channel, uint32_t* width, uint32_t* height, rDataType* pixelType);
		void (*UnmapFrame)(rDevice device, rFrame frame, const char* channel);
		rRenderer(*NewRenderer)(rDevice device, const char* type);
		void (*RenderFrame)(rDevice device, rFrame frame);
		int (*FrameReady)(rDevice device, rFrame frame, rWaitMask mask);
		void (*DiscardFrame)(rDevice device, rFrame frame);
	};
	typedef struct rdev_impl rdev_impl;
	// 创建lib获取渲染接口输出到opt
	rLibrary loadRLibrary(rdev_impl* opt);
	void unloadRLibrary(rLibrary m);
#ifdef __cplusplus
} // extern "C"

namespace vkr {
	/*
		对象：Camera、World、Renderer、Frame、Instance、Group、Surface、Geometry、Material、Light
		Frame		: Camera(1:1)、World(1:1)、Renderer(1:1)
		World		: Instance(1:N)、Surface(1:N)、Light(1:N)
		Instance	: Group(1:1)	属性名transform、group。一个Instance只能绑定一个Group
		Group		: Surface(1:N)、Light(1:N)，相当一个完整的模型
		Surface		: Geometry(1:1)、Material(1:1)
		Geometry	: transform、动画等属性，网格数据，一个节点

		场景渲染至少需要：camera、light、world、renderer、frame、instance 
	*/
	class vDevice
	{
	public:
		rLibrary lib = 0;
		rdev_impl* rcb = 0;
		rDevice dev = 0;
	public:
		vDevice();
		~vDevice();
		rDevice init(rLibrary library, const char* type/* = "default"*/);
		rArray1D newArray1D(const void* appMemory, rMemoryDeleter deleter, const void* userData, rDataType dataType, uint64_t numElements1);
		rArray2D newArray2D(const void* appMemory, rMemoryDeleter deleter, const void* userData, rDataType dataType, uint64_t numElements1, uint64_t numElements2);
		rArray3D newArray3D(const void* appMemory, rMemoryDeleter deleter, const void* userData, rDataType dataType, uint64_t numElements1, uint64_t numElements2, uint64_t numElements3);
		void* mapArray(rArray array);
		void  unmapArray(rArray array);
		// 创建光源
		rLight newLight(const char* type);
		// 创建相机
		rCamera newCamera(const char* type);
		// 创建几何体
		rGeometry newGeometry(const char* type);
		// 创建表面实体，支持绑定一个几何体和材质
		rSurface newSurface();
		// 创建材质，绑定纹理或颜色等参数
		rMaterial newMaterial(const char* type);
		// 创建纹理采样器
		rSampler newSampler(const char* type);
		// 创建组，包含多个表面和光源，支持直接导入gltf模型
		rGroup newGroup();
		// 创建实例，绑定一个组，支持变换属性
		rInstance newInstance(const char* type);
		// 创建世界，包含多个实例、表面和光源
		rWorld newWorld();
		// 创建以上对象
		rObject newObject(const char* objectType, const char* type);
		// 设置参数
		void  setParameter(rObject object, const char* name, rDataType dataType, const void* mem);
		// 取消设置参数
		void  unsetParameter(rObject object, const char* name);
		// 取消设置所有参数
		void  unsetAllParameters(rObject object);
		// 输入对象，映射参数数组
		void* mapParameterArray1D(rObject object, const char* name, rDataType dataType, uint64_t numElements1, uint64_t* elementStride);
		void* mapParameterArray2D(rObject object, const char* name, rDataType dataType, uint64_t numElements1, uint64_t numElements2, uint64_t* elementStride);
		void* mapParameterArray3D(rObject object, const char* name, rDataType dataType, uint64_t numElements1, uint64_t numElements2, uint64_t numElements3, uint64_t* elementStride);
		void  unmapParameterArray(rObject object, const char* name);
		// 提交参数设置
		void  commitParameters(rObject object);
		// 释放对象
		void  release(rObject object);
		// 引用对象
		void  retain(rObject object);
		// 获取设备子类型
		const char** getObjectSubtypes(rDataType objectType);
		// 获取对象信息
		const void* getObjectInfo(rDataType objectType, const char* objectSubtype, const char* infoName, rDataType infoType);
		// 获取参数信息
		const void* getParameterInfo(rDataType objectType, const char* objectSubtype, const char* parameterName, rDataType parameterType, const char* infoName, rDataType infoType);
		// 获取属性
		int  getProperty(rObject object, const char* name, rDataType type, void* mem, uint64_t size, rWaitMask mask);
		// 创建帧缓冲
		rFrame newFrame();
		// 映射帧数据
		const void* mapFrame(rFrame frame, const char* channel, uint32_t* width, uint32_t* height, rDataType* pixelType);
		// 解除映射帧数据
		void  unmapFrame(rFrame frame, const char* channel);
		// 创建渲染器
		rRenderer newRenderer(const char* type);
		// 提交帧渲染
		void  renderFrame(rFrame frame);
		// 帧渲染完成检查
		int  frameReady(rFrame frame, rWaitMask mask);
		// 丢弃帧
		void  discardFrame(rFrame frame);
	};
	// 可输入显卡名称创建vDevice对象
	vDevice* new_vdevice(const char* devname);
	void free_vdevice(vDevice* p);

}
// !vkr

#endif
