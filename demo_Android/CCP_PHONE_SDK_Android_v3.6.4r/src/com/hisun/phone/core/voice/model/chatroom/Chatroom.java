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
 */package com.hisun.phone.core.voice.model.chatroom;

import com.hisun.phone.core.voice.model.Response;


public class Chatroom extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = -5457251963139759939L;

	private String roomNo;
	private String roomName;
	private String creator;
	private String square;
	private String joined;
	private String validate;
	private String keywords;

	/**
	 * 返回聊天室房间号
	 * @return
	 */
	public String getRoomNo() {
		return roomNo;
	}

	public void setRoomNo(String roomNo) {
		this.roomNo = roomNo;
	}

	/**
	 * 返回聊天室名称
	 * @return
	 */
	public String getRoomName() {
		return roomName;
	}

	public void setRoomName(String roomName) {
		this.roomName = roomName;
	}

	/**
	 * 返回聊天室创建者
	 * @return
	 */
	public String getCreator() {
		return creator;
	}

	public void setCreator(String creator) {
		this.creator = creator;
	}

	/**
	 * 返回聊天室最大参与人数
	 * @return
	 */
	public String getSquare() {
		return square;
	}

	public void setSquare(String square) {
		this.square = square;
	}

	/**
	 * 是否已加入聊天室
	 * @return
	 */
	public String getJoined() {
		return joined;
	}

	public void setJoined(String joined) {
		this.joined = joined;
	}

	/**
	 * 聊天室验证权限，即加入是否需要密码验证
	 * @return
	 */
	public String getValidate() {
		return validate;
	}

	public void setValidate(String validate) {
		this.validate = validate;
	}

	/**
	 * 业务扩展参数，平台自定义
	 * @return
	 */
	public String getKeywords() {
		return keywords;
	}

	public void setKeywords(String keywords) {
		this.keywords = keywords;
	}
}
