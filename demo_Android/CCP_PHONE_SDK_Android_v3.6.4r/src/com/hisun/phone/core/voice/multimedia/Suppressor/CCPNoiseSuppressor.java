package com.hisun.phone.core.voice.multimedia.Suppressor;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.util.Log4Util;

import android.annotation.TargetApi;
import android.media.AudioRecord;
import android.media.audiofx.NoiseSuppressor;
/**
 * 
 * @author Jorstin Chan
 * @version 3.3
 */
@TargetApi(16)
public class CCPNoiseSuppressor implements Suppressor {

	private NoiseSuppressor mNoiseSuppressor = null;
	
	@TargetApi(16)
	public CCPNoiseSuppressor(AudioRecord audioRecord){
		if(NoiseSuppressor.isAvailable()) {
			mNoiseSuppressor = NoiseSuppressor.create(audioRecord.getAudioSessionId());
		}
	}
	@Override
	public void setEnable() {
		try {
			if(mNoiseSuppressor != null ) {
				int enabled = mNoiseSuppressor.setEnabled(true);
				if(enabled != NoiseSuppressor.SUCCESS) {
					Log4Util.i(Device.TAG, "[CCPNoiseSuppressor - setEnable] CCPNoiseSuppressor setEnable failed " + enabled);
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
			Log4Util.v(Device.TAG, "[CCPNoiseSuppressor - setEnable] Enable Error " + e.getLocalizedMessage());
		}
	}

	@Override
	public boolean isAvailable() {
		return NoiseSuppressor.isAvailable();
	}

}
