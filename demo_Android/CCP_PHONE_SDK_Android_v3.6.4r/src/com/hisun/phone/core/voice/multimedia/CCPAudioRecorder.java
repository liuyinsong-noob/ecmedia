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
 */
package com.hisun.phone.core.voice.multimedia;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.RecourProductInfo;
import com.hisun.phone.core.voice.multimedia.Suppressor.AudioPreProcess;
import com.hisun.phone.core.voice.util.Log4Util;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;

/**
 * 		// start record..
 *		mAudRecord = new CCPAudioRecorder(CCPAudioRecorder.mAudioSource,
 *				CCPAudioRecorder.sampleRates[4], CCPAudioRecorder.mChannelConfig, CCPAudioRecorder.mAudioEncoding ,isChunked);
 *		mAudRecord.setOutputFile(fileName, type);
 *		mAudRecord.setSendParameters(groupId, toVoip);
 *		mAudRecord.prepare();
 *		mAudRecord.start();
 * 		
 * 		// stop android release ..
 * 		if (mAudRecord != null) {
 *			mAudRecord.stop();
 *			mAudRecord.release();
 *		}
 *	Basically what this class does is construct a valid AudioRecord Object, wrap AudioRecord methods, and provide 
 * 	conversion to decibels. It also caches the AudioRecord configuration and prevents multiple instances of the recorder. 
 * 
 * @version Time: 2013-6-22
 */
public class CCPAudioRecorder {
	
	public static final byte[] AMR_CODEC_HEAD = new byte[] { 0x23, 0x21, 0x41,0x4d, 0x52, 0x0a };
	
	public static final byte[] AMR_STOP = new byte[] { 0x23, 0x21, 0x48, 0x49,0x53, 0x55, 0x4e, 0x53, 0x54, 0x4f, 0x50 };
	
	// [35, 33, 72, 73, 83, 85, 78, 69, 82, 82, 79, 82]
	public static final byte[] AMR_ERROR = new byte[] { 0x23, 0x21, 0x48, 0x49,0x53, 0x55, 0x4e, 0x45, 0x52, 0x52, 0x4f , 0x52 };
	
	public final static int[] sampleRates = { 44100, 16000, 22050, 11025, 8000 };

    
    /**
     * Channel CHANNEL_IN_STEREO audio recording for two-channel, 
     * CHANNEL_CONFIGURATION_MONO for mono
     * 
     */
    public static final int mChannelConfig 								= AudioFormat.CHANNEL_CONFIGURATION_MONO;  
    
    /**
     * audio data format: 
     * PCM 16 bits per sample.  Ensure the equipment support.
     * PCM 8 bits per sample. Not be able to get the device support.
     */
    public static final int mAudioEncoding 								= AudioFormat.ENCODING_PCM_16BIT;

    /**
     *  The interval in which the recorded samples are output to the file
     *  
     */
	private static final int TIMER_INTERVAL 							= 120;


	/**
	 * Recorder instance 
	 */
	private AudioRecord mAudioRecord 									= null;
	private RecourProductInfo info 										= null;

	private Looper mRecordLooper;
	private Handler mRecordHandler;
	private Thread mEncodeThread                                        = null ;

	/**
	 * Stores current amplitude 
	 */
	private int mAmplitude 												= 0;

	/**
	 * Output file path
	 * Save the audio stream stored in local path
	 */
	private String filePath 											= null;

	/**
	 * Recorder state
	 * {@link State}
	 */
	private State state;

	AmrEncoder encoder 													= null;

	// Number of channels, sample rate, sample size(size in bits), buffer size,
	// audio source, sample size(see AudioFormat)
	private short nChannels;
	private int mSampleRate;
	private short bSamples;
	private int mBufferSize												= AudioRecord.ERROR_BAD_VALUE;;
	private int mAudioSource;
	private int mAudioFormat;

	// Number of frames written to file on each output(only in uncompressed
	// mode)
	private int mFramePeriod;

	// Buffer for output(only in uncompressed mode)
	private byte[] mBuffer;

	private boolean isChunked 											= false;
	
	
    
	/**
	 * 
	 * Returns the state of the recorder in a RehearsalAudioRecord.State typed
	 * object. Useful, as no exceptions are thrown.
	 * 
	 * @return recorder state
	 */
	public State getState() {
		return state;
	}
	
	public boolean isChunkedRecord() {
		return this.isChunked;
	}

	/*
	 * 
	 * Method used for recording.
	 */
	private AudioRecord.OnRecordPositionUpdateListener updateListener = new AudioRecord.OnRecordPositionUpdateListener() {
		public void onPeriodicNotification(AudioRecord recorder) {
			int readSize = mAudioRecord.read(mBuffer, 0, mBuffer.length); // Fill
			Log4Util.d(Device.TAG , "OnRecordPositionUpdateListener on thread: " + Thread.currentThread().getName());
			if(AudioRecordManager.DEBUG_RECORD) {
				// buffer
				Log4Util.v(Device.TAG, "read len :" + readSize);
			}
			
			if (readSize > 0 && encoder != null) {
				encoder.putData(mBuffer, readSize);
				
				if (bSamples == 16) {
					for (int i = 0; i < mBuffer.length / 2; i++) { // 16bit
						// sample
						// size
						short curSample = getShort(mBuffer[i * 2], mBuffer[i * 2 + 1]);
						if (curSample > mAmplitude) { 
							// Check amplitude
							mAmplitude = curSample;
						}
					}
				} else { 
					// 8bit sample size
					for (int i = 0; i < mBuffer.length; i++) {
						if (mBuffer[i] > mAmplitude) { 
							mAmplitude = mBuffer[i];
						}
					}
				}
				
			}

		}

		public void onMarkerReached(AudioRecord recorder) {
			// NOT USED
		}
	};

	/**
	 * Default constructor
	 * Instantiates a new recorder, in case of compressed recording the
	 * parameters can be left as 0. In case of errors, no exception is thrown,
	 * but the state is set to ERROR
	 * 
	 */
	public CCPAudioRecorder(int audioSource,
			int sampleRate, int channelConfig, int audioFormat , boolean isChunked) {
		
		this.isChunked = isChunked;
		try {
			if (audioFormat == AudioFormat.ENCODING_PCM_16BIT) {
				bSamples = 16;
			} else {
				bSamples = 8;
			}

			if (channelConfig == AudioFormat.CHANNEL_CONFIGURATION_MONO) {
				nChannels = 1;
			} else {
				nChannels = 2;
			}

			mAudioSource = audioSource;
			mSampleRate = sampleRate;
			mAudioFormat = audioFormat;

			// read the recording data of buffer should be bufferSize/2, 
			// otherwise there will be stop, 
			// data delay and confusion (data not correctly written documents)
			mFramePeriod = sampleRate * TIMER_INTERVAL / 1000;
			mBufferSize = mFramePeriod * 2 * bSamples * nChannels / 8;
			if(AudioRecordManager.DEBUG_RECORD) {
				
				Log4Util.v(Device.TAG,
						"[CCPAudioRecorder - Construction method ] bufferSize " + Integer.toString(mBufferSize));
			}
			
			int minBufferSize = AudioRecord.getMinBufferSize(sampleRate,
					channelConfig, audioFormat);
			if (mBufferSize < minBufferSize) {
				
				// Check to make sure
				// buffer size is not
				// smaller than the
				// smallest allowed one
				mBufferSize = (minBufferSize/2) * 10;
				
				if(AudioRecordManager.DEBUG_RECORD) {
					
					// Set frame period and timer interval accordingly
					//framePeriod = bufferSize / (2 * bSamples * nChannels / 8);
					Log4Util.v(Device.TAG,
							"[CCPAudioRecorder - Construction method ] Increasing buffer size to "
							+ Integer.toString(mBufferSize));
				}
				
			}
			mAudioRecord = new AudioRecord(audioSource, sampleRate,
					channelConfig, audioFormat, mBufferSize );

			if (mAudioRecord.getState() != AudioRecord.STATE_INITIALIZED)
				throw new Exception("AudioRecord initialization failed");
			
			AudioPreProcess audioPreProcess = new AudioPreProcess();
			audioPreProcess.init(mAudioRecord);
			
			HandlerThread handlerThread = new HandlerThread("CCPPcmRecorder", android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
			handlerThread.start();
			mRecordLooper = handlerThread.getLooper();
			mRecordHandler = new Handler(mRecordLooper);
			
			mAudioRecord.setRecordPositionUpdateListener(updateListener , mRecordHandler);
			mAudioRecord.setPositionNotificationPeriod(mFramePeriod);
				
			mAmplitude = 0;
			filePath = null;
			state = State.INITIALIZING;
			
		} catch (Exception e) {
			e.printStackTrace();
			if (e.getMessage() != null) {
				if(AudioRecordManager.DEBUG_RECORD) {
					
					Log4Util.v(Device.TAG, e.getMessage());
				}
			} else {
				if(AudioRecordManager.DEBUG_RECORD) {
					
					Log4Util.v(Device.TAG,
						"[CCPAudioRecorder - Construction method ] Unknown error occured while initializing recording");
				}
			}
			state = State.ERROR;
		}
	}


	/**
	 * Sets output file path, call directly after construction/reset.
	 * @param argPath output file path
	 */
	public void setOutputFile(String argPath) {
		try {
			if (state == State.INITIALIZING) {
				filePath = argPath;
				info = new RecourProductInfo();
				info.fileName = argPath;
			}
		} catch (Exception e) {
			e.printStackTrace();
			if (e.getMessage() != null) {
				if(AudioRecordManager.DEBUG_RECORD) {
					
					Log4Util.v(Device.TAG, e.getMessage());
				}
			} else {
				if(AudioRecordManager.DEBUG_RECORD) {
					
					Log4Util.v(Device.TAG,
						"[CCPAudioRecorder - setOutputFile ] Unknown error occured while setting output path");
				}
			}
			state = State.ERROR;
		}
	}


	/**
	 * Sets output file path, call directly after construction/reset.
	 * Note: 3.0.1 Add the parameters, namely the unique identifier for the audio message
	 * @param output file path
	 */
	public void setSendParameters(String groupId, String userData , String uniqueID) {
		try {
			if (state == State.INITIALIZING) {
				info.groupId = groupId;
				info.userData = userData;
				info.uniqueID = uniqueID;
			} else {

				throw new Exception("AudioRecord is uninitializing ..");
			}
		} catch (Exception e) {
			if (e.getMessage() != null) {
				if(AudioRecordManager.DEBUG_RECORD) {
					
					Log4Util.v(Device.TAG, e.getMessage());
				}
			} else {
				if(AudioRecordManager.DEBUG_RECORD) {
					Log4Util.v(Device.TAG,
						"[CCPAudioRecorder - setSendParameters ] Unknown error occured while setting output path");
				}
					
			}
			state = State.ERROR;
		}
	}

	/**
	 * Returns the largest amplitude sampled since the last call to this method.
	 * @return returns the largest amplitude since the last call, or 0 when not
	 *         in recording state.
	 */
	public int getMaxAmplitude() {
		if (state == State.RECORDING) {
				int result = mAmplitude;
				mAmplitude = 0;
				return result;
		} else {
			return 0;
		}
	}

	/**
	 * 
	 * Prepares the recorder for recording, in case the recorder is not in the
	 * INITIALIZING state and the file path was not set the recorder is set to
	 * the ERROR state, which makes a reconstruction necessary. In case
	 * uncompressed recording is toggled, the header of the AMR file is
	 * written. In case of an exception, the state is changed to ERROR
	 * 
	 */
	public void prepare() {
		try {
			if (state == State.INITIALIZING) {
				if ((mAudioRecord.getState() == AudioRecord.STATE_INITIALIZED)
						& (filePath != null)) {

					mBuffer = new byte[mFramePeriod * bSamples / 8 * nChannels];
					if(AudioRecordManager.DEBUG_RECORD) {
						Log4Util.v(Device.TAG,
								"[CCPAudioRecorder - prepare ] buffer length : "
								+ (mFramePeriod * bSamples / 8 * nChannels));
					}
						

					encoder = new AmrEncoder(info , isChunked);
					mEncodeThread = new Thread(encoder);
					encoder.setRunning(true);
					mEncodeThread.start();

					state = State.READY;
				} else {
					if(AudioRecordManager.DEBUG_RECORD) {
						
						Log4Util.v(Device.TAG,
							"[CCPAudioRecorder - prepare ] prepare() method called on uninitialized recorder");
					}
					state = State.ERROR;
				}
			} else {
				if(AudioRecordManager.DEBUG_RECORD) {
					
					Log4Util.v(Device.TAG,
						"[CCPAudioRecorder - prepare ] prepare() method called on illegal state");
				}
				release();
				state = State.ERROR;
			}
		} catch (Exception e) {
			if (e.getMessage() != null) {
				if(AudioRecordManager.DEBUG_RECORD) {
					
					Log4Util.v(Device.TAG, e.getMessage());
				}
			} else {
				if(AudioRecordManager.DEBUG_RECORD) {
					
					Log4Util.v(Device.TAG,
						"[CCPAudioRecorder - prepare ] Unknown error occured in prepare()");
				}
			}
			state = State.ERROR;
		}
	}

	/**
	 * Releases the resources associated with this class, and removes the
	 * unnecessary files, when necessary
	 */
	public void release() {
		if (state == State.RECORDING) {
			stop();
		}

		if (mAudioRecord != null) {
			mAudioRecord.release();
			mAudioRecord = null;
			if(mRecordLooper != null) {
				mRecordLooper.quit();
				mRecordLooper = null;
			}
			mRecordHandler = null;
			if(AudioRecordManager.DEBUG_RECORD) {
				
				Log4Util.v(Device.TAG,
					"[CCPAudioRecorder - release] audioRecorder release .");
			}
		}
		
		if(encoder != null) {
			encoder.setRunning(false);
		}
		
		mEncodeThread = null;
		filePath = null;
		mFramePeriod = 0;
		mAmplitude = 0;
		encoder = null;
		isChunked = false;
		mAudioRecord = null;
		
	}

	/**
	 * Resets the recorder to the INITIALIZING state, as if it was just created.
	 * In case the class was in RECORDING state, the recording is stopped. In
	 * case of exceptions the class is set to the ERROR state.
	 */
	public void reset() {
		try {
			if (state != State.ERROR) {
				release();
				filePath = null; // Reset file path
				mAmplitude = 0; // Reset amplitude
				
				mAudioRecord = new AudioRecord(mAudioSource, mSampleRate,
						nChannels + 1, mAudioFormat, mBufferSize);
				
				state = State.INITIALIZING;
			}
		} catch (Exception e) {
			if(AudioRecordManager.DEBUG_RECORD) {
				
				Log4Util.v(Device.TAG,
						"[CCPAudioRecorder - reset] " +  e.getMessage());
			}
			state = State.ERROR;
		}
	}

	/**
	 * Starts the recording, and sets the state to RECORDING. Call after
	 * prepare().
	 */
	public void start() {
		if (state == State.READY) {
			mAudioRecord.startRecording();
			
			try {
				Thread.sleep(100);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
			// To start recording, to read buffer, will inform the activation of listener.
			int	len = mAudioRecord.read(mBuffer, 0, mBuffer.length);
			state = State.RECORDING;
				
			if(AudioRecordManager.DEBUG_RECORD) {
				
				Log4Util.v(Device.TAG, "[CCPAudioRecorder - start] To start recording, first read the data length : " + len);
			}
		} else {
			if(AudioRecordManager.DEBUG_RECORD) {
				
				Log4Util.v(Device.TAG, 
					"[CCPAudioRecorder - start] start() called on illegal state");
			}
			state = State.ERROR;
		}
	}

	/**
	 * Stops the recording, and sets the state to STOPPED. In case of further
	 * usage, a reset is needed. Also finalizes the AMR file in case of
	 * uncompressed recording.
	 */
	public void stop() {
		if (state == State.RECORDING) {
			mAudioRecord.stop();
			mAudioRecord.setRecordPositionUpdateListener(null);
			state = State.STOPPED;
			encoder.setRunning(false);
			encoder.putData(new byte[0], 0);
			if(AudioRecordManager.DEBUG_RECORD) {
				Log4Util.v(Device.TAG,
					"[CCPAudioRecorder - stop] audioRecorder stop .");
			}
				
		}else {
			if(AudioRecordManager.DEBUG_RECORD) {
				
				Log4Util.v(Device.TAG,
					"[CCPAudioRecorder - stop]  stop() called on illegal state");
			}
			state = State.ERROR;
		}
	}

	/**
	 * Stops the recording, and sets the state to STOPPED. In case of further
	 * usage, a reset is needed. Also finalizes the AMR file in case of
	 * uncompressed recording.
	 */
	public void cancleEnable(boolean isCancle) {
		if (state == State.RECORDING && encoder != null ) {
			encoder.setIsCancle(isCancle);
		}
	}

	/*
	 * 
	 * Converts a byte[2] to a short, in LITTLE_ENDIAN format
	 */
	private short getShort(byte argB1, byte argB2) {
		return (short) (argB1 | (argB2 << 8));
	}

	
	/**
	 * INITIALIZING : recorder is initializing; 
	 * READY : recorder has been initialized, recorder not yet started 
	 * RECORDING : recording 
	 * ERROR :reconstruction needed 
	 * STOPPED: reset needed
	 */
	public enum State {
		INITIALIZING, READY, RECORDING, ERROR, STOPPED
	};
}