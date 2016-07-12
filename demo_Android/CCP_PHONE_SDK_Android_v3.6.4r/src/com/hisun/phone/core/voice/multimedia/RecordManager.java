package com.hisun.phone.core.voice.multimedia;

import com.hisun.phone.core.voice.DeviceImpl;

import android.media.MediaRecorder;
import android.media.MediaRecorder.AudioEncoder;
import android.media.MediaRecorder.AudioSource;
import android.media.MediaRecorder.OutputFormat;
import android.os.Handler;
import android.os.Message;

public class RecordManager {
	public static final int DEFAULT_MAX_DURATION = 1000 * 60;
	public static final int FILE_FORMAT = OutputFormat.RAW_AMR;
	public static final int AUDIO_CODEC = AudioEncoder.AMR_NB;
	private static RecordManager mInstance = null;
	private MediaRecorder mRec = null;
	boolean mRecording = false;
	private DeviceImpl listener;

	public static synchronized RecordManager getInstance() {
		if (null == mInstance) {
			mInstance = new RecordManager();
		}
		return mInstance;
	}

	private RecordManager() {

	}

	public int getMaxDuration() {
		return DEFAULT_MAX_DURATION;
	}
	
	public void setLisenter(DeviceImpl listener) {
		this.listener = listener;
	}

	public void startRecord(String fileName) throws Exception {
		this.mRec = new MediaRecorder();
		this.mRec.setAudioSource(AudioSource.MIC);
		this.mRec.setOutputFormat(FILE_FORMAT);
		this.mRec.setAudioEncoder(AUDIO_CODEC);
		this.mRec.setAudioChannels(1);
		this.mRec.setAudioEncodingBitRate(16);
		
		this.mRec.setOutputFile(fileName);
		this.mRec.setMaxDuration(getMaxDuration());
		handler.sendEmptyMessageDelayed(1, 100);
		try {
			this.mRec.prepare();
			this.mRec.start();
			this.mRecording = true;
		} catch (Exception e) {
			this.mRec.release();
			this.mRec = null;
			this.mRecording = false;
			throw e;
		}
	}
	
	public void stopRecord() {
		if (this.mRecording) {
			this.mRecording = false;
			if (null != this.mRec) {
				this.mRec.stop();
				this.mRec.release();
				this.mRec = null;
			}
		}
	}
	
	Handler handler = new Handler() {
		int time = 0;
		@Override
		public void handleMessage(Message msg) {
			if (mRecording) {
				if (listener != null) {
					double amp = 0;
					if(mRec!=null){
						amp = mRec.getMaxAmplitude();
					}
					listener.onRecordingAmplitude(amp);
					if(time>=DEFAULT_MAX_DURATION){
						listener.onRecordingTimeOut(DEFAULT_MAX_DURATION);
					}
				}
				time = time+100;
				sendEmptyMessageDelayed(1, 100);
			}else{
				time = 0;
			}
			super.handleMessage(msg);
		}		
	};
	
	public void release() {
		if(mRec!=null){
			mRec.stop();
			mRec.release();
		}	
		mRec = null;
	}
}
