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
 */
package com.voice.demo.outboundmarketing;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.voice.demo.R;
import com.voice.demo.contacts.ChooseContactsActivity;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.CCPBaseActivity;

import java.util.ArrayList;

/// Marketing outbound activity
public class MarketActivity extends CCPBaseActivity implements
		View.OnClickListener {
	private EditText mPhoneNumEdit1;
	private EditText mPhoneNumEdit2;
	private EditText mPhoneNumEdit3;
	private EditText mPhoneNumEdit4;
	private EditText mPhoneNumEdit5;

	private Button mOutboundButton;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		handleTitleDisplay(getString(R.string.btn_title_back),
				getString(R.string.str_market_head_text),
				getString(R.string.str_button_str_outbound));

		// EditText
		mPhoneNumEdit1 = (EditText) findViewById(R.id.phoneNum_Edit1);
		SharedPreferences sp = CcpPreferences.getSharedPreferences();
		mPhoneNumEdit1.setText(sp.getString(
				CCPPreferenceSettings.SETTING_PHONE_NUMBER.getId(),
				(String) CCPPreferenceSettings.SETTING_PHONE_NUMBER
						.getDefaultValue()));

		mPhoneNumEdit2 = (EditText) findViewById(R.id.phoneNum_Edit2);
		mPhoneNumEdit3 = (EditText) findViewById(R.id.phoneNum_Edit3);
		mPhoneNumEdit4 = (EditText) findViewById(R.id.phoneNum_Edit4);
		mPhoneNumEdit5 = (EditText) findViewById(R.id.phoneNum_Edit5);

		// button
		mOutboundButton = (Button) findViewById(R.id.btn_Outbound);
		mOutboundButton.setOnClickListener(this);

		Button btn_choosecontact = (Button) findViewById(R.id.btn_choosecontact);
		btn_choosecontact.setOnClickListener(this);
	}

	ArrayList<String> phoneNumArray = null;

	@Override
	protected void handleTitleAction(int direction) {
		if (direction == TITLE_RIGHT_ACTION) {
			phoneNumArray = new ArrayList<String>();
			Boolean isEmpty = true;

			if (!TextUtils.isEmpty(mPhoneNumEdit1.getText().toString())) {
				isEmpty = false;
				phoneNumArray.add(mPhoneNumEdit1.getText().toString());
			}

			if (!TextUtils.isEmpty(mPhoneNumEdit2.getText().toString())) {
				isEmpty = false;
				phoneNumArray.add(mPhoneNumEdit2.getText().toString());
			}

			if (!TextUtils.isEmpty(mPhoneNumEdit3.getText().toString())) {
				isEmpty = false;
				phoneNumArray.add(mPhoneNumEdit3.getText().toString());
			}

			if (!TextUtils.isEmpty(mPhoneNumEdit4.getText().toString())) {
				isEmpty = false;
				phoneNumArray.add(mPhoneNumEdit4.getText().toString());
			}

			if (!TextUtils.isEmpty(mPhoneNumEdit5.getText().toString())) {
				isEmpty = false;
				phoneNumArray.add(mPhoneNumEdit5.getText().toString());
			}

			if (isEmpty) {
				Toast.makeText(this, R.string.Toast_market_phone_empty_text,
						Toast.LENGTH_LONG).show();
				return;
			}

			// display an input dialog .
			// Allows the user to input a number is added to the list of choices
			// .
			// showEditDialog(phoneNumArray);
			// showEditTextDialog(DIALOG_SHOW_KEY_INVITE,
			// InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_FLAG_MULTI_LINE ,
			// false, 2, getString(R.string.dialog_title_input_audio_name),
			// null);
			startAction(phoneNumArray, null);
		} else {
			super.handleTitleAction(direction);
		}
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.btn_Outbound: // Began to call
		{
			handleTitleAction(TITLE_RIGHT_ACTION);
		}
			break;
		case R.id.btn_choosecontact: {
			Intent toContactIntent = new Intent(this, ChooseContactsActivity.class);
//			toContactIntent.putExtras(new Bundle());
			this.startActivityForResult(toContactIntent, ChooseContactsActivity.CHOOSE_CONTACT);
		}
			break;
		default:
			break;
		}

	}

	@Override
	protected void handleEditDialogOkEvent(int requestKey, String editText,
			boolean checked) {
		super.handleEditDialogOkEvent(requestKey, editText, checked);

		if (requestKey == DIALOG_SHOW_KEY_INVITE) {
			String mAudioName = editText;
			startAction(phoneNumArray, mAudioName);
		}
	}

	@Override
	protected void handleDialogCancelEvent(int requestKey) {
		super.handleDialogCancelEvent(requestKey);

		startAction(phoneNumArray, null);
	}

	private void startAction(final ArrayList<String> numbers, String mAudioName) {
		//
		Intent stateIntent = new Intent(MarketActivity.this,
				MarketStateActivity.class);
		/*
		 * if(mAudioName != null && !TextUtils.isEmpty(mAudioName)){ } else {
		 * stateIntent.putExtra("audio_name", "marketingcall.wav"); }
		 */
		stateIntent.putExtra("audio_name", mAudioName);
		stateIntent.putExtra("Outbound_phoneNum", numbers);
		startActivity(stateIntent);
	}

	@Override
	protected int getLayoutId() {
		return R.layout.layout_market_activity;
	}
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		if(requestCode==ChooseContactsActivity.CHOOSE_CONTACT && data!=null){
			 String[] stringArrayExtra = data.getStringArrayListExtra("phones").toArray(new String[0]);
			 for(int i=0;i<stringArrayExtra.length;i++){
				 if(i==0)
				 mPhoneNumEdit1.setText(stringArrayExtra[0]);
				 else if(i==1)
					 mPhoneNumEdit2.setText(stringArrayExtra[1]);
				 else if(i==2)
					 mPhoneNumEdit3.setText(stringArrayExtra[2]);
				 else if(i==3)
					 mPhoneNumEdit4.setText(stringArrayExtra[3]);
				 else if(i==4)
					 mPhoneNumEdit5.setText(stringArrayExtra[4]);
					 
			 }
		}
	}
}
