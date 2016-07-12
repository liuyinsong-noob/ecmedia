package com.hisun.phone.core.voice.multimedia;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

import com.hisun.phone.core.voice.CCPCallImpl;
import com.hisun.phone.core.voice.CallControlManager;
import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.RecordProduct;
import com.hisun.phone.core.voice.model.RecordProduct.RecordProductType;
import com.hisun.phone.core.voice.model.RecourProductInfo;
import com.hisun.phone.core.voice.util.Log4Util;

/**
 * @version Time: 2013-6-23
 */
public class AmrEncoder implements Runnable {

	private BlockingQueue<byte[]> blockingQueue = new LinkedBlockingQueue<byte[]>();
	
	private final Object mutex = new Object();
	private int frameSize;
	private byte[] encodData = new byte[1024];
	private byte[] netPackage =  new byte[650];
	private volatile boolean isRunning;
	private volatile boolean isCancle;
	private volatile boolean isChunked = false;
	
	private volatile boolean isSetAmrHead ;
	
	private CallControlManager mControlManager;

	// File writer (only in uncompressed mode)
	private RandomAccessFile randomAccessWriter;
	
	private RecourProductInfo info ;
	
	public AmrEncoder(RecourProductInfo info , boolean isChunked) {
		super();
		this.isChunked = isChunked;
		// write file header
		frameSize = 320;
		mControlManager = CCPCallImpl.getInstance().getCallControlManager();
		this.info = info ;
		
	}

	public void run() {

		android.os.Process
				.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);

		int netPackageLength = 0;
		while (this.isRunning()) {
			synchronized (mutex) {
				while (isIdle()) {
					try {
						mutex.wait();
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
				}
			}
			synchronized (mutex) {
				try {
					byte[] temp = new byte[frameSize];
					byte[] rawdata = blockingQueue.take();
					if(rawdata.length > 0) {
						for (int i = 0; i < (rawdata.length / frameSize); i++) {
							System.arraycopy(rawdata, i * frameSize, temp, 0, frameSize);
							
							// The PCM data recording code
							// There is a problem is when the incoming greater than 320 bytes length data coding error,
							// so must pass 320 length
							// Code is 14 bytes, need to remove the first byte, the first is invalid
							int length = mControlManager.AmrNBEncode(temp, frameSize, encodData, 0);
							
							if(length > 0) {
								//byte[] rbuffer = new byte[length - 1];
								System.arraycopy(encodData, 1, netPackage, netPackageLength, length - 1);
								
								netPackageLength += (length - 1);
								
							}
							if(netPackageLength < 650 && isRunning) {
								// Requirements for recording data reached 650 bytes 
								// sent more to go out (which is the second sampling data)
								continue ;
							}
							
							if(!isSetAmrHead) {
								
								if(isChunked) {
									
									// Network to send packets
									// The coded data packet and sending
									// If this is the first request, need to add request header and the data in AMR format information
									// Second do not need direct contract
									
									RecordProduct producer = new RecordProduct( RecordProductType.ProductStart);
									RecordBlockingQueue.getInstance().produce(producer);
									
									RecordProduct producerInfo = new RecordProduct( RecordProductType.ProductInfo);
									producerInfo.info = info;
									RecordBlockingQueue.getInstance().produce(producerInfo);
									
									RecordProduct producerdHead = new RecordProduct( RecordProductType.ProductData);
									producerdHead.data = CCPAudioRecorder.AMR_CODEC_HEAD;
									RecordBlockingQueue.getInstance() .produce(producerdHead);
								}
								
								try {
									// write file header
									randomAccessWriter = new RandomAccessFile(info.fileName,"rw");
									
									// Set file length to 0, to prevent unexpected behavior in case
									// the file already existed
									randomAccessWriter.setLength(0); 
									randomAccessWriter.write(CCPAudioRecorder.AMR_CODEC_HEAD);
								} catch (Exception e) {
									e.printStackTrace();
								}
								
								
								isSetAmrHead = true;
							} 
							
							byte[] amrData = new byte[netPackageLength];
							System.arraycopy(netPackage, 0, amrData, 0, netPackageLength);
							buildNetPackage(amrData , amrData.length);
							netPackageLength = 0;
						}
					} else {
						if(netPackageLength > 0 && isSetAmrHead) {
							byte[] amrData = new byte[netPackageLength];
							System.arraycopy(netPackage, 0, amrData, 0, netPackageLength);
							/*netPackageLength = */buildNetPackage(amrData , amrData.length);
							netPackageLength = 0;
						}
						
						try {
							if(randomAccessWriter != null ) {
								randomAccessWriter.close();
								if(AudioRecordManager.DEBUG_RECORD) {
									Log4Util.d(Device.TAG,
									"[AmrEncoder - run ] Read and write files, close reading and writing .");
								}
							}
						} catch (Exception e) {
							e.printStackTrace();
						}
						
					}
				}catch (IOException e) {
					e.printStackTrace();
					
					// BUG int v3.3 
					// If the file path of the local does not exist or the local SDcard does not exist, 
					// then fire to capture IO Exception, to give the netPackageLength initialization here, 
					// otherwise an exception will be thrown java.lang.ArrayIndexOutOfBoundsException
					netPackageLength = 0;
					if(AudioRecordManager.DEBUG_RECORD) {
						
						Log4Util.d(Device.TAG,
						"[AmrEncoder - run] I/O exception occured while closing output file");
					}
				} catch (InterruptedException e) {
					
					e.printStackTrace();
				}
			}

		
		}
		
		try {
			if(isSetAmrHead && isChunked) {
				
				if(!isCancle) {
					RecordProduct isOk = new RecordProduct( RecordProductType.ProductData);
					// When the normal end of recording (no manual trigger to
					// cancel the operation),
					// then at the end of data finally together with the
					// recording mark,
					// for the server to determine whether the normal end,
					
					// The end marker used to judge whether it is cancel
					// sending, or send failure is unknown.
					// cancle record , so cancle to send ..
					isOk.data = CCPAudioRecorder.AMR_STOP;
					if(AudioRecordManager.DEBUG_RECORD) {
						
						Log4Util.d(Device.TAG,
						"[AmrEncoder - run] record success , then begin to send end skip. ");
					}
					RecordBlockingQueue.getInstance().produce(isOk);
				} else {
					doCCPRecordVoiceError();
				}
				
				
				// Record audio data end, construction of the package at the end symbol
				RecordProduct producer = new RecordProduct( RecordProductType.ProductEnd);
				try {
					RecordBlockingQueue.getInstance().produce(producer);
					if(AudioRecordManager.DEBUG_RECORD) {
						
						Log4Util.d(Device.TAG, "[AmrEncoder - run] PCM recording audio data end, set up the network send end flag");
					}
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}

			if(AudioRecordManager.DEBUG_RECORD) {
				
				// isOk.data = "success".getBytes("UTF-8");
				// success record , so to send ..
				// do nothing ..
				Log4Util.d(Device.TAG,
					"[AmrEncoder - run] PCM AMR code is finished executing, complete the network transmission requests");
			}
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		if(AudioRecordManager.DEBUG_RECORD) {
			Log4Util.d(Device.TAG , "[AmrEncoder - run] AmrEncoder code end of execution");
		}
		isSetAmrHead = false;
		isChunked = false;
	}
	
	/**
	 * 
	 */
	private void doCCPRecordVoiceError() {
		RecordProduct isOk = new RecordProduct( RecordProductType.ProductData);
		isOk.data = CCPAudioRecorder.AMR_ERROR;
		if(AudioRecordManager.DEBUG_RECORD) {
			Log4Util.d(Device.TAG,
				"[AmrEncoder - run] record complete , cancel the send operation. ");
		}
	}

	private void buildNetPackage(byte[] amrData, int Length)
			throws InterruptedException, IOException {
		
		if(this.isChunked) {
			RecordProduct producer = new RecordProduct(RecordProductType.ProductData);
			
			producer.data = amrData;
			RecordBlockingQueue.getInstance().produce(producer);
		}
		
		try {
			// Write file, recording data is larger than 650 bytes to write data to a file
			randomAccessWriter.write(netPackage , 0 , Length);
			//netPackageLength = 0;
			//return netPackageLength;
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void putData(byte[] data, int size) {
		synchronized (mutex) {
			
			try {
				byte[] rawdata = new byte[size];
				System.arraycopy(data, 0, rawdata, 0, size);
				blockingQueue.put(rawdata);
				//this.leftSize = size;
				mutex.notify();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}

	public boolean isIdle() {
		return blockingQueue.size() == 0 ? true : false;
	}

	public void setIsCancle(boolean isCancle){
		this.isCancle = isCancle;
	}
	
	public void setRunning(boolean isRunning) {
		synchronized (mutex) {
			this.isRunning = isRunning;
			if (this.isRunning) {
				mutex.notify();
			}
		}
	}

	public boolean isRunning() {
		synchronized (mutex) {
			return isRunning;
		}
	}
}
