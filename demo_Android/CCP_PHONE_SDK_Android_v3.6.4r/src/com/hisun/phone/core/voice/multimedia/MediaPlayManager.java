package com.hisun.phone.core.voice.multimedia;

import java.io.File;
import java.io.FileInputStream;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.DeviceImpl;
import com.hisun.phone.core.voice.util.Log4Util;

import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;

/**
 * 
* <p>Title: MediaPlayManager.java</p>
* <p>Description:
*    The October 9, 2013 update, there was time to switch the replacement of audio stream, 
*    It turns out the right way to do this is through the following code.
*    Play through the ear piece {@link MediaPlayer#setAudioStreamType(int)} , e.g setAudioStreamType (AudioManager.STREAM_VOICE_CALL);
*    Play through the speaker phone MediaPlayer.setAudioStreamType (AudioManager.STREAM_MUSIC);
*    That is it. Any other solution Getting audio to play through earpiece) does not work consistently across devices. 
*    In fact using MODE_IN_CALL  ensures that your audio will never play on certain devices. </p>
*    @see MediaPlayer#setAudioStreamType(int)
*    @see AudioManager#STREAM_MUSIC
*    @see AudioManager#STREAM_VOICE_CALL
* <p>Copyright: Copyright (c) 2007</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-10-9
* @version 3.4.1.1
 */
public final class MediaPlayManager {

    private static MediaPlayManager mInstance = null;
    
    private AudioFocusLinstener mAudioFoucsManager;
    
    public static final int ERROR = 0;

    private DeviceImpl listener;
    
    private String fileUrl = "";


    public enum State {
        NO_SOURCE, 
        IDLE, 
        PLAYING,
        STOP,
        COMPLETION
    }

    private State mState = State.NO_SOURCE;

    private MediaPlayer mPlayer = new MediaPlayer();

    
    synchronized public static MediaPlayManager getInstance() {
        if (null == mInstance) {
            mInstance = new MediaPlayManager();
        }
        return mInstance;
    }
    
    synchronized public boolean setSource(String path , boolean speakerOn) {
		this.mState = State.NO_SOURCE;

		
		if(TextUtils.isEmpty(path) || !new File(path).exists()) {
			Log4Util.d(Device.TAG, "Set file source failed, path " + path);
			return false;
		}
		
		fileUrl = path;
		
        if (null != this.mPlayer) {
            this.mPlayer.release();
            this.mPlayer = null;
        }
        this.mPlayer = new MediaPlayer();
        
        // add by zhanjc when switch Earpiece to Speaker
        // 2013.10.9
        mAudioFoucsManager.clearFocus();
        int streamtype = AudioManager.STREAM_MUSIC;
        if(!speakerOn)  {
        	streamtype = AudioManager.STREAM_VOICE_CALL;
        }
        this.mPlayer.reset();
        this.mPlayer.setAudioStreamType(streamtype);
        Log4Util.v(Device.TAG, "MediaPlay.streamtype  " + streamtype);
        try {
        	FileInputStream inputStream = new FileInputStream(new File(path));
        	this.mPlayer.setLooping(false);
        	this.mPlayer.setDataSource(inputStream.getFD());
            this.mPlayer.prepare();
            this.mState = State.IDLE;
            this.mPlayer.setOnCompletionListener(this.mMediaEndListener);
            return true;
        } catch (Exception e) {
          e.printStackTrace();
          Log4Util.i(Device.TAG, "[MediaPlayManager] playImp : failed, exception = " + e.getMessage());
        }
        return false;
    }

    public State getState() {
        return this.mState;
    }
    
    synchronized public boolean play(DeviceImpl listener) {
    	this.listener = listener;
    	//handler.sendEmptyMessageDelayed(1, 100);
        if (State.IDLE == this.mState||State.STOP == this.mState) {
        	
        	// request Focus ..
        	mAudioFoucsManager.requestFocus();
        	
            this.mPlayer.start();
            this.mState = State.PLAYING;
            return true;
        }
        
        Log4Util.e(Device.TAG, "[MediaPlayManager - play] play file[" + fileUrl+ "] failed . ");
        
        return false;
    }

    synchronized public boolean pause() {
        if (State.PLAYING == this.mState) {
            this.mPlayer.pause();
            this.mState = State.IDLE;
            return true;
        }
        return false;
    }
    
    synchronized public void stop(){
    	//this.listener = null;
    	if(this.mState == State.STOP 
    			|| this.mState == State.COMPLETION) {
    		return;
    	}
    	// 
    	mAudioFoucsManager.clearFocus();
    	this.mPlayer.stop();
    	
    	// 3.4.1.1
    	this.mPlayer.release();
    	this.mState = State.STOP;
    }
    
    synchronized public void release(){
    	if(mPlayer!=null){
    		this.mPlayer.stop();
        	this.mPlayer.release();
    	}
    	this.listener = null;
    	this.mPlayer = null;
    	this.mState = State.NO_SOURCE;
    }

    synchronized public boolean seek(final int position) {
        if (State.NO_SOURCE != this.mState) {
            this.mPlayer.seekTo(position);
            return true;
        }
        return false;
    }

    public int getCurrentPosition() {
        if (State.NO_SOURCE != this.mState) {
            return this.mPlayer.getCurrentPosition();
        }
        return ERROR;
    }

    public int getMediaLength() {
        if (State.NO_SOURCE != this.mState) {
            return this.mPlayer.getDuration();
        }
        return ERROR;
    }
    
    private final OnCompletionListener mMediaEndListener = 
            new OnCompletionListener() {
        @Override
        public void onCompletion(final MediaPlayer mp) {
            try {
				//mp.seekTo(0);
				MediaPlayManager.this.mState = State.COMPLETION;
				handler.sendEmptyMessage(0);
				mPlayer.release();
				mAudioFoucsManager.clearFocus();
			} catch (Exception e) {
				e.printStackTrace();
				Log4Util.v(Device.TAG, "[MediaPlayManager - setCompletion] File[" + fileUrl + "] ErrMsg[" + e.getStackTrace() + "]");
			}
        }
    };
    
	Handler handler = new Handler() {
		
		@Override
		public void handleMessage(Message msg) {
			try {
				if (listener != null && mState == State.COMPLETION) {
					listener.onFinishedPlaying();	
				}
			} catch (Exception e) {
				e.printStackTrace();
			}
			super.handleMessage(msg);
		}		
	};
	
	/**
	 * 
	* <p>Title: getListener</p>
	* <p>Description: </p>
	* @version 3.4.1.1
	* @return {@link DeviceImpl}
	* @see DeviceImpl
	 */
	public DeviceImpl getListener() {
		return listener;
	}

	/**
	 * 
	* <p>Title: setListener</p>
	* <p>Description: set listener for {@link MediaPlayManager},</p>
	* @version 3.4.1.1
	* @see MediaPlayer#setAudioStreamType(int)
	* @param listener
	 */
	public void setListener(DeviceImpl listener) {
		this.listener = listener;
	}
	
	/**
	 * 
	* <p>Title: setOnAudioFocusLinstener</p>
	* <p>Description: </p>
	* @param linstener
	 */
	public void setOnAudioFocusLinstener (AudioFocusLinstener linstener) {
	
		this.mAudioFoucsManager = linstener;
	}
	
	
	public interface AudioFocusLinstener {
		
		boolean requestFocus();
		boolean clearFocus();
	}
}
