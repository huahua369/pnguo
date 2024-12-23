
#include <pch1.h>

#include <random>
#include <vkgui/win_core.h>
#include "win32msg.h"
#include <vkgui/event.h>
#include <vkgui/pnguo.h>
#include <vkgui/tinysdl3.h>
#include <vkgui/vkrenderer.h>
#include <vkgui/page.h>
#include <vkgui/mapView.h>

#include <cairo/cairo.h>

#include "mshell.h"
#ifdef GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#endif

auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman";// , Malgun Gothic";
auto fontn1 = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Malgun Gothic";

extern "C" {
}

#if 1
#include <regex>
namespace hz {
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
#ifndef FCV
	union u_col
	{
		unsigned int uc;
		unsigned char u[4];
		struct urgba
		{
			unsigned char r, g, b, a;
		}c;
	};
#define FCV 255.0
#define FCV1 256.0
	// rgba
	inline unsigned int to_uint(glm::vec4 col)
	{
		u_col t;
		t.u[0] = col.x * FCV + 0.5;
		t.u[1] = col.y * FCV + 0.5;
		t.u[2] = col.z * FCV + 0.5;
		t.u[3] = col.w * FCV + 0.5;
		return t.uc;
	}
#endif
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
	static uint64_t toUInt(const njson& v, uint64_t de = 0)
	{
		uint64_t ret = de;
		if (v.is_number())
		{
			ret = v.get<uint64_t>();
		}
		else if (!v.is_null())
		{
			ret = std::atoll(trim(v.dump(), "\"").c_str());
		}
		return ret;
	}
	static int64_t toInt(const njson& v, const char* k, int64_t de)
	{
		int64_t ret = de;
		if (v.find(k) == v.end())
		{
			return ret;
		}
		if (v[k].is_number())
		{
			ret = v[k].get<int64_t>();
		}
		else if (!v[k].is_null())
		{
			ret = std::atoll(trim(v[k].dump(), "\"").c_str());
		}
		return ret;
	}
	static int64_t toInt(const njson& v, int64_t de = 0)
	{
		int64_t ret = de;
		if (v.is_number())
		{
			ret = v.get<int64_t>();
		}
		else if (!v.is_null())
		{
			ret = std::atoll(trim(v.dump(), "\"").c_str());
		}
		return ret;
	}
	static double toDouble(const njson& v, double de = 0)
	{
		double ret = de;
		if (v.is_number())
		{
			ret = v.get<double>();
		}
		else if (!v.is_null())
		{
			ret = std::atof(trim(v.dump(), "\"").c_str());
		}
		return ret;
	}
#define toFloat toDouble

	static bool toBool(const njson& v, bool def = false)
	{
		bool ret = def;
		if (v.is_boolean())
		{
			ret = v.get<bool>();
		}
		else if (v.is_number())
		{
			ret = v.get<int>();
		}
		//else
		//{
		//	ret = trim(v.dump(), "\"") != "null";
		//}
		return ret;
	}
	static bool toBool(const njson& v, const char* k, bool def)
	{
		bool ret = def;
		if (v.find(k) != v.end())
		{
			auto& t = v[k];
			if (t.is_boolean())
			{
				ret = t.get<bool>();
			}
			else if (t.is_number())
			{
				ret = t.get<int>();
			}
		}
		//else
		//{
		//	ret = trim(v.dump(), "\"") != "null";
		//}
		return ret;
	}
	static std::string toStr(const njson& v, const char* k, const std::string& des)
	{
		std::string ret = des;
		if (v.find(k) == v.end())
		{
			return ret;
		}
		if (v[k].is_string())
		{
			ret = v[k].get<std::string>();
		}
		else
		{
			ret = trim(v[k].dump(), "\"");
		}
		return ret;
	}
	static std::string toStr(const njson& v, const std::string& des = "")
	{
		std::string ret = des;
		if (v.is_null())
		{
			//ret = "";
		}
		else if (v.is_string())
		{
			ret = v.get<std::string>();
		}
		else
		{
			ret = trim(v.dump(), "\"");
		}
		return ret;
	}
	static std::string getStr(const njson& v, const std::string& key)
	{
		std::string ret;
		if (v.is_null())
		{
			//ret = "";
		}
		else if (v.is_string())
		{
			ret = v.get<std::string>();
		}
		else if (v.is_object() && key != "" && v.find(key) != v.end())
		{
			ret = v[key].get<std::string>();
		}
		else
		{
			ret = trim(v.dump(), "\"");
		}
		return ret;
	}
	template<class T>
	static std::vector<T> toVector(const njson& n)
	{
		std::vector<T> ret;
		if (n.is_array())
		{
			ret = n.get<std::vector<T>>();
		}
		return ret;
	}

	static std::string toStr(double price)
	{
		auto res = /*n > 0 ? std::_Floating_to_string(("%." + std::to_string(n) + "f").c_str(), price) :*/ std::to_string(price);
		const std::string format("$1");
		try {
			std::regex r("(\\d*)\\.0{6}|");
			std::regex r2("(\\d*\\.{1}0*[^0]+)0*");
			res = std::regex_replace(res, r2, format);
			res = std::regex_replace(res, r, format);
		}
		catch (const std::exception& e) {
			return res;
		}
		return res;
	}
	static std::string toStr(double price, int n)
	{
		auto ret = std::to_string(price);
		//return n > 0 ? std::_Floating_to_string(("%." + std::to_string(n) + "f").c_str(), price) : std::to_string(price);
		if (n > 0)
		{
			auto pos = ret.find('.');
			if (pos != std::string::npos)
			{
				auto c = pos + n + 1;
				if (c < ret.size())
				{
					ret.resize(c);
				}
			}
		}
		return ret;
	}
	static std::string toStr(int64_t n)
	{
		return std::to_string(n);
	}
	static uint64_t toHex(const njson& v, uint64_t d = 0)
	{
		uint64_t ret = d;
		do {
			if (v.is_string())
			{
				std::string buf = md::replace_s(toStr(v), " ", "");

				char* str = 0;
				//buf.resize(10);
				if (buf[0] == '#')
				{
					buf.erase(buf.begin());
					int k = buf.size();
					if (k > 8)
					{
						buf.resize(8);
					}
					if (k >= 6)
					{
						std::swap(*((short*)&buf[0]), *((short*)&buf[4]));
					}
					else if (k == 3)
					{
						std::string nc;
						for (auto it : buf)
						{
							nc.insert(nc.begin(), { it, it });
						}
						buf = nc;
					}
					if (buf.size() == 8)
					{
						buf = buf.substr(6, 2) + buf.substr(0, 6);
					}
					else if (buf.size() == 6)
					{
						buf.insert(buf.begin(), { 'f','f' });
					}
					buf.insert(buf.begin(), { '0','x' });
				}
				int cs = 10;
				if (buf.find("0x") == 0)
				{
					cs = 16;
				}
				ret = std::strtoll(buf.c_str(), &str, cs);
			}
			else if (v.is_number())
			{
				ret = toUInt(v);
			}
			else if (v.is_array())
			{
				bool isr = false;
				for (auto& it : v)
				{
					if (!it.is_number())
					{
						isr = true;
						break;
						assert(0);
					}
				}
				if (isr)break;
				// 1 3 4数量
				std::vector<double> trv = v;
				trv.resize(4);
				if (v.size() < 4)
				{
					trv[3] = 1.0;
					if (v.size() == 1)
					{
						trv[1] = trv[2] = trv[0];
					}
				}
				glm::vec4 v1;
				for (int i = 0; i < 4; i++) v1[i] = trv[i];
				ret = to_uint(v1);
			}
		} while (0);
		return ret;
	}
	static unsigned int toColor(const njson& v, unsigned int d = 0)
	{
		return toHex(v, d);
	}
	static std::string ptHex(const std::string& str, char ch = 'x', char step = '\0')
	{
		char spch[8] = { '%', '0', '2', ch, step, 0 };
		std::string chBuffer;
		char chEach[10];
		int nCount;
		memset(chEach, 0, 10);
		unsigned char* d = (unsigned char*)str.c_str();
		auto len = str.size();
		for (nCount = 0; nCount < len /*&& d[nCount]>0*/; nCount++)
		{
			sprintf(chEach, spch, d[nCount]);
			chBuffer += chEach;
		}
		return chBuffer;
	}

	static std::string toColor2(unsigned int d)
	{
		std::string t;
		t.resize(sizeof(unsigned int));
		memcpy((void*)t.data(), &d, t.size());
		std::reverse(t.begin(), t.end());
		t = t.empty() ? "0x00" : "0x" + ptHex(t);
		return t;
	}

#if (defined(GLM_VERSION))
	static glm::ivec2 toiVec2(const njson& v, int d = -1)
	{
		glm::ivec2 rv = { d, d };

		if (v.is_number())
		{
			rv[0] = rv[1] = rv[2] = rv[3] = v.get<int64_t>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 2 && i < v.size(); i++)
			{
				rv[i] = v[i].get<int64_t>();
			}
		}
		return rv;
	}
	static glm::ivec2 toiVec2(const njson& v, glm::ivec2& ot)
	{
		glm::ivec2& rv = ot;

		if (v.is_number())
		{
			rv[0] = rv[1] = rv[2] = rv[3] = v.get<int64_t>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 2 && i < v.size(); i++)
			{
				rv[i] = v[i].get<int64_t>();
			}
		}
		return rv;
	}
	static glm::vec2 toVec2(const njson& v, double d = 0)
	{
		glm::vec2 rv = { d, d };

		if (v.is_number())
		{
			rv[0] = rv[1] = v.get<double>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 2 && i < v.size(); i++)
			{
				rv[i] = v[i].get<double>();
			}
		}
		return rv;
	}
	static glm::vec2 toVec2(const njson& v, glm::vec2& ot)
	{
		glm::vec2& rv = ot;

		if (v.is_number())
		{
			rv[0] = rv[1] = v.get<double>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 2 && i < v.size(); i++)
			{
				rv[i] = v[i].get<double>();
			}
		}
		return rv;
	}
	static glm::vec3 toVec3(const njson& v, double d = 0)
	{
		glm::vec3 rv = { d, d, d };

		if (v.is_number())
		{
			rv[0] = rv[1] = rv[2] = v.get<double>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 3 && i < v.size(); i++)
			{
				rv[i] = v[i].get<double>();
			}
		}
		return rv;
	}
	static glm::vec4 toVec4(const std::vector<double>& v, bool one1 = false)
	{
		glm::vec4 rv;
		{
			std::vector<double> trv = v;
			if (one1 && trv.size() == 1) {
				rv[0] = rv[1] = rv[2] = rv[3] = trv[0];
			}
			else {
				for (size_t i = 0; i < v.size() && i < 4; i++)
				{
					rv[i] = v[i];
				}
			}
		}
		return rv;
	}
	static glm::vec4 toVec4(const njson& v, double d = 0)
	{
		glm::vec4 rv = { d, d, d, d };
		if (v.is_number())
		{
			rv[0] = rv[1] = rv[2] = rv[3] = v.get<double>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 4 && i < v.size(); i++)
			{
				rv[i] = v[i].get<double>();
			}
		}
		return rv;
	}
	static glm::ivec4 toiVec4(const njson& v, int d = 0)
	{
		glm::ivec4 rv = { d, d, d, d };
		if (v.is_number())
		{
			rv[0] = rv[1] = rv[2] = rv[3] = v.get<int64_t>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 4 && i < v.size(); i++)
			{
				rv[i] = v[i].get<int64_t>();
			}
		}
		return rv;
	}
	static glm::ivec3 toiVec3(const njson& v, int d = 0)
	{
		glm::ivec3 rv = { d, d, d };
		if (v.is_number())
		{
			rv[0] = rv[1] = rv[2] = v.get<int64_t>();
		}
		else if (v.is_array() && v.size() > 0)
		{
			for (size_t i = 0; i < 3 && i < v.size(); i++)
			{
				rv[i] = v[i].get<int64_t>();
			}
		}
		return rv;
	}
	static njson v2to(const glm::vec2& v)
	{
		std::vector<double> rv = { v.x, v.y };
		return rv;
	}
	static njson v3to(const glm::vec3& v)
	{
		std::vector<double> rv = { v.x, v.y, v.z };
		return rv;
	}
	static njson v4to(const glm::vec4& v)
	{
		std::vector<double> rv = { v.x, v.y, v.z, v.w };
		return rv;
	}

	template <typename T>
	static void v2to_a(const T& v, njson& a)
	{
		a.push_back(v.x);
		a.push_back(v.y);
	}
	template <typename T>
	static void v3to_a(const T& v, njson& a)
	{
		a.push_back(v.x);
		a.push_back(v.y);
		a.push_back(v.z);
	}
	template <typename T>
	static void v4to_a(const T& v, njson& a)
	{
		a.push_back(v.x);
		a.push_back(v.y);
		a.push_back(v.z);
		a.push_back(v.w);
	}
#endif
	template <typename T>
	static T* toPtr(const njson& v)
	{
#if 0
		std::string buf = toStr(v);
		unsigned long long ret = 0;
		char* str = 0;
		buf.resize(18);
		ret = std::strtoll(buf.c_str(), &str, buf.find("0x") == 0 ? 16 : 10);
		return (T*)ret;
#else
		return (T*)toHex(v);
#endif
	}
#ifndef _ui64toa_s
	static char* _ui64toa_s(uint64_t num, char* str, int size, int radix)
	{/*索引表*/
		char index[] = "0123456789ABCDEF";
		uint64_t unum = 0;/*中间变量*/
		int i = 0, j, k;
		/*确定unum的值*/
		if (radix == 10 && num < 0)/*十进制负数*/
		{
			unum = (uint64_t)-num;
			str[i++] = '-';
		}
		else unum = (uint64_t)num;/*其他情况*/
		/*转换*/
		do {
			str[i++] = index[unum % (uint64_t)radix];
			unum /= radix;
		} while (unum);
		str[i] = '\0';
		/*逆序*/
		if (str[0] == '-')
			k = 1;/*十进制负数*/
		else
			k = 0;

		for (j = k; j <= (i - 1) / 2; j++)
		{
			char temp;
			temp = str[j];
			str[j] = str[i - 1 + k - j];
			str[i - 1 + k - j] = temp;
		}
		return str;
	}
#endif // !_ui64toa_s
	//template <typename T>
	static std::string fromPtr(void* pv)
	{
		std::string buf = "0x";
		buf.resize(sizeof(void*) * 4);
		_ui64toa_s((uint64_t)pv, (char*)(buf.data() + 2), buf.size() - 2, 16);
		return buf.c_str();
	}
}
#endif
struct librpspr_lib
{
	void* ptr = 0;
	// 创建渲染器，类型是rgba，可以提前设置data做缓冲区，uint32数组或uint8
	int (*new_rp)(int w, int h, int64_t* ret, int bgrtex) = 0;
	// 删除渲染器
	void (*free_rp)(int64_t p) = 0;
	// 渲染完可以获取渲染的数据，ret的大小要跟渲染器一样
	int (*get_image)(void* p, uint8_t* ret) = 0;
	int (*rp_clear)(void* p, uint32_t c, double depth) = 0;
	// 新api。以下文件名支持网址、本地文件
	// 添加spr和对应的调色板文件名，调色板偏移(有的可能要负数)。返回序号draw_spr用
	int (*add_spr)(void* p, const char* fn, const char* palfn, int pal_pos) = 0;
	// 获取spr帧数
	int (*get_spr_frame_count)(void* p, int idx) = 0;
	// 保存spr的图片到png文件
	int (*save_spr2png)(void* p, int idx, const char* fn) = 0;
	// 加载坐骑/附件/足迹光环等，文件名用分号分隔json;png;graypng;pal.bmp顺序不限。坐骑深度信息没有.pal.bmp就拼三种文件名即可
	int (*add_part)(void* p, const char* uris) = 0;
	int (*set_ride)(void* p, const char* fly, int d, const char* fnm, const char* fntcp) = 0;
	int (*set_addon)(void* p, const char* addons) = 0;

	// 渲染顺序排序ride\spr\part\addon
	int (*draw_ride)(void* p, int frame, int* pos) = 0;
	int (*draw_spr)(void* p, int idx, int frame, int* pos, int z) = 0;
	int (*draw_part)(void* p, int idx, int frame, int* pos, int z) = 0;
	int (*draw_addon_flr)(void* p, int frame, int* pos) = 0;
	double (*get_max_depth)(void* p) = 0;
	int (*draw_spr_ps)(void* p, int idx, int frame, int* dst, int z, int ps) = 0;
public:
	librpspr_lib();
	~librpspr_lib();
	bool load();
};
librpspr_lib::librpspr_lib() {
	load();
}
librpspr_lib::~librpspr_lib() {
	if (ptr)hz::shared_destroy(ptr);
	delete ptr; ptr = 0;
}
bool librpspr_lib::load()
{
	if (ptr)return true;
	auto k = hz::shared_load(R"(rpspr.dll)");
	static const char* ccfn[] = { "ptr_null",
	"new_rp","free_rp","get_image","rp_clear","add_spr","get_spr_frame_count","save_spr2png"
	,"add_part","set_ride","set_addon","draw_ride","draw_spr","draw_part","draw_addon_flr","get_max_depth","draw_spr_ps"
	};
	if (k)
	{
		ptr = k;
		hz::shared_get(k, ccfn, (void**)&ptr, sizeof(ccfn) / sizeof(char*));
	}
	return ptr && new_rp && free_rp;
}
class dspr_cx
{
public:
	librpspr_lib* so = 0;
	void* ctx = 0;
	std::vector<glm::ivec4> sprv;
	std::vector<uint32_t> imgdata;
	cairo_surface_t* img = 0;

	// 坐骑的渲染坐标
	glm::ivec2 ride_pos = {};
	glm::ivec2 csize = {};
public:
	dspr_cx();
	~dspr_cx();

	void init(int w, int h);
	void clear();
	void add_spr(const std::string& sprfn, const std::string& palbmp, int pal_pos);
	//不同坐骑可能要设置不同值
	void set_spr_pos(uint32_t idx, int x, int y, int z = 950000);
	void set_ride(const std::string& fly, int d, const std::string& addon, const std::string& fnm_dir, const std::string& fntcp_dir);
	void draw(int i, int x, int y);
private:

};

dspr_cx::dspr_cx()
{
	so = new librpspr_lib();
}

dspr_cx::~dspr_cx()
{
	if (img)
		free_image_cr(img);
	img = 0;
	if (so)
	{
		if (ctx)
			so->free_rp((int64_t)ctx);
		delete so; so = 0;
	}
}

void dspr_cx::init(int w, int h)
{
	bool tex_bgr = 1;
	csize = { w,h };
	if (ctx)
		so->free_rp((int64_t)ctx);
	so->new_rp(w, h, (int64_t*)&ctx, tex_bgr);
	glm::ivec2 csize = { w,h };
	imgdata.resize(w * h);
	if (img)
		free_image_cr(img);
	img = 0;
	img = new_image_cr(csize, imgdata.data());
	sprv.clear();
}


//在__init__函数调用 add_spr\set_ride
void dspr_cx::add_spr(const std::string& sprfn, const std::string& palbmp, int pal_pos)
{
	auto w = so->add_spr(ctx, sprfn.c_str(), palbmp.c_str(), pal_pos);
	sprv.push_back({ 0,0,0,w });
}

void dspr_cx::set_ride(const std::string& fly, int d, const std::string& addon, const std::string& fnm_dir, const std::string& fntcp_dir)
{
	// 动作类型，方向，坐骑深度目录(需要有这三种文件json;png;graypng)，坐骑tcp目录（json;png;graypng;pal.bmp）
	//so->set_ride(ctx, "stand", 0, R"(https://xyq.gsf.netease.com/h5avtres/1.25/e135776/p722/)", R"(https://xyq.gsf.netease.com/static_h5/shape/char/0722/)");
	so->set_ride(ctx, fly.c_str(), d, fnm_dir.c_str(), fntcp_dir.c_str());
	// 附件，路径和坐骑tcp一样，（json;png;graypng;pal.bmp）保存在addon_f之类文件夹。支持fblr四种要有相应文件夹
	so->set_addon(ctx, addon.c_str());
}

void dspr_cx::set_spr_pos(uint32_t idx, int x, int y, int z)
{
	if (idx < sprv.size())
	{
		sprv[idx].x = x;
		sprv[idx].y = y;
		sprv[idx].z = z;
	}
}

void dspr_cx::draw(int i, int x, int y)
{
	glm::ivec2 pos = { x,y };
	auto rpos = pos + ride_pos;
	//渲染调用 
	so->rp_clear(ctx, 0xff000000, 0.0);
	so->draw_ride(ctx, i, (int*)&rpos);//写入坐骑原始深度\渲染坐骑和附件b
	auto maxd = so->get_max_depth(ctx);
	//渲染角色/装备等
	static int ps[10] = {};
	int q = 0;
	for (auto& it : sprv)
	{
		glm::ivec2 sp = it;
		sp += pos;
		if (it.w > 0)
			so->draw_spr_ps(ctx, it.w, i, (int*)&sp, it.z, ps[q]);
		q++;
	}
	auto maxd0 = so->get_max_depth(ctx);
	// 渲染坐骑挂件
	so->draw_addon_flr(ctx, i, (int*)&rpos);
	so->get_image(ctx, (uint8_t*)imgdata.data());
}

std::string get_temp_path() {
	std::string str; str.resize(MAX_PATH * 2);
	auto n = GetTempPathA(str.size(), str.data());
	if (n > 0)
		str.resize(n);
	else
		str.clear();
	return str;
}

class spr_dev
{
public:
	dspr_cx* sp = 0;
	std::string cstr;
	int fp = 0;
public:
	spr_dev();
	~spr_dev();
	void set_text(const std::string& str);

	void draw(cairo_t* cr, const glm::vec2& dps);
private:

};

spr_dev::spr_dev()
{
	sp = new dspr_cx();
}

spr_dev::~spr_dev()
{
}

void spr_dev::set_text(const std::string& str)
{
	try
	{
		njson nc = njson::parse(str);
		auto kstr = nc.dump(2);
		if (kstr != cstr && sp)
		{
			cstr = kstr;

			glm::ivec2 csize = hz::toiVec2(nc["csize"]);
			auto ride_pos = hz::toiVec2(nc["ride_pos"]);
			auto fly = hz::toStr(nc["fly"]);
			auto dir = hz::toInt(nc["dir"]);
			auto addon = hz::toStr(nc["addon"]);
			auto ride_dir = hz::toStr(nc["ride_dir"]);
			auto ride_tcp_dir = hz::toStr(nc["ride_tcp_dir"]);
			auto spr_dir = hz::toStr(nc["spr_dir"]);
			auto& cspr = nc["spr"];
			sp->init(csize.x, csize.y);
			sp->ride_pos = ride_pos;
			sp->set_ride(fly, dir, addon, ride_dir, ride_tcp_dir);
			int i = 0;
			for (auto& c0 : cspr)
			{
				std::string palurl = R"(https://xyq.gsf.netease.com/static_h5/pal/equip/)";
				auto pos = hz::toiVec3(c0["pos"]);
				auto pal = hz::toiVec2(c0["pal"]);
				auto uri = spr_dir + hz::toStr(c0["name"]);
				sp->add_spr((char*)uri.c_str(), palurl + std::to_string(pal.x) + ".pal.bmp", pal.y);
				sp->set_spr_pos(i, pos.x, pos.y, pos.z);
				i++;
			}
		}
	}
	catch (const std::exception& e)
	{
		printf("json error:\t%s", e.what());
	}
}

void spr_dev::draw(cairo_t* cr, const glm::vec2& dps)
{
	if (sp) {
		sp->draw(fp, 0, 0);
		fp++;
		if (fp > 100000)fp = 0;
		cairo_as _ss_(cr);
		cairo_translate(cr, dps.x, dps.y);
		image_b pimg = {};
		pimg.image = sp->img;
		pimg.rc = { 0,0,sp->csize };
		// 图形通用软渲染接口
		draw_ge(cr, &pimg, 1);
	}
}


void build_spr_ui(form_x* form0)
{
	std::string tp = get_temp_path();
	glm::ivec2 csize = { 800,600 };
	njson c;
	c["csize"] = { 800,600 };
	c["fly"] = "stand";
	c["dir"] = 0;
	c["addon"] = "flr";
	c["ride_dir"] = R"(https://xyq.gsf.netease.com/h5avtres/1.25/e135776/p722/)";
	c["ride_tcp_dir"] = R"(https://xyq.gsf.netease.com/static_h5/shape/char/0722/)";
	c["spr_dir"] = R"(https://xyq.gsf.netease.com/avtres_hd_full_dir/)";
	c["ride_pos"] = { 250,280 };
	auto& cspr = c["spr"];
	for (size_t i = 0; i < 1; i++)
	{
		njson c0;
		c0["pos"] = { 230,210,950000 };
		c0["pal"] = { 24608,192 };
		c0["name"] = R"(e24608/p2/8379f964.spr)";
		cspr.push_back(c0);
	}
	glm::ivec2 size = { 1024,800 };
	glm::ivec2 cview = { 10240,10240 };
	auto p = new plane_cx();
	uint32_t pbc = 0xff2c2c2c;// 背景色
	p->set_color({ 0x80ff802C,1,5,pbc });
	p->set_rss(5);
	p->_lms = { 8,8 };
	form0->bind(p);	// 绑定到窗口  
	p->add_familys(fontn, 0);
	p->add_familys(fontn1, 0);
	p->draggable = false; //可拖动
	p->fontsize = 12;
	p->set_size(size);
	p->set_pos({ 1,30 });
	//p->set_select_box(2, 0.012);
	int width = 16;
	int rcw = 14;
	{
		// 设置带滚动条
		//p->set_scroll(width, rcw, { 4, 4 }, { 2,0 }, { 0,2 });
		//p->set_scroll_hide(1);
		//p->set_view(size, cview);
	}
	auto spredit = p->add_input("", { 1000,300 }, 0);
	auto cstr = c.dump(2);
	static spr_dev* sp = new spr_dev();
	sp->set_text(cstr);
	spredit->set_text(cstr.c_str(), cstr.size());
	spredit->changed_cb = [=](edit_tl* ptr) {
		};
	auto btn0 = p->add_cbutton((char*)u8"修改", { 100,30 }, 0);
	btn0->click_cb = [=](void* p, int css) {
		auto str = spredit->_text;
		sp->set_text(str);
		spredit->set_text(sp->cstr.c_str(), sp->cstr.size());
		};

	auto dpx1 = p->push_dragpos({ 140,360 });// , { 300,600 });// 增加一个拖动坐标
	p->on_click = [](plane_cx* p, int state, int clicks, const glm::ivec2& mpos) {};
	p->update_cb = [=](float delta)
		{
			static double xt = 0;
			xt += delta;
			if (xt > 0.1) { xt = 0; return 1; }
			return 0;
		};
	p->draw_front_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
		};
	p->draw_back_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
			auto dps = p->get_dragpos(dpx1);//获取拖动时的坐标
			sp->draw(cr, dps);

		};
}


/*
容器控件：子项渲染、事件处理、数据结构
*/
class vcpkg_cx
{
public:
	std::queue<std::string> cmds;
	std::mutex _lock;
	std::string rootdir;// vcpkg根目录
	njson vlist;
	njson vsearch;
public:
	vcpkg_cx();
	~vcpkg_cx();
	void do_clone(const std::string& dir, int depth);
	void do_bootstrap();
	// 拉取
	void do_pull();
	// 比较更新了哪些库
	void do_update(const std::string& t);
	// 搜索库
	void do_search();
	// 列出安装库
	void do_list();
	// 集成到全局
	void do_integrate_i();
	// 移除全局
	void do_integrate_r();
	// 查看支持的架构
	void do_get_triplet();
	// 安装库
	void do_install(const std::string& t, const std::string& triplet);
	// 删除库
	void do_remove(const std::string& t, const std::string& triplet);
	// 执行自定义命令
	void push_cmd(const std::string& c);
	// 执行命令
	void do_cmd();
private:

};
vcpkg_cx::vcpkg_cx() {}
vcpkg_cx::~vcpkg_cx() {}
void vcpkg_cx::do_clone(const std::string& dir, int depth)
{
	if (dir.size() < 2)return;
	std::lock_guard<std::mutex> lock(_lock);
	rootdir = dir;
	std::string c = "git clone https://github.com/microsoft/vcpkg.git";
	if (depth > 0)
	{
		c += " --depth=" + std::to_string(depth);
	}
	cmds.push(c);
}
void vcpkg_cx::do_bootstrap()
{
	if (rootdir.size() < 2)return;
	std::lock_guard<std::mutex> lock(_lock);
#ifdef _WIN32
	cmds.push("bootstrap-vcpkg.bat");
#else
	cmds.push("bootstrap-vcpkg.sh");
#endif // _WIN32

}
void vcpkg_cx::do_pull()
{
	if (rootdir.size() < 2)return;
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("git pull");
}
void vcpkg_cx::do_update(const std::string& t)
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg update");
}
void vcpkg_cx::do_search()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg search --x-full-desc --x-json");
}
void vcpkg_cx::do_list()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg list --x-full-desc --x-json");

}

void vcpkg_cx::do_integrate_i()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg integrate install");
}

void vcpkg_cx::do_integrate_r()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg integrate remove");
}

void vcpkg_cx::do_get_triplet()
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push("vcpkg help triplet");
}

void vcpkg_cx::do_install(const std::string& t, const std::string& triplet)
{
	std::lock_guard<std::mutex> lock(_lock);
	auto c = "vcpkg install " + t;
	if (triplet.size())
	{
		c += ":" + triplet;
	}
	cmds.push(c);
}

void vcpkg_cx::do_remove(const std::string& t, const std::string& triplet)
{
	std::lock_guard<std::mutex> lock(_lock);
	auto c = "vcpkg remove " + t;
	if (triplet.size())
	{
		c += ":" + triplet;
	}
	cmds.push(c);
}

void vcpkg_cx::push_cmd(const std::string& c)
{
	std::lock_guard<std::mutex> lock(_lock);
	cmds.push(c);
}


void vcpkg_cx::do_cmd()
{
	std::string c;
	for (; cmds.size();) {
		{
			std::lock_guard<std::mutex> lock(_lock);
			c.swap(cmds.front()); cmds.pop();
		}
		if (c.empty())continue;
		auto jstr = hz::cmdexe(c, rootdir.empty() ? nullptr : rootdir.c_str());
		try
		{
			njson k = njson::parse(jstr);
			if (k.size())
				vlist = k;
		}
		catch (const std::exception& e)
		{
			printf("json error:\t%s\n", e.what());
		}
		printf("%s\n", jstr.c_str());
		// todo 处理返回结果
	}
}


menu_cx* menu_m(form_x* form0)
{
	menumain_info m = {};
	m.form0 = form0;
	m.fontn = fontn;
	// 主菜单
	std::vector<std::string> mvs = { (char*)u8"文件",(char*)u8"编辑",(char*)u8"视图",(char*)u8"工具",(char*)u8"帮助" };
	m.mvs = &mvs;
	auto mc = mg::new_mm(&m);
	return mc;
}


void show_ui(form_x* form0, menu_cx* gm)
{
	if (!form0)return;

	glm::ivec2 size = { 1024,800 };
	glm::ivec2 cview = { 10240,10240 };
	auto p = new plane_cx();
	uint32_t pbc = 0xf02c2c2c;
	p->set_color({ 0x80ff802C,1,5,pbc });
	form0->bind(p);	// 绑定到窗口  

	build_spr_ui(form0);
	p->set_rss(5);
	p->_lms = { 8,8 };
	p->add_familys(fontn, 0);
	p->add_familys(fontn1, 0);
	p->draggable = false; //可拖动
	p->set_size(size);
	p->set_pos({ 1,30 });
	p->set_select_box(2, 0.012);
	p->on_click = [](plane_cx* p, int state, int clicks, const glm::ivec2& mpos) {};
	p->fontsize = 16;
	int width = 16;
	int rcw = 14;
	{
		// 设置带滚动条
		p->set_scroll(width, rcw, { 4, 4 }, { 2,0 }, { 0,2 });
		p->set_scroll_hide(1);
		p->set_view(size, cview);
	}
	size.y -= 50;
	size.x -= 50 * 10;
	size /= 2;
	vcpkg_cx* vx = new vcpkg_cx();
	std::vector<const char*> btnname = {
	"install",//安装库(const std::string & t, const std::string & triplet); 
	"remove",//删除库(const std::string & t, const std::string & triplet); 
	"clone",//(const std::string & dir, int depth); 
	"bootstrap",// 
	"update",//比较更新了哪些库(const std::string & t);	 
	"ecmd",//执行自定义命令(const std::string & c); 
	};
	std::vector<const char*> mname = {
	"list",//列出安装库 
	"search",//搜索库 
	"pull",//拉取  		 
	"get triplet",//查看支持的架构  
	"integrate install",//集成到全局 
	"integrate remove",//移除全局 
	};
	auto color = 0x2c2c2c;// hz::get_themecolor();
	((uint8_t*)(&color))[3] = 0x80;
#if 0
	std::vector<menu_info> vm;
	vm.push_back({ mname.data(),mname.size() });
	auto mms = gm->new_menu_g(vm.data(), vm.size(), { 200,30 }, [=](mitem_t* pm, int type, int idx)
		{
			if (type == 1)
			{
				printf("click:%d\t%d\n", type, idx);
			}
		});
	//for (auto& nt : btnname)
	{
		auto cp = p->add_gbutton((char*)u8"功能", { 160,30 }, color);
		cp->click_cb = [=](void* ptr, int clicks)
			{
				auto cps = cp->get_pos();
				cps.y += cp->size.y + cp->thickness;
				gm->show_mg(mms, 0, cps);
			};
	}
#else
	for (auto& nt : mname)
	{
		auto cp = p->add_cbutton(nt, { 180,30 }, 0);
		cp->effect = uTheme::light;
		cp->click_cb = [=](void* ptr, int clicks)
			{
				printf("%d\n", clicks);
				vx->do_list();
				vx->do_cmd();
			};
	}
	auto ce = p->add_input("", { 1000,30 }, 1);
	ce->set_family(0, 12);
	for (auto& nt : btnname)
	{
		auto cp = p->add_gbutton(nt, { 160,30 }, color);
		cp->click_cb = [=](void* ptr, int clicks)
			{
			};
	}

#endif
	int cs[] = { sizeof(glm::vec1),sizeof(glm::vec2),sizeof(glm::vec3),sizeof(glm::ivec3) };
	auto a4 = glm::translate(glm::vec3(1.2, 0.2, 1.3));
	auto a40 = glm::translate(glm::vec3(0.2, 0.2, 1.3));
	auto aa = a4 * a40;
	glm::vec4 a = glm::vec4(1.0, 2.2, 3.0, 1.0);
	auto b = glm::vec4(0.1, .2, .3, 1.0);
	auto c = a * b;
	auto dpx1 = p->push_dragpos({ 0,0 });// , { 300,600 });// 增加一个拖动坐标
	size /= 2;
	auto dpx = p->push_dragpos(size, { 900 ,630 });// 增加一个拖动坐标
	auto rint = get_rand64(0, (uint32_t)-1);

	auto ft = p->ltx->ctx;
	int fc = ft->get_count();						// 获取注册的字体数量
	const char* get_family(int idx);		// 获取字体名称
	const char* get_family_en(const char* family);		// 获取字体名称
	const char* get_family_full(int idx);	// 获取字体全名
	struct ft_info
	{
		std::string name, full, style;
	};
	static std::vector<ft_info> fvs;
	fvs.resize(fc);
	static std::vector<std::string> ftns;
	std::string k;
	for (size_t i = 0; i < fc; i++)
	{
		int cs = ft->get_count_style(i);			// 获取idx字体风格数量
		auto& it = fvs[i];
		it.name = ft->get_family_cn(i);
		it.full = ft->get_family_full(i);
		for (size_t c = 0; c < cs; c++)
		{
			auto sty = ft->get_family_style(i, c);
			it.style += sty; it.style.push_back(';');
		}
		{
			if (i > 0 && (i % 30 == 0))
			{
				ftns.push_back(k); k.clear();
			}
			k += it.name + "\n";
		}
	}
	auto ftff = ft->get_font("Source Han Sans SC", 0);
	if (k.size())
		ftns.push_back(k);
	static int stt = 0;
	p->update_cb = [=](float delta)
		{
			return 0;
		};
	p->draw_front_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
		};
	p->draw_back_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
			auto f = ft->get_font("a", 0);
			auto dps = p->get_dragpos(dpx);//获取拖动时的坐标
			auto dps1 = p->get_dragpos(dpx1);//获取拖动时的坐标

			uint32_t color = 0x80FF7373;// hz::get_themecolor();

			{
				cairo_as _ss_(cr);
				cairo_translate(cr, dps.x, dps.y);
				glm::vec4 rc = { 0,0,300,1000 };
				text_style_t st = {};
				st.font = 1;
				st.text_align = { 0.0,0.0 };
				st.font_size = 16;
				st.text_color = -1;
				//auto rc1 = p->ltx->get_text_rect(st.font, str.c_str(), -1, st.font_size);
				auto rc2 = rc;
				draw_rect(cr, { -2.5,  -2.5,900 + 6,630 + 6 }, 0xf0236F23, 0x80ffffff, 2, 1);
				for (auto& str : ftns)
				{
					draw_text(cr, p->ltx, str.c_str(), -1, rc, &st);
					rc.x += 300;
				}
			}
		};
}

void test_img() {

	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr.png", 0);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr60.jpg", 60);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr80.jpg", 80);
}

int main()
{
#ifdef _DEBUG
	system("rd /s /q E:\\temcpp\\SymbolCache\\tcmp.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\vkcmp.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\cedit.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\p86.pdb");
#endif   
	const char* wtitle = (char*)u8"vcpkg管理工具";
	auto app = new_app();
	do_text(fontn, 0, strlen(fontn));
	glm::ivec2 ws = { 1280,860 };
	// ef_vulkan ef_gpu
	form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, (ef_vulkan | ef_resizable));//ef_gpuef_dx11| ef_vsync
	//form_x* form1 = (form_x*)new_form(app, wtitle1, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable);
	auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
	auto kd = sdldev.vkdev;
	sdldev.vkdev = 0;								// 清空使用独立创建逻辑设备
	std::vector<device_info_t> devs = get_devices(sdldev.inst); // 获取设备名称列表
	auto gm = menu_m(form0);
	show_ui(form0, gm);
	//show_cpuinfo(form0);
	// 运行消息循环
	run_app(app, 0);
	free_app(app);
	return 0;
}
