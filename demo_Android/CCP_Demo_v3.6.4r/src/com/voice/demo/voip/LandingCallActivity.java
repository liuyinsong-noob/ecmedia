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

import java.io.InvalidClassException;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import com.voice.demo.R;
import com.voice.demo.CCPApplication;
import com.voice.demo.contacts.ContactListActivity;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.CCPBaseActivity;

public class LandingCallActivity extends CCPBaseActivity implements OnClickListener{

	private static final int VOIP_CALL_TYPE_BACK_TALK = 203;
	private static final int VOIP_CALL_TYPE_DIRECT_TALK = 204;
	
	private EditText mPhoneNum;
	private EditText mOutPhoneNumEt;
	
	private Button mDirectCallBtn;
	private Button mCallBackBtn;
	
	private ImageView mReadIcon;
	private TextView mReadTips;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_landingcall)
				, null);
		
		initResourceRefs();
	}

	private void initResourceRefs() {
		mPhoneNum = (EditText) findViewById(R.id.phone_num_input);
		SharedPreferences sp = CcpPreferences.getSharedPreferences();
		mPhoneNum.setText(sp.getString(CCPPreferenceSettings.SETTING_PHONE_NUMBER.getId(), (String)CCPPreferenceSettings.SETTING_PHONE_NUMBER.getDefaultValue()));
		mPhoneNum.setSelection(mPhoneNum.length());
		
		mOutPhoneNumEt = (EditText) findViewById(R.id.number_input);
		mDirectCallBtn = (Button) findViewById(R.id.netphone_direct_call);
		mCallBackBtn = (Button) findViewById(R.id.netphone_call_back);
		
		mReadIcon = (ImageView) findViewById(R.id.ready_icon);
		mReadTips = (TextView) findViewById(R.id.ready_tips);
		
		Button btn_choosecontact = (Button) findViewById(R.id.btn_choosecontact);
		btn_choosecontact.setOnClickListener(this);
		
		mDirectCallBtn.setOnClickListener(this);
		mCallBackBtn.setOnClickListener(this);
		mPhoneNum.addTextChangedListener(localTextWatcher);
		
		mOutPhoneNumEt.addTextChangedListener(new TextWatcher() {
			
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
					mDirectCallBtn.setEnabled(false);
					mCallBackBtn.setEnabled(false);
					mReadIcon.setImageResource(R.drawable.status_quit);
					mReadTips.setText("请先输入本机号码和呼出号码才能进行回拨呼叫");
				} else {
					mDirectCallBtn.setEnabled(true);
					mReadIcon.setImageResource(R.drawable.status_speaking);
					if(TextUtils.isEmpty(mPhoneNum.getText().toString())) {
						mCallBackBtn.setEnabled(false);
						mReadTips.setText("可以拨打网络直拨,输入本机号码拨打网络回拨");
					} else {
						mCallBackBtn.setEnabled(true);
						mReadTips.setText("连接已准备就绪,可以拨打落地电话");
					}
				}
				
				
			}
		});
	}

	
	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.netphone_direct_call: // Direct-dial
			//设置用户输入信息，跳转页面
			CCPConfig.Src_phone = mPhoneNum.getText().toString();
			if(checkeDeviceHelper())
				getDeviceHelper().setSelfPhoneNumber(CCPConfig.Src_phone);
			startVoipCall(VOIP_CALL_TYPE_DIRECT_TALK);
			break;
		case R.id.netphone_call_back: // Call back ...
			//设置用户输入信息，跳转页面
			CCPConfig.Src_phone = mPhoneNum.getText().toString();
			startVoipCall(VOIP_CALL_TYPE_BACK_TALK);
			
			break;
		case R.id.btn_choosecontact:  
			Intent intent = new Intent(LandingCallActivity.this,ContactListActivity.class);
			intent.putExtras(new Bundle());
			this.startActivityForResult(intent, ContactListActivity.CHOOSE_CONTACT);
			break;

		default:
			break;
		}
		if(!TextUtils.isEmpty(CCPConfig.Src_phone)) {
			try {
				CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_PHONE_NUMBER, CCPConfig.Src_phone, true);
			} catch (InvalidClassException e) {
				e.printStackTrace();
			}
		}
	}
	
	void startVoipCall(int mCallType) {

		String phoneStr = mOutPhoneNumEt.getText().toString();
		
		if (TextUtils.isEmpty(phoneStr)) {
			CCPApplication.getInstance().showToast(R.string.edit_input_empty);
			return;
		}

		Intent intent = new Intent(this, CallOutActivity.class);
		// 需要根据模式传递相应参数
		switch (mCallType) {
		case VOIP_CALL_TYPE_DIRECT_TALK:
			// 直拨电话
			/*if (!phoneStr.startsWith("0") && !phoneStr.startsWith("1")) {
				// 判断输入合法性
				CCPApplication.getInstance().showToast(
						getString(R.string.network_dial_number_format));
				return;
			}*/
			
			intent.putExtra(CCPApplication.VALUE_DIAL_VOIP_INPUT, phoneStr);
			intent.putExtra(CCPApplication.VALUE_DIAL_MODE,
					CCPApplication.VALUE_DIAL_MODE_DIRECT);
			startActivity(intent);
			break;
		case VOIP_CALL_TYPE_BACK_TALK:
			// 回拨电话
			if (!phoneStr.startsWith("0") && !phoneStr.startsWith("1")) {
				// 判断输入合法性
				CCPApplication.getInstance().showToast(
						getString(R.string.network_dial_number_format));
				return;
			}
			if (CCPConfig.Src_phone == null || CCPConfig.Src_phone.equals("")) {
				// 判断本机号码是否正确
				CCPApplication.getInstance().showToast(
						getString(R.string.src_phone_empty));
				return;
			}
			intent.putExtra(CCPApplication.VALUE_DIAL_VOIP_INPUT, phoneStr);
			intent.putExtra(CCPApplication.VALUE_DIAL_MODE,
					CCPApplication.VALUE_DIAL_MODE_BACK);
			startActivity(intent);

			break;
		}
	}
	
	TextWatcher localTextWatcher = new TextWatcher() {
		
		@Override
		public void onTextChanged(CharSequence s, int start, int before, int count) {
			
		}
		
		@Override
		public void beforeTextChanged(CharSequence s, int start, int count,
				int after) {
			
		}
		
		@Override
		public void afterTextChanged(Editable s) {
			if(TextUtils.isEmpty(s.toString()) || TextUtils.isEmpty(mOutPhoneNumEt.getText().toString())) {
				mCallBackBtn.setEnabled(false);
				if(TextUtils.isEmpty(mOutPhoneNumEt.getText().toString())){
					mReadIcon.setImageResource(R.drawable.status_quit);
					mReadTips.setText("请先输入本机号码和呼出号码才能进行落地电话呼叫");
				} else {
					mReadIcon.setImageResource(R.drawable.status_speaking);
					mReadTips.setText("可以拨打网络直拨,输入本机号码拨打网络回拨");
				}
			} else {
				mCallBackBtn.setEnabled(true);
				mReadIcon.setImageResource(R.drawable.status_speaking);
				mReadTips.setText("连接已准备就绪,可以拨打落地电话");
			}
		}
		
	};

	@Override
	protected int getLayoutId() {
		return R.layout.layout_netphone_landing_call_activity;
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		if(requestCode==ContactListActivity.CHOOSE_CONTACT && data != null){
			String phone = data.getStringExtra("phone");
			mOutPhoneNumEt.setText(phone);
		}
	}
	
	
}
