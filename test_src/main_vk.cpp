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



		std::string k8a = (char*)u8"أَبْجَدِيَّة عَرَبِيَّة➗😊😎😭\n💣🚩❓❌\t🟦⬜👨‍👨‍👧qb ab我\n的大刀";

		std::string k80 = (char*)u8"👨‍👨‍👧q";
		std::string k8 = (char*)u8"➗😊😎😭\n💣🚩❓❌\t🟦⬜👨‍👨‍👧qb abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ我\n的大刀";
		std::string k821 = (char*)u8"abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ\t我\n的大刀";
		auto family = new_font_family(fctx, (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Consolas,Malgun Gothic");

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
			auto pcb = new texture_cb();
			assert(pcb);
			get_sdl_texture_cb(pcb);
			//void* vg2dtex = nullptr;
			const char* filename = "temp/vkvg_gb.png";
			bspline_ct* bs = new bspline_ct();
			std::vector<glm::vec2> pts = { {100,500},{200,600},{300,400},{400,700},{500,500} };
			auto bptr = bs->new_bspline(pts.data(), pts.size());

			canvas2d_t* td3 = new canvas2d_t();
			auto rvgd = new rvg_data_cx();
			td3->set_renderer(form0->renderer, pcb, { 0,0,ws.x, ws.y });
			td3->init_vgdev(&devinfo, 8);
			td3->familys = family;
			void* tex3d = pcb->new_texture_vk(form0->renderer, vki.size.x, vki.size.y, vki.vkimage, 0);// 创建SDL的rgba纹理 
			pcb->set_texture_blend(tex3d, 0, 0);
			//VkvgSurface sf = td3->vgdev->new_surface(vki.vkimage, 0, vki.size.x, vki.size.y);
			std::string str = (char*)u8"这个例子实现了：矢量图渲染（基于vkvg），spine动画渲染，3D动画渲染！\nThis example demonstrates: vector graphics rendering (vkvg), Spine animation rendering, and 3D animation rendering!";
			text_box_t tbox = {};
			tbox.text_align = { 0,0.5 };
			tbox.rc = { 10,10,1500,600 };
			tbox.auto_break = 1;
			tbox.word_wrap = 1;

			auto ptb = new text_t1();
			text_t1_set(ptb, &tbox);
			auto ptb1 = new text_t1();
			auto mtext = new rich_text_t();
			//auto mrtext = new multi_rich_text_t();
			//text_style nst = {};
			//nst.family = family;
			//nst.fontsize = 20;

			//auto bidx = mrt_add_box(mrtext, { 400,20 }, { 850,300 });
			//mrt_add_text(mrtext, bidx, str.c_str(), str.size(), 0, &nst);
			//str = "abcdefg";
			//bidx = mrt_add_box(mrtext, { 400,200 }, { 850,300 });
			//mrt_add_text(mrtext, bidx, str.c_str(), str.size(), 0, &nst);
			//auto pbox = mrt_get_boxinfo(mrtext, 0);
			//pbox->auto_break = 1;
			//pbox->word_wrap = 1;
			//pbox->ellipsis;
			//mrt_build(mrtext);
			rt_set(mtext, &tbox);
			tbox.rc = { 10,320,1500,600 };
			text_t1_set(ptb1, &tbox);
			glm::ivec2 sc_size = { 1024,1024 };

			auto dvv = new div_cx();
			dvv->set_size({ 500,300 });
			dvv->set_pos({ 10,360 });
			dvv->flex.wrap = flex_wrap::WRAP;
			dvv->flex.direction = flex_direction::ROW;
			dvv->flex.justify_content = flex_align::ALIGN_START;	// x轴，主轴对齐
			dvv->flex.align_content = flex_align::ALIGN_START;		// y轴，多行交叉轴对齐
			dvv->flex.align_items = flex_align::ALIGN_START;		// y轴，交叉轴对齐

			dvv->flex_child.margin_left = 2;		// 子元素外边距
			dvv->flex_child.margin_right = 2;
			dvv->flex_child.margin_top = 2;
			dvv->flex_child.margin_bottom = 2;
			//dvv->flex.direction = flex_direction::COLUMN;
			uint32_t colors[5] = { 0x905050fc,0x9050fc50,0x90fc5050,0x90ffffff,0x90282828 };
			//x=默认，y=鼠标进入，z=按下
			glm::uvec3 gradTop = { 0xff4a4a4a,0x80404040,0xff292929 }, gradBot = { 0xff3a3a3a,0x80303030,0xff1d1d1d };
			glm::uvec3 gradTop1 = { 0xff8a8a8a,0x80bebebe,0xff303030 }, gradBot1 = { 0xff5a5a5a,0x80303030,0xff1d1d1d };
			glm::uvec2 blackb = { 0x805c5c5c , 0x801d1d1d };
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
				btn->str = (char*)u8"按钮gbutton " + std::to_string(i);
				if (i == 4) {
					//btn->borderLight = blackb.y;
					//btn->borderDark = blackb.x;
					btn->gradTop = gradTop1;
					btn->gradBot = gradBot1;
				}
			}
			auto gr = new group_radio_t();
			for (int i = 0; i < 2;i++) {
				auto r = new radio_tl();
				r->gr = gr;
				r->set_size({ 36,36 });
				r->style.line_col = 0xffff8020;
				r->style.thickness = 1;
				r->style.radius = 7;
				dvv->add_widget(r);
			}
			form0->add_event(dvv, [=](uint32_t type, et_un_t* e, void* ud) {
				auto div = (div_cx*)ud;
				div->on_event(type, e);
				});
			std::atomic_int wait2d = 0;

			//			std::thread jt([=, &wait2d]() {
			//
			//#if 1
			//				while (0) {
			//					if (wait2d == 0) {
			//						wait2d = 1;
			//					}
			//					Sleep(1);
			//				}
			//#endif
			//
			//				});
			//			jt.detach();
			form0->render_cb = [=](SDL_Renderer* renderer, double delta)
				{
					vkd->on_render();		// 渲染到fbo纹理tex3d
					auto sem = vkd->get_fbo_semaphore();
					form0->add_vk_semaphores(sem, 0, 0);
					texture_dt tdt = {};
					tdt.src_rect = { 0,0,vki.size.x,vki.size.y };
					tdt.dst_rect = { 0,0,vki.size.x,vki.size.y };
					if (tex3d) pcb->render_texture(renderer, tex3d, &tdt, 1);//3d
					td3->draw_surface(rvgd->surfaces[0].surface, { 600,20 }, glm::ivec4(0, 0, td3->get_size()), td3->get_size() * 0.5);
					td3->draw_textdata(mtext, { 0,0 });
					td3->draw_rvg(rvgd);
					//td3->draw_boxtext(mrt_get_box_index(mrtext, 0), {});
					//td3->draw_boxtext(mrt_get_box_index(mrtext, 1), {});
				};
			form0->up_cb = [=, &wait2d](float delta, int* ret)
				{
					int d = delta * 1000;
					auto light = vkd->get_light(0);
					vkd->_state.SelectedTonemapperIndex;	// 0-5: Tonemapper算法选择
					vkd->_state.Exposure;					// 曝光度：默认1.0
					vkd->_state.bUseTAA;
					vkd->_state.WireframeMode;				// 0 1 2 6
					static int ity = 6;
					light->_intensity = ity;
					dvv->update(delta);


					rvg_cx rvg;
					rvg.pos = dvv->get_pos();
					rvg.save();
					rvg.push_null(0);
					rvg.add_rect({ 0,0,dvv->get_size() }, 5);
					rvg.set_color(0xaf888888);
					rvg.fill();
					rvg.restore();
					dvv->draw(&rvg);
					td3->update_rvg(&rvg, rvgd);


					form0->io->WantCaptureMouse = dvv->press_test(form0->io->MousePos);
					vkd->update(form0->io);	// 更新事件
					static double kti = 0.0;
					kti += delta;
					if (kti > 0.1)
					{
						kti = 0.0;
						rt_clear(mtext);
						auto img = fctx->bcc._data[0];
						std::string str = vkd->get_label();
						str += (char*)u8"emoji表情💻🔥➗️👪️🍕";
						//rt_add_image(mtext, img, { 0,20,64,64 }, {}, -1, { 64 * 2,64 }, { 60,20 }, true);
						rt_add_text(mtext, str.c_str(), str.size(), 0, family, 16, 0xff222222);
						str = (char*)u8"渐变色表情: 💻🔥➗️👪️🍕";
						//str = (char*)u8"abcdefg";
						// 添加文本
						rt_add_text(mtext, str.c_str(), str.size(), 0, family, 32, 0xafffffff);
						//	rt_add_text(mtext, str.c_str(), str.size(), 0, family, 32, 0xaf0080ff);//添加不同字号和颜色的文本
						// 添加图片，提供图片对象、渲染位置\大小、九宫格设置、颜色混合、dsize渲染大小、是否固定坐标不参与布局等参数
						static glm::ivec2 imgpos = { 100,100 };
						//rt_add_image(mtext, img, { 0,20,64,64 }, { 10,10,26,26 }, 0x50ffffff, { 64 * 5,64 * 3 }, imgpos, true);
						//	rt_add_image(mtext, img, { 0,20,64,64 }, {}, -1, { 32,32 }, {}, false);

						rt_build(mtext);
						td3->update(mtext, 0);

						static bool save_test = false;
						if (save_test)
						{
							save_img_png(img, "temp/test_font_cache2026.png"); save_test = false;
						}
					}
					static bool savepng = false;
					auto afilename = savepng ? filename : 0;
					//test_drawvkvg(ctx, surf, bs, afilename);
					if (savepng)
					{
						savepng = false;
					}
					//app->wait_device();
					//app->wait_queue(0);
					wait2d = 0;
					//while (wait2d == 0) {
					//	Sleep(1);
					//}
				};
			{
				static const char* vertex_glsl = R"glsl(
layout(push_constant) uniform uPushConstant
//layout(binding=0) uniform u_UniformBuffer 
{
    mat4 u_mvpMatrix;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 col; 

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out struct{
    vec4 col;
    vec2 uv;
} o;

void main(){
    gl_Position = u_mvpMatrix * vec4(pos.xyz, 1);
    o.uv = uv;
    o.col = col;  
}
)glsl";
				static const char* fragment_glsl = R"glsl(
layout(binding=1) uniform sampler2D u_Texture;
layout(location = 0) in struct{
    vec4 col;
    vec2 uv;
} d;
layout(location = 0) out vec4 o_Color;
void main()
{
  vec4 c = texture(u_Texture, d.uv.st);
  o_Color = d.col * c; 
}

)glsl";
				std::string v1 = "#version 450\n";
				std::string f1 = "#version 450\n";
				//v1 += v;
				v1 += vertex_glsl;
				//f1 += f;
				f1 += fragment_glsl;
				//vkg::new_shader(dctx, f1.c_str(), v1.c_str(), 1);
			}
			// 运行消息循环
			run_app(app, 0);
			td3->free_rvg(rvgd);
			delete rvgd;
			delete ptb;
			delete td3;

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
