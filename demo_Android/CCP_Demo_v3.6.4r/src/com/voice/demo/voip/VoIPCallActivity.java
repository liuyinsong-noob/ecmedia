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
package com.voice.demo.voip;

import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.interphone.InviteInterPhoneActivity;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.ui.CCPHelper;

public class VoIPCallActivity extends CCPBaseActivity implements OnClickListener{

	private static final int REQUEST_CODE_VOIP_CALL = 10;
	
	private EditText mPhoneNum;
	private ImageButton mVoipSelectIbn;
	private TextView mVoipInputEt;
	
	private Button mVoipCallBtn;
	
	private ImageView mReadIcon;
	private TextView mReadTips;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_voip)
				, null);
		
		initResourceRefs();
	}

	private void initResourceRefs() {
		mPhoneNum = (EditText) findViewById(R.id.phone_num_input);
		mVoipSelectIbn = (ImageButton) findViewById(R.id.btn_select_voip);
		mVoipInputEt = (TextView) findViewById(R.id.voip_input);
		mVoipCallBtn = (Button) findViewById(R.id.netphone_voip_call);
		
		mReadIcon = (ImageView) findViewById(R.id.ready_icon);
		mReadTips = (TextView) findViewById(R.id.ready_tips);
		
		mPhoneNum.setText(CCPConfig.VoIP_ID);
		mVoipSelectIbn.setOnClickListener(this) ;
		mVoipInputEt.setOnClickListener(this);
		mVoipCallBtn.setOnClickListener(this);
		mVoipInputEt.addTextChangedListener(new TextWatcher() {
			
			@Override
			public void onTextChanged(CharSequence s, int start, int before, int count) {
				
			}
			
			@Override
			public void beforeTextChanged(CharSequence s, int start, int count,
					int after) {
				
			}
			
			@Override
			public void afterTextChanged(Editable s) {
				if(TextUtils.isEmpty(s.toString())) {
					mVoipCallBtn.setEnabled(false);
					mReadIcon.setImageResource(R.drawable.status_quit);
					mReadTips.setText(R.string.str_input_called_voip);
				} else {
					mVoipCallBtn.setEnabled(true);
					mReadIcon.setImageResource(R.drawable.status_speaking);
					mReadTips.setText(R.string.str_connection_ready);
				}
			}
		});
	}
	
	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.netphone_voip_call:
			
			startVoipCall();
			break;
		case R.id.voip_input:
		case R.id.btn_select_voip: //select voip ...
			Intent intent = new Intent(VoIPCallActivity.this, InviteInterPhoneActivity.class);
			intent.putExtra("create_to", InviteInterPhoneActivity.CREATE_TO_VOIP_CALL);
			startActivityForResult(intent, REQUEST_CODE_VOIP_CALL);


			break;

		default:
			break;
		}
	}
	
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);

		Log4Util.d(CCPHelper.DEMO_TAG ,"[SelectVoiceActivity] onActivityResult: requestCode=" + requestCode
				+ ", resultCode=" + resultCode + ", data=" + data);

		// If there's no data (because the user didn't select a number and
		// just hit BACK, for example), there's nothing to do.
		if (requestCode != REQUEST_CODE_VOIP_CALL ) {
			if (data == null) {
				return;
			}
		} else if (resultCode != RESULT_OK) {
			Log4Util.d(CCPHelper.DEMO_TAG ,"[SelectVoiceActivity] onActivityResult: bail due to resultCode=" + resultCode);
			return;
		}
		
		String phoneStr = "" ;
		if(data.hasExtra("VOIP_CALL_NUMNBER")) {
			Bundle extras = data.getExtras();
			if (extras != null) {
				phoneStr = extras.getString("VOIP_CALL_NUMNBER");
			}
		}
		
		if(TextUtils.isEmpty(phoneStr)) {
			CCPApplication.getInstance().showToast(R.string.edit_input_empty);
			return ;
		}

		if (mVoipInputEt != null ) {
			mVoipInputEt.setText(phoneStr);
		}
		//startVoipCall(phoneStr);
	}
	

	void startVoipCall() {

		String phoneStr = mVoipInputEt.getText().toString();
		
		if (TextUtils.isEmpty(phoneStr)) {
			CCPApplication.getInstance().showToast(R.string.edit_input_empty);
			return;
		}

		Intent intent = new Intent(this, CallOutActivity.class);
		
		// need according to the mode transfer of corresponding parameters
		// VoIP of free telephone
		/*if (!phoneStr.startsWith("8") || phoneStr.length() != 14) {
			
			// Input legitimacy..
			VoiceApplication.getInstance().showToast(
					getString(R.string.voip_number_format));
			return;
		}*/

		intent.putExtra(CCPApplication.VALUE_DIAL_VOIP_INPUT, phoneStr);
		intent.putExtra(CCPApplication.VALUE_DIAL_MODE,CCPApplication.VALUE_DIAL_MODE_FREE);
		startActivity(intent);
	}

	@Override
	protected int getLayoutId() {
		return R.layout.layout_netphone_voip_call_activity;
	}
}
