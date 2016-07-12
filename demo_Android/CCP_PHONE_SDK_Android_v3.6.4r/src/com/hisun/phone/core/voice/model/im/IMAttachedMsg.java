package com.hisun.phone.core.voice.model.im;

public class IMAttachedMsg extends InstanceMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = -343703724607089777L;
	
	private String msgId;
	private String dateCreated;
	private String sender;
	private String receiver;
	private long fileSize;
	private String fileUrl;
	private boolean Chunked;
	private String ext;
	private String userData;
	
	public String getDateCreated() {
		return dateCreated;
	}
	public void setDateCreated(String dateCreated) {
		this.dateCreated = dateCreated;
	}
	public long getFileSize() {
		return fileSize;
	}
	public void setFileSize(long fileSize) {
		this.fileSize = fileSize;
	}
	public String getFileUrl() {
		return fileUrl;
	}
	public void setFileUrl(String fileUrl) {
		this.fileUrl = fileUrl;
	}
	public String getExt() {
		return ext;
	}
	public void setExt(String ext) {
		this.ext = ext;
	}
	public String getSender() {
		return sender;
	}
	public void setSender(String sender) {
		this.sender = sender;
	}
	public String getReceiver() {
		return receiver;
	}
	public void setReceiver(String receiver) {
		this.receiver = receiver;
	}
	public boolean isChunked() {
		return Chunked;
	}
	public void setChunked(boolean chunked) {
		Chunked = chunked;
	}
	public String getUserData() {
		return userData;
	}
	public void setUserData(String userData) {
		this.userData = userData;
	}
	
	public String getMsgId() {
		return msgId;
	}
	public void setMsgId(String msgId) {
		this.msgId = msgId;
	}
	public IMAttachedMsg() {
		super();
	}
	public IMAttachedMsg(String msgId) {
		super();
		this.msgId = msgId;
	}
}
