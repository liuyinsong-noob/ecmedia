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

import java.io.InvalidClassException;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.R;
import com.voice.demo.group.model.IMGroup;
import com.voice.demo.outboundmarketing.RestHelper.ERequestState;
import com.voice.demo.sqlite.CCPSqliteManager;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.tools.net.ITask;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.CCPHelper;

/**
 * Group list, can I join the group and all the group list
 * @version Time: 2013-7-18
 */
public class GroupListActivity extends GroupBaseActivity implements OnItemClickListener ,View.OnClickListener{

	public static final int ITASK_DO_BACKGROUND = 0xA;
	
	private static final int TAB_JOIN_GROUP = 0x1;
	private static final int TAB_PUBLIC_GROUP = 0x2;
	
	private ListView mGroupListView;
	private LinearLayout mSwitchingLy;
	
	private GroupAdapter mGroupAdapter;
	
	public ArrayList<String> mReceiveJoinGroupId = new ArrayList<String>();
	public ArrayList<String> mJoinGroupId = new ArrayList<String>();
	public ArrayList<IMGroup> mJoinGroups;
	public ArrayList<IMGroup> mPubGroups;
	
	
	public int currentTab = TAB_JOIN_GROUP;
	private boolean mActivityPause = false;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.layout_group_list_activity);
		
		
		Button lButton = (Button) findViewById(R.id.voice_btn_back);
		lButton.setText(R.string.btn_title_back);
		lButton.setOnClickListener(this);
		lButton.setVisibility(View.VISIBLE);
		
		Button rButton = (Button) findViewById(R.id.voice_right_btn);
		rButton.setText(R.string.str_button_new_create_group);
		rButton.setOnClickListener(this);
		rButton.setVisibility(View.VISIBLE);
		
		
		
		initResource();
		
		initGroupList(currentTab);
		
		registerReceiver(new String[] {
				CCPIntentUtils.INTENT_REMOVE_FROM_GROUP, 
				CCPIntentUtils.INTENT_JOIN_GROUP_SUCCESS});
	}
	
	private TextView mGroupTitleTips;
	
	private void initResource() {
		mGroupListView = (ListView) findViewById(R.id.group_list_join);
		mGroupListView.setOnItemClickListener(this);
		
		if(currentTab == TAB_JOIN_GROUP)
			registerForContextMenu(mGroupListView);
		
		mSwitchingLy = (LinearLayout) findViewById(R.id.switching_ly);
		findViewById(R.id.title_join_group).setOnClickListener(this);
		findViewById(R.id.title_public_group).setOnClickListener(this);
		
		
		mGroupTitleTips = (TextView) findViewById(R.id.group_join_tips);
		
	}


	private void initGroupList(final int tab) {
		
		if(tab == TAB_JOIN_GROUP) {
			if(mJoinGroups == null ) {
				showConnectionProgress(getString(R.string.dialog_load_group_list));
				
				addTask(new ITask(RestGroupManagerHelper.KEY_QUERY_GROUPS_VOIP));
			} else {
				handleQueryGroupsOfVoip(ERequestState.Success, mJoinGroups);
			}
		} else {
			if(mPubGroups == null ) {
				showConnectionProgress(getString(R.string.dialog_load_group_list));
				addTask(new ITask(RestGroupManagerHelper.KEY_GET_PUBLIC_GROUPS_MSG));

			} else {
				handleGetPublicGroups(ERequestState.Success, mPubGroups);
			}
		}
		currentTab = tab;
	}
	
	
	@Override
	protected void handleQueryGroupsOfVoip(ERequestState reason,
			ArrayList<IMGroup> imGroups) {
		super.handleQueryGroupsOfVoip(reason, imGroups);
		
		closeConnectionProgress();
		if(reason == ERequestState.Success) {
			if(imGroups != null ) {
				mJoinGroups = imGroups;
				mGroupAdapter = new GroupAdapter(getApplicationContext(), mJoinGroups);
				mGroupListView.setAdapter(mGroupAdapter);
				
				try {
					CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_JOIN_GROUP_SIZE,  mJoinGroups.size(), true);
				} catch (InvalidClassException e) {
					e.printStackTrace();
				}
				
				ITask iTask = new ITask(ITASK_DO_BACKGROUND);
				iTask.setTaskParameters("mJoinGroups", mJoinGroups);
				addTask(iTask);
				
				mJoinGroupId.clear();
				for(IMGroup imGroup :imGroups) {
					if(imGroup != null && !TextUtils.isEmpty(imGroup.groupId)) {
						mJoinGroupId.add(imGroup.groupId);
					}
				}
			}
			
		}
		
	}
	
	@SuppressWarnings("unchecked")
	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		int key = iTask.getKey();
		if(key == RestGroupManagerHelper.KEY_QUERY_GROUPS_VOIP) { //query join group 
			RestGroupManagerHelper.getInstance().queryGroupsOfVoip();
		} else if (key == RestGroupManagerHelper.KEY_GET_PUBLIC_GROUPS_MSG) { // query pub group ..
			RestGroupManagerHelper.getInstance().getPublicGroups("0", 20);
		} else if (key == RestGroupManagerHelper.KEY_QUIT_GROUP) {
			String groupId = (String) iTask.getTaskParameters(KEY_GROUP_ID);
			RestGroupManagerHelper.getInstance().quitGroup(groupId);			
		} else if (key == ITASK_DO_BACKGROUND) {
			ArrayList<IMGroup> imGroups = (ArrayList<IMGroup>) iTask.getTaskParameters("mJoinGroups");
			checkGroupChange(imGroups);
		} else if (key == RestGroupManagerHelper.KEY_QUERY_GROUP_INFO) {
			String groupId = (String) iTask.getTaskParameters(KEY_GROUP_ID);
			RestGroupManagerHelper.getInstance().queryGroupWithGroupId(groupId);
		}
	}
	
	@Override
	protected void handleQueryGroupWithGroupId(ERequestState reason,
			IMGroup group) {
		super.handleQueryGroupWithGroupId(reason, group);
		
		if(reason == ERequestState.Success) {
			synchronized (GroupListActivity.class) {
				if(currentTab == TAB_JOIN_GROUP) {
					if(mGroupAdapter == null ) {
						ArrayList<IMGroup> imGroups = new ArrayList<IMGroup>();
						imGroups.add(group);
						mGroupAdapter = new GroupAdapter(getApplicationContext(), imGroups);
						mGroupListView.setAdapter(mGroupAdapter);
					} else {
						mGroupAdapter.insert(group, mGroupAdapter.getCount());
					}
				} else {
					mJoinGroups.add(group);
				}
			}
			
		}

	}
	
	
	
	@Override
	protected void handleGetPublicGroups(ERequestState reason,
			ArrayList<IMGroup> imGroupSummarys) {
		super.handleGetPublicGroups(reason, imGroupSummarys);
		
		Log4Util.v(CCPHelper.DEMO_TAG, "[GroupListActivity - handleGetPublicGroups]" +
				"query public group success ,then callback handleGetPublicGroups ,that reason " + reason);
		closeConnectionProgress();
		if(reason == ERequestState.Success) {
			if(imGroupSummarys != null ) {
				mPubGroups = imGroupSummarys;
				mGroupAdapter = new GroupAdapter(getApplicationContext(), mPubGroups);
				mGroupListView.setAdapter(mGroupAdapter);
				
				try {
					CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_PUB_GROUP_SIZE, mPubGroups.size(), true);
				} catch (InvalidClassException e) {
					e.printStackTrace();
				}
			}
		}
		
	}
	
	@Override
	protected void handleQuitGroup(ERequestState reason) {
		super.handleQuitGroup(reason);
		
		closeConnectionProgress();
		if(reason == ERequestState.Success) {
			if(delImGroup != null ) {
				mGroupAdapter.remove(delImGroup);
			}
		}
	}
	
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		
		
		if(mJoinGroups != null ) {
			mJoinGroups.clear();
			mJoinGroups = null;
		}
		if(mPubGroups != null ) {
			mPubGroups.clear();
			mPubGroups = null;
		}
		
		if(mJoinGroupId != null ) {
			mJoinGroupId.clear();
			mJoinGroupId = null;
		}
		if(mReceiveJoinGroupId != null ) {
			mReceiveJoinGroupId.clear();
			mReceiveJoinGroupId = null;
		}
	}

	@Override
	protected void onReceiveBroadcast(Intent intent) {
		super.onReceiveBroadcast(intent);
		
		if (CCPIntentUtils.INTENT_REMOVE_FROM_GROUP.equals(intent.getAction())) {
			
			// When leaving the group delete database records 
			// and notify the loading list
			
			String groupId = intent.getStringExtra(KEY_GROUP_ID);
			if(currentTab == TAB_JOIN_GROUP) {
				if(mGroupAdapter != null ) {
					for(int i = 0 ; i < mGroupAdapter.getCount(); i ++) {
						IMGroup imGroup = mGroupAdapter.getItem(i);
						if(imGroup != null && groupId.equals(imGroup.groupId)) {
							mGroupAdapter.remove(imGroup);
						}
					}
				} else {
					mJoinGroups = null;
					initGroupList(currentTab);
				}
				if(mPubGroups != null) {
					IMGroup group = null;
					for(IMGroup imGroup : mPubGroups) {
						if(groupId.equals(imGroup.groupId)) {
							group = imGroup;
							break;
						}
					}
					if(group != null) {
						mPubGroups.remove(group);
						group = null;
					}
				}
			} else {
				if(mJoinGroups != null ) {
					
					for(IMGroup imGroup :mJoinGroups) {
						if(imGroup != null && groupId.equals(imGroup.groupId)) {
							mJoinGroups.remove(imGroup);
						}
					}
				} else {
					initGroupList(currentTab);
				}
				
				if(mPubGroups != null) {
					IMGroup group = null;
					for(IMGroup imGroup : mPubGroups) {
						if(groupId.equals(imGroup.groupId)) {
							group = imGroup;
							break;
						}
					}
					if(mGroupAdapter != null ) {
						mGroupAdapter.remove(group);
					}
				}
			}
			mJoinGroupId.remove(groupId);
			
		} else if (CCPIntentUtils.INTENT_JOIN_GROUP_SUCCESS.equals(intent.getAction())) {
			if(intent.hasExtra(KEY_GROUP_ID)) {
				String groupId = intent.getStringExtra(KEY_GROUP_ID);
				if(mActivityPause) { 
					mReceiveJoinGroupId.add(groupId);
				} else {
					addQueryGroupInfoTask(groupId);
				}
				mJoinGroupId.add(groupId);
			} else if (intent.hasExtra("IMGroup")) {
				IMGroup imGroup= (IMGroup) intent.getSerializableExtra("IMGroup");
				
				handleQueryGroupWithGroupId(ERequestState.Success, imGroup);
				mJoinGroupId.add(imGroup.groupId);
				
				if(intent.hasExtra("isCreate")) {
					boolean isCreate = intent.getBooleanExtra("isCreate", false);
					if(isCreate) {
						if(currentTab == TAB_PUBLIC_GROUP) {
							if(mGroupAdapter == null ) {
								ArrayList<IMGroup> imGroups = new ArrayList<IMGroup>();
								imGroups.add(0,imGroup);
								mGroupAdapter = new GroupAdapter(getApplicationContext(), imGroups);
								mGroupListView.setAdapter(mGroupAdapter);
							} else {
								mGroupAdapter.insert(imGroup, 0);
							}
						} else {
							if(mPubGroups != null ) {
								mPubGroups.add(0,imGroup);
							}
							
						}
					}
				}
			}
			
			
		}
	}
	
	
	@Override
	protected void onResume() {
		super.onResume();
		mActivityPause = false;
		if(mReceiveJoinGroupId != null && !mReceiveJoinGroupId.isEmpty()){
			for(String groupId : mReceiveJoinGroupId) {
				addQueryGroupInfoTask(groupId);
			}
			
			mReceiveJoinGroupId.clear();
		}
	}


	private void addQueryGroupInfoTask(String groupId) {
		ITask iTask = new ITask(RestGroupManagerHelper.KEY_QUERY_GROUP_INFO);
		iTask.setTaskParameters(KEY_GROUP_ID, groupId);
		addTask(iTask);
	}
	
	@Override
	protected void onPause() {
		super.onPause();
		mActivityPause = true;
	}
	
	// The group list adapter, display a list of all the group information
	class GroupAdapter extends ArrayAdapter<IMGroup> {

		LayoutInflater mInflater;
		public GroupAdapter(Context context,List<IMGroup> imGroups) {
			super(context, 0, imGroups);
			
			mInflater = getLayoutInflater();
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			
			VoiceHolder holder;
			if (convertView == null|| convertView.getTag() == null) {
				convertView = mInflater.inflate(R.layout.list_group_item, null);
				holder = new VoiceHolder();
				
				holder.groupName = (TextView) convertView.findViewById(R.id.group_name);
				//holder.groupNotice = (TextView) convertView.findViewById(R.id.group_notice);
				holder.lockIcon = (ImageView) convertView.findViewById(R.id.group_lock);
				holder.groupDesc = (TextView) convertView.findViewById(R.id.group_desc);
			} else {
				holder = (VoiceHolder) convertView.getTag();
			}
			
			IMGroup imGroup = getItem(position);
			if(imGroup != null) {
				holder.groupName.setText(imGroup.name);
				//holder.groupNotice.setText(imGroup.declared);
				
				if ("1".equals(imGroup.permission) && currentTab == TAB_PUBLIC_GROUP) {
					holder.lockIcon.setVisibility(View.VISIBLE);
				} else {
					holder.lockIcon.setVisibility(View.GONE);
				}
				
				if(isJoin(imGroup)) {
					holder.groupDesc.setText(getString(R.string.str_group_list_desc_join, imGroup.count));
				} else {
					holder.groupDesc.setText(getString(R.string.str_group_list_desc_unjoin, imGroup.count));
				}
				
			}
			
			
			return convertView;
		}
		
		
		class VoiceHolder {
			TextView groupName;
			TextView groupNotice;
			ImageView lockIcon;
			TextView groupDesc;
		}
	}


	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		
		if(mGroupAdapter != null ) {
			IMGroup item = mGroupAdapter.getItem(position);
			if(item != null && !TextUtils.isEmpty(item.groupId))  {
				
				boolean isJoin = isJoin(item);
				Intent intent = null;
				if(isJoin) {
					intent = new Intent(GroupListActivity.this, GroupChatActivity.class);
					intent.putExtra("groupName", item.name);
				} else {
					intent = new Intent(GroupListActivity.this, GroupDetailActivity.class);
					intent.putExtra("isJoin", isJoin);
				}
				intent.putExtra(KEY_GROUP_ID, item.groupId);
				startActivity(intent);
			}
			
		} else {
			
			// failed ..
			Toast.makeText(getApplicationContext(), R.string.toast_click_into_group_error, Toast.LENGTH_SHORT).show();
		}
	}


	private boolean isJoin(IMGroup item) {
		
		if(item == null 
				|| TextUtils.isEmpty(item.groupId) 
				|| !mJoinGroupId.contains(item.groupId)) {
			return false;
		}
		
		return true;
	}
	
	
	private IMGroup delImGroup = null ;
	@Override
	public void onCreateContextMenu(ContextMenu menu, View v,
			ContextMenuInfo menuInfo) {
		
		AdapterView.AdapterContextMenuInfo menuInfoIn;
        try {
        	menuInfoIn = (AdapterView.AdapterContextMenuInfo) menuInfo;
        } catch (ClassCastException e) {
        	
            return;
        }
        
        if(mGroupAdapter != null ) {
        	IMGroup item = mGroupAdapter.getItem(menuInfoIn.position);
        	menu.setHeaderTitle(item.groupId);
        	menu.add(0, R.string.menu_delete_group, 0, getString(R.string.menu_delete_group));
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
		
		if(mGroupAdapter != null) {
			final IMGroup imGroup = mGroupAdapter.getItem(menuInfo.position);
			switch (item.getItemId()) {
			case R.string.menu_delete_group:
				showConnectionProgress(getString(R.string.dialod_message_uer_removing));
				delImGroup = imGroup;
				ITask iTask = new ITask(RestGroupManagerHelper.KEY_QUIT_GROUP);
				iTask.setTaskParameters(KEY_GROUP_ID, imGroup.groupId);
				addTask(iTask);
				break;
				
			default:
				break;
			}
		}
			
		return super.onContextItemSelected(item);

	}


	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.voice_btn_back:
			
			finish();
			break;
		case R.id.voice_right_btn:
			startActivity(new Intent(GroupListActivity.this, CreateGroupActivity.class));
			break;
			
		case R.id.title_join_group:
			if(currentTab == TAB_JOIN_GROUP) {
				return;
			}
			
			registerForContextMenu(mGroupListView);
			initGroupList(TAB_JOIN_GROUP);
			mGroupTitleTips.setText(R.string.str_group_list_join_tips);
			mSwitchingLy.setBackgroundResource(R.drawable.title_group_left);
			break;
		case R.id.title_public_group:
			if(currentTab == TAB_PUBLIC_GROUP) {
				return;
			}
			
			initGroupList(TAB_PUBLIC_GROUP);
			unregisterForContextMenu(mGroupListView);
			mGroupTitleTips.setText(R.string.str_group_list_pub_tips);
			mSwitchingLy.setBackgroundResource(R.drawable.title_group_right);
			break;

		default:
			break;
		}
	}
	
	
	void checkGroupChange(ArrayList<IMGroup> imGroups){
		
		try {
			ArrayList<IMGroup> noExistImGroups = new ArrayList<IMGroup>();
			for (IMGroup imGroup : imGroups) {
				String groupId = CCPSqliteManager.getInstance().isExistsGroupId(imGroup.groupId);
				if(TextUtils.isEmpty(groupId)) {
					noExistImGroups.add(imGroup);
					continue;
				}
			}
			
			CCPSqliteManager.getInstance().insertIMGroupInfos(noExistImGroups);
		} catch (SQLException e) {
			e.printStackTrace();
		}
	}
	
}
