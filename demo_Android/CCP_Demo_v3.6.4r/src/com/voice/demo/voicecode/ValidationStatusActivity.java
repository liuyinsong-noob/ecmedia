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
package com.voice.demo.voicecode;


import com.voice.demo.R;
import com.voice.demo.outboundmarketing.RestHelper.ERequestState;
import com.voice.demo.ui.CCPBaseActivity;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;

public class ValidationStatusActivity extends CCPBaseActivity implements View.OnClickListener {

	public static final int OPERATING_INPUT_AGAIN = 0x1;
	public static final int OPERATING_GET_NEW_VERIFY = 0x2;
	public static final int OPERATING_VIEW_OVER = 0x3;
	private TextView mVerifyStatus;
	private TextView mVerifyTips;
	private RelativeLayout mVerifyFailLy;
	private Button mVerifyInput;
	private Button mVerifyGet;
	private Button mVerifyShowEnd;
	private Button mVerifySuccess;
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_veri_status)
				, null);
		
		initResourceRefs();
		
		initialize(savedInstanceState);
	}

	private void initResourceRefs() {
		
		mVerifyStatus = (TextView) findViewById(R.id.verify_status);
		mVerifyTips = (TextView) findViewById(R.id.verify_status_tips);
		mVerifyFailLy = (RelativeLayout) findViewById(R.id.verify_failed_bottom);
		
		mVerifyInput = (Button) findViewById(R.id.verify_input_again);
		mVerifyGet = (Button) findViewById(R.id.verify_get_again);
		mVerifyShowEnd = (Button) findViewById(R.id.verify_show_end);
		mVerifySuccess = (Button) findViewById(R.id.verify_success);
		
		mVerifyInput.setOnClickListener(this);
		mVerifyGet.setOnClickListener(this);
		mVerifyShowEnd.setOnClickListener(this);
		mVerifySuccess.setOnClickListener(this);
		
	}
	
	
	private void initialize(Bundle savedInstanceState) {
		// Read parameters or previously saved state of this activity.
		Intent intent = getIntent();
		ERequestState state = ERequestState.Failed;
		if(intent.hasExtra("ERequest_State")) {
			Bundle extras = intent.getExtras();
			if (extras != null) {
				state = (ERequestState) extras.getSerializable("ERequest_State"); 
			}
			
		}
		
		if(state == ERequestState.Success) {
			mVerifyStatus.setText(R.string.str_verify_status_success);
			mVerifyTips.setText(R.string.str_verify_status_success_tips);
			mVerifyFailLy.setVisibility(View.GONE);
			mVerifySuccess.setVisibility(View.VISIBLE);
		} else {
			//failed ...
			mVerifyStatus.setText(R.string.str_verify_status_failed);
			mVerifyTips.setText(R.string.str_verify_status_failed_tips);
			mVerifyFailLy.setVisibility(View.VISIBLE);
			mVerifySuccess.setVisibility(View.GONE);
			
		}
	
	}

	@Override
	public void onClick(View v) {
		Intent intent = new Intent(ValidationStatusActivity.this, VoiceVerificationCodeActivity.class) ;
		switch (v.getId()) {
		case R.id.verify_success:
			handleTitleAction(TITLE_LEFT_ACTION);
			break;
		case R.id.verify_input_again:
			intent.putExtra("Operating", OPERATING_INPUT_AGAIN);
			setResult(RESULT_OK, intent);
			finish();
			break;
		case R.id.verify_get_again:
			intent.putExtra("Operating", OPERATING_GET_NEW_VERIFY);
			setResult(RESULT_OK, intent);
			finish();
			break;
		case R.id.verify_show_end:
			intent.putExtra("Operating", OPERATING_VIEW_OVER);
			setResult(RESULT_OK, intent);
			finish();
			break;
		default:
			break;
		}
	}

	@Override
	protected int getLayoutId() {
		return R.layout.layout_validation_status_activity;
	}
}
