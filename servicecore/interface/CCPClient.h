
#ifndef _CCPCLIENT_H_
#define _CCPCLIENT_H_

#include "sdk_common.h"
//#include "servicecore.h"

// #ifdef WIN32
// #define CCPAPI  _declspec(dllexport)
// #else
 #define CCPAPI  
// #endif


#ifndef __cplusplus
typedef enum {false, true} bool;
#endif

#ifdef __cplusplus
extern "C" {
#endif

	enum {
		ERR_SDK_ALREADY_INIT =-1000,
		ERR_NO_MEMORY,
		ERR_SDK_UN_INIT,
		ERR_INVALID_CALLED,
		ERR_INVALID_CALLID,
		ERR_INVALID_CALL_BACK_INTERFACE,
		ERR_INVALID_URL,
		ERR_INVALID_PARAM,
		ERR_GET_LOGIN_INFO,
		ERR_NOT_SUPPORT,
		ERR_GET_SOFTSWITCH_ADDRESS,
	};
	enum {
		LOG_LEVEL_NONE	= 0,
		LOG_LEVEL_DEBUG	,
		LOG_LEVEL_END
	};

    
#ifdef OLDERRORCODE
    enum {
        ReasonNone,
        ReasonNoResponse,
        ReasonBadCredentials,
        ReasonDeclined,
        ReasonNotFound,
        ReasonCallMissed,
        ReasonBusy,
        ReasonNoNetwork,
        ReasonReFetchSoftSwitch,
        ReasonKickedOff,
        ReasonCalleeNoVoip,
        ReasonInvalidProxy,
        ReasonHaiyuntongInitFailed
    };
#else
    enum {
        ReasonNone = 175000,
        ReasonNoResponse,
        ReasonBadCredentials,
        ReasonReFetchSoftSwitch,
        ReasonKickedOff,
        ReasonCalleeNoVoip,
        ReasonInvalidProxy,
        ReasonNoNetwork,
        ReasonConfNotExist,
        ReasonConfSipNotIn,
        ReasonConfMemberNotIn,
        ReasonConfMemberNOVideo,
        ReasonHaiyuntongInitFailed
        
    };
#endif
	enum {
		USERDATA_FOR_TOKEN	=0,
		USERDATA_FOR_USER_AGENT,
		USERDATA_FOR_INVITE
	};

	enum {
		VOICE_CALL,
		VIDEO_CALL
	};

	enum {
		NETWORK_NONE,
		NETWORK_LAN,
		NETWORK_WIFI,
		NETWORK_GPRS,
		NETWORK_3G
	};

	enum  {
		G_EVENT_EarlyMedia,
		G_EVENT_MessageCommand,
		G_EVENT_RemoteVideoRatio,
		G_EVENT_MediaInitFailed,
        G_EVENT_AudioDestinationChanged,
        G_EVENT_VideoDestinationChanged,
		G_EVENT_OpenCameraFailed,
		G_EVENT_VideoCaptureStatus,
		G_EVENT_VideoPacketTimeOut
	};
    
    typedef enum _SerphoneAudioMode{
        SerphoneAudioModeLowBandWidth,
        SerphoneAudioModeGoodHighQuality
    } SerphoneAudioMode;
    
    enum  {
        codec_PCMU,
		codec_G729,
        codec_OPUS48K,
        codec_OPUS16K,
        codec_OPUS8K,
		codec_VP8,
		codec_H264,
		codec_H264SVC,
		codec_AMR
	};

//呼叫回调函数
struct _CALLBACKINTERFACE{
	void (*onGetCapabilityToken)();			//由APP提供的获取能力token的回调方法
	void (*onConnected)();					//与云通讯平台连接成功
	void (*onConnectError)(int reason);		//与云通讯平台连接断开或者出错
	void (*onIncomingCallReceived)(int callType, const char *callid, const char *caller);  //有呼叫呼入
	void (*onCallProceeding)(const char*callied);		//呼叫已经被云通讯平台处理
	void (*onCallAlerting)(const char *callid);			//呼叫振铃
	void (*onCallAnswered)(const char *callid);			//外呼对方应答
	void (*onMakeCallFailed)(const char *callid,int reason);//外呼失败
	void (*onCallPaused)(const char* callid);				//本地Pause呼叫成功
	void (*onCallPausedByRemote)(const char *callid);		//呼叫被被叫pasue
	void (*onCallReleased)(const char *callid);				//呼叫挂机
	void (*onCallTransfered)(const char *callid , const char *destionation); //呼叫被转接
	void (*onDtmfReceived)(const char *callid, char dtmf);		//收到DTMF按键时的回调
	void (*onTextMessageReceived)(const char *sender, const char *receiver, const char *sendtime, const char *msgid, const char *message, const char *userdata);		//收到文本短消息
	//void (*onGroupTextMessageReceived)(const char* sender, const char* groupid, const char *message); //收到群组文本短消息
	void (*onMessageSendReport)(const char*msgid, const char*time, int status); //发送消息结果
	void (*onLogInfo)(const char* loginfo); // 用于接收底层的log信息,调试出现的问题.
	void (*onResumed)(const char* callid);	
	void (*onNotifyGeneralEvent)(const char*callid, int eventType, const char*userdata , int intdata);	//通用事件通知
    void (*onCallMediaUpdateRequest)(const char*callid,int request); // 收到对方请求的更新媒体 request：0  请求增加视频（需要响应） 1:请求删除视频（不需要响应）
    void (*onCallMediaUpdateResponse)(const char*callid,int response);  // 本地请求更新媒体后，更新后的媒体状态 0 有视频 1 无视频
    void (*onDeliverVideoFrame)(const char*callid, unsigned char*buf, int size, int width, int height); //视频通话过程中，如果请求本地视频，视频数据通过这个函数上报。视频格式是RGB24
    void (*onRecordVoiceStatus)(const char *callid, const char *fileName, int status); //通话录音结束或者出现错误，上报事件。filenName是上层传下的文件名。 status是录音状态：0： 成功  -1：失败，录音文件删除  -2：写文件失败，保留已经保存的录音。
    void (*onAudioData)(const char *callid, const void *inData, int inLen, void *outData, int &outLen, bool send); //在音频数据发送之前，将音频数据返回给上层处理，然后将上层处理后的数据返回来。
    void (*onOriginalAudioData)(const char *callid, const void *inData, int inLen, int sampleRate, int numChannels, const char *codec, bool send); //将原始音频数据抛到上层。
    void (*onMessageRemoteVideoRotate)(const char *degree);//当远端视频发生旋转时，将旋转的角度上报，degree为向左旋转的度数（0，90，180，270）。
    void (*onRequestSpecifiedVideoFailed)(const char *callid, const char *sip, int reason);//视频会议时，请求视频数据失败
	void (*onStopSpecifiedVideoResponse)(const char *callid, const char *sip, int response, void *window);//视频会议时，取消视频数据响应
    void (*onEnableSrtp)(const char *sip, bool isCaller);//设置srtp加密属性
    void (*onRemoteVideoRatioChanged)(const char *callid, int width, int height, bool isVideoConference, const char *sipNo);//远端视频媒体分辨率变化时上报
    void (*onLogOut)();
    void (*oneXosipThreadStop)();
    void (*onReceiverStats)(const char *callid, const int framerate, const int bitrate);//接收端统计的接收帧率，码率，默认1秒更新一次
	void (*onIncomingCodecChanged)(const char *callid, const int width, const int height); //接收端的解码图像的宽，高
};

typedef struct _CALLBACKINTERFACE CCallbackInterface;

    /*! @function
     ********************************************************************************
     函数名   : getVersion
     功能     : 获取SDK版本信息
     返回值   : 版本信息字符串。
     说明     : 版本信息格式为："版本号#平台#ARM版本#音频开关#视频开关#编译日期 编译时间"
               版本号: 格式为X.X.X 如1.1.18
               平台:  Android、 Windows、 iOS、 Mac OS、 Linux
               ARM版本:  arm、 armv7、 armv5
               音频开关:  voice=false、 voice=true
               视频开关:  video=false、 video=true
               编译日期:  "MM DD YYYY"  如"Jan 19 2013"
               编译时间:  "hh:mm:ss"    如”08:30:23”）
     *******************************************************************************/
	CCPAPI const char* getVersion();
	/*! @function
	********************************************************************************
	函数名   : initialize
	功能     : 初始化CCP SDK
	参数     : [IN]  CCallbackInterface   : 回调函数指针结构
	返回值   : 是否初始化成功 0：成功； 非0失败
	*******************************************************************************/
	CCPAPI int initialize( CCallbackInterface *CCallbackInterface );


	

	/*! @function
	********************************************************************************
	函数名   : setUserName
	功能     : 设置显示用户名
	参数     : [IN]  username   : 用户名
	返回值   : 是否初始化成功 0：成功； 非0失败
	*******************************************************************************/
	CCPAPI int setUserName(const char*username);

	/*! @function
	********************************************************************************
	函数名   : makeCall
	功能     : 发起呼叫
	参数     : [IN]  called	  : 被叫方号码
	返回值   : 返回值为callid,本次呼叫的唯一标识; NULL表示失败.
	*******************************************************************************/
	CCPAPI const char* makeCall(int callType, const char *called );
	/*! @function
	********************************************************************************
	函数名   : acceptCall
	功能     : 应答呼入
	参数     : [IN]  callid	  : 当前呼叫的唯一标识
	返回值   : 是否成功 0：成功； 非0失败
	*******************************************************************************/
	CCPAPI int acceptCall(const char *callid);

	/*! @function
	********************************************************************************
	函数名   : rejectCall
	功能     : 拒绝呼叫
	参数     : [IN]  callid	  : 当前呼叫的唯一标识
			   [IN]	 reason	  : 拒绝呼叫的原因
	返回值   : 是否成功 0：成功； 非0失败
	*******************************************************************************/
	CCPAPI int rejectCall(const char *callid , int reason);

	/*! @function
	********************************************************************************
	函数名   : pauseCall
	功能     : 暂停呼叫，呼叫暂停以后, 本地的语音数据将不再传递到对方.
	参数     : [IN]  callid	  : 当前呼叫的唯一标识
			   [IN]	 reason	  : 拒绝呼叫的原因
	返回值   : 是否成功 0：成功； 非0失败
	*******************************************************************************/
	CCPAPI int pauseCall(const char *callid );

	/*! @function
	********************************************************************************
	函数名   : resumeCall
	功能     : 恢复暂停的呼叫
	参数     : [IN]  callid	  : 当前呼叫的唯一标识
	返回值   : 是否成功 0：成功； 非0失败
	*******************************************************************************/
	CCPAPI int resumeCall(const char *callid );

	/*! @function
	********************************************************************************
	函数名   : transferCall
	功能     : 呼叫转移
	参数     : [IN]  callid			: 当前呼叫的唯一标识
			   [IN]	 destination	: 目标号码
               [IN]  type           : 呼转类型（预留）
	返回值   : 是否成功 0：成功； 非0失败
	*******************************************************************************/
	CCPAPI int transferCall(const char *callid , const char *destination, int type = 0);

	/*! @function
	********************************************************************************
	函数名   : releaseCall
	功能     : 挂机
	参数     : [IN]  callid	  : 当前呼叫的唯一标识， 如果callid 为NULL,这代表所有呼叫.
			   [IN]	 reason	  : 释放呼叫的原因
	返回值   : 是否成功 0：成功； 非0失败
	*******************************************************************************/
	CCPAPI int releaseCall(const char *callid , int reason);

		
	/*! @function
	********************************************************************************
	函数名   : sendDTMF
	功能     : 发送按键信息
	参数     : [IN]  callid	  : 当前呼叫的唯一标识.
			   [IN]	 dtmf	  : 一个按键值，'0'-'9' '*' ,'#'
	返回值   : 是否成功 0：成功； 非0失败
	*******************************************************************************/
	CCPAPI int sendDTMF(const char *callid, const char dtmf);


	/*! @function
	********************************************************************************
	函数名   : unInitialize
	功能     : 客户端注销
	返回值   : 是否成功 0：成功； 非0失败
	*******************************************************************************/
	CCPAPI int unInitialize();
    
    /*! @function
     ********************************************************************************
     函数名   : acceptCallByMediaType
     功能     : 接受呼叫，可以选择媒体类型
     参数     : [IN]  callid	  : 当前呼叫的唯一标识.
     [IN]  type    : 1 音视频  0 音频
     返回值   : 是否成功 0：成功； 非0失败
     *******************************************************************************/
    CCPAPI int acceptCallByMediaType(const char *callid, int type);
    
    /*! @function
     ********************************************************************************
     函数名   : updateCallMedia
     功能     : 更新已存在呼叫的媒体类型
     参数     : [IN]  callid	  : 当前呼叫的唯一标识.
     [IN]  request  : 1 添加视频 0 去除视频
     返回值   : 是否成功 0：成功； 非0失败
     *******************************************************************************/
    CCPAPI int  updateCallMedia(const char *callid, int request);
    
    /*! @function
     ********************************************************************************
     函数名   : answerCallMediaUpdate
     功能     : 回复对方的更新请求
     参数     : [IN]  callid	  : 当前呼叫的唯一标识.
     [IN]  action   : 1 同意  0 拒绝
     返回值   : 是否成功 0：成功； 非0失败
     *******************************************************************************/
    CCPAPI int answerCallMediaUpdate(const char *callid, int action);
    
    /*! @function
     ********************************************************************************
     函数名   : getCallMeidaType
     功能     : 获取呼叫的媒体类型
     参数     : [IN]  callid	  : 当前呼叫的唯一标识.
     返回值   : 成功返回mediatype（VOICE_CALL/VIDEO_CALL) -1失败
     *******************************************************************************/
    CCPAPI int getCallMediaType(const char *callid);

    
    /*! @function
     ********************************************************************************
     函数名   : setSrtpEnabled
     功能     : 设置SRTP加密属性
     参数     : [IN]  tls : ture设置信令加密； false不设置信令加密.
                [IN]  srtp : true设置srtp加密；false不设置srtp加密。该值为false时，userMode、cryptType、key等参数均忽略。
                [IN]  userMode : true用户模式，false标准模式。用户模式时需要用户设置srtp的key，非用户模式时，srtp得key由程序自动生成。而且用户模式时，本地收发都是用的本地的key，不会用sdp中的key。
                [IN]  cryptType :
                     AES_256_SHA1_80 =3,
                     AES_256_SHA1_32 =4,
                [IN]  key : 加解密秘钥（长度46个字节）
     返回值   : -1
     *******************************************************************************/
    CCPAPI int setSrtpEnabled(bool tls, bool srtp, bool userMode, int cryptType, const char *key);
    
    /*! @function
     ********************************************************************************
     函数名   : setTlsSrtpEnabled
     功能     : 设置SRTP加密属性
     参数     : [IN]  tls : ture设置信令加密； false不设置信令加密.
                [IN]  srtp : true设置srtp加密；false不设置srtp加密。该值为false时，userMode、cryptType、key等参数均忽略。
                [IN]  cryptType :
                         AES_256_SHA1_80 =3,
                         AES_256_SHA1_32 =4,
                [IN]  key : 加解密秘钥（长度46个字节）。不需要设置key值时，请传NULL。当key值为NULL时，应用随机生成key值。
     返回值   : -1
     *******************************************************************************/
    CCPAPI int setTlsSrtpEnabled(bool tls, bool srtp, int cryptType, const char *key);

    /*! @function
     ********************************************************************************
     函数名   : setProcessDataEnabled
     功能     : 设置允许上层处理音频数据
     参数     : [IN]  callid	  : 当前呼叫的唯一标识.
                [IN] flag     : true 允许上层处理； false 不允许上层处理。
     返回值   : 成功 0 失败-1
     *******************************************************************************/
    CCPAPI int setProcessDataEnabled(const char *callid, bool flag);
    
    /*! @function
     ********************************************************************************
     函数名   : setPrivateCloud
     功能     : 私有云校验接口
     参数     : [IN]  companyID	  : 企业ID.
                [IN] restAddr     : rest地址.
                [IN] nativeCheck  : 是否本地校验.
     返回值   : 成功 0 -1 companyID过长（最大199） -2 restAdd过长（99）
     *******************************************************************************/
    CCPAPI int setPrivateCloud(const char *companyID, const char *restAddr, bool nativeCheck);
    
    /*! @function
     ********************************************************************************
     函数名   : setRootCAPath
     功能     : 私有云校验接口
     参数     : [IN]  caPath	  : 根证书路径.
     返回值   : 成功 0 失败 -1
     *******************************************************************************/
    CCPAPI int setRootCAPath(const char * caPath);
    
    /*! @function
     ********************************************************************************
     函数名   : registerAudioDevice
     功能     : 初始化音频设备
     返回值   : 成功 0 失败 -1
     *******************************************************************************/
    CCPAPI int registerAudioDevice();
    
    /*! @function
     ********************************************************************************
     函数名   : deregisterAudioDevice
     功能     : 释放音频设备
     返回值   : 成功 0 失败 -1
     *******************************************************************************/
    CCPAPI int deregisterAudioDevice();
    
    /*! @function
     ********************************************************************************
     函数名   : SetNetworkGroupId
     功能     : 设置网络组ID
     返回值   : 成功 0 失败 -1
     *******************************************************************************/
    CCPAPI int SetNetworkGroupId(const char *groupID);
    
    /*! @function
     ********************************************************************************
     函数名   : setProcessOriginalDataEnabled
     功能     : 设置允许上层处理音频数据
     参数     : [IN]  callid	  : 当前呼叫的唯一标识.
                [IN] flag     : true 允许上层处理； false 不允许上层处理。
     返回值   : 成功 0 失败-1
     *******************************************************************************/
    CCPAPI int setProcessOriginalDataEnabled(const char *callid, bool flag);
    
    /*! @function
     ********************************************************************************
     函数名   : setVideoConferenceAddr
     功能     : 设置视频会议服务器地址
     参数     : [IN] ip : 视频会议服务器ip.
     返回值   : 成功 0 失败 -1
     *******************************************************************************/
    CCPAPI int setVideoConferenceAddr(const char *ip);
    /*! @function
     ********************************************************************************
     函数名   : requestMemberVideo
     功能     : 视频会议中请求某一远端视频
     参数     : [IN]  conferenceNo	  :  所在会议号.
                [IN]  conferencePasswd : 所在会议密码.
                [IN]  remoteSipNo     :  请求远端用户的sip号.
                [IN]  videoWindow     :  当成功请求时，展示该成员的窗口.
                [IN]  port            :  当前请求的目的端口.
     返回值   : 成功 0 失败 -1(remoteSipNo为NULL) -2(videoWindow为NULL) -3(conferenceNo为NULL) -4(confPasswd为NULL) -5(自己的sip号为NULL) -6(会议服务器的ip为NULL) -7(该账户的视频已经成功请求) -8(正在停止当前用户视频流，需要等待)
     *******************************************************************************/
    CCPAPI int requestMemberVideo(const char *conferenceNo, const char *conferencePasswd, const char *remoteSipNo, void *videoWindow, int port);

    /*! @function
     ********************************************************************************
     函数名   : stopMemberVideo
     功能     : 视频会议中停止某一远端视频
     参数     : [IN]  conferenceNo	  :  所在会议号.
                [IN]  conferencePasswd : 所在会议密码.
                [IN]  remoteSipNo     :  请求远端用户的sip号.
     返回值   : 成功 0 失败 -1(remoteSipNo为NULL) -3(conferenceNo为NULL) -4(confPasswd为NULL) -5(自己的sip号为NULL) -6(会议服务器的ip为NULL) -7(该账户的视频没有成功请求)
     *******************************************************************************/
	CCPAPI int stopMemberVideo(const char *conferenceNo, const char *conferencePasswd, const char *remoteSipNo);
    
    /*! @function
     ********************************************************************************
     函数名   : resetVideoConfWindow
     功能     : 设置音频编解码模式
     参数     : [IN]  sip	  :  需要调整的sip号.
                [IN]  newWindow	  :  新窗口.
     返回值   : 成功 0 失败 -1(不支持视频) -2(sip号为NULL) -3(newWindow为NULL) -4(找不到该sip号相关的资料)
     *******************************************************************************/
    CCPAPI int resetVideoConfWindow(const char *sip, void *newWindow);
    
    /*! @function
     ********************************************************************************
     函数名   : setAudioMode
     功能     : 设置音频编解码模式
     参数     : [IN]  mode	  :  音频编解码模式（0 低带宽 1 好音质）.
     返回值   : 成功 0 失败 -1
     *******************************************************************************/
	CCPAPI int setAudioMode(int mode);

    
    CCPAPI int setDeviceID(const char *deviceid, const char *appId, bool testFlag);
    
    CCPAPI int setHaiyuntongEnabled(bool flag);
    /*! @function
     ********************************************************************************
     函数名   : haiyuntongFileEncrypt
     功能     : 海运通文件加密
     参数     : [IN]  file	  :  加密内容.
                [IN] fileLen    :   加密内容长度.
                [IN] remoteSip  :   对端sip号.
                [IN] remoteSipLen   :   对端sip号长度.
                [OUT] fileCrpEnvelopp    :   加密后的内容.
                [OUT] fileCrpEnvelopLen  :   加密后内容的长度.
     返回值   : 成功 0 失败 -1 不支持 -1001
     *******************************************************************************/
    CCPAPI int haiyuntongFileEncrypt(const unsigned char *file, long fileLen, char *remoteSip, long remoteSipLen, unsigned char *fileCrpEnvelopp, long* fileCrpEnvelopLen);
    
    /*! @function
     ********************************************************************************
     函数名   : haiyuntongFileDecrypt
     功能     : 海运通文件解密
     参数     : [IN]  file	  :  待解密内容.
                 [IN] fileLen    :   待解密内容长度.
                 [IN] remoteSip  :   对端sip号.
                 [IN] remoteSipLen   :   对端sip号长度.
                 [OUT] fileDeCrpt    :   解密后的内容.
                 [OUT] fileDeCrptLen  :   解密后内容的长度.
     返回值   : 成功 0 失败 -1 不支持 -1001
     *******************************************************************************/
    CCPAPI int haiyuntongFileDecrypt(const unsigned char*file, long fileLen, char *remoteSip, long remoteSipLen, unsigned char *fileDeCrpt, long* fileDeCrptLen);
    
    /*! @function
     ********************************************************************************
     函数名   : haiyuntongGroupFileEncrypt
     功能     : 海运通群组文件加密
     参数     : [IN]  file	  :  加密内容.
                 [IN] fileLen    :   加密内容长度.
                 [IN] userList  :   接收方用户列表.
                 [IN] earchLen  :   用户列表中每个用户的长度.
                 [IN] numOfUsers   :   接收方用户数量.
                 [OUT] groupFileCrpEnvelopp    :   加密后的内容.
                 [OUT] groupFileCrpEnvelopLen  :   加密后内容的长度.
     返回值   : 成功 0 失败 -1 不支持 -1001
     *******************************************************************************/
    CCPAPI int haiyuntongGroupFileEncrypt(const unsigned char *file, long fileLen, char **userList, long *eachLen, int numOfUsers,  unsigned char *groupFileCrpEnvelopp, long* groupFileCrpEnvelopLen);
    
    /*! @function
     ********************************************************************************
     函数名   : haiyuntongGroupFileDecrypt
     功能     : 海运通群组文件解密
     参数     : [IN]  file	  :  待解密内容.
                 [IN] fileLen    :   待解密内容长度.
                 [IN] proxyAddr  :   服务器id.
                 [IN] proxyAddrLen   :   服务器id长度.
                 [OUT] groupFileDecrpt    :   解密后的内容.
                 [OUT] groupFileDecrptLen  :   解密后内容的长度.
     返回值   : 成功 0 失败 -1 不支持 -1001
     *******************************************************************************/
    CCPAPI int haiyuntongGroupFileDecrypt(const unsigned char *file, long fileLen, char *selfSip, long selfSipLen, unsigned char *groupFileDecrpt, long*groupFileDecrptLen);
    
    /*! @function
     ********************************************************************************
     函数名   : haiyuntongAddContact
     功能     : 海运通添加联系人
     参数     : [IN]  remoteSip	  :  sip号.
                 [IN] remoteSipLen    :   sip号长度.
     返回值   : 成功 0 失败 -1 不支持 -1001
     *******************************************************************************/
    CCPAPI int haiyuntongAddContact(char **userlist, long* userlistLen, int num);
    
    /*! @function
     ********************************************************************************
     函数名   : haiyuntongDelContact
     功能     : 海运通删除联系人
     参数     : [IN]  remoteSip	  :  sip号.
                [IN] remoteSipLen    :   sip号长度.
     返回值   : 成功 0 失败 -1 不支持 -1001
     *******************************************************************************/
    CCPAPI int haiyuntongDelContact(char *remoteSip, long remoteSipLen);
    
    /*! @function
     ********************************************************************************
     函数名   : isExistCert
     功能     : 海运通根据sip判断证书是否存在
     参数     : [IN]  sip	  :  sip号.
     [IN] remoteSipLen    :   sip号长度.
     返回值   : 成功 0 失败 -1 不支持 -1001-	
     *******************************************************************************/
    CCPAPI int haiyuntongIsExistCert(const char *sip, long sipLen);
    
    CCPAPI int openTraceFile(const char *filePath);
    CCPAPI int openTraceFile2(const char *filePath);
    
    CCPAPI int setReconnectFlag(bool flag);
    CCPAPI int EnableOpusFEC(bool enable);
    CCPAPI  int SetOpusPacketLossRate(int rate);

	CCPAPI  int setAudioKeepAlive(char *callid, bool enable, int interval);
	CCPAPI  int setVideoKeepAlive(char *callid, bool enable, int interval);

	CCPAPI void *createLiveStream();
	CCPAPI int playLiveStream(void *handle, const char * url, void *renderView);
	CCPAPI int pushLiveStream(void *handle, const char * url, void *renderView);
	CCPAPI void setLiveVideoSource(void *handle, int video_source);
	CCPAPI void stopLiveStream(void *handle);
	CCPAPI void releaseLiveStream(void *handle);
	CCPAPI int selectCameraLiveStream(void *handle, int index, int width, int height, int fps);
	CCPAPI void selectShareWindow(void *handle, int type, int id);
	CCPAPI int getShareWindows(void *handle, WindowShare ** windows);
	CCPAPI int startSendRtpPacket(int &channel, const char *ip, int rtp_port);
	CCPAPI int startRecvRtpPacket(int channelNum);
    
    // record local mp4 video.
    CCPAPI int startRecordLocalMedia(const char *fileName, void *localview);
    CCPAPI void stopRecordLocalMedia();
    
#ifdef __cplusplus
}
#endif

#endif //_CCPCLIENT_H_
