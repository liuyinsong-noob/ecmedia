// DlgStatistics.cpp : 实现文件
//

#include "stdafx.h"
#include "serphonetest.h"
#include "DlgStatistics.h"
#include "afxdialogex.h"


// CDlgStatistics 对话框

IMPLEMENT_DYNAMIC(CDlgStatistics, CDialogEx)

CDlgStatistics::CDlgStatistics(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlgStatistics::IDD, pParent)
{

}

CDlgStatistics::~CDlgStatistics()
{
}

void CDlgStatistics::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgStatistics, CDialogEx)
END_MESSAGE_MAP()


// CDlgStatistics 消息处理程序
