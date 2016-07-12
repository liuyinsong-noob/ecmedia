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
 */
package com.voice.demo.group.model;

import android.text.TextUtils;

import com.hisun.phone.core.voice.model.im.InstanceMsg;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPUtil;

/**
 * Group chat or point to point IM chat message object,
 * Display in the session details
 * Including text, files, and voice.
 * 
 * @version Time: 2013-7-23
 */
public class IMChatMessageDetail extends InstanceMsg{

	/**
	 * 
	 */
	private static final long serialVersionUID = -6780590549815508314L;
	
	public static final int TYPE_MSG_TEXT = 1;
	public static final int TYPE_MSG_FILE = 2;
	public static final int TYPE_MSG_VOICE = 3;
	
	public static final int STATE_UNREAD = 0;
	public static final int STATE_READED = 1;
	
	public static final int STATE_IM_SENDING = 0;
	public static final int STATE_IM_SEND_SUCCESS = 1;
	public static final int STATE_IM_SEND_FAILED = 2;
	public static final int STATE_IM_RECEIVEED = 3;
	
	private String messageId;           // 消息id
	private String sessionId;			// 会话id
	private int    messageType;         // 消息类型  如：文本、文件、语音等  可以自定义
	private String groupSender;         // 消息的发送者
	private int    isRead;              // 消息是否已读
	private int    imState;             // 消息的状态    
	private String dateCreated;         // 服务器时间
	private String curDate;				// 本地时间
	private String userData;			// 发送userdata字段内容
	private String messageContent;      // 文本消息内容
	private String fileUrl;             // 附件下载地址
	private String filePath;			// 附件本地地址
	private String fileExt;				// 附件的后缀
	private long duration;				// 语音时间
	
	
	public String getGroupSender() {
		return groupSender;
	}
	public void setGroupSender(String groupSender) {
		this.groupSender = groupSender;
	}

	public String getMessageId() {
		return messageId;
	}
	public void setMessageId(String messageId) {
		this.messageId = messageId;
	}
	public String getSessionId() {
		return sessionId;
	}
	public void setSessionId(String sessionId) {
		this.sessionId = sessionId;
	}
	public String getDateCreated() {
		return dateCreated;
	}
	public void setDateCreated(String dateCreated) {
		this.dateCreated = dateCreated;
	}
	public String getCurDate() {
		return curDate;
	}
	public void setCurDate(String curDate) {
		this.curDate = curDate;
	}
	public String getFileUrl() {
		return fileUrl;
	}
	public void setFileUrl(String fileUrl) {
		this.fileUrl = fileUrl;
	}
	public String getFilePath() {
		return filePath;
	}
	public void setFilePath(String filePath) {
		this.filePath = filePath;
	}
	public String getFileExt() {
		return fileExt;
	}
	public void setFileExt(String fileExt) {
		this.fileExt = fileExt;
	}
	public String getMessageContent() {
		return messageContent;
	}
	public void setMessageContent(String messageContent) {
		this.messageContent = messageContent;
	}

	public int getReadStatus() {
		return isRead;
	}
	public void setReadStatus(int readStatus) {
		this.isRead = readStatus;
	}
	
	public int getImState() {
		return imState;
	}
	public void setImState(int imState) {
		this.imState = imState;
	}
	public String getUserData() {
		return userData;
	}
	public void setUserData(String userData) {
		this.userData = userData;
	}
	public int getMessageType() {
		return messageType;
	}
	public void setMessageType(int messageType) {
		this.messageType = messageType;
	}
	
	public long getDuration() {
		return duration;
	}
	public void setDuration(long duration) {
		this.duration = duration;
	}
	public IMChatMessageDetail() {
		super();
	}
	
	
	
	public IMChatMessageDetail(String messageId, String sessionId,
			int messageType, String groupSender, int isRead, int imState,
			String dateCreated, String curDate, String userData,
			String messageContent, String fileUrl, String filePath,
			String fileExt) {
		super();
		this.messageId = messageId;
		this.sessionId = sessionId;
		this.messageType = messageType;
		this.groupSender = groupSender;
		this.isRead = isRead;
		this.imState = imState;
		this.dateCreated = dateCreated;
		this.curDate = curDate;
		this.userData = userData;
		this.messageContent = messageContent;
		this.fileUrl = fileUrl;
		this.filePath = filePath;
		this.fileExt = fileExt;
	}
	/***
	 * 
	 * @param textOrFile
	 * @param sendType
	 * @param contactId
	 * @param contactName
	 * @return
	 */
	public static IMChatMessageDetail getGroupItemMessage(int textOrFile  , int imState , String sessionId) {
		IMChatMessageDetail chatMessageDetail = new IMChatMessageDetail();
		chatMessageDetail.setCurDate(CCPUtil.getDateCreate());
		chatMessageDetail.setSessionId(sessionId);
		
		chatMessageDetail.setReadStatus(IMChatMessageDetail.STATE_READED);
		chatMessageDetail.setGroupSender(CCPConfig.VoIP_ID);
		
		chatMessageDetail.setImState(imState);
		chatMessageDetail.setMessageType(textOrFile);
		
		return chatMessageDetail;
	}
	/***
	 * 
	 * @param textOrFile
	 * @param sendType
	 * @param contactId
	 * @param contactName
	 * @return
	 */
	public static IMChatMessageDetail getGroupItemMessageReceived(String msgId , int textOrFile ,String sessionId , String sender) {
		IMChatMessageDetail chatMessageDetail = new IMChatMessageDetail();
		if(TextUtils.isEmpty(msgId)) {
			return null;
		}
		chatMessageDetail.setMessageId(msgId);
		chatMessageDetail.setCurDate(CCPUtil.getDateCreate());
		chatMessageDetail.setSessionId(sessionId);
		chatMessageDetail.setReadStatus(IMChatMessageDetail.STATE_UNREAD);
		chatMessageDetail.setGroupSender(sender);
		
		chatMessageDetail.setImState(IMChatMessageDetail.STATE_IM_RECEIVEED);
		chatMessageDetail.setMessageType(textOrFile);
		
		return chatMessageDetail;
	}

}
