/*
文本渲染：fontconfig、stb_truetype、harfBuzz、icu
矢量图渲染： vkvg
窗口：SDL3

文本渲染数据：图集+三角形
视图纹理：前景、背景各一张。文本渲染在中间

*/
#include <pch1.h>
#include <random>
#include <pnguo/win_core.h>
#include "win32msg.h"
#include <pnguo/event.h>
//#define GUI_STATIC_LIB
#include <pnguo/pnguo.h>
#include <pnguo/tinysdl3.h>
#include <pnguo/app.h>
#include <pnguo/vkrenderer.h>
#include <pnguo/gpu_vk.h>
#include <pnguo/page.h>
#include <pnguo/mapView.h>
#include <pnguo/print_time.h> 
#include <pnguo/mesh3d.h>
#include "mshell.h"
//#include <cgltf.h>
#include <mcut/stlrw.h> 
#include <spine/spine-sdl3/spinesdl3.h>
#include <pnguo/render.h>
#include <pnguo/win_core.h>

#include <SDL3/SDL.h>
#include <pnguo/plot.h>
auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman";// , Malgun Gothic";

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

//void new_ui(form_x* form0, vkdg_cx* vkd) {
//	auto p = new plane_cx();
//	uint32_t pbc = 0xc02c2c2c;
//	p->set_color({ 0x80ff802C,1,5,pbc });
//	//form0->bind(p);	// 绑定到窗口  
//	p->set_rss(5);
//	p->_lms = { 6,6 };
//	p->add_familys(fontn, 0);
//	p->draggable = true; //可拖动
//	p->set_size({ 320,660 });
//	p->set_pos({ 1000,100 });
//	p->fontsize = 16;
//	int width = 16;
//	int rcw = 14;
//	{
//		// 设置带滚动条
//		p->set_scroll(width, rcw, { 0, 0 }, { 2,0 }, { 0,2 });
//		p->set_scroll_hide(1);
//	}
//	p->set_view({ 320,660 }, { 600, 660 });
//	glm::vec2 bs = { 150,22 };
//	glm::vec2 bs1 = { 50,22 };
//	std::vector<std::string> boolstr = { "TAA", "LightFrustum",	"BoundingBoxes","ShowMilliseconds" };
//	if (vkd)
//	{
//		std::vector<bool*> bps = { &vkd->_state.bUseTAA, &vkd->_state.bDrawLightFrustum, &vkd->_state.bDrawBoundingBoxes, &vkd->_state.bShowMilliseconds };
//		for (size_t i = 0; i < boolstr.size(); i++)
//		{
//			auto& it = boolstr[i];
//			auto kcb = p->add_label(it.c_str(), bs, 0);
//			{
//				kcb->_disabled_events = true;
//				kcb->text_color = 0xff7373ff;
//				auto sw1 = (switch_tl*)p->add_switch(bs1, it.c_str(), *(bps[i]));
//				//sw1->_disabled_events = true;
//				sw1->get_pos();
//				sw1->bind_ptr(bps[i]);
//			}
//		}
//		std::vector<color_btn*>* pbs = new std::vector<color_btn*>();
//		bs.x = 300;
//		auto& lbs = *pbs;
//		for (size_t i = 0; i < 30; i++)
//		{
//			auto kcb = p->add_label("", bs, 0);
//			kcb->text_color = 0xff80F61F;
//			kcb->_disabled_events = true;
//			lbs.push_back(kcb);
//		}
//	}
//
//
//	uint32_t* cc = get_wcolor() + 8;
//	for (int i = 0; i < 0 * 8; i++) {
//		auto p = new plane_cx();
//		uint32_t tc = cc[i];
//		p->set_color({ 0x80ff802C,1,5,0xff2c2c2c });
//		//form0->bind(p);	// 绑定到窗口  
//		p->set_rss(5);
//		p->_lms = { 6,6 };
//		p->add_familys(fontn, 0);
//		p->draggable = true; //可拖动
//		p->set_size({ 500,300 });
//		p->set_pos({ 100 + i * 20,100 + i * 20 });
//		for (size_t j = 0; j < boolstr.size(); j++)
//		{
//			auto& it = boolstr[j];
//			auto kcb = p->add_label(it.c_str(), bs, 0);
//			{
//				kcb->_disabled_events = true;
//				kcb->text_color = tc;
//				auto sw1 = (switch_tl*)p->add_switch(bs1, it.c_str(), 0);
//				sw1->get_pos();
//				//sw1->bind_ptr(bps[j]);
//			}
//		}
//	}
//}
//
//void show_cpuinfo(form_x* form0)
//{
//	cpuinfo_t cpuinfo = get_cpuinfo();
//	if (!form0)return;
//	auto p = new plane_cx();
//	uint32_t pbc = 0xf02c2c2c;
//	p->set_color({ 0x80ff802C,1,5,pbc });
//	//form0->bind(p);	// 绑定到窗口  
//	p->set_rss(5);
//	p->_lms = { 8,8 };
//	p->add_familys(fontn, 0);
//	p->draggable = true; //可拖动
//	p->set_size({ 420,660 });
//	p->set_pos({ 100,100 });
//	p->fontsize = 16;
//	int width = 16;
//	int rcw = 14;
//	{
//		// 设置带滚动条
//		p->set_scroll(width, rcw, { 0, 0 }, { 2,0 }, { 0,2 });
//		p->set_scroll_hide(1);
//	}
//	// 视图大小，内容大小
//	p->set_view({ 420,660 }, { 420, 660 });
//	glm::vec2 bs = { 150,22 };
//	glm::vec2 bs1 = { 50,22 };
//	std::vector<std::string> ncsstr = { (char*)u8"CPU信息","NumLogicalCPUCores","CPUCacheLineSize","SystemRAM","SIMDAlignment" };
//	std::vector<std::string> boolstr = { "AltiVec","MMX","SSE","SSE2","SSE3","SSE41","SSE42","AVX","AVX2","AVX512F","ARMSIMD","NEON","LSX","LASX" };
//
//	static std::vector<color_btn*> lbs;
//	bs.x = 300;
//	uint32_t txtcolor = 0xfff2f2f2;// 0xff7373ff;
//	int64_t ds[] = { 0, cpuinfo.NumLogicalCPUCores,cpuinfo.CPUCacheLineSize,cpuinfo.SystemRAM ,cpuinfo.SIMDAlignment };
//	{
//		int i = 0;
//		for (auto& it : ncsstr)
//		{
//			std::string txt = vkr::format("%-20s: %lld", it.c_str(), ds[i]);
//			auto tc = txtcolor;
//			if (i == 0) {
//				txt = it + ": " + cpuinfo.name;
//				tc = 0xffF6801F;
//			}
//			i++;
//			auto kcb = p->add_label(txt, bs, 0);
//			kcb->text_color = tc;
//			kcb->_disabled_events = true;
//			lbs.push_back(kcb);
//		}
//	}
//	bool* bps = &cpuinfo.AltiVec;
//	bs.x = 160;
//	for (size_t i = 0; i < boolstr.size(); i++)
//	{
//		auto& it = boolstr[i];
//		auto kcb = p->add_label(it.c_str(), bs, 0);
//		{
//			kcb->_disabled_events = true;
//			kcb->text_color = txtcolor;
//			auto sw1 = (switch_tl*)p->add_switch(bs1, it.c_str(), bps[i]);
//			sw1->_disabled_events = true;
//			sw1->get_pos();
//			kcb = p->add_label("", bs, 0);
//			kcb->_disabled_events = true;
//		}
//	}
//}
#include <entt/entt.hpp>
struct node_t
{
	int16_t type, count;//种类、数量
};
struct belt_t
{
	node_t* p = 0;	// 节点
	size_t n = 0;	// 节点数量
	glm::ivec2 pos[3] = {};// 曲线
	belt_t* link_ptr = 0;	// 尾部连接
	size_t link_idx = 0;		// 连接位置
};
// 传送带
class belt_cx
{
public:
	std::vector<entt::entity> lines;
	glm::vec2 pos = { 0.5,0 };
	int w = 56;
	uint32_t linecolor = 0xcc222222;
	std::vector<glm::vec2> bline;
	std::vector<int> bline_idx;
	int dxx = 0;
	double ts = 0.0;
public:
	belt_cx();
	~belt_cx();

	void update(float delta);
	void draw(void* cr);
private:

};

belt_cx::belt_cx()
{
	int n = w * 10 / 6.0; size_t nc = 0;
	int nx = w / 6;
	int xx = 0;

	{
		for (size_t i = 0; i < n; i++, nc++)
		{
			bline.push_back({ xx + i * nx,1 });
			bline.push_back({ xx + i * nx,w - 2 });
			bline_idx.push_back(i + nc);
			bline_idx.push_back(i + nc + 1);
			bline_idx.push_back(-1);
		}
	}

}

belt_cx::~belt_cx()
{}
void belt_cx::update(float delta) {
	ts += delta;
	if (ts > 0.12 * 0.5)
	{
		dxx++;
		ts = 0;
	}
	if (dxx > 7)dxx = 0;
}
#ifdef _CR__
void belt_cx::draw(cairo_t* cr) {

	for (size_t i = 0; i < 10; i++)
	{
		draw_rect(cr, { i * w,0,w,w }, 0xf0cccccc, 0xffff802C, 2, 1);
	}
	auto ps = pos;
	ps.x += dxx;
	// 裁剪区域
	cairo_rectangle(cr, 0, 0, 10 * w, w);
	cairo_clip(cr);
	draw_polylines(cr, ps, bline.data(), bline.size(), bline_idx.data(), bline_idx.size(), linecolor, 1);
}
#endif 


void test_img() {

	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr.png", 0);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr10.jpg", 10);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr30.jpg", 30);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr60.jpg", 60);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr80.jpg", 80);
}
glm::vec4 p2v(float yaw, float pitch)
{
	return glm::vec4(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch), 0);
}
#if 0
vec3 AMDTonemapper(vec3 color) {}//太长了0
vec3 DX11DSK(vec3 color)
{
	float  MIDDLE_GRAY = 0.72f;
	float  LUM_WHITE = 1.5f;
	// Tone mapping
	color.rgb *= MIDDLE_GRAY;
	color.rgb *= (1.0f + color / LUM_WHITE);
	color.rgb /= (1.0f + color);
	return color;
}
vec3 Reinhard(vec3 color)
{
	return color / (1 + color);
}
vec3 Uncharted2TonemapOp(vec3 x)
{
	float A = 0.15;	float B = 0.50;	float C = 0.10;	float D = 0.20;	float E = 0.02;	float F = 0.30;
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}
vec3 Uncharted2Tonemap(vec3 color)
{
	float W = 11.2;
	return Uncharted2TonemapOp(2.0 * color) / Uncharted2TonemapOp(vec3(W));
}
vec3 tonemapACES(vec3 x)
{
	float a = 2.51;	float b = 0.03;	float c = 2.43;	float d = 0.59;	float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}
//case 0: return AMDTonemapper(color);
//case 1: return DX11DSK(color);
//case 2: return Reinhard(color);
//case 3: return Uncharted2Tonemap(color);
//case 4: return tonemapACES(color);
//case 5: return color;	// 不做色调映射
#endif

// 定义顶点结构
struct Vertex_c {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

// 生成圆角立方体的函数
void GenerateRoundedCube(
	float size,
	float radius,
	int segments,
	std::vector<Vertex_c>& vertices,
	std::vector<unsigned int>& indices)
{
	vertices.clear();
	indices.clear();

	// 确保半径不超过立方体大小的一半
	radius = glm::min(radius, size * 0.5f);

	// 立方体的核心尺寸（减去圆角后的部分）
	float coreSize = size - 2.0f * radius;

	// 立方体的8个角点（未圆角前的原始角点）
	glm::vec3 corners[8] = {
		glm::vec3(-0.5f * coreSize, -0.5f * coreSize, -0.5f * coreSize),
		glm::vec3(0.5f * coreSize, -0.5f * coreSize, -0.5f * coreSize),
		glm::vec3(0.5f * coreSize,  0.5f * coreSize, -0.5f * coreSize),
		glm::vec3(-0.5f * coreSize,  0.5f * coreSize, -0.5f * coreSize),
		glm::vec3(-0.5f * coreSize, -0.5f * coreSize,  0.5f * coreSize),
		glm::vec3(0.5f * coreSize, -0.5f * coreSize,  0.5f * coreSize),
		glm::vec3(0.5f * coreSize,  0.5f * coreSize,  0.5f * coreSize),
		glm::vec3(-0.5f * coreSize,  0.5f * coreSize,  0.5f * coreSize)
	};

	// 生成每个角的球面部分
	for (int corner = 0; corner < 8; ++corner) {
		// 确定当前角的符号（决定球面部分的方向）
		glm::vec3 sign(
			(corner & 1) ? 1.0f : -1.0f,
			(corner & 2) ? 1.0f : -1.0f,
			(corner & 4) ? 1.0f : -1.0f
		);

		// 当前角的中心点
		glm::vec3 cornerCenter = corners[corner] + sign * radius;

		// 生成球面部分的顶点
		int baseIndex = vertices.size();

		for (int i = 0; i <= segments; ++i) {
			float phi = i * glm::pi<float>() / (2.0f * segments);

			for (int j = 0; j <= segments; ++j) {
				float theta = j * glm::pi<float>() / (2.0f * segments);

				// 计算球面上的点
				glm::vec3 spherePoint(
					radius * sin(phi) * cos(theta),
					radius * sin(phi) * sin(theta),
					radius * cos(phi)
				);

				// 根据当前角的方向调整点的位置
				spherePoint.x *= sign.x;
				spherePoint.y *= sign.y;
				spherePoint.z *= sign.z;

				// 计算最终顶点位置
				Vertex_c vertex;
				vertex.position = cornerCenter + spherePoint;
				vertex.normal = glm::normalize(spherePoint);

				// 简单的纹理坐标映射
				vertex.texCoord = glm::vec2(
					static_cast<float>(j) / segments,
					static_cast<float>(i) / segments
				);

				vertices.push_back(vertex);
			}
		}

		// 生成球面部分的索引
		for (int i = 0; i < segments; ++i) {
			for (int j = 0; j < segments; ++j) {
				int idx = baseIndex + i * (segments + 1) + j;

				indices.push_back(idx);
				indices.push_back(idx + 1);
				indices.push_back(idx + segments + 1);

				indices.push_back(idx + 1);
				indices.push_back(idx + segments + 2);
				indices.push_back(idx + segments + 1);
			}
		}
	}

	// 生成连接角点的平面部分（这里需要为6个面分别生成平面）
	// 由于实现较为复杂，这里只提供了球面部分的实现
	// 完整的实现还需要生成连接这些球面部分的平面区域

	// 注意：完整的圆角立方体实现需要生成:
	// 1. 8个角上的球面部分（已实现）
	// 2. 12条边上的圆柱面部分
	// 3. 6个面上的平面部分
}

// 水管截面参数
struct PipeProfile {
	float outerRadius;
	float innerRadius;
	int segments; // 截面细分段数
};

// 水管路径点
struct PathPoint {
	glm::vec3 position;
	glm::vec3 direction; // 归一化的方向向量
};

// 生成水管网格
class PipeGenerator {
public:
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<unsigned int> indices;

	// 生成直线水管
	void generateStraightPipe(const glm::vec3& start, const glm::vec3& end,
		const PipeProfile& profile) {
		glm::vec3 direction = glm::normalize(end - start);
		float length = glm::distance(start, end);

		generatePipeSegment(start, direction, length, profile);
	}

	// 生成转角水管（带圆角）
	void generateBendPipe(const glm::vec3& start, const glm::vec3& bendCenter,
		const glm::vec3& end, float bendRadius,
		const PipeProfile& profile, int bendSegments = 8) {
		// 计算起始方向
		glm::vec3 startDir = glm::normalize(bendCenter - start);
		glm::vec3 endDir = glm::normalize(end - bendCenter);

		// 生成直线部分
		float straightLength = glm::distance(start, bendCenter) - bendRadius;
		if (straightLength > 0) {
			generatePipeSegment(start, startDir, straightLength, profile);
		}

		// 生成弯曲部分
		// 计算弯曲平面和角度
		glm::vec3 bendAxis = glm::normalize(glm::cross(startDir, endDir));
		float bendAngle = acos(glm::dot(startDir, endDir));

		if (bendAngle > 0.01f) { // 避免除零和微小角度
			// 生成弯曲段
			for (int i = 0; i <= bendSegments; i++) {
				float t = static_cast<float>(i) / bendSegments;
				float angle = t * bendAngle;

				// 旋转方向向量
				glm::vec3 dir = glm::rotate(startDir, angle, bendAxis);

				// 计算位置
				glm::vec3 centerOffset = startDir * bendRadius;
				glm::vec3 pos = bendCenter - centerOffset + glm::rotate(centerOffset, angle, bendAxis);

				// 生成截面
				generateCrossSection(pos, dir, profile);
			}
		}

		// 生成结束直线部分
		straightLength = glm::distance(bendCenter, end) - bendRadius;
		if (straightLength > 0) {
			generatePipeSegment(bendCenter + endDir * bendRadius, endDir, straightLength, profile);
		}
	}

private:
	// 生成管道段
	void generatePipeSegment(const glm::vec3& start, const glm::vec3& direction,
		float length, const PipeProfile& profile) {
		// 开始截面
		generateCrossSection(start, direction, profile);

		// 结束截面
		generateCrossSection(start + direction * length, direction, profile);

		// 连接两个截面形成管道
		connectSections(profile.segments);
	}

	// 生成管道截面
	void generateCrossSection(const glm::vec3& center, const glm::vec3& direction,
		const PipeProfile& profile) {
		// 计算局部坐标系
		glm::vec3 up(0.0f, 1.0f, 0.0f);
		if (glm::abs(glm::dot(direction, up)) > 0.99f) {
			up = glm::vec3(0.0f, 0.0f, 1.0f);
		}

		glm::vec3 right = glm::normalize(glm::cross(direction, up));
		up = glm::normalize(glm::cross(right, direction));

		// 存储当前截面起始索引
		size_t startIndex = vertices.size();

		// 生成内外圆顶点
		for (int i = 0; i <= profile.segments; i++) {
			float angle = static_cast<float>(i) / profile.segments * glm::two_pi<float>();

			// 计算圆上的点
			glm::vec2 circlePoint(std::cos(angle), std::sin(angle));

			// 外圆顶点
			glm::vec3 outerPos = center + right * (circlePoint.x * profile.outerRadius)
				+ up * (circlePoint.y * profile.outerRadius);
			vertices.push_back(outerPos);
			normals.push_back(glm::normalize(right * circlePoint.x + up * circlePoint.y));

			// 内圆顶点
			glm::vec3 innerPos = center + right * (circlePoint.x * profile.innerRadius)
				+ up * (circlePoint.y * profile.innerRadius);
			vertices.push_back(innerPos);
			normals.push_back(glm::normalize(-(right * circlePoint.x + up * circlePoint.y)));
		}

		// 如果是第一个截面，不需要连接，否则连接前一个截面
		if (startIndex > 0) {
			connectSections(profile.segments);
		}
	}

	// 连接两个截面
	void connectSections(int segments) {
		size_t prevStart = vertices.size() - 2 * (segments + 1);
		size_t currStart = vertices.size() - (segments + 1) * 2;

		for (int i = 0; i < segments; i++) {
			// 外表面四边形
			indices.push_back(prevStart + i * 2);
			indices.push_back(prevStart + (i + 1) * 2);
			indices.push_back(currStart + i * 2);

			indices.push_back(prevStart + (i + 1) * 2);
			indices.push_back(currStart + (i + 1) * 2);
			indices.push_back(currStart + i * 2);

			// 内表面四边形
			indices.push_back(prevStart + i * 2 + 1);
			indices.push_back(currStart + i * 2 + 1);
			indices.push_back(prevStart + (i + 1) * 2 + 1);

			indices.push_back(prevStart + (i + 1) * 2 + 1);
			indices.push_back(currStart + i * 2 + 1);
			indices.push_back(currStart + (i + 1) * 2 + 1);

			// 连接内外圆的侧面（管壁）
			indices.push_back(prevStart + i * 2);
			indices.push_back(currStart + i * 2);
			indices.push_back(prevStart + i * 2 + 1);

			indices.push_back(currStart + i * 2);
			indices.push_back(currStart + i * 2 + 1);
			indices.push_back(prevStart + i * 2 + 1);
		}
	}
};

// 示例用法
int maing() {
	PipeGenerator generator;
	PipeProfile profile{ 1.0f, 0.8f, 16 }; // 外径1.0，内径0.8，16段细分

	// 生成直线水管
	generator.generateStraightPipe(
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(5.0f, 0.0f, 0.0f),
		profile
	);

	// 生成带圆角的转角水管
	generator.generateBendPipe(
		glm::vec3(5.0f, 0.0f, 0.0f),
		glm::vec3(7.0f, 0.0f, 0.0f),
		glm::vec3(7.0f, 3.0f, 0.0f),
		1.5f, // 弯曲半径
		profile,
		12 // 弯曲段细分
	);

	std::cout << "Generated pipe with " << generator.vertices.size()
		<< " vertices and " << generator.indices.size() / 3
		<< " triangles." << std::endl;

	return 0;
}
// 示例使用
int mainc() {
	std::vector<Vertex_c> vertices;
	std::vector<unsigned int> indices;

	// 生成立方体，大小为2.0，圆角半径为0.2，分段数为8
	GenerateRoundedCube(2.0f, 0.2f, 8, vertices, indices);

	// 这里可以添加代码将顶点和索引数据上传到GPU进行渲染

	return 0;
}
void test_vkvg(const char* fn, dev_info_c* dc);
struct listm_t
{
	void* data = 0;
	int cap_count = 0;	// 容量
	int count = 0;		// 当前使用数量
	listm_t* next = 0;	// 下一个节点
};

uint32_t arc4random() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<uint32_t> dis(0, UINT32_MAX);
	return dis(gen);
}
void fill(size_t pos, struct kpair* val, void* dat)
{

	val->x = pos;
	val->y = arc4random() / (double)UINT32_MAX;
}
VkvgSurface plot_main(drawable_cx* ctx, int width, int height)
{
	struct kpair	 points1[50], points2[50];
	struct kdata* d1, * d2;
	struct kplot* p;

	VkvgSurface surf;
	size_t		 i;
	VkvgContext cr;
	vkvg_status_t	 st;
	int		 rc;

	rc = EXIT_FAILURE;

	d1 = d2 = NULL;
	p = NULL;

	for (i = 0; i < 50; i++) {
		points1[i].x = points2[i].x = (i + 1) / 50.0;
		points1[i].y = log((i + 1) / 50.0);
		points2[i].y = -log((i + 1) / 50.0) + points1[0].y;
	}

	if (NULL == (d1 = kdata_array_alloc(points1, 50))) {
		perror(NULL);
		goto out;
	}
	else if (NULL == (d2 = kdata_array_alloc(points2, 50))) {
		perror(NULL);
		goto out;
	}
	else if (NULL == (p = kplot_alloc(NULL))) {
		perror(NULL);
		goto out;
	}

	struct kdatacfg	 cfg;
	{

		kdatacfg_defaults(&cfg);
		cfg.line.clr.type = KPLOTCTYPE_PATTERN;
		cfg.line.clr.pattern = vkvg_pattern_create_linear(0.0, 0.0, 600.0, 400.0);
		st = vkvg_pattern_status(cfg.line.clr.pattern);
		if (VKVG_STATUS_SUCCESS != st) {
			fprintf(stderr, "%s", vkvg_status_to_string(st));
			vkvg_pattern_destroy(cfg.line.clr.pattern);
			kplot_free(p);
			return 0;
		}
		vkvg_pattern_add_color_stop_rgba(cfg.line.clr.pattern, 0.25, 1.0, 0.0, 0.0, 1.0);
		vkvg_pattern_add_color_stop_rgba(cfg.line.clr.pattern, 0.5, 0.0, 1.0, 0.0, 1.0);
		vkvg_pattern_add_color_stop_rgba(cfg.line.clr.pattern, 0.75, 0.0, 0.0, 1.0, 1.0);

		auto d = kdata_bucket_alloc(0, 100);
		assert(NULL != d);
		for (i = 0; i < 100; i++)
			kdata_bucket_set(d, i, i, i);
		//if (!kplot_attach_data(p, d, KPLOT_LINES, &cfg)) {
		//	perror(NULL);
		//	goto out;
		//}

		vkvg_pattern_destroy(cfg.line.clr.pattern);

		kdata_destroy(d);
		double		 u;
		d = kdata_hist_alloc(0.0, 1.0, 100);
		assert(NULL != d);
		for (i = 0; i < 4000; i++) {
			u = -log(1.0 - (arc4random() / (double)UINT32_MAX)) / 2.5;
			kdata_hist_add(d, u, 1.0);
		}
		kplot_attach_smooth(p, d, KPLOT_LINES, NULL, KSMOOTH_CDF, NULL);
	}
	//else if (!kplot_attach_data(p, d1, KPLOT_LINES, NULL)) {
	//	perror(NULL);
	//	goto out;
	//}
	//else if (!kplot_attach_data(p, d2, KPLOT_POINTS, NULL)) {
	//	perror(NULL);
	//	goto out;
	//}
	{
		enum kplottype	 ts[2];
		ts[0] = KPLOT_POINTS;
		ts[1] = KPLOT_LINES;
		kdata* md[] = { d1,d2 };
		//kplot_attach_datas(p, 2, md, ts, NULL, KPLOTS_YERRORBAR);
	}
	kdata_destroy(d1);
	kdata_destroy(d2);
	d1 = d2 = NULL;
	surf = (VkvgSurface)ctx->new_surface(width, height);

	st = vkvg_surface_status(surf);
	if (VKVG_STATUS_SUCCESS != st) {
		fprintf(stderr, "%s", vkvg_status_to_string(st));
		vkvg_surface_destroy(surf);
		kplot_free(p);
		return 0;
	}

	cr = vkvg_create(surf);
	//vkvg_surface_destroy(surf);

	st = vkvg_status(cr);
	if (VKVG_STATUS_SUCCESS != st) {
		fprintf(stderr, "%s", vkvg_status_to_string(st));
		vkvg_destroy(cr);
		kplot_free(p);
		return 0;

	}

	{
		grid_info_t g = {};
		g.width = 200;
		g.count = 10;
		g.linewidth = 1;
		g.back_color = 0xff353535;
		g.inline_color = 0xff404040;
		g.line_color = 0xff181818;

		vkvg_set_source_rgb(cr, 1.0, 1.0, 1.0);
		vkvg_rectangle(cr, 0.0, 0.0, 1200.0, 1200.0);
		vkvg_fill(cr);
		vkvg_save(cr);
		vkvg_translate(cr, 0, 0);
		for (size_t i = 0; i < 6; i++)
		{
			vkvg_save(cr);
			vkvg_translate(cr, 0, g.width * i);
			for (size_t x = 0; x < 6; x++) {
				draw_grid(cr, &g, ctx->vgcb);
				vkvg_translate(cr, g.width, 0);
			}
			vkvg_restore(cr);
		}
		vkvg_restore(cr);

		//kplot_draw(p, 600.0, 400.0, cr);

		kplot_free(p);
	}
	{
		auto d = kdata_array_alloc(NULL, 20);
		assert(NULL != d);
		auto c = kdata_array_fill(d, NULL, fill);
		assert(c);

		if (NULL == (p = kplot_alloc(NULL))) {
			perror(NULL);
			goto out;
		}

		if (!kplot_attach_data(p, d, KPLOT_LINESPOINTS, NULL)) {
			perror(NULL);
			goto out;
		}

		if (!kplot_attach_smooth(p, d, KPLOT_LINESPOINTS,
			NULL, KSMOOTH_MOVAVG, NULL)) {
			perror(NULL);
			goto out;
		}

		kdata_destroy(d);
		vkvg_translate(cr, 600.0, 0.0);
		vkvg_set_source_rgb(cr, 1.0, 1.0, 1.0);
		vkvg_rectangle(cr, 1.0, 0.0, 600.0, 400.0);
		vkvg_fill(cr);
		kplot_draw(p, 600.0, 400.0, cr);
	}
	vkvg_flush(cr);
	vkvg_surface_resolve(surf);
	st = vkvg_surface_write_to_png
	(vkvg_get_target(cr), "temp/example0_1.png");

	if (VKVG_STATUS_SUCCESS != st) {
		fprintf(stderr, "%s", vkvg_status_to_string(st));
		vkvg_destroy(cr);
		kplot_free(p);
		return surf;
	}

	vkvg_destroy(cr);
	rc = EXIT_SUCCESS;
out:
	kplot_free(p);
	kdata_destroy(d1);
	kdata_destroy(d2);

	return surf;
}


int main()
{
	auto k = time(0);
	main_heightmap(k);
	Sleep(1000);
	if (1) {
		//hz::main_ssh2();
		//return 0;
		//test_img();

		auto app = new_app();
		// 常用分辨率
		glm::ivec2 dpis[] = {
			{1024,768},
			{1152,864},
			{1280,720},
			{1280,768},
			{1280,800},
			{1280,960},
			{1280,1024},
			{1360,768},
			{1366,768},
			{1400,1050},
			{1440,900},
			{1600,900},
			{1680,1050},
			{1920,1080},
		};
		// 渲染fbo尺寸比例从50%到200%,步长5%
		int fbo_scale[3] = { 50,200,5 };
#ifdef _WIN32
#ifdef _DEBUG
		system("rd /s /q E:\\temcpp\\SymbolCache\\tcmp.pdb");
		system("rd /s /q E:\\temcpp\\SymbolCache\\vkcmp.pdb");
		system("rd /s /q E:\\temcpp\\SymbolCache\\cedit.pdb");
		system("rd /s /q E:\\temcpp\\SymbolCache\\p86.pdb");
		//auto rd = hz::shared_load(R"(E:\Program Files\RenderDoc_1.37_64\renderdoc.dll)");

#endif 
#endif // _WIN32


		auto fctx = app->font_ctx;
		//auto ksun = fctx->get_font((char*)u8"新宋体", 0);
		//auto seg = fctx->get_font((char*)u8"Segoe UI Emoji", 0);
		//auto sues = fctx->add2file(R"(data\seguiemj.ttf)", 0);
		//auto sue = sues[0]; 
		//std::vector<font_t*> familys = { ksun ,seg };


		fctx->set_cache_size(600, 1024);

		std::string k8a = (char*)u8"أَبْجَدِيَّة عَرَبِيَّة➗😊😎😭\n💣🚩❓❌\t🟦⬜👨‍👨‍👧qb ab我\n的大刀";

		std::string k8aaa0 = (char*)u8"👨‍👨‍👧q";
		std::string k80 = (char*)u8"👪️q";
		std::string k8 = (char*)u8"➗😊😎😭\n💣🚩❓❌\t🟦⬜👨‍👨‍👧qb abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ我\n的大刀";
		std::string k821 = (char*)u8"abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ\t我\n的大刀,Malgun Gothic";
		auto family = new_font_family(fctx, (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Consolas");
		auto rctts = get_text_rect(family, 16, k80.c_str(), k80.size(), 0);
		auto bll = font_get_lineheight(family, 16);
		//text_style ts = {};
		//ts.family = family;
		//ts.fontsize = 16;
		//ts.color = 0xfffF5000;
		//// 文本块
		//text_block tb = {};
		//tb.style = &ts;
		//tb.str = k8.c_str();
		//tb.first = 0;
		//tb.size = k8.size();
		//text_render_o trt = {};
		//trt.box.rc = { 0,0,300,500 };
		//trt.box.auto_break = 1;
		//trt.box.word_wrap = 0;
		//build_text_render(&tb, &trt);
		std::vector<uint32_t> vd;
		image_ptr_t dst = {};
		glm::ivec2 imgsize = { 500,100 };
		if (vd.size() < imgsize.x * imgsize.y)
		{
			vd.resize(imgsize.x * imgsize.y);
		}
		dst.data = vd.data();
		dst.width = imgsize.x; dst.height = imgsize.y;
		dst.comp = 4; dst.stride = dst.width * sizeof(uint32_t);
		//c_render_data(&trt, &dst);
		//delete_font_family(family);
		uint32_t* pcolor = get_wcolor();
		for (size_t i = 0; i < 16; i++)
		{
			auto str = get_wcname(i, 0);
			printf("\x1b[01;3%dm%s\x1b[0m\n", (int)i % 8, str);
		}

		glm::ivec2 ws = dpis[4];// { 1280, 860 };
		const char* wtitle = (char*)u8"窗口0";
		const char* wtitle1 = (char*)u8"窗口1";
		auto devname = "Intel(R)";
		devname = 0;
		adevice3_t* gd = new_gdev(0, 0);
		auto dctx = (vkg::cxDevice*)gd->new_device(gd->ctx, 0, 0, 0, 0);
		dev_info_cx devinfo = {};
		get_dev_info(dctx, &devinfo);

		vkdg_cx* vkd = new_vkdg(devinfo.inst, devinfo.phy, devinfo.vkdev, 0, 0, devname);	// 创建vk渲染器 

		auto sampler = gd->new_object((aDevice*)dctx, (int)obj_type_e::OBJ_SAMPLER, "sampler");	// 创建vk渲染器对象

		// 准备使用3D渲染器的设备创建SDL渲染器
		app->set_dev(vkd->_dev_info.inst, vkd->_dev_info.phy, vkd->_dev_info.vkdev);

		auto f = glm::mat4(1.4420948028564453, 0.0, 0.0, 0.0, 0.0, -0.00016932648114409524, 1.4420948028564453, 0.0, 0.0, -1.4420948028564453, -0.00016932648114409524, 0.0, 0.0, 0.0, 0.0, 1.0);
		auto f1 = glm::mat4(0.009999999776482582, 0.0, 0.0, 0.0, 0.0, -0.009999999776482582, 0.0, 0.0, 0.0, 0.0, -0.009999999776482582, 0.0, 0.0, 0.0, 0.0, 1.0);
		auto f2 = f * f1;

		glm::mat2x2 aaa;
		//vkr::new_ms_pipe(vkd->_dev_info.vkdev, vkd->renderpass_opaque);
		form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable /*| ef_vsync| ef_borderless*/);
		//form_x* form1 = (form_x*)new_form(app, wtitle1, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable);
		auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
		app->main = form0;
		auto kd = sdldev.vkdev;

		//void* aaaa = vkr::new_ms_pipe(vkd->dev, vkd->renderpass_fbo);

		//sdldev.vkdev = 0;	// 清空使用独立创建逻辑设备
		auto devs = vkr::get_devices(sdldev.inst); // 获取设备名称列表
		bool grab_enable = false;	// 设置鼠标范围在窗口内
		bool rmode = true;			// 设置窗口的相对鼠标模式。
		//form0->set_mouse_mode(grab_enable, rmode);
		get_queue_info(sdldev.phy);
		// 菜单窗口
		//form_x* popw = new_form_popup(form0, 200, 600);

		//form_x* ttw = new_form1(app, 200, 600, 0);
		//popw->set_pos({ -100,00 });
		//ttw->set_pos({ 50,100 });
		//popw->render_cb = [=](SDL_Renderer* renderer, double delta)
		//	{
		//		SDL_FRect rc = { 0,0,180,580 };
		//		SDL_SetRenderDrawColorFloat(renderer, 0.90f, 0.10f, 0.0f, 0.8f);
		//		SDL_RenderFillRect(renderer, &rc);
		//	};

		//ttw->render_cb = [=](SDL_Renderer* renderer, double delta)
		//	{
		//		SDL_FRect rc = { 0,0,180,580 };
		//		SDL_SetRenderDrawColorFloat(renderer, 0.10f, 0.90f, 0.0f, 0.8f);
		//		SDL_RenderFillRect(renderer, &rc);
		//	};
		//ttw->mmove_type = 1;
		//popw->mmove_type = 1;
		//popw->_focus_lost_hide = 0; 
		bool shadowMap = true;
		if (vkd) {
			auto rmd = hz::read_json("data/vksample.json");
			auto& scenes = rmd["scenes"];
			if (scenes.empty())
			{
				{
					njson item;
					item["name"] = "busterDrone";
					item["dir"] = R"(E:\model\testgltf\)";
					item["filename"] = "busterDrone.gltf";
					item["pos"] = { 0.0,1.8,0.0 };
					item["scale"] = 1.0;
					item["shadowMap"] = true;
					item["visible"] = true;
					scenes.push_back(item);
				}
				{
					njson item;
					item["name"] = "droide";
					item["dir"] = R"(E:\model\testgltf\)";
					item["filename"] = "free_droide_de_seguridad_k-2so_by_oscar_creativo.glb";
					item["pos"] = { 0.0,0.0,0.0 };
					item["scale"] = 10.0;
					item["shadowMap"] = true;
					item["visible"] = true;
					scenes.push_back(item);
				}
				hz::save_json("data/vksample.json", rmd, 2);
			}
			for (auto& it : scenes)
			{
				std::string path = it["dir"];
				std::string fn = it["filename"];
				if (fn.empty() || !hz::toBool(it["visible"]))continue;
				if (path.size()) {
					char xx = *path.rbegin();
					char fx = fn[0];
					if (!(xx == '\\' || xx == '/') && !(fx == '\\' || fx == '/')) {
						path += "/";
					}
				}
				path += fn;
				if (hz::access_2(path.c_str()))
				{
					auto pos = hz::toVec3(it["pos"]);
					vkd->add_gltf(path.c_str(), pos, hz::toDouble(it["scale"], 1.0), hz::toUInt(it["instanceCount"], 1.0), hz::toBool(it["shadowMap"]));
				}
			}
			vkd->resize(ws);				// 设置fbo缓冲区大小
			auto vki = vkd->get_vkimage(0);	// 获取fbo纹理弄到窗口显示 nullptr;//
			/*
			case 0: return AMDTonemapper(color);
			case 1: return DX11DSK(color);
			case 2: return Reinhard(color);
			case 3: return Uncharted2Tonemap(color);
			case 4: return tonemapACES( color );
			case 5: return color;
			*/
			vkd->_state.SelectedTonemapperIndex = 0;
			vkd->_state.Exposure = 0.8;
			vkd->_state.EmissiveFactor = 1;
			vkd->_state.IBLFactor = 0.5;
			//new_ui(form0, vkd); 
			//void* vg2dtex = nullptr;
			const char* filename = "temp/vkvg_gb.png";
			bspline_ct* bs = new bspline_ct();
			std::vector<glm::vec2> pts = { {100,500},{200,600},{300,400},{400,700},{500,500} };
			auto bptr = bs->new_bspline(pts.data(), pts.size());

			auto view = new viewdev_cx();
			view->init_vgdev(&devinfo, 8);
			view->familys = family;
			view->app = app;
			auto pcb = view->pcb;
			drawable_cx* td3 = new drawable_cx();
			td3->init(form0, pcb, { 0,0,ws.x, ws.y }, view->_vgdev);
			td3->familys = family;

			//auto ptm = plot_main(td3, 1200, 1200);


			void* tex3d = pcb->new_texture_vk(form0->renderer, vki.size.x, vki.size.y, vki.vkimage, 0);// 创建SDL的rgba纹理 
			pcb->set_texture_blend(tex3d, 0, 0);


			auto dvv = new div_cx();
			dvv->set_size({ 500,400 });
			dvv->set_pos({ 100,60 });
			dvv->family = family;
			dvv->border = { 0xffacacac,1,5,0x9f666666 };	// 颜色，线粗，圆角，背景色
			dvv->flex.wrap = flex_wrap::WRAP;
			dvv->flex.direction = flex_direction::ROW;
			dvv->flex.justify_content = flex_align::ALIGN_START;	// x轴，主轴对齐
			dvv->flex.align_items = flex_align::ALIGN_CENTER;		// y轴，交叉轴对齐
			dvv->flex.align_content = flex_align::ALIGN_START;		// y轴，多行交叉轴对齐
			dvv->flex_child.margin_left = 2;		// 子元素外边距
			dvv->flex_child.margin_right = 2;
			dvv->flex_child.margin_top = 2;
			dvv->flex_child.margin_bottom = 2;
			dvv->draggable = true;
			//dvv->flex.direction = flex_direction::COLUMN;
			uint32_t colors[5] = { 0x905050fc,0x9050fc50,0x90fc5050,0x90ffffff,0x90282828 };
			//x=默认，y=鼠标进入，z=按下
			glm::uvec3 gradTop = { 0xff4a4a4a,0x80404040,0xff292929 }, gradBot = { 0xff3a3a3a,0x80303030,0xff1d1d1d };
			glm::uvec3 gradTop1 = { 0xff8a8a8a,0x80bebebe,0xff303030 }, gradBot1 = { 0xff5a5a5a,0x80303030,0xff1d1d1d };
			glm::uvec2 blackb = { 0x805c5c5c , 0x801d1d1d };
#if 1
			for (int i = 0; i < 5; i++) {
				auto btn = new gradient_btn();
				btn->rounding = 4;
				dvv->add_widget(btn);
				btn->set_size({ 200,36 });
				btn->back_color = colors[i];
				btn->borderLight = blackb.x;
				btn->borderDark = blackb.y;
				btn->gradTop = gradTop;
				btn->gradBot = gradBot;
				btn->font_size = 16;
				btn->text_color = -1;
				btn->str = (char*)u8"🔥按钮gbutton " + std::to_string(i);
				if (i == 4) {
					//btn->borderLight = blackb.y;
					//btn->borderDark = blackb.x;
					btn->gradTop = gradTop1;
					btn->gradBot = gradBot1;
				}
			}
			auto gr = new group_radio_t();
			for (int i = 0; i < 5; i++) {
				auto r = new radio_tl();
				r->gr = gr;
				r->set_size({ 36,36 });
				r->style.line_col = 0xffff8020;
				r->style.thickness = 1;
				r->style.radius = 7;
				dvv->add_widget(r);
			}
			for (int i = 0; i < 7; i++) {
				auto btn = new color_btn();
				btn->rounding = 4;
				btn->set_btn_color_bgr(fmod(i, 5));
				dvv->add_widget(btn);
				btn->set_size({ 128,36 });
				btn->font_size = 16;
				btn->text_color = 0;
				btn->str = (char*)u8"🍕按钮 " + std::to_string(5 + i);
			}
			for (int i = 0; i < 4; i++) {
				auto r = new checkbox_tl();
				r->set_size({ 36,36 });
				r->style.line_col = 0xffff8020;
				r->style.thickness = 1;
				dvv->add_widget(r);
			}
			{
				auto r = new checkbox_tl();
				r->set_size({ 36,36 });
				r->style.line_col = 0xffff8020;
				r->style.thickness = 1;
				r->v.mixed = true;
				dvv->add_widget(r);
			}
#endif

			auto edit1 = new edit_cx();
			{
				auto r = edit1;
				r->set_size({ 236,32 });
				//r->set_single(false);
				dvv->add_widget(r);
				r->dindex = 0;
			}
			auto pro = new progress_tl();
			dvv->add_widget(pro);
			pro->set_size({ 120,26 });
			pro->right_inside = true;
			pro->rounding = 10;
			pro->height = 20;
			pro->width = 110;
			pro->color = { 0xffff9e40, 0x5f6c6c6c };
			pro->set_value(0.5);
			auto colorpick = new colorpick_tl();
			{
				auto c = colorpick;
				c->init(0xff282828, 300, 20, true);
				c->font_size = 15;
				dvv->add_widget(c);
			}
			static bool loadf = false;

			auto dvv2 = new div_cx();
			{
				auto dvv1 = new div_cx();
				dvv1->set_size({ 180,150 });
				dvv1->set_pos({ 100,60 });
				dvv1->family = family;
				dvv1->border = { 0xffacacac,1,5,0xaf66f666 };	// 颜色，线粗，圆角，背景色
				dvv1->flex_child.margin_left = 2;		// 子元素外边距
				dvv1->flex_child.margin_right = 2;
				dvv1->flex_child.margin_top = 2;
				dvv1->flex_child.margin_bottom = 2;
				dvv1->draggable = true;
				dvv1->_absolute = true;
				{
					auto btn = new color_btn();
					btn->rounding = 4;
					btn->set_btn_color_bgr(fmod(4, 5));
					dvv1->add_widget(btn);
					btn->set_size({ 128,36 });
					btn->font_size = 16;
					btn->text_color = -1;
					btn->str = (char*)u8"🍕按钮 ";
					btn->click_cb = [=](void* p, int clicks, const glm::vec2& mpos) {

						};
					{
						auto r = new checkbox_tl();
						r->set_size({ 36,36 });
						r->style.line_col = 0xffff8020;
						r->style.thickness = 1;
						r->v.mixed = true;
						dvv1->add_widget(r);
					}
					{
						auto r = new switch_tl();
						r->set_size({ 60,36 });
						r->set_value(true);
						r->v.mixed = true;
						dvv1->add_widget(r);
					}
				}
				dvv->add_widget(dvv1);
			}
			auto fpslab = new color_btn();
			{
				dvv2->set_size({ 500,400 });
				dvv2->set_pos({ 10,10 });
				dvv2->family = family;
				dvv2->border = { 0xF11212ac,1,5,0x50f66666 };	// 颜色，线粗，圆角，背景色
				dvv2->flex_child.margin_left = 4;		// 子元素外边距
				dvv2->flex_child.margin_right = 4;
				dvv2->flex_child.margin_top = 4;
				dvv2->flex_child.margin_bottom = 4;
				dvv2->draggable = true;
				dvv2->docking = true;
				dvv2->_absolute = true;
				td3->add_widget(dvv2);
				{
					auto btn = fpslab;
					btn->rounding = 4;
					btn->text_align = {};
					btn->set_btn_color_bgr(fmod(2, 5));
					btn->effect = uTheme::light;
					btn->_disabled_events = true;
					dvv2->add_widget(btn);
					btn->set_size({ 492,392 });
					btn->font_size = 16;
					btn->text_color = -1;
					/*btn->str = (char*)u8"🍕透明按钮 ";*/
				}
				dvv2->mevent_cb = [=](void* p, int type, const glm::vec2& mps)
					{
						if (type == (int)event_type2::on_dragend) {
							glm::vec2 pos = dvv2->get_pos();
							glm::vec2 size = dvv2->get_size();
							glm::vec2 main_pos = app->main->get_pos();
							glm::vec2 main_size = app->main->get_size();
							if (pos.x < 0 || pos.y < 0 || pos.x + size.x >  main_size.x || pos.y + size.y >  main_size.y)
							{
								//view->set_div(dvv2);
							}
						}
					};
			}
			td3->add_widget(dvv);
			auto colorpicker = new div_cx();
			{
				// 调色板

				auto dvv = colorpicker;
				td3->add_widget(dvv);
				dvv->set_size({ 1000,400 });
				dvv->set_pos({ 100,60 });
				dvv->family = family;
				dvv->border = { 0xffacacac,1,5,0xf9666666 };	// 颜色，线粗，圆角，背景色
				dvv->flex.wrap = flex_wrap::WRAP;
				dvv->flex.direction = flex_direction::ROW;
				dvv->flex.justify_content = flex_align::ALIGN_START;	// x轴，主轴对齐
				dvv->flex.align_items = flex_align::ALIGN_CENTER;		// y轴，交叉轴对齐
				dvv->flex.align_content = flex_align::ALIGN_START;		// y轴，多行交叉轴对齐
				dvv->flex_child.margin_left = 2;		// 子元素外边距
				dvv->flex_child.margin_right = 2;
				dvv->flex_child.margin_top = 2;
				dvv->flex_child.margin_bottom = 2;
				dvv->draggable = true;
			}
			render_update(*td3, 0);
			{
				app->e_window_cb = [=](form_x* fw, int type)
					{
						if (type == SDL_EVENT_WINDOW_MOVED)
						{
							//view->get_div(fw);
						}
					};
			}
#if 0
			form0->render_cb = [=](SDL_Renderer* renderer, double delta)
				{
					return;

				};
			form0->up_cb = [=](float delta, int* ret)
				{
					static double kti = 0.0;
					kti += delta;
					static glm::ivec3 sct = {};
					glm::ivec3 mps1 = { edit1->_cmpos,edit1->get_cursor_idx() };
					glm::ivec3 mps = { dvv->evupdate,0,0 };
					if (mps != sct)
					{
						sct = mps;
					}

				};
#endif
			app->set_fps(1000);
			vkd->_state.has_fence = false;
			std::string gpustr;
			std::vector<text_vx> vtx;
			std::vector<uint32_t> idx;
			glm::ivec4 rc = { 0,0,360,360 };
			glm::ivec2 psize = { 360,360 };
			glm::vec4 c1 = glm::vec4(0, 0, 0.8, 1.0);// 0xffcc0000;
			glm::vec4 ohsv = { 0.7,1,1,1 };
			glm::vec4 pick0 = c1, pick1 = c1;
			HSVtoRGB(ohsv, c1);
			auto vtx_pos = vtx.size();
			auto vtxd = vtx.data();
			{
				auto rect_mcolor = [](const glm::ivec4& rc, const glm::ivec4& color, std::vector<text_vx>& vtx, std::vector<uint32_t>& idx)
					{
						glm::vec2 uv = {};
						glm::vec2 a = { rc.x,rc.y }, c = { rc.x + rc.z,rc.y + rc.w };
						uint32_t vps = vtx.size();
						uint32_t ips = idx.size();
						vtx.resize(vps + 4);
						idx.insert(idx.end(), { vps + 0,vps + 1,vps + 2,vps + 0,vps + 2,vps + 3 });
						auto t = vtx.data() + vps;
						t->pos = a; t->uv = uv; t->color = ucolor2fx(color.x); t++;
						t->pos = glm::vec2(c.x, a.y); t->uv = uv; t->color = ucolor2fx(color.y); t++;
						t->pos = c; t->uv = uv; t->color = ucolor2fx(color.z); t++;
						t->pos = glm::vec2(a.x, c.y); t->uv = uv; t->color = ucolor2fx(color.w); t++;
					};
				auto rect_mcolor4f = [](const glm::ivec4& rc, const glm::vec4* color, std::vector<text_vx>& vtx, std::vector<uint32_t>& idx)
					{
						glm::vec2 uv = {};
						glm::vec2 a = { rc.x,rc.y }, c = { rc.x + rc.z,rc.y + rc.w };
						uint32_t vps = vtx.size();
						uint32_t ips = idx.size();
						vtx.resize(vps + 4);
						idx.insert(idx.end(), { vps + 0,vps + 1,vps + 2,vps + 0,vps + 2,vps + 3 });
						auto t = vtx.data() + vps;
						t->pos = a; t->uv = uv; t->color = (color[0]); t++;
						t->pos = glm::vec2(c.x, a.y); t->uv = uv; t->color = (color[1]); t++;
						t->pos = c; t->uv = uv; t->color = (color[2]); t++;
						t->pos = glm::vec2(a.x, c.y); t->uv = uv; t->color = (color[3]); t++;
					};
				auto rect_color4f = [](const glm::ivec4& rc, const glm::vec4& color, std::vector<text_vx>& vtx, std::vector<uint32_t>& idx)
					{
						glm::vec2 uv = {};
						glm::vec2 a = { rc.x,rc.y }, c = { rc.x + rc.z,rc.y + rc.w };
						uint32_t vps = vtx.size();
						uint32_t ips = idx.size();
						vtx.resize(vps + 4);
						idx.insert(idx.end(), { vps + 0,vps + 1,vps + 2,vps + 0,vps + 2,vps + 3 });
						auto t = vtx.data() + vps;
						t->pos = a; t->uv = uv; t->color = color; t++;
						t->pos = glm::vec2(c.x, a.y); t->uv = uv; t->color = color; t++;
						t->pos = c; t->uv = uv; t->color = color; t++;
						t->pos = glm::vec2(a.x, c.y); t->uv = uv; t->color = color; t++;
					};
				auto build_huedata = [=, &vtx_pos, &vtxd, &pick0, &pick1](const glm::vec4& incolor, const glm::ivec4& rc0, std::vector<text_vx>& vtx, std::vector<uint32_t>& idx) {
					std::vector<glm::vec4> color = { glm::vec4(1.0) ,incolor,incolor,glm::vec4(1.0) };
					auto rc = rc0;
					vtx.clear();
					idx.clear();
					rect_mcolor4f(rc, color.data(), vtx, idx);	//白色->颜色
					color = { glm::vec4(0.0),glm::vec4(0.0),glm::vec4(0,0,0,1) ,glm::vec4(0,0,0,1) };
					rect_mcolor4f(rc, color.data(), vtx, idx);	//透明->黑
					static const glm::vec4 col_hues[6 + 1] = { glm::vec4(1,0,0,1), glm::vec4(1,1,0,1), glm::vec4(0,1,0,1),
						glm::vec4(0,1,1,1), glm::vec4(0,0,1,1), glm::vec4(1,0,1,1), glm::vec4(1,0,0,1) };
					rc.x += rc.z + 4;
					auto rc1 = rc;
					auto fx = rc.z / 6.0;
					rc.z = fx;
					for (size_t i = 0; i < 6; i++)
					{
						glm::vec4 c4f[4] = { col_hues[i],col_hues[i + 1] };
						c4f[2] = c4f[1];
						c4f[3] = c4f[0];
						rect_mcolor4f(rc, c4f, vtx, idx);
						rc.x += fx;
					}
					rect_mcolor4f(rc1, color.data(), vtx, idx);	//透明->黑
					vtx_pos = vtx.size();
					rc1.x += rc1.z + 6;
					rc1.z = rc1.w = 50;
					rect_color4f(rc1, pick0, vtx, idx);	//颜色块
					rc1.x += 56;
					rect_color4f(rc1, pick1, vtx, idx);	//颜色块

					vtxd = vtx.data();
					};
				// 获取色盘颜色
				static auto get_color_cb = [](const glm::ivec2& pos, const glm::ivec2& size, float h) {
					glm::vec2 n = (glm::vec2)pos / (glm::vec2)size;
					glm::vec4 hc = {};
					glm::vec4 hsv = { h,0,0,1 };
					hsv.y = n.x;
					hsv.z = 1.0 - n.y;
					HSVtoRGB(hsv, hc);
					return hc;
					};
				// 获取色调颜色
				static auto get_hue_color_cb = [](const glm::ivec2& pos, const glm::ivec2& size) {
					glm::vec2 n = (glm::vec2)pos / (glm::vec2)size;
					glm::vec4 hc = {};
					glm::vec4 hsv = { n.x,0,0,1 };
					hsv.y = 1;
					hsv.z = 1.0 - n.y;
					HSVtoRGB(hsv, hc);
					return hc;
					};

				colorpicker->mevent_cb = [=, &pick0, &pick1, &vtx_pos, &vtx, &idx](void* p, int type, const glm::vec2& mpos)
					{
						if (type == (int)event_type2::on_drag) {
							glm::vec2 pos = colorpicker->get_pos() + 6;
							glm::ivec4 rc = { 0,0,psize };
							rc.x = pos.x;
							rc.y = pos.y;
							build_huedata(c1, rc, vtx, idx);
						}
						else if (type == (int)event_type2::on_down) {
							auto mps = mpos - (glm::vec2)colorpicker->get_pos();
							mps -= 6;
							if (mps.y > psize.y)
								return;
							auto d = vtx.data() + vtx_pos;
							if (mps.x < psize.x)
							{
								pick0 = get_color_cb(mps, psize, ohsv.x);
								for (size_t i = 0; i < 4; i++)
								{
									d[i].color = pick0;
								}
							}
							d += 4;
							mps.x -= 4 + psize.x;
							if (mps.x > 0 && mps.x < psize.x)
							{
								pick1 = get_hue_color_cb(mps, psize);
								for (size_t i = 0; i < 4; i++)
								{
									d[i].color = pick1;
								}
							}
							colorpicker->valid = true;
						}};

				glm::vec2 pos = colorpicker->get_pos() + 6;
				glm::ivec4 rc = { 0,0,psize };
				rc.x = pos.x;
				rc.y = pos.y;
				build_huedata(c1, rc, vtx, idx);
			}

			c_runtime_cx rtc; // 高精度计时器
			int uims = 0, ms3d = 0, SDLms = 0;
			// 运行消息循环			
			do {
				auto delta = app->update_event();
				if (!app->form_count())break;
				auto fps = app->get_fps();
				form0->update(delta);
				if (form0->is_minimized())
				{
					td3->pause();
					continue;
				}
				auto io = form0->get_io();
				// 判断鼠标是否点中控件，点中了就设置io->WantCaptureMouse = true，让3d渲染器不处理鼠标事件，交给控件处理
				if (td3->press_test() && io)
					io->WantCaptureMouse = true;

				edit1->_color.x = colorpick->get_color();
				vkd->update(form0->io);	// 更新事件
				gpustr = vkd->get_label();
				static double upt = 0.0;
				upt += delta;
				if (upt > 1.0)
				{
					upt = 0.0;
					fpslab->str = vkr::format("CPU FPS\t\t: %d\nUIcmd\t\t\t: %d ms\n3Dcmd\t\t\t: %d ms\nSDLms\t\t\t: %d ms\nCPUms\t\t\t: %d ms\n", fps, uims, ms3d, SDLms, uims + ms3d + SDLms) + gpustr;
				}
				rtc.begin();
				auto ct = td3->update(delta);
				uims = rtc.get_ms();
				int64_t sem3d = 0;
				{
					//rtc.begin();
					//vkd->on_render();		// 渲染到fbo纹理tex3d
					//if (!vkd->_state.has_fence) {
					//	sem3d = vkd->get_fbo_semaphore();
					//}
					//ms3d = rtc.get_ms();
					//ct++;
				}
				form0->add_vk_semaphores(sem3d, (int64_t)0, 0);
				if (ct) {
					form0->set_state();	// 清空/设置交换链接状态 
					//texture_dt tdt = {};
					//tdt.src_rect = { 0,0,vki.size.x,vki.size.y };
					//tdt.dst_rect = { 0,0,vki.size.x,vki.size.y };
					//if (tex3d) pcb->render_texture(form0->renderer, tex3d, &tdt, 1);//3d
					td3->cmd_draw();
					auto vd = vtx.data();
					td3->draw_geometry(0, glm::ivec4(colorpicker->get_pos(), colorpicker->get_size()), &vtx, &idx);
					form0->present();
					rtc.begin();		// 开始计时录制SDL渲染命令
					view->_vgdev->wait_dev();
					SDLms = rtc.get_ms();
				}
			} while (app->form_count());
			//run_app(app, 0);
			delete td3;
			delete view;

		}

		//show_belt(form0);
		//show_cpuinfo(form0);

		free_vkdg(vkd);
		gd->release(sampler);
		gd->release(dctx);
		free_gdev(gd);
		free_app(app);
	}
#ifdef _WIN32
	//ExitProcess(0);
#endif
	return 0;
}
