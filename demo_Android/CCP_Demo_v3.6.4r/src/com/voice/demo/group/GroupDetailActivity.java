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

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.InputType;
import android.text.TextUtils;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.group.model.IMGroup;
import com.voice.demo.group.model.IMMember;
import com.voice.demo.interphone.InviteInterPhoneActivity;
import com.voice.demo.outboundmarketing.RestHelper.ERequestState;
import com.voice.demo.sqlite.CCPSqliteManager;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.tools.net.ITask;
import com.voice.demo.tools.net.TaskKey;
import com.voice.demo.ui.CCPHelper;

/**
 * View group information, including members of view and delete members, 
 * empty the group chat message .
 * @version Time: 2013-7-18
 */
public class GroupDetailActivity extends GroupBaseActivity implements View.OnClickListener , OnItemLongClickListener
																								,OnItemClickListener{
	private static final int TAB_ALREADY_EXISTS_IN_GROUP = 0x1;
	private static final int TAB_NOT_TO_JOIN_GROUP = 0x2;
	private static final int TAB_GROUP_CARD = 0x3;
	private static final int TAB_GROUP_DETAIL = 0x4;
	
	private int currentTabModel = TAB_NOT_TO_JOIN_GROUP;
	private int tabType = TAB_GROUP_DETAIL;
	private EditText mGroupNotice;
	private GridView mGroupGridView;
	private Button mClearMesg;
	private Button mQuitGroup;
	
	private LinearLayout mApplyLy;
	private LinearLayout mQuitorClear;
	private LinearLayout mSwitchingLy;
	private LinearLayout mGroupCardLy;
	private LinearLayout mCardItemLy;
	private RelativeLayout mGroupInfoLy;
	private Button mApplyJoinBtn;
	private TextView mMembersTips;
	
	private String groupId;
	private IMGroup mGroup;
	private GroupDetailAdapter mGridViewApapter;
	private boolean isCloseDialog = false;
	private int delPosition = -1 ;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.layout_group_detail_activity);
		
		initialize(savedInstanceState);
		
		handleTitleDisplay(getString(R.string.btn_title_back), groupId, getString(R.string.right_button_modify_group));
		
		// The default in the Edit button.
		getTitleRightButton().setVisibility(View.INVISIBLE);
		findViewById(R.id.title_group_detail).setOnClickListener(this);
		findViewById(R.id.title_group_card).setOnClickListener(this);
		mSwitchingLy = (LinearLayout) findViewById(R.id.switching_ly);
		
		mGroupCardLy = (LinearLayout) findViewById(R.id.group_card_ly);
		mGroupInfoLy = (RelativeLayout) findViewById(R.id.group_info_ly);
		mCardItemLy = (LinearLayout) findViewById(R.id.group_card_layout);
		
		layoutParams = new LinearLayout.LayoutParams(
				LinearLayout.LayoutParams.MATCH_PARENT,
				LinearLayout.LayoutParams.WRAP_CONTENT);
		initResource();
		
		registerReceiver(new String[]{
				CCPIntentUtils.INTENT_REMOVE_FROM_GROUP ,
				CCPIntentUtils.INTENT_JOIN_GROUP_SUCCESS});
		
	}

	private void initResource() {
		
		if(tabType == TAB_GROUP_CARD) {
			mGroupCardLy.setVisibility(View.VISIBLE);
			mGroupInfoLy.setVisibility(View.GONE);
			if(CurrentCard == null) {
				showConnectionProgress(getString(R.string.dialog_load_hold));
				ITask iTask = new ITask(RestGroupManagerHelper.KEY_QUERY_GROUPCARD);
				addTask(iTask);
			}
		} else {
			mGroupInfoLy.setVisibility(View.VISIBLE);
			mGroupCardLy.setVisibility(View.GONE);
			mGroupNotice = (EditText) findViewById(R.id.group_notice_input);
			
			mGroupNotice.addTextChangedListener(mTextEditorWatcher);
			mGroupGridView = (GridView) findViewById(R.id.member_list_gd);
			//mGroupGridView.setOnItemLongClickListener(this);
			mGroupGridView.setOnItemClickListener(this);
			registerForContextMenu(mGroupGridView);
			
			mClearMesg = (Button) findViewById(R.id.clear_msg_btn);
			mClearMesg.setEnabled(true);
			mQuitGroup = (Button) findViewById(R.id.quit_group_btn);
			mQuitGroup.setEnabled(true);
			
			mClearMesg.setOnClickListener(this);
			mQuitGroup.setOnClickListener(this);
			
			mMembersTips = (TextView) findViewById(R.id.gmembers_tips);
			mApplyJoinBtn = (Button) findViewById(R.id.apply_join);
			mApplyJoinBtn.setOnClickListener(this);
			mApplyLy = (LinearLayout) findViewById(R.id.apply_join_ly);
			mQuitorClear = (LinearLayout) findViewById(R.id.bottom_ly);
			
			if(currentTabModel == TAB_ALREADY_EXISTS_IN_GROUP) {
				
				// Hide application to join the layout
				mApplyLy.setVisibility(View.GONE);
				mMembersTips.setText(R.string.str_group_members_tips);
				
				// The display group member layout and quit the group 
				// and group information in the clear button
				mGroupGridView.setVisibility(View.VISIBLE);
				mQuitorClear.setVisibility(View.VISIBLE);
				
				
			} else {
				mApplyLy.setVisibility(View.VISIBLE);
				mMembersTips.setText(R.string.str_not_group_members);
				
				mGroupGridView.setVisibility(View.GONE);
				
				// To hide the bottom emptying and back button
				mQuitorClear.setVisibility(View.GONE);
				
				
			}
			
			initGroupDetail();
		}
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
		
		if (intent.hasExtra("isJoin")) {
			Bundle extras = intent.getExtras();
			if (extras != null) {
				boolean isJoinExist = (Boolean) extras.get("isJoin");
				if(isJoinExist) {
					currentTabModel = TAB_ALREADY_EXISTS_IN_GROUP;
				}
			}
		}
		
		if(TextUtils.isEmpty(groupId)) {
			Toast.makeText(getApplicationContext(), R.string.toast_group_id_error, Toast.LENGTH_SHORT).show();
			finish();
		}
		
		
	}

	private void initGroupDetail() {
		// According to the group ID and group membership information 
		// bulletin loading
		showConnectionProgress(getString(R.string.dialog_load_group_info));
		ITask iTask = new ITask(RestGroupManagerHelper.KEY_QUERY_GROUP_INFO);
		addTask(iTask);
	}
	
	private static final int REQUEST_CODE_INVITE_MEMEBER = 0x1;
	
	ArrayList<String> number ;
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		

		Log4Util.d(CCPHelper.DEMO_TAG ,"[GroupDetailActivity] onActivityResult: requestCode=" + requestCode
				+ ", resultCode=" + resultCode + ", data=" + data);

		// If there's no data (because the user didn't select a number and
		// just hit BACK, for example), there's nothing to do.
		if (requestCode != REQUEST_CODE_INVITE_MEMEBER || requestCode != REQUEST_CODE_EDIT_GROUP_CARD) {
			if (data == null) {
				return;
			}
		} else if (resultCode != RESULT_OK) {
			Log4Util.d(CCPHelper.DEMO_TAG ,"[GroupDetailActivity] onActivityResult: bail due to resultCode=" + resultCode);
			return;
		}
		
		switch (requestCode) {
		case REQUEST_CODE_INVITE_MEMEBER:
			//ArrayList<String> number = null;
			if(data.hasExtra("to")) {
				Bundle extras = data.getExtras();
				if (extras != null) {
					number = extras.getStringArrayList("to");
				}
			}
			
			if(number == null || number.isEmpty()) {
				CCPApplication.getInstance().showToast(R.string.toast_invite_group_empty);
				return ;
			}
			
			
			//showInvitationDialog(number.toArray(new String[]{}));
			showEditTextDialog(DIALOG_SHOW_KEY_CHECKBOX
					, InputType.TYPE_CLASS_TEXT |InputType.TYPE_TEXT_FLAG_MULTI_LINE 
					, true
					, 3
					, getString(R.string.dialog_title_invite_info)
					, getString(R.string.dialog_message_invite));
			break;
		case REQUEST_CODE_EDIT_GROUP_CARD:
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
	
	
	@Override
	protected void handleTitleAction(int direction) {
		
		if(direction == TITLE_RIGHT_ACTION) {
			showConnectionProgress(getString(R.string.dialod_message_modify_group));
			if(mGroup != null ) {
				
				ITask iTask = new ITask(RestGroupManagerHelper.KEY_MODIFY_GROUP);
				addTask(iTask);
			}
		} else {
			super.handleTitleAction(direction);
		}
		
	}
	
	/********************************************************************
	 *         The rest interface to implement the callback code        *
	 ********************************************************************/
	
	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		int key = iTask.getKey();
		if(key == RestGroupManagerHelper.KEY_QUERY_GROUP_INFO) {
			RestGroupManagerHelper.getInstance().queryGroupWithGroupId(groupId);
			if(currentTabModel == TAB_ALREADY_EXISTS_IN_GROUP) {
				
				// If exists in this group, group member is loaded or not loaded
				RestGroupManagerHelper.getInstance().queryMembersOfGroup(groupId);
			}
		} else if (key == RestGroupManagerHelper.KEY_MODIFY_GROUP) {
			
			// Modify group information
			RestGroupManagerHelper.getInstance().modifyGroup(groupId, mGroup.name
					, mGroupNotice.getText().toString(), Integer.parseInt(mGroup.permission));
			
		} else if (key == RestGroupManagerHelper.KEY_QUIT_GROUP) {
			
			// Exit the group and to delete the database group chat log
			RestGroupManagerHelper.getInstance().quitGroup(groupId);
		}else if(key == RestGroupManagerHelper.KEY_DELETE_GROUP){
			
			// delete the group and to delete the database group chat log
			RestGroupManagerHelper.getInstance().deleteGroup(groupId);
			
		} else if (key == RestGroupManagerHelper.KEY_DEL_MEMBER_OF_GROUP) {
			
			// Delete group members
			String[] members = (String[]) iTask.getTaskParameters("members");
			RestGroupManagerHelper.getInstance().deleteMembersFromGroup(groupId, members);
			
		} else if (key == RestGroupManagerHelper.KEY_INVITE_JOIN_GROUP) {
			
			// Invite members to join the group
			String declared = (String) iTask.getTaskParameters("declared");
			String confirm = (String) iTask.getTaskParameters("confirm");
			String[] members = (String[]) iTask.getTaskParameters("members");
			RestGroupManagerHelper.getInstance().inviteSomebodyJoinGroup(groupId
					, CCPConfig.VoIP_ID
					, members
					, declared
					, confirm);				
			
		} else if (key == RestGroupManagerHelper.KEY_QUERY_MEMBERS_GROUP) {
			
			// query group membership information
			RestGroupManagerHelper.getInstance().queryMembersOfGroup(groupId);
			
		} else if (key == RestGroupManagerHelper.KEY_JOIN_GROUP) {  // join group 
			String groupId = (String) iTask.getTaskParameters(KEY_GROUP_ID);
			String applyReason = (String) iTask.getTaskParameters("applyReason");
			RestGroupManagerHelper.getInstance().joinGroup(groupId, applyReason);
			
		}  else if ( key == RestGroupManagerHelper.KEY_FORBIDS_PEAK) {
			
			// The group members gag operation
			String groupId = (String) iTask.getTaskParameters(KEY_GROUP_ID);
			String member = (String) iTask.getTaskParameters("member");
			int operation = (Integer) iTask.getTaskParameters("operation");
			RestGroupManagerHelper.getInstance().ForbidSpeakForUser(groupId, member, operation);
			
		} else if ( key == RestGroupManagerHelper.KEY_QUERY_GROUPCARD) {
			
			// Group membership card information query
			IMMember imMember = new IMMember();
			imMember.belong = groupId;
			imMember.voipAccount = CCPConfig.VoIP_ID;
			RestGroupManagerHelper.getInstance().queryGroupCard(imMember);
			
		} else if (key == TaskKey.TASK_KEY_DEL_MESSAGE) {
			try {
				ArrayList<String> iFileLocalPaths = CCPSqliteManager.getInstance().queryIMMessageFileLocalPathBySession(mGroup.groupId);
				CCPSqliteManager.getInstance().deleteIMMessageBySessionId(mGroup.groupId);
				if(iFileLocalPaths != null) {
					for(String filePath : iFileLocalPaths) {
						CCPUtil.delFile(filePath);
					}
				}
				sendIMinitBroadcast(CCPIntentUtils.INTENT_DELETE_GROUP_MESSAGE);
				runOnUiThread(new Runnable() {
					
					@Override
					public void run() {
						Toast.makeText(getApplicationContext(), R.string.toast_clear_all_group_message, Toast.LENGTH_SHORT).show();
						
					}
				});
				closeConnectionProgress();
			} catch (SQLException e) {
				e.printStackTrace();
			}
		}
	}
	
	
	
	@Override
	protected void handleQueryMembersOfGroup(ERequestState reason,
			ArrayList<IMMember> members) {
		super.handleQueryMembersOfGroup(reason, members);
		
		if(isCloseDialog) {
			closeConnectionProgress();
		} else {
			isCloseDialog = true;
		}
		if(reason == ERequestState.Success) {
			
			if(mGroup == null) {
				return;
			}
			mGridViewApapter = new GroupDetailAdapter(getApplicationContext(), members);
			mGroupGridView.setAdapter(mGridViewApapter);
		}else{
			
			// When removed out of the group members when the query returns 110095 permissions problems
			currentTabModel = TAB_NOT_TO_JOIN_GROUP;
			mApplyLy.setVisibility(View.VISIBLE);
			mMembersTips.setText(R.string.str_not_group_members);
			
			mGroupGridView.setVisibility(View.GONE);
			// To hide the bottom emptying and back button
			mQuitorClear.setVisibility(View.GONE);
		}
	}
	
	
	@Override
	protected void handleQueryGroupWithGroupId(ERequestState reason,
			IMGroup group) {
		super.handleQueryGroupWithGroupId(reason, group);
		
		if(isCloseDialog || currentTabModel == TAB_NOT_TO_JOIN_GROUP  ) {
			closeConnectionProgress();
		} else {
			isCloseDialog = true;
		}
		
		if(reason == ERequestState.Success) {
			mGroup = group;
			
			//setActivityTitle(group.name);
			if(!TextUtils.isEmpty(group.declared)) {
				mGroupNotice.setText(group.declared);
				mGroupNotice.setSelection(group.declared.length());
			}
			if(!TextUtils.isEmpty(group.owner) && CCPConfig.VoIP_ID.equals(group.owner)) {
				
				mQuitGroup.setText(R.string.str_group_dissolution);
				registerForContextMenu(mGroupGridView);
			} else {
				mQuitGroup.setText(R.string.str_group_quit);
				unregisterForContextMenu(mGroupGridView);
			}
			
			if(CCPConfig.VoIP_ID.equals(group.owner)) {
				mGroupNotice.setEnabled(true);
				getTitleRightButton().setVisibility(View.VISIBLE);
			} else {
				mGroupNotice.setEnabled(false);
				getTitleRightButton().setVisibility(View.INVISIBLE);
			}
		}
	}
	
	@Override
	protected void handleDeleteMembersFromGroup(ERequestState reason) {
		super.handleDeleteMembersFromGroup(reason);
		
		closeConnectionProgress();
		if(reason == ERequestState.Success) {
			if(mGridViewApapter != null && delPosition >= 0) {
				IMMember item = mGridViewApapter.getItem(delPosition);
				if(item == null ) {
					return;
				}
				mGridViewApapter.remove(item);
				Log4Util.d(CCPHelper.DEMO_TAG, "[GroupDetailActivity - handleDeleteMembersFromGroup]" +
						"remove user " + item.voipAccount + " from group that groupid is :" + groupId);
			}
			
		} else {
			Toast.makeText(getApplicationContext(), R.string.toast_delete_group_member_error, Toast.LENGTH_SHORT).show();
		}
		
	}
	
	
	@Override
	protected void handleInviteSomebodyJoinGroup(ERequestState reason) {
		super.handleInviteSomebodyJoinGroup(reason);
		
		closeConnectionProgress();
		if(reason == ERequestState.Success) {
			/*if(mGridViewApapter != null ) {
				for (String userVoip : number) {
					
					mGridViewApapter.insert(userVoip, mGridViewApapter.getCount() - 1);
				}
			}*/
			
			ITask iTask = new ITask(RestGroupManagerHelper.KEY_QUERY_MEMBERS_GROUP);
			addTask(iTask);
			
		}
	}
	
	
	@Override
	protected void handleQuitGroup(ERequestState reason) {
		super.handleQuitGroup(reason);
		closeConnectionProgress();
		if(reason == ERequestState.Success) {
			try {
				
				// When leaving the group delete database records 
				// and notify the loading list
				CCPSqliteManager.getInstance().deleteIMMessageBySessionId(mGroup.groupId);
				//sendBroadcast(new Intent(INTENT_QUIT_GROUP));
				sendIMinitBroadcast(CCPIntentUtils.INTENT_REMOVE_FROM_GROUP);
				this.finish();
			} catch (SQLException e) {
				e.printStackTrace();
			}
		} else {
			
			// failed ..
			// TODO
		}
	}
	
	@Override
	protected void handleDeleteGroup(ERequestState reason) {
		super.handleDeleteGroup(reason);
		closeConnectionProgress();
		if(reason == ERequestState.Success) {
			try {
				
				// When leaving the group delete database records 
				// and notify the loading list
				CCPSqliteManager.getInstance().deleteIMMessageBySessionId(mGroup.groupId);
				//sendBroadcast(new Intent(INTENT_QUIT_GROUP));
				sendIMinitBroadcast(CCPIntentUtils.INTENT_REMOVE_FROM_GROUP);
				this.finish();
			} catch (SQLException e) {
				e.printStackTrace();
			}
		} else {
			
			// failed ..
			// TODO
		}
	}
	private void sendIMinitBroadcast(String action) {
		Intent intent = new Intent(action);
		intent.putExtra(KEY_GROUP_ID, groupId);
		sendBroadcast(intent);
	}
	
	
	@Override
	protected void handleModifyGroup(ERequestState reason) {
		super.handleModifyGroup(reason);
		
		closeConnectionProgress();
		if(reason == ERequestState.Success) {
			CCPApplication.getInstance().showToast(R.string.toast_message_modify_group_success);
		}
		
	}
	
	
	@Override
	protected void handleJoinGroup(ERequestState reason) {
		super.handleJoinGroup(reason);
		if(reason == ERequestState.Success) {
			Log4Util.d(CCPHelper.DEMO_TAG , "[GroupListActivity- handleJoinGroup]" +
					"The successful application sending authentication request ..");
			
			if(mGroup.permission.equals(IMGroup.MODEL_GROUP_PUBLIC + "")) {
				currentTabModel = TAB_ALREADY_EXISTS_IN_GROUP;
				//initResource();
				
				mGroup.count = Integer.parseInt(mGroup.count) + 1 + "";
				Intent intent = new Intent(CCPIntentUtils.INTENT_JOIN_GROUP_SUCCESS);
				intent.putExtra("IMGroup", mGroup);
				sendBroadcast(intent);
				return;
			}
			
			CCPApplication.getInstance().showToast(R.string.toast_wait_auth_success);
			closeConnectionProgress();
			
		} else {
			Log4Util.d(CCPHelper.DEMO_TAG , "[GroupListActivity- handleJoinGroup]" +
				"Validation failed to send request ..");
			
			closeConnectionProgress();
		}
	}
	

	/********************************************************************
	 *                                end                               *
	 ********************************************************************/
	private TextView clickView;
	@Override
	public void onClick(View v) {
		ITask iTask =null;
		switch (v.getId()) {
		case R.id.clear_msg_btn:
			
			showConnectionProgress(getString(R.string.str_dialog_message_default));
			iTask = new ITask(TaskKey.TASK_KEY_DEL_MESSAGE);
			addTask(iTask);
			
			break;
		case R.id.quit_group_btn:
			
			showConnectionProgress(getString(R.string.str_dialog_message_default));
			if(!TextUtils.isEmpty(mGroup.owner) && CCPConfig.VoIP_ID.equals(mGroup.owner)){
				iTask = new ITask(RestGroupManagerHelper.KEY_DELETE_GROUP);
				addTask(iTask);
			} else {
				iTask = new ITask(RestGroupManagerHelper.KEY_QUIT_GROUP);
				addTask(iTask);
			}
			break;
			
		case R.id.apply_join:
			
			if(mGroup != null && mGroup.permission.equals(IMGroup.MODEL_GROUP_AUTHENTICATION + "")) {
			
				//showAuthenticationDialog();
				showEditTextDialog(DIALOG_SHOW_KEY_INVITE
						, InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_FLAG_MULTI_LINE 
						, 3
						, getString(R.string.dialog_title_auth)
						, getString(R.string.dialog_message_auth_reason));
				
				return ;
			} 
			
			applyJoinGroup(null);
		
			
			break;
			
		case R.id.title_group_detail:
			if(tabType == TAB_GROUP_DETAIL) {
				return;
			}
			getTitleRightButton().setVisibility(View.VISIBLE);
			tabType = TAB_GROUP_DETAIL;
			mSwitchingLy.setBackgroundResource(R.drawable.title_group_left);
			initResource();
			break;
			
		case R.id.title_group_card:
			
			doGroupCardClick();
			
			break;
			
			
		case GroupCardSetting.GROUP_CARD_NAME:
		case GroupCardSetting.GROUP_CARD_TELEPHONE:
		case GroupCardSetting.GROUP_CARD_MAIL:
		case GroupCardSetting.GROUP_CARD_SIGNATURE:
			Intent intent = new Intent(GroupDetailActivity.this, GroupEditActivity.class);
			intent.putExtra("Edit_Tag", v.getId());
			clickView = (TextView) v.findViewById(R.id.group_card_raw_detail);
			if(clickView == null || CurrentCard == null) {
				return;
			}
			
			intent.putExtra("Edit_Content", clickView.getText().toString());
			intent.putExtra(KEY_GROUP_ID, groupId);
			intent.putExtra("voipAccount", CurrentCard.voipAccount);
			startActivityForResult(intent, REQUEST_CODE_EDIT_GROUP_CARD);
			break;
		default:
			break;
		}
		
	}

	private void doGroupCardClick() {
		if(tabType == TAB_GROUP_CARD) {
			return;
		}
		getTitleRightButton().setVisibility(View.INVISIBLE);
		// If not a member of the group is to apply to join the group
		if(currentTabModel != TAB_ALREADY_EXISTS_IN_GROUP) {
			return;
		}
		
		tabType = TAB_GROUP_CARD;
		// init group card resource ..
		initResource();
		mSwitchingLy.setBackgroundResource(R.drawable.title_group_right);
	}
	
	private static final int REQUEST_KEY_DELETE_GROUP_MEMBERS = 0x1;
	public static final int REQUEST_CODE_EDIT_GROUP_CARD = 0x2;
	@Override
	public boolean onItemLongClick(AdapterView<?> parent, View view,
			int position, long id) {
		if(mGridViewApapter != null ) {
			
			if(position == mGridViewApapter.getCount() - 1) {
				return false;
			}
			
			 IMMember mImMember = mGridViewApapter.getItem(position);
			if(mImMember == null) {
				return false;
			}
			String mUserName = mImMember.voipAccount;
			delPosition = position;
			showAlertTipsDialog(REQUEST_KEY_DELETE_GROUP_MEMBERS
					, getString(R.string.dialod_title_remove_user)
					, getString(R.string.dialod_message_remove_user, mUserName)
					, getString(R.string.dialog_btn)
					, getString(R.string.dialog_cancle_btn));
		}
			
		return true;
	}
	
	
	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		
		if (mGridViewApapter != null) {
			if (CCPConfig.VoIP_ID.equals(mGroup.owner)) {
				if (position == mGridViewApapter.getCount() - 1) {
					Intent intent = new Intent(GroupDetailActivity.this,
							InviteInterPhoneActivity.class);
					intent.putExtra("create_to",
							InviteInterPhoneActivity.MULTIPLE_CHOICE);
					startActivityForResult(intent, REQUEST_CODE_INVITE_MEMEBER);
					return;
				}
			}
			IMMember item = mGridViewApapter.getItem(position);
			
			if(item.voipAccount.equals(CCPConfig.VoIP_ID)) {
				doGroupCardClick();
			} else {
				
				Intent intent = new Intent(GroupDetailActivity.this,
						GroupMemberCardActivity.class);
				intent.putExtra(KEY_GROUP_ID, groupId);
				intent.putExtra("voipAccount", item.voipAccount);
				if(CCPConfig.VoIP_ID.equals(mGroup.owner)) {
					intent.putExtra("modify", true);
				} else{
					intent.putExtra("modify", false);
				}
				startActivity(intent);
			}
			
			
			//}
		}
	}
	
	@Override
	protected void handleDialogOkEvent(int requestKey) {
		super.handleDialogOkEvent(requestKey);
		
		if(requestKey == REQUEST_KEY_DELETE_GROUP_MEMBERS) {
			if(mGridViewApapter != null ) {
				
				final IMMember mImMember = mGridViewApapter.getItem(delPosition);
				if(mImMember == null) {
					return;
				}
				String mUserName = mImMember.voipAccount;
				if(TextUtils.isEmpty(mUserName)) {
					Toast.makeText(getApplicationContext(), R.string.toast_group_click_error, Toast.LENGTH_SHORT).show();
					return ;
				}
				
				showConnectionProgress(getString(R.string.dialod_message_uer_removing));
				
				ITask iTask = new ITask(RestGroupManagerHelper.KEY_DEL_MEMBER_OF_GROUP);
				iTask.setTaskParameters("members", new String[]{mUserName});
				addTask(iTask);
			}
		}
	}
	
	
	@Override
	protected void handleEditDialogOkEvent(int requestKey, String editText,
			boolean checked) {
		super.handleEditDialogOkEvent(requestKey, editText, checked);
		
		if(requestKey == DIALOG_SHOW_KEY_INVITE) {
			showConnectionProgress(getString(R.string.dialog_message_send_apply_reason));
			final String applyReason = editText;
			if(!TextUtils.isEmpty(applyReason)){
				applyJoinGroup(applyReason);
			}else{
				applyJoinGroup(" ");
			}
		} else if (requestKey == DIALOG_SHOW_KEY_CHECKBOX) {
			showConnectionProgress(getString(R.string.dialog_message_send_apply_reason));
			final String applyReason = editText;
			final String isNeedSure = checked? "0" : "1";
			// invite this phone call ...
			ITask iTask = new ITask(RestGroupManagerHelper.KEY_INVITE_JOIN_GROUP);
			iTask.setTaskParameters("declared", applyReason == null ? "" : applyReason);
			iTask.setTaskParameters("confirm", isNeedSure);
			iTask.setTaskParameters("members", number.toArray(new String[]{}));
			addTask(iTask);
		}
		
	}
	
	@Override
	protected void handleDialogCancelEvent(int requestKey) {
		super.handleDialogCancelEvent(requestKey);
		
		if(number != null ) {
			number.clear();
			number = null;
		}
	}
	
	@Override
	public void handleForbidSpeakForUser(ERequestState reason) {
		super.handleForbidSpeakForUser(reason);
		closeConnectionProgress();
		if(ERequestState.Success == reason) {
			if(mGridViewApapter != null && delPosition >= 0) {
				IMMember imMember = mGridViewApapter.getItem(delPosition);
				mGridViewApapter.remove(imMember);
				imMember.isBan = imMember.isBan == 0 ? 1 :0;;
				mGridViewApapter.insert(imMember, delPosition);
			}
		}
	}
	
	class GroupDetailAdapter extends ArrayAdapter<IMMember> {

		LayoutInflater mInflater;
		public GroupDetailAdapter(Context context,List<IMMember> iMsString) {
			super(context, 0, iMsString);
			
			mInflater = getLayoutInflater();
		}

		@Override
		public int getCount() {
			if(CCPConfig.VoIP_ID.equals(mGroup.owner)) {
				return super.getCount() + 1;
			}
			
			return super.getCount();
		}
		
		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			
			VoiceHolder holder;
			if (convertView == null|| convertView.getTag() == null) {
				convertView = mInflater.inflate(R.layout.list_gird_view_item, null);
				holder = new VoiceHolder();
				
				holder.userName = (TextView) convertView.findViewById(R.id.item_text);
				holder.userIcon = (ImageView) convertView.findViewById(R.id.item_icon);
				holder.forbidden_speak = (ImageView) convertView.findViewById(R.id.forbidden_to_speak);
			} else {
				holder = (VoiceHolder) convertView.getTag();
			}
			
			if(CCPConfig.VoIP_ID.equals(mGroup.owner) && (getCount() - 1 == position)) {
				holder.userIcon.setImageResource(R.drawable.add_group_memebers_icon);
				holder.userName.setText("添加");
				return convertView;
			}
			IMMember iMember = getItem(position);
			if(iMember != null) {
				String displayName = "";
				String voipAccount = iMember.voipAccount;
				if(!TextUtils.isEmpty(voipAccount) && voipAccount.length() > 4) {
					displayName = voipAccount.substring(voipAccount.length() - 4 , voipAccount.length());
					
				}
					
				holder.userName.setText(displayName);
				if(iMember.isBan != 0 ) {
					if(CCPConfig.VoIP_ID.equals(mGroup.owner)) {
						// Only the administrator can see the user has been banned
						holder.forbidden_speak.setVisibility(View.VISIBLE) ;
					} else{
						holder.forbidden_speak.setVisibility(View.GONE) ;
					}
				} else{
					holder.forbidden_speak.setVisibility(View.GONE) ;
				}
			}
			
			
			
			return convertView;
		}
		
		
		class VoiceHolder {
			ImageView userIcon;
			TextView userName;
			ImageView forbidden_speak;
		}
	}
	
	/**
	 * 
	 * @param applyReason
	 */
	private void applyJoinGroup(String applyReason) {
		ITask iTask = new ITask(RestGroupManagerHelper.KEY_JOIN_GROUP);
		iTask.setTaskParameters(KEY_GROUP_ID, groupId);
		if(!TextUtils.isEmpty(applyReason)) {
			iTask.setTaskParameters("applyReason", applyReason);
		} 
		addTask(iTask);
	}
	
	@Override
	public void onCreateContextMenu(ContextMenu menu, View v,
			ContextMenuInfo menuInfo) {
		
		AdapterView.AdapterContextMenuInfo menuInfoIn;
        try {
        	menuInfoIn = (AdapterView.AdapterContextMenuInfo) menuInfo;
        } catch (ClassCastException e) {
        	
            return;
        }
        
        
        
        if(mGridViewApapter != null ) {
        	if(menuInfoIn.position == mGridViewApapter.getCount() - 1) {
        		return;
        	}
        	IMMember item = mGridViewApapter.getItem(menuInfoIn.position);
        	if(item == null || TextUtils.isEmpty(item.voipAccount)) {
        		return;
        	}
        	menu.setHeaderTitle(item.voipAccount);
        	if(!item.voipAccount.equals(CCPConfig.VoIP_ID)) {
        		menu.add(0, R.string.menu_delete_group, 0, getString(R.string.menu_delete_group));
	        	if(item.isBan == 0) {
	        		menu.add(0, R.string.menu_forbidden_to_speak, 0, getString(R.string.menu_forbidden_to_speak));
	        	} else {
	        		menu.add(0, R.string.menu_forbidden_to_speak_cancle, 0, getString(R.string.menu_forbidden_to_speak_cancle));
	        	}
        	}
        }
	}
	
	@Override
	public boolean onContextItemSelected(MenuItem item) {
		AdapterView.AdapterContextMenuInfo menuInfo;
		try {
			menuInfo = (AdapterView.AdapterContextMenuInfo) item.getMenuInfo();
		} catch (ClassCastException e) {
			return false;
		}
		
		if(mGridViewApapter != null) {
			final IMMember mImMember = mGridViewApapter.getItem(menuInfo.position);
			if(mImMember == null) {
				return super.onContextItemSelected(item);
			}
			delPosition = menuInfo.position;
			switch (item.getItemId()) {
			case R.string.menu_delete_group:
					
				showAlertTipsDialog(REQUEST_KEY_DELETE_GROUP_MEMBERS
						, getString(R.string.dialod_title_remove_user)
						, getString(R.string.dialod_message_remove_user, mImMember.voipAccount)
						, getString(R.string.dialog_btn)
						, getString(R.string.dialog_cancle_btn));
				break;
			
			case R.string.menu_forbidden_to_speak:
			case R.string.menu_forbidden_to_speak_cancle:
				int isBan = mImMember.isBan == 0 ? 1 : 0;
				/*int resourceId = R.array.member_memu_forbidden;
				if(isBan == 0) {
					resourceId = R.array.member_memu_forbidden_cancle;
				}
				AlertDialog.Builder builder = new AlertDialog.Builder(GroupDetailActivity.this);
			    builder.setTitle(mUserName)
			           .setItems(resourceId, new DialogInterface.OnClickListener() {
			               public void onClick(DialogInterface dialog, int which) {
			               // The 'which' argument contains the index position
			               // of the selected item
			            	   
			           }
			    });
			    builder.create().show();*/
				
				showConnectionProgress(getString(R.string.dialod_message_operationling));
				ITask iTask = new ITask(RestGroupManagerHelper.KEY_FORBIDS_PEAK);
				iTask.setTaskParameters(KEY_GROUP_ID, groupId);
				iTask.setTaskParameters("member", mImMember.voipAccount);
				iTask.setTaskParameters("operation", (Integer)isBan);
				addTask(iTask);
				break;
			default:
				break;
			}
		}
			
		return super.onContextItemSelected(item);

	}	
	
	@Override
	protected void onReceiveBroadcast(Intent intent) {
		super.onReceiveBroadcast(intent);
		
		if(CCPIntentUtils.INTENT_REMOVE_FROM_GROUP.equals(intent.getAction())) {
			this.finish();
		} else if (CCPIntentUtils.INTENT_JOIN_GROUP_SUCCESS.equals(intent.getAction())) {
			currentTabModel = TAB_ALREADY_EXISTS_IN_GROUP;
			initResource();
			CCPApplication.getInstance().showToast(R.string.toast_accept_join_group_op);
		}
	}
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		unregisterForContextMenu(mGroupGridView);
		groupId = null;
		mGroup = null;
		mGridViewApapter = null;
		isCloseDialog = false;
		delPosition = -1 ;
	}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	// ----------------------------------------- group card --
	private int[] title = {
			R.string.str_group_card_id,
			R.string.str_group_card_voipaccount,
			R.string.str_group_card_name,
			/*R.string.str_group_card_sex,
			R.string.str_group_card_birthday,*/
			R.string.str_group_card_telephone,
			R.string.str_group_card_mail,
			R.string.str_group_card_signature};
	
	public static class GroupCardSetting {
		public static final int GROUP_CARD_ID = 1;
		public static final int GROUP_CARD_VOIP= 2;
		public static final int GROUP_CARD_NAME = 3;
		//public static final int GROUP_CARD_SEX = 3;
		//public static final int GROUP_CARD_BIRTHDAY = 4;
		public static final int GROUP_CARD_TELEPHONE = 4;
		public static final int GROUP_CARD_MAIL = 5;
		public static final int GROUP_CARD_SIGNATURE = 6;
	}
	
	private LinearLayout.LayoutParams layoutParams = null;
	
	private IMMember CurrentCard;
	/**
	 * 创建一个View 并设置id
	 *
	 * @param layoutid
	 * @param id
	 * @return
	 */
	private View createItemView(int layoutid, int id) {
		View view = getLayoutInflater().inflate(layoutid, null);
		view.setId(id);
		int index = id-1;

		TextView textView = (TextView) view
				.findViewById(R.id.group_card_raw_desc);
		textView.setText(getString(title[index]));
		if(id != GroupCardSetting.GROUP_CARD_ID)
			view.setOnClickListener(this);
		return view;
	}
	
	/**
	 * 创建一条栏目
	 *
	 * @param id
	 */
	private void addItemSettingView(int id) {
		View view = createItemView(R.layout.list_group_card_item, id);
		TextView gCardDeatil = (TextView) view.findViewById(R.id.group_card_raw_detail);
		gCardDeatil.setSingleLine();
		ImageView gCardIcon = (ImageView) view.findViewById(R.id.group_card_raw_icon);
		if (id == GroupCardSetting.GROUP_CARD_ID) {
			gCardDeatil.setText(CurrentCard.belong);
			view.setBackgroundResource(R.drawable.group_card_header_selector);
			gCardIcon.setVisibility(View.INVISIBLE);
		} else if (id == GroupCardSetting.GROUP_CARD_SIGNATURE) {
			if(!TextUtils.isEmpty(CurrentCard.remark))
				gCardDeatil.setText(CurrentCard.remark);
			gCardDeatil.setSingleLine(false);
			gCardDeatil.setLines(4);
			view.setBackgroundResource(R.drawable.group_card_bottom_selector);
			view.findViewById(R.id.group_card_botton_line).setVisibility(View.GONE);
			gCardIcon.setVisibility(View.VISIBLE);
		} else {
			view.setBackgroundResource(R.drawable.group_card_middle_selector);
			gCardIcon.setVisibility(View.VISIBLE);
		}
		
		switch (id) {
		case GroupCardSetting.GROUP_CARD_VOIP:
			if(!TextUtils.isEmpty(CurrentCard.voipAccount))
				gCardDeatil.setText(CurrentCard.voipAccount);
			break;
		case GroupCardSetting.GROUP_CARD_NAME:
			if(!TextUtils.isEmpty(CurrentCard.displayName))
				gCardDeatil.setText(CurrentCard.displayName);
			break;
		/*case GroupCardSetting.GROUP_CARD_SEX:
			if(!TextUtils.isEmpty(CurrentCard.sex))
				gCardDeatil.setText(CurrentCard.sex);
			break;
		case GroupCardSetting.GROUP_CARD_BIRTHDAY:
			if(!TextUtils.isEmpty(CurrentCard.birth))
				gCardDeatil.setText(CurrentCard.birth);
			break;*/
		case GroupCardSetting.GROUP_CARD_TELEPHONE:
			if(!TextUtils.isEmpty(CurrentCard.tel))
				gCardDeatil.setText(CurrentCard.tel);
			break;
		case GroupCardSetting.GROUP_CARD_MAIL:
			if(!TextUtils.isEmpty(CurrentCard.mail))
				gCardDeatil.setText(CurrentCard.mail);
			break;

		default:
			break;
		}

		mCardItemLy.addView(view, layoutParams);

	}
	
	/**
	 *
	 * 初始化设置列表;
	 */
	private void initViewListView() {
		for (int i = 1; i <= GroupCardSetting.GROUP_CARD_SIGNATURE; i++) {
			addItemSettingView(i);
		}
	}
	
	@Override
	protected void handleQueryGroupCard(ERequestState reason , 	IMMember imMember) {
		super.handleQueryGroupCard(reason , imMember);
		closeConnectionProgress();
		if(reason == ERequestState.Success) {
			CurrentCard = imMember;
			if(imMember == null ) {
				return ;
			}
			initViewListView();
		}
	}
	
}
