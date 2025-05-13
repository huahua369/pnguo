/*
信号逻辑门电路模拟
创建日期：2025/5/9
作者：华仔
*/
#include <pch1.h>
#include <tiny2d.h>
#include "logic_gates.h"

glm::ivec4 get_lgates_rc(int i)
{
	glm::ivec4 rc = { 0,0,0,0 };
	switch (i)
	{
	case 0: rc = { 10,0,6,6 }; break;
	case 1: rc = { 10,50,12,12 }; break;
	case 2: rc = { 10,30,90,62 }; break;
	case 3: rc = { 10,110,90,62 }; break;
	case 4: rc = { 130,30,90,62 }; break;
	case 5: rc = { 130,110,90,62 }; break;
	case 6: rc = { 230,0,30,30 }; break;
	case 7: rc = { 230,30,30,30 }; break;
	case 8: rc = { 270,0,16,30 }; break;
	case 9: rc = { 270,30,16,30 }; break;
	case 10: rc = { 300,0,30,30 }; break;
	case 11: rc = { 300,30,30,30 }; break;
	default:
		break;
	}
	return rc;
}

// 信号线，
