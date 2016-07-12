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

import java.util.ArrayList;
import java.util.HashMap;

import android.app.ProgressDialog;
import android.os.Bundle;
import android.os.Message;
import android.text.TextUtils;

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.R;
import com.voice.demo.group.model.IMGroup;
import com.voice.demo.group.model.IMMember;
import com.voice.demo.outboundmarketing.RestHelper.ERequestState;
import com.voice.demo.tools.net.ITask;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.ui.CCPHelper;

/**
 * group base activity .
 * All associated with group rest interface callback operation, 
 * and then to call the subclass implementation method
 * @version Time: 2013-7-19
 */
public class GroupBaseActivity extends CCPBaseActivity implements RestGroupManagerHelper.onRestGroupManagerHelpListener {

	public static final String KEY_GROUP_ID = "groupId";
	public static final String KEY_MESSAGE_ID = "messageId";
	
	private final android.os.Handler handler = new android.os.Handler() {
		@SuppressWarnings("unchecked")
		public void handleMessage(Message msg) {
			
			if(msg.what == WHAT_SHOW_PROGRESS) {
				pDialog = new ProgressDialog(GroupBaseActivity.this);
				pDialog.setTitle(R.string.str_dialog_title);
				pDialog.setMessage(getString(R.string.str_dialog_message_default));
				pDialog.setCanceledOnTouchOutside(false);
				String message = (String) msg.obj;
				if(!TextUtils.isEmpty(message))
					pDialog.setMessage(message);
				pDialog.show();
				Log4Util.d(CCPHelper.DEMO_TAG, "dialog  show");
			} else if (msg.what == WHAT_CLOSE_PROGRESS) {
				if(pDialog != null && pDialog.isShowing()) {
					pDialog.dismiss();
					pDialog = null;
				}
				Log4Util.d(CCPHelper.DEMO_TAG, "dialog  dismiss");
			} else {
				// Normally we would do some work here.
				Bundle b = (Bundle) msg.obj;
				int what = msg.arg1;
				ERequestState reason = (ERequestState) b.getSerializable("ERequestState");
				Log4Util.i(CCPHelper.DEMO_TAG, "What: " + what);
				
				
				switch (what) {
				case RestGroupManagerHelper.KEY_CREATE_GROUP:
					
					String groupId = b.getString(KEY_GROUP_ID);
					handleCreateGroup(reason, groupId);
					
					break;
				case RestGroupManagerHelper.KEY_MODIFY_GROUP:
					handleModifyGroup(reason);
					
					break;
				case RestGroupManagerHelper.KEY_DELETE_GROUP:
					
					handleDeleteGroup(reason);
					
					break;
				case RestGroupManagerHelper.KEY_QUERY_GROUP_INFO:
					
					IMGroup group = (IMGroup) b.getSerializable("IMGroup");
					handleQueryGroupWithGroupId(reason, group);
					
					break;
				case RestGroupManagerHelper.KEY_JOIN_GROUP:
					
					handleJoinGroup(reason);
					break;
				case RestGroupManagerHelper.KEY_INVITE_JOIN_GROUP:
					
					handleInviteSomebodyJoinGroup(reason);
					
					break;
				case RestGroupManagerHelper.KEY_DEL_MEMBER_OF_GROUP:
					
					handleDeleteMembersFromGroup(reason);
					
					break;
				case RestGroupManagerHelper.KEY_QUIT_GROUP:
					
					handleQuitGroup(reason);
					
					break;
				case RestGroupManagerHelper.KEY_ADD_GROUPCARD:
					
					handleAddGroupCard(reason);
					break;
				case RestGroupManagerHelper.KEY_MODIFY_GROUPCARD:
					
					handleModifyGroupCard(reason);
					break;
				case RestGroupManagerHelper.KEY_QUERY_GROUPCARD:
					IMMember iMember = (IMMember) b.getSerializable("IMMember");
					handleQueryGroupCard(reason , iMember);
					break;
				case RestGroupManagerHelper.KEY_QUERY_MEMBERS_GROUP:
					
					ArrayList<IMMember> members = (ArrayList<IMMember>) b.getSerializable("members");
					handleQueryMembersOfGroup(reason, members);
					
					break;
				case RestGroupManagerHelper.KEY_QUERY_GROUPS_VOIP:
					ArrayList<IMGroup> imGroups = (ArrayList<IMGroup>) b.getSerializable("imGroups");
					handleQueryGroupsOfVoip(reason, imGroups);
					break;
				case RestGroupManagerHelper.KEY_VERIFY_JOIN_GROUP:
					
					handleVerifyJoinGroup(reason);
					break;
				case RestGroupManagerHelper.KEY_ANSWER_INVITE_GROUP:
					
					handleAnswerInviteGroup(reason);
					break;
				case RestGroupManagerHelper.KEY_SET_RULE_GROUP_MSG:
					
					handleSetRuleOfReceiveGroupMsg(reason);
					break;
					
				case RestGroupManagerHelper.KEY_GET_PUBLIC_GROUPS_MSG:
					    ArrayList<IMGroup> imGroupList =  (ArrayList<IMGroup>) b.getSerializable("IMGroupSummary");
					    handleGetPublicGroups(reason, imGroupList);
					break;
					
				case RestGroupManagerHelper.KEY_FORBIDS_PEAK:
					
					handleForbidSpeakForUser(reason);
					
					break;
				default:
					doHandleExpertCallback(msg);
					break;
				}
			};
		}
	};
	
	/**
	 * @return the handler
	 */
	public android.os.Handler getHandler() {
		return handler;
	}
	
	
	public final Message getHandleMessage() {
		// For each start request, send a message to start a job and deliver the
		// start ID so we know which request we're stopping when we finish the
		// job
		Message msg = getHandler().obtainMessage();
		return msg;
	}

	public final void sendHandleMessage(Message msg) {
		getHandler().sendMessage(msg);
	}

	private ProgressDialog pDialog = null;
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		//ThreadPoolManager.getInstance().setOnTaskDoingLinstener(this);
		
	}
	
	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPBaseActivity#onDestroy()
	 */
	@Override
	protected void onDestroy() {
		RestGroupManagerHelper.getInstance().setOnRestGroupManagerHelpListener(null);
		super.onDestroy();
	}
	
	/**
	 * when sub class can't show progress, then you can override it.
	 */
	public void showConnectionProgress(String messageContent) {
		Message message = Message.obtain();
		message.obj = messageContent;
		message.what = WHAT_SHOW_PROGRESS;
		handler.sendMessage(message);
	}

	public void closeConnectionProgress() {
		handler.sendEmptyMessage(WHAT_CLOSE_PROGRESS);
	}
	
	
	@Override
	public void onCreateGroup(ERequestState reason, String groupId) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    b.putString(KEY_GROUP_ID, groupId);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_CREATE_GROUP;
	    sendHandleMessage(msg);
	}

	@Override
	public void onModifyGroup(ERequestState reason) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_MODIFY_GROUP;
	    sendHandleMessage(msg);
	}

	@Override
	public void onDeleteGroup(ERequestState reason) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_DELETE_GROUP;
	    sendHandleMessage(msg);
	}

	@Override
	public void onQueryGroupWithGroupId(ERequestState reason, IMGroup group) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    b.putSerializable("IMGroup", group);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_QUERY_GROUP_INFO;
	    sendHandleMessage(msg);
	    
	}

	@Override
	public void onJoinGroup(ERequestState reason) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_JOIN_GROUP;
	    sendHandleMessage(msg);
	}

	@Override
	public void onInviteSomebodyJoinGroup(ERequestState reason) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_INVITE_JOIN_GROUP;
	    sendHandleMessage(msg);
	    
	}

	@Override
	public void onDeleteMembersFromGroup(ERequestState reason) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_DEL_MEMBER_OF_GROUP;
	    sendHandleMessage(msg);
	    
	}

	@Override
	public void onQuitGroup(ERequestState reason) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_QUIT_GROUP;
	    sendHandleMessage(msg);
	    
	}

	@Override
	public void onAddGroupCard(ERequestState reason) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_ADD_GROUPCARD;
	    sendHandleMessage(msg);
	    
	}

	@Override
	public void onModifyGroupCard(ERequestState reason) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_MODIFY_GROUPCARD;
	    sendHandleMessage(msg);
	    
	}

	@Override
	public void onQueryGroupCard(ERequestState reason, IMMember member) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    b.putSerializable("IMMember", member);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_QUERY_GROUPCARD;
	    sendHandleMessage(msg);
	}

	@Override
	public void onQueryMembersOfGroup(ERequestState reason,
			ArrayList<IMMember> members) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    b.putSerializable("members", members);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_QUERY_MEMBERS_GROUP;
	    sendHandleMessage(msg);
	    
	}

	@Override
	public void onQueryGroupsOfVoip(ERequestState reason,
			ArrayList<IMGroup> imGroups) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    b.putSerializable("imGroups", imGroups);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_QUERY_GROUPS_VOIP;
	    sendHandleMessage(msg);
	    
	}

	@Override
	public void onVerifyJoinGroup(ERequestState reason) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_VERIFY_JOIN_GROUP;
	    sendHandleMessage(msg);
	}

	@Override
	public void onAnswerInviteGroup(ERequestState reason) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_ANSWER_INVITE_GROUP;
	    sendHandleMessage(msg);
	}

	@Override
	public void onSetRuleOfReceiveGroupMsg(ERequestState reason) {
		
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_SET_RULE_GROUP_MSG;
	    sendHandleMessage(msg);
	}
	
	@Override
	public void onGetPublicGroups(ERequestState reason,
			ArrayList<IMGroup> imGroupSummarys) {

		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    b.putSerializable("IMGroupSummary", imGroupSummarys);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_GET_PUBLIC_GROUPS_MSG;
	    sendHandleMessage(msg);
	}

	
	
	protected void handleCreateGroup(ERequestState reason, String groupId) {
		
	}
	
	protected void handleModifyGroup(ERequestState reason) {
		
	}
	
	protected void handleDeleteGroup(ERequestState reason) {
		
	}
	
	protected void handleQueryGroupWithGroupId(ERequestState reason, IMGroup group) {
		
	}
	
	protected void handleJoinGroup(ERequestState reason) {
		
	}
	
	protected void handleInviteSomebodyJoinGroup(ERequestState reason) {
		
	}
	
	protected void handleDeleteMembersFromGroup(ERequestState reason) {
		
	}
	
	protected void handleQuitGroup(ERequestState reason) {
		
	}
	
	protected void handleAddGroupCard(ERequestState reason) {
		
	}
	
	protected void handleModifyGroupCard(ERequestState reason) {
		
	}
	
	protected void handleQueryGroupCard(ERequestState reason , IMMember iMember ) {
		
	}
	
	protected void handleQueryMembersOfGroup(ERequestState reason,
			ArrayList<IMMember> members) {
		
	}
	
	protected void handleQueryGroupsOfVoip(ERequestState reason,
			ArrayList<IMGroup> imGroups) {
		
	}
	
	protected void handleVerifyJoinGroup(ERequestState reason) {
		
	}
	
	protected void handleAnswerInviteGroup(ERequestState reason) {
		
	}
	
	protected void handleSetRuleOfReceiveGroupMsg(ERequestState reason) {
		
	}


	protected void handleGetPublicGroups(ERequestState reason,
			ArrayList<IMGroup> imGroupSummarys) {
		
	}
	
	
	private HashMap<String, Object> parameters = new HashMap<String, Object>();

	
	public void setTaskParameters(String key , Object value) {
		parameters.put(key, value);
		
	}
	public Object getTaskParameters(String key) {
		return parameters.remove(key);
	}



	@Override
	public void onForbidSpeakForUser(ERequestState reason) {
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    msg.obj = b;
	    msg.arg1 = RestGroupManagerHelper.KEY_FORBIDS_PEAK;
	    sendHandleMessage(msg);
		
	}
	
	public void handleForbidSpeakForUser(ERequestState reason) {
		
	}
	
	protected void doHandleExpertCallback(Message msg) {
		
	}


	@Override
	protected int getLayoutId() {
		return -1;
	}
	
	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPBaseActivity#addTask(com.voice.demo.tools.net.ITask)
	 */
	@Override
	public void addTask(ITask iTask) {
		RestGroupManagerHelper.getInstance().setOnRestGroupManagerHelpListener(this);
		super.addTask(iTask);
	}
}
