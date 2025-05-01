#include <pch1.h>
#include "minesweeper.h"
struct res_a
{
	int mwidth = 32;
	int step = 2;
	glm::ivec4 mnum = { 10,10,32,32 };		// 9个，0-8*
	glm::ivec4 block_0 = { 10,50,32,32 };	// 3个，凸、凹、红*
	glm::ivec4 block = { 330,10,68,68 };	// 覆盖块，7种风格*
	glm::ivec4 num = { 10,90,30,56 };		// 数字0-10*
	glm::ivec4 bq = { 10,170,66,64 };		// 表情，3种*
	glm::ivec4 mine = { 10,240,70,70 };		// 雷
	glm::ivec4 flag = { 82,240,70,70 };		// 旗帜
	glm::ivec4 flag1 = { 10,320,70,70 };	// 旗帜
	glm::ivec4 rcha = { 160,240,70,70 };	// 红叉
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
	// 随机放置雷
	for (int i = 0; i < mine_count; i++)
	{
		int idx = dis(gen);
		if (_map[idx].type == -1 || idx == empty_pos)
		{
			i--;
			continue;
		}
		_map[idx].type = -1;
	}
	// 计算周围雷数
	for (int i = 0; i < _map.size(); i++) {
		for (int j = -1; j <= 1; j++)
		{
			for (int k = -1; k <= 1; k++)
			{
				int x = i % size.x + j;
				int y = i / size.x + k;
				if (x < 0 || x >= size.x || y < 0 || y >= size.y)
					continue;
				int nidx = x + y * size.x;
				if (_map[nidx].type != -1)
					_map[nidx].type++;
			}
		}
	}

}

bool expand_blank(minesweeper_cx::node_t* mine, int x, int y, int w, int h)
{
	int front = 0, rear = 0;
	bool ret = false;
	std::queue<glm::ivec2> q;  // 队列存储待处理坐标 
	// 初始化队列，标记已处理 
	mine[x + y * w].search = 1;  // 展开当前格子 
	mine[x + y * w].status = 1;
	q.push({ x, y });
	int dx[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
	int dy[] = { -1, 0, 1, -1, 1, -1, 0, 1 };

	while (front < rear) {
		auto current = q.front(); q.pop();
		int cx = current.x, cy = current.y;
		// 遍历8个方向 
		for (int i = 0; i < 8; i++) {
			int nx = cx + dx[i];
			int ny = cy + dy[i];

			// 检查坐标有效性（保护层内）
			if (nx < 1 || nx >= w || ny < 1 || ny >= h) continue;
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
	if (g_result != 0 || texture == 0)return;
	_draw_data.clear();
	_draw_data.reserve(size.x * size.y * 2);
	for (size_t y = 0; y < size.y; y++)
	{
		auto yh = y * size.x;
		for (size_t x = 0; x < size.x; x++)
		{
			int idx = x + yh;
			auto& it = _map[idx];

			struct nodea_t
			{
				char type = 0;		 // 0-8数字，-1雷
				char status = 0;	 // 状态，0未翻开、1已翻开、2叉、3爆
				char mark = 0;		 // 0无，1旗帜、2问号、3叹号
				bool search = false; // 是否搜索过
			};
			struct daraw_data_t
			{
				glm::ivec4 src, dst;
				float scale = 1.0f;
				float w9 = 0;// 9Grid
			};
			if (it.type < 1)
			{
				draw_data_t dd = {};
				dd.src = res->mnum;
				dd.dst = { pos.x + x * res->mwidth,  pos.y + y * res->mwidth, dd.src.z,dd.src.w };
				dd.scale = 1.0f;
				_draw_data.push_back(dd);
				if (it.type < 0)
				{
					dd.dst = { pos.x + x * res->mwidth,  pos.y + y * res->mwidth, dd.src.z,dd.src.w };
					dd.src = res->mine;
					dd.scale = (double)dd.dst.z / res->mine.z;
					_draw_data.push_back(dd);
				}
			}
			else
			{
				draw_data_t dd = {};
				dd.src = res->mnum;
				dd.src.x += it.type * (res->mwidth + res->step);
				dd.dst = { pos.x + x * res->mwidth, pos.y + y * res->mwidth, dd.src.z,dd.src.w };
				dd.scale = 1.0f;
				_draw_data.push_back(dd);
			}
		}
	}
}

void minesweeper_cx::send_event(const glm::ivec2& pos, int btn)
{
	if (g_result != 0)return;
	if (pos.x < 0 || pos.x >= size.x || pos.y < 0 || pos.y >= size.y)
		return;
	int idx = pos.x + pos.y * size.x;
	if (_map[idx].status == 1)
		return;
	if (btn == 0)
	{
		if (_map[idx].mark > 0) {
			flag_count--; _map[idx].mark = 0;
			return;
		}
		if (_map[idx].type == -1)
		{
			g_result = 2;
			_map[idx].status = 3;
			return;
		}
		else
		{
			_map[idx].status = 1;
			if (_map[idx].type == 0)
			{
				expand_blank(_map.data(), pos.x, pos.y, size.x, size.y);
			}
		}
	}
	else if (btn == 1)
	{
		if (_map[idx].mark == 0)
		{
			flag_count++; _map[idx].mark = 1;
		}
		else
		{
			flag_count--; _map[idx].mark = 0;
		}
	}
	else if (btn == 2)
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
