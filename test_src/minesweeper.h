#pragma once
struct res_a;
class minesweeper_cx
{
public:
	struct node_t
	{
		char type = 0;		 // 0-8数字，-1雷
		char status = 0;	 // 状态，0未翻开、1已翻开、2爆、3叉
		char mark = 0;		 // 0无，1旗帜、2问号、3叹号
		bool search = false; // 是否搜索过
	};
	struct draw_data_t
	{
		glm::vec4 src, dst;
		float scale = 1.0f;
		float w9 = 0;// 9Grid
	};
	// 9*9、16*16、16*30、
	// 10、40、99
	glm::ivec2 size = { 39,24 };	// 地图大小
	int mine_count = 10;		// 雷数
	float mcc = 0.25;
	int flag_count = 0;		    // 标记的旗帜数
	double tick = 0;			// 计时器
	char g_result = -1;			// 游戏结果，0未结束、1胜利、2失败
	std::vector<node_t> _map;	// 地图数据
	void* texture = 0;			// 纹理
	std::vector<draw_data_t> _draw_data;	// 渲染数据
	std::stack<glm::ivec2> _stack_e;
	glm::ivec2 _pos = { };	// 渲染坐标
	glm::ivec2 btn_pos = { };	// 表情按钮坐标
	res_a* res = 0;	// 资源管理器
	std::string savedir = "data/";
public:
	minesweeper_cx();
	~minesweeper_cx();
	// 设置纹理
	void set_texture(void* tex);
	// 设置地图大小、雷数
	void resize(int w, int h, float mc);
	// 指定雷坐标初始
	void resize(int w, int h, float mc, glm::ivec2* mc_pos);
	// 清除地图
	void clear_map();
	// 设置地图数据，输入鼠标坐标，保证第一次点击为空
	void init_map(const glm::ivec2& pos);

	// 更新渲染
	void update(const glm::ivec2& pos, double delta);
	// 发送事件，btn=0打开、1插旗、2打开周围8格
	void send_event(const glm::ivec2& pos, int btn);

	draw_data_t* data();
	size_t count();
	// 列出存档
	void load_list();
	// 加载列表指定的存档
	void load(int idx);
	// 保存
	void save();
private:
	// 计算雷数量
	void get_mine_count();
	bool expand_blank(int x, int y);
	void over_mark_map(int type);
	void make_num(const glm::ivec2& pos, int num, int maxcount);
};