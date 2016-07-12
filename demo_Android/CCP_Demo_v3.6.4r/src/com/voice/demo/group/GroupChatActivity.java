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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InvalidClassException;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import android.annotation.TargetApi;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.AssetFileDescriptor;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.graphics.drawable.AnimationDrawable;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.provider.MediaStore;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.im.IMAttachedMsg;
import com.hisun.phone.core.voice.model.im.IMTextMsg;
import com.hisun.phone.core.voice.model.im.InstanceMsg;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.VoiceUtil;
import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.chatroom.XQuickActionBar;
import com.voice.demo.group.baseui.CCPChatFooter;
import com.voice.demo.group.baseui.CCPTextView;
import com.voice.demo.group.model.IMChatMessageDetail;
import com.voice.demo.group.utils.FileUtils;
import com.voice.demo.group.utils.MimeTypesTools;
import com.voice.demo.sqlite.CCPSqliteManager;
import com.voice.demo.tools.CCPAudioManager;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.tools.net.ITask;
import com.voice.demo.tools.net.TaskKey;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.CCPHelper;

/**
 * The details of interface group chat, voice chat and send files or text.
 * @version Time: 2013-7-21
 */
@TargetApi(9)
public class GroupChatActivity extends GroupBaseActivity implements View.OnClickListener 
																	/*,onVoiceMediaPlayListener*/
																	,CCPChatFooter.OnChattingLinstener{

	private static final String USER_DATA 											= "yuntongxun.com";
	public static final int CHAT_MODE_IM_POINT 										= 0x1;
	public static final int CHAT_MODE_IM_GROUP 										= 0x2;
	
	public static final int REQUEST_CODE_TAKE_PICTURE 								= 11;
	public static final int REQUEST_CODE_SELECT_FILE 								= 12;
	
	// recording of three states
	public static final int RECORD_NO 												= 0;
	public static final int RECORD_ING 												= 1;
	public static final int RECORD_ED 												= 2;

	public int mRecordState    													    = 0; 
	// the most short recording time, in milliseconds seconds, 
	// 0 for no time limit is set to 1000, suggestion
	// recording time
	// microphone gain volume value
	private static final int MIX_TIME 												= 1000;
	protected static final int CAMERA_RECORD_ACTIVITY = 13; 
	
    public static HashMap<String, Boolean> voiceMessage = new HashMap<String, Boolean>();
    
	private IMGroupChatItemAdapter mIMGroupApapter;
	private String currentRecName;
	private String mGroupId;
	private String mGroupName;
	
	private ListView mIMGroupListView;
	private TextView mNoticeTips;
	
	private CCPChatFooter mChatFooter = null;
	
	private XQuickActionBar xQuickActionBar;
	
	private boolean isRecordAndSend = false;
	private int chatModel = CHAT_MODE_IM_POINT;
	
	private long recodeTime = 0; 
	
	public int mPosition = -1;
	
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		initResourceRefs();
		
		registerReceiver(new String[]{CCPIntentUtils.INTENT_IM_RECIVE 
				,CCPIntentUtils.INTENT_REMOVE_FROM_GROUP
				,CCPIntentUtils.INTENT_DELETE_GROUP_MESSAGE});
		
		SharedPreferences ccpDemoSP = CcpPreferences.getSharedPreferences();
		isRecordAndSend = ccpDemoSP.getBoolean(CCPPreferenceSettings.SETTING_VOICE_ISCHUNKED
				.getId(), ((Boolean)CCPPreferenceSettings.SETTING_VOICE_ISCHUNKED
				.getDefaultValue()).booleanValue());		
		
		CCPHelper.getInstance().setHandler(mIMChatHandler);
		if(checkeDeviceHelper()) {
			initialize(savedInstanceState);
			String  title = null;
			String  rightButton = null;
			if(chatModel == CHAT_MODE_IM_GROUP) {
				title = mGroupName;
				//rightButton = getString(R.string.str_title_right_group_info);
				rightButton = getString(R.string.app_title_right_button_pull_down);
			} else {
				title = "TO:" + mGroupName;
				rightButton = getString(R.string.btn_clear_all_text);
			}
			handleTitleDisplay(getString(R.string.btn_title_back)
					, title
					, rightButton);
			
		}
		
		
	}
	
	private void initResourceRefs() {
		
		final View activityRootView = findViewById(R.id.im_root);
		activityRootView.getViewTreeObserver().addOnGlobalLayoutListener(new OnGlobalLayoutListener() {
			boolean  showInput = false;
		    @Override
		    public void onGlobalLayout() {
		    	
		    	Rect r = new Rect();
		    	activityRootView.getWindowVisibleDisplayFrame(r);
                int heightDiff = activityRootView.getRootView().getHeight() - (r.bottom - r.top);
                
		        //int heightDiff = activityRootView.getRootView().getHeight() - activityRootView.getHeight();
		        if (heightDiff > 100) { 
		        	showInput  = true;
		        	if(heightDiff >= 100) {
		        		
		        		// do nothing ..
		        		return;
		        	}
		        	// If the difference is more than 100 pixels, is likely to be a soft keyboard...
		            // do something here
		        	
		        	if(mIMGroupListView != null && mIMGroupApapter != null) {
		        		mIMGroupListView.setSelection(mIMGroupApapter.getCount() - 1 );
		        		
		        	}
		        	// The judge of this input method is the pop-up state, 
		        	// then set the record button is not available
		        	mChatFooter.setMode(1);
		        } else {
		        	
		        	//int heightDensity = Math.round(38 * getResources().getDisplayMetrics().densityDpi / 160.0F);
		        	if(!showInput) {
		        		return;
		        	}
		        	showInput = false;
		        	if(mChatFooter.getMode() == 1) {
		        		mChatFooter.setMode(2 , false);
		        	}
		        }
		     }
		});
		
		mNoticeTips = (TextView) findViewById(R.id.notice_tips);
		mNoticeTips.setVisibility(View.GONE);
		mIMGroupListView = (ListView) findViewById(R.id.im_chat_list);
		
		// 
		mIMGroupListView.setTranscriptMode(ListView.TRANSCRIPT_MODE_ALWAYS_SCROLL);
		mIMGroupListView.setOnTouchListener(new OnTouchListener() {
			
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				
				HideSoftKeyboard();
				
				// After the input method you can use the record button.
				//mGroudChatRecdBtn.setEnabled(true);
				mChatFooter.setMode(2 , false);
				return false;
			}
		});
		
		
		mChatFooter = (CCPChatFooter) findViewById(R.id.nav_footer);
		mChatFooter.setOnChattingLinstener(this);
		
	}

	private void initialize(Bundle savedInstanceState) {
		// Read parameters or previously saved state of this activity.
		Intent intent = getIntent();
		if (intent.hasExtra(KEY_GROUP_ID)) {
			Bundle extras = intent.getExtras();
			if (extras != null) {
				mGroupId = (String) extras.get(KEY_GROUP_ID);
				
			}
		}
		
		if (intent.hasExtra("groupName")) {
			Bundle extras = intent.getExtras();
			if (extras != null) {
				mGroupName = (String) extras.get("groupName");
				
			}
		}
		
		if(TextUtils.isEmpty(mGroupId)) {
			Toast.makeText(getApplicationContext(), R.string.toast_group_id_error, Toast.LENGTH_SHORT).show();
			finish();
			return;
		}
		
		if(TextUtils.isEmpty(mGroupName)) {
			mGroupName = mGroupId;
		}
		
		
		
		if(mGroupId.startsWith("g")) {
			chatModel = CHAT_MODE_IM_GROUP;
		} else {
			chatModel = CHAT_MODE_IM_POINT;
		}
		
		new IMListyncTask().execute(mGroupId);
	}
	
	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		int key = iTask.getKey();
		if(key == TaskKey.TASK_KEY_DEL_MESSAGE) {
			try {
				CCPSqliteManager.getInstance().deleteIMMessageBySessionId(mGroupId);
				if(mIMGroupApapter != null) {
					for (int i = 0; i < mIMGroupApapter.getCount(); i++) {
						IMChatMessageDetail item = mIMGroupApapter.getItem(i);
						if(item == null || item.getMessageType() == IMChatMessageDetail.TYPE_MSG_TEXT) {
							continue;
						}
						
						CCPUtil.delFile(item.getFilePath());
						
					}
				}
				closeConnectionProgress();
				sendbroadcast();//after dismiss progress 
			} catch (SQLException e) {
				e.printStackTrace();
			}
		}
	}
	
	@Override
	protected void handleTitleAction(int direction) {
		
		if(direction == TITLE_RIGHT_ACTION) {
			if(chatModel == CHAT_MODE_IM_GROUP) {
				if(xQuickActionBar==null){
					xQuickActionBar = new XQuickActionBar(findViewById(R.id.voice_right_btn));
					xQuickActionBar.setOnPopClickListener(popListener);
				}
				
				int switchId = isEarpiece ? R.string.pull_mode_speaker : R.string.pull_mode_earpiece;
				
				int[] arrays = new int[]{switchId , R.string.str_title_right_group_info};
				
				xQuickActionBar.setArrays(arrays);
				xQuickActionBar.show();
				
				return ;
			}
			
			showConnectionProgress(getString(R.string.str_dialog_message_default));
			ITask iTask = new ITask(TaskKey.TASK_KEY_DEL_MESSAGE);
			addTask(iTask);
			
		} else {
			super.handleTitleAction(direction);
		}
	}

	String uniqueId = null;
	
	@Override
	public void onRecordInit() {
		
		if (getRecordState() != RECORD_ING) {
			setRecordState(RECORD_ING);
			
			// release all playing voice file
			releaseVoiceAnim(-1);
			readyOperation();
			
			mChatFooter.showVoiceDialog(findViewById(R.id.im_root).getHeight() - mChatFooter.getHeight());
			
			// True audio data recorded immediately transmitted to the server
			// False just recording audio data, then send audio file after the completion of recording..
			// isRecordAndSend = true; 
			new Thread(new Runnable() {
				
				@Override
				public void run() {
					currentRecName =   + System.currentTimeMillis() + ".amr";
					File directory = getCurrentVoicePath();
					if(checkeDeviceHelper()) {
						// If it is sent non't in chunked mode, only second parameters
						try {
							uniqueId = getDeviceHelper().startVoiceRecording(mGroupId, directory.getAbsolutePath(), isRecordAndSend , USER_DATA);
							voiceMessage.put(uniqueId, true);
						} catch (Exception e) {
							e.printStackTrace();
						}
					}
				}
			}).start();
			
		}
	}
	
	@Override
	public void onRecordStart() {
		
		// If you are in when being loaded, then send to start recording
		mIMChatHandler.removeMessages(WHAT_ON_COMPUTATION_TIME);
		mIMChatHandler.sendEmptyMessageDelayed(WHAT_ON_COMPUTATION_TIME, GroupChatActivity.TONE_LENGTH_MS);
	}

	@Override
	public void onRecordCancle() {
		handleMotionEventActionUp(true);
	}

	@Override
	public void onRecordOver() {
		handleMotionEventActionUp(false);
	}
	
	
	@Override
	public void onSendTextMesage(String text){

		if (TextUtils.isEmpty(text)) {
			return;
		}
		
		
		
		IMChatMessageDetail chatMessageDetail = IMChatMessageDetail.getGroupItemMessage(IMChatMessageDetail.TYPE_MSG_TEXT 
				, IMChatMessageDetail.STATE_IM_SENDING , mGroupId);
		chatMessageDetail.setMessageContent(text);
		
		if(!checkeDeviceHelper()) {
			return;
		}
		try {
			String uniqueID = getDeviceHelper().sendInstanceMessage(mGroupId, text.toString(), null, null);
			if(TextUtils.isEmpty(uniqueID)) {
				CCPApplication.getInstance().showToast(R.string.toast_send_group_message_failed);
				chatMessageDetail.setImState(IMChatMessageDetail.STATE_IM_SEND_FAILED);
				return ;
			}
			chatMessageDetail.setMessageId(uniqueID);
			chatMessageDetail.setUserData(USER_DATA);
			CCPSqliteManager.getInstance().insertIMMessage(chatMessageDetail);
			sendbroadcast();
		} catch (SQLException e) {
			e.printStackTrace();
		}
		
		text = null;
	}
	
	@Override
	public void onSelectVideo() {
		new AlertDialog.Builder(this)
		.setItems(R.array.chatvideo_select_item, new DialogInterface.OnClickListener() {
               public void onClick(DialogInterface dialog, int which) {
            	   if(!Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
            		   Toast.makeText(getApplicationContext(), R.string.sdcard_not_file_trans_disable, Toast.LENGTH_SHORT).show();
            		   return;
            	   }
            	   
					if(which == 0){// take videoRecored 
						Intent intent = new Intent(GroupChatActivity.this, VideoRecordActivity.class) ;
						startActivityForResult(intent , REQUEST_CODE_SELECT_FILE);
//						Intent mIntent = new Intent(MediaStore.ACTION_VIDEO_CAPTURE);
//						mIntent.putExtra(MediaStore.EXTRA_DURATION_LIMIT, 30);//这个值以秒为单位，显示视频采集的时长
//						startActivityForResult(mIntent, CAMERA_RECORD_ACTIVITY);//CAMERA_ACTIVITY = 1
						
					}else if (which == 1){//go to photo album 
						Intent intent = new Intent(GroupChatActivity.this, FileBrowserActivity.class) ;
						//intent.putExtra("to", recipient);
						startActivityForResult(intent , REQUEST_CODE_SELECT_FILE);
					}
               }
        }).setTitle(R.string.dialog_list_item_title)
		.create().show();
	}

	@Override
	public void onSelectFile() {
		if(!CCPUtil.isExistExternalStore()) {
			Toast.makeText(getApplicationContext(), "SD card is un_mounted ", Toast.LENGTH_SHORT).show();
			return;
		}
		new AlertDialog.Builder(this)
		.setItems(R.array.chat_select_item, new DialogInterface.OnClickListener() {
               public void onClick(DialogInterface dialog, int which) {
           		// The 'which' argument contains the index position
					// of the selected item
					if(which == 0){// take pic 
						takePicture();
					}else if (which == 1){//save
						if(!Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
							Toast.makeText(getApplicationContext(), R.string.sdcard_not_file_trans_disable, Toast.LENGTH_SHORT).show();
							return;
						}
						Intent intent = new Intent(GroupChatActivity.this, FileBrowserActivity.class) ;
						//intent.putExtra("to", recipient);
						startActivityForResult(intent , REQUEST_CODE_SELECT_FILE);
					}
               }
        }).setTitle(R.string.dialog_list_item_title)
		.create().show();
	}
	
	
	/**
	 * @version 3.5
	 * <p>Title: handleMotionEventActionUp</p>
	 * <p>Description: The current activity is not visible, if you are recording, 
	 *  stop recording and performing transmission or cancel the operation
	 *  {@link GroupChatActivity#mRecordState   }
	 *  {@link GroupChatActivity#RECORD_ED}
	 *  {@link GroupChatActivity#RECORD_ING}
	 *  {@link GroupChatActivity#RECORD_NO}</p>
	 *  
	 *  @see Device#startVoiceRecording(String, String, boolean, String);
	 *  @see Device#cancelVoiceRecording();
	 *  @see Device#stopVoiceRecording();
	 */
	private void handleMotionEventActionUp(boolean doCancle) {
		
		if(getRecordState()  == RECORD_ING) {
			
			if(checkeDeviceHelper()) {
				if(doCancle) {
					getDeviceHelper().cancelVoiceRecording();
				} else {
					getDeviceHelper().stopVoiceRecording();
				}
			}
			doProcesOperationRecordOver(doCancle);
			Log4Util.d(CCPHelper.DEMO_TAG, "handleMotionEventActionUp");
		}
	}

	private static final int REQUEST_KEY_RESEND_IMMESSAGE = 0X1;
	private static final int WHAT_ON_COMPUTATION_TIME = 10000;
	private long computationTime = -1L;
	private Toast mRecordTipsToast;
	private void readyOperation() {
		computationTime = -1L;
		mRecordTipsToast = null;
		playTone(ToneGenerator.TONE_PROP_BEEP, TONE_LENGTH_MS);
		new Handler().postDelayed(new Runnable() {
			
			@Override
			public void run() {
				stopTone();
			}
		}, TONE_LENGTH_MS);
		vibrate(50L);
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
			
			// version 3.5
			// re send message where failed message 
		case R.id.error_Icon:
			
			try {
				Integer position = (Integer) v.getTag();
				if(mIMGroupApapter != null && mIMGroupApapter.getItem(position) != null) {
					mPosition = position;
					showAlertTipsDialog(REQUEST_KEY_RESEND_IMMESSAGE, getString(R.string.str_chatting_resend_title)
							, getString(R.string.str_chatting_resend_content)
							, getString(R.string.dialog_btn)
							, getString(R.string.dialog_cancle_btn));
				}
			} catch (Exception e) {
				e.printStackTrace();
			}
			
			break;
		default:
			break;
		}
	}
	
	@Override
	protected void handleDialogOkEvent(int requestKey) {
		
		if(requestKey == REQUEST_KEY_RESEND_IMMESSAGE) {
			if(mPosition == -1) {
				return;
			}
			reSendImMessage(mPosition);
		} else {
			
			super.handleDialogOkEvent(requestKey);
		}
		
		
	}
	
	
	@Override
	protected void handleDialogCancelEvent(int requestKey) {
		if(requestKey == REQUEST_KEY_RESEND_IMMESSAGE) {
			mPosition = -1;
		} else {
			super.handleDialogCancelEvent(requestKey);
		}
	}
	
	/**
	 * 
	* <p>Title: reSendImMessage</p>
	* <p>Description: </p>
	* @param postion
	 */
	public void reSendImMessage(int position) {
		
		if(mIMGroupApapter != null && mIMGroupApapter.getItem(position) != null) {
			IMChatMessageDetail item = mIMGroupApapter.getItem(position);
			
			try {
				String uniqueID = null;
				if(checkeDeviceHelper()) {
					if(item.getMessageType() == IMChatMessageDetail.TYPE_MSG_TEXT) {
						uniqueID = getDeviceHelper().sendInstanceMessage(mGroupId, item.getMessageContent(), null, USER_DATA);
					} else {
						uniqueID = getDeviceHelper().sendInstanceMessage(mGroupId, null, item.getFilePath(), USER_DATA);
					}
				}
				if(TextUtils.isEmpty(uniqueID)) {
					CCPApplication.getInstance().showToast(R.string.toast_send_group_message_failed);
					item.setImState(IMChatMessageDetail.STATE_IM_SEND_FAILED);
					return ;
				}
				CCPSqliteManager.getInstance().deleteIMMessageByMessageId(item.getMessageId());
				item.setMessageId(uniqueID);
				item.setImState(IMChatMessageDetail.STATE_IM_SENDING);
				CCPSqliteManager.getInstance().insertIMMessage(item);

				mIMGroupApapter.notifyDataSetChanged();
			} catch (SQLException e) {
				e.printStackTrace();
			}
			
		}
	}
	
	
	
	
	private boolean isEarpiece = true;
	@Override
	protected void onResume() {
		super.onResume();
		
		updateReadStatus();
		
		// default speaker
		isEarpiece = CcpPreferences.getSharedPreferences().getBoolean(
				CCPPreferenceSettings.SETTING_HANDSET.getId(),
				((Boolean) CCPPreferenceSettings.SETTING_HANDSET
						.getDefaultValue()).booleanValue());
		
	}
	
	
	/**
	 * @author Jorstin Chan
	 * @version 3.4.1.1
	 */
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		Log4Util.d(CCPHelper.DEMO_TAG , "isEarpiece :" + isEarpiece );
		AudioManager mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
		int streamType;
		if(!isEarpiece) {
			streamType = AudioManager.STREAM_MUSIC;
		} else {
			streamType = AudioManager.STREAM_VOICE_CALL;
		}
		
		int maxVolumeVoiceCall= mAudioManager.getStreamMaxVolume(streamType);
		int index = maxVolumeVoiceCall / 7 ;
		if(index == 0) {
			index = 1;
		}
		int currentVolume = mAudioManager.getStreamVolume(streamType);;
		if(keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
			
			mAudioManager.setStreamVolume(streamType, currentVolume  - index, AudioManager.FLAG_SHOW_UI |AudioManager.FLAG_PLAY_SOUND);
			return true;
		} else if (keyCode == KeyEvent.KEYCODE_VOLUME_UP){
			
			mAudioManager.setStreamVolume(streamType, currentVolume  + index, AudioManager.FLAG_SHOW_UI |AudioManager.FLAG_PLAY_SOUND);
			return true;
		} else {
			
			// If a recording tool button to display, the display area of the recording instrument
			// this BUG for 3.4.1.1 before
			if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {
				Log4Util.d(CCPHelper.DEMO_TAG, "keycode back , chatfooter mode: " +  mChatFooter.getMode());
				if(mChatFooter.getMode() != 2) {
					mChatFooter.setMode(2 , false);
					return true;
				}
			}
			
			return super.onKeyDown(keyCode, event);
		}
		
	}

	private void updateReadStatus() {
		try {
			CCPSqliteManager.getInstance().updateIMMessageUnreadStatusToReadBySessionId(mGroupId,IMChatMessageDetail.STATE_READED);
		} catch (SQLException e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * @version 3.5
	 */
	@Override
	protected void onDestroy() {
		super.onDestroy();
		try {
			// release voice record resource.
			if(mChatFooter != null ) {
				handleMotionEventActionUp(mChatFooter.isVoiceRecordCancle());
			}
			
			// release voice play resources 
			releaseVoiceAnim(-1);
			
		} catch (Exception e) {
			e.printStackTrace();
		}
		
		if(mChatFooter != null ) {
			mChatFooter.onDistory();
			mChatFooter = null;
		}
		mIMGroupApapter = null ;
		
		if(mIMChatHandler != null ) {
			mIMChatHandler = null;
		}
		
	}
	
	@Override
	protected void onPause() {
		super.onPause();
		
		HideSoftKeyboard();
		// release voice record resource.
		if(mChatFooter != null ) {
			handleMotionEventActionUp(mChatFooter.isVoiceRecordCancle());
		}
		
		// release voice play resources 
		releaseVoiceAnim(-1);
	}
	
	String fileName;
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);

		Log4Util.d(CCPHelper.DEMO_TAG ,"[IMChatActivity] onActivityResult: requestCode=" + requestCode
				+ ", resultCode=" + resultCode + ", data=" + data);

		// If there's no data (because the user didn't select a file or take pic  and
		// just hit BACK, for example), there's nothing to do.
		if (requestCode != REQUEST_CODE_TAKE_PICTURE || requestCode == REQUEST_CODE_SELECT_FILE 
			) {
			if (data == null) {
				return;
			}
		} else if (resultCode != RESULT_OK) {
			Log4Util.d(CCPHelper.DEMO_TAG ,"[GroupChatActivity] onActivityResult: bail due to resultCode=" + resultCode);
			return;
		}
		
		String fileName = null ;
		String filePath = null;
		switch (requestCode) {
		case CAMERA_RECORD_ACTIVITY: {
			if (resultCode != RESULT_OK)
				return;
			
			try {
				AssetFileDescriptor videoAsset = getContentResolver()
						.openAssetFileDescriptor(data.getData(), "r");
				FileInputStream fis = videoAsset.createInputStream();
				File tmpFile = CCPUtil.TackVideoFilePath();
				FileOutputStream fos = new FileOutputStream(tmpFile);
				byte[] buf = new byte[1024];
				int len;
				while ((len = fis.read(buf)) > 0) {
					fos.write(buf, 0, len);
				}
				fis.close();
				fos.close();
				fileName =tmpFile.getName();
				filePath = tmpFile.getAbsolutePath();
				
			} catch (IOException io_e) {
			}
			break;
		}
			case REQUEST_CODE_TAKE_PICTURE: {
				File file = takepicfile;
				if(file == null || !file.exists()) {
					return;
				}
				//Uri uri = Uri.fromFile(file);
				//addImage(uri, false);
				
				filePath = file.getAbsolutePath();
				// do send pic ...
				break;
			}
			
			case REQUEST_CODE_SELECT_FILE: {
				
				if(data.hasExtra("file_name")) {
					Bundle extras = data.getExtras();
					if (extras != null) {
						fileName = extras.getString("file_name");
					}
				}
				
				if(data.hasExtra("file_url")) {
					Bundle extras = data.getExtras();
					if (extras != null) {
						filePath = extras.getString("file_url");
					}
				}
				
				break;
			}
		}
		
		if(TextUtils.isEmpty(filePath)) {
			// Select the local file does not exist or has been deleted.
			Toast.makeText(GroupChatActivity.this, R.string.toast_file_exist, Toast.LENGTH_SHORT).show();
			return ;
		}
		
		
		
		if(TextUtils.isEmpty(fileName)) {
			fileName = new File(filePath).getName();
			//fileName = filePath.substring(filePath.indexOf("/"), filePath.length());
		}
		
		IMChatMessageDetail chatMessageDetail = IMChatMessageDetail.getGroupItemMessage(IMChatMessageDetail.TYPE_MSG_FILE 
				, IMChatMessageDetail.STATE_IM_SENDING , mGroupId) ;
		chatMessageDetail.setMessageContent(fileName);
		chatMessageDetail.setFilePath(filePath);
		String extensionName = VoiceUtil.getExtensionName(fileName);
		if("amr".equals(extensionName)) {
			chatMessageDetail.setMessageType(IMChatMessageDetail.TYPE_MSG_VOICE);
		}
		chatMessageDetail.setFileExt(extensionName);
		
		if(!checkeDeviceHelper()) {
			return;
		}
		
		try {
			String uniqueID = getDeviceHelper().sendInstanceMessage(mGroupId, null, filePath, USER_DATA);
			chatMessageDetail.setMessageId(uniqueID);
			
			CCPSqliteManager.getInstance().insertIMMessage(chatMessageDetail);
			chatMessageDetail.setUserData(USER_DATA);
			notifyGroupDateChange(chatMessageDetail);
			
		} catch (SQLException e) {
			e.printStackTrace();
		}
	}

	private void notifyGroupDateChange(IMChatMessageDetail chatMessageDetail) {
		if(mIMGroupApapter == null ) {
			ArrayList<IMChatMessageDetail> iChatMsg = new ArrayList<IMChatMessageDetail>();
			iChatMsg.add(chatMessageDetail);
			mIMGroupApapter = new IMGroupChatItemAdapter(iChatMsg);
			mIMGroupListView.setAdapter(mIMGroupApapter);
		} else {
			mIMGroupApapter.insert(chatMessageDetail, mIMGroupApapter.getCount());
		}
		
		mIMGroupListView.setSelection(mIMGroupListView.getCount());
	}
	
	
	private void sendbroadcast() {
		Intent intent = new Intent(CCPIntentUtils.INTENT_IM_RECIVE);
		intent.putExtra(KEY_GROUP_ID, mGroupId);
		sendBroadcast(intent);
	}
	
	class IMGroupChatItemAdapter extends ArrayAdapter<IMChatMessageDetail> {

		LayoutInflater mInflater;
		public IMGroupChatItemAdapter(List<IMChatMessageDetail> iChatMsg) {
			super(GroupChatActivity.this, 0, iChatMsg);
			
			mInflater = getLayoutInflater();
		}

		@Override
		public View getView(final int position, View convertView, ViewGroup parent) {
			
			final GroupMsgHolder holder;
			if (convertView == null|| convertView.getTag() == null) {
				convertView = mInflater.inflate(R.layout.list_item_voice_mseeage, null);
				holder = new GroupMsgHolder();
				convertView.setTag(holder);
				
				holder.lavatar = (ImageView) convertView.findViewById(R.id.voice_chat_avatar_l);
				holder.ravatar = (ImageView) convertView.findViewById(R.id.voice_chat_avatar_r);
				holder.gLayoutLeft = (LinearLayout) convertView.findViewById(R.id.voice_item_left);
				holder.gLayoutRight = (LinearLayout) convertView.findViewById(R.id.voice_item_right);
				
				
				holder.gTime = (TextView) convertView.findViewById(R.id.voice_chat_time);
				
				holder.gNameleft = (TextView) convertView.findViewById(R.id.name_l);
				holder.gNameRight = (TextView) convertView.findViewById(R.id.name_r);
				
				holder.gVoiceChatLyLeft = (LinearLayout) convertView.findViewById(R.id.voice_chat_ly_l);
				holder.gIMChatLyLeft = (LinearLayout) convertView.findViewById(R.id.im_chat_ly);
				holder.gIMChatLyLeft_videopreview_fl =  convertView.findViewById(R.id.fl_imageview);
				holder.gIMChatLyLeft_videopreview = (ImageView) convertView.findViewById(R.id.imageview);
				holder.gIMChatLyLeft_btn_play = (Button) convertView.findViewById(R.id.btn_play);
				
				holder.gVoiceChatLyRight = (LinearLayout) convertView.findViewById(R.id.voice_chat_ly_r);
				holder.gIMChatLyRight = (LinearLayout) convertView.findViewById(R.id.im_chat_ly_r);
				holder.gIMChatLyRight_videopreview_fl = convertView.findViewById(R.id.fl_imageview_right);
				holder.gIMChatLyRight_videopreview = (ImageView) convertView.findViewById(R.id.imageview_right);
				holder.gIMChatLyRight_btn_play = (Button) convertView.findViewById(R.id.btn_play_right);
				
				
				
				holder.imFileIconL = (ImageView) convertView.findViewById(R.id.im_chatting_file_icon_l);
				holder.imFileIconR = (ImageView) convertView.findViewById(R.id.im_chatting_file_icon);
				
				holder.imFileNameLeft = (CCPTextView) convertView.findViewById(R.id.file_name_left);
				holder.imFileNameRight = (CCPTextView) convertView.findViewById(R.id.file_name_right);
				
				holder.imTimeLeft = (TextView) convertView.findViewById(R.id.im_chat_time_left);
				holder.imTimeRight = (TextView) convertView.findViewById(R.id.im_chat_time_right);
				
				holder.rProBar = (ProgressBar) convertView.findViewById(R.id.voice_sending_r);
				
				
				// voice item  time
				holder.lDuration = (TextView) convertView.findViewById(R.id.voice_content_len_l);
				holder.rDuration = (TextView) convertView.findViewById(R.id.voice_content_len_r);
				
				// vioce chat animation
				holder.vChatContentFrom = (ImageView) convertView.findViewById(R.id.voice_chat_recd_tv_l);
				holder.vChatContentTo = (ImageView) convertView.findViewById(R.id.voice_chat_recd_tv_r);
				
				
				holder.vErrorIcon = (ImageView) convertView.findViewById(R.id.error_Icon);
				holder.vErrorIcon.setOnClickListener(GroupChatActivity.this);
			} else {
				holder = (GroupMsgHolder) convertView.getTag();
			}
			
			final IMChatMessageDetail item = getItem(position);
			if(item != null ) {
				View.OnClickListener onClickListener = new OnClickListener() {
					
					@Override
					public void onClick(View v) {
						Intent intent = new Intent(GroupChatActivity.this,
								GroupMemberCardActivity.class);
						intent.putExtra(KEY_GROUP_ID, mGroupId);
						intent.putExtra("voipAccount", item.getGroupSender());
						intent.putExtra("modify", false);
						startActivity(intent);
						
					}
				};
				if(item.getImState() == IMChatMessageDetail.STATE_IM_RECEIVEED) {
					holder.gLayoutLeft.setVisibility(View.VISIBLE);
					holder.gLayoutRight.setVisibility(View.GONE);
					String groupSender = item.getGroupSender();
					if(!TextUtils.isEmpty(groupSender) && groupSender.length() > 4) {
						groupSender = groupSender.substring(groupSender.length() - 4, groupSender.length());
					}
					if(chatModel == CHAT_MODE_IM_GROUP){
						holder.lavatar.setOnClickListener(onClickListener);
					}
					
					holder.gNameleft.setText(groupSender);
					
					if(item.getMessageType() == IMChatMessageDetail.TYPE_MSG_VOICE) { //voice chat ...itme
						
						// If the speech information, you need to display the voice information 
						// distribution, and voice information unified time display in the middle 
						// position above the voice information
						// And hidden files IM layout
						holder.gVoiceChatLyLeft.setVisibility(View.VISIBLE);
						holder.gIMChatLyLeft.setVisibility(View.GONE);
						holder.gTime.setVisibility(View.VISIBLE);
						
						int duration = 0;
						if(checkeDeviceHelper()) {
							duration = (int) Math.ceil(getDeviceHelper().getVoiceDuration(item.getFilePath())/1000) ;
						}
						duration = duration == 0 ? 1 :duration;
						holder.lDuration.setText(duration + "''");
						
						holder.gVoiceChatLyLeft.setOnClickListener(new OnClickListener() {
							
							@Override
							public void onClick(View v) {
								// It shows only in the presence of the voice files
								if(!TextUtils.isEmpty(item.getFilePath()) && new File(item.getFilePath()).exists()) {
									ViewPlayAnim(holder.vChatContentFrom ,item.getFilePath() , position);
									//CCPVoiceMediaPlayManager.getInstance(GroupChatActivity.this).putVoicePlayQueue(position, item.getFilePath());
								} else {
									Toast.makeText(getApplicationContext(), R.string.media_ejected, Toast.LENGTH_LONG).show();
								}
							}
							
						});
						
					} else {
						// TEXT FILE
						// If not the voice information, you need to display the IM file layout, 
						// and the layout of item audio information hiding, also need the voice 
						// information time display view hide this time, only need to display 
						// the time view IM style
						holder.gVoiceChatLyLeft.setVisibility(View.GONE);
						holder.gIMChatLyLeft.setVisibility(View.VISIBLE);
						holder.gTime.setVisibility(View.GONE);
						if(item.getMessageType() == IMChatMessageDetail.TYPE_MSG_TEXT) {
							holder.imFileNameLeft.setEmojiText(item.getMessageContent());
							holder.imFileIconL.setVisibility(View.GONE);
							
						} else if (item.getMessageType() == IMChatMessageDetail.TYPE_MSG_FILE) {
							holder.imFileIconL.setVisibility(View.VISIBLE);
							
							OnClickListener onClickListener2 = new OnClickListener() {
								
								@Override
								public void onClick(View v) {
									//String vLocalPath =  new File(VoiceApplication.getInstance().getVoiceStore() , item.getMessageContent()).getAbsolutePath();
									snedFilePrevieIntent(item.getFilePath());
								}
							};
							holder.gIMChatLyLeft.setOnClickListener(onClickListener2);
							holder.gIMChatLyLeft_videopreview.setOnClickListener(onClickListener2);
							holder.gIMChatLyLeft_btn_play.setOnClickListener(onClickListener2);
							
							//file name
							holder.imFileNameLeft.setEmojiText(item.getMessageContent());
							
						}
						
						holder.imTimeLeft.setText(item.getCurDate());
						
						//是视频的话  加 预览图片
						if("mp4".equals(item.getFileExt())){
							Bitmap createVideoThumbnail = FileUtils.createVideoThumbnail(item.getFilePath());
							if(createVideoThumbnail!=null){
								holder.gIMChatLyLeft_videopreview_fl.setVisibility(View.VISIBLE);
								holder.gIMChatLyLeft_videopreview.setImageBitmap(createVideoThumbnail);
							}
						}else{
							holder.gIMChatLyLeft_videopreview_fl.setVisibility(View.GONE);
						}
					}
				} else {
					if(chatModel == CHAT_MODE_IM_GROUP){
						holder.ravatar.setOnClickListener(onClickListener);
					}
					holder.gLayoutLeft.setVisibility(View.GONE);
					holder.gLayoutRight.setVisibility(View.VISIBLE);
					holder.gNameRight.setText(CCPConfig.VoIP_ID.substring(CCPConfig.VoIP_ID.length() - 4, CCPConfig.VoIP_ID.length()));
					
					if(item.getMessageType() == IMChatMessageDetail.TYPE_MSG_VOICE) { 
						
						//voice chat ...itme
						holder.gVoiceChatLyRight.setVisibility(View.VISIBLE);
						holder.gIMChatLyRight.setVisibility(View.GONE);
						holder.gTime.setVisibility(View.VISIBLE);
						
						int duration = 0;
						if(checkeDeviceHelper()) {
							duration = (int) Math.ceil(getDeviceHelper().getVoiceDuration(item.getFilePath())/1000) ;
							
						}
						duration = duration == 0 ? 1 :duration;
						holder.rDuration.setText(duration + "''");
						
						holder.gVoiceChatLyRight.setOnClickListener(new OnClickListener() {
							
							@Override
							public void onClick(View v) {
								// It shows only in the presence of the voice files
								if(!TextUtils.isEmpty(item.getFilePath()) && new File(item.getFilePath()).exists()) {
									ViewPlayAnim(holder.vChatContentTo ,item.getFilePath() , position);
									//CCPVoiceMediaPlayManager.getInstance(GroupChatActivity.this).putVoicePlayQueue(position, item.getFilePath());
								} else {
									Toast.makeText(getApplicationContext(), R.string.media_ejected, Toast.LENGTH_LONG).show();
								}
								
							}
						});
						
					} else {
						// TEXT FILE
						holder.gVoiceChatLyRight.setVisibility(View.GONE);
						holder.gIMChatLyRight.setVisibility(View.VISIBLE);
						holder.gTime.setVisibility(View.GONE);
						
						
						if(item.getMessageType() == IMChatMessageDetail.TYPE_MSG_TEXT) {
							holder.imFileNameRight.setEmojiText(item.getMessageContent());
							holder.imFileIconR.setVisibility(View.GONE);
							
							// If it is sent the text is not realistic loading ...
							holder.rProBar.setVisibility(View.GONE);
							
						} else if (item.getMessageType() == IMChatMessageDetail.TYPE_MSG_FILE) {
							holder.imFileIconR.setVisibility(View.VISIBLE);
							//file name
							holder.imFileNameRight.setEmojiText(item.getMessageContent());
							OnClickListener onClickListener2 = new View.OnClickListener() {
								
								@Override
								public void onClick(View v) {
									snedFilePrevieIntent(item.getFilePath());
								}
							};
							holder.gIMChatLyRight.setOnClickListener(onClickListener2);
							holder.gIMChatLyRight_btn_play.setOnClickListener(onClickListener2);
							holder.gIMChatLyRight_videopreview.setOnClickListener(onClickListener2);
							
						}
						
						holder.imTimeRight.setText(item.getCurDate());
					}
					
					// is sending ?
					if(item.getImState() == IMChatMessageDetail.STATE_IM_SENDING) {
						holder.rProBar.setVisibility(View.VISIBLE);
						holder.vErrorIcon.setVisibility(View.GONE);
					} else if (item.getImState() == IMChatMessageDetail.STATE_IM_SEND_SUCCESS) {
						holder.rProBar.setVisibility(View.GONE);
						holder.vErrorIcon.setVisibility(View.GONE);
					} else if (item.getImState() == IMChatMessageDetail.STATE_IM_SEND_FAILED) {
						holder.vErrorIcon.setVisibility(View.VISIBLE);
						holder.rProBar.setVisibility(View.GONE);
						
						// version 3.5 
						holder.vErrorIcon.setTag(position);
					}
					
					//是视频的话  加 预览图片 
					if("mp4".equals(item.getFileExt())){
						Bitmap createVideoThumbnail = FileUtils.createVideoThumbnail(item.getFilePath());
						if(createVideoThumbnail!=null){
							holder.gIMChatLyRight_videopreview_fl.setVisibility(View.VISIBLE);
							holder.gIMChatLyRight_videopreview.setImageBitmap(createVideoThumbnail);
						}
					}else{
						holder.gIMChatLyRight_videopreview_fl.setVisibility(View.GONE);
					}
				}
				
				holder.gTime.setText(item.getCurDate());
				
			}
			
			
			return convertView;
		}
		
		class GroupMsgHolder {
			ImageView lavatar;
			ImageView ravatar;
			// root layout
			LinearLayout gLayoutLeft;
			LinearLayout gLayoutRight;
			
			TextView gTime;
			
			TextView gNameleft;
			TextView gNameRight;
			
			LinearLayout gVoiceChatLyLeft; 
			LinearLayout gIMChatLyLeft;   
			View gIMChatLyLeft_videopreview_fl;   
			ImageView gIMChatLyLeft_videopreview;   
			Button gIMChatLyLeft_btn_play;
			
			LinearLayout gVoiceChatLyRight;
			LinearLayout gIMChatLyRight;   
			View gIMChatLyRight_videopreview_fl;   
			ImageView gIMChatLyRight_videopreview;   
			Button gIMChatLyRight_btn_play;   
			
			ImageView imFileIconL;       // 	IM FILE
			ImageView imFileIconR;
			CCPTextView imFileNameLeft;
			CCPTextView imFileNameRight;
			TextView imTimeLeft;
			TextView imTimeRight;
			
			ProgressBar rProBar;
			
			// voice time 
			TextView lDuration;
			TextView rDuration;
			
			ImageView vChatContentFrom;
			ImageView vChatContentTo;
			
			
			ImageView vErrorIcon;
		}
		
		
		
		
	}

	class IMListyncTask extends AsyncTask<String , Void, ArrayList<IMChatMessageDetail>>{

		boolean isReceiveNewMessage = false;
		@Override
		protected ArrayList<IMChatMessageDetail> doInBackground(String... params) {
			if(params != null && params.length > 0) {
				
				try {
					if(params.length > 1) {
						// new Message .
						ArrayList<IMChatMessageDetail> newImMessages = CCPSqliteManager.getInstance().queryNewIMMessagesBySessionId(params[0]);
						CCPSqliteManager.getInstance().updateIMMessageUnreadStatusToRead(newImMessages,IMChatMessageDetail.STATE_READED);
						isReceiveNewMessage = true;
						
						return newImMessages;
					}
					
					updateReadStatus();
					return CCPSqliteManager.getInstance().queryIMMessagesBySessionId(params[0]);
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
			return null;
		}

		@Override
		protected void onPostExecute(ArrayList<IMChatMessageDetail> result) {
			super.onPostExecute(result);
			
			if(result != null && !result.isEmpty()) {
				
				// add for versoin 3.5 when receive new message
				if(isReceiveNewMessage && mIMGroupApapter != null) {
					for(IMChatMessageDetail imCMessageDetail : result) {
						mIMGroupApapter.insert(imCMessageDetail, mIMGroupApapter.getCount());
					}
					return;
				}
				
				mIMGroupApapter = new IMGroupChatItemAdapter(result);
				mIMGroupListView.setAdapter(mIMGroupApapter);
			} else {
				if(result==null)
				mIMGroupListView.setAdapter(null);
			}
		}
	}
	
	private XQuickActionBar.OnPopClickListener popListener = new XQuickActionBar.OnPopClickListener() {

		@Override
		public void onPopClick(int index) {
			switch (index) {
			case R.string.str_title_right_group_info:
				
				if(!TextUtils.isEmpty(mGroupId)) {
					
					Intent intent = new Intent(GroupChatActivity.this, GroupDetailActivity.class);
					intent.putExtra(KEY_GROUP_ID, mGroupId);
					intent.putExtra("isJoin", true);
					startActivity(intent);
					
				} else {
					
					// failed ..
					Toast.makeText(getApplicationContext(), R.string.toast_click_into_group_error, Toast.LENGTH_SHORT).show();
				}
				
				break;
			case R.string.pull_mode_earpiece:
			case R.string.pull_mode_speaker:
				try {
					
					CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_HANDSET, !isEarpiece, true);
					isEarpiece = CcpPreferences.getSharedPreferences().getBoolean(
							CCPPreferenceSettings.SETTING_HANDSET.getId(),
							((Boolean) CCPPreferenceSettings.SETTING_HANDSET
									.getDefaultValue()).booleanValue());
					int text = isEarpiece ? R.string.fmt_route_phone:R.string.fmt_route_speaker;
					addNotificatoinToView(getString(text) , Gravity.TOP);
				} catch (InvalidClassException e) {
					e.printStackTrace();
				}
				
				break;
				
			default:
					
				break;
			}
			xQuickActionBar.dismissBar();
		}
	};
	
	/**
	 * @param fileName
	 */
	void snedFilePrevieIntent(String fileName) { 
		String type = "";
		try {
			Intent intent = new Intent();
			intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			intent.setAction(android.content.Intent.ACTION_VIEW);
			type = MimeTypesTools.getMimeType(getApplicationContext(), fileName);
			File file = new File(fileName);
			intent.setDataAndType(Uri.fromFile(file), type);
			startActivity(intent);
		} catch (Exception e) {
			System.out.println("android.content.ActivityNotFoundException: No Activity found to handle Intent { act=android.intent.action.VIEW dat=file:///mnt/sdcard/xxx typ="
							+ type + " flg=0x10000000");
		}
	 } 

	@Override
	protected void onReceiveBroadcast(Intent intent) {
		super.onReceiveBroadcast(intent);
		if(intent == null ) {
			return;
		}
		
		if (CCPIntentUtils.INTENT_IM_RECIVE.equals(intent.getAction())
				|| CCPIntentUtils.INTENT_DELETE_GROUP_MESSAGE.equals(intent.getAction())) {
			//update UI...
			//new IMListyncTask().execute();
			if(intent.hasExtra(KEY_GROUP_ID)) {
				String sender = intent.getStringExtra(KEY_GROUP_ID);
				
				// receive new message ,then load this message set adapter of listView.
				String newMessageId = null;
				if(intent.hasExtra(KEY_MESSAGE_ID)) {
					newMessageId = intent.getStringExtra(KEY_MESSAGE_ID);
				}
				
				
				
				
				if(!TextUtils.isEmpty(sender) && sender.equals(mGroupId)) {
					if(TextUtils.isEmpty(newMessageId)) {
						new IMListyncTask().execute(mGroupId);
					} else {
						new IMListyncTask().execute(mGroupId , newMessageId);
					}
				}
			}
			
		}else if (CCPIntentUtils.INTENT_REMOVE_FROM_GROUP.equals(intent.getAction())) {
			// remove from group ...
			this.finish();
		}
	}

	
	private android.os.Handler mIMChatHandler = new android.os.Handler() {

		@Override
		public void handleMessage(Message msg) {
			super.handleMessage(msg);
			Bundle b = null;
			int reason = -1;
			if (msg.obj instanceof Bundle) {
				b = (Bundle) msg.obj;
			}
			
			switch (msg.what) {
			case CCPHelper.WHAT_ON_SEND_MEDIAMSG_RES:
				if(b == null ){
					return ;
				}
				// receive a new IM message
				// then shown in the list.
				try {
					reason = b.getInt(Device.REASON);
					InstanceMsg instancemsg = (InstanceMsg)b.getSerializable(Device.MEDIA_MESSAGE);
					if(instancemsg == null ) {
						return ;
					}
					
					//IMChatMessageDetail chatMessageDetail = null;
					int sendType = IMChatMessageDetail.STATE_IM_SEND_FAILED;
					String messageId = null;
					if(instancemsg instanceof IMAttachedMsg) {
						IMAttachedMsg rMediaInfo = (IMAttachedMsg)instancemsg;
						messageId = rMediaInfo.getMsgId();
						if (reason == 0 ) {

							sendType = IMChatMessageDetail.STATE_IM_SEND_SUCCESS;
							
							try {
								CCPUtil.playNotifycationMusic(getApplicationContext(), "voice_message_sent.mp3");
							} catch (IOException e) {
								e.printStackTrace();
							}
						} else {
							if(reason == 230007 && mChatFooter != null && mChatFooter.isVoiceRecordCancle()) {
								mChatFooter.setCancle(false);
								// Here need to determine whether is it right? You cancel this recording, 
								// and callback upload failed in real-time recording uploaded case, 
								// so we need to do that here, when cancel the upload is not prompt the user interface
								// 230007 is the server did not receive a normal AMR recording end for chunked...
								return ;
							}
							
							
							if(GroupChatActivity.voiceMessage.containsKey(rMediaInfo.getMsgId()) ) {
								Log4Util.e(CCPHelper.DEMO_TAG, "isRecordAndSend");
								isRecordAndSend = false;
								return;
							}
							
							// This is a representative chunked patterns are transmitted speech file
							// If the execution returns to the false, is not chunked or send files
							//VoiceSQLManager.getInstance().updateIMChatMessage(rMediaInfo.getMsgId(), IMChatMsgDetail.TYPE_MSG_SEND_FAILED);
							sendType = IMChatMessageDetail.STATE_IM_SEND_FAILED;
							
							// failed
							// If the recording mode of data collection is the recording side upload (chunked), 
							// then in the recording process may be done to interrupt transfer for various reasons,
							// so, This failed reason can callback method ,But can't immediately begin to upload voice file , 
							// because there may not completed recording ,
							// You can set a Identification here on behalf of the recording process, transmission failure, 
							// wait for real recording completed then sending voice recording file
							
							// If it is after recording then uploading files,When the transmission 
							// failure can be Send out in  second times in this callback methods
							Toast.makeText(getApplicationContext(), R.string.toast_voice_send_failed, Toast.LENGTH_SHORT).show();
							if(mIMGroupApapter != null )  {
								//mIMGroupApapter.remove(msgDetail);
								//mIMGroupApapter = null ;
							}
						}
						
					} else if (instancemsg instanceof IMTextMsg) {
						IMTextMsg imTextMsg = (IMTextMsg)instancemsg;
						messageId = imTextMsg.getMsgId();
						if(reason == 0 ) {
							sendType = IMChatMessageDetail.STATE_IM_SEND_SUCCESS;
						} else {
							// do send text message failed ..
							sendType = IMChatMessageDetail.STATE_IM_SEND_FAILED;
						}
					}
					CCPSqliteManager.getInstance().updateIMMessageSendStatusByMessageId(messageId, sendType);
				} catch (SQLException e) {
					e.printStackTrace();
				}
				sendbroadcast();
				break;
			case CCPHelper.WHAT_ON_AMPLITUDE:
				
				double amplitude = b.getDouble(Device.VOICE_AMPLITUDE);
				
				if(mChatFooter != null ) {
					mChatFooter.displayAmplitude(amplitude);
				}
				
				break;
				
			case CCPHelper.WHAT_ON_RECODE_TIMEOUT:
				
				doProcesOperationRecordOver(false);
				break;
				
			case CCPHelper.WHAT_ON_PLAY_VOICE_FINSHING: 
				mVoicePlayState = TYPE_VOICE_STOP;
				releaseVoiceAnim(-1);
				//VoiceApplication.getInstance().setSpeakerEnable(false);
				CCPAudioManager.getInstance().resetSpeakerState(GroupChatActivity.this);
				break;
			case WHAT_ON_COMPUTATION_TIME:
				if(promptRecordTime() && getRecordState() == RECORD_ING) {
					sendEmptyMessageDelayed(WHAT_ON_COMPUTATION_TIME, TONE_LENGTH_MS);
				}
				
				break;
				
				
				// This call may be redundant, but to ensure compatibility system event more, 
				// not only is the system call
			case CCPHelper.WHAT_ON_RECEIVE_SYSTEM_EVENTS:
				
				onPause();
				break;
			default:
				break;
			}
		}


	};
	
	// voice local save path ..
	private File getCurrentVoicePath() {
		File directory = new File(CCPApplication.getInstance().getVoiceStore(),currentRecName);
		return directory;
	}
	
	
	
	/**
	 * 
	* <p>Title: doProcesOperationRecordOver</p>
	* <p>Description: 
	*    update version 3.5 .</p>
	* @param isCancleSend
	 */
	private void doProcesOperationRecordOver(boolean isCancleSend) {
		if (getRecordState()  == RECORD_ING) {
			// version 3.5
			// Here sometimes bug. for MODE chunked..
			// Because the record data of CCP SDK Record in the underlying collection according to the AMR code, 
			// if the first packet of data up to 650 bytes, so will open a thread transmission of the data, 
			// if you have not at the 650 bytes, so will not open the thread for the transmission of 
			// data and does not generate local audio files, So here if you are choosing a chunked transmission.
			// You or there is a better way to determine whether to send, at least judging there will be 
			// problems at a time.
			
			// e.g (When you use Chunke to transmission, so you can according to local file exists to decision 
			// whether the voice is too short) .In other . you can according to the speech voice duration to 
			// decision whether the voice is too short
			
			
			boolean isVoiceToShort = false;
			
			if(getCurrentVoicePath()!=null&&new File(getCurrentVoicePath().getAbsolutePath()).exists() && checkeDeviceHelper()) {
				recodeTime = getDeviceHelper().getVoiceDuration(
						getCurrentVoicePath().getAbsolutePath());
				
				// if not chunked ,then the voice file duration is greater than 1000ms.
				// If it is chunked. the voice file exists that has been send out
				if(!isRecordAndSend) {
					if (recodeTime < MIX_TIME) {
						isVoiceToShort = true;
					}
				}
				
			} else {
				
				isVoiceToShort = true;
			}
			
			setRecordState(RECORD_NO);
			
			if(mChatFooter != null ) {
				
				if (isVoiceToShort && !isCancleSend) {
					mChatFooter.tooShortPopuWindow();
					return;
				}
				
				mChatFooter.removePopuWindow();
			}

			if(!isCancleSend) {
				
				IMChatMessageDetail mVoicechatMessageDetail = IMChatMessageDetail
				.getGroupItemMessage(IMChatMessageDetail.TYPE_MSG_VOICE,IMChatMessageDetail.STATE_IM_SENDING, mGroupId);
				
				mVoicechatMessageDetail.setFilePath(getCurrentVoicePath()
						.getAbsolutePath());
				
				if (!isRecordAndSend && checkeDeviceHelper()) {
					// send
					uniqueId = getDeviceHelper().sendInstanceMessage(mGroupId,
							null, getCurrentVoicePath().getAbsolutePath(), USER_DATA);
				} else {
					voiceMessage.remove(uniqueId);
				}
				
				try {
					mVoicechatMessageDetail.setMessageId(uniqueId);
					mVoicechatMessageDetail.setUserData(USER_DATA);
					mVoicechatMessageDetail.setFileExt("amr");
					CCPSqliteManager.getInstance().insertIMMessage(
							mVoicechatMessageDetail);
					
					
					
					notifyGroupDateChange(mVoicechatMessageDetail);
					
				} catch (SQLException e) {
					e.printStackTrace();
					
				}
				
			}

		}
		recodeTime = 0;
	}
	
	AnimationDrawable mVoiceAnimation = null;
	ImageView mVoiceAnimImage;
	
	private static final int TYPE_VOICE_PLAYING = 3;
	private static final int TYPE_VOICE_STOP = 4;
	private int mVoicePlayState = TYPE_VOICE_STOP;;
	private int mPlayPosition = -1;
	
	void ViewPlayAnim(final ImageView iView , String path , int position) {
		int releasePosition = releaseVoiceAnim(position);
		
		if(releasePosition == position) {
			return;
		}
		mPlayPosition = position;
		try {
			// local downloaded file
			if(!TextUtils.isEmpty(path)&& isLocalAmr(path)) {
				
				if (mVoicePlayState == TYPE_VOICE_STOP) {
					if(!checkeDeviceHelper()) {
						return ;
					}
					
					mVoiceAnimation =  (AnimationDrawable) iView.getDrawable();
					mVoiceAnimImage = iView;
					// ---
					CCPAudioManager.getInstance().switchSpeakerEarpiece(GroupChatActivity.this, isEarpiece);
					
					
					// Here we suggest to try not to use SDK voice play interface 
					// and you can achieve Voice file playback interface 
					//  for example CCPVoiceMediaPlayManager.getInstance(GroupChatActivity.this).putVoicePlayQueue(position, path);
					
					// Interface of new speakerOn parameters,(Earpiece or Speaker)
					getDeviceHelper().playVoiceMsg(path , !isEarpiece);
					
					// 3.4.1.2
					updateVoicePlayModelView(isEarpiece);
					mVoiceAnimation.start();
					mVoicePlayState = TYPE_VOICE_PLAYING;
				}
			}
			
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/**
	 * 
	* <p>Title: updateVoicePlayModelView</p>
	* <p>Description: </p>
	* @param isEarpiece
	* @version 3.4.1.2
	 */
	private void updateVoicePlayModelView(boolean isEarpiece) {
		String speakerEarpiece = null;
		if(isEarpiece) {
			speakerEarpiece = getString(R.string.voice_listen_earpiece);
		} else {
			speakerEarpiece = getString(R.string.voice_listen_speaker);
		}
		
		addNotificatoinToView(speakerEarpiece , Gravity.TOP);
	}

	/**
	 * @version 3.5
	* <p>Title: releaseAnim</p>
	* <p>Description: The release is in a voice playback process resources picture, 
	*  animation resources, and stop playing the voice
	*  {@link GroupChatActivity#TYPE_VOICE_PLAYING}
	*  {@link GroupChatActivity#TYPE_VOICE_STOP}</p>
	*  
	*  @see Device#stopVoiceMsg()
	*  @see CCPAudioManager#resetSpeakerState(Context);
	 */
	private int releaseVoiceAnim(int position) {
		
		if(mVoiceAnimation != null ) {
			mVoiceAnimation.stop();
			int id = 0;
			if(mVoiceAnimImage.getId() == R.id.voice_chat_recd_tv_l) {
				id = R.anim.voice_play_from;
			}else if (mVoiceAnimImage.getId() == R.id.voice_chat_recd_tv_r) {
				id = R.anim.voice_play_to;
			}
			mVoiceAnimImage.setImageResource(0);
			mVoiceAnimImage.setImageResource(id);
			
			mVoiceAnimation = null;
			mVoiceAnimImage = null;
			
			
		}
		
		// if position is -1 ,then release Animatoin and can't start new Play.
		if(position == -1) {
			mPlayPosition = position;
		}
		
		// if amr voice file is playing ,then stop play
		if(mVoicePlayState == TYPE_VOICE_PLAYING) {
			if(!checkeDeviceHelper()) {
				return -1;
			}
			getDeviceHelper().stopVoiceMsg();
			// reset speaker 
			CCPAudioManager.getInstance().resetSpeakerState(GroupChatActivity.this);
			mVoicePlayState = TYPE_VOICE_STOP;
			
			return mPlayPosition;
		} 
		
		return -1;
	}
	
	boolean isLocalAmr(String url){
		if(new File(url).exists()) {
			return true ;
		} 
		Toast.makeText(this, R.string.toast_local_voice_file_does_not_exist, Toast.LENGTH_SHORT).show();
		return false;
	}
	
	private File takepicfile;
	private void takePicture() {
		Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
		takepicfile = CCPUtil.TackPicFilePath();
		if (takepicfile != null) {
			Uri uri = Uri.fromFile(takepicfile);
			if (uri != null) {
				intent.putExtra(MediaStore.EXTRA_OUTPUT, uri);
			}
		}
		startActivityForResult(intent, REQUEST_CODE_TAKE_PICTURE);
		
	}
	
	
   private boolean promptRecordTime() {
	   if(computationTime == -1L) {
			computationTime = SystemClock.elapsedRealtime();
		}
		long period = SystemClock.elapsedRealtime() - computationTime;
		int duration ;
		if(period >= 50000L && period <= 60000L) {
			if(mRecordTipsToast == null) {
				vibrate(50L);
				duration = (int )((60000L - period) / 1000L) ;
				Log4Util.i(CCPHelper.DEMO_TAG, "The remaining recording time :" + duration);
				mRecordTipsToast = Toast.makeText(getApplicationContext(), getString(R.string.chatting_rcd_time_limit, duration), Toast.LENGTH_SHORT);
			}
		}else {
			if(period < 60000L) {
				//sendEmptyMessageDelayed(WHAT_ON_COMPUTATION_TIME, TONE_LENGTH_MS);
				return true;
			}
			
			return false;
			
		}
		
		if(mRecordTipsToast != null ) {
			duration = (int )((60000L - period) / 1000L) ;
			Log4Util.i(CCPHelper.DEMO_TAG, "The remaining recording time :" + duration);
			mRecordTipsToast.setText(getString(R.string.chatting_rcd_time_limit, duration));
			mRecordTipsToast.show();
		}
		return true;
   }
   
   
   @Override
	protected int getLayoutId() {
		return R.layout.layout_group_chat_activity;
	}
   
   public int getRecordState() {
		return mRecordState;
	}
	
	public void setRecordState(int state) {
		this.mRecordState = state;
	}

// 3.4.1.1
   int getFirstMsgListItemBodyTop(){
	   
	   return -1;
   }

}
