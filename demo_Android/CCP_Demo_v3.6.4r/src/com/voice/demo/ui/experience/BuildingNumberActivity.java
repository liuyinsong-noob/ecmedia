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
import android.text.Editable;
import android.text.InputType;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;

import com.voice.demo.R;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPDrawableUtils;
import com.voice.demo.tools.net.LoginRestHelper;
import com.voice.demo.tools.net.ITask;
import com.voice.demo.ui.LoginUIActivity;
import com.voice.demo.views.CCPButton;
import com.voice.demo.views.CCPTitleViewBase;

/**
 * 
 * @ClassName: BuildingNunberActivity 
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-3
 *
 */
public class BuildingNumberActivity extends LoginUIActivity implements View.OnClickListener{

	private CCPButton mBuildConfrim;
	
	private CCPTitleViewBase mCcpTitleViewBase;

	private EditText rePhone;

	private String oldNum;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		initLayoutTitleBar();
		
		initBuildLayoutView();
		
		initViewUI();
		
		getIntentExtra();
	}
	
	/**
	 * 
	 */
	private void initLayoutTitleBar() {
		mCcpTitleViewBase = new CCPTitleViewBase(this);
		mCcpTitleViewBase.setCCPTitleBackground(R.drawable.video_title_bg);
		mCcpTitleViewBase.setCCPTitleViewText(getString(R.string.app_title_build_number));
		mCcpTitleViewBase.setCCPBackImageButton(R.drawable.video_back_button_selector , this).setBackgroundDrawable(null);
	}
	
	/**
	 * 
	 */
	private void initBuildLayoutView() {
		rePhone = (EditText) findViewById(R.id.test_phone);
		rePhone.setInputType(InputType.TYPE_CLASS_PHONE);
		rePhone.setCompoundDrawables(CCPDrawableUtils.getDrawables(this, R.drawable.mobile), null, null, null);
		rePhone.addTextChangedListener(new TextWatcher() {
			
			@Override
			public void onTextChanged(CharSequence s, int start, int before, int count) {
				
			}
			
			@Override
			public void beforeTextChanged(CharSequence s, int start, int count,
					int after) {
				
			}
			
			@Override
			public void afterTextChanged(Editable s) {
				mBuildConfrim.setEnabled(TextUtils.isEmpty(s.toString()) ? false :true);
			}
		});
		
		mBuildConfrim = (CCPButton) findViewById(R.id.build_confrim);
		mBuildConfrim.setOnClickListener(this);
		mBuildConfrim.setBackgroundResource(R.drawable.video_blue_button_selector);
		mBuildConfrim.setImageResource(R.drawable.start_experience);
	}
	
	/**
	 * set test number in view
	 */
	private void initViewUI(){
		rePhone.setHint(R.string.str_input_testnumbe_hint);
	}

	
	public void getIntentExtra() {
		Intent intent = getIntent();
		if(intent.hasExtra("phone")) {
			oldNum = intent.getStringExtra("phone");
			
			if(!TextUtils.isEmpty(oldNum)) {
				
				setBuildNumber(oldNum);
			}
		}
	}
	
	private void setBuildNumber(String phone) {
		rePhone.setText(phone);
		rePhone.setSelection(phone.length());
		
	}


	@Override
	protected int getLayoutId() {
		return R.layout.test_number;
	}


	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPBaseActivity#onResume()
	 */
	@Override
	protected void onResume() {
		super.onResume();
		DisplaySoftKeyboard();
	}
	@Override
	public void onClick(View v) {
		
		switch (v.getId()) {
		case R.id.title_btn4:
			finish();
			break;
	
		case R.id.build_confrim:
			if(TextUtils.isEmpty(rePhone.getText())){
				return;
			}
			if(CCPConfig.mobile.equals(rePhone.getText().toString())){
				Toast.makeText(getApplicationContext(), "请不要用注册号码做绑定号码", Toast.LENGTH_LONG).show();
				return;
			}
			HideSoftKeyboard();
			
			showConnectionProgress(getString(R.string.str_dialog_message_default));
			ITask iTask = new ITask(LoginRestHelper.REST_BUILD_TEST_NUMBER);
			iTask.setTaskParameters("newNum", rePhone.getText().toString());
			addTask(iTask);
			break;
		default:
			break;
		}
	}
	
	
	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		
		if(iTask.getKey() == LoginRestHelper.REST_BUILD_TEST_NUMBER) {
			LoginRestHelper.getInstance().doTestNumber(oldNum, rePhone.getText().toString());
		}
	}

	
	@Override
	protected void handleTestNumber(String number) {
		super.handleTestNumber(number);
		closeConnectionProgress();
		Intent intent = new Intent(BuildingNumberActivity.this, TestNumberActivity.class);
		intent.putExtra("phone", number);
		setResult(RESULT_OK , intent);
		finish();
		Toast.makeText(getApplicationContext(), getString(R.string.toast_test_number, number), Toast.LENGTH_SHORT).show();
	}
	
}
