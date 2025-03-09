#include <pch1.h>  
#include <pnguo/win_core.h>
#include "win32msg.h"
#include <pnguo/event.h>
#define GUI_STATIC_LIB
#include <pnguo/pnguo.h>
#include <pnguo/tinysdl3.h>
#include <pnguo/vkrenderer.h>
#include <pnguo/page.h>
#include <pnguo/mapView.h>
#include <pnguo/print_time.h>
#include <pnguo/mnet.h> 
#include <cairo/cairo.h>
#include <cmark.h> 
 
int main() {
#if 1
	hz::mfile_t mfile;
	auto markdown_text = mfile.open_d("E:\\d3m\\pnguo\\README.md", true);
	cmark_node* doc = cmark_parse_document(markdown_text, strlen(markdown_text), CMARK_OPT_DEFAULT);
	if (1) {
		cmark_iter* iter = cmark_iter_new(doc);
		while (cmark_iter_next(iter) != CMARK_EVENT_DONE) {
			cmark_node* node = cmark_iter_get_node(iter);
			auto type = cmark_node_get_type(node);
			switch (type) {

				/* Block */
			case CMARK_NODE_DOCUMENT:
			{
				// 文档渲染
				printf("document\n");
			}
			break;
			case CMARK_NODE_BLOCK_QUOTE:
			{
				// 块引用渲染
				printf("block quote\n");
			}
			break;
			case CMARK_NODE_LIST:
			{
				// 列表渲染
				auto list_type = cmark_node_get_list_type(node);
				auto list_start = cmark_node_get_list_start(node);
				printf("list %d %d\n", list_type, list_start);
			}break;
			case CMARK_NODE_ITEM:
			{
				// 列表项渲染
				auto list_type = cmark_node_get_list_type(node);
				auto list_start = cmark_node_get_list_start(node);
				printf("item %d %d\n", list_type, list_start);
			}break;
			case CMARK_NODE_CODE_BLOCK:
			{
				// 代码块渲染
				auto str = cmark_node_get_literal(node);
				printf("code block\t%s\n", str);
			}break;
			case CMARK_NODE_HTML_BLOCK:
			{
				// 块级HTML渲染
				auto str = cmark_node_get_literal(node);
				printf("html block\t%s\n", str);
			}break;
			case CMARK_NODE_CUSTOM_BLOCK:
			{
				// 自定义块渲染
				auto str = cmark_node_get_literal(node);
				printf("custom block\t%s\n", str);
			}break;
			case CMARK_NODE_PARAGRAPH:
			{
				// 段落渲染
				auto str = cmark_node_get_literal(node);
				printf("paragraph\t%s\n", str);
			}break;
			case CMARK_NODE_HEADING:
			{
				// 标题渲染（调整字体大小）
				auto hl = cmark_node_get_heading_level(node);
				printf("heading %d\n", hl);
				//cairo_set_font_size(cr, 24 - 2 * cmark_node_get_heading_level(node));
			}
			break;
			case CMARK_NODE_THEMATIC_BREAK:
			{
				// 分割线渲染
				printf("thematic break\n");
			}break;
			/* Inline */
			case CMARK_NODE_TEXT:
			{
				// 文本渲染 
				auto str = cmark_node_get_literal(node);
				printf("text\t%s\n", str);
			}
			break;
			case CMARK_NODE_SOFTBREAK:
			{
				// 软换行渲染
				printf("soft break\n");
			}break;
			case CMARK_NODE_LINEBREAK:
			{
				// 换行渲染
				printf("line break\n");
			}break;
			case CMARK_NODE_CODE:
			{
				// 代码渲染
				auto str = cmark_node_get_literal(node);
				printf("code\t%s\n", str);
			}break;
			case CMARK_NODE_HTML_INLINE:
			{
				// 内联HTML渲染
				auto str = cmark_node_get_literal(node);
				printf("html\t%s\n", str);
			}break;
			case CMARK_NODE_CUSTOM_INLINE:
			{
				// 自定义内联渲染
				auto str = cmark_node_get_literal(node);
				printf("custom\t%s\n", str);
			}break;
			case CMARK_NODE_EMPH:
			{
				// 强调渲染
				auto str = cmark_node_get_literal(node);
				printf("emph\t%s\n", str);
			}break;
			case CMARK_NODE_STRONG:
			{
				// 加粗渲染
				auto str = cmark_node_get_literal(node);
				printf("strong\t%s\n", str);
			}break;
			case CMARK_NODE_LINK:
			{
				// 链接渲染
				auto str = cmark_node_get_literal(node);
				printf("link\t%s\n", str);
			}break;
			case CMARK_NODE_IMAGE:
			{
				// 图片渲染
				auto str = cmark_node_get_literal(node);
				printf("image\t%s\n", str);
			}break;
			default:
				break;
			}
		}
		cmark_node_free(doc);
	}

#endif
	return 0;
}