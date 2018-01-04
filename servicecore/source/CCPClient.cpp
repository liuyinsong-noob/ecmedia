#include "CCPClient.h"
#include "CCPClient_Internal.h"
#include "serphoneinterface.h"
#include "serphonecall.h"
#include "servicecore.h"
#include "sal_eXosip2.h"
#include "AuthToken.h"
#include "ice.h"
#include "RESTClient.h"
#include "ECMedia.h"

#ifdef __APPLE_CC__
#include "TargetConditionals.h"
#endif


extern void setSsrcMediaType(unsigned int& ssrc, int type);
extern void setSsrcMediaAttribute(unsigned int& ssrc, unsigned short width, unsigned short height, unsigned char maxFramerate);

#define CCP_SDK_VERSION "1.1.23.7"
#ifdef WIN32

#include < windows.h >
#include < process.h >

typedef unsigned long ccthread;
 int cc_createthread(ccthread& t, void *(*f) (void *), void* p)
{
	return t = (ccthread)_beginthread((void( __cdecl * )( void * ))f, 0, p);
}

#else

// Use POSIX threading...
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef pthread_t ccthread;

static int cc_createthread(ccthread& t, void *(*f) (void *), void* p)
{
	return pthread_create((pthread_t*)&t, 0, f, p);
}

static void Sleep(unsigned long dwMilliseconds)
{
	usleep(dwMilliseconds * 1000);
}

#endif


#define SDK_UN_INITIAL_ERROR(ret) if( !g_pSerCore) { return ret;}

static int gAliveTimeFor3G =	15000;
static int gAliveTimeForWifi=	60000;
static int gAliveTimeForPC=	60000;
static bool g_Iterate = false;
static bool g_IterateRuning = false;
static char gDisplayName[32] = {'\0'};
CCallbackInterface g_cbInterface;
static ServiceCore  *g_pSerCore = NULL;
static ccthread g_Thread;
static char gToken[128]= {'\0'};
char *gUserDataForInvite = NULL;
char *gRecvUserData = NULL;
char gVersionString[256]={'\0'};
//static int  gDebugLevel = LOG_LEVEL_DEBUG;
static int g_bConnected = false;
static int g_NetworkType = NETWORK_LAN;
int g_RegisterTimerFlag = 1;
time_t g_RegisterTimerB;
int g_RegisterErrorCount = 0;
static bool g_hasRegistered = false;
#ifdef WIN32
static int g_keepAliveTime = gAliveTimeForPC;
#else
static int g_keepAliveTime = gAliveTimeForWifi;
#endif

static bool l_kickoff = false;

FILE *traceFile = NULL;

#if 0
extern char * globalFilePath;
extern char * globalFilePath2;
#endif
typedef struct proxyAddr {
    struct proxyAddr *next;
    char* addr;
    int port;
    char* account;
    char* passwd;
    char* capability;
} proxyAddrList;

static proxyAddrList *gProxyAddrLst = NULL;

#define CHECK_PROXY_VALID_KEY  "acbef97b9a534203b2ebcf3befb2a240"


/////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

void CCPClientPrintLog(int loglevel, const char *loginfo)
{
	if(g_cbInterface.onLogInfo )
		g_cbInterface.onLogInfo(loginfo);
}

std::string timetodate(time_t const timer)
{
    struct tm *l=localtime(&timer);

    char buf[128];
    snprintf(buf,sizeof(buf),"%04d-%02d-%02d %02d:%02d:%02d",l->tm_year+1900,l->tm_mon+1,l->tm_mday,l->tm_hour,l->tm_min,l->tm_sec);
    std::string s(buf);
    return s;

}


//global state change
void mainStateChanged(class ServiceCore *lc, SerphoneGlobalState gstate, const char *message)
{
	//PrintConsole("global state change %s,state=%s\n",message,serphone_global_state_to_string(gstate));
	return;
}

void GetNumberForSIPAccount(const char *sipaccount ,char *number, int calledlen)
{

	char *str;
	str = strstr((char*)sipaccount,"sip:");
	str = str + 4;
	int i = 0;
	while(i <calledlen)
	{
		if(str[i] == '@')
		{
			number[i] = '\0';
			break;
		}
		number[i] = str[i];
		i++;
	}
	return;
}

//call state change
void callStateChanged(class ServiceCore *lc, SerPhoneCall *call, SerphoneCallState cstate, const char *message)
{
	static char callid[64] = {'\0'};
	static char called[64] = {'\0'};

	if (!call->op) {
        PrintConsole("unknow cal salop \n");
        return ;
    }
	/*update begin------------------Sean20130729----------for video ice------------*/
//    snprintf(callid,63,"%d",call->op->cid);
    memcpy(callid, call->_user_call_id, 9);
    if (!call->core) {
        PrintConsole("unknown cal core");
        return;
    }
    /*update end--------------------Sean20130729----------for video ice------------*/

	switch( cstate )
	{
	case LinphoneCallIdle:
		{

		}
	break;

	case LinphoneCallOutgoingInit:
#ifndef WIN32
		sal_set_keepalive_period(lc->sal,5000);
#endif
		break;

	case LinphoneCallOutgoingProgress:
		break;

	case LinphoneCallOutgoingProceeding:
		{
			if(!g_cbInterface.onCallProceeding )
				return;
			PrintConsole("[APICall] onCallProceeding (callid=%s)\n",callid);
			g_cbInterface.onCallProceeding(callid);
		}
		break;
	case LinphoneCallConnected:
		{
#if 0
            g_pSerCore->serphone_set_ringback(call,true);
#endif
			if(!g_cbInterface.onCallAnswered )
				return;

			PrintConsole("[APICall] onCallAnswered (callid=%s)\n",callid);
			g_cbInterface.onCallAnswered(callid);
		}
		break;
	case LinphoneCallOutgoingEarlyMedia:
		{
//#ifdef WIN32
//            g_pSerCore->serphone_pre_after_ring_stop(true);
//#endif
			if(g_cbInterface.onNotifyGeneralEvent) {
#ifdef OLDERRORCODE
				g_cbInterface.onNotifyGeneralEvent(callid,G_EVENT_EarlyMedia,"",0);
#else
                g_cbInterface.onNotifyGeneralEvent(callid,G_EVENT_EarlyMedia,"",ReasonNone);
#endif
				break;
			}

			//break;
		}
	case LinphoneCallOutgoingRinging:
		{
//#ifdef WIN32
//            g_pSerCore->serphone_pre_after_ring_stop(true);
//#endif
			if( !g_cbInterface.onCallAlerting )
				return;
			PrintConsole("[APICall] onCallAlerting (callid=%s)\n",callid);
			g_cbInterface.onCallAlerting(callid);
		}
		break;
	case LinphoneCallIncomingReceived:
		{
			if( !g_cbInterface.onIncomingCallReceived )
				return;

			char *from = serphone_call_get_remote_address_as_string(call);
			GetNumberForSIPAccount(from,called,64);
			ms_free((void **)&from);
            from = NULL;
			if(  call->op->invite_userdata)
			{
				if(gRecvUserData)
					ms_free((void**)&gRecvUserData);
				gRecvUserData = ms_strdup(call->op->invite_userdata);
			}
			else
			{
				if(gRecvUserData)
					ms_free((void**)&gRecvUserData);
				gRecvUserData = ms_strdup("");
			}
			PrintConsole("[APICall] onIncomingCallReceived (type=%s,callid=%s,called=%s)\n",
				call->params.has_video ? "Video" : "Voice",callid,called);
			g_cbInterface.onIncomingCallReceived(call->params.has_video, callid,called);
#ifndef WIN32
			sal_set_keepalive_period(lc->sal,5000);
			sendKeepAlive();
#endif
			break;
		}

	case LinphoneCallStreamsRunning:
#ifndef WIN32
		sal_set_keepalive_period(lc->sal,5000);
#endif
		break;

	case LinphoneCallEnd:
		{
            g_pSerCore->serphone_core_set_process_original_audio_data_flag(call, false);
#ifdef VIDEO_ENABLED
            lc->serphone_set_video_conference_released();
#endif
#ifdef HAIYUNTONG
            lc->serphone_delete_transmit_key();
#endif
#ifdef WIN32
			PrintConsole("[APICall] LinphoneCallEnd (callid=%s,call->dir=%d)\n",callid,call->dir);
            g_pSerCore->serphone_pre_after_ring_stop(false);
            if (call->dir == LinphoneCallOutgoing) {
                g_pSerCore->serphone_afterring_start();
            }
#endif

			if( !g_cbInterface.onCallReleased )
			return;

			sal_set_keepalive_period(g_pSerCore->sal,g_keepAliveTime);
			if( call->dir == LinphoneCallOutgoing &&  !call->callConnected  )
			{

				PrintConsole("[APICall] onMakeCallFailed (callid=%s,reason=%d)\n",
					callid,call->reason );
				g_cbInterface.onMakeCallFailed(callid,call->reason);

				return;
			}
			PrintConsole("[APICall] onCallReleased (callid=%s)\n",callid);
			g_cbInterface.onCallReleased(callid);
			PrintConsole("[APICall] onCallReleased (callid=%s) out\n",callid);
		}
		break;
	case LinphoneCallPausing:
		break;
	case LinphoneCallPaused:
		{
			if( !g_cbInterface.onCallPaused )
				return;

			PrintConsole("[APICall] onCallPaused (callid=%s)\n",callid);
			g_cbInterface.onCallPaused(callid);
		}
		break;
        case LinphoneCallResuming:
            break;
        case LinphoneCallResumed:
        {
			PrintConsole("[APICall] onResumed (callid=%s)\n",callid);
            if( !g_cbInterface.onResumed )
				return;

			PrintConsole("[APICall] onResumed (callid=%s)\n",callid);
			g_cbInterface.onResumed(callid);
        }
		break;
	case LinphoneCallRefered:
		break;
	case LinphoneCallError:
		{
#ifdef WIN32
            g_pSerCore->serphone_pre_after_ring_stop(false);
//            g_pSerCore->serphone_afterring_start();
#endif
            sal_set_keepalive_period(g_pSerCore->sal,g_keepAliveTime);
			if(!g_cbInterface.onMakeCallFailed )
				return;
				PrintConsole("[APICall] onMakeCallFailed (callid=%s,reason=%d)\n",
					callid,call->reason );
			g_cbInterface.onMakeCallFailed(callid,call->reason);

		}
		break;
	case LinphoneCallPausedByRemote:
		{
			if(!g_cbInterface.onCallPaused )
				return;
			PrintConsole("[APICall] onCallPausedByRemote (callid=%s)\n",callid);
			g_cbInterface.onCallPausedByRemote(callid);
		}
		break;


	case LinphoneCallUpdatedByRemote:
        {
            bool automatically_accept = false;
            bool video_requested=  ( call->remote_params.has_video!=0);
            bool video_used= (call->current_params.has_video!=0);
            PrintConsole("[APICall] LinphoneCallUpdatedByRemote Video used=%i, video requested=%i, automatically_accept=%i\n",
                         video_used,video_requested,automatically_accept);

            if(g_cbInterface.onCallMediaUpdateRequest)
            {
                //remote request video
                if (!video_used && video_requested && !automatically_accept){
                    lc->serphone_core_defer_call_update(call);
                    g_cbInterface.onCallMediaUpdateRequest(callid, 0);
                }
                //remote stop video
                if(video_used && !video_requested) {
                    g_cbInterface.onCallMediaUpdateRequest(callid, 1);
                }
            }
        }
		break;
	case LinphoneCallIncomingEarlyMedia:
		break;

	case LinphoneCallUpdated:
        {
            bool video_result= (call->params.has_video!=0);
            bool video_used= (call->current_params.has_video!=0);
            PrintConsole("[APICall] LinphoneCallUpdated Video used=%i, video requested=%i\r\n",
                         video_used,video_result);

            if(g_cbInterface.onCallMediaUpdateResponse)
            {
                //remote accept add video
                if (!video_used){
                    g_cbInterface.onCallMediaUpdateResponse(callid, 0);
                }
                //remote accept remove video
                if(video_used) {
                    g_cbInterface.onCallMediaUpdateResponse(callid, 1);
                }
            }
        }
        break;
    case LinphoneCallUpdatedRemoteVideoratio:
        {
            PrintConsole("LinphoneCallUpdatedRemoteVideoratio %s\n", message);
            if(g_cbInterface.onNotifyGeneralEvent)
            {
#ifdef OLDERRORCODE
                g_cbInterface.onNotifyGeneralEvent(callid,G_EVENT_RemoteVideoRatio,message,0);
#else
                g_cbInterface.onNotifyGeneralEvent(callid,G_EVENT_RemoteVideoRatio,message,ReasonNone);
#endif
            }
        }
        break;
	case LinphoneCallReleased:
		{
			if(!g_cbInterface.onCallReleased )
				return;
			//sprintf(callid,"%d",call->op->cid);
			//g_cbInterface.onCallReleased(callid);
		}
		break;
    case LinphoneCallUpdatedAudioDestinationChanged:
    {
        PrintConsole("SerphoneCallUpdatedAudioDestinationChanged new destination is %s\n",message);
        if(g_cbInterface.onNotifyGeneralEvent)
        {
#ifdef OLDERRORCODE
            g_cbInterface.onNotifyGeneralEvent(callid,G_EVENT_AudioDestinationChanged,message,0);
#else
            g_cbInterface.onNotifyGeneralEvent(callid,G_EVENT_AudioDestinationChanged,message,ReasonNone);
#endif
        }
    }
        break;
    case LinphoneCallUpdatedVideoDestinationChanged:
    {
        PrintConsole("SerphoneCallUpdatedVideoDestinationChanged new destination is %s\n",message);
        if(g_cbInterface.onNotifyGeneralEvent)
        {
#ifdef OLDERRORCODE
            g_cbInterface.onNotifyGeneralEvent(callid,G_EVENT_VideoDestinationChanged,message,0);
#else
            g_cbInterface.onNotifyGeneralEvent(callid,G_EVENT_VideoDestinationChanged,message,ReasonNone);
#endif
        }
    }
        break;
	}

	return;
}

static void* CheckProxyVaildFun(void *p)
{
    int status;
    bool proxyValid = false;

    if (!strcmp((char*)p,"103.5.126.154") || !strcmp((char*)p,"103.5.126.155") || !strcmp((char*)p,"10.0.19.11") || !strcmp((char *)p, "10.0.19.15") || !strcmp((char *)p, "211.157.178.195") || !strcmp((char *)p, "211.157.178.196") || !strcmp((char *)p, "192.168.2.190") || !strcmp((char *)p, "192.168.178.171") || !strcmp((char *)p, "192.168.1.3") || !strcmp((char *)p, "192.168.1.2") || !strcmp((char *)p, "211.157.178.211"))
	return NULL;

    //	TRESTClient *client = new TRESTClient("10.0.19.11", 8881, "", "", "");//("app.cloopen.net", 8881, "", "", "");  //xinwei
    TRESTClient *client = new TRESTClient("app.cloopen.com", 8881, "", "", "");//("app.cloopen.net", 8881, "", "", "");
    client->SetTraceFunc(PrintConsole);
    if( client->CheckProxyValid((char*)p, "", CHECK_PROXY_VALID_KEY, status) )
    // if( client->CheckProxyValid("42.121.118.111", "", CHECK_PROXY_VALID_KEY, status) )
    {
        if(status == 0)
            proxyValid = true;
    }
    PrintConsole("DEBUG: CheckProxyVaildFun: errormsg: %s\n",client->GetErrorMessge().c_str());
    delete client;

    if(!proxyValid)
    {
        PrintConsole("[APICall] CheckProxyVaildFun, onConnectError(%d) status=%d\n", ReasonInvalidProxy, status);

        if(g_pSerCore)
        {
            if (traceFile) {
                time_t temp = time(NULL);
                std::string strTime = timetodate(temp);
                fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                fwrite(" Something goes wrong Unregister called2\n", strlen(" Something goes wrong Unregister called2\n"), 1, traceFile);
                fflush(traceFile);
            }
            g_pSerCore->serphone_proxy_remove((char*)p);
        }
        g_bConnected = false;

        if( !g_cbInterface.onConnectError )
            return NULL;
        g_cbInterface.onConnectError(status);
    }
    else
    {
        PrintConsole("[APICall] onConnected()\n");
        g_cbInterface.onConnected();

    }
    return NULL;
}

static void* CheckPrivateCloud(void *lc)
{

    int status;
    TRESTClient *client = NULL;
    bool proxyValid = false;
    SerphoneProxyConfig *cfg = NULL;
    const char *restAddr = NULL;
    bool nativeCheck = false;
    const char *companyID = NULL;
    if (lc) {
        ServiceCore *tempServiceCore = (ServiceCore *)lc;
        tempServiceCore->serphone_core_get_default_proxy( &cfg);
        restAddr = tempServiceCore->serphone_get_privateCloudCheckAddress();
        nativeCheck = tempServiceCore->serphone_get_nativeCheck();
        companyID = tempServiceCore->serphone_get_privateCloudCompanyID();
    }
    else
    {
        PrintConsole("ERROR: CheckPrivateCloud: lc is NULL!\n");
        return NULL;
    }
//#ifdef NOLISCENSECHECK
//    proxyValid = true;
//#else
//    if (!strcmp(cfg->realm, "192.168.178.84")) {
//        proxyValid = true;
//    }
//    else
//    {
        if (nativeCheck && restAddr) {
            const char *colonPos = strchr(restAddr, ':');
            char nativeRestAddr[64] = {0};
            int nativeRestPort = 0;
            if (colonPos) {
                memcpy(nativeRestAddr,restAddr,colonPos-restAddr);
                nativeRestPort = atoi(colonPos+1);
            }
            else
            {
                memcpy(nativeRestAddr, restAddr, strlen(restAddr));
                nativeRestPort = 8881;
            }
            client = new TRESTClient(nativeRestAddr, nativeRestPort==0?8881:nativeRestPort, "", "", "");
        }
        else
        {
            client = new TRESTClient("app.cloopen.com", 8881, "", "", "");
        }
        client->SetTraceFunc(PrintConsole);
        if( cfg && companyID && client->CheckPrivateProxyValid(companyID, cfg->realm, cfg->port, status) )
        {
            if(status == 0)
                proxyValid = true;
        }

        PrintConsole("DEBUG: CheckPrivateCloud: errormsg: %s\n",client->GetErrorMessge().c_str());
        delete client;
//    }
//#endif


    if(!proxyValid)
    {
        PrintConsole("[APICall] CheckPrivateCloud,onConnectError(%d) status=%d\n", ReasonInvalidProxy, status);

        if(g_pSerCore)
        {
            if (traceFile) {
                time_t temp = time(NULL);
                std::string strTime = timetodate(temp);
                fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                fwrite(" Something goes wrong Unregister called3\n", strlen(" Something goes wrong Unregister called3\n"), 1, traceFile);
                fflush(traceFile);
            }
            g_pSerCore->serphone_proxy_remove(cfg->realm);
        }
        g_bConnected = false;

        if( !g_cbInterface.onConnectError )
            return NULL;
        g_cbInterface.onConnectError(ReasonInvalidProxy);
    }
    else
    {
        PrintConsole("[APICall] onConnected()\n");
        g_bConnected = true;
        if (g_pSerCore)
            sal_set_keepalive_period(g_pSerCore->sal,g_keepAliveTime);
        g_cbInterface.onConnected();
    }
    return NULL;
}

//retister state change
void registrationStateChanged(class ServiceCore *lc, SerphoneProxyConfig *cfg,
					SerphoneRegistrationState cstate, const char *message)
{

	switch(cstate){
	case LinphoneRegistrationCleared:	//Unregistration succeeded
		{
            if (g_pSerCore)
                sal_set_keepalive_period(g_pSerCore->sal,600000);
            //sean todo
            //
            if (!g_cbInterface.onLogOut) {
                PrintConsole("[APICALL] onLogOut not implemented\n");
                return;
            }
            g_bConnected = false;
            g_cbInterface.onLogOut();
            g_hasRegistered = false;
            if (traceFile) {
                time_t temp = time(NULL);
                std::string strTime = timetodate(temp);
                fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                fwrite(" Registration cleared\n", strlen(" Registration cleared\n"), 1, traceFile);
                fflush(traceFile);
            }
            l_kickoff = true;
		}
		break;
	case LinphoneRegistrationNone:		//Initial state for registrations
		break;
	case LinphoneRegistrationProgress:	//Registration is in progress
		break;
	case LinphoneRegistrationOk:		//Registration is successful
		{
            g_RegisterTimerFlag = 1;
            l_kickoff = false;

            g_hasRegistered = true;
            g_RegisterErrorCount = 0;

            if (traceFile) {
                time_t temp = time(NULL);
                std::string strTime = timetodate(temp);
                fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                fwrite(" Registration ok\n", strlen(" Registration ok\n"), 1, traceFile);
                fflush(traceFile);
            }

			if(!g_cbInterface.onConnected )
            {
                if (traceFile) {
                    time_t temp = time(NULL);
                    std::string strTime = timetodate(temp);
                    fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                    fwrite(" !g_cbInterface.onConnected not implemented\n", strlen(" !g_cbInterface.onConnected not implemented\n"), 1, traceFile);
                    fflush(traceFile);
                }
				return;
            }
			if( !g_bConnected )
			{

                PrintConsole("[APICall reconenct %s]",lc->serphone_get_reconnect()?"true":"false");
                if (traceFile) {
                    time_t temp = time(NULL);
                    std::string strTime = timetodate(temp);
                    fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                    char tempp[200] = {0};
                    sprintf(tempp, " [APICall reconenct %s]\n",lc->serphone_get_reconnect()?"true":"false");
                    fwrite(tempp, strlen(tempp), 1, traceFile);
                    fflush(traceFile);
                }
//#ifdef HAIYUNTONG
                PrintConsole("[APICall] onConnected()\n");
                g_bConnected = true;
                if (g_pSerCore)
                    sal_set_keepalive_period(g_pSerCore->sal,g_keepAliveTime);
                g_cbInterface.onConnected();

//#else
//                if (lc->serphone_get_reconnect()) {
//                    PrintConsole("[APICall] onConnected()\n");
//                    g_bConnected = true;
//                    if (g_pSerCore)
//                        sal_set_keepalive_period(g_pSerCore->sal,g_keepAliveTime);
//                    g_cbInterface.onConnected();
//                }
//                else
//                {
//#ifdef XINWEI
//                    PrintConsole("[APICall] onConnected()\n");
//                    g_cbInterface.onConnected();
//#endif
//#ifdef XINWEI
//                    const char *domain=cfg->serphone_proxy_config_get_domain();
//#endif
//                    ccthread thread;
//#ifdef XINWEI
//                    if (strlen(lc->serphone_get_privateCloudCompanyID())) {
//#endif
//                        cc_createthread(thread, CheckPrivateCloud, lc);
//#ifdef XINWEI
//                    }
//                    else
//                        cc_createthread(thread,CheckProxyVaildFun,(void*)domain);
//#endif
//                }
//#endif //end of HAIYUNTONG



            }
            g_bConnected = true;
		}
		break;
	case LinphoneRegistrationFailed:	//Registration failed
		{
            proxyAddrList *tempAddr = gProxyAddrLst->next;
            if (tempAddr) {
                gProxyAddrLst->next = tempAddr->next;
                connectToCCP(tempAddr->addr, tempAddr->port, tempAddr->account, tempAddr->passwd, tempAddr->capability);
                delete [] tempAddr->addr;
                tempAddr->addr = NULL;
                delete [] tempAddr->account;
                tempAddr->account = NULL;
                delete [] tempAddr->passwd;
                tempAddr->passwd = NULL;
                delete [] tempAddr->capability;
                tempAddr->capability = NULL;
                delete tempAddr;
                tempAddr = NULL;
                return;
            }
            PrintConsole("[APICall] registrationStateChanged,onConnectError(%d)\n",cfg->error);
            g_bConnected = false;
            g_RegisterTimerFlag = 1;
            l_kickoff = false;
            g_RegisterErrorCount = 0;

            if (traceFile) {
                time_t temp = time(NULL);
                std::string strTime = timetodate(temp);
                fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                char tempp[200] = {0};
                sprintf(tempp, " [APICall] registrationStateChanged,onConnectError(%d)\n",cfg->error);
                fwrite(tempp, strlen(tempp), 1, traceFile);
                fflush(traceFile);
            }

			if( !g_cbInterface.onConnectError )
            {
                if (traceFile) {
                    time_t temp = time(NULL);
                    std::string strTime = timetodate(temp);
                    fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                    fwrite(" !g_cbInterface.onConnectError not implemented\n", strlen(" !g_cbInterface.onConnectError not implemented\n"), 1, traceFile);
                    fflush(traceFile);
                }
				return;
            }
			g_cbInterface.onConnectError(cfg->error);
		}
		break;
	}
	return;
}
//
void recvTextMessage(ServiceCore *lc, SerphoneChatRoom *room,
			const SerphoneAddress *from, const SerphoneAddress *to, const char *msgid, const char *message, const char *date)
{
    PrintConsole("[APICall] on RecvTextMessage %s \n",message);

    size_t msglen = strlen(message);
    int i = 0;
    for( ; i < msglen; i++) {
		PrintConsole(" i= %d \n",i);
		if( message[i] == '\r' ||  message[i] == '\n' || message[i]==' ')
            continue;
        else
            break;
    }
    if( strncmp(message+i, "type=", 5) != 0 ) {
        if( !g_cbInterface.onTextMessageReceived ) {
            PrintConsole("[APICall] not set onTextMessageReceived callback\n");
            return;
        }

        char sender[65] ={0};
        char *fromAccount = serphone_address_as_string(from);
        GetNumberForSIPAccount(fromAccount,sender,64);
        ms_free((void **)&fromAccount);
        fromAccount = NULL;

        char receiver[65] = {0};
        char *toAccount = serphone_address_as_string(to);
        GetNumberForSIPAccount(toAccount,receiver,64);
        ms_free((void **)&toAccount);
        toAccount = NULL;

        g_cbInterface.onTextMessageReceived(sender, receiver, NULL, msgid, message+i, NULL);
    }
    else if( strncmp(message+i, "type=0\r\n", 8) == 0 ) {
        if( !g_cbInterface.onTextMessageReceived ) {
            PrintConsole("[APICall] not set onTextMessageReceived callback\n");
            return;
        }

        char sender[65] ={0};
        char *fromAccount = serphone_address_as_string(from);
        GetNumberForSIPAccount(fromAccount,sender,64);
        ms_free((void **)&fromAccount);
        fromAccount =  NULL;

        char receiver[65] = {0};
        char *toAccount = serphone_address_as_string(to);
        GetNumberForSIPAccount(toAccount,receiver,64);
        ms_free((void **)&toAccount);
        toAccount = NULL;

        char *usrdata = NULL;
        char *sendtime = NULL;

        i += 8;
        if( strncmp(message+i, "[[[", 3) == 0 ) {
            i += 3;
            const char *endStrTime = strstr(message+i, "]]]\r\n");
            if(endStrTime) {
                int usrLen = endStrTime - (message+i);
                sendtime = (char *)ms_malloc(usrLen+1);
				memset(sendtime, 0, usrLen+1);
                memcpy(sendtime, message+i, usrLen);
                i += usrLen;
                i += 5;
            } else {
                 i -= 3;
            }
        }

        if( strncmp(message+i, "<<<", 3) == 0 )
        {
            i += 3;
            char *endUsrData = (char*)strstr(message+i, ">>>\r\n");
            if(endUsrData)
            {
                int usrLen = endUsrData - (message+i);
                usrdata = (char *)ms_malloc(usrLen+1);
				memset(usrdata, 0, usrLen+1);
                memcpy(usrdata, message+i, usrLen);
                usrdata[usrLen] = '\0';
                i += usrLen;
                i += 5;
            }
            else
            {
                i -= 3;
            }
        }
#ifdef HAIYUNTONG
        //call message decrypt
        //only decrypt type=0
        bool needDecrypt = true;
        if (sender) {
            int len1 = strlen(sender);
            if (len1>=8) {
                needDecrypt = strncasecmp(sender+len1-8, "00000001", 8)==0?false:true;
            }
        }
        if (lc->serphone_haiyuntong_enabled() && needDecrypt) {
            char messageDecrypt[4096] = {0};
            PrintConsole("[DEBUG HAIYUNTONG] decrypt input message:%s",message);
            long messageDecryptLen = 0;
            int ret;
            if (receiver[0] == 'g') {
                ret = lc->serphone_group_sms_decrypt((char *)(message+i),strlen(message+i),sender,strlen(sender),messageDecrypt, &messageDecryptLen);
            }
            else
                ret = lc->serphone_sms_decrypt((char *)(message+i),strlen(message+i),sender,strlen(sender),messageDecrypt, &messageDecryptLen);
            if (0 == ret) {
                g_cbInterface.onTextMessageReceived(sender, receiver, sendtime, msgid, messageDecrypt, usrdata);
            }
        }
        else
        {
            g_cbInterface.onTextMessageReceived(sender, receiver, sendtime, msgid, message+i, usrdata);
        }
#else
        g_cbInterface.onTextMessageReceived(sender, receiver, sendtime, msgid, message+i, usrdata);
#endif
        ms_free((void **)&usrdata);
        usrdata = NULL;
        ms_free((void **)&sendtime);
        sendtime = NULL;
    }
    else if( strncmp(message+i, "type=1\r\n", 8) == 0 )
    {
        if(!g_cbInterface.onNotifyGeneralEvent) {
			PrintConsole("[APICall] not set onNotifyGeneralEvent callback\n");
			return;
		}
        i += 8;
		PrintConsole("[APICall] call onNotifyGeneralEvent(NULL,G_EVENT_MessageCommand...) \n");
#ifdef OLDERRORCODE
		g_cbInterface.onNotifyGeneralEvent(NULL,G_EVENT_MessageCommand,message+i,0);
#else
        g_cbInterface.onNotifyGeneralEvent(NULL,G_EVENT_MessageCommand,message+i,ReasonNone);
#endif
		return ;
    }
    else if( strncmp(message+i, "type=2", 6) == 0 )
    {
        PrintConsole("[APICall] Other client use the same account, pushed off!!\n");
        if( !g_cbInterface.onConnectError )
            return;
        g_bConnected = false;
        g_hasRegistered = false;
		PrintConsole("[APICall] recvTextMessage,onConnectError(%d)\n",ReasonKickedOff);
        if (traceFile) {
            time_t temp = time(NULL);
            std::string strTime = timetodate(temp);
            fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
            fwrite(" Registration kicked off\n", strlen(" Registration kicked off\n"), 1, traceFile);
            fflush(traceFile);
        }

        g_cbInterface.onConnectError(ReasonKickedOff);
        l_kickoff = true;
        if(g_pSerCore)
        {
            g_pSerCore->serphone_core_reg_kickedoff();
        }
        return;
    }
    else if( strncmp(message+i, "type=3\r\n", 8) == 0 )
    {
        if( !g_cbInterface.onMessageSendReport) {
            return;
        }
        i += 8;
		PrintConsole("[APICall] call onMessageSendReport send to the receiver\n");
#ifdef OLDERRORCODE
		g_cbInterface.onMessageSendReport(message+i, date, 200);
#else
        g_cbInterface.onMessageSendReport(message+i, date, 175200);
#endif
		return ;
    }
    else if (strncmp(message+i, "type=4\r\n", 8) == 0) //Remote Video Rotate
    {
        if (!g_cbInterface.onMessageRemoteVideoRotate) {
            return;
        }
        i += 8;
		PrintConsole("[APICall] call onMessageRemoteVideoRotate send to the receiver\n");
		g_cbInterface.onMessageRemoteVideoRotate(message+i);
		return ;
    }
    else
    {
        PrintConsole("[APICall] invalid message, discard!\n");
    }

	return;
}
void textSendReport(ServiceCore *lc, const char *msgid, const char *date, int status)
{
	if( g_cbInterface.onMessageSendReport) {
#ifndef OLDERRORCODE
        status = ReasonNone+status;
#endif
		PrintConsole("[APICall] onMessageSendReport(%s,%d)\n",msgid,status);
		g_cbInterface.onMessageSendReport(msgid,date,status);
	}
}
void dtmfReceived(class ServiceCore* lc, const char *callid, char dtmf)
{
	if( !g_cbInterface.onDtmfReceived )
		return;
	static char callidl[64];
	memcpy(callidl, callid, 9);
	g_cbInterface.onDtmfReceived( callid,dtmf);
	return;
}

void mediaInitFailed(class ServiceCore* lc, SerPhoneCall *call, int type, int error)
{
    char mediaType[8] = {'\0'};
    sprintf(mediaType, "%d", type);

#ifndef OLDERRORCODE
    error = ReasonNone+error;
#endif

    if (call) {
        char callid[64] = {'\0'};
        if (!call->op) {
            PrintConsole("mediaInitFailed unknow cal salop \n");
            return ;
        }
        snprintf(callid,63,"%d",call->op->cid);
        if(g_cbInterface.onNotifyGeneralEvent)
            g_cbInterface.onNotifyGeneralEvent(callid,G_EVENT_MediaInitFailed,mediaType,error);
    }
    else{
        //video init when no call
        if(g_cbInterface.onNotifyGeneralEvent)
            g_cbInterface.onNotifyGeneralEvent(NULL,G_EVENT_MediaInitFailed,mediaType,error);
    }
    PrintConsole("[APICall] MediaInitFailed(%s %d)\n",(type==0)?"audio":"video", error);
}

void deliverVideoFrame(class ServiceCore* lc, SerPhoneCall *call, uint8_t* buf, int size, int width, int height)
{
    if( !g_cbInterface.onDeliverVideoFrame || !call->op )
		return;
	static char callid[64];
	memcpy(callid, call->_user_call_id, 9);
	g_cbInterface.onDeliverVideoFrame(callid, (unsigned char*)buf, size, width, height);
}

void recordAudioCallBack(class ServiceCore *lc, SerPhoneCall *call, const char *file_name, int status)
{
    if( !g_cbInterface.onRecordVoiceStatus)
		return;
    if(call)
        g_cbInterface.onRecordVoiceStatus(call->_user_call_id, file_name, status);
    else
        g_cbInterface.onRecordVoiceStatus(NULL, file_name, status);
}

void throwDataToProcessCallBack(class ServiceCore *lc, SerPhoneCall *call, const void *inData, int inLen, void *outData, int &outLen, bool send)
{
    if (!g_cbInterface.onAudioData) {
        return;
    }
    if (call) {
        g_cbInterface.onAudioData(call->_user_call_id,inData,inLen,outData,outLen,send);
    }
}

void throwOriginalDataToProcessCallBack(class ServiceCore *lc, SerPhoneCall *call, const void *inData, int inLen, int sampleRate, int numChannels, const char *codec, bool send)
{
    if (!g_cbInterface.onOriginalAudioData) {
        return;
    }
    if (call) {
        g_cbInterface.onOriginalAudioData(call->_user_call_id,inData,inLen,sampleRate,numChannels,codec, send);
    }
}

void transferCallStateChangedCallBack(class ServiceCore *lc, SerPhoneCall *call, SerphoneCallState new_call_state)
{
    if (!g_cbInterface.onCallTransfered) {
        return;
    }
    if (call) {
        if (new_call_state == LinphoneCallConnected) {
            g_cbInterface.onCallTransfered(call->_user_call_id,lc->serphone_get_referTo());
        }
    }
}
void connectCameraFailed(class ServiceCore *lc, SerPhoneCall *call, int cameraIndex, const char *cameraName)
{
	if(!call || !call->op)
		return;

	char callid[64] = {'\0'};
	snprintf(callid,63,"%d",call->op->cid);
	if(g_cbInterface.onNotifyGeneralEvent)
		g_cbInterface.onNotifyGeneralEvent(callid,G_EVENT_OpenCameraFailed, cameraName, cameraIndex);
}

void videoCaptureStatus(class ServiceCore *lc, SerPhoneCall *call, int status)
{
	if(!call || !call->op)
		return;

	char callid[64] = {'\0'};
	snprintf(callid,63,"%d",call->op->cid);
	if(g_cbInterface.onNotifyGeneralEvent)
		g_cbInterface.onNotifyGeneralEvent(callid,G_EVENT_VideoCaptureStatus, "", status);
}

void videoPacketTimeOut(class ServiceCore *lc, SerPhoneCall *call, int type)
{
	if(!call || !call->op)
		return;

	char callid[64] = {'\0'};
	snprintf(callid,63,"%d",call->op->cid);

	if(g_cbInterface.onNotifyGeneralEvent)
		g_cbInterface.onNotifyGeneralEvent(callid,G_EVENT_VideoPacketTimeOut, "", type);
}
void requestSpecifiedVideoFailed(class ServiceCore *lc, SerPhoneCall *call, const char *sip, int reason)
{
    if (!g_cbInterface.onRequestSpecifiedVideoFailed) {
        return;
    }
    if (call) {
        g_cbInterface.onRequestSpecifiedVideoFailed(call->_user_call_id,sip,reason);
    }
}

void stopSpecifiedVideoResponse(class ServiceCore *lc, SerPhoneCall *call, const char *sip, int response, void *window)
{
	if (!g_cbInterface.onStopSpecifiedVideoResponse) {
		return;
	}
	if (call) {
		g_cbInterface.onStopSpecifiedVideoResponse(call->_user_call_id,sip,response, window);
	}
}

void remoteVideoRatioChanged(SerPhoneCall *call, int width, int height, bool isVideoConference, const char *sipNo)
{
	if (!g_cbInterface.onRemoteVideoRatioChanged) {
		return;
	}
	if (call) {
		g_cbInterface.onRemoteVideoRatioChanged(call->_user_call_id, width, height, isVideoConference, sipNo);
	}else
	{// VideoConference for only manage type,no makecall. by zhangjunliang 20141022
		g_cbInterface.onRemoteVideoRatioChanged("", width, height, isVideoConference, sipNo);
	}
}

extern "C" void eXosipThreadStop()
{
    PrintConsole("[DEBUG] sean current servicecore:%p",g_pSerCore);
    g_cbInterface.oneXosipThreadStop();
}


static void* iterate_fun(void *p)
{
	while(g_Iterate)
	{
        if(g_pSerCore) {
#ifdef WIN32
            g_pSerCore->serphone_check_pre_after_ring_timeout();
#endif
            static int local_count = 0;
            local_count++;
            if (local_count % 100 == 0) {
//                PrintConsole("[DEBUG] sean g_hasRegistered:%s, g_RegisterTimerFlag:%s",g_hasRegistered?"true":"false",g_RegisterTimerFlag==1?"true":"false");
            }

            if (local_count % 1000 == 0) { //To validate thread is active
                if (traceFile) {
                    time_t temp = time(NULL);
                    std::string strTime = timetodate(temp);
                    fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                    char tempp[200] = {0};
                    sprintf(tempp, " sean g_hasRegistered:%s, g_RegisterTimerFlag:%s\n",g_hasRegistered?"true":"false",g_RegisterTimerFlag==1?"true":"false");
                    fwrite(tempp, strlen(tempp), 1, traceFile);
                    fflush(traceFile);
                }
            }
            if (local_count > 10000) {
                local_count = 0;
            }

            if ((g_hasRegistered && (g_RegisterTimerFlag == 0) && (time(NULL) - g_RegisterTimerB > 60)) || g_RegisterErrorCount > 25) {
                if (traceFile) {
                    time_t temp = time(NULL);
                    std::string strTime = timetodate(temp);
                    fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                    char tempp[200] = {0};
                    sprintf(tempp, " g_RegisterTimerFlag = 1 in func iterate_fun, g_NetworkType = %d\n",g_NetworkType);
                    PrintConsole(" g_RegisterTimerFlag = 1 in func iterate_fun, g_NetworkType = %d\n",g_NetworkType);
                    fwrite(tempp, strlen(tempp), 1, traceFile);
                    fflush(traceFile);
                }
                g_RegisterTimerFlag = 1;
                if (!g_cbInterface.oneXosipThreadStop) {
                    if (traceFile) {
                        time_t temp = time(NULL);
                        std::string strTime = timetodate(temp);
                        fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                        fwrite(" !g_cbInterface.oneXosipThreadStop not implemented\n", strlen(" !g_cbInterface.oneXosipThreadStop not implemented\n"), 1, traceFile);
                        fflush(traceFile);
                    }
                }
                if (g_cbInterface.oneXosipThreadStop && g_NetworkType != NETWORK_NONE) {

                    if (traceFile) {
                        time_t temp = time(NULL);
                        std::string strTime = timetodate(temp);
                        fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                        char tempp[200] = {0};
                        PrintConsole(" g_RegisterErrorCount = %d\n",g_RegisterErrorCount);
                        sprintf(tempp, " g_RegisterErrorCount = %d\n",g_RegisterErrorCount);

                        fwrite(tempp, strlen(tempp), 1, traceFile);
                        fflush(traceFile);
                    }

                    if (traceFile) {
                        time_t temp = time(NULL);
                        std::string strTime = timetodate(temp);
                        fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                        char tempp[200] = {0};
                        sprintf(tempp, " reinit eXosip called\n");

                        fwrite(tempp, strlen(tempp), 1, traceFile);
                        fflush(traceFile);
                    }
                    g_RegisterErrorCount = 0;
                    g_cbInterface.oneXosipThreadStop();
                }
            }
			g_pSerCore->serphone_core_iterate();
        }
	}
	g_IterateRuning = false;
	PrintConsole("iterate_fun end");
	return (void*)0;
}

int findCall( const char* callid, SerPhoneCall** ppCall)
{
	if(callid == NULL)
	{
		return ERR_INVALID_CALLID;
	}
    /*update begin------------------Sean20130729----------for video ice------------*/
//	int cid = atoi(callid);
//    PrintConsole("[APICall] findCall cid=%d\n", cid);
//	*ppCall = g_pSerCore->serphone_core_find_call_by_cid(cid);
    PrintConsole("[APICall] findCall cid=%s\n", callid);
    *ppCall = g_pSerCore->serphone_core_find_call_by_user_cid(callid);

    /*update end--------------------Sean20130729----------for video ice------------*/
	if( !*ppCall) {
		return ERR_INVALID_CALLID;
	}

	return 0;
}

int parseUrl( const char *loginUrl, char* host, short* port, char* resource)
{
	if( !loginUrl || !port || !resource )
		return -1;
	int state = 0 ;
	char cport[32] = {0};
	const char *ptr = loginUrl ,*begin = loginUrl;
	strcpy(cport,"80");
	while( *ptr != '\0'  ) {
		if( *ptr == ':'  ) {
			if( state == 0 )  {
				if (*(ptr+1)=='/' && *(ptr+2)=='/')
				{
					begin = ptr+3;
					ptr = begin;
					state = 1;	//begin for host
				}
				else {
					strncpy(host,begin,ptr-begin);
					host[ptr-begin] = 0;
					begin = ptr+1;
					state = 2;
				}
			}
			else if( state == 1) {
				strncpy(host,begin, ptr-begin);
				host[ptr-begin] = 0;
				begin = ptr+1;
				ptr = begin;
				state = 2;	//begin for port
			}
			else
				return -1;
		}
		else if( *ptr == '/') {
			if( state == 0  || state == 1) {
				strncpy(host,begin, ptr-begin);
				host[ptr-begin] = 0;
				strncpy(resource,ptr,256);
				*port = atoi(cport);
				return 0;
			}
			else if( state == 2) {
				strncpy(cport,begin,ptr-begin);
				*port = atoi(cport);
				strncpy(resource,ptr,256);
				return 0;
			}
			else
				return -1;
		}
		ptr++;
	}
	if( state == 0 || state==1 )
	{
		strcpy(host,loginUrl);
	}
	else if (state == 2 ) {
		strncpy(cport,begin,ptr-begin);

	}
	strcpy(resource,"/");
	*port = atoi(cport);

	return 0;
}

int synHttpRequest(char *host, int port, char *message, char *response , int buflen)
{
	int sock ;
	char cport[32]={0};
	struct addrinfo *result = NULL;
    struct addrinfo hints;
	sprintf(cport,"%d",port);
	memset( &hints,0,sizeof(addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_protocol=IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;

	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		return -1;
	}
	if( getaddrinfo(host,cport, NULL, &result) != 0) {
		return -1;
	}
	if(connect(sock, result->ai_addr, result->ai_addrlen) < 0){
		freeaddrinfo( result);
		return -1;
	}
	freeaddrinfo( result);
  //Send the query to the server
	int recvlen=0,sendlen = 0 ,len = 0;
	while(sendlen < (int)strlen(message))
	{
		 len = send(sock, message+sendlen, strlen(message)-sendlen, 0);
		if(len == -1){
			return -1;
		}
		sendlen += len;
	}
	memset(response, 0,buflen);
	while((len = recv(sock, response+recvlen, buflen-recvlen, 0)) > 0){
		if(  strstr(response+recvlen-4, "\r\n\r\n") == NULL ) {
			if( recvlen >  buflen ) {
				 return -1;
			}
			else {
				break;
			}
		}
		recvlen += len;
	}
	PrintConsole("%s\n", response);
#ifdef WIN32
	closesocket(sock);
#else
	close(sock);
#endif
  return 0;
}


void receiverStatsReceived(class ServiceCore *lc, const char *callid, const int framerate, const int bitrate)
{
	if( !g_cbInterface.onReceiverStats )
		return;
	static char callidl[64];
	memcpy(callidl, callid, 9);
	g_cbInterface.onReceiverStats( callid, framerate, bitrate);
	return;
}

void receiverCodecChanged(class ServiceCore *lc, const char *callid, const int width, const int height)
{
	if (!g_cbInterface.onIncomingCodecChanged)
	{
		return;
	}
	static char callidl[64];
	memcpy(callidl, callid, 9);
	g_cbInterface.onIncomingCodecChanged( callid, width, height);
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" int initialize( CCallbackInterface *cbInterface)
{
    const char *platform ="unknow",*arch = "arm",*voice="voice=false",*video="video=false";
#ifdef __ANDROID__
    platform = "Android";
#elif defined WIN32
    platform = "Windows";
#elif defined __APPLE__
#if TARGET_OS_IPHONE
    platform ="iOS";
#else
    platform ="Mac OS";
#endif
#elif __linux
    platform ="Linux";
#endif
#if defined WEBRTC_ARCH_ARM_V7A
    arch = "armv7";
#elif defined WEBRTC_ARCH_ARM_V5
    arch = "armv5";
#endif

#ifndef NO_VOIP_FUNCTION
    voice="voice=true";
#ifdef VIDEO_ENABLED
    video="video=true";
#endif
#endif

	PrintConsole("[APICall] initialize\n");

    if (traceFile) {
        time_t temp = time(NULL);
        std::string strTime = timetodate(temp);
        fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
        fwrite(" [APICall] initialize\n", strlen(" [APICall] initialize\n"), 1, traceFile);
        fflush(traceFile);
    }

	PrintConsole("[APICall] CCPClient version %s for %s(%s) %s Build#%s %s\n",
			CCP_SDK_VERSION,platform,arch,video,__DATE__,__TIME__);
	sprintf(gVersionString,"%s#%s#%s#%s#%s#%s %s",CCP_SDK_VERSION,platform,arch,voice,video,__DATE__,__TIME__);
	if(!cbInterface )
    {
        if (traceFile) {
            time_t temp = time(NULL);
            std::string strTime = timetodate(temp);
            fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
            fwrite(" ERR_INVALID_CALL_BACK_INTERFACE\n", strlen(" ERR_INVALID_CALL_BACK_INTERFACE\n"), 1, traceFile);
            fflush(traceFile);
        }
		return ERR_INVALID_CALL_BACK_INTERFACE;
    }

	if( g_pSerCore)
		return  ERR_SDK_ALREADY_INIT;
	memcpy( &g_cbInterface, cbInterface, sizeof(CCallbackInterface));

	SerphoneCoreVTable l_vTable;
	memset(&l_vTable,0,sizeof(SerphoneCoreVTable));
	l_vTable.global_state_changed = mainStateChanged;
	l_vTable.call_state_changed = callStateChanged;
	l_vTable.registration_state_changed = registrationStateChanged;
	l_vTable.text_received = recvTextMessage;
	l_vTable.dtmf_received = dtmfReceived;
	l_vTable.text_send_report = textSendReport;
    l_vTable.media_init_failed = mediaInitFailed;
    l_vTable.deliver_video_frame = deliverVideoFrame;
    l_vTable.record_audio_callback  = recordAudioCallBack;
    l_vTable.throw_data_2_process = throwDataToProcessCallBack;
	l_vTable.video_capture_status = videoCaptureStatus;
	l_vTable.video_packet_timeout = videoPacketTimeOut;
    l_vTable.throw_original_data_2_process = throwOriginalDataToProcessCallBack;
    l_vTable.transfer_state_changed = transferCallStateChangedCallBack;
    l_vTable.connect_camera_failed = connectCameraFailed;
    l_vTable.request_specified_video_failed = requestSpecifiedVideoFailed;
	l_vTable.stop_specified_video_response = stopSpecifiedVideoResponse;
    l_vTable.remote_video_ratio_changed = remoteVideoRatioChanged;
#ifdef HAIYUNTONG
    l_vTable.init_haiyuntong_failed = initHaiyuntongFailed;
#endif
    l_vTable.eXosip_thread_stop = eXosipThreadStop;

	l_vTable.receiver_stats_received = receiverStatsReceived;
	l_vTable.receiver_codec_changed = receiverCodecChanged;

	//g_pSerCore = serphone_core_new(&l_vTable,"phone.ini",NULL,NULL);

	g_pSerCore = serphone_core_new(&l_vTable,NULL,NULL,NULL);
	if( !g_pSerCore)  {
        if (traceFile) {
            time_t temp = time(NULL);
            std::string strTime = timetodate(temp);
            fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
            fwrite(" ERR_NO_MEMORY\n", strlen(" ERR_NO_MEMORY\n"), 1, traceFile);
            fflush(traceFile);
        }
		return ERR_NO_MEMORY;
	}

    PrintConsole("[DEBUG] sean new serviecore:%p",g_pSerCore);

    if (traceFile) {
        time_t temp = time(NULL);
        std::string strTime = timetodate(temp);
        fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
        char tempp[200] = {0};
        sprintf(tempp, " [DEBUG] sean new serviecore:%p\n",g_pSerCore);

        fwrite(tempp, strlen(tempp), 1, traceFile);
        fflush(traceFile);
    }

	/*if( cbInterface->onGetCapabilityToken) {
		PrintConsole("[APICall] onGetCapabilityToken\n");
		cbInterface->onGetCapabilityToken();
	}*/

    g_Iterate = true;
	g_IterateRuning = true;
	cc_createthread(g_Thread,iterate_fun,0);
	sal_set_keepalive_period(g_pSerCore->sal,g_keepAliveTime);

    if( g_NetworkType == NETWORK_GPRS ) {
         //60ms audio packetsize
        g_pSerCore->serphone_core_set_audio_pacinterval(60);
    } else {
         //20ms
        g_pSerCore->serphone_core_set_audio_pacinterval(60);
    }

    gProxyAddrLst = new proxyAddrList;
    gProxyAddrLst->next = NULL;
    gProxyAddrLst->addr = NULL;
    gProxyAddrLst->account = NULL;
    gProxyAddrLst->passwd = NULL;
    gProxyAddrLst->capability = NULL;
    gProxyAddrLst->port = 0;
    return 0;
}
/*
extern "C"  int loginToCCP(const char *rest_addr, int rest_port,
					const char *subaccount,  const char *subaccountpwd, const char *sipid,const char *sippwd)
{
#ifdef WIN32
	std::string ip = "";
	int port =0 ;
	if( rest_port == 0 )
		rest_port = 8883;

	TRESTClient restClient(rest_addr,rest_port,"","","");
	if( !restClient.GetLoginServer(subaccount,sipid,subaccountpwd, ip,port) ) {
		return ERR_GET_SOFTSWITCH_ADDRESS;
	}
	return connectToCCP(ip.c_str(),port,sipid,sippwd);
#else
	return ERR_NOT_SUPPORT;
#endif

}
*/

extern "C"  int connectToCCP(const char *proxy_addr, int proxy_port,
					const char *account, const char *password, const char *capability)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	if( !proxy_addr || !account ||!password)
    {
        if (traceFile) {
            time_t temp = time(NULL);
            std::string strTime = timetodate(temp);
            fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
            fwrite(" connectToCCP ERR_INVALID_PARAM\n", strlen(" connectToCCP ERR_INVALID_PARAM\n"), 1, traceFile);
            fflush(traceFile);
        }
		return ERR_INVALID_PARAM;
    }

	PrintConsole("[APICall] Connect to CCP host(%s),port(%d),account(%s),password(****),capability(%s) \n",
		proxy_addr,proxy_port,account,capability);
	g_bConnected =false;

	if( proxy_port != 0) {
		g_pSerCore->serphone_set_reg_info(proxy_addr,proxy_port,account,password,gDisplayName, capability, gToken);
		return 0;
	}
	return ERR_INVALID_PARAM;
}

extern "C" int connectToCCPWithXML(const char *addressXML, const char *account, const char *password, const char *capability)
{

    if (traceFile) {
        time_t temp = time(NULL);
        std::string strTime = timetodate(temp);
        fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
        fwrite(" connectToCCPWithXML\n", strlen(" connectToCCPWithXML\n"), 1, traceFile);
        fflush(traceFile);
    }
    //解析xml，生成proxyList
    cloopenwebrtc::tinyxml2::XMLDocument doc;
    if( doc.Parse(addressXML)!= 0) {
        PrintConsole("WARNING: connectToCCPWithMultiAddress ERROR: INVALID XML\n");
        if (traceFile) {
            time_t temp = time(NULL);
            std::string strTime = timetodate(temp);
            fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
            fwrite(" WARNING: connectToCCPWithMultiAddress ERROR: INVALID XML\n", strlen(" WARNING: connectToCCPWithMultiAddress ERROR: INVALID XML\n"), 1, traceFile);
            fflush(traceFile);
        }
		return -1;
	}
    cloopenwebrtc::tinyxml2::XMLElement* rootElement = doc.RootElement();
    int statusCode = atoi(rootElement->FirstChildElement("statusCode")->GetText());
    if (statusCode) {
        PrintConsole("WARNING: connectToCCPWithMultiAddress ERROR: WRONG STATUS CODE\n");
        if (traceFile) {
            time_t temp = time(NULL);
            std::string strTime = timetodate(temp);
            fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
            fwrite(" WARNING: connectToCCPWithMultiAddress ERROR: WRONG STATUS CODE\n", strlen(" WARNING: connectToCCPWithMultiAddress ERROR: WRONG STATUS CODE\n"), 1, traceFile);
            fflush(traceFile);
        }
        return -2;
    }
    cloopenwebrtc::tinyxml2::XMLElement* switchElement = rootElement->FirstChildElement("Switch");
    if (switchElement) {
        //将proxyAddrList清空
        proxyAddrList *lastAddr = gProxyAddrLst->next;
        while (lastAddr) {
            gProxyAddrLst->next = lastAddr->next;
            delete [] lastAddr->addr;
            lastAddr->addr = NULL;
            delete [] lastAddr->account;
            lastAddr->account = NULL;
            delete [] lastAddr->passwd;
            lastAddr->passwd = NULL;
            delete [] lastAddr->capability;
            lastAddr->capability = NULL;
            delete lastAddr;
            lastAddr = gProxyAddrLst->next;
        }
        lastAddr = gProxyAddrLst;
        cloopenwebrtc::tinyxml2::XMLElement *clpssElement = switchElement->FirstChildElement("clpss");
        if (!clpssElement) {
            PrintConsole("WARNING: connectToCCPWithMultiAddress ERROR: NO IP\n");
            if (traceFile) {
                time_t temp = time(NULL);
                std::string strTime = timetodate(temp);
                fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                fwrite(" WARNING: connectToCCPWithMultiAddress ERROR: NO IP\n", strlen(" WARNING: connectToCCPWithMultiAddress ERROR: NO IP\n"), 1, traceFile);
                fflush(traceFile);
            }
            return -3;
        }
        while (clpssElement) {
            proxyAddrList *tempAddr = new proxyAddrList;
            cloopenwebrtc::tinyxml2::XMLElement *ipElement = clpssElement->FirstChildElement("ip");
            tempAddr->addr = new char[strlen(ipElement->GetText())+1];
            memcpy(tempAddr->addr, ipElement->GetText(), strlen(ipElement->GetText()));
            tempAddr->addr[strlen(ipElement->GetText())] = '\0';
            cloopenwebrtc::tinyxml2::XMLElement *portElement = clpssElement->FirstChildElement("port");
            tempAddr->port = atoi(portElement->GetText());
            tempAddr->account = new char[strlen(account)+1];
            memcpy(tempAddr->account, account, strlen(account));
            tempAddr->account[strlen(account)] = '\0';
            tempAddr->passwd = new char[strlen(password)+1];
            memcpy(tempAddr->passwd, password, strlen(password));
            tempAddr->passwd[strlen(password)] = '\0';
            tempAddr->capability = new char[strlen(capability)+1];
            memcpy(tempAddr->capability, capability, strlen(capability));
            tempAddr->capability[strlen(capability)] ='\0';
            tempAddr->next = NULL;
            lastAddr->next = tempAddr;
            lastAddr = tempAddr;
            clpssElement = clpssElement->NextSiblingElement("clpss");
        }
    }
    else
    {
        PrintConsole("WARNING: connectToCCPWithMultiAddress ERROR: NO IP111\n");
        if (traceFile) {
            time_t temp = time(NULL);
            std::string strTime = timetodate(temp);
            fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
            fwrite(" WARNING: connectToCCPWithMultiAddress ERROR: NO IP111\n", strlen(" WARNING: connectToCCPWithMultiAddress ERROR: NO IP111\n"), 1, traceFile);
            fflush(traceFile);
        }
        return -3;
    }
    proxyAddrList *cursorElement = gProxyAddrLst->next;
    gProxyAddrLst->next = cursorElement->next;
    connectToCCP(cursorElement->addr, cursorElement->port, cursorElement->account, cursorElement->passwd, cursorElement->capability);
    delete [] cursorElement->addr;
    cursorElement->addr = NULL;
    delete [] cursorElement->account;
    cursorElement->account = NULL;
    delete [] cursorElement->passwd;
    cursorElement->passwd = NULL;
    delete [] cursorElement->capability;
    cursorElement->capability = NULL;
    delete cursorElement;
    cursorElement = NULL;
    return 0;
}

extern "C"  int disConnectToCCP()
{
//	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
//#ifndef HAIYUNTONG
//	PrintConsole("[APICall] Disconnect to CCP\n");
//
//    if (traceFile) {
//        time_t temp = time(NULL);
//        std::string strTime = timetodate(temp);
//        fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
//        fwrite(" Disconnect to CCP\n", strlen(" Disconnect to CCP\n"), 1, traceFile);
//        fflush(traceFile);
//    }
//
//	g_bConnected =false;
//    g_pSerCore->serphone_core_enable_temp_auth(false);
//    if (traceFile) {
//        time_t temp = time(NULL);
//        std::string strTime = timetodate(temp);
//        fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
//        fwrite(" Something goes wrong Unregister called1\n", strlen(" Something goes wrong Unregister called1\n"), 1, traceFile);
//        fflush(traceFile);
//    }
//    g_pSerCore->serphone_proxy_remove(NULL);
//#endif

	return 0;
}

extern "C"  int setUserName(const char*username)
{
	if(!username)
		return ERR_INVALID_PARAM;
	PrintConsole("[APICall] setUserName (%s) \n",username);
	memcpy(gDisplayName,username,sizeof(gDisplayName));
	return 0;
}

extern "C"  const char* makeCall(int callType, const char *called )
{

	//hubintest 
	int channelID;
	if (strncmp(called, "1906", 4) == 0) {
		startSendRtpPacket(channelID, "192.168.0.89", 7084);
		return "as";
	}
	else if (strncmp(called, "1907", 4) == 0) {
		startSendRtpPacket(channelID, "192.168.0.89", 7086);
		return "as";
	}
	else if (strncmp(called, "1908", 4) == 0) {
		startSendRtpPacket(channelID, "192.168.0.89", 7088);
		return "as";
	}

	//end

    if(!g_bConnected)
    {
        PrintConsole("[APICall] makeCall (type=%s, called=%s ) Failed! unConnected\n",
                     callType == VOICE_CALL ? "voice": "video",called);
        return NULL;
    }//设置一个回调，通知上层若是加密电话，则设置响应参数
    if (g_cbInterface.onEnableSrtp) {
        g_cbInterface.onEnableSrtp(g_pSerCore->serphone_get_self_sipNo(),true);
    }


	if(called == NULL)
	{
		return NULL;
	}
	PrintConsole("[APICall] makeCall (type=%s, called=%s ) \n",
		callType == VOICE_CALL ? "voice": "video",called);


    if(strncmp(called, "8", 1) == 0 && strlen(called) >= 14 ) {
        g_pSerCore->serphone_core_set_audio_pacinterval(60);
    }
    else {
        g_pSerCore->serphone_core_set_audio_pacinterval(20);
    }

	static char callid[64] = {'\0'};
	SerPhoneCall *pCall;
	SDK_UN_INITIAL_ERROR(NULL);

	if( callType == VIDEO_CALL)
		g_pSerCore->video_policy.automatically_initiate = true;
	else
		g_pSerCore->video_policy.automatically_initiate = false;

	pCall = g_pSerCore->serphone_core_invite(called, gUserDataForInvite);
    g_pSerCore->_terminate_call = 0;
	if(!pCall )
		return NULL;
	SalOp *sop = pCall->op;
	if( !sop)
		return NULL;

    /*update begin------------------Sean20130729----------for video ice------------*/
//	sprintf(callid,"%d",sop->cid);
    memcpy(callid, pCall->_user_call_id, 8);
    callid[8] = '\0';
    /*update begin------------------Sean20130729----------for video ice------------*/
#ifdef WIN32
    g_pSerCore->serphone_pre_after_ring_stop(true);
    g_pSerCore->serphone_pre_after_ring_stop(false);
    g_pSerCore->serphone_prering_start();
#endif
	return callid;
}
extern "C" int setVideoView(void* view,void *localView)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    g_pSerCore->m_videoWindow = view;
    g_pSerCore->localVideoWindow = localView;
	return 0;
}
extern "C" int resetVideoViews(const char *callid, void* rWnd, void* lWnd)
{
	PrintConsole("[APICall] changeVideoWindows (callid=%s ) rWnd=%0x lWnd=%0x\n",callid ? callid  :"null", rWnd, lWnd);
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	if(!rWnd && !lWnd)
		return ERR_INVALID_PARAM;

	SerPhoneCall *pCall = NULL;
	findCall(callid,&pCall);
	return g_pSerCore->serphone_call_reset_video_views(pCall, rWnd, lWnd);
}

extern "C"  int acceptCall(const char *callid)
{
	PrintConsole("[APICall] acceptCall (callid=%s ) \n",callid ? callid  :"null");
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	if(!callid)
		return ERR_INVALID_PARAM;
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid,&pCall);
	if(ret != 0)
		return  ret;
	return g_pSerCore->serphone_core_accept_call(pCall);
}
extern "C"  int releaseCall(const char *callid , int reason)
{
	PrintConsole("[APICall] releaseCall (callid=%s ) \n",callid ? callid  :"null");
#ifdef WIN32
    g_pSerCore->serphone_pre_after_ring_stop(true);
    g_pSerCore->serphone_pre_after_ring_stop(false);
#endif
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	if( callid == NULL )
		return g_pSerCore->serphone_core_terminate_all_calls();

	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid,&pCall);
	if(ret != 0)
    {
		return  ret;
    }
    /*add begin------------------Sean20130729----------for video ice------------*/
    pCall->core->_terminate_call = 1;
    /*add end--------------------Sean20130729----------for video ice------------*/
    pCall->reason =  (SerphoneReason)reason;
	return g_pSerCore->serphone_core_terminate_call(pCall);
}


extern "C"  int rejectCall(const char *callid , int reason)
{
	PrintConsole("[APICall] rejectCall (callid=%s , reason=%d) \n",callid ? callid  :"null",reason);
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	if( callid == NULL )
		return g_pSerCore->serphone_core_terminate_all_calls();

	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid,&pCall);
	if(ret != 0)
		return  ret;
    /*add begin------------------Sean20130729----------for video ice------------*/
    pCall->core->_terminate_call = 1;
    /*add end--------------------Sean20130729----------for video ice------------*/
	pCall->reason =  (SerphoneReason)reason;
	return g_pSerCore->serphone_core_terminate_call(pCall);
}

extern "C"  int pauseCall(const char *callid)
{
	PrintConsole("[APICall] pauseCall (callid=%s ) \n",callid ? callid  :"null");
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid,&pCall);
	if(ret != 0)
    {
        PrintConsole("[APICall] pauseCall cannot find call\n");
		return  ret;
    }
    /*add begin------------------Sean20130729----------for video ice------------*/
    pCall->core->_terminate_call = 1;
    /*add end--------------------Sean20130729----------for video ice------------*/
	return g_pSerCore->serphone_core_pause_call(pCall);
}

extern "C"  int resumeCall(const char *callid)
{
	PrintConsole("[APICall] resumeCall (callid=%s ) \n",callid ? callid  :"null");
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid,&pCall);
	if(ret != 0)
		return  ret;
	return g_pSerCore->serphone_core_resume_call(pCall);}

extern "C"  int transferCall(const char *callid , const char *destination, int type)
{
	PrintConsole("[APICall] transferCall (callid=%s ) \n",callid ? callid  :"null");
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid,&pCall);
	if(ret != 0)
	{
		PrintConsole("WARNNING: cannot find call specified by callid(%s), return = %d\n",callid,ret);
		return  ret;
	}
	if (LinphoneCallConnected != pCall->state && LinphoneCallStreamsRunning != pCall->state)
	{
		PrintConsole("WARNNING: call cannot be transfered before answered!\n");
		return -1;
	}

    if(-1 == g_pSerCore->serphone_set_referTo(destination))
    {
        PrintConsole("[ERROR] destination is NULL");
        return -1;
    }
    g_pSerCore->serphone_set_isRefering(true);
	return g_pSerCore->serphone_core_transfer_call(pCall,destination, type);
}


extern "C" int acceptCallByMediaType(const char *callid, int type)
{
    PrintConsole("[APICall] acceptCallByMediaType (callid=%s ) type:%d\n",callid ? callid  :"null", type);
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    SerPhoneCall *pCall = NULL;
    int ret = findCall(callid, &pCall);
    if(ret != 0)
        return ret;

    PrintConsole("[APICall] acceptCallByMediaType");

    SerphoneCallParams *params = serphone_call_params_copy(&pCall->params);
    params->has_video = type==1?true:false;
    if(g_pSerCore)
        ret = g_pSerCore->serphone_core_accept_call_with_params(pCall, params);
    else ret = -1;
    serphone_call_params_destroy(params);

    PrintConsole("[APICall] acceptCallByMediaType ret=%d\n", ret);

    return ret;
}

extern "C" int  updateCallMedia(const char *callid, int request)
{
    PrintConsole("[APICall] updateCallMedia (callid=%s ) request:%d\n",callid ? callid  :"null", request);
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    SerPhoneCall *pCall = NULL;
    int ret = findCall(callid, &pCall);
    if(ret != 0)
        return ret;

    SerphoneCallParams *params=serphone_call_params_copy(&pCall->current_params);
    params->has_video = request==1?true:false;
    if(g_pSerCore)
    {
        ret = g_pSerCore->serphone_core_update_call(pCall, params);
    }
    else ret = -1;
    serphone_call_params_destroy (params);

    return ret;
}

extern "C" int answerCallMediaUpdate(const char *callid, int action)
{
    PrintConsole("[APICall] answerCallMediaUpdate (callid=%s ) action:%d\n",callid ? callid  :"null", action);
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    SerPhoneCall *pCall = NULL;
    int ret = findCall(callid, &pCall);
    if(ret != 0)
        return ret;

    SerphoneCallParams *params=serphone_call_params_copy(&pCall->params);
    params->has_video = action==1?true:false;
    if(g_pSerCore)
    {
        ret = g_pSerCore->serphone_core_accept_call_update(pCall, params);
    }
    else ret = -1;
    serphone_call_params_destroy (params);

    return ret;
}

extern "C" const char *getCurrentCall()
{
//	PrintConsole("[APICall] getCurrentCall \n");
	SDK_UN_INITIAL_ERROR(NULL);
	static char callid[64]={0};
	if( ! g_pSerCore->current_call)
		return NULL;
	snprintf(callid,63,"%s",g_pSerCore->current_call->_user_call_id);
	return callid;
}

extern "C" int getCallMediaType(const char *callid)
{
    PrintConsole("[APICall] getCallMediaType callid=%s\n", callid);
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    SerPhoneCall *pCall = NULL;
    int ret = findCall(callid, &pCall);
    if(ret != 0)
        return -1;
    if(pCall->params.has_video)
        return VIDEO_CALL;
    else
        return VOICE_CALL;

    return 0;
}

//?y????callidδ???. ???????????call????DTMF.
extern "C"  int sendDTMF(const char *callid, const char dtmf)
{
	PrintConsole("[APICall] sendDTMF (callid=%s,dtmf=%c ) \n",callid ? callid  :"null",dtmf);
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	if(!callid)
		return ERR_INVALID_PARAM;
	g_pSerCore->serphone_core_send_dtmf(dtmf);
	return 0;

}

extern "C"  const char *sendTextMessage(const char *receiver, const char *message, const char *userdata)
{
	PrintConsole("[APICall] sendTextMessage (receiver=%s,message length=%d, userdata length=%d) \n",
		receiver ? receiver  : "null",
		message ? strlen(message) : 0,
        userdata ? strlen(userdata):0 );

	if(!g_pSerCore || !receiver || !message)
		return "";
    char *bufPtr = NULL;
#ifdef HAIYUNTONG
    //call message encrypt
    char messageEncrypt[4096] = {0};

    if (g_pSerCore->serphone_haiyuntong_enabled()) {
        long messageEncryptLen;
        int ret;
        if (receiver[0] == 'g') {
            //group message
            ret = g_pSerCore->serphone_group_sms_encrypt((char *)message, strlen(message), (char *)receiver, strlen(receiver), messageEncrypt, &messageEncryptLen);
        }
        else
        {
            ret = g_pSerCore->serphone_sms_encrypt((char *)message, strlen(message), (char *)receiver, strlen(receiver), messageEncrypt, &messageEncryptLen);
        }
        if (0 != ret) {
            char errorCode[64]={0};
            sprintf(errorCode, "%d", ret);
            return  errorCode;
        }
        messageEncrypt[messageEncryptLen] = '\0';
        PrintConsole("[DEBUG HAIYUNTONG] encrypted message:%s",messageEncrypt);
        int bufLen = 64+messageEncryptLen;
        if(userdata)
            bufLen += strlen(userdata);
        bufPtr = (char*)ms_malloc(bufLen);
        if (!bufPtr) {
            return "";
        }

        if(userdata)
            sprintf(bufPtr, "type=0\r\n<<<%s>>>\r\n%s", userdata, messageEncrypt);
        else
            sprintf(bufPtr, "type=0\r\n%s",messageEncrypt);
    }
    else
    {
        int bufLen = 64+strlen(message);
        if(userdata)
            bufLen += strlen(userdata);
        bufPtr = (char*)ms_malloc(bufLen);
        if (!bufPtr) {
            return "";
        }

        if(userdata)
            sprintf(bufPtr, "type=0\r\n<<<%s>>>\r\n%s", userdata, message);
        else
            sprintf(bufPtr, "type=0\r\n%s",message);
    }

#else
    int bufLen = 64+strlen(message);
    if(userdata)
        bufLen += strlen(userdata);
    bufPtr = (char*)ms_malloc(bufLen);
    if (!bufPtr) {
        return "";
    }

    if(userdata)
        sprintf(bufPtr, "type=0\r\n<<<%s>>>\r\n%s", userdata, message);
    else
        sprintf(bufPtr, "type=0\r\n%s",message);
#endif



    PrintConsole("[APICall] sendTextMessage");

    const char *callid = g_pSerCore->serphone_core_send_text(receiver,bufPtr);

    ms_free((void **)&bufPtr);
    bufPtr = NULL;
    return callid;
}

extern "C"	int	enableLoudsSpeaker(bool enable)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall] enableLoudsSpeaker (enable=%s) \n",enable ? "YES":"NO");
	return g_pSerCore->serphone_set_louds_speaker_status(enable);
}

extern "C"	int	enableGlobalAudioInDevice(bool enable)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall] enableGlobalAudioInDevice (enable=%s) \n",enable ? "YES":"NO");
	return g_pSerCore->serphone_set_global_audio_in_device(enable);
}
extern "C" bool getLoudsSpeakerStatus()
{
	PrintConsole("[APICall] getLoudsSpeakerStatus \n");
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	 if( g_pSerCore->serphone_get_louds_speaker_status() )
		 return true;
	 else
		 return false;
}
extern "C" int setMute(bool on)
{
	PrintConsole("[APICall] setMute (on=%s) \n",on ? "YES":"NO");
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->serphone_set_mute_status(on);
}

extern "C" bool getMuteStatus()
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	if( g_pSerCore->serphone_get_mute_status() )
		return true;
	else
		return false;
}

extern "C" int setSpeakerMute(bool on)
{
	PrintConsole("[APICall] setSpeakerMute (on=%s) \n",on ? "YES":"NO");
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->serphone_set_speaker_mute_status(on);
}

extern "C" bool getSpeakerMuteStatus()
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	if( g_pSerCore->serphone_get_speaker_mute_status() )
		return true;
	else
		return false;
}

extern "C" bool getMuteStatusSoft(const char* callid)
{
    PrintConsole("[APICall] getMuteStatusSoft callid=%s\n", callid);
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    SerPhoneCall *pCall = NULL;
    int ret = findCall(callid, &pCall);
    if(ret != 0)
        return false;
    return g_pSerCore->serphone_soft_get_mute_status(pCall);
}

extern "C" int setMuteSoft(const char*callid, bool on)
{
    PrintConsole("[APICall] setMuteSoft callid=%s,on=%d\n", callid,on);
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    SerPhoneCall *pCall = NULL;
    int ret = findCall(callid, &pCall);
    if(ret != 0)
        return -1;
    return g_pSerCore->serphone_soft_mute(pCall, on);
}

extern "C" int setRing(const char* filename)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	g_pSerCore->serphone_core_set_ring(filename);
	return 0;
}

extern "C" int setRingback(const char*filename)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	g_pSerCore->serphone_core_set_ringback(filename);
	return 0;
}


extern "C" int setPreRing(const char* filename)
{
    g_pSerCore->serphone_core_set_prering(filename);
    return 0;
}

extern "C" int setAfterRing(const char* filename)
{
    g_pSerCore->serphone_core_set_afterring(filename);
    return 0;
}

#if 0
extern "C" int openTraceFile(const char *filePath)
{
    if (NULL == filePath) {
        PrintConsole("[APICall WARNNING] %s filePath is NULL");
        return -1;
    }
    globalFilePath = (char *)malloc(strlen(filePath)+1);
    memcpy(globalFilePath, filePath, strlen(filePath));
    globalFilePath[strlen(filePath)] = '\0';
    return 0;
}

extern "C" int openTraceFile2(const char *filePath)
{
    if (NULL == filePath) {
        PrintConsole("[APICall WARNNING] %s filePath is NULL");
        return -1;
    }
    globalFilePath2 = (char *)malloc(strlen(filePath)+1);
    memcpy(globalFilePath2, filePath, strlen(filePath));
    globalFilePath2[strlen(filePath)] = '\0';
    return 0;
}
#endif

extern "C" int setLogLevel(int level)
{
	//if(level >= LOG_LEVEL_NONE && level < LOG_LEVEL_END ) {
	//	gDebugLevel = level;
	//	return 0;
	//}
    return -1;
}

extern "C" void setTraceFlag(bool enable)
{
	ECMedia_set_trace(NULL, NULL, 23, 100);
	ServiceCore::serphone_set_traceFlag();
}

extern "C" void setAudioRecordStatus(const char *path, bool enable)
{
    g_pSerCore->serphone_set_audioRecordStatus(path, enable);
}

extern "C" void setCapabilityToken(const char *token)
{
	PrintConsole("[APICall] setCapabilityToken (token=%s) \n",token ? token:"NULL");
	/*if( token && token[0] ) {
		char capability[32] = {0};
		DecodeAuthToken(token,KEY_TOKEN[0],capability);
		PrintConsole("decode token is  %s\n",capability);
	}*/
}

extern "C" int setUserData(int type, const char *data)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall] setUserData (type=%d ,data=%s) \n",type, data ? data:"NULL");
	switch( type)
	{
	case USERDATA_FOR_TOKEN:
        {
            strncpy(gToken,data,sizeof(gToken));
            if (g_pSerCore) {
                g_pSerCore->serphone_core_enable_temp_auth(true);
            }
        }

		break;

	case USERDATA_FOR_INVITE:
			if(gUserDataForInvite)
				ms_free((void**)&gUserDataForInvite);
			gUserDataForInvite = ms_strdup(data);
		break;

	case USERDATA_FOR_USER_AGENT:
		if(g_pSerCore)
			g_pSerCore->apply_user_agent(data);
		break;
	default:
		return ERR_NOT_SUPPORT;
	}
	return 0;

}
extern "C" int getUserData(int type, char* buffer,int  buflen)
{
	PrintConsole("[APICall] getUserData (type=%d)\n",type);
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	switch( type)
	{

	case USERDATA_FOR_INVITE:
		if(gRecvUserData) {
			strncpy(buffer,gRecvUserData,buflen);
			buffer[buflen-1] = 0;
		} else {
			buffer[0] = '\0';
		}
		break;
	default:
		return ERR_NOT_SUPPORT;
	}
	PrintConsole("[APICall] getUserData (type=%d) return(%s)\n",type,buffer);
	return 0;
}

extern "C" int getCallState(const char *callid)
{
	return -1;
}

extern "C" int unInitialize()
{
    PrintConsole("[DEBUG] sean %s called, serviecore:%p",__FUNCTION__,g_pSerCore);
	PrintConsole("[APICall] unInitialize\n");
	g_Iterate =false;
#if 0
    if (globalFilePath) {
        free(globalFilePath);
        globalFilePath = NULL;
    }
    if (globalFilePath2) {
        free(globalFilePath2);
        globalFilePath2 = NULL;
    }
#endif
	int waitTimes = 30;   //total 3s
	while (g_IterateRuning && waitTimes > 0)
	{
		PrintConsole("[APICall] unInitialize waiting iterate stop\n");
		Sleep(100);
		waitTimes--;
	}

	//Sleep(1000);
	if (g_pSerCore)
	{
		serphone_core_destroy(g_pSerCore);
		g_pSerCore = NULL;
	}
	g_bConnected = false;
	PrintConsole("[APICall] unInitialize finished\n");
    if (gProxyAddrLst) {
        proxyAddrList *temp = gProxyAddrLst->next;
        while (temp) {
            gProxyAddrLst->next = temp->next;
            delete [] temp->addr;
            temp->addr = NULL;
            delete [] temp->account;
            temp->account = NULL;
            delete [] temp->passwd;
            temp->passwd = NULL;
            delete [] temp->capability;
            temp->capability = NULL;
            delete temp;
            temp = gProxyAddrLst->next;
        }
        delete gProxyAddrLst;
		gProxyAddrLst = NULL;
    }
	if(gUserDataForInvite)
		ms_free((void**)&gUserDataForInvite);
	if(gRecvUserData)
		ms_free((void**)&gRecvUserData);
    return 0;
}
extern "C" int setAndroidObjects(void* javaVM, void* env, void* context)
{
    PrintConsole("[APICall] setAndroidObjects javaVM=%0x, env=%0x, context=%0x", javaVM, env,
    context);
    serphone_core_set_android_objects(javaVM,env,context);
	return 0;
}
extern "C" void sendKeepAlive()
{
	sal_send_keep_alive();
}

extern "C" void setNetworkType(int networktype,bool connected,bool reconnect)
{
	const char *p = "unkonw";
	const char* type[] = { "NONE","LAN","WIFI","GPRS","3G"};
	if( (unsigned int)networktype < sizeof(type)/sizeof(char*))
		p = type[networktype];

    g_NetworkType = networktype;
	PrintConsole("[APICall] setNetworkType (networktype=%s, connected=%s, reconnect=%s)\n",
		p,connected ? "true":"false", reconnect?"true" : "false");

    if( g_pSerCore) {

        switch (networktype) {
            case NETWORK_NONE:
                g_pSerCore->serphone_set_networkType("noNetwork");
                break;
            case NETWORK_LAN:
                g_pSerCore->serphone_set_networkType("lan");
                break;
            case NETWORK_GPRS:
                g_pSerCore->serphone_set_networkType("gprs");
                break;
            case NETWORK_3G:
                g_pSerCore->serphone_set_networkType("3g");
                break;
            case NETWORK_WIFI:
                g_pSerCore->serphone_set_networkType("wifi");
                break;
            default:
                break;
        }

        if( networktype == NETWORK_GPRS ) {
            g_pSerCore->serphone_core_set_audio_pacinterval(60);
        } else {
            g_pSerCore->serphone_core_set_audio_pacinterval(60);
        }
    }

	if( networktype == NETWORK_3G) {
		g_keepAliveTime = gAliveTimeFor3G;
    }
	else {
		g_keepAliveTime = gAliveTimeForWifi;
    }

	PrintConsole("Adjust KeepAlive interval to %d ms\n",g_keepAliveTime);

	if (g_pSerCore)
		sal_set_keepalive_period(g_pSerCore->sal,g_keepAliveTime);

	if( !connected) {
		PrintConsole("[APICall] onConnectError(NoNetwork)\n");
		g_bConnected = false;
		PrintConsole("[APICall] setNetworkType,onConnectError(%d)\n",ReasonNoNetwork);
        if(g_cbInterface.onConnectError)
            g_cbInterface.onConnectError(ReasonNoNetwork);
		return;
	}
	if( reconnect && networktype != NETWORK_NONE )
	{
		PrintConsole("[APICall] reconnect int network\n");
            g_pSerCore->sip_check_thread_active();

        if (traceFile) {
            time_t temp = time(NULL);
            std::string strTime = timetodate(temp);
            fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
            fwrite(" g_RegisterTimerFlag = 0 in func setNetworkType\n", strlen(" g_RegisterTimerFlag = 0 in func setNetworkType\n"), 1, traceFile);
            fflush(traceFile);
        }
        g_RegisterTimerFlag = 0;
        g_RegisterTimerB = time(NULL);
		sal_reinit_network();


        SerPhoneCall *pCall = g_pSerCore->serphone_core_get_current_call();

        if(pCall)
        {
            int ret = 0;
            SerphoneCallParams *params=serphone_call_params_copy(&pCall->current_params);
            if(g_pSerCore)
            {
                if(pCall->ice_session)
                    g_pSerCore->serphone_call_delete_ice_session(pCall);
                g_pSerCore->serphone_core_get_local_ip(NULL,pCall->localip);
                ret = g_pSerCore->serphone_core_update_call(pCall, params);
            }
            else ret = -1;
            serphone_call_params_destroy (params);
        }
	}
}
extern "C" int selectCamera(int cameraIndex, int capabilityIndex,int fps,int rotate,bool force)
{
	PrintConsole("[APICall] selectCamera (cameraIndex=%d,capabilityIndex=%d,fps=%d,ratate=%d force=%d )\n",
		cameraIndex,capabilityIndex,fps,rotate,force);
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->selectCamera(cameraIndex,capabilityIndex,fps,rotate,force);
}

extern "C" 	int getCameraInfo(CameraInfo **ci)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->getCameraInfo(ci);
}
extern "C" int setSpeakerVolume(unsigned int volume)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->serphone_core_set_speaker_volume(volume);
}
extern "C" unsigned int getSpeakerVolume()
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->serphone_core_get_speaker_volume();
}

extern "C" int getCallStatistics(int type,MediaStatisticsInfo *stats)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    return g_pSerCore->getCallStatistics(type,stats);
}
extern "C" const char* getVersion()
{
    if( strlen(gVersionString) <= 0 )
    {
        const char *platform ="unknow",*arch = "arm",*voice="voice=false",*video="video=false";
#ifdef __ANDROID__
        platform = "Android";
#elif defined WIN32
        platform = "Windows";
#elif defined __APPLE__
#if TARGET_OS_IPHONE
        platform ="iOS";
#else
        platform ="Mac OS";
#endif
#elif __linux
        platform ="Linux";
#endif
#if defined WEBRTC_ARCH_ARM_V7A
        arch = "armv7";
#elif defined WEBRTC_ARCH_ARM_V5
        arch = "armv5";
#endif

#ifndef NO_VOIP_FUNCTION
        voice="voice=true";
#ifdef VIDEO_ENABLED
        video="video=true";
#endif
#endif
        sprintf(gVersionString,"%s#%s#%s#%s#%s#%s %s",CCP_SDK_VERSION,platform,arch,voice, video,__DATE__,__TIME__);
    }

	return gVersionString;
}
extern "C" void setKeepAliveTimeout(int forWifi, int for3G)
{
	PrintConsole("[APICall] setKeepAliveTime forWifi(%d), for3G(%d)\n",forWifi,for3G);
	gAliveTimeFor3G = for3G*1000;
	gAliveTimeForWifi = forWifi*1000;

	if(g_NetworkType == NETWORK_3G)
		g_keepAliveTime = gAliveTimeFor3G;
	else
		g_keepAliveTime = gAliveTimeForWifi;

	PrintConsole("Adjust KeepAlive interval to %ds\n",g_keepAliveTime);
	if (g_pSerCore)
		sal_set_keepalive_period(g_pSerCore->sal,g_keepAliveTime);
}

extern "C" void  enableKeepAlive(bool enable)
{

}

extern "C" int setCodecEnabled(int type, bool enabled)
{
    char mime[32];
    int freq = 8000;
    switch (type) {
	case codec_PCMU:
            sprintf(mime, "PCMU");
            break;
        case codec_G729:
            sprintf(mime, "G729");
            break;
        case codec_OPUS8K:
            sprintf(mime, "opus");
            break;
        case codec_OPUS16K:
            sprintf(mime, "opus");
            freq = 16000;
            break;
        case codec_OPUS48K:
            sprintf(mime, "opus");
            freq = 48000;
            break;
        case codec_VP8:
            sprintf(mime, "VP8");
            freq = 90000;
            break;
        case codec_H264:
            sprintf(mime, "H264");
            freq = 90000;
            break;
//		case Codec_H264SVC:
//			sprintf(mime, "H264-SVC");
//			freq = 90000;
//			break;
//        case codec_SILK8K:
//            sprintf(mime, "SILK");
//            freq = 8000;
//            break;
//        case codec_SILK12K:
//            sprintf(mime, "SILK");
//            freq = 12000;
//            break;
//        case codec_SILK16K:
//            sprintf(mime, "SILK");
//            freq = 16000;
//            break;
//        case codec_AMR:
//            sprintf(mime, "AMR");
//            freq = 8000;
//            break;
        default:
            sprintf(mime, "none");
            break;
    }
    if(g_pSerCore)
        g_pSerCore->serphone_core_enable_payload_type(mime, freq, enabled);
    return 0;
}
bool getCodecEnabled(int type)
{
    char mime[32];
    int freq = 8000;
    switch (type) {
        case codec_PCMU:
            sprintf(mime, "PCMU");
            break;
        case codec_G729:
            sprintf(mime, "G729");
            break;
        case codec_OPUS8K:
            sprintf(mime, "opus");
            break;
        case codec_OPUS16K:
            sprintf(mime, "opus");
			freq = 16000;
            break;
        case codec_OPUS48K:
            sprintf(mime, "opus");
			freq = 48000;
            break;
        case codec_VP8:
            sprintf(mime, "VP8");
			freq = 90000;
            break;
        case codec_H264:
            sprintf(mime, "H264");
			freq = 90000;
            break;
//		case codec_H264SVC:
//			sprintf(mime, "H264-SVC");
//			break;
//        case codec_SILK8K:
//            sprintf(mime, "SILK");
//            freq = 8000;
//            break;
//        case codec_SILK12K:
//            sprintf(mime, "SILK");
//            freq = 12000;
//            break;
//        case codec_SILK16K:
//            sprintf(mime, "SILK");
//            freq = 16000;
//            break;
//        case codec_AMR:
//            sprintf(mime, "AMR");
//            freq = 8000;
//            break;
        default:
            sprintf(mime, "none");
            break;
    }
    bool enabled = false;
    if(g_pSerCore)
        enabled =  g_pSerCore->serphone_core_is_payload_type_enable(mime, freq);
    PrintConsole("getCodecEnabled mimi=%s enabled=%s\r\n", mime, enabled?"yes":"no");
    return enabled;
}

extern "C" int setAudioConfigEnabled(int type, bool enabled, int mode)
{
    PrintConsole("[APICall setAudioConfigEnabled type=%d enabled=%d mode=%d\n",type,  enabled, mode);
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    return g_pSerCore->serphone_core_set_audio_config_enabled(type, enabled, mode);
}

extern "C" int getAudioConfigEnabled(int type, bool *enabled, int *mode)
{
    PrintConsole("[APICall getAgcEnabled\n");
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    int ret =g_pSerCore->serphone_core_get_audio_config_enabled(type, (bool_t*)enabled, mode);

    PrintConsole("[APICall getAgcEnabled type=%d enabled=%d, mode=%d\n", type, enabled, mode);
    return ret;
}

extern "C" int resetAudioDevice()
{
    PrintConsole("[APICall getAgcEnabled\n");
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    int ret = g_pSerCore->serphone_core_reset_audio_device();
    return ret;
}

extern "C" void setDtxEnabled(bool enabled)
{
    PrintConsole("[APICall getAgcEnabled\n");
    if (g_pSerCore)
        g_pSerCore->serphone_core_set_dtx_enabled(enabled);
    return;
}

extern "C" void setVideoBitRates(int bitrates)
{
    PrintConsole("[APICall setVideoBitRates %d\n", bitrates);
    if (g_pSerCore)
        g_pSerCore->serphone_core_set_video_bitrates(bitrates);
}

extern "C" int setSrtpEnabled(bool tls, bool srtp, bool userMode, int cryptType, const char *key)
{
    PrintConsole("[APICall setSrtpEnabled\n");
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    if (g_pSerCore) {
        //        g_pSerCore->serphone_core_enable_srtp(tls, srtp, userMode, cryptType, key);
        g_pSerCore->serphone_core_enable_srtp(FALSE, srtp, userMode, cryptType, key);
    }
    return -1;
}

extern "C" int setTlsSrtpEnabled(bool tls, bool srtp, int cryptType, const char *key)
{
    PrintConsole("[APICall setSrtpEnabled\n");
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    if (g_pSerCore) {
        //        g_pSerCore->serphone_core_enable_srtp(tls, srtp, userMode, cryptType, key);
        g_pSerCore->serphone_core_enable_srtp(tls, srtp, cryptType, key);
    }
    return -1;
}

CCPAPI int setProcessDataEnabled(const char *callid, bool flag)
{
    PrintConsole("[APICall setProcessDataEnabled\n");
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    if (g_pSerCore) {
        SerPhoneCall *pCall = NULL;
        int ret = findCall(callid, &pCall);
        if (ret != 0) {
            return -1;
        }
        g_pSerCore->serphone_core_set_process_audio_data_flag(pCall, flag);
    }
    return 0;
}

extern "C" int startRtpDump(const char *callid, int mediaType, const char *fileName, int direction)
{
    PrintConsole("[APICall startRtpDump callid=%s mediaType=%d fileName=%s direction=%d\n", callid, mediaType, fileName, direction);
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);

    SerPhoneCall *pCall = NULL;
    int ret = findCall(callid, &pCall);
    if(ret != 0)
        return -1;
    return g_pSerCore->serphone_core_start_rtp_dump(pCall, mediaType, fileName, (cloopenwebrtc::RTPDirections)direction);
}

extern "C" int stopRtpDump(const char *callid, int mediaType, int direction)
{
    PrintConsole("[APICall stopRtpDump callid=%s mediaType=%d direction=%d\n", callid, mediaType, direction);
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);

    SerPhoneCall *pCall = NULL;
    int ret = findCall(callid, &pCall);
    if(ret != 0)
        return -1;
    return g_pSerCore->serphone_core_stop_rtp_dump(pCall, mediaType, (cloopenwebrtc::RTPDirections)direction);
}

int getSpeakerInfo(SpeakerInfo **speakerinfo)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->getPlayoutDeviceInfo(speakerinfo);
}

int selectSpeaker(int speakerIndex)
{
	PrintConsole("[APICall selectSpeaker speakerIndex=%d\n", speakerIndex);
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->selectPlayoutDevice(speakerIndex);
}

int getMicroPhoneInfo(MicroPhoneInfo** microphoneinfo)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->getRecordDeviceInfo(microphoneinfo);
}

int selectMicroPhone(int microphoneIndex)
{
	PrintConsole("[APICall selectMicroPhone microphoneIndex=%d\n", microphoneIndex);
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->selectRecordDevice(microphoneIndex);
}

int getUniqueID(char *uniqueid, int len)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return serphone_core_get_unique_id(uniqueid, len);
}

int setStunServer(const char *server, int port)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    char ip_with_port[64+12] = {'\0'};
    if (NULL == server) {
        PrintConsole("[APICall setStunServer server is NULL\n");
        return -1;
    }
    if (port <= 0 || port >=0xffff) {
        PrintConsole("[APICall setStunServer port = %d, which is invalid\n",port);
    }
    sprintf(ip_with_port, "%s:%d",server,port);
    g_pSerCore->serphone_core_set_stun_server(ip_with_port);
    return 0;
}

int setFirewallPolicy(CCPClientFirewallPolicy policy)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    if (SerphonePolicyUseIce == policy) {
        policy = (CCPClientFirewallPolicy)(policy + 2);
    }
    SerphoneFirewallPolicy ser_policy = (SerphoneFirewallPolicy)policy;
    g_pSerCore->serphone_core_set_firewall_policy(ser_policy);
    return 0;
}

int setShieldMosaic(bool flag)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    PrintConsole("[APICall setShieldMosaic called, flag = %d\n",flag);
    g_pSerCore->serphone_set_mosaic(flag);
    return 0;
}

CCPAPI int seRateAfterP2PSucceed(int rate)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    PrintConsole("[APICall] seRateAfterP2PSucceed called, rate = %d\n",rate);
    if (rate <= 0 || rate > 2000) {
        PrintConsole("[APICall] seRateAfterP2PSucceed called, illegal rate[%d]. Rate should bewteen 1~2000\n",rate);
        return -1;
    }
    g_pSerCore->serphone_set_rate_p2p(rate);
    return 0;
}

CCPAPI int startDeliverVideoFrame(const char *callid)
{
    PrintConsole("[APICall startDeliverVideoFrame callid = %s\n",callid);

    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);

    SerPhoneCall *pCall = NULL;
    int ret = findCall(callid, &pCall);
    if(ret != 0)
        return -1;
    return g_pSerCore->startDeliverVideoFrame(pCall);
}

CCPAPI int stopDeliverVideoFrame(const char *callid)
{
    PrintConsole("[APICall stopDeliverVideoFrame callid = %s\n",callid);

    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);

    SerPhoneCall *pCall = NULL;
    int ret = findCall(callid, &pCall);
    if(ret != 0)
        return -1;
    return g_pSerCore->stopDeliverVideoFrame(pCall);
}

CCPAPI int startRecordVoice(const char *callid, const char *filename)
{
	PrintConsole("[APICall startRecordVoice callid = %s\n",callid);
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);

	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if(ret != 0)
		return -1;
	return g_pSerCore->serphone_call_start_record_audio(pCall, filename);
}

CCPAPI int stopRecordVoice(const char *callid)
{
	PrintConsole("[APICall stopRecordVoice callid = %s\n",callid);

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if(ret != 0)
		return -1;
	return g_pSerCore->serphone_call_stop_record_audio(pCall);
}

CCPAPI int startRecordVoiceEx(const char *callid, const char *rFileName, const char *lFileName)
{
	PrintConsole("[APICall startRecordVoiceEx callid = %s\n",callid);
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);

	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if(ret != 0)
		return -1;
	return g_pSerCore->serphone_call_start_record_audio_ex(pCall, rFileName, lFileName);
}

CCPAPI int getLocalVideoSnapshot(const char *callid, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height)
{
    PrintConsole("[APICall getLocalVideoSnapshot callid = %s\n",callid);

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if(ret != 0)
		return -1;
	return g_pSerCore->getLocalVideoSnapshot(pCall, buf, size, width, height);
}

CCPAPI int getRemoteVideoSnapshot(const char *callid, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height)
{
    PrintConsole("[APICall getRemoteVideoSnapshot callid = %s\n",callid);

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if(ret != 0)
		return -1;
	return g_pSerCore->getRemoteVideoSnapshot(pCall, buf, size, width, height);
}

CCPAPI int getLocalVideoSnapshotExt(const char *callid, const char *fielName)
{
	PrintConsole("[APICall getLocalVideoSnapshotExt callid = %s fileName=%s\n",callid, fielName);

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if(ret != 0)
		return -1;
	return g_pSerCore->getLocalVideoSnapshot(pCall,fielName);
}
CCPAPI int getRemoteVideoSnapshotExt(const char *callid, const char *fielName)
{
	PrintConsole("[APICall getRemoteVideoSnapshotExt callid = %s fielName=%s\n",callid, fielName);

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if(ret != 0)
		return -1;
	return g_pSerCore->getRemoteVideoSnapshot(pCall, fielName);
}

CCPAPI int startRecordRemoteVideo(const char *callid, const char *filename)
{
	if (!callid || !filename) {
		PrintConsole("[APICall startRecordRemoteVideo Failed\n");
		return -1;
	}

	PrintConsole("[APICall startRecordRemoteVideo callid = %s filename=%s\n", callid, filename);

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if (ret != 0)
		return -1;
	return g_pSerCore->serphone_call_start_record_r_video(pCall, filename);
}

CCPAPI int stopRecordRemoteVideo(const char *callid)
{
	PrintConsole("[APICall stopRecordRemoteVideo callid = %s\n", callid);

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if (ret != 0)
		return -1;
	return g_pSerCore->serphone_call_stop_record_r_video(pCall);
}

CCPAPI int startRecordLocalVideo(const char *callid, const char *filename)
{
	if (!callid || !filename) {
		PrintConsole("[APICall startRecordLocalVideo Failed\n");
		return -1;
	}

	PrintConsole("[APICall startRecordLocalVideo callid = %s filename=%s\n", callid, filename);

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if (ret != 0)
		return -1;
	return g_pSerCore->serphone_call_start_record_l_video(pCall, filename);
}

CCPAPI int stopRecordLocalVideo(const char *callid)
{
	PrintConsole("[APICall stopRecordLocalVideo callid = %s\n", callid);

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if (ret != 0)
		return -1;
	return g_pSerCore->serphone_call_stop_record_l_video(pCall);
}

CCPAPI int noiseSuppression(const void* audioSamples,short *out)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    PrintConsole("[APICall noiseSuppression called\n");
    return g_pSerCore->serphone_noise_suppression(audioSamples, out);
}

CCPAPI  int checkUserOnline(const char *user)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall checkUserOnline called\n");
	return g_pSerCore->serphone_core_check_account_online((char*)user);
}

CCPAPI int getNetworkStatistic(const char *callid, long long *duration, long long *send_total_sim, long long *recv_total_sim, long long *send_total_wifi, long long *recv_total_wifi)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	//PrintConsole("[APICall getNetworkStatistic called\n");
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if(ret != 0)
		return -1;
	return g_pSerCore->serphone_call_get_network_statistic(pCall, duration, send_total_sim, recv_total_sim, send_total_wifi, recv_total_wifi);
}

extern "C"  const char *notifyVideoRotate(const char *receiver, const char *degree)
{
	PrintConsole("[APICall] notifyVideoRotate (receiver=%s, local video rotate left %s degree) \n",
                 receiver ? receiver  : "null",
                 degree
                  );

	if(!g_pSerCore || !receiver || !degree)
		return "";

    int bufLen = 64+strlen(degree);

    char *bufPtr = (char*)ms_malloc(bufLen);
    if (!bufPtr) {
        return "";
    }

    sprintf(bufPtr, "type=4\r\n%s",degree);

    PrintConsole("[APICall] notifyVideoRotate");

    const char *callid = g_pSerCore->serphone_core_send_text(receiver,bufPtr);
    ms_free((void **)&bufPtr);
    bufPtr = NULL;

    return callid;
}

extern "C" int android_media_init_audio()
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall android_media_init_audio called\n");
    ECMedia_init_audio();
    return 0;
}

extern "C" int android_media_uninit_audio()
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall android_media_uninit_audio called\n");
    ECMedia_uninit_audio();
    return 0;
}

extern "C" int setAudioGain(float inaudio_gain, float outaudio_gain)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall setAudioGain called\n");
    g_pSerCore->serphone_set_audio_gain(inaudio_gain,outaudio_gain);
    return 0;
}

extern "C" int setPrivateCloud(const char *companyID, const char *restAddr, bool nativeCheck)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall setPrivateCloud called\n");
    return g_pSerCore->serphone_set_privateCloud(companyID,restAddr,nativeCheck);
}

extern "C" int setRootCAPath(const char * caPath)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall setRootCAPath called, root ca path:%s\n",caPath);
    return g_pSerCore->serphone_set_root_ca_path(caPath);
}

//sean add begin 20140626 init and release audio device
extern "C" int registerAudioDevice()
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall registerAudioDevice called\n");
    g_pSerCore->serphone_register_audio_device();
    return 0;
}

extern "C" int deregisterAudioDevice()
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall deregisterAudioDevice called\n");
    g_pSerCore->serphone_deregister_audio_device();
    return 0;
}

extern "C" int SetNetworkGroupId(const char *groupID)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall SetNetworkGroupId called\n");
    g_pSerCore->serphone_set_groupID(groupID);
    return 0;
}

//sean add end 20140626 init and release audio device
extern "C" int setProcessOriginalDataEnabled(const char *callid, bool flag)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall setProcessOriginalDataEnabled called\n");
    if (g_pSerCore) {
        SerPhoneCall *pCall = NULL;
        int ret = findCall(callid, &pCall);
        if (ret != 0) {
            return -1;
        }
        g_pSerCore->serphone_core_set_process_original_audio_data_flag(pCall, flag);
    }
    return 0;
}
//sean add end 20140626 init and release audio device

extern "C"  int startRecordScreen(const char *callid, const char *filename, int bitrates, int fps, int type)
{
	if(!callid || !filename) {
		PrintConsole("[APICall startRecordScreen Failed\n");
		return -1;
	}

	PrintConsole("[APICall startRecordScreen callid = %s filename=%s\n",callid, filename);

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if(ret != 0)
		return -1;
	return g_pSerCore->serphone_call_start_record_screen(pCall, filename, bitrates, fps, type);
}

extern "C"  int stopRecordScreen(const char *callid)
{
	PrintConsole("[APICall stopRecordScreen callid = %s\n",callid);

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if(ret != 0)
		return -1;
	return g_pSerCore->serphone_call_stop_record_screen(pCall);
}

extern "C"  int getUsedCameraInfo(int *cameraIndex, int *capabilityIndex, int *maxFps, int *cameraRotate)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	*cameraIndex = g_pSerCore->m_usedCameraIndex;
	*capabilityIndex = g_pSerCore->m_usedCapabilityIndex;
	*cameraRotate = g_pSerCore->m_camerRotate;
	*maxFps = g_pSerCore->m_maxFPS;
	return 0;
}
extern "C"  void setBindLocalIP(const char* localip)
{
	if(!localip) {
		PrintConsole("[API Call setBindLocalIP failed. localip=%0x g_pSerCore=%0x\n", localip, g_pSerCore);
		return ;
	}
	PrintConsole("[API Call setBindLocalIP :%s\n", localip);
	serphone_core_set_bind_local_addr(localip);
}
extern "C" int setVideoConferenceAddr(const char *ip)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall setVideoConferenceAddr called\n");
    return g_pSerCore->serphone_set_video_conference_addr(ip);
}

extern "C" int requestMemberVideo(const char *conferenceNo, const char *conferencePasswd, const char *remoteSipNo, void *videoWindow, int port)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall requestMemberVideo called\n");
    return g_pSerCore->serphone_set_video_window_and_request_video_accord_sip(remoteSipNo, videoWindow, conferenceNo, conferencePasswd, port);
}

extern "C" int stopMemberVideo(const char *conferenceNo, const char *conferencePasswd, const char *remoteSipNo)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall stopMemberVideo called\n");
	return g_pSerCore->serphone_stop_conference_video_accord_sip(remoteSipNo, conferenceNo, conferencePasswd);
}

int resetVideoConfWindow(const char *sip, void *newWindow)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    PrintConsole("[APICall resetVideoConfWindow called\n");
    if (NULL == sip) {
        return -2;
    }
    if (NULL == newWindow) {
        return -3;
    }
    return g_pSerCore->serphone_reset_conference_video_window(sip, newWindow);
}

extern "C" int startVideoCapture(const char* callid)
{
	if(!callid)
		return -1;

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);

	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if(ret != 0)
		return -1;
	return g_pSerCore->startVideoCapture(pCall);
}

extern "C" int setSilkRate(int rate)
{
//    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
//    PrintConsole("[APICall setSilkRate called, rate:%d\n",rate);
//    if (!getCodecEnabled(codec_SILK8K)) {
//        PrintConsole("[WARNING] currnt audio codec is not silk, set audio codec to silk first\n");
//        return -1;
//    }
//    return g_pSerCore->serphone_set_silk_rate(rate);
	return -1;
}

extern "C" int setAudioMode(int mode)
{
    int ret = 0;
//    switch (SerphoneAudioMode(mode)) {
//        case SerphoneAudioModeLowBandWidth:
//        {
//            setCodecEnabled(codec_G729,true);
////            setCodecEnabled(codec_AMR, false);
////            setCodecEnabled(codec_iLBC, false);
//            setCodecEnabled(codec_PCMA, false);
//            setCodecEnabled(codec_PCMU, false);
//            setCodecEnabled(codec_SILK12K, false);
//            setCodecEnabled(codec_SILK16K, false);
//            setCodecEnabled(codec_SILK8K, false);
//        }
//            break;
//        case SerphoneAudioModeGoodHighQuality:
//        {
//            setCodecEnabled(codec_G729,false);
//            setCodecEnabled(codec_AMR, false);
//            setCodecEnabled(codec_iLBC, false);
//            setCodecEnabled(codec_PCMA, false);
//            setCodecEnabled(codec_PCMU, false);
//            setCodecEnabled(codec_SILK12K, false);
//            setCodecEnabled(codec_SILK16K, false);
//            setCodecEnabled(codec_SILK8K, true);
//        }
//            break;
//        default:
//        {
//            PrintConsole("[WARNING] do not support this mode currently\n");
//            ret = -1;
//        }
//            break;
//    }
    return ret;
}
extern "C" int PlayAudioFromRtpDump(int localPort, const char *ptName, int ploadType, int crypt_type, const char* key)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->PlayAudioFromRtpDump(localPort, ptName, ploadType, (cloopenwebrtc::ccp_srtp_crypto_suite_t)crypt_type, key);
}

extern "C" int StopPlayAudioFromRtpDump()
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->StopPlayAudioFromRtpDump();
}

extern "C" int PlayVideoFromRtpDump(int localPort, const char *ptName, int ploadType, void *videoWindow, int crypt_type, const char* key)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->PlayVideoFromRtpDump(localPort, ptName, ploadType, videoWindow, (cloopenwebrtc::ccp_srtp_crypto_suite_t)crypt_type, key);
}
extern "C" int StopPlayVideoFromRtpDump()
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->StopPlayVideoFromRtpDump();
}


extern "C" int startVideoWithoutCall(int cameraIndex, int videoW, int videoH, int rotate, void *videoWnd)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->startVideoWithoutCall(cameraIndex, videoW, videoH, rotate, videoWnd);
}

extern "C" int stopVideoWithoutCall()
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->stopVideoWithoutCall();
}

extern "C" int getSnapshotWithoutCall(const char *filePath)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->getSnapshotWithoutCall(filePath);
}

extern "C" int setNackEnabled(bool audioEnabled, bool videoEnabled)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	g_pSerCore->SetNackEnabled(audioEnabled, videoEnabled);
	return 0;
}

extern "C" int setVideoProtectionMode(int mode)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	g_pSerCore->SetVideoProtectionMode(mode);
	return 0;
}


extern "C" int setP2PEnabled(bool enabled)
{
#ifdef ENABLE_REMB_TMMBR_CONFIG
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	g_pSerCore->SetP2PEnabled(enabled);
	return 0;
#else
    return -1;
#endif
}

extern "C" int setRembEnabled(bool enabled)
{
#ifdef ENABLE_REMB_TMMBR_CONFIG
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	g_pSerCore->SetRembEnabled(enabled);
	return 0;
#else
    return -1;
#endif
}

extern "C" int setTmmbrEnabled(bool enabled)
{
#ifdef ENABLE_REMB_TMMBR_CONFIG
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	g_pSerCore->SetTmmbrEnabled(enabled);
	return 0;
#else
    return -1;
#endif
}

extern "C" int setVideoMode(int videoModeIndex)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	g_pSerCore->setVideoMode(videoModeIndex);
	return 0;
}
extern "C" int setDesktopShareParam(int desktop_width, int desktop_height, int desktop_frame_rate, int desktop_bit_rate)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	g_pSerCore->setDesktopShareParam(desktop_width, desktop_height, desktop_frame_rate, desktop_bit_rate);
	return 0;
}


extern "C" int setDeviceID(const char *deviceid, const char *appId, bool testFlag)
{
#ifdef HAIYUNTONG
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    PrintConsole("[APICall %s called, deviceid:%s, appid:%s, testFlag\n",__FUNCTION__,deviceid,appId,testFlag);
    int ret = g_pSerCore->serphone_set_deviceid_pincode(deviceid, appId, testFlag);
    return (-9999==ret)?-1001:ret;
#endif
    return -1001;
}

extern "C" int setHaiyuntongEnabled(bool flag)
{
#ifdef HAIYUNTONG
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    PrintConsole("[APICall %s called, enable haiyuntong:%s\n",__FUNCTION__,flag?"TRUE":"FALSE");
    return g_pSerCore->serphone_enable_haiyuntong(flag);
#endif
    return -1001;
}

extern "C" int haiyuntongFileEncrypt(const unsigned char *file, long fileLen, char *remoteSip, long remoteSipLen, unsigned char *fileCrpEnvelopp, long* fileCrpEnvelopLen)
{
#ifdef HAIYUNTONG
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    PrintConsole("[APICall %s called, remoteSip:%s\n",__FUNCTION__,remoteSip);
    int ret = g_pSerCore->serphone_file_encrypt(file, fileLen, remoteSip, remoteSipLen, fileCrpEnvelopp, fileCrpEnvelopLen);
    return (-9999==ret)?-1001:ret;
#endif
    return -1001;
}


extern "C" int haiyuntongFileDecrypt(const unsigned char *file, long fileLen, char *remoteSip, long remoteSipLen, unsigned char *fileDeCrpt, long* fileDeCrptLen)
{
#ifdef HAIYUNTONG
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    PrintConsole("[APICall %s called, remoteSip:%s\n",__FUNCTION__,remoteSip);
    int ret = g_pSerCore->serphone_file_decrypt(file, fileLen, remoteSip, remoteSipLen, fileDeCrpt,  fileDeCrptLen);
    return (-9999==ret)?-1001:ret;
#endif
    return -1001;
}


extern "C" int haiyuntongGroupFileEncrypt(const unsigned  char *file, long fileLen, char **userList, long *eachLen, int numOfUsers, unsigned char *groupFileCrpEnvelopp, long* groupFileCrpEnvelopLen)
{
#ifdef HAIYUNTONG
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    PrintConsole("[APICall %s called, member no:%d\n",__FUNCTION__,numOfUsers);
    int ret = g_pSerCore->serphone_group_file_encrypt(file, fileLen, userList, eachLen, numOfUsers, groupFileCrpEnvelopp, groupFileCrpEnvelopLen);

    return (-9999==ret)?-1001:ret;
#endif
    return -1001;
}


extern "C" int haiyuntongGroupFileDecrypt(const unsigned  char *file, long fileLen, char *selfSip, long selfSipLen, unsigned char *groupFileDecrpt, long*groupFileDecrptLen)
{
#ifdef HAIYUNTONG
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    PrintConsole("[APICall %s called, server id:%s\n",__FUNCTION__,selfSip);
    int ret = g_pSerCore->serphone_group_file_decrypt(file, fileLen, selfSip, selfSipLen, groupFileDecrpt, groupFileDecrptLen);
    return (-9999==ret)?-1001:ret;
#endif
    return -1001;

}


extern "C" int haiyuntongAddContact(char **userlist, long* userlistLen, int num)
{
#ifdef HAIYUNTONG
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    PrintConsole("[APICall %s called, num:%d\n",__FUNCTION__,num);
    int ret = g_pSerCore->serphone_add_contact(userlist, userlistLen, num);
    return (-9999==ret)?-1001:ret;
#endif
    return -1001;
}


extern "C" int haiyuntongDelContact(char *remoteSip, long remoteSipLen)
{
#ifdef HAIYUNTONG
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    PrintConsole("[APICall %s called, remoteSip:%s\n",__FUNCTION__,remoteSip);
    int ret = g_pSerCore->serphone_del_contact(remoteSip, remoteSipLen);
    return (-9999==ret)?-1001:ret;
#endif
    return -1001;
}

extern "C" void initHaiyuntongFailed()
{
#ifdef HAIYUNTONG
    PrintConsole("[APICAll %s called, reason:%d\n",__FUNCTION__, ReasonHaiyuntongInitFailed);
    g_cbInterface.onConnectError(ReasonHaiyuntongInitFailed);
#endif
}


CCPAPI int haiyuntongIsExistCert(const char *sip, long sipLen)
{
#ifdef HAIYUNTONG
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    PrintConsole("[APICall %s called, sip:%s\n",__FUNCTION__,sip);
    int ret = g_pSerCore->serphone_certExisted(sip, sipLen);
    return (-9999==ret)?-1001:ret;
#endif
    return -1001;
}

#if 0
extern "C" int openTraceFile(const char *filePath)
{
    if (NULL == filePath) {
        PrintConsole("[APICall WARNNING] %s filePath is NULL");
        return -1;
    }
    globalFilePath = (char *)malloc(strlen(filePath)+1);
    memcpy(globalFilePath, filePath, strlen(filePath));
    globalFilePath[strlen(filePath)] = '\0';
    traceFile = fopen(globalFilePath, "a+");

    if (traceFile) {
        fwrite("\n\n///////////////", strlen("\n\n///////////////"), 1, traceFile);
        time_t temp = time(NULL);
        std::string strTime = timetodate(temp);
        fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
        fwrite("///////////////\n", strlen("///////////////\n"), 1, traceFile);
        fflush(traceFile);
    }
}
#endif

extern "C" int setReconnectFlag(bool flag)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    return g_pSerCore->serphone_set_reconnect(flag);
}

#if 0
extern "C" int setRingbackFlag(const char * callid, bool flag)
{
    PrintConsole("[APICall setRingbackFlag callid=%s, flag=%d\n", callid, flag);
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);

    SerPhoneCall *pCall = NULL;
    int ret = findCall(callid, &pCall);
    if(ret != 0)
        return -1;
    return g_pSerCore->serphone_set_ringback(pCall, flag);

}
#endif

extern "C" int setVideoPacketTimeOut(int timeout)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	g_pSerCore->m_VideoTimeOut = timeout;
	return 0;
}

extern "C" int getLocalIP(const char *dst, char *result)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	//g_pSerCore->serphone_core_get_local_ip(dst, result);
	return 0;
}

extern "C" int FixedCameraInfo(const char *cameraName, const char *cameraId, int width, int height)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->FixedCameraInfo(cameraName, cameraId, width, height);
}

extern "C" int ConfigureChromaKey(const char *bgImage, float angle, float noise_level, int r, int g, int b)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->ConfigureChromaKey(bgImage, angle, noise_level, r, g, b);
}

extern "C" int StartVirtualBackGround()
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->StartVirtualBackGround();
}

extern "C" int StopVirtualBakcGround()
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->StopVirtualBakcGround();
}

extern "C" int GetStatsData(int type, char* callid, void** pbDataArray, int *pArraySize)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->GetStatsData(type, callid, pbDataArray, pArraySize);
}

extern "C" int DeleteStatsData(void* pbDataArray)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	g_pSerCore->DeleteStatsData(pbDataArray);
	return 0;
}
extern "C" int GetBandwidthUsage(const char* callid,
								unsigned int& total_bitrate_sent,
								unsigned int& video_bitrate_sent,
								unsigned int& fec_bitrate_sent,
								unsigned int& nackBitrateSent)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->GetBandwidthUsage(callid, total_bitrate_sent, video_bitrate_sent, fec_bitrate_sent, nackBitrateSent);
}


extern "C" int GetEstimatedReceiveBandwidth(const char* callid,
											unsigned int* estimated_bandwidth)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->GetEstimatedReceiveBandwidth(callid, estimated_bandwidth);
}


extern "C" int GetEstimatedSendBandwidth(const char* callid,
										unsigned int* estimated_bandwidth)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->GetEstimatedSendBandwidth(callid, estimated_bandwidth);
}

extern "C" int GetReceiveChannelRtcpStatistics(const char* callid,
	_RtcpStatistics& basic_stats,
	__int64& rtt_ms)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->GetReceiveChannelRtcpStatistics(callid, basic_stats, rtt_ms);
}

extern "C" int GetSendChannelRtcpStatistics(const char* callid,
	_RtcpStatistics& basic_stats,
	__int64& rtt_ms)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->GetSendChannelRtcpStatistics(callid, basic_stats, rtt_ms);
}

extern "C" int GetRtpStatistics(const char* callid,
	_StreamDataCounters& sent,
	_StreamDataCounters& received)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->GetRtpStatistics(callid, sent, received);
}

extern "C" int GetSendStats(const char* callid, int &encode_frame_rate, int &media_bitrate_bps, int &width, int &height, bool &suspended)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	//TODO:
	//cloopenwebrtc::VideoSendStream::Stats sendStats;
	//if(!g_pSerCore->GetSendStats(callid, sendStats))
	//{
	//	encode_frame_rate = sendStats.encode_frame_rate;
	//	media_bitrate_bps = sendStats.media_bitrate_bps;
	//	width = sendStats.sent_width;
	//	height = sendStats.sent_height;
	//	return 0;
	//}
	return -1;
}

extern "C" int EnableOpusFEC(bool enable)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    return g_pSerCore->Serphone_enable_opus_FEC(enable);
}

extern "C" int SetOpusPacketLossRate(int rate)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    return g_pSerCore->Serphone_set_opus_packet_loss_rate(rate);

}

extern "C" int StartRecord()
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->startRecord();
}
extern "C" int StopRecord()
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->stopRecord();
}

extern "C" int setAudioKeepAlive(char *callid, bool enable, int interval)
{
	if (!callid)
		return -1;

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);

	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if (ret != 0)
		return -1;
	return g_pSerCore->SetAudioKeepAlive(pCall, enable, interval);
}
extern "C" int setVideoKeepAlive(char *callid, bool enable, int interval)
{
	if (!callid)
		return -1;

	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);

	SerPhoneCall *pCall = NULL;
	int ret = findCall(callid, &pCall);
	if (ret != 0)
		return -1;
	return g_pSerCore->SetVideoKeepAlive(pCall, enable, interval);
}

extern "C" void *createLiveStream()
{
	SDK_UN_INITIAL_ERROR(NULL);
	return g_pSerCore->createLiveStream();
}
extern "C" int playLiveStream(void *handle, const char * url, void *renderView, ECLiveStreamNetworkStatusCallBack callback)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->playLiveStream(handle, url, renderView, callback);
}
extern "C" int pushLiveStream(void *handle, const char * url, void *renderView, ECLiveStreamNetworkStatusCallBack callback)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	return g_pSerCore->pushLiveStream(handle, url, renderView, callback);
}
extern "C" void stopLiveStream(void *handle)
{
	PrintConsole("try to stop live stream\n");
	if (!g_pSerCore)
		return;

	g_pSerCore->stopLiveStream(handle);
	PrintConsole("live stream stopped\n");

}
extern "C" void releaseLiveStream(void *handle)
{
	if (!g_pSerCore)
		return;
	g_pSerCore->releaseLiveStream(handle);
}

extern "C" void enableLiveStreamBeauty(void *handle)
{
	if (!g_pSerCore)
		return;
	g_pSerCore->enableLiveStreamBeauty(handle);
}

extern "C" void disableLiveStreamBeauty(void *handle)
{
	if (!g_pSerCore)
		return;
	g_pSerCore->disableLiveStreamBeauty(handle);
}

extern "C" int configLiveVideoStream(void *handle, LiveVideoStreamConfig config) {
    if (!g_pSerCore)
        return -1;
    return g_pSerCore->configLiveVideoStream(handle, config);
}

extern "C" int selectCameraLiveStream(void *handle, int index)
{
	if (!g_pSerCore)
		return -1;
	return g_pSerCore->liveStream_SelectCamera(handle, index);
}

extern "C" void setLiveVideoSource(void *handle, int video_source)
{
	if (!g_pSerCore)
		return ;
	 return g_pSerCore->setLiveVideoSource(handle, video_source);
	 
}
extern "C" int getShareWindows(void *handle, WindowShare ** windows)
{
#ifdef VIDEO_ENABLED
	return ECMedia_GetShareWindows(handle, windows);
#endif
}

extern "C"  void selectShareWindow(void *handle, int type, int id)
{
#ifdef VIDEO_ENABLED
	ECMedia_SelectShareWindow(handle, type, id);
#endif
}

extern "C" int startSendRtpPacket(int &channel, const char *ip, int rtp_port)
{
	if (!g_pSerCore) {
		return -1;
	}
	return g_pSerCore->startSendRtpPacket(channel, ip, rtp_port);
}

extern "C" int startRecvRtpPacket(int channelNum)
{
	if (!g_pSerCore) {
		return -1;
	}
	return g_pSerCore->startRecvRtpPacket(channelNum);
}

extern "C" int startRecordLocalMedia(const char *fileName, void *localview)
{
#ifdef VIDEO_ENABLED
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
    return g_pSerCore->startRecordLocalMedia(fileName, localview);
#endif
    return 0;
}

extern "C" void stopRecordLocalMedia()
{
#ifdef VIDEO_ENABLED
    PrintConsole("try to stop record local media\n");
    if (!g_pSerCore)
        return;
    
    g_pSerCore->stopRecordLocalMedia();
    PrintConsole("record local media stopped\n");
#endif
    
}
extern "C" int setScreeShareActivity(char *callid, void *activity)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall setScreeShareActivity called\n");
    if (g_pSerCore) {
        SerPhoneCall *pCall = NULL;
        int ret = findCall(callid, &pCall);
        if (ret != 0) {
            return -1;
        }
        g_pSerCore->setScreeShareActivity(pCall, activity);
    }
    return 0;
}

extern "C"  int sendTmmbr(char *callid, int ssrc)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall sendTmmbr called\n");
	if (g_pSerCore) {
		SerPhoneCall *pCall = NULL;
		int ret = findCall(callid, &pCall);
		if (ret != 0) {
			return -1;
		}
		g_pSerCore->send_tmmbr_request_video(pCall, ssrc);
	}
	return 0;
}

extern "C"  int requestVideo(char *callid, int width, int height)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall sendTmmbr called\n");
	if (g_pSerCore) {
		SerPhoneCall *pCall = NULL;
		int ret = findCall(callid, &pCall);
		if (ret != 0) {
			return -1;
		}

		unsigned int ssrc = pCall->m_partnerSSRC;
		setSsrcMediaType(ssrc, 1);
		setSsrcMediaAttribute(ssrc, width, height, 15);
		g_pSerCore->send_tmmbr_request_video(pCall, ssrc);
	}
	return 0;
}




extern "C"  int cancelTmmbr(char *callid)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall sendTmmbr called\n");
	if (g_pSerCore) {
		SerPhoneCall *pCall = NULL;
		int ret = findCall(callid, &pCall);
		if (ret != 0) {
			return -1;
		}
		g_pSerCore->cancel_tmmbr_request_video(pCall);
	}
	return 0;
}

extern "C"  int VideoStartReceive(char *callid)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall sendTmmbr called\n");
	if (g_pSerCore) {
		SerPhoneCall *pCall = NULL;
		int ret = findCall(callid, &pCall);
		if (ret != 0) {
			return -1;
		}
		g_pSerCore->video_start_receive(pCall);
	}
	return 0;
}

extern "C"  int VideoStopReceive(char *callid)
{
	SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT);
	PrintConsole("[APICall sendTmmbr called\n");
	if (g_pSerCore) {
		SerPhoneCall *pCall = NULL;
		int ret = findCall(callid, &pCall);
		if (ret != 0) {
			return -1;
		}
		g_pSerCore->video_stop_receive(pCall);
	}
	return 0;
}


extern "C" int SetRotateCapturedFrames(char *callid, ECMediaRotateCapturedFrame tr)//int ECMedia_set_rotate_captured_frames(int deviceid, ECMediaRotateCapturedFrame tr)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT)
    PrintConsole("[APICall %s called, callid %s, rotate %d]", __FUNCTION__, callid, tr);
    if (g_pSerCore)
    {
        SerPhoneCall *pCall = NULL;
        int ret = findCall(callid, &pCall);
        if (ret != 0) {
            return -1;
        }
        return g_pSerCore->set_rotate_captured_frames(pCall->m_CaptureDeviceId, tr);
    }
}


extern "C" int audioEnableMagicSound(bool enabled, int pitch, int tempo, int rate)
{
    SDK_UN_INITIAL_ERROR(ERR_SDK_UN_INIT)
    PrintConsole("[APICall called enabled %s, pitch %d, tempo %d, rate %d\n", enabled?"TRUE":"FALSE", pitch, tempo, rate);
    if (g_pSerCore) {
        g_pSerCore->audio_enable_magic_sound(enabled, pitch, tempo, rate);
    }
    return 0;
    
}
