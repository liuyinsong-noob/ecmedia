package com.hisun.phone.core.voice.model.chatroom;

import java.util.ArrayList;

import com.hisun.phone.core.voice.model.Response;

public class ChatRoomMemberList extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = -7281148190651477280L;
	public String count;
	public ArrayList<ChatroomMember> chatRoomInfos = new ArrayList<ChatroomMember>();
}
