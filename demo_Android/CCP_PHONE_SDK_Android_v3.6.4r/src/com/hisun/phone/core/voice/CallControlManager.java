/**
 * 
 */
package com.hisun.phone.core.voice;

import android.content.Context;
import android.os.Bundle;
import android.os.Environment;
import android.os.Message;
import android.text.TextUtils;
import android.view.View;

import com.CCP.phone.CCPCallEvent;
import com.CCP.phone.CameraInfo;
import com.CCP.phone.NativeInterface;
import com.hisun.phone.core.voice.Device.AudioMode;
import com.hisun.phone.core.voice.Device.AudioType;
import com.hisun.phone.core.voice.Device.CallType;
import com.hisun.phone.core.voice.model.DownloadInfo;
import com.hisun.phone.core.voice.model.setup.UserAgentConfig;
import com.hisun.phone.core.voice.model.videoconference.VideoPartnerPortrait;
import com.hisun.phone.core.voice.net.HttpManager;
import com.hisun.phone.core.voice.opts.ConferenceOptions;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.SdkErrorCode;
import com.hisun.phone.core.voice.util.VoiceUtil;

import java.util.ArrayList;

/**
 * This class just receive low layer message.
 * 
 */
public final class CallControlManager extends AbstractDispatcher {

	private static CallControlManager instance;

	private CallControlManager(Context context) {
		super(context);
		Log4Util.v(Device.TAG, "constructor thread: "
				+ Thread.currentThread().getName());

		setTraceFlag(true);
		// NativeInterface.setCallBackParams(this, "eventCallBack",
		// "(ILjava/lang/String;Ljava/lang/String;I)V");
		NativeInterface.setCallBackParams(this, "eventCallBack",
				"(ILjava/lang/String;[BI)Ljava/lang/Object;");

		NativeInterface.setAudioContext(context);

		NativeInterface.initialize();

		NativeInterface.setLogLevel(2);
		// amr encode
		NativeInterface.AmrNBCreateEnc();
		NativeInterface.AmrNBEncoderInit(0);

		// 默认开启开启静音检测 和噪声抑制
		NativeInterface.setDtxEnabled(true);
		NativeInterface.setAudioConfig(AudioType.AUDIO_EC.getValue(), true,
				AudioMode.kEcConference.getValue());
	}

	/**
	 * 实例化CallControlManager
	 * 
	 * @param context
	 *            context实例
	 * @return 返回CallControlManager实例
	 */
	public static CallControlManager initialize(Context context) {
		if (instance == null) {
			instance = new CallControlManager(context);
		}
		return instance;
	}

	/**
	 * 初始化SDK设置，并连接云通讯服务器
	 * 
	 * @param listener
	 *            设置监听句柄
	 * @param userAgent
	 *            设置用户agent
	 * @throws Exception
	 */
	void init(CCPCallEvent listener, UserAgentConfig userAgent)
			throws Exception {
		Log4Util.d(Device.TAG, "init thread: "
				+ Thread.currentThread().getName());

		setSSLPort(userAgent.getUa_port());
		setListener(listener);

		setUserAgentConfig(userAgent);
		setUserData(CCPCallEvent.USERDATA_FOR_USER_AGENT, userAgent.getUa());


        Log4Util.e(Device.TAG, "get storage path: " +
        Environment.getExternalStorageDirectory().getAbsolutePath() + " canwrite:" +
                        Environment.getExternalStorageDirectory().canWrite());

		// get softswitch address, just execute subthread

		if (!userAgent.getPassword().isEmpty()) {
			NativeInterface.connectToCCP(userAgent.getUa_server(), userAgent.getUa_port(),
					userAgent.getSid(), userAgent.getPassword(), "");
		} else {
			doQuerySoftSwitchAddress(userAgent.getPrivateCloud(),
					userAgent.getSid(), userAgent.getSubaccountid(),
					userAgent.getSubpassword());
		}


//		if (userAgent.getPassword().compareTo("1234") == 0) {
//			NativeInterface.connectToCCP("114.215.241.49", 8600,
//					userAgent.getSid(), "yuntongxun_pass3wd", "");
//		} else if (userAgent.getPassword().compareTo("yuntongxun_pass3wd") == 0) {
//            NativeInterface.connectToCCP("118.194.243.237", 7600,
//                    userAgent.getSid(), userAgent.getPassword(), "");
//        }else {
//			doQuerySoftSwitchAddress(userAgent.getPrivateCloud(),
//					userAgent.getSid(), userAgent.getSubaccountid(),
//					userAgent.getSubpassword());
//		}
	}

	private static final int EVENT_MESSAGE = 0x00;
	public static final int EVENT_CONNECT = 0x01;
	private static final int EVENT_DISCONNECT = 0x02;
	private static final int EVENT_CALL = 0x03;
	private static final int EVENT_DTMF_RECEIVED = 0x04;
	private static final int EVENT_GET_CAPABILITY_TOKEN = 0x05;
	private static final int EVENT_LOG_INFO = 0x06;
	private static final int EVENT_MESSAGE_REPORT = 0x07;
	private static final int EVENT_GENERAL = 0x08;
	private static final int EVENT_STATE_CALL_VIDEO_RATIO = 0x12;

	/**
	 * for Video Conference 3.5
	 */
	@Deprecated
	private static final int EVENT_DELIVER_VIDEO_FRAME = 0x0a;
	private static final int EVENT_RECORD_CALL_VOICE = 0x0b;
	private static final int EVENT_AUDIO_DATA_PROCESS = 0x0c;
	private static final int EVENT_TRANSFER_STATE_STATE_SUCCESS = 0x0d;
	private static final int EVENT_ORIGINAL_AUDIO_DATA_PROCESS = 0x0e;
	// 视频会议时，请求视频数据失败
	private static final int EVENT_REQUEST_VIDEO_RESULT = 0x0f;
	private static final int EVENT_ENABLE_SRTP = 0x10;
	// 视频会议时，取消视频数据响应
	private static final int EVENT_STOP_VIDEO_RESPONSE = 0x11;

	private static final int GENERAL_EVENT_STATE_EARLYMEDIA = 0x00;
	private static final int GENERAL_EVENT_STATE_MESSAGE_COMMAND = 0x01;
	private static final int GENERAL_EVENT_STATE_REMOTE_VIDEO_RATIO = 0x02;
	private static final int GENERAL_EVENT_STATE_MEDIA_INIT_FAILED = 0x03;
	private static final int GENERAL_EVENT_STATE_AUDIOD_ESTINATION = 0x04;
	private static final int GENERAL_EVENT_STATE_VIDEOD_ESTINATION = 0x05;

	private static final int CALL_STATE_MAKE_CALL_FAILED = 0x01;
	private static final int CALL_STATE_CALL_INCOMING = 0x02;
	private static final int CALL_STATE_CALL_ALERTING = 0x03;
	private static final int CALL_STATE_CALL_ANSWERED = 0x04;
	private static final int CALL_STATE_CALL_PAUSED = 0x05;
	private static final int CALL_STATE_CALL_REMOTE_PAUSED = 0x06;
	private static final int CALL_STATE_CALL_RELEASED = 0x07;
	private static final int CALL_STATE_CALL_TRANSFERRED = 0x08;
	private static final int CALL_STATE_CALL_VIDEO = 0x09;
	private static final int CALL_STATE_CALL_PROCEEDING = 0x0a;
	private static final int CALL_STATE_CALL_VIDEO_UPDATE_REQUEST = 0x0b;
	private static final int CALL_STATE_CALL_VIDEO_UPDATE_RESPONSE = 0x0c;

	/**
	 * SDK中消息回调函数
	 * 
	 * @param event
	 *            消息类型
	 * @param id
	 *            消息id
	 * @param message
	 *            消息内容
	 * @param state
	 *            消息子类型
	 */
	public Object eventCallBack(int event, String id, byte[] message, int state) {
		if (message != null) {
			Log4Util.d(Device.TAG,
					"[CallControllerManager - eventCallBack] event： " + event
							+ " , length : " + message.length + " ,state: "
							+ state);
		}
		try {
			Log4Util.d(Device.TAG, "[CallControllerManager - eventCallBack] "
					+ " event: " + event + ", id: " + id + ", message: "
					+ (message != null ? new String(message) : null)
					+ ", state: " + state);

		} catch (Exception e) {
		}

		switch (event) {
		case EVENT_LOG_INFO:
			break;
		case EVENT_GET_CAPABILITY_TOKEN:
			break;

		case EVENT_AUDIO_DATA_PROCESS:

			return OnCallProcessDataEvent(message, state);
		case EVENT_ORIGINAL_AUDIO_DATA_PROCESS:
			OnProcessOriginalDataEvent(message);
			break;
		case EVENT_REQUEST_VIDEO_RESULT:
			// 视频会议时，请求视频数据失败
			getListener().onRequestConferenceMemberVideoFailed(state, id,
					new String(message));
			break;
		case EVENT_STOP_VIDEO_RESPONSE:
			// 视频会议时，取消视频数据响应
			getListener().onCancelConferenceMemberVideo(state, id,
					new String(message));
			break;
		case EVENT_ENABLE_SRTP:
			getListener().OnTriggerSrtp(id, state == 1 ? true : false);
			break;
		case EVENT_TRANSFER_STATE_STATE_SUCCESS:
			getListener().onTransferStateSucceed(id, state == 1 ? true : false);
			break;
		default:
			if (message == null) {
				callbackEvent(event, id, null, state);
				return null;
			}
			callbackEvent(event, id, new String(message), state);
			break;
		}
		return null;
	}

	/**
	 * SDK中消息回调函数
	 * 
	 * @param event
	 *            消息类型
	 * @param id
	 *            消息id
	 * @param message
	 *            消息内容
	 * @param state
	 *            消息子类型
	 */
	public void _eventCallBack(int event, String id, String message, int state) {
		Log4Util.d(Device.TAG, "[CallControllerManager - eventCallBack] "
				+ " event: " + event + ", id: " + id + ", message: " + message
				+ ", state: " + state);
		switch (event) {
		case EVENT_LOG_INFO:
			if (getListener() != null) {
				// getListener().onLogInfo(message);
			}
			break;
		case EVENT_GET_CAPABILITY_TOKEN:
			Log4Util.d(Device.TAG, "[CallControllerManager - eventCallBack] "
					+ " event: " + event + ", id: " + id + ", message: "
					+ message + ", state: " + state);
			break;
		default:
			callbackEvent(event, id, message, state);
			break;
		}
	}

	/**
	 * 内部回调函数
	 * 
	 * @param event
	 *            消息类型
	 * @param id
	 *            消息id
	 * @param message
	 *            消息内容
	 * @param state
	 *            消息子类型
	 */
	private void callbackEvent(final int event, final String id,
			String message, final int state) {
		try {
			switch (event) {
			case EVENT_MESSAGE:
				getListener().onTextMessageReceived(id, message);
				break;
			case EVENT_CONNECT:
				if (state == 0) {
					getListener().onConnected();
					checkSimState();
				} else {
					getListener().onConnectError(state);
				}
				break;
			case EVENT_DISCONNECT:
				getListener().onDisconnect();
				break;
			case EVENT_CALL:
				onCallEvent(id, message, state);
				break;
			case EVENT_DTMF_RECEIVED:
				getListener().onDtmfReceived(id, (char) state);
				break;
			case EVENT_MESSAGE_REPORT:
				if (TextUtils.isEmpty(message)) {
					getListener().onMessageSendReport(id, state);
				} else {
					// for versoin 3.4.1.1
					getListener().onMessageSendReport(id,
							VoiceUtil.getTextReceiveDate(message), state);
				}
				break;
			case EVENT_GENERAL:
				onGeneralEvent(id, message, state);
				break;
			case EVENT_STATE_CALL_VIDEO_RATIO:
				if (getListener() == null) {
					return;
				}
				getListener().onCallVideoRatioChanged(
						id,
						message.substring(message.indexOf("0:") + 1,
								message.length()), state);

				break;
			case EVENT_RECORD_CALL_VOICE:

				if (getListener() != null)
					getListener().onCallRecord(id, message, state);
				break;
			default:
				break;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/**
	 * 统一处理GeneralEvent消息
	 * 
	 * @param callid
	 * @param message
	 * @param state
	 * @throws Exception
	 */
	private void onGeneralEvent(String callid, String message, int state)
			throws Exception {
		switch (state) {
		case GENERAL_EVENT_STATE_EARLYMEDIA:
			break;
		case GENERAL_EVENT_STATE_MESSAGE_COMMAND:
			byte[] data = new byte[8];
			System.arraycopy(message.getBytes(), 0, data, 0, data.length);
			message = formatXml(message);
			getListener().onPushMessageArrived(message);
			break;
		case GENERAL_EVENT_STATE_REMOTE_VIDEO_RATIO:
			// getListener().onCallVideoRatioChanged(callid,
			// message.substring(message.indexOf("0:")+2, message.length()));
			break;
		case GENERAL_EVENT_STATE_MEDIA_INIT_FAILED:
			getListener()
					.onCallMediaInitFailed(
							callid,
							Integer.parseInt(message.substring(0,
									message.indexOf(":"))));

			break;
		case GENERAL_EVENT_STATE_AUDIOD_ESTINATION:
		case GENERAL_EVENT_STATE_VIDEOD_ESTINATION:
			// P2P provided a success callback callback, failed can't callback ,
			getListener().onFirewallPolicyEnabled();
			break;
		default:
			break;
		}
	}

	/**
	 * 统一处理CallEvent消息
	 * 
	 * @param callid
	 * @param message
	 * @param state
	 * @throws Exception
	 */
	private void onCallEvent(String callid, String message, int state)
			throws Exception {
		switch (state) {
		case CALL_STATE_MAKE_CALL_FAILED:
			this.getListener().onMakeCallFailed(callid,
					Integer.parseInt(message));
			break;
		case CALL_STATE_CALL_INCOMING:
		case CALL_STATE_CALL_VIDEO:
			this.getListener().onIncomingCallReceived(
					state == CALL_STATE_CALL_VIDEO ? CallType.VIDEO
							: CallType.VOICE, callid, message);
			break;
		case CALL_STATE_CALL_ALERTING:
			this.getListener().onCallAlerting(callid);
			break;
		case CALL_STATE_CALL_ANSWERED:
			this.getListener().onCallAnswered(callid);
			break;
		case CALL_STATE_CALL_PAUSED:
			this.getListener().onCallPaused(callid);
			break;
		case CALL_STATE_CALL_REMOTE_PAUSED:
			this.getListener().onCallPausedByRemote(callid);
			break;
		case CALL_STATE_CALL_RELEASED:
			this.getListener().onCallReleased(callid);
			break;
		case CALL_STATE_CALL_TRANSFERRED:
			this.getListener().onCallTransfered(callid, message);
			break;
		case CALL_STATE_CALL_PROCEEDING:
			this.getListener().onCallProceeding(callid);
			break;
		case CALL_STATE_CALL_VIDEO_UPDATE_REQUEST:
			this.getListener().onCallMediaUpdateRequest(callid,
					Integer.parseInt(message));
			break;
		case CALL_STATE_CALL_VIDEO_UPDATE_RESPONSE:
			this.getListener().onCallMediaUpdateResponse(callid,
					Integer.parseInt(message));
			break;
		default:
			break;
		}
	}

	private Object OnCallProcessDataEvent(byte[] message, int state) {
		long currentTimeMillis = System.currentTimeMillis();
		byte[] onCallProcessData = this.getListener().onCallProcessData(
				message, state);
		Log4Util.v(Device.TAG,
				"[CallControlManager - OnCallProcessDataEvent]CallProcess time:"
						+ (System.currentTimeMillis() - currentTimeMillis)
						+ "mils");
		if (onCallProcessData == null) {
			return message;
		}
		return onCallProcessData;
	}

	private void OnProcessOriginalDataEvent(byte[] b) {
		getListener().onProcessOriginalData(b);
	}

	/**
	 * 获取push的xml消息内容
	 * 
	 * @param message
	 * @return
	 */
	String formatXml(final String message) {
		String rMessage = message.substring(message.indexOf("<"),
				message.length());
		return rMessage;
	}

	/**
	 * 
	 * @return true soft address normal, else errors.
	 */
	boolean checkNormalSoftAddress() {
		return VoiceUtil.checkIPAddr(getSoftSwitchAddress())
				&& getSoftSwitchPort() > 0;
	}

	public void destroy() {
		super.destroy();

		try {
			instance = null;
			NativeInterface.disConnectToCCP();
			unInitialize();
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}

	public void unInitialize() {
		NativeInterface.unInitialize();

		// amr encode release
		NativeInterface.AmrNBFreeEnc();
	}

	public void setNetworkType(int type, boolean connected, boolean reconnect) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - setNetworkType] type: " + type
						+ ", connected: " + connected + ", reconnect: "
						+ reconnect);
		NativeInterface.setNetworkType(type, connected, reconnect);
	}

	public void setLogLevel(int level) {
		NativeInterface.setLogLevel(level);
	}

	public int setUserName(String name) {
		return NativeInterface.setUserName(name);
	}

	private void setSSLPort(int port) {
		HttpManager.setSSLPort(port);
	}

	public int setUserData(int type, String userData) {
		return NativeInterface.setUserData(type, userData);
	}

	public String getUserData(int type) {
		return NativeInterface.getUserData(type);
	}

	public int setVideoView(String account, Object view, Object localview) {
		//Log4Util.i(Device.TAG, "[CallControllerManager - setVideoView] account: " +account + " view: "
		//		+ view.getClass().getSimpleName());
        Log4Util.i(Device.TAG, "[CallControllerManager - setVideoView] hubintest");
        Log4Util.i(Device.TAG, "[CallControllerManager - setVideoView] account: " + account + " view: " + view.getClass().getSimpleName());
		return NativeInterface.setVideoView(account, localview);
	}

	public int selectCamera(int cameraIndex, int capabilityIndex, int fps,
			int rotate, boolean force) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - selectCamera] cameraIndex: "
						+ cameraIndex + ", capabilityIndex: " + capabilityIndex
						+ ", fps: " + fps + ", rotate: " + rotate + " , force:"
						+ force);
		return NativeInterface.selectCamera(cameraIndex, capabilityIndex, fps,
				rotate, force);
	}

	public CameraInfo[] getCameraInfo() {
		return NativeInterface.getCameraInfo();
	}

	// Video ..
	public int setCodecEnabled(int type, boolean enabled) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - setCodecEnabled] type: " + type
						+ ", enabled: " + enabled);
		return NativeInterface.setCodecEnabled(type, enabled);
	}

	// Video ..
	public boolean getCodecEnabled(int type) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - getCodecEnabled] type: " + type);
		return NativeInterface.getCodecEnabled(type);
	}

	public int setSrtpEnabled(boolean srtp, boolean userMode, int cryptType,
			String key) {
		return NativeInterface.setSrtpEnabled(false, srtp, userMode, cryptType,
				key);
	}

	public String makeCall(int callType, final String accountSid) {
		if (!getVersionSupport(callType)) {
			this.getListener().onMakeCallFailed(accountSid,
					SdkErrorCode.SDK_VERSION_NOTSUPPORT);
			return accountSid;
		}

		Log4Util.i(
				Device.TAG,
				"[CallControllerManager - makeCall] sid: "
						+ accountSid
						+ " , callType "
						+ (callType == CallType.VOICE.getValue() ? CallType.VOICE
								: CallType.VIDEO));
		return NativeInterface.makeCall(callType, accountSid);
	}

	public void releaseCall(final String callid, final int reason) {
		Log4Util.i(Device.TAG, "[CallControllerManager - releaseCall] callid: "
				+ callid);
		NativeInterface.releaseCall(callid, reason);
	}

	public void acceptCall(String callid) {
		Log4Util.i(Device.TAG, "[CallControllerManager - acceptCall] callid: "
				+ callid);
		NativeInterface.acceptCall(callid);
	}

	public void rejectCall(String callid, int reason) {
		Log4Util.i(Device.TAG, "[CallControllerManager - rejectCall] callid: "
				+ callid);
		NativeInterface.rejectCall(callid, reason);
	}

	public void pauseCall(String callid) {
		Log4Util.i(Device.TAG, "[CallControllerManager - pauseCall] callid: "
				+ callid);
		NativeInterface.pauseCall(callid);
	}

	public int resumeCall(String callid) {
		Log4Util.i(Device.TAG, "[CallControllerManager - resumeCall] callid: "
				+ callid);
		return NativeInterface.resumeCall(callid);
	}

	public int transferCall(String callid, String destionation, int callType) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - transferCall] callid: " + callid
						+ ", destionation:" + destionation + " , callType "
						+ callType);
		return NativeInterface.transferCall(callid, destionation, callType);
	}

	public int cancelCall(String callid) {
		Log4Util.i(Device.TAG, "[CallControllerManager - cancelCall] callid: "
				+ callid);
		return NativeInterface.cancelCall(callid);
	}

    public int startRecord(String callid) {
        Log4Util.i(Device.TAG, "[CallControllerManager - startRecord] callid: "
                + callid);
        return NativeInterface.startRecord(callid);
    }
    public int stopRecord(String callid) {
        Log4Util.i(Device.TAG, "[CallControllerManager - stopRecord] callid: "
                + callid);
        return NativeInterface.stopRecord(callid);
    }

	/**********************************************************************
	 * video methods 2013/5/30 *
	 **********************************************************************/
	public int acceptCallByMediaType(String callid, int type) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - acceptCallByMediaType] callid: "
						+ callid + ", type: " + type);
		return NativeInterface.acceptCallByMediaType(callid, type);
	}

	public int updateCallMedia(String callid, int request) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - updateCallMedia] callid: " + callid
						+ ", request: " + request);
		return NativeInterface.updateCallMedia(callid, request);
	}

	public int answerCallMediaUpdate(String callid, int action) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - answerCallMediaUpdate] callid: "
						+ callid + ", action:" + action);
		return NativeInterface.answerCallMediaUpdate(callid, action);
	}

	public int getCallMediaType(String callid) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - getCallMediaType] callid: " + callid);
		return NativeInterface.getCallMeidaType(callid);
	}

    public int setScreenShareActivity(String callid, View view) {
        Log4Util.i(Device.TAG,
                "[CallControllerManager - setScreenShareActivity] callid: " + callid);
        return NativeInterface.setScreenShareActivity(callid, view);
    }

	/**********************************************************************
	 * config methods 2013/6/26 *
	 **********************************************************************/
	public int setAudioConfigEnabled(int type, boolean enabled, int mode) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - setAudioConfigEnabled] type: " + type
						+ ", enable:" + enabled + ", mode:" + mode);
		return NativeInterface.setAudioConfig(type, enabled, mode);
	}

	public boolean getAudioConfig(int type) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - getAudioConfig] type: " + type);
		return NativeInterface.getAudioConfig(type);
	}

	public int getAudioConfigMode(int type) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - getAudioConfigMode] type: " + type);
		return NativeInterface.getAudioConfigMode(type);
	}

	public void setVideoBitRates(int bitrates) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - setVideoBitRates] bitrates: "
						+ bitrates);
		NativeInterface.setVideoBitRates(bitrates);
	}

	public int startRtpDump(String callid, int mediaType, String fileName,
			int direction) {
		Log4Util.i(Device.TAG, "[CallControllerManager - stopRtpDump] callid: "
				+ callid + ", mediaType:" + mediaType + ", fileName:"
				+ fileName + ", direction:" + direction);
		return NativeInterface.startRtpDump(callid, mediaType, fileName,
				direction);
	}

	public int stopRtpDump(String callid, int mediaType, int direction) {
		Log4Util.i(Device.TAG, "[CallControllerManager - stopRtpDump] callid: "
				+ callid + ", mediaType:" + mediaType + ", direction:"
				+ direction);
		return NativeInterface.stopRtpDump(callid, mediaType, direction);
	}

	/**
	 * @deprecated
	 */
	public int getCallState(String callid) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - getCallState] callid: " + callid);
		return NativeInterface.getCallState(callid);
	}

	public String getCurrentCall() {
		String currentCallId = NativeInterface.getCurrentCall();
		Log4Util.i(Device.TAG,
				"[CallControllerManager - getCurrentCall] callid: "
						+ currentCallId);
		return currentCallId;
	}

	/**
	 * dial and message
	 */
	public int sendDTMF(String callid, char dtmf) {
		Log4Util.i(Device.TAG, "[CallControllerManager - sendDTMF] callid: "
				+ callid + " ,dtmf:" + dtmf);
		return NativeInterface.sendDTMF(callid, dtmf);
	}

	/**
	 * 获取通话的网络流量信息
	 * 
	 * @param callid
	 * @see NativeInterface#getNetworkStatistic(String)
	 * @return
	 * 
	 * @version 3.6.1
	 */
	public String getNetworkStatistic(String callid) {
		//Log4Util.i(Device.TAG,
		//		"[CallControllerManager - getNetworkStatistic] callid: "
		//				+ callid);
		return NativeInterface.getNetworkStatistic(callid);
	}

	/**
	 * 
	 * @param receiver
	 * @param message
	 * @return
	 */
	public String sendTextMessage(String receiver, String message,
			String userdata) {
		return NativeInterface.sendTextMessage(receiver, message, userdata);
	}

	/**
	 * 私有云校验接口
	 * 
	 * @param companyID
	 *            企业ID.
	 * @param restAddr
	 *            软交换地址.
	 * @param nativeCheck
	 *            是否本地校验.
	 * @return 成功 0 -1 companyID过长（最大199） -2 restAdd过长（99）
	 */
	public static void setPrivateCloud(String companyID, String restAddr,
			boolean nativeCheck) {
		NativeInterface.setPrivateCloud(companyID, restAddr, nativeCheck);
	}

	/**
	 * 
	 * @return
	 */
	public String getUniqueID() {
		return NativeInterface.GetUniqueID();
	}

	// -------------------------------------------------------------------
	// Set Stun Server. Network access by this server address.
	/**
	 * Function: set Stun Server. Network access by this server address.
	 * Parameters: [IN] server: the server address Parameters: [IN] port: the
	 * service port, the default 3478 Return value: 0 successful failure -1
	 */
	public int SetStunServer(String server) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - SetStunServer] server: " + server);
		return NativeInterface.SetStunServer(server);
	}

	// Custom IP and port
	public int setStunServer(String server, int port) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - SetStunServer] server: " + server
						+ " , port :" + port);
		return NativeInterface.SetStunServer(server, port);
	}

	/**
	 * Function name: setFirewallPolicy setting up a firewall types.
	 * 
	 * @param policy
	 *            Parameters: [IN] policy: types of firewall.
	 * @return value: 0 successful failure -1
	 */
	public int setFirewallPolicy(int policy) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - SetFirewallPolicy] policy: " + policy);
		return NativeInterface.SetFirewallPolicy(policy);
	}

	/**
	 * version 3.3
	 */
	public String getVersion() {
		return NativeInterface.getVersion();
	}

	/**
	 * According to a given call type judge SDK version does not support call
	 * 
	 * @param callType
	 * @return
	 */
	private boolean getVersionSupport(int callType) {
		try {
			String version = getVersion();
			if (TextUtils.isEmpty(version)) {
				return true;
			}
			String[] split = version.split("#");
			String[] str = null;
			for (int i = 0; i < split.length; i++) {
				switch (callType) {
				case 0:
					if (split[i].startsWith("voice")) {
						str = split[i].split("=");
						break;
					}
					break;
				case 1:
					if (split[i].startsWith("video")) {
						str = split[i].split("=");
						break;
					}
					break;

				default:
					break;
				}
			}
			return Boolean.valueOf(str[1]).booleanValue();
		} catch (Exception e) {
			return false;
		}
	}

	/**
	 * version 3.3
	 * 
	 * @param type
	 * @return
	 */
	public String getCallStatistics(int type) {
		//Log4Util.i(Device.TAG,
		//		"[CallControllerManager - getCallStatistics] type: " + type);
		return NativeInterface.getCallStatistics(type);
	}

	/**
	 * version 3.4.1.1 {@link NativeInterface#SetShieldMosaic(boolean flag)}
	 * 
	 * @see
	 * @param flag
	 * @return
	 */
	public int setShieldMosaic(boolean flag) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - SetShieldMosaic] flag: " + flag);
		return NativeInterface.SetShieldMosaic(flag);
	}

	/**
	 * version 3.4.1.1
	 * {@link NativeInterface#SetStartBitRateAfterP2PSucceed(int)}
	 * 
	 * @param rate
	 * @return
	 */
	public int setStartBitRateAfterP2PSucceed(int rate) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - SetStartBitRateAfterP2PSucceed] rate: "
						+ rate);
		return NativeInterface.SetStartBitRateAfterP2PSucceed(rate);
	}

	/**
	 * 
	 * <p>
	 * Title: disConnectToCCP
	 * </p>
	 * <p>
	 * Description:
	 * </p>
	 * {@link NativeInterface#disConnectToCCP()}
	 * 
	 * @version 3.5
	 * @return
	 */
	public int disConnectToCCP() {
		return NativeInterface.disConnectToCCP();
	}

	/**
	 * @see {@link NativeInterface#getLocalVideoSnapshot(String)}
	 * @param callid
	 * @return
	 */
	public Object getLocalVideoSnapshot(String callid) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - getLocalVideoSnapshot] callid: "
						+ callid);
		return NativeInterface.getLocalVideoSnapshot(callid);
	}

	/**
	 * @see {@link NativeInterface#getRemoteVideoSnapshot(String)}
	 * @param callid
	 * @return
	 */
	public Object getRemoteVideoSnapshot(String callid) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - getRemoteVideoSnapshot] callid: "
						+ callid);
		return NativeInterface.getRemoteVideoSnapshot(callid);
	}

	/**
	 * start recording voice when in Voice/Video Call,
	 * 
	 * @param callid
	 *            current voice or video calls id.
	 * @param fileName
	 *            The Recording File local path .
	 * @version 3.6
	 * @see {@link NativeInterface#startRecordVoice(String, String)}
	 * @return
	 */
	public int startRecordVoice(String callid, String fileName) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - startRecordVoice] callid: " + callid
						+ " , fileName :" + fileName);
		return NativeInterface.startRecordVoice(callid, fileName);
	}

	/**
	 * Stop voice recording
	 * 
	 * @param callid
	 *            current voice or video calls id.
	 * @see {@link NativeInterface#stopRecordVoice(String)}
	 * @version 3.6
	 * @return
	 */
	public int stopRecordVoice(String callid) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - stopRecordVoice] callid: " + callid);
		return NativeInterface.stopRecordVoice(callid);
	}

	/**
	 * start recording voice when in Video Call,
	 * 
	 * @param callid
	 *            current video calls id.
	 * @param fileName
	 *            The Recording File local path .
	 * @see {@link NativeInterface#startRecordVoip(String)}
	 * @version 3.6
	 * @return
	 */
	public int startRecordVoip(String callid, String fileName) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - startRecordVoip] callid: " + callid
						+ " , fileName :" + fileName);
		return NativeInterface.startRecordVoip(callid, fileName);
	}

	/**
	 * Stop voice recording of Voice Calls
	 * 
	 * @param callid
	 *            current Video calls id.
	 * @see {@link NativeInterface#stopRecordVoip(String)}
	 * @version 3.6
	 * @return
	 */
	public int stopRecordVoip(String callid) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - stopRecordVoip] callid: " + callid);
		return NativeInterface.stopRecordVoip(callid);
	}

	/**
	 * Whether to open log switch int C/C++ code.
	 * 
	 * @see {@link NativeInterface#setTraceFlag(boolean)}
	 * @version 3.6
	 * @param enabled
	 */
	public void setTraceFlag(boolean enabled) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - setTraceFlag] enabled: " + enabled);
		NativeInterface.setTraceFlag(enabled);
	}

	/**
	 * @param account
	 * @return
	 * @version 3.6
	 */
	public int checkUserOnline(String account) {
		return NativeInterface.checkUserOnline(account);
	}

	public int setMute(final boolean on) {
		Log4Util.i(Device.TAG, "[CallControllerManager - setMute] on/off: "
				+ on);
		return NativeInterface.setMute(on);
	}

	public int enableLoudsSpeaker(boolean enable) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - enableLoudsSpeaker] enable: "
						+ enable);
		return NativeInterface.enableLoudsSpeaker(enable);
	}

	public boolean getMuteStatus() {
		return NativeInterface.getMuteStatus();
	}

	public boolean getLoudsSpeakerStatus() {
		return NativeInterface.getLoudsSpeakerStatus();
	}

	/**
	 * 
	 */
	public int setProcessDataEnabled(String callid, boolean enabled) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - setProcessDataEnabled] callid: "
						+ callid + " , enabled:" + enabled);
		return NativeInterface.setProcessDataEnabled(callid, enabled);
	}

	/**
	 * 
	 * @param callid
	 * @param enabled
	 * @return
	 */
	public int setProcessOriginalDataEnabled(String callid, boolean enabled) {
		Log4Util.i(Device.TAG,
				"[CallControllerManager - setProcessOriginalDataEnabled] callid: "
						+ callid + " , enabled:" + enabled);
		return NativeInterface.setProcessOriginalDataEnabled(callid, enabled);
	}

	/**
	 * @param wifi
	 * @param mobile
	 * @return
	 */
	public int setKeepAliveTimeout(int wifi, int mobile) {
		return NativeInterface.setKeepAliveTimeout(wifi, mobile);
	}

	/**
	 * @deprecated
	 */
	public int setRing(String filename) {
		return NativeInterface.setRing(filename);
	}

	/**
	 * @deprecated
	 */
	public int setRingback(String filename) {
		return NativeInterface.setRingback(filename);
	}

	/**********************************************************************
	 * Amrcodec methods 2013/6/7 *
	 **********************************************************************/
	public int AmrNBEncode(byte[] input, int len, byte[] output, int mode) {
		// Log4Util.i(Device.TAG, "[CallControllerManager - AmrNBEncode] len: "
		// + len + " , mode:" + mode);
		return NativeInterface.AmrNBEncode(input, len, output, mode);
	}

	public int registerAudioDevice() {
		return NativeInterface.registerAudioDevice();
	}

	public int deregisterAudioDevice() {
		return NativeInterface.deregisterAudioDevice();
	}

	public int setNetworkGroupId(String groupId) {
		return NativeInterface.SetNetworkGroupId(groupId);
	}

	/******************************** Rest Method ****************************************************/

	public void makeCallBack(final String selfPhoneNumber,
			final String destPhoneNumber, String srcSerNum, String destSerNum) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.CCP_SELF_PHONENUMBER, selfPhoneNumber);
		b.putString(Constant.CCP_DEST_PHONENUMBER, destPhoneNumber);
		b.putString(Constant.CCP_SELF_SERNUM, srcSerNum);
		b.putString(Constant.CCP_DEST_SERNUM, destSerNum);
		msg.obj = b;
		msg.arg1 = WHAT_MAKECALLBACK;
		sendHandleMessage(msg);
	}

	/**
	 * 创建实时语音对讲
	 * 
	 * @param members
	 * @param type
	 * @param appid
	 */
	public void startInterphone(String[] members, String type, String appid) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putStringArray(Constant.CCP_ARRAY_MEMBERS, members);
		b.putString(Constant.CCP_TYPE, type);
		b.putString(Constant.APP_ID, appid);
		msg.obj = b;
		msg.arg1 = WHAT_START_INTERPHONE;
		sendHandleMessage(msg);
	}

	/**
	 * 控制麦克
	 * 
	 * @param confNo
	 */
	public void controlMIC(String confNo) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.INTERPHONE_ID, confNo);
		msg.obj = b;
		msg.arg1 = WHAT_CONTROL_MIC;
		sendHandleMessage(msg);

	}

	/**
	 * 释放麦克
	 * 
	 * @param confNo
	 */
	public void releaseMic(String confNo) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.INTERPHONE_ID, confNo);
		msg.obj = b;
		msg.arg1 = WHAT_RELEASE_MIC;
		sendHandleMessage(msg);

	}

	/**
	 * 查询参与对讲成员
	 * 
	 * @param confNo
	 */
	public void queryMembersWithInterphone(String confNo) {

		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.INTERPHONE_ID, confNo);
		msg.obj = b;
		msg.arg1 = WHAT_QUERY_INTERPHONE_MEMBERS;
		sendHandleMessage(msg);

	}

	/**********************************************************************
	 * Chatroom *
	 **********************************************************************/

	/**
	 * 
	 * @param appid
	 * @param roomName
	 * @param square
	 * @param keywords
	 * @param pwd
	 * @param isAutoClose
	 * @param voiceMod
	 *            0没有提示音有背景音，、1全部提示音、2无提示音无背景音。默认值为1全部提示音
	 * @param isAutoDelete
	 *            autoDelete 是否自动删除，默认值为1自动删除
	 */
	public void startChatroom(String appid, String roomName, int square,
			String keywords, String pwd, int isAutoClose, int voiceMod,
			int isAutoDelete) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.APP_ID, appid);
		b.putString(Constant.CCP_ROOM_NAME, roomName);
		b.putInt(Constant.CCP_SQUARE, square);
		b.putString(Constant.CCP_KEYWORDS, keywords);
		b.putString(Constant.CCP_PWD, pwd);
		b.putInt(Constant.CCP_FLAGE, isAutoClose);
		b.putInt(Constant.CCP_FLAGE_AUTODELETE, isAutoDelete);
		b.putInt(Constant.CCP_VOICEMOD, voiceMod);
		msg.obj = b;
		msg.arg1 = WHAT_START_CHATROOM;
		sendHandleMessage(msg);
	}

	/**
	 * 解散聊天室
	 * 
	 * @param appId
	 *            应用id
	 * @param roomNo
	 *            房间号
	 */
	public void dismissChatroom(String appId, String roomNo) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.APP_ID, appId);
		b.putString(Constant.CHATROOM_ID, roomNo);
		msg.obj = b;
		msg.arg1 = WHAT_DISMISS_CHATROOM;
		sendHandleMessageAtFront(msg);
	}

	/**
	 * 踢出聊天室成员
	 * 
	 * @param appId
	 *            应用id
	 * @param roomNo
	 *            房间号
	 * @param member
	 *            成员号码
	 */
	public void removeMemberFromChatroom(String appId, String roomNo,
			String member) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.APP_ID, appId);
		b.putString(Constant.CHATROOM_ID, roomNo);
		b.putString(Constant.CCP_MEMBER, member);
		msg.obj = b;
		msg.arg1 = WHAT_REMOVE_MEMBER_CHATROOM;
		sendHandleMessage(msg);
	}

	public void setChatroomMemberSpeakOpt(String appId, String roomNo,
			String member, int opt) {

		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.APP_ID, appId);
		b.putString(Constant.CHATROOM_ID, roomNo);
		b.putString(Constant.CCP_MEMBER, member);
		b.putInt(Constant.SPEAK_OPT, opt);
		msg.obj = b;
		msg.arg1 = WHAT_SET_CHATROOM_SPEAK_OPT;
		sendHandleMessage(msg);
	}

	/**
	 * 获取房间列表
	 * 
	 * @param appId
	 * @param keywords
	 */
	public void queryChatRooms(String appId, String keywords) {

		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.APP_ID, appId);
		b.putString(Constant.CCP_KEYWORDS, keywords);
		msg.obj = b;
		msg.arg1 = WHAT_QUERY_CHATROOMS;
		sendHandleMessage(msg);
	}

	/**
	 * 邀请群聊成员
	 * 
	 * @param members
	 * @param roomNo
	 * @param appId
	 */
	public void inviteMembersJoinChatroom(String[] members, String roomNo,
			String appId) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putStringArray(Constant.CCP_ARRAY_MEMBERS, members);
		b.putString(Constant.CHATROOM_ID, roomNo);
		b.putString(Constant.APP_ID, appId);
		msg.obj = b;
		msg.arg1 = WHAT_INVITE_JOIN_CHATROOM;
		sendHandleMessage(msg);
	}

	/**
	 * 查询参与群聊成员
	 * 
	 * @param chatRoomId
	 */
	public void queryMembersWithChatroom(String chatRoomId) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.CHATROOM_ID, chatRoomId);
		msg.obj = b;
		msg.arg1 = WHAT_QUERY_CHATROOM_MEMBERS;
		sendHandleMessage(msg);
	}

	/**********************************************************************
	 * media message *
	 **********************************************************************/
	/**
	 * 发送多媒体消息
	 * 
	 * @param fileName
	 * @param receiver
	 * @param userData
	 */
	public void sendMediaMsg(String uniqueID, String fileName, String receiver,
			String userData) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.CCP_FILENAME, fileName);
		b.putString(Constant.CCP_RECEIVER, receiver);
		b.putString(Constant.CCP_UNIQUEID, uniqueID);
		b.putString(Constant.CCP_USERDATA, userData);
		msg.obj = b;
		msg.arg1 = WHAT_SEND_MEDIA_MSG;
		Log4Util.d(Device.TAG, "sendMediaMsg\r\n");
		sendHandleMessage(msg);
	}

	/**
	 * Modify the MEDIA message status on the REST server This method will be
	 * required returned results immediately after the completion of execute
	 * 
	 * @param msgId
	 */
	public void confirmDownloadMediaMessage(String[] msgIds) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putStringArray(Constant.CCP_MSGIDS, msgIds);
		msg.obj = b;
		msg.arg1 = WHAT_CONFIRM_DOWNLOAD_MEDIAMESSAGE;
		sendHandleMessage(msg);

		// return doConfirmDownloadMediaMessage(msgId);
	}

	/**
	 * 下载文件
	 * 
	 * @param dLoadList
	 */
	public void downloadAttachmentFiles(ArrayList<DownloadInfo> dLoadList) {

		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putSerializable(Constant.CCP_DOWNLOAD_PARAMETERS, dLoadList);
		msg.obj = b;
		msg.arg1 = WHAT_DOWNLOAD_MEDIA_MSG;
		sendHandleMessage(msg);

	}

	public void UploadMediaChunked(String uniqueID, String fileName,
			String receiver, String userData) {

		// Because the mechanism of acquisition of audio data by edge upload
		// (chunked),
		// and has been implemented in asynchronous thread operation initiates
		// uploading place,
		// so here you can directly call transfer, no longer send message
		// asynchronous call

		// release Note : V3.0.1 version adds a parameter, parameter is uploaded
		// to the server,
		// the server according to push things back to the sender of the message
		// to match
		// The unique identifier for this message
		// add user for user deverplor
		doUploadMediaChunked(uniqueID, fileName, receiver, userData);
	}

	/**
	 * 
	 * <p>
	 * Title: CCPdownloadFiles
	 * </p>
	 * <p>
	 * Description: CCP unified download method
	 * </p>
	 * 
	 * @param dLoadList
	 * @deprecated
	 */
	public void CCPdownloadFiles(ArrayList<DownloadInfo> dLoadList) {

		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putSerializable(Constant.CCP_DOWNLOAD_PARAMETERS, dLoadList);
		msg.obj = b;
		msg.arg1 = WHAT_CCP_DOWNLOAD;
		sendHandleMessage(msg);

	}

	/**
	 * 
	 * <p>
	 * Title: DownloadVideoConferencePortraits
	 * </p>
	 * <p>
	 * Description: CCP unified download method
	 * </p>
	 * 
	 * @param dLoadList
	 */
	public void DownloadVideoConferencePortraits(
			ArrayList<VideoPartnerPortrait> portraitsList) {

		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putSerializable(Constant.CCP_DOWNLOAD_PARAMETERS_PORTRAIT,
				portraitsList);
		msg.obj = b;
		msg.arg1 = msg.what = WHAT_CCP_DOWNLOAD;
		sendHandleMessage(msg);

	}

	@Override
	public void setStunServer(String server) {
		if (!TextUtils.isEmpty(server)) {
			try {
				String[] split = server.split(":");

				if (split != null && split.length == 2) {
					setStunServer(split[0], Integer.parseInt(split[1]));
				} else {
					SetStunServer(server);
				}
				// return this.callControlManager.setFirewallPolicy(policy);
			} catch (Exception e) {
			}
		}
	}

	public void startVideoConference(String appId, String conferenceName,
			int square, String keywords, String conferencePwd, int isAutoClose,
			int voiceMod, int isAutoDelete) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.APP_ID, appId);
		b.putString(Constant.CCP_ROOM_NAME, conferenceName);
		b.putInt(Constant.CCP_SQUARE, square);
		b.putString(Constant.CCP_KEYWORDS, keywords);
		b.putString(Constant.CCP_PWD, conferencePwd);
		b.putInt(Constant.CCP_FLAGE, isAutoClose);
		b.putInt(Constant.CCP_FLAGE_AUTODELETE, isAutoDelete);
		b.putInt(Constant.CCP_VOICEMOD, voiceMod);
		msg.obj = b;
		msg.arg1 = WHAT_START_VIDEOCONFERENCE;
		sendHandleMessage(msg);
	}

	/**
	 * 
	 * @param appId
	 * @param conferenceName
	 * @param conferencePwd
	 * @param options
	 */
	public void startVideoConference(String appId, String conferenceName,
			String conferencePwd, ConferenceOptions options) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.APP_ID, appId);
		b.putString(Constant.CCP_ROOM_NAME, conferenceName);
		b.putString(Constant.CCP_PWD, conferencePwd);
		b.putSerializable(Constant.CCP_CONF_OPTIONS, options);
		msg.obj = b;
		msg.arg1 = WHAT_START_VIDEOCONFERENCE_MULTI;
		sendHandleMessage(msg);
	}

	public void queryMembersInVideoConference(String conferenceId) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.CONFERENCE_ID, conferenceId);
		msg.obj = b;
		msg.arg1 = msg.what = WHAT_QUERY_VIDEOCONFERENCE_MEMBERS;
		sendHandleMessage(msg);
	}

	public void queryVideoConferences(String appId, String keywords) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.APP_ID, appId);
		b.putString(Constant.CCP_KEYWORDS, keywords);
		msg.obj = b;
		msg.arg1 = msg.what = WHAT_QUERY_VIDEOCONFERENCES;
		sendHandleMessage(msg);
	}

	/**
	 * 
	 * <p>
	 * Title: dismissVideoConference
	 * </p>
	 * <p>
	 * Description:
	 * </p>
	 * 
	 * @param appId
	 * @param conferenceId
	 *            the Video Conference No. ID
	 * @see AbstractDispatcher#doDismissVideoConference(String, String)
	 */
	public void dismissVideoConference(String appId, String conferenceId) {

		removeVideoConferenceHandleMessage();

		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.APP_ID, appId);
		b.putString(Constant.CONFERENCE_ID, conferenceId);
		msg.obj = b;
		msg.arg1 = msg.what = WHAT_DISMISS_VIDEOCONFERENCE;
		sendHandleMessageAtFront(msg);
	}

	/**
	 * 
	 * @param appId
	 * @param conferenceId
	 * @param publish
	 */
	public void handleVideoPublishOperate(String appId, String conferenceId,
			boolean publish) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.APP_ID, appId);
		b.putString(Constant.CONFERENCE_ID, conferenceId);
		b.putBoolean(Constant.VIDEO_PUBLISH, publish);
		msg.obj = b;
		msg.arg1 = msg.what = WHAT_PUBLISH_VIDEO;
		sendHandleMessageAtFront(msg);
	}

	/**
	 * 
	 * <p>
	 * Title: removeMemberFromVideoConference
	 * </p>
	 * <p>
	 * Description:
	 * </p>
	 * 
	 * @param appId
	 * @param conferenceId
	 *            the Video Conference No. ID
	 * @param member
	 * @see AbstractDispatcher#doRemoveMemberFromVideoConference(String, String,
	 *      String);
	 */
	public void removeMemberFromVideoConference(String appId,
			String conferenceId, String member) {

		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.APP_ID, appId);
		b.putString(Constant.CONFERENCE_ID, conferenceId);
		b.putString(Constant.CCP_MEMBER, member);
		msg.obj = b;
		msg.arg1 = msg.what = WHAT_REMOVE_MEMBER_VIDEOCONFERENCE;
		sendHandleMessage(msg);
	}

	/**
	 * 
	 * <p>
	 * Title: switchRealScreenToVoip
	 * </p>
	 * <p>
	 * Description:
	 * </p>
	 * 
	 * @param appId
	 * @param conferenceId
	 *            the Video Conference No. ID
	 * @param voip
	 * @see AbstractDispatcher#doswitchRealScreenToVoip(String, String, String)
	 */
	public void switchRealScreenToVoip(String appId, String conferenceId,
			String voip) {

		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.APP_ID, appId);
		b.putString(Constant.CONFERENCE_ID, conferenceId);
		b.putString(Constant.CCP_VOIP, voip);
		msg.obj = b;
		msg.arg1 = msg.what = WHAT_SWITCH_REALSCREEN;
		sendHandleMessage(msg);
	}

	/**
	 * 
	 * <p>
	 * Title: GetPortraitsFromVideoConference
	 * </p>
	 * <p>
	 * Description: query the porprait of Video Conference
	 * </p>
	 * 
	 * @param conferenceId
	 *            the Video Conference No. ID
	 * 
	 * @see AbstractDispatcher#doPortraitsFromVideoConference(String)
	 */
	public void GetPortraitsFromVideoConference(String conferenceId) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.CONFERENCE_ID, conferenceId);
		msg.obj = b;
		msg.arg1 = msg.what = WHAT_GET_VIDEO_CONFERENCE_PORPRRAIT;
		sendHandleMessage(msg);
	}

	/**
	 * 
	 * <p>
	 * Title: SendLocalPortrait
	 * </p>
	 * <p>
	 * Description: Upload video conference personal picture
	 * </p>
	 * 
	 * @param fileName
	 *            the file path thar upload.
	 * @param conferenceId
	 *            the Video Conference No. ID
	 * @see AbstractDispatcher#doSendLocalPortrait(String, String);
	 */
	public void SendLocalPortrait(String fileName, String conferenceId) {
		Message msg = getHandleMessage();
		Bundle b = new Bundle();
		b.putString(Constant.CCP_FILENAME, fileName);
		b.putString(Constant.CONFERENCE_ID, conferenceId);
		msg.obj = b;
		msg.arg1 = msg.what = WHAT_SEND_LOCAL_VIDEO_PORPRTAIT;
		sendHandleMessage(msg);
	}

	/**
	 * 
	 */
	public void removeVideoConferenceHandleMessage() {
		for (int what = WHAT_QUERY_VIDEOCONFERENCES; what <= WHAT_SEND_LOCAL_VIDEO_PORPRTAIT; what++) {
			removeHandleMessages(what);
		}
	}
}
