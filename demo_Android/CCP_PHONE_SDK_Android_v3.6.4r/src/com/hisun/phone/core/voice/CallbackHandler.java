/**
 * 
 * @ClassName: CallbackHandler.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2014-1-7
 * @version 3.6
 */
package com.hisun.phone.core.voice;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Parcelable;
import android.text.TextUtils;

import com.hisun.phone.core.voice.Device.CCPListenerInfo;
import com.hisun.phone.core.voice.Device.CallType;
import com.hisun.phone.core.voice.DeviceListener.Reason;
import com.hisun.phone.core.voice.model.CloopenReason;
import com.hisun.phone.core.voice.model.chatroom.Chatroom;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMember;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMsg;
import com.hisun.phone.core.voice.model.im.InstanceMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneMember;
import com.hisun.phone.core.voice.model.interphone.InterphoneMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConference;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMember;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoPartnerPortrait;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.SdkErrorCode;

import java.util.ArrayList;
import java.util.List;

/**
 * 
 * @ClassName: CallbackHandler.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2014-1-7
 * @version 3.6
 */
public class CallbackHandler extends Handler {

	public static final int WHAT_REST_CHATROOM 						= 1;
	public static final int WHAT_REST_INTERPHONE 					= 2;
	public static final int WHAT_REST_IM 							= 3;
	public static final int WHAT_REST_VIDEOCONFERENCE 				= 4;
	public static final int WHAT_VOIP_CALL 							= 5;
	public static final int WHAT_VOIP_CALL_RECORD 					= 6;
	public static final int WHAT_DEVICE                             = 7;
	
	
	
	public static final int WHAT_ON_CONNECT 						= 0x2000;
	public static final int WHAT_ON_DISCONNECT 						= 0x2001;
	
	public static final int VOIP_CALL_FAILED	        			= 0x2002;
	public static final int VOIP_CALL_INCOMING		        		= 0x2003;
	public static final int VOIP_CALL_ALERTING		       	 		= 0x2004;
	public static final int VOIP_CALL_ANSWERED		        		= 0x2005;
	public static final int VOIP_CALL_PAUSED			       	 	= 0x2006;
	public static final int VOIP_CALL_REMOTE_PAUSED	        		= 0x2007;
	public static final int VOIP_CALL_RELEASED		        		= 0x2008;
	public static final int VOIP_CALL_TRANSFERRED	        		= 0x2009;
	public static final int VOIP_CALL_VIDEO			        		= 0x200A;
	public static final int VOIP_CALL_PROCEEDING		        	= 0x200B;
	public static final int VOIP_CALL_VIDEO_UPDATE_REQUEST  		= 0x200C;
	public static final int VOIP_CALL_VIDEO_UPDATE_RESPONSE   		= 0x200D;
	public static final int VOIP_REMOTE_VIDEO_RATIO           		= 0x200E;
	public static final int VOIP_MEDIA_INIT_FAILED 	    			= 0x200F;
	
	public static final int VOIP_MAKECALLBACK                       = 0x2010;
	public static final int WHAT_RECEIVE_CHATROOM_MESSAGE           = 0x2011;
	public static final int WHAT_RECEIVE_INTERPHONE_MESSAGE         = 0x2012;
	public static final int WHAT_RECEIVE_INSTANCE_MESSAGE         	= 0x2013;
	public static final int WHAT_RECEIVE_VIDEO_CONGERENCE_MESSAGE   = 0x2014;
	public static final int WHAT_START_INTERPHONE 					= 0x2015;
	public static final int WHAT_CONTROL_MIC 						= 0x2016;
	public static final int WHAT_RELEASE_MIC 						= 0x2017;
	public static final int WHAT_QUERY_INTERPHONE_MEMBERS 			= 0x2018;
	public static final int WHAT_START_CHATROOM 					= 0x2019;
	public static final int WHAT_DISMISS_CHATROOM 					= 0x201A;
	public static final int WHAT_REMOVE_MEMBER_CHATROOM 			= 0x201B;
	public static final int WHAT_QUERY_CHATROOMS 					= 0x201C;
	public static final int WHAT_INVITE_JOIN_CHATROOM 				= 0x201D;
	public static final int WHAT_QUERY_CHATROOM_MEMBERS 			= 0x201E;
	public static final int WHAT_SEND_MEDIA_MSG 					= 0x201F;
	public static final int WHAT_DOWNLOAD_MEDIA_MSG 				= 0x2020;
	public static final int WHAT_CONFIRM_DOWNLOAD_MEDIAMESSAGE 		= 0x2021;
	public static final int WHAT_START_VIDEOCONFERENCE 				= 0x2022;
	public static final int WHAT_QUERY_VIDEOCONFERENCE_MEMBERS 		= 0x2023;
	public static final int WHAT_QUERY_VIDEOCONFERENCES 			= 0x2024;
	public static final int WHAT_DISMISS_VIDEOCONFERENCE 			= 0x2025;
	public static final int WHAT_REMOVE_MEMBER_VIDEOCONFERENCE 		= 0x2026;
	public static final int WHAT_SWITCH_REALSCREEN 					= 0x2027;
	public static final int WHAT_GET_VIDEO_CONFERENCE_PORPRRAIT 	= 0x2028;
	public static final int WHAT_CCP_DOWNLOAD 						= 0x2029;
	public static final int WHAT_SEND_LOCAL_VIDEO_PORPRTAIT 		= 0x202A;
	public static final int FIREWALL_POLICY_ENABLED 				= 0x202B;
	
	public static final int WHAT_CALL_RECORDING_OVER                = 0x202C;
	public static final int WHAT_CALL_RECORDING_ERROR               = 0x202D;
	public static final int WHAT_TRANSFER_STATE_STATE_SUCCESS       = 0x202E;
	public static final int WHAT_CHATROOM_MEMBER_SPRAK_OPT		 	= 0x202F;
	public static final int WHAT_VIDEO_PUBLISH = 0x2030;
	
	public static final int VIDEOCONF_REMOTE_VIDEO_RATIO           		= 0x2031;
	
	CCPListenerInfo mCCPListenerInfo;
	
	public CallbackHandler(CCPListenerInfo listener) {
		super(Looper.getMainLooper());
		mCCPListenerInfo = listener;
	}
	
	 /**
     * The Handler is used for forwarding a background thread callback to the UI thread.
     */
	@Override
	public void handleMessage(Message msg) {
		
		 if (null == getCCPListenerInfo()) {
             return;
         }
		 
		try {
			switch (msg.what) {
			case WHAT_REST_CHATROOM :
				if(getCCPListenerInfo().mOnChatroomListener == null) {
					return;
				}
				handleChatroomCallback(msg);
			case WHAT_REST_INTERPHONE :
				if(getCCPListenerInfo().mOnInterphoneListener == null) {
					return;
				}
				handleInterphoneCallback(msg);
			case WHAT_REST_IM :
				if(getCCPListenerInfo().mOnIMListener == null) {
					return;
				}
				handleInstanceMessageCallback(msg);
			case WHAT_REST_VIDEOCONFERENCE :
				if(getCCPListenerInfo().mOnVideoConferenceListener == null) {
					return;
				}
				handleVideoConferenceCallbcak(msg);
				break;
				
			case WHAT_VOIP_CALL:
				if(getCCPListenerInfo().mOnVoIPListener == null) {
					return;
				}
				handleVoIPCallback(msg);
				break;
			case WHAT_VOIP_CALL_RECORD:
				if(getCCPListenerInfo().mOnCallRecordListener == null) {
					return;
				}
				handleVoIPCallRecordingCallback(msg);
			case WHAT_DEVICE:
				if(getCCPListenerInfo().deviceListener == null) {
					return;
				}
				handleDeviceCallback(msg);
				break;
			default:
				break;
			}
		} catch (Exception e) {
		}
		 
		super.handleMessage(msg);
	}
	
	
	/**
	 * @param msg
	 */
	void handleVoIPCallRecordingCallback(Message msg) {
		switch (msg.arg1) {
		case WHAT_CALL_RECORDING_OVER:
			getCCPListenerInfo().mOnCallRecordListener.onCallRecordDone((String)msg.obj);
			break;

		case WHAT_CALL_RECORDING_ERROR:
			getCCPListenerInfo().mOnCallRecordListener.onCallRecordError(msg.arg2);
			break;
		default:
			break;
		}
	}

	/**
	 * @param msg
	 */
	void handleInstanceMessageCallback(Message msg) {
		Bundle bundle = null;
		CloopenReason cloopenReason = null;
		if(msg.obj instanceof Bundle) {
			bundle = (Bundle) msg.obj;
			
			if(bundle.containsKey(Device.yuntongxun_REASON)) {
				cloopenReason = (CloopenReason) bundle.getSerializable(Device.yuntongxun_REASON);
			}
			
		}
		switch (msg.arg1) {
			case WHAT_SEND_MEDIA_MSG :
				InstanceMsg instanceMsg = null;
				if(bundle.containsKey("InstanceMsg")) {
					instanceMsg = (InstanceMsg) bundle.getSerializable("InstanceMsg");
				}
				getCCPListenerInfo().mOnIMListener.onSendInstanceMessage(cloopenReason, instanceMsg);
				
				break;
			case WHAT_DOWNLOAD_MEDIA_MSG :
				String fileName = null;
				if(bundle.containsKey("fileName")) {
					fileName = bundle.getString("fileName");
				}
				getCCPListenerInfo().mOnIMListener.onDownloadAttached(cloopenReason , fileName);
				
				break;
			case WHAT_CONFIRM_DOWNLOAD_MEDIAMESSAGE :
				getCCPListenerInfo().mOnIMListener.onConfirmIntanceMessage(cloopenReason);
				
				break;
			case WHAT_RECEIVE_INSTANCE_MESSAGE:
				InstanceMsg receiveInstanceMsg = null;
				if(bundle.containsKey("InstanceMsg")) {
					receiveInstanceMsg = (InstanceMsg) bundle.getSerializable("InstanceMsg");
				}
				getCCPListenerInfo().mOnIMListener.onReceiveInstanceMessage(receiveInstanceMsg);
				
				break;
        default:
            break;
        }
	}

	/**
	 * @param msg
	 */
	void handleInterphoneCallback(Message msg) {
		Bundle bundle = null;
		CloopenReason cloopenReason = null;
		if(msg.obj instanceof Bundle) {
			bundle = (Bundle) msg.obj;
			
			if(bundle.containsKey(Device.yuntongxun_REASON)) {
				cloopenReason = (CloopenReason) bundle.getSerializable(Device.yuntongxun_REASON);
			}
			
		}
		String confNo = null;
		switch (msg.arg1) {
			case WHAT_START_INTERPHONE :
				if(bundle != null && bundle.containsKey(Device.CONFNO)) {
					confNo = bundle.getString(Device.CONFNO);
				}
				getCCPListenerInfo().mOnInterphoneListener.onInterphoneState(cloopenReason, confNo);
				
				break;
			case WHAT_CONTROL_MIC :
				String speaker = null;
				if(bundle != null && bundle.containsKey("speaker")) {
					speaker = bundle.getString("speaker");
				}
				getCCPListenerInfo().mOnInterphoneListener.onControlMicState(cloopenReason, speaker);
				
				break;
			case WHAT_RELEASE_MIC :
				getCCPListenerInfo().mOnInterphoneListener.onReleaseMicState(cloopenReason);
				
				break;
			case WHAT_QUERY_INTERPHONE_MEMBERS :
				List<InterphoneMember> members = null;
				if(bundle.containsKey("member")) {
					ArrayList<Parcelable> parcelableArrayList = bundle.getParcelableArrayList("member");
					members = (List<InterphoneMember>) parcelableArrayList.get(0);
				}
				getCCPListenerInfo().mOnInterphoneListener.onInterphoneMembers(cloopenReason, members);
				break;
				
			case WHAT_RECEIVE_INTERPHONE_MESSAGE :
				InterphoneMsg interphoneMsg = null;
				if(bundle.containsKey("InterphoneMsg")) {
					interphoneMsg = (InterphoneMsg) bundle.getSerializable("InterphoneMsg");
				}
				getCCPListenerInfo().mOnInterphoneListener.onReceiveInterphoneMsg(interphoneMsg);
				break;
        default:
            break;
        }
	}

	/**
	 * @param msg
	 */
	void handleChatroomCallback(Message msg) {
		Bundle bundle = null;
		CloopenReason cloopenReason = null;
		if(msg.obj instanceof Bundle) {
			bundle = (Bundle) msg.obj;
			
			if(bundle.containsKey(Device.yuntongxun_REASON)) {
				cloopenReason = (CloopenReason) bundle.getSerializable(Device.yuntongxun_REASON);
			}
			
		}
		String confNo = null;
		String member = null;
		switch (msg.arg1) {
			case WHAT_START_CHATROOM :
				if(bundle != null && bundle.containsKey(Device.CONFNO)) {
					confNo = bundle.getString(Device.CONFNO);
				}
				getCCPListenerInfo().mOnChatroomListener.onChatroomState(cloopenReason, confNo);
				break;
			case WHAT_DISMISS_CHATROOM :
				if(bundle != null && bundle.containsKey(Device.CONFNO)) {
					confNo = bundle.getString(Device.CONFNO);
				}
				getCCPListenerInfo().mOnChatroomListener.onChatroomDismiss(cloopenReason, confNo);
				break;
				
			case WHAT_REMOVE_MEMBER_CHATROOM :
				if(bundle != null && bundle.containsKey("member")) {
					member = bundle.getString("member");
				}
				getCCPListenerInfo().mOnChatroomListener.onChatroomRemoveMember(cloopenReason, member);
				break;
			case WHAT_CHATROOM_MEMBER_SPRAK_OPT:
				if(bundle != null && bundle.containsKey("member")) {
					member = bundle.getString("member");
				}
				getCCPListenerInfo().mOnChatroomListener.onSetMemberSpeakOpt(cloopenReason, member);
				break;
			case WHAT_QUERY_CHATROOMS :
				List<Chatroom> chatrooms = null;
				if(bundle.containsKey("chatroomList")) {
					ArrayList<Parcelable> parcelableArrayList = bundle.getParcelableArrayList("chatroomList");
					chatrooms = (List<Chatroom>) parcelableArrayList.get(0);
				}
				getCCPListenerInfo().mOnChatroomListener.onChatrooms(cloopenReason, chatrooms);
				
				break;
			case WHAT_INVITE_JOIN_CHATROOM :
				if(bundle != null && bundle.containsKey(Device.CONFNO)) {
					confNo = bundle.getString(Device.CONFNO);
				}
				getCCPListenerInfo().mOnChatroomListener.onChatroomInviteMembers(cloopenReason, confNo);
				
				break;
			case WHAT_QUERY_CHATROOM_MEMBERS :
				List<ChatroomMember> chatroomMembers = null;
				if(bundle.containsKey("chatroomMembers")) {
					ArrayList<Parcelable> parcelableArrayList = bundle.getParcelableArrayList("chatroomMembers");
					chatroomMembers = (List<ChatroomMember>) parcelableArrayList.get(0);
				}
				getCCPListenerInfo().mOnChatroomListener.onChatroomMembers(cloopenReason, chatroomMembers);
				
				break;
			case WHAT_RECEIVE_CHATROOM_MESSAGE:
				ChatroomMsg chatroomMsg = null;
				if(bundle.containsKey("ChatroomMsg")) {
					chatroomMsg = (ChatroomMsg) bundle.getSerializable("ChatroomMsg");
				}
				getCCPListenerInfo().mOnChatroomListener.onReceiveChatroomMsg(chatroomMsg);
				break;
        default:
            break;
        }
	}

	/**
	 * @param msg
	 */
	private void handleDeviceCallback(Message msg) {
		switch (msg.arg1) {
		case WHAT_ON_CONNECT:
			getCCPListenerInfo().deviceListener.onConnected();
			break;
		case WHAT_ON_DISCONNECT:
			getCCPListenerInfo().deviceListener.onDisconnect((Reason)msg.obj);
			break;
		case FIREWALL_POLICY_ENABLED:
			
			getCCPListenerInfo().deviceListener.onFirewallPolicyEnabled();
			break;
		default:
			break;
		}
	}

	@SuppressWarnings("unchecked")
	void handleVideoConferenceCallbcak(Message msg) {
		
		Bundle bundle = null;
		CloopenReason cloopenReason = null;
		if(msg.obj instanceof Bundle) {
			bundle = (Bundle) msg.obj;
			
			if(bundle.containsKey(Device.yuntongxun_REASON)) {
				cloopenReason = (CloopenReason) bundle.getSerializable(Device.yuntongxun_REASON);
			}
			
		}
		String confNo = null;
		String member = null;
		switch (msg.arg1) {
			case WHAT_START_VIDEOCONFERENCE :
				if(bundle != null && bundle.containsKey(Device.CONFNO)) {
					confNo = bundle.getString(Device.CONFNO);
				}
				getCCPListenerInfo().mOnVideoConferenceListener.onVideoConferenceState(cloopenReason, confNo);
				break;
			case WHAT_QUERY_VIDEOCONFERENCE_MEMBERS :
				List<VideoConferenceMember> videoMembers = null;
				if(bundle.containsKey("videoMembers")) {
					ArrayList<Parcelable> parcelableArrayList = bundle.getParcelableArrayList("videoMembers");
					videoMembers = (List<VideoConferenceMember>) parcelableArrayList.get(0);
				}
				getCCPListenerInfo().mOnVideoConferenceListener.onVideoConferenceMembers(cloopenReason, videoMembers);
				
				break;
			case WHAT_QUERY_VIDEOCONFERENCES :
				List<VideoConference> conferences = null;
				if(bundle.containsKey("conferences")) {
					ArrayList<Parcelable> parcelableArrayList = bundle.getParcelableArrayList("conferences");
					conferences = (List<VideoConference>) parcelableArrayList.get(0);
				}
				getCCPListenerInfo().mOnVideoConferenceListener.onVideoConferences(cloopenReason, conferences);
				
				break;
			case WHAT_DISMISS_VIDEOCONFERENCE :
				if(bundle != null && bundle.containsKey(Device.CONFNO)) {
					confNo = bundle.getString(Device.CONFNO);
				}
				getCCPListenerInfo().mOnVideoConferenceListener.onVideoConferenceDismiss(cloopenReason, confNo);
				
				break;
			case WHAT_VIDEO_PUBLISH:
				if(bundle != null && bundle.containsKey("type")) {
					int type =  bundle.getInt("type");
					if(type == 0) {
						getCCPListenerInfo().onVideoMemberFrameListener.onPublisVideoFrameRequest(cloopenReason);
					} else {
						getCCPListenerInfo().onVideoMemberFrameListener.onStopVideoFrameRequest(cloopenReason);
					}
				}
				break;
			case WHAT_REMOVE_MEMBER_VIDEOCONFERENCE :
				if(bundle != null && bundle.containsKey("member")) {
					member = bundle.getString("member");
				}
				getCCPListenerInfo().mOnVideoConferenceListener.onVideoConferenceRemoveMember(cloopenReason, member);
				
				break;
			case WHAT_SWITCH_REALSCREEN :
				getCCPListenerInfo().mOnVideoConferenceListener.onSwitchRealScreenToVoip(cloopenReason);
				
				break;
			case WHAT_GET_VIDEO_CONFERENCE_PORPRRAIT :
				List<VideoPartnerPortrait> videoPortraits = null;
				if(bundle.containsKey("videoPortraits")) {
					ArrayList<Parcelable> parcelableArrayList = bundle.getParcelableArrayList("videoPortraits");
					videoPortraits = (List<VideoPartnerPortrait>) parcelableArrayList.get(0);
				}
				getCCPListenerInfo().mOnVideoConferenceListener.onGetPortraitsFromVideoConference(cloopenReason, videoPortraits);
				break;
				// version 3.5
			case WHAT_CCP_DOWNLOAD :
				VideoPartnerPortrait portrait = null;
				if(bundle.containsKey("portrait")) {
					portrait = (VideoPartnerPortrait) bundle.getSerializable("portrait");
				}
				getCCPListenerInfo().mOnVideoConferenceListener.onDownloadVideoConferencePortraits(cloopenReason ,portrait);
				break;
			case WHAT_SEND_LOCAL_VIDEO_PORPRTAIT :
				if(bundle != null && bundle.containsKey(Device.CONFNO)) {
					confNo = bundle.getString(Device.CONFNO);
				}
				getCCPListenerInfo().mOnVideoConferenceListener.onSendLocalPortrait(cloopenReason, confNo);
				break;
			case WHAT_RECEIVE_VIDEO_CONGERENCE_MESSAGE :
				VideoConferenceMsg videoConferenceMsg = null;
				if(bundle.containsKey("VideoConferenceMsg")) {
					videoConferenceMsg = (VideoConferenceMsg) bundle.getSerializable("VideoConferenceMsg");
				}
				getCCPListenerInfo().mOnVideoConferenceListener.onReceiveVideoConferenceMsg(videoConferenceMsg);
				break;
			case VIDEOCONF_REMOTE_VIDEO_RATIO:
				try {
	        		String paramers = (String) msg.obj;
	        		if(!TextUtils.isEmpty(paramers)) {
	        			String[] split = paramers.split(",");
	        			if(split != null) {
	        				Log4Util.e("VOIP_REMOTE_VIDEO_RATIO == " + split[0] + " " + split[1] );
	        				String[] resoluSplit = split[1].split("x");
	        				getCCPListenerInfo().mOnVideoConferenceListener.onVideoRatioChanged(split[0], 
	        						Integer.valueOf(resoluSplit[0]), Integer.valueOf(resoluSplit[1]));
	        			}
	        		}
					} catch (Exception e) {}
				break;
        default:
            break;
        }
	}
	
	/**
	 * @param msg
	 */
	void handleVoIPCallback(Message msg) {
		Bundle bundle = null;
		String callid = null;
		if(msg.obj instanceof Bundle) {
			bundle = (Bundle) msg.obj;
			
			if(bundle.containsKey("callid")) {
				callid = bundle.getString("callid");
			}
			
		} else if (msg.obj instanceof String) {
			callid = (String)msg.obj;
		}
		switch (msg.arg1) {
		case VOIP_CALL_PROCEEDING:
			getCCPListenerInfo().mOnVoIPListener.onCallProceeding(callid);
			break;
		case VOIP_CALL_ALERTING:
			getCCPListenerInfo().mOnVoIPListener.onCallAlerting(callid);
			break;
		case VOIP_CALL_ANSWERED:
			getCCPListenerInfo().mOnVoIPListener.onCallAnswered(callid);
			break;
		case VOIP_CALL_PAUSED:
			getCCPListenerInfo().mOnVoIPListener.onCallPaused(callid);
			break;
		case VOIP_CALL_REMOTE_PAUSED:
			getCCPListenerInfo().mOnVoIPListener.onCallPausedByRemote(callid);
			break;
		case VOIP_CALL_RELEASED:
			getCCPListenerInfo().mOnVoIPListener.onCallReleased(callid);
			break;
			
		case VOIP_CALL_FAILED:
			Reason reason = null;
			if(bundle != null) {
				if(bundle.containsKey(Device.REASON)) {
					reason = (Reason) bundle.getSerializable(Device.REASON);
				}
			}
			getCCPListenerInfo().mOnVoIPListener.onMakeCallFailed(callid, reason);
			break;
		case VOIP_CALL_TRANSFERRED:
			try {
        		String paramers = (String) msg.obj;
        		if(!TextUtils.isEmpty(paramers)) {
        			String[] split = paramers.split(",");
        			if(split != null) {
        				getCCPListenerInfo().mOnVoIPListener.onCallTransfered(split[0], split[1]);
        			}
        		}
				} catch (Exception e) {}
			break;
		case VOIP_CALL_VIDEO_UPDATE_REQUEST:
			getCCPListenerInfo().mOnVoIPListener.onCallMediaUpdateRequest(callid, msg.arg2);
			getCCPListenerInfo().mOnVoIPListener.onSwitchCallMediaTypeRequest(callid, msg.arg2 == 0 ? CallType.VIDEO :CallType.VOICE);
			break;
		case VOIP_CALL_VIDEO_UPDATE_RESPONSE:
			getCCPListenerInfo().mOnVoIPListener.onCallMediaUpdateResponse((String)msg.obj, msg.arg2);
			getCCPListenerInfo().mOnVoIPListener.onSwitchCallMediaTypeResponse(callid, msg.arg2 == 0 ? CallType.VIDEO :CallType.VOICE);
			break;
		case VOIP_REMOTE_VIDEO_RATIO:
			try {
        		String paramers = (String) msg.obj;
        		if(!TextUtils.isEmpty(paramers)) {
        			String[] split = paramers.split(",");
        			if(split != null) {
        				Log4Util.e("VOIP_REMOTE_VIDEO_RATIO == " + split[0] + " " + split[1] );
        				String[] resoluSplit = split[1].split("x");
        				getCCPListenerInfo().mOnVoIPListener.onCallVideoRatioChanged(split[0], Integer.valueOf(resoluSplit[0]), Integer.valueOf(resoluSplit[1]));
        			}
        		}
				} catch (Exception e) {}
			break;
			
		case VOIP_MEDIA_INIT_FAILED:
			getCCPListenerInfo().mOnVoIPListener.onCallMediaInitFailed((String)msg.obj, msg.arg2 == 0 ? CallType.VOICE :CallType.VIDEO);
			break;
			
		case VOIP_MAKECALLBACK :
        	try {
        		String paramers = (String) msg.obj;
        		if(!TextUtils.isEmpty(paramers)) {
        			String[] split = paramers.split(",");
        			if(split != null) {
        				getCCPListenerInfo().mOnVoIPListener.onCallback(msg.arg2, split[0], split[1]);
        			}
        		}
				} catch (Exception e) {}
        	break;
		case WHAT_TRANSFER_STATE_STATE_SUCCESS:
			boolean result = false;
			if(bundle.containsKey("result")) {
				result = bundle.getBoolean("result");
			}
			getCCPListenerInfo().mOnVoIPListener.onTransferStateSucceed(callid, result);
		default:
			break;
		}
	}

	public void setCCPListenerInfo(CCPListenerInfo listener) {
		this.mCCPListenerInfo = listener;
	}
	
	CCPListenerInfo getCCPListenerInfo() {
		return this.mCCPListenerInfo;
	}
	
	
	/**
	 * @param reason
	 * @return
	 */
	public static CloopenReason getCloopenReason(int reason) {
		if (reason == 0) {
			return getCloopenReason(reason, "Request 200 /OK");
			
		} else if (reason == SdkErrorCode.SDK_HTTP_ERROR) {
			return getCloopenReason(reason, "网络请求错误, 请检查网络设置或稍后重试");
			
		} else if (reason == SdkErrorCode.SDK_XML_ERROR) {
			return getCloopenReason(reason, "请求包体格式错误");
			
		} else if (reason == SdkErrorCode.SDK_XMLBODY_ERROR) {
			return getCloopenReason(reason, "对不起, 请求包体解析错误");

		} else if(reason == SdkErrorCode.SDK_CALL_BUSY) {
			return new CloopenReason(SdkErrorCode.SDK_CALL_BUSY , "客户端当前未处于空闲状态");
			
		} else if (reason == SdkErrorCode.SDK_NOT_REGISTED) {
			return new CloopenReason(SdkErrorCode.SDK_NOT_REGISTED , "客户端未注册");
			
		} else if (reason == SdkErrorCode.SDK_TEXT_LENGTH_LIMIT) {
			return new CloopenReason(SdkErrorCode.SDK_TEXT_LENGTH_LIMIT , "IM文本长度超过限制, 最大长度2000字节");
		} else {
			return getCloopenReason(reason, "无法连接服务器，请稍后重试");
		}
	}
	
	public static CloopenReason getCloopenReason(int reason , String message) {
		if(TextUtils.isEmpty(message)) {
			return getCloopenReason(reason);
		}
		return new CloopenReason(reason, message);
	}
	
	
}
