/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.yuntongxun.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */package com.voice.demo.ui.model;

import java.util.ArrayList;

import com.hisun.phone.core.voice.model.Response;
import com.hisun.phone.core.voice.model.setup.SubAccount;

/**
 * 
 * 
 * @ClassName: Account.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-5
 * @version 3.6
 */
public class Application extends Response{

	/**
	 * 
	 */
	private static final long serialVersionUID = 3911821086406316831L;

	private String appId;
	
	private String friendlyName;

	private ArrayList<SubAccount> subAccounts = new ArrayList<SubAccount>();

	public String getAppId() {
		return appId;
	}

	public void setAppId(String appId) {
		this.appId = appId;
	}

	public ArrayList<SubAccount> getSubAccounts() {
		return subAccounts;
	}

	public void putSubAccount(SubAccount subAccount) {
		subAccounts.add(subAccount);
	}

	public String getFriendlyName() {
		return friendlyName;
	}

	public void setFriendlyName(String friendlyName) {
		this.friendlyName = friendlyName;
	}
	
	
}
