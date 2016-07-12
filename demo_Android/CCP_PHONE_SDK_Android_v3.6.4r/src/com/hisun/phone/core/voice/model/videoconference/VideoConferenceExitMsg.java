package com.hisun.phone.core.voice.model.videoconference;

/**
 * 
* <p>Title: VideoConferenceExitMsg.java</p>
* <p>Description: Notice the client has quit the video conference</p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.cloopen.com</p>
* @author Jorstin Chan
* @date 2013-10-25
* @version 3.5
 */
public class VideoConferenceExitMsg extends VideoConferenceMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = -6306850137326626644L;

	/**
	 * the member that join Video Conference.
	 */
	private String[] whos;

	public String[] getWhos() {
		return whos;
	}

	public void setWhos(String[] whos) {
		this.whos = whos;
	}
}
