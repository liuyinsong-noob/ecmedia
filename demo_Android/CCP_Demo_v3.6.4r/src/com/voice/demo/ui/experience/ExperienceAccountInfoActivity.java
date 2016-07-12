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
 */package com.voice.demo.ui.experience;

import android.content.Intent;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;

import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPDrawableUtils;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.LoginUIActivity;
import com.voice.demo.views.CCPButton;
import com.voice.demo.views.CCPTitleViewBase;

public class ExperienceAccountInfoActivity extends LoginUIActivity implements
		OnClickListener {

	private TextView mMailTextView;
	private TextView mNickTextView;
	private TextView mMainTextView;

	private TextView mAppidTextView;
	private TextView mAppnameTextView;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		initTitle();
		
		initLayoutUI();
		
		initAccountUI();
		
		if(CCPApplication.getInstance().getDemoAccounts() == null) {
			launchCCP();
		}
	}

	private void initTitle() {
		CCPTitleViewBase mCcpTitleViewBase = new CCPTitleViewBase(this);
		mCcpTitleViewBase.setCCPTitleBackground(R.drawable.video_title_bg);
		mCcpTitleViewBase.setCCPTitleViewText(getString(R.string.experience_account_head));
		mCcpTitleViewBase.setCCPBackImageButton(R.drawable.video_back_button_selector, this).setBackgroundDrawable(null);
	}

	private void initLayoutUI()
	{
		mMailTextView = (TextView)findViewById(R.id.mail);
		mNickTextView = (TextView)findViewById(R.id.nick);
		
		mMainTextView = (TextView)findViewById(R.id.main_account);
		mAppidTextView = (TextView)findViewById(R.id.appid);
		mAppnameTextView = (TextView)findViewById(R.id.appname);
		
		mMailTextView.setCompoundDrawables(CCPDrawableUtils.getDrawables(this, R.drawable.account_mail), null, null, null);
		mNickTextView.setCompoundDrawables(CCPDrawableUtils.getDrawables(this, R.drawable.account_nick), null, null, null);
		mMainTextView.setCompoundDrawables(CCPDrawableUtils.getDrawables(this, R.drawable.account_main_account), null, null, null);
		mAppidTextView.setCompoundDrawables(CCPDrawableUtils.getDrawables(this, R.drawable.account_appid), null, null, null);
		mAppnameTextView.setCompoundDrawables(CCPDrawableUtils.getDrawables(this, R.drawable.account_appname), null, null, null);
		
		// init sid switch icon
		TextView bindTestNum = (TextView)findViewById(R.id.bind_test_num);
		bindTestNum.setCompoundDrawables(null, null, CCPDrawableUtils.getDrawables(this, R.drawable.video_item_goto), null);
		bindTestNum.setPadding(CCPUtil.getMetricsDensity(this , 12.0F), 0, CCPUtil.getMetricsDensity(this , 22.0F), 0);		
		bindTestNum.setOnClickListener(this);
		bindTestNum.setText(R.string.experience_account_btn_str);
		
		CCPButton mCcpImaButton = (CCPButton) findViewById(R.id.unlogin_confrim);
		mCcpImaButton.setImageResource(R.drawable.unlogin_text);
		mCcpImaButton.setOnClickListener(this);
	}
	
	
	private void initAccountUI() {
		String userMail = CcpPreferences.getSharedPreferences().getString(
				CCPPreferenceSettings.SETTING_USERNAME_MAIL.getId(),
				(String)CCPPreferenceSettings.SETTING_USERNAME_MAIL.getDefaultValue());
		
		mMailTextView.setText(userMail);
		mNickTextView.setText(CCPConfig.nickname);
		mMainTextView.setText(CCPConfig.Main_Account);
		mAppidTextView.setText(CCPConfig.App_ID);
		mAppnameTextView.setText(CCPConfig.friendlyName);
	}
	
	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.title_btn4:
			finish();
			break;
		case R.id.bind_test_num:
			startActivity(new Intent(ExperienceAccountInfoActivity.this , TestNumberActivity.class));
			break;
			
		case R.id.unlogin_confrim:
			try {

				CCPApplication.getInstance().putDemoAccounts(null);
				CcpPreferences.savePreference(
						CCPPreferenceSettings.SETTING_USER_PASSWORD, "", true);
				
				CCPUtil.exitCCPDemo();
				launchCCP(); 
			} catch (Exception e) {
				e.printStackTrace();
			}
			
			finish();
			 break;
		default:
			break;
		}
	}

	@Override
	protected int getLayoutId() {
		return R.layout.experience_account_info;
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {

		if (keyCode == KeyEvent.KEYCODE_BACK) {
			
			finish();
		}
		return super.onKeyDown(keyCode, event);

	}
	
}
