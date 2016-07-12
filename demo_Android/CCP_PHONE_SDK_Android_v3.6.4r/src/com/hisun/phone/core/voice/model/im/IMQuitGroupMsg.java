package com.hisun.phone.core.voice.model.im;

public class IMQuitGroupMsg extends InstanceMsg {

	
	/**
	 * 
	 */
	private static final long serialVersionUID = -3878822039584051643L;
	private String groupId = null;
	private String member = null;

	public String getGroupId() {
		return groupId;
	}

	public void setGroupId(String groupId) {
		this.groupId = groupId;
	}

	public String getMember() {
		return member;
	}

	public void setMember(String member) {
		this.member = member;
	}
}
