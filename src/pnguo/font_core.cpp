/*
字体文本渲染管理实现
*/
#include "pch1.h" 
#include "font_core.h"
#include "mapView.h" 

#ifdef _WIN32
#include <dwrite.h>
#ifdef min
#undef min
#undef max
#endif
#endif // _WIN32

#ifndef TTF_USE_HARFBUZZ
#define TTF_USE_HARFBUZZ 1
#endif

#ifndef NO_FONT_CX 

#ifndef NO_FONT_ICU
#include <hb.h>
#include <hb-ot.h>
#include <fontconfig/fontconfig.h> 
#include <unicode/uchar.h>
#include <unicode/ucnv.h>
#include <unicode/utypes.h>
#include <unicode/ucsdet.h>
#include <unicode/unistr.h>
#include <unicode/ustring.h>
#include <unicode/ubidi.h> 
#include <unicode/uscript.h>  
#endif

#include <stb_image.h>
#include <stb_image_write.h>
#include <stb_rect_pack.h>
#include <stb_truetype.h>

typedef uint16_t Offset16;
typedef uint32_t Offset32;

/*
* 获取字体信息名
1 family名
2 样式名
3 唯一标识符
4 全名
6 postscript名
*/
std::vector<std::string> get_name_idx(hb_face_t* face, int idx, const std::string& fn)
{
	uint32_t n = 0;
	auto ns = hb_ot_name_list_names(face, &n);
	std::string name;
	std::vector<std::string> r;
	for (size_t i = 0; i < n; i++)
	{
		auto it = ns[i];
		if (it.name_id != idx)
		{
			continue;
		}
		uint32_t ss = 0;
		auto kn8 = hb_ot_name_get_utf8(face, it.name_id, it.language, &ss, 0);
		name.resize(kn8 + 1); ss = kn8 + 1;
		auto kn = hb_ot_name_get_utf8(face, it.name_id, it.language, &ss, (char*)name.c_str());
		//printf("%d\t%s\n", it.name_id, name.c_str());
		name = name.c_str();
		if (name.size() && name != fn)
			r.push_back(name.c_str());
	}
	return r;
}
std::string get_pat_str(FcPattern* font, const char* o, int n)
{
	FcChar8* s = nullptr;
	std::string r;
	if (o && ::FcPatternGetString(font, o, n, &s) == FcResultMatch)
	{
		if (s)
		{
			r = (char*)s;
		}
	}
	return r;
}
std::vector<std::string> get_pat_strs(FcPattern* font, const char* o)
{
	std::vector<std::string> rv;
	FcChar8* s = nullptr;
	int n = 0;
	do {
		if (o && ::FcPatternGetString(font, o, n, &s) == FcResultMatch)
		{
			if (s)
			{
				rv.push_back((char*)s);
			}
		}
		else { break; }
		n++;
	} while (1);
	return rv;
}
int get_pat_int(FcPattern* font, const char* o)
{
	int s = 0;
	if (o && ::FcPatternGetInteger(font, o, 0, &s) == FcResultMatch)
	{
	}
	return s;
}

// 辅助函数：获取字体文件路径 
#ifdef _WIN32
std::wstring GetFontFilePath(IDWriteFont* pFont) {
	IDWriteFontFace* pFontFace = nullptr;
	HRESULT hr = pFont->CreateFontFace(&pFontFace);
	if (FAILED(hr)) return L"";

	UINT32 fileCount = 0;
	hr = pFontFace->GetFiles(&fileCount, nullptr); // 获取文件数量 
	if (FAILED(hr) || fileCount == 0) {
		pFontFace->Release();
		return L"";
	}

	std::vector<IDWriteFontFile*> files(fileCount);
	hr = pFontFace->GetFiles(&fileCount, files.data());
	if (FAILED(hr)) {
		pFontFace->Release();
		return L"";
	}

	std::wstring filePath;
	for (UINT32 i = 0; i < fileCount; ++i) {
		IDWriteFontFileLoader* pLoader = nullptr;
		hr = files[i]->GetLoader(&pLoader);
		if (SUCCEEDED(hr)) {
			const void* refKey;
			UINT32 refKeySize;
			hr = files[i]->GetReferenceKey(&refKey, &refKeySize);

			if (SUCCEEDED(hr)) {
				IDWriteLocalFontFileLoader* pLocalLoader = nullptr;
				hr = pLoader->QueryInterface(&pLocalLoader);
				if (SUCCEEDED(hr) && pLocalLoader != nullptr) {
					UINT32 pathLength = 0;
					hr = pLocalLoader->GetFilePathLengthFromKey(refKey, refKeySize, &pathLength);
					if (SUCCEEDED(hr) && pathLength > 0) {
						std::vector<wchar_t> pathBuffer(pathLength + 1);
						hr = pLocalLoader->GetFilePathFromKey(refKey, refKeySize, pathBuffer.data(), pathLength + 1);
						if (SUCCEEDED(hr)) {
							filePath = std::wstring(pathBuffer.data());
							break; // 找到路径即退出循环 
						}
					}
					pLocalLoader->Release();
				}
			}
			pLoader->Release();
		}
		files[i]->Release();
	}
	pFontFace->Release();
	return filePath;
}
#endif
std::map<std::string, fontns> get_allfont()
{
	int r = 0;
	std::map<std::string, fontns> fyv;
	int nfamilies = 0;
	if (FcInit()) {
#ifdef WIN32
		{
			//std::string yourFontFilePath = "seguiemj.ttf";
			std::string yourFontFilePath = "C:\\Windows\\Fonts\\seguiemj.ttf";
			const FcChar8* file = (const FcChar8*)yourFontFilePath.c_str();
			FcBool fontAddStatus = FcConfigAppFontAddFile(FcConfigGetCurrent(), file);
		}
		if (0)
		{
			CoInitialize(nullptr);
			IDWriteFactory* pDWriteFactory = nullptr;
			HRESULT hr = DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory),
				reinterpret_cast<IUnknown**>(&pDWriteFactory)
			);
			IDWriteFontCollection* pFontCollection = nullptr;
			hr = pDWriteFactory->GetSystemFontCollection(&pFontCollection);
			UINT32 familyCount = pFontCollection->GetFontFamilyCount();
			std::vector<std::wstring> fns;
			std::map<std::wstring, std::vector<std::wstring>> fpath;
			std::wstring name;
			for (UINT32 i = 0; i < familyCount; ++i)
			{
				IDWriteFontFamily* pFontFamily = nullptr;
				hr = pFontCollection->GetFontFamily(i, &pFontFamily);
				if (SUCCEEDED(hr))
				{
					// 获取字体家族名称 
					IDWriteLocalizedStrings* pFamilyNames = nullptr;
					hr = pFontFamily->GetFamilyNames(&pFamilyNames);
					UINT32 fontCount = pFontFamily->GetFontCount();
					if (SUCCEEDED(hr))
					{
						UINT32 index = 0;
						BOOL exists = FALSE;
						UINT32 length = 0;
						if (i == 30)
							length = 0;
						hr = pFamilyNames->FindLocaleName(L"zh-cn", &index, &exists); // 查找中文名称
						if (exists)
						{
							pFamilyNames->GetStringLength(index, &length);
							name.resize(length);
							length++;
							pFamilyNames->GetString(index, name.data(), length);
							//wprintf(L"字体家族名称: %s\n", name);
						}
						else
						{
							pFamilyNames->GetStringLength(0, &length);
							name.resize(length);
							length++;
							pFamilyNames->GetString(0, name.data(), length); // 默认语言 
						}
						fns.push_back(name);
						pFamilyNames->Release();
					}
					for (UINT32 j = 0; j < fontCount; ++j) {
						IDWriteFont* pFont = nullptr;
						hr = pFontFamily->GetFont(j, &pFont);
						if (SUCCEEDED(hr)) {
							DWRITE_FONT_STYLE style = pFont->GetStyle(); // 获取样式（如Italic）
							DWRITE_FONT_WEIGHT weight = pFont->GetWeight(); // 获取粗细（如Bold）
							// 获取字体文件路径 
							std::wstring filePath = GetFontFilePath(pFont);
							if (!filePath.empty()) {
								fpath[name].push_back(filePath);
							}
							pFont->Release();
						}
					}

					pFontFamily->Release();
				}
			}
			if (pFontCollection) pFontCollection->Release();
			if (pDWriteFactory) pDWriteFactory->Release();
			CoUninitialize();
		}
		//{
		//	std::string yourFontFilePath = "Noto-COLRv1-emojicompat.ttf";
		//	const FcChar8* file = (const FcChar8*)yourFontFilePath.c_str();
		//	FcBool fontAddStatus = FcConfigAppFontAddFile(FcConfigGetCurrent(), file);
		//}
#endif
		FcPattern* pat = ::FcPatternCreate();
		FcObjectSet* os = ::FcObjectSetBuild(FC_FILE, FC_FULLNAME, FC_FAMILY, FC_STYLE, FC_CHARSET, FC_WIDTH, FC_LANG, (char*)0);
		FcFontSet* fs = ::FcFontList(0, pat, os);
		for (size_t i = 0; i < fs->nfont; ++i) {
			FcPattern* font = fs->fonts[i];
			FcChar8* family = nullptr;
			FcChar8* family0 = nullptr;
			FcChar8* fp = nullptr;
			FcChar8* style = nullptr;
			auto lang = get_pat_int(font, FC_LANG);
			auto w = get_pat_int(font, FC_WIDTH);
			auto ct = get_pat_str(font, FC_CHARSET, 0);
			auto fullname = get_pat_str(font, FC_FULLNAME, 0);
			auto fph = get_pat_str(font, FC_FILE, 0);

			if (fph.size())
			{
				auto ttf = fph.rfind(".ttf");
				auto ttc = fph.rfind(".ttc");
				auto otf = fph.rfind(".otf");
				if (ttf != std::string::npos || ttc != std::string::npos || otf != std::string::npos) {

					auto family = get_pat_str(font, FC_FAMILY, 0);
					auto familys = get_pat_strs(font, FC_FAMILY);
					auto fullnames = get_pat_strs(font, FC_FULLNAME);
					auto style = get_pat_str(font, FC_STYLE, 0);
					if (family.size() && style.size())
					{
						auto& kt = fyv[family];
#ifdef _WIN32
						fph = md::replace_s(fph, "/", "\\");
						do
						{
							if (fph.find("\\\\") == std::string::npos)break;
							fph = md::replace_s(fph, "\\\\", "\\");
						} while (true);
#else
						fph = md::replace_s(fph, "\\", "/");
						do
						{
							if (fph.find("//") == std::string::npos)break;
							fph = md::replace_s(fph, "//", "/");
						} while (true);
#endif // _WIN32
						int ic = 0;
						for (size_t m = 0; m < kt.style.size(); m++)
						{
							auto fpath = kt.fpath[m];
							auto stt = kt.style[m];
							if (stt == style && fpath == fph)
							{
								ic++;
							}
						}
						if (family == "Segoe UI Emoji")
							family = family;
						if (family == "NSimSun")
							family = family;
						if (ic == 0)
						{
							kt.fpath.push_back(fph);
							kt.family = family;
							for (auto it : familys)
								kt.alias.insert(it);
							if (kt.fullname.empty())
								kt.fullname = fullname;
							kt.style.push_back(style);
						}
					}
				}
			}
		}

		FcObjectSetDestroy(os);
		FcPatternDestroy(pat);
		FcFontSetDestroy(fs);

		FcFini();
	}
	// 删除空字体
	auto newfn = fyv;
	for (auto& [k, v] : fyv) {
		if (v.family.empty() || v.fpath.empty())
		{
			newfn.erase(k); r++;
		}
	}
	return newfn;
}
// 枚举字体名称/路径名
//std::map<std::string, fontns> get_all_font()
//{
//	static std::map<std::string, fontns> fyv = get_allfont();
//	return fyv;
//}
struct font_impl;
class path_v;
class info_one;
class fd_info0;


class font_imp
{
public:
	// add font string
	std::string addstr = {};
	struct mem_ft
	{
		char* data;
		std::set<std::string> vname;
	};
private:
	std::vector<font_t*> fonts;		// 字体
	std::vector<fd_info0*> fd_data;	// 字体数据
	std::vector<mem_ft> fd_data_m;	// 字体数据copy
	std::map<std::string, font_t*>	mk;	// 字体名搜索
public:
	font_imp();
	~font_imp();
	std::vector<font_t*> add_font_file(const std::string& fn, std::vector<std::string>* pname);
	std::vector<font_t*> add_font_mem(const char* data, size_t len, bool iscp, std::vector<std::string>* pname, int* rc);

	int get_font_names(std::vector<std::string>* pname);
	const char* get_font_names(const char* sp);
	font_t* get_font(size_t idx);
	size_t size();
	void free_ft(const std::string& name);
	void free_ftp(font_t* p);
private:
};


#if 1
class UTF16 {
public:
	static int toUCS4(const unsigned short* utf16, unsigned short* ucs4);
	static int toUTF8(const unsigned short* utf16, unsigned char* utf8);
	static int toUTF8(const unsigned short* utf16, int n, unsigned char* utf8);
};

using namespace std;

int UTF16::toUCS4(const unsigned short* utf16, unsigned short* ucs4)
{
	if (utf16[0] >= 0xd800 && utf16[0] <= 0xdfff)
	{
		if (utf16[0] < 0xdc00)
		{
			if (utf16[1] >= 0xdc00 && utf16[1] <= 0xdfff)
			{
				ucs4[1] = (utf16[0] & 0x3ff);
				ucs4[0] = (utf16[1] & 0x3ff);
				ucs4[0] = ((ucs4[1] << 10) | ucs4[0]);
				ucs4[1] = ((ucs4[1] >> 6) | 1);

				//printf("%04x\n", ucs4[0]);
				//printf("%04x\n", ucs4[1]);

				return 2;
			}

			return -1;
		}

		return -1;
	}
	else
	{
		ucs4[0] = utf16[0];
		ucs4[1] = 0x00;
	}

	return 1;
}

int UTF16::toUTF8(const unsigned short* utf16, unsigned char* utf8)
{
	unsigned short ucs4[2];
	uint32_t* u = (uint32_t*)ucs4;
	int w;

	if (utf16[0] >= 0xd800 && utf16[0] <= 0xdfff)
	{
		if (utf16[0] < 0xdc00)
		{
			if (utf16[1] >= 0xdc00 && utf16[1] <= 0xdfff)
			{
				ucs4[1] = (utf16[0] & 0x3ff);
				ucs4[0] = (utf16[1] & 0x3ff);
				ucs4[0] = ((ucs4[1] << 10) | ucs4[0]);
				ucs4[1] = ((ucs4[1] >> 6) | 1);
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}
	else
	{
		ucs4[0] = utf16[0];
		ucs4[1] = 0x00;
	}

	w = *u;

	if (w <= 0x0000007f)
	{
		/*U-00000000 - U-0000007F:  0xxxxxxx*/
		utf8[0] = (w & 0x7f);

		return 1;
	}
	else if (w >= 0x00000080 && w <= 0x000007ff)
	{
		/*U-00000080 - U-000007FF:  110xxxxx 10xxxxxx*/
		utf8[1] = (w & 0x3f) | 0x80;
		utf8[0] = ((w >> 6) & 0x1f) | 0xc0;

		return 2;
	}
	else if (w >= 0x00000800 && w <= 0x0000ffff)
	{
		/*U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx*/
		utf8[2] = (w & 0x3f) | 0x80;
		utf8[1] = ((w >> 6) & 0x3f) | 0x80;
		utf8[0] = ((w >> 12) & 0x0f) | 0xe0;

		return 3;
	}
	else if (w >= 0x00010000 && w <= 0x001fffff)
	{
		/*U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx*/
		utf8[3] = (w & 0x3f) | 0x80;
		utf8[2] = ((w >> 6) & 0x3f) | 0x80;
		utf8[1] = ((w >> 12) & 0x3f) | 0x80;
		utf8[0] = ((w >> 18) & 0x07) | 0xf0;

		return 4;
	}
	else if (w >= 0x00200000 && w <= 0x03ffffff)
	{
		/*U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx*/
		utf8[4] = (w & 0x3f) | 0x80;
		utf8[3] = ((w >> 6) & 0x3f) | 0x80;
		utf8[2] = ((w >> 12) & 0x3f) | 0x80;
		utf8[1] = ((w >> 18) & 0x3f) | 0x80;
		utf8[0] = ((w >> 24) & 0x03) | 0xf8;

		return 5;
	}
	else if (w >= 0x04000000 && w <= 0x7fffffff)
	{
		/*U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx*/
		utf8[5] = (w & 0x3f) | 0x80;
		utf8[4] = ((w >> 6) & 0x3f) | 0x80;
		utf8[3] = ((w >> 12) & 0x3f) | 0x80;
		utf8[2] = ((w >> 18) & 0x3f) | 0x80;
		utf8[1] = ((w >> 24) & 0x03) | 0xf8;
		utf8[0] = ((w >> 30) & 0x01) | 0xfc;

		return 6;
	}

	return 0;
}

int UTF16::toUTF8(const unsigned short* utf16, int n, unsigned char* utf8)
{
	unsigned short ucs4[2];
	uint32_t* u = (uint32_t*)ucs4;
	int w;
	int m = 0;
	int e = 0;
	int i = 0;
	int j = 0;

	for (i = 0; i < n; i += m)
	{
		if (utf16[i] >= 0xd800 && utf16[i] <= 0xdfff)
		{
			if (utf16[i] < 0xdc00)
			{
				if (utf16[i + 1] >= 0xdc00 && utf16[i + 1] <= 0xdfff)
				{
					ucs4[1] = (utf16[i + 0] & 0x3ff);
					ucs4[0] = (utf16[i + 1] & 0x3ff);
					ucs4[0] = ((ucs4[1] << 10) | ucs4[0]);
					ucs4[1] = ((ucs4[1] >> 6) | 1);

					m = 2;
				}
				else
				{
					m = -1;
				}
			}
			else
			{
				m = -1;
			}
		}
		else
		{
			ucs4[0] = utf16[i];
			ucs4[1] = 0x00;

			m = 1;
		}

		if (m == -1)
		{
			utf8[j] = 0x00;

			return j;
		}

		w = *u;

		e = 0;

		if (w <= 0x0000007f)
		{
			/*U-00000000 - U-0000007F:  0xxxxxxx*/
			utf8[j + 0] = (w & 0x7f);

			e = 1;
		}
		else if (w >= 0x00000080 && w <= 0x000007ff)
		{
			/*U-00000080 - U-000007FF:  110xxxxx 10xxxxxx*/
			utf8[j + 1] = (w & 0x3f) | 0x80;
			utf8[j + 0] = ((w >> 6) & 0x1f) | 0xc0;

			e = 2;
		}
		else if (w >= 0x00000800 && w <= 0x0000ffff)
		{
			/*U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx*/
			utf8[j + 2] = (w & 0x3f) | 0x80;
			utf8[j + 1] = ((w >> 6) & 0x3f) | 0x80;
			utf8[j + 0] = ((w >> 12) & 0x0f) | 0xe0;

			e = 3;
		}
		else if (w >= 0x00010000 && w <= 0x001fffff)
		{
			/*U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx*/
			utf8[j + 3] = (w & 0x3f) | 0x80;
			utf8[j + 2] = ((w >> 6) & 0x3f) | 0x80;
			utf8[j + 1] = ((w >> 12) & 0x3f) | 0x80;
			utf8[j + 0] = ((w >> 18) & 0x07) | 0xf0;

			e = 4;
		}
		else if (w >= 0x00200000 && w <= 0x03ffffff)
		{
			/*U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx*/
			utf8[j + 4] = (w & 0x3f) | 0x80;
			utf8[j + 3] = ((w >> 6) & 0x3f) | 0x80;
			utf8[j + 2] = ((w >> 12) & 0x3f) | 0x80;
			utf8[j + 1] = ((w >> 18) & 0x3f) | 0x80;
			utf8[j + 0] = ((w >> 24) & 0x03) | 0xf8;

			e = 5;
		}
		else if (w >= 0x04000000 && w <= 0x7fffffff)
		{
			/*U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx*/
			utf8[j + 5] = (w & 0x3f) | 0x80;
			utf8[j + 4] = ((w >> 6) & 0x3f) | 0x80;
			utf8[j + 3] = ((w >> 12) & 0x3f) | 0x80;
			utf8[j + 2] = ((w >> 18) & 0x3f) | 0x80;
			utf8[j + 1] = ((w >> 24) & 0x03) | 0xf8;
			utf8[j + 0] = ((w >> 30) & 0x01) | 0xfc;

			e = 6;
		}

		j += e;
	}

	utf8[j] = 0x00;

	return j;
}


info_one::info_one(int platform, int encoding, int language, int nameid, const char* name, int len)
{
	platform_id = platform;
	encoding_id = encoding;
	language_id = language;
	name_id = nameid;
	length_ = len;
	std::wstring w;
	size_t wlen = length_ / 2;
	char* temp = (char*)name;
	if (*temp)
	{
		name_a.assign(temp, length_);
	}
	ndata.assign(temp, length_);
	auto ss = length_ / 2;

	name_.assign((wchar_t*)name, length_ / 2);
}
char* att_ushort(char* p)
{
	std::swap(p[0], p[1]);
	return p + 2;
}

std::string info_one::get_name()
{
	std::string n, gbkstr, u8str, wstr;
	if (encoding_id)
	{
		auto sws = ndata.size() * 0.5;
		std::string nd(ndata.c_str(), ndata.size());
		char* temp = (char*)nd.data();
		for (int i = 0; i < sws; ++i)
		{
			temp = att_ushort(temp);
		}
		u8str.resize(ndata.size());
		UTF16::toUTF8((uint16_t*)nd.c_str(), (int)sws, (uint8_t*)u8str.data());

		n = u8str.c_str();
		switch (platform_id)
		{
		case 0:
			// unicode
			break;
		case 1:
			// Macintosh
			break;
		case 2:
			// (reserved; do not use)
			break;
		case 3:
			// Microsoft
			if (encoding_id == 3 && language_id == 2052)
			{
				n = "";
				char* tw = (char*)ndata.data();
				tw++;
				int len = length_ / 2;
				for (size_t i = 0; i < len; i++, tw += 2)
				{
					auto it = *tw;
					n.push_back(it);
				}
				//n = gbk_u8(n);
			}
			break;
		default:
			break;
		}
	}
	else {
		n = name_a.c_str();
	}
	return n;
}
info_one::~info_one() {}


#if 1
#define FONS_NOTUSED(v)  (void)sizeof(v)
typedef uint8_t byte;
typedef int32_t Fixed;
#define FWord int16_t
#define uFWord uint16_t
#define Card8 uint8_t
#define Card16 uint16_t
#define Card32 uint32_t

#define head_VERSION VERSION(1, 0)

#define DATE_TIME_SIZE 8
typedef Card8 longDateTime[DATE_TIME_SIZE];

/* TrueType Collection Header */
typedef struct
{
	Card32 TTCTag;
	Fixed Version;
	Card32 DirectoryCount;
	Card32* TableDirectory; /* [DirectoryCount] */
	Card32 DSIGTag;
	Card32 DSIGLength;
	Card32 DSIGOffset;
} ttcfTbl;
typedef struct
{
	Fixed	version;//	0x00010000 if (version 1.0)
	Fixed	fontRevision;//	set by font manufacturer
	uint32_t	checkSumAdjustment;//	To compute : set it to 0, calculate the checksum for the 'head' table and put it in the table directory, sum the entire font as a uint32_t, then store 0xB1B0AFBA - sum. (The checksum for the 'head' table will be wrong as a result.That is OK; do not reset it.)
	uint32_t	magicNumber;//	set to 0x5F0F3CF5
	uint16_t	flags;/*	bit 0 - y value of 0 specifies baseline
	bit 1 - x position of left most black bit is LSB
	bit 2 - scaled point size and actual point size will differ(i.e. 24 point glyph differs from 12 point glyph scaled by factor of 2)
	bit 3 - use integer scaling instead of fractional
	bit 4 - (used by the Microsoft implementation of the TrueType scaler)
	bit 5 - This bit should be set in fonts that are intended to e laid out vertically, and in which the glyphs have been drawn such that an x - coordinate of 0 corresponds to the desired vertical baseline.
	bit 6 - This bit must be set to zero.
	bit 7 - This bit should be set if the font requires layout for correct linguistic rendering(e.g.Arabic fonts).
	bit 8 - This bit should be set for an AAT font which has one or more metamorphosis effects designated as happening by default.
	bit 9 - This bit should be set if the font contains any strong right - to - left glyphs.
	bit 10 - This bit should be set if the font contains Indic - style rearrangement effects.
	bits 11 - 13 - Defined by Adobe.
	bit 14 - This bit should be set if the glyphs in the font are simply generic symbols for code point ranges, such as for a last resort font.
	*/
	uint16_t	unitsPerEm;//	range from 64 to 16384
	longDateTime	created;//	international date
	longDateTime	modified;//	international date
	FWord	xMin;//	for all glyph bounding boxes
	FWord	yMin;//	for all glyph bounding boxes
	FWord	xMax;//	for all glyph bounding boxes
	FWord	yMax;//	for all glyph bounding boxes
	uint16_t	macStyle;/*	bit 0 bold
	bit 1 italic
	bit 2 underline
	bit 3 outline
	bit 4 shadow
	bit 5 condensed(narrow)
	bit 6 extended*/
	uint16_t	lowestRecPPEM;//	smallest readable size in pixels
	int16_t	fontDirectionHint;/*	0 Mixed directional glyphs
	1 Only strongly left to right glyphs
	2 Like 1 but also contains neutrals
	- 1 Only strongly right to left glyphs
	- 2 Like - 1 but also contains neutrals*/
	int16_t	indexToLocFormat;//	0 for short offsets, 1 for long
	int16_t	glyphDataFormat;//	0 for current format

}head_table;
// Entry
typedef struct sfnt_header_
{
	uint32_t tag = 0;
	uint32_t checksum;
	uint32_t offset; //From beginning of header.
	uint32_t logicalLength;
}sfnt_header;
typedef struct
{
	Fixed version;
	Card16 numTables;
	Card16 searchRange;
	Card16 entrySelector;
	Card16 rangeShift;
	sfnt_header* directory; /* [numTables] */
} sfntTbl;
typedef struct post_header_
{
	int32_t	format;//	Format of this table
	int32_t	italicAngle;	//Italic angle in degrees
	int16_t	underlinePosition;	//Underline position
	int16_t	underlineThickness;	//Underline thickness
	uint32_t	isFixedPitch;	//Font is monospaced; set to 1 if the font is monospaced and 0 otherwise(N.B., to maintain compatibility with older versions of the TrueType spec, accept any non - zero value as meaning that the font is monospaced)
	uint32_t	minMemType42;	//Minimum memory usage when a TrueType font is downloaded as a Type 42 font
	uint32_t	maxMemType42;	//Maximum memory usage when a TrueType font is downloaded as a Type 42 font
	uint32_t	minMemType1;	//Minimum memory usage when a TrueType font is downloaded as a Type 1 font
	uint32_t	maxMemType1;
}post_header;
typedef struct
{
	Fixed version;
	FWord ascender;
	FWord descender;
	FWord lineGap;
	uFWord advanceWidthMax;
	FWord minLeftSideBearing;
	FWord minRightSideBearing;
	FWord xMaxExtent;
	int16_t caretSlopeRise;
	int16_t caretSlopeRun;
	int16_t caretOffset;
	int16_t reserved[4];
	int16_t metricDataFormat;
	uint16_t numberOfLongHorMetrics;
} hheaTbl;


typedef struct {
	uint16_t format;
	uint16_t length;
	uint16_t language;
	unsigned char glyphId[256];
} Format0;
#define FORMAT0_SIZE (uint16 * 3 + uint8 * 256)
/*
Format 2: High - byte mapping through table
*/
typedef struct {
	unsigned short firstCode;
	unsigned short entryCount;
	short idDelta;
	unsigned short idRangeOffset;
} Segment2;
#define SEGMENT2_SIZE (uint16 * 3 + int16 * 1)

typedef struct {
	unsigned short format;
	unsigned short length;
	unsigned short language;
	unsigned short segmentKeys[256];
	Segment2* segment;
	unsigned short* glyphId;
} Format2;
// ------------------------------------------------------------------------------------------------
struct Bitmap_p
{
	uint32_t    rows;
	uint32_t    width;
	int             pitch;
	float			advance;
	float			bearingX;
	float			bearingY;
	int             bit_depth;
	unsigned char* buffer;
	uint32_t	capacity = 0;
	unsigned short  num_grays;
	unsigned char   pixel_mode;
	unsigned char   lcd_mode;
	void* data = 0;
	unsigned char   palette_mode;
	void* palette;
	int x, y;
};

union ft_key_s
{
	uint64_t u = 0;
	struct
	{
		char32_t unicode_codepoint;
		unsigned short font_dpi;
		// 字号支持 1-255
		unsigned char font_size;
		// 模糊大小支持 0-255
		unsigned char blur_size;
		//unsigned char is_bitmap;
	}v;
};
union ft_char_s
{
	uint64_t u = 0;
	struct
	{
		char32_t unicode_codepoint;
		unsigned short font_dpi;
		// 字号支持 1-255
		unsigned char font_size;
	}v;
};

typedef enum  _Pixel_Mode_
{
	PX_NONE = 0,
	PX_MONO,
	PX_GRAY,
	PX_GRAY2,
	PX_GRAY4,
	PX_LCD,
	PX_LCD_V,
	PX_BGRA,

	PX_MAX      /* do not remove */

} Pixel_Mode;

typedef enum STT_
{
	TYPE_NONE = 0,
	TYPE_EBLC, /* `EBLC' (Microsoft), */
	/* `bloc' (Apple)      */
	TYPE_CBLC, /* `CBLC' (Google)     */
	TYPE_SBIX, /* `sbix' (Apple)      */
	/* do not remove */
	TYPE_MAX
} sbit_table_type;

#define DL_long(v) v=ttLONG(data);data+=4
#define DL_short(v) v=ttSHORT(data);data+=2
#define DL_ulong(v) v=ttULONG(data);data+=4
#define DL_ushort(v) v=ttUSHORT(data);data+=2

#define FONS_INVALID -1

enum FONSflags {
	FONS_ZERO_TOPLEFT = 1,
	FONS_ZERO_BOTTOMLEFT = 2,
};

enum FONSalign {
	// Horizontal align
	FONS_ALIGN_LEFT = 1 << 0,	// Default
	FONS_ALIGN_CENTER = 1 << 1,
	FONS_ALIGN_RIGHT = 1 << 2,
	// Vertical align
	FONS_ALIGN_TOP = 1 << 3,
	FONS_ALIGN_MIDDLE = 1 << 4,
	FONS_ALIGN_BOTTOM = 1 << 5,
	FONS_ALIGN_BASELINE = 1 << 6, // Default
};

enum FONSglyphBitmap {
	FONS_GLYPH_BITMAP_OPTIONAL = 1,
	FONS_GLYPH_BITMAP_REQUIRED = 2,
};

enum FONSerrorCode {
	// Font atlas is full.
	FONS_ATLAS_FULL = 1,
	// Scratch memory used to render glyphs is full, requested size reported in 'val', you may need to bump up FONS_SCRATCH_BUF_SIZE.
	FONS_SCRATCH_FULL = 2,
	// Calls to fonsPushState has created too large stack, if you need deep state stack bump up FONS_MAX_STATES.
	FONS_STATES_OVERFLOW = 3,
	// Trying to pop too many states fonsPopState().
	FONS_STATES_UNDERFLOW = 4,
};

#endif

struct meta_tag
{
	std::string tag;
	std::string v;
};


struct font_impl :public stbtt_fontinfo
{

	int colr = -1, cpal = -1; // table locations as offset from start of .ttf

	std::map<std::string, sfnt_header> _tb;
	// EBLC	Embedded bitmap location data	嵌入式位图位置数据
	int eblc = 0;
	uint32_t sbit_table_size = 0;
	int      sbit_table_type = 0;
	uint32_t sbit_num_strikes = 0;
	// EBDT	Embedded bitmap data	嵌入式位图数据 either `CBDT', `EBDT', or `bdat'
	uint32_t ebdt_start = 0;
	uint32_t ebdt_size = 0;
	// EBSC	Embedded bitmap scaling data	嵌入式位图缩放数据
	uint32_t ebsc = 0;
	int format = 0;
};
struct hps_t {
	head_table head;
	hheaTbl hhea;
	post_header post;
#if TTF_USE_HARFBUZZ
	hb_font_t* hb_font;
	hb_language_t hb_language;
	hb_buffer_t* hb_buffer;
#endif
};
class stb_font
{
public:
	stb_font()
	{
	}

	~stb_font()
	{
	}
	static font_impl* new_fontinfo()
	{
		return new font_impl();
	}
	static void free_fontinfo(font_impl* p)
	{
		font_impl* pf = (font_impl*)p;
		delete pf;
	}
public:

	static int init(void*)
	{
		return 1;
	}

	static int done(void*)
	{
		return 1;
	}

	static int loadFont(font_impl* font, const void* data, int idx, void* ud)
	{
		int stbError;
		FONS_NOTUSED(idx);

		font->userdata = ud;
		int fso = get_offset(data, idx);
		stbError = stbtt_InitFont(font, (unsigned char*)data, fso);
		stbError = init_table(font, (unsigned char*)data, fso);
		// 字体格式
		font->format = ttUSHORT(font->data + font->index_map + 0);
		return stbError;
	}
	static int get_numbers(const void* data)
	{
		return stbtt_GetNumberOfFonts((unsigned char*)data);
	}
	static int get_offset(const void* data, int idx)
	{
		return stbtt_GetFontOffsetForIndex((unsigned char*)data, idx);
	}
	static void getFontVMetrics(font_impl* font, int* ascent, int* descent, int* lineGap)
	{
		stbtt_GetFontVMetrics(font, ascent, descent, lineGap);
	}
	static void getFontHMetrics(font_impl* font, int glyph, int* advance, int* lsb)
	{
		stbtt_GetGlyphHMetrics(font, glyph, advance, lsb);
		return;
	}

	static float getPixelHeightScale(font_impl* font, float size)
	{
		return size < 0 ? stbtt_ScaleForPixelHeight(font, -size) : stbtt_ScaleForMappingEmToPixels(font, size);
	}

	// 获取utf8字符
	static const char* get_glyph_index_last(font_impl* font, const char* str, int* index)
	{
		uint32_t codepoint = 0;
		uint32_t utf8state = 0;
		*index = -1;
		for (; str && *str && *index == -1; ++str) {
			if (md::fons_decutf8(&utf8state, &codepoint, *(const unsigned char*)str))
				continue;
			*index = stbtt_FindGlyphIndex(font, codepoint);
		}
		return str;
	}

	static uint32_t get_u8_to_u16(const char* str)
	{
		uint32_t codepoint = 0;
		uint32_t utf8state = 0;
		for (; str && *str; ++str) {
			if (md::fons_decutf8(&utf8state, &codepoint, *(const unsigned char*)str))
				continue;
			break;
		}
		return codepoint;
	}
	static const char* get_glyph_index(font_impl* font, const char* str, int* idx)
	{
		uint32_t codepoint = 0;
		uint32_t utf8state = 0;
		int index = -1;
		for (; str && *str && index == -1; ++str) {
			if (md::fons_decutf8(&utf8state, &codepoint, *(const unsigned char*)str))
				continue;
			index = stbtt_FindGlyphIndex(font, codepoint);
		}
		*idx = index;
		return str;
	}


#define BYTE_( p, i )  ( ((const unsigned char*)(p))[(i)] )
#define BYTE_U16( p, i, s1 )  ( (uint16_t)( BYTE_( p, i ) ) << (s1) )
#define BYTE_U32( p, i, s1 )  ( (uint32_t)( BYTE_( p, i ) ) << (s1) )
#define PEEK_USHORT( p )  uint16_t( BYTE_U16( p, 0, 8 ) | BYTE_U16( p, 1, 0 ) )
#define PEEK_LONG( p )  int32_t( BYTE_U32( p, 0, 24 ) | BYTE_U32( p, 1, 16 ) | BYTE_U32( p, 2,  8 ) | BYTE_U32( p, 3,  0 ) )
#define PEEK_ULONG( p )  uint32_t(BYTE_U32( p, 0, 24 ) | BYTE_U32( p, 1, 16 ) | BYTE_U32( p, 2,  8 ) | BYTE_U32( p, 3,  0 ) )
#define PEEK_OFF3( p )  ( int32_t( BYTE_U32( p, 0, 24 ) | BYTE_U32( p, 1, 16 ) | BYTE_U32( p, 2,  8 ) ) >> 8 )
#define PEEK_UOFF3( p )  uint32_t( BYTE_U32( p, 0, 16 ) | BYTE_U32( p, 1,  8 ) | BYTE_U32( p, 2,  0 ) )
#define NEXT_CHAR( buffer ) ( (signed char)*buffer++ )

#define NEXT_BYTE( buffer ) ( (unsigned char)*buffer++ )
#define NEXT_SHORT( b ) ( (short)( b += 2, PEEK_USHORT( b - 2 ) ) )
#define NEXT_USHORT( b ) ( (unsigned short)( b += 2, PEEK_USHORT( b - 2 ) ) )
#define NEXT_LONG( buffer ) ( (long)( buffer += 4, PEEK_LONG( buffer - 4 ) ) )
#define NEXT_ULONG( buffer ) ( (unsigned long)( buffer += 4, PEEK_ULONG( buffer - 4 ) ) )
#define NEXT_OFF3( buffer ) ( buffer += 3, PEEK_OFF3( buffer - 3 ) )
#define NEXT_UOFF3( buffer ) ( buffer += 3, PEEK_UOFF3( buffer - 3 ) )

#define INT_TO_F26DOT6( x )    ( (long)(x) * 64  )    /* << 6  */
#define INT_TO_F2DOT14( x )    ( (long)(x) * 16384 )  /* << 14 */
#define INT_TO_FIXED( x )      ( (long)(x) * 65536 )  /* << 16 */
#define F2DOT14_TO_FIXED( x )  ( (long)(x) * 4 )      /* << 2  */
#define FIXED_TO_INT( x )      ( RoundFix( x ) >> 16 )
#define ROUND_F26DOT6( x )     ( ( (x) + 32 - ( x < 0 ) ) & -64 )

	int RoundFix(uint32_t a)
	{
		return ((a + (0x8000L - (a < 0)))) & ~0xFFFFL;
	}

	static unsigned char* tt_cmap2_get_subheader(unsigned char* table, uint32_t char_code)
	{
		unsigned char* result = NULL;
		if (char_code < 0x10000UL)
		{
			uint32_t   char_lo = (uint32_t)(char_code & 0xFF);
			uint32_t   char_hi = (uint32_t)(char_code >> 8);
			unsigned char* p = table + 6;    /* keys table */
			unsigned char* subs = table + 518;  /* subheaders table */
			unsigned char* sub;
			if (char_hi == 0)
			{
				/* an 8-bit character code -- we use subHeader 0 in this case */
				/* to test whether the character code is in the charmap       */
				/*                                                            */
				sub = subs;
				p += char_lo * 2;
				if (PEEK_USHORT(p) != 0)sub = 0;
			}
			else
			{
				p += char_hi * 2;
				sub = subs + (uint64_t)PEEK_USHORT(p);
			}
			result = sub;
		}
		return result;
	}

	static int tt_cmap2_char_index(unsigned char* table, uint32_t char_code)
	{
		uint32_t   result = 0;
		unsigned char* subheader;
		subheader = tt_cmap2_get_subheader(table, char_code);
		if (subheader)
		{
			unsigned char* p = subheader;
			uint32_t   idx = ((uint32_t)(char_code) & 0xFF);
			uint32_t   start, count;
			int    delta;
			uint32_t   offset;
			start = NEXT_USHORT(p);
			count = NEXT_USHORT(p);
			delta = NEXT_SHORT(p);
			offset = PEEK_USHORT(p);
			idx -= start;
			if (idx < count && offset != 0)
			{
				p += offset + 2 * idx;
				idx = PEEK_USHORT(p);
				if (idx != 0)
					result = (uint32_t)((int)idx + delta) & 0xFFFFU;
			}
		}
		return result;
	}
	static int get_ext_glyph_index(const stbtt_fontinfo* info, uint32_t codepoint)
	{
		int ret = 0;
		// todo
		return ret;
	}
	// GBK字符串
	static int get_glyph_index2(font_impl* font, const char* t)
	{
		const stbtt_fontinfo* info = font;
		int ret = 0;
		uint8_t* data = info->data;
		uint32_t index_map = info->index_map;
		uint16_t format = ttUSHORT(data + index_map + 0);
		uint32_t codepoint = 0;
		if (format == 2) {
			// @TODO: high-byte mapping for japanese/chinese/korean
			codepoint = (uint32_t)t[0] & 0xff;
			if (codepoint > 127)
			{
				codepoint <<= 8;
				codepoint |= (uint32_t)t[1] & 0xff;
			}
			ret = tt_cmap2_char_index(data + index_map, codepoint);
		}
		return ret;
	}

	static int getGlyphIndex(font_impl* font, int codepoint)
	{
		return font->format == 2 ? get_ext_glyph_index(font, codepoint) : stbtt_FindGlyphIndex(font, codepoint);
	}

	static int buildGlyphBitmap(font_impl* font, int glyph, float scale,
		int* advance, int* lsb, int* x0, int* y0, int* x1, int* y1)
	{
		stbtt_GetGlyphHMetrics(font, glyph, advance, lsb);
		stbtt_GetGlyphBitmapBox(font, glyph, scale, scale, x0, y0, x1, y1);
		return 1;
	}

	static void renderGlyphBitmap(font_impl* font, unsigned char* output, int outWidth, int outHeight, int outStride,
		float scaleX, float scaleY, int glyph)
	{
		stbtt_MakeGlyphBitmap(font, output, outWidth, outHeight, outStride, scaleX, scaleY, glyph);
	}

	static int getGlyphKernAdvance(font_impl* font, int glyph1, int glyph2)
	{
		return stbtt_GetGlyphKernAdvance(font, glyph1, glyph2);
	}
	// 
	static int getKernAdvanceCH(font_impl* font, int ch1, int ch2)
	{
		return stbtt_GetCodepointKernAdvance(font, ch1, ch2);
	}
	static void get_head(font_impl* font, head_table* p)
	{
		if (font && p)
		{
			auto info = font;
			auto data = info->data + info->head;
			DL_long(p->version);
			DL_long(p->fontRevision);
			DL_long(p->checkSumAdjustment);
			DL_long(p->magicNumber);
			DL_ushort(p->flags);
			DL_ushort(p->unitsPerEm);
			int sldt = sizeof(longDateTime);
			memcpy(p->created, data, sldt); data += sldt;
			memcpy(p->modified, data, sldt); data += sldt;
			auto pxy = info->data + info->head + 36;
			DL_ushort(p->xMin);
			DL_ushort(p->yMin);
			DL_ushort(p->xMax);
			DL_ushort(p->yMax);
			DL_ushort(p->macStyle);
			DL_ushort(p->lowestRecPPEM);
			DL_short(p->fontDirectionHint);
			DL_short(p->indexToLocFormat);
			DL_short(p->glyphDataFormat);
		}
	}
	static void get_hhea(font_impl* font, hheaTbl* hhea)
	{
		if (font && hhea)
		{
			auto info = font;
			auto data = info->data + info->hhea;

			DL_long(hhea->version);
			DL_short(hhea->ascender);
			DL_short(hhea->descender);
			DL_short(hhea->lineGap);
			DL_ushort(hhea->advanceWidthMax);
			DL_short(hhea->minLeftSideBearing);
			DL_short(hhea->minRightSideBearing);
			DL_short(hhea->xMaxExtent);
			DL_short(hhea->caretSlopeRise);
			DL_short(hhea->caretSlopeRun);
			DL_short(hhea->caretOffset);
			DL_short(hhea->reserved[0]);
			DL_short(hhea->reserved[1]);
			DL_short(hhea->reserved[2]);
			DL_short(hhea->reserved[3]);
			DL_short(hhea->metricDataFormat);
			DL_ushort(hhea->numberOfLongHorMetrics);
		}
	}
	static glm::ivec4 get_bounding_box(font_impl* font)
	{
		int x0 = 0, y0 = 0, x1, y1; // =0 suppresses compiler warning
		stbtt_GetFontBoundingBox(font, &x0, &y0, &x1, &y1);
		return glm::ivec4(x0, y0, x1, y1);
	}
	static glm::ivec2 get_codepoint_hmetrics(font_impl* font, int ch)
	{
		int advance = 0, lsb = 0;
		stbtt_GetCodepointHMetrics(font, ch, &advance, &lsb);
		return glm::ivec2(advance, lsb);
	}
	static glm::ivec2 get_glyph_hmetrics(font_impl* font, int gidx)
	{
		int advancei, lsb;
		stbtt_GetGlyphHMetrics(font, gidx, &advancei, &lsb);
		return glm::ivec2(advancei, lsb);
	}
	static glm::ivec4 get_glyph_box(font_impl* font, int gidx, double scale, double shift_x = .0, double shift_y = 0.0f)
	{
		int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
		stbtt_GetGlyphBitmapBoxSubpixel(font, gidx, scale, scale, shift_x, shift_y, &x0, &y0, &x1, &y1);
		return glm::ivec4(x0, y0, x1, y1);
	}
	static void stbtt_MakeGlyphBitmapSubpixel0(const stbtt_fontinfo* info, unsigned char* output, int out_w, int out_h, int out_stride, float scale_x, float scale_y, float shift_x, float shift_y, int glyph, int xf)
	{
		int ix0, iy0, x1, y1;
		stbtt_vertex* vertices;
		int num_verts = stbtt_GetGlyphShape(info, glyph, &vertices);
		stbtt__bitmap gbm;

		stbtt_GetGlyphBitmapBoxSubpixel(info, glyph, scale_x, scale_y, shift_x, shift_y, &ix0, &iy0, &x1, &y1);
		gbm.pixels = output;
		gbm.w = out_w;
		gbm.h = out_h;
		gbm.stride = out_stride;
		ix0 += xf;	// 修正某些抗锯齿裁剪
		if (gbm.w && gbm.h)
			stbtt_Rasterize(&gbm, 0.35f, vertices, num_verts, scale_x, scale_y, 0, 0, ix0, iy0, 1, info->userdata);

		free(vertices);
	}

	static char* get_glyph_bitmap(font_impl* font, int gidx, double scale, glm::ivec4* ot, std::vector<char>* out, glm::vec3* adlsb, glm::vec2 lcd = { 1, 1 }, int xf = 0)
	{
		int ascent, descent, linegap;
		int x0 = 0, y0 = 0, x1, y1;
		stbtt_GetFontVMetrics(font, &ascent, &descent, &linegap);
		int advancei, lsb;
		stbtt_GetGlyphHMetrics(font, gidx, &advancei, &lsb);
		double adv = advancei * scale;
		double bearing = lsb * scale;
		float shift_x = .0, shift_y = 0.0f;
		stbtt_GetGlyphBitmapBoxSubpixel(font, gidx, scale * lcd.x, scale * lcd.y, shift_x, shift_y, &x0, &y0, &x1, &y1);
		glm::ivec4 ot0 = {};
		if (!ot)ot = &ot0;
		adlsb->z = adv;
		adlsb->x = bearing;
		ot->x = x0;
		ot->y = y0;
		//if (!out)
		{
			ot->z = x1 - x0;
			ot->w = y1 - y0;
		}
		size_t pcs = (int64_t)ot->z * ot->w;
		char* pxs = 0;
		if (out)
		{
			if (out->size() < pcs)
			{
				out->resize(pcs);
			}
			pxs = out->data();
			memset(pxs, 0, out->size());
			if (xf == 0)
				stbtt_MakeGlyphBitmapSubpixel(font, (unsigned char*)pxs, ot->z, ot->w, ot->z, scale * lcd.x, scale * lcd.y, shift_x, shift_y, gidx);
			else
				stbtt_MakeGlyphBitmapSubpixel0(font, (unsigned char*)pxs, ot->z, ot->w, ot->z, scale * lcd.x, scale * lcd.y, shift_x, shift_y, gidx, xf);
		}

		return pxs;
	}
public:
	static std::string get_font_name(font_impl* font, std::map<int, std::vector<info_one>>* m)
	{
		int len = 0;
		auto str = getFontNameString(font, m);
		return str ? str : "";
	}
#if 1

	static uint16_t ttUSHORT(uint8_t* p) { return p[0] * 256 + p[1]; }
	static int16_t ttSHORT(uint8_t* p) { return p[0] * 256 + p[1]; }
	static uint32_t ttULONG(uint8_t* p) { return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3]; }
	static int ttLONG(uint8_t* p) { return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3]; }
#ifndef stbtt_tag4
#define stbtt_tag4(p,c0,c1,c2,c3) ((p)[0] == (c0) && (p)[1] == (c1) && (p)[2] == (c2) && (p)[3] == (c3))
#define stbtt_tag(p,str)           stbtt_tag4(p,str[0],str[1],str[2],str[3])
#endif // !stbtt_tag4

	// @OPTIMIZE: binary search
	static uint32_t find_table(uint8_t* data, uint32_t fontstart, const char* tag)
	{
		int num_tables = ttUSHORT(data + fontstart + 4);
		uint32_t tabledir = fontstart + 12;
		int i;
		char* t = 0;
		for (i = 0; i < num_tables; ++i) {
			uint32_t loc = tabledir + 16 * i;
			t = (char*)data + loc + 0;
			if (stbtt_tag(data + loc + 0, tag))
				return ttULONG(data + loc + 8);
		}
		return 0;
	}
#endif // !1
	static void enum_table(uint8_t* data, uint32_t fontstart, std::map<std::string, sfnt_header>& out)
	{
		char buf[128];
		memcpy(buf, data + fontstart, 128);
		int num_tables = ttUSHORT(data + fontstart + 4);
		memcpy(buf, data + fontstart + 4, 4);
		uint32_t tabledir = fontstart + 12;
		int i;
		char* t = 0;
		std::string n;
		sfnt_header sh;
		if (num_tables > 0)
		{
			for (i = 0; i < num_tables; ++i) {
				uint32_t loc = tabledir + 16 * i;
				t = (char*)data + loc + 0;
				memcpy(buf, t, 128);
				n.assign(t, 4);
				sh.tag = ttULONG(data + loc + 0);
				sh.checksum = ttULONG(data + loc + 4);
				sh.offset = ttULONG(data + loc + 8);
				sh.logicalLength = ttULONG(data + loc + 12);
				out[n] = sh; //{sh.tag, sh.checksum, sh.offset, sh.logicalLength};
			}
		}
	}

	// 获取字体信息
	static const char* getFontNameString(const stbtt_fontinfo* font, std::map<int, std::vector<info_one>>* m)
	{
		int i, count, stringOffset;
		uint8_t* fc = font->data;
		uint32_t offset = font->fontstart;
		uint32_t nm = find_table(fc, offset, "name");
		if (!nm || !m) return 0;
		std::map<int, std::vector<info_one>>& m1 = *m;
		count = ttUSHORT(fc + nm + 2);
		stringOffset = nm + ttUSHORT(fc + nm + 4);
		for (i = 0; i < count; ++i) {
			uint32_t loc = nm + 6 + 12 * i;
			int platform = ttUSHORT(fc + loc + 0);
			int encoding = ttUSHORT(fc + loc + 2);
			int language = ttUSHORT(fc + loc + 4);
			int nameid = ttUSHORT(fc + loc + 6);
			int length = ttUSHORT(fc + loc + 8);
			const char* name = (const char*)(fc + stringOffset + ttUSHORT(fc + loc + 10));
			m1[language].push_back(info_one(platform, encoding, language, nameid, name, length));
		}
		return NULL;
	}


	struct metainfo_t
	{
		uint32_t	version;//	The version of the table format, currently 1
		uint32_t	flags;//	Flags, currently unusedand set to 0
		uint32_t	dataOffset;//	Offset from the beginning of the table to the data
		uint32_t	numDataMaps;//	The number of data maps in the table
	};
	struct DataMaps_t
	{
		char tag[4];
		uint32_t	dataOffset;//	Offset from the beginning of the table to the data for this tag
		uint32_t	dataLength;//	Length of the data.The data is not required to be padded to any byte boundary.
	};
	struct metainfo_tw
	{
		uint32_t	version;//	Version number of the metadata table — set to 1.
		uint32_t	flags;//	Flags — currently unused; set to 0.
		uint32_t	reserved;//	Not used; should be set to 0.
		uint32_t	dataMapsCount;//	The number of data maps in the table.
		DataMaps_t dataMaps[1];//[dataMapsCount]	Array of data map records.
	};
	struct meta_tag_t
	{
		char tag[5];
		std::string v;
	};
	// 获取字体meta信息
	static int get_meta_string(const stbtt_fontinfo* font, std::vector<meta_tag>& mtv)
	{
		int i, count, stringOffset;
		uint8_t* fc = font->data;
		uint32_t offset = font->fontstart;
		uint32_t nm = find_table(fc, offset, "meta");
		if (!nm) return 0;
		//font_t* ttp = (font_t*)font->userdata;
		uint8_t* tp = fc + nm;
		metainfo_tw meta = {};
		uint32_t* ti = (uint32_t*)&meta;

		for (int i = 0; i < 4; i++)
			ti[i] = ttULONG(tp + i * 4);
		if (meta.dataMapsCount > 0)
		{
			auto dm = (tp + 16);
			auto dm1 = (tp);
			mtv.resize(meta.dataMapsCount);
			for (size_t i = 0; i < meta.dataMapsCount; i++)
			{
				auto& it = mtv[i];
				it.tag.assign((char*)dm, 4);
				auto offset = ttULONG(dm + 4);
				auto length = ttULONG(dm + 8);
				if (length > 0)
					it.v.assign((char*)dm1 + offset, length);
				dm += 12;
			}
		}
		return 0;
	}
#if 0
	struct gsub_h
	{
		uint16_t	majorVersion;//	Major version of the GSUB table, = 1.
		uint16_t	minorVersion;//	Minor version of the GSUB table, = 1.
		Offset16	scriptListOffset;//	Offset to ScriptList table, from beginning of GSUB table.
		Offset16	featureListOffset;//	Offset to FeatureList table, from beginning of GSUB table.
		Offset16	lookupListOffset;//	Offset to LookupList table, from beginning of GSUB table.
		Offset32	featureVariationsOffset;//	Offset to FeatureVariations table, from beginning of the GSUB table(may be NULL).
	};
	struct oth_h {
		char v[4];
		uint16_t n;
	};
	static int get_gsub_string(const stbtt_fontinfo* font)
	{
		int i, count, stringOffset;
		uint8_t* fc = font->data;
		uint32_t offset = font->fontstart;
		auto fcs = (fc + offset);
		// 获取SFNT版本标识（前4字节）
		unsigned int sfnt_version = *(unsigned int*)fcs;
		auto hp = (oth_h*)fcs;
		// 判断OpenType签名 
		// OpenType: 0x4F54544F ("OTTO")
		// TrueType: 0x00010000 ("true")
		bool ot = (sfnt_version == 0x4F54544F);
		uint32_t gsub_offset = find_table(fc, offset, "GSUB");
		uint32_t gpos_offset = find_table(fc, offset, "GPOS");
		if (gsub_offset > 0)
		{
			if (12383584 == gsub_offset)
				gsub_offset = gsub_offset;
			gsub_h gsub = {};
			auto fcn = fc + gsub_offset;
			auto gsub_table = fcn;
			gsub.majorVersion = ttUSHORT(fcn);
			fcn += sizeof(uint16_t);
			gsub.minorVersion = ttUSHORT(fcn);
			fcn += sizeof(uint16_t);
			gsub.scriptListOffset = ttUSHORT(fcn);
			fcn += sizeof(uint16_t);
			gsub.featureListOffset = ttUSHORT(fcn);
			fcn += sizeof(uint16_t);
			gsub.lookupListOffset = ttUSHORT(fcn);
			fcn += sizeof(uint16_t);
			gsub.featureVariationsOffset = ttULONG(fcn);
			// 获取LookupList 
			uint16_t lookupCount = ttUSHORT(gsub_table + gsub.lookupListOffset);
			uint16_t* lookups = (uint16_t*)(gsub_table + gsub.lookupListOffset + 2);

			for (int i = 0; i < lookupCount; ++i) {
				uint16_t lookupOffset = ttUSHORT((uint8_t*)lookups + i);
				uint16_t lookupType = ttUSHORT(gsub_table + lookupOffset);
				/*
					查找类型 1 子表：单个替换
					查找类型 2 子表：多重替换
					查找类型 3 子表：替代替换
					查找类型 4 子表：连字替换
					查找类型 5 子表：上下文替换
					查找类型 6 子表：链式上下文替换
					查找类型 7 子表：替换子表扩展
					查找类型 8 子表：反向链接上下文单个替换
				*/
				if (lookupType == 4) { // 连字替换 
					uint16_t subTableCount = ttUSHORT(gsub_table + lookupOffset + 4);
					uint16_t* subTableOffsets = (uint16_t*)(gsub_table + lookupOffset + 6);

					for (int j = 0; j < subTableCount; ++j) {
						uint16_t subTableOffset = ttUSHORT((uint8_t*)&subTableOffsets[j]);
						// 解析连字子表（格式：覆盖范围+连字集）
						uint16_t coverageOffset = ttUSHORT(gsub_table + subTableOffset);
						uint16_t ligSetCount = ttUSHORT(gsub_table + subTableOffset + 2);
						uint16_t* ligSetOffsets = (uint16_t*)(gsub_table + subTableOffset + 4);

						// 遍历连字集
						for (int k = 0; k < ligSetCount; ++k) {
							uint16_t ligatureOffset = ttUSHORT((uint8_t*)&ligSetOffsets[k]);
							uint16_t ligGlyph = ttUSHORT(gsub_table + ligatureOffset);
							uint16_t compCount = ttUSHORT(gsub_table + ligatureOffset + 2);
							uint16_t* components = (uint16_t*)(gsub_table + ligatureOffset + 4);

							// 示例：打印连字规则（如 "f" + "i" -> "ﬁ"）
							printf("Ligature: GlyphID=%d, Components=", ligGlyph);
							for (int c = 0; c < compCount; c++)
								printf("%d ", ttUSHORT((uint8_t*)&components[c]));
							printf("\n");
						}
					}
				}
			}
			return 0;
		}
		else
		{
			return 0;
		}
	}
#endif
	class eblc_h
	{
	public:
		uint16_t majorVersion = 0, minorVersion = 0;
		uint32_t numSizes = 0;
	public:
		eblc_h()
		{
		}

		~eblc_h()
		{
		}

	private:

	};
	//// 获取ebdt信息
	//static const char* get_ebdt(font_impl* font_i)
	//{
	//	const stbtt_fontinfo* font = &font_i->font;
	//	int i, count, stringOffset;
	//	uint8_t* fc = font->data;
	//	uint32_t offset = font->fontstart;
	//	uint32_t nm = get_tag(font_i, "EBDT");
	//	uint32_t eblc = get_tag(font_i, "EBLC");
	//	uint32_t ebsc = get_tag(font_i, "EBSC");
	//	if (!nm) return 0;
	//	font_t* ttp = (font_t*)font->userdata;
	//	int majorVersion = ttUSHORT(fc + nm + 0);
	//	int minorVersion = ttUSHORT(fc + nm + 2);
	//	//eblc
	//	uint32_t numSizes = ttULONG(fc + eblc + 4);
	//	char* b = (char*)fc + eblc;
	//	eblc_h* eblcp = (eblc_h*)b;

	//	return NULL;
	//}
public:
	// 获取字体轮廓
	static stbtt_vertex* get_char_shape(font_impl* font, const char* str, int& verCount)
	{
		stbtt_vertex* stbVertex = NULL;
		verCount = 0;
		int idx = 0;
		get_glyph_index(font, str, &idx);
		if (!(idx < 0))
			verCount = stbtt_GetGlyphShape(font, idx, &stbVertex);
		return stbVertex;
	}
	static stbtt_vertex* get_char_shape(font_impl* font, int cp, int& verCount)
	{
		stbtt_vertex* stbVertex = NULL;
		verCount = 0;
		int idx = getGlyphIndex(font, cp);
		if (!(idx < 0))
			verCount = stbtt_GetGlyphShape(font, idx, &stbVertex);
		return stbVertex;
	}
	static stbtt_vertex* get_char_shape_idx(font_impl* font, int idx, int& verCount)
	{
		stbtt_vertex* stbVertex = NULL;
		verCount = 0;
		if (!(idx < 0))
			verCount = stbtt_GetGlyphShape(font, idx, &stbVertex);
		return stbVertex;
	}
	static void free_shape(stbtt_fontinfo* font, stbtt_vertex* v)
	{
		stbtt_FreeShape(font, v);
	}
	static void free_shape(font_impl* font, stbtt_vertex* v)
	{
		stbtt_FreeShape(font, v);
	}
	static int init_table(font_impl* font, unsigned char* data, int fontstart)
	{
		enum_table(data, fontstart, font->_tb);

		return 1;
	}
	struct fontinfo_tt
	{
		void* userdata;
		unsigned char* data;              // pointer to .ttf file
		int              fontstart;         // offset of start of font

		int numGlyphs;                     // number of glyphs, needed for range checking

		int loca, head, glyf, hhea, hmtx, kern, gpos, svg; // table locations as offset from start of .ttf
		int index_map;                     // a cmap mapping for our chosen character encoding
		int indexToLocFormat;              // format needed to map from glyph index to glyph

		stbtt__buf cff;                    // cff font data
		stbtt__buf charstrings;            // the charstring index
		stbtt__buf gsubrs;                 // global charstring subroutines index
		stbtt__buf subrs;                  // private charstring subroutines index
		stbtt__buf fontdicts;              // array of font dicts
		stbtt__buf fdselect;               // map from glyph to fontdict
	};

	struct glyph_header_t
	{
		int16_t	numberOfContours;//	If the number of contours is greater than or equal to zero, this is a simple glyph.If negative, this is a composite glyph — the value - 1 should be used for composite glyphs.
		int16_t	xMin;//	Minimum x for coordinate data.
		int16_t	yMin;//	Minimum y for coordinate data.
		int16_t	xMax;//	Maximum x for coordinate data.
		int16_t	yMax;//	Maximum y for coordinate data.
	};
	static int GetGlyfOffset(const fontinfo_tt* info, int glyph_index)
	{
		int g1, g2;

		assert(!info->cff.size);

		if (glyph_index >= info->numGlyphs) return -1; // glyph index out of range
		if (info->indexToLocFormat >= 2)    return -1; // unknown index->glyph map format

		if (info->indexToLocFormat == 0) {
			g1 = info->glyf + ttUSHORT(info->data + info->loca + glyph_index * 2) * 2;
			g2 = info->glyf + ttUSHORT(info->data + info->loca + glyph_index * 2 + 2) * 2;
		}
		else {
			g1 = info->glyf + ttULONG(info->data + info->loca + glyph_index * 4);
			g2 = info->glyf + ttULONG(info->data + info->loca + glyph_index * 4 + 4);
		}

		return g1 == g2 ? -1 : g1; // if length is 0, return -1
	}

	static void setvertex(vertex_f* v, uint8_t type, int x, int y, int cx, int cy)
	{
		v->type = type;
		v->x = x;
		v->y = y;
		v->cx = cx;
		v->cy = cy;
	}
	static int close_shape(vertex_f* vertices, int num_vertices, int was_off, int start_off, int sx, int sy, int scx, int scy, int cx, int cy)
	{
		if (start_off) {
			if (was_off)
				setvertex(&vertices[num_vertices++], STBTT_vcurve, (cx + scx) >> 1, (cy + scy) >> 1, cx, cy);
			setvertex(&vertices[num_vertices++], STBTT_vcurve, sx, sy, scx, scy);
		}
		else {
			if (was_off)
				setvertex(&vertices[num_vertices++], STBTT_vcurve, sx, sy, cx, cy);
			else
				setvertex(&vertices[num_vertices++], STBTT_vline, sx, sy, 0, 0);
		}
		return num_vertices;
	}

	static int GetGlyphShapeTT(const fontinfo_tt* info, int glyph_index, std::vector<vertex_f>* vd)
	{
		int16_t numberOfContours;
		uint8_t* endPtsOfContours;
		uint8_t* data = info->data;
		vertex_f* vertices = 0;
		int num_vertices = 0;
		int g = GetGlyfOffset(info, glyph_index);


		if (g < 0) return 0;
		auto ghp = (glyph_header_t*)(data + g);
		numberOfContours = ttSHORT(data + g);

		if (numberOfContours > 0) {
			uint8_t flags = 0, flagcount;
			int32_t ins, i, j = 0, m, n, next_move, was_off = 0, off, start_off = 0;
			int32_t x, y, cx, cy, sx, sy, scx, scy;
			uint8_t* points;
			endPtsOfContours = (data + g + 10);
			ins = ttUSHORT(data + g + 10 + numberOfContours * 2);
			points = data + g + 10 + numberOfContours * 2 + 2 + ins;

			n = 1 + ttUSHORT(endPtsOfContours + numberOfContours * 2 - 2);

			m = n + 2 * numberOfContours;  // a loose bound on how many vertices we might need
			vd->resize(m);
			vertices = vd->data();// STBTT_malloc(m * sizeof(vertices[0]), info->userdata);
			if (vertices == 0)
				return 0;

			next_move = 0;
			flagcount = 0;

			// in first pass, we load uninterpreted data into the allocated array
			// above, shifted to the end of the array so we won't overwrite it when
			// we create our final data starting from the front

			off = m - n; // starting offset for uninterpreted data, regardless of how m ends up being calculated

			// first load flags

			for (i = 0; i < n; ++i) {
				if (flagcount == 0) {
					flags = *points++;
					if (flags & 8)
						flagcount = *points++;
				}
				else
					--flagcount;
				vertices[off + i].type = flags;
			}

			// now load x coordinates
			x = 0;
			for (i = 0; i < n; ++i) {
				flags = vertices[off + i].type;
				if (flags & 2) {
					int16_t dx = *points++;
					x += (flags & 16) ? dx : -dx; // ???
				}
				else {
					if (!(flags & 16)) {
						x = x + (int16_t)(points[0] * 256 + points[1]);
						points += 2;
					}
				}
				vertices[off + i].x = (int16_t)x;
			}

			// now load y coordinates
			y = 0;
			for (i = 0; i < n; ++i) {
				flags = vertices[off + i].type;
				if (flags & 4) {
					int16_t dy = *points++;
					y += (flags & 32) ? dy : -dy; // ???
				}
				else {
					if (!(flags & 32)) {
						y = y + (int16_t)(points[0] * 256 + points[1]);
						points += 2;
					}
				}
				vertices[off + i].y = (int16_t)y;
			}

			// now convert them to our format
			num_vertices = 0;
			sx = sy = cx = cy = scx = scy = 0;
			for (i = 0; i < n; ++i) {
				flags = vertices[off + i].type;
				x = (int16_t)vertices[off + i].x;
				y = (int16_t)vertices[off + i].y;

				if (next_move == i) {
					if (i != 0)
						num_vertices = close_shape(vertices, num_vertices, was_off, start_off, sx, sy, scx, scy, cx, cy);

					// now start the new one
					start_off = !(flags & 1);
					if (start_off) {
						// if we start off with an off-curve point, then when we need to find a point on the curve
						// where we can start, and we need to save some state for when we wraparound.
						scx = x;
						scy = y;
						if (!(vertices[off + i + 1].type & 1)) {
							// next point is also a curve point, so interpolate an on-point curve
							sx = (x + (int32_t)vertices[off + i + 1].x) >> 1;
							sy = (y + (int32_t)vertices[off + i + 1].y) >> 1;
						}
						else {
							// otherwise just use the next point as our start point
							sx = (int32_t)vertices[off + i + 1].x;
							sy = (int32_t)vertices[off + i + 1].y;
							++i; // we're using point i+1 as the starting point, so skip it
						}
					}
					else {
						sx = x;
						sy = y;
					}
					setvertex(&vertices[num_vertices++], STBTT_vmove, sx, sy, 0, 0);
					was_off = 0;
					next_move = 1 + ttUSHORT(endPtsOfContours + j * 2);
					++j;
				}
				else {
					if (!(flags & 1)) { // if it's a curve
						if (was_off) // two off-curve control points in a row means interpolate an on-curve midpoint
							setvertex(&vertices[num_vertices++], STBTT_vcurve, (cx + x) >> 1, (cy + y) >> 1, cx, cy);
						cx = x;
						cy = y;
						was_off = 1;
					}
					else {
						if (was_off)
							setvertex(&vertices[num_vertices++], STBTT_vcurve, x, y, cx, cy);
						else
							setvertex(&vertices[num_vertices++], STBTT_vline, x, y, 0, 0);
						was_off = 0;
					}
				}
			}
			num_vertices = close_shape(vertices, num_vertices, was_off, start_off, sx, sy, scx, scy, cx, cy);
		}
		else if (numberOfContours < 0) {
			// Compound shapes.
			int more = 1;
			uint8_t* comp = data + g + 10;
			num_vertices = 0;
			vertices = 0;
			std::vector<vertex_f> cvd;
			while (more) {
				uint16_t flags, gidx;
				int comp_num_verts = 0, i;
				vertex_f* comp_verts = 0, * tmp = 0;
				float mtx[6] = { 1,0,0,1,0,0 }, m, n;
				flags = ttSHORT(comp); comp += 2;
				gidx = ttSHORT(comp); comp += 2;

				if (flags & 2) { // XY values
					if (flags & 1) { // shorts
						mtx[4] = ttSHORT(comp); comp += 2;
						mtx[5] = ttSHORT(comp); comp += 2;
					}
					else {
						mtx[4] = *((int8_t*)comp); comp += 1;
						mtx[5] = *((int8_t*)comp); comp += 1;
					}
				}
				else {
					// @TODO handle matching point
					assert(0);
				}
				if (flags & (1 << 3)) { // WE_HAVE_A_SCALE
					auto c16 = ttSHORT(comp);
					mtx[0] = mtx[3] = c16 / 16384.0f; comp += 2;
					mtx[1] = mtx[2] = 0;
				}
				else if (flags & (1 << 6)) { // WE_HAVE_AN_X_AND_YSCALE
					mtx[0] = ttSHORT(comp) / 16384.0f; comp += 2;
					mtx[1] = mtx[2] = 0;
					mtx[3] = ttSHORT(comp) / 16384.0f; comp += 2;
				}
				else if (flags & (1 << 7)) { // WE_HAVE_A_TWO_BY_TWO
					mtx[0] = ttSHORT(comp) / 16384.0f; comp += 2;
					mtx[1] = ttSHORT(comp) / 16384.0f; comp += 2;
					mtx[2] = ttSHORT(comp) / 16384.0f; comp += 2;
					mtx[3] = ttSHORT(comp) / 16384.0f; comp += 2;

				}
				auto mx23 = glm::mat3x2(mtx[0], mtx[1], mtx[2], mtx[3], mtx[4], mtx[5]);
				// Get indexed glyph.
				comp_num_verts = GetGlyphShapeTT(info, gidx, &cvd);
				comp_verts = cvd.data();
				if (comp_num_verts > 0) {
					// Transform vertices.
					for (i = 0; i < comp_num_verts; ++i) {
						vertex_f* v = &comp_verts[i];
						glm::vec3 v2 = glm::vec3(*(glm::vec2*)v, 1.0);
						// m[0][0] * v.x + m[1][0] * v.y + mtx[4],
						// m[0][1] * v.x + m[1][1] * v.y + mtx[5]);
						auto vv2 = mx23 * v2;
						v->x = vv2.x;
						v->y = vv2.y;
						v2 = glm::vec3(v->cx, v->cy, 1.0);
						vv2 = mx23 * v2;
						v->cx = vv2.x;
						v->cy = vv2.y;
						//v2 = glm::vec3(v->cx1, v->cy1, 1.0);
						//v2 = mx * v2;
						//v->cx1 = v2.x;
						//v->cy1 = v2.y;
					}
					// Append vertices.
					vd->resize(num_vertices + comp_num_verts);
					if (vd->size() == num_vertices + comp_num_verts)
					{
						tmp = vd->data();
						memcpy(tmp + num_vertices, comp_verts, comp_num_verts * sizeof(cvd[0]));
					}
					else { return 0; }
					num_vertices += comp_num_verts;
				}
				// More components ?
				more = flags & (1 << 5);
			}
		}
		else {
			// numberOfCounters == 0, do nothing
		}

		//*pvertices = vertices;
		return num_vertices;
	}



private:
};




//！stb_font
#ifndef NO_STBR

/*
todo 路径填充光栅化-function
输入路径path，缩放比例scale、bpm_size{宽高xy,每行步长z}、invert 1倒置, 0正常，
xy_off偏移默认0
输出灰度图
支持移动、直线、2/3次贝塞尔曲线，构成的路径。
*/
unsigned char* get_glyph_bitmap_subpixel(stbtt_vertex* vertices, int num_verts, glm::vec2 scale, glm::vec2 shift, glm::vec2 xy_off
	, std::vector<unsigned char>* out, glm::ivec3 bpm_size, int invert)
{
	stbtt__bitmap gbm;
	if (!(scale.x > 0 && scale.y > 0))
	{
		scale.x = scale.y = std::max(std::max(scale.x, scale.y), 1.0f);
	}
	// now we get the size
	gbm.w = bpm_size.x;
	gbm.h = bpm_size.y;
	gbm.pixels = NULL; // in case we error
	int bms = gbm.w * gbm.h;
	if (bpm_size.z < 1) bpm_size.z = gbm.w;
	if (bms > 0)
	{
		if (out->empty())
			out->resize(bms);
		gbm.pixels = (unsigned char*)out->data();
		if (gbm.pixels)
		{
			gbm.stride = bpm_size.z;
			stbtt_Rasterize(&gbm, 0.35f, vertices, num_verts, scale.x, scale.y, shift.x, shift.y, xy_off.x, xy_off.y, invert, 0);
		}
	}
	return gbm.pixels;
}

void get_path_bitmap(vertex_32f* vertices, size_t num_verts, image_gray* bmp, glm::vec2 scale, glm::vec2 xy_off, int invert)
{
	std::vector<stbtt_vertex> v;
	v.resize(num_verts);
	for (size_t i = 0; i < num_verts; i++)
	{
		auto& it = v[i];
		auto& v2 = vertices[i];
		it.type = v2.type;
		it.x = v2.p.x; it.y = v2.p.y;
		it.cx = v2.c.x; it.cy = v2.c.y;
		it.cx1 = v2.c1.x; it.cy1 = v2.c1.y;
	}
	get_glyph_bitmap_subpixel((stbtt_vertex*)v.data(), num_verts, scale, { 0, 0 }, xy_off, &bmp->_data, { bmp->width, bmp->height, 0 }, invert);
}


#define APREC 16
#define ZPREC 7 
void blurCols(unsigned char* dst, int w, int h, int dstStride, int alpha)
{
	int x, y;
	for (y = 0; y < h; y++) {
		int z = 0; // force zero border
		for (x = 1; x < w; x++) {
			z += (alpha * (((int)(dst[x]) << ZPREC) - z)) >> APREC;
			dst[x] = (unsigned char)(z >> ZPREC);
		}
		dst[w - 1] = 0; // force zero border
		z = 0;
		for (x = w - 2; x >= 0; x--) {
			z += (alpha * (((int)(dst[x]) << ZPREC) - z)) >> APREC;
			dst[x] = (unsigned char)(z >> ZPREC);
		}
		dst[0] = 0; // force zero border
		dst += dstStride;
	}
}

void blurRows(unsigned char* dst, int w, int h, int dstStride, int alpha)
{
	int x, y;
	for (x = 0; x < w; x++) {
		int z = 0; // force zero border
		for (y = dstStride; y < h * dstStride; y += dstStride) {
			z += (alpha * (((int)(dst[y]) << ZPREC) - z)) >> APREC;
			dst[y] = (unsigned char)(z >> ZPREC);
		}
		dst[(h - 1) * dstStride] = 0; // force zero border
		z = 0;
		for (y = (h - 2) * dstStride; y >= 0; y -= dstStride) {
			z += (alpha * (((int)(dst[y]) << ZPREC) - z)) >> APREC;
			dst[y] = (unsigned char)(z >> ZPREC);
		}
		dst[0] = 0; // force zero border
		dst++;
	}
}

void blur2gray(unsigned char* dst, int w, int h, int dstStride, float blur, int n, int mode)
{
	int alpha;
	float sigma;
	if (blur < 1)
		return;
	// Calculate the alpha such that 90% of the kernel is within the radius. (Kernel extends to infinity)
	sigma = (float)blur * 0.57735f; // 1 / sqrt(3)
	alpha = (int)((1 << APREC) * (1.0f - expf(-2.3f / (sigma + 1.0f))));
	for (int i = 0; i < n; i++)
	{
		if (mode & 0x01)
			blurRows(dst, w, h, dstStride, alpha);
		if (mode & 0x02)
			blurCols(dst, w, h, dstStride, alpha);
	}
}
#if 0
void build_blur(image_gray* grayblur, float blur, unsigned int fill, int blurcount, Image* dst, glm::ivec2 pos, bool iscp)
{

	std::vector<unsigned char> rbd;
	unsigned char* bdata = grayblur->data();
	glm::ivec2 rbs = { grayblur->width, grayblur->height };
	if (iscp)
	{
		rbd.resize((int64_t)rbs.x * rbs.y);
		auto cpd = rbd.data();
		memcpy(cpd, bdata, rbd.size());
		bdata = cpd;
		grayblur->ud = cpd;
	}
	// 模糊
	blur2gray(bdata, rbs.x, rbs.y, rbs.x, blur, blurcount);
	// 转成rgba
	if (dst)
	{
		glm::ivec4 src4 = { 0, 0, rbs.x, rbs.y };
		dst->copy2(grayblur, pos, src4, fill);
		//dst->copy_to_image(bdata, rbs.x, src4, fill, 2);
	}
}
#endif
#endif // !NO_STBR


void test_stbfont()
{
	auto str = L"stb";
	/* 加载字体（.ttf）文件 */
	long int size = 0;
	unsigned char* fontBuffer = NULL;

	FILE* fontFile = fopen("C:\\Windows\\Fonts\\Arial.ttf", "rb");
	if (fontFile == NULL)
	{
		printf("Can not open font file!\n");
		return;
	}
	fseek(fontFile, 0, SEEK_END); /* 设置文件指针到文件尾，基于文件尾偏移0字节 */
	size = ftell(fontFile);       /* 获取文件大小（文件尾 - 文件头  单位：字节） */
	fseek(fontFile, 0, SEEK_SET); /* 重新设置文件指针到文件头 */

	fontBuffer = (unsigned char*)calloc(size, sizeof(unsigned char));
	fread(fontBuffer, size, 1, fontFile);
	fclose(fontFile);

	/* 初始化字体 */
	stbtt_fontinfo info;
	if (!stbtt_InitFont(&info, fontBuffer, 0))
	{
		printf("stb init font failed\n");
	}

	/* 创建位图 */
	int bitmap_w = 512; /* 位图的宽 */
	int bitmap_h = 128; /* 位图的高 */
	unsigned char* bitmap = (unsigned char*)calloc(bitmap_w * bitmap_h, sizeof(unsigned char));

	/* 计算字体缩放 */
	float pixels = 64.0;                                    /* 字体大小（字号） */
	float scale = stbtt_ScaleForPixelHeight(&info, pixels); /* scale = pixels / (ascent - descent) */

	/**
	 * 获取垂直方向上的度量
	 * ascent：字体从基线到顶部的高度；
	 * descent：基线到底部的高度，通常为负值；
	 * lineGap：两个字体之间的间距；
	 * 行间距为：ascent - descent + lineGap。
	*/
	int ascent = 0;
	int descent = 0;
	int lineGap = 0;
	stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

	/* 根据缩放调整字高 */
	ascent = roundf(ascent * scale);
	descent = roundf(descent * scale);

	int x = 0; /*位图的x*/
	auto nw = wcslen(str);
	/* 循环加载str中每个字符 */
	for (int i = 0; i < nw; ++i)
	{
		/**
		  * 获取水平方向上的度量
		  * advanceWidth：字宽；
		  * leftSideBearing：左侧位置；
		*/
		int advanceWidth = 0;
		int leftSideBearing = 0;
		stbtt_GetCodepointHMetrics(&info, str[i], &advanceWidth, &leftSideBearing);

		/* 获取字符的边框（边界） */
		int c_x1, c_y1, c_x2, c_y2;
		stbtt_GetCodepointBitmapBox(&info, str[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

		/* 计算位图的y (不同字符的高度不同） */
		int y = ascent + c_y1;

		/* 渲染字符 */
		int byteOffset = x + roundf(leftSideBearing * scale) + (y * bitmap_w);
		stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, bitmap_w, scale, scale, str[i]);

		/* 调整x */
		x += roundf(advanceWidth * scale);

		/* 调整字距 */
		int kern;
		kern = stbtt_GetCodepointKernAdvance(&info, str[i], str[i + 1]);
		x += roundf(kern * scale);
	}

	/* 将位图数据保存到1通道的png图像中 */
	stbi_write_png("STB.png", bitmap_w, bitmap_h, 1, bitmap, bitmap_w);

	free(fontBuffer);
	free(bitmap);
}

//k=language_id			2052 简体中文	1033 英语

std::string get_info_str(int language, int idx, std::map<int, std::vector<info_one>>& _detail)
{
	std::string ret;
	int ls[] = { language, 1033 };

	for (int i = 0; i < 2 && ret.empty(); i++)
	{
		auto it = _detail.find(ls[i]);
		if (it != _detail.end())
		{
			auto& p = it->second;
			for (size_t j = 0; j < p.size(); j++)
			{
				if (p[j].name_id == idx)
				{
					ret = (p[j].get_name());
				}
			}
		}
	}
	return ret;
}

font_t::font_t()
{
	font = new font_impl();
	hp = new hps_t();
	assert(font);
}
void free_colorinfo(gcolors_t* colorinfo);

font_t::~font_t()
{
	std::map<int, std::vector<info_one*>> detail;
	for (auto& [k, v] : detail)
	{
		for (auto it : v)
		{
			if (it)
			{
				delete it;
			}
		}
	}
	detail.clear();
	if (bitinfo)delete bitinfo; bitinfo = 0;
	if (colorinfo) {
		free_colorinfo(colorinfo);
		delete colorinfo; colorinfo = 0;
	}
	if (font)delete font; font = 0;
#if TTF_USE_HARFBUZZ
	if (hp->hb_font) {
		hb_face_t* face = hb_font_get_face(hp->hb_font);
		if (face)hb_face_destroy(face);
		hb_font_destroy(hp->hb_font);
	}
	if (hp->hb_buffer)
		hb_buffer_destroy(hp->hb_buffer);
#endif
	if (hp)delete hp; hp = 0;
}
float font_t::get_scale(int px)
{
	return stb_font::getPixelHeightScale(font, px);
}
void font_t::init_post_table()
{
	hheaTbl hhea[1] = {}; head_table p[1] = {};
	stb_font::get_hhea(font, hhea);
	stb_font::get_head(font, p);
	xMaxExtent = hhea->xMaxExtent;
	lineGap = hhea->lineGap;


#if TTF_USE_HARFBUZZ

	hb_blob_t* blob = hb_blob_create((char*)font->data, dataSize, HB_MEMORY_MODE_READONLY, 0, 0); //HB_MEMORY_MODE_WRITABLEhb_blob_create_from_file(ttfn);
	if (hb_blob_get_length(blob) > 0)
	{
		hb_face_t* face = hb_face_create(blob, _index);
		hp->hb_font = hb_font_create(face);
		if (hp->hb_font == NULL) {
			if (face)hb_face_destroy(face);
		}
		//hb_ft_font_set_load_flags(font->hb_font, FT_LOAD_DEFAULT | font->ft_load_target); 
		hp->hb_language = hb_language_from_string("", -1);

		hp->hb_buffer = hb_buffer_create();
	}
	if (blob)hb_blob_destroy(blob);
#endif

}

uint32_t font_t::CollectGlyphsFromFont(const void* text, size_t length, int type, int direction, uint32_t script, GlyphPositions* positions)
{
#if TTF_USE_HARFBUZZ
	// Create a buffer for harfbuzz to use
	auto hb_buffer = hp->hb_buffer;
	if (type == 8 && length == -1) {
		if (length == -1)
		{
			length = strlen((char*)text);
		}
	}
	if (!hb_buffer || !text || length == 0 || length == -1) {
		//SDL_SetError("Cannot create harfbuzz buffer");
		return 0;
	}
	hb_buffer_reset(hb_buffer);
	// Set global configuration
	hb_buffer_set_language(hb_buffer, hp->hb_language);
	hb_buffer_set_direction(hb_buffer, (hb_direction_t)(direction == 0 ? HB_DIRECTION_LTR : HB_DIRECTION_RTL));
	hb_buffer_set_script(hb_buffer, (hb_script_t)script);
	switch (type) {
	case 8:
	{
		hb_buffer_add_utf8(hb_buffer, (char*)text, (int)length, 0, -1);
	}
	break;
	case 16:
	{
		hb_buffer_add_utf16(hb_buffer, (uint16_t*)text, (int)length, 0, -1);
	}
	break;
	case 32:
	{
		hb_buffer_add_utf32(hb_buffer, (uint32_t*)text, (int)length, 0, -1);
	}
	break;
	}
	hb_buffer_guess_segment_properties(hb_buffer);

	hb_feature_t userfeatures[2];
	userfeatures[0].tag = HB_TAG('k', 'e', 'r', 'n');
	userfeatures[0].value = enable_kerning;
	userfeatures[0].start = HB_FEATURE_GLOBAL_START;
	userfeatures[0].end = HB_FEATURE_GLOBAL_END;
	userfeatures[1].tag = HB_TAG('l', 'i', 'g', 'a');
	userfeatures[1].value = enable_liga;
	userfeatures[1].start = HB_FEATURE_GLOBAL_START;
	userfeatures[1].end = HB_FEATURE_GLOBAL_END;

	hb_shape(hp->hb_font, hb_buffer, userfeatures, 2);

	// Get the result
	unsigned int glyph_count_u = 0;
	hb_glyph_info_t* hb_glyph_info = hb_buffer_get_glyph_infos(hb_buffer, &glyph_count_u);
	hb_glyph_position_t* hb_glyph_position = hb_buffer_get_glyph_positions(hb_buffer, &glyph_count_u);

	// Adjust for bold text
	int advance_if_bold = 0;
	//if (TTF_HANDLE_STYLE_BOLD(font)) {
	//	advance_if_bold = F26Dot6(font->glyph_overhang);
	//}

	// Realloc, if needed
	uint32_t glyph_count = (int)glyph_count_u;
	_tnpos.resize(glyph_count);
	if (positions)
	{
		positions->pos = _tnpos.data();
		positions->len = glyph_count;
		for (int i = 0; i < glyph_count; ++i) {
			GlyphPosition* pos = &positions->pos[i];
			pos->font = this;
			pos->index = hb_glyph_info[i].codepoint;
			pos->x_advance = hb_glyph_position[i].x_advance + advance_if_bold;
			pos->y_advance = hb_glyph_position[i].y_advance;
			pos->x_offset = hb_glyph_position[i].x_offset;
			pos->y_offset = hb_glyph_position[i].y_offset;
			pos->offset = (int)hb_glyph_info[i].cluster;
			//if (!Find_GlyphByIndex(font, pos->index, 0, 0, 0, 0, 0, 0, &pos->glyph, NULL)) 
			if (pos->index <= 0)continue;
		}
	}
#endif
	return glyph_count;
}

int font_t::GetGlyphShapeTT(int glyph_index, std::vector<vertex_f>* vd)
{
	return stb_font::GetGlyphShapeTT((stb_font::fontinfo_tt*)font, glyph_index, vd);
}

#ifndef TAGS_H_
#define TAGS_H_


#define MAKE_TAG(_x1, _x2, _x3, _x4) (uint32_t)(((uint32_t)_x1 << 24)|((uint32_t)_x2 << 16)|((uint32_t)_x3 << 8)|(uint32_t)_x4)


#define TAG_avar  "avar"
#define TAG_BASE  "BASE"
#define TAG_bdat  "bdat"
#define TAG_BDF   "BDF "
#define TAG_bhed  "bhed"
#define TAG_bloc  "bloc"
#define TAG_bsln  "bsln"
#define TAG_CBDT  "CBDT"
#define TAG_CBLC  "CBLC"
#define TAG_CFF   "CFF "
#define TAG_CID   "CID "
#define TAG_CPAL  "CPAL"
#define TAG_COLR  "COLR"
#define TAG_cmap  "cmap"
#define TAG_cvar  "cvar"
#define TAG_cvt   "cvt "
#define TAG_DSIG  "DSIG"
#define TAG_EBDT  "EBDT"
#define TAG_EBLC  "EBLC"
#define TAG_EBSC  "EBSC"
#define TAG_feat  "feat"
#define TAG_FOND  "FOND"
#define TAG_fpgm  "fpgm"
#define TAG_fvar  "fvar"
#define TAG_gasp  "gasp"
#define TAG_GDEF  "GDEF"
#define TAG_glyf  "glyf"
#define TAG_GPOS  "GPOS"
#define TAG_GSUB  "GSUB"
#define TAG_gvar  "gvar"
#define TAG_hdmx  "hdmx"
#define TAG_head  "head"
#define TAG_hhea  "hhea"
#define TAG_hmtx  "hmtx"
#define TAG_JSTF  "JSTF"
#define TAG_just  "just"
#define TAG_kern  "kern"
#define TAG_lcar  "lcar"
#define TAG_loca  "loca"
#define TAG_LTSH  "LTSH"
#define TAG_LWFN  "LWFN"
#define TAG_MATH  "MATH"
#define TAG_maxp  "maxp"
#define TAG_META  "META"
#define TAG_MMFX  "MMFX"
#define TAG_MMSD  "MMSD"
#define TAG_mort  "mort"
#define TAG_morx  "morx"
#define TAG_name  "name"
#define TAG_opbd  "opbd"
#define TAG_OS2   "OS/2"
#define TAG_OTTO  "OTTO"
#define TAG_PCLT  "PCLT"
#define TAG_POST  "POST"
#define TAG_post  "post"
#define TAG_prep  "prep"
#define TAG_prop  "prop"
#define TAG_sbix  "sbix"
#define TAG_sfnt  "sfnt"
#define TAG_SING  "SING"
#define TAG_trak  "trak"
#define TAG_true  "true"
#define TAG_ttc   "ttc "
#define TAG_ttcf  "ttcf"
#define TAG_TYP1  "TYP1"
#define TAG_typ1  "typ1"
#define TAG_VDMX  "VDMX"
#define TAG_vhea  "vhea"
#define TAG_vmtx  "vmtx"
#define TAG_wOFF  "wOFF"



#endif /* TAGS_H_ */


sfnt_header* get_tag(font_impl* font_i, const std::string& tag)
{
	auto it = font_i->_tb.find(tag);
	return it != font_i->_tb.end() ? &it->second : nullptr;
}


typedef struct  LayerIterator_
{
	uint32_t   num_layers;
	uint32_t   layer;
	uint8_t* p;

} LayerIterator;
typedef struct  Palette_Data_ {
	uint16_t         num_palettes;
	const uint16_t* palette_name_ids;
	const uint16_t* palette_flags;

	uint16_t         num_palette_entries;
	const uint16_t* palette_entry_name_ids;

} Palette_Data;

union Color_2
{
	uint32_t c;
	struct {
		uint8_t r, g, b, a;
	};
	struct {
		uint8_t red, green, blue, alpha;
	};
};
struct Cpal;
struct Colr;
struct gcolors_t
{
	Cpal* cpal;
	Colr* colr;
	/* glyph colors */
	Palette_Data palette_data;         /* since 2.10 */
	uint16_t palette_index;
	Color_2* palette;
	Color_2 foreground_color;
	bool have_foreground_color;
	char td[256];
};
struct GlyphSlot;

typedef union {
	uint32_t color;
	struct {
		unsigned char b, g, r, a;
	} argb;
} stbtt_color;
bool stbtt_FontHasPalette(const stbtt_fontinfo* info);
unsigned short stbtt_FontPaletteCount(const stbtt_fontinfo* info);
unsigned short stbtt_FontPaletteGetColors(const stbtt_fontinfo* info, unsigned short paletteIndex, stbtt_color** colorPalette);
//// Glyph layers (COLR) /////////////////////////////////////////////////////
typedef struct {
	unsigned short glyphid, colorid;
} stbtt_glyphlayer;
bool stbtt_FontHasLayers(const stbtt_fontinfo* info);
unsigned short stbtt_GetGlyphLayers(const stbtt_fontinfo* info, unsigned short glypId, stbtt_glyphlayer** glyphLayer);
unsigned short stbtt_GetCodepointLayers(const stbtt_fontinfo* info, unsigned short codePoint, stbtt_glyphlayer** glyphLayer);



void free_colorinfo(gcolors_t* colorinfo) {

	if (colorinfo->palette_data.palette_name_ids)delete[]colorinfo->palette_data.palette_name_ids;
	if (colorinfo->palette_data.palette_flags)delete[]colorinfo->palette_data.palette_flags;
	if (colorinfo->palette_data.palette_entry_name_ids)delete[]colorinfo->palette_data.palette_entry_name_ids;
}


int tt_face_load_colr(font_t* face, uint8_t* b, sfnt_header* sp);

void tt_face_free_colr(font_t*);

bool tt_face_get_colr_layer(font_t* face,
	uint32_t            base_glyph,
	uint32_t* aglyph_index,
	uint32_t* acolor_index,
	LayerIterator* iterator);
// 获取颜色
Color_2 get_c2(font_t* face1, uint32_t color_index);

int tt_face_colr_blend_layer(font_t* face, uint32_t       color_index, GlyphSlot* dstSlot, GlyphSlot* srcSlot);


int tt_face_load_cpal(font_t* face, uint8_t* b, sfnt_header* sp);

void tt_face_free_cpal(font_t* face);

int tt_face_palette_set(font_t* face, uint32_t  palette_index);
// 初始化颜色
int font_t::init_color()
{
	font_impl* font_i = font;
	const stbtt_fontinfo* font = font_i;
	int i, count, stringOffset;
	uint8_t* fc = font->data;
	uint32_t offset = font->fontstart, table_size = 0, sbit_num_strikes = 0;
	uint32_t ebdt_start = 0, ebdt_size = 0;
	sfnt_header* ebdt_table = 0;
	auto cpal_table = get_tag(font_i, TAG_CPAL);
	auto colr_table = get_tag(font_i, TAG_COLR);
	if (!cpal_table || !colr_table)
		return 0;
	if (!colorinfo)
		colorinfo = new gcolors_t();
	font_t* ttp = (font_t*)font->userdata;
	uint8_t* b = fc + cpal_table->offset;
	uint8_t* b1 = fc + colr_table->offset;
	tt_face_load_cpal(this, b, cpal_table);
	tt_face_load_colr(this, b1, colr_table);

	return 0;
}

int font_t::get_gcolor(uint32_t base_glyph, std::vector<glm::uvec2>& cols)
{
	uint32_t aglyph_index = base_glyph;
	uint32_t acolor_index = 0;
	LayerIterator it = {};
	std::vector<vertex_f> vdp;
	for (;;)
	{
		if (!tt_face_get_colr_layer(this, aglyph_index, &aglyph_index, &acolor_index, &it))
		{
			break;
		}
		cols.push_back({ aglyph_index, get_c2(this, acolor_index).c });
	}
	return it.num_layers;
}


#ifndef no_colr

uint32_t stbtt__find_table(uint8_t* data, uint32_t fontstart, const char* tag)
{
	int32_t num_tables = stb_font::ttUSHORT(data + fontstart + 4);
	uint32_t tabledir = fontstart + 12;
	int32_t i;
	for (i = 0; i < num_tables; ++i) {
		uint32_t loc = tabledir + 16 * i;
		if (stbtt_tag(data + loc + 0, tag))
			return stb_font::ttULONG(data + loc + 8);
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////////
// Glyph layered color font support using COLR/CPAL tables
//

static int stbtt__get_cpal(font_impl* info)
{
	if (info->cpal < 0 && info != NULL) //Load table if not exists
		info->cpal = stbtt__find_table(info->data, info->fontstart, "CPAL");
	return info->cpal;
}

static int stbtt__get_colr(font_impl* info)
{
	if (info->colr < 0 && info != NULL) //Load table if not exists
	{
		info->colr = stbtt__find_table(info->data, info->fontstart, "COLR");
		if (info->colr > 0) //swap bytes on table, so it can be returned
		{
			const uint32_t layerRecordsOffset = stb_font::ttULONG(info->data + info->colr + 8);
			const uint16_t numLayerRecords = stb_font::ttUSHORT(info->data + info->colr + 12);
			unsigned char* colOffset = info->data + info->colr + layerRecordsOffset;
			for (int i = 0; i < numLayerRecords; i++) //Swap bytes
			{
				unsigned short* col = (unsigned short*)colOffset + (2 * i);
				col[0] = (col[0] >> 8) | (col[0] << 8);
				col[1] = (col[1] >> 8) | (col[1] << 8);
			}
		}
	}
	return info->colr;
}

bool stbtt_FontHasLayers(const font_impl* info)
{
	const int table_colr = stbtt__get_colr((font_impl*)info);
	if (table_colr > 0 && info != NULL) //Check table and if theres glyphs
		if (stb_font::ttUSHORT(info->data + table_colr + 2 /*numBaseGlyphRecords*/) > 0)
			return 1;
	return 0;
}

bool stbtt_FontHasPalette(const font_impl* info)
{
	const int table_cpal = stbtt__get_cpal((font_impl*)info);
	if (table_cpal > 0 && info != NULL) //Check table and if theres palettes
		if (stb_font::ttUSHORT(info->data + table_cpal + 4 /*numPalettes*/) > 0)
			return 1;
	return 0;
}

unsigned short stbtt_FontPaletteCount(const font_impl* info)
{
	const int table_cpal = stbtt__get_cpal((font_impl*)info);
	if (table_cpal > 0 && info != NULL) //Check palettes and input
	{
		const uint16_t numPalettes = stb_font::ttUSHORT(info->data + table_cpal + 4);
		return numPalettes; //Success
	}
	return 0; //Failed
}

unsigned short stbtt_FontPaletteGetColors(const font_impl* info, unsigned short paletteIndex, stbtt_color** colorPalette)
{
	const int table_cpal = stbtt__get_cpal((font_impl*)info);
	if (table_cpal > 0 && info != NULL) //Check palettes
	{
		const uint16_t numPaletteEntries = stb_font::ttUSHORT(info->data + table_cpal + 2);
		if (colorPalette)
		{
			const uint16_t numPalettes = stb_font::ttUSHORT(info->data + table_cpal + 4);
			if (paletteIndex > numPalettes - 1) return 0; //Invalid palette index
			const uint32_t colorRecordsArrayOffset = stb_font::ttULONG(info->data + table_cpal + 8);
			const uint16_t colorRecordIndices = stb_font::ttUSHORT(info->data + table_cpal + 12 + (2 * paletteIndex));
			const uint8_t* colorptr = info->data + table_cpal + colorRecordsArrayOffset + (colorRecordIndices * 4);
			*colorPalette = (stbtt_color*)colorptr;
		}
		return numPaletteEntries;
	}
	return 0; //Failed
}

unsigned short stbtt_GetGlyphLayers(const font_impl* info, unsigned short glypId, stbtt_glyphlayer** glyphLayer)
{
	const int table_colr = stbtt__get_colr((font_impl*)info);
	if (table_colr > 0 && info != NULL) //Check glyph table
	{
		const uint16_t numBaseGlyphRecords = stb_font::ttUSHORT(info->data + table_colr + 2);
		const uint32_t baseGlyphRecordsOffset = stb_font::ttULONG(info->data + table_colr + 4);
		const uint32_t layerRecordsOffset = stb_font::ttULONG(info->data + table_colr + 8);
		int32_t low = 0;
		int32_t high = (int32_t)numBaseGlyphRecords;
		while (low < high) // Binary search, lookup glyph table.
		{
			int32_t mid = low + ((high - low) >> 1);
			uint16_t foundGlyphID = stb_font::ttUSHORT(info->data + table_colr + baseGlyphRecordsOffset + (6 * mid));
			if ((uint32_t)glypId < foundGlyphID) //Trim high
				high = mid;
			else if ((uint32_t)glypId > foundGlyphID) //Trim low
				low = mid + 1;
			else //Result found
			{
				const uint16_t numLayers = stb_font::ttUSHORT(info->data + table_colr + baseGlyphRecordsOffset + (6 * mid) + 4);
				if (glyphLayer)
				{
					const uint16_t firstLayerIndex = stb_font::ttUSHORT(info->data + table_colr + baseGlyphRecordsOffset + (6 * mid) + 2);
					const uint8_t* layerptr = info->data + table_colr + layerRecordsOffset + (firstLayerIndex * 4);
					*glyphLayer = (stbtt_glyphlayer*)layerptr;
				}
				return numLayers; //Sucess
			}
		}
	}
	return 0; //Not found, failed
}

unsigned short stbtt_GetCodepointLayers(const font_impl* info, unsigned short codePoint, stbtt_glyphlayer** glyphLayer)
{
	return stbtt_GetGlyphLayers(info, stbtt_FindGlyphIndex(info, codePoint), glyphLayer); //Lookup by glyph id
}
#endif // !no_colr







void init_bitmap_bitdepth(Bitmap_p* bitmap, int bit_depth);
/*
输入
int gidx			字符索引号
double height		期望高度
bool first_bitmap	是否优先查找位图字体
输出
glm::ivec4* ot		x,y,z=width,w=height
std::string* out	输出缓存区
Bitmap_p* bitmap		输出位图信息
返回1成功
*/

int font_t::get_glyph_image(int gidx, double height, glm::ivec4* ot, Bitmap_p* bitmap, std::vector<char>* out, int lcd_type, uint32_t unicode_codepoint, int xf)
{
	int ret = 0;
	if (gidx > 0)
	{
		double scale = get_scale(height);
		//double scale = get_scale_height(height);
		if (height < 0)
		{
			height *= -1;
		}
#ifndef _FONT_NO_BITMAP
		if (first_bitmap)
		{
			// 解析位图
			ret = get_glyph_bitmap(gidx, height, ot, bitmap, out);
			// 找不到位图时尝试用自定义解码
			if (!ret)
				ret = get_custom_decoder_bitmap(unicode_codepoint, height, ot, bitmap, out);
		}
#endif
		if (!ret)
		{
			// 解析轮廓并光栅化 
			glm::vec3 adlsb = { 0,0,height };
			glm::vec2 lcds[] = { {1, 1}, {3, 1}, {1, 3}, {4, 1, } };
			stb_font::get_glyph_bitmap(font, gidx, scale, ot, out, &adlsb, lcds[lcd_type], xf);
			if (bitmap)
			{
				auto hh = hp->hhea;
				auto he = hp->hhea.ascender + hp->hhea.descender + hp->hhea.lineGap;
				auto hef = hp->hhea.ascender - hp->hhea.descender + hp->hhea.lineGap;

				double hed = (scale * he);//ceil
				double hedf = (scale * hef);
				double lg = (scale * hp->hhea.lineGap);
				if (out)
					bitmap->buffer = (unsigned char*)out->data();
				bitmap->width = bitmap->pitch = ot->z;
				bitmap->rows = ot->w;
				bitmap->advance = adlsb.z;
				bitmap->bearingX = adlsb.x;
				bitmap->pixel_mode = PX_GRAY;	//255灰度图
				bitmap->lcd_mode = lcd_type;
				init_bitmap_bitdepth(bitmap, 8);
				ret = 1;
			}
		}
		if (bitmap)
		{
			bitmap->x = ot->x;
			bitmap->y = ot->y;
		}
	}
	return ret;
}


double font_t::get_base_line(double height)
{
	float scale = get_scale(height);
	double f = ascender;
	//return ceil(f * scale);
	return floor(f * scale); // 向下取整
}

double font_t::get_line_height(double height)
{
	float scale = get_scale(height);
	double f = ascender - descender + lineGap;
	return ceil(f * scale);// 向上取整
}


glm::ivec3 font_t::get_text_rect(int fontsize, const void* str8, int len)
{
	glm::ivec3 ret = {};
	if (fontsize < 5 || !str8 || !len)
		return ret;
	auto str = (const char*)str8;
	auto font = this;
	int x = 0;
	int y = 0;
	int n = 1;
	int lineheight = 0;// get_lineheight(idx, fontsize);

	font_t* oft0 = 0;
	do
	{
		if (!str || !(*str)) { break; }
		int ch = 0;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		str += kk;
		//str = md::get_u8_last(str, &ch);
		if (ch == '\n')
		{
			ret.x = std::max(ret.x, x);
			x = 0;
			n++;
			continue;
		}
		font_t* oft = 0;
		auto rc = font->get_char_extent(ch, fontsize, 0, &oft);
		if (oft != oft0 && oft) {
			oft0 = oft;
			double scale = fontsize == 0 ? 1.0 : oft->get_scale(fontsize);
			lineheight = std::max((int)((oft->ascender - oft->descender + oft->lineGap) * scale), lineheight);
		}

		x += rc.z;
		y = std::max(rc.y, y);
		ret.y = std::max(ret.y, y);
		ret.z = std::max(ret.z, rc.w);
	} while (str && *str);
	ret.x = std::max(ret.x, x);
	if (n > 1)
		ret.y = lineheight * n;
	else
		ret.y = get_line_height(fontsize);
	return ret;
}

int font_t::get_xmax_extent(double height, int* line_gap)
{
	float scale = get_scale(height);
	double f = xMaxExtent, lig = lineGap;
	if (line_gap)
	{
		*line_gap = ceil(lig * scale);
	}
	return ceil(f * scale);
}
// 获取字体最大box
glm::ivec4 font_t::get_bounding_box(double scale, bool is_align0)
{
	auto ret = stb_font::get_bounding_box(font);
	if (is_align0)
	{
		if (ret.x != 0)
		{
			ret.z -= ret.x; ret.x = 0;
		}
		if (ret.y != 0)
		{
			ret.w -= ret.y; ret.y = 0;
		}
	}
	glm::vec4 s = ret;
	s *= scale;
	return ceil(s);
}
tinypath_t font_t::get_shape(const void* str8, int height, std::vector<vertex_f>* opt, int adv, float scale1)
{
	int vc = 0;
	tinypath_t r = {};
	if (!opt)return r;
	auto p = &r;
	stbtt_vertex* v = stb_font::get_char_shape(font, (char*)str8, vc);
	if (v)
	{
		if (p && vc > 1)
		{
			auto bs = get_shape_box(str8, height);
			r.advance = bs.x;
			r.bearing = { bs.y,0 };
			auto pss = opt->size();
			r.first = pss;
			opt->resize(pss + vc);
			r.v = opt->data() + pss;
			p->count = vc;
			for (size_t i = 0; i < vc; i++)
			{
				r.v[i].type = v[i].type;
				r.v[i].x = v[i].x;
				r.v[i].y = v[i].y;
				r.v[i].cx = v[i].cx;
				r.v[i].cy = v[i].cy;
				r.v[i].cx1 = v[i].cx1;
				r.v[i].cy1 = v[i].cy1;
			}
			if (height != 0)
			{
				float scale = get_scale(height) * scale1;
				if (scale > 0)
				{
					adv += bs.y;
					r.baseline = ascender * scale;
					auto v1 = r.v;
					for (size_t i = 0; i < vc; i++)
					{
						v1->x *= scale;
						v1->y *= scale;
						v1->cx *= scale;
						v1->cy *= scale;
						v1->cx1 *= scale;
						v1->cy1 *= scale;
						v1++;
					}
				}
				else {
					r.baseline = ascender;
				}
			}
			{
				auto v1 = r.v;
				for (size_t i = 0; i < vc; i++)
				{
					v1->x += adv;
					v1->cx += adv;
					v1->cx1 += adv;
					v1++;
				}
			}
		}
		stb_font::free_shape(font, v);
	}
	return r;
}
glm::ivec2 font_t::get_shape_box(const void* str8, int height)
{
	auto ch = stb_font::get_u8_to_u16((char*)str8);
	glm::vec2 v = stb_font::get_codepoint_hmetrics(font, ch);
	if (height != 0)
	{
		float scale = get_scale(height);
		v *= scale;
	}
	return v;
}
glm::ivec2 font_t::get_shape_box(uint32_t ch, int height)
{
	glm::vec2 v = stb_font::get_codepoint_hmetrics(font, ch);
	if (height != 0)
	{
		float scale = get_scale(height);
		v *= scale;
	}
	return v;
}
glm::ivec2 font_t::get_shape_box_glyph(uint32_t gid, int height, glm::vec4* pbox)
{
	glm::vec2 v = stb_font::get_glyph_hmetrics(font, gid);
	if (height != 0)
	{
		float scale = get_scale(height);
		if (pbox)
			*pbox = stb_font::get_glyph_box(font, gid, scale);
		v *= scale;
	}
	return v;
}
tinypath_t font_t::get_shape(int cp, int height, std::vector<vertex_f>* opt, int adv, float scale1)
{
	int vc = 0;
	tinypath_t r = {};
	auto p = &r;
	stbtt_vertex* v = stb_font::get_char_shape(font, cp, vc);
	if (v)
	{
		if (p && vc > 1)
		{
			auto bs = get_shape_box(cp, height);
			r.advance = bs.x;
			r.bearing = { bs.y,0 };
			auto pss = opt->size();
			r.first = pss;
			opt->resize(pss + vc);
			r.v = opt->data() + pss;
			p->count = vc;
			for (size_t i = 0; i < vc; i++)
			{
				r.v[i].type = v[i].type;
				r.v[i].x = v[i].x;
				r.v[i].y = v[i].y;
				r.v[i].cx = v[i].cx;
				r.v[i].cy = v[i].cy;
				r.v[i].cx1 = v[i].cx1;
				r.v[i].cy1 = v[i].cy1;
			}
			if (height != 0)
			{
				float scale = get_scale(height) * scale1;
				if (scale > 0)
				{
					r.baseline = ascender * scale;
					auto v1 = r.v;
					for (size_t i = 0; i < vc; i++)
					{
						v1->x *= scale;
						v1->y *= scale;
						v1->cx *= scale;
						v1->cy *= scale;
						v1->cx1 *= scale;
						v1->cy1 *= scale;
						v1++;
					}
				}
			}
			{
				auto v1 = r.v;
				for (size_t i = 0; i < vc; i++)
				{
					v1->x += adv;
					v1->cx += adv;
					v1->cx1 += adv;
					v1++;
				}
			}
		}
		stb_font::free_shape(font, v);
	}
	return r;
}

tinypath_t font_t::get_shape_gid(int gid, int height, std::vector<vertex_f>* opt, int adv, float scale1)
{
	int vc = 0;
	tinypath_t r = {};
	auto p = &r;
	stbtt_vertex* v = stb_font::get_char_shape_idx(font, gid, vc);
	if (v)
	{
		if (p && vc > 1)
		{
			glm::vec4 box = {};
			auto bs = get_shape_box_glyph(gid, height, &box);
			r.advance = bs.x;
			r.bearing = { bs.y,0 };
			auto pss = opt->size();
			r.first = pss;
			opt->resize(pss + vc);
			r.v = opt->data() + pss;
			p->count = vc;
			for (size_t i = 0; i < vc; i++)
			{
				r.v[i].type = v[i].type;
				r.v[i].x = v[i].x;
				r.v[i].y = v[i].y;
				r.v[i].cx = v[i].cx;
				r.v[i].cy = v[i].cy;
				r.v[i].cx1 = v[i].cx1;
				r.v[i].cy1 = v[i].cy1;
			}
			if (height != 0)
			{
				float scale = get_scale(height) * scale1;
				if (scale > 0)
				{
					r.baseline = ascender * scale;
					auto v1 = r.v;
					for (size_t i = 0; i < vc; i++)
					{
						v1->x *= scale;
						v1->y *= scale;
						v1->cx *= scale;
						v1->cy *= scale;
						v1->cx1 *= scale;
						v1->cy1 *= scale;
						v1++;
					}
				}
			}
			{
				auto v1 = r.v;
				for (size_t i = 0; i < vc; i++)
				{
					v1->x += adv;
					v1->cx += adv;
					v1->cx1 += adv;
					v1++;
				}
			}
		}
		stb_font::free_shape(font, v);
	}
	return r;
}

// todo 获取字符大小

glm::ivec4 font_t::get_char_extent(char32_t ch, unsigned char font_size, /*unsigned short font_dpi,*/ std::vector<font_t*>* fallbacks, font_t** oft)
{
	ft_char_s cs;
	cs.v.font_dpi = 0;// font_dpi;
	cs.v.font_size = font_size;
	cs.v.unicode_codepoint = ch;
#if 0
	{
		auto it = _char_lut.find(cs.u);
		if (it != _char_lut.end())
		{
			return it->second;
		}
	}
#endif
	glm::ivec4 ret = {};
	font_t* rfont = nullptr;
	auto g = get_glyph_index(ch, &rfont, fallbacks);
	if (g)
	{
		if (oft)*oft = rfont;
		double fns = font_size;// round((double)font_size * font_dpi / 72.0);
		double scale = rfont->get_scale(fns);
		int x0 = 0, y0 = 0, x1 = 0, y1 = 0, advance, lsb;
		stb_font::buildGlyphBitmap(rfont->font, g, scale, &advance, &lsb, &x0, &y0, &x1, &y1);
		double adv = scale * advance;
		auto bl = rfont->get_base_line(font_size);
		ret = { x1 - x0, y1 - y0, adv, bl };
		//_char_lut[cs.u] = ret;
	}
	return ret;
}

void font_t::clear_char_lut()
{
	//_char_lut.clear();
}

const char* font_t::get_glyph_index_u8(const char* u8str, int* oidx, font_t** renderFont, std::vector<font_t*>* fallbacks)
{
	int g = 0;
	const char* str = u8str;
	if (fallbacks)
	{
		for (auto it : *fallbacks)
		{
			str = stb_font::get_glyph_index(it->font, u8str, &g);
			if (g > 0) {
				*renderFont = it;
				*oidx = g;
				break;
			}
		}
	}
	return str;
}

union gcache_key
{
	uint64_t u;
	struct {
		uint32_t glyph_index;
		uint32_t height;
	}v;
};
font_item_t* font_t::push_gcache(uint32_t glyph_index, uint32_t height, image_ptr_t* img, const glm::ivec4& rect, const glm::ivec2& pos)
{
	gcache_key k = {}; k.v.glyph_index = glyph_index; k.v.height = height;
	auto& pt = _cache_glyphidx[k.u];
	if (!pt)
	{
		if (cache_count == 0)
		{
			cache_data.push_back(new font_item_t[ccount]);
			cache_count = ccount;
		}
		auto npt = cache_data.rbegin();
		pt = *npt;
		pt += ccount - cache_count;
		cache_count--;
	}
	pt->_glyph_index = glyph_index;
	pt->_image = img;
	pt->_dwpos = pos;
	pt->_rect = rect;
	return pt;
}
font_item_t* font_t::get_gcache(uint32_t glyph_index, uint32_t height)
{
	gcache_key k = {}; k.v.glyph_index = glyph_index; k.v.height = height;
	font_item_t* ret = 0;
	auto it = _cache_glyphidx.find(k.u);
	if (it != _cache_glyphidx.end())
	{
		ret = it->second;
	}
	return ret;
}
void font_t::clear_gcache()
{
	for (auto it : cache_data)
	{
		delete[]it;
	}
	cache_data.clear();
	_cache_glyphidx.clear();
}

int font_t::get_glyph_index(uint32_t codepoint, font_t** renderFont, std::vector<font_t*>* fallbacks)
{
	int g = stb_font::getGlyphIndex(font, codepoint);
	if (g == 0) {
		if (fallbacks)
		{
			for (auto it : *fallbacks)
			{
				if (font == it->font)continue;
				int fallbackIndex = stb_font::getGlyphIndex(it->font, codepoint);
				if (fallbackIndex != 0) {
					g = fallbackIndex;
					if (renderFont)
						*renderFont = it;
					break;
				}
			}
		}
	}
	else
	{
		if (renderFont)
			*renderFont = this;
	}
	return g;
}

int font_t::get_glyph_index0(uint32_t codepoint, font_t** renderFont, font_family_t* fallbacks)
{
	int g = stb_font::getGlyphIndex(font, codepoint);
	if (g == 0) {
		if (fallbacks)
		{
			for (size_t i = 0; i < fallbacks->count; i++)
			{
				auto it = fallbacks->familys[i];
				if (font == it->font)continue;
				int fallbackIndex = stb_font::getGlyphIndex(it->font, codepoint);
				if (fallbackIndex != 0) {
					g = fallbackIndex;
					if (renderFont)
						*renderFont = it;
					break;
				}
			}
		}
	}
	else
	{
		if (renderFont)
			*renderFont = this;
	}
	return g;
}

font_item_t font_t::get_glyph_item(uint32_t glyph_index, uint32_t unicode_codepoint, int fontsize, bitmap_cache_cx* use_ctx)
{
	uint32_t col = -1;
	int lcd_type = 0;
	int linegap = 4;
	font_t* rfont = this;
	Bitmap_p bitmap[1] = {};
	std::vector<char> bitbuf[1];
	glm::ivec4 rc;
	std::vector<font_item_t>  ps;
	glm::ivec3 rets;
	font_item_t ret = {};
	if (!use_ctx)
	{
		use_ctx = bc_ctx;
	}
	if (-1 == glyph_index)
		glyph_index = get_glyph_index(unicode_codepoint, &rfont, 0);
	if (rfont && glyph_index)
	{
		auto rp = rfont->get_gcache(glyph_index, fontsize);
		do {
			if (rp)break;
			auto bit = rfont->get_glyph_image(glyph_index, fontsize, &rc, bitmap, 0, lcd_type, unicode_codepoint);
			if (colorinfo && bit && !use_no_color)
			{
				glm::ivec4 bs = { rc.x, rc.y, bitmap->width, bitmap->rows };
				std::vector<glm::uvec2> cols;	// uvec2.x=glyph_index,.y=color
				if (get_gcolor(glyph_index, cols))
				{
					glm::ivec2 pos = {};
					image_ptr_t* img = 0;
					glm::ivec4 maxbox = { MAXINT,MAXINT,0,0 };
					for (size_t i = 0; i < cols.size(); i++)
					{
						glm::vec4 abox = {};
						rfont->get_shape_box_glyph(cols[i].x, fontsize, &abox);
						maxbox.x = std::min(maxbox.x, (int)abox.x);
						maxbox.y = std::min(maxbox.y, (int)abox.y);
						maxbox.z = std::max(maxbox.z, (int)(abox.z));
						maxbox.w = std::max(maxbox.w, (int)(abox.w));
					}
					bs = maxbox;
					bitmap->width = bitmap->pitch = std::max(rc.z, bs.z - bs.x);// 重置组合大小
					bitmap->rows = std::max(rc.w, bs.w - bs.y);
					bs.z = bitmap->width;
					bs.w = bitmap->rows;
					img = use_ctx->push_cache_bitmap_old(bitmap, &pos, 0, img, linegap); // 申请缓存位置 
					glm::vec2 dpos = pos;
					dpos.x += bs.x < 0 ? abs(bs.x) : 0;
					dpos.y += abs(bs.y);
					for (auto& ct : cols)
					{
						bitbuf->clear();
						bitbuf->resize(bitmap->width * bitmap->rows);
						// 光栅化单个颜色块
						auto bit = rfont->get_glyph_image(ct.x, fontsize, &rc, bitmap, bitbuf, lcd_type, unicode_codepoint, 0);
						if (bit)
						{
							glm::ivec2 ps1 = dpos;
							ps1.x += (bitmap->x) - bs.x, ps1.y += (bitmap->y);
							img = use_ctx->push_cache_bitmap_old(bitmap, &ps1, ct.y, img, linegap);
						}
					}
					glm::ivec4 rc4 = { pos.x, pos.y, bs.z, bs.w };
					if (img)
					{
						// 提交到查询缓存
						rp = rfont->push_gcache(glyph_index, fontsize, img, rc4, bs);
						if (rp)
						{
							rp->color = -1;
							rp->advance = bitmap->advance;
						}
					}
					break;
				}
			}
			bit = rfont->get_glyph_image(glyph_index, fontsize, &rc, bitmap, bitbuf, lcd_type, unicode_codepoint);
			if (bit)
			{
				glm::ivec2 pos = {};
				auto img = use_ctx->push_cache_bitmap(bitmap, &pos, linegap, col);//申请缓存位置
				glm::ivec4 rc4 = { pos.x, pos.y, bitmap->width, bitmap->rows };
				if (img)
				{
					rp = rfont->push_gcache(glyph_index, fontsize, img, rc4, { rc.x, rc.y });
					rp->color = 0;
					if (rp)rp->advance = bitmap->advance;
				}
			}
		} while (0);
		if (rp)
		{
			ret = *rp;
		}
	}
	return ret;
}



#ifndef _FONT_NO_BITMAP

#ifdef NO_CPU_LENDIAN
#define UINT8_BITFIELD_BENDIAN
#else
#define UINT8_BITFIELD_LENDIAN
#endif
#ifdef UINT8_BITFIELD_LENDIAN
#define UINT8_BITFIELD(f0, f1, f2, f3, f4, f5, f6, f7) \
        uint8_t f0 : 1; \
        uint8_t f1 : 1; \
        uint8_t f2 : 1; \
        uint8_t f3 : 1; \
        uint8_t f4 : 1; \
        uint8_t f5 : 1; \
        uint8_t f6 : 1; \
        uint8_t f7 : 1;
#else
#define UINT8_BITFIELD(f0, f1, f2, f3, f4, f5, f6, f7) \
        uint8_t f7 : 1; \
        uint8_t f6 : 1; \
        uint8_t f5 : 1; \
        uint8_t f4 : 1; \
        uint8_t f3 : 1; \
        uint8_t f2 : 1; \
        uint8_t f1 : 1; \
        uint8_t f0 : 1;
#endif
#define BYTE_BITFIELD UINT8_BITFIELD
// EBLC头用到的结构
struct SbitLineMetrics {
	char ascender;
	char descender;
	uint8_t widthMax;
	char caretSlopeNumerator;
	char caretSlopeDenominator;
	char caretOffset;
	char minOriginSB;
	char minAdvanceSB;
	char maxBeforeBL;
	char minAfterBL;
	char pad1;
	char pad2;
};

struct BigGlyphMetrics {
	uint8_t height;
	uint8_t width;
	char horiBearingX;
	char horiBearingY;
	uint8_t horiAdvance;
	char vertBearingX;
	char vertBearingY;
	uint8_t vertAdvance;
};

struct SmallGlyphMetrics {
	uint8_t height;
	uint8_t width;
	char bearingX;
	char bearingY;
	uint8_t advance;
};
struct BitmapSizeTable {
	uint32_t indexSubTableArrayOffset; //offset to indexSubtableArray from beginning of EBLC.
	uint32_t indexTablesSize; //number of bytes in corresponding index subtables and array
	uint32_t numberOfIndexSubTables; //an index subtable for each range or format change
	uint32_t colorRef; //not used; set to 0.
	SbitLineMetrics hori; //line metrics for text rendered horizontally
	SbitLineMetrics vert; //line metrics for text rendered vertically
	unsigned short startGlyphIndex; //lowest glyph index for this size
	unsigned short endGlyphIndex; //highest glyph index for this size
	uint8_t ppemX; //horizontal pixels per Em
	uint8_t ppemY; //vertical pixels per Em
	struct BitDepth {
		enum Value : uint8_t {
			BW = 1,
			Gray4 = 2,
			Gray16 = 4,
			Gray256 = 8,
		};
		uint8_t value;
	} bitDepth; //the Microsoft rasterizer v.1.7 or greater supports
	union Flags {
		struct Field {
			//0-7
			BYTE_BITFIELD(
				Horizontal, // Horizontal small glyph metrics
				Vertical,  // Vertical small glyph metrics
				Reserved02,
				Reserved03,
				Reserved04,
				Reserved05,
				Reserved06,
				Reserved07)
		} field;
		struct Raw {
			static const char Horizontal = 1u << 0;
			static const char Vertical = 1u << 1;
			char value;
		} raw;
	} flags;
}; //bitmapSizeTable[numSizes];

struct IndexSubTableArray {
	unsigned short firstGlyphIndex; //first glyph code of this range
	unsigned short lastGlyphIndex; //last glyph code of this range (inclusive)
	uint32_t additionalOffsetToIndexSubtable; //add to BitmapSizeTable::indexSubTableArrayOffset to get offset from beginning of 'EBLC'
}; //indexSubTableArray[BitmapSizeTable::numberOfIndexSubTables];

struct IndexSubHeader {
	unsigned short indexFormat; //format of this indexSubTable
	unsigned short imageFormat; //format of 'EBDT' image data
	uint32_t imageDataOffset; //offset to image data in 'EBDT' table
};

// Variable metrics glyphs with 4 byte offsets
struct IndexSubTable1 {
	IndexSubHeader header;
	//uint32_t offsetArray[lastGlyphIndex - firstGlyphIndex + 1 + 1]; //last element points to one past end of last glyph
	//glyphData = offsetArray[glyphIndex - firstGlyphIndex] + imageDataOffset
};

// All Glyphs have identical metrics
struct IndexSubTable2 {
	IndexSubHeader header;
	uint32_t imageSize; // all glyphs are of the same size
	BigGlyphMetrics bigMetrics; // all glyphs have the same metrics; glyph data may be compressed, byte-aligned, or bit-aligned
};

// Variable metrics glyphs with 2 byte offsets
struct IndexSubTable3 {
	IndexSubHeader header;
	//unsigned short offsetArray[lastGlyphIndex - firstGlyphIndex + 1 + 1]; //last element points to one past end of last glyph, may have extra element to force even number of elements
	//glyphData = offsetArray[glyphIndex - firstGlyphIndex] + imageDataOffset
};

// Variable metrics glyphs with sparse glyph codes
struct IndexSubTable4 {
	IndexSubHeader header;
	uint32_t numGlyphs;
	struct CodeOffsetPair {
		unsigned short glyphCode;
		unsigned short offset; //location in EBDT
	}; //glyphArray[numGlyphs+1]
};

// Constant metrics glyphs with sparse glyph codes
struct IndexSubTable5 {
	IndexSubHeader header;
	uint32_t imageSize; //all glyphs have the same data size
	BigGlyphMetrics bigMetrics; //all glyphs have the same metrics
	uint32_t numGlyphs;
	//unsigned short glyphCodeArray[numGlyphs] //must have even number of entries (set pad to 0)
};

union IndexSubTable {
	IndexSubHeader header;
	IndexSubTable1 format1;
	IndexSubTable2 format2;
	IndexSubTable3 format3;
	IndexSubTable4 format4;
	IndexSubTable5 format5;
};

union GlyphMetrics
{
	struct BigGlyphMetrics _big;
	struct SmallGlyphMetrics _small;
};
class SBitDecoder
{
public:
	Bitmap_p bitmap[1] = {};
	std::vector<char> bitdata;
	BigGlyphMetrics metrics[1] = {};
	bool          metrics_loaded;
	bool          bitmap_allocated;
	uint8_t          bit_depth;

	BitmapSizeTable* _bst;
	std::vector<IndexSubTableArray>* _ist;

	uint8_t* ebdt_base;
	uint8_t* eblc_base;
	uint8_t* eblc_limit;
	uint8_t* p, * p_limit;

	uint32_t _strike_index = 0;
	font_t* _face = 0;

	// 是否在回收状态
	int _recycle = 0;
public:
	SBitDecoder()
	{

	}

	~SBitDecoder()
	{
	}

	int init(font_t* ttp, uint32_t strike_index);
public:
	IndexSubTableArray* get_image_offset(uint32_t glyph_index)
	{
		auto& ist = *_ist;
		for (size_t i = 0; i < ist.size(); i++)
		{
			auto& it = ist[i];
			if (glyph_index >= it.firstGlyphIndex && glyph_index <= it.lastGlyphIndex)
			{
				return &ist[i];
			}
		}
		static IndexSubTableArray a;
		a.firstGlyphIndex = 0;
		a.lastGlyphIndex = 98;
		a.additionalOffsetToIndexSubtable = 0;
		return nullptr;
	}
	int resize_bitmap(uint32_t size)
	{
		if (size > bitdata.size())
		{
			bitmap->capacity = size;
			bitdata.resize(size);
			bitmap->buffer = (unsigned char*)bitdata.data();
		}
		memset(bitmap->buffer, 0, size);
		return 0;
	}
private:

};

// 获取一个字体索引的位图
int get_index(SBitDecoder* decoder, uint32_t glyph_index, int x_pos, int y_pos);
int load_metrics(uint8_t** pp, uint8_t* limit, int big, BigGlyphMetrics* metrics);

class bitmap_ttinfo
{
public:

	sbit_table_type _sttype = sbit_table_type::TYPE_NONE;
	// _sbit_table可能是CBLC、EBLC、bloc、sbix
	sfnt_header* _sbit_table = 0;
	sfnt_header* _ebdt_table = 0;
	sfnt_header* _ebsc_table = 0;
	std::vector<BitmapSizeTable> _bsts;
	std::vector<std::vector<IndexSubTableArray>> _index_sub_table;
	std::unordered_map<uint8_t, uint32_t> _msidx_table;
	std::set<SBitDecoder*> _dec_table;
	std::queue<SBitDecoder*> _free_dec;
	//LockS _sbit_lock;
	font_t* _t = 0;
	// 自定义位图
	image_ptr_t _bimg = {};
	uint32_t* _buf = 0;
	// 支持的大小
	std::vector<glm::ivec2> _chsize;
	// 范围
	std::vector<glm::ivec2> _unicode_rang;
public:
	bitmap_ttinfo() {

	}
	~bitmap_ttinfo() {
		//Image::destroy(_buf);
		if (_buf)
		{
			delete _buf; _buf = 0;
		}
		destroy_all_dec();
	}
	int get_sidx(int height)
	{
		auto it = _msidx_table.find(height);
		return it != _msidx_table.end() ? it->second : -1;
	}
	std::unordered_map<uint8_t, uint32_t>* get_msidx_table()
	{
		return &_msidx_table;
	}

	// 创建sbit解码器
	SBitDecoder* new_SBitDecoder()
	{
		SBitDecoder* p = nullptr;
		//LOCK_W(_sbit_lock);
#ifndef NO_SBIT_RECYCLE
		if (_free_dec.size())
		{
			p = _free_dec.front();
			_free_dec.pop();
			p->_recycle = 0;
		}
		else
#endif // !NO_SBIT_RECYCLE
		{
			p = new SBitDecoder();
			_dec_table.insert(p);
		}
		return p;
	}
	// 回收
	void recycle(SBitDecoder* p)
	{
		if (p)
		{
			//LOCK_W(_sbit_lock);
			// 不是自己的不回收
			auto it = _dec_table.find(p);
			if (it != _dec_table.end() && p->_recycle == 0)
			{
#ifndef NO_SBIT_RECYCLE
				p->_recycle = 1;
				_free_dec.push(p);
#else
				_dec_table.erase(p);
				delete p;
#endif
			}

		}
	}
	void destroy_all_dec()
	{
		//LOCK_W(_sbit_lock);
		for (auto it : _dec_table)
			if (it)delete it;
		_dec_table.clear();
	}

};
int SBitDecoder::init(font_t* ttp, uint32_t strike_index)
{
	int ret = 0;
	SBitDecoder* decoder = this;
	auto face = ttp->bitinfo;
	if (!face->_ebdt_table || !face->_sbit_table
		|| strike_index >= face->_bsts.size() || strike_index >= face->_index_sub_table.size())
		return 0;
	if (_face == ttp && _strike_index == strike_index)
	{
		return 1;
	}
	_face = ttp;
	_strike_index = strike_index;
	auto font_i = ttp->font;
	const stbtt_fontinfo* font = font_i;
	decoder->_bst = &face->_bsts[strike_index];
	decoder->_ist = &face->_index_sub_table[strike_index];
	uint8_t* fc = font->data;
	decoder->eblc_base = fc + face->_sbit_table->offset;
	decoder->ebdt_base = fc + face->_ebdt_table->offset;
	decoder->metrics_loaded = 0;
	decoder->bitmap_allocated = 0;
	decoder->bit_depth = decoder->_bst->bitDepth.value;
	decoder->p = decoder->eblc_base + decoder->_bst->indexSubTableArrayOffset;
	decoder->p_limit = decoder->p + decoder->_bst->indexTablesSize;
	ret = 1;
	return ret;
}

typedef int(*SBitDecoder_LoadFunc)(SBitDecoder* decoder, uint8_t* p, uint8_t* plimit, int x_pos, int y_pos);
// 复制位图到bitmap
static int load_byte_aligned(SBitDecoder* decoder, uint8_t* p, uint8_t* limit, int x_pos, int y_pos)
{
	Bitmap_p* bitmap = decoder->bitmap;
	uint8_t* line;
	int      pitch, width, height, line_bits, h;
	uint32_t     bit_height, bit_width;
	bit_width = bitmap->width;
	bit_height = bitmap->rows;
	pitch = bitmap->pitch;
	line = bitmap->buffer;

	width = bitmap->width;
	height = bitmap->rows;

	line_bits = width * bitmap->bit_depth;

	if (x_pos < 0 || (uint32_t)(x_pos + width) > bit_width ||
		y_pos < 0 || (uint32_t)(y_pos + height) > bit_height)
	{
		printf("tt_sbit_decoder_load_byte_aligned:"
			" invalid bitmap dimensions\n"); return 0;
	}
	if (p + ((line_bits + 7) >> 3) * height > limit)
	{
		printf("tt_sbit_decoder_load_byte_aligned: broken bitmap\n");
		return 0;
	}
	/* now do the blit */
	line += y_pos * pitch + (x_pos >> 3);
	x_pos &= 7;

	if (x_pos == 0)
	{
		for (h = height; h > 0; h--, line += pitch)
		{
			uint8_t* pwrite = line;
			int    w;
			for (w = line_bits; w >= 8; w -= 8)
			{
				pwrite[0] = (uint8_t)(pwrite[0] | *p++);
				pwrite += 1;
			}
			if (w > 0)
				pwrite[0] = (uint8_t)(pwrite[0] | (*p++ & (0xFF00U >> w)));
		}
	}
	else  /* x_pos > 0 */
	{
		for (h = height; h > 0; h--, line += pitch)
		{
			uint8_t* pwrite = line;
			int    w;
			uint32_t   wval = 0;
			for (w = line_bits; w >= 8; w -= 8)
			{
				wval = (uint32_t)(wval | *p++);
				pwrite[0] = (uint8_t)(pwrite[0] | (wval >> x_pos));
				pwrite += 1;
				wval <<= 8;
			}
			if (w > 0)
				wval = (uint32_t)(wval | (*p++ & (0xFF00U >> w)));
			/* 读取所有位，并有'x_pos+w'位要写入 */
			pwrite[0] = (uint8_t)(pwrite[0] | (wval >> x_pos));

			if (x_pos + w > 8)
			{
				pwrite++;
				wval <<= 8;
				pwrite[0] = (uint8_t)(pwrite[0] | (wval >> x_pos));
			}
		}
	}

	return 1;
}
// 按像素位复制
static int load_bit_aligned(SBitDecoder* decoder, uint8_t* p, uint8_t* limit, int x_pos, int y_pos)
{
	Bitmap_p* bitmap = decoder->bitmap;
	int ret = 1;
	uint8_t* line;
	int      pitch, width, height, line_bits, h, nbits;
	uint32_t     bit_height, bit_width;
	unsigned short   rval;

	bit_width = bitmap->width;
	bit_height = bitmap->rows;
	pitch = bitmap->pitch;
	line = bitmap->buffer;

	width = bit_width;
	height = bit_height;

	line_bits = width * bitmap->bit_depth;

	if (x_pos < 0 || (uint32_t)(x_pos + width) > bit_width ||
		y_pos < 0 || (uint32_t)(y_pos + height) > bit_height)
	{
		printf("tt_sbit_decoder_load_bit_aligned:"
			" invalid bitmap dimensions\n");
		return 0;
	}

	if (p + ((line_bits * height + 7) >> 3) > limit)
	{
		printf("tt_sbit_decoder_load_bit_aligned: broken bitmap\n");
		return 0;
	}

	if (!line_bits || !height)
	{
		/* nothing to do */
		return 0;
	}

	/* now do the blit */

	/* adjust `line' to point to the first byte of the bitmap */
	line += (uint64_t)y_pos * pitch + (x_pos >> 3);
	x_pos &= 7;

	/* the higher byte of `rval' is used as a buffer */
	rval = 0;
	nbits = 0;

	for (h = height; h > 0; h--, line += pitch)
	{
		uint8_t* pwrite = line;
		int    w = line_bits;
		/* handle initial byte (in target bitmap) specially if necessary */
		if (x_pos)
		{
			w = (line_bits < 8 - x_pos) ? line_bits : 8 - x_pos;
			if (h == height)
			{
				rval = *p++;
				nbits = x_pos;
			}
			else if (nbits < w)
			{
				if (p < limit)
					rval |= *p++;
				nbits += 8 - w;
			}
			else
			{
				rval >>= 8;
				nbits -= w;
			}

			*pwrite++ |= ((rval >> nbits) & 0xFF) &
				(~(0xFFU << w) << (8 - w - x_pos));
			rval <<= 8;
			w = line_bits - w;
		}

		/* handle medial bytes */
		for (; w >= 8; w -= 8)
		{
			rval |= *p++;
			*pwrite++ |= (rval >> nbits) & 0xFF;
			rval <<= 8;
		}

		/* handle final byte if necessary */
		if (w > 0)
		{
			if (nbits < w)
			{
				if (p < limit)
					rval |= *p++;
				*pwrite |= ((rval >> nbits) & 0xFF) & (0xFF00U >> w);
				nbits += 8 - w;

				rval <<= 8;
			}
			else
			{
				*pwrite |= ((rval >> nbits) & 0xFF) & (0xFF00U >> w);
				nbits -= w;
			}
		}
	}
	return ret;
}

// 解码复制
static int load_compound(SBitDecoder* decoder, uint8_t* p, uint8_t* limit, int x_pos, int y_pos)
{
	Bitmap_p* bitmap = decoder->bitmap;
	int   num_components, error = 0;
	num_components = stb_font::ttUSHORT(p); p += 2;
	for (int i = 0; i < num_components; i++)
	{
		uint32_t  gindex = stb_font::ttUSHORT(p); p += 2;
		char  dx = *p; p++;
		char  dy = *p; p++;
		/* NB: a recursive call */
		error = get_index(decoder, gindex, x_pos + dx, y_pos + dy);
		if (error)
			break;
	}
	return 0;
}
// 分配位置空间
static int alloc_bitmap(SBitDecoder* decoder)
{
	uint32_t     width, height;
	uint32_t    size;
	Bitmap_p* bitmap = decoder->bitmap;
	int bit_depth = decoder->bit_depth;
	BigGlyphMetrics* metrics = decoder->metrics;
	width = metrics->width;
	height = metrics->height;

	bitmap->width = width;
	bitmap->rows = height;
	bitmap->bit_depth = bit_depth;

	switch (bit_depth)
	{
	case 1:
		bitmap->pixel_mode = Pixel_Mode::PX_MONO;
		bitmap->pitch = (int)((bitmap->width + 7) >> 3);
		bitmap->num_grays = 2;
		break;

	case 2:
		bitmap->pixel_mode = Pixel_Mode::PX_GRAY2;
		bitmap->pitch = (int)((bitmap->width + 3) >> 2);
		bitmap->num_grays = 4;
		break;

	case 4:
		bitmap->pixel_mode = Pixel_Mode::PX_GRAY4;
		bitmap->pitch = (int)((bitmap->width + 1) >> 1);
		bitmap->num_grays = 16;
		break;

	case 8:
		bitmap->pixel_mode = Pixel_Mode::PX_GRAY;
		bitmap->pitch = (int)(bitmap->width);
		bitmap->num_grays = 256;
		break;

	case 32:
		bitmap->pixel_mode = Pixel_Mode::PX_BGRA;
		bitmap->pitch = (int)(bitmap->width * 4);
		bitmap->num_grays = 256;
		break;

	default:
		return 0;
	}

	size = bitmap->rows * (uint32_t)bitmap->pitch;

	/* check that there is no empty image */
	if (size == 0)
		return 0;     /* exit successfully! */
	decoder->resize_bitmap(size);
	return 1;
}
// 解析加载位图
static int load_bitmap(
	SBitDecoder* decoder,
	uint32_t glyph_format,
	uint32_t glyph_start,
	uint32_t glyph_size,
	int x_pos, int y_pos)
{
	uint8_t* p, * p_limit, * data;
	data = decoder->ebdt_base + glyph_start;
	p = data;
	p_limit = p + glyph_size;
	//uint8_t* ebdt_base,  Bitmap_p* bitmap,
	// 根据字形格式glyph_format读取数据int bit_depth,
	BigGlyphMetrics* metrics = decoder->metrics;
	switch (glyph_format)
	{
	case 1:
	case 2:
	case 8:
	case 17:
		load_metrics(&p, p_limit, 0, metrics);
		break;
	case 6:
	case 7:
	case 9:
	case 18:
		load_metrics(&p, p_limit, 1, metrics);
		break;
	default:
		return 0;
	}
	SBitDecoder_LoadFunc  loader = 0;
	int ret = 0;
	{
		switch (glyph_format)
		{
		case 1:
		case 6:
			loader = load_byte_aligned;
			break;

		case 2:
		case 7:
		{
			/* Don't trust `glyph_format'.  For example, Apple's main Korean */
			/* system font, `AppleMyungJo.ttf' (version 7.0d2e6), uses glyph */
			/* format 7, but the data is format 6.  We check whether we have */
			/* an excessive number of bytes in the image: If it is equal to  */
			/* the value for a byte-aligned glyph, use the other loading     */
			/* routine.                                                      */
			/*                                                               */
			/* Note that for some (width,height) combinations, where the     */
			/* width is not a multiple of 8, the sizes for bit- and          */
			/* byte-aligned data are equal, for example (7,7) or (15,6).  We */
			/* then prefer what `glyph_format' specifies.                    */

			int  width = metrics->width;
			int  height = metrics->height;

			int  bit_size = (width * height + 7) >> 3;
			int  byte_size = height * ((width + 7) >> 3);


			if (bit_size < byte_size &&
				byte_size == (int)(p_limit - p))
				loader = load_byte_aligned;
			else
				loader = load_bit_aligned;
		}
		break;

		case 5:
			loader = load_bit_aligned;
			break;

		case 8:
			if (p + 1 > p_limit)
				return 0;

			p += 1;  /* skip padding */
			/* fall-through */

		case 9:
			loader = load_compound;
			break;

		case 17: /* small metrics, PNG image data   */
		case 18: /* big metrics, PNG image data     */
		case 19: /* metrics in EBLC, PNG image data */
#ifdef FT_CONFIG_OPTION_USE_PNG
			loader = load_png;
			break;
#else
			return 0;
#endif /* FT_CONFIG_OPTION_USE_PNG */

		default:
			return 0;
		}
		ret = alloc_bitmap(decoder);
		if (!ret)
			return ret;
	}

	ret = loader(decoder, p, p_limit, x_pos, y_pos);

	return ret;
}

// 获取一个字体索引的位图
int get_index(SBitDecoder* decoder, uint32_t glyph_index, int x_pos, int y_pos)
{
	uint32_t image_start = 0, image_end = 0, image_offset = 0;
	uint32_t   start, end, index_format, image_format;
	uint8_t* p = decoder->p, * p_limit = decoder->p_limit;
	auto it = decoder->get_image_offset(glyph_index);
	if (!it)
	{
		return 0;
	}
	start = it->firstGlyphIndex;
	end = it->lastGlyphIndex;
	image_offset = it->additionalOffsetToIndexSubtable;
	if (!image_offset)
	{
		//return 0;
	}
	p += image_offset;
	//p_limit
	index_format = stb_font::ttUSHORT(p);
	image_format = stb_font::ttUSHORT(p + 2);
	image_offset = stb_font::ttULONG(p + 4);
	p += 8;
	switch (index_format)
	{
	case 1: /* 4-byte offsets relative to `image_offset' */
		p += (uint64_t)4 * (glyph_index - start);
		if (p + 8 > p_limit)
			return 0;
		image_start = stb_font::ttULONG(p);
		p += 4;
		image_end = stb_font::ttULONG(p);
		p += 4;
		if (image_start == image_end)  /* missing glyph */
			return 0;
		break;
	case 2: /* big metrics, constant image size */
	{
		uint32_t  image_size;
		if (p + 12 > p_limit)
			return 0;
		image_size = stb_font::ttULONG(p);
		p += 4;
		if (!load_metrics(&p, p_limit, 1, decoder->metrics))return 0;
		image_start = image_size * (glyph_index - start);
		image_end = image_start + image_size;
	}
	break;
	case 3: /* 2-byte offsets relative to 'image_offset' */
		p += (uint64_t)2 * (glyph_index - start);
		if (p + 4 > p_limit)
			return 0;
		image_start = stb_font::ttUSHORT(p);
		p += 2;
		image_end = stb_font::ttUSHORT(p);
		p += 2;
		if (image_start == image_end)  /* missing glyph */
			return 0;
		break;
	case 4: /* sparse glyph array with (glyph,offset) pairs */
	{
		uint32_t  mm, num_glyphs;
		if (p + 4 > p_limit)
			return 0;
		num_glyphs = stb_font::ttULONG(p);
		p += 4;
		/* overflow check for p + ( num_glyphs + 1 ) * 4 */
		if (p + 4 > p_limit ||
			num_glyphs > (uint32_t)(((p_limit - p) >> 2) - 1))
			return 0;
		for (mm = 0; mm < num_glyphs; mm++)
		{
			uint32_t  gindex = stb_font::ttUSHORT(p);
			p += 2;
			if (gindex == glyph_index)
			{
				image_start = stb_font::ttUSHORT(p);
				p += 4;
				image_end = stb_font::ttUSHORT(p);
				break;
			}
			p += 2;
		}
		if (mm >= num_glyphs)
			return 0;
	}
	break;
	case 5: /* constant metrics with sparse glyph codes */
	case 19:
	{
		uint32_t  image_size, mm, num_glyphs;
		if (p + 16 > p_limit)
			return 0;
		image_size = stb_font::ttULONG(p);
		p += 4;
		if (!load_metrics(&p, p_limit, 1, decoder->metrics))return 0;
		num_glyphs = stb_font::ttULONG(p);
		p += 4;
		/* overflow check for p + 2 * num_glyphs */
		if (num_glyphs > (uint32_t)((p_limit - p) >> 1))
			return 0;

		for (mm = 0; mm < num_glyphs; mm++)
		{
			uint32_t  gindex = stb_font::ttUSHORT(p);
			p += 2;
			if (gindex == glyph_index)
				break;
		}
		if (mm >= num_glyphs)
			return 0;
		image_start = image_size * mm;
		image_end = image_start + image_size;
	}
	break;

	default:
		return 0;
	}
	if (image_start > image_end)
	{
		return 0;
	}
	image_end -= image_start;
	image_start = image_offset + image_start;
	return load_bitmap(decoder, image_format, image_start, image_end, x_pos, y_pos);
}

// 获取子表
static void get_index_sub_table(uint8_t* d, uint32_t n, std::vector<IndexSubTableArray>& out)
{
	out.resize(n);
	for (int i = 0; i < n; i++)
	{
		out[i].firstGlyphIndex = stb_font::ttUSHORT(d);
		out[i].lastGlyphIndex = stb_font::ttUSHORT(d + 2);
		out[i].additionalOffsetToIndexSubtable = stb_font::ttULONG(d + 4);
		d += 8;
	}
	return;
}
// 获取BitmapSizeTable
njson get_bitmap_size_table(uint8_t* blc, uint32_t count,
	std::vector<BitmapSizeTable>& bsts,
	std::vector<std::vector<IndexSubTableArray>>& index_sub_table, std::unordered_map<uint8_t, uint32_t>& ms)
{
	auto b = blc + 8;
	bsts.resize(count);
	index_sub_table.resize(count);
	njson ns, n;
	for (size_t i = 0; i < count; i++)
	{
		auto& it = bsts[i];
		it.indexSubTableArrayOffset = stb_font::ttULONG(b); //offset to indexSubtableArray from beginning of EBLC.
		b += 4;
		it.indexTablesSize = stb_font::ttULONG(b); //number of bytes in corresponding index subtables and array
		b += 4;
		it.numberOfIndexSubTables = stb_font::ttULONG(b); //an index subtable for each range or format change
		b += 4;
		it.colorRef = stb_font::ttULONG(b); //not used; set to 0.
		b += 4;
		memcpy(&it.hori, b, sizeof(SbitLineMetrics));
		b += sizeof(SbitLineMetrics);
		memcpy(&it.vert, b, sizeof(SbitLineMetrics));
		b += sizeof(SbitLineMetrics);
		it.startGlyphIndex = stb_font::ttUSHORT(b); //lowest glyph index for this size
		b += 2;
		it.endGlyphIndex = stb_font::ttUSHORT(b); //highest glyph index for this size
		b += 2;
		it.ppemX = *b; //horizontal pixels per Em
		b += 1;
		it.ppemY = *b; //vertical pixels per Em
		b += 1;
		it.bitDepth.value = *b;
		b += 1;
		it.flags.raw.value = *b;
		b += 1;
		get_index_sub_table(blc + it.indexSubTableArrayOffset, it.numberOfIndexSubTables, index_sub_table[i]);
		n["ppem"] = { it.ppemX, it.ppemY };
		ms[it.ppemY] = i;
		n["id"] = i;
		ns.push_back(n);
	}
	return ns;
}
int load_metrics(uint8_t** pp, uint8_t* limit, int big, BigGlyphMetrics* metrics)
{
	uint8_t* p = *pp;
	if (p + 5 > limit)
		return 0;
	metrics->height = p[0];
	metrics->width = p[1];
	metrics->horiBearingX = (char)p[2];
	metrics->horiBearingY = (char)p[3];
	metrics->horiAdvance = p[4];
	p += 5;
	if (big)
	{
		if (p + 3 > limit)
			return 0;
		metrics->vertBearingX = (char)p[0];
		metrics->vertBearingY = (char)p[1];
		metrics->vertAdvance = p[2];
		p += 3;
	}
	else
	{
		/* avoid uninitialized data in case there is no vertical info -- */
		metrics->vertBearingX = 0;
		metrics->vertBearingY = 0;
		metrics->vertAdvance = 0;
	}
	*pp = p;
	return 1;
}


unsigned char fdata[2180] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
	0x00, 0x00, 0x02, 0xF0, 0x00, 0x00, 0x00, 0x2A, 0x01, 0x03, 0x00, 0x00, 0x00, 0xEF, 0x52, 0xFD,
	0x59, 0x00, 0x00, 0x00, 0x03, 0x73, 0x42, 0x49, 0x54, 0x08, 0x08, 0x08, 0xDB, 0xE1, 0x4F, 0xE0,
	0x00, 0x00, 0x00, 0x06, 0x50, 0x4C, 0x54, 0x45, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x55, 0xC2,
	0xD3, 0x7E, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0E, 0x9C, 0x00, 0x00,
	0x0E, 0x9C, 0x01, 0x07, 0x94, 0x53, 0xDD, 0x00, 0x00, 0x00, 0x1C, 0x74, 0x45, 0x58, 0x74, 0x53,
	0x6F, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x00, 0x41, 0x64, 0x6F, 0x62, 0x65, 0x20, 0x46, 0x69,
	0x72, 0x65, 0x77, 0x6F, 0x72, 0x6B, 0x73, 0x20, 0x43, 0x53, 0x36, 0xE8, 0xBC, 0xB2, 0x8C, 0x00,
	0x00, 0x00, 0x16, 0x74, 0x45, 0x58, 0x74, 0x43, 0x72, 0x65, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x20,
	0x54, 0x69, 0x6D, 0x65, 0x00, 0x31, 0x32, 0x2F, 0x31, 0x30, 0x2F, 0x32, 0x30, 0x4A, 0xBE, 0xBD,
	0xE5, 0x00, 0x00, 0x07, 0xCB, 0x49, 0x44, 0x41, 0x54, 0x48, 0x89, 0xAD, 0xD6, 0x5F, 0x6C, 0x14,
	0xC7, 0x19, 0x00, 0x70, 0x13, 0xA3, 0x1C, 0x8A, 0x48, 0x5C, 0x13, 0xA9, 0xBA, 0x07, 0x83, 0xA3,
	0x82, 0x1A, 0xF1, 0xD0, 0x2A, 0x4D, 0x54, 0xC5, 0xFC, 0x8B, 0x41, 0xED, 0x0B, 0x2A, 0x6D, 0xF2,
	0x80, 0xED, 0x2A, 0x04, 0xDC, 0x88, 0x4A, 0xA9, 0x82, 0x8C, 0x4D, 0x08, 0x77, 0x29, 0x9B, 0xE3,
	0x72, 0x3D, 0xA9, 0x26, 0xAA, 0xCB, 0x29, 0x42, 0x15, 0x95, 0x90, 0xB9, 0x07, 0x94, 0xA2, 0xE0,
	0x82, 0x43, 0x2D, 0xDF, 0x62, 0x6F, 0xC6, 0x1B, 0xB3, 0x84, 0x93, 0xD5, 0x86, 0x93, 0x52, 0xA1,
	0x85, 0x2E, 0x7B, 0x13, 0x74, 0x45, 0x2B, 0xD5, 0xEC, 0x7D, 0xB6, 0x4E, 0xEE, 0x60, 0x8F, 0xF7,
	0xA6, 0xDF, 0xEC, 0xDE, 0x81, 0x6D, 0x6C, 0xE7, 0x8F, 0x3A, 0x2F, 0x77, 0x3B, 0xBB, 0xFB, 0x9B,
	0x6F, 0xBF, 0xF9, 0x66, 0x76, 0x6B, 0x44, 0xBB, 0x10, 0xDC, 0xD3, 0xC5, 0xD7, 0x68, 0xB3, 0xF1,
	0x54, 0x1C, 0x16, 0xE9, 0x2F, 0x2B, 0x67, 0x5E, 0xA9, 0x59, 0xA2, 0x89, 0x76, 0x26, 0x38, 0xA7,
	0xF2, 0xB2, 0xAF, 0x1A, 0xC3, 0x83, 0xA2, 0xCE, 0x66, 0xC5, 0xB4, 0x48, 0xE7, 0xE8, 0x40, 0x76,
	0x0E, 0xCF, 0xED, 0x8F, 0x97, 0xE2, 0xF3, 0xC3, 0x05, 0x35, 0x1A, 0xF0, 0xFB, 0xF3, 0x4D, 0x3C,
	0xD4, 0xB4, 0x3F, 0x25, 0xBC, 0xF2, 0xFD, 0xC9, 0xDB, 0x4D, 0x3F, 0x59, 0xD9, 0xB0, 0xEB, 0xF1,
	0x15, 0x8F, 0x3F, 0x73, 0xDC, 0xE8, 0x69, 0xDA, 0xF1, 0xDC, 0x67, 0x3B, 0x37, 0xAC, 0x05, 0x8C,
	0xBD, 0x2C, 0x3C, 0xD1, 0x09, 0x54, 0x63, 0x73, 0x79, 0x3A, 0xB1, 0x14, 0x4F, 0x67, 0x6E, 0xB4,
	0xEA, 0x0C, 0x8E, 0xE1, 0x65, 0x7B, 0x7B, 0xCD, 0xC3, 0xC7, 0xE8, 0xDE, 0xA8, 0xE0, 0x22, 0x7A,
	0x30, 0x6F, 0x76, 0x64, 0xAC, 0x0E, 0xAA, 0x75, 0x7E, 0xDF, 0xEC, 0x88, 0x98, 0x2D, 0x99, 0x1B,
	0x07, 0x2E, 0x10, 0x56, 0xE1, 0x31, 0x45, 0x3C, 0xE0, 0xD9, 0x57, 0xF2, 0x13, 0x6D, 0xBF, 0xD0,
	0x99, 0xBB, 0x0F, 0xAF, 0xB2, 0x6D, 0x7A, 0x38, 0x56, 0xB6, 0xA3, 0x71, 0x56, 0xAB, 0xBE, 0x7A,
	0xDD, 0x74, 0x2F, 0xAA, 0x2E, 0x1D, 0x86, 0x1F, 0xB6, 0x21, 0xCF, 0x90, 0xEF, 0xFF, 0x56, 0xFC,
	0x8C, 0xD3, 0xFA, 0x32, 0x2B, 0x36, 0x35, 0xD7, 0x96, 0x6D, 0xFA, 0xC9, 0xF8, 0x93, 0x2F, 0x15,
	0xDE, 0xF0, 0x40, 0xE4, 0xBF, 0xB8, 0xF8, 0xA2, 0x7B, 0xE2, 0xC5, 0x3B, 0x74, 0x08, 0xFE, 0xD2,
	0xB6, 0x3F, 0xB2, 0x89, 0x65, 0x06, 0xA3, 0xFD, 0x31, 0xC9, 0x5F, 0xDF, 0x7A, 0xE8, 0x7A, 0xC3,
	0xFA, 0xF4, 0xE6, 0xE8, 0xEA, 0xAD, 0xC6, 0xD5, 0x3F, 0xBF, 0xBE, 0xE1, 0x4F, 0x3B, 0x9F, 0x6F,
	0xE0, 0xB0, 0x0C, 0x5F, 0xFA, 0xA8, 0x9D, 0x15, 0x5F, 0xC2, 0x99, 0xB3, 0x69, 0x7C, 0xCF, 0xDB,
	0xB6, 0x7D, 0x4A, 0x40, 0xAD, 0xFD, 0xC5, 0x45, 0x8C, 0x3E, 0xF3, 0x03, 0x8C, 0xBE, 0x55, 0xED,
	0x8C, 0x8C, 0xB0, 0xCC, 0x30, 0xA3, 0x31, 0x8E, 0x3C, 0x66, 0x4C, 0x55, 0x3A, 0xE8, 0x30, 0x6B,
	0xB5, 0x3A, 0x32, 0x0A, 0x0E, 0x7B, 0x40, 0x1D, 0x59, 0x26, 0x7A, 0x31, 0x71, 0x64, 0xAC, 0xDD,
	0x4F, 0x8E, 0x67, 0xE7, 0xC5, 0x8E, 0x29, 0xDB, 0x66, 0xF8, 0xC8, 0xE4, 0xD6, 0xEF, 0x4D, 0x77,
	0x84, 0xC9, 0xDC, 0xB7, 0x9E, 0xEA, 0xFC, 0x2E, 0x8D, 0xAA, 0x9B, 0xC7, 0xF3, 0x9E, 0xE4, 0xAF,
	0x59, 0x6E, 0xF7, 0xBB, 0x2E, 0x1D, 0xF2, 0xF9, 0x59, 0x96, 0xB9, 0xCC, 0xD4, 0x63, 0xCB, 0xF1,
	0x33, 0x37, 0x3E, 0xEA, 0xE7, 0x54, 0xA6, 0x13, 0x65, 0xE5, 0x88, 0x6D, 0x77, 0xE2, 0xD4, 0xC6,
	0x8B, 0x19, 0x9C, 0x5A, 0x8E, 0xFC, 0xC1, 0x56, 0xB3, 0xF3, 0xA0, 0x19, 0x55, 0x8F, 0x8E, 0xE7,
	0xED, 0x4A, 0xF4, 0xBA, 0xE2, 0x56, 0xA2, 0x17, 0xB2, 0x3F, 0xB7, 0x1C, 0x4F, 0x67, 0x0A, 0x99,
	0x76, 0x2C, 0x4C, 0x28, 0xF3, 0x42, 0x4F, 0x78, 0xF2, 0xC4, 0xB6, 0x42, 0x3A, 0xE6, 0x95, 0xBD,
	0xC9, 0xE7, 0x9A, 0x8C, 0x95, 0xEF, 0xFE, 0x23, 0x74, 0xA9, 0xE6, 0x95, 0x5F, 0x66, 0x7B, 0xB6,
	0x17, 0x72, 0x47, 0xC7, 0x7A, 0xD6, 0xE2, 0xA4, 0x88, 0x2B, 0x5B, 0x27, 0xAE, 0x35, 0xFC, 0x34,
	0x35, 0xB0, 0x63, 0xF7, 0xAE, 0xC2, 0xD5, 0x77, 0x0E, 0x19, 0x47, 0xC7, 0x8A, 0xEB, 0x96, 0x8B,
	0x5E, 0xCE, 0x3E, 0xAE, 0x5A, 0x2A, 0x1E, 0x54, 0xB2, 0x37, 0x67, 0x25, 0xF9, 0x4B, 0xCD, 0xA9,
	0xAC, 0xDA, 0x58, 0x2A, 0xF8, 0xC3, 0x75, 0x7F, 0x09, 0x42, 0x70, 0x07, 0x2D, 0x2B, 0xE9, 0x65,
	0xF8, 0xE5, 0xDA, 0x6C, 0x7C, 0xFE, 0xF1, 0x44, 0x73, 0xD0, 0xEB, 0xF7, 0x67, 0x0B, 0x7E, 0x5F,
	0x33, 0x26, 0xF3, 0x5B, 0xF2, 0x4B, 0x8D, 0x2A, 0x72, 0x4B, 0x9D, 0x8A, 0xF3, 0x4F, 0xE7, 0xF1,
	0xA6, 0x88, 0x0B, 0xC7, 0xD3, 0x83, 0x73, 0x5F, 0x0B, 0x4F, 0xDD, 0xE7, 0x71, 0xB6, 0xC4, 0xB9,
	0xDC, 0x6F, 0xA7, 0xB6, 0xCF, 0xE5, 0xA1, 0xFF, 0xE0, 0x16, 0x85, 0x05, 0x9B, 0x4E, 0xD9, 0x81,
	0x54, 0xE3, 0xE5, 0x4D, 0x5F, 0x1A, 0x98, 0x63, 0xCA, 0x44, 0x61, 0x09, 0x82, 0x94, 0xE5, 0xB5,
	0x71, 0x91, 0x9E, 0x15, 0x69, 0xB8, 0x37, 0x3F, 0x7F, 0x34, 0x56, 0x9E, 0x17, 0x3D, 0xB4, 0x77,
	0xB4, 0x1C, 0x41, 0x5E, 0xFA, 0x65, 0x2B, 0x15, 0x01, 0xED, 0xBF, 0x45, 0x12, 0xC5, 0xED, 0x93,
	0x4F, 0xDA, 0xC5, 0xBA, 0x55, 0x5D, 0xCF, 0x74, 0x1D, 0x4F, 0xAE, 0xDC, 0x3E, 0xF9, 0xD8, 0x0B,
	0xC9, 0xEB, 0x75, 0xAB, 0x56, 0x3C, 0xF6, 0xC4, 0xAA, 0xFA, 0xD0, 0x48, 0x39, 0x78, 0x52, 0xCA,
	0xE3, 0x14, 0x8A, 0x62, 0x01, 0x2F, 0xE6, 0xF3, 0xB3, 0x7F, 0xDC, 0x3D, 0x05, 0x0C, 0xDA, 0x30,
	0x3D, 0x65, 0x02, 0x11, 0xD8, 0x02, 0x1E, 0x51, 0xF0, 0x6E, 0xAF, 0xF8, 0x37, 0x88, 0xB8, 0x9A,
	0x7A, 0xD1, 0x86, 0xBC, 0x3B, 0xFD, 0x42, 0x87, 0xAD, 0x46, 0xC6, 0x0F, 0xA8, 0x6F, 0xDA, 0xC3,
	0xF0, 0x90, 0xC7, 0x7A, 0x2B, 0x57, 0xF9, 0xF8, 0xE2, 0xBC, 0xE8, 0x78, 0x7B, 0x4A, 0x30, 0x48,
	0x21, 0xEF, 0x11, 0x60, 0xC5, 0xCB, 0xBF, 0x41, 0x3E, 0x57, 0x9E, 0x58, 0x57, 0x3C, 0x6F, 0x1E,
	0x71, 0x2D, 0x96, 0x47, 0xDE, 0x9E, 0x1E, 0xD8, 0x67, 0x2B, 0x11, 0xA1, 0xA8, 0x6F, 0x8E, 0x9B,
	0xDF, 0x8C, 0x2F, 0xE7, 0x8F, 0xBC, 0x15, 0x67, 0xD0, 0x1F, 0xF0, 0x70, 0x46, 0x3B, 0x17, 0x33,
	0x0E, 0xE7, 0x88, 0x10, 0x31, 0xE4, 0x3D, 0x2B, 0x42, 0x91, 0x27, 0xFC, 0xC7, 0xC8, 0xD7, 0xC5,
	0x95, 0xE1, 0xFF, 0x8C, 0x9B, 0x54, 0x97, 0xBB, 0x77, 0xB6, 0xEE, 0xA9, 0xAD, 0xBF, 0x9A, 0xF8,
	0x67, 0xEA, 0x44, 0xAA, 0xEB, 0xD7, 0x93, 0xC9, 0x2B, 0xCF, 0xAE, 0x7E, 0xFA, 0x7B, 0x4F, 0xEC,
	0xAC, 0x0F, 0x3D, 0xCA, 0xD3, 0xCD, 0xE7, 0x31, 0xFA, 0xDD, 0xA1, 0x89, 0x46, 0x8E, 0xBC, 0xAB,
	0xF7, 0x2B, 0xD6, 0x1F, 0xE4, 0xD2, 0x52, 0xCE, 0x9B, 0x4F, 0x7A, 0x37, 0xBB, 0xE8, 0x28, 0xE4,
	0xB7, 0xCD, 0x0C, 0xEE, 0x33, 0x94, 0xF6, 0x34, 0xF2, 0x0C, 0x74, 0x9F, 0xD7, 0x22, 0xAE, 0xA5,
	0xB9, 0x9F, 0x43, 0x91, 0x1E, 0x1D, 0x38, 0x68, 0x6B, 0x67, 0x40, 0x3E, 0x99, 0xD6, 0x89, 0xE7,
	0x16, 0xE4, 0xBE, 0xEE, 0x35, 0xC9, 0x53, 0x8C, 0x1E, 0x79, 0x7A, 0xAF, 0xBD, 0x5F, 0x21, 0x5C,
	0x60, 0x72, 0xF8, 0xF9, 0x9C, 0x8C, 0xFE, 0xF6, 0x5E, 0x4C, 0x0E, 0xF2, 0x54, 0xE9, 0x04, 0x65,
	0xF8, 0xDF, 0xE3, 0x15, 0xBE, 0x10, 0xF1, 0x6E, 0x6E, 0x71, 0x6D, 0x7C, 0x3F, 0x1E, 0x55, 0x91,
	0xD7, 0x9B, 0xE5, 0xD0, 0x26, 0x5D, 0x18, 0x3D, 0xBE, 0x45, 0x5E, 0xFB, 0x2B, 0xF2, 0x72, 0x99,
	0x33, 0x02, 0x23, 0xA0, 0xF9, 0x3C, 0x4E, 0xAD, 0xB8, 0xA4, 0x21, 0x1F, 0xE4, 0x7E, 0x26, 0xB3,
	0x8F, 0xFE, 0x4C, 0xF2, 0x53, 0xE3, 0x10, 0xD7, 0x47, 0x90, 0xB7, 0x22, 0x9E, 0xC5, 0x4B, 0xC8,
	0x37, 0x07, 0x3C, 0x5D, 0x9C, 0x87, 0xB2, 0xFD, 0xEA, 0x05, 0x1D, 0xEB, 0xFE, 0x13, 0x51, 0x62,
	0x16, 0x56, 0x8E, 0xA6, 0x2A, 0x44, 0xCD, 0x61, 0x61, 0x4E, 0xE7, 0x35, 0x59, 0x39, 0x45, 0x59,
	0x39, 0xFF, 0xCA, 0x74, 0x50, 0x35, 0x02, 0x0A, 0x99, 0xB2, 0x21, 0xEA, 0xF3, 0x7E, 0x72, 0xC0,
	0x96, 0xC9, 0x91, 0xFC, 0x08, 0x28, 0xDB, 0x70, 0x68, 0x75, 0x91, 0xA9, 0xDD, 0xF2, 0x56, 0x9C,
	0xCB, 0x12, 0x28, 0x31, 0x27, 0x19, 0x6A, 0x6C, 0x78, 0x87, 0x1B, 0xB9, 0x9C, 0x4E, 0xD9, 0xF4,
	0x9D, 0xEF, 0xD4, 0x85, 0xBB, 0xD6, 0x4F, 0x1E, 0x4F, 0xDE, 0xDE, 0xBE, 0x26, 0x79, 0x38, 0x75,
	0xED, 0xF9, 0x75, 0x5B, 0x63, 0x53, 0x3D, 0xD7, 0x42, 0x3E, 0x3F, 0x5A, 0xB7, 0x6E, 0x75, 0x17,
	0x9E, 0xBB, 0xB7, 0x6B, 0xCD, 0xEF, 0x7E, 0x64, 0x5C, 0x79, 0xAA, 0xB1, 0xA1, 0x77, 0x6A, 0x0C,
	0x1E, 0x99, 0xDA, 0xA0, 0xA4, 0x3C, 0x1D, 0x79, 0x31, 0xE7, 0x23, 0x46, 0x9F, 0xB3, 0x54, 0xCA,
	0xFE, 0x92, 0x16, 0xA5, 0xEA, 0x71, 0xEA, 0x66, 0xF5, 0x5F, 0xB1, 0x72, 0x0F, 0x96, 0xA8, 0xDF,
	0x00, 0x37, 0x85, 0x47, 0xF8, 0xA0, 0xCD, 0xD9, 0x86, 0x17, 0xB4, 0x72, 0xF3, 0x23, 0x5D, 0x9F,
	0x06, 0x55, 0x3E, 0x29, 0x44, 0xA3, 0x3F, 0xE0, 0x64, 0xD0, 0xDD, 0x28, 0x63, 0x5D, 0x9C, 0xFF,
	0xFF, 0x37, 0x26, 0x68, 0x8D, 0x70, 0xE5, 0xBF, 0xB4, 0xA7, 0x7F, 0xE3, 0xBB, 0xEF, 0x88, 0x6E,
	0x47, 0xA4, 0x97, 0x3E, 0x3F, 0xCB, 0x9A, 0x3F, 0xAC, 0x11, 0x96, 0x7C, 0x4D, 0xD1, 0x60, 0xCB,
	0x5C, 0x26, 0x43, 0x8F, 0x36, 0xD0, 0x99, 0x25, 0x16, 0xFB, 0xE6, 0xAC, 0x34, 0xCE, 0x28, 0xAF,
	0x01, 0xCB, 0x35, 0xB4, 0x1C, 0x65, 0xC1, 0x75, 0xDC, 0x61, 0x86, 0xC1, 0x13, 0x4E, 0xC2, 0xC8,
	0xCA, 0xB1, 0x74, 0x23, 0x0B, 0xA1, 0xD1, 0x50, 0x6D, 0xAD, 0x11, 0x4E, 0x8E, 0xD6, 0x37, 0xD6,
	0x37, 0x65, 0x6B, 0xB3, 0xB5, 0xA3, 0x2B, 0xC2, 0xE1, 0x8D, 0xE1, 0xFA, 0x04, 0xD0, 0xB9, 0x9B,
	0x7E, 0xAE, 0xB4, 0x70, 0x28, 0xE4, 0x45, 0x0D, 0xA8, 0xEE, 0xE0, 0x1E, 0x15, 0x3F, 0x03, 0xFD,
	0x93, 0xDC, 0xCA, 0x12, 0x52, 0xD2, 0x2D, 0x42, 0x34, 0x3C, 0x10, 0x94, 0x68, 0xA0, 0x11, 0x0D,
	0x8F, 0x70, 0xB3, 0xD3, 0xF4, 0x81, 0x5E, 0xFF, 0xA0, 0x83, 0x90, 0x3E, 0xA2, 0x29, 0x15, 0xBE,
	0x19, 0xBF, 0x69, 0x4B, 0xA2, 0x39, 0x57, 0x6A, 0x4E, 0x2D, 0xC6, 0x0B, 0x77, 0xB0, 0x4D, 0xC5,
	0xD7, 0xB2, 0xFF, 0x62, 0xE6, 0x84, 0x49, 0x5E, 0xE7, 0x84, 0xC8, 0xA9, 0x01, 0xF2, 0xA1, 0x83,
	0x9E, 0xAA, 0x4A, 0x5E, 0xD5, 0x4F, 0xF6, 0xC6, 0x48, 0x8C, 0x10, 0x40, 0xDE, 0xD2, 0x78, 0x85,
	0xD7, 0xF1, 0x29, 0x5D, 0x59, 0xC8, 0x7A, 0x35, 0x7A, 0x98, 0xC7, 0x27, 0x54, 0x9F, 0xDF, 0x28,
	0x2F, 0x66, 0xC8, 0x7B, 0xE3, 0x3E, 0xEF, 0x08, 0x48, 0xB0, 0xDE, 0x16, 0x53, 0x8B, 0x23, 0x1F,
	0x47, 0xBE, 0x5F, 0x3F, 0x75, 0x41, 0xF2, 0x3A, 0x10, 0xAD, 0xCF, 0x32, 0xF1, 0x79, 0x1F, 0xF0,
	0xB0, 0x0C, 0x6F, 0x65, 0x5F, 0xEE, 0x47, 0x3E, 0x5B, 0xE1, 0x3D, 0x6E, 0x75, 0x9F, 0x65, 0x16,
	0x71, 0x80, 0xE2, 0x87, 0x7F, 0x8B, 0xD9, 0x1D, 0x27, 0xDD, 0xDD, 0x92, 0x4F, 0xE9, 0x27, 0x25,
	0x9F, 0xA4, 0x40, 0xAC, 0x3E, 0xA7, 0xCA, 0x67, 0xCF, 0x15, 0x46, 0x8D, 0xE4, 0xE8, 0xD9, 0x54,
	0xE1, 0x12, 0xAC, 0x58, 0x6F, 0x3C, 0x7B, 0xB7, 0x7E, 0x34, 0xB9, 0xA9, 0x7E, 0xC3, 0xFB, 0xE1,
	0x44, 0x95, 0x2F, 0xF5, 0x7D, 0x80, 0xBC, 0xF9, 0x9E, 0x48, 0x60, 0x36, 0x18, 0x77, 0x2C, 0xAD,
	0x97, 0x39, 0x43, 0x7E, 0x5C, 0xAC, 0xC5, 0x0A, 0xA2, 0x4F, 0xCA, 0xE8, 0x07, 0x06, 0x63, 0xC9,
	0x98, 0xEA, 0xF8, 0x3C, 0x40, 0xC0, 0x6B, 0xA7, 0x6D, 0x4C, 0x97, 0xD6, 0x0B, 0x5C, 0x85, 0x4C,
	0x2F, 0x39, 0x7D, 0xEB, 0x73, 0x4C, 0x9E, 0x16, 0xB1, 0x88, 0xC6, 0xB0, 0x5C, 0x24, 0x3F, 0x75,
	0x77, 0x4F, 0x35, 0x39, 0xC8, 0x33, 0xAE, 0xE1, 0x6B, 0xC5, 0xFA, 0x0C, 0xAB, 0x15, 0x03, 0x68,
	0x31, 0x88, 0xE4, 0x0F, 0xF9, 0xB9, 0xEF, 0x1B, 0x8C, 0xE9, 0x31, 0x95, 0x03, 0x31, 0x4F, 0xE2,
	0xB3, 0x01, 0x93, 0x77, 0x94, 0x88, 0x2E, 0x79, 0x02, 0xBC, 0x1F, 0x32, 0x04, 0x1B, 0x95, 0xC9,
	0x1B, 0x74, 0x2C, 0xE6, 0x47, 0x2F, 0x44, 0x62, 0xEA, 0xC6, 0x81, 0xEA, 0xD4, 0x22, 0x0F, 0xDE,
	0x7B, 0xC8, 0x13, 0x4F, 0xE6, 0x90, 0xB1, 0x16, 0x0D, 0x79, 0x5D, 0x57, 0x1E, 0xF2, 0x9A, 0x87,
	0xBC, 0x69, 0x55, 0xF9, 0xBF, 0x23, 0x9F, 0x80, 0x2C, 0xF2, 0xA9, 0x0A, 0x6F, 0xE0, 0xF9, 0x0B,
	0x60, 0x41, 0x85, 0xB7, 0x06, 0x6F, 0xBC, 0x21, 0x2A, 0x99, 0xA4, 0x84, 0x51, 0xC2, 0x49, 0xC0,
	0x63, 0xE5, 0x88, 0xB3, 0xF2, 0x0E, 0xA2, 0x07, 0x85, 0xD9, 0x37, 0x14, 0xD3, 0x7F, 0x8E, 0x7F,
	0x89, 0x83, 0x3C, 0x82, 0x7E, 0x72, 0x90, 0xD7, 0x41, 0x27, 0x7E, 0x72, 0x7C, 0x5E, 0x0E, 0xDF,
	0xE7, 0x38, 0x50, 0xCD, 0xFD, 0xE0, 0x60, 0x44, 0xBE, 0x4F, 0x70, 0x81, 0xA7, 0xA9, 0x85, 0x85,
	0xC9, 0x35, 0x1B, 0xD3, 0xEB, 0xC8, 0xBA, 0x77, 0xB0, 0xE4, 0x35, 0xAD, 0xC2, 0x93, 0x3E, 0x0D,
	0x6B, 0x5F, 0x8E, 0x0D, 0x26, 0xA9, 0xF2, 0x03, 0xA7, 0xED, 0x21, 0x02, 0x06, 0x1E, 0x69, 0x92,
	0x3F, 0x7D, 0x8B, 0x12, 0x77, 0x08, 0x2C, 0xCB, 0xB1, 0xAB, 0xBC, 0xA1, 0xBD, 0x2F, 0xE4, 0xA6,
	0x40, 0x45, 0x5A, 0x77, 0x46, 0x13, 0xC6, 0x97, 0xD9, 0x02, 0x18, 0x38, 0x3C, 0xAE, 0x5A, 0xC7,
	0x08, 0x87, 0xB2, 0xA1, 0xDA, 0xC6, 0x6C, 0x38, 0xC9, 0xEA, 0x57, 0x6C, 0x5C, 0x9B, 0x6D, 0xCC,
	0x36, 0x24, 0xB3, 0x06, 0x98, 0xE1, 0x6C, 0x6D, 0xC0, 0x8F, 0x9D, 0x2B, 0x5C, 0x35, 0x92, 0x24,
	0x9C, 0xE4, 0xF5, 0xC9, 0x35, 0x61, 0xE3, 0xDC, 0xDD, 0x50, 0xB2, 0x74, 0x15, 0xAC, 0x0F, 0x20,
	0x51, 0x49, 0x8E, 0xFF, 0x69, 0xEC, 0xE9, 0x92, 0x9F, 0xB7, 0xCD, 0x2F, 0x58, 0xE3, 0xD1, 0xE0,
	0x67, 0xFA, 0x41, 0xC7, 0x1D, 0xEF, 0xE1, 0x32, 0xC5, 0x45, 0xA8, 0xF9, 0x7F, 0xF4, 0xEA, 0xAE,
	0x05, 0x72, 0x4B, 0x7B, 0xC0, 0x07, 0x0B, 0x64, 0x5E, 0x5B, 0xC0, 0xD7, 0x89, 0x85, 0x0D, 0xA5,
	0xEA, 0x7E, 0x6E, 0x08, 0xD1, 0xED, 0xFF, 0x69, 0xBC, 0x5F, 0xE9, 0x49, 0xFB, 0xEE, 0xFF, 0x00,
	0xAA, 0xCF, 0x5C, 0xA0, 0xB7, 0x28, 0x68, 0xF7, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44,
	0xAE, 0x42, 0x60, 0x82
};

/*
新宋体ascii 94个符号位图数据
// 12号像素是6*14宽高
char r[2] = { 0x21, 0x7e };
12：6*14=564
14：7*16=658
16：8*18=752
*/
void nsimsun_ascii(bitmap_ttinfo* obt)
{
	std::string str = R"(!"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)";
	uint32_t* img = obt->_buf;
	if (!img)
	{
		stbimage_load lad;
		if (lad.load_mem((char*)fdata, 2180))
		{
			img = new uint32_t[lad.width * lad.height];
			obt->_buf = img;
			memcpy(img, lad.data, sizeof(uint32_t) * lad.width * lad.height);
			obt->_bimg.data = img;
			obt->_bimg.width = lad.width;
			obt->_bimg.height = lad.height;
			obt->_bimg.type = 0;
			obt->_bimg.valid = 1;
		}
	}

	if (!img)
	{
		obt->_chsize.clear();
		obt->_unicode_rang.clear();
		return;
	}
	// 支持的大小
	obt->_chsize = { {12, 6}, {14, 7}, {16, 8} };
	// 范围
	obt->_unicode_rang = { {0x21, 0x7e}, {0x21, 0x7e}, {0x21, 0x7e} };
}


int font_t::init_sbit()
{
	auto font_i = font;
	const stbtt_fontinfo* font = font_i;
	int i, count, stringOffset;
	uint8_t* fc = font->data;
	uint32_t offset = font->fontstart, table_size = 0, sbit_num_strikes = 0;
	uint32_t ebdt_start = 0, ebdt_size = 0;
	sfnt_header* ebdt_table = 0;
	auto sbit_table = get_tag(font_i, TAG_CBLC);
	sbit_table_type stt = sbit_table_type::TYPE_NONE;
	do
	{
		if (sbit_table)
		{
			stt = sbit_table_type::TYPE_CBLC;
			break;
		}
		sbit_table = get_tag(font_i, TAG_EBLC);
		if (sbit_table)
		{
			stt = sbit_table_type::TYPE_EBLC;
			break;
		}
		sbit_table = get_tag(font_i, TAG_bloc);
		if (sbit_table)
		{
			stt = sbit_table_type::TYPE_EBLC;
			break;
		}
		sbit_table = get_tag(font_i, TAG_sbix);
		if (sbit_table)
		{
			stt = sbit_table_type::TYPE_SBIX;
			break;
		}
		else
		{
			// error
			return 0;
		}
	} while (0);
	table_size = sbit_table->logicalLength;
	switch (stt)
	{
	case sbit_table_type::TYPE_EBLC:
	case sbit_table_type::TYPE_CBLC:
	{
		uint8_t* p = fc + sbit_table->offset;
		uint32_t   count;
		int majorVersion = stb_font::ttUSHORT(p + 0);
		int minorVersion = stb_font::ttUSHORT(p + 2);
		uint32_t num_strikes = stb_font::ttULONG(p + 4);
		if (num_strikes >= 0x10000UL)
		{
			return 0;
		}

		/*
		*  Count the number of strikes available in the table.  We are a bit
		*  paranoid there and don't trust the data.
		*/
		count = num_strikes;
		if (8 + 48UL * count > table_size)
			count = (uint32_t)((table_size - 8) / 48);
		sbit_num_strikes = count;
	}
	break;
	case sbit_table_type::TYPE_SBIX:
	{
		//TODO		解析SBIX
	}
	break;
	default:
		break;
	}
	do
	{
		ebdt_table = get_tag(font_i, TAG_CBDT);
		if (ebdt_table) break;
		ebdt_table = get_tag(font_i, TAG_EBDT);
		if (ebdt_table) break;
		ebdt_table = get_tag(font_i, TAG_bdat);
		if (!ebdt_table) return 0;
	} while (0);

	auto ebsc_table = get_tag(font_i, TAG_EBSC);
	font_t* ttp = (font_t*)font->userdata;
	uint8_t* b = fc + sbit_table->offset;
	count = stb_font::ttULONG(b + 4);
	if (!count)
	{
		return 0;
	}
	bitinfo = new bitmap_ttinfo();
	// 位图表eblc
	//std::vector<BitmapSizeTable> bsts;
	//std::vector<std::vector<IndexSubTableArray>> index_sub_table;
	njson ns = get_bitmap_size_table(b, count, bitinfo->_bsts, bitinfo->_index_sub_table, bitinfo->_msidx_table);
#if 0
	//ndef nnDEBUG
	printf("<%s>包含位图大小\n", _aname.c_str());
	for (auto& it : ns)
		printf("%s\n", it.dump().c_str());
#endif // !nnDEBUG
	// 位图数据表ebdt
	b = fc + ebdt_table->offset;
	glm::ivec2 version = { stb_font::ttUSHORT(b + 0), stb_font::ttUSHORT(b + 2) };
	bitinfo->_sbit_table = sbit_table;
	bitinfo->_ebdt_table = ebdt_table;
	bitinfo->_ebsc_table = ebsc_table;
	if (ns.size()) {
		first_bitmap = true;
	}
	return ns.size();
}


#endif // !_FONT_NO_BITMAP



#ifndef NO_COLOR_FONT

/**************************************************************************
 *
 * `COLR' table specification:
 *
 *   https://www.microsoft.com/typography/otspec/colr.htm
 *
 */



 /* NOTE: These are the table sizes calculated through the specs. */
#define BASE_GLYPH_SIZE0            6
#define LAYER_SIZE0                 4
#define COLR_HEADER_SIZE          14
/* 3 * uint16 + 2 * Offset32 */
#define COLRV0_HEADER_SIZE               14U
/* COLRV0_HEADER_SIZE + 5 * Offset32 */
#define COLRV1_HEADER_SIZE               34U

#define BASE_GLYPH_SIZE                   6U
#define BASE_GLYPH_PAINT_RECORD_SIZE      6U
#define LAYER_V1_LIST_PAINT_OFFSET_SIZE   4U
#define LAYER_V1_LIST_NUM_LAYERS_SIZE     4U
#define COLOR_STOP_SIZE                   6U
#define VAR_IDX_BASE_SIZE                 4U
#define LAYER_SIZE                        4U


struct BaseGlyphRecord
{
	uint16_t  gid;
	uint16_t  first_layer_index;
	uint16_t  num_layers;

};


struct Colr
{
	uint16_t  version;
	uint16_t  num_base_glyphs;
	uint16_t  num_layers;

	uint8_t* base_glyphs;
	uint8_t* layers;

	uint32_t  num_base_glyphs_v1;
	/* Points at beginning of BaseGlyphV1List. */
	uint8_t* base_glyphs_v1;

	uint32_t  num_layers_v1;
	uint8_t* layers_v1;

	uint8_t* clip_list;

	/*
	 * Paint tables start at the minimum of the end of the LayerList and the
	 * end of the BaseGlyphList.  Record this location in a field here for
	 * safety checks when accessing paint tables.
	 */
	uint8_t* paints_start_v1;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
	/* Item Variation Store for variable 'COLR' v1. */
	GX_ItemVarStoreRec    var_store;
	GX_DeltaSetIdxMapRec  delta_set_idx_map;
#endif

	/* The memory which backs up the `COLR' table. */
	void* table;
	unsigned long  table_size;

};
struct colr_v0 {
	uint16_t	version;//	Table version number—set to 0.
	uint16_t	numBaseGlyphRecords;//	Number of BaseGlyph records.
	Offset32	baseGlyphRecordsOffset;//	Offset to baseGlyphRecords array, from beginning of COLR table.
	Offset32	layerRecordsOffset;//	Offset to layerRecords array, from beginning of COLR table.
	uint16_t	numLayerRecords;//	Number of Layer records.
};
struct colr_v1 {
	uint16_t	version;//	Table version number—set to 1.
	uint16_t	numBaseGlyphRecords;//	Number of BaseGlyph records; may be 0 in a version 1 table.
	Offset32	baseGlyphRecordsOffset;//	Offset to baseGlyphRecords array, from beginning of COLR table(may be NULL).
	Offset32	layerRecordsOffset;//	Offset to layerRecords array, from beginning of COLR table(may be NULL).
	uint16_t	numLayerRecords;//	Number of Layer records; may be 0 in a version 1 table.
	Offset32	baseGlyphListOffset;//	Offset to BaseGlyphList table, from beginning of COLR table.
	Offset32	layerListOffset;//	Offset to LayerList table, from beginning of COLR table(may be NULL).
	Offset32	clipListOffset;//	Offset to ClipList table, from beginning of COLR table(may be NULL).
	Offset32	varIndexMapOffset;//	Offset to DeltaSetIndexMap table, from beginning of COLR table(may be NULL).
	Offset32	itemVariationStoreOffset;//	Offset to ItemVariationStore, from beginning of COLR table(may be NULL).
};

int	tt_face_load_colr(font_t* face, uint8_t* b, sfnt_header* sp)
{
	int   error = 0;
	//FT_Memory  memory = face->root.memory;

	uint8_t* table = b;
	uint8_t* p = NULL;

	Colr* colr = NULL;

	//uint32_t  base_glyph_offset, layer_offset;
	uint32_t  table_size = sp->logicalLength;
	auto cp = face->colorinfo;

	uint8_t* p1 = NULL;
	uint32_t  base_glyph_offset, layer_offset;
	uint32_t  base_glyphs_offset_v1, num_base_glyphs_v1;
	uint32_t  layer_offset_v1, num_layers_v1, clip_list_offset;
#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
	uint32_t  colr_offset_in_stream;
#endif
	/* `COLR' always needs `CPAL' */
	//if (!face->cpal)
	//	return FT_THROW(Invalid_File_Format);

	//error = face->goto_table(face, TTAG_COLR, stream, &table_size);
	//if (error)
	//	goto NoColr;
	do {

		if (table_size < COLR_HEADER_SIZE)break;
		//	goto InvalidTable;

		//if (FT_FRAME_EXTRACT(table_size, table))
		//	goto NoColr;

		p = table;
		colr = (Colr*)cp->td;

		if (!(colr))
			break;
		auto cv0 = (colr_v0*)p;
		auto cv1 = (colr_v1*)p;
		colr->version = NEXT_USHORT(p);
		if (colr->version > 1)
		{
			error = -1; break;
		}

		colr->num_base_glyphs = NEXT_USHORT(p);
		base_glyph_offset = NEXT_ULONG(p);

		if (base_glyph_offset >= table_size)
		{
			error = -1; break;
		}
		if (colr->num_base_glyphs * BASE_GLYPH_SIZE >
			table_size - base_glyph_offset)
		{
			error = -1; break;
		}

		layer_offset = NEXT_ULONG(p);
		colr->num_layers = NEXT_USHORT(p);

		if (layer_offset >= table_size)
		{
			error = -1; break;
		}
		if (colr->num_layers * LAYER_SIZE > table_size - layer_offset)
		{
			error = -1; break;
		}
		if (colr->version == 1) {
			if (table_size < COLRV1_HEADER_SIZE)
			{
				error = -21; break;
			}

			base_glyphs_offset_v1 = NEXT_ULONG(p);

			if (base_glyphs_offset_v1 + 4 >= table_size)
				break;

			p1 = (uint8_t*)(table + base_glyphs_offset_v1);
			num_base_glyphs_v1 = PEEK_ULONG(p1);

			if (num_base_glyphs_v1 * BASE_GLYPH_PAINT_RECORD_SIZE >
				table_size - base_glyphs_offset_v1)
				break;

			colr->num_base_glyphs_v1 = num_base_glyphs_v1;
			colr->base_glyphs_v1 = p1;

			layer_offset_v1 = NEXT_ULONG(p);

			if (layer_offset_v1 >= table_size)
				break;

			if (layer_offset_v1)
			{
				if (layer_offset_v1 + 4 >= table_size)
					break;

				p1 = (uint8_t*)(table + layer_offset_v1);
				num_layers_v1 = PEEK_ULONG(p1);

				if (num_layers_v1 * LAYER_V1_LIST_PAINT_OFFSET_SIZE >
					table_size - layer_offset_v1)
					break;

				colr->num_layers_v1 = num_layers_v1;
				colr->layers_v1 = p1;

				colr->paints_start_v1 =
					std::min(colr->base_glyphs_v1 +
						colr->num_base_glyphs_v1 * BASE_GLYPH_PAINT_RECORD_SIZE,
						colr->layers_v1 +
						colr->num_layers_v1 * LAYER_V1_LIST_PAINT_OFFSET_SIZE);
			}
			else
			{
				colr->num_layers_v1 = 0;
				colr->layers_v1 = 0;
				colr->paints_start_v1 =
					colr->base_glyphs_v1 +
					colr->num_base_glyphs_v1 * BASE_GLYPH_PAINT_RECORD_SIZE;
			}

			clip_list_offset = NEXT_ULONG(p);

			if (clip_list_offset >= table_size)
				break;

			if (clip_list_offset)
				colr->clip_list = (uint8_t*)(table + clip_list_offset);
			else
				colr->clip_list = 0;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
			colr->var_store.dataCount = 0;
			colr->var_store.varData = NULL;
			colr->var_store.axisCount = 0;
			colr->var_store.regionCount = 0;
			colr->var_store.varRegionList = 0;

			colr->delta_set_idx_map.mapCount = 0;
			colr->delta_set_idx_map.outerIndex = NULL;
			colr->delta_set_idx_map.innerIndex = NULL;

			if (face->variation_support & TT_FACE_FLAG_VAR_FVAR)
			{
				uint32_t  var_idx_map_offset, var_store_offset;

				FT_Service_MultiMasters  mm = (FT_Service_MultiMasters)face->mm;


				var_idx_map_offset = FT_NEXT_ULONG(p);

				if (var_idx_map_offset >= table_size)
					goto InvalidTable;

				var_store_offset = FT_NEXT_ULONG(p);
				if (var_store_offset >= table_size)
					goto InvalidTable;

				if (var_store_offset)
				{
					/* If variation info has not been initialized yet, try doing so, */
					/* otherwise loading the variation store will fail as it         */
					/* requires access to `blend` for checking the number of axes.   */
					if (!face->blend)
						if (mm->get_mm_var(FT_FACE(face), NULL))
							goto InvalidTable;

					/* Try loading `VarIdxMap` and `VarStore`. */
					error = mm->load_item_var_store(
						FT_FACE(face),
						colr_offset_in_stream + var_store_offset,
						&colr->var_store);
					if (error != FT_Err_Ok)
						goto InvalidTable;
				}

				if (colr->var_store.axisCount && var_idx_map_offset)
				{
					error = mm->load_delta_set_idx_map(
						FT_FACE(face),
						colr_offset_in_stream + var_idx_map_offset,
						&colr->delta_set_idx_map,
						&colr->var_store,
						table_size);
					if (error != FT_Err_Ok)
						goto InvalidTable;
				}
			}
#endif /* TT_CONFIG_OPTION_GX_VAR_SUPPORT */
		}

		colr->base_glyphs = (uint8_t*)(table + base_glyph_offset);
		colr->layers = (uint8_t*)(table + layer_offset);
		colr->table = table;
		colr->table_size = table_size;

		cp->colr = colr;


	} while (0);

	return error;
}


void tt_face_free_colr(font_t* p)
{
	//sfnt_header* sp = face->root.stream;
	////FT_Memory  memory = face->root.memory;

	//Colr* colr = (Colr*)face->colr;


	//if (colr)
	//{
	//	FT_FRAME_RELEASE(colr->table);
	//	FT_FREE(colr);
	//}
}


static bool find_base_glyph_record(uint8_t* base_glyph_begin,
	int            num_base_glyph,
	uint32_t           glyph_id,
	BaseGlyphRecord* record)
{
	int  min = 0;
	int  max = num_base_glyph - 1;


	while (min <= max)
	{
		int    mid = min + (max - min) / 2;
		uint8_t* p = base_glyph_begin + mid * BASE_GLYPH_SIZE;

		uint16_t  gid = NEXT_USHORT(p);


		if (gid < glyph_id)
			min = mid + 1;
		else if (gid > glyph_id)
			max = mid - 1;
		else
		{
			record->gid = gid;
			record->first_layer_index = NEXT_USHORT(p);
			record->num_layers = NEXT_USHORT(p);

			return 1;
		}
	}

	return 0;
}


bool tt_face_get_colr_layer(font_t* face,
	uint32_t            base_glyph,
	uint32_t* aglyph_index,
	uint32_t* acolor_index,
	LayerIterator* iterator)
{
	Colr* colr = (Colr*)face->colorinfo->colr;
	BaseGlyphRecord  glyph_record = {};
	auto cp = face->colorinfo;

	if (!colr)
		return 0;

	if (!iterator->p)
	{
		uint32_t  offset;


		/* first call to function */
		iterator->layer = 0;

		if (!find_base_glyph_record(colr->base_glyphs,
			colr->num_base_glyphs,
			base_glyph,
			&glyph_record))
			return 0;

		if (glyph_record.num_layers)
			iterator->num_layers = glyph_record.num_layers;
		else
			return 0;

		offset = LAYER_SIZE * glyph_record.first_layer_index;
		if (offset + LAYER_SIZE * glyph_record.num_layers > colr->table_size)
			return 0;

		iterator->p = colr->layers + offset;
	}

	if (iterator->layer >= iterator->num_layers)
		return 0;

	*aglyph_index = NEXT_USHORT(iterator->p);
	*acolor_index = NEXT_USHORT(iterator->p);

	if (*aglyph_index >= (uint32_t)(face->num_glyphs) ||
		(*acolor_index != 0xFFFF &&
			*acolor_index >= cp->palette_data.num_palette_entries))
		return 0;

	iterator->layer++;

	return 1;
}

#define PALETTE_FOR_LIGHT_BACKGROUND  0x01
#define PALETTE_FOR_DARK_BACKGROUND   0x02
Color_2 get_c2(font_t* face1, uint32_t color_index)
{
	Color_2 c = {};
	auto face = face1->colorinfo;
	if (color_index == 0xFFFF)
	{
		if (face->have_foreground_color)
		{
			c.b = face->foreground_color.blue;
			c.g = face->foreground_color.green;
			c.r = face->foreground_color.red;
			c.alpha = face->foreground_color.alpha;
		}
		else
		{
			if (face->palette_data.palette_flags &&
				(face->palette_data.palette_flags[face->palette_index] &
					PALETTE_FOR_DARK_BACKGROUND))
			{
				/* white opaque */
				c.b = 0xFF;
				c.g = 0xFF;
				c.r = 0xFF;
				c.alpha = 0xFF;
			}
			else
			{
				/* black opaque */
				c.b = 0x00;
				c.g = 0x00;
				c.r = 0x00;
				c.alpha = 0xFF;
			}
		}
	}
	else
	{
		c = face->palette[color_index];
	}
	return c;
}
int tt_face_colr_blend_layer(font_t* face1,
	uint32_t       color_index,
	GlyphSlot* dstSlot,
	GlyphSlot* srcSlot)
{
	int  error = 0;
	auto face = face1->colorinfo;
	uint32_t  x, y;
	uint8_t  b, g, r, alpha;

	uint32_t  size;
	uint8_t* src;
	uint8_t* dst;
#if 0

	if (!dstSlot->bitmap.buffer)
	{
		/* Initialize destination of color bitmap */
		/* with the size of first component.      */
		dstSlot->bitmap_left = srcSlot->bitmap_left;
		dstSlot->bitmap_top = srcSlot->bitmap_top;

		dstSlot->bitmap.width = srcSlot->bitmap.width;
		dstSlot->bitmap.rows = srcSlot->bitmap.rows;
		dstSlot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;
		dstSlot->bitmap.pitch = (int)dstSlot->bitmap.width * 4;
		dstSlot->bitmap.num_grays = 256;

		size = dstSlot->bitmap.rows * (uint32_t)dstSlot->bitmap.pitch;

		error = ft_glyphslot_alloc_bitmap(dstSlot, size);
		if (error)
			return error;

		FT_MEM_ZERO(dstSlot->bitmap.buffer, size);
	}
	else
	{
		/* Resize destination if needed such that new component fits. */
		int  x_min, x_max, y_min, y_max;


		x_min = FT_MIN(dstSlot->bitmap_left, srcSlot->bitmap_left);
		x_max = FT_MAX(dstSlot->bitmap_left + (int)dstSlot->bitmap.width,
			srcSlot->bitmap_left + (int)srcSlot->bitmap.width);

		y_min = FT_MIN(dstSlot->bitmap_top - (int)dstSlot->bitmap.rows,
			srcSlot->bitmap_top - (int)srcSlot->bitmap.rows);
		y_max = FT_MAX(dstSlot->bitmap_top, srcSlot->bitmap_top);

		if (x_min != dstSlot->bitmap_left ||
			x_max != dstSlot->bitmap_left + (int)dstSlot->bitmap.width ||
			y_min != dstSlot->bitmap_top - (int)dstSlot->bitmap.rows ||
			y_max != dstSlot->bitmap_top)
		{
			FT_Memory  memory = face->root.memory;

			uint32_t  width = (uint32_t)(x_max - x_min);
			uint32_t  rows = (uint32_t)(y_max - y_min);
			uint32_t  pitch = width * 4;

			uint8_t* buf = NULL;
			uint8_t* p;
			uint8_t* q;


			size = rows * pitch;
			if (FT_ALLOC(buf, size))
				return error;

			p = dstSlot->bitmap.buffer;
			q = buf +
				(int)pitch * (y_max - dstSlot->bitmap_top) +
				4 * (dstSlot->bitmap_left - x_min);

			for (y = 0; y < dstSlot->bitmap.rows; y++)
			{
				FT_MEM_COPY(q, p, dstSlot->bitmap.width * 4);

				p += dstSlot->bitmap.pitch;
				q += pitch;
			}

			ft_glyphslot_set_bitmap(dstSlot, buf);

			dstSlot->bitmap_top = y_max;
			dstSlot->bitmap_left = x_min;

			dstSlot->bitmap.width = width;
			dstSlot->bitmap.rows = rows;
			dstSlot->bitmap.pitch = (int)pitch;

			dstSlot->internal->flags |= FT_GLYPH_OWN_BITMAP;
			dstSlot->format = FT_GLYPH_FORMAT_BITMAP;
		}
	}

	if (color_index == 0xFFFF)
	{
		if (face->have_foreground_color)
		{
			b = face->foreground_color.blue;
			g = face->foreground_color.green;
			r = face->foreground_color.red;
			alpha = face->foreground_color.alpha;
		}
		else
		{
			if (face->palette_data.palette_flags &&
				(face->palette_data.palette_flags[face->palette_index] &
					FT_PALETTE_FOR_DARK_BACKGROUND))
			{
				/* white opaque */
				b = 0xFF;
				g = 0xFF;
				r = 0xFF;
				alpha = 0xFF;
			}
			else
			{
				/* black opaque */
				b = 0x00;
				g = 0x00;
				r = 0x00;
				alpha = 0xFF;
			}
		}
	}
	else
	{
		b = face->palette[color_index].blue;
		g = face->palette[color_index].green;
		r = face->palette[color_index].red;
		alpha = face->palette[color_index].alpha;
	}

	/* XXX Convert if srcSlot.bitmap is not grey? */
	src = srcSlot->bitmap.buffer;
	dst = dstSlot->bitmap.buffer +
		dstSlot->bitmap.pitch * (dstSlot->bitmap_top - srcSlot->bitmap_top) +
		4 * (srcSlot->bitmap_left - dstSlot->bitmap_left);

	for (y = 0; y < srcSlot->bitmap.rows; y++)
	{
		for (x = 0; x < srcSlot->bitmap.width; x++)
		{
			int  aa = src[x];
			int  fa = alpha * aa / 255;

			int  fb = b * fa / 255;
			int  fg = g * fa / 255;
			int  fr = r * fa / 255;

			int  ba2 = 255 - fa;

			int  bb = dst[4 * x + 0];
			int  bg = dst[4 * x + 1];
			int  br = dst[4 * x + 2];
			int  ba = dst[4 * x + 3];


			dst[4 * x + 0] = (uint8_t)(bb * ba2 / 255 + fb);
			dst[4 * x + 1] = (uint8_t)(bg * ba2 / 255 + fg);
			dst[4 * x + 2] = (uint8_t)(br * ba2 / 255 + fr);
			dst[4 * x + 3] = (uint8_t)(ba * ba2 / 255 + fa);
		}

		src += srcSlot->bitmap.pitch;
		dst += dstSlot->bitmap.pitch;
	}
#endif
	return error;
}



/**************************************************************************
 *
 * `CPAL' table specification:
 *
 *   https://www.microsoft.com/typography/otspec/cpal.htm
 *
 */

 /* NOTE: These are the table sizes calculated through the specs. */
#define CPAL_V0_HEADER_BASE_SIZE  12
#define COLOR_SIZE                 4


  /* all data from `CPAL' not covered in FT_Palette_Data */
struct Cpal
{
	uint16_t  version;        /* Table version number (0 or 1 supported). */
	uint16_t  num_colors;               /* Total number of color records, */
	/* combined for all palettes.     */
	uint8_t* colors;                              /* RGBA array of colors */
	uint8_t* color_indices; /* Index of each palette's first color record */
	/* in the combined color record array.        */

/* The memory which backs up the `CPAL' table. */
	void* table;
	uint32_t  table_size;

};
enum PaintFormat_
{
	COLR_PAINTFORMAT_COLR_LAYERS = 1,
	COLR_PAINTFORMAT_SOLID = 2,
	COLR_PAINTFORMAT_LINEAR_GRADIENT = 4,
	COLR_PAINTFORMAT_RADIAL_GRADIENT = 6,
	COLR_PAINTFORMAT_SWEEP_GRADIENT = 8,
	COLR_PAINTFORMAT_GLYPH = 10,
	COLR_PAINTFORMAT_COLR_GLYPH = 11,
	COLR_PAINTFORMAT_TRANSFORM = 12,
	COLR_PAINTFORMAT_TRANSLATE = 14,
	COLR_PAINTFORMAT_SCALE = 16,
	COLR_PAINTFORMAT_ROTATE = 24,
	COLR_PAINTFORMAT_SKEW = 28,
	COLR_PAINTFORMAT_COMPOSITE = 32,
	COLR_PAINT_FORMAT_MAX = 33,
	COLR_PAINTFORMAT_UNSUPPORTED = 255

};
struct  PaintColrLayers
{
	uint32_t   num_layers;
	uint32_t   layer;
	uint8_t* p;
};

struct PaintScale
{
	uint8_t* p;
	int32_t  scale_x;
	int32_t  scale_y;

	int32_t  center_x;
	int32_t  center_y;
};
struct  PaintRotate
{
	uint8_t* p;
	int32_t  angle;
	int32_t  center_x;
	int32_t  center_y;
};
struct PaintTranslate
{
	uint8_t* p;
	int32_t  dx;
	int32_t  dy;
};
struct  Affine_23
{
	int32_t  xx, xy, dx;
	int32_t  yx, yy, dy;
};
struct  PaintTransform
{
	uint8_t* p;
	Affine_23 affine;
};
struct PaintColrGlyph
{
	uint32_t  glyphID;
};
struct PaintGlyph
{
	uint8_t* p;
	uint32_t glyphID;

};
struct  COLR_Paint_
{
	uint8_t format;

	union
	{
		PaintColrLayers      colr_layers;
		PaintGlyph           glyph;
		//PaintSolid           solid;
		//PaintLinearGradient  linear_gradient;
		//PaintRadialGradient  radial_gradient;
		//PaintSweepGradient   sweep_gradient;
		PaintTransform       transform;
		PaintTranslate       translate;
		PaintScale           scale;
		PaintRotate          rotate;
		//PaintSkew            skew;
		//PaintComposite       composite;
		PaintColrGlyph       colr_glyph;

	} u;

};


int tt_face_load_cpal(font_t* face1, uint8_t* b, sfnt_header* sp)
{
	int   error = 0;
	//Memory  memory = face->root.memory;
	auto face = face1->colorinfo;
	uint8_t* table = b;
	uint8_t* p = NULL;

	Cpal* cpal = NULL;

	uint32_t  colors_offset = 0;
	uint32_t  table_size = sp->logicalLength;

#if 1
	//error = face->goto_table(face, TTAG_CPAL, stream, &table_size);
	//if (error)
	//	goto NoCpal;
	do {

		if (table_size < CPAL_V0_HEADER_BASE_SIZE)
			break;

		//if (FRAME_EXTRACT(table_size, table))
		//	goto NoCpal;

		p = table;
		cpal = (Cpal*)&face->td[sizeof(Colr)];
		if (!cpal)
			break;

		cpal->version = NEXT_USHORT(p);
		if (cpal->version > 1)
		{
			error = -1; break;
		}

		face->palette_data.num_palette_entries = NEXT_USHORT(p);
		face->palette_data.num_palettes = NEXT_USHORT(p);

		cpal->num_colors = NEXT_USHORT(p);
		colors_offset = NEXT_ULONG(p);

		if (CPAL_V0_HEADER_BASE_SIZE +
			face->palette_data.num_palettes * 2U > table_size)
		{
			error = -1; break;
		}

		if (colors_offset >= table_size)
		{
			error = -1; break;
		}
		if (cpal->num_colors * COLOR_SIZE > table_size - colors_offset)
		{
			error = -1; break;
		}

		if (face->palette_data.num_palette_entries > cpal->num_colors)
		{
			error = -1; break;
		}

		cpal->color_indices = p;
		cpal->colors = (uint8_t*)(table + colors_offset);

		if (cpal->version == 1)
		{
			uint32_t    type_offset, label_offset, entry_label_offset;
			uint16_t* array0 = NULL;
			uint16_t* limit;
			uint16_t* q;


			if (CPAL_V0_HEADER_BASE_SIZE +
				face->palette_data.num_palettes * 2U +
				3U * 4 > table_size)
			{
				error = -1; break;
			}

			p += face->palette_data.num_palettes * 2;

			type_offset = NEXT_ULONG(p);
			label_offset = NEXT_ULONG(p);
			entry_label_offset = NEXT_ULONG(p);

			if (type_offset)
			{
				if (type_offset >= table_size)
				{
					error = -1; break;
				}
				if (face->palette_data.num_palettes * 2 >
					table_size - type_offset)
				{
					error = -1; break;
				}
				array0 = new uint16_t[face->palette_data.num_palettes];
				if (!array0)//face1->uac.new_mem(array, face->palette_data.num_palettes))
				{
					error = -2; break;
				}

				p = table + type_offset;
				q = array0;
				limit = q + face->palette_data.num_palettes;

				while (q < limit)
					*q++ = NEXT_USHORT(p);

				face->palette_data.palette_flags = array0;
			}

			if (label_offset)
			{
				if (label_offset >= table_size)
				{
					error = -1; break;
				}
				if (face->palette_data.num_palettes * 2 >
					table_size - label_offset)
				{
					error = -1; break;
				}

				//if (QNEW_ARRAY(array, face->palette_data.num_palettes))

				array0 = new uint16_t[face->palette_data.num_palettes];
				if (!array0)//face1->uac.new_mem(array, face->palette_data.num_palettes))
				{
					error = -2; break;
				}

				p = table + label_offset;
				q = array0;
				limit = q + face->palette_data.num_palettes;

				while (q < limit)
					*q++ = NEXT_USHORT(p);

				face->palette_data.palette_name_ids = array0;
			}

			if (entry_label_offset)
			{
				if (entry_label_offset >= table_size)
				{
					error = -1; break;
				}
				if (face->palette_data.num_palette_entries * 2 >
					table_size - entry_label_offset)
				{
					error = -1; break;
				}

				array0 = new uint16_t[face->palette_data.num_palette_entries];
				if (!array0)//face1->uac.new_mem(array, face->palette_data.num_palette_entries))
				{
					error = -2; break;
				}

				p = table + entry_label_offset;
				q = array0;
				limit = q + face->palette_data.num_palette_entries;

				while (q < limit)
					*q++ = NEXT_USHORT(p);

				face->palette_data.palette_entry_name_ids = array0;
			}
		}

		cpal->table = table;
		cpal->table_size = table_size;

		face->cpal = cpal;

		/* set up default palette */
		face->palette = new Color_2[face->palette_data.num_palette_entries];
		//if (!face1->uac.new_mem(, face->palette_data.num_palette_entries))
		if (!face->palette) {
			error = -2; break;
		}

		if (tt_face_palette_set(face1, 0))
		{
			error = -1; break;
		}
		error = 0;
		break;

	} while (0);
	if (error < 0)
	{
		//InvalidTable:
		//	error = -1;// THROW(Invalid_Table);

		//NoCpal:
			//FRAME_RELEASE(table);
		//face1->uac.free_mem(cpal, 1);

		face->cpal = NULL;

	}
	/* arrays in `face->palette_data' and `face->palette' */
	/* are freed in `sfnt_done_face'                      */
#endif
	return error;
}


void tt_face_free_cpal(font_t* face)
{
	//sfnt_header* sp = face->colorinfo.;
	//Memory  memory = face->root.memory;

	//Cpal* cpal = (Cpal*)face->cpal;


	//if (cpal)
	{
		//	FRAME_RELEASE(cpal->table);
		//	FREE(cpal);
	}
}

int tt_face_palette_set(font_t* face, uint32_t  palette_index)
{
	Cpal* cpal = (Cpal*)face->colorinfo->cpal;
	auto cp = face->colorinfo;
	uint8_t* offset;
	uint8_t* p;

	Color_2* q;
	Color_2* limit;

	uint16_t  color_index;

	if (!cpal || palette_index >= cp->palette_data.num_palettes)
		return -6;// THROW(Invalid_Argument);

	offset = cpal->color_indices + 2 * palette_index;
	color_index = PEEK_USHORT(offset);

	if (color_index + cp->palette_data.num_palette_entries >
		cpal->num_colors)
		return -7;// THROW(Invalid_Table);

	p = cpal->colors + COLOR_SIZE * color_index;
	q = (Color_2*)cp->palette;
	limit = q + cp->palette_data.num_palette_entries;

	while (q < limit)
	{
		q->blue = NEXT_BYTE(p);
		q->green = NEXT_BYTE(p);
		q->red = NEXT_BYTE(p);
		q->alpha = NEXT_BYTE(p);

		q++;
	}

	return 0;
}

#define ENSURE_READ_BYTES( byte_size )                             \
  if ( p < colr->paints_start_v1                                || \
       p > (uint8_t*)colr->table + colr->table_size - byte_size )  \
    return 0

static bool get_child_table_pointer(Colr* colr,
	uint8_t* paint_base,
	uint8_t** p,
	uint8_t** child_table_pointer)
{
	uint32_t  paint_offset;
	uint8_t* child_table_p;


	if (!child_table_pointer)
		return 0;

	if (*p < colr->paints_start_v1 ||
		*p >(uint8_t*)colr->table + colr->table_size - 1 - 3)
		return 0;

	paint_offset = NEXT_UOFF3(*p);
	if (!paint_offset)
		return 0;

	child_table_p = (uint8_t*)(paint_base + paint_offset);

	if (child_table_p < colr->paints_start_v1 ||
		child_table_p >= ((uint8_t*)colr->table + colr->table_size))
		return 0;

	*child_table_pointer = child_table_p;
	return 1;
}
static bool read_paint(font_t* face, Colr* colr, uint8_t* p, COLR_Paint_* apaint)
{
	uint8_t* paint_base = p;
	uint8_t* child_table_p = NULL;
	bool   do_read_var = FALSE;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
	ULong         var_index_base = 0;
	/* Longest varIndexBase offset is 5 in the spec. */
	ItemVarDelta  item_deltas[6] = { 0, 0, 0, 0, 0, 0 };
#else
	//UNUSED(face);
#endif


	if (!p || !colr || !colr->table)
		return 0;

	/* The last byte of the 'COLR' table is at 'size-1'; subtract 1 of    */
	/* that to account for the expected format byte we are going to read. */
	if (p < colr->paints_start_v1 ||
		p >(uint8_t*)colr->table + colr->table_size - 2)
		return 0;

	apaint->format = NEXT_BYTE(p);

	if (apaint->format >= COLR_PAINT_FORMAT_MAX)
		return 0;

	if (apaint->format == COLR_PAINTFORMAT_COLR_LAYERS)
	{
		/* Initialize layer iterator/ */
		uint8_t    num_layers;
		uint32_t  first_layer_index;


		num_layers = NEXT_BYTE(p);
		if (num_layers > colr->num_layers_v1)
			return 0;

		first_layer_index = NEXT_ULONG(p);
		if (first_layer_index + num_layers > colr->num_layers_v1)
			return 0;

		apaint->u.colr_layers.num_layers = num_layers;
		apaint->u.colr_layers.layer = 0;
		/* TODO: Check whether pointer is outside colr? */
		apaint->u.colr_layers.p =
			colr->layers_v1 +
			LAYER_V1_LIST_NUM_LAYERS_SIZE +
			LAYER_V1_LIST_PAINT_OFFSET_SIZE * first_layer_index;

		return 1;
	}
#if 0
	else if (apaint->format == COLR_PAINTFORMAT_SOLID)//|| apaint->format == COLR_PAINTFORMAT_INTERNAL_VAR_SOLID)
	{
		ENSURE_READ_BYTES(4);
		apaint->u.solid.color.palette_index = NEXT_USHORT(p);
		apaint->u.solid.color.alpha = NEXT_SHORT(p);

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
		if ((PaintFormat_Internal)apaint->format ==
			COLR_PAINTFORMAT_INTERNAL_VAR_SOLID)
		{
			ENSURE_READ_BYTES(4);
			var_index_base = NEXT_ULONG(p);

			if (!get_deltas_for_var_index_base(face, colr, var_index_base, 1,
				item_deltas))
				return 0;

			apaint->u.solid.color.alpha += (F2Dot14)item_deltas[0];
		}
#endif

		apaint->format = COLR_PAINTFORMAT_SOLID;

		return 1;
	}
#endif
	else if (apaint->format == COLR_PAINTFORMAT_COLR_GLYPH)
	{
		ENSURE_READ_BYTES(2);
		apaint->u.colr_glyph.glyphID = NEXT_USHORT(p);

		return 1;
	}

	/*
	 * Grouped below here are all paint formats that have an offset to a
	 * child paint table as the first entry (for example, a color line or a
	 * child paint table).  Retrieve that and determine whether that paint
	 * offset is valid first.
	 */

	if (!get_child_table_pointer(colr, paint_base, &p, &child_table_p))
		return 0;
#if 0
	if (apaint->format == COLR_PAINTFORMAT_LINEAR_GRADIENT ||
		(do_read_var =
			((PaintFormat_Internal)apaint->format ==
				COLR_PAINTFORMAT_INTERNAL_VAR_LINEAR_GRADIENT)))
	{
		if (!read_color_line(colr,
			child_table_p,
			&apaint->u.linear_gradient.colorline,
			do_read_var))
			return 0;

		/*
		 * In order to support variations expose these as Fixed 16.16
		 * values so that we can support fractional values after
		 * interpolation.
		 */
		ENSURE_READ_BYTES(12);
		apaint->u.linear_gradient.p0.x = INT_TO_FIXED(NEXT_SHORT(p));
		apaint->u.linear_gradient.p0.y = INT_TO_FIXED(NEXT_SHORT(p));
		apaint->u.linear_gradient.p1.x = INT_TO_FIXED(NEXT_SHORT(p));
		apaint->u.linear_gradient.p1.y = INT_TO_FIXED(NEXT_SHORT(p));
		apaint->u.linear_gradient.p2.x = INT_TO_FIXED(NEXT_SHORT(p));
		apaint->u.linear_gradient.p2.y = INT_TO_FIXED(NEXT_SHORT(p));

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
		if (do_read_var)
		{
			ENSURE_READ_BYTES(4);
			var_index_base = NEXT_ULONG(p);

			if (!get_deltas_for_var_index_base(face, colr, var_index_base, 6,
				item_deltas))
				return 0;

			apaint->u.linear_gradient.p0.x += INT_TO_FIXED(item_deltas[0]);
			apaint->u.linear_gradient.p0.y += INT_TO_FIXED(item_deltas[1]);
			apaint->u.linear_gradient.p1.x += INT_TO_FIXED(item_deltas[2]);
			apaint->u.linear_gradient.p1.y += INT_TO_FIXED(item_deltas[3]);
			apaint->u.linear_gradient.p2.x += INT_TO_FIXED(item_deltas[4]);
			apaint->u.linear_gradient.p2.y += INT_TO_FIXED(item_deltas[5]);
		}
#endif

		apaint->format = COLR_PAINTFORMAT_LINEAR_GRADIENT;

		return 1;
	}

	else if (apaint->format == COLR_PAINTFORMAT_RADIAL_GRADIENT ||
		(do_read_var =
			((PaintFormat_Internal)apaint->format ==
				COLR_PAINTFORMAT_INTERNAL_VAR_RADIAL_GRADIENT)))
	{
		Pos  tmp;


		if (!read_color_line(colr,
			child_table_p,
			&apaint->u.radial_gradient.colorline,
			do_read_var))
			return 0;


		/* In the OpenType specification, `r0` and `r1` are defined as   */
		/* `UFWORD`.  Since FreeType doesn't have a corresponding 16.16  */
		/* format we convert to `FWORD` and replace negative values with */
		/* (32bit) `INT_MAX`.                                         */

		ENSURE_READ_BYTES(12);

		apaint->u.radial_gradient.c0.x = INT_TO_FIXED(NEXT_SHORT(p));
		apaint->u.radial_gradient.c0.y = INT_TO_FIXED(NEXT_SHORT(p));

		tmp = INT_TO_FIXED(NEXT_SHORT(p));
		apaint->u.radial_gradient.r0 = tmp < 0 ? INT_MAX : tmp;

		apaint->u.radial_gradient.c1.x = INT_TO_FIXED(NEXT_SHORT(p));
		apaint->u.radial_gradient.c1.y = INT_TO_FIXED(NEXT_SHORT(p));

		tmp = INT_TO_FIXED(NEXT_SHORT(p));
		apaint->u.radial_gradient.r1 = tmp < 0 ? INT_MAX : tmp;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
		if (do_read_var)
		{
			ENSURE_READ_BYTES(4);
			var_index_base = NEXT_ULONG(p);

			if (!get_deltas_for_var_index_base(face, colr, var_index_base, 6,
				item_deltas))
				return 0;

			apaint->u.radial_gradient.c0.x += INT_TO_FIXED(item_deltas[0]);
			apaint->u.radial_gradient.c0.y += INT_TO_FIXED(item_deltas[1]);

			// TODO: Anything to be done about UFWORD deltas here?
			apaint->u.radial_gradient.r0 += INT_TO_FIXED(item_deltas[2]);

			apaint->u.radial_gradient.c1.x += INT_TO_FIXED(item_deltas[3]);
			apaint->u.radial_gradient.c1.y += INT_TO_FIXED(item_deltas[4]);

			apaint->u.radial_gradient.r1 += INT_TO_FIXED(item_deltas[5]);
		}
#endif

		apaint->format = COLR_PAINTFORMAT_RADIAL_GRADIENT;

		return 1;
	}

	else if (apaint->format == COLR_PAINTFORMAT_SWEEP_GRADIENT ||
		(do_read_var =
			((PaintFormat_Internal)apaint->format ==
				COLR_PAINTFORMAT_INTERNAL_VAR_SWEEP_GRADIENT)))
	{
		if (!read_color_line(colr,
			child_table_p,
			&apaint->u.sweep_gradient.colorline,
			do_read_var))
			return 0;

		ENSURE_READ_BYTES(8);

		apaint->u.sweep_gradient.center.x =
			INT_TO_FIXED(NEXT_SHORT(p));
		apaint->u.sweep_gradient.center.y =
			INT_TO_FIXED(NEXT_SHORT(p));

		apaint->u.sweep_gradient.start_angle =
			F2DOT14_TO_FIXED(NEXT_SHORT(p));
		apaint->u.sweep_gradient.end_angle =
			F2DOT14_TO_FIXED(NEXT_SHORT(p));

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
		if (do_read_var)
		{
			ENSURE_READ_BYTES(4);
			var_index_base = NEXT_ULONG(p);

			if (!get_deltas_for_var_index_base(face, colr, var_index_base, 4,
				item_deltas))
				return 0;

			// TODO: Handle overflow?
			apaint->u.sweep_gradient.center.x += INT_TO_FIXED(item_deltas[0]);
			apaint->u.sweep_gradient.center.y += INT_TO_FIXED(item_deltas[1]);

			apaint->u.sweep_gradient.start_angle +=
				F2DOT14_TO_FIXED(item_deltas[2]);
			apaint->u.sweep_gradient.end_angle +=
				F2DOT14_TO_FIXED(item_deltas[3]);
		}
#endif
		apaint->format = COLR_PAINTFORMAT_SWEEP_GRADIENT;

		return 1;
	}
#endif
	if (apaint->format == COLR_PAINTFORMAT_GLYPH)
	{
		ENSURE_READ_BYTES(2);
		apaint->u.glyph.p = child_table_p;
		//apaint->u.glyph.paint.insert_root_transform = 0;
		apaint->u.glyph.glyphID = NEXT_USHORT(p);

		return 1;
	}

	else if (apaint->format == COLR_PAINTFORMAT_TRANSFORM)//||		(PaintFormat_Internal)apaint->format ==		COLR_PAINTFORMAT_INTERNAL_VAR_TRANSFORM)
	{
		apaint->u.transform.p = child_table_p;
		//apaint->u.transform.paint.insert_root_transform = 0;

		if (!get_child_table_pointer(colr, paint_base, &p, &child_table_p))
			return 0;

		p = child_table_p;

		/*
		 * The following matrix coefficients are encoded as
		 * OpenType 16.16 fixed-point values.
		 */
		ENSURE_READ_BYTES(24);
		apaint->u.transform.affine.xx = NEXT_LONG(p);
		apaint->u.transform.affine.yx = NEXT_LONG(p);
		apaint->u.transform.affine.xy = NEXT_LONG(p);
		apaint->u.transform.affine.yy = NEXT_LONG(p);
		apaint->u.transform.affine.dx = NEXT_LONG(p);
		apaint->u.transform.affine.dy = NEXT_LONG(p);

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
		if ((PaintFormat_Internal)apaint->format ==
			COLR_PAINTFORMAT_INTERNAL_VAR_TRANSFORM)
		{
			ENSURE_READ_BYTES(4);
			var_index_base = NEXT_ULONG(p);

			if (!get_deltas_for_var_index_base(face, colr, var_index_base, 6,
				item_deltas))
				return 0;

			apaint->u.transform.affine.xx += (Fixed)item_deltas[0];
			apaint->u.transform.affine.yx += (Fixed)item_deltas[1];
			apaint->u.transform.affine.xy += (Fixed)item_deltas[2];
			apaint->u.transform.affine.yy += (Fixed)item_deltas[3];
			apaint->u.transform.affine.dx += (Fixed)item_deltas[4];
			apaint->u.transform.affine.dy += (Fixed)item_deltas[5];
		}
#endif

		apaint->format = COLR_PAINTFORMAT_TRANSFORM;

		return 1;
	}

	else if (apaint->format == COLR_PAINTFORMAT_TRANSLATE)// || (PaintFormat_Internal)apaint->format == COLR_PAINTFORMAT_INTERNAL_VAR_TRANSLATE)
	{
		apaint->u.translate.p = child_table_p;
		//apaint->u.translate.paint.insert_root_transform = 0;

		ENSURE_READ_BYTES(4);
		apaint->u.translate.dx = INT_TO_FIXED(NEXT_SHORT(p));
		apaint->u.translate.dy = INT_TO_FIXED(NEXT_SHORT(p));

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
		if ((PaintFormat_Internal)apaint->format ==
			COLR_PAINTFORMAT_INTERNAL_VAR_TRANSLATE)
		{
			ENSURE_READ_BYTES(4);
			var_index_base = NEXT_ULONG(p);

			if (!get_deltas_for_var_index_base(face, colr, var_index_base, 2,
				item_deltas))
				return 0;

			apaint->u.translate.dx += INT_TO_FIXED(item_deltas[0]);
			apaint->u.translate.dy += INT_TO_FIXED(item_deltas[1]);
		}
#endif

		apaint->format = COLR_PAINTFORMAT_TRANSLATE;

		return 1;
	}

	else if (apaint->format >= COLR_PAINTFORMAT_SCALE)// && (PaintFormat_Internal)apaint->format <= COLR_PAINTFORMAT_INTERNAL_VAR_SCALE_UNIFORM_CENTER)
	{
		apaint->u.scale.p = child_table_p;
		//apaint->u.scale.paint.insert_root_transform = 0;

		/* All scale paints get at least one scale value. */
		ENSURE_READ_BYTES(2);
		apaint->u.scale.scale_x = F2DOT14_TO_FIXED(NEXT_SHORT(p));

		/* Non-uniform ones read an extra y value. */
		//if (apaint->format == COLR_PAINTFORMAT_SCALE ||
		//	(PaintFormat_Internal)apaint->format ==
		//	COLR_PAINTFORMAT_INTERNAL_VAR_SCALE ||
		//	(PaintFormat_Internal)apaint->format ==
		//	COLR_PAINTFORMAT_INTERNAL_SCALE_CENTER ||
		//	(PaintFormat_Internal)apaint->format ==
		//	COLR_PAINTFORMAT_INTERNAL_VAR_SCALE_CENTER)
		//{
		//	ENSURE_READ_BYTES(2);
		//	apaint->u.scale.scale_y = F2DOT14_TO_FIXED(NEXT_SHORT(p));
		//}
		//else
		apaint->u.scale.scale_y = apaint->u.scale.scale_x;

		/* Scale paints that have a center read center coordinates, */
		/* otherwise the center is (0,0).                           */
		//if ((PaintFormat_Internal)apaint->format ==
		//	COLR_PAINTFORMAT_INTERNAL_SCALE_CENTER ||
		//	(PaintFormat_Internal)apaint->format ==
		//	COLR_PAINTFORMAT_INTERNAL_VAR_SCALE_CENTER ||
		//	(PaintFormat_Internal)apaint->format ==
		//	COLR_PAINTFORMAT_INTERNAL_SCALE_UNIFORM_CENTER ||
		//	(PaintFormat_Internal)apaint->format ==
		//	COLR_PAINTFORMAT_INTERNAL_VAR_SCALE_UNIFORM_CENTER)
		//{
		//	ENSURE_READ_BYTES(4);
		//	apaint->u.scale.center_x = INT_TO_FIXED(NEXT_SHORT(p));
		//	apaint->u.scale.center_y = INT_TO_FIXED(NEXT_SHORT(p));
		//}
		//else
		{
			apaint->u.scale.center_x = 0;
			apaint->u.scale.center_y = 0;
		}

		/* Base values set, now handle variations. */

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
		if ((PaintFormat_Internal)apaint->format ==
			COLR_PAINTFORMAT_INTERNAL_VAR_SCALE ||
			(PaintFormat_Internal)apaint->format ==
			COLR_PAINTFORMAT_INTERNAL_VAR_SCALE_CENTER ||
			(PaintFormat_Internal)apaint->format ==
			COLR_PAINTFORMAT_INTERNAL_VAR_SCALE_UNIFORM ||
			(PaintFormat_Internal)apaint->format ==
			COLR_PAINTFORMAT_INTERNAL_VAR_SCALE_UNIFORM_CENTER)
		{
			ENSURE_READ_BYTES(4);
			var_index_base = NEXT_ULONG(p);

			if ((PaintFormat_Internal)apaint->format ==
				COLR_PAINTFORMAT_INTERNAL_VAR_SCALE)
			{
				if (!get_deltas_for_var_index_base(face, colr, var_index_base, 2,
					item_deltas))
					return 0;

				apaint->u.scale.scale_x += F2DOT14_TO_FIXED(item_deltas[0]);
				apaint->u.scale.scale_y += F2DOT14_TO_FIXED(item_deltas[1]);
			}

			if ((PaintFormat_Internal)apaint->format ==
				COLR_PAINTFORMAT_INTERNAL_VAR_SCALE_CENTER)
			{
				if (!get_deltas_for_var_index_base(face, colr, var_index_base, 4,
					item_deltas))
					return 0;

				apaint->u.scale.scale_x += F2DOT14_TO_FIXED(item_deltas[0]);
				apaint->u.scale.scale_y += F2DOT14_TO_FIXED(item_deltas[1]);
				apaint->u.scale.center_x += INT_TO_FIXED(item_deltas[2]);
				apaint->u.scale.center_y += INT_TO_FIXED(item_deltas[3]);
			}

			if ((PaintFormat_Internal)apaint->format ==
				COLR_PAINTFORMAT_INTERNAL_VAR_SCALE_UNIFORM)
			{
				if (!get_deltas_for_var_index_base(face, colr, var_index_base, 1,
					item_deltas))
					return 0;

				apaint->u.scale.scale_x += F2DOT14_TO_FIXED(item_deltas[0]);
				apaint->u.scale.scale_y += F2DOT14_TO_FIXED(item_deltas[0]);
			}

			if ((PaintFormat_Internal)apaint->format ==
				COLR_PAINTFORMAT_INTERNAL_VAR_SCALE_UNIFORM_CENTER)
			{
				if (!get_deltas_for_var_index_base(face, colr, var_index_base, 3,
					item_deltas))
					return 0;

				apaint->u.scale.scale_x += F2DOT14_TO_FIXED(item_deltas[0]);
				apaint->u.scale.scale_y += F2DOT14_TO_FIXED(item_deltas[0]);
				apaint->u.scale.center_x += INT_TO_FIXED(item_deltas[1]);
				apaint->u.scale.center_y += INT_TO_FIXED(item_deltas[2]);
			}
		}
#endif

		/* FT 'COLR' v1 API output format always returns fully defined */
		/* structs; we thus set the format to the public API value.    */
		apaint->format = COLR_PAINTFORMAT_SCALE;

		return 1;
	}

	else if (apaint->format == COLR_PAINTFORMAT_ROTATE)// || (PaintFormat_Internal)apaint->format == COLR_PAINTFORMAT_INTERNAL_ROTATE_CENTER || (PaintFormat_Internal)apaint->format == COLR_PAINTFORMAT_INTERNAL_VAR_ROTATE || (PaintFormat_Internal)apaint->format == COLR_PAINTFORMAT_INTERNAL_VAR_ROTATE_CENTER)
	{
		apaint->u.rotate.p = child_table_p;
		//apaint->u.rotate.paint.insert_root_transform = 0;

		ENSURE_READ_BYTES(2);
		apaint->u.rotate.angle = F2DOT14_TO_FIXED(NEXT_SHORT(p));

		//if ((PaintFormat_Internal)apaint->format == COLR_PAINTFORMAT_INTERNAL_ROTATE_CENTER || (PaintFormat_Internal)apaint->format == COLR_PAINTFORMAT_INTERNAL_VAR_ROTATE_CENTER)
		//{
		//	ENSURE_READ_BYTES(4);
		//	apaint->u.rotate.center_x = INT_TO_FIXED(NEXT_SHORT(p));
		//	apaint->u.rotate.center_y = INT_TO_FIXED(NEXT_SHORT(p));
		//}
		//else
		{
			apaint->u.rotate.center_x = 0;
			apaint->u.rotate.center_y = 0;
		}

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
		if ((PaintFormat_Internal)apaint->format ==
			COLR_PAINTFORMAT_INTERNAL_VAR_ROTATE ||
			(PaintFormat_Internal)apaint->format ==
			COLR_PAINTFORMAT_INTERNAL_VAR_ROTATE_CENTER)
		{
			uint32_t  num_deltas = 0;


			ENSURE_READ_BYTES(4);
			var_index_base = NEXT_ULONG(p);

			if ((PaintFormat_Internal)apaint->format ==
				COLR_PAINTFORMAT_INTERNAL_VAR_ROTATE_CENTER)
				num_deltas = 3;
			if ((PaintFormat_Internal)apaint->format ==
				COLR_PAINTFORMAT_INTERNAL_VAR_ROTATE)
				num_deltas = 1;

			if (num_deltas > 0)
			{
				if (!get_deltas_for_var_index_base(face, colr, var_index_base,
					num_deltas, item_deltas))
					return 0;

				apaint->u.rotate.angle += F2DOT14_TO_FIXED(item_deltas[0]);

				if (num_deltas == 3)
				{
					apaint->u.rotate.center_x += INT_TO_FIXED(item_deltas[1]);
					apaint->u.rotate.center_y += INT_TO_FIXED(item_deltas[2]);
				}
			}
		}
#endif

		apaint->format = COLR_PAINTFORMAT_ROTATE;


		return 1;
	}
#if 0
	else if (apaint->format == COLR_PAINTFORMAT_SKEW ||
		(PaintFormat_Internal)apaint->format ==
		COLR_PAINTFORMAT_INTERNAL_VAR_SKEW ||
		(PaintFormat_Internal)apaint->format ==
		COLR_PAINTFORMAT_INTERNAL_SKEW_CENTER ||
		(PaintFormat_Internal)apaint->format ==
		COLR_PAINTFORMAT_INTERNAL_VAR_SKEW_CENTER)
	{
		apaint->u.skew.paint.p = child_table_p;
		apaint->u.skew.paint.insert_root_transform = 0;

		ENSURE_READ_BYTES(4);
		apaint->u.skew.x_skew_angle = F2DOT14_TO_FIXED(NEXT_SHORT(p));
		apaint->u.skew.y_skew_angle = F2DOT14_TO_FIXED(NEXT_SHORT(p));

		if ((PaintFormat_Internal)apaint->format ==
			COLR_PAINTFORMAT_INTERNAL_SKEW_CENTER ||
			(PaintFormat_Internal)apaint->format ==
			COLR_PAINTFORMAT_INTERNAL_VAR_SKEW_CENTER)
		{
			ENSURE_READ_BYTES(4);
			apaint->u.skew.center_x = INT_TO_FIXED(NEXT_SHORT(p));
			apaint->u.skew.center_y = INT_TO_FIXED(NEXT_SHORT(p));
		}
		else
		{
			apaint->u.skew.center_x = 0;
			apaint->u.skew.center_y = 0;
		}


#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
		if ((PaintFormat_Internal)apaint->format ==
			COLR_PAINTFORMAT_INTERNAL_VAR_SKEW ||
			(PaintFormat_Internal)apaint->format ==
			COLR_PAINTFORMAT_INTERNAL_VAR_SKEW_CENTER)
		{
			ENSURE_READ_BYTES(4);
			var_index_base = NEXT_ULONG(p);

			if ((PaintFormat_Internal)apaint->format ==
				COLR_PAINTFORMAT_INTERNAL_VAR_SKEW)
			{
				if (!get_deltas_for_var_index_base(face, colr, var_index_base, 2,
					item_deltas))
					return 0;

				apaint->u.skew.x_skew_angle += F2DOT14_TO_FIXED(item_deltas[0]);
				apaint->u.skew.y_skew_angle += F2DOT14_TO_FIXED(item_deltas[1]);
			}

			if ((PaintFormat_Internal)apaint->format ==
				COLR_PAINTFORMAT_INTERNAL_VAR_SKEW_CENTER)
			{
				if (!get_deltas_for_var_index_base(face, colr, var_index_base, 4,
					item_deltas))
					return 0;

				apaint->u.skew.x_skew_angle += F2DOT14_TO_FIXED(item_deltas[0]);
				apaint->u.skew.y_skew_angle += F2DOT14_TO_FIXED(item_deltas[1]);
				apaint->u.skew.center_x += INT_TO_FIXED(item_deltas[2]);
				apaint->u.skew.center_y += INT_TO_FIXED(item_deltas[3]);
			}
		}
#endif

		apaint->format = COLR_PAINTFORMAT_SKEW;

		return 1;
	}

	else if (apaint->format == COLR_PAINTFORMAT_COMPOSITE)
	{
		uint32_t  composite_mode;


		apaint->u.composite.source_paint.p = child_table_p;
		apaint->u.composite.source_paint.insert_root_transform = 0;

		ENSURE_READ_BYTES(1);
		composite_mode = NEXT_BYTE(p);
		if (composite_mode >= COLR_COMPOSITE_MAX)
			return 0;

		apaint->u.composite.composite_mode = (Composite_Mode)composite_mode;

		if (!get_child_table_pointer(colr, paint_base, &p, &child_table_p))
			return 0;

		apaint->u.composite.backdrop_paint.p = child_table_p;
		apaint->u.composite.backdrop_paint.insert_root_transform = 0;

		return 1;
	}
#endif
	return 0;
}






#endif // !NO_COLOR_FONT





int font_t::get_glyph_bitmap(int gidx, int height, glm::ivec4* ot, Bitmap_p* out_bitmap, std::vector<char>* out)
{
	Bitmap_p* bitmap = 0;
	int x_pos = 0, y_pos = 0, ret = 0;
	int sidx = bitinfo->get_sidx(height);
	if (sidx < 0)
	{
		return 0;
	}
	int x = 10, y = 10;
	SBitDecoder* decoder = bitinfo->new_SBitDecoder();
	decoder->init(this, sidx);
	bitmap = decoder->bitmap;
	BigGlyphMetrics* metrics = decoder->metrics;

	if (get_index(decoder, gidx, x_pos, y_pos))
	{
		if (out) {
			out->resize((uint64_t)bitmap->rows * bitmap->pitch);
			memcpy(out->data(), bitmap->buffer, out->size());
		}
		if (ot) {
			ot->x = metrics->horiBearingX;
			ot->y = -metrics->horiBearingY;
			ot->z = bitmap->width;
			ot->w = bitmap->rows;
		}
		auto ha = metrics->horiAdvance;
		bitmap->advance = std::max(metrics->horiAdvance, metrics->vertAdvance);
		if (out_bitmap)
		{
			*out_bitmap = *bitmap;
			if (out)
				out_bitmap->buffer = (unsigned char*)out->data();
		}
		ret = 2;
	}
	else
	{
		ret = 0;
	}
	bitinfo->recycle(decoder);
	return ret;
}

void init_bitmap_bitdepth(Bitmap_p* bitmap, int bit_depth)
{
	bitmap->bit_depth = bit_depth;

	switch (bit_depth)
	{
	case 1:
		bitmap->pixel_mode = Pixel_Mode::PX_MONO;
		bitmap->pitch = (int)((bitmap->width + 7) >> 3);
		bitmap->num_grays = 2;
		break;

	case 2:
		bitmap->pixel_mode = Pixel_Mode::PX_GRAY2;
		bitmap->pitch = (int)((bitmap->width + 3) >> 2);
		bitmap->num_grays = 4;
		break;

	case 4:
		bitmap->pixel_mode = Pixel_Mode::PX_GRAY4;
		bitmap->pitch = (int)((bitmap->width + 1) >> 1);
		bitmap->num_grays = 16;
		break;

	case 8:
		bitmap->pixel_mode = Pixel_Mode::PX_GRAY;
		bitmap->pitch = (int)(bitmap->width);
		bitmap->num_grays = 256;
		break;

	case 32:
		bitmap->pixel_mode = Pixel_Mode::PX_BGRA;
		bitmap->pitch = (int)(bitmap->width * 4);
		bitmap->num_grays = 256;
		break;

	default:
		return;
	}
}
int font_t::get_custom_decoder_bitmap(uint32_t unicode_codepoint, int height, glm::ivec4* ot, Bitmap_p* out_bitmap, std::vector<char>* out)
{
	auto& bch = bitinfo->_chsize;
	auto& bur = bitinfo->_unicode_rang;
	auto img = (image_ptr_t*)&bitinfo->_bimg;
	int ret = 0;
	if (img && bch.size())
	{
		// 910=12，11=14，12/13=16f
		glm::ivec2 uc = {}, bc = {}; size_t idx = 0;
		int inc = 2;
		if (height & 1)
		{
			height--;
			inc--;
		}
		for (size_t i = 0; i < bch.size(); i++)
		{
			auto& it = bch[i];
			if (it.x == height)
			{
				uc = bur[i];
				bc = it;
				break;
			}
			idx += it.x;
		}
		if (!uc.x || !uc.y || !unicode_codepoint)
		{
			return ret;
		}
		if (unicode_codepoint >= uc.x && unicode_codepoint <= uc.y)
		{
			int px = (unicode_codepoint - uc.x) * bc.y;
			int py = idx;
			out_bitmap->rows = bc.x;
			out_bitmap->width = bc.y;
			init_bitmap_bitdepth(out_bitmap, 32);
			// RGBA
			out_bitmap->pitch = bc.y * 4;
			out_bitmap->advance = bc.y;
			if (ot) {
				ot->x = 0;
				ot->y = -bc.x + inc;
				ot->z = bc.y;
				ot->w = bc.x;
			}

			size_t size = out_bitmap->rows * (uint64_t)out_bitmap->pitch;
			if (out) {
				if (size > out->size())
				{
					out->resize(size);
				}
				out_bitmap->buffer = (unsigned char*)out->data();
				std::vector<uint32_t> tem;
				tem.reserve(bc.y * bc.x);
				size_t x1 = std::min(img->width, px + bc.y);
				size_t y1 = std::min(img->height, py + bc.x);
				for (size_t y = py; y < y1; y++)
				{
					for (size_t x = px; x < x1; x++)
					{
						auto c = img->data[y * img->width + x];
						if (c == 0xff000000)
						{
							c = 0;
						}
						tem.push_back(c);
					}
				}
				memcpy(out_bitmap->buffer, tem.data(), size);
			}
			ret = 1;
		}
	}
	return ret;
}



packer_base::packer_base()
{
}
packer_base::~packer_base()
{
}

void packer_base::init_target(int width, int height) {}
void packer_base::clear() {}
int packer_base::push_rect(glm::ivec4* rc, int n) { return 0; }
int packer_base::push_rect(glm::ivec2 rc, glm::ivec2* pos) { return 0; }



#if 0
ndef STB_RECT_PACK_VERSION

struct stbrp_rect
{
	// reserved for your use:
	int            id;

	// input:
	uint16_t    w, h;

	// output:
	uint16_t    x, y;
	int            was_packed;  // non-zero if valid packing

}; // 16 bytes, nominally
#endif
#ifdef STB_RECT_PACK_VERSION
class stb_packer :public packer_base
{
public:
	stbrp_context _ctx = {};
	image_ptr_t img = {};
#ifdef STL_VU
	std::vector<uint32_t> ptr;
	std::vector<stbrp_node> _rpns;
#else
	uint32_t* _ptr = nullptr;
	stbrp_node* _rpns_ptr = nullptr;
	size_t cap = 0;
#endif
public:
	stb_packer() {}
	~stb_packer() {
#ifndef STL_VU 
		if (_ptr)
		{
			free(_ptr);
		}
#endif
	}
	image_ptr_t* get() {
		return (image_ptr_t*)&img;
	}
	// BL = 0 “从下向左塞”（快速降低高度）
	// BF = 1 “精打细算”（最小化空间浪费）
	void init_target(int width, int height, int heuristic = 0) {
		assert(!(width < 10 || height < 10));
		if (width < 10 || height < 10)return;
#ifdef STL_VU
		if (newptr) { ptr.resize(width * height); }
#else
		size_t ac = width * height * sizeof(uint32_t);
		ac += sizeof(stbrp_node) * width;
		if (cap != ac) {
			auto nptr = (uint32_t*)realloc(_ptr, ac);
			if (nptr)
			{
				cap = ac;
				_ptr = nptr;
			}
			else {
				assert(nptr);
				return;
			}
		}
#endif // STL_VU 
		auto img = get();
		img->width = width;
		img->height = height;
		img->valid = 1;
		this->width = width;
		this->height = height;
#ifdef STL_VU
		img->data = ptr.data();
		_rpns.resize(width);
		memset(_rpns.data(), 0, _rpns.size() * sizeof(stbrp_node));
		stbrp_init_target(&_ctx, width, height, _rpns.data(), _rpns.size());
#else
		img->data = _ptr;
		_rpns_ptr = (stbrp_node*)(_ptr + width * height);
		memset(_rpns_ptr, 0, width * sizeof(stbrp_node));
		stbrp_init_target(&_ctx, width, height, _rpns_ptr, width);
#endif
		stbrp_setup_heuristic(&_ctx, heuristic);
		memset(_ptr, 0, width * height * sizeof(uint32_t));
		stbrp_setup_allow_out_of_mem(&_ctx, 0);
	}
	void clear() {
		init_target(_ctx.width, _ctx.height);
	}
	int push_rect(glm::ivec4* rc, int n)
	{
		if (!rc || n < 1)return 0;
		std::vector<stbrp_rect> rct(n);
		auto r = rc;
		for (auto& it : rct)
		{
			it.w = r->z; it.h = r->w; r++;
		}
		int ret = stbrp_pack_rects(&_ctx, rct.data(), n);
		r = rc;
		for (auto& it : rct)
		{
			r->x = it.x; r->y = it.y; r++;
		}
		auto img = get();
		img->valid = 1;
		return ret;
	}
	int push_rect(glm::ivec2 rc, glm::ivec2* pos)
	{
		stbrp_rect rct[2] = {};
		rct->w = rc.x;
		rct->h = rc.y;
		int ret = stbrp_pack_rects(&_ctx, rct, 1);
		if (pos)
		{
			*pos = { rct->x,rct->y };
		}
		auto img = get();
		img->valid = 1;
		return ret;
	}
public:
	// todo stb结构
	int pack_rects(stbrp_rect* rects, int num_rects)
	{
		return stbrp_pack_rects(&_ctx, rects, num_rects);
	}
	void setup_allow_out_of_mem(int allow_out_of_mem)
	{
		stbrp_setup_allow_out_of_mem(&_ctx, allow_out_of_mem);
	}
	//可以选择库应该使用哪个打包启发式方法。不同启发式方法将为不同的数据集生成更好/更差的结果。 如果再次调用init，将重置为默认值。	
	void setup_heuristic(int heuristic = 1)
	{
		stbrp_setup_heuristic(&_ctx, heuristic);
	}
private:

};

void test_rect()
{
	glm::ivec2 r = { 730,1000 };
	stb_packer pack;
	pack.init_target(r.x, r.y);
	//hz::Image tespack;
	//tespack.resize(r.x, r.y);
	std::vector<uint32_t> colors = {
	0xff0000ff,
	0xff00ff00,
	0xffff0000,
	0xff0080ff,
	0xff8000ff,
	0xff00ff80,
	0xff80ff00,
	0xffff0080,
	0xffff8000,

	};
	std::vector<glm::ivec2> rects = {
		//{1120,800},
		{700,300},
		{470,300},
		{230,140},
		{230,140},
		{350,140},
		{360,300},
		{350,140},
		{350,140},
		{350,140},
		{350,140},
		{350,140},
		{350,140},
	};
	{
		//print_time at("stb pack");
		int i = 0;
		for (auto& it : rects)
		{
			glm::ivec2 prs;
			int k = pack.push_rect({ it.x,it.y }, &prs);
			if (k)
			{
				glm::ivec4 trc = { prs.x,prs.y,it.x,it.y };
				auto c = colors[i++];
				c |= 0x80000000;
				//tespack.draw_rect(trc, 0, c);
			}
		}
	}
	std::string fn = "test_packer_stb.png";
	//tespack.saveImage(fn);
	exit(0);
}

#endif // STB_RECT_PACK_VERSION

packer_base* new_packer(int width, int height)
{
	auto p = new stb_packer();
	if (p)
	{
		p->init_target(width, height);
	}
	return p;
}

void free_packer(packer_base* p)
{
	if (p)
	{
		delete p;
	}
}

bitmap_cache_cx::bitmap_cache_cx()
{
	resize(width, height);
	// 填充20x20白色
	fill_color(20, 20, -1);
}

bitmap_cache_cx::~bitmap_cache_cx()
{
}

void bitmap_cache_cx::resize(int w, int h)
{
	if (w < 10 || h < 10 || (w == width && h == height))return;
	clear();
	width = w;
	height = h;
}

glm::ivec2 bitmap_cache_cx::fill_color(int w, int h, uint32_t color)
{
	glm::ivec2 pos = {};
	auto pt = get_last_packer(false);
	if (!pt || w < 1 || h < 1)return pos;
	auto ret = pt->push_rect({ w, h }, &pos);
	auto ptr = pt->get();
	auto px = ((uint32_t*)ptr->data) + pos.x;
	px += pos.y * width;
	for (size_t i = 0; i < h; i++)
	{
		for (size_t x = 0; x < w; x++)
		{
			px[x] = color;
		}
		px += width;
	}
	return pos;
}

void bitmap_cache_cx::clear()
{
	for (auto it : _packer)
	{
		if (it)delete it;
	}
	_packer.clear();
	_data.clear();
}
stb_packer* bitmap_cache_cx::get_last_packer(bool isnew)
{
	if (_packer.empty() || isnew)
	{
		auto p = new stb_packer();
		if (!p)return 0;
		_packer.push_back(p);
		p->init_target(width, height);
		_data.push_back(p->get());
	}
	return *_packer.rbegin();
}
image_ptr_t* bitmap_cache_cx::push_cache_bitmap(Bitmap_p* bitmap, glm::ivec2* pos, int linegap, uint32_t col)
{
	int width = align_up(bitmap->width + linegap, 2), height = align_up(bitmap->rows + linegap, 2);
	glm::ivec4 rc4 = { 0, 0, bitmap->width, bitmap->rows };
	auto pt = get_last_packer(false);
	if (!pt)return 0;
	auto ret = pt->push_rect({ width, height }, pos);
	if (!ret)
	{
		pt = get_last_packer(true);
		ret = pt->push_rect({ width, height }, pos);
	}
	if (ret)
	{
		rc4.x = pos->x;
		rc4.y = pos->y;
		image_ptr_t src = {}, dst = {};
		auto ptr = pt->get();
		dst.data = (uint32_t*)ptr->data;
		dst.width = ptr->width;
		dst.height = ptr->height;

		src.data = (uint32_t*)bitmap->buffer;
		src.stride = bitmap->pitch;
		src.width = bitmap->width;
		src.height = bitmap->rows;
		src.comp = 1;
		switch (bitmap->bit_depth)
		{
		case 1:
		{
			bit_copy2rgba(&dst, &src, { rc4.x,rc4.y }, { 0,0,rc4.z,rc4.w }, col);
		}
		break;
		case 8:
		{
			gray_copy2rgba(&dst, &src, { rc4.x,rc4.y }, { 0,0,rc4.z,rc4.w }, col, true);
		}
		break;
		case 32:
		{
			src.comp = 4;
			rgba_copy2rgba(&dst, &src, { rc4.x,rc4.y }, { 0,0,rc4.z,rc4.w }, -1, true);
		}
		break;
		default:
			break;
		}
		ptr->valid = 1; // 更新缓存标志
	}
	return pt->get();
}
image_ptr_t* bitmap_cache_cx::push_cache_bitmap(image_ptr_t* bitmap, glm::ivec2* pos, int linegap, uint32_t col)
{
	int width = align_up(bitmap->width + linegap, 2), height = align_up(bitmap->height + linegap, 2);
	glm::ivec4 rc4 = { 0, 0, bitmap->width, bitmap->height };
	auto pt = get_last_packer(false);
	if (!pt)return 0;
	auto ret = pt->push_rect({ width, height }, pos);
	if (!ret)
	{
		pt = get_last_packer(true);
		ret = pt->push_rect({ width, height }, pos);
	}
	if (ret)
	{
		rc4.x = pos->x;
		rc4.y = pos->y;
		image_ptr_t src = *bitmap, dst = {};
		auto ptr = pt->get();
		dst.data = (uint32_t*)ptr->data;
		dst.width = ptr->width;
		dst.height = ptr->height;

		switch (bitmap->comp)
		{
		case 0:
		{
			src.comp = 1;
			bit_copy2rgba(&dst, &src, { rc4.x,rc4.y }, { 0,0,rc4.z,rc4.w }, col);
		}
		break;
		case 1:
		{
			src.comp = 1;
			gray_copy2rgba(&dst, &src, { rc4.x,rc4.y }, { 0,0,rc4.z,rc4.w }, col, true);
		}
		break;
		case 4:
		{
			rgba_copy2rgba(&dst, &src, { rc4.x,rc4.y }, { 0,0,rc4.z,rc4.w }, -1, true);
		}
		break;
		default:
			break;
		}
		ptr->valid = 1; // 更新缓存标志
	}
	return pt->get();
}
image_ptr_t* bitmap_cache_cx::push_cache_bitmap_old(Bitmap_p* bitmap, glm::ivec2* pos, uint32_t col, image_ptr_t* oldimg, int linegap)
{
	if (!oldimg)
	{
		return push_cache_bitmap(bitmap, pos, linegap, col);
	}
	else {
		glm::ivec4 rc4 = { 0, 0, bitmap->width, bitmap->rows };
		rc4.x = pos->x;
		rc4.y = pos->y;

		image_ptr_t src = {}, dst = {};
		dst.data = (uint32_t*)oldimg->data;
		dst.width = oldimg->width;
		dst.height = oldimg->height;

		src.data = (uint32_t*)bitmap->buffer;
		src.stride = bitmap->pitch;
		src.width = bitmap->width;
		src.height = bitmap->rows;
		src.comp = 1;
		gray_copy2rgba(&dst, &src, { rc4.x,rc4.y }, { 0,0,rc4.z,rc4.w }, col, true);
		oldimg->valid = 1;
	}
	return oldimg;
}


//image_ptr_t to_imgptr(image_ptr_t* p)
//{
//	image_ptr_t r = {};
//	r.width = p->width;
//	r.height = p->height;
//	r.data = p->data;
//	r.type = p->format;
//	r.comp = 4;
//	return r;
//}




std::map<int, std::vector<info_one>> font_t::get_detail()
{
	std::map<int, std::vector<info_one>> detail;
	stb_font ft;
	ft.get_font_name(font, &detail);
	return detail;
}


class fd_info0
{
public:
	hz::mfile_t* _fp = nullptr;
	char* _data = nullptr;
	int64_t _size = 0;
	std::set<std::string> vname;
public:
	fd_info0()
	{
	}
	fd_info0(hz::mfile_t* p)
	{
		init(p);
	}

	~fd_info0()
	{
		free_mv();
	}
	void free_mv()
	{
		auto dp = _data;
		if (_fp)
		{
			if (dp == _fp->data())dp = 0;
			delete (_fp); _fp = 0;
		}
		if (dp)
		{
			delete[]dp; dp = 0;
		}
	}
	void init(hz::mfile_t* p)
	{
		_fp = p;
		if (_fp)
		{
			_data = (char*)_fp->data();
			_size = _fp->size();
		}
	}
	char* data()
	{
		return _data;
	}
	int64_t size()
	{
		return _size;
	}
private:

};
font_imp::font_imp()
{
}

font_imp::~font_imp()
{
#if 0
	if (prun)
	{
		delete prun; prun = 0;
	}
#endif
	for (auto it : fd_data)
	{
		delete it;
	}
	for (auto it : fonts)
	{
		delete it;
	}
	for (auto it : fd_data_m)
	{
		delete[] it.data;
	}
}
size_t font_imp::size()
{
	return fonts.size();
}
std::vector<font_t*> font_imp::add_font_file(const std::string& fn, std::vector<std::string>* pname)
{
	std::vector<font_t*> ret;
	auto mv = new hz::mfile_t();
	auto md = mv->open_d(fn, true);
	if (!md)
	{
		delete mv;
		return ret;
	}
	if (mv && mv->get_file_size())
	{
		int rc = 0;
		auto fdi = new fd_info0(mv);
		ret = add_font_mem(fdi->data(), fdi->size(), false, pname, &rc);
		if (rc > 0)
		{
			fd_data.emplace_back(fdi);
			for (auto it : ret)
			{
				fdi->vname.insert(it->_name);
			}
		}
		else {
			delete fdi;
		}
	}
	return ret;
}

std::vector<font_t*> font_imp::add_font_mem(const char* data, size_t len, bool iscp, std::vector<std::string>* pname, int* rc)
{
	std::vector<font_t*> fp;
	mem_ft mft = {};
	if (iscp)
	{
		char* cpd = new char[len];
		if (!cpd)return fp;
		memcpy(cpd, data, len);
		mft.data = cpd;
		data = cpd;
	}
	stb_font ft;
	int nums = ft.get_numbers(data);
	int hr = 0;
	std::vector<std::string> tpn;
	std::string sv;
	if (!pname)
		pname = &tpn;
	std::set<font_t*> fs;
	for (size_t i = 0; i < nums; i++)
	{
		font_t* font = new font_t();
		hr = ft.loadFont(font->font, data, i, 0);
		if (hr)
		{
			font->num_glyphs = font->font->numGlyphs;
			int ascent = 0, descent = 0, lineGap = 0;
			ft.getFontVMetrics(font->font, &ascent, &descent, &lineGap);
			auto fh = ascent - descent;
			std::vector<meta_tag>	_meta;
			std::map<int, std::vector<info_one>> detail;
			//auto& detail = font->detail;
			ft.get_font_name(font->font, &detail);
			//font->fh = fh;
			font->dataSize = len;
			font->_index = i;
			font->ascender = (float)ascent;
			font->descender = (float)descent;
			font->lineh = (float)(fh + lineGap);
			font->_name = get_info_str(2052, 1, detail);
			font->fullname = get_info_str(2052, 4, detail);
			font->_namew = md::u8_w(font->_name);
			auto& mt = mk[font->_name];
			if (mt)
			{
				fs.insert(font);
				fp.push_back(mt);
				continue;
			}
			mft.vname.insert(font->_name);
			if (rc)
				*rc += 1;
			mt = font;
			fp.push_back(font);

			fonts.push_back(font);
			if (pname)
			{
				pname->push_back(font->_name);
			}
			auto cn_name = get_info_str(1033, 1, detail);
			//font->_aname = u8_gbk(font->_name);
			auto a_style = get_info_str(2052, 2, detail);
			auto a_style1 = get_info_str(1033, 2, detail);
			auto str6 = get_info_str(2052, 6, detail);
			auto str61 = get_info_str(1033, 6, detail);
			font->_style = a_style1;
			ft.get_meta_string(font->font, _meta);
			//ft.get_gsub_string(font->font);
			for (auto& it : _meta)
			{
				if (it.tag == "slng")
				{
					font->_slng = it.v;
				}
			}
			font->init_post_table();
			font->init_color();
#ifndef _FONT_NO_BITMAP
			font->init_sbit();
			if (cn_name == "NSimSun")
			{
				nsimsun_ascii(font->bitinfo);
			}
#endif // !_FONT_NO_BITMAP
		}
		else
		{
			fs.insert(font);
		}
	}
	for (auto it : fs)
	{
		if (it)
			delete it;
	}
	if (mft.vname.size())
	{
		fd_data_m.push_back(mft);
	}
	return fp;
}

int font_imp::get_font_names(std::vector<std::string>* pname)
{
	if (pname)
	{
		pname->clear();
		pname->reserve(fonts.size());
		for (auto it : fonts)
		{
			pname->push_back(it->_name);
		}
	}
	return fonts.size();
}
const char* font_imp::get_font_names(const char* split)
{
	if (fonts.empty())return 0;
	if (!split || !*split)
	{
		split = ",";
	}
	std::string str;
	for (auto& it : fonts)
	{
		str += it->_name + split;
	}
	addstr = str.c_str();
	return addstr.c_str();
}

font_t* font_imp::get_font(size_t idx)
{
	font_t* p = 0;
	if (fonts.empty())return p;
	if (idx < fonts.size())
	{
		p = fonts[idx];
	}
	else {
		p = *fonts.rbegin();
	}
	return p;
}
void font_imp::free_ft(const std::string& name)
{
	font_t* p = 0;
	for (auto it : fonts) {
		if (it->_name == name)
		{
			p = it; free_ftp(p);
			break;
		}
	}
}
void font_imp::free_ftp(font_t* p)
{
	for (auto pt = fd_data.begin(); pt != fd_data.end(); pt++) {
		auto mt = *pt;
		auto br = mt->vname.find(p->_name);
		if (br != mt->vname.end()) {
			mt->vname.erase(p->_name);
			if (mt->vname.empty())
			{
				auto& v = fd_data;
				delete mt;
				v.erase(pt);
			}
			break;
		}
	}

	for (auto pt = fd_data_m.begin(); pt != fd_data_m.end(); pt++) {
		auto& mt = pt;
		auto br = mt->vname.find(p->_name);
		if (br != mt->vname.end()) {
			mt->vname.erase(p->_name);
			if (mt->vname.empty())
			{
				auto& v = fd_data_m;
				delete[]mt->data;
				v.erase(pt);
			}
			break;
		}
	}
	auto& v = fonts;
	delete p;
	v.erase(std::remove(v.begin(), v.end(), p), v.end());
}
#endif


font_rctx::font_rctx()
{
	fyv = get_allfont();
	if (fyv.size())
	{
		fyvs.resize(fyv.size());
		size_t i = 0;
		for (auto& [k, v] : fyv) {
			fyvs[i] = &v; i++;
		}
	}

	imp = new font_imp();
}

font_rctx::~font_rctx()
{
	if (imp)delete imp;
	imp = 0;
	fyv.clear();
}

int font_rctx::get_count()
{
	return fyv.size();
}

int font_rctx::get_count_style(int idx)
{
	int r = 0;
	if (idx >= 0 && idx < fyvs.size())
	{
		r = fyvs[idx]->style.size();
	}
	return r;
}

const char* font_rctx::get_family(int idx)
{
	std::string* r = 0;
	if (idx >= 0 && idx < fyvs.size())
	{
		return fyvs[idx]->family.c_str();
	}
	return "";
}

const char* font_rctx::get_family_en(const char* family)
{
	const std::string* r = 0;
	std::string ret;
	if (!r)
	{
		std::set<std::string> nst;
		std::string n;
		auto t = family;
		for (size_t i = 0; *t; i++)
		{
			if (*t == ',')
			{
				nst.insert(n); n.clear();
			}
			else {
				n.push_back(*t);
			}
			t++;
		}
		if (n.size())
		{
			nst.insert(n);
		}
		for (auto& it : nst) {
			const std::string* r = 0;
			for (auto& [k, v] : fyv) {
				auto ats = v.alias.find(it);
				if (k.find(it) != std::string::npos || v.fullname == it || ats != v.alias.end())
				{
					r = &k;
					break;
				}
			}
			ret += (r) ? r->c_str() : it;
			ret.push_back(',');
		}
		if (ret.size() > 1)
		{
			ret.pop_back();
		}
		temfamily = ret;
		r = &temfamily;
	}
	return r ? r->c_str() : nullptr;
}

const char* font_rctx::get_family_cn(int idx)
{
	if (idx >= 0 && idx < fyvs.size())
	{
		auto v = fyvs[idx];
		auto r = v->family.c_str();
		if (v->alias.size())
			r = v->alias.rbegin()->c_str();
		return r;
	}
	return "";
}
const char* font_rctx::get_family_alias(int idx)
{
	static std::string r;
	r.clear();
	if (idx >= 0 && idx < fyvs.size())
	{
		auto v = fyvs[idx];
		for (int i = 0; i < v->alias.size(); i++)
		{
			r += v->alias.rbegin()->c_str(); r.push_back(';');
		}
	}
	return r.c_str();
}

const char* font_rctx::get_family_full(int idx)
{
	if (idx >= 0 && idx < fyvs.size())
	{
		return fyvs[idx]->fullname.c_str();
	}
	return "";
}

const char* font_rctx::get_family_style(int idx, int stidx)
{
	std::string* r = 0;
	size_t st = stidx;
	if (idx >= 0 && idx < fyvs.size())
	{
		auto& v = *fyvs[idx];
		if (v.style.size())
		{
			if (st > v.style.size())
				st = 0;
			return v.style[st].c_str();
		}
	}
	return "";
	//for (auto& [k, v] : fyv) {
	//	if (idx == 0)
	//	{
	//		if (v.style.size())
	//		{
	//			if (st > v.style.size())
	//				st = 0;
	//			r = &v.style[st];
	//		}
	//		break;
	//	}
	//	idx--;
	//}
	//return r ? r->c_str() : nullptr;
}
font_t* font_rctx::get_mk(fontns& v, size_t st)
{
	if (v.vptr.empty())
	{
		v.vptr.resize(v.style.size());
	}
	if (v.style.empty())
	{
		return 0;
	}
	auto r = (font_t*)v.vptr[st];
	if (!r)
	{
		auto vn = v.family;
		auto stn = v.style[st];
		auto pv = imp->add_font_file(v.fpath[st], 0);
		for (auto it : pv)
		{
			bool hasa = v.alias.find(it->_name) != v.alias.end();
			if ((hasa || vn == it->_name || v.fullname == it->fullname) && it->_style.find(stn) != std::string::npos)
			{
				r = it;
				r->bc_ctx = &bcc;
				v.vptr[st] = it;
				current = it;
			}
		}
		if (!r && pv.size())//&& pv[0]->_style == "Regular" && stn == "Normal") 
		{
			r = pv[0];
			r->bc_ctx = &bcc;
			v.vptr[st] = r;
			current = r;
		}
	}
	return r;
}
font_t* font_rctx::get_font(int idx, int styleidx)
{
	font_t* r = 0;
	size_t st = styleidx;
	if (idx >= 0 && idx < fyvs.size())
	{
		auto& v = *fyvs[idx];
		if (v.style.size())
		{
			if (st > v.style.size())
				st = 0;
		}
		r = get_mk(v, st);
	}
	return r;
	//for (auto& [k, v] : fyv) {
	//	if (idx == 0)
	//	{
	//		if (v.style.size())
	//		{
	//			if (st > v.style.size())
	//				st = 0;
	//		}
	//		r = get_mk(v, st);
	//		break;
	//	}
	//	idx--;
	//}
	//return r;
}

font_t* font_rctx::get_font(const char* family, const char* style)
{
	font_t* r = 0;
	size_t st = 0;
	{
		auto ft = fyv.find(family);
		if (ft != fyv.end())
		{
			do {
				r = get_mk(ft->second, st);
				if (!style || (!*style))break;
				for (size_t i = 1; i < ft->second.style.size(); i++)
				{
					if (style == ft->second.style[i])
					{
						r = get_mk(ft->second, i);
						break;
					}
				}
			} while (0);
		}
	}
	if (!r)
	{
		for (auto& [k, v] : fyv) {
			auto ats = v.alias.find(family);
			if (k.find(family) != std::string::npos || v.fullname == family || ats != v.alias.end())
			{
				r = get_mk(v, st);
				if (!style || (!*style))break;
				for (size_t i = 1; i < v.style.size(); i++)
				{
					if (style == v.style[i])
					{
						r = get_mk(v, i);
						break;
					}
				}
				break;
			}
		}
	}
	return r;
}

font_t* font_rctx::get_font_cur()
{
	return current;
}

void font_rctx::set_script(int scriptCode, const char* family)
{
	script_family[scriptCode] = family;
}

void font_rctx::set_script_cn(const char* family)
{
	script_family[HB_SCRIPT_HAN] = family;
}

void font_rctx::set_script_en(const char* family)
{
	script_family[HB_SCRIPT_LATIN] = family;
}

// hb_script_t
const char* font_rctx::select_font_by_script(int scriptCode)
{
	auto h = hb_script_t(scriptCode);
	auto it = script_family.find(scriptCode);
	if (it != script_family.end())
	{
		return it->second.c_str();
	}
	auto default_font = "Microsoft YaHei";
	return default_font;
}
font_t* font_rctx::get_mfont(const std::string& name) {
	auto it = fzv.find(name);
	return it != fzv.end() ? it->second : nullptr;
}
std::vector<font_t*> font_rctx::add2file(const std::string& fn, std::vector<std::string>* pname)
{
	auto r = imp->add_font_file(fn, pname);
	for (auto it : r)
	{
		it->bc_ctx = &bcc;
		auto& oldp = fzv[it->_name];
		if (oldp)
		{
			imp->free_ft(oldp->_name);
		}
		oldp = it;
	}
	return r;
}
std::vector<font_t*> font_rctx::add2mem(const char* data, size_t len, bool iscp, std::vector<std::string>* pname)
{
	auto r = imp->add_font_mem(data, len, true, pname, 0);
	for (auto it : r)
	{
		it->bc_ctx = &bcc;
		auto& oldp = fzv[it->_name];
		if (oldp)
		{
			imp->free_ft(oldp->_name);
		}
		oldp = it;
	}
	return r;
}
void font_rctx::free_font(const std::string& name)
{
	for (auto& [k, v] : fyv) {
		auto ats = v.alias.find(name);
		if (k == name || v.fullname == name || ats != v.alias.end())
		{
			v.vptr;
			for (auto it : v.vptr) {
				imp->free_ftp((font_t*)it);
			}
			v.vptr.clear();
			auto& v1 = fyvs;
			for (auto it = v1.begin(); it != v1.end(); it++)
			{
				if (*it == &v)
				{
					v1.erase(it); break;
				}
			}
			break;
		}
	}
}


font_rctx* new_fonts_ctx()
{
	return new font_rctx();
}
void free_fonts_ctx(font_rctx* p)
{
	if (p)delete p;
}





internal_text_cx::internal_text_cx()
{
}

internal_text_cx::~internal_text_cx()
{
}



#endif
// !NO_FONT_CX

#ifndef NO_HB_CX


// todo icu 
#if 1
struct icu_lib_t
{
	void (*_ubidi_setPara)(UBiDi* pBiDi, const UChar* text, int32_t length, UBiDiLevel paraLevel, UBiDiLevel* embeddingLevels, UErrorCode* pErrorCode);
	int32_t(*_ubidi_countRuns) (UBiDi* pBiDi, UErrorCode* pErrorCode);
	UBiDi* (*_ubidi_open)(void);
	void (*_ubidi_close)(UBiDi* pBiDi);
	UBiDiDirection(*_ubidi_getVisualRun)(UBiDi* pBiDi, int32_t runIndex, int32_t* pLogicalStart, int32_t* pLength);
	void (*_ubidi_setClassCallback)(UBiDi* pBiDi, UBiDiClassCallback* newFn,
		const void* newContext, UBiDiClassCallback** oldFn,
		const void** oldContext, UErrorCode* pErrorCode);
	void (*_ubidi_getClassCallback)(UBiDi* pBiDi, UBiDiClassCallback** fn, const void** context);


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

	void* _handle;
};

static std::vector<std::string> icu_funstr = { "ubidi_setPara","ubidi_countRuns","ubidi_open","ubidi_close","ubidi_getVisualRun",
"ubidi_setClassCallback","ubidi_getClassCallback"
,"ucnv_convert" , "uscript_getScript","uscript_hasScript","uscript_getName","uscript_getShortName",
"ucsdet_open",
"ucsdet_close",
"ucsdet_setText",
"ucsdet_setDeclaredEncoding",
"ucsdet_detect",
"ucsdet_getName",
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
#if 0
#ifdef _WIN32
			dlln = "icuuc" + std::to_string(v1);
			dlln0 = "icudt" + std::to_string(v1);
			auto so0 = loadso(dlln0);
			auto so = loadso(dlln);
			if (!so0)
			{
				dlln0 = "icu";
				so0 = loadso(dlln0);
			}
			if (!so)
			{
				dlln = "icuuc";
				so = loadso(dlln);
			}
#else
			auto so = loadso(dlln);
#endif // _WIN32
#endif
			auto so = loadso("icu");
			if (!so) {
				so = loadso("libicu");
				if (!so)
					throw std::runtime_error("-1");
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
			if (n.size())
			{
				for (auto& it : icu_funstr)
				{
					it += "_" + n;
				}
			}
			icu_lib_t tb = {};
			so->dlsyms(icu_funstr, (void**)&tb);
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



#endif

struct ScriptRecord
{
	UChar32 startChar;
	UChar32 endChar;
	UScriptCode scriptCode;
	font_t* font;
};

struct ParenStackEntry
{
	int32_t pairIndex;
	UScriptCode scriptCode;
};

class ScriptRun /*: public UObject*/ {
public:
	ScriptRun();
	~ScriptRun();

	ScriptRun(const UChar chars[], int32_t length);

	ScriptRun(const UChar chars[], int32_t start, int32_t length);

	void reset();

	void reset(int32_t start, int32_t count);

	void reset(const UChar chars[], int32_t start, int32_t length);

	int32_t getScriptStart();

	int32_t getScriptEnd();

	UScriptCode getScriptCode();

	UBool next();

	/**
	 * ICU "poor man's RTTI", returns a UClassID for the actual class.
	 *
	 * @stable ICU 2.2
	 */
	virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

	/**
	 * ICU "poor man's RTTI", returns a UClassID for this class.
	 *
	 * @stable ICU 2.2
	 */
	static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

	icu_lib_t* icub = 0;
	font_family_t* family = 0;
private:

	static UBool sameScript(int32_t scriptOne, int32_t scriptTwo);

	int32_t charStart;
	int32_t charLimit;
	const UChar* charArray;

	int32_t scriptStart;
	int32_t scriptEnd;
	UScriptCode scriptCode;
	font_t* font = 0;
	ParenStackEntry parenStack[128];
	int32_t parenSP;

	static int8_t highBit(int32_t value);
	static int32_t getPairIndex(UChar32 ch);

	static UChar32 pairedChars[];
	static const int32_t pairedCharCount;
	static const int32_t pairedCharPower;
	static const int32_t pairedCharExtra;

	/**
	 * The address of this static class variable serves as this class's ID
	 * for ICU "poor man's RTTI".
	 */
	static const char fgClassID;
};

const char ScriptRun::fgClassID = 0;

UChar32 ScriptRun::pairedChars[] = {
	0x0028, 0x0029, // ascii paired punctuation
	0x003c, 0x003e,
	0x005b, 0x005d,
	0x007b, 0x007d,
	0x00ab, 0x00bb, // guillemets
	0x2018, 0x2019, // general punctuation
	0x201c, 0x201d,
	0x2039, 0x203a,
	0x3008, 0x3009, // chinese paired punctuation
	0x300a, 0x300b,
	0x300c, 0x300d,
	0x300e, 0x300f,
	0x3010, 0x3011,
	0x3014, 0x3015,
	0x3016, 0x3017,
	0x3018, 0x3019,
	0x301a, 0x301b
};
#ifndef UPRV_LENGTHOF
#define UPRV_LENGTHOF(a) (int32_t)(sizeof(a)/sizeof((a)[0]))
#endif
const int32_t ScriptRun::pairedCharCount = UPRV_LENGTHOF(pairedChars);
const int32_t ScriptRun::pairedCharPower = 1 << highBit(pairedCharCount);
const int32_t ScriptRun::pairedCharExtra = pairedCharCount - pairedCharPower;

int8_t ScriptRun::highBit(int32_t value)
{
	if (value <= 0) {
		return -32;
	}

	int8_t bit = 0;

	if (value >= 1 << 16) {
		value >>= 16;
		bit += 16;
	}

	if (value >= 1 << 8) {
		value >>= 8;
		bit += 8;
	}

	if (value >= 1 << 4) {
		value >>= 4;
		bit += 4;
	}

	if (value >= 1 << 2) {
		value >>= 2;
		bit += 2;
	}

	if (value >= 1 << 1) {
		value >>= 1;
		bit += 1;
	}

	return bit;
}

int32_t ScriptRun::getPairIndex(UChar32 ch)
{
	int32_t probe = pairedCharPower;
	int32_t index = 0;

	if (ch >= pairedChars[pairedCharExtra]) {
		index = pairedCharExtra;
	}

	while (probe > (1 << 0)) {
		probe >>= 1;

		if (ch >= pairedChars[index + probe]) {
			index += probe;
		}
	}

	if (pairedChars[index] != ch) {
		index = -1;
	}

	return index;
}

UBool ScriptRun::sameScript(int32_t scriptOne, int32_t scriptTwo)
{
	return scriptOne <= USCRIPT_INHERITED || scriptTwo <= USCRIPT_INHERITED || scriptOne == scriptTwo;
}

UBool ScriptRun::next()
{

	int32_t startSP = parenSP;  // used to find the first new open character
	UErrorCode error = U_ZERO_ERROR;

	// if we've fallen off the end of the text, we're done
	if (scriptEnd >= charLimit || !icub || !icub->_uscript_getScript || !icub->_uscript_hasScript) {
		return false;
	}

	scriptCode = USCRIPT_COMMON;

	for (scriptStart = scriptEnd; scriptEnd < charLimit; scriptEnd += 1) {
		UChar   high = charArray[scriptEnd];
		UChar32 ch = high;

		// if the character is a high surrogate and it's not the last one
		// in the text, see if it's followed by a low surrogate
		if (high >= 0xD800 && high <= 0xDBFF && scriptEnd < charLimit - 1)
		{
			UChar low = charArray[scriptEnd + 1];

			// if it is followed by a low surrogate,
			// consume it and form the full character
			if (low >= 0xDC00 && low <= 0xDFFF) {
				ch = (high - 0xD800) * 0x0400 + low - 0xDC00 + 0x10000;
				scriptEnd += 1;
			}
		}
		//uscript_getCode();
		UScriptCode sc = icub->_uscript_getScript(ch, &error);
		if (sc == USCRIPT_ARABIC)
		{
			if (icub->_uscript_hasScript(ch, USCRIPT_UGARITIC))
			{
				sc = USCRIPT_UGARITIC;
			}
		}
		int32_t pairIndex = getPairIndex(ch);

		// Paired character handling:
		//
		// if it's an open character, push it onto the stack.
		// if it's a close character, find the matching open on the
		// stack, and use that script code. Any non-matching open
		// characters above it on the stack will be poped.
		if (pairIndex >= 0) {
			if ((pairIndex & 1) == 0) {
				parenStack[++parenSP].pairIndex = pairIndex;
				parenStack[parenSP].scriptCode = scriptCode;
			}
			else if (parenSP >= 0) {
				int32_t pi = pairIndex & ~1;

				while (parenSP >= 0 && parenStack[parenSP].pairIndex != pi) {
					parenSP -= 1;
				}

				if (parenSP < startSP) {
					startSP = parenSP;
				}

				if (parenSP >= 0) {
					sc = parenStack[parenSP].scriptCode;
				}
			}
		}

		if (sameScript(scriptCode, sc)) {
			if (scriptCode <= USCRIPT_INHERITED && sc > USCRIPT_INHERITED) {
				scriptCode = sc;

				// now that we have a final script code, fix any open
				// characters we pushed before we knew the script code.
				while (startSP < parenSP) {
					parenStack[++startSP].scriptCode = scriptCode;
				}
			}

			// if this character is a close paired character,
			// pop it from the stack
			if (pairIndex >= 0 && (pairIndex & 1) != 0 && parenSP >= 0) {
				parenSP -= 1;
				startSP -= 1;
			}
		}
		else {
			// if the run broke on a surrogate pair,
			// end it before the high surrogate
			if (ch >= 0x10000) {
				scriptEnd -= 1;
			}

			break;
		}
	}

	return true;
}

ScriptRun::ScriptRun()
{
	reset(NULL, 0, 0);
}
ScriptRun::~ScriptRun()
{
}

ScriptRun::ScriptRun(const UChar chars[], int32_t length)
{
	reset(chars, 0, length);
}

ScriptRun::ScriptRun(const UChar chars[], int32_t start, int32_t length)
{
	reset(chars, start, length);
}

int32_t ScriptRun::getScriptStart()
{
	return scriptStart;
}

int32_t ScriptRun::getScriptEnd()
{
	return scriptEnd;
}

UScriptCode ScriptRun::getScriptCode()
{
	return scriptCode;
}

void ScriptRun::reset()
{
	scriptStart = charStart;
	scriptEnd = charStart;
	scriptCode = USCRIPT_INVALID_CODE;
	parenSP = -1;
}

void ScriptRun::reset(int32_t start, int32_t length)
{
	charStart = start;
	charLimit = start + length;

	reset();
}

void ScriptRun::reset(const UChar chars[], int32_t start, int32_t length)
{
	charArray = chars;

	reset(start, length);
}

//UObject::UObject(){}
//UObject::~UObject(){}

static hb_script_t get_script(const char* str)
{
#define KN(v,v1) {#v1,#v},{#v,#v}
	static std::map<std::string, std::string> ms =
	{
		KN(Adlm, Adlam),
		KN(Aghb, Caucasian_Albanian),
		KN(Ahom, Ahom),
		KN(Arab, Arabic),
		KN(Armi, Imperial_Aramaic),
		KN(Armn, Armenian),
		KN(Avst, Avestan),
		KN(Bali, Balinese),
		KN(Bamu, Bamum),
		KN(Bass, Bassa_Vah),
		KN(Batk, Batak),
		KN(Beng, Bengali),
		KN(Bhks, Bhaiksuki),
		KN(Bopo, Bopomofo),
		KN(Brah, Brahmi),
		KN(Brai, Braille),
		KN(Bugi, Buginese),
		KN(Buhd, Buhid),
		KN(Cakm, Chakma),
		KN(Cans, Canadian_Aboriginal),
		KN(Cari, Carian),
		KN(Cham, Cham),
		KN(Cher, Cherokee),
		KN(Chrs, Chorasmian),
		KN(Copt, Coptic),
		KN(Cprt, Cypriot),
		KN(Cyrl, Cyrillic),
		KN(Deva, Devanagari),
		KN(Diak, Dives_Akuru),
		KN(Dogr, Dogra),
		KN(Dsrt, Deseret),
		KN(Dupl, Duployan),
		KN(Egyp, Egyptian_Hieroglyphs),
		KN(Elba, Elbasan),
		KN(Elym, Elymaic),
		KN(Ethi, Ethiopic),
		KN(Geor, Georgian),
		KN(Glag, Glagolitic),
		KN(Gong, Gunjala_Gondi),
		KN(Gonm, Masaram_Gondi),
		KN(Goth, Gothic),
		KN(Gran, Grantha),
		KN(Grek, Greek),
		KN(Gujr, Gujarati),
		KN(Guru, Gurmukhi),
		KN(Hang, Hangul),
		KN(Hani, Han),
		KN(Hani, Hans),
		KN(Hani, Hant),
		KN(Hano, Hanunoo),
		KN(Hatr, Hatran),
		KN(Hebr, Hebrew),
		KN(Hira, Hiragana),
		KN(Hluw, Anatolian_Hieroglyphs),
		KN(Hmng, Pahawh_Hmong),
		KN(Hmnp, Nyiakeng_Puachue_Hmong),
		KN(Hrkt, Katakana_Or_Hiragana),
		KN(Hung, Old_Hungarian),
		KN(Ital, Old_Italic),
		KN(Java, Javanese),
		KN(Kali, Kayah_Li),
		KN(Kana, Katakana),
		KN(Khar, Kharoshthi),
		KN(Khmr, Khmer),
		KN(Khoj, Khojki),
		KN(Kits, Khitan_Small_Script),
		KN(Knda, Kannada),
		KN(Kthi, Kaithi),
		KN(Lana, Tai_Tham),
		KN(Laoo, Lao),
		KN(Latn, Latin),
		KN(Lepc, Lepcha),
		KN(Limb, Limbu),
		KN(Lina, Linear_A),
		KN(Linb, Linear_B),
		KN(Lisu, Lisu),
		KN(Lyci, Lycian),
		KN(Lydi, Lydian),
		KN(Mahj, Mahajani),
		KN(Maka, Makasar),
		KN(Mand, Mandaic),
		KN(Mani, Manichaean),
		KN(Marc, Marchen),
		KN(Medf, Medefaidrin),
		KN(Mend, Mende_Kikakui),
		KN(Merc, Meroitic_Cursive),
		KN(Mero, Meroitic_Hieroglyphs),
		KN(Mlym, Malayalam),
		KN(Modi, Modi),
		KN(Mong, Mongolian),
		KN(Mroo, Mro),
		KN(Mtei, Meetei_Mayek),
		KN(Mult, Multani),
		KN(Mymr, Myanmar),
		KN(Nand, Nandinagari),
		KN(Narb, Old_North_Arabian),
		KN(Nbat, Nabataean),
		KN(Newa, Newa),
		KN(Nkoo, Nko),
		KN(Nshu, Nushu),
		KN(Ogam, Ogham),
		KN(Olck, Ol_Chiki),
		KN(Orkh, Old_Turkic),
		KN(Orya, Oriya),
		KN(Osge, Osage),
		KN(Osma, Osmanya),
		KN(Palm, Palmyrene),
		KN(Pauc, Pau_Cin_Hau),
		KN(Perm, Old_Permic),
		KN(Phag, Phags_Pa),
		KN(Phli, Inscriptional_Pahlavi),
		KN(Phlp, Psalter_Pahlavi),
		KN(Phnx, Phoenician),
		KN(Plrd, Miao),
		KN(Prti, Inscriptional_Parthian),
		KN(Rjng, Rejang),
		KN(Rohg, Hanifi_Rohingya),
		KN(Runr, Runic),
		KN(Samr, Samaritan),
		KN(Sarb, Old_South_Arabian),
		KN(Saur, Saurashtra),
		KN(Sgnw, SignWriting),
		KN(Shaw, Shavian),
		KN(Shrd, Sharada),
		KN(Sidd, Siddham),
		KN(Sind, Khudawadi),
		KN(Sinh, Sinhala),
		KN(Sogd, Sogdian),
		KN(Sogo, Old_Sogdian),
		KN(Sora, Sora_Sompeng),
		KN(Soyo, Soyombo),
		KN(Sund, Sundanese),
		KN(Sylo, Syloti_Nagri),
		KN(Syrc, Syriac),
		KN(Tagb, Tagbanwa),
		KN(Takr, Takri),
		KN(Tale, Tai_Le),
		KN(Talu, New_Tai_Lue),
		KN(Taml, Tamil),
		KN(Tang, Tangut),
		KN(Tavt, Tai_Viet),
		KN(Telu, Telugu),
		KN(Tfng, Tifinagh),
		KN(Tglg, Tagalog),
		KN(Thaa, Thaana),
		KN(Thai, Thai),
		KN(Tibt, Tibetan),
		KN(Tirh, Tirhuta),
		KN(Ugar, Ugaritic),
		KN(Vaii, Vai),
		KN(Wara, Warang_Citi),
		KN(Wcho, Wancho),
		KN(Xpeo, Old_Persian),
		KN(Xsux, Cuneiform),
		KN(Yezi, Yezidi),
		KN(Yiii, Yi),
		KN(Zanb, Zanabazar_Square),
		KN(Zinh, Inherited),
		KN(Zyyy, Common),
		KN(Zzzz, Unknonn)
	};
#undef KN
	hb_script_t ret = HB_SCRIPT_UNKNOWN;
	auto it = ms.find(str);
	if (it != ms.end())
	{
		ret = hb_script_from_string(it->second.c_str(), 4);
	}
	return ret;
}

struct char_item_t
{
	unsigned int glyph_index;	// 索引
	glm::ivec2 offset;			// 偏移
	glm::ivec2 adv;				// xy_advance
	glm::ivec2 size;			// 原始大小
};
struct str_info_t
{
	union str_t
	{
		const uint8_t* p8;
		const uint16_t* p16;
		const uint32_t* p32 = 0;
	}str;
	unsigned int	type = 0;
	int             text_length = -1;
	unsigned int    item_offset = 0;
	int             item_length = -1;
	str_info_t() {}
	str_info_t(const char* s, int tlen = -1, unsigned int pos = 0, int ilen = -1) :text_length(tlen), item_offset(pos), item_length(ilen)
	{
		str.p8 = (uint8_t*)s;
	}
	str_info_t(const uint16_t* s, int tlen = -1, unsigned int pos = 0, int ilen = -1) :type(1), text_length(tlen), item_offset(pos), item_length(ilen)
	{
		str.p16 = (uint16_t*)s;
	}
	str_info_t(const uint32_t* s, int tlen = -1, unsigned int pos = 0, int ilen = -1) :type(2), text_length(tlen), item_offset(pos), item_length(ilen)
	{
		str.p32 = (uint32_t*)s;
	}
};
enum class lt_dir
{
	invalid = 0,
	ltr = 4,	// 左-右
	rtl,		// 右-左
	ttb,		// 竖
	btt			// 倒竖
};
class word_key
{
private:
	// hb_script_t
	std::set<int> _sc;
	std::set<char> _split_ch;
public:
	word_key();
	~word_key();
	void push(int sc);
	// hb_script_t
	bool is_word(int sc);
	int get_split(char c);
private:

};
struct lt_item_t
{
	unsigned int _glyph_index = 0;
	// 缓存位置xy, 字符图像zw(width,height)
	glm::ivec4 _rect;
	// 渲染偏移坐标
	glm::ivec2 _dwpos;
	// 原始的advance
	int advance = 0;
	// hb偏移
	glm::ivec2 _offset;
	// hb计算的
	glm::ivec2 adv;
	// 字符
	char32_t ch[8] = {};
	int chlen = 0;
	uint32_t cluster = 0;
	// 连词
	int last_n = 0;
	// 渲染颜色
	uint32_t color = 0;
	image_ptr_t* _image = nullptr;
	bool rtl = false;
};

class hb_cx
{
public:
	enum class hb_dir
	{
		invalid = 0,
		ltr = 4,	// 左-右
		rtl,		// 右-左
		ttb,		// 竖
		btt			// 倒竖
	};
	std::map<font_t*, hb_font_t*> _font;
	hb_buffer_t* _buffer = 0;
public:
	hb_cx();
	~hb_cx();
	font_t* push_font(font_t* p);
	void shape(const str_info_t& str, font_t* ttp, hb_dir dir);
	void shape(const str_info_t* str, font_t* ttp, hb_dir dir);
	//glm::ivec2 draw_to_image(const std::string& str, font_t* ttf, double fontheight, image_ptr_t* img, glm::ivec3 dstpos);
public:

private:
	void free_font();
};

// 文本布局
class layout_text
{
public:
	enum class lt_dir
	{
		invalid = 0,
		ltr = 4,	// 左-右
		rtl,		// 右-左
		ttb,		// 竖
		btt			// 倒竖
	};
	struct dlinfo_t
	{
		font_t* font;
		double font_height;
		lt_dir dir;
		//Image* img;
		// x,y ,z=baseline
		//glm::ivec3 pos;
		bool is_word1;
		word_key* wks = 0;
		// out
		glm::ivec2 rc;
	};
	std::vector<font_t*> _familys;
private:
	hb_cx* _hb = 0;
	//auto_destroy_cx dc;
	word_key wks;
public:
	layout_text();
	~layout_text();
	// 设置字体集合
	void set_font_family(font_t** font_family, int count);
	void cpy(layout_text* p);
	void get_familys(font_t** bufptr, int first, int count);
	font_t* get_familys(const std::string& name, const std::string& style);
	size_t size();
	//double get_base_line(double height); 
	//void do_text(item_box_info* info);
	// familys要在set_font_family里先设置
	//void do_text(item_box_info* info, hz::familys_t* familys);

private:
	void do_layout(const std::string& u8str, dlinfo_t* info, std::vector<lt_item_t>& outlt);
	int tolayout(const std::string& str, dlinfo_t* info, std::vector<lt_item_t>& outlt);
	//void make_extent(item_box_info& info);

};
/*
		layout_text lt;
	std::vector<font_t*> familys = { SourceHanSans,nsimsun,Consolas, Majalla, Uighur,  seguiemj, utsaah };
	lt.set_font_family(familys.data(), familys.size());	// 设置字体集

	item_box_info res;
	res.s.str = str.c_str();
	res.s.count = get_utf8_count(res.s.str, str.size());
	res.s.first = 0;
	res.fontheight = 16;
	hz::text_extent str_rc;
	// 设置视图宽度
	str_rc.set_view_width(520);
	{
		print_time ftpt("do_text");
		lt.do_text(res);
	}
	{
		print_time ftpt("layout computing");
		lt.make_extent(res, &str_rc);
	}


*/
layout_text::layout_text()
{
	_hb = new hb_cx();
}

layout_text::~layout_text()
{
	delete _hb;
}
void layout_text::set_font_family(font_t** font_family, int count)
{
	assert(_hb);
	if (font_family)
	{
		for (int i = 0; i < count; i++)
		{
			auto it = font_family[i];
			if (it)
			{
				auto p = _hb->push_font(it);
				if (p)
					_familys.push_back(p);
			}
		}
	}
}
void layout_text::cpy(layout_text* p)
{
	set_font_family(p->_familys.data(), p->_familys.size());
}
void layout_text::get_familys(font_t** bufptr, int first, int count)
{
	do
	{
		if (count < 0)
			count = _familys.size() - first;
		if (!bufptr || (first + count > _familys.size()) || first < 0)
			break;
		std::copy_n(_familys.data() + first, count, bufptr);
	} while (0);
}
font_t* layout_text::get_familys(const std::string& name, const std::string& style)
{
	font_t* ret = 0;
	font_t* ret1 = 0;
	for (auto it : _familys)
	{
		if (style.size() && it->_style != style)
		{
			continue;
		}
		if (it->_name == name || it->fullname == name)
		{
			ret = it;
			break;
		}
		if (it->_name.find(name) || it->fullname.find(name))
		{
			ret1 = it;
		}
	}
	return ret;
}
size_t layout_text::size()
{
	return _familys.size();
}
void layout_text::do_layout(const std::string& u8str, dlinfo_t* info, std::vector<lt_item_t>& outlt)
{
	// 整形
	_hb->shape({ u8str.c_str(), (int)u8str.size(), 0, -1 }, info->font, (hb_cx::hb_dir)info->dir);
	// 获取坐标信息
	tolayout(u8str, info, outlt);
	//info->rc = _hb->draw_to_image(u8str, info->font, info->font_height, info->img, info->pos);
}

int layout_text::tolayout(const std::string& str, dlinfo_t* dinfo, std::vector<lt_item_t>& outlt)
{
	hb_buffer_t* buf = _hb->_buffer;
	unsigned int count = hb_buffer_get_length(buf);
	if (count == 0)
	{
		return 0;
	}
	font_t* ttf = dinfo->font;
	double fontheight = dinfo->font_height;
	bool is_word1 = dinfo->is_word1;
	word_key& wks = *dinfo->wks;
	hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buf, nullptr);
	hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buf, nullptr);
	auto scale_h = ttf->get_scale(fontheight);
	auto pstr = str.c_str();
	outlt.reserve(outlt.size() + count);
	size_t bidx = outlt.size();
	size_t n = 0;
	size_t sc = str.size();
	const char* laststr = 0;
	int64_t last_cluster = infos[0].cluster; char32_t cp32;
	bool isn = sc != count;
	std::vector<int64_t> cluster;
	int ic = 1;
	bool isback = (dinfo->dir == lt_dir::ltr) || (dinfo->dir == lt_dir::ttb);
	if (!isback)
		cluster.push_back(sc);
	// 收集cluster判断是否连写
	for (unsigned int i = 0; i < count; i++)
	{
		cluster.push_back(infos[i].cluster);
	}
	if (isback) { cluster.push_back(sc); }
	else {
		ic = 0;
	}

	for (unsigned int i = 0; i < count; i++)
	{
		hb_glyph_info_t* info = &infos[i];
		hb_glyph_position_t* pos = &positions[i];
		glm::vec2 adv = { ceil(pos->x_advance * scale_h), ceil(pos->y_advance * scale_h) };
		glm::vec2 offset = { ceil(pos->x_offset * scale_h), -ceil(pos->y_offset * scale_h) };
		unsigned int cp = 0;
		//laststr = md::get_u8_last(pstr + info->cluster, &cp);
		int ch = 0;
		auto nstr = pstr + info->cluster;
		auto kk = md::utf8_to_unicode(nstr, &ch);
		if (kk < 1)break;
		cp = ch;
		cp32 = cp;
		auto git = ttf->get_glyph_item(info->codepoint, cp, fontheight);

		lt_item_t lti = {};
		//memcpy(&lti, &git, sizeof(ftg_item));
		lti._image = git._image;
		lti.color = git.color;
		lti._glyph_index = git._glyph_index;
		lti._rect = git._rect;
		lti._dwpos = git._dwpos;
		lti.advance = git.advance;
		lti.rtl = !isback;
		lti.adv = adv;
		lti._offset = offset;
		lti.ch[0] = cp;
		lti.chlen = 1;
		if (isn)
		{
			int64_t clu[2] = { info->cluster, cluster[i + ic] };
			if (clu[0] > clu[1]) { std::swap(clu[0], clu[1]); }
			int64_t dif = clu[1];
			if (clu[0] != clu[1])
			{
				int c = 0;
				unsigned int cp1 = 0;
				auto uc = md::get_utf8_count(pstr + clu[0], clu[1] - clu[0]);
				if (uc > 1)
				{
					laststr = pstr + clu[0];
					for (int k = 0; k < uc; k++)
					{
						//laststr = md::get_u8_last(laststr, &cp1);
						auto kk = md::utf8_to_unicode(laststr, (int*)&cp1);
						assert(c < 8 && kk>0);
						if (kk > 0)
						{
							laststr += kk;
							lti.ch[c++] = cp1;
						}
					}
				}
				lti.chlen = uc;
			}
			if (dif == 1)
			{
				cp32 = 0;
			}
		}
		bool isctr = false;
		{

			static std::string ctn = R"(~!@#$%^&*()-+/\|/;'',.<>?`)";
			static std::u16string cn = uR"(【】、；‘：“’”，。、《》？（）——！￥)";
			if (cn.find(cp) != std::u16string::npos)
				isctr = true;
			if (cp < 255 && ctn.find(cp) != std::string::npos)
				isctr = false;
			if (cp < 255 && iscntrl(cp))
			{
				do
				{
					isctr = true;
				} while (0);
				if (cp != '\t')
					lti.adv.x = 0;
			}
		}
		lti.last_n = 0;
		lti.cluster = info->cluster;
		bool issplit = (cp < 255 && wks.get_split(cp));
		bool ispush = (is_word1 && cp > 255) || count - i == 1;
		last_cluster = info->cluster;
		if (issplit || isctr)
		{
			if (n > 0)
			{
				outlt[bidx].last_n = n;
				bidx += n; n = 0;
			}
			ispush = true;
		}
		n++;
		outlt.push_back(lti);
		// 分割
		if (ispush)
		{
			outlt[bidx].last_n = n;
			bidx += n; n = 0;
		}
	}
	return count;
}




struct BidiScriptRunRecords {
	bool isRtl;
	std::deque<ScriptRecord> records;
};
static void collectBidiScriptRuns(BidiScriptRunRecords& scriptRunRecords, const UChar* chars, int32_t start, int32_t end, bool isRTL, font_family_t* family)
{
	scriptRunRecords.isRtl = isRTL;

	ScriptRun scriptRun(chars, start, end);
	scriptRun.icub = get_icu(U_ICU_VERSION_MAJOR_NUM);
	while (scriptRun.next()) {
		ScriptRecord scriptRecord;
		scriptRecord.startChar = scriptRun.getScriptStart();
		scriptRecord.endChar = scriptRun.getScriptEnd();
		scriptRecord.scriptCode = scriptRun.getScriptCode();

		scriptRunRecords.records.push_back(scriptRecord);
	}
}

word_key::word_key()
{
	_sc.insert(HB_SCRIPT_HAN);
	_split_ch.insert(' ');
	_split_ch.insert('\t');
}

word_key::~word_key()
{
}
void word_key::push(int sc)
{
	_sc.insert(sc);
}
bool word_key::is_word(int sc)
{
	return _sc.find(sc) != _sc.end();
}
int word_key::get_split(char c)
{
	return _split_ch.find(c) != _split_ch.end();
}

// todo hb
#if 1
static int icn = 0;
hb_cx::hb_cx()
{
	_buffer = hb_buffer_create();
	icn++;
}

hb_cx::~hb_cx()
{
	free_font();
	icn--;
	auto kuf = hb_buffer_get_unicode_funcs(_buffer);
	hb_buffer_destroy(_buffer);
	if (icn == 0)
	{
		hb_unicode_funcs_destroy(kuf);
	}
}

font_t* hb_cx::push_font(font_t* p)
{
	font_t* ret = 0;
	if (!p || _font.find(p) != _font.end())
	{
		return 0;
	}
	hb_blob_t* blob = hb_blob_create((char*)p->font->data, p->dataSize, HB_MEMORY_MODE_READONLY, 0, 0); //HB_MEMORY_MODE_WRITABLEhb_blob_create_from_file(ttfn);
	if (hb_blob_get_length(blob) > 0)
	{
		hb_face_t* face = hb_face_create(blob, p->_index);
		hb_font_t* hbf = hb_font_create(face);
		if (face && hbf)
		{
			//unsigned int upem = hb_face_get_upem(face);
			//hb_font_set_scale(hbf, upem, upem);
			//hb_font_set_ppem(hbf, ppem, ppem);
			_font[p] = hbf;
			ret = p;
		}
	}
	if (blob)
		hb_blob_destroy(blob);
	return ret;
}
void hb_cx::shape(const str_info_t& s, font_t* ttp, hb_dir dir)
{
	shape(&s, ttp, dir);
}
void hb_cx::shape(const str_info_t* str, font_t* ttp, hb_dir dir)
{
	if (_font.find(ttp) == _font.end())
	{
		return;
	}
	auto font = _font[ttp];
	if (font)
	{
		hb_buffer_t* buf = _buffer;
		hb_buffer_clear_contents(buf);
		hb_buffer_set_direction(buf, (hb_direction_t)dir);
		typedef void(*bufadd_func)(hb_buffer_t* buffer, const void* text, int text_length, unsigned int item_offset, int item_length);
		static bufadd_func cb[3] = { (bufadd_func)hb_buffer_add_utf8, (bufadd_func)hb_buffer_add_utf16, (bufadd_func)hb_buffer_add_utf32 };
		if (str->type < 3)
		{
			cb[str->type](buf, str->str.p8, str->text_length, str->item_offset, str->item_length);
			hb_buffer_guess_segment_properties(buf);
			hb_shape(font, buf, nullptr, 0);
		}

	}
}

#if 0
glm::ivec2 hb_cx::draw_to_image(const std::string& str, font_t* ttf, double fontheight, hz::Image* img, glm::ivec3 dstpos)
{
	hb_buffer_t* buf = _buffer;
	unsigned int count = hb_buffer_get_length(buf);
	hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buf, nullptr);
	hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buf, nullptr);
	auto scale_h = ttf->get_scale_height(fontheight);

	auto bl = ttf->get_base_line(fontheight);
	double base_line = dstpos.z < 0 ? bl : dstpos.z;			// 基线
	//ttf ->get_VMetrics(&finfo[0], &finfo[1], &finfo[2]);
	int x = dstpos.x, y = dstpos.y;
	auto dir = hb_buffer_get_direction(buf);
	glm::ivec2 ret;
	glm::ivec2 dpos;
	auto u8p = str.c_str();
	//printf("%f\n", scale_h);
	for (unsigned int i = 0; i < count; i++)
	{
		hb_glyph_info_t* info = &infos[i];
		hb_glyph_position_t* pos = &positions[i];
		int adv = pos->x_advance * scale_h;
		int advy = pos->y_advance * scale_h;
		//printf("cluster %d	glyph 0x%x at	(%d,%d)+(%d,%d) %d\n",
		//	info->cluster,
		//	info->codepoint,
		//	pos->x_offset,
		//	pos->y_offset,
		//	pos->x_advance,
		//	pos->y_advance, adv);

		dpos.x = x + (pos->x_offset * scale_h);
		dpos.y = y - (pos->y_offset * scale_h) + base_line;
		unsigned int cp = 0;
		u8p = md::get_u8_last(u8p, &cp);
		//auto git = ttf->mk_glyph(info->codepoint, cp, fontheight);
		glm::vec2 size = {};// md::mk_fontbitmap(ttf, info->codepoint, cp, fontheight, dpos, -1, img);
		//printf("%d\t", dpos.x);
		x += adv;
		//x += size.z;
		if (dir == HB_DIRECTION_TTB && abs(advy) < size.y)
			advy = -size.y;
		y -= advy;
		ret.x += adv;
		if (size.y > ret.y)
		{
			ret.y = size.y;
		}
	}
	//printf("\n");
	return ret;
}
#endif
void hb_cx::free_font()
{
	for (auto& [k, v] : _font)
	{
		auto face = hb_font_get_face(v);
		hb_face_destroy(face);
		hb_font_destroy(v);
	}
	//hb_font_funcs_destroy(fcb);
	_font.clear();

}
#endif

UCharDirection bidiccb(const void* context, UChar32 c) {
	return U_LEFT_TO_RIGHT;
}

void do_bidi(UChar* testChars, int len, font_family_t* family, std::vector<bidi_item>& info)
{
	//print_time ftpt("ubidi");

	static auto icub = get_icu(U_ICU_VERSION_MAJOR_NUM);
	do {
		if (!icub)
		{
			icub = get_icu(U_ICU_VERSION_MAJOR_NUM);
		}
		if (!icub)return;
		if (!icub->_ubidi_open)
		{
			return;
		}
	} while (0);


	UBiDi* bidi = icub->_ubidi_open();
	UBiDiLevel bidiReq = UBIDI_DEFAULT_LTR;
	int stringLen = len;
	if (bidi) {
		UErrorCode status = U_ZERO_ERROR;
		// Set callbacks to override bidi classes of new emoji
		//ubidi_setClassCallback(bidi, emojiBidiOverride, nullptr, nullptr, nullptr, &status);
		if (!U_SUCCESS(status)) {
			printf("error setting bidi callback function, status = %d", status);
			return;
		}
		info.reserve(len);
		//icub->_ubidi_setClassCallback(bidi, bidiccb, NULL, NULL, NULL, &status);
		icub->_ubidi_setPara(bidi, testChars, stringLen, bidiReq, NULL, &status);
		if (U_SUCCESS(status)) {
			//int paraDir = ubidi_getParaLevel(bidi);
			size_t rc = icub->_ubidi_countRuns(bidi, &status);
			for (size_t i = 0; i < rc; ++i) {
				int32_t startRun = 0;
				int32_t lengthRun = 0;
				UBiDiDirection runDir = icub->_ubidi_getVisualRun(bidi, i, &startRun, &lengthRun);
				bool isRTL = (runDir == UBIDI_RTL);
				std::string u8strd = md::u16_u8((uint16_t*)(testChars + startRun), lengthRun);
				size_t endRun = startRun + lengthRun;
				info.push_back({ /*sc,scrn,*/ u8strd,  (size_t)(testChars + startRun), endRun, isRTL });
#if 0
				//printf("Processing Bidi Run = %lld -- run-start = %d, run-len = %d, isRTL = %d\n", i, startRun, lengthRun, isRTL);
				BidiScriptRunRecords scriptRunRecords;
				collectBidiScriptRuns(scriptRunRecords, testChars, startRun, lengthRun, isRTL, family);

				//print_time ftpt("ubidi_2");
				while (!scriptRunRecords.records.empty()) {
					ScriptRecord scriptRecord;
					if (scriptRunRecords.isRtl) {
						scriptRecord = scriptRunRecords.records.back();
						scriptRunRecords.records.pop_back();
					}
					else {
						scriptRecord = scriptRunRecords.records.front();
						scriptRunRecords.records.pop_front();
					}

					uint32_t     start = scriptRecord.startChar;
					uint32_t     end = scriptRecord.endChar;
					UScriptCode code = scriptRecord.scriptCode;
					auto scrn = icub->_uscript_getName(code);
					auto sc = get_script(scrn);
					std::string u8strd = md::u16_u8((uint16_t*)(testChars + start), end - start);
					info.push_back({ /*sc,scrn,*/ u8strd, start, end, isRTL });
					//printf("Script '%s' from %d to %d.\t%d\n", scrn, start, end, sc);
				}
#endif
			}
		}
	}
	return;
}


font_family_t* new_font_family(font_rctx* ctx, const char* family, const char* styles)
{
	font_family_t* p = 0;
	do {
		if (!ctx || !family || !(*family))break;
		std::vector<std::string> v, st;
		split_v(family, ",", v);
		if (styles && *styles)
			split_v(styles, ",", st);
		if (v.empty())
		{
			break;
		}
		const char* style = st.size() ? st[0].c_str() : nullptr;
		size_t ix = 1;
		p = new font_family_t();
		*p = {};
		p->familys = new font_p[v.size()];
		p->count = 0;
		auto t = p->familys;
		for (auto& it : v)
		{
			auto font = ctx->get_font(it.c_str(), style);
			if (ix < st.size())
			{
				style = st.size() ? st[ix].c_str() : nullptr;
			}
			ix++;
			if (font)
			{
				*t = font; t++; p->count++;
			}
		}
	} while (0);
	return p;
}

void delete_font_family(font_family_t* p)
{
	if (p) {
		if (p->familys)
		{
			delete[]p->familys;
		}
		delete p;
	}
}


struct value_block {
	union {
		text_block* t;
		image_block* i;
	};
	int type = 0;
};
/*
	text_block_t* t;
	size_t count;
	font_family_t* family;
*/
class text_run_cx :public text_run_t
{
public:
	std::vector<bidi_item> bv;
	std::vector<font_item_t> _tm;
	std::vector<strfont_t> _block;
	std::vector<value_block> _value;

public:
	text_run_cx();
	~text_run_cx();

private:

};

text_run_cx::text_run_cx()
{
}

text_run_cx::~text_run_cx()
{
}
text_bp text_create(int width, int height, bool autobr)
{
	auto p = new text_run_cx();
	text_bp pr = p;
	if (p)
	{
		p->_rect.z = width;
		p->_rect.w = height;
		pr->_private = p;
		p->autobr = autobr;
	}
	return pr;
}

void text_free(text_bp p)
{
	if (p)
	{
		auto p1 = (text_run_cx*)p->_private;
		delete p1;
	}
}
void text_set_show(text_bp p, size_t first_line, int64_t x)
{
	if (!p)return;
	auto p1 = (text_run_cx*)p->_private;
	p->first_line = first_line;
	p->posx = x;
}

void text_set_rect(text_bp p, const glm::ivec4& rc)
{
	if (!p)return;
	auto p1 = (text_run_cx*)p->_private;
	p1->_rect = rc;
}
void text_add(text_bp p, text_block* tb)
{
	if (!p)return;
	auto p1 = (text_run_cx*)p->_private;
	p1->_value.push_back({ .t = tb, .type = 0 });
}

void text_add_image(text_bp p, image_block* img)
{
	if (!p)return;
	auto p1 = (text_run_cx*)p->_private;
	if (!p)return;
	p1->_value.push_back({ .i = img, .type = 1 });
}

void text_clear(text_bp p)
{
	if (!p)return;
	auto p1 = (text_run_cx*)p->_private;
	p1->bv.clear();
	p1->_tm.clear();
	p1->_block.clear();
}

void text_update(text_bp p)
{
	if (!p)return;
	auto p1 = (text_run_cx*)p->_private;
	for (auto& it : p1->_value)
	{
		if (it.type == 0)
		{
			auto tb = it.t;
			//text_update_text(p1, tb);
		}
		else if (it.type == 1)
		{
		}
	}
	return;
}

void text_set_bidi(std::vector<bidi_item>& bv, const char* str, size_t first, size_t count, font_family_t* family)
{
	auto t = md::utf8_char_pos(str, first, count);
	auto te = t + count;// md::utf8_char_pos(t, count, count);
	auto wk = md::u8_u16(t, count);
	const uint16_t* str1 = (const uint16_t*)wk.c_str();
	size_t n = wk.size();
	{
		bv.clear();
		do_bidi((UChar*)str1, n, family, bv);
		std::stable_sort(bv.begin(), bv.end(), [](const bidi_item& bi, const bidi_item& bi1) { return bi.first < bi1.first; });
	}
	return;
}
font_t* get_font(font_family_t* p, const std::string& hs) {
	if (!p)return 0;
	font_t* r = p->familys[0];
	for (size_t i = 0; i < p->count; i++)
	{
		auto font = p->familys[i];
		if (font)
		{
			if (std::string::npos != font->_slng.find(hs)) {
				r = font; break;
			}
		}
	}
	return r;
}

glm::ivec4 font_get_char_extent(char32_t ch, unsigned char font_size, font_family_t* fallbacks, font_t** oft)
{
	ft_char_s cs;
	cs.v.font_dpi = 0;// font_dpi;
	cs.v.font_size = font_size;
	cs.v.unicode_codepoint = ch;
#if 0
	{
		auto it = _char_lut.find(cs.u);
		if (it != _char_lut.end())
		{
			return it->second;
		}
	}
#endif
	glm::ivec4 ret = {};
	font_t* rfont = nullptr;
	auto g = fallbacks->familys[0]->get_glyph_index0(ch, &rfont, fallbacks);
	if (g)
	{
		if (oft)*oft = rfont;
		double fns = font_size;// round((double)font_size * font_dpi / 72.0);
		double scale = rfont->get_scale(fns);
		int x0 = 0, y0 = 0, x1 = 0, y1 = 0, advance, lsb;
		stb_font::buildGlyphBitmap(rfont->font, g, scale, &advance, &lsb, &x0, &y0, &x1, &y1);
		double adv = scale * advance;
		auto bl = rfont->get_base_line(font_size);
		ret = { x1 - x0, y1 - y0, adv, bl };
		//_char_lut[cs.u] = ret;
	}
	return ret;
}
void get_font_fallbacks(font_family_t* p, const void* str8, int len, bool rtl, std::vector<strfont_t>& vstr)
{
	if (!str8 || !len || !p || !p->count)
		return;
	auto str = (const char*)str8;
	auto str0 = (const char*)str8;
	auto font = p->familys[0];
	font_t* oft0 = font;
	font_t* rfont = nullptr;

	do
	{
		if (!str || !(*str)) { break; }
		int ch = 0;
		rfont = nullptr;
		auto kk = md::utf8_to_unicode(str, &ch);
		if (kk < 1)break;
		if (ch == '\n')
		{
			strfont_t t = {};
			if (str0 != str)
			{
				t.v = std::string_view(str0, str);
				t.rtl = rtl;
				t.font = oft0;
				vstr.push_back(t);
			}
			{
				t.v = std::string_view(str, str + kk);
				t.rtl = rtl;
				t.font = 0;
				vstr.push_back(t);
			}
			str0 = str + kk;
			str += kk;
			continue;
		}
		auto g = font->get_glyph_index0(ch, &rfont, p);
		if (ch == 0x200d)
			rfont = oft0;
		if (oft0 != rfont)
		{
			if (rfont && str0 != str) {
				strfont_t t = {};
				t.v = std::string_view(str0, str);
				t.rtl = rtl;
				t.font = oft0;
				str0 = str;
				vstr.push_back(t);
			}
			oft0 = rfont;
		}
		str += kk;
	} while (str && *str);
	if (str0 != str)
	{
		vstr.push_back({ std::string_view(str0,str), oft0 });
	}
	return;
}

void update_text(text_render_o* p, text_block* tb)
{
	if (!p)return;
	p->_vstr.clear();
	p->_block.clear();
	p->bv.clear();
	std::vector<hb_tag_t> vv;
	auto& vstr = p->_block;
	vstr.clear();
	auto t = tb->style;
	text_set_bidi(p->bv, tb->str, tb->first, tb->size, t->family);
	for (auto& it : p->bv)
	{
		get_font_fallbacks(t->family, it.s.c_str(), it.s.size(), it.rtl, vstr);
	}
	for (auto& kt : vstr)
	{
		if (!kt.font)
		{
			continue;
		}
		auto st = kt.v;
		font_t::GlyphPositions gp = {};// 执行harfbuzz
		auto w = md::u8_w(std::string(st));
		auto nn0 = kt.font->CollectGlyphsFromFont(st.data(), st.size(), 8, kt.rtl, 0, &gp);
		kt._tnpos.insert(kt._tnpos.end(), gp.pos, gp.pos + gp.len);
	}
	int dh = tb->line_height;
	int baseline = tb->baseline;
	for (auto& kt : vstr)
	{
		if (!kt.font) {
			font_item_t git = {}; git.cpt = kt.v[0];
			p->_vstr.push_back(git); continue;
		}
		double scale_h = kt.font->get_scale(t->fontsize);
		uint32_t color = -1;
		if (tb->line_height == 0)
		{
			int h = kt.font->get_line_height(t->fontsize);
			dh = std::max(dh, h);
		}
		if (tb->baseline == 0)
		{
			int bl = kt.font->get_base_line(t->fontsize);
			baseline = std::max(baseline, bl);
		}
		// 光栅化glyph index并缓存
		for (auto& gt : kt._tnpos)
		{
			auto pos = &gt;
			auto git = kt.font->get_glyph_item(pos->index, 0, t->fontsize);
			glm::vec2 offset = { ceil(pos->x_offset * scale_h), -ceil(pos->y_offset * scale_h) };
			git._apos = offset;
			git.advance = ceil(pos->x_advance * scale_h);
			//git.y_advance = ceil(pos->y_advance * scale_h);
			p->_vstr.push_back(git);
		}
	}
	glm::vec2 rct = {};
	float xxx = 0;
	int line_count = 1;
	auto ta = tb->style->text_align;
	glm::vec2 tps = {};
	glm::ivec4 rc = tb->rc;
	glm::vec2 ss = { rc.z,rc.w }, bearing = { 0, baseline };
	if (tb->style->autobr && rc.z > 0)
	{
		auto ps = bearing;
		ps.x += rc.x;
		ps.y += rc.y;
		for (auto& it : p->_vstr)
		{
			if (it.cpt == '\n' || tps.x + it.advance > ss.x)
			{
				tps.y += dh;
				tps.x = 0;
			}
			if (it.cpt == '\t')
			{
				it.advance = t->fontsize;
			}
			it._dwpos += it._apos;
			it._apos = ps + tps;
			tps.x += it.advance;
		}
	}
	else
	{
		for (auto& it : p->_vstr)
		{
			if (it.cpt == '\t' && it.advance < 1)
			{
				it.advance = t->fontsize;
			}
			if (it.cpt == '\n')
			{
				rct.x = std::max(rct.x, xxx);
				line_count++;
			}
			else
			{
				xxx += it.advance;
			}
		}
		rct.x = std::max(rct.x, xxx);
		rct.y = line_count * dh;
		if (ta.x < 0)ta.x = 0;
		if (ta.y < 0)ta.y = 0;
		// 区域大小 - 文本包围盒大小。居中就是text_align={0.5,0.5}
		auto ps = (ss - rct) * ta + bearing;
		ps.x += rc.x;
		ps.y += rc.y;
		for (auto& it : p->_vstr)
		{
			if (it.cpt == '\n')
			{
				tps.y += dh;
				tps.x = 0;
			}
			if (it.cpt == '\t')
			{
				it.advance = t->fontsize;
			}
			it._dwpos += it._apos;
			it._apos = ps + tps;
			tps.x += it.advance;
		}
	}
	p->tb = tb;
	return;
}

void build_text_render(text_block* tb, text_render_o* trt)
{
	if (!tb || !trt)return;
	update_text(trt, tb);
}

/*
[
  { "text": "标题", "bold": true, "size": 24 },
  "\n", // 换行
  { "text": "这是一段普通文本" },
  { "text": "加粗文本", "bold": true },
  { "text": "红色斜体", "italic": true, "color": "#ff0000" },
  {
	"type": "image",
	"src": "https://example.com/image.jpg",
	"width": 200,
	"height": 100
  },
  {
	"type": "link",
	"url": "https://example.com",
	"text": "这是一个链接"
  }
]
*/


#else
void do_text(const char* str, size_t first, size_t count)
{
}
#endif // !NO_HB_CX


#if 1

struct u84
{
	uint8_t r, g, b, a;
};
double get_alpha_f(uint32_t c)
{
	auto* t = (u84*)&c;
	return  t->a / 255.0;
}
bool is_alpha(uint32_t c)
{
	return (c & 0xFF000000);
}
uint32_t set_alpha(uint32_t c, uint32_t a)
{
	auto* t = (u84*)&c;
	t->a = std::min(a, (uint32_t)255);
	return c;
}
#ifdef USE_BGRA_PACKED_COLOR
#define COL32_R_SHIFT    16
#define COL32_G_SHIFT    8
#define COL32_B_SHIFT    0
#define COL32_A_SHIFT    24
#define COL32_A_MASK     0xFF000000
#else
#define COL32_R_SHIFT    0
#define COL32_G_SHIFT    8
#define COL32_B_SHIFT    16
#define COL32_A_SHIFT    24
#define COL32_A_MASK     0xFF000000
#endif
glm::vec4 ColorConvertU32ToFloat4(uint32_t in)
{
	float s = 1.0f / 255.0f;
	return glm::vec4(
		((in >> COL32_R_SHIFT) & 0xFF) * s,
		((in >> COL32_G_SHIFT) & 0xFF) * s,
		((in >> COL32_B_SHIFT) & 0xFF) * s,
		((in >> COL32_A_SHIFT) & 0xFF) * s);
}
void px_blend2c(uint32_t* pDstBmp, uint32_t src, uint32_t col)
{
	// C实现
	unsigned char* pSrc = (unsigned char*)&src;
	unsigned char* pDst = (unsigned char*)pDstBmp;
	uint32_t below_A, below_R, below_G, below_B;
	uint32_t above_A, above_R, above_G, above_B;
	glm::vec4 cf;

	above_B = *pSrc++;
	above_G = *pSrc++;
	above_R = *pSrc++;
	above_A = *pSrc++;
	if (col != -1 && above_A > 0)
	{
		cf = ColorConvertU32ToFloat4(col);
		above_B = cf.x * above_B;
		above_G = cf.y * above_G;
		above_R = cf.z * above_B;
		above_A = cf.w * above_A;
	}
	if (above_A == 0)
	{
		pDst += 4;
		return;
	}
	below_B = *pDst;
	below_G = *(pDst + 1);
	below_R = *(pDst + 2);
	below_A = *(pDst + 3);
	if (below_A == 0)
	{
		*pDstBmp = src;
		return;
	}
	uint32_t uc[] = { below_B - (below_B - above_B) * above_A / 255,
		below_G - (below_G - above_G) * above_A / 255,
		below_R - (below_R - above_R) * above_A / 255,
	};
	unsigned char d[4];
	d[0] = below_B - (below_B - above_B) * above_A / 255;
	d[1] = below_G - (below_G - above_G) * above_A / 255;
	d[2] = below_R - (below_R - above_R) * above_A / 255;
	auto lsa = pDst;
	if (below_A == 255)
		d[3] = 255;
	else
		d[3] = below_A - (below_A - above_A) * above_A / 255;
	*pDst++ = d[0];
	*pDst++ = d[1];
	*pDst++ = d[2];
	*pDst++ = d[3];
	return;
}
// 灰度图转rgba
void gray_copy2rgba(image_ptr_t* dst, image_ptr_t* src, const glm::ivec2& dst_pos, const glm::ivec4& rc, uint32_t col, bool isblend)
{
	if (src && src->data && src->comp == 1 && dst && dst->width > 0 && dst->height > 0 && dst->data && dst->comp == 4)
	{
		float brightness = 0;
		int w = rc.z, h = rc.w, posx = rc.x, posy = rc.y;
		glm::ivec2 ts = { dst->width,dst->height };
		auto bdata = (uint32_t*)dst->data;
		auto bit = (unsigned char*)(src->data) + (rc.y * src->width);

		int pitch = src->width;
		int ic = 0;
		auto ca = get_alpha_f(col);
		for (int j = 0; j < h && (j + dst_pos.y) < ts.y; j++)
		{
			auto pj = pitch * j;
			unsigned char* pixel = bit + pj;
			auto jp = j + dst_pos.y;
			int64_t psy = (jp * ts.x);
			if (psy < 0 || jp >= dst->height)
			{
				continue;
			}
			auto expanded_data = bdata + psy;
			uint32_t* dc = (uint32_t*)expanded_data;
			for (int i = 0; (i < w) && ((i + dst_pos.x) < ts.x); i++)
			{
				unsigned char c = pixel[i];
				if (c)
				{
					uint32_t uc = 0, ut = std::min(255.0f, brightness * c + c);
					if (ut > 255)ut = 255;
					ut *= ca;
					uc = set_alpha(col, ut);
					if (isblend)
					{
						px_blend2c(&dc[i + dst_pos.x], uc, -1);
					}
					else {
						dc[i + dst_pos.x] = uc;
					}
					ic++;
				}
			}
		}
		if (ic > 0)
			dst->valid = true;
	}
}
void gray_copy2rgba(image_ptr_t* dst, image_gray* src1, const glm::ivec2& dst_pos, const glm::ivec4& rc, uint32_t col, bool isblend)
{
	image_ptr_t src = {};
	src.data = (uint32_t*)src1->data();
	src.stride = src1->width;
	src.width = src1->width;
	src.height = src1->height;
	src.comp = 1;
	gray_copy2rgba(dst, &src, dst_pos, rc, col, true);
}
//单色位图1位
void bit_copy2rgba(image_ptr_t* dst, image_ptr_t* src, const glm::ivec2& dst_pos, const glm::ivec4& rc, uint32_t color)
{
	int posx = dst_pos.x, posy = dst_pos.y;
	int w = rc.z, h = rc.w;
	glm::ivec2 outsize = { dst->width,dst->height };
	unsigned char* bit = (unsigned char*)src->data;
	auto bdata = (uint32_t*)dst->data;
	for (int j = 0; j < h && (j + posy) < outsize.y; j++)
	{
		auto jp = j + posy;
		int64_t psy = (jp * outsize.x);
		if (psy < 0 || jp >= dst->height)
		{
			continue;
		}
		unsigned char* pixel = bit + src->stride * j;
		auto expanded_data = bdata + psy;
		unsigned int* dc = (unsigned int*)expanded_data;
		for (int i = 0; (i < w) && ((i + posx) < outsize.x); i++)
		{
			unsigned char c = (pixel[i / 8] & (0x80 >> (i & 7))) ? 255 : 0;
			if (c)
			{
				dc[i + posx] = color;
			}
		}
	}
}
uint32_t multiply_colors(uint32_t color1, uint32_t color2) {
	glm::vec4 c = *(glm::u8vec4*)&color1;
	glm::vec4 c1 = *(glm::u8vec4*)&color2;
	c /= 255;
	c1 /= 255;
	c *= c1;
	c *= 255;
	glm::u8vec4 col = c;
	return *(uint32_t*)&col;
}
void rgba_copy2rgba(image_ptr_t* dst, image_ptr_t* src, const glm::ivec2& dst_pos, const glm::ivec4& rc, uint32_t color, bool isblend)
{
	if (src && src->data && src->comp == 4 && dst && dst->width > 0 && dst->height > 0 && dst->data && dst->comp == 4)
	{
		int w = rc.z, h = rc.w, posx = rc.x, posy = rc.y;
		glm::ivec2 ts = { dst->width,dst->height };
		auto bdata = (uint32_t*)dst->data;
		auto bit = (uint32_t*)(src->data) + (rc.y * src->width);

		int pitch = src->width;
		int ic = 0;
		for (int j = 0; j < h && (j + dst_pos.y) < ts.y; j++)
		{
			auto pj = pitch * j;
			auto pixel = bit + pj + posx;
			auto jp = j + dst_pos.y;
			int64_t psy = (jp * ts.x);
			if (psy < 0 || jp >= dst->height)
			{
				continue;
			}
			auto expanded_data = bdata + psy;
			uint32_t* dc = (uint32_t*)expanded_data;
			for (int i = 0; (i < w) && ((i + dst_pos.x) < ts.x); i++)
			{
				auto c = pixel[i];
				if (c)
				{
					if (color != -1)
						c = multiply_colors(c, color);
					if (isblend)
					{
						px_blend2c(&dc[i + dst_pos.x], c, -1);
					}
					else {
						dc[i + dst_pos.x] = c;
					}
					ic++;
				}
			}
		}
		if (ic > 0)
			dst->valid = true;
	}
}


stbimage_load::stbimage_load()
{
}

stbimage_load::stbimage_load(const char* fn)
{
	load(fn);
}

stbimage_load::~stbimage_load()
{
	stbi_image_free(data);
}
void stbimage_load::free_img(stbimage_load* p)
{
	if (p)
		delete p;
}
stbimage_load* stbimage_load::new_load(const void* fnd, size_t len)
{
	stbimage_load t;
	if (fnd)
	{
		if (len)
			t.load_mem((char*)fnd, len);
		else
			t.load((char*)fnd);
	}
	if (t.data && t.width && t.height)
	{
		auto p = new stbimage_load();
		if (p)
		{
			*p = t;
			t.data = 0;
		}
		return p;
	}
	return nullptr;
}
void stbimage_load::tobgr()
{
	auto n = width * height;
	auto t = (char*)data;
	for (size_t i = 0; i < n; i++)
	{
		std::swap(*(t + 0), *(t + 2));
		t += 4;
	}
}
bool stbimage_load::load(const char* fn)
{
	hz::mfile_t mf;
	if (!fn || !*fn)return false;
	auto rawd = mf.open_d(fn, true);
	if (!rawd)
	{
		return false;
	}
	data = (uint32_t*)stbi_load_from_memory((stbi_uc*)rawd, mf.size(), &width, &height, &rcomp, comp);
	type = 0;
	return (data ? true : false);
}
bool stbimage_load::load_mem(const char* d, size_t s)
{
	data = (uint32_t*)stbi_load_from_memory((stbi_uc*)d, s, &width, &height, &rcomp, comp);
	type = 0;
	return (data ? true : false);
}

void save_img_png(image_ptr_t* p, const char* str)
{
	if (p && str && *str) {
		stbi_write_png(str, p->width, p->height, p->comp, p->data, p->stride);
	}
}

void save_img_png(image_gray* p, const char* str)
{
	if (p && str && *str) {
		stbi_write_png(str, p->width, p->height, 1, p->data(), p->width);
	}
}

#endif

#if 1

image_gray::image_gray()
{
}

image_gray::~image_gray()
{
}

unsigned char* image_gray::data()
{
	return _data.data();
}
size_t image_gray::size()
{
	return _data.size();
}
void image_gray::resize(size_t w, size_t h)
{
	if (w * h != width * height)
	{
		_data.resize(w * h);
		width = w;
		height = h;
	}
}
void image_gray::clear_color(unsigned char fill)
{
	memset(_data.data(), fill, _data.size());
}
int distance(const glm::vec2& s, const glm::vec2& p)
{
	int dis = (s.x - p.x) * (s.x - p.x) + (s.y - p.y) * (s.y - p.y);
	return dis;
}
//{c.xy = pos, c.z = radius}
bool inCircle(const glm::vec3& c, const glm::vec2& p)
{
	//计算点p和 当前圆圆心c 的距离
	int dis = distance(p, glm::vec2(c.x, c.y));
	//和半径比较
	return (dis <= c.z * c.z);
}

bool inRect(const glm::vec4& r, const glm::vec2& p)
{
	return !(p.x < r.x || p.y < r.y || p.x > r.x + r.z || p.y > r.y + r.w);
}

template<class T>
void draw_line2(const glm::ivec4& p, T color, T* dst, size_t w, size_t dst_as)
{
	int StartX = p.x, StartY = p.y, EndX = p.z, EndY = p.w;
	//////////////////////////////////////////////////////
	// Draws a line using the Bresenham line algorithm
	// Thanks to Jordan DeLozier <JDL>
	//////////////////////////////////////////////////////
	int jlen = 2;

	int x1 = StartX;
	int y1 = StartY;
	int x = x1;                       // Start x off at the first pixel
	int y = y1;                       // Start y off at the first pixel
	int x2 = EndX;
	int y2 = EndY;
	int xinc1, xinc2, yinc1, yinc2;      // Increasing values
	int den, num, numadd, numpixels;
	int deltax = abs(x2 - x1);        // The difference between the x's
	int deltay = abs(y2 - y1);        // The difference between the y's
	// Get Increasing Values
	if (x2 >= x1) {                // The x-values are increasing
		xinc1 = 1; xinc2 = 1;
	}
	else {                         // The x-values are decreasing
		xinc1 = -1; xinc2 = -1;
	}

	if (y2 >= y1) {                // The y-values are increasing
		yinc1 = 1; yinc2 = 1;
	}
	else {                         // The y-values are decreasing
		yinc1 = -1; yinc2 = -1;
	}

	// Actually draw the line
	if (deltax >= deltay)         // There is at least one x-value for every y-value
	{
		xinc1 = 0;                  // Don't change the x when numerator >= denominator
		yinc2 = 0;                  // Don't change the y for every iteration
		den = deltax;
		num = deltax / jlen;
		numadd = deltay;
		numpixels = deltax;         // There are more x-values than y-values
	}
	else                          // There is at least one y-value for every x-value
	{
		xinc2 = 0;                  // Don't change the x for every iteration
		yinc1 = 0;                  // Don't change the y when numerator >= denominator
		den = deltay;
		num = deltay / jlen;
		numadd = deltax;
		numpixels = deltay;         // There are more y-values than x-values
	}

	for (int curpixel = 0; curpixel <= numpixels; curpixel++)
	{
		// Draw the current pixel
		size_t xx = x + y * w;
		if (xx < dst_as)
			dst[xx] = color;

		num += numadd;              // Increase the numerator by the top of the fraction
		if (num >= den)             // Check if numerator >= denominator
		{
			num -= den;               // Calculate the new numerator value
			x += xinc1;               // Change the x as appropriate
			y += yinc1;               // Change the y as appropriate
		}
		x += xinc2;                 // Change the x as appropriate
		y += yinc2;                 // Change the y as appropriate
	}

}
void image_gray::draw_rect(const glm::ivec4& rect, unsigned char col, unsigned char fill, glm::ivec4 rounding)
{
	auto r = rect;
	glm::vec2 a = { r.x, r.y }; glm::vec2 b = { r.x + r.z,r.y + r.w };
	auto ds = _data.size();
	auto bdata = _data.data();
	//if (fill)
	{
		glm::vec3 c[4] = {};
		glm::vec4 rt[4] = {};
		int hr = std::min(r.w * 0.5, r.z * 0.5);
		if (rounding.x > 0)
		{
			c[0].z = std::min(rounding.x, hr);
			c[0].x = r.x + c[0].z;
			c[0].y = r.y + c[0].z;
			rt[0] = { r.x , r.y, c[0].z ,c[0].z };
		}
		if (rounding.y > 0)
		{
			c[1].z = std::min(rounding.y, hr);
			c[1].x = b.x - c[1].z - 1;
			c[1].y = r.y + c[1].z;
			rt[1] = { b.x - c[1].z , r.y, c[1].z ,c[1].z };
		}
		if (rounding.z > 0)
		{
			c[2].z = std::min(rounding.z, hr);
			c[2].x = b.x - c[2].z - 1;
			c[2].y = b.y - c[2].z - 1;
			rt[2] = { b.x - c[2].z ,b.y - c[2].z, c[2].z ,c[2].z };
		}
		if (rounding.w > 0)
		{
			c[3].z = std::min(rounding.w, hr);
			c[3].x = r.x + c[3].z;
			c[3].y = b.y - c[3].z - 1;
			rt[3] = { r.x , b.y - c[3].z, c[3].z ,c[3].z };
		}

		for (size_t y = a.y; y < b.y; y++)
		{
			for (size_t x = a.x; x < b.x; x++)
			{
				size_t idx = x + y * width;
				if (idx < ds)
				{
					bool isfill = true;
					for (int i = 0; i < 4; i++)
					{
						if (c[i].z <= 0)continue;
						glm::ivec2 pos = { x,y };
						bool ir = inRect(rt[i], pos);
						if (ir)
						{
							bool ic = inCircle(c[i], pos);
							if (!ic)
							{
								isfill = false;
							}
							break;
						}
					}
					if (isfill)
						bdata[idx] = fill;
				}
			}
		}
	}
	if (col)
	{
		draw_line2({ a.x, a.y, b.x, a.y }, col, bdata, width, ds);
		draw_line2({ b.x, a.y, b.x, b.y }, col, bdata, width, ds);
		draw_line2({ b.x, b.y, a.x, b.y }, col, bdata, width, ds);
		draw_line2({ a.x, a.y, a.x, b.y }, col, bdata, width, ds);
	}
}
void image_gray::draw_circle_fill(const glm::vec3& c, unsigned char fill)
{
	double r2 = c.z;
	glm::vec2 a = { c.x - r2, c.y - r2 }; glm::vec2 b = { c.x + r2,c.y + r2 };
	auto ds = _data.size();
	auto bdata = _data.data();
	for (size_t y = a.y; y < b.y; y++)
	{
		for (size_t x = a.x; x < b.x; x++)
		{
			size_t idx = x + y * width;
			if (idx < ds)
			{
				glm::ivec2 pos = { x,y };
				bool ir = inCircle(c, pos);
				if (ir)
					bdata[idx] = fill;
			}
		}
	}
}

//比较两个直方图

void yHist::init(int c)
{
	hists.resize(c);
	memset(hists.data(), 0, sizeof(int) * c);
}

void yHist::Hist(unsigned char c)
{
	hists[c]++;
}

bool yHist::diff(yHist* p, int dmin, int dmax)
{
	mset.clear();
	for (size_t i = 0; i < 256; i++)
	{
		mset.insert(hists[i] - p->hists[i]);
	}
	hmin = abs(*mset.begin());
	hmax = abs(*mset.rbegin());
	bool ret = (hmin > dmin) || (hmax > dmax);
	if (ret)
	{
		ret = true;
	}
	return ret;
}
double yHist::calculate_relation(yHist* p)
{
	assert(p && hists.size() == p->hists.size());
	// 计算内积
	double innerMul = 0.0;
	double amp1 = 0.0;
	double amp2 = 0.0;
	for (int i = 0; i < hists.size(); i++)
	{
		innerMul += hists[i] * p->hists[i];
		amp1 += pow(hists[i], 2);
		amp2 += pow(p->hists[i], 2);
	}
	double correlation = innerMul / (sqrt(amp1) * sqrt(amp2));
	return correlation;
}
#endif // 1

#ifdef __ANDROID__

njson get_sysfont(njson d)
{
	njson r;
	njson first0;
	std::vector<std::string> fnvs;
	std::set<std::string> fns;
	for (auto& [k, v] : d.items()) {
		if (v.find("[child]") == v.end())continue;
		auto& vc = v["[child]"];
		for (auto& ct : vc) {
			if (ct.find("family") == ct.end())continue;
			auto lang = toStr(ct["family"]["@lang"], "@undefined");
			auto& a = r["c"][lang];
			std::set<std::string> asv;
			auto& ft = ct["family"]["[child]"];
			for (auto& font : ft) {
				auto ft1 = font["font"];
				ft1["@style"];
				ft1["@weight"];
				for (auto& [k1, v1] : ft1.items()) {
					std::string fn1;
					if (k1 == "#text")
					{
						fn1 = v1;
					}
					else
					{
						if (!v1.is_array())continue;
						auto v10 = v1;
						auto fn = v10[0]["#text"];
						auto fnn = fn.get<std::string>();
						for (auto ch : fnn) {
							if (ch == '\r')
								break;
							fn1.push_back((ch));
						}
					}
					if (fn1.size()) {
						if (asv.insert(fn1).second)
							a.push_back(fn1);
						//if (fns.insert(fn1).second)
						//	fnvs.push_back(fn1);
					}
				}
			}
			if (first0.empty())
				first0 = fnvs;
			//fnvs.push_back("");
		}
	}
	r["a"] = first0;
	return r;
}

void ead_font(const std::string& internalDataPath)
{
	if (!ctx)return;
	std::string ftxml;
	hz::read_binary_file("/system/etc/fonts.xml", ftxml);
	do {
		auto dlt = ctx->get_lt();
		if (ftxml.empty())
		{
			break;
		}
		hz::save_cache("fonts.xml", ftxml.data(), ftxml.size(), ctx);
		auto ftn = xj::xml2pjson(ftxml);
		if (ftn) {
			{
				auto ndd = ftn->dump(2);
				save_cache("fonts1.json", ndd.data(), ndd.size(), ctx);
			}
			auto vsf = get_sysfont(*ftn);
			{
				auto& sfn = vsf["c"]["zh"];
				for (auto& it : sfn) {
					std::string k = it;
					k = "/system/fonts/" + hstring::trim_ch(k, "\t\r\n ");
					add_font(k.c_str(), false);
				}
			}
			{
				auto& sfn = vsf["c"]["zh-Hans"];
				for (auto& it : sfn) {
					std::string k = it;
					k = "/system/fonts/" + hstring::trim_ch(k, "\t\r\n ");
					add_font(k.c_str(), false);
				}
			}
#ifdef _WIN32
#else
			std::vector<std::string> mss = { "entypo.ttf" };
			for (auto it : mss)
			{
				std::string d;
				it = "fonts/" + it;
				hz::read_binary_file(it.c_str(), d);
				if (d.size())
				{
					auto fj = internalDataPath + it;//"entypo.ttf" 
					save_file(fj.c_str(), d.data(), d.size());
				}

			}

#endif // _WIN32
			auto fj = internalDataPath + "fonts/entypo.ttf";
			add_font(fj.c_str(), false);
			if (dlt)
			{
				dlt->set_font_family(vft.data(), vft.size());
			}
			auto ndd = vsf.dump(2);
			hz::save_cache("fonts2.json", ndd.data(), ndd.size(), ctx);
			delete ftn;
		}

	} while (0);
}
#endif // __ANDROID__
