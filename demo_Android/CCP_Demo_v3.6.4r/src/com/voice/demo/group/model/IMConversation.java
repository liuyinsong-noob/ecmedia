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

/**
 * IM message session list (IM point to point, group, system validation message)
 * @version Time: 2013-7-22
 */
public class IMConversation {

	public static final int CONVER_TYPE_MESSAGE = 0x1;
	public static final int CONVER_TYPE_SYSTEM = 0x2;
	
	private String id;
	private String contact;
	private String dateCreated;
	private String unReadNum;
	private String recentMessage;
	private int type;

	public String getId() {
		return id;
	}

	public void setId(String id) {
		this.id = id;
	}

	public String getContact() {
		return contact;
	}

	public void setContact(String contact) {
		this.contact = contact;
	}

	public String getDateCreated() {
		return dateCreated;
	}

	public void setDateCreated(String dateCreated) {
		this.dateCreated = dateCreated;
	}

	public String getUnReadNum() {
		return unReadNum;
	}

	public void setUnReadNum(String unReadNum) {
		this.unReadNum = unReadNum;
	}

	public String getRecentMessage() {
		return recentMessage;
	}

	public void setRecentMessage(String recentMessage) {
		this.recentMessage = recentMessage;
	}

	public int getType() {
		return type;
	}

	public void setType(int type) {
		this.type = type;
	}

	public IMConversation(String id, String contact, String dateCreated,
			String unReadNum, String recentMessage,	int type) {
		super();
		this.id = id;
		this.contact = contact;
		this.dateCreated = dateCreated;
		this.unReadNum = unReadNum;
		this.recentMessage = recentMessage;
		this.type = type;
	}

	public IMConversation() {
		super();
	}

	
}
