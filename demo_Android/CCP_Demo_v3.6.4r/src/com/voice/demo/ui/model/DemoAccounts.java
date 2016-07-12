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
package com.voice.demo.ui.model;

import java.util.ArrayList;

import com.hisun.phone.core.voice.model.Response;

/**
 * 
 * @ClassName: DemoAccounts.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-6
 * @version 3.6
 */
public class DemoAccounts extends Response{

	/**
	 * 
	 */
	private static final long serialVersionUID = -6375166184187855844L;
	
	/**
	 * 
	 */
	private String mainAccount;
	
	/**
	 * 
	 */
	private String mainToken;
	
	/**
	 * 
	 */
	private String mobile;
	
	/**
	 * 
	 */
	private String testNumber;
	
	private String nickname;
	
	/**
	 * 
	 */
	private ArrayList<Application> applications = new ArrayList<Application>();

	public String getMainAccount() {
		return mainAccount;
	}

	public void setMainAccount(String mainAccount) {
		this.mainAccount = mainAccount;
	}

	public String getMainToken() {
		return mainToken;
	}

	public void setMainToken(String mainToken) {
		this.mainToken = mainToken;
	}

	public String getMobile() {
		return mobile;
	}

	public void setMobile(String mobile) {
		this.mobile = mobile;
	}

	public String getTestNumber() {
		return testNumber;
	}

	public void setTestNumber(String testNumber) {
		this.testNumber = testNumber;
	}
	

	public String getNickname() {
		return nickname;
	}

	public void setNickname(String nickname) {
		this.nickname = nickname;
	}

	public ArrayList<Application> getApplications() {
		return applications;
	}

	public void putApplication(Application application) {
		applications.add(application);
	}
}
