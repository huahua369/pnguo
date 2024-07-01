/*

	主class buffer_t
	2024-07-01 更新
	2021/3/31
	编辑文本

	原始加载数据不变，编辑的内容保存在额外文本块，视图引用数据
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
	// 实现在pnguo
	int64_t get_utf8_count(const char* buffer, int64_t len);
	const char* get_u8_last(const char* str, uint32_t* codepoint);
	uint32_t get_u8_idx(const char* str, int64_t idx);
	const char* utf8_char_pos(const char* buffer, int64_t pos, uint64_t len);
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
		operate_stack() {}
		~operate_stack() {}
		void push_operate(undoable_operate* p)            //插入操作
		{
			assert(p);
			if (p->_op_type == e_undo::OP_UNDO)
			{
				_undo.push(p);
			}
			else
			{
				_redo.push(p);
			}
		}
		void execute(e_undo type)                            //Ctrl-Z、Ctrl-Y执行一次撤消或重做
		{
			if (type == e_undo::OP_UNDO)
			{
				if (_undo.empty())return;
				undoable_operate* p = _undo.top();
				p->undo();
				p->_op_type = e_undo::OP_REDO;
				push_operate(p);
				pop_operate(type);
			}
			else if (type == e_undo::OP_REDO)
			{
				if (_redo.empty())return;
				undoable_operate* p = _redo.top();
				p->redo();
				p->_op_type = e_undo::OP_UNDO;
				push_operate(p);
				pop_operate(type);
			}
		}
		void clear_stack(int type = 0)                                 //清空操作
		{
			int n = (type == 0 ? 0x01 | 0x02 : 0);
			if ((e_undo)type == e_undo::OP_UNDO)
			{
				n |= 0x01;
			}
			if ((e_undo)type == e_undo::OP_REDO)
			{
				n |= 0x02;
			}
			if (free_cb)
			{
				if (n & 0x01)
				{
					while (!_undo.empty())
					{
						free_cb(_undo.top());
						_undo.pop();
					}
				}

				if (n & 0x02)
				{
					while (!_redo.empty())
					{
						free_cb(_redo.top());
						_redo.pop();
					}
				}

			}
			else {
				if (n & 0x01)
				{
					while (!_undo.empty())
					{
						delete (_undo.top());
						_undo.pop();
					}
				}

				if (n & 0x02)
				{
					while (!_redo.empty())
					{
						delete (_redo.top());
						_redo.pop();
					}
				}
			}
		}
	protected:
		void pop_operate(e_undo type)
		{
			if (type == e_undo::OP_UNDO)
			{
				_undo.pop();
			}
			else
			{
				_redo.pop();
			}
		}
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
		// 获取指定行字符串
		std::string get_row_str(size_t y, size_t x = 0, size_t len = -1);
		// 获取所有字符串
		std::string get_row_str();
		// 获取指定行号数量的字符串：first_row开始行号，count行数，cb(str=字符串，size=字节，count=字数)
		void get_row_str_cb(int first_row, int count, std::function<void(const char* str, int size, int count)> cb);
		std::string get_range(const glm::ivec2& pos, const glm::ivec2& pos2);
	private:

		// 获取开始行号，n行数据，返回整行数据。
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

		// 删除数据
		void view_remove(const glm::ivec2& pos, const glm::ivec2& pos2);
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
	//namespace rb {

	class PieceTreeBase
	{
	public:
		node_tp root = 0;
	public:
		PieceTreeBase();
		~PieceTreeBase();
	public:
		void rbDelete(node_tp z);
		void fixInsert(node_tp x);
		void updateTreeMetadata(node_tp x, size_t delta, size_t lineFeedCntDelta);
		virtual void detach(node_rbt* p) {
			p->parent = 0;
			p->left = 0;
			p->right = 0;
		}
	private:
		void leftRotate(node_tp x);
		void rightRotate(node_tp y);
		void recomputeTreeMetadata(node_tp x);
		void resetSentinel();

	};
	using Range = glm::ivec4;
	class Position :public glm::ivec2 {
	public:
		Position() {}
		Position(size_t y, size_t x) {
			this->x = x;
			this->y = y;
		}

	};
	class NodePosition
	{
	public:

		/**
		 * Piece Index
		 */
		node_tp node = 0;
		/**
		 * remainer in current piece.
		*/
		size_t remainder = 0;
		/**
		 * node start offset in document.
		 */
		size_t nodeStartOffset = 0;
	public:
		NodePosition()
		{
		}

		~NodePosition()
		{
		}

	private:

	};
	class PieceTreeSearchCache;
	class PieceTree :public PieceTreeBase
	{
	protected:
		std::list<block_t> _buffers;
		//std::vector<block_t> _buffers; // 0 is change buffer, others are readonly original buffer.

		size_t _lineCnt = 1;
		size_t _length = 0;
		size_t _count = 0;
		std::string _EOL = "\n";
		size_t _EOLLength = 1;
		bool _EOLNormalized = false;

		const int AverageBufferSize = 65535;
	private:
		glm::ivec2	_lastChangeBufferPos;
		PieceTreeSearchCache* _searchCache = 0;
		struct vline_t
		{
			int lineNumber = 0; std::string value;
		};
		vline_t _lastVisitedLine = {};
		size_t _node_count = 0;
		std::set<node_rbt*> _gf;
	public:
		PieceTree();
		~PieceTree();

		void init(const std::string& chunks, const std::string& eol = "\n"/*: '\r\n' | '\n'*/, bool eolNormalized = false);
		void init(const char* chunks, int len, const std::string& eol = "\n"/*: '\r\n' | '\n'*/, bool eolNormalized = false);

		virtual void detach(node_rbt* t);
		std::string get_range(glm::ivec2 startPosition, glm::ivec2 endPosition);
	public:
		std::vector<std::string> getLinesContent();
		// 字节
		size_t getLength();
		// 字数
		size_t get_count();
		// 行数
		size_t getLineCount();
		std::string getLineContent(size_t lineNumber);

		std::string getValueInRange2(NodePosition startPosition, NodePosition endPosition);

		void normalizeEOL(const std::string& eol/*: '\r\n' | '\n'*/);
		std::string getEOL();
		void setEOL(const std::string& newEOL);
		bool equal(PieceTree& other);
		size_t getOffsetAt(size_t lineNumber, size_t column);
		size_t getOffsetAt(const glm::ivec2& pos);
		Position getPositionAt(size_t offset);


		void computeBufferMetadata();

		bool iterate(node_tp node, std::function<bool(node_tp node)> callback);



		std::string getNodeContent(node_tp node);
		std::string getPieceContent(const piece_t& piece);

		node_tp rbInsertLeft(node_tp node, const piece_t& p);
		node_tp rbInsertRight(node_tp node, const piece_t& p);

		size_t getLineFeedCnt(block_it bt, glm::ivec2 start, glm::ivec2 end);
		size_t getLineFeedCnt(block_t* bt, glm::ivec2 start, glm::ivec2 end);
		size_t offsetInBuffer(block_t* bt, const glm::ivec2& cursor);
		size_t offsetInBuffer(block_it bt, const glm::ivec2& cursor);
		void deleteNodes(std::vector<node_tp> nodes);
		std::vector<piece_t> createNewPieces(std::string text);
		std::string	getLinesRawContent();

		std::string getLineRawContent(int lineNumber, int endOffset = 0);
		size_t getAccumulatedValue(node_tp node, int64_t index);



		void deleteNodeTail(node_tp node, glm::ivec2 pos);
		void deleteNodeHead(node_tp node, glm::ivec2 pos);
		void shrinkNode(node_tp node, glm::ivec2 start, glm::ivec2 end);
		void appendToNode(node_tp node, std::string value);
		NodePosition nodeAt(size_t offset);
		NodePosition nodeAt2(size_t lineNumber, size_t column);
		size_t nodeCharCodeAt(node_tp node, size_t offset);
		size_t offsetOfNode(node_tp node);
		// #endregion
		// #region CRLF
		bool shouldCheckCRLF();
		bool startWithLF(node_tp val);
		bool endWithCR(node_tp val);
		bool startWithLF(const std::string& val);
		bool endWithCR(const std::string& val);
		void validateCRLFWithPrevNode(node_tp nextNode);
		void validateCRLFWithNextNode(node_tp node);
		void fixCRLF(node_tp prev, node_tp next);
		bool adjustCarriageReturnFromNext(std::string value, node_tp node);


		glm::ivec2 getIndexOf(node_tp node, size_t accumulatedValue);

		std::string getValueInRange(Range range, const std::string& eol);

		size_t getLineCharCode(size_t lineNumber, size_t index);
		size_t getLineLength(size_t lineNumber);

		size_t insert_v(size_t offset, std::string value, bool eolNormalized = false);

		void remove_v(size_t offset, size_t cnt);


		void insertContentToNodeLeft(std::string value, node_tp node);
		void insertContentToNodeRight(std::string value, node_tp node);
		glm::ivec2 positionInBuffer(node_tp node, size_t remainder, glm::ivec2* ret = nullptr);
		std::string getContentOfSubTree(node_tp node);
	private:
		// 字数索引，数量，str输出
		void get_sub_string(block_t* bt, size_t idx, size_t count, std::string& str);
		std::string get_sub_string(block_t* bt, size_t idx, size_t count);
	private:
		node_rbt* new_node(const piece_t& piece, NColor color);
		//void free_node(node_rbt* p);
	};

}
//!hz
