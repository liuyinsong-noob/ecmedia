package com.hisun.phone.core.voice.multimedia;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.media.MediaRecorder.AudioSource;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;

import com.CCP.phone.NativeInterface;
import com.hisun.phone.core.voice.CCPCallImpl;
import com.hisun.phone.core.voice.CallControlManager;
import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.DeviceImpl;
import com.hisun.phone.core.voice.util.AdaptationTools;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.VoiceUtil;

/**
 * audio recoder manager ..
 * @version Time: 2013-6-7
 */
public class AudioRecordManager{

	private Context mContext;
	
	public static  String AUDIOT_RECORD_TIME_OUT ;
	
	public static final boolean DEBUG_RECORD = true;    // Set to false before record
	private static final int DEFAULT_MAX_DURATION = 1000 * 60;
	private static final int WHAT_ON_RECORDING_AMPLITUDE = 0x1;
	
	private static AudioRecordManager mInstance = null;
	
	/**
	 * buffer size in bytes
	 */
	private int bufferSizeInBytes = 0;  
	private int mLocks = 0; 
	private DeviceImpl listener;
	
	
	/**
	 * create an alarm clock, used for monitoring and recording 60 seconds timeout notification ,
	 * Certified broadcast monitoring alarm clock action
	 */
	private AlarmManager alarmManager;
	private RecordTimeoutReceiver receiver;
	private CCPAudioRecorder mAudRecord;
	CallControlManager callControlManager;
	
	public static synchronized AudioRecordManager getInstance() {
		if(mInstance == null ) {
			mInstance = new AudioRecordManager();
		}
		return mInstance;
	}
	
	private AudioRecordManager () {
		
		mContext = CCPCallImpl.getInstance().getContext();
		// at first init
		alarmManager = (AlarmManager) mContext.getSystemService(Context.ALARM_SERVICE);
		
		this.callControlManager = CCPCallImpl.getInstance().getCallControlManager();
		
		AUDIOT_RECORD_TIME_OUT = VoiceUtil.getPackageName(mContext) + ".intent.AUDIOT_RECORD_TIME_OUT";
		
		// register network receiver
		IntentFilter filter = new IntentFilter(AUDIOT_RECORD_TIME_OUT);
		receiver = new RecordTimeoutReceiver();
		mContext.registerReceiver(receiver, filter);
		
		// This code for testing, testing whether the 
		// machine support parameters set
		//createAudioRecord();
		
		
		// start consumer thread ..
		RecordConsumer.getInstance();
		
        Log4Util.d(Device.TAG , "[AudioRecordManager - Construction method ] this buffer size : " + bufferSizeInBytes);
	}
	
	// Just record the audio data, do not send real-time
	public void startRecord(String uniqueID , String fileName) {
		initRecording(uniqueID , false, fileName, null, null);
	}
	
	// Record audio data at the same time, the chunked will send 
	// the recorded audio data to the server
	public void startRecord(String uniqueID , String fileName, String groupId, String userData) {
		
		initRecording(uniqueID, true, fileName, groupId, userData);
				
	}
	
	// Initialize the recording, whether to use the chunked collection 
	// of audio data upload mode
	// Note: 3.0.1 Add the parameters, namely the unique identifier for the audio message
	// and delete parameters type for voice delay...
	synchronized void initRecording(String uniqueID , boolean isChunked ,String fileName, String groupId, String userData) {
		if (mAudRecord != null
				&& mAudRecord.getState() != CCPAudioRecorder.State.STOPPED) {

			return ;
		}

		if (mLocks == 0) {
			/*mAudRecord = new CCPAudioRecorder(CCPAudioRecorder.mAudioSource,
				mSampleRate, mChannelConfig, mAudioFormat ,isChunked);*/
			mAudRecord = new CCPAudioRecorder(MediaRecorder.AudioSource.MIC,
					CCPAudioRecorder.sampleRates[4], AudioFormat.CHANNEL_CONFIGURATION_MONO, AudioFormat.ENCODING_PCM_16BIT ,isChunked);
			mAudRecord.setOutputFile(fileName);
			mAudRecord.setSendParameters(groupId, userData, uniqueID);
			mAudRecord.prepare();
			mAudRecord.start();
			
			long triggerAtTime = SystemClock.elapsedRealtime() + DEFAULT_MAX_DURATION + 100;
			setAlarmTime(triggerAtTime, AUDIOT_RECORD_TIME_OUT);
			
			if(handler != null ) {
				handler.sendEmptyMessage(WHAT_ON_RECORDING_AMPLITUDE);
			}
			
			if(!AdaptationTools.callNoiseSuppression()){
				NativeInterface.InitAudioDevice();
			}
			mLocks++; 
		}
		Log4Util.i("initRecording mLocks " + mLocks);
		
	}
	
	public void cancleRecord(boolean isCancle) {
		if(mAudRecord != null ) {
			mAudRecord.cancleEnable(isCancle);
		}
	}
	
	public synchronized void stopRecord() {
		if(mAudRecord != null) {
			Log4Util.i("stopRecord mAudRecord.getState() " + mAudRecord.getState());
		}
		if(mAudRecord != null && mAudRecord.getState() == CCPAudioRecorder.State.RECORDING) {
			mLocks--; 
			
			Log4Util.i("stopRecord mLocks " + mLocks);
			if (mLocks == 0) {
				mAudRecord.stop();
				mAudRecord.release();
				mAudRecord = null;
				cancelAlarmTime(AUDIOT_RECORD_TIME_OUT);
			}
			if(!AdaptationTools.callNoiseSuppression()){
				NativeInterface.UNInitAudioDevice();
			}
		} else {
			mLocks = 0;
		}
	}
	
	/**
	 * release All resources associated with the recording..
	 */
	public void release() {
		if (mContext != null) {
			mContext.unregisterReceiver(receiver);
			receiver = null;
		}
		handler = null;
		
		// cancle alarm clock.
		cancelAlarmTime(AUDIOT_RECORD_TIME_OUT);
		mAudRecord = null;
		mInstance = null;
	}
	
	
	public DeviceImpl getListener() {
		return listener;
	}

	public void setListener(DeviceImpl listener) {
		this.listener = listener;
	}
	
	public int getMaxDuration() {
		return DEFAULT_MAX_DURATION;
	}
	
	Handler handler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			
			int what = msg.what;
			
			if(what == WHAT_ON_RECORDING_AMPLITUDE ) {
				if(mAudRecord != null && mAudRecord.getState() == CCPAudioRecorder.State.RECORDING) {
					
					int maxAmplitude = mAudRecord.getMaxAmplitude();
					final double amplitude = maxAmplitude % 100 ;
					if(amplitude > 0 ) {
						
					}
					if(listener != null ) {
						listener.onRecordingAmplitude(amplitude);
					}
					
					sendEmptyMessageDelayed(WHAT_ON_RECORDING_AMPLITUDE , 200);
				}
			}

			super.handleMessage(msg);
		}		
	};

	private void setAlarmTime(long triggerAtTime, String action) {
		PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0,new Intent(action), PendingIntent.FLAG_UPDATE_CURRENT);
		alarmManager.set(AlarmManager.ELAPSED_REALTIME, triggerAtTime , pendingIntent);
	}
	
	private void cancelAlarmTime(String action) {
		PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0, new Intent(action), PendingIntent.FLAG_CANCEL_CURRENT);
		alarmManager.cancel(pendingIntent);
	}
	
	class RecordTimeoutReceiver extends BroadcastReceiver {

		public RecordTimeoutReceiver() {
			
		}
		
		@Override
		public void onReceive(Context context, Intent intent) {
			if (intent == null || context == null) {
				return;
			}
			
			final String action = intent.getAction() == null ? "" : intent.getAction();
			Log4Util.w(Device.TAG, "[AudioRecordManager - onReceive] action = " + action);
			if(action.equals(AUDIOT_RECORD_TIME_OUT)) {
				if(mLocks ==0 ) {
					return;
				}
				stopRecord();
				if(listener != null ) {
					listener.onRecordingTimeOut(DEFAULT_MAX_DURATION);
				}
				cancelAlarmTime(AUDIOT_RECORD_TIME_OUT);
			}
		}
	}

	private int mSampleRate;
	private short mAudioFormat;
	private short mChannelConfig ;  
	
	@Deprecated
	public void createAudioRecord() { 
        if (mSampleRate > 0 && mAudioFormat > 0 && mChannelConfig > 0) { 
            return; 
        } 
 
        // should try user's specific combinations first, if it's invalid, then do for loop to get a 
        // available combination instead. 
 
        // Find best/compatible AudioRecord 
        // If all combinations are invalid, throw IllegalStateException 
        for (int sampleRate : new int[] { 8000, 11025, 16000, 22050, 32000, 
                44100, 47250, 48000 }) { 
            for (short audioFormat : new short[] { 
                    AudioFormat.ENCODING_PCM_16BIT, 
                    AudioFormat.ENCODING_PCM_8BIT }) { 
                for (short channelConfig : new short[] { 
                        AudioFormat.CHANNEL_IN_MONO, 
                        AudioFormat.CHANNEL_IN_STEREO, 
                        AudioFormat.CHANNEL_CONFIGURATION_MONO, 
                        AudioFormat.CHANNEL_CONFIGURATION_STEREO }) { 
 
                    // Try to initialize 
                    try { 
                       int mBufferSize = AudioRecord.getMinBufferSize(sampleRate, 
                                channelConfig, audioFormat); 
 
                        if (mBufferSize < 0) { 
                            continue; 
                        } 
 
                        AudioRecord   mAudioRecord = new AudioRecord(AudioSource.MIC, 
                                sampleRate, channelConfig, audioFormat, 
                                mBufferSize); 
 
                        if (mAudioRecord.getState() == AudioRecord.STATE_INITIALIZED) { 
                            mSampleRate = sampleRate; 
                            mAudioFormat = audioFormat; 
                            mChannelConfig = channelConfig; 
                            
                            Log4Util.d(Device.TAG, "[initRecorder ] mSampleRate :" + mSampleRate + " , mAudioFormat:" + mAudioFormat + " , mChannelConfig:" + mChannelConfig);
                            mAudioRecord.release(); 
                            mAudioRecord = null; 
                            return; 
                        } 
 
                    } catch (Exception e) { 
                        // Do nothing 
                    } 
                } 
            } 
        } 
 
        // ADDED(billhoo) all combinations are failed on this device. 
        throw new IllegalStateException( 
                "getInstance() failed : no suitable audio configurations on this device."); 
    }
	

}
