#ifndef __WIN32MSG_H__
#define __WIN32MSG_H__
#include <WinSock2.h>
#include <windows.h>
#include <richedit.h>
#include <Commctrl.h>
#ifndef _RICHOLE_
#include <RichOle.h>
#define _RICHOLE_
#endif
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <assert.h>

//include <view/utils_info.h>
//#include <base/hlUtil.h>
//#include "base_util.h"
//#include <view/mem_pe.h> 
//-------------使用系统当前风格---------------------------------------------------------------- 
#ifdef _WIN32
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif 
//----------------------------------------------------------------------------- 
#ifdef DEBUG
#define ASSERT_R(e) ASSERT_R(e)
#else
#define ASSERT_R(e) e
#endif // DEBUG


namespace hwm
{
	class hotkey_base
	{
	public:
		HWND _hWnd = 0;
		std::unordered_map<int, std::function<void(int)>> _hot_kid;
	public:
		hotkey_base() {}
		virtual ~hotkey_base()
		{
			for (auto& [k, v] : _hot_kid)
			{
				unreg_hot_key(k);
			}
		}

		//void reghotkey(HWND hWnd, uint32_t uMod, uint32_t uVk)
		//{
		//	uMod = HotkeyToMod(uMod);
		//	if (!RegisterHotKey(hWnd, HOTKEY_SHOT, uMod, uVk))
		//	{
		//		MessageBox(hDlg, L"注册热键失败，可能发生冲突", L"Error", MB_OK);
		//		break;
		//	}
		//}
		/*
	todo:热键
			auto hf = [](int id) {
	printf("hot: %d\n", id);
};
//pw->reg_hot_key("ctrl F5", hf);
pw->reg_hot_key("win numpad_5", hf);
pw->reg_hot_key("ctrl alt", hf);
pw->reg_hot_key("ctrl alt numpad_5", hf);
*/
// alt、ctrl、shift、win
// 成功返回id
		int reg_hot_key(const std::string& kstr, std::function<void(int id)> func)
		{
			uint32_t vk = 0;
			int fsmod = get_hk(kstr, vk);
			uint32_t id = (fsmod << 8) | vk;
			if (fsmod)
			{
				fsmod |= MOD_NOREPEAT;
			}
			auto hr = ::RegisterHotKey(_hWnd, id, fsmod, vk);
			if (hr)
			{
				_hot_kid[id] = func;
			}
			else {
				id = 0;
			}
			return id;
		}
		void call_hot_funcs(int id)
		{
			auto it = _hot_kid.find(id);
			if (it != _hot_kid.end() && it->second)
			{
				it->second(id);
			}
		}
		bool unreg_hot_key(int id)
		{
			return ::UnregisterHotKey(_hWnd, id) == TRUE;
		}
		// 多分割符
		static std::vector<std::string> split_m(const std::string& str, const std::string& pattern, bool is_space)
		{
			std::vector<std::string> vs;
			std::string tem;
			for (auto ch : str)
			{
				if (pattern.find(ch) == -1 || pattern.empty())
				{
					tem.push_back(ch);
				}
				else
				{
					if (tem == "" && !is_space)
						continue;
					vs.push_back(tem);
					tem = "";
				}
			}
			if (tem != "")
			{
				vs.push_back(tem);
			}
			return vs;
		}
		static int get_hk(const std::string& kstr, uint32_t& vk)
		{
			static std::unordered_map<std::string, int> hkt = { {"alt",MOD_ALT }, {"ctrl",MOD_CONTROL }
			, {"shift",MOD_SHIFT }, {"win",MOD_WIN } };
			int fsmod = 0;
			auto k = split_m(kstr, " ", false);
			for (auto& it : k)
			{
				auto ht = hkt.find(it);
				if (ht != hkt.end())
				{
					fsmod |= ht->second;
				}
				else
				{
					auto c = hz::vkcode2i(it);
					if (c && !vk)
					{
						vk = c;
					}
				}
			}
			return fsmod;
		}
	private:
	};
	class ctrl_base
	{
	public:
		HWND m_hWnd = 0;
	public:
		ctrl_base()
		{
		}

		ctrl_base(HWND h) :m_hWnd(h)
		{
		}

		virtual ~ctrl_base()
		{
		}
		operator HWND()
		{
			return m_hWnd;
		}
	public:

		void set_enable(bool be)
		{
			BOOL b = be ? TRUE : FALSE;
			EnableWindow(m_hWnd, b);
		}

		void set_text(std::string str)
		{
			//str = hz::u8_gbk(str);
			::SendMessage(m_hWnd, WM_SETTEXT, 0, (LPARAM)str.c_str());
		}
		std::string get_text()
		{
			std::string ret;
			size_t s = GetWindowTextLength(m_hWnd);
			if (s > 0)
			{
				ret.resize(s + 1);
				s = GetWindowText(m_hWnd, ret.data(), ret.size());
			}
			return ret.c_str();
		}
		void set_hotkey(std::string ks)
		{
			uint32_t vk = 0;
			int fsmod = hotkey_base::get_hk(ks, vk);
			if (!m_hWnd)return;
			uint32_t mk = fsmod << 8;
			mk |= vk;
			DWORD dwVal = SendMessage(m_hWnd, HKM_SETHOTKEY, mk, 0);
			return;
		}
		std::string get_hotkey()
		{
			std::string ret;
			if (!m_hWnd)return ret;
			DWORD dwVal = SendMessage(m_hWnd, HKM_GETHOTKEY, 0, 0);
			UINT uVk = LOBYTE(LOWORD(dwVal));
			UINT uMod = HIBYTE(LOWORD(dwVal));

			if (uMod & HOTKEYF_CONTROL)
			{
				ret += "ctrl";
			}
			if (uMod & HOTKEYF_SHIFT)
			{
				if (ret.size())ret += " ";
				ret += "shift";
			}
			if (uMod & HOTKEYF_ALT)
			{
				if (ret.size())ret += " ";
				ret += "alt";
			}
			if (ret.size())ret += " ";
			ret += hz::vkcode2str(uVk);
			return ret;
		}
		UINT HotkeyToMod(UINT fsModifiers)
		{
			if ((fsModifiers & HOTKEYF_SHIFT) && !(fsModifiers & HOTKEYF_ALT)) // shift转alt
			{
				fsModifiers &= ~HOTKEYF_SHIFT;
				fsModifiers |= MOD_SHIFT;
			}
			else if (!(fsModifiers & HOTKEYF_SHIFT) && (fsModifiers & HOTKEYF_ALT)) // alt转shift
			{
				fsModifiers &= ~HOTKEYF_ALT;
				fsModifiers |= MOD_ALT;
			}
			return fsModifiers;
		}

		UINT ModToHotkey(UINT fsModifiers)
		{
			if ((fsModifiers & MOD_SHIFT) && !(fsModifiers & MOD_ALT)) // shift转alt
			{
				fsModifiers &= ~MOD_SHIFT;
				fsModifiers |= HOTKEYF_SHIFT;
			}
			else if (!(fsModifiers & MOD_SHIFT) && (fsModifiers & MOD_ALT)) // alt转shift
			{
				fsModifiers &= ~MOD_ALT;
				fsModifiers |= HOTKEYF_ALT;
			}
			return fsModifiers;
		}

	private:

	};
	class cbo :public ctrl_base
	{
	public:
		cbo()
		{
		}

		cbo(HWND h) :ctrl_base(h)
		{
		}
		~cbo()
		{
		}
	public:
		void set_readonly(int readonly)
		{
			auto cbedit = ::GetWindow(m_hWnd, GW_CHILD);
			::SendMessage(cbedit, EM_SETREADONLY, readonly, 0);
		}
		int GetCount() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_GETCOUNT, 0, 0);
		}
		int GetCurSel() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_GETCURSEL, 0, 0);
		}
		int SetCurSel(int nSelect)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_SETCURSEL, nSelect, 0);
		}
		DWORD GetEditSel() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return DWORD(::SendMessage(m_hWnd, CB_GETEDITSEL, 0, 0));
		}
		BOOL LimitText(int nMaxChars)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, CB_LIMITTEXT, nMaxChars, 0);
		}
		BOOL SetEditSel(_In_ int nStartChar, _In_ int nEndChar)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, CB_SETEDITSEL, 0, MAKELONG(nStartChar, nEndChar));
		}
		DWORD_PTR GetItemData(int nIndex) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return ::SendMessage(m_hWnd, CB_GETITEMDATA, nIndex, 0);
		}
		int SetItemData(int nIndex, DWORD_PTR dwItemData)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_SETITEMDATA, nIndex, (LPARAM)dwItemData);
		}
		void* GetItemDataPtr(int nIndex) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (LPVOID)GetItemData(nIndex);
		}
		int SetItemDataPtr(int nIndex, void* pData)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return SetItemData(nIndex, (DWORD_PTR)(LPVOID)pData);
		}
#pragma warning(push)
#pragma warning(disable: 6001 6054)
		int GetLBText(_In_ int nIndex, _Pre_notnull_ _Post_z_ LPTSTR lpszText) const
		{
			ASSERT_R(::IsWindow(m_hWnd));
			return (int)::SendMessage(m_hWnd, CB_GETLBTEXT, nIndex, (LPARAM)lpszText);
		}
#pragma warning(pop)
		int GetLBTextLen(int nIndex) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_GETLBTEXTLEN, nIndex, 0);
		}
		void ShowDropDown(BOOL bShowIt)
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, CB_SHOWDROPDOWN, bShowIt, 0);
		}
		int AddString(const std::string& str)
		{
#ifdef __utils_info_h__
			auto s = hz::u8_gbk((str));
#else
			auto& s = str;
#endif
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_ADDSTRING, 0, (LPARAM)s.c_str());
		}
		int DeleteString(UINT nIndex)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_DELETESTRING, nIndex, 0);
		}
		int InsertString(_In_ int nIndex, _In_z_ const std::string& str)
		{
#ifdef __utils_info_h__
			auto s = hz::u8_gbk((str));
#else
			auto& s = str;
#endif
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_INSERTSTRING, nIndex, (LPARAM)s.c_str());
		}
		void ResetContent()
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, CB_RESETCONTENT, 0, 0);
		}
		int Dir(_In_ UINT attr, _In_ LPCTSTR lpszWildCard)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_DIR, attr, (LPARAM)lpszWildCard);
		}
		int FindString(_In_ int nStartAfter, _In_z_ const std::string& str) const
		{
#ifdef __utils_info_h__
			auto s = hz::u8_gbk((str));
#else
			auto& s = str;
#endif
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_FINDSTRING, nStartAfter,
				(LPARAM)s.c_str());
		}
		int SelectString(int nStartAfter, const std::string& str)
		{
#ifdef __utils_info_h__
			auto s = hz::u8_gbk((str));
#else
			auto& s = str;
#endif
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_SELECTSTRING,
				nStartAfter, (LPARAM)s.c_str());
		}
		void Clear()
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_CLEAR, 0, 0);
		}
		void Copy()
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_COPY, 0, 0);
		}
		void Cut()
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_CUT, 0, 0);
		}
		void Paste()
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_PASTE, 0, 0);
		}
		int SetItemHeight(int nIndex, UINT cyItemHeight)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_SETITEMHEIGHT, nIndex, MAKELONG(cyItemHeight, 0));
		}
		int GetItemHeight(int nIndex) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_GETITEMHEIGHT, nIndex, 0L);
		}
		int FindStringExact(int nIndexStart, LPCTSTR lpszFind) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_FINDSTRINGEXACT, nIndexStart, (LPARAM)lpszFind);
		}
		int SetExtendedUI(BOOL bExtended)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_SETEXTENDEDUI, bExtended, 0L);
		}
		BOOL GetExtendedUI() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, CB_GETEXTENDEDUI, 0, 0L);
		}
		void GetDroppedControlRect(LPRECT lprect) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)lprect);
		}
		BOOL GetDroppedState() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, CB_GETDROPPEDSTATE, 0, 0L);
		}
		LCID GetLocale() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (LCID)::SendMessage(m_hWnd, CB_GETLOCALE, 0, 0);
		}
		LCID SetLocale(LCID nNewLocale)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (LCID)::SendMessage(m_hWnd, CB_SETLOCALE, (WPARAM)nNewLocale, 0);
		}
		int GetTopIndex() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_GETTOPINDEX, 0, 0);
		}
		int SetTopIndex(int nIndex)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_SETTOPINDEX, nIndex, 0);
		}
		int InitStorage(int nItems, UINT nBytes)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_INITSTORAGE, (WPARAM)nItems, nBytes);
		}
		void SetHorizontalExtent(UINT nExtent)
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, CB_SETHORIZONTALEXTENT, nExtent, 0);
		}
		UINT GetHorizontalExtent() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (UINT)::SendMessage(m_hWnd, CB_GETHORIZONTALEXTENT, 0, 0);
		}
		int SetDroppedWidth(UINT nWidth)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_SETDROPPEDWIDTH, nWidth, 0);
		}
		int GetDroppedWidth() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, CB_GETDROPPEDWIDTH, 0, 0);
		}
	private:

	};
	struct ivec2 { int x, y; };
	struct ivec3 { int x, y,z; };
	class pbm :public ctrl_base
	{
	private:
		ivec2 _range;
	public:
		pbm()
		{
		}
		pbm(HWND h) :ctrl_base(h)
		{
		}

		~pbm()
		{
		}
		void operator= (HWND other)
		{
			m_hWnd = other;
		}
	public:

		COLORREF SetBkColor(_In_ COLORREF clrNew)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (COLORREF) ::SendMessage(m_hWnd, PBM_SETBKCOLOR, 0, (LPARAM)clrNew);
		}
		void SetRange(_In_ short nLower, _In_ short nUpper)
		{
			_range.x = nLower;
			_range.y = nUpper;
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, PBM_SETRANGE, 0, MAKELPARAM(nLower, nUpper));
		}
		void SetRange32(_In_ int nLower, _In_ int nUpper)
		{
			_range.x = nLower;
			_range.y = nUpper;
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, PBM_SETRANGE32, (WPARAM)nLower, (LPARAM)nUpper);
		}
		ivec2 get_range()
		{
			return _range;
		}
		int GetPos() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int) ::SendMessage(m_hWnd, PBM_GETPOS, 0, 0);
		}
		int OffsetPos(_In_ int nPos)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int) ::SendMessage(m_hWnd, PBM_DELTAPOS, nPos, 0L);
		}
		int SetStep(_In_ int nStep)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int) ::SendMessage(m_hWnd, PBM_SETSTEP, nStep, 0L);
		}
		int StepIt()
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int) ::SendMessage(m_hWnd, PBM_STEPIT, 0, 0L);
		}
	private:

	};
	class rich_edit :public ctrl_base
	{
	public:
		rich_edit()
		{
		}

		~rich_edit()
		{
		}
		void operator= (HWND other)
		{
			m_hWnd = other;
		}
	public:
	public:
		BOOL CanUndo() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_CANUNDO, 0, 0);
		}
		BOOL CanRedo() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_CANREDO, 0, 0);
		}
		UNDONAMEID GetUndoName() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (UNDONAMEID) ::SendMessage(m_hWnd, EM_GETUNDONAME, 0, 0);
		}
		UNDONAMEID GetRedoName() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (UNDONAMEID) ::SendMessage(m_hWnd, EM_GETREDONAME, 0, 0);
		}
		int GetLineCount() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, EM_GETLINECOUNT, 0, 0);
		}
		BOOL GetModify() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_GETMODIFY, 0, 0);
		}
		void SetModify(_In_ BOOL bModified /* = TRUE */)
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_SETMODIFY, bModified, 0);
		}
		BOOL SetTextMode(_In_ UINT fMode)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, EM_SETTEXTMODE, (WPARAM)fMode, 0);
		}
		UINT GetTextMode() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (UINT) ::SendMessage(m_hWnd, EM_GETTEXTMODE, 0, 0);
		}
		void GetRect(_Out_ LPRECT lpRect) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_GETRECT, 0, (LPARAM)lpRect);
		}
		ivec2 GetCharPos(_In_ long lChar) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); ivec2 pt; ::SendMessage(m_hWnd, EM_POSFROMCHAR, (WPARAM)&pt, (LPARAM)lChar); return pt;
		}
		UINT GetOptions() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (UINT) ::SendMessage(m_hWnd, EM_GETOPTIONS, 0, 0);
		}
		void SetOptions(_In_ WORD wOp, _In_ DWORD dwFlags)
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_SETOPTIONS, (WPARAM)wOp, (LPARAM)dwFlags);
		}
		BOOL SetAutoURLDetect(_In_ BOOL bEnable /* = TRUE */)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, EM_AUTOURLDETECT, (WPARAM)bEnable, 0);
		}
		void EmptyUndoBuffer()
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_EMPTYUNDOBUFFER, 0, 0);
		}
		UINT SetUndoLimit(_In_ UINT nLimit)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (UINT) ::SendMessage(m_hWnd, EM_SETUNDOLIMIT, (WPARAM)nLimit, 0);
		}
		void ReplaceSel(_In_z_ LPCTSTR lpszNewText, _In_ BOOL bCanUndo)
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_REPLACESEL, (WPARAM)bCanUndo, (LPARAM)lpszNewText);
		}
		void SetRect(_In_ LPCRECT lpRect)
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_SETRECT, 0, (LPARAM)lpRect);
		}
		void StopGroupTyping()
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_STOPGROUPTYPING, 0, 0);
		}
		BOOL Redo()
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, EM_REDO, 0, 0);
		}
		BOOL Undo()
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_UNDO, 0, 0);
		}
		void Clear()
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_CLEAR, 0, 0);
		}
		void Copy()
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_COPY, 0, 0);
		}
		void Cut()
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_CUT, 0, 0);
		}
		void Paste()
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_PASTE, 0, 0);
		}
		BOOL SetReadOnly(_In_ BOOL bReadOnly /* = TRUE */)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_SETREADONLY, bReadOnly, 0L);
		}
		int GetFirstVisibleLine() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, EM_GETFIRSTVISIBLELINE, 0, 0L);
		}
		BOOL DisplayBand(_In_ LPRECT pDisplayRect)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_DISPLAYBAND, 0, (LPARAM)pDisplayRect);
		}
		ivec3 GetSel() const
		{
			CHARRANGE cr = { 0 };
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
			return { cr.cpMin,cr.cpMax, cr.cpMax - cr.cpMin };
		}
		BOOL GetPunctuation(_In_ UINT fType, _Out_ PUNCTUATION* lpPunc) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, EM_GETPUNCTUATION, (WPARAM)fType, (LPARAM)lpPunc);
		}
		BOOL SetPunctuation(_In_ UINT fType, _In_ PUNCTUATION* lpPunc)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, EM_SETPUNCTUATION, (WPARAM)fType, (LPARAM)lpPunc);
		}
		// 设置文本上限
		void LimitText(_In_ long nChars)
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_EXLIMITTEXT, 0, nChars);
		}
		long LineFromChar(_In_ long nIndex) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_EXLINEFROMCHAR, 0, nIndex);
		}
		ivec2 PosFromChar(_In_ UINT nChar) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); POINTL pt; ::SendMessage(m_hWnd, EM_POSFROMCHAR, (WPARAM)&pt, nChar); return { pt.x, pt.y };
		}
		int CharFromPos(_In_ ivec2 pt) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); POINTL ptl = { pt.x, pt.y }; return (int)::SendMessage(m_hWnd, EM_CHARFROMPOS, 0, (LPARAM)&ptl);
		}
		void SetSel(ivec2 v2)
		{
			CHARRANGE cr = { v2.x, v2.y };
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);
		}
		DWORD FindWordBreak(_In_ UINT nCode, _In_ DWORD nStart) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (DWORD)::SendMessage(m_hWnd, EM_FINDWORDBREAK, (WPARAM)nCode, (LPARAM)nStart);
		}

#pragma push_macro("FindTextA")
#pragma push_macro("FindTextW")
#undef FindTextA
#undef FindTextW
		long FindText(_In_ DWORD dwFlags, _Out_ FINDTEXTEX* pFindText) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_FINDTEXTEX, dwFlags, (LPARAM)pFindText);
		}
#pragma pop_macro("FindTextA")
#pragma pop_macro("FindTextW")

		long FormatRange(_In_ FORMATRANGE* pfr, _In_ BOOL bDisplay)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_FORMATRANGE, (WPARAM)bDisplay, (LPARAM)pfr);
		}

		long GetEventMask() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_GETEVENTMASK, 0, 0L);
		}

		long GetLimitText() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_GETLIMITTEXT, 0, 0L);
		}

#pragma warning(push)
#pragma warning(disable: 6054)
		std::string GetSelText()
		{

			auto n = GetSel();
			std::string ret;
			ret.resize(n.z * 2 + 1);
			ASSERT_R(::IsWindow(m_hWnd)); auto hr = ::SendMessage(m_hWnd, EM_GETSELTEXT, 0, (LPARAM)ret.data());
			ret.resize(hr);
			return ret;
		}
#pragma warning(pop)

		void HideSelection(_In_ BOOL bHide, _In_ BOOL bPerm)
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_HIDESELECTION, bHide, bPerm);
		}

		void RequestResize()
		{
			ASSERT_R(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_REQUESTRESIZE, 0, 0L);
		}

		WORD GetSelectionType() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (WORD)::SendMessage(m_hWnd, EM_SELECTIONTYPE, 0, 0L);
		}

		UINT GetWordWrapMode() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (UINT) ::SendMessage(m_hWnd, EM_GETWORDWRAPMODE, 0, 0);
		}

		UINT SetWordWrapMode(_In_ UINT uFlags) const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (UINT) ::SendMessage(m_hWnd, EM_SETWORDWRAPMODE, (WPARAM)uFlags, 0);
		}

		COLORREF SetBackgroundColor(unsigned int col, BOOL bSysColor = FALSE)
		{
			col &= 0x00ffffff;
			ASSERT_R(::IsWindow(m_hWnd)); return (COLORREF)::SendMessage(m_hWnd, EM_SETBKGNDCOLOR, bSysColor, col);
		}

		DWORD SetEventMask(_In_ DWORD dwEventMask)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (DWORD)::SendMessage(m_hWnd, EM_SETEVENTMASK, 0, dwEventMask);
		}

		BOOL SetOLECallback(_In_ IRichEditOleCallback* pCallback)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_SETOLECALLBACK, 0, (LPARAM)pCallback);
		}

		BOOL SetTargetDevice(_In_ HDC hDC, _In_ long lLineWidth)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_SETTARGETDEVICE, (WPARAM)hDC, lLineWidth);
		}

		long StreamIn(_In_ int nFormat, EDITSTREAM& es)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_STREAMIN, nFormat, (LPARAM)&es);
		}

		long StreamOut(_In_ int nFormat, EDITSTREAM& es)
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_STREAMOUT, nFormat, (LPARAM)&es);
		}

		long GetTextLength() const
		{
			ASSERT_R(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, WM_GETTEXTLENGTH, NULL, NULL);
		}
		void clear_text()
		{
			SetSel({ 0,-1 });
			ReplaceSel("", TRUE);
		}

		void add_text(const std::string& s)
		{
			SetSel({ -1,0 });
			ReplaceSel(s.c_str(), TRUE);
		}
		//设置带格式文本, 范围为字符数量
		int set_font_info(std::string fontname, UINT fontsize, UINT color, ivec2 cr = { -1, -1 })
		{
			HWND richedit = m_hWnd;
			if (!IsWindow(m_hWnd))
			{
				return 1;
			}
#ifdef __utils_info_h__
			fontname = hz::u8_gbk(fontname);
#else 
#endif 
			CHARFORMAT cf;
			ZeroMemory(&cf, sizeof(CHARFORMAT));
			cf.cbSize = sizeof(CHARFORMAT);
			cf.dwMask = CFM_BOLD | CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_SIZE | CFM_UNDERLINE;
			cf.dwEffects = 0;
			cf.yHeight = fontsize;//文字高度
			cf.crTextColor = color & 0x00ffffff;// RGB(color.x, color.y, color.z); //文字颜色
			memcpy(cf.szFaceName, fontname.c_str(), fontname.length());//设置字体
			unsigned int uflags = SCF_SELECTION;
			::SendMessage(richedit, EM_SETSEL, cr.x, cr.y);
			auto hr = ::SendMessage(richedit, EM_SETCHARFORMAT, (WPARAM)uflags, (LPARAM)&cf);
			return hr;
		}

		void set_replacesel_text(std::string str, bool isbm = true)
		{
			if (!IsWindow(m_hWnd))
			{
				return;
			}
#ifdef __utils_info_h__ 
			str = hz::u8_gbk(str);
#else 
#endif 
			if (str.size())
			{
				::SendMessage(m_hWnd, EM_REPLACESEL, 0, (LPARAM)str.c_str());
			}
			if (isbm)
				::SendMessage(m_hWnd, WM_VSCROLL, SB_BOTTOM, 0);
		}
		njson get_char_format()
		{
			CHARFORMAT cf;
			ZeroMemory(&cf, sizeof(CHARFORMAT));
			cf.cbSize = sizeof(CHARFORMAT);
			::SendMessage(m_hWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
			njson ret;
			ret["mask"] = cf.dwMask;
			ret["effects"] = cf.dwEffects;
			ret["y_height"] = cf.yHeight;
			ret["y_offset"] = cf.yOffset;
			ret["color"] = cf.crTextColor;
			ret["b_char_set"] = cf.bCharSet;
			ret["b_pitch_family"] = cf.bPitchAndFamily;
			ret["face_name"] = cf.szFaceName;
			return ret;
		}
	private:

	};

	//--------------------------------------------------------------------------------------------------------------------------------
	class wmsg
	{
	public:
		HWND _root = nullptr;
		std::function<void(HWND hdlg)> initdialog_func;
		std::function<unsigned int(unsigned int msg, uint64_t wp, void* lp)> command_func, msg_proc_func;
	public:
		wmsg()
		{
			static auto riched = LoadLibrary("riched20.dll");
			static auto mscomctl = LoadLibrary("mscomctl.ocx");
			initdialog_func = [](HWND hdlg) {};
			command_func = [](unsigned int msg, uint64_t wp, void* lp) {return 0; };
			msg_proc_func = [](unsigned int msg, uint64_t wp, void* lp) { return 0; };
			//auto riched = LoadLibrary("msftedit.dll");
		}
		virtual ~wmsg() {}
	public:
		template<class _Ty, class _T2>
		auto mapvector(_Ty& dst, const _T2& t2)
		{
			size_t ds = dst.size();
			dst.resize(ds + t2.size());
			for (auto it : t2)
			{
				dst[ds++] = (HWND)it;
			}
			return dst.size();
		}
		ctrl_base get_item(int id)
		{
			return GetDlgItem(_root, id);
		}

		auto get_items(std::vector<int> id)
		{
			std::vector<ctrl_base> ret;
			for (auto it : id)
			{
				auto p = GetDlgItem(_root, it);
				ret.push_back(p);
			}
			return ret;
		}
		auto get_item_text(int id)
		{
			std::string s;
			//memset()
			s.resize(1024);
			unsigned int c = GetDlgItemText(_root, id, s.data(), s.size());
			s.resize(c);
			return s;
		}


	public:
#if 0
		// 可读json或cbor
		static njson read_json(std::string fn)
		{
			if (fn[1] != ':')
			{
				fn = hz::File::getAP(fn);
			}
			auto v = hz::File::read_binary_file(fn);
			njson ret;
			try
			{
				if (hz::is_json(v.data(), v.size()))
				{
					ret = njson::parse(v.begin(), v.end());
				}
				else
				{
					ret = njson::from_cbor(v.begin(), v.end());
				}
			}
			catch (const std::exception&)
			{

			}
			return ret;
		}
		// 保存json到文件，-1则保存成cbor
		static void save_json(const njson& n, std::string fn, int indent_cbor = 0)
		{
			if (fn[1] != ':')
			{
				fn = hz::File::getAP(fn);
			}
			if (indent_cbor < 0)
			{
				auto s = njson::to_cbor(n);
				if (s.size())
					hz::File::save_binary_file(fn, (char*)s.data(), s.size());
			}
			else
			{
				std::string s;
				if (indent_cbor == 0)
				{
					s = n.dump();
				}
				else
				{
					s = n.dump(indent_cbor);
				}
				if (s.size())
					hz::File::save_binary_file(fn, (char*)s.data(), s.size());
			}
		}
		void mk_app_dir(std::string& fn)
		{
			if (fn[1] != ':')
			{
				//fn = hz::File::getAP(fn);
			}
		}
#endif
	public:

		int run_param(int dialog)
		{
			auto a = DialogBoxParam(0, MAKEINTRESOURCE(dialog), nullptr, (DLGPROC)DlgProc, (LPARAM)this);
			return a;
		}
		static INT_PTR CALLBACK DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
		{
			switch (msg)
			{
			case WM_INITDIALOG:
			{
				wmsg* p = (wmsg*)lp;
				SetProp(hdlg, "app_ptr", p);
				p->initdialog_func(hdlg);

			}
			break;
			case WM_COMMAND:
			{
				wmsg* p = (wmsg*)GetProp(hdlg, "app_ptr");
				if (p)
				{
					auto hr = p->command_func(msg, LOWORD(wp), (void*)lp);
					if (hr != 0)
					{
						return hr;
					}
				}
			}
			break;
			default:
			{
				wmsg* p = (wmsg*)GetProp(hdlg, "app_ptr");
				if (p)
				{
					auto hr = p->msg_proc_func(msg, (uint64_t)wp, (void*)lp);
					if (hr != 0)
					{
						return hr;
					}
				}
				break;
			}
			};
			return 0;
		}
	private:
	};//!wmsg
};//!hwm
#endif // !__WIN32MSG_H__
