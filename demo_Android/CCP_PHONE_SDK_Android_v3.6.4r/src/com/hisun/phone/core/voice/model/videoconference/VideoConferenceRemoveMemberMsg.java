package com.hisun.phone.core.voice.model.videoconference;

/**
 * 
* <p>Title: VideoConferenceRemoveMemberMsg.java</p>
* <p>Description: Notice the client had been removed from the video conference</p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-10-25
* @version 3.5
 */
public class VideoConferenceRemoveMemberMsg extends VideoConferenceMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = -5832028199004667430L;

	/**
	 * Removed the video conference member number
	 */
	private String who;

	public String getWho() {
		return who;
	}

	public void setWho(String who) {
		this.who = who;
	}
	
}
