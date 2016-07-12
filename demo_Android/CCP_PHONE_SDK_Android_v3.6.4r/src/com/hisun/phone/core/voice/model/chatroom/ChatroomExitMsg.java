package com.hisun.phone.core.voice.model.chatroom;

//有人退出了会议
public class ChatroomExitMsg extends ChatroomMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = 281442913150110112L;
	private String[] whos;
	public String[] getWhos() {
		return whos;
	}
	public void setWhos(String[] whos) {
		this.whos = whos;
	}
}
