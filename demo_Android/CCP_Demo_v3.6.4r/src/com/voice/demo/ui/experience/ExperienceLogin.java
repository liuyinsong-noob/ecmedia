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
import android.graphics.Paint;
import android.os.Bundle;
import android.text.TextUtils;
import android.text.method.PasswordTransformationMethod;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.tools.CCPDrawableUtils;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.LoginUIActivity;
import com.voice.demo.views.CCPButton;
import com.voice.demo.views.CCPTitleViewBase;

public class ExperienceLogin extends LoginUIActivity {
	
	CCPTitleViewBase mCcpTitleViewBase;
	Button registButton;
	Button loginButton;
	LinearLayout loginView;
	LinearLayout registView;
	
	EditText mMailEdit;
	EditText mPwdEdit;
	
	CCPButton mExperLogin;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		CCPApplication.getInstance().setDeveloperMode(false);
		mCcpTitleViewBase = new CCPTitleViewBase(this);
		mCcpTitleViewBase.setCCPTitleBackground(R.drawable.video_title_bg);
		mCcpTitleViewBase.setCCPTitleViewText(getString(R.string.experience_login_head));
		mCcpTitleViewBase.setCCPBackImageButton(R.drawable.video_back_button_selector , this).setBackgroundDrawable(null);
		
		initResourceRefs();
		
		initExperienceView();
		
		initialize(savedInstanceState);
	}
	
	private void initResourceRefs() {

		loginView = (LinearLayout)findViewById(R.id.experience_login_id);
		registView = (LinearLayout)findViewById(R.id.experience_regist_id);
		
		registButton = (Button) findViewById(R.id.switch_regist_view);
		registButton.setOnClickListener(this);
		loginButton = (Button) findViewById(R.id.switch_login_view);
		loginButton.setOnClickListener(this);
	}

	/**
	 * 
	 * @Title: initExperienceView 
	 * @Description: TODO 
	 * @param  
	 * @return void 
	 * @throws
	 */
	private void initExperienceView() {
		mMailEdit = (EditText) findViewById(R.id.mail_edit_text);
		mPwdEdit = (EditText) findViewById(R.id.pwd_edit_text);
		mExperLogin = (CCPButton) findViewById(R.id.experience_login_submit);
		
		TextView view = (TextView) findViewById(R.id.forget_password);
		view.getPaint().setFlags(Paint.UNDERLINE_TEXT_FLAG);

		view.setOnClickListener(this);
		mExperLogin.setOnClickListener(this);

		initExperienceUI();
	}
	
	/**
	 * 
	 * @Title: initExperienceUI 
	 * @Description: Initialization interface resources. 
	 * @param  
	 * @return void 
	 * @throws
	 */
	private void initExperienceUI() {
		
		mMailEdit.setCompoundDrawables(CCPDrawableUtils.getDrawables(this, R.drawable.mail), null, null, null);
		mPwdEdit.setCompoundDrawables(CCPDrawableUtils.getDrawables(this, R.drawable.password), null, null, null);
		
		mMailEdit.setHint(R.string.experience_input_regist_mail);
		mPwdEdit.setTransformationMethod(PasswordTransformationMethod.getInstance());
		
		mExperLogin.setImageResource(R.drawable.experience_next);
	}
	
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		
		if(keyCode == KeyEvent.KEYCODE_BACK) {
			HideSoftKeyboard();
			finishLogin();
		}
		return super.onKeyDown(keyCode, event);
		
	}
	
	private void finishLogin() {
		finish();
	}
	
	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.switch_regist_view:
			HideSoftKeyboard();
			loginView.setVisibility(View.GONE);
			registView.setVisibility(View.VISIBLE);
			registButton.setEnabled(false);
			loginButton.setEnabled(true);
			break;
		case R.id.switch_login_view:
			registView.setVisibility(View.GONE);
			loginView.setVisibility(View.VISIBLE);
			registButton.setEnabled(true);
			loginButton.setEnabled(false);
			break;
		case R.id.forget_password:
			startActivity(new Intent(ExperienceLogin.this , ExperienceForgetPwd.class));
			break;
		case R.id.title_btn4:
			finishLogin();
			break;

		case R.id.experience_login_submit:
			
			HideSoftKeyboard();
			
			doLoginReuqest();
			//startActivity(new Intent(ExperienceLogin.this, AccountChooseActivity.class));
			break;
		default:
			break;
		}

	}
	
	
	/**
	 * 
	 */
	private void doLoginReuqest() {
		String userName = mMailEdit.getText().toString();
		String userPass = mPwdEdit.getText().toString();
		
		if(TextUtils.isEmpty(userName) || TextUtils.isEmpty(userPass)) {
			Toast.makeText(getApplicationContext(), R.string.toast_login_params_empty, Toast.LENGTH_SHORT).show();
			return;
		}
		
		showConnectionProgress(getString(R.string.dialog_message_text));
		doExperienceAutoLogin(userName, userPass);
	}
	

	private void initialize(Bundle savedInstanceState) {
		
		String userName = CcpPreferences.getSharedPreferences().getString(
				CCPPreferenceSettings.SETTING_USERNAME_MAIL.getId(),
				(String) CCPPreferenceSettings.SETTING_USERNAME_MAIL
						.getDefaultValue());
		String userPassword = CcpPreferences.getSharedPreferences().getString(
				CCPPreferenceSettings.SETTING_USER_PASSWORD.getId(),
				(String) CCPPreferenceSettings.SETTING_USER_PASSWORD
				.getDefaultValue());
		
		mMailEdit.setText(userName);
		mMailEdit.setSelection(userName.length());
		mPwdEdit.setText(userPassword);
		
		if(!TextUtils.isEmpty(userName)) {
			mPwdEdit.requestFocus();
		}
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		DisplaySoftKeyboard();
	}
	
	@Override
	protected int getLayoutId() {
		return R.layout.experience_login_view;
	}

	/**
	 * 
	 */
	@Override
	public void handleClientLoginRequest() {
			
        try {
            
            CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_USERNAME_MAIL, mMailEdit.getText().toString(), true);
            CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_USER_PASSWORD, mPwdEdit.getText().toString(), true);
        } catch (Exception e) {
        	e.printStackTrace();
        }
        
		startActivity(new Intent(ExperienceLogin.this, AccountChooseActivity.class));
		
		finishLogin();
		
	}

	
}
