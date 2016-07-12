package com.hisun.phone.core.voice.model.im;

public class IMProposerMsg extends InstanceMsg {
	/**
	 * 
	 */
	private static final long serialVersionUID = -7788693701183913765L;
	
	private String groupId = null;
	private String proposer = null;
	private String declared = null;
	private String dateCreated = null;
	
	public String getGroupId() {
		return groupId;
	}
	public void setGroupId(String groupId) {
		this.groupId = groupId;
	}
	public String getProposer() {
		return proposer;
	}
	public void setProposer(String proposer) {
		this.proposer = proposer;
	}
	public String getDeclared() {
		return declared;
	}
	public void setDeclared(String declared) {
		this.declared = declared;
	}
	public String getDateCreated() {
		return dateCreated;
	}
	public void setDateCreated(String dateCreated) {
		this.dateCreated = dateCreated;
	}
}
