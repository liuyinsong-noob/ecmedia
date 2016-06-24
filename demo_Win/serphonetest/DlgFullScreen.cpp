// DlgFullScreen.cpp : 实现文件
//

#include "stdafx.h"
#include "serphonetest.h"
#include "DlgFullScreen.h"
#include "afxdialogex.h"


// CDlgFullScreen 对话框

IMPLEMENT_DYNAMIC(CDlgFullScreen, CDialogEx)

CDlgFullScreen::CDlgFullScreen(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlgFullScreen::IDD, pParent)
{

}

CDlgFullScreen::~CDlgFullScreen()
{
}

void CDlgFullScreen::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgFullScreen, CDialogEx)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEHOVER()
END_MESSAGE_MAP()


// CDlgFullScreen 消息处理程序


void CDlgFullScreen::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	PostMessage(WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(point.x,point.y));
	CDialogEx::OnLButtonDown(nFlags, point);
}


void CDlgFullScreen::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnLButtonUp(nFlags, point);
}


void CDlgFullScreen::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnMouseHover(nFlags, point);
}
