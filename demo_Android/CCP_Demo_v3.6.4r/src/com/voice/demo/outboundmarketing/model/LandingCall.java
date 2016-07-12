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
package com.voice.demo.outboundmarketing.model;

import com.hisun.phone.core.voice.model.Response;

public class LandingCall extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = 4887509258036503931L;

	public String callSid;
	public String dateCreated;
	
	public String phoneNumber;
	
	public int callStatuImage;
	public String callStatus;
	
	

	public String getCallSid() {
		return callSid;
	}

	public void setCallSid(String callSid) {
		this.callSid = callSid;
	}

	public String getDateCreated() {
		return dateCreated;
	}

	public void setDateCreated(String dateCreated) {
		this.dateCreated = dateCreated;
	}

	public String getPhoneNumber() {
		return phoneNumber;
	}

	public void setPhoneNumber(String phoneNumber) {
		this.phoneNumber = phoneNumber;
	}

	/**
	 * @param callSid
	 * @param dateCreated
	 * @param phoneNumber
	 */
	public LandingCall(String callSid, String dateCreated, String phoneNumber) {
		super();
		this.callSid = callSid;
		this.dateCreated = dateCreated;
		this.phoneNumber = phoneNumber;
	}

	/**
	 * @param callSid
	 * @param phoneNumber
	 */
	public LandingCall(String callSid, String phoneNumber) {
		super();
		this.callSid = callSid;
		this.phoneNumber = phoneNumber;
	}

	/**
	 * @param phoneNumber
	 */
	public LandingCall(String phoneNumber) {
		super();
		this.phoneNumber = phoneNumber;
	}

	
	public int getCallStatuImage() {
		return callStatuImage;
	}

	public void setCallStatuImage(int callStatuImage) {
		this.callStatuImage = callStatuImage;
	}

	public String getCallStatus() {
		return callStatus;
	}

	public void setCallStatus(String callStatus) {
		this.callStatus = callStatus;
	}

	/**
	 * 
	 */
	public LandingCall() {
		super();
	}
	
}
