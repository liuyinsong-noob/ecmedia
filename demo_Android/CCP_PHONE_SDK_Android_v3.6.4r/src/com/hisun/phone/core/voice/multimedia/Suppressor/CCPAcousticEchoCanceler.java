package com.hisun.phone.core.voice.multimedia.Suppressor;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.util.Log4Util;

import android.annotation.TargetApi;
import android.media.AudioRecord;
import android.media.audiofx.AcousticEchoCanceler;
import android.media.audiofx.NoiseSuppressor;
/**
 * 
 * @author Jorstin Chan
 */
@TargetApi(16)
public class CCPAcousticEchoCanceler implements Suppressor {

	private AcousticEchoCanceler mAcousticEchoCanceler = null;
	
	@TargetApi(16)
	public CCPAcousticEchoCanceler(AudioRecord audioRecord){
		if(NoiseSuppressor.isAvailable()) {
			mAcousticEchoCanceler = AcousticEchoCanceler.create(audioRecord.getAudioSessionId());
		}
	}
	@Override
	public void setEnable() {
		try {
			if(mAcousticEchoCanceler != null ) {
				int enabled = mAcousticEchoCanceler.setEnabled(true);
				if(enabled != AcousticEchoCanceler.SUCCESS) {
					Log4Util.i(Device.TAG, "[CCPAcousticEchoCanceler - setEnable] CCPAcousticEchoCanceler setEnable failed " + enabled);
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
			Log4Util.v(Device.TAG, "[CCPAcousticEchoCanceler - setEnable] Enable Error " + e.getLocalizedMessage());
		}
	}

	@Override
	public boolean isAvailable() {
		return AcousticEchoCanceler.isAvailable();
	}

}
