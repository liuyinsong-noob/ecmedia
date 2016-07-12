package com.hisun.phone.core.voice.model.im;

public class IMTextMsg extends InstanceMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = 8487249406598039970L;
	
	public static final String MESSAGE_REPORT_SEND = "0";             // send message to the server
	public static final String MESSAGE_REPORT_RECEIVE = "1";          // the other has receive message
	public static final String MESSAGE_REPORT_FAILED = "-1";          // send failed

	private String msgId;
	private String dateCreated;
	private String sender;
	private String receiver;
	private String message;
	private String userData;
	private String status = "0";
	
	/**
	 * @param msgId
	 * @param status
	 */
	public IMTextMsg(String msgId, String status) {
		this.msgId = msgId;
		this.status = status;
	}
	
	/**
	 * @param msgId
	 * @param dateCreated
	 * @param sender
	 * @param receiver
	 * @param message
	 * @param status
	 */
	public IMTextMsg(String msgId, String dateCreated, String sender,
			String receiver, String message, String status) {
		this.msgId = msgId;
		this.dateCreated = dateCreated;
		this.sender = sender;
		this.receiver = receiver;
		this.message = message;
		this.status = status;
	}



	public IMTextMsg() {
	}

	public String getDateCreated() {
		return dateCreated;
	}
	public void setDateCreated(String dateCreated) {
		this.dateCreated = dateCreated;
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
	public String getMessage() {
		return message;
	}
	public void setMessage(String message) {
		this.message = message;
	}

	/**
	 * @return the status
	 */
	public String getStatus() {
		return status;
	}

	/**
	 * @param status the status to set
	 */
	public void setStatus(String status) {
		this.status = status;
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
	
}
