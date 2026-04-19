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

void new_ui(form_x* form0, vkdg_cx* vkd) {
	auto p = new plane_cx();
	uint32_t pbc = 0xc02c2c2c;
	p->set_color({ 0x80ff802C,1,5,pbc });
	//form0->bind(p);	// 绑定到窗口  
	p->set_rss(5);
	p->_lms = { 6,6 };
	p->add_familys(fontn, 0);
	p->draggable = true; //可拖动
	p->set_size({ 320,660 });
	p->set_pos({ 1000,100 });
	p->fontsize = 16;
	int width = 16;
	int rcw = 14;
	{
		// 设置带滚动条
		p->set_scroll(width, rcw, { 0, 0 }, { 2,0 }, { 0,2 });
		p->set_scroll_hide(1);
	}
	p->set_view({ 320,660 }, { 600, 660 });
	glm::vec2 bs = { 150,22 };
	glm::vec2 bs1 = { 50,22 };
	std::vector<std::string> boolstr = { "TAA", "LightFrustum",	"BoundingBoxes","ShowMilliseconds" };
	if (vkd)
	{
		std::vector<bool*> bps = { &vkd->_state.bUseTAA, &vkd->_state.bDrawLightFrustum, &vkd->_state.bDrawBoundingBoxes, &vkd->_state.bShowMilliseconds };
		for (size_t i = 0; i < boolstr.size(); i++)
		{
			auto& it = boolstr[i];
			auto kcb = p->add_label(it.c_str(), bs, 0);
			{
				kcb->_disabled_events = true;
				kcb->text_color = 0xff7373ff;
				auto sw1 = (switch_tl*)p->add_switch(bs1, it.c_str(), *(bps[i]));
				//sw1->_disabled_events = true;
				sw1->get_pos();
				sw1->bind_ptr(bps[i]);
			}
		}
		std::vector<color_btn*>* pbs = new std::vector<color_btn*>();
		bs.x = 300;
		auto& lbs = *pbs;
		for (size_t i = 0; i < 30; i++)
		{
			auto kcb = p->add_label("", bs, 0);
			kcb->text_color = 0xff80F61F;
			kcb->_disabled_events = true;
			lbs.push_back(kcb);
		}
	}


	uint32_t* cc = get_wcolor() + 8;
	for (int i = 0; i < 0 * 8; i++) {
		auto p = new plane_cx();
		uint32_t tc = cc[i];
		p->set_color({ 0x80ff802C,1,5,0xff2c2c2c });
		//form0->bind(p);	// 绑定到窗口  
		p->set_rss(5);
		p->_lms = { 6,6 };
		p->add_familys(fontn, 0);
		p->draggable = true; //可拖动
		p->set_size({ 500,300 });
		p->set_pos({ 100 + i * 20,100 + i * 20 });
		for (size_t j = 0; j < boolstr.size(); j++)
		{
			auto& it = boolstr[j];
			auto kcb = p->add_label(it.c_str(), bs, 0);
			{
				kcb->_disabled_events = true;
				kcb->text_color = tc;
				auto sw1 = (switch_tl*)p->add_switch(bs1, it.c_str(), 0);
				sw1->get_pos();
				//sw1->bind_ptr(bps[j]);
			}
		}
	}
}

void show_cpuinfo(form_x* form0)
{
	cpuinfo_t cpuinfo = get_cpuinfo();
	if (!form0)return;
	auto p = new plane_cx();
	uint32_t pbc = 0xf02c2c2c;
	p->set_color({ 0x80ff802C,1,5,pbc });
	//form0->bind(p);	// 绑定到窗口  
	p->set_rss(5);
	p->_lms = { 8,8 };
	p->add_familys(fontn, 0);
	p->draggable = true; //可拖动
	p->set_size({ 420,660 });
	p->set_pos({ 100,100 });
	p->fontsize = 16;
	int width = 16;
	int rcw = 14;
	{
		// 设置带滚动条
		p->set_scroll(width, rcw, { 0, 0 }, { 2,0 }, { 0,2 });
		p->set_scroll_hide(1);
	}
	// 视图大小，内容大小
	p->set_view({ 420,660 }, { 420, 660 });
	glm::vec2 bs = { 150,22 };
	glm::vec2 bs1 = { 50,22 };
	std::vector<std::string> ncsstr = { (char*)u8"CPU信息","NumLogicalCPUCores","CPUCacheLineSize","SystemRAM","SIMDAlignment" };
	std::vector<std::string> boolstr = { "AltiVec","MMX","SSE","SSE2","SSE3","SSE41","SSE42","AVX","AVX2","AVX512F","ARMSIMD","NEON","LSX","LASX" };

	static std::vector<color_btn*> lbs;
	bs.x = 300;
	uint32_t txtcolor = 0xfff2f2f2;// 0xff7373ff;
	int64_t ds[] = { 0, cpuinfo.NumLogicalCPUCores,cpuinfo.CPUCacheLineSize,cpuinfo.SystemRAM ,cpuinfo.SIMDAlignment };
	{
		int i = 0;
		for (auto& it : ncsstr)
		{
			std::string txt = vkr::format("%-20s: %lld", it.c_str(), ds[i]);
			auto tc = txtcolor;
			if (i == 0) {
				txt = it + ": " + cpuinfo.name;
				tc = 0xffF6801F;
			}
			i++;
			auto kcb = p->add_label(txt, bs, 0);
			kcb->text_color = tc;
			kcb->_disabled_events = true;
			lbs.push_back(kcb);
		}
	}
	bool* bps = &cpuinfo.AltiVec;
	bs.x = 160;
	for (size_t i = 0; i < boolstr.size(); i++)
	{
		auto& it = boolstr[i];
		auto kcb = p->add_label(it.c_str(), bs, 0);
		{
			kcb->_disabled_events = true;
			kcb->text_color = txtcolor;
			auto sw1 = (switch_tl*)p->add_switch(bs1, it.c_str(), bps[i]);
			sw1->_disabled_events = true;
			sw1->get_pos();
			kcb = p->add_label("", bs, 0);
			kcb->_disabled_events = true;
		}
	}
}
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
void show_belt(form_x* form0)
{
	if (!form0)return;
	glm::ivec2 size = { 1024,800 };
	glm::ivec2 cview = { 10240,10240 };
	auto p = new plane_cx();
	uint32_t pbc = 0xf02c2c2c;
	p->set_color({ 0x80ff802C,1,5,pbc });
	//form0->bind(p);	// 绑定到窗口  
	p->set_rss(5);
	p->_lms = { 8,8 };
	p->add_familys(fontn, 0);
	p->draggable = false; //可拖动
	p->set_size(size);
	p->set_pos({ 30,50 });
	p->on_click = [](plane_ev* e) {};
	p->fontsize = 16;

	size.y -= 50;
	size.x -= 50 * 10;
	size /= 2;

	auto dpx = p->push_dragpos(size);// 增加一个拖动坐标

	auto bp = new belt_cx();
	p->update_cb = [=](float delta)
		{

			bp->update(delta);
			return 1;
		};
#ifdef _CR__
	p->draw_back_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
			auto dps = p->get_dragpos(dpx);//获取拖动时的坐标
			cairo_translate(cr, dps.x, dps.y);
			{
				cairo_as _ss_(cr);
				bp->draw(cr);
			}
			std::string str = (char*)u8"🍉";
			glm::vec4 rc = { 0,0,200,56 };
			text_style_t st = {};
			st.font = 0;
			st.text_align = { 0.0,0.5 };
			st.font_size = 39;
			st.text_color = -1;
			auto rc1 = p->ltx->get_text_rect(st.font, st.font_size, str.c_str(), -1);
			auto rc2 = rc;
			//rc2.z = rc1.x; rc2.w = rc1.y;
			//rc2.y = (rc.w - rc1.y) * st.text_align.y;
			//draw_rect(cr, rc2, 0xf0222222, 0xff802Cff, 2, 1);
			draw_text(cr, p->ltx, str.c_str(), -1, rc, &st);
		};
#endif
}


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
#ifndef NOT_UI

//  cb;支持的type有on_move/on_scroll/on_drag/on_down/on_up/on_click/on_dblclick/on_tripleclick
struct widget_t
{
public:
	int id = 0;
	WIDGET_TYPE wtype = WIDGET_TYPE::WT_NULL;
	int _bst = 1;			// 鼠标状态
	glm::vec2 pos = {};		// 控件坐标
	glm::vec2 size = {};	// 控件大小
	glm::ivec2 curpos = {};	// 当前拖动鼠标坐标
	glm::ivec2 cmpos = {};	// 当前鼠标坐标
	glm::ivec2 mmpos = {};	// 当前鼠标坐标
	glm::ivec2 ppos = {};	// 父级坐标 
	std::string text;		// 内部显示用字符串
	std::string family;
	float font_size = 16;
	glm::vec2 text_align = { 0.5,.5 }; // 文本对齐
	int rounding = 4;		// 圆角
	float thickness = 1.0;	// 边框线粗

	std::function<void(uint32_t type, et_un_t* e, const glm::vec2& pos)> on_event_cb;	//自定义事件处理
	std::function<void(void* p, int type, const glm::vec2& mps)> mevent_cb;	//通用事件处理
	std::function<void(void* p, int clicks)> click_cb;						//左键点击事件
	int _clicks = 0;		// 点击数量
	layout_text_x* ltx = 0;

	glm::ivec2 hscroll = { 1,1 };// x=1则受水平滚动条影响，y=1则受垂直滚动条影响
	int _old_bst = 0;			// 鼠标状态
	int cks = 0;				// 鼠标点击状态
	widget_t* parent = 0;
	double dtime = 0.0;
	bool _disabled_events = false;
	bool visible = true;
	bool _absolute = false;		// true绝对坐标，false布局计算
	bool has_drag = false;	// 是否有拖动事件
	bool _autofree = false;
	bool has_hover_sc = 0;	// 滚动在父级接收
public:
	widget_t();
	widget_t(WIDGET_TYPE wt);
	virtual ~widget_t();
	//event_type2
	virtual bool on_mevent(int type, const glm::vec2& mps);
	virtual bool update(float delta);
	virtual void draw(rvg_cx* rv);
	virtual glm::ivec2 get_pos(bool has_parent = true);
	virtual glm::ivec2 get_spos();	// 滚动坐标
};
void widget_on_event(widget_t* p, uint32_t type, et_un_t* e, const glm::vec2& pos);
void send_hover(widget_t* wp, const glm::vec2& mps);

widget_t::widget_t()
{}
widget_t::widget_t(WIDGET_TYPE wt) :wtype(wt)
{}
widget_t::~widget_t()
{}
bool widget_t::on_mevent(int type, const glm::vec2& mps)
{
	return false;
}

bool widget_t::update(float delta)
{
	return false;
}

void widget_t::draw(rvg_cx* rv)
{}
glm::ivec2 widget_t::get_pos(bool has_parent)
{
	glm::ivec2 ps = pos;
	if (parent) {
		auto pss = parent->get_pos();
		auto ss = parent->get_spos();
		ss *= hscroll;
		ps += ss;
		if (has_parent) { ps += pss; }
	}
	return ps;
}

glm::ivec2 widget_t::get_spos()
{
	return glm::ivec2();
}


#endif // !NOT_UI


#if 1
// 通用控件鼠标事件处理 type有on_move/on_scroll/on_drag/on_down/on_up/on_click/on_dblclick/on_tripleclick
bool widget_on_move(widget_t* wp, uint32_t type, et_un_t* ep, const glm::vec2& pos) {
	bool hover = false;
	if (!wp)return hover;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	if (t == devent_type_e::mouse_move_e)
	{
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y }; mps -= pos;
		wp->mmpos = mps;
		// 判断是否鼠标进入 
		glm::vec4 trc = { wp->pos  ,wp->size };
		auto k = check_box_cr1(mps, &trc, 1, sizeof(glm::vec4));
		if (k.x) {
			bool hoverold = wp->_bst & (int)BTN_STATE::STATE_HOVER;
			wp->_bst |= (int)BTN_STATE::STATE_HOVER;   hover = true;
			if (!(wp->_bst & (int)BTN_STATE::STATE_ACTIVE))// 不是鼠标则独占
				ep->ret = 1;
			if (!hoverold)
			{
				// 鼠标进入
				wp->on_mevent((int)event_type2::on_enter, mps);
				if (wp->mevent_cb) {
					wp->mevent_cb(wp, (int)event_type2::on_enter, mps);
				}
			}
		}
		else {
			if (wp->_bst & (int)BTN_STATE::STATE_HOVER)
			{
				wp->_bst &= ~(int)BTN_STATE::STATE_HOVER;
				// 鼠标离开
				wp->on_mevent((int)event_type2::on_leave, mps);
				if (wp->mevent_cb) {
					wp->mevent_cb(wp, (int)event_type2::on_leave, mps);
				}
			}
		}

		{
			if (wp->_bst & (int)BTN_STATE::STATE_HOVER)
			{
				wp->on_mevent((int)event_type2::on_move, mps);
				if (wp->mevent_cb)
				{
					wp->mevent_cb(wp, (int)event_type2::on_move, mps);
				}
			}
			if (wp->_bst & (int)BTN_STATE::STATE_ACTIVE) {
				auto dps = mps - wp->curpos;
				wp->on_mevent((int)event_type2::on_drag, dps);		// 拖动事件
				if (wp->mevent_cb)
				{
					wp->mevent_cb(wp, (int)event_type2::on_drag, dps);		// 拖动事件
				}
			}
		}
	}
	return hover;
}

void widget_on_event(widget_t* wp, uint32_t type, et_un_t* ep, const glm::vec2& pos) {
	if (!wp)return;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	switch (t)
	{
	case devent_type_e::mouse_move_e:
		widget_on_move(wp, type, ep, pos);
		break;
	case devent_type_e::mouse_button_e:
	{
		auto p = e->b;
		glm::ivec2 mps = { p->x,p->y }; mps -= pos;
		bool isd = wp->cmpos == mps;
		wp->cmpos = mps;
		if (wp->_bst & (int)BTN_STATE::STATE_HOVER) {
			if (p->down == 1)
			{
				ep->ret = 1;
			}
			if (p->button == 1) {
				if (p->down == 1) {
					wp->_bst |= (int)BTN_STATE::STATE_ACTIVE;
					wp->curpos = mps - (glm::ivec2)wp->pos;
					wp->cks = 0;
					wp->on_mevent((int)event_type2::on_down, mps);
					if (wp->mevent_cb) { wp->mevent_cb(wp, (int)event_type2::on_down, mps); }
				}
				else {
					if ((wp->_bst & (int)BTN_STATE::STATE_ACTIVE) && (isd || !wp->has_drag))
					{
						wp->cks = p->clicks;
						wp->on_mevent((int)event_type2::on_up, mps);
						if (wp->mevent_cb) {
							wp->mevent_cb(wp, (int)event_type2::on_up, mps);
						}
						int tc = (int)event_type2::on_click; //左键单击
						if (p->clicks == 2) { tc = (int)event_type2::on_dblclick; }
						else if (p->clicks == 3) { tc = (int)event_type2::on_tripleclick; }
						wp->_clicks = p->clicks;
						wp->on_mevent(tc, mps);
						if (wp->mevent_cb) {
							wp->mevent_cb(wp, tc, mps);
						}
						if (wp->click_cb)
						{
							wp->click_cb(wp, p->clicks);
						}
					}
					wp->_bst &= ~(int)BTN_STATE::STATE_ACTIVE;
				}
			}
		}
		if (p->down == 0) {
			wp->_bst &= ~(int)BTN_STATE::STATE_ACTIVE;
			wp->_bst |= (int)BTN_STATE::STATE_NOMAL;
			wp->on_mevent((int)event_type2::mouse_up, mps);
			if (wp->mevent_cb) {
				wp->mevent_cb(wp, (int)event_type2::mouse_up, mps);
			}
		}
	}
	break;
	case devent_type_e::mouse_wheel_e:
	{
		auto p = e->w;
		glm::vec2 mps = { p->x, p->y };
		if (wp->_bst & (int)BTN_STATE::STATE_HOVER || wp->has_hover_sc)
		{
			ep->ret = wp->on_mevent((int)event_type2::on_scroll, mps);
			if (wp->mevent_cb) {
				wp->mevent_cb(wp, (int)event_type2::on_scroll, mps);
			}
			ep->ret = 1;
		}
	}
	break;
	case devent_type_e::keyboard_e:
	{
		// todo
		//on_keyboard(ep);
	}
	break;
	default:
		break;
	}

}
void send_hover(widget_t* wp, const glm::vec2& mps) {

	wp->on_mevent((int)event_type2::on_hover, mps);
	if (wp->mevent_cb)
	{
		wp->mevent_cb(wp, (int)event_type2::on_hover, mps);
	}
}

bool on_wpe(widget_t* pw, int type, et_un_t* ep, const glm::ivec2& ppos)
{
	bool r = false;
	auto e = &ep->v;
	auto t = (devent_type_e)type;
	if(pw->on_event_cb)
	{
		pw->on_event_cb(type, ep, ppos);
	}
	else {
		widget_on_event(pw, type, ep, ppos);
		if (ep->ret && t == devent_type_e::mouse_button_e)
		{
			auto p = e->b;
			if (p->down == 1)
				get_input_state(0, 1);
		}
	}
	return (ep->ret);
}

class div_cx :public widget_t
{
public:
	glm::vec4 viewport = {};
	glm::dvec4 _hover_eq = { 0,0.5,0,0 };	// 时间
	glm::ivec2 _move_pos = {};
	int evupdate = 0;
	int ckinc = 0;
	int ckup = 0;
	std::vector<widget_t*> widgets, event_wts, event_wts1;
	std::vector<drag_v6> drags;	// 拖动坐标
	std::vector<drag_v6*> dragsp;	// 拖动区域
public:
	div_cx();
	~div_cx();

	void on_event(uint32_t type, et_un_t* ep);
private:
	void sortdg();
	bool _hover = true;
};

div_cx::div_cx()
{}

div_cx::~div_cx()
{}
void div_cx::sortdg()
{
	std::stable_sort(dragsp.begin(), dragsp.end(), [](const drag_v6* t1, const drag_v6* t2) { return t1->z < t2->z; });
}
void div_cx::on_event(uint32_t type, et_un_t* ep)
{
	auto e = &ep->v;
	if (!visible)return;
	auto t = (devent_type_e)type;
	glm::ivec2 vgpos = viewport;
	int r1 = 0;
	auto ppos = get_pos();
	auto sps = get_spos();
	_hover_eq.w = type;
	widget_t* hpw = 0;
	if (t == devent_type_e::mouse_move_e)
	{
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y };
		glm::vec4 trc = viewport;
		auto k2 = check_box_cr1(mps, &trc, 1, sizeof(glm::vec4));
		if (k2.x) {
			_bst |= (int)BTN_STATE::STATE_HOVER;
			r1 = 1;
			p->cursor = (int)cursor_st::cursor_arrow;
			_hover = true;
			if (_move_pos != mps)
			{
				_move_pos = mps;
				_hover_eq.x = 0;
			}
			//printf("_hover\n");
		}
		else {
			_move_pos = mps;
			_hover_eq.z = 0;
			_bst &= ~(int)BTN_STATE::STATE_HOVER;
			if (ckinc == 0)
				_hover = false;
			//printf("on_leave\n");
		}
		//if (horizontal)
		//{
		//	widget_on_event(horizontal, type, ep, ppos);// 水平滚动条
		//}
		//if (vertical) {
		//	widget_on_event(vertical, type, ep, ppos);// 垂直滚动条 
		//}
		for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
			auto pw = *it;
			if (!pw || !pw->visible || pw->_disabled_events)continue;
			auto vpos = sps * pw->hscroll;
			on_wpe(pw, type, ep, ppos + vpos);
		}

		event_wts.clear();
		event_wts1.clear();
		//if (horizontal) {
		//	horizontal->_bst& (int)BTN_STATE::STATE_HOVER ? event_wts.push_back(horizontal) : event_wts1.push_back(horizontal);//水平滚动条
		//}
		//if (vertical) {
		//	vertical->_bst& (int)BTN_STATE::STATE_HOVER ? event_wts.push_back(vertical) : event_wts1.push_back(vertical);//垂直滚动条
		//}
		for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
			if ((*it)->_bst & (int)BTN_STATE::STATE_HOVER)
				event_wts.push_back(*it);
			else
				event_wts1.push_back(*it);
		}
		auto length = event_wts.size();
		{
			// 生成鼠标离开消息
			for (size_t i = 1; i < length; i++) {
				auto pw = event_wts[i];
				if (!pw || !pw->visible || pw->_disabled_events)continue;
				auto vpos = sps * pw->hscroll;
				auto p = e->m;
				glm::ivec2 mps = { p->x,p->y }; mps -= ppos + vpos;
				bool isd = pw->cmpos == mps;
				pw->cmpos = mps;
				pw->_bst &= ~(int)BTN_STATE::STATE_HOVER;
				// 鼠标离开
				pw->on_mevent((int)event_type2::on_leave, mps);
				if (pw->mevent_cb) {
					pw->mevent_cb(pw, (int)event_type2::on_leave, mps);
				}
			}
		}

	}
	else
	{
		bool btn = !(t == devent_type_e::mouse_button_e && e->b->down == 0);// 弹起判断
		int icc = 0;
		auto length = event_wts.size();
		for (size_t i = 0; i < length; i++)
		{
			auto pw = event_wts[i];
			icc++;
			if (!pw || !pw->visible || pw->_disabled_events)continue;
			auto vpos = sps * pw->hscroll;
			on_wpe(pw, type, ep, ppos + vpos);
			if (ep->ret && btn) {
				hpw = pw;
				break;
			}
		}
		if (!hpw)
		{
			auto ln = event_wts1.size();
			for (size_t i = 0; i < ln; i++)
			{
				auto pw = event_wts1[i];
				if (!pw || !pw->visible || pw->_disabled_events)continue;
				auto vpos = sps * pw->hscroll;
				on_wpe(pw, type, ep, ppos + vpos);
				if (ep->ret && btn) {
					hpw = pw; break;
				}
			}
		}
	}
	if (!ep->ret)
		ep->ret = r1;
	switch (t)
	{
	case devent_type_e::mouse_move_e:
	{
		auto length = event_wts.size();
		auto p = e->m;
		glm::ivec2 mps = { p->x,p->y };
		//on_motion(mps);
		_hover_eq.z = (length > 0) ? 1 : 0;// 悬停准备
		if (ckinc > 0)
		{
			for (auto& it : drags)
			{
				if (it.ck > 0)
				{
					it.pos = mps - it.tp - ppos;	// 处理拖动坐标
					it.cp1 = mps - ppos;
				}
			}
		}
		else {
			for (auto& it : drags)
			{
				it.ck = 0;
			}
		}
	}
	break;
	case devent_type_e::mouse_button_e:
	{
		auto p = e->b;
		glm::ivec2 mps = { p->x,p->y };
		//on_button(p->button, p->down, mps, p->clicks, ep->ret);

		if (p->button == 1) {
			if (p->down == 1) {
				mps -= ppos;
				drag_v6* dp = 0;

				for (auto vt = dragsp.rbegin(); vt != dragsp.rend(); vt++)
				{
					auto dp1 = *vt;
					auto& it = *dp1;
					it.z = 0;
					if (it.size.x > 0 && it.size.y > 0)
					{
						if (dp)continue;
						glm::vec4 trc = { it.pos + sps,it.size };
						auto k2 = check_box_cr1(mps, &trc, 1, sizeof(glm::vec4));
						if (k2.x)
						{
							it.tp = mps - it.pos;	// 记录当前拖动坐标
							it.ck = 1;
							it.cp1 = it.cp0 = mps;
							it.z = 1;
							dp = dp1;
						}
					}
				}
				if (!dp)
				{
					for (auto vt = dragsp.rbegin(); vt != dragsp.rend(); vt++)
					{
						auto dp1 = *vt;
						auto& it = *dp1;
						if (it.size.x == 0 || it.size.y == 0)
						{
							it.z = 0;
							it.cp1 = it.cp0 = mps;
							it.tp = mps - it.pos;	// 记录当前拖动坐标
							it.ck = 1;
						}
					}
				}
				else
				{
					for (auto vt = dragsp.rbegin(); vt != dragsp.rend(); vt++)
					{
						auto dp1 = *vt;
						auto& it = *dp1;
						if (it.size.x == 0 && it.size.y == 0)
						{
							it.z = 0;
							it.cp1 = it.cp0 = mps;
						}
					}
					sortdg();
				}
			}
		}

		_hover_eq.z = 0;
		//printf("ck:%d\t%p\n", ckinc, this);
		if (ckup > 0)
			ep->ret = 1;
	}
	break;
	case devent_type_e::mouse_wheel_e:
	{
		auto p = e->w;
		//on_wheel(p->x, p->y);
		if (_hover)
		{
			ep->ret = 1;		// 滚轮独占本事件
		}
	}
	break;
	case devent_type_e::keyboard_e:
	{
		//on_keyboard(ep);
	}
	break;
	}
	evupdate++;
}

#endif // 1


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
			new_ui(form0, vkd);
			auto pcb = new texture_cb();
			assert(pcb);
			get_sdl_texture_cb(pcb);
			//void* vg2dtex = nullptr;
			const char* filename = "temp/vkvg_gb.png";
			bspline_ct* bs = new bspline_ct();
			std::vector<glm::vec2> pts = { {100,500},{200,600},{300,400},{400,700},{500,500} };
			auto bptr = bs->new_bspline(pts.data(), pts.size());

			canvas2d_t* td3 = new canvas2d_t();
			td3->set_renderer(form0->renderer, pcb, { 0,0,ws.x, ws.y });
			td3->init_vgdev(&devinfo, 8);
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
			auto gbtn = new gradient_btn_t[5];
			auto gt = gbtn;
			rt_set(mtext, &tbox);
			tbox.rc = { 10,320,1500,600 };
			text_t1_set(ptb1, &tbox);
			glm::ivec2 sc_size = { 1024,1024 };
			auto ck = td3->new_surface(sc_size.x, sc_size.y);
			//gbtn->_bst = (int)BTN_STATE::STATE_ACTIVE;
			{

#define white 1, 1, 1
#define red   1, 0, 0
#define green 0, 1, 0
#define blue  0, 0, 1
				auto ctx = (VkvgContext)td3->ctx_begin(ck);
				vkvg_set_line_width(ctx, 2);
				vkvg_set_source_rgba(ctx, 1, 0.5, 0, 0.8);
				float scale = 1.0;
				draw_arrow(ctx, glm::vec2(100.5, 300.5), glm::vec2(512.5, 520.5), 5, 20);
				vkvg_set_line_width(ctx, 1);
				vkvg_set_source_rgba(ctx, white, 0.8);
				double        vertices_array[] = { 100, 100, 400, 100, 400, 400, 100, 400, 300, 200, 200, 200, 200, 300, 300, 300 };
				const double* contours_array[] = { vertices_array, vertices_array + 8, vertices_array + 16 };
				int           contours_size = 3;
				for (int i = 0; i < contours_size - 1; i++) {
					auto p = contours_array[i];
					vkvg_move_to(ctx, (p[0] * scale) + 0.5, (p[1] * scale + 0.5));
					p += 2;
					while (p < contours_array[i + 1]) {
						draw_arrow_to(ctx, (p[0] * scale) + 0.5, (p[1] * scale) + 0.5);
						p += 2;
					}
					vkvg_stroke(ctx);
				}
				uint32_t colors[5] = { 0x905050fc,0x9050fc50,0x90fc5050,0x90ffffff,0x90282828 };
				int xx = 0;
				for (size_t i = 0; i < 5; i++)
				{
					gt->pos = { 500 ,150 + xx };
					gt->size = { 200,36 };
					gt->back_color = colors[i];
					gt->borderLight = 0x805c5c5c;
					gt->borderDark = 0x801d1d1d;
					gradient_btn_update(gt, 0.0);
					gradient_btn_draw(ctx, gt);
					gt++;
					xx += 40;
				}
				VkvgSurface t = vkvg_get_target(ctx);
				vkvg_flush(ctx);
				vkvg_surface_resolve(t);
				vkvg_surface_write_to_png(t, "temp/vkvgtest1150.png");
				td3->ctx_end(ctx);
			}

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
					td3->draw_textdata(mtext, { 0,0 });
					td3->draw_surface(ck, { 0,0 }, { 0,0,sc_size.x,sc_size.y }, sc_size);
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
					vkd->update(form0->io);	// 更新事件
					{
						td3->update(ck, delta);
						rt_clear(mtext);
						auto img = fctx->bcc._data[0];
						std::string str = vkd->get_label(); str += (char*)u8"emoji表情🔥➗️👪️";
						//rt_add_image(mtext, img, { 0,20,64,64 }, {}, -1, { 64 * 2,64 }, { 60,20 }, true);
						rt_add_text(mtext, str.c_str(), str.size(), 0, family, 16, 0xff222222);
						str = (char*)u8"渐变色表情:\n💻🔥➗️👪️🍕";
						// 添加文本
						rt_add_text(mtext, str.c_str(), str.size(), 0, family, 64, 0xafffffff);
						rt_add_text(mtext, str.c_str(), str.size(), 0, family, 32, 0xaf0080ff);//添加不同字号和颜色的文本
						// 添加图片，提供图片对象、渲染位置\大小、九宫格设置、颜色混合、dsize渲染大小、是否固定坐标不参与布局等参数
						static glm::ivec2 imgpos = { 100,100 };
						//rt_add_image(mtext, img, { 0,20,64,64 }, { 10,10,26,26 }, 0x50ffffff, { 64 * 5,64 * 3 }, imgpos, true);
						rt_add_image(mtext, img, { 0,20,64,64 }, {}, -1, { 32,32 }, {}, false);

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
			td3->free_surface(ck);
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
