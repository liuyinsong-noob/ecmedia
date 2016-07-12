package com.hisun.phone.core.voice.model.im;

import java.util.ArrayList;

import com.hisun.phone.core.voice.model.Response;

public class NewMediaMsgList extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = 5289981169923558695L;

	
	public String count;

	public ArrayList<InstanceMsg> newMsgs = new ArrayList<InstanceMsg>();
}
