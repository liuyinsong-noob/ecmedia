package com.hisun.phone.core.voice.model.im;

public class IMReplyJoinGroupMsg extends InstanceMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = -6942662618942546114L;

	private String groupId;
	private String admin;
	private String confirm;
	private String member;
	public String getGroupId() {
		return groupId;
	}
	public void setGroupId(String groupId) {
		this.groupId = groupId;
	}
	public String getAdmin() {
		return admin;
	}
	public void setAdmin(String admin) {
		this.admin = admin;
	}
	public String getConfirm() {
		return confirm;
	}
	public void setConfirm(String confirm) {
		this.confirm = confirm;
	}
	/**
	 * @return the who
	 */
	public String getMember() {
		return member;
	}

	/**
	 * @param who the who to set
	 */
	public void setMember(String member) {
		this.member = member;
	}
	
}
