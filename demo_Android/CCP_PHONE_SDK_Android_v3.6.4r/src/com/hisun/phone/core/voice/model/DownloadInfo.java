package com.hisun.phone.core.voice.model;

/**
 * 下载语音文件信息
 * 
 */
public class DownloadInfo extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = -3708020704255889529L;
	
	/**
	 * 网络文件地址
	 */
	private String uploadurl;

	/**
	 * 本地存储地址
	 */
	private String fileName;
	
	private String voiceMessageSid;
	
	private String savePath;
	
	private boolean Chunked;

	public boolean isChunked() {
		return Chunked;
	}

	public void setChunked(boolean chunked) {
		Chunked = chunked;
	}

	public String getSavePath() {
		return savePath;
	}

	public void setSavePath(String savePath) {
		this.savePath = savePath;
	}

	public String getVoiceMessageSid() {
		return voiceMessageSid;
	}

	public void setVoiceMessageSid(String voiceMessageSid) {
		this.voiceMessageSid = voiceMessageSid;
	}
	
	
	public String getUploadurl() {
		return uploadurl;
	}

	public void setUploadurl(String uploadurl) {
		this.uploadurl = uploadurl;
	}

	public String getFileName() {
		return fileName;
	}

	public void setFileName(String fileName) {
		this.fileName = fileName;
	}

	public DownloadInfo() {
		super();
	}
	
	

	/**
	 * 
	 * @param uploadurl
	 * @param savePath
	 */
	public DownloadInfo(String uploadurl, String savePath , boolean isChunked) {
		super();
		this.uploadurl = uploadurl;
		this.savePath = savePath;
		this.Chunked = isChunked;
	}
	
	public DownloadInfo(String uploadurl, String fileName,
			String voiceMessageSid, String savePath) {
		super();
		this.uploadurl = uploadurl;
		this.fileName = fileName;
		this.voiceMessageSid = voiceMessageSid;
		this.savePath = savePath;
	}

	public DownloadInfo(String uploadurl, String fileName,
			String voiceMessageSid, String savePath, boolean isChunked) {
		super();
		this.uploadurl = uploadurl;
		this.fileName = fileName;
		this.voiceMessageSid = voiceMessageSid;
		this.savePath = savePath;
		this.Chunked = isChunked;
	}

}
