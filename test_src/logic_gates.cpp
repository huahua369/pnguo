/*
信号逻辑门电路模拟
创建日期：2025/5/9
作者：华仔
*/
#include <pch1.h>
#include <tiny2d.h>
#include <tinysdl3.h>
#include <editor_2d.h>
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
logic_cx* new_logic(atlas_xt* xha)
{
	auto p = new logic_cx();
	if (p)
		p->init(xha);
	return p;
}
void free_logic(logic_cx* p)
{
	if (p)delete p;
}

njson new_lgat()
{
	njson gat;
	// 非门
	gat["not"]["images"] = { "not_b","not_w","not_gr","not_rg","ygs0" };
	// 与
	gat["and"]["images"] = { "and_b","and_w","and_g0","and_g01","and_g1","and_g11","and_g2"
		,"and_r0","and_r01","and_r1","and_r11","and_r2","and_bg","and_rc0","and_rc1" };
	// 或
	gat["or"]["images"] = { "or_b","or_w","or_g0","or_g1","or_g2","or_r0","or_r1","or_r2" };
	// 异或
	gat["xor"]["images"] = { "xor_b","xor_w","xor_g0","xor_g1","xor_g2","xor_r0","xor_r1","xor_r2"
		,"xor_c","xor_c0","xor_c1" };
	// 小圆
	gat["yuan0"]["images"] = { "yg0","yr0","yw0" };
	// 大圆
	gat["yuan1"]["images"] = { "yg1","yr1","yw1" };
	// 线
	gat["line_blue"]["images"] = { "line_blue0","line_blue1","line_blue2","line_blue3","line_blue4","line_blue5" };
	gat["line_red"]["images"] = { "line_red0","line_red1","line_red2","line_red3","line_red4","line_red5" };
	gat["line_green"]["images"] = { "line_green0","line_green1","line_green2","line_green3","line_green4","line_green5" };
	gat["line_white"]["images"] = { "line_white0","line_white1","line_white2","line_white3","line_white4","line_white5" };
	// 线桥
	gat["line_bridge"]["images"] = { "line_bridge0","line_bridge1","line_bridge2","line_bridge3" };


	return gat;
}

logic_cx::logic_cx()
{
}

logic_cx::~logic_cx()
{
}

void logic_cx::init(atlas_xt* p)
{
	gat = new_lgat();
	if (p)
	{
		axt = p;
		for (auto& [k, v] : gat.items()) {
			auto& ip = v["images"];
			auto& gm = gat_map[k];
			for (size_t i = 0; i < ip.size(); i++)
			{
				auto it = ip[i].get<std::string>();
				auto a = atlas_findi(p, it);
				gm.push_back(a);
			}
		}
	}
}

void logic_cx::add_gate(dType t, const std::string& name, const glm::ivec2& pos, int degrees)
{
	gatedata_st st = {};
	st.type = (uint16_t)t;
	st.name = name;
	st.pos2 = { pos.x,pos.y,0,0 };
	st.degrees = degrees;
	gates.push_back(st);
}

void draw_atlas(atlas_xt* xha, size_t idx, const glm::vec2& pos, int degrees, float scale)
{
	if (!xha || idx >= xha->region.size())return;
	auto& xr = xha->region;
	texture_angle_dt adt1 = {};
	auto& k = xr[idx];
	adt1.src_rect = k.bounds;
	adt1.dst_rect = { pos.x + k.offsets.x,pos.y + k.offsets.y,adt1.src_rect.z,adt1.src_rect.w };
	if (degrees != 0) {
		adt1.angle = degrees;
		if (k.offsets.z > 0 && k.offsets.w > 0)
		{
			adt1.center = { k.offsets.z / 2,k.offsets.w / 2 };
		}
		else
		{
			adt1.center = { adt1.src_rect.z / 2,adt1.src_rect.w / 2 };
		}
	}
	int sc = scale * 100;
	if (sc > 0 && sc != 100)
	{
		scale = sc / 100.0f;
		adt1.dst_rect *= scale;
		adt1.center *= scale;
	}
	xha->rendererObject->cb->render_texture_rotated(xha->rendererObject->renderer, xha->pages->rendererObject, &adt1, 1);
}
void logic_cx::draw_and(gatedata_st* p, float scale)
{
	auto it = gat_map.find("and");
	if (it != gat_map.end())
	{
		std::string ak[] = { "and_b","and_w","and_g0","and_g01","and_g1","and_g11","and_g2"
,"and_r0","and_r01","and_r1","and_r11","and_r2","and_bg","and_rc0","and_rc1" };
		//2 - 6;
		//7-11,13,14
		auto& gm = it->second;
		glm::vec2 pos = { p->pos2.x * scale,p->pos2.y * scale };
		pos += _pos;
		if (p->build > 0)
		{
			draw_atlas(axt, gm[1], pos, p->degrees, scale);
		}
		else
		{
			draw_atlas(axt, gm[0], pos, p->degrees, scale);
			glm::ivec2 ipt = { 7,9 };
			glm::ivec3 ipt1 = { 13,10,11 };
			if (p->input & 1)
			{
				ipt.x = 2;
				ipt1.x = 3;
			}
			else {
				ipt1.x = 8;
			}
			if (p->input & 2)
			{
				ipt.y = 4;
				ipt1.y = !(p->input & 1) ? 14 : 5;
			}

			if (p->input & 1 && p->input & 2) {
				p->output = 1;
			}
			else {
				p->output = 0;
			}
			if (p->output & 1)
			{
				ipt1.z = 6;
			}
			// bg
			draw_atlas(axt, gm[12], pos, p->degrees, scale);
			draw_atlas(axt, gm[ipt.x], pos, p->degrees, scale);
			draw_atlas(axt, gm[ipt.y], pos, p->degrees, scale);
			draw_atlas(axt, gm[ipt1.x], pos, p->degrees, scale);
			draw_atlas(axt, gm[ipt1.y], pos, p->degrees, scale);
			draw_atlas(axt, gm[ipt1.z], pos, p->degrees, scale);
		}
	}
}
void logic_cx::draw_or(gatedata_st* p, float scale)
{
	auto it = gat_map.find("or");
	if (it != gat_map.end())
	{
		std::string ak[] = { "or_b","or_w"
			,"or_g0","or_g1","or_g2","or_r0","or_r1","or_r2" };
		//		2		3		4		5		6		7
		auto& gm = it->second;
		glm::vec2 pos = { p->pos2.x * scale,p->pos2.y * scale };
		pos += _pos;
		if (p->build > 0)
		{
			draw_atlas(axt, gm[1], pos, p->degrees, scale);
		}
		else
		{
			draw_atlas(axt, gm[0], pos, p->degrees, scale);
			glm::ivec3 ipt = { 5,6,7 };
			if (p->input & 1)
			{
				ipt.x = 2;
			}
			if (p->input & 2)
			{
				ipt.y = 3;
			}

			if (p->input & 1 || p->input & 2) {
				p->output = 1;
			}
			else {
				p->output = 0;
			}
			if (p->output & 1)
			{
				ipt.z = 4;
			}
			// bg 
			draw_atlas(axt, gm[ipt.x], pos, p->degrees, scale);
			draw_atlas(axt, gm[ipt.y], pos, p->degrees, scale);
			draw_atlas(axt, gm[ipt.z], pos, p->degrees, scale);
		}
	}
}

void logic_cx::draw_xor(gatedata_st* p, float scale)
{
	auto it = gat_map.find("xor");
	if (it != gat_map.end())
	{
		std::string ak[] = { "xor_b","xor_w"//12
			,"xor_g0","xor_g1","xor_g2"//234
			,"xor_r0","xor_r1","xor_r2"			//567
			,"xor_c","xor_c0","xor_c1" };//89 10
		auto& gm = it->second;
		glm::vec2 pos = { p->pos2.x * scale,p->pos2.y * scale };
		pos += _pos;
		if (p->build > 0)
		{
			draw_atlas(axt, gm[1], pos, p->degrees, scale);//建造中
		}
		else
		{
			draw_atlas(axt, gm[0], pos, p->degrees, scale);//背板
			glm::ivec3 ipt = { 5,6,7 };
			if (p->input & 1)
			{
				ipt.x = 2;
			}
			if (p->input & 2)
			{
				ipt.y = 3;
			}
			bool b0 = (p->input & 1), b1 = (p->input & 2);
			bool b = b0 ^ b1;
			if (b) {
				p->output = 1;
				ipt.z = 4;
			}
			else {
				p->output = 0;
				if (p->input == 0)
				{
					ipt.z = 9;
				}
				if (p->input == 0x03)
				{
					ipt.z = 10;
				}
			}

			// bg 
			draw_atlas(axt, gm[8], pos, p->degrees, scale);
			draw_atlas(axt, gm[ipt.x], pos, p->degrees, scale);
			draw_atlas(axt, gm[ipt.y], pos, p->degrees, scale);
			draw_atlas(axt, gm[ipt.z], pos, p->degrees, scale);
			if (!b)
				draw_atlas(axt, gm[7], pos, p->degrees, scale);
		}
	}
}
void logic_cx::draw_not(gatedata_st* p, float scale)
{
	auto it = gat_map.find("not");
	if (it != gat_map.end())
	{
		std::string ak[] = { "not_b","not_w","not_gr","not_rg" };
		auto& gm = it->second;
		glm::vec2 pos = { p->pos2.x * scale,p->pos2.y * scale };
		pos += _pos;
		if (p->build > 0)
		{
			draw_atlas(axt, gm[1], pos, p->degrees, scale);
		}
		else
		{
			draw_atlas(axt, gm[0], pos, p->degrees, scale);
			int x = 3;
			if (p->input & 1)
			{
				x = 2;
			}
			draw_atlas(axt, gm[x], pos, p->degrees, scale);
			if (p->input & 1)
				draw_atlas(axt, gm[4], pos, p->degrees, scale);//光晕
		}
	}
}

void logic_cx::draw_gate(gatedata_st* p, float scale)
{
	auto type = (dType)p->type;
	switch (type)
	{
	case dType::NULL_ST:
		break;
	case dType::SIGNAL_SWITCH:
		break;
	case dType::SIGNAL_LINE:
		break;
	case dType::SIGNAL_LINE_GROUP:
		break;
	case dType::SIGNAL_SELECTOR:
		break;
	case dType::SIGNAL_DISTRIBUTOR:
		break;
	case dType::RIBBON_READER:
		break;
	case dType::RIBBON_WRITER:
		break;
	case dType::AND_GATE:
		draw_and(p, scale);
		break;
	case dType::OR_GATE:
		draw_or(p, scale);
		break;
	case dType::NOT_GATE:
		draw_not(p, scale);
		break;
	case dType::XOR_GATE:
		draw_xor(p, scale);
		break;
	case dType::LATCH:
		break;
	case dType::FLIP_FLOP:
		break;
	case dType::FILTER_GATE:
		break;
	case dType::BUFFER_GATE:
		break;
	case dType::TIMER_SENSOR:
		break;
	case dType::HYDRO_SENSOR:
		break;
	case dType::ATMO_SENSOR:
		break;
	case dType::THERMO_SENSOR:
		break;
	case dType::WEIGHT_PLATE:
		break;
	default:
		break;
	}
}

void logic_cx::draw_update(double delta)
{
	auto length = gates.size();
	for (size_t i = 0; i < length; i++)
	{
		auto p = &gates[i];
		draw_gate(p, _scale);
	}
}

