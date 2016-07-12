package com.hisun.phone.core.voice.model.im;

public class IMCooperMsg extends IMAttachedMsg {

	/**
	 * 
	 */
	private static final long serialVersionUID = -4274381207236112776L;

	private String message;

	private int type;
	
	public String getMessage() {
		return message;
	}

	public void setMessage(String message) {
		this.message = message;
	}

	public int getType() {
		return type;
	}

	public void setType(int type) {
		this.type = type;
	}
}
