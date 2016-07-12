/**
 * 
 */
package com.hisun.phone.core.voice;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.text.TextUtils;
import android.webkit.URLUtil;

import com.CCP.phone.CCPCallEvent;
import com.CCP.phone.NativeInterface;
import com.hisun.phone.core.voice.DeviceListener.APN;
import com.hisun.phone.core.voice.exception.CCPException;
import com.hisun.phone.core.voice.exception.CCPHttpException;
import com.hisun.phone.core.voice.exception.CCPXmlParserException;
import com.hisun.phone.core.voice.model.CCPParameters;
import com.hisun.phone.core.voice.model.DownloadInfo;
import com.hisun.phone.core.voice.model.Response;
import com.hisun.phone.core.voice.model.UploadImessage;
import com.hisun.phone.core.voice.model.chatroom.ChatRoomList;
import com.hisun.phone.core.voice.model.chatroom.ChatRoomMemberList;
import com.hisun.phone.core.voice.model.chatroom.Chatroom;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMember;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMsg;
import com.hisun.phone.core.voice.model.im.IMAttachedMsg;
import com.hisun.phone.core.voice.model.im.IMTextMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneControlMic;
import com.hisun.phone.core.voice.model.interphone.InterphoneMember;
import com.hisun.phone.core.voice.model.interphone.InterphoneMemberList;
import com.hisun.phone.core.voice.model.interphone.InterphoneMsg;
import com.hisun.phone.core.voice.model.setup.SoftSwitch;
import com.hisun.phone.core.voice.model.setup.SoftSwitch.Clpss;
import com.hisun.phone.core.voice.model.setup.UserAgentConfig;
import com.hisun.phone.core.voice.model.videoconference.VideoConference;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceList;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMember;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMemberList;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoPartnerPortrait;
import com.hisun.phone.core.voice.model.videoconference.VideoPartnerPortraitList;
import com.hisun.phone.core.voice.net.ApiParser;
import com.hisun.phone.core.voice.net.HttpClientUtil;
import com.hisun.phone.core.voice.net.HttpManager;
import com.hisun.phone.core.voice.opts.ConferenceOptions;
import com.hisun.phone.core.voice.token.Base64;
import com.hisun.phone.core.voice.util.CheckApnTypeUtils;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.SdkErrorCode;
import com.hisun.phone.core.voice.util.UserAgentUtils;
import com.hisun.phone.core.voice.util.VoiceUtil;
import com.hisun.phone.core.voice.util.VoiceUtil.SIMProvider;

/**
 * This class just handle REST Server Request.
 * 
 */
@SuppressLint("HandlerLeak")
public abstract class AbstractDispatcher {

	private ServiceHandler mServiceHandler;
	private Looper mServiceLooper;
	private UserAgentConfig userAgentConfig;
	private CCPCallEvent listener;
	
	/**
	 * 
	 */
	private String registBody = null;
	private String softSwitchDefaultAddress = null;
	private int softSwitchDefaultPort = 0;
	private String control = null;
	
	private Context context;
	private String protocol = "https";
	
	public AbstractDispatcher(Context context) {
		this.context = context;
		checkSimState();
		initServiceHandler();
	}
	
	public void checkSimState() {
		if(CheckApnTypeUtils.checkNetworkAPNType(context) == APN.WIFI) {
			protocol = "https";
			return;
		}
		SIMProvider provider = VoiceUtil.getSIMProvider(this.context);
		if (provider == SIMProvider.TELECOM) {
			protocol = "http";
		} else {
			
			// determine whether it is China Mobile and China Unicom wap
			if(CheckApnTypeUtils._isChinaMobileUnicomWap(this.context)) {
				protocol = "http";
			} else {
				protocol = "https";
			}
		}
	}

	private void initServiceHandler() {
		// Start up the thread running the service. Note that we create a
		// separate thread because the service normally runs in the process's
		// main thread, which we don't want to block. We also make it
		// background priority so CPU-intensive work will not disrupt our UI.
		HandlerThread thread = new HandlerThread("CCPServiceArguments",
				Process.THREAD_PRIORITY_BACKGROUND);
		thread.start();

		// Get the HandlerThread's Looper and use it for our Handler
		mServiceLooper = thread.getLooper();
		mServiceHandler = new ServiceHandler(mServiceLooper);
	}

	public final Message getHandleMessage() {
		// For each start request, send a message to start a job and deliver the
		// start ID so we know which request we're stopping when we finish the
		// job
		Message msg = mServiceHandler.obtainMessage();
		return msg;
	}

	public final void sendHandleMessage(Message msg) {
		mServiceHandler.sendMessage(msg);
	}
	
	/**
	 * Enqueue a message at the front of the message queue, to be processed on
     * the next iteration of the message loop.
	 * @param msg
	 * 
	 * @version 3.5
	 */
	public final void sendHandleMessageAtFront(Message msg) {
		mServiceHandler.sendMessageAtFrontOfQueue(msg);
	}
	
	/**
     * Remove any pending posts of messages with code 'what' that are in the
     * message queue.
     * 
     *  @version 3.5
     */
    public final void removeHandleMessages(int what) {
    	mServiceHandler.removeMessages(what);
    } 

	public static final int WHAT_MAKECALLBACK = 0x1;
	public static final int WHAT_START_INTERPHONE = 0x7;
	public static final int WHAT_CONTROL_MIC = 0x8;
	public static final int WHAT_RELEASE_MIC = 0x9;
	public static final int WHAT_QUERY_INTERPHONE_MEMBERS = 0xA;
	public static final int WHAT_START_CHATROOM = 0xB;
	public static final int WHAT_QUERY_CHATROOMS = 0xC;
	public static final int WHAT_INVITE_JOIN_CHATROOM = 0xD;
	public static final int WHAT_QUERY_CHATROOM_MEMBERS = 0xE;
	public static final int WHAT_SEND_MEDIA_MSG = 0xF;
	public static final int WHAT_NOTIFY_MEDIA_MSG_STATUES = 0x10;
	public static final int WHAT_DOWNLOAD_MEDIA_MSG = 0x11;
	public static final int WHAT_UPLOAD_MEDIA_CHUNKED = 0x12;
	public static final int WHAT_CONFIRM_DOWNLOAD_MEDIAMESSAGE = 0x13;

	//新加群聊天室接口
	public static final int WHAT_DISMISS_CHATROOM = 0x14;
	public static final int WHAT_REMOVE_MEMBER_CHATROOM = 0x15;
	
	//视频会议接口
	public static final int WHAT_START_VIDEOCONFERENCE = 0x16;
	public static final int WHAT_QUERY_VIDEOCONFERENCES = 0x17;
	public static final int WHAT_QUERY_VIDEOCONFERENCE_MEMBERS = 0x18;
	public static final int WHAT_REMOVE_MEMBER_VIDEOCONFERENCE = 0x19;
	public static final int WHAT_DISMISS_VIDEOCONFERENCE = 0x1A;
	public static final int WHAT_SWITCH_REALSCREEN = 0x1B;
	
	public static final int WHAT_CCP_DOWNLOAD = 0x1C;
	public static final int WHAT_GET_VIDEO_CONFERENCE_PORPRRAIT = 0x1D;
	
	public static final int WHAT_SEND_LOCAL_VIDEO_PORPRTAIT = 0x1E;
	public static final int WHAT_SET_CHATROOM_SPEAK_OPT = 0x1F;
	public static final int WHAT_START_VIDEOCONFERENCE_MULTI = 0x20;
	public static final int WHAT_PUBLISH_VIDEO = 0x21;
	
	
	private final class ServiceHandler extends Handler {
		public ServiceHandler(Looper looper) {
			super(looper);
		}

		@SuppressWarnings("unchecked")
		@Override
		public void handleMessage(Message msg) {
			try {
				Thread.sleep(40L);
			} catch (InterruptedException e) {
			}

			Log4Util.i(Device.TAG, Thread.currentThread().getName());
			// Normally we would do some work here.
			Bundle b = (Bundle) msg.obj;
			int what = msg.arg1;
			Log4Util.i(Device.TAG, "What: " + what);
			
			if (what == WHAT_MAKECALLBACK) {
				String selfPhoneNumber = (String) b.get(Constant.CCP_SELF_PHONENUMBER);
				String destPhoneNumber = (String) b.get(Constant.CCP_DEST_PHONENUMBER);
				String srcSerNum = (String) b.get(Constant.CCP_SELF_SERNUM);
				String destSerNum = (String) b.get(Constant.CCP_DEST_SERNUM);
				doMakeCallBack(selfPhoneNumber, destPhoneNumber , srcSerNum , destSerNum);
			} else if (what == WHAT_START_INTERPHONE) {

				String[] members = b.getStringArray(Constant.CCP_ARRAY_MEMBERS);
				String type = b.getString(Constant.CCP_TYPE);
				String appid = b.getString(Constant.APP_ID);
				doStartInterphone(members, type, appid);
			} else if (what == WHAT_CONTROL_MIC) {

				String confNo = b.getString(Constant.INTERPHONE_ID);
				doControlMIC(confNo);
			} else if (what == WHAT_RELEASE_MIC) {

				String confNo = b.getString(Constant.INTERPHONE_ID);
				doReleaseMIC(confNo);
			} else if (what == WHAT_QUERY_INTERPHONE_MEMBERS) {

				String confNo = b.getString(Constant.INTERPHONE_ID);
				doQueryMembersWithInterphone(confNo);
			} else if (what == WHAT_START_CHATROOM) {
				
				String appid = b.getString(Constant.APP_ID);
				String roomName = b.getString(Constant.CCP_ROOM_NAME);
				int square = b.getInt(Constant.CCP_SQUARE);
				String keywords = b.getString(Constant.CCP_KEYWORDS);
				String pwd = b.getString(Constant.CCP_PWD);
				int isAutoClose = b.getInt(Constant.CCP_FLAGE);
				int isAutoDelete = b.getInt(Constant.CCP_FLAGE_AUTODELETE);
				int voiceMod = b.getInt(Constant.CCP_VOICEMOD);
				doStartChatroom(appid, roomName, square, keywords, pwd, isAutoClose, voiceMod, isAutoDelete);
			}else if(what == WHAT_DISMISS_CHATROOM){
				
				String appid = b.getString(Constant.APP_ID);
				String roomNo = b.getString(Constant.CHATROOM_ID);
				doDismissChatRomm(appid, roomNo);
				
			}else if(what == WHAT_REMOVE_MEMBER_CHATROOM){
				
				String appid = b.getString(Constant.APP_ID);
				String roomNo = b.getString(Constant.CHATROOM_ID);
				String member = b.getString(Constant.CCP_MEMBER);
				doRemoveMemberFromChatRoom(appid, roomNo, member);
				
			} else if (what == WHAT_SET_CHATROOM_SPEAK_OPT) {
				String appid = b.getString(Constant.APP_ID);
				String roomNo = b.getString(Constant.CHATROOM_ID);
				String member = b.getString(Constant.CCP_MEMBER);
				int speakOpt = b.getInt(Constant.SPEAK_OPT);
				doSetChatroomSpeakType(appid, roomNo, member, speakOpt);
			} else if (what == WHAT_QUERY_CHATROOMS) {

				String appId = b.getString(Constant.APP_ID);
				String keywords = b.getString(Constant.CCP_KEYWORDS);
				doQueryChatRooms(appId, keywords);
			} else if (what == WHAT_INVITE_JOIN_CHATROOM) {

				String[] members = b.getStringArray(Constant.CCP_ARRAY_MEMBERS);
				String roomNo = b.getString(Constant.CHATROOM_ID);
				String appId = b.getString(Constant.APP_ID);
				doInviteMembersJoinChatroom(members, roomNo, appId);
			} else if (what == WHAT_QUERY_CHATROOM_MEMBERS) {

				String chatRoomId = b.getString(Constant.CHATROOM_ID);
				doQueryMembersWithChatroom(chatRoomId);
			} else if (what == WHAT_SEND_MEDIA_MSG) {
				String fileName = b.getString(Constant.CCP_FILENAME);
				String receiver = b.getString(Constant.CCP_RECEIVER);
				String uniqueID = b.getString(Constant.CCP_UNIQUEID);
				String userData = b.getString(Constant.CCP_USERDATA);
				Log4Util.w(Device.TAG, "doSendMediaMsg WHAT_SEND_MEDIA_MSG\r\n");
				doSendMediaMsg(uniqueID, fileName, receiver , userData);
				
			} else if (what == WHAT_DOWNLOAD_MEDIA_MSG) {

				ArrayList<DownloadInfo> dLoadList = (ArrayList<DownloadInfo>) b
						.getSerializable(Constant.CCP_DOWNLOAD_PARAMETERS);
				doDownloadAttachmentFiles(dLoadList);
			} else if (what == WHAT_CONFIRM_DOWNLOAD_MEDIAMESSAGE) {
				String[] msgIds = b.getStringArray(Constant.CCP_MSGIDS);
				doConfirmDownloadMediaMessage(msgIds);
				
				
			} else if (what == WHAT_START_VIDEOCONFERENCE){
				
				String appid = b.getString(Constant.APP_ID);
				String name = b.getString(Constant.CCP_ROOM_NAME);
				int square = b.getInt(Constant.CCP_SQUARE);
				String keywords = b.getString(Constant.CCP_KEYWORDS);
				String pwd = b.getString(Constant.CCP_PWD);
				int isAutoClose = b.getInt(Constant.CCP_FLAGE);
				int isAutoDelete = b.getInt(Constant.CCP_FLAGE_AUTODELETE);
				int voiceMod = b.getInt(Constant.CCP_VOICEMOD);
				doStartVideoConference(appid, name, square, keywords, pwd , isAutoClose , voiceMod , isAutoDelete);
			} else if (what == WHAT_START_VIDEOCONFERENCE_MULTI) {
				String appid = b.getString(Constant.APP_ID);
				String name = b.getString(Constant.CCP_ROOM_NAME);
				String pwd = b.getString(Constant.CCP_PWD);
				ConferenceOptions options = (ConferenceOptions) b.getSerializable(Constant.CCP_CONF_OPTIONS);
				doStartVideoConference(appid, name, pwd, options);
			} else if (what == WHAT_QUERY_VIDEOCONFERENCE_MEMBERS){
				String conferenceId = b.getString(Constant.CONFERENCE_ID);
				doQueryMembersInVideoConference(conferenceId);
			} else if (what == WHAT_QUERY_VIDEOCONFERENCES){
				String appid = b.getString(Constant.APP_ID);
				String keywords = b.getString(Constant.CCP_KEYWORDS);
				doQueryVideoConferences(appid, keywords);
			} else if (what == WHAT_DISMISS_VIDEOCONFERENCE){
				String appId = b.getString(Constant.APP_ID);
				String conferenceId = b.getString(Constant.CONFERENCE_ID);
				doDismissVideoConference(appId, conferenceId);
			} else if (what == WHAT_REMOVE_MEMBER_VIDEOCONFERENCE){
				
				String appId = b.getString(Constant.APP_ID);
				String conferenceId = b.getString(Constant.CONFERENCE_ID);
				String member = b.getString(Constant.CCP_MEMBER);
				doRemoveMemberFromVideoConference(appId, conferenceId, member);
			} else if (what == WHAT_SWITCH_REALSCREEN){
				
				String appId = b.getString(Constant.APP_ID);
				String conferenceId = b.getString(Constant.CONFERENCE_ID);
				String voip = b.getString(Constant.CCP_VOIP);
				doswitchRealScreenToVoip(appId, conferenceId, voip);
				
			} else if (what == WHAT_GET_VIDEO_CONFERENCE_PORPRRAIT) {
				String conferenceId = b.getString(Constant.CONFERENCE_ID);
				doPortraitsFromVideoConference(conferenceId);
				// version 3.5
			} else if (what == WHAT_CCP_DOWNLOAD) {

				ArrayList<VideoPartnerPortrait> portraitsList = (ArrayList<VideoPartnerPortrait>) b
						.getSerializable(Constant.CCP_DOWNLOAD_PARAMETERS_PORTRAIT);
				doDownloadVideoConferencePortraits(portraitsList);
				
			} else if (what == WHAT_SEND_LOCAL_VIDEO_PORPRTAIT) {
				String fileName = b.getString(Constant.CCP_FILENAME);
				String conferenceId = b.getString(Constant.CONFERENCE_ID);
				doSendLocalPortrait(fileName, conferenceId);
			} else if (what == WHAT_PUBLISH_VIDEO) {
				String appId = b.getString(Constant.APP_ID);
				String conferenceId = b.getString(Constant.CONFERENCE_ID);
				boolean publish = b.getBoolean(Constant.VIDEO_PUBLISH);
				if(publish) {
					doPublishVideoRequest(appId, conferenceId);
				} else {
					doUnPublishVideoRequest(appId, conferenceId);
				}
			}
			super.handleMessage(msg);
		}
	}

	private StringBuffer getStringBuffer() {
		StringBuffer sb = new StringBuffer(this.userAgentConfig.getRequestUrl(protocol));
		sb.append("/" + Build.REST_VERSION);
		return sb;
	}

	private void register(final String xmlBody ,final String accountSid, final String password) {
		if (TextUtils.isEmpty(softSwitchDefaultAddress) || softSwitchDefaultPort == 0) {
			Log4Util.e(Device.TAG, "soft address errors, can't register.");
			return;
		}

		if (accountSid == null || password == null) {
			Log4Util.e(Device.TAG, "voip account errors, can't register.");
			return;
		}

		if(TextUtils.isEmpty(xmlBody)) {
			if (NativeInterface.connectToCCP(softSwitchDefaultAddress, softSwitchDefaultPort, accountSid, password , getControl()) != 0) {
				throw new IllegalStateException("connectToCCP failed.");
			}
		} else {
			//if(NativeInterface.setPrivateCloud("11", "118.194.243.246:8020", true) == 0) {
				
				if(NativeInterface.connectToCCPWithXML(xmlBody ,accountSid, password , getControl()) != 0) {
					throw new IllegalStateException("connectToCCP failed.");
				}
			//}
		}
		
		Log4Util.i(Device.TAG,"[CallControllerManager - register] register finished, sid: " + accountSid + ", pwd:" + password);
		
	}

	void doMakeCallBack(final String selfPhoneNumber, final String destPhoneNumber , String srcSerNum , String destSerNum) {
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		final StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append(
				"/Calls/Callback");
		
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));
		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("CallBack");
		parameters.add("from", VoiceUtil.getStandardMDN(selfPhoneNumber));
		parameters.add("to", VoiceUtil.getStandardMDN(VoiceUtil.getStandardMDN(destPhoneNumber)));
		if(!TextUtils.isEmpty(srcSerNum)) {
			parameters.add("fromSerNum", srcSerNum);
		}
		if(!TextUtils.isEmpty(destSerNum)) {
			parameters.add("customerSerNum", destSerNum);
		}

		try {
			if (listener != null) {
				listener.onCallBack(SdkErrorCode.SDK_NET_CONNETING, selfPhoneNumber, destPhoneNumber);
			}

			try {
				Thread.sleep(150L);
			} catch (InterruptedException e) {
			}
			String xml = HttpManager.doRequestUrl(url.toString(), HttpManager.HTTPMETHOD_POST, parameters);

			Response response = ApiParser.doParser(
					ApiParser.KEY_NETWORK_CALLBACK, new ByteArrayInputStream(
							xml.getBytes()));
			int status = -1;
			if (response != null) {
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

			if (listener != null) {
				listener.onCallBack(status, selfPhoneNumber, destPhoneNumber);
			}
		} catch (Exception e) {
			int errorCode = handleSDKRequestException(e);
			if (listener != null) {
				listener.onCallBack(errorCode, selfPhoneNumber, destPhoneNumber);
			}
		} finally {
			
			clearRequestCache(parameters);
		}
	}

	void doQuerySoftSwitchAddress() throws Exception {
		if(this.userAgentConfig == null) {
			return ;
		}
		doQuerySoftSwitchAddress(this.userAgentConfig.getPrivateCloud() ,this.userAgentConfig.getSid(), this.userAgentConfig.getSubaccountid(), this.userAgentConfig.getSubpassword());
	}
	
	void doQuerySoftSwitchAddress(String privateCloud , String accountId, String subAccountId, String subAuthToken) throws Exception {
		String  formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());
		StringBuffer url = getStringBuffer();
		url.append("/Switchs/").append(accountId);
		
		// REST get a private cloud soft exchange interface (Xin Wei)
		// @version 3.6
		if(!TextUtils.isEmpty(privateCloud)) {
			url.append("/").append(privateCloud);
		}
		
		url.append("?sig=").append(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));
		url.append("&deviceNo=").append(VoiceUtil.createDeviceNo(this.context));
		url.append("&ua=").append(Base64.encode(UserAgentUtils.getUser_Agent(getContext()).getBytes()));
		
		checkSecurityUrl(url);
		
		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		// Initialize the get request parameters
		CCPParameters params = new CCPParameters();
		params.add("Authorization", authorization);
		
		String method = HttpManager.HTTPMETHOD_GET;
		boolean encrypt = false;
		if(!TextUtils.isEmpty(privateCloud) && url.indexOf(privateCloud) != -1) {
			encrypt = true;
			method = HttpManager.HTTPMETHOD_POST;
			params.setParamerTagKey("Switch");
			params.add("validate", this.userAgentConfig.getValidate());
			params.add("voippwd", this.userAgentConfig.getPassword());
		}
		
		int status = SdkErrorCode.SDK_UNKNOW_ERROR;
		try {
			String xml = HttpManager.doRequestUrl(url.toString(), method, params , encrypt);
			Response response = ApiParser.doParser(ApiParser.KEY_SOFTSWITCH_ADDRESS, new ByteArrayInputStream(
							xml.getBytes()));

			if (response == null) {
				Log4Util.e(Device.TAG, "get soft address return null.");
				//throw new IllegalStateException("get soft address return null.");
				status = SdkErrorCode.SDK_REQUEST_TIMEOUT;
			}

			if (response.isError()) {
				Log4Util.e(Device.TAG, "get soft address return errors: "
						+ response.statusCode);
				//throw new IllegalStateException("get soft address return errors: "+ response.statusCode);
				try {
					status = Integer.valueOf(response.statusCode);
				} catch (Exception e) {}
			}

			if (response instanceof SoftSwitch) {
				status = SdkErrorCode.REQUEST_SUCCESS;
				SoftSwitch sswitch = (SoftSwitch) response;
				
				if(sswitch.getSoftClpss() != null && !sswitch.getSoftClpss().isEmpty()) {
					Clpss clpss = sswitch.getSoftClpss().get(0);
					if (clpss.getIp().equals(softSwitchDefaultAddress)) {
						Log4Util.w(Device.TAG,"currently soft address was samed with last.");
						// return;
					} else {
						softSwitchDefaultAddress = clpss.getIp();
						softSwitchDefaultPort = clpss.getPort();
					}
				}
				registBody = sswitch.toXMLBody();
				control = sswitch.getControl();
				Log4Util.d("nwgid " + sswitch.getNetworkGroupId());
				NativeInterface.SetNetworkGroupId(sswitch.getNetworkGroupId());
				if(!TextUtils.isEmpty(sswitch.getP2PServerPort())) {
					// set p2p server 2013/9/10
					userAgentConfig.setP2pServerPort(sswitch.getP2PServerPort());
					// add 
					setStunServer(sswitch.getP2PServerPort());
				}
			}
			Log4Util.i(Device.TAG, "Request successful, SoftSwitch : " + registBody);
			register(registBody ,userAgentConfig.getSid(), userAgentConfig.getPassword());
			return ;
		} catch (Exception e) {
			e.printStackTrace();
			//throw e;
			status = handleSDKRequestException(e);
		} finally {
			// 
			clearRequestCache(params);
		}
		if (getListener() != null && status != SdkErrorCode.REQUEST_SUCCESS) {
			getListener().onConnectError(status);
		}
	}
	
	
	void QueryNWGID() throws Exception {
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/QueryNWGID");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);
		
		parameters.setParamerTagKey("Request");
		parameters.add("sha", Base64.encode((VoiceUtil.getIMEI(getContext()) + ";" + VoiceUtil.getLocalMacAddress(getContext())).getBytes()));
		
		String networkGroupId = null;
		String message = null;
		int status = -1;
		try {

			String xml = HttpManager.doRequestUrl(url.toString(), HttpManager.HTTPMETHOD_POST, parameters);
			
			Response response = ApiParser.doParser(
					ApiParser.KEY_NETWORK_GROUPID, new ByteArrayInputStream(xml
							.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
					SoftSwitch sswitch = (SoftSwitch) response;
					networkGroupId = sswitch.getNetworkGroupId();
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}
			
			if(!TextUtils.isEmpty(networkGroupId)) {
				if(VoiceUtil.isNetWorkTypeWIFI()) {
					NativeInterface.SetNetworkGroupId(networkGroupId);
				}
			}

		} catch (Exception e) {
			status = handleSDKRequestException(e);
			
		} finally {
			clearRequestCache(parameters);
		}

		
	}

	

	private void clearRequestCache(CCPParameters params) {
		if(params != null) {
			params.clear();
			params = null;
		}
	}

	private void checkSecurityUrl(StringBuffer url) {
		if (!URLUtil.isHttpsUrl(url.toString()) && !URLUtil.isHttpUrl(url.toString())) {
			throw new RuntimeException("login security address invalid.");
		}
	}



	/**
	 * 创建实时对讲
	 * 
	 * @param members
	 * @param type
	 * @param appid
	 */
	void doStartInterphone(String[] members, String type, String appid) {

		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/interphone/create");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);
		
		parameters.setParamerTagKey("Interphone");
		parameters.add("appId", appid);
		parameters.add("type", type);
		
		StringBuffer body = new StringBuffer();
		for (int i = 0; i < members.length; i++) {
			body.append("\t<member>").append(members[i])
					.append("</member>\r\n");
		}
		parameters.add("members", body.toString());

		String confN = null;
		String message = null;
		int status = -1;
		try {

			String xml = HttpManager.doRequestUrl(url.toString(), HttpManager.HTTPMETHOD_POST, parameters);
			
			Response response = ApiParser.doParser(
					ApiParser.KEY_STARTINTERPHONE, new ByteArrayInputStream(xml
							.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
					InterphoneMsg interphone = (InterphoneMsg) response;
					confN = interphone.interphoneId;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

		} catch (Exception e) {
			status = handleSDKRequestException(e);
			
		} finally {
			clearRequestCache(parameters);
		}
		
		if (getListener() != null) {
			getListener().onInterphoneState(CallbackHandler.getCloopenReason(status, message), confN);
		}

	}

	/**
	 * 
	 * @param e
	 * @return
	 */
	private int handleSDKRequestException(Exception e) {
		e.printStackTrace();
		Log4Util.e(Device.TAG, e.getMessage());
		if(e instanceof CCPHttpException) {
			CCPHttpException httpException = (CCPHttpException) e;
			return (httpException.getStatusCode());
			
		} else if (e instanceof CCPXmlParserException) {
			return SdkErrorCode.SDK_XMLBODY_ERROR;
			
		} else {
			return SdkErrorCode.SDK_UNKNOW_ERROR;
		}
	}

	/**
	 * 抢麦
	 * 
	 * @param confNo
	 */
	void doControlMIC(String confNo) {

		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/interphone/robMic");

		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("Interphone");
		parameters.add("interphoneId", confNo);

		String speaker = null;
		String message = null;
		int status = -1;
		
		try {

			String xml = HttpManager.doRequestUrl(url.toString(), HttpManager.HTTPMETHOD_POST, parameters);

			Response response = ApiParser.doParser(ApiParser.KEY_CONTROLMIC,
					new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				try {
					status = Integer.parseInt(response.statusCode);
				} catch (NumberFormatException e) {
				}
				if (response.isError()) {
					InterphoneControlMic mic = (InterphoneControlMic) response;
					speaker = mic.speaker;
				} else {
					speaker = userAgentConfig.getSid();
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}


		} catch (Exception e) {
			status = handleSDKRequestException(e);

		} finally {
			
			clearRequestCache(parameters);
		}

		if (getListener() != null) {
			getListener().onControlMicState(CallbackHandler.getCloopenReason(status, message), speaker);
		}
	}

	/**
	 * 放麦
	 * 
	 * @param confNo
	 */
	void doReleaseMIC(String confNo) {
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/interphone/releaseMic");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("Interphone");
		parameters.add("interphoneId", confNo);

		int status = -1;
		String message = null;
		try {

			String xml = HttpManager.doRequestUrl(url.toString(), HttpManager.HTTPMETHOD_POST, parameters);
			
			Response response = ApiParser.doParser(ApiParser.KEY_RELEASEMIC,
					new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

		} catch (Exception e) {

			status = handleSDKRequestException(e);
		} finally {
			
			clearRequestCache(parameters);
		}

		if (getListener() != null) {
			getListener().onReleaseMicState(CallbackHandler.getCloopenReason(status, message));
		}
	}

	/**
	 * Query interphone members
	 * 
	 * @param confNo
	 */
	void doQueryMembersWithInterphone(String confNo) {

		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/interphone/members");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("Interphone");
		parameters.add("interphoneId", confNo);

		int status = -1;
		String message = null;
		ArrayList<InterphoneMember> interphoneMembers = null;
		try {

			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);

			Response response = ApiParser.doParser(
					ApiParser.KEY_INTERPHONE_MEMBER_LIST,
					new ByteArrayInputStream(xml.getBytes()));

			InterphoneMemberList list = null;
			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
					list = (InterphoneMemberList) response;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

			interphoneMembers = list == null ? null : list.interphoneMemberList;
		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		
		if (getListener() != null) {
			getListener().onInterphoneMembers(CallbackHandler.getCloopenReason(status, message), interphoneMembers);
		}

	}

	/**********************************************************************
	 * ChatRoom *
	 **********************************************************************/
	/**
	 * @param appid
	 * @param roomName
	 * @param square
	 * @param keywords
	 * @param pwd
	 * @param isAutoClose is auto dismiss chatroom.
	 */
	void doStartChatroom(String appid, String roomName, int square,
			String keywords, String pwd , int isAutoClose , int voiceMod ,int isAutoDelete) {

		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/chatroom/create");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("ChatRoom");
		parameters.add("appId", appid);
		parameters.add("square", square);
		parameters.add("roomName", roomName);
		if (!TextUtils.isEmpty(pwd)) {
			parameters.add("pwd", pwd);
		}
		if (!TextUtils.isEmpty(keywords)) {
			parameters.add("keywords", keywords);
		}
		parameters.add("isAutoClose", isAutoClose);
		parameters.add("autoDelete", isAutoDelete);
		parameters.add("voiceMod", voiceMod);
		
		String ChatRoomId = null;
		String message = null;
		int status = -1;
		try {
			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			
			Response response = ApiParser.doParser(ApiParser.KEY_STARTCHATROOM,
					new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
					ChatroomMsg chatRoomMsg = (ChatroomMsg) response;
					if (chatRoomMsg != null) {
						ChatRoomId = chatRoomMsg.getRoomNo();
					}
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		
		if (getListener() != null) {
			getListener().onChatRoomState(CallbackHandler.getCloopenReason(status, message), ChatRoomId);
		}

	}

	/**
	 * 解散群组
	 * @param appId
	 * @param chatroomid
	 */
	void doDismissChatRomm(String appId, String chatroomid)
	{
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/chatroom/dismiss");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));
		
		checkSecurityUrl(url);
		
		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("ChatRoom");
		parameters.add("appId", appId);
		parameters.add("roomId", chatroomid);
	 
		int status = -1;
		String message = null;
		try {
			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			Response response = ApiParser.doParser(ApiParser.KEY_DISMISS_CHATROOM,new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		if (getListener() != null) {
			getListener().onChatRoomDismiss(CallbackHandler.getCloopenReason(status, message), chatroomid);
		}
	}
	
	void doRemoveMemberFromChatRoom(String appId, String chatroomid, String member)
	{
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/chatroom/remove");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));
		
		checkSecurityUrl(url);
		
		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("ChatRoom");
		parameters.add("appId", appId);
		parameters.add("roomId", chatroomid);
		parameters.add("mobile", member);

		int status = -1;
		String message = null;
		try {
			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			Response response = ApiParser.doParser(ApiParser.KEY_REMOVE_MEMBER_CHATROOM,new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		
		if (getListener() != null) {
			getListener().onChatRoomRemoveMember(CallbackHandler.getCloopenReason(status, message), member);
		}
	}
	
	void doSetChatroomSpeakType(String appId, String chatroomid, String member , int opt)
	{
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/chatroom/allowSpeak");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));
		
		checkSecurityUrl(url);
		
		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("ChatRoom");
		parameters.add("appId", appId);
		parameters.add("roomId", chatroomid);
		parameters.add("number", member);
		parameters.add("opt", opt);

		int status = -1;
		String message = null;
		try {
			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			Response response = ApiParser.doParser(ApiParser.KEY_CHATROOM_SPEAK_OPREATE,new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		
		if (getListener() != null) {
			getListener().onSetChatroomSpeakOpt(CallbackHandler.getCloopenReason(status, message), member);
		}
	}
	
	/**
	 * 查询聊天室列表
	 * 
	 * @param appId
	 * @param keywords
	 */
	void doQueryChatRooms(String appId, String keywords) {

		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/chatroom/roomlist");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("ChatRoom");
		parameters.add("appId", appId);
		if (!TextUtils.isEmpty(keywords)) {
			parameters.add("keywords", keywords);
		}

		int status = -1;
		String message = null;
		List<Chatroom> chatRoomList = null;
		try {

			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			
			Response response = ApiParser.doParser(
					ApiParser.KEY_QUERY_CHATROOM_LIST,
					new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
					ChatRoomList list = (ChatRoomList) response;
					if(list != null) {
						chatRoomList = list.chatroomInfos;
					}
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		
		if (getListener() != null) {
			getListener().onChatRooms(CallbackHandler.getCloopenReason(status, message), chatRoomList);
		}

	}

	/**
	 * 邀请加入聊天室
	 * 
	 * @param members
	 * @param roomNo
	 * @param appId
	 */
	void doInviteMembersJoinChatroom(String[] members, String roomNo,
			String appId) {

		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/chatroom/invite");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("ChatRoom");
		parameters.add("appId", appId);
		parameters.add("roomId", roomNo);
		StringBuffer body = new StringBuffer();
		for (int i = 0; i < members.length; i++) {
			body.append("\t<mobile>").append(members[i])
					.append("</mobile>\r\n");
		}
		parameters.add("mobiles", body.toString());

		int status = -1;
		String message = null;
		try {

			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			Response response = ApiParser.doParser(
					ApiParser.KEY_INVITE_CHATROOM, new ByteArrayInputStream(xml
							.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}

		if (getListener() != null) {
			getListener().onChatRoomInvite(CallbackHandler.getCloopenReason(status, message), roomNo);
		}
	}

	/**
	 * 语音群聊 - 获取房间成员列表语音群聊 - 获取房间成员列表
	 * @param chatRoomId
	 */
	void doQueryMembersWithChatroom(String chatRoomId) {
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/chatroom/members");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("ChatRoom");
		parameters.add("roomId", chatRoomId);

		List<ChatroomMember> member = null;
		int status = -1;
		String message = null;
		try {

			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			Response response = ApiParser.doParser(
					ApiParser.KEY_CHATROOM_MEMBER_LIST,
					new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
					ChatRoomMemberList list = (ChatRoomMemberList) response;
					if(list != null) {
						member = list.chatRoomInfos;
					}
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}
		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		if (getListener() != null) {
			getListener().onChatRoomMembers(CallbackHandler.getCloopenReason(status, message), member);
		}
	}

	/**********************************************************************
	 * media message *
	 **********************************************************************/
	/**
	 * 发送多媒体文件(文件等)
	 * 
	 * @param fileName
	 * @param receiver
	 * @param userData 
	 */
	void doSendMediaMsg(String uniqueID, String fileName, String receiver, String userData) {

		String accountId = this.userAgentConfig.getSid();
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());


		File file = new File(fileName);
		if (!file.exists()) {
			throw new RuntimeException("The file URL does not exist,  " + fileName+ ".");
		}
		
		String state = android.os.Environment.getExternalStorageState();
		if (!state.equals(android.os.Environment.MEDIA_MOUNTED)) {
			throw new RuntimeException("SD Card is not mounted,It is  " + state
					+ ".");
		}
		
		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/IM/SendMsg");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);
		
		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("InstanceMessage");
		parameters.add("sender", accountId);
		parameters.add("receiver", receiver);
		parameters.add("type", 0);
		parameters.add("fileSize", file.length());

		// V3.0.1 version added request packet body with questions, view the function header files
		parameters.add("msgId", uniqueID);
		if(!TextUtils.isEmpty(userData)) {
			parameters.add("userData", userData);
		}
		String ext = VoiceUtil.getExtensionName(fileName);
		
		if(fileName.length() == ext.length()) {
			ext = "";
		}
		parameters.add("fileExt", ext);

		int status = -1;
		String message = null;
		IMAttachedMsg imAttachedMsg = null;
		try {
			
			UploadImessage response = querySendMediaMsgServerAddress(url.toString(), parameters);
			
			if(response == null) {
				Log4Util.e(Device.TAG, "Get the file server address error .");
				throw new IllegalStateException("Get the file server address error .");
			}
			
			status = Integer.parseInt(response.statusCode);
			message = response.getStatusMsg();
			if(response.isError() 
					|| TextUtils.isEmpty(response.getUploadUrl())
					|| response.mediaMsg == null) {
				if (getListener() != null) {
					getListener().onSendInstanceMessage(CallbackHandler.getCloopenReason(status, message), response.mediaMsg);
				}
				return;
			}
			
			imAttachedMsg = response.mediaMsg;
			imAttachedMsg.setMsgId(uniqueID);
			imAttachedMsg.setExt(ext);
			imAttachedMsg.setFileUrl(fileName);
			formatTimestamp = VoiceUtil.formatTimestamp(System
					.currentTimeMillis());
			
			StringBuffer uploadUrl = new StringBuffer(response.getUploadUrl());
			uploadUrl.append("?token=").append(response.getUploadToken());
			
			Log4Util.w(Device.TAG, "url: " + uploadUrl + "\r\n");
			
			String xml = HttpClientUtil.postRequestUploadFileChunk(uploadUrl.toString(), fileName);
			Log4Util.w(Device.TAG, xml + "\r\n");
			
			Response uVoiceRes = ApiParser.doParser(
					ApiParser.KEY_PARSER_STATUSCODE, new ByteArrayInputStream(xml
							.getBytes()));
			imAttachedMsg.setFileSize(file.length());
			imAttachedMsg.setSender(accountId);
			imAttachedMsg.setReceiver(receiver);
			
			if (uVoiceRes != null) {
				response.statusCode = uVoiceRes.statusCode;
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
				message = response.getStatusMsg();
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}
		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		
		if (getListener() != null) {
			imAttachedMsg = new IMAttachedMsg(uniqueID);
			imAttachedMsg.setExt(ext);
			imAttachedMsg.setFileUrl(fileName);
			getListener().onSendInstanceMessage(CallbackHandler.getCloopenReason(status, message), imAttachedMsg);
		}
	}

	UploadImessage querySendMediaMsgServerAddress(String url, CCPParameters parameters) throws CCPException{
		
		String xml = HttpManager.doRequestPostUrl(url, parameters);
		Log4Util.w(Device.TAG, xml + "\r\n");
		return (UploadImessage) ApiParser.doParser(ApiParser.KEY_SEND_MEIDAMSG,
				new ByteArrayInputStream(xml.getBytes()));
	}

	/**
	 * Modify the MEDIA message status on the REST server This method will be
	 * required returned results immediately after the completion of execute
	 * 
	 * @param msgId
	 */
	void doConfirmDownloadMediaMessage(String[] msgId) {

		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();

		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/IM/Done");
		
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));
		
		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);
		parameters.setParamerTagKey("InstanceMessage");
		
		if(msgId != null) {
			StringBuffer body = new StringBuffer();
			for (int i = 0; i < msgId.length; i++) {
				body.append("\t<msgId>").append(msgId[i]).append("</msgId>\r\n");
			}
			parameters.add("InstanceMessage", body.toString());
		}
		
		int status = -1;
		String message = null;
		try {
			String xml = HttpManager.doRequestUrl(url.toString(), HttpManager.HTTPMETHOD_POST, parameters);
			
			Response response = ApiParser.doParser(
					ApiParser.KEY_SOFTSWITCH_ADDRESS, new ByteArrayInputStream(
							xml.getBytes()));

			
			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}
		} catch (Exception e) {
			status = handleSDKRequestException(e);
		}
		
		if(getListener() != null) {
			getListener().onConfirmIntanceMessage(CallbackHandler.getCloopenReason(status, message));
		}

	}

	/**
	 * 
	 * @param dLoadList
	 */
	void doDownloadAttachmentFiles(ArrayList<DownloadInfo> dLoadList) {
		if (dLoadList != null && !dLoadList.isEmpty()) {
			for (DownloadInfo dLoadInfo : dLoadList) {
				int status = SdkErrorCode.SDK_UNKNOW_ERROR;
				String url = null;
				try {
					if (dLoadInfo != null && !TextUtils.isEmpty(dLoadInfo.getUploadurl())) {
						//int fileExtIndex = dLoadInfo.getUploadurl().lastIndexOf(".");
						//String fileExt = dLoadInfo.getUploadurl().substring(fileExtIndex + 1, dLoadInfo.getUploadurl().length());
						String fileExt = VoiceUtil.getExtensionName(dLoadInfo.getUploadurl());
						if(dLoadInfo.isChunked() && "amr".equals(fileExt)) {
							// If it is chunked, chunked is used to download, 
							// this time do not need to judge a voice recording 
							// marks the end of is it right?
							status = HttpClientUtil.postRequestDownload(dLoadInfo.getUploadurl(), dLoadInfo.getSavePath());
							
						} else {
							status = HttpManager.httpDowloadFile(dLoadInfo.getUploadurl(), dLoadInfo.getSavePath());
						}
						Log4Util.w(Device.TAG, "url: " + dLoadInfo.getUploadurl() + "\r\n");
						
						url = dLoadInfo.getSavePath();
					}
				} catch (Exception e) {
					e.printStackTrace();
					status = SdkErrorCode.SDK_UNKNOW_ERROR;
				}
				if (getListener() != null) {
					getListener().onDownloadAttached(CallbackHandler.getCloopenReason(status), url);
				}
			}
		}
	}
	
	
	void doCCPDownload(ArrayList<DownloadInfo> dLoadList) {
		if (dLoadList != null && !dLoadList.isEmpty()) {
			for (DownloadInfo dLoadInfo : dLoadList) {
				int reason = SdkErrorCode.SDK_XML_ERROR;
				try {
					if (dLoadInfo != null && !TextUtils.isEmpty(dLoadInfo.getUploadurl())) {
						Log4Util.w(Device.TAG, "url: " + dLoadInfo.getUploadurl() + "\r\n");
						
						if (HttpManager.httpDowload(dLoadInfo.getUploadurl(), dLoadInfo.getSavePath())) {
							reason = 0;
						} else {
							reason = SdkErrorCode.SDK_XML_ERROR;
						}
						
					}
				} catch (Exception e) {
					e.printStackTrace();
					reason = SdkErrorCode.SDK_XML_ERROR;
					/*if (getListener() != null) {
						getListener().onDownloadAttached(SDKErrorCode.SDK_UNKNOW_ERROR, null);
					}*/
				}
				doRedirectCCPDownloadCallback(reason, dLoadInfo);
			}
		}
	}
	
	/**
	 * 
	* <p>Title: doRedirectCCPDownloadCallback</p>
	* <p>Description: </p>
	* @param reason
	* @param downloadInfo
	* 
	* @version 3.5
	* 
	* @deprecated
	 */
	void doRedirectCCPDownloadCallback(int reason , DownloadInfo downloadInfo) {
		
		if(downloadInfo == null || getListener() == null) {
			return;
		}
		
	}
	
	
	void doDownloadVideoConferencePortraits(ArrayList<VideoPartnerPortrait> portraitsList) {
		if (portraitsList != null && !portraitsList.isEmpty()) {
			for (VideoPartnerPortrait portraits : portraitsList) {
				int reason = SdkErrorCode.SDK_XML_ERROR;
				try {
					if (portraits != null && !TextUtils.isEmpty(portraits.getFileUrl())) {
						//String url = portraits.getFileUrl() + "?fileName=" + portraits.getFileName();
						Log4Util.w(Device.TAG, "doDownloadVideoConferencePortraits.url: " + portraits.getFileUrl() + "\r\n");
						
						if (HttpManager.httpDowload(portraits.getFileUrl(), portraits.getFileLocalPath())) {
							reason = 0;
						} else {
							reason = SdkErrorCode.SDK_HTTP_ERROR;
						}
						
					}
				} catch (Exception e) {
					e.printStackTrace();
					reason = SdkErrorCode.SDK_UNKNOW_ERROR;
				}
				if (getListener() != null) {
					getListener().onDownloadVideoConferencePortraits(CallbackHandler.getCloopenReason(reason), portraits);
				}
				
			}
		}
	}
	
	
	
	/**
	 * use unbuffered chunk-encoded POST reques
	 * @param fileName
	 * @param duration
	 * @param groupId
	 * @param toVoip
	 */
	void doUploadMediaChunked(String uniqueID, String fileName, String receiver, String userData) {
		
		String accountId = this.userAgentConfig.getSid();
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());
		
		StringBuffer httpsurl = getStringBuffer();
		httpsurl.append("/SubAccounts/").append(subAccountId).append("/IM/SendMsg");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));
		
		checkSecurityUrl(httpsurl);
		
		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);
		
		parameters.setParamerTagKey("InstanceMessage");
		parameters.add("sender" , accountId);
		parameters.add("receiver" , receiver);
		parameters.add("type" , 1);
		// add zhanc 
		parameters.add("fileSize" , 0);
		
		// V3.0.1 version added request packet body with questions, view the function header files
		parameters.add("msgId" , uniqueID);
		if(!TextUtils.isEmpty(userData)) {
			parameters.add("userData" , userData);
		}
		parameters.add("fileExt" , "amr");
		
		int status = -1;
		String message = null;
		IMAttachedMsg imAttachedMsg = null;
		try {
			UploadImessage response = querySendMediaMsgServerAddress(httpsurl.toString(), parameters);

			if(response == null) {
				Log4Util.e(Device.TAG, "Get the file server address error .");
				throw new IllegalStateException("Get the file server address error .");
			}
			
			status = Integer.parseInt(response.statusCode);
			message = response.getStatusMsg();
			if(response.isError() 
					|| TextUtils.isEmpty(response.getUploadUrl())
					|| response.mediaMsg == null) {
				if (getListener() != null) {
					getListener().onSendInstanceMessage(CallbackHandler.getCloopenReason(status, message), response.mediaMsg);
				}
				return;
			}
			
			imAttachedMsg = response.mediaMsg;
			imAttachedMsg.setMsgId(uniqueID);
			imAttachedMsg.setFileUrl(fileName);
			imAttachedMsg.setExt("amr");
			imAttachedMsg.setSender(accountId);
			imAttachedMsg.setReceiver(receiver);
			
			StringBuffer url = new StringBuffer(response.getUploadUrl());
			url.append("?token=").append(response.getUploadToken());
			Log4Util.w(Device.TAG, "url: " + url + "\r\n");
				
			String xml = HttpClientUtil.postRequestUploadChunk(url.toString());
			Log4Util.w(Device.TAG, xml + "\r\n");
			
			Response uVoiceRes = ApiParser.doParser(ApiParser.KEY_PARSER_STATUSCODE,new ByteArrayInputStream(xml.getBytes()));
			
			if (uVoiceRes != null) {
				response.statusCode = uVoiceRes.statusCode;
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
				message = response.getStatusMsg();
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}
			
		} catch (Exception e) {
			status = handleSDKRequestException(e);
			imAttachedMsg = new IMAttachedMsg(uniqueID);
			imAttachedMsg.setFileUrl(fileName);
			imAttachedMsg.setExt("amr");
		} finally {
			clearRequestCache(parameters);
		}
		if (getListener() != null) {
			getListener().onSendInstanceMessage(CallbackHandler.getCloopenReason(status, message), imAttachedMsg);
		}
	}
	
	
	@Deprecated
	void doStartVideoConference(String appid, String roomName, int square,
			String keywords, String pwd ,int isAutoClose) {
		
		doStartVideoConference(appid, roomName, square, keywords, pwd, isAutoClose, 1, 1);
	}
	
	/**
	 * @param appid
	 * @param roomName
	 * @param square
	 * @param keywords
	 * @param pwd
	 * @param isAutoClose 创建者退出是否自动解散
	 * @param voiceMod 0没有提示音有背景音，、1全部提示音、2无提示音无背景音
	 * 					默认值为1全部提示音
	 * @param isAutoDelete 是否自动删除，默认值为1自动删除。
	 */
	void doStartVideoConference(String appid, String roomName, int square,
			String keywords, String pwd ,int isAutoClose ,int voiceMod ,int isAutoDelete) {

		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/video/create");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("VideoConf");
		parameters.add("appId", appid);
		parameters.add("square", square);
		parameters.add("roomName", roomName);
		if (!TextUtils.isEmpty(pwd)) {
			parameters.add("pwd", pwd);
		}
		if (!TextUtils.isEmpty(keywords)) {
			parameters.add("keywords", keywords);
		}
		parameters.add("isAutoClose", isAutoClose);
		parameters.add("autoDelete", isAutoDelete);
		parameters.add("voiceMod", voiceMod);
		
		String videoConference = null;
		int status = -1;
		String message = null;
		try {
			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			Response response = ApiParser.doParser(ApiParser.KEY_START_VIDEOCONFENERCE,
					new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
					VideoConferenceMsg roomMsg = (VideoConferenceMsg) response;
					if (roomMsg != null) {
						videoConference = roomMsg.getConferenceId();
					}
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		if (getListener() != null) {
			getListener().onVideoConferenceState(CallbackHandler.getCloopenReason(status, message), videoConference);
		}
	}
	
	void doStartVideoConference(String appid, String roomName, String pwd , ConferenceOptions options) {

		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/video/create");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("VideoConf");
		parameters.add("appId", appid);
		parameters.add("square", options.inSquare);
		parameters.add("roomName", roomName);
		if (!TextUtils.isEmpty(pwd)) {
			parameters.add("pwd", pwd);
		}
		if (!TextUtils.isEmpty(options.inKeywords)) {
			parameters.add("keywords", options.inKeywords);
		}
		parameters.add("isAutoClose", options.inAutoClose ? 0 : 1);
		parameters.add("autoDelete", options.inAutoDelete ? 1 : 0);
		parameters.add("voiceMod", options.inVoiceMod);
		parameters.add("isMultiVideo", options.inMultiVideo ? 1 : 0);
		parameters.add("isPresenter", options.inPresenter ? 1 : 0);
		
		String videoConference = null;
		int status = -1;
		String message = null;
		try {
			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			Response response = ApiParser.doParser(ApiParser.KEY_START_VIDEOCONFENERCE,
					new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
					VideoConferenceMsg roomMsg = (VideoConferenceMsg) response;
					if (roomMsg != null) {
						videoConference = roomMsg.getConferenceId();
					}
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		if (getListener() != null) {
			getListener().onVideoConferenceState(CallbackHandler.getCloopenReason(status, message), videoConference);
		}
	}

	
	void doQueryVideoConferences(String appId, String keywords) {

		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/video/roomlist");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("VideoConf");
		parameters.add("appId", appId);
		
		if (!TextUtils.isEmpty(keywords)) {
			parameters.add("keywords", keywords);
		}

		List<VideoConference> conferences = null;
		int status = -1;
		String message = null;
		try {

			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);

			Response response = ApiParser.doParser(
					ApiParser.KEY_QUERY_VIDEOCONFENERCE_LIST,
					new ByteArrayInputStream(xml.getBytes()));
			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
					VideoConferenceList list  = (VideoConferenceList) response;
					if(list != null ) {
						conferences = list.videoConferences;
					}
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		if (getListener() != null) {
			getListener().onVideoConferences(CallbackHandler.getCloopenReason(status, message), conferences);
		}
	}

	void doQueryMembersInVideoConference(String conferenceId) {
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/video/members");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);

		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("VideoConf");
		parameters.add("roomId", conferenceId);

		int status = -1;
		String message = null;
		ArrayList<VideoConferenceMember> videoConferenceList = null;
		try {

			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			Response response = ApiParser.doParser(
					ApiParser.KEY_VIDEOCONFENERCE_MEMBER_LIST,
					new ByteArrayInputStream(xml.getBytes()));

			VideoConferenceMemberList list = null;
			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
					list = (VideoConferenceMemberList) response;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}
			videoConferenceList = list == null? null : list.videoConferenceMembers;
		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		if (getListener() != null) {
			getListener().onVideoConferenceMembers(CallbackHandler.getCloopenReason(status, message), videoConferenceList);
		}
	}
	/**
	 * 
	* <p>Title: doDismissVideoConference</p>
	* <p>Description: The dissolution of video conference</p>
	* @param appId The application of ID
	* @param conferenceId the Video Conference No. ID
	* @version 3.5
	 */
	void doDismissVideoConference(String appId , String conferenceId)
	{
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/video/dismiss");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);
		
		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("VideoConf");
		parameters.add("appId", appId);
		parameters.add("roomId", conferenceId);
	 
		int status = -1;
		String message = null;
		try {
			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);

			Response response = ApiParser.doParser(ApiParser.KEY_DISMISS_VIDEOCONFENERCE,new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		if (getListener() != null) {
			getListener().onVideoConferenceDismiss(CallbackHandler.getCloopenReason(status, message), conferenceId);
		}
	}
	
	/**
	 * 
	* <p>Title: doRemoveMemberFromVideoConference</p>
	* <p>Description: The members play video conference</p>
	* @param appId  The application of ID
	* @param conferenceId the Video Conference No. ID
	* @param member Need to play the member account
	* @version 3.5
	 */
	void doRemoveMemberFromVideoConference(String appId , String conferenceId , String member)
	{
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/video/remove");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);
		
		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("VideoConf");
		parameters.add("appId", appId);
		parameters.add("roomId", conferenceId);
		parameters.add("voip", member);

		int status = -1;
		String message = null;
		try {
			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			
			Response response = ApiParser.doParser(ApiParser.KEY_REMOVE_MEMBER_VIDEOCONFENERCE,new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}
		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		if (getListener() != null) {
			getListener().onVideoConferenceRemoveMember(CallbackHandler.getCloopenReason(status, message), member);
		}
	}
	
	
	void doPublishVideoRequest(String appId , String conferenceId) {
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/video/publish");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);
		
		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("VideoConf");
		parameters.add("appId", appId);
		parameters.add("roomId", conferenceId);
		
		int status = -1;
		String message = null;
		try {
			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			
			Response response = ApiParser.doParser(ApiParser.KEY_REQUEST_DEFAULT,new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}
		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}

		if(getListener() != null) {
			getListener().OnPublishVideoFrameRequest(0,CallbackHandler.getCloopenReason(status, message));
		}
	}
	
	void doUnPublishVideoRequest(String appId , String conferenceId) {
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/video/unpublish");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);
		
		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("VideoConf");
		parameters.add("appId", appId);
		parameters.add("roomId", conferenceId);
		
		int status = -1;
		String message = null;
		try {
			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			
			Response response = ApiParser.doParser(ApiParser.KEY_REQUEST_DEFAULT,new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}
		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		if(getListener() != null) {
			getListener().OnPublishVideoFrameRequest(1,CallbackHandler.getCloopenReason(status, message));
		}
	}
	
	/**
	 * 
	* <p>Title: doswitchRealScreenToVoip</p>
	* <p>Description:switch Video Conference where voip  </p>
	* @param appId The application of ID
	* @param conferenceId The need to switch the Video Conference No. ID
	* @param voip of Video Conference
	* @version 3.5
	 */
	void doswitchRealScreenToVoip(String appId , String conferenceId , String voip)
	{
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/video/changed");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);
		
		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("VideoConf");
		parameters.add("appId", appId);
		parameters.add("roomId", conferenceId);
		parameters.add("toVoip", voip);

		int status = -1;
		String message = null;
		try {
			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);
			Response response = ApiParser.doParser(ApiParser.KEY_SWITCH_VIDEOCONFENERCE,new ByteArrayInputStream(xml.getBytes()));

			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}

		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		if (getListener() != null) {
			getListener().onSwitchRealScreenToVoip(CallbackHandler.getCloopenReason(status, message));
		}
	}
	
	
	/**
	 * 
	* <p>Title: doPortraitsFromVideoConference</p>
	* <p>Description: the Video Conference No. ID</p>
	* @param conferenceId the Video Conference No. ID
	 */
	void doPortraitsFromVideoConference(String conferenceId) {
		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/video/download");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		checkSecurityUrl(url);
		
		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("VideoConf");
		parameters.add("roomId", conferenceId);

		int status = -1;
		String message = null;
		ArrayList<VideoPartnerPortrait> videoPortraitList = null;
		try {
			String xml = HttpManager.doRequestPostUrl(url.toString(), parameters);

			Response response = ApiParser.doParser(ApiParser.KEY_VIDEOCONFENERCE_PORTRAIT,new ByteArrayInputStream(xml.getBytes()));

			VideoPartnerPortraitList list = null;
			if (response != null) {
				message = response.getStatusMsg();
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
					list = (VideoPartnerPortraitList) response;
				}
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}
			videoPortraitList = list == null ? null :list.videoPartnerPortraits;

		} catch (Exception e) {
			status = handleSDKRequestException(e);
		} finally {
			clearRequestCache(parameters);
		}
		if (getListener() != null) {
			getListener().onGetPortraitsFromVideoConference(CallbackHandler.getCloopenReason(status, message), videoPortraitList);
		}
	}
	
	
	void doSendLocalPortrait(String fileName, String conferenceId) {

		String subAccountId = this.userAgentConfig.getSubaccountid();
		String subAuthToken = this.userAgentConfig.getSubpassword();
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());

		StringBuffer url = getStringBuffer();
		url.append("/SubAccounts/").append(subAccountId).append("/video/upload");
		CCPParameters parameters = getCCPParameters(VoiceUtil.md5(subAccountId + subAuthToken + formatTimestamp));

		File file = new File(fileName);
		if (!file.exists()) {
			throw new RuntimeException("The file URL does not exist,  " + fileName+ ".");
		}
		
		String state = android.os.Environment.getExternalStorageState();
		if (!state.equals(android.os.Environment.MEDIA_MOUNTED)) {
			throw new RuntimeException("SD Card is not mounted,It is  " + state
					+ ".");
		}

		checkSecurityUrl(url);
		
		String authorization = Base64.encode((subAccountId + ":" + formatTimestamp).getBytes());
		parameters.add("Authorization", authorization);

		parameters.setParamerTagKey("VideoConf");
		parameters.add("roomId", conferenceId);
		parameters.add("fileSize", file.length());
		String ext = VoiceUtil.getExtensionName(fileName);
		
		if(fileName.length() == ext.length()) {
			ext = "";
		}
		parameters.add("fileExt", ext);

		int status = -1;
		String message = null;
		try {
			UploadImessage response = querySendMediaMsgServerAddress(url.toString(), parameters);
			
			if(response == null) {
				Log4Util.e(Device.TAG, "Get the file server address error .");
				throw new IllegalStateException("Get the file server address error .");
			}
			
			status = Integer.parseInt(response.statusCode);
			message = response.getStatusMsg();
			if(response.isError() 
					|| TextUtils.isEmpty(response.getUploadUrl())) {
				if (getListener() != null) {
					getListener().onSendLocalPortrait(CallbackHandler.getCloopenReason(status, message), conferenceId);
				}
				return;
			}
			
			formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());

			StringBuffer uploadUrl = new StringBuffer(response.getUploadUrl());
			uploadUrl.append("?token=").append(response.getUploadToken());

			Log4Util.w(Device.TAG, "url: " + uploadUrl + "\r\n");
				
			String xml = HttpClientUtil.postRequestUploadFileChunk(uploadUrl.toString(), fileName);
			Log4Util.w(Device.TAG, xml + "\r\n");
			
			Response uVoiceRes = ApiParser.doParser(
					ApiParser.KEY_PARSER_STATUSCODE, new ByteArrayInputStream(xml
							.getBytes()));

			if (uVoiceRes != null) {
				response.statusCode = uVoiceRes.statusCode;
				if (response.isError()) {
					try {
						status = Integer.parseInt(response.statusCode);
					} catch (NumberFormatException e) {
					}
				} else {
					status = 0;
				}
				message = response.getStatusMsg();
			} else {
				status = SdkErrorCode.SDK_XML_ERROR;
			}
			
		} catch (Exception e) {
			status = handleSDKRequestException(e);
			Log4Util.w(Device.TAG, "doSendLocalPorprtait result : " + status + "\r\n");
		} finally {
			clearRequestCache(parameters);
		}
		if(getListener() != null) {
			getListener().onSendLocalPortrait(CallbackHandler.getCloopenReason(status, message), conferenceId);
		}
		
	}
	
	/**
	 * 
	 * @param capacity
	 * @return
	 */
	CCPParameters getCCPParameters(String capacity) {
		
		CCPParameters parameters = new CCPParameters();
		if(!TextUtils.isEmpty(capacity))
			parameters.add("sig", capacity);
		
		return parameters;
	}
	
	
	
	protected void setStunServer (String server) {}

	void destroy() {
		if (this.userAgentConfig != null) {
			this.userAgentConfig.released();
		}
		this.userAgentConfig = null;
		this.listener = null;
		if (this.mServiceLooper != null) {
			this.mServiceLooper.quit();
		}
		this.mServiceHandler = null;
	}
	
	/**
	 * @return the userAgentConfig
	 */
	UserAgentConfig getUserAgentConfig() {
		return userAgentConfig;
	}

	/**
	 * @param userAgentConfig
	 *            the userAgentConfig to set
	 * @throws Exception
	 */
	void setUserAgentConfig(UserAgentConfig userAgentConfig)
			throws Exception {
		this.userAgentConfig = userAgentConfig;
	}

	/**
	 * @return the listener
	 */
	CCPCallEvent getListener() {
		return listener;
	}

	/**
	 * @param listener
	 *            the listener to set
	 */
	void setListener(CCPCallEvent listener) {
		this.listener = listener;
	}

	/**
	 * @return the context
	 */
	Context getContext() {
		return context;
	}

	/**
	 * @return the softSwitchAddress
	 */
	String getSoftSwitchAddress() {
		return softSwitchDefaultAddress;
	}

	/**
	 * @return the softSwitchPort
	 */
	int getSoftSwitchPort() {
		return softSwitchDefaultPort;
	}
	
	/**
	 * 
	 * @Title: getControl 
	 * @Description: TODO 
	 * @param @return 
	 * @return String 
	 * @throws
	 * @version 3.6
	 */
	String getControl() {
		return control;
	}

	/**
	 * @return the mServiceHandler
	 */
	public ServiceHandler getServiceHandler() {
		return mServiceHandler;
	}

	/**
	 * for outside 
	 * 
	 * @param command
	 */
	public void postCommand(Runnable command) {
		if (this.mServiceHandler != null) {
			this.mServiceHandler.post(command);
		} else {
			Log4Util.d(Device.TAG, "mServiceHandler is null, then init.");
			initServiceHandler();
		}
	}
	
	/**
	 * 
	* <p>Title: Constantr.java</p>
	* <p>Description: </p>
	* @author Jorstin Chan
	* @date 2013-11-15
	* @version 3.5
	 */
	protected class Constant {

		/**
		 * Uniquely identifies the application
		 */
		public static final String APP_ID 								= "com.ccp.phone.appId";
		
		/**
		 * Unique identification information, such as multimedia IM message
		 * @see IMTextMsg#getMsgId()
		 */
		public static final String CCP_MSGIDS		 					= "com.ccp.phone.msgIds";
		
		/**
		 * The largest number of participants that Create a Chatroom, Interphone or Video Conference
		 */
		public static final String CCP_SQUARE 							= "com.ccp.phone.square";
		
		/**
		 * This is a business field, by the developers in their own applications define 
		 * also {@link Constant#CCP_USERDATA}
		 */
		public static final String CCP_KEYWORDS 						= "com.ccp.phone.keywords";
		
		/**
		 * Set password when Create a Chatroom, Interphone or Video Conferencet
		 * Need to provide the password when apply to joined.
		 */
		public static final String CCP_PWD 								= "com.ccp.phone.pwd";
		
		/**
		 * For the member query involved of Chatroom, Interphone or Video Conference
		 */
		public static final String CCP_ARRAY_MEMBERS 					= "com.ccp.phone.members";
		
		/**
		 * 
		 */
		public static final String CCP_TYPE 							= "com.ccp.phone.type";
		
		/**
		 * Use for Identifies a room name, such as Chatrooms Interphone or Vide Conference.
		 */
		public static final String CCP_ROOM_NAME 						= "com.ccp.phone.roomName";
		
		/**
		 * 
		 */
		public static final String CCP_MEMBER 							= "com.ccp.phone.member";
		public static final String SPEAK_OPT							= "com.ccp.phone.speak_opt";
		
		/**
		 * Identifies the user's VoIP account
		 */
		public static final String CCP_VOIP 							= "com.ccp.phone.voip";
		
		/**
		 * The absolute path address file
		 */
		public static final String CCP_FILENAME 						= "com.ccp.phone.fileName";
		
		/**
		 * 
		 */
		public static final String CCP_RECEIVER 						= "com.ccp.phone.receiver";
		
		/**
		 * 
		 */
		public static final String CCP_UNIQUEID 						= "com.ccp.phone.uniqueID";
		
		/**
		 * {@link Constant#CCP_KEYWORDS}
		 */
		public static final String CCP_USERDATA 						= "com.ccp.phone.userData";
		
		/**
		 * Use for downloading files passe parameters .
		 */
		public static final String CCP_DOWNLOAD_PARAMETERS 				= "com.ccp.phone.downloadParameters";
		
		/**
		 * Use for downloading files passe parameters .
		 */
		public static final String CCP_DOWNLOAD_PARAMETERS_PORTRAIT 				= "com.ccp.phone.downloadParameters.PORTRAIT";
		
		/**
		 * This machine to identify users mobile phone number
		 */
		public static final String CCP_SELF_PHONENUMBER 				= Device.SELFPHONE;
		
		/**
		 * 
		 */
		public static final String CCP_DEST_PHONENUMBER 				= Device.DESTPHONE;
		
		/**
		 * The calling side number displayed, 
		 * depending on the platform side explicit rules control.
		 */
		public static final String CCP_SELF_SERNUM                      = "com.ccp.phone.srcSerNum";
		
		/**
		 * The called side show the customer service number, 
		 * depending on the platform side explicit rules control
		 */
		public static final String CCP_DEST_SERNUM                      = "com.ccp.phone.destSerNum";
		
		/**
		 * the Interphone No. ID
		 */
		public static final String INTERPHONE_ID 						= "com.ccp.phone.confNo";
		
		/**
		 * the Chatroom No. ID
		 */
		public static final String CHATROOM_ID 							= "com.ccp.phone.roomNo";
		
		/**
		 * the Video Conference No. ID
		 */
		public static final String CONFERENCE_ID 						= Device.CONFERENCE_ID;
		public static final String VIDEO_PUBLISH 						= "com.ccp.phone.video_publish";
		
		/**
		 * 创建者退出是否自动解散
		 */
		public static final String CCP_FLAGE 							= "com.ccp.phone.flag";
		
		/**
		 * 会议无人是否自动解散
		 */
		public static final String CCP_FLAGE_AUTODELETE						= "com.ccp.phone.flag.isAutoDelete";
		
		/**
		 * voiceMod 0没有提示音有背景音，、1全部提示音、2无提示音无背景音。默认值为1全部提示音
		 */
		public static final String CCP_VOICEMOD						= "com.ccp.phone.voiceMod";
		public static final String CCP_CONF_OPTIONS = "com.ccp.phone.conf_options";
		
	}
}
