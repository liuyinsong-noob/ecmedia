/**
 *
 */
package com.hisun.phone.core.voice;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Message;
import android.os.Parcel;
import android.os.Parcelable;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.view.SurfaceView;
import android.view.View;

import com.CCP.phone.CCPCallEvent;
import com.CCP.phone.CameraInfo;
import com.CCP.phone.NativeInterface;
import com.CCP.phone.VideoSnapshot;
import com.hisun.phone.core.voice.DeviceListener.Reason;
import com.hisun.phone.core.voice.exception.CCPRecordException;
import com.hisun.phone.core.voice.listener.OnChatroomListener;
import com.hisun.phone.core.voice.listener.OnIMListener;
import com.hisun.phone.core.voice.listener.OnInterphoneListener;
import com.hisun.phone.core.voice.listener.OnProcessOriginalAudioDataListener;
import com.hisun.phone.core.voice.listener.OnTriggerSrtpListener;
import com.hisun.phone.core.voice.listener.OnVideoConferenceListener;
import com.hisun.phone.core.voice.listener.OnVideoMemberFrameListener;
import com.hisun.phone.core.voice.listener.OnVoIPListener;
import com.hisun.phone.core.voice.listener.OnVoIPListener.OnCallProcessDataListener;
import com.hisun.phone.core.voice.listener.OnVoIPListener.OnCallRecordListener;
import com.hisun.phone.core.voice.model.CallStatisticsInfo;
import com.hisun.phone.core.voice.model.CloopenReason;
import com.hisun.phone.core.voice.model.DownloadInfo;
import com.hisun.phone.core.voice.model.NetworkStatistic;
import com.hisun.phone.core.voice.model.Response;
import com.hisun.phone.core.voice.model.chatroom.Chatroom;
import com.hisun.phone.core.voice.model.chatroom.ChatroomDismissMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMember;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomRemoveMemberMsg;
import com.hisun.phone.core.voice.model.im.IMTextMsg;
import com.hisun.phone.core.voice.model.im.InstanceMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneMember;
import com.hisun.phone.core.voice.model.interphone.InterphoneMsg;
import com.hisun.phone.core.voice.model.setup.UserAgentConfig;
import com.hisun.phone.core.voice.model.videoconference.VideoConference;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceDismissMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMember;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceRemoveMemberMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoPartnerPortrait;
import com.hisun.phone.core.voice.multimedia.AudioFoucsManager;
import com.hisun.phone.core.voice.multimedia.AudioRecordManager;
import com.hisun.phone.core.voice.multimedia.MediaManager;
import com.hisun.phone.core.voice.multimedia.MediaPlayManager;
import com.hisun.phone.core.voice.net.ApiParser;
import com.hisun.phone.core.voice.opts.ConferenceOptions;
import com.hisun.phone.core.voice.util.AdaptationTools;
import com.hisun.phone.core.voice.util.ConferenceUtils;
import com.hisun.phone.core.voice.util.Cryptos;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.SdkErrorCode;
import com.hisun.phone.core.voice.util.VoiceUtil;

import java.io.ByteArrayInputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.UUID;

/**
 * Device implemention class.
 *
 */
@SuppressWarnings("deprecation")
public final class DeviceImpl extends Device implements CCPCallEvent {

	private Context context;
	private String capabilityToken;
	private PendingIntent incomingIntent;
	private CallControlManager callControlManager;
	private MediaManager mediaManager;
	private AudioRecordManager audRecordManager;
	private MediaPlayManager mediaPlayManager;
	private TelephonyManager mTelephonyManager;
	private CallType makeCallType;

	private CallbackHandler mCallbackHandler;

	private final UUID uuid = UUID.randomUUID();
	private volatile boolean online = false;
	private volatile boolean keepingCall = false;

	private static Map<String , SurfaceView> mRemoteViewMap = new HashMap<String , SurfaceView>();

	// 3.4.1.2
	private volatile boolean voiceRecording = false;

	/**
	 * If the call is busy, when the active release to stop playing the ring
	 * back tone
	 */
	private volatile boolean voicePlaying = false;

	/**
	 * default conference params.
	 */
	private boolean isAutoJoin = true;

	private Map<String, String> userAgentParams;

	static final int WAKE_TIME = 15000;

	private volatile boolean isIncomingCall = false;
	private final LinkedList<String> incomingCallIDArray = new LinkedList<String>();
	private final LinkedList<String> outgoingCallIDArray = new LinkedList<String>();
	private final HashMap<String, Object> multiViewHashMap = new HashMap<String, Object>();
	// user data info.
	private String selfPhone = "";
	private String selfName = "";
	private String confpwd = "";

	//private SurfaceView remoteView;

	@SuppressWarnings("unused")
	private SurfaceView localView;

	private String ccpCallingID;
	private RunningType runningType = RunningType.RunningType_None;

	/**
	 * The conference access length adjustment 32
	 * ('conf'+serviceid+cmserial+roomid+listen+speak ), Among them: the prefix
	 * conf fixed content
	 * ,serviceid为10位，cmserial为5位、roomid为11位、listen为1位，0不可听，1可听
	 * 、speak为1位，0不可讲、1可讲 2、cmserial+roomid组成的串要保证平台唯一
	 * 3、会议邀请外呼获取callmanager的cmserial解析逻辑要调整，即要支持8位的会议id解析，也要支持16位的解析
	 */
	private String mOriginalConferenceNo;

	/**
	 * Management of CCP contains callback monitoring function
	 *
	 * @version 3.5
	 */
	CCPListenerInfo mListenerInfo;

	Object mLocks = new Object();

	/**
	 *
	 * <p>
	 * Title: getCCPListenerInfo
	 * </p>
	 * <p>
	 * Description:
	 * </p>
	 *
	 * @version 3.5
	 * @return
	 */
	CCPListenerInfo getCCPListenerInfo() {
		if (mListenerInfo != null) {
			return mListenerInfo;
		}
		mListenerInfo = new CCPListenerInfo();
		return mListenerInfo;
	}

	DeviceImpl() {
		this.context = CCPCallImpl.getInstance().getContext();
		this.callControlManager = CCPCallImpl.getInstance()
				.getCallControlManager();
		this.mediaManager = CCPCallImpl.getInstance().getMediaManager();
		// this.recordManager = CCPCallImpl.getInstance().getRecordManager();
		this.audRecordManager = CCPCallImpl.getInstance()
				.getAudioRecordManager();
		this.mediaPlayManager = CCPCallImpl.getInstance().getMediaPlayManager();
		this.mTelephonyManager = (TelephonyManager) getContext()
				.getSystemService(Context.TELEPHONY_SERVICE);

	}

    DeviceImpl(DeviceListener deviceListener, Map<String, String> params)
			throws Exception {
		this();
		setDeviceListener(deviceListener);
		setCapabilityToken(params);

		setCallbackHandler(new CallbackHandler(getCCPListenerInfo()));
	}

	DeviceImpl(DeviceImpl d) throws Exception {
		this(d.getDeviceListener(), d.getUserAgentParams());
	}

	private void setCapabilityToken(final Map<String, String> params)
			throws Exception {
		this.userAgentParams = params;
		final CallControlManager mCallManager = callControlManager;
		callControlManager.postCommand(new Runnable() {

			@Override
			public void run() {
				try {
					mCallManager.init(DeviceImpl.this, new UserAgentConfig(
							params));
				} catch (Exception e) {
					e.printStackTrace();
					onConnectError(Reason.UNKNOWN.getStatus());
				}
			}
		});
	}

	/**
	 * @return the uuid
	 */
	public UUID getUUID() {
		return uuid;
	}

	/**
	 * assistant methods
	 *
	 */
	public void setDeviceListener(DeviceListener inListener) {
		getCCPListenerInfo().deviceListener = inListener;
		audRecordManager.setListener(this);
		mediaPlayManager.setListener(this);
		mediaPlayManager.setOnAudioFocusLinstener(new AudioFoucsManager(
				getContext()));
	}

	public void setCallbackHandler(CallbackHandler handler) {
		this.mCallbackHandler = handler;
	}

	/**
	 * @param what
	 * @param arg1
	 * @param arg2
	 * @param object
	 */
	public void sendToTarget(int what, int arg1, int arg2, Object object) {
		if (mCallbackHandler != null) {
			mCallbackHandler.obtainMessage(what, arg1, arg2, object)
					.sendToTarget();
		}
	}

	public void sendToTarget(int what, Object object) {
		if (mCallbackHandler != null) {
			mCallbackHandler.obtainMessage(what, object).sendToTarget();
		}
	}

	/**
	 * send object to activity by handler.
	 *
	 * @param what
	 *            message id of handler
	 * @param obj
	 *            message of handler
	 */
	private void sendTarget(int what, int arg1, Object obj) {
		long currentTimeMillis = System.currentTimeMillis();
		if (mCallbackHandler != null) {
			Message obtainMessage = mCallbackHandler.obtainMessage(what);
			obtainMessage.arg1 = arg1;
			obtainMessage.obj = obj;
			obtainMessage.sendToTarget();
		}
		Log4Util.v(Device.TAG, "time consum "
				+ (System.currentTimeMillis() - currentTimeMillis) + "mils");
	}

	public void setIncomingIntent(PendingIntent paramPendingIntent) {
		this.incomingIntent = paramPendingIntent;
		CCPCallImpl.getInstance().deviceChanged(this);
	}

	void destroy() {

		try {
			online = false;
			context = null;
			capabilityToken = null;
			getCCPListenerInfo().release();
			mListenerInfo = null;
			incomingIntent = null;
			callControlManager = null;
			mediaManager = null;
			selfName = null;
			selfPhone = null;
			confpwd = null;
			//remoteView = null;
			localView = null;
			if (audRecordManager != null) {
				audRecordManager.release();
			}
			if (userAgentParams != null) {
				userAgentParams.clear();
			}
			if (incomingCallIDArray != null) {
				incomingCallIDArray.clear();
			}
			if (outgoingCallIDArray != null) {
				outgoingCallIDArray.clear();
			}
		} catch (Exception e) {
		}
	}

	public void release() {
		CCPCall.shutdown();
	}

	public void setSelfName(String name) {
		if (name == null || name.length() == 0) {
			Log4Util.e("[DeviceImpl - setSelfUserName] name is null.");
			return;
		}

		selfName = "nickname=" + name + ";";
		Log4Util.i("[DeviceImpl - setSelfUserName] set user name: " + selfName);

		if (this.callControlManager != null) {
			this.callControlManager.setUserData(USERDATA_FOR_INVITE,
					getSelfInfo());
		}
	}

	/**
	 * The conference password
	 *
	 * @param pwd
	 */
	void setConfpwd(String pwd) {
		if (TextUtils.isEmpty(pwd)) {
			Log4Util.e("[DeviceImpl - setConfpwd] pwd is null.");
			return;
		}

		confpwd = "confpwd="
				+ Cryptos.toBase64QES(Cryptos.SECRET_CONF_KEY, pwd) + ";";
		Log4Util.i("[DeviceImpl - setConfpwd] set conference password: " + pwd);

		if (this.callControlManager != null) {
			this.callControlManager.setUserData(USERDATA_FOR_INVITE,
					getSelfInfo());
		}
	}

	private String getSelfInfo() {
		StringBuffer sb = new StringBuffer();
		if (!TextUtils.isEmpty(selfName)) {
			sb.append(selfName);
		}

		if (!TextUtils.isEmpty(selfPhone)) {
			sb.append(selfPhone);
		}
		// The conference password
		if (!TextUtils.isEmpty(confpwd)) {
			sb.append(confpwd);
		}
		return sb.length() != 0 ? sb.toString() : "";
	}

	public void setSelfPhoneNumber(String mobile) {
		if (mobile == null || mobile.length() == 0) {
			Log4Util.e("[DeviceImpl - setSelfPhoneNumber] mobile is null.");
			return;
		}

		selfPhone = "tel=" + mobile + ";";
		Log4Util.i("[DeviceImpl - setSelfPhoneNumber] set mobile number: "
				+ selfPhone);

		if (this.callControlManager != null) {
			this.callControlManager.setUserData(USERDATA_FOR_INVITE,
					getSelfInfo());
		}
	}

	/** 用于非VOIP通话时清空底层userdate **/
	private void clearUserData() {
		if (this.callControlManager != null) {
			this.callControlManager.setUserData(USERDATA_FOR_INVITE, "");
		}
	}

	/** 用于VOIP通话时恢复底层userdate **/
	private void resetUserData() {
		if (this.callControlManager != null) {
			this.callControlManager.setUserData(USERDATA_FOR_INVITE,
					getSelfInfo());
		}
	}

	@Override
	public void setVideoView(String userid, SurfaceView remoteView, SurfaceView localView) {
        mRemoteViewMap.put(userid, remoteView);
        //this.remoteView = remoteView;
        this.localView = localView;
    }

    public static SurfaceView getVideoView(String account)
    {
        return mRemoteViewMap.get(account);
    }

	/**
	 * call methods
	 */
	public State isOnline() {
		return online ? State.ONLINE : State.OFFLINE;
	}

	boolean isKeepingCall() {
		return keepingCall;
	}

	boolean isHoldSystemCall() {
		return this.mTelephonyManager.getCallState() == TelephonyManager.CALL_STATE_OFFHOOK;
	}

	long mt = 0;

	public String makeCall(CallType callType, String accountId) {
		// default by voip call.
		return ccpCallingID = makeCallByRunningType(callType, accountId,
				RunningType.RunningType_Voip);
	}

	/**
	 * Interface makeAudio or makeVideo Call.
	 *
	 * @param callType
	 * @param accountId
	 * @param rType
	 * @return
	 */
	String makeCallByRunningType(CallType callType, String accountId,
			RunningType rType) {

		// If the SDK is not in the idle state, then return error.
		if (runningType != RunningType.RunningType_None) {
			onMakeCallFailed("0", SdkErrorCode.SDK_CALL_BUSY);
			return null;
		}

		if (!checkCallToken()) {
			Log4Util.e("[DeviceImpl - makeCall] your server capability token forbid your call.");
			onMakeCallFailed("", 2);
			return null;
		}

		if (isKeepingCall() || isHoldSystemCall()) {
			Log4Util.e(Device.TAG, "Currently hold a call.");
			return getCurrentCall();
		}

		this.makeCallType = callType;

		if (this.callControlManager != null
				&& (Math.abs(System.currentTimeMillis() - mt) > 2000)) {
			mt = System.currentTimeMillis();

			if (callType == CallType.VIDEO) {

				if (mRemoteViewMap.get(accountId) != null) {
					this.callControlManager.setVideoView(accountId, mRemoteViewMap.get(accountId), null);
					Log4Util.i(Device.TAG,
							"[DeviceImpl -  makeCall] video view has been setting.");
				} else {
					Log4Util.w(Device.TAG,
							"[DeviceImpl -  makeCall] video view is null.");
				}

			} else {

				if (VoiceUtil.isP2LCallNumnber(accountId)) {

					setFirewallPolicy(0);
				}
			}
			resetUserData();
			runningType = rType;
			final String callid = this.callControlManager.makeCall(
					(callType.getValue()), accountId);
			Log4Util.i(Device.TAG,
					"[DeviceImpl - makeCallByRunningType] Current SDK  runningType "
							+ runningType.getValue());

			if (runningType == RunningType.RunningType_Voip) {
				// If the VoIP call, then add.
				synchronized (outgoingCallIDArray) {
					outgoingCallIDArray.add(callid);
				}
			}

			// bug for makeCall failed the callid is null.
			if (TextUtils.isEmpty(callid)) {
				this.callControlManager.postCommand(new Runnable() {

					@Override
					public void run() {
						try {
							Thread.sleep(500L);
							onMakeCallFailed(callid,
									SdkErrorCode.SDK_NOT_REGISTED);
							runningType = RunningType.RunningType_None;
						} catch (InterruptedException e) {
							e.printStackTrace();
						}

					}
				});
			}

			return callid;
		}

		throw new RuntimeException("Call intervals not less than 2000s");

	}

	public void releaseCall(String callid) {
		this.keepingCall = false;
		this.makeCallType = null;
		this.incomingCallType = null;
		mOriginalConferenceNo = null;
		if (this.callControlManager != null) {
			this.callControlManager.releaseCall(callid, 0);
		}
		onCallReleased(callid);
	}

	private CallType incomingCallType;

	// just called invoked method
	public void acceptCall(String callid, String account) {
		if (this.callControlManager != null) {

			if (incomingCallType == CallType.VIDEO) {

				if (mRemoteViewMap.get(account) != null) {
					this.callControlManager.setVideoView(account, mRemoteViewMap.get(account), null);
					// this.callControlManager.selectCamera(1, 0, 15, 0 ,
					// false);
					Log4Util.i(Device.TAG,
							"[DeviceImpl -  acceptCall] video view has been setting.");
				} else {
					Log4Util.w(Device.TAG,
							"[DeviceImpl -  acceptCall] video view is null.");
				}
			}

			this.callControlManager.acceptCall(callid);
		}
	}

	public void rejectCall(String callid) {
		rejectCall(callid, 3);
	}

	@Override
	public void rejectCall(String callid, int reason) {
		if (this.callControlManager != null) {
			this.keepingCall = false;
			this.makeCallType = null;
			this.incomingCallType = null;
			this.callControlManager.rejectCall(callid, reason);
		}
	}

	public void pauseCall(String callid) {
		if (this.callControlManager != null) {
			this.callControlManager.pauseCall(callid);
		}
	}

	public void resumeCall(String callid) {
		if (this.callControlManager != null) {
			this.callControlManager.resumeCall(callid);
		}
	}

	/**
	 * Transfer an VoIP call . default call type for {@link CallType#VOICE}
	 *
	 * @see #transferCall(String, String, CallType)
	 */
	public int transferCall(String callid, String destionation) {
		return transferCall(callid, destionation, CallType.VOICE);
	}

	/**
	 * Transfer an VoIP call .
	 *
	 * @param callid
	 *            Uniquely identifies the current call
	 * @param destionation
	 *            The called number.
	 * @param callType
	 *            {@link CallType#VOICE}
	 * @return The success of 0: successfully; 0 Non failure
	 */
	public int transferCall(String callid, String destionation,
			CallType callType) {
		if (this.callControlManager != null) {
			return this.callControlManager.transferCall(callid, destionation,
					callType.getValue());
		}

		return SdkErrorCode.SDK_MAKECALL_FAILED;
	}

	public void cancelCall(String callid) {
		// 如果已经实时对讲,则不做操作 会议
		if (runningType == RunningType.RunningType_Interphone
				|| runningType == RunningType.RunningType_ChatRoom) {
			return;
		}
		if (this.callControlManager != null) {
			this.callControlManager.cancelCall(callid);
		}
	}

	public String getCurrentCall() {
		if (this.callControlManager != null) {
			return this.callControlManager.getCurrentCall();
		}
		return null;
	}

	/**
	 * dial and message
	 */
	@Override
	public void sendDTMF(String callid, char dtmf) {
		if (this.callControlManager != null) {
			this.callControlManager.sendDTMF(callid, dtmf);
		}
	}

	@Override
	public void makeCallback(String selfPhone, String destPhone) {
		makeCallback(selfPhone, destPhone, null, null);
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see com.hisun.phone.core.voice.Device#makeCallback(java.lang.String,
	 * java.lang.String, java.lang.String, java.lang.String)
	 */
	@Override
	public void makeCallback(String selfPhoneNumber, String destPhoneNumber,
			String srcSerNum, String destSerNum) {
		// 如果已经实时对讲,则不做操作
		if (runningType != RunningType.RunningType_None) {
			return;
		}
		if (this.callControlManager != null) {
			resetUserData();
			this.callControlManager.makeCallBack(selfPhoneNumber,
					destPhoneNumber, srcSerNum, destSerNum);
		}
	}

	/**
	 * audio recorder control
	 */
	@Override
	public void enableLoudsSpeaker(boolean enable) {
		try {

			if (enable == getLoudsSpeakerStatus()) {
				return;
			}

			if (this.callControlManager != null) {
				this.callControlManager.enableLoudsSpeaker(enable);
			}

			Log4Util.d(Device.TAG, "LoudsSpeaker is " + enable);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Override
	public boolean getLoudsSpeakerStatus() {
		/*
		 * AudioManager am = (AudioManager)
		 * getContext().getSystemService(Context.AUDIO_SERVICE); return
		 * am.isSpeakerphoneOn();
		 */
		return this.callControlManager.getLoudsSpeakerStatus();
	}

	/**
	 * modify version 3.6 enabel native method
	 */
	@Override
	public void setMute(boolean isMute) {
		try {
			/*
			 * AudioManager am = (AudioManager)
			 * getContext().getSystemService(Context.AUDIO_SERVICE);
			 * am.setMicrophoneMute(!am.isMicrophoneMute());
			 */
			if (isMute == getMuteStatus()) {
				return;
			}

			this.callControlManager.setMute(isMute);
		} catch (Exception e) {
			Log4Util.d(Device.TAG, "[DeviceImpl -  setMute] " + e.getMessage());
		}
	}

	/**
	 * modify version 3.6
	 */
	@Override
	public boolean getMuteStatus() {
		// AudioManager am = (AudioManager)
		// getContext().getSystemService(Context.AUDIO_SERVICE);
		// return am.isMicrophoneMute();
		return this.callControlManager.getMuteStatus();
	}

	// 打开扬声器
	public void OpenSpeaker() {
		try {
			AudioManager audioManager = (AudioManager) getContext()
					.getSystemService(Context.AUDIO_SERVICE);
			audioManager.setMode(AudioManager.MODE_IN_COMMUNICATION);

			if (!audioManager.isSpeakerphoneOn()) {
				audioManager.setSpeakerphoneOn(true);
				audioManager
						.setStreamVolume(
								AudioManager.STREAM_VOICE_CALL,
								audioManager
										.getStreamMaxVolume(AudioManager.STREAM_VOICE_CALL),
								AudioManager.STREAM_VOICE_CALL);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	// 关闭扬声器
	private void CloseSpeaker() {
		try {
			AudioManager audioManager = (AudioManager) getContext()
					.getSystemService(Context.AUDIO_SERVICE);
			if (audioManager != null) {
				audioManager.setMode(AudioManager.MODE_IN_COMMUNICATION);
				audioManager.setSpeakerphoneOn(false);
				audioManager.setStreamVolume(AudioManager.STREAM_VOICE_CALL, 5,
						AudioManager.STREAM_VOICE_CALL);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	private int currVolume = 0;

	@Deprecated
	public void _OpenSpeaker() {
		try {
			AudioManager audioManager = (AudioManager) getContext()
					.getSystemService(Context.AUDIO_SERVICE);
			audioManager.setMode(AudioManager.ROUTE_SPEAKER);
			currVolume = audioManager
					.getStreamVolume(AudioManager.STREAM_VOICE_CALL);

			if (!audioManager.isSpeakerphoneOn()) {
				// setSpeakerphoneOn() only work when audio mode set to
				// MODE_IN_CALL.
				audioManager.setMode(AudioManager.MODE_INVALID);
				audioManager.setSpeakerphoneOn(true);
				audioManager
						.setStreamVolume(
								AudioManager.STREAM_VOICE_CALL,
								audioManager
										.getStreamMaxVolume(AudioManager.STREAM_VOICE_CALL),
								AudioManager.STREAM_VOICE_CALL);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Deprecated
	public void _CloseSpeaker() {
		try {
			AudioManager audioManager = (AudioManager) getContext()
					.getSystemService(Context.AUDIO_SERVICE);
			if (audioManager != null) {
				if (audioManager.isSpeakerphoneOn()) {
					audioManager.setSpeakerphoneOn(false);
					audioManager.setStreamVolume(
							AudioManager.STREAM_VOICE_CALL, currVolume,
							AudioManager.STREAM_VOICE_CALL);
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Override
	public DeviceListener getDeviceListener() {
		return getCCPListenerInfo().deviceListener;
	}

	/******************************************* SDK Callback *********************************************************/
	public static final int RING_TIME = 60 * 1000;

	@Override
	public void onCallProceeding(String callid) {
		Log4Util.i(Device.TAG, "[DeviceImpl - onCallProceeding] " + callid);

		if (AdaptationTools.isXinweiDevice()) {
			CloseSpeaker();
		}

		// If VOIP, then the callback
		if (runningType == RunningType.RunningType_Voip) {
			sendToTarget(CallbackHandler.WHAT_VOIP_CALL,
					CallbackHandler.VOIP_CALL_PROCEEDING, -1, callid);
		}
	}

	/**
	 * just called invoked this method
	 *
	 * 只有主叫呼叫，接通，回调该方法，播放回铃声
	 */
	@Override
	public void onCallAlerting(String callid) {
		Log4Util.i(Device.TAG, "[DeviceImpl - onCallAlerting] " + callid);

		keepingCall = true;
		// note: play headphones ring when outgoing
		this.mediaManager.queueSound(MediaManager.StockSound.OUTGOING,
				RING_TIME, new MediaManager.SoundPlaybackListener() {
					public void onCompletion() {
						Log4Util.i("[DeviceImpl - makeCall]MediaManager.StockSound.OUTGOING onCompletion.");
					}
				});

		sendToTarget(CallbackHandler.WHAT_VOIP_CALL,
				CallbackHandler.VOIP_CALL_ALERTING, -1, callid);
	}

	/**
	 * just called invoked this method
	 *
	 */
	@Override
	public void onCallAnswered(String callid) {
		Log4Util.i(Device.TAG, "[DeviceImpl - onCallAnswered] " + callid);
		keepingCall = true;
		this.mediaManager.stop();
		// CloseSpeaker();

		Bundle bundle = getCloopenReasonBundle(CallbackHandler
				.getCloopenReason(0));
		bundle.putString(Device.CONFNO, mOriginalConferenceNo);
		if (runningType == RunningType.RunningType_Interphone) {
			sendTarget(CallbackHandler.WHAT_REST_INTERPHONE,
					CallbackHandler.WHAT_START_INTERPHONE, bundle);
			return;
		} else if (runningType == RunningType.RunningType_ChatRoom) {
			bundle.putString(Device.CONFNO, ConferenceUtils
					.getProcesConferenceNo(mOriginalConferenceNo));
			sendTarget(CallbackHandler.WHAT_REST_CHATROOM,
					CallbackHandler.WHAT_START_CHATROOM, bundle);
			return;
		} else if (runningType == RunningType.RunningType_VideoConference) {
			bundle.putString(Device.CONFNO, ConferenceUtils
					.getProcesConferenceNo(mOriginalConferenceNo));
			sendTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
					CallbackHandler.WHAT_START_VIDEOCONFERENCE, bundle);
			return;
		}

		sendToTarget(CallbackHandler.WHAT_VOIP_CALL,
				CallbackHandler.VOIP_CALL_ANSWERED, -1, callid);
	}

	@Override
	public void onCallPaused(String callid) {

		Log4Util.i(Device.TAG, "[DeviceImpl - onCallPaused] " + callid);
		if (runningType == RunningType.RunningType_Voip) {
			sendToTarget(CallbackHandler.WHAT_VOIP_CALL,
					CallbackHandler.VOIP_CALL_PAUSED, -1, callid);
		}
	}

	@Override
	public void onCallPausedByRemote(String callid) {
		Log4Util.i(Device.TAG, "[DeviceImpl - onCallPausedByRemote] " + callid);
		if (runningType == RunningType.RunningType_Voip) {
			sendToTarget(CallbackHandler.WHAT_VOIP_CALL,
					CallbackHandler.VOIP_CALL_REMOTE_PAUSED, -1, callid);
		}
	}

	@Override
	public void onCallReleased(final String callid) {
		Log4Util.i(Device.TAG, "[DeviceImpl - onCallReleased] " + callid);
		boolean isUpwards = false;
		try {
			synchronized (incomingCallIDArray) {
				if (incomingCallIDArray.size() > 0) {
					for (String element : incomingCallIDArray) {
						if (element.equals(callid)) {
							isUpwards = true;
							incomingCallIDArray.clear();
							Log4Util.i(Device.TAG,
									"[DeviceImpl - onCallReleased] removed incoming callid success, it's "
											+ callid + ", size = "
											+ incomingCallIDArray.size());
						} else {
							Log4Util.i(
									Device.TAG,
									"[DeviceImpl - onCallReleased] removed incoming callid failed, it's no equals, callid: "
											+ callid + ", first = " + element);
						}
					}
				}
			}

			synchronized (outgoingCallIDArray) {
				if (outgoingCallIDArray.size() > 0) {
					for (String element : outgoingCallIDArray) {
						if (element.equals(callid)) {
							isUpwards = true;
							outgoingCallIDArray.clear();
							Log4Util.i(Device.TAG,
									"[DeviceImpl - onCallReleased] removed outgoing callid success, it's "
											+ callid + ", size = "
											+ incomingCallIDArray.size());
						} else {
							Log4Util.i(
									Device.TAG,
									"[DeviceImpl - onCallReleased] removed outgoing callid failed, it's no equals, callid: "
											+ callid + ", first = " + element);
						}
					}
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
			isUpwards = true;
		}
		enableLoudsSpeaker(false);
		NativeInterface.UNInitAudioDevice();
		confpwd = null;
		// if another callid is same current callid, handup current call
		if (isUpwards) {
			keepingCall = false;
			// set autojoin true, that
			isAutoJoin = true;
			makeCallType = null;
			incomingCallType = null;
			mOriginalConferenceNo = null;
			ccpCallingID = null;
			this.mediaManager.stop();
			Log4Util.i(Device.TAG,
					"[DeviceImpl - onIncomingCallReceived] Current SDK  runningType "
							+ runningType.getValue());

			// If isUpwards is true, is VoIP.
			if (runningType == RunningType.RunningType_Voip) {
				runningType = RunningType.RunningType_None;
				sendToTarget(CallbackHandler.WHAT_VOIP_CALL,
						CallbackHandler.VOIP_CALL_RELEASED, -1, callid);
			}
			Log4Util.e(Device.TAG,
					"[DeviceImpl - onCallReleased] upwards callid: " + callid);

			// enable p2p server.
			setFirewallPolicy(1);
		}

	}

	@Override
	public void onCallTransfered(String callid, String destionation) {
		if (runningType == RunningType.RunningType_Voip) {

			Log4Util.i(Device.TAG, "[DeviceImpl - onCallTransfered] " + callid);
			sendToTarget(CallbackHandler.WHAT_VOIP_CALL,
					CallbackHandler.VOIP_CALL_TRANSFERRED, -1, callid + " ,"
							+ destionation);
		}
	}

	@Override
	public void onConnectError(int reason) {
		Log4Util.i(Device.TAG, "[DeviceImpl - onConnectError] " + reason);
		online = false;

		Reason r = Reason.UNKNOWN;
		switch (reason) {
		case 0:
			r = Reason.UNKNOWN;
			break;
		case 1:
			r = Reason.NOTRESPONSE;
			break;
		case 2:
			r = Reason.AUTHFAILED;
			break;
		case 3:
			r = Reason.DECLINED;
			break;
		case 4:
			r = Reason.NOTFOUND;
			break;
		case 5:
			r = Reason.CALLMISSED;
			break;
		case 6:
			r = Reason.BUSY;
			break;
		case 7:
		case SdkErrorCode.SDK_HTTP_ERROR:
		case SdkErrorCode.SDK_REQUEST_TIMEOUT:
			r = Reason.NOTNETWORK;
			break;
		case 8:
			if (this.callControlManager != null) {
				this.callControlManager.getSoftSwitchAddress();
			}
			break;
		case 9:
			r = Reason.KICKEDOFF;
			break;

		case 11:
			/**
			 * version 3.5 Operation SDK (Demo) limit client privately with
			 * other words group cooperation
			 */
			r = Reason.INVALIDPROXY; // soft switching address is Legitimate
			break;
		}
		Log4Util.i(Device.TAG, "[DeviceImpl - onConnectError] " + r.name());
		sendToTarget(CallbackHandler.WHAT_DEVICE,
				CallbackHandler.WHAT_ON_DISCONNECT, -1, r);
	}

	@Override
	public void onConnected() {
		online = true;
		Log4Util.i(Device.TAG, "[DeviceImpl - onConnected] connect success.");
		sendToTarget(CallbackHandler.WHAT_DEVICE,
				CallbackHandler.WHAT_ON_CONNECT, -1, null);
	}

	@Override
	public void onDisconnect() {
		Log4Util.i(Device.TAG, "[DeviceImpl - onDisconnect] connect failed.");
		onConnectError(1);
	}

	@Override
	public void onDtmfReceived(String callid, char dtmf) {
		Log4Util.i(Device.TAG, "[DeviceImpl - onDtmfReceived] callid: "
				+ callid + ", dtmf: " + dtmf);
		if (getCCPListenerInfo().mOnVoIPListener != null) {
			// getCCPListenerInfo().mOnVoIPListener.onDtmfReceived(callid,
			// dtmf);
		}
	}

	// check token expires when make call
	private boolean checkCallToken() {
		return true;
	}

	@Override
	public void onIncomingCallReceived(final CallType type,
			final String callid, final String caller) {
		Log4Util.i(Device.TAG,
				"[DeviceImpl - onIncomingCallReceived] onIncoming received: Type: "
						+ type + ", callid: " + callid + ", caller: " + caller);

		// If the current is not idle�� And the current state is not VOIP.
		// Ignore the call request
		if (runningType != RunningType.RunningType_None || isHoldSystemCall()) {
			Log4Util.i(Device.TAG,
					"[DeviceImpl - onIncomingCallReceived] Current is not idle , runningType "
							+ runningType.getValue() + " , then reject.");
			// busy
			if (this.callControlManager != null) {
				this.callControlManager.rejectCall(callid, 6);
			}
			return;
		}
		Log4Util.i(Device.TAG,
				"[DeviceImpl - onIncomingCallReceived] Current SDK  runningType "
						+ runningType.getValue());
		try {
			// handle multi-way call, just support one
			synchronized (incomingCallIDArray) {

				if (incomingCallIDArray.size() > 0) {
					// TODO: relase this callid.
					releaseCall(callid);
					Log4Util.i(
							Device.TAG,
							"[DeviceImpl - onIncomingCallReceived] another call incoming, so release another callid: "
									+ callid);
					return;
				}

				incomingCallIDArray.add(callid);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}

		if (isKeepingCall() || isHoldSystemCall()) {
			Log4Util.d(Device.TAG,
					"Currently hold other call, system call or voip call.");
			return;
		}

		isIncomingCall = true;
		incomingCallType = type;
		if (this.incomingIntent != null) {
			// this is VOIP call.
			runningType = RunningType.RunningType_Voip;
			Intent intent = new Intent();
			intent.putExtra(CALLTYPE, type);
			intent.putExtra(CALLID, callid);
			intent.putExtra(CALLER, caller);

			String[] infos = VoiceUtil.split(
					this.callControlManager.getUserData(USERDATA_FOR_INVITE),
					";");
			if (infos != null) {
				// put remote params
				intent.putExtra(REMOTE, infos);

				for (int i = 0; i < infos.length; i++) {
					Log4Util.i("[DeviceImpl - onIncomingCallReceived] get remote caller params: "
							+ infos[i] + "\r\n");
				}
			}

			try {
				this.incomingIntent.send(getContext(), 0, intent);

				// play local ring
				this.mediaManager.queueSound(MediaManager.StockSound.INCOMING,
						RING_TIME, new MediaManager.SoundPlaybackListener() {
							public void onCompletion() {
								Log4Util.i("[DeviceImpl - onIncomingCallReceived]MediaManager.StockSound.INCOMING onCompletion.");

								if (isIncomingCall) {
									isIncomingCall = false;
									onMakeCallFailed(callid, 5);
								}
							}
						});

				// wake up screen
			} catch (PendingIntent.CanceledException e) {
				Log4Util.e(
						Device.TAG,
						"Unable to send PendingIntent for incoming connection: PendingIntent was canceled");
			}
		}
	}

	@Override
	public void onMakeCallFailed(final String callid, int reason) {
		Reason r = Reason.UNKNOWN;
		switch (reason) {
		case 0:
			r = Reason.UNKNOWN;
			break;
		case 1:
			r = Reason.NOTRESPONSE;
			break;
		case 2:
			r = Reason.AUTHFAILED;
			break;
		case 3:
			r = Reason.DECLINED;
			break;
		case 4:
			r = Reason.NOTFOUND;
			break;
		case 5:
			r = Reason.CALLMISSED;
			break;
		case 408:
			r = Reason.TIME_OUT;
			break;
		case 488: // TODO:SDK增加处理一种错误码：488 媒体协商失败
			r = Reason.MEDIACONSULTFAILED;
			break;
		case 700: // -- 700 第三方鉴权地址连接失败
			r = Reason.AUTHADDRESSFAILED;
			break;
		case 701: // new code 701 , old 410-- 701 第三方主账号余额不足
		case 410:
			r = Reason.MAINACCOUNTPAYMENT;
			break;
		case 702: // -- 702 第三方应用ID未找到
			r = Reason.MAINACCOUNTINVALID;
			break;
		case 704: // -- 704 第三方应用未上线限制呼叫已配置测试号码
			r = Reason.CALLERSAMECALLED;
			break;
		case 705:// -- 705 第三方鉴权失败，子账号余额
			r = Reason.SUBACCOUNTPAYMENT;
			break;
		case 707:// -- 705 呼入会议号已解散不存在
			r = Reason.CONFERENCE_NOT_EXIST;
			break;
		case 708:// -- 705 呼入会议号密码验证失败
			r = Reason.PASSWORD_ERROR;
			break;
		case 6:
			r = Reason.BUSY;
			break;

		case 10:

			r = Reason.OTHERVERSIONNOTSUPPORT;
			break;
		case SdkErrorCode.SDK_VERSION_NOTSUPPORT: // current SDK version is not
													// support ...

			r = Reason.VERSIONNOTSUPPORT;
			break;
		case SdkErrorCode.SDK_CALL_BUSY: // current SDK version is not idle ...

			r = Reason.LOCAL_CALL_BUSY;
			break;
		default:
			Log4Util.e(Device.TAG,
					"[DeviceImpl - onMakeCallFailed] found new reason: "
							+ reason + ", currentyly can't handle.");
			break;
		}

		this.keepingCall = false;

		Bundle bundle = getCloopenReasonBundle(CallbackHandler
				.getCloopenReason(reason, r.getValue()));
		bundle.putString(Device.CONFNO, mOriginalConferenceNo);

		RunningType _rRunningType = runningType;
		runningType = RunningType.RunningType_None;

		if (_rRunningType == RunningType.RunningType_Interphone) {
			sendTarget(CallbackHandler.WHAT_REST_INTERPHONE,
					CallbackHandler.WHAT_START_INTERPHONE, bundle);

		} else if (_rRunningType == RunningType.RunningType_ChatRoom) {
			sendTarget(CallbackHandler.WHAT_REST_CHATROOM,
					CallbackHandler.WHAT_START_CHATROOM, bundle);

		} else if (_rRunningType == RunningType.RunningType_VideoConference) {
			sendTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
					CallbackHandler.WHAT_START_VIDEOCONFERENCE, bundle);

		} else {

			this.mediaManager.stop();
			if (r == Reason.BUSY || r == Reason.DECLINED) {
				runningType = RunningType.RunningType_Voip;
				// Remout reject this call.
				this.mediaManager.queueSound(MediaManager.StockSound.BUSY,
						new MediaManager.SoundPlaybackListener() {
							public void onCompletion() {
								Log4Util.i("[DeviceImpl - makeCall]MediaManager.StockSound.BUSY onCompletion.");
								onCallReleased(callid);
							}
						});
			} else if (r == Reason.CALLMISSED) {
				runningType = RunningType.RunningType_Voip;
			}

			bundle.clear();
			bundle.putString("callid", callid);
			bundle.putSerializable(Device.REASON, r);
			sendTarget(CallbackHandler.WHAT_VOIP_CALL,
					CallbackHandler.VOIP_CALL_FAILED, bundle);
		}

		Log4Util.i(Device.TAG, "[DeviceImpl - onMakeCallFailed] callid: "
				+ callid + ", reason: " + r.name());
	}

	@Override
	public void onTextMessageReceived(String sender, String message) {

		// Modified v3.0.1 send text messages (group or point to point) format
		// and analysis
		// event: 0,
		// id:
		// <sender>80013600000073</sender><receiver>80013600000074</receiver>,
		// message: <message>淡定淡定淡</message><userdata>(null)</userdata>, state:
		// 0

		try {
			Log4Util.i(Device.TAG, "[DeviceImpl - onTextMessageReceived] "
					+ sender + message);
			StringBuffer buffer = new StringBuffer("<Response>");
			buffer.append(sender).append(message).append("</Response>");
			Response doParser = ApiParser.doParser(
					ApiParser.KEY_TEXT_MESSAGE_ARRIVED,
					new ByteArrayInputStream(buffer.toString().getBytes()));
			if (doParser instanceof IMTextMsg) {
				IMTextMsg imTextMsg = (IMTextMsg) doParser;
				onReceiveInstanceMessage(imTextMsg);
			} else {
				Log4Util.e(Device.TAG, "Response parser not IMTextMsg object.");
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/**
	 * 返回发送消息结果
	 *
	 * 200: 对方收到；100、202、404、487、477: 发送到服务器，对方未收到； 其他：失败
	 *
	 * @version for version 3.4.1.1 before
	 */
	@Deprecated
	public void onMessageSendReport(String msgid, int status) {
		Log4Util.d(Device.TAG,
				"[DeviceImpl - onMessageSendReport] receive message report that msgId :"
						+ msgid + " , status " + status);
		;
		String sendStatus = IMTextMsg.MESSAGE_REPORT_SEND;
		switch (status) {
		case 200:
			sendStatus = IMTextMsg.MESSAGE_REPORT_RECEIVE;
			break;

		case 100:
		case 202:
		case 404:
		case 408:
		case 477:
			sendStatus = IMTextMsg.MESSAGE_REPORT_SEND;
			break;
		default:
			sendStatus = IMTextMsg.MESSAGE_REPORT_FAILED;
			break;
		}
		if (IMTextMsg.MESSAGE_REPORT_FAILED.equals(sendStatus)) {
			onSendInstanceMessage(CallbackHandler.getCloopenReason(status),
					new IMTextMsg(msgid, sendStatus));
		} else {
			onSendInstanceMessage(CallbackHandler.getCloopenReason(0),
					new IMTextMsg(msgid, sendStatus));
		}
	}

	/**
	 * 返回发送消息结果
	 *
	 * 200: 对方收到；100、202、404、487、477: 发送到服务器，对方未收到； 其他：失败
	 *
	 * @version for version 3.4.1.1 before
	 */
	public void onMessageSendReport(String msgid, String date, int status) {
		Log4Util.d(Device.TAG,
				"[DeviceImpl - onMessageSendReport] receive message report that msgId :"
						+ msgid + " , status " + status + " , date " + date);
		String sendStatus = IMTextMsg.MESSAGE_REPORT_SEND;
		switch (status) {
		case 200:
			sendStatus = IMTextMsg.MESSAGE_REPORT_RECEIVE;
			break;

		case 100:
		case 202:
		case 404:
		case 408:
		case 477:
			sendStatus = IMTextMsg.MESSAGE_REPORT_SEND;
			break;
		default:
			sendStatus = IMTextMsg.MESSAGE_REPORT_FAILED;
			break;
		}

		int reason = status;
		if (!IMTextMsg.MESSAGE_REPORT_FAILED.equals(sendStatus)) {
			reason = 0;
		}
		IMTextMsg imTextMsg = new IMTextMsg(msgid, sendStatus);
		imTextMsg.setDateCreated(date);
		onSendInstanceMessage(CallbackHandler.getCloopenReason(reason),
				imTextMsg);
	}

	@Override
	public void onCallBack(int status, String self, String dest) {
		sendToTarget(CallbackHandler.WHAT_VOIP_CALL,
				CallbackHandler.VOIP_MAKECALLBACK, status, self + "," + dest);
	}

	/******************************************* End *********************************************************/

	public int describeContents() {
		return 0;
	}

	public void writeToParcel(Parcel out, int flags) {
		out.writeSerializable(this.uuid);
	}

	public static final Parcelable.Creator<DeviceImpl> CREATOR = new Parcelable.Creator<DeviceImpl>() {
		public DeviceImpl createFromParcel(Parcel in) {
			UUID uuid = (UUID) in.readSerializable();
			return CCPCallImpl.getInstance().findDeviceByUUID(uuid);
		}

		public DeviceImpl[] newArray(int size) {
			throw new UnsupportedOperationException();
		}
	};

	/**
	 * @return the context
	 */
	public Context getContext() {
		return context;
	}

	/**
	 * @return the capabilityToken
	 */
	public String getCapabilityToken() {
		return capabilityToken;
	}

	/**
	 * @return the userAgentParams
	 */
	public Map<String, String> getUserAgentParams() {
		return userAgentParams;
	}

	/**********************************************************************
	 * voice message *
	 **********************************************************************/

	// ui
	@Override
	public void onRecordingTimeOut(long mills) {
		voiceRecording = false;
		if (getCCPListenerInfo().mOnIMListener != null) {

			getCCPListenerInfo().mOnIMListener.onRecordingTimeOut(mills);
		}
	}

	// ui
	@Override
	public void onRecordingAmplitude(double amplitude) {
		if (getCCPListenerInfo().mOnIMListener != null) {

			getCCPListenerInfo().mOnIMListener.onRecordingAmplitude(amplitude);
		}
	}

	@Override
	public void onFinishedPlaying() {
		voicePlaying = false;
		if (getCCPListenerInfo().mOnIMListener != null) {

			getCCPListenerInfo().mOnIMListener.onFinishedPlaying();
		}
	}

	/**
	 *
	 * <p>
	 * Title: voiceRecording
	 * </p>
	 * <p>
	 * Description:
	 * </p>
	 *
	 * @version 3.4.1.2
	 * @return
	 */
	public boolean isVoiceRecording() {
		return voiceRecording;
	}

	/**
	 *
	 * <p>
	 * Title: voicePlaying
	 * </p>
	 * <p>
	 * Description:
	 * </p>
	 *
	 * @version 3.4.1.2
	 * @return
	 */
	public boolean isVoicePlaying() {
		return voicePlaying;
	}

	@Override
	public long getVoiceDuration(String fileName) {
		mediaPlayManager.setSource(fileName, true);
		return mediaPlayManager.getMediaLength();
	}

	/**********************************************************************
	 * Interphone *
	 **********************************************************************/

	@Override
	public void startInterphone(String[] members, String appid) {
		if (runningType == RunningType.RunningType_None) {
			if (this.callControlManager != null) {
				this.callControlManager.startInterphone(members, "1", appid);
			}
		} else {
			onInterphoneState(
					CallbackHandler
							.getCloopenReason(SdkErrorCode.SDK_CALL_BUSY),
					null);
		}
	}

	@Override
	public void joinInterphone(String confNo) {
		if (runningType == RunningType.RunningType_None) {
			mOriginalConferenceNo = confNo;
			clearUserData();
			ccpCallingID = makeCallByRunningType(CallType.VOICE, confNo,
					RunningType.RunningType_Interphone);
		} else {
			onInterphoneState(
					CallbackHandler
							.getCloopenReason(SdkErrorCode.SDK_CALL_BUSY),
					confNo);
		}

	}

	@Override
	public boolean exitInterphone() {
		runningType = RunningType.RunningType_None;
		if (ccpCallingID != null) {
			releaseCall(ccpCallingID);
			ccpCallingID = null;
			return true;
		}
		return false;
	}

	@Override
	public void controlMic(String confNo) {
		if (runningType == RunningType.RunningType_Interphone) {
			if (this.callControlManager != null) {
				this.callControlManager.controlMIC(confNo);
			}
		} else {
			onControlMicState(
					CallbackHandler
							.getCloopenReason(SdkErrorCode.SDK_CALL_BUSY),
					null);
		}
	}

	@Override
	public void releaseMic(String confNo) {
		if (runningType == RunningType.RunningType_Interphone) {
			if (this.callControlManager != null) {
				this.callControlManager.releaseMic(confNo);
			}
		} else {
			onReleaseMicState(CallbackHandler
					.getCloopenReason(SdkErrorCode.SDK_CALL_BUSY));
		}
	}

	@Override
	public void onInterphoneState(CloopenReason reason, String confNo) {
		if (reason != null && !reason.isError()
				&& runningType == RunningType.RunningType_None) {
			mOriginalConferenceNo = confNo;
			clearUserData();
			ccpCallingID = makeCallByRunningType(CallType.VOICE, confNo,
					RunningType.RunningType_Interphone);
		} else {

			runningType = RunningType.RunningType_None;
			Bundle bundle = getCloopenReasonBundle(reason);
			bundle.putString(Device.CONFNO, confNo);
			sendTarget(CallbackHandler.WHAT_REST_INTERPHONE,
					CallbackHandler.WHAT_START_INTERPHONE, bundle);
		}
	}

	@Override
	public void onControlMicState(CloopenReason reason, String speaker) {
		Bundle bundle = getCloopenReasonBundle(reason);
		bundle.putString("speaker", speaker);
		sendTarget(CallbackHandler.WHAT_REST_INTERPHONE,
				CallbackHandler.WHAT_CONTROL_MIC, bundle);
	}

	@Override
	public void onReleaseMicState(CloopenReason reason) {

		Bundle bundle = getCloopenReasonBundle(reason);
		bundle.putString("speaker", null);
		sendTarget(CallbackHandler.WHAT_REST_INTERPHONE,
				CallbackHandler.WHAT_RELEASE_MIC, bundle);
	}

	@Override
	public void onPushMessageArrived(String body) {
		if (this.callControlManager == null) {
			return;
		}
		try {
			Response response = ApiParser.doParser(
					ApiParser.KEY_MESSAGE_ARRIVED, new ByteArrayInputStream(
							body.getBytes()));
			if (response instanceof InterphoneMsg) {
				onReceiveInterphoneMsg((InterphoneMsg) response);
			} else if (response instanceof ChatroomMsg) {
				// ChatRoom ...
				// To be removed or dissolution of the conference timely release
				// of resources,
				if (response instanceof ChatroomDismissMsg
						|| response instanceof ChatroomRemoveMemberMsg) {
					if (response instanceof ChatroomRemoveMemberMsg) {
						ChatroomRemoveMemberMsg removeMemberMsg = (ChatroomRemoveMemberMsg) response;
						if (removeMemberMsg != null
								&& removeMemberMsg.getWho().equals(
										this.callControlManager
												.getUserAgentConfig().getSid())) {
							String roomNo = removeMemberMsg.getRoomNo();
							if (!TextUtils.isEmpty(roomNo)
									&& roomNo.equals(mOriginalConferenceNo)) {
								exitChatroom();
							}
						}
					} else {
						ChatroomDismissMsg dismissMsg = (ChatroomDismissMsg) response;
						String roomNo = dismissMsg.getRoomNo();
						if (!TextUtils.isEmpty(roomNo)
								&& roomNo.equals(mOriginalConferenceNo)) {
							exitChatroom();
						}
					}
				}
				onReceiveChatroomMsg((ChatroomMsg) response);

			} else if (response instanceof InstanceMsg) {
				onReceiveInstanceMessage((InstanceMsg) response);

				// Video Conference
			} else if (response instanceof VideoConferenceMsg) {
				// To be removed or dissolution of the conference timely release
				// of resources,
				if (response instanceof VideoConferenceDismissMsg
						|| response instanceof VideoConferenceRemoveMemberMsg) {
					if (response instanceof VideoConferenceRemoveMemberMsg) {
						VideoConferenceRemoveMemberMsg removeMemberMsg = (VideoConferenceRemoveMemberMsg) response;
						String conferenceId = removeMemberMsg.getConferenceId();
						if (removeMemberMsg != null
								&& removeMemberMsg.getWho().equals(
										this.callControlManager
												.getUserAgentConfig().getSid())) {
							// if current conference ID defferent.
							// igore
							if (!TextUtils.isEmpty(conferenceId)
									&& conferenceId
											.equals(mOriginalConferenceNo)) {

								exitVideoConference();
							}
						}
					} else {
						VideoConferenceDismissMsg dismissMsg = (VideoConferenceDismissMsg) response;
						String conferenceId = dismissMsg.getConferenceId();
						if (!TextUtils.isEmpty(conferenceId)
								&& conferenceId.equals(mOriginalConferenceNo)) {

							exitVideoConference();
						}
					}
				}

				onReceiveVideoConferenceMsg((VideoConferenceMsg) response);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void onReceiveInterphoneMsg(InterphoneMsg msg) {
		Bundle bundle = new Bundle();
		bundle.putSerializable("InterphoneMsg", msg);
		sendTarget(CallbackHandler.WHAT_REST_INTERPHONE,
				CallbackHandler.WHAT_RECEIVE_INTERPHONE_MESSAGE, bundle);
	}

	public void onReceiveChatroomMsg(ChatroomMsg msg) {
		Bundle bundle = new Bundle();
		bundle.putSerializable("ChatroomMsg", msg);
		sendTarget(CallbackHandler.WHAT_REST_CHATROOM,
				CallbackHandler.WHAT_RECEIVE_CHATROOM_MESSAGE, bundle);
	}

	public void onReceiveVideoConferenceMsg(VideoConferenceMsg VideoMsg) {
		Bundle bundle = new Bundle();
		bundle.putSerializable("VideoConferenceMsg", VideoMsg);
		sendTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
				CallbackHandler.WHAT_RECEIVE_VIDEO_CONGERENCE_MESSAGE, bundle);
	}

	@Override
	public void queryMembersWithInterphone(String confNo) {
		if (this.callControlManager != null) {
			this.callControlManager.queryMembersWithInterphone(confNo);
		}
	}

	@Override
	public void onInterphoneMembers(CloopenReason reason,
			List<InterphoneMember> member) {
		if (keepingCall && member != null) {
			for (InterphoneMember interphoneMember : member) {
				if (interphoneMember.type != null
						&& this.callControlManager != null
						&& interphoneMember.voipId
								.equals(this.callControlManager
										.getUserAgentConfig().getSid())) {
					interphoneMember.online = "1";
				}
			}
		}

		Bundle bundle = getCloopenReasonBundle(reason);
		ArrayList arrayList = new ArrayList();
		arrayList.add(member);
		bundle.putParcelableArrayList("member", arrayList);
		sendTarget(CallbackHandler.WHAT_REST_INTERPHONE,
				CallbackHandler.WHAT_QUERY_INTERPHONE_MEMBERS, bundle);
	}

	/**********************************************************************
	 * Chatroom *
	 **********************************************************************/
	@Deprecated
	public void startChatroom(String appId, String roomName, int square,
			String keywords, String pwd) {
		startChatroom(appId, roomName, square, keywords, pwd, true);
	}

	@Deprecated
	public void startChatroom(String appId, String roomName, int square,
			String keywords, String pwd, boolean isAutoClose) {
		startChatroom(appId, roomName, square, keywords, pwd, isAutoClose, 1,
				true);
	}

	/**
	 * @see #startChatroom(String, String, int, String, String, boolean, int,
	 *      boolean, boolean) The default value for this method that autojoin
	 *      the conference after create.
	 */
	public void startChatroom(String appId, String roomName, int square,
			String keywords, String pwd, boolean isAutoClose, int voiceMod,
			boolean isAutoDelete) {

		startChatroom(appId, roomName, square, keywords, pwd, isAutoClose,
				voiceMod, isAutoDelete, true);
	}

	/**
	 * @param appId
	 *            The application of ID
	 * @param conferenceName
	 *            The name that create.
	 * @param square
	 *            The biggest party in the number
	 * @param keywords
	 *            Business property, application defined
	 * @param conferencePwd
	 *            The room for the password, default null.
	 * @param isAutoClose
	 *            创建者推出是否自动解散
	 * @param voiceMod
	 *            0没有提示音有背景音，、1全部提示音、2无提示音无背景音。默认值为1全部提示音
	 * @param isAutoDelete
	 *            autoDelete 是否自动删除，默认值为1自动删除
	 * @version 3.6.3
	 */
	public void startChatroom(String appId, String roomName, int square,
			String keywords, String pwd, boolean isAutoClose, int voiceMod,
			boolean isAutoDelete, boolean isAutoJoin) {
		this.isAutoJoin = isAutoJoin;
		if (runningType == RunningType.RunningType_None) {
			if (this.callControlManager != null) {
				if (!TextUtils.isEmpty(pwd)) {
					setConfpwd(pwd);
					keywords = Cryptos
							.toBase64QES(Cryptos.SECRET_CONF_KEY, pwd);
				}
				this.callControlManager.startChatroom(appId, roomName, square,
						keywords, keywords, isAutoClose ? 0 : 1, voiceMod,
						isAutoDelete ? 1 : 0);
			}
		} else {
			onChatRoomState(
					CallbackHandler
							.getCloopenReason(SdkErrorCode.SDK_CALL_BUSY),
					null);
		}
	}

	@Override
	public void dismissChatroom(String appId, String roomNo) {
		if (this.callControlManager != null) {
			this.callControlManager.dismissChatroom(appId, roomNo);
		}
	}

	@Override
	public void removeMemberFromChatroom(String appId, String roomNo,
			String member) {
		if (this.callControlManager != null) {
			this.callControlManager.removeMemberFromChatroom(appId, roomNo,
					member);
		}
	}

	@Override
	public void joinChatroom(String roomNo, String pwd) {
		if (runningType == RunningType.RunningType_None) {
			mOriginalConferenceNo = ConferenceUtils
					.getOriginalConferenceNo(roomNo);
			clearUserData();
			if (!TextUtils.isEmpty(pwd)) {
				setConfpwd(pwd);
			}
			ccpCallingID = makeCallByRunningType(CallType.VOICE,
					mOriginalConferenceNo, RunningType.RunningType_ChatRoom);
		} else {
			onChatRoomState(
					CallbackHandler
							.getCloopenReason(SdkErrorCode.SDK_CALL_BUSY),
					null);
		}
	}

	@Override
	public boolean exitChatroom() {
		runningType = RunningType.RunningType_None;
		if (ccpCallingID != null) {
			releaseCall(ccpCallingID);
			ccpCallingID = null;
			return true;
		}
		return false;
	}

	@Override
	public void queryMembersWithChatroom(String roomNo) {
		if (this.callControlManager != null) {
			this.callControlManager.queryMembersWithChatroom(roomNo);
		}
	}

	@Override
	public void setChatroomMemberSpeakOpt(String appId, String roomNo,
			String member, int opt) {
		if (this.callControlManager != null) {
			this.callControlManager.setChatroomMemberSpeakOpt(appId, roomNo,
					member, opt);
		}
	}

	@Override
	public void onChatRoomState(CloopenReason reason, String confNo) {
		// if isAutoJoin true , then makeCall,
		if (reason != null && !reason.isError()
				&& runningType == RunningType.RunningType_None && isAutoJoin) {
			mOriginalConferenceNo = confNo;
			ConferenceUtils.procesConferenceNo(
					RunningType.RunningType_ChatRoom, confNo);
			clearUserData();
			ccpCallingID = makeCallByRunningType(CallType.VOICE, confNo,
					RunningType.RunningType_ChatRoom);
		} else {
			runningType = RunningType.RunningType_None;
			Bundle bundle = getCloopenReasonBundle(reason);
			bundle.putString(Device.CONFNO,
					ConferenceUtils.getProcesConferenceNo(confNo));
			sendTarget(CallbackHandler.WHAT_REST_CHATROOM,
					CallbackHandler.WHAT_START_CHATROOM, bundle);
			isAutoJoin = true;
		}
	}

	@Override
	public void onChatRoomDismiss(CloopenReason reason, String roomNo) {

		releaseCallNoneVoip();
		Bundle bundle = getCloopenReasonBundle(reason);
		bundle.putString(Device.CONFNO, roomNo);
		sendTarget(CallbackHandler.WHAT_REST_CHATROOM,
				CallbackHandler.WHAT_DISMISS_CHATROOM, bundle);
	}

	@Override
	public void onChatRoomRemoveMember(CloopenReason reason, String member) {

		if (this.callControlManager != null) {
			String sid = this.callControlManager.getUserAgentConfig().getSid();
			if (!TextUtils.isEmpty(sid) && sid.equals(member)) {
				releaseCall(ccpCallingID);
			}
		}

		Bundle bundle = getCloopenReasonBundle(reason);
		bundle.putString("member", member);
		sendTarget(CallbackHandler.WHAT_REST_CHATROOM,
				CallbackHandler.WHAT_REMOVE_MEMBER_CHATROOM, bundle);
	}

	@Override
	public void onSetChatroomSpeakOpt(CloopenReason reason, String member) {
		Bundle bundle = getCloopenReasonBundle(reason);
		bundle.putString("member", member);
		sendTarget(CallbackHandler.WHAT_REST_CHATROOM,
				CallbackHandler.WHAT_CHATROOM_MEMBER_SPRAK_OPT, bundle);
	}

	@Override
	public void setCodecEnabled(Codec codec, boolean enabled) {
		if (this.callControlManager != null) {
			this.callControlManager.setCodecEnabled(codec.getValue(), enabled);
		}
	}

	@Override
	public boolean getCodecEnabled(Codec codec) {
		if (this.callControlManager != null) {
			return this.callControlManager.getCodecEnabled(codec.getValue());
		}
		return true;
	}

	@Override
	public void setSrtpEnabled(String key) {
		if (this.callControlManager != null) {
			this.callControlManager.setSrtpEnabled(true, true, 3, key);
		}
	}

	@Override
	public void onChatRooms(CloopenReason reason, List<Chatroom> chatRoomList) {
		Bundle bundle = getCloopenReasonBundle(reason);
		List<Chatroom> procesChatrooms = ConferenceUtils
				.procesChatrooms(chatRoomList);
		ArrayList arrayList = new ArrayList();
		arrayList.add(procesChatrooms);
		bundle.putParcelableArrayList("chatroomList", arrayList);
		sendTarget(CallbackHandler.WHAT_REST_CHATROOM,
				CallbackHandler.WHAT_QUERY_CHATROOMS, bundle);
	}

	@Override
	public CameraInfo[] getCameraInfo() {
		if (this.callControlManager == null) {
			return null;
		}
		return this.callControlManager.getCameraInfo();
	}

	@Override
	public void selectCamera(int cameraIndex, int capabilityIndex, int fps,
			Rotate rotate, boolean force) {
		if (this.callControlManager != null) {
			this.callControlManager.selectCamera(cameraIndex, capabilityIndex,
					fps, rotate.getValue(), force);
		}
	}

	@Override
	public void onChatRoomMembers(CloopenReason reason,
			List<ChatroomMember> member) {
		Bundle bundle = getCloopenReasonBundle(reason);
		ArrayList arrayList = new ArrayList();
		arrayList.add(member);
		bundle.putParcelableArrayList("chatroomMembers", arrayList);
		sendTarget(CallbackHandler.WHAT_REST_CHATROOM,
				CallbackHandler.WHAT_QUERY_CHATROOM_MEMBERS, bundle);

	}

	@Override
	public void onChatRoomInvite(CloopenReason reason, String confNo) {
		Bundle bundle = getCloopenReasonBundle(reason);
		bundle.putString(Device.CONFNO, confNo);
		sendTarget(CallbackHandler.WHAT_REST_CHATROOM,
				CallbackHandler.WHAT_INVITE_JOIN_CHATROOM, bundle);
	}

	@Override
	public void queryChatrooms(String appId, String keywords) {
		if (this.callControlManager != null) {
			this.callControlManager.queryChatRooms(appId, keywords);
		}
	}

	@Override
	public void inviteMembersJoinChatroom(String[] members, String roomNo,
			String appId) {
		if (this.callControlManager != null) {
			this.callControlManager.inviteMembersJoinChatroom(members, roomNo,
					appId);
		}
	}

	@Override
	public void acceptCall(String callid, String accountid, CallType callType) {
		if (this.callControlManager != null) {

			if (incomingCallType == CallType.VIDEO) {

				if (mRemoteViewMap.get(accountid) != null) {
					this.callControlManager.setVideoView(accountid, mRemoteViewMap.get(accountid), null);
					// this.callControlManager.selectCamera(1, 0, 15, 0 ,
					// false);
					Log4Util.i(Device.TAG,
							"[DeviceImpl -  acceptCall] video view has been setting.");
				} else {
					Log4Util.w(Device.TAG,
							"[DeviceImpl -  acceptCall] video view is null.");
				}
			}

			this.callControlManager.acceptCallByMediaType(callid,
					callType.getValue());
		}
	}

	@Override
	public void updateCallType(String callid, CallType callType) {
		requestSwitchCallMediaType(callid, callType);
	}

	@Override
	public void requestSwitchCallMediaType(String callid, CallType callType) {
		if (this.callControlManager != null) {
			this.callControlManager
					.updateCallMedia(callid, callType.getValue());
		}
	}

	@Override
	public void answerCallTypeUpdate(String callid, int action) {
		responseSwitchCallMediaType(callid, action);
	}

	@Override
	public void responseSwitchCallMediaType(String callid, int action) {
		if (this.callControlManager != null) {
			this.callControlManager.answerCallMediaUpdate(callid, action);
		}
	}

	@Override
	public CallType getCallType(String callid) {
		if (this.callControlManager != null) {
			int type = this.callControlManager.getCallMediaType(callid);
			if (type == 0) {
				return CallType.VOICE;
			} else if (type == 1) {
				return CallType.VIDEO;
			}
		}
		return null;
	}

	@Override
	public int setAudioConfigEnabled(AudioType type, boolean enabled,
			AudioMode mode) {
		if (this.callControlManager == null) {
			return -1;
		}
		return this.callControlManager.setAudioConfigEnabled(type.getValue(),
				enabled, mode.getValue());
	}

	@Override
	public boolean getAudioConfig(AudioType type) {
		if (this.callControlManager == null) {
			return false;
		}
		return this.callControlManager.getAudioConfig(type.getValue());
	}

	@Override
	public AudioMode getAudioConfigMode(AudioType type) {
		if (this.callControlManager == null) {
			return null;
		}
		int mode = this.callControlManager.getAudioConfigMode(type.getValue());
		AudioMode audiomode = null;
		if (type == AudioType.AUDIO_NS) {
			switch (mode) {
			case 0:
				audiomode = AudioMode.kNsUnchanged;
				break;
			case 1:
				audiomode = AudioMode.kNsDefault;
				break;
			case 2:
				audiomode = AudioMode.kNsConference;
				break;
			case 3:
				audiomode = AudioMode.kNsLowSuppression;
				break;
			case 4:
				audiomode = AudioMode.kNsModerateSuppression;
				break;
			case 5:
				audiomode = AudioMode.kNsHighSuppression;
				break;
			case 6:
				audiomode = AudioMode.kNsVeryHighSuppression;
				break;
			default:
				audiomode = AudioMode.kNsDefault;
				break;
			}
		} else if (type == AudioType.AUDIO_AGC) {
			switch (mode) {
			case 0:
				audiomode = AudioMode.kAgcUnchanged;
				break;
			case 1:
				audiomode = AudioMode.kAgcDefault;
				break;
			case 2:
				audiomode = AudioMode.kAgcAdaptiveAnalog;
				break;
			case 3:
				audiomode = AudioMode.kAgcAdaptiveDigital;
				break;
			case 4:
				audiomode = AudioMode.kAgcFixedDigital;
				break;
			default:
				audiomode = AudioMode.kAgcUnchanged;
				break;
			}
		} else if (type == AudioType.AUDIO_EC) {
			switch (mode) {
			case 0:
				audiomode = AudioMode.kEcUnchanged;
				break;
			case 1:
				audiomode = AudioMode.kEcDefault;
				break;
			case 2:
				audiomode = AudioMode.kEcConference;
				break;
			case 3:
				audiomode = AudioMode.kEcAec;
				break;
			case 4:
				audiomode = AudioMode.kEcAecm;
				break;
			default:
				audiomode = AudioMode.kEcUnchanged;
				break;
			}
		}
		return audiomode;
	}

	@Override
	public void setVideoBitRates(int bitrates) {
		if (this.callControlManager != null) {
			this.callControlManager.setVideoBitRates(bitrates);
		}
	}

	@Override
	public int startRtpDump(String callid, int mediaType, String fileName,
			int direction) {
		if (this.callControlManager == null) {
			return -1;
		}
		return this.callControlManager.startRtpDump(callid, mediaType,
				fileName, direction);
	}

	@Override
	public int stopRtpDump(String callid, int mediaType, int direction) {
		if (this.callControlManager == null) {
			return -1;
		}
		return this.callControlManager
				.stopRtpDump(callid, mediaType, direction);
	}

	@Override
	public void onCallMediaUpdateRequest(String callid, int reason) {
		if (runningType == RunningType.RunningType_Voip) {
			sendToTarget(CallbackHandler.WHAT_VOIP_CALL,
					CallbackHandler.VOIP_CALL_VIDEO_UPDATE_REQUEST, reason,
					callid);
		}
	}

	@Override
	public void onCallMediaUpdateResponse(String callid, int reason) {
		if (runningType == RunningType.RunningType_Voip) {
			sendToTarget(CallbackHandler.WHAT_VOIP_CALL,
					CallbackHandler.VOIP_CALL_VIDEO_UPDATE_RESPONSE, reason,
					callid);
		}
	}

	@Override
	public void onCallVideoRatioChanged(String callid, String resolution,
			int state) {
		if (runningType == RunningType.RunningType_Voip) {
			sendToTarget(CallbackHandler.WHAT_VOIP_CALL,
					CallbackHandler.VOIP_REMOTE_VIDEO_RATIO, -1, callid + ","
							+ resolution);
		} else if (runningType == RunningType.RunningType_VideoConference) {
			sendToTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
					CallbackHandler.VIDEOCONF_REMOTE_VIDEO_RATIO, -1, callid
							+ "," + resolution);
		}
	}

	@Override
	public void onCallMediaInitFailed(String callid, int reason) {
		if (runningType == RunningType.RunningType_Voip) {
			sendToTarget(CallbackHandler.WHAT_VOIP_CALL,
					CallbackHandler.VOIP_MEDIA_INIT_FAILED, reason, callid);
		}
	}

	/**
	 * V3.0.1 amended to send the return value is String (uniquely identifies
	 * the message)
	 */
	@Override
	public String sendInstanceMessage(String receiver, String text,
			String attached, String userData) {
		if (this.callControlManager == null) {
			return null;
		}
		String uniqueID = null;
		if (text != null && attached == null) {
			int errorCode = SdkErrorCode.REQUEST_SUCCESS;
			if (State.ONLINE == isOnline()) {
				if (text.getBytes().length <= 2000
						&& (TextUtils.isEmpty(userData) || userData.getBytes().length <= 255)) {
					uniqueID = callControlManager.sendTextMessage(receiver,
							text, userData);
					errorCode = SdkErrorCode.REQUEST_SUCCESS;
				} else {
					errorCode = SdkErrorCode.SDK_TEXT_LENGTH_LIMIT;
				}

			} else {
				errorCode = SdkErrorCode.SDK_NOT_REGISTED;
			}
			final int code = errorCode;
			if (errorCode != SdkErrorCode.REQUEST_SUCCESS) {
				final String msgId = callControlManager.getUniqueID();
				uniqueID = msgId;
				this.callControlManager.postCommand(new Runnable() {

					@Override
					public void run() {
						try {
							Thread.sleep(500L);
							onSendInstanceMessage(CallbackHandler
									.getCloopenReason(code), new IMTextMsg(
									msgId, IMTextMsg.MESSAGE_REPORT_FAILED));
						} catch (InterruptedException e) {
							e.printStackTrace();
						}

					}
				});
			}
			return uniqueID;
		}

		if (attached != null && this.callControlManager != null) {
			uniqueID = callControlManager.getUniqueID();
			callControlManager.sendMediaMsg(uniqueID, attached, receiver,
					userData);
		}
		return uniqueID;
	}

	@Override
	public void startVideoRecording(String receiver, String path,
			boolean chunked) {

	}

	@Override
	// Note: 3.0.1 Add the parameters, namely the unique identifier for the
	// audio message
	// and delete parameters type for voice delay...
	public String startVoiceRecording(String receiver, String path,
			boolean chunked, String userData) throws CCPRecordException {

		synchronized (mLocks) {
			if (!VoiceUtil.isAvaiableSpace(1)) {
				throw new CCPRecordException(
						"The current did not load SDcard or lack of SDcard memory space.");
			}
			if (this.callControlManager == null) {
				return null;
			}
			String uniqueID = callControlManager.getUniqueID();
			voiceRecording = true;
			if (chunked) {
				audRecordManager
						.startRecord(uniqueID, path, receiver, userData);
			} else {
				audRecordManager.startRecord(uniqueID, path);
			}
			return uniqueID;
		}
	}

	@Override
	public void onSendInstanceMessage(CloopenReason reason, InstanceMsg data) {
		Bundle bundle = getCloopenReasonBundle(reason);
		bundle.putSerializable("InstanceMsg", data);
		sendTarget(CallbackHandler.WHAT_REST_IM,
				CallbackHandler.WHAT_SEND_MEDIA_MSG, bundle);
	}

	@Override
	public void onDownloadAttached(CloopenReason reason, String fileName) {
		Bundle bundle = getCloopenReasonBundle(reason);
		bundle.putString("fileName", fileName);
		sendTarget(CallbackHandler.WHAT_REST_IM,
				CallbackHandler.WHAT_DOWNLOAD_MEDIA_MSG, bundle);
	}

	@Override
	public void onReceiveInstanceMessage(InstanceMsg msg) {
		Bundle bundle = new Bundle();
		bundle.putSerializable("InstanceMsg", msg);
		sendTarget(CallbackHandler.WHAT_REST_IM,
				CallbackHandler.WHAT_RECEIVE_INSTANCE_MESSAGE, bundle);
	}

	@Override
	public void confirmIntanceMessage(String[] msgId) {
		if (this.callControlManager != null) {

			this.callControlManager.confirmDownloadMediaMessage(msgId);
		}
	}

	@Override
	public void downloadAttached(ArrayList<DownloadInfo> urlList) {
		if (this.callControlManager != null) {
			this.callControlManager.downloadAttachmentFiles(urlList);
		}
	}

	@Override
	public void stopVoiceRecording() {
		synchronized (mLocks) {
			voiceRecording = false;
			Log4Util.i("stopVoiceRecording");
			this.audRecordManager.stopRecord();
		}
	}

	@Override
	public void playVoiceMsg(String path) {
		playVoiceMsg(path, true);
	}

	@Override
	public void playVoiceMsg(String path, boolean speakerOn) {
		voicePlaying = true;
		mediaPlayManager.setSource(path, speakerOn);
		mediaPlayManager.play(this);
	}

	@Override
	public void stopVoiceMsg() {
		voicePlaying = false;
		mediaPlayManager.stop();
	}

	@Override
	public void cancelVoiceRecording() {
		synchronized (mLocks) {
			voiceRecording = false;
			this.audRecordManager.cancleRecord(true);
			this.audRecordManager.stopRecord();
		}
	}

	@Override
	public void onConfirmIntanceMessage(CloopenReason reason) {
		Bundle bundle = getCloopenReasonBundle(reason);
		sendTarget(CallbackHandler.WHAT_REST_IM,
				CallbackHandler.WHAT_CONFIRM_DOWNLOAD_MEDIAMESSAGE, bundle);
	}

	// ---------------------------------------------------------------------
	// for SDK version 3.3
	@Override
	public String getVersion() {
		if (this.callControlManager == null) {
			return null;
		}
		return this.callControlManager.getVersion();
	}

	/**
	 * @Override 获取通话的统计信息后，根据丢包率和延迟时间，判断通话的网络状况。也可以统计通话的网络流量。
	 *           获取统计信息的间隔要在4秒以上。统计信息中，重要的是丢包率和延迟时间，能够反映网络状况。
	 *
	 */
	public CallStatisticsInfo getCallStatistics(CallType callType) {
		if (this.callControlManager == null) {
			return null;
		}

		String callState = this.callControlManager.getCallStatistics(callType
				.getValue());
		Log4Util.d("getCallStatistics", callState);
		String[] split = callState.split("#");
		CallStatisticsInfo callStatisticsInfo = null;
		if (split != null && split.length >= 9) {
			try {
				callStatisticsInfo = new CallStatisticsInfo();
				callStatisticsInfo.setFractionLost(Integer.parseInt(split[0]));
				callStatisticsInfo
						.setCumulativeLost(Integer.parseInt(split[1]));
				callStatisticsInfo.setExtendedMax(Integer.parseInt(split[2]));
				callStatisticsInfo.setJitterSamples(Integer.parseInt(split[3]));
				callStatisticsInfo.setRttMs(Integer.parseInt(split[4]));
				callStatisticsInfo.setBytesSent(Integer.parseInt(split[5]));
				callStatisticsInfo.setPacketsSent(Integer.parseInt(split[6]));
				callStatisticsInfo.setBytesReceived(Integer.parseInt(split[7]));
				callStatisticsInfo.setPacketsReceived(Integer
						.parseInt(split[8]));
			} catch (NumberFormatException e) {
			}
		}
		return callStatisticsInfo;
	}

	// ----------------------------------------------------------------------
	// After the underlying SDK and P2P function, is as follows:
	// for SDK version 3.4
	public int setFirewallPolicy(int policy) {
		if (this.callControlManager == null) {
			return -1;
		}
		return this.callControlManager.setFirewallPolicy(policy);
	}

	/**
	 * p2p enabled.
	 */
	@Override
	public void onFirewallPolicyEnabled() {
		sendToTarget(CallbackHandler.WHAT_DEVICE,
				CallbackHandler.FIREWALL_POLICY_ENABLED, -1, null);
	}

	/**
	 * @version 3.5
	 */
	@Override
	public void setOnVoIPListener(OnVoIPListener onVoIPListener) {

		getCCPListenerInfo().mOnVoIPListener = onVoIPListener;
	}

	/**
	 * @version 3.5
	 */
	@Override
	public void setOnChatroomListener(OnChatroomListener onChatroomListener) {

		getCCPListenerInfo().mOnChatroomListener = onChatroomListener;
	}

	/**
	 * @version 3.5
	 */
	@Override
	public void setOnInterphoneListener(
			OnInterphoneListener onInterphoneListener) {

		getCCPListenerInfo().mOnInterphoneListener = onInterphoneListener;
	}

	/**
	 * @version 3.5
	 */
	@Override
	public void setOnIMListener(OnIMListener onIMListener) {
		getCCPListenerInfo().mOnIMListener = onIMListener;
	}

	/**
	 * 3.5
	 */
	@Override
	public void setOnVideoConferenceListener(
			OnVideoConferenceListener onVideoConferenceListener) {
		getCCPListenerInfo().mOnVideoConferenceListener = onVideoConferenceListener;

	}

	@Deprecated
	public void startVideoConference(String appId, String conferenceName,
			int square, String keywords, String conferencePwd) {
		startVideoConference(appId, conferenceName, square, keywords,
				conferencePwd, true);
	}

	@Deprecated
	public void startVideoConference(String appId, String conferenceName,
			int square, String keywords, String conferencePwd,
			boolean isAutoClose) {

		startVideoConference(appId, conferenceName, square, keywords,
				conferencePwd, isAutoClose, 1, true);

	}

	/**
	 * @see #startVideoConference(String, String, int, String, String, boolean,
	 *      int, boolean, boolean) The default value for this method that
	 *      autojoin the conference after create.
	 */
	@Override
	public void startVideoConference(String appId, String conferenceName,
			int square, String keywords, String conferencePwd,
			boolean isAutoClose, int voiceMod, boolean isAutoDelete) {
		startVideoConference(appId, conferenceName, square, keywords,
				conferencePwd, isAutoClose, voiceMod, isAutoDelete, true);
	}

	/**
	 * @param appId
	 *            The application of ID
	 * @param conferenceName
	 *            The name that create.
	 * @param square
	 *            The biggest party in the number
	 * @param keywords
	 *            Business property, application defined
	 * @param conferencePwd
	 *            The room for the password, default null.
	 * @param isAutoClose
	 *            创建者推出是否自动解散
	 * @param voiceMod
	 *            0没有提示音有背景音，、1全部提示音、2无提示音无背景音。默认值为1全部提示音
	 * @param isAutoDelete
	 *            autoDelete 是否自动删除，默认值为1自动删除
	 * @param isAutoJoin
	 *            isAutoJoin 是否自动加入会议
	 * @version 3.6.3
	 */
	@Override
	public void startVideoConference(String appId, String conferenceName,
			int square, String keywords, String conferencePwd,
			boolean isAutoClose, int voiceMod, boolean isAutoDelete,
			boolean isAutoJoin) {
		this.isAutoJoin = isAutoJoin;
		if (runningType == RunningType.RunningType_None) {
			if (this.callControlManager != null) {
				this.callControlManager.startVideoConference(appId,
						conferenceName, square, keywords, conferencePwd,
						isAutoClose ? 0 : 1, voiceMod, isAutoDelete ? 1 : 0);
			}
		} else {
			onVideoConferenceState(
					CallbackHandler
							.getCloopenReason(SdkErrorCode.SDK_CALL_BUSY),
					null);
		}

	}

	/*
	 * (non-Javadoc)
	 *
	 * @see
	 * com.hisun.phone.core.voice.Device#startMultiVideoConference(java.lang
	 * .String, java.lang.String, int, java.lang.String, java.lang.String,
	 * boolean, int, boolean, boolean)
	 */
	@Override
	public void startMultiVideoConference(String appId, String conferenceName,
			int square, String keywords, String conferencePwd,
			boolean isAutoClose, int voiceMod, boolean isAutoDelete,
			boolean isAutoJoin) {
		ConferenceOptions options = new ConferenceOptions();
		options.inAutoClose = isAutoClose;
		options.inVoiceMod = voiceMod;
		options.inMultiVideo = true;
		options.inAutoDelete = isAutoDelete;
		startVideoConference(appId, conferenceName, conferencePwd, isAutoJoin,
				options);
	}

	private void startVideoConference(String appId, String conferenceName,
			String conferencePwd, boolean isAutoJoin, ConferenceOptions options) {
		this.isAutoJoin = isAutoJoin;
		if (runningType == RunningType.RunningType_None) {
			if (this.callControlManager != null) {
				this.callControlManager.startVideoConference(appId,
						conferenceName, conferencePwd, options);
			}
		} else {
			onVideoConferenceState(
					CallbackHandler
							.getCloopenReason(SdkErrorCode.SDK_CALL_BUSY),
					null);
		}
	}

	@Override
	public void joinVideoConference(String conferenceId) {
		if (runningType == RunningType.RunningType_None) {
			mOriginalConferenceNo = ConferenceUtils
					.getOriginalConferenceNo(conferenceId);

			clearUserData();
			ccpCallingID = makeCallByRunningType(CallType.VIDEO,
					mOriginalConferenceNo,
					RunningType.RunningType_VideoConference);
		} else {
			onVideoConferenceState(
					CallbackHandler
							.getCloopenReason(SdkErrorCode.SDK_CALL_BUSY),
					null);
		}
	}

	@Override
	public boolean exitVideoConference() {
		runningType = RunningType.RunningType_None;
		if (ccpCallingID != null) {
			releaseCall(ccpCallingID);
			ccpCallingID = null;
			if (this.callControlManager != null) {
				this.callControlManager.removeVideoConferenceHandleMessage();
			}
			return true;
		}
		return false;
	}

	@Override
	public void queryMembersInVideoConference(String conferenceId) {
		if (this.callControlManager != null) {
			this.callControlManager.queryMembersInVideoConference(conferenceId);
		}
	}

	@Override
	public void queryVideoConferences(String appId, String keywords) {
		if (this.callControlManager != null) {
			this.callControlManager.queryVideoConferences(appId, keywords);
		}
	}

	@Override
	public void dismissVideoConference(String appId, String conferenceId) {
		if (this.callControlManager != null) {
			this.callControlManager.dismissVideoConference(appId, conferenceId);
		}
	}

	@Override
	public void removeMemberFromVideoConference(String appId,
			String conferenceId, String member) {
		if (this.callControlManager != null) {
			this.callControlManager.removeMemberFromVideoConference(appId,
					conferenceId, member);
		}
	}

	@Override
	public void switchRealScreenToVoip(String appId, String conferenceId,
			String voip) {
		if (this.callControlManager != null) {
			this.callControlManager.switchRealScreenToVoip(appId, conferenceId,
					voip);
		}
	}

	@Override
	public void onVideoConferenceState(CloopenReason reason, String conferenceId) {
		// if isAutoJoin true , then makeCall,
		if (reason != null && !reason.isError()
				&& runningType == RunningType.RunningType_None && isAutoJoin) {
			mOriginalConferenceNo = conferenceId;
			ConferenceUtils.procesConferenceNo(
					RunningType.RunningType_VideoConference, conferenceId);
			clearUserData();
			ccpCallingID = makeCallByRunningType(CallType.VIDEO, conferenceId,
					RunningType.RunningType_VideoConference);
		} else {
			runningType = RunningType.RunningType_None;
			Bundle bundle = getCloopenReasonBundle(reason);
			bundle.putString(Device.CONFNO,
					ConferenceUtils.getProcesConferenceNo(conferenceId));
			sendTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
					CallbackHandler.WHAT_START_VIDEOCONFERENCE, bundle);
			isAutoJoin = false;
		}

	}

	@Override
	public void onVideoConferenceMembers(CloopenReason reason,
			List<VideoConferenceMember> members) {

		Bundle bundle = getCloopenReasonBundle(reason);
		ArrayList arrayList = new ArrayList();
		arrayList.add(members);
		bundle.putParcelableArrayList("videoMembers", arrayList);
		sendTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
				CallbackHandler.WHAT_QUERY_VIDEOCONFERENCE_MEMBERS, bundle);
	}

	@Override
	public void onVideoConferences(CloopenReason reason,
			List<VideoConference> conferences) {

		Bundle bundle = getCloopenReasonBundle(reason);
		conferences = ConferenceUtils.procesVideoConferences(conferences);
		ArrayList arrayList = new ArrayList();
		arrayList.add(conferences);
		bundle.putParcelableArrayList("conferences", arrayList);
		sendTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
				CallbackHandler.WHAT_QUERY_VIDEOCONFERENCES, bundle);
	}

	@Override
	public void onVideoConferenceInvite(CloopenReason reason,
			String conferenceId) {
	}

	@Override
	public void onVideoConferenceDismiss(CloopenReason reason,
			String conferenceId) {

		releaseCallNoneVoip();
		Bundle bundle = getCloopenReasonBundle(reason);
		bundle.putString(Device.CONFNO, conferenceId);
		sendTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
				CallbackHandler.WHAT_DISMISS_VIDEOCONFERENCE, bundle);
	}

	/**
	 *
	 * <p>
	 * Title: releaseCallNoneVoip
	 * </p>
	 * <p>
	 * Description:
	 * </p>
	 */
	private void releaseCallNoneVoip() {
		// A release of protection
		runningType = RunningType.RunningType_None;
		if (!TextUtils.isEmpty(ccpCallingID)) {
			releaseCall(ccpCallingID);
			ccpCallingID = null;
		}
	}

	@Override
	public void onVideoConferenceRemoveMember(CloopenReason reason,
			String member) {

		if (this.callControlManager != null) {
			String sid = this.callControlManager.getUserAgentConfig().getSid();
			if (TextUtils.isEmpty(sid) && sid.equals(member)) {
				releaseCall(ccpCallingID);
			}
		}

		Bundle bundle = getCloopenReasonBundle(reason);
		bundle.putString("member", member);
		sendTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
				CallbackHandler.WHAT_REMOVE_MEMBER_VIDEOCONFERENCE, bundle);
	}

	public void disConnectToCCP() {
		if (this.callControlManager != null) {
			this.callControlManager.disConnectToCCP();
		}
	}

	@Override
	public void onDownloadVideoConferencePortraits(CloopenReason reason,
			VideoPartnerPortrait portrait) {

		Bundle bundle = getCloopenReasonBundle(reason);
		bundle.putSerializable("portrait", portrait);
		sendTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
				CallbackHandler.WHAT_CCP_DOWNLOAD, bundle);
	}

	@Override
	public void getPortraitsFromVideoConference(String conferenceId) {
		if (this.callControlManager != null) {
			this.callControlManager
					.GetPortraitsFromVideoConference(conferenceId);
		}
	}

	@Override
	public void onGetPortraitsFromVideoConference(CloopenReason reason,
			List<VideoPartnerPortrait> videoPortraits) {
		Bundle bundle = getCloopenReasonBundle(reason);
		ArrayList arrayList = new ArrayList();
		arrayList.add(videoPortraits);
		bundle.putParcelableArrayList("videoPortraits", arrayList);
		sendTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
				CallbackHandler.WHAT_GET_VIDEO_CONFERENCE_PORPRRAIT, bundle);
	}

	/**
	 * @param reason
	 * @return
	 */
	Bundle getCloopenReasonBundle(CloopenReason reason) {
		Bundle bundle = new Bundle();
		bundle.putSerializable(Device.CLOOPEN_REASON, reason);
		return bundle;
	}

	/**
	 * Get local video picture data,If you do not receive local or remote video
	 * data, then return null
	 *
	 * @return VideoSnapshot if receive local or remote video data
	 *
	 * @see {@link CallControlManager#getLocalVideoSnapshot(String)}
	 * @version 3.6
	 */
	@Override
	public VideoSnapshot getLocalVideoSnapshot() {

		VideoSnapshot vSnapshot = null;
		if (runningType != RunningType.RunningType_None
				&& makeCallType == CallType.VIDEO) {
			if (this.callControlManager != null) {

				Object localVideoSnapshot = this.callControlManager
						.getLocalVideoSnapshot(ccpCallingID);

				if (localVideoSnapshot == null) {
					return vSnapshot;
				}

				if (localVideoSnapshot instanceof VideoSnapshot) {
					vSnapshot = (VideoSnapshot) localVideoSnapshot;
				}
			}

		}

		return vSnapshot;
	}

	/**
	 * Get local video picture data,If you do not receive local or remote video
	 * data, then return null
	 *
	 * @return VideoSnapshot if receive local or remote video data
	 *
	 * @see {@link CallControlManager#getRemoteVideoSnapshot(String)}
	 * @version 3.6
	 */
	@Override
	public VideoSnapshot getRemoteVideoSnapshot() {

		VideoSnapshot vSnapshot = null;
		if (runningType != RunningType.RunningType_None
				&& makeCallType == CallType.VIDEO) {
			if (this.callControlManager != null) {

				Object localVideoSnapshot = this.callControlManager
						.getRemoteVideoSnapshot(ccpCallingID);

				if (localVideoSnapshot == null) {
					return vSnapshot;
				}

				if (localVideoSnapshot instanceof VideoSnapshot) {
					vSnapshot = (VideoSnapshot) localVideoSnapshot;
				}
			}

		}

		return vSnapshot;
	}

	@Override
	public void sendLocalPortrait(String fileName, String conferenceId) {
		if (this.callControlManager != null) {
			this.callControlManager.SendLocalPortrait(fileName, conferenceId);
		}
	}

	@Override
	public void onSwitchRealScreenToVoip(CloopenReason reason) {
		Bundle bundle = getCloopenReasonBundle(reason);
		sendTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
				CallbackHandler.WHAT_SWITCH_REALSCREEN, bundle);
	}

	@Override
	public void downloadVideoConferencePortraits(
			ArrayList<VideoPartnerPortrait> portraitsList) {
		if (this.callControlManager != null) {

			this.callControlManager
					.DownloadVideoConferencePortraits(portraitsList);
		}
	}

	@Override
	public void onSendLocalPortrait(CloopenReason reason, String conferenceId) {
		Bundle bundle = getCloopenReasonBundle(reason);
		bundle.putString(Device.CONFNO, conferenceId);
		sendTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
				CallbackHandler.WHAT_SEND_LOCAL_VIDEO_PORPRTAIT, bundle);
	}

	/**
	 * @version 3.6
	 */
	@Override
	public void startVoiceCallRecording(String callid, String fileName)
			throws CCPRecordException {
		if (runningType == RunningType.RunningType_None) {
			// error
			throw new CCPRecordException("The current state of the SDK "
					+ runningType + ", there is no calls in progress.");
		}

		if (makeCallType == null || makeCallType != CallType.VOICE) {
			// error.
			throw new CCPRecordException(
					"The current call in progress not Voice , that is "
							+ makeCallType);
		}

		try {
			if (this.callControlManager != null) {
				this.callControlManager.startRecordVoice(callid, fileName);
			}

		} catch (Exception e) {
			throw new CCPRecordException("Record Voice error ,"
					+ e.getMessage());
		}

	}

	/**
	 * @version 3.6
	 */
	@Override
	public void stopVoiceCallRecording(String callid) {
		try {
			if (this.callControlManager != null) {
				this.callControlManager.stopRecordVoice(callid);
			}

		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/**
	 * @version 3.6
	 */
	@Override
	public void onCallRecord(String callid, String fileName, int reason) {

		if (getCCPListenerInfo().mOnCallRecordListener != null) {
			if (reason == 0) {
				sendTarget(CallbackHandler.WHAT_VOIP_CALL_RECORD,
						CallbackHandler.WHAT_CALL_RECORDING_OVER, fileName);
			} else {
				sendToTarget(CallbackHandler.WHAT_VOIP_CALL_RECORD,
						CallbackHandler.WHAT_CALL_RECORDING_ERROR, reason, null);
			}
		}
	}

	/**
	 * @version 3.6
	 */
	@Override
	public void setOnCallRecordListener(
			OnCallRecordListener onCallRecordListener) {
		getCCPListenerInfo().mOnCallRecordListener = onCallRecordListener;
	}

	@Override
	public void setOnTriggerSrtpListener(
			OnTriggerSrtpListener onTriggerSrtpListener) {
		getCCPListenerInfo().mOnTriggerSrtpListener = onTriggerSrtpListener;
	}

	@Override
	public void OnTriggerSrtp(String callId, boolean caller) {
		if (getCCPListenerInfo().mOnTriggerSrtpListener != null) {
			getCCPListenerInfo().mOnTriggerSrtpListener.OnTriggerSrtp(callId,
					caller);
		}
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see com.hisun.phone.core.voice.Device#setShieldMosaic(boolean)
	 */
	@Override
	public int setShieldMosaic(boolean flag) {
		if (this.callControlManager == null) {
			return -1;
		}
		return this.callControlManager.setShieldMosaic(flag);
	}

	@Override
	public State checkUserOnline(String account) {
		if (this.callControlManager == null) {
			return State.OFFLINE;
		}
		int _userOnline = this.callControlManager.checkUserOnline(account);
		State state = State.OFFLINE;
		switch (_userOnline) {
		case 0:
			state = State.ONLINE;
			break;
		case 1:
			state = State.OFFLINE;
			break;
		case 2:
			state = State.NOTEXIST;
			break;
		case 3:
			state = State.TIMEOUT;
			break;
		default:
			break;
		}
		return state;
	}

	/**
	 * <duration>%lld</duration><send>%lld</send><recv>%lld</recv>"
	 */
	@Override
	public NetworkStatistic getNetworkStatistic(String callid) {

		if (!TextUtils.isEmpty(callid) && !callid.matches("[0-9]+")) {
			// conf
			callid = ccpCallingID;
		}

		if (this.callControlManager == null || TextUtils.isEmpty(callid)) {
			return null;
		}
		String trafficStats = this.callControlManager
				.getNetworkStatistic(callid);
		Log4Util.d(Device.TAG, "callid : " + callid + " , trafficStats: "
				+ trafficStats);

		if (TextUtils.isEmpty(trafficStats)) {
			return null;
		}
		return VoiceUtil.parserTrafficStats(trafficStats);
	}

	@Override
	public byte[] onCallProcessData(byte[] b, int transDirection) {

		if (getCCPListenerInfo().mOnCallProcessDataListener != null) {
			return getCCPListenerInfo().mOnCallProcessDataListener
					.onCallProcessData(b, transDirection);
		}
		return b;
	}

	@Override
	public void setProcessDataEnabled(String callid, boolean enabled,
			OnCallProcessDataListener l) {

		if (this.callControlManager != null) {
			this.callControlManager.setProcessDataEnabled(callid, enabled);
		}

		if (enabled) {
			getCCPListenerInfo().mOnCallProcessDataListener = l;
		}
	}

	/**
	 * for debuge
	 */
	public void setNativeLog(boolean enabled) {
		this.callControlManager.setTraceFlag(enabled);
	}

	@Override
	public void setKeepAliveTimeout(int wifi, int mobile) {
		if (this.callControlManager != null) {
			this.callControlManager.setKeepAliveTimeout(wifi, mobile);
		}
	}

	@Override
	public boolean setRootCAPath(String caPath) {
		int enabled = NativeInterface.setRootCAPath(caPath);
		return checkSuccess(enabled);
	}

	public boolean checkSuccess(int value) {
		if (value == 0) {
			return true;
		}
		return false;
	}

	@Override
	public boolean setClientCertPath(String certPath) {
		int enabled = NativeInterface.setClientCertPath(certPath);
		return checkSuccess(enabled);
	}

	@Override
	public boolean setClientKeyPath(String keyPath) {
		int enabled = NativeInterface.setClientKeyPath(keyPath);
		return checkSuccess(enabled);
	}

	@Override
	public boolean setTlsSrtpEnabled(boolean tls, boolean srtp, int cryptType,
			String key) {
		NativeInterface.setTlsSrtpEnabled(tls, srtp, cryptType, key);
		return true;
	}

	@Override
	public void onTransferStateSucceed(String callid, boolean result) {
		Bundle bundle = new Bundle();
		bundle.putString("callid", callid);
		bundle.putBoolean("result", result);
		sendTarget(CallbackHandler.WHAT_VOIP_CALL,
				CallbackHandler.WHAT_TRANSFER_STATE_STATE_SUCCESS, bundle);
	}

	@Override
	public boolean registerAudioDevice() {
		if (callControlManager != null) {
			return callControlManager.registerAudioDevice() == 0;
		}
		return false;
	}

	@Override
	public boolean deregisterAudioDevice() {
		if (callControlManager != null) {
			return callControlManager.deregisterAudioDevice() == 0;
		}
		return false;
	}

	@Override
	public boolean SetNetworkGroupId(String groupId) {
		if (callControlManager != null) {
			return callControlManager.setNetworkGroupId(groupId) == 0;
		}
		return false;
	}

	@Override
	public void setProcessOriginalDataEnabled(String callid, boolean enabled,
			OnProcessOriginalAudioDataListener l) {
		if (this.callControlManager != null) {
			this.callControlManager.setProcessOriginalDataEnabled(callid,
					enabled);
		}

		if (enabled) {
			getCCPListenerInfo().mProcessOriginalAudioDataListener = l;
		}
	}

	@Override
	public byte[] onProcessOriginalData(byte[] b) {
		if (getCCPListenerInfo().mProcessOriginalAudioDataListener != null) {
			getCCPListenerInfo().mProcessOriginalAudioDataListener
					.onProcessOriginalAudioData(ccpCallingID, b,
							(b != null ? b.length : 0));
		}
		return null;
	}

	@Override
	public void setSrtpEnabled(boolean srtp, boolean userMode, int cryptType,
			String key) {
		if (this.callControlManager != null) {
			this.callControlManager.setSrtpEnabled(srtp, userMode, cryptType,
					key);
		}
	}

	@Override
	public int requestMemberVideo(String conferenceNo, String conferencePasswd,
			String remoteSipNo, View videoWindow, String ip, int port) {
		NativeInterface.SetVideoConferenceAddr(ip);

		return NativeInterface.requestMemberVideo(conferenceNo,
				conferencePasswd, remoteSipNo, videoWindow, port,
				multiViewHashMap);
	}

	@Override
	public int cancelMemberVideo(String conferenceNo, String conferencePasswd,
			String remoteSipNo) {
		Log4Util.d("stopMemberVideo remoteSipNo :=" + remoteSipNo);
		return NativeInterface.stopMemberVideo(conferenceNo, conferencePasswd,
				remoteSipNo, multiViewHashMap);
	}

	@Override
	public void publishVideoFrame(String appId, String conferenceNo,
			OnVideoMemberFrameListener l) {
		if (callControlManager != null) {
			getCCPListenerInfo().onVideoMemberFrameListener = l;
			callControlManager.handleVideoPublishOperate(appId, conferenceNo,
					true);
		}
	}

	@Override
	public void unPublishVideoFrame(String appId, String conferenceNo,
			OnVideoMemberFrameListener l) {
		if (callControlManager != null) {
			getCCPListenerInfo().onVideoMemberFrameListener = l;
			callControlManager.handleVideoPublishOperate(appId, conferenceNo,
					false);
		}
	}

	@Override
	public void resetVideoConfWindow(String sipNo, View videoWindow) {
		if (TextUtils.isEmpty(sipNo)) {
			return;
		}
		int resetVideoConfWindow = NativeInterface.resetVideoConfWindow(sipNo,
				videoWindow);
		Log4Util.i("resetVideoConfWindow " + resetVideoConfWindow);
	}

	@Override
	public void OnPublishVideoFrameRequest(int type, CloopenReason reason) {
		Bundle bundle = getCloopenReasonBundle(reason);
		bundle.putInt("type", type);
		sendTarget(CallbackHandler.WHAT_REST_VIDEOCONFERENCE,
				CallbackHandler.WHAT_VIDEO_PUBLISH, bundle);
	}

	@Override
	public void onRequestConferenceMemberVideoFailed(int reason,
			String conferenceId, String voip) {
		if (getCCPListenerInfo().mOnVideoConferenceListener != null) {
			getCCPListenerInfo().mOnVideoConferenceListener
					.onRequestConferenceMemberVideoFailed(reason, conferenceId,
							voip);
		}
	}

	@Override
	public void onCancelConferenceMemberVideo(int reason, String conferenceId,
			String voip) {
		if (getCCPListenerInfo().mOnVideoConferenceListener != null) {
			getCCPListenerInfo().mOnVideoConferenceListener
					.onCancelConferenceMemberVideo(reason, conferenceId, voip);
		}
	}

}