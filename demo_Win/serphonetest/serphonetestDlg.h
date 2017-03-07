
// serphonetestDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include <list>

#include "DlgFullScreen.h"

class ServiceCore;
struct _SerPhoneCall;
typedef struct _SerPhoneCall  SerPhoneCall;

#ifndef PATH_MAX
#define  PATH_MAX  256
#endif

using namespace  std;

typedef struct _ConfMember {
	std::string roomid;
	std::string sipNo;
	std::string videoIP;
	int              videoPort;
	CWnd *videoWnd;
}ConfMember;

const UINT CALL_STATE_CHANGE   = 0x1421;
const UINT RECV_TEXT           = 0x1422;
const UINT UPDATE_DLG          = 0x1423;
// CserphonetestDlg 对话框
class CserphonetestDlg : public CDialogEx
{
// 构造
public:
	CserphonetestDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CserphonetestDlg();

// 对话框数据
	enum { IDD = IDD_SERPHONETEST_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

protected:
	ServiceCore  *p_service;
	SerPhoneCall *p_CurrentCall;
    char configfile_name[PATH_MAX];
public:

	ConfMember *_firtMember;
	ConfMember *_secordMember;
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT OnCallStateChange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRecvMsgTxt(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateDlg(WPARAM wParam, LPARAM lParam);
	CString m_outCallAddr;
	CString m_inComingCall;
	CString m_sipAccount;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton6();
	CButton m_startbutton;
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton7();
	afx_msg void OnBnClickedButton14();
	afx_msg void OnBnClickedButton8();
	afx_msg void OnBnClickedButton9();
	afx_msg void OnBnClickedButton10();
	afx_msg void OnBnClickedButton11();
	afx_msg void OnBnClickedButton12();
	afx_msg void OnBnClickedButton13();
	afx_msg void OnBnClickedButton15();
	afx_msg void OnBnClickedButton16();
	afx_msg void OnBnClickedButton17();
	afx_msg void OnBnClickedButton18();
	afx_msg void OnBnClickedButton19();
	CString m_playfiletoremote;
	CButton m_playfilebutton;
	afx_msg void OnBnClickedButton20();
	CString m_transfercall;
	afx_msg void OnBnClickedButton21();
	CString m_messagetxt;
	afx_msg void OnBnClickedButton22();
	CString m_reveiver;
	afx_msg void OnEnChangeRichedit21();
	afx_msg void OnClose();
	afx_msg void OnBnClickedAutotest();
	CButton m_btnAutoTest;
	afx_msg void OnBnClickedButton23();
	afx_msg void OnBnClickedButton24();
	afx_msg void OnBnClickedButton25();
	afx_msg void OnBnClickedButton26();
	afx_msg void OnBnClickedButton27();
	CString m_audioPayloadType;
	CString m_sipID;
	CString m_MediaServerIP;
	CString m_sipPassword;
	CString m_CameraIdx;
	CString m_CameraCapIdx;
	CRichEditCtrl m_richEdit21;
//	CListBox m_SipAccountListCtl;
	CComboBox m_sipAccountCtrl;
	afx_msg void OnBnClickedButton28();
	CComboBox m_enableP2PCtrl;
	CComboBox m_enableNACKCtrl;
	CComboBox m_enableREMBCtrl;
	CComboBox m_enableTMMBRCtrl;
	CRichEditCtrl m_RichEditCtrl23;
	afx_msg void OnBnClickedButton29();

private:
	BOOL m_bExpand;
	int m_nExpandedWidth;
	int m_nNormalWidth;

	//成员变量  
	BOOL            m_bFullScreen;     // 全屏标志  
	CRect           m_FullScreenRect;  // 整个屏幕尺寸  
	WINDOWPLACEMENT m_OldWndPlacement; // 全屏时对话框的位置属性  
	WINDOWPLACEMENT m_NewWndPlacement; // 全屏后对话框的位置属性  
public:
	UINT m_totalSentBr;
	UINT m_videoSentBr;
	UINT m_fecSentBr;
	UINT m_nackSentBr;
	UINT m_estimatedBrBySender;
	UINT m_estimatedBrByReceiver;
	UINT m_jitterRtcpReceiver;
	UINT m_fractionLostRtcpReceiver;
	UINT m_cummutiveLostPackRtcpReceiver;
	UINT m_jitterRtcpSender;
	UINT m_fractionLostRtcpSender;
	UINT m_cummutiveLostRtcpSender;
	UINT m_retransmittedPacketsRtpReceiver;
	UINT m_retransmittedPacketsRtpSender;
	UINT m_packetRtpSend;
	UINT m_packetRtpReceived;
	DWORD m_firstPacketRtpReceivedInterval;
	DWORD m_firstPacketRtpSendInterval;
	int m_SentBitrate;
	int m_SentFramerate;
	UINT m_ReceivedBitrate;
	UINT m_ReceivedFrameRate;
	CString m_ReceivedResolution;
	CString m_SentResolution;
	int     m_ReceivedResolution_width;
	int		m_ReceivedResolution_height;
	int		m_SentResolution_width;
	int		m_SentResolution_height;

	char serverIP[16];
	char serverPort[10];
	char sipAcount1[20],sipAcount2[20];
	char sipAcount1Password[20],sipAcount2Password[20];
	UINT m_videoPT;
	CString m_videoPayloadeName;
	CString m_audioPayloadName;
	CString m_encryptionKey;
	UINT m_audioPT;
	afx_msg void OnBnClickedButtonDesktopShare();
	CComboBox m_videoCodecCtrl;
	CComboBox m_videoModeCtrl;
	CComboBox m_encryptType;
	unsigned int m_videoCodec; //0: h264, 1: VP8
	unsigned int m_videoMode; //0: real-time, 1: screen-share
	//以下为读配置文件
	char m_desktop_width[5];
	char m_desktop_height[5];
	char m_desktop_frame_rate[5];
	char m_desktop_bit_rate[10];
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnEnMsgfilterRichedit21(NMHDR *pNMHDR, LRESULT *pResult);

public: 
	CDlgFullScreen *m_dlgFullScreen;

	long long m_RTTSender;
	CComboBox m_videoProtectionMode;
	afx_msg void OnCbnSelchangeVideoSource();
	afx_msg void OnBnClickedPlayStream();
	afx_msg void OnBnClickedStopLive();
	afx_msg void OnEnChangeLiveUrl();
	afx_msg void OnBnClickedPushStream();
	CString m_live_url;

	CComboBox m_video_source;
	CComboBox m_share_windows;
	afx_msg void OnCbnSelchangeShareWindow();
	afx_msg void OnCbnDropdownShareWindow();
	afx_msg void OnEnChangeEdit10();
	afx_msg void OnEnChangeEdit12();
	afx_msg void OnEnChangeEdit14();
	afx_msg void OnEnChangeEdit11();
	afx_msg void OnEnChangeAudioPayloadName();
	afx_msg void OnEnChangeAudioPt();
	afx_msg void OnEnChangeVideoPayloadName();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnEnChangeEdit3();
	afx_msg void OnEnChangeEdit4();
	afx_msg void OnEnChangeEncryptionKey();
	afx_msg void OnCbnSelchangeComboVideoCodec();
	afx_msg void OnCbnSelchangeComboVideoMode();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnCbnSelchangeComboEncryptType();
};
