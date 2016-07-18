package com.CCP.phone;

import java.util.HashMap;

import android.view.View;

import com.hisun.phone.core.voice.AbstractDispatcher;
import com.hisun.phone.core.voice.Device.CallType;

public class NativeInterface {

	public static void init() {
		//System.loadLibrary("crypto");
		//System.loadLibrary("VoipCpt");
		System.loadLibrary("serphone");
	}

	/**
	 * The client initialization
	 * @return
	 */
	public static native int initialize();

	@Deprecated
	public static native int connectToCCP(String proxy_addr, int proxy_port,
			String account, String password);
	
	/**
	 * Connect cloud communication platform based on the call control server.
	 * 
	 * @param proxy_addr The call control server address
	 * @param proxy_port The call control server port
	 * @param account The user account
	 * 		{@link AbstractDispatcher}
	 * @param password The user account password
	 * @param control Call control parameters
	 * @return If successfully initialized 0: successfully; 0 Non failure
	 */
	@Deprecated public static native int connectToCCP(String proxy_addr, int proxy_port,
			String account, String password , String control);
	
	/**
	 * @version 3.6.3
	 * @param xmlBody
	 * @param account
	 * @param password
	 * @param control
	 * @return
	 */
	public static native int connectToCCPWithXML(String xmlBody ,String account, String password , String control);
	
	/**
	 * 
	* <p>Title: disConnectToCCP</p>
	* <p>Description: Cloud communication SDK cancellation</p>
	* @return
	* @version 3.5
	 */
	public static native int disConnectToCCP();

	/**
	 * Set the user display name
	 * 
	 * @param name the user display name
	 * @return
	 */
	public static native int setUserName(String name);

	/**
	 * start a VoIP call request
	 * 
	 * @param type Call type {@link CallType} 0: audio 1: video
	 * @param called The called number
	 * 
	 * @return callid The caller location identification
	 * 		NULL On behalf of call failed.
	 */
	public static native String makeCall(int type, String called);

	/**
	 * release a VoIP call
	 * 
	 * @param callid Uniquely identifies the current call, 
	 * 		if callid is null, which represents all call
	 * @param reason
	 * @return
	 */
	public static native int releaseCall(String callid, int reason);

	/**
	 * Accept an VoIP call .
	 * 
	 * @param callid Uniquely identifies the current call
	 * @return 0: successfully; 0 Non failure
	 */
	public static native int acceptCall(String callid);

	/**
	 * Reject an VoIP call .
	 * 
	 * @param callid Uniquely identifies the current call
	 * @param reason The reasons of Reject
	 * @return The success of 0: successfully; 0 Non failure
	 */
	public static native int rejectCall(String callid, int reason);

	/**
	 * Hang up an VoIP call 
	 * 
 	 * @param callid Uniquely identifies the current call
	 * @return The success of 0: successfully; 0 Non failure
	 */
	public static native int pauseCall(String callid);

	/**
	 * Resume an VoIP call .
	 * 
	 * @param callid Uniquely identifies the current call
	 * @return The success of 0: successfully; 0 Non failure
	 */
	public static native int resumeCall(String callid);

	/**
	 * Transfer an VoIP call .
	 * 
	 * @param callid Uniquely identifies the current call
	 * @param destionation The called number.
	 * @return The success of 0: successfully; 0 Non failure
	 */
	public static native int transferCall(String callid, String destionation ,int callType);

	/**
	 * Cancle an VoIP call . has not realized the no use
	 * @param callid Uniquely identifies the current call
	 * @return
	 * @deprecated
	 */
	public static native int cancelCall(String callid);

	/**
	 * Send DTMF message
	 * 
	 * @param callid Uniquely identifies the current call
	 * @param dtmf Key values,'0'-'9'' * ''#'
	 * @return The success of 0: successfully; 0 Non failure
	 */
	public static native int sendDTMF(String callid, char dtmf);

	/**
	 * Send an text message.
	 * 
	 * @param receiver Recipient of The text message 
	 * @param message The content of text message
	 * @param userdata User defined data
	 * @return The success of 0: successfully; 0 Non failure
	 */
	public static native String sendTextMessage(String receiver, String message, String userdata);

	/**
	 * Get phone state
	 * @param callid Uniquely identifies the current call
	 * @return The success of 0: successfully; 0 Non failure
	 */
	public static native int getCallState(String callid);

	/**
	 * Client logoff
	 * @return 
	 */
	public static native int unInitialize();

	/**
	 * Method for setting the callback function
	 * 
	 * @param obj Implementation of the callback class
	 * @param method Method name
	 * @param methodSig Method sig
	 */
	public static native void setCallBackParams(Object obj, String method,
			String methodSig);

	/**
	 * Set the logging level.
	 * 
	 * @param level 1: open the log 
	 * 				0: log is closed
	 */
	public static native void setLogLevel(int level);

	/**
	 * Setting the speaker status
	 * 
	 * @param enable True opens , the false closed
	 * @return
	 */
	public static native int enableLoudsSpeaker(boolean enable);

	/**
	 * Gets the speaker state
	 * 
	 * @return True opens false closed
	 */
	public static native boolean getLoudsSpeakerStatus();

	/**
	 * Set the phone Mute state .
	 * 
	 * @param on True opens , the false closed
	 * @return
	 */
	public static native int setMute(boolean on);

	/**
	 * Get the mute state
	 * @return True opens , the false closed
	 */
	public static native boolean getMuteStatus();

	/**
	 * Set the tone.
	 * 
	 * @param filename The sound file path
	 * @return 
	 */
	public static native int setRing(String filename);

	/**
	 * Set the ring back tone.
	 * 
	 * @param filename The sound file path
	 * @return
	 */
	public static native int setRingback(String filename);

	/**
	 * Gets the current call.
	 * 
	 * @return Uniquely identifies the current call
	 */
	public static native String getCurrentCall();

	/**
	 * The user sets the custom data.
	 * 
	 * @param type A custom value
	 * @param userData The custom string
	 * @return
	 */
	public static native int setUserData(int type, String userData);

	/**
	 * Get the user sets the custom data.
	 * 
	 * @param type A custom value
	 * @return userData The custom string
	 */
	public static native String getUserData(int type);

	/**
	 * Set the audio device context
	 * @param obj Context handle
	 * @return
	 */
	public static native int setAudioContext(Object obj);

	/**
	 * Set the network type.
	 * 
	 * @param type Network type 2G, 3G WIFI
	 * @param connected Whether connected
	 * @param reconnect Whether reconnection
	 */
	public static native void setNetworkType(int type, boolean connected,
			boolean reconnect);

	/**
	 * Set the video call view.
	 * 
	 * @param view The other video view
	 * @param localview The local video view
	 * @return
	 */
	public static native int setVideoView(Object view, Object localview);

	/**
	 * Choose the camera equipment.
	 * 
	 * @param cameraIndex The camera device index value pre or post
	 * 		{@link CameraInfo}
	 * @param capabilityIndex Capability index
	 * @param fps Frame rate
	 * @param rotate The degree of 0: rotation of the default 
	 * 				1: rotating 0 degrees 
	 * 				2: rotating 90 degrees 
	 * 				3: rotated 180 degrees
	 * 				4: rotated 270 degrees
	 * @param force
	 * @return
	 * 
	 * @version 3.5 add force
	 */
	public static native int selectCamera(int cameraIndex, int capabilityIndex,
			int fps, int rotate , boolean force);

	/**
	 * Get device camera information.
	 * 
	 * @return The camera device information array
	 */
	public static native CameraInfo[] getCameraInfo();

	/**
	 * Set the encoding and decoding method of support, the default all support.
	 * 
	 * @param type Codec type.0 iLBC, 1 G729,2 PCMU, 3 PCMA, 4 VP8,5 H264
	 * @param enabled 0 does not support; 
	 * 				1 support
	 * @return Always returns -1
	 */
	public static native int setCodecEnabled(int type, boolean enabled);
	public static native boolean getCodecEnabled(int type);

	/**
	 * Set the SRTP encryption attribute.
	 * 
	 * @param tls True set the signaling encryption; 
	 * 			  false does not set the signaling encryption
	 * @param srtp True STRP false SRTP does not set the encryption encryption
	 * @param userMode True user mode; false standard mode
	 * @param cryptType AES_256_SHA1_80=3;AES_256_SHA1_32=4
	 * @param key Encryption and decryption keys (no less than 46 bytes in length)
	 * @return
	 */
	public static native int setSrtpEnabled(boolean tls, boolean srtp,
			boolean userMode, int cryptType, String key);

	/**
	 * To accept the call, can select the media type.
	 * 
	 * @param callid Uniquely identifies the current call
	 * @param type 1 audio and video; 0 audio {@link CallType}
	 * @return The success of 0: successfully; 0 Non failure
	 */
	public static native int acceptCallByMediaType(String callid, int type);

	/**
	 * Request update existing call media type.
	 * 
	 * @param callid Uniquely identifies the current call
	 * @param request 1 audio and video; 0 audio {@link CallType}
	 * @return The success of 0: successfully; 0 Non failure
	 */
	public static native int updateCallMedia(String callid, int request);

	/**
	 * Accept other update request.
	 * 
	 * @param callid Uniquely identifies the current call
	 * @param action 1:Accept , 0: refused
	 * @return
	 */
	public static native int answerCallMediaUpdate(String callid, int action);

	/**
	 * Gets the call media type.
	 * 
	 * @param callid Uniquely identifies the current call
	 * @return The failure of -1, 0:audio , 1: video
	 */
	public static native int getCallMeidaType(String callid);

	/**
	 * Set switch audio processing, before make the call.
	 *  
	 * @param type Audio processing type. Enum {AUDIO_AGC, AUDIO_EC, AUDIO_NS};
	 * @param enabled The AGC is turned off by default; EC and NS by default on
	 * @param mode The corresponding mode: AgcMode, EcMode, NsMode
	 * @return The failure of -1, 0 success.
	 */
	public static native int setAudioConfig(int type, boolean enabled, int mode);

	/**
	 * Get to audio type if enabled or not.
	 * @param type Audio processing type. Enum {AUDIO_AGC, AUDIO_EC, AUDIO_NS};
	 * @return True enabled; false is not enabled
	 */
	public static native boolean getAudioConfig(int type);

	/**
	 * Get the mode of audio types that enabled.please call getAudioConfig functions before, 
	 * confirmation has enabled this type.
	 * 
	 * @param type Audio processing type. Enum {AUDIO_AGC, AUDIO_EC, AUDIO_NS};
	 * @return The corresponding mode: AgcMode, EcMode, NsMode
	 */
	public static native int getAudioConfigMode(int type);

	/**
	 * Set the video compressed stream, in a call before make the call.
	 * 
	 * @param bitrates Video, kb/s
	 */
	public static native void setVideoBitRates(int bitrates);

	/**
	 * Save the Rtp data to a file. only call the method during a call,
	 * if you do not call stopRtpDump, it will calling automatically when call end
	 * in C/C++.
	 * 
	 * @param callid  Uniquely identifies the current call
	 * @param mediaType Call type {@link CallType} 0: audio 1: video
	 * @param fileName file name.
	 * @param direction The need to save the direction of RTP, 0:Receive , 1: send 
	 * @return
	 */
	public static native int startRtpDump(String callid, int mediaType,
			String fileName, int direction);

	/**
	 * Stop the save RTP data, can only be invoked during a call.
	 * 
	 * @param callid Uniquely identifies the current call
	 * @param mediaType Call type {@link CallType} 0: audio 1: video
	 * @param direction The need to save the direction of RTP, 0:Receive , 1: send
	 * @return
	 */
	public static native int stopRtpDump(String callid, int mediaType,
			int direction);

	
	
	
	
	// --------------------------------------------------------
	// AMR encode method ...
	public static native int AmrNBCreateEnc();

	public static native int AmrNBCreateDec();

	public static native int AmrNBFreeEnc();

	public static native int AmrNBFreeDec();

	public static native int AmrNBEncode(byte[] input, int len, byte[] output, int mode);

	public static native int AmrNBEncoderInit(int dtxMode);

	public static native int AmrNBDecode(byte[] encoded, int len, byte[] decoded);

	public static native String AmrNBVersion();
	
	public static native String GetUniqueID();
	
	// -------------------------------------------------------------------
	// for SDK version 3.3
	// 
	/**
	 * the function name: getVersion (for SDK version 3.3)
	 * function: get SDK version information
	 * the return value: String Version information.
	 * Note: the version information format: 
	 * 		 "version number # platform version of #ARM # audio switch # video switch # compile date compilation time"
	 * version number: X.X.X format such as 1.1.18
	 * platform: Android, Windows, iOS, Mac, OS, Linux
	 * ARM version: arm, armv7, armv5
	 * audio switch: voice=false, voice=true
	 * the video switch: video=false, video=true
	 * compile date: "MM DD YYYY" "Jan 192013"
	 * compile time: "hh:mm:ss" as "08:30:23")
	 *
	 * @return
	 */
	public static native String getVersion();
	
	/**
	 * Function name: getCallStatistics
	 * Function：Obtain statistical data call
	 * 			Access to statistical information communication, according to the packet loss rate and delay time, 
	 * 			judgment of communication network. Network traffic statistics can also be call.
	 *          
	 *          to obtain statistics of interval at least 4 seconds. In the statistical information, 
	 *          it is important to the packet loss rate and delay time, can reflect the network status.
	 * @param type
	 * @return
	 */
	public static native String getCallStatistics(int type);
	// -------------------------------------------------------------------
	// for SDK version 3.4
	// Set Stun Server. Network access by this server address.
	/**
	 * Function: set Stun Server. Network access by this server address.
	 * Parameters: [IN] server: the server address
	 * Parameters: [IN] port: the service port, the default 3478
	 * Return value: 0 successful failure -1
	 */
	public static native int SetStunServer(String server);
	// Custom IP and port
	public static native int SetStunServer(String server, int port);
	
	/**
	 * Function name: setFirewallPolicy
	 * setting up a firewall types.
	 * @param policy
	 * 	Parameters: [IN] policy: types of firewall.
	 * @return value: 0 successful failure -1
	 */
	public static native int SetFirewallPolicy(int policy);
	
	/**
	 * Set whether shielded video decoding process of mosaic. The default not shield.
	 * @version 3.4.1.1
	 * @param flag TRUE FALSE not shield shield. 
	 * @return 0 successful failure -1
	 */
	public static native int SetShieldMosaic(boolean flag);
	
	/**
	 * P2P video calling and success, in order to achieve better image quality, 
	 * this time can be set larger stream (depending on the testing has a best value). 
	 * The default is 330.
	 * @version 3.4.1.1
	 * @param rate  P2P is set after the success of the stream, the default 330.
	 * @return 0 successful failure -1
	 */
	public static native int SetStartBitRateAfterP2PSucceed(int rate);
	
	/**
	 * 
	* <p>Title: startDeliverVideoFrame</p>
	* <p>Description: </p>
	* @param callid
	* @return
	* @version 3.5
	* 
	* @deprecated
	 */
	public static native int startDeliverVideoFrame(String callid);
	
	@Deprecated
	public static native int stopDeliverVideoFrame(String callid);
	
	/**
	 * Get local video picture data
	 * @param callid
	 * @version 3.6
	 */
	public static native Object getLocalVideoSnapshot(String callid);
	
	/**
	 *  Get remote video picture data
	 * @param callid
	 * @version 3.6
	 */
	public static native Object getRemoteVideoSnapshot(String callid);
	
	/**
	 * start recording voice when in Voice Call,
	 * @param callid current voice calls id.
	 * @param fileName The Recording File local path .
	 * @return
	 */
	public static native int startRecordVoice(String callid , String fileName);
	
	/**
	 * start recording voice when in Video Call,
	 * @param callid current video calls id.
	 * @param fileName The Recording File local path . 
	 * @return
	 */
	public static native int startRecordVoip(String callid , String fileName);
	
	/**
	 * Stop voice recording of Voice Calls
	 * @param callid current voice calls id.
	 * @return
	 */
	public static native int stopRecordVoice(String callid);
	
	/**
	 * Stop voice recording of Voice Calls
	 * @param callid current Video calls id.
	 * @return
	 */
	public static native int stopRecordVoip(String callid);
	
	/**
	 * Whether to open log switch int C/C++ code.
	 * @param enabled
	 */
	public static native void setTraceFlag(boolean enabled);
	
	/**
	 * check online state of remout account.
	 * @param account
	 * @return 
	 */
	public static native int checkUserOnline(String account);
	
	/**
	 * 获取通话的网络流量信息
	 * @param callid
	 * @return <duration>%lld</duration><send>%lld</send><recv>%lld</recv>"
	 * 			duration： 媒体交互的持续时间，单位秒，可能为0；
	 *			send_total：在duration时间内，网络发送的总流量，单位字节；
	 *			recv_total：在duration时间内，网络接收的总流量，单位字节；
	 */
	public static native String getNetworkStatistic(String callid);
	
	/**
	 * 设置允许上层处理音频数据
	 * @param callid 当前呼叫的唯一标识.
	 * @param flag true 允许上层处理； false 不允许上层处理。
	 * @return 成功 0 失败-1
	 */
	public static native int setProcessDataEnabled(String callid, boolean flag);
	
	/**
	 * 设置允许上层处理音频数据
	 * @param callid 当前呼叫的唯一标识.
	 * @param flag true 允许上层处理； false 不允许上层处理。
	 * @return 成功 0 失败-1
	 */
	public static native int setProcessOriginalDataEnabled(String callid, boolean flag);
	
	
	public static native int InitAudioDevice();
	public static native int UNInitAudioDevice();
	
	/**
	 *  两个参数都是0， 表示关掉keepAlive
	 * @param wifi
	 * @param mobile
	 * @return 
	 */
	public static native int setKeepAliveTimeout(int wifi , int mobile);
	
	/*! @function
     ********************************************************************************
	     函数名   : setRootCAPath
	     功能     : 根证书设置接口
	     参数     : [IN]  caPath   : 根证书路径.
	     返回值   : 成功 0 失败 -1
     *******************************************************************************/
	public static native int setRootCAPath(String caPath);
	
	 /*! @function
     ********************************************************************************
	     函数名   : setClientCertPath
	     功能     : 客户端证书设置接口
	     参数     : [IN]  certPath   : 客户端证书路径.
	     返回值   : 成功 0 失败 -1
     *******************************************************************************/
	public static native int setClientCertPath(String certPath);
	
	 /*! @function
     ********************************************************************************
	     函数名   : setClientKeyPath
	     功能     : 客户端秘钥设置接口
	     参数     : [IN]  keyPath   : 客户端秘钥路径.
	     返回值   : 成功 0 失败 -1
     *******************************************************************************/
	public static native int setClientKeyPath(String keyPath);
	
	
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
	public static native int setTlsSrtpEnabled(boolean tls , boolean srtp , int cryptType , String key);
	
	/**
	 *  私有云校验接口
	 * @param companyID  企业ID.
	 * @param restAddr 软交换地址.
	 * @param nativeCheck 是否本地校验.
	 * @return 成功 0 -1 companyID过长（最大199） -2 restAdd过长（99）
	 */
	public static native int setPrivateCloud(String companyID , String restAddr , boolean nativeCheck);
	
	
	
	/*! @function
     ********************************************************************************
     	函数名   : initAudioDevice
     	功能     : 初始化音频设备
     	返回值   : 成功 0 失败 -1
     *******************************************************************************/
    public static native int registerAudioDevice();
    
    /*! @function
     ********************************************************************************
     	函数名   : uninitAudioDevice
     	功能     : 释放音频设备
     	返回值   : 成功 0 失败 -1
     *******************************************************************************/
    public static native int deregisterAudioDevice();
    
    /**
     * 静音检测是否开启
     * @param enabled
     * @return
     */
    public static native int setDtxEnabled(boolean enabled);
	
	/**
	 * ip路由功能,设置网络组ID
	 * @param groupId
	 * @return  成功 0 失败 -1
	 */
	public static native int SetNetworkGroupId(String groupId);
	
	/**
	 * 设置视频会议服务器地址
	 * @param ip 视频会议服务器ip.
	 * @return
	 */
	public static native int SetVideoConferenceAddr(String ip);
	
	/**
	 * 视频会议中请求某一远端视频
	 * @param conferenceNo 所在会议号
	 * @param conferencePasswd 所在会议密码
	 * @param remoteSipNo  请求远端用户的sip号
	 * @param videoWindow 当成功请求时，展示该成员的窗口
	 * @param port 当前请求的目的端口
	 * @return
	 */
	public static native int requestMemberVideo(String conferenceNo , String conferencePasswd , String remoteSipNo , View videoWindow , int port , HashMap<String, Object> map);
	
	/**
	 * 视频会议中停止某一远端视频
	 * @param conferenceNo 所在会议号
	 * @param conferencePasswd 所在会议密码
	 * @param remoteSipNo 请求远端用户的sip号
	 * @return
	 */
	public static native int stopMemberVideo(String conferenceNo , String conferencePasswd , String remoteSipNo , HashMap<String, Object> map);
	public static native int resetVideoConfWindow(String sipNo , View videoWindow);
	
	public static native int startRecord(String callid);

    public static native int stopRecord(String callid);
}
