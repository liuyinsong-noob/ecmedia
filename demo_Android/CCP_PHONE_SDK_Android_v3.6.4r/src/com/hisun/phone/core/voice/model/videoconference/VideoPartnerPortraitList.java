package com.hisun.phone.core.voice.model.videoconference;

import java.util.ArrayList;

import com.hisun.phone.core.voice.model.Response;

public class VideoPartnerPortraitList extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = -5343696685686588861L;

	public String count;
	public ArrayList<VideoPartnerPortrait> videoPartnerPortraits = new ArrayList<VideoPartnerPortrait>();
}
