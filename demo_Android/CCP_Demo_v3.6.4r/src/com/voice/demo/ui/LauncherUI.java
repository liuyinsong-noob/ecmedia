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
 */package com.voice.demo.ui;


import com.voice.demo.R;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.developer.DeveloperLoginActivity;
import com.voice.demo.ui.experience.AccountChooseActivity;
import com.voice.demo.ui.experience.ExperienceLogin;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.view.*;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;

/**
 * 
* <p>Title: LauncherUI.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2013</p>
* <p>Company: http://www.cloopen.com</p>
* @author Jorstin Chan
* @date 2013-11-27
* @version 3.6
 */
public class LauncherUI extends Activity implements View.OnClickListener {
	private static final int STOPSPLASH = 0;
	// time in milliseconds
	private static final long SPLASHTIME = 3000;

	private RelativeLayout splash;

	/**
	 * 
	 */
	private View mWelcomeView;

	private Handler splashHandler = new Handler() {
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case STOPSPLASH:
					splash.setVisibility(View.GONE);
			}
			super.handleMessage(msg);
		}
	};
	private FrameLayout mLauncherView;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		 
		setContentView(getLayoutId());

		splash = (RelativeLayout) findViewById(R.id.splashscreen);

		Message msg = new Message();
		msg.what = STOPSPLASH;
		splashHandler.sendMessageDelayed(msg, SPLASHTIME);

		LinearLayout mLauncherLayout = (LinearLayout) findViewById(R.id.launcher_root);
		
		mLauncherView = addFeatureGuide();
		mLauncherLayout.addView(mLauncherView);
		initLayoutResource();
		
		initWelcomeInstructionResourceRefs();
	}
	
	
	private void initLayoutResource() {
		
		findViewById(R.id.developer).setOnClickListener(this);
		findViewById(R.id.experience).setOnClickListener(this);
	}
	
	private void initWelcomeInstructionResourceRefs() {
		if(findViewById(R.id.welcome_id) != null)
			findViewById(R.id.welcome_id).setOnClickListener(this);
	}


	//@Override
	protected int getLayoutId() {
		return R.layout.launcher;
		//return R.layout.app_panel;
	}

	/*@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {
			
			VoiceApplication.getInstance().quitApp();
			return true;
		}

		return super.onKeyDown(keyCode, event);
	}*/

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.developer:
			startActivity(new Intent(LauncherUI.this , DeveloperLoginActivity.class));
			
			break;
		case R.id.experience:
			
			if(checkAutoLogin()) {
				
				startActivity(new Intent(LauncherUI.this , AccountChooseActivity.class));
				return;
			}
			
			startActivity(new Intent(LauncherUI.this , ExperienceLogin.class));
			break;
			
		case R.id.welcome_id:
			
			try {
				if(mLauncherView != null) {
					mLauncherView.removeView(mWelcomeView);
					CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_SHOW_WELCOME_VIEW, Boolean.FALSE, true);
				}
			} catch (Exception e) {
			}
			break;
		default:
			break;
		}
	}


	/**
	 * @return
	 */
	boolean checkAutoLogin() {
		String userName = CcpPreferences.getSharedPreferences().getString(
				CCPPreferenceSettings.SETTING_USERNAME_MAIL.getId(),
				(String) CCPPreferenceSettings.SETTING_USER_PASSWORD
						.getDefaultValue());
		
		String userpas = CcpPreferences.getSharedPreferences().getString(
				CCPPreferenceSettings.SETTING_USER_PASSWORD.getId(),
				(String) CCPPreferenceSettings.SETTING_USER_PASSWORD
				.getDefaultValue());
		
		if(TextUtils.isEmpty(userName) || TextUtils.isEmpty(userpas)) {
			return false;
		}
		
		return true;
	}
	
	
	private FrameLayout addFeatureGuide() {
		
		FrameLayout.LayoutParams iViewFLayoutParams = new FrameLayout.LayoutParams(
				FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT);
		iViewFLayoutParams.gravity = 17;
		
		FrameLayout frameLayout = new FrameLayout(this);
		
		View launcherModel = getLayoutInflater().inflate(R.layout.launcher_mode, null);
		
		frameLayout.addView(launcherModel);
		
		boolean instrucViewEnable = CcpPreferences.getSharedPreferences()
				.getBoolean(CCPPreferenceSettings.SETTING_SHOW_WELCOME_VIEW.getId(), Boolean.TRUE);
		if(instrucViewEnable && mWelcomeView == null) {
			mWelcomeView = getLayoutInflater().inflate(R.layout.welcome_instructios , null);
			mWelcomeView.setLayoutParams(iViewFLayoutParams);
			mWelcomeView.setVisibility(View.VISIBLE);
			frameLayout.addView(mWelcomeView);
		}
		
		return frameLayout;
	}

}
