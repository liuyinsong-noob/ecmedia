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
package com.hisun.phone.core.voice.model.chatroom;


/**
 * <p>Title: ChatroomMemberSpeakOpt.java</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2014</p>
 * <p>Company: Beijing Speedtong Information Technology Co.,Ltd</p>
 * @author Jorstin Chan
 * @date 2014-8-2
 * @version 1.0
 */
public class ChatroomMemberForbidOpt extends ChatroomMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = -6887888612242890196L;

	private String member;
	private ForbidOptions options;
	/**
	 * @return the member
	 */
	public String getMember() {
		return member;
	}
	/**
	 * @param member the member to set
	 */
	public void setMember(String member) {
		this.member = member;
	}
	
	/**
	 * @return the options
	 */
	public ForbidOptions getOptions() {
		return options;
	}
	/**
	 * @param options the options to set
	 */
	public void setOptions(ForbidOptions options) {
		this.options = options;
	}
}
