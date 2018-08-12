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
 */package com.voice.demo.tools;

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.ui.CCPHelper;

import android.annotation.TargetApi;
import android.content.Context;
import android.media.AudioManager;
import android.os.Build;
import android.text.TextUtils;

/**
 * 
* <p>Title: CCPAudioManager.java</p>
* <p>Description: //mAudioManager.setMode(AudioManager.MODE_IN_CALL); //2
		//mAudioManager.setMode(AudioManager.MODE_CURRENT); // -1
		//mAudioManager.setMode(AudioManager.MODE_INVALID); // -2
		//mAudioManager.setMode(AudioManager.MODE_NORMAL); // 0
		//mAudioManager.setMode(AudioManager.MODE_RINGTONE); // 1</p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-10-16
* @version 3.5
 */
@TargetApi(14)
public class CCPAudioManager {

	
	private static boolean isOpera = false;
	
	private static String BUILD_SUMSUNG = "SAMSUNG";
	private static boolean mIsAudioSpeakerOn;
	private static int mAudioMode;
	private static boolean mIsBluetoothScoOn;
	
	
	private static CCPAudioManager sInstance = null;
	
	public static CCPAudioManager getInstance () {
		
		if(sInstance == null) {
			sInstance = new CCPAudioManager();
		}
		
		return sInstance;
	}
	
	
	
	public CCPAudioManager() {
		
		// ..default 
		//BluetoothAdapter.getDefaultAdapter().getProfileConnectionState(1);
	}
	
	
	/**
	 * 
	* <p>Title: saveSpeakerState</p>
	* <p>Description: Save the audio settings</p>
	* @param mAudioManager
	 */
	private void saveSpeakerState(AudioManager mAudioManager) {
		
		if(!isOpera) {
			mIsAudioSpeakerOn = mAudioManager.isSpeakerphoneOn();
			mAudioMode = mAudioManager.getMode();
			mIsBluetoothScoOn = mAudioManager.isBluetoothScoOn();
			isOpera = true;
		}
		
	}

	/**
	 * 
	* <p>Title: resetSpeakerState</p>
	* <p>Description: Reset the system audio settings</p>
	* @param context
	 */
	public void resetSpeakerState(Context context ) {
		if(isOpera) {
			AudioManager mAudioManager = (AudioManager) context .getSystemService(Context.AUDIO_SERVICE);
			mAudioManager.setSpeakerphoneOn(mIsAudioSpeakerOn);
			mAudioManager.setMode(mAudioMode);
			mAudioManager.setBluetoothScoOn(mIsBluetoothScoOn);
			mAudioManager.stopBluetoothSco();
			isOpera = false;
		}
	}
	
	// android Earpiece  Speaker
	@TargetApi(11)
	public void switchSpeakerEarpiece(Context context , boolean isEarpiece) {
		
		AudioManager mAudioManager = (AudioManager) context .getSystemService(Context.AUDIO_SERVICE);
		
		// The AudioManager parameter before storage.
		saveSpeakerState(mAudioManager);
		Log4Util.d(CCPHelper.DEMO_TAG, "isSpeakerphoneOn() " + mIsAudioSpeakerOn + " ,mAudioMode " + mAudioMode);
		
		// For XIAOMI mobile, need to open the speaker model first, 
		// otherwise it will open the speaker failed
		if(!isEarpiece) {
			
			if(!mIsAudioSpeakerOn)
				mAudioManager.setSpeakerphoneOn(true); 
		} else {
			mAudioManager.setSpeakerphoneOn(false); 
		}
		
		if(!isEarpiece) {
			mAudioManager.setMode(AudioManager.MODE_NORMAL);  
            // Speaker On
		} else {
			if(Build.VERSION.SDK_INT > 11) {
				if(!TextUtils.isEmpty(Build.MANUFACTURER) && 
						Build.MANUFACTURER.toLowerCase().indexOf(BUILD_SUMSUNG.toLowerCase()) >= 0 ) {
					mAudioManager.setMode(AudioManager.MODE_IN_CALL);  
				}
				mAudioManager.setMode(AudioManager.MODE_IN_COMMUNICATION);
				Log4Util.v(CCPHelper.DEMO_TAG, "isSpeakerphoneOn() " + mAudioManager.isSpeakerphoneOn() + " ,isEarpiece " + isEarpiece + " , Mode " + mAudioManager.getMode());
				return ;
			}
			mAudioManager.setMode(AudioManager.MODE_IN_CALL);  
		}
		
		boolean speakerphoneOn = mAudioManager.isSpeakerphoneOn();
		Log4Util.v(CCPHelper.DEMO_TAG, "isSpeakerphoneOn() " + speakerphoneOn + " ,isEarpiece " + isEarpiece + " , Mode " + mAudioManager.getMode());
	}
}
