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
package com.voice.demo.setting;

import java.util.HashMap;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.text.InputFilter;
import android.text.InputType;
import android.text.TextUtils;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.hisun.phone.core.voice.Device.AudioMode;
import com.hisun.phone.core.voice.Device.AudioType;
import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.netmonitor.NetworkMonitoringActivity;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.ui.CCPHelper;

public class SettingsActivity extends CCPBaseActivity implements View.OnClickListener ,SlipButton.OnChangedListener{

	private static final int REUQEST_KEY_SHOW_VIDEOSTREAMCONTROL = 0x1;
	private static final int REUQEST_KEY_SHOW_P2P = 0x2;
	private static final int REUQEST_KEY_SHOW_P2P_STARTBITRATE = 0x3;
	
	@Deprecated
	public static final String P2P_SERVER_DEFAULT = "42.121.15.99";
	@Deprecated
	public static final int P2P_PORT_DEFAULT = 3478;
	
	private int[] titleNameArray = {
			R.string.str_setting_item_automanage,           // Automatic gain control(自动增益控制)
			R.string.str_setting_item_echocancel,           // Echo cancellation(回音消除)
			R.string.str_setting_item_silencerestrain,      // Silence suppression(静音抑制)
			R.string.str_setting_item_videostreamcontrol,   // Video control(视频码流控制)
			R.string.str_setting_item_codec,   				// 支持编解码
			R.string.str_setting_item_netcheck,             // net check (网络检测)
			R.string.str_setting_item_ischunked,            // While recording and uploading pronunciation（语音边录边传）
			R.string.str_setting_item_p2p,                  // P2P 
			R.string.str_setting_item_p2p_startbitrate,     // set larger stream (P2P通话码流设置)
			R.string.str_setting_item_p2p_video_mosaic,      // shielded video decoding process of mosaic(P2P视频通话马赛克抑制)
			R.string.str_use_handset,     					// audio speaker on or false
			R.string.str_setting_item_video_call_resolution,   				// 视频通话分辨率
			R.string.str_setting_item_abount      			//abount us
			}; 
	
	public static class CCPSetting {
		public static final int SETTING_AUTOMANAGE_ID = 1;
		public static final int SETTING_ECHOCANCEL_ID = 2;
		public static final int SETTING_SILENCERESTRAIN_ID = 3;
		public static final int SETTING_VIDEOSTREAMCONTROL_ID = 4;
		public static final int SETTING_CODE = 5;
		public static final int SETTING_NETCHECK_ID = 6;
		public static final int SETTING_ISCHUNKED_ID = 7;
		public static final int SETTING_P2P_ID = 8;
		public static final int SETTING_P2P_STARTBITRATE_ID = 9;
		public static final int SETTING_P2P_VIDEO_MOSAIC_ID = 10;
		public static final int SETTING_USE_HANDSET = 11;
		public static final int SETTING_VIDEO_CALL_RESOLUTION = 12;
		public static final int SETTING_ABOUT = 13;//add abount us
		
		/**
		 * Voice /Video calls record.
		 */
		public static final int SETTING_CALL_RECORDING   = 11;
	}
	
	/**
	 * Automatic gain control(自动增益控制)
	 */
	public String[] settingAutomanage = null;
	
	/**
	 * Echo cancellation(回音消除)
	 */
	public String[] settingEchocancelled = null;
	
	/**
	 * Silence suppression(静音抑制)
	 */
	public String[] settingSilencerestrain = null;
	
	private HashMap<Integer, String>	mSettingArray;
	private HashMap<Integer, Boolean>   mSettingCheckArray;
	private LinearLayout mOtherLayout ;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		handleTitleDisplay(getString(R.string.btn_title_back),
				getString(R.string.str_setting_head), null);
		
		
		mOtherLayout = (LinearLayout) findViewById(R.id.setting_other);
		
		initArrayResource();
		
		initSettingValue();
		
		initViewListView();
		
		registerReceiver(new String[]{CCPIntentUtils.INTENT_P2P_ENABLED});
	}
	
	/**
	 * init automanage echocancle and silencerestrain resource.
	 */
	public void initArrayResource() {
		settingAutomanage = getResources().getStringArray(R.array.setting_automanage);
		settingEchocancelled = getResources().getStringArray(R.array.setting_echocancelled);
		settingSilencerestrain = getResources().getStringArray(R.array.setting_silencerestrain);
	}
	
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
		TextView mNoticeTips  = (TextView) view.findViewById(R.id.notice_tips);
		if(id == CCPSetting.SETTING_AUTOMANAGE_ID) {
			mNoticeTips.setText(R.string.str_setting_head1);
			mNoticeTips.setVisibility(View.VISIBLE);
		} else if (id == CCPSetting.SETTING_NETCHECK_ID) {
			mNoticeTips.setText(R.string.str_setting_head2);
			mNoticeTips.setVisibility(View.VISIBLE);
		} else {
			mNoticeTips.setVisibility(View.GONE);
		}
		
		
		
		TextView viewName = (TextView) view.findViewById(R.id.item_textView);
		viewName.setText(getString(titleNameArray[index]));
		TextView viewSubName = (TextView) view.findViewById(R.id.item_sub_textView);
		//viewSubName.setText(getString(titleNameArray[index]));
		if(mSettingArray != null && !TextUtils.isEmpty(mSettingArray.get(id))) {
			viewSubName.setText(mSettingArray.get(id));
			viewSubName.setVisibility(View.VISIBLE);
		} else{
			viewSubName.setVisibility(View.GONE);
		}
		if(id != CCPSetting.SETTING_NETCHECK_ID && id != CCPSetting.SETTING_P2P_STARTBITRATE_ID&& id != CCPSetting.SETTING_ABOUT) {
			
			SlipButton slipButton = (SlipButton) view.findViewById(R.id.switch_btn);
			if(mSettingCheckArray != null && mSettingCheckArray.get(id) != null) {
				slipButton.setChecked(mSettingCheckArray.get(id));
				slipButton.setVisibility(View.VISIBLE);
				//if(id != CCPSetting.SETTING_P2P_ID) {
					// just for demo test ..
					// The current P2P unstable, if you need to set up, 
					// the judge can be removed
					slipButton.SetOnChangedListener(getString(titleNameArray[index]), this);
				//}
			} else{
				slipButton.setVisibility(View.GONE);
			}
		}
		
		//if(id != CCPSetting.SETTING_P2P_ID) {
			// just for demo test ..
			// The current P2P unstable, if you need to set up, 
			// the judge can be removed
			view.setOnClickListener(this);
		//}
		
		return view;
	}
	
	/**
	 * 创建一条栏目
	 *
	 * @param id
	 */
	private void addItemSettingView(int id) {
		 
		if(id != CCPSetting.SETTING_P2P_ID 
				|| id != CCPSetting.SETTING_P2P_STARTBITRATE_ID
				|| id != CCPSetting.SETTING_P2P_VIDEO_MOSAIC_ID) {
			View view = createItemView(R.layout.list_setting_item, id);
			mOtherLayout.addView(view);
			
		}

	}
	
	/**
	 *
	 * 初始化设置列表;
	 */
	private void initViewListView() {
		for (int i = 1; i <= CCPSetting.SETTING_ABOUT; i++) {
			if(i == CCPSetting.SETTING_P2P_ID || i == CCPSetting.SETTING_P2P_STARTBITRATE_ID) {
				continue;
			}
			addItemSettingView(i);
		}
	}
	
	
	void initSettingValue(){
        if(mSettingArray == null ) {
        	mSettingArray = new HashMap<Integer, String>();
        }
        mSettingArray.clear();
        
		putSetingArrayValue(
				CCPSetting.SETTING_AUTOMANAGE_ID,
				settingAutomanage[getSettingIntValue(CCPPreferenceSettings.SETTING_AUTO_MANAGE_VALUE)]);
		// echocancel default sub text.
		putSetingArrayValue(
				CCPSetting.SETTING_ECHOCANCEL_ID,
				settingEchocancelled[getSettingIntValue(CCPPreferenceSettings.SETTING_ECHOCANCEL_VALUE)]);
		// silencerestrain default sub text.
		putSetingArrayValue(
				CCPSetting.SETTING_SILENCERESTRAIN_ID,
				settingSilencerestrain[getSettingIntValue(CCPPreferenceSettings.SETTING_SILENCERESTRAIN_VALUE)]);

		// video stream value.
		putSetingArrayValue(
				CCPSetting.SETTING_VIDEOSTREAMCONTROL_ID,
				getSettingIntValue(CCPPreferenceSettings.SETTING_VIDEO_STREAM_CONTROL_VALUE)
						+ "");
		
		putSetingArrayValue(
				CCPSetting.SETTING_CODE,"");
		putSetingArrayValue(
				CCPSetting.SETTING_VIDEO_CALL_RESOLUTION,"");

		// p2p server address
		putSetingArrayValue(
				CCPSetting.SETTING_P2P_ID,
				getSettingStrValue(CCPPreferenceSettings.SETTING_P2P_VALUE));

		// p2p startbitrate value.
		putSetingArrayValue(
				CCPSetting.SETTING_P2P_STARTBITRATE_ID,
				getSettingIntValue(CCPPreferenceSettings.SETTING_P2P_STARTBITRATE_VALUE)
						+ "");

        initSettingDefaultEnable();
    }
	
	void initSettingDefaultEnable() {
		if (mSettingCheckArray == null) {
			mSettingCheckArray = new HashMap<Integer, Boolean>();
		}
		mSettingCheckArray.clear();
		
		putSettingDefaultEnable(CCPSetting.SETTING_AUTOMANAGE_ID, CCPPreferenceSettings.SETTING_AUTO_MANAGE);
		putSettingDefaultEnable(CCPSetting.SETTING_ECHOCANCEL_ID, CCPPreferenceSettings.SETTING_ECHOCANCEL);
		putSettingDefaultEnable(CCPSetting.SETTING_SILENCERESTRAIN_ID, CCPPreferenceSettings.SETTING_SILENCERESTRAIN);
		putSettingDefaultEnable(CCPSetting.SETTING_VIDEOSTREAMCONTROL_ID, CCPPreferenceSettings.SETTING_VIDEO_STREAM_CONTROL);
		putSettingDefaultEnable(CCPSetting.SETTING_CODE, CCPPreferenceSettings.SETTING_CODE);
		putSettingDefaultEnable(CCPSetting.SETTING_ISCHUNKED_ID, CCPPreferenceSettings.SETTING_VOICE_ISCHUNKED);
		putSettingDefaultEnable(CCPSetting.SETTING_P2P_ID, CCPPreferenceSettings.SETTING_P2P);
		putSettingDefaultEnable(CCPSetting.SETTING_P2P_STARTBITRATE_ID, CCPPreferenceSettings.SETTING_P2P_STARTBITRATE);
		putSettingDefaultEnable(CCPSetting.SETTING_P2P_VIDEO_MOSAIC_ID, CCPPreferenceSettings.SETTING_P2P_VIDEO_MOSAIC);
		putSettingDefaultEnable(CCPSetting.SETTING_USE_HANDSET, CCPPreferenceSettings.SETTING_HANDSET);
		
		putSettingDefaultEnable(CCPSetting.SETTING_CALL_RECORDING, CCPPreferenceSettings.SETTING_CALL_RECORDING);
	}
	
	/**
	 * 
	 * @param id
	 * @param spKey
	 * @return
	 */
	int getSettingIntValue(CCPPreferenceSettings spKey) {
		return  getSharedPreferences().getInt(spKey.getId(),
				((Integer) spKey.getDefaultValue()).intValue());

	}
	
	/**
	 * 
	 * @param spKey
	 * @return
	 */
	String getSettingStrValue(CCPPreferenceSettings spKey) {
		return  getSharedPreferences().getString(spKey.getId(),
				(String) spKey.getDefaultValue());
	}
	
	/**
	 * 
	 * @param spKey
	 * @return
	 */
	boolean getSettingBoolValue(CCPPreferenceSettings spKey) {
		return getSharedPreferences().getBoolean(spKey.getId(),
				((Boolean) spKey.getDefaultValue()).booleanValue());
	}
	
	/**
	 * 
	 * @param key
	 * @param value
	 */
	void putSetingArrayValue(int key ,String value) {
		mSettingArray.put(key, value);
	}
	
	/**
	 * 
	 * @param id
	 * @param spKey
	 * @return
	 */
	void putSettingDefaultEnable(int id ,CCPPreferenceSettings spKey) {

		mSettingCheckArray.put(id, getSettingBoolValue(spKey));
	}
	
	@Override
	public void onClick(View v) {
		int id = v.getId();
		Intent intent = null;
		switch (id) {
		case CCPSetting.SETTING_AUTOMANAGE_ID: {
			if (mSettingCheckArray != null && mSettingCheckArray.get(id)) {
				// switch״̬Ϊon�������û���ȥ{
				intent = new Intent(SettingsActivity.this, AutoManageSettingActivity.class);
				intent.putExtra("SettingType", AutoManageSettingActivity.SETTING_AUTOMANAGE);
				startActivityForResult(intent, AutoManageSettingActivity.SETTING_AUTOMANAGE);
			}
			 
			break;
		}
		case CCPSetting.SETTING_ECHOCANCEL_ID: {
			if (mSettingCheckArray != null && mSettingCheckArray.get(id)) {
				intent = new Intent(SettingsActivity.this, AutoManageSettingActivity.class);
				intent.putExtra("SettingType", AutoManageSettingActivity.SETTING_ECHOCANCELLED);
				startActivityForResult(intent, AutoManageSettingActivity.SETTING_ECHOCANCELLED);
			}
			break;
		}
		case CCPSetting.SETTING_SILENCERESTRAIN_ID: {
			if (mSettingCheckArray != null && mSettingCheckArray.get(id)) {
				intent = new Intent(SettingsActivity.this, AutoManageSettingActivity.class);
				intent.putExtra("SettingType", AutoManageSettingActivity.SETTING_SILENCERESTRAIN);
				startActivityForResult(intent, AutoManageSettingActivity.SETTING_SILENCERESTRAIN);
			}
			break;
		}
		case CCPSetting.SETTING_VIDEOSTREAMCONTROL_ID:{
			if (mSettingCheckArray != null && mSettingCheckArray.get(id)) {
				//视频码流控制 { if (listItemAdapter.getIsSelected().get(3))
				//如果用户把视频码流的switch关掉，则不允许输入 { showEditDialog(); } }
				showEditDialog(REUQEST_KEY_SHOW_VIDEOSTREAMCONTROL , InputType.TYPE_CLASS_NUMBER
						, getString(R.string.str_setting_item_videostreamcontrol), getString(R.string.str_setting_dialog_input_vide_rule));
			}
			break;
		}
		case CCPSetting.SETTING_CODE:
			startActivity(new Intent(SettingsActivity.this, SupportCodecActivity.class));
			break;
		case CCPSetting.SETTING_NETCHECK_ID:
			startActivity(new Intent(SettingsActivity.this, NetworkMonitoringActivity.class));
			break;
		case CCPSetting.SETTING_ABOUT:
			startActivity(new Intent(SettingsActivity.this, SettingAboutActivity.class));
			break;
		case CCPSetting.SETTING_VIDEO_CALL_RESOLUTION:
			startActivity(new Intent(SettingsActivity.this, VideoCallResolutionSettings.class));
			break;
		case CCPSetting.SETTING_ISCHUNKED_ID:
			break;
		case CCPSetting.SETTING_P2P_ID:
			/*if (mSettingCheckArray != null && mSettingCheckArray.get(id)) {
			//视频码流控制 { if (listItemAdapter.getIsSelected().get(3))
			//如果用户把视频码流的switch关掉，则不允许输入 { showEditDialog(); } }
			showEditDialog(REUQEST_KEY_SHOW_P2P, InputType.TYPE_CLASS_TEXT
					, getString(R.string.str_setting_item_p2p), getString(R.string.str_setting_dialog_input_p2p_rule));
		}*/
			break;
			
		case CCPSetting.SETTING_P2P_STARTBITRATE_ID:
			
			// for p2p enable.
			if(mSettingCheckArray != null && mSettingCheckArray.get(CCPSetting.SETTING_P2P_ID)){
				showEditDialog(REUQEST_KEY_SHOW_P2P_STARTBITRATE , InputType.TYPE_CLASS_NUMBER
						, getString(R.string.str_setting_item_p2p_startbitrate), getString(R.string.str_setting_dialog_input_startbitrate_rule));
			}
			
			break;
		}
	}
	
	
	void showEditDialog(final int requestKey , int inputType , String title , String message)
	{		
		Dialog dialog = null;
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setTitle(/*R.string.str_setting_item_videostreamcontrol*/title);
		View view = getLayoutInflater().inflate(R.layout.dialog_edittext_and_textview, null);
		TextView dialogText = (TextView)view.findViewById(R.id.notice_tips);
		if(!TextUtils.isEmpty(message)){
			dialogText.setVisibility(View.VISIBLE);
			dialogText.setText(message);
		} else {
			dialogText.setVisibility(View.GONE);
		}
		final EditText mInvitEt = (EditText) view.findViewById(R.id.video_stream_edit);
		mInvitEt.setInputType(inputType);
		if(inputType == InputType.TYPE_CLASS_NUMBER) {
			mInvitEt.setFilters(new InputFilter[]{new InputFilter.LengthFilter(5)});
		}
		
		// Monitor whether focus pop-up or hidden input....
		mInvitEt.setOnFocusChangeListener(new OnFocusChangeListener() {
			@Override
			public void onFocusChange(View v, final boolean hasFocus) {
				if (hasFocus) {
					new Handler().postDelayed(new Runnable() {
						public void run() {
							InputMethodManager imm = (InputMethodManager) mInvitEt
									.getContext().getSystemService(
											Context.INPUT_METHOD_SERVICE);

							if (hasFocus) {
								imm.toggleSoftInput(0,
										InputMethodManager.HIDE_NOT_ALWAYS);
							} else {
								imm.hideSoftInputFromWindow(
										mInvitEt.getWindowToken(), 0);
							}
						}
					}, 300);
				}
			}
		});
		
		builder.setPositiveButton(R.string.dialog_btn,
				new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				String mPhoneNumber = mInvitEt.getText().toString();
				handleEditDialogOkEvent(requestKey, mPhoneNumber, true);
				dialog.dismiss();
			}
		});
		
		builder.setNegativeButton(R.string.dialog_cancle_btn,
				new DialogInterface.OnClickListener() 
		{
			public void onClick(DialogInterface dialog, int which) 
			{
				dialog.dismiss();
			}

		});
		
		builder.setView(view);
		dialog = builder.create();
		dialog.show();
		dialog.setCanceledOnTouchOutside(false);
	}
	
	
	@Override
	protected void handleEditDialogOkEvent(int requestKey, String editText,
			boolean checked) {
		super.handleEditDialogOkEvent(requestKey, editText, checked);
		try {
			
			if(REUQEST_KEY_SHOW_VIDEOSTREAMCONTROL == requestKey){
				
				int bitrates = 150;
				if(!TextUtils.isEmpty(editText)) {
					
					bitrates = Integer.valueOf(editText).intValue();
					CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_VIDEO_STREAM_CONTROL_VALUE, bitrates, true);
					if(checkeDeviceHelper()) getDeviceHelper().setVideoBitRates(bitrates);
				}
				putSetingArrayValue(CCPSetting.SETTING_VIDEOSTREAMCONTROL_ID, bitrates + "");
				changeSubView(CCPSetting.SETTING_VIDEOSTREAMCONTROL_ID);
			} else if (REUQEST_KEY_SHOW_P2P == requestKey) {
				putSetingArrayValue(CCPSetting.SETTING_P2P_ID, editText);
				CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_P2P_VALUE, editText, true);
				changeSubView(CCPSetting.SETTING_P2P_ID);
			} else if (REUQEST_KEY_SHOW_P2P_STARTBITRATE == requestKey) {
				
				int startBitrate = 330; // default 330
				if(!TextUtils.isEmpty(editText)) {
					startBitrate = Integer.valueOf(editText).intValue();
					CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_P2P_STARTBITRATE_VALUE, startBitrate, true);
					// @version 3.4.1.2 hide
					//VoiceHelper.getInstance().getDevice().setStartBitRateAfterP2PSucceed(startBitrate);
				}
				putSetingArrayValue(CCPSetting.SETTING_P2P_STARTBITRATE_ID, editText);
				changeSubView(CCPSetting.SETTING_P2P_STARTBITRATE_ID);
				
			}
		} catch (Exception e) {
		}
	}

	@Override
	public void onChanged(String strname, boolean checkState) {
        
        AudioType audioType = AudioType.AUDIO_AGC;
        AudioMode audioMode = AudioMode.kNsDefault;
        
        try {
			
        	if (strname.equals(getString(titleNameArray[3])))
        	{
        		CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_VIDEO_STREAM_CONTROL, checkState, true);
        		int bitrates = 150;
        		
        		if (checkState)
        		{
        			bitrates = getSharedPreferences().getInt(CCPPreferenceSettings.SETTING_VIDEO_STREAM_CONTROL_VALUE.getId() ,(Integer) CCPPreferenceSettings.SETTING_VIDEO_STREAM_CONTROL_VALUE.getDefaultValue() );
        		}
        		
        		if(checkeDeviceHelper()) getDeviceHelper().setVideoBitRates(bitrates);
        	} else if (getString(titleNameArray[5]).equals(strname)) {
        		CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_VOICE_ISCHUNKED, checkState, true);
        	}/* else if (getString(titleNameArray[7]).equals(strname)) {
			
			// set larger stream (P2P通话码流设置)
			VoiceHelper.getInstance().getDevice().set
		} */else if (getString(titleNameArray[8]).equals(strname)) {
			
			// shielded video decoding process of mosaic(P2P视频通话马赛克抑制)
			CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_P2P_VIDEO_MOSAIC, checkState, true);
			
			// @version 3.4.1.2 hide
			//VoiceHelper.getInstance().getDevice().setShieldMosaic(checkState);
		} else if (getString(titleNameArray[9]).equals(strname)) {
			
			// audio speaker on ..
			CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_HANDSET, checkState, true);
			
		}else if (getString(titleNameArray[6]).equals(strname)) {
			if(checkState) {
				
				// Set to 1 to enable p2p
				// Set to 0 to disable the P2P
				String p2pServerPort = getSharedPreferences().getString(CCPPreferenceSettings.SETTING_P2P_VALUE.getId() , (String)CCPPreferenceSettings.SETTING_P2P_VALUE.getDefaultValue());
				String[] split = p2pServerPort.split(":");
				if(split != null && split.length > 1){
					/*int port = P2P_PORT_DEFAULT;
					try {
						port = Integer.parseInt(split[1]);
					} catch (NumberFormatException e) {
						e.printStackTrace();
					}*/
					
					//VoiceApplication.getInstance().getVoiceHelper().getDevice().setStunServer(split[0], port);
				} else if (split != null && split.length ==1) {
					//VoiceApplication.getInstance().getVoiceHelper().getDevice().setStunServer(split[0]);
				}
				//VoiceHelper.getInstance().getDevice().setFirewallPolicy(1);
			} else {
				//VoiceHelper.getInstance().getDevice().setFirewallPolicy(0);
			}
			CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_P2P, checkState, true);
		} else if (getString(titleNameArray[10]).equals(strname)) {
			CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_CALL_RECORDING, checkState, true);
			
		} else {
			if (strname.equals(getString(titleNameArray[0])))
			{
				CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_AUTO_MANAGE, checkState, true);
				audioType = AudioType.AUDIO_AGC;
				audioMode = CCPUtil.getAudioMode(audioType, getSharedPreferences().getInt(CCPPreferenceSettings.SETTING_AUTO_MANAGE_VALUE.getId()
						 , (Integer)CCPPreferenceSettings.SETTING_AUTO_MANAGE_VALUE.getDefaultValue()));
			}
			else if (strname.equals(getString(titleNameArray[1])))
			{
				CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_ECHOCANCEL, checkState, true);
				audioType = AudioType.AUDIO_EC;
				audioMode = CCPUtil.getAudioMode(audioType, getSharedPreferences().getInt(CCPPreferenceSettings.SETTING_ECHOCANCEL_VALUE.getId()
						, (Integer)CCPPreferenceSettings.SETTING_ECHOCANCEL_VALUE.getDefaultValue()));
			}
			else if (strname.equals(getString(titleNameArray[2])))
			{
				CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_SILENCERESTRAIN, checkState, true);
				audioType = AudioType.AUDIO_NS;
				audioMode = CCPUtil.getAudioMode(audioType, getSharedPreferences().getInt(CCPPreferenceSettings.SETTING_SILENCERESTRAIN_VALUE.getId()
						, (Integer)CCPPreferenceSettings.SETTING_SILENCERESTRAIN_VALUE.getDefaultValue()));
			}
			
			if(checkeDeviceHelper()) getDeviceHelper().setAudioConfigEnabled(audioType, checkState, audioMode);
		}
        	
        	initSettingDefaultEnable();
		} catch (Exception e) {}
	}
	
	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPBaseActivity#onDestroy()
	 */
	@Override
	protected void onDestroy() {
		super.onDestroy();
		
		if(mSettingArray != null) {
			mSettingArray.clear();
			mSettingArray = null;
		}
		
		if(mSettingCheckArray != null) {
			mSettingCheckArray.clear();
			mSettingCheckArray = null;
		}
		
		settingAutomanage = null;
		settingEchocancelled = null;
		settingSilencerestrain = null;
		
	}

	/**
	 * 
	 * @return
	 */
	private SharedPreferences getSharedPreferences() {
		return CcpPreferences.getSharedPreferences();
	}
	
	
	void changeSubView(int id ) {
		View view = findViewById(id);
		TextView suTextView = (TextView) view.findViewById(R.id.item_sub_textView);
		suTextView.setText(mSettingArray.get(id));
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);

		Log4Util.d(CCPHelper.DEMO_TAG ,"[IMChatActivity] onActivityResult: requestCode=" + requestCode
				+ ", resultCode=" + resultCode + ", data=" + data);

		// If there's no data (because the user didn't select a file or take pic  and
		// just hit BACK, for example), there's nothing to do.
		if (resultCode != RESULT_OK) {
			Log4Util.d(CCPHelper.DEMO_TAG ,"[GroupChatActivity] onActivityResult: bail due to resultCode=" + resultCode);
			return;
		}
		int index ;
		switch (requestCode) {
		case AutoManageSettingActivity.SETTING_AUTOMANAGE:
			index = getSettingIntValue(CCPPreferenceSettings.SETTING_AUTO_MANAGE_VALUE);
			putSetingArrayValue(CCPSetting.SETTING_AUTOMANAGE_ID, settingAutomanage[index]);
			changeSubView(CCPSetting.SETTING_AUTOMANAGE_ID);
			break;
		case AutoManageSettingActivity.SETTING_ECHOCANCELLED:
			index = getSettingIntValue(CCPPreferenceSettings.SETTING_ECHOCANCEL_VALUE);
			putSetingArrayValue(CCPSetting.SETTING_ECHOCANCEL_ID, settingEchocancelled[index]);
			changeSubView(CCPSetting.SETTING_ECHOCANCEL_ID);
			break;
		case AutoManageSettingActivity.SETTING_SILENCERESTRAIN:
			index = getSettingIntValue(CCPPreferenceSettings.SETTING_SILENCERESTRAIN_VALUE);
			putSetingArrayValue(CCPSetting.SETTING_SILENCERESTRAIN_ID, settingSilencerestrain[index]);
			changeSubView(CCPSetting.SETTING_SILENCERESTRAIN_ID);
			break;

		default:
			break;
		}
	}
	
	@Override
	protected void onReceiveBroadcast(Intent intent) {
		super.onReceiveBroadcast(intent);
		if(intent.getAction().equals(CCPIntentUtils.INTENT_P2P_ENABLED)) {
			CCPApplication.getInstance().showToast(R.string.dialog_p2p_mesage);
		}
	}

	@Override
	protected int getLayoutId() {
		return R.layout.layout_setting_home_activity;
	}
}
