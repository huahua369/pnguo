#include <pch1.h>
#include "minesweeper.h"
struct res_a
{
	int mwidth = 32;
	int step = 2;
	glm::ivec4 mnum = { 10,10,32,32 };		// 9个，0-8*
	glm::ivec4 block_0 = { 10,50,32,32 };	// 3个，凸、凹、红*、叉
	glm::ivec4 block = { 330,10,68,68 };	// 覆盖块，7种风格*
	glm::ivec4 num = { 10,90,30,56 };		// 数字0-10*
	glm::ivec4 bq = { 148,50,32,32 };		// 表情，3种*
	glm::ivec4 mine = { 252,50,32,32 };		// 雷
	glm::ivec4 flag = { 290,50,32,32 };		// 旗帜
	glm::ivec4 flag1 = { 10,320,70,70 };	// 旗帜 
	glm::ivec4 ask = { 240,240,40,70 };		// 问号
	glm::ivec4 ask1 = { 290,240,40,70 };	// 问号
	glm::ivec4 sigh = { 340,240,14,70 };	// 叹号
	glm::ivec4 sigh1 = { 360,240,14,70 };	// 叹号
	float scale = 1.0f;	// 缩放
};

minesweeper_cx::minesweeper_cx()
{
	res = new res_a();
}

minesweeper_cx::~minesweeper_cx()
{
	if (res)delete res; res = 0;
}

void minesweeper_cx::set_texture(void* tex)
{
	texture = tex;
}

void minesweeper_cx::resize(int w, int h, int mc)
{
	if (w < 6)
		w = 6;
	if (h < 6)
		h = 6;
	if (mc < 6)
		mc = 6;
	size.x = w;
	size.y = h;
	mine_count = mc;
	flag_count = 0;
	tick = 0;
}

void minesweeper_cx::clear_map()
{
	_map.clear();
	_map.resize(size.x * size.y);
	for (int i = 0; i < _map.size(); i++)
	{
		_map[i] = {};
	}
}

void minesweeper_cx::init_map(const glm::ivec2& pos)
{
	// 清除地图
	clear_map();
	// 随机数种子
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dis(0, _map.size() - 1);
	int empty_pos = pos.x + pos.y * size.x;
	int xx[9] = { 0,0,0,0,0,0,0,0,0 };
	int dx[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
	int dy[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
	for (int i = 0; i < 8; i++) {
		int nx = pos.x + dx[i];
		int ny = pos.y + dy[i];
		xx[i] = nx + ny * size.x;
	}
	// 随机放置雷 
	for (int i = 0; i < mine_count; i++)
	{
		int idx = dis(gen);
		for (size_t k = 0; k < 8; k++)
		{
			if (idx == xx[k])
			{
				idx = -1;
				break;
			}
		}
		if (idx < 0 || _map[idx].type == -1 || idx == empty_pos)
		{
			i--;
			continue;
		}
		_map[idx].type = -1;
	}
	// 计算周围雷数

	for (size_t y = 0; y < size.y; y++)
	{
		auto yh = y * size.x;
		for (size_t x = 0; x < size.x; x++)
		{
			int idx = x + yh;
			auto& it = _map[idx];
			if (it.type == -1)continue;
			// 遍历8个方向 
			for (int i = 0; i < 8; i++) {
				int nx = x + dx[i];
				int ny = y + dy[i];
				// 检查坐标有效性（保护层内）
				if (nx < 0 || nx >= size.x || ny < 0 || ny >= size.y) continue;
				auto& pt = _map[nx + ny * size.x];
				if (pt.type == -1)
				{
					it.type++;
				}
			}
		}
	}
	return;
}

bool minesweeper_cx::expand_blank(int x, int y)
{
	bool ret = false;
	int w = size.x, h = size.y;
	std::stack<glm::ivec2>& q = _stack_e;  // 队列存储待处理坐标 
	// 初始化队列，标记已处理 
	for (auto& it : _map) { it.search = 0; }//清空上次搜索标记
	auto mine = _map.data();
	mine[x + y * w].search = 1;  // 展开当前格子 
	mine[x + y * w].status = 1;
	while (q.size()) {
		q.pop();
	}
	q.push({ x, y });
	int dx[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
	int dy[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
	while (q.size()) {
		auto current = q.top(); q.pop();
		int cx = current.x, cy = current.y;
		// 遍历8个方向 
		for (int i = 0; i < 8; i++) {
			int nx = cx + dx[i];
			int ny = cy + dy[i];
			if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
			auto& pt = mine[nx + ny * w];
			// 若未处理且为空白或数字 
			if (!pt.search && pt.type != -1) {
				pt.status = 1;  // 显示数字或空白 
				pt.search = 1;  // 标记为已处理
				if (pt.type == 0) {  // 仅将空白格子加入队列 
					q.push({ nx, ny });
				}
				ret = true;
			}
		}
	}
	return ret;
}

void minesweeper_cx::update(const glm::ivec2& pos, double delta)
{
	_pos = pos;
	if (/*g_result != 0 ||*/ texture == 0)return;
	_draw_data.clear();
	_draw_data.reserve(size.x * size.y * 2);
	for (size_t y = 0; y < size.y; y++)
	{
		auto yh = y * size.x;
		for (size_t x = 0; x < size.x; x++)
		{
			int idx = x + yh;
			auto& it = _map[idx];

			if (it.type < 1)
			{
				draw_data_t dd = {};
				dd.src = res->mnum;
				if (it.status == 2)
				{
					dd.src = res->block_0;
					dd.src.x += it.status * (res->mwidth + res->step);// 爆雷
				}
				dd.dst = { pos.x + x * res->mwidth,  pos.y + y * res->mwidth, dd.src.z,dd.src.w };
				dd.scale = 1.0f;
				_draw_data.push_back(dd);
				if (it.type < 0)
				{
					dd.src = res->mine;// 显示雷
					dd.dst = { pos.x + x * res->mwidth,  pos.y + y * res->mwidth, dd.src.z,dd.src.w };
					dd.scale = 0.86;
					auto d2 = res->mwidth * (1.0 - dd.scale);
					dd.dst.z -= d2;
					dd.dst.w -= d2;
					dd.dst.x += d2 * 0.5;
					dd.dst.y += d2 * 0.5;
					dd.scale = 1.0;
					_draw_data.push_back(dd);
				}
			}
			else
			{
				draw_data_t dd = {};
				dd.src = res->mnum;//雷数量
				dd.src.x += it.type * (res->mwidth + res->step);
				dd.dst = { pos.x + x * res->mwidth, pos.y + y * res->mwidth, dd.src.z,dd.src.w };
				dd.scale = 1.0f;
				_draw_data.push_back(dd);
			}
			if (it.status == 0)
			{
				draw_data_t dd = {};// 未翻开
				dd.src = res->block_0; dd.src.x += it.status * (res->mwidth + res->step);
				dd.dst = { pos.x + x * res->mwidth, pos.y + y * res->mwidth, dd.src.z,dd.src.w };
				dd.scale = 1.0f;
				_draw_data.push_back(dd);
			}
			if (it.mark > 0)
			{
				draw_data_t dd = {};
				dd.src = res->flag;
				dd.dst = { pos.x + x * res->mwidth, pos.y + y * res->mwidth, dd.src.z,dd.src.w };
				dd.scale = 1.0f;
				if (!(it.mark == 1 && it.status == 3))
					_draw_data.push_back(dd);
			}
			if (it.status == 3)
			{
				draw_data_t dd = {};// 叉
				if (it.mark == 1)
				{
					dd.src = res->mine;// 显示雷
					dd.dst = { pos.x + x * res->mwidth,  pos.y + y * res->mwidth, dd.src.z,dd.src.w };
					dd.scale = 0.86;
					auto d2 = res->mwidth * (1.0 - dd.scale);
					dd.dst.z -= d2;
					dd.dst.w -= d2;
					dd.dst.x += d2 * 0.5;
					dd.dst.y += d2 * 0.5;
					dd.scale = 1.0;
					_draw_data.push_back(dd);
				}
				dd.src = res->block_0; dd.src.x += it.status * (res->mwidth + res->step);
				dd.dst = { pos.x + x * res->mwidth, pos.y + y * res->mwidth, dd.src.z,dd.src.w };
				dd.scale = 1.0f;
				_draw_data.push_back(dd);
			}
		}
	}
}

void minesweeper_cx::send_event(const glm::ivec2& opos, int btn)
{
	auto pos = opos - _pos;
	pos /= res->mwidth;
	if (g_result < 0)
	{
		init_map(pos);
		g_result = 0;
	}
	if (g_result != 0)return;
	if (pos.x < 0 || pos.x >= size.x || pos.y < 0 || pos.y >= size.y)
		return;
	int idx = pos.x + pos.y * size.x;
	auto& pt = _map[idx];
	if (pt.status == 1)
		return;
	if (btn == 0)//左
	{
		if (pt.mark > 0) {
			flag_count--; pt.mark = 0;
			return;
		}
		if (pt.type == -1)
		{
			g_result = 2;
			for (auto& it : _map) {
				it.status = 1;
				if (it.mark == 1 && it.type != -1)
				{
					it.status = 3;
				}
			}
			pt.status = 2;
			return;
		}
		else
		{
			pt.status = 1;
			if (pt.type == 0)
			{
				expand_blank(pos.x, pos.y);
			}
		}
	}
	else if (btn == 1)//右
	{
		if (pt.mark == 0)
		{
			flag_count++; pt.mark = 1;
			if (flag_count == mine_count)
			{
				int count = 0;
				for (auto& it : _map) {
					if (it.type == -1 && it.mark == 1)
					{
						count++;
					}
				}
				if (count == flag_count)
				{
					g_result = 1; 
					for (auto& it : _map) {
						if (it.status == 0)
						{
							it.status = 1;
						}
					}
				}
			}
		}
		else
		{
			flag_count--; pt.mark = 0;
		}
	}
	else if (btn == 2)// 双击
	{

	}

}

minesweeper_cx::draw_data_t* minesweeper_cx::data()
{
	return _draw_data.empty() ? nullptr : _draw_data.data();
}

size_t minesweeper_cx::count()
{
	return _draw_data.size();
}
