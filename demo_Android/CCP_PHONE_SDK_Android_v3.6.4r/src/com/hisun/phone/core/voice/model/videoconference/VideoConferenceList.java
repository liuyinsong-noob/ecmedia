package com.hisun.phone.core.voice.model.videoconference;

import java.util.ArrayList;

import com.hisun.phone.core.voice.model.Response;

public class VideoConferenceList extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = -8421980958447021103L;
	public String count;
	public ArrayList<VideoConference> videoConferences = new ArrayList<VideoConference>();
}
