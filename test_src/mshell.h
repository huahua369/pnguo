#pragma once
/*依赖
find_package(Libssh2 CONFIG REQUIRED)
libssh2::libssh2
*/

uint32_t* get_wcolor();
const char* get_wcname(int x, int lang);
namespace hz {
	int main_ssh2();
	std::string cmdexe(const std::string& cmdstr, const char* cd);
	bool cmdexe(const std::string& cmdstr, const char* cd, std::function<void(const char*, int)> cb);
	uint32_t get_themecolor();
}