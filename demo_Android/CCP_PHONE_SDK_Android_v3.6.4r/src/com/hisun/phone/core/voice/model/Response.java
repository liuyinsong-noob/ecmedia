/**
 * 
 */
package com.hisun.phone.core.voice.model;

import java.io.Serializable;

import android.text.TextUtils;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.util.Log4Util;

public class Response implements Serializable {

	/**
	 * 
	 */
	private static final long serialVersionUID = -3974808563240398663L;
	public String statusCode;
	
	/**
	 * request error message .
	 */
	public String statusMsg;

	public void released() {
		statusCode = null;
		statusMsg = null;
	}

	public boolean isError() {
		try {
			Integer valueOf = Integer.valueOf(this.statusCode);
			return valueOf != 0;
		} catch (Exception e) {
			return true;
		}
	}
	
	/**
	 * @return The request failed with error reason
	 */
	public String getStatusMsg() {
		return statusMsg;
	}

	public void print() {
		if (isError() && !TextUtils.isEmpty(statusCode)) {
			Log4Util.e(Device.TAG, "The request failed , Cause of failed :" + this.statusMsg  + "(" + statusCode  + ")");
		}
	}
}
