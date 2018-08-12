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
package com.voice.demo.voip;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.voice.demo.R;
import com.voice.demo.group.GroupBaseActivity;
import com.voice.demo.group.GroupChatActivity;
import com.voice.demo.group.GroupDetailActivity;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.ui.CapabilityChoiceActivity;
import com.voice.demo.voip.VoIPCallActivity;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.InputType;
import android.text.TextUtils;
import android.view.*;
import android.widget.*;
import android.widget.AdapterView.OnItemClickListener;

/**
 * invited to select contact interface
 * 1, select the VoIP network telephone contact.
 * 2, select the video call contacts.
 * 3, select the voice message recipient
 * 4, select the voice group chat participants
 *
 */
public class GetNumberToTransferActivity extends CCPBaseActivity implements View.OnClickListener ,OnItemClickListener{

	//public static final int CREATE_TO_DELAY_VOICE = 0x0;
	public static final int CREATE_TO_IM_TALK = 0x4 ;
	
	public static final int SIGLE_CHOICE = 0x5 ;
	public static final int MULTIPLE_CHOICE = 0x6 ;
	
	private ListView InterMemList;
	private Button mJoinInter;
	private TextView mJoinTips;
	private TextView mHeadTips;
	
	private InviteAdapter mInviteAdapter;
	private List<String> arr;
	
	private int mVoiceType;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, "选择接听方"
				, "落地呼转");
		
		
		InterMemList = (ListView) findViewById(R.id.interphone_member_list);
		mJoinTips = (TextView) findViewById(R.id.join_select_tips);
		mHeadTips = (TextView) findViewById(R.id.notice_tips);
		
		// Allows the user to input a number is added to the list of choices .
		mJoinInter = (Button) findViewById(R.id.id_confirm);
		mJoinInter.setVisibility(View.VISIBLE);
		mJoinInter.setOnClickListener(this);
		mJoinInter.setEnabled(false);
		
		InterMemList.setOnItemClickListener(this);
		String[] mInviterMember = null;
		if( CCPConfig.VoIP_ID_LIST != null) {
			mInviterMember = CCPConfig.VoIP_ID_LIST.split(",");
			if(mInviterMember == null || mInviterMember.length == 0) {
				throw new IllegalArgumentException("Load the VOIP account information errors" +
						", configuration information can not be empty" + mInviterMember);
			}
		}
		arr = Arrays.asList(mInviterMember);
		arr= CCPUtil.removeString(arr, CCPConfig.VoIP_ID);
		mInviteAdapter = new InviteAdapter(getApplicationContext(), arr);
		InterMemList.setAdapter(mInviteAdapter);
		
		initialize(savedInstanceState);
		
	}

	private void initialize(Bundle savedInstanceState) {
		// Read parameters or previously saved state of this activity.
		mVoiceType = SIGLE_CHOICE;
		
		int resourceId = R.string.str_check_participants_transfer;
		mHeadTips.setText(resourceId);
		
	}
	
	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.id_confirm :
			
			// start a session interphone ...
			ArrayList<String> list = new ArrayList<String>();
			if(mInviteAdapter!=null){
				HashMap<Integer, Boolean> isSelected =mInviteAdapter.getIsSelected();
				for (int i = 0; isSelected!=null&&i < isSelected.size(); i++) {
					if(isSelected.get(i)){
						list.add(arr.get(i));
					}
				}
			}
			if(list == null || list.isEmpty()) {
				return ;
			}
//			
//			Intent intent = new Intent() ;
//			if(mVoiceType == CREATE_TO_INTER_PHONE_VOICE) {
////				intent.setClass(GetNumberToTransferActivity.this, InterPhoneRoomActivity.class);
////				intent.putStringArrayListExtra("InviterMember", list) ;
//			} else if (mVoiceType == CREATE_TO_VOIP_CALL) {
//				intent.setClass(GetNumberToTransferActivity.this, VoIPCallActivity.class);
//				intent.putExtra("VOIP_CALL_NUMNBER", list.get(0)) ;
//			} else if (mVoiceType == CREATE_TO_VIDEO_CALL) {
//				intent.setClass(GetNumberToTransferActivity.this, CapabilityChoiceActivity.class);
//				intent.putExtra("VOIP_CALL_NUMNBER", list.get(0)) ;
//			} else if (mVoiceType == CREATE_TO_IM_TALK) {
//				intent.setClass(GetNumberToTransferActivity.this, GroupChatActivity.class);
//				intent.putExtra(GroupBaseActivity.KEY_GROUP_ID, list.get(0)) ;
//			} else if (mVoiceType == MULTIPLE_CHOICE ) {
//				intent.setClass(GetNumberToTransferActivity.this, GroupDetailActivity.class);
//				intent.putStringArrayListExtra("to", list);
//			}
//			
//			// If the video call or call VOIP function returns...
//			if(mVoiceType == CREATE_TO_VOIP_CALL || mVoiceType == CREATE_TO_VIDEO_CALL || mVoiceType == MULTIPLE_CHOICE )  {
//				setResult(RESULT_OK,intent);
//			} else {
//				startActivity(intent);
//			}
//			finish() ;
			Intent intent = new Intent() ;
			intent.setClass(GetNumberToTransferActivity.this, CallOutActivity.class);
			intent.putExtra("VOIP_CALL_NUMNBER", list.get(0)) ;
			setResult(RESULT_OK,intent);
			finish() ;
			break;
			
		default:
			break;
		}
	}
	
	@Override
	protected void handleTitleAction(int direction) {
		
		if(direction == TITLE_RIGHT_ACTION) {
			// display an input dialog .
			// Allows the user to input a number is added to the list of choices .
			// Click the OK button to enter the number added to the list of choices...
			//showEditDialog();
			//showEditTextDialog(DIALOG_SHOW_KEY_INVITE, getString(R.string.dialog_title_input_number), null);
			
			showEditTextDialog(DIALOG_SHOW_KEY_INVITE
					, InputType.TYPE_CLASS_TEXT 
					, 1
					, getString(R.string.dialog_title_input_number_transfer)
					, null);
		} else {
			
			super.handleTitleAction(direction);
		}
	}
	
	@Override
	protected void handleEditDialogOkEvent(int requestKey, String editText,
			boolean checked) {
		super.handleEditDialogOkEvent(requestKey, editText, checked);
		
		if(requestKey == DIALOG_SHOW_KEY_INVITE) {
			String mPhoneNumber = editText;
			if(mPhoneNumber != null && !TextUtils.isEmpty(mPhoneNumber)){
				// invite this phone call ...
				if(arr == null ) {
					arr = new ArrayList<String>();
				}
				arr.add(mPhoneNumber);
				mInviteAdapter.isSelected.put((Integer)arr.size() - 1, false);
				
				if(mInviteAdapter == null) {
					mInviteAdapter = new InviteAdapter(getApplicationContext(), arr);
					InterMemList.setAdapter(mInviteAdapter);
				} 
				mInviteAdapter.notifyDataSetChanged();
				
			}
		}
	}

	private int count ;
	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		CheckBox cBox = (CheckBox) view.findViewById(R.id.check_box);
			cBox.toggle();// Changes the state of checkbox access to click in every times.
				for (int i = 0 ; i < mInviteAdapter.getIsSelected().size() ; i++) {
					mInviteAdapter.getIsSelected().put(i, false);
				}
			if(cBox.isChecked()) {
				count ++;
			} else {
				if(count > 0) {
					count --;
				}
			}
			mInviteAdapter.getIsSelected().put(position, cBox.isChecked());
		mInviteAdapter.notifyDataSetChanged();
		for (Map.Entry<Integer, Boolean> entry : mInviteAdapter.isSelected.entrySet()) {
			if(entry.getValue()) {
				mJoinInter.setEnabled(true);
				int resourceId = 0;
				 if (mVoiceType == CREATE_TO_IM_TALK) {
					
					resourceId = R.string.str_check_to_participants_im_talk;
				} else if (mVoiceType == MULTIPLE_CHOICE) {
					resourceId = R.string.str_check_to_participants_group;
				}
//				mJoinTips.setText(resourceId);
				return ;
			} 
			
			mJoinInter.setEnabled(false);
			mJoinTips.setText(R.string.str_not_check_participants_transfer);
		}
	}
	
	class InviteAdapter extends ArrayAdapter<String> {

		LayoutInflater mInflater;
		HashMap<Integer, Boolean> isSelected;
		
		int count ;
		
		public InviteAdapter(Context context,List<String> objects) {
			super(context, 0, objects);
			
			mInflater = getLayoutInflater();
			init(objects);
		}
		
		// initialize all checkbox are not selected
		public void init(List<String> objects) {
			if(isSelected!=null){
				isSelected.clear();
			}else{
				isSelected = new HashMap<Integer, Boolean>();
			}
			for (int i = 0; i < objects.size(); i++) {
				isSelected.put(i, false);
			}
		}
		
		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			
			InviteHolder holder;
			if (convertView == null|| convertView.getTag() == null) {
				convertView = mInflater.inflate(R.layout.invite_member_list_item, null);
				holder = new InviteHolder();
				convertView.setTag(holder);
				
				
				holder.name = (TextView) convertView.findViewById(R.id.name);
				holder.checkBox = (CheckBox) convertView.findViewById(R.id.check_box);
					holder.checkBox.setButtonDrawable(R.drawable.checkbox_btn);
			} else {
				holder = (InviteHolder) convertView.getTag();
			}
			
			// do ..
			String name = getItem(position);
			if(!TextUtils.isEmpty(name)) {
				holder.name.setText(name);
			}
			holder.checkBox.setChecked(isSelected.get(position));
			return convertView;
		}
		
		
		class InviteHolder {
			TextView name;
			CheckBox checkBox;
		}
		
		public HashMap<Integer, Boolean> getIsSelected() {
			return isSelected;
		}

		
	}

	@Override
	protected int getLayoutId() {
		return R.layout.layout_gettransfernumber_activity;
	}
	
	 
}