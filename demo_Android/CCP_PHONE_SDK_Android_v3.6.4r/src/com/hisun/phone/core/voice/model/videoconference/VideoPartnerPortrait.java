package com.hisun.phone.core.voice.model.videoconference;

/**
 * 
* <p>Title: VideoPartnerPortrait.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-11-6
* @version 13.5
 */
public class VideoPartnerPortrait extends VideoConferenceMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = 9193804072630247154L;

	/**
	 * update date
	 */
	private String dateUpdate;
	
	/**
	 * the voip of this portrait
	 */
	private String voip;
	
	/**
	 * file path in local
	 */
	private String fileName;
	private String fileUrl;
	
	private String fileLocalPath;
	
	
	public String getDateUpdate() {
		return dateUpdate;
	}
	public void setDateUpdate(String dateUpdate) {
		this.dateUpdate = dateUpdate;
	}
	public String getVoip() {
		return voip;
	}
	public void setVoip(String voip) {
		this.voip = voip;
	}
	public String getFileName() {
		return fileName;
	}
	public void setFileName(String fileName) {
		this.fileName = fileName;
	}
	public String getFileUrl() {
		return fileUrl;
	}
	public void setFileUrl(String fileUrl) {
		this.fileUrl = fileUrl;
	}
	public String getFileLocalPath() {
		return fileLocalPath;
	}
	public void setFileLocalPath(String fileLocalPath) {
		this.fileLocalPath = fileLocalPath;
	}
	
	
	
}
