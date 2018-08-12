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

public class IMSession {
	
	private int id;
	private String recipient;
	private String lastMessage;
	private String lastDate;
	private String msgCount;
	private int msgType;

	public int getMsgType() {
		return this.msgType;
	}

	public void setMsgType(int msgType) {
		this.msgType = msgType;
	}

	public int getId() {
		return this.id;
	}

	public void setId(int id) {
		this.id = id;
	}

	public String getRecipient() {
		return this.recipient;
	}

	public void setRecipient(String recipient) {
		this.recipient = recipient;
	}

	public String getLastMessage() {
		return this.lastMessage;
	}

	public void setLastMessage(String lastMessage) {
		this.lastMessage = lastMessage;
	}

	public String getLastDate() {
		return this.lastDate;
	}

	public void setLastDate(String lastDate) {
		this.lastDate = lastDate;
	}

	public String getMsgCount() {
		return this.msgCount;
	}

	public void setMsgCount(String msgCount) {
		this.msgCount = msgCount;
	}

	public IMSession(int id, String recipient, String lastMessage,
			String lastDate, String msgCount) {
		this.id = id;
		this.recipient = recipient;
		this.lastMessage = lastMessage;
		this.lastDate = lastDate;
		this.msgCount = msgCount;
	}

	public IMSession() {
	}
}