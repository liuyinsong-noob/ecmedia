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
 */package com.voice.demo.ui;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ScrollView;
import android.widget.TextView;

import com.hisun.phone.core.voice.Build;
import com.hisun.phone.core.voice.Device.AudioMode;
import com.hisun.phone.core.voice.Device.AudioType;
import com.hisun.phone.core.voice.DeviceListener.APN;
import com.hisun.phone.core.voice.model.DownloadInfo;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.VoiceUtil;
import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.ExConsultation.ExpertMainActivity;
import com.voice.demo.chatroom.ChatroomConversation;
import com.voice.demo.contacts.ContactListActivity;
import com.voice.demo.group.GroupMessageListActivity;
import com.voice.demo.group.model.IMChatMessageDetail;
import com.voice.demo.interphone.InterPhoneActivity;
import com.voice.demo.interphone.InviteInterPhoneActivity;
import com.voice.demo.outboundmarketing.MarketActivity;
import com.voice.demo.setting.SettingsActivity;
import com.voice.demo.sqlite.CCPSqliteManager;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.tools.SDKVersion;
import com.voice.demo.tools.net.ITask;
import com.voice.demo.tools.net.TaskKey;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.baseui.CCPCapabilityItemView;
import com.voice.demo.video.VideoActivity;
import com.voice.demo.videoconference.VideoconferenceConversation;
import com.voice.demo.videoconference.WizardVideoconferenceActivity;
import com.voice.demo.views.CCPAlertDialog;
import com.voice.demo.voicecode.VoiceVerificationCodeActivity;
import com.voice.demo.voip.NetPhoneCallActivity;

/**
 * 
 * 
* <p>Title: CapacityChoiceActivity.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2013</p>
* <p>Company: http://www.cloopen.com</p>
* @author Jorstin Chan
* @date 2013-12-2
* @version 3.6
 */
@SuppressLint("UseSparseArrays")
public class CapabilityChoiceActivity extends CCPBaseActivity implements CCPCapabilityItemView.OnCapacityItemClickListener
																		,CCPAlertDialog.OnPopuItemClickListener{

	private static final int REQUEST_CODE_VIDEO_CALL = 11;
	
	private String phoneStr;
	
	private ScrollView mScrollView;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		

		initLayoutResource();
		
		initCapacityUI();
		
		if(checkeDeviceHelper()) {
			//initAudioConfig();
			//设置码流
			//getDeviceHelper().setVideoBitRates(400);
			checkSDKversion();
		}
		
		
		CCPApplication.getInstance().initSQLiteManager();
	}
	
	
	
	private void initLayoutResource() {
		mScrollView = (ScrollView) findViewById(R.id.ccp_body_sv);
	}

	
	private void initCapacityUI() {
		
		LinearLayout layout = new LinearLayout(this);
		layout.setOrientation(LinearLayout.VERTICAL);
		LinearLayout.LayoutParams bottomLayoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT
				,LinearLayout.LayoutParams.MATCH_PARENT);
		layout.setLayoutParams(bottomLayoutParams);
		
//		View attentionView = getLayoutInflater().inflate(R.layout.ccp_attention, null);
//		attentionView.setPadding(0, getResources().getDimensionPixelSize(R.dimen.small_margin_space), 0, 0);
//		TextView findViewById = (TextView) attentionView.findViewById(R.id.attention_tips);
//		findViewById.setText(R.string.str_capacity_attention);
//		layout.addView(attentionView);
		
		// voice item
		int[] voiceIconResid = new int[]{
				R.drawable.ccp_landing_calls,
				R.drawable.ccp_video_call};
		
		int[] voiceBgResid= new int[]{
				R.drawable.ccp_capacity_09_selector ,
				R.drawable.ccp_capacity_09_selector};
		
		CCPCapabilityItemView voiceCapacityItemView = getCCPCapacityItemView(R.string.str_voice, voiceIconResid, voiceBgResid);
		layout.addView(voiceCapacityItemView);
		
		// video item
		int[] videoIconResid = new int[]{
				R.drawable.ccp_voice_group_chat,
//				R.drawable.ccp_video_conference,
//				R.drawable.ccp_intercom
		};

		int[] videoBgResid= new int[]{
				R.drawable.ccp_capacity_08_selector,
//				R.drawable.ccp_capacity_05_selector,
//				R.drawable.ccp_capacity_04_selector
		};

		CCPCapabilityItemView videoCapacityItemView = getCCPCapacityItemView(R.string.str_video, videoIconResid, videoBgResid);
		layout.addView(videoCapacityItemView);
//
//		// IVR item
//		int[] ivrIconResid = new int[]{
//				R.drawable.ccp_voice_verification_code,
//				R.drawable.ccp_market_outside_call };
//
//		int[] ivrBgResid= new int[]{
//				R.drawable.ccp_capacity_01_selector,
//				R.drawable.ccp_capacity_06_selector};
//
//		CCPCapabilityItemView ivrCapacityItemView = getCCPCapacityItemView(R.string.str_ivr, ivrIconResid, ivrBgResid);
//		layout.addView(ivrCapacityItemView);
		
		// other item
		int[] moreIconResid = new int[]{
//				R.drawable.ccp_im,
//				R.drawable.ccp_contact,
//				R.drawable.ccp_ex_consultation ,
				R.drawable.ccp_setting };
		
		int[] moreBgResid= new int[]{
//				R.drawable.ccp_capacity_03_selector ,
//				R.drawable.ccp_capacity_09_selector,
//				R.drawable.ccp_capacity_07_selector ,
				R.drawable.ccp_capacity_02_selector };
		
		CCPCapabilityItemView moreCapacityItemView = getCCPCapacityItemView(R.string.str_more, moreIconResid, moreBgResid);
		layout.addView(moreCapacityItemView);
		
		mScrollView.addView(layout);
	}
	
	/**
	 * 
	 * @param capacityTextid The Capacity View title
	 * @param iconResid The capacity view icon
	 * @param bgResid The capacity view background resource.
	 * @return
	 */
	public CCPCapabilityItemView getCCPCapacityItemView(int capacityTextid , int[] iconResid , int[] bgResid) {
		// other item
		List<Map<Integer, Integer>> capacityViews = new ArrayList<Map<Integer, Integer>>();
		CCPCapabilityItemView iCapacityItemView = new CCPCapabilityItemView(this);
		iCapacityItemView.setCapabilityId(capacityTextid);
		iCapacityItemView.setCapacityText(capacityTextid);
		for(int i = 0 ; i < iconResid.length ; i++) {
			HashMap<Integer, Integer> iResids = new HashMap<Integer, Integer>();
			iResids.put(iconResid[i], bgResid[i]);
			capacityViews.add(iResids);
		}
		iCapacityItemView.setCapacityItem(capacityViews);
		iCapacityItemView.setOnCapacityItemClickListener(this);
		
		return iCapacityItemView;
	}


	@Override
	protected int getLayoutId() {
		return R.layout.ccp_choice_capacity;
	}



	@Override
	public void OnCapacityItemClick(int id , int resid) {
		switch (resid) {
		case R.drawable.ccp_voice_group_chat:
			startActivity(new Intent(CapabilityChoiceActivity.this, ChatroomConversation.class));
			break;
		case R.drawable.ccp_voice_verification_code:
			
			startActivity(new Intent(CapabilityChoiceActivity.this, VoiceVerificationCodeActivity.class));
			break;
		case R.drawable.ccp_intercom:
			
			startActivity(new Intent(CapabilityChoiceActivity.this, InterPhoneActivity.class));
			break;
		case R.drawable.ccp_landing_calls:
			startActivity(new Intent(CapabilityChoiceActivity.this, NetPhoneCallActivity.class));
			
			break;
		case R.drawable.ccp_video_call:
			Intent intent = new Intent(CapabilityChoiceActivity.this, InviteInterPhoneActivity.class);
			intent.setFlags(Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
			intent.putExtra("create_to", InviteInterPhoneActivity.CREATE_TO_VIDEO_CALL);
			startActivityForResult(intent, REQUEST_CODE_VIDEO_CALL);
			
			break;
		case R.drawable.ccp_video_one2more:
		case R.drawable.ccp_video_conference:
			Intent videoConfIntent = new Intent(CapabilityChoiceActivity.this, WizardVideoconferenceActivity.class);
			if(resid == R.drawable.ccp_video_conference) {
				videoConfIntent.putExtra(VideoconferenceConversation.CONFERENCE_TYPE, 1);
				startActivity(videoConfIntent);
			} else if (resid == R.drawable.ccp_video_one2more) {
				videoConfIntent.putExtra(VideoconferenceConversation.CONFERENCE_TYPE, 0);
				startActivity(videoConfIntent);
			}
			break;
		case R.drawable.ccp_ex_consultation:
			startActivity(new Intent(CapabilityChoiceActivity.this, ExpertMainActivity.class));
			
			break;
		case R.drawable.ccp_market_outside_call:
			startActivity(new Intent(CapabilityChoiceActivity.this, MarketActivity.class));
			
			break;
		case R.drawable.ccp_setting:
			startActivity(new Intent(CapabilityChoiceActivity.this, SettingsActivity.class));
			
			break;
		case R.drawable.ccp_im:
			startActivity(new Intent(CapabilityChoiceActivity.this, GroupMessageListActivity.class));
			
			break;
		case R.drawable.ccp_contact:
			 //contacts list or contact management for bakcup and restore??
			startActivity(new Intent(CapabilityChoiceActivity.this, ContactListActivity.class));
			 
			break;
		default:
			break;
		}
	}
	
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		phoneStr = null;
		
		if(mScrollView != null) {
			mScrollView.removeAllViews();
			mScrollView = null;
		}
	}

	
	private void initAudioConfig(){
		SharedPreferences sp = CcpPreferences.getSharedPreferences();
		//设置AUDIO_AGC
		Boolean flag = CcpPreferences.getSharedPreferences().getBoolean(CCPPreferenceSettings.SETTING_AUTO_MANAGE.getId(), (Boolean)CCPPreferenceSettings.SETTING_AUTO_MANAGE.getDefaultValue());
		AudioMode audioMode = CCPUtil.getAudioMode(AudioType.AUDIO_AGC,	sp.getInt("AUTOMANAGE_INDEX_KEY", 3));
		getDeviceHelper().setAudioConfigEnabled(AudioType.AUDIO_AGC, flag, audioMode);
		
		//设置AUDIO_EC
		flag = sp.getBoolean("ECHOCANCELLED_SWITCH_KEY", true);
		audioMode = CCPUtil.getAudioMode(AudioType.AUDIO_EC, sp.getInt("ECHOCANCELLED_INDEX_KEY", 4));
		getDeviceHelper().setAudioConfigEnabled(AudioType.AUDIO_EC, flag, audioMode);

		//设置AUDIO_NS
		flag = sp.getBoolean("SILENCERESTRAIN_SWITCH_KEY", true);
		audioMode = CCPUtil.getAudioMode(AudioType.AUDIO_NS, sp.getInt("SILENCERESTRAIN_INDEX_KEY", 6));
		getDeviceHelper().setAudioConfigEnabled(AudioType.AUDIO_NS, flag, audioMode);

		//设置码流
		flag = sp.getBoolean("VIDEOSTREAM_SWITCH_KEY", false);
		if(flag){
			String bitString = sp.getString("VIDEOSTREAM_CONTENT_KEY", "400");
			int bitrates = Integer.valueOf(bitString).intValue();
			getDeviceHelper().setVideoBitRates(bitrates);
		}

	}
	
//  Video ...
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);

		Log4Util.d(CCPHelper.DEMO_TAG ,"[SelectVoiceActivity] onActivityResult: requestCode=" + requestCode
				+ ", resultCode=" + resultCode + ", data=" + data);

		// If there's no data (because the user didn't select a number and
		// just hit BACK, for example), there's nothing to do.
		if (requestCode != REQUEST_CODE_VIDEO_CALL ) {
			if (data == null) {
				return;
			}
		} else if (resultCode != RESULT_OK) {
			Log4Util.d(CCPHelper.DEMO_TAG ,"[SelectVoiceActivity] onActivityResult: bail due to resultCode=" + resultCode);
			return;
		}
		
		phoneStr = "";
		if(data.hasExtra("VOIP_CALL_NUMNBER")) {
			Bundle extras = data.getExtras();
			if (extras != null) {
				phoneStr = extras.getString("VOIP_CALL_NUMNBER");
			}
		}
		
		if(TextUtils.isEmpty(phoneStr)) {
			CCPApplication.getInstance().showToast(R.string.edit_input_empty);
			return ;
		}

		/*// VOIP免费电话
		if (!phoneStr.startsWith("8") || phoneStr.length() != 14) {
			// 判断输入合法性
			CCPApplication.getInstance().showToast(
					getString(R.string.voip_number_format));
			return;
		}*/
		
		if(VoiceUtil.checkNetworkAPNType(this) == APN.UNKNOWN) {
			showNetworkNotAvailable();
			return;
		}
		
		if(VoiceUtil.checkNetworkAPNType(this) != APN.WIFI) {
			showVoIPWifiWarnning();
		} else {
			startVoIPCallAction();
		}
		
		
	}
	
	@Override
	protected void handleDialogOkEvent(int requestKey) {
		
		if(requestKey == DIALOG_REQUEST_VOIP_NOT_WIFI_WARNNING) {
			startVoIPCallAction();
		} else {
			super.handleDialogOkEvent(requestKey);
		}
	}

	private void startVoIPCallAction() {
		if(!TextUtils.isEmpty(phoneStr)) {
			Intent intent = new Intent(CapabilityChoiceActivity.this, VideoActivity.class);
			intent.putExtra(CCPApplication.VALUE_DIAL_VOIP_INPUT, phoneStr);
			startActivity(intent);
		}
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {
			showExitDialog();
			return true;
		}

		return super.onKeyDown(keyCode, event);
	}
	
	private void checkSDKversion() {
		SDKVersion sdkVersion = CCPUtil.getSDKVersion(getDeviceHelper().getVersion());
		if(sdkVersion != null) {
			String tips = getString(R.string.str_sdk_support_tips, Build.SDK_VERSION_NAME,String.valueOf(Build.LIBVERSION.AUDIO_SWITCH)
					,String.valueOf(Build.LIBVERSION.VIDEO_SWITCH),CCPApplication.getInstance().getVersion());
			//addNotificatoinToView(tips);
			Log4Util.i(CCPHelper.DEMO_TAG, "The current SDK version number :" + sdkVersion.toString());
		}
	}
	
	
	void showExitDialog() {
		CCPAlertDialog ccpAlertDialog = new CCPAlertDialog(CapabilityChoiceActivity.this
				, R.string.settings_logout_warning_tip
				, null
				, R.string.settings_logout
				, R.string.dialog_cancle_btn);
		
		// set CCP UIKey
		ccpAlertDialog.setOnItemClickListener(this);
		ccpAlertDialog.create();
		ccpAlertDialog.show();
	}
	
	
	@Override
	protected void onReceiveBroadcast(Intent intent) {
		super.onReceiveBroadcast(intent);
		if (intent != null && CCPIntentUtils.INTENT_CONNECT_CCP.equals(intent.getAction())) {
			CCPApplication.getInstance().showToast(R.string.ccp_http_connect);
		}  else if (intent != null && CCPIntentUtils.INTENT_DISCONNECT_CCP.equals(intent.getAction())) {
			CCPApplication.getInstance().showToast(R.string.ccp_http_err);
			
		} 
	}
	
	
	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		
		if(iTask.getKey() == TaskKey.KEY_QUERY_UNDOWNLOAD_IMMESSAGE) {
			ArrayList<IMChatMessageDetail> unImMessage = CCPSqliteManager.getInstance().queryNotDownloadIMVoiceMessages();
			ArrayList<DownloadInfo> dLoadList = new ArrayList<DownloadInfo>();
			if(unImMessage != null) {
				for(IMChatMessageDetail imMessage : unImMessage) {
					
					dLoadList.add(new DownloadInfo(imMessage.getFileUrl(), imMessage.getFilePath() , false));
					CCPApplication.getInstance().putMediaData(imMessage.getFilePath(), imMessage);
					Log4Util.d(CCPHelper.DEMO_TAG, "execut download undownload immessage :" + imMessage.getFileUrl());
				}
			}
			if(dLoadList.isEmpty()) {
				Log4Util.d(CCPHelper.DEMO_TAG, "there has no  undownload immessag:");
				return;
			}
			if(checkeDeviceHelper()) {
				getDeviceHelper().downloadAttached(dLoadList);
			}
		} else if (iTask.getKey() == TaskKey.KEY_SDK_UNREGIST) {
			try {
				// update sending message status ...
				CCPSqliteManager.getInstance().updateAllIMMessageSendFailed();
				
			} catch (SQLException e) {
				e.printStackTrace();
			}
			
			CCPUtil.exitCCPDemo();
			closeConnectionProgress();
			
			launchCCP();
			
			finish();
		}
	}



	/* (non-Javadoc)
	 * @see com.voice.demo.views.CCPAlertDialog.OnPopuItemClickListener#onItemClick(android.widget.ListView, android.view.View, int, int)
	 */
	@Override
	public void onItemClick(ListView parent, View view, int position,
			int resourceId) {
		switch (resourceId) {
		case R.string.settings_logout:
			showConnectionProgress(getString(R.string.ccp_logout_processing_txt));
			
			ITask iTask = new ITask(TaskKey.KEY_SDK_UNREGIST);
			addTask(iTask);

			break;
		case R.string.dialog_cancle_btn:
			
			break;

		default:
			break;
		}
	}
	
}
