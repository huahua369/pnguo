/*

	主class buffer_t
	2024-07-01 更新
	2021/3/31
	编辑文本

	原始加载数据不变，编辑的内容保存在额外文本块，视图引用数据
void test()
{
	auto buf = new hz::buffer_t();
	buf->append("abc", -1);

	struct input_info_t
	{
		glm::ivec2 pos;
		glm::ivec2 pos1;
		std::string str;
	};
	std::vector<input_info_t> _iit;// 操作列表
	int cuinc = 0;
	auto ic = cuinc;
	bool single_line = false;
	glm::ivec2 r = {};
	for (auto& it : _iit)
	{
		if (it.pos > it.pos1)
		{
			std::swap(it.pos, it.pos1);
		}
		if (it.str.empty())
		{
			// 执行删除
			buf->remove(it.pos, it.pos1);
			r = it.pos; cuinc++;
			//printf("\t光标d:%d\t%d\t%d\n", (int)it.str.size(), r.x, r.y);
		}
		else {
			// 插入文本
			if (single_line)
			{
				std::remove(it.str.begin(), it.str.end(), '\n');
			}
			//if (tvt && tvt->on_input)
			//{
			//	tvt->on_input(&it.str);// 执行回调函数
			//}
			if (it.str.empty())
			{
				r = it.pos;
			}
			else {
				r = buf->insert(it.pos, it.str.c_str(), it.str.size(), &it.pos1);// 插入文本
			}
			cuinc++;
			//printf("\t光标:%d\t%d\t%d\n", (int)it.str.size(), r.x, r.y);
		}
	}
	if (ic != cuinc)
	{
		buf->clear_redo(); // 清空重做栈
	}
	glm::ivec2 cp1 = { 0,0 }, cp2 = { 0,2 };
	// 获取选中文本
	auto str = buf->get_range(cp1, cp2);
}
*/
#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <functional>
#include <stack>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <string>
#include <queue>

namespace md { 
	int64_t get_utf8_count(const char* buffer, int64_t len);
	const char* utf8_char_pos(const char* buffer, int64_t pos, uint64_t len);
	uint32_t get_u8_idx(const char* str, int64_t idx);
	const char* get_u8_last(const char* str, uint32_t* codepoint);
	std::string u16to_u8(uint16_t* str, size_t len);
	std::wstring u8to_w(const char* str, size_t len);
}

#define _DEBUG_EDITf
namespace hz {
	class operate_stack;

	enum class e_undo :uint8_t
	{
		OP_UNDO = 1,
		OP_REDO
	};

	class undoable_operate
	{
	public:
		undoable_operate() {}
		virtual ~undoable_operate() {}

		virtual void undo() {}
		virtual void redo() {}
		virtual void execute() {}

		e_undo _op_type = e_undo::OP_UNDO;
	private:
	};

	class operate_stack
	{
	public:
		std::function<void(undoable_operate*)> free_cb;
	private:
		std::stack<undoable_operate*> _undo;
		std::stack<undoable_operate*> _redo;
	public:
		operate_stack();
		~operate_stack();
		void push_operate(undoable_operate* p);
		void execute(e_undo type);
		void clear_stack(int type = 0);
	protected:
		void pop_operate(e_undo type);
	};



	struct cmd_text_t
	{
		std::string str;
		glm::ivec2	pos;			// x列，y行		
		glm::ivec2	pos1;			// 删除end
		bool is_insert = true;		// true插入操作，false删除操作
	};

	struct block_t
	{
		std::string buffer;				// 字符串数据
		size_t count = 0;				// 字数
		std::vector<size_t> lineStarts;	// 行首位置
	};
	using block_it = std::list<block_t>::iterator;

	struct node_key_t
	{
		size_t size_left = 0, lf_left = 0;
	};
	// 节点保存字符串信息
	struct piece_t {
		glm::ivec2 _start = {};	// 位置开始
		glm::ivec2 _end = {};	// 结尾

		int length = 0;			// 字符数,utf8
		int lineFeedCnt = 0;	// 换行符数
		//int count = 0;
		block_t* _bt = 0;		// 数据指针
#ifdef _DEBUG_EDIT
		std::string str;
#endif
	public:
		piece_t() {}
		~piece_t() {}
		piece_t(block_t* bt, const glm::ivec2& s1, const glm::ivec2& s2, size_t len, size_t cnt)
			:_start(s1), _end(s2), length(len), lineFeedCnt(cnt), _bt(bt)
		{}
	};
	using piece_it = std::list<piece_t>::iterator;
	enum class NColor :uint8_t {
		Black = 0,
		Red = 1,
	};
	struct node_rbt {
		node_rbt* parent = 0;
		node_rbt* left = 0;
		node_rbt* right = 0;
		NColor color = NColor::Black;

		piece_t piece = {};
		// 左子树的大小（不按顺序）, 左子树中的换行cnt（不按顺序）
		size_t size_left = 0, lf_left = 0;
		//size_t size_left_ct = 0;
	public:
		node_rbt() {}
		~node_rbt() {}
		node_rbt(const piece_t& piece, NColor color) {
			this->piece = piece;
			this->color = color;
			this->size_left = 0;
			this->lf_left = 0;
			this->parent = this;
			this->left = this;
			this->right = this;
		}

		node_rbt* next();
		node_rbt* prev();
	};
	struct node_t {
		piece_it piece;
		// 左边数量，左边换行符数量
		size_t size_left = 0, lf_left = 0;
	};
	using node_tp = node_rbt*;
	class PieceTreeBase;
	class PieceTree;


	using node_it = std::list<node_t>::iterator;

	struct node_position {
		piece_it first;
		piece_it second;
		glm::ivec2 pos;
		glm::ivec2 pos2;
	};
	class NodePosition2
	{
	public:

		/**
		 * Piece Index
		 */
		piece_it node;
		/**
		 * remainer in current piece.
		*/
		size_t remainder;
		/**
		 * node start offset in document.
		 */
		size_t nodeStartOffset;
	};
	class text_able;
	// todo buffer
	class buffer_t
	{
	public:
		// 视图
		struct view_block_t
		{
			size_t line_start = 0;		// 开始行号
			size_t column_start = 0;		// 开始列号
			size_t line_cnt = 0;		// 行数
			block_t d;					// 字符串数据
		};

	private:
		// 数据集合块
		std::list<block_t> _data;
		// 最后一个buffer
		block_it _last_it;
		// 编辑节点列表
		std::list<piece_t> _piece;
		//std::list<node_t> _node;

		// 当前块
		node_t _curr_node;

		node_it _root;

		std::map<glm::ivec2, node_t> _mnode;
		// 视图编辑区域
		view_block_t	_view;
		// 当前编辑，[开始、结束位置]，如果编辑位置不在尾部则弹入撤销栈
		glm::ivec2 _epos[2] = { {1, 1}, {1, 1} };
		// 撤销重做
		operate_stack* _cmdstack = 0;
		// 只读模式不能编辑
		bool is_readonly = false;
		// 默认每块字节数
		int block_size = 1024 * 4;
		// 默认视图行数
		int _view_row_size = 100;

		// 总行数
		size_t _row_count = 0;

		// test
		int _line_average = 0;
		int _rl[2] = {};
		std::map<int, int> _linesize;
		PieceTree* _storage = 0;
	public:
		buffer_t();
		~buffer_t();
	public:
		// 编辑
		// 设置数据会把原有的清掉，包括撤销栈
		void set_data(const char* str, int len);
		// 撤销
		glm::ivec2 undo();
		// 重做
		glm::ivec2 redo();
		// 清空重做栈
		void clear_redo();
		// 清空所有栈
		void clear_stack();
		void set_cursor_idx(const glm::ivec2& idx);
		void set_cursor_idx(int64_t idx);
		// 插入数据
		glm::ivec2 append(const char* str, int len, glm::ivec2* rmpos = 0);
		glm::ivec2 insert(const glm::ivec2& pos, const char* str, int len, glm::ivec2* rmpos = 0);
		// 删除数据
		void remove(const glm::ivec2& pos, const glm::ivec2& pos2);
		// 批量删除
		void remove(const std::vector<glm::ivec4>& vpos);
		// 总行数量
		size_t row_size();
		// 获取指定行y偏移x或指定长度的字符串
		std::string get_row_str(size_t y, size_t x = 0, size_t len = -1);
		// 获取所有字符串
		std::string get_row_str(); 
		std::string get_range(const glm::ivec2& pos, const glm::ivec2& pos2);
	private:
		// 获取开始行号，n行数据，返回字节数。
		size_t view_rows(size_t first, size_t n, std::string& ostr);
		// 遍历保存数据，可能会返回多个数据块
		void save(size_t first, size_t n, std::function<void(block_t* bk)> cb);


		const char* data();


		// 更新视图到_data
		void update();
		// cmd
		void cmd_insert(const glm::ivec2& pos, const glm::ivec2& pos2, const char* str, int len);
		// 删除数据
		void cmd_remove(const glm::ivec2& pos, const glm::ivec2& pos2);
		// 批量删除
		void cmd_remove(const std::vector<glm::ivec4>& vpos);
		 
		// 批量删除
		//void view_remove(const std::vector<glm::ivec4>& vpos);
		void cmd_insert(cmd_text_t* ct);
		void cmd_remove(cmd_text_t* ct);
		void cmd_operate(cmd_text_t* ct);
		text_able* get_able(void* uo);
		void push_able(void* uo, text_able* p);

	public:
		// todo test api
		size_t get_node_bitsize();
	}; 

}
//!hz
