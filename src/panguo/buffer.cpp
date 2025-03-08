/*
 *	2023-8-22	删除测试代码
 * 2021/3/31
 * 编辑文本
 *
 */
#include <pch1.h>
#include "buffer.h"
#include <print_time.h>
#define let auto
namespace hz {
	enum BufferType {
		ORIGINAL,
		ADD
	};

	struct Piece {
		BufferType bufferType;
		size_t start;
		size_t length;

		Piece(BufferType type, size_t start, size_t length) : bufferType(type), start(start), length(length) {}
	};

	class PieceTable {
	private:
		std::string originalBuffer;
		std::string addBuffer;
		std::vector<Piece> pieces;

		struct Operation {
			enum Type {
				_INSERT,
				_DELETE
			} type;
			size_t position;
			std::string text;

			Operation(Type type, size_t position, const std::string& text) : type(type), position(position), text(text) {}
		};

		std::stack<Operation> undoStack;
		std::stack<Operation> redoStack;

	public:
		PieceTable(const std::string& text) {
			originalBuffer = text;
			pieces.emplace_back(ORIGINAL, 0, text.length());
		}

		void insert(size_t position, const std::string& text) {
			insertPiece(position, text, true);
			while (!redoStack.empty()) redoStack.pop(); // Clear redo stack on new operation
		}

		void deleteText(size_t position, size_t length) {
			deletePiece(position, length, true);
			while (!redoStack.empty()) redoStack.pop(); // Clear redo stack on new operation
		}

		void undo() {
			if (undoStack.empty()) return;
			Operation op = undoStack.top();
			undoStack.pop();

			if (op.type == Operation::_INSERT) {
				deletePiece(op.position, op.text.length(), false);
			}
			else if (op.type == Operation::_DELETE) {
				insertPiece(op.position, op.text, false);
			}

			redoStack.push(op);
		}

		void redo() {
			if (redoStack.empty()) return;
			Operation op = redoStack.top();
			redoStack.pop();

			if (op.type == Operation::_INSERT) {
				insertPiece(op.position, op.text, false);
			}
			else if (op.type == Operation::_DELETE) {
				deletePiece(op.position, op.text.length(), false);
			}

			undoStack.push(op);
		}

		std::string getText() {
			std::string result;

			for (const auto& piece : pieces) {
				if (piece.bufferType == ORIGINAL) {
					result.append(originalBuffer.substr(piece.start, piece.length));
				}
				else {
					result.append(addBuffer.substr(piece.start, piece.length));
				}
			}

			return result;
		}

	private:
		void insertPiece(size_t position, const std::string& text, bool recordOperation) {
			size_t addStart = addBuffer.length();
			addBuffer.append(text);
			size_t addLength = text.length();

			std::vector<Piece> newPieces;
			size_t currentPos = 0;

			for (auto& piece : pieces) {
				if (currentPos + piece.length <= position) {
					newPieces.push_back(piece);
					currentPos += piece.length;
				}
				else {
					if (position > currentPos) {
						size_t splitLength = position - currentPos;
						newPieces.emplace_back(piece.bufferType, piece.start, splitLength);
						piece.start += splitLength;
						piece.length -= splitLength;
					}

					newPieces.emplace_back(ADD, addStart, addLength);
					newPieces.push_back(piece);
					currentPos = position;
				}
			}

			if (currentPos < position) {
				newPieces.emplace_back(ADD, addStart, addLength);
			}

			pieces = newPieces;

			if (recordOperation) {
				undoStack.emplace(Operation::_INSERT, position, text);
			}
		}

		void deletePiece(size_t position, size_t length, bool recordOperation) {
			std::vector<Piece> newPieces;
			size_t currentPos = 0;
			std::string deletedText;

			for (auto& piece : pieces) {
				if (currentPos + piece.length <= position) {
					newPieces.push_back(piece);
					currentPos += piece.length;
				}
				else {
					if (position > currentPos) {
						size_t splitLength = position - currentPos;
						newPieces.emplace_back(piece.bufferType, piece.start, splitLength);
						piece.start += splitLength;
						piece.length -= splitLength;
						currentPos += splitLength;
					}

					if (piece.length > length) {
						deletedText.append((piece.bufferType == ORIGINAL ? originalBuffer : addBuffer).substr(piece.start, length));
						piece.start += length;
						piece.length -= length;
						length = 0;
						newPieces.push_back(piece);
					}
					else {
						deletedText.append((piece.bufferType == ORIGINAL ? originalBuffer : addBuffer).substr(piece.start, piece.length));
						length -= piece.length;
					}
				}
			}

			pieces = newPieces;

			if (recordOperation) {
				undoStack.emplace(Operation::_DELETE, position, deletedText);
			}
		}
	};
	///////////-------text_able文本框操作--------------------------------------

	class text_able :public undoable_operate
	{
	public:
		std::function<void(cmd_text_t*)> _func;
	private:
		std::vector<cmd_text_t>    _operate;
		bool                    _one = true;
	public:
		text_able() {}
		~text_able() { _operate.clear(); }

		void undo()
		{
			if (!(_func))return;
			for (auto it = _operate.rbegin(); it != _operate.rend(); ++it)
			{
				auto& p = *it;
				_func(&p);
				it->is_insert = !it->is_insert;
			}
		}
		void redo()
		{
			if (!(_func))return;
			for (auto it = _operate.begin(); it != _operate.end(); ++it)
			{
				auto& p = *it;
				_func(&p);
				it->is_insert = !it->is_insert;
			}
		}
		void execute()
		{
			redo();
		}
		void add(cmd_text_t so)
		{
			_operate.push_back(so);
		}
		size_t  size()
		{
			return _operate.size();
		}
		void clear()
		{
			_operate.clear();
		}
	};

#if 0
	OperateStack* _txt_opstack = OperateStack::create();
	void undo()
	{
		if (getReadonly() || !_txt_opstack)
			return;
		_txt_opstack->Execute(OP_UNDO);

	}
	void redo()
	{
		if (getReadonly() || !_txt_opstack)
			return;

		_txt_opstack->Execute(OP_REDO);
	}
	void on_ime_text(char* str, int len)
	{
		TxtOperate* ptxt = TxtOperate::create();
		ptxt->_func = [=](StrOperate& so)
			{
				so._to_type == StrOperate::TO_INSERT ? str_insert(so) : str_delete(so);
			};
		ptxt->_op_type = OP_UNDO; //撤消操作
		// 删除选中操作
		ptxt->insertOperate(StrOperate{ seletxt.c_str(), spos, s_strat, s_end, StrOperate::TO_DELETE, 1 });
		// 插入文本操作
		ptxt->insertOperate(StrOperate{ wstr.c_str(), spos, 0, StrOperate::TO_INSERT, 0 });

		ptxt->Execute();						//执行
		if (_txt_opstack && ptxt->size())		//判断是否开启撤销功能
		{
			_txt_opstack->pushOperate(ptxt);    //弹入撤消栈
		}
		else
		{
			TxtOperate::destroy(ptxt);
		}
	}
#endif // 0

#if 1
#ifndef HZ_TCB_1
#define HZ_TCB_1(__selector__, ...)	 std::bind(__selector__, this, std::placeholders::_1, ##__VA_ARGS__)
#endif // !HZ_TCB_1
	text_able* buffer_t::get_able(void* uo) {

		auto p = new text_able();
		p->clear();
		if (!p->_func)
			p->_func = HZ_TCB_1(&buffer_t::cmd_operate);
		return p;
	}
	void buffer_t::push_able(void* uo, text_able* p)
	{
		if (p)delete p;
		//((objs_text_able*)uo)->push_rc(p);
	}
#endif
	// todo buffer edit
	buffer_t::buffer_t()
	{
		//unobj = new objs_text_able_cx();
		//if (unobj)
		{
			_cmdstack = new operate_stack();
			_cmdstack->free_cb = [=](undoable_operate* p)
				{
					push_able(0, (text_able*)p);
				};
		}

		_storage = new PieceTree();
	}

	buffer_t::~buffer_t()
	{
		if (_storage)delete _storage; _storage = 0;
		if (_cmdstack)delete _cmdstack; _cmdstack = 0;
	}

	void get_brpos(const char* str, int len, int first, std::vector<size_t>& v)
	{
		if (v.empty())
		{
			v.push_back(0);
		}
		for (size_t i = 0; i < len; i++)
		{
			if (str[i] == '\n')
			{
				v.push_back(i + first + 1);
			}
		}
	}
	void buffer_t::set_data(const char* str, int len)
	{
		size_t first = 0;
		if (_cmdstack)
			_cmdstack->clear_stack();
		_data.clear();

		if (len < 0)
		{
			len = strlen(str);
		}

		if (_storage)
		{
			auto olds = _storage->getLinesRawContent();
			if (olds.size() > 0 && olds.size() == len && 0 == olds.compare(0, len, str, len))
				return;

			delete _storage;
			_storage = new PieceTree();

			_storage->init(str, len);
		}

	}

	glm::ivec2 buffer_t::append(const char* str, int len, glm::ivec2* rmpos)
	{
		return insert(_epos[1], str, len, rmpos);
	}


	glm::ivec2 buffer_t::insert(const glm::ivec2& pos1, const char* str, int len, glm::ivec2* rmpos)
	{
		//print_time ftpt("view_insert");
		glm::ivec2 ret = { -1, -1 };
		auto pos = pos1;
		if (len < 0)
		{
			len = strlen(str);
		}
		if (len == 0)
		{
			return ret;
		}
		if (rmpos && pos != *rmpos)
		{
			cmd_remove(pos, *rmpos);
			_epos[0] = _epos[1] = pos;
		}
		// 不在范围内则插入数据fadfdsfasdf
		auto cl = md::get_utf8_count(str, len);
		if (pos != _epos[1])
		{
			if (_epos[0] != _epos[1])
			{
				std::string str;
				if (_storage && _epos[0] < _epos[1])
					str = _storage->get_range(_epos[0], _epos[1]);
				if (str.size())
				{
					cmd_insert(_epos[0], _epos[1], str.c_str(), str.size());
				}
			}
			_epos[0] = _epos[1] = pos;
		}
		if (_storage)
		{
			size_t offset = _storage->getOffsetAt(pos);
			auto nret = _storage->insert_v(offset, std::string(str, len));
			auto epos = _storage->getPositionAt(nret);
			_epos[1] = epos;
		}
		ret = _epos[1];

		return ret;
	}

	void buffer_t::remove(const glm::ivec2& pos, const glm::ivec2& pos2)
	{
		cmd_remove(pos, pos2);
	}
	// 批量删除
	void buffer_t::remove(const std::vector<glm::ivec4>& vpos)
	{
		cmd_remove(vpos);
	}
	 
	glm::ivec2 buffer_t::undo()
	{
		update();
		if (!is_readonly && _cmdstack)
			_cmdstack->execute(e_undo::OP_UNDO);;
		return  _epos[1];
	}

	glm::ivec2 buffer_t::redo()
	{
		update();
		if (!is_readonly && _cmdstack)
			_cmdstack->execute(e_undo::OP_REDO);
		return  _epos[1];
	}

	void buffer_t::clear_redo()
	{
		_cmdstack->clear_stack((int)e_undo::OP_REDO);
	}
	void buffer_t::clear_stack()
	{
		_cmdstack->clear_stack(0);
	}

	void buffer_t::set_cursor_idx(const glm::ivec2& idx)
	{
		//update();
		_epos[0] = _epos[1] = idx;
	}
	void buffer_t::set_cursor_idx(int64_t idx)
	{
		auto pos = _storage->getPositionAt(idx);
		//update();
		_epos[0] = _epos[1] = pos;
	}
	void buffer_t::cmd_insert(cmd_text_t* ct)
	{
		size_t offset = _storage->getOffsetAt(ct->pos);
		auto nret = _storage->insert_v(offset, std::string(ct->str));
		auto epos = _storage->getPositionAt(nret);
		_epos[0] = _epos[1] = epos;
	}
	void buffer_t::cmd_remove(cmd_text_t* ct)
	{
		size_t offset = _storage->getOffsetAt(ct->pos);
		size_t offset1 = _storage->getOffsetAt(ct->pos1);
		if (ct->pos > ct->pos1)
			std::swap(ct->pos, ct->pos1);
		_epos[0] = _epos[1] = ct->pos;
		if (ct->str.empty())
		{
			ct->str = get_range(ct->pos, ct->pos1);
		}
		_storage->remove_v(offset, offset1 - offset);
	}
	void buffer_t::cmd_operate(cmd_text_t* ct)
	{
		if (ct->is_insert)
			cmd_insert(ct);
		else
			cmd_remove(ct);
	}
	char* cp_str(const char* str, int len, size_t& olen)
	{
		if (len < 0)len = strlen(str);
		if (len == 0)
		{
			return 0;
		}
		char* ps = (char*)malloc(len);
		if (ps)
		{
			memcpy(ps, str, len);
		}
		olen = len;
		return ps;
	}
	void buffer_t::cmd_insert(const glm::ivec2& pos, const glm::ivec2& pos2, const char* str, int length)
	{
		assert(!(!str || length < 1));
		if (!str || !_cmdstack)return;
		text_able* p = get_able(0);
		p->_op_type = e_undo::OP_UNDO;
		{
			cmd_text_t ct;
			ct.is_insert = false;
			ct.pos = pos;
			ct.pos1 = pos2;
			if (length < 0)length = strlen(str);
			ct.str.assign(str, length);
			p->add(ct);
		}
		// 这里不需要执行操作
		//p->execute();
		_cmdstack->push_operate(p);
	}


	void buffer_t::cmd_remove(const glm::ivec2& pos, const glm::ivec2& pos2)
	{
		auto str = get_range(pos, pos2);
		if (str.empty())
		{
			return;
		}
		text_able* p = get_able(0);
		p->_op_type = e_undo::OP_UNDO;
		{
			cmd_text_t ct;
			ct.is_insert = false;
			ct.str = std::move(str);
			ct.pos = pos;
			ct.pos1 = pos2;
			p->add(ct);
		}
		p->execute();
		if (_cmdstack)
		{
			_cmdstack->push_operate(p);
		}
		else {
			push_able(0, p);
		}
	}
	// glm::ivec2 pos, glm::ivec2 pos2
	void buffer_t::cmd_remove(const std::vector<glm::ivec4>& vpos)
	{
		text_able* p = get_able(0);
		p->_op_type = e_undo::OP_UNDO;
		for (auto& it : vpos)
		{
			cmd_text_t ct;
			ct.is_insert = false;
			ct.pos = { it.x, it.y };
			ct.pos1 = { it.z, it.w };
			ct.str = std::move(get_range(ct.pos, ct.pos1));
			p->add(ct);
		}
		p->execute();
		if (_cmdstack)
		{
			_cmdstack->push_operate(p);
		}
		else {
			push_able(0, p);
		}
	}

	size_t buffer_t::view_rows(size_t first, size_t n, std::string& ostr)
	{ 
		ostr.clear();
		if (first < _view.line_start || first > _view.line_start + _view.line_cnt)
		{
			for (size_t i = 0; i < n; i++)
			{
				ostr += get_row_str(i + first);
			}
		}
		else
		{
			ostr = get_range({ 1, first }, { -1, first + n - 1 });
		}
		return ostr.size();
	}
	// 获取指定行字符串
	std::string buffer_t::get_row_str(size_t y, size_t x /*= 0*/, size_t len /*= -1*/)
	{
		auto ret = _storage->getLineRawContent(y);
		if (ret.size())
		{
			if (len > ret.size())len = ret.size();
			if (x < ret.size() && x > 0) {
				ret = ret.substr(x, len - x);
			}
		} 
		return ret;
	}
	// 获取所有字符串
	std::string buffer_t::get_row_str()
	{
		return _storage->getLinesRawContent();
	}
	size_t buffer_t::row_size()
	{
		return _row_count;
	}

	void buffer_t::save(size_t first, size_t n, std::function<void(block_t* bk)> cb)
	{

	}
	const char* buffer_t::data()
	{
		return _view.d.buffer.c_str();
	}

	std::string buffer_t::get_range(const glm::ivec2& pos, const glm::ivec2& pos2)
	{
		std::string ret;
		if (_storage)
			ret = _storage->get_range(pos, pos2);
		return ret;
	}

	void buffer_t::update()
	{
		if (_epos[0] != _epos[1] && _epos[0] < _epos[1])
		{
			std::string str;
			if (_storage)
				str = _storage->get_range(_epos[0], _epos[1]);
			if (str.size())
			{
				cmd_insert(_epos[0], _epos[1], str.c_str(), str.size());
			}
			_epos[0] = _epos[1];
		}
		_curr_node.piece = _piece.begin();
		_curr_node.size_left = 0;
		_curr_node.lf_left = 0;
	}


	size_t buffer_t::get_node_bitsize()
	{
		size_t ret = _piece.size();
		size_t pts = sizeof(_piece);
		size_t pts1 = sizeof(piece_it);
		size_t pts2 = sizeof(piece_t);
		return ret * pts2;
	}

}
//!hz

/*
#include <ntype.h>
#include <queue>
#include <base/camera.h>
#include <data/json_helper.h>

#include "buffer.h"
*/

namespace hz {
	node_tp leftest(node_tp node);
	node_tp righttest(node_tp node);

	size_t calculateSize(node_tp node);
	size_t calculateLF(node_tp node);
	//namespace rb {
#define let auto
		/*
		enum class NColor :uint8_t {
			Black = 0,
			Red = 1,
		};class node_rbt {
		public:
			node_rbt* parent = this;
			node_rbt* left = this;
			node_rbt* right = this;
			NColor color = NColor::Black;

			piece_t piece;
			size_t size_left = 0, lf_left = 0;
		public:
			node_rbt() {}
			node_rbt(piece_t piece, NColor color) {
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
		//using node_tp = node_rbt*;*/
		//using node_it = std::list<node_rbt>::iterator;

		//node_rbt parent,		left,	//	right;
		//NColor color;

		// piece_t
		//piece;
		//size_t size_left; // size of the left subtree (not inorder)
		//size_t lf_left; // line feeds cnt in the left subtree (not in order)
#if 1
	//fixInsert, leftest, rbDelete, righttest, updateTreeMetadata
	//node_tp next(node_tp t);
	//node_tp prev(node_tp t);

#endif
#define SENTINEL nullptr

#if 1

	node_tp node_rbt::next()
	{
		node_tp t = this;
		if (t->right != SENTINEL) {
			return leftest(t->right);
		}

		node_tp node = t;

		while (node->parent != SENTINEL) {
			if (node->parent->left == node) {
				break;
			}

			node = node->parent;
		}

		if (node->parent == SENTINEL) {
			return SENTINEL;
		}
		else {
			return node->parent;
		}
	}

	node_tp node_rbt::prev()
	{
		node_tp t = this;
		if (t->left != SENTINEL) {
			return righttest(t->left);
		}

		auto node = t;

		while (node->parent != SENTINEL) {
			if (node->parent->right == node) {
				break;
			}

			node = node->parent;
		}

		if (node->parent == SENTINEL) {
			return SENTINEL;
		}
		else {
			return node->parent;
		}
	}

	//void detach(node_tp t) {
	//	t->parent = 0;
	//	t->left = 0;
	//	t->right = 0;

	//}


	node_tp leftest(node_tp node)
	{
		while (node->left != SENTINEL) {
			node = node->left;
		}
		return node;
	}

	node_tp righttest(node_tp node)
	{
		while (node->right != SENTINEL) {
			node = node->right;
		}
		return node;
	}

	size_t calculateSize(node_tp node)
	{
		if (node == SENTINEL) {
			return 0;
		}

		return node->size_left + node->piece.length + calculateSize(node->right);
	}

	size_t calculateLF(node_tp node)
	{
		if (node == SENTINEL) {
			return 0;
		}

		return node->lf_left + node->piece.lineFeedCnt + calculateLF(node->right);
	}



	PieceTreeBase::PieceTreeBase() {}
	PieceTreeBase::~PieceTreeBase() {}
	void PieceTreeBase::resetSentinel() {
		//PieceTreeBase::SENTINEL->parent = SENTINEL;
	}
	void PieceTreeBase::leftRotate(node_tp x)
	{
		auto y = x->right;
		if (!y)return;
		// fix size_left
		y->size_left += x->size_left + x->piece.length;// (x->piece ? x->piece.length : 0);
		y->lf_left += x->lf_left + x->piece.lineFeedCnt;// (x->piece ? x->piece.lineFeedCnt : 0);
		x->right = y->left;

		if (y->left != SENTINEL) {
			y->left->parent = x;
		}
		y->parent = x->parent;
		if (x->parent == SENTINEL) {
			root = y;
		}
		else if (x->parent->left == x) {
			x->parent->left = y;
		}
		else {
			x->parent->right = y;
		}
		y->left = x;
		x->parent = y;
	}

	void PieceTreeBase::rightRotate(node_tp y)
	{
		auto x = y->left;
		if (!x)return;
		y->left = x->right;
		if (x->right != SENTINEL) {
			x->right->parent = y;
		}
		x->parent = y->parent;

		// fix size_left
		y->size_left -= x->size_left + x->piece.length;//(x->piece ? x->piece.length : 0);
		y->lf_left -= x->lf_left + x->piece.lineFeedCnt;//(x->piece ? x->piece.lineFeedCnt : 0);

		if (y->parent == SENTINEL) {
			root = x;
		}
		else if (y == y->parent->right) {
			y->parent->right = x;
		}
		else {
			y->parent->left = x;
		}

		x->right = y;
		y->parent = x;
	}

	void PieceTreeBase::rbDelete(node_tp z)
	{
		node_tp x = 0;
		node_tp y = 0;
		if (!z)return;
		if (z->left == SENTINEL) {
			y = z;
			x = y->right;
		}
		else if (z->right == SENTINEL) {
			y = z;
			x = y->left;
		}
		else {
			y = leftest(z->right);
			x = y->right;
		}

		if (y == root) {
			root = x;

			// if x is null, we are removing the only node
			if (x)
				x->color = NColor::Black;
			detach(z);
			resetSentinel();
			if (root)
				root->parent = SENTINEL;

			return;
		}

		auto yWasRed = (y->color == NColor::Red);

		if (y == y->parent->left) {
			y->parent->left = x;
		}
		else {
			y->parent->right = x;
		}

		if (y == z) {
			if (x)
			{
				x->parent = y->parent;
				recomputeTreeMetadata(x);
			}
		}
		else {
			if (x)
			{
				if (y->parent == z) {
					x->parent = y;
				}
				else {
					x->parent = y->parent;
				}

				// as we make changes to x's hierarchy, update size_left of subtree first
				recomputeTreeMetadata(x);
			}
			y->left = z->left;
			y->right = z->right;
			y->parent = z->parent;
			y->color = z->color;

			if (z == root) {
				root = y;
			}
			else {
				if (z == z->parent->left) {
					z->parent->left = y;
				}
				else {
					z->parent->right = y;
				}
			}

			if (y->left != SENTINEL) {
				y->left->parent = y;
			}
			if (y->right != SENTINEL) {
				y->right->parent = y;
			}
			// update metadata
			// we replace z with y, so in this sub tree, the length change is z->item.length
			y->size_left = z->size_left;
			y->lf_left = z->lf_left;
			recomputeTreeMetadata(y);
		}

		detach(z);
		if (x)
		{
			if (x->parent->left == x) {
				auto newSizeLeft = calculateSize(x);
				auto newLFLeft = calculateLF(x);
				if (newSizeLeft != x->parent->size_left || newLFLeft != x->parent->lf_left) {
					auto delta = newSizeLeft - x->parent->size_left;
					auto lf_delta = newLFLeft - x->parent->lf_left;
					x->parent->size_left = newSizeLeft;
					x->parent->lf_left = newLFLeft;
					updateTreeMetadata(x->parent, delta, lf_delta);
				}
			}

			recomputeTreeMetadata(x->parent);
		}
		else {
			//assert(x);
		}
		if (yWasRed) {
			resetSentinel();
			return;
		}
		if (x)
		{
			// RB-DELETE-FIXUP
			node_tp w;
			while (x != root && x->color == NColor::Black) {
				if (x == x->parent->left) {
					w = x->parent->right;
					if (!w)
					{
						break;
					}
					if (w->color == NColor::Red) {
						w->color = NColor::Black;
						x->parent->color = NColor::Red;
						leftRotate(x->parent);
						w = x->parent->right;
						if (!w)
						{
							break;
						}
					}
					// todo w->right==0
					if (w->left && w->left->color == NColor::Black && w->right && w->right->color == NColor::Black) {
						w->color = NColor::Red;
						x = x->parent;
					}
					else {
						if (w->right && w->right->color == NColor::Black) {
							if (w->left)
								w->left->color = NColor::Black;
							w->color = NColor::Red;
							rightRotate(w);
							w = x->parent->right;
						}

						w->color = x->parent->color;
						x->parent->color = NColor::Black;
						if (w->right)
							w->right->color = NColor::Black;
						leftRotate(x->parent);
						x = root;
					}
				}
				else {
					w = x->parent->left;

					if (w->color == NColor::Red) {
						w->color = NColor::Black;
						x->parent->color = NColor::Red;
						rightRotate(x->parent);
						w = x->parent->left;
					}
					// todo w->right==0
					if (w->left->color == NColor::Black && w->right && w->right->color == NColor::Black) {
						w->color = NColor::Red;
						x = x->parent;

					}
					else {
						if (w->left->color == NColor::Black) {
							w->right->color = NColor::Black;
							w->color = NColor::Red;
							leftRotate(w);
							w = x->parent->left;
						}

						w->color = x->parent->color;
						x->parent->color = NColor::Black;
						w->left->color = NColor::Black;
						rightRotate(x->parent);
						x = root;
					}
				}
			}
			x->color = NColor::Black;
		}
		resetSentinel();
	}

	void PieceTreeBase::fixInsert(node_tp x)
	{
		recomputeTreeMetadata(x);

		while (x != root && x->parent->color == NColor::Red) {
			if (x->parent == x->parent->parent->left) {
				auto y = x->parent->parent->right;

				if (y && y->color == NColor::Red) {
					x->parent->color = NColor::Black;
					y->color = NColor::Black;
					x->parent->parent->color = NColor::Red;
					x = x->parent->parent;
				}
				else {
					if (x == x->parent->right) {
						x = x->parent;
						leftRotate(x);
					}

					x->parent->color = NColor::Black;
					x->parent->parent->color = NColor::Red;
					rightRotate(x->parent->parent);
				}
			}
			else {
				auto y = x->parent->parent->left;
				// todo y is nullptr
				if (y && y->color == NColor::Red) {
					x->parent->color = NColor::Black;
					y->color = NColor::Black;
					x->parent->parent->color = NColor::Red;
					x = x->parent->parent;
				}
				else {
					if (x == x->parent->left) {
						x = x->parent;
						rightRotate(x);
					}
					x->parent->color = NColor::Black;
					x->parent->parent->color = NColor::Red;
					leftRotate(x->parent->parent);
				}
			}
		}

		root->color = NColor::Black;
	}

	void PieceTreeBase::updateTreeMetadata(node_tp x, size_t delta, size_t lineFeedCntDelta)
	{
		// node length change or line feed count change
		while (x != root && x != SENTINEL) {
			if (x->parent->left == x) {
				x->parent->size_left += delta;
				x->parent->lf_left += lineFeedCntDelta;
			}

			x = x->parent;
		}
	}

	void PieceTreeBase::recomputeTreeMetadata(node_tp x)
	{
		auto delta = 0;
		auto lf_delta = 0;
		if (x == root) {
			return;
		}

		if (delta == 0) {
			// go upwards till the node whose left subtree is changed.
			while (x != root && x == x->parent->right) {
				x = x->parent;
			}

			if (x == root) {
				// well, it means we add a node to the end (inorder)
				return;
			}

			// x is the node whose right subtree is changed.
			x = x->parent;

			delta = calculateSize(x->left) - x->size_left;
			lf_delta = calculateLF(x->left) - x->lf_left;
			x->size_left += delta;
			x->lf_left += lf_delta;
		}

		// go upwards till root. O(logN)
		while (x != root && (delta != 0 || lf_delta != 0)) {
			if (x->parent->left == x) {
				x->parent->size_left += delta;
				x->parent->lf_left += lf_delta;
			}

			x = x->parent;
		}
	}
#endif


#if 1
	enum class CharCode :uint32_t
	{
		Null = 0,
		// The `\b` character.
		Backspace = 8,

		// * The `\t` character.

		Tab = 9,

		// * The `\n` character.

		LineFeed = 10,

		// * The `\r` character.

		CarriageReturn = 13,
		Space = 32,

		// * The `!` character.

		ExclamationMark = 33,

		// * The `"` character.

		DoubleQuote = 34,

		// * The `#` character.

		Hash = 35,

		// * The `$` character.

		DollarSign = 36,

		// * The `%` character.

		PercentSign = 37,

		// * The `&` character.

		Ampersand = 38,

		// * The `'` character.

		SingleQuote = 39,

		// * The `(` character.

		OpenParen = 40,

		// * The `)` character.

		CloseParen = 41,

		// * The `*` character.

		Asterisk = 42,

		// * The `+` character.

		Plus = 43,

		// * The `,` character.

		Comma = 44,

		// * The `-` character.

		Dash = 45,

		// * The `.` character.

		Period = 46,

		// * The `/` character.

		Slash = 47,

		Digit0 = 48,
		Digit1 = 49,
		Digit2 = 50,
		Digit3 = 51,
		Digit4 = 52,
		Digit5 = 53,
		Digit6 = 54,
		Digit7 = 55,
		Digit8 = 56,
		Digit9 = 57,


		// * The `:` character.

		Colon = 58,

		// * The `;` character.

		Semicolon = 59,

		// * The `<` character.

		LessThan = 60,

		// * The `=` character.

		Equals = 61,

		// * The `>` character.

		GreaterThan = 62,

		// * The `?` character.

		QuestionMark = 63,

		// * The `@` character.

		AtSign = 64,

		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 84,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,


		// * The `[` character.

		OpenSquareBracket = 91,

		// * The `\` character.

		Backslash = 92,

		// * The `]` character.

		CloseSquareBracket = 93,

		// * The `^` character.

		Caret = 94,

		// * The `_` character.

		Underline = 95,

		// * The ``(`)`` character.

		BackTick = 96,

		a = 97,
		b = 98,
		c = 99,
		d = 100,
		e = 101,
		f = 102,
		g = 103,
		h = 104,
		i = 105,
		j = 106,
		k = 107,
		l = 108,
		m = 109,
		n = 110,
		o = 111,
		p = 112,
		q = 113,
		r = 114,
		s = 115,
		t = 116,
		u = 117,
		v = 118,
		w = 119,
		x = 120,
		y = 121,
		z = 122,


		// * The `{` character.

		OpenCurlyBrace = 123,

		// * The `|` character.

		Pipe = 124,

		// * The `}` character.

		CloseCurlyBrace = 125,

		// * The `~` character.

		Tilde = 126,

		U_Combining_Grave_Accent = 0x0300,								//	U+0300	Combining Grave Accent
		U_Combining_Acute_Accent = 0x0301,								//	U+0301	Combining Acute Accent
		U_Combining_Circumflex_Accent = 0x0302,							//	U+0302	Combining Circumflex Accent
		U_Combining_Tilde = 0x0303,										//	U+0303	Combining Tilde
		U_Combining_Macron = 0x0304,									//	U+0304	Combining Macron
		U_Combining_Overline = 0x0305,									//	U+0305	Combining Overline
		U_Combining_Breve = 0x0306,										//	U+0306	Combining Breve
		U_Combining_Dot_Above = 0x0307,									//	U+0307	Combining Dot Above
		U_Combining_Diaeresis = 0x0308,									//	U+0308	Combining Diaeresis
		U_Combining_Hook_Above = 0x0309,								//	U+0309	Combining Hook Above
		U_Combining_Ring_Above = 0x030A,								//	U+030A	Combining Ring Above
		U_Combining_Double_Acute_Accent = 0x030B,						//	U+030B	Combining Double Acute Accent
		U_Combining_Caron = 0x030C,										//	U+030C	Combining Caron
		U_Combining_Vertical_Line_Above = 0x030D,						//	U+030D	Combining Vertical Line Above
		U_Combining_Double_Vertical_Line_Above = 0x030E,				//	U+030E	Combining Double Vertical Line Above
		U_Combining_Double_Grave_Accent = 0x030F,						//	U+030F	Combining Double Grave Accent
		U_Combining_Candrabindu = 0x0310,								//	U+0310	Combining Candrabindu
		U_Combining_Inverted_Breve = 0x0311,							//	U+0311	Combining Inverted Breve
		U_Combining_Turned_Comma_Above = 0x0312,						//	U+0312	Combining Turned Comma Above
		U_Combining_Comma_Above = 0x0313,								//	U+0313	Combining Comma Above
		U_Combining_Reversed_Comma_Above = 0x0314,						//	U+0314	Combining Reversed Comma Above
		U_Combining_Comma_Above_Right = 0x0315,							//	U+0315	Combining Comma Above Right
		U_Combining_Grave_Accent_Below = 0x0316,						//	U+0316	Combining Grave Accent Below
		U_Combining_Acute_Accent_Below = 0x0317,						//	U+0317	Combining Acute Accent Below
		U_Combining_Left_Tack_Below = 0x0318,							//	U+0318	Combining Left Tack Below
		U_Combining_Right_Tack_Below = 0x0319,							//	U+0319	Combining Right Tack Below
		U_Combining_Left_Angle_Above = 0x031A,							//	U+031A	Combining Left Angle Above
		U_Combining_Horn = 0x031B,										//	U+031B	Combining Horn
		U_Combining_Left_Half_Ring_Below = 0x031C,						//	U+031C	Combining Left Half Ring Below
		U_Combining_Up_Tack_Below = 0x031D,								//	U+031D	Combining Up Tack Below
		U_Combining_Down_Tack_Below = 0x031E,							//	U+031E	Combining Down Tack Below
		U_Combining_Plus_Sign_Below = 0x031F,							//	U+031F	Combining Plus Sign Below
		U_Combining_Minus_Sign_Below = 0x0320,							//	U+0320	Combining Minus Sign Below
		U_Combining_Palatalized_Hook_Below = 0x0321,					//	U+0321	Combining Palatalized Hook Below
		U_Combining_Retroflex_Hook_Below = 0x0322,						//	U+0322	Combining Retroflex Hook Below
		U_Combining_Dot_Below = 0x0323,									//	U+0323	Combining Dot Below
		U_Combining_Diaeresis_Below = 0x0324,							//	U+0324	Combining Diaeresis Below
		U_Combining_Ring_Below = 0x0325,								//	U+0325	Combining Ring Below
		U_Combining_Comma_Below = 0x0326,								//	U+0326	Combining Comma Below
		U_Combining_Cedilla = 0x0327,									//	U+0327	Combining Cedilla
		U_Combining_Ogonek = 0x0328,									//	U+0328	Combining Ogonek
		U_Combining_Vertical_Line_Below = 0x0329,						//	U+0329	Combining Vertical Line Below
		U_Combining_Bridge_Below = 0x032A,								//	U+032A	Combining Bridge Below
		U_Combining_Inverted_Double_Arch_Below = 0x032B,				//	U+032B	Combining Inverted Double Arch Below
		U_Combining_Caron_Below = 0x032C,								//	U+032C	Combining Caron Below
		U_Combining_Circumflex_Accent_Below = 0x032D,					//	U+032D	Combining Circumflex Accent Below
		U_Combining_Breve_Below = 0x032E,								//	U+032E	Combining Breve Below
		U_Combining_Inverted_Breve_Below = 0x032F,						//	U+032F	Combining Inverted Breve Below
		U_Combining_Tilde_Below = 0x0330,								//	U+0330	Combining Tilde Below
		U_Combining_Macron_Below = 0x0331,								//	U+0331	Combining Macron Below
		U_Combining_Low_Line = 0x0332,									//	U+0332	Combining Low Line
		U_Combining_Double_Low_Line = 0x0333,							//	U+0333	Combining Double Low Line
		U_Combining_Tilde_Overlay = 0x0334,								//	U+0334	Combining Tilde Overlay
		U_Combining_Short_Stroke_Overlay = 0x0335,						//	U+0335	Combining Short Stroke Overlay
		U_Combining_Long_Stroke_Overlay = 0x0336,						//	U+0336	Combining Long Stroke Overlay
		U_Combining_Short_Solidus_Overlay = 0x0337,						//	U+0337	Combining Short Solidus Overlay
		U_Combining_Long_Solidus_Overlay = 0x0338,						//	U+0338	Combining Long Solidus Overlay
		U_Combining_Right_Half_Ring_Below = 0x0339,						//	U+0339	Combining Right Half Ring Below
		U_Combining_Inverted_Bridge_Below = 0x033A,						//	U+033A	Combining Inverted Bridge Below
		U_Combining_Square_Below = 0x033B,								//	U+033B	Combining Square Below
		U_Combining_Seagull_Below = 0x033C,								//	U+033C	Combining Seagull Below
		U_Combining_X_Above = 0x033D,									//	U+033D	Combining X Above
		U_Combining_Vertical_Tilde = 0x033E,							//	U+033E	Combining Vertical Tilde
		U_Combining_Double_Overline = 0x033F,							//	U+033F	Combining Double Overline
		U_Combining_Grave_Tone_Mark = 0x0340,							//	U+0340	Combining Grave Tone Mark
		U_Combining_Acute_Tone_Mark = 0x0341,							//	U+0341	Combining Acute Tone Mark
		U_Combining_Greek_Perispomeni = 0x0342,							//	U+0342	Combining Greek Perispomeni
		U_Combining_Greek_Koronis = 0x0343,								//	U+0343	Combining Greek Koronis
		U_Combining_Greek_Dialytika_Tonos = 0x0344,						//	U+0344	Combining Greek Dialytika Tonos
		U_Combining_Greek_Ypogegrammeni = 0x0345,						//	U+0345	Combining Greek Ypogegrammeni
		U_Combining_Bridge_Above = 0x0346,								//	U+0346	Combining Bridge Above
		U_Combining_Equals_Sign_Below = 0x0347,							//	U+0347	Combining Equals Sign Below
		U_Combining_Double_Vertical_Line_Below = 0x0348,				//	U+0348	Combining Double Vertical Line Below
		U_Combining_Left_Angle_Below = 0x0349,							//	U+0349	Combining Left Angle Below
		U_Combining_Not_Tilde_Above = 0x034A,							//	U+034A	Combining Not Tilde Above
		U_Combining_Homothetic_Above = 0x034B,							//	U+034B	Combining Homothetic Above
		U_Combining_Almost_Equal_To_Above = 0x034C,						//	U+034C	Combining Almost Equal To Above
		U_Combining_Left_Right_Arrow_Below = 0x034D,					//	U+034D	Combining Left Right Arrow Below
		U_Combining_Upwards_Arrow_Below = 0x034E,						//	U+034E	Combining Upwards Arrow Below
		U_Combining_Grapheme_Joiner = 0x034F,							//	U+034F	Combining Grapheme Joiner
		U_Combining_Right_Arrowhead_Above = 0x0350,						//	U+0350	Combining Right Arrowhead Above
		U_Combining_Left_Half_Ring_Above = 0x0351,						//	U+0351	Combining Left Half Ring Above
		U_Combining_Fermata = 0x0352,									//	U+0352	Combining Fermata
		U_Combining_X_Below = 0x0353,									//	U+0353	Combining X Below
		U_Combining_Left_Arrowhead_Below = 0x0354,						//	U+0354	Combining Left Arrowhead Below
		U_Combining_Right_Arrowhead_Below = 0x0355,						//	U+0355	Combining Right Arrowhead Below
		U_Combining_Right_Arrowhead_And_Up_Arrowhead_Below = 0x0356,	//	U+0356	Combining Right Arrowhead And Up Arrowhead Below
		U_Combining_Right_Half_Ring_Above = 0x0357,						//	U+0357	Combining Right Half Ring Above
		U_Combining_Dot_Above_Right = 0x0358,							//	U+0358	Combining Dot Above Right
		U_Combining_Asterisk_Below = 0x0359,							//	U+0359	Combining Asterisk Below
		U_Combining_Double_Ring_Below = 0x035A,							//	U+035A	Combining Double Ring Below
		U_Combining_Zigzag_Above = 0x035B,								//	U+035B	Combining Zigzag Above
		U_Combining_Double_Breve_Below = 0x035C,						//	U+035C	Combining Double Breve Below
		U_Combining_Double_Breve = 0x035D,								//	U+035D	Combining Double Breve
		U_Combining_Double_Macron = 0x035E,								//	U+035E	Combining Double Macron
		U_Combining_Double_Macron_Below = 0x035F,						//	U+035F	Combining Double Macron Below
		U_Combining_Double_Tilde = 0x0360,								//	U+0360	Combining Double Tilde
		U_Combining_Double_Inverted_Breve = 0x0361,						//	U+0361	Combining Double Inverted Breve
		U_Combining_Double_Rightwards_Arrow_Below = 0x0362,				//	U+0362	Combining Double Rightwards Arrow Below
		U_Combining_Latin_Small_Letter_A = 0x0363, 						//	U+0363	Combining Latin Small Letter A
		U_Combining_Latin_Small_Letter_E = 0x0364, 						//	U+0364	Combining Latin Small Letter E
		U_Combining_Latin_Small_Letter_I = 0x0365, 						//	U+0365	Combining Latin Small Letter I
		U_Combining_Latin_Small_Letter_O = 0x0366, 						//	U+0366	Combining Latin Small Letter O
		U_Combining_Latin_Small_Letter_U = 0x0367, 						//	U+0367	Combining Latin Small Letter U
		U_Combining_Latin_Small_Letter_C = 0x0368, 						//	U+0368	Combining Latin Small Letter C
		U_Combining_Latin_Small_Letter_D = 0x0369, 						//	U+0369	Combining Latin Small Letter D
		U_Combining_Latin_Small_Letter_H = 0x036A, 						//	U+036A	Combining Latin Small Letter H
		U_Combining_Latin_Small_Letter_M = 0x036B, 						//	U+036B	Combining Latin Small Letter M
		U_Combining_Latin_Small_Letter_R = 0x036C, 						//	U+036C	Combining Latin Small Letter R
		U_Combining_Latin_Small_Letter_T = 0x036D, 						//	U+036D	Combining Latin Small Letter T
		U_Combining_Latin_Small_Letter_V = 0x036E, 						//	U+036E	Combining Latin Small Letter V
		U_Combining_Latin_Small_Letter_X = 0x036F, 						//	U+036F	Combining Latin Small Letter X

		/*
		 * Unicode Character 'LINE SEPARATOR' (U+2028)
		 * http://www.fileformat.info/info/unicode/char/2028/index.htm
		 */
		LINE_SEPARATOR_2028 = 8232,

		// http://www.fileformat.info/info/unicode/category/Sk/list.htm
		U_CIRCUMFLEX = 0x005E,									// U+005E	CIRCUMFLEX
		U_GRAVE_ACCENT = 0x0060,								// U+0060	GRAVE ACCENT
		U_DIAERESIS = 0x00A8,									// U+00A8	DIAERESIS
		U_MACRON = 0x00AF,										// U+00AF	MACRON
		U_ACUTE_ACCENT = 0x00B4,								// U+00B4	ACUTE ACCENT
		U_CEDILLA = 0x00B8,										// U+00B8	CEDILLA
		U_MODIFIER_LETTER_LEFT_ARROWHEAD = 0x02C2,				// U+02C2	MODIFIER LETTER LEFT ARROWHEAD
		U_MODIFIER_LETTER_RIGHT_ARROWHEAD = 0x02C3,				// U+02C3	MODIFIER LETTER RIGHT ARROWHEAD
		U_MODIFIER_LETTER_UP_ARROWHEAD = 0x02C4,				// U+02C4	MODIFIER LETTER UP ARROWHEAD
		U_MODIFIER_LETTER_DOWN_ARROWHEAD = 0x02C5,				// U+02C5	MODIFIER LETTER DOWN ARROWHEAD
		U_MODIFIER_LETTER_CENTRED_RIGHT_HALF_RING = 0x02D2,		// U+02D2	MODIFIER LETTER CENTRED RIGHT HALF RING
		U_MODIFIER_LETTER_CENTRED_LEFT_HALF_RING = 0x02D3,		// U+02D3	MODIFIER LETTER CENTRED LEFT HALF RING
		U_MODIFIER_LETTER_UP_TACK = 0x02D4,						// U+02D4	MODIFIER LETTER UP TACK
		U_MODIFIER_LETTER_DOWN_TACK = 0x02D5,					// U+02D5	MODIFIER LETTER DOWN TACK
		U_MODIFIER_LETTER_PLUS_SIGN = 0x02D6,					// U+02D6	MODIFIER LETTER PLUS SIGN
		U_MODIFIER_LETTER_MINUS_SIGN = 0x02D7,					// U+02D7	MODIFIER LETTER MINUS SIGN
		U_BREVE = 0x02D8,										// U+02D8	BREVE
		U_DOT_ABOVE = 0x02D9,									// U+02D9	DOT ABOVE
		U_RING_ABOVE = 0x02DA,									// U+02DA	RING ABOVE
		U_OGONEK = 0x02DB,										// U+02DB	OGONEK
		U_SMALL_TILDE = 0x02DC,									// U+02DC	SMALL TILDE
		U_DOUBLE_ACUTE_ACCENT = 0x02DD,							// U+02DD	DOUBLE ACUTE ACCENT
		U_MODIFIER_LETTER_RHOTIC_HOOK = 0x02DE,					// U+02DE	MODIFIER LETTER RHOTIC HOOK
		U_MODIFIER_LETTER_CROSS_ACCENT = 0x02DF,				// U+02DF	MODIFIER LETTER CROSS ACCENT
		U_MODIFIER_LETTER_EXTRA_HIGH_TONE_BAR = 0x02E5,			// U+02E5	MODIFIER LETTER EXTRA-HIGH TONE BAR
		U_MODIFIER_LETTER_HIGH_TONE_BAR = 0x02E6,				// U+02E6	MODIFIER LETTER HIGH TONE BAR
		U_MODIFIER_LETTER_MID_TONE_BAR = 0x02E7,				// U+02E7	MODIFIER LETTER MID TONE BAR
		U_MODIFIER_LETTER_LOW_TONE_BAR = 0x02E8,				// U+02E8	MODIFIER LETTER LOW TONE BAR
		U_MODIFIER_LETTER_EXTRA_LOW_TONE_BAR = 0x02E9,			// U+02E9	MODIFIER LETTER EXTRA-LOW TONE BAR
		U_MODIFIER_LETTER_YIN_DEPARTING_TONE_MARK = 0x02EA,		// U+02EA	MODIFIER LETTER YIN DEPARTING TONE MARK
		U_MODIFIER_LETTER_YANG_DEPARTING_TONE_MARK = 0x02EB,	// U+02EB	MODIFIER LETTER YANG DEPARTING TONE MARK
		U_MODIFIER_LETTER_UNASPIRATED = 0x02ED,					// U+02ED	MODIFIER LETTER UNASPIRATED
		U_MODIFIER_LETTER_LOW_DOWN_ARROWHEAD = 0x02EF,			// U+02EF	MODIFIER LETTER LOW DOWN ARROWHEAD
		U_MODIFIER_LETTER_LOW_UP_ARROWHEAD = 0x02F0,			// U+02F0	MODIFIER LETTER LOW UP ARROWHEAD
		U_MODIFIER_LETTER_LOW_LEFT_ARROWHEAD = 0x02F1,			// U+02F1	MODIFIER LETTER LOW LEFT ARROWHEAD
		U_MODIFIER_LETTER_LOW_RIGHT_ARROWHEAD = 0x02F2,			// U+02F2	MODIFIER LETTER LOW RIGHT ARROWHEAD
		U_MODIFIER_LETTER_LOW_RING = 0x02F3,					// U+02F3	MODIFIER LETTER LOW RING
		U_MODIFIER_LETTER_MIDDLE_GRAVE_ACCENT = 0x02F4,			// U+02F4	MODIFIER LETTER MIDDLE GRAVE ACCENT
		U_MODIFIER_LETTER_MIDDLE_DOUBLE_GRAVE_ACCENT = 0x02F5,	// U+02F5	MODIFIER LETTER MIDDLE DOUBLE GRAVE ACCENT
		U_MODIFIER_LETTER_MIDDLE_DOUBLE_ACUTE_ACCENT = 0x02F6,	// U+02F6	MODIFIER LETTER MIDDLE DOUBLE ACUTE ACCENT
		U_MODIFIER_LETTER_LOW_TILDE = 0x02F7,					// U+02F7	MODIFIER LETTER LOW TILDE
		U_MODIFIER_LETTER_RAISED_COLON = 0x02F8,				// U+02F8	MODIFIER LETTER RAISED COLON
		U_MODIFIER_LETTER_BEGIN_HIGH_TONE = 0x02F9,				// U+02F9	MODIFIER LETTER BEGIN HIGH TONE
		U_MODIFIER_LETTER_END_HIGH_TONE = 0x02FA,				// U+02FA	MODIFIER LETTER END HIGH TONE
		U_MODIFIER_LETTER_BEGIN_LOW_TONE = 0x02FB,				// U+02FB	MODIFIER LETTER BEGIN LOW TONE
		U_MODIFIER_LETTER_END_LOW_TONE = 0x02FC,				// U+02FC	MODIFIER LETTER END LOW TONE
		U_MODIFIER_LETTER_SHELF = 0x02FD,						// U+02FD	MODIFIER LETTER SHELF
		U_MODIFIER_LETTER_OPEN_SHELF = 0x02FE,					// U+02FE	MODIFIER LETTER OPEN SHELF
		U_MODIFIER_LETTER_LOW_LEFT_ARROW = 0x02FF,				// U+02FF	MODIFIER LETTER LOW LEFT ARROW
		U_GREEK_LOWER_NUMERAL_SIGN = 0x0375,					// U+0375	GREEK LOWER NUMERAL SIGN
		U_GREEK_TONOS = 0x0384,									// U+0384	GREEK TONOS
		U_GREEK_DIALYTIKA_TONOS = 0x0385,						// U+0385	GREEK DIALYTIKA TONOS
		U_GREEK_KORONIS = 0x1FBD,								// U+1FBD	GREEK KORONIS
		U_GREEK_PSILI = 0x1FBF,									// U+1FBF	GREEK PSILI
		U_GREEK_PERISPOMENI = 0x1FC0,							// U+1FC0	GREEK PERISPOMENI
		U_GREEK_DIALYTIKA_AND_PERISPOMENI = 0x1FC1,				// U+1FC1	GREEK DIALYTIKA AND PERISPOMENI
		U_GREEK_PSILI_AND_VARIA = 0x1FCD,						// U+1FCD	GREEK PSILI AND VARIA
		U_GREEK_PSILI_AND_OXIA = 0x1FCE,						// U+1FCE	GREEK PSILI AND OXIA
		U_GREEK_PSILI_AND_PERISPOMENI = 0x1FCF,					// U+1FCF	GREEK PSILI AND PERISPOMENI
		U_GREEK_DASIA_AND_VARIA = 0x1FDD,						// U+1FDD	GREEK DASIA AND VARIA
		U_GREEK_DASIA_AND_OXIA = 0x1FDE,						// U+1FDE	GREEK DASIA AND OXIA
		U_GREEK_DASIA_AND_PERISPOMENI = 0x1FDF,					// U+1FDF	GREEK DASIA AND PERISPOMENI
		U_GREEK_DIALYTIKA_AND_VARIA = 0x1FED,					// U+1FED	GREEK DIALYTIKA AND VARIA
		U_GREEK_DIALYTIKA_AND_OXIA = 0x1FEE,					// U+1FEE	GREEK DIALYTIKA AND OXIA
		U_GREEK_VARIA = 0x1FEF,									// U+1FEF	GREEK VARIA
		U_GREEK_OXIA = 0x1FFD,									// U+1FFD	GREEK OXIA
		U_GREEK_DASIA = 0x1FFE,									// U+1FFE	GREEK DASIA


		U_OVERLINE = 0x203E, // Unicode Character 'OVERLINE'

		/*
		 * UTF-8 BOM
		 * Unicode Character 'ZERO WIDTH NO-BREAK SPACE' (U+FEFF)
		 * http://www.fileformat.info/info/unicode/char/feff/index.htm
		 */
		UTF8_BOM = 65279
	};
	struct LineStarts {

		std::vector<size_t> lineStarts;
		char cr;
		char lf;
		char crlf;
		bool isBasicASCII;
		LineStarts() {}
		~LineStarts() {}
	};

	std::vector<size_t> createLineStartsFast(const std::string& str, bool readonly = true) {
		std::vector<size_t> r;
		r.reserve(str.size());
		r.push_back(0);
		size_t rLength = 1;
		auto t = str.c_str();
		auto ct = md::get_utf8_count(t, str.size());
		unsigned int cp = 0;
		for (size_t i = 0; i < ct; i++) {
			t = md::get_u8_last(t, &cp);
			CharCode chr = (CharCode)cp;// str.at(i);

			if (chr == CharCode::CarriageReturn) {
				t = md::get_u8_last(t, &cp);// str.at(i + 1)
				if (i + 1 < ct && (CharCode)cp == CharCode::LineFeed) {
					// \r\n... case
					r.push_back(i + 2);
					i++; // skip \n
				}
				else {
					// \r... case
					r.push_back(i + 1);
				}
			}
			else if (chr == CharCode::LineFeed) {
				r.push_back(i + 1);
			}
		}
		//if (readonly) {
		//	return createUintArray(r);
		//}
		return r;
	}

	LineStarts  createLineStarts(std::vector<size_t>& r, const std::string& str)
	{
		//r.length = 0;
		//r[0] = 0;
		let rLength = 1;
		let cr = 0, lf = 0, crlf = 0;
		let isBasicASCII = true;
		auto t = str.c_str();
		auto ct = md::get_utf8_count(t, str.size());
		unsigned int cp = 0;
		for (size_t i = 0; i < ct; i++) {
			t = md::get_u8_last(t, &cp);
			CharCode chr = (CharCode)cp;// str.at(i);

			if (chr == CharCode::CarriageReturn) {
				t = md::get_u8_last(t, &cp);// str.at(i + 1)
				if (i + 1 < ct && (CharCode)cp == CharCode::LineFeed) {
					// \r\n... case
					crlf++;
					r[rLength++] = i + 2;
					i++; // skip \n
				}
				else {
					cr++;
					// \r... case
					r[rLength++] = i + 1;
				}
			}
			else if (chr == CharCode::LineFeed) {
				lf++;
				r[rLength++] = i + 1;
			}
			else {
				if (isBasicASCII) {
					if (chr != CharCode::Tab && (chr < (CharCode)32 || chr >(CharCode)126)) {
						isBasicASCII = false;
					}
				}
			}
		}
		LineStarts result; result.lineStarts = r;  result.cr = cr, result.lf = lf, result.crlf = crlf, result.isBasicASCII = isBasicASCII;
		//r.length = 0;

		return result;
	}

#if 1

	struct CacheEntry {
		node_tp node = 0;
		int nodeStartOffset = 0;
		int	nodeStartLineNumber = 0;
	};

	class PieceTreeSearchCache
	{
	private:
		int _limit = 0;//readonly  : number;

		std::list<CacheEntry> _cache;
	public:
		PieceTreeSearchCache() {
			this->_limit = 1;
			this->_cache;
		}
		PieceTreeSearchCache(int limit) {
			this->_limit = limit;
			this->_cache;
		}
		~PieceTreeSearchCache() {}

		void set_limit(int limit) {
			this->_limit = limit;
		}
		CacheEntry* get(int offset)
		{
			for (auto it = _cache.rbegin(); it != _cache.rend(); it++) {
				let nodePos = &(*it);
				if (nodePos->nodeStartOffset <= offset && nodePos->nodeStartOffset + nodePos->node->piece.length >= offset) {
					return nodePos;
				}
			}
			return nullptr;
		}

		CacheEntry* get2(int lineNumber)// : { node: TreeNode, nodeStartOffset : number, nodeStartLineNumber : number } | null{
		{
			for (auto it = _cache.rbegin(); it != _cache.rend(); it++) {
				let nodePos = &(*it);
				if (nodePos->nodeStartLineNumber && nodePos->nodeStartLineNumber < lineNumber && nodePos->nodeStartLineNumber + nodePos->node->piece.lineFeedCnt >= lineNumber) {
					//return < { node: TreeNode, nodeStartOffset : number, nodeStartLineNumber : number } > nodePos;
					return nodePos;
				}
			}
			return nullptr;
		}

		void set(CacheEntry nodePosition) {
			if (this->_cache.size() >= this->_limit) {
				this->_cache.pop_front();//shift();
			}
			this->_cache.push_back(nodePosition);
		}
		void set(NodePosition nodePosition) {
			set({ nodePosition.node, (int)nodePosition.nodeStartOffset, (int)nodePosition.remainder });
		}

		int validate(int offset) {
			let hasInvalidVal = 0;
			//let tmp = this->_cache;
			//size_t ts = tmp.size();
			for (auto it = _cache.begin(); it != _cache.end(); ) {
				let nodePos = &(*it);
				if (nodePos->node->parent == nullptr || nodePos->nodeStartOffset >= offset) {
					//nodePos->node = nullptr;
					hasInvalidVal++;
					_cache.erase(it++);
					continue;
				}
				else
				{
					it++;
				}
			}
			return hasInvalidVal;
		}
	};
#endif // 1

	PieceTree::PieceTree()
	{
		_buffers.push_back({});
		_searchCache = new PieceTreeSearchCache();
	}

	PieceTree::~PieceTree()
	{
		if (_searchCache)
			delete _searchCache;
		_searchCache = 0;
		for (auto p : _gf) {
			delete p;
		}
	}


	//export class PieceTreeBase {
	//	root: node_tp;
	//	protected _buffers: StringBuffer[]; // 0 is change buffer, others are readonly original buffer.
	//	protected _lineCnt: number;
	//	protected _length: number;
	//	protected _EOL: string;
	//	protected _EOLLength: number;
	//	protected _EOLNormalized: boolean;
	//	private _lastChangeBufferPos: BufferCursor;
	//	private _searchCache: PieceTreeSearchCache;
	//	private _lastVisitedLine: { lineNumber: number; value: string; };
	//
	//	constructor(chunks: StringBuffer[], eol: '\r\n' | '\n', eolNormalized: boolean) {
	//		this->create(chunks, eol, eolNormalized);
	//	}
	//: StringBuffer[]
	void PieceTree::init(const std::string& chunks, const std::string& eol/*: '\r\n' | '\n'*/, bool eolNormalized)
	{
		init(chunks.c_str(), chunks.size(), eol, eolNormalized);
	}
	void PieceTree::init(const char* chunks, int len, const std::string& eol/*: '\r\n' | '\n'*/, bool eolNormalized)
	{
		//this->_lastChangeBufferPos = {  0, column: 0 };
		//root = SENTINEL;
		this->_lineCnt = 1;
		//this->_length = 0;
		this->_EOL = eol;
		this->_EOLLength = eol.length();
		this->_EOLNormalized = eolNormalized;

		//node_tp lastNode = nullptr;
		//auto it = chunks.begin();
		//for (size_t i = 0, len = chunks.size(); i < len; i++) {
		//	if (it->buffer.length() > 0) {
		//		//if (!it->lineStarts) {
		//		//	it->lineStarts = createLineStartsFast(it->buffer);
		//		//}
		//		// todo piece_t
		//		piece_t piece = { &(*it),
		//			 {  0,  0 },
		//			{  it->lineStarts.size() - 1,  it->buffer.size() - it->lineStarts[it->lineStarts.size() - 1] },
		//			it->buffer.length(),
		//			it->lineStarts.size() - 1,

		//		};
		//		//this->_buffers.push(chunks[i]);
		//		lastNode = this->rbInsertRight(lastNode, piece);
		//	}
		//}
		std::string dstr;
		if (chunks && len > 0)
			dstr.assign(chunks, len);
		remove_v(0, _length);
		//_buffers.clear();
		//_buffers.push_back({});
		//rbDelete(root);
		insert_v(0, dstr);
		_searchCache->set_limit(1);
		this->_lastVisitedLine = {};
		this->computeBufferMetadata();
	}

	void PieceTree::normalizeEOL(const std::string& eol/*: '\r\n' | '\n'*/)
	{
		let averageBufferSize = AverageBufferSize;
		let mint = averageBufferSize - floor(averageBufferSize / 3);
		let maxt = mint * 2;

		std::string tempChunk;
		let tempChunkLen = 0;
		//let chunks : StringBuffer[] = [];
		std::string chunks;
		this->iterate(root, [&](node_tp node)
			{
				let str = this->getNodeContent(node);
				let len = str.size();
				if (tempChunkLen <= mint || tempChunkLen + len < maxt) {
					tempChunk += str;
					tempChunkLen += len;
					return true;
				}

				// flush anyways
				let text = tempChunk;// .replace("/\r\n|\r|\n/g", eol);
				chunks.append(text);// new StringBuffer(text, createLineStartsFast(text)));
				tempChunk = str;
				tempChunkLen = len;
				return true;
			});

		if (tempChunkLen > 0) {
			let text = tempChunk;// .replace("/\r\n|\r|\n/g", eol);
			chunks.append(text);//push(new StringBuffer(text, createLineStartsFast(text)));
		}

		this->init(chunks, eol, true);
	}

	// #region Buffer API
	std::string PieceTree::getEOL() {
		return this->_EOL;
	}

	void PieceTree::setEOL(const std::string& newEOL /*: '\r\n' | '\n'*/)
	{
		this->_EOL = newEOL;
		this->_EOLLength = this->_EOL.length();
		this->normalizeEOL(newEOL);
	}

	// ITextSnapshot createSnapshot(BOM: string) {
	//	return new PieceTreeSnapshot(this, BOM);
	//}

	bool PieceTree::equal(PieceTree& other) {
		if (this->getLength() != other.getLength()) {
			return false;
		}
		if (this->getLineCount() != other.getLineCount()) {
			return false;
		}

		let offset = 0;
		let ret = this->iterate(root, [&](node_tp node)
			{
				if (node == SENTINEL) {
					return true;
				}
				let str = this->getNodeContent(node);
				let len = md::get_utf8_count(str.c_str(), str.size());
				// len=str.size()
				let startPosition = other.nodeAt(offset);
				let endPosition = other.nodeAt(offset + len);
				let val = other.getValueInRange2(startPosition, endPosition);

				return str == val;
			});

		return ret;
	}

	size_t PieceTree::getOffsetAt(const glm::ivec2& pos) {
		return getOffsetAt(pos.y, pos.x);
	}
	size_t PieceTree::getOffsetAt(size_t lineNumber, size_t column) {
		let leftLen = 0; // inorder

		let x = root;

		while (x != SENTINEL) {
			if (x->left != SENTINEL && x->lf_left + 1 >= lineNumber) {
				x = x->left;
			}
			else if (x->lf_left + x->piece.lineFeedCnt + 1 >= lineNumber) {
				leftLen += x->size_left;
				// lineNumber >= 2
				let accumualtedValInCurrentIndex = this->getAccumulatedValue(x, lineNumber - x->lf_left - 2);
				leftLen += accumualtedValInCurrentIndex + column - 1;
				break;
			}
			else {
				lineNumber -= x->lf_left + x->piece.lineFeedCnt;
				leftLen += x->size_left + x->piece.length;
				x = x->right;
			}
		}

		return leftLen;
	}

	Position  PieceTree::getPositionAt(size_t offset)
	{
		offset = floor(offset);
		offset = std::max((size_t)0, offset);

		let x = root;
		let lfCnt = 0;
		let originalOffset = offset;

		while (x != SENTINEL) {
			if (x->size_left != 0 && x->size_left >= offset) {
				x = x->left;
			}
			else if (x->size_left + x->piece.length >= offset) {
				let out = this->getIndexOf(x, offset - x->size_left);

				lfCnt += x->lf_left + out.y;

				if (out.y == 0) {
					let lineStartOffset = this->getOffsetAt(lfCnt + 1, 1);
					let column = originalOffset - lineStartOffset;
					return Position(lfCnt + 1, column + 1);
				}

				return Position(lfCnt + 1, out.x + 1);
			}
			else {
				offset -= x->size_left + x->piece.length;
				lfCnt += x->lf_left + x->piece.lineFeedCnt;

				if (x->right == SENTINEL) {
					// last node
					let lineStartOffset = this->getOffsetAt(lfCnt + 1, 1);
					let column = originalOffset - offset - lineStartOffset;
					return Position(lfCnt + 1, column + 1);
				}
				else {
					x = x->right;
				}
			}
		}

		return Position(1, 1);
	}

	std::string PieceTree::getValueInRange(Range range, const std::string& eol) {
		if (range.y == range.w && range.x == range.z) {
			return "";
		}

		//let startPosition = this->nodeAt2(range.startLineNumber, range.startColumn);
		//let endPosition = this->nodeAt2(range.endLineNumber, range.endColumn);
		let startPosition = this->nodeAt2(range.y, range.x);
		let endPosition = this->nodeAt2(range.w, range.z);

		let value = this->getValueInRange2(startPosition, endPosition);
		if (eol.size()) {
			// todo getValueInRange
			//if (eol != this->_EOL || !this->_EOLNormalized) {
			//	return value.replace("/\r\n|\r|\n/g", eol);
			//}

			//if (eol == this->getEOL() && this->_EOLNormalized) {
			//	if (eol == "\r\n") {

			//	}
			//	return value;
			//}
			//return value.replace("/\r\n|\r|\n/g", eol);
		}
		return value;
	}
	std::string PieceTree::get_range(glm::ivec2 first, glm::ivec2 second)
	{
		if (first > second)
		{
			std::swap(first, second);
		}
		let startPosition = this->nodeAt2(first.y, first.x);
		let endPosition = this->nodeAt2(second.y, second.x);
		std::string ret;
		if (startPosition.node && endPosition.node)
		{
			ret = this->getValueInRange2(startPosition, endPosition);
		}
		return ret;
	}
	std::string PieceTree::getValueInRange2(NodePosition startPosition, NodePosition endPosition) {
		if (startPosition.node == endPosition.node) {
			let node = startPosition.node;
			let& buffer = node->piece._bt->buffer;//this->_buffers[node->piece.bufferIndex].buffer;
			let startOffset = this->offsetInBuffer(node->piece._bt, node->piece._start);
			return get_sub_string(node->piece._bt, startOffset + startPosition.remainder, /*startOffset +*/ endPosition.remainder - startPosition.remainder);
			//return buffer.substr(startOffset + startPosition.remainder, /*startOffset +*/ endPosition.remainder - startPosition.remainder);
		}

		let x = startPosition.node;
		let& buffer = x->piece._bt->buffer;// this->_buffers[x->piece._bt].buffer;
		let startOffset = this->offsetInBuffer(x->piece._bt, x->piece._start);
		let ret = //buffer.substr(startOffset + startPosition.remainder,/* startOffset +*/ x->piece.length - startPosition.remainder);
			get_sub_string(x->piece._bt, startOffset + startPosition.remainder,/* startOffset +*/ x->piece.length - startPosition.remainder);
		x = x->next();
		while (x != SENTINEL) {
			let& buffer = x->piece._bt->buffer;// this->_buffers[x->piece._bt].buffer;
			let startOffset = this->offsetInBuffer(x->piece._bt, x->piece._start);

			if (x == endPosition.node) {
				get_sub_string(x->piece._bt, startOffset, /*startOffset +*/ endPosition.remainder, ret);
				//ret += buffer.substr(startOffset, /*startOffset +*/ endPosition.remainder);
				break;
			}
			else {
				get_sub_string(x->piece._bt, startOffset, x->piece.length, ret);
				//ret += buffer.substr(startOffset, x->piece.length);
			}

			x = x->next();
		}

		return ret;
	}

	std::vector<std::string> PieceTree::getLinesContent()
	{
		return { this->getContentOfSubTree(root) };// .split("/\r\n|\r|\n/");
	}

	size_t PieceTree::get_count() {
		return this->_count;
	}
	size_t PieceTree::getLength() {
		return this->_length;
	}

	size_t PieceTree::getLineCount() {
		return this->_lineCnt;
	}

	/**
	 * @param lineNumber 1 based
	 */
	std::string PieceTree::getLineContent(size_t lineNumber) {
		if (this->_lastVisitedLine.lineNumber == lineNumber) {
			return this->_lastVisitedLine.value;
		}

		this->_lastVisitedLine.lineNumber = lineNumber;

		if (lineNumber == this->_lineCnt) {
			this->_lastVisitedLine.value = this->getLineRawContent(lineNumber);
		}
		else if (this->_EOLNormalized) {
			this->_lastVisitedLine.value = this->getLineRawContent(lineNumber, this->_EOLLength);
		}
		else {
			this->_lastVisitedLine.value = this->getLineRawContent(lineNumber);// .replace(/ (\r\n | \r | \n)$ / , "");
		}

		return this->_lastVisitedLine.value;
	}

	size_t PieceTree::getLineCharCode(size_t lineNumber, size_t index) {
		let nodePos = this->nodeAt2(lineNumber, index + 1);
		if (nodePos.remainder == nodePos.node->piece.length) {
			// the char we want to fetch is at the head of next node.
			let matchingNode = nodePos.node->next();
			if (!matchingNode) {
				return 0;
			}

			let& buffer = matchingNode->piece._bt->buffer;//this->_buffers[matchingNode->piece._bt];
			let startOffset = this->offsetInBuffer(matchingNode->piece._bt, matchingNode->piece._start);
			//return buffer.at(startOffset);
			return md::get_u8_idx(buffer.c_str(), startOffset);
		}
		else {
			let& buffer = nodePos.node->piece._bt->buffer;// this->_buffers[nodePos.node->piece._bt];
			let startOffset = this->offsetInBuffer(nodePos.node->piece._bt, nodePos.node->piece._start);
			let targetOffset = startOffset + nodePos.remainder;

			//return buffer.at(targetOffset);
			return md::get_u8_idx(buffer.c_str(), targetOffset);
		}
	}

	size_t PieceTree::getLineLength(size_t lineNumber) {
		if (lineNumber == this->getLineCount()) {
			let startOffset = this->getOffsetAt(lineNumber, 1);
			return this->getLength() - startOffset;
		}
		return this->getOffsetAt(lineNumber + 1, 1) - this->getOffsetAt(lineNumber, 1) - this->_EOLLength;
	}

	// #endregion

	// #region Piece Table
	size_t PieceTree::insert_v(size_t offset, std::string value, bool eolNormalized)
	{
		size_t ret = offset + md::get_utf8_count(value.c_str(), value.size());// get_uvalue.size();
		this->_EOLNormalized = this->_EOLNormalized && eolNormalized;
#if 1
		this->_lastVisitedLine.lineNumber = 0;
		this->_lastVisitedLine.value = "";

		if (root != SENTINEL) {
			auto an = this->nodeAt(offset);
			if (!an.node)
			{
				an = this->nodeAt(offset);
				return ret;
			}
			let piece = an.node->piece;
			let _bt = piece._bt;
			let insertPosInBuffer = this->positionInBuffer(an.node, an.remainder);
			if (an.node->piece._bt->buffer.empty() &&
				piece._end.y == this->_lastChangeBufferPos.y &&
				piece._end.x == this->_lastChangeBufferPos.x &&
				(an.nodeStartOffset + piece.length == offset) &&
				value.size() < AverageBufferSize
				) {
				// changed buffer
				this->appendToNode(an.node, value);
				this->computeBufferMetadata();
				return ret;
			}

			if (an.nodeStartOffset == offset) {
				this->insertContentToNodeLeft(value, an.node);
				_searchCache->validate(offset);
			}
			else if (an.nodeStartOffset + an.node->piece.length > offset) {
				// we are inserting into the middle of a node.
				std::vector<node_tp> nodesToDel;
				// todo piece_t
				let newRightPiece = piece_t(
					piece._bt,
					insertPosInBuffer,
					piece._end,
					this->offsetInBuffer(_bt, piece._end) - this->offsetInBuffer(_bt, insertPosInBuffer),

					this->getLineFeedCnt(piece._bt, insertPosInBuffer, piece._end)
				);

				if (this->shouldCheckCRLF() && this->endWithCR(value)) {
					let headOfRight = this->nodeCharCodeAt(an.node, an.remainder);

					if (headOfRight == 10 /** \n */) {
						glm::ivec2 newStart = { 0, newRightPiece._start.y + 1 };
						newRightPiece = piece_t{
							newRightPiece._bt, newStart,
							newRightPiece._end,
							(size_t)newRightPiece.length - 1,

							this->getLineFeedCnt(newRightPiece._bt, newStart, newRightPiece._end),

						};

						value.push_back('\n');
					}
				}

				// reuse node for content before insertion point.
				if (this->shouldCheckCRLF() && this->startWithLF(value)) {
					let tailOfLeft = this->nodeCharCodeAt(an.node, an.remainder - 1);
					if (tailOfLeft == 13 /** \r */) {
						let previousPos = this->positionInBuffer(an.node, an.remainder - 1);
						this->deleteNodeTail(an.node, previousPos);
						value.insert(value.begin(), '\r');

						if (an.node->piece.length == 0) {
							nodesToDel.push_back(an.node);
						}
					}
					else {
						this->deleteNodeTail(an.node, insertPosInBuffer);
					}
				}
				else {
					this->deleteNodeTail(an.node, insertPosInBuffer);
				}

				let newPieces = this->createNewPieces(value);
				if (newRightPiece.length > 0) {
					this->rbInsertRight(an.node, newRightPiece);
				}

				let tmpNode = an.node;
				for (let k = 0; k < newPieces.size(); k++) {
					tmpNode = this->rbInsertRight(tmpNode, newPieces[k]);
				}
				this->deleteNodes(nodesToDel);
			}
			else {
				this->insertContentToNodeRight(value, an.node);
			}
		}
		else {
			// insert new node
			let pieces = this->createNewPieces(value);
			let node = this->rbInsertLeft(0, pieces[0]);

			for (let k = 1; k < pieces.size(); k++) {
				node = this->rbInsertRight(node, pieces[k]);
			}
		}
#endif
		// todo, this is too brutal. Total line feed count should be updated the same way as lf_left.
		this->computeBufferMetadata();
		return ret;
	}

	void PieceTree::remove_v(size_t offset, size_t cnt)
	{
		this->_lastVisitedLine.lineNumber = 0;
		this->_lastVisitedLine.value = "";

		if (cnt <= 0 || root == SENTINEL) {
			return;
		}

		let startPosition = this->nodeAt(offset);
		let endPosition = this->nodeAt(offset + cnt);
		let startNode = startPosition.node;
		let endNode = endPosition.node;

		if (startNode && startNode == endNode) {
			let startSplitPosInBuffer = this->positionInBuffer(startNode, startPosition.remainder);
			let endSplitPosInBuffer = this->positionInBuffer(startNode, endPosition.remainder);

			if (startPosition.nodeStartOffset == offset) {
				if (cnt == startNode->piece.length) { // delete node
					let next = startNode->next();
					rbDelete(startNode);

					this->validateCRLFWithPrevNode(next);
					this->computeBufferMetadata();
					return;
				}
				this->deleteNodeHead(startNode, endSplitPosInBuffer);
				_searchCache->validate(offset);
				this->validateCRLFWithPrevNode(startNode);
				this->computeBufferMetadata();
				return;
			}

			if (startPosition.nodeStartOffset + startNode->piece.length == offset + cnt) {
				this->deleteNodeTail(startNode, startSplitPosInBuffer);
				this->validateCRLFWithNextNode(startNode);
				this->computeBufferMetadata();
				return;
			}

			// delete content in the middle, this node will be splitted to nodes
			this->shrinkNode(startNode, startSplitPosInBuffer, endSplitPosInBuffer);
			this->computeBufferMetadata();
			return;
		}
		if (startNode)
		{
			std::vector<node_tp> nodesToDel;

			let startSplitPosInBuffer = this->positionInBuffer(startNode, startPosition.remainder);
			this->deleteNodeTail(startNode, startSplitPosInBuffer);
			_searchCache->validate(offset);
			if (startNode->piece.length == 0) {
				nodesToDel.push_back(startNode);
			}

			// update last touched node
			if (endNode)
			{
				let endSplitPosInBuffer = this->positionInBuffer(endNode, endPosition.remainder);
				this->deleteNodeHead(endNode, endSplitPosInBuffer);
				if (endNode->piece.length == 0) {
					nodesToDel.push_back(endNode);
				}
			}
			else {
				printf("not end node!\n");
			}

			// delete nodes in between
			let secondNode = startNode->next();
			for (let node = secondNode; node != SENTINEL && node != endNode; node = node->next()) {
				nodesToDel.push_back(node);
			}

			let prev = startNode->piece.length == 0 ? startNode->prev() : startNode;
			this->deleteNodes(nodesToDel);
			this->validateCRLFWithNextNode(prev);
			this->computeBufferMetadata();
		}
	}

	void PieceTree::insertContentToNodeLeft(std::string value, node_tp node) {
		// we are inserting content to the beginning of node
		std::vector<node_tp> nodesToDel;
		if (this->shouldCheckCRLF() && this->endWithCR(value) && this->startWithLF(node)) {
			// move `\n` to new node.

			let piece = node->piece;
			glm::ivec2 newStart = { 0, piece._start.y + 1 };
			let cnt = this->getLineFeedCnt(piece._bt, newStart, piece._end);
			// todo piece_t
			let nPiece = piece_t{
				piece._bt, newStart,
				piece._end,
				(size_t)piece.length - 1,

				cnt,
			};

			node->piece = nPiece;

			value.push_back('\n');
			updateTreeMetadata(node, -1, -1);

			if (node->piece.length == 0) {
				nodesToDel.push_back(node);
			}
		}

		let newPieces = this->createNewPieces(value);
		let newNode = this->rbInsertLeft(node, newPieces[newPieces.size() - 1]);
		for (int64_t k = newPieces.size() - 2; k >= 0; k--) {
			newNode = this->rbInsertLeft(newNode, newPieces[k]);
		}
		this->validateCRLFWithPrevNode(newNode);
		this->deleteNodes(nodesToDel);
	}


	void PieceTree::insertContentToNodeRight(std::string value, node_tp node) {
		// we are inserting to the right of this node.
		if (this->adjustCarriageReturnFromNext(value, node)) {
			// move \n to the new node.
			value.push_back('\n');
		}

		let newPieces = this->createNewPieces(value);
		let newNode = this->rbInsertRight(node, newPieces[0]);
		let tmpNode = newNode;

		for (let k = 1; k < newPieces.size(); k++) {
			tmpNode = this->rbInsertRight(tmpNode, newPieces[k]);
		}

		this->validateCRLFWithPrevNode(newNode);
	}

	//positionInBuffer(node_tp node, remainder : number) : BufferCursor;
	//positionInBuffer(node_tp node, remainder : number, ret : BufferCursor) : null;
	glm::ivec2 PieceTree::positionInBuffer(node_tp node, size_t remainder, glm::ivec2* ret)
	{
		let piece = node->piece;
		let bufferIndex = node->piece._bt;
		let& lineStarts = node->piece._bt->lineStarts;// this->_buffers[bufferIndex].lineStarts;
		if (lineStarts.empty())
			return glm::ivec2{ 0, 0 };
		let startOffset = lineStarts[piece._start.y] + piece._start.x;

		let offset = startOffset + remainder;

		// binary search offset between startOffset and endOffset
		let low = piece._start.y;
		let high = piece._end.y;

		int mid = 0;
		int midStop = 0;
		int midStart = 0;

		while (low <= high) {
			mid = low + ((high - low) / 2) | 0;
			midStart = lineStarts[mid];

			if (mid == high) {
				break;
			}

			midStop = lineStarts[mid + 1];

			if (offset < midStart) {
				high = mid - 1;
			}
			else if (offset >= midStop) {
				low = mid + 1;
			}
			else {
				break;
			}
		}
		if (ret) {
			ret->y = mid;
			ret->x = offset - midStart;
		}

		return { offset - midStart, mid };
	}
	// todo glfc
	size_t PieceTree::getLineFeedCnt(block_it bt, glm::ivec2 start, glm::ivec2 end)
	{
		auto it = &(*bt);
		return getLineFeedCnt(it, start, end);
	}
	size_t PieceTree::getLineFeedCnt(block_t* bt, glm::ivec2 start, glm::ivec2 end)
	{
		// we don't need to worry about start: abc\r|\n, or abc|\r, or abc|\n, or abc|\r\n doesn't change the fact that, there is one line break after start.
		// now let's take care of end: abc\r|\n, if end is in between \r and \n, we need to add line feed count by 1
		if (end.x == 0) {
			return end.y - start.y;
		}

		let& lineStarts = bt->lineStarts;// this->_buffers[bufferIndex].lineStarts;
		if (end.y == lineStarts.size() - 1) { // it means, there is no \n after end, otherwise, there will be one more lineStart.
			return end.y - start.y;
		}

		let nextLineStartOffset = lineStarts[end.y + 1];
		let endOffset = lineStarts[end.y] + end.x;
		if (nextLineStartOffset > endOffset + 1) { // there are more than 1 character after end, which means it can't be \n
			return end.y - start.y;
		}
		// endOffset + 1 === nextLineStartOffset
		// character at endOffset is \n, so we check the character before first
		// if character at endOffset is \r, end.x is 0 and we can't get here.
		let previousCharOffset = endOffset - 1; // end.x > 0 so it's okay.
		//let buffer = this->_buffers[bufferIndex].buffer;
		let& buffer = bt->buffer;

		//if (buffer.at(previousCharOffset) == 13) {
		if (md::get_u8_idx(buffer.c_str(), previousCharOffset) == 13) {
			return end.y - start.y + 1;
		}
		else {
			return end.y - start.y;
		}
	}

	size_t PieceTree::offsetInBuffer(block_t* bt, const glm::ivec2& cursor) {
		auto& lineStarts = bt->lineStarts;
		int ret = cursor.x;
		if (cursor.y < lineStarts.size())
		{
			ret += lineStarts[cursor.y];
		}
		else {
			ret += bt->count;// buffer.size();
		}
		return ret;
	}
	size_t PieceTree::offsetInBuffer(block_it bt, const glm::ivec2& cursor) {
		let& lineStarts = bt->lineStarts;// this->_buffers[bufferIndex].lineStarts;
		return lineStarts[cursor.y] + cursor.x;
	}

	void PieceTree::deleteNodes(std::vector<node_tp> nodes) {
		for (let i = 0; i < nodes.size(); i++) {
			rbDelete(nodes[i]);
		}
	}

	std::vector<piece_t> PieceTree::createNewPieces(std::string text)
	{
		std::vector<piece_t>  newPieces;
		if (text.length() > AverageBufferSize) {
			// the content is large, operations like substr, charCode becomes slow
			// so here we split it into smaller chunks, just like what we did for CR/LF normalization


			while (text.length() > AverageBufferSize) {
				uint32_t lastChar = text.at(AverageBufferSize - 1);
				std::string splitText;
				if (lastChar == (uint32_t)CharCode::CarriageReturn || (lastChar >= 0xD800 && lastChar <= 0xDBFF)) {
					// last character is \r or a high surrogate => keep it back
					splitText = text.substr(0, AverageBufferSize - 1);
					text = text.substr(AverageBufferSize - 1);
				}
				else {
					splitText = text.substr(0, AverageBufferSize);
					text = text.substr(AverageBufferSize);
				}

				let lineStarts = createLineStartsFast(splitText);
				auto stc = md::get_utf8_count(splitText.c_str(), splitText.size());
				this->_buffers.push_back({ splitText, (size_t)stc, lineStarts });
				// todo piece_t
				newPieces.push_back(piece_t(
					&(*(--_buffers.end())),
					{ 0, 0 },
					{ stc - lineStarts[lineStarts.size() - 1], lineStarts.size() - 1 },
					(size_t)stc,
					lineStarts.size() - 1
				));
			}

			let lineStarts = createLineStartsFast(text);
			size_t textcount = md::get_utf8_count(text.c_str(), text.size());
			_buffers.push_back({ text, textcount, lineStarts });
			newPieces.push_back(piece_t(
				&(*(--_buffers.end())),
				{ 0, 0 },
				{ textcount - lineStarts[lineStarts.size() - 1], lineStarts.size() - 1 },
				textcount,
				lineStarts.size() - 1
			));

			return newPieces;
		}

		auto b0 = _buffers.begin();
		let startOffset = b0->count;
		let lineStarts = createLineStartsFast(text, false);

		auto tc = md::get_utf8_count(text.c_str(), text.size());
		let start = this->_lastChangeBufferPos;
		if (b0->lineStarts.empty())
		{
			b0->lineStarts.push_back(0);
		}
		if (b0->lineStarts[b0->lineStarts.size() - 1] == startOffset
			&& startOffset != 0
			&& this->startWithLF(text)
			&& this->endWithCR(b0->buffer) // todo, we can check this->_lastChangeBufferPos's column as it's the last one
			) {
			this->_lastChangeBufferPos = { this->_lastChangeBufferPos.x + 1, this->_lastChangeBufferPos.y };
			start = this->_lastChangeBufferPos;

			for (let i = 0; i < lineStarts.size(); i++) {
				lineStarts[i] += startOffset + 1;
			}

			b0->lineStarts.insert(b0->lineStarts.end(), lineStarts.begin() + 1, lineStarts.end());// = (<number[]>b0->lineStarts).concat(<number[]>lineStarts.slice(1));
			b0->buffer.push_back('_');
			b0->buffer += text;
			b0->count++;
			startOffset += 1;
		}
		else {
			if (startOffset != 0) {
				for (let i = 0; i < lineStarts.size(); i++) {
					lineStarts[i] += startOffset;
				}
			}
			b0->lineStarts.insert(b0->lineStarts.end(), lineStarts.begin() + 1, lineStarts.end());// = (<number[]>b0->lineStarts).concat(<number[]>lineStarts.slice(1));
			b0->buffer += text;
		}

		b0->count += tc;
		auto endOffset = b0->count;
		let endIndex = b0->lineStarts.size() - 1;
		let endColumn = endOffset - b0->lineStarts[endIndex];
		glm::ivec2 endPos = { endColumn, endIndex };
		let newPiece = piece_t(
			/** todo@peng */
			&*b0,
			start,
			endPos,
			endOffset - startOffset,
			this->getLineFeedCnt(b0, start, endPos)

		);
		this->_lastChangeBufferPos = endPos;
		newPieces.push_back(newPiece);
		return newPieces;
	}

	std::string	PieceTree::getLinesRawContent() {
		return this->getContentOfSubTree(root);
	}


	std::string PieceTree::getLineRawContent(int lineNumber, int endOffset)
	{
		let x = root;
		std::string ret;
		let cache = _searchCache->get2(lineNumber);
		if (cache) {
			x = cache->node;
			let prevAccumulatedValue = getAccumulatedValue(x, lineNumber - cache->nodeStartLineNumber - 1);
			let& buffer = x->piece._bt->buffer;
			let startOffset = offsetInBuffer(x->piece._bt, x->piece._start);
			if (cache->nodeStartLineNumber + x->piece.lineFeedCnt == lineNumber) {
				get_sub_string(x->piece._bt, startOffset + prevAccumulatedValue, /*startOffset +*/ x->piece.length - prevAccumulatedValue, ret);
			}
			else {
				let accumulatedValue = getAccumulatedValue(x, lineNumber - cache->nodeStartLineNumber);
				return get_sub_string(x->piece._bt, startOffset + prevAccumulatedValue, /*startOffset +*/ accumulatedValue - endOffset - prevAccumulatedValue);
			}
		}
		else {
			let nodeStartOffset = 0;
			let originalLineNumber = lineNumber;
			while (x != SENTINEL) {
				if (x->left != SENTINEL && x->lf_left >= lineNumber - 1) {
					x = x->left;
				}
				else if (x->lf_left + x->piece.lineFeedCnt > lineNumber - 1) {
					let prevAccumulatedValue = getAccumulatedValue(x, lineNumber - x->lf_left - 2);
					let accumulatedValue = getAccumulatedValue(x, lineNumber - x->lf_left - 1);
					let& buffer = x->piece._bt->buffer;
					let startOffset = offsetInBuffer(x->piece._bt, x->piece._start);
					nodeStartOffset += x->size_left;
					_searchCache->set({ x, nodeStartOffset, (int)(originalLineNumber - (lineNumber - 1 - x->lf_left)) });

					return get_sub_string(x->piece._bt, startOffset + prevAccumulatedValue, /*startOffset +*/ accumulatedValue - endOffset - prevAccumulatedValue);
				}
				else if (x->lf_left + x->piece.lineFeedCnt == lineNumber - 1) {
					let prevAccumulatedValue = getAccumulatedValue(x, lineNumber - x->lf_left - 2);
					let& buffer = x->piece._bt->buffer;
					let startOffset = offsetInBuffer(x->piece._bt, x->piece._start);

					ret = get_sub_string(x->piece._bt, startOffset + prevAccumulatedValue, /*startOffset +*/ x->piece.length - prevAccumulatedValue);
					break;
				}
				else {
					lineNumber -= x->lf_left + x->piece.lineFeedCnt;
					nodeStartOffset += x->size_left + x->piece.length;
					x = x->right;
				}
			}
		}

		// search in order, to find the node contains end column
		x = x->next();
		while (x != SENTINEL) {
			let& buffer = x->piece._bt->buffer;

			if (x->piece.lineFeedCnt > 0) {
				let accumulatedValue = getAccumulatedValue(x, 0);
				let startOffset = offsetInBuffer(x->piece._bt, x->piece._start);

				//ret += buffer.substr(startOffset,/* startOffset +*/ accumulatedValue - endOffset);
				get_sub_string(x->piece._bt, startOffset,/* startOffset +*/ accumulatedValue - endOffset, ret);
				return ret;
			}
			else {
				let startOffset = offsetInBuffer(x->piece._bt, x->piece._start);
				//ret += buffer.substr(startOffset, x->piece.length);
				get_sub_string(x->piece._bt, startOffset, x->piece.length, ret);
			}

			x = x->next();
		}

		return ret;
	}
	void PieceTree::computeBufferMetadata() {
		let x = root;

		let lfCnt = 1;
		let len = 0;
		//let ct = 0;

		while (x != SENTINEL) {
			lfCnt += x->lf_left + x->piece.lineFeedCnt;
			len += x->size_left + x->piece.length;
			//ct += x->size_left_ct + x->piece.count;
			x = x->right;
		}

		this->_lineCnt = lfCnt;
		this->_length = len;
		//this->_count = ct;
		_searchCache->validate(this->_length);
	}

	// #region node operations
	glm::ivec2 PieceTree::getIndexOf(node_tp node, size_t accumulatedValue)
	{
		let piece = node->piece;
		let pos = this->positionInBuffer(node, accumulatedValue);
		let lineCnt = pos.y - piece._start.y;

		if (this->offsetInBuffer(piece._bt, piece._end) - this->offsetInBuffer(piece._bt, piece._start) == accumulatedValue) {
			// we are checking the end of this node, so a CRLF check is necessary.
			let realLineCnt = this->getLineFeedCnt(node->piece._bt, piece._start, pos);
			if (realLineCnt != lineCnt) {
				// aha yes, CRLF
				return { 0, realLineCnt };
			}
		}

		return { pos.x, lineCnt };
	}

	size_t PieceTree::getAccumulatedValue(node_tp node, int64_t index) {
		if (index < 0) {
			return 0;
		}
		let piece = node->piece;
		let& lineStarts = piece._bt->lineStarts;// this->_buffers[piece._bt].lineStarts;
		let expectedLineStartIndex = piece._start.y + index + 1;
		if (expectedLineStartIndex > piece._end.y) {
			return lineStarts[piece._end.y] + piece._end.x - lineStarts[piece._start.y] - piece._start.x;
		}
		else {
			return lineStarts[expectedLineStartIndex] - lineStarts[piece._start.y] - piece._start.x;
		}
	}

	void PieceTree::deleteNodeTail(node_tp node, glm::ivec2 pos) {
		auto piece = node->piece;
		auto originalLFCnt = piece.lineFeedCnt;
		auto originalEndOffset = this->offsetInBuffer(piece._bt, piece._end);

		auto newEnd = pos;
		auto newEndOffset = this->offsetInBuffer(piece._bt, newEnd);
		auto newLineFeedCnt = this->getLineFeedCnt(piece._bt, piece._start, newEnd);

		auto lf_delta = newLineFeedCnt - originalLFCnt;
		auto size_delta = newEndOffset - originalEndOffset;
		auto newLength = piece.length + size_delta;

		node->piece = piece_t(
			piece._bt,
			piece._start,
			newEnd,
			newLength,
			newLineFeedCnt
		);

		updateTreeMetadata(node, size_delta, lf_delta);
	}

	void PieceTree::deleteNodeHead(node_tp node, glm::ivec2 pos)
	{
		auto piece = node->piece;
		auto originalLFCnt = piece.lineFeedCnt;
		auto originalStartOffset = this->offsetInBuffer(piece._bt, piece._start);

		auto newStart = pos;
		auto newLineFeedCnt = this->getLineFeedCnt(piece._bt, newStart, piece._end);
		auto newStartOffset = this->offsetInBuffer(piece._bt, newStart);
		auto lf_delta = newLineFeedCnt - originalLFCnt;
		auto size_delta = originalStartOffset - newStartOffset;
		auto newLength = piece.length + size_delta;
		node->piece = piece_t(
			piece._bt,
			newStart,
			piece._end,
			newLength,
			newLineFeedCnt
		);

		updateTreeMetadata(node, size_delta, lf_delta);
	}

	void PieceTree::shrinkNode(node_tp node, glm::ivec2 start, glm::ivec2 end) {
		auto piece = node->piece;
		auto originalStartPos = piece._start;
		auto originalEndPos = piece._end;

		// old piece, originalStartPos, start
		auto oldLength = piece.length;
		auto oldLFCnt = piece.lineFeedCnt;
		auto newEnd = start;
		auto newLineFeedCnt = this->getLineFeedCnt(piece._bt, piece._start, newEnd);
		auto newLength = this->offsetInBuffer(piece._bt, start) - this->offsetInBuffer(piece._bt, originalStartPos);

		node->piece = piece_t(
			piece._bt,
			piece._start,
			newEnd,
			newLength,
			newLineFeedCnt
		);

		updateTreeMetadata(node, newLength - oldLength, newLineFeedCnt - oldLFCnt);

		// new right piece, end, originalEndPos
		let newPiece = piece_t(
			piece._bt,
			end,
			originalEndPos,
			this->offsetInBuffer(piece._bt, originalEndPos) - this->offsetInBuffer(piece._bt, end),
			this->getLineFeedCnt(piece._bt, end, originalEndPos)

		);

		let newNode = this->rbInsertRight(node, newPiece);
		this->validateCRLFWithPrevNode(newNode);
	}

	void PieceTree::appendToNode(node_tp node, std::string value)
	{
		if (this->adjustCarriageReturnFromNext(value, node)) {
			value.push_back('\n');
		}

		auto b0 = _buffers.begin();
		auto hitCRLF = this->shouldCheckCRLF() && this->startWithLF(value) && this->endWithCR(node);
		auto startOffset = b0->count;
		b0->buffer += value;
		auto vcount = md::get_utf8_count(value.c_str(), value.size());
		b0->count += vcount;
		let lineStarts = createLineStartsFast(value, false);
		for (let i = 0; i < lineStarts.size(); i++) {
			lineStarts[i] += startOffset;
		}
		if (hitCRLF) {
			let prevStartOffset = b0->lineStarts[b0->lineStarts.size() - 2];
			b0->lineStarts.pop_back();
			// _lastChangeBufferPos is already wrong
			this->_lastChangeBufferPos = { startOffset - prevStartOffset, this->_lastChangeBufferPos.y - 1 };
		}

		b0->lineStarts.insert(b0->lineStarts.end(), lineStarts.begin() + 1, lineStarts.end());//= (<number[]>b0->lineStarts).concat(<number[]>lineStarts.slice(1));
		auto endIndex = b0->lineStarts.size() - 1;
		auto endColumn = b0->count - b0->lineStarts[endIndex];
		glm::ivec2 newEnd = { endColumn, endIndex };
		auto newLength = node->piece.length + vcount;
		auto oldLineFeedCnt = node->piece.lineFeedCnt;
		auto newLineFeedCnt = this->getLineFeedCnt(b0, node->piece._start, newEnd);
		auto lf_delta = newLineFeedCnt - oldLineFeedCnt;

		node->piece = piece_t(
			node->piece._bt,
			node->piece._start,
			newEnd,
			newLength,
			newLineFeedCnt
		);

		this->_lastChangeBufferPos = newEnd;
		updateTreeMetadata(node, vcount, lf_delta);
	}

	NodePosition PieceTree::nodeAt(size_t offset) {
		let x = root;
		NodePosition ret = {};
#if 1
		let cache = _searchCache->get(offset);
		if (cache) {
			ret.node = cache->node;
			ret.nodeStartOffset = cache->nodeStartOffset;
			ret.remainder = offset - cache->nodeStartOffset;
			return ret;
		}
#endif
		let nodeStartOffset = 0;

		while (x != SENTINEL) {
			if (x->size_left > offset) {
				x = x->left;
			}
			else if (x->size_left + x->piece.length >= offset) {
				nodeStartOffset += x->size_left;
				ret.node = x;
				ret.remainder = offset - x->size_left;
				ret.nodeStartOffset = nodeStartOffset;
				_searchCache->set(ret);
				return ret;
			}
			else {
				offset -= x->size_left + x->piece.length;
				nodeStartOffset += x->size_left + x->piece.length;
				x = x->right;
			}
		}
		return ret;
	}
	NodePosition PieceTree::nodeAt2(size_t lineNumber, size_t column) {
		let x = root;
		size_t nodeStartOffset = 0;

		NodePosition ret;
#if 1
		while (x != SENTINEL) {
			if (x->left != SENTINEL && x->lf_left >= lineNumber - 1) {
				x = x->left;
			}
			else if (x->lf_left + x->piece.lineFeedCnt > lineNumber - 1) {
				let prevAccumualtedValue = this->getAccumulatedValue(x, lineNumber - x->lf_left - 2);
				let accumualtedValue = this->getAccumulatedValue(x, lineNumber - x->lf_left - 1);
				nodeStartOffset += x->size_left;
				ret.node = x;
				ret.remainder = std::min(prevAccumualtedValue + column - 1, accumualtedValue);
				ret.nodeStartOffset = nodeStartOffset;
				return ret;
			}
			else if (x->lf_left + x->piece.lineFeedCnt == lineNumber - 1) {
				let prevAccumualtedValue = this->getAccumulatedValue(x, lineNumber - x->lf_left - 2);
				if (prevAccumualtedValue + column - 1 <= x->piece.length) {
					ret.node = x;
					ret.remainder = prevAccumualtedValue + column - 1;
					ret.nodeStartOffset = nodeStartOffset;
					return ret;
				}
				else {
					column -= x->piece.length - prevAccumualtedValue;
					break;
				}
			}
			else {
				lineNumber -= x->lf_left + x->piece.lineFeedCnt;
				nodeStartOffset += x->size_left + x->piece.length;
				x = x->right;
			}
		}

		// search in order, to find the node contains position.x
		if (x)
		{

			x = x->next();
			while (x != SENTINEL) {

				if (x->piece.lineFeedCnt > 0) {
					let accumualtedValue = this->getAccumulatedValue(x, 0);
					let nodeStartOffset = this->offsetOfNode(x);
					ret.node = x;
					ret.remainder = std::min(column - 1, accumualtedValue);
					ret.nodeStartOffset = nodeStartOffset;
					return ret;
				}
				else {
					if (x->piece.length >= column - 1) {
						let nodeStartOffset = this->offsetOfNode(x);
						ret.node = x;
						ret.remainder = column - 1;
						ret.nodeStartOffset = nodeStartOffset;
						return ret;
					}
					else {
						column -= x->piece.length;
					}
				}

				x = x->next();
			}
		}
#endif

		return ret;
	}

	size_t PieceTree::nodeCharCodeAt(node_tp node, size_t offset) {
		if (node->piece.lineFeedCnt < 1) {
			return -1;
		}
		let& buffer = node->piece._bt->buffer;
		let newOffset = this->offsetInBuffer(node->piece._bt, node->piece._start) + offset;
		return md::get_u8_idx(buffer.c_str(), newOffset);
		//return buffer.at(newOffset);
	}

	size_t PieceTree::offsetOfNode(node_tp node) {
		if (!node) {
			return 0;
		}
		let pos = node->size_left;
		while (node != root) {
			if (node->parent->right == node) {
				pos += node->parent->size_left + node->parent->piece.length;
			}

			node = node->parent;
		}

		return pos;
	}

	// #endregion

	// #region CRLF
	bool PieceTree::shouldCheckCRLF() {
		return !(this->_EOLNormalized && this->_EOL == "\n");
	}

	bool PieceTree::startWithLF(const std::string& val) {
		return val.size() && val.at(0) == 10;
	}
	bool PieceTree::startWithLF(node_tp val) {


		if (val == SENTINEL || val->piece.lineFeedCnt == 0) {
			return false;
		}

		let piece = val->piece;
		let& lineStarts = piece._bt->lineStarts;// this->_buffers[piece._bt].lineStarts;
		let line = piece._start.y;
		let startOffset = lineStarts[line] + piece._start.x;
		if (line == lineStarts.size() - 1) {
			// last line, so there is no line feed at the end of this line
			return false;
		}
		let nextLineOffset = lineStarts[line + 1];
		if (nextLineOffset > startOffset + 1) {
			return false;
		}
		return md::get_u8_idx(piece._bt->buffer.c_str(), startOffset) == 10;
		//return piece._bt->buffer.at(startOffset) == 10;
	}

	bool PieceTree::endWithCR(const std::string& val) {
		return val.size() && val.at(val.size() - 1) == 13;

	}
	bool PieceTree::endWithCR(node_tp val) {
		if (val == SENTINEL || val->piece.lineFeedCnt == 0) {
			return false;
		}

		return this->nodeCharCodeAt(val, val->piece.length - 1) == 13;
	}

	void PieceTree::validateCRLFWithPrevNode(node_tp nextNode) {
		if (this->shouldCheckCRLF() && this->startWithLF(nextNode)) {
			let node = nextNode->prev();
			if (this->endWithCR(node)) {
				this->fixCRLF(node, nextNode);
			}
		}
	}

	void PieceTree::validateCRLFWithNextNode(node_tp node) {
		if (this->shouldCheckCRLF() && this->endWithCR(node)) {
			let nextNode = node->next();
			if (this->startWithLF(nextNode)) {
				this->fixCRLF(node, nextNode);
			}
		}
	}

	void PieceTree::fixCRLF(node_tp prev, node_tp next) {
		std::queue<node_tp> nodesToDel;
		// update node
		let& lineStarts = prev->piece._bt->lineStarts;
		glm::ivec2 newEnd;
		if (prev->piece._end.x == 0) {
			// it means, last line ends with \r, not \r\n
			newEnd = { lineStarts[prev->piece._end.y] - lineStarts[prev->piece._end.y - 1] - 1, prev->piece._end.y - 1 };
		}
		else {
			// \r\n
			newEnd = { prev->piece._end.x - 1, prev->piece._end.y };
		}

		auto prevNewLength = prev->piece.length - 1;
		auto prevNewLFCnt = prev->piece.lineFeedCnt - 1;
		prev->piece = piece_t(
			prev->piece._bt,
			prev->piece._start,
			newEnd,
			prevNewLength,
			prevNewLFCnt
		);

		updateTreeMetadata(prev, -1, -1);
		if (prev->piece.length == 0) {
			nodesToDel.push(prev);
		}

		// update nextNode
		glm::ivec2 newStart = { 0, next->piece._start.y + 1 };
		auto newLength = next->piece.length - 1;
		auto newLineFeedCnt = this->getLineFeedCnt(next->piece._bt, newStart, next->piece._end);
		next->piece = piece_t(
			next->piece._bt,
			newStart,
			next->piece._end,
			newLength,
			newLineFeedCnt
		);

		updateTreeMetadata(next, -1, -1);
		if (next->piece.length == 0) {
			nodesToDel.push(next);
		}

		// create new piece which contains \r\n
		let pieces = this->createNewPieces("\r\n");
		this->rbInsertRight(prev, pieces[0]);
		// delete empty nodes

		for (; nodesToDel.size();) {
			rbDelete(nodesToDel.back()); nodesToDel.pop();
		}
	}

	bool PieceTree::adjustCarriageReturnFromNext(std::string value, node_tp node)
	{
		if (this->shouldCheckCRLF() && this->endWithCR(value)) {
			let nextNode = node->next();
			if (this->startWithLF(nextNode)) {
				// move `\n` forward
				value.push_back('\n');

				if (nextNode->piece.length == 1) {
					rbDelete(nextNode);
				}
				else {

					auto piece = nextNode->piece;
					glm::ivec2 newStart = { 0, piece._start.y + 1 };
					auto newLength = piece.length - 1;
					auto newLineFeedCnt = this->getLineFeedCnt(piece._bt, newStart, piece._end);
					nextNode->piece = piece_t(
						piece._bt,
						newStart,
						piece._end,
						newLength,
						newLineFeedCnt
					);

					updateTreeMetadata(nextNode, -1, -1);
				}
				return true;
			}
		}

		return false;
	}

	// #endregion

	// #endregion

	// #region Tree operations
	bool PieceTree::iterate(node_tp node, std::function<bool(node_tp node)> callback)
	{
		if (node == SENTINEL) {
			return callback(SENTINEL);
		}

		let leftRet = this->iterate(node->left, callback);
		if (!leftRet) {
			return leftRet;
		}

		return callback(node) && this->iterate(node->right, callback);
	}

	std::string PieceTree::getNodeContent(node_tp node)
	{
		if (node == SENTINEL) {
			return "";
		}
		let& buffer = node->piece._bt->buffer;
		//std::string currentContent;
		let& piece = node->piece;
		let startOffset = this->offsetInBuffer(piece._bt, piece._start);
		let endOffset = this->offsetInBuffer(piece._bt, piece._end);
		//currentContent = buffer.substr(startOffset, endOffset - startOffset);
		return get_sub_string(piece._bt, startOffset, endOffset - startOffset);
	}

	std::string PieceTree::getPieceContent(const piece_t& piece) {
		let& buffer = piece._bt->buffer;
		let startOffset = this->offsetInBuffer(piece._bt, piece._start);
		let endOffset = this->offsetInBuffer(piece._bt, piece._end);
		//let currentContent = buffer.substr(startOffset, endOffset - startOffset);
		return get_sub_string(piece._bt, startOffset, endOffset - startOffset);
	}

	/**
	 *      node              node
	 *     /  \              /  \
	 *    a   b    <----   a    b
	 *                         /
	 *                        z
	 */


	node_tp PieceTree::rbInsertRight(node_tp node, const piece_t& p)
	{
		let z = new_node(p, NColor::Red);
		z->left = SENTINEL;
		z->right = SENTINEL;
		z->parent = SENTINEL;
		z->size_left = 0;
		z->lf_left = 0;

		let x = root;
		if (x == SENTINEL) {
			root = z;
			z->color = NColor::Black;
		}
		else if (node) {
			if (node->right == SENTINEL) {
				node->right = z;
				z->parent = node;
			}
			else {
				let nextNode = leftest(node->right);
				nextNode->left = z;
				z->parent = nextNode;
			}
		}

		fixInsert(z);
		return z;
	}

	/**
	 *      node              node
	 *     /  \              /  \
	 *    a   b     ---->   a    b
	 *                       \
	 *                        z
	 */
	node_tp PieceTree::rbInsertLeft(node_tp node, const piece_t& p)
	{
		let z = new_node(p, NColor::Red);
		z->left = SENTINEL;
		z->right = SENTINEL;
		z->parent = SENTINEL;
		z->size_left = 0;
		z->lf_left = 0;

		if (root == SENTINEL) {
			root = z;
			z->color = NColor::Black;
		}
		else if (node) {
			if (node->left == SENTINEL) {
				node->left = z;
				z->parent = node;
			}
			else {
				let prevNode = righttest(node->left); // a
				prevNode->right = z;
				z->parent = prevNode;
			}
		}

		fixInsert(z);
		return z;
	}

	std::string PieceTree::getContentOfSubTree(node_tp node) {
		std::string str = "";

		this->iterate(node, [&](node_tp nt)
			{
				str += this->getNodeContent(nt);
				return true;
			});

		return str;
	}
	// #endregion

	node_rbt* PieceTree::new_node(const piece_t& piece, NColor color)
	{
		auto ret = new node_rbt(piece, color);
		_node_count++;
		_gf.insert(ret);
		return ret;
	}

	void PieceTree::detach(node_rbt* p)
	{
		if (p)
		{
			_node_count--;
			p->parent = 0;
			p->left = 0;
			p->right = 0;
			_gf.erase(p);
			delete p;
		}
	}

	void PieceTree::get_sub_string(block_t* bt, size_t idx, size_t count, std::string& str)
	{
		let& buffer = bt->buffer;
		let bs = buffer.size();
		if (count > 0)
		{
			const char* t = buffer.c_str();
			auto t0 = md::utf8_char_pos(t, idx, bs);
			size_t tc0 = t0 - t;
			auto t1 = md::utf8_char_pos(t0, count, bs - tc0);
			int64_t c = size_t(t1 - t0);
			if (t0 && t1 && c > 0)
			{
				str.reserve(str.size() + c);
				str.append(t0, t1);
			}
		}
	}
	std::string PieceTree::get_sub_string(block_t* bt, size_t idx, size_t count)
	{
		std::string ret;
		get_sub_string(bt, idx, count, ret);
		return ret;
	}

#endif // 1
	//}
	// !rb
}
// !hz
