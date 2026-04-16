
// TouchPadEnableDlg.h: 头文件
//

#pragma once

#include <shellapi.h>

#define WM_TRAYICON              (WM_USER + 100)
#define WM_UPDATE_TOUCHPAD_STATUS (WM_USER + 101)
#define HOTKEY_ID_TOGGLE    1

// OSD 窗口类
class COSDWnd : public CWnd
{
public:
	COSDWnd();
	BOOL CreateOSD();
	void ShowOSD(LPCTSTR pszText, int nDurationMs = 2000);

protected:
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	DECLARE_MESSAGE_MAP()

private:
	CString m_strText;
	static const UINT_PTR TIMER_HIDE_OSD = 100;
	static const int OSD_WIDTH = 400;
	static const int OSD_HEIGHT = 80;
};

// CTouchPadEnableDlg 对话框
class CTouchPadEnableDlg : public CDialogEx
{
// 构造
public:
	CTouchPadEnableDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TOUCHPADENABLE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	NOTIFYICONDATA m_nid;
	COSDWnd m_osdWnd;

	void CreateTrayIcon();
	void RemoveTrayIcon();
	void ToggleTouchPad();
	int GetTouchPadStatus();

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnHotKey(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTrayIcon(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateTouchPadStatus(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTrayExit();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonEnable();
	int showTouchPadStatus();
};
