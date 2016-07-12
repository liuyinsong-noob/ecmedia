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

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.R;
import com.voice.demo.group.GroupDetailActivity.GroupCardSetting;
import com.voice.demo.group.model.IMMember;
import com.voice.demo.outboundmarketing.RestHelper.ERequestState;
import com.voice.demo.tools.net.ITask;
import com.voice.demo.ui.CCPHelper;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

public class GroupMemberCardActivity extends GroupBaseActivity implements  View.OnClickListener {

	private String groupId;
	private String voipAccount;
	private boolean modify;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.layout_group_member_card);

		handleTitleDisplay(getString(R.string.btn_title_back),
				getString(R.string.app_title_group_member_card), null);

		initialize(savedInstanceState);

		showConnectionProgress(getString(R.string.dialog_load_hold));
		ITask iTask = new ITask(RestGroupManagerHelper.KEY_QUERY_GROUPCARD);
		addTask(iTask);
	}

	private void initialize(Bundle savedInstanceState) {
		// Read parameters or previously saved state of this activity.
		Intent intent = getIntent();
		if (intent.hasExtra(KEY_GROUP_ID)) {
			Bundle extras = intent.getExtras();
			if (extras != null) {
				groupId = (String) extras.get(KEY_GROUP_ID);
				
			}
		}
		
		if (intent.hasExtra("voipAccount")) {
			Bundle extras = intent.getExtras();
			if (extras != null) {
				voipAccount = (String) extras.get("voipAccount");
			}
		}
		if (intent.hasExtra("modify")) {
			Bundle extras = intent.getExtras();
			if (extras != null) {
				modify = (Boolean) extras.get("modify");
			}
		}
		
		if(TextUtils.isEmpty(groupId)) {
			Toast.makeText(getApplicationContext(), R.string.toast_group_id_error, Toast.LENGTH_SHORT).show();
			finish();
		}
		
		
	}
	
	@Override
	protected void handleQueryGroupCard(ERequestState reason, IMMember iMember) {
		super.handleQueryGroupCard(reason, iMember);
		closeConnectionProgress();
		if(reason == ERequestState.Success) {
			
			initResourceRefs(iMember);
		}
	}

	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		int key = iTask.getKey();
		if (key == RestGroupManagerHelper.KEY_QUERY_GROUPCARD) {

			// Group membership card information query
			IMMember imMember = new IMMember();
			imMember.belong = groupId;
			imMember.voipAccount = voipAccount;
			RestGroupManagerHelper.getInstance().queryGroupCard(imMember);
		}
	}
	
	private void initResourceRefs(IMMember iMember) {
		
		fillDataViewItem(findViewById(R.id.groupid), groupId);
		fillDataViewItem(findViewById(R.id.voipaccount), iMember.voipAccount);
		fillDataViewItem(findViewById(R.id.nickname), iMember.displayName);
		//fillDataViewItem(findViewById(R.id.gender), iMember.sex);
		//fillDataViewItem(findViewById(R.id.birthday), iMember.birth);
		fillDataViewItem(findViewById(R.id.tel), iMember.tel);
		fillDataViewItem(findViewById(R.id.mail), iMember.mail);
		fillDataViewItem(findViewById(R.id.signature), iMember.remark);
	}
	
	
	void fillDataViewItem(View view , String text) {
		TextView textView = (TextView)view.findViewById(R.id.txt);
		if(!TextUtils.isEmpty(text))
			textView.setText(text);
		
		if(modify) {
			if(view.getId() != R.id.groupid && view.getId() != R.id.voipaccount) {
				view.findViewById(R.id.indicator).setVisibility(View.VISIBLE);
				view.setOnClickListener(this);
			} else{
				view.setOnClickListener(null);
			}
		} else{
			view.findViewById(R.id.indicator).setVisibility(View.INVISIBLE);
			view.setOnClickListener(null);
		}
	}
	TextView clickView;
	@Override
	public void onClick(View v) {
		Intent intent = new Intent(GroupMemberCardActivity.this, GroupEditActivity.class);
		int editTag = GroupCardSetting.GROUP_CARD_SIGNATURE;
		switch (v.getId()) {
		case R.id.nickname:
			editTag = GroupCardSetting.GROUP_CARD_NAME;
			break;
		case R.id.tel:
			editTag = GroupCardSetting.GROUP_CARD_TELEPHONE;
			
			break;
		case R.id.mail:
			editTag = GroupCardSetting.GROUP_CARD_MAIL;
			
			break;
		case R.id.signature:
			editTag = GroupCardSetting.GROUP_CARD_SIGNATURE;
			
			break;

		default:
			break;
		}
		intent.putExtra("Edit_Tag", editTag);
		clickView = (TextView) v.findViewById(R.id.txt);
		if(clickView == null) {
			return;
		}
		intent.putExtra("Edit_Content", clickView.getText().toString());
		intent.putExtra("voipAccount", voipAccount);
		intent.putExtra(KEY_GROUP_ID, groupId);
		startActivityForResult(intent, GroupDetailActivity.REQUEST_CODE_EDIT_GROUP_CARD);
	}
	
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);

		Log4Util.d(CCPHelper.DEMO_TAG ,"[GroupDetailActivity] onActivityResult: requestCode=" + requestCode
				+ ", resultCode=" + resultCode + ", data=" + data);

		// If there's no data (because the user didn't select a number and
		// just hit BACK, for example), there's nothing to do.
		if (requestCode != GroupDetailActivity.REQUEST_CODE_EDIT_GROUP_CARD) {
			if (data == null) {
				return;
			}
		} else if (resultCode != RESULT_OK) {
			Log4Util.d(CCPHelper.DEMO_TAG ,"[GroupDetailActivity] onActivityResult: bail due to resultCode=" + resultCode);
			return;
		}
		
		switch (requestCode) {
		case GroupDetailActivity.REQUEST_CODE_EDIT_GROUP_CARD:
			if(data.hasExtra("Edit_Content")) {
				Bundle extras = data.getExtras();
				if (extras != null) {
					String editContent = extras.getString("Edit_Content");
					
					if(clickView != null ) {
						clickView.setText(editContent);
						clickView = null;
					}
				}
			}
			
			break;
		default:
			break;
		}
	}

}
