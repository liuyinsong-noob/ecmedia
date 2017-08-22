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
 */package com.voice.demo.ui.developer;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.R;
import com.voice.demo.CCPApplication;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPDrawableUtils;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.ui.CCPHelper;
import com.voice.demo.ui.CapabilityChoiceActivity;
import com.voice.demo.ui.LoginUIActivity;
import com.voice.demo.views.CCPButton;
import com.voice.demo.views.CCPTitleViewBase;

/**
 * 
 * @ClassName: DeveloperLoginActivity 
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-4
 *
 */
public class DeveloperLoginActivity extends LoginUIActivity  implements View.OnClickListener{
	

	CCPTitleViewBase mCcpTitleViewBase;
	CCPButton mCcpImaButton;

	private TextView mSubsidTextView;

	private TextView mVoipToken;
	private TextView mSubAccount;
	private TextView mSubToken;
	private EditText mServerIPEditText;
	private EditText mPassWDEditText;
	private EditText mServerPortEditText;

	private String[] mVoipArray;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		//
		CCPApplication.getInstance().setDeveloperMode(true);
		initLayoutTitleBar();

		initLoginLayout();

		initViewUI();

		CCPConfig.initProperties(getApplicationContext());

		initConfigInfomation();
	}


	private void initLayoutTitleBar() {
		mCcpTitleViewBase = new CCPTitleViewBase(this);
		mCcpTitleViewBase.setCCPTitleBackground(R.drawable.video_title_bg);
		mCcpTitleViewBase.setCCPTitleViewText(getString(R.string.app_title_developer_login));
		//mCcpTitleViewBase.setCCPBackImageButton(R.drawable.video_back_button_selector , this).setBackgroundDrawable(null);
	}



	/**
	 *
	 * @Title: initLoginLayout
	 * @Description: TODO
	 * @param
	 * @return void
	 * @throws
	 */
	private void initLoginLayout() {

		mSubsidTextView = (TextView) findViewById(R.id.sub_sid);

		//update subaccount info ..
		mSubAccount = (TextView) findViewById(R.id.sub_account);
		mSubToken = (TextView) findViewById(R.id.sub_token);
		mVoipToken = (TextView) findViewById(R.id.voip_token);

		mCcpImaButton = (CCPButton) findViewById(R.id.login_confrim);
		mCcpImaButton.setEnabled(true);
		mCcpImaButton.setOnClickListener(this);

		mServerIPEditText = (EditText) findViewById(R.id.voip_server_ip);
		mPassWDEditText = (EditText) findViewById(R.id.voip_server_password);
		mServerPortEditText = (EditText)findViewById(R.id.voip_server_port);


	}

	/**
	 *
	 * @Title: initViewUI
	 * @Description: TODO
	 * @param
	 * @return void
	 * @throws
	 */
	private void initViewUI() {

		// init sid switch icon
		mSubsidTextView.setCompoundDrawables(null, null, CCPDrawableUtils.getDrawables(this, R.drawable.sid_switch_selector), null);
		mSubsidTextView.setPadding(CCPUtil.getMetricsDensity(this , 12.0F), 0, CCPUtil.getMetricsDensity(this , 22.0F), 0);

//		mSubsidTextView.setOnClickListener(this);

		mCcpImaButton.setImageResource(R.drawable.login);

//		mSubsidTextView.setText("单击选择VoIP帐号");
//		mSubsidTextView.setGravity(Gravity.CENTER);
//		mSubsidTextView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
	}


	@Override
	public int getTitleLayout() {
		return R.layout.ccp_title;
	}


	protected int getLayoutId() {
		return R.layout.developer_login;
	}


	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.sub_sid:

			showSubaccountDialog();

			break;
		case R.id.title_btn4:
			finish();
			break;
		case R.id.login_confrim:

			HideSoftKeyboard();

			// server ip address.
			CCPConfig.REST_SERVER_ADDRESS = mServerIPEditText.getText().toString();
			// server ip address.
			CCPConfig.VoIP_PWD = mPassWDEditText.getText().toString();
			CCPConfig.REST_SERVER_PORT = mServerPortEditText.getText().toString();
			CCPConfig.VoIP_ID = mSubsidTextView.getText().toString();
			doSDKRegist();
			break;
		default:
			break;
		}
	}


	private void showSubaccountDialog() {
		//Select voip account from list dialog shower ...
		 AlertDialog.Builder builder = new AlertDialog.Builder(this);
		    builder.setTitle(R.string.str_select_voip_account)
		           .setItems(mVoipArray, new DialogInterface.OnClickListener() {
		               public void onClick(DialogInterface dialog, int which) {

		               // The 'which' argument contains the index position
		               // of the selected item
		        	   mSubsidTextView.setText(mVoipArray[which]);
						   // 选择登陆voip ID , 例如 1004， 1005
		        	   CCPConfig.VoIP_ID = mVoipArray[which];
		        	   fillSubAccountInfo(which);
		        	   dialog.dismiss();
		           }

		    });
		    builder.create().show();
	}


	private void initConfigInfomation() {

		if(CCPConfig.VoIP_ID_LIST != null ) {
			mVoipArray = CCPConfig.VoIP_ID_LIST.split(",");

			if(mVoipArray == null || mVoipArray.length == 0) {
				throw new IllegalArgumentException("Load the VOIP account information errors" +
						", configuration information can not be empty" + mVoipArray);
			}
		}
	}

	/**
	 *
	 * @Title: fillSubAccountInfo
	 * @Description: TODO
	 * @param @param index
	 * @return void
	 * @throws
	 */
	void fillSubAccountInfo(int index) {
		if(CCPConfig.Sub_Account_LIST != null ) {
			String[] split = CCPConfig.Sub_Account_LIST.split(",");
			mSubAccount.setText(getString(R.string.sub_account_title_text, split[index]));
			CCPConfig.Sub_Account = split[index];
		}
		if(CCPConfig.Sub_Token_LIST != null ) {
			String[] split = CCPConfig.Sub_Token_LIST.split(",");
			mSubToken.setText(getString(R.string.sub_token_title_text, split[index]));
			CCPConfig.Sub_Token = split[index];
		}
		// server ip address.
		CCPConfig.REST_SERVER_ADDRESS = mServerIPEditText.getText().toString();
		// server ip address.
		CCPConfig.VoIP_PWD = mPassWDEditText.getText().toString();
		CCPConfig.REST_SERVER_PORT = mServerPortEditText.getText().toString();



//		if(CCPConfig.VoIP_PWD_LIST != null ) {
//			String[] split = CCPConfig.VoIP_PWD_LIST.split(",");
//			mVoipToken.setText(getString(R.string.voip_pwd_title_text, split[index]));
//			// server ip address.
//			CCPConfig.VoIP_PWD = mPassWDEditText.getText().toString(); // split[index];
//		}
		Log4Util.d(CCPHelper.DEMO_TAG, "1");
		mCcpImaButton.setEnabled(true);
		Log4Util.d(CCPHelper.DEMO_TAG, "2");

	}

	@Override
	protected void startAction() {
		super.startAction();
		// Confirmation Information,then send to next activity ,.
		if (!CCPConfig.check()) {
			CCPApplication.getInstance().showToast(R.string.config_error_text);
			return;
		}
		Intent intent = new Intent();
		intent.setClass(DeveloperLoginActivity.this, CapabilityChoiceActivity.class);
		startActivity(intent);
		mCcpImaButton.setEnabled(true);
		this.finish();
	}

	@Override
	protected boolean checkeDeviceHelper() {
		return true;
	}
}
