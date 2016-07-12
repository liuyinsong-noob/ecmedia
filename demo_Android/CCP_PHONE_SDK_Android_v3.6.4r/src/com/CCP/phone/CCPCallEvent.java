package com.CCP.phone;

import java.util.List;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.Device.CallType;
import com.hisun.phone.core.voice.Device.Codec;
import com.hisun.phone.core.voice.model.CloopenReason;
import com.hisun.phone.core.voice.model.chatroom.Chatroom;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMember;
import com.hisun.phone.core.voice.model.im.InstanceMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneMember;
import com.hisun.phone.core.voice.model.videoconference.VideoConference;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMember;
import com.hisun.phone.core.voice.model.videoconference.VideoPartnerPortrait;

/**
 * protected interface
 */
public interface CCPCallEvent {
	
	// transfer params to another user by this key
	static final int USERDATA_FOR_TOKEN = 0;
	static final int USERDATA_FOR_USER_AGENT = 1;
	static final int USERDATA_FOR_INVITE = 2;
	
	void onConnected();
	void onConnectError(int reason);
	void onDisconnect();
	void onCallAlerting(String callid);
	void onCallProceeding(String callid);
	void onCallAnswered(String callid);
	void onMakeCallFailed(String callid, int reason);
	void onCallPaused(String callid);
	void onCallPausedByRemote(String callid);
	void onCallReleased(String callid);
	void onCallTransfered(String callid, String destionation);
	void onIncomingCallReceived(CallType type, String callid, String caller);
	void onDtmfReceived(String callid, char dtmf);
	
	void onTextMessageReceived(String sender, String message);
	@Deprecated
	void onMessageSendReport(String msgid, int status);
	void onMessageSendReport(String msgid,String date,  int status);
	
	void onCallBack(int status, String self, String dest);
	
	void onCallMediaUpdateRequest(String callid, int reason);
	void onCallMediaUpdateResponse(String callid, int reason);
	
	void onCallVideoRatioChanged(String callid, String resolution , int state);
	
	void onCallMediaInitFailed(String callid, int reason);
	
	/**********************************************************************
	 *                     voice message                                  *
	 **********************************************************************/
	void onRecordingTimeOut(long mills);
	void onRecordingAmplitude(double amplitude);
	void onFinishedPlaying();
	
	
	/**********************************************************************
	 *                         Interphone                                 *
	 **********************************************************************/
	
	public abstract void onInterphoneState(CloopenReason reason, String confNo);
	public abstract void onControlMicState(CloopenReason reason, String speaker);
	public abstract void onReleaseMicState(CloopenReason reason);
	public abstract void onInterphoneMembers(CloopenReason reason, List<InterphoneMember> member);
	public abstract void onPushMessageArrived(String body);
	
	/**********************************************************************
	 *                         ChatRoom                                 *
	 **********************************************************************/
	public abstract void onChatRoomState(CloopenReason reason, String confNo);
	public abstract void onChatRoomMembers(CloopenReason reason, List<ChatroomMember> member);
	public abstract void setCodecEnabled(Codec codec, boolean enabled);
	public abstract void onChatRooms(CloopenReason reason,List<Chatroom> chatRoomList);
	public abstract void onChatRoomInvite(CloopenReason reason, String confNo);
	public abstract void onChatRoomDismiss(CloopenReason reason, String roomNo);
	public abstract void onChatRoomRemoveMember(CloopenReason reason, String member);
	public abstract void onSetChatroomSpeakOpt(CloopenReason reason, String member);
	
	
	/**********************************************************************
	 *                         IM                                         *
	 **********************************************************************/
	public abstract void onSendInstanceMessage(CloopenReason reason, InstanceMsg data);
	public abstract void onDownloadAttached(CloopenReason reason, String fileName);
	public abstract void onReceiveInstanceMessage(InstanceMsg msg);
	
	public abstract void onConfirmIntanceMessage(CloopenReason reason);
	
	/**********************************************************************
	 *                         VideConference                              *
	 **********************************************************************/
	public abstract void onVideoConferenceState(CloopenReason reason , String conferenceId);
	public abstract void onVideoConferenceMembers(CloopenReason reason , List<VideoConferenceMember> members);
	public abstract void onVideoConferences(CloopenReason reason , List<VideoConference> conferences);
	public abstract void onVideoConferenceInvite(CloopenReason reason , String conferenceId);
	public abstract void onVideoConferenceDismiss(CloopenReason reason , String conferenceId);
	public abstract void onVideoConferenceRemoveMember(CloopenReason reason , String member);
	/**
	 * 
	* <p>Title: onDownloadVideoConferencePortraits</p>
	* <p>Description: Video conference image download the callback method
	* </p>
	* @param reason State value, returns 0, 
	* 		 other according to the value can know the current error information
	* @param portrait Download information entity, Including the head of a picture, save path.
	 */
	public abstract void onDownloadVideoConferencePortraits(CloopenReason reason , VideoPartnerPortrait portrait);
	/**
	 * 
	* <p>Title: onGetPortraitsFromVideoConference</p>
	* <p>Description: When the application calls the interface for query list porprait of the video conference 
	* {@link Device#getPortraitsFromVideoConference(String)}
	* SDK will be Execution the callback method notification ,application processing results.
	* 
	* The application can realize their own the code to handle the message
	* The method runs in the non main thread</p>
	* @param reason State value, returns 0, 
	* 		 other according to the value can know the current error information
	* @param conferences 
	* 
	* @see Device#getPortraitsFromVideoConference(String)
	* 
	 */
	public abstract void onGetPortraitsFromVideoConference(CloopenReason reason , List<VideoPartnerPortrait> videoPortraits);
	
	
	/**
	 * 
	* <p>Title: onSwitchRealScreenToVoip</p>
	* <p>Description: </p>
	* @param reason
	 */
	public abstract void onSwitchRealScreenToVoip(CloopenReason reason);
	
	/**
	 * 
	* <p>Title: onSendLocalPortrait</p>
	* <p>Description: </p>
	* @param reason
	* @param conferenceId
	 */
	public abstract void onSendLocalPortrait(CloopenReason reason , String conferenceId);
	
	// ----------------------------------------------
	// p2p set callback
	public abstract void onFirewallPolicyEnabled();
	
	/**
	 * The callback of calls recording ,Call recording ends or an error, reporting events.
	 * @param callid
	 * @param fileName is the top pass under the file name
	 * @param reason recording status: 
	 * 		  0: Success 
	 *        -1: Recording failed then delete the recording file 
	 *        -2: Recording failed of write file but still to retain the record file.
	 *        
	 * @version 3.6
	 */
	public abstract void onCallRecord(String callid , String fileName , int reason);
	
	
	/**
	 * Called when a flag has be set by @setProcessDataEnabled 
	 * 
	 * @param b The audio data
	 * @param transDirection Transmitting or receiving the data transfer direction
	 * @return  The audio data after the operation (Example: Encryption) 
	 */
	public abstract byte[] onCallProcessData(byte[] b , int transDirection);
	public abstract byte[] onProcessOriginalData(byte[] b);
	
	public abstract void onTransferStateSucceed(String callid , boolean result);
	public abstract void OnTriggerSrtp(String callid , boolean caller);
	
	public abstract void OnPublishVideoFrameRequest(int type , CloopenReason reason);
	
	public abstract void onRequestConferenceMemberVideoFailed(int reason , String conferenceId , String voip);
	public abstract void onCancelConferenceMemberVideo(int reason , String conferenceId , String voip );
}
