
// TouchPadEnableDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "TouchPadEnable.h"
#include "TouchPadEnableDlg.h"
#include "afxdialogex.h"
#include <thread>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


///////////////////////////////////////////////////////////
// COSDWnd - OSD 透明显示窗口
///////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(COSDWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

COSDWnd::COSDWnd()
{
}

BOOL COSDWnd::CreateOSD()
{
	// 注册窗口类
	CString strClass = AfxRegisterWndClass(
		CS_HREDRAW | CS_VREDRAW,
		::LoadCursor(NULL, IDC_ARROW),
		NULL, NULL);

	// 创建 WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT 窗口
	// 不显示在任务栏，始终置顶，不接收焦点
	return CreateEx(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
		strClass,
		_T("OSD"),
		WS_POPUP,
		0, 0, OSD_WIDTH, OSD_HEIGHT,
		NULL, NULL);
}

void COSDWnd::ShowOSD(LPCTSTR pszText, int nDurationMs)
{
	m_strText = pszText;

	// 取主显示器信息，居中偏下显示
	POINT ptZero = { 0, 0 };
	HMONITOR hMon = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfo(hMon, &mi);

	int x = mi.rcMonitor.left + (mi.rcMonitor.right - mi.rcMonitor.left - OSD_WIDTH) / 2;
	int y = mi.rcMonitor.top + (mi.rcMonitor.bottom - mi.rcMonitor.top) * 3 / 4;

	// 使用 UpdateLayeredWindow 实现半透明效果
	HDC hdcScreen = ::GetDC(NULL);
	HDC hdcMem = ::CreateCompatibleDC(hdcScreen);
	HBITMAP hBmp = ::CreateCompatibleBitmap(hdcScreen, OSD_WIDTH, OSD_HEIGHT);
	HBITMAP hOldBmp = (HBITMAP)::SelectObject(hdcMem, hBmp);

	// 填充半透明黑色背景 (ARGB)
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = OSD_WIDTH;
	bmi.bmiHeader.biHeight = -OSD_HEIGHT; // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void* pBits = NULL;
	HBITMAP hDibBmp = ::CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
	HDC hdcDib = ::CreateCompatibleDC(hdcScreen);
	HBITMAP hOldDib = (HBITMAP)::SelectObject(hdcDib, hDibBmp);

	// 填充半透明黑色 (alpha=200)
	DWORD* pPixels = (DWORD*)pBits;
	BYTE bgAlpha = 200;
	for (int i = 0; i < OSD_WIDTH * OSD_HEIGHT; i++)
	{
		// premultiplied ARGB: B=0, G=0, R=0, A=bgAlpha
		pPixels[i] = (bgAlpha << 24);
	}

	// 画圆角矩形区域（简化处理：直接用全矩形）
	// 在 DIB 上绘制文字
	::SetBkMode(hdcDib, TRANSPARENT);

	// 创建字体
	LOGFONT lf = {};
	lf.lfHeight = 40;
	lf.lfWeight = FW_BOLD;
	_tcscpy_s(lf.lfFaceName, _T("Segoe UI"));
	HFONT hFont = ::CreateFontIndirect(&lf);
	HFONT hOldFont = (HFONT)::SelectObject(hdcDib, hFont);

	// 预乘白色文字 (alpha=255)
	::SetTextColor(hdcDib, RGB(255, 255, 255));

	RECT rcText = { 0, 0, OSD_WIDTH, OSD_HEIGHT };
	::DrawText(hdcDib, m_strText, m_strText.GetLength(), &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	// 修正文字像素的alpha值 - 文字区域设为不透明
	for (int i = 0; i < OSD_WIDTH * OSD_HEIGHT; i++)
	{
		BYTE b = (BYTE)(pPixels[i] & 0xFF);
		BYTE g = (BYTE)((pPixels[i] >> 8) & 0xFF);
		BYTE r = (BYTE)((pPixels[i] >> 16) & 0xFF);
		BYTE a = (BYTE)((pPixels[i] >> 24) & 0xFF);

		// 如果 RGB 都是 0 且 alpha 是背景值，保持不变
		// 如果有文字绘制（RGB != 0），设置为完全不透明
		if (r > 0 || g > 0 || b > 0)
		{
			// 文字像素：使用完全不透明
			a = 255;
			pPixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	}

	::SelectObject(hdcDib, hOldFont);
	::DeleteObject(hFont);

	// 移动窗口到位
	MoveWindow(x, y, OSD_WIDTH, OSD_HEIGHT);

	// UpdateLayeredWindow
	POINT ptSrc = { 0, 0 };
	SIZE szWnd = { OSD_WIDTH, OSD_HEIGHT };
	POINT ptDst = { x, y };
	BLENDFUNCTION bf = {};
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = 255;
	bf.AlphaFormat = AC_SRC_ALPHA;

	::UpdateLayeredWindow(GetSafeHwnd(), hdcScreen, &ptDst, &szWnd, hdcDib, &ptSrc, 0, &bf, ULW_ALPHA);

	// 清理
	::SelectObject(hdcDib, hOldDib);
	::DeleteObject(hDibBmp);
	::DeleteDC(hdcDib);
	::SelectObject(hdcMem, hOldBmp);
	::DeleteObject(hBmp);
	::DeleteDC(hdcMem);
	::ReleaseDC(NULL, hdcScreen);

	// 显示窗口
	ShowWindow(SW_SHOWNOACTIVATE);

	// 设置定时器自动隐藏
	KillTimer(TIMER_HIDE_OSD);
	SetTimer(TIMER_HIDE_OSD, nDurationMs, NULL);
}

void COSDWnd::OnPaint()
{
	CPaintDC dc(this);
	// 由 UpdateLayeredWindow 控制，不需要在这里画
}

void COSDWnd::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_HIDE_OSD)
	{
		KillTimer(TIMER_HIDE_OSD);
		ShowWindow(SW_HIDE);
	}
	CWnd::OnTimer(nIDEvent);
}

BOOL COSDWnd::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}


///////////////////////////////////////////////////////////
// CAboutDlg - 关于对话框
///////////////////////////////////////////////////////////

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


///////////////////////////////////////////////////////////
// CTouchPadEnableDlg 对话框
///////////////////////////////////////////////////////////

CTouchPadEnableDlg::CTouchPadEnableDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TOUCHPADENABLE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	memset(&m_nid, 0, sizeof(m_nid));
}

void CTouchPadEnableDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTouchPadEnableDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_MESSAGE(WM_HOTKEY, &CTouchPadEnableDlg::OnHotKey)
	ON_MESSAGE(WM_TRAYICON, &CTouchPadEnableDlg::OnTrayIcon)
	ON_MESSAGE(WM_UPDATE_TOUCHPAD_STATUS, &CTouchPadEnableDlg::OnUpdateTouchPadStatus)
	ON_BN_CLICKED(IDC_BUTTON_ENABLE, &CTouchPadEnableDlg::OnBnClickedButtonEnable)
	ON_COMMAND(ID_TRAY_EXIT, &CTouchPadEnableDlg::OnTrayExit)
END_MESSAGE_MAP()


// CTouchPadEnableDlg 消息处理程序

BOOL CTouchPadEnableDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将"关于..."菜单项添加到系统菜单中。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// 创建 OSD 窗口（必须在 showTouchPadStatus 之前）
	m_osdWnd.CreateOSD();

	// 创建托盘图标
	CreateTrayIcon();

	// 注册全局快捷键 Alt+1
	if (!RegisterHotKey(GetSafeHwnd(), HOTKEY_ID_TOGGLE, MOD_ALT, '1'))
	{
		AfxMessageBox(_T("注册快捷键 Alt+1 失败，可能已被其他程序占用。"), MB_ICONWARNING);
	}

	// 显示触摸板状态
	showTouchPadStatus();

	// 启动注册表监视线程
	std::thread([this] {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (hEvent == NULL)
		{
			return;
		}

		HKEY hKey;
		LSTATUS status = RegOpenKeyEx(HKEY_CURRENT_USER,
			_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad\\Status"),
			0,
			KEY_NOTIFY,
			&hKey);
		if (status != ERROR_SUCCESS)
		{
			CloseHandle(hEvent);
			return;
		}

		while (true)
		{
			RegNotifyChangeKeyValue(hKey,
				TRUE,
				REG_NOTIFY_CHANGE_LAST_SET,
				hEvent,
				TRUE);
			if (WaitForSingleObject(hEvent, INFINITE) == WAIT_FAILED)
			{
				break;
			}
			this->PostMessage(WM_UPDATE_TOUCHPAD_STATUS, 0, 0);
		}

		RegCloseKey(hKey);
		CloseHandle(hEvent);
	}).detach();

	// 启动后缩小到托盘（不显示主窗口）
	ShowWindow(SW_HIDE);

	return TRUE;
}

void CTouchPadEnableDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else if ((nID & 0xFFF0) == SC_MINIMIZE)
	{
		// 最小化时也缩小到托盘
		ShowWindow(SW_HIDE);
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 重写 OnCancel：点击关闭按钮(X)时隐藏到托盘而不是退出
void CTouchPadEnableDlg::OnCancel()
{
	ShowWindow(SW_HIDE);
}

void CTouchPadEnableDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CTouchPadEnableDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CTouchPadEnableDlg::OnDestroy()
{
	// 注销快捷键
	UnregisterHotKey(GetSafeHwnd(), HOTKEY_ID_TOGGLE);

	// 移除托盘图标
	RemoveTrayIcon();

	// 销毁 OSD 窗口
	if (m_osdWnd.GetSafeHwnd())
	{
		m_osdWnd.DestroyWindow();
	}

	CDialogEx::OnDestroy();
}

void CTouchPadEnableDlg::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);
}

// 全局快捷键消息处理 Alt+Q
LRESULT CTouchPadEnableDlg::OnHotKey(WPARAM wParam, LPARAM lParam)
{
	if (wParam == HOTKEY_ID_TOGGLE)
	{
		ToggleTouchPad();
	}
	return 0;
}

// 托盘图标消息处理
LRESULT CTouchPadEnableDlg::OnTrayIcon(WPARAM wParam, LPARAM lParam)
{
	if (lParam == WM_RBUTTONUP)
	{
		// 右键弹出菜单
		CMenu menu;
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, ID_TRAY_EXIT, _T("退出"));

		// 获取鼠标位置
		CPoint pt;
		GetCursorPos(&pt);

		// 设置前台窗口，否则菜单无法正常关闭
		SetForegroundWindow();
		menu.TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
		PostMessage(WM_NULL, 0, 0);
	}
	else if (lParam == WM_LBUTTONDBLCLK)
	{
		// 双击托盘图标，显示主窗口
		ShowWindow(SW_SHOW);
		ShowWindow(SW_RESTORE);
		SetForegroundWindow();
	}
	return 0;
}

// 后台线程通知主线程更新触摸板状态（线程安全）
LRESULT CTouchPadEnableDlg::OnUpdateTouchPadStatus(WPARAM wParam, LPARAM lParam)
{
	showTouchPadStatus();
	return 0;
}

// 托盘菜单"退出"
void CTouchPadEnableDlg::OnTrayExit()
{
	// 真正退出程序
	CDialogEx::OnCancel();
}

void CTouchPadEnableDlg::OnBnClickedButtonEnable()
{
	ToggleTouchPad();
}

// 切换触摸板开关
void CTouchPadEnableDlg::ToggleTouchPad()
{
	// 先释放可能被按住的修饰键（如通过 Alt+1 快捷键触发时 Alt 仍被按下）
	keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);

	// 模拟 Ctrl+Win+F24 来切换触摸板
	keybd_event(VK_CONTROL, 0, 0, 0);
	keybd_event(VK_LWIN, 0, 0, 0);
	keybd_event(VK_F24, 0, 0, 0);
	keybd_event(VK_F24, 0, KEYEVENTF_KEYUP, 0);
	keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
	keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
}

// 获取触摸板状态: 1=Enabled, 0=Disabled, -1=Unknown
int CTouchPadEnableDlg::GetTouchPadStatus()
{
	DWORD dwValue = 0;
	DWORD cbData = sizeof(dwValue);
	LSTATUS status = RegGetValue(HKEY_CURRENT_USER,
		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad\\Status"),
		_T("Enabled"),
		RRF_RT_REG_DWORD,
		NULL,
		&dwValue,
		&cbData);
	if (status == ERROR_SUCCESS)
	{
		return (dwValue == 1) ? 1 : 0;
	}
	return -1;
}

int CTouchPadEnableDlg::showTouchPadStatus()
{
	DWORD dwValue = 0;
	DWORD cbData = sizeof(dwValue);
	LSTATUS status = RegGetValue(HKEY_CURRENT_USER,
		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad\\Status"),
		_T("Enabled"),
		RRF_RT_REG_DWORD,
		NULL,
		&dwValue,
		&cbData);
	if (status == ERROR_SUCCESS)
	{
		if (dwValue == 1)
		{
			SetDlgItemText(IDC_STATIC_STATUS, _T("Enabled"));
			m_osdWnd.ShowOSD(_T("TouchPad: Enable"));
		}
		else
		{
			SetDlgItemText(IDC_STATIC_STATUS, _T("Disabled"));
			m_osdWnd.ShowOSD(_T("TouchPad: Disable"));
		}
	}
	else
	{
		SetDlgItemText(IDC_STATIC_STATUS, _T("Unknown"));
		return -1;
	}
	return 0;
}

// 创建托盘图标
void CTouchPadEnableDlg::CreateTrayIcon()
{
	m_nid.cbSize = sizeof(NOTIFYICONDATA);
	m_nid.hWnd = GetSafeHwnd();
	m_nid.uID = 1;
	m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	m_nid.uCallbackMessage = WM_TRAYICON;
	m_nid.hIcon = m_hIcon;
	_tcscpy_s(m_nid.szTip, _T("TouchPad Enable (Alt+1)"));
	Shell_NotifyIcon(NIM_ADD, &m_nid);
}

// 移除托盘图标
void CTouchPadEnableDlg::RemoveTrayIcon()
{
	Shell_NotifyIcon(NIM_DELETE, &m_nid);
}
