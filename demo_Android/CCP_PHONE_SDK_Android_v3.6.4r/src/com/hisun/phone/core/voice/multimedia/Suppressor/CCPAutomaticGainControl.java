package com.hisun.phone.core.voice.multimedia.Suppressor;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.util.Log4Util;

import android.annotation.TargetApi;
import android.media.AudioRecord;
import android.media.audiofx.AutomaticGainControl;
import android.media.audiofx.NoiseSuppressor;
/**
 * 
 * @author Jorstin Chan
 * @version 3.3
 */
@TargetApi(16)
public class CCPAutomaticGainControl implements Suppressor {

	private AutomaticGainControl mAutomaticGainControl = null;
	
	@TargetApi(16)
	public CCPAutomaticGainControl(AudioRecord audioRecord){
		if(NoiseSuppressor.isAvailable()) {
			mAutomaticGainControl = AutomaticGainControl.create(audioRecord.getAudioSessionId());
		}
	}
	@Override
	public void setEnable() {
		try {
			if(mAutomaticGainControl != null ) {
				int enabled = mAutomaticGainControl.setEnabled(true);
				if(enabled != AutomaticGainControl.SUCCESS) {
					Log4Util.i(Device.TAG, "[CCPAutomaticGainControl - setEnable] CCPAutomaticGainControl setEnable failed " + enabled);
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
			Log4Util.v(Device.TAG, "[CCPAutomaticGainControl - setEnable] Enable Error " + e.getLocalizedMessage());
		}
	}

	@Override
	public boolean isAvailable() {
		return AutomaticGainControl.isAvailable();
	}

}
