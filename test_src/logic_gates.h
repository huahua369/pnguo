/*
信号逻辑门电路模拟
逻辑门、信号线、信号生产器、存储器。
信号线用于连接到逻辑门的引脚，或者连接到其他信号线
*/
#pragma once

enum class dType :uint8_t {
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
struct sline_t
{
	int x, y;
	int x1, y1;
};

