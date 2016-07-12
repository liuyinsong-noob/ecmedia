/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.cloopen.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
package com.voice.demo.ExConsultation.model;

import com.hisun.phone.core.voice.model.Response;
/**
 * 
 * @author Jorstin Chan
 * @version 3.3
 */
public class ServiceNum extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = -9049660190928929923L;
	
	private String voipphone;
	private String ivrphone;
	public String getVoipphone() {
		return voipphone;
	}
	public void setVoipphone(String voipphone) {
		this.voipphone = voipphone;
	}
	public String getIvrphone() {
		return ivrphone;
	}
	public void setIvrphone(String ivrphone) {
		this.ivrphone = ivrphone;
	}
	

}
