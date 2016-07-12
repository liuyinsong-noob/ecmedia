
/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.cloopen.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */package com.voice.demo.videoconference;

import java.util.ArrayList;
import java.util.List;

import android.app.ProgressDialog;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.listener.OnVideoConferenceListener;
import com.hisun.phone.core.voice.model.CloopenReason;
import com.hisun.phone.core.voice.model.videoconference.VideoConference;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceList;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMember;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMemberList;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoPartnerPortrait;
import com.hisun.phone.core.voice.model.videoconference.VideoPartnerPortraitList;
import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.R;
import com.voice.demo.group.GroupBaseActivity;
import com.voice.demo.tools.net.ITask;
import com.voice.demo.tools.net.ThreadPoolManager;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.ui.CCPHelper;

/**
 * 
* <p>Title: VideoConfBaseActivity.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.cloopen.com</p>
* @author Jorstin Chan
* @date 2013-10-28
* @version 3.5
 */
public abstract class VideoconferenceBaseActivity extends CCPBaseActivity implements OnVideoConferenceListener
																				,ThreadPoolManager.OnTaskDoingLinstener{

	/**
     * defined message code so that the recipient can identify 
     * what this message is about. Each {@link Handler} has its own name-space
     * for message codes, so you do not need to worry about yours conflicting
     * with other handlers.
     */
	public static final int KEY_VIDEO_RECEIVE_MESSAGE 				= 0x1;
	
	/**
	 * defined message code for Receive Video Conference Message that you create
	 * or Join.
	 */
	public static final int KEY_VIDEO_CONFERENCE_STATE 				= 0x2;
	
	/**
	 * defined message code for query members
	 */
	public static final int KEY_VIDEO_CONFERENCE_MEMBERS 			= 0x3;
	
	/**
	 * defined message code for query Video Conference list.
	 */
	public static final int KEY_VIDEO_CONFERENCE_LIST 				= 0x4;
	
	/**
	 * defined message code for invite member join Video Conference.
	 */
	public static final int KEY_VIDEO_CONFERENCE_INVITE_MEMBER 		= 0x5;
	
	/**
	 * defined message code for dismiss a Video Conference.
	 */
	public static final int KEY_VIDEO_CONFERENCE_DISMISS 			= 0x6;
	
	/**
	 * defined message code for remove member from a Video Conference.
	 */
	public static final int KEY_VIDEO_REMOVE_MEMBER 				= 0x7;
	
	
	/**
	 * defined message code for download video conference member portrait.
	 */
	public static final int KEY_VIDEO_DOWNLOAD_PORTRAIT 			= 0x8;
	
	/**
	 * defined message code for query video conference member portrait list.
	 */
	public static final int KEY_VIDEO_GET_PORPRTAIT 				= 0x9;
	
	/**
	 * defined message code for get local porprtait of Video Conference.
	 * @deprecated
	 */
	public static final int KEY_DELIVER_VIDEO_FRAME                 = 0x10;
	
	/**
	 * defined message code for Sswitch main screen of Video Conference.
	 */
	public static final int KEY_SWITCH_VIDEO_SCREEN                 = 0x11;
	
	/**
	 * defined message code for do something in background
	 * @see ThreadPoolManager#addTask(ITask)
	 */
	public static final int KEY_TASK_DOWNLOAD_PORPRTAIT             = 0x12;
	
	/**
	 * defined message code for init members to VideoUI
	 * @see ThreadPoolManager#addTask(ITask)
	 */
	public static final int KEY_TASK_INIT_VIDEOUI_MEMBERS            = 0x13;
	
	/**
	 * defined message code for Sswitch main screen of Video Conference.
	 */
	public static final int KEY_SEND_LOCAL_PORTRAIT                 = 0x14;
	
	private ProgressDialog pVideoDialog = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		if(checkeDeviceHelper()) {
			getDeviceHelper().setOnVideoConferenceListener(this);
		}
	}
	
	@Override
	protected void onStart() {
		super.onStart();
	}
	
	@Override
	protected void onPause() {
		super.onPause();
	}
	
	private final android.os.Handler handler = new android.os.Handler() {
		
		public void handleMessage(Message msg) {
			
			if(msg.what == GroupBaseActivity.WHAT_SHOW_PROGRESS) {
				pVideoDialog = new ProgressDialog(VideoconferenceBaseActivity.this);
				pVideoDialog.setTitle(R.string.str_dialog_title);
				pVideoDialog.setMessage(getString(R.string.str_dialog_message_default));
				pVideoDialog.setCanceledOnTouchOutside(false);
				String message = (String) msg.obj;
				if(!TextUtils.isEmpty(message))
					pVideoDialog.setMessage(message);
				pVideoDialog.show();
				Log4Util.d(CCPHelper.DEMO_TAG, "dialog  show");
			} else if (msg.what == GroupBaseActivity.WHAT_CLOSE_PROGRESS) {
				if(pVideoDialog != null ) {
					pVideoDialog.dismiss();
					pVideoDialog = null;
				}
				Log4Util.d(CCPHelper.DEMO_TAG, "dialog  dismiss");
			} else {
				Bundle b = (Bundle) msg.obj;
				int what = msg.arg1;
				
				int reason = -1;
				String conferenceId = "";
				if(b.containsKey(Device.REASON)) {
					reason = b.getInt(Device.REASON);
				}
				if(b.containsKey(Device.REASON)) {
					conferenceId = b.getString(Device.CONFERENCE_ID);
				}
				
				switch (what) {
				case KEY_VIDEO_RECEIVE_MESSAGE:
					VideoConferenceMsg vConferenceMsg = (VideoConferenceMsg) b.getSerializable("VideoConferenceMsg");
					handleReceiveVideoConferenceMsg(vConferenceMsg);
					break;
				case KEY_VIDEO_CONFERENCE_STATE:
					
					handleVideoConferenceState(reason, conferenceId);
					break;
				case KEY_VIDEO_CONFERENCE_MEMBERS:
					
					VideoConferenceMemberList videoMembers = (VideoConferenceMemberList) b.getSerializable("members");
					handleVideoConferenceMembers(reason, videoMembers.videoConferenceMembers);
					break;
				case KEY_VIDEO_CONFERENCE_LIST:
					
					VideoConferenceList videoCongferences = (VideoConferenceList) b.getSerializable("conferences");
					handleVideoConferences(reason, videoCongferences.videoConferences);
					break;
				case KEY_VIDEO_CONFERENCE_INVITE_MEMBER:
					
					handleVideoConferenceInviteMembers(reason, conferenceId);
					break;
				case KEY_VIDEO_CONFERENCE_DISMISS:
					
					handleVideoConferenceDismiss(reason, conferenceId);
					
					break;
				case KEY_VIDEO_REMOVE_MEMBER:
					
					String member = b.getString("member");
					handleVideoConferenceRemoveMember(reason, member);
					break;
					
				case KEY_VIDEO_GET_PORPRTAIT:
					
					VideoPartnerPortraitList vPortraitLists = (VideoPartnerPortraitList) b.getSerializable("vPortraitList");
					handleGetPortraitsFromVideoConference(reason, vPortraitLists.videoPartnerPortraits);
					break;
					
				case KEY_VIDEO_DOWNLOAD_PORTRAIT:
					
					VideoPartnerPortrait vPortrait = (VideoPartnerPortrait) b.getSerializable("portrait");
					handleDownloadVideoConferencePortraits(reason, vPortrait);
					break;
				case KEY_SWITCH_VIDEO_SCREEN:
					
					handleSwitchRealScreenToVoip(reason);
					
					break;
				case KEY_SEND_LOCAL_PORTRAIT:
					
					handleSendLocalPortrait(reason, conferenceId);
					
					break;
				default:
					break;
				}
				
			}
		}
		
	};
	
	
	/**
	 * when sub class can't show progress, then you can override it.
	 */
	public void showConnectionProgress(String messageContent) {
		Message message = Message.obtain();
		message.obj = messageContent;
		message.what = GroupBaseActivity.WHAT_SHOW_PROGRESS;
		handler.sendMessage(message);
	}

	public void closeConnectionProgress() {
		handler.sendEmptyMessage(GroupBaseActivity.WHAT_CLOSE_PROGRESS);
	}
	
	/**
	 * @return the handler
	 */
	public android.os.Handler getHandler() {
		return handler;
	}
	
	
	public final Message getHandleMessage() {
		// For each start request, send a message to start a job and deliver the
		// start ID so we know which request we're stopping when we finish the
		// job
		Message msg = getHandler().obtainMessage();
		return msg;
	}

	public final void sendHandleMessage(Message msg) {
		getHandler().sendMessage(msg);
	}
	

	@Override
	public void onReceiveVideoConferenceMsg(VideoConferenceMsg VideoMsg) {
		Log4Util.d(CCPHelper.DEMO_TAG, "Receive Video Conference message " + VideoMsg);
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("VideoConferenceMsg", VideoMsg);
	    msg.obj = b;
	    msg.arg1 = KEY_VIDEO_RECEIVE_MESSAGE;
	    sendHandleMessage(msg);
	}

	@Override
	public void onVideoConferenceState(CloopenReason reason, String conferenceId) {
		Log4Util.d(CCPHelper.DEMO_TAG, "Receive Video Conference state " + reason + " , conferenceId " + conferenceId);
		showRequestErrorToast(reason);
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putInt(Device.REASON, reason.getReasonCode());
	    b.putString(Device.CONFERENCE_ID, conferenceId);
	    msg.obj = b;
	    msg.arg1 = KEY_VIDEO_CONFERENCE_STATE;
	    sendHandleMessage(msg);
	}

	@Override
	public void onVideoConferenceMembers(CloopenReason reason,
			List<VideoConferenceMember> members) {
		Log4Util.d(CCPHelper.DEMO_TAG, "Video Conference Members reason " + reason + " , members " + members);
		showRequestErrorToast(reason);
		VideoConferenceMemberList conferenceMemberList = new VideoConferenceMemberList();
		conferenceMemberList.videoConferenceMembers = (ArrayList<VideoConferenceMember>) members;
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putInt(Device.REASON, reason.getReasonCode());
	    b.putSerializable("members", conferenceMemberList);
	    msg.obj = b;
	    msg.arg1 = KEY_VIDEO_CONFERENCE_MEMBERS;
	    sendHandleMessage(msg);
		
	}

	@Override
	public void onVideoConferences(CloopenReason reason, List<VideoConference> conferences) {
		VideoConferenceList conferencesList = new VideoConferenceList();
		Log4Util.d(CCPHelper.DEMO_TAG, "Video Conference reason " + reason );
		showRequestErrorToast(reason);
		conferencesList.videoConferences = (ArrayList<VideoConference>) conferences;
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putInt(Device.REASON, reason.getReasonCode());
	    b.putSerializable("conferences", conferencesList);
	    msg.obj = b;
	    msg.arg1 = KEY_VIDEO_CONFERENCE_LIST;
	    sendHandleMessage(msg);
	}

	@Deprecated
	public void onVideoConferenceInviteMembers(CloopenReason reason, String conferenceId) {
		
		Log4Util.d(CCPHelper.DEMO_TAG, "Video Conference Invite Members reason " + reason  + " , conferenceId " + conferenceId);
		showRequestErrorToast(reason);
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putInt(Device.REASON, reason.getReasonCode());
	    b.putString(Device.CONFERENCE_ID, conferenceId);
	    msg.obj = b;
	    msg.arg1 = KEY_VIDEO_CONFERENCE_INVITE_MEMBER;
	    sendHandleMessage(msg);
	}

	@Override
	public void onVideoConferenceDismiss(CloopenReason reason, String conferenceId) {
		
		Log4Util.d(CCPHelper.DEMO_TAG, "Video Conference Dismiss reason " + reason  + " , conferenceId " + conferenceId);
		showRequestErrorToast(reason);
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putInt(Device.REASON, reason.getReasonCode());
	    b.putString(Device.CONFERENCE_ID, conferenceId);
	    msg.obj = b;
	    msg.arg1 = KEY_VIDEO_CONFERENCE_DISMISS;
	    sendHandleMessage(msg);
	}

	@Override
	public void onVideoConferenceRemoveMember(CloopenReason reason, String member) {
		Log4Util.d(CCPHelper.DEMO_TAG, "Video Conference Remove Member reason " + reason  + " , conferenceId " + member);
		showRequestErrorToast(reason);
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putInt(Device.REASON, reason.getReasonCode());
	    b.putString("member", member);
	    msg.obj = b;
	    msg.arg1 = KEY_VIDEO_REMOVE_MEMBER;
	    sendHandleMessage(msg);
	}
	
	@Override
	public void onDownloadVideoConferencePortraits(CloopenReason reason,
			VideoPartnerPortrait portrait) {

		Log4Util.d(CCPHelper.DEMO_TAG, "Video Conference download reason " + reason  + " , filePath " + portrait.getFileName());
		showRequestErrorToast(reason);
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putInt(Device.REASON, reason.getReasonCode());
	    b.putSerializable("portrait", portrait);
	    msg.obj = b;
	    msg.arg1 = KEY_VIDEO_DOWNLOAD_PORTRAIT;
	    sendHandleMessage(msg);
	}
	
	@Override
	public void onGetPortraitsFromVideoConference(CloopenReason reason,
			List<VideoPartnerPortrait> videoPortraits) {
		
		Log4Util.d(CCPHelper.DEMO_TAG, "Video Conference get porprtait reason " + reason );
		showRequestErrorToast(reason);
		VideoPartnerPortraitList vPortraitList = new VideoPartnerPortraitList();
		vPortraitList.videoPartnerPortraits = (ArrayList<VideoPartnerPortrait>) videoPortraits;
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putInt(Device.REASON, reason.getReasonCode());
	    b.putSerializable("vPortraitList", vPortraitList);
	    msg.obj = b;
	    msg.arg1 = KEY_VIDEO_GET_PORPRTAIT;
	    sendHandleMessage(msg);
	}
	
	@Override
	public void onSwitchRealScreenToVoip(CloopenReason reason) {
		showRequestErrorToast(reason);
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putInt(Device.REASON, reason.getReasonCode());
	    msg.obj = b;
	    msg.arg1 = KEY_SWITCH_VIDEO_SCREEN;
	    sendHandleMessage(msg);
		
	}
	
	@Override
	public void onSendLocalPortrait(CloopenReason reason, String conferenceId) {
		showRequestErrorToast(reason);
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putInt(Device.REASON, reason.getReasonCode());
	    b.putString(Device.CONFERENCE_ID, conferenceId);
	    msg.obj = b;
	    msg.arg1 = KEY_SEND_LOCAL_PORTRAIT;
	    sendHandleMessage(msg);		
	}
	
	
	/**
	 * 
	* <p>Title: handleReceiveVideoConferenceMsg</p>
	* <p>Description: SIP message callback, when the application receives 
	* the message of the video conference then trigger this method.
	* {@link VideoconferenceBaseActivity#KEY_VIDEO_RECEIVE_MESSAGE} , and 
	* {@link VideoconferenceBaseActivity#onReceiveVideoConferenceMsg(VideoConferenceMsg)}
	* 
	* The application can realize their own the code to handle the message
	* The method runs in the main thread</p>
	* @param VideoMsg
	* 
	* @see #onReceiveVideoConferenceMsg(VideoConferenceMsg)
	* @see #KEY_VIDEO_RECEIVE_MESSAGE
	 */
	protected void handleReceiveVideoConferenceMsg(VideoConferenceMsg VideoMsg) { }
	
	/**
	 * 
	* <p>Title: handleVideoConferenceState</p>
	* <p>Description: The method trigger by the application calls the video conference interface
	* For example: When the application calls for creating video conferencing interface or 
	* calls join into the video interface, SDK will be Execution the callback method notification 
	* application processing results.
	* The application can realize their own the code to handle the message
	*  
	* The method runs in the main thread</p>
	* @param reason Return 0 if successful ,Others return error code
	* @param conferenceId
	* 
	* @see #onVideoConferenceState(int, String)
	* @see #KEY_VIDEO_CONFERENCE_STATE
	 */
	protected void handleVideoConferenceState(int reason, String conferenceId) { }
	
	
	/**
	 * 
	* <p>Title: handleVideoConferenceMembers</p>
	* <p>Description: When the application calls the interface for query member of the video conference 
	* SDK will be Execution the callback method notification ,application processing results.
	* The application can realize their own the code to handle the message
	* 
	* The method runs in the main thread</p>
	* @param reason Return 0 if successful ,Others return error code
	* @param members
	* 
	* @see #onVideoConferenceMembers(int, List)
	* @see #KEY_VIDEO_CONFERENCE_MEMBERS
	 */
	protected void handleVideoConferenceMembers(int reason, List<VideoConferenceMember> members) { }
	
	/**
	 * 
	* <p>Title: handleVideoConferences</p>
	* <p>Description: When the application calls the interface for query list of the video conference 
	* SDK will be Execution the callback method notification ,application processing results.
	* The application can realize their own the code to handle the message
	* 
	* The method runs in the main thread</p>
	* @param reason Return 0 if successful ,Others return error code
	* @param conferences
	* 
	* @see #onVideoConferences(int, List)
	* @see #KEY_VIDEO_CONFERENCE_LIST
	 */
	protected void handleVideoConferences(int reason, List<VideoConference> conferences) { }
	
	/**
	 * 
	* <p>Title: handleVideoConferenceInviteMembers</p>
	* <p>Description: When the application calls the interface for invite member join the 
	* existing video conference , SDK will be Execution the callback method notification ,
	* application processing results.
	* The application can realize their own the code to handle the message
	* 
	* The method runs in the main thread</p>
	* @param reason Return 0 if successful ,Others return error code
	* @param conferenceId
	* 
	* @see #onVideoConferenceInviteMembers(int, String)
	* @see #KEY_VIDEO_CONFERENCE_INVITE_MEMBER
	* 
	* @deprecated
	 */
	protected void handleVideoConferenceInviteMembers(int reason, String conferenceId) { }
	
	/**
	 * 
	* <p>Title: handleVideoConferenceDismiss</p>
	* <p>Description: When the creator of the Video Conference calls the interface to dismiss
	* existing video conference , SDK will be Execution the callback method notification Interface caller,
	* that processing results.However, video conference member will be notified video conference was 
	* dismiss by SIP messages {@link #onReceiveVideoConferenceMsg(VideoConferenceMsg)}
	* The application can realize their own the code to handle the message
	* 
	* The method runs in the main thread</p>
	* @param reason Return 0 if successful ,Others return error code
	* @param conferenceId
	* 
	* @see #onReceiveVideoConferenceMsg(VideoConferenceMsg)
	* @see #onVideoConferenceDismiss(int, String)
	* @see #KEY_VIDEO_CONFERENCE_DISMISS
	 */
	protected void handleVideoConferenceDismiss(int reason, String conferenceId) { }
	
	/**
	 * 
	* <p>Title: handleVideoConferenceRemoveMember</p>
	* <p>Description: When the creator of the Video Conference calls the interface to remove a member
	* from existing video conference ,SDK will be Execution the callback method notification Interface caller,
	* that processing results.However, video conference member will be notified that one member ha's be remove
	* by SIP messages
	* The application can realize their own the code to handle the message
	*  
	* The method runs in the main thread</p>
	* @param reason Return 0 if successful ,Others return error code
	* @param member
	* 
	* @see #onReceiveVideoConferenceMsg(VideoConferenceMsg)
	* @see #onVideoConferenceRemoveMember(int, String)
	* @see #KEY_VIDEO_REMOVE_MEMBER
	 */
	protected void handleVideoConferenceRemoveMember(int reason, String member) { }
	
	
	/**
	 * 
	* <p>Title: handleDownloadVideoConferencePortraits</p>
	* <p>Description: When the caller of download the portrait of voip from existing video conference
	* SDK will be Execution the callback method notification Interface caller,
	* that processing results.However, video conference member will be notified that this file is download
	* by SIP messages
	* The application can realize their own the code to handle the message
	* 
	* The method runs in the main thread</p>
	* @param reason Return 0 if successful ,Others return error code
	* @param portrait
	* 
	* @see #onDownloadVideoConferencePortraits(int, VideoPartnerPortrait)
	* @see Device#executeCCPDownload(ArrayList)
	* @see #KEY_VIDEO_DOWNLOAD_PORTRAIT
	 */
	protected void handleDownloadVideoConferencePortraits(int reason, VideoPartnerPortrait portrait){}
	
	/**
	 * 
	* <p>Title: handleGetPortraitsFromVideoConference</p>
	* <p>Description: When the caller of query the portrait of voip from existing video conference
	* SDK will be Execution the callback method notification Interface caller,
	* that processing results.However, video conference member will be notified that this list of porprtait
	* by SIP messages
	* The application can realize their own the code to handle the message
	* 
	* The method runs in the main thread</p>
	* @param reason Return 0 if successful ,Others return error code
	* @param videoPortraits 
	* 
	* @see #onGetPortraitsFromVideoConference(int, List)
	* @see Device#getPortraitsFromVideoConference(String)
	* @see #KEY_VIDEO_GET_PORPRTAIT
	 */
	protected void handleGetPortraitsFromVideoConference(int reason, List<VideoPartnerPortrait> videoPortraits) {}
	
	/**
	 * 
	* <p>Title: handleSwitchRealScreenToVoip</p>
	* <p>Description: When the caller of switch main screen from existing video conference
	* SDK will be Execution the callback method notification Interface caller,
	* that processing results.However, video conference member will be notified that the switch result 
	* by SIP messages
	* 
	* The application can realize their own the code to handle the message
	* 
	* The method runs in the main thread</p>
	* @param reason
	 */
	protected void handleSwitchRealScreenToVoip(int reason) {}
	
	/**
	 * 
	* <p>Title: handleSendLocalPortrait</p>
	* <p>Description: When the caller of send the portrait of voip from existing video conference
	* SDK will be Execution the callback method notification Interface caller,
	* that processing results.However, video conference member will be notified that the Video Frame send result 
	* of porprtait by SIP messages
	* 
	* The application can realize their own the code to handle the message
	* 
	* The method runs in the main thread</p>
	* @param reason
	* @param conferenceId
	 */
	protected void handleSendLocalPortrait(int reason, String conferenceId){};
	
	
	
	
	public void setVideoTitleBackground() {
		findViewById(R.id.nav_title).setBackgroundResource(R.drawable.video_title_bg);
	}
	
	@Deprecated
	public void setVideoBackSelector() {
		findViewById(R.id.voice_btn_back).setBackgroundResource(R.drawable.video_back_button_selector);
		findViewById(R.id.voice_btn_back).setVisibility(View.VISIBLE);
	}
	
	public void setVideoTitle(String title) {
		((TextView)findViewById(R.id.title)).setText(title);
	}

	
	
	@Override
	public void onRequestConferenceMemberVideoFailed(final int reason,
			String conferenceId, final String voip) {
		getHandler().post(new Runnable() {
			
			@Override
			public void run() {
				Toast.makeText(VideoconferenceBaseActivity.this, "请求成员[" + voip + "]视频数据失败（错误码：" + reason+ "）", Toast.LENGTH_LONG).show();
			}
		});
		
	}
	
	@Override
	public void onCancelConferenceMemberVideo(final int reason, String conferenceId,
			final String voip) {
		getHandler().post(new Runnable() {
			
			@Override
			public void run() {
				if(reason == 0) {
					Toast.makeText(VideoconferenceBaseActivity.this, "取消成员[" + voip + "]视频数据成功", Toast.LENGTH_LONG).show();
					return ;
				}
				Toast.makeText(VideoconferenceBaseActivity.this, "取消成员[" + voip + "]视频数据失败（错误码：" + reason+ "）", Toast.LENGTH_LONG).show();
			}
		});
	}
	
	@Override
	public void onVideoRatioChanged(String voip, int width, int height) {
		
	}
}