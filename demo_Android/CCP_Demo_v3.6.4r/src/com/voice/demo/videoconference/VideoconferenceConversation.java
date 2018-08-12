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
package com.voice.demo.videoconference;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.DeviceListener.APN;
import com.hisun.phone.core.voice.model.videoconference.VideoConference;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceDismissMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceExitMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceJoinMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMsg;
import com.hisun.phone.core.voice.util.VoiceUtil;
import com.voice.demo.R;
import com.voice.demo.chatroom.ChatroomName;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.views.CCPAlertDialog;
import com.voice.demo.views.CCPTitleViewBase;

/**
 * 
* <p>Title: VideoconferenceConversation.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-10-29
* @version 3.5
 */
public class VideoconferenceConversation extends VideoconferenceBaseActivity implements View.OnClickListener
																						,OnItemClickListener,CCPAlertDialog.OnPopuItemClickListener{

	public static final String CONFERENCE_CREATOR = "com.voice.demo.ccp.VIDEO_CREATE";
	public static final String CONFERENCE_TYPE = "COM.VOICE.DEMO.CCP.VIDEO_TYPE";
	
	public static final int TYPE_SINGLE = 0;
	public static final int TYPE_MULIT = 1;
	
	private RelativeLayout mVideoCEmpty;
	private ListView mVideoCListView;
	private TextView mVideoTips;
	
	CCPTitleViewBase mCcpTitleViewBase;
	private ArrayList<VideoConference> mVideoCList;
	private VideoConferenceConvAdapter mVideoCAdapter;
	
	private int mVideoConfType = TYPE_SINGLE;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		mCcpTitleViewBase = new CCPTitleViewBase(this);
		mCcpTitleViewBase.setCCPTitleBackground(R.drawable.video_title_bg);
		mCcpTitleViewBase.setCCPTitleViewText(getString(R.string.apptitle_video_conference));
		mCcpTitleViewBase.setCCPBackImageButton(R.drawable.video_back_button_selector , this).setBackgroundDrawable(null);
		mCcpTitleViewBase.setCCPActionImageButton(R.drawable.video_action_add, this).setBackgroundDrawable(null);
		
		initResourceRefs();
		mVideoConfType = getIntent().getIntExtra(CONFERENCE_TYPE, mVideoConfType);
		
		registerReceiver(new String[]{CCPIntentUtils.INTENT_VIDEO_CONFERENCE_DISMISS});
	}
	@Override
	protected void onResume() {
		super.onResume();
		if(checkeDeviceHelper()) {
			//launchCCP();
			getDeviceHelper().queryVideoConferences(CCPConfig.App_ID, null);
		}
	}
	private void initResourceRefs() {
		mVideoCEmpty = (RelativeLayout) findViewById(R.id.video_conferenc_empty);
		mVideoCListView = (ListView) findViewById(R.id.video_conferenc_list);
		mVideoTips = (TextView) findViewById(R.id.video_notice_tips);
		
		findViewById(R.id.begin_create_video_conference).setOnClickListener(this);
		mVideoCListView.setOnItemClickListener(this);
		mVideoCListView.setEmptyView(mVideoCEmpty);
		
	}
	
	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.title_btn4:
			
			this.finish();
			break;
		case R.id.title_btn1:
		case R.id.begin_create_video_conference:
			
			if(VoiceUtil.checkNetworkAPNType(this) == APN.UNKNOWN) {
				showNetworkNotAvailable();
				return;
			}
			
			if(VoiceUtil.checkNetworkAPNType(this) != APN.WIFI) {
				showVoIPWifiWarnning();
			} else {
				Intent intent = new Intent(VideoconferenceConversation.this, CreateVideoConference.class);
				intent.putExtra(VideoconferenceConversation.CONFERENCE_TYPE, mVideoConfType);
				startActivity(intent);
				overridePendingTransition(R.anim.video_push_up_in, R.anim.push_empty_out);
			}
			
			break;
		default:
			break;
		}
	}
	
	int mPosition = -1;

	private VideoConference videoConferenceInfo;
	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		mPosition = -1;
		if(VoiceUtil.checkNetworkAPNType(this) == APN.UNKNOWN) {
			showNetworkNotAvailable();
			return;
		}
		
		if(VoiceUtil.checkNetworkAPNType(this) != APN.WIFI) {
			mPosition = position;
			showVoIPWifiWarnning();
		} else {
			startJoinVideoConferenceAction(position);
		}
		
		
	}
	
	
	@Override
	protected void handleDialogOkEvent(int requestKey) {
		
		if(requestKey == DIALOG_REQUEST_VOIP_NOT_WIFI_WARNNING) {
			if (mPosition >= 0) {
				startJoinVideoConferenceAction(mPosition);
				mPosition = -1;
			} else {
				startActivity(new Intent(VideoconferenceConversation.this, CreateVideoConference.class));
				overridePendingTransition(R.anim.video_push_up_in, R.anim.push_empty_out);
			}
		} else {
			super.handleDialogOkEvent(requestKey);
		}
	}
	
	/**
	 * 
	* <p>Title: startJoinVideoConferenceAction</p>
	* <p>Description: </p>
	* @param position
	 */
	private void startJoinVideoConferenceAction(int position) {
		if(mVideoCAdapter != null && mVideoCAdapter.getItem(position) != null ) {
			videoConferenceInfo = mVideoCAdapter.getItem(position);
			if(CCPConfig.VoIP_ID.equals(videoConferenceInfo.getCreator())){
				doVideoConferenceControl();
			}else{
				doVideoConferenceAction();
			}
		}
	}
	
	// ----------------------------------CCP SDK Device CallBack.
	
	private void doVideoConferenceControl() {
		int[] ccpAlertResArray = null;
		int title =R.string.chatroom_control_tip;
				ccpAlertResArray = new int[]{R.string.chatroom_c_join,R.string.chatroom_c_dismiss};
		CCPAlertDialog ccpAlertDialog = new CCPAlertDialog(VideoconferenceConversation.this
				, title
				, ccpAlertResArray
				, 0
				, R.string.dialog_cancle_btn);
		
		ccpAlertDialog.setOnItemClickListener(VideoconferenceConversation.this);
		ccpAlertDialog.create();
		ccpAlertDialog.show();
	}
	@Override
	protected void handleVideoConferences(int reason,
			List<VideoConference> conferences) {
		super.handleVideoConferences(reason, conferences);
		
		if(reason == 0 ) {
			
			mVideoCList = new ArrayList<VideoConference>();
			for(VideoConference conference : conferences) {
				if(conference.isMultiVideo() && mVideoConfType == TYPE_MULIT) {
					mVideoCList.add(conference);
					continue;
				}
				
				if(!conference.isMultiVideo() && mVideoConfType != TYPE_MULIT ) {
					mVideoCList.add(conference);
				}
			}
			
			if(mVideoCList == null || mVideoCList.isEmpty()) {
				if(mVideoCAdapter!=null) {
					mVideoCAdapter.clear();
				}
				return ;
			}
			
			//str_click_into_video_conference
			mVideoTips.setText(R.string.str_click_into_video_conference);
			initVideoConferenceAdapter();
		} else {
			Toast.makeText(getApplicationContext(), getString(R.string.toast_get_chatroom_list_failed, reason , 0), Toast.LENGTH_SHORT).show();
		}
	}
	
	
	private void initVideoConferenceAdapter() {
		if(mVideoCAdapter == null ) {
			mVideoCAdapter = new VideoConferenceConvAdapter(VideoconferenceConversation.this);
			mVideoCListView.setAdapter(mVideoCAdapter);
		} 
		
		mVideoCAdapter.setData(mVideoCList);
	}
	
	// --------------------------------SDK Callback -----------------
	
	@Override
	protected void handleReceiveVideoConferenceMsg(VideoConferenceMsg VideoMsg) {
		super.handleReceiveVideoConferenceMsg(VideoMsg);
		synchronized (VideoconferenceConversation.class) {
			if(VideoMsg == null ) {
				return;
			}
			
			// remove the Video Conference Conversation form the Video Adapter.
			if(VideoMsg instanceof VideoConferenceDismissMsg) {
				VideoConferenceDismissMsg videoConferenceDismissMsg = (VideoConferenceDismissMsg) VideoMsg;
				
				String conferenceId = videoConferenceDismissMsg.getConferenceId();
				if(mVideoCAdapter == null || mVideoCAdapter.isEmpty()) {
					return;
				}
				
				for(int position = 0 ; position < mVideoCAdapter.getCount() ; position++) {
					VideoConference item = mVideoCAdapter.getItem(position);
					if(item == null || TextUtils.isEmpty(item.getConferenceId())){
						continue;
					}
					
					if(item.getConferenceId().equals(conferenceId)) {
						mVideoCAdapter.remove(item);
						return;
					}
				}
				
				// if Video Conference list empty .
				if(mVideoCAdapter.isEmpty()) {
					//str_tips_no_video_c
					mVideoTips.setText(R.string.str_tips_no_video_c);
				}
			}else if(VideoMsg instanceof VideoConferenceJoinMsg||VideoMsg instanceof VideoConferenceExitMsg) {
				if(checkeDeviceHelper()) {
					//launchCCP();
					getDeviceHelper().queryVideoConferences(CCPConfig.App_ID, null);
				}
			}
		}
	}
	
	@Override
	protected void onReceiveBroadcast(Intent intent) {
		super.onReceiveBroadcast(intent);
		
		if(CCPIntentUtils.INTENT_VIDEO_CONFERENCE_DISMISS.equals(intent.getAction())) {
			// receive action dismiss Video Conference.
			if(intent.hasExtra(Device.CONFERENCE_ID)) {
				String conferenceId = intent.getStringExtra(Device.CONFERENCE_ID);
				VideoConferenceDismissMsg VideoMsg = new VideoConferenceDismissMsg();
				VideoMsg.setConferenceId(conferenceId);
				handleReceiveVideoConferenceMsg(VideoMsg);
			}
			
		}
	}
	
	/**
	 * 
	 * @ClassName: VideoConferenceConvAdapter 
	 * @Description: TODO
	 * @author Jorstin Chan 
	 * @date 2013-12-3
	 *
	 */
	class VideoConferenceConvAdapter extends ArrayAdapter<VideoConference> {

		LayoutInflater mInflater;
		
		public VideoConferenceConvAdapter(Context context) {
			super(context, 0);
			
			mInflater = getLayoutInflater();
		}
		
		public void setData(List<VideoConference> data) {
			setNotifyOnChange(false);
			clear();
			setNotifyOnChange(true);
			if (data != null) {
				for (VideoConference appEntry : data) {
					add(appEntry);
				}
			}
		}
		
		
		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			
			VideoConferenceHolder holder;
			if (convertView == null|| convertView.getTag() == null) {
				convertView = mInflater.inflate(R.layout.list_item_chatroom, null);
				holder = new VideoConferenceHolder();
				convertView.setTag(holder);
				
				holder.videoConName = (TextView) convertView.findViewById(R.id.chatroom_name);
				holder.videoConTips = (TextView) convertView.findViewById(R.id.chatroom_tips);
				holder.gotoIcon = (TextView) convertView.findViewById(R.id.goto_icon);
				
				LinearLayout.LayoutParams params = (LayoutParams) holder.gotoIcon.getLayoutParams();
				params.setMargins(0, 0, 10, 0);
				holder.gotoIcon.setLayoutParams(params);
				
				convertView.setBackgroundResource(R.drawable.video_list_item_conference);
				// set Video Conference Item Style
				holder.videoConName.setTextColor(Color.parseColor("#FFFFFF"));
				holder.gotoIcon.setBackgroundResource(R.drawable.video_item_goto);
			} else {
				holder = (VideoConferenceHolder) convertView.getTag();
			}
			
			try {
				// do ..
				VideoConference item = getItem(position);
				if(item != null ) {
					
					String conferenceName = "";
					String voipAccount = item.getCreator();
					if(!TextUtils.isEmpty(voipAccount) && voipAccount.length() > 4) {
						voipAccount = voipAccount.substring(voipAccount.length() - 4 , voipAccount.length());
						
					} else {
						voipAccount = " ";
					}
					
					// Video Conference Name
					if(!TextUtils.isEmpty(item.getConferenceName())) {
						conferenceName = item.getConferenceName();
					} else {
						conferenceName = getString(R.string.video_conference_name , voipAccount);
					}
					
					holder.videoConName.setText(conferenceName);
					
					int resourceId ;
					if("8".equals(item.getJoinNum())) {
						//
						resourceId = R.string.str_chatroom_list_join_full;
					} else {
						
						resourceId = R.string.str_chatroom_list_join_unfull;
					}
					
					holder.videoConTips.setText(getString(resourceId, item.getJoinNum() , voipAccount));
				}
				
			} catch (Exception e) {
				e.printStackTrace();
			}
			
			return convertView;
		}
		
		
		class VideoConferenceHolder {
			TextView videoConName;
			TextView videoConTips;
			TextView gotoIcon;
		}
		
	}
	
	@Override
	protected int getLayoutId() {
		return R.layout.video_conference_conversation;
	}
	@Override
	public void onItemClick(ListView parent, View view, int position,
			int resourceId) {
		switch (resourceId) {
		case R.string.chatroom_c_join:
				doVideoConferenceAction();
			break;
		case R.string.chatroom_c_dismiss:
			if(videoConferenceInfo!=null && checkeDeviceHelper()){
				getDeviceHelper().dismissVideoConference(CCPConfig.App_ID, videoConferenceInfo.getConferenceId());
				getDeviceHelper().queryVideoConferences(CCPConfig.App_ID, null);
			}
			break;
	}
		
	}
	/**
	 * 
	 */
	private void doVideoConferenceAction() {
		if(videoConferenceInfo == null){
			Toast.makeText(this, "会议号不存在", Toast.LENGTH_LONG).show();
			return ;
		}
		Intent intent = new Intent();
		if(mVideoConfType == 0) {
			intent.setClass(VideoconferenceConversation.this , VideoConferenceChattingUI.class);
		} else {
			intent.setClass(VideoconferenceConversation.this , MultiVideoconference.class);
			
		}
		intent.putExtra(Device.CONFERENCE_ID, videoConferenceInfo.getConferenceId());
		intent.putExtra(CONFERENCE_CREATOR, videoConferenceInfo.getCreator());
		if(!TextUtils.isEmpty(videoConferenceInfo.getConferenceName())) {
			intent.putExtra(ChatroomName.CHATROOM_NAME, videoConferenceInfo.getConferenceName());
		}
		startActivity(intent) ;
	}

}
