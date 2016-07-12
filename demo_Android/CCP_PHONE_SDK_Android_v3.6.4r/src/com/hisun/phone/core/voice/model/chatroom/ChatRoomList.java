package com.hisun.phone.core.voice.model.chatroom;

import java.util.ArrayList;

import com.hisun.phone.core.voice.model.Response;

public class ChatRoomList extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = 5292116657600935765L;
	public String count;
	public ArrayList<Chatroom> chatroomInfos = new ArrayList<Chatroom>();
}
