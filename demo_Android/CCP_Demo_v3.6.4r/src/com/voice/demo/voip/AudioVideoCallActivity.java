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
 */package com.voice.demo.voip;

import java.io.File;
import java.lang.ref.WeakReference;

import com.yuntongxun.ecsdk.core.voip.ViEFilterRenderView;
import com.yuntongxun.ecsdk.core.voip.ViERenderer;

import android.app.KeyguardManager;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.Toast;

import com.CCP.phone.CameraCapbility;
import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.Device.CallType;
import com.hisun.phone.core.voice.Device.Rotate;
import com.hisun.phone.core.voice.DeviceListener.Reason;
import com.hisun.phone.core.voice.exception.CCPRecordException;
import com.hisun.phone.core.voice.listener.OnVoIPListener;
import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.R;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.tools.CCPNotificationManager;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.ui.CCPHelper;

import static java.lang.Math.abs;

/**
 * 
* <p>Title: AudioVideoCallActivity.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.cloopen.com</p>
* @author Jorstin Chan
* @date 2013-11-15
* @version 3.5
 */
public class AudioVideoCallActivity extends CCPBaseActivity implements OnVoIPListener.OnCallRecordListener
																		{

	public static final int WHAT_ON_CODE_CALL_STATUS = 11;
	public static final int WHAT_ON_SHOW_LOCAL_SURFACEVIEW = 12;
	
	public static String mCurrentCallId;
	
	protected boolean isConnect = false;
	
	protected boolean isIncomingCall = false;
	
	protected boolean callRecordEnabled;
	// video 
	protected CallType mCallType;
	
	// Local Video
	public RelativeLayout mLoaclVideoView;
	
	protected RelativeLayout.LayoutParams layoutParams =  null;
	
	public static ImageView mCallTransferBtn;
	// 静音按钮
	public ImageView mCallMute;
	// 免提按钮
	public ImageView mCallHandFree;
	protected SeekBar mSeekBar;
	
	// The first rear facing camera
	public  int defaultCameraId;
	
	public int cameraCurrentlyLocked;
	
	public int mCameraCapbilityIndex;
	
	// 是否静音
	public boolean isMute = false;
	// 是否免提
	public boolean isHandsfree = false;
	
	public VideoCallHandle mVideoCallHandle;
	
	private KeyguardManager.KeyguardLock mKeyguardLock = null;
	private KeyguardManager mKeyguardManager = null;
	private PowerManager.WakeLock mWakeLock;
	protected int scale;
	private CameraCapbility[] cameraCapbilities;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);
		
		super.onCreate(savedInstanceState);
		
		mVideoCallHandle = new VideoCallHandle(this);
		CCPHelper.getInstance().setHandler(mVideoCallHandle);
		
		layoutParams = new RelativeLayout.LayoutParams(
				RelativeLayout.LayoutParams.MATCH_PARENT,
				RelativeLayout.LayoutParams.MATCH_PARENT);
		
	}
	
	public void enterIncallMode() {
		if (!(mWakeLock.isHeld())) {
			// wake up screen
			// BUG java.lang.RuntimeException: WakeLock under-locked
			mWakeLock.setReferenceCounted(false);
			mWakeLock.acquire();
		}
		mKeyguardLock = this.mKeyguardManager.newKeyguardLock("");
		mKeyguardLock.disableKeyguard();
	}
	
	public void initProwerManager() {
		mWakeLock = ((PowerManager) getSystemService("power")).newWakeLock(
				PowerManager.SCREEN_DIM_WAKE_LOCK | PowerManager.ACQUIRE_CAUSES_WAKEUP , "CALL_ACTIVITY#" + super.getClass().getName());
		mKeyguardManager = ((KeyguardManager) getSystemService("keyguard"));
	}
	
	public void releaseWakeLock() {
		try {
			if (this.mWakeLock.isHeld()) {
				if (this.mKeyguardLock != null) {
					this.mKeyguardLock.reenableKeyguard();
					this.mKeyguardLock = null;
				}
				this.mWakeLock.release();
			}
			return;
		} catch (Exception localException) {
			Log4Util.e("AndroidRuntime", localException.toString());
		}
	}


	@Override
	protected void onResume() {
		super.onResume();
		
		//lockScreen();

		if (mCallType == Device.CallType.VIDEO && checkeDeviceHelper()) {
			cameraCurrentlyLocked = defaultCameraId;
			
			getDeviceHelper().selectCamera(cameraCurrentlyLocked, mCameraCapbilityIndex, 15, Rotate.Rotate_Auto, true);
		}
		CCPNotificationManager.cancleCCPNotification(this,
				CCPNotificationManager.CCP_NOTIFICATOIN_ID_CALLING);

		callRecordEnabled = CcpPreferences.getSharedPreferences().getBoolean(
				CCPPreferenceSettings.SETTING_CALL_RECORDING.getId(),
				(Boolean) CCPPreferenceSettings.SETTING_CALL_RECORDING.getDefaultValue());

		if (checkeDeviceHelper()) {
			getDeviceHelper().setOnCallRecordListener(this);
		}
	}
	
	@Override
	protected void onPause() {
		super.onPause();
		//releaseLockScreen();
	}
	
	@Override
	protected void onStop() {
		super.onStop();
	
		if(isConnect) {
			if(isIncomingCall) {
				CCPNotificationManager.showInCallingNotication(
						getApplicationContext(), mCallType,
						getString(R.string.voip_is_talking_tip), null);
				
			} else {
				CCPNotificationManager.showOutCallingNotication(
						getApplicationContext(), mCallType,
						getString(R.string.voip_is_talking_tip), null);
				
			}
		}
		
	}
	
	
	public void DisplayLocalSurfaceView() {
		boolean use_vie_filter_render = true;
		if(mCallType == Device.CallType.VIDEO && mLoaclVideoView != null 
				&& mLoaclVideoView.getVisibility() == View.VISIBLE) {
			// Create a RelativeLayout container that will hold a SurfaceView,
	        // and set it as the content of our activity.
			if(use_vie_filter_render) {
				ViEFilterRenderView localView = ViEFilterRenderView.createFilterRenderer(this);
				CameraCapbility cap = cameraCapbilities[mCameraCapbilityIndex];
				localView.setImageFrameSize(cap.width, cap.height);
				localView.setLayoutParams(layoutParams);
				localView.setZOrderOnTop(true);
				mLoaclVideoView.removeAllViews();
				mLoaclVideoView.setBackgroundColor(getResources().getColor(R.color.white));
				mLoaclVideoView.addView(localView);
			} else {
				SurfaceView localView = ViERenderer.CreateLocalRenderer(this);
				localView.setLayoutParams(layoutParams);
				localView.setZOrderOnTop(true);
				mLoaclVideoView.removeAllViews();
				mLoaclVideoView.setBackgroundColor(getResources().getColor(R.color.white));
				mLoaclVideoView.addView(localView);
			}
		}
	}
	
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		CCPNotificationManager.cancleCCPNotification(this, CCPNotificationManager.CCP_NOTIFICATOIN_ID_CALLING);
	}
	
	
	@Override
	protected void onReceiveBroadcast(Intent intent) {
		super.onReceiveBroadcast(intent);
		if(intent.getAction().equals(CCPIntentUtils.INTENT_P2P_ENABLED)){
			setDuration(-1);
			addNotificatoinToView(getString(R.string.str_p2p_enable)/*, Gravity.TOP*/);
		}/*else if (intent != null && CCPIntentUtils.INTENT_DISCONNECT_CCP.equals(intent.getAction())) {
			
			// If disconnection with cloud communication server port, then hang up the local call
			doHandUpReleaseCall();
			CCPApplication.getInstance().showToast(R.string.ccp_http_err_voip);
		} */
	}
	
	/**
	 * 
	* <p>Title: doHandUpReleaseCall</p>
	* <p>Description: </p>
	 */
	protected void doHandUpReleaseCall(){}

	@Override
	protected int getLayoutId() {
		return -1;
	}
	
	/**
	 * Callback return {@link VideoCallHandle}
	 * @return
	 */
	public VideoCallHandle getCallHandler() {
		return mVideoCallHandle;
	}
	
	public void setCallHandler(VideoCallHandle handler){
		mVideoCallHandle = handler;
	}

	/**
	 * @param callid
	 */
	public void startVoiceRecording(String callid) {
		if(getDeviceHelper() != null && callRecordEnabled) {
			if(!CCPUtil.isExistExternalStore()) {
				return;
			}
			File callRecordFile = CCPUtil.createCallRecordFilePath(callid, "wav");
			if(callRecordFile != null) {
				
				try {
					getDeviceHelper().startVoiceCallRecording(callid, callRecordFile.getAbsolutePath());
					Toast.makeText(getApplicationContext(), R.string.str_call_record_start, Toast.LENGTH_LONG).show();
				} catch (CCPRecordException e) {
					e.printStackTrace();
				}
			}
		}
	}
	
	public void initCallTools() {
		if(checkeDeviceHelper()) {
			try {
				isMute = getDeviceHelper().getMuteStatus();
				isHandsfree = getDeviceHelper().getLoudsSpeakerStatus();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}
	
	/**
	 * 设置呼叫转接
	 */
	public int setCallTransfer(String mCurrentCallId,String transToNumber) {
		if(checkeDeviceHelper()) {
			int transferCall = getDeviceHelper().transferCall(mCurrentCallId,transToNumber);
			if(transferCall==0){
				mCallMute.setImageResource(R.drawable.call_interface_mute_on);
			}
			else{
				//呼叫转接失败
				Toast.makeText(getApplicationContext(), "转接失败 返回码"+transferCall, 1).show();
			}
			return transferCall;
		}
		return -1;
	}
	/**
	 * 设置静音
	 */
	public void setMuteUI() {
		try {
			if(checkeDeviceHelper()) {
				getDeviceHelper().setMute(!isMute);
			}
			isMute = getDeviceHelper().getMuteStatus();
			
			if (isMute) {
				mCallMute.setImageResource(R.drawable.call_interface_mute_on);
			} else {
				mCallMute.setImageResource(R.drawable.call_interface_mute);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * 设置免提
	 */
	public void sethandfreeUI() {
		try {
			if(checkeDeviceHelper()) {
				getDeviceHelper().enableLoudsSpeaker(!isHandsfree);
				isHandsfree = getDeviceHelper().getLoudsSpeakerStatus();
			}
			
			if (isHandsfree) {
				mCallHandFree.setImageResource(R.drawable.call_interface_hands_free_on);
			} else {
				mCallHandFree.setImageResource(R.drawable.call_interface_hands_free);
			}

		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	public void stopVoiceRecording(String callid) {
		if(getDeviceHelper() != null && callRecordEnabled) {
			getDeviceHelper().stopVoiceCallRecording(callid);
		}
	}
	
	@Override
	public void onCallRecordDone(String filePath) {
		Toast.makeText(getApplicationContext(), getString(R.string.str_call_record_success, filePath), Toast.LENGTH_LONG).show();
	}

	/**
	 * The callback of calls recording throws an error, reporting events.
	 * @param reason recording status: 
	 * 		  0: Success 
	 *        -1: Recording failed then delete the recording file 
	 *        -2: Recording failed of write file but still to retain the record file.
	 */
	@Override
	public void onCallRecordError(int reason) {
		switch (reason) {
		case -1:
			Toast.makeText(getApplicationContext(), getString(R.string.str_call_record_error_2, reason), Toast.LENGTH_LONG).show();
			break;
		case -2:
			Toast.makeText(getApplicationContext(), getString(R.string.str_call_record_error_1, reason), Toast.LENGTH_LONG).show();
			break;
		default:
			Toast.makeText(getApplicationContext(), getString(R.string.str_call_record_error_0, reason), Toast.LENGTH_LONG).show();
			break;
		}
	}
	
	protected void onCallAlerting(String callid) {}
	protected void onCallAnswered(String callid) {}
	protected void onCallProceeding(String callid) {}
	protected void onCallReleased(String callid) {}
	@Deprecated protected void onCallVideoRatioChanged(String callid, String resolution) {}
	protected void onCallVideoRatioChanged(String callid, int width , int height) {}
	protected void onMakeCallFailed(String callid, Reason reason) {}
	protected void onCallback(int status, String self, String dest) { }
	
	public static class VideoCallHandle extends Handler {
		
		WeakReference<AudioVideoCallActivity> mActivity;
		public VideoCallHandle(AudioVideoCallActivity activity) {
			mActivity = new WeakReference<AudioVideoCallActivity>(activity);
		}
		
		@Override
		public void handleMessage(Message msg) {
			super.handleMessage(msg);
			AudioVideoCallActivity mCallActivity = mActivity.get();
			
			if(mCallActivity == null ) {
				return;
			}
			String callid = null;
			Reason reason = Reason.UNKNOWN;
			Bundle b = null;
			// 获取通话ID
			if(msg.obj instanceof String ){
				callid = (String)msg.obj;
			} else if (msg.obj instanceof Bundle){
				b = (Bundle) msg.obj;
				
				if(b.containsKey(Device.CALLID)) {
					callid = b.getString(Device.CALLID);
				}
				
				if(b.containsKey(Device.REASON)) {
					try {
						reason = (Reason) b.get(Device.REASON);
					} catch (Exception e) {
						Log.e(this.getClass().getName(), "b.get(Device.REASON)");
					}
				}
			}
			switch (msg.what) {
			case CCPHelper.WHAT_ON_CALL_ALERTING:
				mCallActivity.onCallAlerting(callid);
				break;
			case CCPHelper.WHAT_ON_CALL_PROCEEDING:
				mCallActivity.onCallProceeding(callid);
				break;
			case CCPHelper.WHAT_ON_CALL_ANSWERED:
				mCallActivity.onCallAnswered(callid);
				if(mCallTransferBtn!=null)
				mCallTransferBtn.setEnabled(true);
				break;
				
			case CCPHelper.WHAT_ON_CALL_RELEASED:
				mCallActivity.onCallReleased(callid);
//				mCurrentCallId=null;
				if(mCallTransferBtn!=null)
				mCallTransferBtn.setEnabled(false);
				break;
			case CCPHelper.WHAT_ON_CALL_MAKECALL_FAILED:
				mCallActivity.onMakeCallFailed(callid, reason);
				break;
			case WHAT_ON_CODE_CALL_STATUS:
			case CCPHelper.WHAT_ON_RECEIVE_SYSTEM_EVENTS:
				mCallActivity.handleNotifyMessage(msg);
				break;
			case CCPHelper.WHAT_ON_CALLVIDEO_RATIO_CHANGED:
				if(b.containsKey("width") && b.containsKey("height")) {
					int width = b.getInt("width");
					int height = b.getInt("height");
					mCallActivity.onCallVideoRatioChanged(callid, width, height);
				}
				break;
			case CCPHelper.WHAT_ON_CALL_BACKING:
				
				if(b == null) {
					return;
				}
				int status = -1;
				if (b.containsKey(Device.CBSTATE)) {
					status = b.getInt(Device.CBSTATE);
				}
				String self = "";
				if (b.containsKey(Device.SELFPHONE)) {
					self = b.getString(Device.SELFPHONE);
				}
				String dest = "";
				if (b.containsKey(Device.DESTPHONE)) {
					dest = b.getString(Device.DESTPHONE);
				}
				mCallActivity.onCallback(status, self, dest);
				break;
			case CCPHelper.WHAT_ON_CALL_TRANSFERSTATESUCCEED:
				String callId = (String) msg.obj;
				if(mCurrentCallId.equals(callId)){
					Toast.makeText(mCallActivity, "呼转成功！", 1).show();
				}
				break;

			default:
				break;
			}
			
		}
	}
	
	/**
	 * @param caps
	 */
	public void comportCapbilityIndex(CameraCapbility[] caps) {
        if(caps == null ) {
            return;
        }
		cameraCapbilities = caps;
        int resolution =  CcpPreferences.getSharedPreferences().
                getInt(CCPPreferenceSettings.SETTING_VIDEO_CALL_RESOLUTION.getId(),
                        (Integer)CCPPreferenceSettings.SETTING_VIDEO_CALL_RESOLUTION.getDefaultValue());

		int pixel[] = new int[caps.length];
		for(CameraCapbility cap : caps) {
            if (cap.index >= pixel.length) {
                continue;
            }
            pixel[cap.index] = cap.width * cap.height;
        }

        int smallestDiff = 0xFFFFFF;
        int smallestDiffIndex = -1;
		for(int i = 0 ; i < caps.length ; i++) {
            int diff = abs(pixel[i] - resolution);
            if(diff < smallestDiff) {
                smallestDiff = diff;
                smallestDiffIndex = i;
            }
		}
        mCameraCapbilityIndex = smallestDiffIndex;
        Log4Util.i("comportCapbilityIndex w:"+caps[mCameraCapbilityIndex].width + " h:"+caps[mCameraCapbilityIndex].height);
	}
	
	/**
	 * 像素转化dip
	 * @param context
	 * @param pxValue
	 * @return
	 */
	public static int px2dip(Context context, float pxValue){

		final float scale = context.getResources().getDisplayMetrics().density;

		return (int)(pxValue / scale + 0.5f);

	}
	
	
	public int[] decodeDisplayMetrics() {
		int[] metrics = new int[2];
		DisplayMetrics displayMetrics =new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
		metrics[0] = displayMetrics.widthPixels;
		metrics[1] = displayMetrics.heightPixels;
		return metrics;
	}
}
