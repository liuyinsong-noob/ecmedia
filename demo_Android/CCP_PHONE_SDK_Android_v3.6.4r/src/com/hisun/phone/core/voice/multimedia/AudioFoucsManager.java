package com.hisun.phone.core.voice.multimedia;

import com.hisun.phone.core.voice.multimedia.MediaPlayManager.AudioFocusLinstener;

import android.content.Context;
import android.media.AudioManager;

/**
 * 
* <p>Title: AudioFoucsManager.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2007</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-10-9
* @version 1.0
 */
public class AudioFoucsManager implements AudioFocusLinstener{

	private AudioManager mAudioManager;
	private Context context;
	
	private AudioManager.OnAudioFocusChangeListener mOnAudioFocusChangeListener = new OnAudioFoucsListener();
	
	public AudioFoucsManager(Context context) {
		
		this.context = context;
	}
	public boolean requestFocus() {
		
		if(this.mAudioManager == null && context != null ) {
			this.mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
		}
		
		// Audio broadcast request focus
		int result = mAudioManager.requestAudioFocus(mOnAudioFocusChangeListener,
		                             // Specifying audio stream
		                             AudioManager.STREAM_MUSIC,
		                             // Request short focus
		 AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);

		return result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED ? true: false;
	}
	
	public boolean clearFocus() {
		
		if(this.mAudioManager == null && context != null ) {
			this.mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
		}
		
		// The playback end, you need to release the audio focus
		int abandonAudioFocus = mAudioManager.abandonAudioFocus(mOnAudioFocusChangeListener);
		if(abandonAudioFocus == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
			return true;
		}
		return false;
	}
	
	
	/**
	 * 
	* <p>Title: AudioFoucsManager.java</p>
	* <p>Description: </p>
	* <p>Copyright: Copyright (c) 2007</p>
	* <p>Company: http://www.yuntongxun.com</p>
	* @author zhanjichun
	* @date 2013-10-9
	* @version 1.0
	 */
	class OnAudioFoucsListener implements AudioManager.OnAudioFocusChangeListener {

		@Override
		public void onAudioFocusChange(int focusChange) {
			
			// do change
		}
		
	}
}
