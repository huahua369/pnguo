#pragma once


uint32_t* get_wcolor();
const char* get_wcname(int x, int lang);
namespace hz {
	int main_ssh2();
	std::string cmdexe(const std::string &cmdstr);
}