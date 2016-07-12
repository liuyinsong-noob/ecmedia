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
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.group.baseui.CCPTextView;
import com.voice.demo.group.model.IMConversation;
import com.voice.demo.group.utils.EmoticonUtil;
import com.voice.demo.interphone.InviteInterPhoneActivity;
import com.voice.demo.sqlite.CCPSqliteManager;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.tools.net.ITask;
import com.voice.demo.tools.net.TaskKey;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;

/**
 * IM, group chat entrance interface
 * Display all the IM, group chat sessions and the system notification list
 * 
 * @version Time: 2013-6-15
 */
public class GroupMessageListActivity extends GroupBaseActivity  implements View.OnClickListener ,OnItemClickListener{
	
	private TextView mContactNum;
	private TextView mGroupNum;
	
	private LinearLayout mGroupTopContentLy;
	private ListView mGroupListLv;
	private LinearLayout mGroupListEmpty;
	
	private IMConvAdapter mIMAdapter;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_group_msg_list)
				, getString(R.string.btn_clear_all_text));
		
		initResourceRefs();
		
		// init emoji .
		EmoticonUtil.initEmoji();
		
		//initConversation();
		
		// regist .
		registerReceiver(new String[]{CCPIntentUtils.INTENT_IM_RECIVE
				,CCPIntentUtils.INTENT_DELETE_GROUP_MESSAGE
				,CCPIntentUtils.INTENT_REMOVE_FROM_GROUP
				,CCPIntentUtils.INTENT_RECEIVE_SYSTEM_MESSAGE
				,CCPIntentUtils.INTENT_JOIN_GROUP_SUCCESS});
	}

	private void initResourceRefs() {
		
		mGroupTopContentLy = (LinearLayout) findViewById(R.id.group_info_base_content);
		
		// The contact item
		View inflate = getLayoutInflater().inflate(R.layout.list_item_group_base, null);
		inflate.setId(R.drawable.im_contact_icon);
		inflate.setOnClickListener(this);
		((ImageView) inflate.findViewById(R.id.icon)).setImageResource(R.drawable.im_contact_icon);
		((TextView) inflate.findViewById(R.id.tv_name)).setText(R.string.str_contact);
		mContactNum = ((TextView) inflate.findViewById(R.id.group_count_tv));
		mGroupTopContentLy.addView(inflate);
		
		
		// The group item
		inflate = getLayoutInflater().inflate(R.layout.list_item_group_base, null);
		inflate.setId(R.drawable.group_icon);
		inflate.setOnClickListener(this);
		
		((ImageView) inflate.findViewById(R.id.icon)).setImageResource(R.drawable.group_icon);
		((TextView) inflate.findViewById(R.id.tv_name)).setText(R.string.str_group);
		mGroupNum = ((TextView) inflate.findViewById(R.id.group_count_tv));
		
		mGroupTopContentLy.addView(inflate);
		
		if(CCPConfig.VoIP_ID_LIST != null) {
			String[] split = CCPConfig.VoIP_ID_LIST.split(",");
			if(split != null ) {
				int size = split.length > 0?split.length - 1:0;
				mContactNum.setText( "(" + size + ")");
			}
		}
		
		initListView();
	}

	private void initListView() {
		mGroupListLv = (ListView) findViewById(R.id.group_list_content);
		mGroupListLv.setOnItemClickListener(this);
		mGroupListEmpty = (LinearLayout) findViewById(R.id.group_list_empty);
		mGroupListLv.setEmptyView(mGroupListEmpty);
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		SharedPreferences cPreferences = CcpPreferences.getSharedPreferences();
		int joinNum = cPreferences
				.getInt(CCPPreferenceSettings.SETTING_JOIN_GROUP_SIZE.getId(),
						(Integer)CCPPreferenceSettings.SETTING_JOIN_GROUP_SIZE
								.getDefaultValue());
		int pubNum = cPreferences.getInt(
				CCPPreferenceSettings.SETTING_PUB_GROUP_SIZE.getId(),
				(Integer) CCPPreferenceSettings.SETTING_PUB_GROUP_SIZE
						.getDefaultValue());
		if(joinNum != 0 || pubNum != 0) {
			mGroupNum.setText("(" + joinNum + "/" + pubNum + ")");
		}
		
		initConversation();
	}
	
	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		int key = iTask.getKey();
		if(key == TaskKey.TASK_KEY_DEL_MESSAGE) {
			// delete all IM message and del local file also.
			try {
				CCPSqliteManager.getInstance().deleteAllIMMessage();
				CCPUtil.delAllFile(CCPApplication.getInstance().getVoiceStore().getAbsolutePath());
				CCPSqliteManager.getInstance().deleteAllNoticeMessage();
				sendBroadcast(new Intent(CCPIntentUtils.INTENT_IM_RECIVE));
				closeConnectionProgress();
			} catch (SQLException e) {
				e.printStackTrace();
			}
		}
	}
	
	@Override
	protected void handleTitleAction(int direction) {
		
		if(direction == TITLE_RIGHT_ACTION) {
			showConnectionProgress(getString(R.string.str_dialog_message_default));
			ITask iTask = new ITask(TaskKey.TASK_KEY_DEL_MESSAGE);
			addTask(iTask);
		} else {
			super.handleTitleAction(direction);
		}
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.drawable.im_contact_icon:
			
			//startActivity(new Intent(GroupMessageListActivity.this, IMChatActivity.class));
			
			Intent intent = new Intent(GroupMessageListActivity.this, InviteInterPhoneActivity.class);
			intent.putExtra("create_to", InviteInterPhoneActivity.CREATE_TO_IM_TALK);
			startActivity(intent);
			
			break;
			
		case R.drawable.group_icon:
			
			startActivity(new Intent(GroupMessageListActivity.this, GroupListActivity.class));
			
			break;
		default:
			break;
		}
	}
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		
		EmoticonUtil.getInstace().release();
		
	}

	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		
		IMConversation vSession = mIMAdapter.getItem(position);
		if(vSession != null ) {
			Intent intent = null;
			if(vSession.getType() == IMConversation.CONVER_TYPE_SYSTEM) {
				intent = new Intent(GroupMessageListActivity.this, SystemMsgActivity.class) ;
				
			} else {
				intent = new Intent(GroupMessageListActivity.this, GroupChatActivity.class) ;
				intent.putExtra(KEY_GROUP_ID, vSession.getId());
				intent.putExtra("groupName", vSession.getContact());
			}
			startActivity(intent);
		}
		
	}
	
	
	class IMConvAdapter extends ArrayAdapter<IMConversation> {

		LayoutInflater mInflater;
		public IMConvAdapter(Context context,List<IMConversation> iMList) {
			super(context, 0, iMList);
			
			mInflater = getLayoutInflater();
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			
			IMConvHolder holder;
			if (convertView == null|| convertView.getTag() == null) {
				convertView = mInflater.inflate(R.layout.im_chat_list_item, null);
				holder = new IMConvHolder();
				
				holder.avatar = (ImageView) convertView.findViewById(R.id.avatar);
				holder.name = (TextView) convertView.findViewById(R.id.name);
				holder.updateTime = (TextView) convertView.findViewById(R.id.update_time);
				holder.iLastMessage = (CCPTextView) convertView.findViewById(R.id.im_last_msg);
				holder.newCount = (TextView) convertView.findViewById(R.id.im_unread_count);
				holder.newCountLy = (LinearLayout) convertView.findViewById(R.id.unread_count_ly);
			} else {
				holder = (IMConvHolder) convertView.getTag();
			}
			
			IMConversation iSession = getItem(position);
			if(iSession != null) {
				
				if(iSession.getType() == IMConversation.CONVER_TYPE_SYSTEM) {
					
					holder.avatar.setImageResource(R.drawable.system_messages_icon);
				} else if (iSession.getType() == IMConversation.CONVER_TYPE_MESSAGE) {
					if(iSession.getId().startsWith("g")) {
						holder.avatar.setImageResource(R.drawable.group_icon);
					} else {
						holder.avatar.setImageResource(R.drawable.sigle_icon);
					}
				}

				holder.name.setText(iSession.getContact());
				holder.updateTime.setText(iSession.getDateCreated());
				
				if(!TextUtils.isEmpty(iSession.getUnReadNum()) && !"0".equals(iSession.getUnReadNum())) {
					holder.newCount.setText(iSession.getUnReadNum());
					holder.newCount.setVisibility(View.VISIBLE);
				} else{
					holder.newCount.setVisibility(View.GONE);
				}
				
				holder.iLastMessage.setEmojiText(iSession.getRecentMessage());
			}
			
			
			return convertView;
		}
		
		
		class IMConvHolder {
			
			ImageView avatar;
			TextView name;
			TextView updateTime;
			
			CCPTextView iLastMessage;
			TextView newCount;
			LinearLayout newCountLy;
		}
	}
	
	
	class IMMsgAsyncTask extends AsyncTask<Void, Void, ArrayList<IMConversation>>{

		@Override
		protected ArrayList<IMConversation> doInBackground(Void... params) {
				try {
					return CCPSqliteManager.getInstance().queryIMConversation();
				} catch (Exception e) {
					e.printStackTrace();
				}
			return null;
		}
		
		@Override
		protected void onPostExecute(ArrayList<IMConversation> result) {
			super.onPostExecute(result);
			if(result != null && !result.isEmpty()){
				//
				mIMAdapter = new IMConvAdapter(getApplicationContext(), result);
				mGroupListLv.setAdapter(mIMAdapter);
				mGroupListEmpty.setVisibility(View.GONE);
			} else {
				//
				mGroupListLv.setAdapter(null);
				mGroupListEmpty.setVisibility(View.VISIBLE);
			}
		}
	}
	
	
	@Override
	protected void onReceiveBroadcast(Intent intent) {
		super.onReceiveBroadcast(intent);
		if (intent != null && (CCPIntentUtils.INTENT_IM_RECIVE.equals(intent.getAction())
				|| CCPIntentUtils.INTENT_DELETE_GROUP_MESSAGE.equals(intent.getAction())
				|| CCPIntentUtils.INTENT_REMOVE_FROM_GROUP.equals(intent.getAction()))
				|| CCPIntentUtils.INTENT_RECEIVE_SYSTEM_MESSAGE.equals(intent.getAction())
				|| CCPIntentUtils.INTENT_JOIN_GROUP_SUCCESS.equals(intent.getAction())) {
			//update UI...
			initConversation();
		}
		
		
	}
	
	
	@Override
	protected int getLayoutId() {
		return R.layout.layout_im_group_activity;
	}
	

	private void initConversation() {
		new IMMsgAsyncTask().execute();
	}

}
