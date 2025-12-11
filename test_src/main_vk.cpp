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
#define GUI_STATIC_LIB
#include <pnguo/pnguo.h>
#include <pnguo/tinysdl3.h>
#include <pnguo/vkrenderer.h>
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
		auto texok = form0->new_texture(vki.size, vki.vkimage, 0);// 创建SDL的rgba纹理  
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
		int64_t prev_time = 0;
		while (1) {
			uint64_t curr_time = vkr_get_ticks();
			if (prev_time > 0)
			{
				double delta = (curr_time - prev_time);
				mio.DeltaTime = delta / 1000.0f;
			}
			vkd->update(&mio);		// 更新事件
			vkd->on_render();		// 执行渲染 
			// todo 提交到窗口渲染
			Sleep(1);
			prev_time = curr_time;
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

		uint32_t ch = 0;
		uint32_t ch1 = 0;
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

void c_render_data(text_render_o* p, image_ptr_t* dst)
{
	glm::vec2 pos = { 0, 0 };
	std::vector<font_item_t>& tm = p->_vstr;
	text_image_t opt = {};
	int xx = 0;
	uint32_t color = p->tb->style->color;
	for (auto& git : tm) {
		if (git._image) {
			auto ps = git._dwpos + git._apos;
			ps += pos;
			rgba_copy2rgba(dst, git._image, ps, git._rect, git.color ? git.color : color, true);
		}
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

		//{
		//	step_cx stp;
		//	stp.load(R"(data\ACEB10.stp)");


		//}
		//printf("");

		auto fctx = app->font_ctx;
		auto ksun = fctx->get_font((char*)u8"新宋体", 0);
		auto seg = fctx->get_font((char*)u8"Segoe UI Emoji", 0);
		//auto sues = fctx->add2file(R"(data\seguiemj.ttf)", 0);
		//auto sue = sues[0];
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



		std::string k8a = (char*)u8"أَبْجَدِيَّة عَرَبِيَّة➗😊😎😭\n💣🚩❓❌\t🟦⬜👨‍👨‍👧qb ab我\n的大刀";

		std::string k80 = (char*)u8"👨‍👨‍👧q";
		std::string k8 = (char*)u8"➗😊😎😭\n💣🚩❓❌\t🟦⬜👨‍👨‍👧qb abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ我\n的大刀";
		std::string k821 = (char*)u8"abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ\t我\n的大刀";
		auto family = new_font_family(fctx, (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Consolas,Malgun Gothic");

		text_style ts = {};
		ts.family = family;
		ts.fontsize = 16;
		ts.color = 0xfffF5000;
		// 文本块
		text_block tb = {};
		tb.style = &ts;
		tb.str = k8.c_str();
		tb.first = 0;
		tb.size = k8.size();
		text_render_o trt = {};
		trt.box.rc = { 0,0,300,500 };
		trt.box.auto_break = 1;
		trt.box.word_wrap = 0;
		build_text_render(&tb, &trt);
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

		uint32_t* cc = get_wcolor();
		for (size_t i = 0; i < 16; i++)
		{
			auto str = get_wcname(i, 0);
			printf("\x1b[01;3%dm%s\x1b[0m\n", (int)i % 8, str);
		}

		glm::ivec2 ws = { 1280,860 };
		const char* wtitle = (char*)u8"窗口0";
		const char* wtitle1 = (char*)u8"窗口1";
		auto devname = "Intel(R)";
		devname = 0;
		vkdg_cx* vkd = new_vkdg(0, 0, 0, 0, 0, devname);	// 创建vk渲染器 
		// 使用3D渲染器的设备创建渲染器
		app->set_dev(vkd->_dev_info.inst, vkd->_dev_info.phy, vkd->_dev_info.vkdev);
		vkvg_dev* vctx = 0;
		{
			dev_info_c cc = {};
			cc.inst = (VkInstance)vkd->_dev_info.inst; cc.phy = (VkPhysicalDevice)vkd->_dev_info.phy; cc.vkdev = (VkDevice)vkd->_dev_info.vkdev;
			cc.qFamIdx = vkd->_dev_info.qFamIdx; cc.qIndex = vkd->_dev_info.qIndex;

			vctx = new_vkvgdev(&cc, 8);
		}

		auto f = glm::mat4(1.4420948028564453, 0.0, 0.0, 0.0, 0.0, -0.00016932648114409524, 1.4420948028564453, 0.0, 0.0, -1.4420948028564453, -0.00016932648114409524, 0.0, 0.0, 0.0, 0.0, 1.0);
		auto f1 = glm::mat4(0.009999999776482582, 0.0, 0.0, 0.0, 0.0, -0.009999999776482582, 0.0, 0.0, 0.0, 0.0, -0.009999999776482582, 0.0, 0.0, 0.0, 0.0, 1.0);
		auto f2 = f * f1;

		glm::mat2x2 aaa;
		//vkr::new_ms_pipe(vkd->_dev_info.vkdev, vkd->renderpass_opaque);
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
					vkd->add_gltf(path.c_str(), pos, hz::toDouble(it["scale"], 1.0), hz::toBool(it["shadowMap"]));
				}
			}
			vkd->resize(dpis[4]);				// 设置fbo缓冲区大小
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
			vkd->_state.Exposure = 1.0;
			vkd->_state.EmissiveFactor = 30;
			vkd->_state.IBLFactor = 1.0;
			new_ui(form0, vkd);
			auto pcb = new texture_cb();
			assert(pcb);
			get_sdl_texture_cb(pcb);
			auto ptrt = &trt;

			void* tex3d = pcb->new_texture_vk(form0->renderer, vki.size.x, vki.size.y, vki.vkimage, 0);// 创建SDL的rgba纹理 

			// 动画测试
			auto d2 = sp_ctx_create(form0->renderer, (draw_geometry_fun)pcb->draw_geometry, (newTexture_fun)pcb->new_texture_0
				, (UpdateTexture_fun)pcb->update_texture, (DestroyTexture_fun)pcb->free_texture, (SetTextureBlendMode_fun)pcb->set_texture_blend);

			auto a1 = sp_new_atlas(d2, R"(E:\vsz\g3d\s2d\spine-runtimes\spine-sfml\cpp\data\spineboy-pma.atlas)");
			float scale = 0.50f;
			auto dd1 = sp_new_drawable(d2, a1, R"(E:\vsz\g3d\s2d\spine-runtimes\spine-sfml\cpp\data\spineboy-pro.json)", 0, scale);

			static std::vector<char*> nv;
			sp_drawable_get_anim_names(dd1, &nv);
			sp_drawable_set_animationbyname(dd1, 0, "portal", 0);
			sp_drawable_add_animationbyname(dd1, 0, "run", -1, 0);
			sp_drawable_set_pos(dd1, 500, 500);
			{
				sdl3_textdata* td3 = new sdl3_textdata();
				td3->rcb = pcb;
				form0->render_cb = [=](SDL_Renderer* renderer, double delta)
					{
						texture_dt tdt = {};
						tdt.src_rect = { 0,0,vki.size.x,vki.size.y };
						tdt.dst_rect = { 0,0,vki.size.x,vki.size.y };
						if (tex3d)
							pcb->render_texture(renderer, tex3d, &tdt, 1);
						sp_drawable_draw(dd1);
						r_render_data_text(renderer, ptrt, { 200,100 }, td3);
					};
				form0->up_cb = [=](float delta, int* ret)
					{
						sp_drawable_update(dd1, delta);
						auto light = vkd->get_light(0);
						vkd->_state.SelectedTonemapperIndex;	// 0-5: Tonemapper算法选择
						vkd->_state.Exposure;				// 曝光度：默认1.0
						vkd->_state.bUseTAA;
						static int ity = 10.5;
						light->_intensity = ity;
						vkd->update(form0->io);	// 更新事件
						vkd->on_render();		// 渲染到fbo纹理tex3d
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
