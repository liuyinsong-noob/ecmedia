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
package com.voice.demo.videoconference;

import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;

import com.voice.demo.R;
import com.voice.demo.chatroom.ChatroomName;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPDrawableUtils;
import com.voice.demo.views.CCPButton;
import com.voice.demo.views.CCPTitleViewBase;

/**
 * 
* <p>Title: CreateVideoConference.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-11-1
* @version 3.5
 */
public class CreateVideoConference extends VideoconferenceBaseActivity implements View.OnClickListener{

	CCPTitleViewBase mCcpTitleViewBase;
	
	private EditText mVideoCEditText;
	private CCPButton mVideoCSubmit;

	private CheckBox cb_autoclose;
	private CheckBox cb_autojoin;
	private int voiceMod = 1;
	private int autoDelete = 1;
	private int mVideoConfType = VideoconferenceConversation.TYPE_SINGLE;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		mCcpTitleViewBase = new CCPTitleViewBase(this);
		mCcpTitleViewBase.setCCPTitleBackground(R.drawable.video_title_bg);
		mCcpTitleViewBase.setCCPTitleViewText(getString(R.string.str_button_new_video_conference));
		mCcpTitleViewBase.setCCPBackImageButton(R.drawable.video_back_button_selector , this).setBackgroundDrawable(null);
		
		// Bring up the softkeyboard so the user can immediately enter msg. This
		// call won't do anything on devices with a hard keyboard.
		/*getWindow().setSoftInputMode(
				WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE
						| WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);*/

		// Initialize members for UI elements.
		initResourceRefs();
		initialize(savedInstanceState);
		
	}

	private void initResourceRefs() {

		RadioGroup rg_autoDelete = (RadioGroup) findViewById(R.id.rg1_video);
		rg_autoDelete.check(R.id.rb1_video);
		rg_autoDelete.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(RadioGroup arg0, int id) {
				if(id==R.id.rb1_video){
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
		View ll_cb = findViewById(R.id.ll_cb_autoclose);
		ll_cb.setOnClickListener(this);
		cb_autoclose = (CheckBox) findViewById(R.id.cb_autoclose);
		
		View ll_cb2 = findViewById(R.id.ll_cb_autojoin);
		ll_cb2.setOnClickListener(this);
		cb_autojoin = (CheckBox) findViewById(R.id.cb_autojoin);
		
		mVideoCEditText = (EditText) findViewById(R.id.room_name);
		
		mVideoCEditText.setCompoundDrawables(CCPDrawableUtils.getDrawables(this, R.drawable.video_input_icon), null, null, null);
		mVideoCEditText.setHint(R.string.str_create_input_name);
		
		mVideoCEditText.setSelection(mVideoCEditText.getText().length());
		mVideoCSubmit = (CCPButton) findViewById(R.id.create_video_c_submit);
		mVideoCSubmit.setImageResource(R.drawable.create_video);
		mVideoCSubmit.setOnClickListener(this);
		
		
		mVideoCEditText.addTextChangedListener(new TextWatcher() {
			
			@Override
			public void onTextChanged(CharSequence s, int start, int before, int count) {
				if(mVideoCEditText.getText().length() <= 0) {
					mVideoCSubmit.setEnabled(false);
				} else {
					mVideoCSubmit.setEnabled(true);
				}
			}
			
			@Override
			public void beforeTextChanged(CharSequence s, int start, int count,
					int after) { }
			
			@Override
			public void afterTextChanged(Editable s) { }
		});
		
		mVideoCEditText.setOnFocusChangeListener(new OnFocusChangeListener() {
			
			@Override
			public void onFocusChange(View v, boolean hasFocus) {
				if (hasFocus) {
					mVideoCEditText.setBackgroundResource(R.drawable.video_name_input);
				} else {
					mVideoCEditText.setBackgroundResource(R.drawable.video_name_input_no);
				}
			}
		});
		
	}

	private void initialize(Bundle savedInstanceState) {
		mVideoConfType = getIntent().getIntExtra(VideoconferenceConversation.CONFERENCE_TYPE, mVideoConfType);
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		DisplaySoftKeyboard();
		
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
		case R.id.create_video_c_submit:
			HideSoftKeyboard();
			if(!cb_autojoin.isChecked()){
				if(mVideoConfType == 0) {
					getDeviceHelper().startVideoConference(CCPConfig.App_ID,  mVideoCEditText.getText().toString(), 5, null, null,cb_autoclose.isChecked(),voiceMod,autoDelete==1?true:false,false);
				} else {
					getDeviceHelper().startMultiVideoConference(CCPConfig.App_ID,  mVideoCEditText.getText().toString(), 5, null, null,cb_autoclose.isChecked(),voiceMod,autoDelete==1?true:false,false);
					
				}
				finish();
				return;
			}
			
			Intent intent = new Intent();
			if(mVideoConfType == 0) {
				intent.setClass(CreateVideoConference.this , VideoConferenceChattingUI.class);
			} else {
				intent.setClass(CreateVideoConference.this , MultiVideoconference.class);
				
			}
			intent.putExtra(VideoconferenceConversation.CONFERENCE_CREATOR, CCPConfig.VoIP_ID);
			intent.putExtra(ChatroomName.CHATROOM_NAME, mVideoCEditText.getText().toString());
			intent.putExtra(ChatroomName.IS_AUTO_CLOSE, cb_autoclose.isChecked());
			intent.putExtra(ChatroomName.AUTO_DELETE, autoDelete);
			intent.putExtra(ChatroomName.VOICE_MOD, voiceMod);
			startActivity(intent) ;
			finish();
			break;

		case R.id.title_btn4:
			finishVideo();
			break;
		default:
			break;
		}
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		
		if(keyCode == KeyEvent.KEYCODE_BACK) {
			HideSoftKeyboard();
			finishVideo();
		}
		return super.onKeyDown(keyCode, event);
		
	}

	private void finishVideo() {
		finish();
		overridePendingTransition(R.anim.push_empty_out, R.anim.video_push_down_out);
	}
	
	@Override
	protected int getLayoutId() {
		return R.layout.video_conference_create;
	}
	
	@Override
	protected void onStop() {
		super.onStop();
	}

	
}
