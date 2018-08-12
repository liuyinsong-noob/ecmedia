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
package com.voice.demo.group;



import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Toast;

import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.group.model.IMGroup;
import com.voice.demo.outboundmarketing.RestHelper.ERequestState;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.tools.net.ITask;

public class CreateGroupActivity extends GroupBaseActivity  {

	private EditText mGroupName;
	private EditText mGroupNotice;
	
	private Spinner mInfoSpinner;
	private Spinner mJoinModelSpinner;
	
	private int mGroupInfo;
	private int mGroupJoinModel;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.layout_create_group_activity);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_create_new_group)
				, getString(R.string.str_button_create_chatroom));
		
		
		
		initResource();
		initialize();
		
	}
	
	private void initialize() {
		ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(
                this, R.array.group_info, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mInfoSpinner.setAdapter(adapter);
        mInfoSpinner.setOnItemSelectedListener(
                new OnItemSelectedListener() {
                    public void onItemSelected(
                            AdapterView<?> parent, View view, int position, long id) {
                    	mGroupInfo = position;
                    	
                    	// �����д��� ---------------------------------
                    	//TextView tv=(TextView)view;
                    	//tv.setTextColor(CreateGroupActivity.this.getResources().getColor(R.color.blue));
                    	
                    	// ����-------------------------------------
                    }

                    public void onNothingSelected(AdapterView<?> parent) {
                    }
                });
        adapter = ArrayAdapter.createFromResource(this, R.array.group_join_model,
                android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mJoinModelSpinner.setAdapter(adapter);
        mJoinModelSpinner.setOnItemSelectedListener(
                new OnItemSelectedListener() {
                    public void onItemSelected(
                            AdapterView<?> parent, View view, int position, long id) {
                    	mGroupJoinModel = position;
                    }

                    public void onNothingSelected(AdapterView<?> parent) {
                    }
                });
	}
	
	
	
	
	private void initResource() {
		mGroupName = (EditText) findViewById(R.id.group_name);
		mGroupNotice = (EditText) findViewById(R.id.group_notice);
		mGroupNotice.addTextChangedListener(mTextEditorWatcher);
		
		mInfoSpinner = (Spinner) findViewById(R.id.str_group_info_spinner);
		
		mJoinModelSpinner = (Spinner) findViewById(R.id.str_group_join_model_spinner);
	}


	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		
		int key = iTask.getKey();
		if(key == RestGroupManagerHelper.KEY_CREATE_GROUP) {
			String name = (String) iTask.getTaskParameters("name");
			String declared = (String) iTask.getTaskParameters("declared");
			RestGroupManagerHelper.getInstance().createGroup(name, mGroupInfo, declared, mGroupJoinModel);
		}
	}


	@Override
	protected void handleTitleAction(int direction) {
		
		if(direction == TITLE_RIGHT_ACTION ) {
			
			final String name = mGroupName.getText().toString();
			final String declared = mGroupNotice.getText().toString();
			
			if(TextUtils.isEmpty(name)){
				Toast.makeText(getApplicationContext(), R.string.toast_group_name_empty, Toast.LENGTH_SHORT).show();
				return ;
			}
			
			showConnectionProgress(null);
			// 
			ITask iTask = new ITask(RestGroupManagerHelper.KEY_CREATE_GROUP);
			iTask.setTaskParameters("name" , name);
			iTask.setTaskParameters("declared" , declared);
			addTask(iTask);
		} else {
			
			super.handleTitleAction(direction);
		}
	}

	@Override
	protected void handleCreateGroup(ERequestState reason, String groupId) {
		super.handleCreateGroup(reason, groupId);
		// Treatment group is established the result callback
		
		closeConnectionProgress();
		if(reason == ERequestState.Success) {
			Toast.makeText(getApplicationContext(), groupId + "", Toast.LENGTH_SHORT).show();
			IMGroup imGroup = new IMGroup();
			imGroup.name = mGroupName.getText().toString();
			imGroup.groupId = groupId;
			imGroup.declared = mGroupNotice.getText().toString();
			imGroup.type = "0";
			imGroup.permission = "0";
			imGroup.count = "1";
			
			Intent intent = new Intent(CCPIntentUtils.INTENT_JOIN_GROUP_SUCCESS);
			intent.putExtra("IMGroup", imGroup);
			intent.putExtra("isCreate", true);
			sendBroadcast(intent);
			this.finish();
		}
		
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
				msgContentCount = 100;
			}
		}

		public void afterTextChanged(Editable s) {
			if(mMsgContent.length() > msgContentCount){
				CCPApplication.getInstance().showToast(R.string.toast_declared_word_number);
				s.delete(msgContentCount, mMsgContent.length());
			}
		}
	};
}
