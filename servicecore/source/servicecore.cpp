
#define   _CRTDBG_MAP_ALLOC
#include "servicecore.h"
#include "serphonecall.h"
#include "friends.h"
#include "lpconfig.h"
#include "enum.h"
#include "sometools.h"
#include "serprivate.h"
#include "sal_eXosip2.h"

#include "ECMedia.h"

//haiyuntong
#ifdef HAIYUNTONG
#include "voip.h"
#endif

//#ifdef VIDEO_ENABLED
//#include "vie_network.h"
//#endif

#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <base64.h>

extern "C"
{
    #include <crypto_kernel.h>
}

char *ca_certificate_path;
//#include "thread_wrapper.h"
#include "Utility/cJSON.h"

#define MAX_SERVICE_CALL   10
extern SalCallbacks serphone_sal_callbacks;

extern FILE *traceFile;
extern std::string timetodate(time_t const timer);
#define PACKAGE_DATA_DIR "./"
#define ROOT_CA_FILE PACKAGE_DATA_DIR "/serphone/rootca.pem"
#define DEFAULT_SIP_UDP_PORT    5065

char g_bind_local_addr[64] = {'\0'};

extern void CCPClientPrintLog(int loglevel, const char *loginfo);

//namespace cloopenwebrtc {
//class SerphoneTraceCallBack : public TraceCallback {
//public:
//	virtual void Print(const TraceLevel level,
//                       const char *traceString,
//                       const int length)
//		{
//            PrintConsole("%s\n",traceString);
//		}
//};
//}
//cloopenwebrtc::SerphoneTraceCallBack g_serphoneTraceCallBack;
std::map<int,VideoConferenceDesc*> ServiceCore::videoConferenceM;
SerphoneCoreVTable ServiceCore::vtable;
const char *Serphone_reason_to_string(SerphoneReason err)
{
	switch(err){
		case SerphoneReasonNone:
			return "No error";
		case SerphoneReasonNoResponse:
			return "No response";
		case SerphoneReasonBadCredentials:
			return "Bad credentials";
		case SerphoneReasonDeclined:
			return "Call declined";
		case SerphoneReasonNotFound:
			return "User not found";
        case SerphoneReasonCallMissed:
            return "Call missed";
        case SerphoneReasonBusy:
            return "User busy";
        case SerphoneReasonNoVoip:
            return "User's version not support voip";
	}
	return "unknown error";
}

int serphone_core_get_unique_id(char *uniqueid, int len)
{

    char *strCallID = NULL;
    sal_get_call_id(&strCallID);
    if(strCallID)
    {
        if(len <= strlen(strCallID) )
            snprintf(uniqueid, len-1, "%s", strCallID);
        else
            sprintf(uniqueid, "%s", strCallID);
        osip_free(strCallID);
        return 0;
    }
    return -1;
}

static bool_t username_match(const char *u1, const char *u2){
	if (u1==NULL && u2==NULL) return TRUE;
	if (u1 && u2 && strcasecmp(u1,u2)==0) return TRUE;
	return FALSE;
}

const char *serphone_global_state_to_string(SerphoneGlobalState gs)
{
	switch(gs){
		case LinphoneGlobalOff:
			return "LinphoneGlobalOff";
		break;
		case LinphoneGlobalOn:
			return "LinphoneGlobalOn";
		break;
		case LinphoneGlobalStartup:
			return "LinphoneGlobalStartup";
		break;
		case LinphoneGlobalShutdown:
			return "LinphoneGlobalShutdown";
		break;
	}
	return NULL;
}

SalPresenceStatus serphone_online_status_to_sal(SerphoneOnlineStatus os)
{
	switch(os){
		case LinphoneStatusOffline:
			return SalPresenceOffline;
		break;
		case LinphoneStatusOnline:
			return SalPresenceOnline;
		break;
		case LinphoneStatusBusy:
			return SalPresenceBusy;
		break;
		case LinphoneStatusBeRightBack:
			return SalPresenceBerightback;
		break;
		case LinphoneStatusAway:
			return SalPresenceAway;
		break;
		case LinphoneStatusOnThePhone:
			return SalPresenceOnthephone;
		break;
		case LinphoneStatusOutToLunch:
			return SalPresenceOuttolunch;
		break;
		case LinphoneStatusDoNotDisturb:
			return SalPresenceDonotdisturb;
		break;
		case LinphoneStatusMoved:
			return SalPresenceMoved;
		break;
		case LinphoneStatusAltService:
			return SalPresenceAltService;
		break;
		case LinphoneStatusPending:
			return SalPresenceOffline;
		break;
		default:
			return SalPresenceOffline;
		break;
	}
	return SalPresenceOffline;
}

typedef struct Hook{
	SerphoneCoreIterateHook fun;
	void *data;
}Hook;

static Hook *hook_new(SerphoneCoreIterateHook hook, void *hook_data){
	Hook *h=ms_new(Hook,1);  //ms_new


	h->fun=hook;
	h->data=hook_data;
	return h;
}

static void hook_invoke(Hook **h){
	(*h)->fun((*h)->data);
}

bool_t transports_unchanged(const LCSipTransports * tr1, const LCSipTransports * tr2)
{
	return
		tr2->udp_port==tr1->udp_port &&
		tr2->tcp_port==tr1->tcp_port &&
		tr2->dtls_port==tr1->dtls_port &&
		tr2->tls_port==tr1->tls_port;
}

static bool_t key_match(const char *tmp1, const char *tmp2){
	if (tmp1==NULL && tmp2==NULL) return TRUE;
	if (tmp1!=NULL && tmp2!=NULL && strcmp(tmp1,tmp2)==0) return TRUE;
	return FALSE;

}
static char * remove_quotes(char * input){
	char *tmp;
	if (*input=='"') input++;
	tmp=strchr(input,'"');
	if (tmp) *tmp='\0';
	return input;
}

static int realm_match(const char *realm1, const char *realm2){
	if (realm1==NULL && realm2==NULL) return TRUE;
	if (realm1!=NULL && realm2!=NULL){
		if (strcmp(realm1,realm2)==0) return TRUE;
		else{
			char tmp1[128];
			char tmp2[128];
			char *p1,*p2;
			strncpy(tmp1,realm1,sizeof(tmp1)-1);
			strncpy(tmp2,realm2,sizeof(tmp2)-1);
			p1=remove_quotes(tmp1);
			p2=remove_quotes(tmp2);
			return strcmp(p1,p2)==0;
		}
	}
	return FALSE;
}
/**
 * Create a SerphoneAuthInfo object with supplied information.
 *
 * The object can be created empty, that is with all arguments set to NULL.
 * Username, userid, password and realm can be set later using specific methods.
**/
SerphoneAuthInfo *serphone_auth_info_new(const char *username, const char *userid,
				   										const char *passwd, const char *ha1,const char *realm)
{
//	SerphoneAuthInfo *obj=ms_new0(SerphoneAuthInfo,1);  //ms_new0
	SerphoneAuthInfo *obj=(SerphoneAuthInfo *)malloc(sizeof(SerphoneAuthInfo)*1);  //ms_new0
	memset((void *)obj,0,sizeof(SerphoneAuthInfo)*1);


	if (username!=NULL && (strlen(username)>0) ) obj->username=ms_strdup(username);
	if (userid!=NULL && (strlen(userid)>0)) obj->userid=ms_strdup(userid);
	if (passwd!=NULL && (strlen(passwd)>0)) obj->passwd=ms_strdup(passwd);
	if (ha1!=NULL && (strlen(ha1)>0)) obj->ha1=ms_strdup(ha1);
	if (realm!=NULL && (strlen(realm)>0)) obj->realm=ms_strdup(realm);
	obj->works=FALSE;
	return obj;
}

SerphoneAuthInfo *serphone_auth_info_clone(const SerphoneAuthInfo *ai){
//	SerphoneAuthInfo *obj=ms_new0(SerphoneAuthInfo,1);  //ms_new0
	SerphoneAuthInfo *obj=(SerphoneAuthInfo *)malloc(sizeof(SerphoneAuthInfo)*1);  //ms_new0
	memset((void *)obj,0,sizeof(SerphoneAuthInfo)*1);


	if (ai->username) obj->username=ms_strdup(ai->username);
	if (ai->userid) obj->userid=ms_strdup(ai->userid);
	if (ai->passwd) obj->passwd=ms_strdup(ai->passwd);
	if (ai->ha1)	obj->ha1=ms_strdup(ai->ha1);
	if (ai->realm)	obj->realm=ms_strdup(ai->realm);
	obj->works=FALSE;
	obj->usecount=0;
	return obj;
}

/**
 * Returns username.
**/
const char *serphone_auth_info_get_username(const SerphoneAuthInfo *i){
	return i->username;
}

/**
 * Returns password.
**/
const char *serphone_auth_info_get_passwd(const SerphoneAuthInfo *i){
	return i->passwd;
}

const char *serphone_auth_info_get_userid(const SerphoneAuthInfo *i){
	return i->userid;
}

/**
 * Sets the password.
**/
void serphone_auth_info_set_passwd(SerphoneAuthInfo *info, const char *passwd){
	if (info->passwd!=NULL)
    {
		ms_free((void **)&info->passwd);
	}
	if (passwd!=NULL && (strlen(passwd)>0))
		info->passwd=ms_strdup(passwd);
}

/**
 * Sets the username.
**/
void serphone_auth_info_set_username(SerphoneAuthInfo *info, const char *username){
	if (info->username)
    {
		ms_free((void **)&info->username);
	}
	if (username && strlen(username)>0)
		info->username=ms_strdup(username);
}

/**
 * Sets userid.
**/
void serphone_auth_info_set_userid(SerphoneAuthInfo *info, const char *userid){
	if (info->userid){
		ms_free((void **)&info->userid);
	}
	if (userid && strlen(userid)>0)
		info->userid=ms_strdup(userid);
}

/**
 * Destroys a LinphoneAuthInfo object.
**/
void serphone_auth_info_destroy(SerphoneAuthInfo **obj){
	if ((*obj)->username!=NULL) {ms_free((void **)&(*obj)->username);}
	if ((*obj)->userid!=NULL) {ms_free((void **)&(*obj)->userid);}
	if ((*obj)->passwd!=NULL) {ms_free((void **)&(*obj)->passwd);}
	if ((*obj)->ha1!=NULL) {ms_free((void **)&(*obj)->ha1);}
	if ((*obj)->realm!=NULL) {ms_free((void **)&(*obj)->realm);}
	ms_free((void **)obj);
}

SerphoneAuthInfo *serphone_auth_info_new_from_config_file(LpConfig * config, int pos)
{
	char key[50];
	const char *username,*userid,*passwd,*ha1,*realm;

	sprintf(key,"auth_info_%i",pos);
	if (!lp_config_has_section(config,key)){
		return NULL;
	}

	username=lp_config_get_string(config,key,"username",NULL);
	userid=lp_config_get_string(config,key,"userid",NULL);
	passwd=lp_config_get_string(config,key,"passwd",NULL);
	ha1=lp_config_get_string(config,key,"ha1",NULL);
	realm=lp_config_get_string(config,key,"realm",NULL);
	return serphone_auth_info_new(username,userid,passwd,ha1,realm);
}

void serphone_proxy_config_write_to_config_file(LpConfig *config, SerphoneProxyConfig *obj, int index)
{
	char key[50];

	sprintf(key,"proxy_%i",index);
	lp_config_clean_section(config,key);
	if (obj==NULL){
		return;
	}
	if (obj->type!=NULL){
		lp_config_set_string(config,key,"type",obj->type);
	}
	if (obj->reg_proxy!=NULL){
		lp_config_set_string(config,key,"reg_proxy",obj->reg_proxy);
	}
	if (obj->reg_route!=NULL){
		lp_config_set_string(config,key,"reg_route",obj->reg_route);
	}
	if (obj->reg_identity!=NULL){
		lp_config_set_string(config,key,"reg_identity",obj->reg_identity);
	}
	lp_config_set_int(config,key,"reg_expires",obj->expires);
	lp_config_set_int(config,key,"reg_sendregister",obj->reg_sendregister);
	lp_config_set_int(config,key,"publish",obj->publish);
	lp_config_set_int(config,key,"dial_escape_plus",obj->dial_escape_plus);
	lp_config_set_string(config,key,"dial_prefix",obj->dial_prefix);
}

void serphone_auth_info_write_config(LpConfig *config, SerphoneAuthInfo *obj, int pos)
{
	char key[50];
	sprintf(key,"auth_info_%i",pos);
	lp_config_clean_section(config,key);

	if (obj==NULL){
		return;
	}
	if (obj->username!=NULL){
		lp_config_set_string(config,key,"username",obj->username);
	}
	if (obj->userid!=NULL){
		lp_config_set_string(config,key,"userid",obj->userid);
	}
	if (obj->passwd!=NULL){
		lp_config_set_string(config,key,"passwd",obj->passwd);
	}
	if (obj->ha1!=NULL){
		lp_config_set_string(config,key,"ha1",obj->ha1);
	}
	if (obj->realm!=NULL){
		lp_config_set_string(config,key,"realm",obj->realm);
	}
}

// add by yuanfang
void *serphone_core_get_user_data(ServiceCore *lc)
{
	return lc->data;
}

void serphone_core_set_user_data(ServiceCore *lc, void *userdata)
{
	lc->data=userdata;
}

static void internal_str_check_copy(char **dest, const char *src)
{
    size_t tempLen = strlen(src);
    if (!*dest) {
        *dest = new char[tempLen+1];
    }
    else if(strlen(*dest) < tempLen) {
        delete [] *dest;
        *dest = new char[tempLen + 1];
    }
    memcpy(*dest, src, tempLen);
    (*dest)[tempLen] = '\0';
}


ServiceCore::ServiceCore()
{
	sal = NULL;
	config = NULL;
	payload_types = NULL;
	default_proxy = NULL;
	friends = NULL;
	auth_info = NULL;
	current_call = NULL;
	calls = NULL;
	call_logs = NULL;
	chatrooms = NULL;
	alt_contact = NULL;
	prevtime = 0;
	data = NULL;
	bl_reqs = NULL;
	subscribers = NULL;
    audio_bw = 0;
	netup_time = 0;
	hooks = NULL;
	m_videoWindow = NULL;
	m_cameraInfo = NULL;
	m_cameraCount = 0;
	m_usedCameraIndex = 0;//sean update for easy test
	m_usedCapabilityIndex =0 ;

	m_SnapshotDeviceId = -1;
	m_SnapshotChannelID = -1;

	audioNackEnabled = false;
	videoNackEnabled = true;
	videoProtectionMode = -1;
	presence_mode = LinphoneStatusOffline;
	srandom((unsigned int) time(NULL));
	m_criticalSection = cloopenwebrtc::CriticalSectionWrapper::CreateCriticalSection();

	max_calls = MAX_SERVICE_CALL;
	memset(&net_conf,0,sizeof(net_conf));
	memset(&sip_conf,0,sizeof(sip_conf));
	memset(&rtp_conf,0,sizeof(rtp_conf));
	memset(&sound_conf,0,sizeof(sound_conf));
	memset(&video_conf,0,sizeof(video_conf));
	memset(&codecs_conf,0,sizeof(codecs_conf));
    memset(&capability_conf, 0, sizeof(capability_conf));

	memset(local_addr, 0, sizeof(local_addr));
///////////the following code added by zdm

	m_ringplay_flag = FALSE;
	dmfs_playing_start_time = 0;
	local_playfile_channelID = -1;
    local_playfile_channelID_prering = -1;
    local_playfile_channelID_afterring = -1;

	m_speakerInfo = NULL;
	m_speakerCount = 0;
	m_microphoneInfo = NULL;
	m_microphoneCount = 0;
	m_usedSpeakerIndex = -1;
	m_usedMicrophoneIndex = -1;

    m_silkBitRate = 0;
    m_packetInterval = 20;
    m_agcEnabled = false;
    m_ecEnabled = true;
    m_nsEnabled = true;
    m_hcEnabled = false;

#ifdef _WIN32
	m_agcMode = cloopenwebrtc::kAgcAdaptiveAnalog;
	m_ecMode = cloopenwebrtc::kEcAec;
	m_nsMode = cloopenwebrtc::kNsModerateSuppression;
#else
    m_agcMode = cloopenwebrtc::kAgcAdaptiveDigital;
    m_ecMode = cloopenwebrtc::kEcAecm;
    m_nsMode = cloopenwebrtc::kNsModerateSuppression;
#endif

    m_dtxEnabled = false;
    //sean 201305009
    tls_enable = false;
    srtp_enable = false;
    user_mode = false;
    encryptType = (cloopenwebrtc::ccp_srtp_crypto_suite_t)-1;
	m_SrtpKey = NULL;
	m_SrtpKey = (char *)malloc(SRTP_KEY_SZ);//currently 100
    memset(m_SrtpKey, 0, SRTP_KEY_SZ);
//    key = "012345678901234567890123456789012345678901234";
    _terminate_call = -1;

//    sean add begin 0915
    _shield_mosaic = false/*true*/; //by ylr, 20150513

#ifdef  _WIN32
	m_videoBitRates = 150/*512*/;  //by ylr, 20150513
	 _rate_after_p2p =768;
	 m_maxFPS = 15;
	 m_sendVideoFps = 15;
	 m_sendVideoWidth = 352;
	 m_sendVideoHeight = 288;
#else
	m_videoBitRates = 600;
	 _rate_after_p2p = 1000;
	 m_maxFPS = 12;
	 m_sendVideoFps = 12;
#ifdef WEBRTC_ANDROID
	 m_sendVideoWidth = 320;
	 m_sendVideoHeight = 240;
#else
	 m_sendVideoWidth = 240;
	 m_sendVideoHeight = 320;
#endif
#endif

//    sean add end 0915
    processAudioData = false;
#ifdef HAIYUNTONG
    processAudioData = true;
#endif
    processOriginalAudioData = false;
    softMuteStatus = false;
//sean add begin 20140424 SetPrivateCloud
    memset(privateCloudCampanyID, 0, 200);
#ifndef XINWEI
    memcpy(privateCloudCampanyID, "yuntongxun1000", 14);
#endif
    memset(privateCloudCheckAddress, 0, 100);
    nativeCheck = false;
//sean add end 20140424 SetPrivateCloud
    isRefering = false;
//sean add begin 20140505 tls root ca
    rootCaPath = NULL;
//sean add end 20140505 tls root ca
    userKeySetted = false;
    referTo = NULL;
	speaker_volume = 255;
    groupID = new char[strlen("default")+1];
    if (groupID) {
        memcpy(groupID, "default", strlen("default"));
        groupID[strlen("default")]='\0';
    }
    else
    {
        PrintConsole("ERROR: mem alloc error, groupID");
    }

	networkType = new char[strlen("wifi")+1];
	if (networkType) {
		memcpy(networkType, "wifi", strlen("wifi"));
		networkType[strlen("wifi")]='\0';
	}
	else
	{
		PrintConsole("ERROR: mem alloc error, networkType");
	}
    groupIDAndNetworkType = NULL;
    videoConferenceIp = NULL;
    selfSipNo = NULL;
    registerUserdata = NULL;
    proxyAddr = NULL;
    remoteSipNo = NULL;
    isInVideoConference = false;
#ifdef HAIYUNTONG
    isLandingCall = false;
    isAudioConf = false;
    confID = NULL;
    deviceID = NULL;
    appID = NULL;
    enableHaiyuntong = true;
    pinCode = NULL;
    haiyuntongTestFlag = false;
#endif
#if 0
    ringbackFlag = true;
#endif
    tempAuth = false;
    reconnect = false;


	//svc parameters inital
	m_svcEnabled = false;
	m_spatial_layer_num = 1;
	m_temporal_layer_num = 1;
	m_origin_video_width = 0;
	m_origin_video_height = 0;
	m_origin_fps = 0;

	m_VideoTimeOut = 0;
//#ifdef _WIN32
//	m_ChromaKeyFilter = NULL;
//#endif

#ifdef ENABLE_REMB_TMMBR_CONFIG
	rembEnabled = false;
	tmmbrEnabled = false;
#endif

	m_pScreenInfo = NULL;
	m_pWindowInfo = NULL;

	m_videoModeChoose = 0;
	m_desktop_width = -1;
	m_desktop_height = -1;
	m_desktop_bit_rate = 0;
	m_desktop_frame_rate = 0;
    m_enable_fec = true;
    m_opus_packet_loss_rate = 0;

	//pSendStats_ = NULL;
	//pReceiveStats_ = NULL;
}

ServiceCore::~ServiceCore()
{
	if(m_cameraInfo)
    {
		for(int i=0; i< m_cameraCount;i++){
			delete []m_cameraInfo[i].capability;
			m_cameraInfo[i].capability = NULL;
		}
		delete []m_cameraInfo;
        m_cameraInfo = NULL;
    }
	if(m_speakerInfo)
    {
		delete m_speakerInfo;
        m_speakerInfo = NULL;
    }
	if(m_microphoneInfo)
    {
		delete m_microphoneInfo;
        m_microphoneInfo = NULL;
    }
//#ifdef _WIN32
//	if(m_ChromaKeyFilter)
//	{
//		delete m_ChromaKeyFilter;
//		m_ChromaKeyFilter = NULL;
//	}
//#endif

    delete m_criticalSection;
    m_criticalSection = NULL;
    free(m_SrtpKey);
	m_SrtpKey = NULL;
    if (rootCaPath) {

        delete [] rootCaPath;
        rootCaPath = NULL;
        ca_certificate_path = NULL;
    }
    if (referTo) {
        delete [] referTo;
        referTo = NULL;
    }

    if (groupID) {
        delete [] groupID;
        groupID = NULL;
    }

	if (networkType) {
		delete [] networkType;
		networkType = NULL;
	}

	if (groupIDAndNetworkType) {
		delete [] groupIDAndNetworkType;
		groupIDAndNetworkType = NULL;
	}
    if (videoConferenceIp) {
        free(videoConferenceIp);
        videoConferenceIp = NULL;
    }

    if (selfSipNo) {
        delete [] selfSipNo;
        selfSipNo = NULL;
    }
    if (registerUserdata) {
        delete [] registerUserdata;
        registerUserdata = NULL;
    }

    if (proxyAddr) {
        delete [] proxyAddr;
        proxyAddr = NULL;
    }

    if (remoteSipNo) {
        delete [] remoteSipNo;
        remoteSipNo = NULL;
    }
#ifdef HAIYUNTONG
    if (confID) {
        delete [] confID;
        confID = NULL;
    }
    if (deviceID) {
        delete [] deviceID;
        deviceID = NULL;
    }
    if (pinCode) {
        delete [] pinCode;
        pinCode = NULL;
    }
    if (appID) {
        delete [] appID;
        appID = NULL;
    }
#endif
}

int ServiceCore::sip_check_thread_active()
{
    int ret = sal_check_sip_thread_active();
    if (-1 == ret) {
        vtable.eXosip_thread_stop();
    }
    return 0;
}

void ServiceCore::sip_config_read()
{
	char *contact;
	const char *tmpstr;
	LCSipTransports tr;
	int i,tmp;
	int ipv6;

	tmp=lp_config_get_int(config,"sip","use_info",0);
	serphone_core_set_use_info_for_dtmf(tmp);

	if (lp_config_get_int(config,"sip","use_session_timers",1)==1){
		sal_use_session_timers(sal,200);
	}

	sal_use_rport(sal,lp_config_get_int(config,"sip","use_rport",1));
	sal_use_101(sal,lp_config_get_int(config,"sip","use_101",1));
	sal_reuse_authorization(sal, lp_config_get_int(config,"sip","reuse_authorization",0));

	tmp=lp_config_get_int(config,"sip","use_rfc2833",1);
	serphone_core_set_use_rfc2833_for_dtmf(tmp);

	ipv6=lp_config_get_int(config,"sip","use_ipv6",-1);
	if (ipv6==-1){
		ipv6=0;
	}
	serphone_core_enable_ipv6(ipv6);
	memset(&tr,0,sizeof(tr));

	if (lp_config_get_int(config,"sip","sip_random_port",0)) {
		tr.udp_port=(0xDFF&+random())+1024;
	} else {
		//tr.udp_port=lp_config_get_int(config,"sip","sip_port",DEFAULT_SIP_UDP_PORT);  //5060);deleted by zdm
        tr.udp_port=lp_config_get_int(config,"sip","sip_port",0);
	}
	if (lp_config_get_int(config,"sip","sip_tcp_random_port",1))
 //   if (lp_config_get_int(config,"sip","sip_tcp_random_port",1))
    {
		tr.tcp_port=(0xDFF&+random())+1024;
        PrintConsole("Using TCP port %d for SIP\n",tr.tcp_port);
	} else {
		tr.tcp_port=lp_config_get_int(config,"sip","sip_tcp_port",0);
	}
	if (lp_config_get_int(config,"sip","sip_tls_random_port",0)) {
		tr.tls_port=(0xDFF&+random())+1024;
	} else {
		tr.tls_port=lp_config_get_int(config,"sip","sip_tls_port",0);
	}

#ifdef __linux
	sal_set_root_ca(sal, lp_config_get_string(config,"sip","root_ca", "/etc/ssl/certs"));
#else
//	sal_set_root_ca(sal, lp_config_get_string(config,"sip","root_ca", ROOT_CA_FILE));
#endif
	serphone_core_verify_server_certificates(lp_config_get_int(config,"sip","verify_server_certs",TRUE));
	/*start listening on ports*/
 	serphone_core_set_sip_transports(&tr);

	tmpstr=lp_config_get_string(this->config,"sip","contact",NULL);
	if (tmpstr==NULL || serphone_core_set_primary_contact(tmpstr)==-1) {
		const char *hostname=NULL;
		const char *username=NULL;
#ifdef HAVE_GETENV
		hostname=getenv("HOST");
		username=getenv("USER");
		if (hostname==NULL) hostname=getenv("HOSTNAME");
#endif /*HAVE_GETENV*/
		if (hostname==NULL)
			hostname="unknown-host";
		if (username==NULL){
			username="toto";
		}
		contact=ser_strdup_printf("sip:%s@%s",username,hostname);
		serphone_core_set_primary_contact(contact);
		ms_free((void **)&contact);
	}

	tmp=lp_config_get_int(config,"sip","guess_hostname",0);
	serphone_core_set_guess_hostname(tmp);


	tmp=lp_config_get_int(config,"sip","inc_timeout",65);
	serphone_core_set_inc_timeout(tmp);

	/* get proxies config */
	for(i=0;; i++){
		SerphoneProxyConfig *cfg=serphone_proxy_config_new_from_config_file(this->config,i);
		if (cfg!=NULL){
			serphone_core_add_proxy_config(cfg);
		}else{
			break;
		}
	}
	/* get the default proxy */
	tmp=lp_config_get_int(this->config,"sip","default_proxy",-1);
	serphone_core_set_default_proxy_index(tmp);

	/* read authentication information */
	for(i=0;; i++){
		SerphoneAuthInfo *ai=serphone_auth_info_new_from_config_file(config,i);
		if (ai!=NULL){
			serphone_core_add_auth_info(ai);
			serphone_auth_info_destroy(&ai);
		}else{
			break;
		}
	}
	/*this is to filter out unsupported encryption schemes*/
//	linphone_core_set_media_encryption(lc,linphone_core_get_media_encryption(lc)); 暂时不处理加密RTP流工??
	/*for tuning or test*/
	sip_conf.sdp_200_ack=lp_config_get_int(config,"sip","sdp_200_ack",0);
	sip_conf.ping_with_options=lp_config_get_int(config,"sip","ping_with_options",0);
	sip_conf.register_only_when_network_is_up=
			lp_config_get_int(config,"sip","register_only_when_network_is_up",0);

	if( sip_conf.register_only_when_network_is_up ) {
		sip_conf.auto_net_state_mon = 1;
	}
	else {
		sip_conf.auto_net_state_mon=lp_config_get_int(config,"sip","auto_net_state_mon",0);
	}

	if( tr.tcp_port !=  0 )
		sip_conf.keepalive_period=lp_config_get_int(config,"sip","keepalive_period",60000);
	else
		sip_conf.keepalive_period=lp_config_get_int(config,"sip","keepalive_period",30000);

	sal_set_keepalive_period(this->sal,this->sip_conf.keepalive_period);
	sal_use_one_matching_codec_policy(this->sal,lp_config_get_int(config,"sip","only_one_codec",1));
	sal_use_double_registrations(this->sal,lp_config_get_int(config,"sip","use_double_registrations",1));
}

void ServiceCore::rtp_config_read()
{
	int port;
	int jitt_comp;
	int nortp_timeout;
	bool_t rtp_no_xmit_on_audio_mute;

	port=lp_config_get_int(config,"rtp","audio_rtp_port",7078);
	serphone_core_set_audio_port(port);

	port=lp_config_get_int(config,"rtp","video_rtp_port",9078);
	if (port==0) port=9078;
	serphone_core_set_video_port(port);

//sean add begin 20140705 video conference
    CursorVideoConferencePort = serphone_core_get_video_port();
//sean add end 20140705 video conference

	jitt_comp=lp_config_get_int(config,"rtp","audio_jitt_comp",60);
	serphone_core_set_audio_jittcomp(jitt_comp);
	jitt_comp=lp_config_get_int(config,"rtp","video_jitt_comp",60);
	if (jitt_comp==0) jitt_comp=60;
	rtp_conf.video_jitt_comp=jitt_comp;
	nortp_timeout=lp_config_get_int(config,"rtp","nortp_timeout",30);
	serphone_core_set_nortp_timeout(nortp_timeout);
	rtp_no_xmit_on_audio_mute=lp_config_get_int(config,"rtp","rtp_no_xmit_on_audio_mute",FALSE);
	serphone_core_set_rtp_no_xmit_on_audio_mute(rtp_no_xmit_on_audio_mute);
}

bool_t ServiceCore::already_a_call_pending()
{
	MSList *elem;
	for(elem=calls;elem!=NULL;elem=elem->next){
		SerPhoneCall *call=(SerPhoneCall*)elem->data;
		if (call->state==LinphoneCallIncomingReceived
		    || call->state==LinphoneCallOutgoingInit
		    || call->state==LinphoneCallOutgoingProgress
			|| call->state==LinphoneCallOutgoingProceeding
		    || call->state==LinphoneCallOutgoingEarlyMedia
		    || call->state==LinphoneCallOutgoingRinging){
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * Specifiies a ring back tone to be played to far end during incoming calls.
**/
void ServiceCore::serphone_core_set_remote_ringback_tone( const char *file){
	if (sound_conf.ringback_tone){
		ms_free((void **)&sound_conf.ringback_tone);
	}
	if (file)
		sound_conf.ringback_tone=ms_strdup(file);
}

/**
 * Returns the ring back tone played to far end during incoming calls.
**/
const char *ServiceCore::serphone_core_get_remote_ringback_tone()
{
	return sound_conf.ringback_tone;
}

bool_t ServiceCore::already_a_call_with_remote_address(const SerphoneAddress *remote)
{
	MSList *elem;
	for(elem=calls;elem!=NULL;elem=elem->next){
		SerPhoneCall *call=(SerPhoneCall*)elem->data;
		const SerphoneAddress *cRemote=serphone_call_get_remote_address(call);
		if (serphone_address_weak_equal(cRemote,remote)) {
			return TRUE;
		}
	}
	return FALSE;
}

SerPhoneCall* ServiceCore::serphone_core_find_call_by_cid( int cid)
{
//	cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	MSList *elem;
	for(elem=calls;elem!=NULL;elem=elem->next){
		SerPhoneCall *call=(SerPhoneCall*)elem->data;
		if(!call || !call->op)
			continue;
		if(call->op->cid == cid)
			return call;
	}
	return NULL;
}

SerPhoneCall* ServiceCore::serphone_core_find_call_by_user_cid(const char* cid)
{
    //	cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	MSList *elem;
	for(elem=calls;elem!=NULL;elem=elem->next){
		SerPhoneCall *call=(SerPhoneCall*)elem->data;
		if(!call || !call->op)
			continue;
//		if(call->op->cid == cid)
        if (strncmp(cid, call->_user_call_id, 8)==0) {
            return call;
        }
	}
	return NULL;
}
//SerPhoneCall* ServiceCore::serphone_core_find_call_by_audio_channel_id( int channel_id)
//{
//    MSList *elem;
//	for(elem=calls;elem!=NULL;elem=elem->next){
//		SerPhoneCall *call=(SerPhoneCall*)elem->data;
//		if(!call || !call->op)
//			continue;
//		if(call->m_AudioChannelID == channel_id)
//			return call;
//	}
//	return NULL;
//}
//
//SerPhoneCall* ServiceCore::serphone_core_find_call_by_video_channel_id( int channel_id)
//{
//    MSList *elem;
//	for(elem=calls;elem!=NULL;elem=elem->next){
//		SerPhoneCall *call=(SerPhoneCall*)elem->data;
//		if(!call || !call->op)
//			continue;
//		if(call->m_VideoChannelID == channel_id)
//			return call;
//	}
//	return NULL;
//}

//const char *ServiceCore::get_user_call_id()
//{
//    return this->_user_call_id;
//}

//void ServiceCore::set_user_call_id(const char * user_call_id)
//{
//    memcpy(_user_call_id, user_call_id, 8);
//    _user_call_id[8] = '\0';
//}

//sean add begin 0915
void ServiceCore::serphone_set_mosaic(bool flag)
{
    _shield_mosaic = flag;
}
void ServiceCore::serphone_set_rate_p2p(int rate)
{
    _rate_after_p2p = rate;
}
//sean add end 0915

int ServiceCore::serphone_core_get_calls_nb()
{
	return  ms_list_size(calls);
}

bool_t ServiceCore::serphone_core_can_we_add_call()
{
	if(serphone_core_get_calls_nb() < max_calls)
		return TRUE;
	PrintConsole("Maximum amount of simultaneous calls reached !\n");
	return FALSE;
}

int ServiceCore::serphone_core_add_call(SerPhoneCall *call)
{
	if(serphone_core_can_we_add_call())
	{
		calls = ms_list_append(calls,call);
		return 0;
	}
	return -1;
}

int ServiceCore::serphone_core_del_call(SerPhoneCall *call)
{
	MSList *it;
	MSList *the_calls = calls;

	it=ms_list_find(the_calls,call);
	if (it)
	{
		the_calls = ms_list_remove_link(the_calls,it);
	}
	else
	{
		PrintConsole("could not find the call into the list\n");
		return -1;
	}
	calls = the_calls;
	return 0;
}

ortp_socket_t ServiceCore::serphone_core_get_sip_socket()
{
	 return sal_get_socket(sal);
}

 void ServiceCore::serphone_core_set_inc_timeout(int seconds)
 {
	 sip_conf.inc_timeout=seconds;
 }

 int ServiceCore::serphone_core_get_inc_timeout()
 {
     return sip_conf.inc_timeout;
 }

const char *ServiceCore::serphone_core_get_nat_address_resolved()
{
	struct sockaddr_storage ss;
	socklen_t ss_len;
	int error;
	char ipstring[INET6_ADDRSTRLEN];

	if (net_conf.nat_address==NULL) return NULL;

	if (parse_hostname_to_addr (net_conf.nat_address, &ss, &ss_len)<0) {
		return net_conf.nat_address;
	}

	error = getnameinfo((struct sockaddr *)&ss, ss_len,
		ipstring, sizeof(ipstring), NULL, 0, NI_NUMERICHOST);
	if (error) {
		return net_conf.nat_address;
	}

	if (net_conf.nat_address_ip!=NULL){
		ms_free((void **)&net_conf.nat_address_ip);
	}
	net_conf.nat_address_ip = ms_strdup (ipstring);
	return net_conf.nat_address_ip;
}

/*result must be an array of chars at least SERPHONE_IPADDR_SIZE */
void ServiceCore::serphone_core_get_local_ip(const char *dest, char *result)
{
	const char *ip;
	if (serphone_core_get_firewall_policy()==LinphonePolicyUseNatAddress
	    && (ip=serphone_core_get_nat_address_resolved())!=NULL){
		strncpy(result,ip,SERPHONE_IPADDR_SIZE);
		return;
	}
	if (serphone_core_get_local_ip_for(sip_conf.ipv6_enabled ? AF_INET6 : AF_INET,dest,result)==0)
		return;
	/*else fallback to SAL routine that will attempt to find the most realistic interface */
	sal_get_default_local_ip(sal,sip_conf.ipv6_enabled ? AF_INET6 : AF_INET,result,SERPHONE_IPADDR_SIZE);
}

/**
 * Sets the ports to be used for each of transport (UDP or TCP)
 *
 * A zero value port for a given transport means the transport
 * is not used.
 *
 * @ingroup network_parameters
**/
int ServiceCore::serphone_core_set_sip_transports(const LCSipTransports * tr)
{

	if (transports_unchanged(tr,&sip_conf.transports))
		return 0;
	memcpy(&sip_conf.transports,tr,sizeof(*tr));

	if (serphone_core_ready()){
		lp_config_set_int(config,"sip","sip_port",tr->udp_port);
		lp_config_set_int(config,"sip","sip_tcp_port",tr->tcp_port);
		lp_config_set_int(config,"sip","sip_tls_port",tr->tls_port);
	}

	if (sal==NULL) return 0;
	return apply_transports();
}

/**
 * Retrieves the ports used for each transport (udp, tcp).
 * A zero value port for a given transport means the transport
 * is not used.
 * @ingroup network_parameters
**/
int ServiceCore::serphone_core_get_sip_transports(LCSipTransports *tr)
{
	memcpy(tr,&sip_conf.transports,sizeof(*tr));
	return 0;
}

/**
 * Returns the UDP port used by SIP.
 *
 * Deprecated: use serphone_core_get_sip_transports() instead.
 * @ingroup network_parameters
**/
int ServiceCore::serphone_core_get_sip_port()
{
	LCSipTransports *tr=&sip_conf.transports;
	return tr->udp_port>0 ? tr->udp_port : (tr->tcp_port > 0 ? tr->tcp_port : tr->tls_port);
}

/**
 * Indicates whether SIP INFO is used for sending digits.
 *
 * @ingroup media_parameters
**/
bool_t ServiceCore::serphone_core_get_use_info_for_dtmf()
{
	return sip_conf.use_info;
}

void ServiceCore::serphone_core_set_use_info_for_dtmf(bool_t use_info)
{
	sip_conf.use_info=use_info;
}

/**
 * Indicates whether RFC2833 is used for sending digits.
 *
**/
bool_t ServiceCore::serphone_core_get_use_rfc2833_for_dtmf()
{
	return sip_conf.use_rfc2833;
}

/**
 * Sets whether RFC2833 is to be used for sending digits.
 *
 * @ingroup media_parameters
**/
void ServiceCore::serphone_core_set_use_rfc2833_for_dtmf(bool_t use_rfc2833)
{
	sip_conf.use_rfc2833=use_rfc2833;
}

/**
 * Turns IPv6 support on or off.
 *
 * @ingroup network_parameters
 *
 * @note IPv6 support is exclusive with IPv4 in liblinphone:
 * when IPv6 is turned on, IPv4 calls won't be possible anymore.
 * By default IPv6 support is off.
**/
void ServiceCore::serphone_core_enable_ipv6(bool_t val)
{
	if (sip_conf.ipv6_enabled!=val){
		sip_conf.ipv6_enabled=val;
		if (sal){
			/* we need to restart eXosip */
			apply_transports();
		}
	}
}

bool_t ServiceCore::serphone_core_ipv6_enabled()
{
	return sip_conf.ipv6_enabled;
}

int ServiceCore::apply_transports()
{
	Sal *l_sal=this->sal;
	const char *anyaddr;
	LCSipTransports *tr=&sip_conf.transports;

	/*first of all invalidate all current registrations so that we can register again with new transports*/
	__serphone_core_invalidate_registers();

	if( strlen(g_bind_local_addr) > 0)
		anyaddr = (char*)&g_bind_local_addr;
	else if (sip_conf.ipv6_enabled)
		anyaddr="::0";
	else
		anyaddr="0.0.0.0";

	sal_unlisten_ports(l_sal);
	if (tr->udp_port>0){
		if (sal_listen_port (l_sal,anyaddr,tr->udp_port,SalTransportUDP,FALSE)!=0){
			int i=1;
			for(int i=1; i<=20; i++) {
				if (!sal_listen_port (l_sal,anyaddr,tr->udp_port+i,SalTransportUDP,FALSE)){
					PrintConsole("UDP port %d maybe already used, change to %d", tr->udp_port, tr->udp_port+i);
					tr->udp_port += i;
					break;
				}
			}
			if(i>20) {
				PrintConsole("UDP port %d~%d maybe already used", tr->udp_port, tr->udp_port+i);
				transport_error("udp",tr->udp_port);
				return -1;
			}
		}
	}
	if (tr->tcp_port>0){
		if (sal_listen_port (l_sal,anyaddr,tr->tcp_port,SalTransportTCP,FALSE)!=0){
			int i=1;
			for(int i=1; i<=20; i++) {
				if (!sal_listen_port (l_sal,anyaddr,tr->tcp_port+i,SalTransportTCP,FALSE)){
					PrintConsole("TCP port %d maybe already used, change to %d", tr->tcp_port, tr->tcp_port+i);
					tr->tcp_port += i;
					break;
				}
			}
			if(i>20) {
				PrintConsole("TCP port %d~%d maybe already used", tr->tcp_port, tr->tcp_port+i);
				transport_error("tcp",tr->tcp_port);
				return -1;
			}
		}
	}
	if (tr->tls_port>0){
		if (sal_listen_port (l_sal,anyaddr,tr->tls_port,SalTransportTLS,TRUE)!=0){

			int i=1;
			for(int i=1; i<=20; i++) {
				if (!sal_listen_port (l_sal,anyaddr,tr->tls_port+i,SalTransportTLS,TRUE)){
					PrintConsole("TLS port %d maybe already used, change to %d", tr->tls_port, tr->tls_port+i);
					tr->tls_port += i;
					break;
				}
			}
			if(i>20) {
				PrintConsole("TLS port %d~%d maybe already used", tr->tls_port, tr->tls_port+i);
				transport_error("tls",tr->tls_port);
				return -1;
			}
		}
	}
	apply_user_agent(NULL);
	return 0;
}

static const char *_ua_name="";
static char _ua_version[64]=SERPHONE_VERSION;


void ServiceCore::apply_user_agent(const char *agent)
{
	char ua_string[256];
	if(agent)
		_ua_name = agent;
	snprintf(ua_string,sizeof(ua_string)-1,"Hisunsray CCPClient:%s:%s",
		_ua_version,_ua_name);
	if(sal)
		sal_set_user_agent(sal,ua_string);
}

void ServiceCore::transport_error(const char* transport, int port)
{
	char *msg=ser_strdup_printf("Could not start %s transport on port %i, maybe this port is already used.",transport,port);
	PrintConsole("%s\n",msg);
	if (vtable.display_warning)
		vtable.display_warning(this,msg);
	ms_free((void **)&msg);
}

void ServiceCore::__serphone_core_invalidate_registers()
{
	const MSList *elem=serphone_core_get_proxy_config_list();
	for(;elem!=NULL;elem=elem->next){
		SerphoneProxyConfig *cfg=(SerphoneProxyConfig*)elem->data;
		if (cfg->serphone_proxy_config_register_enabled()) {

            if (traceFile) {
                time_t temp = time(NULL);
                std::string strTime = timetodate(temp);
                fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                fwrite(" Something goes wrong Unregister called4\n", strlen(" Something goes wrong Unregister called4\n"), 1, traceFile);
                fflush(traceFile);
            }

			cfg->serphone_proxy_config_edit();
			cfg->serphone_proxy_config_done();
		}
	}
}

/**
 * Returns the default identity when no proxy configuration is used.
 *
 * @ingroup proxies
**/
const char *ServiceCore::serphone_core_get_primary_contact()
{
	char *identity;

	if (sip_conf.guess_hostname){
		if (sip_conf.guessed_contact==NULL || sip_conf.loopback_only){
			update_primary_contact();
		}
		identity=sip_conf.guessed_contact;
	}else{
		identity=sip_conf.contact;
	}
	return identity;
}

int ServiceCore::serphone_core_set_primary_contact(const char *contact)
{
	SerphoneAddress *ctt;

	if ((ctt=serphone_address_new(contact))==0) {
		PrintConsole("Bad contact url: %s\n",contact);
		return -1;
	}
	if (sip_conf.contact!=NULL) {ms_free((void **)&sip_conf.contact);sip_conf.contact = NULL;}
	sip_conf.contact=ms_strdup(contact);

	if (sip_conf.guessed_contact!=NULL){
		ms_free((void **)&sip_conf.guessed_contact);
	}
	serphone_address_destroy(ctt);
	return 0;
}

/**
 * Tells SerphoneCore to guess local hostname automatically in primary contact.
 *
 * @ingroup proxies
**/
void ServiceCore::serphone_core_set_guess_hostname( bool_t val)
{
	sip_conf.guess_hostname=val;
}

/**
 * Returns TRUE if hostname part of primary contact is guessed automatically.
 *
 * @ingroup proxies
**/
bool_t ServiceCore::serphone_core_get_guess_hostname()
{
	return sip_conf.guess_hostname;
}

/**
 * Same as serphone_core_get_primary_contact_parsed() but the result is a SerphoneAddress object
 * instead of const char*
 *
 * @ingroup proxies
**/
SerphoneAddress *ServiceCore::serphone_core_get_primary_contact_parsed()
{
	return serphone_address_new(serphone_core_get_primary_contact());
}

void ServiceCore::update_primary_contact()
{
	char *guessed=NULL;
	char tmp[SERPHONE_IPADDR_SIZE];

	SerphoneAddress *url;
	if (sip_conf.guessed_contact!=NULL){
		ms_free((void **)&sip_conf.guessed_contact);
	}
	url=serphone_address_new(sip_conf.contact);
	if (!url){
		PrintConsole("Could not parse identity contact !\n");
		url=serphone_address_new("sip:unknown@unkwownhost");
	}
	serphone_core_get_local_ip(NULL, tmp);
	if (strcmp(tmp,"127.0.0.1")==0 || strcmp(tmp,"::1")==0 ){
		PrintConsole("Local loopback network only !\n");
		sip_conf.loopback_only=TRUE;
	}else sip_conf.loopback_only=FALSE;
	serphone_address_set_domain(url,tmp);
	serphone_address_set_port_int(url,serphone_core_get_sip_port ());
	guessed=serphone_address_as_string(url);
	sip_conf.guessed_contact=guessed;
	serphone_address_destroy(url);
}

/**
 * Returns the default identity SIP address.
 *
 * @ingroup proxies
 * This is an helper function:
 *
 * If no default proxy is set, this will return the primary contact (
 * see serphone_core_get_primary_contact() ). If a default proxy is set
 * it returns the registered identity on the proxy.
**/
const char * ServiceCore::serphone_core_get_identity()
{
	SerphoneProxyConfig *proxy=NULL;
	const char *from;
	serphone_core_get_default_proxy(&proxy);
	if (proxy!=NULL) {
		from=proxy->serphone_proxy_config_get_identity();
	}else from=serphone_core_get_primary_contact();
	return from;
}

 void ServiceCore::serphone_core_set_stun_server(const char *server)
 {
 	if (net_conf.stun_server!=NULL)
    {
        ms_free((void **)&net_conf.stun_server);
    }
	if (server)
		net_conf.stun_server=ms_strdup(server);
	else net_conf.stun_server=NULL;
}

const char * ServiceCore::serphone_core_get_stun_server()
{
	return net_conf.stun_server;
}


int ServiceCore::sendStunRequest(int sock, const struct sockaddr *server, socklen_t addrlen, int id, bool_t changeAddr){
	char buf[STUN_MAX_MESSAGE_SIZE];
	int len = STUN_MAX_MESSAGE_SIZE;
	StunAtrString username;
   	StunAtrString password;
	StunMessage req;
	int err;
	memset(&req, 0, sizeof(StunMessage));
	memset(&username,0,sizeof(username));
	memset(&password,0,sizeof(password));
	stunBuildReqSimple( &req, &username, changeAddr , changeAddr , id);
	len = stunEncodeMessage( &req, buf, len, &password);
	if (len<=0){
		PrintConsole("Fail to encode stun message.\n");
		return -1;
	}
	err=sendto(sock,buf,len,0,server,addrlen);
	if (err<0){
		PrintConsole("sendto failed: %s\n",strerror(errno));
		return -1;
	}
	return 0;
}


//int ServiceCore::recvStunResponse(ortp_socket_t sock, char *ipaddr, int *port, int *id,SerPhoneCall *call){
int ServiceCore::recvStunResponse(ortp_socket_t sock, char *ipaddr, int *port, int *id){
	char buf[STUN_MAX_MESSAGE_SIZE];
   	int len = STUN_MAX_MESSAGE_SIZE;
	StunMessage resp;
	len=recv(sock,buf,len,0);
	if (len>0){
		struct in_addr ia;
		stunParseMessage(buf,len, &resp );
		*id=resp.msgHdr.tr_id.octet[0];
		if (resp.hasXorMappedAddress){
			*port = resp.xorMappedAddress.ipv4.port;
			ia.s_addr=htonl(resp.xorMappedAddress.ipv4.addr);
		}else if (resp.hasMappedAddress){
			*port = resp.mappedAddress.ipv4.port;
			ia.s_addr=htonl(resp.mappedAddress.ipv4.addr);
		}else return -1;
		strncpy(ipaddr,inet_ntoa(ia),SERPHONE_IPADDR_SIZE);
	}
	return len;
}


void ServiceCore::update_media_description_from_stun(SalMediaDescription *md, const StunCandidate *ac, const StunCandidate *vc)
{
    int i;
	for (i = 0; i < md->n_active_streams; i++) {
		if ((md->streams[i].type == SalAudio) && (ac->port != 0)) {
			strcpy(md->streams[0].rtp_addr,ac->addr);
			md->streams[0].rtp_port=ac->port;
			if ((ac->addr[0]!='\0' && vc->addr[0]!='\0' && strcmp(ac->addr,vc->addr)==0) || md->n_active_streams==1){
				strcpy(md->addr,ac->addr);
			}
		}
		if ((md->streams[i].type == SalVideo) && (vc->port != 0)) {
			strcpy(md->streams[1].rtp_addr,vc->addr);
			md->streams[1].rtp_port=vc->port;
		}
	}
}

void ServiceCore::serphone_core_update_local_media_description_from_ice(SalMediaDescription *desc, IceSession *session)
{
//    printf("%s called\n",__FUNCTION__);
	const char *rtp_addr, *rtcp_addr;
	IceSessionState session_state = ice_session_state(session);
	int nb_candidates;
	int i, j;
	bool_t result;

	if (session_state == IS_Completed) {
		desc->ice_completed = TRUE;
		result = ice_check_list_selected_valid_local_candidate(ice_session_check_list(session, 0), &rtp_addr, NULL, NULL, NULL);
		if (result == TRUE) {
			strncpy(desc->addr, rtp_addr, sizeof(desc->addr));
		} else {
			PrintConsole("If ICE has completed successfully, rtp_addr should be set!\n");
//            printf("If ICE has completed successfully, rtp_addr should be set!\n");
		}
	}
	else {
        PrintConsole("If ICE has not completed yet, wait...!\n");
		desc->ice_completed = FALSE;
	}

	strncpy(desc->ice_pwd, ice_session_local_pwd(session), sizeof(desc->ice_pwd));
	strncpy(desc->ice_ufrag, ice_session_local_ufrag(session), sizeof(desc->ice_ufrag));
	for (i = 0; i < desc->n_active_streams; i++) {
		SalStreamDescription *stream = &desc->streams[i];
		IceCheckList *cl = ice_session_check_list(session, i);
		nb_candidates = 0;
		if (cl == NULL) continue;
		if (ice_check_list_state(cl) == ICL_Completed) {
			stream->ice_completed = TRUE;
			result = ice_check_list_selected_valid_local_candidate(ice_session_check_list(session, i), &rtp_addr, &stream->rtp_port, &rtcp_addr, &stream->rtcp_port);
		} else {
			stream->ice_completed = FALSE;
			result = ice_check_list_default_local_candidate(ice_session_check_list(session, i), &rtp_addr, &stream->rtp_port, &rtcp_addr, &stream->rtcp_port);
		}
		if (result == TRUE) {
			strncpy(stream->rtp_addr, rtp_addr, sizeof(stream->rtp_addr));
			strncpy(stream->rtcp_addr, rtcp_addr, sizeof(stream->rtcp_addr));
		} else {
			memset(stream->rtp_addr, 0, sizeof(stream->rtp_addr));
			memset(stream->rtcp_addr, 0, sizeof(stream->rtcp_addr));
		}
		if ((strlen(ice_check_list_local_pwd(cl)) != strlen(desc->ice_pwd)) || (strcmp(ice_check_list_local_pwd(cl), desc->ice_pwd)))
        {
			strncpy(stream->ice_pwd, ice_check_list_local_pwd(cl), sizeof(stream->ice_pwd));
        }
		else
        {
			memset(stream->ice_pwd, 0, sizeof(stream->ice_pwd));
        }
		if ((strlen(ice_check_list_local_ufrag(cl)) != strlen(desc->ice_ufrag)) || (strcmp(ice_check_list_local_ufrag(cl), desc->ice_ufrag)))
        {
			strncpy(stream->ice_ufrag, ice_check_list_local_ufrag(cl), sizeof(stream->ice_ufrag));
        }
		else
        {
			memset(stream->ice_pwd, 0, sizeof(stream->ice_pwd));
        }
		stream->ice_mismatch = ice_check_list_is_mismatch(cl);
		if ((ice_check_list_state(cl) == ICL_Running) || (ice_check_list_state(cl) == ICL_Completed)) {
			memset(stream->ice_candidates, 0, sizeof(stream->ice_candidates));
			for (j = 0; j < MIN(ms_list_size(cl->local_candidates), SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES); j++) {
				SalIceCandidate *sal_candidate = &stream->ice_candidates[nb_candidates];
				IceCandidate *ice_candidate = (IceCandidate *)ms_list_nth_data(cl->local_candidates, j);
				const char *default_addr = NULL;
				int default_port = 0;
				if (ice_candidate->componentID == 1) {
					default_addr = stream->rtp_addr;
					default_port = stream->rtp_port;
				} else if (ice_candidate->componentID == 2) {
					default_addr = stream->rtcp_addr;
					default_port = stream->rtcp_port;
				} else continue;
				if (default_addr[0] == '\0') {
                    default_addr = desc->addr;
                }
				/* Only include the candidates matching the default destination for each component of the stream if the state is Completed as specified in RFC5245 section 9.1.2.2. */
				if ((ice_check_list_state(cl) == ICL_Completed)
					&& !((ice_candidate->taddr.port == default_port) && (strlen(ice_candidate->taddr.ip) == strlen(default_addr)) && (strcmp(ice_candidate->taddr.ip, default_addr) == 0)))
                {
					continue;
                }
				strncpy(sal_candidate->foundation, ice_candidate->foundation, sizeof(sal_candidate->foundation));
				sal_candidate->componentID = ice_candidate->componentID;
				sal_candidate->priority = ice_candidate->priority;
				strncpy(sal_candidate->type, ice_candidate_type(ice_candidate), sizeof(sal_candidate->type));
				strncpy(sal_candidate->addr, ice_candidate->taddr.ip, sizeof(sal_candidate->addr));
				sal_candidate->port = ice_candidate->taddr.port;
				if ((ice_candidate->base != NULL) && (ice_candidate->base != ice_candidate)) {
					strncpy(sal_candidate->raddr, ice_candidate->base->taddr.ip, sizeof(sal_candidate->raddr));
					sal_candidate->rport = ice_candidate->base->taddr.port;
				}
				nb_candidates++;
			}//END for (j = 0; j < MIN(ms_list_size(cl->local_candidates), SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES); j++)
		}
        //only for caller
		if ((ice_check_list_state(cl) == ICL_Completed) && (ice_session_role(session) == IR_Controlling)) {
			int rtp_port, rtcp_port;
			memset(stream->ice_remote_candidates, 0, sizeof(stream->ice_remote_candidates));
			if (ice_check_list_selected_valid_remote_candidate(cl, &rtp_addr, &rtp_port, &rtcp_addr, &rtcp_port) == TRUE) {
				strncpy(stream->ice_remote_candidates[0].addr, rtp_addr, sizeof(stream->ice_remote_candidates[0].addr));
				stream->ice_remote_candidates[0].port = rtp_port;
				strncpy(stream->ice_remote_candidates[1].addr, rtcp_addr, sizeof(stream->ice_remote_candidates[1].addr));
				stream->ice_remote_candidates[1].port = rtcp_port;
			} else {
				PrintConsole("ice: Selected valid remote candidates should be present if the check list is in the Completed state");
			}
		} else {
			for (j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_ICE_REMOTE_CANDIDATES; j++) {
				stream->ice_remote_candidates[j].addr[0] = '\0';
				stream->ice_remote_candidates[j].port = 0;
			}
		}
	}
}


void ServiceCore::get_default_addr_and_port(uint16_t componentID, const SalMediaDescription *md, const SalStreamDescription *stream, const char **addr, int *port)
{
	if (componentID == 1) {
		*addr = stream->rtp_addr;
		*port = stream->rtp_port;
	} else if (componentID == 2) {
		*addr = stream->rtcp_addr;
		*port = stream->rtcp_port;
	} else {
        return;
    }
	if ((*addr)[0] == '\0') {
        *addr = md->addr;
    }
}

int ServiceCore::serphone_core_run_stun_tests(SerPhoneCall *call)
{
//	const char *server=serphone_core_get_stun_server();
//	StunCandidate *ac=&call->ac;
//	StunCandidate *vc=&call->vc;
//	if (this->sip_conf.ipv6_enabled){
//		PrintConsole("stun support is not implemented for ipv6");
//		return -1;
//	}
//	if (server!=NULL){
//		struct sockaddr_storage ss;
//		socklen_t ss_len;
//		ortp_socket_t sock1=-1, sock2=-1;
//		int loops=0;
//		bool_t video_enabled=serphone_core_video_enabled();
//		bool_t got_audio,got_video;
//		bool_t cone_audio=FALSE,cone_video=FALSE;
//		struct timeval init,cur;
//		double elapsed;
//		int ret=0;
//
//		if (parse_hostname_to_addr(server,&ss,&ss_len)<0){
//			PrintConsole("Fail to parser stun server address: %s\n",server);
//			return -1;
//		}
//		if (this->vtable.display_status!=NULL)
//			this->vtable.display_status(this,_("Stun lookup in progress..."));
//
//		/*create the two audio and video RTP sockets, and send STUN message to our stun server */
//		sock1=create_socket(call->audio_port);
//		if (sock1==-1) return -1;
//		if (video_enabled){
//			sock2=create_socket(call->video_port);
//			if (sock2==-1) return -1;
//		}
//		got_audio=FALSE;
//		got_video=FALSE;
//		gettimeofday(&init,NULL);
//		do{
//
//			int id;
//			if (loops%20==0){
//				PrintConsole("Sending stun requests...\n");
//				sendStunRequest(sock1,(struct sockaddr*)&ss,ss_len,11,TRUE);
//				sendStunRequest(sock1,(struct sockaddr*)&ss,ss_len,1,FALSE);
//				if (sock2!=-1){
//					sendStunRequest(sock2,(struct sockaddr*)&ss,ss_len,22,TRUE);
//					sendStunRequest(sock2,(struct sockaddr*)&ss,ss_len,2,FALSE);
//				}
//			}
//#ifdef WIN32
//			Sleep(10);
//#else
//			usleep(10000);
//#endif
//
//			if (recvStunResponse(sock1,ac->addr,
//                                 &ac->port,&id)>0){
//				PrintConsole("STUN test result: local audio port maps to %s:%i\n",
//                           ac->addr,
//                           ac->port);
//				if (id==11)
//					cone_audio=TRUE;
//				got_audio=TRUE;
//			}
//			if (recvStunResponse(sock2,vc->addr,
//                                 &vc->port,&id)>0){
//				PrintConsole("STUN test result: local video port maps to %s:%i\n",
//                           vc->addr,
//                           vc->port);
//				if (id==22)
//					cone_video=TRUE;
//				got_video=TRUE;
//			}
//			gettimeofday(&cur,NULL);
//			elapsed=((cur.tv_sec-init.tv_sec)*1000.0) +  ((cur.tv_usec-init.tv_usec)/1000.0);
//			if (elapsed>2000)  {
//				PrintConsole("Stun responses timeout, going ahead.\n");
//				ret=-1;
//				break;
//			}
//			loops++;
//		}while(!(got_audio && (got_video||sock2==-1)  ) );
//		if (ret==0) ret=(int)elapsed;
//		if (!got_audio){
//			PrintConsole("No stun server response for audio port.\n");
//		}else{
//			if (!cone_audio) {
//				PrintConsole("NAT is symmetric for audio port\n");
//			}
//		}
//		if (sock2!=-1){
//			if (!got_video){
//				PrintConsole("No stun server response for video port.\n");
//			}else{
//				if (!cone_video) {
//					PrintConsole("NAT is symmetric for video port.\n");
//				}
//			}
//		}
//		close_socket(sock1);
//		if (sock2!=-1) close_socket(sock2);
//		return ret;
//	}
	return -1;
}

int ServiceCore::serphone_core_check_account_online(char *account)
{
	SalOp *op=sal_op_new(sal);

	const char *from=NULL;
	char *real_url=NULL;
	const char *contact=NULL;
	SerphoneProxyConfig *proxy=NULL;
	SerphoneProxyConfig *dest_proxy=NULL;
	SerphoneAddress *addr=NULL;
	Account_Status status = account_Status_None;

	addr = serphone_core_interpret_url(account);
	sal_address_set_transport(addr,SalTransportTCP);
	real_url=serphone_address_as_string(addr);

	serphone_core_get_default_proxy(&proxy);
	dest_proxy=serphone_core_lookup_known_proxy(addr);
	if (proxy!=dest_proxy && dest_proxy!=NULL) {
		PrintConsole("The used identity will be %s\n",dest_proxy->serphone_proxy_config_get_identity());
	}
	if (dest_proxy!=NULL)
		from=dest_proxy->serphone_proxy_config_get_identity();
	else if (proxy!=NULL)
		from=proxy->serphone_proxy_config_get_identity();

	/* if no proxy or no identity defined for this proxy, default to primary contact*/
	if (from==NULL) from=serphone_core_get_primary_contact();

	op->check_account_online = true;
	sal_op_set_user_pointer(op, &status);

	sal_check_account_online(op, from, real_url);
	if (real_url!=NULL)
    {
        ms_free((void **)&real_url);
    }

	time_t start_time = time(NULL);
	while(1) {
		if( (time(NULL) - start_time) > 3) {
			status = account_Status_TimeOut;
			break;
		}
		if(status != account_Status_None) {
			break;
		}

#ifdef _WIN32
        Sleep(20);
#else
        timespec t;
        t.tv_sec = 0;
        t.tv_nsec = 20*1000000;
        nanosleep(&t,NULL);
#endif
	}
	sal_op_release(op);
	return status;
}

void ServiceCore::serphone_call_delete_ice_session(SerPhoneCall *call){
	if (call->ice_session != NULL) {
		ice_session_destroy(call->ice_session);
		call->ice_session = NULL;
		if (call->audiostream != NULL) call->audiostream->ms.ice_check_list = NULL;
		if (call->videostream != NULL) call->videostream->ms.ice_check_list = NULL;
		call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = SerPhoneIceStateNotActivated;
		call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = SerPhoneIceStateNotActivated;
	}
}

void ServiceCore::serphone_core_set_nat_address( const char *addr)
{
	if (net_conf.nat_address!=NULL){
		ms_free((void **)&net_conf.nat_address);
	}
	if (addr!=NULL){
		net_conf.nat_address=ms_strdup(addr);
	}
	else net_conf.nat_address=NULL;
	if (sip_conf.contact) update_primary_contact();
}

const char *ServiceCore::serphone_core_get_nat_address()
{
	return net_conf.nat_address;
}

void ServiceCore::serphone_core_set_firewall_policy( SerphoneFirewallPolicy pol)
{
	net_conf.firewall_policy=pol;
	if (sip_conf.contact) update_primary_contact();
}

SerphoneFirewallPolicy ServiceCore::serphone_core_get_firewall_policy()
{
//#ifndef XINWEI
//    if (remoteSipNo) {
//        if(strncmp(remoteSipNo, "8", 1) == 0 && strlen(remoteSipNo) >= 14 )
//            return (SerphoneFirewallPolicy)net_conf.firewall_policy;
//        else
//            return LinphonePolicyNoFirewall;
//    }
//    else
//        return (SerphoneFirewallPolicy)net_conf.firewall_policy;
//#else
//    return (SerphoneFirewallPolicy)net_conf.firewall_policy;
//#endif


//    sean test for ice
    return LinphonePolicyUseIce;
//    return LinphonePolicyNoFirewall;
}

 const char * ServiceCore::serphone_core_get_relay_addr()
 {
	 	return net_conf.relay;
 }

 int ServiceCore::serphone_core_set_relay_addr(const char *addr)
 {
 	if (net_conf.relay!=NULL){
		ms_free((void **)&net_conf.relay);
	}
	if (addr){
		net_conf.relay=ms_strdup(addr);
	}
	return 0;
}

/**
 * Sets the nominal audio jitter buffer size in milliseconds.
 *
 * @ingroup media_parameters
**/
void ServiceCore::serphone_core_set_audio_jittcomp( int value)
{
	rtp_conf.audio_jitt_comp=value;
}

void ServiceCore::serphone_core_set_rtp_no_xmit_on_audio_mute(bool_t rtp_no_xmit_on_audio_mute)
{
	rtp_conf.rtp_no_xmit_on_audio_mute=rtp_no_xmit_on_audio_mute;
}

void ServiceCore::serphone_core_set_nortp_timeout(int nortp_timeout)
{
	rtp_conf.nortp_timeout=nortp_timeout;
}

bool_t ServiceCore::serphone_core_get_rtp_no_xmit_on_audio_mute( )
{
	return rtp_conf.rtp_no_xmit_on_audio_mute;
}

/**
 * Returns the value in seconds of the no-rtp timeout.
 *
 * @ingroup media_parameters
 * When no RTP or RTCP packets have been received for a while
 * ServiceCore will consider the call is broken (remote end crashed or
 * disconnected from the network), and thus will terminate the call.
 * The no-rtp timeout is the duration above which the call is considered broken.
**/
int  ServiceCore::serphone_core_get_nortp_timeout()
{
	return rtp_conf.nortp_timeout;
}

/**
 * Add a proxy configuration.
 * This will start registration on the proxy, if registration is enabled.
**/
int ServiceCore::serphone_core_add_proxy_config(SerphoneProxyConfig *cfg)
{
	if (!serphone_proxy_config_check(cfg)) {
		return -1;
	}
	if (ms_list_find(sip_conf.proxies,cfg)!=NULL){
		PrintConsole("ProxyConfig already entered, ignored.\n");
		return 0;
	}
	sip_conf.proxies=ms_list_append(sip_conf.proxies,(void *)cfg);
	cfg->serphone_proxy_config_apply(this);
	return 0;
}

int ServiceCore::serphone_core_get_default_proxy(SerphoneProxyConfig **config)
{
	int pos=-1;
	if (config!=NULL) *config=default_proxy;
	if (default_proxy!=NULL){
		pos=ms_list_position(sip_conf.proxies,ms_list_find(sip_conf.proxies,(void *)default_proxy));
	}
	return pos;
}

bool_t ServiceCore::serphone_proxy_config_check( SerphoneProxyConfig *obj)
{
	if (obj->reg_proxy==NULL){
		if (vtable.display_warning)
			vtable.display_warning(this,_("The sip proxy address you entered is invalid, it must start with \"sip:\""
						" followed by a hostname."));
		return FALSE;
	}
	if (obj->reg_identity==NULL){
		if (vtable.display_warning)
			vtable.display_warning(this,_("The sip identity you entered is invalid.\nIt should look like "
					"sip:username@proxydomain, such as sip:alice@example.net"));
		return FALSE;
	}
	return TRUE;
}

void ServiceCore::serphone_proxy_config_write_all_to_config_file()
{
	MSList *elem;
	int i;
	if (!serphone_core_ready()) return;

	for(elem=sip_conf.proxies,i=0;elem!=NULL;elem=ms_list_next(elem),i++){
		SerphoneProxyConfig *cfg=(SerphoneProxyConfig*)elem->data;
		serphone_proxy_config_write_to_config_file(config,cfg,i);
	}
	/*to ensure removed configs are erased:*/
	serphone_proxy_config_write_to_config_file(config,NULL,i);
	lp_config_set_int(config,"sip","default_proxy",serphone_core_get_default_proxy(NULL));
}

MSList *ServiceCore::serphone_core_get_proxy_config_list()
{
	return sip_conf.proxies;
}

MSList *ServiceCore::serphone_core_get_remove_config_list()
{
    return sip_conf.deleted_proxies;
}

const char * ServiceCore::serphone_core_get_route()
{
	SerphoneProxyConfig *proxy=NULL;
	const char *route=NULL;
	serphone_core_get_default_proxy(&proxy);
	if (proxy!=NULL) {
		route=proxy->serphone_proxy_config_get_route();
	}
	return route;
}


SerphoneProxyConfig * ServiceCore::serphone_core_lookup_known_proxy(const SerphoneAddress *uri)
{
	const MSList *elem;
	SerphoneProxyConfig *found_cfg=NULL;
	SerphoneProxyConfig *default_cfg=default_proxy;

	/*always prefer the default proxy if it is matching the destination uri*/
	if (default_cfg){
		const char *domain=default_cfg->serphone_proxy_config_get_domain();
		if (strcmp(domain,serphone_address_get_domain(uri))==0)
			return default_cfg;
	}

	/*otherwise iterate through the other proxy config and return the first matching*/
	for (elem=serphone_core_get_proxy_config_list();elem!=NULL;elem=elem->next){
		SerphoneProxyConfig *cfg=(SerphoneProxyConfig*)elem->data;
		const char *domain=cfg->serphone_proxy_config_get_domain();
		if (domain!=NULL && strcmp(domain,serphone_address_get_domain(uri))==0){
			found_cfg=cfg;
			break;
		}
	}
	return found_cfg;
}

const char *ServiceCore::serphone_core_find_best_identity(const SerphoneAddress *to, const char **route)
{
	SerphoneProxyConfig *cfg=serphone_core_lookup_known_proxy(to);
	if (cfg==NULL)
		serphone_core_get_default_proxy (&cfg);
	if (cfg!=NULL){
		if (route) *route=cfg->serphone_proxy_config_get_route();
		return cfg->serphone_proxy_config_get_identity ();
	}
	return serphone_core_get_primary_contact ();
}

void ServiceCore::serphone_set_reg_info(const char *proxy_addr, int proxy_port,
		const char *account, const char *password, const char *displayname, const char *capability_token, const char *gTokenp)
{
	cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);

	SerphoneProxyConfig *cfg;
	MSList *elem = NULL;

	if (proxy_addr == NULL || account ==NULL){
		return;
	}
	if( !proxy_port)
		proxy_port = 5060;

	char sipaccount[128], identify[128],proxy[128];
	sprintf(sipaccount,"<sip:%s@%s:%d>",account,proxy_addr,proxy_port);
	sprintf(identify,"%s%s",displayname,sipaccount);
	sprintf(proxy,"sip:%s:%d",proxy_addr,proxy_port);
//sean add begin 20140705 video conference
    internal_str_check_copy(&selfSipNo,account);
    internal_str_check_copy(&proxyAddr, proxy_addr);
    if (strlen(gTokenp)) {
        internal_str_check_copy(&registerUserdata, gTokenp);
    }
    proxyPort = proxy_port;
#ifdef HAIYUNTONG
    if (!reconnect) {
        int rethai = -1001;
        if (enableHaiyuntong) {
            if (deviceID && password && appID) {
                internal_str_check_copy(&pinCode, password);
                rethai = serphone_caller_init_haiyuntong(appID, strlen(appID),  selfSipNo, strlen(selfSipNo), deviceID);
            }
            else
                rethai = -1001;
        }
        if (0 != rethai) {

            if (traceFile) {
                time_t temp = time(NULL);
                std::string strTime = timetodate(temp);
                fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                char tempp[200] = {0};
                sprintf(tempp, " chuangyuan crypto lib init failed ret=%d\n",rethai);
                PrintConsole("[WARNNING HAIYUNTONG] lib crypto init failed %d\n",rethai);
                fwrite(tempp, strlen(tempp), 1, traceFile);
                fflush(traceFile);
            }
            if (vtable.init_haiyuntong_failed) {
                vtable.init_haiyuntong_failed();
            }
            return;
        }
    }
#endif
//sean add end 20140705 video conference

    //sean 20130509
    if (tls_enable) {
//        printf("Here we go into tls_enable\n");
        LCSipTransports tr;
        serphone_core_get_sip_transports(&tr);
        tr.udp_port = 0;
        tr.tcp_port =0;
        tr.tls_port = proxy_port;
        if (rootCaPath) {
            ca_certificate_path = rootCaPath;
        }
        else
        {
            PrintConsole("ERROR: TLS enabled without root ca!!!\n");
        }
        serphone_core_set_sip_transports(&tr);
    }

	char *contact=ser_strdup_printf("sip:%s@%s",account,proxy_addr);
	serphone_core_set_primary_contact(contact);
	ms_free((void **)&contact);

	if ( password != NULL || password[0]!='\0'){
		SerphoneAddress *from;
		SerphoneAuthInfo *info;
		if ((from = serphone_address_new(identify))!=NULL){
			char realm[128];
			snprintf(realm,sizeof(realm)-1,"\"%s\"",serphone_address_get_domain(from));
			info=(SerphoneAuthInfo*)serphone_core_find_auth_info(realm,account);
			if( !info) {
				info = serphone_auth_info_new(serphone_address_get_username(from),NULL,password,NULL,realm);
				serphone_core_add_auth_info(info);
				serphone_auth_info_destroy(&info);
			}
            else
            {
                if (strcasecmp(password, serphone_auth_info_get_passwd(info))) {
                    serphone_auth_info_set_passwd(info, password);
                }
            }
			serphone_address_destroy(from);
		}
	}
	elem = serphone_core_get_proxy_config_list();
	if (elem) {
		cfg=(SerphoneProxyConfig*)elem->data;
	}
	else
		cfg=serphone_proxy_config_new();

	if( cfg->reg_identity && cfg->reg_proxy && !strcmp(cfg->reg_identity,identify) &&
		!strcmp(cfg->reg_proxy,proxy) )
	{
		cfg->serphone_proxy_config_refresh_register();
	}
	else {

        if (traceFile) {
            time_t temp = time(NULL);
            std::string strTime = timetodate(temp);
            fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
            fwrite(" Something goes wrong Unregister called1\n", strlen(" Something goes wrong Unregister called1\n"), 1, traceFile);
            fflush(traceFile);
        }
		cfg->serphone_proxy_config_edit(); //unregister
        ms_list_for_each(this->chatrooms,(void (*)(void**))serphone_chat_room_destroy);
        this->chatrooms=ms_list_free(this->chatrooms);
		cfg->serphone_proxy_config_set_identity(identify);
		cfg->serphone_proxy_config_set_server_addr(proxy);
		cfg->serphone_proxy_config_enable_register(TRUE);
		cfg->serphone_proxy_config_done();
#ifdef WIN32
        Sleep(200);
#else
        usleep(200000);
#endif

        sal_reinit_network();
	}

	if (elem)
	{
		//cfg->serphone_proxy_config_done();
	}
	else
	{
        serphone_core_add_proxy_config(cfg);
        serphone_core_set_default_proxy(cfg);
	}

    memset(&capability_conf, 0, sizeof(capability_conf));
    serphone_core_parse_capability_token(capability_token);

//
	capability_conf.hdvideo = 1;
    capability_conf.localrec = 1;
    capability_conf.localrecvoip = 1;

	return ;
}


//haiyuntong
#ifdef HAIYUNTONG

int ServiceCore::serphone_certExisted(const char *sip, long sipLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    int ret = isExistOfCert((char *)sip, sipLen);
    if (1 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] isExistOfCert exist");
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] isExistOfCert not exist\n");
    }
    return ret;
}

int ServiceCore::serphone_set_deviceid_pincode(const char *devId, const char *appId, bool testFlag)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    internal_str_check_copy(&deviceID,devId);
    internal_str_check_copy(&appID, appId);
    haiyuntongTestFlag = testFlag;
    return 0;
}

int ServiceCore::serphone_enable_haiyuntong(bool flag)
{
    enableHaiyuntong = flag;
    return 0;
}

bool ServiceCore::serphone_haiyuntong_enabled()
{
    return enableHaiyuntong;
}

int ServiceCore::serphone_caller_init_haiyuntong(char *appid, long appidLen,  char *selfSipNo, long selfSipNoLen, char *devId)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    if (NULL == pinCode) {
        PrintConsole("[WARNNING HAIYUNTONG] set pin code first");
        return -8888;
    }
    PrintConsole("[DEBUG HAIYUNTONG] appid:%s, appidLen:%d, selfSipNo:%s, selfSipNo len:%d\n",appid, appidLen, selfSipNo, selfSipNoLen);

    int ret = setPIN(pinCode, strlen(pinCode), haiyuntongTestFlag);
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] setPin code succeed");
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] setPin code failed, ret:%d\n");
        return ret;
    }

    ret = init(appid, appidLen, selfSipNo, selfSipNoLen, devId);
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] init succeed\n");
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] init failed, ret:%d\n",ret);
    }

    return ret;
}
//zhu jiao
int ServiceCore::serphone_caller_180_183_transmitKeySet(SalOp *op,char *callee, long calleeLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    if (!op) {
        PrintConsole("[DEBUG HAIYUNTONG] error happens, op is NULL\n");
        return -3;
    }
    if (!op->bkey) {
        PrintConsole("[DEBUG HAIYUNTONG] error happens, bkey is NULL, probably bei jiao failed to create bkey, see bei jiao log.");
        return -4;
    }
    PrintConsole("[DEBUG HAIYUNTONG] inviteKey:%s, inviteLen:%d, callee:%s, callee len:%d\n",op->bkey,strlen(op->bkey), callee, calleeLen);
    int ret = -1;
    if (isLandingCall) {
        PrintConsole("[DEBUG HAIYUNTONG] landing call, id:%s",remoteSipNo);
        ret = VOIPtransmitKeyRequest(op->bkey, strlen(op->bkey), remoteSipNo, strlen(remoteSipNo));
    }
    else
    {
        ret = transmitKeyRequest(op->bkey, strlen(op->bkey), callee, calleeLen);
    }
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] %s succeed\n",isLandingCall?"VOIPtransmitKeyRequest":"transmitKeyRequest");
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] %s failed, ret:%d\n",isLandingCall?"VOIPtransmitKeyRequest":"transmitKeyRequest",ret);
    }
    return ret;
}
//bei jiao
int ServiceCore::serphone_callee_create_bKey(SalOp *op,char *caller, long callerLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    if (!op || !op->akey) {
        PrintConsole("[DEBUG HAIYUNTONG] akey is null, check it");
        return -5;
    }
    PrintConsole("[DEBUG HAIYUNTONG] inviteKey:%s, inviteLen:%d, callee:%s, callee len:%d\n",op->akey,strlen(op->akey), caller, callerLen);
    int ret = -1;
    char bkey[1024] = {0};
    long bkeyLen = 0;
    ret = transmitKeyRequest(op->akey, strlen(op->akey), caller, callerLen, bkey, &bkeyLen);
    if (0 == ret) {
        if (op->bkey) {
            ms_free((void **)op->bkey);
        }
        op->bkey = (char *)malloc(bkeyLen + 1);
        memcpy(op->bkey, bkey, bkeyLen);
        op->bkey[bkeyLen] = '\0';
        PrintConsole("[DEBUG HAIYUNTONG] serphone_callee_create_bKey succeed\n");
        PrintConsole("[DEBUG HAIYUNTONG] dump bkey:%s\n",op->bkey);
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] serphone_callee_create_bKey failed, ret:%d\n",ret);
    }
    return ret;
}

int ServiceCore::serphone_encrypt(char *mediaStream, long mediaStreamLen, char *selfSipNo, long selfSipNoLen, char *mediaStreamCrpp , long* mediaStreamCrpLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    PrintConsole("[DEBUG HAIYUNTONG] serphone_encrypt: original stream len:%lu, selfSipNo: %s, selfSipNo length :%d\n",mediaStreamLen,selfSipNo,strlen(selfSipNo));
    int ret = -1;
    ret = mediaStreamCrp(mediaStream, mediaStreamLen, selfSipNo, selfSipNoLen, mediaStreamCrpp , mediaStreamCrpLen );
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] serphone_encrypt succeed\n");
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] serphone_encrypt failed, ret:%d\n",ret);
    }
    return  ret;
}

int ServiceCore::serphone_decrypt(char *mediaStream, long mediaStreamLen, char *remoteSipNo, long remoteSipNoLen, char *mediaStreamDecrpp , long* mediaStreamDecrpLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    PrintConsole("[DEBUG HAIYUNTONG] serphone_decrypt: original stream len:%lu, selfSipNo: %s, selfSipNo length :%d\n",mediaStreamLen,remoteSipNo,strlen(remoteSipNo));
    int ret = -1;
    ret = mediaStreamDecrp(mediaStream, mediaStreamLen, remoteSipNo, remoteSipNoLen, mediaStreamDecrpp, mediaStreamDecrpLen);
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] serphone_decrypt succeed\n");
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] serphone_decrypt failed, ret:%d\n",ret);
    }
    return  ret;
}

int ServiceCore::serphone_delete_transmit_key()
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    PrintConsole("[DEBUG HAIYUNTONG] serphone_delete_transmit_key: sip no:%s\n",selfSipNo);
    int ret = -1;
    if (isLandingCall) {
//        ret = deleteVOIPTransmitKey(proxyAddr, strlen(proxyAddr));
        ret = deleteVOIPTransmitKey("119119", strlen("119119"));
    }
    {
        ret = deleteTransmitKey(selfSipNo, strlen(selfSipNo));
    }
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] serphone_delete_transmit_key succeed\n");
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] serphone_delete_transmit_key failed, ret:%d\n",ret);
    }
    return  ret;
}

int ServiceCore::serphone_audio_conf_key_set(SalOp *op)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    //check audio conf?
    if (!isAudioConf) {
        PrintConsole("[WARNNING HAIYUNTONG] not in conf mode, you are not supposed to go into func %s\n",__FUNCTION__);
        return 0;
    }
    int ret = -1;
    ret = groupTransmitKeyDecrp(op->confKey, (long)strlen(op->confKey)/*, confID, (long)strlen(confID)*/);
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] serphone_audio_conf_key_set succeed\n");
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] serphone_audio_conf_key_set failed, ret:%d\n",ret);
    }
    return ret;
}


//sms
int ServiceCore::serphone_sms_encrypt(char *sms, long smsLen, char *remoteSip, long remoteSipLen, char *smsCrpt, long* smsCrptLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    PrintConsole("[DEBUG HAIYUNTONG] %s: sip remote sip:%s\n",__FUNCTION__,remoteSip);
    int ret = -1;
    ret = smsCrpRequest(sms, smsLen, remoteSip, remoteSipLen, smsCrpt, smsCrptLen );
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] %s succeed, inLen:%d, outLen:%d\n",__FUNCTION__,smsLen,*smsCrptLen);

//        unsigned char *dumpData = (unsigned char *)sms;
//        char dumpbufin[1024] = {0};
//        for (int i = 0; i < smsLen; i++) {
//            sprintf(dumpbufin+3*i, "%02X ",*(dumpData+i));//("%02X ",*(dumpData+i));
//        }
//
//        dumpData = (unsigned char *)smsCrpt;
//        char dumpbufout[1024] = {0};
//        for (int i = 0; i < *smsCrptLen; i++) {
//            sprintf(dumpbufout+3*i, "%02X ",*(dumpData+i));//("%02X ",*(dumpData+i));
//        }
//        PrintConsole("[DEBUG HAIYUNTONG] serphone_sms_encrypt called, data before processed:%s",dumpbufin);
//        PrintConsole("[DEBUG HAIYUNTONG] serphone_sms_encrypt called, data after processed:%s",dumpbufout);
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] %s failed, ret:%d\n",__FUNCTION__,ret);
    }
    return  ret;
}

int ServiceCore::serphone_sms_decrypt(char *sms, long smsLen, char *remoteSip, long remoteSipLen, char *smsDeCrpt, long* smsDeCrptLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    PrintConsole("[DEBUG HAIYUNTONG] %s: remote sip:%s, input message:%s\n",__FUNCTION__,remoteSip, sms);

    int ret = -1;
    ret = smsDecrpRequest(sms, smsLen, remoteSip, remoteSipLen, smsDeCrpt, smsDeCrptLen );
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] %s succeed, inLen:%d, outLen:%ld\n",__FUNCTION__,smsLen, *smsDeCrptLen);
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] %s failed, ret:%d\n",__FUNCTION__,ret);
    }
    PrintConsole("[DEBUG HAIYUNTONG] original message:%s",smsDeCrpt);
    return  ret;
}


//group sms
int ServiceCore::serphone_group_sms_encrypt(char *sms, long smsLen, char *proxyAddr, long proxyAddrLen, char *smsCrpt, long* smsCrptLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    PrintConsole("[DEBUG HAIYUNTONG] %s: sip server addr:%s\n",__FUNCTION__,proxyAddr);

    int ret = -1;
    ret = groupSmsCrpEnvelop(sms, smsLen, proxyAddr, proxyAddrLen, smsCrpt, smsCrptLen );
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] %s succeed\n",__FUNCTION__);
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] %s failed, ret:%d\n",__FUNCTION__,ret);
    }
    return  ret;
}


int ServiceCore::serphone_group_sms_decrypt(char *sms, long smsLen, char *proxyAddr, long proxyAddrLen, char *smsDeCrpt, long* smsDeCrptLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    PrintConsole("[DEBUG HAIYUNTONG] %s: sip server addr:%s\n",__FUNCTION__,proxyAddr);

    int ret = -1;
    ret = groupSmsDecrpEnvelop(sms, smsLen, proxyAddr, proxyAddrLen, smsDeCrpt, smsDeCrptLen );
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] %s succeed\n",__FUNCTION__);
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] %s failed, ret:%d\n",__FUNCTION__,ret);
    }
    return  ret;
}


//file
int ServiceCore::serphone_file_encrypt(const unsigned char *file, long fileLen, char *remoteSip, long remoteSipLen, unsigned char *fileCrpEnvelopp, long* fileCrpEnvelopLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    PrintConsole("[DEBUG HAIYUNTONG] %s: sip remote sip:%s\n",__FUNCTION__,remoteSip);

    int ret = -1;
    ret = fileCrpEnvelop((unsigned char *)file, fileLen, remoteSip, remoteSipLen, fileCrpEnvelopp, fileCrpEnvelopLen);
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] %s succeed\n",__FUNCTION__);
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] %s failed, ret:%d\n",__FUNCTION__,ret);
    }
    return  ret;
}


int ServiceCore::serphone_file_decrypt(const unsigned char *file, long fileLen, char *remoteSip, long remoteSipLen, unsigned char *fileDeCrpt, long* fileDeCrptLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    PrintConsole("[DEBUG HAIYUNTONG] %s: sip remote sip:%s\n",__FUNCTION__,remoteSip);

    int ret = -1;
    ret = fileDecrpEnvelop((unsigned char *)file, fileLen, remoteSip, remoteSipLen, fileDeCrpt, fileDeCrptLen );
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] %s succeed\n",__FUNCTION__);
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] %s failed, ret:%d\n",__FUNCTION__,ret);
    }
    return  ret;
}


//group file
int ServiceCore::serphone_group_file_encrypt(const unsigned char *file, long fileLen, char **userList, long *eachLen, int numOfUsers, unsigned char *groupFileCrpEnvelopp, long* groupFileCrpEnvelopLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    PrintConsole("[DEBUG HAIYUNTONG] %s called\n",__FUNCTION__,proxyAddr);

    int ret = -1;
    ret = groupFileCrpEnvelop((unsigned char *)file, fileLen, userList, eachLen, numOfUsers, groupFileCrpEnvelopp, groupFileCrpEnvelopLen);
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] %s succeed\n",__FUNCTION__);
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] %s failed, ret:%d\n",__FUNCTION__,ret);
    }
    return  ret;
}


int ServiceCore::serphone_group_file_decrypt(const unsigned char *file, long fileLen, char *proxyAddr, long proxyAddrLen, unsigned char *groupFileDecrpt, long*groupFileDecrptLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    PrintConsole("[DEBUG HAIYUNTONG] %s: sip server addr:%s\n",__FUNCTION__,proxyAddr);

    int ret = -1;
    ret = groupFileDecrpEnvelop((unsigned char *)file, fileLen, proxyAddr, proxyAddrLen, groupFileDecrpt, groupFileDecrptLen );
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] %s succeed\n",__FUNCTION__);
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] %s failed, ret:%d\n",__FUNCTION__,ret);
    }
    return  ret;
}


//manange contact
int ServiceCore::serphone_add_contact(char **desid, long *desidLen, int num)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    PrintConsole("[DEBUG HAIYUNTONG] %s: sip remote sip:%s\n",__FUNCTION__);

    int ret = -1;
    ret = contactListAdd (desid, desidLen, num);
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] %s succeed\n",__FUNCTION__);
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] %s failed, ret:%d\n",__FUNCTION__,ret);
    }
    return  ret;
}


int ServiceCore::serphone_del_contact(char *remoteSip, long remoteSipLen)
{
    if (!enableHaiyuntong) {
        PrintConsole("[WARNNING HAIYUNTONG] haiyuntong is not enalbed currently! To use this feature, call serphone_enable_haiyuntong first. Func:%s",__FUNCTION__);
        return -9999;
    }
    PrintConsole("[DEBUG HAIYUNTONG] %s: sip remote sip:%s\n",__FUNCTION__,remoteSip);

    int ret = -1;
    ret = contactListDel(remoteSip, remoteSipLen);
    if (0 == ret) {
        PrintConsole("[DEBUG HAIYUNTONG] %s succeed\n",__FUNCTION__);
    }
    else
    {
        PrintConsole("[DEBUG HAIYUNTONG] %s failed, ret:%d\n",__FUNCTION__,ret);
    }
    return  ret;
}


bool ServiceCore::serphone_get_isAudioConf()
{
    return isAudioConf;
}

#endif


void ServiceCore::serphone_proxy_remove(const char *proxyAddr)
{
    SerphoneProxyConfig *rmCfg=NULL;
    if( proxyAddr && strlen(proxyAddr)>0 ) {
        const MSList *elem;
        SerphoneProxyConfig *default_cfg=default_proxy;

        /*always prefer the default proxy if it is matching the destination uri*/
        if (default_cfg){
            const char *domain=default_cfg->serphone_proxy_config_get_domain();
            if (strcmp(domain,proxyAddr)==0)
                rmCfg = default_cfg;
        }
        else {
            /*otherwise iterate through the other proxy config and return the first matching*/
            for (elem=serphone_core_get_proxy_config_list();elem!=NULL;elem=elem->next){
                SerphoneProxyConfig *cfg=(SerphoneProxyConfig*)elem->data;
                const char *domain=cfg->serphone_proxy_config_get_domain();
                if (domain!=NULL && strcmp(domain,proxyAddr)==0){
                    rmCfg=cfg;
                }
            }
        }
    }
    else {
        //remove the first
        MSList *elem = NULL;
        elem = serphone_core_get_proxy_config_list();
        if(elem)
            rmCfg=(SerphoneProxyConfig*)elem->data;
    }

    if(rmCfg)
    {
        sip_conf.proxies = ms_list_remove(sip_conf.proxies, rmCfg);
        sip_conf.deleted_proxies = ms_list_append(sip_conf.deleted_proxies, rmCfg);
        rmCfg->deletion_date=ms_time(NULL);
        if (rmCfg->state==LinphoneRegistrationOk){
            /* this will unREGISTER */
            if (traceFile) {
                time_t temp = time(NULL);
                std::string strTime = timetodate(temp);
                fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                fwrite(" Something goes wrong Unregister called5\n", strlen(" Something goes wrong Unregister called5\n"), 1, traceFile);
                fflush(traceFile);
            }
            rmCfg->serphone_proxy_config_edit();
        }
        serphone_core_set_default_proxy(NULL);
    }
}

void ServiceCore::serphone_set_reg_displayname(const char *displayName)
{
	cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	SerphoneProxyConfig *cfg = NULL;
	serphone_core_get_default_proxy( &cfg);

	if( !cfg)
		return;
	/*char identity[128];
	sprintf(identity,"%s%s",displayName,gSIPAccount);
	//cfg->serphone_proxy_config_edit();
	cfg->serphone_proxy_config_set_identity(gIdentity);
	cfg->serphone_proxy_config_refresh_register();

	//cfg->serphone_proxy_config_done();*/
}

bool_t ServiceCore::serphone_core_is_media_encryption_mandatory() {
	return (bool_t)lp_config_get_int(this->config, "sip", "media_encryption_mandatory", 0);
}

SerphoneMediaEncryption ServiceCore::serphone_core_get_media_encryption()
{
	const char* menc = lp_config_get_string(config, "sip", "media_encryption", NULL);

	if (menc == NULL)
		return LinphoneMediaEncryptionNone;
	else if (strcmp(menc, "srtp")==0)
		return LinphoneMediaEncryptionSRTP;
	else if (strcmp(menc, "zrtp")==0)
		return LinphoneMediaEncryptionZRTP;
	else
		return LinphoneMediaEncryptionNone;
}

/**
 * Init call params using ServiceCore's current configuration
 */
void ServiceCore::serphone_core_init_default_params(SerphoneCallParams *params)
{
	params->has_video=video_policy.automatically_initiate;  //linphone_core_video_enabled(lc) && video_policy.automatically_initiate;
	params->media_encryption=serphone_core_get_media_encryption();
	params->in_conference=FALSE;
	params->invite_userdata = NULL;
    params->group_id = NULL;
//haiyuntong
#ifdef HAIYUNTONG
    params->akey = NULL;
    params->bkey = NULL;
    params->confKey = NULL;
#endif
}

SerphoneCallParams *ServiceCore::serphone_core_create_default_call_parameters()
{
//	SerphoneCallParams *p=ms_new0(SerphoneCallParams,1);  //ms_new0
	SerphoneCallParams *p=(SerphoneCallParams *)malloc(sizeof(SerphoneCallParams)*1);  //ms_new0
	memset((void *)p,0,sizeof(SerphoneCallParams)*1);


	serphone_core_init_default_params(p);
	return p;
}

void serphone_call_params_destroy(SerphoneCallParams *p)
{
	if(p->invite_userdata)
    {
        ms_free((void **)&p->invite_userdata);
    }
    if(p->group_id)
    {
        ms_free((void **)&p->group_id);
    }
//haiyuntong
#ifdef HAIYUNTONG
    if (p->akey) {
        ms_free((void **)&p->akey);
    }
    if (p->bkey) {
        ms_free((void **)&p->bkey);
    }
    if (p->confKey) {
        ms_free((void **)&p->confKey);
    }
#endif

	ms_free((void **)&p);
}

SerphoneCallParams * serphone_call_params_copy(const SerphoneCallParams *cp)
{
//	SerphoneCallParams *ncp=ms_new0(SerphoneCallParams,1);  //ms_new0
	SerphoneCallParams *ncp=(SerphoneCallParams *)malloc(sizeof(SerphoneCallParams)*1);  //ms_new0
	memcpy(ncp,cp,sizeof(SerphoneCallParams));
	if(cp->invite_userdata)
		ncp->invite_userdata = ms_strdup(cp->invite_userdata);
    if(cp->group_id)
		ncp->group_id = ms_strdup(cp->group_id);
//haiyuntong
#ifdef HAIYUNTONG
    if(cp->akey)
        ncp->akey = ms_strdup(cp->akey);
    if(cp->bkey)
        ncp->bkey = ms_strdup(cp->bkey);
    if(cp->confKey)
        ncp->confKey = ms_strdup(cp->confKey);
#endif
	return ncp;
}

void ServiceCore::update_local_media_description(SerPhoneCall *call)
{
	SalMediaDescription *md=call->localdesc;
	if (md== NULL) {
//		call->localdesc = create_local_media_description(call);
        create_local_media_description(call);
	} else {
//		call->localdesc = _create_local_media_description(call,md->session_id,md->session_ver+1);
//		sal_media_description_unref(md);
        _create_local_media_description(call,md->session_id,md->session_ver+1);
	}
}

SalMediaDescription *ServiceCore::create_local_media_description( SerPhoneCall *call)
{
	unsigned int id=rand() & 0xfff;
	return _create_local_media_description(call,id,id);
}

SalMediaDescription *ServiceCore::_create_local_media_description( SerPhoneCall *call, unsigned int session_id, unsigned int session_ver)
{
	MSList *l;
	PayloadType *pt;

    SalMediaDescription *old_md=call->localdesc;
	int i;


	const char *me=serphone_core_get_identity();
	SerphoneAddress *addr=serphone_address_new(me);
	const char *username=serphone_address_get_username (addr);
	SalMediaDescription *md=sal_media_description_new();


//	md->session_id=session_id;
//	md->session_ver=session_ver;
    md->session_id=(old_md ? old_md->session_id : (rand() & 0xfff));
	md->session_ver=(old_md ? (old_md->session_ver+1) : (rand() & 0xfff));
	md->n_total_streams=(old_md ? old_md->n_total_streams : 1);

	md->nstreams=md->n_total_streams;
    md->n_active_streams = 1;
	strncpy(md->addr,call->localip,sizeof(md->addr));
	strncpy(md->username,username,sizeof(md->username));
	md->bandwidth=serphone_core_get_download_bandwidth();

	//set audio capabilities
	strncpy(md->streams[0].addr,call->localip,sizeof(md->streams[0].addr));
    md->streams[0].port=call->audio_port;


    strncpy(md->streams[0].rtp_addr, call->localip, sizeof(md->streams[0].rtp_addr));
    strncpy(md->streams[0].rtcp_addr,call->localip,sizeof(md->streams[0].rtcp_addr));
    md->streams[0].rtp_port = call->audio_port;
    md->streams[0].rtcp_port=call->audio_port+1;
	md->streams[0].proto=(call->params.media_encryption == LinphoneMediaEncryptionSRTP) ?
		SalProtoRtpSavp : SalProtoRtpAvp;
	md->streams[0].type=SalAudio;
	md->streams[0].ptime=net_conf.down_ptime;
	md->streams[0].nack_support = audioNackEnabled;
	l=make_codec_list(this->codecs_conf.audio_codecs,call->params.audio_bw,&md->streams[0].max_rate);
	pt=payload_type_clone(rtp_profile_get_payload_from_mime(&av_profile,"telephone-event"));
	l=ms_list_append(l,pt);
//    pt=payload_type_clone(rtp_profile_get_payload_from_mime(&av_profile,"CN"));
//	l=ms_list_append(l,pt);
	md->streams[0].payloads=l;

	if (call->params.has_video){
        md->n_active_streams++;
//		md->nstreams++;
		strncpy(md->streams[1].addr,call->localip,sizeof(md->streams[1].addr));
		md->streams[1].port=call->video_port;
        md->streams[1].rtp_port=call->video_port;
		md->streams[1].rtcp_port=call->video_port+1;
		md->streams[1].proto=md->streams[0].proto;
		md->streams[1].type=SalVideo;
		md->streams[1].nack_support = videoNackEnabled;
		l=make_codec_list(this->codecs_conf.video_codecs,0,NULL);
		md->streams[1].payloads=l;
	}

    if (md->n_total_streams < md->n_active_streams)
    {
        md->nstreams = md->n_active_streams;
        md->n_total_streams = md->n_active_streams;
    }


    /* Deactivate inactive streams. */
	for (i = md->n_active_streams; i < md->n_total_streams; i++) {
		md->streams[i].rtp_port = 0;
		md->streams[i].rtcp_port = 0;
		md->streams[i].proto = SalProtoRtpAvp;
		md->streams[i].type = old_md->streams[i].type;
		md->streams[i].dir = SalStreamInactive;
		l = make_codec_list(this->codecs_conf.video_codecs, 0, NULL);
		md->streams[i].payloads = l;
	}

	for(int i=0; i<md->nstreams; i++) {
		if (md->streams[i].proto == SalProtoRtpSavp) {
			md->streams[i].crypto[0].tag = 1;
			md->streams[i].crypto[0].algo = cloopenwebrtc::CCPAES_256_SHA1_80;
			if (!generate_b64_crypto_key(46, md->streams[i].crypto[0].master_key, (const char *)m_SrtpKey)) {
				md->streams[i].crypto[0].algo = (cloopenwebrtc::ccp_srtp_crypto_suite_t)0;
			}

			md->streams[i].crypto[1].tag = 2;
			md->streams[i].crypto[1].algo = cloopenwebrtc::CCPAES_256_SHA1_32;
			if (!generate_b64_crypto_key(46, md->streams[i].crypto[1].master_key, (const char *)m_SrtpKey)) {
				md->streams[i].crypto[1].algo = (cloopenwebrtc::ccp_srtp_crypto_suite_t)0;
			}

			md->streams[i].crypto[2].algo = (cloopenwebrtc::ccp_srtp_crypto_suite_t)0;
		}
	}


    update_media_description_from_stun(md,&call->ac,&call->vc);
	if (call->ice_session != NULL) {
		serphone_core_update_local_media_description_from_ice(md, call->ice_session);
		serphone_core_update_ice_state_in_call_stats(call);
	}
//*/
	serphone_address_destroy(addr);
    call->localdesc=md;
	if (old_md) sal_media_description_unref(&old_md);
	return md;
}

/**
 * Sets maximum available download bandwidth
 *
 * @ingroup media_parameters
 *
 * This is IP bandwidth, in kbit/s.
 * This information is used signaled to other parties during
 * calls (within SDP messages) so that the remote end can have
 * sufficient knowledge to properly configure its audio & video
 * codec output bitrate to not overflow available bandwidth.
 *
 * @param bw the bandwidth in kbits/s, 0 for infinite
 */
void ServiceCore::serphone_core_set_download_bandwidth(int bw)
{
	net_conf.download_bw=bw;
	if (serphone_core_ready()) lp_config_set_int(config,"net","download_bw",bw);
}

/**
 * Sets maximum available upload bandwidth
 *
 * @ingroup media_parameters
 *
 * This is IP bandwidth, in kbit/s.
 * This information is used by liblinphone together with remote
 * side available bandwidth signaled in SDP messages to properly
 * configure audio & video codec's output bitrate.
 *
 * @param bw the bandwidth in kbits/s, 0 for infinite
 */
void ServiceCore::serphone_core_set_upload_bandwidth(int bw)
{
	net_conf.upload_bw=bw;
	if (serphone_core_ready()) lp_config_set_int(config,"net","upload_bw",bw);
}

/**
 * Retrieve the maximum available download bandwidth.
 *
 * @ingroup media_parameters
 *
 * This value was set by serphone_core_set_download_bandwidth().
 *
**/
int ServiceCore::serphone_core_get_download_bandwidth()
{
	return net_conf.download_bw;
}

/**
 * Retrieve the maximum available upload bandwidth.
 *
 * @ingroup media_parameters
 *
 * This value was set by serphone_core_set_upload_bandwidth().
 *
**/
int ServiceCore::serphone_core_get_upload_bandwidth()
{
	return net_conf.upload_bw;
}

SerPhoneCall * ServiceCore::serphone_call_new_incoming(SerphoneAddress *from, SerphoneAddress *to, SalOp *op)
{
    /*add begin------------------Sean20130812----------for video ice------------*/
    cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
    /*add end--------------------Sean20130812----------for video ice------------*/
//	SerPhoneCall *call=ms_new0(SerPhoneCall,1);   //ms_new0
	SerPhoneCall *call=(SerPhoneCall *)malloc(sizeof(SerPhoneCall)*1);   //ms_new0
	memset((void *)call,0,sizeof(SerPhoneCall)*1);

    const SalMediaDescription *md;

	char *from_str;
	call->dir=LinphoneCallIncoming;
	sal_op_set_user_pointer(op,call);
	call->op=op;
	call->core=this;
	call->m_AudioChannelID = -1;
	call->m_VideoChannelID = -1;
	call->m_CaptureDeviceId = -1;
	call->m_desktopShareDeviceId = -1;
	call->callConnected = false;

    struct tm *pt = NULL;
	time_t curr_time;
    curr_time = time(NULL);

#ifdef WIN32
    pt = localtime(&curr_time);
#else
    struct tm t1;
    pt = localtime_r(&curr_time,&t1);
#endif
    char userCallID[16] = {'\0'};
    sprintf(userCallID, "%02d%02d%02d%02d",pt->tm_mday,pt->tm_hour,pt->tm_min,pt->tm_sec);
    memcpy(call->_user_call_id, userCallID, 8);
    call->_user_call_id[8] = '\0';


	if (sip_conf.ping_with_options){        //这段代码可以取消   zdm
		//the following sends an option request back to the caller so that
		// we get a chance to discover our nat'd address before answering.
		call->ping_op=sal_op_new(sal);
		from_str=serphone_address_as_string_uri_only(from);
		sal_op_set_route(call->ping_op,sal_op_get_network_origin(op));
		sal_op_set_user_pointer(call->ping_op,call);
		sal_ping(call->ping_op,serphone_core_find_best_identity(from,NULL),from_str);
		ms_free((void **)&from_str);
	}
	serphone_address_clean(from);
	serphone_core_get_local_ip( serphone_address_get_domain(from),call->localip);
	serphone_call_init_common(call,from, to);
	serphone_core_init_default_params( &call->params);
	//call->params.has_video &= !!this->video_policy.automatically_accept;
//	call->params.has_video = false;

    md=sal_call_get_remote_media_description(op);
	call->params.has_video &= !!this->video_policy.automatically_accept;

	if (md) {
		// It is licit to receive an INVITE without SDP
		// In this case WE chose the media parameters according to policy.
		call->params.has_video &= serphone_core_media_description_contains_video_stream(md);
	}

    call->params.media_encryption = LinphoneMediaEncryptionNone;
	if( op->base.remote_media ) {
		for ( int i = 0 ; i < op->base.remote_media->nstreams ; i++ )
        {
			if( op->base.remote_media->streams[i].type == SalVideo) {
				call->params.has_video = true;
			}
            /*sean 20130425*/
            if (op->base.remote_media->streams[i].proto == SalProtoRtpSavp) {
                call->params.media_encryption = LinphoneMediaEncryptionSRTP;
                //sean todo to get remote key
            }
        }
	}

//	call->localdesc=create_local_media_description (call);

    create_local_media_description (call);

    //sean test 20130510
//    srtp_enable = true;
    if (srtp_enable) {
        if (op->base.remote_media) {
            for (int i = 0; i < op->base.remote_media->nstreams; i++) {

                if (op->base.remote_media->streams[i].proto == SalProtoRtpSavp) {
                    call->localdesc->streams[i].crypto[0].tag = op->base.remote_media->streams[i].crypto[0].tag;
                    call->localdesc->streams[i].crypto[0].algo = op->base.remote_media->streams[i].crypto[0].algo;
                    memcpy(call->localdesc->streams[i].crypto[0].master_key, op->base.remote_media->streams[i].crypto[0].master_key, 65);//sean todo

                    call->localdesc->streams[i].crypto[1].tag = op->base.remote_media->streams[i].crypto[1].tag;
                    call->localdesc->streams[i].crypto[1].algo = op->base.remote_media->streams[i].crypto[1].algo;
                    memcpy(call->localdesc->streams[i].crypto[1].master_key, op->base.remote_media->streams[i].crypto[1].master_key, 65);
                }
            }
        }
    }

	call->camera_active=call->params.has_video;

    switch (serphone_core_get_firewall_policy()) {
		case LinphonePolicyUseIce:
            PrintConsole("Incoming call using LinphonePolicyUseIce, stunserver = %s\n",serphone_core_get_stun_server());
			call->ice_session = ice_session_new();
			ice_session_set_role(call->ice_session, IR_Controlled);
			serphone_core_update_ice_from_remote_media_description(call, sal_call_get_remote_media_description(op));
			if(!call->ice_session) {
				PrintConsole("LinphonePolicyUseIce call->ice_sessoin == NULL\n");
			}
			break;
		case LinphonePolicyUseStun:
            PrintConsole("Using LinphonePoicyUseStun\n");
			call->ping_time=serphone_core_run_stun_tests(call);
			/* No break to also destroy ice session in this case. */
			break;
		default:
            PrintConsole("Incoming call not using LinphonePolicyUseIce");
			break;
	}

	serphone_call_init_media_streams(call);

	if (serphone_core_get_firewall_policy() == LinphonePolicyUseIce && call->ice_session != NULL) {

		serphone_call_start_media_streams_for_ice_gathering(call); //Useless
		if (serphone_core_gather_ice_candidates(/*call->core,*/call)<0) {
			/* Ice candidates gathering failed, proceed with the call anyway. */
			serphone_call_delete_ice_session(call);
			serphone_call_stop_media_streams_for_ice_gathering(call);
		}
	}

	discover_mtu(serphone_address_get_domain(from));
	return call;
}

/**
 * Returns the UDP port used for audio streaming.
 *
 * @ingroup network_parameters
**/
int ServiceCore::serphone_core_get_audio_port()
{
	return rtp_conf.audio_rtp_port;
}

/**
 * Returns the UDP port used for video streaming.
 *
 * @ingroup network_parameters
**/
int ServiceCore::serphone_core_get_video_port()
{
	return rtp_conf.video_rtp_port;
}

/**
 * Sets the UDP port used for audio streaming.
 *
 * @ingroup network_parameters
**/
void ServiceCore::serphone_core_set_audio_port(int port)
{
	rtp_conf.audio_rtp_port=port;
}


/**
 * Sets the UDP port used for video streaming.
 *
 * @ingroup network_parameters
**/
void ServiceCore::serphone_core_set_video_port(int port)
{
	rtp_conf.video_rtp_port=port;
}

int ServiceCore::find_port_offset()
{
	int offset;
	MSList *elem;
	int audio_port;
	bool_t already_used=FALSE;
	for(offset=0;offset<100;offset+=2){
		audio_port=serphone_core_get_audio_port ()+offset;
		already_used=FALSE;
		for(elem=this->calls;elem!=NULL;elem=elem->next){
			SerPhoneCall *call=(SerPhoneCall*)elem->data;
			if (call->audio_port==audio_port) {
				already_used=TRUE;
				break;
			}
		}
		if (!already_used) break;
	}
	if (offset==100){
		PrintConsole("Could not find any free port !\n");
		return -1;
	}
	return offset;
}



SerphoneFriend *ServiceCore::serphone_core_get_friend_by_address(const char *uri)
{
	SerphoneAddress *puri=serphone_address_new(uri);
	const MSList *elem;
	const char *username;
	const char *domain;
	SerphoneFriend *lf=NULL;

	if (puri==NULL){
		return NULL;
	}
	username=serphone_address_get_username(puri);
	domain=serphone_address_get_domain(puri);
	if (domain==NULL) {
		serphone_address_destroy(puri);
		return NULL;
	}
	for(elem=friends;elem!=NULL;elem=ms_list_next(elem)){
		lf=(SerphoneFriend*)elem->data;
		const char *it_username=serphone_address_get_username(lf->uri);
		const char *it_host=serphone_address_get_domain(lf->uri);;
		if (strcasecmp(domain,it_host)==0 && username_match(username,it_username)){
			break;
		}
		lf=NULL;
	}
	serphone_address_destroy(puri);
	return lf;
}

SerphoneFriend *ServiceCore::serphone_core_get_friend_by_ref_key(const char *key)
{
	const MSList *elem;
	if (key==NULL) return NULL;
	for(elem=serphone_core_get_friend_list();elem!=NULL;elem=elem->next){
		SerphoneFriend *lf=(SerphoneFriend*)elem->data;
		if (lf->refkey!=NULL && strcmp(lf->refkey,key)==0){
			return lf;
		}
	}
	return NULL;
}

const MSList * ServiceCore::serphone_core_get_friend_list()
{
	return friends;
}

void ServiceCore::discover_mtu(const char *remote)
{
	int mtu;
	if (net_conf.mtu==0	){
		/*attempt to discover mtu*/
		mtu=ms_discover_mtu(remote);
		if (mtu>0){
			ms_set_mtu(mtu);
			PrintConsole("Discovered mtu is %i, RTP payload max size is %i\n",
				mtu, ms_get_payload_max_size());
		}
	}
}

/**
 * Accept an incoming call.
 *
 * @ingroup call_control
 * Basically the application is notified of incoming calls within the
 * call_state_changed callback of the #SerphoneCoreVTable structure, where it will receive
 * a LinphoneCallIncoming event with the associated LinphoneCall object.
 * The application can later accept the call using this method.
 * @param lc the LinphoneCore object
 * @param call the LinphoneCall object representing the call to be answered.
 *
**/
int ServiceCore::serphone_core_accept_call(SerPhoneCall *call)
{
    cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	return serphone_core_accept_call_with_params(call,NULL);
}

/**
 * Accept an incoming call, with parameters.
 *
 * @ingroup call_control
 * Basically the application is notified of incoming calls within the
 * call_state_changed callback of the #SerphoneCoreVTable structure, where it will receive
 * a LinphoneCallIncoming event with the associated LinphoneCall object.
 * The application can later accept the call using
 * this method.
 * @param call the SerphoneCall object representing the call to be answered.
 * @param params the specific parameters for this call, for example whether video is accepted or not. Use NULL to use default parameters.
 *
**/
int ServiceCore::serphone_core_accept_call_with_params(SerPhoneCall *call, const SerphoneCallParams *params)
{
    cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	SerphoneProxyConfig *cfg=NULL,*dest_proxy=NULL;
	const char *contact=NULL;
	SalOp *replaced;
	SalMediaDescription *new_md;
	bool_t was_ringing=FALSE;

	if (call==NULL){
		//if just one call is present answer the only one ...
		if(serphone_core_get_calls_nb () != 1)
			return -1;
		else
			call = (SerPhoneCall*)serphone_core_get_calls()->data;
	}

	if (call->state != LinphoneCallIncomingReceived){
		/*call already accepted*/
		return -1;
	}
	if(call->dir != LinphoneCallIncoming) {
		/*can't answer self*/
		return -1;
	}

//    int pre_used_camera_index = m_usedCameraIndex;
//#ifdef MAC_IPHONE
//    selectCamera(0, m_usedCapabilityIndex, m_maxFPS, m_camerRotate,false);
//#endif
	/* check if this call is supposed to replace an already running one*/
	replaced=sal_call_get_replaces(call->op);
	if (replaced){
		SerPhoneCall *rc=(SerPhoneCall*)sal_op_get_user_pointer (replaced);
		if (rc){
			PrintConsole("Call %p replaces call %p. This last one is going to be terminated automatically.\n",
			           call,rc);
			serphone_core_terminate_call(rc);
		}
	}

	if (this->current_call!=call){
		serphone_core_preempt_sound_resources();
	}

	/*stop ringing */
	if ( m_ringplay_flag ){
		ring_stop();
		m_ringplay_flag = FALSE;
		dmfs_playing_start_time = 0;
		was_ringing=TRUE;
	}
	if (call->ringing_beep){
		serphone_core_stop_dtmf();
		call->ringing_beep=FALSE;
	}

	serphone_core_get_default_proxy(&cfg);
	dest_proxy=cfg;
	dest_proxy=serphone_core_lookup_known_proxy(call->log->to);

	if (cfg!=dest_proxy && dest_proxy!=NULL) {
		PrintConsole("The used identity will be %s\n",dest_proxy->serphone_proxy_config_get_identity());
	}
	/*try to be best-effort in giving real local or routable contact address*/
	contact=get_fixed_contact(call,dest_proxy);
	if (contact)
		sal_op_set_contact(call->op,contact);

	if (call->m_AudioChannelID <0)
		serphone_call_init_media_streams(call);

	if (!was_ringing && call->m_AudioChannelID >=0){
		audio_stream_prepare_sound(call);
	}
/*
	if (!was_ringing && call->audiostream->ticker==NULL){
		audio_stream_prepare_sound(call->audiostream,sound_conf.play_sndcard,lc->sound_conf.capt_sndcard);
	}
*/
	if (params){
		call->params=*params;
		call->camera_active=call->params.has_video;
		update_local_media_description(call);
		sal_call_set_local_media_description(call->op,call->localdesc);
	}

	sal_call_accept(call->op);
	if (vtable.display_status!=NULL)
		vtable.display_status(this,_("Connected."));
	this->current_call=call;
	serphone_call_set_state(call,LinphoneCallConnected,"Connected");
	new_md=sal_call_get_final_media_description(call->op);
	serphone_core_update_streams(call, new_md);
	if (new_md){
		serphone_call_set_state(call,LinphoneCallStreamsRunning,"Connected (streams running)");
	}else call->media_pending=TRUE;

	if ( contact )  //this code added by zdm,avoid memory leak
	{
        ms_free((void **)&contact);
    }
	PrintConsole("call answered.\n");
//#ifdef MAC_IPHONE
//    selectCamera(pre_used_camera_index, m_usedCapabilityIndex, m_maxFPS, m_camerRotate,false);
//#endif
	return 0;
}

SerPhoneCall *ServiceCore::serphone_core_get_current_call()
{
	return current_call;
}

/**
 * Returns the current list of calls.
 *
 * Note that this list is read-only and might be changed by the core after a function call to serphone_core_iterate().
 * Similarly the LinphoneCall objects inside it might be destroyed without prior notice.
 * To hold references to LinphoneCall object into your program, you must use linphone_call_ref().
 *
 * @ingroup call_control
**/
const MSList *ServiceCore::serphone_core_get_calls()
{
	return calls;
}

void ServiceCore::terminate_call(SerPhoneCall *call)
{
	if (call->state==LinphoneCallIncomingReceived){
		call->reason=SerphoneReasonDeclined;
	}
	/*stop ringing*/
	if ( m_ringplay_flag ){
		ring_stop();
		m_ringplay_flag = FALSE;
		dmfs_playing_start_time = 0;
	}

	//mute change the system device status and will stay on the next startup
	//so resume when call end
	serphone_set_mute_status(false);
	serphone_set_speaker_mute_status(false);

	serphone_call_stop_media_streams(call);
	if (vtable.display_status!=NULL)
		vtable.display_status(this,_("Call ended") );
}


/**
 * Terminates a call.
 *
 * @ingroup call_control
 * @param the_call the SerPhoneCall object representing the call to be terminated.
**/
int ServiceCore::serphone_core_terminate_call(SerPhoneCall *the_call)
{
    cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	SerPhoneCall *call;
	if (the_call == NULL){
		call = serphone_core_get_current_call();
		if (ms_list_size(calls)==1){
			call=(SerPhoneCall*)calls->data;
		}else{
			PrintConsole("No unique call to terminate !\n");
			return -1;
		}
	}
	else
	{
		call = the_call;
	}
    processOriginalAudioData = false;
    if( !call->op || call->state == LinphoneCallReleased) {
        PrintConsole("Call already release , -2 \n");
        return -2;
    }
	if(call->reason == SerphoneReasonBusy) {
		sal_call_decline(call->op,  SalReasonBusy, NULL);
		sal_op_release(call->op);
	}
	else if (call->reason >=  SerphoneReasonUserDefinedError && call->reason < SerphoneReasonNone)
    {
        sal_call_decline(call->op, (SalReason)((int)call->reason+603), NULL);
        sal_op_release(call->op);
    }
    else
    {
		sal_call_terminate(call->op);
		terminate_call(call);
	}

	serphone_call_set_state(call,LinphoneCallEnd,"Call terminated");
	return 0;
}

/**
 * Terminates all the calls.
 *
 * @ingroup call_control
 **/
int ServiceCore::serphone_core_terminate_all_calls()
{
    cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	MSList *lp_calls=calls;
	while(calls) {
		SerPhoneCall *c=(SerPhoneCall*)lp_calls->data;
		calls=calls->next;
		serphone_core_terminate_call(c);
	}
	return 0;
}


/**
 * Interpret a call destination as supplied by the user, and returns a fully qualified
 * LinphoneAddress.
 *
 * A sip address should look like DisplayName <sip:username@domain:port> .
 * Basically this function performs the following tasks
 * - if a phone number is entered, prepend country prefix of the default proxy
 *   configuration, eventually escape the '+' by 00.
 * - if no domain part is supplied, append the domain name of the default proxy
 * - if no sip: is present, prepend it
 *
 * The result is a syntaxically correct SIP address.
**/
SerphoneAddress * ServiceCore::serphone_core_interpret_url(const char *url)
{
	enum_lookup_res_t *enumres=NULL;
	char *enum_domain=NULL;
	SerphoneProxyConfig *proxy=default_proxy;;
	char *tmpurl;
	SerphoneAddress *uri;

	if (is_enum(url,&enum_domain)){
		if (vtable.display_status!=NULL)
			vtable.display_status(this,_("Looking for telephone number destination..."));
		if (enum_lookup(enum_domain,&enumres)<0){
			if (vtable.display_status!=NULL)
				vtable.display_status(this,_("Could not resolve this number."));
			ms_free((void **)&enum_domain);
			return NULL;
		}
		ms_free((void **)&enum_domain);
		tmpurl=enumres->sip_address[0];
		uri=serphone_address_new(tmpurl);
		enum_lookup_res_free(enumres);
        enumres = NULL;
		return uri;
	}
	/* check if we have a "sip:" */
	if (strstr(url,"sip:")==NULL){
		/* this doesn't look like a true sip uri */
		if (strchr(url,'@')!=NULL){
			/* seems like sip: is missing !*/
			tmpurl=ms_strdup_printf("sip:%s",url);
			uri=serphone_address_new(tmpurl);
			ms_free((void **)&tmpurl);
			if (uri){
				return uri;
			}
		}

		if (proxy!=NULL){
			/* append the proxy domain suffix */
			const char *identity=proxy->serphone_proxy_config_get_identity();
			char normalized_username[128];
			uri=serphone_address_new(identity);
			if (uri==NULL){
				return NULL;
			}
			serphone_address_set_display_name(uri,NULL);
			proxy->serphone_proxy_config_normalize_number(url,normalized_username,
			    					sizeof(normalized_username));
			serphone_address_set_username(uri,normalized_username);
			return uri;
		}else return NULL;
	}
	uri=serphone_address_new(url);
	if (uri!=NULL){
		return uri;
	}
	/* else we could not do anything with url given by user, so display an error */
	if (vtable.display_warning!=NULL){
		vtable.display_warning(this,_("Could not parse given sip address. A sip url usually looks like sip:user@domain"));
	}
	return NULL;
}

/**
 * Initiates an outgoing call
 *
 * @ingroup call_control
 * @param url the destination of the call (sip address, or phone number).
 *
 * The application doesn't own a reference to the returned SerPhoneCall object.
 * Use linphone_call_ref() to safely keep the SerPhoneCall pointer valid within your application.
 *
 * @return a SerPhoneCall object or NULL in case of failure
**/
SerPhoneCall * ServiceCore::serphone_core_invite( const char *url ,char* userdata)
{
    cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	SerPhoneCall *call;

	SerphoneCallParams *p=serphone_core_create_default_call_parameters ();
	if( userdata)
		p->invite_userdata = ms_strdup(userdata);

    if( groupID)
		p->group_id = ms_strdup(groupIDAndNetworkType);
    internal_str_check_copy(&remoteSipNo, url);
//haiyuntong
#ifdef HAIYUNTONG
    if (enableHaiyuntong) {
        isAudioConf = false;
        if (!strncasecmp("conf", url, 4)) {
            isAudioConf = true;
            internal_str_check_copy(&confID, url);
        }
        else
        {
            char akey[1024] = {0};
            long akeyLen = 0;
            isLandingCall = false;
            if (!strncasecmp("0086", url, 4))
            {
                isLandingCall = true;
            }
            int ret = -1;
            if (isLandingCall)
            {
                PrintConsole("[DEBUG HAIYUNTONG] landing call when invite:%s",remoteSipNo);
                ret = inviteVOIPKeyRequest(remoteSipNo, strlen(remoteSipNo), akey, &akeyLen);
                //            ret = inviteVOIPKeyRequest("119119", strlen("119119"), akey, &akeyLen);
            }
            else
            {
                ret = inviteKeyRequest(remoteSipNo, strlen(remoteSipNo), akey, &akeyLen);
            }
            if (0 == ret) {
                PrintConsole("[DEBUG HAIYUNTONG] %s OK\n",isLandingCall?"inviteVOIPKeyRequest":"inviteKeyRequest");
                p->akey = ms_strdup(akey);
            }
            else
            {
                PrintConsole("[DEBUG HAIYUNTONG] %s failed, ret:%d\n",isLandingCall?"inviteVOIPKeyRequest":"inviteKeyRequest",ret);
                return NULL;
            }
        }
    }
#endif

	p->has_video = this->video_policy.automatically_initiate;
    if (srtp_enable) {
        p->media_encryption =LinphoneMediaEncryptionSRTP;
    }

	call=serphone_core_invite_with_params(url,p);
//    memcpy(call->_user_call_id, usercallid, 8);
//    call->_user_call_id[8] = '\0';

	serphone_call_params_destroy(p);
	return call;
}

/**
 * Initiates an outgoing call given a destination LinphoneAddress
 *
 * @ingroup call_control
 * @param lc the LinphoneCore object
 * @param addr the destination of the call (sip address).
 *
 * The LinphoneAddress can be constructed directly using linphone_address_new(), or
 * created by linphone_core_interpret_url().
 * The application doesn't own a reference to the returned LinphoneCall object.
 * Use linphone_call_ref() to safely keep the LinphoneCall pointer valid within your application.
 *
 * @return a LinphoneCall object or NULL in case of failure
**/
SerPhoneCall * ServiceCore::serphone_core_invite_address(const SerphoneAddress *addr)
{
	SerPhoneCall *call;
	SerphoneCallParams *p=serphone_core_create_default_call_parameters();
	p->has_video= video_policy.automatically_initiate;
	call=serphone_core_invite_address_with_params(addr,p);
	serphone_call_params_destroy(p);
	return call;
}

/**
 * Initiates an outgoing call according to supplied call parameters
 *
 * @ingroup call_control
 * @param lc the LinphoneCore object
 * @param url the destination of the call (sip address, or phone number).
 * @param p call parameters
 *
 * The application doesn't own a reference to the returned LinphoneCall object.
 * Use linphone_call_ref() to safely keep the LinphoneCall pointer valid within your application.
 *
 * @return a LinphoneCall object or NULL in case of failure
**/
SerPhoneCall * ServiceCore::serphone_core_invite_with_params(const char *url, const SerphoneCallParams *p)
{
	SerphoneAddress *addr=serphone_core_interpret_url(url);
	if (addr){
		SerPhoneCall *call;
		call=serphone_core_invite_address_with_params(addr,p);
		serphone_address_destroy(addr);
		return call;
	}
	return NULL;
}

/**
 * Initiates an outgoing call given a destination LinphoneAddress
 *
 * @ingroup call_control
 * @param lc the LinphoneCore object
 * @param addr the destination of the call (sip address).
	@param params call parameters
 *
 * The LinphoneAddress can be constructed directly using linphone_address_new(), or
 * created by linphone_core_interpret_url().
 * The application doesn't own a reference to the returned LinphoneCall object.
 * Use linphone_call_ref() to safely keep the LinphoneCall pointer valid within your application.
 *
 * @return a LinphoneCall object or NULL in case of failure
**/
SerPhoneCall * ServiceCore::serphone_core_invite_address_with_params(const SerphoneAddress *addr,
	const SerphoneCallParams *params)
{
    /*add begin------------------Sean20130812----------for video ice------------*/
    cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
    /*add end--------------------Sean20130812----------for video ice------------*/
	const char *route=NULL;
	const char *from=NULL;
	SerphoneProxyConfig *proxy=NULL;
	SerphoneAddress *parsed_url2=NULL;
	char *real_url=NULL;
	SerphoneProxyConfig *dest_proxy=NULL;
	SerPhoneCall *call;

    bool_t defer = FALSE;

	serphone_core_preempt_sound_resources();

	if(!serphone_core_can_we_add_call()){
		if (vtable.display_warning)
			vtable.display_warning(this,_("Sorry, we have reached the maximum number of simultaneous calls"));
		return NULL;
	}
	serphone_core_get_default_proxy(&proxy);
	route=serphone_core_get_route();
	real_url=serphone_address_as_string(addr);
	dest_proxy=serphone_core_lookup_known_proxy(addr);

	if (proxy!=dest_proxy && dest_proxy!=NULL) {
		PrintConsole("The used identity will be %s\n",dest_proxy->serphone_proxy_config_get_identity());
	}

	if (dest_proxy!=NULL)
		from=dest_proxy->serphone_proxy_config_get_identity();
	else if (proxy!=NULL)
		from=proxy->serphone_proxy_config_get_identity();

	/* if no proxy or no identity defined for this proxy, default to primary contact*/
	if (from==NULL) from=serphone_core_get_primary_contact();

	parsed_url2=serphone_address_new(from);

	call=serphone_call_new_outgoing(parsed_url2,serphone_address_clone(addr),params);
	sal_op_set_route(call->op,route);

	if(serphone_core_add_call(call)!= 0)
	{
		PrintConsole("we had a problem in adding the call into the invite ... weird\n");
		serphone_call_unref(call);
		//added  by zdm for avoiding memory leak
	    if (real_url!=NULL) {ms_free((void **)&real_url);}
		if (!parsed_url2 ) serphone_address_destroy(parsed_url2);
		return NULL;
	}
	/* this call becomes now the current one*/
	this->current_call=call;
	serphone_call_set_state (call,LinphoneCallOutgoingInit,"Starting outgoing call");

	serphone_call_init_media_streams(call);

    if (serphone_core_get_firewall_policy() == LinphonePolicyUseIce) {
		/* Defer the start of the call after the ICE gathering process. */
		//serphone_call_init_media_streams(call);
		serphone_call_start_media_streams_for_ice_gathering(call);//没有实现，没??		call->start_time=time(NULL);
		if (serphone_core_gather_ice_candidates(call)<0) {
			/* Ice candidates gathering failed, proceed with the call anyway. */
			serphone_call_delete_ice_session(call);
			serphone_call_stop_media_streams_for_ice_gathering(call);
		} else {
			defer = TRUE;
		}
	}

	if (dest_proxy!=NULL || sip_conf.ping_with_options==FALSE){
    //according to ice candidate collection to delay invite
        if (FALSE==defer) {
            serphone_core_start_invite(call,dest_proxy);
        }
	}else{
		/*defer the start of the call after the OPTIONS ping*/
		call->ping_op=sal_op_new(this->sal);
		sal_ping(call->ping_op,from,real_url);
		sal_op_set_user_pointer(call->ping_op,call);
		call->start_time=time(NULL);
	}

    if (real_url!=NULL) {ms_free((void **)&real_url);}
	return call;
}

/**
 * Performs a simple call transfer to the specified destination.
 *
 * The remote endpoint is expected to issue a new call to the specified destination.
 * The current call remains active and thus can be later paused or terminated.
**/
int ServiceCore::serphone_core_transfer_call( SerPhoneCall *call, const char *url, int type)
{
     cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	char *real_url=NULL;
	SerphoneAddress *real_parsed_url=serphone_core_interpret_url(referTo);

	if (!real_parsed_url){
		/* bad url */
		return -1;
	}
	if (call==NULL){
		PrintConsole("No established call to refer.\n");
		return -1;
	}

    if (LinphonePolicyUseIce == serphone_core_get_firewall_policy() && isRefering && call->localdesc->ice_completed) {
        if (call->ice_session != NULL) {
            PrintConsole("ICE candidates gathering from [%s] has not finished yet, proceed with the call without ICE anyway.\n"
                         ,serphone_core_get_stun_server());
            serphone_call_delete_ice_session(call);
            serphone_call_stop_media_streams_for_ice_gathering(call);
        }
        call->localdesc->streams[0].rtp_port = 0;
        serphone_core_update_call(call, &call->current_params);
    }
    else
    {
        real_url=serphone_address_as_string (real_parsed_url);
        call->localdesc->streams[0].rtp_port = 0;

        if (call->params.has_video) {
            call->localdesc->streams[1].rtp_port = 0;
        }
        sal_call_refer(call->op,real_url);
        ms_free((void **)&real_url);
        serphone_address_destroy(real_parsed_url);
        serphone_call_set_transfer_state(call, LinphoneCallOutgoingInit);
    }
	return 0;
}



/**
 * Transfer a call to destination of another running call. This is used for "attended transfer" scenarios.
 * @param call a running call you want to transfer
 * @param dest a running call whose remote person will receive the transfer
 *
 * The transfered call is supposed to be in paused state, so that it is able to accept the transfer immediately.
 * The destination call is a call previously established to introduce the transfered person.
 * This method will send a transfer request to the transfered person. The phone of the transfered is then
 * expected to automatically call to the destination of the transfer. The receiver of the transfer will then automatically
 * close the call with us (the 'dest' call).
**/
int ServiceCore::serphone_core_transfer_call_to_another(SerPhoneCall *call, SerPhoneCall *dest)
{
	int result = sal_call_refer_with_replaces (call->op,dest->op);
	serphone_call_set_transfer_state(call, LinphoneCallOutgoingInit);
	return result;
}

bool_t ServiceCore::serphone_core_inc_invite_pending()
{
	SerPhoneCall *call = serphone_core_get_current_call();
	if(call != NULL)
	{
		if(call->dir==LinphoneCallIncoming
			&& (call->state == LinphoneCallIncomingReceived || call->state ==  LinphoneCallIncomingEarlyMedia))
			return TRUE;
	}
	return FALSE;
}

/**
 * Returns TRUE if there is a call running.
 *
 * @ingroup call_control
**/
bool_t ServiceCore::serphone_core_in_call()
{
	return serphone_core_get_current_call()!=NULL;  // || linphone_core_is_in_conference(lc); commented by zdm
}

int ServiceCore::serphone_core_start_invite(SerPhoneCall *call, SerphoneProxyConfig *dest_proxy)
{
	int err;
	char *contact;
	char *real_url,*barmsg;
	char *from;

	/*try to be best-effort in giving real local or routable contact address */
	contact=get_fixed_contact(call,dest_proxy);
	if (contact){
		sal_op_set_contact(call->op, contact);
		ms_free((void **)&contact);
	}
//	call->localdesc = create_local_media_description(call);
    create_local_media_description(call);
	//serphone_core_stop_dtmf_stream();
	//serphone_call_init_media_streams(call);

	if (!this->m_ringplay_flag)
		audio_stream_prepare_sound(call);
	if (!sip_conf.sdp_200_ack){
		call->media_pending=TRUE;
		sal_call_set_local_media_description(call->op,call->localdesc);
	}
	real_url=serphone_address_as_string(call->log->to);
	from=serphone_address_as_string(call->log->from);

    call->op->base.local_media->usermode = user_mode;
	err=sal_call(call->op,from,real_url);
	if (sip_conf.sdp_200_ack){
		call->media_pending=TRUE;
		sal_call_set_local_media_description(call->op,call->localdesc);
	}
	barmsg=ser_strdup_printf("%s %s", _("Contacting"), real_url);
	if (vtable.display_status!=NULL)
		vtable.display_status(this,barmsg);
	ms_free((void **)&barmsg);

	if (err<0){
		if (vtable.display_status!=NULL)
			vtable.display_status(this,_("Could not call"));
		serphone_call_stop_media_streams(call);
		serphone_call_set_state(call,LinphoneCallError,"Call failed");
	}else {
		serphone_call_set_state(call,LinphoneCallOutgoingProgress,"Outgoing call in progress");
	}
	ms_free((void **)&real_url);
	ms_free((void **)&from);
	return err;
}

void ServiceCore::serphone_core_start_refered_call(SerPhoneCall *call)
{
	if (call->refer_pending){
		SerphoneCallParams *cp=serphone_core_create_default_call_parameters();
		SerPhoneCall *newcall;
		cp->has_video = video_policy.automatically_initiate;
		cp->referer=call;
		call->refer_pending=FALSE;
		newcall=serphone_core_invite_with_params(call->refer_to,cp);
		serphone_call_params_destroy(cp);
		if (newcall) serphone_core_notify_refer_state(call,newcall);
	}
}

void ServiceCore::serphone_core_notify_refer_state(SerPhoneCall *referer, SerPhoneCall *newcall)
{
	if (referer->op!=NULL){
		sal_call_notify_refer_state(referer->op,newcall ? newcall->op : NULL);
	}
}

char *ServiceCore::get_fixed_contact(SerPhoneCall *call , SerphoneProxyConfig *dest_proxy)
{
	SerphoneAddress *ctt;
	const char *localip=call->localip;

	/* first use user's supplied ip address if asked*/
	if (serphone_core_get_firewall_policy()==LinphonePolicyUseNatAddress){
		ctt=serphone_core_get_primary_contact_parsed();
		const char *nataddr = serphone_core_get_nat_address_resolved();
		if( ctt && nataddr) {
			return ms_strdup_printf("sip:%s@%s",serphone_address_get_username(ctt),nataddr);
		}
	}

	/* if already choosed, don't change it */
	if (call->op && sal_op_get_contact(call->op)!=NULL){
		return NULL;
	}
	/* if the ping OPTIONS request succeeded use the contact guessed from the
	 received, rport*/
	if (call->ping_op){
		const char *guessed=sal_op_get_contact(call->ping_op);
		if (guessed){
			PrintConsole("Contact has been fixed using OPTIONS to %s\n",guessed);
			return ms_strdup(guessed);
		}
	}

	/*if using a proxy, use the contact address as guessed with the REGISTERs*/
	if (dest_proxy && dest_proxy->op){
		const char *fixed_contact=sal_op_get_contact(dest_proxy->op);
		if (fixed_contact) {
			return ms_strdup(fixed_contact);
		}
	}

	ctt= serphone_core_get_primary_contact_parsed();
	if(ctt!=NULL) {
		char * ret = serphone_address_as_string(ctt);
		serphone_address_destroy(ctt);
		return ret;
	}
	/*if (ctt!=NULL){
		char *ret;
		//otherwise use supllied localip
		serphone_address_set_domain(ctt,localip);
		serphone_address_set_port_int(ctt,serphone_core_get_sip_port( ));
		ret=serphone_address_as_string_uri_only(ctt);
		serphone_address_destroy(ctt);
		PrintConsole("Contact has been fixed using local ip to %s\n",ret);
		return ret;
	}*/
	return NULL;
}

int ServiceCore::serphone_core_get_current_call_duration(){
	SerPhoneCall *call=serphone_core_get_current_call();
	if (call)  return serphone_call_get_duration(call);
	return -1;
}

SerPhoneCall * ServiceCore::serphone_call_new_outgoing(SerphoneAddress *from, SerphoneAddress *to,
		const SerphoneCallParams *params)
{
//	SerPhoneCall *call=ms_new0(SerPhoneCall,1);   //ms_new0
	SerPhoneCall *call=(SerPhoneCall *)malloc(sizeof(SerPhoneCall)*1);   //ms_new0
	memset((void *)call,0,sizeof(SerPhoneCall)*1);

    struct tm *pt = NULL;
	time_t curr_time;
    curr_time = time(NULL);
#ifdef WIN32
    pt = localtime(&curr_time);
#else
    struct tm t1;
    pt = localtime_r(&curr_time,&t1);
#endif
    char userCallID[16] = {'\0'};
    sprintf(userCallID, "%02d%02d%02d%02d",pt->tm_mday,pt->tm_hour,pt->tm_min,pt->tm_sec);
    memcpy(call->_user_call_id, userCallID, 8);
    call->_user_call_id[8] = '\0';

	call->dir=LinphoneCallOutgoing;
	call->op=sal_op_new(sal);
	sal_op_set_user_pointer(call->op,call);
	call->core=this;
	call->m_AudioChannelID = -1;
	call->m_VideoChannelID = -1;
	call->m_CaptureDeviceId = -1;
	call->m_desktopShareDeviceId = -1;
	call->callConnected = false;

    call->params.media_encryption = params->media_encryption;
	if(params->invite_userdata)
		call->op->invite_userdata =ms_strdup( params->invite_userdata);

    if(params->group_id)
		call->op->group_id =ms_strdup( params->group_id);

//haiyuntong
#ifdef HAIYUNTONG
    if (params->akey) {
        call->op->akey = ms_strdup(params->akey);
    }
#endif

	serphone_core_get_local_ip(serphone_address_get_domain(to),call->localip);
	PrintConsole("get media stream address is %s\n",call->localip);
	serphone_call_init_common(call,from,to);
	call->params=*params;

//	call->localdesc=create_local_media_description (call);
    create_local_media_description (call);
	call->camera_active=params->has_video;

    //sean ice
    if (serphone_core_get_firewall_policy() == LinphonePolicyUseIce) {
        PrintConsole("Outgoing call using LinphonePolicyUseIce, stunserver = %s\n",serphone_core_get_stun_server());
		call->ice_session = ice_session_new();
		ice_session_set_role(call->ice_session, IR_Controlling);
	}
    else
    {
        PrintConsole("Outgoing call not using LinphonePolicyUseIce");
    }

	if (serphone_core_get_firewall_policy()==LinphonePolicyUseStun)
		serphone_core_run_stun_tests(call);
	PrintConsole("discover mtu\n");
	discover_mtu(serphone_address_get_domain (to));
	PrintConsole("discover mtu end\n");
	if (params->referer){
		sal_call_set_referer(call->op,params->referer->op);
		call->referer=serphone_call_ref(params->referer);
	}
	return call;
}

int ServiceCore::serphone_core_abort_call(SerPhoneCall *call, const char *error)
{
	sal_call_terminate(call->op);

	/*stop ringing*/
	if ( m_ringplay_flag ){
		ring_stop();
		m_ringplay_flag = FALSE;
		dmfs_playing_start_time = 0;
	}
	serphone_call_stop_media_streams(call);
	if (vtable.display_status!=NULL)
		vtable.display_status(this,_("Call aborted") );
	serphone_call_set_state(call,LinphoneCallError,error);
	return 0;
}

/**
 * @ingroup call_control
 * Accept call modifications initiated by other end.
 *
 * This call may be performed in response to a #LinphoneCallUpdatedByRemote state notification.
 * When such notification arrives, the application can decide to call linphone_core_defer_update_call() so that it can
 * have the time to prompt the user. linphone_call_get_remote_params() can be used to get information about the call parameters
 * requested by the other party, such as whether a video stream is requested.
 *
 * When the user accepts or refuse the change, linphone_core_accept_call_update() can be done to answer to the other party.
 * If params is NULL, then the same call parameters established before the update request will continue to be used (no change).
 * If params is not NULL, then the update will be accepted according to the parameters passed.
 * Typical example is when a user accepts to start video, then params should indicate that video stream should be used
 * (see linphone_call_params_enable_video()).
 * @param lc the linphone core object.
 * @param call the LinphoneCall object
 * @param params a LinphoneCallParams object describing the call parameters to accept.
 * @Returns 0 if sucessful, -1 otherwise (actually when this function call is performed outside ot #LinphoneCallUpdatedByRemote state).
**/
int ServiceCore::serphone_core_accept_call_update(SerPhoneCall *call, const SerphoneCallParams *params)
{
    cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
    SalMediaDescription *remote_desc;
	bool_t keep_sdp_version;
#ifdef VIDEO_ENABLED
	bool_t old_has_video = call->params.has_video;
#endif
	if (call->state!=LinphoneCallUpdatedByRemote){
		PrintConsole("Serphone_core_accept_update(): invalid state %s to call this function.\n",
		         serphone_call_state_to_string(call->state));
		return -1;
	}
	remote_desc = sal_call_get_remote_media_description(call->op);
	keep_sdp_version = lp_config_get_int(this->config, "sip", "keep_sdp_version", 0);
	if (keep_sdp_version &&(remote_desc->session_id == call->remote_session_id) && (remote_desc->session_ver == call->remote_session_ver)) {
		/* Remote has sent an INVITE with the same SDP as before, so send a 200 OK with the same SDP as before. */
		PrintConsole("SDP version has not changed, send same SDP as before.\n");
		sal_call_accept(call->op);
		serphone_call_set_state(call,LinphoneCallStreamsRunning,"Connected (streams running)");
		return 0;
	}
	if (params==NULL){
		call->params.has_video=this->video_policy.automatically_accept || call->current_params.has_video;
	}else
		call->params=*params;

//	if (call->params.has_video && !serphone_core_video_enabled()){
//		PrintConsole("linphone_core_accept_call_update(): requested video but video support is globally disabled. Refusing video.\n");
//		call->params.has_video=FALSE;
//	}
	if (call->current_params.in_conference) {
		PrintConsole("Video isn't supported in conference");
		call->params.has_video = FALSE;
	}
//    sean ice video
	call->params.has_video &= serphone_core_media_description_contains_video_stream(remote_desc);
    //To ensure caller invite only with audio,but callee answer with both audio and video,here remove video
    if (!call->params.has_video && call->videostream) {
        video_stream_stop(call->m_VideoChannelID,call->m_CaptureDeviceId);
    }
	call->camera_active=call->params.has_video;
//	serphone_call_make_local_media_description(lc,call);

    //if reinvite has no ice, remove local ice session
    if (call->ice_session != NULL) {
        if ((remote_desc->ice_pwd[0] != '\0') && (remote_desc->ice_ufrag[0] != '\0')) {
//            for (int i = 0; i < remote_desc->n_total_streams; i++) {
//                const SalStreamDescription *stream = &remote_desc->streams[i];
//                const SalIceCandidate *candidate = &stream->ice_candidates[0];
//                if (candidate->addr[0] == '\0')
//                {
//                    serphone_call_delete_ice_session(call);
//                    break;
//                }
//            }
        }
        else {
            serphone_call_delete_ice_session(call);
        }
    }

    update_local_media_description(call);
	if (call->ice_session != NULL) {
		serphone_core_update_ice_from_remote_media_description(call, remote_desc);
#ifdef VIDEO_ENABLED
//        sean ice video
		if ((call->ice_session != NULL) &&!ice_session_candidates_gathered(call->ice_session)) {
			if ((call->params.has_video) && (call->params.has_video != old_has_video)) {
				serphone_call_init_video_stream(call);
				video_stream_prepare_video(call->videostream);
				if (serphone_core_gather_ice_candidates(call)<0) {
					/* Ice candidates gathering failed, proceed with the call anyway. */
					serphone_call_delete_ice_session(call);
				} else return 0;
			}
		}
#endif //VIDEO_ENABLED
	}

#if BUILD_UPNP
	if(call->upnp_session != NULL) {
		linphone_core_update_upnp_from_remote_media_description(call, sal_call_get_remote_media_description(call->op));
#ifdef VIDEO_ENABLED
		if ((call->params.has_video) && (call->params.has_video != old_has_video)) {
			linphone_call_init_video_stream(call);
			video_stream_prepare_video(call->videostream);
			if (linphone_core_update_upnp(lc, call)<0) {
				/* uPnP update failed, proceed with the call anyway. */
				linphone_call_delete_upnp_session(call);
			} else return 0;
		}
#endif //VIDEO_ENABLED
	}
#endif //BUILD_UPNP

	serphone_core_start_accept_call_update(call);
	return 0;
}


/**
 * Pauses the call. If a music file has been setup using linphone_core_set_play_file(),
 * this file will be played to the remote user.
 *
 * @ingroup call_control
**/
int ServiceCore::serphone_core_pause_call(SerPhoneCall *call)
{
    cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	const char *subject=NULL;

	if (call->state!=LinphoneCallStreamsRunning && call->state!=LinphoneCallPausedByRemote){
		PrintConsole("Cannot pause this call, it is not active.\n");
		return -1;
	}
	update_local_media_description(call);
//    sean pause resume add begin 20140113
    if (call->ice_session != NULL) {
		serphone_core_update_local_media_description_from_ice(call->localdesc, call->ice_session);
	}
//    sean pause resume add end 20140113
	if (sal_media_description_has_dir(call->resultdesc,SalStreamSendRecv)){
		sal_media_description_set_dir(call->localdesc,SalStreamSendOnly);
		subject="Call on hold";
	}else if (sal_media_description_has_dir(call->resultdesc,SalStreamRecvOnly)){
		sal_media_description_set_dir(call->localdesc,SalStreamSendOnly);
		subject="Call on hold for me too";
	}else{
		PrintConsole("No reason to pause this call, it is already paused or inactive.\n");
		return -1;
	}
	sal_call_set_local_media_description(call->op,call->localdesc);
	if (sal_call_update(call->op,subject) != 0){
		if (vtable.display_warning)
			vtable.display_warning(this,_("Could not pause the call"));
	}
	this->current_call=NULL;
	serphone_call_set_state(call,LinphoneCallPausing,"Pausing call");
	if (vtable.display_status)
		vtable.display_status(this,_("Pausing the current call..."));
	if (call->m_AudioChannelID>=0 || call->m_VideoChannelID >=0 )
		serphone_call_stop_media_streams (call);
	return 0;
}

/**
 * Pause all currently running calls.
**/
int ServiceCore::serphone_core_pause_all_calls()
{
	const MSList *elem;
	for(elem=this->calls;elem!=NULL;elem=elem->next){
		SerPhoneCall *call=(SerPhoneCall *)elem->data;
		SerphoneCallState cs=serphone_call_get_state(call);
		if (cs==LinphoneCallStreamsRunning || cs==LinphoneCallPausedByRemote){
			serphone_core_pause_call(call);
		}
	}
	return 0;
}

/**
 * Resumes the call.
 *
 * @ingroup call_control
**/
int ServiceCore::serphone_core_resume_call(SerPhoneCall *the_call)
{
    cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	char temp[255]={0};
	SerPhoneCall *call = the_call;
	const char *subject="Call resuming";

	if(call->state!=LinphoneCallPaused ){
		PrintConsole("we cannot resume a call that has not been established and paused before\n");
		return -1;
	}
	if (call->params.in_conference==FALSE){
		serphone_core_preempt_sound_resources();
		PrintConsole("Resuming call %p\n",call);
	}

	/* Stop playing music immediately. If remote side is a conference it
	 prevents the participants to hear it while the 200OK comes back.*/
	if (call->m_AudioChannelID >=0 ) audio_stream_play(call, NULL);

	update_local_media_description(the_call);

    if (call->ice_session != NULL) {
		serphone_core_update_local_media_description_from_ice(call->localdesc, call->ice_session);
	}

	sal_call_set_local_media_description(call->op,call->localdesc);
	sal_media_description_set_dir(call->localdesc,SalStreamSendRecv);
	if (call->params.in_conference && !call->current_params.in_conference) subject="Conference";
	if(sal_call_update(call->op,subject) != 0){
		return -1;
	}
	serphone_call_set_state (call,LinphoneCallResuming,"Resuming");
	char *tmp = serphone_call_get_remote_address_as_string(call);
	snprintf(temp,sizeof(temp)-1,"Resuming the call with %s",tmp); //modified by zdm for avoiding memory leak
	ms_free((void **)&tmp);
	if (vtable.display_status)
		vtable.display_status(this,temp);
	return 0;
}


/**
 * @ingroup call_control
 * Updates a running call according to supplied call parameters or parameters changed in the LinphoneCore.
 *
 * In this version this is limited to the following use cases:
 * - setting up/down the video stream according to the video parameter of the LinphoneCallParams (see linphone_call_params_enable_video() ).
 * - changing the size of the transmitted video after calling linphone_core_set_preferred_video_size()
 *
 * In case no changes are requested through the LinphoneCallParams argument, then this argument can be omitted and set to NULL.
 * @param call the call to be updated
 * @param params the new call parameters to use. (may be NULL)
 * @return 0 if successful, -1 otherwise.
**/
int ServiceCore::serphone_core_update_call(SerPhoneCall *call, const SerphoneCallParams *params)
{
	int err=0;
	if (params!=NULL){
		const char *subject;
		call->params=*params;
		call->camera_active=call->params.has_video;
		update_local_media_description(call);

		if (params->in_conference){
			subject="Conference";
		}else{
			subject="Media change";
		}
		if (vtable.display_status)
			vtable.display_status(this,_("Modifying call parameters..."));
		sal_call_set_local_media_description (call->op,call->localdesc);
		err=sal_call_update(call->op,subject);
	}else{
#ifdef VIDEO_ENABLED
		/*if (call->videostream!=NULL){
			video_stream_set_sent_video_size(call->videostream,linphone_core_get_preferred_video_size(this));
			if (call->camera_active && call->videostream->cam!=this->video_conf.device){
				video_stream_change_camera(call->videostream,this->video_conf.device);
			}else video_stream_update_video_params(call->videostream);
		}*/
#endif
	}

	return err;
}

/**
 * @ingroup call_control
 * When receiving a #LinphoneCallUpdatedByRemote state notification, prevent LinphoneCore from performing an automatic answer.
 *
 * When receiving a #LinphoneCallUpdatedByRemote state notification (ie an incoming reINVITE), the default behaviour of
 * LinphoneCore is to automatically answer the reINIVTE with call parameters unchanged.
 * However when for example when the remote party updated the call to propose a video stream, it can be useful
 * to prompt the user before answering. This can be achieved by calling linphone_core_defer_call_update() during
 * the call state notifiacation, to deactivate the automatic answer that would just confirm the audio but reject the video.
 * Then, when the user responds to dialog prompt, it becomes possible to call linphone_core_accept_call_update() to answer
 * the reINVITE, with eventually video enabled in the LinphoneCallParams argument.
 *
 * @Returns 0 if successful, -1 if the linphone_core_defer_call_update() was done outside a #LinphoneCallUpdatedByRemote notification, which is illegal.
**/
int ServiceCore::serphone_core_defer_call_update(SerPhoneCall *call)
{
	if (call->state==LinphoneCallUpdatedByRemote){
		call->defer_update=TRUE;
		return 0;
	}
	PrintConsole("serphone_core_defer_call_update() not done in state LinphoneCallUpdatedByRemote\n");
	return -1;
}


/**
 * Adds authentication information to the serphoneCore.
 *
 * This information will be used during all SIP transacations that require authentication.
**/
void ServiceCore::serphone_core_add_auth_info(const SerphoneAuthInfo *info)
{
	SerphoneAuthInfo *ai;
	MSList *elem;
	MSList *l;

	/* find if we are attempting to modify an existing auth info */
	ai=(SerphoneAuthInfo*)serphone_core_find_auth_info(info->realm,info->username);
	if (ai!=NULL){
		auth_info=ms_list_remove(auth_info,ai);
		serphone_auth_info_destroy(&ai);
	}
	auth_info=ms_list_append(auth_info,serphone_auth_info_clone(info));
	/* retry pending authentication operations */
	for(l=elem=sal_get_pending_auths(sal);elem!=NULL;elem=elem->next){
		const char *username,*realm;
		SalOp *op=(SalOp*)elem->data;
		SerphoneAuthInfo *ai;
		sal_op_get_auth_requested(op,&realm,&username);
		ai=(SerphoneAuthInfo*)serphone_core_find_auth_info(realm,username);
		if (ai){
			SalAuthInfo sai;
			sai.username=ai->username;
			sai.userid=ai->userid;
			sai.realm=ai->realm;
			sai.password=ai->passwd;
			sal_op_authenticate(op,&sai);
			ai->usecount++;
		}
	}
	ms_list_free(l);
	write_auth_infos();
}

/**
 * Removes an authentication information object.
**/
void ServiceCore::serphone_core_remove_auth_info(const SerphoneAuthInfo *info)
{
	SerphoneAuthInfo *r;
	r=(SerphoneAuthInfo*)serphone_core_find_auth_info(info->realm,info->username);
	if (r){
		auth_info=ms_list_remove(auth_info,r);
		/*printf("len=%i newlen=%i\n",len,newlen);*/
		serphone_auth_info_destroy(&r);
		write_auth_infos();
	}
}

/**
 * Returns an unmodifiable list of currently entered SerphoneAuthInfo.
**/
const MSList *ServiceCore::serphone_core_get_auth_info_list()
{
     return auth_info;
}

/**
 * Retrieves a LinphoneAuthInfo previously entered into the LinphoneCore.
**/
const SerphoneAuthInfo *ServiceCore::serphone_core_find_auth_info(const char *realm, const char *username)
{
	MSList *elem;
	SerphoneAuthInfo *ret=NULL,*candidate=NULL;
	for (elem=auth_info;elem!=NULL;elem=elem->next){
		SerphoneAuthInfo *pinfo=(SerphoneAuthInfo*)elem->data;
		if (realm==NULL){
			/*return the authinfo for any realm provided that there is only one for that username*/
			if (key_match(pinfo->username,username)){
				if (ret!=NULL){
					PrintConsole("There are several auth info for username '%s'\n",username);
					return NULL;
				}
				ret=pinfo;
			}
		}else{
			/*return the exact authinfo, or an authinfo for which realm was not supplied yet*/
			if (pinfo->realm!=NULL){
				if (realm_match(pinfo->realm,realm)
					&& key_match(pinfo->username,username))
					ret=pinfo;
			}else {
				if (key_match(pinfo->username,username))
					candidate=pinfo;
			}
		}
	}
	if (ret==NULL && candidate!=NULL)
		ret=candidate;
	return ret;
}

void ServiceCore::serphone_core_abort_authentication( SerphoneAuthInfo *info)
{
}

/**
 * Clear all authentication information.
**/
void ServiceCore::serphone_core_clear_all_auth_info()
{
	MSList *elem;
	int i;
	for(i=0,elem=auth_info;elem!=NULL;elem=ms_list_next(elem),i++){
		SerphoneAuthInfo *info=(SerphoneAuthInfo*)elem->data;
		serphone_auth_info_destroy(&info);
		serphone_auth_info_write_config(config,NULL,i);
	}
	ms_list_free(auth_info);
	auth_info=NULL;
}

/**
 * Update authentication information when receive different realm.
**/
void ServiceCore::serphone_core_update_auth_info(const char *realm, const char *username)
{
	if(!realm  || strlen(realm) == 0)
		return;

	MSList *elem;
	for (elem=auth_info;elem!=NULL;elem=elem->next){
		SerphoneAuthInfo *pinfo=(SerphoneAuthInfo*)elem->data;
		if (key_match(pinfo->username,username)) {
			if(pinfo->realm) {
				ms_free((void**)&pinfo->realm);
			}
			pinfo->realm = ms_strdup(realm);
		}
	}
}

void ServiceCore::write_auth_infos()
{
	MSList *elem;
	int i;

	if (!serphone_core_ready()) return;
	for(elem=auth_info,i=0;elem!=NULL;elem=ms_list_next(elem),i++){
		SerphoneAuthInfo *ai=(SerphoneAuthInfo*)(elem->data);
		serphone_auth_info_write_config(config,ai,i);
	}
	serphone_auth_info_write_config(config,NULL,i); /* mark the end */
}

void ServiceCore::serphone_core_write_friends_config()
{
	MSList *elem;
	int i;
	if (! serphone_core_ready()) return; /*dont write config when reading it !*/
	for (elem=friends,i=0; elem!=NULL; elem=ms_list_next(elem),i++){
		serphone_friend_write_to_config_file(config,(SerphoneFriend*)elem->data,i);
	}
	serphone_friend_write_to_config_file(config,NULL,i);	/* set the end */
}

SerphoneGlobalState ServiceCore::serphone_core_get_global_state()
{
	return state;
}

static int remote_address_compare(SerPhoneCall *call, const SerphoneAddress *raddr){
	const SerphoneAddress *addr=serphone_call_get_remote_address (call);
	return !serphone_address_weak_equal (addr,raddr);
}

/**
 * Get the call with the remote_address specified
 * @param remote_address
 * @return the SerPhoneCall of the call if found
 */
SerPhoneCall *ServiceCore::serphone_core_get_call_by_remote_address(const char *remote_address)
{
	SerphoneAddress *raddr=serphone_address_new(remote_address);
	MSList *elem=ms_list_find_custom(this->calls,(int (*)(const void*,const void *))remote_address_compare,raddr);
	if ( raddr )                    //the following two line added by zdm for avoiding memory leak
		serphone_address_destroy(raddr);
	if (elem)
		return (SerPhoneCall*) elem->data;
	return NULL;
}

void ServiceCore::serphone_core_set_state(SerphoneGlobalState gstate, const char *message)
{
	this->state=gstate;
	if (this->vtable.global_state_changed){
		this->vtable.global_state_changed(this,gstate,message);
	}
}

void ServiceCore::serphone_core_assign_payload_type( PayloadType *const_pt, int number, const char *recv_fmtp)
{
	PayloadType *pt;
	pt=payload_type_clone(const_pt);
	if (number==-1){
		/*look for a free number */
		MSList *elem;
		int i;
		for(i=this->dyn_pt;i<=127;++i){
			bool_t already_assigned=FALSE;
			for(elem=this->payload_types;elem!=NULL;elem=elem->next){
				PayloadType *it=(PayloadType*)elem->data;
				if (payload_type_get_number(it)==i){
					already_assigned=TRUE;
					break;
				}
			}
			if (!already_assigned){
				number=i;
				this->dyn_pt=i+1;

				break;
			}
		}
		if (number==-1){
			PrintConsole("FIXME: too many codecs, no more free numbers.\n");
		}
	}
	PrintConsole("assigning %s/%i payload type number %i\n",pt->mime_type,pt->clock_rate,number);
	payload_type_set_number(pt,number);
	if (recv_fmtp!=NULL) payload_type_set_recv_fmtp(pt,recv_fmtp);
	rtp_profile_set_payload(&av_profile,number,pt);
	this->payload_types=ms_list_append(this->payload_types,pt);
}

void ServiceCore::serphone_core_free_payload_types()
{
//	ms_list_for_each(this->payload_types,(void (*)(void**))payload_type_destroy);
    MSList *tempList = this->payload_types;
    for(;tempList!=NULL;tempList=tempList->next){
        payload_type_destroy((PayloadType *)tempList->data);
    }
	ms_list_free(this->payload_types);
	this->payload_types=NULL;
}

void ServiceCore::serphone_core_handle_static_payloads()
{
	RtpProfile *prof=&av_profile;
	int i;
	for(i=0;i<128;++i){
		PayloadType *pt=rtp_profile_get_payload(prof,i);
		if (pt){
			if (payload_type_get_number(pt)!=i){
				serphone_core_assign_payload_type( pt,i,NULL);
			}
		}
	}
}

void ServiceCore::serphone_core_init (const SerphoneCoreVTable *vtable, const char *config_path,
              const char *factory_config_path, void * userdata)
{
	this->data=userdata;
	this->ringstream_autorelease=TRUE;

	memcpy(&this->vtable,vtable,sizeof(SerphoneCoreVTable));

	serphone_core_set_state(LinphoneGlobalStartup,"Starting up");
	this->dyn_pt=96;

//    //payloadtype 不要修改，和codedatabase.cc里面对应
 //    serphone_core_assign_payload_type(&payload_type_opus, 124, NULL);//48k
 //    serphone_core_assign_payload_type(&payload_type_opus8k, 121, NULL);
  //   serphone_core_assign_payload_type(&payload_type_opus16k, 122, NULL);

//    serphone_core_assign_payload_type(&payload_type_silk_nb,111,NULL);
//    serphone_core_assign_payload_type(&payload_type_silk_mb,112,NULL);
//    serphone_core_assign_payload_type(&payload_type_silk_wb,113,NULL);
//    serphone_core_assign_payload_type(&payload_type_ilbc,97,NULL);

 //	serphone_core_assign_payload_type(&payload_type_g729,18,"annexb=no");
 	serphone_core_assign_payload_type(&payload_type_pcmu8000,0,NULL);
	serphone_core_assign_payload_type(&ccp_payload_type_telephone_event,106,"0-15");

//    serphone_core_assign_payload_type(&payload_type_cn8k,13,NULL);
//    serphone_core_assign_payload_type(&payload_type_amr, 105, NULL);



	//xzq add for support other codec
	//serphone_core_assign_payload_type(&payload_type_g7231,4,NULL);

#if defined(ANDROID) || defined (__IPHONE_OS_VERSION_MIN_REQUIRED)
	/*shorten the DNS lookup time and send more retransmissions on mobiles:
	 - to workaround potential packet losses
	 - to avoid hanging for 30 seconds when the network doesn't work despite the phone thinks it does.
	 */
//hubin	_linphone_core_configure_resolver();
#endif

#ifdef VIDEO_ENABLED
//	serphone_core_assign_payload_type(&payload_type_h264,-1,"profile-level-id=42e01e; packetization-mode=1; max-br=452; max-mbps=11880");
	serphone_core_assign_payload_type(&payload_type_vp8,120,NULL);

   // serphone_core_assign_payload_type(&payload_type_h264_svc,98, "profile-level-id=428014"); //profile-level-id need to be fixed.
	/* due to limited space in SDP, we have to disable this h264 line which is normally no more necessary */
	/* serphone_core_assign_payload_type(&payload_type_h264,-1,"packetization-mode=1;profile-level-id=428014");*/
#endif

	/*add all payload type for which we don't care about the number */
	//固定编码的放前面，动态编码的放后????0','101'，防止被占用,动态编码用'-1'表示 zdm
	//serphone_core_assign_payload_type(&payload_type_amr,-1,"octet-align=1");
//	serphone_core_assign_payload_type(&payload_type_amrwb,-1,"octet-align=1");

	serphone_core_handle_static_payloads();

	/* create a mediastreamer2 event queue and set it as global */
	/* This allows to run event's callback in serphone_core_iterate() */
	this->config=lp_config_new(config_path);
	if (factory_config_path)
		lp_config_read_file(this->config,factory_config_path);

	this->sal=sal_init();
	this->sal->m_parentLock =m_criticalSection;

	sal_set_user_pointer(this->sal,this);
	sal_set_callbacks(this->sal,&serphone_sal_callbacks);

	sip_setup_register_all();

	sound_config_read();
	net_config_read();
	rtp_config_read();
	codecs_config_read();
    video_config_read();
	sip_config_read(); /* this will start eXosip*/
//	video_config_read(lc);
	//autoreplier_config_init(&lc->autoreplier_conf);
	this->presence_mode=LinphoneStatusOnline;
	misc_config_read();
	ui_config_read();
#ifdef VIDEO_ENABLED
	ECMedia_init_video();
#endif

#ifdef WIN32
    //media_init_audio();
	ECMedia_init_audio();
#endif

#ifdef TUNNEL_ENABLED
	this->tunnel=linphone_core_tunnel_new(lc);
	if (this->tunnel) linphone_tunnel_configure(lc->tunnel);
#endif
	if (this->vtable.display_status)
		this->vtable.display_status(this,_("Ready"));
	this->auto_net_state_mon=this->sip_conf.auto_net_state_mon;
	serphone_core_set_state(LinphoneGlobalOn,"Ready");
}

void ServiceCore::serphone_core_uninit()
{
	serphone_core_free_hooks();
	while(calls)
	{
		SerPhoneCall *the_call = (SerPhoneCall *)calls->data;
		serphone_core_terminate_call(the_call);
		//serphone_core_iterate();
	}

	if (this->friends)
		ms_list_for_each(this->friends,(void (*)(void **))serphone_friend_close_subscriptions);
	serphone_core_set_state(LinphoneGlobalShutdown,"Shutting down");
#ifdef VIDEO_ENABLED
	/*if (lc->previewstream!=NULL){
		video_preview_stop(lc->previewstream);
		lc->previewstream=NULL;
	}*/
#endif
	/* save all config */
	net_config_uninit();
	rtp_config_uninit();
	if ( m_ringplay_flag ) ring_stop();
	sound_config_uninit();
//	video_config_uninit(lc);
	codecs_config_uninit();
	ui_config_uninit();


	sip_setup_unregister_all();

	ms_list_for_each(this->call_logs,(void (*)(void**))serphone_call_log_destroy);
	this->call_logs=ms_list_free(this->call_logs);

	sip_config_uninit();
	if (lp_config_needs_commit(config))
			lp_config_sync(config);
	lp_config_destroy(this->config);
	this->config = NULL; /* Mark the config as NULL to block further calls */

	serphone_core_free_payload_types();
	PrintConsole("Release Media \n");
#ifdef VIDEO_ENABLED
	ECMedia_uninit_video();
#endif
#ifdef WIN32
    ECMedia_uninit_audio();
#endif
	serphone_core_set_state(LinphoneGlobalOff,"Off");
#ifdef TUNNEL_ENABLED
	if (this->tunnel) serphone_tunnel_destroy(this->tunnel);
#endif

	//PrintConsole("Release Trace... \n");
 //   cloopenwebrtc::Trace::ReturnTrace();
	//PrintConsole("Release Finish \n");
}

void ServiceCore::serphone_core_set_default_proxy_index(int index)
{
	if (index<0) serphone_core_set_default_proxy(NULL);
	else serphone_core_set_default_proxy((SerphoneProxyConfig *)ms_list_nth_data(sip_conf.proxies,index));
}

/**
 * Sets the default proxy.
 *
 * This default proxy must be part of the list of already entered LinphoneProxyConfig.
 * Toggling it as default will make LinphoneCore use the identity associated with
 * the proxy configuration in all incoming and outgoing calls.
**/
void ServiceCore::serphone_core_set_default_proxy(SerphoneProxyConfig *l_config)
{
	/* check if this proxy is in our list */
	if (l_config!=NULL){
		if (ms_list_find(sip_conf.proxies,l_config)==NULL){
			PrintConsole("Bad proxy address: it is not in the list !\n");
			default_proxy=NULL;
			return ;
		}
	}
	default_proxy=l_config;
	if (serphone_core_ready())
		lp_config_set_int(config,"sip","default_proxy",serphone_core_get_default_proxy(NULL));
}

/**
 * Specify whether the tls server certificate must be verified when connecting to a SIP/TLS server.
**/
void ServiceCore::serphone_core_verify_server_certificates(bool_t yesno)
{
	sal_verify_server_certificates(sal,yesno);
}

void ServiceCore::ui_config_read()
{
	SerphoneFriend *lf;
	int i;
	for (i=0;(lf=serphone_friend_new_from_config_file(this,i))!=NULL;i++){
		serphone_core_add_friend(lf);
	}
}

void ServiceCore::misc_config_read ()
{
	LpConfig *lp_config=config;
    max_call_logs=lp_config_get_int(lp_config,"misc","history_max_size",15);
    max_calls=lp_config_get_int(lp_config,"misc","max_calls",NB_MAX_CALLS);
}

void ServiceCore::serphone_core_run_hooks()
{
	ms_list_for_each(this->hooks,(void (*)(void**))hook_invoke);
}

void ServiceCore::serphone_core_free_hooks()
{
	ms_list_for_each(hooks,(void (*)(void**))ms_free);
	ms_list_free(hooks);
	hooks=NULL;
}

void ServiceCore::rtp_config_uninit()
{
	rtp_config_t *config=&this->rtp_conf;
	lp_config_set_int(this->config,"rtp","audio_rtp_port",config->audio_rtp_port);
	lp_config_set_int(this->config,"rtp","video_rtp_port",config->video_rtp_port);
	lp_config_set_int(this->config,"rtp","audio_jitt_comp",config->audio_jitt_comp);
	lp_config_set_int(this->config,"rtp","video_jitt_comp",config->video_jitt_comp);
	lp_config_set_int(this->config,"rtp","nortp_timeout",config->nortp_timeout);
}

void ServiceCore::ui_config_uninit()
{
	if (this->friends){
		ms_list_for_each(this->friends,(void (*)(void **))serphone_friend_destroy);
		ms_list_free(this->friends);
		this->friends=NULL;
	}
}

void ServiceCore::sip_config_uninit()
{
	MSList *elem;
	int i;
	sip_config_t *config=&this->sip_conf;

	lp_config_set_int(this->config,"sip","guess_hostname",config->guess_hostname);
	lp_config_set_string(this->config,"sip","contact",config->contact);
	lp_config_set_int(this->config,"sip","inc_timeout",config->inc_timeout);
	lp_config_set_int(this->config,"sip","use_info",config->use_info);
	lp_config_set_int(this->config,"sip","use_rfc2833",config->use_rfc2833);
	lp_config_set_int(this->config,"sip","use_ipv6",config->ipv6_enabled);
	lp_config_set_int(this->config,"sip","register_only_when_network_is_up",config->register_only_when_network_is_up);


	for(elem=config->proxies,i=0;elem!=NULL;elem=ms_list_next(elem),i++){
		SerphoneProxyConfig *cfg=(SerphoneProxyConfig*)(elem->data);
        if (traceFile) {
            time_t temp = time(NULL);
            std::string strTime = timetodate(temp);
            fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
            fwrite(" Something goes wrong Unregister called6\n", strlen(" Something goes wrong Unregister called6\n"), 1, traceFile);
            fflush(traceFile);
        }
		cfg->serphone_proxy_config_edit();	/* to unregister */
	}

/*	for (i=0;i<20;i++){
		sal_iterate(this->sal);
#ifndef WIN32
		usleep(100000);
#else
		Sleep(100);
#endif
	}*/
	ms_list_for_each(this->chatrooms,(void (*)(void**))serphone_chat_room_destroy);
	this->chatrooms=ms_list_free(this->chatrooms);

	ms_list_for_each(config->proxies,(void (*)(void**)) serphone_proxy_config_destroy);
	ms_list_free(config->proxies);
	config->proxies=NULL;

	serphone_proxy_config_write_to_config_file(this->config,NULL,i);	/*mark the end */

	ms_list_for_each(this->auth_info,(void (*)(void**))serphone_auth_info_destroy);
	ms_list_free(this->auth_info);
	this->auth_info=NULL;

	sal_uninit(this->sal);
	this->sal=NULL;

	if (this->sip_conf.guessed_contact)
    {
        ms_free((void **)&this->sip_conf.guessed_contact);
    }
	if (config->contact)
    {
        ms_free((void **)&config->contact);
    }

}

void ServiceCore::serphone_core_preempt_sound_resources()
{
	SerPhoneCall *lp_current_call;
/*  commented by zdm
	if (linphone_core_is_in_conference(lc)){
		linphone_core_leave_conference(lc);
		return;
	}
*/
	lp_current_call=serphone_core_get_current_call();
	if(lp_current_call != NULL){
		PrintConsole("Pausing automatically the current call.\n");
		serphone_core_pause_call(lp_current_call);
	}
}

/**
 * Main loop function. It is crucial that your application call it periodically.
 *
 * @ingroup initializing
 * serphone_core_iterate() performs various backgrounds tasks:
 * - receiving of SIP messages
 * - handles timers and timeout
 * - performs registration to proxies
 * - authentication retries
 * The application MUST call this function periodically, in its main loop.
 * Be careful that this function must be called from the same thread as
 * other libserphone methods. If it is not the case make sure all liblinphone calls are
 * serialized with a mutex.
**/
void ServiceCore::serphone_core_iterate()
{
    cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	MSList *calls;
	SerPhoneCall *call;
	time_t curtime=time(NULL);
	int elapsed;
	bool_t one_second_elapsed=FALSE;

	if (curtime-this->prevtime>=1){
		this->prevtime=curtime;
		one_second_elapsed=TRUE;
	}
/*
	if (this->preview_finished){
		lc->preview_finished=0;
		ring_stop();
		lc->ringstream=NULL;
		lc_callback_obj_invoke(&lc->preview_finished_cb,lc);
	}
*/
	if (m_ringplay_flag && ringstream_autorelease && dmfs_playing_start_time!=0
	    && (curtime-dmfs_playing_start_time)>5){
		ring_stop();
		m_ringplay_flag=FALSE;
		dmfs_playing_start_time=0;
	}
	m_criticalSection->Leave();
	sal_iterate(sal);
	m_criticalSection->Enter();
    if (auto_net_state_mon) monitor_network_state(curtime);

	proxy_update();

	//we have to iterate for each call
	calls= this->calls;
	while(calls!= NULL){
		call = (SerPhoneCall *)calls->data;
		 /* get immediately a reference to next one in case the one
		 we are going to examine is destroy and removed during
		 linphone_core_start_invite() */
		calls=calls->next;
        serphone_call_background_tasks(call,one_second_elapsed);
		if (call->state==LinphoneCallOutgoingInit && (curtime-call->start_time>=300)){
            if (call->ice_session != NULL) {
				PrintConsole("ICE candidates gathering from [%s] has not finished yet, proceed with the call without ICE anyway.\n"
                           ,serphone_core_get_stun_server());
				serphone_call_delete_ice_session(call);
				serphone_call_stop_media_streams_for_ice_gathering(call);
			}
			/*start the call even if the OPTIONS reply did not arrive*/
			serphone_core_start_invite(call,NULL);
		}
		if (call->state==LinphoneCallIncomingReceived){
			elapsed= int(time(NULL)-call->start_time);
//			PrintConsole("incoming call ringing for %i seconds\n",elapsed);
			if (elapsed>sip_conf.inc_timeout){
				call->log->status=LinphoneCallMissed;
				serphone_core_terminate_call(call);
			}
		}
        //conference mode, check request timeout
        serphone_check_video_conference_request_failed();
	}
#ifdef WIN32
        serphone_check_pre_after_ring_timeout();
#endif
/*
	if (linphone_core_video_preview_enabled(lc)){
		if (lc->previewstream==NULL && lc->calls==NULL)
			toggle_video_preview(lc,TRUE);
#ifdef VIDEO_ENABLED
		if (lc->previewstream) video_stream_iterate(lc->previewstream);
#endif
	}else{
		if (lc->previewstream!=NULL)
			toggle_video_preview(lc,FALSE);
	}
*/
	serphone_core_run_hooks();
//	linphone_core_do_plugin_tasks(lc);

	if (initial_subscribes_sent==FALSE && netup_time!=0 &&
	    (curtime-netup_time)>3){
		serphone_core_send_initial_subscribes();
		initial_subscribes_sent=TRUE;
	}

	if (config && one_second_elapsed && lp_config_needs_commit(config)){
		lp_config_sync(config);
	}
	//m_criticalSection->Leave();
}

void ServiceCore::proxy_update()
{
	MSList *elem,*next;
	ms_list_for_each(sip_conf.proxies,(void (*)(void**))serphone_proxy_config_update);
	for(elem=sip_conf.deleted_proxies;elem!=NULL;elem=next){
		SerphoneProxyConfig* cfg = (SerphoneProxyConfig*)elem->data;
		next=elem->next;
		if (ms_time(NULL) - cfg->deletion_date > 5) {
			sip_conf.deleted_proxies =ms_list_remove_link(sip_conf.deleted_proxies,elem);
			PrintConsole("clearing proxy config for [%s]\n",cfg->serphone_proxy_config_get_addr());
			serphone_proxy_config_destroy(&cfg);
		}
	}
}

void ServiceCore::monitor_network_state(time_t curtime)
{
	static time_t last_check=0;
	static bool_t last_status=FALSE;
	char result[SERPHONE_IPADDR_SIZE];
	bool_t new_status=last_status;

	/* only do the network up checking every five seconds */
	if (last_check==0 || (curtime-last_check)>=5){
		serphone_core_get_local_ip_for(sip_conf.ipv6_enabled ? AF_INET6 : AF_INET,NULL,result);
		if (strcmp(result,"::1")!=0 && strcmp(result,"127.0.0.1")!=0){
			new_status=TRUE;
		}else new_status=FALSE;
		last_check=curtime;
		if (new_status!=last_status) {
			if (new_status){
				PrintConsole("New local ip address is %s\n",result);
			}
			set_network_reachable(new_status, curtime);
			last_status=new_status;
		}
	}
}

void ServiceCore::set_network_reachable(bool_t isReachable, time_t curtime)
{
	PrintConsole("Network state is now [%s]\n",isReachable?"UP":"DOWN");
	// second get the list of available proxies
	const MSList *elem=serphone_core_get_proxy_config_list();
	for(;elem!=NULL;elem=elem->next){
		SerphoneProxyConfig *cfg=(SerphoneProxyConfig*)elem->data;
		if (cfg->serphone_proxy_config_register_enabled() ) {
			if (!isReachable) {
				cfg->serphone_proxy_config_set_state( LinphoneRegistrationNone,"Registration impossible (network down)");
			}else{
				cfg->commit=TRUE;
			}
		}
	}
	this->netup_time=curtime;
	this->network_reachable=isReachable;
	if(!isReachable) {
		sal_unlisten_ports (sal);
	} else {
		apply_transports();
	}
}

MSList *ServiceCore::make_codec_list(const MSList *codecs, int bandwidth_limit,int* max_sample_rate)
{
	MSList *l=NULL;
	const MSList *it;
	if (max_sample_rate) *max_sample_rate=0;
	for(it=codecs;it!=NULL;it=it->next){
		PayloadType *pt=(PayloadType*)it->data;
		if (pt->flags & PAYLOAD_TYPE_ENABLED){
			if (bandwidth_limit>0 && !serphone_core_is_payload_type_usable_for_bandwidth(pt,bandwidth_limit)){
				PrintConsole("Codec %s/%i eliminated because of audio bandwidth constraint.\n",pt->mime_type,pt->clock_rate);
				continue;
			}
			if (serphone_core_check_payload_type_usability(pt)){
				l=ms_list_append(l,payload_type_clone(pt));
				if (max_sample_rate && payload_type_get_rate(pt)>*max_sample_rate) *max_sample_rate=payload_type_get_rate(pt);
			}
		}
	}
	return l;
}

bool_t ServiceCore::serphone_core_is_payload_type_usable_for_bandwidth(PayloadType *pt,  int bandwidth_limit)
{
	double codec_band;
	bool_t ret=FALSE;

	switch (pt->type){
		case PAYLOAD_AUDIO_CONTINUOUS:
		case PAYLOAD_AUDIO_PACKETIZED:
			codec_band=(int)get_audio_payload_bandwidth(pt);
			ret=bandwidth_is_greater(bandwidth_limit*1000,(int)codec_band);
			/*hack to avoid using uwb codecs when having low bitrate and video*/
			if (bandwidth_is_greater(199,bandwidth_limit)){
				if (serphone_core_video_enabled() && pt->clock_rate>16000){
					ret=FALSE;
				}
			}
			//ms_message("Payload %s: %g",pt->mime_type,codec_band);
			break;
		case PAYLOAD_VIDEO:
			if (bandwidth_limit!=0) {/* infinite (-1) or strictly positive*/
				ret=TRUE;
			}
			else ret=FALSE;
			break;
	}
	return ret;
}

/* return TRUE if codec can be used with bandwidth, FALSE else*/
bool_t ServiceCore::serphone_core_check_payload_type_usability(PayloadType *pt)
{
	double codec_band;
	int allowed_bw,video_bw;
	bool_t ret=FALSE;

	serphone_core_update_allocated_audio_bandwidth();
	allowed_bw=get_min_bandwidth(serphone_core_get_download_bandwidth(),
					serphone_core_get_upload_bandwidth());
	if (allowed_bw==0) {
		allowed_bw=-1;
		video_bw=1500; /*around 1.5 Mbit/s*/
	}else
		video_bw=get_video_bandwidth(allowed_bw,this->audio_bw);

	switch (pt->type){
		case PAYLOAD_AUDIO_CONTINUOUS:
		case PAYLOAD_AUDIO_PACKETIZED:
			codec_band=get_audio_payload_bandwidth(pt);
			ret=(int)bandwidth_is_greater(allowed_bw*1000,(int)codec_band);
			/*hack to avoid using uwb codecs when having low bitrate and video*/
			if (bandwidth_is_greater(199,allowed_bw)){
				if (serphone_core_video_enabled() && pt->clock_rate>16000){
					ret=FALSE;
				}
			}
			//ms_message("Payload %s: %g",pt->mime_type,codec_band);
			break;
		case PAYLOAD_VIDEO:
			if (video_bw>0){
				pt->normal_bitrate=video_bw*1000;
				ret=TRUE;
			}
			else ret=FALSE;
			break;
	}
	return ret;
}

double ServiceCore::get_audio_payload_bandwidth(const PayloadType *pt)
{
	double npacket=50;
	double packet_size;
	int bitrate;
	bitrate=get_codec_bitrate(pt);
	packet_size= (((double)bitrate)/(50*8))+UDP_HDR_SZ+RTP_HDR_SZ+IP4_HDR_SZ;
	return packet_size*8.0*npacket;
}

/*this function makes a special case for speex/8000.
This codec is variable bitrate. The 8kbit/s mode is interesting when having a low upload bandwidth, but its quality
is not very good. We 'd better use its 15kbt/s mode when we have enough bandwidth*/
int ServiceCore::get_codec_bitrate(const PayloadType *pt)
{
	int upload_bw=serphone_core_get_upload_bandwidth();
	if (bandwidth_is_greater(upload_bw,129) || (bandwidth_is_greater(upload_bw,33) && !serphone_core_video_enabled()) ) {
		if (strcmp(pt->mime_type,"speex")==0 && pt->clock_rate==8000){
			return 15000;
		}
	}
	return pt->normal_bitrate;
}

bool_t ServiceCore::generate_b64_crypto_key(int key_length, char* key_out ,const char *user_key)
{
	Base64encode(key_out ,user_key,key_length);
	return TRUE;
}

bool_t ServiceCore::serphone_core_video_enabled()
{
	return (this->video_conf.display || this->video_conf.capture);
}

void ServiceCore::serphone_core_update_allocated_audio_bandwidth()
{
	const MSList *elem;
	PayloadType *max=NULL;
	for(elem=serphone_core_get_audio_codecs();elem!=NULL;elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
		if (payload_type_enabled(pt)){
			int pt_bitrate=get_codec_bitrate(pt);
			if (max==NULL) max=pt;
			else if (max->normal_bitrate<pt_bitrate){
				max=pt;
			}
		}
	}
	if (max) {
		this->audio_bw=(int)(get_audio_payload_bandwidth(max)/1000.0);
	}
}

/**
 * Returns the list of available audio codecs.
 *
 * This list is unmodifiable. The ->data field of the MSList points a PayloadType
 * structure holding the codec information.
 * It is possible to make copy of the list with ms_list_copy() in order to modify it
 * (such as the order of codecs).
**/
const MSList *ServiceCore::serphone_core_get_audio_codecs()
{
	return this->codecs_conf.audio_codecs;
}

#define RANK_END 10000
static const char *codec_pref_order[]={
    "red",
	"OPUS",
    "SILK",
	"speex",
	"iLBC",
	"G729",
	"amr",
	"gsm",
    "pcmu",
	"pcma",
	"VP8",
	"H264",    
	"H264-SVC",
	"MP4V-ES",
	"H263-1998",
	NULL,
};

static int find_codec_rank(const char *mime){
	int i;
	for(i=0;codec_pref_order[i]!=NULL;++i){
		if (strcasecmp(codec_pref_order[i],mime)==0)
			return i;
	}
	return RANK_END;
}

static int codec_compare(const PayloadType *a, const PayloadType *b){
	int ra,rb;
	ra=find_codec_rank(a->mime_type);
	rb=find_codec_rank(b->mime_type);
	if (ra>rb) return 1;
	if (ra<rb) return -1;
	return 0;
}

static MSList *add_missing_codecs(SalStreamType mtype, MSList *l){
	int i;
	for(i=0;i<127;++i){
		PayloadType *pt=rtp_profile_get_payload(&av_profile,i);
		if (pt){
			if (mtype==SalVideo && pt->type!=PAYLOAD_VIDEO)
				pt=NULL;
			else if (mtype==SalAudio && (pt->type!=PAYLOAD_AUDIO_PACKETIZED
			    && pt->type!=PAYLOAD_AUDIO_CONTINUOUS)){
				pt=NULL;
			}
			if (pt){ // && ms_filter_codec_supported(pt->mime_type)){  在这里不做这个判??不会改这个代码，deleted by zdm
//			if (pt && ms_filter_codec_supported(pt->mime_type)){
				if (ms_list_find(l,pt)==NULL){
					/*unranked codecs are disabled by default*/
					if (find_codec_rank(pt->mime_type)!=RANK_END){
						payload_type_set_flag(pt,PAYLOAD_TYPE_ENABLED);
					}
					PrintConsole("Adding new codec %s/%i with fmtp %s\n",
					    pt->mime_type,pt->clock_rate,pt->recv_fmtp ? pt->recv_fmtp : "");
					l=ms_list_insert_sorted(l,pt,(int (*)(const void *, const void *))codec_compare);
				}
			}
		}
	}
	return l;
}

static MSList *codec_append_if_new(MSList *l, PayloadType *pt){
	MSList *elem;
	for (elem=l;elem!=NULL;elem=elem->next){
		PayloadType *ept=(PayloadType*)elem->data;
		if (pt==ept)
			return l;
	}
	l=ms_list_append(l,pt);
	return l;
}

void ServiceCore::serphone_core_enable_payload_type(const char *mimeType, int freq, bool bEnable)
{
    const MSList *it;
	for(it=this->codecs_conf.audio_codecs;it!=NULL;it=it->next)
    {
        PayloadType *pt=(PayloadType*)(it->data);
        if ((strcmp(pt->mime_type,mimeType)==0) && (pt->clock_rate == freq))
        {
            PrintConsole("serphone_core_enable_payload_type mime_type=%s pt->flags=%d bEnable=%d\r\n",mimeType, pt->flags,bEnable);
            if(bEnable)
                pt->flags |= PAYLOAD_TYPE_ENABLED;
            else
                pt->flags &= ~PAYLOAD_TYPE_ENABLED;
            break;
        }
    }
    for(it=this->codecs_conf.video_codecs;it!=NULL;it=it->next)
    {
        PayloadType *pt=(PayloadType*)(it->data);
        if (strcmp(pt->mime_type,mimeType)==0)
        {
            PrintConsole("serphone_core_enable_payload_type mime_type=%s pt->flags=%d bEnable=%d\r\n",mimeType, pt->flags, bEnable);
            if(bEnable)
                pt->flags |= PAYLOAD_TYPE_ENABLED;
            else
                pt->flags &= ~PAYLOAD_TYPE_ENABLED;
            break;
        }
    }
}

bool ServiceCore::serphone_core_is_payload_type_enable(const char *mimeType, int freq)
{
    const MSList *it;
	for(it=this->codecs_conf.audio_codecs;it!=NULL;it=it->next)
    {
        PayloadType *pt=(PayloadType*)(it->data);
        if ((strcmp(pt->mime_type,mimeType)==0) && (pt->clock_rate == freq))
        {
            PrintConsole("serphone_core_is_payload_type_enable mime_type=%s flags=%d\r\n", mimeType,pt->flags);
            return payload_type_enabled(pt);
        }
    }
    for(it=this->codecs_conf.video_codecs;it!=NULL;it=it->next)
    {
        PayloadType *pt=(PayloadType*)(it->data);
        if (strcmp(pt->mime_type,mimeType)==0)
        {
            PrintConsole("serphone_core_is_payload_type_enable mime_type=%s flags=%d\r\n", mimeType,pt->flags);
            return payload_type_enabled(pt);
        }
    }
    return false;
}

void ServiceCore::serphone_core_enable_srtp(bool tls, bool srtp, bool userMode, int cryptType, const char *pkey)
{
    PrintConsole("[DEBUG] serphone_core_enable_srtp userMode = %d, pkey = %s\n",userMode,pkey?pkey:"NULL");
   // tls_enable = tls;
   // srtp_enable = srtp;
   // if (srtp_enable) {
   //     if (pkey && 46 <= strlen(pkey)) {
   //         user_mode = userMode;
   //         userKeySetted = true;
   //         memcpy(m_SrtpKey, pkey, 46);
			//m_SrtpKey[46] ='\0';
   //     }
   //     encryptType = (ccp_srtp_crypto_suite_t)cryptType;
   // }

	tls_enable = tls;
	srtp_enable = srtp;
	user_mode = userMode;
	encryptType = (cloopenwebrtc::ccp_srtp_crypto_suite_t)cryptType;
	memcpy(m_SrtpKey, pkey, strlen(pkey));
	m_SrtpKey[strlen(pkey)] = '\0';

}

void ServiceCore::serphone_core_enable_srtp(bool tls, bool srtp, int cryptType, const char *pkey)
{
    PrintConsole("[DEBUG] serphone_core_enable_srtp pkey = %s, kenLen = %d\n",pkey,strlen(pkey));
    tls_enable = tls;
    srtp_enable = srtp;
    if (srtp_enable) {
        if (pkey && 46 <= strlen(pkey)) {
            userKeySetted = true;
            memcpy(m_SrtpKey, pkey, 46);
			m_SrtpKey[46] ='\0';
        }
        encryptType = (cloopenwebrtc::ccp_srtp_crypto_suite_t)cryptType;
    }
}

void ServiceCore::codecs_config_read()
{
	int i;
	PayloadType *pt;
	MSList *audio_codecs=NULL;
	MSList *video_codecs=NULL;
	for (i=0;get_codec(this->config,"audio_codec",i,&pt);i++){
		if (pt){
/*			if (!ms_filter_codec_supported(pt->mime_type)){   注释掉，先不改，deleted by zdm
				PrintConsole("Codec %s is not supported by mediastreamer2, removed.\n",pt->mime_type);
			}else */
				audio_codecs=codec_append_if_new(audio_codecs,pt);
		}
	}
	audio_codecs=add_missing_codecs(SalAudio,audio_codecs);
	for (i=0;get_codec(this->config,"video_codec",i,&pt);i++){
		if (pt){
/*			if (!ms_filter_codec_supported(pt->mime_type)){
				PrintConsole("Codec %s is not supported by mediastreamer2, removed.\n",pt->mime_type);
			}else */
				video_codecs=codec_append_if_new(video_codecs,pt);
		}
	}
	video_codecs=add_missing_codecs(SalVideo,video_codecs);
	serphone_core_set_audio_codecs(audio_codecs);
	serphone_core_set_video_codecs(video_codecs);
	serphone_core_update_allocated_audio_bandwidth();
}

void ServiceCore::video_config_read(){
//#ifdef VIDEO_ENABLED
//	int capture, display, self_view;
//#endif
	const char *str;
#ifdef VIDEO_ENABLED
	SerphoneVideoPolicy vpol;
	memset(&vpol, 0, sizeof(SerphoneVideoPolicy));
#endif
//	build_video_devices_table(lc);
//
//	str=lp_config_get_string(config,"video","device",NULL);
//	if (str && str[0]==0) str=NULL;
//	serphone_core_set_video_device(lc,str);
//
//	linphone_core_set_preferred_video_size_by_name(lc,
//                                                   lp_config_get_string(lc->config,"video","size","cif"));

#ifdef VIDEO_ENABLED
//	capture=lp_config_get_int(config,"video","capture",1);
//	display=lp_config_get_int(config,"video","display",1);
//	self_view=lp_config_get_int(config,"video","self_view",1);
	vpol.automatically_initiate=lp_config_get_int(config,"video","automatically_initiate",1);
	vpol.automatically_accept=lp_config_get_int(config,"video","automatically_accept",1);
	video_conf.displaytype=lp_config_get_string(config,"video","displaytype",NULL);
	if(video_conf.displaytype)
		PrintConsole("we are using a specific display:%s\n",video_conf.displaytype);

//	serphone_core_enable_video(lc,capture,display);
//	serphone_core_enable_video_preview(lc,lp_config_get_int(lc->config,"video","show_local",0));
//	serphone_core_enable_self_view(lc,self_view);
	serphone_core_set_video_policy(&vpol);
#endif
}

void ServiceCore::serphone_core_set_video_policy(const SerphoneVideoPolicy *policy){
	this->video_policy=*policy;
	if (serphone_core_ready()){
		lp_config_set_int(config,"video","automatically_initiate",policy->automatically_initiate);
		lp_config_set_int(config,"video","automatically_accept",policy->automatically_accept);
	}
}


/**
 * Sets the list of audio codecs.
 *
 * @ingroup media_parameters
 * The list is taken by the ServiceCore thus the application should not free it.
 * This list is made of struct PayloadType describing the codec parameters.
**/
int ServiceCore::serphone_core_set_audio_codecs(MSList *codecs)
{
	if (this->codecs_conf.audio_codecs!=NULL) ms_list_free(this->codecs_conf.audio_codecs);
	this->codecs_conf.audio_codecs=codecs;
	return 0;
}

/**
 * Sets the list of video codecs.
 *
 * @ingroup media_parameters
 * The list is taken by the ServiceCore thus the application should not free it.
 * This list is made of struct PayloadType describing the codec parameters.
**/
int ServiceCore::serphone_core_set_video_codecs(MSList *codecs)
{
	if (this->codecs_conf.video_codecs!=NULL) ms_list_free(this->codecs_conf.video_codecs);
	this->codecs_conf.video_codecs=codecs;
// #define OPENH264_TEST
// #ifdef OPENH264_TEST
// 	PayloadType pl = payload_type_h264_svc;
// 	ms_list_append(this->codecs_conf.video_codecs, &pl);
// #endif
	return 0;
}

bool_t ServiceCore::get_codec(LpConfig *config, const char* type, int index, PayloadType **ret)
{
	char codeckey[50];
	const char *mime,*fmtp;
	int rate,enabled;
	PayloadType *pt;

	*ret=NULL;
	snprintf(codeckey,50,"%s_%i",type,index);
	mime=lp_config_get_string(config,codeckey,"mime",NULL);
	if (mime==NULL || strlen(mime)==0 ) return FALSE;

	rate=lp_config_get_int(config,codeckey,"rate",8000);
	fmtp=lp_config_get_string(config,codeckey,"recv_fmtp",NULL);
	enabled=lp_config_get_int(config,codeckey,"enabled",1);
	pt=find_payload(&av_profile,mime,rate,fmtp);
	if (pt && enabled ) pt->flags|=PAYLOAD_TYPE_ENABLED;
	//ms_message("Found codec %s/%i",pt->mime_type,pt->clock_rate);
	if (pt==NULL) PrintConsole("Ignoring codec config %s/%i with fmtp=%s because unsupported\n",
	    		mime,rate,fmtp ? fmtp : "");
	*ret=pt;
	return TRUE;
}

bool_t ServiceCore::serphone_core_adaptive_rate_control_enabled()
{
	return lp_config_get_int(config,"net","adaptive_rate_control",TRUE);
}

/**
 * Set audio packetization time linphone will send (in absence of requirement from peer)
 * A value of 0 stands for the current codec default packetization time.
 *
**/
int ServiceCore::serphone_core_get_upload_ptime()
{
	return lp_config_get_int(config,"rtp","upload_ptime",0);
}

void ServiceCore::codecs_config_uninit()
{
	PayloadType *pt;
	codecs_config_t *config=&codecs_conf;
	MSList *node;
	char key[50];
	int index;
	index=0;
	for(node=config->audio_codecs;node!=NULL;node=ms_list_next(node)){
		pt=(PayloadType*)(node->data);
		sprintf(key,"audio_codec_%i",index);
		lp_config_set_string(this->config,key,"mime",pt->mime_type);
		lp_config_set_int(this->config,key,"rate",pt->clock_rate);
		lp_config_set_int(this->config,key,"enabled",serphone_core_payload_type_enabled(pt));
		index++;
	}
	sprintf(key,"audio_codec_%i",index);
	lp_config_clean_section (this->config,key);

	index=0;
	for(node=config->video_codecs;node!=NULL;node=ms_list_next(node)){
		pt=(PayloadType*)(node->data);
		sprintf(key,"video_codec_%i",index);
		lp_config_set_string(this->config,key,"mime",pt->mime_type);
		lp_config_set_int(this->config,key,"rate",pt->clock_rate);
		lp_config_set_int(this->config,key,"enabled",serphone_core_payload_type_enabled(pt));
		lp_config_set_string(this->config,key,"recv_fmtp",pt->recv_fmtp);
		index++;
	}
	sprintf(key,"video_codec_%i",index);
	lp_config_clean_section (this->config,key);

	ms_list_free(this->codecs_conf.audio_codecs);
	ms_list_free(this->codecs_conf.video_codecs);
}

bool_t ServiceCore::serphone_core_payload_type_enabled(const PayloadType *pt)
{
	if (ms_list_find(codecs_conf.audio_codecs, (PayloadType*) pt) || ms_list_find(codecs_conf.video_codecs, (PayloadType*)pt)){
		return payload_type_enabled(pt);
	}
	PrintConsole("Getting enablement status of codec not in audio or video list of PayloadType !\n");
	return FALSE;
}

#define LOCAL_RING "ring.wav"
#define REMOTE_RING "ringback.wav"
#ifdef WIN32
#define PRE_RING "prering.wav"
#define AFTER_RING "afterring.wav"
#endif
#define HOLD_MUSIC "hold.wav"

void ServiceCore::sound_config_read()
{
	const char *tmpbuf;
	tmpbuf= LOCAL_RING;
	tmpbuf=lp_config_get_string(this->config,"sound","local_ring",tmpbuf);
	if (ccp_ortp_file_exist(tmpbuf)==-1) {
		PrintConsole("%s does not exist\n",tmpbuf);
		tmpbuf= LOCAL_RING;
	}
	if (strstr(tmpbuf,".wav")==NULL){
		/* it currently uses old sound files, so replace them */
		tmpbuf= LOCAL_RING;
	}
	serphone_core_set_ring(tmpbuf);

	tmpbuf= REMOTE_RING;
	tmpbuf=lp_config_get_string(this->config,"sound","remote_ring",tmpbuf);
	if (ccp_ortp_file_exist(tmpbuf)==-1){
		tmpbuf=REMOTE_RING;
	}
	if (strstr(tmpbuf,".wav")==NULL){
		/* it currently uses old sound files, so replace them */
		tmpbuf= REMOTE_RING;
	}
	serphone_core_set_ringback(tmpbuf);

#ifdef WIN32
    tmpbuf= PRE_RING;
    tmpbuf=lp_config_get_string(this->config,"sound","pre_ring",tmpbuf);
    if (ccp_ortp_file_exist(tmpbuf)==-1){
        tmpbuf=PRE_RING;
    }
    if (strstr(tmpbuf,".wav")==NULL){
        /* it currently uses old sound files, so replace them */
        tmpbuf= PRE_RING;
    }
    serphone_core_set_prering(tmpbuf);

    tmpbuf= AFTER_RING;
    tmpbuf=lp_config_get_string(this->config,"sound","after_ring",tmpbuf);
    if (ccp_ortp_file_exist(tmpbuf)==-1){
        tmpbuf=AFTER_RING;
    }
    if (strstr(tmpbuf,".wav")==NULL){
        /* it currently uses old sound files, so replace them */
        tmpbuf= AFTER_RING;
    }
    serphone_core_set_afterring(tmpbuf);
#endif


	lp_config_get_string(this->config,"sound","hold_music",HOLD_MUSIC);
}

void ServiceCore::serphone_core_set_ring(const char *path)
{
	if (this->sound_conf.local_ring!=0)
    {
		ms_free((void **)&this->sound_conf.local_ring);
	}
	if (path)
		this->sound_conf.local_ring=ms_strdup(path);
	if ( serphone_core_ready() && this->sound_conf.local_ring)
		lp_config_set_string(this->config,"sound","local_ring",this->sound_conf.local_ring);
}

void ServiceCore::serphone_core_set_ringback( const char *path)
{
	if (this->sound_conf.remote_ring!=0)
    {
		ms_free((void **)&this->sound_conf.remote_ring);
	}
	this->sound_conf.remote_ring=ms_strdup(path);
}

void ServiceCore::serphone_core_set_prering(const char *path)
{
#ifdef WIN32
    if (this->sound_conf.pre_ring!=0)
    {
        ms_free((void **)&this->sound_conf.pre_ring);
    }
    if (path)
        this->sound_conf.pre_ring=ms_strdup(path);
    if ( serphone_core_ready() && this->sound_conf.pre_ring)
        lp_config_set_string(this->config,"sound","pre_ring",this->sound_conf.pre_ring);
    this->sound_conf.pre_ring_starttime = 0;
#endif
}
void ServiceCore::serphone_core_set_afterring( const char *path)
{
#ifdef WIN32
    if (this->sound_conf.after_ring!=0)
    {
        ms_free((void **)&this->sound_conf.after_ring);
    }
    if (path)
        this->sound_conf.after_ring=ms_strdup(path);
    if ( serphone_core_ready() && this->sound_conf.after_ring)
        lp_config_set_string(this->config,"sound","after_ring",this->sound_conf.after_ring);
    this->sound_conf.after_ring_starttime = 0;
#endif
}


int ServiceCore::serphone_prering_start()
{
#ifdef WIN32
	PrintConsole("ServiceCore::serphone_prering_start()\n");
    if (sound_conf.pre_ring) {
        ring_start(sound_conf.pre_ring, 2000, 1);
        sound_conf.pre_ring_starttime = time(NULL);
    }
#endif
    return -1;
}

int ServiceCore::serphone_afterring_start()
{
#ifdef WIN32
	PrintConsole("ServiceCore::serphone_afterring_start()\n");
    if (sound_conf.after_ring) {
        ring_start(sound_conf.after_ring, 2000, 2);
        sound_conf.after_ring_starttime = time(NULL);
    }
#endif
    return -1;
}

void ServiceCore::serphone_pre_after_ring_stop(bool ispreRing)
{
#ifdef WIN32
//PrintConsole("ServiceCore::serphone_pre_after_ring_stop(),isPreRing=%d\n",ispreRing);
    if (ispreRing) {
        if (sound_conf.pre_ring_starttime <= 0) {
            return;
        }
        sound_conf.pre_ring_starttime=0;
        ring_stop(1);
    }
	else
    {
        if (sound_conf.after_ring_starttime <= 0) {
            return;
        }
        sound_conf.after_ring_starttime=0;
        ring_stop(2);
    }

#endif
}

void ServiceCore::serphone_check_pre_after_ring_timeout()
{
#ifdef WIN32
    if ((this->sound_conf.pre_ring_starttime > 0 && (time(NULL) - this->sound_conf.pre_ring_starttime >= 10)) || (this->sound_conf.after_ring_starttime > 0 && (time(NULL) - this->sound_conf.after_ring_starttime >= 4))) {
		 PrintConsole("serphone_check_pre_after_ring_timeout(), haha\n");
		serphone_pre_after_ring_stop(true);
        serphone_pre_after_ring_stop(false);

    }
#endif
}

void ServiceCore::onStopPlayPreRing()
{
#ifdef WIN32
    serphone_pre_after_ring_stop(true);
	//PrintConsole("ServiceCore::onStopPlayPreRing()\n");
#endif
}

void ServiceCore::sound_config_uninit()
{
	sound_config_t *config=&this->sound_conf;

	lp_config_set_string(this->config,"sound","remote_ring",config->remote_ring);

	if (config->local_ring) {ms_free((void **)&config->local_ring);}
	if (config->remote_ring) {ms_free((void **)&config->remote_ring);}
//    sean 2015
#ifdef WIN32
    if (config->pre_ring) {ms_free((void **)&config->pre_ring);}
    if (config->after_ring) {ms_free((void **)&config->after_ring);}
#endif
}


void ServiceCore::net_config_read()
{
	const char *tempstr = NULL;
	net_conf.download_bw =lp_config_get_int(config,"net","download_bw",0);
	net_conf.upload_bw=lp_config_get_int(config,"net","upload_bw",0);
//    sean update begin 20131011 use p2p default
//	tempstr = lp_config_get_string(config,"net","stun_server",NULL);
    tempstr = lp_config_get_string(config,"net","stun_server","192.168.178.138");
//    sean update end 20131011 use p2p default
//    sean test for ice
//    tempstr = "42.121.15.99";
//    tempstr = "stunserver.org";
     tempstr = "stunserver.org";
	serphone_core_set_stun_server(tempstr);
	tempstr=lp_config_get_string(config,"net","nat_address",NULL);
	if (tempstr!=NULL && (strlen(tempstr)<1))
		tempstr=NULL;
	serphone_core_set_nat_address(tempstr);
//    sean update begin 20131011 use p2p default
	// net_conf.firewall_policy =lp_config_get_int(config,"net","firewall_policy",0);
    net_conf.firewall_policy =lp_config_get_int(config,"net","firewall_policy",LinphonePolicyUseIce);
//    sean update end 20131011 use p2p default
	net_conf.nat_sdp_only=lp_config_get_int(config,"net","nat_sdp_only",0);
	net_conf.mtu=lp_config_get_int(config,"net","mtu",1);
	net_conf.down_ptime =lp_config_get_int(config,"net","download_ptime",0);

}

void ServiceCore::net_config_uninit()
{
	ms_free((void**)&net_conf.stun_server);
}

void ServiceCore::serphone_core_reg_kickedoff()
{
    ms_list_for_each(sip_conf.proxies,(void (*)(void**)) serphone_proxy_config_destroy);
    ms_list_free(sip_conf.proxies);
	sip_conf.proxies=NULL;
    serphone_core_set_default_proxy(NULL);

    Sal *l_sal=this->sal;
	const char *anyaddr;
	LCSipTransports *tr=&sip_conf.transports;

	if( strlen(g_bind_local_addr) > 0)
		anyaddr = (char*)&g_bind_local_addr;
	else if (sip_conf.ipv6_enabled)
		anyaddr="::0";
	else
		anyaddr="0.0.0.0";

	sal_unlisten_ports(l_sal);
	if (tr->udp_port>0){
		if (sal_listen_port (l_sal,anyaddr,tr->udp_port,SalTransportUDP,FALSE)!=0){
			transport_error("udp",tr->udp_port);
			return;
		}
	}
	if (tr->tcp_port>0){
		if (sal_listen_port (l_sal,anyaddr,tr->tcp_port,SalTransportTCP,FALSE)!=0){
			transport_error("tcp",tr->tcp_port);
		}
	}
	if (tr->tls_port>0){
		if (sal_listen_port (l_sal,anyaddr,tr->tls_port,SalTransportTLS,TRUE)!=0){
			transport_error("tls",tr->tls_port);
		}
	}
	apply_user_agent(NULL);

	return;

}

void ServiceCore::serphone_core_parse_capability_token(const char *token)
{
#if !defined(NO_VOIP_FUNCTION)
    if(!token) {
        return ;
    }

	int decode_len = Base64decode_len(token);
	char *decode_token = (char*)malloc(decode_len);
	Base64decode(decode_token, token);

    cJSON *root = cJSON_Parse(decode_token);
    if(!root) {
        free(decode_token);
        return;
    }
    cJSON *localrec = cJSON_GetObjectItem(root, "localrec");
    if( localrec && strcmp(localrec->valuestring, "1")==0 ) {
        capability_conf.localrec = 1;
    }

	cJSON *localrecvoip = cJSON_GetObjectItem(root, "localrecvoip");
	if( localrecvoip && strcmp(localrecvoip->valuestring, "1")==0 ) {
		capability_conf.localrecvoip = 1;
	}

	cJSON *hdvideo = cJSON_GetObjectItem(root, "hdvideo");
	if( hdvideo && strcmp(hdvideo->valuestring, "1")==0 ) {
		capability_conf.hdvideo = 1;
	}


    cJSON_Delete(root);
    free(decode_token);

	PrintConsole("serphone_core_parse_capability_token. localrec=%d localrecvoip=%d hdvieo=%d\n",
		capability_conf.localrec,capability_conf.localrecvoip,capability_conf.hdvideo);
#endif
    return;
}

bool_t ServiceCore::serphone_core_set_process_audio_data_flag(SerPhoneCall *call,bool flag)
{
    processAudioData = flag;
#ifdef HAIYUNTONG
    if (enableHaiyuntong) {
        processAudioData = true;
    }
#endif
    return true;
}

bool_t ServiceCore::serphone_core_set_process_original_audio_data_flag(SerPhoneCall *call,bool flag)
{
    processOriginalAudioData = flag;
    return true;
}

bool_t ServiceCore::serphone_core_set_process_audio_data_flag_internel(SerPhoneCall *call)
{
#if !defined(NO_VOIP_FUNCTION)
	//TODO:
    //cloopenwebrtc::VoEBase *tempVoEBase = cloopenwebrtc::VoEBase::GetInterface(m_voe);
    //tempVoEBase->setProcessData(call->m_AudioChannelID, processAudioData, processOriginalAudioData);
    //tempVoEBase->Release();
    return true;
#endif
	return false;
}

//sean add begin 20140424 SetPrivateCloud
const char *ServiceCore::serphone_get_privateCloudCheckAddress()
{
    return privateCloudCheckAddress;
}
const char *ServiceCore::serphone_get_privateCloudCompanyID()
{
    return privateCloudCampanyID;
}

int ServiceCore::serphone_set_privateCloud(const char *companyID, const char *restAddr, bool isNativeCheck)
{
    if (!companyID || (companyID && strlen(companyID) > 200)) {
        PrintConsole("ERROR: ServiceCore::serphone_set_privateCloud companyID is NULL or is too long, max length 200!\n");
        return -1;
    }
    else {
        memset(privateCloudCampanyID, 0, 200);
        memcpy(privateCloudCampanyID, companyID, strlen(companyID));
    }
    if (isNativeCheck) {
        if (!restAddr || (restAddr && strlen(restAddr) > 100)) {
            PrintConsole("ERROR: ServiceCore::serphone_set_privateCloud restAddr is NULL or is too long, max length 100!\n");
            return -2;
        }
        else
        {
            memset(privateCloudCheckAddress, 0, 100);
            memcpy(privateCloudCheckAddress, restAddr, strlen(restAddr));
        }
    }


    nativeCheck = isNativeCheck;
    return 0;

}

bool ServiceCore::serphone_get_nativeCheck()
{
    return nativeCheck;
}
//sean add end 20140424 SetPrivateCloud

//sean add begin 20140505 tls root ca
int ServiceCore::serphone_set_root_ca_path(const char *caPath)
{
    if ( caPath && strlen(caPath) > 0) {
        rootCaPath = new char[strlen(caPath)+1];
        if (rootCaPath) {
            memcpy(rootCaPath, caPath, strlen(caPath));
            rootCaPath[strlen(caPath)] = '\0';
            PrintConsole("DEBUG: serphone_set_root_ca_path:%s\n",rootCaPath);
            return 0;
        }
        return -1;

    }
    return -1;
}
//sean add end 20140505 tls root ca

//sean add begin 20140512 transfer call
void ServiceCore::serphone_set_isRefering(bool flag)
{
    isRefering = flag;
}
bool ServiceCore::serphone_get_isRefering()
{
    return isRefering;
}
int ServiceCore::serphone_set_referTo(const char *referNo)
{
    if (referNo) {
        int noLen = strlen(referNo);
        if (referTo && strlen(referTo) < noLen) {
            delete [] referTo;
            referTo = NULL;
        }
        if (NULL == referTo) {
            referTo = new char[noLen+1];
        }

        memcpy(referTo, referNo, noLen);
        referTo[noLen] = '\0';
        return 0;
    }
    return -1;

}

int ServiceCore::serphone_set_remote_sip(char *remote)
{
    if (remote) {
        int noLen = strlen(remote);
        if (remoteSipNo && strlen(remoteSipNo) < noLen) {
            delete [] remoteSipNo;
            remoteSipNo = NULL;
        }
        if (NULL == remoteSipNo) {
            remoteSipNo = new char[noLen+1];
        }

        memcpy(remoteSipNo, remote, noLen);
        remoteSipNo[noLen] = '\0';
        return 0;
    }
    return -1;
}

char * ServiceCore::serphone_get_referTo()
{
    return referTo;
}
//sean add end 20140424 SetPrivateCloud

//sean add begin 20140626 set group ID for IP route
int ServiceCore::serphone_set_groupID(const char *group)
{
    if (group) {
        int newLen = strlen(group);
        if (groupID && strlen(groupID) < newLen) {
            delete [] groupID;
            groupID = NULL;
        }
        if (NULL == groupID) {
            groupID = new char[newLen+1];
        }
        memcpy(groupID, group, newLen);
        groupID[newLen] = '\0';
        return 0;
    }
    return -1;
}

int ServiceCore::serphone_set_networkType(const char *type)
{
    if (type && current_call) {
        int newTypeLen = strlen(type);
        if (networkType && strlen(networkType) < newTypeLen) {
            delete [] networkType;
            networkType = NULL;
        }
        if (NULL == networkType) {
            networkType = new char[newTypeLen+1];
        }
        memcpy(networkType, type, newTypeLen);
        networkType[newTypeLen] = '\0';

		ECMedia_set_network_type(current_call->m_AudioChannelID, current_call->m_VideoChannelID, type);
        return 0;
    }
    return -1;
}

const char * ServiceCore::serphone_get_groupID()
{
    int groupLen = strlen(groupID);
    int newLen = groupLen+strlen(networkType)+1;
    if (groupIDAndNetworkType && strlen(groupIDAndNetworkType) < newLen) {
        delete [] groupIDAndNetworkType;
        groupIDAndNetworkType = NULL;
    }
    if (NULL == groupIDAndNetworkType) {
        groupIDAndNetworkType = new char[newLen+1];
    }
    memcpy(groupIDAndNetworkType, groupID, groupLen);
    groupIDAndNetworkType[groupLen] = ';';
    memcpy(groupIDAndNetworkType+groupLen+1, networkType, strlen(networkType));
    groupIDAndNetworkType[newLen] = '\0';
    return groupIDAndNetworkType;
}

const char * ServiceCore::serphone_get_self_sipNo()
{
    return selfSipNo;
}
//const char * ServiceCore::serphone_get_networkType()
//{
//    return networkType;
//}
//sean add end 20140626 set group ID for IP route

void serphone_core_set_bind_local_addr(const char* addr)
{
	if(!addr) return;
	memset(g_bind_local_addr, 0, sizeof(g_bind_local_addr));
	strncpy(g_bind_local_addr, addr, sizeof(g_bind_local_addr));
	g_bind_local_addr[sizeof(g_bind_local_addr)-1] = '\0';
}

int ServiceCore::serphone_set_traceFlag(/*bool flag*/) //Don't use flag for the time being
{
	ECMedia_set_trace(NULL, (void*)CCPClientPrintLog, 25, 100);
	return 0;
}

#ifdef HAIYUNTONG
int ServiceCore::serphone_dump_conf_key(SalOp *op)
{
    if (op->confKey) {
        PrintConsole("[DEBUG HAIYUNTONG] confKey:%s\n",op->confKey);
    }
}
#endif
const char * ServiceCore::serphone_get_proxyAddr()
{
    return proxyAddr;
}
int ServiceCore::serphone_get_proxyPort()
{
    return proxyPort;
}

void ServiceCore::onDtmf(const char *callid, char dtmf)
{
//    typedef void (*DtmfReceived)(class ServiceCore* lc, SerPhoneCall *call, char dtmf);
    vtable.dtmf_received(this, callid, dtmf);
}

#if 0
int ServiceCore::serphone_set_ringback(SerPhoneCall *call, bool flag)
{
    //将来若用这个功能的话，还需要在创建audio channel的时候，将ringbackFlag设置到对应的channel中??    ringbackFlag = flag;
    return 0;
}
#endif

void ServiceCore::serphone_core_enable_temp_auth(bool flag)
{
    tempAuth = flag;
}

bool ServiceCore::serphone_core_get_temp_auth()
{
    return tempAuth;
}

const char * ServiceCore::serphone_core_get_registerUserdata()
{
    return registerUserdata;
}

void ServiceCore::SetNackEnabled(bool audioEnabled, bool videoEnabled)
{
	audioNackEnabled = audioEnabled;
	videoNackEnabled = videoEnabled;
}

void ServiceCore::SetVideoProtectionMode(int mode)//0:nack	1:fec	2:hybrid
{
	videoProtectionMode = mode;
}

void ServiceCore::SetP2PEnabled(bool enable)
{
#ifdef ENABLE_REMB_TMMBR_CONFIG
    p2pEnabled = enable;
#endif
}
void ServiceCore::SetRembEnabled(bool enable)
{
#ifdef ENABLE_REMB_TMMBR_CONFIG
	rembEnabled = enable;
#endif
}
void ServiceCore::SetTmmbrEnabled(bool enable)
{
#ifdef ENABLE_REMB_TMMBR_CONFIG
	tmmbrEnabled = enable;
#endif
}

void ServiceCore::setVideoMode(int videoModeIndex)
{
	m_videoModeChoose = videoModeIndex;
}
void ServiceCore::setDesktopShareParam(int desktop_width, int desktop_height, int desktop_frame_rate, int desktop_bit_rate)
{
	m_desktop_width = desktop_width;
	m_desktop_height = desktop_height;
	m_desktop_frame_rate = desktop_frame_rate;
	m_desktop_bit_rate = desktop_bit_rate;
}

int ServiceCore::setScreeShareActivity(SerPhoneCall *call, void *activity)
{
#ifdef VIDEO_ENABLED
    m_desktop_activity = activity;
    return ECMedia_set_screen_share_activity(call->m_desktopShareDeviceId, activity);
#endif
    return 0;
}


int ServiceCore::serphone_set_reconnect(bool flag)
{
    reconnect = flag;
    return 0;
}

bool ServiceCore::serphone_get_reconnect()
{
    return reconnect;
}

void ServiceCore::onReceiverStats(const char *callid, const int framerate, const int bitrate)
{
	vtable.receiver_stats_received(this, callid, framerate, bitrate);
}

void ServiceCore::onIncomingCodecChanged(const char *callid, const int width, const int height)
{
	vtable.receiver_codec_changed(this, callid, width, height);
}


int ServiceCore::startRecordLocalMedia(const char *fileName, void *localview) {
#ifdef VIDEO_ENABLED
    return ECMedia_startRecordLocalMedia(fileName, localview);
#endif
    return 0;
}


void ServiceCore::stopRecordLocalMedia() {
#ifdef VIDEO_ENABLED
    ECMedia_stopRecordLocalMedia();
#endif
}

//cloopenwebrtc::VideoSendStream::Config ServiceCore::CreateVideoSendStreamConfig()
//{
//	cloopenwebrtc::VideoSendStream::Config config;
//	config.encoder_settings.payload_name = "libvpx";
//	config.encoder_settings.payload_type = 97;
//	config.encoder_settings.internal_source = true;
//	//todo....
//
//	return config;
//}




