#pragma once
namespace hz {
	// 获取监视器缩放比例,glm::ivec2*
	float get_monitor_scale(void* pels);
	// 获取打印机名称
	std::vector<std::string> get_print_devname();


	/*

		#define CF_PRIVATEFIRST     0x0200
		#define CF_PRIVATELAST      0x02FF

		drop_info_cx用来接收文本、文件名
	*/
	class drop_info_cx
	{
	public:
		// 接收返回数据type 0文本，1文件
		std::function<void(int type, int idx)> on_drop_cb = nullptr;
		// 准备接收时
		std::function<void(int x, int y)> on_dragover_cb = nullptr;
		size_t count = 0;
		int _type = 0;
		int has = 0;
		std::string _str;
		std::vector<std::string> _files;
		std::vector<const char*> _tmp;

		bool _drop_state = false;
		void* _pri = 0;
	public:
		drop_info_cx();
		~drop_info_cx();
		void make_pri();
		void clear_pri();
	public:
		void on_drop(int type, int idx);
		bool on_dragover(int x, int y);
	};
	// 拖放接收管理
	class drop_regs
	{
	private:
		void* _ctx = 0;
		void* _hwnd = 0;
	public:
		drop_regs();
		~drop_regs();
		void init(void* hwnd);
		void revoke();
		void release();
		void set_target(drop_info_cx* p);
		void set_over(std::function<void(int x, int y, int fmt)> overfunc);
	private:

	};

#if 0
#define CF_TEXT 1
#define CF_UNICODETEXT 13
#define CF_PRIVATEFIRST 0x0200
#define CF_PRIVATELAST 0x02ff
#endif // !0
	bool do_dragdrop_begin(unsigned short cf, const char* data, size_t dsize);
	bool do_dragdrop_begin(const char* str, size_t size);
	bool do_dragdrop_begin(const wchar_t* str, size_t size);
	bool do_dragdrop_begin(const std::string& str);
	bool do_dragdrop_begin(const std::wstring& str);

	void get_fullscreen_image(std::vector<uint32_t>* dst, int* width, int* height, const std::string& fn, int quality);
}
//!hz
