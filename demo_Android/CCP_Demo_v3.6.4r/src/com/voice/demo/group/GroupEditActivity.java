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
package com.voice.demo.group;

import com.voice.demo.R;
import com.voice.demo.CCPApplication;
import com.voice.demo.group.GroupDetailActivity.GroupCardSetting;
import com.voice.demo.group.model.IMMember;
import com.voice.demo.outboundmarketing.RestHelper.ERequestState;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.tools.net.ITask;

import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputType;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.TextView;

public class GroupEditActivity extends GroupBaseActivity {

	
	private int mEditType;
	private String mEditContent;
	
	private EditText mInputEdit;
	private TextView mNumTip;
	
	private String groupId;
	private String voipAccount;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.layout_group_card_edit);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, null
				, getString(R.string.btn_complete));
		
		// Bring up the softkeyboard so the user can immediately enter msg. This
		// call won't do anything on devices with a hard keyboard.
		getWindow().setSoftInputMode(
				WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE
						| WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);
		InitResource();
		initialize();
	}
	
	
	@Override
	protected void handleTitleAction(int direction) {
		
		if(direction == TITLE_RIGHT_ACTION) {
			showConnectionProgress(getString(R.string.dialod_message_operationling));
			ITask iTask = new ITask(RestGroupManagerHelper.KEY_MODIFY_GROUPCARD);
			addTask(iTask);
		} else {
			super.handleTitleAction(direction);
		}
	}
	
	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		
		int key = iTask.getKey();
		if(key == RestGroupManagerHelper.KEY_MODIFY_GROUPCARD) {
			
			IMMember imMember = new IMMember();
			
			switch (mEditType) {
			case GroupCardSetting.GROUP_CARD_NAME:
				imMember.displayName = mInputEdit.getText().toString();
				break;
			case GroupCardSetting.GROUP_CARD_TELEPHONE:
				imMember.tel = mInputEdit.getText().toString();
				break;
			case GroupCardSetting.GROUP_CARD_MAIL:
				imMember.mail = mInputEdit.getText().toString();
				break;
			case GroupCardSetting.GROUP_CARD_SIGNATURE:
				imMember.remark = mInputEdit.getText().toString();
				break;

			default:
				break;
			}
			imMember.belong = groupId;
			if(!TextUtils.isEmpty(voipAccount)) {
				imMember.voipAccount = voipAccount;
			}
			RestGroupManagerHelper.getInstance().modifyGroupCard(imMember);
			
		}
	}
	
	@Override
	protected void handleModifyGroupCard(ERequestState reason) {
		super.handleModifyGroupCard(reason);
		closeConnectionProgress();
		if(reason == ERequestState.Success) {
			Intent intent = new Intent(GroupEditActivity.this, GroupDetailActivity.class);
			intent.putExtra("Edit_Content", mInputEdit.getText().toString());
			setResult(RESULT_OK, intent);
			finish();
		} else {
			CCPApplication.getInstance().showToast(R.string.toast_str_modify_card_failed);
		}
	}

	private void initialize() {
		// Read parameters or previously saved state of this activity.
		Intent intent = getIntent();
		
		if (intent.hasExtra(KEY_GROUP_ID)) {
			Bundle extras = intent.getExtras();
			if (extras != null) {
				groupId = (String) extras.get(KEY_GROUP_ID);
				
			}
		}
		
		if(TextUtils.isEmpty(groupId)) {
			finish();
		}
		
		
		if (intent.hasExtra("Edit_Tag")) {
			Bundle extras = intent.getExtras();
			if (extras != null) {
				mEditType = (Integer) extras.get("Edit_Tag");
				
			}
		}
		
		if (intent.hasExtra("Edit_Content")) {
			Bundle extras = intent.getExtras();
			if (extras != null) {
				mEditContent = (String) extras.get("Edit_Content");
				
			}
		}
		
		if (intent.hasExtra("voipAccount")) {
			Bundle extras = intent.getExtras();
			if (extras != null) {
				voipAccount = (String) extras.get("voipAccount");
			}
		}
		
		mInputEdit.setSingleLine();
		switch (mEditType) {
		case GroupCardSetting.GROUP_CARD_NAME:
			msgContentCount = 64;
			setActivityTitle(R.string.str_group_card_name);
			break;
		case GroupCardSetting.GROUP_CARD_TELEPHONE:
			msgContentCount = 20;
			mInputEdit.setInputType(InputType.TYPE_CLASS_PHONE);
			setActivityTitle(R.string.str_group_card_telephone);
			break;
		case GroupCardSetting.GROUP_CARD_MAIL:
			msgContentCount = 30;
			mInputEdit.setInputType(InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS);
			setActivityTitle(R.string.str_group_card_mail);
			break;
		case GroupCardSetting.GROUP_CARD_SIGNATURE:
			mInputEdit.setSingleLine(false);
			mInputEdit.setInputType(InputType.TYPE_CLASS_TEXT |InputType.TYPE_TEXT_FLAG_MULTI_LINE);
			mInputEdit.setLines(4);
			msgContentCount = 200;
			setActivityTitle(R.string.str_group_card_signature);
			break;

		default:
			break;
		}
		
		if(!TextUtils.isEmpty(mEditContent)) {
			mInputEdit.setText(mEditContent);
			mInputEdit.setSelection(mEditContent.length());
			
		}
		
		mNumTip.setText(mEditContent.length() + "/" + msgContentCount);
	}

	private void InitResource() {
		
		mInputEdit = (EditText) findViewById(R.id.group_edit_clearinput);
		mNumTip = (TextView) findViewById(R.id.group_edit_num_tip);
		
		mInputEdit.addTextChangedListener(mTextEditorWatcher);
	}
	
	
	// 
	private CharSequence mMsgContent;
	private int msgContentCount = 200;
	public final TextWatcher mTextEditorWatcher = new TextWatcher() {
		public void beforeTextChanged(CharSequence s, int start, int count,
				int after) {
			mMsgContent = s;
		}

		public void onTextChanged(CharSequence s, int start, int before,
				int count) {
			
			if(CCPUtil.hasFullSize(s.toString())){
				switch (mEditType) {
				case GroupCardSetting.GROUP_CARD_NAME:
					msgContentCount = 32;
					break;
				case GroupCardSetting.GROUP_CARD_SIGNATURE:
					msgContentCount = 100;
					break;
				}
			}
		}

		public void afterTextChanged(Editable s) {
			if(mMsgContent.length() > msgContentCount){
				//VoiceApplication.getInstance().showToast(R.string.toast_declared_word_number);
				s.delete(msgContentCount, mMsgContent.length());
			}
			updateEditCounter();
		}
	};
	
	
	private void updateEditCounter(){
		if (mMsgContent.length() > 0) {
			//mNumTip.setVisibility(View.VISIBLE);
			mNumTip.setText(this.mInputEdit.getText().length() + "/" + msgContentCount);
		} else {
			mNumTip.setText("0/" + msgContentCount);
		}
	}
}
