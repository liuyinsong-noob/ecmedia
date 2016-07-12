package com.hisun.phone.core.voice.model.videoconference;

/**
 * 
* <p>Title: VideoConferenceMember.java</p>
* <p>Description: Video Conference member</p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.cloopen.com</p>
* @author Jorstin Chan
* @date 2013-10-25
* @version 3.5
 */
public class VideoConferenceMember extends VideoConferenceMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = -2792598311852403472L;

	private int type;
	private String number;
	
	/**
	 * if the main screen
	 */
	private int screen;
	
	private int publishStatus;
	private String ip;
	private int port;
	
	public int getType() {
		return type;
	}
	public void setType(int type) {
		this.type = type;
	}
	public String getNumber() {
		return number;
	}
	public void setNumber(String number) {
		this.number = number;
	}
	public int getScreen() {
		return screen;
	}
	public void setScreen(int screen) {
		this.screen = screen;
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
