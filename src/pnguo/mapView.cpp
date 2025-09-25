
#define MAPVIEW
#ifdef _WIN32
#include <WinSock2.h>
#include <windows.h>
#include <shlwapi.h>
#include <Shlobj.h>
#else
#include <unistd.h>  
#include <sys/mman.h>
#include <sys/types.h>  
#include <sys/stat.h> 
#include <fcntl.h>
#endif
#include <string>
#include <string.h>

#include <io.h>


//#define MAPVIEW
#ifdef _WIN32
#ifndef _WINDOWS_ 
#include <sysinfoapi.h> 
#endif
#else
#include <unistd.h>  
#include <sys/mman.h>
#include <sys/types.h>  
#include <sys/stat.h> 
#include <fcntl.h>
#endif


#include <stdio.h>
#ifdef _WIN32
#include <shlwapi.h>
#include <sys/timeb.h>
#include <direct.h>
#include <io.h>
#define mkdir(a, b) _mkdir(a)
#define mkdirw(a, b) _wmkdir(a)
#else
#include <sys/stat.h>
#include <sys/inotify.h>
#endif
#if __has_include(<glib.h>)
#include <glib.h>
#endif

#include "pch1.h"
#include "mapView.h"

#ifdef _WIN32
#define fseeki64 _fseeki64
#define ftelli64 _ftelli64
#else			
#define fseeki64 fseeko64
#define ftelli64 ftello64
#endif // _WIN32


#ifndef NO_MV_ICU 
#include <unicode/uchar.h>
#include <unicode/ucnv.h>
#include <unicode/utypes.h>
#include <unicode/ucsdet.h>
#include <unicode/unistr.h>
#include <unicode/ustring.h>
#include <unicode/ubidi.h> 
#include <unicode/uscript.h>  
#endif



namespace md {

	int64_t file_size(FILE* fp)
	{
		int64_t size = 0;
		fseeki64(fp, 0L, SEEK_END);
		size = ftelli64(fp);
		fseeki64(fp, 0L, SEEK_SET);
		return size;
	}
	bool isNum(const std::string& str)
	{
		std::stringstream sin_(str);
		double d;
		int64_t i64;
		char c;
		if (!(sin_ >> i64))
		{
			return false;
		}
		if (!(sin_ >> d))
		{
			return false;
		}
		if (sin_ >> c)
		{
			return false;
		}
		return true;
	}
	void split(std::string str, const std::string& pattern, std::vector<std::string>& result)
	{
		std::string::size_type pos;
		str += pattern;//扩展字符串以方便操作
		int size = str.size();
		result.clear();
		int ct = 0;
		for (int i = 0; i < size; i++)
		{
			pos = str.find(pattern, i);
			if (pos < size)
			{
				std::string s = str.substr(i, pos - i);
				result.push_back(s);
				i = pos + pattern.size() - 1;
				ct++;
			}
		}
	}

	std::vector<std::string> split(const std::string& str, const std::string& pattern)
	{
		std::vector<std::string> vs;
		split(str, pattern, vs);
		return vs;
	}
	// 多分割符
	std::vector<std::string> split_m(const std::string& str, const std::string& pattern, bool is_space)
	{
		std::vector<std::string> vs;
		std::string tem;
		for (auto ch : str)
		{
			if (pattern.find(ch) == -1 || pattern.empty())
			{
				tem.push_back(ch);
			}
			else
			{
				if (tem == "" && !is_space)
					continue;
				vs.push_back(tem);
				tem = "";
			}
		}
		if (tem != "")
		{
			vs.push_back(tem);
		}
		return vs;
	}
	void get_lines(const std::string& str, std::function<void(const char* str)> cb)
	{
		auto lss = split_m(str, "\r\n", false);
		if (cb)
		{
			for (auto& it : lss) {
				cb(it.c_str());
			}
		}
	}

	template<typename Str>
	Str& replace_all(Str& str, const Str& old_value, const Str& new_value)
	{
		uint64_t pos(0);
		while (true) {
			if ((pos = str.find(old_value, pos)) != Str::npos)
			{
				str.replace(pos, old_value.length(), new_value); pos += new_value.length();
			}
			else
				break;
		}
		return str;
	}

	std::string replace_s(const std::string& str, const std::string& old_value, const std::string& new_value)
	{
		auto nstr = str;
		replace_all(nstr, old_value, new_value);
		return nstr;
	}

	bool validate_u8(const char* str, int len)
	{
		if (len < 0)len = strlen(str);
#ifdef __G_LIB_H__
		return len > 0 ? g_utf8_validate(str, len, 0) : false;
#endif
		return false;
	}
	// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
	// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
#ifndef FONS_UTF8_ACCEPT
#define FONS_UTF8_ACCEPT 0
#define FONS_UTF8_REJECT 12
#endif // FONS_UTF8_ACCEPT

	uint32_t fons_decutf8(uint32_t* down, uint32_t* codep, uint32_t byte)
	{
		static const unsigned char utf8d[] = {
			// The first part of the table maps bytes to character classes that
			// to reduce the size of the transition table and create bitmasks.
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
			7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
			8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
			10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 11, 6, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,

			// The second part is a transition table that maps a combination
			// of a state of the automaton and a character class to a state.
			0, 12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
			12, 0, 12, 12, 12, 12, 12, 0, 12, 0, 12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12,
			12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12,
			12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12,
			12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
		};

		uint32_t type = utf8d[byte];

		*codep = (*down != FONS_UTF8_ACCEPT) ?
			(byte & 0x3fu) | (*codep << 6) :
			(0xff >> type) & (byte);

		*down = utf8d[256 + *down + type];
		return *down;
	}

	uint32_t get_u8_idx(const char* str, int64_t idx)
	{
		uint32_t utf8state = 0;
		uint32_t codepoint = 0;
		str = utf8_char_pos(str, idx, -1);
		for (; str && *str; str++) {
			if (fons_decutf8(&utf8state, &codepoint, *((const unsigned char*)str)))
				continue;
			break;
		}
		return codepoint;
	}

	const char* get_u8_last(const char* str, uint32_t* codepoint)
	{
		uint32_t utf8state = 0;
		auto t = str;
		for (; str && *str; ) {
			if (fons_decutf8(&utf8state, codepoint, *((const unsigned char*)str++)))
				continue;
			break;
		}
		return utf8_next_char(t);
	}
	// 将Unicode字符转换为UTF-8字节序列
	void unicode_to_utf8(char* utf8, uint32_t unicode) {
		if (unicode < 0x80) {
			utf8[0] = unicode;
			utf8[1] = 0;
		}
		else if (unicode < 0x800) {
			utf8[0] = 0xC0 | (unicode >> 6);
			utf8[1] = 0x80 | (unicode & 0x3F);
			utf8[2] = 0;
		}
		else if (unicode < 0x10000) {
			utf8[0] = 0xE0 | (unicode >> 12);
			utf8[1] = 0x80 | ((unicode >> 6) & 0x3F);
			utf8[2] = 0x80 | (unicode & 0x3F);
			utf8[3] = 0;
		}
		else {
			utf8[0] = 0xF0 | (unicode >> 18);
			utf8[1] = 0x80 | ((unicode >> 12) & 0x3F);
			utf8[2] = 0x80 | ((unicode >> 6) & 0x3F);
			utf8[3] = 0x80 | (unicode & 0x3F);
			utf8[4] = 0;
		}
	}
	// 将UTF - 8编码转换为Unicode码点 
	int utf8_to_unicode(const char* str, int* unicode)
	{
		auto utf8 = (const unsigned char*)str;
		if (*utf8 == '\0') return 0;

		if ((*utf8 & 0x80) == 0) {
			// 单字节UTF - 8字符 
			*unicode = *utf8;
			return 1;
		}
		else if ((*utf8 & 0xE0) == 0xC0) {
			// 双字节UTF - 8字符 
			if ((*(utf8 + 1) & 0xC0) != 0x80) return -1;
			*unicode = ((utf8[0] & 0x1F) << 6) | (utf8[1] & 0x3F);
			return 2;
		}
		else if ((*utf8 & 0xF0) == 0xE0) {
			// 三字节UTF - 8字符 
			if ((*(utf8 + 1) & 0xC0) != 0x80 || (*(utf8 + 2) & 0xC0) != 0x80) return -1;
			*unicode = ((utf8[0] & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) | (utf8[2] & 0x3F);
			return 3;
		}
		else if ((*utf8 & 0xF8) == 0xF0) {
			// 四字节UTF - 8字符 
			if ((*(utf8 + 1) & 0xC0) != 0x80 || (*(utf8 + 2) & 0xC0) != 0x80 || (*(utf8 + 3) & 0xC0) != 0x80) return -1;
			*unicode = ((utf8[0] & 0x07) << 18) | ((utf8[1] & 0x3F) << 12) | ((utf8[2] & 0x3F) << 6) | (utf8[3] & 0x3F);
			return 4;
		}

		return -1;
	}

	// 处理UTF - 8字符串转换为Unicode数组 
	int utf8_string_to_unicode(const char* utf8, int* unicode_array, int max_count) {
		int count = 0;
		while (*utf8 != '\0' && count < max_count) {
			int unicode;
			int bytes = utf8_to_unicode(utf8, &unicode);
			if (bytes < 0) {
				return -1;
			}
			unicode_array[count++] = unicode;
			utf8 += bytes;
		}
		return count;
	}

	std::wstring u8_u16(const std::string& str)
	{
		return u8_w(str.c_str(), str.size());
	}
	std::wstring u8_w(const char* str, size_t len)
	{
		std::wstring wt;
		auto t = str;
		for (; t && *t && len > 0; len--)
		{
			int unicode = 0;
			int bytes = utf8_to_unicode(t, &unicode);
			if (bytes > 0)
				t += bytes;
			else
				break;
			//t = md::get_u8_last(t, &codepoint);
			if (unicode)
			{
				wt.push_back(unicode);
			}
			else { break; }
		}
		return wt;
	}
	std::string gb_u8(const char* str, size_t len)
	{
		return hz::gb_to_u8(str, len);
	}
	std::string u16_u8(uint16_t* str, size_t len)
	{
		char utf8_str[8] = {};
		std::string r;
		auto t = str;
		for (size_t i = 0; t && *t && i < len; ++i, t++) {
			unicode_to_utf8(utf8_str, *t);
			if (utf8_str[0])
			{
				r += utf8_str; utf8_str[0] = 0;
			}
			else { break; }
		}
		return r;
	}



#if 1

#ifndef UTF8_FIRST
#define UTF8_ASCII(b) (((unsigned char)(b)>=0x00)&&((unsigned char)(b)<=0x7F))
#define UTF8_FIRST(b) (((unsigned char)(b)>=0xC0)&&((unsigned char)(b)<=0xFD))
#define UTF8_OTHER(b) (((unsigned char)(b)>=0x80)&&((unsigned char)(b)<=0xBF))
#endif // UTF8_FIRST
	//根据首字节,获取utf8字符后续所占字节数
	int get_utf8_last_num(unsigned char ch)
	{
		static unsigned char t[] = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00,
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
			0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0, 0 };
		return t[ch];
	}
	int64_t get_utf8_count(const char* buffer, int64_t len)
	{
		const char* p = 0, * pend = buffer + len;
		int64_t count = 0;
		if (!buffer || len <= 0)
		{
			return 0;
		}
		for (p = buffer; p < pend; p++)
		{
			if (UTF8_ASCII(*p) || (UTF8_FIRST(*p)))
			{
				count++;
				//p += get_utf8_last_num(*p);
			}
		}
		return count;
	}
	// 获取一个utf8字符串开始位置
	const char* get_utf8_first(const char* str)
	{
		const char* p = str;
		for (; *p && !(UTF8_ASCII(*p) || (UTF8_FIRST(*p))); p++);
		return p;
	}
	// 获取前一个utf8位置
	const char* get_utf8_prev(const char* str)
	{
		const char* p = str;
		for (; *p && !(UTF8_ASCII(*p) || (UTF8_FIRST(*p))); p--);
		return p;
	}
	// 获取第n个字符的位置
	const char* utf8_char_pos(const char* buffer, int64_t pos, uint64_t len)
	{
		const char* p = 0, * pend = (len == -1 ? (char*)len : buffer + len);
		int64_t count = 0;
		if (!buffer || len == 0)
		{
			return 0;
		}
		for (p = buffer; *p && p < pend; p++)
		{
			if (UTF8_ASCII(*p) || (UTF8_FIRST(*p)))
			{
				count++;
				if (count > pos)
				{
					break;
				}
			}
		}
		return p;
	}
	const char* utf8_prev_char(const char* p)
	{
		auto p1 = get_utf8_prev(p);
		while (p && *p)
		{
			p--;
			if ((*p & 0xc0) != 0x80)
				break;
		}
		return p;
	}
#define mUNICODE_VALID(Char) ((Char) < 0x110000 && (((Char) & 0xFFFFF800) != 0xD800))

	static const char utf8_skip_data[256] = {
	  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
	};
	const char* utf8_next_char(const char* p) {
		return (char*)((p)+utf8_skip_data[*(const unsigned char*)(p)]);
	}
	// 返回字符偏移
	int utf8_pointer_to_offset(const char* str, const char* pos)
	{
		const char* s = str;
		int offset = 0;

		if (pos < str)
			offset = -utf8_pointer_to_offset(pos, str);
		else
			while (s < pos)
			{
				s = md::utf8_next_char(s);
				offset++;
			}

		return offset;
	}
	// 返回字节偏移
	char* utf8_offset_to_pointer(const char* str, int offset)
	{
		const char* s = str;

		if (offset > 0)
			while (offset--)
				s = md::utf8_next_char(s);
		else
		{
			const char* s1;

			/* This nice technique for fast backwards stepping
			 * through a UTF-8 string was dubbed "stutter stepping"
			 * by its inventor, Larry Ewing.
			 */
			while (offset)
			{
				s1 = s;
				s += offset;
				while ((*s & 0xc0) == 0x80)
					s--;

				offset += utf8_pointer_to_offset(s, s1);
			}
		}

		return (char*)s;
	}

#endif // 1

	// 去掉头尾空格
	std::string trim(const std::string& str, const char* pch)
	{
		auto s = str;
		if (s.empty())
		{
			return s;
		}
		s.erase(0, s.find_first_not_of(pch));
		s.erase(s.find_last_not_of(pch) + 1);
		return s;
	}
	std::string trim_ch(const std::string& str, const std::string& pch)
	{
		std::string r = str;
		if (pch.size() && str.size())
		{
			size_t p1 = 0, p2 = str.size();
			for (int i = 0; i < str.size(); i++)
			{
				auto ch = str[i];
				if (pch.find(ch) == std::string::npos)
				{
					p1 = i;
					break;
				}
			}
			for (int i = str.size() - 1; i > 0; i--)
			{
				auto ch = str[i];
				if (pch.find(ch) == std::string::npos)
				{
					p2 = i + 1;
					break;
				}
			}
			r = str.substr(p1, (p2 - p1));
		}
		return r;
	}

}
//!md

//---------------------------------------------------------------------------------------------
/*
使用方法
MapView mv;
mv.openfile(str.c_str());			//打开文件
char *buf = (char*)mv.mapview();	//获取映射内容
if (buf)
{
guiSetStr(_edit_cmdout, buf);
}

*/
//---------------------------------------------------------------------------------------------
#ifndef NO_MAPFILE
namespace hz
{
	// type 0全部，1=只有文件file,2=只有文件夹folder
	std::vector<std::string> listFiles(const std::string& path, bool isRecursive, int type)
	{
		std::vector<std::string> r;
		listFiles(path.c_str(), isRecursive, type, &r);
		return r;
	}

	size_t listFiles(const char* path, bool isRecursive, int type, std::vector<std::string>* p)
	{
		size_t rs = 0;
		if (!p)return rs;
#ifdef _WIN32
		auto& r = *p;
		auto ps = r.size();
		std::stack<std::string> q;
		char searchPath[1024];
		q.push(path);
		for (; q.size();)
		{
			struct _finddata_t fileInfo;
			auto qf = q.top(); q.pop();
			if (qf.empty())continue;
			sprintf(searchPath, "%s\\*.*", qf.c_str());  // 构造搜索路径：原路径 + \*.*			
			auto handle = _findfirst(searchPath, &fileInfo);  // 开始查找
			if (handle == -1) {
				perror("Failed to open directory");
				continue;
			}
			do {
				if (strcmp(fileInfo.name, ".") != 0 && strcmp(fileInfo.name, "..") != 0) {
					auto nn = qf + "\\" + fileInfo.name;
					switch (type) {
					case 0:
					{
						r.push_back(nn);
					}
					break;
					case 1:
					{
						if (fileInfo.attrib & _A_ARCH)r.push_back(nn);
					}
					break;
					case 2:
					{
						if (fileInfo.attrib & _A_SUBDIR)r.push_back(nn);
					}
					break;
					}
					// 递归遍历子目录（可选）
					if ((fileInfo.attrib & _A_SUBDIR) && isRecursive) {
						q.push(nn);
					}
				}
			} while (_findnext(handle, &fileInfo) == 0);  // 继续查找下一个
			_findclose(handle);  // 关闭搜索 
		}
		rs = r.size() - ps;
#endif
		return rs;
	}

	std::string get_temp_path()
	{
		std::string str;
#ifdef _WIN32
		str.resize(MAX_PATH * 2);
		auto n = GetTempPathA(str.size(), str.data());
		if (n > 0)
			str.resize(n);
		else
			str.clear();
#else
		char* path = getenv("HOME");
		if (path)str = path;
#endif
		return str;
	}
	std::string replace(std::string strBase, const std::string& strSrc, const std::string& strDes)
	{
		std::string::size_type pos = 0;
		std::string::size_type srcLen = strSrc.size();
		std::string::size_type desLen = strDes.size();
		pos = strBase.find(strSrc, pos);
		while ((pos != std::string::npos))
		{
			strBase.replace(pos, srcLen, strDes);
			pos = strBase.find(strSrc, (pos + desLen));
		}
		return strBase;
	}
	std::wstring replace(std::wstring strBase, const std::wstring& strSrc, const std::wstring& strDes)
	{
		std::wstring::size_type pos = 0;
		std::wstring::size_type srcLen = strSrc.size();
		std::wstring::size_type desLen = strDes.size();
		pos = strBase.find(strSrc, pos);
		while ((pos != std::wstring::npos))
		{
			strBase.replace(pos, srcLen, strDes);
			pos = strBase.find(strSrc, (pos + desLen));
		}
		return strBase;
	}

	enum PathE
	{
		pathdrive = 0x01,
		pathdir = 0x02,
		pathfname = 0x04,
		pathext = 0x08,
		path_all = pathdrive | pathdir,
		path_noext = pathdrive | pathdir | pathfname,
	};

	void check_make_path(const std::string& filename, unsigned int mod/* = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH*/)
	{
		char* file_name = (char*)filename.c_str();
		char* t = file_name;
		char chr = '/';
		char* ch = strchr(t, '\\');
		if (ch)
		{
			chr = *ch;
		}

		for (; t && *t; t++)
		{
			for (; !(*t == '/' || *t == '\\') && *t; t++);
			if (*t)
			{
				chr = *t;
				*t = 0;
			}
			else
			{
				break;
			}
			if (access(file_name, 0) != -1)
			{
				if (t)
					*t = chr;
				continue;
			}
#ifdef _WIN32
			mkdir(file_name, mod);
#else
#ifndef __ANDROID__
			mkdir(file_name, mod);
#endif
#endif
			if (t)
				*t = chr;
		}
	}
	void check_make_path(const std::wstring& filename, unsigned int mod/* = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH*/)
	{
		wchar_t* file_name = (wchar_t*)filename.c_str();
		wchar_t* t = file_name;
		wchar_t chr = L'/';
		wchar_t* ch = wcschr(t, '\\');
		if (ch)
		{
			chr = *ch;
		}

		for (; t && *t; t++)
		{
			for (; !(*t == '/' || *t == '\\') && *t; t++);
			if (*t)
			{
				chr = *t;
				*t = 0;
			}
			else
			{
				break;
			}
			if (_waccess(file_name, 0) != -1)
			{
				if (t)
					*t = chr;
				continue;
			}
#ifdef _WIN32
			mkdirw(file_name, mod);
#else
#ifndef __ANDROID__
			mkdir(file_name, mod);
#endif
#endif
			if (t)
				*t = chr;
		}
	}
	std::string genfn(std::string fn)
	{
#ifdef _WIN32
		fn = replace(fn, "/", "\\");
		fn = replace(fn, "\\\\", "\\");
#else
		fn = replace(fn, "\\", "/");
		fn = replace(fn, "//", "/");
#endif // _WNI32
		check_make_path(fn, pathdrive | pathdir);
		return fn;
	}
	std::wstring genfn(std::wstring fn)
	{
#ifdef _WIN32
		fn = replace(fn, L"/", L"\\");
		fn = replace(fn, L"\\\\", L"\\");
#else
		fn = replace(fn, "\\", "/");
		fn = replace(fn, "//", "/");
#endif // _WNI32
		check_make_path(fn, pathdrive | pathdir);
		return fn;
	}
	mfile_t::mfile_t()
	{
	}

	mfile_t::~mfile_t()
	{
		unmap();
		close_m();
	}
#ifdef _WIN32

	bool mfile_t::open_m(const std::string& fn, bool is_rdonly, bool is_shared, bool is_async)
	{
		bool ret = true;
		std::wstring fnn;
		std::string fnna;
		// 验证路径是否为utf8
		if (md::validate_u8(fn.c_str(), fn.size()))
		{
			fnn = u8_to_u16(fn);
			fnn = genfn(fnn);
		}
		else
		{
			fnna = genfn(fn);
		}
		_is_rdonly = is_rdonly;
		int dwcd = OPEN_EXISTING;
		_flags_o = GENERIC_READ;
		if (!is_rdonly)
		{
			_flags_o |= GENERIC_WRITE;
			dwcd = OPEN_ALWAYS;
		}
		_flags = FILE_SHARE_READ;
		if (is_shared)
		{
			_flags |= FILE_SHARE_WRITE;
		}
		//打开磁盘上的文件得到句柄   
		if (fnna.size())
		{
			hFile = CreateFileA(fnna.c_str(), _flags_o, _flags, NULL, dwcd, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		}
		else if (fnn.size())
		{
			hFile = CreateFileW(fnn.c_str(), _flags_o, _flags, NULL, dwcd, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		}
		//判断文件是否创建成功 
		if (hFile == INVALID_HANDLE_VALUE)
		{
			hFile = 0;
			_errstr = "openFile error: " + getLastError();
			ret = false;
		}
		else
		{
			_fsize = get_file_size();
			set_end(_fsize);
		}

		return ret;
	}

	int mfile_t::ftruncate_m(int64_t rs)
	{
		int ret = 0;
		if (hFile && rs > 0)
		{
			auto oldpm = _pm;
			unmap(true);	// 关闭映射才能重新设置大小
			LARGE_INTEGER ds;
			ds.QuadPart = rs;
			ret = SetFilePointerEx(hFile, ds, NULL, FILE_BEGIN);
			if (ret)
			{
				ret = SetEndOfFile(hFile);
			}
			if (!ret)
				_errstr = "ftruncate_m error: " + getLastError();

			auto m = map(rs, 0);
			if (m)
			{
				set(_pm, rs);
				seek(0, SEEK_SET);
			}
		}
		return ret;
	}
	char* mfile_t::map(uint64_t ms, int64_t pos)
	{
		_msize = ms;
		if (!_pm && ms)
		{
			if (!hMapFile && pos == 0)
			{
				// 创建File mapping 
				hMapFile = CreateFileMapping(hFile, // 需要映射的文件的句柄 
					NULL, // 安全选项：默认 
					_is_rdonly ? PAGE_READONLY : PAGE_READWRITE, // 可读，可写 
					(DWORD)(ms >> 32), // mapping对象的大小，高位
					(DWORD)(ms & 0xFFFFFFFF),  // mapping对象的大小，低位 
					NULL); // mapping对象的名字 
				if (hMapFile == NULL)
				{
					_errstr = "CreateFileMapping error: " + getLastError();
				}
			}
			if (hMapFile)
			{
				_pm = (char*)MapViewOfFile(hMapFile, // mapping对象的句柄 
					_is_rdonly ? SECTION_MAP_READ : FILE_MAP_ALL_ACCESS, // 可读，可写 
					(DWORD)(pos >> 32), // 映射的文件偏移，高32位
					(DWORD)(pos & 0xFFFFFFFF),// 映射的文件偏移，低32位 
					ms); // 映射到View的数据大小 
				set(_pm, ms + pos);
				seek(0, SEEK_SET);
			}
		}
		return _pm;
	}
	int mfile_t::flush(size_t pos, size_t size)
	{
		int ret = 0;
		if (_pm && _msize)
		{
			auto p = _pm + pos;
			if (size - pos > _msize)
			{
				size = _msize;
			}
			ret = FlushViewOfFile(p, size);
		}
		return ret;
	}
	int mfile_t::unmap(bool isclose)
	{
		int ret = 0;
		if (_pm && hMapFile)
		{
			ret = UnmapViewOfFile(_pm);
			if (isclose)
			{
				// 关闭mapping对象 
				if (!CloseHandle(hMapFile))
				{
					_errstr = "CloseMapping error: " + getLastError();
				}
				hMapFile = nullptr;
			}
			set(nullptr, 0);
			_pm = nullptr;
			_msize = 0;
		}
		return ret;
	}
	void mfile_t::close_m()
	{
		// 关闭mapping对象 
		if (hMapFile && !CloseHandle(hMapFile))
		{
			_errstr = "CloseMapping error: " + getLastError();
		}
		if (hFile && !CloseHandle(hFile))
		{
			_errstr = "CloseFile error: " + getLastError();
		}
		_fsize = 0;
	}

	uint64_t mfile_t::get_file_size()
	{
		uint64_t ret = 0;
		if (hFile)
		{
#if 0
			unsigned long dwsize = 0, dwheihgt = 0;
			dwsize = GetFileSize(hFile, &dwheihgt);
			ret = dwheihgt;
			ret << 32;
			ret |= dwsize;
#else
			LARGE_INTEGER ds = {};
			GetFileSizeEx(hFile, &ds);
			ret = ds.QuadPart;
#endif // 0
		}
		return ret;
	}


#else
	bool mfile_t::open_m(const std::string& fnn, bool is_rdonly, bool is_shared, bool is_async)
	{
		bool ret = true;
		std::string fn;
		// 验证路径是否为utf8
		if (!md::validate_u8(fnn.c_str(), fnn.size()))
		{
			fn = gbk_to_u8(fnn);
		}
		fn = genfn(fn);
		_is_rdonly = is_rdonly;

		_flags_o = is_async ? O_ASYNC : O_SYNC;
		if (is_rdonly)
		{
			_flags_o |= O_RDONLY;
			_fd = open(fn.c_str(), _flags_o);
		}
		else
		{
			_flags_o |= O_RDWR;
			_fd = open(fn.c_str(), _flags_o);
			if (_fd == -1)
			{
				_flags_o |= O_CREAT;
				_fd = open(fn.c_str(), _flags_o, 00666);
			}
		}

		if (_fd == -1)
		{
			ret = false;
		}
		else
		{
			_fsize = get_file_size();
			set_end(_fsize);
		}
		_fn = fn;
		_prot = _is_rdonly ? PROT_READ : PROT_READ | PROT_WRITE;
		_flags = is_shared ? MAP_SHARED : MAP_PRIVATE;
		_flags_fl = is_shared ? MS_ASYNC : MS_SYNC;
		return ret;
	}
	int mfile_t::ftruncate_m(int64_t rs)
	{
		int ret = 0;
		if (_fd && rs > 0)
		{
			bool ismap = _pm;
			unmap(true);
			ret = ftruncate64(_fd, rs);
			lseek(_fd, 0L, SEEK_SET);
			if (ismap)
			{
				auto m = map(rs, 0);
				if (m)
				{
					set(_pm, rs);
					seek(0, SEEK_SET);
				}
			}
		}
		return ret;
	}
	char* mfile_t::map(uint64_t ms, int64_t pos)
	{
		_msize = ms;
		if (!_pm && ms > 0)
		{
			_pm = (char*)mmap64(0, ms, _prot, _flags, _fd, pos);

			//auto er = errno;
			set(_pm, ms + pos);
			seek(0, SEEK_SET);
		}
		return _pm;
	}
	int  mfile_t::flush(size_t pos, size_t size)
	{
		int ret = 0;
		if (_pm)
		{
			auto p = _pm + pos;
			if (size - pos > _msize)
			{
				size = _msize;
			}
			_msize = pos + size;
			ret = msync(p, size, _flags_fl);
		}
		return ret;
	}
	int mfile_t::unmap(bool isclose)
	{
		int ret = 0;
		if (_pm && _msize)
			ret = munmap(_pm, _msize);
		set(nullptr, 0);
		_pm = nullptr;
		_msize = 0;
		return ret;
	}

	void mfile_t::close_m()
	{
		if (_fd)
			close(_fd);
		_fd = -1;
	}


	uint64_t mfile_t::get_file_size()
	{
		uint64_t ret = 0;
		if (_fd)
		{
			ret = lseek64(_fd, 0L, SEEK_END);
			lseek(_fd, 0L, SEEK_SET);
		}
		return ret;
	}


#endif // _WIN32


	size_t mfile_t::write_m(const char* data, size_t len, bool up)
	{
		const char* my = nullptr;
		size_t ret = 0;
		if (_pm)
		{
			ret = write(data, len);
			//my = (char*)memcpy(_pm, data, len);
			//seek(len, SEEK_CUR);
			if (up)
			{
				flush();
			}
			//ret = len;
		}
		return ret;
	}

	uint64_t mfile_t::get_size()
	{
		return _fsize;
	}

	char* mfile_t::open_d(const std::string& fn, bool is_rdonly)
	{
		if (open_m(fn, is_rdonly))
		{
			return map(size(), 0);
		}
		return nullptr;
	}
	// 打开或创建一个文件，并修改大小，0则不改变大小，文件不存在需要重新ftruncate_m和map
	char* mfile_t::new_m(const std::string& fn, size_t size1)
	{
		open_d(fn, false);
		if (size1 == 0)
			size1 = size();
		else
			ftruncate_m(size1);
		return map(size1, 0);
	}

	void mfile_t::clear_ptr()
	{
#ifdef _WIN32
		hFile = nullptr; // 文件的句柄 
		hMapFile = nullptr; // 文件内存映射区域的句柄 
#else
		_fd = 0;
#endif // _WIN32
		_pm = 0;
	}

	std::string mfile_t::getLastError()
	{
		std::string str;
#ifdef _WIN32

		char* buf = 0;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf, 0, NULL);
		if (buf)
			str = buf;
		LocalFree(buf);
#else
#ifdef errno
		str = strerror(errno);
#endif
#endif // _WIN32
		return str;
	}



#ifndef NO_BF 
#ifdef _WIN32
	static int CALLBACK _SHBrowseForFolderCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
	{
		switch (uMsg)
		{
		case BFFM_INITIALIZED:
			::SendMessageW(hwnd, BFFM_SETSELECTION, TRUE, lpData);   //传递默认打开路径 break;  
		case BFFM_SELCHANGED:    //选择路径变化，BFFM_SELCHANGED  
		{
			wchar_t curr[MAX_PATH];
			SHGetPathFromIDListW((LPCITEMIDLIST)lParam, (LPWSTR)curr);
			::SendMessageW(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)&curr[0]);
		}
		break;
		default:
			break;
		}
		return 0;
	}
#endif
	int browse_folder(const std::string& strCurrentPath, std::function<void(const std::string&)> rfunc, const std::string& title)
	{
		auto r = browse_folder(strCurrentPath, title);
		if (rfunc)
		{
			rfunc(r);
		}
		return r.size() ? 0 : -1;
	}
	int browse_folder(const std::string& strCurrentPath, const std::string& title, std::function<void(const std::string&)> rfunc)
	{
		auto r = browse_folder(strCurrentPath, title);
		if (rfunc)
		{
			rfunc(r);
		}
		return r.size() ? 0 : -1;
	}
	std::string browse_folder(const std::string& strCurrentPath, const std::string& title)
	{
		std::string ret;
#ifdef _WIN32
		BROWSEINFO bi;
		std::string buf;
		buf.resize(MAX_PATH * 4);
		std::string szr;
		szr.resize(MAX_PATH * 4);
		auto Buffer = buf.data();
		auto szResult = szr.data();
		ITEMIDLIST* ppidl = 0;
		HWND hWnd = GetForegroundWindow();
		auto rh = SHGetSpecialFolderLocation(hWnd, CSIDL_DRIVES, &ppidl);
		if (ppidl == NULL)
			MessageBox(hWnd, "启动路径浏览失败", "提示", MB_OK);
		//初始化入口参数bi开始  
		bi.hwndOwner = hWnd;
		bi.pidlRoot = ppidl;//根目录  
		bi.pszDisplayName = Buffer;//此参数如为NULL则不能显示对话框  
		std::string dtitle;
		if (get_text_code(title.c_str(), title.size()) == "UTF-8") {
			dtitle = icu_u8_gbk(title.c_str(), title.size());
			bi.lpszTitle = dtitle.c_str();
		}
		else
		{
			bi.lpszTitle = title.c_str();//\0浏览文件夹";//下标题  
		}
		bi.ulFlags = //BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
			BIF_RETURNONLYFSDIRS | BIF_USENEWUI /*包含一个编辑框 用户可以手动填写路径 对话框可以调整大小之类的..*/ |
			BIF_UAHINT /*带TIPS提示*/
			//| BIF_NONEWFOLDERBUTTON /*不带新建文件夹按钮*/
			;
		bi.lpfn = _SHBrowseForFolderCallbackProc;
		//bi.iImage=IDR_MAINFRAME;  
		std::string ddir;
		if (get_text_code(strCurrentPath.c_str(), strCurrentPath.size()) == "UTF-8") {
			ddir = icu_u8_gbk(strCurrentPath.c_str(), strCurrentPath.size());
			bi.lParam = LPARAM(ddir.c_str());
		}
		else
		{
			bi.lParam = LPARAM(strCurrentPath.c_str());    //设置默认路径  
		}

		//初始化入口参数bi结束  
		LPITEMIDLIST pIDList = SHBrowseForFolder(&bi);//调用显示选择对话框  
		if (pIDList)
		{
			//取得文件夹路径到Buffer里  
			SHGetPathFromIDList(pIDList, szResult);
			//sFolderPath就是我们选择的路径  

			ret = szResult;
			//printf("Select path %s\n", szResult);
		}

		do {
			LPMALLOC lpMalloc;
			if (FAILED(SHGetMalloc(&lpMalloc)))
				break;
			lpMalloc->Free(pIDList);
			lpMalloc->Release();
		} while (0);
#endif
		return ret;
	}
	std::string browse_folder_w(const std::wstring& strCurrentPath, const std::wstring& title)
	{
		std::string ret;
#ifdef _WIN32
		BROWSEINFOW bi;
		std::wstring buf;
		buf.resize(MAX_PATH * 4);
		std::wstring szr;
		szr.resize(MAX_PATH * 4);
		auto Buffer = buf.data();
		auto szResult = szr.data();
		ITEMIDLIST* ppidl = 0;
		HWND hWnd = GetForegroundWindow();
		auto rh = SHGetSpecialFolderLocation(hWnd, CSIDL_DRIVES, &ppidl);
		if (ppidl == NULL)
			MessageBox(hWnd, "启动路径浏览失败", "提示", MB_OK);
		std::wstring wt = title;
		//初始化入口参数bi开始  
		bi.hwndOwner = hWnd;
		bi.pidlRoot = ppidl;//根目录  
		bi.pszDisplayName = Buffer;//此参数如为NULL则不能显示对话框  
		bi.lpszTitle = wt.c_str();//\0浏览文件夹";//下标题  
		bi.ulFlags = //BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
			BIF_RETURNONLYFSDIRS | BIF_USENEWUI /*包含一个编辑框 用户可以手动填写路径 对话框可以调整大小之类的..*/ |
			BIF_UAHINT /*带TIPS提示*/
			//| BIF_NONEWFOLDERBUTTON /*不带新建文件夹按钮*/
			;
		bi.lpfn = _SHBrowseForFolderCallbackProc;
		//bi.iImage=IDR_MAINFRAME;  
		bi.lParam = LPARAM(strCurrentPath.c_str());    //设置默认路径  

		//初始化入口参数bi结束  
		LPITEMIDLIST pIDList = SHBrowseForFolderW(&bi);//调用显示选择对话框  
		if (pIDList)
		{
			//取得文件夹路径到Buffer里  
			SHGetPathFromIDListW(pIDList, szResult);
			//sFolderPath就是我们选择的路径  

			ret = md::u16_u8((uint16_t*)szResult, -1);
			//printf("Select path %s\n", szResult);
		}

		do {
			LPMALLOC lpMalloc;
			if (FAILED(SHGetMalloc(&lpMalloc)))
				break;
			lpMalloc->Free(pIDList);
			lpMalloc->Release();
		} while (0);
#endif
		return ret;
	}

	std::string browse_save_file(const std::string& title, const std::string& strCurrentPath, std::string filter, void* hWnd)
	{
		std::string ret;

#ifdef _WIN32
		const int n = 10;
		OPENFILENAME opfn = {};
		//CHAR strFilename[MAX_PATH * 100];//存放文件名  
		std::vector<char> sfn;
		sfn.resize(MAX_PATH * n);
		char* strFilename = sfn.data();
		//初始化   
		opfn.lStructSize = sizeof(OPENFILENAME);//结构体大小  
		opfn.hwndOwner = (HWND)hWnd;

		for (auto& it : filter)
		{
			if (it == '\t')
				it = '\0';
		}
		//设置过滤  
		opfn.lpstrFilter = filter.size() ? filter.c_str() : "所有文件\0*.*\0\0文本文件\0*.txt\0";
		//默认过滤器索引设为1  
		opfn.nFilterIndex = 1;
		//文件名的字段必须先把第一个字符设为 \0  
		opfn.lpstrFile = strFilename;
		opfn.lpstrFile[0] = '\0';
		opfn.nMaxFile = sfn.size();// sizeof(strFilename);
		//设置标志位，检查目录或文件是否存在  
		opfn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;// OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;

		opfn.lpstrInitialDir = strCurrentPath.c_str();
		opfn.lpstrTitle = strCurrentPath.c_str();
		// 显示对话框让用户选择文件  
		if (GetSaveFileName(&opfn))
		{
			//在文本框中显示文件路径  
			//if (rfunc)
			{
				ret = strFilename;
			}
		}
#endif
		return ret;
	}

	std::string browse_save_file_u8(const std::string& title, const std::string& strCurrentPath, std::string filter, void* hWnd)
	{
		std::string ret;

#ifdef _WIN32
		const int n = 10;
		OPENFILENAMEW opfn = {};
		//CHAR strFilename[MAX_PATH * 100];//存放文件名  
		std::vector<wchar_t> sfn;
		sfn.resize(MAX_PATH * n);
		wchar_t* strFilename = sfn.data();
		//初始化   
		opfn.lStructSize = sizeof(OPENFILENAMEW);//结构体大小  
		opfn.hwndOwner = (HWND)hWnd;

		for (auto& it : filter)
		{
			if (it == '\t')
				it = '\0';
		}
		auto wt = md::u8_u16(title);
		auto fwt = md::u8_u16(filter);
		auto cp = md::u8_u16(strCurrentPath);
		//设置过滤  
		opfn.lpstrFilter = fwt.size() ? fwt.c_str() : L"所有文件\0*.*\0\0文本文件\0*.txt\0";
		//默认过滤器索引设为1  
		opfn.nFilterIndex = 1;
		//文件名的字段必须先把第一个字符设为 \0  
		opfn.lpstrFile = strFilename;
		opfn.lpstrFile[0] = '\0';
		opfn.nMaxFile = sfn.size();// sizeof(strFilename);
		//设置标志位，检查目录或文件是否存在  
		opfn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;// OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;

		opfn.lpstrInitialDir = cp.c_str();
		opfn.lpstrTitle = wt.c_str();
		// 显示对话框让用户选择文件  
		if (GetSaveFileNameW(&opfn))
		{
			//在文本框中显示文件路径  
			//if (rfunc)
			{
				ret = md::u16_u8((uint16_t*)strFilename, -1);
			}
		}
#endif
		return ret;
	}


	std::vector<std::string> browse_openfile(const std::string& title, const std::string& strCurrentPath, std::string filter, void* hWnd, bool multi_select)
	{
		std::vector<std::string> ret;

		for (auto& it : filter)
		{
			if (it == '\t')
				it = '\0';
		}
#ifdef _WIN32
		const int n = 10;
		OPENFILENAME opfn = {};
		//CHAR strFilename[MAX_PATH * 100];//存放文件名  
		std::vector<char> sfn;
		sfn.resize(MAX_PATH * n);
		auto strFilename = sfn.data();
		//初始化   
		opfn.lStructSize = sizeof(OPENFILENAMEW);//结构体大小  
		opfn.hwndOwner = (HWND)hWnd;

		//设置过滤  
		opfn.lpstrFilter = filter.size() ? filter.c_str() : "所有文件\0*.*\0\0文本文件\0*.txt\0";
		//默认过滤器索引设为1  
		opfn.nFilterIndex = 1;
		//文件名的字段必须先把第一个字符设为 \0  
		opfn.lpstrFile = strFilename;
		opfn.lpstrFile[0] = '\0';
		opfn.nMaxFile = sfn.size();// sizeof(strFilename);
		//设置标志位，检查目录或文件是否存在  
		opfn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
		if (multi_select)
		{
			opfn.Flags |= OFN_ALLOWMULTISELECT;
		}
		opfn.lpstrInitialDir = strCurrentPath.c_str();
		opfn.lpstrTitle = title.c_str();
		// 显示对话框让用户选择文件  
		if (GetOpenFileName(&opfn))
		{
			//在文本框中显示文件路径  
			//if (rfunc)
			{
				auto t = strFilename;
				std::string st;
				std::vector<std::string> sfn;
				for (; *t;)
				{
					st = t;
					t += st.size() + 1;
					sfn.push_back(st);
				}
				if (sfn.size() > 1)
				{
					std::string dirs = sfn[0] + "\\";
					std::vector<std::string> ts;
					ret.reserve(sfn.size());
					for (size_t i = 1; i < sfn.size(); i++)
					{
						ret.push_back(dirs + sfn[i]);
					}
				}
				else {
					ret = sfn;
				}
			}
		}
#endif
		return ret;
	}

	std::vector<std::string> browse_openfile_u8(const std::string& title, const std::string& strCurrentPath, std::string filter, void* hWnd, bool multi_select)
	{
		std::vector<std::string> ret;

		for (auto& it : filter)
		{
			if (it == '\t')
				it = '\0';
		}
#ifdef _WIN32
		const int n = 10;
		OPENFILENAMEW opfn = {};
		//CHAR strFilename[MAX_PATH * 100];//存放文件名  
		std::vector<wchar_t> sfn;
		sfn.resize(MAX_PATH * n);
		auto strFilename = sfn.data();
		//初始化   
		opfn.lStructSize = sizeof(OPENFILENAMEW);//结构体大小  
		opfn.hwndOwner = (HWND)hWnd;

		auto wt = md::u8_u16(title);
		auto fwt = md::u8_u16(filter);
		auto cp = md::u8_u16(strCurrentPath);
		//设置过滤  
		opfn.lpstrFilter = fwt.size() ? fwt.c_str() : L"所有文件\0*.*\0\0文本文件\0*.txt\0";
		//默认过滤器索引设为1  
		opfn.nFilterIndex = 1;
		//文件名的字段必须先把第一个字符设为 \0  
		opfn.lpstrFile = strFilename;
		opfn.lpstrFile[0] = '\0';
		opfn.nMaxFile = sfn.size();// sizeof(strFilename);
		//设置标志位，检查目录或文件是否存在  
		opfn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
		if (multi_select)
		{
			opfn.Flags |= OFN_ALLOWMULTISELECT;
		}
		opfn.lpstrInitialDir = cp.c_str();
		opfn.lpstrTitle = wt.c_str();
		// 显示对话框让用户选择文件  
		if (GetOpenFileNameW(&opfn))
		{
			//在文本框中显示文件路径  
			//if (rfunc)
			{
				auto t = strFilename;
				std::wstring st;
				std::vector<std::string> sfn;
				for (; *t;)
				{
					st = t;
					t += st.size() + 1;
					sfn.push_back(md::u16_u8((uint16_t*)st.c_str(), -1));
				}
				if (sfn.size() > 1)
				{
					std::string dirs = sfn[0] + "\\";
					std::vector<std::string> ts;
					ret.reserve(sfn.size());
					for (size_t i = 1; i < sfn.size(); i++)
					{
						ret.push_back(dirs + sfn[i]);
					}
				}
				else {
					ret = sfn;
				}
			}
		}
#endif
		return ret;
	}


#ifdef _WIN32
	BOOL OpenFolderAndSelectFile(LPSTR lpszFilePath)
	{
		//
		// GetFolder
		//
		DWORD dw = lstrlenA(lpszFilePath) - 1;
		for (; dw != -1; dw--)
		{
			if (lpszFilePath[dw] == '\\')
			{
				break;
			}
		}
		if (dw == -1)
		{
			return FALSE;
		}
		//
		// Get a pointer to the Desktop's IShellFolder interface.
		// 
		LPSHELLFOLDER pDesktopFolder;
		if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder)))
		{
			// 
			// IShellFolder::ParseDisplayName requires the file name be in
			// Unicode.
			// 
			OLECHAR oleStr[MAX_PATH];

			lpszFilePath[dw] = '\0';
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
				lpszFilePath, -1, oleStr, _countof(oleStr));
			// 
			// Convert the path to an ITEMIDLIST.
			// 
			LPITEMIDLIST     pidl;
			ULONG             chEaten;
			ULONG             dwAttributes;
			HRESULT             hr;

			hr = pDesktopFolder->ParseDisplayName(
				NULL, NULL, oleStr, &chEaten, &pidl, &dwAttributes);
			if (FAILED(hr))
			{
				pDesktopFolder->Release();
				return FALSE;
			}
			LPCITEMIDLIST pidlFolder = pidl;

			lpszFilePath[dw] = '\\';
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
				lpszFilePath, -1, oleStr, _countof(oleStr));

			hr = pDesktopFolder->ParseDisplayName(
				NULL, NULL, oleStr, &chEaten, &pidl, &dwAttributes);
			if (FAILED(hr))
			{
				pDesktopFolder->Release();
				return FALSE;
			}
			LPCITEMIDLIST pidlFile = pidl;

			CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
			hr = SHOpenFolderAndSelectItems(pidlFolder, 1, &pidlFile, 0);

			pDesktopFolder->Release();

			if (hr == S_OK)
			{
				return TRUE;
			}
		}
		return FALSE;
	}
#endif
	bool open_folder_select_file(std::string n)
	{
#ifdef _WIN32
		return n.size() ? OpenFolderAndSelectFile((LPSTR)n.c_str()) : false;
#endif
		return false;
	}
#define BrowseForFolder browse_folder

	int browse_openfile(const std::string& title, const std::string& strCurrentPath, std::string filter, void* hWnd
		, std::function<void(const std::vector<std::string>&)> rfunc, int n)
	{
#ifdef _WIN32
		for (auto& it : filter)
		{
			if (it == '\t')
				it = '\0';
		}
		OPENFILENAME opfn;
		//CHAR strFilename[MAX_PATH * 100];//存放文件名  
		std::vector<char> sfn;
		sfn.resize(MAX_PATH * n);
		CHAR* strFilename = sfn.data();
		//初始化  
		ZeroMemory(&opfn, sizeof(OPENFILENAME));
		opfn.lStructSize = sizeof(OPENFILENAME);//结构体大小  
		opfn.hwndOwner = (HWND)hWnd;

		//设置过滤  
		opfn.lpstrFilter = filter.size() ? filter.c_str() : "所有文件\0*.*\0\0文本文件\0*.txt\0";
		//默认过滤器索引设为1  
		opfn.nFilterIndex = 1;
		//文件名的字段必须先把第一个字符设为 \0  
		opfn.lpstrFile = strFilename;
		opfn.lpstrFile[0] = '\0';
		opfn.nMaxFile = sfn.size();// sizeof(strFilename);
		//设置标志位，检查目录或文件是否存在  
		opfn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT;
		opfn.lpstrInitialDir = strCurrentPath.c_str();
		opfn.lpstrTitle = title.c_str();
		// 显示对话框让用户选择文件  
		if (GetOpenFileName(&opfn))
		{
			//在文本框中显示文件路径  
			if (rfunc)
			{
				char* t = strFilename;
				std::string st;
				std::vector<std::string> sfn;
				for (; *t;)
				{
					st = t;
					t += st.size() + 1;
					sfn.push_back(st);
				}
				if (sfn.size() > 1)
				{
					std::string dirs = sfn[0] + "\\";
					std::vector<std::string> ts;
					for (size_t i = 1; i < sfn.size(); i++)
					{
						ts.push_back(dirs + sfn[i]);
					}
					sfn = ts;
				}
				rfunc(sfn);
			}
		}
#endif
		return 0;
	}

#endif

	std::string gbk_to_u8(const std::string& str)
	{
		std::string ret;
#ifdef __G_LIB_H__
		GError* error = NULL;
		gchar* utf8_string = 0;
		gchar* fstr = (gchar*)str.c_str();
		// 初始化GLib
		//g_type_init();
		// 将GBK转换为UTF-8
		utf8_string = g_convert(fstr, -1, "UTF-8", "GBK", 0, NULL, &error);
		if (error) {
			//g_warning((char*)"转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			//g_print("转换成功: %s\n", utf8_string);
			ret = utf8_string;
			g_free(utf8_string);
		}
#endif
		return ret;
	}
	std::string gb_to_u8(const char* str, size_t len)
	{
		std::string ret;
#ifdef __G_LIB_H__
		GError* error = NULL;
		gchar* utf8_string = 0;
		gchar* fstr = (gchar*)str;
		// 初始化GLib
		//g_type_init();
		// 将GBK转换为UTF-8
		utf8_string = g_convert(fstr, len, "UTF-8", "GB18030", 0, NULL, &error);
		if (error)
		{
			g_error_free(error); error = 0;
			utf8_string = g_convert(fstr, -1, "UTF-8", "GBK", 0, NULL, &error);
		}
		if (error) {
			//g_warning((char*)"转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			//g_print("转换成功: %s\n", utf8_string);
			ret = utf8_string;
			g_free(utf8_string);
		}
#endif
		return ret;
	}
	std::string big5_to_u8(const char* str, size_t len)
	{
		std::string ret;
#ifdef __G_LIB_H__
		GError* error = NULL;
		gchar* utf8_string = 0;
		gchar* fstr = (gchar*)str;
		// 初始化GLib
		//g_type_init();
		// 将GBK转换为UTF-8
		utf8_string = g_convert(fstr, len, "UTF-8", "BIG5", 0, NULL, &error);
		if (error) {
			//g_warning((char*)"转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			//g_print("转换成功: %s\n", utf8_string);
			ret = utf8_string;
			g_free(utf8_string);
		}
#endif
		return ret;
	}
	std::string shift_jis_to_u8(const char* str, size_t len)
	{
		std::string ret;
#ifdef __G_LIB_H__
		GError* error = NULL;
		gchar* utf8_string = 0;
		gchar* fstr = (gchar*)str;
		// 初始化GLib
		//g_type_init(); 
		utf8_string = g_convert(fstr, len, "UTF-8", "SHIFT-JIS", 0, NULL, &error);
		if (error) {
			//g_warning((char*)"转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			//g_print("转换成功: %s\n", utf8_string);
			ret = utf8_string;
			g_free(utf8_string);
		}
#endif
		return ret;
	}
	std::string u16_to_u8(const std::wstring& str)
	{
		std::string ret;
#ifdef __G_LIB_H__
		GError* error = NULL;
		gchar* utf8_string = 0;
		gchar* fstr = (gchar*)str.c_str();
		// 初始化GLib
		//g_type_init();
		// 将GBK转换为UTF-8
		gsize cs = str.size() * 2;
		utf8_string = g_convert(fstr, cs, "UTF-8", "UTF-16LE", 0, NULL, &error);
		if (error) {
			//g_warning((char*)"转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			//g_print("转换成功: %s\n", utf8_string);
			ret = utf8_string;
			g_free(utf8_string);
		}
#else
		ret = md::u16_u8((uint16_t*)str.c_str(), str.size());
#endif
		return ret;
	}
	std::string u16_to_gbk(const std::wstring& str)
	{
		std::string ret;
#ifdef __G_LIB_H__
		GError* error = NULL;
		gchar* utf8_string = 0;
		gchar* fstr = (gchar*)str.c_str();
		// 初始化GLib
		//g_type_init();
		// 将GBK转换为UTF-8
		gsize cs = str.size() * 2;
		utf8_string = g_convert(fstr, cs, "GBK", "UTF-16LE", 0, NULL, &error);
		if (error) {
			//g_warning((char*)"转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			//g_print("转换成功: %s\n", utf8_string);
			ret = utf8_string;
			g_free(utf8_string);
		}
#endif
		return ret;
	}
	std::string u8_to_gbk(const std::string& str)
	{
		std::string ret;
#ifdef __G_LIB_H__
		GError* error = NULL;
		gchar* utf8_string = 0;
		gchar* fstr = (gchar*)str.c_str();
		// 初始化GLib
		//g_type_init();
		// 将GBK转换为UTF-8
		utf8_string = g_convert(fstr, -1, "GBK", "UTF-8", 0, NULL, &error);
		if (error) {
			g_warning((char*)"转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			//g_print("转换成功: %s\n", utf8_string);
			ret = utf8_string;
			g_free(utf8_string);
		}
#endif
		return ret;
	}
	std::wstring u8_to_u16(const std::string& str)
	{
		std::wstring ret;
#ifdef __G_LIB_H__
		GError* error = NULL;
		gchar* tstr = 0;
		gchar* fstr = (gchar*)str.c_str();
		gsize cs = str.size();
		tstr = g_convert(fstr, cs, "UTF-16LE", "UTF-8", 0, NULL, &error);
		if (error) {
			g_warning((char*)"转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			//g_print((char*)u8"转换成功: %s\n", tstr);
			ret = (wchar_t*)tstr;
			g_free(tstr);
		}
#else
		ret = md::u8_u16(str);
#endif
		return ret;
	}
	std::wstring gbk_to_u16(const std::string& str)
	{
		std::wstring ret;
#ifdef __G_LIB_H__
		GError* error = NULL;
		gchar* tstr = 0;
		gchar* fstr = (gchar*)str.c_str();
		gsize cs = str.size();
		tstr = g_convert(fstr, cs, "UTF-16LE", "GBK", 0, NULL, &error);
		if (error) {
			g_warning((char*)"转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			//g_print("转换成功: %s\n", tstr);
			ret = (wchar_t*)tstr;
			g_free(tstr);
		}
#endif
		return ret;
	}
	// 跳过bom
	char* tbom(char* str, int* outn)
	{
		//UTF-8
		char utf8[] = { (char)0xEF ,(char)0xBB ,(char)0xBF };
		//UTF-16（大端序）
		char utf16d[] = { (char)0xFE ,(char)0xFF };
		//UTF-16（小端序）
		char utf16[] = { (char)0xFF ,(char)0xFE };
		//UTF-32（大端序）
		char utf32d[] = { (char)0x00 ,(char)0x00 ,(char)0xFE ,(char)0xFF };
		//UTF-32（小端序）
		char utf32[] = { (char)0xFF ,(char)0xFE ,(char)0x00 ,(char)0x00 };

		int bomlen[] = { 3,2,2,4,4 };
		char* bom[] = { utf8,utf16d,utf16,utf32d,utf32 };
		int u = -1;
		for (int i = 0; i < 5; ++i)
		{
			if (0 == memcmp(str, bom[i], bomlen[i]))
			{
				u = i;
			}
		}
		int ret = 0;
		if (u >= 0)
		{
			ret = bomlen[u];
			str += ret;
		}
		if (outn)
		{
			*outn = ret;
		}
		return str;

	}
	int boms(const char* str)
	{
		//UTF-8
		char utf8[] = { (char)0xEF ,(char)0xBB ,(char)0xBF };
		//UTF-16（大端序）
		char utf16d[] = { (char)0xFE ,(char)0xFF };
		//UTF-16（小端序）
		char utf16[] = { (char)0xFF ,(char)0xFE };
		//UTF-32（大端序）
		char utf32d[] = { (char)0x00 ,(char)0x00 ,(char)0xFE ,(char)0xFF };
		//UTF-32（小端序）
		char utf32[] = { (char)0xFF ,(char)0xFE ,(char)0x00 ,(char)0x00 };

		int bomlen[] = { 3,2,2,4,4 };
		char* bom[] = { utf8,utf16d,utf16,utf32d,utf32 };
		int u = -1;
		for (int i = 0; i < 5; ++i)
		{
			if (0 == memcmp(str, bom[i], bomlen[i]))
			{
				u = i;
			}
		}
		int ret = 0;
		if (u >= 0)
		{
			ret = bomlen[u];
		}
		return ret;
	}
	bool is_utf8(const char* str, int len)
	{
		while (*str && (len > 0 || len < 0)) {
			len--;
			if ((*str & 0x80) == 0x00) {
				str++; // 1-byte character, skip this byte
			}
			else if ((*str & 0xe0) == 0xc0) {
				if ((*(str + 1) & 0xc0) != 0x80) return false;
				str += 2; // 2-byte character, skip these two bytes
			}
			else if ((*str & 0xf0) == 0xe0) {
				if ((*(str + 1) & 0xc0) != 0x80 || (*(str + 2) & 0xc0) != 0x80) return false;
				str += 3; // 3-byte character, skip these three bytes
			}
			else if ((*str & 0xf8) == 0xf0) {
				if ((*(str + 1) & 0xc0) != 0x80 || (*(str + 2) & 0xc0) != 0x80 || (*(str + 3) & 0xc0) != 0x80) return false;
				str += 4; // 4-byte character, skip these four bytes
			}
			else {
				return false;   // invalid start byte
			}
		}
		return true;
	}
	inline bool isse(int ch)
	{
		return ch == 0 || (ch > 0 && isspace(ch));
	}
	inline static bool is_json(const char* str, size_t len)
	{
		bool ret = false;
		char* te = (char*)str + len - 1;
		char* t = (char*)str;
		if (str && len > 1)
		{
			do
			{
				while (isse(*t))
				{
					t++;
				}
				if (*t != '[' && *t != '{')
				{
					break;
				}
				while (isse(*te))
				{
					te--;
				}
				if ((*t == '[' && *te == ']') || (*t == '{' && *te == '}'))
				{
					ret = true;
				}
			} while (0);
		}
		return ret;
	}

	njson read_json(const std::string& fn)
	{
#ifdef _WIN32
		if (fn[1] != ':' && fn[0] != '/')
		{
			//fn = hz::File::getAP(fn);
		}
#endif
		mfile_t mt;
		auto vt = mt.open_d(fn, false);
		std::string v;
		v.assign(vt, mt.get_size());
		njson ret;
		if (v.size() && v != "null")
		{
			try
			{
				int bs = boms(v.data());
				if (hz::is_json(v.data() + bs, v.size() - bs))
				{
					ret = njson::parse(v.begin() + bs, v.end());
				}
				else
				{
					ret = njson::from_cbor(v.begin() + bs, v.end());
				}
			}
			catch (const std::exception& e)
			{
				printf("parse json:\t%s\n", e.what());
			}
		}
		return ret;
	}
	// 保存json到文件，-1则保存成cbor
	void save_json(const std::string& fn, const njson& n, int indent_cbor)
	{
#ifdef _WIN32
		if (fn[1] != ':' && fn[0] != '/')
		{
			//fn = hz::File::getAP(fn);
		}
#endif
		mfile_t mt;
		auto vt = mt.open_m(fn, false);
		if (vt)
		{
			if (indent_cbor < 0)
			{
				auto s = njson::to_cbor(n);
				if (s.size())
				{
					mt.ftruncate_m(s.size());
					auto d = mt.map(s.size(), 0);
					if (d)
						memcpy(d, (char*)s.data(), s.size());
					mt.flush();
				}
			}
			else
			{
				std::string s;
				if (indent_cbor <= 0)
				{
					s = n.dump();
				}
				else
				{
					s = n.dump(indent_cbor);
				}
				if (s.size())
				{
					mt.ftruncate_m(s.size());
					auto d = mt.map(s.size(), 0);
					if (d)
						memcpy(d, (char*)s.data(), s.size());
					mt.flush();
				}
			}
		}
	}
	std::string get_dir(const char* t)
	{
		std::string ret = t;
		char* t1 = (char*)ret.c_str() + ret.size();
		char tch = '/';
#ifdef _WIN32
		tch = '\\';
#endif // !_WIN32
		for (; *t1 != tch; t1--);
		t1++; *t1 = 0;

		return ret.c_str();
	}



	bool save_file(const std::string& fn0, const char* data, uint64_t size, uint64_t pos, bool is_plus)
	{
		uint64_t  retval;
		if (!data || !size)
		{
			return false;
		}
		auto filename = genfn(fn0);
		//LOGW(("read_binary_file0:" + filename).c_str());
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
		filename = [[[NSBundle mainBundle]resourcePath] stringByAppendingPathComponent:@(filename)] .UTF8String;
#endif 
		check_make_path(filename.c_str(), pathdrive | pathdir);
		const char* fn = filename.c_str();
		FILE* fp = fopen(filename.c_str(), is_plus ? "ab" : "wb");

		if (!fp && !is_plus)
		{
			mfile_t mv;
			bool ret = false;
			do {
				auto md = mv.new_m(fn, size);
				if (md && size > 0)
				{
					memcpy(md, data, size);
					ret = true;
				}
			} while (0);
			return ret;// "fail to open file: " + filename;
		}
		if (is_plus)
			fseeki64(fp, pos, SEEK_SET);
		retval = fwrite(data, size, 1, fp);
		assert(retval == 1);
		fclose(fp);
		return true;
	}

#if 1

	Shared::Shared() {}
	Shared::~Shared() {
		dll_close();
	}
	std::string Shared::toLower(const std::string& s)
	{
		std::string str;
		std::transform(s.begin(), s.end(), str.begin(), ::tolower);
		return str;
	}

	Shared* Shared::loadShared(const std::string& fnstr, std::vector<std::string>* pdir)
	{
#ifdef NDEBUG
		auto nd = "debug";
#else
		auto nd = "release";
#endif // NDEBUG
		Shared* ptr = new Shared();
		bool is = ptr->loadFile(fnstr);
		if (!is && pdir)
		{
			for (auto it : *pdir)
			{
				auto nit = toLower(it);
				if (nit.find(nd) != std::string::npos)
				{
					continue;
				}
				if ('/' != *it.rbegin() && '\\' != *it.rbegin())
				{
					it.push_back('/');
				}
				is = ptr->loadFile(it + fnstr);
				if (is)
				{
					break;
				}
			}
		}
		if (!is)
		{
			delete ptr;
			ptr = nullptr;
		}
		if (ptr) {

		}
		return ptr;
	}

	void Shared::destroy(Shared* p)
	{
		if (p)
			delete p;
	}

	bool Shared::loadFile(const std::string& fnstr)
	{
#ifdef _WIN32
#define _DL_OPEN(d) LoadLibraryExA(d,0,LOAD_WITH_ALTERED_SEARCH_PATH)
		const char* sysdirstr = nullptr;
		const char* sys64 = nullptr;
#else
#define _DL_OPEN(d) dlopen(d,RTLD_NOW)
		const char* sysdirstr = "/usr/local/lib/";
		const char* sys64 = "/system/lib64/";
#endif
		//std::call_once(oc, [=]() {
		int inc = 0;
		std::string dfn = fnstr;
		std::string errstr;
		isinit = false;
		do
		{
			_ptr = _DL_OPEN(dfn.c_str());
			if (_ptr)break;
#ifndef _WIN32
			auto er = dlerror();
			if (er)
				errstr = er;
#endif // !_WIN32
#ifdef __FILE__h__
			dfn = File::getAP(dfn);
#endif
			_ptr = _DL_OPEN(dfn.c_str());
			if (_ptr)break;
			if (sysdirstr)
			{
				dfn = sysdirstr + fnstr;
				_ptr = _DL_OPEN(dfn.c_str());
			}
			if (_ptr)break;
			if (sys64)
			{
				dfn = sys64 + fnstr;
				_ptr = _DL_OPEN(dfn.c_str());
			}
			if (!_ptr) {
				errstr = getLastError();
				printf("Could not load %s dll library!\n", fnstr.c_str());
			}
		} while (0);

		if (_ptr)
		{
			isinit = true;
		}
		return isinit;
	}

	void Shared::dll_close()
	{
		if (_ptr)
		{

#ifdef _WIN32
			FreeLibrary((HMODULE)_ptr);
#else
			dlclose(_ptr);
#endif
			_ptr = 0;
		}
	}

	void* Shared::_dlsym(const char* funcname)
	{
#if defined(_WIN32)
#define __dlsym GetProcAddress
#define LIBPTR HMODULE
#else
#define __dlsym dlsym
#define LIBPTR void*
#endif
		void* func = (void*)__dlsym((LIBPTR)_ptr, funcname);
		if (!func)
			func = (void*)__dlsym((LIBPTR)_ptr, funcname);
		return func;
#undef __dlsym
#undef LIBPTR
	}

	// 批量获取
	void Shared::dllsyms(const char** funs, void** outbuf, int n)
	{
		for (int i = 0; i < n; i++)
		{
			auto fcn = funs[i];
			if (fcn)
			{
				auto it = _dlsym(fcn);
				if (it)
				{
					outbuf[i] = it;
				}
			}
		}
	}

	void Shared::dlsyms(const std::vector<std::string>& funs, void** outbuf)
	{
		auto n = funs.size();
		for (size_t i = 0; i < n; i++)
		{
			auto fcn = funs[i];
			if (fcn.size())
			{
				auto it = _dlsym(fcn.c_str());
				if (it)
				{
					outbuf[i] = it;
				}
			}
		}
	}

	void Shared::dlsyms(const std::vector<const char*>& funs, void** outbuf)
	{
		auto n = funs.size();
		for (size_t i = 0; i < n; i++)
		{
			auto fcn = funs[i];
			if (fcn && *fcn)
			{
				auto it = _dlsym(fcn);
				if (it)
				{
					outbuf[i] = it;
				}
			}
		}
	}

	void* Shared::dllsym(void* ptr, const char* fn)
	{
		Shared* ctx = (Shared*)ptr;
		void* ret = nullptr;
		if (ctx)
		{
			ret = ctx->_dlsym(fn);
		}
		return ret;
	}

	std::string Shared::getLastError()
	{
		std::string str;
#ifdef _WIN32

		char* buf = 0;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf, 0, NULL);
		if (buf)
		{
			str = buf;
			LocalFree(buf);
		}
#else
#ifdef errno
		str = strerror(errno);
#endif
#endif // _WIN32
		return str;
	}

	Shared* Shared::loadShared1(Shared* ptr, const std::string& fnstr, std::vector<std::string>* pdir)
	{
#ifdef NDEBUG
		auto nd = "debug";
#else
		auto nd = "release";
#endif // NDEBUG
		bool is = ptr->loadFile(fnstr);
		if (!is && pdir)
		{
			for (auto it : *pdir)
			{
				auto str = toLower(it);
				if (str.find(nd) != std::string::npos)
				{
					continue;
				}
				if ('/' != *it.rbegin() && '\\' != *it.rbegin())
				{
					it.push_back('/');
				}
				is = ptr->loadFile(it + fnstr);
				if (is)
				{
					break;
				}
			}
		}
		if (!is)
		{
			ptr = nullptr;
		}
		return ptr;
	}

#endif // 1


	// todo icu 
#ifndef NO_MV_ICU
	struct icu_lib_t
	{
		void (*_ubidi_setPara)(UBiDi* pBiDi, const UChar* text, int32_t length, UBiDiLevel paraLevel, UBiDiLevel* embeddingLevels, UErrorCode* pErrorCode);
		int32_t(*_ubidi_countRuns) (UBiDi* pBiDi, UErrorCode* pErrorCode);
		UBiDi* (*_ubidi_open)(void);
		void (*_ubidi_close)(UBiDi* pBiDi);
		UBiDiDirection(*_ubidi_getVisualRun)(UBiDi* pBiDi, int32_t runIndex, int32_t* pLogicalStart, int32_t* pLength);
		int32_t(*_ucnv_convert)(const char* toConverterName, const char* fromConverterName, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode);
		UScriptCode(*_uscript_getScript)(UChar32 codepoint, UErrorCode* err);
		UBool(*_uscript_hasScript)(UChar32 c, UScriptCode sc);
		const char* (*_uscript_getName)(UScriptCode scriptCode);
		const char* (*_uscript_getShortName)(UScriptCode scriptCode);

		UCharsetDetector* (*_ucsdet_open)(UErrorCode* status);
		void (*_ucsdet_close)(UCharsetDetector* ucsd);
		void (*_ucsdet_setText)(UCharsetDetector* ucsd, const char* textIn, int32_t len, UErrorCode* status);
		void (*_ucsdet_setDeclaredEncoding)(UCharsetDetector* ucsd, const char* encoding, int32_t length, UErrorCode* status);
		const UCharsetMatch* (*_ucsdet_detect)(UCharsetDetector* ucsd, UErrorCode* status);
		const char* (*_ucsdet_getName)(const UCharsetMatch* ucsm, UErrorCode* status);

		int32_t(*_ucnv_countAvailable)(void);
		const char* (*_ucnv_getAvailableName)(int32_t n);

		void* _handle;
	};

	static std::vector<std::string> icu_funstr = { "ubidi_setPara","ubidi_countRuns","ubidi_open","ubidi_close","ubidi_getVisualRun"
	,"ucnv_convert" , "uscript_getScript","uscript_hasScript","uscript_getName","uscript_getShortName",
	"ucsdet_open", "ucsdet_close", "ucsdet_setText", "ucsdet_setDeclaredEncoding", "ucsdet_detect", "ucsdet_getName",
	"ucnv_countAvailable","ucnv_getAvailableName",
	};

#ifdef _WIN32
	const char* exts = ".dll";
#else
	const char* exts = ".so";
#endif // _WIN32
#ifdef EAD_ICONV
	static iconv_info_t ic = {};
#endif
	static std::once_flag icof, icuf;
	static std::set<std::string> lsic;
	static icu_lib_t* icub = 0;

	hz::Shared* loadso(const std::string& dlln)
	{
		auto so = hz::Shared::loadShared(dlln + exts);
		return so;
	}
	icu_lib_t* get_icu(int v)
	{
#define mxv 1000
		if (!icub)
		{
			try {
				std::string dlln = "libicuuc";
				std::string dlln0 = "icudt";
				int v1 = v;
#ifdef _WIN32
				//dlln = "icuuc" + std::to_string(v1);
				//dlln0 = "icudt" + std::to_string(v1);
				//auto so0 = loadso(dlln0);
				//auto so = loadso(dlln);
				//if (!so0)
				//{
				//	dlln0 = "icu";
				//	so0 = loadso(dlln0);
				//}
				//if (!so)
				//{
				//	dlln = "icuuc";
				//	so = loadso(dlln);
				//}
				auto so = loadso("icu");
#else
				auto so = loadso("libicu");
				if (!so) loadso(dlln);
#endif // _WIN32

				if (!so) {
					so = loadso(dlln);
					if (!so)
						throw std::runtime_error("-1");
					return 0;
				}
				std::string n;
				void* uc = 0;

#if 1 
				int nc = 0;
				do
				{
					for (int i = v1; i > 0; i--)
					{
						auto n1 = std::to_string(i);
						auto k = "ubidi_close_" + n1;
						auto fb = so->_dlsym(k.c_str());
						if (fb)
						{
							n = n1;
							uc = fb;
							break;
						}
					}
					if (n.empty() && nc == 0) { v1 = mxv; nc++; continue; }

				} while (0);
#endif
				auto ifun = icu_funstr;
				if (n.size())
				{
					for (auto& it : ifun)
					{
						it += "_" + n;
					}
				}
				icu_lib_t tb = {};
				so->dlsyms(ifun, (void**)&tb);
				if (tb._ubidi_close)
				{
					icub = new icu_lib_t();
					*icub = tb;
					icub->_handle = so;
				}
				else {
					hz::Shared::destroy(so);
				}
			}
			catch (const std::exception& e)
			{
				auto ew = e.what();
				if (ew)
				{
					printf(ew);
					printf("load icu error!\n");
				}
			}
		}
		return icub;
	}

	void un_icu()
	{
		if (icub)
		{

			if (icub->_handle)
			{
				hz::Shared::destroy((hz::Shared*)icub->_handle);
			}
			delete icub;
			icub = 0;
		}
	}

	void init_icu()
	{
		if (!icub)
			get_icu(U_ICU_VERSION_MAJOR_NUM);
	}



	std::string icu_convert(const char* instring, int32_t inlen, const char* dst_name, const char* src_name)
	{
		if (!icub)
			get_icu(U_ICU_VERSION_MAJOR_NUM);
		std::string r;
		int32_t iret = 0;
		if (inlen < 0)
			inlen = strlen(instring);
		do {
			if (!icub || !instring || inlen == 0 || !src_name || !(*src_name) || !dst_name || !(*dst_name))break;
			if (icub->_ucnv_convert)
			{
				UErrorCode err_code = {};
				r.resize(align_up(inlen * 4, 16));
				iret = icub->_ucnv_convert(dst_name, src_name, r.data(), r.size(), instring, inlen, &err_code);
				if (err_code == U_BUFFER_OVERFLOW_ERROR)
				{
					iret = icub->_ucnv_convert(dst_name, src_name, 0, 0, instring, inlen, &err_code);
					if (iret > 0)
					{
						r.resize(align_up(iret, 16));
						iret = icub->_ucnv_convert(dst_name, src_name, r.data(), r.size(), instring, inlen, &err_code);
					}
				}
				if (err_code == U_ZERO_ERROR)
				{
					r.resize(iret);
					break;
				}
				if (err_code == U_STRING_NOT_TERMINATED_WARNING)
				{
					break;
				}
			}
		} while (0);
		return r;
	}

	//utf-8,gb2312,ucs4
	//utf-8:  一个英文字母或者是数字占用一个字节，汉字占3个字节
	//gb2312: 一个英文字母或者是数字占用一个字节，汉字占2个字节  

	std::string icu_gbk_u8(const char* str, size_t size)
	{
		std::string r;
		if (str)
		{
			if (size == -1)size = strlen(str);
			if (size > 0)
			{
				r = icu_convert(str, size, "utf-8", "gbk");
			}
		}
		return r;
	}
	std::string icu_u8_gbk(const char* str, size_t size)
	{
		std::string r;
		if (str)
		{
			if (size == -1)size = strlen(str);
			if (size > 0)
			{
				r = icu_convert(str, size, "gbk", "utf-8");
			}
		}
		return r;
	}
	int utf16len(unsigned short* utf16)
	{
		int utf;
		int ret = 0;
		while (utf = *utf16++)
			ret += ((utf < 0xD800) || (utf > 0xDFFF)) ? 2 : 1;
		return ret;
	}
	std::string icu_u16_gbk(const void* str, size_t size)
	{
		std::string r;
		if (str)
		{
			if (size == -1)size = utf16len((uint16_t*)str);
			if (size > 0)
			{
				r = icu_convert((char*)str, size, "gbk", "utf-16le");
			}
		}
		return r;
	}
	std::string icu_u16_u8(const void* str, size_t size)
	{
		std::string r;
		if (str)
		{
			if (size == -1)size = utf16len((uint16_t*)str);
			if (size > 0)
			{
				r = icu_convert((char*)str, size, "utf-8", "utf-16le");
			}
		}
		return r;
	}

	std::u16string icu_u8_u16(const char* str, size_t size)
	{
		std::u16string r;
		if (str)
		{
			if (size == -1)size = strlen(str);
			if (size > 0)
			{
				auto kr = icu_convert((char*)str, size, "utf-16le", "utf-8");
				r.resize(kr.size() / 2);
				memcpy(r.data(), kr.c_str(), kr.size());
			}
		}
		return r;
	}
	std::string get_text_code(const char* data8, size_t size)
	{
		if (!icub)
			get_icu(U_ICU_VERSION_MAJOR_NUM);
		if (!icub || !icub->_ucsdet_open || !icub->_ucsdet_setText || !icub->_ucsdet_detect || !icub->_ucsdet_getName || !icub->_ucsdet_close)return std::string();
		UErrorCode status = U_ZERO_ERROR;
		UCharsetDetector* detector = icub->_ucsdet_open(&status);
		if (U_FAILURE(status)) return std::string();

		icub->_ucsdet_setText(detector, data8, size, &status);
		if (U_FAILURE(status)) {
			icub->_ucsdet_close(detector);
			return std::string();
		}

		const UCharsetMatch* match = icub->_ucsdet_detect(detector, &status);
		if (U_FAILURE(status) || !match) {
			icub->_ucsdet_close(detector);
			return std::string();
		}

		const char* charset = icub->_ucsdet_getName(match, &status);
		if (U_FAILURE(status)) {
			icub->_ucsdet_close(detector);
			return std::string();
		}
		icub->_ucsdet_close(detector);
		return charset;
	}
	std::vector<const char*> get_convert_name()
	{
		if (!icub)
			get_icu(U_ICU_VERSION_MAJOR_NUM);
		std::vector<const char*> r;
		if (icub)
		{
			int32_t n = icub->_ucnv_countAvailable();
			r.reserve(n);
			for (size_t i = 0; i < n; i++)
			{
				const char* p = icub->_ucnv_getAvailableName(i);
				if (p)
				{
					r.push_back(p);
				}
			}
		}
		return r;
	}
#endif



}//!hz

// align val to the next multiple of alignment
uint64_t align_up(uint64_t val, uint64_t alignment)
{
	return (val + alignment - (uint64_t)1) & ~(alignment - (uint64_t)1);
}
// align val to the previous multiple of alignment
uint64_t align_down(uint64_t val, uint64_t alignment)
{
	return val & ~(alignment - (uint64_t)1);
}

uint64_t divideroundingup(uint64_t a, uint64_t b)
{
	return (a + b - (uint64_t)1) / b;
}

#endif //!NO_MAPFILE
