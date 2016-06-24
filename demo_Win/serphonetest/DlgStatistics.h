#pragma once


// CDlgStatistics 对话框

class CDlgStatistics : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgStatistics)

public:
	CDlgStatistics(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDlgStatistics();

// 对话框数据
	enum { IDD = IDD_DIALOG_STATISTICS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
