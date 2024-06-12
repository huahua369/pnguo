
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
#include <pch1.h>
#include <mapView.h>
#include <stdio.h>
#ifdef _WIN32
#include <shlwapi.h>
#include <sys/timeb.h>
#include <direct.h>
#define mkdir(a, b) _mkdir(a)
#else
#include <sys/stat.h>
#include <sys/inotify.h>
#endif
#include <glib.h>

namespace md {
	uint32_t get_u8_idx(const char* str, int64_t idx);
	const char* get_u8_last(const char* str, uint32_t* codepoint);
	std::string u16to_u8(uint16_t* str, size_t len);
	std::wstring u8to_w(const char* str, size_t len);
}

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
			mkdir(file_name);
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
		std::string fnn = genfn(fn);
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
		hFile = CreateFileA(fnn.c_str(), _flags_o, _flags, NULL, dwcd
			, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
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
			LARGE_INTEGER ds;
			ds.QuadPart = rs;
			ret = SetFilePointerEx(hFile, ds, NULL, FILE_BEGIN);
			//ret = SetFilePointerEx(hFile, (LARGE_INTEGER)rs, NULL, FILE_BEGIN);
			SetEndOfFile(hFile);
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
		if (!CloseHandle(hFile))
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
		std::string fn = genfn(fnn);
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
			ret = ftruncate64(_fd, rs);
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
		bi.lpszTitle = title.c_str();//\0浏览文件夹";//下标题  
		bi.ulFlags = //BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
			BIF_RETURNONLYFSDIRS | BIF_USENEWUI /*包含一个编辑框 用户可以手动填写路径 对话框可以调整大小之类的..*/ |
			BIF_UAHINT /*带TIPS提示*/
			//| BIF_NONEWFOLDERBUTTON /*不带新建文件夹按钮*/
			;
		bi.lpfn = _SHBrowseForFolderCallbackProc;
		//bi.iImage=IDR_MAINFRAME;  
		bi.lParam = LPARAM(strCurrentPath.c_str());    //设置默认路径  

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
	std::string browse_folder_u8(const std::string& strCurrentPath, const std::string& title)
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
		std::wstring wt = md::u8to_w(title.c_str(), -1);
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

			ret = md::u16to_u8((uint16_t*)szResult, -1);
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
		auto wt = md::u8to_w(title.c_str(), -1);
		auto fwt = md::u8to_w(filter.c_str(), -1);
		auto cp = md::u8to_w(strCurrentPath.c_str(), -1);
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
				ret = md::u16to_u8((uint16_t*)strFilename, -1);
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

		auto wt = md::u8to_w(title.c_str(), -1);
		auto fwt = md::u8to_w(filter.c_str(), -1);
		auto cp = md::u8to_w(strCurrentPath.c_str(), -1);
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
					sfn.push_back(md::u16to_u8((uint16_t*)st.c_str(), -1));
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

	int browse_openfile(const std::string& title, const std::string& strCurrentPath, std::string filter, HWND hWnd
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
		opfn.hwndOwner = hWnd;

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
		GError* error = NULL;
		gchar* utf8_string = 0;
		gchar* fstr = (gchar*)str.c_str();
		// 初始化GLib
		//g_type_init();
		// 将GBK转换为UTF-8
		utf8_string = g_convert(fstr, -1, "UTF-8", "GBK", 0, NULL, &error);
		if (error) {
			g_warning("转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			//g_print("转换成功: %s\n", utf8_string);
			ret = utf8_string;
			g_free(utf8_string);
		}
		return ret;
	}
	std::string u16_to_u8(const std::wstring& str)
	{
		std::string ret;
		GError* error = NULL;
		gchar* utf8_string = 0;
		gchar* fstr = (gchar*)str.c_str();
		// 初始化GLib
		//g_type_init();
		// 将GBK转换为UTF-8
		gsize cs = str.size() * 2;
		utf8_string = g_convert(fstr, cs, "UTF-8", "UTF-16LE", 0, NULL, &error);
		if (error) {
			g_warning("转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			//g_print("转换成功: %s\n", utf8_string);
			ret = utf8_string;
			g_free(utf8_string);
		}
		return ret;
	}
	std::string u16_to_gbk(const std::wstring& str)
	{
		std::string ret;
		GError* error = NULL;
		gchar* utf8_string = 0;
		gchar* fstr = (gchar*)str.c_str();
		// 初始化GLib
		//g_type_init();
		// 将GBK转换为UTF-8
		gsize cs = str.size() * 2;
		utf8_string = g_convert(fstr, cs, "GBK", "UTF-16LE", 0, NULL, &error);
		if (error) {
			g_warning("转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			g_print("转换成功: %s\n", utf8_string);
			ret = utf8_string;
			g_free(utf8_string);
		}
		return ret;
	}
	std::string u8_to_gbk(const std::string& str)
	{
		std::string ret;
		GError* error = NULL;
		gchar* utf8_string = 0;
		gchar* fstr = (gchar*)str.c_str();
		// 初始化GLib
		//g_type_init();
		// 将GBK转换为UTF-8
		utf8_string = g_convert(fstr, -1, "GBK", "UTF-8", 0, NULL, &error);
		if (error) {
			g_warning("转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			g_print("转换成功: %s\n", utf8_string);
			ret = utf8_string;
			g_free(utf8_string);
		}
		return ret;
	}
	std::wstring u8_to_u16(const std::string& str)
	{
		std::wstring ret;
		GError* error = NULL;
		gchar* tstr = 0;
		gchar* fstr = (gchar*)str.c_str();
		gsize cs = str.size();
		tstr = g_convert(fstr, cs, "UTF-16LE", "UTF-8", 0, NULL, &error);
		if (error) {
			g_warning("转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			g_print("转换成功: %s\n", tstr);
			ret = (wchar_t*)tstr;
			g_free(tstr);
		}
		return ret;
	}
	std::wstring gbk_to_u16(const std::string& str)
	{
		std::wstring ret;
		GError* error = NULL;
		gchar* tstr = 0;
		gchar* fstr = (gchar*)str.c_str();
		gsize cs = str.size();
		tstr = g_convert(fstr, cs, "UTF-16LE", "GBK", 0, NULL, &error);
		if (error) {
			g_warning("转换出错: %s", error->message);
			g_error_free(error);
		}
		else {
			g_print("转换成功: %s\n", tstr);
			ret = (wchar_t*)tstr;
			g_free(tstr);
		}
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
			if (outn)
			{
				*outn = ret;
			}
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

	njson read_json(std::string fn)
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
		if (v.size())
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
	void save_json(std::string fn, const njson& n, int indent_cbor)
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










}//!hz

#endif //!NO_MAPFILE
