package com.hisun.phone.core.voice.model.chatroom;

//有人加入了会议
public class ChatroomJoinMsg extends ChatroomMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = 684999052907717528L;
	private String[] whos;
	public String[] getWhos() {
		return whos;
	}
	public void setWhos(String[] whos) {
		this.whos = whos;
	}
	
	
}
