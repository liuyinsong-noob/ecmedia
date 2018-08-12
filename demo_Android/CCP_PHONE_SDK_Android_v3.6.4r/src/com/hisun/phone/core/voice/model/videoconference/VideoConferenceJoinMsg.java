package com.hisun.phone.core.voice.model.videoconference;

/**
 * 
* <p>Title: VideoConferenceJoinMsg.java</p>
* <p>Description: Notice the client who joined the video conference</p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-10-25
* @version 3.5
 */
public class VideoConferenceJoinMsg extends VideoConferenceMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = -6893082978053667836L;

	/**
	 * the member that join Video Conference.
	 */
	private String[] whos;
	
	private int publishStatus;
	private String ip;
	private int port;

	public String[] getWhos() {
		return whos;
	}

	public void setWhos(String[] whos) {
		this.whos = whos;
	}
	
	/**
	 * @return the publishStatus
	 */
	public int getPublishStatus() {
		return publishStatus;
	}
	/**
	 * @param publishStatus the publishStatus to set
	 */
	public void setPublishStatus(int publishStatus) {
		this.publishStatus = publishStatus;
	}
	/**
	 * @return the ip
	 */
	public String getIp() {
		return ip;
	}
	/**
	 * @param ip the ip to set
	 */
	public void setIp(String ip) {
		this.ip = ip;
	}
	/**
	 * @return the port
	 */
	public int getPort() {
		return port;
	}
	/**
	 * @param port the port to set
	 */
	public void setPort(int port) {
		this.port = port;
	}

	
}
