#include <pch1.h>
#include "minesweeper.h"
#include <mapView.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <pnguo/win_core.h>
#include "win32msg.h"
#include <pnguo/event.h>
#define GUI_STATIC_LIB
#include <pnguo/pnguo.h>
#include <tinysdl3.h>
#include <pnguo/editor_2d.h>
#include <spine/spine-sdl3/spinesdl3.h>
#include <stb_image_write.h>


struct res_a
{
	int mwidth = 32;
	int title_height = 60;
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
	load_list();
}

minesweeper_cx::~minesweeper_cx()
{
	if (res)delete res; res = 0;
}

void minesweeper_cx::set_texture(void* tex)
{
	texture = tex;
}

void minesweeper_cx::resize(int w, int h, float mc)
{
	if (w < 6)
		w = 6;
	if (h < 6)
		h = 6;
	mcc = mc;
	if (mc < 0.9)
	{
		mc = w * h * mc;
		int nc = mc;
		if (nc & 1)nc++;
		mc = nc;
	}
	if (mc < 6)
		mc = 6;
	size.x = w;
	size.y = h;
	mine_count = mc;
	save();
}

void minesweeper_cx::resize(int w, int h, float mc, glm::ivec2* mc_pos)
{
	resize(w, h, mc);
	// 清除地图
	clear_map();
	auto t = mc_pos;
	for (int i = 0; i < mine_count; i++)
	{
		int idx = t->x + t->y * w;
		t++;
		if (idx < 0 || _map[idx].type == -1)
		{
			i--;
			continue;
		}
		_map[idx].type = -1;
	}
	// 计算周围雷数
	get_mine_count();
	g_result = 0;
	save();
}

void minesweeper_cx::clear_map()
{
	_map.clear();
	_map.resize(size.x * size.y);
	for (int i = 0; i < _map.size(); i++)
	{
		_map[i] = {};
	}
	flag_count = 0;
	tick = 0;
}

void minesweeper_cx::init_map(const glm::ivec2& pos)
{
	// 清除地图
	clear_map();
	// 随机数种子
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dis(1, _map.size() - 2);
	int empty_pos = pos.x + pos.y * size.x;
	int xx[9] = { 0,0,0,0,0,0,0,0,0 };
	int dx[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
	int dy[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
	for (int i = 0; i < 8; i++) {
		int nx = pos.x + dx[i];
		int ny = pos.y + dy[i];
		xx[i] = nx + ny * size.x;
	}
	auto l2 = _map.size() - size.x;
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
		if (idx < 0 || _map[idx].type == -1 || idx == empty_pos || idx == size.x - 1 || idx == l2)
		{
			i--;
			continue;
		}
		_map[idx].type = -1;
	}
	if (_map[empty_pos].type != 0)
	{

	}
	get_mine_count();
	g_result = 0;
	return;
}

void minesweeper_cx::get_mine_count()
{
	// 计算周围雷数
	int dx[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
	int dy[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
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
			// 若未处理且为空白或数字,未标旗的 
			if (!pt.search && pt.type != -1 && pt.mark == 0) {
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


void minesweeper_cx::update(const glm::ivec2& pos0, double delta)
{
	auto pos1 = pos0;
	auto pos = pos0;
	pos.y += res->title_height;
	_pos = pos;
	if (/*g_result != 0 ||*/ texture == 0 || _map.empty())return;
	if (g_result == 0)
		tick += delta;
	_draw_data.clear();
	_draw_data.reserve(size.x * size.y * 2);
	// 背景

	// 遍历地图生成
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

	// 生成计数表情渲染数据
	{
		auto ct = std::to_string(mine_count);
		auto ct0 = std::to_string((int64_t)tick);
		auto n = ct.size();
		auto nt = mine_count - flag_count;
		// 旗子剩余数量 
		make_num(pos0, nt, glm::clamp((int)n, 3, mine_count));
		auto maxw = res->mwidth * size.x;
		auto ct00 = glm::clamp((int)ct0.size(), 3, 6);
		auto jsw = ct00 * res->num.z;
		auto qw = n * res->num.z;

		draw_data_t dd = {};
		dd.src = res->bq;
		dd.src.x += glm::clamp((int)g_result, 0, 2) * (res->mwidth + res->step);
		// 居中
		auto ce = (maxw - (res->mwidth + jsw + qw)) * 0.5;
		if (ce < qw)
		{
			ce = qw;
		}
		btn_pos = { pos1.x + ce + qw, pos1.y + (res->title_height - dd.src.w) * 0.5 };

		dd.dst = { btn_pos , dd.src.z,dd.src.w };
		dd.scale = 1.0f;
		// 表情
		_draw_data.push_back(dd);
		jsw = maxw - jsw;
		if (jsw < ce + res->mwidth)
		{
			jsw = ce + res->mwidth;
		}
		pos1.x += jsw;
		// 计时器
		make_num(pos1, tick, ct00);
	}
}

void minesweeper_cx::send_event(const glm::ivec2& opos, int btn)
{
	auto pos = opos - _pos;
	glm::vec2 ps1 = opos, bps = btn_pos;
	if (glm::distance(ps1, bps) < res->mwidth && btn == 0)
	{
		// 点中表情
		if (g_result != 0)
		{
			clear_map();
			g_result = -1;
		}
		return;
	}
	pos /= res->mwidth;
	if (pos.x < 0 || pos.x >= size.x || pos.y < 0 || pos.y >= size.y)
		return;
	if (g_result < 0)
	{
		init_map(pos);
	}
	if (g_result != 0)return;
	int idx = pos.x + pos.y * size.x;
	auto& pt = _map[idx];
	if (pt.status == 1 && btn != 2)
		return;
	if (btn == 0)//左打开
	{
		if (pt.mark > 0) {
			flag_count--; pt.mark = 0;
			return;
		}
		if (pt.type == -1)
		{
			over_mark_map(2);
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
			over_mark_map(1);
		}
	}
	else if (btn == 1)//右插旗
	{
		if (pt.mark == 0)
		{
			if (flag_count < mine_count)
			{
				flag_count++;
				pt.mark = 1;
			}
		}
		else
		{
			flag_count--; pt.mark = 0;
		}
		flag_count = glm::clamp(flag_count, 0, mine_count);
	}
	else if (btn == 2)// 打开8格
	{
		if (pt.status != 1 || pt.type < 1)return;
		int dx[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
		int dy[] = { -1, 0, 1, -1, 1, -1, 0, 1 };

		std::stack<glm::ivec2>& q = _stack_e;  // 队列存储待处理坐标 
		auto mine = _map.data();
		while (q.size()) {
			q.pop();
		}

		int cx = pos.x, cy = pos.y;
		// 遍历8个方向 
		int cc = 0;
		for (int i = 0; i < 8; i++) {
			int nx = cx + dx[i];
			int ny = cy + dy[i];
			if (nx < 0 || nx >= size.x || ny < 0 || ny >= size.y) continue;
			auto& p = mine[nx + ny * size.x];
			// 若未处理且为空白或数字,未标旗的 
			if (p.mark == 1 && p.status == 0) {
				cc++;
			}
		}
		if (cc < pt.type)
		{
			return;
		}
		for (int i = 0; i < 8; i++) {
			int nx = cx + dx[i];
			int ny = cy + dy[i];
			if (nx < 0 || nx >= size.x || ny < 0 || ny >= size.y) continue;
			auto& p = mine[nx + ny * size.x];
			// 若未处理且为空白或数字,未标旗的 
			if (p.mark == 0 && p.status == 0) {
				if (p.type == -1)
				{
					over_mark_map(2);
					p.status = 2;
					return;
				}
				p.status = 1;  // 显示数字或空白  
				if (p.type == 0) {  // 仅将空白格子加入队列 
					q.push({ nx, ny });
				}
			}
		}
		while (q.size()) {
			auto n = q.top(); q.pop();
			auto& p = mine[n.x + n.y * size.x];
			if (p.type == 0)
			{
				expand_blank(n.x, n.y);
			}
		}
		over_mark_map(1);
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

void minesweeper_cx::load_list()
{
	auto r = hz::read_json(savedir + "save_mw.json");
	if (r.is_object() && r.size() > 0)
	{
		int w = r["width"];
		int h = r["height"];
		float mc = r["mine_count"];
		resize(w, h, mc);
		clear_map();
	}
	else {
		resize(size.x, size.y, mcc);
		clear_map();
	}
}

void minesweeper_cx::load(int idx)
{
}

void minesweeper_cx::save()
{
	njson0 j;
	j["width"] = size.x;
	j["height"] = size.y;
	j["mine_count"] = mcc;
	hz::save_json(savedir + "save_mw.json", j, 2);
}

void minesweeper_cx::over_mark_map(int type)
{
	if (type == 1)
	{
		int count = 0;
		for (auto& it : _map) {
			if (it.type >= 0 && it.status == 0)
			{
				count++;
				break;
			}
		}
		if (count != 0)
		{
			return;
		}
	}
	g_result = type;
	for (auto& it : _map) {
		auto cs = it.status;
		it.status = 1;
		if (cs == 0 && it.type == -1 && type == 1)
		{
			it.status = 0;
			it.mark = 1;
		}
		if (it.mark == 1 && it.type != -1)
		{
			it.status = 3;
		}
	}
}

void minesweeper_cx::make_num(const glm::ivec2& pos, int num, int maxcount)
{
	if (num < 0)num = 0;
	auto ctn = std::to_string(num);
	if (ctn.size() < maxcount && maxcount > 0)
	{
		auto nn = maxcount - ctn.size();
		ctn.insert(ctn.begin(), nn, '0');
	}
	for (size_t i = 0; i < maxcount; i++)
	{
		draw_data_t dd = {};
		dd.src = res->num;
		dd.src.x += (ctn[i] - '0') * (dd.src.z + res->step);
		dd.dst = { pos.x + i * dd.src.z, pos.y, dd.src.z,dd.src.w };
		dd.scale = 1.0f;
		_draw_data.push_back(dd);
	}

}

int main()
{
	const char* wtitle = (char*)u8"多功能管理工具";
	auto tstr = hz::u8_to_gbk(wtitle);
	auto app = new_app();
	cpuinfo_t cpuinfo = get_cpuinfo();
	glm::ivec2 ws = { 1280,860 };
	// ef_vulkan ef_gpu|ef_resizable
	form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, (ef_transparent | ef_vulkan | ef_resizable));
	hz::audio_backend_t abc = { app->get_audio_device(),app_cx::new_audio_stream,app_cx::free_audio_stream,app_cx::bindaudio,app_cx::unbindaudio,app_cx::unbindaudios
		,app_cx::get_audio_stream_queued,app_cx::get_audio_stream_available,app_cx::get_audio_dst_framesize
		,app_cx::put_audio,app_cx::pause_audio,app_cx::mix_audio,app_cx::clear_audio,app_cx::sleep_ms,app_cx::get_ticks };
	auto audio_ctx = new hz::audio_cx();
	audio_ctx->init(&abc, "data/config_music.json");
	audio_ctx->run_thread();
	coders_t* cp = new_coders();
	auto d2 = new sp_drawable();
	d2->set_renderer(form0->renderer);
	//d2->add_pkg("temp/spineboy-j.spt", 0.25, 0.2);
	//d2->add_pkg("temp/spineboy-skel.spt", 0.25, 0.2);
	//d2->set_pos(0, 600, 650);
	//d2->set_pos(1, 300, 650); 
	texture_cb tex_cb = get_texture_cb();
#ifdef _DEBUG
	auto ltx = new layout_text_x();
	ltx->set_ctx(app->font_ctx);
	std::vector<std::string> efn;
	//auto fpv = app->font_ctx->add2file(R"(data\seguiemj.ttf)", &efn);
	std::string familys = (char*)u8"Consolas,新宋体,Segoe UI Emoji,Times New Roman,Malgun Gothic";
	ltx->add_familys(familys.c_str(), "");
	auto cache_tex = ltx->new_cache({ 1024,1024 });
	char* tb1 = (char*)u8"😊😎😭💣🚩❓❌🟦⬜➗";
	char* tb = (char*)u8"好";
	auto tbt = ltx->new_text_dta(0, 100, tb1, -1, 0);
	//auto tbt = ltx->new_text_dta1(fpv[0], 100, tb, -1, 0);// 使用单个字体
	SDL_Texture* text_tex = 0;
	if (tbt) {

		uint32_t kw = md::get_u8_idx(tb, 0);

		auto sue = app->font_ctx->get_font("Segoe UI Emoji", 0);
		auto sue1 = sue;
		auto gis = sue->get_glyph_index(kw, 0, 0);
		std::vector<vertex_f> vdp;
		auto vnn = sue->GetGlyphShapeTT(gis, &vdp);


		text_path_t op;
		auto pd = ltx->get_shape(0, 39, tb, &op, 0);
		path_v opt;
		opt._pos = { 100,-300 };
		text_path2path_v(pd, &opt);
		auto rc = opt.mkbox();
		opt._baseline = op.baseline;
		auto lcn = opt.get_line_count();
		save_png_v(&opt, 1, "temp/chuh.png", 1, 0.1);

		auto ft0 = cache_tex->_data.data();
		auto n = cache_tex->_data.size();
		for (size_t i = 0; i < n; i++)
		{
			auto p = ft0[i];
			save_img_png(p, "temp/font_test51.png");
			auto tex = tex_cb.make_tex(form0->renderer, p);
		}
		text_tex = (SDL_Texture*)ft0[0]->texid;
		//tbt->tv.clear();
	}
#endif
	auto minesweeper_tex = (SDL_Texture*)tex_cb.new_texture_file(form0->renderer, "data/mw2.png");
	if (minesweeper_tex)
	{
		minesweeper_cx* mscx = new minesweeper_cx();
		mscx->set_texture(minesweeper_tex);
		glm::ivec2 mcpos[] = { {6,0},{5,1},{3,2},{7,2},{8,2},{8,3},{0,4},{7,4},{5,5},{8,6} };
		//mscx->resize(9, 9, 10, mcpos);
		//mscx->resize(39, 24, 0.3);
		//mscx->clear_map();
		form0->add_event(mscx, [](uint32_t type, et_un_t* e, void* ud) {
			auto ptr = (minesweeper_cx*)ud;
			auto btn = e->v.b;
			if (type != (uint32_t)devent_type_e::mouse_button_e || btn->down)return;
			static int bns[] = { 0,0,2,1 };
			if (btn->button > 3)
			{
				return;
			}
			int bn = bns[btn->button];
			if (btn->clicks > 1) {
				bn = 2;
			}
			ptr->send_event({ btn->x,btn->y }, bn);
			});
		form0->render_cb = [=](SDL_Renderer* renderer, double delta)
			{
				static double deltas = 0;
				deltas += delta;

				glm::vec2 pos = { 20,20 };
				mscx->update(pos, delta);
				auto d = mscx->data();
				auto cnt = mscx->count();
				for (size_t i = 0; i < cnt; i++)
				{
					auto it = d[i];
					if (it.w9 > 0)
					{
						auto w = it.w9;
						SDL_RenderTexture9Grid(renderer, (SDL_Texture*)mscx->texture, (SDL_FRect*)&it.src, w, w, w, w, it.scale, (SDL_FRect*)&it.dst);
					}
					else if (it.scale > 1.0 || it.scale < 1.0)
					{
						SDL_RenderTextureTiled(renderer, (SDL_Texture*)mscx->texture, (SDL_FRect*)&it.src, it.scale, (SDL_FRect*)&it.dst);
					}
					else {
						SDL_RenderTexture(renderer, (SDL_Texture*)mscx->texture, (SDL_FRect*)&it.src, (SDL_FRect*)&it.dst);
					}
				}
				return;
#if 0


				SDL_SetRenderDrawColor(renderer, 0x42, 0x87, 0xf5, SDL_ALPHA_OPAQUE);  // light blue background.
				SDL_RenderClear(renderer);
				{
					pos.y += 100;
					static float angle = 0;
					angle += delta;
					if (angle > 1)
					{
						angle = 0;
					}
					auto it = tbt->tv[6];
					SDL_FRect src = { it._rect.x, it._rect.y, it._rect.z, it._rect.w };
					SDL_FRect rc = { pos.x + it._dwpos.x,pos.y + it._dwpos.y, it._rect.z, it._rect.w };
					SDL_FPoint center = { rc.w * 0.5,  rc.h * 0.5 };
					SDL_RenderTextureRotated(renderer, text_tex, &src, &rc, angle * 360, &center, SDL_FLIP_NONE);
					rc.w *= 3.9;
					rc.h *= 3.0;
					SDL_RenderTextureTiled(renderer, text_tex, &src, 1.0, &rc);

					auto it1 = tbt->tv[6];
					SDL_FRect src1 = { it1._rect.x, it1._rect.y, it1._rect.z, it1._rect.w };
					SDL_FRect rc1 = { pos.x + it._dwpos.x + 500,pos.y + it._dwpos.y, 550, 150 };
					int w = src1.w * 0.2;
					SDL_RenderTexture9GridTiled(renderer, text_tex, (SDL_FRect*)&src1, w, w, w, w, 1.0, (SDL_FRect*)&rc1, 1.0);
					rc1.y += 200;
					SDL_RenderTexture9Grid(renderer, text_tex, (SDL_FRect*)&src1, w, w, w, w, 1.0, (SDL_FRect*)&rc1);
				}
				// 渲染文本
				pos.y += 300;
				for (auto it : tbt->tv)
				{
					if (!it._image || !it._image->texid)continue;
					SDL_FRect src = { it._rect.x, it._rect.y, it._rect.z, it._rect.w };
					SDL_FRect rc = { pos.x + it._apos.x + it._dwpos.x,pos.y + it._apos.y + it._dwpos.y, it._rect.z, it._rect.w };
					SDL_RenderTexture(renderer, (SDL_Texture*)it._image->texid, &src, &rc);
				}
				return;
#endif
			};
	}
	// 运行消息循环
	run_app(app, 0);
	delete d2;
	delete audio_ctx;
	free_coders(cp);
	free_app(app);
	return 0;
}