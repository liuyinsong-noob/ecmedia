// serphonetestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "serphonetest.h"
#include "serphonetestDlg.h"
#include "afxdialogex.h"
#include "CCPClient.h"
#include "CCPClient_Internal.h"
#include "../ECMedia/interface/ECMedia.h"

//#include "minIni.h"

#include < windows.h >
#include < process.h >

#include <atlconv.h>

#ifdef DEBUG
#include "../third_party/vld/include/vld.h"
#endif

#include "DlgStatistics.h"

static CDlgFullScreen *g_dlgFullScreen = NULL;



#define TIMER_STATISTICS 2

typedef unsigned long cctestthread;
int cc_test_createthread(cctestthread& t, void *(*f) (void *), void* p)
{
	return t = (cctestthread)_beginthread((void( __cdecl * )( void * ))f, 0, p);
}

void onLogInfo(const char *loginfo) ;

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



typedef struct statisticInfo {
	int incomingCount;
	int outgoingCount;
	int altertingCount;
	int outAlteringTime;
	time_t lastCallOutTime;
} TStatisticInfo;


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框
CserphonetestDlg *g_dlg=NULL;
char g_currentCallId[128] = {0};
static bool g_bAutoTest = false;
TStatisticInfo g_statisticInfo;
unsigned int g_testTimer = 0;

void * g_rtmpLiveStreamHandle = NULL;

wchar_t* TransformUTF8ToUnicodeM(const char* _str)
{
	int textlen =0;
	wchar_t * result = NULL;
	if (_str)
	{
		textlen = MultiByteToWideChar( CP_UTF8, 0, _str,-1, NULL,0 );
		result = (wchar_t *)malloc((textlen+1)*sizeof(wchar_t));
		memset(result,0,(textlen+1)*sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0,_str,-1,(LPWSTR)result,textlen );
	}
	return result;
}
char* TransformUnicodeToUTF8M(const wchar_t* _str)
{
	char* result = NULL;
	int textlen = 0;
	if (_str)
	{
		textlen = WideCharToMultiByte( CP_UTF8, 0, _str, -1, NULL, 0, NULL, NULL );
		result =(char *)malloc((textlen+1)*sizeof(char));
		memset(result, 0, sizeof(char) * ( textlen + 1 ) );
		WideCharToMultiByte( CP_UTF8, 0, _str, -1, result, textlen, NULL, NULL );
	}
	return result;
}

int ANSIToUTF8(char *pszCode, char *UTF8code)
{
	WCHAR Unicode[100]={0,}; 
	char utf8[100]={0,};

	// read char Lenth
	int nUnicodeSize = MultiByteToWideChar(CP_ACP, 0, pszCode, strlen(pszCode), Unicode, sizeof(Unicode)); 

	// read UTF-8 Lenth
	int nUTF8codeSize = WideCharToMultiByte(CP_UTF8, 0, Unicode, nUnicodeSize, UTF8code, sizeof(Unicode), NULL, NULL); 

	// convert to UTF-8 
	MultiByteToWideChar(CP_UTF8, 0, utf8, nUTF8codeSize, Unicode, sizeof(Unicode)); 
	return nUTF8codeSize;
}

void onIncomingCallReceived( int type ,const char * callid, const char *caller) 
{
	g_dlg->m_inComingCall = caller;
	strncpy( g_currentCallId,callid,sizeof(g_currentCallId)-1);
	g_dlg->PostMessageW(UPDATE_DLG,0,0);
	g_dlg->SetDlgItemText(IDC_CALL_STATE,L"有呼入,请应答...") ;

	acceptCallByMediaType(g_currentCallId, 1);
	requestVideo(g_currentCallId, 320, 240);
}

void onConnected()
{	
	g_dlg->SetDlgItemText(IDC_REGISTER_STATE,g_dlg->m_sipAccount+L"  注册成功") ;
	g_dlg->m_startbutton.EnableWindow(true);

}
void onConnectError(int reason)
{
	g_dlg->SetDlgItemText(IDC_REGISTER_STATE,g_dlg->m_sipAccount+L"  注册失败") ;
	g_dlg->m_startbutton.EnableWindow(true);
}

void onCallAlerting( const char *callid) 
{

	if( g_testTimer)
		g_dlg->KillTimer(g_testTimer);

	g_statisticInfo.altertingCount++;
	g_dlg->SetDlgItemText(IDC_CALL_STATE,L"呼叫振铃中...") ;
	if(g_bAutoTest) {

		g_statisticInfo.outAlteringTime += (int)(time(NULL)- g_statisticInfo.lastCallOutTime);
		Sleep(2000);
		releaseCall(g_currentCallId,0);
		int t = rand()%30+10;
		g_testTimer = g_dlg->SetTimer(1,t*1000,0);
	}
}

static void* get_networkd_static(void *p)
{
	const char *callid = (const char *)p;
	int ret;

	while(true) {
		long long duration, send_sim, recv_sim, send_wifi, recv_wifi;

		ret = getNetworkStatistic(callid, &duration, &send_sim, &recv_sim, &send_wifi, &recv_wifi);
		if(ret >=0) {
			char buf[128];
			sprintf(buf, "getNetworkStatistic %llu, %llu, %llu\r\n", duration, send_sim+send_wifi, recv_sim+recv_wifi);
			printf(buf);
			//onLogInfo(buf);
		}

		Sleep(500);
	}
	return NULL;
}

void onCallAnswered( const char *callid) 
{
	g_dlg->SetDlgItemText(IDC_CALL_STATE,L"通话中...") ;
	g_dlg->m_dlgFullScreen->ShowWindow(SW_HIDE);

	//const char * filen = "./123.wav";
	//startRecordVoice(callid, filen);

	//const char *mp4File = "./callRecord.mp4";
	//startRecordVoip(callid, mp4File);

	//cctestthread mythread;
	//cc_test_createthread(mythread, get_networkd_static, (void*)callid);

}

void onMakeCallFailed(const char*callid,int reason) 
{
	g_dlg->SetDlgItemText(IDC_CALL_STATE,L"呼叫失败") ;
}

void onCallReleased(const char*callid) 
{
	g_dlg->m_dlgFullScreen->ShowWindow(SW_HIDE);
	g_dlg->SetDlgItemText(IDC_CALL_STATE,L"呼叫结束");
}

void onCallPaused(const char*callid) 
{
	g_dlg->SetDlgItemText(IDC_CALL_STATE,L"呼叫暂停");
}

void onCallPausedByRemote(const char*callid) 
{
	g_dlg->SetDlgItemText(IDC_CALL_STATE,L"呼叫暂停");
}

void onGetCapabilityToken() 
{
	const char *token = "eyJjYWxsaW5nIjoiMSIsImNhbGxlZCI6IjEiLCJtc2dmbGFnIjoiMSIsImNyZWF0ZWNvbmYiOiIxIiwiam9pbmNvbmYiOiIxIn0=";
	setCapabilityToken(token);
}
void onLogInfo(const char *loginfo) 
{
	BSTR unicodestr;
	int lenA = lstrlenA(loginfo);
	int lenW = ::MultiByteToWideChar(CP_ACP, 0, loginfo,lenA, 0, 0);
	if (lenW > 0)
	{
		// Check whether conversion was successful
		unicodestr = ::SysAllocStringLen(0, lenW);
		::MultiByteToWideChar(CP_ACP, 0, loginfo, lenA, unicodestr, lenW);
		OutputDebugString(unicodestr);
		::SysFreeString(unicodestr);
	}
}
void onTextMessageReceived(const char *sender, const char *receiver, const char *sendtime, const char *msgid, const char *message, const char *userdata)
{
	//BSTR unicodestr;
	//char showMessage[32];
	//sprintf(showMessage, "%s Send:%s", receiver, message);
	//int lenA = lstrlenA(showMessage);
	//int lenW = ::MultiByteToWideChar(CP_UTF8, 0, showMessage,lenA, 0, 0);
	//if (lenW > 0)
	//{
	//		// Check whether conversion was successful
	//	 unicodestr = ::SysAllocStringLen(0, lenW);
	//	::MultiByteToWideChar(CP_UTF8, 0, showMessage, lenA, unicodestr, lenW);
	//	g_dlg->SetDlgItemText(IDC_RICHEDIT21,unicodestr);
	//	::SysFreeString(unicodestr);
	//}
}

void onGroupTextMessageReceived(const char* sender, const char* groupid, const char*message)
{
	BSTR unicodestr;
	char showMessage[320];
	sprintf(showMessage, "%s Send to %s:%s", sender, groupid, message);
	int lenA = lstrlenA(showMessage);
	int lenW = ::MultiByteToWideChar(CP_UTF8, 0, showMessage,lenA, 0, 0);
	if (lenW > 0)
	{
		// Check whether conversion was successful
		unicodestr = ::SysAllocStringLen(0, lenW);
		::MultiByteToWideChar(CP_UTF8, 0, showMessage, lenA, unicodestr, lenW);
		g_dlg->SetDlgItemText(IDC_RICHEDIT21,unicodestr);
		::SysFreeString(unicodestr);
	}

}

void onReceiverStats(const char *callid, const int framerate, const int bitrate)
{
	USES_CONVERSION;
	//int call_id = atoi(callid);
	//CString info;
	//info.Format(A2W("\r\ndecoder stats: callid=%d, framerate = %d, bitrate=%d\r\n"), call_id, framerate, bitrate);
	//g_dlg->m_RichEditCtrl23.SetSel(-1, -1);
	//g_dlg->m_RichEditCtrl23.ReplaceSel((LPCTSTR)info);
	//g_dlg->m_RichEditCtrl23.PostMessage(WM_VSCROLL, SB_BOTTOM,0);

	g_dlg->m_ReceivedBitrate = bitrate/1000;
	g_dlg->m_ReceivedFrameRate = framerate;
}

void onIncomingCodecChanged(const char *callid, const int width, const int height)
{
	g_dlg->m_ReceivedResolution_width = width;
	g_dlg->m_ReceivedResolution_height = height;
}

void onRemoteVideoRatioChanged(const char *callid, int width, int height, bool isVideoConference, const char *sipNo)
{
	char log[1024];
	sprintf(log, "onRemoteVideoRatioChanged callid=%s width=%d height=%d isVideoConference=%d sipNo=%s\n",
		callid, width, height, isVideoConference, sipNo);
	onLogInfo(log);

	if (g_dlgFullScreen)
	{
		CRect   temprect(0, 0, width, height);
		g_dlgFullScreen->MoveWindow(&temprect, TRUE);
		g_dlgFullScreen->ShowWindow(SW_SHOW);
	}
}


class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CserphonetestDlg 对话框

CserphonetestDlg::CserphonetestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CserphonetestDlg::IDD, pParent)
	, m_outCallAddr(_T(""))
	, m_inComingCall(_T(""))
	, m_playfiletoremote(_T(""))
	, m_transfercall(_T(""))
	, m_messagetxt(_T(""))
	, m_reveiver(_T(""))
	, m_live_url(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_sipID = _T("1003");
	m_CameraIdx = _T("1");
	m_CameraCapIdx = _T("4");
	m_bExpand = FALSE;
	m_totalSentBr = 0;
	m_videoSentBr = 0;
	m_fecSentBr = 0;
	m_nackSentBr = 0;
	m_estimatedBrBySender = 0;
	m_jitterRtcpReceiver = 0;
	m_fractionLostRtcpReceiver = 0;
	m_cummutiveLostPackRtcpReceiver = 0;
	m_jitterRtcpSender = 0;
	m_fractionLostRtcpSender = 0;
	m_cummutiveLostRtcpSender = 0;
	m_retransmittedPacketsRtpReceiver = 0;
	m_retransmittedPacketsRtpSender = 0;
	m_packetRtpSend = 0;
	m_packetRtpReceived = 0;
	m_firstPacketRtpReceivedInterval = 0;
	m_firstPacketRtpSendInterval = 0;
	m_SentBitrate = 0;
	m_SentFramerate = 0;
	m_ReceivedBitrate = 0;
	m_ReceivedFrameRate = 0;
	m_ReceivedResolution = _T("");
	m_SentResolution = _T("");
	m_ReceivedResolution_width = 0;
	m_ReceivedResolution_height = 0;
	m_SentResolution_width = 0;
	m_SentResolution_height = 0;
	m_videoPayloadeName = _T("H264");
	m_audioPayloadName = _T("G729");
	m_videoPT = 97;
	m_audioPT = 111;
	m_localSSRC = 0;
	m_remoteSSRC = 0;
	m_encryptionKey = _T("");
	m_bFullScreen = FALSE;
	m_dlgFullScreen = NULL;
	m_RTTSender = 0;
}

CserphonetestDlg::~CserphonetestDlg()
{
	if (m_dlgFullScreen)
	{
		delete m_dlgFullScreen;
	}
}

void CserphonetestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_outCallAddr);
	DDX_Text(pDX, IDC_EDIT2, m_inComingCall);
	DDX_Control(pDX, IDC_BUTTON1, m_startbutton);
	DDX_Text(pDX, IDC_EDIT3, m_playfiletoremote);
	DDX_Control(pDX, IDC_BUTTON19, m_playfilebutton);
	DDX_Text(pDX, IDC_EDIT4, m_transfercall);
	DDX_Text(pDX, IDC_RICHEDIT21, m_messagetxt);
	DDX_Text(pDX, IDC_EDIT5, m_reveiver);
	DDX_Control(pDX, IDC_AutoTest, m_btnAutoTest);
	DDX_Text(pDX, IDC_EDIT6, m_audioPayloadType);
	DDX_Text(pDX, IDC_EDIT11, m_MediaServerIP);
	DDX_Text(pDX, IDC_EDIT10, m_CameraIdx);
	DDX_Text(pDX, IDC_EDIT12, m_CameraCapIdx);
	DDX_Text(pDX, IDC_EDIT13, m_localSSRC);
	DDX_Text(pDX, IDC_EDIT16, m_remoteSSRC);
	DDX_Control(pDX, IDC_RICHEDIT21, m_richEdit21);
	DDX_Control(pDX, IDC_COMBO1, m_sipAccountCtrl);
	DDX_Control(pDX, IDC_COMBO6, m_enableP2PCtrl);
	DDX_Control(pDX, IDC_COMBO2, m_enableNACKCtrl);
	DDX_Control(pDX, IDC_COMBO3, m_enableREMBCtrl);
	DDX_Control(pDX, IDC_COMBO4, m_enableTMMBRCtrl);
	DDX_Control(pDX, IDC_RICHEDIT23, m_RichEditCtrl23);
	DDX_Text(pDX, IDC_STATIC_TOTAL_SENT_BR, m_totalSentBr);
	DDX_Text(pDX, IDC_STATIC_VIDEO_SENT_BR2, m_videoSentBr);
	DDX_Text(pDX, IDC_STATIC_FEC_SENT_BR3, m_fecSentBr);
	DDX_Text(pDX, IDC_STATIC_NACK_SENT_BR4, m_nackSentBr);
	DDX_Text(pDX, IDC_STATIC_ESTIMATIED_BR_BY_SENDER, m_estimatedBrBySender);
	DDX_Text(pDX, IDC_STATIC_ESTIMATIED_BR_BY_RECEIVER, m_estimatedBrByReceiver);
	DDX_Text(pDX, IDC_STATIC_JITTER_RTCP_RECEIVER2, m_jitterRtcpReceiver);
	DDX_Text(pDX, IDC_STATIC_FRACTION_LOST_RTCP_RECEIVER, m_fractionLostRtcpReceiver);
	DDX_Text(pDX, IDC_STATIC_CUMMUTIVE_LOST_PACKS_RTCP_RECEIVER, m_cummutiveLostPackRtcpReceiver);
	DDX_Text(pDX, IDC_STATIC_JITTER_RTCP_SEND, m_jitterRtcpSender);
	DDX_Text(pDX, IDC_STATIC_FRACTION_LOST_RTCP_SEND, m_fractionLostRtcpSender);
	DDX_Text(pDX, IDC_STATIC_CUMMUTIVE_LOST_PACKS_RTCP_SEND, m_cummutiveLostRtcpSender);
	DDX_Text(pDX, IDC_STATIC_RETRANSMITTED_PACKETS_RTP_RECEIVER, m_retransmittedPacketsRtpReceiver);
	DDX_Text(pDX, IDC_STATIC_RETRANSMITTED_PACKETS_RTP_SEND, m_retransmittedPacketsRtpSender);
	DDX_Text(pDX, IDC_STATIC_PACKETS_RTP_SEND, m_packetRtpSend);
	DDX_Text(pDX, IDC_STATIC_PACKETS_RTP_RECEIVER, m_packetRtpReceived);
	DDX_Text(pDX, IDC_STATIC_FIRST_PACKET_RTP_RECEIVER, m_firstPacketRtpReceivedInterval);
	DDX_Text(pDX, IDC_STATIC_FIRST_PACKET_RTP_SEND, m_firstPacketRtpSendInterval);
	DDX_Text(pDX, IDC_STATIC_SENT_BITRATE, m_SentBitrate);
	DDX_Text(pDX, IDC_STATIC_SENT_FRAME_RATE, m_SentFramerate);
	DDX_Text(pDX, IDC_STATIC_RECEIVED_BITRATE, m_ReceivedBitrate);
	DDX_Text(pDX, IDC_STATIC_RECEIVED_FRAME_RATE, m_ReceivedFrameRate);
	DDX_Text(pDX, IDC_STATIC_RECEIVED_RESOLUTION, m_ReceivedResolution);
	DDX_Text(pDX, IDC_STATIC_SENT_RESOLUTION, m_SentResolution);
	DDX_Text(pDX, IDC_VIDEO_PT, m_videoPT);
	DDX_Text(pDX, IDC_VIDEO_PAYLOAD_NAME, m_videoPayloadeName);
	DDX_Text(pDX, IDC_AUDIO_PAYLOAD_NAME, m_audioPayloadName);
	DDX_Text(pDX, IDC_ENCRYPTION_KEY, m_encryptionKey);
	DDX_Text(pDX, IDC_AUDIO_PT, m_audioPT);
	DDX_Control(pDX, IDC_COMBO_VIDEO_CODEC, m_videoCodecCtrl);
	DDX_Control(pDX, IDC_COMBO_VIDEO_MODE, m_videoModeCtrl);
	DDX_Control(pDX, IDC_COMBO_ENCRYPT_TYPE, m_encryptType);
	DDX_Text(pDX, IDC_STATIC_RTT_SENDER, m_RTTSender);
	DDX_Control(pDX, IDC_COMBO_VIDEO_PROTECTION_MODE, m_videoProtectionMode);
	DDX_Text(pDX, IDC_LIVE_URL, m_live_url);
	DDX_Control(pDX, IDC_VIDEO_SOURCE, m_video_source);
	DDX_Control(pDX, IDC_SHARE_WINDOW, m_share_windows);
}

BEGIN_MESSAGE_MAP(CserphonetestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CserphonetestDlg::OnBnClickedButton1)
	ON_WM_TIMER()
	ON_WM_CREATE()
	ON_MESSAGE(RECV_TEXT, OnRecvMsgTxt)
	ON_MESSAGE(UPDATE_DLG, OnUpdateDlg)
	ON_BN_CLICKED(IDOK, &CserphonetestDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON3, &CserphonetestDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON6, &CserphonetestDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON2, &CserphonetestDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON4, &CserphonetestDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CserphonetestDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON7, &CserphonetestDlg::OnBnClickedButton7)
	ON_BN_CLICKED(IDC_BUTTON8, &CserphonetestDlg::OnBnClickedButton8)
	ON_BN_CLICKED(IDC_BUTTON9, &CserphonetestDlg::OnBnClickedButton9)
	ON_BN_CLICKED(IDC_BUTTON10, &CserphonetestDlg::OnBnClickedButton10)
	ON_BN_CLICKED(IDC_BUTTON11, &CserphonetestDlg::OnBnClickedButton11)
	ON_BN_CLICKED(IDC_BUTTON12, &CserphonetestDlg::OnBnClickedButton12)
	ON_BN_CLICKED(IDC_BUTTON13, &CserphonetestDlg::OnBnClickedButton13)
	ON_BN_CLICKED(IDC_BUTTON14, &CserphonetestDlg::OnBnClickedButton14)
	ON_BN_CLICKED(IDC_BUTTON15, &CserphonetestDlg::OnBnClickedButton15)
	ON_BN_CLICKED(IDC_BUTTON16, &CserphonetestDlg::OnBnClickedButton16)
	ON_BN_CLICKED(IDC_BUTTON17, &CserphonetestDlg::OnBnClickedButton17)
	ON_BN_CLICKED(IDC_BUTTON18, &CserphonetestDlg::OnBnClickedButton18)
	ON_BN_CLICKED(IDC_BUTTON19, &CserphonetestDlg::OnBnClickedButton19)
	ON_BN_CLICKED(IDC_BUTTON20, &CserphonetestDlg::OnBnClickedButton20)
	ON_BN_CLICKED(IDC_BUTTON21, &CserphonetestDlg::OnBnClickedButton21)
	ON_BN_CLICKED(IDC_BUTTON22, &CserphonetestDlg::OnBnClickedButton22)
	ON_EN_CHANGE(IDC_RICHEDIT21, &CserphonetestDlg::OnEnChangeRichedit21)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_AutoTest, &CserphonetestDlg::OnBnClickedAutotest)
	ON_BN_CLICKED(IDC_BUTTON23, &CserphonetestDlg::OnBnClickedButton23)
//	ON_EN_CHANGE(IDC_RICHEDIT22, &CserphonetestDlg::OnEnChangeRichedit22)
ON_BN_CLICKED(IDC_BUTTON24, &CserphonetestDlg::OnBnClickedButton24)
ON_BN_CLICKED(IDC_BUTTON25, &CserphonetestDlg::OnBnClickedButton25)
ON_BN_CLICKED(IDC_BUTTON26, &CserphonetestDlg::OnBnClickedButton26)
ON_BN_CLICKED(IDC_BUTTON27, &CserphonetestDlg::OnBnClickedButton27)
ON_BN_CLICKED(IDC_BUTTON28, &CserphonetestDlg::OnBnClickedButton28)
ON_BN_CLICKED(IDC_BUTTON29, &CserphonetestDlg::OnBnClickedButton29)
ON_WM_LBUTTONDBLCLK()
ON_NOTIFY(EN_MSGFILTER, IDC_RICHEDIT21, &CserphonetestDlg::OnEnMsgfilterRichedit21)
ON_CBN_SELCHANGE(IDC_VIDEO_SOURCE, &CserphonetestDlg::OnCbnSelchangeVideoSource)
ON_BN_CLICKED(IDC_PLAY_STREAM, &CserphonetestDlg::OnBnClickedPlayStream)
ON_BN_CLICKED(IDC_STOP_LIVE, &CserphonetestDlg::OnBnClickedStopLive)
ON_EN_CHANGE(IDC_LIVE_URL, &CserphonetestDlg::OnEnChangeLiveUrl)

ON_BN_CLICKED(IDC_PUSH_STREAM, &CserphonetestDlg::OnBnClickedPushStream)
ON_CBN_SELCHANGE(IDC_SHARE_WINDOW, &CserphonetestDlg::OnCbnSelchangeShareWindow)
ON_CBN_DROPDOWN(IDC_SHARE_WINDOW, &CserphonetestDlg::OnCbnDropdownShareWindow)
ON_EN_CHANGE(IDC_EDIT10, &CserphonetestDlg::OnEnChangeEdit10)
ON_EN_CHANGE(IDC_EDIT12, &CserphonetestDlg::OnEnChangeEdit12)
ON_EN_CHANGE(IDC_EDIT11, &CserphonetestDlg::OnEnChangeEdit11)
ON_EN_CHANGE(IDC_AUDIO_PAYLOAD_NAME, &CserphonetestDlg::OnEnChangeAudioPayloadName)
ON_EN_CHANGE(IDC_AUDIO_PT, &CserphonetestDlg::OnEnChangeAudioPt)
ON_EN_CHANGE(IDC_VIDEO_PAYLOAD_NAME, &CserphonetestDlg::OnEnChangeVideoPayloadName)
ON_EN_CHANGE(IDC_EDIT1, &CserphonetestDlg::OnEnChangeEdit1)
ON_EN_CHANGE(IDC_EDIT2, &CserphonetestDlg::OnEnChangeEdit2)
ON_EN_CHANGE(IDC_EDIT3, &CserphonetestDlg::OnEnChangeEdit3)
ON_EN_CHANGE(IDC_EDIT4, &CserphonetestDlg::OnEnChangeEdit4)
ON_EN_CHANGE(IDC_ENCRYPTION_KEY, &CserphonetestDlg::OnEnChangeEncryptionKey)
ON_CBN_SELCHANGE(IDC_COMBO_VIDEO_CODEC, &CserphonetestDlg::OnCbnSelchangeComboVideoCodec)
ON_CBN_SELCHANGE(IDC_COMBO_VIDEO_MODE, &CserphonetestDlg::OnCbnSelchangeComboVideoMode)
ON_CBN_SELCHANGE(IDC_COMBO1, &CserphonetestDlg::OnCbnSelchangeCombo1)
ON_CBN_SELCHANGE(IDC_COMBO_ENCRYPT_TYPE, &CserphonetestDlg::OnCbnSelchangeComboEncryptType)
ON_EN_CHANGE(IDC_EDIT13, &CserphonetestDlg::OnEnChangeEdit13)
ON_EN_CHANGE(IDC_VIDEO_PT, &CserphonetestDlg::OnEnChangeVideoPt)
ON_EN_CHANGE(IDC_EDIT16, &CserphonetestDlg::OnEnChangeEdit16)
ON_BN_CLICKED(IDC_BUTTON30, &CserphonetestDlg::OnBnClickedButton30)
END_MESSAGE_MAP()


// CserphonetestDlg 消息处理程序

BOOL CserphonetestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	// TODO: 在此添加额外的初始化代码
	g_dlg = this;

	CCallbackInterface callback;
	memset(&callback,0,sizeof(CCallbackInterface));
	callback.onIncomingCallReceived = onIncomingCallReceived;
	callback.onCallAnswered =onCallAnswered;
	callback.onCallAlerting =onCallAlerting;
	callback.onConnected = onConnected;
	callback.onConnectError = onConnectError;
	callback.onMakeCallFailed = onMakeCallFailed;
	callback.onCallReleased = onCallReleased;
	callback.onCallPaused = onCallPaused;
	callback.onCallPausedByRemote = onCallPausedByRemote;
	callback.onLogInfo = onLogInfo;
	callback.onGetCapabilityToken = onGetCapabilityToken;
	callback.onTextMessageReceived = onTextMessageReceived;
	callback.onReceiverStats = onReceiverStats;
	callback.onIncomingCodecChanged = onIncomingCodecChanged;
	callback.onRemoteVideoRatioChanged = onRemoteVideoRatioChanged;
	//	callback.onGroupTextMessageReceived = onGroupTextMessageReceived;
	setLogLevel(LOG_LEVEL_DEBUG);
	setTraceFlag(true);
	initialize( &callback);
	m_enableP2PCtrl.AddString(_T("Yes"));
	m_enableP2PCtrl.AddString(_T("No"));
	m_enableP2PCtrl.SetCurSel(0);

	m_enableNACKCtrl.AddString(_T("Yes"));
	m_enableNACKCtrl.AddString(_T("No"));
	m_enableNACKCtrl.SetCurSel(0);

	m_enableREMBCtrl.AddString(_T("Yes"));
	m_enableREMBCtrl.AddString(_T("No"));
	m_enableREMBCtrl.SetCurSel(0);

	m_enableTMMBRCtrl.AddString(_T("Yes"));
	m_enableTMMBRCtrl.AddString(_T("No"));
	m_enableTMMBRCtrl.SetCurSel(1);

	m_videoCodecCtrl.AddString(_T("H264"));
	m_videoCodecCtrl.AddString(_T("VP8"));
	m_videoCodecCtrl.AddString(_T("ALL"));
	m_videoCodecCtrl.SetCurSel(2);

	m_videoModeCtrl.AddString(_T("real-time"));
	m_videoModeCtrl.AddString(_T("screen-share"));
	m_videoModeCtrl.SetCurSel(0);

	m_videoProtectionMode.AddString(_T("NACK"));
	m_videoProtectionMode.AddString(_T("FEC"));
	m_videoProtectionMode.AddString(_T("Hybrid"));
	m_videoProtectionMode.SetCurSel(0);

	m_video_source.AddString(_T("摄像头"));
	m_video_source.AddString(_T("桌面"));
	m_video_source.SetCurSel(0);

	m_encryptType.AddString(_T(""));
	m_encryptType.AddString(_T("CCPAES_128_SHA1_80"));
	m_encryptType.AddString(_T("CCPAES_128_SHA1_32"));
	m_encryptType.AddString(_T("CCPAES_256_SHA1_80"));
	m_encryptType.AddString(_T("CCPAES_256_SHA1_32"));
	m_encryptType.AddString(_T("CCPAES_128_NO_AUTH"));
	m_encryptType.AddString(_T("CCPNO_CIPHER_SHA1_80"));
	m_encryptType.SetCurSel(0);

	CRect rectNormal;
	CRect rectExpand;
	GetWindowRect(&rectExpand);
	m_nExpandedWidth=rectExpand.Width();

	GetDlgItem(IDC_SEPARATOR)->GetWindowRect(&rectNormal);
	m_nNormalWidth=rectNormal.right-rectExpand.left;

	rectExpand.SetRect(rectExpand.left,rectExpand.top,rectExpand.left+m_nNormalWidth, rectExpand.bottom);
	MoveWindow(&rectExpand,TRUE);

	DWORD num = GetPrivateProfileStringA("ACCOUNT", "mediaServerAddr","", serverIP, sizeof(serverIP), ".\\account.ini");
	num = GetPrivateProfileStringA("ACCOUNT", "mediaServerPort","", serverPort, sizeof(serverPort), ".\\account.ini");
	num = GetPrivateProfileStringA("ACCOUNT", "sipAcount1","", sipAcount1, sizeof(sipAcount1), ".\\account.ini");
	num = GetPrivateProfileStringA("ACCOUNT", "sipAcount1Password","", sipAcount1Password, sizeof(sipAcount1Password), ".\\account.ini");
	num = GetPrivateProfileStringA("ACCOUNT", "sipAcount2","", sipAcount2, sizeof(sipAcount2), ".\\account.ini");
	num = GetPrivateProfileStringA("ACCOUNT", "sipAcount2Password","", sipAcount2Password, sizeof(sipAcount2Password), ".\\account.ini");

	num =GetPrivateProfileStringA("DESKTOP_SHARE", "desktop_width","", m_desktop_width, sizeof(m_desktop_width), ".\\account.ini");
	num =GetPrivateProfileStringA("DESKTOP_SHARE", "desktop_height","", m_desktop_height, sizeof(m_desktop_height), ".\\account.ini");
	num =GetPrivateProfileStringA("DESKTOP_SHARE", "desktop_frame_rate","", m_desktop_frame_rate, sizeof(m_desktop_frame_rate), ".\\account.ini");
	num =GetPrivateProfileStringA("DESKTOP_SHARE", "desktop_bit_rate","", m_desktop_bit_rate, sizeof(m_desktop_bit_rate), ".\\account.ini");

	USES_CONVERSION;
	m_MediaServerIP = A2W(serverIP);
	m_outCallAddr = A2W(sipAcount2);
	m_live_url = "rtmp://live.yuntongxun.com/live/livestream";
	m_sipAccountCtrl.AddString(A2W(sipAcount1));
	m_sipAccountCtrl.AddString(A2W(sipAcount2));
	m_sipAccountCtrl.SetCurSel(0);

	if (m_dlgFullScreen == NULL)
	{
		CRect   temprect(0,0,1440,900);
		m_dlgFullScreen = new CDlgFullScreen;
		m_dlgFullScreen->Create(IDD_DIALOG_FULL_SCREEN, this);
		m_dlgFullScreen->MoveWindow(&temprect, TRUE);
	}

	UpdateData(FALSE);
	
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CserphonetestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CserphonetestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CserphonetestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CserphonetestDlg::OnTimer(UINT_PTR nIDEven)
{
	if(g_bAutoTest)
		OnBnClickedButton2();
	//sendKeepAlive();

	switch(nIDEven)
	{
	case TIMER_STATISTICS:
		const char *callid = getCurrentCall();	
		if (callid)
		{
			_RtcpStatistics receiverRtcpStats;
			_RtcpStatistics senderRtcpStas;
			_StreamDataCounters sent;
			_StreamDataCounters received;
			__int64 rtt;

			m_totalSentBr = 0;
			m_videoSentBr = 0;
			m_fecSentBr = 0;
			m_nackSentBr = 0;

			m_estimatedBrBySender = 0;
			m_estimatedBrByReceiver = 0;

			m_cummutiveLostPackRtcpReceiver = 0;
			m_fractionLostRtcpReceiver = 0;
			m_jitterRtcpReceiver = 0;
			m_cummutiveLostRtcpSender = 0;
			m_fractionLostRtcpSender = 0;
			m_jitterRtcpReceiver = 0;

			m_SentBitrate = 0;
			m_SentFramerate = 0;

			GetBandwidthUsage(callid, m_totalSentBr, m_videoSentBr, m_fecSentBr, m_nackSentBr);
			GetEstimatedSendBandwidth(callid, &m_estimatedBrBySender);
			GetEstimatedReceiveBandwidth(callid, &m_estimatedBrByReceiver);
			GetReceiveChannelRtcpStatistics(callid, receiverRtcpStats, rtt);
			GetSendChannelRtcpStatistics(callid, senderRtcpStas, m_RTTSender);
			GetRtpStatistics(callid, sent, received);
//			GetSendStats(callid, m_SentFramerate, m_SentBitrate, m_SentResolution_width, m_SentResolution_height, suspend);

			m_totalSentBr = m_totalSentBr/1000;
			m_videoSentBr = m_videoSentBr/1000;
			m_fecSentBr = m_fecSentBr/1000;
			m_nackSentBr = m_nackSentBr/1000;
			m_estimatedBrByReceiver = m_estimatedBrByReceiver/1000; //kb/s
			m_estimatedBrBySender = m_estimatedBrBySender/1000;


			m_cummutiveLostPackRtcpReceiver = receiverRtcpStats.cumulative_lost;
			m_fractionLostRtcpReceiver = receiverRtcpStats.fraction_lost;
			m_jitterRtcpReceiver = (receiverRtcpStats.jitter+89)/90;

			m_cummutiveLostRtcpSender = senderRtcpStas.cumulative_lost;
			m_fractionLostRtcpSender = senderRtcpStas.fraction_lost;
			m_jitterRtcpSender = (senderRtcpStas.jitter+89)/90; //unit ms, video is 90kHz, jitter here is computed by rtp_ts

			m_packetRtpReceived = received.packets;
			m_retransmittedPacketsRtpReceiver = received.retransmitted_packets;
			m_firstPacketRtpReceivedInterval = received.first_packet_time_ms;

			m_packetRtpSend = sent.packets;
			m_retransmittedPacketsRtpSender = sent.retransmitted_packets;
			m_firstPacketRtpSendInterval = sent.first_packet_time_ms;

			m_SentBitrate /= 1000;

			CString resolution;
			resolution.Format(_T("%dx%d"), m_SentResolution_width, m_SentResolution_height);
			m_SentResolution = resolution;

			resolution.Format(_T("%dx%d"), m_ReceivedResolution_width, m_ReceivedResolution_height);
			m_ReceivedResolution = resolution;

		    UpdateData(FALSE);
		}
		break;
	}
}

void CserphonetestDlg::OnBnClickedButton1()
{
	USES_CONVERSION; 
	UpdateData(TRUE);
	int index = m_enableP2PCtrl.GetCurSel();
	if (index == 0)
	{
		setP2PEnabled(true);
	}else if (index ==1)
	{
		setP2PEnabled(false);
	}

	index = m_enableNACKCtrl.GetCurSel();
	if (index == 0)
	{
		setNackEnabled(true,true);
	}else if (index ==1)
	{
		setNackEnabled(true, false);
	}

	index = m_videoProtectionMode.GetCurSel();
	setVideoProtectionMode(index); //0:nack	1:fec	2:hybrid

	index = m_enableREMBCtrl.GetCurSel();
	if (index == 0)
	{
		setRembEnabled(true);
	}else if (index ==1)
	{
		setRembEnabled(false);
	}

	index = m_enableTMMBRCtrl.GetCurSel();
	if (index == 0)
	{
		setTmmbrEnabled(true);
	}else if (index ==1)
	{
		setTmmbrEnabled(false);
	}

	index = m_videoCodecCtrl.GetCurSel();
	if(index == 0) {
		setCodecEnabled(codec_VP8,  false);
		setCodecEnabled(codec_H264,  true);
	}
	else if(index == 1) {
		setCodecEnabled(codec_VP8,  true);
		setCodecEnabled(codec_H264,  false);
	} else {
		setCodecEnabled(codec_VP8,  true);
		setCodecEnabled(codec_H264,  true);
	}

	index = m_videoModeCtrl.GetCurSel();
	setVideoMode(index);
	setDesktopShareParam(atoi(m_desktop_width), atoi(m_desktop_height), atoi(m_desktop_frame_rate), atoi(m_desktop_bit_rate));


	setRing("ring.wav");
	setRingback("ring.wav");
	
	CWnd *rcwnd = g_dlg->GetDlgItem(IDC_RICHEDIT21); //返回窗口中指定参数ID的子元素的句柄
	CWnd *lcwnd = g_dlg->GetDlgItem(IDC_RICHEDIT22);

    if (index == 1)//screen share
	{
		rcwnd = m_dlgFullScreen;
	}

	if (m_dlgFullScreen == NULL)
	{
		CRect   temprect(0, 0, 1440, 900);
		m_dlgFullScreen = new CDlgFullScreen;
		m_dlgFullScreen->Create(IDD_DIALOG_FULL_SCREEN, this);
		m_dlgFullScreen->MoveWindow(&temprect, TRUE);
	}
	g_dlgFullScreen = m_dlgFullScreen;
	setVideoView(g_dlgFullScreen->GetSafeHwnd(), lcwnd->GetSafeHwnd()); //得到一个窗口对象（CWnd的派生对象）指针的句柄（HWND）
	//setVideoView(rcwnd->GetSafeHwnd(),  lcwnd->GetSafeHwnd()); //得到一个窗口对象（CWnd的派生对象）指针的句柄（HWND）
	int cameraIdx = _ttoi(m_CameraIdx);
	int cameraCapIdx = _ttoi(m_CameraCapIdx);
	int rotate = 0;
	bool force = false;

	CameraInfo* cameraInfo = NULL;
	getCameraInfo(&cameraInfo);
	selectCamera(cameraIdx, cameraCapIdx, 15, rotate, force);

	int sipIdx = m_sipAccountCtrl.GetCurSel();

	if (sipIdx == 0)
	{
		m_sipID = A2W(sipAcount1);
		m_sipPassword = A2W(sipAcount1Password);
	}else if (sipIdx == 1)
	{
		m_sipID = A2W(sipAcount2);
		m_sipPassword = A2W(sipAcount2Password);
	}

	g_dlg->m_sipAccount = m_sipID;
	g_dlg->m_sipAccount += "@";
	g_dlg->m_sipAccount += m_MediaServerIP;

	int ret = connectToCCP(W2A((LPCTSTR)m_MediaServerIP), atoi(serverPort),W2A((LPCTSTR)m_sipID), W2A((LPCTSTR)m_sipPassword),"eyJsb2NhbHJlYyI6IjEifQ==");
	setUserData(USERDATA_FOR_INVITE,"phone=15810763885");

	if (ret < 0) {
		g_dlg->SetDlgItemText(IDC_REGISTER_STATE, L"  注册失败，参数不正确");
		return;
	}
		m_startbutton.EnableWindow(false);

	//setLocalSSRC(m_localSSRC);
//	SetTimer(TIMER_STATISTICS, 1000, 0);
}


int CserphonetestDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;
	srand(time_t(NULL));
	// TODO:  在此添加您专用的创建代码
	//this->SetTimer(1,1000,0);

	return 0;
}
void CserphonetestDlg::OnBnClickedOk()
{
	if (m_dlgFullScreen)
		delete m_dlgFullScreen;
	m_dlgFullScreen = NULL;

	unInitialize();
//	KillTimer(TIMER_STATISTICS);
	CDialogEx::OnOK();
}


void CserphonetestDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	//acceptCall(g_currentCallId);
	acceptCallByMediaType(g_currentCallId, 1);
}


void CserphonetestDlg::OnBnClickedButton6()
{
	// TODO: 在此添加控件通知处理程序代码
	//rejectCall(g_currentCallId, ReasonDeclined);
	releaseCall(g_currentCallId, 0);
	//m_inComingCall = "";
	//m_outCallAddr = "";
	p_CurrentCall = NULL;
	this->UpdateData( false);
}


void CserphonetestDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	this->UpdateData( true);
	int len = WideCharToMultiByte(CP_ACP,0,m_outCallAddr,m_outCallAddr.GetLength(),
		NULL,0,NULL,NULL);
	char *p_url = new char[len+1];
	WideCharToMultiByte(CP_ACP,0,m_outCallAddr,m_outCallAddr.GetLength(),
		p_url,len,NULL,NULL);
	p_url[len]='\0';
	CWnd *cwnd = g_dlg->GetDlgItem(IDC_RICHEDIT21);
	if(!strcmp(p_url,""))
		return;
	setKeepAliveTimeout(1000,1000);
	
	//LpConfig * config = lp_config_new("phone.ini");
	const char * phone = "" ; //lp_config_get_string(config,"phone","phone","");
	const char * user = "" ;  //lp_config_get_string(config,"phone","user","1015");
	
	char buf[256];
	
	if( !strcmp(phone,"") ){
		sprintf(buf,"tel=%s;nickname=test",user);
	}
	else
		sprintf(buf,"tel=%s;nickname=tst",phone);

	setUserData(USERDATA_FOR_INVITE,buf);
	const char *callid = makeCall(1,p_url);
	if( callid) {
		strncpy(g_currentCallId,callid,sizeof(g_currentCallId));
	}
	delete p_url;
	g_statisticInfo.outgoingCount++;
	g_statisticInfo.lastCallOutTime = time(NULL);

	g_dlg->SetDlgItemText(IDC_CALL_STATE,L"呼出中....") ;
}


void CserphonetestDlg::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码
	pauseCall(g_currentCallId);
}

void CserphonetestDlg::OnBnClickedButton5()
{
	// TODO: 在此添加控件通知处理程序代码
		resumeCall(g_currentCallId);
}

void CserphonetestDlg::OnBnClickedButton7()
{
	//setMute(true);

	// TODO: 在此添加控件通知处理程序代码
	//sendDTMF(g_currentCallId,'1');
	//setMute(true);
	cancelTmmbr(g_currentCallId);

	//SpeakerInfo *speak;
	//getSpeakerInfo(&speak);

	//const char * filen = "./123.wav";
	//startRecordVoice(g_currentCallId, filen);

	//const char *mp4File = "./callRecord.mp4";
	//char mp4File_UTF8[128]={0};
	//ANSIToUTF8((char*)mp4File, mp4File_UTF8);
	//startRecordVoip(g_currentCallId, mp4File_UTF8);

	//SpeakerInfo *speakerInfo;
	//int speakerCount = getSpeakerInfo(&speakerInfo);
	//for(int i=0; i<speakerCount; i++)
	//{
	//	char log[256];
	//	sprintf(log,"hubintest speakser=%d name=%s guid=%s\r\n\r\n", speakerInfo[i].index, speakerInfo[i].name, speakerInfo[i].guid);
	//	wchar_t *wLog = TransformUTF8ToUnicodeM(log);
	//	wprintf(wLog);
	//	//printf("hubintest speakser=%d name=%s guid=%s\r\n\r\n", microphone[i].index, microphone[i].name, microphone[i].guid);
	//}

	//checkUserOnline("801755000");//00006");
	//long long duration,  sendTotal_sim,  recvTotal_sim, sendTotal_wifi, recvTotal_wifi;

	//int ret = getNetworkStatistic(g_currentCallId, &duration, &sendTotal_sim, &recvTotal_sim, &sendTotal_wifi, &recvTotal_wifi);

	//char log[256];
	//sprintf(log,"hubintest getNetworkStatistic=%d duration=%d sendTotal=%d recfTotal=%d\r\n\r\n", ret, duration, sendTotal_sim+sendTotal_wifi, recvTotal_sim+recvTotal_wifi);
	//wchar_t *wLog = TransformUTF8ToUnicodeM(log);
	//wprintf(wLog);



	//setSrtpEnabled(false, true, true, 1, "12345678901234567890123456789012345678901234");
}

void CserphonetestDlg::OnBnClickedButton8()
{
	// TODO: 在此添加控件通知处理程序代码
	//sendDTMF(g_currentCallId, '2');
	//setMute(false);
	//VideoStartReceive(g_currentCallId);
	requestVideo(g_currentCallId, 320, 240);
	//stopRecordVoice(g_currentCallId);
	//setMute(false);
	//stopRecordVoip(g_currentCallId);



	//setSrtpEnabled(false, true, true, 2, "12345678901234567890123456789012345678901234");
}

_declspec(dllimport) void  _stdcall PrintConsole(const char * fmt, ...);
void CserphonetestDlg::OnBnClickedButton9()
{
	// TODO: 在此添加控件通知处理程序代码
	//sendDTMF(g_currentCallId,'3');
	//VideoStopReceive(g_currentCallId);
	requestVideo(g_currentCallId, 640, 480);
	//startRecordVoice(g_currentCallId, "audio_record.wav");
	//setSrtpEnabled(false, true, true, 3, "12345678901234567890123456789012345678901234");

}


void CserphonetestDlg::OnBnClickedButton10()
{
	// TODO: 在此添加控件通知处理程序代码
	sendDTMF(g_currentCallId,'4');
	//stopRecordVoice(g_currentCallId);
	setSrtpEnabled(false, true, true, 4, "12345678901234567890123456789012345678901234");
}


void CserphonetestDlg::OnBnClickedButton11()
{
	// TODO: 在此添加控件通知处理程序代码
	sendDTMF(g_currentCallId,'5');
	//unsigned char *jpgBuf = NULL;
	//unsigned int jpgBufSize = 0;
	//unsigned int width, height;
	//if (getLocalVideoSnapshot(g_currentCallId, &jpgBuf, &jpgBufSize, &width, &height) == 0) {
	//	FILE *jpeg = fopen("L:\\local.jpg", "wb");
	//	if (jpeg) {
	//		fwrite(jpgBuf, 1, jpgBufSize, jpeg);
	//		fclose(jpeg);
	//	}
	//	void* aa = malloc(1025);
	//	memset(aa, 0xFF, 1025);
	//}
	setSrtpEnabled(false, true, true, 5, "12345678901234567890123456789012345678901234");
	}


void CserphonetestDlg::OnBnClickedButton12()
{
	// TODO: 在此添加控件通知处理程序代码
	sendDTMF(g_currentCallId, '6');


	//unsigned char *jpgBuf = NULL;
	//unsigned int jpgBufSize = 0;
	//unsigned int width, height;
	//if (getRemoteVideoSnapshot(g_currentCallId, &jpgBuf, &jpgBufSize, &width, &height) == 0) {
	//	FILE *jpeg = fopen("L:\\remote.jpg", "wb");
	//	if (jpeg) {
	//		fwrite(jpgBuf, 1, jpgBufSize, jpeg);
	//		fclose(jpeg);
	//	}
	//}
	setSrtpEnabled(false, true, true, 6, "12345678901234567890123456789012345678901234");

}

void CserphonetestDlg::OnBnClickedButton13()
{
	// TODO: 在此添加控件通知处理程序代码
	sendDTMF(g_currentCallId,'7');
	setSrtpEnabled(false, true, true, 1, "1234567890");
}

void CserphonetestDlg::OnBnClickedButton14()
{
	// TODO: 在此添加控件通知处理程序代码
	sendDTMF(g_currentCallId, '8');
	setSrtpEnabled(false, true, true, 4, "1234567890");
}

void CserphonetestDlg::OnBnClickedButton15()
{

	// TODO: 在此添加控件通知处理程序代码
	sendDTMF(g_currentCallId,'9');
}


void CserphonetestDlg::OnBnClickedButton16()
{
	// TODO: 在此添加控件通知处理程序代码
	sendDTMF(g_currentCallId,'*');
	static int index = 0;
	index++;
	char filename[256];
	sprintf(filename, "./RecordFile_%d.mp4", index);
	startRecordScreen(g_currentCallId, filename, 2000, 10, 0);
}


void CserphonetestDlg::OnBnClickedButton17()
{
	// TODO: 在此添加控件通知处理程序代码
	sendDTMF(g_currentCallId,'0');
	stopRecordScreen(g_currentCallId);
}


void CserphonetestDlg::OnBnClickedButton18()
{
	// TODO: 在此添加控件通知处理程序代码
	//sendDTMF(g_currentCallId,'#');
	CString str; 
	GetDlgItem(IDC_BUTTON18)->GetWindowTextW(str);
	if (str == "#")
	{
		GetDlgItem(IDC_BUTTON18)->SetWindowTextW(L"##");
		startRtpDump(g_currentCallId, 0, "audio_in.dump", 0);
		startRtpDump(g_currentCallId, 0, "audio_out.dump", 1);
		startRtpDump(g_currentCallId, 1, "video_in.dump", 0);
		startRtpDump(g_currentCallId, 1, "video_out.dump", 1);
	}
	else
	{
		GetDlgItem(IDC_BUTTON18)->SetWindowTextW(L"#");
		stopRtpDump(g_currentCallId, 0, 0);
		stopRtpDump(g_currentCallId, 0, 1);
		stopRtpDump(g_currentCallId, 1, 0);
		stopRtpDump(g_currentCallId, 1, 1);
	}

}


void CserphonetestDlg::OnBnClickedButton19()
{
	// TODO: 在此添加控件通知处理程序代码
	this->UpdateData(true );
	int len = WideCharToMultiByte(CP_ACP,0,m_playfiletoremote,m_playfiletoremote.GetLength(),
		NULL,0,NULL,NULL);
	char *p_filename = new char[len+1];
	WideCharToMultiByte(CP_ACP,0,m_playfiletoremote,m_playfiletoremote.GetLength(),
		p_filename,len,NULL,NULL);
	p_filename[len]='\0';

//	if ( p_service )
		//p_service->serphone_core_playfile_to_remote( p_CurrentCall, p_filename);
	delete p_filename;
	
}


void CserphonetestDlg::OnBnClickedButton20()
{
	// TODO: 在此添加控件通知处理程序代码
	this->UpdateData(true );
	int len = WideCharToMultiByte(CP_ACP,0,m_transfercall,m_transfercall.GetLength(),
		NULL,0,NULL,NULL);
	char *p_refer = new char[len+1];
	WideCharToMultiByte(CP_ACP,0,m_transfercall,m_transfercall.GetLength(),
		p_refer,len,NULL,NULL);
	p_refer[len]='\0';
	transferCall(g_currentCallId,p_refer);
	delete p_refer;
}


void CserphonetestDlg::OnBnClickedButton21()
{
	// TODO: 在此添加控件通知处理程序代码
//	if ( p_service )
//		p_service->serphone_core_stop_playfile_to_remote( p_CurrentCall);
}


void CserphonetestDlg::OnBnClickedButton22()
{
	// TODO: 在此添加控件通知处理程序代码
	//发送message
	this->UpdateData(true );
	int len = WideCharToMultiByte(CP_ACP,0,m_reveiver,m_reveiver.GetLength(),
		NULL,0,NULL,NULL);
	char *p_to = new char[len+1];
	WideCharToMultiByte(CP_ACP,0,m_reveiver,m_reveiver.GetLength(),
		p_to,len,NULL,NULL);
	p_to[len]='\0';
//////////////////to end ///////////
	len = WideCharToMultiByte(CP_ACP,0,m_messagetxt,m_messagetxt.GetLength(),
		NULL,0,NULL,NULL);
	char *p_msg = new char[len+1];
	WideCharToMultiByte(CP_ACP,0,m_messagetxt,m_messagetxt.GetLength(),
		p_msg,len,NULL,NULL);
	p_msg[len]='\0';

	char msg[4192]={0};
	memset(msg,'c',3500);
	sendTextMessage(p_to,p_msg, NULL);
	delete p_to;
	delete p_msg;
	m_reveiver = "";
	m_messagetxt="";
}
LRESULT CserphonetestDlg::OnUpdateDlg(WPARAM wParam, LPARAM lParam)
{
	g_dlg->UpdateData(false);
	return 0;
}

LRESULT CserphonetestDlg::OnRecvMsgTxt(WPARAM wParam, LPARAM lParam)
{
	char *msg = (char *)lParam;
	m_messagetxt = msg;
	this->UpdateData(false);
	delete msg;
	return 0;
}


void CserphonetestDlg::OnEnChangeRichedit21()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}


void CserphonetestDlg::OnClose()
{
	if (g_rtmpLiveStreamHandle) {
		stopLiveStream(g_rtmpLiveStreamHandle);
	}
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	unInitialize();
	CDialogEx::OnClose();
}


void CserphonetestDlg::OnBnClickedAutotest()
{
	// TODO: 在此添加控件通知处理程序代码

	MediaStatisticsInfo stats;
	getCallStatistics(0,&stats);
		char buf[4096];
		int i =sprintf(buf, "byte received %d\n",stats.extendedMax);
		i += sprintf(buf+i, "byte send %d\n",stats.bytesSent);
		i += sprintf(buf+i, "cumulativeLost %d\n",stats.cumulativeLost);
		i += sprintf(buf+i, "extendedMax %d\n",stats.extendedMax);
		i += sprintf(buf+i, "fractionLost %d\n",stats.fractionLost);
		i += sprintf(buf+i, "jitterSamples %d\n",stats.jitterSamples);
		i += sprintf(buf+i, "packetsReceived %d\n",stats.packetsReceived);
		i += sprintf(buf+i, "packetsSent %d\n",stats.packetsSent);
		i += sprintf(buf+i, "rttMs %d\n",stats.rttMs);
		onLogInfo(buf);
	
	/*char message[2049] ={0};
	memset(message,'c',2048);
	for( int i =0 ; i <2 ; i++ )
		sendTextMessage("80000200000036",message);

	g_bAutoTest = !g_bAutoTest;
	if( g_bAutoTest) {
		m_btnAutoTest.SetWindowText(L"停止测试");
		memset(&g_statisticInfo,0,sizeof(TStatisticInfo));
		OnBnClickedButton2();
	}
	else {
		m_btnAutoTest.SetWindowText(L"自动测试");
		char buf[4096];
		int i =sprintf(buf, "Incoming call %d\n",g_statisticInfo.incomingCount);
		i += sprintf(buf+i, "Outgoing call %d\n",g_statisticInfo.outgoingCount);
		i += sprintf(buf+i, "Alerting call %d\n",g_statisticInfo.altertingCount);
		i += sprintf(buf+i, "Avg Altering Time %d\n",g_statisticInfo.outAlteringTime/g_statisticInfo.outgoingCount);
		onLogInfo(buf);
	}*/
}


void CserphonetestDlg::OnBnClickedButton23()
{
	// TODO: 在此添加控件通知处理程序代码
	 bool muted = getMuteStatus();
	
	muted = !muted;
	setMute(muted);

	if(muted)
		SetDlgItemText(IDC_BUTTON23,L"开麦"); 
	else
		SetDlgItemText(IDC_BUTTON23,L"静音");

}


//void CserphonetestDlg::OnEnChangeRichedit22()
//{
//	// TODO:  如果该控件是 RICHEDIT 控件，它将不
//	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
//	// 函数并调用 CRichEditCtrl().SetEventMask()，
//	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。
//
//	// TODO:  在此添加控件通知处理程序代码
//}


void CserphonetestDlg::OnBnClickedButton25()
{
	USES_CONVERSION;
	// TODO: 在此添加控件通知处理程序代码
	this->UpdateData(TRUE);
	int localPort = 9078;
	const char* ptName = "";
	int payloadType = m_videoPT;
	CWnd *lcwnd = g_dlg->GetDlgItem(IDC_RICHEDIT21);

	//if (m_dlgFullScreen == NULL)
	//{
	//	CRect   temprect(0,0,1200,750);
	//	//CRect   temprect(0,0,1440,900);
	//	m_dlgFullScreen = new CDlgFullScreen;
	//	m_dlgFullScreen->Create(IDD_DIALOG_FULL_SCREEN, this);
	//	m_dlgFullScreen->MoveWindow(&temprect, TRUE);

	//}
	//m_dlgFullScreen->ShowWindow(SW_SHOW);

	if (!strcmp(W2A(m_videoPayloadeName.GetBuffer(0)), "H264")||!strcmp(W2A(m_videoPayloadeName.GetBuffer(0)), "h264"))
	{
		ptName = "H264";
	}else if (!strcmp(W2A(m_videoPayloadeName.GetBuffer(0)), "VP8")||!strcmp(W2A(m_videoPayloadeName.GetBuffer(0)), "vp8"))
	{
		ptName = "VP8";
	}

	int index = m_encryptType.GetCurSel();
	//int ret = PlayVideoFromRtpDump(localPort, ptName, payloadType, /*m_dlgFullScreen->GetSafeHwnd()*/lcwnd->GetSafeHwnd(), index, W2A(m_encryptionKey.GetBuffer(0)));
	//int ret = PlayVideoFromRtpDump(localPort, "VP8", 120, lcwnd->GetSafeHwnd(), index, W2A(m_encryptionKey.GetBuffer(0)));
	int ret = PlayVideoFromRtpDump(localPort, "VP8", 120, m_dlgFullScreen->GetSafeHwnd(), index, W2A(m_encryptionKey.GetBuffer(0)));


}


void CserphonetestDlg::OnBnClickedButton26()
{
	// TODO: 在此添加控件通知处理程序代码
	USES_CONVERSION;
	this->UpdateData(TRUE);
	int localPort = 7078;
	char ptName[5];
	int payloadType = m_audioPT;
	memset(ptName, 0, 5);
	memcpy(ptName, W2A(m_audioPayloadName.GetBuffer(0)), 4*sizeof(char));
	CWnd *lcwnd = g_dlg->GetDlgItem(IDC_RICHEDIT21);

	int index = m_encryptType.GetCurSel();
	int ret = PlayAudioFromRtpDump(localPort, ptName, payloadType, index, W2A(m_encryptionKey.GetBuffer(0)));
}


void CserphonetestDlg::OnBnClickedButton27()
{
	// TODO: 在此添加控件通知处理程序代码
	//AfxMessageBox(_T("get camera info"), MB_OK, 0);
	CameraInfo *cameraInfo=NULL;
	int cameraNum = getCameraInfo(&cameraInfo);
	
	USES_CONVERSION;
	for (int cameraIdx=0; cameraIdx<cameraNum; cameraIdx++)
	{
		//name为UTF8，需转化为unicode编码进行显示
		DWORD nWszLen = MultiByteToWideChar(CP_UTF8, NULL, cameraInfo[cameraIdx].name, -1, NULL, NULL);
		CStringW strUTF16;
		nWszLen = MultiByteToWideChar(CP_UTF8, NULL, cameraInfo[cameraIdx].name, -1, strUTF16.GetBuffer(nWszLen), nWszLen);
		strUTF16.ReleaseBuffer();

		CString info;
		info.Format(_T("camera[%d]: %s\r\n"), cameraIdx, strUTF16); 
		m_RichEditCtrl23.SetSel(-1,-1);
		m_RichEditCtrl23.ReplaceSel(info);
		for(int i=0; i<cameraInfo[cameraIdx].capabilityCount; i++)
		{
			info.Format( A2W("\tcapability[%2d]: %4d x %4d  @ %d maxFPS\r\n"), i, \
				cameraInfo[cameraIdx].capability[i].width, cameraInfo[cameraIdx].capability[i].height, cameraInfo[cameraIdx].capability[i].maxfps);
			m_RichEditCtrl23.SetSel(-1, -1);
			m_RichEditCtrl23.ReplaceSel(info);
		}
	}
}

void CserphonetestDlg::OnBnClickedButton28()
{
	// TODO: 在此添加控件通知处理程序代码
	StopPlayVideoFromRtpDump();
	m_dlgFullScreen->ShowWindow(SW_HIDE);
}


void CserphonetestDlg::OnBnClickedButton29()
{
	// TODO: 在此添加控件通知处理程序代码
	/*	CDlgStatistics *pdlg = new CDlgStatistics;
	pdlg->DoModal()*/;	

#if 0
	CString str;
	//获得按钮文本
	GetDlgItemText(IDC_BUTTON29,str);
	if(str=="显示统计信息")
	{
		//设置按钮文本
		SetDlgItemText(IDC_BUTTON29,_T("隐藏统计信息"));
	}
	else
	{
		SetDlgItemText(IDC_BUTTON29,_T("显示统计信息"));
	}
	//两个静态变量，存储对话框尺寸信息
	static CRect rectLarge;
	static CRect rectSmall;
	//如果还没有填充数值
	if(rectLarge.IsRectNull())
	{
		CRect rectSeparator;
		//获取完整对话框位置参数
		GetWindowRect(&rectLarge);
		//获取图像控件的位置参数（有用的是right）
		GetDlgItem(IDC_SEPARATOR)->GetWindowRect(&rectSeparator);
		rectSmall.left=rectLarge.left;
		rectSmall.top=rectLarge.top;
		rectSmall.right=rectSeparator.right;//替换新值
		rectSmall.bottom=rectLarge.bottom;
	}
	if(str=="<<收缩")
	{
		//显示“简化版”对话框
		SetWindowPos(NULL,0,0,rectSmall.Width(),rectSmall.Height(), SWP_NOMOVE|SWP_NOZORDER);
	}
	else
	{
		//显示“完整版”对话框
		SetWindowPos(NULL,0,0,rectLarge.Width(),rectLarge.Height(), SWP_NOMOVE|SWP_NOZORDER);
	}

#else
	CRect rect;
	GetWindowRect(&rect);   //缩小后的矩形
	if(!m_bExpand)
	{
		rect.SetRect(rect.left,rect.top,rect.left+m_nExpandedWidth, rect.bottom); //扩展
		SetDlgItemText(IDC_BUTTON29,_T("隐藏统计信息"));
		m_bExpand=TRUE;
	}
	else
	{
		rect.SetRect(rect.left,rect.top,rect.left+m_nNormalWidth, rect.bottom);
		SetDlgItemText(IDC_BUTTON29,_T("显示统计信息"));
		m_bExpand=FALSE;
	}
    MoveWindow(&rect,TRUE);
#endif
}


void CserphonetestDlg::OnBnClickedButtonDesktopShare()
{
	// TODO: 在此添加控件通知处理程序代码
	//serserphone_call_start_desktop_share();
}


void CserphonetestDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	
	CWnd *p_wnd = GetDlgItem(IDC_RICHEDIT21);
	if(m_bFullScreen==FALSE)
	{  
		m_bFullScreen = TRUE;  

		//获取对话框原始位置  
		GetWindowPlacement(&m_OldWndPlacement);  
		CRect WindowRect;  
		GetWindowRect(&WindowRect);  
		CRect ClientRect;  
		RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, &ClientRect);  
		ClientToScreen(&ClientRect);  

		//获取屏幕的分辨率  
		int nFullWidth  = GetSystemMetrics(SM_CXSCREEN);  
		int nFullHeight = GetSystemMetrics(SM_CYSCREEN);  

		//对话框全屏显示  
		m_FullScreenRect.left = WindowRect.left - ClientRect.left;  
		int m_top = WindowRect.top - ClientRect.top;  
		m_FullScreenRect.top    = m_top;  
		m_FullScreenRect.right  = WindowRect.right - ClientRect.right + nFullWidth;  
		m_FullScreenRect.bottom = WindowRect.bottom - ClientRect.bottom + nFullHeight;  

		m_NewWndPlacement.length           = sizeof(WINDOWPLACEMENT);  
		m_NewWndPlacement.flags            = 0;  
		m_NewWndPlacement.showCmd          = SW_SHOWNORMAL;  
		m_NewWndPlacement.rcNormalPosition = m_FullScreenRect;  
		SetWindowPlacement(&m_NewWndPlacement);  

		//Picture控件全屏显示  
		p_wnd->MoveWindow(CRect(0, 0, nFullWidth, nFullHeight));  

		//隐藏控件  
		//GetDlgItem(IDC_PLAY)->ShowWindow(SW_HIDE); 
	} 
	else
	{
		//恢复默认窗口  
		SetWindowPlacement(&m_OldWndPlacement);  

		m_bFullScreen = FALSE;  

		//显示控件  
		//GetDlgItem(IDC_PLAY)->ShowWindow(SW_SHOW);  

	}
	
	CDialogEx::OnLButtonDblClk(nFlags, point);
}


void CserphonetestDlg::OnEnMsgfilterRichedit21(NMHDR *pNMHDR, LRESULT *pResult)
{
	MSGFILTER *pMsgFilter = reinterpret_cast<MSGFILTER *>(pNMHDR);
	// TODO:  控件将不发送此通知，除非您重写
	// CDialogEx::OnInitDialog() 函数，以将 EM_SETEVENTMASK 消息发送
	// 到该控件，同时将 ENM_KEYEVENTS 或 ENM_MOUSEEVENTS 标志
	//“或”运算到 lParam 掩码中。

	// TODO:  在此添加控件通知处理程序代码

	*pResult = 0;
}


void CserphonetestDlg::OnBnClickedPlayStream()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!g_rtmpLiveStreamHandle) {
		g_rtmpLiveStreamHandle = createLiveStream();
	}
	else
		stopLiveStream(g_rtmpLiveStreamHandle);
	this->UpdateData(true);

	CWnd *rcwnd = g_dlg->GetDlgItem(IDC_RICHEDIT21);
	USES_CONVERSION;
	char* url = T2A(m_live_url.GetBuffer(0));
	playLiveStream(g_rtmpLiveStreamHandle, url, rcwnd->GetSafeHwnd());
}



void CserphonetestDlg::OnCbnSelchangeVideoSource()
{
	// TODO: 在此添加控件通知处理程序代码
	this->UpdateData(true);
	int i = m_video_source.GetCurSel();
	if (i == 1) { //share desktop
		if (!g_rtmpLiveStreamHandle) {
			g_rtmpLiveStreamHandle = createLiveStream();
		}
		int m = m_share_windows.GetCount();
		m_share_windows.ResetContent();
		WindowShare *windows;
		int num = getShareWindows(g_rtmpLiveStreamHandle, &windows);
		for (int i = 0; i < num; i++) {
			m_share_windows.AddString(TransformUTF8ToUnicodeM(windows[i].title));
			PrintConsole("add new %d  id %d\n", i, windows[i].id);
		}
		
	}
	else if (i == 0) {
		m_share_windows.ResetContent();
	}
}

void CserphonetestDlg::OnBnClickedButton24()
{
	// TODO: 在此添加控件通知处理程序代码
	//	setDenoisingEnabled(true);
	StopPlayAudioFromRtpDump();
}

void CserphonetestDlg::OnBnClickedStopLive()
{
	// TODO: 在此添加控件通知处理程序代码
	if (g_rtmpLiveStreamHandle) {
		stopLiveStream(g_rtmpLiveStreamHandle);
#if 0
		disableLiveStreamBeauty(g_rtmpLiveStreamHandle);
#endif
	}
}


void CserphonetestDlg::OnEnChangeLiveUrl()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}




void CserphonetestDlg::OnBnClickedPushStream()
{
	if (!g_rtmpLiveStreamHandle) {
		g_rtmpLiveStreamHandle = createLiveStream();
	}
	else
		stopLiveStream(g_rtmpLiveStreamHandle);

	this->UpdateData(true);
	int i = m_video_source.GetCurSel();
	setLiveVideoSource(g_rtmpLiveStreamHandle, i);
//	selectCameraLiveStream(g_rtmpLiveStreamHandle, 1, 640, 480, 15);
	CWnd *rcwnd = g_dlg->GetDlgItem(IDC_RICHEDIT21);

	USES_CONVERSION;
	char* url = T2A(m_live_url.GetBuffer(0));
	pushLiveStream(g_rtmpLiveStreamHandle, url, rcwnd->GetSafeHwnd());

#if 0
	enableLiveStreamBeauty(g_rtmpLiveStreamHandle);
#endif

}


void CserphonetestDlg::OnCbnSelchangeShareWindow()
{
	// TODO: 在此添加控件通知处理程序代码
	this->UpdateData(true);
	if (g_rtmpLiveStreamHandle) {

		WindowShare *windows;
		int index = m_share_windows.GetCurSel();
		int num = getShareWindows(g_rtmpLiveStreamHandle, &windows);
		if (index < num) {
			selectShareWindow(g_rtmpLiveStreamHandle, windows[index].type, windows[index].id);
			PrintConsole(" select index %d  id %d\n", index, windows[index].id);
		}
	}
}


void CserphonetestDlg::OnCbnDropdownShareWindow()
{
	// TODO: 在此添加控件通知处理程序代码
	OnCbnSelchangeVideoSource();
}


void CserphonetestDlg::OnEnChangeEdit10()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnEnChangeEdit12()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnEnChangeEdit14()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnEnChangeEdit11()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnEnChangeAudioPayloadName()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnEnChangeAudioPt()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnEnChangeVideoPayloadName()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnEnChangeEdit1()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnEnChangeEdit2()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnEnChangeEdit3()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnEnChangeEdit4()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnEnChangeEncryptionKey()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnCbnSelchangeComboVideoCodec()
{
	// TODO: Add your control notification handler code here
}


void CserphonetestDlg::OnCbnSelchangeComboVideoMode()
{
	// TODO: Add your control notification handler code here
}


void CserphonetestDlg::OnCbnSelchangeCombo1()
{
	// TODO: Add your control notification handler code here
}


void CserphonetestDlg::OnCbnSelchangeComboEncryptType()
{
	// TODO: Add your control notification handler code here
	// TODO: 在此添加控件通知处理程序代码
	this->UpdateData(true);
	m_encryptionKey.Empty();
	this->UpdateData(false);
}


void CserphonetestDlg::OnEnChangeEdit13()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnEnChangeVideoPt()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnEnChangeEdit16()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CserphonetestDlg::OnBnClickedButton30()
{
	// TODO: Add your control notification handler code here
	USES_CONVERSION;
	UpdateData(TRUE);
	sendTmmbr(g_currentCallId, m_remoteSSRC);
}
