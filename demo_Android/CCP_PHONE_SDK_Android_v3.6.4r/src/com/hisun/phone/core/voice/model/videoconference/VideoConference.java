package com.hisun.phone.core.voice.model.videoconference;

import com.hisun.phone.core.voice.model.Response;

/**
 * 
* <p>Title: VideoConference.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-10-25
* @version 3.5
 */
public class VideoConference extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	/**
	 * the Video Conference No.id
	 */
	private String conferenceId;
	
	/**
	 * the Video Conference name
	 */
	private String conferenceName;
	
	/**
	 * Video conference Creator
	 */
	private String creator;
	
	/**
	 * 
	 */
	private String square;
	
	/**
	 * 
	 */
	private String keywords;
	
	/**
	 * 
	 */
	private String joinNum;
	
	/**
	 * 
	 */
	private String validate;
	
	private boolean isMultiVideo;
	
	
	public String getConferenceId() {
		return conferenceId;
	}
	public void setConferenceId(String conferenceId) {
		this.conferenceId = conferenceId;
	}
	public String getConferenceName() {
		return conferenceName;
	}
	public void setConferenceName(String conferenceName) {
		this.conferenceName = conferenceName;
	}
	public String getCreator() {
		return creator;
	}
	public void setCreator(String creator) {
		this.creator = creator;
	}
	public String getSquare() {
		return square;
	}
	public void setSquare(String square) {
		this.square = square;
	}
	public String getKeywords() {
		return keywords;
	}
	public void setKeywords(String keywords) {
		this.keywords = keywords;
	}
	public String getJoinNum() {
		return joinNum;
	}
	public void setJoinNum(String joinNum) {
		this.joinNum = joinNum;
	}
	public String getValidate() {
		return validate;
	}
	public void setValidate(String validate) {
		this.validate = validate;
	}
	/**
	 * @return the isMultiVideo
	 */
	public boolean isMultiVideo() {
		return isMultiVideo;
	}
	/**
	 * @param isMultiVideo the isMultiVideo to set
	 */
	public void setMultiVideo(boolean isMultiVideo) {
		this.isMultiVideo = isMultiVideo;
	}
	
	
	

}
