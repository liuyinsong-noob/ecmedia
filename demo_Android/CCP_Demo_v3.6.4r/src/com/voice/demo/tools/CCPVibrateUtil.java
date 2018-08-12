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
package com.voice.demo.tools;

import android.content.Context;
import android.content.res.Resources;
import android.os.SystemClock;
import android.os.Vibrator;

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.R;
import com.voice.demo.CCPApplication;

/**
 * 
* <p>Title: VibrateUtil.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-10-30
* @version 3.5
 */
public class CCPVibrateUtil {
	//使用方法
	//VibrateUtil.getInstace().doVibrate();
	
	
	private static CCPVibrateUtil instance;
	private Context mContext ;
	
	private boolean mVibrateOn = true ;//default true;
	private long[] mVibratePattern;
	private static final int VIBRATE_NO_REPEAT = -1;
	private Vibrator mVibrator;
	
	private long mVibratorSystemTime;
	
	
	private CCPVibrateUtil(){
		mContext = CCPApplication.getInstance().getApplicationContext();
		Resources r = mContext.getResources();
		initVibrationPattern(r);
	}
	
	public static CCPVibrateUtil getInstace(){
		if(instance == null){
			instance = new CCPVibrateUtil();
		}
		return instance;
	}
	
	/**
	 * 振动
	 */
	public void doVibrate(){
		vibrate();
	}
	
	
	
	/**
	 * Triggers haptic feedback (if enabled) for dialer key presses.
	 */
	private synchronized void vibrate() {
		Log4Util.d("vabrate");
		
		mVibrateOn = isVibrator();
		
		if (!mVibrateOn) {
			return;
		}
		if (mVibrator == null) {
			mVibrator = (Vibrator) /* new Vibrator(); */this.mContext
					.getSystemService(Context.VIBRATOR_SERVICE);
			Log4Util.d(".getSystemService(Context.VIBRATOR_SERVICE)");
		}
		mVibrator.vibrate(mVibratePattern, VIBRATE_NO_REPEAT);
	}

	/**
	 * Initialize the vibration parameters.
	 * 
	 * @param r
	 *            The Resources with the vibration parameters.
	 */
	private void initVibrationPattern(Resources r) {
		int[] pattern = null;
		try {
			//mVibrateOn = r.getBoolean(R.bool.config_enable_dialer_key_vibration);
			pattern = r.getIntArray(R.array.config_newImMessageVibePattern);
			if (null == pattern) {
				Log4Util.d("Vibrate pattern is null.");
				mVibrateOn = false;
			}
			if (mVibrator == null) {
				mVibrator = (Vibrator) /* new Vibrator(); */this.mContext
						.getSystemService(Context.VIBRATOR_SERVICE);
			}
		} catch (Resources.NotFoundException nfe) {
			Log4Util.d("Vibrate control bool or pattern missing." + nfe);
			mVibrateOn = false;
		}

		if (!mVibrateOn) {
			return;
		}

		// int[] to long[] conversion.
		mVibratePattern = new long[pattern.length];
		for (int i = 0; i < pattern.length; i++) {
			mVibratePattern[i] = pattern[i];
		}
	}
	
	/**
	 * 
	* <p>Title: isVibrator</p>
	* <p>Description: </p>
	* @return
	 */
	boolean isVibrator() {
		
		boolean isVibrator = false;
		long elapsedRealtime = SystemClock.elapsedRealtime();
		if((elapsedRealtime - mVibratorSystemTime) > 3000L) {
			isVibrator =  true;
		}
		if(mVibratorSystemTime == 0 ) {
			isVibrator = true;
		}
		
		if(isVibrator) {
			mVibratorSystemTime = elapsedRealtime;
		}
		
		return isVibrator;
	}
}
