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
 */
package com.voice.demo.ui;

import java.util.ArrayList;

import android.content.Intent;
import android.os.Bundle;
import android.os.Message;
import android.text.TextUtils;
import android.view.*;
import android.widget.Toast;

import com.hisun.phone.core.voice.model.setup.SubAccount;
import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.R;
import com.voice.demo.contacts.T9Service;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.net.*;
import com.voice.demo.ui.experience.ExperienceLogin;
import com.voice.demo.ui.model.Application;
import com.voice.demo.ui.model.DemoAccounts;

/**
 * 
 * @ClassName: LoginUIActivity.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-6
 * @version 3.6
 */
public class LoginUIActivity extends CCPBaseActivity implements View.OnClickListener
																	,LoginRestHelper.OnRestManagerHelpListener
																	,CCPHelper.RegistCallBack{
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
	}

	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPBaseActivity#getLayoutId()
	 */
	@Override
	protected int getLayoutId() {
		return -1;
	}
	
	@Override
	protected void onStart() {
		LoginRestHelper.getInstance().setOnRestManagerHelpListener(this);
		super.onStart();
	}
	
	/* (non-Javadoc)
	 * @see android.view.View.OnClickListener#onClick(android.view.View)
	 */
	@Override
	public void onClick(View v) {
		
	}
	
	public final Message getHandleMessage() {
		// For each start request, send a message to start a job and deliver the
		// start ID so we know which request we're stopping when we finish the
		// job
		Message msg = getBaseHandle().obtainMessage();
		return msg;
	}

	/* (non-Javadoc)
	 * @see com.voice.demo.tools.net.BaseRestHelper.OnRestManagerHelpListener#ooClientLoginRequest(com.voice.demo.outboundmarketing.RestHelper.ERequestState, com.voice.demo.ui.model.DemoAccounts)
	 */
	@Override
	public void onClientLoginRequest(DemoAccounts demoAccounts) {
	
		storageAccountInformation(demoAccounts);
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("demoAccounts", demoAccounts);
	    msg.obj = b;
	    msg.arg1 = LoginRestHelper.REST_CLIENT_LOGIN;
	    sendHandleMessage(msg);
	}
	
	/* (non-Javadoc)
	 * @see com.voice.demo.tools.net.BaseRestHelper.OnRestManagerHelpListener#onTestNumber(com.voice.demo.outboundmarketing.RestHelper.ERequestState)
	 */
	@Override
	public void onTestNumber(String number) {
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putString("number", number);
	    msg.obj = b;
	    msg.arg1 = LoginRestHelper.REST_BUILD_TEST_NUMBER;
	    sendHandleMessage(msg);
	}
	
	@Override
	public void onRequestFailed(int requestType , int errorCode, String errorMessage) {
		
		closeConnectionProgress();
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putInt("requestType", requestType);
	    b.putInt("errorCode", errorCode);
	    b.putString("errorMessage", errorMessage);
	    msg.obj = b;
	    msg.arg1 = TaskKey.TASK_KEY_REQUEST_FAILED;
	    sendHandleMessage(msg);
	}
	
	public final void sendHandleMessage(Message msg) {
		getBaseHandle().sendMessage(msg);
	}
	
	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPBaseActivity#handleNotifyMessage(android.os.Message)
	 */
	@Override
	protected void handleNotifyMessage(Message msg) {
		
		// Normally we would do some work here.
		Bundle b = (Bundle) msg.obj;
		int what = msg.arg1;
		Log4Util.i(CCPHelper.DEMO_TAG, "What: " + what);
		
		switch (what) {
		case LoginRestHelper.REST_CLIENT_LOGIN:
			handleClientLoginRequest();
			break;
			
		case LoginRestHelper.REST_BUILD_TEST_NUMBER:
			String number = b.getString("number");
			handleTestNumber(number);
			break;
			
		case TaskKey.TASK_KEY_REQUEST_FAILED:
			int requestType =  b.getInt("requestType");
			int errorCode =  b.getInt("errorCode");
			String errorMessage = b.getString("errorMessage");
			
			handleRequestFailed(requestType, errorCode, errorMessage);
			break;
		default:
			
			super.handleNotifyMessage(msg);
			break;
		}
			
		
	}
	
	/**
	 * 
	 * @param reason
	 * @param demoAccounts
	 */
	protected void handleClientLoginRequest() {
		
	}
	
	/**
	 * 
	 * @param reason
	 */
	protected void handleTestNumber(String number) {
		
	}

	
	protected void handleRequestFailed(int requestType , int errorCode, String errorMessage) {
		closeConnectionProgress();
		Toast.makeText(getApplicationContext(), errorMessage + "(" + errorCode + ")", Toast.LENGTH_LONG).show();
	}
	
	
	// ---------------------------------------
	
	/**
	 * 
	 * @Title: showInitErrDialog 
	 * @Description: TODO 
	 * @param @param reason 
	 * @return void 
	 * @throws
	 */
	void showInitErrToast(String reason) {
		String message = null ;
		if(TextUtils.isEmpty(reason)) {
			message = getString(R.string.str_dialog_init_error_message);
		} else {
			message = getString(R.string.str_dialog_init_error_message) + "(" +reason+ ")";
		}
		
		Toast.makeText(getApplicationContext(), message, Toast.LENGTH_LONG).show();
	}
	
	
	
	/**
	 * 
	 */
	public void doSDKRegist() {
		showConnectionProgress(getString(R.string.dialog_message_text));
		ITask iTask = new ITask(TaskKey.KEY_SDK_REGIST);
		addTask(iTask);
	}
	
	public void doExperienceAutoLogin(String userName , String userPass) {
		ITask iTask = new ITask(LoginRestHelper.REST_CLIENT_LOGIN);
		iTask.setTaskParameters("userName", userName);
		iTask.setTaskParameters("userPass", userPass);
		addTask(iTask);
	}
	/**
	 * @param demoAccounts
	 */
	public boolean storageAccountInformation(DemoAccounts demoAccounts) {
		
		if(demoAccounts == null) {
			return false;
		}
		ArrayList<Application> applications = demoAccounts.getApplications();
		if(applications.isEmpty()) {
			return false;
		}
		
		StringBuffer subAccounts = new StringBuffer();
		StringBuffer subAccountTokens = new StringBuffer();
		StringBuffer voipSids = new StringBuffer();
		StringBuffer voipSidTokens = new StringBuffer();
		for(SubAccount subAccount : applications.get(0).getSubAccounts()) {
			subAccounts.append(subAccount.accountSid).append(",");
			subAccountTokens.append(subAccount.authToken).append(",");
			voipSids.append(subAccount.sipCode).append(",");
			voipSidTokens.append(subAccount.sipPwd).append(",");
		}
		
		if(subAccounts == null ||subAccountTokens == null ||voipSids == null ||voipSidTokens == null) {
			return false;
		}
		CCPConfig.Sub_Account_LIST = subAccounts.substring(0, subAccounts.length() - 1).toString();;
		CCPConfig.Sub_Token_LIST = subAccountTokens.substring(0, subAccountTokens.length() - 1).toString();;
		CCPConfig.VoIP_ID_LIST = voipSids.substring(0, voipSids.length() - 1).toString();;
		CCPConfig.VoIP_PWD_LIST = voipSidTokens.substring(0, voipSidTokens.length() - 1).toString();;
		
		CCPConfig.App_ID = applications.get(0).getAppId();
		CCPConfig.Main_Account = demoAccounts.getMainAccount();
		CCPConfig.Main_Token = demoAccounts.getMainToken();
		
		CCPConfig.friendlyName = applications.get(0).getFriendlyName();
		CCPConfig.mobile = demoAccounts.getMobile();
		CCPConfig.nickname = demoAccounts.getNickname();
		CCPConfig.test_number = demoAccounts.getTestNumber();
		
		return true;
	}
	
	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPBaseActivity#handleTaskBackGround(com.voice.demo.tools.net.ITask)
	 */
	@Override
	protected void handleTaskBackGround(ITask iTask) {

		if(iTask.getKey() == TaskKey.KEY_SDK_REGIST) {
			
			CCPHelper.getInstance().registerCCP(this);
		} else if(iTask.getKey() == LoginRestHelper.REST_CLIENT_LOGIN) {
			
			String userName = (String) iTask.getTaskParameters("userName");
			String userPass = (String) iTask.getTaskParameters("userPass");
			// 
			LoginRestHelper.getInstance().doClientLoginRequest(userName, userPass);
		}else {
			
			super.handleTaskBackGround(iTask);
		}
	}
	
	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPBaseActivity#checkeDeviceHelper()
	 */
	@Override
	protected boolean checkeDeviceHelper() {
		return true;
	}

	/**
	 * 
	 * @Title: startAction 
	 * @Description: TODO 
	 * @param  
	 * @return void 
	 * @throws
	 */
	protected void startAction() {}

	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPHelper.RegistCallBack#onRegistResult(int)
	 */
	@Override
	public void onRegistResult(final int reason , final String msg) {
		runOnUiThread(new Runnable() {
			
			@Override
			public void run() {
				try {
					closeConnectionProgress();
					if (reason == CCPHelper.WHAT_ON_CONNECT) {
						
						Intent startService = new Intent(LoginUIActivity.this, T9Service.class);
						startService(startService);
						
						startAction();
					} else if (reason == CCPHelper.WHAT_ON_DISCONNECT || reason == CCPHelper.WHAT_INIT_ERROR) {
						// do nothing ...
						showInitErrToast(msg);
					} else {
						Log4Util.d(CCPHelper.DEMO_TAG , "Sorry , can't handle a message " + msg);
					}
				} catch (Exception e) {
					e.printStackTrace();
				}
				CCPHelper.getInstance().setRegistCallback(null);
			}
		});
	}
	
	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPBaseActivity#addTask(com.voice.demo.tools.net.ITask)
	 */
	@Override
	public void addTask(ITask iTask) {
		
		LoginRestHelper.getInstance().setOnRestManagerHelpListener(this);
		super.addTask(iTask);
	}
	
	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPBaseActivity#onDestroy()
	 */
	@Override
	protected void onDestroy() {
		LoginRestHelper.getInstance().setOnRestManagerHelpListener(null);
		super.onDestroy();
	}
}
