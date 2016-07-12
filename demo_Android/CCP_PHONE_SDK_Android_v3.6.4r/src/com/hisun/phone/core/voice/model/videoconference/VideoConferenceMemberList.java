package com.hisun.phone.core.voice.model.videoconference;

import java.util.ArrayList;

import com.hisun.phone.core.voice.model.Response;

public class VideoConferenceMemberList extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = 4189215648065194045L;
	public String count;
	public ArrayList<VideoConferenceMember> videoConferenceMembers = new ArrayList<VideoConferenceMember>();
}
