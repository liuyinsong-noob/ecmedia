package com.hisun.phone.core.voice.model;

import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMsg;

@Deprecated
public class LocalPortrait extends VideoConferenceMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = -3293545217920369439L;

	private int width;
	private int height;
	private String callId;
	
	/**
	 * byte length
	 */
	private int size;
	
	private byte[] bytes;
	
	public int getWidth() {
		return width;
	}
	public void setWidth(int width) {
		this.width = width;
	}
	public int getHeight() {
		return height;
	}
	public void setHeight(int height) {
		this.height = height;
	}
	public String getCallId() {
		return callId;
	}
	public void setCallId(String callId) {
		this.callId = callId;
	}
	public byte[] getBytes() {
		return bytes;
	}
	public void setBytes(byte[] bytes) {
		this.bytes = bytes;
	}
	public int getSize() {
		return size;
	}
	public void setSize(int size) {
		this.size = size;
	}
	
	
}
