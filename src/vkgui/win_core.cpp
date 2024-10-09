
#ifdef _WIN32

#include <WinSock2.h>
#include <windows.h>
#include <Shlobj.h>
#include <shellapi.h>
#include <Shlwapi.h>
//-----------------------------------------------------------------------------
#pragma comment(lib,"Shell32.lib")
#pragma comment(lib,"shlwapi.lib")
#include <comdef.h>//这个头文件是必须的，要不编译时会有很多错误。 

//#include <ntype.h>
#include <pch1.h>
#include <vector>
#include <functional>
#include <string> 
#include <thread>

#include <win_core.h> 
#include <mapView.h>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.UI.Xaml.Media.Imaging.h>


#include <stb_image_write.h>

namespace hz {

	HMONITOR GetPrimaryMonitor()
	{
		POINT ptZero = { 0, 0 };
		return MonitorFromPoint(ptZero,
			MONITOR_DEFAULTTOPRIMARY);
	}


	float GetMonitorScalingRatio(HMONITOR monitor, int* x, int* y)
	{
		MONITORINFOEX info = { };
		info.cbSize = sizeof(info);
		GetMonitorInfo(monitor, &info);
		DEVMODE devmode = {};
		devmode.dmSize = sizeof(DEVMODE);
		EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
		if (x)
		{
			*x = devmode.dmPelsWidth;
		}
		if (y)
		{
			*y = devmode.dmPelsHeight;
		}
		return static_cast<float>(devmode.dmPelsWidth) / (info.rcMonitor.right - info.rcMonitor.left);
	}
	float get_monitor_scalev2(glm::ivec2* pels)
	{
		auto h = GetPrimaryMonitor();
		if (pels)
			return GetMonitorScalingRatio(h, &pels->x, &pels->y);
		else
			return GetMonitorScalingRatio(h, 0, 0);
	}
	float get_monitor_scale(void* pels)
	{
		return get_monitor_scalev2((glm::ivec2*)pels);
	}
	static void enumPrinters(std::vector<std::string>& out)
	{
		DWORD Flags = PRINTER_ENUM_FAVORITE | PRINTER_ENUM_LOCAL;
		DWORD cbBuf = 0;
		DWORD pcReturned = 0;

		DWORD Level = 2;
		TCHAR Name[500] = { 0 };

		//printf("打印机开始查询\n");

		//第一次调用得到结构体的大小
		::EnumPrinters(Flags, Name, Level,
			NULL,
			0,
			&cbBuf, //需要多少内存 
			&pcReturned);
		if (cbBuf < 1)
		{
			return;
		}
		const LPPRINTER_INFO_2 pPrinterEnum = (LPPRINTER_INFO_2)LocalAlloc(LPTR, cbBuf + 4);

		if (!pPrinterEnum)
		{
			printf("error is %d", GetLastError());
			return;
		}

		if (EnumPrinters(Flags, Name, Level, (LPBYTE)pPrinterEnum, cbBuf, &cbBuf, &pcReturned))
		{

			out.clear();
			//printf("打印机数量 %d\n", pcReturned);
			for (unsigned int i = 0; i < pcReturned; i++)
			{
				//插入一行
				LPPRINTER_INFO_2 pInfo = &pPrinterEnum[i];
				std::string str = pInfo->pPrinterName;
				//str += ";"; str += (pInfo->pServerName) ? pInfo->pServerName : "";
				//str += ";"; str += (pInfo->pDriverName) ? pInfo->pDriverName : "";
				//str += ";"; str += (pInfo->pPrintProcessor) ? pInfo->pPrintProcessor : "";
				out.push_back(str);
			}
		}
		LocalFree(pPrinterEnum);
	}
	std::string statusstr(int x)
	{
		std::string str;
		if (x & PRINTER_STATUS_BUSY)str += (char*)u8"打印机正忙。\t";
		if (x & PRINTER_STATUS_DOOR_OPEN)str += (char*)u8"打印机门已打开。\t";
		if (x & PRINTER_STATUS_ERROR)str += (char*)u8"打印机处于错误状态。\t";
		if (x & PRINTER_STATUS_INITIALIZING)str += (char*)u8"打印机正在初始化。\t";
		if (x & PRINTER_STATUS_IO_ACTIVE)str += (char*)u8"打印机处于活动输入 / 输出状态\t";
		if (x & PRINTER_STATUS_MANUAL_FEED)str += (char*)u8"打印机处于手动馈送状态。\t";
		if (x & PRINTER_STATUS_NO_TONER)str += (char*)u8"打印机墨粉用完。\t";
		if (x & PRINTER_STATUS_NOT_AVAILABLE)str += (char*)u8"打印机不可用于打印。\t";
		if (x & PRINTER_STATUS_OFFLINE)str += (char*)u8"打印机处于脱机状态。\t";
		if (x & PRINTER_STATUS_OUT_OF_MEMORY)str += (char*)u8"打印机内存不足。\t";
		if (x & PRINTER_STATUS_OUTPUT_BIN_FULL)str += (char*)u8"打印机的输出纸盒已满。\t";
		if (x & PRINTER_STATUS_PAGE_PUNT)str += (char*)u8"打印机无法打印当前页。\t";
		if (x & PRINTER_STATUS_PAPER_JAM)str += (char*)u8"纸张卡在打印机中\t";
		if (x & PRINTER_STATUS_PAPER_OUT)str += (char*)u8"打印机缺纸。\t";
		if (x & PRINTER_STATUS_PAPER_PROBLEM)str += (char*)u8"打印机有纸张问题。\t";
		if (x & PRINTER_STATUS_PAUSED)str += (char*)u8"打印机已暂停。\t";
		if (x & PRINTER_STATUS_PENDING_DELETION)str += (char*)u8"正在删除打印机。\t";
		if (x & PRINTER_STATUS_POWER_SAVE)str += (char*)u8"打印机处于节能模式。\t";
		if (x & PRINTER_STATUS_PRINTING)str += (char*)u8"打印机正在打印。\t";
		if (x & PRINTER_STATUS_PROCESSING)str += (char*)u8"打印机正在处理打印作业。\t";
		if (x & PRINTER_STATUS_SERVER_UNKNOWN)str += (char*)u8"打印机状态未知。\t";
		if (x & PRINTER_STATUS_TONER_LOW)str += (char*)u8"打印机的碳粉不足。\t";
		if (x & PRINTER_STATUS_USER_INTERVENTION)str += (char*)u8"打印机有一个错误，要求用户执行某些操作。\t";
		if (x & PRINTER_STATUS_WAITING)str += (char*)u8"打印机正在等待。\t";
		if (x & PRINTER_STATUS_WARMING_UP)str += (char*)u8"打印机正在预热。\t";
		return str;
	}

	std::vector<std::string> get_print_devname()
	{
		std::vector<std::string> print_name;
		enumPrinters(print_name);
		return print_name;
	}

	class print_dc
	{
	public:
		struct value_pt
		{
			std::string n, n1;
			glm::ivec2	ps;
			int			paper;
		};
		HDC hdcPrint = NULL;
		// 打印机名称列表
		std::vector<std::string> print_name;
		//尺寸，分辨率
		std::vector<value_pt> _v;
		std::vector<glm::ivec2> _ert;
		glm::ivec2 size = {};
		glm::ivec2 sizei = {};
		glm::ivec4 hpt = {};
		PRINTER_INFO_2W* pPrinterData = 0;
		std::vector<BYTE> pbuf;
	public:
		print_dc();
		~print_dc();
		static print_dc* new_pdc();
		static void free_pdc(print_dc* p);
		void set_name(const std::string& prname);
		void set_value(std::string u_paper, int dir = 1);
		void begin(const std::string& title);
		void end();
	private:
		std::string getPrn(const std::string& prname);
		glm::ivec4 get_hpt();
		bool getname(const std::string& prname, PRINTER_INFO_2W** pPrinterData, std::vector<BYTE>& pbuf);
		int getPrintDeviceW(std::wstring printname, std::vector<value_pt>& out);
	};

	print_dc::print_dc()
	{
		enumPrinters(print_name);
	}
	// print_dc::
	print_dc::~print_dc()
	{
	}
	print_dc* print_dc::new_pdc()
	{
		return new print_dc();
	}
	void print_dc::free_pdc(print_dc* p)
	{
		if (p)delete p;
	}
	void print_dc::begin(const std::string& title)
	{
		if (!hdcPrint)return;
		std::wstring ws = u8_to_u16(title);
		DOCINFOW di = {};
		char aa[1024] = {};
		di.cbSize = sizeof(DOCINFOW);
		di.lpszDocName = ws.c_str();
		//di.lpszDocName = title.c_str();
		di.lpszOutput = NULL;
		// todo 调试会异常winrt::hresult_error
		try
		{
			StartDocW(hdcPrint, &di);
			//auto thumbnail{ co_await imageFile.GetThumbnailAsync(FileProperties::ThumbnailMode::PicturesView) };
			//if (thumbnail) bitmapImage.SetSource(thumbnail);
		}
		catch (winrt::hresult_error const& ex)
		{
			winrt::hresult hr = ex.code(); // HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND).
			winrt::hstring message = ex.message(); // The system cannot find the file specified.
			auto c = message.c_str();
			printf("%ws\n", c);
		}
		StartPage(hdcPrint);
		//Escape(hdcPrint, STARTDOC, title.size(), (LPSTR)title.c_str(), NULL);		
	}
	void print_dc::end()
	{
		if (!hdcPrint)return;
		//Escape(hdcPrint, NEWFRAME, 0, NULL, NULL);
		//Escape(hdcPrint, ENDDOC, 0, NULL, NULL);
		EndPage(hdcPrint);
		EndDoc(hdcPrint);

		//删除打印机DC。  
		DeleteDC(hdcPrint);
	}
	std::string print_dc::getPrn(const std::string& prname)
	{
		std::string ret;
		if (prname != "")
		{
			for (auto it : print_name)
			{
				if (it.find(prname) != std::string::npos)
				{
					ret = it;
					break;
				}
			}
		}
		return ret;
	}
	void print_dc::set_name(const std::string& prname)
	{
		wchar_t szDriver[16] = L"WINSPOOL";

		auto ol = getname(prname, &pPrinterData, pbuf);
		if (ol && pPrinterData && pPrinterData->Status == 0)
		{
			assert(!hdcPrint);
			//创建打印DC
			hdcPrint = CreateDCW(szDriver, pPrinterData->pDriverName, 0 /*pPrinterData->pPortName*/, NULL);
			//hdcPrint = printInfo.hDC;
		}
		else {
			if (pPrinterData && pPrinterData->Status)
			{
				auto ks = statusstr(pPrinterData->Status);
				if (ks.size())
				{
					printf("%s\n", ks.c_str());
				}
				return;
			}
		}
	}
	void print_dc::set_value(std::string u_paper, int dir)
	{
		if (hdcPrint)
		{
			//获取打印的时候的dc，然后往这个dc上绘图就是打印出来的样子了  

			//锁定全局对象，获取对象指针。 devmode是有关设备初始化和打印机环境的信息
			GlobalLock(pPrinterData->pDevMode);
			DEVMODEW* devMode = (DEVMODEW*)pPrinterData->pDevMode;
			if (devMode == 0)
			{
				printf((char*)u8"获取打印机设置时发生了错误.\n");
				ResetDCW(hdcPrint, devMode);
				//解锁全局对象，对应GlobalLock
				GlobalUnlock(pPrinterData->pDevMode);
				//删除打印机DC。  
				DeleteDC(hdcPrint); hdcPrint = 0;
				return;
			}
			if (_v.size() > 0)
			{
				size = { devMode->dmPaperWidth, devMode->dmPaperLength };
				for (size_t i = 0; i < _v.size(); i++)
				{
					auto& it = _v[i];
					if (it.n.find(u_paper) != std::string::npos || it.n1.find(u_paper) != std::string::npos)
					{
						devMode->dmPaperSize = it.paper;               //打印机纸张设置。
						devMode->dmPaperWidth = it.ps.x;
						devMode->dmPaperLength = it.ps.y;
						size = it.ps;
					}
				}
			}
			DMORIENT_LANDSCAPE;
			devMode->dmOrientation = dir;		       //打印方向设置
			// DMORIENT_PORTRAIT; 纵向
			// DMORIENT_LANDSCAPE 是横向打印
			//对打印方向的设置，会影响hPrintDC的大小，假设宽度为1024，高度为300
			//则在横向打印的时候dc大小会是宽1024 * 高300
			//而纵向打印的时候dc大小会是宽300 * 高1024

			int printQuality = devMode->dmPrintQuality;          //获取打印机的打印质量
			//devMode->dmPrintQuality = DMRES_MEDIUM;
			//设置打印质量的，因为像素被打印到纸上的时候是有做转换的
			//单位是dpi，意为像素每英寸(dots per inch)。就是一英寸的纸张上
			//打印多少个像素点，意味着这个质量越高，打印结果越精细，越低，越粗糙
			//设置的质量可以是具体数值，也可以是宏DMRES_MEDIUM
			//一般我们选择300，或者600，DMRES_MEDIUM = 600dpi

			//应用我们修改过后的设置.
			ResetDCW(hdcPrint, devMode);
			//解锁全局对象，对应GlobalLock
			GlobalUnlock(pPrinterData->pDevMode);
			get_hpt();
		}
	}

	glm::ivec4 print_dc::get_hpt()
	{
		int cx = GetDeviceCaps(hdcPrint, LOGPIXELSX), cy = GetDeviceCaps(hdcPrint, LOGPIXELSY);
		int cw = GetDeviceCaps(hdcPrint, HORZSIZE), ch = GetDeviceCaps(hdcPrint, VERTSIZE);
		int dc_page_width = cw / 25.4 * cx;
		int dc_page_height = ch / 25.4 * cy;
		sizei = { cw,ch };
		hpt = { cx,cy,dc_page_width, dc_page_height };
		return hpt;
	}
	bool print_dc::getname(const std::string& prname, PRINTER_INFO_2W** pPrinterData, std::vector<BYTE>& pbuf)
	{
		wchar_t szPrinter[1024];
		DWORD cchBuffer = 1024;
		HDC hdcPrint = NULL;
		HANDLE hPrinter = NULL;

		BYTE* pdBuffer = 0;//[16384];
		BOOL bReturn = FALSE;

		DWORD cbBuf = 0;
		DWORD cbNeeded = 0;

		auto pn = getPrn(prname);

		std::wstring szw;
		if (pn.size())
		{
			szw = gbk_to_u16(pn);
			//sprintf(szPrinter, "%s", pn.c_str());
		}
		//else if (prname.size())
		//{
		//	return;
		//}
		else {
			//获取默认打印机名称
			GetDefaultPrinterW(szPrinter, &cchBuffer);
			szw = szPrinter;
			szw.resize(cchBuffer);
		}

		getPrintDeviceW(szw, _v);
		//if (bReturn) 
		{
			//打开默认打印机
			bReturn = OpenPrinterW((LPWSTR)szw.c_str(), &hPrinter, NULL);
		}

		if (bReturn)
		{
			//获取打印机端口名称
			auto ks = GetPrinterW(hPrinter, 2, 0, 0, &cbNeeded);
			if (cbNeeded > 0)
			{
				pbuf.resize(cbNeeded);
				pdBuffer = pbuf.data();
				cbBuf = cbNeeded;
				bReturn = GetPrinterW(hPrinter, 2, &pdBuffer[0], cbBuf, &cbNeeded);
			}
			//这个句柄不再需要了
			ClosePrinter(hPrinter);
		}
		else
		{
			return false;
		}
		*pPrinterData = (PRINTER_INFO_2W*)pdBuffer;
		//status = '脱机''在线'
		return pPrinterData && (*pPrinterData) && !((*pPrinterData)->Status & PRINTER_STATUS_OFFLINE);

	}
	int print_dc::getPrintDeviceW(std::wstring printname, std::vector<value_pt>& out)
	{
		auto lpt1w = L"";//LPT1
		int nNeeded = DeviceCapabilitiesW(printname.c_str(), lpt1w, DC_PAPERNAMES, NULL, NULL);
		int duplex = 0;
		std::vector<unsigned short> ps;
		std::vector<POINT> pss;
		std::vector<POINT> ert;
		if (nNeeded)
		{
			out.clear();
			std::vector<wchar_t> pszPaperNames; //分配纸张名称数组 
			//获得可用打印机纸张 
			pss.resize(nNeeded);
			ps.resize(nNeeded);
			pszPaperNames.resize(nNeeded * 64);
			wchar_t mystr[64] = { 0 };
			if (DeviceCapabilitiesW(printname.c_str(), lpt1w, DC_PAPERS, (LPWSTR)ps.data(), NULL) != -1)
			{
			}
			duplex = DeviceCapabilitiesW(printname.c_str(), lpt1w, DC_DUPLEX, 0, 0);
			if (DeviceCapabilitiesW(printname.c_str(), lpt1w, DC_PAPERSIZE, (LPWSTR)pss.data(), NULL) != -1)
			{

			}
			if (DeviceCapabilitiesW(printname.c_str(), lpt1w, DC_PAPERNAMES, pszPaperNames.data(), NULL) != -1)
			{
				//复制列表
				for (int i = 0; i < nNeeded; i++)
				{
					for (int j = 0; j < 64; j++)
					{
						mystr[j] = pszPaperNames[j + i * 64];
					}
					auto mw1 = hz::u16_to_u8(mystr);
					auto mw = hz::u16_to_gbk(mystr);
					value_pt v = {};
					v.n = mw1;
					v.n1 = mw;
					v.paper = ps[i];
					v.ps = { pss[i].x, pss[i].y };
					out.push_back(v);
				}
			}
			int n = DeviceCapabilitiesW(printname.c_str(), lpt1w, DC_ENUMRESOLUTIONS, 0, NULL);
			if (n > 0)
			{
				ert.resize(n);
				DeviceCapabilitiesW(printname.c_str(), lpt1w, DC_ENUMRESOLUTIONS, (LPWSTR)ert.data(), NULL);
				_ert.clear();
				_ert.reserve(n);
				for (auto& it : ert)
				{
					_ert.push_back({ it.x,it.y });
				}
			}
		}
		return nNeeded;
	}
#if 0
	void copyToDc(HDC hdc, const glm::ivec2& pos, int ct)
	{
		if (_data.empty() || !(width > 0 && height > 0))
		{
			return;
		}
		memset(&bmpInfo, 0, sizeof(bmpInfo.bmiHeader));
		bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
		bmpInfo.bmiHeader.biWidth = width;
		bmpInfo.bmiHeader.biHeight = -height;
		bmpInfo.bmiHeader.biPlanes = 1;
		bmpInfo.bmiHeader.biBitCount = 32;
		bmpInfo.bmiHeader.biCompression = BI_BITFIELDS;

		if (ct == 0)
		{
			//RGB
			*(unsigned int*)(bmpInfo.bmiColors + 2) = 0xFF0000;	// red分量
			*(unsigned int*)(bmpInfo.bmiColors + 1) = 0x00FF00;	// green分量
			*(unsigned int*)(bmpInfo.bmiColors + 0) = 0x0000FF;	// blue分量
		}
		else
		{
			//BGR
			*(unsigned int*)(bmpInfo.bmiColors + 0) = 0xFF0000;	// red分量
			*(unsigned int*)(bmpInfo.bmiColors + 1) = 0x00FF00;	// green分量
			*(unsigned int*)(bmpInfo.bmiColors + 2) = 0x0000FF;	// blue分量
		}
		::SetDIBitsToDevice(hdc,
			pos.x, pos.y, width, height,
			0, 0, 0, height,
			_data.data(), &bmpInfo, DIB_RGB_COLORS);

	}
	void copyToDcS(HDC hdc, glm::ivec2 hdcsize, const glm::ivec2& pos)
	{
		if (_data.empty() || !(width > 0 && height > 0))
		{
			return;
		}
		int ct = 0;
		memset(&bmpInfo, 0, sizeof(bmpInfo.bmiHeader));
		bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
		bmpInfo.bmiHeader.biWidth = width;
		bmpInfo.bmiHeader.biHeight = -height;
		bmpInfo.bmiHeader.biPlanes = 1;
		bmpInfo.bmiHeader.biBitCount = 32;
		bmpInfo.bmiHeader.biCompression = BI_BITFIELDS;

		if (ct == 0)
		{
			//RGB
			*(unsigned int*)(bmpInfo.bmiColors + 2) = 0xFF0000;	// red分量
			*(unsigned int*)(bmpInfo.bmiColors + 1) = 0x00FF00;	// green分量
			*(unsigned int*)(bmpInfo.bmiColors + 0) = 0x0000FF;	// blue分量
		}
		else
		{
			//BGR
			*(unsigned int*)(bmpInfo.bmiColors + 0) = 0xFF0000;	// red分量
			*(unsigned int*)(bmpInfo.bmiColors + 1) = 0x00FF00;	// green分量
			*(unsigned int*)(bmpInfo.bmiColors + 2) = 0x0000FF;	// blue分量
		}
		if (hdcsize.x < 1 || hdcsize.y < 1)
		{
			hdcsize = { width, height };
		}
		StretchDIBits(hdc, pos.x, pos.y, hdcsize.x, hdcsize.y, 0, 0, width, height, _data.data(), &bmpInfo, DIB_RGB_COLORS, SRCCOPY);
	}
#endif
	void print_grid(HDC hdc, int cx, glm::ivec4 hpt, glm::ivec2 size, bool high)
	{
#if 0
		Image img[1];
		glm::ivec2 ks = { hpt.z, hpt.w };
		if (high)
		{
			size = { hpt.z, hpt.w };
		}
		img->resize(size.x, size.y);
		img->clear_color(-1);
		int ccx = cx;
		if (cx < 2)
		{
			ccx = 20;
		}
		int wn = size.x / ccx, hn = size.y / ccx;
		wn++; hn++;
		for (int x = 0; x < wn; x++)
		{
			glm::ivec2 pos = { x * ccx, 0 };
			img->draw_line({ pos.x,pos.y,pos.x,img->height }, 0xff000000);
		}
		for (int y = 0; y < hn; y++)
		{
			glm::ivec2 pos = { 0, y * ccx };
			img->draw_line({ pos.x,pos.y,img->width,pos.y }, 0xff000000);
		}
		//输出图像打印到DC
		glm::ivec2 pos1 = { 1,1 };
		if (high)
			img->copyToDc(hdc, pos1);		// 直接复制到目标dc
		else
			img->copyToDcS(hdc, ks, pos1);	//缩放到目标dc
#endif
	}
	void testprint()
	{
		print_dc pt;
		pt.set_name("PDF");//选择打印机
		pt.set_value("A5", 1);//纸张大小、方向
		pt.begin((char*)u8"标题");
		print_grid(pt.hdcPrint, 200, pt.hpt, pt.size, 1);
		pt.end();
		exit(0);
		system("pause");
	}

	void setClipboard(const char* text, int len)
	{
		//打开剪切板
		if (OpenClipboard(0) && len > 0)
		{
			HGLOBAL hClip;
			TCHAR* pBuf = nullptr;
			if (text == "")
			{
				CloseClipboard();
				return;
			}
			//清空剪切板内容
			EmptyClipboard();
			//分配新全局内存空间
			hClip = GlobalAlloc(GHND, len + 1);
			//锁住全局内存空间
			pBuf = (TCHAR*)GlobalLock(hClip);
			//将内容写入全局内存空间
			memcpy(pBuf, text, len);
			//将空间中的内容写入剪切板
#ifndef UNICODE
			SetClipboardData(CF_TEXT, hClip);         //设置数据
#else
			SetClipboardData(CF_UNICODETEXT, hClip);         //设置数据
#endif
			//解锁全局内存空间
			GlobalUnlock(hClip);         //解锁
			//释放全局内存空间
			GlobalFree(hClip);
			//关闭剪切板
			CloseClipboard();
		}
	}

	void setClipboardW(const std::wstring& text)
	{
		//打开剪切板
		if (!text.empty() && OpenClipboard(0))
		{
			HGLOBAL hClip;
			TCHAR* pBuf = nullptr;
			if (text.empty())
			{
				CloseClipboard();
				return;
			}
			//清空剪切板内容
			EmptyClipboard();
			//分配新全局内存空间
			size_t len = (text.length() + 1) * sizeof(wchar_t);
			hClip = GlobalAlloc(GHND, len);
			//锁住全局内存空间
			pBuf = (TCHAR*)GlobalLock(hClip);
			//将内容写入全局内存空间
			memcpy(pBuf, text.c_str(), len);
			//将空间中的内容写入剪切板
			SetClipboardData(CF_UNICODETEXT, hClip);         //设置数据
			//解锁全局内存空间
			GlobalUnlock(hClip);         //解锁
			//释放全局内存空间
			GlobalFree(hClip);
			//关闭剪切板
			CloseClipboard();
		}
	}
	const char* getClipboard(int& type)
	{
		char* t = 0;
		if (IsClipboardFormatAvailable(CF_UNICODETEXT))
			type = CF_UNICODETEXT;
		else if (IsClipboardFormatAvailable(CF_TEXT))
			type = CF_TEXT;

		if ((type == CF_UNICODETEXT || type == CF_TEXT) && OpenClipboard(0))
		{
			HGLOBAL hMem = GetClipboardData(type);
			t = (char*)GlobalLock(hMem);
			GlobalUnlock(hMem);
			CloseClipboard();
		}
		return t;
	}

	int vkcode(const std::string& vks)
	{
		static std::unordered_map<std::string, int> vk_code = {
			{ "back",0x08}, { "tab" ,0x09}, { "clear" ,0x0C}
			,{ "enter" ,0x0D},{ "shift" ,0x10},{ "ctrl" ,0x11},{ "alt" ,0x12}
			,{ "pause" ,0x13},{ "caps_lock" ,0x14},{ "esc" ,0x1B}
			,{ "space" ,0x20}
			,{ "page_up" ,0x21},{ "page_down" ,0x22},{ "end" ,0x23},{ "home" ,0x24}
			,{ "left_arrow" ,0x25},{ "up_arrow" ,0x26},{ "right_arrow" ,0x27},{ "down_arrow" ,0x28}
			,{ "select" ,0x29},{ "print" ,0x2A},{ "execute" ,0x2B},{ "print_screen" ,0x2C}
			,{ "ins" ,0x2D},{ "del" ,0x2E},{ "help" ,0x2F}
			,{ "0" ,0x30},{ "1" ,0x31},{ "2" ,0x32},{ "3" ,0x33},{ "4" ,0x34},{ "5" ,0x35},{ "6" ,0x36},{ "7" ,0x37},{ "8" ,0x38},{ "9" ,0x39}
			,{ "a" ,0x41},{ "b" ,0x42},{ "c" ,0x43},{ "d" ,0x44},{ "e" ,0x45},{ "f" ,0x46},{ "g" ,0x47},{ "h" ,0x48},{ "i" ,0x49},{ "j" ,0x4A},{ "k" ,0x4B},{ "l" ,0x4C},{ "m" ,0x4D},{ "n" ,0x4E},{ "o" ,0x4F},{ "p" ,0x50},{ "q" ,0x51},{ "r" ,0x52},{ "s" ,0x53},{ "t" ,0x54},{ "u" ,0x55},{ "v" ,0x56},{ "w" ,0x57},{ "x" ,0x58},{ "y" ,0x59},{ "z" ,0x5A}
			,{ "lwin", 0x5b },{ "rwin", 0x5c },{ "apps", 0x5d },{ "sleep", 0x5f }
			,{ "numpad_0" ,0x60},{ "numpad_1" ,0x61},{ "numpad_2" ,0x62},{ "numpad_3" ,0x63},{ "numpad_4" ,0x64},{ "numpad_5" ,0x65},{ "numpad_6" ,0x66},{ "numpad_7" ,0x67},{ "numpad_8" ,0x68},{ "numpad_9" ,0x69}
			,{ "multiply_key" ,0x6A},{ "add_key" ,0x6B},{ "separator_key" ,0x6C},{ "subtract_key" ,0x6D},{ "decimal_key" ,0x6E},{ "divide_key" ,0x6F}
			,{ "F1" ,0x70},{ "F2" ,0x71},{ "F3" ,0x72},{ "F4" ,0x73},{ "F5" ,0x74},{ "F6" ,0x75},{ "F7" ,0x76},{ "F8" ,0x77},{ "F9" ,0x78},{ "F10" ,0x79},{ "F11" ,0x7A},{ "F12" ,0x7B}
			,{ "F13" ,0x7C},{ "F14" ,0x7D},{ "F15" ,0x7E},{ "F16" ,0x7F},{ "F17" ,0x80},{ "F18" ,0x81},{ "F19" ,0x82},{ "F20" ,0x83},{ "F21" ,0x84},{ "F22" ,0x85},{ "F23" ,0x86},{ "F24" ,0x87}
			,{ "num_lock" ,0x90},{ "scroll_lock" ,0x91}
			,{ "left_shift" ,0xA0},{ "right_shift " ,0xA1}
			,{ "left_control" ,0xA2},{ "right_control" ,0xA3}
			,{ "left_menu" ,0xA4},{ "right_menu" ,0xA5}
			,{ "browser_back" ,0xA6},{ "browser_forward" ,0xA7},{ "browser_refresh" ,0xA8},{ "browser_stop" ,0xA9},{ "browser_search" ,0xAA},{ "browser_favorites" ,0xAB},{ "browser_start_and_home" ,0xAC}
			,{ "volume_mute" ,0xAD},{ "volume_Down" ,0xAE},{ "volume_up" ,0xAF}
			,{ "next_track" ,0xB0},{ "previous_track" ,0xB1},{ "stop_media" ,0xB2},{ "play/pause_media" ,0xB3}
			,{ "start_mail" ,0xB4},{ "select_media" ,0xB5},{ "start_application_1" ,0xB6},{ "start_application_2" ,0xB7}
			,{ "attn_key" ,0xF6},{ "crsel_key" ,0xF7},{ "exsel_key" ,0xF8},{ "play_key" ,0xFA},{ "zoom_key" ,0xFB},{ "clear_key" ,0xFE}
			,{ "+" ,0xBB},{ "=" ,0xBB}
			,{ "," ,0xBC},{ "<" ,0xBC}
			,{ "-" ,0xBD},{ "_" ,0xBD}
			,{ "." ,0xBE} ,{ ">" ,0xBE}
			,{ "/" ,0xBF},{ "?" ,0xBF}
			,{ "`" ,0xC0},{ "~" ,0xC0}
			,{ ";" ,0xBA},{ ":" ,0xBA}
			,{ "[" ,0xDB},{ "{" ,0xDB}
			,{ "\\" ,0xDC},{ "|" ,0xDC}
			,{ "]" ,0xDD},{ "}" ,0xDD}
			,{ "'" ,0xDE},{ "\"" ,0xde}
		};
		auto it = vk_code.find(vks);
		return it != vk_code.end() ? it->second : 0;
	}

	//
// Usage: SetThreadName (-1, "MainThread");
//
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // must be 0x1000
		LPCSTR szName; // pointer to name (in user addr space)
		DWORD dwThreadID; // thread ID (-1=caller thread)
		DWORD dwFlags; // reserved for future use, must be zero
	} THREADNAME_INFO;

	void SetThreadName(unsigned long dwThreadID, const char* szThreadName)
	{
		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = szThreadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;

		__try
		{
			RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_CONTINUE_EXECUTION)
		{
		}
	}


	// 串口

	class port_cx
	{
	public:
		port_cx();
		~port_cx();
		static port_cx* new_port(int size);
		void init(int buflen = 1024);
		//---------------------------------------------------------------------------------

		//1、打开串口—默认异步方式
		void* Open_driver(const char* name, uint32_t BaudRate, int idx = 6, bool isSA = false);

		//2、  串口发送(写操作)
		unsigned char Send_driver(unsigned char* data, int len);

		//3、  串口接收(读操作)
		unsigned char Receive_driver();

		//4、  关闭串口
		int Close_driver();

		int send_data(const void* data, int len);
		int receive();

		void updateSend();
		void updateRece();

		void setRead(bool is);
		std::string getData();
		void endRead();
	public:
		void* _fd = 0;							//串口
		int _sendSta = 0, _receSta = 0;			//异步发送读取状态 1正在进行,2成功,4失败
		uint32_t dwRealSend = 0;	//发送数据长度
		uint32_t dwRealRead = 0;	//收到的数据长度
		std::vector<unsigned char> dataSend, dataRead;//写读缓冲地区
		int _buflen = 1024;
		//usp_ac ac;
		std::thread td;
		std::string tstr;
		int bit[14] = { 110, 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 43000, 56000, 57600, 115200, 256000 };
	private:
		OVERLAPPED			wrOverlapped = {};					//异步串口用
		uint32_t sdwError = 0, rdwError = 0;
		int isRead = false;
		bool _isAsync = false;						//是否为异步
		bool _endread = false;
		bool isUpdateUI = false;
	};



	port_cx* port_cx::new_port(int size)
	{
		auto p = new port_cx();
		if (size < 16)size = 16;
		if (p)
		{
			p->init(size);
		}
		return p;
	}

	port_cx::port_cx() : _fd(0), isRead(0), _buflen(1024)
		, _sendSta(0), _receSta(0)
	{

	}

	port_cx::~port_cx()
	{
		td.join();
		Close_driver();
	}
	//TCHAR* port_cx::inttostr(int n, int radix)
	//{
	//	static TCHAR buf[BUFLEN];
	//	ZeroMemory(buf, BUFLEN * sizeof(TCHAR));
	//	_itot_s(n, buf, BUFLEN, radix);
	//	return buf;
	//}
	//int port_cx::strtoint(LPCTSTR str)
	//{
	//	return _ttoi(str);
	//}
	//LPCTSTR port_cx::getComboBoxTxt(CComboBox* b, int n)
	//{
	//	static TCHAR buf[1024];
	//	int ind = n;
	//	if (n < 0)
	//	{
	//		ind = b->GetCurSel();
	//	}
	//	CString str, sBaudRate;
	//	ZeroMemory(buf, 1024 * sizeof(TCHAR));
	//	b->GetLBText(ind, buf);
	//	return buf;
	//}
	///-----------------------------------------
	void port_cx::init(int buflen)
	{
		//_cwin = (CSerialPortDlg*)w;

		if (buflen > 0)
		{
			_buflen = buflen;
		}
		dataSend.resize(_buflen);// = ac.new_mem<unsigned char>(_buflen);
		dataRead.resize(_buflen);//= ac.new_mem<unsigned char>(_buflen);
		//initComboBox();
		std::thread t([=]() {
			_endread = false;
			while (true)
			{
				Sleep(1);
				if (_endread)
				{
					break;
				}
				if (!isRead)
				{
					continue;
				}
				updateSend();
				updateRece();

				//if (_receSta == 2)
				//{
				//	std::string data = getData();
				//	data.erase(remove(data.begin(), data.end(), '\r'), data.end());
				//	data.erase(remove(data.begin(), data.end(), '\n'), data.end());
				//	if (!data.empty())
				//	{
				//		while (isUpdateUI)
				//		{
				//			hz::sleep(1);
				//		}
				//		//if (_cwin)
				//		//{
				//		//	_cwin->getStrPF(data.c_str(), _cwin);
				//		//	//_cwin->updateCurve();

				//		//}
				//		//isUpdateUI = true;
				//	}
				//}
				Receive_driver();

			}});
			t.swap(td);
	}

	void port_cx::endRead()
	{
		_endread = true;
	}


	//---------------------------------------------------------------------------------

	//1、打开串口

	void* port_cx::Open_driver(const char* name, uint32_t BaudRate, int idx, bool isSA)
	{
		if (!BaudRate)
		{
			return NULL;
		}
		if (_fd)		//已经打开就关闭
		{
			Close_driver();
		}
		//打开串口
		_isAsync = isSA;
		HANDLE m_hCom = CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0, NULL,

			OPEN_EXISTING, isSA ? FILE_FLAG_OVERLAPPED : 0, NULL);

		if (m_hCom == INVALID_HANDLE_VALUE)
		{

			printf("Create File faile\n");

			return NULL;

		}

		//设置缓冲区大小

		if (!SetupComm(m_hCom, 1024, 1024))
		{

			printf("SetupComm fail!\n");

			CloseHandle(m_hCom);

			return NULL;

		}

		//设置超时

		COMMTIMEOUTS TimeOuts = {};
		TimeOuts.ReadIntervalTimeout = 100;
		TimeOuts.ReadTotalTimeoutConstant = 1000;
		TimeOuts.ReadTotalTimeoutMultiplier = 100;
		TimeOuts.WriteTotalTimeoutConstant = 2000;
		TimeOuts.WriteTotalTimeoutMultiplier = 50;
		SetCommTimeouts(m_hCom, &TimeOuts);
		PurgeComm(m_hCom, PURGE_TXCLEAR | PURGE_RXCLEAR);
		//设置串口参数
		DCB dcb = { 0 };
		if (!GetCommState(m_hCom, &dcb))
		{
			printf("GetCommState fail\n");
			return NULL;
		}
		dcb.DCBlength = sizeof(dcb);
		std::string c = std::to_string(bit[idx]) + ",n,8,1";
		if (!BuildCommDCB(c.c_str(), &dcb))//填充ＤＣＢ的数据传输率、奇偶校验类型、数据位、停止位
		{

			printf("BuileCOmmDCB fail\n");

			CloseHandle(m_hCom);

			return NULL;

		}
		dcb.BaudRate = BaudRate;
		if (SetCommState(m_hCom, &dcb))
		{

			printf("SetCommState OK!\n");

		}

		if (isSA)
		{
			//异步需要建立并初始化重叠结构

			ZeroMemory(&wrOverlapped, sizeof(wrOverlapped));

			if (wrOverlapped.hEvent != NULL)
			{

				ResetEvent(wrOverlapped.hEvent);

				wrOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			}
		}

		_fd = m_hCom;
		return m_hCom;

	}

	//2、  串口发送(写操作)—异步方式

	unsigned char port_cx::Send_driver(unsigned char* data, int len)
	{
		if (!_fd || !data || len < 1)
		{
			return 1;
		}
		sdwError;

		DWORD dwExpectSend = len;

		dwRealSend = 0;

		BYTE* pSendBuffer;

		pSendBuffer = data;

		DWORD de = 0;
		if (ClearCommError(_fd, &de, NULL))
		{
			PurgeComm(_fd, PURGE_TXABORT | PURGE_TXCLEAR);
		}
		sdwError = de;
		_sendSta = 0;
		int ret = 0;
		DWORD rs = 0;
		if (_isAsync)
		{
			if (!WriteFile(_fd, pSendBuffer, dwExpectSend, &rs, &wrOverlapped))
			{
				if (GetLastError() == ERROR_IO_PENDING)
				{
					_sendSta = 1;
				}
			}
			dwRealSend = rs;
		}
		else
		{
			if (!WriteFile(_fd, pSendBuffer, dwExpectSend, &rs, NULL))
			{
				//写串口失败
				printf((char*)"发送失败!\n");
				_sendSta = 4;
				ret = 1;

			}
			else {
				_sendSta = 2;
			}
			dwRealSend = rs;
		}

		return ret;

	}

	//3、  串口接收(读操作)--异步方式

	unsigned char port_cx::Receive_driver()
	{
		if (!_fd)
		{
			return 1;
		}

		memset(dataRead.data(), 0, _buflen);
		DWORD dwError;

		DWORD dwWantRead = _buflen;

		dwRealRead = 0;

		BYTE* pReadBuf;

		int i;

		BYTE crc;

		pReadBuf = dataRead.data();

		if (ClearCommError(_fd, &dwError, NULL))
		{

			PurgeComm(_fd, PURGE_RXABORT | PURGE_RXCLEAR);

		}
		_receSta = 0;
		int ret = 0;
		DWORD rs = 0;
		if (_isAsync)
		{
			if (!ReadFile(_fd, pReadBuf, dwWantRead, &rs, &wrOverlapped))
			{
				auto k = mfile_t::getLastError();
				dwError = GetLastError();

				if (dwError == ERROR_IO_PENDING)
				{
					_receSta = 1;
				}
			}
		}
		else
		{
			if (!ReadFile(_fd, pReadBuf, dwWantRead, &rs, NULL))    //成功返回非0 //失败返回0
			{
				ret = 1;
			}
			if (rs > 0)
			{
				tstr.append((char*)pReadBuf, rs);
				_receSta = 2;
			}
		}
		dwRealRead = rs;
		/*

			if (dwRealRead > 0)
			printf("recv_len = %d\n", dwRealRead);

			printf("接收数据\n");

			for (i = 0; i < 20; i++)
			{

			printf("%x ", data[i]);

			}*/

		return 0;

	}

	int port_cx::send_data(const void* data, int len)
	{
		return Send_driver((unsigned char*)data, len);
	}
	int port_cx::receive()
	{
		isRead = true;
		return 0;
	}
	//4、  关闭串口

	int port_cx::Close_driver()
	{
		CloseHandle(_fd);
		_fd = 0;
		return 0;
	}
	void port_cx::updateSend()
	{
		if (!_fd)
		{
			return;
		}

		DWORD rs = 0, de = 0;
		do
		{
			if (_sendSta != 1)break;
			if (!GetOverlappedResult(_fd, &wrOverlapped, &rs, FALSE))				//成功返回非0，失败返回0
			{

				if (GetLastError() == ERROR_IO_INCOMPLETE)
				{
					//continue;
				}
				else
				{
					printf("发送失败!\n");
					ClearCommError(_fd, &de, NULL);
					_sendSta = 3;
					break;
				}
			}
			else
			{
				_sendSta = 2;
			}
			sdwError = de;
			dwRealSend = rs;
		} while (0);
	}
	void port_cx::updateRece()
	{
		if (!_fd || _receSta != 1 || !isRead)return;

		DWORD rs = 0, de = 0;
		if (GetOverlappedResult(_fd, &wrOverlapped, &rs, TRUE))				//成功返回非0, 失败返回0
		{
			if (rs > 0)
			{
				/*	static string t;
					t += (char*)dataRead;*/
				_receSta = 2;
			}
		}
		else {
			if (GetLastError() == ERROR_IO_INCOMPLETE)
			{
				//没读完
				char* t = (char*)dataRead.data();
				_receSta = 5;
			}
			else
			{
				_receSta = 3;
			}
		}
		dwRealRead = rs;
	}
	void port_cx::setRead(bool is)
	{
		isRead = is;
		if (isRead && _isAsync)
		{
			Receive_driver();
		}
	}
	std::string port_cx::getData()
	{
		auto ks = tstr;
		tstr.clear();
		return ks;
	}
}
//!hz 

#if 0
/*
参数 意义
dwFlags Long，下表中标志之一或它们的组合
dx，dy Long，根据MOUSEEVENTF_ABSOLUTE标志，指定x，y方向的绝对位置或相对位置
cButtons Long，没有使用
dwExtraInfo Long，没有使用

dwFlags常数 意义

const int MOUSEEVENTF_MOVE = 0x0001;      移动鼠标
const int MOUSEEVENTF_LEFTDOWN = 0x0002; 模拟鼠标左键按下
const int MOUSEEVENTF_LEFTUP = 0x0004; 模拟鼠标左键抬起
const int MOUSEEVENTF_RIGHTDOWN = 0x0008; 模拟鼠标右键按下
const int MOUSEEVENTF_RIGHTUP = 0x0010; 模拟鼠标右键抬起
const int MOUSEEVENTF_MIDDLEDOWN = 0x0020; 模拟鼠标中键按下
const int MOUSEEVENTF_MIDDLEUP = 0x0040; 模拟鼠标中键抬起
const int MOUSEEVENTF_ABSOLUTE = 0x8000; 标示是否采用绝对坐标



程序中我们直接调用mouse_event函数就可以了
mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, 500, 500, 0, 0);

1、这里是鼠标左键按下和松开两个事件的组合即一次单击：
mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)



2、模拟鼠标右键单击事件：
mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0)



3、两次连续的鼠标左键单击事件 构成一次鼠标双击事件：
mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)



4、使用绝对坐标
MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, 500, 500, 0, 0

需要说明的是，如果没有使用MOUSEEVENTF_ABSOLUTE，函数默认的是相对于鼠标当前位置的点，如果dx，和dy，用0，0表示，这函数认为是当前鼠标所在的点。



５、直接设定绝对坐标并单击
mouse_event(MOUSEEVENTF_LEFTDOWN, X * 65536 / 1024, Y * 65536 / 768, 0, 0);
mouse_event(MOUSEEVENTF_LEFTUP, X * 65536 / 1024, Y * 65536 / 768, 0, 0);
其中X，Y分别是你要点击的点的横坐标和纵坐标
*/

#endif
#ifdef __has_include
#if (__has_include(<anjn/anjn.h>))
#include <anjn/anjn.h>

namespace hz {
	std::unordered_map<std::string, int>* vkcode_s()
	{
		static std::unordered_map<std::string, int> vk_code = {
			{ "back",0x08}, { "tab" ,0x09}, { "clear" ,0x0C}
			,{ "enter" ,0x0D},{ "shift" ,0x10},{ "ctrl" ,0x11},{ "alt" ,0x12}
			,{ "pause" ,0x13},{ "caps_lock" ,0x14},{ "esc" ,0x1B}
			,{ "space" ,0x20}
			,{ "page_up" ,0x21},{ "page_down" ,0x22},{ "end" ,0x23},{ "home" ,0x24}
			,{ "left_arrow" ,0x25},{ "up_arrow" ,0x26},{ "right_arrow" ,0x27},{ "down_arrow" ,0x28}
			,{ "select" ,0x29},{ "print" ,0x2A},{ "execute" ,0x2B},{ "print_screen" ,0x2C}
			,{ "ins" ,0x2D},{ "del" ,0x2E},{ "help" ,0x2F}
			,{ "0" ,0x30},{ "1" ,0x31},{ "2" ,0x32},{ "3" ,0x33},{ "4" ,0x34},{ "5" ,0x35},{ "6" ,0x36},{ "7" ,0x37},{ "8" ,0x38},{ "9" ,0x39}
			,{ "a" ,0x41},{ "b" ,0x42},{ "c" ,0x43},{ "d" ,0x44},{ "e" ,0x45},{ "f" ,0x46},{ "g" ,0x47},{ "h" ,0x48},{ "i" ,0x49},{ "j" ,0x4A},{ "k" ,0x4B},{ "l" ,0x4C},{ "m" ,0x4D},{ "n" ,0x4E},{ "o" ,0x4F},{ "p" ,0x50},{ "q" ,0x51},{ "r" ,0x52},{ "s" ,0x53},{ "t" ,0x54},{ "u" ,0x55},{ "v" ,0x56},{ "w" ,0x57},{ "x" ,0x58},{ "y" ,0x59},{ "z" ,0x5A}
			,{ "lwin", 0x5b },{ "rwin", 0x5c },{ "apps", 0x5d },{ "sleep", 0x5f }
			,{ "numpad_0" ,0x60},{ "numpad_1" ,0x61},{ "numpad_2" ,0x62},{ "numpad_3" ,0x63},{ "numpad_4" ,0x64},{ "numpad_5" ,0x65},{ "numpad_6" ,0x66},{ "numpad_7" ,0x67},{ "numpad_8" ,0x68},{ "numpad_9" ,0x69}
			,{ "multiply_key" ,0x6A},{ "add_key" ,0x6B},{ "separator_key" ,0x6C},{ "subtract_key" ,0x6D},{ "decimal_key" ,0x6E},{ "divide_key" ,0x6F}
			,{ "F1" ,0x70},{ "F2" ,0x71},{ "F3" ,0x72},{ "F4" ,0x73},{ "F5" ,0x74},{ "F6" ,0x75},{ "F7" ,0x76},{ "F8" ,0x77},{ "F9" ,0x78},{ "F10" ,0x79},{ "F11" ,0x7A},{ "F12" ,0x7B}
			,{ "F13" ,0x7C},{ "F14" ,0x7D},{ "F15" ,0x7E},{ "F16" ,0x7F},{ "F17" ,0x80},{ "F18" ,0x81},{ "F19" ,0x82},{ "F20" ,0x83},{ "F21" ,0x84},{ "F22" ,0x85},{ "F23" ,0x86},{ "F24" ,0x87}
			,{ "num_lock" ,0x90},{ "scroll_lock" ,0x91}
			,{ "left_shift" ,0xA0},{ "right_shift " ,0xA1}
			,{ "left_control" ,0xA2},{ "right_control" ,0xA3}
			,{ "left_menu" ,0xA4},{ "right_menu" ,0xA5}
			,{ "browser_back" ,0xA6},{ "browser_forward" ,0xA7},{ "browser_refresh" ,0xA8},{ "browser_stop" ,0xA9},{ "browser_search" ,0xAA},{ "browser_favorites" ,0xAB},{ "browser_start_and_home" ,0xAC}
			,{ "volume_mute" ,0xAD},{ "volume_Down" ,0xAE},{ "volume_up" ,0xAF}
			,{ "next_track" ,0xB0},{ "previous_track" ,0xB1},{ "stop_media" ,0xB2},{ "play/pause_media" ,0xB3}
			,{ "start_mail" ,0xB4},{ "select_media" ,0xB5},{ "start_application_1" ,0xB6},{ "start_application_2" ,0xB7}
			,{ "attn_key" ,0xF6},{ "crsel_key" ,0xF7},{ "exsel_key" ,0xF8},{ "play_key" ,0xFA},{ "zoom_key" ,0xFB},{ "clear_key" ,0xFE}
			,{ "+" ,0xBB},{ "=" ,0xBB}
			,{ "," ,0xBC},{ "<" ,0xBC}
			,{ "-" ,0xBD},{ "_" ,0xBD}
			,{ "." ,0xBE} ,{ ">" ,0xBE}
			,{ "/" ,0xBF},{ "?" ,0xBF}
			,{ "`" ,0xC0},{ "~" ,0xC0}
			,{ ";" ,0xBA},{ ":" ,0xBA}
			,{ "[" ,0xDB},{ "{" ,0xDB}
			,{ "\\" ,0xDC},{ "|" ,0xDC}
			,{ "]" ,0xDD},{ "}" ,0xDD}
			,{ "'" ,0xDE},{ "\"" ,0xde}
		};
		return &vk_code;
	}

	int anjn::vkcode(const std::string& vks)
	{
		auto vc = vkcode_s();
		auto it = vc->find(vks);
		return it != vc->end() ? it->second : 0;
	}

	// 获取窗口标题
	std::string anjn::get_window_title(HWND hwnd)
	{
		std::string ret;
		int tl = ::GetWindowTextLengthA(hwnd);
		if (tl > 0)
		{
			ret.resize(tl + 1);
			::GetWindowTextA(hwnd, ret.data(), tl + 1);
			ret.pop_back();
		}
		return ret;
	}
	// --------------------------------------------------------------------------------------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------------------------------------------------------------------------------------

	BOOL anjn::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
	{
		//保存显示器信息
		MONITORINFO monitorinfo;
		monitorinfo.cbSize = sizeof(MONITORINFO);
		//获得显示器信息，将信息保存到monitorinfo中
		GetMonitorInfo(hMonitor, &monitorinfo);
		anjn* p = (anjn*)dwData;
		return p ? p->monitor(monitorinfo) : FALSE;
	}

	BOOL anjn::monitor(MONITORINFO& monitorinfo)
	{
		//若检测到主屏
		if (monitorinfo.dwFlags == MONITORINFOF_PRIMARY)
		{
			if (first)  //第一次检测到主屏
			{
				first = FALSE;
				numScreen = 1;

				//将显示器的分辨率信息保存到rect
				rect.insert(rect.begin(), monitorinfo.rcMonitor);
				return TRUE;

			}
			else //第二次检测到主屏,说明所有的监视器都已经检测了一遍，故可以停止检测了
			{
				first = TRUE;    //标志复位
				return FALSE;    //结束检测
			}
		}

		rect.push_back(monitorinfo.rcMonitor);
		numScreen++;
		return TRUE;
	}

	void anjn::get_devs()
	{
		EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)this);
		d_width = rect.rbegin()->right;
		d_height = std::max(rect[0].bottom, rect.rbegin()->bottom);
	}
	void anjn::mouse_move(int x, int y)
	{
		static std::once_flag meflag;
		//int display_x = 0, display_y = 0;
		static double dpx = 0.0, dpy = 0.0;
		std::call_once(meflag, []() {
			DEVMODEW dm;
			ZeroMemory(&dm, sizeof(dm));
			dm.dmSize = sizeof(dm);
			EnumDisplaySettingsW(LR"(\\.\display1)", ENUM_CURRENT_SETTINGS, &dm);
			dpx = 65535.0 / dm.dmPelsWidth;
			dpy = 65535.0 / dm.dmPelsHeight;
			});
		::mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, (1.0 + x) * dpx, (1.0 + y) * dpy, 0, 0);
	}
	void anjn::mouse_down(int z)
	{
		static int mz[] = { MOUSEEVENTF_LEFTDOWN , MOUSEEVENTF_RIGHTDOWN , MOUSEEVENTF_MIDDLEDOWN };
		mouse_event(mz[z], 0, 0, 0, 0);
	}
	void anjn::mouse_up(int z)
	{
		static int mz[] = { MOUSEEVENTF_LEFTUP ,  MOUSEEVENTF_RIGHTUP ,  MOUSEEVENTF_MIDDLEUP };
		mouse_event(mz[z], 0, 0, 0, 0);
	}
	void anjn::mouse_click(int count, int z, int d)
	{
		static int mz[] = { MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP
			, MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP
			, MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_MIDDLEUP
		};
		for (int i = 0; i < count; i++)
		{
			mouse_event(mz[z], 0, 0, 0, 0);
			sleep(d);
		}
	}
	void anjn::mouse_scroll(int y, int x)
	{
		if (x != 0)
			mouse_event(MOUSEEVENTF_HWHEEL, 0, 0, WHEEL_DELTA * x, NULL);
		if (y != 0)
			mouse_event(MOUSEEVENTF_WHEEL, 0, 0, WHEEL_DELTA * y, NULL);
	}

#define keybd_e(k,flags) keybd_event(k,MapVirtualKey(k, MAPVK_VK_TO_VSC),flags,0)
	void anjn::key_cb(const char* k, int dms)
	{
		std::vector<int> v;
#if 1
		auto kv = split_m(k, " ", false);
		for (auto& it : kv)
		{
			v.push_back(vkcode(it));
		}
#else
		auto t = k;
		std::string tem;
		for (; *t; t++)
		{
			char ch = *t;
			if (ch == ' ')
			{
				tem.push_back(ch);
			}
			else
			{
				if (tem == "")
					continue;
				v.push_back(vkcode(tem));
				tem = "";
			}
		}
#endif // 1
		for (auto vc : v)
		{
			keybd_e(vc, 0);
		}
		sleep(dms);
		for (auto vc : v)
		{
			keybd_e(vc, KEYEVENTF_KEYUP);
		}
	}
	void anjn::shift(const char* ks, int z)
	{
		int k = vkcode(ks);
		int s = z > 0 ? VK_RSHIFT : VK_LSHIFT;
		keybd_e(s, 0);
		keybd_e(k, 0);
		keybd_e(k, KEYEVENTF_KEYUP);
		keybd_e(s, KEYEVENTF_KEYUP);
	}

	void anjn::ctrl(const char* ks, int z)
	{
		int k = vkcode(ks);
		int s = z > 0 ? VK_RCONTROL : VK_LCONTROL;
		keybd_e(s, 0);
		keybd_e(k, 0);
		keybd_e(k, KEYEVENTF_KEYUP);
		keybd_e(s, KEYEVENTF_KEYUP);
	}

	// 多分割符
	std::vector<std::string> anjn::split_m(const std::string& str, const std::string& pattern, bool is_space)
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

	void anjn::press_key(const char* ks)
	{
		int k = vkcode(ks);
		keybd_e(k, 0);
	}

	void anjn::release_key(const char* ks)
	{
		int k = vkcode(ks);
		keybd_e(k, KEYEVENTF_KEYUP);
	}

	void anjn::keybd(const char* ks)
	{
		int k = vkcode(ks);
		keybd_e(k, 0);
		keybd_e(k, KEYEVENTF_KEYUP);
	}

	void anjn::get_display_size(int* width, int* height)
	{
		anjn an;
		an.get_devs();
		if (width && height)
		{
			*width = an.d_width;
			*height = an.d_height;
		}
	}

	void anjn::get_mouse_pos(int* x, int* y)
	{
		POINT pt = {};
		GetCursorPos(&pt);
		if (x && y)
		{
			*x = pt.x;
			*y = pt.y;
		}
	}

	glm::vec2 anjn::get_mouse_pos2()
	{
		POINT pt = {};
		GetCursorPos(&pt);
		return glm::vec2(pt.x, pt.y);
	}

	int anjn::sunday_search_s(const std::string& src, const std::string& substr, std::vector<int>* mark)
	{
		return sunday_search(src.c_str(), substr.c_str(), src.size(), substr.size(), mark);
	}

	int anjn::sunday_search(const char* src, const char* substr, int str_len, int sub_len, std::vector<int>* mark)
	{
		unsigned char* str = (unsigned char*)src, * sub = (unsigned char*)substr;
		int* marks;
		int i, j, k;
		int ret = -1;

		if (str_len < 0)
			str_len = strlen((char*)str);
		if (sub_len < 0)
			sub_len = strlen((char*)sub);

		std::vector<int>tm;
		if (!mark)
		{
			mark = &tm;
		}
		if (mark->empty())
		{
			mark->resize(256);
			marks = mark->data();
			for (i = 0; i < 256; i++) {
				marks[i] = -1;
			}
			j = 0;
			for (i = sub_len - 1; i >= 0; i--)
			{
				if (marks[sub[i]] == -1)
				{
					marks[sub[i]] = sub_len - i;
					if (++j == 256) break;
				}
			}
		}
		else
		{
			marks = mark->data();
		}
		i = 0;    j = str_len - sub_len + 1;
		while (i < j)
		{
			for (k = 0; k < sub_len; k++)
			{
				if (str[i + k] != sub[k]) break;
			}
			if (k == sub_len) {
				ret = i;// (str + i);
				break;
			}
			k = marks[str[i + sub_len]];
			//(k == -1) ? i = i + sub_len + 1 : i = i + k;
			i += (k == -1) ? sub_len + 1 : k;
		}
		return ret;
	}


	void anjn::sleep(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
	// todo test实时获取鼠标坐标窗口句柄
	int anjn::mouse_point_test()
	{
		std::thread([]() {
			HWND cuw = 0;
			while (1)
			{
				POINT pNow = { 0,0 };
				// 获取鼠标当前位置
				if (GetCursorPos(&pNow))
				{
					HWND hwndPointNow = WindowFromPoint(pNow);// 获取鼠标所在窗口的句柄

					if (hwndPointNow && hwndPointNow != cuw)
					{
						cuw = hwndPointNow;
						// 获取窗口标题
						//std::cout << std::hex << (int)hwndPointNow << std::endl;					// 鼠标所在窗口的句柄
						//std::cout << get_window_title(hwndPointNow) << std::endl;					// 鼠标所在窗口的标题
						DWORD dwPID = 0, dwTID = 0;
						dwTID = GetWindowThreadProcessId(hwndPointNow, &dwPID);
						//printf("%d\n", dwPID);

					}
					int mx, my;
					get_mouse_pos(&mx, &my);
					std::string t = ("mouse_pos: " + std::to_string(mx) + ", " + std::to_string(my));
					printf("\x1b]0;%s\x07", t.c_str());
					//SetConsoleTitle(t.c_str());
				}
				sleep(100);
			}
			}).detach();
		return 0;
	}
	void ted()
	{
		//::RegisterHotKey();
	}
	int anjn::test_run()
	{
		anjn::get_display_size(0, 0);

		ted();
		// 移动鼠标
		//mouse_move(1, 2);
		//// 单击
		//mouse_click();
		//// 双击
		//mouse_click(2);
		//// 中单击
		//mouse_click(1, 2);
		//// 右单击
		//mouse_click(1, 1);
		//key_cb("e");
		//// 组合键 win+d
		//key_cb("lwin d");
		//key_cb("ctrl a");
		//key_cb("ctrl a b");
		//reg_hot_key("ctrl shift s b");

		mouse_point_test();
		return 0;
	}
	image_nt::image_nt()
	{
	}
	image_nt::~image_nt()
	{
		for (auto it : _imgs)
		{
			if (it)
			{
				delete it;
			}
		}
		_imgs.clear();
		if (_fscimg)
		{
			delete _fscimg;
			_fscimg = 0;
		}
	}

	image_nt* image_nt::new_imgnt()
	{
		return new image_nt();
	}
	void image_nt::new_imgnt(image_nt* p)
	{
		if (p)
			delete p;
	}
	int image_nt::load(const std::string& fn)
	{
		Image* img = Image::create(fn);
		if (img)
		{
			_imgs.push_back(img);
			_fns.push_back(fn);
		}
		return _imgs.size();
	}
	int image_nt::load_zip(const std::string& fn, const std::string& zpass)
	{
		int ret = -1;
		Zip z;
		if (z.open_file(fn, true, zpass) == 0)
		{
			auto n = z.get_maps();
			std::vector<char> od;
			for (auto& it : n)
			{
				std::string ifn = toStr(it["name"]);
				if (z.readfile(ifn, od))
				{
					Image* img = Image::create_mem(od.data(), od.size());
					if (img)
					{
						_imgs.push_back(img);
						_fns.push_back(ifn);
					}
				}
			}
			ret = _imgs.size();
		}
		return ret;
	}
	Image* image_nt::get_rect_image(Image* src, glm::ivec4 src_rect, Image* dst)
	{
		Image* tsi = src;
		if (src_rect.z > 0 && src_rect.w > 0 && src_rect.x < tsi->width && src_rect.y < tsi->height)
		{
			src_rect.x = std::max(0, src_rect.x);
			src_rect.y = std::max(0, src_rect.y);
			src_rect.z = std::min(src_rect.z, (int)tsi->width - src_rect.x);
			src_rect.w = std::min(src_rect.w, (int)tsi->height - src_rect.y);
			tsi = src->getBox(src_rect, dst);
		}
		return tsi;
	}
	std::vector<glm::ivec2> image_nt::find(int idx, size_t cidx, glm::ivec4 rect, glm::ivec4 src_rect)
	{
		std::vector<glm::ivec2> ret;
		std::vector<int> mark;
		if (_imgs.size() >= idx || idx < 0)
		{
			return ret;
		}
		auto img = _imgs[idx];
		Image* tsi = get_rect_image(_fscimg, src_rect, &_tsimg);
		Image* tbi = get_rect_image(img, rect, &_tbimg);

		auto src = tsi->data();
		auto src_len = tsi->datasize();
		auto sub = tbi->data();
		auto sw = tsi->width;
		auto swb = sw * sizeof(uint32_t);
		auto fw = tbi->width;
		auto fwb = fw * sizeof(uint32_t);
		auto dey = tsi->height - tbi->height + 1;
		auto fsl = src_len;
		auto fsrc = src;
		size_t cy = 0;
		if (cidx > tbi->height)
		{
			std::vector<yHist> fyh;
			fyh.resize(tbi->height);
			unsigned char* c = (unsigned char*)sub;
			int cx = 0;
			for (auto& it : fyh)
			{
				it.init();
				for (size_t i = 0; i < fw; i++)
				{
					//it.Hist(*c);
					it.mset.insert(*c);
					c++;
				}
				if (cx < it.mset.size())
				{
					cx = it.mset.size();
					cidx = cy;
				}
				cy++;
			}
		}
		auto fsub = sub + cidx * fw;
		size_t i = 0, x, tx = 0, ty = 0;
		for (; i < dey;)
		{
			auto ps = anjn::sunday_search((char*)fsrc, (char*)fsub, fsl, fwb, &mark);
			if (ps < 0)
			{
				break;
			}
			int64_t ops = ps;
			ps /= 4;
			fsrc += ps;
			auto ly = ps / sw;
			i += ly;
			ps = ps % sw;
			fsl -= ops + 4;
			tx += ps;
			auto sub1 = sub;
			int k = -1;
			auto src1 = fsrc;
			src1 -= cidx * sw;
			auto swb1 = swb - ps;
			for (x = 0; x < tbi->height; x++, src1 += sw, sub1 += fw)
			{
				if (x == cidx)
				{
					continue;
				}
				k = memcmp(src1, sub1, fwb);
				if (k != 0)
				{
					break;
				}
			}
			if (k == 0)
			{
				ret.push_back({ tx, i - cidx });
				//break;
			}
			fsrc++;
			tx++;
			if (tx > sw)
			{
				tx -= sw;
				i++;
			}
		}
		if (tbi != img)
		{
			//Image::destroy(tbi);
		}
		if (tsi != _fscimg)
		{
			//Image::destroy(tsi);
		}
		return ret;
	}
	void image_nt::set_src_img(Image* img, bool free_old)
	{
		if (img)
		{
			if (free_old && _fscimg && _fscimg != img)
			{
				Image::destroy(_fscimg);
			}
			_fscimg = img;
		}
	}
	// 全屏截图
	void image_nt::get_fullscreen(const std::string& fn)
	{
		// 窗口 桌面句柄
		auto hwnd = GetDesktopWindow();
		// 矩形_桌面
		RECT grc = {};
		GetWindowRect(hwnd, &grc);
		auto gdc = GetDC(hwnd);
		//位图 句柄
		auto bitgb = CreateCompatibleBitmap(gdc, grc.right, grc.bottom);
		auto ij_DC = CreateCompatibleDC(gdc); // 创建兼容的内存DC
		SelectObject(ij_DC, bitgb);
		BitBlt(ij_DC, 0, 0, grc.right, grc.bottom, gdc, 0, 0, SRCCOPY);
		// 位 图
		BITMAP  bm = { 0 };
		GetObject(bitgb, sizeof(BITMAP), &bm);
		// 文件头
		BITMAPFILEHEADER bm_fnt = { 0 };
		// 信息头
		char bmit[256] = {};
		BITMAPINFO* bmpInfo = (BITMAPINFO*)bmit;

		DWORD ds = bm.bmWidth * bm.bmHeight * 4; // 16位 = 8字节
		// 数据指针
		std::vector<char> d;
		d.resize(ds);
		char* data = d.data();
		int ct = 1;
		//填充文件 头
		bm_fnt.bfType = 0x4D42; //'BM' 必须是 没有为什么
		bm_fnt.bfSize = ds + 54;
		bm_fnt.bfReserved1 = 0; //保留 没有为什么
		bm_fnt.bfReserved2 = 0; //同上
		bm_fnt.bfOffBits = 54;
		//填充信息头
		auto& bt = bmpInfo->bmiHeader;
		bt.biSize = sizeof(BITMAPINFO) + 8; //本结构大小 40直接sizeof
		bt.biHeight = -bm.bmHeight;
		bt.biWidth = bm.bmWidth;
		bt.biPlanes = 1; //必须是1 没有理由
		bt.biBitCount = 32; //32真彩高清
		bt.biCompression = BI_RGB;
		bt.biSizeImage = ds; // = bm数据大小
		bt.biXPelsPerMeter = 0;
		bt.biYPelsPerMeter = 0;
		bt.biClrUsed = 0;
		bt.biClrImportant = 0;

		bmpInfo->bmiHeader.biCompression = BI_BITFIELDS;
		int iss = sizeof(bmpInfo);
		LPRGBQUAD bmic = bmpInfo->bmiColors;
		if (ct == 0)
		{
			//RGB
			*(UINT*)(bmic + 2) = 0xFF0000;	// red分量
			*(UINT*)(bmic + 1) = 0x00FF00;	// green分量
			*(UINT*)(bmic + 0) = 0x0000FF;	// blue分量
		}
		else
		{
			//BGR
			*(UINT*)(bmic + 0) = 0xFF0000;	// red分量
			*(UINT*)(bmic + 1) = 0x00FF00;	// green分量
			*(UINT*)(bmic + 2) = 0x0000FF;	// blue分量
		}
		GetDIBits(gdc, bitgb, 0, bm.bmHeight, data, (LPBITMAPINFO)bmpInfo, DIB_RGB_COLORS);
		if (!_fscimg)
		{
			_fscimg = Image::create_null(bm.bmWidth, bm.bmHeight);
		}
		else
		{
			_fscimg->resize(bm.bmWidth, bm.bmHeight);
		}
		auto imgd = _fscimg->data();
		if (imgd)
		{
			memcpy(imgd, data, ds);
			_fscimg->bgra2rgba();
			if (fn.size())
				_fscimg->saveImage(fn);
		}
		DeleteObject(bitgb);
		DeleteDC(ij_DC);
		ReleaseDC(hwnd, gdc);
	}


	anjn_lua::anjn_lua(lua_cx* p) :ctx(p)
	{
	}

	anjn_lua::~anjn_lua()
	{
	}
	void anjn_lua::makedata()
	{
		auto vc = vkcode_s();
		ctx->add_table(*vc);
		ctx->set_field("vk_code");
	}
	void anjn_lua::init()
	{
		ctx->insert_pn("anj");
		/*
		*	把函数注册到anj表
			anj.sleep(100);
			anj.mouse_move(100,200);
			local mx,my = anj.get_mouse_pos();
		*/
		ctx->begin_pn("anj", 0);
		makedata();
		ctx->reg_cb1("sleep", anjn::sleep);
		ctx->reg_cb1("mouse_move", anjn::mouse_move);
		ctx->reg_cb1("mouse_down", anjn::mouse_down);
		ctx->reg_cb1("mouse_up", anjn::mouse_up);
		ctx->reg_cb1("mouse_click", anjn::mouse_click);
		ctx->reg_cb1("mouse_scroll", anjn::mouse_scroll);
		ctx->reg_cb1("keybd", anjn::keybd);
		ctx->reg_cb1("press_key", anjn::press_key);
		ctx->reg_cb1("release_key", anjn::release_key);
		ctx->reg_cb1("shift", anjn::shift);
		ctx->reg_cb1("ctrl", anjn::ctrl);
		ctx->reg_cb1("key_cb", anjn::key_cb);
		ctx->reg_cb1("get_mouse_pos", anjn::get_mouse_pos2);
		ctx->end_pn();
	}

}
//!hz

#endif
#endif




#define SHCNRF_InterruptLevel  0x0001 //Interrupt level notifications from the file system
#define SHCNRF_ShellLevel   0x0002 //Shell-level notifications from the shell
#define SHCNRF_RecursiveInterrupt  0x1000 //Interrupt events on the whole subtree
#define SHCNRF_NewDelivery   0x8000 //Messages received use shared memory
#define WM_USERDEF_FILECHANGED (WM_USER + 105)


#define OFFSETOFCLASS0(base, derived) \
    ((unsigned long)(DWORD_PTR)(STATIC_CAST(base*)((derived*)8))-8)
#define QITABENTMULTI0(Cthis, Ifoo, Iimpl) \
    { (IID*) &IID_##Ifoo, OFFSETOFCLASS0(Iimpl, Cthis) }

#define QITABENT0(Cthis, Ifoo) QITABENTMULTI0(Cthis, Ifoo, Ifoo)


namespace hz
{
	/*

	typedef struct tSHChangeNotifyEntry
	{
	LPCITEMIDLIST pidl; //Pointer to an item identifier list (PIDL) for which to receive notifications
	BOOL fRecursive; //Flag indicating whether to post notifications for children of this PIDL
	}SHChangeNotifyEntry;
	*/

	typedef struct tSHNotifyInfo
	{
		DWORD dwItem1;  // dwItem1 contains the previous PIDL or name of the folder. 
		DWORD dwItem2;  // dwItem2 contains the new PIDL or name of the folder. 
	}SHNotifyInfo;

	typedef ULONG
	(WINAPI* pfnSHChangeNotifyRegister)
		(
			HWND hWnd,
			int  fSource,
			LONG fEvents,
			UINT wMsg,
			int  cEntries,
			SHChangeNotifyEntry* pfsne
			);
	typedef BOOL(WINAPI* pfnSHChangeNotifyDeregister)(ULONG ulID);

#ifdef MAKEINTRESOURCE
#undef MAKEINTRESOURCE
#define MAKEINTRESOURCE  MAKEINTRESOURCEA
#endif // !UNICODE

	class DropFile
	{
	public:
		DropFile() {}
		~DropFile() {}
		BOOL Init(HWND hwnd)
		{
			//加载Shell32.dll
			m_hShell32 = LoadLibraryA("Shell32.dll");
			if (m_hShell32 == NULL)
			{
				return FALSE;
			}

			//取函数地址

			m_pfnRegister = (pfnSHChangeNotifyRegister)GetProcAddress(m_hShell32, MAKEINTRESOURCE(2));
			m_pfnDeregister = (pfnSHChangeNotifyDeregister)GetProcAddress(m_hShell32, MAKEINTRESOURCE(4));
			if (m_pfnRegister == NULL || m_pfnDeregister == NULL)
			{
				return FALSE;
			}

			SHChangeNotifyEntry shEntry = { 0 };
			shEntry.fRecursive = TRUE;
			shEntry.pidl = 0;


			//注册Shell监视函数
			m_ulNotifyId = m_pfnRegister(
				hwnd,
				SHCNRF_InterruptLevel | SHCNRF_ShellLevel,
				SHCNE_ALLEVENTS,
				WM_USERDEF_FILECHANGED, //自定义消息
				1,
				&shEntry
			);
			if (m_ulNotifyId == 0)
			{
				MessageBoxA(0, "Register failed!", "ERROR", MB_OK | MB_ICONERROR);
				return FALSE;
			}
			return TRUE;
		}
		// 在CListCtrlEx类中再添加如下函数。该函数的作用是从PIDL中解出实际字符路径。
		std::string GetPathFromPIDL(size_t pidl)
		{
			char szPath[MAX_PATH];
			std::string strTemp = ("");
			if (SHGetPathFromIDListA((struct _ITEMIDLIST*)pidl, szPath))
			{
				strTemp = szPath;
			}
			return strTemp;
		}
		LRESULT OnFileChanged(WPARAM wParam, LPARAM lParam)
		{
			std::string  strOriginal = ("");
			std::string  strCurrent = ("");
			SHNotifyInfo* pShellInfo = (SHNotifyInfo*)wParam;

			strOriginal = GetPathFromPIDL(pShellInfo->dwItem1);
			if (strOriginal.empty())
			{
				return NULL;
			}

			switch (lParam)
			{
			case SHCNE_CREATE:
				break;

			case SHCNE_DELETE:
				break;
			case SHCNE_RENAMEITEM:
				break;
			}
			return NULL;
		}

	private:

		HMODULE  m_hShell32 = 0;
		pfnSHChangeNotifyRegister      m_pfnRegister = NULL;
		pfnSHChangeNotifyDeregister    m_pfnDeregister = NULL;

		BOOL m_ulNotifyId = 0;
	};


	//-----------------------------------------------------------------------------
	class MouseCursor
	{
	public:
		MouseCursor() {}
		~MouseCursor() {}

		void OnHoverCursor(HWND hWnd, LPCTSTR idc)
		{
			_hWnd = hWnd;

#ifdef _WIN64
			clsCur = (HCURSOR)GetClassLongPtrW(hWnd, GCLP_HCURSOR); //取当前的光标值
			SetClassLongPtrW(hWnd, GCLP_HCURSOR, NULL);   //关闭窗口类对光标的控制
#else
			SetLastError(0);
			clsCur = (HCURSOR)GetClassLongW(hWnd, GCL_HCURSOR); //取当前的光标值
			DWORD err = GetLastError();
			SetClassLongW(hWnd, GCL_HCURSOR, NULL);   //关闭窗口类对光标的控制
#endif
#ifdef CUR_NO
			clsCur = SetCursor(hl::HLCaret::getCursor(CUR_NO)); //设置光标为输入指针
#endif
		}

		void OnLeaveCursor()
		{
			// if (clsCur)
			{

#ifdef _WIN64
				SetClassLongPtrW(_hWnd, GCLP_HCURSOR, (LONG_PTR)clsCur);
#else
				SetClassLongW(_hWnd, GCL_HCURSOR, (LONG)clsCur);  //恢复窗口类对光标的控制
#endif
				SetCursor(clsCur); //恢复箭头光标
			}
		}
	private:
		HWND _hWnd = NULL;
		HCURSOR clsCur;
	};

	//-----------------------------------------------------------------------------
	class IDuiDropTarget
	{
	public:
		virtual	HRESULT OnDragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL ptl, DWORD* pdwEffect) = 0;
		virtual HRESULT  OnDragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) = 0;
		virtual HRESULT  OnDragLeave() = 0;
		virtual HRESULT  OnDrop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) = 0;
	};

	// 接收拖动OLE目标
	class CDropTargetEx : public IDropTarget
	{
	public:
		CDropTargetEx(void)
		{
			m_lRefCount = 1;
			// Create an instance of the shell DnD helper object.
			if (SUCCEEDED(CoCreateInstance(CLSID_DragDropHelper, NULL,
				CLSCTX_INPROC_SERVER,
				IID_IDropTargetHelper,
				(void**)&m_piDropHelper)))
			{
				//   m_bUseDnDHelper = true;
			}
		}
		~CDropTargetEx(void)
		{
			if (m_piDropHelper)
			{
				m_piDropHelper->Release();
			}
			m_bUseDnDHelper = false;
			m_lRefCount = 0;
			DragDropRevoke(m_hWnd);
		}
		bool DragDropRegister(HWND hWnd, DWORD AcceptKeyState = MK_LBUTTON)
		{
			if (!IsWindow(hWnd))return false;
			DragAcceptFiles(hWnd, FALSE);
			HRESULT s = ::RegisterDragDrop(hWnd, this);
			m_hWnd = hWnd;
			if (SUCCEEDED(s))
			{
				m_dAcceptKeyState = AcceptKeyState;
				return true;
			}
			else { return false; }
		}
		void setOver(std::function<void(int x, int y, int fmt)> overfunc)
		{
			_over_func = overfunc;
		}
		void SetDragDrop(IDuiDropTarget* pDuiDropTarget)
		{
			if (_isDrop)
			{
				m_pDuiDropTarget = pDuiDropTarget;
			}
			else
			{
				m_pDuiDropTarget = 0;
			}
		}
		bool DragDropRevoke(HWND hWnd)
		{
			if (!hWnd)
			{
				hWnd = m_hWnd;
			}
			if (!IsWindow(hWnd))return false;

			HRESULT s = ::RevokeDragDrop(hWnd);
			m_pDuiDropTarget = NULL;
			return SUCCEEDED(s);
		}
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, __RPC__deref_out void** ppvObject)
		{
			static const QITAB qit[] =
			{
				QITABENT0(CDropTargetEx, IDropTarget),
				{ 0 }
			};

			return QISearch(this, qit, riid, ppvObject);
		}
		ULONG STDMETHODCALLTYPE AddRef()
		{
			return InterlockedIncrement(&m_lRefCount);
		}
		ULONG STDMETHODCALLTYPE Release()
		{
			ULONG lRef = InterlockedDecrement(&m_lRefCount);
			ULONG ret = m_lRefCount;
			if (0 == lRef)
			{
				delete this;
			}
			return ret;
		}

		int GetDropData_fmt(__RPC__in_opt IDataObject* pDataObj)
		{
			IEnumFORMATETC* pEnumFmt = NULL;
			HRESULT ret = pDataObj->EnumFormatEtc(DATADIR_GET, &pEnumFmt);
			if (SUCCEEDED(ret))
			{
				pEnumFmt->Reset();
				HRESULT Ret = S_OK;
				FORMATETC cFmt = { };

				STGMEDIUM stgMedium;
				//
				ULONG Fetched = 0;
				size_t inc = 0;
				do {
					Ret = pEnumFmt->Next(1, &cFmt, &Fetched);
					if (SUCCEEDED(ret))
					{
						if (cFmt.cfFormat == CF_HDROP ||
							cFmt.cfFormat == CF_TEXT || cFmt.cfFormat == CF_OEMTEXT ||
							cFmt.cfFormat == CF_UNICODETEXT
							)
						{
							return cFmt.cfFormat == CF_HDROP ? 1 : 0;
						}
					}
				} while (Ret == S_OK);
			}
			return -1;
		}
		//进入
		HRESULT STDMETHODCALLTYPE DragEnter(__RPC__in_opt IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, __RPC__inout DWORD* pdwEffect)
		{
			_isDrop = true;
			//_cur.OnHoverCursor(m_hWnd, IDC_NO);
			fmt = GetDropData_fmt(pDataObj);
			if (m_bUseDnDHelper)
			{
				m_piDropHelper->DragEnter(m_hWnd, pDataObj, (LPPOINT)&pt, *pdwEffect);
			}
			//printf("DragEnter\n");
			if (m_pDuiDropTarget)
				return m_pDuiDropTarget->OnDragEnter(pDataObj, grfKeyState, pt, pdwEffect);

			return S_OK;
		}

		//移动
		HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, __RPC__inout DWORD* pdwEffect)
		{
			//*pdwEffect = DROPEFFECT_NONE;//表示不接受拖放好像没用
			//*pdwEffect = DROPEFFECT_COPY;//复制
			//_cur.OnHoverCursor(m_hWnd, IDC_NO);
			if (_over_func)
			{
				_over_func(pt.x, pt.y, fmt);
			}
			if (m_bUseDnDHelper)
			{
				m_piDropHelper->DragOver((LPPOINT)&pt, *pdwEffect);
			}
			*pdwEffect = DROPEFFECT_NONE;

			//printf("DragOver%p,%p\n", m_pDuiDropTarget, m_bUseDnDHelper);
			if (m_pDuiDropTarget)
				return m_pDuiDropTarget->OnDragOver(grfKeyState, pt, pdwEffect);
			return S_OK;
		}

		//离开
		HRESULT STDMETHODCALLTYPE DragLeave()
		{
			_isDrop = false;
			if (m_bUseDnDHelper)
			{
				m_piDropHelper->DragLeave();
			}
			HRESULT hr = S_OK;
			if (m_pDuiDropTarget)
			{
				hr = m_pDuiDropTarget->OnDragLeave();
				m_pDuiDropTarget = nullptr;
			}
			//printf("DragLeave\n");
			return hr;
		}

		//释放
		HRESULT STDMETHODCALLTYPE Drop(__RPC__in_opt IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, __RPC__inout DWORD* pdwEffect)
		{
			m_piDropHelper->Drop(pDataObj, (LPPOINT)&pt, *pdwEffect);
			HRESULT hr = S_OK;
			if (m_pDuiDropTarget)
				hr = m_pDuiDropTarget->OnDrop(pDataObj, grfKeyState, pt, pdwEffect);
			m_pDuiDropTarget = nullptr;
			_isDrop = false;
			//printf("-------------Drop\n");
			return hr;
		}

		//void GetDropData(__RPC__in_opt IDataObject *pDataObj);
		//BOOL ProcessDrop(HDROP hDrop);
	private:
		HWND m_hWnd = nullptr;
		IDropTargetHelper* m_piDropHelper = 0;
		bool	m_bUseDnDHelper = false;
		IDuiDropTarget* m_pDuiDropTarget = 0;
		DWORD m_dAcceptKeyState = 0;
		ULONG  m_lRefCount = 0;
		bool _isDrop = false;
		MouseCursor _cur;
		int fmt = -1;
		std::function<void(int x, int y, int fmt)> _over_func;
	};

	typedef struct _DRAGDATA
	{

		int cfFormat;
		STGMEDIUM stgMedium;
	}DRAGDATA, * LPDRAGDATA;

	//---------drag-----------------------------------------------------------------------------------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------
	class SdkDropSource : public IDropSource
	{
	public:

		SdkDropSource()
		{
			m_lRefCount = 1;
		}
		~SdkDropSource()
		{
			m_lRefCount = 0;
		}

		// Methods of IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
		{

			static const QITAB qit[] =
			{
				QITABENT0(SdkDropSource, IDropSource),
				{  0 }
			};

			return QISearch(this, qit, riid, ppv);
		}
		IFACEMETHODIMP_(ULONG) AddRef(void)
		{
			return InterlockedIncrement(&m_lRefCount);
		}

		IFACEMETHODIMP_(ULONG) Release(void)
		{
			ULONG lRef = InterlockedDecrement(&m_lRefCount);
			ULONG ret = m_lRefCount;
			if (0 == lRef)
			{
				delete this;
			}
			return ret;
		}

		// Methods of IDropSource 右键取消
		IFACEMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
		{
			if (TRUE == fEscapePressed || (grfKeyState & (MK_RBUTTON)))
			{
				return DRAGDROP_S_CANCEL;
			}

			// If the left button of mouse is released
			if (0 == (grfKeyState & (MK_LBUTTON)))
			{
				return DRAGDROP_S_DROP;
			}

			return S_OK;
		}
		IFACEMETHODIMP GiveFeedback(DWORD dwEffect)
		{
			UNREFERENCED_PARAMETER(dwEffect);
			return DRAGDROP_S_USEDEFAULTCURSORS;
		}

	private:

		volatile LONG   m_lRefCount;        //!< The reference count
	};


	//-----------------------------------------------------------------------------

	typedef struct _DATASTORAGE
	{
		FORMATETC* m_formatEtc;
		STGMEDIUM* m_stgMedium;

	} DATASTORAGE_t, * LPDATASTORAGE_t;

	class SdkDataObject : public IDataObject
	{
	public:

		SdkDataObject(SdkDropSource* pDropSource = NULL)
		{
			m_pDropSource = pDropSource;
			m_lRefCount = 1;
		}
		BOOL IsDataAvailable(CLIPFORMAT cfFormat);
		BOOL GetGlobalData(CLIPFORMAT cfFormat, void** ppData);
		BOOL GetGlobalDataArray(CLIPFORMAT cfFormat,
			HGLOBAL* pDataArray, DWORD dwCount);
		BOOL SetGlobalData(CLIPFORMAT cfFormat, void* pData, BOOL fRelease = TRUE);
		BOOL SetGlobalDataArray(CLIPFORMAT cfFormat,
			HGLOBAL* pDataArray, DWORD dwCount, BOOL fRelease = TRUE);
		BOOL SetDropTip(DROPIMAGETYPE type, PCWSTR pszMsg, PCWSTR pszInsert);

		// The com interface.
		IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
		{
			static const QITAB qit[] =
			{
				QITABENT0(SdkDataObject, IDataObject),
				{  0 },
			};
			return QISearch(this, qit, riid, ppv);
		}
		IFACEMETHODIMP_(ULONG) AddRef()
		{
			return InterlockedIncrement(&m_lRefCount);
		}
		IFACEMETHODIMP_(ULONG) Release()
		{
			ULONG lRef = InterlockedDecrement(&m_lRefCount);
			ULONG ret = m_lRefCount;
			if (0 == lRef)
			{
				delete this;
			}
			return ret;
		}
		IFACEMETHODIMP GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium)
		{
			if ((NULL == pformatetcIn) || (NULL == pmedium))
			{
				return E_INVALIDARG;
			}

			pmedium->hGlobal = NULL;

			int nSize = (int)m_dataStorageCL.size();
			for (int i = 0; i < nSize; ++i)
			{
				DATASTORAGE_t dataEntry = m_dataStorageCL.at(i);
				if ((pformatetcIn->tymed & dataEntry.m_formatEtc->tymed) &&
					(pformatetcIn->dwAspect == dataEntry.m_formatEtc->dwAspect) &&
					(pformatetcIn->cfFormat == dataEntry.m_formatEtc->cfFormat))
				{
					return CopyMedium(pmedium,
						dataEntry.m_stgMedium, dataEntry.m_formatEtc);
				}
			}

			return DV_E_FORMATETC;
		}
		IFACEMETHODIMP SetData(FORMATETC* pformatetc,
			STGMEDIUM* pmedium, BOOL fRelease)
		{
			if ((NULL == pformatetc) || (NULL == pmedium))
			{
				return E_INVALIDARG;
			}

			if (pformatetc->tymed != pmedium->tymed)
			{
				return E_FAIL;
			}

			FORMATETC* fetc = new FORMATETC;
			STGMEDIUM* pStgMed = new STGMEDIUM;
			ZeroMemory(fetc, sizeof(FORMATETC));
			ZeroMemory(pStgMed, sizeof(STGMEDIUM));

			*fetc = *pformatetc;

			if (TRUE == fRelease)
			{
				*pStgMed = *pmedium;
			}
			else
			{
				CopyMedium(pStgMed, pmedium, pformatetc);
			}

			DATASTORAGE_t dataEntry = { fetc, pStgMed };
			m_dataStorageCL.push_back(dataEntry);

			return S_OK;
		}
		IFACEMETHODIMP GetDataHere(FORMATETC* pformatetc, STGMEDIUM* pmedium)
		{
			UNREFERENCED_PARAMETER(pformatetc);
			UNREFERENCED_PARAMETER(pmedium);
			return E_NOTIMPL;
		}
		IFACEMETHODIMP QueryGetData(FORMATETC* pformatetc)
		{
			if (NULL == pformatetc)
			{
				return E_INVALIDARG;
			}
			if (!(DVASPECT_CONTENT & pformatetc->dwAspect))
			{
				return DV_E_DVASPECT;
			}
			HRESULT hr = DV_E_TYMED;
			size_t nSize = m_dataStorageCL.size();
			for (size_t i = 0; i < nSize; ++i)
			{
				DATASTORAGE_t dataEnrty = m_dataStorageCL.at(i);
				if (dataEnrty.m_formatEtc->tymed & pformatetc->tymed)
				{
					if (dataEnrty.m_formatEtc->cfFormat == pformatetc->cfFormat)
					{
						return S_OK;
					}
					else
					{
						hr = DV_E_CLIPFORMAT;
					}
				}
				else
				{
					hr = DV_E_TYMED;
				}
			}
			return hr;
		}
		IFACEMETHODIMP GetCanonicalFormatEtc(FORMATETC* pformatetcIn,
			FORMATETC* pformatetcOut)
		{
			pformatetcOut->ptd = NULL;

			return E_NOTIMPL;
		}
		IFACEMETHODIMP EnumFormatEtc(DWORD dwDirection,
			IEnumFORMATETC** ppenumFormatEtc)
		{
			if (NULL == ppenumFormatEtc)
			{
				return E_INVALIDARG;
			}
			*ppenumFormatEtc = NULL;
			HRESULT hr = E_NOTIMPL;
			if (DATADIR_GET == dwDirection)
			{
				FORMATETC rgfmtetc[] =
				{
					{ _cfFormat, NULL, DVASPECT_CONTENT, 0, TYMED_HGLOBAL },
				};
				hr = SHCreateStdEnumFmtEtc(ARRAYSIZE(rgfmtetc), rgfmtetc, ppenumFormatEtc);
			}
			return hr;
		}
		IFACEMETHODIMP DAdvise(FORMATETC* pformatetc, DWORD advf,
			IAdviseSink* pAdvSnk, DWORD* pdwConnection)
		{
			UNREFERENCED_PARAMETER(pformatetc);
			UNREFERENCED_PARAMETER(advf);
			UNREFERENCED_PARAMETER(pAdvSnk);
			UNREFERENCED_PARAMETER(pdwConnection);
			return E_NOTIMPL;
		}
		IFACEMETHODIMP DUnadvise(DWORD dwConnection)
		{
			UNREFERENCED_PARAMETER(dwConnection);
			return E_NOTIMPL;
		}
		IFACEMETHODIMP EnumDAdvise(IEnumSTATDATA** ppenumAdvise)
		{
			UNREFERENCED_PARAMETER(ppenumAdvise);
			return E_NOTIMPL;
		}

		HRESULT SetBlob(CLIPFORMAT cf, const void* pvBlob, UINT cbBlob)
		{
			void* pv = GlobalAlloc(GPTR, cbBlob);
			*(char*)pv = 0;
			HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
			if (SUCCEEDED(hr))
			{
				CopyMemory(pv, pvBlob, cbBlob);
				FORMATETC fmte = { cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
				_cfFormat = cf;
				// The STGMEDIUM structure is used to define how to handle a global memory transfer.
				// This structure includes a flag, tymed, which indicates the medium
				// to be used, and a union comprising pointers and a handle for getting whichever
				// medium is specified in tymed.
				STGMEDIUM medium = {};
				medium.tymed = TYMED_HGLOBAL;
				medium.hGlobal = pv;
				hr = this->SetData(&fmte, &medium, TRUE);
				if (FAILED(hr))
				{
					GlobalFree(pv);
				}
			}
			return hr;
		}
	private:

		~SdkDataObject(void)
		{
			m_lRefCount = 0;

			int nSize = (int)m_dataStorageCL.size();
			for (int i = 0; i < nSize; ++i)
			{
				DATASTORAGE_t dataEntry = m_dataStorageCL.at(i);
				ReleaseStgMedium(dataEntry.m_stgMedium);
				delete[](dataEntry.m_stgMedium);
				delete[](dataEntry.m_formatEtc);
			}
		}
		SdkDataObject(const SdkDataObject&);
		SdkDataObject& operator = (const SdkDataObject&);
		HRESULT CopyMedium(STGMEDIUM* pMedDest,
			STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc)
		{
			if ((NULL == pMedDest) || (NULL == pMedSrc) || (NULL == pFmtSrc))
			{
				return E_INVALIDARG;
			}
			switch (pMedSrc->tymed)
			{
			case TYMED_HGLOBAL:
				pMedDest->hGlobal = (HGLOBAL)OleDuplicateData(pMedSrc->hGlobal, pFmtSrc->cfFormat, NULL);
				break;
			case TYMED_GDI:
				pMedDest->hBitmap = (HBITMAP)OleDuplicateData(pMedSrc->hBitmap, pFmtSrc->cfFormat, NULL);
				break;
			case TYMED_MFPICT:
				pMedDest->hMetaFilePict = (HMETAFILEPICT)OleDuplicateData(pMedSrc->hMetaFilePict, pFmtSrc->cfFormat, NULL);
				break;
			case TYMED_ENHMF:
				pMedDest->hEnhMetaFile = (HENHMETAFILE)OleDuplicateData(pMedSrc->hEnhMetaFile, pFmtSrc->cfFormat, NULL);
				break;
			case TYMED_FILE:
				pMedSrc->lpszFileName = (LPOLESTR)OleDuplicateData(pMedSrc->lpszFileName, pFmtSrc->cfFormat, NULL);
				break;
			case TYMED_ISTREAM:
				pMedDest->pstm = pMedSrc->pstm;
				pMedSrc->pstm->AddRef();
				break;
			case TYMED_ISTORAGE:
				pMedDest->pstg = pMedSrc->pstg;
				pMedSrc->pstg->AddRef();
				break;
			case TYMED_NULL:
			default:
				break;
			}
			pMedDest->tymed = pMedSrc->tymed;
			pMedDest->pUnkForRelease = NULL;
			if (pMedSrc->pUnkForRelease != NULL)
			{
				pMedDest->pUnkForRelease = pMedSrc->pUnkForRelease;
				pMedSrc->pUnkForRelease->AddRef();
			}
			return S_OK;
		}

	private:

		//!< The reference of count
		volatile LONG           m_lRefCount;
		//!< The pointer to CDropSource object  
		SdkDropSource* m_pDropSource;
		//!< The collection of DATASTORAGE_t structure    
		std::vector<DATASTORAGE_t>   m_dataStorageCL;

		CLIPFORMAT _cfFormat = CF_UNICODETEXT;//CF_HDROP;
	};


	//----------接收拖动OLE-------------------------------------------------------------------
#if 1
	class GuiDropTarget :public IDuiDropTarget
	{
	public:
		drop_info_cx* _p = 0;
	public:
		static GuiDropTarget* create()
		{
			return new GuiDropTarget();
		}

	public:
		GuiDropTarget() {}
		~GuiDropTarget() {}

		HRESULT OnDragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL ptl, DWORD* pdwEffect)
		{
			_p->_drop_state = true;
			return S_OK;
		}
		HRESULT OnDragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
		{
			/*
			DROPEFFECT_NONE=0 表示此窗口不能接受拖放。
			DROPEFFECT_COPY=1 表示拖放将引起源对象的。
			DROPEFFECT_MOVE=2 表示拖放的结果将使源对象被删除。
			DROPEFFECT_LINK=4 表示拖放源对象创建了一个对自己的连接。
			DROPEFFECT_SCROLL=0x80000000表示拖放目标窗口正在或将要进行卷滚。此标志可以和其他几个合用。
			对于拖放对象来说，一般只要使用DROPEFFECT_NONE和DROPEFFECT_COPY即可。
			*/
			DWORD effect = DROPEFFECT_COPY;
			//effect = _p->on_dragover(pt.x, pt.y) ? DROPEFFECT_COPY : DROPEFFECT_NONE;
			_p->on_dragover(pt.x, pt.y);
			*pdwEffect = effect;
			return S_OK;
		}
		HRESULT OnDragLeave()
		{
			_p->_drop_state = false;
			return S_OK;
		}
		HRESULT OnDrop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
		{
			GetDropData(pDataObj);
			_p->_drop_state = false;
			return S_OK;
		}
		bool isUTF8(const std::string& str) {
			int i = 0;
			while (i < str.size()) {
				unsigned char ch = str[i];
				if (ch >= 0 && ch <= 127) { // 单字节字符
					i++;
				}
				else if ((ch & 0xE0) == 0xC0) { // 双字节字符
					if (i + 1 >= str.size() || (str[i + 1] & 0xC0) != 0x80) {
						return false;
					}
					i += 2;
				}
				else if ((ch & 0xF0) == 0xE0) { // 三字节字符
					if (i + 2 >= str.size() || (str[i + 1] & 0xC0) != 0x80 || (str[i + 2] & 0xC0) != 0x80) {
						return false;
					}
					i += 3;
				}
				else { // 不是UTF-8
					return false;
				}
			}
			return true;
		}

		bool isGBK(const std::string& str) {
			for (unsigned char ch : str) {
				if (ch >= 0 && ch <= 127) { // 单字节字符
					continue;
				}
				else if ((ch >= 0x81 && ch <= 0xFE) && (ch != 0x7F)) { // GBK双字节字符
					continue;
				}
				else {
					return false;
				}
			}
			return true;
		}
		void GetDropData(__RPC__in_opt IDataObject* pDataObj)
		{
			IEnumFORMATETC* pEnumFmt = NULL;
			HRESULT ret = pDataObj->EnumFormatEtc(DATADIR_GET, &pEnumFmt);
			if (SUCCEEDED(ret))
			{
				pEnumFmt->Reset();
				HRESULT Ret = S_OK;
				FORMATETC cFmt = { (CLIPFORMAT)CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

				STGMEDIUM stgMedium;
				//
				ULONG Fetched = 0;
				size_t inc = 0;
				do {
					Ret = pEnumFmt->Next(1, &cFmt, &Fetched);
					if (SUCCEEDED(ret))
					{
						if (cFmt.cfFormat == CF_HDROP ||
							cFmt.cfFormat == CF_TEXT || cFmt.cfFormat == CF_OEMTEXT ||
							cFmt.cfFormat == CF_UNICODETEXT
							)
						{
							if (cFmt.cfFormat != CF_HDROP)
								cFmt.cfFormat = CF_UNICODETEXT;
							ret = pDataObj->GetData(&cFmt, &stgMedium);

							if (FAILED(ret))
								return;

							if (stgMedium.pUnkForRelease != NULL)
								return;
							///////////////////////////////////////////
							switch (stgMedium.tymed)
							{
							case TYMED_HGLOBAL:
							{
								DRAGDATA pData;
								pData.cfFormat = cFmt.cfFormat;
								memcpy(&pData.stgMedium, &stgMedium, sizeof(STGMEDIUM));

								if (cFmt.cfFormat == CF_HDROP)
								{
									ProcessDrop((HDROP)stgMedium.hGlobal);
								}
								if (cFmt.cfFormat == CF_TEXT || cFmt.cfFormat == CF_OEMTEXT)
								{
									char* pBuff = 0;

									HGLOBAL  hText = stgMedium.hGlobal;
									pBuff = (LPSTR)GlobalLock(hText);
									{
										size_t tlen = GlobalSize(hText);
										if (pBuff && tlen)
										{
											_p->_str.assign(pBuff, tlen);
											if (isGBK(_p->_str) || !isUTF8(_p->_str)) {
												_p->_str = hz::gbk_to_u8(_p->_str);
												//printf("gbk\n");
											}
										}
										_p->on_drop(0, inc++);
									}
									GlobalUnlock(hText);
								}
								if (cFmt.cfFormat == CF_UNICODETEXT)
								{
									HGLOBAL  hText = stgMedium.hGlobal;
									LPWSTR pW = (LPWSTR)GlobalLock(hText);
									size_t tlen = GlobalSize(hText);
									std::wstring wstr(pW, tlen / sizeof(wchar_t));
									std::string str = hz::u16_to_u8(wstr);
									{
										_p->_str = str;
										_p->on_drop(0, inc++);
									}
									GlobalUnlock(hText);
								}

								return;
								break;
							}

							default:                        // type not supported, so return error
							{
								::ReleaseStgMedium(&stgMedium);
							}
							break;
							}
						}
					}
				} while (Ret == S_OK);
			}
		}
		BOOL ProcessDrop(HDROP hDrop)
		{
			UINT iFiles = -1, ich = 0;
			int bufsize = 8192;
			std::wstring buf;
			std::string str;
			std::vector<std::string> fns;
			buf.resize(bufsize);
			memset(&iFiles, 0xff, sizeof(iFiles));
			int Count = ::DragQueryFileW(hDrop, iFiles, buf.data(), 0);
			if (Count)
			{
				for (int i = 0; i < Count; ++i)
				{
					if (::DragQueryFileW(hDrop, i, buf.data(), bufsize))
					{
						str = hz::u16_to_u8(buf.c_str());
						fns.push_back(str);
					}
				}
				_p->_files.swap(fns);
				_p->on_drop(1, Count);
			}
			::DragFinish(hDrop);
			return true;
		}
	};

#endif

	//-----------------------------------------------------------------------------

#if 0
	//示例：
	//	接收拖动OLE

	CDropTargetEx _WDrop;
	void init()
	{
		::DragAcceptFiles(_hWnd, false);

		_WDrop.DragDropRegister(_hWnd);
		_WDrop.setOver([this](int x, int y)
			{
				// 拖动的时候鼠标事件
				POINT pt = { x,y };
				ScreenToClient(_hWnd, &pt);
				//printf("drop:x<%d>,y<%d>\n", pt.x, pt.y);
				Node* n = getPickNode({ pt.x, pt.y });
				onMouseMoveCallBack(0, pt.x, pt.y);

			});
	}
	// 创建接收处理事件
	_drop = GuiDropTarget::create();
	_drop->pOnChar = pOnChar;
	_drop->pOnOver = pOnOver;

	// 设置当前接收
	IDuiDropTarget* pDuiDropTarget = _drop : nullptr;
	_WDrop.SetDragDrop(pDuiDropTarget);


	//拖动_mouse_txt文本到OLE,类型可用CF_UNICODETEXT或CF_TEXT
	DoDragDropBegin(DragDropData{ CF_UNICODETEXT, (void*)_mouse_txt.c_str(), (int)_mouse_txt.length(), &_isDragEnd }, [this]()
		{
			if (!getReadonly())
				this->on_ime_char("", 0);
		});
#endif
}





namespace hz {

	drop_info_cx::drop_info_cx()
	{
		make_pri();
	}

	drop_info_cx::~drop_info_cx()
	{
		clear_pri();
	}

	void drop_info_cx::make_pri()
	{
#ifdef _WIN32
		if (!_pri)
		{
			auto prid = new GuiDropTarget();
			prid->_p = this;
			_pri = prid;
		}
#endif // _WIN32
	}

	void drop_info_cx::clear_pri()
	{
#ifdef _WIN32
		if (_pri)
		{
			auto prid = (GuiDropTarget*)_pri;
			delete prid; _pri = 0;
		}
#endif // _WIN32
	}

	void drop_info_cx::on_drop(int type, int idx)
	{
		_type = type;
		if (on_drop_cb)
		{
			on_drop_cb(type, idx);
		}
	}

	bool drop_info_cx::on_dragover(int x, int y)
	{
		if (on_dragover_cb)
		{
			on_dragover_cb(x, y);
		}
		return true;
	}

	drop_regs::drop_regs()
	{
	}

	drop_regs::~drop_regs()
	{
		release();
	}

	void drop_regs::init(void* hwnd)
	{
#ifdef _WIN32
		CDropTargetEx* ctx = (CDropTargetEx*)_ctx;
		if (!_ctx)
		{
			ctx = new CDropTargetEx();
			_ctx = ctx;
		}
		if (!_hwnd)
		{
			if (ctx->DragDropRegister((HWND)hwnd))
			{
				_hwnd = hwnd;
			}
		}
#endif // _WIN32
	}

	void drop_regs::revoke()
	{
#ifdef _WIN32
		if (!_ctx)return;
		CDropTargetEx* ctx = (CDropTargetEx*)_ctx;
		ctx->DragDropRevoke(0);
#endif // _WIN32
	}

	void drop_regs::release()
	{
#ifdef _WIN32
		if (!_ctx)return;
		CDropTargetEx* ctx = (CDropTargetEx*)_ctx;
		delete ctx; _ctx = 0;
#endif // _WIN32
	}

	void drop_regs::set_target(drop_info_cx* p)
	{
#ifdef _WIN32
		if (!_ctx)return;
		CDropTargetEx* ctx = (CDropTargetEx*)_ctx;
		GuiDropTarget* cp = 0;
		if (p)
		{
			cp = (GuiDropTarget*)p->_pri;
		}
		ctx->SetDragDrop(cp);
#endif // _WIN32
	}
	void drop_regs::set_over(std::function<void(int x, int y, int fmt)> overfunc)
	{
#ifdef _WIN32
		if (!_ctx || !overfunc)return;
		CDropTargetEx* ctx = (CDropTargetEx*)_ctx;
		ctx->setOver(overfunc);
#endif // _WIN32
	}
#ifndef CF_TEXT
#define CF_TEXT 1
#define CF_UNICODETEXT 13
#define CF_PRIVATEFIRST 0x0200
#define CF_PRIVATELAST 0x02ff
#endif // !CF_TEXT

	bool do_dragdrop_begin(unsigned short cf, const char* data, size_t dsize)
	{
#ifdef _WIN32
		CF_UNICODETEXT; CF_OEMTEXT; CF_TEXT; CF_PRIVATEFIRST; CF_PRIVATELAST;
		//OleInitialize(NULL);
		SdkDataObject* pDataObject = new SdkDataObject(NULL);
		pDataObject->SetBlob(cf, data, dsize);
		SdkDropSource* pSource = new SdkDropSource();
		DWORD  pdwEffect = 0;
		auto hr = DoDragDrop(pDataObject, pSource, DROPEFFECT_MOVE | DROPEFFECT_COPY, &pdwEffect);
		pDataObject->Release();
		pSource->Release();
		//OleUninitialize();
		return pdwEffect;// &DROPEFFECT_MOVE;
#else
		return false;
#endif // _WIN32
	}
	bool do_dragdrop_begin(const char* str, size_t size)
	{
		return do_dragdrop_begin(CF_TEXT, str, size);
	}
	bool do_dragdrop_begin(const wchar_t* str, size_t size)
	{
		return do_dragdrop_begin(CF_UNICODETEXT, (char*)str, size * sizeof(wchar_t));
	}
	bool do_dragdrop_begin(const std::string& str)
	{
		return do_dragdrop_begin(CF_TEXT, str.c_str(), str.size());
	}
	bool do_dragdrop_begin(const std::wstring& str)
	{
		return do_dragdrop_begin(CF_UNICODETEXT, (char*)str.c_str(), str.size() * sizeof(wchar_t));
	}




	// 全屏截图
	void get_fullscreen_image(std::vector<uint32_t>* dst, int* width, int* height, const std::string& fn, int quality)
	{
		// 窗口 桌面句柄
		auto hwnd = GetDesktopWindow();
		// 矩形_桌面
		RECT grc = {};
		GetWindowRect(hwnd, &grc);
		auto gdc = GetDC(hwnd);
		//位图 句柄
		auto bitgb = CreateCompatibleBitmap(gdc, grc.right, grc.bottom);
		auto ij_DC = CreateCompatibleDC(gdc); // 创建兼容的内存DC
		SelectObject(ij_DC, bitgb);
		BitBlt(ij_DC, 0, 0, grc.right, grc.bottom, gdc, 0, 0, SRCCOPY);
		// 位 图
		BITMAP  bm = { 0 };
		GetObject(bitgb, sizeof(BITMAP), &bm);
		// 文件头
		BITMAPFILEHEADER bm_fnt = { 0 };
		// 信息头
		char bmit[256] = {};
		BITMAPINFO* bmpInfo = (BITMAPINFO*)bmit;
		int w = 0, h = 0;
		if (!width)width = &w;
		if (!height)height = &h;
		*width = bm.bmWidth;
		*height = bm.bmHeight;
		DWORD ds = bm.bmWidth * bm.bmHeight; // 16位 = 8字节
		std::vector<uint32_t> dst0;
		if (!dst)dst = &dst0;
		// 数据指针 
		dst->resize(ds);
		auto data = dst->data();
		int ct = 1;
		//填充文件 头
		bm_fnt.bfType = 0x4D42; //'BM' 必须是 没有为什么
		bm_fnt.bfSize = ds + 54;
		bm_fnt.bfReserved1 = 0; //保留 没有为什么
		bm_fnt.bfReserved2 = 0; //同上
		bm_fnt.bfOffBits = 54;
		//填充信息头
		auto& bt = bmpInfo->bmiHeader;
		bt.biSize = sizeof(BITMAPINFO) + 8; //本结构大小 40直接sizeof
		bt.biHeight = -bm.bmHeight;
		bt.biWidth = bm.bmWidth;
		bt.biPlanes = 1; //必须是1 没有理由
		bt.biBitCount = 32; //32真彩高清
		bt.biCompression = BI_RGB;
		bt.biSizeImage = ds; // = bm数据大小
		bt.biXPelsPerMeter = 0;
		bt.biYPelsPerMeter = 0;
		bt.biClrUsed = 0;
		bt.biClrImportant = 0;

		bmpInfo->bmiHeader.biCompression = BI_BITFIELDS;
		int iss = sizeof(bmpInfo);
		LPRGBQUAD bmic = bmpInfo->bmiColors;
		if (ct == 0)
		{
			//RGB
			*(UINT*)(bmic + 2) = 0xFF0000;	// red分量
			*(UINT*)(bmic + 1) = 0x00FF00;	// green分量
			*(UINT*)(bmic + 0) = 0x0000FF;	// blue分量
		}
		else
		{
			//BGR
			*(UINT*)(bmic + 0) = 0xFF0000;	// red分量
			*(UINT*)(bmic + 1) = 0x00FF00;	// green分量
			*(UINT*)(bmic + 2) = 0x0000FF;	// blue分量
		}
		GetDIBits(gdc, bitgb, 0, bm.bmHeight, data, (LPBITMAPINFO)bmpInfo, DIB_RGB_COLORS);
		auto length = dst->size();
		for (size_t i = 0; i < length; i++)
		{
			uint8_t* px = (uint8_t*)(&data[i]);
			std::swap(px[0], px[2]);//bgr转rgb
		}
		if (fn.size() > 2)
		{
			if (fn.find("png") != std::string::npos)
				stbi_write_png(fn.c_str(), w, h, 4, data, w * sizeof(int));
			if (fn.find("jpg") != std::string::npos)
				stbi_write_jpg(fn.c_str(), w, h, 4, data, quality);
		}
		DeleteObject(bitgb);
		DeleteDC(ij_DC);
		ReleaseDC(hwnd, gdc);
	}






}
//!hz
#endif // _WIN32