package com.hisun.phone.core.voice.model.chatroom;

import com.hisun.phone.core.voice.model.Response;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMsg.ForbidOptions;

public class ChatroomMember extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = -7825999635909992812L;

	private String number;
	private String type;
	private ForbidOptions options;

	public ChatroomMember(String number, String type) {
		super();
		this.number = number;
		this.type = type;
		options = new ForbidOptions(ForbidOptions.OPTION_LISTEN_FREE, ForbidOptions.OPTION_SPEAK_FREE);
	}

	public ChatroomMember() {
		this(null, null);
	}

	public String getNumber() {
		return number;
	}

	public void setNumber(String number) {
		this.number = number;
	}

	public String getType() {
		return type;
	}

	public void setType(String type) {
		this.type = type;
	}
	
	/**
	 * @return the options
	 */
	public ForbidOptions getOptions() {
		return options;
	}
	/**
	 * @param options the options to set
	 */
	public void setOptions(ForbidOptions options) {
		this.options = options;
	}

}
