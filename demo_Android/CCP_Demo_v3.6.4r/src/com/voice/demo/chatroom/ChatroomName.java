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
package com.voice.demo.chatroom;

import com.voice.demo.R;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.ui.CapabilityChoiceActivity;

import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.*;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.Toast;
import android.widget.RadioGroup.OnCheckedChangeListener;

/**
 * Set Chatroom Name ...
 *
 */
public class ChatroomName extends CCPBaseActivity implements View.OnClickListener{
	public static final String IS_AUTO_JOIN  = "isAutoJoin" ;
	public static final String VOICE_MOD = "voiceMod" ;
	public static final String AUTO_DELETE = "autoDelete" ;
	public static final String IS_AUTO_CLOSE = "isAutoClose" ;
	public static final String CHATROOM_NAME = "ChatroomName" ;
	public static final String CHATROOM_PWD = "ChatroomPwd" ;
	public static final String CHATROOM_CREATOR = "ChatroomCreator" ;
	
	private EditText mChatroomName;
	private EditText mChatroomPwd;
	private Button mSubmit ;
	private CheckBox cb_autoclose; 
	private CheckBox cb_autojoin; 
	private int voiceMod = 1;
	private int autoDelete = 1;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_chatroom_create)
				, null);
		
		
		initResourceRefs();
	}

	private void initResourceRefs() {
		
		RadioGroup rg_autoDelete = (RadioGroup) findViewById(R.id.rg1);
		rg_autoDelete.check(R.id.rb1);
		rg_autoDelete.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(RadioGroup arg0, int id) {
				if(id==R.id.rb1){
					autoDelete =1;
				}else{
					autoDelete =0;
				}
			}
		});
		RadioGroup rg_2 = (RadioGroup) findViewById(R.id.rg2);
		rg_2.check(R.id.rb3);
		rg_2.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(RadioGroup arg0, int id) {
				switch (id) {
				case R.id.rb3:
					voiceMod =1;
					break;
				case R.id.rb4:
					voiceMod =0;
					break;
				case R.id.rb5:
					voiceMod =2;
					break;

				 
				}
			}
		});
		
		View ll_cb =  findViewById(R.id.ll_cb_autoclose);
		ll_cb.setOnClickListener(this);
		View ll_cb2 =  findViewById(R.id.ll_cb_autojoin);
		ll_cb2.setOnClickListener(this);
		cb_autoclose = (CheckBox) findViewById(R.id.cb_autoclose);
		cb_autojoin = (CheckBox) findViewById(R.id.cb_autojoin);
		mChatroomName = (EditText) findViewById(R.id.chatroom_name);
		mChatroomPwd = (EditText) findViewById(R.id.chatroom_pwd);
		mChatroomName.setSelection(mChatroomName.getText().length());
		mChatroomName.addTextChangedListener(new TextWatcher() {
			
			@Override
			public void onTextChanged(CharSequence s, int start, int before, int count) {
				if(mChatroomName.getText().length() <= 0) {
					mSubmit.setEnabled(false);
				} else {
					mSubmit.setEnabled(true);
					
				}
			}
			
			@Override
			public void beforeTextChanged(CharSequence s, int start, int count,
					int after) {
				
			}
			
			@Override
			public void afterTextChanged(Editable s) {
				
			}
		});
		
		mSubmit = (Button) findViewById(R.id.create_chatroom_submit);
		mSubmit.setOnClickListener(this);
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.ll_cb_autojoin:
			cb_autojoin.setChecked(!cb_autojoin.isChecked());
			break;
		case R.id.ll_cb_autoclose:
			cb_autoclose.setChecked(!cb_autoclose.isChecked());
			break;
		case R.id.create_chatroom_submit:
			if(TextUtils.isEmpty(mChatroomName.getText())){
				Toast.makeText(getApplicationContext(), "请输入房间名称.", Toast.LENGTH_LONG).show();
				return;
			}
			HideSoftKeyboard();
			if(!cb_autojoin.isChecked()){
				getDeviceHelper().startChatroom(CCPConfig.App_ID, mChatroomName.getText().toString(), 8, null, mChatroomPwd.getText().toString(),cb_autoclose.isChecked(),voiceMod,autoDelete==1?true:false,false);
				finish();
				return;
			}
			String pwd = mChatroomPwd.getText().toString();
			Intent intent = new Intent(ChatroomName.this, ChatroomActivity.class);
			intent.putExtra(CHATROOM_NAME, mChatroomName.getText().toString());
			if(!TextUtils.isEmpty(pwd)) {
				intent.putExtra(CHATROOM_PWD, pwd);
			}
			intent.putExtra(CHATROOM_CREATOR, CCPConfig.VoIP_ID);
			intent.putExtra(IS_AUTO_CLOSE, cb_autoclose.isChecked());
			intent.putExtra(IS_AUTO_JOIN, cb_autojoin.isChecked());
			intent.putExtra(ChatroomName.AUTO_DELETE, autoDelete);
			intent.putExtra(ChatroomName.VOICE_MOD, voiceMod);
			startActivity(intent);
			finish();
			break;

		default:
			break;
		}
	}

	
	@Override
	protected void onResume() {
		super.onResume();
		
		DisplaySoftKeyboard();
	}
	
	@Override
	protected void handleTitleAction(int direction) {
		if(direction == TITLE_LEFT_ACTION) {
			finishChatroom();
		} else {
			super.handleTitleAction(direction);
		}
	}
	
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		
		if(keyCode == KeyEvent.KEYCODE_BACK) {
			HideSoftKeyboard();
			finishChatroom();
		}
		return super.onKeyDown(keyCode, event);
		
	}

	private void finishChatroom() {
		finish();
		overridePendingTransition(R.anim.push_empty_out, R.anim.video_push_down_out);
	}
	
	@Override
	protected int getLayoutId() {
		return R.layout.layout_set_chatroom_name_activity;
	}
}
