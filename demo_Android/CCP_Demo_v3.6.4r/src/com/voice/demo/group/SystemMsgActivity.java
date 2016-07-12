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
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import com.voice.demo.R;
import com.voice.demo.group.model.IMSystemMessage;
import com.voice.demo.outboundmarketing.RestHelper.ERequestState;
import com.voice.demo.sqlite.CCPSqliteManager;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.tools.net.ITask;

/**
 * The system notification interface
 * 1, the user application to join the group
 * 2, the user received the invitation to join the group
 * 
 * @version Time: 2013-7-22
 */
public class SystemMsgActivity extends GroupBaseActivity implements OnItemClickListener{

	private TextView mNoticeTips;
	private ListView mSystemMsgListView;
	
	private SystemMsgAdapter mSystemAdapter;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.layout_system_msg_activity);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_system_notice)
				, getString(R.string.btn_clear_all_text));
		
		InitResource();
		
		initialize();
		
		registerReceiver(new String[]{CCPIntentUtils.INTENT_RECEIVE_SYSTEM_MESSAGE});
	}
	
	
	private void initialize() {
		try {
			// Set as read
			ArrayList<IMSystemMessage> imSystemMessages = CCPSqliteManager.getInstance().queryNoticeMessages();
			if(imSystemMessages != null ) {
				mSystemAdapter = new SystemMsgAdapter(getApplicationContext(), imSystemMessages);
				mSystemMsgListView.setAdapter(mSystemAdapter);
				
			} else {
				mSystemMsgListView.setAdapter(null);
			}
			
			
			int num = CCPSqliteManager.getInstance().queryNoticeMessageUnreadNum();
			if(num > 0) {
				mNoticeTips.setText(getString(R.string.str_notice_unread_message, num + ""));
			}
			
			CCPSqliteManager.getInstance().updateAllNoticeMessageStatus(IMSystemMessage.STATE_READED);
			
		} catch (Exception e) {
			e.printStackTrace();
		}
	}


	private void InitResource() {
		mNoticeTips = (TextView) findViewById(R.id.notice_tips);
		mSystemMsgListView = (ListView) findViewById(R.id.systemmsg_list_content);
		
		mSystemMsgListView.setOnItemClickListener(this);
		LinearLayout empty = (LinearLayout) findViewById(R.id.system_list_empty);
		mSystemMsgListView.setEmptyView(empty);
	}


	@Override
	protected void handleTitleAction(int direction) {
		if(direction == TITLE_RIGHT_ACTION) {
			
			// clear all the system message ..
			// TODO
			try {
				CCPSqliteManager.getInstance().deleteAllNoticeMessage();
				sendDateChangeBroadcast();
			} catch (SQLException e) {
				e.printStackTrace();
			}
		} else {
			super.handleTitleAction(direction);
		}
	}


	private void sendDateChangeBroadcast() {
		Intent intent = new Intent(CCPIntentUtils.INTENT_RECEIVE_SYSTEM_MESSAGE);
		sendBroadcast(intent);
	}
	
	String currentMessageId = null;
	int operationType = -1;
	@Override
	protected void handleVerifyJoinGroup(ERequestState reason) {
		super.handleVerifyJoinGroup(reason);
		
		modifyLocalMsgReadStatus(reason);
	}

	
	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		int confirm = 1;
		if(operationType == IMSystemMessage.SYSTEM_MESSAGE_THROUGH) {
			confirm = 0;
		} else {
			confirm = 1;
		}
		int key = iTask.getKey();
		if(key == RestGroupManagerHelper.KEY_VERIFY_JOIN_GROUP) {
			
			String groupId = (String) iTask.getTaskParameters(KEY_GROUP_ID);
			String asker = (String) iTask.getTaskParameters("asker");
			RestGroupManagerHelper.getInstance().verifyJoinGroup(groupId, asker, confirm);
		} else  if (key == RestGroupManagerHelper.KEY_ANSWER_INVITE_GROUP) {
			String groupId = (String) iTask.getTaskParameters(KEY_GROUP_ID);
			RestGroupManagerHelper.getInstance().answerInviteGroup(groupId, confirm);
		}
		
		
	}

	private void modifyLocalMsgReadStatus(ERequestState reason) {
		closeConnectionProgress();
		if(reason == ERequestState.Success) {
			if(currentMessageId != null ) {
				try {
					CCPSqliteManager.getInstance().updateNoticeMessageOperationStatus(currentMessageId, 
							operationType == IMSystemMessage.SYSTEM_MESSAGE_THROUGH ? IMSystemMessage.SYSTEM_MESSAGE_THROUGH :IMSystemMessage.SYSTEM_MESSAGE_REFUSE);
					
					initialize();
				} catch (SQLException e) {
					e.printStackTrace();
				}
			}
		}
	}
	
	@Override
	protected void handleAnswerInviteGroup(ERequestState reason) {
		super.handleAnswerInviteGroup(reason);
		modifyLocalMsgReadStatus(reason);
	}
	
	
	
	
	// The group list adapter, display a list of all the group information
	class SystemMsgAdapter extends ArrayAdapter<IMSystemMessage> {

		LayoutInflater mInflater;
		public SystemMsgAdapter(Context context,List<IMSystemMessage> imGroups) {
			super(context, 0, imGroups);
			
			mInflater = getLayoutInflater();
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			
			SystemMsgHolder holder;
			if (convertView == null|| convertView.getTag() == null) {
				convertView = mInflater.inflate(R.layout.list_item_system_msg, null);
				holder = new SystemMsgHolder();
				
				holder.msgType = (TextView) convertView.findViewById(R.id.msg_type);
				holder.nickname = (TextView) convertView.findViewById(R.id.user_nickname);
				holder.msgTime = (TextView) convertView.findViewById(R.id.msg_time);
				holder.sysMsgFrom = (TextView) convertView.findViewById(R.id.sysMsg_from);
				holder.resultShow = (TextView) convertView.findViewById(R.id.result_show);
				holder.resultSummary = (TextView) convertView.findViewById(R.id.result_summary);
				holder.acceptBtn = (Button) convertView.findViewById(R.id.accept_btn);
				holder.refuseBtn = (Button) convertView.findViewById(R.id.Refuse_btn);
				holder.operationLy = (LinearLayout) convertView.findViewById(R.id.operation_ly);
			} else {
				holder = (SystemMsgHolder) convertView.getTag();
			}
			
			final IMSystemMessage imSystemMessage = getItem(position);
			if(imSystemMessage != null ) {
				
				if(imSystemMessage.getMessageType() == IMSystemMessage.SYSTEM_TYPE_APPLY_JOIN) {
					holder.msgType.setText(R.string.str_system_message_type_apply);
				} else if (imSystemMessage.getMessageType() == IMSystemMessage.SYSTEM_TYPE_INVITE_JOIN) {
					holder.msgType.setText(R.string.str_system_message_type_invite);
				} else if (imSystemMessage.getMessageType() == IMSystemMessage.SYSTEM_TYPE_REMOVE
						|| imSystemMessage.getMessageType() == IMSystemMessage.SYSTEM_TYPE_ACCEPT_OR_REJECT_JOIN
						|| imSystemMessage.getMessageType() == IMSystemMessage.SYSTEM_TYPE_APPLY_JOIN_UNVALIDATION 
						|| imSystemMessage.getMessageType() == IMSystemMessage.SYSTEM_TYPE_GROUP_MEMBER_QUIT) {
					holder.msgType.setText(R.string.str_system_message_group_notice);
				}
				
				
				holder.nickname.setText(imSystemMessage.getWho());
				holder.msgTime.setText(imSystemMessage.getCurDate());
				holder.resultSummary.setText(imSystemMessage.getVerifyMsg());
				holder.sysMsgFrom.setText(getString(R.string.str_system_come_from, imSystemMessage.getGroupId()));
				
				
				
				if(imSystemMessage.getState() == IMSystemMessage.SYSTEM_MESSAGE_NEED_REPLAY) {
					
					// System information about the invitation to join the group 
					// or join the group needs to operate, Whether is it right? Read or unread, 
					// as long as the state has not operation can display the operating button
					holder.operationLy.setVisibility(View.VISIBLE);
					holder.resultShow.setVisibility(View.GONE);
					
				} else {
					// Other notice about information, only need to display 
					// without the need to have relevant operation
					holder.operationLy.setVisibility(View.GONE);
					holder.resultShow.setVisibility(View.VISIBLE);
					if (imSystemMessage.getState() == IMSystemMessage.SYSTEM_MESSAGE_REFUSE) {
						holder.resultShow.setText(R.string.str_system_message_operation_result_refuse);
					} else if (imSystemMessage.getState() == IMSystemMessage.SYSTEM_MESSAGE_THROUGH) {
						holder.resultShow.setText(R.string.str_system_message_operation_result_through);
						
					} else{
						holder.resultShow.setVisibility(View.GONE);
					}
				}
				
				
				holder.acceptBtn.setOnClickListener(new OnClickListener() {
					
					@Override
					public void onClick(View v) {
						// 
						OperationGroupSystemMsg(IMSystemMessage.SYSTEM_MESSAGE_THROUGH , imSystemMessage);
					}
				});
				holder.refuseBtn.setOnClickListener(new OnClickListener() {
					
					@Override
					public void onClick(View v) {
						OperationGroupSystemMsg(IMSystemMessage.SYSTEM_MESSAGE_REFUSE , imSystemMessage);
					}
				});
			}
			
			
			return convertView;
		}
		
		/**
		 * 处理接受或者拒绝邀请
		 * @param isAccept
		 * @param imSystemMessage
		 */
		protected void OperationGroupSystemMsg(final int isAccept,final  IMSystemMessage imSystemMessage) {
			showConnectionProgress(getString(R.string.dialod_message_operationling));
			synchronized (SystemMsgActivity.class) {
				
				currentMessageId = imSystemMessage.getMessageId();
				operationType = isAccept;
				
				ITask iTask = new ITask();
				
				if((imSystemMessage.getMessageType() == IMSystemMessage.SYSTEM_TYPE_APPLY_JOIN)) {
					iTask.setKey(RestGroupManagerHelper.KEY_VERIFY_JOIN_GROUP);
					
				} else if(imSystemMessage.getMessageType() == IMSystemMessage.SYSTEM_TYPE_INVITE_JOIN) {
					iTask.setKey(RestGroupManagerHelper.KEY_ANSWER_INVITE_GROUP);
				}
				iTask.setTaskParameters(KEY_GROUP_ID, imSystemMessage.getGroupId());
				iTask.setTaskParameters("asker", imSystemMessage.getWho());
				
				addTask(iTask);
			}
		}


		class SystemMsgHolder {
			LinearLayout operationLy;
			TextView msgType;
			TextView resultShow;
			TextView nickname;
			TextView sysMsgFrom;
			TextView msgTime;
			TextView resultSummary;
			Button acceptBtn; // accetp
			Button refuseBtn;
		}
	}


	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		
	}
	
	
	@Override
	protected void onReceiveBroadcast(Intent intent) {
		super.onReceiveBroadcast(intent);
		
		if (CCPIntentUtils.INTENT_RECEIVE_SYSTEM_MESSAGE.equals(intent.getAction())
				|| CCPIntentUtils.INTENT_REMOVE_FROM_GROUP.equals(intent.getAction())
				|| CCPIntentUtils.INTENT_JOIN_GROUP_SUCCESS.equals(intent.getAction())){
			//update UI...
			initialize();
		}
	}
}
