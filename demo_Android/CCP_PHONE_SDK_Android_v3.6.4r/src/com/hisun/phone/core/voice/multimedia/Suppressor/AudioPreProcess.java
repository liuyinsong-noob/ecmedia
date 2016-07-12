package com.hisun.phone.core.voice.multimedia.Suppressor;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.VoiceUtil;

import android.media.AudioRecord;
import android.os.Build;
/**
 * 
 * @author Jorstin Chan
 * @version 3.3
 */
public class AudioPreProcess {
	
	private CCPAcousticEchoCanceler mEchoCanceler = null;
	private CCPAutomaticGainControl mGainControl = null;
	private CCPNoiseSuppressor mNoiseSuppressor = null;
	
	private volatile boolean isEnable = false;
	public boolean init(AudioRecord audioRecord ){
		Log4Util.i(Device.TAG, "[AudioPreProcess - init] The current SDK version number " + Build.VERSION.SDK_INT);
		if(VoiceUtil.isHeighVersion(16)) {
			if(audioRecord == null ) {
				Log4Util.i(Device.TAG, "[AudioPreProcess - init] AudioRecord is " + audioRecord);
			} else {
				mEchoCanceler = new CCPAcousticEchoCanceler(audioRecord);
				if(mEchoCanceler != null && mEchoCanceler.isAvailable() && !isEnable) {
					mEchoCanceler.setEnable();
					isEnable = true;
				}
				if(mGainControl != null && mGainControl.isAvailable() &&isEnable) {
					mGainControl.setEnable();
					isEnable = true;
				}
				if(mNoiseSuppressor != null && mNoiseSuppressor.isAvailable() && isEnable) {
					mNoiseSuppressor.setEnable();
					isEnable = true;
				}
			}
			
		}
		return isEnable;
	}
}
