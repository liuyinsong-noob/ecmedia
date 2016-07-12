package com.hisun.phone.core.voice.model.chatroom;

public class ChatroomRemoveMemberMsg extends ChatroomMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = -4006806827048451136L;

	private String who;

	public String getWho() {
		return who;
	}

	public void setWho(String who) {
		this.who = who;
	}
}
