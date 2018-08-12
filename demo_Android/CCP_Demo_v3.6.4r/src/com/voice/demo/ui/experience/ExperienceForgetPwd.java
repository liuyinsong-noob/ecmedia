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
 */package com.voice.demo.ui.experience;

import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;

import com.voice.demo.R;
import com.voice.demo.ui.LoginUIActivity;
import com.voice.demo.views.CCPTitleViewBase;

public class ExperienceForgetPwd extends LoginUIActivity implements
		OnClickListener {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		CCPTitleViewBase mCcpTitleViewBase = new CCPTitleViewBase(this);
		mCcpTitleViewBase.setCCPTitleBackground(R.drawable.video_title_bg);
		mCcpTitleViewBase.setCCPTitleViewText(getString(R.string.experience_forgetpwd_head));
		mCcpTitleViewBase.setCCPBackImageButton(R.drawable.video_back_button_selector , this).setBackgroundDrawable(null);
		
	}
	
	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.title_btn4:
			finishLogin();
			break;

		default:
			break;
		}


	}

	@Override
	protected int getLayoutId() {
		return R.layout.experience_forgetpwd_layout;
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		
		if(keyCode == KeyEvent.KEYCODE_BACK) {
			finishLogin();
		}
		return super.onKeyDown(keyCode, event);
		
	}
	
	private void finishLogin() {
		finish();
		overridePendingTransition(R.anim.push_empty_out, R.anim.video_push_down_out);
	}
	
}
