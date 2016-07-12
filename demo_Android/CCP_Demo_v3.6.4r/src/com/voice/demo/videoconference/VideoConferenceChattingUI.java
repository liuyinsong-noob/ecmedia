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
package com.voice.demo.videoconference;

import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.yuntongxun.ecsdk.core.voip.ViERenderer;

import com.CCP.phone.CameraInfo;
import com.CCP.phone.VideoSnapshot;
import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.Device.Rotate;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceDismissMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceExitMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceJoinMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMember;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceRemoveMemberMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceSwitch;
import com.hisun.phone.core.voice.model.videoconference.VideoPartnerPortrait;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.VoiceUtil;
import com.voice.demo.R;
import com.voice.demo.chatroom.ChatroomName;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.tools.net.ITask;
import com.voice.demo.tools.net.TaskKey;
import com.voice.demo.tools.net.ThreadPoolManager;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.CCPHelper;
import com.voice.demo.videoconference.baseui.CCPVideoConUI;
import com.voice.demo.videoconference.baseui.CCPVideoConUI.OnVideoUIItemClickListener;
import com.voice.demo.views.CCPAlertDialog;

import android.app.AlarmManager;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.TransitionDrawable;
import android.os.Bundle;
import android.os.Message;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

/**
 * 
* <p>Title:VideoConferenceChattingUI.java</p>
* <p>Description: Set the main video, if the main video new set of main video server ID and current ID the same, 
* 	will cancel the compulsory primary video, becomes automatic switching mode.
* So you can also use this method to achieve the "abolition of compulsory primary video" function.</p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.cloopen.com</p>
* @author Jorstin Chan
* @date 2013-11-1
* @version 3.5
 */
public class VideoConferenceChattingUI extends VideoconferenceBaseActivity implements View.OnClickListener
																						,CCPVideoConUI.OnVideoUIItemClickListener
																						,CCPAlertDialog.OnPopuItemClickListener{

	/**
	 * The definition of video conference pattern of joining
	 * if the creator or join
	 * Invitation model
	 * @see #modeType
	 * @see #MODE_VIDEO_C_INITIATED_INTERCOM
	 */
	private static final int MODE_VIDEO_C_INVITATION = 0x0;
	
	/**
	 * Creator pattern model
	 * @see #modeType
	 * @see #MODE_VIDEO_C_INVITATION
	 */
	private static final int MODE_VIDEO_C_INITIATED_INTERCOM = 0x1; 
	
	/**
	 * Unique identifier defined message queue
	 * @see  #getBaseHandle()
	 */
	public static final int WHAT_ON_VIDEO_NOTIFY_TIPS = 0X2;
	
	/**
	 * Unique identifier defined message queue
	 * @see  #getBaseHandle()
	 */
	public static final int WHAT_ON_VIDEO_REFRESH_VIDEOUI = 0X3;
	
	/**
	 * The definition of the status bar at the top of the transition time to 
	 * update the state background
	 */
	public static final int ANIMATION_DURATION = 2000;
	
	/**
	 * The definition of the status bar at the top of the transition time to 
	 * update the state background
	 */
	public static final int ANIMATION_DURATION_RESET = 1000;
	
	/**
	 * 
	 */
	public static final String PREFIX_LOCAL_VIDEO = "local_";
	
	/**
	 * Broadcast action
	 * To monitor the alarm clock, every 30s for a local picture is sent to the server
	 */
	public static final String ACTION_CCP_REVIEW_LOCAL_PORPRTAIT = "com.voice.demo.ccp.ACTION_CCP_REVIEW_LOCAL_PORPRTAIT";
	
	/**
	 * To monitor the alarm clock, every 40s for download picture of Video Conference
	 */
	public static final String ACTION_CCP_REVIEW_PORPRTAIT_REMOTE = "com.voice.demo.ccp.ACTION_CCP_REVIEW_PORPRTAIT_REMOTE";
	
	public HashMap<String, Integer> mVideoMemberUI = new HashMap<String, Integer>();
	public HashMap<String, VideoPartnerPortrait> mVideoPorprtaitCache = new HashMap<String, VideoPartnerPortrait>();
	
	private TextView mVideoTips;
	private CCPVideoConUI mVideoConUI;
	private ImageButton mCameraControl;
	private ImageButton mMuteControl;
	private ImageButton mVideoControl;
	
	private FrameLayout mVideoUIRoot;
	private View instructionsView;
	private View videoMainView;
	private Button mExitVideoCon;
	
	private AlarmManager mAlarmManager;
	
	private String mVideoMainScreenVoIP;
	private String mVideoConferenceId;
	private String mVideoCreate;
	private CameraInfo[] cameraInfos;
	// The first rear facing camera
	int defaultCameraId;
	int numberOfCameras;
	int cameraCurrentlyLocked;
	
	/**
	 * Capbility index of pixel.
	 */
	int mCameraCapbilityIndex;
	
	private int modeType ;
	private boolean isMute = false;
	private boolean isVideoConCreate = false;
	private boolean isVideoChatting = false;
	
	private boolean isSynchronousData = false;
	
	// Whether to display all the members including frequency
	@Deprecated
	private boolean isDisplayAllMembers = true;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		//setContentView(R.layout.video_conference);
		mVideoUIRoot = addFeatureGuide();
		setContentView(mVideoUIRoot);

		// Initialize members for UI elements.
		initResourceRefs();
		
		if(checkeDeviceHelper()) {
			initialize(savedInstanceState);
			cameraInfos = getDeviceHelper().getCameraInfo();
		}
		mAlarmManager = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
		
		// Find the ID of the default camera
		if(cameraInfos != null) {
			numberOfCameras = cameraInfos.length;
		}
		
		// Find the total number of cameras available
		for (int i = 0; i < numberOfCameras; i++) {
            if (cameraInfos[i].index == android.hardware.Camera.CameraInfo.CAMERA_FACING_FRONT) {
                defaultCameraId = i;
                mCameraCapbilityIndex = CCPUtil.comportCapbilityIndex(cameraInfos[i].caps);
            }
        }
		
		
		registerReceiver(new String[]{ACTION_CCP_REVIEW_LOCAL_PORPRTAIT ,ACTION_CCP_REVIEW_PORPRTAIT_REMOTE});
		
		
	}

	private void initResourceRefs() {
		mVideoTips = (TextView) findViewById(R.id.notice_tips);
		
		mVideoConUI = (CCPVideoConUI) findViewById(R.id.video_ui);
		mVideoConUI.setOnVideoUIItemClickListener(this);
		
		mCameraControl = (ImageButton) findViewById(R.id.camera_control);
		mMuteControl = (ImageButton) findViewById(R.id.mute_control);
		mVideoControl = (ImageButton) findViewById(R.id.video_control);
		mVideoControl.setVisibility(View.GONE);
		mCameraControl.setOnClickListener(this);
		mMuteControl.setOnClickListener(this);
		mMuteControl.setEnabled(false);
		mCameraControl.setEnabled(false);
		mVideoControl.setOnClickListener(this);
		
		mExitVideoCon = (Button) findViewById(R.id.out_video_c_submit);
		mExitVideoCon.setOnClickListener(this);
		
		initNewInstructionResourceRefs();
	}
	
	private void initNewInstructionResourceRefs() {
		if(instructionsView != null) 
			findViewById(R.id.begin_video_conference).setOnClickListener(this);
	}

	private void initialize(Bundle savedInstanceState) {
		Intent intent = getIntent();
		String roomName = null ;
		boolean is_auto_close=true;
		int autoDelete = 1;
		int voiceMode = 1;
		if(intent.hasExtra(ChatroomName.AUTO_DELETE)){
			Bundle extras = intent.getExtras();
			if (extras != null) {
				autoDelete=extras.getInt(ChatroomName.AUTO_DELETE);
			}
		}
		if(intent.hasExtra(ChatroomName.VOICE_MOD)){
			Bundle extras = intent.getExtras();
			if (extras != null) {
				voiceMode=extras.getInt(ChatroomName.VOICE_MOD);
			}
		}
		if(intent.hasExtra(ChatroomName.IS_AUTO_CLOSE)){
			Bundle extras = intent.getExtras();
			if (extras != null) {
				is_auto_close=extras.getBoolean(ChatroomName.IS_AUTO_CLOSE);
			}
		}
		if(intent.hasExtra(ChatroomName.IS_AUTO_CLOSE)){
			Bundle extras = intent.getExtras();
			if (extras != null) {
				is_auto_close=extras.getBoolean(ChatroomName.IS_AUTO_CLOSE);
			}
		}
		if(intent.hasExtra(ChatroomName.CHATROOM_NAME)) {
			modeType = MODE_VIDEO_C_INITIATED_INTERCOM;
			Bundle extras = intent.getExtras();
			if (extras != null) {
				roomName = extras.getString(ChatroomName.CHATROOM_NAME); 
				if(TextUtils.isEmpty(roomName)) {
					finish();
				} else {
					mVideoCreate = extras.getString(VideoconferenceConversation.CONFERENCE_CREATOR); 
					isVideoConCreate = CCPConfig.VoIP_ID.equals(mVideoCreate);
					//mVideoConUI.setOperableEnable(isVideoConCreate);
					if(!isVideoConCreate && instructionsView != null) instructionsView.setVisibility(View.GONE);
				}
				
			}
		}
		
		if(intent.hasExtra(Device.CONFERENCE_ID)) {
			// To invite voice group chat
			modeType = MODE_VIDEO_C_INVITATION;
			Bundle extras = intent.getExtras();
			if (extras != null) {
				mVideoConferenceId = extras.getString(Device.CONFERENCE_ID); 
				if(TextUtils.isEmpty(mVideoConferenceId)) {
					finish();
				}
			}
			
		}

		// init VideoUI View key
		initUIKey();
		
		mVideoTips.setText(R.string.top_tips_connecting_wait);
		
		if(checkeDeviceHelper()) {
			//hubin modified
			getDeviceHelper().setVideoView(mVideoConferenceId, mVideoConUI.getMainSurfaceView(), null);
			// Launched a Viode Conference request, waiting for SDK to return.
			if(modeType == MODE_VIDEO_C_INITIATED_INTERCOM ) {
				getDeviceHelper().startVideoConference(CCPConfig.App_ID, roomName, 5, null, null,is_auto_close,voiceMode,autoDelete==1?true:false,true);
			} else if (modeType == MODE_VIDEO_C_INVITATION) {
				
				// Initiate a join Viode Conference request, waiting for SDK to return.
				getDeviceHelper().joinVideoConference(mVideoConferenceId);
			}
		}
	}

	private void initMute() {
		if (isMute) {
			mMuteControl.setImageResource(R.drawable.mute_forbid_selector);
		} else {
			mMuteControl.setImageResource(R.drawable.mute_enable_selector);
		}
	}
	
	/**
	 * 
	* <p>Title: setMuteUI</p>
	* <p>Description: </p>
	 */
	private void setMuteUI() {
		if(!checkeDeviceHelper()) {
			return;
		}
		try {
			getDeviceHelper().setMute(!isMute);
			isMute = getDeviceHelper().getMuteStatus();
			initMute();
			
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	
	/**
	* <p>Title: updateVideoNoticeTipsUI</p>
	* <p>Description: Display a background update the status bar, as well as text
	* between update and normal.</p>
	* @param text
	 */
	public void updateVideoNoticeTipsUI(CharSequence text){
		
		if(!TextUtils.isEmpty(text)) {
			getBaseHandle().removeMessages(WHAT_ON_VIDEO_NOTIFY_TIPS);
			mVideoTips.setText(text);
			TransitionDrawable  transition  = (TransitionDrawable) mVideoTips.getBackground();
			transition.resetTransition();
			transition.startTransition(ANIMATION_DURATION);
			Message msg = getBaseHandle().obtainMessage(WHAT_ON_VIDEO_NOTIFY_TIPS);
			getBaseHandle().sendMessageDelayed(msg, 6000);
		}
	}
	
	
	
	
	ArrayList<Integer> UIKey = new ArrayList<Integer>();
	
	/**
	 * 
	* <p>Title: getCCPVideoConUIKey</p>
	* <p>Description: </p>
	* @return
	 */
	public synchronized Integer getCCPVideoConUIKey() {
		
		if(UIKey.isEmpty()) {
			return null;
		}
		Log4Util.v(CCPHelper.DEMO_TAG, "remove CCP Key out : " + UIKey.toString());
		return UIKey.remove(0);
	}
	
	public synchronized void putCCPVideoConUIKey(Integer key) {
		
		if(UIKey.size() > 4) {
			return ;
		}
		
		if(key <= 2) {
			return ;
		}
		
		UIKey.add(/*key - 3, */key);
		Collections.sort(UIKey, new Comparator<Integer>() {
			@Override
			public int compare(Integer lsdKey, Integer rsdKey) {
				
				//Apply sort mode
				return lsdKey.compareTo(rsdKey);
			}
		});
		Log4Util.v(CCPHelper.DEMO_TAG, "put CCP Key into : " + key + " ,UIKey " + UIKey.toString());
	}
	
	/**
	 * 
	* <p>Title: putVideoUIMemberCache</p>
	* <p>Description: </p>
	* @param who
	* @param key
	 */
	public void putVideoUIMemberCache(String who ,Integer key) {
		synchronized (mVideoMemberUI) {
			Log4Util.v(CCPHelper.DEMO_TAG, "put VideoUIMember who : " + who + " , key " + key);
			if(key == CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW) {
				mVideoMainScreenVoIP = who;
			} else {
				mVideoMemberUI.put(who , key);
			}
		}
	}
	
	/**
	 * 
	* <p>Title: isVideoUIMemberCacheEmpty</p>
	* <p>Description: </p>
	* @return
	 */
	public boolean isVideoUIMemberCacheEmpty() {
		synchronized (mVideoMemberUI) {
			Log4Util.v(CCPHelper.DEMO_TAG, "VideoUIMember size : " + mVideoMemberUI.size() );
			return mVideoMemberUI.isEmpty();
		}
	}
	
	/**
	 * 
	* <p>Title: removeVideoUIMemberFormCache</p>
	* <p>Description: </p>
	* @param who
	* @return
	 */
	public Integer removeVideoUIMemberFormCache(String who) {
		synchronized (mVideoMemberUI) {
			Integer key =  mVideoMemberUI.remove(who);
			Log4Util.v(CCPHelper.DEMO_TAG, "remove VideoUIMember who : " + who + " , key " + key);
			return key;
		}
	}
	
	/**
	 * 
	* <p>Title: queryVideoUIMemberFormCache</p>
	* <p>Description: </p>
	* @param who
	* @return
	 */
	public Integer queryVideoUIMemberFormCache(String who) {
		synchronized (mVideoMemberUI) {
			if(!mVideoMemberUI.containsKey(who) && !who.equals(mVideoMainScreenVoIP)) {
				Log4Util.v(CCPHelper.DEMO_TAG, "Sorry , current VideoUI Member Cache not have " + who);
				return null;
			}
			Integer key = null;
			
			
			if(mVideoMemberUI.containsKey(who)) {
				key = mVideoMemberUI.get(who);
			} else {
				
				if(who.equals(mVideoMainScreenVoIP)) {
					key = CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW;
				}
			}
			
			Log4Util.v(CCPHelper.DEMO_TAG, "query VideoUIMember who : " + who + " key is  " + key);
			return key;
		}
	}
	
	/**
	 * 
	* <p>Title: getVideoVoIPByCCPUIKey</p>
	* <p>Description: </p>
	* @param CCPUIKey
	* @return
	 */
	private String getVideoVoIPByCCPUIKey(Integer CCPUIKey) {
		synchronized (mVideoMemberUI) {
			if(CCPUIKey == CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW) {
				return mVideoMainScreenVoIP;
			}
			if(CCPUIKey != null ) {
				for(Map.Entry<String, Integer> entry : mVideoMemberUI.entrySet()) {
					if(CCPUIKey.intValue() == entry.getValue().intValue()) {
						Log4Util.v(CCPHelper.DEMO_TAG, "getVideoVoIPByCCPUIKey : who " + entry.getKey() + " , key " + CCPUIKey);
						return entry.getKey();
					}
				}
			}
			return null;
		}
	}
	
	/**
	 * 
	* <p>Title: isVideoUIMemberExist</p>
	* <p>Description: </p>
	* @param who
	* @return
	 */
	private boolean isVideoUIMemberExist(String who) {
		synchronized (mVideoMemberUI) {
			if(TextUtils.isEmpty(who)) {
				return false;
			}
			
			if(mVideoMemberUI.containsKey(who)) {
				return true;
			}
			
			return false;
		}
	}
	
	public synchronized void initUIKey() {
		UIKey.clear();
		UIKey.add(CCPVideoConUI.LAYOUT_KEY_SUB_IMAGEVIEW_1);
		UIKey.add(CCPVideoConUI.LAYOUT_KEY_SUB_IMAGEVIEW_2);
		UIKey.add(CCPVideoConUI.LAYOUT_KEY_SUB_IMAGEVIEW_3);
		UIKey.add(CCPVideoConUI.LAYOUT_KEY_SUB_IMAGEVIEW_4);
		
		Log4Util.v(CCPHelper.DEMO_TAG, "initUIKey ,UIKey " + UIKey.toString());
	}
	
	
	public String PORTRAIT_PATH = CCPUtil.getExternalStorePath()+  "/" +CCPUtil.DEMO_ROOT_STORE + "/.videoPortrait";
	

	public File CreatePortraitFilePath(String fileName , String ext) {
		File localFile = new File(PORTRAIT_PATH , fileName + "." + ext);
		if ((!localFile.getParentFile().exists())
				&& (!localFile.getParentFile().mkdirs())) {
			localFile = null;
		}
		return localFile;
	}
	
	
	public String getPortraitFilePath(String fileName) {
		File localFile = new File(PORTRAIT_PATH , fileName);
		return localFile.getAbsolutePath();
	}
	
	/**
	 * 
	* <p>Title: loadVideoPorprtaitPath</p>
	* <p>Description: </p>
	* @param fileName
	* @return
	 */
	public File loadVideoPorprtaitPath(String fileName) {
		File file = new File(PORTRAIT_PATH);
		if(file.isDirectory() && file.exists()) {
			File[] listFiles = file.listFiles(new CCPFilenameFilter(fileName));
			if(listFiles != null && listFiles.length > 0) {
				return listFiles[0];
			}
		}
		return null;
	}
	
	/**
	 * 
	* <p>Title: setAlarmTime</p>
	* <p>Description: </p>
	* @param triggerAtTime
	* @param interval
	* @param action
	 */
	private void setAlarmTime(long triggerAtTime ,long interval, String action) {
		setAlarmTime(triggerAtTime, 0 ,interval, action);
	}
	
	private void cancelAlarmTime(String action , int requestCode) {
		isSynchronousData = false;
		PendingIntent pendingIntent = PendingIntent.getBroadcast(this, requestCode, new Intent(action), PendingIntent.FLAG_CANCEL_CURRENT);
		mAlarmManager.cancel(pendingIntent);
	}
	
	private void setAlarmTime(long triggerAtTime, int requestCode ,long interval, String action) {
		isSynchronousData = true;
        PendingIntent pendingIntent = PendingIntent.getBroadcast(this, requestCode, new Intent(action), PendingIntent.FLAG_CANCEL_CURRENT);
        mAlarmManager.setRepeating(AlarmManager.RTC_WAKEUP, triggerAtTime, interval , pendingIntent);
	}
	
	/**
	 * 
	* <p>Title: cancelAlarmTime</p>
	* <p>Description: </p>
	* @param action
	 */
	private void cancelAlarmTime(String action) {
		cancelAlarmTime(action, 0);
	}
	
	/**
	 * 
	* <p>Title: getVideoRemotePorprtaitDrawable</p>
	* <p>Description: </p>
	* @param portrait
	* @return
	* @throws IOException
	 */
	private BitmapDrawable getVideoRemotePorprtaitDrawable(
			VideoPartnerPortrait portrait) throws IOException {
		return CCPUtil.getImageDrawable(
				getPortraitFilePath(portrait.getVoip() +"."+ VoiceUtil.getExtensionName(portrait.getFileName())));
	}
	
	/**
	 * 
	 * @Title: addFeatureGuide 
	 * @Description: TODO 
	 * @param @return 
	 * @return FrameLayout 
	 * @throws
	 */
	private FrameLayout addFeatureGuide() {
		FrameLayout.LayoutParams iViewFLayoutParams = new FrameLayout.LayoutParams(
				FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT);
		iViewFLayoutParams.gravity = 17;
		
		videoMainView = getLayoutInflater().inflate(R.layout.video_conference , null);
		videoMainView.setLayoutParams(iViewFLayoutParams);

		FrameLayout frameLayout = new FrameLayout(this);
		frameLayout.addView(videoMainView);
		
		boolean instrucViewEnable = CcpPreferences.getSharedPreferences()
				.getBoolean(CCPPreferenceSettings.SETTING_SHOW_INSTRUCTIONS_VIEW.getId(), Boolean.TRUE);
		if(instrucViewEnable && instructionsView == null) {
			instructionsView = getLayoutInflater().inflate(R.layout.new_instructions , null);
			instructionsView.setLayoutParams(iViewFLayoutParams);
			instructionsView.setVisibility(View.VISIBLE);
			frameLayout.addView(instructionsView);
		}
		
		return frameLayout;
	}
	
	// --------------------------------------android  circle callback -------------------
	
	@Override
	protected void onResume() {
		super.onResume();
		DisplayLocalSurfaceView();
		
		setSycnAlarmTimeClock();
		lockScreen();
		
	}

	private void setSycnAlarmTimeClock() {
		if(!isSynchronousData && isVideoChatting) {
			// every 30 mills upload one video frame data
			setAlarmTime(System.currentTimeMillis() + 5*1000, 30*1000, ACTION_CCP_REVIEW_LOCAL_PORPRTAIT);
			setAlarmTime(System.currentTimeMillis() + 10*1000, 1,  40*1000, ACTION_CCP_REVIEW_PORPRTAIT_REMOTE);
		}
	}

	private void DisplayLocalSurfaceView() {
		SurfaceView localView = ViERenderer.CreateLocalRenderer(this);
		localView.setZOrderOnTop(false);
		mVideoConUI.setSubSurfaceView(localView);
		
		cameraCurrentlyLocked = defaultCameraId;
		if(checkeDeviceHelper()) {
			getDeviceHelper().selectCamera(cameraCurrentlyLocked, mCameraCapbilityIndex, 15, Rotate.Rotate_Auto , true);
		}
	}
	
	/**
	 * 
	* <p>Title: exitOrDismissVideoConference</p>
	* <p>Description: dismiss or exit of Video Conference</p>
	* @param exit
	 */
	public void exitOrDismissVideoConference(boolean dismiss) {
		cancleAlarmTimeClock();
		if(!checkeDeviceHelper()) {
			finish();
			return;
		}
		
		if(dismiss && isVideoConCreate) {
			
			showConnectionProgress(getString(R.string.str_dialog_message_default));
			getDeviceHelper().dismissVideoConference(CCPConfig.App_ID, mVideoConferenceId);
		} else {
			getDeviceHelper().exitVideoConference();
			
			// If it is the creator of the video conference, then don't send broadcast
			// when exit of the video conference.
			// Because the creators of Video Conference in create a video conference 
			// don't add to video conference list. And the video conference creator exit that is 
			// dismiss video conference, without notice to refresh the list
			if(!isVideoConCreate && dismiss) {
				Intent disIntent = new Intent(CCPIntentUtils.INTENT_VIDEO_CONFERENCE_DISMISS);
				disIntent.putExtra(Device.CONFERENCE_ID, mVideoConferenceId);
				sendBroadcast(disIntent);
			}
			finish();
		}
	}
	
	@Override
	protected void onPause() {
		super.onPause();
		
		releaseLockScreen();
		cancleAlarmTimeClock();
	}

	private void cancleAlarmTimeClock() {
		cancelAlarmTime(ACTION_CCP_REVIEW_LOCAL_PORPRTAIT);
		cancelAlarmTime(ACTION_CCP_REVIEW_PORPRTAIT_REMOTE , 1);
		isSynchronousData = false;
	}
	
	
	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.out_video_c_submit:
			
			mExitVideoCon.setEnabled(false);
			doVideoConferenceDisconnect();
			mExitVideoCon.setEnabled(true);
			break;
			
		case R.id.camera_control:
			
			mCameraControl.setEnabled(false);
			// check for availability of multiple cameras
            if (cameraInfos.length == 1) {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setMessage(this.getString(R.string.camera_alert))
                       .setNeutralButton(R.string.dialog_alert_close, null);
                AlertDialog alert = builder.create();
                alert.show();
                return ;
            }

            // OK, we have multiple cameras.
            // Release this camera -> cameraCurrentlyLocked
            cameraCurrentlyLocked = (cameraCurrentlyLocked + 1)  % numberOfCameras;
            mCameraCapbilityIndex =  CCPUtil.comportCapbilityIndex(cameraInfos[cameraCurrentlyLocked].caps);
            
            if(cameraCurrentlyLocked == android.hardware.Camera.CameraInfo.CAMERA_FACING_FRONT) {
            	defaultCameraId=android.hardware.Camera.CameraInfo.CAMERA_FACING_FRONT;
            	Toast.makeText(VideoConferenceChattingUI.this, R.string.camera_switch_front, Toast.LENGTH_SHORT).show();
            	mCameraControl.setImageResource(R.drawable.camera_switch_back_selector);
            } else {
            	Toast.makeText(VideoConferenceChattingUI.this,  R.string.camera_switch_back, Toast.LENGTH_SHORT).show();
            	defaultCameraId=android.hardware.Camera.CameraInfo.CAMERA_FACING_BACK;
            	mCameraControl.setImageResource(R.drawable.camera_switch_font_selector);
            	
            }
            
            if(checkeDeviceHelper())
            	getDeviceHelper().selectCamera(cameraCurrentlyLocked, mCameraCapbilityIndex, 15, Rotate.Rotate_Auto , false);
			
            mCameraControl.setEnabled(true);
			break;
			
		case R.id.mute_control:
			
			setMuteUI();
			break;
		case R.id.begin_video_conference:
			try {
				if(mVideoUIRoot != null) {
					mVideoUIRoot.removeView(instructionsView);
					CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_SHOW_INSTRUCTIONS_VIEW, Boolean.FALSE, true);
				}
			} catch (Exception e) {
			}
			break;
		default:
			break;
		}
	}
	
	
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		
		if(mVideoMemberUI != null) {
			mVideoMemberUI.clear();
			mVideoMemberUI = null;
		}
		
		if(mVideoPorprtaitCache != null) {
			mVideoPorprtaitCache.clear();
			mVideoPorprtaitCache = null;
		}
		
		if(mVideoConUI != null) {
			mVideoConUI.release();
			mVideoConUI = null;
		}
		
		instructionsView = null;
		videoMainView = null;
		mAlarmManager = null;
		
		mVideoConferenceId = null;
		mVideoCreate = null;
		cameraInfos = null;
		
		// The first rear facing camera
		isMute = false;
		isSynchronousData = false;
		isVideoConCreate = false;
		isVideoChatting = false;
		
	}
	
	
	/**
	 * Set the main video, if the main video new set of main video server ID and current ID the same, 
	 * will cancel the compulsory primary video, becomes automatic switching mode.
	 * So you can also use this method to achieve the "abolition of compulsory primary video" function.
	 */
	@Override
	public void onVideoUIItemClick(int CCPUIKey) {
		
		int[] ccpAlertResArray = null;
		int title = 0;
		if(CCPUIKey == CCPVideoConUI.LAYOUT_KEY_SUB_SURFACEVIEW) {
			// If he is the create of Video Conference
			// The main object is not myself
			if(CCPConfig.VoIP_ID.equals(mVideoMainScreenVoIP)) {
				title = R.string.str_switch_video_tips;
				ccpAlertResArray = new int[]{
						R.string.video_c_dismiss};
//				ccpAlertResArray = new int[]{
//						R.string.str_switch_video_free};
			} else {
				ccpAlertResArray = new int[]{R.string.video_c_dismiss,
						R.string.str_video_manager_self_main_screen};
			}
		} else {
			String who = getVideoVoIPByCCPUIKey(CCPUIKey);
			if(!TextUtils.isEmpty(who) && who.equals(mVideoMainScreenVoIP)) {
				ccpAlertResArray = new int[]{
						R.string.str_video_manager_self_main_screen
						,R.string.str_video_manager_remove};
			} else {
				ccpAlertResArray = new int[]{
						R.string.str_video_manager_main_screen
						,R.string.str_video_manager_remove};
			}
		}
		CCPAlertDialog ccpAlertDialog = new CCPAlertDialog(VideoConferenceChattingUI.this
				, title
				, ccpAlertResArray
				, 0
				, R.string.dialog_cancle_btn);
		
		// set CCP UIKey
		ccpAlertDialog.setUserData(CCPUIKey);
		ccpAlertDialog.setOnItemClickListener(this);
		ccpAlertDialog.create();
		ccpAlertDialog.show();
	}
	

	@Override
	public void onItemClick(ListView parent, View view, int position,
			int resourceId) {
		switch (resourceId) {
		case R.string.video_c_logout:
			exitOrDismissVideoConference(false);
			break;
		case R.string.video_c_dismiss:
			exitOrDismissVideoConference(isVideoConCreate);
			break;
			
			// The members of the main screen video conference
		case R.string.str_video_manager_main_screen:
		case R.string.str_video_manager_self_main_screen:
			
			
			Integer CCPUIKey = (Integer) view.getTag();
			String voipSwitch = null;
			if(CCPUIKey.intValue() == CCPVideoConUI.LAYOUT_KEY_SUB_SURFACEVIEW) {
				voipSwitch = CCPConfig.VoIP_ID;
			} else {
				
				voipSwitch = getVideoVoIPByCCPUIKey(CCPUIKey);
				if(TextUtils.isEmpty(mVideoMainScreenVoIP)) {
					return ;
				}
				
				if(mVideoMainScreenVoIP.equals(voipSwitch)) {
					voipSwitch = CCPConfig.VoIP_ID;
				}
				
			}
			
			if(TextUtils.isEmpty(voipSwitch)){
				return;
			}
			
			if(!TextUtils.isEmpty(voipSwitch) && checkeDeviceHelper()) {
				showConnectionProgress(getString(R.string.str_dialog_message_default));
				getDeviceHelper().switchRealScreenToVoip(CCPConfig.App_ID, mVideoConferenceId, voipSwitch);
				
			}
			break;
			// The members will be removed from the video conference
		case R.string.str_video_manager_remove:
			Integer CCPUIKeyRemove = (Integer) view.getTag();
			String voipRemove = getVideoVoIPByCCPUIKey(CCPUIKeyRemove);
			if(!TextUtils.isEmpty(voipRemove) && checkeDeviceHelper()) {
				showConnectionProgress(getString(R.string.str_dialog_message_default));
				getDeviceHelper().removeMemberFromVideoConference(CCPConfig.App_ID, mVideoConferenceId, voipRemove);
				
			}
			break;
		case R.string.dialog_cancle_btn:

			// do nothing.
			break;
		}
	}

	
	/**
	 * 
	* <p>Title: doVideoConferenceDisconnect</p>
	* <p>Description: The end of processing video conference popu menu list</p>
	* 
	* @see CCPAlertDialog#CCPAlertDialog(Context, int, int[], int, int)
	 */
	private void doVideoConferenceDisconnect() {
		cancleAlarmTimeClock();
		int videoTips = R.string.video_c_logout_warning_tip;
		int videoExit = R.string.video_c_logout;
//		if(isVideoConCreate) {
//			videoTips = R.string.video_c_dismiss_warning_tip;
////			videoExit = R.string.video_c_dismiss;
//		}
		
		CCPAlertDialog ccpAlertDialog = new CCPAlertDialog(VideoConferenceChattingUI.this
				, videoTips
				, null
				, videoExit
				, R.string.dialog_cancle_btn);
		ccpAlertDialog.setOnItemClickListener(this);
		ccpAlertDialog.setOnDismissListener(new OnDismissListener() {
			
			@Override
			public void onDismiss(DialogInterface dialog) {
				setSycnAlarmTimeClock();
				Log4Util.d(CCPHelper.DEMO_TAG, "CCPAlertDialog dismiss.");
			}
		});
		ccpAlertDialog.create();
		ccpAlertDialog.show();
	}
	
	
	/**
	 * 
	* <p>Title: queryVideoMembersPorprtait</p>
	* <p>Description: Query image on the local SDCARD According to the VOIP , 
	* if the local does not exist, then get the download address from a network</p>
	* @param ccpVideoConUIKey
	* @param who
	* @throws IOException
	 */
	private void queryVideoMembersPorprtait(Integer ccpVideoConUIKey, String who)
			throws IOException {
		File videoFile = loadVideoPorprtaitPath(who);
		if(videoFile != null && videoFile.exists()) {
			
			BitmapDrawable imageDrawable = CCPUtil.getImageDrawable(videoFile.getAbsolutePath());
			mVideoConUI.setImageViewDrawable(ccpVideoConUIKey, imageDrawable);
		} else {
			mVideoConUI.setImageViewDrawableLoading(ccpVideoConUIKey, "加载中...", null);
			if(checkeDeviceHelper()) {
				// Obtain the members the head
				// The interface will return all the video conference member avatar download address
				getDeviceHelper().getPortraitsFromVideoConference(mVideoConferenceId);
			}
		}
	}

	/**
	 * 
	* <p>Title: setVideoUITextOperable</p>
	* <p>Description: Unified setting for VideoUI text display, and set whether can click operation</p>
	* @param ccpVideoConUIKey
	* @param who
	* 
	* @see CCPVideoConUI#setOnVideoUIItemClickListener(OnVideoUIItemClickListener l)
	 */
	private void setVideoUITextOperable(Integer ccpVideoConUIKey, String who) {
		
		if(isDisplayAllMembers) {
			mVideoConUI.setVideoUIText(ccpVideoConUIKey, CCPUtil.interceptStringOfIndex(who, 4) , isVideoConCreate);
			return ;
		}
		
		if(!isVideoConCreate || CCPConfig.VoIP_ID.equals(who)) {
			
			// If not the creator, so for all members of VideoUI are set to not click operation
			// If self ,then do the same thing
			mVideoConUI.setVideoUIText(ccpVideoConUIKey, CCPUtil.interceptStringOfIndex(who, 4));
			return;
		}
		
		// Otherwise, display of operation and management of the icon, 
		// and enable a listener of callback method that set by 
		// CCPVideoConUI#setOnVideoUIItemClickListener(OnVideoUIItemClickListener l)
		mVideoConUI.setVideoUIText(ccpVideoConUIKey, CCPUtil.interceptStringOfIndex(who, 4) , true);
		
	}

	/**
	 * 
	* <p>Title: removeMemberFormVideoUI</p>
	* <p>Description: remove the member of Video Conference form VideoUI</p>
	* @param who
	 */
	private void removeMemberFormVideoUI(String who) {
		Integer ccpVideoConUIKey = removeVideoUIMemberFormCache(who);
		removeMemberFromVideoUI(ccpVideoConUIKey);
		
	}

	/**
	* <p>Title: removeMemberFromVideoUI</p>
	* <p>Description: </p>
	* @param ccpVideoConUIKey
	 */
	private void removeMemberFromVideoUI(Integer ccpVideoConUIKey) {
		if(ccpVideoConUIKey != null) {
			// The layout of ID release where the rooms,
			putCCPVideoConUIKey(ccpVideoConUIKey);
			mVideoConUI.setImageViewDrawable(ccpVideoConUIKey, null);
			setVideoUITextOperable(ccpVideoConUIKey, null);
			Log4Util.v(CCPHelper.DEMO_TAG, "set VideoUI key " + ccpVideoConUIKey + " null.");
		}
	}
	
	
	/**
	 * 
	* <p>Title: checkPorprtaitCache</p>
	* <p>Description: </p>
	* @param videoPortraits
	 */
	private void checkPorprtaitCache(List<VideoPartnerPortrait> videoPortraits) {
		ArrayList<VideoPartnerPortrait> urlList = new ArrayList<VideoPartnerPortrait>();
		for(VideoPartnerPortrait portrait : videoPortraits) {
			try {
				if(mVideoMemberUI == null || portrait == null) {
					continue;
				}
				String who = portrait.getVoip();
				// Not in the list of members is not within the frequency
				if(queryVideoUIMemberFormCache(who) == null || CCPConfig.VoIP_ID.equals(who)) {
					// do nothing..
					// At present we deal only with the current video conference member's Avatar
					// Ignore the other.
					Log4Util.d(CCPHelper.DEMO_TAG, "The portrait sender " + who + " not in VideoConference " + mVideoConferenceId);
					continue; 
				}
				
				// If the avatar is updated only added to the download queue
				boolean isChange = true;
				// The member of video conference picture download address stored in the cache.
				// Because self don't need pictures, so we ignore
				if(!CCPConfig.VoIP_ID.equals(who)) {
					VideoPartnerPortrait tempPortrait = mVideoPorprtaitCache.get(who);
					if(tempPortrait != null && tempPortrait.getDateUpdate().equals(portrait.getDateUpdate())) {
						// If the avatar is updated only added to the download queue
						isChange = false;
					}
					mVideoPorprtaitCache.put(who, portrait);
				}
				
				if(queryVideoUIMemberFormCache(who) == CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW
						&& CCPConfig.VoIP_ID.equals(who)) {
					
					// if the main screen member or self, then do ignore download picture;
					// do nothing..
					continue;
				}
				
				if(isChange) {
					// If the avatar is updated only added to the download queue
					File createPortraitFilePath = CreatePortraitFilePath(who, VoiceUtil.getExtensionName(portrait.getFileName()));
					if(createPortraitFilePath == null ) {
						return;
					}
					portrait.setFileLocalPath(createPortraitFilePath.getAbsolutePath());
					urlList.add(portrait);
				}
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		
		if(!urlList.isEmpty() && checkeDeviceHelper()) {
			// do download portrait ..
			getDeviceHelper().downloadVideoConferencePortraits(urlList);
		}
	}
	

	/**
	 * 
	* <p>Title: initMembersOnVideoUI</p>
	* <p>Description:Will refresh the list of members to VideoUI video 
	*  do background thread .</p>
	* @param members
	 */
	private void initMembersOnVideoUI(List<VideoConferenceMember> members) {
		for(VideoConferenceMember member : members) {
			// Send an message to the message queue
			// Update VideoUI in the main thread
			Message obtainMessage = getBaseHandle().obtainMessage(WHAT_ON_VIDEO_REFRESH_VIDEOUI);
			Bundle b = new Bundle();
			
			if(member.getScreen() == 1) {
				putVideoUIMemberCache(member.getNumber() , CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW);
				
				b.putInt("ccpVideoConUIKey", CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW);
			} else {
				
				if(CCPConfig.VoIP_ID.equals(member.getNumber())) {
					
					continue;
				}
				
				if(isVideoUIMemberExist(member.getNumber())) {
					// Because this is just in the query members, if not start query members, 
					// but someone had just joined, will have the question
					//So here the query to the existing members, ignored
					continue;
				}
				
			}
			
			Integer ccpVideoConUIKey = null;
			if(CCPConfig.VoIP_ID.equals(member.getNumber())) {
				ccpVideoConUIKey = CCPVideoConUI.LAYOUT_KEY_SUB_SURFACEVIEW;
			} else {
				
				ccpVideoConUIKey = getCCPVideoConUIKey();
			}
			
			if(ccpVideoConUIKey == null) {
				break;
			}
			
			putVideoUIMemberCache(member.getNumber() , ccpVideoConUIKey);
			b.putInt("ccpVideoConUIKey", ccpVideoConUIKey);
			
			b.putString("who", member.getNumber());
			obtainMessage.obj = b;
			getBaseHandle().sendMessage(obtainMessage);
		}
		
		if(checkeDeviceHelper()) {
			
			// Obtain the members the head
			// The interface will return all the video conference member avatar download address
			getDeviceHelper().getPortraitsFromVideoConference(mVideoConferenceId);
		}
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {
			
			if(isVideoChatting) {
				doVideoConferenceDisconnect();
			}
			return true;
		}

		return super.onKeyDown(keyCode, event);
	}
	
	
	@Override
	protected void handleDialogOkEvent(int requestKey) {
		super.handleDialogOkEvent(requestKey);
		
		if(DIALOG_SHOW_KEY_DISSMISS_VIDEO == requestKey) {
			exitOrDismissVideoConference(true);
		} else if (DIALOG_SHOW_KEY_REMOVE_VIDEO == requestKey) {
			// The removed member of the video conference is self.
			exitOrDismissVideoConference(false);
		}
	}
	
	// -----------------------------------------------------SDK Callback ---------------------------
	@Override
	protected void handleVideoConferenceState(int reason, String conferenceId) {
		super.handleVideoConferenceState(reason, conferenceId);
		
		if(!checkeDeviceHelper()) {
			return;
		}
		
		if(reason == 0) {
			isVideoChatting = true;
			mVideoConferenceId = conferenceId;
			mExitVideoCon.setEnabled(true);
			mCameraControl.setEnabled(true);
			mMuteControl.setEnabled(true);
			isMute = getDeviceHelper().getMuteStatus();
			initMute();
			
			updateVideoNoticeTipsUI(getString(R.string.video_tips_joining, conferenceId));
			
			if(isVideoConCreate){
				
				// if self is create of Video Conference ,then put self CCPKey in VideoUIMemberCache.
				//putVideoUIMemberCache(CCPConfig.VoIP_ID , CCPVideoConUI.LAYOUT_KEY_SUB_SURFACEVIEW);
				
				// If you are the creator, then will be locked for the main screen to display video conference
				// Otherwise it will default to display the main screen according to the volume of the sound
				getDeviceHelper().switchRealScreenToVoip(CCPConfig.App_ID, conferenceId, CCPConfig.VoIP_ID);
				mVideoMainScreenVoIP = CCPConfig.VoIP_ID;
			}
			// Query the participation of all members of the video conference
			getDeviceHelper().queryMembersInVideoConference(conferenceId);
			
			setSycnAlarmTimeClock();
		} else {
			isVideoChatting = false;
			Log4Util.d(CCPHelper.DEMO_TAG , " Sorry ,join Video Conference failed ...");
			getDeviceHelper().exitVideoConference();
			Toast.makeText(getApplicationContext(), getString(R.string.str_join_video_c_failed, reason), Toast.LENGTH_SHORT).show();
			finish();
		}
	}
	
	
	@Override
	protected void handleVideoConferenceDismiss(int reason, String conferenceId) {
		super.handleVideoConferenceDismiss(reason, conferenceId);
		
		closeConnectionProgress();
		
		// 111805【视频群聊】房间未找到
		if(reason != 0 && (reason != 111805)) {
			Toast.makeText(VideoConferenceChattingUI.this, getString(R.string.toast_video_dismiss_result , reason), Toast.LENGTH_SHORT).show();
			return;
		}
		
		// When the create of video conference dismiss this.
		// Will also receive a sip notification video conference has be dismiss, 
		// execute exit video conference operation this time
		exitOrDismissVideoConference(false);
	}
	
	@Override
	protected void handleReceiveVideoConferenceMsg(VideoConferenceMsg VideoMsg) {
		super.handleReceiveVideoConferenceMsg(VideoMsg);
		
		synchronized (VideoConferenceChattingUI.class) {
			try {
				// if the Video Conference ID is empty .then The next step
				if(VideoMsg == null || !mVideoConferenceId.equals(VideoMsg.getConferenceId())) {
					
					// not current Video Conference . then do nothing.
					return;
				}
				
				
				if(VideoMsg instanceof VideoConferenceJoinMsg) {
					
					VideoConferenceJoinMsg videoJoinMessage = (VideoConferenceJoinMsg) VideoMsg;
					String[] whos = videoJoinMessage.getWhos();
					
					for(String who : whos) {
						
						if(isVideoUIMemberExist(who)) {
							continue;
						}
						if(CCPConfig.VoIP_ID.equals(who)) {
							continue;
						}
						
						// has Somebody join 
						Integer ccpVideoConUIKey = getCCPVideoConUIKey();
						if(ccpVideoConUIKey == null) {
							return;
						}
						
						putVideoUIMemberCache(who, ccpVideoConUIKey);
						
						queryVideoMembersPorprtait(ccpVideoConUIKey, who);
						
						// If there is no image, then show the account information also.
						setVideoUITextOperable(ccpVideoConUIKey, who);
						updateVideoNoticeTipsUI(getString(R.string.str_video_conference_join , who));
						
					}
					
					// some one exit Video Conference..
				} else if (VideoMsg instanceof VideoConferenceExitMsg) {
					VideoConferenceExitMsg videoExitMessage = (VideoConferenceExitMsg) VideoMsg;
					String[] whos = videoExitMessage.getWhos();
					for (String who : whos) {
						
						// remove the member of Video Conference form VideoUI
						removeMemberFormVideoUI(who);
						updateVideoNoticeTipsUI(getString(R.string.str_video_conference_exit , who));
						
					}
					
				} else if (VideoMsg instanceof VideoConferenceDismissMsg) {
					
					// If it is the creator of the video conference, then don't send broadcast
					// when exit of the video conference.
					// Because the creators of Video Conference in create a video conference 
					// don't add to video conference list. And the video conference creator exit that is 
					// dismiss video conference, without notice to refresh the list
					if(isVideoConCreate) {
						
						// do nothing .
						return ;
					}
					
					//  The creator to dismiss of video conference (PUSH to all staff room)
					VideoConferenceDismissMsg videoConferenceDismissMsg = (VideoConferenceDismissMsg) VideoMsg;
					if(videoConferenceDismissMsg.getConferenceId().equals(mVideoConferenceId)) {
						showAlertTipsDialog(DIALOG_SHOW_KEY_DISSMISS_VIDEO
								, getString(R.string.dialog_title_be_dissmiss_video_conference)
								, getString(R.string.dialog_message_be_dissmiss_video_conference)
								, getString(R.string.dialog_btn)
								, null);
					}
					
				} else if (VideoMsg instanceof VideoConferenceRemoveMemberMsg) {
					//The creator to remove a member(PUSH to all staff room)
					VideoConferenceRemoveMemberMsg vCRemoveMemberMsg = (VideoConferenceRemoveMemberMsg) VideoMsg;
					
					
					if(CCPConfig.VoIP_ID.equals(vCRemoveMemberMsg.getWho())) {
						// The removed member of the video conference is self.
						showAlertTipsDialog(DIALOG_SHOW_KEY_REMOVE_VIDEO
								, getString(R.string.str_system_message_remove_v_title)
								, getString(R.string.str_system_message_remove_v_message)
								, getString(R.string.dialog_btn)
								, null);
					} else {
						removeMemberFormVideoUI(vCRemoveMemberMsg.getWho());
					}
					
				} else if (VideoMsg instanceof VideoConferenceSwitch) {
					// Video conferencing switching the main screen (PUSH to all staff room)
					VideoConferenceSwitch videoConferenceSwitch = (VideoConferenceSwitch) VideoMsg;
					String who = videoConferenceSwitch.getWho(); //76
					if(TextUtils.isEmpty(who)) {
						return;
					}
					
					// 1.get the voip account of the main screen in VideoUI.
					// 2.update the VideoUI of subImageView.
					//String voip = mVideoConUI.getVideoUIText(CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW);
					// 76
					String voipMainScreen = getVideoVoIPByCCPUIKey(CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW);
					
					if(TextUtils.isEmpty(voipMainScreen)) {
						// If the main screen image is empty, then ignore the sip message
						return ;
					}
					
					if(who.equals(voipMainScreen)) {
						return;
					}
					
					if(!isDisplayAllMembers) {
						// If you do not display all the members, when the display 
						// a frequency when not displaying pictures
						if(!doSwitchScreenModel(voipMainScreen ,who)) {
							return;
						}
					}
					
					// Will be switched to a new main Screen of the VoIP into the buffer queue
					putVideoUIMemberCache(who, /*ccpVideoConUIKey*/CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW);
					
					// update the VideoUI of switch main screen.
					setVideoUITextOperable(CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW, who);
					
					// Set the frequency display area display frame members
					if(CCPConfig.VoIP_ID.equals(who)) {
						mVideoConUI.setVideoUIMainScreen(CCPVideoConUI.LAYOUT_KEY_SUB_SURFACEVIEW);
					} else {
						Integer ccpVideoConUIKeyWho = queryVideoUIMemberFormCache(who);
						if(ccpVideoConUIKeyWho != null) {
							mVideoConUI.setVideoUIMainScreen(ccpVideoConUIKeyWho);
						}
					}
					// Notice the status bar and who is switched into frequency
					updateVideoNoticeTipsUI(getString(R.string.video_tips_switch , who));
				}
				
			} catch (Exception e) {
				e.printStackTrace();
				Log4Util.i(CCPHelper.DEMO_TAG, "Sorry,update VideoUI error. " + VideoMsg);
			}
		}
	}
	
	/**
	 * 
	* <p>Title: doSwitchScreenModel</p>
	* <p>Description: If you do not display all the members, when the display 
	* 	a frequency when not displaying pictures</p>
	* @param who
	* @return
	* @throws IOException
	 */
	@Deprecated
	private boolean doSwitchScreenModel(String voipMainScreen , String who) throws IOException {
		
		Integer ccpVideoConUIKey = null;
		if(CCPConfig.VoIP_ID.equals(who)) {
			
			// If self are switched into the main screen. 
			// You need to add a new sub VideoUI for removed from the main screen placed member
			ccpVideoConUIKey = getCCPVideoConUIKey();
		} else {
			
			// If other members to switch into frequency, 
			// then we need to exchange the position display
			// remove the member that of new main screen from sub VideoUI.
			ccpVideoConUIKey = removeVideoUIMemberFormCache(who);
		}
		
		
		if(ccpVideoConUIKey != null) {
			
			if(CCPConfig.VoIP_ID.equals(voipMainScreen)) {
				// If self switched into the main screen, it does not operate
				// Then put CCPKey back into the queue, and releases the screen resources
				// remove the member of Video Conference form VideoUI
				removeMemberFromVideoUI(ccpVideoConUIKey);
				removeVideoUIMemberFormCache(voipMainScreen);
				// Will be switched to a new main Screen of the VoIP into the buffer queue
				//putVideoUIMemberCache(who, /*ccpVideoConUIKey*/CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW);
			} else {
				
				// Otherwise it will be removed from the main screen back member
				// into the sub VideoUI
				// Loading has been removed from the main screen picture
				putVideoUIMemberCache(voipMainScreen, ccpVideoConUIKey);
				
				queryVideoMembersPorprtait(ccpVideoConUIKey, voipMainScreen);
				// If there is no image, then show the account information also.
				setVideoUITextOperable(ccpVideoConUIKey, voipMainScreen);
			}
			
		}
		
		return true;
	}

	
	@Override
	protected void handleVideoConferenceMembers(int reason,
			List<VideoConferenceMember> members) {
		super.handleVideoConferenceMembers(reason, members);
		
		if(reason == 0) {
			ITask iTask = new ITask(KEY_TASK_INIT_VIDEOUI_MEMBERS);
			iTask.setTaskParameters("members", members);
			addTask(iTask);
		}
	}
	
	
	@Override
	protected void handleGetPortraitsFromVideoConference(int reason,
			List<VideoPartnerPortrait> videoPortraits) {
		super.handleGetPortraitsFromVideoConference(reason, videoPortraits);
		
		if(!CCPUtil.isExistExternalStore()) {
			return;
		}
		if(reason == 0) {
			ITask iTask = new ITask(KEY_TASK_DOWNLOAD_PORPRTAIT);
			iTask.setTaskParameters("videoPortraits", videoPortraits);
			addTask(iTask);
		}
	}
	
	/**
	 * Execute in the background
	 * 
	 * @see ThreadPoolManager#addTask(ITask)
	 */
	@SuppressWarnings("unchecked")
	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		int key = iTask.getKey();
		if(key == KEY_TASK_DOWNLOAD_PORPRTAIT) {
			
			// To find out the head members updated, then  download.
			List<VideoPartnerPortrait> videoPortraits = (List<VideoPartnerPortrait>) iTask.getTaskParameters("videoPortraits");
			checkPorprtaitCache(videoPortraits);
			
		} else if (key == KEY_TASK_INIT_VIDEOUI_MEMBERS) {
			
			// init members on VideoUI
			List<VideoConferenceMember> members = (List<VideoConferenceMember>) iTask.getTaskParameters("members");
			initMembersOnVideoUI(members);
		} else if (key == TaskKey.TASK_KEY_VIDEO_FRAME) {
			
			VideoSnapshot snapshot = getDeviceHelper().getLocalVideoSnapshot();
			
			if(snapshot != null) {
				
				handleVideoConferenceLocalPortrait(snapshot);
				
			}
		}
	}

	
	@Override
	protected void handleSendLocalPortrait(int reason, String conferenceId) {
		super.handleSendLocalPortrait(reason, conferenceId);
		setAlarmTime(System.currentTimeMillis() + 30*1000, 30*1000, ACTION_CCP_REVIEW_LOCAL_PORPRTAIT);
	}
	
	
	@Override
	protected void handleSwitchRealScreenToVoip(int reason) {
		super.handleSwitchRealScreenToVoip(reason);
		closeConnectionProgress();
		if(reason != 0) {
			Toast.makeText(getApplicationContext(), getString(R.string.str_video_switch_failed, reason), Toast.LENGTH_SHORT).show();
		}
	}
	
	@Override
	protected void handleVideoConferenceRemoveMember(int reason, String member) {
		super.handleVideoConferenceRemoveMember(reason, member);
		closeConnectionProgress();
		if(reason != 0) {
			Toast.makeText(getApplicationContext(), getString(R.string.str_video_remove_failed, member,reason), Toast.LENGTH_SHORT).show();
			return;
		}
	}
	
	@Override
	protected void handleDownloadVideoConferencePortraits(int reason,
			VideoPartnerPortrait portrait) {
		super.handleDownloadVideoConferencePortraits(reason, portrait);
		
		if(reason == 0) {
			// Because self don't need pictures, so we ignore
			if(CCPConfig.VoIP_ID.equals(portrait.getVoip())) {
				return;
			}
			
			Integer integer = queryVideoUIMemberFormCache(portrait.getVoip())/*mVideoMemberUI.get(portrait.getVoip())*/;
			if(integer == null ) {
				return;
			}
			if(mVideoConUI != null /*&& integer != CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW*/) {
				
				try {
					setVideoUITextOperable(integer, portrait.getVoip());
					mVideoConUI.setImageViewDrawable(integer, getVideoRemotePorprtaitDrawable(portrait));
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}
	}

	/**
	 * @param snapshot
	 */
	public void handleVideoConferenceLocalPortrait(VideoSnapshot snapshot) {
		if(!checkeDeviceHelper()) {
			return;
		}
		if(snapshot != null) {
			if(CCPUtil.isExistExternalStore()) {
				String fileName = CreatePortraitFilePath(PREFIX_LOCAL_VIDEO + CCPConfig.VoIP_ID, "png").getAbsolutePath();
				CCPUtil.saveByteToFile(snapshot, fileName);
				getDeviceHelper().sendLocalPortrait(fileName, mVideoConferenceId);
			}
		}
	}
	
	@Override
	protected void onReceiveBroadcast(Intent intent) {
		super.onReceiveBroadcast(intent);
		if(!checkeDeviceHelper()) {
			return;
		}
		if(intent.getAction().equals(ACTION_CCP_REVIEW_LOCAL_PORPRTAIT)){
			// The local video frame data
			cancelAlarmTime(ACTION_CCP_REVIEW_LOCAL_PORPRTAIT);
			//getDeviceHelper().getVideoConferenceLocalPortrait();
			
			ITask iTask = new ITask(TaskKey.TASK_KEY_VIDEO_FRAME);
			addTask(iTask);
			
		} else if (intent.getAction().equals(ACTION_CCP_REVIEW_PORPRTAIT_REMOTE)) {
			
			// Obtain the members the head
			// The interface will return all the video conference member avatar download address
			//cancelAlarmTime(ACTION_CCP_REVIEW_PORPRTAIT_REMOTE);
			getDeviceHelper().getPortraitsFromVideoConference(mVideoConferenceId);
		}
	}

	@Override
	protected void handleNotifyMessage(Message msg) {
		super.handleNotifyMessage(msg);
		
		Bundle bundle = null;
		int what = msg.what;
		if(what == WHAT_ON_VIDEO_NOTIFY_TIPS) {
			mVideoTips.setText(getString(R.string.video_tips_joining, mVideoConferenceId));
			TransitionDrawable  transition  = (TransitionDrawable) mVideoTips.getBackground();
			transition.reverseTransition(ANIMATION_DURATION_RESET);
			
		} else if (what == WHAT_ON_VIDEO_REFRESH_VIDEOUI) {
			if (msg.obj instanceof Bundle) {
				bundle = (Bundle) msg.obj;
				int ccpVideoConUIKey = bundle.getInt("ccpVideoConUIKey");
				String members = bundle.getString("who");
				
				if(ccpVideoConUIKey != 0 && members != null) {
					
					try {
						if(ccpVideoConUIKey >= 3) {
							File videoFile = loadVideoPorprtaitPath(members);
							if(videoFile != null && videoFile.exists()) {
								
								BitmapDrawable imageDrawable = CCPUtil.getImageDrawable(videoFile.getAbsolutePath());
								mVideoConUI.setImageViewDrawable(ccpVideoConUIKey, imageDrawable);
								
							} else {
								
								mVideoConUI.setImageViewDrawableLoading(ccpVideoConUIKey, "加载中...", null);
							}
						}
						
						if(members.equals(mVideoMainScreenVoIP) && ccpVideoConUIKey != CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW) {
							mVideoConUI.setVideoUIMainScreen(ccpVideoConUIKey);
						}
					} catch (IOException e) {
						e.printStackTrace();
					}
					setVideoUITextOperable(ccpVideoConUIKey, members);
				}
				
				if(!TextUtils.isEmpty(mVideoMainScreenVoIP) && mVideoMainScreenVoIP.equals(members)) {
					setVideoUITextOperable(CCPVideoConUI.LAYOUT_KEY_MAIN_SURFACEVIEW, members);
				}
			}
		}
		
	}
	
	
	/**
	 * 
	* <p>Title: VideoConferenceChattingUI.java</p>
	* <p>Description: </p>
	* <p>Copyright: Copyright (c) 2007</p>
	* <p>Company: http://www.cloopen.com</p>
	* @author zhanjichun
	* @date 2013-11-7
	* @version 1.0
	 */
	class CCPFilenameFilter implements FilenameFilter{

		String fileName = null;
		 public CCPFilenameFilter(String fileNoExtensionNoDot)
		 {
			 fileName = fileNoExtensionNoDot;
		 }
		 
		@Override
		public boolean accept(File dir, String filename) {

			
			
			return filename.startsWith(fileName);
		}
		
	}


	@Override
	protected int getLayoutId() {
		return -1;
	}

}
