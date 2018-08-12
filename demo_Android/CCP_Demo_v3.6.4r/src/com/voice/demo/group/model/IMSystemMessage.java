/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.yuntongxun.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
package com.voice.demo.group.model;

import com.hisun.phone.core.voice.model.im.InstanceMsg;

public class IMSystemMessage extends InstanceMsg {
	
	public static final int SYSTEM_MESSAGE_NEED_REPLAY = 1;
	public static final int SYSTEM_MESSAGE_NONEED_REPLAY = 2;
	public static final int SYSTEM_MESSAGE_THROUGH = 3;
	public static final int SYSTEM_MESSAGE_REFUSE = 4;
	
	public static final int STATE_UNREAD = 0;
	public static final int STATE_READED = 1;
	
	public static final int SYSTEM_TYPE_APPLY_JOIN = 401;
	public static final int SYSTEM_TYPE_ACCEPT_OR_REJECT_JOIN = 402;
	public static final int SYSTEM_TYPE_INVITE_JOIN = 403;
	public static final int SYSTEM_TYPE_REMOVE = 404;
	public static final int SYSTEM_TYPE_GROUP_MEMBER_QUIT = 405;
	public static final int SYSTEM_TYPE_GROUP_DISMISS = 406;
	public static final int SYSTEM_TYPE_APPLY_JOIN_UNVALIDATION = 407;
	public static final int SYSTEM_TYPE_REPLY_GROUP_APPLY = 408;
	/**
	 * 
	 */
	private static final long serialVersionUID = -4092674621364796466L;

	private String messageId;
	private int messageType;
	private String verifyMsg;
	private int state;
	private String groupId;
	private String who;
	private int isRead;
	private String curDate;
	
	public String getMessageId() {
		return messageId;
	}
	public void setMessageId(String messageId) {
		this.messageId = messageId;
	}
	public int getMessageType() {
		return messageType;
	}
	public void setMessageType(int messageType) {
		this.messageType = messageType;
	}
	public String getVerifyMsg() {
		return verifyMsg;
	}
	public void setVerifyMsg(String verifyMsg) {
		this.verifyMsg = verifyMsg;
	}
	public int getState() {
		return state;
	}
	public void setState(int state) {
		this.state = state;
	}
	public String getGroupId() {
		return groupId;
	}
	public void setGroupId(String groupId) {
		this.groupId = groupId;
	}
	public String getWho() {
		return who;
	}
	public void setWho(String who) {
		this.who = who;
	}
	public int getIsRead() {
		return isRead;
	}
	public void setIsRead(int isRead) {
		this.isRead = isRead;
	}
	public String getCurDate() {
		return curDate;
	}
	public void setCurDate(String curDate) {
		this.curDate = curDate;
	}
	public IMSystemMessage(String messageId, int messageType, String verifyMsg,
			int state, String groupId, String who, int isRead, String curDate) {
		super();
		this.messageId = messageId;
		this.messageType = messageType;
		this.verifyMsg = verifyMsg;
		this.state = state;
		this.groupId = groupId;
		this.who = who;
		this.isRead = isRead;
		this.curDate = curDate;
	}
	public IMSystemMessage() {
		super();
	}
	
	
	
}
