/*
文本渲染：fontconfig、stb_truetype、harfBuzz、icu
矢量图渲染： vkvg
窗口：SDL3
*/
#include <pch1.h>

#include <random>
#include <pnguo/win_core.h>
#include "win32msg.h"
#include <pnguo/event.h>
#define GUI_STATIC_LIB
#include <pnguo/pnguo.h>
#include <pnguo/tinysdl3.h>
#include <pnguo/vkrenderer.h>
#include <pnguo/page.h>
#include <pnguo/mapView.h>
#include <pnguo/print_time.h>
#include <cairo/cairo.h>
#include <pnguo/mesh3d.h>
#include "mshell.h"
//#include <cgltf.h>
#include <mcut/stlrw.h>
#include <cairo/cairo.h>

#include <vkvgcx.h>

auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman";// , Malgun Gothic";

void new_ui(form_x* form0, vkdg_cx* vkd) {
	auto p = new plane_cx();
	uint32_t pbc = 0xc02c2c2c;
	p->set_color({ 0x80ff802C,1,5,pbc });
	form0->bind(p);	// 绑定到窗口  
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
		vkd->set_label_cb([=](int count, int idx, const char* str)
			{
				if (idx < 0) {
					for (size_t i = 0; i < 18; i++)
						lbs[i]->str.clear();
				}
				else
				{
					lbs[idx]->str = str;
				}
			});
	}


	uint32_t* cc = get_wcolor() + 8;
	for (int i = 0; i < 0 * 8; i++) {
		auto p = new plane_cx();
		uint32_t tc = cc[i];
		p->set_color({ 0x80ff802C,1,5,0xff2c2c2c });
		form0->bind(p);	// 绑定到窗口  
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
	form0->bind(p);	// 绑定到窗口  
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
	void draw(cairo_t* cr);
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
{
}
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
	form0->bind(p);	// 绑定到窗口  
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

void vkrender_test(form_x* form0)
{
	void* inst0 = 0; void* inst = 0; void* phy = 0;	void* vkdev = 0;
	if (form0) {
		auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
		inst = sdldev.inst; phy = sdldev.phy; vkdev = sdldev.vkdev;	// 获取vk设备
	}
	else {
		inst0 = new_instance();
		inst = inst0;
	}
	std::vector<device_info_t> devs = get_devices(inst);  // 获取设备名称列表 
	if (devs.empty())return;
	if (!phy) { phy = devs[0].phd; }
	vkdg_cx* vkd = new_vkdg(inst, phy, vkdev);			// 创建vk渲染器  
	vkd->add_gltf(R"(E:\model\sharp2.glb)", { 0,0,0 }, 1.0);// 添加一个地板
	vkd->resize(1024, 800);						// 设置fbo缓冲区大小
	auto vki = vkd->get_vkimage(0);			// 获取第一个fbo纹理弄到窗口显示
	auto dstate = vkd->_state;	// 渲染状态属性
	if (form0) {
		auto texok = form0->add_vkimage(vki.size, vki.vkimage, { 20,36 }, 0);// 创建SDL的rgba纹理  
		if (texok) {
			form0->up_cb = [=](float delta, int* ret)
				{
					vkd->update(form0->io);	// 更新事件
					vkd->on_render();		// 执行渲染 
				};
		}
	}
	else {		// 独立线程或主线程执行渲染
		mouse_state_t mio = {};
		while (1) {
			vkd->update(&mio);		// 更新事件
			vkd->on_render();		// 执行渲染 
			// todo 提交到窗口渲染
			Sleep(1);
		}
		free_vkdg(vkd);			// 释放渲染器
		free_instance(inst0);		// 释放实例
	}
	return;
}
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



#if 1
class draw2d_b
{
public:
	draw2d_b();
	~draw2d_b();
	struct rect_t {
		glm::vec4 rc; glm::vec4 r;
	};
	struct triangle_t {
		glm::vec2 pos;  glm::vec2 size; glm::vec2 dirspos;
	};
	struct circle_t {
		glm::vec2 cpos; float r;
	};
	struct ellipse_t {
		glm::vec2 cpos; glm::vec2 r;
	};
	struct polyline_t {
		glm::vec2 pos; glm::vec2* points; int points_count; int* idx; int idx_count;
	};
	// 圆角矩形r，左右下左
	void add_rectangle(const glm::vec4& rc, const glm::vec4& r);
	void draw_triangle(const glm::vec2& pos, const glm::vec2& size, const glm::vec2& dirspos)
	{
		glm::vec2 tpos[3] = {};
		float df = dirspos.y;
		switch ((int)dirspos.x)
		{
		case 0:
		{
			tpos[0] = { size.x * df, 0 };
			tpos[1] = { size.x, size.y };
			tpos[2] = { 0, size.y };
		}
		break;
		case 1:
		{
			tpos[0] = { size.x, size.y * df };
			tpos[1] = { 0, size.y };
			tpos[2] = { 0, 0 };
		}
		break;
		case 2:
		{
			tpos[0] = { size.x * df, size.y };
			tpos[1] = { 0, 0 };
			tpos[2] = { size.x, 0 };
		}
		break;
		case 3:
		{
			tpos[0] = { 0, size.y * df };
			tpos[1] = { size.x, 0 };
			tpos[2] = { size.x, size.y };
		}
		break;
		default:
			break;
		}
		//cairo_move_to(cr, pos.x + tpos[0].x, pos.y + tpos[0].y);
		//cairo_line_to(cr, pos.x + tpos[1].x, pos.y + tpos[1].y);
		//cairo_line_to(cr, pos.x + tpos[2].x, pos.y + tpos[2].y);
		//cairo_close_path(cr);
	}

	//vg_style_t
	//float* dash = 0;		// 虚线逗号/空格分隔的数字
	//int dashOffset = 0;

	void fill_stroke(vg_style_t* st) {

	}
	void fill_stroke(uint32_t fill, uint32_t color, int linewidth, bool isbgr) {

	}
	// 画圆
	void draw_circle(const glm::vec2& pos, float r)
	{
	}

	void draw_ellipse(const glm::vec2& c, const glm::vec2& r)
	{
		double cx = c.x, cy = c.y, rx = r.x, ry = r.y;
		if (rx > 0.0f && ry > 0.0f) {
			//cairo_move_to(cr, cx + rx, cy);
			//cairo_curve_to(cr, cx + rx, cy + ry * c_KAPPA90, cx + rx * c_KAPPA90, cy + ry, cx, cy + ry);
			//cairo_curve_to(cr, cx - rx * c_KAPPA90, cy + ry, cx - rx, cy + ry * c_KAPPA90, cx - rx, cy);
			//cairo_curve_to(cr, cx - rx, cy - ry * c_KAPPA90, cx - rx * c_KAPPA90, cy - ry, cx, cy - ry);
			//cairo_curve_to(cr, cx + rx * c_KAPPA90, cy - ry, cx + rx, cy - ry * c_KAPPA90, cx + rx, cy);
			//cairo_close_path(cr);
		}
	}

	void draw_polyline(const glm::vec2& pos, const glm::vec2* points, int points_count, unsigned int col, bool closed, float thickness)
	{
		if (!points || points_count < 2 || !col)return;

	}
	void draw_polyline(const glm::vec2* points, int points_count, bool closed)
	{
		if (!points || points_count < 2)return;

	}

	void draw_polyline(const PathsD* p, bool closed)
	{
		if (!p || p->size() < 1)return;
		auto& d = *p;
		auto length = d.size();
		for (size_t i = 0; i < length; i++)
		{
			auto points = d[i];
			//cairo_move_to(cr, points[0].x, points[0].y);
			auto points_count = points.size();
			for (size_t i = 1; i < points_count; i++)
			{
				//cairo_line_to(cr, points[i].x, points[i].y);
			}
			if (closed) {
				//cairo_close_path(cr);
			}
		}
	}

	void draw_polylines(const glm::vec2& pos, const glm::vec2* points, int points_count, int* idx, int idx_count, unsigned int col, float thickness)
	{
		if (!points || points_count < 2 || idx_count < 2 || !col)return;
		//cairo_save(cr);
		//cairo_translate(cr, pos.x, pos.y);
		//cairo_set_line_width(cr, thickness);
		int nc = 0;
		for (size_t i = 0; i < idx_count; i++)
		{
			auto x = idx[i];
			if (x < 0)
			{
				if (nc > 1) {
					//set_color(cr, col);
					//cairo_stroke(cr);
				}
				nc = 0;
				continue;
			}
			if (nc == 0)
			{
				//cairo_move_to(cr, points[x].x, points[x].y);
			}
			else
			{
				//cairo_line_to(cr, points[x].x, points[x].y);
			}
			nc++;
		}
		if (nc > 1) {
			//set_color(cr, col);
			//cairo_stroke(cr);
		}
		//cairo_restore(cr);
	}
	void draw_rect(const glm::vec4& rc, uint32_t fill, uint32_t color, double r, int linewidth)
	{
		//cairo_as _ss_(cr);
		//draw_rectangle(cr, rc, r);
		//fill_stroke(cr, fill, color, linewidth, false);
	}
private:

};

draw2d_b::draw2d_b()
{
}

draw2d_b::~draw2d_b()
{
}
static void png_save(const char* filename, int width, int height, void* pixels)
{
	static const unsigned t[] = {
		0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
		0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
	};
	FILE* fp = fopen(filename, "wb");
	if (!fp || !pixels)
		return;
	unsigned a = 1, b = 0, c, p = width * 4 + 1, x, y, i;
	unsigned char* data = (unsigned char*)pixels;
#define PNG_U8A(ua, l) for (i = 0; i < l; i++) fputc((ua)[i], fp);
#define PNG_U32(u) do { fputc((u) >> 24, fp); fputc(((u) >> 16) & 255, fp); fputc(((u) >> 8) & 255, fp); fputc((u) & 255, fp); } while(0)
#define PNG_U8C(u) do { fputc(u, fp); c ^= (u); c = (c >> 4) ^ t[c & 15]; c = (c >> 4) ^ t[c & 15]; } while(0)
#define PNG_U8AC(ua, l) for (i = 0; i < l; i++) PNG_U8C((ua)[i])
#define PNG_U16LC(u) do { PNG_U8C((u) & 255); PNG_U8C(((u) >> 8) & 255); } while(0)
#define PNG_U32C(u) do { PNG_U8C((u) >> 24); PNG_U8C(((u) >> 16) & 255); PNG_U8C(((u) >> 8) & 255); PNG_U8C((u) & 255); } while(0)
#define PNG_U8ADLER(u) do { PNG_U8C(u); a = (a + (u)) % 65521; b = (b + a) % 65521; } while(0)
#define PNG_BEGIN(s, l) do { PNG_U32(l); c = ~0U; PNG_U8AC(s, 4); } while(0)
#define PNG_END() PNG_U32(~c)
	PNG_U8A("\x89PNG\r\n\32\n", 8);
	PNG_BEGIN("IHDR", 13);
	PNG_U32C(width);
	PNG_U32C(height);
	PNG_U8C(8);
	PNG_U8C(6);
	PNG_U8AC("\0\0\0", 3);
	PNG_END();
	PNG_BEGIN("IDAT", 2 + height * (5 + p) + 4);
	PNG_U8AC("\x78\1", 2);
	for (y = 0; y < height; y++)
	{
		PNG_U8C(y == height - 1);
		PNG_U16LC(p);
		PNG_U16LC(~p);
		PNG_U8ADLER(0);
		for (x = 0; x < p - 1; x++, data++)
			PNG_U8ADLER(*data);
	}
	PNG_U32C((b << 16) | a);
	PNG_END();
	PNG_BEGIN("IEND", 0);
	PNG_END();
	fclose(fp);
}

static void cg_surface_write_to_png(struct cg_surface_t* surface, const char* filename)
{
	unsigned char* data = surface->pixels;
	int width = surface->width;
	int height = surface->height;
	int stride = surface->stride;
	unsigned char* image = (unsigned char*)malloc((size_t)(stride * height));
	for (int y = 0; y < height; y++)
	{
		uint32_t* src = (uint32_t*)(data + stride * y);
		uint32_t* dst = (uint32_t*)(image + stride * y);
		for (int x = 0; x < width; x++)
		{
			uint32_t a = src[x] >> 24;
			if (a != 0)
			{
				uint32_t r = (((src[x] >> 16) & 0xff) * 255) / a;
				uint32_t g = (((src[x] >> 8) & 0xff) * 255) / a;
				uint32_t b = (((src[x] >> 0) & 0xff) * 255) / a;
				dst[x] = (a << 24) | (b << 16) | (g << 8) | r;
			}
			else
			{
				dst[x] = 0;
			}
		}
	}
	png_save(filename, width, height, image);
	free(image);
}
#endif

void test_vkvg(const char* fn, dev_info_c* dc);


bitmap_cache_cx* bc_ctx = 0;  //纹理缓存

text_image_t* get_glyph_item(std::vector<font_t*>& familys, int fontsize, const void* str8, text_image_t* opt)
{
	auto str = (const char*)str8;
	font_t* r = 0;
	do
	{
		if (!str || !(*str) || !opt) { opt = 0; break; }
		int gidx = 0;
		r = 0;
		if (*str == '\n')
			gidx = 0;
		auto ostr = str;

		int ch = 0;
		int ch1 = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
#if 0
		auto bstr = str;
		auto bstr1 = str;
		bstr1 += kk;
		int d2 = 0;
		s32.clear();
		s32.push_back(ch);
		for (int zwj = 1; zwj < 2; )
		{
			auto nk1 = md::utf8_to_unicode(bstr1, &ch1);
			bstr1 += nk1;
			if (ch1 != 0x200d)
			{
				zwj++;
				if (zwj == 2)break;
				s32.push_back(ch1);
			}
			else {
				zwj = 0; d2++;
			}
		}
#endif
		font_t::get_glyph_index_u8(ostr, &gidx, &r, &familys);
		if (r && gidx >= 0)
		{
			auto k = r->get_glyph_item(gidx, ch, fontsize, bc_ctx);
			if (k._glyph_index)
			{
				k.cpt = ch;
				opt->tv.push_back(k);
			}
		}
		else {
			font_item_t k = {};
			k.cpt = ch;
			k.advance = 0;
			opt->tv.push_back(k);
		}
	} while (str && *str);
	return opt;
}

glm::vec2 draw_image1(cairo_t* cr, cairo_surface_t* image, const glm::vec2& pos, const glm::vec4& rc, uint32_t color, const glm::vec2& dsize)
{
	glm::vec2 ss = { rc.z, rc.w };
	if (ss.x < 0)
	{
		ss.x = cairo_image_surface_get_width(image);
	}
	if (ss.y < 0)
	{
		ss.y = cairo_image_surface_get_height(image);
	}
	if (ss.x > 0 && ss.y > 0)
	{
		glm::vec2 sc = { 1,1 };
		if (dsize.x > 0 && dsize.y > 0) {
			sc = dsize / ss;
		}
		if (color > 0 && color != -1)
		{
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_translate(cr, pos.x, pos.y);
			if (sc.x != 1 || sc.y != 1)
				cairo_scale(cr, sc.x, sc.y);
			cairo_rectangle(cr, 0, 0, ss.x, ss.y);
			cairo_clip(cr);
			cairo_set_source_rgba(cr, 1, 1, 1, 1);
			cairo_mask_surface(cr, image, -rc.x, -rc.y);
			cairo_restore(cr);
		}
		else {
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_translate(cr, pos.x, pos.y);
			if (sc.x != 1 || sc.y != 1)
				cairo_scale(cr, sc.x, sc.y);
			cairo_set_source_surface(cr, image, -rc.x, -rc.y);
			cairo_rectangle(cr, 0, 0, ss.x, ss.y);
			cairo_fill(cr);
			cairo_restore(cr);
		}
	}
	return ss;
}
void image_set_ud(cairo_surface_t* p, uint64_t key, void* ud, void(*destroy_func)(void* data))
{
	if (p && key) {
		cairo_surface_set_user_data(p, (cairo_user_data_key_t*)key, ud, destroy_func);
	}
}
#ifndef key_def_data
#define key_def_data 1024
#define key_def_data_iptr 1025
#define key_def_data_svgptr 1026
#define key_def_data_done 1000
#endif
void destroy_image_data(void* d) {
	auto p = (uint32_t*)d;
	if (d)delete[]p;
}
void free_image_cr(cairo_surface_t* image)
{
	if (image)
	{
		cairo_surface_destroy(image);
	}
}
cairo_surface_t* new_image_cr(image_ptr_t* img)
{
	cairo_surface_t* image = 0;
	if (img->stride < 1)img->stride = img->width * sizeof(uint32_t);
	auto px = new uint32_t[img->width * img->height];
	image = cairo_image_surface_create_for_data((unsigned char*)px, CAIRO_FORMAT_ARGB32, img->width, img->height, img->width * sizeof(int));
	if (image)
	{
		image_set_ud(image, key_def_data, px, destroy_image_data);
		memcpy(px, (unsigned char*)img->data, img->height * img->width * sizeof(int));
		if (img->multiply && img->type == 1)
		{
		}
		else {
			int stride = cairo_image_surface_get_stride(image);
			auto data = cairo_image_surface_get_data(image);
			auto t = data;
			auto ts = (unsigned char*)img->data;
			for (size_t i = 0; i < img->height; i++)
			{
				premultiply_data(img->width * 4, t, img->type, !img->multiply);
				t += stride;
				ts += img->stride;
			}
			//#define _DEBUG
			//			save_img_png(img, "update_text_img.png");
			//			cairo_surface_write_to_png(image, "update_text_surface.png");
			//#endif
		}
	}
	else {
		delete[]px;
	}
	return image;
}
cairo_surface_t* new_image_cr(const glm::ivec2& size, uint32_t* data)
{
	cairo_surface_t* image = 0;
	image_ptr_t img[1] = {};
	img->width = size.x;
	img->height = size.y;
	if (img->stride < 1)img->stride = img->width * sizeof(uint32_t);
	auto px = data ? data : new uint32_t[img->width * img->height];
	image = cairo_image_surface_create_for_data((unsigned char*)px, CAIRO_FORMAT_ARGB32, img->width, img->height, img->width * sizeof(int));
	if (image)
	{
		if (!data)
			image_set_ud(image, key_def_data, px, destroy_image_data);
		memset(px, 0, img->width * img->height * sizeof(int));
	}
	else {
		delete[]px;
	}
	return image;
}
void draw_text(cairo_t* cr, const std::vector<font_item_t>& rtv, uint32_t color, const glm::ivec2& pos)
{
	auto mx = rtv.size();
	glm::ivec2 nps = pos;
	for (size_t i = 0; i < mx; i++)
	{
		auto& it = rtv[i];
		if (it._image)
		{
			auto ft = (cairo_surface_t*)it._image->ptr;
			if (!ft) { ft = new_image_cr(it._image); it._image->ptr = ft; }
			if (ft)
			{
				draw_image1(cr, ft, it._dwpos + it._apos + nps, it._rect, it.color ? it.color : color, {});
			}
		}
		nps.x += it.advance;
	}
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

#ifdef _DEBUG
		system("rd /s /q E:\\temcpp\\SymbolCache\\tcmp.pdb");
		system("rd /s /q E:\\temcpp\\SymbolCache\\vkcmp.pdb");
		system("rd /s /q E:\\temcpp\\SymbolCache\\cedit.pdb");
		system("rd /s /q E:\\temcpp\\SymbolCache\\p86.pdb");
		//auto rd = hz::shared_load(R"(E:\Program Files\RenderDoc_1.37_64\renderdoc.dll)");
#endif 

		{
			step_cx stp;
			stp.load(R"(data\ACEB10.stp)");

			{
				const char* filename = "temp/dash.png";
				struct cg_surface_t* surface = cg_surface_create(256, 256);
				struct cg_ctx_t* ctx = cg_create(surface);

				{
					print_time ptt(filename);
					double dashes[] = { 50.0, 10.0, 10.0, 10.0 };
					int ndash = sizeof(dashes) / sizeof(dashes[0]);
					double offset = 30.0;
					cg_set_dash(ctx, dashes, ndash, offset);
					cg_set_line_width(ctx, 10.0);
					cg_move_to(ctx, 128.0, 25.6);
					cg_line_to(ctx, 230.4, 230.4);
					cg_rel_line_to(ctx, -102.4, 0.0);
					cg_curve_to(ctx, 51.2, 230.4, 51.2, 128.0, 128.0, 128.0);
					cg_stroke(ctx);
				}

				cg_surface_write_to_png(surface, filename);
				cg_destroy(ctx);
				cg_surface_destroy(surface);
			}

			{
				const char* filename = "temp/fill_and_stroke.png";
				struct cg_surface_t* surface = cg_surface_create(256, 256);
				struct cg_ctx_t* ctx = cg_create(surface);

				{
					print_time ptt(filename);
					cg_move_to(ctx, 128.0, 25.6);
					cg_line_to(ctx, 230.4, 230.4);
					cg_rel_line_to(ctx, -102.4, 0.0);
					cg_curve_to(ctx, 51.2, 230.4, 51.2, 128.0, 128.0, 128.0);
					cg_close_path(ctx);

					cg_move_to(ctx, 64.0, 25.6);
					cg_rel_line_to(ctx, 51.2, 51.2);
					cg_rel_line_to(ctx, -51.2, 51.2);
					cg_rel_line_to(ctx, -51.2, -51.2);
					cg_close_path(ctx);

					cg_set_line_width(ctx, 10.0);
					cg_set_source_rgb(ctx, 0, 0, 1);
					cg_fill_preserve(ctx);
					cg_set_source_rgb(ctx, 0, 0, 0);
					cg_stroke(ctx);
				}

				cg_surface_write_to_png(surface, filename);
				cg_destroy(ctx);
				cg_surface_destroy(surface);
			}

			{
				const char* filename = "temp/fill_style.png";
				struct cg_surface_t* surface = cg_surface_create(256, 256);
				struct cg_ctx_t* ctx = cg_create(surface);

				{
					print_time ptt(filename);
					cg_set_line_width(ctx, 6);

					cg_rectangle(ctx, 12, 12, 232, 70);
					cg_circle(ctx, 64, 64, 40);
					cg_circle(ctx, 192, 64, 40);
					cg_set_fill_rule(ctx, CG_FILL_RULE_EVEN_ODD);
					cg_set_source_rgb(ctx, 0, 0.7, 0);
					cg_fill_preserve(ctx);
					cg_set_source_rgb(ctx, 0, 0, 0);
					cg_stroke(ctx);

					cg_save(ctx);
					cg_translate(ctx, 0, 128);
					cg_rectangle(ctx, 12, 12, 232, 70);
					cg_circle(ctx, 64, 64, 40);
					cg_circle(ctx, 192, 64, 40);
					cg_set_fill_rule(ctx, CG_FILL_RULE_NON_ZERO);
					cg_set_source_rgb(ctx, 0, 0, 0.9);
					cg_fill_preserve(ctx);
					cg_set_source_rgb(ctx, 0, 0, 0);
					cg_stroke(ctx);
					cg_restore(ctx);
				}

				cg_surface_write_to_png(surface, filename);
				cg_destroy(ctx);
				cg_surface_destroy(surface);
			}

			{
				const char* filename = "temp/cg_gradient.png";
				struct cg_surface_t* surface = cg_surface_create(256, 256);
				struct cg_ctx_t* ctx = cg_create(surface);
				struct cg_gradient_t* grad;
				{
					print_time ptt(filename);

					cg_rectangle(ctx, 0, 0, 256, 256);
					grad = cg_set_source_linear_gradient(ctx, 0.0, 0.0, 0.0, 256.0);
					cg_gradient_add_stop_rgba(grad, 0, 1, 1, 1, 1);
					cg_gradient_add_stop_rgba(grad, 1, 0, 0, 0, 1);
					cg_fill(ctx);

					grad = cg_set_source_radial_gradient(ctx, -10, -10, 25.6, 152.4, 152.4, 128.0);
					cg_gradient_add_stop_rgba(grad, 0.2, 1, 0, 0, 1);
					cg_gradient_add_stop_rgba(grad, 1, 1, 1, 0, 1);
					//cg_arc(ctx, 128.0, 128.0, 76.8, 0, 2 * glm::pi<double>());
					cg_rectangle(ctx, 0, 0, 256, 256);
					cg_fill(ctx);
					//unsigned char* image = surface->pixels; // 存储RGB图像数据
					//int centerX = surface->width / 2;               // 渐变中心X坐标
					//int centerY = surface->height / 2;              // 渐变中心Y坐标
					//int maxRadius = (surface->width > surface->height ? surface->width : surface->height) / 2; // 最大半径
					//int w = surface->width * sizeof(int);
					//// 遍历每个像素
					//for (int y = 0; y < surface->height; y++) {
					//	auto px = image + y * w;
					//	for (int x = 0; x < surface->width; x++) {
					//		// 计算当前像素到中心的距离
					//		double dx = x - centerX;
					//		double dy = y - centerY;
					//		double distance = sqrt(dx * dx + dy * dy);

					//		// 归一化距离到[0, 1]范围
					//		double t = distance / maxRadius;
					//		if (t > 1.0) t = 1.0;

					//		// 根据距离生成颜色（从白色到黑色渐变）
					//		unsigned char color = (unsigned char)((1.0 - t) * 255.0);
					//		px[0] = color; // R
					//		px[1] = color; // G
					//		px[2] = color; // B
					//		px[3] = color; // A
					//		px += sizeof(int);
					//	}
					//}

				}

				cg_surface_write_to_png(surface, filename);
				cg_destroy(ctx);
				cg_surface_destroy(surface);
			}

			{
				const char* filename = "temp/cairo_gradient.png";
				cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 256, 256);
				cairo_t* ctx = cairo_create(surface);
				//struct cairo_gradient_t* grad;
				{
					print_time ptt(filename);
					cairo_pattern_t* grad = cairo_pattern_create_linear(0.0, 0.0, 0.0, 256.0);
					cairo_pattern_add_color_stop_rgba(grad, 0, 1, 1, 1, 1);
					cairo_pattern_add_color_stop_rgba(grad, 1, 0, 0, 0, 1);
					cairo_set_source(ctx, grad);
					cairo_rectangle(ctx, 0, 0, 256, 256);
					cairo_fill(ctx);
					//cairo_pattern_t* rg = cairo_pattern_create_radial(15.2, 12.4, 25.6, 102.4, 102.4, 128.0);
					cairo_pattern_t* rg = cairo_pattern_create_radial(115.2, 102.4, 25.6, 102.4, 102.4, 128.0);
					cairo_pattern_add_color_stop_rgba(rg, 0, 1, 1, 0, 1);
					cairo_pattern_add_color_stop_rgba(rg, 1, 1, 0, 0, 1);
					cairo_pattern_add_color_stop_rgba(rg, 0.2, 0, 1, 0, 1);
					cairo_set_source(ctx, rg);
					//cairo_arc(ctx, 128.0, 128.0, 76.8, 0, 2 * glm::pi<double>());
					cairo_rectangle(ctx, 0, 0, 256, 256);
					cairo_fill(ctx);
				}
				cairo_surface_write_to_png(surface, filename);
			}
		}
		printf("");

		auto fctx = app->font_ctx;
		auto ksun = fctx->get_font((char*)u8"新宋体", 0);
		auto seg = fctx->get_font((char*)u8"Segoe UI Emoji", 0);
		auto sues = fctx->add2file(R"(data\seguiemj.ttf)", 0);
		auto sue = sues[0];
		{
			//纹理缓存
			auto p = new bitmap_cache_cx();
			if (p)
			{
				p->resize(1024, 1024);
				if (!bc_ctx)
					bc_ctx = p;
			}
		}
		std::vector<font_t*> familys = { ksun ,seg };
		std::string k8 = (char*)u8"q我的大刀➗😊😎😭💣🚩❓❌🟦⬜";
		std::string k81 = (char*)u8"🏳️‍🌈";
		std::string k80 = (char*)u8"👨‍👨‍👧";
		k8 += k80 + k81;
		text_image_t opt = {};
		//text_image_t* a = get_glyph_item(familys, 32, estr, &opt);
		auto img = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 2024, 512);
		auto cr = cairo_create(img);

		auto family = new_font_family(fctx, (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Consolas,Malgun Gothic"); 
		text_p text = text_create(k8.c_str(), k8.size(), family);

		//glm::extractEulerAngleYXZ();
		delete_font_family(family);
		font_t::GlyphPositions gp = {};// 执行harfbuzz
		auto nn0 = sue->CollectGlyphsFromFont(k8.data(), k8.size(), 8, 0, 0, &gp);
		int fontsize = 128;
		double scale_h = sue->get_scale(fontsize);
		uint32_t color = -1;
		int xx = 0;
		int yy = sue->get_line_height(fontsize);
		std::vector<font_item_t> tm;
		// 光栅化glyph index并缓存
		for (size_t i = 0; i < gp.len; i++)
		{
			auto pos = &gp.pos[i];
			auto git = sue->get_glyph_item(pos->index, 0, fontsize);
			glm::vec2 offset = { ceil(pos->x_offset * scale_h), -ceil(pos->y_offset * scale_h) };
			git._apos = offset;
			tm.push_back(git);
		}
		for (size_t i = 0; i < gp.len; i++)
		{
			auto pos = &gp.pos[i];
			glm::vec2 adv = { ceil(pos->x_advance * scale_h), ceil(pos->y_advance * scale_h) };
			glm::vec2 offset = { ceil(pos->x_offset * scale_h), -ceil(pos->y_offset * scale_h) };
			auto git = tm[i];
			if (git._image) {
				auto ft = (cairo_surface_t*)git._image->ptr;
				if (!ft) {
					ft = new_image_cr(git._image);
					git._image->ptr = ft;
					cairo_surface_write_to_png(ft, "temp/cacheemoji.png");
				}
				if (ft)
				{
					auto ps = git._dwpos;// +git._apos;
					ps.x += xx;
					ps += offset;
					ps.y += yy;
					draw_image1(cr, ft, ps, git._rect, git.color ? git.color : color, {});
				}
			}
			xx += adv.x;
		}


		std::vector<vertex_f> vdp;
		int gidx[3] = { 57889,57888,57890 };
		image_gray bmp[1] = {};
		bmp->width = bmp->height = 1024;
		int fheight = 300;
		auto bl = sue->get_base_line(fheight);
		auto sc = sue->get_scale(fheight);
		std::string fni = "temp/chu_x.png";
		std::vector<uint32_t> idata;
		idata.resize(bmp->width * bmp->height);
		for (auto& it : idata) { it = 0xff000000; }
		image_ptr_t rgba = {};
		rgba.data = idata.data();
		rgba.width = bmp->width;
		rgba.height = bmp->height;
		rgba.stride = bmp->width * 4;
		rgba.comp = 4;
		int posy = 100;
		std::vector<glm::vec4> kvs;
		for (int i = 0; i < 3; i++) {
			auto vnn = sue->GetGlyphShapeTT(gidx[i], &vdp);
			glm::vec4 box = {};
			auto bs = sue->get_shape_box_glyph(gidx[i], fheight, &box);
			kvs.push_back({ bs.x,bs.y,box.x,box.y });
			get_path_bitmap((vertex_32f*)vdp.data(), vdp.size(), bmp, { sc,sc }, { box.x, box.y }, 1);
			gray_copy2rgba(&rgba, bmp, { box.x,bl + box.y + posy }, glm::ivec4(0, 0, box.z, box.w), -1, true);
			std::string fni0 = "temp/chu_" + std::to_string(i) + ".png";
			save_img_png(bmp, fni0.c_str());
		}
		save_img_png(&rgba, fni.c_str());
		size_t c = 0;
		for (auto& it : bmp->_data) {
			if (it > 0)
			{
				c++;
			}
		}
		std::string fn = "temp/emojitest2.png";
		cairo_surface_write_to_png(img, fn.c_str());
		cairo_destroy(cr);
		cairo_surface_destroy(img);
		//vkrender_test(0);
		{


#if 0

			glm::quat qk = glm::quat(1, 0, 0, 0);
			static constexpr float AMD_PI = 3.1415926535897932384626433832795f;
			static constexpr float AMD_PI_OVER_2 = 1.5707963267948966192313216916398f;
			static constexpr float AMD_PI_OVER_4 = 0.78539816339744830961566084581988f;
			auto a = glm::radians(75.0f);
			auto ak = AMD_PI_OVER_4;
			auto ac = cosf(a);
			auto ac0 = cosf(ak);
			printf("\n")
				vkr::Transform transform = {};
			transform.LookAt(p2v(AMD_PI_OVER_2, 0.58f) * 3.5f, glm::vec4(0, 0, 0, 0), false);
			vkr::light_t l = {};
			l._type = vkr::light_t::LIGHT_SPOTLIGHT;
			l._intensity = 50.0;
			l._color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			l._range = 15.0;
			l._outerConeAngle = AMD_PI_OVER_4;// glm::radians(20.0);
			l._innerConeAngle = AMD_PI_OVER_4 * 0.9f;
			glm::mat4 lightMat = transform.GetWorldMat();
			glm::mat4 lightView = glm::affineInverse(lightMat);
			glm::mat4 mLightViewProj;
			glm::mat4 per = glm::perspective(l._outerConeAngle * 2.0f, 1.0f, .1f, 100.0f);
			auto mLightView = lightView;
			if (l._type == vkr::light_t::LIGHT_SPOTLIGHT)
				mLightViewProj = per * lightView;
			//transpose
			auto direction = glm::transpose(lightView) * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
			auto position = lightMat[3];
			auto outerConeCos = cosf(l._outerConeAngle);
			auto innerConeCos = cosf(l._innerConeAngle);

			glm::vec4 vCenter = glm::vec4(0.0f, 0.0f, 0.5f, 0.0f);
			glm::vec4 vRadius = glm::vec4(1.0f, 1.0f, 0.5f, 0.0f);
			glm::vec4 vColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::mat4 spotlightMatrix = glm::inverse(mLightViewProj);

			printf("Spotlight Matrix:\n");
#endif
		}



		//{
		//	hz::mfile_t m;
		//	auto pd = m.open_d(R"(E:\zt\c00951ea.spr)", 0);
		//	std::vector<int> fv;
		//	if (pd)
		//	{
		//		fv.resize(20);
		//		memcpy(fv.data(), pd, 20 * 4);
		//	}
		//}
		uint32_t* cc = get_wcolor();
		for (size_t i = 0; i < 16; i++)
		{
			auto str = get_wcname(i, 0);
			printf("\x1b[01;3%dm%s\x1b[0m\n", (int)i % 8, str);
		}
		//testnjson();
		glm::ivec2 ws = { 1280,860 };
		const char* wtitle = (char*)u8"窗口0";
		const char* wtitle1 = (char*)u8"窗口1";
		vkdg_cx* vkd = new_vkdg(0, 0, 0);	// 创建vk渲染器 
		// 使用3D渲染器的设备创建渲染器
		app->set_dev(vkd->_dev_info.inst, vkd->_dev_info.phy, vkd->_dev_info.vkdev);
		{
			dev_info_c cc = {};
			cc.inst = (VkInstance)vkd->_dev_info.inst; cc.phy = (VkPhysicalDevice)vkd->_dev_info.phy; cc.vkdev = (VkDevice)vkd->_dev_info.vkdev;
			cc.qFamIdx = vkd->_dev_info.qFamIdx; cc.qIndex = vkd->_dev_info.qIndex;
			test_vkvg(0, &cc);
		}
		glm::mat2x2 aaa;
		vkr::new_ms_pipe(vkd->_dev_info.vkdev, vkd->renderpass_opaque);
		form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable /*| ef_borderless*/);
		//form_x* form1 = (form_x*)new_form(app, wtitle1, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable);
		auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
		auto kd = sdldev.vkdev;
		//sdldev.vkdev = 0;	// 清空使用独立创建逻辑设备
		std::vector<device_info_t> devs = get_devices(sdldev.inst); // 获取设备名称列表
		bool grab_enable = false;	// 设置鼠标范围在窗口内
		bool rmode = true;			// 设置窗口的相对鼠标模式。
		//form0->set_mouse_mode(grab_enable, rmode);
		get_queue_info(sdldev.phy);
		//sdldev.vkdev = 0;
		//vkdg_cx* vkd1 = new_vkdg(&sdldev);	// 创建vk渲染器  
		//SetWindowDisplayAffinity((HWND)form0->get_nptr(), WDA_MONITOR);// 反截图
		//if (vkd1) {
		//	vkd1->add_gltf(R"(E:\model\helicopter_space_ship.glb)", { 15,0,8 }, 1.0);
		//	vkd1->resize(1024, 800);				// 设置fbo缓冲区大小
		//	auto vr = vkd1->get_vkimage(0);	// 获取fbo纹理弄到窗口显示 nullptr;//
		//	auto texok = form1->add_vkimage(vr.size, vr.vkimageptr, { 20,36 }, 1);// 创建SDL的bgra纹理 
		//	//vkd->state.SelectedTonemapperIndex = 1;
		//	vkd1->state.Exposure = 0.9928;
		//	vkd1->state.EmissiveFactor = 250;
		//	new_ui(form1, vkd1);
		//	if (texok)
		//	{
		//		form1->up_cb = [=](float delta, int* ret)
		//			{
		//				auto light = vkd1->get_light(0);
		//				vkd1->state.SelectedTonemapperIndex;	// 0-5: Tonemapper算法选择
		//				vkd1->state.Exposure;				// 曝光度：默认1.0
		//				vkd1->state.bUseTAA;
		//				vkd1->update(form1->io);	// 更新事件
		//				vkd1->on_render();		// 执行渲染
		//			};
		//	}
		//}
		if (vkd) {
			{
				int kadf[] = { sizeof(std::string),sizeof(std::vector<char>) };
				//vkd->add_gltf(R"(E:\model\sharp2.glb)", { 0,0,0 }, 1.0);// 地板
				//vkd->add_gltf(R"(E:\model\zw\fantasy_church_ruins.glb)", { -5,0,-6 }, 1.0);// 加载gltf
				//vkd->add_gltf(R"(E:\model\zw\autumnal_forest.glb)", { -15,0,-6 }, 1.0);// 加载gltf
				//vkd->add_gltf(R"(E:\model\realistic_palm_tree_10_free.glb)", { 2,0,0 }, 1.0);
				//vkd->add_gltf(R"(E:\model\bc22.glb)", { 0,0,5 }, 0.52);
				//vkd->add_gltf(R"(E:\vsz\h\avl\av\Bee.glb)", { 0,0,0 }, 10.0); 
				//vkd->add_gltf(R"(E:\code\c\assimp\test\models\glTF2\textureTransform\TextureTransformTest.gltf)", { 0,0,0 }, 1.0);
				// 机器
				vkd->add_gltf(R"(E:\ag\glTFSample\media\Cauldron-Media\buster_drone\busterDrone.gltf)", { 0,01.8,0 }, 1.0, true);
				//vkd->add_gltf(R"(E:\model\lets_go_to_the_beach_-_beach_themed_diorama.glb)", { 0,0,20 }, 1.0);
				//vkd->add_gltf( R"(E:\model\hero_alice_lobby.glb)");
				//变形树
				//vkd->add_gltf(R"(E:\model\pale_radiance_tree.glb)", { }, 1.0);
				//vkd->add_gltf(R"(E:\model\ka-2000__scx2800-2_cranes (1).glb)", { 5,0,-8 }, 1.0);
				//vkd->add_gltf(R"(E:\model\maple_trees.glb)", { 20,0,10 }, 0.10);

				//vkd->add_gltf(R"(E:\model\rock_monster.glb)", { 5,0,10 }, 0.5);
				vkd->add_gltf(R"(E:\model\helicopter_space_ship.glb)", { 10,0,20 }, 1.0, true);// 飞船
				//vkd->add_gltf(R"(E:\zmodel\cr123.glb)", {0,0,10}, 10.0);
				//vkd->add_gltf(R"(E:\model\spaceship.glb)", { 0 * 5,10,-8 * 0 }, 1.0);

				//vkd->add_gltf(R"(E:\zmodel\glTF-Sample-Models-main\2.0\ClearcoatRing\glTF\ClearcoatRing.gltf)", {  }, 1.0);
				//vkd->add_gltf(R"(E:\zmodel\glTF-Sample-Models-main\2.0\MorphStressTest\glTF-Binary\MorphStressTest.glb)", {  }, 1.0);
				//vkd->add_gltf(R"(E:\zmodel\MorphStressTest.glb)", { }, 1.0);
				//vkd->add_gltf(R"(E:\model\psx_houses.glb)", { 15,0,-8 }, 1.0);
				//vkd->add_gltf(R"(E:\model\psx_old_house.glb)", { 0 * 5,0,-8 * 0 }, 1.0);
				//vkd->add_gltf(R"(E:\model\o-tech_reaper-4k-materialtest.glb)", { 5,10,-8 }, 10.0);
				//vkd->add_gltf(R"(E:\zmodel\sofa.glb)", { 0,0,0 }, 1.0);
				//vkd->add_gltf(R"(E:\tx\parlahti.glb)", { 0,0,0 }, 1.0);
				//vkd->add_gltf(R"(E:\model\black_hole.glb)", { 0,0,0 }, 0.0010);

				//vkd->add_gltf(R"(E:\model\space_station_4.glb)");
				//vkd->add_gltf(R"(E:\model\sexy_guardian_woman_model_18.glb)");
				//vkd->add_gltf(R"(E:\code\hub\cpp\vulkanFrame\vulkanFrame\DamagedHelmet.glb)");
				//vkd->add_gltf(R"(E:\model\DragonAttenuation.glb)", { 0,0,0 }, 1.0);//玻璃龙
				//vkd->add_gltf(R"(E:\zmodel\glTF-Sample-Models-main\2.0\Fox\glTF-Binary\Fox.glb)", { 0,0,0 }, 0.20);//
				//vkd->add_gltf(R"(E:\zmodel\glTF-Sample-Models-main\2.0\TextureSettingsTest\glTF-Binary\TextureSettingsTest.glb)", { 0,11,0 }, 1.0);//
				//vkd->add_gltf(R"(E:\zmodel\glTF-Sample-Models-main\2.0\NegativeScaleTest\glTF-Binary\NegativeScaleTest.glb)", { 0,0,0 }, 1.0);//
				//vkd->add_gltf(R"(E:\zmodel\NodePerformanceTest.glb)", { 0,0.1,0 }, 1.0);//
			}
			vkd->resize(1024, 800);				// 设置fbo缓冲区大小
			auto vki = vkd->get_vkimage(0);	// 获取fbo纹理弄到窗口显示 nullptr;//
			auto texok = form0->add_vkimage(vki.size, vki.vkimage, { 20,36 }, 0);// 创建SDL的rgba纹理 
			/*
			case 0: return AMDTonemapper(color);
			case 1: return DX11DSK(color);
			case 2: return Reinhard(color);
			case 3: return Uncharted2Tonemap(color);
			case 4: return tonemapACES( color );
			case 5: return color;
			*/
			vkd->_state.SelectedTonemapperIndex = 0;
			vkd->_state.Exposure = 1.0;
			vkd->_state.EmissiveFactor = 30;
			vkd->_state.IBLFactor = 1.0;
			new_ui(form0, vkd);
			if (texok)
			{
				form0->up_cb = [=](float delta, int* ret)
					{
						auto light = vkd->get_light(0);
						vkd->_state.SelectedTonemapperIndex;	// 0-5: Tonemapper算法选择
						vkd->_state.Exposure;				// 曝光度：默认1.0
						vkd->_state.bUseTAA;
						static int ity = 10.5;
						light->_intensity = ity;
						vkd->update(form0->io);	// 更新事件
						vkd->on_render();		// 执行渲染
						static bool sa = false;
						if (sa)
						{
							auto img = vkd->save_shadow(0);	// 保存阴影贴图
							auto pf = (float*)img.data;
							for (size_t i = 0; i < img.size.x * img.size.y; i++)
							{
								img.data[i] = gray_float_to_rgba(*pf);
								pf++;
							}
							stbi_write_png("temp/shadow.png", img.size.x, img.size.y, 4, img.data, img.size.x * 4);
							sa = false;
						}
					};
			}

		}

		//show_belt(form0);
		//show_cpuinfo(form0);
		// 运行消息循环
		run_app(app, 0);
		free_vkdg(vkd);
		free_app(app);
	}
#ifdef _WIN32
	ExitProcess(0);
#endif
	return 0;
}
