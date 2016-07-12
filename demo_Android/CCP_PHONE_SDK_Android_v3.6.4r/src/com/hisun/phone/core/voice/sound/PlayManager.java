package com.hisun.phone.core.voice.sound;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.content.Context;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Vibrator;
import android.text.TextUtils;

import com.hisun.phone.core.voice.CallControlManager;
import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.multimedia.MediaManager;
import com.hisun.phone.core.voice.util.AdaptationTools;
import com.hisun.phone.core.voice.util.Log4Util;

/**
 * @ClassName: PlayManager
 * @Description: 播放铃声及文件处理
 * @author liujingbo
 */
public class PlayManager {

	private static final String EXTNAME = ".ogg";//默认铃声扩展名
	AudioManager manager = null;
	//自定义铃声控制
	private MediaPlayer mMediaPlayer = null;
	private SoundPool.Listener listener = null;
	private SoundPool.Sound xSound = null;
	private Context context;
	private Handler mHandler;
	private boolean openspeaker;
	private static final long DURATIME = 60000l;

	public PlayManager(Context context) {
		this.context = context;
		this.mHandler = CallControlManager.initialize(context).getServiceHandler();
		this.manager = (AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
		if(this.manager != null) {
			openspeaker = manager.isSpeakerphoneOn();
		}
	}

	/**
	 * 播放音频
	 * @param streamType 播放流类型
	 * @param loop 是否循环
	 * @param delay 播放时间 大于0有效
	 * @param sound
	 * @param ener
	 */
	public void playMedia(boolean loop, final long duration, boolean vibrate, final SoundPool.Sound sound, SoundPool.Listener ener) {

		listener = ener;
		xSound = sound;
		int streamType = AudioManager.STREAM_VOICE_CALL;

		if(sound.path == null || sound.path.trim().length() <= 0)
			return;
		try {
			Log4Util.d("[PlayManager] playMedia duration " + duration + "ms  " + sound.path);
			Uri defalultUri = null;

			//manager.setStreamMute(AudioManager.STREAM_MUSIC , true);
			Log4Util.d(Device.TAG, "Sound file :" + getAudioFilePath(context, MediaManager.StockSound.INCOMING.getResName())) ;
			String audioFilePath = getAudioFilePath(context, MediaManager.StockSound.INCOMING.getResName());
			if(!sound.path.endsWith(audioFilePath)) {
				if(!AdaptationTools.DeviceIsMi108()) {
					closeSpeaker(manager);
				}
			}else {
				streamType = AudioManager.STREAM_RING;
				defalultUri = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_RINGTONE);
				openSpeaker(manager);
			}
			

			if (mMediaPlayer == null) {
				// 创建MediaPlayer对象并设置Listenero
				mMediaPlayer = new MediaPlayer();
				mMediaPlayer.setOnPreparedListener(new OnPreparedListener() {

					@Override
					public void onPrepared(MediaPlayer mp) {
						mp.start();
						long d = duration;
						if(d <= 0)
							d = DURATIME;
						mHandler.postDelayed(run, d);
					}
				});
				mMediaPlayer.setOnCompletionListener(new OnCompletionListener() {

					@Override
					public void onCompletion(MediaPlayer mp) {
						reset();
					}
				});
				
				float streamMaxVolume = manager.getStreamMaxVolume(AudioManager.STREAM_RING);
				float streamVolume = manager.getStreamVolume(AudioManager.STREAM_RING);
				
				
				mMediaPlayer.setScreenOnWhilePlaying(true);
				mMediaPlayer.setVolume((streamVolume/streamMaxVolume), (streamVolume/streamMaxVolume));
			} 
			
			// 复用MediaPlayer对象
			mMediaPlayer.reset();

			mMediaPlayer.setAudioStreamType(streamType);
			//mMediaPlayer.setDataSource(sound.path);
			if(defalultUri != null) {
				mMediaPlayer.setDataSource(context, defalultUri);
				
				// 仅仅在
				if(isVibrate()) {
					startVibrate(duration);
				}
			} else {
				mMediaPlayer.setDataSource(sound.path);
			}

			mMediaPlayer.setLooping(loop);
			mMediaPlayer.prepare();

		} catch (Exception e) {
			resetAudio();
			e.printStackTrace();
		}
		
	}

	/**
	* 关闭扬声器
	* @param audioManager
	*/
	private void closeSpeaker(AudioManager audioManager) {
		try {
			if (audioManager != null) {
				audioManager.setMode(AudioManager.MODE_IN_CALL);
				if (audioManager.isSpeakerphoneOn()) {
					audioManager.setSpeakerphoneOn(false);
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * 重置铃声信息，关闭铃声，震动并回调
	 * 自然关闭，未主动关闭铃声
	 */
	private void reset() {
		resetAudio();
		stopMedia();
		stopVibrate();
		if(listener != null && xSound != null) {
			listener.onPlayComplete(xSound.soundId);
			xSound = null;
		}
	}

	/**
	 * 关闭铃声
	 */
	public void stopMedia() {
		Log4Util.d("[PlayManager]: media stop!");
		stopVibrate();
		try {
			if (mMediaPlayer != null && mMediaPlayer.isPlaying()) {
				mMediaPlayer.stop();
			}
			if (mMediaPlayer != null) {
				mMediaPlayer.release();
				mMediaPlayer = null;
			}
			resetAudio();
			if(mHandler != null) {
				mHandler.removeCallbacks(run);
			}
		} catch (IllegalStateException e) {
			e.printStackTrace();
		}

	}


	public boolean isPlaying() {
		if(mMediaPlayer != null && mMediaPlayer.isPlaying())
			return true;
		return false;
	}
	
	public boolean isVibrate() {

        int ringerMode = manager.getRingerMode();
        if ((ringerMode == AudioManager.RINGER_MODE_VIBRATE)) {
            return true;
        }
        
        return false;
	}

	private Runnable run = new Runnable() {

		@Override
		public void run() {
			reset();
		}
	};

	public static void playOgg(Context context, String path) {
		Uri soundUri = Uri.parse(path);
		if (soundUri != null) {
			final Ringtone sfx = RingtoneManager.getRingtone(context, soundUri);
			if (sfx != null) {
				sfx.setStreamType(AudioManager.STREAM_SYSTEM);
				sfx.play();
			} else {
				Log4Util.d("playSounds: failed to load ringtone from uri: " + soundUri);
			}
		} else {
			Log4Util.d("playSounds: could not parse Uri: " + soundUri);
		}
	}


	public static boolean saveAudioFile(Context context, String filename) throws IOException{
		String path = getAudioFilePath(context, filename);
		File file = new File(path);
		if(!file.exists() || file.length() <= 0) {
			try {
				return createAudioFile(context, file.getName(), file.getName());
			} catch (Exception e) {
				throw new IOException("save filename IOException");
			}
		}
		return true;
	}

	public static String getAudioFilePath(Context context, String filename) {
		return context.getFilesDir()+"/"+filename+EXTNAME;
	}

	public static boolean isExistFile(String path) {
		File file = new File(path);
		return file.exists();
	}

	private static boolean createAudioFile(Context context, String filename,
			String path) throws IOException {
		FileOutputStream fos = null;
		BufferedInputStream bis = null;
		boolean success = false;
		byte[] buffer = new byte[1024];
		int size = -1;
		try {
			// fos = new FileOutputStream("/sdcard/temp/sound.ogg");

			fos = context.openFileOutput(path, Context.MODE_WORLD_READABLE);
			InputStream inStream = context.getAssets().open(filename);
			if (inStream != null) {
				bis = new BufferedInputStream(inStream);
				while ((size = bis.read(buffer)) != -1) {
					fos.write(buffer, 0, size);
				}
				success = true;
			}
		} catch (FileNotFoundException e) {
			e.printStackTrace();
			throw new IOException("filename not found!!");
		} catch (IOException e) {
			e.printStackTrace();
			throw new IOException("filename write IOException!!");
		}  catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (bis != null) {
				bis.close();
			}
			if (fos != null) {
				fos.close();
			}
		}
		return success;
	}

	/**
	 * 打开扬声器
	 * @param audioManager
	 */
	private void openSpeaker(AudioManager audioManager) {

		try{
			if(audioManager != null) {
				audioManager.setMode(AudioManager.MODE_NORMAL);
				if(!audioManager.isSpeakerphoneOn()) {
					audioManager.setSpeakerphoneOn(true);
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}


	/**
	 * 恢复AudioManager
	 */
	private void resetAudio() {
		if(manager != null) {
			
			if(!TextUtils.isEmpty(Build.MANUFACTURER) && 
					Build.MANUFACTURER.toLowerCase().indexOf("XINWEI".toLowerCase()) >= 0 ) {
				manager.setSpeakerphoneOn(openspeaker);
				//manager.setStreamMute(AudioManager.STREAM_MUSIC , false);
				return ;
			}
			
			manager.setMode(AudioManager.MODE_NORMAL);
			manager.setSpeakerphoneOn(openspeaker);
			//manager.setStreamMute(AudioManager.STREAM_MUSIC , false);
		}
	}


	/**
	 * 振铃
	 */
	private Vibrator vi= null;
	public void startVibrate(long duration) {
		if(vi == null) {
			vi = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
		}
		long[] pattern = {1000,1000,1000,1000}; // OFF/ON/OFF/ON...
		vi.vibrate(pattern, 0);
	}

	public void stopVibrate() {
		if (vi != null) {
			vi.cancel();
			vi = null;
		}
	}

}

