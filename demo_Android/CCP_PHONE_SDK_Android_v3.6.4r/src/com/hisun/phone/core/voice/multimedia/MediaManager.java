/**
 *
 */
package com.hisun.phone.core.voice.multimedia;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.Queue;

import android.content.Context;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.sound.SoundPool;
import com.hisun.phone.core.voice.util.Log4Util;

/**
 * 实现多播放器、多资源统一管理
 *
 * @author chao
 *
 */
public final class MediaManager implements SoundPool.Listener {

	private static MediaManager sInstance;
	private final Context context;
	private SoundPool soundPool;
	private final Queue<PlaybackItem> playQueue = new LinkedList<PlaybackItem>();
	private PlaybackItem lastRunningItem;

	private final Map<Integer, PlaybackItem> soundsInFlight = new HashMap<Integer, PlaybackItem>();
	private int lastPlaybackId = 0;
	
	public static MediaManager getInstance() {
		if (sInstance == null) {
			throw new IllegalStateException("MediaManager has not been created yet");
		}
		return sInstance;
	}

	public static MediaManager initialize(Context context) {
		if (sInstance != null) {
			throw new IllegalStateException("MediaManager has already been initalized");
		}
		sInstance = new MediaManager(context);
		return sInstance;
	}

	public void destroy() {
		if (this.soundPool != null) {
			this.soundPool.release();
		}
		if (playQueue != null) {
			playQueue.clear();
		}
		if (soundsInFlight != null) {
			soundsInFlight.clear();
		}
		sInstance = null;
	}

	private MediaManager(Context context) {
		this.context = context;
		loadAssets();
	}

	/**
	 *  初始化加载assets文件夹下的铃声文件
	 */
	private void loadAssets(){
		this.soundPool = new SoundPool(this);
		for (StockSound sound : StockSound.values()) {
			sound.setSoundId(this.soundPool.load(this.context, sound.getResName(), null));
		}
	}

	/**
	 * 只播放一次
	 * @param sound
	 * @param music
	 * @param listener
	 * @return
	 */
	public synchronized int queueSound(StockSound sound, SoundPlaybackListener listener){
		return queueSound(sound, false, 0, listener);
	}

	/**
	 * 循环播放，播放时间控制
	 * @param sound
	 * @param duration
	 * @param listener
	 * @return
	 */
	public synchronized int queueSound(StockSound sound, long duration, SoundPlaybackListener listener){
		return queueSound(sound, true, duration, listener);
	}

	private synchronized int queueSound(StockSound sound, boolean loop, long duration, SoundPlaybackListener listener) {
		int playbackId = 0;
		playbackId = ++this.lastPlaybackId;
		if (playbackId == 0) {
			playbackId = ++this.lastPlaybackId;
		}

		//回铃音播放停止，清楚所有正在或准备播放的声音
		/*if(sound.resName.equals(StockSound.RINGBACKTONE.resName)) {
			stop();
		}*/

		PlaybackItem item = new PlaybackItem(playbackId, sound, loop, duration, listener);
		Log4Util.d("[MediaManager] queueSound = " + sound.getResName());
		
		if(item.sound.isAvailable()) {
			if (this.lastRunningItem == null) {
				this.lastRunningItem = item;
				this.soundsInFlight.put(Integer.valueOf(item.playbackId), item);
				item.sound.soundId = this.soundPool.play(context, item.sound.soundId, item.loop, duration);
				
				Log4Util.w(Device.TAG, "item.sound.soundId:" + item.sound.soundId);
				if (item.sound.soundId == 0) {
					onPlayComplete(item.sound.soundId);
				}
			} else {
				this.playQueue.offer(item);
			}
		}
		
		return playbackId;
	}

	/**
	 *
	 * @param playbackId
	 */
	public synchronized void cancel(int playbackId) {
		PlaybackItem item = (PlaybackItem)this.soundsInFlight.get(Integer.valueOf(playbackId));
		if (item != null) {
			this.soundPool.stop();
			handleComplete(item);
		}

		for (PlaybackItem queuedItem : this.playQueue) {
			if (queuedItem.playbackId == playbackId) {
				this.playQueue.remove(queuedItem);
				break;
			}
		}

	}

	/**
	 * 设置呼入振铃
	 * @param vibrate
	 */
	public void setInComingVibrate(boolean vibrate) {
		if (this.soundPool != null) {
			this.soundPool.setIncomingVibrate(vibrate);
		}
	}

	/**
	 * 停止播放 在主动呼出接通，被动接听，挂断，需要调用
	 */
	public void stop() {
		if (this.soundPool != null) {
			this.soundPool.stop();
		}
		this.playQueue.clear();
		this.lastRunningItem = null;
	}

	/**
	 * 释放资源
	 */
	public void release() {

		if (this.soundPool != null) {
			this.soundPool.release();
			this.soundPool = null;
		}
		this.playQueue.clear();
		this.lastRunningItem = null;
	}

	protected void finalize()
	{
		release();
	}

	/**
	 * 队列播放，一次完成播放
	 * @param item
	 */
	private void handleComplete(PlaybackItem item)
	{
		this.soundsInFlight.remove(Integer.valueOf(item.playbackId));
		if (item == this.lastRunningItem) {
			this.lastRunningItem = ((PlaybackItem)this.playQueue.poll());
			if (this.lastRunningItem != null) {
				Log4Util.d("[MediaManager] handleComplete nextsound = " + lastRunningItem.sound.resName);
				this.soundsInFlight.put(Integer.valueOf(this.lastRunningItem.playbackId), this.lastRunningItem);

				if ((!this.lastRunningItem.sound.isAvailable()) && (this.lastRunningItem.listener != null)) {
					this.lastRunningItem.listener.onCompletion();
					handleComplete(this.lastRunningItem);
				} else {
					this.lastRunningItem.sound.soundId = this.soundPool.play(context, this.lastRunningItem.sound.soundId,
							this.lastRunningItem.loop, this.lastRunningItem.duration);
				}
			}
		}
	}
	/**
	 * 加载成功回调
	 */
	@Override
	public void onLoadComplete(int soundId) {
		for (StockSound sound : StockSound.values()) {
			if (sound.getSoundId() != soundId)
				continue;
			sound.setAvailable(true);
			break;
		}
	}
	/**
	 * 加载失败回调
	 */
	@Override
	public void onLoadFailed(int soundId) {
		for (StockSound sound : StockSound.values()) {
			if (sound.getSoundId() != soundId)
				continue;
			sound.setAvailable(false);
			break;
		}
	}

	/**
	 * 播放完成回调
	 */
	@Override
	public void onPlayComplete(int soundid) {
		Log4Util.d("[MediaManager] onPlayComplete play finish " + soundid);
		for (Map.Entry<Integer, PlaybackItem> entry : this.soundsInFlight.entrySet()) {
			PlaybackItem item = (PlaybackItem)entry.getValue();
			if (item.sound.soundId == soundid) {
				if (item.listener != null) {
					item.listener.onCompletion();
				}
				handleComplete(item);
				break;
			}
		}
	}


	/**
	 *
	 * @ClassName: SoundPlaybackListener
	 * @Description: 铃声播放完成回调
	 *
	 */
	public static abstract interface SoundPlaybackListener {
		public abstract void onCompletion();
	}

	private static class PlaybackItem {
		final int playbackId;
		final MediaManager.StockSound sound;
		final boolean loop;
		final MediaManager.SoundPlaybackListener listener;
		final long duration;

		PlaybackItem(int playbackId, MediaManager.StockSound sound, boolean loop, long duration, MediaManager.SoundPlaybackListener listener) {
			this.playbackId = playbackId;
			this.sound = sound;
			this.loop = loop;
			this.duration = duration;
			this.listener = listener;
		}
	}

	public static enum StockSound {
		//DISCONNECT("disconnect"),
		INCOMING("incoming"),
		OUTGOING("outgoing"),
		//RINGBACKTONE("ringbacktone"),
		BUSY("busy");

		private final String resName;
		private int soundId;
		private boolean available = false;
		private StockSound(String resName) {
			this.resName = resName;
		}

		public String getResName() {
			return this.resName;
		}

		public int getSoundId() {
			return this.soundId;
		}

		public void setSoundId(int soundId) {
			this.soundId = soundId;
		}

		public boolean isAvailable() {
			return this.available;
		}

		public void setAvailable(boolean available) {
			this.available = available;
		}
	}
}
