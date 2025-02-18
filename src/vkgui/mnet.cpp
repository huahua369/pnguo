
#include <pch1.h>
#include "mnet.h"
#include "mapView.h"
#ifndef NO_CURL

#include <curl/curl.h>

namespace hz {


	mcurl_cx::mcurl_cx()
	{
	}

	mcurl_cx::~mcurl_cx()
	{
	}


	static size_t writeFunc(void* ptr, size_t size, size_t nmemb, void* userdata)
	{
		mcurl_cx::tNode* node = (mcurl_cx::tNode*)userdata;
		size_t written = 0;
		if (node->startPos + size * nmemb <= node->endPos)
		{
			auto p = node->fpd + node->startPos;
			memcpy(p, ptr, size * nmemb);
			node->startPos += size * nmemb;
			written = nmemb;
		}
		else
		{
			auto p = node->fpd + node->startPos;
			memcpy(p, ptr, node->endPos - node->startPos + 1);
			node->startPos = node->endPos;
			written = nmemb;
		}
		return written;
	}

	int progressFunc(void* ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded)
	{
		mcurl_cx* p = (mcurl_cx*)ptr;
		int percent = 0;
		if (totalToDownload > 0)
		{
			percent = (int)(nowDownloaded / totalToDownload * 100);
		}

		if (p && percent > p->cnt)
		{
			p->cnt = percent;
			//printf((char*)u8"下载进度%0d%%\n", percent);
		}
		return 0;
	}

	/************************************************************************/
	/* 获取要下载的远程文件的大小 											*/
	/************************************************************************/
	int64_t mcurl_cx::getDownloadFileLenth(const char* url)
	{
		double downloadFileLenth = 0;
		CURL* handle = curl_easy_init();
		curl_easy_setopt(handle, CURLOPT_URL, url);
		curl_easy_setopt(handle, CURLOPT_HEADER, 1);	//只需要header头
		curl_easy_setopt(handle, CURLOPT_NOBODY, 1);	//不需要body
		if (curl_easy_perform(handle) == CURLE_OK)
		{
			curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &downloadFileLenth);
		}
		else
		{
			downloadFileLenth = -1;
		}
		return downloadFileLenth;
	}


	bool mcurl_cx::downLoad(int threadNum, const std::string& Url)
	{
		auto fileLength = getDownloadFileLenth(Url.c_str());

		if (fileLength <= 0)
		{
			printf("get the file length error...");
			return false;
		}
		data.resize(fileLength);

		long partSize = fileLength / threadNum;

		for (int i = 0; i <= threadNum; i++)
		{
			tNode* pNode = new tNode();

			if (i < threadNum)
			{
				pNode->startPos = i * partSize;
				pNode->endPos = (i + 1) * partSize - 1;
			}
			else
			{
				if (fileLength % threadNum != 0)
				{
					pNode->startPos = i * partSize;
					pNode->endPos = fileLength - 1;
				}
				else
					break;
			}

			CURL* curl = curl_easy_init();

			pNode->curl = curl;
			pNode->fpd = data.data();

			char range[64] = { 0 };
			snprintf(range, sizeof(range), "%lld-%lld", pNode->startPos, pNode->endPos);

			// Download pacakge
			curl_easy_setopt(curl, CURLOPT_URL, Url.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)pNode);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
			curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressFunc);
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 5L);
			curl_easy_setopt(curl, CURLOPT_RANGE, range);

			std::thread a([=]() {
				if (pNode)
				{
					int res = curl_easy_perform(pNode->curl);
					if (res != 0)
					{

					}
					curl_easy_cleanup(pNode->curl);
					printf("thred %ld exit\n", pNode->tid);
					delete pNode;
				}
				});
			tv.push_back(std::move(a));
		}

		for (auto& it : tv) {
			it.join();
		}

		printf("download succed......\n");
		return true;
	}
#endif // !NO_CURL

	inline std::string to_hexstr(uint64_t _Val) {
		const auto _Len = static_cast<size_t>(_CSTD _scprintf("%llx", _Val));
		std::string _Str(_Len, '\0');
		_CSTD sprintf_s(&_Str[0], _Len + 1, "%llx", _Val);
		return _Str;
	}
	std::string get_hash(const std::string& str) {
		std::hash<std::string> h;
		size_t n = h(str);
		return to_hexstr(n);
	}

	net_cache_cx::net_cache_cx()
	{
	}

	net_cache_cx::~net_cache_cx()
	{
	}
	void net_cache_cx::set_dir(const std::string& name)
	{
		tp = hz::get_temp_path() + name;
#ifdef _WIN32
#define DIR_SC '\\'
#else
#define DIR_SC '/'
#endif // _WIN32
		if (*name.rbegin() != DIR_SC) {
			tp.push_back(DIR_SC);
		}
	}
	void net_cache_cx::load_uri_file(const std::string& fn, void* ptr, bool(*cb)(char* data, int len, void* ptr))
	{
		if (fn.find("http") == 0)
		{
			auto npath = get_hash(fn);
			if (tp.size())
			{
				if (npath.size())
				{
					auto nn = mu[fn];
					mu[fn] = npath;
					npath = tp + npath;
					{
						hz::mfile_t m;
						auto pd = m.open_d(npath, 0);
						if (pd && cb && m.size() > 0)
						{
							if (cb(pd, m.size(), ptr))
								return;
						}
					}
				}
			}
			mcurl_cx fbg;
			if (mcount < 1)mcount = 1;
			fbg.downLoad(mcount, fn);
			if (cb && fbg.data.size() > 2)
			{
				if (cb(fbg.data.data(), fbg.data.size(), ptr))
				{
					if (npath.size())
					{
						hz::mfile_t m;
						auto pd = m.new_m(npath, 0);
						m.ftruncate_m(fbg.data.size());
						auto wd = m.map(fbg.data.size(), 0);
						if (wd)
						{
							memcpy(wd, fbg.data.data(), fbg.data.size());
							m.flush();
						}
					}
				}
			}
		}
		else
		{
			hz::mfile_t m;
			auto pd = m.open_d(fn, 0);
			if (pd && cb)
			{
				cb(pd, m.size(), ptr);
			}
		}
	}
}
//!hz