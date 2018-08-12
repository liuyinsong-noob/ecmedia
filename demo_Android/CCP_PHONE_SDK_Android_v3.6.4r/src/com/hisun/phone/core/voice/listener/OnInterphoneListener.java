package com.hisun.phone.core.voice.listener;

import java.util.List;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.CloopenReason;
import com.hisun.phone.core.voice.model.interphone.InterphoneControlMicMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneExitMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneJoinMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneMember;
import com.hisun.phone.core.voice.model.interphone.InterphoneMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneOverMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneReleaseMicMsg;

/**
 * 
* <p>Title: InterphoneListener.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-10-22
* @version 3.5
 */
public interface OnInterphoneListener {

	/**********************************************************************
	 * Interphone *
	 **********************************************************************/

	/**
	 * 创建实时对讲或者加入实时对讲回调接口，客户端在调用SDK接口{@link Device#startInterphone(String[], String)}
	 * 发起一个新的实时对讲或者接口{@link Device#joinInterphone(String)}加入一个已经存在的实时对讲，SDK会回调该接口
	 * 通知客户端创建或者加入结果。
	 * 
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param confNo 创建或者加入的实时对讲房间号
	 * 
	 * @see Device#startInterphone(String[], String)
	 * @see Device#joinInterphone(String)
	 */
	public abstract void onInterphoneState(CloopenReason reason, String confNo);

	/**
	 * 实时对讲控麦结果回调，客户端在调用SDK接口{@link Device#controlMic(String)}发起控麦请求，
	 * SDK会通过该接口回调通知客户端控麦结果。
	 * 
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param speaker 当前控麦者的账号
	 * 
	 * @see Device#controlMic(String)
	 */
	public abstract void onControlMicState(CloopenReason reason, String speaker);

	/**
	 * 实时对讲释放麦结果回调，客户端在调用SDK接口{@link Device#releaseMic(String)}发起释放麦请求，
	 * SDK会通过该回调接口通知客户端释放结果
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * 
	 * @see Device#releaseMic(String)
	 */
	public abstract void onReleaseMicState(CloopenReason reason);

	/**
	 * 查询实时对讲成员结果回调，客户端在调用SDK接口{@link Device#queryMembersWithInterphone(String)}
	 * 请求查询实时对讲成员时，SDK将查询结果通过该接口回调通知客户端。
	 * 
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param members 当前实时对讲成员列表，查询失败则为null
	 * 
	 * @see Device#queryMembersWithInterphone(String)
	 */
	public abstract void onInterphoneMembers(CloopenReason reason,
			List<InterphoneMember> members);

	/**
	 * 实时对讲新消息通知接口，当SDK接收到新的实时对讲消息（如：有人加入、有人退出、有人控麦、有人放麦）
	 * SDK将相应的消息类型通过该回调接口通知客户端。
	 * 
	 * @param msg 消息类型（如：有人加入、有人退出、有人控麦、有人放麦）
	 * 
	 * @see InterphoneJoinMsg
	 * @see InterphoneExitMsg
	 * @see InterphoneOverMsg
	 * @see InterphoneControlMicMsg
	 * @see InterphoneReleaseMicMsg
	 */
	public abstract void onReceiveInterphoneMsg(InterphoneMsg msg);
}
