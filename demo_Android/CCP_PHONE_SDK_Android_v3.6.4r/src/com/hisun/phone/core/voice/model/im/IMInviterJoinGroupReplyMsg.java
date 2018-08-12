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
package com.hisun.phone.core.voice.model.im;

/**
 * <p>Title: IMInviterJoinGroupReplyMsg.java</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2014</p>
 * <p>Company: Beijing Speedtong Information Technology Co.,Ltd</p>
 * @author Jorstin Chan
 * @date 2014-10-16
 * @version 1.0
 */
public class IMInviterJoinGroupReplyMsg extends InstanceMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	private String groupId;
	private String admin;
	private String declared;
	private String confirm;
	private String member;
	/**
	 * @return the groupId
	 */
	public String getGroupId() {
		return groupId;
	}
	/**
	 * @param groupId the groupId to set
	 */
	public void setGroupId(String groupId) {
		this.groupId = groupId;
	}
	/**
	 * @return the admin
	 */
	public String getAdmin() {
		return admin;
	}
	/**
	 * @param admin the admin to set
	 */
	public void setAdmin(String admin) {
		this.admin = admin;
	}
	/**
	 * @return the confirm
	 */
	public String getConfirm() {
		return confirm;
	}
	/**
	 * @param confirm the confirm to set
	 */
	public void setConfirm(String confirm) {
		this.confirm = confirm;
	}
	/**
	 * @return the declared
	 */
	public String getDeclared() {
		return declared;
	}
	/**
	 * @param declared the declared to set
	 */
	public void setDeclared(String declared) {
		this.declared = declared;
	}
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

}
