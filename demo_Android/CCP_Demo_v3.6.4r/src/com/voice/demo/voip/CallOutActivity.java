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

import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Chronometer;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.Device.CallType;
import com.hisun.phone.core.voice.DeviceListener.Reason;
import com.hisun.phone.core.voice.listener.OnProcessOriginalAudioDataListener;
import com.hisun.phone.core.voice.model.CallStatisticsInfo;
import com.hisun.phone.core.voice.model.NetworkStatistic;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.VoiceUtil;
import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.ui.CCPHelper;
import com.voice.demo.video.VideoActivity;

/**
 * 
 * Voip呼出界面，呼出方用于显示和操作通话过程。
 * 
 * @version 1.0.0
 */
public class CallOutActivity extends AudioVideoCallActivity implements OnClickListener {

	private static final int REQUEST_CODE_CALL_TRANSFER = 110;

	// 话筒调节控制区
	private LinearLayout mCallAudio;
	// 键盘
	private ImageView mDiaerpadBtn;
	// 键盘区
	private LinearLayout mDiaerpad;

	// 挂机按钮
	private ImageView mVHangUp;
	// 动态状态显示区
	private TextView mCallStateTips;
	private Chronometer mChronometer;
	// 号码显示区
//	private TextView mVtalkNumber;
	// private TextView mCallStatus;
	// 号码
	private String mPhoneNumber;
	// 通话 ID
	// public static String mCurrentCallId;
	// voip 账号
	private String mVoipAccount;
	// 通话类型，直拨，落地, 回拨
	private String mType = "";
	// 是否键盘显示
	private boolean isDialerShow = false;

	private Button mPause;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.layout_call_interface);

		isIncomingCall = false;
		mCallType = Device.CallType.VOICE;
		initProwerManager();
		enterIncallMode();
		initResourceRefs();
		initialize();
		initCall();
		registerReceiver(new String[] { CCPIntentUtils.INTENT_P2P_ENABLED });
		
	}

	/**
	 * Initialize all UI elements from resources.
	 * 
	 */
	private void initResourceRefs() {
		mCallTransferBtn = (ImageView) findViewById(R.id.layout_callin_transfer);
		mCallTransferBtn.setEnabled(false);
		mCallAudio = (LinearLayout) findViewById(R.id.layout_call_audio);
		mCallMute = (ImageView) findViewById(R.id.layout_callin_mute);
		mCallHandFree = (ImageView) findViewById(R.id.layout_callin_handfree);
		mVHangUp = (ImageButton) findViewById(R.id.layout_call_reject);
		mCallStateTips = (TextView) findViewById(R.id.layout_callin_duration);

		// call time
		mChronometer = (Chronometer) findViewById(R.id.chronometer);
		mVtalkNumber = (TextView) findViewById(R.id.layout_callin_number);
		// 键盘按钮
		mDiaerpadBtn = (ImageView) findViewById(R.id.layout_callin_diaerpad);
		mDiaerpad = (LinearLayout) findViewById(R.id.layout_diaerpad);

		mCallTransferBtn.setOnClickListener(this);
		mDiaerpadBtn.setOnClickListener(this);
		mCallMute.setOnClickListener(this);
		mCallHandFree.setOnClickListener(this);
		mVHangUp.setOnClickListener(this);

		setupKeypad();
		mDmfInput = (EditText) findViewById(R.id.dial_input_numer_TXT);

		// mCallStatus = (TextView) findViewById(R.id.call_status);

		mPause = (Button) findViewById(R.id.pause);
		mPause.setOnClickListener(this);
	}

	/**
	 * Read parameters or previously saved state of this activity.
	 */
	private void initialize() {
		Intent intent = getIntent();
		if (intent != null) {
			Bundle bundle = intent.getExtras();
			if (bundle == null) {
				finish();
				return;
			}
			mType = bundle.getString(CCPApplication.VALUE_DIAL_MODE);

			if (mType.equals(CCPApplication.VALUE_DIAL_MODE_FREE)) {
				// voip免费通话时显示voip账号
				mVoipAccount = bundle.getString(CCPApplication.VALUE_DIAL_VOIP_INPUT);
				if (mVoipAccount == null) {
					finish();
					return;
				}
				mVtalkNumber.setText(mVoipAccount);
			} else {
				// 直拨及回拨显示号码
				mPhoneNumber = bundle.getString(CCPApplication.VALUE_DIAL_VOIP_INPUT);
				mVtalkNumber.setText(mPhoneNumber);
			}
		}
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

	/**
	 * Initialize mode
	 * 
	 */
	private void initCall() {
		if (!checkeDeviceHelper()) {
			return;
		}
		try {
			if (mType.equals(CCPApplication.VALUE_DIAL_MODE_FREE)) {
				// voip免费通话
				if (mVoipAccount != null && !TextUtils.isEmpty(mVoipAccount)) {
					mCurrentCallId = getDeviceHelper().makeCall(Device.CallType.VOICE, mVoipAccount);
					Log4Util.d(CCPHelper.DEMO_TAG, "[CallOutActivity] VoIP calll, mVoipAccount " + mVoipAccount + " currentCallId " + mCurrentCallId);
				}
			} else if (mType.equals(CCPApplication.VALUE_DIAL_MODE_DIRECT)) {
				// 直拨
				mCurrentCallId = getDeviceHelper().makeCall(Device.CallType.VOICE, VoiceUtil.getStandardMDN(mPhoneNumber));
				Log4Util.d(CCPHelper.DEMO_TAG, "[CallOutActivity] Direct dial, mPhoneNumber " + mPhoneNumber + " currentCallId " + mCurrentCallId);
			} else if (mType.equals(CCPApplication.VALUE_DIAL_MODE_BACK)) {
				// 回拨
				getDeviceHelper().makeCallback(CCPConfig.Src_phone, mPhoneNumber, mPhoneNumber, CCPConfig.Src_phone);
				mCallAudio.setVisibility(View.GONE);
				Log4Util.d(CCPHelper.DEMO_TAG, "[CallOutActivity] call back, mPhoneNumber " + mPhoneNumber);
				return;
			} else {
				finish();
				return;
			}

			if (mCurrentCallId == null || mCurrentCallId.length() < 1) {
				CCPApplication.getInstance().showToast(R.string.no_support_voip);
				Log4Util.d(CCPHelper.DEMO_TAG, "[CallOutActivity] Sorry, " + getString(R.string.no_support_voip) + " , Call failed. ");
				finish();
				return;
			}

		} catch (Exception e) {
			finish();
			Log4Util.d(CCPHelper.DEMO_TAG, "[CallOutActivity] Sorry, call failure leads to an unknown exception, please try again. ");
			e.printStackTrace();
		}
	}

	boolean isCallPause = false;

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.pause:
			if (checkeDeviceHelper() && !TextUtils.isEmpty(mCurrentCallId)) {
				if (isCallPause) {
					getDeviceHelper().resumeCall(mCurrentCallId);
					isCallPause = false;

				} else {

					getDeviceHelper().pauseCall(mCurrentCallId);
					isCallPause = true;
				}

				mPause.setText(isCallPause ? "resume" : "pause");

			}
			break;
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

		// keybad end ...
		case R.id.layout_call_reject:
			doHandUpReleaseCall();
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
		case R.id.layout_callin_transfer: // select voip ...
			Intent intent = new Intent(this, GetNumberToTransferActivity.class);
			startActivityForResult(intent, REQUEST_CODE_CALL_TRANSFER);

			break;
		default:
			break;
		}
	}

	@Override
	protected void doHandUpReleaseCall() {
		super.doHandUpReleaseCall();
		// 挂断电话
		Log4Util.d(CCPHelper.DEMO_TAG, "[CallOutActivity] Voip talk hand up, CurrentCallId " + mCurrentCallId);
		try {
			if (mCurrentCallId != null && checkeDeviceHelper()) {
				getDeviceHelper().releaseCall(mCurrentCallId);
				stopVoiceRecording(mCurrentCallId);
			}

			// for XINWEI
			getBaseHandle().postDelayed(new Runnable() {

				@Override
				public void run() {
					if (!isConnect) {
						finish();
					}

				}
			}, 1000);
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
		if (checkeDeviceHelper()) {
			if (isMute) {
				getDeviceHelper().setMute(!isMute);
			}
			if (isHandsfree) {
				getDeviceHelper().enableLoudsSpeaker(!isHandsfree);
			}
		}
		releaseWakeLock();
		if (mVHangUp != null) {
			mVHangUp = null;
		}
		if (mCallAudio != null) {
			mCallAudio = null;
		}
		if (mCallStateTips != null) {
			mCallStateTips = null;
		}
		if (mVtalkNumber != null) {
			mVtalkNumber = null;
		}
		if (mCallMute != null) {
			mCallMute = null;
		}
		if (mCallHandFree != null) {
			mCallHandFree = null;
		}
		if (mDiaerpadBtn != null) {
			mDiaerpadBtn = null;
		}
		mPhoneNumber = null;
		mVoipAccount = null;
		mCurrentCallId = null;
		if (getCallHandler() != null) {
			setCallHandler(null);
		}
		CCPApplication.getInstance().setAudioMode(AudioManager.MODE_NORMAL);
		if (mCurrentCallId != null) {
			stopVoiceRecording(mCurrentCallId);
		}
		super.onDestroy();
	}

	public boolean onKeyDown(int keyCode, KeyEvent event) {
		// 屏蔽返回键
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}

	@Override
	protected void onCallAlerting(String callid) {
		super.onCallAlerting(callid);
		// 连接到对端用户，播放铃音
		Log4Util.d(CCPHelper.DEMO_TAG, "[CallOutActivity] voip alerting!!");
		if (callid != null && callid.equals(mCurrentCallId)) {
			mCallStateTips.setText(getString(R.string.voip_calling_wait));
		}
	}

	@Override
	protected void onCallProceeding(String callid) {
		super.onCallProceeding(callid);
		// 连接到服务器
		Log4Util.d(CCPHelper.DEMO_TAG, "[CallOutActivity] voip on call proceeding!!");
		if (callid != null && callid.equals(mCurrentCallId)) {
			mCallStateTips.setText(getString(R.string.voip_call_connect));
		}
		
	}

	@Override
	protected void onCallAnswered(String callid) {
		super.onCallAnswered(callid);
		// 对端应答
		Log4Util.d(CCPHelper.DEMO_TAG, "[CallOutActivity] voip on call answered!!");
		if (callid != null && callid.equals(mCurrentCallId)) {
			isConnect = true;
			mCallMute.setEnabled(true);
			initCallTools();
			// 2013/09/23
			// Show call state position is used to display the packet loss rate
			// and delay
			// mCallStateTips.setVisibility(View.GONE);
			mChronometer.setBase(SystemClock.elapsedRealtime());
			mChronometer.setVisibility(View.VISIBLE);
			mChronometer.start();

			mCallStateTips.setText("");
			if (getCallHandler() != null) {
				getCallHandler().sendMessage(getCallHandler().obtainMessage(VideoActivity.WHAT_ON_CODE_CALL_STATUS));
			}

			//startVoiceRecording(callid);
		}
	}

	@Override
	protected void onCallReleased(String callid) {
		super.onCallReleased(callid);
		// 远端挂断，本地挂断在onClick中处理
		Log4Util.d(CCPHelper.DEMO_TAG, "[CallOutActivity] voip on call released!!");
		if (callid != null && callid.equals(mCurrentCallId)) {
			stopVoiceRecording(callid);
			finishCalling();
		}
	}

	@Override
	protected void onMakeCallFailed(String callid, Reason reason) {
		super.onMakeCallFailed(callid, reason);
		// 发起通话失败
		Log4Util.d(CCPHelper.DEMO_TAG, "[CallOutActivity] voip on call makecall failed!!");
		if (callid != null && callid.equals(mCurrentCallId)) {
			finishCalling(reason);
		}
	}

	@Override
	protected void onCallback(int status, String self, String dest) {
		super.onCallback(status, self, dest);
		// 回拨通话回调
		Log4Util.d(CCPHelper.DEMO_TAG, "[CallOutActivity] voip on callback status : " + status);
		if (status == 170010) {
			mCallStateTips.setText(getString(R.string.voip_call_back_connect));
		} else {
			getCallHandler().postDelayed(finish, 5000);
			if (status == 0) {
				mCallStateTips.setText(getString(R.string.voip_call_back_success));
			} else if (status == 121002) {
				mCallStateTips.setText(getString(R.string.voip_call_fail_no_cash));
			} else {
				mCallStateTips.setText(getString(R.string.voip_call_fail));
			}
			mVHangUp.setClickable(false);
			mVHangUp.setBackgroundResource(R.drawable.call_interface_non_red_button);
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
			CallStatisticsInfo callStatistics = getDeviceHelper().getCallStatistics(Device.CallType.VOICE);

			NetworkStatistic trafficStats = null;
			if (mCallType == CallType.VOICE) {
				trafficStats = getDeviceHelper().getNetworkStatistic(mCurrentCallId);
			}

			if (callStatistics != null) {
				int fractionLost = callStatistics.getFractionLost();
				int rttMs = callStatistics.getRttMs();
				if (mCallStateTips != null) {
					if (trafficStats != null) {
						mCallStateTips.setText(getString(R.string.str_call_traffic_status, rttMs, (fractionLost *100 / 255), trafficStats.getTxBytes() / 1024,
								trafficStats.getRxBytes() / 1024));
					} else {

						mCallStateTips.setText(getString(R.string.str_call_status, rttMs, (fractionLost / 255)));
					}
				}
			}

			if (getCallHandler() != null) {
				Message callMessage = getCallHandler().obtainMessage(VideoActivity.WHAT_ON_CODE_CALL_STATUS);
				getCallHandler().sendMessageDelayed(callMessage, 1000);
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

	/**
	 * 根据状态,修改按钮属性及关闭操作
	 * 
	 * @param reason
	 */
	private void finishCalling(Reason reason) {
		try {
			isConnect = false;
			mChronometer.stop();
			mChronometer.setVisibility(View.GONE);
			mCallStateTips.setVisibility(View.VISIBLE);

			mCallHandFree.setClickable(false);
			mCallMute.setClickable(false);
			mVHangUp.setClickable(false);
			mDiaerpadBtn.setClickable(false);
			mDiaerpadBtn.setImageResource(R.drawable.call_interface_diaerpad);
			mCallHandFree.setImageResource(R.drawable.call_interface_hands_free);
			mCallMute.setImageResource(R.drawable.call_interface_mute);
			mVHangUp.setBackgroundResource(R.drawable.call_interface_non_red_button);
			getCallHandler().postDelayed(finish, 3000);
			// 处理通话结束状态
			if (reason == Reason.DECLINED || reason == Reason.BUSY) {
				mCallStateTips.setText(getString(R.string.voip_calling_refuse));
				getCallHandler().removeCallbacks(finish);
			} else {
				if (reason == Reason.CALLMISSED) {
					mCallStateTips.setText(getString(R.string.voip_calling_timeout));
				} else if (reason == Reason.MAINACCOUNTPAYMENT) {
					mCallStateTips.setText(getString(R.string.voip_call_fail_no_cash));
				} else if (reason == Reason.UNKNOWN) {
					mCallStateTips.setText(getString(R.string.voip_calling_finish));
				} else if (reason == Reason.NOTRESPONSE) {
					mCallStateTips.setText(getString(R.string.voip_call_fail));
				} else if (reason == Reason.VERSIONNOTSUPPORT) {
					mCallStateTips.setText(getString(R.string.str_voip_not_support));
				} else if (reason == Reason.OTHERVERSIONNOTSUPPORT) {
					mCallStateTips.setText(getString(R.string.str_other_voip_not_support));
				} else {

					ThirdSquareError(reason);
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/**
	 * Increased Voip P2P, direct error code:
	 * <p>
	 * Title: ThirdSquareError
	 * </p>
	 * <p>
	 * Description:
	 * </p>
	 * 
	 * @param reason
	 */
	private void ThirdSquareError(Reason reason) {
		if (reason == Reason.AUTHADDRESSFAILED) {// -- 700 第三方鉴权地址连接失败
			mCallStateTips.setText(getString(R.string.voip_call_fail_connection_failed_auth));
		} else if (reason == Reason.MAINACCOUNTPAYMENT) {// -- 701 第三方主账号余额不足

			mCallStateTips.setText(getString(R.string.voip_call_fail_no_pay_account));
		} else if (reason == Reason.MAINACCOUNTINVALID) { // -- 702 第三方应用ID未找到

			mCallStateTips.setText(getString(R.string.voip_call_fail_not_find_appid));
		} else if (reason == Reason.CALLERSAMECALLED) {// -- 704
														// 第三方应用未上线限制呼叫已配置测试号码

			mCallStateTips.setText(getString(R.string.voip_call_fail_not_online_only_call));
		} else if (reason == Reason.SUBACCOUNTPAYMENT) {// -- 705 第三方鉴权失败，子账号余额

			mCallStateTips.setText(getString(R.string.voip_call_auth_failed));
		} else {
			mCallStateTips.setText(getString(R.string.voip_calling_network_instability));
		}
	}

	/**
	 * 用于挂断时修改按钮属性及关闭操作
	 */
	private void finishCalling() {
		try {
			if (isConnect) {
				// set Chronometer view gone..
				isConnect = false;
				mChronometer.stop();
				mChronometer.setVisibility(View.GONE);
				// 接通后关闭
				mCallStateTips.setVisibility(View.VISIBLE);
				mCallStateTips.setText(R.string.voip_calling_finish);
				getCallHandler().postDelayed(finish, 3000);
			} else {
				// 未接通，直接关闭
				finish();
			}
			mCallHandFree.setClickable(false);
			mCallMute.setClickable(false);
			mVHangUp.setClickable(false);
			mDiaerpadBtn.setClickable(false);
			mDiaerpadBtn.setImageResource(R.drawable.call_interface_diaerpad);
			mCallHandFree.setImageResource(R.drawable.call_interface_hands_free);
			mCallMute.setImageResource(R.drawable.call_interface_mute);
			mVHangUp.setBackgroundResource(R.drawable.call_interface_non_red_button);
		} catch (Exception e) {
			e.printStackTrace();
		}
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

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);

		Log4Util.d(CCPHelper.DEMO_TAG, "[SelectVoiceActivity] onActivityResult: requestCode=" + requestCode + ", resultCode=" + resultCode + ", data=" + data);

		// If there's no data (because the user didn't select a number and
		// just hit BACK, for example), there's nothing to do.
		if (requestCode != REQUEST_CODE_CALL_TRANSFER) {
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
}
