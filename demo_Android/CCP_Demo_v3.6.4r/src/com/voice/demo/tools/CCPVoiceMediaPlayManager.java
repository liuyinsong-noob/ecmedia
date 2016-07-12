/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.cloopen.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */package com.voice.demo.tools;

import android.annotation.TargetApi;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingDeque;

import com.voice.demo.ui.CCPHelper;
import com.voice.demo.tools.CCPIntentUtils;
/**
 * 
* <p>Title: CCPVoiceMediaPlayManager.java</p>
* <p>Description: For automatic play audio files, will be playing the voice file in the play queue</p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.cloopen.com</p>
* @author Jorstin Chan
* @date 2013-10-25
* @version 3.5
* 
* @deprecated There is no perfect, does not recommend to use 
 */
@TargetApi(9)
public class CCPVoiceMediaPlayManager implements Runnable{

	public static final int MODE_VOICE_PALY_AUTO = 1;
	public static final int MODE_VOICE_PALY_HAND = 2;
	
	private static final int TYPE_VOICE_PLAYING = 3;
	private static final int TYPE_VOICE_STOP = 4;
	private int mVoicePlayState = TYPE_VOICE_STOP;;
	
	private static CCPVoiceMediaPlayManager mInstance;
	
	private int mVoiceMode = MODE_VOICE_PALY_HAND;
	
	public static CCPVoiceMediaPlayManager getInstance(Context context) {
		if (mInstance == null) {
			mInstance = new CCPVoiceMediaPlayManager(context);
		}
		
		return mInstance;
	}
	
	public CCPVoiceMediaPlayManager(Context context) {
		this.mContext = context;
		context.registerReceiver(new BroadcastReceiver() {
			//INTENT_VOICE_PALY_COMPLETE
			@Override
			public void onReceive(Context context, Intent intent) {
				if(intent != null 
						&& intent.getAction().equalsIgnoreCase(CCPIntentUtils.INTENT_VOICE_PALY_COMPLETE) 
						&& mListener != null) {
					
					if(mCurrentVocie != -1) {
						mVoicePlayState = TYPE_VOICE_STOP;
						mListener.onVoiceComplete(mVoiceMode,mCurrentVocie);
					}
				}
			}
		}, new IntentFilter(CCPIntentUtils.INTENT_VOICE_PALY_COMPLETE));
		setRunning(true);
	}
	
	private Context mContext;
	private final Object synchro = new Object();
	
	private onVoiceMediaPlayListener mListener;
	
	 // for voice play
	private BlockingQueue<Integer> mVoicePlayQueue = new LinkedBlockingDeque<Integer>();
	private HashMap<Integer, String> mVoicePlayUrl = new HashMap<Integer, String>();
	private boolean isVoiceRuning = false;
	
	private int mCurrentVocie = -1;
	
	

	public void putVoicePlayQueue(HashMap<Integer, String> voiceUrl) {
		
		synchronized (synchro) {
			try {
				
				stopVoicePlay();
				
				mVoiceMode = MODE_VOICE_PALY_AUTO;
				for(Map.Entry<Integer, String> entry : voiceUrl.entrySet()) {
					
					mVoicePlayQueue.put(entry.getKey());
				}
				mVoicePlayUrl = voiceUrl;
				synchro.notify();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}

	private void stopVoicePlay() {
		if(mVoicePlayState == TYPE_VOICE_PLAYING) {
			CCPHelper.getInstance().getDevice().stopVoiceMsg();
		}
	}
	
	public void putVoicePlayQueue(int mode , Integer position , String voiceUrl) {
		
		synchronized (synchro) {
			try {
				stopVoicePlay();
				
				if(mode == MODE_VOICE_PALY_HAND) {
					mVoicePlayQueue.clear();
					mVoicePlayUrl.clear();
				}
				mVoiceMode = mode;
				mVoicePlayQueue.put(position);
				mVoicePlayUrl.put(position, voiceUrl);
				synchro.notify();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}
	

	public void setOnVoiceMediaPlayListener(onVoiceMediaPlayListener mListener) {
		this.mListener = mListener;
	}


	public boolean isIdle() {
		return mVoicePlayQueue.size() == 0 ? true : false;
	}

	@Override
	public void run() {
		android.os.Process
			.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
		while (isVoiceRuning) {
			synchronized (synchro) {
				while (isIdle()) {
					try {
						synchro.wait();
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
				}
			}
			
			synchronized (synchro) {
				
				if(mCurrentVocie == -1) {
					CCPAudioManager.getInstance().switchSpeakerEarpiece(mContext ,true);
				}
				
				mCurrentVocie = mVoicePlayQueue.poll();
				if(mVoicePlayUrl.containsKey(mCurrentVocie)) {
					String voiceUrl = mVoicePlayUrl.get(mCurrentVocie);
					CCPHelper.getInstance().getDevice().playVoiceMsg(voiceUrl);
					mListener.onVoiceStartPlay(mVoiceMode ,mCurrentVocie);
					mVoicePlayState = TYPE_VOICE_PLAYING;
				}
			}
			
		}
		
		release();
	}
	
	public void release() {
		 // for voice play
		mVoicePlayQueue.clear();
		mVoicePlayUrl.clear();
		isVoiceRuning = false;
		
		mCurrentVocie = -1;
		mVoicePlayState = TYPE_VOICE_STOP;
		setRunning(false);
		CCPAudioManager.getInstance().resetSpeakerState(mContext);
	}
	
	public void setRunning(boolean isRunning) {
		synchronized (synchro) {
			this.isVoiceRuning = isRunning;
			if (this.isVoiceRuning) {
				synchro.notify();
			}
		}
	}

	public boolean isRunning() {
		synchronized (synchro) {
			return isVoiceRuning;
		}
	}
	
	
	
	
	public interface onVoiceMediaPlayListener {
		
		void onVoiceStartPlay(int mode , Integer position);
		void onVoiceComplete(int mode ,Integer position);
	}
	
	
}
