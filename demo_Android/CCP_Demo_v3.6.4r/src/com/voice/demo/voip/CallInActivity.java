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
package com.voice.demo.voip;

import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Message;
import android.os.SystemClock;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.Chronometer;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import com.CCP.phone.CameraInfo;
import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.Device.CallType;
import com.hisun.phone.core.voice.Device.Rotate;
import com.hisun.phone.core.voice.model.CallStatisticsInfo;
import com.hisun.phone.core.voice.model.NetworkStatistic;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.VoiceUtil;
import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.interphone.InviteInterPhoneActivity;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.ui.CCPHelper;
import com.voice.demo.video.VideoActivity;

/**
 * 
 * Voip incoming interface, called for display and operation process of the
 * call��Oo
 * 
 * @version 1.0.0
 */
public class CallInActivity extends AudioVideoCallActivity implements OnClickListener ,SeekBar.OnSeekBarChangeListener{

	private TextView mVoipInputEt;
	// 键盘
	private ImageView mDiaerpadBtn;
	// 键盘区
	private LinearLayout mDiaerpad;
	// 挂机按钮
	private ImageView mVHangUp;
	// 动态状态显示区
	private TextView mCallStateTips;
	private Chronometer mChronometer;
	// 名称显示区
	private TextView mVtalkName;
	// 号码显示区
//	private TextView mVtalkNumber;

	private TextView mCallStatus;
	// 号码
	private String mPhoneNumber;
	// 名称
	private String mNickName;
	// 通话 ID
	// private String mCurrentCallId;
	// voip 账号
	private String mVoipAccount;
	// 透传号码参数
	private static final String KEY_TEL = "tel";
	// 透传名称参数
	private static final String KEY_NAME = "nickname";
	private static final int REQUEST_CODE_VOIP_CALL = 120;
	private static final int REQUEST_CODE_CALL_TRANSFER = 130;
	private boolean isDialerShow = false;

	private Button mVideoStop;
	private Button mVideoBegin;
	private ImageView mVideoIcon;
	private RelativeLayout mVideoTipsLy;

	private TextView mVideoTopTips;
	// private TextView mVideoCallTips;
	private SurfaceView mVideoView;
	// Remote Video
	private FrameLayout mVideoLayout;

	private ImageButton mCameraSwitch;

	CameraInfo[] cameraInfos;
	Intent currIntent;

	int numberOfCameras;
	private int mWidth;
	private int mHeight;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		isIncomingCall = true;
		initProwerManager();
		enterIncallMode();
		currIntent = getIntent();
		initialize(currIntent);
		initResourceRefs();
		registerReceiver(new String[] { CCPIntentUtils.INTENT_P2P_ENABLED });
	}

	@Override
	protected void onNewIntent(Intent intent) {
		super.onNewIntent(intent);

		if (!isConnect) {
			if (getCallHandler() != null) {
				getCallHandler().removeCallbacks(finish);
			}
			releaseCurrCall(false);
			currIntent = intent;
			initialize(currIntent);
			initResourceRefs();
		}

	}

	/**
	 * Initialize all UI elements from resources.
	 * 
	 */
	private void initResourceRefs() {
		isConnect = false;
		if (mCallType == Device.CallType.VIDEO) {
			// video ..
			setContentView(R.layout.layout_video_activity);

			findViewById(R.id.video_botton_cancle).setVisibility(View.GONE);

			mVideoTipsLy = (RelativeLayout) findViewById(R.id.video_call_in_ly);
			mVideoTipsLy.setVisibility(View.VISIBLE);

			mVideoIcon = (ImageView) findViewById(R.id.video_icon);

			mVideoTopTips = (TextView) findViewById(R.id.notice_tips);
			// Top tips view invited ...
			mVideoTopTips.setText(getString(R.string.str_video_invited_recivie, mVoipAccount.substring(mVoipAccount.length() - 3, mVoipAccount.length())));
			// 底部时间
			mCallStateTips = (TextView) findViewById(R.id.video_call_tips);
			// 接受
			mVideoBegin = (Button) findViewById(R.id.video_botton_begin);
			mVideoBegin.setVisibility(View.VISIBLE);
			// 拒绝
			mVideoStop = (Button) findViewById(R.id.video_stop);
			mVideoStop.setEnabled(true);
			mVideoBegin.setOnClickListener(this);
			mVideoStop.setOnClickListener(this);
			mSeekBar = (SeekBar) findViewById(R.id.seek_bar);
			mSeekBar.setOnSeekBarChangeListener(this);
			mVideoView = (SurfaceView) findViewById(R.id.video_view);
			mVideoView.getHolder().setFixedSize(240, 320);
			mLoaclVideoView = (RelativeLayout) findViewById(R.id.localvideo_view);
			mVideoLayout = (FrameLayout) findViewById(R.id.Video_layout);

			findViewById(R.id.video_switch).setOnClickListener(this);
			;

			mCameraSwitch = (ImageButton) findViewById(R.id.camera_switch);
			mCameraSwitch.setOnClickListener(this);

			mCallStatus = (TextView) findViewById(R.id.call_status);
			mCallStatus.setVisibility(View.GONE);

			if (checkeDeviceHelper()) {

				cameraInfos = getDeviceHelper().getCameraInfo();

				// Find the ID of the default camera
				if (cameraInfos != null) {
					numberOfCameras = cameraInfos.length;
				}

				// Find the total number of cameras available
				for (int i = 0; i < numberOfCameras; i++) {
					if (cameraInfos[i].index == android.hardware.Camera.CameraInfo.CAMERA_FACING_FRONT) {
						defaultCameraId = i;
						comportCapbilityIndex(cameraInfos[i].caps);
					}
				}
				//hubin modified
				getDeviceHelper().setVideoView(mVoipAccount, mVideoView, null);
			}

			DisplayLocalSurfaceView();

		} else {
			setContentView(R.layout.layout_callin);

			mVoipInputEt = (TextView) findViewById(R.id.voip_input);
			mVtalkName = (TextView) findViewById(R.id.layout_callin_name);
			mVtalkNumber = (TextView) findViewById(R.id.layout_callin_number);
			((ImageButton) findViewById(R.id.layout_callin_cancel)).setOnClickListener(this);
			((ImageButton) findViewById(R.id.layout_callin_accept)).setOnClickListener(this);
			setDisplayNameNumber();

		}
	}

	private void setDisplayNameNumber() {
		if (mCallType == Device.CallType.VOICE) {
			// viop call ...
			if (!TextUtils.isEmpty(mVoipAccount)) {
				mVtalkNumber.setText(mVoipAccount);
			}
		} else {
			// viop call ...
			if (!TextUtils.isEmpty(mPhoneNumber)) {
				mVtalkNumber.setText(mPhoneNumber);
				Log4Util.d(CCPHelper.DEMO_TAG, "[CallInActivity] mPhoneNumber " + mPhoneNumber);
			}
			if (!TextUtils.isEmpty(mNickName)) {
				mVtalkName.setText(mNickName);
				Log4Util.d(CCPHelper.DEMO_TAG, "[CallInActivity] VtalkName" + mVtalkName);
			} else {
				mVtalkName.setText(R.string.voip_unknown_user);
			}
		}
	}

	/**
	 * Read parameters or previously saved state of this activity.
	 * 
	 */
	private void initialize(Intent intent) {
		Bundle extras = intent.getExtras();
		if (extras == null) {
			finish();
			return;
		}
		mVoipAccount = extras.getString(Device.CALLER);
		mCurrentCallId = extras.getString(Device.CALLID);
		mCallType = (CallType) extras.get(Device.CALLTYPE);
		// 传入数据是否有误
		if (mVoipAccount == null || mCurrentCallId == null) {
			finish();
			return;
		}
		// 透传信息
		String[] infos = extras.getStringArray(Device.REMOTE);
		if (infos != null && infos.length > 0) {
			for (String str : infos) {
				if (str.startsWith(KEY_TEL)) {
					mPhoneNumber = VoiceUtil.getLastwords(str, "=");
				} else if (str.startsWith(KEY_NAME)) {
					mNickName = VoiceUtil.getLastwords(str, "=");
				}
			}
		}

	}

	public void initCallHold() {
		Log4Util.d(CCPHelper.DEMO_TAG, "[CallInActivity] initCallHold.收到呼叫连接，初始化通话界面.");
		isConnect = true;
		setContentView(R.layout.layout_call_interface);
		mCallTransferBtn = (ImageView) findViewById(R.id.layout_callin_transfer);
		mCallTransferBtn.setOnClickListener(this);
		mCallStateTips = (TextView) findViewById(R.id.layout_callin_duration);
		mCallMute = (ImageView) findViewById(R.id.layout_callin_mute);
		mCallHandFree = (ImageView) findViewById(R.id.layout_callin_handfree);
		mVHangUp = (ImageView) findViewById(R.id.layout_call_reject);
		mVtalkName = (TextView) findViewById(R.id.layout_callin_name);
		mVtalkName.setVisibility(View.VISIBLE);
		mVtalkNumber = (TextView) findViewById(R.id.layout_callin_number);

		mCallStatus = (TextView) findViewById(R.id.call_status);
		// mCallStatus.setVisibility(View.VISIBLE);

		// 显示时间，隐藏状态
		// 2013/09/23
		// Show call state position is used to display the packet loss rate and
		// delay
		// mCallStateTips.setVisibility(View.GONE);

		// 键盘
		mDiaerpadBtn = (ImageView) findViewById(R.id.layout_callin_diaerpad);
		mDiaerpad = (LinearLayout) findViewById(R.id.layout_diaerpad);

		setupKeypad();
		mDmfInput = (EditText) findViewById(R.id.dial_input_numer_TXT);

		mDiaerpadBtn.setOnClickListener(this);
		mCallMute.setOnClickListener(this);
		mCallHandFree.setOnClickListener(this);
		mVHangUp.setOnClickListener(this);

		setDisplayNameNumber();
		initCallTools();
	}
	
	@Override
	protected void onCallVideoRatioChanged(String callid, int width, int height) {
		super.onCallVideoRatioChanged(callid, width, height);
		if(mCallType != CallType.VIDEO) {
			return ;
		}
		try {
			mWidth = width;
			mHeight = height;
			if(mVideoView != null) {
				//mVideoView.getHolder().setFixedSize(Integer.parseInt(split[0]), Integer.parseInt(split[1]));
				LayoutParams layoutParams2 = (LayoutParams) mVideoView.getLayoutParams();
				layoutParams2.width = mWidth;
				layoutParams2.height = mHeight;
				mVideoView.setLayoutParams(layoutParams2);
			}
			int[] decodeDisplayMetrics = decodeDisplayMetrics();
			if(mWidth >= decodeDisplayMetrics[0] || mHeight >= decodeDisplayMetrics[1]) {
				//scale = 1;
				mSeekBar.setMax(decodeDisplayMetrics[0]);
				mSeekBar.setProgress(decodeDisplayMetrics[0]);
				mSeekBar.setVisibility(View.VISIBLE);
				return ;
			}
			
			mSeekBar.setMax(decodeDisplayMetrics[0]);
			mSeekBar.setProgress(decodeDisplayMetrics[0]);
			mSeekBar.setVisibility(View.VISIBLE);
			
			doResizeView(decodeDisplayMetrics[0]);
		} catch (Exception e) {
		}
	}

	@Override
	protected void onCallVideoRatioChanged(String callid, String resolution) {
		super.onCallVideoRatioChanged(callid, resolution);
		
		if(mCallType != CallType.VIDEO) {
			return ;
		}
		if(TextUtils.isEmpty(resolution) || !resolution.contains("x")) {
			return ;
		}
		String[] split = resolution.split("x");
		try {
			onCallVideoRatioChanged(callid, Integer.parseInt(split[0]), Integer.parseInt(split[1]));
		} catch (Exception e) {
		}
	}
	
	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.btn_select_voip: // select voip ...
			Intent intent = new Intent(this, InviteInterPhoneActivity.class);
			intent.putExtra("create_to", InviteInterPhoneActivity.CREATE_TO_VOIP_CALL);
			startActivityForResult(intent, REQUEST_CODE_VOIP_CALL);

			// keypad
		case R.id.zero: {
			keyPressed(KeyEvent.KEYCODE_0);
			return;
		}
		case R.id.one: {
			keyPressed(KeyEvent.KEYCODE_1);
			return;
		}
		case R.id.two: {
			keyPressed(KeyEvent.KEYCODE_2);
			return;
		}
		case R.id.three: {
			keyPressed(KeyEvent.KEYCODE_3);
			return;
		}
		case R.id.four: {
			keyPressed(KeyEvent.KEYCODE_4);
			return;
		}
		case R.id.five: {
			keyPressed(KeyEvent.KEYCODE_5);
			return;
		}
		case R.id.six: {
			keyPressed(KeyEvent.KEYCODE_6);
			return;
		}
		case R.id.seven: {
			keyPressed(KeyEvent.KEYCODE_7);
			return;
		}
		case R.id.eight: {
			keyPressed(KeyEvent.KEYCODE_8);
			return;
		}
		case R.id.nine: {
			keyPressed(KeyEvent.KEYCODE_9);
			return;
		}
		case R.id.star: {
			keyPressed(KeyEvent.KEYCODE_STAR);
			return;
		}
		case R.id.pound: {
			keyPressed(KeyEvent.KEYCODE_POUND);
			return;
		}

		case R.id.layout_callin_accept:
		case R.id.video_botton_begin: // video ..
			// 接受呼叫
			// mTime = 0;
			try {
				if (checkeDeviceHelper() && mCurrentCallId != null) {
					getDeviceHelper().acceptCall(mCurrentCallId, mVoipAccount);
				}
			} catch (Exception e) {
				e.printStackTrace();
			}
			Log4Util.d(CCPHelper.DEMO_TAG, "[CallInActivity] acceptCall...");
			break;
		case R.id.layout_callin_mute:
			// 设置静音
			setMuteUI();
			break;
		case R.id.layout_callin_handfree:
			// 设置免提
			sethandfreeUI();
			break;

		case R.id.layout_callin_diaerpad:

			// 设置键盘
			setDialerpadUI();
			break;
		case R.id.layout_callin_cancel:
		case R.id.layout_call_reject:

			doHandUpReleaseCall();
			break;

		// video back ...
		case R.id.video_stop:
			mVideoStop.setEnabled(false);
			doHandUpReleaseCall();

			break;

		case R.id.video_switch:
			if (checkeDeviceHelper()) {
				CallType callType = getDeviceHelper().getCallType(mCurrentCallId);
				if (callType == Device.CallType.VOICE) {
					getDeviceHelper().updateCallType(mCurrentCallId, Device.CallType.VIDEO);
				} else {
					getDeviceHelper().updateCallType(mCurrentCallId, Device.CallType.VOICE);
				}
			}

			break;
		case R.id.camera_switch:
			// check for availability of multiple cameras
			if (cameraInfos.length == 1) {
				AlertDialog.Builder builder = new AlertDialog.Builder(this);
				builder.setMessage(this.getString(R.string.camera_alert)).setNeutralButton(R.string.dialog_alert_close, null);
				AlertDialog alert = builder.create();
				alert.show();
				return;
			}
			mCameraSwitch.setEnabled(false);

			// OK, we have multiple cameras.
			// Release this camera -> cameraCurrentlyLocked
			cameraCurrentlyLocked = (cameraCurrentlyLocked + 1) % numberOfCameras;
			comportCapbilityIndex(cameraInfos[cameraCurrentlyLocked].caps);

			if (checkeDeviceHelper()) {

				getDeviceHelper().selectCamera(cameraCurrentlyLocked, mCameraCapbilityIndex, 15, Rotate.Rotate_Auto, false);

				if (cameraCurrentlyLocked == android.hardware.Camera.CameraInfo.CAMERA_FACING_FRONT) {
					defaultCameraId = android.hardware.Camera.CameraInfo.CAMERA_FACING_FRONT;
					Toast.makeText(CallInActivity.this, R.string.camera_switch_front, Toast.LENGTH_SHORT).show();
				} else {
					defaultCameraId = android.hardware.Camera.CameraInfo.CAMERA_FACING_BACK;
					Toast.makeText(CallInActivity.this, R.string.camera_switch_back, Toast.LENGTH_SHORT).show();
				}
			}
			mCameraSwitch.setEnabled(true);
			break;
		case R.id.layout_callin_transfer: // select voip ...
			Intent i = new Intent(this, GetNumberToTransferActivity.class);
			startActivityForResult(i, REQUEST_CODE_CALL_TRANSFER);

			break;
		default:
			break;
		}
	}

	@Override
	protected void doHandUpReleaseCall() {
		super.doHandUpReleaseCall();
		try {
			if (isConnect) {
				// 通话中挂断
				if (checkeDeviceHelper() && mCurrentCallId != null) {
					getDeviceHelper().releaseCall(mCurrentCallId);
					stopVoiceRecording(mCurrentCallId);
				}
			} else {
				// 呼入拒绝
				if (checkeDeviceHelper() && mCurrentCallId != null) {
					getDeviceHelper().rejectCall(mCurrentCallId, 6);
				}
				finish();
			}

			Log4Util.d(CCPHelper.DEMO_TAG, "[CallInActivity] call stop isConnect: " + isConnect);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/**
	 * 延时关闭界面
	 */
	final Runnable finish = new Runnable() {
		public void run() {
			finish();
		}
	};

	@Override
	protected void onDestroy() {
		releaseCurrCall(true);
		releaseWakeLock();
		super.onDestroy();
	}

	private void releaseCurrCall(boolean releaseAll) {
		currIntent = null;
		if (getCallHandler() != null && releaseAll) {
			setCallHandler(null);
		}
		mCallTransferBtn = null;
		mCallMute = null;
		mCallHandFree = null;
		mVHangUp = null;
		mCallStateTips = null;
		mVtalkName = null;
		mVtalkNumber = null;

		if (checkeDeviceHelper()) {
			if (isMute) {
				getDeviceHelper().setMute(!isMute);
			}
			if (isHandsfree) {
				getDeviceHelper().enableLoudsSpeaker(!isMute);
			}
			if (TextUtils.isEmpty(mCurrentCallId))
				stopVoiceRecording(mCurrentCallId);
		}
		mPhoneNumber = null;
		CCPApplication.getInstance().setAudioMode(AudioManager.MODE_NORMAL);
	}

	/**
	 * 用于挂断时修改按钮属性及关闭操作
	 */
	private void finishCalling() {
		try {
			if (isConnect) {
				mChronometer.stop();
				mCallStateTips.setVisibility(View.VISIBLE);

				isConnect = false;
				if (mCallType == Device.CallType.VIDEO) {
					mVideoLayout.setVisibility(View.GONE);
					mVideoIcon.setVisibility(View.VISIBLE);
					mVideoTopTips.setVisibility(View.VISIBLE);
					mCameraSwitch.setVisibility(View.GONE);

					mLoaclVideoView.removeAllViews();
					mLoaclVideoView.setVisibility(View.GONE);

					/*
					 * if(mVideoStop.isEnabled()) {
					 * mVideoTopTips.setText(getString
					 * (R.string.str_video_call_end,
					 * CCPConfig.VoIP_ID.substring(CCPConfig.VoIP_ID.length() -
					 * 3, CCPConfig.VoIP_ID.length()))); } else {
					 */
					mVideoTopTips.setText(R.string.voip_calling_finish);
					// }
					// bottom can't click ...
				} else {
					mChronometer.setVisibility(View.GONE);
					mCallStateTips.setText(R.string.voip_calling_finish);
					mCallHandFree.setClickable(false);
					mCallMute.setClickable(false);
					// mCallTransferBtn.setClickable(false);
					mVHangUp.setClickable(false);
					mDiaerpadBtn.setClickable(false);
					mDiaerpadBtn.setImageResource(R.drawable.call_interface_diaerpad);
					mCallHandFree.setImageResource(R.drawable.call_interface_hands_free);
					mCallMute.setImageResource(R.drawable.call_interface_mute);
					mVHangUp.setBackgroundResource(R.drawable.call_interface_non_red_button);
				}

				// 延时关闭
				getCallHandler().postDelayed(finish, 3000);
			} else {
				finish();
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	void setDialerpadUI() {
		if (isDialerShow) {
			mDiaerpadBtn.setImageResource(R.drawable.call_interface_diaerpad);
			mDiaerpad.setVisibility(View.GONE);
			isDialerShow = false;
		} else {
			mDiaerpadBtn.setImageResource(R.drawable.call_interface_diaerpad_on);
			mDiaerpad.setVisibility(View.VISIBLE);
			isDialerShow = true;
		}

	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {

		if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {

			// do nothing.
			return true;
		}

		return super.onKeyDown(keyCode, event);
	}

	@Override
	protected void onCallAnswered(String callid) {
		super.onCallAnswered(callid);
		// answer
		Log4Util.d(CCPHelper.DEMO_TAG, "[CallInActivity] voip on call answered!!");
		if (callid != null && callid.equals(mCurrentCallId)) {
			if (mCallType == Device.CallType.VIDEO) {
				initResVideoSuccess();
				//getDeviceHelper().enableLoudsSpeaker(true);
			} else {
				initialize(currIntent);
				// voip other ..
				initCallHold();
			}

			mChronometer = (Chronometer) findViewById(R.id.chronometer);
			mChronometer.setBase(SystemClock.elapsedRealtime());
			mChronometer.setVisibility(View.VISIBLE);
			mChronometer.start();
			if (getCallHandler() != null) {
				getCallHandler().sendMessage(getCallHandler().obtainMessage(VideoActivity.WHAT_ON_CODE_CALL_STATUS));
			}

			//startVoiceRecording(callid);
		}
	}

	@Override
	protected void onCallReleased(String callid) {
		super.onCallReleased(callid);
		// 挂断
		Log4Util.d(CCPHelper.DEMO_TAG, "[CallInActivity] voip on call released!!");
		try {
			if (callid != null && callid.equals(mCurrentCallId)) {
				stopVoiceRecording(callid);
				finishCalling();
				// finish();
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Override
	protected void handleNotifyMessage(Message msg) {
		super.handleNotifyMessage(msg);
		switch (msg.what) {
		case VideoActivity.WHAT_ON_CODE_CALL_STATUS:
			if (!checkeDeviceHelper()) {
				return;
			}

			if (!isConnect) {
				return;
			}

			CallStatisticsInfo callStatistics = getDeviceHelper().getCallStatistics(mCallType);
			NetworkStatistic trafficStats = null;
			if (mCallType == CallType.VOICE || mCallType == CallType.VIDEO) {
				trafficStats = getDeviceHelper().getNetworkStatistic(mCurrentCallId);
			}
			if (callStatistics != null) {
				int fractionLost = callStatistics.getFractionLost();
				int rttMs = callStatistics.getRttMs();
				if (mCallType == CallType.VOICE && mCallStateTips != null) {
					if (trafficStats != null) {
						mCallStateTips.setText(getString(R.string.str_call_traffic_status, rttMs, (fractionLost / 255), trafficStats.getTxBytes() / 1024,
								trafficStats.getRxBytes() / 1024));
					} else {

						mCallStateTips.setText(getString(R.string.str_call_status, rttMs, (fractionLost / 255)));
					}
				} else {
					if (trafficStats != null) {
						addNotificatoinToView(getString(R.string.str_call_traffic_status, rttMs, (fractionLost / 255), trafficStats.getTxBytes() / 1024,
								trafficStats.getRxBytes() / 1024));
					} else {
						addNotificatoinToView(getString(R.string.str_call_status, rttMs, (fractionLost / 255)));
					}
				}
			}

			if (getCallHandler() != null) {
				Message callMessage = getCallHandler().obtainMessage(VideoActivity.WHAT_ON_CODE_CALL_STATUS);
				getCallHandler().sendMessageDelayed(callMessage, 4000);
			}
			break;

		// This call may be redundant, but to ensure compatibility system event
		// more,
		// not only is the system call
		case CCPHelper.WHAT_ON_RECEIVE_SYSTEM_EVENTS:

			doHandUpReleaseCall();
			break;
		default:
			break;
		}
	}

	// video ..
	private void initResVideoSuccess() {

		mVideoLayout.setVisibility(View.VISIBLE);
		mVideoIcon.setVisibility(View.GONE);
		mCallStateTips.setText(getString(R.string.str_video_bottom_time, mVoipAccount.substring(mVoipAccount.length() - 3, mVoipAccount.length())));
		mCallStateTips.setVisibility(View.VISIBLE);
		mVideoTopTips.setVisibility(View.GONE);
		mCameraSwitch.setVisibility(View.VISIBLE);

		mVideoBegin.setVisibility(View.GONE);
		isConnect = true;
	}

	/*********************************** KeyPad *************************************************/

	private EditText mDmfInput;

	void keyPressed(int keyCode) {
		if (!checkeDeviceHelper()) {
			return;
		}
		KeyEvent event = new KeyEvent(KeyEvent.ACTION_DOWN, keyCode);
		mDmfInput.getText().clear();
		mDmfInput.onKeyDown(keyCode, event);
		getDeviceHelper().sendDTMF(mCurrentCallId, mDmfInput.getText().toString().toCharArray()[0]);
	}

	private void setupKeypad() {
		/** Setup the listeners for the buttons */
		findViewById(R.id.zero).setOnClickListener(this);
		findViewById(R.id.one).setOnClickListener(this);
		findViewById(R.id.two).setOnClickListener(this);
		findViewById(R.id.three).setOnClickListener(this);
		findViewById(R.id.four).setOnClickListener(this);
		findViewById(R.id.five).setOnClickListener(this);
		findViewById(R.id.six).setOnClickListener(this);
		findViewById(R.id.seven).setOnClickListener(this);
		findViewById(R.id.eight).setOnClickListener(this);
		findViewById(R.id.nine).setOnClickListener(this);
		findViewById(R.id.star).setOnClickListener(this);
		findViewById(R.id.pound).setOnClickListener(this);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);

		Log4Util.d(CCPHelper.DEMO_TAG, "[SelectVoiceActivity] onActivityResult: requestCode=" + requestCode + ", resultCode=" + resultCode + ", data=" + data);

		// If there's no data (because the user didn't select a number and
		// just hit BACK, for example), there's nothing to do.
		if (requestCode != REQUEST_CODE_VOIP_CALL) {
			if (data == null) {
				return;
			}
		} else if (resultCode != RESULT_OK) {
			Log4Util.d(CCPHelper.DEMO_TAG, "[SelectVoiceActivity] onActivityResult: bail due to resultCode=" + resultCode);
			return;
		}

		String phoneStr = "";
		if (data.hasExtra("VOIP_CALL_NUMNBER")) {
			Bundle extras = data.getExtras();
			if (extras != null) {
				phoneStr = extras.getString("VOIP_CALL_NUMNBER");
			}
		}
		if (mCurrentCallId != null) {
			int setCallTransfer = setCallTransfer(mCurrentCallId, phoneStr);
			if(setCallTransfer!=0){
				Toast.makeText(getApplicationContext(), "呼转发起失败！", 1).show();
//				mVtalkNumber.setText(phoneStr);
			}
		} else {
			Toast.makeText(getApplicationContext(), "通话已经不存在", 1).show();
		}
	}

	@Override
	public void onProgressChanged(SeekBar arg0, int arg1, boolean arg2) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onStartTrackingTouch(SeekBar arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onStopTrackingTouch(SeekBar arg0) {
		int progress = arg0.getProgress();
		doResizeView(progress);
	}

	/**
	 * @param progress
	 */
	private void doResizeView(int progress) {
		if (mVideoView != null) {
			LayoutParams layoutParams2 = (LayoutParams) mVideoView
					.getLayoutParams();
			layoutParams2.width = progress;
			layoutParams2.height = mHeight * progress / mWidth ;
			mVideoView.setLayoutParams(layoutParams2);
		}
	}
}
