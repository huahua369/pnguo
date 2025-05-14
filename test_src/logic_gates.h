/*
信号逻辑门电路模拟
逻辑门、信号线、信号生产器、存储器。
信号线用于连接到逻辑门的引脚，或者连接到其他信号线
*/
#pragma once

enum class dType :uint16_t {
	NULL_ST,			// 空
	SIGNAL_SWITCH,	    // 信号开关，一输出
	SIGNAL_LINE,		// 信号线，连接到逻辑门的引脚，或者连接到其他信号线
	SIGNAL_LINE_GROUP,	// 信号线组，4位
	SIGNAL_SELECTOR,	// 信号选择器，四个输入引脚，两个控制输入，一个输出引脚
	SIGNAL_DISTRIBUTOR,	// 信号分配器，四个输出引脚，两个控制输入，一个输入引脚
	RIBBON_READER,		// 线组读取器
	RIBBON_WRITER,		// 线组写入器
	AND_GATE, OR_GATE,	// 与，或。两个输入引脚，一个输出引脚
	NOT_GATE,			// 非门。一输入一输出
	XOR_GATE, 			// 异或门。二输入一输出
	LATCH,				// 锁存器。一输入，一重置输入，一输出。Memory Toggle
	FLIP_FLOP,			// 触发器。一输入，一重置输入，一输出。
	FILTER_GATE,		// 过滤门，一输入一输出
	BUFFER_GATE,		// 缓冲门，一输入一输出
	TIMER_SENSOR,		// 定时器，设置时间（红，绿时间，重置），一输出
	HYDRO_SENSOR,		// 液压传感器，设置液体压力，一输出
	ATMO_SENSOR,		// 气压传感器，设置气体压力，一输出
	THERMO_SENSOR,		// 温度传感器，设置温度，一输出
	WEIGHT_PLATE,		// 重量传感器，设置重量，一输出
};
/*
信号飞线,
信号线组，4位
*/
struct gatedata_st
{
	glm::ivec4 pos2 = {};	// 头尾中心位置
	std::string name;
	uint16_t type = 0;
	int8_t input_count;		// 输入引脚数量
	int8_t output_count;	// 输出引脚数量
	uint8_t input = 0;		// 输入引脚最大8个
	uint8_t output = 0;		// 输入引脚
	int degrees = 0;		// 旋转角度
	int8_t build = 0;		// 建造，0-100，等于0时建造完成
	bool visible = 1;
};
// 0小圆点，1大圆点，2非，3与，4或，5异或
glm::ivec4 get_lgates_rc(int i);

class logic_cx
{
public:
	atlas_xt* axt = 0;
	njson gat;
	std::map<std::string, std::vector<int>> gat_map;
	glm::vec2 _pos = {};
	float _scale = 1.0;
	std::vector<gatedata_st> gates;
public:
	logic_cx();
	~logic_cx();
	void init(atlas_xt* p);
	void add_gate(dType t, const std::string& name, const glm::ivec2& pos, int degrees);
	void draw_update(double delta);
private:
	void draw_gate(gatedata_st* p, float scale);

	void draw_and(gatedata_st* p, float scale);
	void draw_or(gatedata_st* p, float scale);
	void draw_not(gatedata_st* p, float scale);
	void draw_xor(gatedata_st* p, float scale);
};

logic_cx* new_logic(atlas_xt* xha);
void free_logic(logic_cx* p);
