/*
	页面管理
	Copyright (c) 华仔
	188665600@qq.com
	创建时间2024-07-10

	用于创建控件之类页面元素。
*/


// 创建列表框，菜单
plane_cx* new_listbox(const std::vector<std::string>& v, const glm::ivec2& pos, style_plane_t* bc);
// 创建工具提示面板，color={背景色，边框色，文本颜色，字号}
plane_cx* new_tooltip(const std::string& str, const glm::ivec2& pos, style_plane_t* bc);
