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
import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Message;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMember;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMsg.ForbidOptions;
import com.voice.demo.R;
import com.voice.demo.CCPApplication;
import com.voice.demo.group.GroupBaseActivity;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.ui.CCPHelper;

public class ChatroomMemberManagerActivity extends GroupBaseActivity {

	private ListView mKickMemList;
	private String roomNo;
	
	private KickAdapter mKickAdapter;
	
	private boolean isKicked = false;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.chatroom_member_manager);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_manager_member)
				, null);
		
		initResourceRefs();
		
		if(checkeDeviceHelper()) {
			CCPHelper.getInstance().setHandler(mChatRoomHandler);
			initialize(savedInstanceState);
		}
	}
	

	private void initResourceRefs() {
		
		mKickMemList = (ListView) findViewById(R.id.kicked_member_list);
	}

	private void initialize(Bundle savedInstanceState) {
		// Read parameters or previously saved state of this activity.
		Intent intent = getIntent();
		if(intent.hasExtra(Device.CONFNO)) {
			// To invite voice group chat
			Bundle extras = intent.getExtras();
			if (extras != null) {
				roomNo = extras.getString(Device.CONFNO); 
			}
			
		}
		
		if(TextUtils.isEmpty(roomNo)) {
			finish();
		}
		if(checkeDeviceHelper()) {
			showConnectionProgress(getString(R.string.str_dialog_message_default));
			getDeviceHelper().queryMembersWithChatroom(roomNo);
		}
	}

	
	@Override
	protected void handleTitleAction(int direction) {
		if(direction == TITLE_LEFT_ACTION) {
			setResultOk();
		} else {
			super.handleTitleAction(direction);
		}
	}
	
	int kickPosition ;
	class KickAdapter extends ArrayAdapter<ChatroomMember> {

		public KickAdapter(Context context,List<ChatroomMember> objects) {
			super(context, 0, objects);
			
		}
		
		@Override
		public View getView(final int position, View convertView, ViewGroup parent) {
			
			KickedHolder holder;
			if (convertView == null|| convertView.getTag() == null) {
				convertView = getLayoutInflater().inflate(R.layout.list_kicked_member_item, null);
				holder = new KickedHolder();
				
				holder.name = (TextView) convertView.findViewById(R.id.name);
				holder.kickButton = (Button) convertView.findViewById(R.id.kicked_btn);
			} else {
				holder = (KickedHolder) convertView.getTag();
			}
			
			// do ..
			final ChatroomMember chatroomMember = getItem(position);
			if(chatroomMember != null ) {
				holder.name.setText(chatroomMember.getNumber());
				
				if(CCPConfig.VoIP_ID.equals(chatroomMember.getNumber())) {
					holder.kickButton.setVisibility(View.GONE);
				} else {
					holder.kickButton.setVisibility(View.VISIBLE);
				}
				
				holder.kickButton.setOnClickListener(new OnClickListener() {
					
					@Override
					public void onClick(View v) {
						if(mKickAdapter != null && checkeDeviceHelper()) {
							showConnectionProgress(getString(R.string.str_dialog_message_default));
							
							// modify 3.4.1.2 reomoveMemberFromChatroom
							getDeviceHelper().removeMemberFromChatroom(CCPConfig.App_ID, roomNo, chatroomMember.getNumber());
							kickPosition = position;
						}
					}
				});
				
			}
			return convertView;
		}
		
		
		class KickedHolder {
			TextView name;
			Button kickButton;
		}
		
	}
	
	private android.os.Handler mChatRoomHandler = new android.os.Handler() {

		@SuppressWarnings("unchecked")
		@Override
		public void handleMessage(Message msg) {
			super.handleMessage(msg);
			Bundle b = null;
			int reason = -1;
			ArrayList<ChatroomMember> members = null;
			if (msg.obj instanceof Bundle) {
				b = (Bundle) msg.obj;
				reason = b.getInt(Device.REASON);
				if (b.getSerializable(Device.CHATROOM_MEMBERS) != null)
					members = (ArrayList<ChatroomMember>) b.getSerializable(Device.CHATROOM_MEMBERS);
			}
			

			switch (msg.what) {
			case CCPHelper.WHAT_ON_CHATROOM_MEMBERS:
				closeConnectionProgress();
				if(reason == 0 && members!=null){
					mKickAdapter = new KickAdapter(getApplicationContext(), members);
					mKickMemList.setAdapter(mKickAdapter);
				} 
				break;
				
			case CCPHelper.WHAT_ON_CHATROOM_KICKMEMBER:
				try {
					closeConnectionProgress();
					if(reason != 0) {
						CCPApplication.getInstance().showToast(getString(R.string.str_member_kicked_out_failed, reason + ""));
						return;
					}
					if(b.getString("kick_member") != null ) {
						String kickMember = b.getString("kick_member");
						if(mKickAdapter != null ) {
							ChatroomMember item = mKickAdapter.getItem(kickPosition);
							if(item != null && kickMember.equals(item.getNumber())) {
								mKickAdapter.remove(item);
								isKicked = true;
							}
						}
					}
				} catch (Exception e) {
					e.printStackTrace();
				}
				
				break;
				
			}
		}


	};
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {
			
			setResultOk();
			return true;
		}

		return super.onKeyDown(keyCode, event);
	}


	private void setResultOk() {
		Intent intent = new Intent(ChatroomMemberManagerActivity.this, ChatroomActivity.class);
		intent.putExtra("isKicked", isKicked);
		setResult(RESULT_OK, intent);
		this.finish();
	}	
}
