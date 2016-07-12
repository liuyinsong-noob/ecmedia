package com.hisun.phone.core.voice.listener;

import java.util.List;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.CloopenReason;
import com.hisun.phone.core.voice.model.videoconference.VideoConference;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMember;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoPartnerPortrait;

/**
 * 
* <p>Title: OnVideoConferenceListener.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.cloopen.com</p>
* @author Jorstin Chan
* @date 2013-10-25
* @version 3.5
 */
public interface OnVideoConferenceListener {

	/**
	 * 收到新的视频会议消息（有人加入、有人退出、有人被移除、有人被设置为主频）
	 * @param VideoMsg 视频会议消息具体类型
	 * 
	 * @see Device#startVideoConference(String, String, int, String, String)
	 * @see Device#joinVideoConference(String)
	 * @see Device#dismissVideoConference(String, String)
	 * @see Device#removeMemberFromVideoConference(String, String, String)
	 */
	public abstract void onReceiveVideoConferenceMsg(VideoConferenceMsg  VideoMsg);

	/**
	 * 创建或者加入视频会议回调接口
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param conferenceId 视频会议房间号
	 * 
	 * @see Device#startVideoConference(String, String, int, String, String)
	 * @see Device#joinVideoConference(String)
	 */
	public abstract void onVideoConferenceState(CloopenReason reason , String conferenceId) ;
	
	/**
	 * 查询视频会议成员回调接口
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param members 视频会议成员列表
	 * 
	 * @see Device#queryVideoConferences(String, String)
	 */
	public abstract void onVideoConferenceMembers(CloopenReason reason , List<VideoConferenceMember> members) ;
	
	/**
	 * 查询视频会议列表回调接口
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param conferences 视频会议列表
	 * 
	 * @see Device#queryVideoConferences(String, String)
	 */
	public abstract void onVideoConferences(CloopenReason reason , List<VideoConference> conferences) ;
	
	/**
	 * 解散视频会议回调接口，通知解散接口调用者解散结果
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param conferenceId 视频会议房间号
	 * 
	 * @see Device#dismissVideoConference(String, String)
	 * @see #onReceiveVideoConferenceMsg(VideoConferenceMsg)
	 */
	public abstract void onVideoConferenceDismiss(CloopenReason reason , String conferenceId) ;
	
	/**
	 * 移除视频会议成员回调接口，通知接口调用方移除结果，
	 * 其他视频会议成员在移除成功后将收到某成员被移除的事件通知 {@link #onReceiveVideoConferenceMsg(VideoConferenceMsg)}
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param member 被移除的成员账号
	 * 
	 * @see Device#removeMemberFromVideoConference(String, String, String)
	 * @see #onReceiveVideoConferenceMsg(VideoConferenceMsg)
	 */
	@Deprecated public abstract void onVideoConferenceRemoveMember(CloopenReason reason , String member) ;
	
	
	/**
	 * 下载视频会议成员头像回调接口，参数代表当前下载成功的视频会议成员的信息
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param portrait 当前下载成功的视频会议成员的信息
	 * 
	 * @see Device#downloadVideoConferencePortraits(java.util.ArrayList)
	 */ 
	@Deprecated public abstract void onDownloadVideoConferencePortraits(CloopenReason reason , VideoPartnerPortrait portrait);
	
	
	/**
	 * 获取当前所有与会成员的头像下载地址
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param videoPortraits 头像下载参数信息
	 * 
	 * @see Device#getPortraitsFromVideoConference(String)
	 */
	@Deprecated public abstract void onGetPortraitsFromVideoConference(CloopenReason reason , List<VideoPartnerPortrait> videoPortraits);
	
	/**
	 * 设置某个成员为主屏会带接口，该接口有权限限制，仅限于会议创建这调用 
	 * 切换成功后，其他与会成员将收到主频被切换事件通知 {@link #onReceiveVideoConferenceMsg(VideoConferenceMsg)}
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * 
	 * @see Device#switchRealScreenToVoip(String, String, String)
	 * @see #onReceiveVideoConferenceMsg(VideoConferenceMsg)
	 */
	@Deprecated public abstract void onSwitchRealScreenToVoip(CloopenReason reason);
	
	/**
	 * 上传本地视频头像，其他与会成员可以通过接口{@link Device#getPortraitsFromVideoConference(String)}
	 * 获得与会成员的头像信息
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param conferenceId 视频会议房间号
	 */
	@Deprecated public abstract void onSendLocalPortrait(CloopenReason reason , String conferenceId);
	public abstract void onRequestConferenceMemberVideoFailed(int reason , String conferenceId , String voip);
	public abstract void onCancelConferenceMemberVideo(int reason , String conferenceId , String voip );
	public abstract void onVideoRatioChanged(String voip, int width , int height);
}
