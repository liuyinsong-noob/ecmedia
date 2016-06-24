#pragma once


// CDlgFullScreen 对话框

class CDlgFullScreen : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgFullScreen)

public:
	CDlgFullScreen(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDlgFullScreen();

// 对话框数据
	enum { IDD = IDD_DIALOG_FULL_SCREEN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
};
