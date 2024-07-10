/*
	页面管理
	Copyright (c) 华仔
	188665600@qq.com
	创建时间2024-07-10

	用于创建控件之类页面元素。
*/
class form_x;
class plane_cx;
struct style_plane_t;
struct style_tooltip
{
	const char* family = 0;
	int fonst_size = 16;
	int text_color = -1;	// 文本颜色
	glm::ivec2 color = {};	// 背景色，边框色
	float thickness = 1.0;	// 线宽
	float radius = 4;		// 矩形圆角 
};

// 显示工具提示面板 
void show_tooltip(form_x* form, const std::string& str, const glm::ivec2& pos, style_tooltip* bc);
void hide_tooltip(form_x* form);
