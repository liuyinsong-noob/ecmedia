package com.hisun.phone.core.voice.model.videoconference;

import com.hisun.phone.core.voice.model.Response;

/**
 * 
* <p>Title: VideoConferenceMsg.java</p>
* <p>Description: Inform the client receives a new video conference information</p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-10-25
* @version 3.5
 */
public class VideoConferenceMsg  extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = 8456156900043769586L;
	
	/**
	 * the Video Conference NO.id
	 */
	private String conferenceId;

	public String getConferenceId() {
		return conferenceId;
	}

	public void setConferenceId(String conferenceId) {
		this.conferenceId = conferenceId;
	}
	
	
}
