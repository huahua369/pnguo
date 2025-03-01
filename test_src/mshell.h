#pragma once
/*依赖
find_package(Libssh2 CONFIG REQUIRED)
libssh2::libssh2
*/

uint32_t* get_wcolor();
const char* get_wcname(int x, int lang);
class cmde_cx
{
public:
#ifdef _WIN32
	HANDLE hInputRead = 0, hInputWrite = 0;
	HANDLE hOutputRead = 0, hOutputWrite = 0;
#endif
	std::string buf, wbuf;
	std::mutex lkm;
	std::thread aat;
	bool rune = true;
public:
	// rcb返回true则退出线程
	cmde_cx(std::function<void(const char*, int)> rcb);
	~cmde_cx();
	void write_str(const std::string& str);
private:

};
namespace hz {
	int main_ssh2();
	std::string cmdexe(const std::string& cmdstr, const char* cd);
	bool cmdexe(const std::string& cmdstr, const char* cd, std::function<void(const char*, int)> rcb, std::function<bool(std::string* str)> wcb);
	uint32_t get_themecolor();
}