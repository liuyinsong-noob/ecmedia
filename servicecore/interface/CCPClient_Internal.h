#ifndef _CCPCLIENT_INTERNAL_H_
#define _CCPCLIENT_INTERNAL_H_

#include "sdk_common.h"
#include "../ECMedia/source/MediaStatisticsData.pb.h"

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

	/*! @function
	********************************************************************************
	函数名   : connect
	功能     : 连接云通讯平台呼叫控制服务器
	参数     : [IN]  proxy_addr   : 云通讯平台呼叫控制服务器地址
			   [IN]  proxy_port	  : 云通讯平台呼叫控制服务器端口
			   [IN]  account	  : 用户帐号
			   [IN]  password	  : 用户密码
              [IN]  capability    : base64编码的用户权限控制
	返回值   : 是否初始化成功 0：成功； 非0失败
	*******************************************************************************/
	CCPAPI int connectToCCP(const char *proxy_addr, int proxy_port, const char *account, const char *password, const char *capability);


    /*! @function
     ********************************************************************************
     函数名   : connectToCCPWithXML
     功能     : 连接云通讯平台呼叫控制服务器
     参数     : [IN]  addressXML     : 包含多个软交换地址的XML
     [IN]  account       : 用户帐号
     [IN]  password      : 用户密码
     [IN]  capability    : base64编码的用户权限控制
     返回值   : 是否初始化成功 0：成功；
     -1：XML格式错误；
     -2：STATUS非零；
     -3：没有IP列表。
     *******************************************************************************/
	CCPAPI int connectToCCPWithXML(const char *addressXML, const char *account, const char *password, const char *capability);

    /*! @function
     ********************************************************************************
     函数名   : connect
     功能     : 断开连接云通讯平台呼叫控制服务器
     返回值   : 是否初始化成功 0：成功； 非0失败
     *******************************************************************************/
    CCPAPI int disConnectToCCP();

	/*! @function
	********************************************************************************
	函数名   : sendTextMessage
	功能     : 发送文本信息
	参数     : [IN]  receiver : 接收者
			   [IN]	 message  : 消息内容
	返回值   : 消息id
	*******************************************************************************/
	CCPAPI const char *sendTextMessage(const char *receiver, const char *message, const char *userdata);

	CCPAPI int getCallState(const char *callid);

	CCPAPI int setLogLevel( int level);

    CCPAPI void setTraceFlag(bool enable);
	
	CCPAPI int	enableGlobalAudioInDevice(bool enable);

	CCPAPI int	enableLoudsSpeaker(bool enable);

	CCPAPI bool getLoudsSpeakerStatus();

	CCPAPI int setMute(bool on);

	CCPAPI bool getMuteStatus();

	CCPAPI int setSpeakerMute(bool on);

	CCPAPI bool getSpeakerMuteStatus();

    CCPAPI bool getMuteStatusSoft(const char* callid);

    CCPAPI int setMuteSoft(const char*callid, bool on);

	CCPAPI int setRing(const char* filename);

	CCPAPI int setRingback(const char*filename);

    CCPAPI int setPreRing(const char* filename);

    CCPAPI int setAfterRing(const char* filename);

	CCPAPI int setUserData(int type ,const char *data);

	CCPAPI int getUserData(int type, char* buffer, int buflen);

	CCPAPI const char *getCurrentCall();

	int setAndroidObjects(void* javaVM, void* env, void* context);
	CCPAPI int setVideoView( void *view, void *localView);

	CCPAPI void setCapabilityToken(const char *token);

	CCPAPI void setNetworkType(int networktype,bool connected,bool reconnect);

	CCPAPI void sendKeepAlive();

	CCPAPI int selectCamera(int cameraIndex, int capabilityIndex,int fps,int rotate,bool force);

	CCPAPI int getCameraInfo(CameraInfo **);

    /*! @function
     ********************************************************************************
     函数名   : getCallStatistics
     功能     : 获取呼叫的统计数据
     参数    : [IN] type : 媒体类型. enum {VOICE_CALL,VIDEO_CALL};
              [OUT] statistics:  MediaStatisticsInfo结构的统计数据
     返回值   :  成功 0
     失败 -1
     *****************************************************************************/
	CCPAPI int getCallStatistics(int type,MediaStatisticsInfo *);

	CCPAPI void  enableKeepAlive(bool enable);

	CCPAPI void  setKeepAliveTimeout(int forWiFi, int for3G);

	//CCPAPI int setUserName(const char*username);

    /*! @function
     ********************************************************************************
     函数名   : setCodecEnabled
     功能     : 设置支持的编解码方式，默认全部都支持
     参数     : [IN]  type : 编解码类型.
                     enum  {
                     codec_iLBC,
                     codec_G729,
                     codec_PCMU,
                     codec_PCMA,
                     codec_VP8,
                     codec_H264,
					 codec_H264SVC,
                     codec_SILK8K,
                     codec_SILK12K,
                     codec_SILK16K,
                     codec_AMR
                     };
                [IN] enabled: 0 不支持；1 支持
     返回值   :  始终返回 0
     *******************************************************************************/
    CCPAPI int setCodecEnabled(int type, bool enabled);
    CCPAPI bool getCodecEnabled(int type);

    /*! @function
     ********************************************************************************
     函数名   : setAudioConfigEnabled
     功能     : 设置音频处理的开关
     参数     :   [IN] type : 音频处理类型. enum { AUDIO_AGC, AUDIO_EC, AUDIO_NS };
                 [IN] enabled:  AGC默认关闭; EC和NS默认开启.
                 [IN] mode: 各自对应的模式: AgcMode、EcMode、NsMode.
     返回值   :  成功 0
                失败 -1
     *****************************************************************************/
    CCPAPI int setAudioConfigEnabled(int type, bool enabled, int mode);
    CCPAPI int getAudioConfigEnabled(int type, bool *enabled, int *mode);

    /*! @function
     ********************************************************************************
     函数名   : resetAudioDevice
     功能     : 重新启动音频设备
     返回值   :  成功 0
     失败 -1
     *****************************************************************************/
    CCPAPI int resetAudioDevice();

    /*! @function
     ********************************************************************************
     函数名   : setDtxEnabled
     功能     : 设置DTX功能的开启，默认是关闭。关闭后没有舒适噪音，减少带宽。     ***************************************************************************/
    CCPAPI void setDtxEnabled(bool enabled);

    /*! @function
     ********************************************************************************
     函数名   : setVideoBitRates
     功能     : 设置视频压缩的码流
     参数     :  [IN] bitrates :  视频码流，kb/s
     ***************************************************************************/
    CCPAPI void setVideoBitRates(int bitrates);

    /*! @function
     ********************************************************************************
     函数名   : startRtpDump
     功能     : 保存Rtp数据到文件， 只能在通话过程中调用，如果没有调用stopRtpDump，通话结束后底层会自动调用
     参数     :  [IN] callid :  回话ID
                [IN] mediaType: 媒体类型， 0：音频 1：视频
                [IN] fileName: 文件名
                [IN] direction: 需要保存RTP的方向，0：接收 1：发送
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int startRtpDump(const char *callid, int mediaType, const char *fileName, int direction);

    /*! @function
     ********************************************************************************
     函数名   : stopRtpDump
     功能     : 停止保存RTP数据，只能在通话过程中调用。
     参数     :  [IN] callid :  回话ID
                [IN] mediaType: 媒体类型， 0：音频 1：视频
                [IN] direction: 需要保存RTP的方向，0：接收 1：发送
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int stopRtpDump(const char *callid, int mediaType, int direction);

	    /*! @function
     ********************************************************************************
     函数名   : getSpeakerInfo
     功能     : 获取扬声器信息，只在windows使用
     参数     :  [IN] speakerinfo：扬声器信息
     返回值   :  扬声器个数
     ***************************************************************************/
    CCPAPI int getSpeakerInfo(SpeakerInfo **speakerinfo);

	    /*! @function
     ********************************************************************************
     函数名   : selectSpeaker
     功能     : 选择扬声器，可以在通话过程中选择，只在windows使用;如果不选择扬声器，使用系统默认扬声器
     参数     :  [IN] speakerIndex : SpeakerInfo的index值，从getSpeakerInfo获取
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int selectSpeaker(int speakerIndex);

	    /*! @function
     ********************************************************************************
     函数名   : getMicroPhoneInfo
     功能     : 获取麦克风信息，只在windows使用
     参数     :  [IN] microphoneinfo :  麦克风信息
     返回值   :  麦克风个数
     *********************************************************************************/
    CCPAPI int getMicroPhoneInfo(MicroPhoneInfo** microphoneinfo);

	    /*! @function
     ********************************************************************************
     函数名   : stopRtpDump
	 功能     : 选择麦克风，可以在通话过程中选择，只在windows使用；如果不选择麦克风，使用系统默认麦克风
	 参数     :  [IN] microphoneIndex : MicroPhoneInfo的index值，从getMicroPhoneInfo获取
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int selectMicroPhone(int microphoneIndex);

	CCPAPI int getUniqueID(char *uniqueid, int len);

    /*! @function
     ********************************************************************************
     函数名   : setStunServer
	 功能     : 设置Stun Server。通过这个server获取公网地址。
	 参数     :  [IN] server : 服务器地址
     参数     :  [IN] port   : 服务端口，默认3478
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int setStunServer(const char *server, int port = 3478);

    /*! @function
     ********************************************************************************
     函数名   : setFirewallPolicy
	 功能     : 设置防火墙类型。
	 参数     :  [IN] policy : 防火墙类型。
                 typedef enum _CCPClientFirewallPolicy {
                 SerphonePolicyNoFirewall = 0,
                 SerphonePolicyUseIce
                 } CCPClientFirewallPolicy;
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int setFirewallPolicy(CCPClientFirewallPolicy policy);

    /*! @function
     ********************************************************************************
     函数名   : setShieldMosaic
	 功能     : 设置是否屏蔽视频解码过程中的马赛克。默认不屏蔽。
	 参数     :  [IN] flag : TRUE 屏蔽；FALSE 不屏蔽。
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int setShieldMosaic(bool flag);

    /*! @function
     ********************************************************************************
     函数名   : seRateAfterP2PSucceed
	 功能     : 视频通话且P2P成功后，为了取得更好的画质，可以将码流设置的高一些。默认330。
	 参数     :  [IN] rate : 码流，默认330。
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int seRateAfterP2PSucceed(int rate);

    /*! @function
     ********************************************************************************
     函数名   : startDeliverVideoFrame
	 功能     : 视频通话中，调用此函数，能够获取本地视频流数据。视频数据通过回调onDeliverVideoFrame上报。
	 参数     :  [IN] callid :  会话ID
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int startDeliverVideoFrame(const char *callid);

    /*! @function
     ********************************************************************************
     函数名   : stopDeliverVideoFrame
	 功能     : 视频通话中，停止获取本事视频流.这个函数不能在onDeliverVideoFrame回调函数中调用。
	 参数     :  [IN] callid :  会话ID
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int stopDeliverVideoFrame(const char *callid);

	    /*! @function
     ********************************************************************************
     函数名   : startRecordVoice
	 功能     :  开始通话录音，要在通话开始后调用。
	 参数     :  [IN] callid :  会话ID
				    [IN] filename: 录音文件名，带路径
     返回值   :  成功 0 失败 -1 账号不支持录音 -3
     *********************************************************************************/
	CCPAPI int startRecordVoice(const char *callid, const char *filename);

	    /*! @function
     ********************************************************************************
     函数名   : stopRecordVoice
	 功能     : 结束通话录音，如果没有调用，通话结束后会自动调用。
	 参数     :  [IN] callid :  会话ID
     返回值   :  成功 0 失败 -1  账号不支持录音 -3
     *********************************************************************************/
	CCPAPI int stopRecordVoice(const char *callid);

	    /*! @function
     ********************************************************************************
     函数名   : startRecordVoiceEx
	 功能     :  开始通话录音，要在通话开始后调用。
	 参数     :  [IN] callid :  会话ID
					[IN] rFileName: 远端录音文件名，带路径
				    [IN] lFileName: 本地录音文件名，带路径
     返回值   :  成功 0 失败 -1 账号不支持录音 -3
     *********************************************************************************/
	CCPAPI int startRecordVoiceEx(const char *callid, const char *rFileName, const char *lFileName);

    /*! @function
     ********************************************************************************
     函数名   : getLocalVideoSnapshot
	 功能     : 视频通话中，抓取本地视频截图
	 参数     :  [IN] callid :  会话ID
                [OUT] buf: 截图内容
                [OUT] size: 内容长度
                [OUT] width: 截图宽度
                [OUT] height: 截图高度
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int getLocalVideoSnapshot(const char *callid, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height);
    /*! @function
     ********************************************************************************
     函数名   : getLocalVideoSnapshot
	 功能     : 视频通话中，抓取远端视频截图
	 参数     :  [IN] callid :  会话ID
     [OUT] buf: 截图内容
     [OUT] size: 内容长度
     [OUT] width: 截图宽度
     [OUT] height: 截图高度
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int getRemoteVideoSnapshot(const char *callid, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height);


	    /*! @function
     ********************************************************************************
     函数名   : getLocalVideoSnapshot
	 功能     : 视频通话中，抓取本地视频截图，保存为JPG格式
	 参数     :  [IN] callid :  会话ID
                  [IN] fielName: 保存图片路径, UTF8
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int getLocalVideoSnapshotExt(const char *callid, const char *fielName);

    /*! @function
     ********************************************************************************
     函数名   : getLocalVideoSnapshot
	 功能     : 视频通话中，抓取远端视频截图，保存为JPG格式
	 参数     :  [IN] callid :  会话ID
				  [IN] fielName: 保存图片路径，UTF8
	 返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int getRemoteVideoSnapshotExt(const char *callid, const char *fielName);

	CCPAPI int startRecordRemoteVideo(const char *callid, const char *filename);
	CCPAPI int stopRecordRemoteVideo(const char *callid);

	CCPAPI int startRecordLocalVideo(const char *callid, const char *filename);
	CCPAPI int stopRecordLocalVideo(const char *callid);
	
    //CCPAPI int startRecordVoip(const char *callid, const char *filename);
    //CCPAPI int stopRecordVoip(const char *callid);

    /*! @function
     ********************************************************************************
     函数名   : noiseSuppression
	 功能     : 录音降噪。
	 参数     :  [IN] audioSamples :  录音数据
     [OUT] out: 降噪后的音频数据
     返回值   :  成功 0 失败 -1
     *********************************************************************************/
    CCPAPI int noiseSuppression(const void* audioSamples,short *out);

		   /*! @function
     ********************************************************************************
     函数名   : checkUserOnline
     功能     : 判断voip用户是否在线，同步返回结果，最长阻塞3秒
     参数     : [IN]  user :  voip账号
     返回值   : Account_Status
     *******************************************************************************/
	CCPAPI int checkUserOnline(const char *user);

	   /*! @function
     ********************************************************************************
     函数名   : getNetworkStatistic
     功能     : 获取通话的网络流量信息
     参数     : [IN]  callid : 会话id；
                [OUT] duration： 媒体交互的持续时间，单位秒，可能为0；
                [OUT] send_total：在duration时间内，网络发送的总流量，单位字节；
				[OUT] recv_total：在duration时间内，网络接收的总流量，单位字节；
     返回值   : 0：成功   -1：失败
     *******************************************************************************/
    CCPAPI int getNetworkStatistic(const char *callid, long long *duration, long long *send_total_sim, long long *recv_total_sim, long long *send_total_wifi, long long *recv_total_wifi);

    /*! @function
     ********************************************************************************
     函数名   : notifyVideoRotate
     功能     : 发送旋转度数（向左）
     参数     : [IN]  receiver : 接收者
     [IN]	 degree  : 向左旋转的度数（0度，90度，180度，270度）
     返回值   : 消息id
     *******************************************************************************/
	CCPAPI const char *notifyVideoRotate(const char *receiver, const char *degree);

    /*! @function
     ********************************************************************************
     函数名   : android_media_init_audio
     功能     : 此函数仅为android IM时初始化音频设备使用
     参数     : NO
     返回值   :
     *******************************************************************************/
	CCPAPI int android_media_init_audio();

    /*! @function
     ********************************************************************************
     函数名   : android_media_uninit_audio
     功能     : 此函数仅为android IM时释放音频设备使用
     参数     : NO
     返回值   :
     *******************************************************************************/
    CCPAPI int android_media_uninit_audio();

    /*! @function
     ********************************************************************************
     函数名   : SetAudioGain
     功能     : 可以控制输出和输入的媒体流进行一定量的放大。此接口是对底层接收和发送的数据进行放大或者缩小。改函数可以在通话过程中实时设置生效。
     参数     : [IN]  inaudio_gain : 对接受数据进行增益处理。
                [IN]  outaudio_gain : 对发送数据进行增益处
     返回值   : 0 成功， -1 失败
     *******************************************************************************/
    CCPAPI int setAudioGain(float inaudio_gain, float outaudio_gain);
		    /*! @function
     ********************************************************************************
     函数名   : setSpeakerVolume
     功能     : 设置语音音量的默认值，范围：【0~255】
     参数     : volume
     返回值   : 0：成功  -1：失败
     *******************************************************************************/
	CCPAPI int setSpeakerVolume(unsigned int volume);

		    /*! @function
     ********************************************************************************
     函数名   : getSpeakerVolume
     功能     : 获取语音音量的默认值，范围：【0~255】
     参数     : volume
     返回值   : 0：成功  -1：失败
     *******************************************************************************/
	CCPAPI unsigned int getSpeakerVolume();


	/*! @function
     ********************************************************************************
     函数名   : startRecordScreen
     功能     : 通话过程中，开始录屏
     参数     : [IN]  callid : 会话id；
                [IN] filename：录屏保存的文件名；
                [IN] bitrates：调节录屏压缩码流，默认640；
				[IN] fps：录屏的帧数，默认10帧每秒；
				[IN] type: 录屏的屏幕选择， 0：主屏 1：辅屏
     返回值   : 0：成功   -1：失败
     *******************************************************************************/
	CCPAPI int startRecordScreen(const char *callid, const char *filename, int bitrates, int fps, int type);

	CCPAPI int startRecordScreen(const char *callid, const char *filename, int bitrates, int fps, int type);

	/*! @function
     ********************************************************************************
     函数名   : stopRecordScreen
     功能     : 停止录屏，通话结束时，会主动调用本函数。
     参数     : [IN]  callid : 会话id；
     返回值   : 0：成功   -1：失败
     *******************************************************************************/
	CCPAPI int stopRecordScreen(const char *callid);

		/*! @function
     ********************************************************************************
     函数名   : getUsedCameraInfo
     功能     : 获取底层正在使用的摄像头信息和能力。初始化后，底层会有选择默认摄像头和能力；调用SelectCamera后，会改变底层正在使用的摄像头信息。
     参数     :  [OUT] cameraIndex :摄像头序列
				  [OUT] capabilityIndex　正在使用的能力
				  [OUT] maxFps   设置的最大视频帧率
				  [OUT] cameraRotate   设置的图像旋转
     返回值   : 0：成功   -1：失败
     *******************************************************************************/
	CCPAPI int getUsedCameraInfo(int *cameraIndex, int *capabilityIndex, int *maxFps, int *cameraRotate);


		/*! @function
     ********************************************************************************
     函数名   : resetVideoViews
     功能     : windows下，如果初始窗口太小，后面窗口变大时，渲染仍然按小窗口，画面就会模糊。通话过程中，
				 窗口大小改变或者重新设置其他窗口，调用这个接口，可以重置render。
     参数     :  [IN] callid : 呼叫ID
				  [IN] rWnd： 显示远端视频窗口句柄，可以为空
				  [IN] lWnd :  显示本地视频窗口句柄，可以为空
     返回值   : 0：成功   -1：失败
     *******************************************************************************/
	CCPAPI int resetVideoViews(const char *callid, void *rWnd, void *lWnd);

	CCPAPI void setBindLocalIP(const char* localip);

	CCPAPI int startVideoCapture(const char* callid);

    /*! @function
     ********************************************************************************
     函数名   : setSilkRate
     功能     : 设置silk码流。
     参数     :  [IN] rate : 码流（5000~20000）。
     返回值   : 0：成功   -1：失败
     *******************************************************************************/
    CCPAPI int setSilkRate(int rate);

	    /*! @function
     ********************************************************************************
     函数名   : PlayAudioFromRtpDump
     功能     :  开始播放rtpplay播放的音频数据
     参数     :  [IN] localPort：
					[IN] ptName :
					[IN] ploadType :
     返回值   : 0：成功   -1：失败
     *******************************************************************************/
	CCPAPI int PlayAudioFromRtpDump(int localPort, const char *ptName, int ploadType, int crypt_type, const char* key);
	    /*! @function
     ********************************************************************************
     函数名   : StopPlayAudioFromRtpDump
     功能     : 停止播放rtpplay播放的音频数据。
     返回值   : 0：成功   -1：失败
     *******************************************************************************/
	CCPAPI int StopPlayAudioFromRtpDump();

	    /*! @function
     ********************************************************************************
     函数名   : PlayVideoFromRtpDump
     功能     : 开始播放rtpplay播放的视频数据。。
	 参数     :  [IN] localPort：
					 [IN] ptName :
					 [IN] ploadType :
					 [IN] videoWindow ：
     返回值   : 0：成功   -1：失败
     *******************************************************************************/
	CCPAPI int PlayVideoFromRtpDump(int localPort, const char *ptName, int ploadType, void *videoWindow, int crypt_type, const char* key);

	    /*! @function
     ********************************************************************************
     函数名   : StopPlayVideoFromRtpDump
     功能     :  停止播放rtpplay播放的视频数据
     返回值   : 0：成功   -1：失败
     *******************************************************************************/
	CCPAPI int StopPlayVideoFromRtpDump();

		    /*! @function
     ********************************************************************************
     函数名   : startVideoWithoutCall
     功能     : 通话之外，打开本地视频, 为抓图做准备
	 参数     :   [IN] cameraIndex：
					 [IN] videoW :
					 [IN] videoH :
					 [IN] rotate ：
					 [IN] videoWnd:
     返回值   : 0：成功   -1：失败
     *******************************************************************************/
	CCPAPI int startVideoWithoutCall(int cameraIndex, int videoW, int videoH, int rotate, void *videoWnd);

		    /*! @function
     ********************************************************************************
     函数名   : stopVideoWithoutCall
     功能     : 通话之外，停止本地视频
     返回值   : 0：成功   -1：失败
     *******************************************************************************/
	CCPAPI int stopVideoWithoutCall();

		    /*! @function
     ********************************************************************************
     函数名   : getSnapshotWithoutCall
	 功能     : 通话之外，打开本地视频后，抓图，保存到本地
	 参数     :  [IN] filePath：
	 返回值   : 0：成功   -1：失败
	 *******************************************************************************/
	CCPAPI int getSnapshotWithoutCall(const char *filePath);

	CCPAPI void initHaiyuntongFailed();

		    /*! @function
     ********************************************************************************
     函数名   : setNackEnablded
	 功能     : 设置Nack重传功能的开启
	 参数     :  [IN] audioEnabled：音频重传
				   [IN] videoEnabled：视频重传
	 返回值   : 0：成功   -1：失败
	 *******************************************************************************/
    CCPAPI int setNackEnabled(bool audioEnabled, bool videoEnabled);
	CCPAPI int setVideoProtectionMode(int mode);

	CCPAPI int setP2PEnabled(bool enabled);
	CCPAPI int setRembEnabled(bool enabled);
	CCPAPI int setTmmbrEnabled(bool enabled);

	CCPAPI int setVideoMode(int videoModeIndex); //only for demo test 0: Real-time, 1:screen-share
	CCPAPI int setDesktopShareParam(int desktop_width, int desktop_height, int desktop_frame_rate, int desktop_bit_rate);
	 /*! @function
     ********************************************************************************
     函数名   : setVideoPacketTimeOut
	 功能     : 设置接收视频数据的超时时间，单位s， 参考事件：G_EVENT_VideoPacketTimeOut
	 参数     :  [IN] timeout：超时间隔
	 返回值   : 0：成功   -1：失败
	 *******************************************************************************/
	CCPAPI int setVideoPacketTimeOut(int timeout);

	/*! @function
     ********************************************************************************
     函数名   : getLocalIP
	 功能     : 获取当前的IP地址
	 参数     :  [IN] dst：连接的目标地址
				   [OUT] result： 获取到的IP地址
	 返回值   : 0：成功   -1：失败
	 *******************************************************************************/
	CCPAPI int getLocalIP(const char *dst, char *result);

		/*! @function
     ********************************************************************************
     函数名   : FixedCameraInfo
	 功能     : 指定使用的摄像头，如果和系统摄像头不匹配，启动摄像头失败
	 参数     :  [IN] cameraName： 指定摄像头名字， 可以为空，不能和cameraId同时为空
				   [IN] cameraId:   指定摄像头id，可以为空，不能和cameraName同时为空
				   [IN]  width:  指定宽，可以为0
 				   [IN]  height: 指定高，可以为0
	 返回值   : 0：成功   -1：失败
	 *******************************************************************************/
	CCPAPI int FixedCameraInfo(const char *cameraName, const char *cameraId, int width, int height);

		/*! @function
     ********************************************************************************
     函数名   : ConfigureChromaKey
	 功能     : 配置虚拟背景墙
	 参数     :  [IN] bgImage 虚拟背景图片， 支持jpg图片
				   [IN] angle： 抠图强度：允许范围0-360，推荐范围10-90
				   [IN] noise_level:  噪点消除：推荐范围10-80
				   [IN]  r 、g 、b： 真实背景墙rgb值
	 返回值   : 0：成功   -1：失败
	 *******************************************************************************/
	CCPAPI int ConfigureChromaKey(const char *bgImage, float angle, float noise_level, int r, int g, int b);

		/*! @function
     ********************************************************************************
     函数名   : StartVirtualBackGround
	 功能     : 开启虚拟背景墙功能
	 返回值   : 0：成功   -1：失败
	 *******************************************************************************/
	CCPAPI int StartVirtualBackGround();

		/*! @function
     ********************************************************************************
     函数名   : StopVirtualBakcGround
	 功能     : 结束虚拟背景墙功能
	 返回值   : 0：成功   -1：失败
	 *******************************************************************************/
	CCPAPI int StopVirtualBakcGround();

	//add by ylr
	/************************************************************************/
	/* 获取统计信息                                                                     */
	/************************************************************************/

	CCPAPI int GetStatsData(int type, char* callid, void** pbDataArray, int *pArraySize);
	CCPAPI int DeleteStatsData(void* pbDataArray);
	CCPAPI int GetBandwidthUsage(const char* callid,
								unsigned int& total_bitrate_sent,
								unsigned int& video_bitrate_sent,
								unsigned int& fec_bitrate_sent,
								unsigned int& nackBitrateSent);
	CCPAPI int GetEstimatedSendBandwidth(const char* callid,
								unsigned int* estimated_bandwidth);
	CCPAPI int GetEstimatedReceiveBandwidth(const char* callid,
								unsigned int* estimated_bandwidth);

	CCPAPI int GetReceiveChannelRtcpStatistics(const char* callid,
								_RtcpStatistics& basic_stats,
								__int64& rtt_ms);

	CCPAPI int GetSendChannelRtcpStatistics(const char* callid,
								_RtcpStatistics& basic_stats,
								__int64& rtt_ms);

	CCPAPI int GetRtpStatistics(const char* callid,
								_StreamDataCounters& sent,
								_StreamDataCounters& received);

	CCPAPI int GetSendStats(const char* callid, int &encode_frame_rate, int &media_bitrate_bps, int &width, int &height, bool &suspended);

	CCPAPI int StartRecord();
	CCPAPI int StopRecord();

#ifdef __cplusplus
}
#endif

#endif //_CCPCLIENT_H_
