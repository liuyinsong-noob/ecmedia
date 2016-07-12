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
package com.voice.demo.chatroom;

import java.util.ArrayList;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.DialogInterface.OnDismissListener;
import android.os.Bundle;
import android.os.Message;
import android.text.InputType;
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
import android.widget.Toast;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.chatroom.Chatroom;
import com.hisun.phone.core.voice.model.chatroom.ChatroomDismissMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomExitMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomJoinMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMember;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomRemoveMemberMsg;
import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.R;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.ui.CCPHelper;
import com.voice.demo.videoconference.VideoConferenceChattingUI;
import com.voice.demo.videoconference.baseui.CCPVideoConUI;
import com.voice.demo.views.CCPAlertDialog;

/**
 * ChatRoom Converstion list ...
 *
 */
public class ChatroomConversation extends CCPBaseActivity implements View.OnClickListener ,OnItemClickListener,CCPAlertDialog.OnPopuItemClickListener{

	private static final int REQUEST_CODE_KICK_MEMBER2 = 100;
	private LinearLayout mChatRoomEmpty;
	private ListView mChatRoomLv;
	
	// voip helper
	private ChatRoomConvAdapter mRoomAdapter;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_chatroom_conv)
				, getString(R.string.str_button_create_chatroom));
		
		initResourceRefs();
		
		CCPHelper.getInstance().setHandler(mChatRoomHandler);
		registerReceiver(new String[]{
				CCPIntentUtils.INTENT_RECIVE_CHAT_ROOM ,
				CCPIntentUtils.INTENT_CHAT_ROOM_DISMISS});
	}
	
	@Override
	protected void onStart() {
		super.onStart();
	}
	
	@Override
	protected void onResume() {
		CCPHelper.getInstance().setHandler(mChatRoomHandler);
		// init voice helper ..
		if(checkeDeviceHelper()) {
			getDeviceHelper().queryChatrooms(CCPConfig.App_ID, null);
		}
		super.onResume();
	}

	private void initResourceRefs() {
		mChatRoomLv = (ListView) findViewById(R.id.chatroom_list);
		mChatRoomLv.setOnItemClickListener(this);
		findViewById(R.id.begin_create_chatroom).setOnClickListener(this);
		mChatRoomEmpty = (LinearLayout) findViewById(R.id.chatroom_empty);
		
		initListView();
	}

	private void initListView() {
		if(chatRoomList != null && !chatRoomList.isEmpty()) {
			mRoomAdapter = new ChatRoomConvAdapter(this, chatRoomList);
			mChatRoomLv.setAdapter(mRoomAdapter);
			mChatRoomLv.setVisibility(View.VISIBLE);
			mChatRoomEmpty.setVisibility(View.GONE);
		} else {
			mChatRoomEmpty.setVisibility(View.VISIBLE);
			mChatRoomLv.setVisibility(View.GONE);
		}
	}

	@Override
	protected void handleTitleAction(int direction) {
		if(direction == TITLE_RIGHT_ACTION) {
			startActivity(new Intent(ChatroomConversation.this, ChatroomName.class));
			overridePendingTransition(R.anim.video_push_up_in, R.anim.push_empty_out);
		} else {
			
			super.handleTitleAction(direction);
		}
	}
	@Override
	public void onClick(View v) {
		switch (v.getId()) {

		case R.id.begin_create_chatroom:
			handleTitleAction(TITLE_RIGHT_ACTION);
			break;
		default:
			break;
		}
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		
		if(mRoomAdapter != null ) {
			
			mRoomAdapter = null ;
		}
		
		mChatroom = null;
	}
	
	
	Chatroom mChatroom = null;
	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		if(mRoomAdapter != null && mRoomAdapter.getItem(position) != null ) {
			Chatroom chatroom = mRoomAdapter.getItem(position);
			
			if(TextUtils.isEmpty(chatroom.getRoomNo())) {
				Toast.makeText(this, "加入语音群聊失败,房间号:" + chatroom.getRoomNo(), Toast.LENGTH_SHORT).show();
				return;
			}

			if(chatroom.getCreator().equals(CCPConfig.VoIP_ID)){
				 //语音会议 加入会议   解散会议  成员管理   取消
				mChatroom = chatroom;
				doChatRoomControl();
				return;
			}
			 
			if("1".equals(chatroom.getValidate())) {
				
				mChatroom = chatroom;
				showEditTextDialog(DIALOG_SHOW_KEY_INVITE
						, InputType.TYPE_CLASS_TEXT 
						, 1
						, getString(R.string.dialog_title_auth)
						, getString(R.string.dialog_message_chatroom_auth_reason));
				return;
			}
			mChatroom = chatroom;
			doChatroomAction(chatroom, null);
		}
		
	}
	

	private void doChatroomAction(Chatroom chatroom  , String roomPwd) {
			
		Intent intent = new Intent(ChatroomConversation.this , ChatroomActivity.class);
		intent.putExtra(Device.CONFNO, chatroom.getRoomNo());
		intent.putExtra(ChatroomName.CHATROOM_CREATOR, chatroom.getCreator());
		if(TextUtils.isEmpty(chatroom.getRoomName())) {
			if(TextUtils.isEmpty(chatroom.getCreator())){
				return;
			}
			intent.putExtra(ChatroomName.CHATROOM_NAME, getString(R.string.app_title_default 
					, chatroom.getCreator().substring(chatroom.getCreator().length() - 3, chatroom.getCreator().length())));
		} else {
			intent.putExtra(ChatroomName.CHATROOM_NAME, chatroom.getRoomName());
		}
		
		if(!TextUtils.isEmpty(roomPwd)) {
			intent.putExtra(ChatroomName.CHATROOM_PWD, roomPwd);
		}
		
		startActivity(intent) ;
	}
	
	class ChatRoomConvAdapter extends ArrayAdapter<Chatroom> {

		LayoutInflater mInflater;
		
		public ChatRoomConvAdapter(Context context, ArrayList<Chatroom> objects) {
			super(context, 0, objects);
			
			mInflater = getLayoutInflater();
		}
		
		
		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			
			ChatRoomHolder holder;
			if (convertView == null|| convertView.getTag() == null) {
				convertView = mInflater.inflate(R.layout.list_item_chatroom, null);
				holder = new ChatRoomHolder();
				convertView.setTag(holder);
				
				holder.chatRoomName = (TextView) convertView.findViewById(R.id.chatroom_name);
				holder.chatRoomTips = (TextView) convertView.findViewById(R.id.chatroom_tips);
				holder.mLock = (ImageView) convertView.findViewById(R.id.lock);
			} else {
				holder = (ChatRoomHolder) convertView.getTag();
			}
			
			try {
				// do ..
				Chatroom item = getItem(position);
				if(item != null ) {
					holder.chatRoomName.setText(item.getRoomName());
					int resourceId ;
					if("8".equals(item.getJoined())) {
						//
						resourceId = R.string.str_chatroom_list_join_full;
					} else {
						
						resourceId = R.string.str_chatroom_list_join_unfull;
					}
					holder.chatRoomTips.setText(getString(resourceId, item.getJoined() , item.getCreator()));
				
					if(!TextUtils.isEmpty(item.getValidate())
							&& "1".equals(item.getValidate())) {
						holder.mLock.setVisibility(View.VISIBLE);
					} else {
						holder.mLock.setVisibility(View.GONE);
					}
				}
				
			} catch (Exception e) {
				e.printStackTrace();
			}
			
			return convertView;
		}
		
		
		class ChatRoomHolder {
			TextView chatRoomName;
			TextView chatRoomTips;
			ImageView mLock;
		}
		
	}

	@Override
	protected void onReceiveBroadcast(Intent intent) {
		super.onReceiveBroadcast(intent);
		if (intent != null && CCPIntentUtils.INTENT_RECIVE_CHAT_ROOM.equals(intent.getAction())) {
			if(intent.hasExtra("ChatRoomInfo")) {
				Chatroom cRoomInfo = (Chatroom) intent.getSerializableExtra("ChatRoomInfo");
				if(cRoomInfo != null) {
					if(chatRoomList == null) {
						chatRoomList = new ArrayList<Chatroom>();
					}
					chatRoomList.add(cRoomInfo);
					initListView();
				} 
			} else {
				if(checkeDeviceHelper())
					getDeviceHelper().queryChatrooms(CCPConfig.App_ID, null);
			}
		} else if(intent.getAction().equals(CCPIntentUtils.INTENT_CHAT_ROOM_DISMISS)) {
			if(intent.hasExtra("roomNo")) {
				String roomNo = intent.getStringExtra("roomNo");
				if(!TextUtils.isEmpty(roomNo) && chatRoomList != null ) {
					for(Chatroom chatroom : chatRoomList){
						if(roomNo.equals(chatroom.getRoomNo())) {
							chatRoomList.remove(chatroom);
							break;
						}
					}
					
					initListView();
					
				}
			}
		}
		
	}
	
	// 回调handler，更新界面显示
	private ArrayList<Chatroom> chatRoomList;
	private android.os.Handler mChatRoomHandler = new android.os.Handler() {


		@SuppressWarnings("unchecked")
		@Override
		public void handleMessage(Message msg) {
			super.handleMessage(msg);
			
			Bundle b = null;
			int reason = 0;
			// 获取通话ID
			if (msg.obj instanceof Bundle) {
				b = (Bundle) msg.obj;
				reason = b.getInt(Device.REASON);
			}
			
			switch (msg.what) {
			//receive a new voice mail messages...
			case CCPHelper.WHAT_ON_CHATROOM_SIP_MESSAGE:
				if(b.getSerializable(Device.CHATROOM_MSG) != null){
					ChatroomMsg	crmsg = (ChatroomMsg) b.getSerializable(Device.CHATROOM_MSG);
					if(crmsg!=null&& crmsg instanceof ChatroomMsg){
						if(checkeDeviceHelper()) {
							getDeviceHelper().queryChatrooms(CCPConfig.App_ID, null);
						}
					}
				}
				break;
			case CCPHelper.WHAT_ON_CHATROOM_LIST:
				if(reason == 0 ) {
					chatRoomList = (ArrayList<Chatroom>) b.getSerializable(Device.CHATROOM_LIST);
					initListView();
				} else {
					Toast.makeText(getApplicationContext(), getString(R.string.toast_get_chatroom_list_failed, reason , 0), Toast.LENGTH_SHORT).show();
				}
				break;

			default:
				break;
			}
			
		}
	};

	@Override
	protected int getLayoutId() {
		return R.layout.layout_chatroom_conversation_activity;
	}
	
	@Override
	protected void handleEditDialogOkEvent(int requestKey, String editText,
			boolean checked) {
		super.handleEditDialogOkEvent(requestKey, editText, checked);
		if(requestKey == DIALOG_SHOW_KEY_INVITE) {
			if(mChatroom != null) {
				doChatroomAction(mChatroom, editText);
			}
		}
	}
	
	
	private void doChatRoomControl() {
		
		int[] ccpAlertResArray = null;
		int title =R.string.chatroom_control_tip;
				ccpAlertResArray = new int[]{R.string.chatroom_c_join,R.string.chatroom_c_dismiss,R.string.chatroom_c_managemenber};
		CCPAlertDialog ccpAlertDialog = new CCPAlertDialog(ChatroomConversation.this
				, title
				, ccpAlertResArray
				, 0
				, R.string.dialog_cancle_btn);
		
		ccpAlertDialog.setOnItemClickListener(this);
		ccpAlertDialog.create();
		ccpAlertDialog.show();
	}

	@Override
	public void onItemClick(ListView parent, View view, int position,
			int resourceId) {
		switch (resourceId) {
			case R.string.chatroom_c_join:
				if("1".equals(mChatroom.getValidate())){
					showEditTextDialog(DIALOG_SHOW_KEY_INVITE
							, InputType.TYPE_CLASS_TEXT 
							, 1
							, getString(R.string.dialog_title_auth)
							, getString(R.string.dialog_message_chatroom_auth_reason));
					return;
				}
				doChatroomAction(mChatroom , null);
				break;
			case R.string.chatroom_c_dismiss:
				getDeviceHelper().dismissChatroom(CCPConfig.App_ID, mChatroom.getRoomNo());
				break;
			case R.string.chatroom_c_managemenber:
				/*Intent intent = new Intent(ChatroomConversation.this, ChatroomMemberManagerActivity.class);
				intent.putExtra(Device.CONFNO, mChatroom.getRoomNo());
				startActivityForResult(intent, REQUEST_CODE_KICK_MEMBER2);*/
				Intent intent = new Intent(ChatroomConversation.this , ChatroomActivity.class);
				intent.putExtra("flag", true);
				intent.putExtra(Device.CONFNO, mChatroom.getRoomNo());
				intent.putExtra(ChatroomName.CHATROOM_CREATOR, mChatroom.getCreator());
				if(TextUtils.isEmpty(mChatroom.getRoomName())) {
					if(TextUtils.isEmpty(mChatroom.getCreator())){
						return;
					}
					intent.putExtra(ChatroomName.CHATROOM_NAME, getString(R.string.app_title_default 
							, mChatroom.getCreator().substring(mChatroom.getCreator().length() - 3, mChatroom.getCreator().length())));
				} else {
					intent.putExtra(ChatroomName.CHATROOM_NAME, mChatroom.getRoomName());
				}
				startActivity(intent);
				break;
		}
		
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		
		// If there's no data (because the user didn't select a number and
		// just hit BACK, for example), there's nothing to do.
		if (requestCode != REQUEST_CODE_KICK_MEMBER2) {
			if (data == null) {
				return;
			}
		} else if (resultCode != RESULT_OK || !checkeDeviceHelper()) {
			
			return;
		}
		
		switch (requestCode) {
		case REQUEST_CODE_KICK_MEMBER2:
			if(data.hasExtra("isKicked")) {
				Bundle extras = data.getExtras();
				if (extras != null) {
					boolean isKicked = extras.getBoolean("isKicked");
					
					if(isKicked) {
						CCPHelper.getInstance().setHandler(mChatRoomHandler);
						getDeviceHelper().queryMembersWithChatroom(mChatroom.getRoomNo());
					}
				}
			}
			
			break;
		default:
			break;
		}
	}
}
