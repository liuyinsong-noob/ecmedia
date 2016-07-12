package com.hisun.phone.core.voice.sound;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.Queue;

import android.content.Context;

import com.hisun.phone.core.voice.multimedia.MediaManager.StockSound;
import com.hisun.phone.core.voice.util.Log4Util;

/**
 * @ClassName: SoundPool
 * @Description: 通话铃声缓存池
 * @author liujingbo
 */
public class SoundPool {
	private final Map<Integer, Sound> sounds = new HashMap<Integer, Sound>();
	private final Queue<LoadRequest> loadQueue = new LinkedList<LoadRequest>();
	private Thread loadThread;
	private int lastSoundId;
	public static final int INVALID_STREAM_ID = 0;
	private final Listener listener;
	private PlayManager mPlayManager = null;
	private boolean vibrate = true;

	public SoundPool(Listener listener) {
		this.listener = listener;
	}

	/**
	 * 创建加载铃声文件信息整理
	 * @param context
	 * @param resName 加载铃声名（incoming，outgoing）
	 * @param path 预定义自定义铃声路径
	 * @return
	 * lastSoundId 关联MediaManager StockSound.Sound id
	 */
	public int load(Context context, String resName, String path) {
		int soundId;
		synchronized (this) {
			soundId = ++this.lastSoundId;
			if (soundId == 0) {
				soundId = ++this.lastSoundId;
			}
		}
		Sound sound = new Sound(soundId);
		LoadRequest req = new LoadRequest(sound, context, resName, path);

		synchronized (this.sounds) {
			this.sounds.put(Integer.valueOf(soundId), sound);
		}
		//加载请求体
		synchronized (this.loadQueue) {
			this.loadQueue.offer(req);
		}
		return startLoadThread(context, soundId);
	}

	/**
	 * 开始加载
	 * @param context
	 * @param soundId 加载的id
	 * @return
	 */
	private int startLoadThread(Context context, int soundId) {

		synchronized (this) {
			if ((soundId != 0) && ((this.loadThread == null) || (!this.loadThread.isAlive()))) {
				this.loadThread = new SoundPoolLoadThread(this.loadQueue, this.listener);
				this.loadThread.start();
			}
		}
		return soundId;
	}

	/**
	 * 预自定义铃声重新加载
	 * @param context
	 * @param stockSound 已经初始化的铃声
	 * @param path 自定义铃声路径
	 * @return
	 */
	public int reLoad(Context context, StockSound stockSound, String path) {
		LoadRequest req = null;
		int soundid = stockSound.getSoundId();
		Sound sound = null;
		synchronized (this.sounds) {
			sound = (Sound)this.sounds.get(Integer.valueOf(stockSound.getSoundId()));
		}
		if(sound == null) {
			soundid = ++lastSoundId;
			sound = new Sound(soundid);
		}
		req = new LoadRequest(sound, context, stockSound.getResName(), path);

		synchronized (this.loadQueue) {
			this.loadQueue.offer(req);
		}
		return startLoadThread(context, stockSound.getSoundId());
	}

	/**
	 * 移出指定id 铃声
	 * @param soundId
	 */
	public void unload(int soundId) {
		synchronized (this.loadQueue) {
			for (LoadRequest req : this.loadQueue) {
				if (req.sound.soundId == soundId) {
					this.loadQueue.remove(req);
					break;
				}
			}
		}
		synchronized (this.sounds) {
			this.sounds.remove(Integer.valueOf(soundId));
		}
	}

	/**
	 * 播放铃声
	 * @param context
	 * @param soundId
	 * @param loop
	 * @param duration
	 * @return
	 */
	public int play(Context context, int soundId, boolean loop, long duration) {
		Sound sound = null;
		synchronized (this.sounds) {
			sound = (Sound)this.sounds.get(Integer.valueOf(soundId));
		}

		if ((sound == null) || (sound.path == null) || (sound.state == SoundPool.Sound.State.FAILED)) {
			return 0;
		}
		Log4Util.d("[SoundPool] play = " + sound.state);
		synchronized (this) {
			try {
				if ((this.mPlayManager == null) || (!this.mPlayManager.isPlaying())) {
					mPlayManager = new PlayManager(context);
					mPlayManager.playMedia(loop, duration, vibrate, sound, this.listener);
				}
			} catch (Exception e) {
				e.printStackTrace();
				this.listener.onPlayComplete(sound.soundId);
			}
		}
		return sound.soundId;
	}

	/**
	 * 设置呼入振铃
	 * @param vibrate
	 */
	public void setIncomingVibrate(boolean vibrate) {
		this.vibrate = vibrate;
	}

	/**
	 * 停止铃声 在主动呼出接通，被动接听，挂断，需要调用
	 */
	public void stop() {
		if(mPlayManager != null) {
			mPlayManager.stopMedia();
			mPlayManager = null;
		}
	}

	/**
	 * 释放铃声池资源信息
	 */
	public void release() {
		if ((this.loadThread != null) && (this.loadThread.isAlive())) {
			this.loadThread.interrupt();
			try {
				this.loadThread.join(3000L);
			} catch (InterruptedException e) {
			}
			this.loadThread = null;
		}
		if(this.sounds != null) {
			this.sounds.clear();
		}
		if(mPlayManager != null) {
			mPlayManager.stopMedia();
		}
		this.loadQueue.clear();
	}

	@Override
	protected void finalize() {
		release();
	}
	
	
	
	/**
	 * @ClassName: LoadRequest
	 * @Description: 将assets瞎音频文件另存储在应用file下
	 */
	static class LoadRequest {
		final SoundPool.Sound sound;
		final Context context;
		final String resName;
		final String path;

		LoadRequest(SoundPool.Sound sound, Context context, String resName, String path) {
			this.sound = sound;
			this.context = context;
			this.resName = resName;
			this.path = path;
			
		}
	}

	static class Sound {
		final int soundId;
		State state = State.UNLOADED;
		String path = null;

		Sound(int soundId) {
			this.soundId = soundId;
		}

		static enum State {
			UNLOADED,
			READY,
			FAILED;
		}
	}

	public static abstract interface Listener {
		public abstract void onLoadComplete(int paramInt);

		public abstract void onPlayComplete(int paramInt);

		public abstract void onLoadFailed(int paramInt);
	}
}
