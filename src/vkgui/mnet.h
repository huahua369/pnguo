#pragma once
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <atomic>


namespace hz {

	class mcurl_cx
	{
	public:
		struct tNode
		{
			char* fpd;
			size_t startPos;
			size_t endPos;
			void* curl;
			int tid;
		};
		std::vector<std::thread> tv;
		std::atomic_int cnt = 0;
		std::vector<char> _data;
		std::vector<char> w_data;
		std::string _url;
		njson0 hn; 
	public:
		mcurl_cx();
		~mcurl_cx();

		bool downLoad(int threadNum, const std::string& Url);
		void set_httpheader(const char* k, const char* d);
		void post(const std::string& url, void* data, int len, bool copyd = false);
	private:
		int64_t getDownloadFileLenth(const char* url);

	};
	class net_cache_cx
	{
	private:
		std::string tp;
		std::map<std::string, std::string> mu;
		// 分段数量
		int mcount = 1;
	public:
		net_cache_cx();
		~net_cache_cx();
		// 设置缓存目录
		void set_dir(const std::string& name);
		// cb返回true则缓存到临时文件
		void load_uri_file(const std::string& fn, void* ptr, bool(*cb)(char* data, int len, void* ptr));
	};
	/*
	bool read_mem(char* data, int len, void* ptr) {
		auto sp = (sp_cx*)ptr;
		sp->load_sp_mem(data, len);
		return false;
	}
	void load_file(const std::string& fn)
	{
		load_uri_file(fn, this, read_mem);
	}
	*/
}
//!hz