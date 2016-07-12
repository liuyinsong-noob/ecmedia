package com.hisun.phone.core.voice.model.chatroom;

import com.hisun.phone.core.voice.model.Response;

public class ChatroomMsg extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = -1968688576159504448L;

	private String roomNo;
	
	public String getRoomNo() {
		return roomNo;
	}
	public void setRoomNo(String roomNo) {
		this.roomNo = roomNo;
	}
	public ChatroomMsg(String roomNo) {
		super();
		this.roomNo = roomNo;
	}
	public ChatroomMsg() {
		super();
	}
	
	public static class ForbidOptions {
		
		public static final int OPTION_SPEAK_LIMIT = 0;
		public static final int OPTION_SPEAK_FREE = 1;
		public static final int OPTION_LISTEN_LIMIT = 0;
		public static final int OPTION_LISTEN_FREE = 1;
		
		public ForbidOptions(int inSpeak , int inListen) {
			this.inSpeak = inSpeak;
			this.inListen = inListen;
		}
		
		public int inSpeak;
		public int inListen;
	}
}
