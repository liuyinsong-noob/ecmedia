package com.hisun.phone.core.voice.model.im;

public class IMInviterMsg extends InstanceMsg {
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 5228186818015525906L;
	
	private String groupId = null;
	private String admin = null;
	private String declared = null;
	private String confirm = null;
	private String dateCreated = null;
	public String getDateCreated() {
		return dateCreated;
	}
	public void setDateCreated(String dateCreated) {
		this.dateCreated = dateCreated;
	}
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
	public String getDeclared() {
		return declared;
	}
	public void setDeclared(String declared) {
		this.declared = declared;
	}
	public String getConfirm() {
		return confirm;
	}
	public void setConfirm(String confirm) {
		this.confirm = confirm;
	}
}
