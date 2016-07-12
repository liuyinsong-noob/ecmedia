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

import java.io.InvalidClassException;
import java.util.HashMap;

import org.json.JSONException;
import org.json.JSONObject;

import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.cloopen.rest.sdk.CCPRestSDK;
import com.cloopen.rest.sdk.CCPRestSDK.BodyType;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.VoiceUtil;
import com.voice.demo.R;
import com.voice.demo.contacts.ContactListActivity;
import com.voice.demo.outboundmarketing.RestHelper;
import com.voice.demo.outboundmarketing.RestHelper.ERequestState;
import com.voice.demo.outboundmarketing.model.LandingCall;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.ui.CCPHelper;

public class VoiceVerificationCodeActivity extends CCPBaseActivity implements View.OnClickListener 
																		,RestHelper.onRestHelperListener{

	private static final int REQUEST_CODE_VERIFY_RESULT = 12;

	
	private EditText mNumber;
	private EditText mVeriCode;
	private Button mCodeBtn;
	private Button mSubmit;
	
	private ProgressDialog mDialog;
	
	private String mCurrentCode;
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_veri_code)
				, null);
		
		initResourceRefs();
		
		RestHelper.getInstance().setOnRestHelperListener(this);
	}

	private void initResourceRefs() {
		mNumber = (EditText) findViewById(R.id.number_input);
		SharedPreferences sp = CcpPreferences.getSharedPreferences();
		mNumber.setText(sp.getString(CCPPreferenceSettings.SETTING_PHONE_NUMBER.getId(), (String)CCPPreferenceSettings.SETTING_PHONE_NUMBER.getDefaultValue()));
		
		mVeriCode = (EditText) findViewById(R.id.code_input);
		mCodeBtn = (Button) findViewById(R.id.btn_code);
		mSubmit = (Button) findViewById(R.id.code_submit);
		mSubmit.setEnabled(true);
		
		mCodeBtn.setOnClickListener(this);
		mSubmit.setOnClickListener(this);
		
		Button btn_choosecontact = (Button) findViewById(R.id.btn_choosecontact);
		btn_choosecontact.setOnClickListener(this);
		
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.btn_code: // request verificatio code ...
			if(TextUtils.isEmpty(mNumber.getText().toString())) {
				Toast.makeText(VoiceVerificationCodeActivity.this, "请输入号码", Toast.LENGTH_SHORT).show();
				return ;
			}
			mCurrentCode = CCPUtil.getCharAndNumr(4, CCPUtil.RANDOM_STRING_NUM);
			mDialog = new ProgressDialog(this);
			mDialog.setMessage(getString(R.string.dialog_verify_message_text));
			mDialog.setCanceledOnTouchOutside(false);
			mDialog.show();
			
			new VoiceVerifyCodeAsyncTask().execute();
			break;
		case R.id.code_submit: //
			String phoneNumber = mNumber.getText().toString();
			if(TextUtils.isEmpty(phoneNumber)) {
				Toast.makeText(VoiceVerificationCodeActivity.this, "请输入号码", Toast.LENGTH_SHORT).show();
				return ;
			}
			try {
				CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_PHONE_NUMBER, phoneNumber, true);
			} catch (InvalidClassException e) {
				e.printStackTrace();
			}
			
			if(TextUtils.isEmpty(mVeriCode.getText().toString())) {
				
				Toast.makeText(VoiceVerificationCodeActivity.this, "请输入验证码", Toast.LENGTH_SHORT).show();
				return ;
			}
			
			Intent intent = new Intent(VoiceVerificationCodeActivity.this, ValidationStatusActivity.class);
			String verifyCode = mVeriCode.getText().toString();
			if(!TextUtils.isEmpty(mCurrentCode) &&  !TextUtils.isEmpty(verifyCode) && mCurrentCode.equals(verifyCode.toLowerCase())) {
				intent.putExtra("ERequest_State", ERequestState.Success);
				
			}else {
				
				intent.putExtra("ERequest_State", ERequestState.Failed);
			}
			startActivityForResult(intent, REQUEST_CODE_VERIFY_RESULT);
			break;
		case R.id.btn_choosecontact:
			Intent toContactIntent = new Intent(this,ContactListActivity.class);
			toContactIntent.putExtras(new Bundle());
			this.startActivityForResult(toContactIntent, ContactListActivity.CHOOSE_CONTACT);
			break;
		default:
			break;
		}
	}
	
	public class VoiceVerifyCodeAsyncTask extends AsyncTask<Void, Void, String> {
		@Override
		protected String doInBackground(Void... params) {
//			RestHelper.getInstance().VoiceVerifyCode(mCurrentCode, "3", VoiceUtil.getStandardMDN(mNumber.getText().toString()) , "" , "");
			CCPRestSDK restAPI = new CCPRestSDK();
			restAPI.init(CCPConfig.REST_SERVER_ADDRESS, CCPConfig.REST_SERVER_PORT);
			restAPI.setAccount(CCPConfig.Main_Account, CCPConfig.Main_Token);
			restAPI.setAppId(CCPConfig.App_ID);
			HashMap<String,Object>  mvoiceVerify=restAPI.voiceVerify(mCurrentCode, mNumber.getText().toString(),"","3","");
			ERequestState state=null;
			if(mvoiceVerify.containsKey("statusCode")&&"000000".equals(mvoiceVerify.get("statusCode"))){
				state = ERequestState.Success;
        	}else{
        		state = ERequestState.Failed;
        	}
			onVoiceCode(state);
			return null;
		}

		@Override
		protected void onPostExecute(String result) {
			super.onPostExecute(result);
			
			if(mDialog != null && mDialog.isShowing()) {
				mDialog.cancel();
			}
		}
	}

	@Override
	public void onLandingCAllsStatus(ERequestState reason, LandingCall callId) {

	}

	@Override
	public void onVoiceCode(ERequestState reason) {
		Log4Util.d(CCPHelper.DEMO_TAG ,"[VoiceVerificationCodeActivity] onVoiceCode: reason .. " + reason);
		Message obtainMessage = mCodeHandler.obtainMessage(CCPHelper.WHAT_ON_VERIFY_CODE);
		obtainMessage.obj = reason;
		mCodeHandler.sendMessage(obtainMessage);
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);

		Log4Util.d(CCPHelper.DEMO_TAG ,"[VoiceVerificationCodeActivity] onActivityResult: requestCode=" + requestCode
				+ ", resultCode=" + resultCode + ", data=" + data);
		//choose contact back
		if(requestCode==ContactListActivity.CHOOSE_CONTACT&&data!=null){
			String phone = data.getStringExtra("phone");
			mNumber.setText(phone);
		}
		// If there's no data (because the user didn't select a number and
		// just hit BACK, for example), there's nothing to do.
		if (resultCode != RESULT_OK) {
			Log4Util.d(CCPHelper.DEMO_TAG ,"[VoiceVerificationCodeActivity] onActivityResult: bail due to resultCode=" + resultCode);
			return;
		}
		
		int Operating = ValidationStatusActivity.OPERATING_GET_NEW_VERIFY  ;
		if(data.hasExtra("Operating")) {
			Bundle extras = data.getExtras();
			if (extras != null) {
				Operating = extras.getInt("Operating");
			}
		}
		
		mVeriCode.getText().clear();
		if(Operating == ValidationStatusActivity.OPERATING_INPUT_AGAIN) {
			mVeriCode.requestFocus();
			requestFocusAndShowInputMode(mVeriCode);
		} else if (Operating == ValidationStatusActivity.OPERATING_GET_NEW_VERIFY) {
			mNumber.getText().clear();
			mNumber.requestFocus();
			requestFocusAndShowInputMode(mNumber);
		} else if (Operating == ValidationStatusActivity.OPERATING_VIEW_OVER) {
			finish();
		} else {
			mNumber.getText().clear();
			mVeriCode.getText().clear();
		}
		
		
	}
	
	
	void requestFocusAndShowInputMode(final EditText tv) {
		
		// Bring up the softkeyboard so the user can immediately enter msg. This
		// call won't do anything on devices with a hard keyboard.
		
		tv.setOnFocusChangeListener(new OnFocusChangeListener() {
			@Override
			public void onFocusChange(View v, final boolean hasFocus) {
				if (hasFocus) {
					Handler handler = new Handler();
					handler.postDelayed(new Runnable() {
						public void run() {
							InputMethodManager imm = (InputMethodManager)
		                    tv.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
		                    if(hasFocus){
		                        imm.toggleSoftInput(0, InputMethodManager.HIDE_NOT_ALWAYS);
		                    }else{
		                    	imm.hideSoftInputFromWindow(tv.getWindowToken(),0);
		                    }
						}
					} ,300);
				}
			}
		});
	}
	
	
	private android.os.Handler mCodeHandler = new android.os.Handler() {


		@Override
		public void handleMessage(Message msg) {
			super.handleMessage(msg);
			
			ERequestState reason = ERequestState.Failed;
			// 获取通话ID
			if (msg.obj instanceof ERequestState) {
				reason = (ERequestState) msg.obj;
			}
			
			switch (msg.what) {
			//receive a new voice mail messages...
			case CCPHelper.WHAT_ON_VERIFY_CODE:
				if(reason == ERequestState.Success) {
					Toast.makeText(VoiceVerificationCodeActivity.this, "获取验证码成功,请等待系统来电", Toast.LENGTH_SHORT).show();
					mCodeBtn.setEnabled(false);
					new CountDownTimer(30000,1000) { 

						@Override 
						public void onTick(long millisUntilFinished) { 
							mCodeBtn.setText(getString(R.string.str_verify_code_timer, millisUntilFinished/1000)) ;
						} 

						@Override 
						public void onFinish() { 
							mCodeBtn.setText(getString(R.string.str_get_verify_code));
							mCodeBtn.setEnabled(true);
						} 
					}.start();
				} else {
					Toast.makeText(VoiceVerificationCodeActivity.this, "获取验证码失败,请重试", Toast.LENGTH_SHORT).show();
					
				}
				break;

			default:
				break;
			}
			
		}
	};


	@Override
	protected int getLayoutId() {
		return R.layout.layout_voice_verificaode_activity;
	}
}
