/**
 * 
 */
package com.hisun.phone.core.voice;

import java.util.ArrayList;

import android.app.Activity;
import android.app.PendingIntent;
import android.os.Parcelable;
import android.view.SurfaceView;
import android.view.View;

import com.CCP.phone.CameraInfo;
import com.CCP.phone.VideoSnapshot;
import com.hisun.phone.core.voice.CCPCall.InitListener;
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
import com.hisun.phone.core.voice.model.chatroom.ChatroomMsg;
import com.hisun.phone.core.voice.model.im.InstanceMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoPartnerPortrait;
import com.hisun.phone.core.voice.util.ObjectIdentifier;
import com.hisun.phone.core.voice.util.ObjectStringIdentifier;

/**
 * This class defined lots of methods for CCP capability.
 * 
 */
public abstract class Device implements Parcelable {

	public static final String TAG = "SDK_DEVICE";

	public static final String DEVICES_ARRAY = "com.ccp.phone.devices";
	public static final String CALLTYPE = "com.ccp.phone.calltype";
	public static final String CALLID = "com.ccp.phone.callid";
	public static final String CALLER = "com.ccp.phone.caller";
	public static final String REMOTE = "com.ccp.phone.remote";
	public static final String DESTIONATION = "com.ccp.phone.destionation";
	public static final String REASON = "com.ccp.phone.reason";
	public static final String SENDER = "com.ccp.phone.sender";
	public static final String MESSAGE = "com.ccp.phone.message";
	public static final String APN = "com.ccp.phone.apn";
	public static final String NS = "com.ccp.phone.ns";
	public static final String MSGID = "com.ccp.phone.msgid";
	public static final String SELFPHONE = "com.ccp.phone.selfphone";
	public static final String DESTPHONE = "com.ccp.phone.destphone";
	public static final String CBSTATE = "com.ccp.phone.cbstate";
	public static final String yuntongxun_REASON = "com.ccp.phone.cloopenreason";
	public static final String MEDIA_MESSAGE = "com.ccp.phone.mediamsg";
	public static final String VOICE_AMPLITUDE = "com.ccp.phone.amplitude";
	
	/**实时对讲常量定义*/
	public static final String CONFNO = "com.ccp.phone.interphone.confNo";
	public static final String MEMBERS = "com.ccp.phone.interphone.members";
	public static final String INTERPHONEMSG = "com.ccp.phone.interphone.interphoneMsg";
	public static final String SPEAKER = "com.ccp.phone.interphone.speaker";
	
	/**语音会议常量定义*/
	public static final String CHATROOM_MEMBERS = "com.ccp.phone.chatroom.members";
	public static final String CHATROOM_MSG = "com.ccp.phone.chatroom.chatroomMsg";
	public static final String CHATROOM_LIST = "com.ccp.phone.chatroom.chatroomList";
	
	/**视频会议*/
	public static final String CONFERENCE_ID = "com.ccp.phone.videoconf.conferenceId";

	/**
	 * 设置device监听,在初始化并创建Device时候会通过该接口设置的Listener回调通知创建结果
	 * @see DeviceListener#onConnected()
	 * @see DeviceListener#onDisconnect(Reason)
	 * 
	 * @param deviceListener 监听的device
	 */
	public abstract void setDeviceListener(final DeviceListener deviceListener);

	/**
	 * 设置呼入事件通知（VoIP、视频），SDK收到来电事件消息会通过设置的PendingIntent唤起来电界面
	 * 
	 * @param pendingIntent
	 */
	public abstract void setIncomingIntent(final PendingIntent pendingIntent);

	/**
	 * 设置需要显示的电话号码，在对方来电（VoIP电话或落地电话）上显示的号码.
	 * 此功能需要服务器支持.
	 * @param phoneNumber
	 */
	public abstract void setSelfPhoneNumber(final String phoneNumber);

	/**
	 * 设置显示昵称，应用与VoIP音频呼叫或者视频通话过程中，SDK将该参数传递给被叫方,
	 * 被叫方获得该参数用于区分来电者
	 * @param nickName
	 */
	public abstract void setSelfName(final String nickName);
 
	/**
	 * 设置视频通话显示的SurfaceView
	 * @param account removet account
	 * @param view  对方显示视图
	 * @param localView 本地显示视图
	 */
	public abstract void setVideoView(final String account, final SurfaceView view,
			final SurfaceView localView);

	/**
	 * 获取手机摄像头参数信息(摄像头个数，名称、以及摄像头所持有的分辨率)
	 * 
	 * @return 手机摄像头参数信息
	 */
	public abstract CameraInfo[] getCameraInfo();

	/**
	 * 切换视频通话摄像头
	 * 
	 * @param cameraIndex 手机摄像头 标识（前置摄像头或者后置摄像头），参考 {@link android.hardware.Camera.CameraInfo}
	 * @param capabilityIndex 手机摄像头所支持的分辨率集合中的index，
	 * @param fps 摄像头码率
	 * @param rotate 摄像头旋转的度数，默认为0，参数范围有（0、90、180、270）
	 * @param force 是否强制初始化摄像头,当cameraIndex和当前正在显示的摄像头一样仍然会重新初始化摄像头
	 * 
	 * @see #getCameraInfo()
	 * @version 支持SDK 版本 3.5 以及以上版本
	 */
	public abstract void selectCamera(int cameraIndex, int capabilityIndex,
			int fps, Rotate rotate , boolean force);

	/**
	 * 设置SDK支持的编解码方式，默认全部支持
	 * @param codec {@link Codec}
	 * @param enabled true支持，false不支持
	 */
	public abstract void setCodecEnabled(final Codec codec, boolean enabled);
	public abstract boolean getCodecEnabled( Codec codec);

	/**
	 * 返回通过 {@link #setDeviceListener(DeviceListener)所设置的Device监听对象}
	 * @return device监听
	 * @see #setDeviceListener(DeviceListener)
	 */
	public abstract DeviceListener getDeviceListener();

	/**
	 * 释放SDK Deviced对象,调用该接口后SDK将处于未初始化状态，
	 * 下次使用需要重新调用初始化接口和创建Device对象
	 * @see CCPCall#init(android.content.Context, InitListener)
	 * @see CCPCall#createDevice(DeviceListener, java.util.Map)
	 */
	public abstract void release();

	/**
	 * 设置音频处理的开关。在呼叫前调用
	 * @param type 音频处理类型 {@link AudioType}
	 * @param enabled 是否启用 ，AGC默认关闭; EC和NS默认开启
	 * @param mode 音频模式 {@link AudioMode}
	 * @return 0设置成功，-1则设置失败
	 */
	public abstract int setAudioConfigEnabled(AudioType type, boolean enabled,
			AudioMode mode);

	/**
	 * 获得音频处理的设置状态。
	 * @param type 音频处理类型 {@link AudioType}
	 * @return true开启，false关闭
	 */
	public abstract boolean getAudioConfig(AudioType type);

	/**
	 * 获取当前设置的音频类型所对应的处理模式，需要通过{@link #setAudioConfigEnabled(AudioType, boolean, AudioMode)}
	 * 设置音频处理状态
	 * @param type 音频处理类型 {@link AudioType}
	 * @return 音频处理模式 {@link AudioType} 
	 * @see #setAudioConfigEnabled(AudioType, boolean, AudioMode)
	 */
	public abstract AudioMode getAudioConfigMode(AudioType type);

	/**
	 * 设置视频通话码流，需要在通话前调用
	 * @param bitrates 
	 */
	public abstract void setVideoBitRates(int bitrates);

	/**
	 * 将rtp数据保存在文件中，通过该接口可对通话数据进行抓包，调用接口{@link #stopRtpDump(String, int, int)}
	 * 结束当前的rtp数据抓包。
	 * @param callid 当前通话的唯一标识
	 * @param mediaType 当前通话类型（视频通话或者音频通话）{@link CallType}
	 * @param fileName 数据保存的全路径，确保当前保存的文件所对应的路径已经存在
	 * @param direction 上行数据或者下行数据，1代表上行数据，0代表下行
	 * @return 0成功，-1失败
	 * @see #stopRtpDump(String, int, int)
	 */
	public abstract int startRtpDump(String callid, int mediaType,
			String fileName, int direction);

	/**
	 * 停止rtp数据抓包，仅仅在通话过程中有效
	 * @param callid 当前通话的唯一标识
	 * @param mediaType 当前通话类型（视频通话或者音频通话）{@link CallType}
	 * @param direction 上行数据或者下行数据，1代表上行数据，0代表下行
	 * @return 0成功，-1失败
	 */
	public abstract int stopRtpDump(String callid, int mediaType, int direction);

	/**
	 * 获取当前SDK注册状态（在线状态），其中{@link State#TIMEOUT}可以当作不在线处理
	 * @return 在线状态：在线、不在线、账号不存在、检测超时
	 * @see State
	 */
	public abstract State isOnline();

	/**
	 * 发起VoIP呼叫或者视频呼叫 ,如果呼叫失败则SDK会通过{@link OnVoIPListener#onMakeCallFailed(String, Reason)
	 * 通知呼叫失败的原因，其他则通过接口{@link OnVoIPListener#onCallProceeding(String)} 通知正在呼叫过程中以及通过
	 * 接口{@link OnVoIPListener#onCallAlerting(String)}提示对方正在振铃 
	 * 
	 * @param callType 呼叫类型：音频或者视频
	 * @param called 被叫号码
	 * @return 当前呼叫的唯一标识，返回值为null的时候可能SDK处于未注册状态
	 * @see #setOnVoIPListener(OnVoIPListener)
	 * @see OnVoIPListener#onMakeCallFailed(String, Reason)
	 * @see OnVoIPListener#onCallProceeding(String)
	 * @see OnVoIPListener#onCallAlerting(String)
	 */
	public abstract String makeCall(CallType callType, String called);

	/**
	 * 释放当前SDK正在进行通话,该接口会触发回调接口{@link OnVoIPListener#onCallReleased(String)} 
	 * @param callid 当前通话的唯一标识
	 * @see #makeCall(CallType, String)
	 * @see #setOnVoIPListener(OnVoIPListener)
	 * @see OnVoIPListener#onCallReleased(String)
	 */
	public abstract void releaseCall(String callid);

	/**
	 * 接听当前所接收到的呼叫请求，需要在SDK注册成功后设置来电PendingIntent
	 * 通话建立成功时会通过{@link OnVoIPListener#onCallAnswered(String)}通知主被叫方
	 * @param callid 当前通话的唯一标识
	 * @see #setIncomingIntent(PendingIntent)
	 * @see OnVoIPListener#onCallAnswered(String)
	 */
	public abstract void acceptCall(String callid, String account);
	
	/**
	 * 接听通话呼叫请求，可以设置接听的类型(音频或者视频)
	 * @param callid 当前通话的唯一标识
	 * @param callType 通话类型
	 * @see CallType
	 */
	public abstract void acceptCall(String callid, String account, CallType callType);

	/**
	 * 通过该接口可以拒绝一路来电呼叫
	 * @param callid 当前通话的唯一标识
	 * @param reason 拒接原因，参考值：3来电主动拒绝接听，当前并非有另外一路通话导致的线路忙
	 * 		  6当前对方正忙，当SDK收到呼叫请求并判断当前SDK状态非空闲，则主动回调该接口并
	 * 		    设置挂机原因为6提示当前正在忙
	 * @see OnVoIPListener#onMakeCallFailed(String, Reason)
	 */
	public abstract void rejectCall(String callid , int reason);
	
	@Deprecated public abstract void rejectCall(String callid);

	/**
	 * 挂起一路通话，调用{@link #resumeCall(String)} 可将当前挂机的通话恢复
	 * @param callid 当前通话的唯一标识
	 * @see #resumeCall(String)
	 * @see OnVoIPListener#onCallPaused(String)
	 * @see OnVoIPListener#onCallPausedByRemote(String)
	 */
	public abstract void pauseCall(String callid);

	/**
	 * 恢复通话
	 * @param callid 当前通话的唯一标识
	 * @see #pauseCall(String)
	 */
	public abstract void resumeCall(String callid);

	/**
	 * Transfer an VoIP call .
	 * default call type for {@link CallType#VOICE}
	 * @see #transferCall(String, String, CallType)
	 */
	public abstract int transferCall(String callid, String destination);
	
	/**
	 * Transfer an VoIP call .
	 * 
	 * @param callid Uniquely identifies the current call
	 * @param destionation The called number.
	 * @param callType {@link CallType#VOICE}
	 * @return The success of 0: successfully; 0 Non failure
	 */
	public abstract int transferCall(String callid, String destination , CallType callType);

	/**
	 * 返回代表当前正在通话的唯一标识
	 * @return 通话的唯一标识
	 */
	public abstract String getCurrentCall();

	/**
	 * @see #requestSwitchCallMediaType(String, CallType)
	 * @param callid
	 * @param callType
	 */
	@Deprecated public abstract void updateCallType(String callid, CallType callType);
	/**
	 * 切换当前的通话类型，音频切换成视频、或者视频切换成音频
	 * @param callid 通话的唯一标识
	 * @param callType 需要转换成的通话类型
	 * @see CallType
	 * @see OnVoIPListener#onCallMediaUpdateRequest(String, int)
	 */
	public abstract void requestSwitchCallMediaType(String callid, CallType callType);

	/**
	 * @see #responseSwitchCallMediaType(String, int)
	 * @param callid
	 * @param action
	 */
	@Deprecated public abstract void answerCallTypeUpdate(String callid, int action);
	/**
	 * 响应对方的切换通话类型请求
	 * @param callid 通话的唯一标识
	 * @param action 1代表同意、0代表拒绝
	 * @see OnVoIPListener#onCallMediaUpdateResponse(String, int)
	 */
	public abstract void responseSwitchCallMediaType(String callid, int action);

	/**
	 * 获取当前的通话类型，音频或者视频
	 * @param callid 通话的唯一标识
	 * @return 通话类型
	 * @see CallType
	 */
	public abstract CallType getCallType(String callid);

	/**
	 * 通话过程中发送DTMF，没有成功失败的通知事件
	 * @param callid 通话的唯一标识
	 * @param dtmf 参考值范围：'0'-'9'' * ''#'
	 */
	public abstract void sendDTMF(String callid, char dtmf);

	/**
	 * 通话过程中切换为扬声器或者听筒模式，该接口只对VoIP通话、实时对讲、语音群聊、视频会议有效
	 * @param enable true为开启扬声器模式，false为听筒模式
	 */
	public abstract void enableLoudsSpeaker(boolean enable);

	/**
	 * 获取当前设备的外放模式：扬声器或者听筒
	 * @return true为扬声器模式，false为听筒模式
	 */
	public abstract boolean getLoudsSpeakerStatus();

	/**
	 * 设置当前设备为静音状态，该接口只对VoIP通话、实时对讲、语音群聊、视频会议有效
	 * @param on true为静音，对方听不到声音，false则回复正常
	 */
	public abstract void setMute(boolean on);

	/**
	 * 返回当前设备的麦克风状态
	 * @return true为静音
	 */
	public abstract boolean getMuteStatus();

	@Deprecated public abstract void makeCallback(String src, String dest);
	/**
	 * 发起回拨通话
	 * @param src 主叫电话号码
	 * @param dest 被叫电话号码
	 * @param srcSerNum 被叫侧显示的客服号码，根据平台侧显号规则控制
	 * @param destSerNum 主叫侧显示的号码，根据平台侧显号规则控制
	 * @see OnVoIPListener#onCallback(int, String, String)
	 */
	public abstract void makeCallback(String src, String dest , String srcSerNum , String destSerNum);

	/**
	 * 获取语音文件时长
	 * @param fileName 语音文件全路径路径
	 * @return 语音文件时长，单位为毫秒
	 */
	public abstract long getVoiceDuration(String fileName);

	/**
	 * 实时对讲接口  - 创建实时对讲场景
	 * @param members 邀请参与实时对讲的成员
	 * @param appId 应用id
	 * @see OnInterphoneListener#onInterphoneState(CloopenReason, String)
	 */
	public abstract void startInterphone(String[] members, String appId);

	/**
	 * 实时对讲接口  - 加入实时对讲,如果实时对讲已经不存在则会加入失败
	 * @param confNo 实时对讲会议号
	 * @see OnInterphoneListener#onInterphoneState(CloopenReason, String)
	 */
	public abstract void joinInterphone(String confNo);

	/**
	 * 实时对讲接口  - 退出当前参与的实时对讲
	 * @return true 成功
	 */
	public abstract boolean exitInterphone();

	/**
	 * 实时对讲接口  - 发起控麦请求,如果当前实时对讲成员正在控麦中，则会返回控麦失败
	 * @param confNo 实时对讲会议号
	 * @see OnInterphoneListener#onControlMicState(CloopenReason, String)
	 */
	public abstract void controlMic(String confNo);

	/**
	 * 实时对讲接口  - 释放麦请求，其他成员可以发起控麦请求
	 * @param confNo  实时对讲会议号
	 * @see OnInterphoneListener#onReleaseMicState(CloopenReason)
	 */
	public abstract void releaseMic(String confNo);

	/**
	 * 实时对讲接口  - 查询实时对讲中参与成员，如果实时对讲不存在、非实时对讲成员则返回失败
	 * @param confNo 实时对讲会议号
	 * 
	 * @see OnInterphoneListener#onInterphoneMembers(CloopenReason, java.util.List)
	 */
	public abstract void queryMembersWithInterphone(String confNo);

	/**
	 * @see #startChatroom(String, String, int, String, String, boolean, int, boolean);
	 */
	@Deprecated public abstract void startChatroom(String appId, String roomName, int square, String keywords, String roomPwd);
	@Deprecated public abstract void startChatroom(String appId, String roomName, int square, String keywords, String roomPwd , boolean isAutoClose);
   
	/**
	 * @see #startChatroom(String, String, int, String, String, boolean, int, boolean, boolean)
	 * The default value for this method that autojoin the conference after create.
	 */
	public abstract void startChatroom(String appId, String roomName,
			int square, String keywords, String roomPwd , boolean isAutoClose ,int voiceMod , boolean isAutoDelete);

	/**
	 * 多人聊天室接口   - 创建聊天室
	 * @param appId 注册申请的应用id
	 * @param roomName 聊天室房间名称
	 * @param square 聊天室房间参与方数
	 * @param keywords 业务属性，应用定义
	 * @param roomPwd 聊天室密码。为空时，无密码
	 * @param isAutoClose 创建者退出时，是否解散聊天室
	 * @param voiceMod 背景音类型 0：无提示音有背景音；1：有提示音有背景音；2：无提示音无背景音；3：有提示音无背景音
	 * @param isAutoDelete 是否永久会议
	 * @param isAutoJoin 是否创建后自动加入
	 * 
	 * @see #setOnChatroomListener(OnChatroomListener)
	 * @see OnChatroomListener#onChatroomState(CloopenReason, String)
	 * @version 3.6.3
	 */
	public abstract void startChatroom(String appId, String roomName,
			int square, String keywords, String roomPwd , boolean isAutoClose ,int voiceMod , boolean isAutoDelete , boolean isAutoJoin);

	/**
	 * 多人聊天室接口   - 解散聊天室
	 * @param appId 注册申请的应用id
	 * @param roomNo 聊天室房间号
	 * 
	 * @see #setOnChatroomListener(OnChatroomListener)
	 * @see OnChatroomListener#onChatroomDismiss(CloopenReason, String)
	 * @see OnChatroomListener#onReceiveChatroomMsg(ChatroomMsg)
	 */
	public abstract void dismissChatroom(String appId,String roomNo);
	
	/**
	 * 多人聊天室接口   - 将某个成员移除出聊天室
	 * @param appId 注册申请的应用id
	 * @param roomNo 聊天室房间号
	 * @param member 被移除的成员号码
	 * @version 3.4.1.2  修改接口名称
	 * 
	 * @see #setOnChatroomListener(OnChatroomListener)
	 * @see OnChatroomListener#onReceiveChatroomMsg(ChatroomMsg)
	 */
	public abstract void removeMemberFromChatroom(String appId,String roomNo, String member);

	/**
	 * 多人聊天室接口   - 加入聊天室
	 * @param roomNo 聊天室房间号
	 * @param pwd 聊天室密码
	 * 
	 * @see #setOnChatroomListener(OnChatroomListener)
	 * @see OnChatroomListener#onChatroomState(CloopenReason, String)
	 */
	public abstract void joinChatroom(String roomNo ,String pwd);

	/**
	 * 多人聊天室接口   - 查询聊天室列表
	 * @param appId 注册申请的应用id
	 * @param keywords 业务属性，创建时由应用定义
	 * 
	 * @see #setOnChatroomListener(OnChatroomListener)
	 * @see OnChatroomListener#onChatrooms(CloopenReason, java.util.List)
	 */
	public abstract void queryChatrooms(String appId, String keywords);

	/**
	 * 多人聊天室接口   - 外呼邀请成员加入聊天室
	 * @param members 被邀请者的电话、账号，支持一次邀请多个
	 * @param roomNo 聊天室房间号
	 * @param appId 注册申请的应用id
	 * 
	 * @see #setOnChatroomListener(OnChatroomListener)
	 * @see OnChatroomListener#onChatroomInviteMembers(CloopenReason, String)
	 */
	public abstract void inviteMembersJoinChatroom(String[] members,
			String roomNo, String appId);

	/**
	 * 多人聊天室接口   - 退出当前聊天室
	 * @return true 成功
	 * @see #setOnChatroomListener(OnChatroomListener)
	 * @see OnChatroomListener#onReceiveChatroomMsg(ChatroomMsg)
	 */
	public abstract boolean exitChatroom();

	/**
	 * 多人聊天室接口   - 查询聊天室成员
	 * @param roomNo 聊天室房间号
	 * @see #setOnChatroomListener(OnChatroomListener)
	 * @see OnChatroomListener#onChatroomMembers(CloopenReason, java.util.List)
	 */
	public abstract void queryMembersWithChatroom(String roomNo);
	
	/**
	 * IM即时消息接口   -	开始录制音频，如果为chunk模式则会实时发送音频数据
	 * 此时不需要再调用IM发送接口
	 * 3.0.1版本修改增加返回消息ID
	 * 
	 * @param receiver 接收者，chunk为false时，可为null
	 * @param path 文件保存全路径路径
	 * @param chunked 是否chunk上传消息，true时边录边上传，false时只保存到本地文件
	 * @return 当前消息的msgId
	 * 
	 * @see #setOnIMListener(OnIMListener)
	 * @see OnIMListener#onSendInstanceMessage(CloopenReason, InstanceMsg)
	 * @see OnIMListener#onRecordingTimeOut(long)
	 * @see OnIMListener#onRecordingAmplitude(double)
	 */
	public abstract String startVoiceRecording(String receiver, String path, boolean chunked, String userDate) throws CCPRecordException;
	
	/**
	 * IM即时消息接口   -	停止录制音频数据，如果为chunk模式则会结束发送音频数据
	 * 此时不需要再调用IM发送接口
	 * 
	 * @see #setOnIMListener(OnIMListener)
	 * @see OnIMListener#onSendInstanceMessage(CloopenReason, InstanceMsg)
	 */
	public abstract void stopVoiceRecording();
	
	/**
	 * IM即时消息接口   -	取消录制,如果startVoiceRecording模式为chunk，则会取消发送
	 * 此时会通过onReceiveInstanceMessage接口回调通知发送被取消
	 * 
	 * @see #setOnIMListener(OnIMListener)
	 * @see OnIMListener#onReceiveInstanceMessage(InstanceMsg)
	 */
	public abstract void cancelVoiceRecording();
	
	
	@Deprecated public abstract void playVoiceMsg(String path);
	/**
	 * IM即时消息接口   -	根据提供的音频路径和播放模式播放音频文件
	 * @param path 音频本地全路径
	 * @param speakerOn 音频播放模式（true为扬声器、false为听筒）
	 * 
	 * @see #setOnIMListener(OnIMListener)
	 * @see OnIMListener#onFinishedPlaying()
	 */
	public abstract void playVoiceMsg(String path , boolean speakerOn);
	
	/**
	 * IM即时消息接口   -	停止播放音频文件
	 */
	public abstract void stopVoiceMsg();

	/**
	 * IM即时消息接口   -	开始录制视频 （预留接口）
	 * @param receiver receiver 接收者，chunk为false时，可为null
	 * @param path 文件保存本地全路径
	 * @param chunked 是否chunk上传消息，true时边录边上传，false时只保存到本地文件
	 * @return 当前消息的msgId
	 */
	public abstract void startVideoRecording(String receiver, String path,
			boolean chunked);

	/**
	 * IM即时消息接口   - 发送IM消息： 如果text不为null，attached为null 则发送文本消息，
	 * 如果text为null，attached不为空 发送文件消息，如果二者都为空则通过回调接口通知发送失败
	 * 注意：文本长度限制2000字节、userData限制长度255字节
	 * @param receiver 接收者
	 * @param text 发送的文本消息
	 * @param attached 发送的文件路径
	 * @param userData 用户自定义数据
	 * @return 当前消息的msgId，如果为null则发送失败，需要检测当前SDK的注册状态
	 * 
	 * @see #setOnIMListener(OnIMListener)
	 * @see OnIMListener#onSendInstanceMessage(CloopenReason, InstanceMsg)
	 * @see OnIMListener#onReceiveInstanceMessage(InstanceMsg)
	 */
	public abstract String sendInstanceMessage(String receiver, String text, String attached, String userData);

	/**
	 * IM即时消息接口   - 确认已下载文件消息
	 * @param msgId 消息id
	 */
	public abstract void confirmIntanceMessage(String[] msgIds);

	/**
	 * IM即时消息接口   - 下载IM消息附件
	 * @param urlList 下载参数（下载地址、如果是语音则需要当前语音的发送模式、消息ID等）
	 * 
	 * @see #setOnIMListener(OnIMListener)
	 * @see OnIMListener#onDownloadAttached(CloopenReason, String)
	 */
	public abstract void downloadAttached(ArrayList<DownloadInfo> urlList);

	/**********************************************************************
	 * enum define *
	 **********************************************************************/
	public static enum RunningType implements ObjectStringIdentifier{
		RunningType_None ("SDK idle"), 
		RunningType_Interphone ("Interphone"), 
		RunningType_Voip ("Audio/Video Call"), 
		RunningType_ChatRoom ("Chatroom" ), 
		RunningType_VideoConference ("Video Conference");

		private String mValue;
		
		private RunningType(String value) {
			this.mValue = value;
		}
		@Override
		public String getValue() {
			return mValue;
		}
	}

	public static enum State implements ObjectIdentifier{
		ONLINE(0), OFFLINE(1) , NOTEXIST (2), TIMEOUT(3);

		private int mValue;
		private State(int value) {
			this.mValue = value;
		}
		
		@Override
		public int getId() {
			return 0;
		}

		@Override
		public int getValue() {
			return mValue;
		}

	}

	public static enum CallType implements ObjectIdentifier{
		// 增加两种方法 点对点 拨打落地电话，来区别是否播放回铃音
		@Deprecated
		VOICEP2P (0),
		@Deprecated
		VOICEP2L(0),
		
		VOICE (0),
		VIDEO (1);
		
		private int mValue;
		private CallType(int value) {
			this.mValue = value;
		}
		@Override
		public int getId() {
			return 0;
		}
		@Override
		public int getValue() {
			return this.mValue;
		}
	}

	public static enum Rotate implements ObjectIdentifier{
		Rotate_Auto (0),
		Rotate_0 (1),
		Rotate_90 (2),
		Rotate_180 (3),
		Rotate_270 (4);
		
		private int mValue;
		private Rotate(int value) {
			this.mValue = value;
		}
		@Override
		public int getId() {
			return 0;
		}
		@Override
		public int getValue() {
			return this.mValue;
		}
	}

	public static enum Codec implements ObjectIdentifier{
		/**
		 * If you enable the current code, you must disable the front of the sort code.
		 */
		Codec_PCMU (0),
		Codec_G729 (1),
		Codec_OPUS48 (2),
		Codec_OPUS16 (3),
		Codec_OPUS8 (4),
		Codec_VP8 (5),
		Codec_H264 (6);
		
		private int mValue;
		private Codec(int value) {
			this.mValue = value;
		}
		@Override
		public int getId() {
			return 0;
		}
		@Override
		public int getValue() {
			return this.mValue;
		}
	}

	public static enum AudioType implements ObjectIdentifier{
		AUDIO_AGC (0),  // 自动增益控制，默认底层是关闭，开启后默认模式是kAgcAdaptiveAnalog
		AUDIO_EC (1), 	// 回音消除，默认开启，模式默认是kEcAecm
		AUDIO_NS (2); 	// 静音抑制，默认开启，模式默认是kNsVeryHighSuppression
		private int mValue;
		private AudioType(int value) {
			this.mValue = value;
		}
		@Override
		public int getId() {
			return 0;
		}
		@Override
		public int getValue() {
			return this.mValue;
		}
	}

	public static enum AudioMode implements ObjectIdentifier{
		// AUDIO_NS mode
		kNsUnchanged (0),
		kNsDefault (1),
		kNsConference (2),
		kNsLowSuppression (3),
		kNsModerateSuppression (4),
		kNsHighSuppression (5),
		kNsVeryHighSuppression (6),

		// AUDIO_AGC mode
		kAgcUnchanged (0),
		kAgcDefault (1),
		kAgcAdaptiveAnalog (2),
		kAgcAdaptiveDigital (3),
		kAgcFixedDigital (4),

		// AUDIO_EC mode
		kEcUnchanged (0),
		kEcDefault (1),
		kEcConference (2),
		kEcAec (3),
		kEcAecm (4);

		private int mValue;
		private AudioMode(int value) {
			this.mValue = value;
		}
		@Override
		public int getId() {
			return 0;
		}
		@Override
		public int getValue() {
			return this.mValue;
		}
	}
	
	
	/**
	 * 获取SDK支持库版本信息(libserphone.so)
	 * 版本信息格式为："版本号#平台#ARM版本#音频开关#视频开关#编译日期 编译时间"
	 * 平台: Android、 Windows、 iOS、 Mac OS、 Linux
	 * ARM版本:  arm、 armv7、 armv5
	 * 音频开关:  voice=false、 voice=true
	 * 视频开关:  video=false、 video=true
	 * 编译日期:  "MM DD YYYY"  如"Jan 19 2013"
	 * 编译时间:  "hh:mm:ss"    如”08:30:23”）
	 * @return 版本信息字符串
	 * 
	 * @version 3.3
	 */
	public abstract String getVersion();
	
	/**
	 * 实时获取通话中的统计数据
	 * 获取通话的统计信息后，根据丢包率和延迟时间，判断通话的网络状况；也可以统计通话的网络流量；获取统计信息的间隔建议在4秒以上。
	 * @param callType 当前通话类型音频或者视频
	 * @return 统计数据信息，获取失败则返回Null
	 */
	public abstract CallStatisticsInfo getCallStatistics(CallType callType);
	

	/**
	 * 是否屏蔽视频解码过程中的马赛克，默认不屏蔽
	 * @param flag true 屏蔽，false不屏蔽
	 * @return true成功，false失败
	 * @version 3.4.1.1
	 */
	public abstract int setShieldMosaic(boolean flag);
	
	/**
	 * @see #startVideoConference(String, String, int, String, String, boolean, int, boolean)
	 * @verson 3.5
	 */
	@Deprecated public abstract void startVideoConference(String appId , String conferenceName , int square , String keywords , String conferencePwd) ;
	@Deprecated public abstract void startVideoConference(String appId , String conferenceName , int square , String keywords , String conferencePwd , boolean isAutoClose) ;
	
	/**
	 * 视频会议接口   - 发起视频会议，该接口默认创建成功后即加入视频会议
	 * @see #startVideoConference(String, String, int, String, String, boolean, int, boolean, boolean)
	 * @param appId 注册申请的应用id
	 * @param conferenceName 视频会议房间名称
	 * @param square 视频会议参与的最大人数
	 * @param keywords 业务属性，应用定义
	 * @param conferencePwd 视频会议密码。为空时则无密码
	 * @param isAutoClose 创建者退出时，是否解散视频会议
	 * @param voiceMod 背景音类型 0：无提示音有背景音；1：有提示音有背景音；2：无提示音无背景音；3：有提示音无背景音
	 * @param isAutoDelete 是否创建后自动加入视频会议
	 * 
	 * @see #setOnVideoConferenceListener(OnVideoConferenceListener)
	 * @see OnVideoConferenceListener#onVideoConferenceState(CloopenReason, String)
	 */
	public abstract void startVideoConference(String appId,
			String conferenceName, int square, String keywords,
			String conferencePwd, boolean isAutoClose, int voiceMod,
			boolean isAutoDelete);
	
	/**
	 * 视频会议接口   - 发起视频会议，该接口需要用户选择创建成功后是否自动加入视频会议
	 * @param appId 注册申请的应用id
	 * @param conferenceName 视频会议房间名称
	 * @param square 视频会议参与的最大人数
	 * @param keywords 业务属性，应用定义
	 * @param conferencePwd 视频会议密码。为空时则无密码
	 * @param isAutoClose 创建者退出时，是否解散视频会议
	 * @param voiceMod 背景音类型 0：无提示音有背景音；1：有提示音有背景音；2：无提示音无背景音；3：有提示音无背景音
	 * @param isAutoDelete 是否永久视频会议
	 * @param isAutoJoin 是否创建后自动加入视频会议
	 * 
	 * @see #setOnVideoConferenceListener(OnVideoConferenceListener)
	 * @see OnVideoConferenceListener#onVideoConferenceState(CloopenReason, String)
	 * 
	 * @version 3.6.3
	 */
	@Deprecated public abstract void startVideoConference(String appId,
			String conferenceName, int square, String keywords,
			String conferencePwd, boolean isAutoClose, int voiceMod,
			boolean isAutoDelete, boolean isAutoJoin);	
	
	public abstract void startMultiVideoConference(String appId,
			String conferenceName, int square, String keywords,
			String conferencePwd, boolean isAutoClose, int voiceMod,
			boolean isAutoDelete, boolean isAutoJoin);
	
	
	//public abstract void startVideoConference(String appId, String conferenceName,String conferencePwd , ConferenceOptions options);
	
	/**
	 * 视频会议接口   - 加入视频会议
	 * @param conferenceId 视频会议房间号
	 * 
	 * @see #setOnVideoConferenceListener(OnVideoConferenceListener)
	 * @see OnVideoConferenceListener#onVideoConferenceState(CloopenReason, String)
	 * @version 3.5
	 */
	public abstract void joinVideoConference(String conferenceId);
	
	/**
	 * 视频会议接口   - 退出视频会议
	 * @return true成功，false失败
	 * 
	 * @see #setOnVideoConferenceListener(OnVideoConferenceListener)
	 * @see OnVideoConferenceListener#onReceiveVideoConferenceMsg(VideoConferenceMsg)
	 * @version 3.5
	 */
	public abstract boolean exitVideoConference();
	
	/**
	 * 视频会议接口   - 查询参与视频会议成员
	 * @param conferenceId 视频会议房间号
	 * 
	 * @see #setOnVideoConferenceListener(OnVideoConferenceListener)
	 * @see OnVideoConferenceListener#onVideoConferenceMembers(CloopenReason, java.util.List)
	 * @version 3.5
	 */
	public abstract void queryMembersInVideoConference(String conferenceId);
	
	/**
	 * 视频会议接口   - 查询视频会议列表
	 * @param appId 注册申请的应用id
	 * @param keywords 业务属性，创建视频会议时由应用定义
	 * 
	 * @see #setOnVideoConferenceListener(OnVideoConferenceListener)
	 * @see OnVideoConferenceListener#onVideoConferences(CloopenReason, java.util.List)
	 * @version 3.5
	 */
	public abstract void queryVideoConferences(String appId , String keywords);
	
	/**
	 * 视频会议接口   - 解散视频会议，该接口只允许视频会议创建者调用
	 * @param appId 注册申请的应用id
	 * @param conferenceId 视频会议房间号
	 * 
	 * @see #setOnVideoConferenceListener(OnVideoConferenceListener)
	 * @see OnVideoConferenceListener#onVideoConferenceDismiss(CloopenReason, String)
	 * @see OnVideoConferenceListener#onReceiveVideoConferenceMsg(VideoConferenceMsg)
	 * @version 3.5
	 */
	public abstract void dismissVideoConference(String appId , String conferenceId);
	
	/**
	 * 视频会议接口   - 将某个成员从视频会议中移除，该接口仅限于单次移除一个成员
	 * @param appId 注册申请的应用id
	 * @param conferenceId 视频会议房间号
	 * @param member 被移除的视频会议成员
	 * 
	 * @see #setOnVideoConferenceListener(OnVideoConferenceListener)
	 * @see OnVideoConferenceListener#onVideoConferenceRemoveMember(CloopenReason, String)
	 * @see OnVideoConferenceListener#onReceiveVideoConferenceMsg(VideoConferenceMsg)
	 * @version 3.5
	 */
	public abstract void removeMemberFromVideoConference(String appId , String conferenceId , String member);
	
	/**
	 * 视频会议接口   - 切换视频会议主频
	 * @param appId 注册申请的应用id
	 * @param conferenceId 视频会议房间号
	 * @param voip 被设置为主屏的成员账号
	 * 
	 * @see #setOnVideoConferenceListener(OnVideoConferenceListener)
	 * @see OnVideoConferenceListener#onVideoConferenceRemoveMember(CloopenReason, String)
	 * @see OnVideoConferenceListener#onReceiveVideoConferenceMsg(VideoConferenceMsg)
	 * @version 3.5
	 */
	@Deprecated public abstract void switchRealScreenToVoip(String appId , String conferenceId , String voip);
	
	
	/**
	 * 视频会议接口   - 获取视频会议头像列表
	 * @param conferenceId 视频会议房间号
	 * 
	 * @see #setOnVideoConferenceListener(OnVideoConferenceListener)
	 * @see OnVideoConferenceListener#onGetPortraitsFromVideoConference(CloopenReason, java.util.List)
	 * @version 3.5
	 */
	@Deprecated public abstract void getPortraitsFromVideoConference(String conferenceId);
	
	
	/**
	 * 视频会议接口   - 上传自己的视频会议头像数据
	 * @param fileName 头像数据本地全路径
	 * @param conferenceId 视频会议房间号
	 * 
	 * @see #setOnVideoConferenceListener(OnVideoConferenceListener)
	 * @see OnVideoConferenceListener#onSendLocalPortrait(CloopenReason, String)
	 * @version 3.5
	 */
	@Deprecated public abstract void sendLocalPortrait(String fileName , String conferenceId);
	
	/**
	 * 视频会议接口   - 下载视频会议头像数据
	 * @param portraitsList 视频会议头像参数信息
	 * 
	 * @see #setOnVideoConferenceListener(OnVideoConferenceListener)
	 * @see OnVideoConferenceListener#onDownloadVideoConferencePortraits(CloopenReason, VideoPartnerPortrait)
	 * @version 3.5
	 */
	@Deprecated public abstract void downloadVideoConferencePortraits(ArrayList<VideoPartnerPortrait> portraitsList);
	
	/**
	 * 获取本地视频帧数据
	 * 视频通话或者视频会议时，调用此接口获取本地视频显示的帧数据；注意：请在非UI线程调用此接口
	 * @return 视频帧数据
	 * @version 3.6
	 */
	@Deprecated public abstract VideoSnapshot getLocalVideoSnapshot() ;
	
	/**
	 * 获取远端视频帧数据
	 * 视频通话或者视频会议时，调用此接口获取本地视频显示的帧数据；注意：请在非UI线程调用此接口
	 * @return 视频帧数据
	 * @version 3.6
	 */
	@Deprecated public abstract VideoSnapshot getRemoteVideoSnapshot() ;
	
	/**
	 * 录制通话音频数据
	 * @param callid 当前通话的唯一标识
	 * @param fileName 音频数据本地保存路径，确保设置的路径已经存在，否则会抛出CCPRecordException异常
	 * @throws CCPRecordException 
	 * 
	 * @see {@link #stopVoiceCallRecording(String)}
	 * @verson 3.6
	 */
	public abstract void startVoiceCallRecording(String callid , String fileName) throws CCPRecordException;
	
	/**
	 * 停止录制音频数据，
	 * @param callid
	 */
	public abstract void stopVoiceCallRecording(String callid);
	
	/**
	 * 检测账号在线状态
	 * @param account 需要检测的账号
	 * @return 在线状态
	 * 
	 * @see State
	 */
	public abstract State checkUserOnline(String account);
	
	/**
	 * 获取VoIP、视频、实时对讲、聊天室、会议上下行流量
	 * @param callid 唯一标识、可以是VoIP、视频通话的callid、可以是会议房间号
	 * @return 
	 */
	public abstract NetworkStatistic getNetworkStatistic(String callid);
	
	/**
	 *  根证书设置接口
	 * @param caPath 根证书路径.
	 * @return 成功 true 失败 false
	 */
	public abstract boolean setRootCAPath(String caPath);
	
	/**
	 * 客户端证书设置接口
	 * @param certPath 客户端证书路径.
	 * @return 成功 true 失败 false
	 */
	public abstract boolean setClientCertPath(String certPath);
	
	/**
	 * 客户端秘钥设置接口
	 * @param keyPath 客户端秘钥路径.
	 *	成功 true 失败 false
	 **/
	public abstract boolean setClientKeyPath(String keyPath);
	
	/**
	 * 设置SRTP加密属性
	 * @param tls ture设置信令加密； false不设置信令加密.
	 * @param srtp true设置srtp加密；false不设置srtp加密。该值为false时，
	 * 				userMode、cryptType、key等参数均忽略。
	 * @param cryptType
	 * 			 AES_256_SHA1_80 =3,
	 * 			 AES_256_SHA1_32 =4,
	 * @param key 加解密秘钥（长度46个字节）。不需要设置key值时，请传NULL。当key值为NULL时，应用随机生成key值。
	     		返回值   : -1
	 */
	public abstract boolean setTlsSrtpEnabled(boolean tls , boolean srtp , int cryptType , String key);
	
	/**
	 *  私有云校验接口
	 * @param companyID  企业ID.
	 * @param restAddr 软交换地址.
	 * @param nativeCheck 是否本地校验.
	 * @return 成功 0 -1 companyID过长（最大199） -2 restAdd过长（99）
	 */
	public static void setPrivateCloud(String companyID , String restAddr , boolean nativeCheck){
		CallControlManager.setPrivateCloud(companyID, restAddr, nativeCheck);
	}
	
	/**
	 * 初始化音频设备
	 * @return 成功 True 失败  False
	 */
	public abstract boolean registerAudioDevice();

	/**
     * 释放音频设备
     * @return 成功 True 失败  False
     */
    public abstract boolean deregisterAudioDevice();
	
	/**
	 * ip路由功能,设置网络组ID
	 * @param groupId
	 * @return  成功 0 失败 -1
	 */
	public abstract boolean SetNetworkGroupId(String groupId);
	
	
	/**
	 * 视频会议中请求某一远端视频
	 * @param conferenceNo 所在会议号
	 * @param conferencePasswd 所在会议密码
	 * @param remoteSipNo  请求远端用户的sip号
	 * @param videoWindow 当成功请求时，展示该成员的窗口
	 * @param port 当前请求的目的端口
	 * @return
	 */
	public abstract int requestMemberVideo(String conferenceId, String conferencePasswd , String voipAccount, View displayView, String ip , int port);
	
	/**
	 * 视频会议中停止某一远端视频
	 * @param conferenceNo 所在会议号
	 * @param conferencePasswd 所在会议密码
	 * @param remoteSipNo 请求远端用户的sip号
	 * @return
	 */
	public abstract int cancelMemberVideo(String conferenceId, String conferencePasswd , String voipAccount);
	
	public abstract void publishVideoFrame(String appId , String conferenceId , OnVideoMemberFrameListener l);
	public abstract void unPublishVideoFrame(String appId , String conferenceId , OnVideoMemberFrameListener l);
	public abstract void resetVideoConfWindow(String conferenceId , View displayView);
	
	/**
	 * 设置VoIP、视频通话状态监听
	 * @param onVoIPListener
	 */
	public abstract void setOnVoIPListener(OnVoIPListener onVoIPListener);
	
	/**
	 * 设置语音聊天室状态监听
	 * @param onChatroomListener
	 */
	public abstract void setOnChatroomListener(OnChatroomListener onChatroomListener);
	
	/**
	 * 设置实时对讲监听
	 * @param onInterphoneListener
	 */
	public abstract void setOnInterphoneListener(OnInterphoneListener onInterphoneListener);
	
	/**
	 * 设置IM即时消息监听
	 * @param onIMListener
	 */
	public abstract void setOnIMListener(OnIMListener onIMListener);
	
	/**
	 * 设置视频会议监听
	 * @param onVideoConferenceListener
	 */
	public abstract void setOnVideoConferenceListener(OnVideoConferenceListener onVideoConferenceListener);
	
	/**
	 * 设置srtp加密监听，该接口需要在通话建立之前调用
	 * @param onTriggerSrtpListener
	 */
	public abstract void setOnTriggerSrtpListener(OnTriggerSrtpListener onTriggerSrtpListener);
	
	/**
	 * 设置通话录制监听
	 * @param onCallRecordListener
	 */
	public abstract void setOnCallRecordListener(OnCallRecordListener onCallRecordListener);
	
	static class CCPListenerInfo {
		
		protected DeviceListener deviceListener;
		/**
		 * Listener for VoIP call 
		 */
		protected OnVoIPListener mOnVoIPListener ;
		
		/**
		 * Listener for Voice/Video record.
		 */
		protected OnCallRecordListener mOnCallRecordListener;
		
		/**
		 * Listener for Voice/Video record.
		 */
		protected OnCallProcessDataListener mOnCallProcessDataListener;
		
		
		protected OnProcessOriginalAudioDataListener mProcessOriginalAudioDataListener;
		
		/**
		 * Listener for Voice Chatroom
		 */
		protected OnChatroomListener mOnChatroomListener ;
		
		/**
		 * Listener for Real-time intercom,interphone
		 */
		protected OnInterphoneListener mOnInterphoneListener ;
		
		/**
		 * Listener for Instant messaging, multimedia message
		 */
		protected OnIMListener mOnIMListener ;
		
		/**
		 * Listener for Video Conference 
		 * @version 3.5
		 */
		protected OnVideoConferenceListener mOnVideoConferenceListener ;
		
		protected OnTriggerSrtpListener mOnTriggerSrtpListener;
		protected OnVideoMemberFrameListener onVideoMemberFrameListener;
		
		
		void release() {
			deviceListener = null;
			mOnVoIPListener = null;
			mOnChatroomListener = null;
			mOnCallRecordListener = null;
			mOnInterphoneListener = null;
			mOnIMListener = null;
			mOnTriggerSrtpListener = null;
		}
	}
	
	
	/**
	 * @see #setSrtpEnabled(boolean, boolean, int, String)
	 */
	@Deprecated public abstract void setSrtpEnabled(final String key);
	
	/**
	 * (定制接口)
	 * 设置srtp加密属性，当服务器询问客户端是否需要srtp加密时会通过{@link OnTriggerSrtpListener#OnTriggerSrtp(String, boolean)}
	 * 接口来通知客户端，客户端收到srtp加密通知时调用该接口来设置srtp加密属性。
	 * @param srtp 启用或关闭srtp加密，该值为false时，userMode、cryptType、key等参数均忽略。
	 * @param userMode 采用的srtp加密模式，当前支持两种模式：true用户模式，false标准模式。
	 * 		     用户模式时将使用接口设置的Key进行加解密，非用户模式时，将采用标准srtp模式。
	 * @param cryptType 参数范围为0和1 ，超过1的统一使用0模式
	 * 		  AES_256_SHA1_80 =0
	 * 		  AES_256_SHA1_32 =1
	 * @param key 加解密秘钥（长度46个字节）
	 */
	public abstract void setSrtpEnabled(boolean srtp, boolean userMode, int cryptType, String key);
	
	/**
	 * (定制接口)
     * Interface definition for a callback to be invoked when enabled that 
     * want to process audio data.
     * 
	 * @param callid 当前呼叫的唯一标识.
	 * @param enabled 允许上层处理； false 不允许上层处理。
	 * @param l 实现该
	 * 
	 * @see OnCallProcessDataListener
	 */
	public abstract void setProcessDataEnabled(String callid, boolean enabled , OnCallProcessDataListener l);
	public abstract void setProcessOriginalDataEnabled(String callid, boolean enabled , OnProcessOriginalAudioDataListener l);
	
	/**
	 *（定制接口）
	 * 多人聊天室接口   - 设置聊天室成员可听可讲权限
	 * @param appId 注册申请的应用id
	 * @param roomNo 聊天室房间号
	 * @param member 被设置权限的聊天室成员号，不支持一次性设置多个
	 * @param opt 可听可讲状态
	 * 
	 * @see #setOnChatroomListener(OnChatroomListener)
	 * @see OnChatroomListener#onSetMemberSpeakOpt(CloopenReason, String)
	 * @see OnChatroomListener#onReceiveChatroomMsg(ChatroomMsg)
	 */
	public abstract void setChatroomMemberSpeakOpt(String appId,String roomNo, String member , int opt);
	
	/**
	 *（定制接口）
	 * 两个参数都是0， 表示关掉keepAlive
	 * @param wifi
	 * @param mobile
	 * @return 
	 */
	public abstract void setKeepAliveTimeout(int wifi , int mobile);
	
	public abstract  int setScreenShareActivity(String callid, View view);
	
}
