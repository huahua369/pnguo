#pragma once


// 蜡烛台
struct candlestick_css {
	glm::ivec2 color = { 0xff0020ff, 0xff00ff20 };		// 显示颜色：阳、阴
	glm::vec2 width = { 5, 1 };	// 宽度
	float height;			// 图表高
	float space = 1;		// 间隔
	float tks = 1;			// 线粗 thickness
	glm::i16vec2 rc = { 0,1 };// 0空心，1填充
	uint32_t macol[4] = { 0xffC67054, 0xff75CC91, 0xff58C8FA, 0xff6666EE };
};
// 蜡烛台图表
class charts_u1
{
public:
	glm::vec2 _pos = {};
	size_t cidx = 0;	//显示下标
private:
	std::vector<glm::vec4> kdata;
	glm::vec2 _size = {};
	candlestick_css _ct = {};
	glm::ivec4 bc = { 0x80ff9e40, 0x20000000,1,4 };
	std::map<int, std::vector<glm::vec2>> ma;
public:
	charts_u1();
	~charts_u1();
	void set_rect(const glm::vec4& r);
	void set_color(const glm::ivec4& c);
	void set_filltype(const glm::ivec2& c);
	candlestick_css* get_ct();
	void push_v4(glm::vec4* v, int n);
	void push_line(const char* str, int n);
	void update(const glm::ivec4& id = { 0, 1, 2, 3 });
	void make_ma(int n);
public:

	void update_draw(cairo_t* cr);
private:

};

std::string get_linestr(const char** p, int& len);
