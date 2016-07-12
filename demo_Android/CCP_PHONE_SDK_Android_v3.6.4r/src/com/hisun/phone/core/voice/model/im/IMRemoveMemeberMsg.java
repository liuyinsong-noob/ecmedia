package com.hisun.phone.core.voice.model.im;

public class IMRemoveMemeberMsg extends InstanceMsg {
	/**
	 * 
	 */
	private static final long serialVersionUID = -5578313942425237207L;
	
	private String groupId = null;
	private String who;

	public String getGroupId() {
		return groupId;
	}

	public void setGroupId(String groupId) {
		this.groupId = groupId;
	}

	/**
	 * @return the who
	 */
	public String getWho() {
		return who;
	}

	/**
	 * @param who the who to set
	 */
	public void setWho(String who) {
		this.who = who;
	}
	
	
}
