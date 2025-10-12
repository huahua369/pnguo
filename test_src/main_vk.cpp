
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
#include <noise/noise.h>
#include "mshell.h"
//#include <cgltf.h>

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


#define MAX_HEIGHT 255 // 最大高度值

// 地形类型枚举 
typedef enum {
	PLAIN,      // 平原 (高度0-50)
	HILL,       // 丘陵 (高度51-150)
	MOUNTAIN,   // 山脉 (高度151-200)
	RIVER       // 河流 (特殊标记)
} TerrainType;


// 初始化随机数生成器
void initRandom(unsigned int seed) {
	srand(seed); // 设置随机种子[4]()[11]()
}

#define OCTAVES 3     // 噪声层数 
#define PERSISTENCE 0.125 // 噪声衰减系数

// 生成随机梯度向量
void initGradient(int* p, int w) {
	for (int i = 0; i < w; i++) {
		p[i] = rand() % 256;
	}
}

// 平滑函数
double fade(double t) {
	return t * t * t * (t * (t * 6 - 15) + 10);
}

// 梯度函数
double grad(int hash, double x, double y, double z) {
	int h = hash & 15;
	double u = h < 8 ? x : y;
	double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// 线性插值
double lerp(double t, double a, double b) {
	return a + t * (b - a);
}
// 计算柏林噪声
double perlinNoise(double x, double y, double z, int* p) {
	int X = (int)x & 255;
	int Y = (int)y & 255;
	int Z = (int)z & 255;

	x -= (int)x;
	y -= (int)y;
	z -= (int)z;

	double u = fade(x);
	double v = fade(y);
	double w = fade(z);

	int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
	int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

	return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
		lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
		lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)),
			lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))));
}

float fractal_noise1(float x, float y, float z, int* p) {
	float total = 0.0;
	float frequency = 1.0;
	float amplitude = 1.0;
	float max_val = 0.0;

	for (int i = 0; i < OCTAVES; i++) {
		total += perlinNoise(x * frequency, y * frequency, z, p) * amplitude;
		max_val += amplitude;
		amplitude *= PERSISTENCE;
		frequency *= 2;
	}
	return total / max_val; // 归一化
}
// 生成基础高程图（使用多频噪声）[3]()[9]()
void generateBaseTerrain(float* heightMap, int width, int height) {
	int p[1024] = {};
	initGradient(p, 1024);
	double z = 0.5;
	float xx = 0, ii = 10000;
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			double noiseValue = fractal_noise1(x, y, z, p) + 0.5;
			xx = fmax(noiseValue, xx);
			ii = fmin(noiseValue, ii);
			heightMap[x + y * width] = glm::clamp(noiseValue, 0.0, 0.125) * MAX_HEIGHT;
		}
	}
	printf("max=%f min=%f\n", xx, ii);
}

// 生成河流系统[3]()[6]()
void generateRivers(float* heightMap, int width, int height) {
	// 选择3个随机起点（山脉区域）
	for (int i = 0; i < 3; i++) {
		int startX = rand() % (width - 10) + 5;
		int startY = rand() % (height - 10) + 5;

		int x = startX, y = startY;

		// 河流长度（50-150步）
		int steps = 50 + rand() % 100;

		for (int s = 0; s < steps; s++) {
			// 寻找最低邻域点（水流方向）[3]()
			int nextX = x, nextY = y;
			float minHeight = heightMap[x + y * width];
			// 标记当前位置为河流 
			heightMap[x + y * width] = -10; // 特殊低值标记 


			// 检查8个相邻方向 
			for (int dx = -1; dx <= 1; dx++) {
				for (int dy = -1; dy <= 1; dy++) {
					int nx = x + dx;
					int ny = y + dy;

					if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
						if (heightMap[nx + ny * width] < minHeight) {
							minHeight = heightMap[nx + ny * width];
							nextX = nx;
							nextY = ny;
						}
					}
				}
			}

			// 如果到达最低点或边界，停止 
			if (nextX == x && nextY == y) break;

			x = nextX;
			y = nextY;
		}
	}
}

// 应用高斯平滑（使地形更自然）[3]()
void applySmoothing(float* heightMap, int width, int height) {
	float kernel[3][3] = {
		{1 / 16.0, 2 / 16.0, 1 / 16.0},
		{2 / 16.0, 4 / 16.0, 2 / 16.0},
		{1 / 16.0, 2 / 16.0, 1 / 16.0}
	};
	std::vector<float> tt;
	tt.resize(width * height);
	float* temp = tt.data();

	for (int x = 1; x < width - 1; x++) {
		for (int y = 1; y < height - 1; y++) {
			float sum = 0;
			for (int i = -1; i <= 1; i++) {
				for (int j = -1; j <= 1; j++) {
					sum += heightMap[x + i + (y + j) * width] * kernel[i + 1][j + 1];
				}
			}
			temp[x + y * width] = sum;
		}
	}

	// 复制回原数组 
	for (int x = 1; x < width - 1; x++) {
		for (int y = 1; y < height - 1; y++) {
			heightMap[x + y * width] = temp[x + y * width];
		}
	}
}

// 分类地形类型[2]()[7]()
TerrainType classifyTerrain(float height) {
	if (height < 0)
		return RIVER;	// 河流
	if (height < 50) return PLAIN;	// 平原
	if (height < 150) return HILL;	// 丘陵
	return MOUNTAIN;				// 山脉
}

// 导出高程图为PGM文件（可视化）
void exportHeightMap(float* heightMap, int width, int height, const char* filename) {
	std::vector<unsigned char> bit;// [WIDTH * HEIGHT] = {};
	bit.resize(width * height);
	auto t = bit.data();
	int mx = 0, mi = 100000;
	int f = 0;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			auto v = heightMap[x + y * width];
			mi = fmin(v, mi);
			mx = fmax(v, mx);
			if (v < 0)
			{
				f++;
			}
			unsigned char value = v;
			*t = value; t++;
		}
	}
	stbi_write_png(filename, width, height, 1, bit.data(), width);
	return;
	FILE* fp = fopen(filename, "w");
	fprintf(fp, "P2\n%d %d\n%d\n", width, height, MAX_HEIGHT);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int value = (int)fmax(0, heightMap[x + y * width]);
			fprintf(fp, "%d ", value);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}

// 打印地形统计信息 
void printTerrainStats(float* heightMap, int width, int height) {
	int counts[4] = { 0 }; // PLAIN, HILL, MOUNTAIN, RIVER 

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			TerrainType type = classifyTerrain(heightMap[x + y * width]);
			counts[type]++;
		}
	}

	printf("\n地形统计:\n");
	printf("平原占比: %.2f%%\n", counts[PLAIN] * 100.0 / (width * height));
	printf("丘陵占比: %.2f%%\n", counts[HILL] * 100.0 / (width * height));
	printf("山脉占比: %.2f%%\n", counts[MOUNTAIN] * 100.0 / (width * height));
	printf("河流占比: %.2f%%\n", counts[RIVER] * 100.0 / (width * height));
}

// 哈希函数生成固定随机值（基于坐标）
float noise_(int x, int y) {
	unsigned int n = x * 157 + y * 113;
	n = (n << 13) ^ n;
	n = (n * (n * n * 15731 + 789221) + 1376312589);
	return (float)(n & 0x7FFFFFFF) / 0x7FFFFFFF; // 归一化到[0,1]
}

// 双线性插值平滑 
float interpolate(float a, float b, float t) {
	return a + t * (b - a);
}

// 值噪声生成 
float value_noise(float x, float y) {
	int ix = (int)x;
	int iy = (int)y;
	float fx = x - ix;
	float fy = y - iy;

	float v1 = noise_(ix, iy);
	float v2 = noise_(ix + 1, iy);
	float v3 = noise_(ix, iy + 1);
	float v4 = noise_(ix + 1, iy + 1);

	float i1 = interpolate(v1, v2, fx);
	float i2 = interpolate(v3, v4, fx);
	return interpolate(i1, i2, fy);
}

// 分形噪声叠加 
float fractal_noise(float x, float y) {
	float total = 0.0;
	float frequency = 1.0;
	float amplitude = 1.0;
	float max_val = 0.0;

	for (int i = 0; i < OCTAVES; i++) {
		total += value_noise(x * frequency, y * frequency) * amplitude;
		max_val += amplitude;
		amplitude *= PERSISTENCE;
		frequency *= 2;
	}
	return total / max_val; // 归一化
}
int gen_heightmap(uint32_t seed, int width, int height, std::vector<float>* pheight_map, std::vector<int>* priver_path)
{
	pheight_map->resize(width * height);
	priver_path->resize(width * height);
	auto height_map = pheight_map->data();
	int* river_path = priver_path->data(); // 河流路径标记

	// 生成基础地形
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			height_map[x + y * width] = fractal_noise(x / 10.0f, y / 10.0f);
		}
	}

	// 生成河流（从随机高山点开始）
	int start_x = -1, start_y = -1;
	for (int attempt = 0; attempt < 100; attempt++) { // 最多尝试100次找源头 
		int x = rand() % width;
		int y = rand() % height;
		if (height_map[x + y * width] > 0.8f) {
			start_x = x;
			start_y = y;
			break;
		}
	}

	if (start_x != -1) { // 找到有效源头
		int x = start_x, y = start_y;
		for (int step = 0; step < 1000; step++) { // 最多1000步 
			river_path[x + y * width] = 1; // 标记河流路径
			float min_height = height_map[x + y * width];
			int next_x = x, next_y = y;

			// 检查8邻域最低点 
			for (int dy = -1; dy <= 1; dy++) {
				for (int dx = -1; dx <= 1; dx++) {
					if (dx == 0 && dy == 0) continue;
					int nx = x + dx, ny = y + dy;
					if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
						if (height_map[nx + ny * width] < min_height && !river_path[nx + ny * width]) {
							min_height = height_map[nx + ny * width];
							next_x = nx;
							next_y = ny;
						}
					}
				}
			}
			if (next_x == x && next_y == y) break; // 无更低点，停止流动 
			x = next_x;
			y = next_y;
		}
	}
	return 0;
}

int generateRivers(int width, int height, std::vector<float>* pheight_map, std::vector<int>* priver_path)
{
	auto height_map = pheight_map->data();
	int* river_path = priver_path->data(); // 河流路径标记

	// 生成河流（从随机高山点开始）
	int start_x = -1, start_y = -1;
	for (int attempt = 0; attempt < 100; attempt++) { // 最多尝试100次找源头 
		int x = rand() % width;
		int y = rand() % height;
		if (height_map[x + y * width] > 100) {
			start_x = x;
			start_y = y;
			break;
		}
	}
	int count = 0;
	if (start_x != -1) { // 找到有效源头
		int x = start_x, y = start_y;
		for (int step = 0; step < 1000; step++) { // 最多1000步 
			river_path[x + y * width] = 1; // 标记河流路径
			float min_height = height_map[x + y * width];
			int next_x = x, next_y = y;
			count++;
			// 检查8邻域最低点 
			for (int dy = -1; dy <= 1; dy++) {
				for (int dx = -1; dx <= 1; dx++) {
					if (dx == 0 && dy == 0) continue;
					int nx = x + dx, ny = y + dy;
					if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
						if (height_map[nx + ny * width] < min_height && !river_path[nx + ny * width]) {
							min_height = height_map[nx + ny * width];
							next_x = nx;
							next_y = ny;
						}
					}
				}
			}
			if (next_x == x && next_y == y)
				break; // 无更低点，停止流动 
			x = next_x;
			y = next_y;
		}
	}
	return 0;
}
int main_heightmap(uint32_t seed)
{

	{
		print_time aa("高度图生成");
		// 初始化随机数生成器[4]()[11]()
		initRandom(seed);
		int w = 50, h = 50;
		// 全局高度图
		std::vector<float> hm;
		std::vector<int> river_path;
		hm.resize(w * h);
		river_path.resize(w * h);
		float* heightMap = hm.data();
		// 地形生成流程 
		generateBaseTerrain(heightMap, w, h);   // 步骤1：生成基础地形 
		applySmoothing(heightMap, w, h);        // 步骤2：平滑处理 
		generateRivers(w, h, &hm, &river_path);
		int river_paths[50][50];
		for (size_t i = 0; i < h; i++)
		{
			auto& it = river_paths[i];
			for (size_t j = 0; j < w; j++)
			{
				it[j] = river_path[j + i * w];
			}
		}
		generateRivers(heightMap, w, h);        // 步骤3：生成河流  


		applySmoothing(heightMap, w, h);        // 步骤4：再次平滑
		// 导出和显示结果 
		exportHeightMap(heightMap, w, h, "terrain.png");
		printf("高程图已生成到 terrain.png\n种子：%u\n", seed);

		printTerrainStats(heightMap, w, h);
	}
	{
		/*
		参数项					山地		平原					对地形影响说明
		噪声幅度					1.0		0.3 ~ 0.5			决定整体起伏强度，越小越平坦
		八度数（octaves）		6 ~ 8	2 ~ 3				控制细节层次，越少越平滑
		持久度（persistence）	0.5		0.3 ~ 0.4			高频成分衰减更快，减少小起伏
		高度映射范围				0 ~ 1	0 ~ 0.4				人为压缩最大高程，突出低地
		后处理函数				线性		幂函数（如 x².⁵）	使中间值更集中，两端拉伸少
		*/
		// 1. 配置噪声模块（生成平原占比高的地形）
   // ---------------------- 
		noise::module::Perlin baseNoise;  // 基础噪声（Perlin噪声，平缓起伏）
		baseNoise.SetFrequency(0.0128);      // 频率（越小，起伏越平缓）
		baseNoise.SetOctaveCount(2);       // 八度（越少，细节越少）
		baseNoise.SetPersistence(0.3);     // 持续性（越小，高八度影响越小）
		baseNoise.SetLacunarity(2.0);      //  Lacunarity（频率倍增系数，默认2.0） 
		baseNoise.SetSeed(seed);
		// 可选：添加细节噪声（小幅度起伏，增强真实感） 
		noise::module::Perlin detailNoise;
		detailNoise.SetFrequency(0.8);     // 高频率（细节）
		detailNoise.SetOctaveCount(1);     // 1层细节 
		noise::module::Multiply detailMod;        // 细节噪声权重（0.1，不影响整体起伏）
		detailMod.SetSourceModule(0, detailNoise);
		noise::module::Const ct;
		ct.SetConstValue(0.1);
		detailMod.SetSourceModule(1, ct);

		// 组合基础噪声和细节噪声（基础占90%，细节占10%）
		noise::module::Add finalNoise;
		finalNoise.SetSourceModule(0, baseNoise);
		finalNoise.SetSourceModule(1, detailMod);

		// 调整噪声范围到[0, 255]（高程图像素值） 
		noise::module::ScaleBias scaleBias;
		scaleBias.SetSourceModule(0, finalNoise);
		scaleBias.SetScale(127.5);  // (-1到1) * 127.5 → -127.5到127.5
		scaleBias.SetBias(127.5);   // +127.5 → 0到255 

		// ---------------------- 
		// 2. 生成高程图数据（2D） 
		// ---------------------- 
		const int width = 50;  // 高程图宽度（像素）
		const int height = 50; // 高程图高度（像素）
		std::vector<float> heightMap(width * height);  // 存储每个像素的高度（0-255）
		std::vector<unsigned char> heightMap1(width * height);  // 存储每个像素的高度（0-255）

		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				// 将像素坐标映射到噪声空间（扩大范围，让起伏更平缓）
				double nx = (double)x / width * 200.0;  // x范围：0→200 
				double ny = (double)y / height * 200.0; // y范围：0→200 
				double nz = 0.0;                        // 2D高程图，z=0

				// 获取噪声值（已映射到0-255）
				double noiseValue = scaleBias.GetValue(nx, ny, nz);
				// 确保值在0-255之间（防止溢出）
				unsigned char pixelValue = static_cast<unsigned char>(std::clamp(noiseValue, 0.0, 255.0));
				heightMap[y * width + x] = pixelValue;
			}
		}
		applySmoothing(heightMap.data(), width, height);
		std::vector<int> river_path;
		river_path.resize(width * height);
		generateRivers(width, height, &heightMap, &river_path);
		for (auto i = 0; i < width * height; i++)
		{
			heightMap1[i] = (unsigned char)heightMap[i];
		}
		int river_paths[50][50];
		for (size_t i = 0; i < height; i++)
		{
			auto& it = river_paths[i];
			for (size_t j = 0; j < width; j++)
			{
				it[j] = river_path[j + i * width];
			}
		}
		// ---------------------- 
		// 3. 保存高程图（PNG可视化 + RAW导入3D引擎）
		// ---------------------- 
		// 保存为PNG（用图片查看器打开，灰度越浅表示海拔越高）
		stbi_write_png("heightmap_plains2.png", width, height, 1, heightMap1.data(), width);
		std::cout << "高程图生成成功！\n- PNG路径：heightmap_plains.png\n-  RAW路径：heightmap_plains.raw" << std::endl;
	}
	return 0;
}



int main()
{
	time(0);
	main_heightmap(1760245150);
	Sleep(1000);
	if (0) {
		//hz::main_ssh2();
		//return 0;
		//test_img();
		auto app = new_app();

#ifdef _DEBUG
		system("rd /s /q E:\\temcpp\\SymbolCache\\tcmp.pdb");
		system("rd /s /q E:\\temcpp\\SymbolCache\\vkcmp.pdb");
		system("rd /s /q E:\\temcpp\\SymbolCache\\cedit.pdb");
		system("rd /s /q E:\\temcpp\\SymbolCache\\p86.pdb");
		auto rd = hz::shared_load(R"(E:\Program Files\RenderDoc_1.37_64\renderdoc.dll)");
#endif 
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

		form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable /*| ef_borderless*/ | ef_transparent);
		//form_x* form1 = (form_x*)new_form(app, wtitle1, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable);
		auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
		auto kd = sdldev.vkdev;
		//sdldev.vkdev = 0;	// 清空使用独立创建逻辑设备
		std::vector<device_info_t> devs = get_devices(sdldev.inst); // 获取设备名称列表
		bool grab_enable = false;	// 设置鼠标范围在窗口内
		bool rmode = true;			// 设置窗口的相对鼠标模式。
		//form0->set_mouse_mode(grab_enable, rmode);
		get_queue_info(sdldev.phy);

		vkdg_cx* vkd = new_vkdg(sdldev.inst, sdldev.phy, sdldev.vkdev);	// 创建vk渲染器 
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
				vkd->add_gltf(R"(E:\model\sharp2.glb)", { 0,0,0 }, 1.0);// 地板
				//vkd->add_gltf(R"(E:\model\zw\fantasy_church_ruins.glb)", { -5,0,-6 }, 1.0);// 加载gltf
				//vkd->add_gltf(R"(E:\model\zw\autumnal_forest.glb)", { -15,0,-6 }, 1.0);// 加载gltf
				//vkd->add_gltf(R"(E:\model\realistic_palm_tree_10_free.glb)", { 2,0,0 }, 1.0);
				//vkd->add_gltf(R"(E:\model\bc22.glb)", { 0,0,5 }, 0.52);
				//vkd->add_gltf(R"(E:\vsz\h\avl\av\Bee.glb)", { 0,0,0 }, 10.0); 
				//vkd->add_gltf(R"(E:\code\c\assimp\test\models\glTF2\textureTransform\TextureTransformTest.gltf)", { 0,0,0 }, 1.0);
				// 机器
				//vkd->add_gltf(R"(E:\ag\glTFSample\media\Cauldron-Media\buster_drone\busterDrone.gltf)", { 0,01.8,0 }, 1.0, true);
				//vkd->add_gltf(R"(E:\model\lets_go_to_the_beach_-_beach_themed_diorama.glb)", { 0,0,20 }, 1.0);
				//vkd->add_gltf( R"(E:\model\hero_alice_lobby.glb)");
				//变形树
				//vkd->add_gltf(R"(E:\model\pale_radiance_tree.glb)", { }, 1.0);
				//vkd->add_gltf(R"(E:\model\ka-2000__scx2800-2_cranes (1).glb)", { 5,0,-8 }, 1.0);
				//vkd->add_gltf(R"(E:\model\maple_trees.glb)", { 20,0,10 }, 0.10);

				//vkd->add_gltf(R"(E:\model\rock_monster.glb)", { 5,0,10 }, 0.5);
				vkd->add_gltf(R"(E:\model\helicopter_space_ship.glb)", { 0,0,0 }, 1.0, true);// 飞船
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
