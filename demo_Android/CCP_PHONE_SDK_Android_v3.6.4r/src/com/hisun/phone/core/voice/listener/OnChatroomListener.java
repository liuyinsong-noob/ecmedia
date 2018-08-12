package com.hisun.phone.core.voice.listener;

import java.util.List;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.CloopenReason;
import com.hisun.phone.core.voice.model.chatroom.Chatroom;
import com.hisun.phone.core.voice.model.chatroom.ChatroomDismissMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomExitMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomJoinMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMember;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMemberForbidOpt;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomRemoveMemberMsg;
/**
 * 
* <p>Title: OnChatroomListener.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author 云通讯
* @date 2013-10-22
* @version 3.5
 */
public interface OnChatroomListener {

	/**********************************************************************
	 * ChatRoom *
	 **********************************************************************/

	/**
	 * 创建语音聊天室、加入语音聊天室回调接口，roomNo为创建聊天室成功以及申请加入的聊天室
	 * 房间号，如果创建聊天室失败则roomNo 为空，reason为当前失败的原因
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param roomNo 聊天室房间号
	 * 
	 * @see CloopenReason
	 * @see Device#startChatroom(String, String, int, String, String, boolean, int, boolean, boolean)
	 * @see Device#joinChatroom(String, String)
	 */
	public abstract void onChatroomState(CloopenReason reason, String roomNo);

	/**
	 * 当聊天室创建者发起解散聊天室请求，SDK在执行完解散聊天室请求后通过此接口回调通知
	 * 创建者解散结果，该回调接口只回调通知{@link Device#dismissChatroom(String, String)}接口调用者
	 * 其他非创建者聊天室成员通过系统事件通知接口{@link #onReceiveChatroomMsg(ChatroomMsg)}获得聊天室
	 * 被解散的通知，并且通知类型为{@link ChatroomDismissMsg}
	 * 
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param roomNo 被解散的聊天室房间号
	 * 
	 * @see ChatroomDismissMsg
	 * @see CloopenReason
	 * @see Device#dismissChatroom(String, String)
	 */
	public abstract void onChatroomDismiss(CloopenReason reason, String roomNo);
	
	/**
	 * 当聊天室创建者发起将某个成员移除出聊天室请求，SDK在执行完移除请求后通过此接口回调
	 * 通知接口调用者移除结果，该回调接口只回调通知{@link Device#removeMemberFromChatroom(String, String, String)}接口调用者
	 * 其他非创建者聊天室成员将收到通过系统事件通知接口{@link #onReceiveChatroomMsg(ChatroomMsg)}获得
	 * 某个成员被移除出聊天室的通知，且通知类型为{@link ChatroomRemoveMemberMsg}
	 * 
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param member 被移除的成员账号
	 * 
	 * @see ChatroomRemoveMemberMsg
	 * @see CloopenReason
	 * @see Device#removeMemberFromChatroom(String, String, String)
	 */
	public abstract void onChatroomRemoveMember(CloopenReason reason, String member);
	
	/**
	 * 当聊天室成员发起查询当前所在聊天室参与成员请求后，SDK在执行完查询请求后会通过此接口回调
	 * 通知发起者查询结果，
	 * 
	 * 注意：该结构调用者必须加入聊天室成功之后调用，否则将返回查询失败
	 * 
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param members 聊天室参与成员集合
	 * 
	 * @see ChatroomMember
	 * @see CloopenReason
	 * @see Device#queryMembersWithChatroom(String)
	 */
	public abstract void onChatroomMembers(CloopenReason reason,
			List<ChatroomMember> members);

	/**
	 * 语音聊天室系统时间通知，如：有人加入、退出聊天室、聊天室被解散等
	 * 该时间通知接口又 SDK主动通知，开发者只需要收到该时间通知并根据参数类型
	 * 的不同来处理不同的应用场景事件。
	 * 
	 * 当执行如下：{@link Device#joinChatroom(String, String)}加入聊天室、{@link Device#exitChatroom()}退出聊天室
	 * {@link Device#dismissChatroom(String, String)}解散聊天室、{@link Device#removeMemberFromChatroom(String, String, String)}从聊天室移除某成员
	 * SDK会通过该事件通知接口来通知当前聊天室成员
	 * 
	 * @param msg 事件通知类型，可能是如下其中的一个：退出、加入：解散、被移除、禁言
	 * 
	 * @see ChatroomExitMsg
	 * @see ChatroomJoinMsg
	 * @see ChatroomDismissMsg
	 * @see ChatroomRemoveMemberMsg
	 * @see ChatroomMemberForbidOpt
	 */
	public abstract void onReceiveChatroomMsg(ChatroomMsg msg);

	/**
	 * 当聊天室创建者/发起者通过外呼邀请接口{@link Device#inviteMembersJoinChatroom(String[], String, String)}
	 * 邀请电话或者VoIP号加入聊天室，SDK在执行完邀请请求后会通过该回调接口通知调用者邀请结果
	 * 
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param roomNo 聊天室房间号
	 * 
	 * @see CloopenReason
	 * @see Device#inviteMembersJoinChatroom(String[], String, String)
	 */
	public abstract void onChatroomInviteMembers(CloopenReason reason, String roomNo);

	/**
	 * 查询聊天室列表回调接口，通知接口调用者查询结果,该接口对应于主调接口{@link Device#queryChatrooms(String, String)}
	 * 
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param chatrooms 聊天室列表
	 * 
	 * @see CloopenReason
	 * @see Device#queryChatrooms(String, String)
	 */
	public abstract void onChatrooms(CloopenReason reason, List<Chatroom> chatrooms);
	
	/**
	 * 该接口为定制接口
	 * 
	 * 设置聊天室成员可听可讲权限回调接口，通知接口调用者设置结果，
	 * 该接口对应于主调接口{@link Device#setChatroomMemberSpeakOpt(String, String, String, int)
	 * 
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param member 被设置权限的成员账号
	 * 
	 * @see CloopenReason
	 * @see Device#setChatroomMemberSpeakOpt(String, String, String, int)
	 */
	public abstract void onSetMemberSpeakOpt(CloopenReason reason, String member);
}
