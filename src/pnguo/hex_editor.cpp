
#include "pch1.h"
#include "hex_editor.h" 
#include "pnguo.h"

// 16进制编辑
#if 1

struct rect_select_x
{
	std::vector<glm::ivec4> rangerc;
	PathsD range_path = {};						// 圆角选区缓存
	path_v ptr_path = {};
};


hex_editor::hex_editor()
{
	auto phex = this;
	dititle = (char*)u8"数据检查器";
	phex->color[0] = 0xffff783b;	//0xfffb8f71;	// 标尺
	phex->color[1] = 0xff999999;	// 行号
	phex->color[2] = 0xffeeeeee;	// 16进制数据
	phex->color[3] = 0xffeeeeee;	// 解码文件
	phex->color[4] = 0xff999999;	// 数据检查器字段头
	phex->color[5] = -1;			// 数据检查器标题
	phex->color[6] = 0xff0cc616;	// 0xff107c10;	// 数据检查器相关信息 
}

hex_editor::~hex_editor()
{
	auto p = (hz::mfile_t*)mapfile;
	if (p)delete p;
	mapfile = 0;
	_data = 0;
	_size = 0;
	if (rsx)delete rsx; rsx = 0;
}

void hex_editor::set_linechar(int height, int charwidth)
{
	if (height > 0)
		line_height = height;
	if (charwidth > 0)
		char_width = charwidth;
}

void hex_editor::set_range(int64_t r, int64_t r1)
{
	range.x = r; range.y = r1;
	range2 = range;
	if (range.x > range.y)
	{
		std::swap(range2.x, range2.y);
	}
}

void hex_editor::set_view_size(const glm::ivec2& s)
{
	if (s.x > 0 && s.y > 0)
		view_size = s;
}

bool hex_editor::set_data(const char* d, size_t len, bool is_copy)
{
	if (d && len > 0)
	{
		if (is_copy) {
			tempstr.resize(len);
			memcpy(tempstr.data(), d, len);
			_data = (unsigned char*)tempstr.data();
		}
		else {
			_data = (unsigned char*)d;
		}
		_size = len;
		return true;
	}
	return false;
}

bool hex_editor::set_file(const char* fn, bool is_rdonly)
{
	_rdonly = is_rdonly;
	auto p = (hz::mfile_t*)mapfile;
	if (fn && *fn) {
		hz::mfile_t bk;
		auto bkt = bk.open_d(fn, is_rdonly);
		if (bkt) {

			if (p) {
				p->unmap();
				p->close_m();
			}
			else {
				p = new hz::mfile_t();
			}
			*p = bk;
			bk.clear_ptr(); // 转移句柄
			_data = (unsigned char*)bkt;
			_size = p->size();
			line_offset = 0;
			mapfile = p;
			is_update = true;
			return true;
		}
	}
	return false;
}

void hex_editor::save_data(size_t pos, size_t len)
{
	auto p = (hz::mfile_t*)mapfile;
	if (!_rdonly && p) {
		p->flush(pos, len);
	}
}

size_t hex_editor::write_data(const void* d, size_t len, size_t pos, bool save)
{
	size_t r = 0;
	auto p = (hz::mfile_t*)mapfile;
	if (p && !_rdonly)
	{
		p->seek(pos);
		r = p->write_m((char*)d, len, save);
	}
	else if (_data && _size > 0) {
		r = std::min(_size - pos, len);
		memcpy(_data + pos, d, r);
	}
	return r;
}
void printBinary(uint8_t num, std::string* t)
{
	char buf[16] = {};
	auto tt = buf;
	for (int i = sizeof(num) * 8 - 1; i >= 0; i--, tt++) {
		sprintf(tt, "%d", (num >> i) & 1);
	}
	if (t)
	{
		t->append(buf); t->push_back('\n');
	}
}
float bfloat16_to_float(uint16_t val)
{
	union {
		float f;
		unsigned int u;
	} result;

	// 将bfloat16扩展为float 
	result.u = ((unsigned int)val) << 16;
	return result.f;
}
// 读取 ULEB128 编码的无符号整数 
uint64_t read_uleb128(const char* data) {
	uint64_t value = 0;
	int shift = 0;
	unsigned char byte;

	do {
		byte = (unsigned char)*data;
		value |= (uint64_t)(byte & 0x7F) << shift;
		shift += 7;
		data++;
	} while (byte & 0x80);

	return value;
}

// 读取 SLEB128 编码的有符号整数 
int64_t read_sleb128(const char* data) {
	long long value = 0;
	int shift = 0;
	unsigned char byte;

	do {
		byte = (unsigned char)*data;
		value |= (int64_t)(byte & 0x7F) << shift;
		shift += 7;
		data++;
	} while (byte & 0x80);

	// 如果符号扩展需要 
	if ((shift < 64) && (byte & 0x40)) {
		value |= ((int64_t)(-1) << shift);
	}

	return value;
}
int is_gb18030_char(const uint8_t* data, size_t len) {
	if (data == NULL || len < 1) return -1; // 无效输入 
	// 单字节检查 
	if (data[0] <= 0x7F) {
		return 1; // 合法ASCII字符 
	}

	// 双字节检查 
	if (data[0] >= 0x81 && data[0] <= 0xFE) {
		if (len < 2) return -1; // 长度不足 
		uint8_t b2 = data[1];
		if ((b2 >= 0x40 && b2 <= 0x7E) || (b2 >= 0x80 && b2 <= 0xFE)) {
			return 2; // 合法双字节 
		}
	}
	// 四字节检查（需至少4字节长度）
	if (len >= 4 && data[1] >= 0x30 && data[1] <= 0x39) {
		if (data[2] >= 0x81 && data[2] <= 0xFE &&
			data[3] >= 0x30 && data[3] <= 0x39) {
			return 4; // 合法四字节 
		}
	}
	return -2; // 非法编码 
}
void print_data_de(const void* p, std::string* t)
{
	std::string str;
	char buf[256] = {};

	// binary
	printBinary(*(uint8_t*)p, t);
	// 8进制octal(1) uint8(1) int8(1)
	uint8_t num8 = *(uint8_t*)p;
	int8_t num8i = *(int8_t*)p;
	uint16_t num16u = *(uint16_t*)p;
	int16_t num16 = *(int16_t*)p;
	uint32_t num32u = *(uint32_t*)p;
	int32_t num32 = *(int32_t*)p;
	uint64_t num64u = *(uint64_t*)p;
	int64_t num64 = *(int64_t*)p;
	sprintf(buf, "%o\n%u\n%d\n", num8, num8, (int8_t)num8);
	str += buf;
	sprintf(buf, "%u\n%d\n", num16u, num16);
	str += buf;

	uint32_t num24 = 0;
	memcpy(&num24, p, 3);
	sprintf(buf, "%u\n%d\n", num24, (int32_t)num24);
	str += buf;

	sprintf(buf, "%u\n%d\n", num32u, num32);
	str += buf;
	sprintf(buf, "%llu\n%lld\n", num64u, num64);
	str += buf;

	auto ul = read_uleb128((char*)p);
	auto sl = read_sleb128((char*)p);
	sprintf(buf, "%llu\n%lld\n", ul, sl);
	str += buf;

	//float16
	double f16 = glm::detail::toFloat32(num16u);
	double bf16 = bfloat16_to_float(num16u);
	sprintf(buf, "%lg\n%lg\n", f16, bf16);
	str += buf;
	double f32 = *(float*)p;
	double f64 = *(double*)p;

	sprintf(buf, "%lg\n%lg\n", f32, f64);
	str += buf;
	// ascii
	char c = num8;
	sprintf(buf, "%c\n", (c >= 32 && c <= 126) ? c : ' ');
	str += buf;
	// 获取utf8
	auto s = (const char*)p;
	auto ss = md::utf8_next_char(s);
	str.append(s, ss); str.push_back('\n');
	// utf16
	{
		auto us16 = md::u16_u8((uint16_t*)p, 2);
		if (us16.size())
		{
			str += us16 + "\n";
		}
		else { str += " \n"; }
	}
	//gb
	{
		auto nc = is_gb18030_char((uint8_t*)p, 4);
		if (nc > 0)
		{
			auto g = md::gb_u8((char*)p, nc);
			if (g.size() > 2)
			{
				str += g + "\n";
			}
		}
		else {
			str += " \n";
		}
	}
	//big5
	{
		auto g = hz::big5_to_u8((char*)p, 2);
		if (g.size() > 2)
		{
			str += g + "\n";
		}
		else { str += " \n"; }
	}
	//shift_jis
	//{
	//	auto g = hz::shift_jis_to_u8((char*)p, 2);
	//	if (g.size() > 2)
	//	{
	//		str += g + "\n";
	//	}
	//	else { str += ".\n"; }
	//}


	if (t)
	{
		t->append(str);
	}
}
void hex_editor::set_pos(size_t pos)
{
	if (pos < _size) {
		bool isr = ctpos == pos;
		ctpos = pos;
		range.x = range.y = pos;
		if (isr)return;
		auto p = _data + pos;
		data_inspector.clear();
		auto di = &data_inspector;
		print_data_de(p, di);
		d_update = true;
	}
}
void hex_editor::input_data(void* data, size_t len)
{
	len = std::min(len, _size - ctpos);
	memcpy(_data + ctpos, data, len);
	set_pos(ctpos + len);
}
void hex_editor::set_linepos(size_t pos)
{
	if (pos < _size && line_offset != pos * bytes_per_line) {
		line_offset = pos * bytes_per_line; is_update = true;
	}
}

glm::i64vec2 get_dv2(int64_t x, int64_t y)
{
	int remainder = x % y;
	if (remainder < 0) remainder += y;
	int prev = x - remainder;
	int next = prev + y;
	return { prev, next };
}
void hex_editor::on_mouse(int clicks, const glm::ivec2& mpos, int64_t hpos, int64_t vpos)
{
	if (!_data || !_size)return;
	glm::i64vec2 nmpos = mpos;
	nmpos.x += hpos;
	nmpos.y += vpos;
	auto ost = get_mpos2offset(nmpos);
	if (clicks > 0)
	{
		set_pos(ost);
		printf("%lld\n", ost);
	}
	else {
		range.y = ost;
	}
	make_rc();
	return;
}

std::string hex_editor::get_ruler_di()
{
	static std::string s = "binary\noctal\nuint8\nint8\nuint16\nint16\nuint24\nint24\nuint32\nint32\nuint64\nint64\nULEB128\nSLEB128\nfloat16\nbfloat16\nfloat32\nfloat64\nASCII\nUTF-8\nUTF-16\nGB18030\nBIG5\n";// SHIFT - JIS\n";
	return s;
}

char* hex_editor::data()
{
	return (char*)_data;
}

size_t hex_editor::size()
{
	return _size;
}
bool hex_editor::update_hex_editor()
{
	int64_t nsize = _size - line_offset;
	if (nsize > 0 && _data && _size > 0)
	{

	}
	else {
		return false;
	}
	auto file_data = this;
	if (file_data->bytes_per_line < 4)
		file_data->bytes_per_line = 4;
	if (file_data->bytes_per_line > 256)
		file_data->bytes_per_line = 256;
	int bps = file_data->bytes_per_line * 5 + 16;
	if (bps < 128)bps = 128;
	if (file_data->bpline.size() != bps)
		file_data->bpline.resize(bps);
	char* line = file_data->bpline.data();
	int nc = view_size.y / file_data->line_height;
	int anc = _size / file_data->bytes_per_line;
	anc += _size % file_data->bytes_per_line > 0 ? 1 : 0;
	acount = anc;
	int dnc = nsize / file_data->bytes_per_line;
	dnc += nsize % file_data->bytes_per_line > 0 ? 1 : 0;
	int newnc = std::min(nc + 1, dnc);
	int idx = _size > UINT32_MAX ? 1 : 0;// 大于4G则用16位数

	if (count != newnc || file_data->is_update)
	{
		count = newnc;
		is_update = true;
	}
	if (is_update)
	{
		d_update = true;
		is_update = false;
		line_number.clear();
		data_hex.clear();
		decoded_text.clear();
		if (bytes_per_line * 3 != ruler.size())
		{
			ruler.clear();
			for (size_t j = 0; j < file_data->bytes_per_line; j++) {
				snprintf(line, bps, "%02zx ", j);
				ruler += line;
			}
		}
		auto lpos = line_offset;
		const char* fmt[2] = { "%08zx: \n" ,"%016zx: \n" };
		auto data = file_data->_data + line_offset;
		int lnw = 0;
		for (size_t i = 0; i < nsize && newnc > 0; i += file_data->bytes_per_line, newnc--) {
			snprintf(line, bps, fmt[idx], i + lpos);
			line_number += line;
			if (lnw == 0)
				lnw = line_number.size();
			line[0] = 0;
			for (int j = 0; j < file_data->bytes_per_line; j++) {
				if (i + j < nsize) {
					snprintf(line + strlen(line), bps - strlen(line), "%02x ", data[i + j]);
				}
				else {
					snprintf(line + strlen(line), bps - strlen(line), "   ");
				}
			}
			snprintf(line + strlen(line), bps - strlen(line), " ");
			data_hex += line;
			data_hex.push_back('\n');
			line[0] = 0;
			for (int j = 0; j < file_data->bytes_per_line; j++) {
				if (i + j < nsize) {
					char c = data[i + j];
					snprintf(line + strlen(line), bps - strlen(line), "%c", (c >= 32 && c <= 126) ? c : '.');
				}
				else {
					snprintf(line + strlen(line), bps - strlen(line), " ");
				}
			}
			decoded_text += line;
			decoded_text.push_back('\n');
		}
		line_number_n = lnw;
	}
	return true;
}
text_draw_t* hex_editor::get_drawt()
{
	if (ruler_di.empty())
		ruler_di = get_ruler_di();
	tdt.text = &ruler;
	tdt.text_rc = text_rc;
	tdt.color = color;
	tdt.count = 7;
	return &tdt;
}

glm::i64vec2 hex_editor::get_range2()
{
	range2 = range;
	if (range.x > range.y)
	{
		std::swap(range2.x, range2.y);
	}
	return range2;
}

glm::ivec2 hex_editor::get_draw_rect()
{
	glm::ivec2 r = {};//最大坐标
	glm::ivec2 pos = {};// 最小坐标
	for (size_t i = 0; i < 7; i++)
	{
		glm::ivec4 it = text_rc[i];
	}
	for (size_t i = 0; i < 7; i++)
	{
		glm::ivec4 it = text_rc[i];
		pos.x = std::min(pos.x, it.x);
		pos.y = std::min(pos.y, it.y);
		r.x = std::max(r.x, it.z + it.x);
		r.y = std::max(r.y, it.w + it.y);
	}
	return r - pos;
}

void hex_editor::draw_rc(cairo_t* cr)
{
	if (!rsx || rsx->rangerc.empty())return;
	set_color(cr, select_color);
	if (round_path > 0)
	{
		if (rsx->range_path.size() && rsx->range_path[0].size() > 3) {
			draw_polyline(cr, &rsx->range_path, true);
			cairo_fill(cr);
		}
	}
	else {
		for (auto& it : rsx->rangerc)
		{
			draw_rectangle(cr, it, std::min(it.z, it.w) * 0.18);
			cairo_fill(cr);
		}
	}
}

bool hex_editor::get_draw_update()
{
	bool r = d_update;
	d_update = false;
	return r;
}

int64_t hex_editor::get_mpos2offset(const glm::i64vec2& mpos)
{
	auto x = (mpos.x / char_width);
	auto y = (mpos.y / line_height);
	x /= 3;					// 3个字符
	// 裁剪到范围
	x = glm::clamp(x, (int64_t)0, (int64_t)bytes_per_line - 1);
	y *= bytes_per_line;
	return x + y;
}

void hex_editor::make_rc()
{
	int64_t x = -select_border;
	int64_t y = 0;
	int64_t w2 = select_border * 2;

	get_range2();
	if (range_c1 == range2)return;
	d_update = true;
	range_c1 = range2;
	auto f = get_dv2(range2.x, bytes_per_line);
	auto s = get_dv2(range2.y, bytes_per_line);
	// 生成选中背景矩形
	if (line_height > 0 && char_width > 0) {
		if (!rsx)
		{
			rsx = new rect_select_x();
		}
		std::vector<glm::ivec4> rss;
		auto hf = f.x / bytes_per_line;
		auto hs = s.x / bytes_per_line;
		auto line_no = _size / bytes_per_line;
		int64_t cw = char_width * 3;
		if (range2.x == range2.y) {
			rss.push_back({ x + (range2.x - f.x) * cw , y + hf * line_height,w2 + char_width * 2, line_height });
		}
		else
		{
			int y2 = range2.y + 1;	// 补一格
			if (f.y == s.y)
			{
				rss.push_back({ x + (range2.x - f.x) * cw , y + hf * line_height, w2 + (y2 - range2.x) * cw - char_width, line_height });// 同一行时
			}
			else {
				rss.push_back({ x + (range2.x - f.x) * cw, y + hf * line_height, w2 + (bytes_per_line - (range2.x - f.x)) * cw - char_width, line_height });// 第一行
			}
			for (int i = hf + 1; i < line_no && i < hs; i++)
			{
				rss.push_back({ x + 0, y + i * line_height, w2 + bytes_per_line * cw - char_width, line_height });// 中间全行
			}
			if (hf < hs)
			{
				rss.push_back({ x + 0, y + hs * line_height, w2 + (y2 - s.x) * cw - char_width, line_height });//最后一行
			}
		}
		rsx->rangerc = rss;
		if (round_path > 0)
		{
			PathsD subjects;
			PathD a;

			int py = 0;
			for (size_t i = 0; i < rss.size(); i++)
			{
				auto& it = rss[i];
				if (it.z < 1)
					it.z = cw;
				a.push_back({ it.x,it.y + py });
				a.push_back({ it.x + it.z,it.y + py });
				a.push_back({ it.x + it.z,it.y + it.w + py });
				a.push_back({ it.x,it.y + it.w + py });
				subjects.push_back(a);
				a.clear();
			}
			subjects = Union(subjects, FillRule::NonZero, 6);
			//range_path = InflatePaths(subjects, 0.5, JoinType::Round, EndType::Polygon);
			auto sn = subjects.size();
			if (sn > 0)
			{
				path_v& ptr = rsx->ptr_path;
				ptr._data.clear();
				for (size_t i = 0; i < sn; i++)
				{
					auto& it = subjects[i];
					ptr.add_lines((glm::dvec2*)it.data(), it.size(), false);
				}
				round_path = glm::clamp(round_path, 0.0f, 0.5f);
				rsx->range_path = gp::path_round(&ptr, -1, char_width * round_path, 16, 0, 0);
			}
			else { rsx->range_path.clear(); }
		}

	}
	//printf("fs\t%lld\t%lld\n", f, s);
}

void hex_editor::update_draw(hex_style_t* hst)
{
	if (!hst || !hst->st || !hst->ltx)return;
	cairo_t* cr = hst->cr;
	auto fl = hst->fl;
	auto pxx = hst->pxx;
	auto pyy = hst->pyy;
	auto vps = pyy / fl;
	pyy = vps * fl;
	set_linepos(vps);
	update_hex_editor();
	// 主绘制代码 
	auto nn = line_number_n;
	int cw = hst->st->font_size * 0.5;
	int line_number_width = nn * cw;
	int data_width = bytes_per_line * cw * 3;
	auto height = hst->view_size.y;
	// 设置各个区域的绘制位置 
	int ruler_w = ruler.size() * cw;
	text_rc[0] = { hst->MARGIN + line_number_width, hst->MARGIN, ruler_w, height }; // ruler 
	text_rc[1] = { hst->MARGIN, hst->MARGIN + fl ,line_number_width, height }; // line_number 
	text_rc[2] = { hst->MARGIN + line_number_width,hst->MARGIN + fl, data_width, height }; // data_hex 
	int dataps = hst->MARGIN + line_number_width + data_width + fl;
	text_rc[3] = { dataps,hst->MARGIN + fl ,hst->DECODED_TEXT_WIDTH, height }; // decoded_text 
	dataps += bytes_per_line * cw + fl;
	text_rc[4] = { dataps,hst->MARGIN + fl,hst->RULER_DI_WIDTH, height }; // ruler_di 
	text_rc[5] = { dataps,hst->MARGIN ,hst->RULER_DI_WIDTH, height }; // dititle 
	dataps += hst->DATA_INSPECTOR_TITLE_WIDTH;
	text_rc[6] = { dataps, hst->MARGIN + fl, hst->RULER_DI_WIDTH, height }; // data_inspector 
	if (!cr)return;
	text_draw_t* dt = get_drawt();
	dt->cr = cr; dt->ltx = hst->ltx;
	dt->st = hst->st;
	glm::vec4 chs = { 0,0,hst->hex_size };
	draw_rect(cr, hst->bgrc, hst->bg_fill, hst->bg_color, hst->bg_r, hst->bg_linewidth);
	clip_cr(cr, chs);
	cairo_translate(cr, -pxx, 0);
	{
		auto nrc = get_draw_rect();
		dtc.reset(cr, nrc);
		dt->cr = dtc.cr;
		if (get_draw_update()) {
			dtc.clear_color(0);
			auto ncr = dtc.cr;
			{
				cairo_as _ss_(ncr);
				auto clip4 = text_rc[2];
				auto sbx = select_border * 2;
				clip4.x -= sbx;
				clip4.z += sbx * 2;
				clip_cr(ncr, clip4);
				cairo_translate(ncr, text_rc[2].x, text_rc[2].y - pyy);
				draw_rc(ncr);
			}
			draw_draw_texts(dt);
		}
		glm::vec4 nnrc = { 0,0,nrc };
		if (dtc.surface)
			draw_image(cr, dtc.surface, {}, nnrc);
	}
}


#endif
//!hex_editor