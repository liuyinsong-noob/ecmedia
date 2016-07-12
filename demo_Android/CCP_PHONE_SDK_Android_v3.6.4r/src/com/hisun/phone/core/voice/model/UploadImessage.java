package com.hisun.phone.core.voice.model;

import com.hisun.phone.core.voice.model.im.IMAttachedMsg;

/**
 * @version Time: 2013-6-15
 */
public class UploadImessage extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = -8924579215553361322L;

	/**
	 * 
	 */

	public IMAttachedMsg mediaMsg;
	/**
	 * 
	 */
	private String uploadUrl;

	/**
	 * 
	 */
	private String uploadToken;

	public UploadImessage(String uploadUrl, String uploadToken) {
		super();
		this.uploadUrl = uploadUrl;
		this.uploadToken = uploadToken;
	}

	public UploadImessage() {
		super();
	}

	public String getUploadUrl() {
		return uploadUrl;
	}

	public void setUploadUrl(String uploadUrl) {
		this.uploadUrl = uploadUrl;
	}

	public String getUploadToken() {
		return uploadToken;
	}

	public void setUploadToken(String uploadToken) {
		this.uploadToken = uploadToken;
	}

}
