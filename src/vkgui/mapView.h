#ifndef __MAPVIEW__H__
#define __MAPVIEW__H__
#include <string> 
#include <functional> 
#include <vector> 
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

#ifndef RWread


#define RWread(p,v,s,c) ((p)->read((void*)v,s*c))
#define RWtell(p) (p)->tell()
#define RWseek(p,pos,o) (p)->seek(pos,o)

#endif // !RWread

#ifndef NO_MAPFILE
namespace hz
{
	class rw_t
	{
	public:
		enum class seek_s
		{
			set = 0,
			cur = 1,
			end = 2
		};

	private:
		char* _data = 0;
		int64_t _pos = 0, _end = 0, _last_size = 0;

	public:
		int64_t tell()
		{
			return _pos;
		}
		size_t seek(int64_t _offset, int _origin = 0)
		{
			switch (_origin)
			{
			case 0:
				_pos = _offset;
				break;
			case 1:
				_pos += _offset;
				break;
			case 2:
				if (_offset > 0)
				{
					_offset = 0;
				}
				_pos = _end + _offset;
				break;
			default:
				break;
			}
			return _pos <= _end ? _pos : -1;
		}
		bool iseof()
		{
			return _pos >= _end;
		}
		virtual int64_t read(void* buf, int64_t len)
		{
			int64_t ret = 0;
			if (_end > 0 && buf)
			{
				char* d = (char*)_data;
				auto s = _end - _pos;
				ret = len;
				if (ret > s)
				{
					ret = s;
				}
				memcpy(buf, d + _pos, ret);
				_pos += ret;
				if (_pos > _end)
				{
					_pos = _end;
				}
			}
			return ret;
		}
		virtual int64_t write(const void* buf, int64_t len)
		{
			int64_t ret = 0, ns = _pos + len;
			if (_last_size < ns)
				_last_size = ns;
			if (_end < _last_size)
			{
				//resize(_last_size);
			}
			if (_end > 0 && buf)
			{
				char* d = (char*)_data;
				auto s = _end - _pos;
				ret = len;
				if (ret > s)
				{
					ret = s;
				}
				memcpy(d + _pos, buf, ret);
				_pos += ret;
				if (_pos > _end)
				{
					_pos = _end;
				}
			}
			return ret;
		}
		virtual int resize(int64_t s)
		{
			return 0;
		}
		int64_t size()
		{
			return _end;
		}
		void set_data(void* d)
		{
			_data = (char*)d;
		}
		void set_end(int64_t e)
		{
			_end = e;
		}
		void set(void* d, int64_t e)
		{
			_data = (char*)d;
			_end = e;
		}
		char* data() { return _data; }
	public:
		rw_t()
		{
		}
		rw_t(char* d, int64_t e) :_data(d), _end(e)
		{
		}

		virtual	~rw_t()
		{
		}

	private:

	};

	class mfile_t :public rw_t
	{
	private:
#ifdef _WIN32
		void* hFile = nullptr; // 文件的句柄 
		void* hMapFile = nullptr; // 文件内存映射区域的句柄 
#else
		int _fd = 0;
#endif // _WIN32
		std::string _fn;
		bool _is_rdonly = true;
		int _prot = 0, _flags = 0, _flags_fl = 0, _flags_o = 0;
		char* _pm = nullptr;
		// 映射的大小
		uint64_t _msize = 0;
		// 文件大小
		uint64_t _fsize = 0;
		// 1M
		int _block = 1024 * 1024;
		std::string _errstr;
	public:
		mfile_t();
		~mfile_t();
		/*
		is_rdonly=true则需要文件存在
		is_shared=是否共享，window设置false则会共享读
		is_async在window无效
		*/
		bool open_m(const std::string& fn, bool is_rdonly, bool is_shared = false, bool is_async = true);
		uint64_t get_file_size();
		// 修改文件大小
		int ftruncate_m(int64_t rs);
		// 创建映射，文件空的话pos必需为0
		char* map(uint64_t ms, int64_t pos);
		int flush(size_t pos = 0, size_t size = -1);
		int unmap(bool isclose = false);
		void close_m();
		// up是否更新到文件
		size_t write_m(const char* data, size_t len, bool up = false);
		uint64_t get_size();
		char* open_d(const std::string& fn, bool is_rdonly);
		// 打开或创建修改大小，0则不改变大小可能返回0
		char* new_m(const std::string& fn, size_t size);
		void clear_ptr();
		static std::string getLastError();
	private:

	};

	// 获取可创建临时文件的目录
	std::string get_temp_path();
	std::string genfn(std::string fn);

	int browse_openfile(const std::string& title, const std::string& strCurrentPath, std::string filter, HWND hWnd
		, std::function<void(const std::vector<std::string>&)> rfunc, int n = 10);
	int browse_folder(const std::string& strCurrentPath, std::function<void(const std::string&)> rfunc, const std::string& title = "");
	int browse_folder(const std::string& strCurrentPath, const std::string& title, std::function<void(const std::string&)> rfunc);
	std::string browse_folder(const std::string& strCurrentPath, const std::string& title);

	std::string browse_save_file(const std::string& title, const std::string& strCurrentPath, std::string filter, void* hWnd);

	std::vector<std::string> browse_openfile(const std::string& title, const std::string& strCurrentPath, std::string filter, void* hWnd, bool multi_select);
	// 打开资源管理器
	bool open_folder_select_file(std::string n);


	std::string gbk_to_u8(const std::string& str);
	std::string u16_to_u8(const std::wstring& str);
	std::string u16_to_gbk(const std::wstring& str);
	std::string u8_to_gbk(const std::string& str);
	std::wstring u8_to_u16(const std::string& str);
	std::wstring gbk_to_u16(const std::string& str);
	std::string gb_to_u8(const char* str, size_t len);
	std::string big5_to_u8(const char* str, size_t len);
	std::string shift_jis_to_u8(const char* str, size_t len);
	njson read_json(std::string fn);
	void save_json(std::string fn, const njson& n, int indent_cbor);
	std::string get_dir(const char* t);

	bool save_file(const std::string& fn, const char* data, uint64_t size, uint64_t pos, bool is_plus);

	bool is_utf8(const char* str, int len);

}//!hz

#endif
#endif /* end __MAPVIEW__H__*/


namespace md {

	void split(std::string str, const std::string& pattern, std::vector<std::string>& result);
	std::vector<std::string> split(const std::string& str, const std::string& pattern);
	// 多分割符
	std::vector<std::string> split_m(const std::string& str, const std::string& pattern, bool is_space);
	void get_lines(const std::string& str, std::function<void(const char* str)> cb);
	std::string replace_s(const std::string& str, const std::string& old_value, const std::string& new_value);
	// 验证是否为ut8编码
	bool validate_u8(const char* str, int len);
	int64_t get_utf8_count(const char* buffer, int64_t len);
	const char* utf8_char_pos(const char* buffer, int64_t pos, uint64_t len);
	uint32_t get_u8_idx(const char* str, int64_t idx);
	const char* get_u8_last(const char* str, uint32_t* codepoint);
	std::string u16_u8(uint16_t* str, size_t len);
	std::wstring u8_u16(const std::string& str);
	std::wstring u8_w(const char* str, size_t len);
	std::string gb_u8(const char* str, size_t len);

	uint32_t fons_decutf8(uint32_t* down, uint32_t* codep, uint32_t byte);
	const char* utf8_next_char(const char* p);
	const char* get_utf8_first(const char* str);
	const char* get_utf8_prev(const char* str);
	std::string trim(const std::string& str, const char* pch);
	std::string trim_ch(const std::string& str, const std::string& pch);
	int64_t file_size(FILE* fp);
}
