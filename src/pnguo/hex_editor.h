#ifndef HEX_EDITOR_H
#define HEX_EDITOR_H


struct rect_select_x;


struct hex_style_t {
	cairo_t* cr = 0;		// 0不渲染
	text_style_t* st = 0;
	layout_text_x* ltx = 0;
	int64_t fl = 0;		// 行高
	int64_t pxx = 0;
	int64_t pyy = 0;
	glm::vec2 hex_size = { 900,600 };
	glm::ivec2 view_size = {};
	glm::vec4 bgrc = {};
	// 定义常量 
	const int MARGIN = 10;
	const int DATA_INSPECTOR_TITLE_WIDTH = 80;
	const int DECODED_TEXT_WIDTH = 150;
	const int RULER_DI_WIDTH = 200;
	uint32_t bg_fill = 0xff121212, bg_color = 0x80ffffff; double bg_r = 2; int bg_linewidth = 1;
};
// 16进制编辑
struct hex_editor
{
public:
	std::string ruler, line_number;	// 行号，标尺
	std::string data_hex;			// 数据hex
	std::string decoded_text;		// 解码文本
	std::string ruler_di;			// 数据检查器头
	std::string dititle;			// 数据检查器标题
	std::string data_inspector;		// 数据检查
	int line_height = 20;
	int char_width = 10;
	int line_number_n = 0;
	int bytes_per_line = 16;		// 第行显示字节数4-256
	int select_border = 2;			// 选中边框宽度
	int64_t acount = 0;				// 行数量 
	glm::i64vec2 range = {};		// 选中范围
	glm::vec4 text_rc[7] = {};		// 渲染区域
	uint32_t color[7] = {};			// 渲染颜色
	uint32_t select_color = 0xfff77d5a;
	float round_path = 0.2;			// 圆角选区
	dtext_cache dtc = {};
private:
	unsigned char* _data = 0;
	size_t _size = 0;
	int64_t count = 0;				// 当前显示行数量
	size_t line_offset = 0;			// 当前行的偏移
	glm::ivec2 view_size = { 600,1080 }; // 视图高 
	text_draw_t tdt = {};
private:
	glm::i64vec2 range2 = {};		// 选中范围,从小到大
	glm::i64vec2 range_c1 = {};		// 选中范围c
	int64_t ctpos = -1;
	rect_select_x* rsx = 0;
	std::string bpline;				// 缓存用
	std::vector<char> tempstr;		// 缓存用
	void* mapfile = 0;				// 映射文件
	bool is_update = true;
	bool _rdonly = false;
	bool d_update = false;
public:
	hex_editor();
	~hex_editor();
	// 设置行高、字宽
	void set_linechar(int height, int charwidth);
	// 设置范围计算选中矩形
	void set_range(int64_t r, int64_t r1);
	void set_view_size(const glm::ivec2& s);
	// 设置数据只显示
	bool set_data(const char* d, size_t len, bool is_copy);
	// 设置文件，是否只读打开
	bool set_file(const char* fn, bool is_rdonly);
	void save_data(size_t pos, size_t len);
	size_t write_data(const void* d, size_t len, size_t pos, bool save);	// 写入数据，是否保存到文件
	// 设置当前光标，计算data_inspector
	void set_pos(size_t pos);
	// 设置显示偏移
	void set_linepos(size_t pos);
	// 鼠标事件，0鼠标移动
	void on_mouse(int clicks, const glm::ivec2& mpos, int64_t hpos, int64_t vpos);
	std::string get_ruler_di();
	char* data();
	size_t size();
	bool update_hex_editor();
	text_draw_t* get_drawt();
	glm::i64vec2 get_range2();
	// 获取渲染大小
	glm::ivec2 get_draw_rect();
	bool get_draw_update();
	void draw_rc(cairo_t* cr);
	// 更新、使用cairo渲染
	void update_draw(hex_style_t* hst);
private:
	// 鼠标坐标转偏移
	int64_t get_mpos2offset(const glm::i64vec2& mpos);
	void make_rc();
};

/*
todo Data Inspector
binary(1)读一个字节
octal(1)
uint8(1)
int8(1)
uint16(2)
int16(2)
uint24(3)
int24(3)
uint32(4)
int32
uint64
int64
ULEB128
SLEB128
float16
bfloat16
float32
float64
ASCII
UTF-8读整个字符
UTF-16读整个字符
GB18030
BIG5
SHIFT-JIS
*/


#ifdef HEX_EDITOR_STATIC_LIB
#include "hex_editor.cpp"
#endif
#endif // !HEX_EDITOR_H
