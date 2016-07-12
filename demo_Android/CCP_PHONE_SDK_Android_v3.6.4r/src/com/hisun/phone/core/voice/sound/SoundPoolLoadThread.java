package com.hisun.phone.core.voice.sound;

import java.io.File;
import java.util.Queue;

import com.hisun.phone.core.voice.sound.PlayManager;
import com.hisun.phone.core.voice.sound.SoundPool;
import com.hisun.phone.core.voice.util.Log4Util;

/**
 * @ClassName: SoundPoolLoadThread
 * @Description: 加载铃声
 * @author liujingbo
 */
public class SoundPoolLoadThread extends Thread {
	private final Queue<SoundPool.LoadRequest> loadQueue;
	private final SoundPool.Listener listener;

	SoundPoolLoadThread(Queue<SoundPool.LoadRequest> loadQueue, SoundPool.Listener listener) {
		this.loadQueue = loadQueue;
		this.listener = listener;
	}

	public void run() {
		while (!isInterrupted()) {
			SoundPool.LoadRequest req = null;
			synchronized (this.loadQueue) {
				req = (SoundPool.LoadRequest)this.loadQueue.poll();
			}
			if (req == null) {
				break;
			}
			try {
				boolean success = false;
				String path = null;
				if(req.path != null && req.path.trim().length() > 0) { //自定义铃声设置
					success = PlayManager.isExistFile(req.path);
					path = req.path;
				}else { //默认铃声加载
					success = PlayManager.saveAudioFile(req.context, req.resName);
					path = PlayManager.getAudioFilePath(req.context, req.resName);
					
					
				}
				if(success) {
					req.sound.state = SoundPool.Sound.State.READY;
					req.sound.path = path;
					
					File file = new File(path);
					if(file.exists() && file.length() > 0)  {
						if (this.listener != null) {
							this.listener.onLoadComplete(req.sound.soundId);
						}
						
					} else {
						if (this.listener != null)
							this.listener.onLoadFailed(req.sound.soundId);
					}
					
				}else {
					req.sound.state = SoundPool.Sound.State.FAILED;
					req.sound.path = null;
					if (this.listener != null)
						this.listener.onLoadFailed(req.sound.soundId);
				}
				Log4Util.d("[XSoundPoolLoadThread] load finish " + req.sound.state);
			} catch (Exception e) {
				Log4Util.e("Failed to load sound with ID " + req.resName + ": " + e.getMessage());
				req.sound.state = SoundPool.Sound.State.FAILED;
				if (this.listener != null)
					this.listener.onLoadFailed(req.sound.soundId);
			}
		}
	}
}
